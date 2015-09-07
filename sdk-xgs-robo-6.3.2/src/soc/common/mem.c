/*
 * $Id: mem.c 1.455.2.3 Broadcom SDK $
 * $Copyright: Copyright 2012 Broadcom Corporation.
 * This program is the proprietary software of Broadcom Corporation
 * and/or its licensors, and may only be used, duplicated, modified
 * or distributed pursuant to the terms and conditions of a separate,
 * written license agreement executed between you and Broadcom
 * (an "Authorized License").  Except as set forth in an Authorized
 * License, Broadcom grants no license (express or implied), right
 * to use, or waiver of any kind with respect to the Software, and
 * Broadcom expressly reserves all rights in and to the Software
 * and all intellectual property rights therein.  IF YOU HAVE
 * NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS SOFTWARE
 * IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 * ALL USE OF THE SOFTWARE.  
 *  
 * Except as expressly set forth in the Authorized License,
 *  
 * 1.     This program, including its structure, sequence and organization,
 * constitutes the valuable trade secrets of Broadcom, and you shall use
 * all reasonable efforts to protect the confidentiality thereof,
 * and to use this information only in connection with your use of
 * Broadcom integrated circuit products.
 *  
 * 2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS
 * PROVIDED "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 * REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 * OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 * DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 * NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 * ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 * CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING
 * OUT OF USE OR PERFORMANCE OF THE SOFTWARE.
 * 
 * 3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL
 * BROADCOM OR ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL,
 * INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER
 * ARISING OUT OF OR IN ANY WAY RELATING TO YOUR USE OF OR INABILITY
 * TO USE THE SOFTWARE EVEN IF BROADCOM HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES; OR (ii) ANY AMOUNT IN EXCESS OF
 * THE AMOUNT ACTUALLY PAID FOR THE SOFTWARE ITSELF OR USD 1.00,
 * WHICHEVER IS GREATER. THESE LIMITATIONS SHALL APPLY NOTWITHSTANDING
 * ANY FAILURE OF ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.$
 *
 * SOC Memory (Table) Utilities
 */

#include <sal/core/libc.h>

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cmic.h>
#include <soc/error.h>
#include <soc/register.h>
#include <soc/drv.h>
#include <soc/enet.h>
#include <soc/hash.h>
#include <soc/l2x.h>
#include <soc/l3x.h>
#include <soc/lpm.h>
#include <soc/vlan.h>
#include <shared/fabric.h>
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif
#ifdef BCM_SBUSDMA_SUPPORT
#include <soc/sbusdma.h>
#endif
#ifdef BCM_TRIUMPH3_SUPPORT
#include <soc/ism.h>
#include <soc/ism_hash.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_HERCULES_SUPPORT)
#include <soc/hercules.h>
#endif
#if defined(BCM_TRIUMPH_SUPPORT)
#include <soc/triumph.h>
#include <soc/er_tcam.h>
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <soc/triumph2.h>
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
#include <soc/trident.h>
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_KATANA_SUPPORT) && defined(BCM_CMICM_SUPPORT)
#include <soc/katana.h>
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_HURRICANE2_SUPPORT) 
#include <soc/hurricane2.h>
#endif /* BCM_HURRICANE2_SUPPORT */
#if defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#include <soc/dpp/drv.h>
#include <soc/dfe/cmn/dfe_drv.h>
#endif

#define LPM_INDEX_4K_ENTRIES 4096
#define LPM_INDEX_2K_ENTRIES 2048
#define LPM_INDEX_1K_ENTRIES 1024

/*
 * This function pointer must be initialized by the application.
 * For the standard SDK this is done in diag_shell(), and
 * the function will point to sal_config_set() by default.
 */
int (*soc_mem_config_set)(char *name, char *value);

#if defined(BCM_PETRA_SUPPORT)
uint32    _soc_mem_entry_multicast_null[SOC_MAX_MEM_WORDS] = {0xffffffff, 0x7ffff};
uint32  _soc_mem_entry_llr_llvp_null[SOC_MAX_MEM_WORDS] = {0x90};
#endif /* BCM_PETRA_SUPPORT */

#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) ||\
    defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)  || defined(BCM_CALADAN3_SUPPORT)

/* extra delay for quickturn */
uint32 soc_mem_fifo_delay_value = 15 * MILLISECOND_USEC;

/*
 * Empty (null) table entries
 */
uint32 _soc_mem_entry_null_zeroes[SOC_MAX_MEM_WORDS];
/* First word f's, rest 0's */
uint32 _soc_mem_entry_null_word0[SOC_MAX_MEM_WORDS] = { ~0, 0 };

#define SOC_MEM_COMPARE_RETURN(a, b) {       \
        if ((a) < (b)) { return -1; }        \
        if ((a) > (b)) { return  1; }        \
}
#define CMIC_B0_DMA_READ_COMMAND   (0x7)
#define CMIC_B0_DMA_WRITE_COMMAND  (0x9)

#define _SOC_MEM_CHK_L2_MEM(mem) \
        (mem == L2Xm || mem == L2_ENTRY_1m || mem == L2_ENTRY_2m)
#define _SOC_MEM_CHK_L3_MEM(mem) \
        (mem == L3_ENTRY_ONLYm || mem == L3_ENTRY_IPV4_UNICASTm || \
         mem == L3_ENTRY_IPV4_MULTICASTm || mem == L3_ENTRY_IPV6_UNICASTm || \
         mem == L3_ENTRY_IPV6_MULTICASTm || mem == L3_ENTRY_1m || \
         mem == L3_ENTRY_2m || mem == L3_ENTRY_4m)
#define _SOC_MEM_CHK_EGR_IP_TUNNEL(mem) \
        (mem == EGR_IP_TUNNELm || mem == EGR_IP_TUNNEL_MPLSm || \
         mem == EGR_IP_TUNNEL_IPV6m)
/* Macro to swap name to the mapped-to (actually-used) mem state name */
#define _SOC_MEM_REUSE_MEM_STATE(unit, mem) {\
    switch(mem) {\
    case VLAN_XLATE_1m: mem = VLAN_XLATEm; break;\
    case EP_VLAN_XLATE_1m: mem = EGR_VLAN_XLATEm; break;\
    case MPLS_ENTRY_1m: mem = MPLS_ENTRYm; break;\
    case VLAN_MACm: \
        if (SOC_IS_TRX(unit) && !soc_feature(unit, soc_feature_ism_memory)) {\
            mem = VLAN_XLATEm;\
        }\
        break;\
    default: break;\
    }\
}

/* Routine to check if this is the mem whose state is being (mapped-to) actually-used by others */
int _SOC_MEM_IS_REUSED_MEM(int unit, soc_mem_t mem) {\
    switch(mem) {\
    case EGR_VLAN_XLATEm: return TRUE;\
    case MPLS_ENTRYm: return TRUE;\
    case VLAN_XLATEm:\
        if (SOC_IS_TRX(unit)) {\
            return TRUE;\
        }\
        break;\
    default: break;\
    }\
    return FALSE;\
}

/* Handle reverse mem state mapping updates - one to one or one to many */
#define _SOC_MEM_SYNC_MAPPED_MEM_STATES(unit, mem) {\
    switch(mem) {\
    case EGR_VLAN_XLATEm: \
        SOC_MEM_STATE(unit, EP_VLAN_XLATE_1m).cache[blk] = cache;\
        SOC_MEM_STATE(unit, EP_VLAN_XLATE_1m).vmap[blk] = vmap;\
        break;\
    case MPLS_ENTRYm: \
        SOC_MEM_STATE(unit, MPLS_ENTRY_1m).cache[blk] = cache;\
        SOC_MEM_STATE(unit, MPLS_ENTRY_1m).vmap[blk] = vmap;\
        break;\
    case VLAN_XLATEm: \
        if (SOC_IS_TRX(unit)) {\
            if (soc_feature(unit, soc_feature_ism_memory)) {\
                SOC_MEM_STATE(unit, VLAN_XLATE_1m).cache[blk] = cache;\
                SOC_MEM_STATE(unit, VLAN_XLATE_1m).vmap[blk] = vmap;\
            } else {\
                SOC_MEM_STATE(unit, VLAN_MACm).cache[blk] = cache;\
                SOC_MEM_STATE(unit, VLAN_MACm).vmap[blk] = vmap;\
            }\
            break;\
        }\
    default: break;\
    }\
}
#ifdef BCM_TRIUMPH3_SUPPORT
#define _SOC_MEM_REPLACE_MEM(unit, mem) {\
    if (soc_feature(unit, soc_feature_ism_memory)) {\
        switch(mem) {\
        case VLAN_XLATE_1m: mem = VLAN_XLATEm; break;\
        case EP_VLAN_XLATE_1m: mem = EGR_VLAN_XLATEm; break;\
        case MPLS_ENTRY_1m: mem = MPLS_ENTRYm; break;\
        default: break;\
        }\
    }\
}
#else
#define _SOC_MEM_REPLACE_MEM(unit, mem)
#endif /* BCM_TRIUMPH3_SUPPORT */

int
_soc_mem_cmp_word0(int unit, void *ent_a, void *ent_b)
{
    COMPILER_REFERENCE(unit);

    /*
     * Good for l3_def_ip_entry_t and l3_l3_entry_t.
     * The first uint32 (IP address) is the sort key.
     */

    SOC_MEM_COMPARE_RETURN(*(uint32 *)ent_a, *(uint32 *)ent_b);

    return 0;
}

int
_soc_mem_cmp_undef(int unit, void *ent_a, void *ent_b)
{
    COMPILER_REFERENCE(ent_a);
    COMPILER_REFERENCE(ent_b);

    soc_cm_print("soc_mem_cmp: cannot compare entries of this type\n");

    assert(0);

    return 0;
}

#ifdef BCM_XGS_SWITCH_SUPPORT

int
_soc_mem_cmp_l2x(int unit, void *ent_a, void *ent_b)
{
    sal_mac_addr_t mac_a, mac_b;
    vlan_id_t vlan_a, vlan_b;

    vlan_a = soc_L2Xm_field32_get(unit, ent_a, VLAN_IDf);
    vlan_b = soc_L2Xm_field32_get(unit, ent_b, VLAN_IDf);
    SOC_MEM_COMPARE_RETURN(vlan_a, vlan_b);

    soc_L2Xm_mac_addr_get(unit, ent_a, MAC_ADDRf, mac_a);
    soc_L2Xm_mac_addr_get(unit, ent_b, MAC_ADDRf, mac_b);

    return ENET_CMP_MACADDR(mac_a, mac_b);
}

int
_soc_mem_cmp_l2x_sync(int unit, void *ent_a, void *ent_b)
{
    l2x_entry_t     *l2x1 = (l2x_entry_t *)ent_a;
    l2x_entry_t     *l2x2 = (l2x_entry_t *)ent_b;

    /* force bits that may be updated by HW to be the same */
    if (SOC_IS_XGS3_SWITCH(unit) || SOC_IS_XGS3_FABRIC(unit)) {
        soc_L2Xm_field32_set(unit, l2x1, HITSAf, 0);
        soc_L2Xm_field32_set(unit, l2x2, HITSAf, 0);
        soc_L2Xm_field32_set(unit, l2x1, HITDAf, 0);
        soc_L2Xm_field32_set(unit, l2x2, HITDAf, 0);
        if (SOC_MEM_FIELD_VALID(unit, L2Xm, LOCAL_SAf)) {
            soc_L2Xm_field32_set(unit, l2x1, LOCAL_SAf, 0);
            soc_L2Xm_field32_set(unit, l2x2, LOCAL_SAf, 0);
        }
        if (SOC_MEM_FIELD_VALID(unit, L2Xm, EVEN_PARITYf)) {
            soc_L2Xm_field32_set(unit, l2x1, EVEN_PARITYf, 0);
            soc_L2Xm_field32_set(unit, l2x2, EVEN_PARITYf, 0);
        }
        if (SOC_MEM_FIELD_VALID(unit, L2Xm, ODD_PARITYf)) {
            soc_L2Xm_field32_set(unit, l2x1, ODD_PARITYf, 0);
            soc_L2Xm_field32_set(unit, l2x2, ODD_PARITYf, 0);
        }
    }

    return sal_memcmp(ent_a, ent_b, sizeof(l2x_entry_t));
}

#ifdef BCM_TRIUMPH3_SUPPORT
int
_soc_mem_cmp_tr3_l2x_sync(int unit, void *ent_a, void *ent_b, uint8 hit_bits)
{
    uint32 val;
    soc_mem_t mem_type = L2_ENTRY_1m;
    l2_entry_1_entry_t *l2_1_1, *l2_1_2;
    l2_entry_2_entry_t *l2_2_1, *l2_2_2;
    
    val = soc_mem_field32_get(unit, L2_ENTRY_1m, (l2_entry_1_entry_t *)ent_a, 
                              KEY_TYPEf);
    if ((val == SOC_MEM_KEY_L2_ENTRY_2_L2_BRIDGE) || 
        (val == SOC_MEM_KEY_L2_ENTRY_2_L2_VFI) ||
        (val == SOC_MEM_KEY_L2_ENTRY_2_L2_TRILL_NONUC_ACCESS)) {
        mem_type = L2_ENTRY_2m;
    }
    
    if (mem_type == L2_ENTRY_1m) {
        l2_1_1 = (l2_entry_1_entry_t *)ent_a;
        l2_1_2 = (l2_entry_1_entry_t *)ent_b;
        if (!(hit_bits & L2X_SHADOW_HIT_BITS)) {
            soc_L2_ENTRY_1m_field32_set(unit, l2_1_1, HITSAf, 0);
            soc_L2_ENTRY_1m_field32_set(unit, l2_1_2, HITSAf, 0);
            soc_L2_ENTRY_1m_field32_set(unit, l2_1_1, HITDAf, 0);
            soc_L2_ENTRY_1m_field32_set(unit, l2_1_2, HITDAf, 0);
        } else {
            if (!(hit_bits & L2X_SHADOW_HIT_SRC)) {
                soc_L2_ENTRY_1m_field32_set(unit, l2_1_1, HITSAf, 0);
                soc_L2_ENTRY_1m_field32_set(unit, l2_1_2, HITSAf, 0);
            }
            if (!(hit_bits & L2X_SHADOW_HIT_DST)) {
                soc_L2_ENTRY_1m_field32_set(unit, l2_1_1, HITDAf, 0);
                soc_L2_ENTRY_1m_field32_set(unit, l2_1_2, HITDAf, 0);
            }
        }
        soc_L2_ENTRY_1m_field32_set(unit, l2_1_1, LOCAL_SAf, 0);
        soc_L2_ENTRY_1m_field32_set(unit, l2_1_2, LOCAL_SAf, 0);
        soc_L2_ENTRY_1m_field32_set(unit, l2_1_1, EVEN_PARITYf, 0);
        soc_L2_ENTRY_1m_field32_set(unit, l2_1_2, EVEN_PARITYf, 0);
        return sal_memcmp(ent_a, ent_b, sizeof(l2_entry_1_entry_t));
    } else {
        l2_2_1 = (l2_entry_2_entry_t *)ent_a;
        l2_2_2 = (l2_entry_2_entry_t *)ent_b;
        if (!(hit_bits & L2X_SHADOW_HIT_BITS)) {
            soc_L2_ENTRY_2m_field32_set(unit, l2_2_1, HITSAf, 0);
            soc_L2_ENTRY_2m_field32_set(unit, l2_2_2, HITSAf, 0);
            soc_L2_ENTRY_2m_field32_set(unit, l2_2_1, HITDAf, 0);
            soc_L2_ENTRY_2m_field32_set(unit, l2_2_2, HITDAf, 0);
        } else {
            if (!(hit_bits & L2X_SHADOW_HIT_SRC)) {
                soc_L2_ENTRY_2m_field32_set(unit, l2_2_1, HITSAf, 0);
                soc_L2_ENTRY_2m_field32_set(unit, l2_2_2, HITSAf, 0);
            }
            if (!(hit_bits & L2X_SHADOW_HIT_DST)) {
                soc_L2_ENTRY_2m_field32_set(unit, l2_2_1, HITDAf, 0);
                soc_L2_ENTRY_2m_field32_set(unit, l2_2_2, HITDAf, 0);
            }
        }
        soc_L2_ENTRY_2m_field32_set(unit, l2_2_1, LOCAL_SAf, 0);
        soc_L2_ENTRY_2m_field32_set(unit, l2_2_2, LOCAL_SAf, 0);
        soc_L2_ENTRY_2m_field32_set(unit, l2_2_1, EVEN_PARITY_0f, 0);
        soc_L2_ENTRY_2m_field32_set(unit, l2_2_2, EVEN_PARITY_0f, 0);
        soc_L2_ENTRY_2m_field32_set(unit, l2_2_1, EVEN_PARITY_1f, 0);
        soc_L2_ENTRY_2m_field32_set(unit, l2_2_2, EVEN_PARITY_1f, 0);
        return sal_memcmp(ent_a, ent_b, sizeof(l2_entry_2_entry_t));
    }
}

int
_soc_mem_cmp_tr3_ext_l2x_1_sync(int unit, void *ent_a, void *ent_b, 
                                uint8 hit_bits)
{
    ext_l2_entry_1_entry_t *ext_l2_1_1, *ext_l2_1_2;
    ext_l2_1_1 = (ext_l2_entry_1_entry_t *)ent_a;
    ext_l2_1_2 = (ext_l2_entry_1_entry_t *)ent_b;
    if (!(hit_bits & L2X_SHADOW_HIT_BITS)) {
        soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_1, HITSAf, 0);
        soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_2, HITSAf, 0);
        soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_1, HITDAf, 0);
        soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_2, HITDAf, 0);
    } else {
        if (!(hit_bits & L2X_SHADOW_HIT_SRC)) {
            soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_1, HITSAf, 0);
            soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_2, HITSAf, 0);
        }
        if (!(hit_bits & L2X_SHADOW_HIT_DST)) {
            soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_1, HITDAf, 0);
            soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_2, HITDAf, 0);
        }
    }
    soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_1, LOCAL_SAf, 0);
    soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_2, LOCAL_SAf, 0);
    soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_1, EVEN_PARITYf, 0);
    soc_EXT_L2_ENTRY_1m_field32_set(unit, ext_l2_1_2, EVEN_PARITYf, 0);
    return sal_memcmp(ent_a, ent_b, sizeof(ext_l2_entry_1_entry_t));
}

int
_soc_mem_cmp_tr3_ext_l2x_2_sync(int unit, void *ent_a, void *ent_b, 
                                uint8 hit_bits)
{
    ext_l2_entry_2_entry_t *ext_l2_2_1, *ext_l2_2_2;
    ext_l2_2_1 = (ext_l2_entry_2_entry_t *)ent_a;
    ext_l2_2_2 = (ext_l2_entry_2_entry_t *)ent_b;
    if (!(hit_bits & L2X_SHADOW_HIT_BITS)) {
        soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_1, HITSAf, 0);
        soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_2, HITSAf, 0);
        soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_1, HITDAf, 0);
        soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_2, HITDAf, 0);
    } else {
        if (!(hit_bits & L2X_SHADOW_HIT_SRC)) {
            soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_1, HITSAf, 0);
            soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_2, HITSAf, 0);
        }
        if (!(hit_bits & L2X_SHADOW_HIT_DST)) {
            soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_1, HITDAf, 0);
            soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_2, HITDAf, 0);
        }
    }
    soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_1, LOCAL_SAf, 0);
    soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_2, LOCAL_SAf, 0);
    soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_1, EVEN_PARITYf, 0);
    soc_EXT_L2_ENTRY_2m_field32_set(unit, ext_l2_2_2, EVEN_PARITYf, 0);
    return sal_memcmp(ent_a, ent_b, sizeof(ext_l2_entry_2_entry_t));
}

int
_soc_mem_cmp_tr3_l2x(int unit, void *ent_a, void *ent_b)
{
    uint32 val_a, val_b;
    sal_mac_addr_t mac_a, mac_b;

    val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a, KEY_TYPEf);
    val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b, KEY_TYPEf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    switch (val_a) {
    case SOC_MEM_KEY_L2_ENTRY_1_L2_BRIDGE: /* BRIDGE */
        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a, L2__VLAN_IDf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b, L2__VLAN_IDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        soc_L2_ENTRY_1m_mac_addr_get(unit, ent_a, L2__MAC_ADDRf, mac_a);
        soc_L2_ENTRY_1m_mac_addr_get(unit, ent_b, L2__MAC_ADDRf, mac_b);
        return ENET_CMP_MACADDR(mac_a, mac_b);

    case SOC_MEM_KEY_L2_ENTRY_2_L2_BRIDGE: /* BRIDGE */
        val_a = soc_L2_ENTRY_2m_field32_get(unit, ent_a, L2__VLAN_IDf);
        val_b = soc_L2_ENTRY_2m_field32_get(unit, ent_b, L2__VLAN_IDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        soc_L2_ENTRY_2m_mac_addr_get(unit, ent_a, L2__MAC_ADDRf, mac_a);
        soc_L2_ENTRY_2m_mac_addr_get(unit, ent_b, L2__MAC_ADDRf, mac_b);
        return ENET_CMP_MACADDR(mac_a, mac_b);
        
    case SOC_MEM_KEY_L2_ENTRY_1_L2_VFI: /* VFI */
        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a, L2__VFIf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b, L2__VFIf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        soc_L2_ENTRY_1m_mac_addr_get(unit, ent_a, L2__MAC_ADDRf, mac_a);
        soc_L2_ENTRY_1m_mac_addr_get(unit, ent_b, L2__MAC_ADDRf, mac_b);
        return ENET_CMP_MACADDR(mac_a, mac_b);

    case SOC_MEM_KEY_L2_ENTRY_2_L2_VFI: /* VFI */
        val_a = soc_L2_ENTRY_2m_field32_get(unit, ent_a, L2__VFIf);
        val_b = soc_L2_ENTRY_2m_field32_get(unit, ent_b, L2__VFIf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        soc_L2_ENTRY_2m_mac_addr_get(unit, ent_a, L2__MAC_ADDRf, mac_a);
        soc_L2_ENTRY_2m_mac_addr_get(unit, ent_b, L2__MAC_ADDRf, mac_b);
        return ENET_CMP_MACADDR(mac_a, mac_b);

    case SOC_MEM_KEY_L2_ENTRY_1_VLAN_SINGLE_CROSS_CONNECT: /* SINGLE_CROSS_CONNECT */
        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a, VLAN__OVIDf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b, VLAN__OVIDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);
        return 0;
        
    case SOC_MEM_KEY_L2_ENTRY_1_VLAN_DOUBLE_CROSS_CONNECT: /* DOUBLE_CROSS_CONNECT */
        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a, VLAN__OVIDf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b, VLAN__OVIDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a, VLAN__IVIDf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b, VLAN__IVIDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);
        return 0;

    case SOC_MEM_KEY_L2_ENTRY_1_VIF_VIF: /* VIF */
        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a, VIF__NAMESPACEf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b, VIF__NAMESPACEf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a, VIF__DST_VIFf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b, VIF__DST_VIFf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a, VIF__Pf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b, VIF__Pf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);
        return 0;

    case SOC_MEM_KEY_L2_ENTRY_2_L2_TRILL_NONUC_ACCESS: /* TRILL_NONUC_ACCESS */
        val_a = soc_L2_ENTRY_2m_field32_get(unit, ent_a,
                                            L2__VLAN_IDf);
        val_b = soc_L2_ENTRY_2m_field32_get(unit, ent_b,
                                            L2__VLAN_IDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        soc_L2_ENTRY_2m_mac_addr_get(unit, ent_a, L2__MAC_ADDRf,
                                     mac_a);
        soc_L2_ENTRY_2m_mac_addr_get(unit, ent_b, L2__MAC_ADDRf,
                                     mac_b);
        return ENET_CMP_MACADDR(mac_a, mac_b);

    case SOC_MEM_KEY_L2_ENTRY_1_TRILL_NONUC_NETWORK_LONG_TRILL_NONUC_NETWORK_LONG: /* TRILL_NONUC_NETWORK_LONG */
        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a,
                                            TRILL_NONUC_NETWORK_LONG__VLAN_IDf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b,
                                            TRILL_NONUC_NETWORK_LONG__VLAN_IDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a,
                                            TRILL_NONUC_NETWORK_LONG__TREE_IDf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b,
                                            TRILL_NONUC_NETWORK_LONG__TREE_IDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        soc_L2_ENTRY_1m_mac_addr_get
            (unit, ent_a, TRILL_NONUC_NETWORK_LONG__MAC_ADDRESSf, mac_a);
        soc_L2_ENTRY_1m_mac_addr_get
            (unit, ent_b, TRILL_NONUC_NETWORK_LONG__MAC_ADDRESSf, mac_b);
        return ENET_CMP_MACADDR(mac_a, mac_b);

    case SOC_MEM_KEY_L2_ENTRY_1_TRILL_NONUC_NETWORK_SHORT_TRILL_NONUC_NETWORK_SHORT: /* TRILL_NONUC_NETWORK_SHORT */
        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a,
                                            TRILL_NONUC_NETWORK_SHORT__VLAN_IDf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b,
                                            TRILL_NONUC_NETWORK_SHORT__VLAN_IDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a,
                                            TRILL_NONUC_NETWORK_SHORT__TREE_IDf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b,
                                            TRILL_NONUC_NETWORK_SHORT__TREE_IDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);
        return 0;

    case SOC_MEM_KEY_L2_ENTRY_1_BFD_BFD: /* BFD */
        val_a = soc_L2_ENTRY_1m_field32_get(unit, ent_a, BFD__YOUR_DISCRIMINATORf);
        val_b = soc_L2_ENTRY_1m_field32_get(unit, ent_b, BFD__YOUR_DISCRIMINATORf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);
        return 0;
    default:
        return 1;
    }
}

int
_soc_mem_cmp_tr3_ext_l2x(int unit, void *ent_a, void *ent_b)
{
    uint32 val_a, val_b;
    sal_mac_addr_t mac_a, mac_b;

    val_a = soc_EXT_L2_ENTRY_1m_field32_get(unit, ent_a, KEY_TYPEf);
    val_b = soc_EXT_L2_ENTRY_1m_field32_get(unit, ent_b, KEY_TYPEf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    if (val_a) { /* VFI */
        val_a = soc_EXT_L2_ENTRY_1m_field32_get(unit, ent_a, VFIf);
        val_b = soc_EXT_L2_ENTRY_1m_field32_get(unit, ent_b, VFIf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);
        
        soc_EXT_L2_ENTRY_1m_mac_addr_get(unit, ent_a, MAC_ADDRf, mac_a);
        soc_EXT_L2_ENTRY_1m_mac_addr_get(unit, ent_b, MAC_ADDRf, mac_b);        
        return ENET_CMP_MACADDR(mac_a, mac_b);    
    } else { /* BRIDGE */
        val_a = soc_EXT_L2_ENTRY_1m_field32_get(unit, ent_a, VLAN_IDf);
        val_b = soc_EXT_L2_ENTRY_1m_field32_get(unit, ent_b, VLAN_IDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        soc_EXT_L2_ENTRY_1m_mac_addr_get(unit, ent_a, MAC_ADDRf, mac_a);
        soc_EXT_L2_ENTRY_1m_mac_addr_get(unit, ent_b, MAC_ADDRf, mac_b);
        return ENET_CMP_MACADDR(mac_a, mac_b);
    }
}
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
int
_soc_mem_cmp_l2x2(int unit, void *ent_a, void *ent_b)
{
    sal_mac_addr_t mac_a, mac_b;
    uint32 val_a, val_b;

    if (SOC_MEM_FIELD_VALID(unit, L2Xm, ENTRY_TYPEf)) {
        val_a = soc_L2Xm_field32_get(unit, ent_a, KEY_TYPEf);
        val_b = soc_L2Xm_field32_get(unit, ent_b, KEY_TYPEf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        switch (val_a) {
        case 0: /* BRIDGE */
            val_a = soc_L2Xm_field32_get(unit, ent_a, VLAN_IDf);
            val_b = soc_L2Xm_field32_get(unit, ent_b, VLAN_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            soc_L2Xm_mac_addr_get(unit, ent_a, MAC_ADDRf, mac_a);
            soc_L2Xm_mac_addr_get(unit, ent_b, MAC_ADDRf, mac_b);
            return ENET_CMP_MACADDR(mac_a, mac_b);

        case 1: /* SINGLE_CROSS_CONNECT */
            val_a = soc_L2Xm_field32_get(unit, ent_a, OVIDf);
            val_b = soc_L2Xm_field32_get(unit, ent_b, OVIDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        case 2: /* DOUBLE_CROSS_CONNECT */
            val_a = soc_L2Xm_field32_get(unit, ent_a, OVIDf);
            val_b = soc_L2Xm_field32_get(unit, ent_b, OVIDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_L2Xm_field32_get(unit, ent_a, IVIDf);
            val_b = soc_L2Xm_field32_get(unit, ent_b, IVIDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;

        case 3: /* VFI */
            val_a = soc_L2Xm_field32_get(unit, ent_a, VFIf);
            val_b = soc_L2Xm_field32_get(unit, ent_b, VFIf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            soc_L2Xm_mac_addr_get(unit, ent_a, MAC_ADDRf, mac_a);
            soc_L2Xm_mac_addr_get(unit, ent_b, MAC_ADDRf, mac_b);
            return ENET_CMP_MACADDR(mac_a, mac_b);

        case 4: /* VIF */
            val_a = soc_L2Xm_field32_get(unit, ent_a, VIF__NAMESPACEf);
            val_b = soc_L2Xm_field32_get(unit, ent_b, VIF__NAMESPACEf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_L2Xm_field32_get(unit, ent_a, VIF__DST_VIFf);
            val_b = soc_L2Xm_field32_get(unit, ent_b, VIF__DST_VIFf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_L2Xm_field32_get(unit, ent_a, VIF__Pf);
            val_b = soc_L2Xm_field32_get(unit, ent_b, VIF__Pf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;

        case 5: /* TRILL_NONUC_ACCESS */
            val_a = soc_L2Xm_field32_get(unit, ent_a,
                                         TRILL_NONUC_ACCESS__VLAN_IDf);
            val_b = soc_L2Xm_field32_get(unit, ent_b,
                                         TRILL_NONUC_ACCESS__VLAN_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            soc_L2Xm_mac_addr_get(unit, ent_a, TRILL_NONUC_ACCESS__MAC_ADDRf,
                                  mac_a);
            soc_L2Xm_mac_addr_get(unit, ent_b, TRILL_NONUC_ACCESS__MAC_ADDRf,
                                  mac_b);
            return ENET_CMP_MACADDR(mac_a, mac_b);

        case 6: /* TRILL_NONUC_NETWORK_LONG */
            val_a = soc_L2Xm_field32_get(unit, ent_a,
                                         TRILL_NONUC_NETWORK_LONG__VLAN_IDf);
            val_b = soc_L2Xm_field32_get(unit, ent_b,
                                         TRILL_NONUC_NETWORK_LONG__VLAN_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_L2Xm_field32_get(unit, ent_a,
                                         TRILL_NONUC_NETWORK_LONG__TREE_IDf);
            val_b = soc_L2Xm_field32_get(unit, ent_b,
                                         TRILL_NONUC_NETWORK_LONG__TREE_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            soc_L2Xm_mac_addr_get
                (unit, ent_a, TRILL_NONUC_NETWORK_LONG__MAC_ADDRESSf, mac_a);
            soc_L2Xm_mac_addr_get
                (unit, ent_b, TRILL_NONUC_NETWORK_LONG__MAC_ADDRESSf, mac_b);
            return ENET_CMP_MACADDR(mac_a, mac_b);

        case 7: /* TRILL_NONUC_NETWORK_SHORT */
            val_a = soc_L2Xm_field32_get(unit, ent_a,
                                         TRILL_NONUC_NETWORK_SHORT__VLAN_IDf);
            val_b = soc_L2Xm_field32_get(unit, ent_b,
                                         TRILL_NONUC_NETWORK_SHORT__VLAN_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_L2Xm_field32_get(unit, ent_a,
                                         TRILL_NONUC_NETWORK_SHORT__TREE_IDf);
            val_b = soc_L2Xm_field32_get(unit, ent_b,
                                         TRILL_NONUC_NETWORK_SHORT__TREE_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        default:
            return 1;
        }
    }

    val_a = soc_L2Xm_field32_get(unit, ent_a, VLAN_IDf);
    val_b = soc_L2Xm_field32_get(unit, ent_b, VLAN_IDf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    soc_L2Xm_mac_addr_get(unit, ent_a, MAC_ADDRf, mac_a);
    soc_L2Xm_mac_addr_get(unit, ent_b, MAC_ADDRf, mac_b);
    return ENET_CMP_MACADDR(mac_a, mac_b);
}

int
_soc_mem_cmp_l3x2(int unit, void *ent_a, void *ent_b)
{
    uint32 val_a, val_b;

    if (!SOC_MEM_FIELD_VALID(unit, L3_ENTRY_ONLYm, KEY_TYPEf)) {
        return 0;
    }

    val_a = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_a, KEY_TYPEf);
    val_b = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_b, KEY_TYPEf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        uint32 buf_a[SOC_MAX_MEM_FIELD_WORDS];
        uint32 buf_b[SOC_MAX_MEM_FIELD_WORDS];
        switch (val_a) {
        case TD2_L3_HASH_KEY_TYPE_V4UC:
            return _soc_mem_cmp_l3x2_ip4ucast(unit, ent_a, ent_b);
        case TD2_L3_HASH_KEY_TYPE_V4UC_EXT:
            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        KEY_TYPE_1f);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        KEY_TYPE_1f);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        IPV4UC_EXT__IP_ADDRf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        IPV4UC_EXT__IP_ADDRf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        IPV4UC_EXT__VRF_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        IPV4UC_EXT__VRF_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        case TD2_L3_HASH_KEY_TYPE_V4MC:
            return _soc_mem_cmp_l3x2_ip4mcast(unit, ent_a, ent_b); 
        case TD2_L3_HASH_KEY_TYPE_V6UC:
            return _soc_mem_cmp_l3x2_ip6ucast(unit, ent_a, ent_b);
        case TD2_L3_HASH_KEY_TYPE_V6UC_EXT:
            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_a,
                                        KEY_TYPE_1f);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_b,
                                        KEY_TYPE_1f);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_a,
                                        KEY_TYPE_2f);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_b,
                                        KEY_TYPE_2f);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_a,
                                        KEY_TYPE_3f);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_b,
                                        KEY_TYPE_3f);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            soc_mem_field_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_a,
                              IPV6UC_EXT__IP_ADDR_LWR_64f, buf_a);
            soc_mem_field_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_b,
                              IPV6UC_EXT__IP_ADDR_LWR_64f, buf_b);
            SOC_MEM_COMPARE_RETURN(buf_a[0], buf_b[0]);
            SOC_MEM_COMPARE_RETURN(buf_a[1], buf_b[1]);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_a,
                                        IPV6UC_EXT__VRF_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_b,
                                        IPV6UC_EXT__VRF_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            soc_mem_field_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_a,
                              IPV6UC_EXT__IP_ADDR_UPR_64f, buf_a);
            soc_mem_field_get(unit, L3_ENTRY_IPV6_MULTICASTm, ent_b,
                              IPV6UC_EXT__IP_ADDR_UPR_64f, buf_b);
            SOC_MEM_COMPARE_RETURN(buf_a[0], buf_b[0]);
            SOC_MEM_COMPARE_RETURN(buf_a[1], buf_b[1]);

            return 0;
        case TD2_L3_HASH_KEY_TYPE_V6MC:
            return _soc_mem_cmp_l3x2_ip6mcast(unit, ent_a, ent_b);

        case TD2_L3_HASH_KEY_TYPE_TRILL:
            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_a,
                                        TRILL__INGRESS_RBRIDGE_NICKNAMEf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_b,
                                        TRILL__INGRESS_RBRIDGE_NICKNAMEf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_a,
                                        TRILL__TREE_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_b,
                                        TRILL__TREE_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN:
            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_a,
                                        FCOE__MASKED_D_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_b,
                                        FCOE__MASKED_D_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_a,
                                        FCOE__VRF_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_b,
                                        FCOE__VRF_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN_EXT:
            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        KEY_TYPE_1f);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        KEY_TYPE_1f);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        FCOE_EXT__MASKED_D_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        FCOE_EXT__MASKED_D_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        FCOE_EXT__VRF_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        FCOE_EXT__VRF_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        case TD2_L3_HASH_KEY_TYPE_FCOE_HOST:
            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_a,
                                        FCOE__D_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_b,
                                        FCOE__D_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_a,
                                        FCOE__VRF_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_b,
                                        FCOE__VRF_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        case TD2_L3_HASH_KEY_TYPE_FCOE_HOST_EXT:
            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        KEY_TYPE_1f);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        KEY_TYPE_1f);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm,  ent_a,
                                        FCOE_EXT__D_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm,  ent_b,
                                        FCOE_EXT__D_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm,  ent_a,
                                        FCOE_EXT__VRF_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm,  ent_b,
                                        FCOE_EXT__VRF_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP:
            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_a,
                                        FCOE__S_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_b,
                                        FCOE__S_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_a,
                                        FCOE__VRF_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm, ent_b,
                                        FCOE__VRF_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP_EXT:
            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        KEY_TYPE_1f);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        KEY_TYPE_1f);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm,  ent_a,
                                        FCOE_EXT__S_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm,  ent_b,
                                        FCOE_EXT__S_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm,  ent_a,
                                        FCOE_EXT__VRF_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm,  ent_b,
                                        FCOE_EXT__VRF_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        case TD2_L3_HASH_KEY_TYPE_DST_NAT:
            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        KEY_TYPE_1f);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        KEY_TYPE_1f);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        NAT__VRF_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        NAT__VRF_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        NAT__IP_ADDRf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        NAT__IP_ADDRf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        case TD2_L3_HASH_KEY_TYPE_DST_NAPT:
            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        KEY_TYPE_1f);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        KEY_TYPE_1f);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        NAT__VRF_IDf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        NAT__VRF_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        NAT__IP_ADDRf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        NAT__IP_ADDRf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_a,
                                        NAT__L4_DEST_PORTf);
            val_b = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm, ent_b,
                                        NAT__L4_DEST_PORTf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        default:
            return 1;
        }
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
    switch (val_a) {
    case 0: /* IPV4_UNICAST */
        return _soc_mem_cmp_l3x2_ip4ucast(unit, ent_a, ent_b);
    case 1: /* IPV4_MULTICAST */
        return _soc_mem_cmp_l3x2_ip4mcast(unit, ent_a, ent_b); 
    case 2: /* IPV6_UNICAST */
        return _soc_mem_cmp_l3x2_ip6ucast(unit, ent_a, ent_b);
    case 3: /* IPV6_MULTICAST */
        return _soc_mem_cmp_l3x2_ip6mcast(unit, ent_a, ent_b);
    case 4: /* CCM_LMEP */
        val_a = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_a, LMEP__SGLPf);
        val_b = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_b, LMEP__SGLPf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_a, LMEP__VIDf);
        val_b = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_b, LMEP__VIDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        return 0;
    case 5: /* CCM_RMEP */
        val_a = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_a, RMEP__SGLPf);
        val_b = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_b, RMEP__SGLPf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_a, RMEP__VIDf);
        val_b = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_b, RMEP__VIDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_a, RMEP__MDLf);
        val_b = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_b, RMEP__MDLf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_a, RMEP__MEPIDf);
        val_b = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_b, RMEP__MEPIDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        return 0;
    case 6: /* TRILL */
        val_a = soc_L3_ENTRY_ONLYm_field32_get
            (unit, ent_a, TRILL__INGRESS_RBRIDGE_NICKNAMEf);
        val_b = soc_L3_ENTRY_ONLYm_field32_get
            (unit, ent_b, TRILL__INGRESS_RBRIDGE_NICKNAMEf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_a, TRILL__TREE_IDf);
        val_b = soc_L3_ENTRY_ONLYm_field32_get(unit, ent_b, TRILL__TREE_IDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        return 0;
    default:
        return 1;
    }
    }
}

int
_soc_mem_cmp_l3x2_ip4ucast(int unit, void *ent_a, void *ent_b)
{
    uint32      type_a, type_b;
    ip_addr_t    ip_a, ip_b;

    if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_IPV4_UNICASTm, VRF_IDf)) {
        type_a = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, ent_a, VRF_IDf);
        type_b = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, ent_b, VRF_IDf);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

    if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_IPV4_UNICASTm, KEY_TYPEf)) {
        type_a = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, ent_a, KEY_TYPEf);
        type_b = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, ent_b, KEY_TYPEf);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    } else {
        type_a = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, ent_a, V6f);
        type_b = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, ent_b, V6f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);

        type_a = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, ent_a, IPMCf);
        type_b = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, ent_b, IPMCf);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

    ip_a = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, ent_a, IP_ADDRf);
    ip_b = soc_L3_ENTRY_IPV4_UNICASTm_field32_get(unit, ent_b, IP_ADDRf);
    SOC_MEM_COMPARE_RETURN(ip_a, ip_b);

    return(0);
}

int
_soc_mem_cmp_l3x2_ip4mcast(int unit, void *ent_a, void *ent_b)
{
    uint32      type_a, type_b;
    ip_addr_t    a, b;
    vlan_id_t    vlan_a, vlan_b;

    if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_IPV4_MULTICASTm, VRF_IDf)) {
        type_a = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_a, VRF_IDf);
        type_b = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_b, VRF_IDf);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }
#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_IPV4_MULTICASTm, KEY_TYPE_0f)) {
        type_a = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_a, KEY_TYPE_0f);
        type_b = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_b, KEY_TYPE_0f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);

        type_a = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_a, KEY_TYPE_1f);
        type_b = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_b, KEY_TYPE_1f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    } else
#endif /* BCM_TRX_SUPPORT */
    {
        type_a = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_a, V6f);
        type_b = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_b, V6f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);

        type_a = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_a, IPMCf);
        type_b = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_b, IPMCf);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

    a = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_a, SOURCE_IP_ADDRf);
    b = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_b, SOURCE_IP_ADDRf);
    SOC_MEM_COMPARE_RETURN(a, b);

    a = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_a, GROUP_IP_ADDRf);
    b = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_b, GROUP_IP_ADDRf);
    SOC_MEM_COMPARE_RETURN(a, b);

    vlan_a = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_a, VLAN_IDf);
    vlan_b = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, ent_b, VLAN_IDf);
    SOC_MEM_COMPARE_RETURN(vlan_a, vlan_b);

    return(0);
}

int
_soc_mem_cmp_l3x2_ip6ucast(int unit, void *ent_a, void *ent_b)
{
    uint32      type_a, type_b;
    uint32      a[SOC_MAX_MEM_FIELD_WORDS];
    uint32      b[SOC_MAX_MEM_FIELD_WORDS];
    int         i;
    int         ent_words;

#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_IPV6_UNICASTm, VRF_IDf)) {
        type_a =
            soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_a, VRF_IDf);
        type_b =
            soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_b, VRF_IDf);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    } else
#endif
    if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_IPV6_UNICASTm, VRF_ID_0f)) {
        type_a =
            soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_a, VRF_ID_0f);
        type_b =
            soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_b, VRF_ID_0f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }
#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_IPV6_UNICASTm, KEY_TYPE_0f)) {
        type_a = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_a, KEY_TYPE_0f);
        type_b = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_b, KEY_TYPE_0f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    } else
#endif /* BCM_TRX_SUPPORT */
    {
        type_a = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_a, V6_0f);
        type_b = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_b, V6_0f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);

        type_a = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_a, IPMC_0f);
        type_b = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_b, IPMC_0f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

    if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_IPV6_UNICASTm, VRF_ID_1f)) {
        type_a =
            soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_a, VRF_ID_1f);
        type_b =
            soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_b, VRF_ID_1f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_IPV6_UNICASTm, KEY_TYPE_1f)) {
        type_a = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_a, KEY_TYPE_1f);
        type_b = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_b, KEY_TYPE_1f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    } else
#endif /* BCM_TRX_SUPPORT */
    {
        type_a = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_a, V6_1f);
        type_b = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_b, V6_1f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);

        type_a = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_a, IPMC_1f);
        type_b = soc_L3_ENTRY_IPV6_UNICASTm_field32_get(unit, ent_b, IPMC_1f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

    soc_mem_field_get(unit, L3_ENTRY_IPV6_UNICASTm, ent_a, IP_ADDR_UPR_64f, a);
    soc_mem_field_get(unit, L3_ENTRY_IPV6_UNICASTm, ent_b, IP_ADDR_UPR_64f, b);
    ent_words = soc_mem_field_length(unit, L3_ENTRY_IPV6_UNICASTm,
                                     IP_ADDR_UPR_64f) / 32;
    for (i = ent_words - 1; i >= 0; i--) {
        SOC_MEM_COMPARE_RETURN(a[i], b[i]);
    }
    soc_mem_field_get(unit, L3_ENTRY_IPV6_UNICASTm, ent_a, IP_ADDR_LWR_64f, a);
    soc_mem_field_get(unit, L3_ENTRY_IPV6_UNICASTm, ent_b, IP_ADDR_LWR_64f, b);
    ent_words = soc_mem_field_length(unit, L3_ENTRY_IPV6_UNICASTm,
                                     IP_ADDR_LWR_64f) / 32;
    for (i = ent_words - 1; i >= 0; i--) {
        SOC_MEM_COMPARE_RETURN(a[i], b[i]);
    }

    return(0);
}

int
_soc_mem_cmp_l3x2_ip6mcast(int u, void *e_a, void *e_b)
{
    uint32      type_a, type_b;
    vlan_id_t    vlan_a, vlan_b;
    uint32      a[SOC_MAX_MEM_FIELD_WORDS];
    uint32      b[SOC_MAX_MEM_FIELD_WORDS];
    int         i;
    int         ent_words;

#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_MEM_FIELD_VALID(u, L3_ENTRY_IPV6_MULTICASTm, VRF_IDf)) {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, VRF_IDf);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, VRF_IDf);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    } else
#endif
    if (SOC_MEM_FIELD_VALID(u, L3_ENTRY_IPV6_MULTICASTm, VRF_ID_0f)) {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, VRF_ID_0f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, VRF_ID_0f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }
#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_FIELD_VALID(u, L3_ENTRY_IPV6_MULTICASTm, KEY_TYPE_0f)) {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, KEY_TYPE_0f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, KEY_TYPE_0f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    } else
#endif /* BCM_TRX_SUPPORT */
    {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, V6_0f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, V6_0f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);

        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, IPMC_0f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, IPMC_0f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

    if (SOC_MEM_FIELD_VALID(u, L3_ENTRY_IPV6_MULTICASTm, VRF_ID_1f)) {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, VRF_ID_1f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, VRF_ID_1f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_FIELD_VALID(u, L3_ENTRY_IPV6_MULTICASTm, KEY_TYPE_1f)) {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, KEY_TYPE_1f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, KEY_TYPE_1f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    } else
#endif /* BCM_TRX_SUPPORT */
    {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, V6_1f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, V6_1f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);

        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, IPMC_1f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, IPMC_1f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

    if (SOC_MEM_FIELD_VALID(u, L3_ENTRY_IPV6_MULTICASTm, VRF_ID_2f)) {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, VRF_ID_2f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, VRF_ID_2f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_FIELD_VALID(u, L3_ENTRY_IPV6_MULTICASTm, KEY_TYPE_2f)) {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, KEY_TYPE_2f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, KEY_TYPE_2f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    } else
#endif /* BCM_TRX_SUPPORT */
    {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, V6_2f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, V6_2f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);

        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, IPMC_2f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, IPMC_2f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

    if (SOC_MEM_FIELD_VALID(u, L3_ENTRY_IPV6_MULTICASTm, VRF_ID_3f)) {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, VRF_ID_3f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, VRF_ID_3f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_FIELD_VALID(u, L3_ENTRY_IPV6_MULTICASTm, KEY_TYPE_3f)) {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, KEY_TYPE_3f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, KEY_TYPE_3f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    } else
#endif /* BCM_TRX_SUPPORT */
    {
        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, V6_3f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, V6_3f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);

        type_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, IPMC_3f);
        type_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, IPMC_3f);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);
    }

    soc_mem_field_get(u, L3_ENTRY_IPV6_MULTICASTm,
                      e_a, SOURCE_IP_ADDR_UPR_64f, a);
    soc_mem_field_get(u, L3_ENTRY_IPV6_MULTICASTm,
                      e_b, SOURCE_IP_ADDR_UPR_64f, b);
    ent_words = soc_mem_field_length(u, L3_ENTRY_IPV6_MULTICASTm,
                                     SOURCE_IP_ADDR_UPR_64f) / 32;
    for (i = ent_words - 1; i >= 0; i--) {
        SOC_MEM_COMPARE_RETURN(a[i], b[i]);
    }

    soc_mem_field_get(u, L3_ENTRY_IPV6_MULTICASTm,
                      e_a, SOURCE_IP_ADDR_LWR_64f, a);
    soc_mem_field_get(u, L3_ENTRY_IPV6_MULTICASTm,
                      e_b, SOURCE_IP_ADDR_LWR_64f, b);
    ent_words = soc_mem_field_length(u, L3_ENTRY_IPV6_MULTICASTm,
                                     SOURCE_IP_ADDR_LWR_64f) / 32;
    for (i = ent_words - 1; i >= 0; i--) {
        SOC_MEM_COMPARE_RETURN(a[i], b[i]);
    }

    soc_mem_field_get(u, L3_ENTRY_IPV6_MULTICASTm,
                      e_a, GROUP_IP_ADDR_UPR_56f, a);
    soc_mem_field_get(u, L3_ENTRY_IPV6_MULTICASTm,
                      e_b, GROUP_IP_ADDR_UPR_56f, b);
    ent_words = soc_mem_field_length(u, L3_ENTRY_IPV6_MULTICASTm,
                                     GROUP_IP_ADDR_UPR_56f) / 32;
    for (i = ent_words - 1; i >= 0; i--) {
        SOC_MEM_COMPARE_RETURN(a[i], b[i]);
    }

    soc_mem_field_get(u, L3_ENTRY_IPV6_MULTICASTm,
                      e_a, GROUP_IP_ADDR_LWR_64f, a);
    soc_mem_field_get(u, L3_ENTRY_IPV6_MULTICASTm,
                      e_b, GROUP_IP_ADDR_LWR_64f, b);
    ent_words = soc_mem_field_length(u, L3_ENTRY_IPV6_MULTICASTm,
                                     GROUP_IP_ADDR_LWR_64f) / 32;
    for (i = ent_words - 1; i >= 0; i--) {
        SOC_MEM_COMPARE_RETURN(a[i], b[i]);
    }

#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_MEM_FIELD_VALID(u, L3_ENTRY_IPV6_MULTICASTm, VLAN_IDf)) {
        vlan_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, VLAN_IDf);
        vlan_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, VLAN_IDf);
        SOC_MEM_COMPARE_RETURN(vlan_a, vlan_b);
    } else
#endif
    {
        vlan_a = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_a, VLAN_ID_0f);
        vlan_b = soc_L3_ENTRY_IPV6_MULTICASTm_field32_get(u, e_b, VLAN_ID_0f);
        SOC_MEM_COMPARE_RETURN(vlan_a, vlan_b);
    }

    return(0);
}

/*
 * Compares both mask and address. Not a masked compare.
 */
int
_soc_mem_cmp_lpm(int u, void *e_a, void *e_b)
{
    uint32      a;
    uint32      b;

    a = soc_mem_field32_get(u, L3_DEFIPm, e_a, VALID1f);
    b = soc_mem_field32_get(u, L3_DEFIPm, e_b, VALID1f);

    if (a && b) {
        a = soc_mem_field32_get(u, L3_DEFIPm, e_a, MASK1f);
        b = soc_mem_field32_get(u, L3_DEFIPm, e_b, MASK1f);
        SOC_MEM_COMPARE_RETURN(a, b);

        a = soc_mem_field32_get(u, L3_DEFIPm, e_a, IP_ADDR1f);
        b = soc_mem_field32_get(u, L3_DEFIPm, e_b, IP_ADDR1f);
        SOC_MEM_COMPARE_RETURN(a, b);

        if (SOC_MEM_FIELD_VALID(u, L3_DEFIPm, VRF_ID_1f)) {
            a = soc_L3_DEFIPm_field32_get(u, e_a, VRF_ID_1f);
            b = soc_L3_DEFIPm_field32_get(u, e_b, VRF_ID_1f);
            SOC_MEM_COMPARE_RETURN(a, b);
        }

        a = soc_mem_field32_get(u, L3_DEFIPm, e_a, MODE1f);
        b = soc_mem_field32_get(u, L3_DEFIPm, e_b, MODE1f);
        SOC_MEM_COMPARE_RETURN(a, b);

        if (a == 0) {
            return(0); /* IPV4 entry */
        }
    }

    a = soc_mem_field32_get(u, L3_DEFIPm, e_a, VALID0f);
    b = soc_mem_field32_get(u, L3_DEFIPm, e_b, VALID0f);

    if (a && b) {
        a = soc_mem_field32_get(u, L3_DEFIPm, e_a, MASK0f);
        b = soc_mem_field32_get(u, L3_DEFIPm, e_b, MASK0f);
        SOC_MEM_COMPARE_RETURN(a, b);

        a = soc_mem_field32_get(u, L3_DEFIPm, e_a, IP_ADDR0f);
        b = soc_mem_field32_get(u, L3_DEFIPm, e_b, IP_ADDR0f);
        SOC_MEM_COMPARE_RETURN(a, b);

        if (SOC_MEM_FIELD_VALID(u, L3_DEFIPm, VRF_ID_0f)) {
            a = soc_L3_DEFIPm_field32_get(u, e_a, VRF_ID_0f);
            b = soc_L3_DEFIPm_field32_get(u, e_b, VRF_ID_0f);
            SOC_MEM_COMPARE_RETURN(a, b);
        }

        a = soc_mem_field32_get(u, L3_DEFIPm, e_a, MODE0f);
        b = soc_mem_field32_get(u, L3_DEFIPm, e_b, MODE0f);
        SOC_MEM_COMPARE_RETURN(a, b);
        if (a == 0) {
            return(0); /* IPV4 entry */
        }
    }

    a = soc_mem_field32_get(u, L3_DEFIPm, e_a, VALID1f);
    b = soc_mem_field32_get(u, L3_DEFIPm, e_b, VALID1f);
    SOC_MEM_COMPARE_RETURN(a, b);
    a = soc_mem_field32_get(u, L3_DEFIPm, e_a, VALID0f);
    b = soc_mem_field32_get(u, L3_DEFIPm, e_b, VALID0f);
    SOC_MEM_COMPARE_RETURN(a, b);

    return (0);
}

#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
int
_soc_mem_cmp_vlan_mac(int unit, void *ent_a, void *ent_b)
{
    sal_mac_addr_t mac_a, mac_b;

    soc_VLAN_MACm_mac_addr_get(unit, ent_a, MAC_ADDRf, mac_a);
    soc_VLAN_MACm_mac_addr_get(unit, ent_b, MAC_ADDRf, mac_b);

    return ENET_CMP_MACADDR(mac_a, mac_b);
}
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_TRX_SUPPORT
int
_soc_mem_cmp_vlan_mac_tr(int unit, void *ent_a, void *ent_b)
{
    sal_mac_addr_t mac_a, mac_b;
    uint32      type_a, type_b;

    type_a = soc_VLAN_MACm_field32_get(unit, ent_a, KEY_TYPEf);
    type_b = soc_VLAN_MACm_field32_get(unit, ent_b, KEY_TYPEf);
    SOC_MEM_COMPARE_RETURN(type_a, type_b);

    switch (type_a) {
    case 3: /* VLAN_MAC */
        soc_VLAN_MACm_mac_addr_get(unit, ent_a, MAC_ADDRf, mac_a);
        soc_VLAN_MACm_mac_addr_get(unit, ent_b, MAC_ADDRf, mac_b);
        return ENET_CMP_MACADDR(mac_a, mac_b);

#ifdef BCM_TRIUMPH2_SUPPORT
    case 7: /* HPAE (MAC_IP_BIND) */
        type_a = soc_VLAN_MACm_field32_get(unit, ent_a, MAC_IP_BIND__SIPf);
        type_b = soc_VLAN_MACm_field32_get(unit, ent_b, MAC_IP_BIND__SIPf);
        SOC_MEM_COMPARE_RETURN(type_a, type_b);

        return 0;
#endif /* BCM_TRIUMPH2_SUPPORT */

    default:
        return 1;
    }
}

int
_soc_mem_cmp_vlan_xlate_tr(int unit, void *ent_a, void *ent_b)
{
    uint32 val_a, val_b;

    val_a = soc_VLAN_XLATEm_field32_get(unit, ent_a, KEY_TYPEf);
    val_b = soc_VLAN_XLATEm_field32_get(unit, ent_b, KEY_TYPEf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    switch (val_a) {
    case 3: /* VLAN_MAC */
    case 7: /* HPAE (MAC_IP_BIND) */
        return _soc_mem_cmp_vlan_mac_tr(unit, ent_a, ent_b);

    case 0: /* IVID_OVID */
    case 1: /* OTAG */
    case 2: /* ITAG */
    case 4: /* OVID */
    case 5: /* IVID */
    case 6: /* PRI_CFI */
        val_a = soc_VLAN_XLATEm_field32_get(unit, ent_a, GLPf);
        val_b = soc_VLAN_XLATEm_field32_get(unit, ent_b, GLPf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_VLAN_XLATEm_field32_get(unit, ent_a, INCOMING_VIDSf);
        val_b = soc_VLAN_XLATEm_field32_get(unit, ent_b, INCOMING_VIDSf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        return 0;

    case 9: /* VIF_VLAN */
        val_a = soc_VLAN_XLATEm_field32_get(unit, ent_a, VIF__VLANf);
        val_b = soc_VLAN_XLATEm_field32_get(unit, ent_b, VIF__VLANf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);
        /* fall through to case for VIF */

    case 8: /* VIF */
        val_a = soc_VLAN_XLATEm_field32_get(unit, ent_a, VIF__GLPf);
        val_b = soc_VLAN_XLATEm_field32_get(unit, ent_b, VIF__GLPf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_VLAN_XLATEm_field32_get(unit, ent_a, VIF__SRC_VIFf);
        val_b = soc_VLAN_XLATEm_field32_get(unit, ent_b, VIF__SRC_VIFf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        return 0;

    default:
        return 1;
    }
}

int
_soc_mem_cmp_egr_vlan_xlate_tr(int unit, void *ent_a, void *ent_b)
{
    sal_mac_addr_t mac_a, mac_b;
    uint32 val_a, val_b;

    if (SOC_MEM_FIELD_VALID(unit, EGR_VLAN_XLATEm, ENTRY_TYPEf)) {
        val_a = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_a, ENTRY_TYPEf);
        val_b = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_b, ENTRY_TYPEf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        switch (val_a) {
        case 0: /* VLAN_XLATE */
        case 1: /* VLAN_XLATE_DVP */
        case 2: /* VLAN_XLATE_WLAN */
            if (SOC_IS_TD2_TT2(unit)) {
                val_a = soc_EGR_VLAN_XLATEm_field32_get
                    (unit, ent_a, XLATE__DST_MODIDf);
                val_b = soc_EGR_VLAN_XLATEm_field32_get
                    (unit, ent_b, XLATE__DST_MODIDf);
                SOC_MEM_COMPARE_RETURN(val_a, val_b);

                val_a = soc_EGR_VLAN_XLATEm_field32_get
                    (unit, ent_a, XLATE__DST_PORTf);
                val_b = soc_EGR_VLAN_XLATEm_field32_get
                    (unit, ent_b, XLATE__DST_PORTf);
                SOC_MEM_COMPARE_RETURN(val_a, val_b);

                val_a = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_a,
                                                        XLATE__OVIDf);
                val_b = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_b,
                                                        XLATE__OVIDf);
                SOC_MEM_COMPARE_RETURN(val_a, val_b);

                val_a = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_a,
                                                        XLATE__IVIDf);
                val_b = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_b,
                                                        XLATE__IVIDf);
                SOC_MEM_COMPARE_RETURN(val_a, val_b);
                return 0;
            } else if (SOC_IS_TD_TT(unit)) {
                val_a = soc_EGR_VLAN_XLATEm_field32_get
                    (unit, ent_a, DST_MODIDf);
                val_b = soc_EGR_VLAN_XLATEm_field32_get
                    (unit, ent_b, DST_MODIDf);
                SOC_MEM_COMPARE_RETURN(val_a, val_b);

                val_a = soc_EGR_VLAN_XLATEm_field32_get
                    (unit, ent_a, DST_PORTf);
                val_b = soc_EGR_VLAN_XLATEm_field32_get
                    (unit, ent_b, DST_PORTf);
                SOC_MEM_COMPARE_RETURN(val_a, val_b);
            } else if (SOC_MEM_FIELD_VALID(unit, EGR_VLAN_XLATEm, DVPf)) {
                val_a = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_a, DVPf);
                val_b = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_b, DVPf);
                SOC_MEM_COMPARE_RETURN(val_a, val_b);
            } else {
                val_a = soc_EGR_VLAN_XLATEm_field32_get
                    (unit, ent_a, PORT_GROUP_IDf);
                val_b = soc_EGR_VLAN_XLATEm_field32_get
                    (unit, ent_b, PORT_GROUP_IDf);
                SOC_MEM_COMPARE_RETURN(val_a, val_b);
            }

            val_a = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_a, OVIDf);
            val_b = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_b, OVIDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_a, IVIDf);
            val_b = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_b, IVIDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;

        case 3: /* ISID_XLATE */
        case 4: /* ISID_DVP_XLATE */
            val_a = soc_EGR_VLAN_XLATEm_field32_get
                (unit, ent_a, MIM_ISID__VFIf);
            val_b = soc_EGR_VLAN_XLATEm_field32_get
                (unit, ent_b, MIM_ISID__VFIf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_EGR_VLAN_XLATEm_field32_get
                (unit, ent_a, MIM_ISID__DVPf);
            val_b = soc_EGR_VLAN_XLATEm_field32_get
                (unit, ent_b, MIM_ISID__DVPf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;

        case 5: /* WLAN_SVP_TUNNEL */
        case 6: /* WLAN_SVP_BSSID */
        case 7: /* WLAN_SVP_BSSID_RID */
            val_a = soc_EGR_VLAN_XLATEm_field32_get
                (unit, ent_a, WLAN_SVP__RIDf);
            val_b = soc_EGR_VLAN_XLATEm_field32_get
                (unit, ent_b, WLAN_SVP__RIDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            soc_mem_mac_addr_get
                (unit, EGR_VLAN_XLATEm, ent_a, WLAN_SVP__BSSIDf, mac_a);
            soc_mem_mac_addr_get
                (unit, EGR_VLAN_XLATEm, ent_b, WLAN_SVP__BSSIDf, mac_b);
            return ENET_CMP_MACADDR(mac_a, mac_b);

        default:
            return 1;
        }
    }

    val_a = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_a, PORT_GROUP_IDf);
    val_b = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_b, PORT_GROUP_IDf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    val_a = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_a, IVIDf);
    val_b = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_b, IVIDf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    val_a = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_a, OVIDf);
    val_b = soc_EGR_VLAN_XLATEm_field32_get(unit, ent_b, OVIDf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    return 0;
}

#ifdef BCM_TRIUMPH_SUPPORT
int
_soc_mem_cmp_mpls_entry_tr(int unit, void *ent_a, void *ent_b)
{
    sal_mac_addr_t mac_a, mac_b;
    uint32 val_a, val_b;

    if (SOC_MEM_FIELD_VALID(unit, MPLS_ENTRYm, ENTRY_TYPEf)) {
        val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, KEY_TYPEf);
        val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, KEY_TYPEf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        switch (val_a) {
        case 0: /* MPLS */
            val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, PORT_NUMf);
            val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, PORT_NUMf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, MODULE_IDf);
            val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, MODULE_IDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, Tf);
            val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, Tf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, MPLS_LABELf);
            val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, MPLS_LABELf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;
        case 1: /* MIM_NVP */
            val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, MIM_NVP__BVIDf);
            val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, MIM_NVP__BVIDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            soc_mem_mac_addr_get
                (unit, MPLS_ENTRYm, ent_a, MIM_NVP__BMACSAf, mac_a);
            soc_mem_mac_addr_get
                (unit, MPLS_ENTRYm, ent_b, MIM_NVP__BMACSAf, mac_b);
            return ENET_CMP_MACADDR(mac_a, mac_b);

        case 2: /* MIM_ISID */
            val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, MIM_ISID__ISIDf);
            val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, MIM_ISID__ISIDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;

        case 3: /* MIM_ISID_SVP */
            val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, MIM_ISID__ISIDf);
            val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, MIM_ISID__ISIDf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, MIM_ISID__SVPf);
            val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, MIM_ISID__SVPf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;

        case 4: /* WLAN_MAC */
            soc_mem_mac_addr_get
                (unit, MPLS_ENTRYm, ent_a, WLAN_MAC__MAC_ADDRf, mac_a);
            soc_mem_mac_addr_get
                (unit, MPLS_ENTRYm, ent_b, WLAN_MAC__MAC_ADDRf, mac_b);
            return ENET_CMP_MACADDR(mac_a, mac_b);

        case 5: /* TRILL */
            val_a = soc_MPLS_ENTRYm_field32_get
                (unit, ent_a, TRILL__RBRIDGE_NICKNAMEf);
            val_b = soc_MPLS_ENTRYm_field32_get
                (unit, ent_b, TRILL__RBRIDGE_NICKNAMEf);
            SOC_MEM_COMPARE_RETURN(val_a, val_b);

            return 0;

        default:
            return 1;
        }
    }

    val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, PORT_NUMf);
    val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, PORT_NUMf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, MODULE_IDf);
    val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, MODULE_IDf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, Tf);
    val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, Tf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    val_a = soc_MPLS_ENTRYm_field32_get(unit, ent_a, MPLS_LABELf);
    val_b = soc_MPLS_ENTRYm_field32_get(unit, ent_b, MPLS_LABELf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    return 0;
}
#endif /* BCM_TRIUMPH_SUPPORT */
#endif /* BCM_TRX_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
int
_soc_mem_cmp_ing_vp_vlan_membership(int unit, void *ent_a, void *ent_b)
{
    uint32 val_a, val_b;

    val_a = soc_mem_field32_get(unit, ING_VP_VLAN_MEMBERSHIPm, ent_a, VLANf);
    val_b = soc_mem_field32_get(unit, ING_VP_VLAN_MEMBERSHIPm, ent_b, VLANf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    val_a = soc_mem_field32_get(unit, ING_VP_VLAN_MEMBERSHIPm, ent_a, VPf);
    val_b = soc_mem_field32_get(unit, ING_VP_VLAN_MEMBERSHIPm, ent_b, VPf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    return 0;
}

int
_soc_mem_cmp_egr_vp_vlan_membership(int unit, void *ent_a, void *ent_b)
{
    uint32 val_a, val_b;

    val_a = soc_mem_field32_get(unit, EGR_VP_VLAN_MEMBERSHIPm, ent_a, VLANf);
    val_b = soc_mem_field32_get(unit, EGR_VP_VLAN_MEMBERSHIPm, ent_b, VLANf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    val_a = soc_mem_field32_get(unit, EGR_VP_VLAN_MEMBERSHIPm, ent_a, VPf);
    val_b = soc_mem_field32_get(unit, EGR_VP_VLAN_MEMBERSHIPm, ent_b, VPf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    return 0;
}

int
_soc_mem_cmp_ing_dnat_address_type(int unit, void *ent_a, void *ent_b)
{
    uint32 val_a, val_b;

    val_a = soc_mem_field32_get(unit, ING_DNAT_ADDRESS_TYPEm, ent_a,
                                DEST_IPV4_ADDRf);
    val_b = soc_mem_field32_get(unit, ING_DNAT_ADDRESS_TYPEm, ent_b,
                                DEST_IPV4_ADDRf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    val_a = soc_mem_field32_get(unit, ING_DNAT_ADDRESS_TYPEm, ent_a, VRFf);
    val_b = soc_mem_field32_get(unit, ING_DNAT_ADDRESS_TYPEm, ent_b, VRFf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    return 0;
}

int
_soc_mem_cmp_l2_endpoint_id(int unit, void *ent_a, void *ent_b)
{
    sal_mac_addr_t mac_a, mac_b;
    uint32 val_a, val_b;

    val_a = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_a, KEY_TYPEf);
    val_b = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_b, KEY_TYPEf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    switch (val_a) {
    case TD2_L2_HASH_KEY_TYPE_BRIDGE:
        val_a = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_a,
                                    L2__VLAN_IDf);
        val_b = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_b,
                                    L2__VLAN_IDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        soc_mem_mac_addr_get(unit, L2_ENDPOINT_IDm, ent_a, L2__MAC_ADDRf,
                             mac_a);
        soc_mem_mac_addr_get(unit, L2_ENDPOINT_IDm, ent_b, L2__MAC_ADDRf,
                             mac_b);
        return ENET_CMP_MACADDR(mac_a, mac_b);

    case TD2_L2_HASH_KEY_TYPE_VFI:
        val_a = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_a, L2__VFIf);
        val_b = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_b, L2__VFIf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        soc_mem_mac_addr_get(unit, L2_ENDPOINT_IDm, ent_a, L2__MAC_ADDRf,
                             mac_a);
        soc_mem_mac_addr_get(unit, L2_ENDPOINT_IDm, ent_b, L2__MAC_ADDRf,
                             mac_b);
        return ENET_CMP_MACADDR(mac_a, mac_b);

    case TD2_L2_HASH_KEY_TYPE_VIF:
        val_a = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_a,
                                    VIF__NAMESPACEf);
        val_b = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_b,
                                    VIF__NAMESPACEf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_a,
                                    VIF__DST_VIFf);
        val_b = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_b,
                                    VIF__DST_VIFf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_a, VIF__Pf);
        val_b = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_b, VIF__Pf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        return 0;

    case TD2_L2_HASH_KEY_TYPE_PE_VID:
        val_a = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_a,
                                    PE_VID__NAMESPACEf);
        val_b = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_b,
                                    PE_VID__NAMESPACEf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_a,
                                    PE_VID__ETAG_VIDf);
        val_b = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, ent_b,
                                    PE_VID__ETAG_VIDf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        return 0;

    default:
        return 1;
    }

    return 0;
}

int
_soc_mem_cmp_endpoint_queue_map(int unit, void *ent_a, void *ent_b)
{
    uint32 val_a, val_b;

    val_a = soc_mem_field32_get(unit, ENDPOINT_QUEUE_MAPm, ent_a, KEY_TYPEf);
    val_b = soc_mem_field32_get(unit, ENDPOINT_QUEUE_MAPm, ent_b, KEY_TYPEf);
    SOC_MEM_COMPARE_RETURN(val_a, val_b);

    switch (val_a) {
    case 0: /* endpoint queueing */
        val_a = soc_mem_field32_get(unit, ENDPOINT_QUEUE_MAPm, ent_a,
                                    EH_QUEUE_TAGf);
        val_b = soc_mem_field32_get(unit, ENDPOINT_QUEUE_MAPm, ent_b,
                                    EH_QUEUE_TAGf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        val_a = soc_mem_field32_get(unit, ENDPOINT_QUEUE_MAPm, ent_a,
                                    DEST_PORTf);
        val_b = soc_mem_field32_get(unit, ENDPOINT_QUEUE_MAPm, ent_b,
                                    DEST_PORTf);
        SOC_MEM_COMPARE_RETURN(val_a, val_b);

        return 0;

    default:
        return 1;
    }

    return 0;
}
#endif /* BCM_TRIDENT2_SUPPORT */

/*
 *  This function caches the info needed by the L3 key comparison, below.
 */
int
_soc_mem_cmp_l3x_sync(int unit)
{
    return SOC_E_NONE;
}

int
_soc_mem_cmp_l3x_set(int unit, uint32 ipmc_config)
{
    return SOC_E_NONE;
}

int
_soc_mem_cmp_l3x(int unit, void *ent_a, void *ent_b)
{
    return 0;
}

/*
 * Function:
 *    soc_mem_index_limit
 * Purpose:
 *    Returns the maximum possible index for any dynamically extendable memory
 */
int 
soc_mem_index_limit(int unit, soc_mem_t mem)
{
    _SOC_MEM_REPLACE_MEM(unit, mem);
    
    return soc_mem_index_count(unit, mem);
}

#endif /* BCM_XGS_SWITCH_SUPPORT */

/*
 * Function:
 *    soc_mem_index_last
 * Purpose:
 *    Compute the highest index currently occupied in a sorted table
 */

int
soc_mem_index_last(int unit, soc_mem_t mem, int copyno)
{
    assert(SOC_MEM_IS_VALID(unit, mem));
    assert(soc_attached(unit));
    assert(soc_mem_is_sorted(unit,mem));

    if (copyno == MEM_BLOCK_ANY) {
        /* coverity[overrun-local : FALSE] */
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    assert(SOC_MEM_BLOCK_VALID(unit, mem, copyno));
    /* coverity[overrun-local : FALSE] */
    return (SOC_MEM_INFO(unit, mem).index_min +
                SOP_MEM_STATE(unit, mem).count[copyno] - 1);
}


/*
 * Function:
 *    soc_mem_entry_dump
 * Purpose:
 *    Debug routine to dump a formatted table entry.
 *
 *    Note:  Prefix != NULL : Dump chg command
 *           Prefix == NULL : Dump     command
 *             (Actually should pass dump_chg flag but keeping for simplicity)
 */
STATIC void
soc_mem_entry_dump_common(int unit, soc_mem_t mem, void *buf, char *prefix, int vertical)
{
    soc_field_info_t *fieldp;
    soc_mem_info_t *memp;
    int f;
    char dummy[]="";
    char *mem_prefix=dummy;
    int  first_print_flag=0;

#if !defined(SOC_NO_NAMES)
    uint32 key_type = 0, default_type = 0;
    uint32 fval[SOC_MAX_MEM_FIELD_WORDS];
    uint32 nfval[SOC_MAX_MEM_FIELD_WORDS]={0};
    char tmp[(SOC_MAX_MEM_FIELD_WORDS * 8) + 3];
#endif
             /* Max nybbles + "0x" + null terminator */

    memp = &SOC_MEM_INFO(unit, mem);
    if (prefix != NULL) {
        mem_prefix = prefix; 
    }

    if (!SOC_MEM_IS_VALID(unit, mem)) {
        soc_cm_print("<%s:%d>Memory not valid for unit\n",mem_prefix,mem);
        return;
    } 
#if !defined(SOC_NO_NAMES)
    if ((memp->flags & SOC_MEM_FLAG_MULTIVIEW) 
#ifdef BCM_ARAD_SUPPORT
             && (!SOC_IS_ARAD(unit))
#endif
         ) {
#ifdef KEY_TYPEf
        if (SOC_MEM_FIELD_VALID(unit, mem, KEY_TYPEf)) {
            soc_mem_field_get(unit, mem, buf, KEY_TYPEf, &key_type);
        } else if (SOC_MEM_FIELD_VALID(unit, mem, KEY_TYPE_0f)) {
            soc_mem_field_get(unit, mem, buf, KEY_TYPE_0f, &key_type);
        } else if (SOC_MEM_FIELD_VALID(unit, mem, VP_TYPEf)) {
            soc_mem_field_get(unit, mem, buf, VP_TYPEf, &key_type);
        } else {
            soc_mem_field_get(unit, mem, buf, ENTRY_TYPEf, &key_type);
        }
#endif
        default_type = 0;
#if defined(BCM_TRIUMPH2_SUPPORT)
        if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
            SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
            SOC_IS_HURRICANE(unit) || SOC_IS_TD_TT(unit) ||
            SOC_IS_KATANA(unit)) {
            if((mem == VLAN_MACm) ||
                (mem == L3_ENTRY_IPV4_MULTICASTm) ||
                (mem == L3_ENTRY_IPV6_UNICASTm) ||
                (mem == L3_ENTRY_IPV6_MULTICASTm)){
                default_type = key_type;
            }
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
        if (sal_strlen(memp->views[key_type]) == 0) {
            soc_cm_print("<Wrong Key_type %d>\n",key_type);
            return;
        }
    }
#endif
    for (f = memp->nFields - 1; f >= 0; f--) {
        fieldp = &memp->fields[f];
#if !defined(SOC_NO_NAMES)
        if ((memp->flags & SOC_MEM_FLAG_MULTIVIEW) 
#ifdef BCM_ARAD_SUPPORT
             && (!SOC_IS_ARAD(unit))
#endif
             ) {
            if (!(strstr(soc_fieldnames[fieldp->field], memp->views[key_type]) ||
                (strcmp(memp->views[key_type], memp->views[default_type]) == 0 
                 && !strstr(soc_fieldnames[fieldp->field], ":"))             ||
                (fieldp->flags & SOCF_GLOBAL))) {
                continue;
            }
        }
        sal_memset(fval, 0, sizeof (fval));
        soc_mem_field_get(unit, mem, buf, fieldp->field, fval);
        if (mem_prefix == prefix) {
            if (sal_memcmp(fval, nfval, SOC_MAX_MEM_FIELD_WORDS *
                           sizeof (uint32)) == 0) {
                continue;
            }
        }
        if (first_print_flag == 0) {
            soc_cm_print((vertical ? "%s" : "%s<"), mem_prefix);
            first_print_flag=1;
        }
        if (vertical) {
            _shr_format_long_integer(tmp, fval, SOC_MAX_MEM_FIELD_WORDS);
            soc_cm_print("\n\t%30s: %s", soc_fieldnames[fieldp->field], tmp);
        } else {
            soc_cm_print("%s=", soc_fieldnames[fieldp->field]);
            _shr_format_long_integer(tmp, fval, SOC_MAX_MEM_FIELD_WORDS);
            soc_cm_print("%s%s", tmp, f > 0 ? "," : "");
        }
#endif
    }
    if (first_print_flag == 1) {
        soc_cm_print(vertical ? "\n" : ">\n");
    }
}
void
soc_mem_entry_dump(int unit, soc_mem_t mem, void *buf)
{
   soc_mem_entry_dump_common(unit, mem, buf, NULL, 0);
}
void
soc_mem_entry_dump_vertical(int unit, soc_mem_t mem, void *buf)
{
   soc_mem_entry_dump_common(unit, mem, buf, NULL, 1);
}
void
soc_mem_entry_dump_if_changed(int unit, soc_mem_t mem, void *buf, char *prefix)
{
    soc_mem_entry_dump_common(unit, mem, buf, prefix, 0);
}

/************************************************************************/
/* Routines for configuring the table memory caches            */
/************************************************************************/

/*
 * Function:
 *    soc_mem_cache_get
 * Purpose:
 *    Return whether or not caching is enabled for a specified memory
 *    or memories
 */

int
soc_mem_cache_get(int unit, soc_mem_t mem, int copyno)
{
    int        rv;

    assert(SOC_UNIT_VALID(unit));
    _SOC_MEM_REUSE_MEM_STATE(unit, mem);
    assert(SOC_MEM_IS_VALID(unit, mem));

    if (!soc_mem_is_cachable(unit, mem) || SOC_MEM_TEST_SKIP_CACHE(unit)) {
        return FALSE;
    }

    MEM_LOCK(unit, mem);

    if (copyno == SOC_BLOCK_ALL) {
        rv = TRUE;
        SOC_MEM_BLOCK_ITER(unit, mem, copyno) {
            rv = (rv && (SOC_MEM_STATE(unit, mem).cache[copyno] != NULL));
        }
    } else {
        rv = (SOC_MEM_STATE(unit, mem).cache[copyno] != NULL);
    }

    MEM_UNLOCK(unit, mem);

    return rv;
}

void
_soc_mem_l3_entry_cache_entry_validate(int unit, soc_mem_t mem, uint32 *entry,
                                       uint8 *vmap, int *index)
{
    if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_ONLYm, KEY_TYPEf)) {
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TD2_TT2(unit)) {
            switch (mem) {
            case L3_ENTRY_ONLYm:
            case L3_ENTRY_IPV4_UNICASTm:
                if (!soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, VALIDf)) {
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                }
                switch (soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, KEY_TYPEf)) {
                case TD2_L3_HASH_KEY_TYPE_V4MC:
                case TD2_L3_HASH_KEY_TYPE_V6UC:
                case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN_EXT:
                case TD2_L3_HASH_KEY_TYPE_FCOE_HOST_EXT:
                case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP_EXT:
                case TD2_L3_HASH_KEY_TYPE_DST_NAT:
                case TD2_L3_HASH_KEY_TYPE_DST_NAPT:
                    CACHE_VMAP_CLR(vmap, *index); *index += 1;
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                case TD2_L3_HASH_KEY_TYPE_V6UC_EXT:
                case TD2_L3_HASH_KEY_TYPE_V6MC:
                    CACHE_VMAP_CLR(vmap, *index); *index += 1;
                    CACHE_VMAP_CLR(vmap, *index); *index += 1;
                    CACHE_VMAP_CLR(vmap, *index); *index += 1;
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                default:
                    break;
                }
                break;
            case L3_ENTRY_IPV4_MULTICASTm:
            case L3_ENTRY_IPV6_UNICASTm:
                if (!soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, VALIDf)) {
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                }
                switch (soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, KEY_TYPEf)) {
                case TD2_L3_HASH_KEY_TYPE_V4UC:
                case TD2_L3_HASH_KEY_TYPE_TRILL:
                case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN:
                case TD2_L3_HASH_KEY_TYPE_FCOE_HOST:
                case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP:
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                case TD2_L3_HASH_KEY_TYPE_V6UC_EXT:
                case TD2_L3_HASH_KEY_TYPE_V6MC:
                    CACHE_VMAP_CLR(vmap, *index); *index += 1;
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                default:
                    break;
                }
                break;
            case L3_ENTRY_IPV6_MULTICASTm:
                if (!soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, VALIDf)) {
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                }
                switch (soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, KEY_TYPEf)) {
                case TD2_L3_HASH_KEY_TYPE_V6UC_EXT:
                case TD2_L3_HASH_KEY_TYPE_V6MC:
                    break;
                default:
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                }
                break;
            default:
                break;
            }
        } else 
#endif /* BCM_TRIDENT2_SUPPORT */
        {
            switch (mem) {
            case L3_ENTRY_ONLYm:
            case L3_ENTRY_IPV4_UNICASTm:
                if (!soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, VALIDf)) {
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                }
                switch (soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, KEY_TYPEf)) {
                case TR_L3_HASH_KEY_TYPE_V4MC:
                case TR_L3_HASH_KEY_TYPE_V6UC:
                    CACHE_VMAP_CLR(vmap, *index); *index += 1;
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                case TR_L3_HASH_KEY_TYPE_V6MC:
                    CACHE_VMAP_CLR(vmap, *index); *index += 1;
                    CACHE_VMAP_CLR(vmap, *index); *index += 1;
                    CACHE_VMAP_CLR(vmap, *index); *index += 1;
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                default:
                    break;
                }
                break;
            case L3_ENTRY_IPV4_MULTICASTm:
            case L3_ENTRY_IPV6_UNICASTm:
                if (!soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, VALIDf)) {
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                }
                switch (soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, KEY_TYPEf)) {
                case TR_L3_HASH_KEY_TYPE_V4UC:
                case TR_L3_HASH_KEY_TYPE_LMEP:
                case TR_L3_HASH_KEY_TYPE_RMEP:
                case TR_L3_HASH_KEY_TYPE_TRILL:
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                case TR_L3_HASH_KEY_TYPE_V6MC:
                    CACHE_VMAP_CLR(vmap, *index); *index += 1;
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                default:
                    break;
                }
                break;
            case L3_ENTRY_IPV6_MULTICASTm:
                if (!soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, VALIDf)) {
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                }
                switch (soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, KEY_TYPEf)) {
                case TR_L3_HASH_KEY_TYPE_V6MC:
                    break;
                default:
                    CACHE_VMAP_CLR(vmap, *index);
                    break;
                }
                break;
            default: 
                break;            
            }
        }
    } else if (SOC_MEM_IS_VALID(unit, L3_ENTRY_1m)) {
        
    }
}

/*
 * Function:
 *    soc_mem_cache_set
 * Purpose:
 *    Enable or disable caching for a specified memory or memories
 * Notes:
 *    Only tables whose contents are not modified by hardware in
 *    any way are eligible for caching.
 */

int
soc_mem_cache_set(int unit, soc_mem_t mem, int copyno, int enable)
{
    int blk, entry_dw, index_cnt;
    int cache_size, vmap_size;
    uint8 load_cache = 1, l2 = 0, *vmap;
    uint32 *cache;
    soc_memstate_t *memState;
    soc_stat_t *stat = SOC_STAT(unit);
    soc_mem_t org_mem = mem;

    assert(SOC_UNIT_VALID(unit));
    _SOC_MEM_REUSE_MEM_STATE(unit, mem);
    assert(SOC_MEM_IS_VALID(unit, mem));

    if (!(soc_mem_is_cachable(unit, mem))) {
        return (enable ? SOC_E_UNAVAIL : SOC_E_NONE);
    }
    memState = &SOC_MEM_STATE(unit, mem);
    if (memState->lock == NULL) {
        return SOC_E_NONE;
    }    
    entry_dw = soc_mem_entry_words(unit, mem);
    index_cnt = soc_mem_index_count(unit, mem);
    cache_size = index_cnt * entry_dw * 4;
    vmap_size = (index_cnt + 7) / 8;

    soc_cm_debug(DK_SOCMEM, "soc_mem_cache_set: unit %d memory %s.%s %sable\n",
                 unit, SOC_MEM_UFNAME(unit, mem), SOC_BLOCK_NAME(unit, copyno),
                 enable ? "en" : "dis");

    if (copyno != COPYNO_ALL) {
        assert(SOC_MEM_BLOCK_VALID(unit, mem, copyno));
    }

    MEM_LOCK(unit, mem);

    SOC_MEM_BLOCK_ITER(unit, mem, blk) {
        if (copyno != COPYNO_ALL && copyno != blk) {
            continue;
        }
        /* Turn off caching if currently enabled. */
        if (memState->cache[blk] != NULL) {
            sal_free(memState->cache[blk]);
            memState->cache[blk] = NULL;
        }
        if (memState->vmap[blk] != NULL) {
            sal_free(memState->vmap[blk]);
            memState->vmap[blk] = NULL;
        }
        if (!enable) {
            continue;
        }
        /* Allocate new cache */
        if ((cache = sal_alloc(cache_size, "table-cache")) == NULL) {
            MEM_UNLOCK(unit, mem);
            return SOC_E_MEMORY;
        }
        stat->mem_cache_count++;
        stat->mem_cache_size += cache_size;

        if ((vmap = sal_alloc(vmap_size, "table-vmap")) == NULL) {
            sal_free(cache);
            MEM_UNLOCK(unit, mem);
            return SOC_E_MEMORY;
        }
        stat->mem_cache_vmap_size += vmap_size;

        sal_memset(vmap, 0, vmap_size);
        if (_SOC_MEM_CHK_L2_MEM(mem)) {
            l2 = 1;
        }
        if (!(SOC_IS_TD_TT(unit) || SOC_IS_ENDURO(unit))) {
            
            load_cache = 0;
        }
        if (SOC_IS_TRIUMPH3(unit) && 
            ((mem == PORT_EHG_RX_TUNNEL_MASKm) ||
             (mem == PORT_EHG_RX_TUNNEL_DATAm) || 
             (mem == PORT_EHG_TX_TUNNEL_DATAm))) {
            
            load_cache = 0;
        }
#if ((defined BCM_XGS_SWITCH_SUPPORT ) || (defined BCM_DFE_SUPPORT) || \
     (defined BCM_PETRA_SUPPORT))
        /* Pre-load cache from hardware table (if quickly-doable) OR
           we are doing Warm-Boot */
        if ((soc_feature(unit, soc_feature_table_dma) &&
             soc_mem_dmaable(unit, mem, blk) && 
             (SOC_CONTROL(unit)->soc_flags & SOC_F_INITED) && !l2) ||
            (SOC_WARM_BOOT(unit) && load_cache &&
             (SOC_CONTROL(unit)->mem_scache_ptr == NULL))) {
            uint32 *table;
            int index, index_max = index_cnt - 1;
            int index_min = soc_mem_index_min(unit, mem);
            uint32 entry[SOC_MAX_MEM_WORDS];

            if ((table = soc_cm_salloc(unit, cache_size, "dma")) != NULL) {
                if (SOC_IS_TRIUMPH3(unit) && 
                    ((mem == PORT_EHG_RX_TUNNEL_MASKm) ||
                     (mem == PORT_EHG_RX_TUNNEL_DATAm) || 
                     (mem == PORT_EHG_TX_TUNNEL_DATAm))) {
                    
                    if (blk == 12 || blk == 13) {
                        index_cnt = 16;
                        index_max = index_cnt - 1;
                    }
                }
                if (soc_mem_read_range(unit, mem, blk, index_min, index_max,
                                       &table[index_min * entry_dw]) >= 0) {
                                        
                    sal_memcpy(cache, table, cache_size);
                    /* Set all vmap bits as valid */
                    sal_memset(&vmap[index_min / 8], 0xff,
                               (index_cnt + 7 - index_min) / 8);
                
                    /* Clear invalid bits at the left and right ends */
                    vmap[index_min / 8] &= 0xff00 >> (8 - index_min % 8);
                    vmap[vmap_size - 1] &= 0x00ff >> ((8 - index_cnt % 8) % 8);
                }
                if (SOC_IS_TD_TT(unit) && (mem == FP_GLOBAL_MASK_TCAMm)) {
                    /* The read of FP_GLOBAL_MASK_TCAM only returns the
                     * X-pipe portions of the pbmp.  We need the Y-pipe
                     * portions also to match what was available
                     * during Cold Boot.  So we explicitly read the
                     * Y-pipe table here and OR the bitmaps together
                     * to form the complete entry that existed in
                     * the Cold Boot cache.
                     */
                    if (soc_mem_read_range(unit, FP_GLOBAL_MASK_TCAM_Ym,
                                           blk, index_min, index_max,
                                       &table[index_min * entry_dw]) >= 0) {
                        soc_pbmp_t pbmp_x, pbmp_y;
                        uint32 *entry_x, *entry_y;
                        for (index = index_min;
                             index <= index_max;
                             index++) {
                            /* FP_GLOBAL_MASK_TCAM[_X|_Y] have the same
                             * field structure, so we can use the field
                             * accessors for just the base memory. */
                            entry_x = cache + index * entry_dw;
                            entry_y = &table[index_min * entry_dw] +
                                index * entry_dw;

                            soc_mem_pbmp_field_get(unit, mem, entry_x,
                                                   IPBM_MASKf, &pbmp_x);
                            SOC_PBMP_AND(pbmp_x, PBMP_XPIPE(unit));
                            soc_mem_pbmp_field_get(unit, mem, entry_y,
                                                   IPBM_MASKf, &pbmp_y);
                            SOC_PBMP_AND(pbmp_y, PBMP_YPIPE(unit));
                            SOC_PBMP_OR(pbmp_x, pbmp_y);
                            soc_mem_pbmp_field_set(unit, mem, entry_x,
                                                   IPBM_MASKf, &pbmp_x);

                            soc_mem_pbmp_field_get(unit, mem, entry_x,
                                                   IPBMf, &pbmp_x);
                            SOC_PBMP_AND(pbmp_x, PBMP_XPIPE(unit));
                            soc_mem_pbmp_field_get(unit, mem, entry_y,
                                                   IPBMf, &pbmp_y);
                            SOC_PBMP_AND(pbmp_y, PBMP_YPIPE(unit));
                            SOC_PBMP_OR(pbmp_x, pbmp_y);
                            soc_mem_pbmp_field_set(unit, mem, entry_x,
                                                   IPBMf, &pbmp_x);
                        }
                    }
                }
                soc_cm_sfree(unit, table);
                if (_SOC_MEM_CHK_L2_MEM(mem)) {
                    for (index = index_min; index <= index_max; index++) {
                        sal_memcpy(entry, cache + index*entry_dw, entry_dw * 4);
                        if ((mem == L2_ENTRY_2m && 
                            (soc_mem_field32_get(unit, mem, entry, STATIC_BIT_0f) &&
                             soc_mem_field32_get(unit, mem, entry, STATIC_BIT_1f))) ||
                            ((mem == L2Xm || mem == L2_ENTRY_1m) &&
                             soc_mem_field32_get(unit, mem, entry, STATIC_BITf))) {
                            /* Continue */
                        } else {
                            CACHE_VMAP_CLR(vmap, index);
                        }
                    }
                } else if (_SOC_MEM_CHK_L3_MEM(mem)) {
                    uint32 *centry;
                    
                    for (index = index_min; index <= index_max; index++) {
                        centry = cache + index*entry_dw;
                        sal_memcpy(entry, centry, entry_dw * 4);
                        _soc_mem_l3_entry_cache_entry_validate(unit, mem, entry, vmap, &index);
                        if (SOC_MEM_FIELD_VALID(unit, mem, HITf)) {
                            soc_mem_field32_set(unit, mem, centry, HITf, 0);
                        } else if (SOC_MEM_FIELD_VALID(unit, mem, HIT_1f)) {
                            soc_mem_field32_set(unit, mem, centry, HIT_0f, 0);
                            soc_mem_field32_set(unit, mem, centry, HIT_1f, 0);
                        }
                        if (SOC_MEM_FIELD_VALID(unit, mem, HIT_3f)) {
                            soc_mem_field32_set(unit, mem, centry, HIT_2f, 0);
                            soc_mem_field32_set(unit, mem, centry, HIT_3f, 0);
                        }
                    }
                } else if (_SOC_MEM_CHK_EGR_IP_TUNNEL(mem)) {
                    uint32 *centry;
                    
                    for (index = index_min; index <= index_max; index++) {
                        centry = cache + index*entry_dw;
                        sal_memcpy(entry, centry, entry_dw * 4);
                        if (soc_mem_field32_get(unit, mem, entry, ENTRY_TYPEf) == 0) {
                            CACHE_VMAP_CLR(vmap, index);
                        }
                        if (mem == EGR_IP_TUNNELm) {
                            if (soc_mem_field32_get(unit, mem, entry, ENTRY_TYPEf) != 1) {
                                CACHE_VMAP_CLR(vmap, index);
                            }
                        } else if (mem == EGR_IP_TUNNEL_MPLSm) {
                            if (soc_mem_field32_get(unit, mem, entry, ENTRY_TYPEf) != 3) {
                                CACHE_VMAP_CLR(vmap, index);
                            }
                        } else if (mem == EGR_IP_TUNNEL_IPV6m) {
                            if (soc_mem_field32_get(unit, mem, entry, ENTRY_TYPEf) != 2) {
                                CACHE_VMAP_CLR(vmap, index);
                            }
                        }
                    }
                } else if (SOC_MEM_SER_CORRECTION_TYPE(unit, mem) &
                           SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE) {
                    uint32 *centry;
                    
                    for (index = index_min; index <= index_max; index++) {
                        centry = cache + index*entry_dw;
                        if (SOC_MEM_FIELD_VALID(unit, mem, HITf)) {
                            soc_mem_field32_set(unit, mem, centry, HITf, 0);
                        } else if (SOC_MEM_FIELD_VALID(unit, mem, HIT0f)) {
                            soc_mem_field32_set(unit, mem, centry, HIT0f, 0);
                        } else if (SOC_MEM_FIELD_VALID(unit, mem, HIT1f)) {
                            soc_mem_field32_set(unit, mem, centry, HIT1f, 0);
                        } else if (SOC_MEM_FIELD_VALID(unit, mem, HIT_0f)) {
                            soc_mem_field32_set(unit, mem, centry, HIT_0f, 0);
                        } else if (SOC_MEM_FIELD_VALID(unit, mem, HIT_1f)) {
                            soc_mem_field32_set(unit, mem, centry, HIT_1f, 0);
                        } else if (SOC_MEM_FIELD_VALID(unit, mem, HIT_2f)) {
                            soc_mem_field32_set(unit, mem, centry, HIT_2f, 0);
                        } else if (SOC_MEM_FIELD_VALID(unit, mem, HIT_3f)) {
                            soc_mem_field32_set(unit, mem, centry, HIT_3f, 0);
                        } else if (SOC_MEM_FIELD_VALID(unit, mem, HIT_BITSf)) {
                            soc_mem_field32_set(unit, mem, centry, HIT_BITSf, 0);
                        }
                    }
                }
            }
        }
#endif /* BCM_XGS_SWITCH_SUPPORT */

        soc_cm_debug(DK_SOCMEM,
                     "soc_mem_cache_set: cache=%p size=%d vmap=%p\n",
                     (void *)cache, cache_size, (void *)vmap);

        memState->vmap[blk] = vmap;
        memState->cache[blk] = cache;
        if (org_mem != mem) {
            /* Share the cache and vmap */
            SOC_MEM_STATE(unit, org_mem).cache[blk] = cache;
            SOC_MEM_STATE(unit, org_mem).vmap[blk] = vmap;
        } else if (_SOC_MEM_IS_REUSED_MEM(unit, mem)) {
            /* Sync the cache and vmap */
            _SOC_MEM_SYNC_MAPPED_MEM_STATES(unit, mem);
        }
    }

    MEM_UNLOCK(unit, mem);

    return SOC_E_NONE;
}

/*
 * Function:
 *    soc_mem_cache_invalidate
 * Purpose:
 *    Invalidate a cache entry. Forces read from the device instead
 *      of from cached copy.
 */

int
soc_mem_cache_invalidate(int unit, soc_mem_t mem, int copyno, int index)
{
    int blk;
    uint8 *vmap;

    assert(SOC_UNIT_VALID(unit));
    _SOC_MEM_REUSE_MEM_STATE(unit, mem);
    assert(SOC_MEM_IS_VALID(unit, mem));

    if (!soc_mem_is_cachable(unit, mem)) {
        return SOC_E_UNAVAIL;
    }

    if (index < soc_mem_index_min(unit, mem) ||
    index > soc_mem_index_max(unit, mem)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_cache_invalidate: invalid index %d "
                     "for memory %s\n",
                     index, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    MEM_LOCK(unit, mem);

    SOC_MEM_BLOCK_ITER(unit, mem, blk) {
        if (copyno != SOC_BLOCK_ALL && copyno != blk) {
            continue;
        }

        if (SOC_MEM_STATE(unit, mem).cache[blk] == NULL) {
            continue;
        }

        /* Invalidate the cached entry */

        vmap = SOC_MEM_STATE(unit, mem).vmap[blk];
        CACHE_VMAP_CLR(vmap, index);
    }

    MEM_UNLOCK(unit, mem);

    return SOC_E_NONE;
}

#ifdef BCM_WARM_BOOT_SUPPORT

/*
 *     Mem cache structure:
 * +++++++++++++++++++++++++++
 * |  Mem 0: Name string     | -> 128 characters
 * +++++++++++++++++++++++++++
 * |                         |
 * |        Mem table        |
 * |                         |
 * |                         |
 * +++++++++++++++++++++++++++
 * |      Valid bitmap       |
 * +++++++++++++++++++++++++++
 *            ....
 *            ....
 *            ....
 *            ....
 * +++++++++++++++++++++++++++
 * |  Mem n: Name string     |
 * +++++++++++++++++++++++++++
 * |                         |
 * |        Mem table        |
 * |                         |
 * |                         |
 * +++++++++++++++++++++++++++
 * |      Valid bitmap       |
 * +++++++++++++++++++++++++++
 * |                         |
 * |       Reg cache         |
 * |                         |
 * +++++++++++++++++++++++++++
 */

int
soc_mem_scache_size_get(int unit)
{
    int size = 0;
#if defined(BCM_XGS_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) 
    int count;
    soc_ser_reg_cache_info(unit, &count, &size);
#endif
    return size + SOC_STAT(unit)->mem_cache_size + 
           SOC_STAT(unit)->mem_cache_count*128;
}

int
soc_mem_cache_scache_init(int unit)
{
    uint32 alloc_get;
    soc_scache_handle_t scache_handle;
    uint8 *mem_scache_ptr;
    int rv, stable_size, alloc_sz = soc_mem_scache_size_get(unit);

    if (NULL == SOC_CONTROL(unit)->mem_scache_ptr) {
        SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
        if ((stable_size > SOC_DEFAULT_LVL2_STABLE_SIZE) &&
            !SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
            SOC_SCACHE_HANDLE_SET(scache_handle, unit,
                                  SOC_SCACHE_MEMCACHE_HANDLE, 0);
            /* Get the pointer for the Level 2 cache */
            rv = soc_scache_ptr_get(unit, scache_handle,
                                    &mem_scache_ptr, &alloc_get);
            if (!SOC_WARM_BOOT(unit) && (SOC_E_NOT_FOUND == rv)) {
                /* Not yet allocated in Cold Boot */
                SOC_IF_ERROR_RETURN
                    (soc_scache_alloc(unit, scache_handle, 
                                      alloc_sz + SOC_WB_SCACHE_CONTROL_SIZE));
                rv = soc_scache_ptr_get(unit, scache_handle,
                                        &mem_scache_ptr, &alloc_get);
                if (SOC_FAILURE(rv)) {
                    return rv;
                } else if (alloc_get != alloc_sz +
                                        SOC_WB_SCACHE_CONTROL_SIZE) {
                    /* Expected size doesn't match retrieved size */
                    return SOC_E_INTERNAL;
                } else if (NULL == mem_scache_ptr) {
                    return SOC_E_MEMORY;
                }
                SOC_CONTROL(unit)->mem_scache_ptr = mem_scache_ptr;
            }
        }
    }
    return SOC_E_NONE;
}

int
soc_mem_cache_scache_sync(int unit)
{
    uint32 *cache;
    int blk, entry_dw, index_cnt;
    int cache_size, vmap_size, offset= 0;
    soc_mem_t mem, tmp_mem;
    uint8 *vmap, *mem_scache_ptr = SOC_CONTROL(unit)->mem_scache_ptr;

    if (NULL == mem_scache_ptr) {
        return SOC_E_UNAVAIL;
    }
    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
        if (!SOC_MEM_IS_VALID(unit, mem)) {
            continue;
        }
        if (soc_mem_index_count(unit, mem) == 0) {
            continue;
        }
        tmp_mem = mem;
        _SOC_MEM_REUSE_MEM_STATE(unit, tmp_mem);
        if (tmp_mem != mem) {
            continue;
        }
        entry_dw = soc_mem_entry_words(unit, mem);
        index_cnt = soc_mem_index_count(unit, mem);
        cache_size = index_cnt * entry_dw * 4;
        vmap_size = (index_cnt + 7) / 8;
    
        MEM_LOCK(unit, mem);
        SOC_MEM_BLOCK_ITER(unit, mem, blk) {
            if (SOC_MEM_STATE(unit, mem).cache[blk] == NULL) {
                continue;
            }
            cache = SOC_MEM_STATE(unit, mem).cache[blk];
            vmap = SOC_MEM_STATE(unit, mem).vmap[blk];
            
            sal_memcpy(mem_scache_ptr+offset, SOC_MEM_UFNAME(unit, mem),
                       strlen(SOC_MEM_UFNAME(unit, mem)));
            soc_cm_debug(DK_SOCMEM+DK_VERBOSE, 
                         "Store at %d %s\n", offset, SOC_MEM_UFNAME(unit, mem));
            offset += 128;
            sal_memcpy(mem_scache_ptr+offset, cache, cache_size);
            offset += cache_size;
            sal_memcpy(mem_scache_ptr+offset, vmap, vmap_size);
            offset += vmap_size;
        }
        MEM_UNLOCK(unit, mem);
    }
    return SOC_E_NONE;
}

int
soc_mem_cache_scache_get(int unit)
{
    uint32 alloc_get;
    soc_scache_handle_t scache_handle;
    uint8 *mem_scache_ptr;
    int stable_size;

    if (NULL == SOC_CONTROL(unit)->mem_scache_ptr) {
        SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));
        if ((stable_size > SOC_DEFAULT_LVL2_STABLE_SIZE) &&
            !SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
            SOC_SCACHE_HANDLE_SET(scache_handle, unit,
                                  SOC_SCACHE_MEMCACHE_HANDLE, 0);
            /* Get the pointer for the Level 2 cache */
            SOC_IF_ERROR_RETURN
                (soc_scache_ptr_get(unit, scache_handle,
                                    &mem_scache_ptr, &alloc_get));
            if (alloc_get == 0) {
                return SOC_E_INTERNAL;
            } else if (NULL == mem_scache_ptr) {
                return SOC_E_MEMORY;
            }
            SOC_CONTROL(unit)->mem_scache_ptr = mem_scache_ptr;
        }
    }                            
    return SOC_E_NONE;
}

int
soc_mem_cache_scache_load(int unit, soc_mem_t mem, int *offset)
{
    int blk, entry_dw, index_cnt;
    int cache_size, vmap_size;
    uint32 *cache;
    uint8 *vmap, *mem_scache_ptr = SOC_CONTROL(unit)->mem_scache_ptr;
    
    if (SOC_WARM_BOOT(unit) && SOC_CONTROL(unit)->mem_scache_ptr) {
        entry_dw = soc_mem_entry_words(unit, mem);
        index_cnt = soc_mem_index_count(unit, mem);
        cache_size = index_cnt * entry_dw * 4;
        vmap_size = (index_cnt + 7) / 8;
            
        SOC_MEM_BLOCK_ITER(unit, mem, blk) {
            if (SOC_MEM_STATE(unit, mem).cache[blk] == NULL) {
                continue;
            }
            cache = SOC_MEM_STATE(unit, mem).cache[blk];
            vmap = SOC_MEM_STATE(unit, mem).vmap[blk];
            
            soc_cm_debug(DK_SOCMEM+DK_VERBOSE, "Load from %d %s to %s\n", *offset, 
                         mem_scache_ptr + *offset, SOC_MEM_UFNAME(unit, mem));
            *offset += 128;
            sal_memcpy(cache, mem_scache_ptr + *offset, cache_size);
            *offset += cache_size;
            sal_memcpy(vmap, mem_scache_ptr + *offset, vmap_size);
            *offset += vmap_size;
        }
    }
    return SOC_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/************************************************************************/
/* Routines for CFAPINIT                                 */
/************************************************************************/

#ifdef BCM_ESW_SUPPORT
/*
 * Function:
 *    cfapinit_write_cb (internal)
 * Purpose:
 *    Memory test callback routine for cfapinit
 */

static int
cfapinit_write_cb(struct soc_mem_test_s *parm,
                  unsigned array_index,
                  int copyno,
                  int index,
                  uint32 *entry_data)
{
    return soc_mem_array_write(parm->unit, parm->mem, array_index, copyno,
                               index, entry_data);
}

/*
 * Function:
 *    cfapinit_read_cb (internal)
 * Purpose:
 *    Memory test callback routine for cfapinit
 */

static int
cfapinit_read_cb(struct soc_mem_test_s *parm,
                 unsigned array_index,
                 int copyno,
                 int index,
                 uint32 *entry_data)
{
    return soc_mem_array_read(parm->unit, parm->mem, array_index, copyno, index, 
                              entry_data);
}

/*
 * Function:
 *    cfapinit_miscompare_cb (internal)
 * Purpose:
 *    Memory test callback routine for cfapinit
 */

static int
cfapinit_miscompare_cb(struct soc_mem_test_s *parm,
                       unsigned array_index,
                       int copyno,
                       int index,
                       uint32 *read_data,
                       uint32 *wrote_data,
                       uint32 *mask_data)
{
    uint8    *bad_list = parm->userdata;

    COMPILER_REFERENCE(array_index);
    COMPILER_REFERENCE(copyno);
    COMPILER_REFERENCE(read_data);
    COMPILER_REFERENCE(wrote_data);
    COMPILER_REFERENCE(mask_data);

    bad_list[index - parm->index_start] = 1;

    return 1;
}

#endif /* BCM_ESW_SUPPORT */
/*
 * Function:
 *    soc_mem_cfap_init
 * Purpose:
 *    Routine to diagnose and initialize CFAP pool.
 * Notes:
 *    Ordinarily this routine is not used.  When the chip is reset, it
 *    automatically writes the CFAP table with the correct pointer
 *    data.
 *
 *    This routine does the same thing, except it also runs a memory
 *    test and excludes bad entries in CBPHEADER/CBPDATA[0-3] from the
 *    CFAP pool.
 *
 *    On some devices, errors in the first 63 entries can't be eliminated.
 *    The routine fails if such errors are found.
 */

#ifdef BCM_ESW_SUPPORT
int
soc_mem_cfap_init(int unit)
{
    soc_mem_test_t    mt;
    uint8        *bad_list;
    int            entry_count, bad_count, bad_count_63;
    int            i, ptr, rv;
    uint32        cfappoolsize;
    soc_mem_t           cbpcellheader_m;
    soc_mem_t           cbppktheader_start_m;
    soc_mem_t           cbppktheader_end_m;
    soc_mem_t           cbpdata_start_m;
    soc_mem_t           cbpdata_end_m;
    soc_mem_t           cfap_m;

    if (!SOC_IS_XGS_SWITCH(unit)) {
        return SOC_E_NONE;
    }

    if (!soc_feature(unit, soc_feature_cfap_pool)) {
        return SOC_E_UNAVAIL;
    }

    cbpcellheader_m      = MMU_CBPCELLHEADERm;
    cbppktheader_start_m = MMU_CBPPKTHEADER0m;
    cbppktheader_end_m   = MMU_CBPPKTHEADER1m;
    cbpdata_start_m      = MMU_CBPDATA0m;
    cbpdata_end_m        = MMU_CBPDATA15m;
    cfap_m               = MMU_CFAPm;
    SOC_IF_ERROR_RETURN(READ_CFAPCONFIGr(unit, &cfappoolsize));
    cfappoolsize = soc_reg_field_get(unit, CFAPCONFIGr,
                                     cfappoolsize, CFAPPOOLSIZEf);

    mt.unit          = unit;
    mt.patterns      = soc_property_get(unit, spn_CFAP_TESTS,
                                        MT_PAT_CHECKER | MT_PAT_ICHECKER);
    mt.copyno        = COPYNO_ALL;
    mt.index_start   = soc_mem_index_min(unit, cbpcellheader_m);
    mt.index_end     = soc_mem_index_max(unit, cbpcellheader_m);
    mt.index_step    = 1;
    mt.read_count    = 1;
    mt.status_cb     = 0;            /* No status */
    mt.write_cb      = cfapinit_write_cb;
    mt.read_cb       = cfapinit_read_cb;
    mt.miscompare_cb = cfapinit_miscompare_cb;

    /*
     * Create array of status per entry, initially with all entries
     * marked good.  The test callback will mark bad entries.
     */

    entry_count = mt.index_end - mt.index_start + 1;

    if ((bad_list = sal_alloc(entry_count, "bad_list")) == NULL) {
        return SOC_E_MEMORY;
    }

    sal_memset(bad_list, 0, entry_count);

    /*
     * Test the five "parallel" memories: CBPHEADER and CBPDATA[0-3].
     */

    mt.userdata = bad_list;
    mt.mem = cbpcellheader_m;
    soc_cm_debug(DK_VERBOSE,
                 "soc_mem_cfap_init: unit %d: testing CBPHEADER\n",
                 unit);

    if ((rv = soc_mem_parity_control(unit, mt.mem,
                                     mt.copyno, FALSE)) < 0) {
        soc_cm_debug(DK_ERR,
                    "Unable to disable parity warnings on %s\n",
                    SOC_MEM_UFNAME(unit, mt.mem));
        goto done;
    }

    (void) soc_mem_test(&mt);

    if ((rv = soc_mem_parity_control(unit, mt.mem,
                                     mt.copyno, TRUE)) < 0) {
        soc_cm_debug(DK_ERR,
                    "Unable to disable parity warnings on %s\n",
                    SOC_MEM_UFNAME(unit, mt.mem));
        goto done;
    }

    for (mt.mem = cbppktheader_start_m;
         mt.mem <= cbppktheader_end_m;
         mt.mem++) {
        soc_cm_debug(DK_VERBOSE,
                     "soc_mem_cfap_init: unit %d: testing CBPPKTHEADER%d\n",
                     unit, mt.mem - cbppktheader_start_m);

        if ((rv = soc_mem_parity_control(unit, mt.mem,
                                         mt.copyno, FALSE)) < 0) {
            soc_cm_debug(DK_ERR,
                         "Unable to disable parity warnings on %s\n",
                         SOC_MEM_UFNAME(unit, mt.mem));
            goto done;
        }
        (void) soc_mem_test(&mt);

        if ((rv = soc_mem_parity_control(unit, mt.mem,
                                         mt.copyno, TRUE)) < 0) {
            soc_cm_debug(DK_ERR,
                         "Unable to disable parity warnings on %s\n",
                         SOC_MEM_UFNAME(unit, mt.mem));
            goto done;
        }
    }

    for (mt.mem = cbpdata_start_m;
         mt.mem <= cbpdata_end_m;
         mt.mem++) {
        soc_cm_debug(DK_VERBOSE,
                     "soc_mem_cfap_init: unit %d: testing CBPDATA%d\n",
                     unit, mt.mem - cbpdata_start_m);

        if ((rv = soc_mem_parity_control(unit, mt.mem,
                                         mt.copyno, FALSE)) < 0) {
            soc_cm_debug(DK_ERR,
                        "Unable to disable parity warnings on %s\n",
                        SOC_MEM_UFNAME(unit, mt.mem));
            goto done;
        }
        (void) soc_mem_test(&mt);

        if ((rv = soc_mem_parity_control(unit, mt.mem,
                                         mt.copyno, TRUE)) < 0) {
            soc_cm_debug(DK_ERR,
                        "Unable to disable parity warnings on %s\n",
                        SOC_MEM_UFNAME(unit, mt.mem));
            goto done;
        }
    }

    /*
     * Test only
     for (i = 1; i < 512; i += 2) {
         if (i < entry_count) {
             bad_list[i] = 1;
         }
     }
     */

    bad_count = 0;
    bad_count_63 = 0;

    for (i = mt.index_start; i <= mt.index_end; i++) {
        if (bad_list[i - mt.index_start]) {
            bad_count++;
            if (i < mt.index_start + 63) {
                bad_count_63++;
            }
        }
    }

    if (bad_count_63 > 0) {
        /*
         * On some devices the first 63 entries MUST be good since they are
         * allocated by the hardware on startup.
         */

        soc_cm_print("soc_mem_cfap_init: unit %d: "
                     "Chip unusable, %d error(s) in low entries\n",
                     unit, bad_count_63);
        sal_free(bad_list);
        return SOC_E_FAIL;
    }

    if (bad_count >= entry_count / 2) {
        soc_cm_print("soc_mem_cfap_init: unit %d: "
                     "Chip unusable, too many bad entries (%d)\n",
                     unit, bad_count);
        sal_free(bad_list);
        return SOC_E_FAIL;
    }

    /*
     * For each bad entry, swap it with a good entry taken from the end
     * of the list.  This puts all bad entries at the end of the table
     * where they will not be used.
     */

    ptr = mt.index_end + 1;

    for (i = mt.index_start; i < ptr; i++) {
        uint32        e_good, e_bad;

        if (bad_list[i - mt.index_start]) {
            soc_cm_debug(DK_VERBOSE,
                         "soc_mem_cfap_init: unit %d: "
                         "mapping out CBP index %d\n",
                         unit, i);

            do {
                --ptr;
            } while (bad_list[ptr - mt.index_start]);

            if ((rv = soc_mem_read(unit, cfap_m,
                                   MEM_BLOCK_ANY, i, &e_bad)) < 0) {
                sal_free(bad_list);
                return rv;
            }

            if ((rv = soc_mem_read(unit, cfap_m,
                                   MEM_BLOCK_ANY, ptr, &e_good)) < 0) {
                sal_free(bad_list);
                return rv;
            }

            if ((rv = soc_mem_write(unit, cfap_m,
                                    MEM_BLOCK_ANY, ptr, &e_bad)) < 0) {
                sal_free(bad_list);
                return rv;
            }

            if ((rv = soc_mem_write(unit, cfap_m,
                                    MEM_BLOCK_ANY, i, &e_good)) < 0) {
                sal_free(bad_list);
                return rv;
            }
        }
    }

    /*
     * Write CFAPPOOLSIZE with the number of good entries (minus 1).
     */

    if (bad_count > 0) {
        uint32  cfap_config = 0;
        uint32  val;
        uint32  cfap_thresh;

        soc_cm_print("soc_mem_cfap_init: unit %d: "
                     "detected and removed %d bad entries\n",
                     unit, bad_count);

        cfappoolsize -= bad_count;

        soc_reg_field_set(unit, CFAPCONFIGr, &cfap_config,
                          CFAPPOOLSIZEf, cfappoolsize);

        if ((rv = WRITE_CFAPCONFIGr(unit, cfappoolsize)) < 0) {
            sal_free(bad_list);
            return rv;
        }

        if ((rv = READ_CFAPFULLTHRESHOLDr(unit, &val)) < 0) {
            sal_free(bad_list);
            return rv;
        }
        cfap_thresh = soc_reg_field_get(unit, CFAPFULLTHRESHOLDr,
                                        val, CFAPFULLRESETPOINTf);
        cfap_thresh -= bad_count;

        soc_reg_field_set(unit, CFAPFULLTHRESHOLDr, &val,
                          CFAPFULLRESETPOINTf, cfap_thresh);

        cfap_thresh = soc_reg_field_get(unit, CFAPFULLTHRESHOLDr,
                                        val, CFAPFULLSETPOINTf);
        cfap_thresh -= bad_count;
        soc_reg_field_set(unit, CFAPFULLTHRESHOLDr, &val,
                          CFAPFULLSETPOINTf, cfap_thresh);
        if ((rv = WRITE_CFAPFULLTHRESHOLDr(unit, val)) < 0) {
            sal_free(bad_list);
            return rv;
        }
    }

done:

    sal_free(bad_list);

    return SOC_E_NONE;
}

#endif /* BCM_ESW_SUPPORT */


/************************************************************************/
/* Routines for Filter Mask START/COUNT Maintenance                     */
/************************************************************************/

#if defined(BCM_XGS_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || \
    defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DFE_SUPPORT) ||\
    defined(BCM_PETRA_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)

/************************************************************************
 * Table DMA
 *
 *    In hardware, it shares the same buffers and registers used for ARL DMA
 *    In software, it shares with ARL DMA the same semaphore and interrupt
 *        handler
 * Assumption
 *    ARL DMA and Table DMA should not co-exist. The buffer should be
 *        pre-allocated at least (index_max - index_min + 1) * sizeof (entry)
 *
 ************************************************************************/

#define SOC_MEM_DMA_MAX_DATA_BEATS    4

/*
 * Function:
 *    soc_mem_dmaable
 * Purpose:
 *    Determine whether a table is DMA_able
 * Returns:
 *    0 if not, 1 otherwise
 */
int
soc_mem_dmaable(int unit, soc_mem_t mem, int copyno)
{
    assert(SOC_MEM_IS_VALID(unit, mem));

    if (SOC_CONTROL(unit)->tableDmaMutex == 0) {    /* not enabled */
        return FALSE;
    }

#if defined(BCM_RCPU_SUPPORT)
    if(SOC_IS_RCPU_ONLY(unit)) {
        return FALSE;
    }
#endif

#if defined(BCM_DFE_SUPPORT)
    if(SOC_IS_FE1600(unit)) {
#ifndef PLISIM
        return TRUE;
#else
        return FALSE;
#endif /* PLISIM */
    }
#endif /* BCM_DFE_SUPPORT */

#if defined(BCM_PETRA_SUPPORT)
    if(SOC_IS_ARAD(unit)) {
#ifndef PLISIM
        return TRUE;
#else
        return FALSE;
#endif /* PLISIM */
    }
#endif /* BCM_PETRA_SUPPORT */


#if defined(BCM_FIREBOLT_SUPPORT)
#if defined(SOC_MEM_L3_DEFIP_WAR) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_l3_defip_map) &&
        (mem == L3_DEFIPm ||
         mem == L3_DEFIP_ONLYm ||
         mem == L3_DEFIP_DATA_ONLYm ||
         mem == L3_DEFIP_HIT_ONLYm ||
         mem == L3_DEFIP_HIT_ONLY_Xm ||
         mem == L3_DEFIP_HIT_ONLY_Ym)) {
        return FALSE;   /* Could be non-contiguous */
    }
#endif /* SOC_MEM_L3_DEFIP_WAR || BCM_TRIUMPH3_SUPPORT */
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_l3_defip_hole) &&
        (mem == L3_DEFIPm ||
         mem == L3_DEFIP_ONLYm ||
         mem == L3_DEFIP_DATA_ONLYm ||
         mem == L3_DEFIP_HIT_ONLYm)) {
        return FALSE;
    }
    if ((mem == LMEPm) || (mem == LMEP_1m)) {
        return FALSE;
    }
    if (mem == EFP_TCAMm) {
        /* Do not use dma for tcams with holes */
        if (soc_feature(unit, soc_feature_field_stage_half_slice)) {
            if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || SOC_IS_VALKYRIE2(unit))  {
                return FALSE;
            }
        }
     }

#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_l3_defip_map) &&
        (mem == L3_DEFIP_PAIR_128m ||
         mem == L3_DEFIP_PAIR_128_ONLYm ||
         mem == L3_DEFIP_PAIR_128_DATA_ONLYm ||
         mem == L3_DEFIP_PAIR_128_HIT_ONLYm ||
         mem == L3_DEFIP_PAIR_128_HIT_ONLY_Xm ||
         mem == L3_DEFIP_PAIR_128_HIT_ONLY_Ym)) {
        return FALSE;
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) || defined(BCM_TRIUMPH2_SUPPORT)
    if (mem == FP_GLOBAL_MASK_TCAMm || mem == FP_TCAMm || mem == EFP_TCAMm) {
        /* Do not use dma for tcams with holes */
        if (soc_feature(unit, soc_feature_field_stage_half_slice)) {
            return FALSE;
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT || BCM_HURRICANE2_SUPPORT*/

#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit) && !SOC_IS_TD2_TT2(unit) && !SAL_BOOT_BCMSIM) {
        if (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_MMU) {
            switch (mem) {
            case CTR_FLEX_COUNT_0m: case CTR_FLEX_COUNT_1m: case CTR_FLEX_COUNT_2m:
            case CTR_FLEX_COUNT_3m: case CTR_FLEX_COUNT_4m: case CTR_FLEX_COUNT_5m:
            case CTR_FLEX_COUNT_6m: case CTR_FLEX_COUNT_7m: case CTR_FLEX_COUNT_8m:
            case CTR_FLEX_COUNT_9m: case CTR_FLEX_COUNT_10m: case CTR_FLEX_COUNT_11m: 
                return TRUE;
            default:
                return FALSE;
            }
        }
    }
#endif /* BCM_TRX_SUPPORT */
    if (!soc_feature(unit, soc_feature_flexible_dma_steps) &&
        soc_mem_index_count(unit, mem) > 1) {
#ifdef BCM_TRIUMPH_SUPPORT
        if (soc_feature(unit, soc_feature_esm_support) &&
            SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) {
            int index0, index1;
            soc_mem_t real_mem;

            /* On BCM56624_A0, don't do DMA for tables whose associated TCAM
             * entry is wider than a single raw TCAM entry */
            soc_tcam_mem_index_to_raw_index(unit, mem, 0, &real_mem, &index0);
            soc_tcam_mem_index_to_raw_index(unit, mem, 1, &real_mem, &index1);
            if (index1 - index0 != 1) {
                return FALSE;
            }
        }
#endif /* BCM_TRIUMPH_SUPPORT */
    }

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TRIDENT(unit) || SOC_IS_TITAN(unit)) {
        /*
         * EGR_VLAN requires to combine entries from both pipe
         * the single entry ISBS_PORT_TO_PIPE_MAPPING and
         * ESBS_PORT_TO_PIPE_MAPPING table need data_beat size adjustment
         */
        if (mem == EGR_VLANm ||
            mem == ISBS_PORT_TO_PIPE_MAPPINGm ||
            mem == ESBS_PORT_TO_PIPE_MAPPINGm) {
            return FALSE;
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        if (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_IBOD) {
            return FALSE;
        }
     
    }
    if (soc_feature(unit, soc_feature_field_stage_quarter_slice)) {
        /* Do not use dma for tcams with holes */
        if (mem == FP_GLOBAL_MASK_TCAMm || mem == FP_TCAMm || 
            mem == VFP_TCAMm || mem == EFP_TCAMm || mem == FP_COUNTER_TABLEm) {
            return FALSE; 
        } 
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        if (mem == EFP_TCAMm) {
            /* Probable Non Contiguous.. Need to Investigate */
            return FALSE;
        }
    }
#endif /* BCM_HURRICANE2_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        /*
         * IL_STAT_MEM_x counters 
         */
        if (mem == IL_STAT_MEM_3m ) {
            return FALSE;
        }
    }
#endif /* BCM_SHADOW_SUPPORT */


#ifdef  BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        return TRUE;
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
      return TRUE;
    }
#endif /* BCM_SIRIUS_SUPPORT */

#ifdef BCM_CALADAN3_SUPPORT
    if (SOC_IS_CALADAN3(unit)) {
      return TRUE;
    }
#endif /* BCM_CALADAN3_SUPPORT */

    return (soc_mem_entry_words(unit, mem) <= SOC_MEM_DMA_MAX_DATA_BEATS);
}

/*
 * Function:
 *    soc_mem_slamable
 * Purpose:
 *    Determine whether a table is SLAMable
 * Returns:
 *    0 if not, 1 otherwise
 */
int
soc_mem_slamable(int unit, soc_mem_t mem, int copyno)
{
    assert(SOC_MEM_IS_VALID(unit, mem));

    if (SOC_CONTROL(unit)->tslamDmaMutex == 0) {        /* not enabled */
        return FALSE;
    }

#if defined(BCM_RCPU_SUPPORT)
    if(SOC_IS_RCPU_ONLY(unit)) {
        return FALSE;
    }
#endif

#if defined(BCM_DFE_SUPPORT)
    if(SOC_IS_FE1600(unit)) {
#ifndef PLISIM
        return TRUE;
#else
        return FALSE;
#endif /* PLISIM */
    }
#endif /* BCM_DFE_SUPPORT */

#if defined(BCM_PETRA_SUPPORT)
    if (SOC_IS_DPP(unit)) {
#ifndef PLISIM
        return TRUE;
#else
        return FALSE;
#endif /* PLISIM */
    }
#endif /* BCM_PETRA_SUPPORT */

#if defined(BCM_FIREBOLT_SUPPORT)
#if defined(SOC_MEM_L3_DEFIP_WAR) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_l3_defip_map) &&
        (mem == L3_DEFIPm ||
         mem == L3_DEFIP_ONLYm ||
         mem == L3_DEFIP_DATA_ONLYm ||
         mem == L3_DEFIP_HIT_ONLYm ||
         mem == L3_DEFIP_HIT_ONLY_Xm ||
         mem == L3_DEFIP_HIT_ONLY_Ym)) {
        return FALSE;   /* Could be non-contiguous */
    }
#endif /* SOC_MEM_L3_DEFIP_WAR || BCM_TRIUMPH3_SUPPORT */
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_l3_defip_hole) &&
        (mem == L3_DEFIPm ||
         mem == L3_DEFIP_ONLYm ||
         mem == L3_DEFIP_DATA_ONLYm ||
         mem == L3_DEFIP_HIT_ONLYm)) {
        return FALSE;
    }
    if ((mem == LMEPm) || (mem == LMEP_1m)) {
        return FALSE;
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_l3_defip_map) &&
        (mem == L3_DEFIP_PAIR_128m ||
         mem == L3_DEFIP_PAIR_128_ONLYm ||
         mem == L3_DEFIP_PAIR_128_DATA_ONLYm ||
         mem == L3_DEFIP_PAIR_128_HIT_ONLYm ||
         mem == L3_DEFIP_PAIR_128_HIT_ONLY_Xm ||
         mem == L3_DEFIP_PAIR_128_HIT_ONLY_Ym)) {
        return FALSE;
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit) && !SOC_IS_TD2_TT2(unit)) {
        if (!SAL_BOOT_BCMSIM && SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_MMU) {
            return FALSE;
        }
    }
#endif /* BCM_TRX_SUPPORT */

    if (!soc_feature(unit, soc_feature_flexible_dma_steps) &&
        soc_mem_index_count(unit, mem) > 1) {
#ifdef BCM_TRIUMPH_SUPPORT
        if (soc_feature(unit, soc_feature_esm_support) &&
            SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) {
            int index0, index1;
            soc_mem_t real_mem;

            /* On BCM56624_A0, don't do DMA for tables whose associated TCAM
             * entry is wider than a single raw TCAM entry */
            soc_tcam_mem_index_to_raw_index(unit, mem, 0, &real_mem, &index0);
            soc_tcam_mem_index_to_raw_index(unit, mem, 1, &real_mem, &index1);
            if (index1 - index0 != 1) {
                return FALSE;
            }
        }
#endif /* BCM_TRIUMPH_SUPPORT */
    }
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_CONTROL(unit)->l3_defip_index_remap && 
        (mem == L3_DEFIP_PAIR_128m || mem == L3_DEFIPm)) {
        return FALSE;
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
    return TRUE;
}

/*
 * Function:
 *    _soc_xgs3_mem_dma
 * Purpose:
 *    DMA acceleration for soc_mem_read_range() on FB/ER
 * Parameters:
 *    buffer -- must be pointer to sufficiently large
 *            DMA-able block of memory
 */
STATIC int
_soc_xgs3_mem_dma(int unit, soc_mem_t mem, unsigned array_index,
                  int copyno, int index_min, int index_max,
                  uint32 ser_flags, void *buffer)
{
    soc_control_t *soc = SOC_CONTROL(unit);
    uint32        start_addr;
    uint32        count;
    uint32        data_beats;
    uint32        spacing;
    int           rv = SOC_E_NONE;
    uint32        cfg, rval;
    soc_mem_t     cmd_mem = INVALIDm;
    uint8         at;
#if (defined  BCM_CMICM_SUPPORT) 
    int cmc = SOC_PCI_CMC(unit);
#endif

    /* coverity[var_tested_neg] */
    soc_cm_debug(DK_SOCMEM | DK_DMA, "_soc_xgs3_mem_dma: unit %d"
                 " mem %s.%s index %d-%d buffer %p\n",
                 unit, SOC_MEM_UFNAME(unit, mem), SOC_BLOCK_NAME(unit, copyno),
                 index_min, index_max, buffer);

    data_beats = soc_mem_entry_words(unit, mem);

    count = index_max - index_min + 1;
    if (count < 1) {
        return SOC_E_NONE;
    }

    if (cmd_mem != INVALIDm) {
        MEM_LOCK(unit, cmd_mem);
    }

    TABLE_DMA_LOCK(unit);

    /* coverity[negative_returns : FALSE] */
    start_addr = soc_mem_addr_get(unit, mem, array_index,
                                  copyno, index_min, &at);

#if defined(BCM_XGS3_SWITCH_SUPPORT)
    if (0 != (ser_flags & _SOC_SER_FLAG_MULTI_PIPE)) {
        uint32 acc_type = ser_flags & _SOC_SER_FLAG_ACC_TYPE_MASK;
        if (0 != acc_type) {
            /* Override ACC_TYPE in address */
            start_addr &= ~(_SOC_MEM_ADDR_ACC_TYPE_MASK <<
                            _SOC_MEM_ADDR_ACC_TYPE_SHIFT);
            start_addr |= (acc_type & _SOC_MEM_ADDR_ACC_TYPE_MASK) <<
                            _SOC_MEM_ADDR_ACC_TYPE_SHIFT;
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#if (defined  BCM_CMICM_SUPPORT)  
    if(soc_feature(unit, soc_feature_cmicm)) {
        soc_pci_write(unit, CMIC_CMCx_TABLE_DMA_PCIMEM_START_ADDR_OFFSET(cmc), 
                      soc_cm_l2p(unit, buffer));
        soc_pci_write(unit, CMIC_CMCx_TABLE_DMA_SBUS_START_ADDR_OFFSET(cmc),
                      start_addr);
        rval = 0;
        soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_ENTRY_COUNTr, &rval,
                          COUNTf, count);
        soc_pci_write(unit, CMIC_CMCx_TABLE_DMA_ENTRY_COUNT_OFFSET(cmc), rval);
    } else
#endif /* CMICM Support */
    {

        WRITE_CMIC_TABLE_DMA_PCIMEM_START_ADDRr(unit,
                                                soc_cm_l2p(unit, buffer));
        WRITE_CMIC_TABLE_DMA_SBUS_START_ADDRr(unit, start_addr);
        rval = 0;
        soc_reg_field_set(unit, CMIC_TABLE_DMA_ENTRY_COUNTr, &rval,
                          COUNTf, count);
        if (soc_feature(unit, soc_feature_flexible_dma_steps) &&
            soc_mem_index_count(unit, mem) > 1) {
#if defined(BCM_TRIUMPH_SUPPORT)
            if (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) {
                int index0, index1, increment;
                soc_mem_t real_mem;

                soc_tcam_mem_index_to_raw_index(unit, mem, 0,
                                                &real_mem, &index0);
                soc_tcam_mem_index_to_raw_index(unit, mem, 1,
                                                &real_mem, &index1);
                increment = _shr_popcount(index1 - index0 - 1);
                soc_reg_field_set(unit, CMIC_TABLE_DMA_ENTRY_COUNTr, &rval,
                                  SBUS_ADDR_INCREMENT_STEPf, increment);
            }
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT)
            if (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU) {
                int index0, index1, increment;
                soc_mem_t real_mem;

                soc_tcam_mem_index_to_raw_index(unit, mem, 0,
                                                &real_mem, &index0);
                soc_tcam_mem_index_to_raw_index(unit, mem, 1,
                                                &real_mem, &index1);
                increment = _shr_popcount(index1 - index0 - 1);
                soc_reg_field_set(unit, CMIC_TABLE_DMA_ENTRY_COUNTr, &rval,
                                  SBUS_ADDR_INCREMENT_STEPf, increment);
            }
#endif /* BCM_TRIUMPH3_SUPPORT */
        }
        WRITE_CMIC_TABLE_DMA_ENTRY_COUNTr(unit, rval);
    }
    soc_cm_debug(DK_SOCMEM | DK_DMA,
                 "_soc_xgs3_mem_dma: table dma of %d entries "
                 "of %d beats from 0x%x\n",
                 count, data_beats, start_addr);

    /* Set beats. Clear table DMA abort,done and error bit. Start DMA */
#if (defined  BCM_CMICM_SUPPORT)  
    if(soc_feature(unit, soc_feature_cmicm)) {
        /* For CMICm retain Endianess etc... */
        cfg = soc_pci_read(unit, CMIC_CMCx_TABLE_DMA_CFG_OFFSET(cmc));
        soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_CFGr, &cfg,
                            BEATSf, data_beats);
        soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_CFGr, &cfg, ABORTf, 0);
        soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_CFGr, &cfg, ENf, 0);
        soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_CFGr, &cfg,
                          ENABLE_MULTIPLE_SBUS_CMDSf, 0);
        soc_pci_write(unit, CMIC_CMCx_TABLE_DMA_CFG_OFFSET(cmc), cfg);
        /* Clearing EN clears stats */

        
#ifdef BCM_EXTND_SBUS_SUPPORT
        if (soc_feature(unit, soc_feature_new_sbus_format)) {
            rval = 0;
            /* Use TR3 style sbus access */
            soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_SBUS_CMD_CONFIGr,
                              &rval, EN_TR3_SBUS_STYLEf, 1);
            soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_SBUS_CMD_CONFIGr,
                              &rval, 
                              SBUS_BLOCKIDf, SOC_BLOCK2SCH(unit, copyno));
            soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_SBUS_CMD_CONFIGr,
                              &rval, 
                              SBUS_ACCTYPEf,
                              (soc_mem_flags(unit, mem) >>
                               SOC_MEM_FLAG_ACCSHIFT) & 7);
            WRITE_CMIC_CMC0_TABLE_DMA_SBUS_CMD_CONFIGr(unit, rval);
        }
#endif
    } else
#endif
    {
        cfg = 0;
        soc_reg_field_set(unit, CMIC_TABLE_DMA_CFGr, &cfg,
                            BEATSf, data_beats);
    }
    if (soc_feature(unit, soc_feature_multi_sbus_cmds)) {
        
        if (soc->sbusCmdSpacing < 0) {
            spacing = data_beats > 7 ? data_beats + 1 : 8;
        } else {
            spacing = soc->sbusCmdSpacing;
        }
        if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_XQPORT) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_GXPORT) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_SPORT) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_GPORT) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_QGPORT) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_LLS) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_MMU)) {
            spacing = 0;
        }

#ifdef BCM_KATANA_SUPPORT
       if (SOC_IS_KATANA(unit)) {
           /* disable MOR for EFP_TCAM and EGR_VLAN_XLATE */
           if (mem == EFP_TCAMm || mem == EGR_VLAN_XLATEm) {
               spacing = 0;
           } 
       }
#endif

        if (spacing) {
#if (defined  BCM_CMICM_SUPPORT) 
            if(soc_feature(unit, soc_feature_cmicm)) {
                soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_CFGr, &cfg,
                                  MULTIPLE_SBUS_CMD_SPACINGf, spacing);
                soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_CFGr, &cfg,
                                  ENABLE_MULTIPLE_SBUS_CMDSf, 1);

            } else
#endif
            {
                soc_reg_field_set(unit, CMIC_TABLE_DMA_CFGr, &cfg,
                                  MULTIPLE_SBUS_CMD_SPACINGf, spacing);
                soc_reg_field_set(unit, CMIC_TABLE_DMA_CFGr, &cfg,
                                  ENABLE_MULTIPLE_SBUS_CMDSf, 1);
            }
        }
    }
#if (defined  BCM_CMICM_SUPPORT) 
    if(soc_feature(unit, soc_feature_cmicm)) {
        soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_CFGr, &cfg, ENf, 1);
        soc_pci_write(unit, CMIC_CMCx_TABLE_DMA_CFG_OFFSET(cmc), cfg);
    } else
#endif
    {
        soc_reg_field_set(unit, CMIC_TABLE_DMA_CFGr, &cfg, ENf, 1);
        WRITE_CMIC_TABLE_DMA_CFGr(unit, cfg);
    }
    rv = SOC_E_TIMEOUT;
    if (soc->tableDmaIntrEnb) {
#if (defined  BCM_CMICM_SUPPORT) 
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_cmicm_intr0_enable(unit, IRQ_CMCx_TDMA_DONE);
            if (sal_sem_take(soc->tableDmaIntr, soc->tableDmaTimeout) < 0) {
                rv = SOC_E_TIMEOUT;
            }
            soc_cmicm_intr0_disable(unit, IRQ_CMCx_TDMA_DONE);

            cfg = soc_pci_read(unit, CMIC_CMCx_TABLE_DMA_STAT_OFFSET(cmc));
            if (soc_reg_field_get(unit, CMIC_CMC0_TABLE_DMA_STATr,
                                                        cfg, DONEf)) {
                rv = SOC_E_NONE;
                if (soc_reg_field_get(unit, CMIC_CMC0_TABLE_DMA_STATr,
                                                        cfg, ERRORf)) {
                    rv = SOC_E_FAIL;
                }
            }
        } else
#endif
        {
            soc_intr_enable(unit, IRQ_TDMA_DONE);
            if (sal_sem_take(soc->tableDmaIntr, soc->tableDmaTimeout) < 0) {
                rv = SOC_E_TIMEOUT;
            }
            soc_intr_disable(unit, IRQ_TDMA_DONE);

            READ_CMIC_TABLE_DMA_CFGr(unit, &cfg);
            if (soc_reg_field_get(unit, CMIC_TABLE_DMA_CFGr,
                    cfg, DONEf)) {
                rv = SOC_E_NONE;
                if (soc_reg_field_get(unit, CMIC_TABLE_DMA_CFGr,
                        cfg, ERRORf)) {
                    rv = SOC_E_FAIL;
                }
            }
        }
    } else {
        soc_timeout_t to;

        soc_timeout_init(&to, soc->tableDmaTimeout, 10000);

        do {
#if (defined  BCM_CMICM_SUPPORT)
            if(soc_feature(unit, soc_feature_cmicm)) {
                cfg = soc_pci_read(unit, CMIC_CMCx_TABLE_DMA_STAT_OFFSET(cmc));
                if (soc_reg_field_get(unit, CMIC_CMC0_TABLE_DMA_STATr,
                                                        cfg, DONEf)) {
                    rv = SOC_E_NONE;
                    if (soc_reg_field_get(unit, CMIC_CMC0_TABLE_DMA_STATr,
                                                        cfg, ERRORf)) {
                        rv = SOC_E_FAIL;
                    }
                    break;
                }
            } else
#endif
            {
                READ_CMIC_TABLE_DMA_CFGr(unit, &cfg);
                if (soc_reg_field_get(unit, CMIC_TABLE_DMA_CFGr,
                        cfg, DONEf)) {
                    rv = SOC_E_NONE;
                    if (soc_reg_field_get(unit, CMIC_TABLE_DMA_CFGr,
                            cfg, ERRORf)) {
                        rv = SOC_E_FAIL;
                    }
                    break;
                }
             }
        } while(!(soc_timeout_check(&to)));
    }

    if (rv < 0) {
        if (rv != SOC_E_TIMEOUT) {
            soc_cm_debug(DK_ERR, "%s: %s.%s failed(NAK)\n",
                         __FUNCTION__, SOC_MEM_UFNAME(unit, mem),
                         SOC_BLOCK_NAME(unit, copyno));
#ifdef BCM_TRIUMPH2_SUPPORT
            if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
                SOC_IS_VALKYRIE2(unit)) {
                /* SCHAN nack */
                soc_triumph2_mem_nack(INT_TO_PTR(unit),
                        INT_TO_PTR(start_addr), 0, 0, 0);
            }
#endif /* BCM_TRIUMPH2_SUPPORT */
        } else {
            soc_timeout_t to;

            soc_cm_debug(DK_ERR, "%s: %s.%s %s timeout\n",
                         __FUNCTION__, SOC_MEM_UFNAME(unit, mem),
                         SOC_BLOCK_NAME(unit, copyno),
                         soc->tableDmaIntrEnb ? "interrupt" : "polling");

#if (defined  BCM_CMICM_SUPPORT) 
            if(soc_feature(unit, soc_feature_cmicm)) {
                /* Abort Table DMA */
                cfg = soc_pci_read(unit, CMIC_CMCx_TABLE_DMA_CFG_OFFSET(cmc));
                soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_CFGr, &cfg, ENf, 0);
                soc_reg_field_set(unit, CMIC_CMC0_TABLE_DMA_CFGr, &cfg, ABORTf, 1);
                soc_pci_write(unit, CMIC_CMCx_TABLE_DMA_CFG_OFFSET(cmc), cfg);

                /* Check the done bit to confirm */
                soc_timeout_init(&to, soc->tableDmaTimeout, 0);
                while (1) {
                    cfg = soc_pci_read(unit, CMIC_CMCx_TABLE_DMA_STAT_OFFSET(cmc));
                    if (soc_reg_field_get(unit, CMIC_CMC0_TABLE_DMA_STATr, cfg, DONEf)) {
                        break;
                    }
                    if (soc_timeout_check(&to)) {
                        soc_cm_debug(DK_ERR,
                                     "_soc_xgs3_mem_dma: Abort Failed\n");
                        break;
                    }
                }
            } else
#endif
            {
                /* Abort Table DMA */
                READ_CMIC_TABLE_DMA_CFGr(unit, &cfg);
                soc_reg_field_set(unit, CMIC_TABLE_DMA_CFGr, &cfg, ENf, 0);
                soc_reg_field_set(unit, CMIC_TABLE_DMA_CFGr, &cfg, ABORTf, 1);
                soc_reg_field_set(unit, CMIC_TABLE_DMA_CFGr, &cfg, DONEf, 0);
                soc_reg_field_set(unit, CMIC_TABLE_DMA_CFGr, &cfg, ERRORf, 0);
                WRITE_CMIC_TABLE_DMA_CFGr(unit, cfg);
                
                /* Check the done bit to confirm */
                soc_timeout_init(&to, soc->tableDmaTimeout, 0);
                while (1) {
                    READ_CMIC_TABLE_DMA_CFGr(unit, &cfg);
                    if (soc_reg_field_get(unit, CMIC_TABLE_DMA_CFGr, cfg, DONEf)) {
                        break;
                    }
                    if (soc_timeout_check(&to)) {
                        soc_cm_debug(DK_ERR,
                                     "_soc_xgs3_mem_dma: Abort Failed\n");
                        break;
                    }
                }
            }
        }
    }

    soc_cm_sinval(unit, (void *)buffer, WORDS2BYTES(data_beats) * count);

    TABLE_DMA_UNLOCK(unit);

    if (cmd_mem != INVALIDm) {
        MEM_UNLOCK(unit, cmd_mem);
    }

    return rv;
}

/*
 * Function:
 *    _soc_xgs3_mem_slam
 * Purpose:
 *    DMA acceleration for soc_mem_write_range() on FB/ER
 * Parameters:
 *    buffer -- must be pointer to sufficiently large
 *            DMA-able block of memory
 *      index_begin <= index_end - Forward direction
 *      index_begin >  index_end - Reverse direction
 */

STATIC int
_soc_xgs3_mem_slam(int unit, uint32 flags, soc_mem_t mem, unsigned array_index, int copyno,
                   int index_begin, int index_end, void *buffer)
{
    soc_control_t *soc = SOC_CONTROL(unit);
    uint32        start_addr;
    uint32        count;
    uint32        data_beats;
    uint32        spacing;
    int           rv = SOC_E_NONE;
    uint32        cfg, rval;
    uint8         at;
#if (defined  BCM_CMICM_SUPPORT) 
    int cmc = SOC_PCI_CMC(unit);
#endif

    soc_cm_debug(DK_SOCMEM | DK_DMA, "_soc_xgs3_mem_slam: unit %d"
                 " mem %s.%s index %d-%d buffer %p\n",
                 unit, SOC_MEM_UFNAME(unit, mem), SOC_BLOCK_NAME(unit, copyno),
                 index_begin, index_end, buffer);
    
    data_beats = soc_mem_entry_words(unit, mem);
    
    if (index_begin > index_end) {
        /* coverity[negative_returns : FALSE] */
        start_addr = soc_mem_addr_get(unit, mem, array_index, copyno, index_end, &at);
        count = index_begin - index_end + 1;
    } else {
        /* coverity[negative_returns : FALSE] */
        start_addr = soc_mem_addr_get(unit, mem, array_index, copyno, index_begin, &at);
        count = index_end - index_begin + 1;
    }
        
    if (!flags || (flags & SOC_MEM_WRITE_SET_ONLY)) {
    
    #if (defined  BCM_CMICM_SUPPORT) 
        if(soc_feature(unit, soc_feature_cmicm)) {
            /* For CMICm retain Endianess etc... */
            cfg = soc_pci_read(unit, CMIC_CMCx_SLAM_DMA_CFG_OFFSET(cmc));
            soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_CFGr, &cfg, ABORTf, 0);
            soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_CFGr, &cfg, ENf, 0);
            soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_CFGr, &cfg,
                              ENABLE_MULTIPLE_SBUS_CMDSf, 0);
            soc_pci_write(unit, CMIC_CMCx_SLAM_DMA_CFG_OFFSET(cmc), cfg);  /* Clearing EN clears the stats */
    
            /* Set beats. Clear tslam DMA abort,done and error bit. Start DMA */
            soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_CFGr, &cfg, BEATSf, data_beats);
            soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_CFGr, &cfg, REV_MODULO_COUNTf,
                              (count % (64 / data_beats)));
            soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_CFGr, &cfg,
                                ORDERf, (index_begin > index_end) ? 1:0);
    
    #ifdef BCM_EXTND_SBUS_SUPPORT
            if (soc_feature(unit, soc_feature_new_sbus_format)) {
                rval = 0;
                /* Use TR3 style sbus access */
                soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_SBUS_CMD_CONFIGr, &rval, 
                                  EN_TR3_SBUS_STYLEf, 1);
                soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_SBUS_CMD_CONFIGr, &rval, 
                                  SBUS_BLOCKIDf, SOC_BLOCK2SCH(unit, copyno));
                soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_SBUS_CMD_CONFIGr, &rval, 
                                  SBUS_ACCTYPEf,
                                  (soc_mem_flags(unit, mem) >>
                                   SOC_MEM_FLAG_ACCSHIFT) & 7);
                WRITE_CMIC_CMC0_SLAM_DMA_SBUS_CMD_CONFIGr(unit, rval);
            }
    #endif
        } else
    #endif
        {
            /* Set beats. Clear tslam DMA abort,done and error bit. Start DMA */
            cfg = 0;
            soc_reg_field_set(unit, CMIC_SLAM_DMA_CFGr, &cfg, REV_MODULO_COUNTf,
                              (count % (64 / data_beats)));
            soc_reg_field_set(unit, CMIC_SLAM_DMA_CFGr, &cfg, BEATSf, data_beats);
            soc_reg_field_set(unit, CMIC_SLAM_DMA_CFGr, &cfg,
                                ORDERf, (index_begin > index_end) ? 1:0);
        }
        if (soc_feature(unit, soc_feature_multi_sbus_cmds)) {
            
            if (soc->sbusCmdSpacing < 0) {
                spacing = data_beats > 7 ? data_beats + 1 : 8;
            } else {
                spacing = soc->sbusCmdSpacing;
            }
            if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) ||
                (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_XQPORT) ||
                (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_GXPORT) ||
                (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_XLPORT) ||
                (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_SPORT) ||
                (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_GPORT) ||
                (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_QGPORT) ||
                (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_LLS) ||
                (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_MMU)) {
                spacing = 0;
            }
    
    #ifdef BCM_KATANA_SUPPORT 
           if (SOC_IS_KATANA(unit)) {
               /* disable MOR for EFP_TCAM and EGR_VLAN_XLATE */
               if (mem == EFP_TCAMm || mem == EGR_VLAN_XLATEm) {
                   spacing = 0;
               }
           } 
    #endif
    
            if (spacing) {
    #if (defined  BCM_CMICM_SUPPORT) 
                if(soc_feature(unit, soc_feature_cmicm)) {
                    soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_CFGr, &cfg,
                                      MULTIPLE_SBUS_CMD_SPACINGf, spacing);
                    soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_CFGr, &cfg,
                                      ENABLE_MULTIPLE_SBUS_CMDSf, 1);
                } else
    #endif
                {
                    soc_reg_field_set(unit, CMIC_SLAM_DMA_CFGr, &cfg,
                                      MULTIPLE_SBUS_CMD_SPACINGf, spacing);
                    soc_reg_field_set(unit, CMIC_SLAM_DMA_CFGr, &cfg,
                                      ENABLE_MULTIPLE_SBUS_CMDSf, 1);
                }
            }
        }
    
        soc_cm_debug(DK_SOCMEM | DK_DMA,
                     "_soc_xgs3_mem_slam: tslam dma of %d entries "
                     "of %d beats from 0x%x to index %d-%d\n",
                     count, data_beats, start_addr, index_begin, index_end);
    
        TSLAM_DMA_LOCK(unit);
    
        soc_cm_sflush(unit, buffer, WORDS2BYTES(data_beats) * count);
    #if (defined  BCM_CMICM_SUPPORT) 
        if(soc_feature(unit, soc_feature_cmicm)) {
            soc_pci_write(unit, CMIC_CMCx_SLAM_DMA_PCIMEM_START_ADDR_OFFSET(cmc), 
                          soc_cm_l2p(unit, buffer));
            soc_pci_write(unit, CMIC_CMCx_SLAM_DMA_SBUS_START_ADDR_OFFSET(cmc), start_addr);
            rval = 0;
            soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_ENTRY_COUNTr, &rval, COUNTf, count);
            soc_pci_write(unit, CMIC_CMCx_SLAM_DMA_ENTRY_COUNT_OFFSET(cmc), rval);
            soc_pci_write(unit, CMIC_CMCx_SLAM_DMA_CFG_OFFSET(cmc), cfg);
        } else
    #endif
        {
            WRITE_CMIC_SLAM_DMA_PCIMEM_START_ADDRr(unit, soc_cm_l2p(unit, buffer));
            WRITE_CMIC_SLAM_DMA_SBUS_START_ADDRr(unit, start_addr);
            rval = 0;
            soc_reg_field_set(unit, CMIC_SLAM_DMA_ENTRY_COUNTr, &rval, COUNTf, count);
            if (soc_feature(unit, soc_feature_flexible_dma_steps) &&
                soc_mem_index_count(unit, mem) > 1) {
    #if defined(BCM_TRIUMPH_SUPPORT)
                if (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) {
                    int index0, index1, increment;
                    soc_mem_t real_mem;
    
                    soc_tcam_mem_index_to_raw_index(unit, mem, 0, &real_mem, &index0);
                    soc_tcam_mem_index_to_raw_index(unit, mem, 1, &real_mem, &index1);
                    increment = _shr_popcount(index1 - index0 - 1);
                    soc_reg_field_set(unit, CMIC_SLAM_DMA_ENTRY_COUNTr, &rval,
                                      SBUS_ADDR_INCREMENT_STEPf, increment);
                }
    #endif /* BCM_TRIUMPH_SUPPORT */
    #if defined(BCM_TRIUMPH3_SUPPORT)
                if (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU) {
                    int index0, index1, increment;
                    soc_mem_t real_mem;
    
                    soc_tcam_mem_index_to_raw_index(unit, mem, 0, &real_mem, &index0);
                    soc_tcam_mem_index_to_raw_index(unit, mem, 1, &real_mem, &index1);
                    increment = _shr_popcount(index1 - index0 - 1);
                    soc_reg_field_set(unit, CMIC_SLAM_DMA_ENTRY_COUNTr, &rval,
                                      SBUS_ADDR_INCREMENT_STEPf, increment);
                }
    #endif /* BCM_TRIUMPH3_SUPPORT */
            }
            WRITE_CMIC_SLAM_DMA_ENTRY_COUNTr(unit, rval);
            WRITE_CMIC_SLAM_DMA_CFGr(unit, cfg);
        }
    }
    
    if (!flags || (flags & SOC_MEM_WRITE_COMMIT_ONLY)) {
        cfg = 0;
        /* write ENf bit to SLAM_DMA_CFG register */
        #if (defined  BCM_CMICM_SUPPORT) 
        if(soc_feature(unit, soc_feature_cmicm)) {
            cfg = soc_pci_read(unit, CMIC_CMCx_SLAM_DMA_CFG_OFFSET(cmc));
            soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_CFGr, &cfg, ENf, 1);
            soc_pci_write(unit, CMIC_CMCx_SLAM_DMA_CFG_OFFSET(cmc), cfg);
        } else
    #endif
        {
            READ_CMIC_SLAM_DMA_CFGr(unit, &cfg);
            soc_reg_field_set(unit, CMIC_SLAM_DMA_CFGr, &cfg, ENf, 1);
            WRITE_CMIC_SLAM_DMA_CFGr(unit, cfg);
        }
    }
    
    if (!flags || (flags & SOC_MEM_WRITE_STATUS_ONLY)) {
        
        rv = SOC_E_TIMEOUT;
        if (soc->tslamDmaIntrEnb) {
    
    #if (defined  BCM_CMICM_SUPPORT) 
            if(soc_feature(unit, soc_feature_cmicm)) {
                soc_cmicm_intr0_enable(unit, IRQ_CMCx_TSLAM_DONE);
                if (sal_sem_take(soc->tslamDmaIntr, soc->tslamDmaTimeout) < 0) {
                    rv = SOC_E_TIMEOUT;
                }
                soc_cmicm_intr0_disable(unit, IRQ_CMCx_TSLAM_DONE);
    
                cfg = soc_pci_read(unit, CMIC_CMCx_SLAM_DMA_STAT_OFFSET(cmc));
                if (soc_reg_field_get(unit, CMIC_CMC0_SLAM_DMA_STATr, cfg, DONEf)) {
                    rv = SOC_E_NONE;
                }
                if (soc_reg_field_get(unit, CMIC_CMC0_SLAM_DMA_STATr, cfg, ERRORf)) {
                    rv = SOC_E_FAIL;
                }
            } else
    #endif
            {
                soc_intr_enable(unit, IRQ_TSLAM_DONE);
                if (sal_sem_take(soc->tslamDmaIntr, soc->tslamDmaTimeout) < 0) {
                    rv = SOC_E_TIMEOUT;
                }
                soc_intr_disable(unit, IRQ_TSLAM_DONE);
    
                READ_CMIC_SLAM_DMA_CFGr(unit, &cfg);
                if (soc_reg_field_get(unit, CMIC_SLAM_DMA_CFGr, cfg, DONEf)) {
                    rv = SOC_E_NONE;
                }
                if (soc_reg_field_get(unit, CMIC_SLAM_DMA_CFGr, cfg, ERRORf)) {
                    rv = SOC_E_FAIL;
                }
            }
        } else {
            soc_timeout_t to;
    
            soc_timeout_init(&to, soc->tslamDmaTimeout, 10000);
    
            do {
    #if (defined  BCM_CMICM_SUPPORT) 
                if(soc_feature(unit, soc_feature_cmicm)) {
                    cfg = soc_pci_read(unit, CMIC_CMCx_SLAM_DMA_STAT_OFFSET(cmc));
                    if (soc_reg_field_get(unit, CMIC_CMC0_SLAM_DMA_STATr, cfg, ERRORf)) {
                        rv = SOC_E_FAIL;
                        break;
                    }
                    if (soc_reg_field_get(unit, CMIC_CMC0_SLAM_DMA_STATr, cfg, DONEf)) {
                        rv = SOC_E_NONE;
                        break;
                    }
                } else
    #endif 
                {
                    READ_CMIC_SLAM_DMA_CFGr(unit, &cfg);
                    if (soc_reg_field_get(unit, CMIC_SLAM_DMA_CFGr, cfg, ERRORf)) {
                        rv = SOC_E_FAIL;
                        break;
                    }
                    if (soc_reg_field_get(unit, CMIC_SLAM_DMA_CFGr, cfg, DONEf)) {
                        rv = SOC_E_NONE;
                        break;
                    }
                }
            } while (!(soc_timeout_check(&to)));
        }
    
        if (rv < 0) {
            if (rv != SOC_E_TIMEOUT) {
                soc_cm_debug(DK_ERR, "%s: %s.%s failed(NAK)\n",
                             __FUNCTION__, SOC_MEM_UFNAME(unit, mem),
                             SOC_BLOCK_NAME(unit, copyno));
    #ifdef BCM_TRIUMPH2_SUPPORT
                if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
                    SOC_IS_VALKYRIE2(unit)) {
                    /* SCHAN nack */
                    soc_triumph2_mem_nack(INT_TO_PTR(unit),
                            INT_TO_PTR(start_addr), 0, 0, 0);
                }
    #endif /* BCM_TRIUMPH2_SUPPORT */
            } else {
                soc_timeout_t to;
    
                soc_cm_debug(DK_ERR, "%s: %s.%s %s timeout\n",
                             __FUNCTION__, SOC_MEM_UFNAME(unit, mem),
                             SOC_BLOCK_NAME(unit, copyno),
                             soc->tslamDmaIntrEnb ? "interrupt" : "polling");
                
    #if (defined  BCM_CMICM_SUPPORT) 
                if(soc_feature(unit, soc_feature_cmicm)) {
                    /* Abort Tslam DMA */
                    cfg = soc_pci_read(unit, CMIC_CMCx_SLAM_DMA_CFG_OFFSET(cmc));
                    soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_CFGr, &cfg, ENf, 0);
                    soc_reg_field_set(unit, CMIC_CMC0_SLAM_DMA_CFGr, &cfg, ABORTf, 1);
                    soc_pci_write(unit, CMIC_CMCx_SLAM_DMA_CFG_OFFSET(cmc), cfg);
    
                    /* Check the done bit to confirm */
                    soc_timeout_init(&to, soc->tslamDmaTimeout, 10000);
                    while (1) {
                        cfg = soc_pci_read(unit, CMIC_CMCx_SLAM_DMA_STAT_OFFSET(cmc));
                        if (soc_reg_field_get(unit, CMIC_CMC0_SLAM_DMA_STATr, cfg, DONEf)) {
                            break;
                        }
                        if (soc_timeout_check(&to)) {
                            soc_cm_debug(DK_ERR,
                                         "_soc_xgs3_mem_slam: Abort Failed\n");
                            break;
                        }
                    }
                } else
    #endif
                {
                    /* Abort Tslam DMA */
                    READ_CMIC_SLAM_DMA_CFGr(unit, &cfg);
                    soc_reg_field_set(unit, CMIC_SLAM_DMA_CFGr, &cfg, ENf, 0);
                    soc_reg_field_set(unit, CMIC_SLAM_DMA_CFGr, &cfg, ABORTf, 1);
                    soc_reg_field_set(unit, CMIC_SLAM_DMA_CFGr, &cfg, DONEf, 0);
                    soc_reg_field_set(unit, CMIC_SLAM_DMA_CFGr, &cfg, ERRORf, 0);
                    WRITE_CMIC_SLAM_DMA_CFGr(unit, cfg);
    
                    /* Check the done bit to confirm */
                    soc_timeout_init(&to, soc->tslamDmaTimeout, 10000);
                    while (1) {
                        READ_CMIC_SLAM_DMA_CFGr(unit, &cfg);
                        if (soc_reg_field_get(unit, CMIC_SLAM_DMA_CFGr, cfg, DONEf)) {
                            break;
                        }
                        if (soc_timeout_check(&to)) {
                            soc_cm_debug(DK_ERR,
                                         "_soc_xgs3_mem_slam: Abort Failed\n");
                            break;
                        }
                    }
                }
            }
        }
    
        TSLAM_DMA_UNLOCK(unit);
    }

    return rv;
}

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
STATIC void
_soc_mem_tcam_xy_to_dm(int unit, soc_mem_t mem, int count,
                       uint32 *xy_entry, uint32 *dm_entry)
{
    uint32 key[SOC_MAX_MEM_FIELD_WORDS], mask[SOC_MAX_MEM_FIELD_WORDS];
    soc_field_t key_field[4], mask_field[4];
    int bit_length[4], word_length[4], field_count;
    int index, i, word, data_words, data_bytes;
    soc_pbmp_t pbmp;

    if (mem == L3_DEFIPm || mem == L3_DEFIP_Xm || mem == L3_DEFIP_Ym ||
        mem == L3_DEFIP_ONLYm
        ) {
        if (SOC_MEM_FIELD_VALID(unit, mem, KEY0f)) {
            key_field[0] = KEY0f;
            key_field[1] = KEY1f;
            mask_field[0] = MASK0f;
            mask_field[1] = MASK1f;
            field_count = 2;
        } else {
            key_field[0] = KEYf;
            mask_field[0] = MASKf;
            field_count = 1;
        }
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA_SUPPORT) || \
    defined(BCM_TRIDENT2_SUPPORT)
    } else if (mem == L3_DEFIP_PAIR_128m || mem == L3_DEFIP_PAIR_128_ONLYm) {
        key_field[0] = KEY0_UPRf;
        key_field[1] = KEY1_UPRf;
        key_field[2] = KEY0_LWRf;
        key_field[3] = KEY1_LWRf;
        mask_field[0] = MASK0_UPRf;
        mask_field[1] = MASK1_UPRf;
        mask_field[2] = MASK0_LWRf;
        mask_field[3] = MASK1_LWRf;
        field_count = 4;
#endif
    } else {
        if (SOC_MEM_FIELD_VALID(unit, mem, FULL_KEYf)) {
            key_field[0] = FULL_KEYf;
            mask_field[0] = FULL_MASKf;
        } else {
            key_field[0] = KEYf;
            mask_field[0] = MASKf;
        }
        field_count = 1;
    }
    for (i = 0; i < field_count; i++) {
        bit_length[i] = soc_mem_field_length(unit, mem, key_field[i]);
        word_length[i] = (bit_length[i] + 31) / 32;
    }
    data_words = soc_mem_entry_words(unit, mem);
    data_bytes = data_words * sizeof(uint32);
    for (index = 0; index < count; index++) {
        if (dm_entry != xy_entry) {
            sal_memcpy(dm_entry, xy_entry, data_bytes);
        }
        for (i = 0; i < field_count; i++) {
            soc_mem_field_get(unit, mem, xy_entry, key_field[i], key);
            soc_mem_field_get(unit, mem, xy_entry, mask_field[i], mask);
            for (word = 0; word < word_length[i]; word++) {
                mask[word] = key[word] | ~mask[word];
            }
            if ((bit_length[i] & 0x1f) != 0) {
                mask[word - 1] &= (1 << (bit_length[i] & 0x1f)) - 1;
            }
            soc_mem_field_set(unit, mem, dm_entry, mask_field[i], mask);
        }

        /* Filter out the bits that is not implemented in the hardware,
         * hardware returns X=0 Y=0 which is D=0 M=1 after above conversion
         * following code will force such bits to D=0 M=0 */
        if (SOC_IS_TD_TT(unit) &&
            (mem == FP_GLOBAL_MASK_TCAM_Xm ||
             mem == FP_GLOBAL_MASK_TCAM_Ym)) {
            soc_mem_pbmp_field_get(unit, mem, dm_entry, IPBM_MASKf, &pbmp);
            if (mem == FP_GLOBAL_MASK_TCAM_Xm) {
                SOC_PBMP_AND(pbmp, PBMP_XPIPE(unit));
            } else {
                SOC_PBMP_AND(pbmp, PBMP_YPIPE(unit));
            }
            soc_mem_pbmp_field_set(unit, mem, dm_entry, IPBM_MASKf, &pbmp);
        }

        xy_entry += data_words;
        dm_entry += data_words;
    }
}

void
_soc_mem_tcam_dm_to_xy(int unit, soc_mem_t mem, int count,
                       uint32 *dm_entry, uint32 *xy_entry,
                       uint32 *cache_entry)
{
    uint32 key[SOC_MAX_MEM_FIELD_WORDS], mask[SOC_MAX_MEM_FIELD_WORDS];
    uint32 converted_key, converted_mask;
    soc_field_t key_field[4], mask_field[4];
    int bit_length[4], word_length[4], field_count;
    int index, i, word, data_words, data_bytes;
    int no_trans = FALSE;

    if (!soc_feature(unit, soc_feature_xy_tcam_direct)) {
        /* Only clear the "don't care" key bits */
        no_trans = TRUE;
    }
    if (mem == L3_DEFIPm || mem == L3_DEFIP_Xm || mem == L3_DEFIP_Ym ||
        mem == L3_DEFIP_ONLYm
        ) {
        if (SOC_MEM_FIELD_VALID(unit, mem, KEY0f)) {
            key_field[0] = KEY0f;
            key_field[1] = KEY1f;
            mask_field[0] = MASK0f;
            mask_field[1] = MASK1f;
            field_count = 2;
        } else {
            key_field[0] = KEYf;
            mask_field[0] = MASKf;
            field_count = 1;
        }
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA_SUPPORT) || \
    defined(BCM_TRIDENT2_SUPPORT)
    } else if (mem == L3_DEFIP_PAIR_128m || mem == L3_DEFIP_PAIR_128_ONLYm) {
        key_field[0] = KEY0_UPRf;
        key_field[1] = KEY1_UPRf;
        key_field[2] = KEY0_LWRf;
        key_field[3] = KEY1_LWRf;
        mask_field[0] = MASK0_UPRf;
        mask_field[1] = MASK1_UPRf;
        mask_field[2] = MASK0_LWRf;
        mask_field[3] = MASK1_LWRf;
        field_count = 4;
#endif
    } else {
        if (SOC_MEM_FIELD_VALID(unit, mem, FULL_KEYf)) {
            key_field[0] = FULL_KEYf;
            mask_field[0] = FULL_MASKf;
        } else {
            key_field[0] = KEYf;
            mask_field[0] = MASKf;
        }
        field_count = 1;
    }
    for (i = 0; i < field_count; i++) {
        bit_length[i] = soc_mem_field_length(unit, mem, key_field[i]);
        word_length[i] = (bit_length[i] + 31) / 32;
    }
    data_words = soc_mem_entry_words(unit, mem);
    data_bytes = data_words * sizeof(uint32);
    for (index = 0; index < count; index++) {
        if (xy_entry != dm_entry) {
            sal_memcpy(xy_entry, dm_entry, data_bytes);
        }
        if (cache_entry) {
            sal_memcpy(cache_entry, dm_entry, data_bytes);
        }
        for (i = 0; i < field_count; i++) {
            soc_mem_field_get(unit, mem, dm_entry, key_field[i], key);
            soc_mem_field_get(unit, mem, dm_entry, mask_field[i], mask);
            for (word = 0; word < word_length[i]; word++) {
                converted_key = key[word] & mask[word];
                if (!no_trans) {
                    converted_mask = key[word] | ~mask[word];
                    mask[word] = converted_mask;
                }
                key[word] = converted_key;
            }
            if ((bit_length[i] & 0x1f) != 0) {
                mask[word - 1] &= (1 << (bit_length[i] & 0x1f)) - 1;
            }
            soc_mem_field_set(unit, mem, xy_entry, key_field[i], key);
            soc_mem_field_set(unit, mem, xy_entry, mask_field[i], mask);
            if (cache_entry) {
                soc_mem_field_set(unit, mem, cache_entry, key_field[i], key);
            }
        }
        dm_entry += data_words;
        xy_entry += data_words;
        if (cache_entry) {
            cache_entry += data_words;
        }
    }
}

STATIC int
_soc_mem_tcam_entry_preserve(int unit, soc_mem_t mem, int copyno, int index,
                             int count, void *entry, soc_mem_t *aggr_mem,
                             void **buffer)
{
    int rv;
    int blk, index_max, entry_words, data_bytes, alloc_size, word_idx, i;
    void *shift_buf;
    uint32 read_buf[SOC_MAX_MEM_WORDS], *entry_ptr;
    int field_count, field_idx;
    soc_field_t fields[4];

    *buffer = NULL;

    switch (mem) {
    case FP_GLOBAL_MASK_TCAMm:
    case FP_GLOBAL_MASK_TCAM_Xm:
    case FP_GLOBAL_MASK_TCAM_Ym:
    case FP_GM_FIELDSm:
    case FP_TCAMm:
    case FP_UDF_TCAMm:
    case EFP_TCAMm:
    case VFP_TCAMm:
        return SOC_E_NONE;
    case CPU_COS_MAP_ONLYm:
        *aggr_mem = CPU_COS_MAPm;
        break;
    case L2_USER_ENTRY_ONLYm:
        *aggr_mem = L2_USER_ENTRYm;
        break;
    case L3_DEFIP_128_ONLYm:
        *aggr_mem = L3_DEFIP_128m; /* will miss hit bit from one pipe */
        break;
    case L3_DEFIP_ONLYm:
        *aggr_mem = L3_DEFIPm; /* will miss hit bit from one pipe */
        break;
    case MY_STATION_TCAM_ENTRY_ONLYm:
        *aggr_mem = MY_STATION_TCAMm;
        break;
    case VLAN_SUBNET_ONLYm:
        *aggr_mem = VLAN_SUBNETm;
        break;
    default:
        *aggr_mem = mem;
        break;
    }

    if (*aggr_mem != mem && count > 1) {
        /* Can't support DMA multiple entries on tcam only table */ 
        return SOC_E_NONE;
    }

    if (copyno == COPYNO_ALL) {
        SOC_MEM_BLOCK_ITER(unit, *aggr_mem, blk) {
            copyno = blk;
            break;
        }
    }

    /* Check if any valid entry will be overwritten */
    if (SOC_MEM_FIELD_VALID(unit, mem, VALIDf)) {
        fields[0] = VALIDf;
        field_count = 1;
    } else if (SOC_MEM_FIELD_VALID(unit, mem, VALID0f)) {
        fields[0] = VALID0f;
        fields[1] = VALID1f;
        field_count = 2;
    } else if (SOC_MEM_FIELD_VALID(unit, mem, VALID0_LWRf)) {
        fields[0] = VALID0_LWRf;
        fields[1] = VALID1_LWRf;
        fields[2] = VALID0_UPRf;
        fields[3] = VALID1_UPRf;
        field_count = 4;
    } else {
        field_count = 0;
    }
    entry_words = soc_mem_entry_words(unit, mem);
    entry_ptr = entry;
    for (i = 0; i < count; i++) {
        /* Read the original entry */
        rv = soc_mem_read(unit, mem, copyno, index + i, read_buf);
        if (rv < 0) {
            soc_cm_print("read %s index %d fail rv=%d\n",
                         SOC_MEM_NAME(unit, mem), index + i, rv);
            return rv;
        }
        /* Bit-wise 'and' the original entry with new entry */ 
        for (word_idx = 0; word_idx < entry_words; word_idx++) {
            read_buf[word_idx] &= entry_ptr[word_idx];
        }
        /* Find if original and new entry are both valid */
        for (field_idx = 0; field_idx < field_count; field_idx++) {
            if (soc_mem_field32_get(unit, *aggr_mem, read_buf,
                                    fields[field_idx])) {
                break;
            }
        }
        if (field_idx != field_count) {
            break;
        }
        entry_ptr += entry_words;
    }
    if (i == count) { /* don't need to preserve entry */
        return SOC_E_NONE;
    }

    data_bytes = soc_mem_entry_words(unit, *aggr_mem) * sizeof(uint32);
    index_max = soc_mem_index_max(unit, *aggr_mem);
    alloc_size = (index_max - index + 1) * data_bytes;
    shift_buf = soc_cm_salloc(unit, alloc_size + data_bytes, "shift buffer");
    if (shift_buf == NULL) {
        return SOC_E_MEMORY;
    }

#ifdef BCM_SBUSDMA_SUPPORT
    if (soc_feature(unit, soc_feature_sbusdma)) {
        rv = _soc_mem_sbusdma_read(unit, *aggr_mem, copyno, index, index_max,
                                   0, shift_buf);
    } else
#endif
    {
        rv = _soc_xgs3_mem_dma(unit, *aggr_mem, 0, copyno, index, index_max,
                               0, shift_buf);
    }
    if (SOC_FAILURE(rv)) {
        soc_cm_sfree(unit, shift_buf);
        return rv;
    }
    _soc_mem_tcam_dm_to_xy(unit, *aggr_mem, 1,
                           soc_mem_entry_null(unit, *aggr_mem),
                           &((uint32 *)shift_buf)
                           [alloc_size / sizeof(uint32)], NULL);

#ifdef BCM_SBUSDMA_SUPPORT
    if (soc_feature(unit, soc_feature_sbusdma)) {
        rv = _soc_mem_sbusdma_write(unit, *aggr_mem, copyno, index_max + 1,
                                    index + 1, shift_buf, FALSE, -1);
    } else
#endif
    {
        rv = _soc_xgs3_mem_slam(unit, 0, *aggr_mem, 0, copyno, index_max + 1,
                                index + 1, shift_buf);
    }

    if (SOC_FAILURE(rv)) {
        soc_cm_sfree(unit, shift_buf);
        return rv;
    }

    *buffer = shift_buf;
    return rv;
}

STATIC int
_soc_mem_tcam_entry_restore(int unit, soc_mem_t mem, int copyno, int index,
                            int count, void *buffer)
{
    int rv;
    int blk, index_max, data_words;
    uint32 *buf_ptr;

    if (buffer == NULL) {
        return SOC_E_NONE;
    }

    if (copyno == COPYNO_ALL) {
        SOC_MEM_BLOCK_ITER(unit, mem, blk) {
            copyno = blk;
            break;
        }
    }

    data_words = soc_mem_entry_words(unit, mem);
    index_max = soc_mem_index_max(unit, mem);
    buf_ptr = &((uint32 *)buffer)[count * data_words];

#ifdef BCM_SBUSDMA_SUPPORT
    if (soc_feature(unit, soc_feature_sbusdma)) {
        rv = _soc_mem_sbusdma_write(unit, mem, copyno, index + count,
                                    index_max + 1, buf_ptr, FALSE, -1);
    } else
#endif
    {
        rv = _soc_xgs3_mem_slam(unit, 0, mem, 0, copyno, index + count,
                                index_max + 1, buf_ptr);
    }

    soc_cm_sfree(unit, buffer);

    return rv;
}
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */

/*
 * Function:
 *     _soc_mem_dma_read
 * Purpose:
 *     DMA acceleration for soc_mem_read_range()
 * Parameters:
 *     buffer -- must be pointer to sufficiently large
 *               DMA-able block of memory
 */
STATIC int
_soc_mem_dma_read(int unit, soc_mem_t mem, unsigned array_index, int copyno,
                  int index_min, int index_max, uint32 ser_flags,
                  void *buffer)
{
    /* Remove assertions from here, because they are checked in calling
     * functions or intentionally excepted for SER correction
     * and Warm Boot.
     */

#ifdef BCM_SBUSDMA_SUPPORT
    if (soc_feature(unit, soc_feature_sbusdma)) {
        SOC_IF_ERROR_RETURN
            (_soc_mem_array_sbusdma_read(unit, mem, array_index,
                                         copyno, index_min, index_max,
                                         ser_flags, buffer));
    } else
#endif
    {
        SOC_IF_ERROR_RETURN
            (_soc_xgs3_mem_dma(unit, mem, array_index, copyno, index_min,
                               index_max, ser_flags, buffer));
    }

    if (0 != (ser_flags & _SOC_SER_FLAG_XY_READ)) {
        /* SER memscan logic must have the raw entries */
        return SOC_E_NONE;
    }

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    if (soc_feature(unit, soc_feature_xy_tcam_direct) &&
        (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) &&
        (!(SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_EXT_CAM))) {
        _soc_mem_tcam_xy_to_dm(unit, mem, index_max - index_min + 1,
                               buffer, buffer);
    }
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */

    return SOC_E_NONE;
}

/*
 * Function:
 *     _soc_mem_dma_write
 * Purpose:
 *     DMA acceleration for soc_mem_write_range()
 * Parameters:
 *     buffer -- must be pointer to sufficiently large
 *               DMA-able block of memory
 *     index_begin <= index_end - Forward direction
 *     index_begin >  index_end - Reverse direction
 */
STATIC int
_soc_mem_dma_write(int unit, uint32 flags, soc_mem_t mem, unsigned array_index, int copyno,
                   int index_begin, int index_end, void *buffer, void *cache_buffer)
{
    int           rv;
    void          *buffer_ptr;
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    int           count, alloc_size;
    soc_mem_t     aggr_mem = INVALIDm;
    void          *shift_buffer = NULL;
#endif

    if (SOC_HW_ACCESS_DISABLE(unit)) {
        return SOC_E_NONE;
    }

    assert(SOC_MEM_BLOCK_VALID(unit, mem, copyno));
#ifndef BCM_WARM_BOOT_SUPPORT
    assert(soc_mem_index_valid(unit, mem, index_begin));
    assert(soc_mem_index_valid(unit, mem, index_end));
#endif

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    if (soc_feature(unit, soc_feature_xy_tcam) &&
        (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) &&
        (!(SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_EXT_CAM))) {
        if (index_begin > index_end) {
            count = index_begin - index_end + 1;
        } else {
            count = index_end - index_begin + 1;
        }
        alloc_size = count * soc_mem_entry_words(unit, mem) * sizeof(uint32);
        buffer_ptr = soc_cm_salloc(unit, alloc_size, "converted buffer");
        if (buffer_ptr == NULL) {
            return SOC_E_MEMORY;
        }
        _soc_mem_tcam_dm_to_xy(unit, mem, count, buffer, buffer_ptr, 
                               cache_buffer);

        if (index_begin <= index_end &&
            SOC_CONTROL(unit)->tcam_protect_write) {
            rv = _soc_mem_tcam_entry_preserve(unit, mem, copyno, index_begin,
                                              count, buffer_ptr, &aggr_mem,
                                              &shift_buffer);
            if (SOC_FAILURE(rv)) {
                soc_cm_sfree(unit, buffer_ptr);
                return rv;
            }
        }
    } else
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */
    {
        buffer_ptr = buffer;
    }

#ifdef BCM_SBUSDMA_SUPPORT
    if (soc_feature(unit, soc_feature_sbusdma)) {
        rv = _soc_mem_array_sbusdma_write(unit, mem, array_index, array_index,
                                          copyno, index_begin, index_end,
                                          buffer_ptr, FALSE, -1);
    } else
#endif
    {
        rv = _soc_xgs3_mem_slam(unit, flags, mem, array_index, copyno, index_begin,
                                index_end, buffer_ptr);
    }

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    if (soc_feature(unit, soc_feature_xy_tcam) &&
        (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) &&
        (!(SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_EXT_CAM))) {
#ifdef INCLUDE_MEM_SCAN
        if (NULL != cache_buffer) {
            /* Update memscan TCAM cache if necessary */
            soc_mem_scan_tcam_cache_update(unit, mem,
                                           index_begin, index_end,
                                           buffer_ptr);
        }
#endif /* INCLUDE_MEM_SCAN */
        soc_cm_sfree(unit, buffer_ptr);
        if (index_begin <= index_end &&
            SOC_CONTROL(unit)->tcam_protect_write) {
            SOC_IF_ERROR_RETURN
                (_soc_mem_tcam_entry_restore(unit, aggr_mem, copyno,
                                             index_begin,
                                             index_end - index_begin + 1,
                                             shift_buffer));
        }
    }
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */

    return rv;
}




#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
/*
 * Function:
 *      _soc_xgs3_mem_clear_slam
 * Purpose:
 *    soc_mem_clear acceleration using table DMA write (slam)
 */
STATIC int
_soc_xgs3_mem_clear_slam(int unit, soc_mem_t mem, 
                         int copyno, void *null_entry)
{
    int       rv, chunk_size, chunk_entries, mem_size, entry_words;
    int       index, index_end, index_min, index_max;
    uint32    *buf;

    if (SOC_WARM_BOOT(unit) || SOC_IS_RELOADING(unit)) {
        return SOC_E_NONE;
    }
#ifdef BCM_SBUSDMA_SUPPORT
    if (soc_feature(unit, soc_feature_sbusdma)) {
        uint32 *null_ptr;
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
        uint32 null_buf[SOC_MAX_MEM_WORDS];
        if (soc_feature(unit, soc_feature_xy_tcam) &&
            (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) &&
            (!(SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_EXT_CAM))) {
            null_ptr = null_buf;
            _soc_mem_tcam_dm_to_xy(unit, mem, 1, null_entry, null_ptr, NULL);
        } else
#endif /* defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) */
        {
            null_ptr = null_entry;
        }
        return _soc_mem_sbusdma_clear(unit, mem, copyno, null_ptr);
    }
#endif
    chunk_size = SOC_MEM_CLEAR_CHUNK_SIZE_GET(unit);  
    index_min = soc_mem_index_min(unit, mem);
    index_max = soc_mem_index_max(unit, mem);
    entry_words = soc_mem_entry_words(unit, mem);
    mem_size = (index_max - index_min + 1) * entry_words * 4;
    if (mem_size < chunk_size) {
        chunk_size = mem_size;
    }

    buf = soc_cm_salloc(unit, chunk_size, "mem_clear_buf");
    if (buf == NULL) {
        return SOC_E_MEMORY;
    }

    chunk_entries = chunk_size / (entry_words * 4);

    if (null_entry == _soc_mem_entry_null_zeroes) {
        sal_memset(buf, 0, chunk_size);
    } else {
        for (index = 0; index < chunk_entries; index++) {
            sal_memcpy(buf + (index * entry_words),
                       null_entry, entry_words * 4);
        }
    }

    rv = SOC_E_NONE;
    for (index = index_min; index <= index_max; index += chunk_entries) {
        index_end = index + chunk_entries - 1;
        if (index_end > index_max) {
            index_end = index_max;
        }

        rv = soc_mem_write_range(unit, mem, copyno, index, index_end, buf);
        if (rv < 0) {
            soc_cm_debug(DK_ERR,
                         "soc_mem_write_range: "
                         "write %s.%s[%d-%d] failed: %s\n",
                         SOC_MEM_UFNAME(unit, mem),
                         SOC_BLOCK_NAME(unit, copyno),
                         index, index_end, soc_errmsg(rv));
            break;
        }
    }

    soc_cm_sfree(unit, buf);
    return rv;
}

/*
 * Function:
 *      _soc_xgs3_mem_clear_pipe
 * Purpose:
 *    soc_mem_clear acceleration using pipeline table clear logic.
 *    Much faster than table slam.  Exists on XGS3.
 */
STATIC int
_soc_xgs3_mem_clear_pipe(int unit, soc_mem_t mem, int blk, void *null_entry)
{
    int rv, mementries, to_usec, to_rv;
    uint32 hwreset1, hwreset2, membase, memoffset, memstage;
    soc_timeout_t to;
#ifdef BCM_EXTND_SBUS_SUPPORT
    uint32 memnum;
#endif /* BCM_EXTND_SBUS_SUPPORT */

    if (null_entry != _soc_mem_entry_null_zeroes) {
        return SOC_E_UNAVAIL;
    }

    if (SOC_WARM_BOOT(unit) || SOC_IS_RELOADING(unit) || SOC_HW_ACCESS_DISABLE(unit)) {
        return SOC_E_NONE;
    }
    if (SAL_BOOT_PLISIM) {
        if (SAL_BOOT_BCMSIM || SAL_BOOT_XGSSIM) {
            if (!(SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit))) {
                /* h/w acceleration is not implemented in some models */
                return SOC_E_UNAVAIL;
            }
        } else {
            return SOC_E_UNAVAIL;
        }
    }

    if (!SOC_REG_IS_VALID(unit, ING_HW_RESET_CONTROL_1r)) {
        return SOC_E_UNAVAIL;
    }


#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        switch (mem) {
        case ING_PW_TERM_COUNTERSm:
        case EGR_IPFIX_SESSION_TABLEm:
        case EGR_IPFIX_EXPORT_FIFOm:
            return SOC_E_UNAVAIL;
        default:
            break;
        }
    }
#if defined(BCM_TRIUMPH2_SUPPORT)
    else if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
             SOC_IS_VALKYRIE2(unit)) {
        if (mem == LMEPm) {
            return SOC_E_UNAVAIL;
        }
        if (soc_feature(unit, soc_feature_ser_parity)) {
            SOC_IF_ERROR_RETURN
                (soc_triumph2_ser_mem_clear(unit, mem));
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_ENDURO_SUPPORT)
    else if (SOC_IS_ENDURO(unit)) {
        if (mem == LMEPm || mem == LMEP_1m) {
            return SOC_E_UNAVAIL;
        }
    }
#endif /* BCM_ENDURO_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
    else if (SOC_IS_TRIDENT(unit) || SOC_IS_TITAN(unit)) {
        if ((SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) ||
            (SOC_MEM_INFO(unit, mem).index_max == 0)) {
            return SOC_E_UNAVAIL;
        }
        switch (mem) {
        case EGR_VLANm:
            return SOC_E_UNAVAIL;
        default:
        break;
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    else if (SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
        if ((SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) ||
            (SOC_MEM_INFO(unit, mem).index_max == 0)) {
            return SOC_E_UNAVAIL;
        }
        if (SOC_IS_TRIUMPH3(unit)) {
            if (mem == EGR_MPLS_VC_AND_SWAP_LABEL_TABLEm || 
                mem == ING_DVP_2_TABLEm) {
                return SOC_E_UNAVAIL;
            }
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
    else if (SOC_IS_KATANA2(unit)) {
             switch(mem) {
             /* WORK-AROUND:SDK-48233
                KT2 SW WAR: IP HW Mem Reset does not reset 
                IARB_ING_PHYSICAL_PORT table  */
             case IARB_ING_PHYSICAL_PORTm:
                    return SOC_E_UNAVAIL;
             default:
                    break;
             }
    }
#endif /* BCM_KATANA2_SUPPORT */
#endif /* BCM_TRIUMPH_SUPPORT */

    if (SAL_BOOT_SIMULATION) {
        to_usec = 10000000;
    } else {
        to_usec = 50000;
    }
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit) && (mem == ING_VLAN_TAG_ACTION_PROFILEm)) {
       return SOC_E_UNAVAIL;
    }

    if (SOC_IS_SHADOW(unit)) {
        /* Reset HW Reset Control */
        SOC_IF_ERROR_RETURN(WRITE_ING_HW_RESET_CONTROL_2r(unit, 0));
        SOC_IF_ERROR_RETURN(WRITE_ING_HW_RESET_CONTROL_1r(unit, 0));
    }
#endif

    rv = SOC_E_NONE;
    to_rv = SOC_E_NONE;
    membase = SOC_MEM_BASE(unit, mem);
    mementries = soc_mem_index_count(unit, mem);
#ifdef BCM_EXTND_SBUS_SUPPORT
    memnum = 0;
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        if (SOC_BLOCK_TYPE(unit, blk) == SOC_BLK_IPIPE) {
            memnum = SOC_MEM_ADDR_NUM_EXTENDED(membase);
            /* For KATANA2: ING_HW_RESET_CONTROL_1:: OFFSET
             26-bit starting offset of table for memory. 
             This is {8-bit MEMSEL, 18-bit MEMIDX}
             Believe below condition should be out of "if BLK_IPIPE" condition 
             */
            if (SOC_IS_KATANA2(unit)) {
                memoffset = SOC_MEM_ADDR_OFFSET_EXTENDED(membase);
            } else { 
                memoffset = SOC_MEM_ADDR_OFFSET_EXTENDED_IPIPE(membase);
            }
        } else {
            memoffset = SOC_MEM_ADDR_OFFSET_EXTENDED(membase);
        }
        memstage = SOC_MEM_ADDR_STAGE_EXTENDED(membase);
    } else
#endif
    {
        memoffset = SOC_MEM_ADDR_OFFSET(membase);
        memstage = SOC_MEM_ADDR_STAGE(membase);
    }

    switch (SOC_BLOCK_TYPE(unit, blk)) {
    case SOC_BLK_IPIPE:
    case SOC_BLK_IPIPE_HI:
        hwreset1 = 0;
#ifdef BCM_EXTND_SBUS_SUPPORT
        if (soc_reg_field_valid(unit, ING_HW_RESET_CONTROL_1r,
                                MEMORY_NUMBERf)) {
            soc_reg_field_set(unit, ING_HW_RESET_CONTROL_1r, &hwreset1,
                  MEMORY_NUMBERf, memnum);
        }
#endif

         soc_reg_field_set(unit, ING_HW_RESET_CONTROL_1r, &hwreset1,
                   OFFSETf, memoffset);
         soc_reg_field_set(unit, ING_HW_RESET_CONTROL_1r, &hwreset1,
                   STAGE_NUMBERf, memstage);
         hwreset2 = 0;
         soc_reg_field_set(unit, ING_HW_RESET_CONTROL_2r, &hwreset2,
                   COUNTf, mementries);
         soc_reg_field_set(unit, ING_HW_RESET_CONTROL_2r, &hwreset2,
                   VALIDf, 1);
         
         rv = WRITE_ING_HW_RESET_CONTROL_1r(unit, hwreset1);
         if (rv < 0) {
             break;
         }
         rv = WRITE_ING_HW_RESET_CONTROL_2r(unit, hwreset2);
         if (rv < 0) {
             break;
         }
         
         soc_timeout_init(&to, to_usec, 100);
         for (;;) {
             rv = READ_ING_HW_RESET_CONTROL_2r(unit, &hwreset2);
             if (rv < 0) {
                 break;
             }
             if (soc_reg_field_get(unit, ING_HW_RESET_CONTROL_2r, hwreset2,
                                   DONEf)) {
                 break;
             }
             if (soc_timeout_check(&to)) {
                 to_rv = SOC_E_TIMEOUT;
                 break;
             }
         }
        hwreset2 = 0;
        if (soc_reg_field_valid(unit, ING_HW_RESET_CONTROL_2r, CMIC_REQ_ENABLEf)) {
            soc_reg_field_set(unit, ING_HW_RESET_CONTROL_2r, &hwreset2, 
                              CMIC_REQ_ENABLEf, 1);
        }
        rv = WRITE_ING_HW_RESET_CONTROL_2r(unit, hwreset2);
        if (rv < 0) {
            break;
        }
        rv = WRITE_ING_HW_RESET_CONTROL_1r(unit, 0);
        break;

    case SOC_BLK_EPIPE:
    case SOC_BLK_EPIPE_HI:
        hwreset1 = 0;
        soc_reg_field_set(unit, EGR_HW_RESET_CONTROL_0r, &hwreset1,
                  START_ADDRESSf, memoffset);
        soc_reg_field_set(unit, EGR_HW_RESET_CONTROL_0r, &hwreset1,
                  STAGE_NUMBERf, memstage);
        hwreset2 = 0;
        soc_reg_field_set(unit, EGR_HW_RESET_CONTROL_1r, &hwreset2,
                  COUNTf, mementries);
        soc_reg_field_set(unit, EGR_HW_RESET_CONTROL_1r, &hwreset2,
                  VALIDf, 1);
        
        rv = WRITE_EGR_HW_RESET_CONTROL_0r(unit, hwreset1);
        if (rv < 0) {
            break;
        }
        rv = WRITE_EGR_HW_RESET_CONTROL_1r(unit, hwreset2);
        if (rv < 0) {
            break;
        }
        
        soc_timeout_init(&to, to_usec, 100);
        for (;;) {
            rv = READ_EGR_HW_RESET_CONTROL_1r(unit, &hwreset2);
            if (rv < 0) {
                break;
            }
            if (soc_reg_field_get(unit, EGR_HW_RESET_CONTROL_1r, hwreset2,
                                  DONEf)) {
                break;
            }
            if (soc_timeout_check(&to)) {
                to_rv = SOC_E_TIMEOUT;
                break;
            }
        }
        
        rv = WRITE_EGR_HW_RESET_CONTROL_1r(unit, 0);
        if (rv < 0) {
            break;
        }
        rv = WRITE_EGR_HW_RESET_CONTROL_0r(unit, 0);
        break;

    case SOC_BLK_ISM:
        hwreset1 = 0;
        soc_reg_field_set(unit, ISM_HW_RESET_CONTROL_0r, &hwreset1,
                  START_ADDRESSf, memoffset);
        soc_reg_field_set(unit, ISM_HW_RESET_CONTROL_0r, &hwreset1,
                  TABLE_IDf, memstage);
        hwreset2 = 0;
        soc_reg_field_set(unit, ISM_HW_RESET_CONTROL_1r, &hwreset2,
                  COUNTf, mementries);
        soc_reg_field_set(unit, ISM_HW_RESET_CONTROL_1r, &hwreset2,
                  VALIDf, 1);
        
        rv = WRITE_ISM_HW_RESET_CONTROL_0r(unit, hwreset1);
        if (rv < 0) {
            break;
        }
        rv = WRITE_ISM_HW_RESET_CONTROL_1r(unit, hwreset2);
        if (rv < 0) {
            break;
        }
        
        soc_timeout_init(&to, to_usec, 100);
        for (;;) {
            rv = READ_ISM_HW_RESET_CONTROL_1r(unit, &hwreset2);
            if (rv < 0) {
                break;
            }
            if (soc_reg_field_get(unit, ISM_HW_RESET_CONTROL_1r, hwreset2,
                                  DONEf)) {
                break;
            }
            if (soc_timeout_check(&to)) {
                to_rv = SOC_E_TIMEOUT;
                break;
            }
        }
        
        rv = WRITE_ISM_HW_RESET_CONTROL_1r(unit, 0);
        if (rv < 0) {
            break;
        }
        rv = WRITE_ISM_HW_RESET_CONTROL_0r(unit, 0);
        break;

    default:
        rv = SOC_E_UNAVAIL;
        break;
    }

    return rv == SOC_E_NONE ? to_rv : rv;
}
#endif /* defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) */

#endif /* defined(BCM_XGS_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || 
          defined(BCM_CALADAN3_SUPPORT) || defined(BCM_DFE_SUPPORT) ||
          defined(BCM_PETRA_SUPPORT) */

/************************************************************************/
/* Routines for reading/writing all tables (except no write to ARL)    */
/************************************************************************/

/*
 * Function:
 *    soc_mem_read
 * Purpose:
 *    Read a memory internal to the SOC.
 * Notes:
 *    GBP/CBP memory should only accessed when MMU is in DEBUG mode.
 */

STATIC int
_soc_mem_read(int unit,
              soc_mem_t mem,
              unsigned array_index, /* in memory arrays this is the element index in the array, otherwise 0 */
              int copyno,
              int index,
              void *entry_data)
{
    schan_msg_t schan_msg, schan_msg_cpy;
    int entry_dw = soc_mem_entry_words(unit, mem);
    soc_mem_info_t *meminfo; 
    int index_valid;
    uint32 *cache;
    uint8 at, *vmap;
    int index2;
    uint32 maddr;
    int rv, allow_intr = 0;
    int resp_word = 0;
#ifdef INCLUDE_TCL
    uint32 entry_data_cache[SOC_MAX_MEM_FIELD_WORDS];
    uint32 cache_consistency_check = 0;
#endif

    if (!soc_mem_is_valid(unit, mem)) {
        return SOC_E_MEMORY;
    }
    
    if (soc_feature(unit, soc_feature_field_stage_ingress_256_half_slice)) { 
        if (mem == FP_GLOBAL_MASK_TCAMm ||  mem == FP_TCAMm) {
            if((index / 64) % 2) {
                return SOC_E_NONE;
            }
        } 
    }


    if (soc_feature(unit, soc_feature_field_stage_egress_256_half_slice)) {
        if (mem == EFP_TCAMm) {
            if((index / 128) % 2) {
                return SOC_E_NONE;
            }
        }
    }


    if (soc_feature(unit, soc_feature_field_stage_lookup_512_half_slice)) {
        if (mem == VFP_TCAMm) {
            if((index / 128) % 2) {
                return SOC_E_NONE;
            }
        }
    }
 
    if (!(soc_feature(unit, soc_feature_field_stage_lookup_512_half_slice) ||
          soc_feature(unit, soc_feature_field_stage_egress_256_half_slice) ||
          soc_feature(unit, soc_feature_field_stage_ingress_256_half_slice))) {
 
        if (!soc_feature(unit, soc_feature_field_stage_quarter_slice)) {
 
        if (mem == FP_GLOBAL_MASK_TCAMm || mem == FP_TCAMm) {
            /* Do not access h/w for tcam index mapping to the holes */
            if (soc_feature(unit, soc_feature_field_stage_half_slice)) { 
                if ((index/128) % 2) {
                    return SOC_E_NONE;
                }
            } else if (soc_feature(unit, soc_feature_field_slice_size128)) {
                if ((index/64) % 2) {
                    return SOC_E_NONE;
                }
            }
        }
        if (mem == EFP_TCAMm) {
            /* Do not access h/w for tcam index mapping to the holes */
            if (soc_feature(unit, soc_feature_field_stage_half_slice)) { 
                if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || SOC_IS_VALKYRIE2(unit))  {
                    if ((index/128) % 2) {
                        return SOC_E_NONE;
                    }
                }
            }
        }
    }
    }

    if (soc_feature(unit, soc_feature_field_stage_quarter_slice)) {
        /* Do not access h/w for tcam index mapping to the holes */
        if (mem == EFP_TCAMm) {
            if ((SOC_IS_TRIUMPH3(unit)) && ((index/128) % 2)) {
                return SOC_E_NONE;
            }
        }
        if(mem == FP_GLOBAL_MASK_TCAMm || mem == FP_TCAMm || mem == VFP_TCAMm) {
             if ((SOC_IS_TRIUMPH3(unit)) && (((index/128) % 2) || 
                                             ((index/64) % 2))) {
                 return SOC_E_NONE;
             }
        }
    }
    
    meminfo = &SOC_MEM_INFO(unit, mem);

    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_read: invalid block %d for memory %s\n",
                     copyno, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    /*
     * When checking index, check for 0 instead of soc_mem_index_min.
     * Diagnostics need to read/write index 0 of Strata ARL and GIRULE.
     */
    index_valid = (index >= 0 &&
                   index <= soc_mem_index_max(unit, mem));

    if (!index_valid) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_read: invalid index %d for memory %s\n",
                     index, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

#ifdef BCM_HERCULES_SUPPORT
    /* Handle Hercules' word read tables separately */
    if (SOC_IS_HERCULES(unit) &&
        (soc_mem_flags(unit, mem) & SOC_MEM_FLAG_WORDADR)) {
        return soc_hercules_mem_read(unit, mem, copyno, index, entry_data);
    }
#endif /* BCM_HERCULES_SUPPORT */

    rv = SOC_E_NONE;

    MEM_LOCK(unit, mem);

    /* Return data from cache if active */
    cache = SOC_MEM_STATE(unit, mem).cache[copyno];
    vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];

    if (index_valid && 
        !(SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE) &&
        (cache != NULL) && CACHE_VMAP_TST(vmap, index) && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
#ifdef INCLUDE_TCL
        /* Check contents of cache match device table */
        if ((SOC_IS_TD_TT(unit) &&
             (mem == FP_GLOBAL_MASK_TCAMm || mem == FP_GM_FIELDSm)) ||
             (!SOC_CONTROL(unit)->cache_coherency_chk)) {
            cache_consistency_check = 0;
        } else {
            cache_consistency_check = 1;
        }

        if (cache_consistency_check) {
            /* Check contents of cache match device table */
            sal_memcpy(entry_data_cache,
                       cache + index * entry_dw, entry_dw * 4);
        } else
#endif
        {
            sal_memcpy(entry_data, cache + index * entry_dw, entry_dw * 4);
            if (!SOC_CONTROL(unit)->force_read_through) {
                goto done;
            }
        }
    }

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc_feature(unit, soc_feature_always_drive_dbus) &&
        SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM &&
        soc_mem_is_cam(unit, mem)) {
        static const soc_field_t field_mask[] =
            {TMW0f, TMW1f, TMW2f, TMW3f, TMW4f, TMW5f};
        static const soc_field_t field_data[] =
            {TDW0f, TDW1f, TDW2f, TDW3f, TDW4f, TDW5f};
        uint32 tmp_entry[SOC_MAX_MEM_WORDS];
        uint32  sdata[SOC_MAX_MEM_WORDS], src_hit, dst_hit;
        soc_mem_t real_mem, defip_data_mem, src_hit_mem, dst_hit_mem, mask_mem;
        int raw_index, valid;
        int tcam_width, i;
        uint32 mask[4], data[4];

        rv = soc_tcam_mem_index_to_raw_index(unit, mem, index, &real_mem,
                                             &raw_index);
        if (SOC_FAILURE(rv)) {
            goto done;
        }
        switch (mem) {
        case EXT_ACL360_TCAM_DATAm:
        case EXT_ACL360_TCAM_DATA_IPV6_SHORTm:
            mask_mem = EXT_ACL360_TCAM_MASKm;
            break;
        case EXT_ACL432_TCAM_DATAm:
        case EXT_ACL432_TCAM_DATA_IPV6_LONGm:
        case EXT_ACL432_TCAM_DATA_L2_IPV4m:
        case EXT_ACL432_TCAM_DATA_L2_IPV6m:
            mask_mem = EXT_ACL432_TCAM_MASKm;
            break;
        default:
            mask_mem = INVALIDm;
            break;
        }

        defip_data_mem = INVALIDm;
        src_hit_mem = INVALIDm;
        dst_hit_mem = INVALIDm;
        switch (real_mem) {
        case EXT_IPV4_DEFIPm:
            defip_data_mem = EXT_DEFIP_DATA_IPV4m;
            src_hit_mem = EXT_SRC_HIT_BITS_IPV4m;
            dst_hit_mem = EXT_DST_HIT_BITS_IPV4m;
            tcam_width = 1;
            break;
        case EXT_IPV6_64_DEFIPm:
            defip_data_mem = EXT_DEFIP_DATA_IPV6_64m;
            src_hit_mem = EXT_SRC_HIT_BITS_IPV6_64m;
            dst_hit_mem = EXT_DST_HIT_BITS_IPV6_64m;
            tcam_width = 1;
            break;
        case EXT_IPV6_128_DEFIPm:
            defip_data_mem = EXT_DEFIP_DATA_IPV6_128m;
            src_hit_mem = EXT_SRC_HIT_BITS_IPV6_128m;
            dst_hit_mem = EXT_DST_HIT_BITS_IPV6_128m;
            tcam_width = 2;
            break;
        case EXT_L2_ENTRY_TCAMm:
        case EXT_IPV4_DEFIP_TCAMm:
        case EXT_IPV6_64_DEFIP_TCAMm:
            tcam_width = 1;
            break;
        case EXT_IPV6_128_DEFIP_TCAMm:
        case EXT_ACL144_TCAMm:
            tcam_width = 2;
            break;
        case EXT_ACL288_TCAMm:
            tcam_width = 4;
            break;
        case EXT_ACL360_TCAM_DATAm:
            tcam_width = 5;
            break;
        case EXT_ACL432_TCAM_DATAm:
            tcam_width = 6;
            break;
        case EXT_L2_ENTRYm:
        default:
            tcam_width = 0;
            break;
        }

        sal_memcpy(entry_data, soc_mem_entry_null(unit, real_mem),
                   soc_mem_entry_words(unit, real_mem) * sizeof(uint32));

        if (defip_data_mem != INVALIDm) {
            rv = soc_mem_read(unit, defip_data_mem, copyno, index, tmp_entry);
            if (SOC_FAILURE(rv)) {
                goto done;
            }
            soc_mem_field_get(unit, defip_data_mem, tmp_entry, SDATAf, sdata);
            soc_mem_field_set(unit, real_mem, entry_data, SDATAf, sdata);
        }
        if (src_hit_mem != INVALIDm) {
            rv = soc_mem_read(unit, src_hit_mem, copyno, index >> 5,
                              tmp_entry);
            if (SOC_FAILURE(rv)) {
                goto done;
            }
            src_hit = (soc_mem_field32_get(unit, src_hit_mem, tmp_entry,
                                           SRC_HITf) >> (index & 0x1f)) & 1;
            soc_mem_field32_set(unit, real_mem, entry_data, SRC_HITf, src_hit);
        }
        if (dst_hit_mem != INVALIDm) {
            rv = soc_mem_read(unit, dst_hit_mem, copyno, index >> 5,
                              tmp_entry);
            if (SOC_FAILURE(rv)) {
                goto done;
            }
            dst_hit = (soc_mem_field32_get(unit, dst_hit_mem, tmp_entry,
                                           DST_HITf) >> (index & 0x1f)) & 1;
            soc_mem_field32_set(unit, real_mem, entry_data, DST_HITf, dst_hit);
        }
        if (mask_mem != INVALIDm) {
            sal_memcpy(tmp_entry, soc_mem_entry_null(unit, mask_mem),
                       soc_mem_entry_words(unit, mask_mem) * sizeof(uint32));
        }
        if (tcam_width) {
            for (i = 0; i < tcam_width; i++) {
                rv = soc_tcam_read_entry(unit, TCAM_PARTITION_RAW,
                                         raw_index + i, mask, data, &valid);
                if (SOC_FAILURE(rv)) {
                    goto done;
                }
                mask[0] = mask[3];
                mask[3] = mask[1];
                mask[1] = mask[2];
                mask[2] = mask[3];
                if (mask_mem == INVALIDm) {
                    soc_mem_field_set(unit, real_mem, entry_data,
                                      field_mask[i], mask);
                } else {
                    soc_mem_field_set(unit, mask_mem, tmp_entry,
                                      field_mask[i], mask);
                }
                data[0] = data[3];
                data[3] = data[1];
                data[1] = data[2];
                data[2] = data[3];
                soc_mem_field_set(unit, real_mem, entry_data, field_data[i],
                                  data);
            }
            if (mask_mem != INVALIDm) {
                rv = soc_mem_write(unit, mask_mem, copyno, 0, tmp_entry);
            }
            goto done;
        }
    }
#endif /* BCM_TRIUMPH_SUPPORT */

    /* Setup S-Channel command packet */
    schan_msg_clear(&schan_msg);
    schan_msg.readcmd.header.opcode = READ_MEMORY_CMD_MSG;
#if defined(BCM_EXTND_SBUS_SUPPORT) || defined(BCM_PETRA_SUPPORT)||\
    defined(BCM_DFE_SUPPORT)
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        schan_msg.readcmd.header.srcblk =
            (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
    } else 
#endif
    {
        schan_msg.readcmd.header.srcblk = SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
    }
    schan_msg.readcmd.header.datalen = 4;    /* Hercules needs it */
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        if (mem == ESBS_PORT_TO_PIPE_MAPPINGm ||
            mem == ISBS_PORT_TO_PIPE_MAPPINGm) {
            schan_msg.readcmd.header.datalen = entry_dw * 4;
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */

    index2 = index;

#if defined(BCM_FIREBOLT_SUPPORT)
#if defined(SOC_MEM_L3_DEFIP_WAR) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_l3_defip_map) &&
        (mem == L3_DEFIPm ||
         mem == L3_DEFIP_ONLYm ||
         mem == L3_DEFIP_DATA_ONLYm ||
         mem == L3_DEFIP_HIT_ONLY_Xm ||
         mem == L3_DEFIP_HIT_ONLY_Ym ||
         mem == L3_DEFIP_HIT_ONLYm)) {
#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            index2 = soc_tr3_l3_defip_index_map(unit, mem, index);
        } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TD2_TT2(unit)) {
            index2 = soc_trident2_l3_defip_index_map(unit, mem, index);
        } else
#endif /* BCM_TRIDENT2_SUPPORT */
        {
#ifdef SOC_MEM_L3_DEFIP_WAR
            index2 = soc_fb_l3_defip_index_map(unit, index);
#endif /* SOC_MEM_L3_DEFIP_WAR */
        }
    }
#endif /* SOC_MEM_L3_DEFIP_WAR || BCM_TRIUMPH3_SUPPORT */
#endif /* BCM_FIREBOLT_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_l3_defip_hole) &&
        (mem == L3_DEFIPm ||
         mem == L3_DEFIP_ONLYm ||
         mem == L3_DEFIP_DATA_ONLYm ||
         mem == L3_DEFIP_HIT_ONLYm)) {
        index2 = soc_tr2_l3_defip_index_map(unit, index);
    }
#endif /* BCM_TRIUMPH2_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (soc_feature(unit, soc_feature_l3_defip_map) &&
        (mem == L3_DEFIP_PAIR_128m ||
         mem == L3_DEFIP_PAIR_128_ONLYm ||
         mem == L3_DEFIP_PAIR_128_DATA_ONLYm ||
         mem == L3_DEFIP_PAIR_128_HIT_ONLY_Xm ||
         mem == L3_DEFIP_PAIR_128_HIT_ONLY_Ym ||
         mem == L3_DEFIP_PAIR_128_HIT_ONLYm)) {
        if (SOC_IS_TRIUMPH3(unit)) {
            index2 = soc_tr3_l3_defip_index_map(unit, mem, index);
        } else {
#if defined(BCM_TRIDENT2_SUPPORT)
            if SOC_IS_TD2_TT2(unit) {
                index2 = soc_trident2_l3_defip_index_map(unit, mem, index);
            }
#endif /* BCM_TRIDENT2_SUPPORT */
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
    maddr = soc_mem_addr_get(unit, mem, array_index, copyno, index2, &at);
    schan_msg.readcmd.address = maddr;
#if defined(BCM_EXTND_SBUS_SUPPORT) || defined(BCM_PETRA_SUPPORT)||\
    defined(BCM_DFE_SUPPORT)
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        schan_msg.readcmd.header.dstblk = SOC_BLOCK2SCH(unit, copyno);
    } else 
#endif
    {
#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) ||\
    defined(BCM_PETRA_SUPPORT)|| defined(BCM_DFE_SUPPORT)
        /* required on XGS3. Optional on other devices */
        schan_msg.readcmd.header.dstblk = ((maddr >> SOC_BLOCK_BP) & 0xf) |
                                  (((maddr >> SOC_BLOCK_MSB_BP) & 0x1) << 4);
#endif /* BCM_XGS3_SWITCH_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT|| BCM_DFE_SUPPORT*/
    }

#ifdef BCM_TRIUMPH_SUPPORT
    if ((!SAL_BOOT_PLISIM || SAL_BOOT_BCMSIM) &&
        soc_feature(unit, soc_feature_esm_support)) {
        /* Following tables have response word in the s-chan message.
           we probably should invent SOC_MEM_FLAG_HAS_RESP_WORD in regsfile
           for such tables (or define a bit in S-chan message header if
           hardware can do that) */
        switch (mem) {
        case EXT_L2_ENTRYm:
        case EXT_L2_ENTRY_TCAMm:
        case EXT_L2_ENTRY_DATAm:
        case EXT_DEFIP_DATAm:
        case EXT_IPV4_DEFIP_TCAMm:
        case EXT_IPV4_DEFIPm:
        case EXT_IPV6_64_DEFIP_TCAMm:
        case EXT_IPV6_64_DEFIPm:
        case EXT_IPV6_128_DEFIP_TCAMm:
        case EXT_IPV6_128_DEFIPm:
        case EXT_FP_POLICYm:
        case EXT_FP_CNTRm:
        case EXT_FP_CNTR8m:
        case EXT_ACL144_TCAMm:
        case EXT_ACL288_TCAMm:
        case EXT_ACL360_TCAM_DATAm: /* not for MASK portion */
        case EXT_ACL432_TCAM_DATAm: /* not for MASK portion */
        /* and following software invented tables as well */
        case EXT_DEFIP_DATA_IPV4m:
        case EXT_DEFIP_DATA_IPV6_64m:
        case EXT_DEFIP_DATA_IPV6_128m:
        case EXT_FP_POLICY_ACL288_L2m:
        case EXT_FP_POLICY_ACL288_IPV4m:
        case EXT_FP_POLICY_ACL360_IPV6_SHORTm:
        case EXT_FP_POLICY_ACL432_IPV6_LONGm:
        case EXT_FP_POLICY_ACL144_L2m:
        case EXT_FP_POLICY_ACL144_IPV4m:
        case EXT_FP_POLICY_ACL144_IPV6m:
        case EXT_FP_POLICY_ACL432_L2_IPV4m:
        case EXT_FP_POLICY_ACL432_L2_IPV6m:
        case EXT_FP_CNTR_ACL288_L2m:
        case EXT_FP_CNTR_ACL288_IPV4m:
        case EXT_FP_CNTR_ACL360_IPV6_SHORTm:
        case EXT_FP_CNTR_ACL432_IPV6_LONGm:
        case EXT_FP_CNTR_ACL144_L2m:
        case EXT_FP_CNTR_ACL144_IPV4m:
        case EXT_FP_CNTR_ACL144_IPV6m:
        case EXT_FP_CNTR_ACL432_L2_IPV4m:
        case EXT_FP_CNTR_ACL432_L2_IPV6m:
        case EXT_FP_CNTR8_ACL288_L2m:
        case EXT_FP_CNTR8_ACL288_IPV4m:
        case EXT_FP_CNTR8_ACL360_IPV6_SHORTm:
        case EXT_FP_CNTR8_ACL432_IPV6_LONGm:
        case EXT_FP_CNTR8_ACL144_L2m:
        case EXT_FP_CNTR8_ACL144_IPV4m:
        case EXT_FP_CNTR8_ACL144_IPV6m:
        case EXT_FP_CNTR8_ACL432_L2_IPV4m:
        case EXT_FP_CNTR8_ACL432_L2_IPV6m:
        case EXT_ACL288_TCAM_L2m:
        case EXT_ACL288_TCAM_IPV4m:
        case EXT_ACL360_TCAM_DATA_IPV6_SHORTm:
        case EXT_ACL432_TCAM_DATA_IPV6_LONGm:
        case EXT_ACL144_TCAM_L2m:
        case EXT_ACL144_TCAM_IPV4m:
        case EXT_ACL144_TCAM_IPV6m:
        case EXT_ACL432_TCAM_DATA_L2_IPV4m:
        case EXT_ACL432_TCAM_DATA_L2_IPV6m:
#ifdef BCM_TRIUMPH3_SUPPORT
            if (SOC_IS_TRIUMPH3(unit)) {
                break;
            }
#endif /* BCM_TRIUMPH3_SUPPORT */
            resp_word = 1;
            break;
        default:
            break;
        }
    }
#endif /* BCM_TRIUMPH_SUPPORT */

#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit) && (!SAL_BOOT_PLISIM || SAL_BOOT_BCMSIM)) {
        schan_msg.readcmd.header.srcblk = 0;
        schan_msg.readcmd.header.datalen = 0;
        /* mask off the block field */
        schan_msg.readcmd.address &= 0x3F0FFFFF;
    }
#endif

    if (SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /*
     * Write onto S-Channel "memory read" command packet consisting of header
     * word + address word, and read back header word + entry_dw data words.
     */
    if (2 + entry_dw > CMIC_SCHAN_WORDS(unit)) {
       soc_cm_debug(DK_WARN, "soc_mem_read: assert will fail for memory %s\n", 
                    SOC_MEM_NAME(unit, mem));
    }
    if (soc_feature(unit, soc_feature_two_ingress_pipes) && (NULL == cache)) {
        sal_memcpy(&schan_msg_cpy, &schan_msg, sizeof(schan_msg));
    }
    rv = soc_schan_op(unit, &schan_msg, 2, 1 + entry_dw + resp_word, allow_intr);
    if (SOC_FAILURE(rv)) {
        if (SOC_SER_CORRECTION_SUPPORT(unit)) {
            if (!SOC_MEM_RETURN_SCHAN_ERR(unit, mem)) {
                if (cache != NULL && CACHE_VMAP_TST(vmap, index) 
                    && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
                    sal_memcpy(entry_data, cache + index * entry_dw, entry_dw * 4);
                    soc_cm_debug(DK_VERBOSE, "Unit:%d Mem[%s] index[%d] "
                                 "Force fetch data from cache.\n", 
                                 unit, SOC_MEM_NAME(unit, mem), index);
                } else {
                    if (SOC_IS_TD2_TT2(unit) &&
                        (mem == L3_DEFIP_ALPM_IPV4m || mem == L3_DEFIP_ALPM_IPV4_1m ||
                         mem == L3_DEFIP_ALPM_IPV6_64m || mem == L3_DEFIP_ALPM_IPV6_64_1m ||
                         mem == L3_DEFIP_ALPM_IPV6_128m)) {
                        /* Fetch data from other pipe */
                        if (schan_msg_cpy.readcmd.header.srcblk == _SOC_MEM_ADDR_ACC_TYPE_PIPE_Y) {
                            schan_msg_cpy.readcmd.header.srcblk = _SOC_MEM_ADDR_ACC_TYPE_PIPE_X;
                        } else {
                            schan_msg_cpy.readcmd.header.srcblk = _SOC_MEM_ADDR_ACC_TYPE_PIPE_Y;
                        }
                        rv = soc_schan_op(unit, &schan_msg_cpy, 2, 1 + entry_dw + resp_word, allow_intr);
                        if (SOC_FAILURE(rv)) {
                            goto done;
                        }
                        sal_memcpy(&schan_msg, &schan_msg_cpy, sizeof(schan_msg));
                        soc_cm_debug(DK_VERBOSE, "Unit:%d Mem[%s] index[%d] "
                                     "Force fetch data from other pipe.\n", 
                                     unit, SOC_MEM_NAME(unit, mem), index);
                        goto return_data;
                    } else {
                        void *null_entry = soc_mem_entry_null(unit, mem);
                        sal_memcpy(entry_data, null_entry, entry_dw * 4);
                        soc_cm_debug(DK_VERBOSE, "Unit:%d Mem[%s] index[%d] "
                                     "Force fetch null data.\n", 
                                     unit, SOC_MEM_NAME(unit, mem), index);
                    }
                }
                rv = SOC_E_NONE;
            } else {
                goto return_data;
            }    
        } 
        goto done;
    }
return_data:
    if ((schan_msg.readresp.header.opcode != READ_MEMORY_ACK_MSG) 
        || (schan_msg.readresp.header.ebit)){
        soc_cm_debug(DK_ERR,
                     "soc_mem_read: "
                     "invalid S-Channel reply, expected READ_MEMORY_ACK:\n");
        soc_schan_dump(unit, &schan_msg, 1 + entry_dw + resp_word);
        rv = SOC_E_INTERNAL;
        goto done;
    }

    sal_memcpy(entry_data,
               resp_word ? schan_msg.genresp.data : schan_msg.readresp.data,
               entry_dw * sizeof (uint32));

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    if (soc_feature(unit, soc_feature_xy_tcam_direct) &&
        (meminfo->flags & SOC_MEM_FLAG_CAM) &&
        (!(meminfo->flags & SOC_MEM_FLAG_EXT_CAM))) {
        _soc_mem_tcam_xy_to_dm(unit, mem, 1, entry_data, entry_data);
    }
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
    /* convert X-Y data of esm external tcam table reads to D-M format */
    if (soc_feature(unit, soc_feature_etu_support) &&
        SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU) {

        uint32 key[SOC_MAX_MEM_FIELD_WORDS], mask[SOC_MAX_MEM_FIELD_WORDS];
        soc_field_t key_field[4], mask_field[4];
        int field_count=0, bit_length, word_length, i, word;

        switch (mem) {
        case EXT_L2_ENTRY_TCAMm:
        case EXT_L2_ENTRY_TCAM_WIDEm:
        case EXT_IPV4_TCAMm:
        case EXT_IPV6_64_TCAMm:
        case EXT_IPV4_DEFIPm:
        case EXT_IPV4_DEFIP_TCAMm:
        case EXT_IPV4_UCASTm:
        case EXT_IPV4_UCAST_TCAMm:
        case EXT_IPV4_UCAST_WIDEm:
        case EXT_IPV4_UCAST_WIDE_TCAMm:
        case EXT_IPV6_64_DEFIPm:
        case EXT_IPV6_64_DEFIP_TCAMm:
        case EXT_ACL80_TCAMm:
            key_field[0] = TDW0f;
            mask_field[0] = TMW0f;
            field_count = 1;
            break;

        case EXT_IPV6_128_TCAMm:
        case EXT_IPV6_128_DEFIPm:
        case EXT_IPV6_128_DEFIP_TCAMm:
        case EXT_IPV6_128_UCASTm:
        case EXT_IPV6_128_UCAST_TCAMm:
        case EXT_IPV6_128_UCAST_WIDEm:
        case EXT_IPV6_128_UCAST_WIDE_TCAMm:
        case EXT_ACL160_TCAMm:
        case EXT_ACL144_TCAM_L2m:
        case EXT_ACL144_TCAM_IPV4m:
        case EXT_ACL144_TCAM_IPV6m:
            key_field[0] = TDW0f;
            key_field[1] = TDW1f;
            mask_field[0] = TMW0f;
            mask_field[1] = TMW1f;
            field_count = 2;
            break;

        case EXT_ACL288_TCAM_L2m:
        case EXT_ACL288_TCAM_IPV4m:
        case EXT_ACL432_TCAM_DATA_L2_IPV4m:
        case EXT_ACL320_TCAMm:
            key_field[0] = TDW0f;
            key_field[1] = TDW1f;
            key_field[2] = TDW2f;
            key_field[3] = TDW3f;
            mask_field[0] = TMW0f;
            mask_field[1] = TMW1f;
            mask_field[2] = TMW2f;
            mask_field[3] = TMW3f;
            field_count = 4;
            break;

        
#ifdef fix_later
        case EXT_ACL360_TCAM_DATA_IPV6_SHORTm:
        case EXT_ACL432_TCAM_DATA_IPV6_LONGm:
        case EXT_ACL432_TCAM_DATA_L2_IPV6m:
        case EXT_ACL432_TCAM_MASKm:
        case EXT_ACL360_TCAM_MASK_IPV6_SHORTm:
        case EXT_ACL432_TCAM_MASK_IPV6_LONGm:
        case EXT_ACL432_TCAM_MASK_L2_IPV6m:
        case EXT_ACL480_TCAM_DATAm:
        case EXT_ACL480_TCAM_MASKm:
            key_field[0] = TDW0f;
            key_field[1] = TDW1f;
            key_field[2] = TDW2f;
            key_field[3] = TDW3f;
            mask_field[0] = TMW0f;
            mask_field[1] = TMW1f;
            mask_field[2] = TMW2f;
            mask_field[3] = TMW3f;
            field_count = 6;
            break;
#endif
        }
        for (i = 0; i < field_count; i++) {
            soc_mem_field_get(unit, mem, entry_data, key_field[i], key);
            soc_mem_field_get(unit, mem, entry_data, mask_field[i], mask);
            bit_length = soc_mem_field_length(unit, mem, key_field[i]);
            word_length = (bit_length + 31) / 32;
            for (word = 0; word < word_length; word++) {
                /* save data word first */
                mask[word] = ~(key[word] | mask[word]);
            }
            if ((bit_length & 0x1f) != 0) {
                mask[word - 1] &= (1 << (bit_length & 0x1f)) - 1;
            }
            soc_mem_field_set(unit, mem, entry_data, mask_field[i], mask);
            soc_mem_field_set(unit, mem, entry_data, key_field[i], key);
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

done:
    MEM_UNLOCK(unit, mem);


#ifdef INCLUDE_TCL
    if (cache_consistency_check) {
#define _SOC_ENTRY_VALID_CHECK(unit, mem, valid_field)                      \
        if (SOC_MEM_FIELD_VALID(unit, mem, valid_field)) {                  \
            if (!soc_mem_field32_get(unit, mem, entry_data, valid_field)) { \
                goto done_cache_check;                                      \
            }                                                               \
        }

#define _SOC_ENTRY_PARITY_CLEAR(unit, mem, ecc_field)                       \
        if (SOC_MEM_FIELD_VALID(unit, mem, ecc_field)) {                    \
            soc_mem_field32_set(unit, mem, entry_data, ecc_field, 0);       \
            soc_mem_field32_set(unit, mem, entry_data_cache, ecc_field, 0); \
        }

        int bytes = soc_mem_entry_bytes(unit, mem);
        
        /* Some exceptions */
        if ((SOC_IS_TD_TT(unit) || SOC_IS_TD2_TT2(unit)) && (mem == EGR_VLANm)) {
            goto done_cache_check;
        }
        /* Hueristic to check if entry is valid */
        _SOC_ENTRY_VALID_CHECK(unit, mem, VALIDf);
        _SOC_ENTRY_VALID_CHECK(unit, mem, VALID_0f);

        /* Hueristic to clear parity/ecc bits */
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, PARITYf);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, EVEN_PARITYf);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, EVEN_PARITY_0f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, EVEN_PARITY_1f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, EVEN_PARITY_2f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, EVEN_PARITY_3f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, EVEN_PARITY_Af);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, EVEN_PARITY_Bf);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, EVEN_PARITY_LOWERf);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, EVEN_PARITY_UPPERf);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCf);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECC_0f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECC_1f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECC_2f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECC_3f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECC_4f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCPf);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP0f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP1f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP2f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP3f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP4f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP5f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP6f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP7f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP_0f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP_1f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP_2f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP_3f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, ECCP_4f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, PARITY0f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, PARITY1f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, PARITY2f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, PARITY3f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, PARITY_0f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, PARITY_1f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, PARITY_2f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, PARITY_3f);
        _SOC_ENTRY_PARITY_CLEAR(unit, mem, PARITY_4f);
        
        if (sal_memcmp(entry_data_cache, entry_data, bytes-1) != 0) {
            if (soc_cm_debug_check(DK_ERR)) {
                soc_cm_print("soc_mem_read unit %d: %s.%s[%d]: cache: ",
                             unit, SOC_MEM_NAME(unit, mem),
                             SOC_BLOCK_NAME(unit, copyno), index);
                soc_mem_entry_dump(unit, mem, entry_data_cache);
                soc_cm_print("\n");
                soc_cm_print("soc_mem_read unit %d: %s.%s[%d]: Device: ",
                             unit, SOC_MEM_NAME(unit, mem),
                             SOC_BLOCK_NAME(unit, copyno), index);
                soc_mem_entry_dump(unit, mem, entry_data);
                soc_cm_print("\n");
            }

        }
    }
done_cache_check:
#endif

    if (NULL != meminfo->snoop_cb && 
         (SOC_MEM_SNOOP_READ & meminfo->snoop_flags)) {
         meminfo->snoop_cb(unit, mem, SOC_MEM_SNOOP_READ, copyno, index, index, 
                           entry_data, meminfo->snoop_user_data);
    }
    if (soc_cm_debug_check(DK_MEM)) {
        soc_cm_print("soc_mem_read unit %d: %s.%s[%d]: ",
                     unit, SOC_MEM_NAME(unit, mem),
                     SOC_BLOCK_NAME(unit, copyno), index);
        soc_mem_entry_dump(unit, mem, entry_data);
        soc_cm_print("\n");
    }

    return rv;
}

/*
 * Function:
 *    soc_mem_pipe_select_read
 * Purpose:
 *    Read a memory internal to the SOC.
 * Notes:
 *    GBP/CBP memory should only accessed when MMU is in DEBUG mode.
 */

int
soc_mem_pipe_select_read(int unit,
                         soc_mem_t mem,
                         int copyno,
                         int acc_type,
                         int index,
                         void *entry_data)
{
    schan_msg_t schan_msg;
    int entry_dw = soc_mem_entry_words(unit, mem);
    soc_mem_info_t *meminfo; 
    int index_valid;
    uint8 at;
    int index2;
    uint32 maddr;
    int rv, allow_intr = 0;
    int resp_word = 0;

    if (!soc_feature(unit, soc_feature_two_ingress_pipes)) {
        /* Not a relevant device */
        return SOC_E_UNAVAIL;
    }

    if (!soc_mem_is_valid(unit, mem)) {
        return SOC_E_MEMORY;
    }

    meminfo = &SOC_MEM_INFO(unit, mem);

    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_pipe_select_read: invalid block %d for memory %s\n",
                     copyno, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    /*
     * When checking index, check for 0 instead of soc_mem_index_min.
     * Diagnostics need to read/write index 0 of Strata ARL and GIRULE.
     */
    index_valid = (index >= 0 &&
                   index <= soc_mem_index_max(unit, mem));

    if (!index_valid) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_pipe_select_read: invalid index %d for memory %s acc_type %d\n",
                     index, SOC_MEM_NAME(unit, mem), acc_type);
        return SOC_E_PARAM;
    }

    rv = SOC_E_NONE;

    MEM_LOCK(unit, mem);

    /* Setup S-Channel command packet */
    schan_msg_clear(&schan_msg);
    schan_msg.readcmd.header.opcode = READ_MEMORY_CMD_MSG;
#if defined(BCM_EXTND_SBUS_SUPPORT)
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        if (0 == acc_type) {
            acc_type =
                (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
        }
        schan_msg.readcmd.header.srcblk = acc_type;
    } else 
#endif
    {
        schan_msg.readcmd.header.srcblk = SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
    }
    schan_msg.readcmd.header.datalen = 4;    /* Hercules needs it */
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        if (mem == ESBS_PORT_TO_PIPE_MAPPINGm ||
            mem == ISBS_PORT_TO_PIPE_MAPPINGm) {
            schan_msg.readcmd.header.datalen = entry_dw * 4;
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */

    index2 = index;

    
#if defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TD2_TT2(unit) &&
            soc_feature(unit, soc_feature_l3_defip_map) &&
            (mem == L3_DEFIPm ||
             mem == L3_DEFIP_ONLYm ||
             mem == L3_DEFIP_DATA_ONLYm ||
             mem == L3_DEFIP_HIT_ONLY_Xm ||
             mem == L3_DEFIP_HIT_ONLY_Ym ||
             mem == L3_DEFIP_HIT_ONLYm ||
             mem == L3_DEFIP_PAIR_128m ||
             mem == L3_DEFIP_PAIR_128_ONLYm ||
             mem == L3_DEFIP_PAIR_128_DATA_ONLYm ||
             mem == L3_DEFIP_PAIR_128_HIT_ONLY_Xm ||
             mem == L3_DEFIP_PAIR_128_HIT_ONLY_Ym ||
             mem == L3_DEFIP_PAIR_128_HIT_ONLYm)) {
            index2 = soc_trident2_l3_defip_index_map(unit, mem, index);
        }
#endif /* BCM_TRIDENT2_SUPPORT */
    maddr = soc_mem_addr_get(unit, mem, 0, copyno, index2, &at);
#if defined(BCM_EXTND_SBUS_SUPPORT)
    if (!soc_feature(unit, soc_feature_new_sbus_format) &&
        (acc_type != 0)) {
        /* Override ACC_TYPE in address */
        maddr &= ~(_SOC_MEM_ADDR_ACC_TYPE_MASK <<
                        _SOC_MEM_ADDR_ACC_TYPE_SHIFT);
        maddr |= (acc_type & _SOC_MEM_ADDR_ACC_TYPE_MASK) <<
            _SOC_MEM_ADDR_ACC_TYPE_SHIFT;
    }
#endif /* BCM_EXTND_SBUS_SUPPORT */
    schan_msg.readcmd.address = maddr;
#if defined(BCM_EXTND_SBUS_SUPPORT)
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        schan_msg.readcmd.header.dstblk = SOC_BLOCK2SCH(unit, copyno);
    } else 
#endif
    {
#if defined(BCM_XGS3_SWITCH_SUPPORT)
        /* required on XGS3. Optional on other devices */
        schan_msg.readcmd.header.dstblk = ((maddr >> SOC_BLOCK_BP) & 0xf) |
                                  (((maddr >> SOC_BLOCK_MSB_BP) & 0x1) << 4);
#endif /* BCM_XGS3_SWITCH_SUPPORT */
    }

    /*
     * Write onto S-Channel "memory read" command packet consisting of header
     * word + address word, and read back header word + entry_dw data words.
     */
    if (2 + entry_dw > CMIC_SCHAN_WORDS(unit)) {
       soc_cm_debug(DK_WARN, "soc_mem_read: assert will fail for memory %s\n", 
                    SOC_MEM_NAME(unit, mem));
    }
    rv = soc_schan_op(unit, &schan_msg, 2, 1 + entry_dw + resp_word, allow_intr);
    if (SOC_FAILURE(rv)) {
        goto done;
    }

    if ((schan_msg.readresp.header.opcode != READ_MEMORY_ACK_MSG) 
        || (schan_msg.readresp.header.ebit)){
        soc_cm_debug(DK_ERR,
                     "soc_mem_read: "
                     "invalid S-Channel reply, expected READ_MEMORY_ACK:\n");
        soc_schan_dump(unit, &schan_msg, 1 + entry_dw + resp_word);
        rv = SOC_E_INTERNAL;
        goto done;
    }

    sal_memcpy(entry_data,
               resp_word ? schan_msg.genresp.data : schan_msg.readresp.data,
               entry_dw * sizeof (uint32));

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT)
    if (soc_feature(unit, soc_feature_xy_tcam_direct) &&
        (meminfo->flags & SOC_MEM_FLAG_CAM) &&
        (!(meminfo->flags & SOC_MEM_FLAG_EXT_CAM))) {
        _soc_mem_tcam_xy_to_dm(unit, mem, 1, entry_data, entry_data);
    }
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */

done:

    MEM_UNLOCK(unit, mem);


    if (NULL != meminfo->snoop_cb && 
         (SOC_MEM_SNOOP_READ & meminfo->snoop_flags)) {
         meminfo->snoop_cb(unit, mem, SOC_MEM_SNOOP_READ, copyno, index, index, 
                           entry_data, meminfo->snoop_user_data);
    }
    if (soc_cm_debug_check(DK_MEM)) {
        soc_cm_print("soc_mem_read unit %d: %s.%s[%d]: ",
                     unit, SOC_MEM_NAME(unit, mem),
                     SOC_BLOCK_NAME(unit, copyno), index);
        soc_mem_entry_dump(unit, mem, entry_data);
        soc_cm_print("\n");
    }

    return rv;
}

int
soc_mem_read(int unit,
             soc_mem_t mem,
             int copyno,
             int index,
             void *entry_data)
{
    _SOC_MEM_REPLACE_MEM(unit, mem);
    return soc_mem_array_read(unit, mem, 0, copyno, index, entry_data);
}

/*
 * Function:    soc_mem_array_read
 * Purpose:     Read a memory array internal to the SOC.
 */
int
soc_mem_array_read(int unit, soc_mem_t mem, unsigned array_index, int copyno, 
                   int index, void *entry_data)
{
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TRIDENT(unit) || SOC_IS_TITAN(unit)) {
        uint32 entry_data_y[SOC_MAX_MEM_WORDS];
        soc_pbmp_t pbmp_x, pbmp_y;

        if (mem == EGR_VLANm) {
            soc_trident_pipe_select(unit, TRUE, 1); /* Y pipe */
            SOC_IF_ERROR_RETURN
                (_soc_mem_read(unit, mem, array_index, copyno, index, entry_data_y));
            soc_trident_pipe_select(unit, TRUE, 0); /* X pipe */
            SOC_IF_ERROR_RETURN
                (_soc_mem_read(unit, mem, array_index, copyno, index, entry_data));
            soc_mem_pbmp_field_get(unit, mem, entry_data, PORT_BITMAPf,
                                   &pbmp_x);
            soc_mem_pbmp_field_get(unit, mem, entry_data_y, PORT_BITMAPf,
                                   &pbmp_y);
            SOC_PBMP_OR(pbmp_x, pbmp_y);
            soc_mem_pbmp_field_set(unit, mem, entry_data, PORT_BITMAPf,
                                   &pbmp_x);
            soc_mem_pbmp_field_get(unit, mem, entry_data, UT_PORT_BITMAPf,
                                   &pbmp_x);
            soc_mem_pbmp_field_get(unit, mem, entry_data_y, UT_PORT_BITMAPf,
                                   &pbmp_y);
            SOC_PBMP_OR(pbmp_x, pbmp_y);
            soc_mem_pbmp_field_set(unit, mem, entry_data, UT_PORT_BITMAPf,
                                   &pbmp_x);
            return SOC_E_NONE;
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */

    return _soc_mem_read(unit, mem, array_index, copyno, index, entry_data);
}

/*
 * Function:
 *    soc_mem_read_range
 * Purpose:
 *    Read a range of chip's memory
 * Parameters:
 *    buffer -- Pointer to block of sufficiently large memory.
 * Notes:
 *    Table DMA only works on tables whose entry is less than
 *    SOC_MEM_DMA_MAX_DATA_BEATS words long.
 *
 *    Table DMA has a minimum transaction size of 4 words, so if the
 *    table entry is 1 or 2 words, then the count of words is modified
 *    to keep this alignment.  For the remainder of the entries, this
 *    function reads in the remainder of the data through mem_read
 *    without using DMA.
 */

int
soc_mem_read_range(int unit, soc_mem_t mem, int copyno,
                   int index_min, int index_max, void *buffer)
{
    int rc;

#ifdef SOC_MEM_DEBUG_SPEED
    int count = (index_max > index_min) ? 
      (index_max - index_min + 1) : 
      (index_min - index_max + 1);
        
    sal_usecs_t start_time;
    int diff_time,start_time = sal_time_usecs();
#endif
    /* COVERITY: Intentional, stack use of 4064 bytes */
    /* coverity[stack_use_callee_max : FALSE] */
    /* coverity[stack_use_return : FALSE] */

    if (SOC_WARM_BOOT(unit) && SOC_CONTROL(unit)->dma_from_mem_cache) {
        uint32 *cache;
        uint8 *vmap;
        int entry_dw, entry_sz, start, indexes;

        if (copyno == MEM_BLOCK_ANY) {
            copyno = SOC_MEM_BLOCK_ANY(unit, mem);
        }
        cache = SOC_MEM_STATE(unit, mem).cache[copyno];
        vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];
        if (cache != NULL && CACHE_VMAP_TST(vmap, 0) &&
            !_SOC_MEM_CHK_L2_MEM(mem) && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
            entry_dw = soc_mem_entry_words(unit, mem);
            entry_sz = entry_dw * sizeof(uint32);
            start = index_min > index_max ? index_max : index_min;
            indexes = (index_max > index_min) ?
                       (index_max - index_min + 1) : (index_min - index_max + 1);
            sal_memcpy(buffer, cache + (start * entry_dw), indexes * entry_sz);
            soc_cm_debug(DK_SOCMEM+DK_VERBOSE, "Unit:%d Mem[%s] "
                         "DMA data from cache.\n", unit, SOC_MEM_NAME(unit, mem));
            return SOC_E_NONE;
        }
    }
    rc = soc_mem_array_read_range(unit, mem, 0, copyno, index_min, 
                                    index_max, buffer);
#ifdef SOC_MEM_DEBUG_SPEED    
    diff_time = SAL_USECS_SUB(sal_time_usecs(), start_time);
    soc_cm_debug(DK_SOCMEM | DK_VERBOSE,
                 "Total dma read time: %d usecs, [%d nsecs per op]\n",
                 diff_time, diff_time*1000/count);
#endif
    return rc;
}

/*
 * Function:
 *    soc_mem_array_read_range
 * Purpose:
 *    Read a range of chip's memory
 * Parameters:
 *    buffer -- Pointer to block of sufficiently large memory.
 *                  For Draco DMA operations, this memory must be
 *                  DMA-able
 * Notes:
 *    Table DMA only works on tables whose entry is less than
 *    SOC_MEM_DMA_MAX_DATA_BEATS words long.
 *
 *    Table DMA has a minimum transaction size of 4 words, so if the
 *    table entry is 1 or 2 words, then the count of words is modified
 *    to keep this alignment.  For the remainder of the entries, this
 *    function reads in the remainder of the data through mem_read
 *    without using DMA.
 */

int
soc_mem_array_read_range(int unit, soc_mem_t mem, unsigned array_index, 
                         int copyno, int index_min, int index_max, void *buffer)
{
    int index, rv;
    int count;
    void *buf;
    soc_mem_info_t  *meminfo;

    if (!soc_mem_is_valid(unit, mem)) {
        return SOC_E_MEMORY;
    }

    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }


    assert(SOC_MEM_BLOCK_VALID(unit, mem, copyno));
    assert(soc_mem_index_valid(unit, mem, index_min));
    assert(soc_mem_index_valid(unit, mem, index_max));
    assert(index_min <= index_max);
    assert(buffer != NULL);

    meminfo = &SOC_MEM_INFO(unit, mem);
    count = 0;

    /* coverity[var_tested_neg : FALSE] */
    soc_cm_debug(DK_SOCMEM,
                 "soc_mem_array_read_range: unit %d memory %s.%s [%d:%d]\n",
                 unit, SOC_MEM_UFNAME(unit, mem),
                 SOC_BLOCK_NAME(unit, copyno),
                 index_min, index_max);

    /* If device is gone fill buffer with null entries. */
    if (SOC_IS_DETACHING(unit)) {
        buf = buffer;
        for (index = index_min; index <= index_max; index++, count++) {
            buf = soc_mem_table_idx_to_pointer(unit, mem, void *,
                                               buffer, count);
            sal_memcpy(buf, soc_mem_entry_null(unit, mem),
                       soc_mem_entry_bytes(unit, mem));
        }
        return (SOC_E_NONE);
    }
    rv = SOC_E_NONE;
 
#if defined(BCM_XGS_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) ||\
    defined(BCM_CALADAN3_SUPPORT) || defined(BCM_88750_SUPPORT) ||\
    defined(BCM_ARAD_SUPPORT)
    /* coverity[negative_returns : FALSE] */ 
    if (soc_mem_dmaable(unit, mem, copyno)) {
#if defined(BCM_SIRIUS_SUPPORT) || defined (BCM_XGS3_SWITCH_SUPPORT) ||\
    defined(BCM_CALADAN3_SUPPORT) || defined(BCM_88750_SUPPORT) ||\
    defined(BCM_ARAD_SUPPORT)

        if (SOC_IS_SIRIUS(unit) || SOC_IS_XGS3_SWITCH(unit) || 
            SOC_IS_CALADAN3(unit) || SOC_IS_FE1600(unit) || SOC_IS_ARAD(unit)) {
            rv = _soc_mem_dma_read(unit, mem, array_index, copyno, index_min,
                                   index_max, 0, buffer);
            
            if (SOC_FAILURE(rv)) {
                if (rv == SOC_E_FAIL && SOC_SER_CORRECTION_SUPPORT(unit)) {
                    soc_cm_debug(DK_VERBOSE, "Unit:%d Mem[%s] "
                                 "DMA fallback to pio.\n", 
                                 unit, SOC_MEM_NAME(unit, mem));
                    goto _dma_fall_back_pio;
                } 
                return rv;
            }
            if (NULL != meminfo->snoop_cb && 
                (SOC_MEM_SNOOP_READ & meminfo->snoop_flags)) {
                meminfo->snoop_cb(unit, mem, SOC_MEM_SNOOP_READ, copyno,
                                  index_min, index_max, buffer,
                                  meminfo->snoop_user_data);
            }
            return SOC_E_NONE;
        }
#endif /* BCM_XGS3_SWITCH_SUPPORT BCM_SIRIUS_SUPPORT BCM_CALADAN3_SUPPORT || 
          defined(BCM_88750_SUPPORT) */
    }
_dma_fall_back_pio:
#endif /* (BCM_XGS_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) ||
          defined(BCM_CALADAN3_SUPPORT) || defined(BCM_88750_SUPPORT) ||
          defined(BCM_ARAD_SUPPORT) */
    /* For the rest of entries, do direct read */
    for (index = index_min; index <= index_max; index++, count++) {
        buf = soc_mem_table_idx_to_pointer(unit, mem, void *,
                                           buffer, count);
        SOC_IF_ERROR_RETURN(soc_mem_array_read(unit, mem, array_index, copyno, 
                                               index, buf));
    }

    return SOC_E_NONE;
}

int soc_mem_array_read_flags(int unit, soc_mem_t mem, unsigned array_index,
                             int copyno, int index, void *entry_data, int flags)
{
    int rv, orig_flag;

    if (!soc_mem_is_valid(unit, mem)) {
        return SOC_E_MEMORY;
    }

    orig_flag = SOC_MEM_FORCE_READ_THROUGH(unit);

    if(flags & SOC_MEM_DONT_USE_CACHE) {
        SOC_MEM_FORCE_READ_THROUGH_SET(unit, 1);
    }

    rv = soc_mem_array_read(unit, mem, array_index, copyno, index, entry_data);
    
    SOC_MEM_FORCE_READ_THROUGH_SET(unit, orig_flag);
    
    return rv; 
}

/*
 * Function:
 *    soc_mem_ser_read_range
 * Purpose:
 *    Read a range of chip's memory for SER detection
 * Parameters:
 *    buffer -- Pointer to block of sufficiently large memory.
 * Notes:
 *    Table DMA only works on tables whose entry is less than
 *    SOC_MEM_DMA_MAX_DATA_BEATS words long.
 *
 *    Table DMA has a minimum transaction size of 4 words, so if the
 *    table entry is 1 or 2 words, then the count of words is modified
 *    to keep this alignment.  For the remainder of the entries, this
 *    function reads in the remainder of the data through mem_read
 *    without using DMA.
 */
int
soc_mem_ser_read_range(int unit, soc_mem_t mem, int copyno,
                       int index_min, int index_max, uint32 ser_flags,
                       void *buffer)
{
    int rv, index, pipe_acc, count;
    void *buf;

    if (!soc_mem_is_valid(unit, mem)) {
        return SOC_E_MEMORY;
    }

    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    /* Skip assertions here because they should have been checked
     * during mem scan init. */
    assert(buffer != NULL);

    if (0 == (ser_flags & _SOC_SER_FLAG_NO_DMA)) {
        rv = _soc_mem_dma_read(unit, mem, 0, copyno, index_min,
                               index_max, ser_flags, buffer);
    } else {
        /* Trigger the slow path iteration below */
        rv = SOC_E_FAIL;
    }

    if (SOC_E_FAIL == rv) {
        /* Scan the table by individual reads to trigger SER correction */
        pipe_acc = (ser_flags & _SOC_SER_FLAG_ACC_TYPE_MASK);
        count = 0;
        for (index = index_min; index <= index_max; index++, count++) {
            buf = soc_mem_table_idx_to_pointer(unit, mem, void *,
                                               buffer, count);
            if (0 != (ser_flags & _SOC_SER_FLAG_MULTI_PIPE)) {
                rv = soc_mem_pipe_select_read(unit, mem, copyno,
                                              pipe_acc, index, buf);
            } else {
                rv = soc_mem_read(unit, mem, copyno, index, buf);
            }
            if (SOC_E_FAIL == rv) {
                /* We're expecting this to fail, that triggers the
                 * correction logic. */
                rv = SOC_E_NONE;
            } else if (SOC_FAILURE(rv)) {
                return rv;
            }
        }
        rv = SOC_E_NONE;
    }

    return rv;
}

#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) || defined(BCM_CALADAN3_SUPPORT)
#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *    soc_scache_mem_read_range
 * Purpose:
 *    Read a range of chip's memory used for L2 Warm Boot
 * Parameters:
 *    buffer -- Pointer to block of sufficiently large memory.
 *                  For DMA operations, this memory must be
 *                  DMA-able
 * Notes:
 *    This function is only used for DMA'ing the scache from the
 *      chip's internal memory.
 */

STATIC int
soc_scache_mem_read_range(int unit, soc_mem_t mem, int copyno,
                          int index_min, int index_max, void *buffer)
{
    int index;
    int count;
    void *buf;
    soc_mem_info_t  *meminfo;
    unsigned int array_index = 0;

    if (!soc_mem_is_valid(unit, mem)) {
        return SOC_E_MEMORY;
    }

    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    assert(SOC_MEM_BLOCK_VALID(unit, mem, copyno));
    assert(buffer != NULL);

    meminfo = &SOC_MEM_INFO(unit, mem);
    count = 0;

    /* coverity[var_tested_neg : FALSE] */
    soc_cm_debug(DK_SOCMEM,
                 "soc_scache_mem_read_range: unit %d memory %s.%s [%d:%d]\n",
                 unit, SOC_MEM_UFNAME(unit, mem),
                 SOC_BLOCK_NAME(unit, copyno),
                 index_min, index_max);

    /* If device is gone fill buffer with null entries. */
    if (SOC_IS_DETACHING(unit)) {
        buf = buffer;
        for (index = index_min; index <= index_max; index++, count++) {
            buf = soc_mem_table_idx_to_pointer(unit, mem, void *,
                                               buffer, count);
            sal_memcpy(buf, soc_mem_entry_null(unit, mem),
                       soc_mem_entry_bytes(unit, mem));
        }
        return (SOC_E_NONE);
    }

    /* coverity[negative_returns : FALSE] */
    if (soc_mem_dmaable(unit, mem, copyno)) {
        if (SOC_IS_XGS3_SWITCH(unit)) {
        
            SOC_IF_ERROR_RETURN
                (_soc_mem_dma_read(unit, mem, array_index, copyno,
                                   index_min, index_max, 0, buffer));

            if (NULL != meminfo->snoop_cb && 
                (SOC_MEM_SNOOP_READ & meminfo->snoop_flags)) {
                meminfo->snoop_cb(unit, mem, SOC_MEM_SNOOP_READ, copyno,
                                  index_min, index_max, buffer,
                                  meminfo->snoop_user_data);
            }
        }
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *    soc_scache_mem_write_range
 * Purpose:
 *    Write a range of chip's memory used for L2 Warm Boot
 * Parameters:
 *    buffer -- Pointer to block of sufficiently large memory.
 *                For slam operations, this memory must be
 *                slammable
 * Notes:
 *    This function is only used for slamming the scache to the
 *      chip's internal memory.
 */
STATIC int
soc_scache_mem_write_range(int unit, soc_mem_t mem, int copyno,
                           int index_min, int index_max, void *buffer)
{
    uint32          entry_dw;
    int             i, rv = SOC_E_NONE;
    soc_mem_info_t  *meminfo;
    void            *cache_buffer = NULL;
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    int             count, alloc_size;
#endif
    uint32          *cache;

    if (!soc_mem_is_valid(unit, mem)) {
        return SOC_E_MEMORY;
    }

    meminfo = &SOC_MEM_INFO(unit, mem);
    entry_dw = soc_mem_entry_words(unit, mem);

    soc_cm_debug(DK_SOCMEM,
                 "soc_scache_mem_write_range: unit %d memory %s.%s [%d:%d]\n",
                 unit, SOC_MEM_UFNAME(unit, mem),
                 SOC_BLOCK_NAME(unit, copyno),
                 index_min, index_max);

    /*    coverity[negative_returns : FALSE]    */
    if (SOC_IS_XGS3_SWITCH(unit) && soc_mem_slamable(unit, mem, copyno)) {
        int blk;

        if (copyno == COPYNO_ALL) {
            SOC_MEM_BLOCK_ITER(unit, mem, blk) {
                copyno = blk;
                break;
            }
        }
        if (copyno == COPYNO_ALL) {
            return SOC_E_INTERNAL;
        }
        cache = SOC_MEM_STATE(unit, mem).cache[copyno];
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
        if (cache != NULL && !SOC_MEM_TEST_SKIP_CACHE(unit) &&
            (soc_feature(unit, soc_feature_xy_tcam) &&
            (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) &&
            (!(SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_EXT_CAM)))) {
            if (index_min > index_max) {
                count = index_min - index_max + 1;
            } else {
                count = index_max - index_min + 1;
            }
            alloc_size = count * entry_dw * sizeof(uint32);
            cache_buffer = sal_alloc(alloc_size, "cache buffer");
            if (cache_buffer == NULL) {
                return SOC_E_MEMORY;
            }
        }
#endif
        MEM_LOCK(unit, mem);
        rv = _soc_mem_dma_write(unit, 0, mem, 0, copyno, index_min, index_max, 
                                buffer, cache_buffer);
        if (rv >= 0) {
            uint8 *vmap, *vmap1 = NULL;
               /* coverity[negative_returns : FALSE]    */
               /* coverity[negative_returns] */
            vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];
            if (_SOC_MEM_CHK_L2_MEM(mem)) {
                if (mem == L2_ENTRY_1m) {
                    vmap1 = SOC_MEM_STATE(unit, L2_ENTRY_2m).vmap[copyno];
                } else if (mem == L2_ENTRY_2m) {
                    vmap1 = SOC_MEM_STATE(unit, L2_ENTRY_1m).vmap[copyno];
                }
            }
    
            if (cache != NULL && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
                sal_memcpy(cache + index_min * entry_dw, cache_buffer ? 
                           cache_buffer : buffer,
                           (index_max - index_min + 1) * entry_dw * 4);
                for (i = index_min; i <= index_max; i++) {
                    if (vmap1) {
                        CACHE_VMAP_CLR(vmap, i);
                        if (mem == L2_ENTRY_1m) {
                            CACHE_VMAP_CLR(vmap1, i/2);
                        } else {
                            CACHE_VMAP_CLR(vmap1, i*2);
                            CACHE_VMAP_CLR(vmap1, i*2 + 1);
                        }
                    } else {
                        CACHE_VMAP_SET(vmap, i);
                    }
                }
            }
        }
        MEM_UNLOCK(unit, mem);
        if (NULL != meminfo->snoop_cb && 
            (SOC_MEM_SNOOP_WRITE & meminfo->snoop_flags)) {
            meminfo->snoop_cb(unit, mem, SOC_MEM_SNOOP_WRITE, copyno, index_min, 
                              index_max, buffer, meminfo->snoop_user_data);
        }
        if (cache_buffer) {
            sal_free(cache_buffer);
        }
    }
    return rv;
}

int
soc_scache_esw_nh_mem_write(int unit, uint8 *buf, int offset, int nbytes) 
{
    int num_entries, bytes2,  rv = SOC_E_NONE;
    int stable_size;
    uint32 stable_index_max, stable_index_min;
    int i, j, bufptr;
    uint8 *buf2; 
    int start, end;
    int scache_offset;
    uint8 mask[SOC_MAX_MEM_BYTES];

    SOC_IF_ERROR_RETURN(
        soc_stable_tmp_access(unit, sf_index_min, &stable_index_min, TRUE));
    SOC_IF_ERROR_RETURN(
        soc_stable_tmp_access(unit, sf_index_max, &stable_index_max, TRUE));
    SOC_IF_ERROR_RETURN(
        soc_stable_size_get(unit, &stable_size));

    bytes2 = soc_mem_entry_words(unit, EGR_L3_NEXT_HOPm) * sizeof(uint32);
    num_entries = stable_index_max - stable_index_min + 1;

    start = stable_index_min + offset / (bytes2);

    scache_offset = offset % (bytes2);
    end = start + (nbytes + scache_offset + bytes2 - 1) / (bytes2);

    buf2 = NULL;
    /* Allocate slammable memory */
    buf2 = soc_cm_salloc(unit, bytes2 * num_entries, "EGR_L3_NEXT_HOP buffer");
    if (NULL == buf2) {
        rv = SOC_E_MEMORY;
        goto cleanup;
    }
    sal_memset(buf2, 0, bytes2 * num_entries);

    /* Copy the buffer to the individual memories */
    bufptr = 0;
    sal_memset(mask, 0, SOC_MAX_MEM_BYTES); 
    soc_mem_datamask_get(unit, EGR_L3_NEXT_HOPm, (uint32 *)mask);

    SOC_IF_ERROR_RETURN
        (soc_scache_mem_read_range(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY,
                                   start, start, buf2));
    if (start != end) {
        SOC_IF_ERROR_RETURN
            (soc_scache_mem_read_range(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY,
                                       end, end, buf2 + end * bytes2));
    }

    for (i = 0; i <= (end - start); i++) {
        if (bufptr < stable_size) {
            for (j = (i == 0) ? scache_offset : 0; j < SOC_MAX_MEM_BYTES; j++) {
                if (mask[j] != 0xff) {
                    continue;
                }
                /*
                sal_memcpy(buf2 + (i - stable_index_min) * bytes2 + j, 
                           (buf + bufptr), 1);
                */
                *(buf2 + i * bytes2 + j) = *(buf + bufptr);
                bufptr++;
            }
        }
    }

    rv = soc_scache_mem_write_range(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ALL,  
                                    start, end, buf2);
cleanup:
    if (buf2 != NULL) {
        soc_cm_sfree(unit, buf2);
    }
    return rv;
}

int
soc_scache_esw_nh_mem_read(int unit, uint8 *buf, int offset, int nbytes)
{
    int i, num_entries, bytes2, rv = SOC_E_NONE;
    int j, bufptr;
    int stable_size;
    uint32 stable_index_min, stable_index_max;
    uint8 *buf2; 
    int start, end;
    int scache_offset;
    uint8 mask[SOC_MAX_MEM_BYTES];

    SOC_IF_ERROR_RETURN(
        soc_stable_tmp_access(unit, sf_index_min, &stable_index_min, TRUE));
    SOC_IF_ERROR_RETURN(
        soc_stable_tmp_access(unit, sf_index_max, &stable_index_max, TRUE));
    SOC_IF_ERROR_RETURN(
        soc_stable_size_get(unit, &stable_size));

    bytes2 = soc_mem_entry_words(unit, EGR_L3_NEXT_HOPm) * sizeof(uint32);
    num_entries = stable_index_max - stable_index_min + 1;

    buf2 = NULL;
    start = stable_index_min + offset / (bytes2);

    scache_offset = offset % (bytes2);
    end = start + (nbytes + scache_offset + bytes2 - 1) / (bytes2);

    /* Allocate DMA'able memory */
    buf2 = soc_cm_salloc(unit, bytes2 * num_entries, "EGR_L3_NEXT_HOP buffer");
    if (NULL == buf2) {
        rv = SOC_E_MEMORY;
        goto cleanup;
    }
    sal_memset(buf2, 0, bytes2 * num_entries);

    /* DMA the tables */
    rv = soc_scache_mem_read_range(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY,  
                                   stable_index_min, stable_index_max, buf2);

    /* Copy out the individual buffers to the scache buffer */
    bufptr = 0;
    sal_memset(mask, 0, SOC_MAX_MEM_BYTES);
    soc_mem_datamask_get(unit, EGR_L3_NEXT_HOPm, (uint32 *)mask);
    for (i = 0; i <= (end - start); i++) {
        if (bufptr < stable_size) {
            for (j = (i == 0) ? scache_offset : 0; j < SOC_MAX_MEM_BYTES; j++) {
                if (mask[j] != 0xff) {
                    continue;
                }
                /*
                sal_memcpy((buf + offset + bufptr), 
                            buf2 + (i - stable_index_min) * bytes2 + j, 1);
                */
                *(buf + bufptr) = *(buf2 + i * bytes2 + j);
                bufptr++;
            }
        }
    }
cleanup:
    if (buf2 != NULL) {
        soc_cm_sfree(unit, buf2);
    }
    return rv;
}
#endif /* BCM_WARM_BOOT_SUPPORT */
#endif /* defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) */

/*
 * Function:
 *    _soc_mem_write
 * Purpose:
 *    Write a memory internal to the SOC.
 * Notes:
 *    GBP/CBP memory should only accessed when MMU is in DEBUG mode.
 */

STATIC int
_soc_mem_write(int unit,
               soc_mem_t mem,
               unsigned array_index, /* in memory arrays this is the element index in the array, otherwise 0 */
               int copyno,    /* Use COPYNO_ALL for all */
               int index,
               void *entry_data)
{
    schan_msg_t schan_msg;
    int blk;
    soc_mem_info_t *meminfo; 
    int entry_dw = soc_mem_entry_words(unit, mem);
    uint32 *cache;
    uint32 maddr;
    uint8 at, *vmap;
    int no_cache;
    int index2, allow_intr=0;
    int rv;
    void *entry_data_ptr;
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    uint32 converted_entry_data[SOC_MAX_MEM_WORDS];
    uint32 cache_entry_data[SOC_MAX_MEM_WORDS];
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */

    if (!soc_mem_is_valid(unit, mem)) {
        return SOC_E_MEMORY;
    }
    meminfo = &SOC_MEM_INFO(unit, mem);

    if (index < 0) {
        index = -index;        /* Negative index bypasses cache for debug */
        no_cache = 1;
    } else {
        no_cache = 0;
    }

    if (meminfo->flags & SOC_MEM_FLAG_READONLY) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_write: attempt to write R/O memory %s\n",
                     SOC_MEM_NAME(unit, mem));
        return SOC_E_INTERNAL;
    }

#ifdef BCM_HERCULES_SUPPORT
    /* Handle Hercules' word read tables separately */
    if (meminfo->flags & SOC_MEM_FLAG_WORDADR) {
        return soc_hercules_mem_write(unit, mem, copyno, index, entry_data);
    }
#endif /* BCM_HERCULES_SUPPORT */

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    if (soc_feature(unit, soc_feature_xy_tcam) &&
        (meminfo->flags & SOC_MEM_FLAG_CAM) &&
        (!(meminfo->flags & SOC_MEM_FLAG_EXT_CAM))) {
        entry_data_ptr = converted_entry_data;
        _soc_mem_tcam_dm_to_xy(unit, mem, 1, entry_data, entry_data_ptr,
                               cache_entry_data);
    } else
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */
    {
        entry_data_ptr = entry_data;
    }

    /*
     * Setup S-Channel command packet
     *
     * NOTE: the datalen field matters only for the Write Memory and
     * Write Register commands, where it is used only by the CMIC to
     * determine how much data to send, and is in units of bytes.
     */

    schan_msg_clear(&schan_msg);
    schan_msg.writecmd.header.opcode = WRITE_MEMORY_CMD_MSG;
    schan_msg.writecmd.header.datalen = entry_dw * sizeof (uint32);
#if defined(BCM_EXTND_SBUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) ||\
    defined(BCM_DFE_SUPPORT) 
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        schan_msg.writecmd.header.srcblk =
            (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
    } else 
#endif /* BCM_EXTND_SBUS_SUPPORT */
    {
        schan_msg.writecmd.header.srcblk = SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));

#ifdef BCM_SIRIUS_SUPPORT
        schan_msg.writecmd.header.srcblk = (SOC_IS_SIRIUS(unit) ? 0 : \
                                            schan_msg.writecmd.header.srcblk);
#endif /* BCM_SIRIUS_SUPPORT */
    }
    sal_memcpy(schan_msg.writecmd.data,
               entry_data_ptr,
               entry_dw * sizeof (uint32));

    if (copyno != COPYNO_ALL) {
        if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
            soc_cm_debug(DK_WARN,
                         "soc_mem_write: invalid block %d for memory %s\n",
                         copyno, SOC_MEM_NAME(unit, mem));
            return SOC_E_PARAM;
        }
    }

    /*
     * When checking index, check for 0 instead of soc_mem_index_min.
     * Diagnostics need to read/write index 0 of Strata ARL and GIRULE.
     */

    if (index < 0 || index > soc_mem_index_max(unit, mem)) {
        soc_cm_debug(DK_WARN, "soc_mem_write: invalid index %d for memory %s\n",
                     index, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    if (soc_cm_debug_check(DK_MEM)) {
        soc_cm_print("soc_mem_write unit %d: %s.%s[%d]: ",
                     unit, SOC_MEM_NAME(unit, mem),
                     SOC_BLOCK_NAME(unit, copyno), index);
        soc_mem_entry_dump(unit, mem, entry_data_ptr);
        soc_cm_print("\n");
    }

    /* Write to one or all copies of the memory */

    rv = SOC_E_NONE;

    MEM_LOCK(unit, mem);

    SOC_MEM_BLOCK_ITER(unit, mem, blk) {
        if (copyno != COPYNO_ALL && copyno != blk) {
            continue;
        }

        index2 = index;

#if defined(BCM_FIREBOLT_SUPPORT)
#if defined(SOC_MEM_L3_DEFIP_WAR) || defined(BCM_TRIUMPH3_SUPPORT)
        if (soc_feature(unit, soc_feature_l3_defip_map) &&
            (mem == L3_DEFIPm ||
             mem == L3_DEFIP_ONLYm ||
             mem == L3_DEFIP_DATA_ONLYm ||
             mem == L3_DEFIP_HIT_ONLY_Xm ||
             mem == L3_DEFIP_HIT_ONLY_Ym ||
             mem == L3_DEFIP_HIT_ONLYm)) {
#ifdef BCM_TRIUMPH3_SUPPORT
            if (SOC_IS_TRIUMPH3(unit)) {
                index2 = soc_tr3_l3_defip_index_map(unit, mem, index);
            } else 
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
            if (SOC_IS_TD2_TT2(unit)) {
                index2 = soc_trident2_l3_defip_index_map(unit, mem, index);
            } else
#endif /* BCM_TRIDENT2_SUPPORT */
            {
#ifdef SOC_MEM_L3_DEFIP_WAR
                index2 = soc_fb_l3_defip_index_map(unit, index);
#endif /* SOC_MEM_L3_DEFIP_WAR */
            }
        }
#endif /* SOC_MEM_L3_DEFIP_WAR || BCM_TRIUMPH3_SUPPORT */
#endif /* BCM_FIREBOLT_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_feature(unit, soc_feature_l3_defip_hole) &&
            (mem == L3_DEFIPm ||
             mem == L3_DEFIP_ONLYm ||
             mem == L3_DEFIP_DATA_ONLYm ||
             mem == L3_DEFIP_HIT_ONLYm)) {
            index2 = soc_tr2_l3_defip_index_map(unit, index);
        }
#endif /* BCM_TRIUMPH2_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
        if (soc_feature(unit, soc_feature_l3_defip_map) &&
            (mem == L3_DEFIP_PAIR_128m ||
             mem == L3_DEFIP_PAIR_128_ONLYm ||
             mem == L3_DEFIP_PAIR_128_DATA_ONLYm ||
             mem == L3_DEFIP_PAIR_128_HIT_ONLY_Xm ||
             mem == L3_DEFIP_PAIR_128_HIT_ONLY_Ym ||
             mem == L3_DEFIP_PAIR_128_HIT_ONLYm)) {
            if (SOC_IS_TRIUMPH3(unit)) {
                index2 = soc_tr3_l3_defip_index_map(unit, mem, index);
            } else {
#if defined(BCM_TRIDENT2_SUPPORT)
                if SOC_IS_TD2_TT2(unit) {
                    index2 = soc_trident2_l3_defip_index_map(unit, mem, index);
                }
#endif /* BCM_TRIDENT2_SUPPORT */
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
        maddr = soc_mem_addr_get(unit, mem, array_index, blk, index2, &at);
        schan_msg.writecmd.address = maddr;
#if defined(BCM_EXTND_SBUS_SUPPORT) || defined(BCM_PETRA_SUPPORT) ||\
    defined(BCM_DFE_SUPPORT) 
        if (soc_feature(unit, soc_feature_new_sbus_format)) {
            schan_msg.writecmd.header.dstblk = SOC_BLOCK2SCH(unit, blk);
        } else 
#endif /* BCM_EXTND_SBUS_SUPPORT || BCM_PETRA_SUPPORT ||\
          BCM_DFE_SUPPORT */
        {
#if defined(BCM_XGS3_SWITCH_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) ||\
    defined(BCM_PETRA_SUPPORT)|| defined(BCM_DFE_SUPPORT)
            /* required on XGS3. Optional on other devices */
            schan_msg.writecmd.header.dstblk = ((maddr >> SOC_BLOCK_BP)
                 & 0xf) | (((maddr >> SOC_BLOCK_MSB_BP) & 0x3) << 4);
#endif /* BCM_XGS3_SWITCH_SUPPORT || BCM_SIRIUS_SUPPORT || BCM_PETRA_SUPPORT ||
          BCM_DFE_SUPPORT */
        }

#ifdef BCM_SIRIUS_SUPPORT
        if (SOC_IS_SIRIUS(unit) && (!SAL_BOOT_PLISIM || SAL_BOOT_BCMSIM)) {
            /* mask off the block field */
            schan_msg.writecmd.address &= 0x3F0FFFFF;
        }
#endif /* BCM_SIRIUS_SUPPORT */

        if (SOC_IS_SAND(unit)) {
            allow_intr = 1;
        }

        /* Write header + address + entry_dw data DWORDs */
        /* Note: The hardware does not send WRITE_MEMORY_ACK_MSG. */
        if (2 + entry_dw > CMIC_SCHAN_WORDS(unit)) {
           soc_cm_debug(DK_WARN, "soc_mem_write: assert will fail for memory %s\n", SOC_MEM_NAME(unit, mem));
        }
#ifdef BCM_ESW_SUPPORT
_retry_op:
#endif /* BCM_ESW_SUPPORT */
        if ((rv = soc_schan_op(unit, &schan_msg,
                   2 + entry_dw, 0, allow_intr)) < 0) {
#ifdef BCM_ESW_SUPPORT
            if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                int rv1;
                
                rv1 = soc_ser_sram_correction(unit, SOC_PIPE_ANY,
                                              schan_msg.writecmd.header.dstblk,
                                              mem, schan_msg.writecmd.address,
                                              blk, index2);
                if (rv1 == SOC_E_NONE) {
                    goto _retry_op;
                } 
            }
#endif /* BCM_ESW_SUPPORT */
            goto done;
        }

        /* Write back to cache if active */
        cache = SOC_MEM_STATE(unit, mem).cache[blk];
        vmap = SOC_MEM_STATE(unit, mem).vmap[blk];

        if (cache != NULL && !no_cache && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
            if (SOC_WARM_BOOT(unit) || SOC_IS_RELOADING(unit) || 
                SOC_HW_ACCESS_DISABLE(unit)) {
                CACHE_VMAP_CLR(vmap, index);
            } else if (_SOC_MEM_CHK_L2_MEM(mem)) {
                /* If an invalid entry is being written then clear the cache */
                if (((mem == L2_ENTRY_2m && 
                     soc_L2_ENTRY_2m_field32_get(unit, entry_data, VALID_0f) &&
                     soc_L2_ENTRY_2m_field32_get(unit, entry_data, VALID_1f)) ||                    
                     ((mem == L2Xm || mem == L2_ENTRY_1m) &&
                      soc_mem_field32_get(unit, mem, entry_data, VALIDf))) &&
                     ((mem == L2_ENTRY_2m && 
                      (soc_mem_field32_get(unit, mem, entry_data, STATIC_BIT_0f) &&
                       soc_mem_field32_get(unit, mem, entry_data, STATIC_BIT_1f))) ||
                       ((mem == L2Xm || mem == L2_ENTRY_1m) &&
                        soc_mem_field32_get(unit, mem, entry_data, STATIC_BITf)))) {
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT)
                    sal_memcpy(cache + index * entry_dw,
                               (entry_data_ptr == converted_entry_data) ?
                               cache_entry_data : entry_data, entry_dw * 4);
#else
                    sal_memcpy(cache + index * entry_dw, entry_data, entry_dw * 4);
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */
                    CACHE_VMAP_SET(vmap, index);
                } else {
                    CACHE_VMAP_CLR(vmap, index);
                }
                if (mem == L2_ENTRY_1m) {
                    vmap = SOC_MEM_STATE(unit, L2_ENTRY_2m).vmap[blk];
                    CACHE_VMAP_CLR(vmap, index/2);
                } else if (mem == L2_ENTRY_2m) {
                    vmap = SOC_MEM_STATE(unit, L2_ENTRY_1m).vmap[blk];
                    CACHE_VMAP_CLR(vmap, index*2);
                    CACHE_VMAP_CLR(vmap, index*2 + 1);
                }
            } else {
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT)
                if (entry_data_ptr == converted_entry_data) {
                    sal_memcpy(cache + index * entry_dw,
                               cache_entry_data, entry_dw * 4);
#if defined(INCLUDE_MEM_SCAN)
                    /* Update memscan TCAM cache if necessary */
                    soc_mem_scan_tcam_cache_update(unit, mem,
                                                   index, index,
                                                   entry_data_ptr);
#endif /* INCLUDE_MEM_SCAN */
                } else {
                    sal_memcpy(cache + index * entry_dw,
                               entry_data, entry_dw * 4);
                }
#else
                sal_memcpy(cache + index * entry_dw, entry_data, entry_dw * 4);
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */
                CACHE_VMAP_SET(vmap, index);
            }
        }
    }

done:
    MEM_UNLOCK(unit, mem);
    if (NULL != meminfo->snoop_cb && 
        (SOC_MEM_SNOOP_WRITE & meminfo->snoop_flags)) {
        meminfo->snoop_cb(unit, mem, SOC_MEM_SNOOP_WRITE, copyno, index, index, 
                          entry_data_ptr, meminfo->snoop_user_data);
    }

    return rv;
}

/*
 * Function:
 *    soc_mem_pipe_select_write
 * Purpose:
 *    Write a memory internal to the SOC with pipe selection.
 * Notes:
 *    GBP/CBP memory should only accessed when MMU is in DEBUG mode.
 */

int
soc_mem_pipe_select_write(int unit,
                          soc_mem_t mem,
                          int copyno,    /* Use COPYNO_ALL for all */
                          int acc_type,
                          int index,
                          void *entry_data)
{
    schan_msg_t schan_msg;
    int blk;
    soc_mem_info_t *meminfo; 
    int entry_dw = soc_mem_entry_words(unit, mem);
    uint32 *cache;
    uint32 maddr;
    uint8 at, *vmap;
    int no_cache;
    int index2, allow_intr=0;
    int rv;
    void *entry_data_ptr;
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT)
    uint32 converted_entry_data[SOC_MAX_MEM_WORDS];
    uint32 cache_entry_data[SOC_MAX_MEM_WORDS];
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */

    if (!soc_feature(unit, soc_feature_two_ingress_pipes)) {
        /* Not a relevant device */
        return SOC_E_UNAVAIL;
    }

    if (!soc_mem_is_valid(unit, mem)) {
        return SOC_E_MEMORY;
    }
    meminfo = &SOC_MEM_INFO(unit, mem);

    if (index < 0) {
        index = -index;        /* Negative index bypasses cache for debug */
        no_cache = 1;
    } else {
        no_cache = 0;
    }

    if (meminfo->flags & SOC_MEM_FLAG_READONLY) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_write: attempt to write R/O memory %s\n",
                     SOC_MEM_NAME(unit, mem));
        return SOC_E_INTERNAL;
    }

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT)
    if (soc_feature(unit, soc_feature_xy_tcam) &&
        (meminfo->flags & SOC_MEM_FLAG_CAM) &&
        (!(meminfo->flags & SOC_MEM_FLAG_EXT_CAM))) {
        entry_data_ptr = converted_entry_data;
        _soc_mem_tcam_dm_to_xy(unit, mem, 1, entry_data, entry_data_ptr,
                               cache_entry_data);
    } else
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */
    {
        entry_data_ptr = entry_data;
    }

    /*
     * Setup S-Channel command packet
     *
     * NOTE: the datalen field matters only for the Write Memory and
     * Write Register commands, where it is used only by the CMIC to
     * determine how much data to send, and is in units of bytes.
     */

    schan_msg_clear(&schan_msg);
    schan_msg.writecmd.header.opcode = WRITE_MEMORY_CMD_MSG;
    schan_msg.writecmd.header.datalen = entry_dw * sizeof (uint32);
#if defined(BCM_EXTND_SBUS_SUPPORT)
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        if (0 == acc_type) {
            acc_type =
                (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
        }
        schan_msg.readcmd.header.srcblk = acc_type;
    } else 
#endif /* BCM_EXTND_SBUS_SUPPORT */
    {
        schan_msg.writecmd.header.srcblk = SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));

#ifdef BCM_SIRIUS_SUPPORT
        schan_msg.writecmd.header.srcblk = (SOC_IS_SIRIUS(unit) ? 0 : \
                                            schan_msg.writecmd.header.srcblk);
#endif /* BCM_SIRIUS_SUPPORT */
    }
    sal_memcpy(schan_msg.writecmd.data,
               entry_data_ptr,
               entry_dw * sizeof (uint32));

    if (copyno != COPYNO_ALL) {
        if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
            soc_cm_debug(DK_WARN,
                         "soc_mem_write: invalid block %d for memory %s\n",
                         copyno, SOC_MEM_NAME(unit, mem));
            return SOC_E_PARAM;
        }
    }

    /*
     * When checking index, check for 0 instead of soc_mem_index_min.
     * Diagnostics need to read/write index 0 of Strata ARL and GIRULE.
     */

    if (index < 0 || index > soc_mem_index_max(unit, mem)) {
        soc_cm_debug(DK_WARN, "soc_mem_pipe_select_write: invalid index %d for memory %s acc_type %d\n",
                     index, SOC_MEM_NAME(unit, mem), acc_type);
        return SOC_E_PARAM;
    }

    if (soc_cm_debug_check(DK_MEM)) {
        soc_cm_print("soc_mem_pipe_select_write unit %d: %s.%s[%d]: ",
                     unit, SOC_MEM_NAME(unit, mem),
                     SOC_BLOCK_NAME(unit, copyno), index);
        soc_mem_entry_dump(unit, mem, entry_data_ptr);
        soc_cm_print("\n");
    }

    /* Write to one or all copies of the memory */

    rv = SOC_E_NONE;

    MEM_LOCK(unit, mem);

    SOC_MEM_BLOCK_ITER(unit, mem, blk) {
        if (copyno != COPYNO_ALL && copyno != blk) {
            continue;
        }

        index2 = index;

    
#if defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TD2_TT2(unit) &&
            soc_feature(unit, soc_feature_l3_defip_map) &&
            (mem == L3_DEFIPm ||
             mem == L3_DEFIP_ONLYm ||
             mem == L3_DEFIP_DATA_ONLYm ||
             mem == L3_DEFIP_HIT_ONLY_Xm ||
             mem == L3_DEFIP_HIT_ONLY_Ym ||
             mem == L3_DEFIP_HIT_ONLYm ||
             mem == L3_DEFIP_PAIR_128m ||
             mem == L3_DEFIP_PAIR_128_ONLYm ||
             mem == L3_DEFIP_PAIR_128_DATA_ONLYm ||
             mem == L3_DEFIP_PAIR_128_HIT_ONLY_Xm ||
             mem == L3_DEFIP_PAIR_128_HIT_ONLY_Ym ||
             mem == L3_DEFIP_PAIR_128_HIT_ONLYm)) {
            index2 = soc_trident2_l3_defip_index_map(unit, mem, index);
        }
#endif /* BCM_TRIDENT2_SUPPORT */
        maddr = soc_mem_addr_get(unit, mem, 0, blk, index2, &at);
#if defined(BCM_EXTND_SBUS_SUPPORT)
        if (!soc_feature(unit, soc_feature_new_sbus_format)) {
            /* Override ACC_TYPE in address */
            maddr &= ~(_SOC_MEM_ADDR_ACC_TYPE_MASK <<
                            _SOC_MEM_ADDR_ACC_TYPE_SHIFT);
            maddr |= (acc_type & _SOC_MEM_ADDR_ACC_TYPE_MASK) <<
                            _SOC_MEM_ADDR_ACC_TYPE_SHIFT;
        }
#endif /* BCM_EXTND_SBUS_SUPPORT */
        schan_msg.writecmd.address = maddr;
#if defined(BCM_EXTND_SBUS_SUPPORT)
        if (soc_feature(unit, soc_feature_new_sbus_format)) {
            schan_msg.writecmd.header.dstblk = SOC_BLOCK2SCH(unit, blk);
        } else 
#endif /* BCM_EXTND_SBUS_SUPPORT */
        {
#if defined(BCM_XGS3_SWITCH_SUPPORT)
            /* required on XGS3. Optional on other devices */
            schan_msg.writecmd.header.dstblk = ((maddr >> SOC_BLOCK_BP)
                 & 0xf) | (((maddr >> SOC_BLOCK_MSB_BP) & 0x3) << 4);
#endif /* BCM_XGS3_SWITCH_SUPPORT */
        }

        /* Write header + address + entry_dw data DWORDs */
        /* Note: The hardware does not send WRITE_MEMORY_ACK_MSG. */
        if (2 + entry_dw > CMIC_SCHAN_WORDS(unit)) {
           soc_cm_debug(DK_WARN, "soc_mem_pipe_select_write: assert will fail for memory %s\n", SOC_MEM_NAME(unit, mem));
        }
#ifdef BCM_ESW_SUPPORT
_retry_op:
#endif /* BCM_ESW_SUPPORT */
        if ((rv = soc_schan_op(unit, &schan_msg,
                   2 + entry_dw, 0, allow_intr)) < 0) {
#ifdef BCM_ESW_SUPPORT
            if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                int rv1;
                
                rv1 = soc_ser_sram_correction(unit, SOC_PIPE_ANY,
                                              schan_msg.writecmd.header.dstblk,
                                              schan_msg.writecmd.address, mem,
                                              blk, index2);
                if (rv1 == SOC_E_NONE) {
                    goto _retry_op;
                } 
            }
#endif /* BCM_ESW_SUPPORT */
            goto done;
        }

        /* Write back to cache if active */
        cache = SOC_MEM_STATE(unit, mem).cache[blk];
        vmap = SOC_MEM_STATE(unit, mem).vmap[blk];

        if (cache != NULL && !no_cache && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
            if (SOC_WARM_BOOT(unit) || SOC_IS_RELOADING(unit) || 
                SOC_HW_ACCESS_DISABLE(unit)) {
                CACHE_VMAP_CLR(vmap, index);
            } else if (_SOC_MEM_CHK_L2_MEM(mem)) {
                /* If an invalid entry is being written then clear the cache */
                if (((mem == L2_ENTRY_2m && 
                     soc_L2_ENTRY_2m_field32_get(unit, entry_data, VALID_0f) &&
                     soc_L2_ENTRY_2m_field32_get(unit, entry_data, VALID_1f)) ||                    
                     ((mem == L2Xm || mem == L2_ENTRY_1m) &&
                      soc_mem_field32_get(unit, mem, entry_data, VALIDf))) &&
                     ((mem == L2_ENTRY_2m && 
                      (soc_mem_field32_get(unit, mem, entry_data, STATIC_BIT_0f) &&
                       soc_mem_field32_get(unit, mem, entry_data, STATIC_BIT_1f))) ||
                       ((mem == L2Xm || mem == L2_ENTRY_1m) &&
                        soc_mem_field32_get(unit, mem, entry_data, STATIC_BITf)))) {
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT)
                    sal_memcpy(cache + index * entry_dw,
                               (entry_data_ptr == converted_entry_data) ?
                               cache_entry_data : entry_data, entry_dw * 4);
#else
                    sal_memcpy(cache + index * entry_dw, entry_data, entry_dw * 4);
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */
                    CACHE_VMAP_SET(vmap, index);
                } else {
                    CACHE_VMAP_CLR(vmap, index);
                }
                if (mem == L2_ENTRY_1m) {
                    vmap = SOC_MEM_STATE(unit, L2_ENTRY_2m).vmap[blk];
                    CACHE_VMAP_CLR(vmap, index/2);
                } else if (mem == L2_ENTRY_2m) {
                    vmap = SOC_MEM_STATE(unit, L2_ENTRY_1m).vmap[blk];
                    CACHE_VMAP_CLR(vmap, index*2);
                    CACHE_VMAP_CLR(vmap, index*2 + 1);
                }
            } else {
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT)
                sal_memcpy(cache + index * entry_dw,
                           (entry_data_ptr == converted_entry_data) ?
                           cache_entry_data : entry_data, entry_dw * 4);
#else
                sal_memcpy(cache + index * entry_dw, entry_data, entry_dw * 4);
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */
                CACHE_VMAP_SET(vmap, index);
            }
        }
    }

done:
    MEM_UNLOCK(unit, mem);
    if (NULL != meminfo->snoop_cb && 
        (SOC_MEM_SNOOP_WRITE & meminfo->snoop_flags)) {
        meminfo->snoop_cb(unit, mem, SOC_MEM_SNOOP_WRITE, copyno, index, index, 
                          entry_data_ptr, meminfo->snoop_user_data);
    }

    return rv;
}

#ifdef BCM_TRIUMPH2_SUPPORT
STATIC int
_soc_mem_op_cpu_tdm(int unit, int enable)
{
    if (enable) {
        soc_IARB_TDM_TABLEm_field32_set(unit, 
                                       &(SOC_CONTROL(unit)->iarb_tdm), 
                                       PORT_NUMf, 0);
    } else {
        soc_IARB_TDM_TABLEm_field32_set(unit, 
                                       &(SOC_CONTROL(unit)->iarb_tdm), 
                                       PORT_NUMf, 63);
    }
    return _soc_mem_write(unit, IARB_TDM_TABLEm, 0, SOC_BLOCK_ALL, 
                          SOC_CONTROL(unit)->iarb_tdm_idx, 
                          &(SOC_CONTROL(unit)->iarb_tdm));
}
#endif /* BCM_TRIUMPH2_SUPPORT */

/*
 * Function:
 *    soc_mem_write
 * Purpose:
 *    Write a memory internal to the SOC.
 * Notes:
 *    GBP/CBP memory should only accessed when MMU is in DEBUG mode.
 */

int
soc_mem_write(int unit,
              soc_mem_t mem,
              int copyno,    /* Use COPYNO_ALL for all */
              int index,
              void *entry_data)
{
    _SOC_MEM_REPLACE_MEM(unit, mem);
    return soc_mem_array_write(unit, mem, 0, copyno, index, entry_data);
}

/*
 * Function:    soc_mem_array_write
 * Purpose:     Write a memory array internal to the SOC.
 */
int
soc_mem_array_write(int unit, soc_mem_t mem, unsigned array_index, int copyno,/* Use COPYNO_ALL for all */
              int index, void *entry_data)
{
    int rv;
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    soc_mem_t     aggr_mem = INVALIDm;
    void          *shift_buffer = NULL;
#endif

    /* if reloading, dont write to register */
    if (SOC_IS_RELOADING(unit)){     
        return SOC_E_NONE;
    }

#if defined(BCM_PETRAB_SUPPORT)
    if (SOC_IS_PETRAB(unit)) {
        return soc_dpp_mem_write(unit, mem, copyno, index, entry_data);
    }
#endif /* BCM_PETRAB_SUPPORT */

#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit)) {
        if ((mem == LMEPm) || (mem == LMEP_1m)) {
            /* Disable CPU slot from TDM */
            SOC_IF_ERROR_RETURN(_soc_mem_op_cpu_tdm(unit, 0));
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    if (soc_feature(unit, soc_feature_xy_tcam) &&
        SOC_CONTROL(unit)->tcam_protect_write &&
        (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) &&
        (!(SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_EXT_CAM))) {
        SOC_IF_ERROR_RETURN
            (_soc_mem_tcam_entry_preserve(unit, mem, copyno, index, 1,
                                          entry_data, &aggr_mem,
                                          &shift_buffer));
    }
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */

    rv = _soc_mem_write(unit, mem, array_index, copyno, index, entry_data);
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
    if (soc_feature(unit, soc_feature_xy_tcam) &&
        SOC_CONTROL(unit)->tcam_protect_write &&
        (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) &&
        (!(SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_EXT_CAM))) {
            SOC_IF_ERROR_RETURN
                (_soc_mem_tcam_entry_restore(unit, aggr_mem, copyno, index, 1,
                                             shift_buffer));
    }
#endif /* BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT */

    SOC_IF_ERROR_RETURN(rv);

#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit)) {
        if ((mem == LMEPm) || (mem == LMEP_1m)) {
            /* Enable CPU slot from TDM */
            SOC_IF_ERROR_RETURN(_soc_mem_op_cpu_tdm(unit, 1));
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    return rv;
}

/*
 * Function:
 *    soc_mem_write_range
 * Purpose:
 *    Write a range of chip's memory with multiple entries
 */

int
soc_mem_write_range(int unit, soc_mem_t mem, int copyno,
                    int index_min, int index_max, void *buffer)
{
    int rc;

#ifdef SOC_MEM_DEBUG_SPEED
e
    sal_usecs_t start_time;
    int diff_time, count = (index_max > index_min) ? 
                           (index_max - index_min + 1) : 
                           (index_min - index_max + 1);

    start_time = sal_time_usecs();
#endif

    rc = soc_mem_array_write_range(unit, 0, mem, 0, copyno, index_min, index_max, 
                                     buffer);

#ifdef SOC_MEM_DEBUG_SPEED
    diff_time = SAL_USECS_SUB(sal_time_usecs(), start_time);
    soc_cm_debug(DK_SOCMEM | DK_VERBOSE,
                 "Total dma write time: %d usecs, [%d nsecs per op]\n",
                 diff_time, diff_time*1000/count);
#endif   
 return rc;
}

/*
 * Function:
 *    soc_mem_array_write_range
 * Purpose:
 *    Write a range of chip's memory with multiple entries
 *
 */

int
soc_mem_array_write_range(int unit, uint32 flags, soc_mem_t mem, unsigned array_index, 
                          int copyno, int index_min, int index_max, void *buffer)
{
    uint32          entry_dw;
    int             i;
    soc_mem_info_t  *meminfo;
    void            *cache_buffer = NULL;
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)

    int             count, alloc_size;
#endif
    uint32          *cache;

    /* if reloading, dont write to register */
    if (SOC_IS_RELOADING(unit))
    {
        return SOC_E_NONE;
    }

    if (!soc_mem_is_valid(unit, mem)) {
        return SOC_E_MEMORY;
    }

    meminfo = &SOC_MEM_INFO(unit, mem);
    entry_dw = soc_mem_entry_words(unit, mem);

    soc_cm_debug(DK_SOCMEM,
                 "soc_mem_array_write_range: unit %d memory %s.%s [%d:%d]\n",
                 unit, SOC_MEM_UFNAME(unit, mem),
                 SOC_BLOCK_NAME(unit, copyno),
                 index_min, index_max);

    /*    coverity[negative_returns : FALSE]    */
    if (soc_mem_slamable(unit, mem, copyno)) {
        int    rv;
        int blk;

        if (copyno == COPYNO_ALL) {
            SOC_MEM_BLOCK_ITER(unit, mem, blk) {
                copyno = blk;
                break;
            }
        }
        if (copyno == COPYNO_ALL) {
            return SOC_E_INTERNAL;
        }
        cache = SOC_MEM_STATE(unit, mem).cache[copyno];
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT) \
    || defined(BCM_HURRICANE2_SUPPORT)
        if (cache != NULL && !SOC_MEM_TEST_SKIP_CACHE(unit)
            && (soc_feature(unit, soc_feature_xy_tcam) &&
            (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) &&
            (!(SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_EXT_CAM)))) {
            if (index_min > index_max) {
                count = index_min - index_max + 1;
            } else {
                count = index_max - index_min + 1;
            }
            alloc_size = count * entry_dw * sizeof(uint32);
            cache_buffer = sal_alloc(alloc_size, "cache buffer");
            if (cache_buffer == NULL) {
                return SOC_E_MEMORY;
            }
        }
#endif

        MEM_LOCK(unit, mem);
        rv = _soc_mem_dma_write(unit, flags, mem, array_index, copyno, index_min,
                                index_max, buffer, cache_buffer);
        if (rv >= 0) {
            uint8 *vmap, *vmap1 = NULL;

            /*    coverity[negative_returns : FALSE]    */
            /* coverity[negative_returns] */
            vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];
            if (_SOC_MEM_CHK_L2_MEM(mem)) {
                if (mem == L2_ENTRY_1m) {
                    vmap1 = SOC_MEM_STATE(unit, L2_ENTRY_2m).vmap[copyno];
                } else if (mem == L2_ENTRY_2m) {
                    vmap1 = SOC_MEM_STATE(unit, L2_ENTRY_1m).vmap[copyno];
                }
            }
            if (cache != NULL && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
                sal_memcpy(cache + index_min * entry_dw, cache_buffer ? 
                           cache_buffer : buffer,
                           (index_max - index_min + 1) * entry_dw * 4);
                for (i = index_min; i <= index_max; i++) {
                    if (vmap1) {
                        CACHE_VMAP_CLR(vmap, i);
                        if (mem == L2_ENTRY_1m) {
                            CACHE_VMAP_CLR(vmap1, i/2);
                        } else {
                            CACHE_VMAP_CLR(vmap1, i*2);
                            CACHE_VMAP_CLR(vmap1, i*2 + 1);
                        }
                    } else {
                        CACHE_VMAP_SET(vmap, i);
                    }
                }
           }
        }
        MEM_UNLOCK(unit, mem);
        if (NULL != meminfo->snoop_cb && 
            (SOC_MEM_SNOOP_WRITE & meminfo->snoop_flags)) {
            meminfo->snoop_cb(unit, mem, SOC_MEM_SNOOP_WRITE, copyno, index_min, 
                              index_max, buffer, meminfo->snoop_user_data);
        }
        if (cache_buffer) {
            sal_free(cache_buffer);
        }
        return rv;
    }

    for (i = index_min; i <= index_max; i++) {
        SOC_IF_ERROR_RETURN(soc_mem_array_write(unit, mem, array_index, copyno, 
                                                i, buffer));
        buffer = ((uint32 *)buffer + entry_dw);
    }
    return SOC_E_NONE;
}

/************************************************************************
 * Routines for reading/writing sorted tables except ARL,        *
 * and searching sorted tables including the ARL.            *
 * Sorted tables are:  IRULE, L3, MCAST, PVLAN                *
 ************************************************************************/

/*
 * Function:
 *    soc_mem_clear
 * Purpose:
 *    Clears a memory.
 *    Operates on all copies of the table if copyno is COPYNO_ALL.
 * Notes:
 *    For non-sorted tables, all entries are cleared.  For sorted
 *    tables, only the entries known to contain data are cleared,
 *    unless the force_all flag is on, in which case all entries
 *    are cleared.  The force_all flag is mainly intended for the first
 *    initialization after a chip reset.
 */

int
soc_mem_clear(int unit,
              soc_mem_t mem,
              int copyno,
              int force_all)
{
    int           rv = SOC_E_NONE;
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    int           blk, index_min, index_max, index;
#endif
#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    uint8         *vmap, *vmap1 = NULL;
    uint32        entry_dw, *cache;
    void          *null_entry = soc_mem_entry_null(unit, mem);
    uint32        empty_entry[SOC_MAX_MEM_WORDS] = {0};
#if defined(INCLUDE_MEM_SCAN) && \
    (defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT))
    uint32        xy_null_entry[SOC_MAX_MEM_WORDS] = {0};
#endif /* INCLUDE_MEM_SCAN && (BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT) */
#endif /* BCM_FIREBOLT_SUPPORT || BCM_SIRIUS_SUPPORT */

    _SOC_MEM_REPLACE_MEM(unit, mem);
    if (soc_mem_index_count(unit, mem) == 0) {
        return SOC_E_NONE;
    }
    
#if defined(BCM_FIREBOLT_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (SOC_IS_FBX(unit) || SOC_IS_SIRIUS(unit)) {
        if (!null_entry) {
            null_entry = empty_entry;
        }
        MEM_LOCK(unit, mem);
        SOC_MEM_BLOCK_ITER(unit, mem, blk) {
            if (copyno != COPYNO_ALL && copyno != blk) {
                continue;
            }
            index_min = soc_mem_index_min(unit, mem);
            index_max = soc_mem_index_max(unit, mem);
            if (SOC_MEM_CLEAR_USE_DMA(unit)) {
                rv = _soc_xgs3_mem_clear_pipe(unit, mem, blk, null_entry);
                if (rv == SOC_E_UNAVAIL) {
                    if (soc_mem_slamable(unit, mem, blk)) {
                        rv = _soc_xgs3_mem_clear_slam(unit, mem, blk, null_entry);
                    } else {
                        goto _clear_use_pio;
                    }
                }
                if (rv < 0) {
                    goto check_cache;
                }
            } else {
_clear_use_pio:
                for (index = index_min; index <= index_max; index++) {
                    if ((rv = soc_mem_write(unit, mem, blk, index,
                                            null_entry)) < 0) {
                        soc_cm_debug(DK_ERR,
                                     "soc_mem_clear: "
                                     "write %s.%s[%d] failed: %s\n",
                                     SOC_MEM_UFNAME(unit, mem),
                                     SOC_BLOCK_NAME(unit, blk),
                                     index, soc_errmsg(rv));
                        goto done;
                    }
                }
            }
check_cache:
            /* Update cache if active */
            cache = SOC_MEM_STATE(unit, mem).cache[blk];
            vmap = SOC_MEM_STATE(unit, mem).vmap[blk];
            if (_SOC_MEM_CHK_L2_MEM(mem)) {
                if (mem == L2_ENTRY_1m) {
                    vmap1 = SOC_MEM_STATE(unit, L2_ENTRY_2m).vmap[copyno];
                } else if (mem == L2_ENTRY_2m) {
                    vmap1 = SOC_MEM_STATE(unit, L2_ENTRY_1m).vmap[copyno];
                }
            }
            entry_dw = soc_mem_entry_words(unit, mem);
            if (cache != NULL && !SOC_MEM_TEST_SKIP_CACHE(unit) &&
                !SOC_WARM_BOOT(unit)) {
#if defined(INCLUDE_MEM_SCAN) && \
    (defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT))
                if (soc_feature(unit, soc_feature_xy_tcam) &&
                    (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) &&
                    (!(SOC_MEM_INFO(unit, mem).flags &
                               SOC_MEM_FLAG_EXT_CAM))) {
                    _soc_mem_tcam_dm_to_xy(unit, mem, 1, null_entry,
                                           xy_null_entry, NULL);
                }
#endif /* INCLUDE_MEM_SCAN && (BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT) */
                for (index = index_min; index <= index_max; index++) {
                    if (SOC_WARM_BOOT(unit) || SOC_IS_RELOADING(unit) || 
                        SOC_HW_ACCESS_DISABLE(unit)) {
                        CACHE_VMAP_CLR(vmap, index);
                    } else if (vmap1) {
                        CACHE_VMAP_CLR(vmap, index);
                        if (mem == L2_ENTRY_1m) {
                            CACHE_VMAP_CLR(vmap1, index/2);
                        } else {
                            CACHE_VMAP_CLR(vmap1, index*2);
                            CACHE_VMAP_CLR(vmap1, index*2 + 1);
                        }
                    } else {
                        sal_memcpy(cache + index * entry_dw, null_entry,
                                   entry_dw * 4);
                        CACHE_VMAP_CLR(vmap, index);
#if defined(INCLUDE_MEM_SCAN) && \
    (defined(BCM_TRIDENT_SUPPORT) || defined(BCM_SHADOW_SUPPORT))
                        if (soc_feature(unit, soc_feature_xy_tcam) &&
                            (SOC_MEM_INFO(unit, mem).flags &
                             SOC_MEM_FLAG_CAM) &&
                            (!(SOC_MEM_INFO(unit, mem).flags &
                               SOC_MEM_FLAG_EXT_CAM))) {
                            /* Update memscan TCAM cache if necessary */
                            soc_mem_scan_tcam_cache_update(unit, mem,
                                                           index, index,
                                                           xy_null_entry);
                        }
#endif /* INCLUDE_MEM_SCAN && (BCM_TRIDENT_SUPPORT || BCM_SHADOW_SUPPORT) */
                    }
                }
            }
        }
        goto done; /* Required if block iter does not execute even once */
    }
#endif /* BCM_FIREBOLT_SUPPORT || BCM_SIRIUS_SUPPORT */

#if defined(BCM_XGS12_FABRIC_SUPPORT)
    if (copyno != COPYNO_ALL) {
        assert(SOC_MEM_BLOCK_VALID(unit, mem, copyno));
    }

    MEM_LOCK(unit, mem);

    SOC_MEM_BLOCK_ITER(unit, mem, blk) {
        if (copyno != COPYNO_ALL && copyno != blk) {
            continue;
        }

        index_min = soc_mem_index_min(unit, mem);
        index_max = soc_mem_index_max(unit, mem);

        switch (mem) {
#if defined(BCM_XGS_SWITCH_SUPPORT)
        case L3_IPMCm:
#endif
#ifdef BCM_XGS12_FABRIC_SUPPORT
        case MEM_UCm:
        case MEM_MCm:
        case MEM_VIDm:
        case MEM_IPMCm:
#ifdef BCM_HERCULES15_SUPPORT
        case MEM_EGR_MODMAPm:
        case MEM_ING_MODMAPm:
        case MEM_ING_SRCMODBLKm:
        case MEM_TRUNK_PORT_POOLm:
#endif /* BCM_HERCULES15_SUPPORT */
#endif /* BCM_XGS12_FABRIC_SUPPORT */
#ifdef BCM_XGS_SWITCH_SUPPORT
        case EGR_MASKm:
        case MAC_BLOCKm:
        case PORT_TABm:
        case STG_TABm:
        case TRUNK_GROUPm:
        case TRUNK_BITMAPm:
        case TRUNK_EGR_MASKm:
        case VLAN_TABm:
#endif /* BCM_XGS_SWITCH_SUPPORT */
            /* NOTE: fall through from above */
            /* Tables where all entries should be cleared to null value */
            for (index = index_min; index <= index_max; index++) {
                if ((rv = soc_mem_write(unit, mem, blk, index,
                                        soc_mem_entry_null(unit, mem))) < 0) {
                    soc_cm_debug(DK_ERR,
                                 "soc_mem_clear: "
                                 "write %s.%s[%d] failed: %s\n",
                                 SOC_MEM_UFNAME(unit, mem),
                                 SOC_BLOCK_NAME(unit, blk),
                                 index, soc_errmsg(rv));
                    goto done;
                }
            }
            break;
#if defined(BCM_5690) || defined(BCM_5665)
        case GFILTER_FFPCOUNTERSm:
        case GFILTER_FFPPACKETCOUNTERSm:
#if defined(BCM_5695)
        case GFILTER_FFP_IN_PROFILE_COUNTERSm:
        case GFILTER_FFP_OUT_PROFILE_COUNTERSm:
#endif
        case GFILTER_METERINGm:
#endif
#if defined(BCM_5673) || defined(BCM_5674)
        case XFILTER_FFPCOUNTERSm:
        case XFILTER_FFPOPPACKETCOUNTERSm:
        case XFILTER_FFPIPPACKETCOUNTERSm:
        case XFILTER_FFPOPBYTECOUNTERSm:
        case XFILTER_FFPIPBYTECOUNTERSm:
        case XFILTER_METERINGm:
#endif
#if defined(BCM_5690) || defined(BCM_5665) || \
    defined(BCM_5673) || defined(BCM_5674)
            if (soc_feature(unit, soc_feature_filter_metering)) {
                /* NOTE: fall through from above */
                /* Tables where all entries should be cleared to null value */
                for (index = index_min; index <= index_max; index++) {
                    if ((rv = soc_mem_write(unit, mem, blk, index,
                                            soc_mem_entry_null(unit,
                                                               mem))) < 0) {
                        soc_cm_debug(DK_ERR,
                                     "soc_mem_clear: "
                                     "write %s.%s[%d] failed: %s\n",
                                     SOC_MEM_UFNAME(unit, mem),
                                     SOC_BLOCK_NAME(unit, blk),
                                     index, soc_errmsg(rv));
                        goto done;
                    }
                }
                break;
            } /* else fall thru to default */
#endif /* BCM_XGS_SWITCH_SUPPORT */
        default:
            soc_cm_debug(DK_ERR,
                         "soc_mem_clear: don't know how to clear %s\n",
                         SOC_MEM_UFNAME(unit, mem));
            rv = SOC_E_PARAM;
            goto done;
        }
    }
#endif /* XGS12_FABRIC */

    rv = SOC_E_NONE;
    goto done;

done:
    MEM_UNLOCK(unit, mem);

    soc_cm_debug(DK_SOCMEM,
                 "soc_mem_clear: unit %d memory %s.%s returns %s\n",
                 unit, SOC_MEM_UFNAME(unit, mem),
                 SOC_BLOCK_NAME(unit, copyno), soc_errmsg(rv));

    return rv;
}

/*
 * Function:
 *      _soc_mem_search
 * Purpose:
 *      Helper function for soc_mem_search.  Does the raw search
 *      without worrying about special memory cases.
 * Parameters:
 *      See soc_mem_search, below.
 * Returns:
 *  SOC_E_NOT_FOUND
 *  Entry not found:  in this case, if
 *                           table is sorted, then index_ptr gets the
 *                           location in which the entry should be
 *                           inserted.
 *  SOC_E_NONE       Entry is found:  index_ptr gets location
 *  SOC_E_XXX        If internal error occurs
 * Notes:
 *  See soc_mem_search, below.
 */

STATIC int
_soc_mem_search(int unit,
               soc_mem_t mem,
               int copyno,
               int *index_ptr,
               void *key_data,
               void *entry_data,
               int lowest_match)
{
    int base, count, mid, last_read = -1;
    int rv, r;

    /*
     * Establish search range within table.  For filter, optimize using
     * cached start/count of rule set for mask specified by FSEL.  Note
     * that all rules using a given FSEL are necessarily contiguous.
     */
    base = soc_mem_index_min(unit, mem);
    count = soc_mem_entries(unit, mem, copyno);

    while (count > 0) {
        mid = base + count / 2;

        if ((rv = soc_mem_read(unit, mem, copyno, mid, entry_data)) < 0) {
            soc_cm_debug(DK_ERR,
                         "soc_mem_search: read %s.%s[%d] failed\n",
                         SOC_MEM_UFNAME(unit, mem),
                         SOC_BLOCK_NAME(unit, copyno), mid);
            return rv;
        }

        last_read = mid;

        r = soc_mem_compare_key(unit, mem, key_data, entry_data);

        if (r == 0) {
            if (lowest_match) {
                count = count / 2;      /* Must treat same as < 0 */
            } else {
                if (index_ptr != NULL) {
                    *index_ptr = mid;   /* Don't care if it's lowest match */
                }
                return SOC_E_NONE;              /* Found */
            }
        } else if (r < 0) {
            count = count / 2;
        } else {
            base = mid + 1;
            count = (count - 1) / 2;
        }
    }

    *index_ptr = base;

    /*
     * base is the expected position of the entry (insertion point).
     * If lowest_match is true, we may have actually found it, and
     * we have to check again here.
     */

    if (lowest_match && base <= soc_mem_index_max(unit, mem)) {
        if (last_read != base &&        /* Optimize out read if possible */
            (rv = soc_mem_read(unit, mem, copyno, base, entry_data)) < 0) {
            soc_cm_debug(DK_ERR,
                         "soc_mem_search: read %s.%s[%d] failed\n",
                         SOC_MEM_UFNAME(unit, mem),
                         SOC_BLOCK_NAME(unit, copyno), base);
            return rv;
        }

        if (soc_mem_compare_key(unit, mem, key_data, entry_data) == 0) {
            return SOC_E_NONE;                  /* Found */
        }
    }

    /* If bitmap conflict found, return SOC_E_CONFIG */
    return SOC_E_NOT_FOUND;       /* No error; not found */
}

/* Special hash mem lookup routine used only in context of generic lookup failure 
   due to parity/ecc error in order to do bank specific lookups or accelerate 
   mem cache indexing by fetching bucket index from h/w */
STATIC int
_soc_mem_bank_lookup(int unit, soc_mem_t mem, int copyno, int32 banks,
                     void *key, void *result, int *index_ptr)
{
    schan_msg_t schan_msg;
    int         rv, entry_dw, allow_intr=0;
    uint8       at;
    int         type, index;
    uint32      *data;

    entry_dw = soc_mem_entry_words(unit, mem);

    schan_msg_clear(&schan_msg);
    schan_msg.gencmd.header.opcode = TABLE_LOOKUP_CMD_MSG;
#ifdef BCM_EXTND_SBUS_SUPPORT
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        schan_msg.gencmd.header.srcblk =
            (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
    } else 
#endif
    {
        schan_msg.gencmd.header.srcblk = SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
    }
    schan_msg.gencmd.header.dstblk = SOC_BLOCK2SCH(unit, copyno);
    if ((soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) ||
        (soc_feature(unit, soc_feature_shared_hash_mem) && ((mem == L2Xm) ||
         (mem == L3_ENTRY_ONLYm) || (mem == L3_ENTRY_IPV4_UNICASTm) || 
         (mem == L3_ENTRY_IPV4_MULTICASTm) || (mem == L3_ENTRY_IPV6_UNICASTm) || 
         (mem == L3_ENTRY_IPV6_MULTICASTm)))) {
        /* don't set bank id in cos */
    } else {
        schan_msg.gencmd.header.cos = banks & 0x3;
    }
    schan_msg.gencmd.header.datalen = entry_dw * 4;

    schan_msg.gencmd.address = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);
    if ((soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) ||
        (soc_feature(unit, soc_feature_shared_hash_mem) && ((mem == L2Xm) ||
         (mem == L3_ENTRY_ONLYm) || (mem == L3_ENTRY_IPV4_UNICASTm) || 
         (mem == L3_ENTRY_IPV4_MULTICASTm) || (mem == L3_ENTRY_IPV6_UNICASTm) || 
         (mem == L3_ENTRY_IPV6_MULTICASTm)))) {
        /* set bank ignore mask in lower 20 bits */
        if (banks && banks != SOC_MEM_HASH_BANK_ALL) {
            if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                schan_msg.gencmd.address |= (~banks & SOC_HASH_BANK_MASK_SHARED);
            } else {
#ifdef BCM_TRIUMPH3_SUPPORT
                uint32 phy_banks = soc_ism_get_phy_bank_mask(unit, banks);
                schan_msg.gencmd.address |= (~phy_banks & SOC_HASH_BANK_MASK_ISM);
#endif /* BCM_TRIUMPH3_SUPPORT */
            }
        }
    } 

    /* Fill in entry data */
    sal_memcpy(schan_msg.gencmd.data, key, entry_dw * 4);

    if (SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /*
     * Execute S-Channel "lookup insert" command packet consisting of
     * (header word + address word + entry_dw), and read back
     * (header word + response word + entry_dw) data words.
     */

    rv = soc_schan_op(unit, &schan_msg, entry_dw + 2, entry_dw + 2, allow_intr);

    /* Check result */

    if (schan_msg.genresp.header.opcode != TABLE_LOOKUP_DONE_MSG) {
        soc_cm_debug(DK_ERR, "soc_mem_bank_lookup: "
                     "invalid S-Channel reply, expected TABLE_LOOKUP_DONE_MSG:\n");
        soc_schan_dump(unit, &schan_msg, entry_dw + 2);
        return SOC_E_INTERNAL;
    }

    if (soc_feature(unit, soc_feature_new_sbus_format) &&
        !soc_feature(unit, soc_feature_new_sbus_old_resp) ) {
        type = schan_msg.genresp_v2.response.type;
        index = schan_msg.genresp_v2.response.index;
        data = schan_msg.genresp_v2.data;
    } else {
        type = schan_msg.genresp.response.type;
        index = schan_msg.genresp.response.index;
        data = schan_msg.genresp.data;
    }
    if ((schan_msg.header.cpu) || (rv == SOC_E_FAIL)) {
        if (type == SCHAN_GEN_RESP_TYPE_NOT_FOUND) {
            rv = SOC_E_NOT_FOUND;
        } else {
            rv = SOC_E_FAIL;
            
            if (!(SOC_IS_TRIUMPH3(unit) || (SOC_IS_TRIDENT2(unit)))) {
                index = schan_msg.dwords[2] & 0xffff;
            }
        }
    } else {
        sal_memcpy(result, data, entry_dw * sizeof(uint32));
    }
    *index_ptr = index;

    if (soc_cm_debug_check(DK_SOCMEM)) {
        soc_cm_print("Bank lookup table[%s]: banks=%d", SOC_MEM_NAME(unit, mem),
                     banks);
        if (soc_cm_debug_check(DK_VERBOSE)) {
            soc_mem_entry_dump(unit, mem, data);
        }
        if (SOC_FAILURE(rv)) {
            if (type == SCHAN_GEN_RESP_TYPE_NOT_FOUND) {
                soc_cm_print(" Not found\n");
            } else {
                soc_cm_print(" Fail, bkt:%d\n", index);
            }
        } else {
            soc_cm_print(" (index=%d)\n", index);
        }
    }

    return rv;
}

#ifdef BCM_TRIDENT2_SUPPORT
STATIC int
_soc_mem_shared_hash_entries_per_bkt(soc_mem_t mem)
{
    switch(mem) {
    case L2Xm:
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
        return 4;
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
        return 2;
    case L3_ENTRY_IPV6_MULTICASTm:
        return 1;
    default:
        return 4;
    }
}
#endif /* BCM_TRIDENT2_SUPPORT */

STATIC int
_soc_mem_hash_entries_per_bkt(int unit, soc_mem_t mem)
{
    if (soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) {
        if (mem == L2_ENTRY_2m || mem == L3_ENTRY_2m || mem == VLAN_XLATE_EXTDm 
            || mem == MPLS_ENTRY_EXTDm) {
            return 2;
        } else if (mem == L3_ENTRY_4m) {
            return 1;
        } else {
            return 4;
        }
    } 
#ifdef BCM_TRIDENT2_SUPPORT
    else {
        return _soc_mem_shared_hash_entries_per_bkt(mem);
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    return 4;
}

STATIC int
_soc_hash_mem_entry_base_get(int unit, soc_mem_t mem, int bank, int base_index,
                             int num_entries)
{
    if (soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) {
        return base_index * num_entries;
    } else if (soc_feature(unit, soc_feature_shared_hash_mem)) {
         if (((mem == L2Xm) ||
             (mem == L3_ENTRY_ONLYm) || (mem == L3_ENTRY_IPV4_UNICASTm) || 
             (mem == L3_ENTRY_IPV4_MULTICASTm) || (mem == L3_ENTRY_IPV6_UNICASTm) || 
             (mem == L3_ENTRY_IPV6_MULTICASTm))) {
            return base_index * num_entries;
         } else {
            return (base_index/num_entries) * num_entries;
         }
    } else {
        num_entries *= 2;
        if (bank) {
            return ((base_index * num_entries ) + 4);
        } else {
            return base_index * num_entries;
        } 
    }    
}

STATIC void
_soc_mem_entry_get_key(int unit, soc_mem_t mem, void *entry, void *key)
{
#ifdef BCM_ISM_SUPPORT
    if (soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) {
        /* Use ism routines */
        (void)soc_gen_key_from_entry(unit, mem, entry, key);
    } else 
#endif /* BCM_ISM_SUPPORT */
    {
        /* Else use key field */
        if (SOC_MEM_FIELD_VALID(unit, mem, KEYf)) {
            uint32 key_buff[SOC_MAX_MEM_FIELD_WORDS];
            soc_mem_field_get(unit, mem, entry, KEYf, key_buff);
            soc_mem_field_set(unit, mem, key, KEYf, key_buff);
        } else {
            soc_cm_debug(DK_ERR, "Unit %d: Unable to retreive key for %s.\n",
                         unit, SOC_MEM_NAME(unit, mem));
        }
    }
}

/* NOTE: index1 is used for legacy dual hash only */
STATIC int
_soc_mem_cache_lookup(int unit, soc_mem_t mem, int copyno, int32 banks,
                      void *key, void *result, int index0, int index1, 
                      int *index_ptr, uint32 *cache, uint8 *vmap)
{
    int i, rc;
    uint32 bmap = 0;
    uint32 *entry_ptr, match_key[SOC_MAX_MEM_WORDS];
    uint32 entry_dw = soc_mem_entry_words(unit, mem);
#ifdef BCM_ISM_SUPPORT
    _soc_ism_mem_banks_t ism_banks;
#endif /* BCM_ISM_SUPPORT */

#ifdef BCM_ISM_SUPPORT
    if (soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) {
        rc = soc_ism_get_banks_for_mem(unit, mem, ism_banks.banks, 
                                       ism_banks.bank_size, &ism_banks.count);
        if (SOC_FAILURE(rc) || ism_banks.count == 0) {
            if (index_ptr) {
                *index_ptr = -1;
            }
            return SOC_E_NOT_FOUND;
        }
        for (i = 0; i < ism_banks.count; i++) {
            bmap |= 1 << ism_banks.banks[i];
        }
        bmap &= banks;
    } else 
#endif /* BCM_ISM_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_shared_hash_mem) && ((mem == L2Xm) ||
         (mem == L3_ENTRY_ONLYm) || (mem == L3_ENTRY_IPV4_UNICASTm) || 
         (mem == L3_ENTRY_IPV4_MULTICASTm) || (mem == L3_ENTRY_IPV6_UNICASTm) || 
         (mem == L3_ENTRY_IPV6_MULTICASTm))) {
        SOC_IF_ERROR_RETURN
            (soc_trident2_hash_bank_bitmap_get(unit, mem, &bmap));
        bmap &= banks;
    } else 
#endif /* BCM_TRIDENT2_SUPPORT */
    { /* Dual hash */
        bmap = 0x3;
        (banks) ? bmap &= banks : bmap;
    }
    /* Issue lookups on all specified & applicable banks */
    if (bmap) {
        int e, entries, entry_base, match, index = -1;
        
        entries = _soc_mem_hash_entries_per_bkt(unit, mem);
        for (i = 0; i < 32; i++) {
            if (bmap & (1 << i)) {
                if (index1 == -1) {
                    rc = _soc_mem_bank_lookup(unit, mem, copyno, 1 << i, key, 
                                              result, &index);
                } else { /* Legacy dual hash: bucket indexes are already known */
                    index = i ? index1 : index0;
                    rc = SOC_E_FAIL;
                }
                if (SOC_FAILURE(rc)) { /* Case where correction has not yet happened */
                    if (rc == SOC_E_NOT_FOUND) {
                        continue;
                    } else if (rc == SOC_E_INTERNAL) {
                        return rc;
                    } else if (rc == SOC_E_FAIL) {
                        /* Check and compare key with all entries in the bucket */
                        soc_cm_debug(DK_VERBOSE, "Check in bucket: %d\n", 
                                     index);
                        entry_base = _soc_hash_mem_entry_base_get(unit, mem, i, 
                                         index, entries);
                        soc_cm_debug(DK_VERBOSE, "Base entry: %d\n", entry_base);
                        for (e = 0; e < entries; e++) {
                            /* Try with only cached entries */
                            if (CACHE_VMAP_TST(vmap, entry_base+e)) {
                                match = 0;
                                entry_ptr = cache + ((entry_base+e) * entry_dw);
                                sal_memset(match_key, 0, sizeof(match_key));
                                /* If match found then return else continue */
                                soc_cm_debug(DK_VERBOSE, "Check cached entry at "
                                             "index: %d\n", entry_base+e);
                                if (soc_feature(unit, soc_feature_ism_memory) && 
                                    soc_mem_is_mview(unit, mem) && !(mem == L2_ENTRY_1m ||
                                    mem == L2_ENTRY_2m)) { /* L2 key compare routine exists for TR3 */
                                    uint32 entry_key[SOC_MAX_MEM_WORDS];
                                    
                                    sal_memset(entry_key, 0, sizeof(entry_key));
                                    _soc_mem_entry_get_key(unit, mem, entry_ptr, match_key);
                                    _soc_mem_entry_get_key(unit, mem, key, entry_key);
                                    if (sal_memcmp(entry_key, match_key, entry_dw * 4)) {
                                        continue;
                                    } else {
                                        match = 1;
                                    }
                                } else if (soc_mem_cmp_fn(unit, mem)) {
                                    if (soc_mem_compare_key(unit, mem, entry_ptr, key)) {
                                        continue;
                                    } else {
                                        match = 1;
                                    }
                                } else {
                                    uint32 entry_key[SOC_MAX_MEM_WORDS];
                                    
                                    sal_memset(entry_key, 0, sizeof(entry_key));
                                    _soc_mem_entry_get_key(unit, mem, entry_ptr, match_key);
                                    _soc_mem_entry_get_key(unit, mem, key, entry_key);
                                    if (sal_memcmp(entry_key, match_key, entry_dw * 4)) {
                                        continue;
                                    } else {
                                        match = 1;
                                    }
                                }
                                if (match) {
                                    if (result) {
                                        sal_memcpy(result, entry_ptr, entry_dw * 4);
                                    }
                                    if (index_ptr) {
                                        *index_ptr = entry_base+e;
                                    }
                                    return SOC_E_NONE;
                                }
                            }
                        }
                    }
                } else { /* Case where correction completes before this routine */
                    if (index_ptr) {
                        *index_ptr = index;
                    }
                    return SOC_E_NONE;
                }
            }
        }
    } 
    if (index_ptr) {
        *index_ptr = -1;
    }
    return SOC_E_NOT_FOUND;
}

/*
 * Function:
 *      soc_mem_generic_lookup
 * Purpose:
 *      Send a lookup message over the S-Channel and receive the response.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      banks - For dual hashing, which halves are selected (inverted)
 *              Note: used to create a bitmap for ISM mems. (All banks: -1)
 *      key - entry to look up
 *      result - entry to receive entire found entry
 *      index_ptr (OUT) - If found, receives table index where found
 * Returns:
 *      SOC_E_INTERNAL if retries exceeded or other internal error
 *      SOC_E_NOT_FOUND if the entry is not found.
 *      SOC_E_NONE (0) on success (entry found):
 */

int
soc_mem_generic_lookup(int unit, soc_mem_t mem, int copyno, int32 banks,
                       void *key, void *result, int *index_ptr)
{
    schan_msg_t schan_msg;
    int         rv, entry_dw, allow_intr=0;
    uint8       at;
    int         type, index, index_valid;
    uint32      *data;

    _SOC_MEM_REPLACE_MEM(unit, mem);
    assert(SOC_MEM_IS_VALID(unit, mem));
    assert(soc_attached(unit));

#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_generic_lookup: not supported on %s\n",
                     SOC_CHIP_STRING(unit));
        return SOC_E_UNAVAIL;
    }
#endif

    if (copyno == MEM_BLOCK_ANY) {
        /* coverity[overrun-local : FALSE] */
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }
    if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_generic_lookup: invalid block %d for memory %s\n",
                     copyno, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    entry_dw = soc_mem_entry_words(unit, mem);

    schan_msg_clear(&schan_msg);
    schan_msg.gencmd.header.opcode = TABLE_LOOKUP_CMD_MSG;
#ifdef BCM_EXTND_SBUS_SUPPORT
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        schan_msg.gencmd.header.srcblk =
            (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
    } else 
#endif
    {
        schan_msg.gencmd.header.srcblk = SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
    }
    schan_msg.gencmd.header.dstblk = SOC_BLOCK2SCH(unit, copyno);
    if ((soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) ||
        (soc_feature(unit, soc_feature_shared_hash_mem) && ((mem == L2Xm) ||
         (mem == L3_ENTRY_ONLYm) || (mem == L3_ENTRY_IPV4_UNICASTm) || 
         (mem == L3_ENTRY_IPV4_MULTICASTm) || (mem == L3_ENTRY_IPV6_UNICASTm) || 
         (mem == L3_ENTRY_IPV6_MULTICASTm)))) {
        /* don't set bank id in cos */
    } else {
        schan_msg.gencmd.header.cos = banks & 0x3;
    }
    schan_msg.gencmd.header.datalen = entry_dw * 4;

    schan_msg.gencmd.address = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);
    if ((soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) ||
        (soc_feature(unit, soc_feature_shared_hash_mem) && ((mem == L2Xm) ||
         (mem == L3_ENTRY_ONLYm) || (mem == L3_ENTRY_IPV4_UNICASTm) || 
         (mem == L3_ENTRY_IPV4_MULTICASTm) || (mem == L3_ENTRY_IPV6_UNICASTm) || 
         (mem == L3_ENTRY_IPV6_MULTICASTm)))) {
        /* set bank ignore mask in lower 20 bits */
        if (banks && banks != SOC_MEM_HASH_BANK_ALL) {
            if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                schan_msg.gencmd.address |= (~banks & SOC_HASH_BANK_MASK_SHARED);
            } else {
#ifdef BCM_TRIUMPH3_SUPPORT
                uint32 phy_banks = soc_ism_get_phy_bank_mask(unit, banks);
                schan_msg.gencmd.address |= (~phy_banks & SOC_HASH_BANK_MASK_ISM);
#endif /* BCM_TRIUMPH3_SUPPORT */
            }
        }
    } 

    /* Fill in entry data */
    sal_memcpy(schan_msg.gencmd.data, key, entry_dw * 4);

    if (SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /*
     * Execute S-Channel "lookup insert" command packet consisting of
     * (header word + address word + entry_dw), and read back
     * (header word + response word + entry_dw) data words.
     */

    rv = soc_schan_op(unit, &schan_msg, entry_dw + 2, entry_dw + 2, allow_intr);

    /* Check result */

    if (schan_msg.genresp.header.opcode != TABLE_LOOKUP_DONE_MSG) {
        soc_cm_debug(DK_ERR, "soc_mem_generic_lookup: "
                     "invalid S-Channel reply, expected TABLE_LOOKUP_DONE_MSG:\n");
        soc_schan_dump(unit, &schan_msg, entry_dw + 2);
        return SOC_E_INTERNAL;
    }

    if (soc_feature(unit, soc_feature_new_sbus_format) &&
        !soc_feature(unit, soc_feature_new_sbus_old_resp)) {
        type = schan_msg.genresp_v2.response.type;
        index = schan_msg.genresp_v2.response.index;
        data = schan_msg.genresp_v2.data;
    } else {
        type = schan_msg.genresp.response.type;
        index = schan_msg.genresp.response.index;
        data = schan_msg.genresp.data;
    }

    if ((schan_msg.header.cpu) || (rv == SOC_E_FAIL)) {
        if (index_ptr) {
            *index_ptr = -1;
        }
        if (type == SCHAN_GEN_RESP_TYPE_NOT_FOUND) {
            rv = SOC_E_NOT_FOUND;
#if defined(BCM_TRIDENT2_SUPPORT)
            if (SOC_IS_TD2_TT2(unit)) {
                if (mem == L3_ENTRY_IPV4_MULTICASTm || mem == L3_ENTRY_IPV6_UNICASTm) {
                    if (index_ptr) {
                        *index_ptr = schan_msg.genresp.response.index;
                    }
                }
            }
#endif /* BCM_TRIDENT2_SUPPORT */
        } else {
            if (SOC_SER_CORRECTION_SUPPORT(unit)) {
                uint32 *cache;
                uint8 *vmap;
                
                cache = SOC_MEM_STATE(unit, mem).cache[copyno];
                vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];
                if (cache != NULL && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
                    int index0 = index, index1 = -1;
                    if (SOC_IS_TRIDENT(unit) || SOC_IS_TRIUMPH2(unit)) { 
                        
                        index0 = schan_msg.dwords[2] & 0xffff;
                        index1 = schan_msg.dwords[2] >> 16;
                    }
                    rv = _soc_mem_cache_lookup(unit, mem, copyno, banks, key, 
                                               result, index0, index1,
                                               index_ptr, cache, vmap);
                }
            } else {
                rv = SOC_E_FAIL;
            }
        }
    } else {
        if (result != NULL) {
            sal_memcpy(result, data, entry_dw * sizeof(uint32));
        }
#ifdef BCM_TRIUMPH_SUPPORT
        if (soc_feature(unit, soc_feature_etu_support) &&
            mem == EXT_L2_ENTRY_2m) {
            soc_mem_t ext_mem;
            int ext_index;
            
            SOC_IF_ERROR_RETURN(soc_tcam_raw_index_to_mem_index(unit, index, 
                                &ext_mem, &ext_index));
            if (mem != ext_mem) {
                return SOC_E_INTERNAL;
            }
            index = ext_index;
        }
#endif /* BCM_TRIUMPH_SUPPORT */
        if (index_ptr != NULL) {
            *index_ptr = index;
        }
        index_valid = (index >= 0 &&
                           index <= soc_mem_index_max(unit, mem));
        if (!index_valid) {
            soc_cm_debug(DK_ERR, "soc_mem_generic_lookup: "
                         "invalid index %d for memory %s\n",
                         index, SOC_MEM_NAME(unit, mem));
            rv = SOC_E_INTERNAL;
        }
    }
    if (soc_cm_debug_check(DK_SOCMEM)) {
        soc_cm_print("Lookup table[%s]: banks=%d", SOC_MEM_NAME(unit, mem),
                     banks);
        if (soc_cm_debug_check(DK_VERBOSE)) {
            soc_mem_entry_dump(unit, mem, data);
        }
        if (SOC_FAILURE(rv)) {
            if (type == SCHAN_GEN_RESP_TYPE_NOT_FOUND) {
                soc_cm_print(" Not found\n");
            } else {
                soc_cm_print(" Fail\n");
            }
        } else {
            soc_cm_print(" (index=%d)\n", index);
        }
    }

    return rv;
}

#define _SOC_ALPM_MEM_OP_RETRY_TO          1000000
#define _SOC_ALPM_MEM_OP_RETRY_COUNT       5
/*
 * Function:
 *      soc_mem_alpm_lookup
 * Purpose:
 *      Send a lookup message over the S-Channel and receive the response
 *      from ALPM memories. ALPM memories have special needs bcoz the entries
 *      are not hashed.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      banks - Banks
 *              Note: used to create a bitmap for ISM mems. (All banks: -1)
 *      key - entry to look up
 *      result - entry to receive entire found entry
 *      index_ptr (OUT) - If found, receives table index where found
 * Returns:
 *      SOC_E_INTERNAL if retries exceeded or other internal error
 *      SOC_E_NOT_FOUND if the entry is not found.
 *      SOC_E_NONE (0) on success (entry found):
 */

int
soc_mem_alpm_lookup(int unit, soc_mem_t mem, int bucket_index,
                    int copyno, uint32 banks_disable,
                    void *key, void *result, int *index_ptr)
{
    schan_msg_t schan_msg;
    uint8       at;
    uint32      *data;
    int         rv, entry_dw, allow_intr=0;
    int         type, index;
    int         _soc_alpm_lookup_retry = 0;

    assert(SOC_MEM_IS_VALID(unit, mem));
    assert(soc_attached(unit));

    /* Only applicable to ALPM memories */
    if ((mem != L3_DEFIP_ALPM_IPV6_128m) &&
        (mem != L3_DEFIP_ALPM_IPV6_64m) &&
        (mem != L3_DEFIP_ALPM_IPV6_64_1m) &&
        (mem != L3_DEFIP_ALPM_IPV4m) &&
        (mem != L3_DEFIP_ALPM_IPV4_1m)) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_alpm_lookup: not supported non-ALPM memories %s\n",
                     SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_generic_lookup: not supported on %s\n",
                     SOC_CHIP_STRING(unit));
        return SOC_E_UNAVAIL;
    }
#endif

    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }
    if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_alpm_lookup: invalid block %d for memory %s\n",
                     copyno, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    entry_dw = soc_mem_entry_words(unit, mem);

    SOC_CONTROL_LOCK(unit);
    SHR_BITSET(&(SOC_CONTROL(unit)->alpm_mem_ops), _SOC_ALPM_LOOKUP);
    SOC_CONTROL_UNLOCK(unit);
_retry:
    schan_msg_clear(&schan_msg);
    schan_msg.gencmd.header.opcode = TABLE_LOOKUP_CMD_MSG;
    schan_msg.gencmd.header.srcblk =
        (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
    schan_msg.gencmd.header.dstblk = SOC_BLOCK2SCH(unit, copyno);
    schan_msg.gencmd.header.cos = banks_disable & 0x3;
    schan_msg.gencmd.header.datalen = entry_dw * 4;
    /* ALPM entries are not hashed */
    schan_msg.gencmd.address = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);
    /* Address contains bank disable, and bucket pointer */
    /* Bucket index and bank disable 
     * addr[13:0]   - bucket 
     * addr[17:14]  - bank disable[3:0]
     */
    schan_msg.gencmd.address &= ~(0x0003ffff);
    schan_msg.gencmd.address |= 
             (((banks_disable & 0xf) << 14) | (bucket_index & 0x3fff));

    /* Fill in entry data */
    sal_memcpy(schan_msg.gencmd.data, key, entry_dw * 4);

    if (SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /*
     * Execute S-Channel "lookup insert" command packet consisting of
     * (header word + address word + entry_dw), and read back
     * (header word + response word + entry_dw) data words.
     */
    rv = soc_schan_op(unit, &schan_msg, entry_dw + 2, entry_dw + 2, allow_intr);

    /* Check result */

    if (schan_msg.genresp.header.opcode != TABLE_LOOKUP_DONE_MSG) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_generic_lookup: "
                     "invalid S-Channel reply, expected TABLE_LOOKUP_DONE_MSG:\n");
        soc_schan_dump(unit, &schan_msg, entry_dw + 2);
        return SOC_E_INTERNAL;
    }

    type = schan_msg.genresp_v2.response.type;
    index = schan_msg.genresp_v2.response.index;
    data = schan_msg.genresp_v2.data;
    if ((schan_msg.header.cpu) || (rv == SOC_E_FAIL)) {
        if (index_ptr) {
            *index_ptr = -1;
        }
        if (type == SCHAN_GEN_RESP_TYPE_NOT_FOUND) {
            rv = SOC_E_NOT_FOUND;
        } else {
            soc_cm_debug(DK_WARN, "unit %d: ALPM lookup operation encountered "
                         "parity error !!\n", unit);
            _soc_alpm_lookup_retry++;
            if (SOC_CONTROL(unit)->alpm_lookup_retry) {
                sal_sem_take(SOC_CONTROL(unit)->alpm_lookup_retry, _SOC_ALPM_MEM_OP_RETRY_TO);
            }
            if (_soc_alpm_lookup_retry < _SOC_ALPM_MEM_OP_RETRY_COUNT) {
                soc_cm_debug(DK_WARN, "unit %d: Retry ALPM lookup operation..\n", unit);
                goto _retry;
            } else {
                soc_cm_debug(DK_ERR, "unit %d: Aborting ALPM lookup operation "
                             "due to un-correctable error !!\n", unit);
            }
            rv = SOC_E_FAIL;
        }
    } else {
        if (result != NULL) {
            sal_memcpy(result, data, entry_dw * sizeof(uint32));
        }
        if (index_ptr != NULL) {
            *index_ptr = index;
        }
    }
    SOC_CONTROL_LOCK(unit);
    SHR_BITCLR(&(SOC_CONTROL(unit)->alpm_mem_ops), _SOC_ALPM_LOOKUP);
    SOC_CONTROL_UNLOCK(unit);

    if (soc_cm_debug_check(DK_SOCMEM)) {
        soc_cm_print("Lookup table[%s]: banks=%d", SOC_MEM_NAME(unit, mem),
                     banks_disable);
        if (soc_cm_debug_check(DK_VERBOSE)) {
            soc_mem_entry_dump(unit, mem, data);
        }
        if (SOC_FAILURE(rv)) {
            if (type == SCHAN_GEN_RESP_TYPE_NOT_FOUND) {
                soc_cm_print(" Not found\n");
            } else {
                soc_cm_print(" Fail\n");
            }
        } else {
            soc_cm_print(" (index=%d)\n", index);
        }
    }

    return rv;
}

/*
 * Function:
 *      soc_mem_alpm_insert
 * Purpose:
 *      Insert an entry into ALPM memory.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      banks - For dual hashing, which halves are selected (inverted)
 *              Note: used to create a bitmap for ISM mems. (All banks: -1)
 *      entry - entry to insert
 *      old_entry - old entry if existing entry was replaced
 * Returns:
 *      SOC_E_NONE - success
 *      SOC_E_EXISTS - existing entry was replaced
 *      SOC_E_FULL - full
 *      SOC_E_BUSY - modfifo full
 * Notes:
 *      Uses hardware insertion; sends an INSERT message over the
 *      S-Channel.
 */

int
soc_mem_alpm_insert(int unit, soc_mem_t mem, int bucket,
                    int copyno, int32 banks_disable,
                    void *entry, void *old_entry, int *index_ptr)
{
    schan_msg_t schan_msg;
    uint8       at, *vmap;
    uint32      *data, *cache;
    int         rv, entry_dw, index_valid;
    int         type, err_info, index, allow_intr=0;
    int         _soc_alpm_insert_retry = 0;

    assert(SOC_MEM_IS_VALID(unit, mem));
    assert(soc_attached(unit));

    /* Only applicable to ALPM memories */
    if ((mem != L3_DEFIP_ALPM_IPV6_128m) &&
        (mem != L3_DEFIP_ALPM_IPV6_64m) &&
        (mem != L3_DEFIP_ALPM_IPV6_64_1m) &&
        (mem != L3_DEFIP_ALPM_IPV4m) &&
        (mem != L3_DEFIP_ALPM_IPV4_1m)) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_alpm_insert: not supported non-ALPM memories %s\n",
                     SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }
    if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_alpm_insert: invalid block %d for memory %s\n",
                     copyno, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    if (soc_cm_debug_check(DK_SOCMEM)) {
        soc_cm_print("Insert table[%s]: banks=%d", SOC_MEM_NAME(unit, mem),
                     banks_disable);
        if (soc_cm_debug_check(DK_VERBOSE)) {
            soc_mem_entry_dump(unit, mem, entry);
        }
        soc_cm_print("\n");
    }

    entry_dw = soc_mem_entry_words(unit, mem);
    
    SOC_CONTROL_LOCK(unit);
    SHR_BITSET(&(SOC_CONTROL(unit)->alpm_mem_ops), _SOC_ALPM_INSERT);
    SOC_CONTROL_UNLOCK(unit);
_retry:
    schan_msg_clear(&schan_msg);
    schan_msg.gencmd.header.opcode = TABLE_INSERT_CMD_MSG;
    schan_msg.gencmd.header.srcblk =
        (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
    schan_msg.gencmd.header.dstblk = SOC_BLOCK2SCH(unit, copyno);
    schan_msg.gencmd.header.datalen = entry_dw * 4;
    schan_msg.gencmd.address = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);
    /* Address contains bank disable, and bucket pointer */
    /* Bucket index and bank disable 
     * addr[13:0]   - bucket 
     * addr[17:14]  - bank disable[3:0]
     */
    schan_msg.gencmd.address &= ~(0x0003ffff);
    schan_msg.gencmd.address |= 
        (((banks_disable & 0xf) << 14) | (bucket & 0x3fff));

    /* Fill in packet data */
    sal_memcpy(schan_msg.gencmd.data, entry, entry_dw * 4);

    /*
     * Execute S-Channel "table insert" command packet consisting of
     * (header word + address word + entry_dw), and read back
     * (header word + response word + entry_dw) data words.
     */

    rv = soc_schan_op(unit, &schan_msg, entry_dw + 2, entry_dw + 2, allow_intr);

    if (schan_msg.gencmd.header.opcode != TABLE_INSERT_DONE_MSG) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_alpm_insert: "
                     "invalid S-Channel reply, expected TABLE_INSERT_DONE_MSG:\n");
        soc_schan_dump(unit, &schan_msg, 1);
        return SOC_E_INTERNAL;
    }

    type = schan_msg.genresp_v2.response.type;
    err_info = schan_msg.genresp_v2.response.err_info;
    index = schan_msg.genresp_v2.response.index;
    data = schan_msg.genresp_v2.data;

    if ((schan_msg.header.cpu) || (rv == SOC_E_FAIL)) {
        if (index_ptr) {
            *index_ptr = -1;
        }
        if (type == SCHAN_GEN_RESP_TYPE_FULL) {
            soc_cm_debug(DK_SOCMEM, "Insert table[%s]: hash bucket full\n",
                         SOC_MEM_NAME(unit, mem));
            rv = SOC_E_FULL;
        } else if (type == SCHAN_GEN_RESP_TYPE_ERROR) {
            if (err_info == SCHAN_GEN_RESP_ERROR_BUSY) {
                soc_cm_debug(DK_SOCMEM, "Insert table[%s]: Modfifo full\n",
                             SOC_MEM_NAME(unit, mem));
                rv = SOC_E_BUSY;
            } else if (err_info == SCHAN_GEN_RESP_ERROR_PARITY) {
                soc_cm_debug(DK_ERR,
                             "unit %d ALPM insert table[%s]: Parity Error Index %d\n",
                             unit, SOC_MEM_NAME(unit, mem), index);
                _soc_alpm_insert_retry++;
                if (SOC_CONTROL(unit)->alpm_insert_retry) {
                    sal_sem_take(SOC_CONTROL(unit)->alpm_insert_retry, _SOC_ALPM_MEM_OP_RETRY_TO);
                }
                if (_soc_alpm_insert_retry < _SOC_ALPM_MEM_OP_RETRY_COUNT) {
                    soc_cm_debug(DK_WARN, "unit %d: Retry ALPM insert operation..\n", unit);
                    goto _retry;
                } else {
                    soc_cm_debug(DK_ERR, "unit %d: Aborting ALPM insert operation "
                                 "due to un-correctable error !!\n", unit);
                }
                rv = SOC_E_INTERNAL;
            }
        } else {
            rv = SOC_E_FAIL;
        }
    } else {
        if (index_ptr != NULL) {
            *index_ptr = index;
        }
        if (type == SCHAN_GEN_RESP_TYPE_REPLACED) {
            if (old_entry != NULL) {
                sal_memcpy(old_entry, data, entry_dw * sizeof(uint32));
            }
            rv = SOC_E_EXISTS;
        }
        /* Update cache if active */
        cache = SOC_MEM_STATE(unit, mem).cache[copyno];
        vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];
        index_valid = (index >= 0 &&
                       index <= soc_mem_index_max(unit, mem));
        if (index_valid && cache != NULL && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
            sal_memcpy(cache + index * entry_dw, entry, entry_dw * 4);
            CACHE_VMAP_SET(vmap, index);
        }
    }
    SOC_CONTROL_LOCK(unit);
    SHR_BITCLR(&(SOC_CONTROL(unit)->alpm_mem_ops), _SOC_ALPM_INSERT);
    SOC_CONTROL_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      soc_mem_alpm_delete
 * Purpose:
 *      Delete an ALPM entry 
 * Parameters:
 *      unit - StrataSwitch unit #
 *      banks - For dual hashing, which halves are selected (inverted)
 *              Note: used to create a bitmap for ISM mems. (All banks: -1)
 *      entry - entry to delete
 *      old_entry - old entry if entry was found and deleted
 * Returns:
 *      SOC_E_NONE - success
 *      SOC_E_NOT_FOUND - full
 *      SOC_E_BUSY - modfifo full
 * Notes:
 *      Uses hardware deletion; sends an DELETE message over the
 *      S-Channel.
 */

int
soc_mem_alpm_delete(int unit, soc_mem_t mem, int bucket, 
                    int copyno, int32 banks_disable,
                    void *entry, void *old_entry, int *index_ptr)
{
    schan_msg_t schan_msg;
    uint8       at, *vmap;
    uint32      *data, *cache;
    int         rv, entry_dw, index_valid;
    int         type, err_info, index, allow_intr=0;
    int         _soc_alpm_delete_retry = 0;
    void        *null_entry = soc_mem_entry_null(unit, mem);

    assert(SOC_MEM_IS_VALID(unit, mem));
    assert(soc_attached(unit));

    /* Only applicable to ALPM memories */
    if ((mem != L3_DEFIP_ALPM_IPV6_128m) &&
        (mem != L3_DEFIP_ALPM_IPV6_64m) &&
        (mem != L3_DEFIP_ALPM_IPV6_64_1m) &&
        (mem != L3_DEFIP_ALPM_IPV4m) &&
        (mem != L3_DEFIP_ALPM_IPV4_1m)) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_alpm_insert: not supported non-ALPM memories %s\n",
                     SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }
#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_generic_delete: not supported on %s\n",
                     SOC_CHIP_STRING(unit));
        return SOC_E_UNAVAIL;
    }
#endif

    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }
    if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_generic_delete: invalid block %d for memory %s\n",
                     copyno, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    if (soc_cm_debug_check(DK_SOCMEM)) {
        soc_cm_print("Delete table[%s]: banks=%d", SOC_MEM_NAME(unit, mem),
                     banks_disable);
        if (soc_cm_debug_check(DK_VERBOSE)) {
            soc_mem_entry_dump(unit, mem, entry);
        }
        soc_cm_print("\n");
    }

    entry_dw = soc_mem_entry_words(unit, mem);
    
    SOC_CONTROL_LOCK(unit);
    SHR_BITSET(&(SOC_CONTROL(unit)->alpm_mem_ops), _SOC_ALPM_DELETE);
    SOC_CONTROL_UNLOCK(unit);
_retry:
    schan_msg_clear(&schan_msg);
    schan_msg.gencmd.header.opcode = TABLE_DELETE_CMD_MSG;
    schan_msg.gencmd.header.srcblk =
        (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
    schan_msg.gencmd.header.dstblk = SOC_BLOCK2SCH(unit, copyno);
    schan_msg.gencmd.header.cos = banks_disable & 0x3;
    schan_msg.gencmd.header.datalen = entry_dw * 4;
    schan_msg.gencmd.address = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);

    /* Address contains bank disable, and bucket pointer */
    /* Bucket index and bank disable 
     * addr[13:0]   - bucket 
     * addr[17:14]  - bank disable[3:0]
     */
    schan_msg.gencmd.address &= ~(0x0003ffff);
    schan_msg.gencmd.address |= 
        (((banks_disable & 0xf) << 14) | (bucket & 0x3fff));

    /* Fill in packet data */
    sal_memcpy(schan_msg.gencmd.data, entry, entry_dw * 4);

    if (SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /*
     * Execute S-Channel "table delete" command packet consisting of
     * (header word + address word + entry_dw), and read back
     * (header word + response word + entry_dw) data words.
     */
    rv = soc_schan_op(unit, &schan_msg, entry_dw + 2, entry_dw + 2, allow_intr);

    if (schan_msg.readresp.header.opcode != TABLE_DELETE_DONE_MSG) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_generic_delete: "
                     "invalid S-Channel reply, expected TABLE_DELETE_DONE_MSG:\n");
        soc_schan_dump(unit, &schan_msg, 1);
        return SOC_E_INTERNAL;
    }

    type = schan_msg.genresp_v2.response.type;
    err_info = schan_msg.genresp_v2.response.err_info;
    index = schan_msg.genresp_v2.response.index;
    data = schan_msg.genresp_v2.data;
    
    if ((schan_msg.header.cpu) || (rv == SOC_E_FAIL)) {
        if (index_ptr) {
            *index_ptr = -1;
        }
        if (type == SCHAN_GEN_RESP_TYPE_NOT_FOUND) {
            soc_cm_debug(DK_SOCMEM, "Delete table[%s]: Not found\n",
                         SOC_MEM_NAME(unit, mem));
            rv = SOC_E_NOT_FOUND;
        } else if (type == SCHAN_GEN_RESP_TYPE_ERROR) {
            if (err_info == SCHAN_GEN_RESP_ERROR_BUSY) {
                soc_cm_debug(DK_SOCMEM, "Delete table[%s]: Modfifo full\n",
                             SOC_MEM_NAME(unit, mem));
                rv = SOC_E_BUSY;
            } else if (err_info == SCHAN_GEN_RESP_ERROR_PARITY) {
                soc_cm_debug(DK_ERR,
                             "ALPM delete table[%s]: Parity Error Index %d\n",
                             SOC_MEM_NAME(unit, mem), index);
                _soc_alpm_delete_retry++;
                if (SOC_CONTROL(unit)->alpm_insert_retry) {
                    sal_sem_take(SOC_CONTROL(unit)->alpm_insert_retry, 
                                 _SOC_ALPM_MEM_OP_RETRY_TO);
                }
                if (_soc_alpm_delete_retry < _SOC_ALPM_MEM_OP_RETRY_COUNT) {
                    soc_cm_debug(DK_WARN, "unit %d: Retry ALPM delete operation..\n", 
                                 unit);
                    goto _retry;
                } else {
                    soc_cm_debug(DK_ERR, "unit %d: Aborting ALPM delete operation "
                                 "due to un-correctable error !!\n", unit);
                }
                rv = SOC_E_INTERNAL;
            }
        } else {
            rv = SOC_E_FAIL;
        }
    } else {
        if (old_entry != NULL) {
            sal_memcpy(old_entry, data, entry_dw * sizeof(uint32));
        }
        if (index_ptr != NULL) {
            *index_ptr = index;
        }
#if defined(BCM_TRIUMPH_SUPPORT)
        if (mem == EXT_L2_ENTRYm) {
            SOC_IF_ERROR_RETURN
                (soc_triumph_ext_l2_entry_update(unit, index, NULL));
        }
#endif /* BCM_TRIUMPH_SUPPORT */
        /* Update cache if active */
        cache = SOC_MEM_STATE(unit, mem).cache[copyno];
        vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];
        index_valid = (index >= 0 &&
                       index <= soc_mem_index_max(unit, mem));
     
        if (index_valid && cache != NULL && CACHE_VMAP_TST(vmap, index)
            && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
            sal_memcpy(cache + index * entry_dw, null_entry, entry_dw * 4);
            CACHE_VMAP_SET(vmap, index);
        }
    }
    SOC_CONTROL_LOCK(unit);
    SHR_BITCLR(&(SOC_CONTROL(unit)->alpm_mem_ops), _SOC_ALPM_DELETE);
    SOC_CONTROL_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      soc_mem_generic_insert
 * Purpose:
 *      Insert an entry
 * Parameters:
 *      unit - StrataSwitch unit #
 *      banks - For dual hashing, which halves are selected (inverted)
 *              Note: used to create a bitmap for ISM mems. (All banks: -1)
 *      entry - entry to insert
 *      old_entry - old entry if existing entry was replaced
 * Returns:
 *      SOC_E_NONE - success
 *      SOC_E_EXISTS - existing entry was replaced
 *      SOC_E_FULL - full
 *      SOC_E_BUSY - modfifo full
 * Notes:
 *      Uses hardware insertion; sends an INSERT message over the
 *      S-Channel.
 */

int
soc_mem_generic_insert(int unit, soc_mem_t mem, int copyno, int32 banks,
                       void *entry, void *old_entry, int *index_ptr)
{
    schan_msg_t schan_msg;
#ifdef BCM_ESW_SUPPORT
    schan_msg_t schan_msg_cpy;
#endif /* BCM_ESW_SUPPORT */
    int         rv, index_valid;
    int         entry_dw = 0; 
    uint8       at, *vmap;
    int         type, err_info, index, allow_intr=0;
    uint32      *data, *cache;

    _SOC_MEM_REPLACE_MEM(unit, mem);
    assert(SOC_MEM_IS_VALID(unit, mem));
    assert(soc_attached(unit));

#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_generic_insert: not supported on %s\n",
                     SOC_CHIP_STRING(unit));
        return SOC_E_UNAVAIL;
    }
#endif

    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }
    if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_generic_insert: invalid block %d for memory %s\n",
                     copyno, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    if (soc_cm_debug_check(DK_SOCMEM)) {
        soc_cm_print("Insert table[%s]: banks=%d", SOC_MEM_NAME(unit, mem),
                     banks);
        if (soc_cm_debug_check(DK_VERBOSE)) {
            soc_mem_entry_dump(unit, mem, entry);
        }
        soc_cm_print("\n");
    }

    entry_dw = soc_mem_entry_words(unit, mem);

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit, soc_feature_shared_hash_ins) && 
        (mem == L3_ENTRY_IPV4_MULTICASTm || mem == L3_ENTRY_IPV6_UNICASTm)) {
        int ok = 0, rv1, index, bix;
        int num_banks, bank_ids[7];

        /* Retreive applicable bank map and try on each */
        SOC_IF_ERROR_RETURN
            (soc_trident2_hash_bank_count_get(unit, mem, &num_banks));
        for (bix = 0; bix < num_banks; bix++) {
            SOC_IF_ERROR_RETURN
                (soc_trident2_hash_bank_number_get(unit, mem, bix,
                                                   &bank_ids[bix]));
            rv1 = soc_mem_generic_lookup(unit, mem, copyno, 1 << bank_ids[bix],
                                         entry, NULL, &index);
            soc_cm_debug(DK_VERBOSE+DK_SOCMEM, "Sh-Ins-Lukp: bank[%d] rv[%d] index[%d]\n",
                         bank_ids[bix], rv1, index);
            if (rv1 == SOC_E_NOT_FOUND) {
                uint32 bkt[SOC_MAX_MEM_WORDS];
            
                rv1 = soc_mem_read(unit, L3_ENTRY_IPV6_MULTICASTm, copyno,
                                   index, bkt);
                if (SOC_FAILURE(rv1)) {
                    return rv1;
                } else {
                    uint32 v0, v1;
            
                    soc_mem_field_get(unit, L3_ENTRY_IPV6_MULTICASTm, bkt,
                                      VALID_0f, &v0);
                    soc_mem_field_get(unit, L3_ENTRY_IPV6_MULTICASTm, bkt,
                                      VALID_1f, &v1);
                    if ((v0 == 0) && (v1 == 0)) {
                        index *= 2;
                        rv1 = soc_mem_write(unit, mem, copyno, index, entry);
                        if (SOC_FAILURE(rv1)) {
                            return rv1;
                        }
                        if (index_ptr) {
                            *index_ptr = index;
                        }
                        ok = 1; rv = SOC_E_NONE;
                        break;
                    } else {
                        soc_mem_field_get(unit, L3_ENTRY_IPV6_MULTICASTm, bkt,
                                          VALID_2f, &v0);
                        soc_mem_field_get(unit, L3_ENTRY_IPV6_MULTICASTm, bkt,
                                          VALID_3f, &v1);
                        if ((v0 == 0) && (v1 == 0)) {
                            index = index*2 + 1;
                            rv1 = soc_mem_write(unit, mem, copyno, index, entry);
                            if (SOC_FAILURE(rv1)) {
                                return rv1;
                            }
                            if (index_ptr) {
                                *index_ptr = index;
                            }
                            ok = 1; rv = SOC_E_NONE;
                            break;
                        }
                    }
                }
            } else {
                if (old_entry) {
                    rv1 = soc_mem_read(unit, mem, copyno, index, old_entry);
                    if (SOC_FAILURE(rv1)) {
                        return rv1;
                    }
                }
                rv1 = soc_mem_write(unit, mem, copyno, index, entry);
                if (SOC_FAILURE(rv1)) {
                    return rv1;
                }
                if (index_ptr) {
                    *index_ptr = index;
                }
                ok = 1; rv = SOC_E_EXISTS;
                break;
            }
        }
        if (!ok) {
            return SOC_E_FULL;
        }
        /* Update cache if active */
        cache = SOC_MEM_STATE(unit, mem).cache[copyno];
        vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];
        index_valid = (index >= 0 &&
                       index <= soc_mem_index_max(unit, mem));
        if (!index_valid) {
            soc_cm_debug(DK_ERR, "soc_mem_generic_insert: "
                         "invalid index %d for memory %s\n",
                         index, SOC_MEM_NAME(unit, mem));
            rv = SOC_E_INTERNAL;
        } else if (cache != NULL && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
            sal_memcpy(cache + index * entry_dw, entry, entry_dw * 4);
            CACHE_VMAP_SET(vmap, index);
        }
        return rv;
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    schan_msg_clear(&schan_msg);
    schan_msg.gencmd.header.opcode = TABLE_INSERT_CMD_MSG;
#ifdef BCM_EXTND_SBUS_SUPPORT
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        schan_msg.gencmd.header.srcblk =
            (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
    } else 
#endif
    {
        schan_msg.gencmd.header.srcblk = SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
    }
    schan_msg.gencmd.header.dstblk = SOC_BLOCK2SCH(unit, copyno);
    if ((soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) ||
        (soc_feature(unit, soc_feature_shared_hash_mem) && ((mem == L2Xm) ||
         (mem == L3_ENTRY_ONLYm) || (mem == L3_ENTRY_IPV4_UNICASTm) || 
         (mem == L3_ENTRY_IPV4_MULTICASTm) || (mem == L3_ENTRY_IPV6_UNICASTm) || 
         (mem == L3_ENTRY_IPV6_MULTICASTm)))) {
        /* don't set bank id in cos */
    } else {
        schan_msg.gencmd.header.cos = banks & 0x3;
    }
    schan_msg.gencmd.header.datalen = entry_dw * 4;

    schan_msg.gencmd.address = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);
    if ((soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) ||
        (soc_feature(unit, soc_feature_shared_hash_mem) && ((mem == L2Xm) ||
         (mem == L3_ENTRY_ONLYm) || (mem == L3_ENTRY_IPV4_UNICASTm) || 
         (mem == L3_ENTRY_IPV4_MULTICASTm) || (mem == L3_ENTRY_IPV6_UNICASTm) || 
         (mem == L3_ENTRY_IPV6_MULTICASTm)))) {
        /* set bank ignore mask in lower 20 bits */
        if (banks && banks != SOC_MEM_HASH_BANK_ALL) {
            if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                schan_msg.gencmd.address |= (~banks & SOC_HASH_BANK_MASK_SHARED);
            } else {
#ifdef BCM_TRIUMPH3_SUPPORT
                uint32 phy_banks = soc_ism_get_phy_bank_mask(unit, banks);
                schan_msg.gencmd.address |= (~phy_banks & SOC_HASH_BANK_MASK_ISM);
#endif /* BCM_TRIUMPH3_SUPPORT */
            }
        }
    } 

    /* Fill in packet data */
    sal_memcpy(schan_msg.gencmd.data, entry, entry_dw * 4);

    if (SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /*
     * Execute S-Channel "table insert" command packet consisting of
     * (header word + address word + entry_dw), and read back
     * (header word + response word + entry_dw) data words.
     */
    if (2 + entry_dw > CMIC_SCHAN_WORDS(unit)) {
       soc_cm_debug(DK_WARN,
                    "soc_mem_generic_insert: assert will fail for memory %s\n",
                    SOC_MEM_NAME(unit, mem));
    }
#ifdef BCM_ESW_SUPPORT
    if (soc_feature(unit, soc_feature_shared_hash_mem)) {
        sal_memcpy(&schan_msg_cpy, &schan_msg, sizeof(schan_msg));
    }
_retry_op:
#endif /* BCM_ESW_SUPPORT */
    rv = soc_schan_op(unit, &schan_msg, entry_dw + 2, entry_dw + 2, allow_intr);

    if (schan_msg.gencmd.header.opcode != TABLE_INSERT_DONE_MSG) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_generic_insert: "
                     "invalid S-Channel reply, expected TABLE_INSERT_DONE_MSG:\n");
        soc_schan_dump(unit, &schan_msg, 1);
        return SOC_E_INTERNAL;
    }

    if (soc_feature(unit, soc_feature_new_sbus_format) &&
        !soc_feature(unit, soc_feature_new_sbus_old_resp) ) {
        type = schan_msg.genresp_v2.response.type;
        err_info = schan_msg.genresp_v2.response.err_info;
        index = schan_msg.genresp_v2.response.index;
        data = schan_msg.genresp_v2.data;
    } else {
        type = schan_msg.genresp.response.type;
        err_info = schan_msg.genresp.response.err_info;
        index = schan_msg.genresp.response.index;
        data = schan_msg.genresp.data;
    }
    if ((schan_msg.header.cpu) || (rv == SOC_E_FAIL)) {
        if (index_ptr) {
            *index_ptr = -1;
        }
        if (type == SCHAN_GEN_RESP_TYPE_FULL) {
            soc_cm_debug(DK_SOCMEM, "Insert table[%s]: hash bucket full\n",
                         SOC_MEM_NAME(unit, mem));
            rv = SOC_E_FULL;
        } else if (type == SCHAN_GEN_RESP_TYPE_ERROR) {
#ifdef BCM_ESW_SUPPORT
            if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                int rv1;
                
                rv1 = soc_ser_sram_correction(unit, SOC_PIPE_ANY,
                                              schan_msg.gencmd.header.dstblk,
                                              schan_msg.gencmd.address, mem, 
                                              copyno, index);
                if (rv1 == SOC_E_NONE) {
                    sal_memcpy(&schan_msg, &schan_msg_cpy, sizeof(schan_msg));
                    goto _retry_op;
                } 
            }
#endif /* BCM_ESW_SUPPORT */
            if (err_info == SCHAN_GEN_RESP_ERROR_BUSY) {
                soc_cm_debug(DK_SOCMEM, "Insert table[%s]: Modfifo full\n",
                             SOC_MEM_NAME(unit, mem));
                rv = SOC_E_BUSY;
            } else if (err_info == SCHAN_GEN_RESP_ERROR_PARITY) {
                soc_cm_debug(DK_ERR,
                             "Insert table[%s]: Parity Error Index %d\n",
                             SOC_MEM_NAME(unit, mem), index);
                rv = SOC_E_INTERNAL;
            }
        } else {
            rv = SOC_E_FAIL;
        }
    } else {
#ifdef BCM_TRIUMPH_SUPPORT
        if (soc_feature(unit, soc_feature_etu_support) &&
            mem == EXT_L2_ENTRY_2m) {
            soc_mem_t ext_mem;
            int ext_index;
            
            SOC_IF_ERROR_RETURN(soc_tcam_raw_index_to_mem_index(unit, index, 
                                &ext_mem, &ext_index));
            if (mem != ext_mem) {
                return SOC_E_INTERNAL;
            }
            index = ext_index;
        }
#endif /* BCM_TRIUMPH_SUPPORT */
        if (index_ptr != NULL) {
            *index_ptr = index;
        }
        if (type == SCHAN_GEN_RESP_TYPE_REPLACED) {
            if (old_entry != NULL) {
                sal_memcpy(old_entry, data, entry_dw * sizeof(uint32));
            }
            rv = SOC_E_EXISTS;
        }
        SOP_MEM_STATE(unit, mem).count[copyno]++;
#if defined(BCM_TRIUMPH_SUPPORT)
        if (mem == EXT_L2_ENTRYm) {
            SOC_IF_ERROR_RETURN
                (soc_triumph_ext_l2_entry_update(unit, index, entry));
        }
#endif /* BCM_TRIUMPH_SUPPORT */
        /* Update cache if active */
        cache = SOC_MEM_STATE(unit, mem).cache[copyno];
        vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];
        index_valid = (index >= 0 &&
                       index <= soc_mem_index_max(unit, mem));
        if (!index_valid) {
            soc_cm_debug(DK_ERR, "soc_mem_generic_insert: "
                         "invalid index %d for memory %s\n",
                         index, SOC_MEM_NAME(unit, mem));
            rv = SOC_E_INTERNAL;
        } else if (cache != NULL && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
            if (_SOC_MEM_CHK_L2_MEM(mem)) {
                /* If an invalid or non-static entry is being written then clear the cache */
                if (((mem == L2_ENTRY_2m && 
                     soc_L2_ENTRY_2m_field32_get(unit, entry, VALID_0f) &&
                     soc_L2_ENTRY_2m_field32_get(unit, entry, VALID_1f)) ||                    
                     ((mem == L2Xm || mem == L2_ENTRY_1m) && 
                      soc_mem_field32_get(unit, mem, entry, VALIDf))) &&
                     (((mem == L2Xm || mem == L2_ENTRY_1m) && 
                       soc_mem_field32_get(unit, mem, entry, STATIC_BITf)) ||
                     (mem == L2_ENTRY_2m && 
                      (soc_mem_field32_get(unit, mem, entry, STATIC_BIT_0f) &&
                       soc_mem_field32_get(unit, mem, entry, STATIC_BIT_1f))))) {
                    sal_memcpy(cache + index * entry_dw, entry, entry_dw * 4);
                    CACHE_VMAP_SET(vmap, index);
                    if (mem == L2_ENTRY_1m) {
                        vmap = SOC_MEM_STATE(unit, L2_ENTRY_2m).vmap[copyno];
                        CACHE_VMAP_CLR(vmap, index/2);
                    } else if (mem == L2_ENTRY_2m) {
                        vmap = SOC_MEM_STATE(unit, L2_ENTRY_1m).vmap[copyno];
                        CACHE_VMAP_CLR(vmap, index*2);
                        CACHE_VMAP_CLR(vmap, index*2 + 1);
                    }
                } else {
                    CACHE_VMAP_CLR(vmap, index);
                }
            } else {
                sal_memcpy(cache + index * entry_dw, entry, entry_dw * 4);
                CACHE_VMAP_SET(vmap, index);
            }
        }
    }
    return rv;
}

/*
 * Function:
 *      soc_mem_generic_delete
 * Purpose:
 *      Delete an entry
 * Parameters:
 *      unit - StrataSwitch unit #
 *      banks - For dual hashing, which halves are selected (inverted)
 *              Note: used to create a bitmap for ISM mems. (All banks: -1)
 *      entry - entry to delete
 *      old_entry - old entry if entry was found and deleted
 * Returns:
 *      SOC_E_NONE - success
 *      SOC_E_NOT_FOUND - full
 *      SOC_E_BUSY - modfifo full
 * Notes:
 *      Uses hardware deletion; sends an DELETE message over the
 *      S-Channel.
 */

int
soc_mem_generic_delete(int unit, soc_mem_t mem, int copyno, int32 banks,
                       void *entry, void *old_entry, int *index_ptr)
{
    schan_msg_t schan_msg;
    int         rv, entry_dw, index_valid;
    uint8       at, *vmap;
    int         type, err_info, index, allow_intr=0;
    uint32      *data, *cache;
    void        *null_entry = soc_mem_entry_null(unit, mem);

    _SOC_MEM_REPLACE_MEM(unit, mem);
    assert(SOC_MEM_IS_VALID(unit, mem));
    assert(soc_attached(unit));

#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_generic_delete: not supported on %s\n",
                     SOC_CHIP_STRING(unit));
    return SOC_E_UNAVAIL;
    }
#endif

    if (copyno == MEM_BLOCK_ANY) {
        /* coverity[overrun-local : FALSE] */
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }
    if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_generic_delete: invalid block %d for memory %s\n",
                     copyno, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    if (soc_cm_debug_check(DK_SOCMEM)) {
        soc_cm_print("Delete table[%s]: banks=%d", SOC_MEM_NAME(unit, mem),
                     banks);
        if (soc_cm_debug_check(DK_VERBOSE)) {
            soc_mem_entry_dump(unit, mem, entry);
        }
        soc_cm_print("\n");
    }

    entry_dw = soc_mem_entry_words(unit, mem);

    schan_msg_clear(&schan_msg);
    schan_msg.gencmd.header.opcode = TABLE_DELETE_CMD_MSG;
#ifdef BCM_EXTND_SBUS_SUPPORT
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        schan_msg.gencmd.header.srcblk =
            (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
    } else 
#endif
    {
        schan_msg.gencmd.header.srcblk = SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
    }
    schan_msg.gencmd.header.dstblk = SOC_BLOCK2SCH(unit, copyno);
    if ((soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) ||
        (soc_feature(unit, soc_feature_shared_hash_mem) && ((mem == L2Xm) ||
         (mem == L3_ENTRY_ONLYm) || (mem == L3_ENTRY_IPV4_UNICASTm) || 
         (mem == L3_ENTRY_IPV4_MULTICASTm) || (mem == L3_ENTRY_IPV6_UNICASTm) || 
         (mem == L3_ENTRY_IPV6_MULTICASTm)))) {
        /* don't set bank id in cos */
    } else {
        schan_msg.gencmd.header.cos = banks & 0x3;
    }
    schan_msg.gencmd.header.datalen = entry_dw * 4;

    schan_msg.gencmd.address = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);
    if ((soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) ||
        (soc_feature(unit, soc_feature_shared_hash_mem) && ((mem == L2Xm) ||
         (mem == L3_ENTRY_ONLYm) || (mem == L3_ENTRY_IPV4_UNICASTm) || 
         (mem == L3_ENTRY_IPV4_MULTICASTm) || (mem == L3_ENTRY_IPV6_UNICASTm) || 
         (mem == L3_ENTRY_IPV6_MULTICASTm)))) {
        /* set bank ignore mask in lower 20 bits */
        if (banks && banks != SOC_MEM_HASH_BANK_ALL) {
            if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                schan_msg.gencmd.address |= (~banks & SOC_HASH_BANK_MASK_SHARED);
            } else {
#ifdef BCM_TRIUMPH3_SUPPORT
                uint32 phy_banks = soc_ism_get_phy_bank_mask(unit, banks);
                schan_msg.gencmd.address |= (~phy_banks & SOC_HASH_BANK_MASK_ISM);
#endif /* BCM_TRIUMPH3_SUPPORT */
            }
        }
    } 

    /* Fill in packet data */
    sal_memcpy(schan_msg.gencmd.data, entry, entry_dw * 4);

    if (SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /*
     * Execute S-Channel "table delete" command packet consisting of
     * (header word + address word + entry_dw), and read back
     * (header word + response word + entry_dw) data words.
     */
    rv = soc_schan_op(unit, &schan_msg, entry_dw + 2, entry_dw + 2, allow_intr);

    if (schan_msg.readresp.header.opcode != TABLE_DELETE_DONE_MSG) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_generic_delete: "
                     "invalid S-Channel reply, expected TABLE_DELETE_DONE_MSG:\n");
        soc_schan_dump(unit, &schan_msg, 1);
        return SOC_E_INTERNAL;
    }

    if (soc_feature(unit, soc_feature_new_sbus_format) &&
        !soc_feature(unit, soc_feature_new_sbus_old_resp) ) {
        type = schan_msg.genresp_v2.response.type;
        err_info = schan_msg.genresp_v2.response.err_info;
        index = schan_msg.genresp_v2.response.index;
        data = schan_msg.genresp_v2.data;
    } else {
        type = schan_msg.genresp.response.type;
        err_info = schan_msg.genresp.response.err_info;
        index = schan_msg.genresp.response.index;
        data = schan_msg.genresp.data;
    }
    if ((schan_msg.header.cpu) || (rv == SOC_E_FAIL)) {
        if (index_ptr) {
            *index_ptr = -1;
        }
        if (type == SCHAN_GEN_RESP_TYPE_NOT_FOUND) {
            soc_cm_debug(DK_SOCMEM, "Delete table[%s]: Not found\n",
                         SOC_MEM_NAME(unit, mem));
            rv = SOC_E_NOT_FOUND;
        } else if (type == SCHAN_GEN_RESP_TYPE_ERROR) {
            if (err_info == SCHAN_GEN_RESP_ERROR_BUSY) {
                soc_cm_debug(DK_SOCMEM, "Delete table[%s]: Modfifo full\n",
                             SOC_MEM_NAME(unit, mem));
                rv = SOC_E_BUSY;
            } else if (err_info == SCHAN_GEN_RESP_ERROR_PARITY) {
                soc_cm_debug(DK_ERR,
                             "Delete table[%s]: Parity Error Index %d\n",
                             SOC_MEM_NAME(unit, mem), index);
                rv = SOC_E_INTERNAL;
            }
        } else {
            rv = SOC_E_FAIL;
        }
    } else {
        if (old_entry != NULL) {
            sal_memcpy(old_entry, data, entry_dw * sizeof(uint32));
        }
#ifdef BCM_TRIUMPH_SUPPORT
        if (soc_feature(unit, soc_feature_etu_support) &&
            mem == EXT_L2_ENTRY_2m) {
            soc_mem_t ext_mem;
            int ext_index;
            
            SOC_IF_ERROR_RETURN(soc_tcam_raw_index_to_mem_index(unit, index, 
                                &ext_mem, &ext_index));
            if (mem != ext_mem) {
                return SOC_E_INTERNAL;
            }
            index = ext_index;
        }
#endif /* BCM_TRIUMPH_SUPPORT */
        if (index_ptr != NULL) {
            *index_ptr = index;
        }
        SOP_MEM_STATE(unit, mem).count[copyno]--;
#if defined(BCM_TRIUMPH_SUPPORT)
        if (mem == EXT_L2_ENTRYm) {
            SOC_IF_ERROR_RETURN
                (soc_triumph_ext_l2_entry_update(unit, index, NULL));
        }
#endif /* BCM_TRIUMPH_SUPPORT */
        if (!SOC_WARM_BOOT(unit)) {
            /* Update cache if active */
            cache = SOC_MEM_STATE(unit, mem).cache[copyno];
            vmap = SOC_MEM_STATE(unit, mem).vmap[copyno];
            index_valid = (index >= 0 &&
                           index <= soc_mem_index_max(unit, mem));
            if (!index_valid) {
                soc_cm_debug(DK_ERR, "soc_mem_generic_delete: "
                             "invalid index %d for memory %s\n",
                             index, SOC_MEM_NAME(unit, mem));
                return SOC_E_INTERNAL;
            }
            if (index_valid && cache != NULL && CACHE_VMAP_TST(vmap, index)
                && !SOC_MEM_TEST_SKIP_CACHE(unit)) {
                if (_SOC_MEM_CHK_L2_MEM(mem)) {
                    CACHE_VMAP_CLR(vmap, index);
                    if (mem == L2_ENTRY_1m) {
                        vmap = SOC_MEM_STATE(unit, L2_ENTRY_2m).vmap[copyno];
                        CACHE_VMAP_CLR(vmap, index/2);
                    } else if (mem == L2_ENTRY_2m) {
                        vmap = SOC_MEM_STATE(unit, L2_ENTRY_1m).vmap[copyno];
                        CACHE_VMAP_CLR(vmap, index*2);
                        CACHE_VMAP_CLR(vmap, index*2 + 1);
                    }
                } else {
                    sal_memcpy(cache + index * entry_dw, null_entry, entry_dw * 4);
                    CACHE_VMAP_SET(vmap, index);
                }
            }
        }
    }
    return rv;
}

/*
 * Function:
 *      soc_mem_search
 * Purpose:
 *      Search a memory for a key.
 * Parameters:
 *      mem               Memory to search
 *      copyno               Which copy to search (if multiple copies)
 *      index_ptr            OUT:
 *                           If entry found gets the location of the entry.
 *                           If not found, and table is sorted, gets the
 *                           location of the insertion point.
 *                           CAN NO LONGER BE NULL.
 *      key_data                IN:  Data to search for
 *      entry_data,             OUT: Data if found
 *                           CAN NO LONGER BE NULL.  Must be big enough
 *                           to handle the appropriate data.
 *      lowest_match         IN:  For sorted tables only.
 *                           If there are duplicate entries in the
 *                           table, and lowest_match is 1, then the
 *                           matching entry at the lowest index is
 *                           returned.  If lowest_match is 0, then any
 *                           of the matching entries may be picked at
 *                           random, which can be faster.
 * Returns:
 *    SOC_E_NOT_FOUND
 *                 Entry not found:  in this case, if
 *                           table is sorted, then index_ptr gets the
 *                           location in which the entry should be
 *                           inserted.
 *    SOC_E_NONE         Entry is found:  index_ptr gets location
 *    SOC_E_XXX         If internal error occurs
 * Notes:
 *    A binary search is performed for a matching table entry.
 *        The appropriate field(s) of entry_data are used for the key.
 *        All other fields in entry_data are ignored.
 *
 *    If found, the index is stored in index_ptr, and if entry_data is
 *        non-NULL, the contents of the found entry are written into it,
 *        and 1 is returned.
 *    If not found, the index of a correct insertion point for the
 *        entry is stored in index_ptr and 0 is returned (unless this
 *        is a hashed table).
 *    If a table read error occurs, SOC_E_XXX is returned.
 *
 *    For the ARL table, all entries are searched.  On revision GSL and
 *        higher, a hardware lookup feature is used.  For earlier
 *        revisions, a software search is used, which is not
 *        recommended since the ARL table may be under constant
 *        update by the hardware.  For hardware lookup, the index_ptr
 *        is written with -1.
 *    For non-ARL sorted tables, only the entries known to contain data
 *        are searched:
 *        (index_min <= search_index < index_min + memInfo.count).
 */

int
soc_mem_search(int unit,
               soc_mem_t mem,
               int copyno,
               int *index_ptr,
               void *key_data,
               void *entry_data,
               int lowest_match)
{
    int rv = 0;

    COMPILER_REFERENCE(rv);
    assert(soc_mem_is_sorted(unit, mem) ||
           soc_mem_is_hashed(unit, mem) ||
           soc_mem_is_cam(unit, mem) ||
           soc_mem_is_cmd(unit, mem));
    assert(entry_data);
    assert(index_ptr);

    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }
    assert(SOC_MEM_BLOCK_VALID(unit, mem, copyno));

#ifdef BCM_TRX_SUPPORT
    if (soc_feature(unit, soc_feature_generic_table_ops)) {
        int32 banks = 0;
        _SOC_MEM_REPLACE_MEM(unit, mem);
        switch (mem) {
        case L2Xm:
        case L3_ENTRY_IPV4_UNICASTm:
        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV6_UNICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
#ifdef BCM_ISM_SUPPORT
        case L2_ENTRY_1m:
        case L2_ENTRY_2m:
        case L3_ENTRY_1m:
        case L3_ENTRY_2m:
        case L3_ENTRY_4m:
        case VLAN_XLATE_EXTDm:
        case MPLS_ENTRY_EXTDm:
#endif /* BCM_ISM_SUPPORT */
        case EGR_VLAN_XLATEm:
        case VLAN_XLATEm:
        case MPLS_ENTRYm:
        /* passthru */
        /* coverity[fallthrough: FALSE] */
#ifdef BCM_TRIUMPH3_SUPPORT
        case AXP_WRX_WCDm:
        case AXP_WRX_SVP_ASSIGNMENTm:
        case EXT_L2_ENTRY_1m:
        case EXT_L2_ENTRY_2m:
#endif /* BCM_TRIUMPH3_SUPPORT */
        case VLAN_MACm:
#ifdef BCM_TRIDENT2_SUPPORT
        case ING_VP_VLAN_MEMBERSHIPm:
        case EGR_VP_VLAN_MEMBERSHIPm:
        case ING_DNAT_ADDRESS_TYPEm:
        case L2_ENDPOINT_IDm:
        case ENDPOINT_QUEUE_MAPm:
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
        case EGR_MP_GROUPm:
#endif
        case EXT_L2_ENTRYm:
            if (soc_feature(unit, soc_feature_ism_memory) && 
                soc_mem_is_mview(unit, mem)) {
                banks = -1;
            }
            return soc_mem_generic_lookup(unit, mem, copyno, banks,
                                          key_data, entry_data, index_ptr);
        default:
            break;
        }
    }
#endif /* BCM_TRX_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FBX(unit)) {
        switch (mem) {
        case L2Xm:
            return soc_fb_l2x_lookup(unit, key_data,
                                     entry_data, index_ptr);
#if defined(INCLUDE_L3)
        case L3_DEFIPm:
            return soc_fb_lpm_match(unit, key_data, entry_data, index_ptr);
        case L3_ENTRY_IPV4_UNICASTm:
        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV6_UNICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
            return soc_fb_l3x_lookup(unit, key_data, entry_data, index_ptr);
#endif /* INCLUDE_L3 */
        case VLAN_MACm:
            return soc_fb_vlanmac_entry_lkup(unit, key_data, entry_data, index_ptr);
        default:
            break;
        }
    }
#endif /* BCM_FIREBOLT_SUPPORT */

    return _soc_mem_search(unit, mem, copyno, index_ptr, key_data,
                           entry_data, lowest_match);
}


#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_TRX_SUPPORT) \
         || defined(BCM_RAVEN_SUPPORT) || defined(BCM_HAWKEYE_SUPPORT)

/*
 * Function:
 *    soc_mem_bank_insert
 * Purpose:
 *    Do a single insert to a banked hash table with bank selection
 */
int
soc_mem_bank_insert(int unit,
                    soc_mem_t mem,
                    int32 banks,
                    int copyno,
                    void *entry_data,
                    void *old_entry_data)
{
    if (soc_feature(unit, soc_feature_generic_table_ops)) {
        return soc_mem_generic_insert(unit, mem, MEM_BLOCK_ANY,
                                      banks, entry_data, old_entry_data, 0);
    }

    switch (mem) {
    case L2Xm:
        return soc_fb_l2x_bank_insert(unit, banks,
                                      (l2x_entry_t *)entry_data);
#if defined(INCLUDE_L3)
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        return soc_fb_l3x_bank_insert(unit, banks,
                        (l3_entry_ipv6_multicast_entry_t *)entry_data);
#endif
#if defined(BCM_RAVEN_SUPPORT)
    case VLAN_MACm:
    if (SOC_IS_RAVEN(unit) || SOC_IS_HAWKEYE(unit)) {
        return soc_fb_vlanmac_entry_bank_ins(unit, banks,
            (vlan_mac_entry_t *)entry_data);
    } else {
        return SOC_E_UNAVAIL;
    }
#endif
    default:
        break;
    }

    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *    soc_mem_bank_search
 * Purpose:
 *    Search a banked hash table with bank selection
 */
int
soc_mem_bank_search(int unit,
                    soc_mem_t mem,
                    uint8 banks,
                    int copyno,
                    int *index_ptr,
                    void *key_data,
                    void *entry_data)
{
    switch (mem) {
    case L2Xm:
        return soc_fb_l2x_bank_lookup(unit, banks, (l2x_entry_t *)key_data,
                                      (l2x_entry_t *)entry_data, index_ptr);
#if defined(INCLUDE_L3)
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        return soc_fb_l3x_bank_lookup(unit, banks,
                        (l3_entry_ipv6_multicast_entry_t *)key_data,
                        (l3_entry_ipv6_multicast_entry_t *)entry_data,
                                        index_ptr);
#endif
    default:
        break;
    }

    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *    _soc_mem_dual_hash_get
 * Purpose:
 *    Calcualte hash for dual hashed memories
 */

STATIC int
_soc_mem_dual_hash_get(int unit, soc_mem_t mem, int hash_sel,
                       void *entry_data)
{
    switch (mem) {
    case L2Xm:
#if defined(BCM_TRX_SUPPORT)
    if (SOC_IS_TRX(unit)) {
        return soc_tr_l2x_entry_hash(unit, hash_sel, entry_data);
    } else
#endif 
    {
        return soc_fb_l2x_entry_hash(unit, hash_sel, entry_data);
    }
#if defined(INCLUDE_L3)
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        return soc_fb_l3x_entry_hash(unit, hash_sel, entry_data);
#endif /* INCLUDE_L3 */
#if defined(BCM_RAVEN_SUPPORT)
    case VLAN_MACm:
        if (SOC_IS_RAVEN(unit)  || SOC_IS_HAWKEYE(unit)) {
            return soc_fb_vlan_mac_entry_hash(unit, hash_sel, entry_data);
        } else {
            return -1;
        }
#endif

#if defined(BCM_TRX_SUPPORT)
    case VLAN_XLATEm:
#if defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TD2_TT2(unit)) {
            return soc_td2_vlan_xlate_entry_hash(unit, hash_sel, entry_data);
        } else
#endif /* BCM_TRIDENT2_SUPPORT */
        if (SOC_IS_TRX(unit)) {
            return soc_tr_vlan_xlate_entry_hash(unit, hash_sel, entry_data);
        }
        return -1;

    case MPLS_ENTRYm:
#if defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TD2_TT2(unit)) {
            return soc_td2_mpls_entry_hash(unit, hash_sel, entry_data);
        } else
#endif /* BCM_TRIDENT2_SUPPORT */
        if (SOC_IS_TR_VL(unit)) {
            return soc_tr_mpls_entry_hash(unit, hash_sel, entry_data);
        }
        return -1;

    case EGR_VLAN_XLATEm:
#if defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TD2_TT2(unit)) {
            return soc_td2_egr_vlan_xlate_entry_hash(unit, hash_sel,
                                                     entry_data);
        } else
#endif /* BCM_TRIDENT2_SUPPORT */
        if (SOC_IS_TRX(unit)) {
            return soc_tr_egr_vlan_xlate_entry_hash(unit, hash_sel, entry_data);
        }
        return -1;
#endif

#ifdef BCM_TRIUMPH3_SUPPORT
    case AXP_WRX_WCDm:
    case AXP_WRX_SVP_ASSIGNMENTm:
        if (SOC_IS_TRIUMPH3(unit)) {
            return soc_tr3_wlan_entry_hash(unit, mem, hash_sel, entry_data);
        }
        return -1;
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
    case ING_VP_VLAN_MEMBERSHIPm:
        if (SOC_IS_TD2_TT2(unit)) {
            return soc_td2_ing_vp_vlan_member_entry_hash(unit, hash_sel,
                                                        entry_data);
        }
        return -1;
    case EGR_VP_VLAN_MEMBERSHIPm:
        if (SOC_IS_TD2_TT2(unit)) {
            return soc_td2_egr_vp_vlan_member_entry_hash(unit, hash_sel,
                                                        entry_data);
        }
        return -1;
    case ING_DNAT_ADDRESS_TYPEm:
        if (SOC_IS_TD2_TT2(unit)) {
            return soc_td2_ing_dnat_address_type_entry_hash(unit, hash_sel,
                                                            entry_data);
        }
        return -1;
    case L2_ENDPOINT_IDm:
        if (SOC_IS_TD2_TT2(unit)) {
            return soc_td2_l2_endpoint_id_entry_hash(unit, hash_sel,
                                                     entry_data);
        }
        return -1;
    case ENDPOINT_QUEUE_MAPm:
        if (SOC_IS_TD2_TT2(unit)) {
            return soc_td2_endpoint_queue_map_entry_hash(unit, hash_sel,
                                                         entry_data);
        }
        return -1;
#endif /* BCM_TRIDENT2_SUPPORT */

    default:
        return -1;
    }
}

/*
 * Function:
 *    _soc_mem_dual_hash_move
 * Purpose:
 *    Recursive move routine for dual hash auto-move inserts
 *      Assumes feature already checked, mem locks taken.
 */

STATIC int
_soc_mem_dual_hash_move(int unit, soc_mem_t mem, /* Assumes memory locked */
                        uint8 banks, int copyno, void *entry_data,
                        dual_hash_info_t *hash_info,
                        SHR_BITDCL *bucket_trace, int recurse_depth)
{
    int hash_base, cur_index = -1, base_index, bucket_index;
    int dest_hash_base, dest_bucket_index = -1;
    int half_bucket, hw_war = 0, trace_size = 0;
    SHR_BITDCL *trace;               /* Buckets involved in recursion.   */
    int rv = SOC_E_NONE, bix, i, found = FALSE;
    uint8 this_bank_bit, this_bank_only, that_bank_only, this_hash, that_hash;
    uint32 move_entry[SOC_MAX_MEM_WORDS];
    static uint32 recurse;

    if (recurse_depth < 0) {
        return SOC_E_FULL;
    }
    if ((SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit) ||
         SOC_IS_SCORPION(unit)) && (mem == EGR_VLAN_XLATEm)) {
        /* Handle h/w limitation for this memory */
        hw_war = TRUE;
    }
    /* Stack variables initialization & memory allocations.*/
    half_bucket = hash_info->bucket_size / 2;

    /* Keep back trace of all buckets affected by recursion. */
    if (NULL == bucket_trace) {
        trace_size =
            SHR_BITALLOCSIZE(soc_mem_index_count(unit, hash_info->base_mem)/
                             half_bucket);
        trace =  sal_alloc(trace_size, "Dual hash");
        if (NULL == trace) {
            return (SOC_E_MEMORY);
        }
        recurse = 0;
    } else {
        trace = bucket_trace;
        recurse++;
    }

    /* Iterate over banks. */
    for (bix = 0; bix < 2; bix++) {
        this_bank_bit =
            (bix == 0) ? SOC_MEM_HASH_BANK0_BIT : SOC_MEM_HASH_BANK1_BIT;
        this_bank_only =
            (bix == 0) ? SOC_MEM_HASH_BANK0_ONLY : SOC_MEM_HASH_BANK1_ONLY;
        that_bank_only =
            (bix == 0) ? SOC_MEM_HASH_BANK1_ONLY : SOC_MEM_HASH_BANK0_ONLY;
        this_hash = (bix == 0) ? hash_info->hash_sel0 : hash_info->hash_sel1;
        that_hash = (bix == 0) ? hash_info->hash_sel1 : hash_info->hash_sel0;

        if (banks & this_bank_bit) {
            /* Not this bank */
            continue;
        }
        hash_base = _soc_mem_dual_hash_get(unit, mem, this_hash, entry_data);
        if (hash_base == -1) {
            rv = SOC_E_INTERNAL;
            break;
        }

        /* Bucket half is based on bank id.      */
        bucket_index = hash_base * hash_info->bucket_size + bix * half_bucket;

        /* Recursion trace initialization. */
        if (NULL == bucket_trace) {
            sal_memset(trace, 0, trace_size);
        }
        SHR_BITSET(trace, bucket_index/half_bucket);

        /* Iterate over entries in the half-bucket.. */
        for (i = 0; i < (hash_info->bucket_size / 2); i++) {
            cur_index = base_index = bucket_index + i;
            /* Read bucket entry. */
            rv = soc_mem_read(unit, hash_info->base_mem, copyno,
                              base_index, move_entry);
            if (SOC_FAILURE(rv)) {
                rv = SOC_E_MEMORY;
                break;
            }
            /* Is this an exception mem OR
               Are we already in recursion or will we be in recursion */
            if (hw_war || recurse || recurse_depth) {
                /* Calculate destination entry hash value. */
                dest_hash_base = _soc_mem_dual_hash_get(unit, mem, that_hash,
                                                        move_entry);
                if (dest_hash_base == -1) {
                    rv = SOC_E_INTERNAL;
                    break;
                }
                dest_bucket_index =
                    dest_hash_base * hash_info->bucket_size + (!bix) * half_bucket;

                /* Make sure we are not touching buckets in bucket trace. */
                if(SHR_BITGET(trace, dest_bucket_index/half_bucket)) {
                    continue;
                }
            }

            /* Attempt to insert it into the other bank. */
            if (hw_war) {
                int e, notfound = TRUE;
                uint32 tmp_entry[SOC_MAX_MEM_WORDS];

                for (e = 0; e < (hash_info->bucket_size / 2); e++) {
                    rv = soc_mem_read(unit, mem, copyno,
                                      dest_bucket_index + e, tmp_entry);
                    if (SOC_FAILURE(rv)) {
                        rv = SOC_E_MEMORY;
                        break;
                    }
                    if (soc_mem_field32_get(unit, mem, tmp_entry, VALIDf)) {
                        continue;
                    } else {
                        rv = soc_mem_write(unit, mem, copyno, dest_bucket_index + e, 
                                           move_entry);
                        if (SOC_FAILURE(rv)) {
                            rv = SOC_E_MEMORY;
                            break;
                        }
                        notfound = FALSE;
                        break;
                    }
                }
                if ((rv != SOC_E_MEMORY) && (notfound == TRUE)) {
                    rv = SOC_E_FULL;
                }
            } else {
                rv = soc_mem_bank_insert(unit, mem, that_bank_only,
                                         copyno, move_entry, NULL);
            }
            if (SOC_FAILURE(rv)) {
                if (rv != SOC_E_FULL) {
                    if ((rv == SOC_E_EXISTS) && soc_cm_debug_check(DK_VERBOSE)) {
                        soc_mem_entry_dump(unit, mem, move_entry);
                    }
                    break;
                }
                /* Recursive call - attempt to create a slot
                   in other bank bucket. */
                rv = _soc_mem_dual_hash_move(unit, mem, that_bank_only,
                                             copyno, move_entry, hash_info,
                                             trace, recurse_depth - 1);
                if (SOC_FAILURE(rv)) {
                    if (rv != SOC_E_FULL) {
                        if ((rv == SOC_E_EXISTS) && soc_cm_debug_check(DK_VERBOSE)) {
                            soc_mem_entry_dump(unit, mem, move_entry);
                        }
                        break;
                    }
                    continue;
                }
            }
            /* Entry was moved successfully. */
            found = TRUE;
            /* Delete old entry from original location */ 
            if (hw_war) {
                /* A entry clear is not needed as the entry is written over later on.
                   soc_mem_field32_set(unit, mem, &move_entry, VALIDf, 0);
                   rv = soc_mem_write(unit, mem, copyno, cur_index, move_entry); 
                */
            } else {
                rv = soc_mem_generic_delete(unit, mem, MEM_BLOCK_ANY, this_bank_only, 
                                            move_entry, NULL, NULL);
            }
            break;
        }  /* Bucket iteration loop. */

        if (found || ((rv < 0) && (rv != SOC_E_FULL))) {
            break;
        }
    } /* Bank iteration loop. */

    if ((rv < 0) && (rv != SOC_E_FULL)) {
        if (NULL == bucket_trace) sal_free(trace);
        return rv;
    }

    if (!found) {
        if (NULL == bucket_trace) sal_free(trace);
        return SOC_E_FULL;
    }
    if (hw_war) {
        rv = soc_mem_write(unit, mem, copyno, cur_index, entry_data);
    } else {
        rv = soc_mem_generic_insert(unit, mem, copyno, this_bank_only, entry_data,
                                    NULL, NULL); 
    }
    if (NULL == bucket_trace) sal_free(trace);
    return (rv);
}

/*
 * Function:
 *    _soc_mem_dual_hash_insert
 * Purpose:
 *    Dual hash auto-move inserts
 *      Assumes feature already checked
 */

int
_soc_mem_dual_hash_insert(int unit,
                          soc_mem_t mem,     /* Assumes memory locked */
                          int copyno,
                          void *entry_data,
                          void *old_entry_data,
                          int recurse_depth)
{
    int rv = SOC_E_NONE;
    dual_hash_info_t hash_info = {0};

    switch (mem) {
    case L2Xm:
        rv = soc_mem_bank_insert(unit, mem, 0, copyno,
                                 entry_data, old_entry_data);
        if (rv != SOC_E_FULL || recurse_depth == 0) {
            return rv;
        }
        SOC_IF_ERROR_RETURN
            (soc_fb_l2x_entry_bank_hash_sel_get(unit, 0,
                                                &(hash_info.hash_sel0)));
        SOC_IF_ERROR_RETURN
            (soc_fb_l2x_entry_bank_hash_sel_get(unit, 1,
                                                &(hash_info.hash_sel1)));
        if (hash_info.hash_sel0 == hash_info.hash_sel1) {
            /* Can't juggle the entries */
            return SOC_E_FULL;
        }
        hash_info.bucket_size = SOC_L2X_BUCKET_SIZE;
        hash_info.base_mem = mem;

        /* Time to shuffle the entries */
        SOC_IF_ERROR_RETURN(soc_l2x_freeze(unit));
        rv = _soc_mem_dual_hash_move(unit, mem, SOC_MEM_HASH_BANK_BOTH,
                                     copyno, entry_data, &hash_info,
                                     NULL, recurse_depth - 1);
        SOC_IF_ERROR_RETURN(soc_l2x_thaw(unit));
        return rv;
#if defined(BCM_TRIUMPH_SUPPORT)
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_TRX_SUPPORT
    case VLAN_XLATEm:
        if (!SOC_IS_TRX(unit)) {
            break;
        }
        /* Fall thru for TRX (VLAN_XLATE & VLAN_MAC are a shared table) */
#endif /* BCM_TRX_SUPPORT */
#if defined(BCM_RAVEN_SUPPORT) || defined(BCM_TRX_SUPPORT)
    case VLAN_MACm:
        if (SOC_IS_FIREBOLT2(unit)) {
            break;
        }
    rv = soc_mem_bank_insert(unit, mem, 0, copyno,
                                 entry_data, old_entry_data);
        if (rv != SOC_E_FULL || recurse_depth == 0) {
            return rv;
        }
#ifdef BCM_TRX_SUPPORT
        if (mem == VLAN_XLATEm) {
            SOC_IF_ERROR_RETURN
                (soc_tr_hash_sel_get(unit, mem, 0, &hash_info.hash_sel0));
            SOC_IF_ERROR_RETURN
                (soc_tr_hash_sel_get(unit, mem, 1, &hash_info.hash_sel1));
            hash_info.bucket_size = SOC_VLAN_XLATE_BUCKET_SIZE;
        } else
#endif /* BCM_TRX_SUPPORT */
        {
            SOC_IF_ERROR_RETURN
                (soc_fb_rv_vlanmac_hash_sel_get(unit, 0, &(hash_info.hash_sel0)));

            SOC_IF_ERROR_RETURN
                (soc_fb_rv_vlanmac_hash_sel_get(unit, 1, &(hash_info.hash_sel1)));
            hash_info.bucket_size = SOC_VLAN_MAC_BUCKET_SIZE;
        }
        if (hash_info.hash_sel0 == hash_info.hash_sel1) {
            /* Can't juggle the entries */
            return SOC_E_FULL;
        }
        hash_info.base_mem = mem;
        /* Time to shuffle the entries */
        if (SOC_IS_TRX(unit)) {
            soc_mem_lock(unit, VLAN_XLATEm);
        }
        soc_mem_lock(unit, VLAN_MACm);
        rv = _soc_mem_dual_hash_move(unit, mem, SOC_MEM_HASH_BANK_BOTH,
                                     copyno, entry_data, &hash_info,
                                     NULL, recurse_depth - 1);
        soc_mem_unlock(unit, VLAN_MACm);
        if (SOC_IS_TRX(unit)) {
            soc_mem_unlock(unit, VLAN_XLATEm);
        }
        return rv;
#endif /* BCM_RAVEN_SUPPORT || BCM_TRX_SUPPORT */
#if defined(INCLUDE_L3)
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        return soc_fb_l3x_insert(unit, entry_data);
#endif /* INCLUDE_L3 */
#if defined(BCM_TRX_SUPPORT)
    case MPLS_ENTRYm:
        hash_info.bucket_size = SOC_MPLS_ENTRY_BUCKET_SIZE;
        break;
    case EGR_VLAN_XLATEm:
        hash_info.bucket_size = SOC_EGR_VLAN_XLATE_BUCKET_SIZE;
        break;
    case AXP_WRX_WCDm:
    case AXP_WRX_SVP_ASSIGNMENTm:
        hash_info.bucket_size = SOC_WLAN_BUCKET_SIZE;
        break;
#ifdef BCM_TRIDENT2_SUPPORT
    case ING_VP_VLAN_MEMBERSHIPm:
        hash_info.bucket_size = SOC_ING_VP_VLAN_MEMBER_BUCKET_SIZE;
        break;
    case EGR_VP_VLAN_MEMBERSHIPm:
        hash_info.bucket_size = SOC_EGR_VP_VLAN_MEMBER_BUCKET_SIZE;
        break;
    case ING_DNAT_ADDRESS_TYPEm:
        hash_info.bucket_size = SOC_ING_DNAT_ADDRESS_TYPE_BUCKET_SIZE;
        break;
    case L2_ENDPOINT_IDm:
        hash_info.bucket_size = SOC_L2_ENDPOINT_ID_BUCKET_SIZE;
        break;
    case ENDPOINT_QUEUE_MAPm:
        hash_info.bucket_size = SOC_ENDPOINT_QUEUE_MAP_BUCKET_SIZE;
        break;
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
    case EGR_MP_GROUPm:
        hash_info.bucket_size = SOC_EGR_MP_GROUP_BUCKET_SIZE;
        break;
#endif
#endif /* BCM_TRX_SUPPORT */
    default:
        return SOC_E_UNAVAIL;
    }

#if defined(BCM_TRX_SUPPORT)
    /*
     * If a block of physical memory is not shared by multiple table and
     * it is not modified by hardware, we should be able to use general case
     * for dual hash insert
     */
    rv = soc_mem_bank_insert(unit, mem, 0, copyno, entry_data, old_entry_data);
    if (rv != SOC_E_FULL || recurse_depth == 0) {
        return rv;
    }
    SOC_IF_ERROR_RETURN
        (soc_tr_hash_sel_get(unit, mem, 0, &hash_info.hash_sel0));
    SOC_IF_ERROR_RETURN
        (soc_tr_hash_sel_get(unit, mem, 1, &hash_info.hash_sel1));
    if (hash_info.hash_sel0 == hash_info.hash_sel1) {
        /* Can't juggle the entries */
        return SOC_E_FULL;
    }
    hash_info.base_mem = mem;
    /* Time to shuffle the entries */
    soc_mem_lock(unit, mem);
    rv = _soc_mem_dual_hash_move(unit, mem, SOC_MEM_HASH_BANK_BOTH, copyno,
                                 entry_data, &hash_info, NULL,
                                 recurse_depth - 1);
    soc_mem_unlock(unit, mem);
#endif /* BCM_TRX_SUPPORT */

    return rv;
}

#ifdef BCM_ISM_SUPPORT
/*
 * Function:
 *    _soc_mem_multi_hash_insert
 * Purpose:
 *    N hash auto-move inserts
 *      Assumes feature already checked
 */

STATIC int
_soc_mem_multi_hash_insert(int unit, soc_mem_t mem, int copyno,
                           void *entry_data, void *old_entry_data,
                           int recurse_depth)
{
    int rv = SOC_E_NONE;
    rv = soc_mem_bank_insert(unit, mem, SOC_MEM_HASH_BANK_ALL, copyno,
                             entry_data, old_entry_data);
    if (rv != SOC_E_FULL) {
        return rv;
    }
    /* Time to shuffle the entries */
    soc_mem_lock(unit, mem);
    rv = soc_mem_multi_hash_move(unit, mem, SOC_MEM_HASH_BANK_ALL, copyno,
                                 entry_data, NULL, NULL, recurse_depth-1);
    soc_mem_unlock(unit, mem);
    return rv;
}
#endif /* BCM_ISM_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
STATIC int
_soc_mem_shared_hash_incr_per_entry(int unit, soc_mem_t mem, void *old_entry,
                                    void *new_entry)
{
    if (mem == L2Xm) {
        return 1;
    } else {
        int oi = 1;
        if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_ONLYm, KEY_TYPEf)) {
            int key_type;

            key_type = soc_mem_field32_get(unit, L3_ENTRY_ONLYm,
                                           old_entry, KEY_TYPEf);
            switch (key_type) {
            case TD2_L3_HASH_KEY_TYPE_V4UC:
            case TD2_L3_HASH_KEY_TYPE_TRILL:
            case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN:
            case TD2_L3_HASH_KEY_TYPE_FCOE_HOST:
            case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP:
                oi = 1;
                break;
            case TD2_L3_HASH_KEY_TYPE_V4UC_EXT:
            case TD2_L3_HASH_KEY_TYPE_V4MC:
            case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN_EXT:
            case TD2_L3_HASH_KEY_TYPE_FCOE_HOST_EXT:
            case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP_EXT:
            case TD2_L3_HASH_KEY_TYPE_DST_NAT:
            case TD2_L3_HASH_KEY_TYPE_DST_NAPT:
            case TD2_L3_HASH_KEY_TYPE_V6UC:
                oi = 2;
                break;
            case TD2_L3_HASH_KEY_TYPE_V6UC_EXT:
            case TD2_L3_HASH_KEY_TYPE_V6MC:
                oi = 4;
                break;
            default:
                oi = 1;
            }
            key_type = soc_mem_field32_get(unit, L3_ENTRY_ONLYm,
                                           new_entry, KEY_TYPEf);
            switch (key_type) {
            case TD2_L3_HASH_KEY_TYPE_V4UC:
            case TD2_L3_HASH_KEY_TYPE_TRILL:
            case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN:
            case TD2_L3_HASH_KEY_TYPE_FCOE_HOST:
            case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP:
                return (oi > 1) ? oi : 1;
            case TD2_L3_HASH_KEY_TYPE_V4UC_EXT:
            case TD2_L3_HASH_KEY_TYPE_V4MC:
            case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN_EXT:
            case TD2_L3_HASH_KEY_TYPE_FCOE_HOST_EXT:
            case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP_EXT:
            case TD2_L3_HASH_KEY_TYPE_DST_NAT:
            case TD2_L3_HASH_KEY_TYPE_DST_NAPT:
            case TD2_L3_HASH_KEY_TYPE_V6UC:
                return (oi > 2) ? oi : 2;
            case TD2_L3_HASH_KEY_TYPE_V6UC_EXT:
            case TD2_L3_HASH_KEY_TYPE_V6MC:
                return 4;
            default:
                return (oi > 1) ? oi : 1;
            }
        } else {
            return 1;
        }
    }
}

STATIC int
_soc_mem_shared_hash_move(int unit, soc_mem_t mem, int32 banks, int copyno,
                          void *entry, SHR_BITDCL *bucket_trace,
                          int recurse_depth)
{
    static int mc = 0, ic = 0;
    static SHR_BITDCL *trace;
    static int num_banks, bank_ids[7];
    static uint8 num_ent = 0;
    static uint32 recurse, numb = 0;
    int32 nbix;
    int rv = SOC_E_NONE, index;
    uint8 i, bix, found = 0, cb = 0;
    uint32 db, dest_bucket, trace_size = 0;
    uint32 bucket, move_entry[SOC_MAX_MEM_WORDS];

    if (recurse_depth < 0) {
        return SOC_E_FULL;
    }
    /* Stack variables initialization & memory allocations */
    if (NULL == bucket_trace) {
        num_ent = _soc_mem_shared_hash_entries_per_bkt(mem);
        /* For simplicity, allocate same/max number of bits for buckets for all banks */
        numb = 65536;
        /* Keep back trace of all buckets affected by recursion. */
        trace_size = SHR_BITALLOCSIZE(numb * 10);
        /* For simplicity, allocate same number of bits for buckets per bank */
        trace =  sal_alloc(trace_size, "Shared hash");
        if (NULL == trace) {
            return SOC_E_MEMORY;
        }
        sal_memset(trace, 0, trace_size);
        SOC_IF_ERROR_RETURN
            (soc_trident2_hash_bank_count_get(unit, mem, &num_banks));
        for (bix = 0; bix < num_banks; bix++) {
            SOC_IF_ERROR_RETURN
                (soc_trident2_hash_bank_number_get(unit, mem, bix,
                                                   &bank_ids[bix]));
        }
    }
    /* Iterate over banks. */
    for (bix = 0; bix < num_banks; bix++) {
        cb = bank_ids[bix];
        if ((banks != SOC_MEM_HASH_BANK_ALL) && (banks == cb)) {
            /* Not this bank */
            continue;
        }
        nbix = (bix+1 >= num_banks) ? 0 : bix+1; /* next bank index */
        db = bank_ids[nbix]; /* next destination bank */
        rv = soc_hash_bucket_get(unit, mem, cb, entry, &bucket);
        if (SOC_FAILURE(rv)) {
            break;
        }
        SHR_BITSET(trace, (numb * bix) + bucket);
        index = soc_hash_index_get(unit, mem, cb, bucket);
        for (i = 0; i < num_ent;) {
            rv = soc_mem_read(unit, mem, copyno, index+i, move_entry);
            if (SOC_FAILURE(rv)) {
                rv = SOC_E_MEMORY;
                break;
            }
            /* Are we already in recursion or will we be in recursion */
            if (recurse || recurse_depth) {
                /* Calculate destination entry hash value. */
                rv = soc_hash_bucket_get(unit, mem, db, move_entry, &dest_bucket);
                if (SOC_FAILURE(rv)) {
                    break;
                }
                if(SHR_BITGET(trace, (numb * nbix) + dest_bucket)) {
                    /* Determine bucket offset increment based upon existing entry type/size */
                    i += _soc_mem_shared_hash_incr_per_entry(unit, mem, move_entry, entry);
                    soc_cm_debug(DK_VERBOSE+DK_SOCMEM, "Skip bucket: %d\n", 
                                 (numb * nbix) + dest_bucket);
                    continue;
                }
            }
            /* Attempt to insert it into the other bank. */
            rv = soc_mem_bank_insert(unit, mem, (uint32)1<<db, copyno, move_entry, NULL);
            if (SOC_FAILURE(rv)) {
                if (rv != SOC_E_FULL) {
                    break;
                }
                /* Recursive call - attempt to create a slot
                   in another bank's bucket. */
                rv = _soc_mem_shared_hash_move(unit, mem, cb, copyno, 
                                               move_entry, trace,
                                               recurse_depth - 1);
                if (SOC_FAILURE(rv)) {
                    if (rv != SOC_E_FULL) {
                        break;
                    }
                    i += _soc_mem_shared_hash_incr_per_entry(unit, mem, move_entry, entry);
                    continue;
                }
            }
            /* Entry was moved successfully. */
            found = TRUE;
            /* Delete old entry from original location */ 
            rv = soc_mem_generic_delete(unit, mem, MEM_BLOCK_ANY, (uint32)1<<cb, move_entry, 
                                        NULL, NULL);
            soc_cm_debug(DK_VERBOSE, "Delete moved entry [%d]: %d\n", mc++, rv);
            break;
        }  /* Bucket iteration loop. */
        if (found || ((rv < 0) && (rv != SOC_E_FULL))) {
            break;
        }
    } /* Bank iteration loop. */
    if ((rv < 0) && (rv != SOC_E_FULL)) {
        if (NULL == bucket_trace) { 
            sal_free(trace);
        }
        return rv;
    }
    if (!found) {
        if (NULL == bucket_trace) {
            sal_free(trace);
        }
        return SOC_E_FULL;
    }
    rv = soc_mem_generic_insert(unit, mem, copyno, (uint32)1<<cb, entry, entry, NULL);
    if (rv) {
        soc_cm_debug(DK_VERBOSE+DK_SOCMEM, "Insert entry: %d\n", rv);
    } else {
        soc_cm_debug(DK_VERBOSE, "Insert entry [%d]\n", ic++);
    }
    if (NULL == bucket_trace) {
        sal_free(trace);
    }
    return rv;
}

STATIC int
_soc_mem_shared_hash_insert(int unit, soc_mem_t mem, int copyno,
                            void *entry_data, void *old_entry_data,
                            int recurse_depth)
{
    int rv = SOC_E_NONE;
    rv = soc_mem_bank_insert(unit, mem, SOC_MEM_HASH_BANK_ALL, copyno,
                             entry_data, old_entry_data);
    if (rv != SOC_E_FULL) {
        return rv;
    }
    /* Time to shuffle the entries */
    soc_mem_lock(unit, mem);
    rv = _soc_mem_shared_hash_move(unit, mem, SOC_MEM_HASH_BANK_ALL, copyno,
                                   entry_data, NULL, recurse_depth-1);
    soc_mem_unlock(unit, mem);
    return rv;
}
#endif /* BCM_TRIDENT2_SUPPORT */

#endif /* BCM_FIREBOLT2_SUPPORT || BCM_TRX_SUPPORT \
          || BCM_RAVEN_SUPPORT */

/*
 * Function:
 *    _soc_mem_insert
 * Purpose:
 *    Helper routine for soc_mem_insert
 */

STATIC int
_soc_mem_insert(int unit,
                soc_mem_t mem,        /* Assumes memory locked */
                int copyno,
                void *entry_data)
{
    uint32        entry_tmp[SOC_MAX_MEM_WORDS];
    int            max, ins_index, index, last, rv;

    max = soc_mem_index_max(unit, mem);
    last = soc_mem_index_last(unit, mem, copyno);

    rv = soc_mem_search(unit, mem, copyno,
                        &ins_index, entry_data, entry_tmp, 0);

    if (rv == SOC_E_NONE) {
        /* If entry with matching key already exists, overwrite it. */

        if ((rv = soc_mem_write(unit, mem, copyno,
                                ins_index, entry_data)) < 0) {
            soc_cm_debug(DK_ERR,
                         "soc_mem_insert: write %s.%s[%d] failed\n",
                         SOC_MEM_UFNAME(unit, mem),
                         SOC_BLOCK_NAME(unit, copyno), ins_index);
            return rv;
        }

        return SOC_E_NONE;
    }

    if (rv != SOC_E_NOT_FOUND) {
        return rv;
    }

    /* Point to first unused entry; fail if table is already full. */

    if ((index = last + 1) > max) {
        return SOC_E_FULL;
    }

    while (index > ins_index) {
        /* Move up one entry */

        if ((rv = soc_mem_read(unit, mem, copyno,
                               index - 1, entry_tmp)) < 0) {
            soc_cm_debug(DK_ERR,
                         "soc_mem_insert: read %s.%s[%d] failed\n",
                         SOC_MEM_UFNAME(unit, mem),
                         SOC_BLOCK_NAME(unit, copyno), index - 1);
            return rv;
        }

        if ((rv = soc_mem_write(unit, mem, copyno,
                                index, entry_tmp)) < 0) {
            soc_cm_debug(DK_ERR,
                         "soc_mem_insert: write %s.%s[%d] failed\n",
                         SOC_MEM_UFNAME(unit, mem),
                         SOC_BLOCK_NAME(unit, copyno), index);
            return rv;
        }

        index--;
    }

    /* Write entry data at insertion point */

    if ((rv = soc_mem_write(unit, mem, copyno,
                            ins_index, entry_data)) < 0) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_insert: write %s.%s[%d] failed\n",
                     SOC_MEM_UFNAME(unit, mem),
                     SOC_BLOCK_NAME(unit, copyno), ins_index);
        return rv;
    }
    _SOC_MEM_REUSE_MEM_STATE(unit, mem);
    SOP_MEM_STATE(unit, mem).count[copyno]++;

    return SOC_E_NONE;
}

/*
 * Function:
 *    soc_mem_insert
 * Purpose:
 *    Insert an entry into a structured table based on key.
 * Returns:
 *    SOC_E_NONE - insertion succeeded
 *    SOC_E_FULL - table full
 *    SOC_E_XXX - other error
 * Notes:
 *      Tables may be binary, hashed, CAM, or command.
 *
 *      Binary tables:
 *    A binary search is made for the entry, and if a matching key is
 *    found, the entry is overwritten with new data, and 0 is returned.
 *    If key is not found:
 *    Starting with the last entry (kept in soc_control_t structure),
 *    entries are moved down until a space is made for the required
 *    index.   The moving process results in temporary duplicates but not
 *    temporarily-out-of-order entries.
 *    Fails if the table is already full before the insert.
 *
 *      Hashed tables:
 *      The entry is passed to the hardware, which performs a hash
 *      calculation on the key.  The entry wil be added to the
 *      corresponding hash bucket, if an available slot exists.
 *      Failes if the hash bucket for that key is full.
 *
 *      CAM tables:
 *      Entries are added to the CAM based on the device's
 *      implementation-specific algorithm.
 *      Fails if table resources are exhausted.
 *
 *      Command supported tables:
 *      Uses memory command hardware support to handle table state,
 *      based on the entry key.
 *      Fails if hardware insert operation is unsuccessful.
 *
 *    If COPYNO_ALL is specified, the insertion is repeated for each
 *    copy of the table.
 */

int
soc_mem_insert(int unit,
               soc_mem_t mem,
               int copyno,
               void *entry_data)
{
    int rv = SOC_E_NONE;

    assert(soc_mem_is_sorted(unit, mem) ||
           soc_mem_is_hashed(unit, mem) ||
           soc_mem_is_cam(unit, mem) ||
           soc_mem_is_cmd(unit, mem));
    assert(entry_data);

    if (copyno == COPYNO_ALL) {

#ifdef BCM_TRX_SUPPORT
        if (SOC_IS_TRX(unit)) {
            _SOC_MEM_REPLACE_MEM(unit, mem);
            switch (mem) {
            case MPLS_ENTRYm:
                if (SOC_IS_SC_CQ(unit)) {
                    break;
                }
            case VLAN_XLATEm:
            case EGR_VLAN_XLATEm:
#ifdef BCM_ISM_SUPPORT
                if (soc_feature(unit, soc_feature_ism_memory)) {
                    break;
                }
#endif /* BCM_ISM_SUPPORT */
            case VLAN_MACm:
            case AXP_WRX_WCDm:
            case AXP_WRX_SVP_ASSIGNMENTm:
#ifdef BCM_TRIDENT2_SUPPORT
            case ING_VP_VLAN_MEMBERSHIPm:
            case EGR_VP_VLAN_MEMBERSHIPm:
            case ING_DNAT_ADDRESS_TYPEm:
            case L2_ENDPOINT_IDm:
            case ENDPOINT_QUEUE_MAPm:
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
            case EGR_MP_GROUPm:
#endif 
                return _soc_mem_dual_hash_insert(unit, mem, copyno,
                                                 entry_data, NULL,
                           soc_dual_hash_recurse_depth_get(unit, mem));
            default:
                break;
            }
        }
#endif /* BCM_TRX_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
        if (SOC_IS_FBX(unit)) {
            switch (mem) {
            case L2Xm:
#ifdef BCM_TRIDENT2_SUPPORT
                if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                    return _soc_mem_shared_hash_insert(unit, mem, copyno, 
                                                       entry_data, NULL, 
                               soc_multi_hash_recurse_depth_get(unit, mem));
                }
#endif /* BCM_TRIDENT2_SUPPORT */
                return soc_fb_l2x_insert(unit, entry_data);
#if defined(INCLUDE_L3)
            case L3_DEFIPm:
                return soc_fb_lpm_insert(unit, entry_data);
            case L3_ENTRY_ONLYm:
            case L3_ENTRY_IPV4_UNICASTm:
            case L3_ENTRY_IPV4_MULTICASTm:
            case L3_ENTRY_IPV6_UNICASTm:
            case L3_ENTRY_IPV6_MULTICASTm:
#ifdef BCM_TRIDENT2_SUPPORT
                if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                    return _soc_mem_shared_hash_insert(unit, mem, copyno, 
                                                       entry_data, NULL, 
                               soc_multi_hash_recurse_depth_get(unit, mem));
                }
#endif /* BCM_TRIDENT2_SUPPORT */
                return soc_fb_l3x_insert(unit, entry_data);
#endif /* INCLUDE_L3 */
            case VLAN_MACm:
#if defined(BCM_RAVEN_SUPPORT)
                if (soc_feature(unit, soc_feature_dual_hash) &&
                    (SOC_IS_RAVEN(unit) || SOC_IS_HAWKEYE(unit))) {
                   return _soc_mem_dual_hash_insert(unit, mem, copyno,
                                                    entry_data, NULL,
                              soc_dual_hash_recurse_depth_get(unit, mem));
                } else
#endif
            {
                return soc_fb_vlanmac_entry_ins(unit, entry_data);
            }
#ifdef BCM_TRIUMPH3_SUPPORT
            case EXT_L2_ENTRY_1m:
            case EXT_L2_ENTRY_2m:
               return soc_mem_generic_insert(unit, mem, copyno, -1,
                                              entry_data, NULL, NULL);
#endif /* BCM_TRIUMPH3_SUPPORT */
            default:
                break;
            }
        }
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_ISM_SUPPORT
        if (soc_feature(unit, soc_feature_ism_memory) && 
            soc_mem_is_mview(unit, mem)) {
            return _soc_mem_multi_hash_insert(unit, mem, copyno, entry_data, 
                       NULL, soc_multi_hash_recurse_depth_get(unit, mem));
        }  
#endif /* BCM_ISM_SUPPORT */

        MEM_LOCK(unit, mem);
        SOC_MEM_BLOCK_ITER(unit, mem, copyno) {
            if ((rv = _soc_mem_insert(unit, mem, copyno, entry_data)) < 0) {
                break;
            }
        }
    } else {
        assert(SOC_MEM_BLOCK_VALID(unit, mem, copyno));
        MEM_LOCK(unit, mem);
        rv = _soc_mem_insert(unit, mem, copyno, entry_data);
    }

    MEM_UNLOCK(unit, mem);

    return rv;
}

/*
 * Function:
 *    soc_mem_insert_return_old
 * Purpose:
 *      Same as soc_mem_insert() with the addtional feature
 *      that if an exiting entry was replaced, the old entry data
 *      is returned back.
 * Returns:
 *    SOC_E_NONE - insertion succeeded
 *    SOC_E_EXISTS - insertion succeeded, old_entry_data is valid
 *    SOC_E_FULL - table full
 *    SOC_E_UNAVAIL - not supported on the specified unit, mem
 *    SOC_E_XXX - other error
 */
int
soc_mem_insert_return_old(int unit,
                          soc_mem_t mem,
                          int copyno,
                          void *entry_data,
                          void *old_entry_data)
{
#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        _SOC_MEM_REPLACE_MEM(unit, mem);
        switch (mem) {
        case L2Xm: /* Note: the order of the case statements should be 
                            changed with care */
#if defined(INCLUDE_L3)
        case L3_ENTRY_ONLYm:
        case L3_ENTRY_IPV4_UNICASTm:
        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV6_UNICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
#endif /* INCLUDE_L3 */
#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_feature(unit, soc_feature_shared_hash_mem)) {
                return _soc_mem_shared_hash_insert(unit, mem, copyno, entry_data, 
                           NULL, soc_multi_hash_recurse_depth_get(unit, mem));
            } else
#endif /* BCM_TRIDENT2_SUPPORT */
            {
                return _soc_mem_dual_hash_insert(unit, mem, copyno,
                                                 entry_data, old_entry_data,
                           soc_dual_hash_recurse_depth_get(unit, mem));
            }
        case MPLS_ENTRYm: /* Note: the order of the case statements should be 
                                   changed with care */
            if (SOC_IS_SC_CQ(unit)) {
                break;
            }
        /* passthru */
        /* coverity[fallthrough: FALSE] */        
        case EGR_VLAN_XLATEm:
        case VLAN_XLATEm:
        case VLAN_MACm:
        case AXP_WRX_WCDm:
        case AXP_WRX_SVP_ASSIGNMENTm:
#ifdef BCM_TRIDENT2_SUPPORT
        case ING_VP_VLAN_MEMBERSHIPm:
        case EGR_VP_VLAN_MEMBERSHIPm:
        case ING_DNAT_ADDRESS_TYPEm:
        case L2_ENDPOINT_IDm:
        case ENDPOINT_QUEUE_MAPm:
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_ISM_SUPPORT
            if (soc_feature(unit, soc_feature_ism_memory) &&
                soc_mem_is_mview(unit, mem)) {
                return _soc_mem_multi_hash_insert(unit, mem, copyno,
                                                  entry_data, old_entry_data,
                           soc_multi_hash_recurse_depth_get(unit, mem));
            } else 
#endif /* BCM_ISM_SUPPORT */
            {
                return _soc_mem_dual_hash_insert(unit, mem, copyno,
                                                 entry_data, old_entry_data,
                           soc_dual_hash_recurse_depth_get(unit, mem));
            }
#ifdef BCM_TRIUMPH3_SUPPORT
        case EXT_L2_ENTRY_1m:
        case EXT_L2_ENTRY_2m:
            return soc_mem_generic_insert(unit, mem, copyno, -1,
                                          entry_data, old_entry_data, NULL);
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_ISM_SUPPORT
        case L2_ENTRY_1m:
        case L2_ENTRY_2m:
        case L3_ENTRY_1m:
        case L3_ENTRY_2m:
        case L3_ENTRY_4m:
        case VLAN_XLATE_EXTDm:
        case MPLS_ENTRY_EXTDm:
            return _soc_mem_multi_hash_insert(unit, mem, copyno,
                                              entry_data, old_entry_data,
                       soc_multi_hash_recurse_depth_get(unit, mem));
#endif /* BCM_ISM_SUPPORT */
            break;
        }
    }
#endif
    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *    _soc_mem_delete_index
 * Purpose:
 *    Helper function for soc_mem_delete and soc_mem_delete_index.
 */

STATIC int
_soc_mem_delete_index(int unit,
                      soc_mem_t mem,        /* Assumes memory locked */
                      int copyno,
                      int del_index)
{
    uint32        entry_tmp[SOC_MAX_MEM_WORDS];
    int            min, index, eot_index, rv;

    /* eot_index points one past last entry in use */

    if (soc_mem_is_cam(unit, mem)) {
        index = del_index;
    } else {
        min = soc_mem_index_min(unit, mem);
        eot_index = soc_mem_index_last(unit, mem, copyno) + 1;

        if (del_index < min || del_index >= eot_index) {
            return SOC_E_NOT_FOUND;
        }

        for (index = del_index; index + 1 < eot_index; index++) {
            /* Move down one entry */

            if ((rv = soc_mem_read(unit, mem, copyno,
                                   index + 1, entry_tmp)) < 0) {
                soc_cm_debug(DK_ERR,
                             "soc_mem_delete_index: "
                             "read %s.%s[%d] failed\n",
                             SOC_MEM_UFNAME(unit, mem),
                             SOC_BLOCK_NAME(unit, copyno), index + 1);
                return rv;
            }

            if ((rv = soc_mem_write(unit, mem, copyno,
                                    index, entry_tmp)) < 0) {
                soc_cm_debug(DK_ERR,
                             "soc_mem_delete_index: "
                             "write %s.%s[%d] failed\n",
                             SOC_MEM_UFNAME(unit, mem),
                             SOC_BLOCK_NAME(unit, copyno), index);
                return rv;
            }
        }
    }

    if ((rv = soc_mem_write(unit, mem, copyno, index,
                            soc_mem_entry_null(unit, mem))) < 0) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_delete_index: "
                     "write %s.%s[%d] failed\n",
                     SOC_MEM_UFNAME(unit, mem),
                     SOC_BLOCK_NAME(unit, copyno), index + 1);
        return rv;
    }
    _SOC_MEM_REUSE_MEM_STATE(unit, mem);
    SOP_MEM_STATE(unit, mem).count[copyno]--;

    return SOC_E_NONE;
}

/*
 * Function:
 *    soc_mem_delete_index
 * Purpose:
 *    Delete an entry from a sorted table based on index,
 *    moving up all successive entries.
 * Notes:
 *    If COPYNO_ALL is specified, the deletion is repeated for each
 *    copy of the table.
 *
 *    For deletes from the ARL, reads the entry and requests hardware
 *    deletion by key.  This shouldn't generally be done, because the
 *    hardware may change the ARL table at any time.
 */

int
soc_mem_delete_index(int unit,
                     soc_mem_t mem,
                     int copyno,
                     int index)
{
    int rv = SOC_E_NONE;

    assert(soc_mem_is_sorted(unit, mem) ||
           soc_mem_is_hashed(unit, mem) ||
           soc_mem_is_cam(unit, mem) ||
           soc_mem_is_cmd(unit, mem));

#ifdef BCM_TRX_SUPPORT
    if (soc_feature(unit, soc_feature_generic_table_ops)) {
        uint32 entry_tmp[SOC_MAX_MEM_WORDS];
        _SOC_MEM_REPLACE_MEM(unit, mem);
        switch (mem) {
        case L2Xm:
        case L3_ENTRY_IPV4_UNICASTm:
        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV6_UNICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
        case EGR_VLAN_XLATEm:
        case VLAN_XLATEm:
        case VLAN_MACm:
        case MPLS_ENTRYm:
        case AXP_WRX_WCDm:
        case AXP_WRX_SVP_ASSIGNMENTm:
#ifdef BCM_TRIDENT2_SUPPORT
        case ING_VP_VLAN_MEMBERSHIPm:
        case EGR_VP_VLAN_MEMBERSHIPm:
        case ING_DNAT_ADDRESS_TYPEm:
        case L2_ENDPOINT_IDm:
        case ENDPOINT_QUEUE_MAPm:
#endif /* BCM_TRIDENT2_SUPPORT */
        case EXT_L2_ENTRYm:
#ifdef BCM_ISM_SUPPORT
        case L2_ENTRY_1m:
        case L2_ENTRY_2m:
        case L3_ENTRY_1m:
        case L3_ENTRY_2m:
        case L3_ENTRY_4m:
        case VLAN_XLATE_EXTDm:
        case MPLS_ENTRY_EXTDm:
#endif /* BCM_ISM_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
        case EXT_L2_ENTRY_1m:
        case EXT_L2_ENTRY_2m:
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
        case EGR_MP_GROUPm:
#endif
            SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY,
                                             index, entry_tmp));
            return soc_mem_delete(unit, mem, copyno, entry_tmp);
        default:
            break;
        }
    }
#endif /* BCM_TRX_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FBX(unit)) {
        uint32 entry_tmp[SOC_MAX_MEM_WORDS];

        switch (mem) {
        case L3_ENTRY_IPV4_UNICASTm:
        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV6_UNICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
        case L2Xm:
        case VLAN_MACm:
            SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY,
                                             index, entry_tmp));
            return soc_mem_delete(unit, mem, copyno, entry_tmp);
        case L3_DEFIPm:
        default:
            return (SOC_E_UNAVAIL);
        }
    }
#endif /* BCM_FIREBOLT_SUPPORT */

    if (copyno != COPYNO_ALL){
        assert(SOC_MEM_BLOCK_VALID(unit, mem, copyno));
        MEM_LOCK(unit, mem);
        rv = _soc_mem_delete_index(unit, mem, copyno, index);
    }

    MEM_UNLOCK(unit, mem);

    return rv;
}

/*
 * Function:
 *    soc_mem_delete
 * Purpose:
 *    Delete an entry from a sorted table based on key,
 *    moving up all successive entries.
 * Returns:
 *    0 on success (whether or not entry was found),
 *    SOC_E_XXX on read or write error.
 * Notes:
 *    A binary search for the key is performed.  If the key is found,
 *    the entry is deleted by moving up successive entries until
 *    the last entry is moved up.  The previously last entry is
 *    overwritten with the null key.
 *
 *    If COPYNO_ALL is specified, the deletion is repeated for each copy
 *    of the table.
 */

int
soc_mem_delete(int unit,
               soc_mem_t mem,
               int copyno,
               void *key_data)
{
    int            index, rv;
    uint32        entry_tmp[SOC_MAX_MEM_WORDS];

    assert(soc_mem_is_sorted(unit, mem) ||
           soc_mem_is_hashed(unit, mem) ||
           soc_mem_is_cam(unit, mem) ||
           soc_mem_is_cmd(unit, mem));
    assert(key_data);

#ifdef BCM_TRX_SUPPORT
    if (soc_feature(unit, soc_feature_generic_table_ops)) {
        int32 banks = 0;
        _SOC_MEM_REPLACE_MEM(unit, mem);
        switch (mem) {
        case L2Xm:
        case L3_ENTRY_IPV4_UNICASTm:
        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV6_UNICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
#ifdef BCM_ISM_SUPPORT
        case L2_ENTRY_1m:
        case L2_ENTRY_2m:
        case L3_ENTRY_1m:
        case L3_ENTRY_2m:
        case L3_ENTRY_4m:
        case VLAN_XLATE_EXTDm:
        case MPLS_ENTRY_EXTDm:
#endif /* BCM_ISM_SUPPORT */
        case EGR_VLAN_XLATEm:
        case VLAN_XLATEm:
        case MPLS_ENTRYm:
        /* passthru */
        /* coverity[fallthrough: FALSE] */
#ifdef BCM_TRIUMPH3_SUPPORT
        case AXP_WRX_WCDm:
        case AXP_WRX_SVP_ASSIGNMENTm:
        case EXT_L2_ENTRY_1m:
        case EXT_L2_ENTRY_2m:
#endif /* BCM_TRIUMPH3_SUPPORT */
        case VLAN_MACm:
#ifdef BCM_TRIDENT2_SUPPORT
        case ING_VP_VLAN_MEMBERSHIPm:
        case EGR_VP_VLAN_MEMBERSHIPm:
        case ING_DNAT_ADDRESS_TYPEm:
        case L2_ENDPOINT_IDm:
        case ENDPOINT_QUEUE_MAPm:
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
        case EGR_MP_GROUPm:
#endif
        case EXT_L2_ENTRYm:
            if (soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) {
                banks = -1;
            }
            return soc_mem_generic_delete(unit, mem, MEM_BLOCK_ANY,
                                          banks, key_data, NULL, 0);
        default:
            break;
        }
    }
#endif /* BCM_TRX_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
    if (SOC_IS_FBX(unit)) {
        switch (mem) {
        case L2Xm:
            return soc_fb_l2x_delete(unit, key_data);
#if defined(INCLUDE_L3)
        case L3_DEFIPm:
            return soc_fb_lpm_delete(unit, key_data);
        case L3_ENTRY_IPV4_UNICASTm:
        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV6_UNICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
            return soc_fb_l3x_delete(unit, key_data);
#endif
        case VLAN_MACm:
            return soc_fb_vlanmac_entry_del(unit, key_data);
        default:
            break;
        }
    }
#endif /* BCM_FIREBOLT_SUPPORT */

    rv = SOC_E_NONE;
    if (copyno == COPYNO_ALL) {
        MEM_LOCK(unit, mem);
        SOC_MEM_BLOCK_ITER(unit, mem, copyno) {
            if ((rv = soc_mem_search(unit, mem, copyno,
                                     &index, key_data, entry_tmp, 0)) < 0) {
                break;
            }
            if ((rv = soc_mem_delete_index(unit, mem, copyno, index)) < 0) {
                break;
            }
        }
    } else {
        assert(SOC_MEM_BLOCK_VALID(unit, mem, copyno));
        MEM_LOCK(unit, mem);
        rv = soc_mem_search(unit, mem, copyno, &index, key_data, entry_tmp, 0);
        if (rv >= 0) {
            rv = soc_mem_delete_index(unit, mem, copyno, index);
        }
    }

    MEM_UNLOCK(unit, mem);

    return rv;
}

/*
 * Function:
 *      soc_mem_delete_return_old
 * Purpose:
 *      Same as soc_mem_delete() with the addtional feature
 *      that if an exiting entry was deleted, the old entry data
 *      is returned back.
 * Returns:
 *      SOC_E_NONE - delete succeeded
 *      SOC_E_UNAVAIL - not supported on the specified unit, mem
 *      SOC_E_XXX - other error
 */
int
soc_mem_delete_return_old(int unit,
                          soc_mem_t mem,
                          int copyno,
                          void *key_data,
                          void *old_entry_data)
{
#ifdef BCM_TRX_SUPPORT
    if (SOC_IS_TRX(unit)) {
        int32 banks = 0;
        _SOC_MEM_REPLACE_MEM(unit, mem);
        switch (mem) {
        case MPLS_ENTRYm: /* Note: the order of the case statements should 
                                   be changed with care */
            if (SOC_IS_SC_CQ(unit)) {
                break;
            }
        /* passthru */
        /* coverity[fallthrough: FALSE] */
        case L2Xm:
        case L3_ENTRY_ONLYm:
        case L3_ENTRY_IPV4_UNICASTm:
        case L3_ENTRY_IPV4_MULTICASTm:
        case L3_ENTRY_IPV6_UNICASTm:
        case L3_ENTRY_IPV6_MULTICASTm:
#ifdef BCM_ISM_SUPPORT
        case L2_ENTRY_1m:
        case L2_ENTRY_2m:
        case L3_ENTRY_1m:
        case L3_ENTRY_2m:
        case L3_ENTRY_4m:
        case VLAN_XLATE_EXTDm:
        case MPLS_ENTRY_EXTDm:
#endif /* BCM_ISM_SUPPORT */
        case EGR_VLAN_XLATEm:
        case VLAN_XLATEm:
        /* passthru */
        /* coverity[fallthrough: FALSE] */
#ifdef BCM_TRIUMPH3_SUPPORT
        case AXP_WRX_WCDm:
        case AXP_WRX_SVP_ASSIGNMENTm:
        case EXT_L2_ENTRY_1m:
        case EXT_L2_ENTRY_2m:
#endif /* BCM_TRIUMPH3_SUPPORT */
        case VLAN_MACm:
#ifdef BCM_TRIDENT2_SUPPORT
        case ING_VP_VLAN_MEMBERSHIPm:
        case EGR_VP_VLAN_MEMBERSHIPm:
        case ING_DNAT_ADDRESS_TYPEm:
        case L2_ENDPOINT_IDm:
        case ENDPOINT_QUEUE_MAPm:
#endif /* BCM_TRIDENT2_SUPPORT */
            if (soc_feature(unit, soc_feature_ism_memory) && soc_mem_is_mview(unit, mem)) {
                banks = -1;
            }
            return soc_mem_generic_delete(unit, mem, MEM_BLOCK_ANY,
                                          banks, key_data, old_entry_data, 0);
        }
    }
#endif
    return SOC_E_UNAVAIL;

}

#if defined(BCM_TRX_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
/*
 * Function:
 *    _soc_mem_pop
 * Purpose:
 *    Helper function for soc_mem_pop.
 */
STATIC int
_soc_mem_pop(int unit, soc_mem_t mem, int copyno, void *entry_data)
{
    schan_msg_t schan_msg;
    int         rv, entry_dw, allow_intr=0;
    uint8       at;

    assert(SOC_MEM_IS_VALID(unit, mem));
    assert(soc_attached(unit));

    if (copyno == MEM_BLOCK_ANY) {
        /* Overrun of "mem_block_any" is not possible as it is of MAX "mem" value NUM_SOC_MEM */
        /* coverity[overrun-local : FALSE] */
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }
    if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_pop: invalid block %d for memory %s\n",
                     copyno, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    entry_dw = soc_mem_entry_words(unit, mem);

    schan_msg_clear(&schan_msg);
    schan_msg.popcmd.header.opcode = FIFO_POP_CMD_MSG;
#ifdef BCM_EXTND_SBUS_SUPPORT
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        schan_msg.popcmd.header.srcblk =
            (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
    } else 
#endif
    {
        schan_msg.popcmd.header.srcblk = SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
    }
    schan_msg.popcmd.header.dstblk = SOC_BLOCK2SCH(unit, copyno);
    schan_msg.popcmd.header.cos = 0;
    schan_msg.popcmd.header.datalen = 0;

    schan_msg.gencmd.address = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);

#ifdef BCM_SIRIUS_SUPPORT
    schan_msg.popcmd.header.srcblk = (SOC_IS_SIRIUS(unit) ? 0 : 
                                     schan_msg.popcmd.header.srcblk);
    if (SOC_IS_SIRIUS(unit) && (!SAL_BOOT_PLISIM || SAL_BOOT_BCMSIM)) {
    /* mask off the block field */
    schan_msg.popcmd.address &= 0x3F0FFFFF;
    }
#endif

    if (SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /*
     * Execute S-Channel "fifo pop" command packet consisting of
     * (header word + address word), and read back
     * (header word + entry_dw) data words.
     */

    rv = soc_schan_op(unit, &schan_msg, 2, entry_dw + 1, allow_intr);

    /* Check result */

    if (schan_msg.popresp.header.opcode != FIFO_POP_DONE_MSG) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_pop: "
                     "invalid S-Channel reply, expected FIFO_POP_DONE_MSG:\n");
        soc_schan_dump(unit, &schan_msg, entry_dw + 2);
        return SOC_E_INTERNAL;
    }

    if (rv == SOC_E_NONE) {
        if (schan_msg.popresp.header.cpu) {
            rv = SOC_E_NOT_FOUND;
        } else {
            sal_memcpy(entry_data, schan_msg.popresp.data, entry_dw * 4);
        }
    }

    if (soc_cm_debug_check(DK_SOCMEM)) {
        soc_cm_debug(DK_SOCMEM, "Fifo pop: ");
        soc_mem_entry_dump(unit, mem, entry_data);
    }

    return rv;
}


/*
 * Function:
 *    _soc_mem_push
 * Purpose:
 *    Helper function for soc_mem_push.
 */

STATIC int
_soc_mem_push(int unit, soc_mem_t mem, int copyno, void *entry_data)
{
    schan_msg_t schan_msg;
    int         rv, entry_dw, allow_intr=0;
    uint8       at;

    assert(SOC_MEM_IS_VALID(unit, mem));
    assert(soc_attached(unit));

    if (copyno == MEM_BLOCK_ANY) {
        /* Overrun of "mem_block_any" is not possible as it is of MAX "mem" value NUM_SOC_MEM */
        /* coverity[overrun-local : FALSE] */
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }
    if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_push: invalid block %d for memory %s\n",
                     copyno, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    entry_dw = soc_mem_entry_words(unit, mem);

    schan_msg_clear(&schan_msg);
    schan_msg.pushcmd.header.opcode = FIFO_PUSH_CMD_MSG;
#ifdef BCM_EXTND_SBUS_SUPPORT
    if (soc_feature(unit, soc_feature_new_sbus_format)) {
        schan_msg.pushcmd.header.srcblk =
            (soc_mem_flags(unit, mem) >> SOC_MEM_FLAG_ACCSHIFT) & 7;
    } else 
#endif
    {
        schan_msg.pushcmd.header.srcblk = SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
    }
    schan_msg.pushcmd.header.dstblk = SOC_BLOCK2SCH(unit, copyno);
    schan_msg.pushcmd.header.cos = 0;
    schan_msg.pushcmd.header.datalen = entry_dw * 4;

    schan_msg.gencmd.address = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);
 
    /* Fill in packet data */
    sal_memcpy(schan_msg.pushcmd.data, entry_data, entry_dw * 4);

    if (SOC_IS_SAND(unit)) {
        allow_intr = 1;
    }

    /*
     * Execute S-Channel "fifo push" command packet consisting of
     * (header word + address word + entry_dw), and read back
     * (header word + entry_dw) data words.
     */

    rv = soc_schan_op(unit, &schan_msg, entry_dw + 2, entry_dw + 1, allow_intr);

    /* Check result */

    if (schan_msg.pushresp.header.opcode != FIFO_PUSH_DONE_MSG) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_push: "
                     "invalid S-Channel reply, expected FIFO_PUSH_DONE_MSG:\n");
        soc_schan_dump(unit, &schan_msg, entry_dw + 2);
        return SOC_E_INTERNAL;
    }

    if ((rv == SOC_E_NONE) && (schan_msg.pushresp.header.cpu)) {
        rv = SOC_E_FULL;
    }

    if (soc_cm_debug_check(DK_SOCMEM)) {
        soc_cm_debug(DK_SOCMEM, "Fifo push: ");
        soc_mem_entry_dump(unit, mem, entry_data);
    }

    return rv;
}

static const struct {
    soc_reg_t   cfg;
    soc_reg_t   sbus_addr;
    soc_reg_t   hostmem_addr;
    soc_reg_t   read_ptr;
    soc_reg_t   write_ptr;
    soc_reg_t   threshold;
    soc_field_t overflow_fld;
    int         overflow_bit;
} _soc_mem_fifo_dma[] = {
    {
        CMIC_FIFO_CH0_RD_DMA_CFGr,
        CMIC_FIFO_CH0_RD_DMA_SBUS_START_ADDRESSr,
        CMIC_FIFO_CH0_RD_DMA_HOSTMEM_START_ADDRESSr,
        CMIC_FIFO_CH0_RD_DMA_HOSTMEM_READ_PTRr,
        CMIC_FIFO_CH0_RD_DMA_HOSTMEM_WRITE_PTRr,
        CMIC_FIFO_CH0_RD_DMA_HOSTMEM_THRESHOLDr,
        FIFO_CH0_DMA_HOSTMEM_OVERFLOWf,
        0,
    },
    {
        CMIC_FIFO_CH1_RD_DMA_CFGr,
        CMIC_FIFO_CH1_RD_DMA_SBUS_START_ADDRESSr,
        CMIC_FIFO_CH1_RD_DMA_HOSTMEM_START_ADDRESSr,
        CMIC_FIFO_CH1_RD_DMA_HOSTMEM_READ_PTRr,
        CMIC_FIFO_CH1_RD_DMA_HOSTMEM_WRITE_PTRr,
        CMIC_FIFO_CH1_RD_DMA_HOSTMEM_THRESHOLDr,
        FIFO_CH1_DMA_HOSTMEM_OVERFLOWf,
        2,
    },
    {
        CMIC_FIFO_CH2_RD_DMA_CFGr,
        CMIC_FIFO_CH2_RD_DMA_SBUS_START_ADDRESSr,
        CMIC_FIFO_CH2_RD_DMA_HOSTMEM_START_ADDRESSr,
        CMIC_FIFO_CH2_RD_DMA_HOSTMEM_READ_PTRr,
        CMIC_FIFO_CH2_RD_DMA_HOSTMEM_WRITE_PTRr,
        CMIC_FIFO_CH2_RD_DMA_HOSTMEM_THRESHOLDr,
        FIFO_CH2_DMA_HOSTMEM_OVERFLOWf,
        4,
    },
    {
        CMIC_FIFO_CH3_RD_DMA_CFGr,
        CMIC_FIFO_CH3_RD_DMA_SBUS_START_ADDRESSr,
        CMIC_FIFO_CH3_RD_DMA_HOSTMEM_START_ADDRESSr,
        CMIC_FIFO_CH3_RD_DMA_HOSTMEM_READ_PTRr,
        CMIC_FIFO_CH3_RD_DMA_HOSTMEM_WRITE_PTRr,
        CMIC_FIFO_CH3_RD_DMA_HOSTMEM_THRESHOLDr,
        FIFO_CH3_DMA_HOSTMEM_OVERFLOWf,
        6,
    },
};
STATIC int
_soc_mem_fifo_dma_start(int unit, int chan, soc_mem_t mem, int copyno,
                        int host_entries, void *host_buf)
{
    soc_control_t  *soc = SOC_CONTROL(unit);
    soc_reg_t cfg_reg;
    uint32 addr, rval, data_beats, sel, spacing;
    uint8 at;

    if (chan < 0 || chan > 3 || host_buf == NULL) {
        return SOC_E_PARAM;
    }

    cfg_reg = _soc_mem_fifo_dma[chan].cfg;

    switch (host_entries) {
    case 64:    sel = 0; break;
    case 128:   sel = 1; break;
    case 256:   sel = 2; break;
    case 512:   sel = 3; break;
    case 1024:  sel = 4; break;
    case 2048:  sel = 5; break;
    case 4096:  sel = 6; break;
    case 8192:  sel = 7; break;
    case 16384: sel = 8; break;
    case 32768: sel = 9; break;
    case 65536: sel = 10; break;
    default:
        return SOC_E_PARAM;
    }

    if (mem != ING_IPFIX_EXPORT_FIFOm && mem != EGR_IPFIX_EXPORT_FIFOm &&
        mem != EXT_L2_MOD_FIFOm && mem != L2_MOD_FIFOm &&
        mem != CS_EJECTION_MESSAGE_TABLEm) {
        return SOC_E_BADID;
    }

    if (copyno == MEM_BLOCK_ANY) {
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }

    data_beats = soc_mem_entry_words(unit, mem);

    addr = soc_reg_addr(unit, _soc_mem_fifo_dma[chan].sbus_addr, REG_PORT_ANY,
                        0);
    rval = soc_mem_addr_get(unit, mem, 0, copyno, 0, &at);
    soc_pci_write(unit, addr, rval);

    addr = soc_reg_addr(unit, _soc_mem_fifo_dma[chan].hostmem_addr,
                        REG_PORT_ANY, 0);
    rval = soc_cm_l2p(unit, host_buf);
    soc_pci_write(unit, addr, rval);

    addr = soc_reg_addr(unit, _soc_mem_fifo_dma[chan].read_ptr, REG_PORT_ANY,
                        0);
    soc_pci_write(unit, addr, rval);

    addr = soc_reg_addr(unit, _soc_mem_fifo_dma[chan].threshold, REG_PORT_ANY,
                        0);
    rval = 0;
    soc_reg_field_set(unit, _soc_mem_fifo_dma[chan].threshold, &rval, ADDRESSf,
                      host_entries / 16 * data_beats * sizeof(uint32));
    soc_pci_write(unit, addr, rval);

    addr = soc_reg_addr(unit, cfg_reg, REG_PORT_ANY, 0);
    rval = 0;
    soc_reg_field_set(unit, cfg_reg, &rval, BEAT_COUNTf, data_beats);
    soc_reg_field_set(unit, cfg_reg, &rval, HOST_NUM_ENTRIES_SELf, sel);
    soc_reg_field_set(unit, cfg_reg, &rval, TIMEOUT_COUNTf, 200);
    if (soc_feature(unit, soc_feature_multi_sbus_cmds)) {
        
        if (soc->sbusCmdSpacing < 0) {
            spacing = data_beats > 7 ? data_beats + 1 : 8;
        } else {
            spacing = soc->sbusCmdSpacing;
        }
        if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_XQPORT) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_GXPORT) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_SPORT) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_GPORT)) {
            spacing = 0;
        }
        if (spacing) {
            soc_reg_field_set(unit, cfg_reg, &rval,
                              MULTIPLE_SBUS_CMD_SPACINGf, spacing);
            soc_reg_field_set(unit, cfg_reg, &rval,
                              ENABLE_MULTIPLE_SBUS_CMDSf, 1);
        }
    }
    soc_pci_write(unit, addr, rval);

    soc_reg_field_set(unit, cfg_reg, &rval, ENABLEf, 1);
    soc_reg_field_set(unit, cfg_reg, &rval, ENABLE_VALf, 1);
    soc_pci_write(unit, addr, rval);

    return SOC_E_NONE;
}

STATIC int
_soc_mem_fifo_dma_stop(int unit, int chan)
{
    uint32 addr, rval;

    if (chan < 0 || chan > 3) {
        return SOC_E_PARAM;
    }

    addr = soc_reg_addr(unit, _soc_mem_fifo_dma[chan].cfg, REG_PORT_ANY, 0);
    rval = 0;
    soc_reg_field_set(unit, _soc_mem_fifo_dma[chan].cfg, &rval, ENABLEf, 1);
    soc_pci_write(unit, addr, rval);

    return SOC_E_NONE;
}

STATIC int
_soc_mem_fifo_dma_get_read_ptr(int unit, int chan, void **host_ptr, int *count)
{
    soc_reg_t cfg_reg;
    int host_entries, data_beats;
    uint32 addr, rval, debug, hostmem_addr, read_ptr, write_ptr;

    if (chan < 0 || chan > 3 || host_ptr == NULL) {
        return SOC_E_PARAM;
    }

    /* read CFG reg first so that write pointer is valid later on */
    cfg_reg = _soc_mem_fifo_dma[chan].cfg;
    addr = soc_reg_addr(unit, cfg_reg, REG_PORT_ANY, 0);
    soc_pci_getreg(unit, addr, &rval);

    addr = soc_reg_addr(unit, _soc_mem_fifo_dma[chan].read_ptr, REG_PORT_ANY,
                        0);
    soc_pci_getreg(unit, addr, &read_ptr);

    addr = soc_reg_addr(unit, _soc_mem_fifo_dma[chan].write_ptr, REG_PORT_ANY,
                        0);
    soc_pci_getreg(unit, addr, &write_ptr);

    if (write_ptr == 0 || 
        (soc_feature(unit, soc_feature_fifo_dma_active) &&
         !soc_reg_field_get(unit, cfg_reg, rval, ACTIVEf))) {
        return SOC_E_EMPTY;
    }

    if (read_ptr == write_ptr) {
        addr = soc_reg_addr(unit, CMIC_FIFO_RD_DMA_DEBUGr, REG_PORT_ANY, 0);
        soc_pci_getreg(unit, addr, &debug);
        if (!soc_reg_field_get(unit, CMIC_FIFO_RD_DMA_DEBUGr,
                               debug, _soc_mem_fifo_dma[chan].overflow_fld)) {
            return SOC_E_EMPTY;
        }
        /* Re-read write pointer */
        addr = soc_reg_addr(unit, _soc_mem_fifo_dma[chan].write_ptr,
                            REG_PORT_ANY, 0);
        soc_pci_getreg(unit, addr, &write_ptr);
    }

    addr = soc_reg_addr(unit, _soc_mem_fifo_dma[chan].hostmem_addr,
                        REG_PORT_ANY, 0);
    soc_pci_getreg(unit, addr, &hostmem_addr);

    data_beats = soc_reg_field_get(unit, cfg_reg, rval, BEAT_COUNTf);
    if (data_beats <= 0) {
        soc_cm_debug(DK_ERR, "Invalid BEAT_COUNT (%d) in "
                     "CMIC_FIFO_CH%d_RD_DMA_CFG \n", data_beats, chan);
        return SOC_E_CONFIG;
    }

    switch (soc_reg_field_get(unit, cfg_reg, rval, HOST_NUM_ENTRIES_SELf)) {
    case 0:  host_entries = 64;    break;
    case 1:  host_entries = 128;   break;
    case 2:  host_entries = 256;   break;
    case 3:  host_entries = 512;   break;
    case 4:  host_entries = 1024;  break;
    case 5:  host_entries = 2048;  break;
    case 6:  host_entries = 4096;  break;
    case 7:  host_entries = 8192;  break;
    case 8:  host_entries = 16384; break;
    case 9:  host_entries = 32768; break;
    case 10: host_entries = 65536; break;
    default: return SOC_E_CONFIG;
    }

    *host_ptr = soc_cm_p2l(unit, read_ptr);
    if (read_ptr >= write_ptr) {
        *count = host_entries -
            (read_ptr - hostmem_addr) / data_beats / sizeof(uint32);
    } else {
        *count = (write_ptr - read_ptr) / data_beats / sizeof(uint32);
    }

    if (SAL_BOOT_QUICKTURN) {
        /* Delay to ensure PCI DMA tranfer to host memory completes */
        sal_usleep(soc_mem_fifo_delay_value);
    }
    return (*count) ? SOC_E_NONE : SOC_E_EMPTY;
}

STATIC int
_soc_mem_fifo_dma_advance_read_ptr(int unit, int chan, int count)
{
    soc_reg_t cfg_reg;
    int host_entries, data_beats;
    uint32 addr, rval, debug;
    uint32 *host_buf, *read_ptr;

    if (chan < 0 || chan > 3) {
        return SOC_E_PARAM;
    }

    cfg_reg = _soc_mem_fifo_dma[chan].cfg;

    addr = soc_reg_addr(unit, cfg_reg, REG_PORT_ANY, 0);
    soc_pci_getreg(unit, addr, &rval);
    data_beats = soc_reg_field_get(unit, cfg_reg, rval, BEAT_COUNTf);

    switch (soc_reg_field_get(unit, cfg_reg, rval, HOST_NUM_ENTRIES_SELf)) {
    case 0:  host_entries = 64;    break;
    case 1:  host_entries = 128;   break;
    case 2:  host_entries = 256;   break;
    case 3:  host_entries = 512;   break;
    case 4:  host_entries = 1024;  break;
    case 5:  host_entries = 2048;  break;
    case 6:  host_entries = 4096;  break;
    case 7:  host_entries = 8192;  break;
    case 8:  host_entries = 16384; break;
    case 9:  host_entries = 32768; break;
    case 10: host_entries = 65536; break;
    default: return SOC_E_CONFIG;
    }

    if (count < 0 || count >= host_entries) {
        return SOC_E_PARAM;
    }

    /* Clear threshold overflow bit */
    addr = soc_reg_addr(unit, CMIC_FIFO_RD_DMA_DEBUGr, REG_PORT_ANY, 0);
    debug = 0;
    soc_reg_field_set(unit, CMIC_FIFO_RD_DMA_DEBUGr,
                      &debug, BIT_POSf, _soc_mem_fifo_dma[chan].overflow_bit);
    soc_pci_write(unit, addr, debug);

    addr = soc_reg_addr(unit, _soc_mem_fifo_dma[chan].hostmem_addr,
                        REG_PORT_ANY, 0);
    soc_pci_getreg(unit, addr, &rval);
    host_buf = soc_cm_p2l(unit, rval);

    addr = soc_reg_addr(unit, _soc_mem_fifo_dma[chan].read_ptr, REG_PORT_ANY,
                        0);
    soc_pci_getreg(unit, addr, &rval);
    read_ptr = soc_cm_p2l(unit, rval);

    read_ptr += count * data_beats;
    if (read_ptr >= &host_buf[host_entries * data_beats]) {
        read_ptr -= host_entries * data_beats;
    }
    rval = soc_cm_l2p(unit, read_ptr);
    soc_pci_write(unit, addr, rval);

    return SOC_E_NONE;
}

#endif /* BCM_TRX_SUPPORT || BCM_SIRIUS_SUPPORT*/

/*
 * Function:
 *      soc_mem_pop
 * Purpose:
 *      Pop an entry from a FIFO.
 * Parameters:
 *      mem               Memory to search
 *      copyno               Which copy to search (if multiple copies)
 *      entry_data              OUT: Data if found
 *                           CAN NO LONGER BE NULL.  Must be big enough
 *                           to handle the appropriate data.
 * Returns:
 *    SOC_E_NOT_FOUND      Entry not found: Fifo is empty
 *    SOC_E_NONE         Entry is found
 *    SOC_E_XXX         If internal error occurs
 */

int
soc_mem_pop(int unit,
            soc_mem_t mem,
            int copyno,
            void *entry_data)
{
#ifdef BCM_TRX_SUPPORT
    if (soc_feature(unit, soc_feature_mem_push_pop)) {
        switch (mem) {
        case ING_IPFIX_EXPORT_FIFOm:
        case EGR_IPFIX_EXPORT_FIFOm:
        case EXT_L2_MOD_FIFOm:
        case L2_MOD_FIFOm:
#ifdef BCM_TRIUMPH3_SUPPORT
        case ING_SER_FIFOm:
        case ING_SER_FIFO_Xm:
        case ING_SER_FIFO_Ym:
        case EGR_SER_FIFOm:
        case EGR_SER_FIFO_Xm:
        case EGR_SER_FIFO_Ym:
        case ISM_SER_FIFOm:
#endif /* BCM_TRIUMPH3_SUPPORT */
            return _soc_mem_pop(unit, mem, copyno, entry_data);
        default:
            return SOC_E_BADID;
            break;
        }
    }
#endif /* BCM_TRX_SUPPORT */

#ifdef BCM_SIRIUS_SUPPORT
    
    if (soc_feature(unit, soc_feature_mem_push_pop)) {
        switch (mem) {
        /* return _soc_mem_pop(unit, mem, copyno, entry_data); */
            default:
        soc_cm_debug(DK_ERR,
                 "soc_mem_pop: implement mem POP\n");
                return SOC_E_BADID;
                break;
        }
    }
#endif /* BCM_SIRIUS_SUPPORT */

    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *      soc_mem_push
 * Purpose:
 *      Push an entry to a FIFO.
 * Parameters:
 *      mem                  Memory to search
 *      copyno               Which copy to search (if multiple copies)
 *      entry_data           In: Data to be pushed
 *
 * Returns:
 *      SOC_E_FULL           Fifo is full
 *      SOC_E_NONE           Entry is pushed successfully
 *      SOC_E_XXX            If internal error occurs
 */

int
soc_mem_push(int unit,
             soc_mem_t mem,
             int copyno,
             void *entry_data)
{
#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
        soc_cm_debug(DK_ERR,
                     "soc_mem_push: not supported on %s\n",
                     SOC_CHIP_STRING(unit));
    return SOC_E_UNAVAIL;
    }
#endif

#ifdef BCM_TRX_SUPPORT
    if (soc_feature(unit, soc_feature_mem_push_pop)) {
        switch (mem) {
        case ING_IPFIX_EXPORT_FIFOm:
        case EGR_IPFIX_EXPORT_FIFOm:
        case EXT_L2_MOD_FIFOm:
        case L2_MOD_FIFOm:
            return _soc_mem_push(unit, mem, copyno, entry_data);
        default:
            return SOC_E_BADID;
            break;
        }
    }
#endif /* BCM_TRX_SUPPORT */

    return SOC_E_UNAVAIL;
}

int
soc_mem_fifo_dma_start(int unit, int chan, soc_mem_t mem, int copyno,
                       int host_entries, void *host_buf)
{
#if defined(BCM_KATANA_SUPPORT) && defined(BCM_CMICM_SUPPORT)
    if (SOC_IS_KATANA(unit) && soc_feature(unit, soc_feature_cmicm)) {
        return _soc_mem_kt_fifo_dma_start(unit, chan, mem, copyno,
                                          host_entries, host_buf);
    }
#endif
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        return _soc_mem_sbus_fifo_dma_start(unit, chan, mem, copyno,
                                            host_entries, host_buf);
    }
#endif /* BCM_CMICM_SUPPORT */
#if defined(BCM_TRX_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (soc_feature(unit, soc_feature_fifo_dma)) {
        return _soc_mem_fifo_dma_start(unit, chan, mem, copyno,
                                       host_entries, host_buf);
    }
#endif /* BCM_TRX_SUPPORT || BCM_SIRIUS_SUPPORT */

    return SOC_E_UNAVAIL;
}

int
soc_mem_fifo_dma_stop(int unit, int chan)
{
#if defined(BCM_KATANA_SUPPORT) && defined(BCM_CMICM_SUPPORT)
    if (SOC_IS_KATANA(unit) && soc_feature(unit, soc_feature_cmicm)) {
        return _soc_mem_kt_fifo_dma_stop(unit, chan);
    }
#endif
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        return _soc_mem_sbus_fifo_dma_stop(unit, chan);
    }
#endif /* BCM_CMICM_SUPPORT */
#if defined(BCM_TRX_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (soc_feature(unit, soc_feature_fifo_dma)) {
        return _soc_mem_fifo_dma_stop(unit, chan);
    }
#endif /* BCM_TRX_SUPPORT || BCM_SIRIUS_SUPPORT */

    return SOC_E_UNAVAIL;
}

int
soc_mem_fifo_dma_get_read_ptr(int unit, int chan, void **host_ptr, int *count)
{

#if defined(BCM_HURRICANE2_SUPPORT) && defined(BCM_CMICM_SUPPORT)
    if (soc_feature(unit, soc_feature_cmicm) && SOC_IS_HURRICANE2(unit)) {
        return _soc_mem_hu2_fifo_dma_get_read_ptr(unit, chan, host_ptr, count);
    }
#endif
#if defined(BCM_KATANA_SUPPORT) && defined(BCM_CMICM_SUPPORT)
    if (soc_feature(unit, soc_feature_cmicm)) {
        return _soc_mem_kt_fifo_dma_get_read_ptr(unit, chan, host_ptr, count);
    }
#endif
#if defined(BCM_TRX_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (soc_feature(unit, soc_feature_fifo_dma)) {
        return _soc_mem_fifo_dma_get_read_ptr(unit, chan, host_ptr, count);
    }
#endif /* BCM_TRX_SUPPORT || BCM_SIRIUS_SUPPORT */

    return SOC_E_UNAVAIL;
}

int
soc_mem_fifo_dma_advance_read_ptr(int unit, int chan, int count)
{

#if defined(BCM_HURRICANE2_SUPPORT) && defined(BCM_CMICM_SUPPORT)
    if (soc_feature(unit, soc_feature_cmicm) && SOC_IS_HURRICANE2(unit)) {
        return _soc_mem_hu2_fifo_dma_advance_read_ptr(unit, chan, count);
    }
#endif
#if defined(BCM_KATANA_SUPPORT) && defined(BCM_CMICM_SUPPORT)
    if (soc_feature(unit, soc_feature_cmicm)) {
        return _soc_mem_kt_fifo_dma_advance_read_ptr(unit, chan, count);
    }
#endif
#if defined(BCM_TRX_SUPPORT) || defined(BCM_SIRIUS_SUPPORT)
    if (soc_feature(unit, soc_feature_fifo_dma)) {
        return _soc_mem_fifo_dma_advance_read_ptr(unit, chan, count);
    }
#endif /* BCM_TRX_SUPPORT || BCM_SIRIUS_SUPPORT */

    return SOC_E_UNAVAIL;
}

int 
soc_mem_fifo_dma_get_num_entries(int unit, int chan, int *count)
{
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        return _soc_mem_sbus_fifo_dma_get_num_entries(unit, chan, count);
    }
#endif /* BCM_CMICM_SUPPORT */
    return SOC_E_UNAVAIL;
}

int 
soc_mem_fifo_dma_set_entries_read(int unit, int chan, uint32 num)
{
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        return _soc_mem_sbus_fifo_dma_set_entries_read(unit, chan, num);
    }
#endif /* BCM_CMICM_SUPPORT */
    return SOC_E_UNAVAIL;
}

#ifdef BCM_XGS_SWITCH_SUPPORT

/*
 * Function:
 *    soc_vlan_entries
 * Purpose:
 *    Return the number of entries in VLAN_TAB with the VALID bit set.
 */

STATIC int
soc_vlan_entries(int unit)
{
    int            index_min, index_max, index;
    vlan_tab_entry_t    ve;
    int            total;

    index_min = soc_mem_index_min(unit, VLAN_TABm);
    index_max = soc_mem_index_max(unit, VLAN_TABm);

    total = 0;

    for (index = index_min; index <= index_max; index++) {
        SOC_IF_ERROR_RETURN
            (soc_mem_read(unit, VLAN_TABm, MEM_BLOCK_ANY, index, &ve));

        total += soc_VLAN_TABm_field32_get(unit, &ve, VALIDf);
    }

    return total;
}

#endif /* BCM_XGS_SWITCH_SUPPORT */

/*
 * Function:
 *    soc_mem_entries
 * Purpose:
 *    Return the number of entries in a sorted table.
 * Notes:
 *    Performs special functions for some tables.
 */

int
soc_mem_entries(int unit,
                soc_mem_t mem,
                int copyno)
{
    int            entries = 0;
    assert(SOC_MEM_IS_VALID(unit, mem));
    assert(soc_attached(unit));
    assert(soc_mem_is_sorted(unit, mem) ||
           soc_mem_is_hashed(unit, mem) ||
           soc_mem_is_cam(unit, mem) ||
           soc_mem_is_cmd(unit, mem) ||
           mem == VLAN_TABm);
    if (copyno == MEM_BLOCK_ANY) {
        /* Overrun of "mem_block_any" is not possible as it is of MAX "mem" value NUM_SOC_MEM */
        /* coverity[overrun-local : FALSE] */ 
        copyno = SOC_MEM_BLOCK_ANY(unit, mem);
    }
    if (!SOC_MEM_BLOCK_VALID(unit, mem, copyno)) {
        soc_cm_debug(DK_WARN,
                     "soc_mem_entries: invalid block %d for memory %s\n",
                     copyno, SOC_MEM_NAME(unit, mem));
        return SOC_E_PARAM;
    }

    switch (mem) {
#ifdef BCM_XGS_SWITCH_SUPPORT
    case VLAN_TABm:
        entries = soc_vlan_entries(unit);    /* Warning: very slow */
        break;
#endif

    default:
        _SOC_MEM_REUSE_MEM_STATE(unit, mem);
        entries = SOP_MEM_STATE(unit, mem).count[copyno];
        break;
    }

    return entries;
}

/*
 * Function:
 *    soc_mem_debug_set
 * Purpose:
 *    Enable or disable MMU debug mode.
 * Returns:
 *    Previous enable state, or SOC_E_XXX on error.
 */

int
soc_mem_debug_set(int unit, int enable)
{
    schan_msg_t schan_msg;
    int    msg, rv = SOC_E_NONE, old_enable, allow_intr=0;

    old_enable = SOC_PERSIST(unit)->debugMode;

    if (enable && !old_enable) {
        msg = ENTER_DEBUG_MODE_MSG;
    } else if (!enable && old_enable) {
        msg = EXIT_DEBUG_MODE_MSG;
    } else {
        msg = -1;
    }

    if (msg >= 0) {
        schan_msg_clear(&schan_msg);
        schan_msg.header.opcode = msg;
        schan_msg.header.dstblk = SOC_BLOCK2SCH(unit, MMU_BLOCK(unit));
        schan_msg.header.srcblk = SOC_BLOCK2SCH(unit, CMIC_BLOCK(unit));
#if defined(BCM_SHADOW_SUPPORT)
    /* schan_msg.gencmd.header.srcblk = 0;*/
#endif

#ifdef BCM_SIRIUS_SUPPORT
    schan_msg.header.srcblk = (SOC_IS_SIRIUS(unit) ? 0 : schan_msg.header.srcblk);
#endif

        if (SOC_IS_SAND(unit)) {
            allow_intr = 1;
        }

        if ((rv = soc_schan_op(unit, &schan_msg, 1, 0, allow_intr)) >= 0) {
            SOC_PERSIST(unit)->debugMode = enable;
        }

        if (!enable) {
            /* Allow packet transfers in progress to drain */
            sal_usleep(100000);
        }
    }

    return (rv < 0) ? rv : old_enable;
}

/*
 * Function:
 *    soc_mem_debug_get
 * Purpose:
 *    Return current MMU debug mode status
 */

int
soc_mem_debug_get(int unit, int *enable)
{
    *enable = SOC_PERSIST(unit)->debugMode;
    return SOC_E_NONE;
}

/*
 * Function:
 *    soc_mem_iterate
 * Purpose:
 *    Iterate over all the valid memories and call user
 *    passed callback function (do_it) for each valid memory.
 */
int
soc_mem_iterate(int unit, soc_mem_iter_f do_it, void *data)
{
    soc_mem_t mem, mem_iter;
    int       rv = SOC_E_NONE;

    if (!do_it) {
        SOC_ERROR_PRINT((DK_ERR,
                         "soc_mem_iterate: Callback function is NULL"));
        return SOC_E_PARAM;
    }

    for (mem_iter = 0; mem_iter < NUM_SOC_MEM; mem_iter++) {
        mem = mem_iter;
        _SOC_MEM_REPLACE_MEM(unit, mem);
        if (!SOC_MEM_IS_VALID(unit, mem)) {
            continue;
        }

#if defined(BCM_HAWKEYE_SUPPORT)
        if (SOC_IS_HAWKEYE(unit) && (soc_mem_index_max(unit, mem) <= 0)) {
            continue;
        }
#endif /* BCM_HAWKEYE_SUPPORT */

        /*
         * Call user provided callback function.
         */
        if ((rv = do_it(unit, mem, data)) < 0) {
            SOC_ERROR_PRINT((DK_ERR,
                         "soc_mem_iterate: Failed on memory (%s)\n",
                         SOC_MEM_NAME(unit, mem)));
           /* break; */
        }
    }
    return rv;
}

#ifdef BCM_CMICM_SUPPORT
/*
 * Function:   soc_host_ccm_copy
 * Purpose:    Copy a range of memory from one host location to aother
 *                 Primarily for Cross-Coupled Memories.
 * Parameters:
 *       unit      - (IN) SOC unit number.
 *       srcbuf   - (IN) Source Buffer.
 *       dstbuf   - (IN) Destiation Buffer.
 *       count    - (IN) Number of dwords to transfer.
 *       flags    - (IN) 
 *                  0x01  Source and destination endian to be changed. 
 *                  0x02  Source is PCI
 * Returns:
 *       SOC_E_XXX
 */
int
soc_host_ccm_copy(int unit, void *srcbuf, void *dstbuf,
                  int count, int flags)
{
    int i, rv;
    soc_control_t *soc = SOC_CONTROL(unit);
    uint32 *srcptr = (uint32 *)srcbuf;
    uint32 *dstptr = (uint32 *)dstbuf;
    int cmc = SOC_PCI_CMC(unit);
    uint32 reg, reg2;
    assert (srcptr && dstptr);

    if (SOC_CONTROL(unit)->ccmDmaMutex == 0) {
        /* If DMA not enabled, or short length use PIO to copy */
        /*soc_cm_debug(DK_WARN, "using PIO mode for CCM copy\n");*/
        for (i = 0; i < count; i ++ ) {
            if (flags & 2) {            /* Read from PCI */
                reg = soc_pci_mcs_read(unit, PTR_TO_INT(srcptr));
            } else {
                reg = *srcptr;
            }
            
            if (flags & 1) {
                reg = ((reg & 0xff000000) >> 24) |
                    ((reg & 0x00ff0000) >> 8) |
                    ((reg & 0x0000ff00) << 8) |
                    ((reg & 0x000000ff) << 24);
            }

            if (flags & 2) {
                *dstptr = reg;
            } else {
                soc_pci_mcs_write(unit, PTR_TO_INT(dstptr), reg);
                reg2 = soc_pci_mcs_read(unit, PTR_TO_INT(dstptr));
                if (reg2 != reg) {
                    soc_cm_debug(DK_ERR, "ccm_dma: compare error %x (%x %x)\n",
                                 PTR_TO_INT(dstptr), reg, reg2);
                }
            }
            
            dstptr++;
            srcptr++;
        }
        return SOC_E_NONE;
    }

    CCM_DMA_LOCK(unit);
    
    soc_pci_write(unit, CMIC_CMCx_CCM_DMA_HOST0_MEM_START_ADDR_OFFSET(cmc), soc_cm_l2p(unit, srcbuf));
    soc_pci_write(unit, CMIC_CMCx_CCM_DMA_HOST1_MEM_START_ADDR_OFFSET(cmc), soc_cm_l2p(unit, dstbuf));
    soc_pci_write(unit, CMIC_CMCx_CCM_DMA_ENTRY_COUNT_OFFSET(cmc), count);
    /* Keep endianess default... */
    reg = soc_pci_read(unit, CMIC_CMCx_CCM_DMA_CFG_OFFSET(cmc));
    soc_reg_field_set(unit, CMIC_CMC0_CCM_DMA_CFGr, &reg, ABORTf, 0);
    soc_reg_field_set(unit, CMIC_CMC0_CCM_DMA_CFGr, &reg, ENf, 0);
    soc_pci_write(unit, CMIC_CMCx_CCM_DMA_CFG_OFFSET(cmc), reg);  /* Clearing EN clears stats */
    /* Start DMA */
    soc_reg_field_set(unit, CMIC_CMC0_CCM_DMA_CFGr, &reg, ENf, 1);
    soc_pci_write(unit, CMIC_CMCx_CCM_DMA_CFG_OFFSET(cmc), reg);

    rv = SOC_E_TIMEOUT;
    if (soc->ccmDmaIntrEnb) {
            soc_cmicm_intr0_enable(unit, IRQ_CMCx_CCMDMA_DONE);
            if (sal_sem_take(soc->ccmDmaIntr, soc->ccmDmaTimeout) < 0) {
                rv = SOC_E_TIMEOUT;
            }
            soc_cmicm_intr0_disable(unit, IRQ_CMCx_CCMDMA_DONE);

            reg = soc_pci_read(unit, CMIC_CMCx_CCM_DMA_STAT_OFFSET(cmc));
            if (soc_reg_field_get(unit, CMIC_CMC0_CCM_DMA_STATr,
                                                        reg, DONEf)) {
                rv = SOC_E_NONE;
                if (soc_reg_field_get(unit, CMIC_CMC0_CCM_DMA_STATr,
                                                        reg, ERRORf)) {
                    rv = SOC_E_FAIL;
                }
            }
    } else {
        soc_timeout_t to;
        soc_cm_debug(DK_WARN, "using Polling mode for CCM DMA\n");
        soc_timeout_init(&to, soc->ccmDmaTimeout, 10000);
        do {
            reg = soc_pci_read(unit, CMIC_CMCx_CCM_DMA_STAT_OFFSET(cmc));
            if (soc_reg_field_get(unit, CMIC_CMC0_CCM_DMA_STATr,
                                                    reg, DONEf)) {
                rv = SOC_E_NONE;
                if (soc_reg_field_get(unit, CMIC_CMC0_CCM_DMA_STATr,
                                                    reg, ERRORf)) {
                    rv = SOC_E_FAIL;
                }
                break;
            }
        } while(!(soc_timeout_check(&to)));
    }

    if (rv == SOC_E_TIMEOUT) {
        soc_cm_debug(DK_ERR, "ccm_dma: timeout\n");

        /* Abort CCM DMA */

        /* Dummy read to allow abort to finish */

        /* Disable CCM DMA */

        /* Clear ccm DMA abort bit */
    }

    CCM_DMA_UNLOCK(unit);
    return rv;
}
#endif

/*
 * @name   - _soc_l3_defip_urpf_index_remap
 * @param  - unit: unit number
 * @param  - wide: 0 - L3_DEFIP, 1 - L3_DEFIP_PAIR_128
 * @param  - index: Physical index
 *
 * @purpose - Remap phyiscal index to logical index
 */
STATIC int
_soc_l3_defip_urpf_index_remap(int unit, int wide, int index)
{
    int new_index = index;
    int num_tcams = 0;
    int tcam_size = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit);
    int entries_left_in_paired_tcam = 0;
    int soc_defip_index_remap = SOC_L3_DEFIP_INDEX_REMAP_GET(unit) / 2;
    int half_defip_size = soc_mem_index_count(unit, L3_DEFIPm) / 2;

    assert(tcam_size);

    if (wide == 0) {
        num_tcams = soc_defip_index_remap / tcam_size;
        entries_left_in_paired_tcam = tcam_size -
            (soc_defip_index_remap % tcam_size);
        new_index = index - (num_tcams * 2 * tcam_size);

        if (new_index >= LPM_INDEX_4K_ENTRIES) {
            new_index = new_index - LPM_INDEX_4K_ENTRIES;
        }

        if (new_index - (soc_defip_index_remap % tcam_size) <
            entries_left_in_paired_tcam) {
            new_index = new_index - (soc_defip_index_remap % tcam_size);
        } else {
            new_index = new_index - ((soc_defip_index_remap % tcam_size) * 2);
        }

        if (index >= LPM_INDEX_4K_ENTRIES) {
            new_index = new_index + half_defip_size;
        }
        return new_index;
    }

    new_index = index;
    if (index >= LPM_INDEX_2K_ENTRIES) {
        new_index = index - LPM_INDEX_2K_ENTRIES + soc_defip_index_remap;
    }
    return new_index;
}

/*
 * @name   - soc_l3_defip_urpf_index_remap
 * @param  - unit: unit number
 * @param  - wide: 0 - L3_DEFIP, 1 - L3_DEFIP_PAIR_128
 * @param  - index: Physical index
 *
 * @purpose - Remap phyiscal index to logical index
 */
int
soc_l3_defip_urpf_index_remap(int unit, int wide, int index)
{
    assert(SOC_L3_DEFIP_INDEX_INIT(unit));
    if (wide == 0) {
        return SOC_L3_DEFIP_URPF_PHY_TO_LOG_INDEX(unit, index);
    }
    return SOC_L3_DEFIP_PAIR_URPF_PHY_TO_LOG_INDEX(unit, index);
}

/*
 * @name   - _soc_l3_defip_index_remap
 * @param  - unit: unit number
 * @param  - wide: 0 - L3_DEFIP, 1 - L3_DEFIP_PAIR_128
 * @param  - index: Physical index
 *
 * @purpose - Remap phyiscal index to logical index
 */

STATIC int
_soc_l3_defip_index_remap(int unit, int wide, int index)
{
    int new_index = index;
    int num_tcams = 0;
    int tcam_size = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit);
    int entries_left_in_paired_tcam = 0;
    int soc_defip_index_remap = SOC_L3_DEFIP_INDEX_REMAP_GET(unit);

    assert(tcam_size); 

    if (wide == 0) {
        num_tcams = soc_defip_index_remap / tcam_size;
        entries_left_in_paired_tcam = tcam_size -
            (soc_defip_index_remap % tcam_size);
        if (index - (num_tcams * 2 * tcam_size) -
            (soc_defip_index_remap % tcam_size) < entries_left_in_paired_tcam) {
            new_index = index - (num_tcams * 2 * tcam_size) -
                        (soc_defip_index_remap % tcam_size);
        } else {
            new_index = index - (num_tcams * 2 * tcam_size) -
                        ((soc_defip_index_remap % tcam_size) * 2);
        }
        return new_index;
    }
    return new_index;
}

/*
 * @name   - soc_l3_defip_index_remap
 * @param  - unit: unit number
 * @param  - wide: 0 - L3_DEFIP, 1 - L3_DEFIP_PAIR_128
 * @param  - index: Physical index
 *
 * @purpose - Remap phyiscal index to logical index
 */
int
soc_l3_defip_index_remap(int unit, int wide, int index)
{
    assert(SOC_L3_DEFIP_INDEX_INIT(unit));
    if (wide == 0) {
        return SOC_L3_DEFIP_PHY_TO_LOG_INDEX(unit, index);
    }
    return SOC_L3_DEFIP_PAIR_PHY_TO_LOG_INDEX(unit, index);
}

/*
 * @name   - _soc_l3_defip_urpf_index_map
 * @param  - unit: unit number
 * @param  - wide: 0 - L3_DEFIP, 1 - L3_DEFIP_PAIR_128
 * @param  - index: Logical index
 *
 * @purpose - Map logical index to physical index
 */
STATIC int
_soc_l3_defip_urpf_index_map(int unit, int wide, int index)
{
    int new_index = index;
    int num_tcams = 0;
    int tcam_size = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit);
    int entries_left_in_paired_tcam = 0;
    int soc_defip_index_remap = SOC_L3_DEFIP_INDEX_REMAP_GET(unit) / 2;
    int half_defip_size = soc_mem_index_count(unit, L3_DEFIPm) / 2;

    assert(tcam_size); 

    if (wide == 0) {
        num_tcams = soc_defip_index_remap / tcam_size;
        entries_left_in_paired_tcam = tcam_size -
                                      (soc_defip_index_remap % tcam_size);
        if (index < entries_left_in_paired_tcam) {
            new_index = index + (num_tcams * 2 * tcam_size) +
                        (soc_defip_index_remap % tcam_size);
        } else if (index >= (half_defip_size)) {
            if ((index - half_defip_size)  < entries_left_in_paired_tcam) {
                new_index = (index - half_defip_size) +
                            (num_tcams * 2 * tcam_size) +
                            (soc_defip_index_remap % tcam_size) +
                            LPM_INDEX_4K_ENTRIES;
            } else {
                new_index = (index - half_defip_size) +
                            (num_tcams * 2 * tcam_size) +
                            ((soc_defip_index_remap % tcam_size) * 2) +
                            LPM_INDEX_4K_ENTRIES;
            }
        } else {
            new_index = index + (num_tcams * 2 * tcam_size) +
                        ((soc_defip_index_remap % tcam_size) * 2);
        }
        return new_index;
    }

    new_index = index;
    if (index >= soc_defip_index_remap) {
        new_index = index + LPM_INDEX_2K_ENTRIES - soc_defip_index_remap;
    }
    return new_index;
}

/*
 * @name   - soc_l3_defip_urpf_index_map
 * @param  - unit: unit number
 * @param  - wide: 0 - L3_DEFIP, 1 - L3_DEFIP_PAIR_128
 * @param  - index: Logical index
 *
 * @purpose - Map logical index to physical index
 */

int
soc_l3_defip_urpf_index_map(int unit, int wide, int index)
{
    assert(SOC_L3_DEFIP_INDEX_INIT(unit));

    if (wide == 0) {
        return SOC_L3_DEFIP_URPF_LOG_TO_PHY_INDEX(unit, index);
    }
    return SOC_L3_DEFIP_PAIR_URPF_LOG_TO_PHY_INDEX(unit, index);
}

/*
 * @name   - _soc_l3_defip_index_map
 * @param  - unit: unit number
 * @param  - wide: 0 - L3_DEFIP, 1 - L3_DEFIP_PAIR_128
 * @param  - index: Logical index
 *
 * @purpose - Map logical index to physical index
 */
STATIC int
_soc_l3_defip_index_map(int unit, int wide, int index)
{
    int new_index = index;
    int num_tcams = 0;
    int tcam_size = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit);
    int entries_left_in_paired_tcam = 0;
    int soc_defip_index_remap = SOC_L3_DEFIP_INDEX_REMAP_GET(unit);

    if (wide == 0) {
        num_tcams = soc_defip_index_remap / tcam_size;
        entries_left_in_paired_tcam = tcam_size -
            (soc_defip_index_remap % tcam_size);
        if (index < entries_left_in_paired_tcam) {
            new_index = index + (num_tcams * 2 * tcam_size) +
                        soc_defip_index_remap % tcam_size;
        } else {
            new_index = index + (num_tcams * 2 * tcam_size) +
                        ((soc_defip_index_remap % tcam_size) * 2);
        }
        return new_index;
    }
    return index;
}


/*
 * @name   - soc_l3_defip_index_map
 * @param  - unit: unit number
 * @param  - wide: 0 - L3_DEFIP, 1 - L3_DEFIP_PAIR_128
 * @param  - index: Logical index
 *
 * @purpose - Map logical index to physical index
 */
int
soc_l3_defip_index_map(int unit, int wide, int index)
{
    assert(SOC_L3_DEFIP_INDEX_INIT(unit));
    if (wide == 0) {
        return SOC_L3_DEFIP_LOG_TO_PHY_INDEX(unit, index);
    }
    return SOC_L3_DEFIP_PAIR_LOG_TO_PHY_INDEX(unit, index);
}

/*
 * @name   - soc_l3_defip_index_mem_map
 * @param  - unit: unit number
 * @param  - index: physical index
 * @param  - (OUT)mem: name of the tcam
 *
 * @purpose - Map L3_DEFIP view using physical index
 */

int
soc_l3_defip_index_mem_map(int unit, int index, soc_mem_t* mem)
{
    int logical_index =  -1; 
    int defip_pair_index = -1;
    int cam_size = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit);

    *mem = L3_DEFIPm;

    if (SOC_CONTROL(unit)->l3_defip_urpf) {
        logical_index = soc_l3_defip_urpf_index_remap(unit, 0, index);

        if (logical_index != -1) {
            *mem = L3_DEFIPm;
        } else {
            *mem = L3_DEFIP_PAIR_128m;  
            defip_pair_index = ((index / (cam_size * 2)) * cam_size) + 
                               (index % cam_size);
            logical_index = soc_l3_defip_urpf_index_remap(unit, 1, 
                                                          defip_pair_index);
        }
        return logical_index;
    }

    logical_index = soc_l3_defip_index_remap(unit, 0, index);

    if (logical_index != -1) {
        *mem = L3_DEFIPm;  
    } else {
        *mem = L3_DEFIP_PAIR_128m;  
        defip_pair_index = ((index / (cam_size * 2)) * cam_size) + 
                           (index % cam_size);
        logical_index = soc_l3_defip_index_remap(unit, 1, defip_pair_index);
    }

    return logical_index;
}

/*
 * @name   - soc_l3_defip_indexes_init 
 * @param  - unit: unit number
 * @returns -  SOC_E_MEMORY, SOC_E_NONE
 *
 * @purpose - generate a map of logical to physical and 
 *            physical to logical indexes 
 */
int 
soc_l3_defip_indexes_init(int unit)
{
    int log_index;
    int phy_index; 
    int temp_index;

    int max_index = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit) * 
                    SOC_L3_DEFIP_MAX_TCAMS_GET(unit); 
    int defip_max_index = soc_mem_index_count(unit, L3_DEFIPm);
    int size = max_index * sizeof(int);

    assert(SOC_L3_DEFIP_TCAM_DEPTH_GET(unit));
    assert(SOC_L3_DEFIP_MAX_TCAMS_GET(unit));

    if (SOC_L3_DEFIP_INDEX_INIT(unit)) {
        sal_free(SOC_L3_DEFIP_INDEX_INIT(unit));
    }

    SOC_L3_DEFIP_INDEX_INIT(unit) = sal_alloc(sizeof(_soc_l3_defip_index_table_t), "defip map table");
    if (SOC_L3_DEFIP_INDEX_INIT(unit) == NULL) {
        return SOC_E_MEMORY;   
    }

    sal_memset(SOC_L3_DEFIP_INDEX_INIT(unit), 0x0, sizeof(_soc_l3_defip_index_table_t));
    size = defip_max_index * sizeof(int);
    SOC_L3_DEFIP_LOG_TO_PHY_ARRAY(unit) = 
                            sal_alloc(size, "defip log to phy index mapping");
    if (SOC_L3_DEFIP_LOG_TO_PHY_ARRAY(unit) == NULL) {
        return SOC_E_MEMORY;   
    } 
    sal_memset(SOC_L3_DEFIP_LOG_TO_PHY_ARRAY(unit), -1, size);

    SOC_L3_DEFIP_URPF_LOG_TO_PHY_ARRAY(unit) = 
                         sal_alloc(size, "urpf defip log to phy index mapping");
    if (SOC_L3_DEFIP_URPF_LOG_TO_PHY_ARRAY(unit) == NULL) {
        return SOC_E_MEMORY;   
    } 
    sal_memset(SOC_L3_DEFIP_URPF_LOG_TO_PHY_ARRAY(unit), -1, size);
 
    size = max_index * sizeof(int);
    SOC_L3_DEFIP_PHY_TO_LOG_ARRAY(unit) = 
                          sal_alloc(size, "defip phy to log index mapping");
    if (SOC_L3_DEFIP_PHY_TO_LOG_ARRAY(unit) == NULL) {
        return SOC_E_MEMORY;   
    } 
    sal_memset(SOC_L3_DEFIP_PHY_TO_LOG_ARRAY(unit), -1, size);
 
    SOC_L3_DEFIP_URPF_PHY_TO_LOG_ARRAY(unit) = 
                             sal_alloc(size, "defip phy to log index mapping");
    if (SOC_L3_DEFIP_URPF_PHY_TO_LOG_ARRAY(unit) == NULL) {
        return SOC_E_MEMORY;   
    }
    sal_memset(SOC_L3_DEFIP_URPF_PHY_TO_LOG_ARRAY(unit), -1, size);
  
    size = soc_mem_index_count(unit, L3_DEFIP_PAIR_128m) * sizeof(int);

    SOC_L3_DEFIP_PAIR_LOG_TO_PHY_ARRAY(unit) = 
                            sal_alloc(size, "defip log to phy index mapping");
    if (SOC_L3_DEFIP_PAIR_LOG_TO_PHY_ARRAY(unit) == NULL) {
        return SOC_E_MEMORY;   
    }
    sal_memset(SOC_L3_DEFIP_PAIR_LOG_TO_PHY_ARRAY(unit), -1, size);

    SOC_L3_DEFIP_PAIR_URPF_LOG_TO_PHY_ARRAY(unit) = 
                         sal_alloc(size, "urpf defip log to phy index mapping");
    if (SOC_L3_DEFIP_PAIR_URPF_LOG_TO_PHY_ARRAY(unit) == NULL) {
        return SOC_E_MEMORY;   
    } 
    sal_memset(SOC_L3_DEFIP_PAIR_URPF_LOG_TO_PHY_ARRAY(unit), -1, size);
 
    size = max_index * sizeof(int);
    SOC_L3_DEFIP_PAIR_PHY_TO_LOG_ARRAY(unit) = 
                          sal_alloc(size, "defip phy to log index mapping");
    if (SOC_L3_DEFIP_PAIR_PHY_TO_LOG_ARRAY(unit) == NULL) {
        return SOC_E_MEMORY;   
    } 
    sal_memset(SOC_L3_DEFIP_PAIR_PHY_TO_LOG_ARRAY(unit), -1, size);
 
    SOC_L3_DEFIP_PAIR_URPF_PHY_TO_LOG_ARRAY(unit) = 
                             sal_alloc(size, "defip phy to log index mapping");
    if (SOC_L3_DEFIP_PAIR_URPF_PHY_TO_LOG_ARRAY(unit) == NULL) {
        return SOC_E_MEMORY;   
    }
    sal_memset(SOC_L3_DEFIP_PAIR_URPF_PHY_TO_LOG_ARRAY(unit), -1, size);
    for (log_index = 0; log_index < defip_max_index; log_index++) {
        phy_index = _soc_l3_defip_index_map(unit, 0, log_index);
        SOC_L3_DEFIP_LOG_TO_PHY_INDEX(unit, log_index) = phy_index;
        temp_index = _soc_l3_defip_index_remap(unit, 0, phy_index);   
        SOC_L3_DEFIP_PHY_TO_LOG_INDEX(unit, phy_index) = temp_index;
        phy_index = _soc_l3_defip_urpf_index_map(unit, 0, log_index);
        SOC_L3_DEFIP_URPF_LOG_TO_PHY_INDEX(unit, log_index) = phy_index;
        temp_index = _soc_l3_defip_urpf_index_remap(unit, 0, phy_index); 
        SOC_L3_DEFIP_URPF_PHY_TO_LOG_INDEX(unit, phy_index) = temp_index;
    }
   
    defip_max_index = soc_mem_index_count(unit, L3_DEFIP_PAIR_128m);
     
    for (log_index = 0; log_index < defip_max_index; log_index++) {
        phy_index = _soc_l3_defip_index_map(unit, 1, log_index);
        SOC_L3_DEFIP_PAIR_LOG_TO_PHY_INDEX(unit, log_index) = phy_index;
        temp_index = _soc_l3_defip_index_remap(unit, 1, phy_index); 
        SOC_L3_DEFIP_PAIR_PHY_TO_LOG_INDEX(unit, phy_index) = temp_index;
        phy_index = _soc_l3_defip_urpf_index_map(unit, 1, log_index);
        SOC_L3_DEFIP_PAIR_URPF_LOG_TO_PHY_INDEX(unit, log_index) = phy_index;
        temp_index = _soc_l3_defip_urpf_index_remap(unit, 1, phy_index);
        SOC_L3_DEFIP_PAIR_URPF_PHY_TO_LOG_INDEX(unit, phy_index) = temp_index;
    }

    return SOC_E_NONE;  
}

#endif /* defined(BCM_ESW_SUPPORT) || defined(BCM_SIRIUS_SUPPORT) ||
          defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT) */

