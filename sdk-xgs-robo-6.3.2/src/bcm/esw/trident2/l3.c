/*
 * $Id: l3.c 1.84.2.3 Broadcom SDK $
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
 * File:    l3.c
 * Purpose: Triumph3 L3 function implementations
 */

#include <soc/defs.h>
#if defined(INCLUDE_L3) && defined(BCM_TRIDENT2_SUPPORT) 

#include <soc/drv.h>
#include <bcm/vlan.h>
#include <bcm/error.h>

#include <bcm/l3.h>
#include <soc/l3x.h>
#include <soc/lpm.h>
#ifdef ALPM_ENABLE
#include <soc/alpm.h>
#endif
#include <bcm/tunnel.h>
#include <bcm/debug.h>
#include <bcm/error.h>
#include <bcm/stack.h>
#include <soc/trident2.h>

#include <bcm_int/esw/trident2.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/flex_ctr.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/flex_ctr.h>
#include <bcm_int/esw/virtual.h>

#if defined(BCM_TRX_SUPPORT) 
#include <bcm_int/esw/trx.h>
#endif /* BCM_TRX_SUPPORT */

#define _BCM_TD2_L3_MEM_BANKS_ALL     (-1)
#define _BCM_TD2_HOST_ENTRY_NOT_FOUND (-1)

#define L3_LOCK(_unit_)   _bcm_esw_l3_lock(_unit_)
#define L3_UNLOCK(_unit_) _bcm_esw_l3_unlock(_unit_)

/* IP Options Handling */
#define L3_INFO(_unit_) (&_bcm_l3_bk_info[_unit_])
#define _BCM_IP_OPTION_PROFILE_CHUNK 256
#define _BCM_IP4_OPTIONS_LEN    \
            (soc_mem_index_count(unit, IP_OPTION_CONTROL_PROFILE_TABLEm)/ \
             _BCM_IP_OPTION_PROFILE_CHUNK)
/*
 * IP_OPTIONS_PROFILE usage bitmap operations
 */
#define _BCM_IP4_OPTIONS_USED_GET(_u_, _identifier_) \
        SHR_BITGET(L3_INFO(_u_)->ip4_options_bitmap, (_identifier_))
#define _BCM_IP4_OPTIONS_USED_SET(_u_, _identifier_) \
        SHR_BITSET(L3_INFO((_u_))->ip4_options_bitmap, (_identifier_))
#define _BCM_IP4_OPTIONS_USED_CLR(_u_, _identifier_) \
        SHR_BITCLR(L3_INFO((_u_))->ip4_options_bitmap, (_identifier_))

#define _BCM_IP_OPTION_REINIT_INVALID_HW_IDX 0xffff

/* Bookkeeping info for ECMP resilient hashing */
typedef struct _td2_ecmp_rh_info_s {
    int num_ecmp_rh_flowset_blocks;
    SHR_BITDCL *ecmp_rh_flowset_block_bitmap; /* Each block corresponds to
                                                 64 entries */
    uint32 ecmp_rh_rand_seed; /* The seed for pseudo-random number generator */
} _td2_ecmp_rh_info_t;

STATIC _td2_ecmp_rh_info_t *_td2_ecmp_rh_info[BCM_MAX_NUM_UNITS];

#define _BCM_ECMP_RH_FLOWSET_BLOCK_USED_GET(_u_, _idx_) \
    SHR_BITGET(_td2_ecmp_rh_info[_u_]->ecmp_rh_flowset_block_bitmap, _idx_)
#define _BCM_ECMP_RH_FLOWSET_BLOCK_USED_SET(_u_, _idx_) \
    SHR_BITSET(_td2_ecmp_rh_info[_u_]->ecmp_rh_flowset_block_bitmap, _idx_)
#define _BCM_ECMP_RH_FLOWSET_BLOCK_USED_CLR(_u_, _idx_) \
    SHR_BITCLR(_td2_ecmp_rh_info[_u_]->ecmp_rh_flowset_block_bitmap, _idx_)
#define _BCM_ECMP_RH_FLOWSET_BLOCK_USED_SET_RANGE(_u_, _idx_, _count_) \
    SHR_BITSET_RANGE(_td2_ecmp_rh_info[_u_]->ecmp_rh_flowset_block_bitmap, \
            _idx_, _count_)
#define _BCM_ECMP_RH_FLOWSET_BLOCK_USED_CLR_RANGE(_u_, _idx_, _count_) \
    SHR_BITCLR_RANGE(_td2_ecmp_rh_info[_u_]->ecmp_rh_flowset_block_bitmap, \
            _idx_, _count_)
#define _BCM_ECMP_RH_FLOWSET_BLOCK_TEST_RANGE(_u_, _idx_, _count_, _result_) \
    SHR_BITTEST_RANGE(_td2_ecmp_rh_info[_u_]->ecmp_rh_flowset_block_bitmap, \
            _idx_, _count_, _result_)

/*----- STATIC FUNCS ----- */
/*
 * Function:
 *      _bcm_td2_l3_ent_init
 * Purpose:
 *      TD2 helper routine used to init l3 host entry buffer
 * Parameters:
 *      unit      - (IN) SOC unit number. 
 *      mem       - (IN) L3 table memory.
 *      l3cfg     - (IN/OUT) l3 entry  lookup key & search result.
 *      l3x_entry - (IN) hw buffer.
 * Returns:
 *      void
 */
STATIC int
_bcm_td2_l3_ent_init(int unit, soc_mem_t mem, 
                      _bcm_l3_cfg_t *l3cfg, void *l3x_entry)
{
    int ipv6;                     /* Entry is IPv6 flag.         */
    uint32 *buf_p;                /* HW buffer address.          */ 


    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Zero destination buffer. */
    buf_p = (uint32 *)l3x_entry;
    sal_memset(buf_p, 0, BCM_L3_MEM_ENT_SIZE(unit, mem)); 

    if (ipv6) { /* ipv6 entry */
        if (BCM_XGS3_L3_MEM(unit, v6) == mem) {
            soc_mem_ip6_addr_set(unit, mem, buf_p, IP_ADDR_LWR_64f,
                             l3cfg->l3c_ip6, SOC_MEM_IP6_LOWER_ONLY);
            soc_mem_ip6_addr_set(unit, mem, buf_p, IP_ADDR_UPR_64f,
                             l3cfg->l3c_ip6, SOC_MEM_IP6_UPPER_ONLY);
            soc_mem_field32_set(unit, mem, buf_p, VRF_IDf,
                                                  l3cfg->l3c_vrf);
            soc_mem_field32_set(unit, mem, buf_p, VALID_0f, 1); 
            soc_mem_field32_set(unit, mem, buf_p, VALID_1f, 1); 
            soc_mem_field32_set(unit, mem, buf_p, KEY_TYPE_0f, 2);
            soc_mem_field32_set(unit, mem, buf_p, KEY_TYPE_1f, 2);
        } else if (BCM_XGS3_L3_MEM(unit, v6_4) == mem) { 
            soc_mem_ip6_addr_set(unit, mem, buf_p, IPV6UC_EXT__IP_ADDR_LWR_64f,
                             l3cfg->l3c_ip6, SOC_MEM_IP6_LOWER_ONLY);
            soc_mem_ip6_addr_set(unit, mem, buf_p, IPV6UC_EXT__IP_ADDR_UPR_64f,
                             l3cfg->l3c_ip6, SOC_MEM_IP6_UPPER_ONLY);
            soc_mem_field32_set(unit, mem, buf_p, IPV6UC_EXT__VRF_IDf,
                                                  l3cfg->l3c_vrf);
            /* ipv6 extended host entry */
            soc_mem_field32_set(unit, mem, buf_p, KEY_TYPE_0f, 3);
            soc_mem_field32_set(unit, mem, buf_p, KEY_TYPE_1f, 3);
            soc_mem_field32_set(unit, mem, buf_p, KEY_TYPE_2f, 3);
            soc_mem_field32_set(unit, mem, buf_p, KEY_TYPE_3f, 3);

            soc_mem_field32_set(unit, mem, buf_p, VALID_0f, 1); 
            soc_mem_field32_set(unit, mem, buf_p, VALID_1f, 1); 
            soc_mem_field32_set(unit, mem, buf_p, VALID_2f, 1); 
            soc_mem_field32_set(unit, mem, buf_p, VALID_3f, 1); 
        } else {
            return BCM_E_NOT_FOUND;
        }
    } else { /* ipv4 entry */                /* if uC is still not ready, return timeout */
        if (BCM_XGS3_L3_MEM(unit, v4) == mem) {
            soc_mem_field32_set(unit, mem, buf_p, IP_ADDRf,
                                               l3cfg->l3c_ip_addr);
            soc_mem_field32_set(unit, mem, buf_p, VRF_IDf,
                                                  l3cfg->l3c_vrf);
            /* ipv4 unicast */
            soc_mem_field32_set(unit, mem, buf_p, KEY_TYPEf, 0); 
            soc_mem_field32_set(unit, mem, buf_p, VALIDf, 1); 
        } else if (BCM_XGS3_L3_MEM(unit, v4_2) == mem) { 
            soc_mem_field32_set(unit, mem, buf_p, IPV4UC_EXT__IP_ADDRf,
                                               l3cfg->l3c_ip_addr);
            soc_mem_field32_set(unit, mem, buf_p, IPV4UC_EXT__VRF_IDf,
                                                  l3cfg->l3c_vrf);
            /* ipv4 extended host entry */
            soc_mem_field32_set(unit, mem, buf_p, KEY_TYPE_0f, 1);
            soc_mem_field32_set(unit, mem, buf_p, KEY_TYPE_1f, 1);
            soc_mem_field32_set(unit, mem, buf_p, VALID_1f, 1); 
            soc_mem_field32_set(unit, mem, buf_p, VALID_0f, 1); 
        } else {
            return BCM_E_NOT_FOUND;
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_ent_parse
 * Purpose:
 *      TD2 helper routine used to parse hw l3 entry to api format.
 * Parameters:
 *      unit      - (IN) SOC unit number. 
 *      mem       - (IN) L3 table memory.
 *      l3cfg     - (IN/OUT) l3 entry key & parse result.
 *      nh_idx    - (IN/OUT) Next hop index. 
 *      l3x_entry - (IN) hw buffer.
 * Returns:
 *      void
 */
STATIC int
_bcm_td2_l3_ent_parse(int unit, soc_mem_t mem, _bcm_l3_cfg_t *l3cfg,
                                        int *nh_idx, void *l3x_entry)
{
    int ipv6;                     /* Entry is IPv6 flag.         */
    uint32 hit;                   /* composite hit = hit_x|hit_y */
    uint32 *buf_p;                /* HW buffer address.          */ 
    _bcm_l3_fields_t *fld;        /* L3 table common fields.     */
    int embedded_nh = FALSE;
    l3_entry_hit_only_x_entry_t hit_x;
    l3_entry_hit_only_y_entry_t hit_y;
    int idx, idx_max, idx_offset, hit_idx_shift;
    soc_field_t hitf[] = { HIT_0f, HIT_1f, HIT_2f, HIT_3f};

    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);
    buf_p = (uint32 *)l3x_entry;

    /* Set table fields */
    BCM_TD2_L3_HOST_TABLE_FLD(unit, mem, ipv6, fld);  

    /* Check if embedded NH table is being used */
    embedded_nh = (mem == L3_ENTRY_IPV4_MULTICASTm && !ipv6) || 
                  (mem == L3_ENTRY_IPV6_MULTICASTm && ipv6);

    /* Reset entry flags */
    l3cfg->l3c_flags = (ipv6) ? BCM_L3_IP6 : 0;

    idx_max = 1;
    idx_offset = (l3cfg->l3c_hw_index & 0x3);
    hit_idx_shift = 2;
    if (mem == L3_ENTRY_IPV4_MULTICASTm || 
        mem == L3_ENTRY_IPV6_UNICASTm) {
        idx_max = 2;
        hit_idx_shift = 1;
        idx_offset = (l3cfg->l3c_hw_index & 0x1) << 1;
    } else if (mem == L3_ENTRY_IPV6_MULTICASTm) {
        idx_max = 4;
        hit_idx_shift = 0;
        idx_offset = 0;
    }

    /* Get info from L3 and next hop table */
    BCM_IF_ERROR_RETURN(
        BCM_XGS3_MEM_READ(unit, L3_ENTRY_HIT_ONLY_Xm,
          (l3cfg->l3c_hw_index >> hit_idx_shift), &hit_x));

    BCM_IF_ERROR_RETURN(
        BCM_XGS3_MEM_READ(unit, L3_ENTRY_HIT_ONLY_Ym,
          (l3cfg->l3c_hw_index >> hit_idx_shift), &hit_y));

    hit = 0;
    for (idx = idx_offset; idx < (idx_offset + idx_max); idx++) {
        hit |= soc_mem_field32_get(unit, L3_ENTRY_HIT_ONLY_Xm,
                                   &hit_x, hitf[idx]);
    }

    for (idx = idx_offset; idx < (idx_offset + idx_max); idx++) {
        hit |= soc_mem_field32_get(unit, L3_ENTRY_HIT_ONLY_Ym,
                                   &hit_y, hitf[idx]);
    }

    soc_mem_field32_set(unit, mem, buf_p, fld->hit, hit);

    if (hit) {
        l3cfg->l3c_flags |= BCM_L3_HIT;
    }

    /* Get priority override flag. */
    if (soc_mem_field32_get(unit, mem, buf_p, fld->rpe)) {
        l3cfg->l3c_flags |= BCM_L3_RPE;
    }

    /* Get destination discard flag. */
    if (soc_mem_field32_get(unit, mem, buf_p, fld->dst_discard)) {
        l3cfg->l3c_flags |= BCM_L3_DST_DISCARD;
    }

    /* Get local addr bit. */
    if (soc_mem_field32_get(unit, mem, buf_p, fld->local_addr)) {
        l3cfg->l3c_flags |= BCM_L3_HOST_LOCAL;
    }

    /* Get classification group id. */
    l3cfg->l3c_lookup_class = 
        soc_mem_field32_get(unit, mem, buf_p, fld->class_id);

    /* Get priority. */
    l3cfg->l3c_prio = soc_mem_field32_get(unit, mem, buf_p, fld->priority);

    /* Get virtual router id. */
    l3cfg->l3c_vrf = soc_mem_field32_get(unit, mem, buf_p, fld->vrf);

    if (embedded_nh) {
        if (nh_idx) {
            *nh_idx = BCM_XGS3_L3_INVALID_INDEX; /* Embedded NH */
        }
        l3cfg->l3c_intf = soc_mem_field32_get(unit, mem, buf_p,
                                           fld->l3_intf);
    } else {
        /* Get next hop info. */
        if (nh_idx) {
            *nh_idx = soc_mem_field32_get(unit, mem, buf_p, fld->nh_idx);
        }
        if (soc_mem_field32_get(unit, mem, buf_p, ECMPf)) {
            /* Mark entry as ecmp */
            l3cfg->l3c_ecmp = TRUE;
            l3cfg->l3c_flags |= BCM_L3_MULTIPATH;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_clear_hit
 * Purpose:
 *      Clear hit bit on l3 entry
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      mem       - (IN)L3 table memory.
 *      l3cfg     - (IN)l3 entry info. 
 *      l3x_entry - (IN)l3 entry filled hw buffer.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_l3_clear_hit(int unit, soc_mem_t mem, _bcm_l3_cfg_t *l3cfg,
                                   void *l3x_entry, int l3_entry_idx)
{
    int ipv6;                     /* Entry is IPv6 flag.      */
    int mcast;                    /* Entry is multicast flag. */ 
    uint32 *buf_p;                /* HW buffer address.       */ 
    _bcm_l3_fields_t *fld;        /* L3 table common fields.  */
    soc_field_t hitf[] = { HIT_0f, HIT_1f, HIT_2f, HIT_3f };
    int idx;

    /* Input parameters check */
    if ((NULL == l3cfg) || (NULL == l3x_entry)) {
        return (BCM_E_PARAM);
    }

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);
    mcast = (l3cfg->l3c_flags & BCM_L3_IPMC);

    /* Set table fields */
    BCM_TD2_L3_HOST_TABLE_FLD(unit, mem, ipv6, fld);

    /* Init memory pointers. */ 
    buf_p = (uint32 *)l3x_entry;

    /* If entry was not hit  there is nothing to clear */
    if (!(l3cfg->l3c_flags & BCM_L3_HIT)) {
        return (BCM_E_NONE);
    }

    /* Reset hit bit. */
    soc_mem_field32_set(unit, mem, buf_p, fld->hit, 0);

    if (ipv6 && mcast) {
        /* IPV6 multicast entry hit reset. */
        for (idx = 0; idx < 4; idx++) {
            soc_mem_field32_set(unit, mem, buf_p, hitf[idx], 0);
        }
    } else if (ipv6 || mcast) {
        /* Reset IPV6 unicast and IPV4 multicast hit bits. */
        for (idx = 0; idx < 2; idx++) {
            soc_mem_field32_set(unit, mem, buf_p, hitf[idx], 0);
        }
    }

    /* Write entry back to hw. */
    return BCM_XGS3_MEM_WRITE(unit, mem, l3_entry_idx, buf_p);
}

/*
 * Function:
 *      _bcm_td2_l3_entry_add
 * Purpose:
 *      Add and entry to TD2 L3 host table.
 * Parameters:
 *      unit   - (IN) SOC unit number.
 *      l3cfg  - (IN) L3 entry info.
 *      nh_idx - (IN) Next hop index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_l3_entry_add(int unit, _bcm_l3_cfg_t *l3cfg, int nh_idx)
{

    int ipv6;
    uint32 *bufp;             /* Hardware buffer ptr     */
    soc_mem_t mem;            /* L3 table memory.        */  
    int rv = BCM_E_NONE;      /* Operation status.       */
    _bcm_l3_fields_t *fld;    /* L3 table common fields. */
    int embedded_nh = FALSE;
    uint32 glp, port_id, modid;
    _bcm_l3_intf_cfg_t l3_intf;
    l3_entry_ipv4_unicast_entry_t l3v4_entry;       /* IPv4 */
    l3_entry_ipv4_multicast_entry_t l3v4_ext_entry;   /* IPv4-Embedded */
    l3_entry_ipv6_unicast_entry_t l3v6_entry;       /* IPv6 */
    l3_entry_ipv6_multicast_entry_t l3v6_ext_entry;   /* IPv6-Embedded */

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Check if embedded NH table is being used */
    embedded_nh = BCM_TD2_L3_USE_EMBEDDED_NEXT_HOP(unit, l3cfg->l3c_intf, nh_idx);

    /* Get table memory. */
    BCM_TD2_L3_HOST_TABLE_MEM(unit, l3cfg->l3c_intf, ipv6, mem, nh_idx);
    
    /* Set table fields */
    BCM_TD2_L3_HOST_TABLE_FLD(unit, mem, ipv6, fld);

    /* Assign entry buf based on table being used */
    BCM_TD2_L3_HOST_ENTRY_BUF(ipv6, mem, bufp,
                                  l3v4_entry,
                                  l3v4_ext_entry,
                                  l3v6_entry,
                                  l3v6_ext_entry);

    /* Prepare host entry for addition. */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_ent_init(unit, mem, l3cfg, bufp));

    /* Set hit bit. */
    if (l3cfg->l3c_flags & BCM_L3_HIT) {
        soc_mem_field32_set(unit, mem, bufp, fld->hit, 1);
    }

    /* Set priority override bit. */
    if (l3cfg->l3c_flags & BCM_L3_RPE) {
        soc_mem_field32_set(unit, mem, bufp, fld->rpe, 1);
    }

    /* Set destination discard bit. */
    if (l3cfg->l3c_flags & BCM_L3_DST_DISCARD) {
        soc_mem_field32_set(unit, mem, bufp, fld->dst_discard, 1);
    }

    /* Set local addr bit. */
    if (l3cfg->l3c_flags & BCM_L3_HOST_LOCAL) {
        soc_mem_field32_set(unit, mem, bufp, fld->local_addr, 1);
    }

    /* Set classification group id. */
    soc_mem_field32_set(unit, mem, bufp, fld->class_id, 
                        l3cfg->l3c_lookup_class);
    /*  Set priority. */
    soc_mem_field32_set(unit, mem, bufp, fld->priority, l3cfg->l3c_prio);

    if (embedded_nh) {
        sal_memset(&l3_intf, 0, sizeof(_bcm_l3_intf_cfg_t));
        BCM_XGS3_L3_INTF_IDX_SET(l3_intf, l3cfg->l3c_intf);

        BCM_XGS3_L3_MODULE_LOCK(unit);
        rv = BCM_XGS3_L3_HWCALL_EXEC(unit, if_get) (unit, &l3_intf);
        BCM_XGS3_L3_MODULE_UNLOCK(unit);
        if (rv >= 0) {
            soc_mem_field32_set(unit, mem, bufp, fld->l3_intf, l3_intf.l3i_vid);
        }
        port_id = (_BCM_TD2_L3_PORT_MASK & l3cfg->l3c_port_tgid);
        modid   = (_BCM_TD2_L3_MODID_MASK & l3cfg->l3c_modid) <<
                                                  _BCM_TD2_L3_MODID_SHIFT;
        glp     = modid | port_id;
 
        if (l3cfg->l3c_flags & BCM_L3_TGID) {
            glp |= 0x8000;
        }
 
        soc_mem_mac_addr_set(unit, mem, bufp,
                             fld->mac_addr, l3cfg->l3c_mac_addr);
        soc_mem_field32_set(unit, mem, bufp, fld->l3_intf, l3cfg->l3c_intf);
        soc_mem_field32_set(unit, mem, bufp, fld->glp, glp);
    } else {
        /* Set next hop index. */
        if (l3cfg->l3c_flags & BCM_L3_MULTIPATH) {
            soc_mem_field32_set(unit, mem, bufp, ECMP_PTRf, nh_idx);
            soc_mem_field32_set(unit, mem, bufp, ECMPf, 1);
        } else {
            soc_mem_field32_set(unit, mem, bufp, fld->nh_idx, nh_idx);
        }
    }

    /* Insert in table */
    rv = soc_mem_insert(unit, mem, MEM_BLOCK_ANY, bufp);

    /* Handle host entry 'replace' actions */
    if ((BCM_E_EXISTS == rv) && (l3cfg->l3c_flags & BCM_L3_REPLACE)) {
        rv = BCM_E_NONE;
    }
    
    /* Write status check. */
    if ((rv >= 0) && (BCM_XGS3_L3_INVALID_INDEX == l3cfg->l3c_hw_index)) {
        (ipv6) ?  BCM_XGS3_L3_IP6_CNT(unit)++ : BCM_XGS3_L3_IP4_CNT(unit)++;
    }

    return rv;
}


/*
 * Function:
 *      _bcm_td2_l3_entry_get
 * Purpose:
 *      Get an entry from TD2 3 host table.
 * Parameters:
 *      unit     - (IN) SOC unit number.
 *      l3cfg    - (IN/OUT) L3 entry  lookup key & search result.
 *      nh_index - (IN/OUT) Next hop index.
 *      embd     - (OUT) If TRUE, entry was found as embedded NH.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_l3_entry_get(int unit, _bcm_l3_cfg_t *l3cfg, int *nh_idx, int *embd)
{
    int ipv6;                                   /* IPv6 entry indicator.    */
    int clear_hit;                              /* Clear hit bit.           */
    int rv = BCM_E_NONE;                        /* Operation return status. */
    soc_mem_t mem, mem_ext;                     /* L3 table memories        */
    uint32 *buf_key, *buf_entry;                /* Key and entry buffer ptrs*/
    l3_entry_ipv4_unicast_entry_t l3v4_key, l3v4_entry;       /* IPv4 */
    l3_entry_ipv4_multicast_entry_t l3v4_ext_key, l3v4_ext_entry;   /* IPv4-Embedded */
    l3_entry_ipv6_unicast_entry_t l3v6_key, l3v6_entry;       /* IPv6 */
    l3_entry_ipv6_multicast_entry_t l3v6_ext_key, l3v6_ext_entry;   /* IPv6-Embedded */

    /* Indexed NH entry */
    *embd = _BCM_TD2_HOST_ENTRY_NOT_FOUND;

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Preserve clear_hit value. */
    clear_hit = l3cfg->l3c_flags & BCM_L3_HIT_CLEAR;

    /* Table memories */
    if (ipv6) {
        mem = BCM_XGS3_L3_MEM(unit, v6); mem_ext = BCM_XGS3_L3_MEM(unit, v6_4);
    } else {
        mem = BCM_XGS3_L3_MEM(unit, v4); mem_ext = BCM_XGS3_L3_MEM(unit, v4_2);
    }

    /* Assign entry-key buf based on table being used */
    BCM_TD2_L3_HOST_ENTRY_BUF(ipv6, mem, buf_key,
                                        l3v4_key,
                                        l3v4_ext_key,
                                        l3v6_key,
                                        l3v6_ext_key);

    /* Assign entry buf based on table being used */
    BCM_TD2_L3_HOST_ENTRY_BUF(ipv6, mem, buf_entry,
                                        l3v4_entry,
                                        l3v4_ext_entry,
                                        l3v6_entry,
                                        l3v6_ext_entry);
    /* Prepare lookup key. */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_ent_init(unit, mem, l3cfg, buf_key));

    /* Perform lookup */
    rv = soc_mem_generic_lookup(unit, mem, MEM_BLOCK_ANY,
                                _BCM_TD2_L3_MEM_BANKS_ALL,
                                buf_key, buf_entry, &l3cfg->l3c_hw_index);
    if (BCM_SUCCESS(rv)) {
        /* Indexed NH entry */
        *embd = FALSE;
        /* Extract entry info. */
        BCM_IF_ERROR_RETURN(_bcm_td2_l3_ent_parse(unit, mem, l3cfg,
                                                  nh_idx, buf_entry));
        /* Clear the HIT bit */
        if (clear_hit) {
            BCM_IF_ERROR_RETURN(_bcm_td2_l3_clear_hit(unit, mem,
                                       l3cfg, buf_entry, l3cfg->l3c_hw_index));
        }
    } else if ((BCM_E_NOT_FOUND == rv) &&
               (soc_feature(unit, soc_feature_l3_extended_host_entry))) {
        /* Search in the extended tables only if the extended host entry
         * feature is supported and if the entry was not found in the 
         * regular host entry */

        /* Assign entry-key buf based on table being used */
        BCM_TD2_L3_HOST_ENTRY_BUF(ipv6, mem_ext, buf_key,
                                        l3v4_key,
                                        l3v4_ext_key,
                                        l3v6_key,
                                        l3v6_ext_key);

        /* Assign entry buf based on table being used */
        BCM_TD2_L3_HOST_ENTRY_BUF(ipv6, mem_ext, buf_entry,
                                        l3v4_entry,
                                        l3v4_ext_entry,
                                        l3v6_entry,
                                        l3v6_ext_entry);

        /* Prepare lookup key. */
        BCM_IF_ERROR_RETURN(_bcm_td2_l3_ent_init(unit, mem_ext, l3cfg, buf_key));

        /* Perform lookup hw. */
        rv = soc_mem_generic_lookup(unit, mem_ext, MEM_BLOCK_ANY,
                                    _BCM_TD2_L3_MEM_BANKS_ALL,
                                    buf_key, buf_entry, &l3cfg->l3c_hw_index);

        if (BCM_SUCCESS(rv)) {

            /* Embedded NH entry */
            *embd = TRUE;
            /* Extract entry info. */
            BCM_IF_ERROR_RETURN(_bcm_td2_l3_ent_parse(unit, mem_ext, l3cfg,
                                                          nh_idx, buf_entry));
            /* Clear the HIT bit */
            if (clear_hit) {
                BCM_IF_ERROR_RETURN(_bcm_td2_l3_clear_hit(unit, mem_ext,
                                              l3cfg, buf_entry, l3cfg->l3c_hw_index));
            }
        }
    } 
     
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l3_entry_del
 * Purpose:
 *      Delete an entry from TD2 L3 host table.
 * Parameters:
 *      unit     - (IN) SOC unit number.
 *      l3cfg    - (IN/OUT) L3 entry  lookup key & search result.
 *      nh_index - (IN/OUT) Next hop index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_l3_entry_del(int unit, _bcm_l3_cfg_t *l3cfg)
{
    int ipv6;                          /* IPv6 entry indicator.*/
    soc_mem_t mem, mem_ext;            /* L3 table memory.     */  
    int rv = BCM_E_NONE;               /* Operation status.    */
    uint32 *buf_entry;                 /* Entry buffer ptrs    */
    l3_entry_ipv4_unicast_entry_t l3v4_entry;       /* IPv4 */
    l3_entry_ipv4_multicast_entry_t l3v4_ext_entry;   /* IPv4-Embedded */
    l3_entry_ipv6_unicast_entry_t l3v6_entry;       /* IPv6 */
    l3_entry_ipv6_multicast_entry_t l3v6_ext_entry;   /* IPv6-Embedded */

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    if (ipv6) {
        mem = BCM_XGS3_L3_MEM(unit, v6); mem_ext = BCM_XGS3_L3_MEM(unit, v6_4);
    } else {
        mem = BCM_XGS3_L3_MEM(unit, v4); mem_ext = BCM_XGS3_L3_MEM(unit, v4_2);
    }

    /* Assign entry-key buf based on table being used */
    BCM_TD2_L3_HOST_ENTRY_BUF(ipv6, mem, buf_entry,
                                        l3v4_entry,
                                        l3v4_ext_entry,
                                        l3v6_entry,
                                        l3v6_ext_entry);
    /* Prepare lookup key. */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_ent_init(unit, mem, l3cfg, buf_entry));

    rv = soc_mem_delete(unit, mem, MEM_BLOCK_ANY, (void*) buf_entry);

    if ((BCM_E_NOT_FOUND == rv) &&
        (soc_feature(unit, soc_feature_l3_extended_host_entry))) { 
        /* Delete from  the extended tables only if the extended host entry
         * feature is supported and if the entry was not found in the 
         * regular host entry */

        /* Assign entry-key buf based on table being used */
        BCM_TD2_L3_HOST_ENTRY_BUF(ipv6, mem_ext, buf_entry,
                                        l3v4_entry,
                                        l3v4_ext_entry,
                                        l3v6_entry,
                                        l3v6_ext_entry);
        /* Prepare lookup key. */
        BCM_IF_ERROR_RETURN(_bcm_td2_l3_ent_init(unit, mem_ext, l3cfg, buf_entry));
    
        rv = soc_mem_delete(unit, mem_ext, MEM_BLOCK_ANY, (void*) buf_entry);
    }

    if (rv >= 0) {
        (ipv6) ?  BCM_XGS3_L3_IP6_CNT(unit)-- : BCM_XGS3_L3_IP4_CNT(unit)--;
    }

    return rv;
}

STATIC INLINE int
_bcm_td2_ip_key_to_l3cfg(int unit, bcm_l3_key_t *ipkey, _bcm_l3_cfg_t *l3cfg)
{
    int ipv6;           /* IPV6 key.                     */

    /* Input parameters check */
    if ((NULL == ipkey) || (NULL == l3cfg)) {
        return (BCM_E_PARAM);
    }

    /* Reset destination buffer first. */
    sal_memset(l3cfg, 0, sizeof(_bcm_l3_cfg_t));

    ipv6 = (ipkey->l3k_flags & BCM_L3_IP6);

    /* Set vrf id. */
    l3cfg->l3c_vrf = ipkey->l3k_vrf;
    l3cfg->l3c_vid = ipkey->l3k_vid;

    if (ipv6) {
        if (BCM_IP6_MULTICAST(ipkey->l3k_ip6_addr)) {
            /* Copy ipv6 group, source & vid. */
            sal_memcpy(l3cfg->l3c_ip6, ipkey->l3k_ip6_addr, sizeof(bcm_ip6_t));
            sal_memcpy(l3cfg->l3c_sip6, ipkey->l3k_sip6_addr,
                       sizeof(bcm_ip6_t));
            l3cfg->l3c_vid = ipkey->l3k_vid;
            l3cfg->l3c_flags = (BCM_L3_IP6 | BCM_L3_IPMC);
        } else {
            /* Copy ipv6 address. */
            sal_memcpy(l3cfg->l3c_ip6, ipkey->l3k_ip6_addr, sizeof(bcm_ip6_t));
            l3cfg->l3c_flags = BCM_L3_IP6;
        }
    } else {
        if (BCM_IP4_MULTICAST(ipkey->l3k_ip_addr)) {
            /* Copy ipv4 mcast group, source & vid. */
            l3cfg->l3c_ip_addr = ipkey->l3k_ip_addr;
            l3cfg->l3c_src_ip_addr = ipkey->l3k_sip_addr;
            l3cfg->l3c_vid = ipkey->l3k_vid;
            l3cfg->l3c_flags = BCM_L3_IPMC;
        } else {
            /* Copy ipv4 address . */
            l3cfg->l3c_ip_addr = ipkey->l3k_ip_addr;
        }
    }
    return (BCM_E_NONE);
}

STATIC INLINE int
_bcm_td2_l3cfg_to_ipkey(int unit, bcm_l3_key_t *ipkey, _bcm_l3_cfg_t *l3cfg)
{
    /* Input parameters check */
    if ((NULL == ipkey) || (NULL == l3cfg)) {
        return (BCM_E_PARAM);
    }

    /* Reset destination buffer first. */
    sal_memset(ipkey, 0, sizeof(bcm_l3_key_t));

    ipkey->l3k_vrf = l3cfg->l3c_vrf;
    ipkey->l3k_vid = l3cfg->l3c_vid;

    if (l3cfg->l3c_flags & BCM_L3_IP6) {
        if (l3cfg->l3c_flags & BCM_L3_IPMC) {
            /* Copy ipv6 group, source & vid. */
            sal_memcpy(ipkey->l3k_ip6_addr, l3cfg->l3c_ip6, sizeof(bcm_ip6_t));
            sal_memcpy(ipkey->l3k_sip6_addr, l3cfg->l3c_sip6,
                       sizeof(bcm_ip6_t));
            ipkey->l3k_vid = l3cfg->l3c_vid;
        } else {
            /* Copy ipv6 address. */
            sal_memcpy(ipkey->l3k_ip6_addr, l3cfg->l3c_ip6, sizeof(bcm_ip6_t));
        }
    } else {
        if (l3cfg->l3c_flags & BCM_L3_IPMC) {
            /* Copy ipv4 mcast group, source & vid. */
            ipkey->l3k_ip_addr = l3cfg->l3c_ip_addr;
            ipkey->l3k_sip_addr = l3cfg->l3c_src_ip_addr;
            ipkey->l3k_vid = l3cfg->l3c_vid;
        } else {
            /* Copy ipv4 address . */
            ipkey->l3k_ip_addr = l3cfg->l3c_ip_addr;
        }
    }
    /* Store entry flags. */
    ipkey->l3k_flags = l3cfg->l3c_flags;
    return (BCM_E_NONE);
}

int
bcm_td2_l3_conflict_get(int unit, bcm_l3_key_t *ipkey, bcm_l3_key_t *cf_array,
                        int cf_max, int *cf_count)
{
    _bcm_l3_cfg_t l3cfg;
    uint32 valid, *bufp;      /* Hardware buffer ptr     */
    soc_mem_t mem;            /* L3 table memory type.   */  
    int nh_idx, index, rv = BCM_E_NONE;      /* Operation status.       */
    _bcm_l3_fields_t *fld;    /* L3 table common fields. */
    l3_entry_ipv4_unicast_entry_t l3v4_entry;    
    l3_entry_ipv4_multicast_entry_t l3v4_ext_entry;    
    l3_entry_ipv6_unicast_entry_t l3v6_entry;
    l3_entry_ipv6_multicast_entry_t l3v6_ext_entry;
    uint8 i, bidx, num_ent, shared_l3_banks;
    uint32 *keyt;
    uint32 l3_entry[SOC_MAX_MEM_WORDS];
    uint8 ipmc, ipv6, embedded_nh;
    uint32 key_type_1[] = {TD2_L3_HASH_KEY_TYPE_V4UC, -1};
    uint32 key_type_2[] = {TD2_L3_HASH_KEY_TYPE_V6UC, -1};
    uint32 key_type_3[] = {TD2_L3_HASH_KEY_TYPE_V4MC, TD2_L3_HASH_KEY_TYPE_V4UC_EXT};
    uint32 key_type_4[] = {TD2_L3_HASH_KEY_TYPE_V6MC, TD2_L3_HASH_KEY_TYPE_V6UC_EXT};

    /*  Make sure module was initialized. */
    if (!BCM_XGS3_L3_INITIALIZED(unit)) {
        return (BCM_E_INIT);
    }

    /* Input parameters check. */
    if ((NULL == ipkey) || (NULL == cf_count) || 
        (NULL == cf_array) || (cf_max <= 0)) {
        return (BCM_E_PARAM);
    }

    embedded_nh = ipkey->l3k_flags & BCM_L3_DEREFERENCED_NEXTHOP;
    nh_idx = (embedded_nh) ? BCM_XGS3_L3_INVALID_INDEX : 0;

    /* Translate lookup key to l3cfg format. */
    BCM_IF_ERROR_RETURN(_bcm_td2_ip_key_to_l3cfg(unit, ipkey, &l3cfg));
    
    /* Get entry type. */
    ipv6 = (l3cfg.l3c_flags & BCM_L3_IP6);
    ipmc = (l3cfg.l3c_flags & BCM_L3_IPMC);

    /* Get table memory. */
    BCM_TD2_L3_HOST_TABLE_MEM(unit, l3cfg.l3c_intf, ipv6, mem, nh_idx);
    /* Bump up the size conditionally */
    mem = (embedded_nh || ipmc) ? (ipv6 ? L3_ENTRY_IPV6_MULTICASTm : L3_ENTRY_IPV4_MULTICASTm) : mem;
    /* Set the key type field array */
    keyt = (embedded_nh || ipmc) ? (ipv6 ? key_type_4 : key_type_3) : 
                                   (ipv6 ? key_type_2 : key_type_1);
    /* Set the valid field */
    valid = VALID_0f;
    
    /* Set table fields */
    BCM_TD2_L3_HOST_TABLE_FLD(unit, mem, ipv6, fld);

    /* Assign entry buf based on table being used */
    BCM_TD2_L3_HOST_ENTRY_BUF(ipv6, mem, bufp,
                                    l3v4_entry,
                                    l3v4_ext_entry,
                                    l3v6_entry,
                                    l3v6_ext_entry);

    /* Prepare host entry for addition. */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_ent_init(unit, mem, &l3cfg, bufp));

    *cf_count = 0;

    /* get number of shared L3 banks */
    shared_l3_banks = (soc_mem_index_count(unit, L3_ENTRY_ONLYm) - 16 * 1024) /
                      (64 * 1024);

    if (shared_l3_banks > 3) {
        return BCM_E_NOT_FOUND;
    }
    /* bank 9 to 6 are dedicated L3 banks, 5 to 3 are dedicated, depends on
     * mode how many shared banks are assigned to l3 
     */
    for (bidx = 9; bidx <= (6 - shared_l3_banks); bidx--) {

        num_ent = 4; /* 4 entries per bucket */

        index = soc_td2_l3x_bank_entry_hash(unit, bidx, bufp);
        if (index == 0) { 
            return BCM_E_INTERNAL;
        }

        for (i = 0; (i < num_ent) && (*cf_count < cf_max);) {
            rv = soc_mem_read(unit, mem, COPYNO_ALL, index + i, l3_entry);
            if (SOC_FAILURE(rv)) {
                return BCM_E_MEMORY;
            }
            if (soc_mem_field32_get(unit, mem, &l3_entry, valid) &&
                ((soc_mem_field32_get(unit, mem, &l3_entry, KEY_TYPEf) == keyt[0]) ||
                 (soc_mem_field32_get(unit, mem, &l3_entry, KEY_TYPEf) == keyt[1]) ||
                 (soc_mem_field32_get(unit, mem, &l3_entry, KEY_TYPEf) == keyt[2]) ||
                 (soc_mem_field32_get(unit, mem, &l3_entry, KEY_TYPEf) == keyt[3]))) {
                switch(soc_mem_field32_get(unit, mem, &l3_entry, fld->key_type)) {
                case TD2_L3_HASH_KEY_TYPE_V4UC:
                      l3cfg.l3c_flags = 0;
                      break;
                case TD2_L3_HASH_KEY_TYPE_V4UC_EXT:
                      l3cfg.l3c_flags = BCM_L3_DEREFERENCED_NEXTHOP;
                      break;
                case TD2_L3_HASH_KEY_TYPE_V4MC:
                      l3cfg.l3c_flags = BCM_L3_IPMC;
                      break;
                case TD2_L3_HASH_KEY_TYPE_V6UC:
                      l3cfg.l3c_flags = BCM_L3_IP6;
                      break;
                case TD2_L3_HASH_KEY_TYPE_V6UC_EXT:
                      l3cfg.l3c_flags = BCM_L3_IP6 | BCM_L3_DEREFERENCED_NEXTHOP;
                      break;
                case TD2_L3_HASH_KEY_TYPE_V6MC:
                      l3cfg.l3c_flags = BCM_L3_IP6 | BCM_L3_IPMC;
                      break;
                default:
                      break;
                }
                BCM_IF_ERROR_RETURN(_bcm_td2_l3_ent_parse(unit, mem, &l3cfg,
                                                          NULL, &l3_entry));
                BCM_IF_ERROR_RETURN
                    (_bcm_td2_l3cfg_to_ipkey(unit, cf_array + (*cf_count), &l3cfg));
                if ((++(*cf_count)) >= cf_max) {
                    break;
                }
            }
            /* increment based upon existing entry */
            i += (mem == L3_ENTRY_IPV6_MULTICASTm) ? 2 : 1;
        }
    } 
    return BCM_E_NONE;
}



/*
 * Function:
 *      _bcm_td2_l3_get_host_ent_by_idx
 * Purpose:
 * Parameters:
 *      unit     - (IN)SOC unit number.
 *      dma_ptr  - (IN)Table pointer in dma. 
 *      soc_mem_ - (IN)Table memory.
 *      idx      - (IN)Index to read.
 *      l3cfg    - (OUT)l3 entry search result.
 *      nh_index - (OUT)Next hop index.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_l3_get_host_ent_by_idx(int unit, void *dma_ptr, soc_mem_t mem,
                                int idx, _bcm_l3_cfg_t *l3cfg, int *nh_idx)
{
    uint32 ipv6;                       /* IPv6 entry indicator.   */
    int key_type;
    int clear_hit;                     /* Clear hit bit flag.     */
    _bcm_l3_fields_t *fld;             /* L3 table common fields. */
    uint32 *buf_entry;                 /* Key and entry buffer ptrs*/
    l3_entry_ipv4_unicast_entry_t l3v4_entry;       /* IPv4 */
    l3_entry_ipv4_multicast_entry_t l3v4_ext_entry; /* IPv4-Embedded */
    l3_entry_ipv6_unicast_entry_t l3v6_entry;       /* IPv6 */
    l3_entry_ipv6_multicast_entry_t l3v6_ext_entry; /* IPv6-Embedded */
    soc_field_t uc_ipv6_lwr = INVALIDf, uc_ipv6_upr = INVALIDf;

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);
    /* Get clear hit flag. */
    clear_hit = l3cfg->l3c_flags & BCM_L3_HIT_CLEAR;

    BCM_TD2_L3_HOST_TABLE_FLD(unit, mem, ipv6, fld);
    BCM_TD2_L3_HOST_ENTRY_BUF(ipv6, mem, buf_entry,
                                    l3v4_entry,
                                    l3v4_ext_entry,
                                    l3v6_entry,
                                    l3v6_ext_entry);

    if (NULL == dma_ptr) { /* Read from hardware. */
        /* Zero buffers. */
        sal_memset(buf_entry, 0, BCM_L3_MEM_ENT_SIZE(unit, mem));

        /* Read entry from hw. */
        BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_READ(unit, mem, idx, buf_entry)); 
    } else { /* Read from dma. */
        buf_entry = soc_mem_table_idx_to_pointer(unit, mem,
                                   uint32 *, dma_ptr, idx);
    }

    if(!soc_mem_field32_get(unit, mem, buf_entry, fld->valid)) {
        return (BCM_E_NOT_FOUND);
    }

    key_type = soc_mem_field32_get(unit, mem, buf_entry, fld->key_type);
    l3cfg->l3c_flags = 0;
    switch (mem) {
    case L3_ENTRY_IPV4_UNICASTm:
        if (key_type != 0) {
            /* All other keys are invalid */
            return BCM_E_NOT_FOUND;
        }
        break;
    case L3_ENTRY_IPV4_MULTICASTm:
        if (key_type == 4) {
            l3cfg->l3c_flags = BCM_L3_IPMC;
        } else if (key_type == 1) {
            l3cfg->l3c_flags = 0;
        } else {
            return BCM_E_NOT_FOUND;
        }
        break;
    case L3_ENTRY_IPV6_UNICASTm:
        if (key_type == 2) {
            l3cfg->l3c_flags = BCM_L3_IP6;
            uc_ipv6_lwr = IPV6UC__IP_ADDR_LWR_64f;
            uc_ipv6_upr = IPV6UC__IP_ADDR_UPR_64f;
        } else {
            return BCM_E_NOT_FOUND;
        }
        break;
    case L3_ENTRY_IPV6_MULTICASTm:
        uc_ipv6_lwr =  IPV6UC_EXT__IP_ADDR_LWR_64f;
        uc_ipv6_upr =  IPV6UC_EXT__IP_ADDR_UPR_64f;
        if (key_type == 5) {
            l3cfg->l3c_flags = BCM_L3_IP6 | BCM_L3_IPMC;
        } if (key_type == 3) {
            l3cfg->l3c_flags = BCM_L3_IP6;
        } else {
            return BCM_E_NOT_FOUND;
        }
        break;
    default :
        return BCM_E_NOT_FOUND;
    }

    /* Ignore protocol mismatch & multicast entries. */
    if ((ipv6  != (l3cfg->l3c_flags & BCM_L3_IP6)) ||
        (l3cfg->l3c_flags & BCM_L3_IPMC)) {
        return (BCM_E_NONE);
    }

    if (ipv6) {
        /* Extract host info from the entry. */
        soc_mem_ip6_addr_get(unit, mem, buf_entry,
                              uc_ipv6_lwr, l3cfg->l3c_ip6,
                             SOC_MEM_IP6_LOWER_ONLY);

        soc_mem_ip6_addr_get(unit, mem, buf_entry,
                              uc_ipv6_upr, l3cfg->l3c_ip6,
                             SOC_MEM_IP6_UPPER_ONLY);
    }
    /* Set index to l3cfg. */
    l3cfg->l3c_hw_index = idx;

    if (!ipv6) {
        l3cfg->l3c_ip_addr = 
            soc_mem_field32_get(unit, mem, buf_entry, fld->ip4);
    }

    /* Parse entry data. */
    BCM_IF_ERROR_RETURN(
        _bcm_td2_l3_ent_parse(unit, mem, l3cfg, nh_idx, buf_entry));

    /* Clear the HIT bit */
    if (clear_hit) {
        BCM_IF_ERROR_RETURN(_bcm_td2_l3_clear_hit(unit, mem, l3cfg,
                                           (void *)buf_entry, idx));
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_td2_l3_ipmc_ent_init
 * Purpose:
 *      Set GROUP/SOURCE/VID/IMPC flag in the entry.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      l3x_entry - (IN/OUT) IPMC entry to fill. 
 *      l3cfg     - (IN) Api IPMC data. 
 * Returns:
 *    BCM_E_XXX
 */
STATIC void
_bcm_td2_l3_ipmc_ent_init(int unit, uint32 *buf_p,
                             _bcm_l3_cfg_t *l3cfg)
{
    soc_mem_t mem;                     /* IPMC table memory.    */
    int ipv6;                          /* IPv6 entry indicator. */
    int idx;                           /* Iteration index.      */
    soc_field_t v4typef[] = { KEY_TYPE_0f, KEY_TYPE_1f };
    soc_field_t v6typef[] = { KEY_TYPE_0f, KEY_TYPE_1f, 
                              KEY_TYPE_2f, KEY_TYPE_3f };
    soc_field_t validf[] = { VALID_0f, VALID_1f, VALID_2f, VALID_3f };

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory. */
    mem =  (ipv6) ? L3_ENTRY_IPV6_MULTICASTm : L3_ENTRY_IPV4_MULTICASTm;

    if (ipv6) {
        /* Set group address. */
        soc_mem_ip6_addr_set(unit, mem, buf_p, IPV6MC__GROUP_IP_ADDR_LWR_64f,
                             l3cfg->l3c_ip6, SOC_MEM_IP6_LOWER_ONLY);
        l3cfg->l3c_ip6[0] = 0x0;    /* Don't write ff entry already mcast. */
        soc_mem_ip6_addr_set(unit, mem, buf_p, IPV6MC__GROUP_IP_ADDR_UPR_56f, 
                             l3cfg->l3c_ip6, SOC_MEM_IP6_UPPER_ONLY);
        l3cfg->l3c_ip6[0] = 0xff;    /* Restore The entry  */

        /* Set source  address. */
        soc_mem_ip6_addr_set(unit, mem, buf_p, IPV6MC__SOURCE_IP_ADDR_LWR_64f, 
                             l3cfg->l3c_sip6, SOC_MEM_IP6_LOWER_ONLY);
        soc_mem_ip6_addr_set(unit, mem, buf_p, IPV6MC__SOURCE_IP_ADDR_UPR_64f, 
                             l3cfg->l3c_sip6, SOC_MEM_IP6_UPPER_ONLY);

        for (idx = 0; idx < 4; idx++) {
            /* Set key type */
            soc_mem_field32_set(unit, mem, buf_p, v6typef[idx], 
                                TD2_L3_HASH_KEY_TYPE_V6MC);

            /* Set entry valid bit. */
            soc_mem_field32_set(unit, mem, buf_p, validf[idx], 1);

        }

        /* Set vlan id or L3_IIF. */
        if (l3cfg->l3c_vid < 4096) {
            soc_mem_field32_set(unit, mem, buf_p, IPV6MC__VLAN_IDf, l3cfg->l3c_vid);
        } else {
            soc_mem_field32_set(unit, mem, buf_p, IPV6MC__L3_IIFf, l3cfg->l3c_vid);
        }

        /* Set virtual router id. */
        soc_mem_field32_set(unit, mem, buf_p, IPV6MC__VRF_IDf, l3cfg->l3c_vrf);

    } else {
        /* Set group id. */
        soc_mem_field32_set(unit, mem, buf_p, IPV4MC__GROUP_IP_ADDRf,
                            l3cfg->l3c_ip_addr);

        /* Set source address. */
        soc_mem_field32_set(unit, mem, buf_p, IPV4MC__SOURCE_IP_ADDRf,
                            l3cfg->l3c_src_ip_addr);

        for (idx = 0; idx < 2; idx++) {
            /* Set key type */
            soc_mem_field32_set(unit, mem, buf_p, v4typef[idx], 
                                TD2_L3_HASH_KEY_TYPE_V4MC);

            /* Set entry valid bit. */
            soc_mem_field32_set(unit, mem, buf_p, validf[idx], 1);
        }

        /* Set vlan id or L3_IIF. */
        if (l3cfg->l3c_vid < 4096) {
            soc_mem_field32_set(unit, mem, buf_p, IPV4MC__VLAN_IDf, l3cfg->l3c_vid);
        } else {
            soc_mem_field32_set(unit, mem, buf_p, IPV4MC__L3_IIFf, l3cfg->l3c_vid);
        }

        /* Set virtual router id. */
        soc_mem_field32_set(unit, mem, buf_p, IPV4MC__VRF_IDf, l3cfg->l3c_vrf);
    }

    return;
}

/*
 * Function:
 *      _bcm_td2_l3_ipmc_ent_parse
 * Purpose:
 *      Service routine used to parse hw l3 ipmc entry to api format.
 * Parameters:
 *      unit      - (IN)SOC unit number. 
 *      l3cfg     - (IN/OUT)l3 entry parsing destination buf.
 *      l3x_entry - (IN/OUT)hw buffer.
 * Returns:
 *      void
 */
STATIC int
_bcm_td2_l3_ipmc_ent_parse(int unit, _bcm_l3_cfg_t *l3cfg,
                          l3_entry_ipv6_multicast_entry_t *l3x_entry)
{
    soc_mem_t mem;                     /* IPMC table memory.    */
    uint32 *buf_p;                     /* HW buffer address.    */
    int ipv6;                          /* IPv6 entry indicator. */
    uint32 hit;                        /* composite hit = hit_x|hit_y */
    l3_entry_hit_only_x_entry_t hit_x;
    l3_entry_hit_only_y_entry_t hit_y;
    int idx, idx_max, idx_offset, hit_idx_shift;   /* Iteration index.      */
    soc_field_t hitf[] = {HIT_0f, HIT_1f, HIT_2f, HIT_3f};
    soc_field_t rpe, dst_discard, vrf_id, l3mc_idx, class_id, pri,
                rpa_id, expected_l3_iif;
    soc_field_t rpf_fail_drop, rpf_fail_tocpu;

    buf_p = (uint32 *)l3x_entry;

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory and table fields */
    if (ipv6) {
        mem         = L3_ENTRY_IPV6_MULTICASTm;
        pri         = IPV6MC__PRIf;
        rpe         = IPV6MC__RPEf;
        vrf_id      = IPV6MC__VRF_IDf;
        l3mc_idx    = IPV6MC__L3MC_INDEXf;
        class_id    = IPV6MC__CLASS_IDf;
        dst_discard = IPV6MC__DST_DISCARDf;
        rpa_id      = IPV6MC__RPA_IDf;
        expected_l3_iif = IPV6MC__EXPECTED_L3_IIFf;
        rpf_fail_drop   = IPV6MC__IPMC_EXPECTED_L3_IIF_MISMATCH_DROPf;
        rpf_fail_tocpu  = IPV6MC__IPMC_EXPECTED_L3_IIF_MISMATCH_TOCPUf;
        idx_max     = 4;
        hit_idx_shift = 0;
        idx_offset = 0;
    } else {
        mem         = L3_ENTRY_IPV4_MULTICASTm;
        pri         = IPV4MC__PRIf;
        rpe         = IPV4MC__RPEf;
        vrf_id      = IPV4MC__VRF_IDf;
        l3mc_idx    = IPV4MC__L3MC_INDEXf;
        class_id    = IPV4MC__CLASS_IDf;
        dst_discard = IPV4MC__DST_DISCARDf;
        rpa_id      = IPV4MC__RPA_IDf;
        expected_l3_iif = IPV4MC__EXPECTED_L3_IIFf;
        rpf_fail_drop   = IPV4MC__IPMC_EXPECTED_L3_IIF_MISMATCH_DROPf;
        rpf_fail_tocpu  = IPV4MC__IPMC_EXPECTED_L3_IIF_MISMATCH_TOCPUf;
        idx_max     = 2;
        hit_idx_shift = 1;
        idx_offset = (l3cfg->l3c_hw_index & 0x1) << 1;
    }

    /* Mark entry as multicast & clear rest of the flags. */
    l3cfg->l3c_flags = BCM_L3_IPMC;
    if (ipv6) {
       l3cfg->l3c_flags |= BCM_L3_IP6;
    }

    BCM_IF_ERROR_RETURN(
        BCM_XGS3_MEM_READ(unit, L3_ENTRY_HIT_ONLY_Xm,
          (l3cfg->l3c_hw_index >> hit_idx_shift), &hit_x));

    BCM_IF_ERROR_RETURN(
        BCM_XGS3_MEM_READ(unit, L3_ENTRY_HIT_ONLY_Ym,
          (l3cfg->l3c_hw_index >> hit_idx_shift), &hit_y));

    hit = 0;
    /* Read hit value. */
    for (idx = idx_offset; idx < (idx_offset + idx_max); idx++) {
        hit |= soc_mem_field32_get(unit, L3_ENTRY_HIT_ONLY_Xm,
                                   &hit_x, hitf[idx]);
    }

    for (idx = idx_offset; idx < (idx_offset + idx_max); idx++) {
        hit |= soc_mem_field32_get(unit, L3_ENTRY_HIT_ONLY_Ym,
                                   &hit_y, hitf[idx]);
    }

    if (hit) {
        l3cfg->l3c_flags |= BCM_L3_HIT;
    }

    /* Set ipv6 group address to multicast. */
    if (ipv6) {
        l3cfg->l3c_ip6[0] = 0xff;   /* Set entry ip to mcast address. */
    }

    /* Read priority override */
    if(soc_mem_field32_get(unit, mem, buf_p, rpe)) { 
        l3cfg->l3c_flags |= BCM_L3_RPE;
    }

    /* Read destination discard bit. */
    if(soc_mem_field32_get(unit, mem, buf_p, dst_discard)) { 
        l3cfg->l3c_flags |= BCM_L3_DST_DISCARD;
    }

    /* Read Virtual Router Id. */
    l3cfg->l3c_vrf = soc_mem_field32_get(unit, mem, buf_p, vrf_id);

    /* Pointer to ipmc replication table. */
    l3cfg->l3c_ipmc_ptr = soc_mem_field32_get(unit, mem, buf_p, l3mc_idx);

    /* Classification lookup class id. */
    l3cfg->l3c_lookup_class = soc_mem_field32_get(unit, mem, buf_p, class_id);

    /* Read priority value. */
    l3cfg->l3c_prio = soc_mem_field32_get(unit, mem, buf_p, pri);

    /* Read RPA_ID value. */
    l3cfg->l3c_rp_id = soc_mem_field32_get(unit, mem, buf_p, rpa_id);
    if (l3cfg->l3c_rp_id == 0 &&
        (l3cfg->l3c_vid != 0 ||
         soc_mem_field32_get(unit, mem, buf_p, expected_l3_iif) != 0)) {
        /* If RPA_ID field value is 0, it could mean (1) this IPMC entry is
         * not a PIM-BIDIR entry, or (2) this entry is a PIM-BIDIR entry and
         * is associated with rendezvous point 0. If either the L3 IIF in the
         * key or the expected L3 IIF in the data is not zero, this
         * entry is not a PIM-BIDIR entry.
         */ 
        l3cfg->l3c_rp_id = BCM_IPMC_RP_ID_INVALID;
    }

    /* Read expected iif */
    l3cfg->l3c_intf = soc_mem_field32_get(unit, mem, buf_p, expected_l3_iif);
    if (0 != l3cfg->l3c_intf) {
        l3cfg->l3c_flags |= BCM_IPMC_POST_LOOKUP_RPF_CHECK;
    }
    if (soc_mem_field32_get(unit, mem, buf_p, rpf_fail_drop)) {
        l3cfg->l3c_flags |= BCM_IPMC_RPF_FAIL_DROP;
    }
    if (soc_mem_field32_get(unit, mem, buf_p, rpf_fail_tocpu)) {
        l3cfg->l3c_flags |= BCM_IPMC_RPF_FAIL_TOCPU;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_host_stat_get_table_info
 * Description:
 *      Provides relevant flex table information(table-name,index with
 *      direction)  for given L3 Host entry
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      port             - (IN) GPORT ID
 *      num_of_tables    - (OUT) Number of flex counter tables
 *      table_info       - (OUT) Flex counter tables information
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
STATIC bcm_error_t
_bcm_td2_l3_host_stat_get_table_info(
                   int                        unit,
                   bcm_l3_host_t              *info,
                   uint32                     *num_of_tables,
                   bcm_stat_flex_table_info_t *table_info)
{
    int rv, embd;
    soc_mem_t mem;          
    _bcm_l3_cfg_t l3cfg;     

    if (NULL == info) {
        return (BCM_E_PARAM);
    }

    if (!(soc_feature(unit, soc_feature_l3_extended_host_entry))) {
        return BCM_E_UNAVAIL;
    }

    mem = INVALIDm;
    /* Vrf is in device supported range check. */
    if ((info->l3a_vrf > SOC_VRF_MAX(unit)) || 
        (info->l3a_vrf < BCM_L3_VRF_DEFAULT)) {
        return (BCM_E_PARAM);
    }
  
    sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
    l3cfg.l3c_flags = info->l3a_flags;
    l3cfg.l3c_vrf = info->l3a_vrf;

    if (info->l3a_flags & BCM_L3_IP6) {
        sal_memcpy(l3cfg.l3c_ip6, info->l3a_ip6_addr, BCM_IP6_ADDRLEN);
    } else {
        l3cfg.l3c_ip_addr = info->l3a_ip_addr;
    }
   
    rv = _bcm_td2_l3_entry_get(unit, &l3cfg, NULL, &embd);
    if (BCM_SUCCESS(rv)) { /* Entry found */
        mem = (l3cfg.l3c_flags & BCM_L3_IP6) ? L3_ENTRY_IPV6_MULTICASTm :
                                               L3_ENTRY_IPV4_MULTICASTm;       
        table_info[*num_of_tables].table = mem;
        table_info[*num_of_tables].index = l3cfg.l3c_hw_index;
        table_info[*num_of_tables].direction = bcmStatFlexDirectionIngress;
        (*num_of_tables)++;
    }
    
    return rv;
}

/*----- END STATIC FUNCS ----- */

/*
 * Function:
 *      _bcm_td2_l3_add
 * Purpose:
 *      Add and entry to TD2 L3 host table.
 *      The decision to add to is based on 
 *      switch control(s) and table full conditions.
 *      conditions.
 * Parameters:
 *      unit   - (IN) SOC unit number.
 *      l3cfg  - (IN) L3 entry info.
 *      nh_idx - (IN) Next hop index.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_l3_add(int unit, _bcm_l3_cfg_t *l3cfg, int nh_idx)
{
    int rv;

    rv = _bcm_td2_l3_entry_add(unit, l3cfg, nh_idx);
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l3_get
 * Purpose:
 *      Get an entry from TD2 L3 host table.
 * Parameters:
 *      unit     - (IN) SOC unit number.
 *      l3cfg    - (IN/OUT) L3 entry  lookup key & search result.
 *      nh_index - (IN/OUT) Next hop index.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_l3_get(int unit, _bcm_l3_cfg_t *l3cfg, int *nh_idx)
{
    int rv;
    int embd = _BCM_TD2_HOST_ENTRY_NOT_FOUND;

    rv = _bcm_td2_l3_entry_get(unit, l3cfg, nh_idx, &embd);
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l3_del
 * Purpose:
 *      Del an entry from TD2 L3 host table.
 * Parameters:
 *      unit     - (IN) SOC unit number.
 *      l3cfg    - (IN/OUT) L3 entry  lookup key & search result.
 *      nh_index - (IN/OUT) Next hop index.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_l3_del(int unit, _bcm_l3_cfg_t *l3cfg)
{
    int rv;

    /* Attempt to delete entry */
    rv = _bcm_td2_l3_entry_del(unit, l3cfg);
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l3_traverse
 * Purpose:
 *      Walk over range of entries in l3 host table.
 * Parameters:
 *      unit - SOC unit number.
 *      flags - L3 entry flags to match during traverse.  
 *      start - First index to read. 
 *      end - Last index to read.
 *      cb  - Caller notification callback.
 *      user_data - User cookie, which should be returned in cb. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_l3_traverse(int unit, int flags, uint32 start, uint32 end,
                      bcm_l3_host_traverse_cb cb, void *user_data)
{
    int rv = BCM_E_NONE;
    int ipv6;                   /* Protocol IPv6/IPv4          */
    int nh_idx;                 /* Next hop index.                 */
    int total = 0;              /* Total number of entries walked. */
    int table_size;             /* Number of entries in the table. */
    int table_ent_size;         /* Table entry size. */
    bcm_l3_host_t info;         /* Callback info buffer.           */
    _bcm_l3_cfg_t l3cfg;        /* HW entry info.                  */
    char *l3_tbl_ptr = NULL;    /* Table dma pointer.              */
    int tbl, idx, idx_min, idx_max;  /* Min and Max iteration index */
    int num_tables = 2;
    soc_mem_t mem[2] = {INVALIDm, INVALIDm};

    /* If no callback provided, we are done.  */
    if (NULL == cb) {
        return (BCM_E_NONE);
    }

    ipv6 = (flags & BCM_L3_IP6) ? TRUE : FALSE;
    /* If table is empty -> there is nothing to read. */
    if (ipv6 && (!BCM_XGS3_L3_IP6_CNT(unit))) {
        return (BCM_E_NONE);
    }
    if (!ipv6 && (!BCM_XGS3_L3_IP4_CNT(unit))) {
        return (BCM_E_NONE);
    }

    if (ipv6) {
        mem[0] = BCM_XGS3_L3_MEM(unit, v6); 
        if (soc_feature(unit, soc_feature_l3_extended_host_entry)) {
            mem[1] = BCM_XGS3_L3_MEM(unit, v6_4); 
        }
    } else {
        mem[0] = BCM_XGS3_L3_MEM(unit, v4); 
        if (soc_feature(unit, soc_feature_l3_extended_host_entry)) {
            mem[1] = BCM_XGS3_L3_MEM(unit, v4_2); 
        }
    }

    for(tbl = 0; tbl < num_tables; tbl++) {
  
        if (INVALIDm == mem[tbl]) {
            continue;
        }
        idx_max = soc_mem_index_max(unit, mem[tbl]);
        idx_min =  soc_mem_index_min(unit, mem[tbl]);
        table_ent_size = BCM_L3_MEM_ENT_SIZE(unit, mem[tbl]);

        BCM_IF_ERROR_RETURN (bcm_xgs3_l3_tbl_dma(unit, mem[tbl], table_ent_size, 
                                      "l3_tbl", &l3_tbl_ptr, &table_size));

        /* Input index`s sanity. */
        if (start > (uint32)table_size || start > end) {
            if(l3_tbl_ptr) {
                soc_cm_sfree(unit, l3_tbl_ptr);
            }
            return (BCM_E_NOT_FOUND);
        }

        /* Iterate over all the entries - show matching ones. */
        for (idx = idx_min; idx <= idx_max; idx++) {
            /* Reset buffer before read. */
            sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
            l3cfg.l3c_flags = flags;

            /* Get entry from  hw. */
            rv = _bcm_td2_l3_get_host_ent_by_idx(unit, l3_tbl_ptr, mem[tbl],
                                                  idx, &l3cfg, &nh_idx);

            /* Check for read errors & invalid entries. */
            if (rv < 0) {
                if (rv != BCM_E_NOT_FOUND) {
                    break;
                }
                continue;
            }

            /* Check read entry flags & update index accordingly. */
            if (BCM_L3_CMP_EQUAL !=
                _bcm_xgs3_trvrs_flags_cmp(unit, flags, l3cfg.l3c_flags, &idx)) {
                continue;
            }

            /* Valid entry found -> increment total table_sizeer */
            total++;
            if ((uint32)total < start) {
                continue;
            }

            /* Don't read beyond last required index. */
            if ((uint32)total > end) {
                break;
            }

            /* Get next hop info. */
            rv = _bcm_xgs3_l3_get_nh_info(unit, &l3cfg, nh_idx);
            if (rv < 0) {
                break;
            }

            /* Fill host info. */
            _bcm_xgs3_host_ent_init(unit, &l3cfg, TRUE, &info);
            if (cb) {
                rv = (*cb) (unit, total, &info, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
                if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                    break;
                }
#endif
            }
        }

        if (l3_tbl_ptr) {
            soc_cm_sfree(unit, l3_tbl_ptr);
        }
    }

    /* Reset last read status. */
    if (BCM_E_NOT_FOUND == rv) {
        rv = BCM_E_NONE;
    }

    return (rv);

}

/*
 * Function:
 *      _bcm_td2_l3_host_stat_attach
 * Description:
 *      Attach counters entries to the given L3 host entry
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 host description  
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *      On BCM56640, the L3 host tables (tables L3_ENTRY_2 and L3_ENTRY_4, 
 *      table EXT_L3_UCAST_DATA_WIDE) may contain a flex stat pointer when 
 *      entry has a dereference next hop (no NEXT_HOP pointer, but the NH info 
 *      instead).  This counter is analogous to what would be in the egress 
 *      object, but is implemented in the ingress and so requires an ingress 
 *      counter pool allocated.
 *
 */
bcm_error_t
_bcm_td2_l3_host_stat_attach(
            int unit,
            bcm_l3_host_t *info,
            uint32 stat_counter_id)
{
    soc_mem_t                 table[4]={0};
    uint32                    flex_table_index=0;
    bcm_stat_flex_direction_t direction=bcmStatFlexDirectionIngress;
    uint32                    pool_number=0;
    uint32                    base_index=0;
    bcm_stat_flex_mode_t      offset_mode=0;
    bcm_stat_object_t         object=bcmStatObjectIngPort;
    bcm_stat_group_mode_t     group_mode= bcmStatGroupModeSingle;
    uint32                    count=0;
    uint32                     actual_num_tables=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    _bcm_esw_stat_get_counter_id_info(
                  stat_counter_id,
                  &group_mode,&object,&offset_mode,&pool_number,&base_index);

        /* Validate object id first */
    if (!((object >= bcmStatObjectIngPort) &&
          (object <= bcmStatObjectEgrL2Gre))) {
           SOC_DEBUG_PRINT((DK_PORT,
                            "Invalid bcm_stat_object_t passed %d \n",object));
           return BCM_E_PARAM;
    }
    /* Validate group_mode */
    if(!((group_mode >= bcmStatGroupModeSingle) &&
         (group_mode <= bcmStatGroupModeDvpType))) {
          SOC_DEBUG_PRINT((DK_PORT,
                           "Invalid bcm_stat_group_mode_t passed %d \n",
                           group_mode));
          return BCM_E_PARAM;
    }
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_get_table_info(
                        unit,object,4,&actual_num_tables,&table[0],&direction));

    BCM_IF_ERROR_RETURN(_bcm_td2_l3_host_stat_get_table_info(
                   unit, info,&num_of_tables,&table_info[0]));
    for (count=0; count < num_of_tables ; count++) {
         for (flex_table_index=0; 
              flex_table_index < actual_num_tables ; 
              flex_table_index++) {
              if ((table_info[count].direction == direction) &&
                  (table_info[count].table == table[flex_table_index]) ) {
                  if (direction == bcmStatFlexDirectionIngress) {
                      return _bcm_esw_stat_flex_attach_ingress_table_counters(
                             unit,
                             table_info[count].table,
                             table_info[count].index,
                             offset_mode,
                             base_index,
                             pool_number);
                  } else {
                      return _bcm_esw_stat_flex_attach_egress_table_counters(
                             unit,
                             table_info[count].table,
                             table_info[count].index,
                             offset_mode,
                             base_index,
                             pool_number);
                  }
              }
         }
    }
    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *      _bcm_td2_l3_host_stat_detach
 * Description:
 *      Detach counters entries to the given L3 host entry
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info		 - (IN) L3 host description
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t
_bcm_td2_l3_host_stat_detach(
            int unit,
            bcm_l3_host_t *info)
{
    uint32                     count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    bcm_error_t                rv[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] =
                                  {BCM_E_FAIL};
    uint32                     flag[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] = {0};

    BCM_IF_ERROR_RETURN(_bcm_td2_l3_host_stat_get_table_info(
                   unit, info,&num_of_tables,&table_info[0]));

    for (count=0; count < num_of_tables ; count++) {
      if (table_info[count].direction == bcmStatFlexDirectionIngress) {
           rv[bcmStatFlexDirectionIngress]=
                    _bcm_esw_stat_flex_detach_ingress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index);
           flag[bcmStatFlexDirectionIngress] = 1;
      } else {
           rv[bcmStatFlexDirectionEgress] =
                     _bcm_esw_stat_flex_detach_egress_table_counters(
                          unit,
                          table_info[count].table,
                          table_info[count].index);
           flag[bcmStatFlexDirectionIngress] = 1;
      }
    }
    if ((rv[bcmStatFlexDirectionIngress] == BCM_E_NONE) ||
        (rv[bcmStatFlexDirectionEgress] == BCM_E_NONE)) {
         return BCM_E_NONE;
    }
    if (flag[bcmStatFlexDirectionIngress] == 1) {
        return rv[bcmStatFlexDirectionIngress];
    }
    if (flag[bcmStatFlexDirectionEgress] == 1) {
        return rv[bcmStatFlexDirectionEgress];
    }
    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *      _bcm_td2_l3_host_stat_counter_get
 * Description:
 *      Get counter statistic values for specific L3 host entry
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 host description
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t 
_bcm_td2_l3_host_stat_counter_get(
            int unit,
            bcm_l3_host_t *info,
            bcm_l3_stat_t stat,
            uint32 num_entries,
            uint32 *counter_indexes,
            bcm_stat_value_t *counter_values)
{
    uint32                          table_count=0;
    uint32                          index_count=0;
    uint32                          num_of_tables=0;
    bcm_stat_flex_direction_t       direction=bcmStatFlexDirectionIngress;
    uint32                          byte_flag=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    if ((stat == bcmL3StatInPackets) ||
        (stat == bcmL3StatInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         /* direction = bcmStatFlexDirectionEgress; */
         return BCM_E_PARAM;
    }
    if (stat == bcmL3StatInPackets) {
        byte_flag=0;
    } else {
        byte_flag=1; 
    }

    BCM_IF_ERROR_RETURN(_bcm_td2_l3_host_stat_get_table_info(
                   unit, info,&num_of_tables,&table_info[0]));

    for (table_count=0; table_count < num_of_tables ; table_count++) {
      if (table_info[table_count].direction == direction) {
          for (index_count=0; index_count < num_entries ; index_count++) {
               /*ctr_offset_info.offset_index = counter_indexes[index_count];*/
            BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_get(
                         unit,
                         table_info[table_count].index,
                         table_info[table_count].table,
                         byte_flag,
                         counter_indexes[index_count],
                         &counter_values[index_count]));
          }
      }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_host_stat_counter_set
 * Description:
 *      Set counter statistic values for specific L3 host entry
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 host description
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (IN) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t 
_bcm_td2_l3_host_stat_counter_set(
            int unit,
            bcm_l3_host_t *info,
            bcm_l3_stat_t stat,
            uint32 num_entries,
            uint32 *counter_indexes,
            bcm_stat_value_t *counter_values)
{
    uint32                          table_count=0;
    uint32                          index_count=0;
    uint32                          num_of_tables=0;
    bcm_stat_flex_direction_t       direction=bcmStatFlexDirectionIngress;
    uint32                          byte_flag=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    if ((stat == bcmL3StatInPackets) ||
        (stat == bcmL3StatInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         /* direction = bcmStatFlexDirectionEgress; */
         return BCM_E_PARAM;
    }
    if (stat == bcmL3StatInPackets) {
        byte_flag=0;
    } else {
        byte_flag=1; 
    }

    BCM_IF_ERROR_RETURN(_bcm_td2_l3_host_stat_get_table_info(
                   unit, info,&num_of_tables,&table_info[0]));

    for (table_count=0; table_count < num_of_tables ; table_count++) {
      if (table_info[table_count].direction == direction) {
          for (index_count=0; index_count < num_entries ; index_count++) {
            BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_set(
                                unit,
                                table_info[table_count].index,
                                table_info[table_count].table,
                                byte_flag,
                                counter_indexes[index_count],
                                &counter_values[index_count]));
          }
      }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_host_stat_id_get
 * Description:
 *      Get stat counter id associated with given L3 host entry
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info             - (IN) L3 host description
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      Stat_counter_id  - (OUT) Stat Counter ID
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t 
_bcm_td2_l3_host_stat_id_get(
            int unit,
            bcm_l3_host_t *info,
            bcm_l3_stat_t stat,
            uint32 *stat_counter_id)
{
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32                     index=0;
    uint32                     num_stat_counter_ids=0;

    if ((stat == bcmL3StatInPackets) ||
        (stat == bcmL3StatInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         /* direction = bcmStatFlexDirectionEgress; */
         return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_td2_l3_host_stat_get_table_info(
                        unit,info,&num_of_tables,&table_info[0]));
    for (index=0; index < num_of_tables ; index++) {
         if (table_info[index].direction == direction)
             return _bcm_esw_stat_flex_get_counter_id(
                                  unit, 1, &table_info[index],
                                  &num_stat_counter_ids,stat_counter_id);
    }
    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm_td2_l3_replace
 * Purpose:
 *      Replace an entry in TD2 L3 host table.
 * Parameters:
 *      unit -  (IN)SOC unit number.
 *      l3cfg - (IN)L3 entry information.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_l3_replace(int unit, _bcm_l3_cfg_t *l3cfg)
{
    
    _bcm_l3_cfg_t entry;        /* Original l3 entry.             */
    int nh_idx_old;             /* Original entry next hop index. */
    int nh_idx_new;             /* New allocated next hop index   */
    int rv = BCM_E_UNAVAIL;     /* Operation return status.       */
    int embd_old, embd_new=0;

    /*  Make sure module was initialized. */
    if (!BCM_XGS3_L3_INITIALIZED(unit)) {
        return (BCM_E_INIT);
    }

    /* Input parameters check. */
    if (NULL == l3cfg) {
        return (BCM_E_PARAM); 
    }

    /* Set lookup key. */
    entry = *l3cfg;

    if (BCM_XGS3_L3_MCAST_ENTRY(l3cfg)) {
        /* Check if identical entry exits. */
        if (BCM_XGS3_L3_HWCALL_CHECK(unit, ipmc_get)) {
            BCM_XGS3_L3_MODULE_LOCK(unit);
            rv = BCM_XGS3_L3_HWCALL_EXEC(unit, ipmc_get) (unit, &entry);
            BCM_XGS3_L3_MODULE_UNLOCK(unit);
        }
        if (rv < 0) {
            return rv;
        }

        /* Write mcast entry to hw. */
        l3cfg->l3c_hw_index = entry.l3c_hw_index;       /*Replacement index & flag. */
        if (BCM_XGS3_L3_HWCALL_CHECK(unit, ipmc_add)) {
            BCM_XGS3_L3_MODULE_LOCK(unit);
            rv = BCM_XGS3_L3_HWCALL_EXEC(unit, ipmc_add) (unit, l3cfg);
            BCM_XGS3_L3_MODULE_UNLOCK(unit);
        } else {
            return (BCM_E_UNAVAIL);
        }
    } else {
        if (!(BCM_XGS3_L3_EGRESS_MODE_ISSET(unit))) {
            /* Trunk id validation. */
            BCM_XGS3_L3_IF_INVALID_TGID_RETURN(unit, l3cfg->l3c_flags,
                                               l3cfg->l3c_port_tgid);
        }

        /* Check if identical entry exits. */
        rv = _bcm_td2_l3_entry_get(unit, &entry, &nh_idx_old, &embd_old);
        if ((BCM_E_NOT_FOUND == rv) || (BCM_E_DISABLED == rv)) {
            return  bcm_xgs3_host_as_route(unit, l3cfg, BCM_XGS3_L3_OP_ADD, rv);
        } else if (BCM_FAILURE(rv)) {
            return rv;
        }

        embd_new = BCM_TD2_L3_USE_EMBEDDED_NEXT_HOP(unit, l3cfg->l3c_intf, nh_idx_old);
        if (BCM_GPORT_IS_BLACK_HOLE(l3cfg->l3c_port_tgid)) {
             nh_idx_new = 0;
        } else if (!embd_new){ /* If new entry is in indexed NH table */
              /* Get next hop index. */
              BCM_IF_ERROR_RETURN
                   (_bcm_xgs3_nh_init_add(unit, l3cfg, NULL, &nh_idx_new));
        } else if (embd_new) { /* If new entry is in embedded NH table */
            nh_idx_new = 0;
        }

        /* Write entry to hw. */
        l3cfg->l3c_hw_index = entry.l3c_hw_index; /*Replacement index & flag. */
        if (BCM_XGS3_L3_HWCALL_CHECK(unit, l3_add)) {
            BCM_XGS3_L3_MODULE_LOCK(unit);
            rv = BCM_XGS3_L3_HWCALL_EXEC(unit, l3_add)(unit, l3cfg, nh_idx_new);
            BCM_XGS3_L3_MODULE_UNLOCK(unit);
        } else {
            rv = BCM_E_UNAVAIL;
        }

        /* Do best effort clean_up if needed. */
        if (BCM_FAILURE(rv)) {
            bcm_xgs3_nh_del(unit, 0, nh_idx_new);
        }

        /* Free original next hop index. */
        if (entry.l3c_flags & BCM_L3_MULTIPATH) {
            /* Destroy ecmp group only if previous route is multipath */
            BCM_IF_ERROR_RETURN(bcm_xgs3_ecmp_group_del(unit, nh_idx_old));
        } else {
            BCM_IF_ERROR_RETURN(bcm_xgs3_nh_del(unit, 0, nh_idx_old));
        }
    }
    return rv;
}

/*
 * Function:
 *      bcm_td2_l3_del_intf
 * Purpose:
 *      Delete all the entries in TD2 L3 host table with
 *      NH matching a certain interface.
 * Parameters:
 *      unit   - (IN)SOC unit number.
 *      l3cfg  - (IN)Pointer to memory for l3 table related information.
 *      negate - (IN)0 means interface match; 1 means not match
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_l3_del_intf(int unit, _bcm_l3_cfg_t *l3cfg, int negate)
{
    int rv;                             /* Operation return status.      */
    int tmp_rv;                         /* First error occured.          */
    bcm_if_t intf;                      /* Deleted interface id.         */
    int nh_index;                       /* Next hop index.               */
    bcm_l3_egress_t egr;                /* Egress object.                */
    _bcm_if_del_pattern_t pattern;      /* Interface & negate combination 
                                           to be used for every l3 entry
                                           comparision.                  */

    /*  Make sure module was initialized. */
    if (!BCM_XGS3_L3_INITIALIZED(unit)) {
        return (BCM_E_INIT);
    }

    /* Input parameters check */
    if (NULL == l3cfg) {
        return (BCM_E_PARAM);
    }

    intf = l3cfg->l3c_intf;

    /* Check l3 switching mode. */ 
    if (BCM_XGS3_L3_EGRESS_MODE_ISSET(unit)) {
        if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, intf) || 
            BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, intf)) {
            if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, intf)) {
                nh_index = intf - BCM_XGS3_EGRESS_IDX_MIN; 
            } else {
                nh_index = intf - BCM_XGS3_DVP_EGRESS_IDX_MIN;
            }
            /* Get egress object information. */
            BCM_IF_ERROR_RETURN(bcm_xgs3_nh_get(unit, nh_index, &egr));
            /* Use egress object interface for iteration */
            intf = egr.intf;
        }
    }

    pattern.l3_intf = intf;
    pattern.negate = negate;

    /* Delete all ipv4 entries matching the interface. */
    tmp_rv = _bcm_td2_l3_del_match(unit, 0, (void *)&pattern,
                                    _bcm_xgs3_l3_intf_cmp, NULL, NULL);

    /* Delete all ipv6 entries matching the interface. */
    rv = _bcm_td2_l3_del_match(unit, BCM_L3_IP6, (void *)&pattern,
                                _bcm_xgs3_l3_intf_cmp, NULL, NULL);

    return (tmp_rv < 0) ? tmp_rv : rv;
}

/*
 * Function:
 *      _bcm_td2_l3_get_by_idx
 * Purpose:
 *      Get an entry from L3 table by index.
 * Parameters:
 *      unit     - (IN)SOC unit number.
 *      dma_ptr  - (IN)Table pointer in dma. 
 *      idx      - (IN)Index to read.
 *      l3cfg    - (OUT)l3 entry search result.
 *      nh_index - (OUT)Next hop index.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_l3_get_by_idx(int unit, void *dma_ptr, int idx,
                        _bcm_l3_cfg_t *l3cfg, int *nh_idx)
{
    int v6;
    soc_mem_t mem;
    int rv = BCM_E_UNAVAIL;     /* Operation return code. */

    /*  Make sure module was initialized. */
    if (!BCM_XGS3_L3_INITIALIZED(unit)) {
        return (BCM_E_INIT);
    }

    if (l3cfg->l3c_flags & BCM_L3_IPMC) {
        /* Get multicast entry. */
        rv = _bcm_td2_l3_ipmc_get_by_idx(unit, NULL, idx, l3cfg);
    } else {
        /* Get unicast entry. */
        v6 = (l3cfg->l3c_flags & BCM_L3_IP6);  
        mem = (v6) ? L3_ENTRY_IPV6_UNICASTm : L3_ENTRY_IPV4_UNICASTm;
        rv = _bcm_td2_l3_get_host_ent_by_idx(unit, NULL, mem, idx, l3cfg, nh_idx);
    }
    return rv;
}


/*
 * Function:
 *      _bcm_td2_l3_del_match
 * Purpose:
 *      Delete an entry in L3 table matches a certain rule.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      flags.    - (IN)Generic l3 flags. 
 *      pattern   - (IN)Comparison match argurment.
 *      cmp_func  - (IN)Comparison function.
 *      notify_cb - (IN)Delete notification callback. 
 *      user_data - (IN)User provided cookie for callback.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_l3_del_match(int unit, int flags, void *pattern,
                       bcm_xgs3_ent_op_cb cmp_func,
                       bcm_l3_host_traverse_cb notify_cb, void *user_data)
{
    int rv = BCM_E_NONE;
    int ipv6;             /* IPv6/IPv4 lookup flag. */
    int nh_idx;           /* Next hop index.        */
    int tbl, idx;         /* Iteration indices.     */
    int cmp_result;       /* Compare against pattern result. */
    bcm_l3_host_t info;   /* Host cb buffer.        */
    _bcm_l3_cfg_t l3cfg;  /* Hw entry info.         */
    int idx_max;
    int num_tables = 2;  
    soc_mem_t mem[2] = {INVALIDm, INVALIDm};

    /* Check Protocol. */
    ipv6 = (flags & BCM_L3_IP6) ? TRUE : FALSE;

    if (ipv6) {
        mem[0] = BCM_XGS3_L3_MEM(unit, v6); 
        if (soc_feature(unit, soc_feature_l3_extended_host_entry)) {
            mem[1] = BCM_XGS3_L3_MEM(unit, v6_4); 
        }
    } else {
        mem[0] = BCM_XGS3_L3_MEM(unit, v4); 
        if (soc_feature(unit, soc_feature_l3_extended_host_entry)) {
            mem[1] = BCM_XGS3_L3_MEM(unit, v4_2); 
        }
    }

    for (tbl = 0; tbl < num_tables; tbl++) {
        if (INVALIDm == mem[tbl]) {
            continue;
        }
        idx_max = soc_mem_index_max(unit, mem[tbl]);
        
        for (idx = 0; idx <= idx_max; idx++) {

            /* Set protocol. */
            sal_memset(&l3cfg, 0, sizeof(_bcm_l3_cfg_t));
            l3cfg.l3c_flags = flags;
            rv = _bcm_td2_l3_get_host_ent_by_idx(unit, NULL, mem[tbl], idx,
                                                      &l3cfg, &nh_idx);
            /* Check for read errors & invalid entries. */
            if (rv < 0) {
                if (rv != BCM_E_NOT_FOUND) {
                    return rv;
                }
                continue;
            }

            /* Check read entry flags & update index accordingly. */
            if (BCM_L3_CMP_EQUAL !=
                _bcm_xgs3_trvrs_flags_cmp(unit, flags, l3cfg.l3c_flags, &idx)) {
                continue;
            }

            /* If compare function provided match the entry against pattern */
            if (cmp_func) {
                BCM_IF_ERROR_RETURN((*cmp_func) (unit, pattern, (void *)&l3cfg,
                                                 (void *)&nh_idx, &cmp_result));
                /* If entry doesn't match the pattern don't delete it. */
                if (BCM_L3_CMP_EQUAL != cmp_result) {
                    continue;
                }
            }

            /* Entry matches the rule -> delete it. */
            BCM_IF_ERROR_RETURN(_bcm_td2_l3_entry_del(unit, &l3cfg));
    
            /* Release next hop index. */
            BCM_IF_ERROR_RETURN(bcm_xgs3_nh_del(unit, 0, nh_idx));
   
            /* Check if notification required. */
            if (notify_cb) {
                /* Fill host info. */
                _bcm_xgs3_host_ent_init(unit, &l3cfg, FALSE, &info);
                /* Call notification callback. */
                (*notify_cb) (unit, idx, &info, user_data);
            }
        }
    }
    /* Reset last read status. */
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_ipmc_get
 * Purpose:
 *      Get TD2 L3 multicast entry.
 * Parameters:
 *      unit  - (IN)SOC unit number.
 *      l3cfg - (IN/OUT)Group/Source key & Get result buffer.
 * Returns:
 *    BCM_E_XXX
 */
int
_bcm_td2_l3_ipmc_get(int unit, _bcm_l3_cfg_t *l3cfg)
{
    l3_entry_ipv6_multicast_entry_t l3x_key;    /* Lookup key buffer.    */
    l3_entry_ipv6_multicast_entry_t l3x_entry;  /* Search result buffer. */
    int clear_hit;                              /* Clear hit flag.       */
    soc_mem_t mem;                              /* IPMC table memory.    */
    int ipv6;                                   /* IPv6 entry indicator. */
    int rv;                                     /* Return value.         */
    int l3_entry_idx;                           /* Entry index           */

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory. */
    mem =  (ipv6) ? L3_ENTRY_IPV6_MULTICASTm : L3_ENTRY_IPV4_MULTICASTm;

    /*  Zero buffers. */
    sal_memcpy(&l3x_key, soc_mem_entry_null(unit, mem), 
                   soc_mem_entry_words(unit,mem) * 4);
    sal_memcpy(&l3x_entry, soc_mem_entry_null(unit, mem), 
                   soc_mem_entry_words(unit,mem) * 4);

    /* Check if clear hit bit is required. */
    clear_hit = l3cfg->l3c_flags & BCM_L3_HIT_CLEAR;

    /* Lookup Key preparation. */
    _bcm_td2_l3_ipmc_ent_init(unit, (uint32 *)&l3x_key, l3cfg);

    /* Perform hw lookup. */
    MEM_LOCK(unit, mem);
    rv = soc_mem_generic_lookup(unit, mem, MEM_BLOCK_ANY,
                                _BCM_TD2_L3_MEM_BANKS_ALL,
                                &l3x_key, &l3x_entry, &l3_entry_idx);
    l3cfg->l3c_hw_index = l3_entry_idx;

    MEM_UNLOCK(unit, mem);
    BCM_XGS3_LKUP_IF_ERROR_RETURN(rv, BCM_E_NOT_FOUND);

    /* Extract buffer information. */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_ipmc_ent_parse(unit, l3cfg, &l3x_entry));

    /* Clear the HIT bit */
    if (clear_hit) {
        BCM_IF_ERROR_RETURN(_bcm_td2_l3_clear_hit(unit, mem, l3cfg, &l3x_entry,
                                                  l3_entry_idx));
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l3_ipmc_get_by_idx
 * Purpose:
 *      Get l3 multicast entry by entry index.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      dma_ptr  - (IN)Table pointer in dma. 
 *      idx       - (IN)Index to read. 
 *      l3cfg     - (IN/OUT)Entry data.
 * Returns:
 *    BCM_E_XXX
 */
int
_bcm_td2_l3_ipmc_get_by_idx(int unit, void *dma_ptr,
                           int idx, _bcm_l3_cfg_t *l3cfg)
{
    l3_entry_ipv6_multicast_entry_t l3_entry_v6;    /* Read buffer.            */
    l3_entry_ipv4_multicast_entry_t l3_entry_v4;    /* Read buffer.            */
    void *buf_p;                      /* Hardware buffer ptr     */
    soc_mem_t mem;                     /* IPMC table memory.      */
    uint32 ipv6;                       /* IPv6 entry indicator.   */
    int clear_hit;                     /* Clear hit bit indicator.*/
    int key_type;
    soc_field_t l3iif;

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory. */
    if (ipv6) {
        mem   = L3_ENTRY_IPV6_MULTICASTm;
        buf_p = (l3_entry_ipv6_multicast_entry_t *)&l3_entry_v6;
        l3iif = IPV6MC__L3_IIFf;
    } else {
        mem   = L3_ENTRY_IPV4_MULTICASTm;
        buf_p = (l3_entry_ipv4_multicast_entry_t *)&l3_entry_v4;
        l3iif = IPV4MC__L3_IIFf;
    }

    /* Check if clear hit is required. */
    clear_hit = l3cfg->l3c_flags & BCM_L3_HIT_CLEAR;

    if (NULL == dma_ptr) {             /* Read from hardware. */
        /* Zero buffers. */
        sal_memcpy(buf_p, soc_mem_entry_null(unit, mem), 
                     soc_mem_entry_words(unit,mem) * 4);

        /* Read entry from hw. */
        BCM_IF_ERROR_RETURN(BCM_XGS3_MEM_READ(unit, mem, idx, buf_p));
    } else {                    /* Read from dma. */
        if (ipv6) {
            buf_p = soc_mem_table_idx_to_pointer(unit, mem,
                     l3_entry_ipv6_multicast_entry_t *, dma_ptr, idx);
        } else {
            buf_p = soc_mem_table_idx_to_pointer(unit, mem,
                     l3_entry_ipv4_multicast_entry_t *, dma_ptr, idx);
        }
    }

    /* Ignore invalid entries. */
    if (!soc_mem_field32_get(unit, mem, buf_p, VALID_0f)) {
        return (BCM_E_NOT_FOUND);
    }

    key_type = soc_mem_field32_get(unit, mem, buf_p, KEY_TYPE_0f);

    switch (key_type) {
      case TD2_L3_HASH_KEY_TYPE_V4MC:
          l3cfg->l3c_flags = BCM_L3_IPMC;
          break;
      case TD2_L3_HASH_KEY_TYPE_V6MC:
          l3cfg->l3c_flags = BCM_L3_IP6 | BCM_L3_IPMC;
          break;
      default:
          /* Catch all other key types, set flags to zero so that the 
           * following flag check is effective for all key types */
          l3cfg->l3c_flags = 0;  
          break;
    }

    /* Ignore protocol mismatch & multicast entries. */
    if ((ipv6  != (l3cfg->l3c_flags & BCM_L3_IP6)) ||
        (!(l3cfg->l3c_flags & BCM_L3_IPMC))) {
        return (BCM_E_NOT_FOUND); 
    }

    /* Set index to l3cfg. */
    l3cfg->l3c_hw_index = idx;

    if (ipv6) {
        /* Get group address. */
        soc_mem_ip6_addr_get(unit, mem, buf_p, IPV6MC__GROUP_IP_ADDR_LWR_64f,
                             l3cfg->l3c_ip6, SOC_MEM_IP6_LOWER_ONLY);

        soc_mem_ip6_addr_get(unit, mem, buf_p, IPV6MC__GROUP_IP_ADDR_UPR_56f,
                             l3cfg->l3c_ip6, SOC_MEM_IP6_UPPER_ONLY);

        /* Get source  address. */
        soc_mem_ip6_addr_get(unit, mem, buf_p, IPV6MC__SOURCE_IP_ADDR_LWR_64f, 
                             l3cfg->l3c_sip6, SOC_MEM_IP6_LOWER_ONLY);
        soc_mem_ip6_addr_get(unit, mem, buf_p, IPV6MC__SOURCE_IP_ADDR_UPR_64f, 
                             l3cfg->l3c_sip6, SOC_MEM_IP6_UPPER_ONLY);

        l3cfg->l3c_ip6[0] = 0xff;    /* Set entry to multicast*/
    } else {
        /* Get group id. */
        l3cfg->l3c_ip_addr =
            soc_mem_field32_get(unit, mem, buf_p, IPV4MC__GROUP_IP_ADDRf);

        /* Get source address. */
        l3cfg->l3c_src_ip_addr =
            soc_mem_field32_get(unit, mem,  buf_p, IPV4MC__SOURCE_IP_ADDRf);
    }

    /* Get L3_IIF or vlan id. */
    l3cfg->l3c_vid = soc_mem_field32_get(unit, mem, buf_p, l3iif);

    /* Parse entry data. */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_ipmc_ent_parse(unit, l3cfg, buf_p));

    /* Clear the HIT bit */
    if (clear_hit) {
        BCM_IF_ERROR_RETURN(_bcm_td2_l3_clear_hit(unit, mem, l3cfg, buf_p,
                                                     l3cfg->l3c_hw_index));
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_td2_l3_ipmc_add
 * Purpose:
 *      Add TD2 L3 multicast entry.
 * Parameters:
 *      unit  - (IN)SOC unit number.
 *      l3cfg - (IN/OUT)Group/Source key.
 * Returns:
 *    BCM_E_XXX
 */
int
 _bcm_td2_l3_ipmc_add(int unit, _bcm_l3_cfg_t *l3cfg)
{
    l3_entry_ipv6_multicast_entry_t l3x_entry;  /* Write entry buffer.   */
    soc_mem_t mem;                              /* IPMC table memory.    */
    uint32 *buf_p;                              /* HW buffer address.    */
    int ipv6;                                   /* IPv6 entry indicator. */
    int idx;                                    /* Iteration index.      */
    int idx_max;                                /* Iteration index max.  */
    int rv;                                     /* Return value.         */
    soc_field_t hitf[] = { HIT_0f, HIT_1f, HIT_2f, HIT_3f};
    soc_field_t rpe, dst_discard, vrf_id, l3mc_idx, class_id, pri, rpa_id;
    soc_field_t exp_iif, exp_iif_drop, exp_iif_copytocpu;

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory and table fields */
    if (ipv6) {
        mem         = L3_ENTRY_IPV6_MULTICASTm;
        pri         = IPV6MC__PRIf;
        rpe         = IPV6MC__RPEf;
        vrf_id      = IPV6MC__VRF_IDf;
        l3mc_idx    = IPV6MC__L3MC_INDEXf;
        class_id    = IPV6MC__CLASS_IDf;
        dst_discard = IPV6MC__DST_DISCARDf;
        rpa_id      = IPV6MC__RPA_IDf;
        exp_iif     = IPV6MC__EXPECTED_L3_IIFf;
        exp_iif_drop      = IPV6MC__IPMC_EXPECTED_L3_IIF_MISMATCH_DROPf;
        exp_iif_copytocpu = IPV6MC__IPMC_EXPECTED_L3_IIF_MISMATCH_TOCPUf;
    } else {
        mem         = L3_ENTRY_IPV4_MULTICASTm;
        pri         = IPV4MC__PRIf;
        rpe         = IPV4MC__RPEf;
        vrf_id      = IPV4MC__VRF_IDf;
        l3mc_idx    = IPV4MC__L3MC_INDEXf;
        class_id    = IPV4MC__CLASS_IDf;
        dst_discard = IPV4MC__DST_DISCARDf;
        rpa_id      = IPV4MC__RPA_IDf;
        exp_iif     = IPV4MC__EXPECTED_L3_IIFf;
        exp_iif_drop      = IPV4MC__IPMC_EXPECTED_L3_IIF_MISMATCH_DROPf;
        exp_iif_copytocpu = IPV4MC__IPMC_EXPECTED_L3_IIF_MISMATCH_TOCPUf;
    }

    /*  Zero buffers. */
    buf_p = (uint32 *)&l3x_entry; 
    sal_memcpy(buf_p, soc_mem_entry_null(unit, mem), 
                   soc_mem_entry_words(unit,mem) * 4);

   /* Prepare entry to write. */
    _bcm_td2_l3_ipmc_ent_init(unit, (uint32 *)&l3x_entry, l3cfg);

    /* Set priority override bit. */
    if (l3cfg->l3c_flags & BCM_L3_RPE) {
        soc_mem_field32_set(unit, mem, buf_p, rpe, 1);
    }

    /* Set destination discard. */
    if (l3cfg->l3c_flags & BCM_L3_DST_DISCARD) {
        soc_mem_field32_set(unit, mem, buf_p, dst_discard, 1);
    }
	
    /* Virtual router id. */
    soc_mem_field32_set(unit, mem, buf_p, vrf_id, l3cfg->l3c_vrf);

    /* Set priority. */
    soc_mem_field32_set(unit, mem, buf_p, pri, l3cfg->l3c_prio);

    /* Pointer to ipmc table. */
    soc_mem_field32_set(unit, mem, buf_p, l3mc_idx, l3cfg->l3c_ipmc_ptr);

    /* Classification lookup class id. */
    soc_mem_field32_set(unit, mem, buf_p, class_id, l3cfg->l3c_lookup_class);
   
    /* Rendezvous point ID. */
    if (l3cfg->l3c_rp_id != BCM_IPMC_RP_ID_INVALID) {
        soc_mem_field32_set(unit, mem, buf_p, rpa_id, l3cfg->l3c_rp_id);
    }

    /* RPF */
    if ((BCM_IPMC_POST_LOOKUP_RPF_CHECK & l3cfg->l3c_flags) &&
        (0 != l3cfg->l3c_intf)) {
        /* Set Expected_IIF to L3_IIF*/
        soc_mem_field32_set(unit, mem, buf_p, exp_iif, l3cfg->l3c_intf);
        if (BCM_IPMC_RPF_FAIL_DROP & l3cfg->l3c_flags) {
            soc_mem_field32_set(unit, mem, buf_p, exp_iif_drop, 1);
        }
        if (BCM_IPMC_RPF_FAIL_TOCPU & l3cfg->l3c_flags) {
            soc_mem_field32_set(unit, mem, buf_p, exp_iif_copytocpu, 1);
        }
    }

    idx_max = (ipv6) ? 4 : 2;
    for (idx = 0; idx < idx_max; idx++) {
        /* Set hit bit. */
        if (l3cfg->l3c_flags & BCM_L3_HIT) {
            soc_mem_field32_set(unit, mem, buf_p, hitf[idx], 1);
        }
    }

    /* Write entry to the hw. */
    MEM_LOCK(unit, mem);
    /* Handle replacement. */
    if (BCM_XGS3_L3_INVALID_INDEX != l3cfg->l3c_hw_index) {
        rv = BCM_XGS3_MEM_WRITE(unit, mem, l3cfg->l3c_hw_index, buf_p);
    } else {
        rv = soc_mem_insert(unit, mem, MEM_BLOCK_ANY, (void *)buf_p);
    }

    /* Increment number of ipmc routes. */
    if (BCM_SUCCESS(rv) && (BCM_XGS3_L3_INVALID_INDEX == l3cfg->l3c_hw_index)) {
        (ipv6) ? BCM_XGS3_L3_IP6_IPMC_CNT(unit)++ : \
                 BCM_XGS3_L3_IP4_IPMC_CNT(unit)++;
    }
    /*    coverity[overrun-local : FALSE]    */
    MEM_UNLOCK(unit, mem);
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l3_ipmc_del
 * Purpose:
 *      Delete TD2 L3 multicast entry.
 * Parameters:
 *      unit  - (IN)SOC unit number.
 *      l3cfg - (IN)Group/Source deletion key.
 * Returns:
 *    BCM_E_XXX
 */
int
_bcm_td2_l3_ipmc_del(int unit, _bcm_l3_cfg_t *l3cfg)
{
    l3_entry_ipv6_multicast_entry_t l3x_entry;  /* Delete buffer.          */
    soc_mem_t mem;                              /* IPMC table memory.      */
    int ipv6;                                   /* IPv6 entry indicator.   */
    int rv;                                     /* Operation return value. */

    /* Get entry type. */
    ipv6 = (l3cfg->l3c_flags & BCM_L3_IP6);

    /* Get table memory. */
    mem =  (ipv6) ? L3_ENTRY_IPV6_MULTICASTm : L3_ENTRY_IPV4_MULTICASTm;

    /*  Zero entry buffer. */
    sal_memcpy(&l3x_entry, soc_mem_entry_null(unit, mem), 
               soc_mem_entry_words(unit,mem) * 4);


    /* Key preparation. */
    _bcm_td2_l3_ipmc_ent_init(unit, (uint32 *)&l3x_entry, l3cfg);

    /* Delete the entry from hw. */
    MEM_LOCK(unit, mem);

    rv = soc_mem_delete(unit, mem, MEM_BLOCK_ANY, (void *)&l3x_entry);

    /* Decrement number of ipmc routes. */
    if (BCM_SUCCESS(rv)) {
        (ipv6) ? BCM_XGS3_L3_IP6_IPMC_CNT(unit)-- : \
            BCM_XGS3_L3_IP4_IPMC_CNT(unit)--;
    }
    MEM_UNLOCK(unit, mem);
    return rv;
}

/*
 * FLEX STAT FUNCTIONS for L3 ROUTE and L3 NAT
 *
 * This following functions implements flex stat routines to attach, detach and get/set
 * flex counter functions.
 * 
 * FLEX ATTACH 
 *   The function allocates flex counter from the counter pool and configures
 *   attributes to set the base counter index and base offset. Flex counter pool,
 *   base index and offset is associated to the table index request for the
 *   module identifiers like, vpn, port, vp Additionally the reference counter
 *   for the flex counter is incremented for multple references of the counter.
 *
 * FLEX DETACH 
 *   The function disassociates flex counter details assigned during flex attach
 *   and decrements the reference counter.
 *
 * FLEX COUNTER GET/SET 
 *   The function read/set the associate counter values. Set is required for the
 *   purpose of clearing the counter.
 *
 *
 * FLEX COUNTER MULTI GET/SET
 *   The function reads/sets multiple flex stats and returns the counter arrays. 
 *
 * FLEX STAT ID
 *   The funciton retrieves the stat id for the given module identifiers. The
 *   flex counter stat id has the following format.
 *
 *          (8 modes)  (32 groups) (16 pools)  (32 objIds)  (64K index)
 *         +---------+------------+----------+-----------+-----------------+ 
 *         |         |            |          |           |                 | 
 *         | ModeId  |  GroupMode |   PoolId | Account   |  Base Index     |
 *         |         |            |          | objectId  |                 |
 *         +---------+------------+----------+-----------+-----------------+
 *        (31)    (29)(28)     (24)(23)   (20)(19)    (15)(14)            (0)
 *
 *
 *   PoolId      : Counter pool no. 
 *                 ( TD2: Ingress(8 pool x 4k counters)
 *                        Egress (4 pool x 4k counters)
 *   GroupMode   : Attribute group mode determines base offset and no. of counters
 *   ModeId      : Ingress/Egress Modes
 *   Base Idx    : Index into counter block of 256 counters
 *   Account Obj : Flexible accounting objects
 */

/*
 * Function:
 *      _bcm_td2_l3_route_stat_get_table_info
 * Description:
 *      Provides relevant flex table information(table-name,index with
 *      direction)  for given ingress interface.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) L3 Route Info
 *      num_of_tables    - (OUT) Number of flex counter tables
 *      table_info       - (OUT) Flex counter tables information
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static 
bcm_error_t  _bcm_td2_l3_route_stat_get_table_info(
            int                        unit,
            bcm_l3_route_t	           *info,
            uint32                     *num_of_tables,
            bcm_stat_flex_table_info_t *table_info)
{
    int rv;
    int index, scale;
    soc_mem_t mem;
    int max_prefix_length;
    _bcm_defip_cfg_t defip;

    if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return BCM_E_UNAVAIL;
    }

    /*  Make sure module was initialized. */
    if (!BCM_XGS3_L3_INITIALIZED(unit)) {
        return (BCM_E_INIT);
    }

    /* Vrf is in device supported range check. */
    if ((info->l3a_vrf > SOC_VRF_MAX(unit)) || 
        (info->l3a_vrf < BCM_L3_VRF_DEFAULT)) {
        return BCM_E_PARAM;
    }

    /*  Check that device supports ipv6. */
    if (BCM_L3_NO_IP6_SUPPORT(unit, info->l3a_flags)) {
        return BCM_E_UNAVAIL;
    }

    mem = L3_DEFIPm;
    scale = 1;

    sal_memset(&defip, 0, sizeof(_bcm_defip_cfg_t));
    defip.defip_flags = info->l3a_flags;
    defip.defip_vrf = info->l3a_vrf;

    L3_LOCK(unit);
    if (info->l3a_flags & BCM_L3_IP6) {
        max_prefix_length = 
            soc_feature(unit, soc_feature_lpm_prefix_length_max_128) ? 128 : 64;
        sal_memcpy(defip.defip_ip6_addr, info->l3a_ip6_net, BCM_IP6_ADDRLEN);
        defip.defip_sub_len = bcm_ip6_mask_length(info->l3a_ip6_mask);
        if (defip.defip_sub_len > max_prefix_length) {
            L3_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        if (defip.defip_sub_len > 64) {
            mem = L3_DEFIP_PAIR_128m;
        } else {
            scale = 2;
        }

        rv = mbcm_driver[unit]->mbcm_ip6_defip_cfg_get(unit, &defip);
    } else {
        defip.defip_ip_addr = info->l3a_subnet & info->l3a_ip_mask;
        defip.defip_sub_len = bcm_ip_mask_length(info->l3a_ip_mask);
        rv = mbcm_driver[unit]->mbcm_ip4_defip_cfg_get(unit, &defip);
    }

    L3_UNLOCK(unit);
    if (rv < 0) {
        return rv;
    }

#ifdef ALPM_ENABLE
    if (soc_property_get(unit, spn_L3_ALPM_ENABLE, 0)) {
        if (!soc_feature(unit, soc_feature_lpm_tcam)) {
            return BCM_E_UNAVAIL;
        }
        if (mem == L3_DEFIP_PAIR_128m) {
            mem = L3_DEFIP_ALPM_IPV6_128m;
        }
    }
#endif

    index = defip.defip_index * scale;

    table_info[*num_of_tables].table = mem;
    table_info[*num_of_tables].index = index;
    table_info[*num_of_tables].direction = bcmStatFlexDirectionIngress;
    (*num_of_tables)++;

    SOC_DEBUG_PRINT((DK_L3,
                     "L3 Route Stat: table = %s, index = %d\n",
                     SOC_MEM_NAME(unit, mem), index));
    
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_td2_l3_route_stat_attach
 * Description:
 *      Attach counters entries to the given  l3 route defip index . Allocate
 *      flex counter and assign counter index and offset for the index pointed
 *      by l3 defip index
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) L3 Route Info
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
    
bcm_error_t  _bcm_td2_l3_route_stat_attach (
                            int                 unit,
                            bcm_l3_route_t	    *info,
                            uint32              stat_counter_id)
{
    int                        rv = BCM_E_NONE;
    int                        counter_flag = 0;
    soc_mem_t                  table[6] = {INVALIDm};
    bcm_stat_flex_direction_t  direction = bcmStatFlexDirectionIngress;
    uint32                     pool_number = 0;
    uint32                     base_index = 0;
    bcm_stat_flex_mode_t       offset_mode = 0;
    bcm_stat_object_t          object = bcmStatObjectIngPort;
    bcm_stat_group_mode_t      group_mode = bcmStatGroupModeSingle;
    uint32                     count = 0;
    uint32                     actual_num_tables = 0;
    uint32                     flex_table_index = 0;
    uint32                     num_of_tables = 0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];


    /* Get pool, base index group mode and offset modes from stat counter id */
    _bcm_esw_stat_get_counter_id_info(
                  stat_counter_id,
                  &group_mode, &object, &offset_mode, &pool_number, &base_index);

    /* Validate object and group mode */
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_object(unit, object, &direction));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_group(unit, group_mode));

    /* Get Table index to attach flexible counter */
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_get_table_info(
                        unit, object, 6, &actual_num_tables, &table[0], &direction));
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_route_stat_get_table_info(
                        unit, info, &num_of_tables, &table_info[0]));

    for (count = 0; count < num_of_tables; count++) {
        for (flex_table_index=0; 
             flex_table_index < actual_num_tables ; 
             flex_table_index++) {
            if ((table_info[count].direction == direction) &&
                (table_info[count].table == table[flex_table_index]) ) {
                if (direction == bcmStatFlexDirectionIngress) {
                    counter_flag = 1;
                    rv = _bcm_esw_stat_flex_attach_ingress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index,
                         offset_mode,
                         base_index,
                         pool_number);
                    if (BCM_FAILURE(rv)) {
                        break;
                    }
                } else {
                    counter_flag = 1;
                    rv = _bcm_esw_stat_flex_attach_egress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index,
                         offset_mode,
                         base_index,
                         pool_number);

                    if(BCM_FAILURE(rv)) {
                        break;
                    }
                } 
            }
        }
    }

    if (counter_flag == 0) {
        rv = BCM_E_NOT_FOUND;
    }

    return rv;
}


/*
 * Function:
 *      _bcm_td2_l3_route_stat_detach
 * Description:
 *      Detach counter entries to the given  l3 route index. 
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info		     - (IN) L3 Route Info
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t  _bcm_td2_l3_route_stat_detach (
                            int                 unit,
                            bcm_l3_route_t	    *info)
{
    int                        counter_flag = 0;
    uint32                     count = 0;
    uint32                     num_of_tables = 0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    bcm_error_t                rv[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] = 
                                  {BCM_E_NONE};

    /* Get the table index to be detached */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_route_stat_get_table_info(
                        unit, info, &num_of_tables, &table_info[0]));

    for (count = 0; count < num_of_tables; count++) {
         if (table_info[count].direction == bcmStatFlexDirectionIngress) {
             counter_flag = 1;
             rv[bcmStatFlexDirectionIngress]=
                   _bcm_esw_stat_flex_detach_ingress_table_counters(
                        unit, table_info[count].table, table_info[count].index);
         } else {
             counter_flag = 1;
             rv[bcmStatFlexDirectionEgress] =
                   _bcm_esw_stat_flex_detach_egress_table_counters(
                        unit, table_info[count].table, table_info[count].index);
         }
    }

    if (counter_flag == 0) {
        return BCM_E_NOT_FOUND;
    }

    if (rv[bcmStatFlexDirectionIngress] != BCM_E_NONE) {
        return rv[bcmStatFlexDirectionIngress];
    }
    if (rv[bcmStatFlexDirectionEgress] != BCM_E_NONE) {
        return rv[bcmStatFlexDirectionEgress];
    }
    
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_route_stat_counter_get
 *
 * Description:
 *  Get l3 route counter value for specified l3 route index
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN)  L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
 bcm_error_t  _bcm_td2_l3_route_stat_counter_get(
                            int                 unit, 
                            bcm_l3_route_t	    *info,
                            bcm_l3_route_stat_t stat, 
                            uint32              num_entries, 
                            uint32              *counter_indexes, 
                            bcm_stat_value_t    *counter_values)
{
    uint32                     table_count = 0;
    uint32                     index_count = 0;
    uint32                     num_of_tables = 0;
    bcm_stat_flex_direction_t  direction = bcmStatFlexDirectionIngress;
    uint32                     byte_flag = 0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    /* Get the flex counter direction - ingress/egress */
    if ((stat == bcmL3RouteInPackets) ||
        (stat == bcmL3RouteInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
        /* direction = bcmStatFlexDirectionEgress;*/
        return BCM_E_UNAVAIL;
    }
    if (stat == bcmL3RouteInPackets) {
        byte_flag=0;
    } else {
        byte_flag=1;
    }
   
    /*  Get Table index to read the flex counter */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_route_stat_get_table_info(
                        unit, info, &num_of_tables, &table_info[0]));

    /* Get the flex counter for the attached table index */
    for (table_count = 0; table_count < num_of_tables ; table_count++) {
         if (table_info[table_count].direction == direction) {
             for (index_count = 0; index_count < num_entries ; index_count++) {
                  BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_get(
                                      unit,
                                      table_info[table_count].index,
                                      table_info[table_count].table,
                                      byte_flag,
                                      counter_indexes[index_count],
                                      &counter_values[index_count]));
             }
         }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_route_stat_counter_set
 *
 * Description:
 *  Set l3 route counter value for specified l3 route index
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
 bcm_error_t  _bcm_td2_l3_route_stat_counter_set(
                            int                 unit, 
                            bcm_l3_route_t	    *info,
                            bcm_l3_route_stat_t stat, 
                            uint32              num_entries, 
                            uint32              *counter_indexes, 
                            bcm_stat_value_t    *counter_values)
{
    uint32                     table_count=0;
    uint32                     index_count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     byte_flag=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    /* Get the flex counter direction - ingress/egress */
    if ((stat == bcmL3RouteInPackets) ||
        (stat == bcmL3RouteInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
        /* direction = bcmStatFlexDirectionEgress;*/
        return BCM_E_UNAVAIL;
    }
    if (stat == bcmL3RouteInPackets) {
        byte_flag=0;
    } else {
        byte_flag=1;
    }
   
    /*  Get Table index to read the flex counter */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_route_stat_get_table_info(
                        unit, info, &num_of_tables, &table_info[0]));

    /* Get the flex counter for the attached table index */
    for (table_count = 0; table_count < num_of_tables ; table_count++) {
         if (table_info[table_count].direction == direction) {
             for (index_count = 0; index_count < num_entries; index_count++) {
                  BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_set(
                                      unit,
                                      table_info[table_count].index,
                                      table_info[table_count].table,
                                      byte_flag,
                                      counter_indexes[index_count],
                                      &counter_values[index_count]));
             }
         }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_route_stat_multi_get
 *
 * Description:
 *  Get Multiple l3 route counter value for specified l3 route index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) L3 Route Info
 *      nstat            - (IN) Number of elements in stat array
 *      stat_arr         - (IN) Collected statistics descriptors array
 *      value_arr        - (OUT) Collected counters values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

    
 bcm_error_t  _bcm_td2_l3_route_stat_multi_get(
                            int                 unit, 
                            bcm_l3_route_t	    *info,
                            int                 nstat, 
                            bcm_l3_route_stat_t *stat_arr,
                            uint64              *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};

    /* Iterate of all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat ;idx++) {
         BCM_IF_ERROR_RETURN(_bcm_td2_l3_route_stat_counter_get( 
                             unit, info, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
         if ((stat_arr[idx] == bcmL3RouteInPackets)) {
             COMPILER_64_SET(value_arr[idx],0,counter_values.packets);
         } else {
             COMPILER_64_SET(value_arr[idx],
                             COMPILER_64_HI(counter_values.bytes),
                             COMPILER_64_LO(counter_values.bytes));
         }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l3_route_stat_multi_get32
 *
 * Description:
 *  Get 32bit l3 route counter value for specified l3 route index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info		 	 - (IN) L3 Route Info
 *      nstat            - (IN) Number of elements in stat array
 *      stat_arr         - (IN) Collected statistics descriptors array
 *      value_arr        - (OUT) Collected counters values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
 bcm_error_t  _bcm_td2_l3_route_stat_multi_get32(
                            int                 unit, 
                            bcm_l3_route_t	    *info,
                            int                 nstat, 
                            bcm_l3_route_stat_t *stat_arr,
                            uint32              *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};

    /* Iterate of all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat ;idx++) {
         BCM_IF_ERROR_RETURN(_bcm_td2_l3_route_stat_counter_get( 
                             unit, info, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
         if ((stat_arr[idx] == bcmL3RouteInPackets)) {
                value_arr[idx] = counter_values.packets;
         } else {
             value_arr[idx] = COMPILER_64_LO(counter_values.bytes);
         }
    }
    return rv;
}

/*
 * Function:
 *     _bcm_td2_l3_route_stat_multi_set
 *
 * Description:
 *  Set l3 route counter value for specified l3 route index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t  _bcm_td2_l3_route_stat_multi_set(
                            int                 unit, 
                            bcm_l3_route_t	    *info,
                            int                 nstat, 
                            bcm_l3_route_stat_t *stat_arr,
                            uint64              *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};

    /* Iterate f all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat ;idx++) {
         if ((stat_arr[idx] == bcmL3RouteInPackets)) {
              counter_values.packets = COMPILER_64_LO(value_arr[idx]);
         } else {
              COMPILER_64_SET(counter_values.bytes,
                              COMPILER_64_HI(value_arr[idx]),
                              COMPILER_64_LO(value_arr[idx]));
         }
          BCM_IF_ERROR_RETURN(_bcm_td2_l3_route_stat_counter_set( 
                             unit, info, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
    }
    return rv;

}

/*
 * Function:
 *      _bcm_td2_l3_route_stat_multi_set32
 *
 * Description:
 *  Set 32bit l3 route counter value for specified l3 route index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

 bcm_error_t  _bcm_td2_l3_route_stat_multi_set32(
                            int                 unit, 
                            bcm_l3_route_t	    *info,
                            int                 nstat, 
                            bcm_l3_route_stat_t *stat_arr,
                            uint32              *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};

    /* Iterate of all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat ;idx++) {
         if ((stat_arr[idx] == bcmL3RouteInPackets)) {
             counter_values.packets = value_arr[idx];
         } else {
             COMPILER_64_SET(counter_values.bytes,0,value_arr[idx]);
         }

         BCM_IF_ERROR_RETURN(_bcm_td2_l3_route_stat_counter_set( 
                             unit, info, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
    }
    return rv;
}

/*
 * Function: 
 *      _bcm_td2_l3_route_stat_id_get
 *
 * Description: 
 *      Get Stat Counter if associated with given l3 route index
 *
 * Parameters: 
 *      unit             - (IN) bcm device
 *      info			 - (IN) L3 Route Info
 *      stat             - (IN) l3 route counter stat types
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */

 bcm_error_t  _bcm_td2_l3_route_stat_id_get(
                            int                 unit,
                            bcm_l3_route_t	    *info,
                            bcm_l3_route_stat_t stat, 
                            uint32              *stat_counter_id) 
{
    bcm_stat_flex_direction_t  direction = bcmStatFlexDirectionIngress;
    uint32                     num_of_tables = 0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32                     index = 0;
    uint32                     num_stat_counter_ids = 0;

    if ((stat == bcmL3RouteInPackets) ||
        (stat == bcmL3RouteInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
        /* direction = bcmStatFlexDirectionEgress;*/
        return BCM_E_UNAVAIL;
    }

    /*  Get Tables, for which flex counter are attached  */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_route_stat_get_table_info(
                        unit, info, &num_of_tables, &table_info[0]));

    /* Retrieve stat counter id */
    for (index = 0; index < num_of_tables ; index++) {
         if (table_info[index].direction == direction)
             return _bcm_esw_stat_flex_get_counter_id(
                                  unit, 1, &table_info[index],
                                  &num_stat_counter_ids, stat_counter_id);
    }
    return BCM_E_NOT_FOUND;
}
/***************************************************************
 *                        IP4 Options
 ****************************************************************/

/* Free IP4 options resources */
int _bcm_td2_l3_ip4_options_free_resources(unit)
{
    _bcm_l3_bookkeeping_t *l3_bk_info = L3_INFO(unit);

    if (!l3_bk_info) {
        return BCM_E_NONE;
    }

    /* free ip4 options profile usage bitmap */
    if (NULL != l3_bk_info->ip4_options_bitmap) {
        sal_free(l3_bk_info->ip4_options_bitmap);
        l3_bk_info->ip4_options_bitmap = NULL;
    }

    /* free ip4 options hw index usage bitmap */
    if (NULL != l3_bk_info->ip4_profiles_hw_idx) {
        sal_free(l3_bk_info->ip4_profiles_hw_idx);
        l3_bk_info->ip4_profiles_hw_idx = NULL;
    }

    return BCM_E_NONE;
}

/* Initialize the IP4 options book keeping */
int 
_bcm_td2_l3_ip4_options_profile_init(int unit)
{
    _bcm_l3_bookkeeping_t *l3_bk_info = L3_INFO(unit);
    int ip4_options_profiles;

    ip4_options_profiles = _BCM_IP4_OPTIONS_LEN;

    /* Allocate ip4 options profile usage bitmap */
    if (NULL == l3_bk_info->ip4_options_bitmap) {
        l3_bk_info->ip4_options_bitmap =
            sal_alloc(SHR_BITALLOCSIZE(ip4_options_profiles), "ip4_options_bitmap");
        if (l3_bk_info->ip4_options_bitmap == NULL) {
            _bcm_td2_l3_ip4_options_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(l3_bk_info->ip4_options_bitmap, 0, SHR_BITALLOCSIZE(ip4_options_profiles));
    if (NULL == l3_bk_info->ip4_profiles_hw_idx) {
        l3_bk_info->ip4_profiles_hw_idx = 
            sal_alloc(sizeof(uint32) * ip4_options_profiles, "ip4_profiles_hw_idx");
        if (l3_bk_info->ip4_profiles_hw_idx == NULL) {
            _bcm_td2_l3_ip4_options_free_resources(unit);
            return BCM_E_MEMORY;
        }    
    }
    sal_memset(l3_bk_info->ip4_profiles_hw_idx, 0, (sizeof(uint32) * ip4_options_profiles));
    return BCM_E_NONE;
}



STATIC int
_bcm_td2_l3_ip4_profile_id_alloc(int unit)
{
    int i, option_profile_size = -1;
    SHR_BITDCL *bitmap;

    bitmap = L3_INFO(unit)->ip4_options_bitmap;
    option_profile_size = _BCM_IP4_OPTIONS_LEN;
    for (i = 0; i < option_profile_size; i++) {
        if (!SHR_BITGET(bitmap, i)) {
            return i;
        }
    }
    return -1;
}

int
_bcm_td2_l3_ip4_options_profile_create(int unit, uint32 flags, 
                                   bcm_l3_ip4_options_action_t default_action, 
                                   int *ip4_options_profile_id)
{
    ip_option_control_profile_table_entry_t ip_option_profile[_BCM_IP_OPTION_PROFILE_CHUNK];
    int id, index = -1, rv = BCM_E_NONE;
    int i, copy_cpu, drop;
    void *entries[1], *entry;

    /* Check for pre-specified ID */
    if (flags & BCM_L3_IP4_OPTIONS_WITH_ID) {
        id = *ip4_options_profile_id;
        if (_BCM_IP4_OPTIONS_USED_GET(unit, id)) {
            if (!(flags & BCM_L3_IP4_OPTIONS_REPLACE)) {
                return BCM_E_EXISTS;
            }
        } else {
            _BCM_IP4_OPTIONS_USED_SET(unit, id);
        }
    } else {
        id = _bcm_td2_l3_ip4_profile_id_alloc(unit);
        if (id == -1) {
            return BCM_E_RESOURCE;
        }
        _BCM_IP4_OPTIONS_USED_SET(unit, id);
        *ip4_options_profile_id = id;
    }
    /* Reserve a chunk in the IP4_OPTION Profile table */
    sal_memset(ip_option_profile, 0, sizeof(ip_option_profile));
    entries[0] = &ip_option_profile;

    /* Set the default action for the profile */
    switch (default_action) {
    case bcmIntfIPOptionActionCopyToCPU:
        copy_cpu = 1;
        drop = 0;
        break;
    case bcmIntfIPOptionActionDrop:
        copy_cpu = 0;
        drop = 1;
        break;
    case bcmIntfIPOptionActionCopyCPUAndDrop:
        copy_cpu = 1;
        drop = 1;
        break;
    case bcmIntfIPOptionActionNone:
    default :
        copy_cpu = 0;
        drop = 0;
        break;
    }
    for (i = 0; i < _BCM_IP_OPTION_PROFILE_CHUNK; i++) {
        entry = &ip_option_profile[i];
        soc_mem_field32_set(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                            COPYTO_CPUf, copy_cpu);
        soc_mem_field32_set(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                            DROPf, drop);
    }

    BCM_IF_ERROR_RETURN(_bcm_l3_ip4_options_profile_entry_add(unit, entries, 
                                        _BCM_IP_OPTION_PROFILE_CHUNK,
                                         (uint32 *)&index));
    L3_INFO(unit)->ip4_profiles_hw_idx[id] = (index / 
                                          _BCM_IP_OPTION_PROFILE_CHUNK);
    return rv;
}



int 
_bcm_td2_l3_ip4_options_profile_destroy(int unit, int ip4_options_profile_id)
{
    int id, rv = BCM_E_UNAVAIL;
    L3_LOCK(unit);
    id = ip4_options_profile_id;
    if (!_BCM_IP4_OPTIONS_USED_GET(unit, id)) {
        L3_UNLOCK(unit);
        return BCM_E_NOT_FOUND;
    } else {
        rv = _bcm_l3_ip4_options_profile_entry_delete
                 (unit, L3_INFO(unit)->ip4_profiles_hw_idx[id] * 
                  _BCM_IP_OPTION_PROFILE_CHUNK);
        L3_INFO(unit)->ip4_profiles_hw_idx[id] = 0;
        _BCM_IP4_OPTIONS_USED_CLR(unit, id);
    }
#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif
    L3_UNLOCK(unit);
    return rv;
}

/* Add an entry to a  ip_option_profile */
/* Read the existing profile chunk, modify what's needed and add the 
 * new profile. This can result in the HW profile index changing for a 
 * given ID */
int 
_bcm_td2_l3_ip4_options_profile_action_set(int unit,
                         int ip4_options_profile_id, 
                         int ip4_option, 
                         bcm_l3_ip4_options_action_t action)
{
    ip_option_control_profile_table_entry_t ip_option_profile[_BCM_IP_OPTION_PROFILE_CHUNK];
    int id, index = -1, rv = BCM_E_NONE;
    void *entries[1], *entry;
    int offset;

    /*  Make sure module was initialized. */
    if (!BCM_XGS3_L3_INITIALIZED(unit)) {
        return (BCM_E_INIT);
    }

    id = ip4_options_profile_id;
    L3_LOCK(unit);

    if (!_BCM_IP4_OPTIONS_USED_GET(unit, id)) {
        L3_UNLOCK(unit);
        return BCM_E_PARAM;
    } else {
        entries[0] = ip_option_profile;

        /* Base index of table in hardware */
        index = L3_INFO(unit)->ip4_profiles_hw_idx[id] * 
                               _BCM_IP_OPTION_PROFILE_CHUNK;

        rv = _bcm_l3_ip4_options_profile_entry_get(unit, index, 
                                            _BCM_IP_OPTION_PROFILE_CHUNK, 
                                            entries);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return (rv);
        }

        /* Offset within table */
        offset = ip4_option;

        /* Modify what's needed */
        entry = &ip_option_profile[offset];
        switch (action) {
        case bcmIntfIPOptionActionCopyToCPU:
            soc_mem_field32_set(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                                COPYTO_CPUf, 1);
            soc_mem_field32_set(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                                DROPf, 0);
            break;

        case bcmIntfIPOptionActionDrop:
            soc_mem_field32_set(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                                COPYTO_CPUf, 0);
            soc_mem_field32_set(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                                DROPf, 1);
            break;

        case bcmIntfIPOptionActionCopyCPUAndDrop:
            soc_mem_field32_set(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                                COPYTO_CPUf, 1);
            soc_mem_field32_set(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                                DROPf, 1);
            break;

        case bcmIntfIPOptionActionNone:
        default :
            soc_mem_field32_set(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                                COPYTO_CPUf, 0);
            soc_mem_field32_set(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                                DROPf, 0);

        }
        /* Delete the old profile chunk */
        rv = _bcm_l3_ip4_options_profile_entry_delete(unit, index);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return (rv);
        }

        /* Add new chunk and store new HW index */
        rv = _bcm_l3_ip4_options_profile_entry_add(unit, entries,
                                            _BCM_IP_OPTION_PROFILE_CHUNK, 
                                            (uint32 *)&index);
        L3_INFO(unit)->ip4_profiles_hw_idx[id] = (index / 
                                             _BCM_IP_OPTION_PROFILE_CHUNK);
    }
#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif
    L3_UNLOCK(unit);
    return rv;
}


int 
_bcm_td2_l3_ip4_options_profile_action_get(int unit,
                         int ip4_options_profile_id, 
                         int ip4_option, 
                         bcm_l3_ip4_options_action_t *action)
{
    ip_option_control_profile_table_entry_t ip_option_profile[_BCM_IP_OPTION_PROFILE_CHUNK];
    int id, index = -1, rv = BCM_E_NONE;
    void *entries[1], *entry;
    int offset;
    int copy_to_cpu, drop;

    /*  Make sure module was initialized. */
    if (!BCM_XGS3_L3_INITIALIZED(unit)) {
        return (BCM_E_INIT);
    }

    id = ip4_options_profile_id;
    L3_LOCK(unit);

    if (!_BCM_IP4_OPTIONS_USED_GET(unit, id)) {
        L3_UNLOCK(unit);
        return BCM_E_PARAM;
    } else {
        entries[0] = ip_option_profile;

        /* Base index of table in hardware */
        index = L3_INFO(unit)->ip4_profiles_hw_idx[id] * 
                               _BCM_IP_OPTION_PROFILE_CHUNK;

        rv = _bcm_l3_ip4_options_profile_entry_get(unit, index, 
                                            _BCM_IP_OPTION_PROFILE_CHUNK, 
                                            entries);
        if (BCM_FAILURE(rv)) {
            L3_UNLOCK(unit);
            return (rv);
        }

        /* Offset within table */
        offset = ip4_option;

        /* Modify what's needed */
        entry = &ip_option_profile[offset];

        copy_to_cpu = soc_mem_field32_get(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                                COPYTO_CPUf);
        drop = soc_mem_field32_get(unit, IP_OPTION_CONTROL_PROFILE_TABLEm, entry,
                                DROPf);

        *action = bcmIntfIPOptionActionNone;
        if ((copy_to_cpu == 0) && (drop == 0)) {
            *action = bcmIntfIPOptionActionNone;
        }
        if ((copy_to_cpu == 1) && (drop == 0)) {
            *action = bcmIntfIPOptionActionCopyToCPU;
        }
        if ((copy_to_cpu == 0) && (drop == 1)) {
            *action = bcmIntfIPOptionActionDrop;
        }
        if ((copy_to_cpu == 1) && (drop == 1)) {
            *action = bcmIntfIPOptionActionCopyCPUAndDrop;
        }
    }
    L3_UNLOCK(unit);
    return rv;
}

/* Get the list of all created IP options profile IDs */
int
_bcm_td2_l3_ip4_options_profile_multi_get(int unit, int array_size, 
                                      int *ip_options_ids_array, 
                                      int *array_count)
{
    int rv = BCM_E_NONE;
    int idx, count;

    L3_LOCK(unit);
    if (array_size == 0) {
        /* querying the number of map-ids for storage allocation */
        if (array_count == NULL) {
            rv = BCM_E_PARAM;
        }
        if (BCM_SUCCESS(rv)) {
            count = 0;
            *array_count = 0;
            SHR_BITCOUNT_RANGE(L3_INFO(unit)->ip4_options_bitmap, count, 
                               0, _BCM_IP4_OPTIONS_LEN);
            *array_count += count;
        }
    } else {
        if ((ip_options_ids_array == NULL) || (array_count == NULL)) {
            rv = BCM_E_PARAM;
        }
        if (BCM_SUCCESS(rv)) {
            count = 0;
            for (idx = 0; ((idx < _BCM_IP4_OPTIONS_LEN) && 
                         (count < array_size)); idx++) {
                if (_BCM_IP4_OPTIONS_USED_GET(unit, idx)) {
                    *(ip_options_ids_array + count) = idx;
                    count++;
                }
            }
            *array_count = count;
        }
    }
    L3_UNLOCK(unit);
    return rv;
}

/* Get the IP options actions for a given profile ID */
int
_bcm_td2_l3_ip4_options_profile_action_multi_get(int unit, uint32 flags,
                          int ip_option_profile_id, int array_size, 
                          bcm_l3_ip4_options_action_t *array, int *array_count)
{
    int             rv = BCM_E_NONE;
    int             num_entries, idx, id, hw_id, alloc_size, entry_size, count;
    uint8           *dma_buf;
    void            *entry;
    soc_mem_t       mem;
    bcm_l3_ip4_options_action_t   *action;
    int copy_to_cpu, drop;

    /* ignore with_id & replace flags */
    id = ip_option_profile_id;
    hw_id = 0;
    num_entries = 0;
    entry_size = 0;
    L3_LOCK(unit);

    if (!_BCM_IP4_OPTIONS_USED_GET(unit, id)) {
        rv = BCM_E_PARAM;
    }
    num_entries = _BCM_IP_OPTION_PROFILE_CHUNK;
    entry_size = sizeof(ip_option_control_profile_table_entry_t);
    hw_id = L3_INFO(unit)->ip4_profiles_hw_idx[id] * _BCM_IP_OPTION_PROFILE_CHUNK;
    mem = IP_OPTION_CONTROL_PROFILE_TABLEm;


    L3_UNLOCK(unit);
    BCM_IF_ERROR_RETURN(rv);

    if (array_size == 0) { /* querying the size of profile-chunk */
        *array_count = num_entries;
        return BCM_E_NONE;
    }
    if (!array || !array_count) {
        return BCM_E_PARAM;
    }

    /* Allocate memory for DMA & regular */
    alloc_size = num_entries * entry_size;
    dma_buf = soc_cm_salloc(unit, alloc_size, "IP option multi get DMA buf");
    if (!dma_buf) {
        return BCM_E_MEMORY;
    }
    sal_memset(dma_buf, 0, alloc_size);

    /* Read the profile chunk */
    rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ANY, hw_id, 
                            (hw_id + num_entries - 1), dma_buf);
    if (BCM_FAILURE(rv)) {
        soc_cm_sfree(unit, dma_buf);
        return rv;
    }

    count = 0;
    L3_LOCK(unit);
    for (idx=0; ((idx<num_entries) && (count < array_size)); idx++) {
        action = &array[idx];
        entry = soc_mem_table_idx_to_pointer(unit, mem, void *, 
                                             dma_buf, idx);
        copy_to_cpu = soc_mem_field32_get(unit, mem, entry, COPY_TO_CPUf);
        drop = soc_mem_field32_get(unit, mem, entry, DROPf);

        *action = bcmIntfIPOptionActionNone;
        if ((copy_to_cpu == 0) && (drop == 0)) {
            *action = bcmIntfIPOptionActionNone;
        }
        if ((copy_to_cpu == 1) && (drop == 0)) {
            *action = bcmIntfIPOptionActionCopyToCPU;
        }
        if ((copy_to_cpu == 0) && (drop == 1)) {
            *action = bcmIntfIPOptionActionDrop;
        }
        if ((copy_to_cpu == 1) && (drop == 1)) {
            *action = bcmIntfIPOptionActionCopyCPUAndDrop;
        }
        count++;
    }
    L3_UNLOCK(unit);
    soc_cm_sfree(unit, dma_buf);

    BCM_IF_ERROR_RETURN(rv);

    *array_count = count;
    return BCM_E_NONE;
}


/* Function:
*	   _bcm_td2_l3_ip4_options_profile_id2idx
* Purpose:
*	   Translate profile ID into hardware table index.
* Parameters:
* Returns:
*	   BCM_E_XXX
*/	   
int
_bcm_td2_l3_ip4_options_profile_id2idx(int unit, int profile_id, int *hw_idx)
{
    if (hw_idx == NULL) {
        return BCM_E_PARAM;
    }
    /*  Make sure module was initialized. */
    if (L3_INFO(unit)->ip4_options_bitmap == NULL) {
        return (BCM_E_INIT);
    }

    L3_LOCK(unit);
    if (!_BCM_IP4_OPTIONS_USED_GET(unit, profile_id)) {
        L3_UNLOCK(unit);
        return BCM_E_NOT_FOUND;
    } else {
        *hw_idx = L3_INFO(unit)->ip4_profiles_hw_idx[profile_id];
    }
    L3_UNLOCK(unit);
    return BCM_E_NONE;
}

/* Function:
*	   _bcm_td2_l3_ip4_options_profile_idx2id
* Purpose:
*	   Translate hardware table index into Profile ID used by API
* Parameters:
* Returns:
*	   BCM_E_XXX
*/	   
int
_bcm_td2_l3_ip4_options_profile_idx2id(int unit, int hw_idx, int *profile_id)
{
    int id;

    if (profile_id == NULL) {
        return BCM_E_PARAM;
    }

    /*  Make sure module was initialized. */
    if (!BCM_XGS3_L3_INITIALIZED(unit)) {
        return (BCM_E_INIT);
    }

    L3_LOCK(unit);
    for (id = 0; id < _BCM_IP4_OPTIONS_LEN; id++) {
        if (_BCM_IP4_OPTIONS_USED_GET(unit, id)) {
            if (L3_INFO(unit)->ip4_profiles_hw_idx[id] == hw_idx) {
                *profile_id = id;
                L3_UNLOCK(unit);
                return BCM_E_NONE;
            }
        }
    }
    L3_UNLOCK(unit);
    return BCM_E_NOT_FOUND;
}

/* 
 * Function:
 *      _bcm_td2_vp_urpf_mode_set
 * Purpose:
 *      Set VP routing urpf mode
 * Parameters:
 *      unit        : (IN) Device Unit Number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_vp_urpf_mode_set(int unit, bcm_port_t port, int arg) {

    int vp;
    source_vp_entry_t svp;

    if (BCM_GPORT_IS_NIV_PORT(port)) {
        vp = BCM_GPORT_NIV_PORT_ID_GET(port);
        if(!_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv)) {
            return BCM_E_NOT_FOUND;
        }
    } else if (BCM_GPORT_IS_EXTENDER_PORT(port)) {
        vp = BCM_GPORT_EXTENDER_PORT_ID_GET(port);
        if(!_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
            return BCM_E_NOT_FOUND;
        }
    } else if (BCM_GPORT_IS_VLAN_PORT(port)) {
        vp = BCM_GPORT_VLAN_PORT_ID_GET(port);
        if(!_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
            return BCM_E_NOT_FOUND;
        }
    } else {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp));
    soc_SOURCE_VPm_field32_set(unit, &svp, URPF_MODEf, (arg ? 1 : 0));
    BCM_IF_ERROR_RETURN(WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp));
    return BCM_E_NONE;
}

/* 
 * Function:
 *      _bcm_td2_vp_urpf_mode_get
 * Purpose:
 *      Get VP routing urpf mode
 * Parameters:
 *      unit        : (IN) Device Unit Number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_vp_urpf_mode_get(int unit, bcm_port_t port, int *arg) {

    int vp;
    source_vp_entry_t svp;

    if (BCM_GPORT_IS_NIV_PORT(port)) {
        vp = BCM_GPORT_NIV_PORT_ID_GET(port);
        if(!_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv)) {
            return BCM_E_NOT_FOUND;
        }
    } else if (BCM_GPORT_IS_EXTENDER_PORT(port)) {
        vp = BCM_GPORT_EXTENDER_PORT_ID_GET(port);
        if(!_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
            return BCM_E_NOT_FOUND;
        }
    } else if (BCM_GPORT_IS_VLAN_PORT(port)) {
        vp = BCM_GPORT_VLAN_PORT_ID_GET(port);
        if(!_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
            return BCM_E_NOT_FOUND;
        }
    } else {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp));
    *arg = soc_SOURCE_VPm_field32_get(unit, &svp, URPF_MODEf);
    return BCM_E_NONE;
}

/* 
 * Function:
 *      _bcm_td2_l3_iif_profile_get
 * Purpose:
 *      Read TD2 iif profile entry and convert to corresponding
 *      legacy vlan profile flags.
 * Parameters:
 *      unit        : (IN) Device Unit Number
 * Returns:
 *      NONE
 */
int
_bcm_td2_l3_intf_iif_profile_get(int unit, bcm_vlan_t vid,
                                 bcm_vlan_control_vlan_t *control) {

    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/

    if (NULL == control) {
        return BCM_E_INTERNAL;
    }

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    iif.intf_id = vid;
    BCM_IF_ERROR_RETURN(_bcm_tr_l3_ingress_interface_get(unit, &iif));

    control->flags |= (iif.flags & BCM_L3_INGRESS_ROUTE_DISABLE_IP4_UCAST) ?
                 BCM_VLAN_IP4_DISABLE: 0;
    control->flags |= (iif.flags & BCM_L3_INGRESS_ROUTE_DISABLE_IP6_UCAST ) ?
                 BCM_VLAN_IP6_DISABLE: 0;
    control->flags |= (iif.flags & BCM_L3_INGRESS_ROUTE_DISABLE_IP4_MCAST ) ?
                 BCM_VLAN_IP4_MCAST_DISABLE: 0;
    control->flags |= (iif.flags & BCM_L3_INGRESS_ROUTE_DISABLE_IP6_MCAST ) ?
                 BCM_VLAN_IP6_MCAST_DISABLE: 0;
    control->flags |= (iif.flags & BCM_L3_INGRESS_UNKNOWN_IP4_MCAST_TOCPU ) ?
                BCM_VLAN_UNKNOWN_IP4_MCAST_TOCPU : 0;
    control->flags |= (iif.flags & BCM_L3_INGRESS_UNKNOWN_IP6_MCAST_TOCPU ) ?
                BCM_VLAN_UNKNOWN_IP6_MCAST_TOCPU : 0;
    control->flags |= (iif.flags & BCM_L3_INGRESS_ICMP_REDIRECT_TOCPU ) ?
                 BCM_VLAN_ICMP_REDIRECT_TOCPU : 0;
    if (iif.flags & BCM_L3_INGRESS_URPF_DEFAULT_ROUTE_CHECK) {
        control->flags &= ~BCM_VLAN_L3_URPF_DEFAULT_ROUTE_CHECK_DISABLE;
    }
    if (iif.flags & BCM_L3_INGRESS_GLOBAL_ROUTE) {
        control->flags &= ~BCM_VLAN_L3_VRF_GLOBAL_DISABLE;
    }

    return (BCM_E_NONE);
}

/* 
 * Function:
 *      _bcm_td2_l3_iif_profile_update
 * Purpose:
 *      Update TD2 iif profile based on vlan intf flags
 * Parameters:
 *      unit        : (IN) Device Unit Number
 * Returns:
 *      NONE
 */
int
_bcm_td2_l3_intf_iif_profile_update(int unit, bcm_vlan_t vid, 
                                    bcm_vlan_control_vlan_t *control) {

    _bcm_l3_ingress_intf_t iif; /* Ingress interface config.*/
    int ret_val;                /* Operation return value.  */

    /* Input parameters sanity check. */
    if ((vid > soc_mem_index_max(unit, L3_IIFm)) || 
        (vid < soc_mem_index_min(unit, L3_IIFm))) {
        return (BCM_E_PARAM);
    }

    iif.intf_id = vid;
    soc_mem_lock(unit, L3_IIFm);

    ret_val = _bcm_tr_l3_ingress_interface_get(unit, &iif);
    if (BCM_FAILURE(ret_val)) {
        soc_mem_unlock(unit, L3_IIFm);
        return (ret_val);
    }

    /* Set default values */
    iif.flags |= BCM_L3_INGRESS_URPF_DEFAULT_ROUTE_CHECK;
    iif.flags |= BCM_L3_INGRESS_GLOBAL_ROUTE;
    iif.flags &= ~BCM_L3_INGRESS_ROUTE_DISABLE_IP4_UCAST;
    iif.flags &= ~BCM_L3_INGRESS_ROUTE_DISABLE_IP6_UCAST;
    iif.flags &= ~BCM_L3_INGRESS_ROUTE_DISABLE_IP4_MCAST;
    iif.flags &= ~BCM_L3_INGRESS_ROUTE_DISABLE_IP6_MCAST;
    iif.flags &= ~BCM_L3_INGRESS_UNKNOWN_IP4_MCAST_TOCPU;
    iif.flags &= ~BCM_L3_INGRESS_UNKNOWN_IP6_MCAST_TOCPU;
    iif.flags &= ~BCM_L3_INGRESS_ICMP_REDIRECT_TOCPU;
    iif.flags &= ~BCM_L3_INGRESS_IPMC_DO_VLAN_DISABLE;

    iif.flags |= (control->flags & BCM_VLAN_IP4_DISABLE) ?
                 BCM_L3_INGRESS_ROUTE_DISABLE_IP4_UCAST : 0;
    iif.flags |= (control->flags & BCM_VLAN_IP6_DISABLE) ?
                 BCM_L3_INGRESS_ROUTE_DISABLE_IP6_UCAST : 0;
    iif.flags |= (control->flags & BCM_VLAN_IP4_MCAST_DISABLE) ?
                 BCM_L3_INGRESS_ROUTE_DISABLE_IP4_MCAST : 0;
    iif.flags |= (control->flags & BCM_VLAN_IP6_MCAST_DISABLE) ?
                 BCM_L3_INGRESS_ROUTE_DISABLE_IP6_MCAST : 0;
    iif.flags |= (control->flags & BCM_VLAN_UNKNOWN_IP4_MCAST_TOCPU) ?
                 BCM_L3_INGRESS_UNKNOWN_IP4_MCAST_TOCPU : 0;
    iif.flags |= (control->flags & BCM_VLAN_UNKNOWN_IP6_MCAST_TOCPU) ?
                 BCM_L3_INGRESS_UNKNOWN_IP6_MCAST_TOCPU : 0;
    iif.flags |= (control->flags & BCM_VLAN_ICMP_REDIRECT_TOCPU) ?
                 BCM_L3_INGRESS_ICMP_REDIRECT_TOCPU : 0;
    iif.flags |= (control->flags & BCM_VLAN_IPMC_DO_VLAN_DISABLE) ?
                 BCM_L3_INGRESS_IPMC_DO_VLAN_DISABLE : 0;

    if (control->flags & BCM_VLAN_L3_URPF_DEFAULT_ROUTE_CHECK_DISABLE) {
        iif.flags &= ~BCM_L3_INGRESS_URPF_DEFAULT_ROUTE_CHECK;
    }
    if (control->flags & BCM_VLAN_L3_VRF_GLOBAL_DISABLE) {
        iif.flags &= ~BCM_L3_INGRESS_GLOBAL_ROUTE;
    }

    ret_val = _bcm_tr_l3_ingress_interface_set(unit, &iif);

    soc_mem_unlock(unit, L3_IIFm);

    return (ret_val);
}

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/* 
 * Function:
 *      _bcm_td2_l3_ip4_options_profile_sw_dump
 * Purpose:
 *      Displays IP oprion profile software state info
 * Parameters:
 *      unit        : (IN) Device Unit Number
 * Returns:
 *      NONE
 */
void
_bcm_td2_l3_ip4_options_profile_sw_dump(int unit)
{
    int i;

    if (!L3_INFO(unit)->l3_initialized) {
        soc_cm_print("ERROR: L3  module not initialized on Unit:%d \n", unit);
        return;
    }

    soc_cm_print("L3 IP Option: IP_OPTION_CONTROL_PROFILE_TABLEm info \n");
    for (i = 0; i < _BCM_IP4_OPTIONS_LEN; i++) {
        if (_BCM_IP4_OPTIONS_USED_GET(unit, i)) {
            soc_cm_print("    Profile id:%4d    HW index:%4d\n", i, 
                         L3_INFO(unit)->ip4_profiles_hw_idx[i]);
        }
    }

}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#ifdef BCM_WARM_BOOT_SUPPORT


/* SCACHE size for WB */
int 
_bcm_td2_l3_ip4_options_profile_scache_len_get(int unit, int *wb_alloc_size)
{
    if (wb_alloc_size == NULL) {
        return BCM_E_PARAM;
    }
    *wb_alloc_size = sizeof(uint32) * _BCM_IP4_OPTIONS_LEN;

    return BCM_E_NONE;
}

/* 
 * Function:
 *      _bcm_td2_l3_ip4_options_profile_reinit_hw_profiles_update
 * Purpose:
 *      Updates the shared memory profile tables reference counts
 * Parameters:
 *      unit    : (IN) Device Unit Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_l3_ip4_options_profile_reinit_hw_profiles_update (int unit)
{
    int     i;
    for (i=0; i < _BCM_IP4_OPTIONS_LEN; i++) {
        if (_BCM_IP4_OPTIONS_USED_GET(unit, i)) {
            BCM_IF_ERROR_RETURN(_bcm_l3_ip4_options_profile_entry_reference(unit, 
                              ((L3_INFO(unit)->ip4_profiles_hw_idx[i]) * 
                               _BCM_IP_OPTION_PROFILE_CHUNK), 
                               _BCM_IP_OPTION_PROFILE_CHUNK));
        }
    }
    return BCM_E_NONE;
}



/* 
 * Function:
 *      _bcm_td2_l3_ip4_options_recover
 * Purpose:
 *      Recover IP Option Profile usage
 * Parameters:
 *      unit    : (IN) Device Unit Number
 *      scache_ptr - (IN) Scache pointer
 *                   (OUT) Updated Scache pointer
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_l3_ip4_options_recover(int unit, uint8 **scache_ptr)
{
    uint32 hw_idx;
    int    idx;
    int    rv = BCM_E_NONE;
    int    stable_size = 0;

    if ((scache_ptr == NULL) || (*scache_ptr == NULL)) {
        return BCM_E_PARAM;
    }

    if (SOC_WARM_BOOT(unit)) {
        SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

        if (stable_size == 0) { /* level 1 */
            /* Nothing to recover */
        } else {  /* Level 2 */
            /* recover from scache into book-keeping structs */
            for (idx = 0; idx < _BCM_IP4_OPTIONS_LEN; idx++) {
                sal_memcpy(&hw_idx, (*scache_ptr), sizeof(uint32));
                (*scache_ptr) += sizeof(hw_idx);
                if (hw_idx != _BCM_IP_OPTION_REINIT_INVALID_HW_IDX) {
                    _BCM_IP4_OPTIONS_USED_SET(unit, idx);
                    L3_INFO(unit)->ip4_profiles_hw_idx[idx] = hw_idx;
                }
            }
            rv = _bcm_td2_l3_ip4_options_profile_reinit_hw_profiles_update(unit);
        }
    }
    return rv;
}

/* 
 * Function:
 *      _bcm_td2_l3_ip4_options_sync
 * Purpose:
 *      Store IP Option Profile usage table into scache.
 * Parameters:
 *      unit    : (IN) Device Unit Number
 *      scache_ptr - (IN) Scache pointer
 *                   (OUT) Updated Scache pointer
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_l3_ip4_options_sync(int unit, uint8 **scache_ptr)
{
    uint32 hw_idx;
    int    idx;
   
    if ((scache_ptr == NULL) || (*scache_ptr == NULL)) {
        return BCM_E_PARAM;
    }

    /* now store the state into the compressed format */
    for (idx = 0; idx < _BCM_IP4_OPTIONS_LEN; idx++) {
        if (_BCM_IP4_OPTIONS_USED_GET(unit, idx)) {
            hw_idx = L3_INFO(unit)->ip4_profiles_hw_idx[idx];
        } else {
            hw_idx = _BCM_IP_OPTION_REINIT_INVALID_HW_IDX;
        }
        sal_memcpy((*scache_ptr), &hw_idx, sizeof(uint32));
        (*scache_ptr) += sizeof(hw_idx);
    }
    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef ALPM_ENABLE
/* ALPM Functions */

/*
 * Function:	
 *     _bcm_td2_alpm_mem_ip6_defip_set
 * Purpose:
 *    Set an IP6 address field in L3_DEFIPm
 * Parameters: 
 *    unit    - (IN) SOC unit number; 
 *    lpm_key - (OUT) Buffer to fill. 
 *    lpm_cfg - (IN) Route information. 
 *    alpm_entry - (OUT) ALPM entry to fill
 * Note:
 *    See soc_mem_ip6_addr_set()
 */
STATIC void
_bcm_td2_mem_ip6_alpm_set(int unit, _bcm_defip_cfg_t *lpm_cfg, 
                          defip_entry_t *lpm_key)
{
    bcm_ip6_t mask;                      /* Subnet mask.        */
    uint8 *ip6;                 /* Ip6 address.       */
    uint32 ip6_word[2];         /* Temp storage.      */
    int idx;                    /* Iteration index .  */

    /* Just to keep variable name short. */
    ip6 = lpm_cfg->defip_ip6_addr;

    /* Create mask from prefix length. */
    bcm_ip6_mask_create(mask, lpm_cfg->defip_sub_len);

    /* Apply subnet mask */
    idx = lpm_cfg->defip_sub_len / 8;   /* Unchanged byte count.    */
    ip6[idx] &= mask[idx];      /* Apply mask on next byte. */
    for (idx++; idx < BCM_IP6_ADDRLEN; idx++) {
        ip6[idx] = 0;           /* Reset rest of bytes.     */
    }

    ip6_word[1] = ((ip6[0] << 24) | (ip6[1] << 16) | (ip6[2] << 8) | (ip6[3]));
    ip6_word[0]= ((ip6[4] << 24) | (ip6[5] << 16) | (ip6[6] << 8) | (ip6[7]));

    /* Set IP Addr */
    soc_L3_DEFIPm_field_set(unit, lpm_key, IP_ADDR1f, (void *)&ip6_word[1]);
    soc_L3_DEFIPm_field_set(unit, lpm_key, IP_ADDR0f, (void *)&ip6_word[0]);

    /* Set IP MASK */
    ip6_word[0] = ((mask[0] << 24) | (mask[1] << 16) | (mask[2] << 8) | (mask[3]));
    soc_L3_DEFIPm_field_set(unit, lpm_key, IP_ADDR_MASK1f, (void *)&ip6_word[0]);

    ip6_word[0] = ((mask[4] << 24) | (mask[5] << 16) | (mask[6] << 8) | (mask[7]));
    soc_L3_DEFIPm_field_set(unit, lpm_key, IP_ADDR_MASK0f, (void *)&ip6_word[0]);
}


/*
 * Function:
 *     _bcm_td2_mem_ip6_alpm_get
 * Purpose:
 *    Get an IP6 address from field L3_DEFIPm
 * Note:
 *    See soc_mem_ip6_addr_set()
 */
void
_bcm_td2_mem_ip6_alpm_get(int unit, _bcm_defip_cfg_t *lpm_cfg, 
                          soc_mem_t mem, void *alpm_entry)
{
    uint8 *ip6;                 /* Ip6 address.       */
    uint8 mask[BCM_IP6_ADDRLEN];        /* Subnet mask.       */
    uint32 ip6_word[2];            /* Temp storage.      */
    uint32 length;
    uint32 *bufp;

    bufp = (uint32 *)alpm_entry;

    sal_memset(mask, 0, sizeof (bcm_ip6_t));

    /* Just to keep variable name short. */
    ip6 = lpm_cfg->defip_ip6_addr;
    sal_memset(ip6, 0, sizeof (bcm_ip6_t));

    soc_mem_field_get(unit, mem, bufp, KEYf, ip6_word);
    ip6[0] = (uint8) ((ip6_word[1] >> 24) & 0xff);
    ip6[1] = (uint8) ((ip6_word[1] >> 16) & 0xff);
    ip6[2] = (uint8) ((ip6_word[1] >> 8) & 0xff);
    ip6[3] = (uint8) (ip6_word[1] & 0xff);

    ip6[4] = (uint8) ((ip6_word[0] >> 24) & 0xff);
    ip6[5] = (uint8) ((ip6_word[0] >> 16) & 0xff);
    ip6[6] = (uint8) ((ip6_word[0] >> 8) & 0xff);
    ip6[7] = (uint8) (ip6_word[0] & 0xff);

    length = soc_mem_field32_get(unit, mem, bufp, LENGTHf);
    lpm_cfg->defip_sub_len = length;
}

/*
 * Function:
 *      _bcm_fb_lpm_clear_hit
 * Purpose:
 *      Clear prefix entry hit bit.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      lpm_cfg   - (IN)Route info.  
 *      lpm_entry - (IN)Buffer to write to hw. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_alpm_clear_hit(int unit, _bcm_defip_cfg_t *lpm_cfg,
                        void *alpm_entry)
{
#if 0
    int rv;                        /* Operation return status. */
    int tbl_idx;                   /* Defip table index.       */
    soc_field_t hit_field = HIT0f; /* HIT bit field id.        */
    uint32 *bufp;

    bufp = (uint32 *)alpm_entry;

    /* Input parameters check */
    if ((NULL == lpm_cfg) || (NULL == lpm_entry)) {
        return (BCM_E_PARAM);
    }

    /* If entry was not hit  clear "clear hit" and return */
    if (!(lpm_cfg->defip_flags & BCM_L3_HIT)) {
        return (BCM_E_NONE);
    }

    soc_mem_field32_set(unit, mem, bufp, HIT1f, &length);


    /* Reset entry hit bit in buffer. */
    if (lpm_cfg->defip_flags & BCM_L3_IP6) {
        tbl_idx = lpm_cfg->defip_index;
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, HIT1f, 0);
    } else {
        tbl_idx = lpm_cfg->defip_index >> 1;
        rv = READ_L3_DEFIPm(unit, MEM_BLOCK_ALL, tbl_idx, lpm_entry);
        if (lpm_cfg->defip_index &= 0x1) {
            hit_field = HIT1f;
        }
    }
    soc_L3_DEFIPm_field32_set(unit, lpm_entry, hit_field, 0);

    /* Write lpm buffer to hardware. */
    rv = BCM_XGS3_MEM_WRITE(unit, L3_DEFIPm, tbl_idx, lpm_entry);
    return rv;
#endif

    return SOC_E_NONE;
}

/*
 * Function:
 *      bcm_td2_internal_lpm_vrf_calc
 * Purpose:
 *      Service routine used to translate API vrf id to hw specific.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      lpm_cfg   - (IN)Prefix info.
 *      vrf_id    - (OUT)Internal vrf id.
 *      vrf_mask  - (OUT)Internal vrf mask.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_internal_lpm_vrf_calc(int unit, _bcm_defip_cfg_t *lpm_cfg, 

                              int *vrf_id, int *vrf_mask)
{
    /* Handle special vrf id cases. */
    switch (lpm_cfg->defip_vrf) {
      case BCM_L3_VRF_OVERRIDE:
      case BCM_L3_VRF_GLOBAL: /* only for td2 */
          *vrf_id = 0;
          *vrf_mask = 0;
          break;
      default:   
          *vrf_id = lpm_cfg->defip_vrf;
          *vrf_mask = SOC_VRF_MAX(unit);
    }

    /* In any case vrf id shouldn't exceed max field mask. */
    if ((*vrf_id < 0) || (*vrf_id > SOC_VRF_MAX(unit))) {
        return (BCM_E_PARAM);
    } 
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_td2_alpm_ent_init
 * Purpose:
 *      Service routine used to initialize lkup key for lpm entry.
 *      Also initializes the ALPM SRAM entry.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      lpm_cfg   - (IN)Prefix info.
 *      lpm_entry - (OUT)Hw buffer to fill.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_alpm_ent_init(int unit, _bcm_defip_cfg_t *lpm_cfg,
                       defip_entry_t *lpm_entry, int nh_ecmp_idx, uint32 *flags)
{
    bcm_ip_t ip4_mask;
    int vrf_id;
    int vrf_mask;
    int ipv6;

    ipv6 = lpm_cfg->defip_flags & BCM_L3_IP6;

    /* Extract entry  vrf id  & vrf mask. */
    BCM_IF_ERROR_RETURN
        (bcm_td2_internal_lpm_vrf_calc(unit, lpm_cfg, &vrf_id, &vrf_mask));


    /* Zero buffers. */
    sal_memset(lpm_entry, 0, BCM_XGS3_L3_ENT_SZ(unit, defip));

    /* Set hit bit. */
    if (lpm_cfg->defip_flags & BCM_L3_HIT) {
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, HIT0f, 1);
    }

    /* Set priority override bit. */
    if (lpm_cfg->defip_flags & BCM_L3_RPE) {
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, RPE0f, 1);
    }

    /* Write priority field. */
    soc_L3_DEFIPm_field32_set(unit, lpm_entry, PRI0f, lpm_cfg->defip_prio);

    /* Fill next hop information. */
    if (lpm_cfg->defip_flags & BCM_L3_MULTIPATH) {
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, ECMP0f, 1);
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, NEXT_HOP_INDEX0f, 
                                  nh_ecmp_idx);
    } else {
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, NEXT_HOP_INDEX0f,
                                  nh_ecmp_idx);
    }


    /* Set destination discard flag. */
    if (lpm_cfg->defip_flags & BCM_L3_DST_DISCARD) {
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, DST_DISCARD0f, 1);
    }

    /* remember src discard flag */
    if (lpm_cfg->defip_flags & BCM_L3_SRC_DISCARD) {
        *flags |= SOC_ALPM_RPF_SRC_DISCARD;
    }

    /* Set classification group id. */
    soc_L3_DEFIPm_field32_set(unit, lpm_entry, CLASS_ID0f, 
                              lpm_cfg->defip_lookup_class);

    /* Set Global route flag. */
    if (BCM_L3_VRF_GLOBAL == lpm_cfg->defip_vrf) {
        soc_mem_field32_set(unit, L3_DEFIPm, lpm_entry, GLOBAL_ROUTE0f, 0x1);
    }

    /* Indicate this is an override entry */
    if (BCM_L3_VRF_OVERRIDE == lpm_cfg->defip_vrf) {
        soc_mem_field32_set(unit, L3_DEFIPm, lpm_entry, GLOBAL_HIGH0f, 0x1);
        soc_mem_field32_set(unit, L3_DEFIPm, lpm_entry, GLOBAL_ROUTE0f, 0x1);
    }

    /* Set VRF */
    soc_L3_DEFIPm_field32_set(unit, lpm_entry, VRF_ID_0f, vrf_id);
    soc_L3_DEFIPm_field32_set(unit, lpm_entry, VRF_ID_MASK0f, vrf_mask);

    if (ipv6) {
        /* Set prefix ip address & mask. */
        _bcm_td2_mem_ip6_alpm_set(unit, lpm_cfg,lpm_entry);

        /* Set second part valid bit. */
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, VALID1f, 1);

        /* Set mode to ipv6 */
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, MODE0f, 1);
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, MODE1f, 1);

        /* Set Virtual Router id */
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, VRF_ID_1f, vrf_id);
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, VRF_ID_MASK1f, vrf_mask);
    } else {
        ip4_mask = BCM_IP4_MASKLEN_TO_ADDR(lpm_cfg->defip_sub_len);
        /* Apply subnet mask. */
        lpm_cfg->defip_ip_addr &= ip4_mask;

        /* Set address to the buffer. */
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, IP_ADDR0f,
                                  lpm_cfg->defip_ip_addr);
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, IP_ADDR_MASK0f, ip4_mask);
    }
    soc_L3_DEFIPm_field32_set(unit, lpm_entry, VALID0f, 1);

    /* Set Mode Masks */
    soc_mem_field32_set(unit, L3_DEFIPm, lpm_entry, MODE_MASK0f, 
                        (1 << soc_mem_field_length(unit, L3_DEFIPm, MODE_MASK0f)) - 1);
    soc_mem_field32_set(unit, L3_DEFIPm, lpm_entry, MODE_MASK1f, 
                        (1 << soc_mem_field_length(unit, L3_DEFIPm, MODE_MASK1f)) - 1);

    /* Set Entry Type Masks */
    soc_mem_field32_set(unit, L3_DEFIPm, lpm_entry, ENTRY_TYPE0f, 
                        (1 << soc_mem_field_length(unit, L3_DEFIPm, ENTRY_TYPE0f)) - 1);
    soc_mem_field32_set(unit, L3_DEFIPm, lpm_entry, ENTRY_TYPE1f, 
                        (1 << soc_mem_field_length(unit, L3_DEFIPm, ENTRY_TYPE1f)) - 1);
    /* 
     * Note ipv4 entries are expected to reside in part 0 of the entry.
     *      ipv6 entries both parts should be filled.  
     *      Hence if entry is ipv6 copy part 0 to part 1
     */
#if 0
    if (lpm_cfg->defip_flags & BCM_L3_IP6) {
        soc_alpm_lpm_ip4entry0_to_1(unit, lpm_entry, lpm_entry, TRUE);
    }
#endif
    return (BCM_E_NONE);
}

/*
 * Function:    
 *     _td2_defip_pair128_ip6_addr_set
 * Purpose:  
 *     Set IP6 address field in memory from ip6 addr type. 
 * Parameters: 
 *     unit  - (IN) BCM device number. 
 *     mem   - (IN) Memory id.
 *     entry - (IN) HW entry buffer.
 *     ip6   - (IN) SW ip6 address buffer.
 * Returns:      void
 */
STATIC void
_td2_defip_pair128_ip6_addr_set(int unit, soc_mem_t mem, uint32 *entry, 
                                const ip6_addr_t ip6)
{
    uint32              ip6_field[4];
    ip6_field[3] = ((ip6[12] << 24)| (ip6[13] << 16) |
                    (ip6[14] << 8) | (ip6[15] << 0));
    soc_mem_field_set(unit, mem, entry, IP_ADDR0_LWRf, &ip6_field[3]);
    ip6_field[2] = ((ip6[8] << 24) | (ip6[9] << 16) |
                    (ip6[10] << 8) | (ip6[11] << 0));
    soc_mem_field_set(unit, mem, entry, IP_ADDR1_LWRf, &ip6_field[2]);
    ip6_field[1] = ((ip6[4] << 24) | (ip6[5] << 16) |
                    (ip6[6] << 8)  | (ip6[7] << 0));
    soc_mem_field_set(unit, mem, entry, IP_ADDR0_UPRf, &ip6_field[1]);
    ip6_field[0] = ((ip6[0] << 24) | (ip6[1] << 16) |
                    (ip6[2] << 8)  | (ip6[3] << 0));
    soc_mem_field_set(unit, mem, entry, IP_ADDR1_UPRf, ip6_field);
}

/*
 * Function:    
 *     _td2_defip_pair128_ip6_mask_set
 * Purpose:  
 *     Set IP6 mask field in memory from ip6 addr type. 
 * Parameters: 
 *     unit  - (IN) BCM device number. 
 *     mem   - (IN) Memory id.
 *     entry - (IN) HW entry buffer.
 *     ip6   - (IN) SW ip6 address buffer.
 * Returns:      void
 */
STATIC void
_td2_defip_pair128_ip6_mask_set(int unit, soc_mem_t mem, uint32 *entry, 
                                const ip6_addr_t ip6)
{
    uint32              ip6_field[4];
    ip6_field[3] = ((ip6[12] << 24)| (ip6[13] << 16) |
                    (ip6[14] << 8) | (ip6[15] << 0));
    soc_mem_field_set(unit, mem, entry, IP_ADDR_MASK0_LWRf, &ip6_field[3]);
    ip6_field[2] = ((ip6[8] << 24) | (ip6[9] << 16) |
                    (ip6[10] << 8) | (ip6[11] << 0));
    soc_mem_field_set(unit, mem, entry, IP_ADDR_MASK1_LWRf, &ip6_field[2]);
    ip6_field[1] = ((ip6[4] << 24) | (ip6[5] << 16) |
                    (ip6[6] << 8)  | (ip6[7] << 0));
    soc_mem_field_set(unit, mem, entry, IP_ADDR_MASK0_UPRf, &ip6_field[1]);
    ip6_field[0] = ((ip6[0] << 24) | (ip6[1] << 16) |
                    (ip6[2] << 8)  | (ip6[3] << 0));
    soc_mem_field_set(unit, mem, entry, IP_ADDR_MASK1_UPRf, ip6_field);
}

STATIC int
_bcm_td2_alpm_128_ent_init(int unit, _bcm_defip_cfg_t *lpm_cfg,
             defip_pair_128_entry_t *lpm_entry, int nh_ecmp_idx, uint32 *flags)
{
    int vrf_id;
    int vrf_mask;
    bcm_ip6_t mask;                          /* Subnet mask.              */

    /* Extract entry  vrf id  & vrf mask. */
    BCM_IF_ERROR_RETURN
        (bcm_td2_internal_lpm_vrf_calc(unit, lpm_cfg, &vrf_id, &vrf_mask));

    /* Zero buffers. */
    sal_memset(lpm_entry, 0, sizeof(*lpm_entry));

    /* Set hit bit. */
    if (lpm_cfg->defip_flags & BCM_L3_HIT) {
        soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, HITf, 1);
    }

    /* Set priority override bit. */
    if (lpm_cfg->defip_flags & BCM_L3_RPE) {
        soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, RPEf, 1);
    }

    /* Write priority field. */
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, PRIf, lpm_cfg->defip_prio);

    /* Fill next hop information. */
    if (lpm_cfg->defip_flags & BCM_L3_MULTIPATH) {
        soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, ECMPf, 1);
        soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, NEXT_HOP_INDEXf, 
                                  nh_ecmp_idx);
    } else {
        soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, NEXT_HOP_INDEXf,
                                  nh_ecmp_idx);
    }

    /* Set destination discard flag. */
    if (lpm_cfg->defip_flags & BCM_L3_DST_DISCARD) {
        soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, DST_DISCARDf, 1);
    }

    /* remember src discard flag */
    if (lpm_cfg->defip_flags & BCM_L3_SRC_DISCARD) {
        *flags |= SOC_ALPM_RPF_SRC_DISCARD;
    }

    /* Set classification group id. */
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, CLASS_IDf, 
                              lpm_cfg->defip_lookup_class);

    /* Set Global route flag. */
    if (BCM_L3_VRF_GLOBAL == lpm_cfg->defip_vrf) {
        soc_mem_field32_set(unit, L3_DEFIP_PAIR_128m, lpm_entry, GLOBAL_ROUTEf, 0x1);
    }

    /* Indicate this is an override entry */
    if (BCM_L3_VRF_OVERRIDE == lpm_cfg->defip_vrf) {
        soc_mem_field32_set(unit, L3_DEFIP_PAIR_128m, lpm_entry, GLOBAL_HIGHf, 0x1);
        soc_mem_field32_set(unit, L3_DEFIP_PAIR_128m, lpm_entry, GLOBAL_ROUTEf, 0x1);
    }

    /* Create mask from prefix length. */
    bcm_ip6_mask_create(mask, lpm_cfg->defip_sub_len);

    /* Apply mask on address. */
    bcm_xgs3_l3_mask6_apply(mask, lpm_cfg->defip_ip6_addr);

    /* Set prefix ip address & mask. */
    _td2_defip_pair128_ip6_addr_set(unit, L3_DEFIP_PAIR_128m, 
                                (uint32 *) lpm_entry, lpm_cfg->defip_ip6_addr);

    _td2_defip_pair128_ip6_mask_set(unit, L3_DEFIP_PAIR_128m, 
                                   (uint32 *) lpm_entry, mask);

    /* Set Virtual Router id */
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VRF_ID_0_LWRf, vrf_id);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VRF_ID_1_LWRf, vrf_id);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VRF_ID_0_UPRf, vrf_id);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VRF_ID_1_UPRf, vrf_id);

    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VRF_ID_MASK0_LWRf, 
                                       vrf_mask);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VRF_ID_MASK1_LWRf, 
                                       vrf_mask);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VRF_ID_MASK0_UPRf, 
                                       vrf_mask);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VRF_ID_MASK1_UPRf, 
                                       vrf_mask);

    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VALID0_LWRf, 1);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VALID1_LWRf, 1);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VALID0_UPRf, 1);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, VALID1_UPRf, 1);

    /* Set mode to ipv6-128 */
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, MODE0_LWRf, 3);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, MODE1_LWRf, 3);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, MODE0_UPRf, 3);
    soc_L3_DEFIP_PAIR_128m_field32_set(unit, lpm_entry, MODE1_UPRf, 3);

    /* Set Mode Masks */
    soc_mem_field32_set(unit, L3_DEFIP_PAIR_128m, lpm_entry, MODE_MASK0_LWRf, 
    (1 << soc_mem_field_length(unit, L3_DEFIP_PAIR_128m, MODE_MASK0_LWRf)) - 1);
    soc_mem_field32_set(unit, L3_DEFIP_PAIR_128m, lpm_entry, MODE_MASK1_LWRf, 
    (1 << soc_mem_field_length(unit, L3_DEFIP_PAIR_128m, MODE_MASK1_LWRf)) - 1);
    soc_mem_field32_set(unit, L3_DEFIP_PAIR_128m, lpm_entry, MODE_MASK0_UPRf, 
    (1 << soc_mem_field_length(unit, L3_DEFIP_PAIR_128m, MODE_MASK0_UPRf)) - 1);
    soc_mem_field32_set(unit, L3_DEFIP_PAIR_128m, lpm_entry, MODE_MASK1_UPRf, 
    (1 << soc_mem_field_length(unit, L3_DEFIP_PAIR_128m, MODE_MASK1_UPRf)) - 1);

    /* Set Entry Type Masks */
    soc_mem_field32_set(unit, L3_DEFIP_PAIR_128m, lpm_entry, ENTRY_TYPE0_LWRf, 
     (1<<soc_mem_field_length(unit, L3_DEFIP_PAIR_128m, ENTRY_TYPE0_LWRf)) - 1);
    soc_mem_field32_set(unit, L3_DEFIP_PAIR_128m, lpm_entry, ENTRY_TYPE1_LWRf, 
     (1<<soc_mem_field_length(unit, L3_DEFIP_PAIR_128m, ENTRY_TYPE1_LWRf)) - 1);
    soc_mem_field32_set(unit, L3_DEFIP_PAIR_128m, lpm_entry, ENTRY_TYPE0_UPRf, 
     (1<<soc_mem_field_length(unit, L3_DEFIP_PAIR_128m, ENTRY_TYPE0_UPRf)) - 1);
    soc_mem_field32_set(unit, L3_DEFIP_PAIR_128m, lpm_entry, ENTRY_TYPE1_UPRf, 
     (1<<soc_mem_field_length(unit, L3_DEFIP_PAIR_128m, ENTRY_TYPE1_UPRf)) - 1);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_td2_lpm_ent_parse
 * Purpose:
 *      Parse an entry from DEFIP table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (OUT)Buffer to fill defip information. 
 *      nh_ecmp_idx - (OUT)Next hop index or ecmp group id.  
 *      lpm_entry   - (IN) Buffer read from hw. 
 * Returns:
 *      void
 */
STATIC INLINE void
_bcm_td2_lpm_ent_parse(int unit, _bcm_defip_cfg_t *lpm_cfg, int *nh_ecmp_idx,
                      defip_entry_t *lpm_entry)
{
    int ipv6 = soc_L3_DEFIPm_field32_get(unit, lpm_entry, MODE0f);

    /* Reset entry flags first. */
    lpm_cfg->defip_flags = 0;

    /* Check if entry points to ecmp group. */
   /*hurricane does not have ecmp flag*/	
    if ((!(SOC_IS_HURRICANE(unit))) &&
	(soc_L3_DEFIPm_field32_get(unit, lpm_entry, ECMP0f))) {
        /* Mark entry as ecmp */
        lpm_cfg->defip_ecmp = 1;
        lpm_cfg->defip_flags |= BCM_L3_MULTIPATH;

        /* Get ecmp group id. */
        if (nh_ecmp_idx) {
            *nh_ecmp_idx =
                soc_L3_DEFIPm_field32_get(unit, lpm_entry, ECMP_PTR0f);
        }
    } else {
        /* Mark entry as non-ecmp. */
        lpm_cfg->defip_ecmp = 0;

        /* Reset ecmp group next hop count. */
        lpm_cfg->defip_ecmp_count = 0;

        /* Get next hop index. */
        if (nh_ecmp_idx) {
            *nh_ecmp_idx =
                soc_L3_DEFIPm_field32_get(unit, lpm_entry, NEXT_HOP_INDEX0f);
        }
    }
    /* Get entry priority. */
    lpm_cfg->defip_prio = soc_L3_DEFIPm_field32_get(unit, lpm_entry, PRI0f);

    /* Get hit bit. */
    if (soc_L3_DEFIPm_field32_get(unit, lpm_entry, HIT0f)) {
        lpm_cfg->defip_flags |= BCM_L3_HIT;
    }

    /* Get priority override bit. */
    if (soc_L3_DEFIPm_field32_get(unit, lpm_entry, RPE0f)) {
        lpm_cfg->defip_flags |= BCM_L3_RPE;
    }

    /* Get destination discard flag. */
    if (SOC_MEM_FIELD_VALID(unit, L3_DEFIPm, DST_DISCARD0f)) {
        if(soc_L3_DEFIPm_field32_get(unit, lpm_entry, DST_DISCARD0f)) {
            lpm_cfg->defip_flags |= BCM_L3_DST_DISCARD;
        }
    }

#if defined(BCM_TRX_SUPPORT)
    /* Set classification group id. */
    if (SOC_MEM_FIELD_VALID(unit, L3_DEFIPm, CLASS_ID0f)) {
        lpm_cfg->defip_lookup_class = 
            soc_L3_DEFIPm_field32_get(unit, lpm_entry, CLASS_ID0f);
    }
#endif /* BCM_TRX_SUPPORT */


    if (ipv6) {
        lpm_cfg->defip_flags |= BCM_L3_IP6;
        /* Get hit bit from the second part of the entry. */
        if (soc_L3_DEFIPm_field32_get(unit, lpm_entry, HIT1f)) {
            lpm_cfg->defip_flags |= BCM_L3_HIT;
        }

        /* Get priority override bit from the second part of the entry. */
        if (soc_L3_DEFIPm_field32_get(unit, lpm_entry, RPE1f)) {
            lpm_cfg->defip_flags |= BCM_L3_RPE;
        }
    }
    return;
}

STATIC INLINE void
_bcm_td2_lpm_128_ent_parse(int unit, _bcm_defip_cfg_t *lpm_cfg, 
                           int *nh_ecmp_idx, defip_pair_128_entry_t *lpm_entry)
{
    int ipv6 = soc_L3_DEFIP_PAIR_128m_field32_get(unit, lpm_entry, MODE0_LWRf);

    /* Reset entry flags first. */
    lpm_cfg->defip_flags = 0;

    /* Check if entry points to ecmp group. */
   /*hurricane does not have ecmp flag*/	
    if ((!(SOC_IS_HURRICANE(unit))) &&
	    (soc_L3_DEFIP_PAIR_128m_field32_get(unit, lpm_entry, ECMPf))) {
        /* Mark entry as ecmp */
        lpm_cfg->defip_ecmp = 1;
        lpm_cfg->defip_flags |= BCM_L3_MULTIPATH;

        /* Get ecmp group id. */
        if (nh_ecmp_idx) {
            *nh_ecmp_idx =
                soc_L3_DEFIP_PAIR_128m_field32_get(unit, lpm_entry, ECMP_PTRf);
        }
    } else {
        /* Mark entry as non-ecmp. */
        lpm_cfg->defip_ecmp = 0;

        /* Reset ecmp group next hop count. */
        lpm_cfg->defip_ecmp_count = 0;

        /* Get next hop index. */
        if (nh_ecmp_idx) {
            *nh_ecmp_idx = soc_L3_DEFIP_PAIR_128m_field32_get(unit, 
                                                    lpm_entry, NEXT_HOP_INDEXf);
        }
    }
    /* Get entry priority. */
    lpm_cfg->defip_prio = 
                    soc_L3_DEFIP_PAIR_128m_field32_get(unit, lpm_entry, PRIf);

    /* Get hit bit. */
    if (soc_L3_DEFIP_PAIR_128m_field32_get(unit, lpm_entry, HITf)) {
        lpm_cfg->defip_flags |= BCM_L3_HIT;
    }

    /* Get priority override bit. */
    if (soc_L3_DEFIP_PAIR_128m_field32_get(unit, lpm_entry, RPEf)) {
        lpm_cfg->defip_flags |= BCM_L3_RPE;
    }

    /* Get destination discard flag. */
    if (SOC_MEM_FIELD_VALID(unit, L3_DEFIP_PAIR_128m, DST_DISCARDf)) {
        if(soc_L3_DEFIP_PAIR_128m_field32_get(unit, lpm_entry, DST_DISCARDf)) {
            lpm_cfg->defip_flags |= BCM_L3_DST_DISCARD;
        }
    }

#if defined(BCM_TRX_SUPPORT)
    /* Set classification group id. */
    if (SOC_MEM_FIELD_VALID(unit, L3_DEFIP_PAIR_128m, CLASS_IDf)) {
        lpm_cfg->defip_lookup_class = 
            soc_L3_DEFIP_PAIR_128m_field32_get(unit, lpm_entry, CLASS_IDf);
    }
#endif /* BCM_TRX_SUPPORT */

    if (ipv6) {
        lpm_cfg->defip_flags |= BCM_L3_IP6;
        /* Get hit bit from the second part of the entry. */
        if (soc_L3_DEFIP_PAIR_128m_field32_get(unit, lpm_entry, HITf)) {
            lpm_cfg->defip_flags |= BCM_L3_HIT;
        }

        /* Get priority override bit from the second part of the entry. */
        if (soc_L3_DEFIP_PAIR_128m_field32_get(unit, lpm_entry, RPEf)) {
            lpm_cfg->defip_flags |= BCM_L3_RPE;
        }
    }
    return;
}

/*
 * Function:
 *      _bcm_td2_l3_defip_mem_get
 * Purpose:
 *      Resolve route table memory for a given route entry.
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      flags      - (IN)IPv6/IPv4 route.
 *      plen       - (IN)Prefix length.
 *      mem        - (OUT)Route table memory.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_l3_defip_mem_get(int unit, uint32 flags, int plen, soc_mem_t *mem)
{
    /* Default value */
    *mem = L3_DEFIPm;
    if ((flags & BCM_L3_IP6) && 
        (soc_mem_index_count(unit, L3_DEFIP_PAIR_128m) > 0)) {
        *mem = L3_DEFIP_PAIR_128m;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_alpm_get
 * Purpose:
 *      Get an entry from DEFIP table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 *      nh_ecmp_idx - (IN)Next hop or ecmp group index
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_alpm_get(int unit, _bcm_defip_cfg_t *lpm_cfg, int *nh_ecmp_idx)
{
    defip_entry_t lpm_key;      /* Route lookup key.        */
    defip_entry_t lpm_entry;    /* Search result buffer.    */
    defip_pair_128_entry_t lpm_128_key;
    defip_pair_128_entry_t lpm_128_entry;
    int clear_hit;              /* Clear hit indicator.     */
    int rv;                     /* Operation return status. */
    uint32 flags = 0;
    soc_mem_t   mem;

    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    /* Zero buffers. */
    sal_memset(&lpm_entry, 0, BCM_XGS3_L3_ENT_SZ(unit, defip));
    sal_memset(&lpm_key, 0, BCM_XGS3_L3_ENT_SZ(unit, defip));

    /* Check if clear hit bit required. */
    clear_hit = lpm_cfg->defip_flags & BCM_L3_HIT_CLEAR;

    BCM_IF_ERROR_RETURN(_bcm_td2_l3_defip_mem_get(unit, lpm_cfg->defip_flags,
                                              lpm_cfg->defip_sub_len, &mem));
    switch (mem) {
      case L3_DEFIP_PAIR_128m:
       /* Initialize lkup key. */
        BCM_IF_ERROR_RETURN(_bcm_td2_alpm_128_ent_init(unit, lpm_cfg, 
                             &lpm_128_key, 0, &flags));
        /* Perform hw lookup. */
        rv = soc_alpm_128_lookup(unit, &lpm_128_key, &lpm_128_entry, 
                                 &lpm_cfg->defip_index);
        BCM_IF_ERROR_RETURN(rv);

        /* Parse hw buffer to defip entry. */
        _bcm_td2_lpm_128_ent_parse(unit, lpm_cfg, nh_ecmp_idx, &lpm_128_entry);
        break;

      default:
        /* Initialize lkup key. */
        BCM_IF_ERROR_RETURN(_bcm_td2_alpm_ent_init(unit, lpm_cfg, &lpm_key, 0, 
                                                    &flags));
        /* Perform hw lookup. */
        rv = soc_alpm_lookup(unit, &lpm_key, &lpm_entry, &lpm_cfg->defip_index);
        BCM_IF_ERROR_RETURN(rv);
    
        /* Parse hw buffer to defip entry. */
        _bcm_td2_lpm_ent_parse(unit, lpm_cfg, nh_ecmp_idx, &lpm_entry);
        break;
    }

    /* Clear the HIT bit */
    if (clear_hit) {
        BCM_IF_ERROR_RETURN(_bcm_td2_alpm_clear_hit(unit, lpm_cfg, &lpm_entry));
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_td2_alpm_add
 * Purpose:
 *      Add an entry to DEFIP table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 *      nh_ecmp_idx - (IN)Next hop or ecmp group index.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_alpm_add(int unit, _bcm_defip_cfg_t *lpm_cfg, int nh_ecmp_idx)
{
    defip_entry_t lpm_entry;
    defip_pair_128_entry_t lpm_128_entry;
    int rv;                     /* Operation return status. */
    uint32  flags= 0;
    soc_mem_t   mem;

    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_td2_l3_defip_mem_get(unit, lpm_cfg->defip_flags,
                                              lpm_cfg->defip_sub_len, &mem));
    switch (mem) {
      case L3_DEFIP_PAIR_128m:
        BCM_IF_ERROR_RETURN(_bcm_td2_alpm_128_ent_init(unit, lpm_cfg, 
                            &lpm_128_entry, nh_ecmp_idx, &flags));
        rv = soc_alpm_128_insert(unit, &lpm_128_entry, flags);
        break;
      default:
        /* 
         * Initialize hw buffer from lpm configuration. 
         * NOTE: DON'T MOVE _bcm_fb_defip_ent_init CALL UP, 
         * We want to copy flags & nh info, avoid  ipv6 mask & address 
         * corruption
         */
        BCM_IF_ERROR_RETURN(_bcm_td2_alpm_ent_init(unit, lpm_cfg, &lpm_entry,
                            nh_ecmp_idx, &flags));

        /* Write buffer to hw. */
        rv = soc_alpm_insert(unit, &lpm_entry, flags);
        break;
    }

    /* If new route added increment total number of routes.  */
    /* Lack of index indicates a new route. */
    if ((rv >= 0) && (BCM_XGS3_L3_INVALID_INDEX == lpm_cfg->defip_index)) {
        BCM_XGS3_L3_DEFIP_CNT_INC(unit, (lpm_cfg->defip_flags & BCM_L3_IP6));
    }
    return rv;
}

/*
 * Function:
 *      _bcm_fb_lpm_del
 * Purpose:
 *      Delete an entry from DEFIP table.
 * Parameters:
 *      unit    - (IN)SOC unit number.
 *      lpm_cfg - (IN)Buffer to fill defip information. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_alpm_del(int unit, _bcm_defip_cfg_t *lpm_cfg)
{
    int rv;                     /* Operation return status. */
    defip_entry_t lpm_entry;    /* Search result buffer.    */
    defip_pair_128_entry_t lpm_128_entry;    /* Search result buffer.    */
    uint32 flags;
    soc_mem_t   mem;

    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    /* Zero buffers. */
    sal_memset(&lpm_entry, 0, BCM_XGS3_L3_ENT_SZ(unit, defip));
    sal_memset(&lpm_128_entry, 0, sizeof(lpm_128_entry));

    BCM_IF_ERROR_RETURN(_bcm_td2_l3_defip_mem_get(unit, lpm_cfg->defip_flags,
                                              lpm_cfg->defip_sub_len, &mem));
    
    switch (mem) {
      case L3_DEFIP_PAIR_128m:
        /* Initialize hw buffer deletion key. */
        BCM_IF_ERROR_RETURN(_bcm_td2_alpm_128_ent_init(unit, lpm_cfg, 
                                                   &lpm_128_entry, 0, &flags));
        rv = soc_alpm_128_delete(unit, &lpm_128_entry);
        break;
      default:
        /* Initialize hw buffer deletion key. */
        BCM_IF_ERROR_RETURN(_bcm_td2_alpm_ent_init(unit, lpm_cfg, &lpm_entry, 0,
                                               &flags));
        /* Write buffer to hw. */
        rv = soc_alpm_delete(unit, &lpm_entry);
        break;
    }

    /* If route deleted, decrement total number of routes.  */
    if (rv >= 0) {
        BCM_XGS3_L3_DEFIP_CNT_DEC(unit, lpm_cfg->defip_flags & BCM_L3_IP6);
    }
    return rv;
}

/*
 * Function:
 *     _bcm_fb_mem_ip6_defip_get
 * Purpose:
 *    Set an IP6 address field in an ALPM memory
 * Note:
 *    See soc_mem_ip6_addr_set()
 */
STATIC void
_bcm_td2_alpm_mem_ip6_defip_get(int unit, const void *lpm_key, soc_mem_t mem,
                                _bcm_defip_cfg_t *lpm_cfg)
{
    uint8 *ip6;                 /* Ip6 address.       */
    uint8 mask[BCM_IP6_ADDRLEN];        /* Subnet mask.       */
    uint32 ip6_word[2];            /* Temp storage.      */

    sal_memset(mask, 0, sizeof (bcm_ip6_t));

    /* Just to keep variable name short. */
    ip6 = lpm_cfg->defip_ip6_addr;
    sal_memset(ip6, 0, sizeof (bcm_ip6_t));

    soc_mem_field_get(unit, mem, lpm_key, KEYf, &ip6_word[0]);
    ip6[0] = (uint8) ((ip6_word[1] >> 24) & 0xff);
    ip6[1] = (uint8) ((ip6_word[1] >> 16) & 0xff);
    ip6[2] = (uint8) ((ip6_word[1] >> 8) & 0xff);
    ip6[3] = (uint8) (ip6_word[1] & 0xff);

    ip6[4] = (uint8) ((ip6_word[0] >> 24) & 0xff);
    ip6[5] = (uint8) ((ip6_word[0] >> 16) & 0xff);
    ip6[6] = (uint8) ((ip6_word[0] >> 8) & 0xff);
    ip6[7] = (uint8) (ip6_word[0] & 0xff);

    lpm_cfg->defip_sub_len = soc_mem_field32_get(unit, mem, lpm_key, LENGTHf);
}

/*
 * Function:
 *      _bcm_td2_alpm_ent_get_key
 * Purpose:
 *      Parse entry key from ALPM table.
 * Parameters:
 *      unit        - (IN) SOC unit number.
 *      lpm_cfg     - (OUT)Buffer to fill defip information. 
 *      *lpm_entry  - (IN) Pointer to lpm buffer read from hw. 
 *      alpm_mem    - (IN) ALPM memory type.
 *      *alpm_entry - (IN) Pointer to alpm entry read from SRAM bank.
 * Returns:
 *      void
 */
STATIC void
_bcm_td2_alpm_ent_get_key(int unit, _bcm_defip_cfg_t *lpm_cfg,
                        defip_entry_t *lpm_entry, soc_mem_t alpm_mem, 
                        void *alpm_entry)
{
    bcm_ip_t v4_mask;
    int ipv6 = lpm_cfg->defip_flags & BCM_L3_IP6;

    /* Set prefix ip address & mask. */
    if (ipv6) {
        _bcm_td2_alpm_mem_ip6_defip_get(unit, alpm_entry, alpm_mem, lpm_cfg);
    } else {
        /* Get ipv4 address. */
        lpm_cfg->defip_ip_addr = soc_mem_field32_get(unit, alpm_mem, 
                                                     alpm_entry, KEYf);

        /* Get subnet mask. */
        v4_mask = soc_mem_field32_get(unit, alpm_mem, alpm_entry, LENGTHf);
        if (v4_mask) {
            v4_mask = ~((1 << (32 - v4_mask)) - 1);
        }

        /* Fill mask length. */
        lpm_cfg->defip_sub_len = bcm_ip_mask_length(v4_mask);
    }

    /* Get Virtual Router id */
    soc_alpm_lpm_vrf_get(unit, lpm_entry, &lpm_cfg->defip_vrf, &ipv6);

    return;
}

/*
 * Function:
 *     _bcm_td2_mem_ip6_defip_get
 * Purpose:
 *    Set an IP6 address field in L3_DEFIPm
 * Note:
 *    See soc_mem_ip6_addr_set()
 */
STATIC void
_bcm_td2_mem_ip6_defip_get(int unit, const void *lpm_key,
                          _bcm_defip_cfg_t *lpm_cfg)
{
    uint8 *ip6;                 /* Ip6 address.       */
    uint8 mask[BCM_IP6_ADDRLEN];        /* Subnet mask.       */
    uint32 ip6_word;            /* Temp storage.      */

    sal_memset(mask, 0, sizeof (bcm_ip6_t));

    /* Just to keep variable name short. */
    ip6 = lpm_cfg->defip_ip6_addr;
    sal_memset(ip6, 0, sizeof (bcm_ip6_t));

    soc_L3_DEFIPm_field_get(unit, lpm_key, IP_ADDR1f, &ip6_word);
    ip6[0] = (uint8) ((ip6_word >> 24) & 0xff);
    ip6[1] = (uint8) ((ip6_word >> 16) & 0xff);
    ip6[2] = (uint8) ((ip6_word >> 8) & 0xff);
    ip6[3] = (uint8) (ip6_word & 0xff);

    soc_L3_DEFIPm_field_get(unit, lpm_key, IP_ADDR0f, &ip6_word);
    ip6[4] = (uint8) ((ip6_word >> 24) & 0xff);
    ip6[5] = (uint8) ((ip6_word >> 16) & 0xff);
    ip6[6] = (uint8) ((ip6_word >> 8) & 0xff);
    ip6[7] = (uint8) (ip6_word & 0xff);

    soc_L3_DEFIPm_field_get(unit, lpm_key, IP_ADDR_MASK1f, &ip6_word);
    mask[0] = (uint8) ((ip6_word >> 24) & 0xff);
    mask[1] = (uint8) ((ip6_word >> 16) & 0xff);
    mask[2] = (uint8) ((ip6_word >> 8) & 0xff);
    mask[3] = (uint8) (ip6_word & 0xff);

    soc_L3_DEFIPm_field_get(unit, lpm_key, IP_ADDR_MASK0f, &ip6_word);
    mask[4] = (uint8) ((ip6_word >> 24) & 0xff);
    mask[5] = (uint8) ((ip6_word >> 16) & 0xff);
    mask[6] = (uint8) ((ip6_word >> 8) & 0xff);
    mask[7] = (uint8) (ip6_word & 0xff);

    lpm_cfg->defip_sub_len = bcm_ip6_mask_length(mask);
}

/*
 * Function:
 *      _bcm_td2_lpm_ent_get_key
 * Purpose:
 *      Parse entry key from DEFIP table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (OUT)Buffer to fill defip information. 
 *      lpm_entry   - (IN) Buffer read from hw. 
 * Returns:
 *      void
 */
STATIC void
_bcm_td2_lpm_ent_get_key(int unit, _bcm_defip_cfg_t *lpm_cfg,
                        defip_entry_t *lpm_entry)
{
    bcm_ip_t v4_mask;
    int ipv6 = lpm_cfg->defip_flags & BCM_L3_IP6, tmp;

    /* Set prefix ip address & mask. */
    if (ipv6) {
        _bcm_td2_mem_ip6_defip_get(unit, lpm_entry, lpm_cfg);
    } else {
        /* Get ipv4 address. */
        lpm_cfg->defip_ip_addr =
            soc_L3_DEFIPm_field32_get(unit, lpm_entry, IP_ADDR0f);

        /* Get subnet mask. */
        v4_mask = soc_L3_DEFIPm_field32_get(unit, lpm_entry, IP_ADDR_MASK0f);

        /* Fill mask length. */
        lpm_cfg->defip_sub_len = bcm_ip_mask_length(v4_mask);
    }

    /* Get Virtual Router id */
    soc_alpm_lpm_vrf_get(unit, lpm_entry, &lpm_cfg->defip_vrf, &tmp);

    return;
}

/*
 * Function:
 *      _bcm_td2_alpm_ent_parse
 * Purpose:
 *      Parse an entry from DEFIP table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (OUT)Buffer to fill defip information. 
 *      nh_ecmp_idx - (OUT)Next hop index or ecmp group id.  
 *      lpm_entry   - (IN) Buffer read from hw. 
 * Returns:
 *      void
 */
STATIC void
_bcm_td2_alpm_ent_parse(int unit, _bcm_defip_cfg_t *lpm_cfg, int *nh_ecmp_idx,
                        defip_entry_t *lpm_entry, soc_mem_t alpm_mem, 
                        void *alpm_entry)
{
    int ipv6 = soc_L3_DEFIPm_field32_get(unit, lpm_entry, MODE0f);

    /* Reset entry flags first. */
    lpm_cfg->defip_flags = 0;

    /* Check if entry points to ecmp group. */
   /*hurricane does not have ecmp flag*/	
    if ((!(SOC_IS_HURRICANEX(unit))) &&
	(soc_mem_field32_get(unit, alpm_mem, alpm_entry, ECMPf))) {
        /* Mark entry as ecmp */
        lpm_cfg->defip_ecmp = 1;
        lpm_cfg->defip_flags |= BCM_L3_MULTIPATH;

        /* Get ecmp group id. */
        if (nh_ecmp_idx) {
            *nh_ecmp_idx =
                soc_mem_field32_get(unit, alpm_mem, alpm_entry, ECMP_PTRf);
        }
    } else {
        /* Mark entry as non-ecmp. */
        lpm_cfg->defip_ecmp = 0;

        /* Reset ecmp group next hop count. */
        lpm_cfg->defip_ecmp_count = 0;

        /* Get next hop index. */
        if (nh_ecmp_idx) {
            *nh_ecmp_idx =
               soc_mem_field32_get(unit, alpm_mem, alpm_entry, NEXT_HOP_INDEXf);
        }
    }
    /* Get entry priority. */
    lpm_cfg->defip_prio = soc_mem_field32_get(unit, alpm_mem, alpm_entry, 
                                             PRIf);

    /* Get hit bit. */
    if (soc_mem_field32_get(unit, alpm_mem, alpm_entry, HITf)) {
        lpm_cfg->defip_flags |= BCM_L3_HIT;
    }

    /* Get priority override bit. */
    if (soc_mem_field32_get(unit, alpm_mem, alpm_entry, RPEf)) {
        lpm_cfg->defip_flags |= BCM_L3_RPE;
    }

    /* Get destination discard flag. */
    if (SOC_MEM_FIELD_VALID(unit, alpm_mem, DST_DISCARDf)) {
        if(soc_mem_field32_get(unit, alpm_mem, alpm_entry, DST_DISCARDf)) {
            lpm_cfg->defip_flags |= BCM_L3_DST_DISCARD;
        }
    }

    /* Set classification group id. */
    lpm_cfg->defip_lookup_class = soc_mem_field32_get(unit, alpm_mem, 
                                                       alpm_entry, CLASS_IDf);

    if (ipv6) {
        lpm_cfg->defip_flags |= BCM_L3_IP6;
    }
    return;
}

STATIC int
_bcm_td2_alpm_128_get_addr(int unit, soc_mem_t mem, uint32 *alpm_entry, 
                           _bcm_defip_cfg_t *lpm_cfg)
{
    uint32 ip6_word[4];

    uint8 *ip6;

    ip6 = lpm_cfg->defip_ip6_addr;

    soc_mem_field_get(unit, mem, alpm_entry, KEYf, ip6_word);
    ip6[0] = (ip6_word[3] >> 24);
    ip6[1] = (ip6_word[3] >> 16) & 0xff;
    ip6[2] = (ip6_word[3] >> 8) & 0xff;
    ip6[3] = ip6_word[3];

    ip6[4] = ip6_word[2] >> 24;
    ip6[5] = (ip6_word[2] >> 16) & 0xff;
    ip6[6] = (ip6_word[2] >> 8) & 0xff;
    ip6[7] = ip6_word[2];

    ip6[8] = ip6_word[1] >> 24;
    ip6[9] = (ip6_word[1] >> 16) & 0xff;
    ip6[10] = (ip6_word[1] >> 8) & 0xff;
    ip6[11] = ip6_word[1];

    ip6[12] = ip6_word[0] >> 24;
    ip6[13] = (ip6_word[0] >> 16) & 0xff;
    ip6[14] = (ip6_word[0] >> 8) & 0xff;
    ip6[15] = ip6_word[0];

    lpm_cfg->defip_sub_len = 
                    soc_mem_field32_get(unit, mem, alpm_entry, LENGTHf);

    return SOC_E_NONE;
}

/*
 * Function:    
 *     _td2_defip_pair128_ip6_mask_get
 * Purpose:  
 *     Read IP6 mask field from memory field to ip6_addr_t buffer. 
 * Parameters: 
 *     unit  - (IN) BCM device number. 
 *     mem   - (IN) Memory id.
 *     entry - (IN) HW entry buffer.
 *     ip6   - (OUT) SW ip6 address buffer.
 * Returns:      void
 */
STATIC void
_td2_defip_pair128_ip6_mask_get(int unit, soc_mem_t mem, const void *entry, 
                                ip6_addr_t ip6)
{
    uint32              ip6_field[4];
    soc_mem_field_get(unit, mem, entry, IP_ADDR_MASK0_LWRf, (uint32 *)&ip6_field[3]);
    ip6[12] = (uint8) (ip6_field[3] >> 24);
    ip6[13] = (uint8) (ip6_field[3] >> 16 & 0xff);
    ip6[14] = (uint8) (ip6_field[3] >> 8 & 0xff);
    ip6[15] = (uint8) (ip6_field[3] & 0xff);
    soc_mem_field_get(unit, mem, entry, IP_ADDR_MASK1_LWRf, (uint32 *)&ip6_field[2]);
    ip6[8] = (uint8) (ip6_field[2] >> 24);
    ip6[9] = (uint8) (ip6_field[2] >> 16 & 0xff);
    ip6[10] = (uint8) (ip6_field[2] >> 8 & 0xff);
    ip6[11] = (uint8) (ip6_field[2] & 0xff);
    soc_mem_field_get(unit, mem, entry, IP_ADDR_MASK0_UPRf, (uint32 *)&ip6_field[1]);
    ip6[4] = (uint8) (ip6_field[1] >> 24);
    ip6[5] = (uint8) (ip6_field[1] >> 16 & 0xff);
    ip6[6] =(uint8) (ip6_field[1] >> 8 & 0xff);
    ip6[7] =(uint8) (ip6_field[1] & 0xff);
    soc_mem_field_get(unit, mem, entry, IP_ADDR_MASK1_UPRf, ip6_field);
    ip6[0] =(uint8) (ip6_field[0] >> 24);
    ip6[1] =(uint8) (ip6_field[0] >> 16 & 0xff);
    ip6[2] =(uint8) (ip6_field[0] >> 8 & 0xff);
    ip6[3] =(uint8) (ip6_field[0] & 0xff);
}

/*
 * Function:    
 *     _td2_defip_pair128_ip6_addr_get
 * Purpose:  
 *     Read IP6 address field from memory field to ip6_addr_t buffer. 
 * Parameters: 
 *     unit  - (IN) BCM device number. 
 *     mem   - (IN) Memory id.
 *     entry - (IN) HW entry buffer.
 *     ip6   - (OUT) SW ip6 address buffer.
 * Returns:      void
 */
STATIC void
_td2_defip_pair128_ip6_addr_get(int unit, soc_mem_t mem, const void *entry, 
                                ip6_addr_t ip6)
{
    uint32              ip6_field[4];
    soc_mem_field_get(unit, mem, entry, IP_ADDR0_LWRf, (uint32 *)&ip6_field[3]);
    ip6[12] = (uint8) (ip6_field[3] >> 24);
    ip6[13] = (uint8) (ip6_field[3] >> 16 & 0xff);
    ip6[14] = (uint8) (ip6_field[3] >> 8 & 0xff);
    ip6[15] = (uint8) (ip6_field[3] & 0xff);
    soc_mem_field_get(unit, mem, entry, IP_ADDR1_LWRf, (uint32 *)&ip6_field[2]);
    ip6[8] = (uint8) (ip6_field[2] >> 24);
    ip6[9] = (uint8) (ip6_field[2] >> 16 & 0xff);
    ip6[10] = (uint8) (ip6_field[2] >> 8 & 0xff);
    ip6[11] = (uint8) (ip6_field[2] & 0xff);
    soc_mem_field_get(unit, mem, entry, IP_ADDR0_UPRf, (uint32 *)&ip6_field[1]);
    ip6[4] = (uint8) (ip6_field[1] >> 24);
    ip6[5] = (uint8) (ip6_field[1] >> 16 & 0xff);
    ip6[6] =(uint8) (ip6_field[1] >> 8 & 0xff);
    ip6[7] =(uint8) (ip6_field[1] & 0xff);
    soc_mem_field_get(unit, mem, entry, IP_ADDR1_UPRf, ip6_field);
    ip6[0] =(uint8) (ip6_field[0] >> 24);
    ip6[1] =(uint8) (ip6_field[0] >> 16 & 0xff);
    ip6[2] =(uint8) (ip6_field[0] >> 8 & 0xff);
    ip6[3] =(uint8) (ip6_field[0] & 0xff);
}

/*
 * Function:
 *      _td2_defip_pair128_get_key
 * Purpose:
 *      Parse route entry key from L3_DEFIP_PAIR_128 table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      entry       - (IN)Hw entry buffer. 
 *      lpm_cfg     - (IN/OUT)Buffer to fill defip information. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_td2_defip_pair128_get_key(int unit, uint32 *hw_entry, 
                           _bcm_defip_cfg_t *lpm_cfg)
{
    soc_mem_t mem = L3_DEFIP_PAIR_128m;  /* Route table memory. */ 
    bcm_ip6_t mask;                      /* Subnet mask.        */

    /* Input parameters check. */
    if ((lpm_cfg == NULL) || (hw_entry == NULL)) {
        return (BCM_E_PARAM);
    }

    /* Extract ip6 address. */
    _td2_defip_pair128_ip6_addr_get(unit,mem, hw_entry, 
                                    lpm_cfg->defip_ip6_addr);

    /* Extract subnet mask. */
    _td2_defip_pair128_ip6_mask_get(unit, mem, hw_entry, mask);
    lpm_cfg->defip_sub_len = bcm_ip6_mask_length(mask);

    /* Extract vrf id & vrf mask. */
    if(!soc_mem_field32_get(unit, mem, hw_entry, VRF_ID_MASK0_LWRf)) {
        lpm_cfg->defip_vrf = BCM_L3_VRF_OVERRIDE;
    } else {
        lpm_cfg->defip_vrf = soc_mem_field32_get(unit, mem, hw_entry, 
                                                 VRF_ID_0_LWRf);
    }
    return (BCM_E_NONE);
}

STATIC int 
_bcm_td2_alpm_128_ent_parse(int unit, soc_mem_t mem, uint32 *hw_entry,
                            _bcm_defip_cfg_t *lpm_cfg, int *nh_ecmp_idx)
{
    /* Input parameters check. */
    if ((lpm_cfg == NULL) || (hw_entry == NULL)) {
        return (BCM_E_PARAM);
    }

    /* Reset entry flags first. */
    lpm_cfg->defip_flags = 0;

    /* Check if entry points to ecmp group. */
    if (soc_mem_field32_get(unit, mem, hw_entry, ECMPf)) {
        /* Mark entry as ecmp */
        lpm_cfg->defip_ecmp = TRUE;
        lpm_cfg->defip_flags |= BCM_L3_MULTIPATH;

        /* Get ecmp group id. */
        if (nh_ecmp_idx != NULL) {
            *nh_ecmp_idx = soc_mem_field32_get(unit, mem, hw_entry, ECMP_PTRf);
        }
    } else {
        /* Mark entry as non-ecmp. */
        lpm_cfg->defip_ecmp = 0;

        /* Reset ecmp group next hop count. */
        lpm_cfg->defip_ecmp_count = 0;

        /* Get next hop index. */
        if (nh_ecmp_idx != NULL) {
            *nh_ecmp_idx =
                soc_mem_field32_get(unit, mem, hw_entry, NEXT_HOP_INDEXf);
        }
    }

    /* Mark entry as IPv6 */
    lpm_cfg->defip_flags |= BCM_L3_IP6;

    /* Get entry priority. */
    lpm_cfg->defip_prio = soc_mem_field32_get(unit, mem, hw_entry, PRIf);

    /* Get classification group id. */
    lpm_cfg->defip_lookup_class = 
        soc_mem_field32_get(unit, mem, hw_entry, CLASS_IDf);

    /* Get hit bit. */
    if (soc_mem_field32_get(unit, mem, hw_entry, HITf)) {
        lpm_cfg->defip_flags |= BCM_L3_HIT;
    }

    /* Get priority override bit. */
    if (soc_mem_field32_get(unit, mem, hw_entry, RPEf)) {
        lpm_cfg->defip_flags |= BCM_L3_RPE;
    }

    /* Get destination discard field. */
    if(soc_mem_field32_get(unit, mem, hw_entry, DST_DISCARDf)) {
        lpm_cfg->defip_flags |= BCM_L3_DST_DISCARD;
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_td2_alpm_warmboot_walk
 * Purpose:
 *      Recover LPM and ALPM entries
 * Parameters:
 *      unit     - (IN)SOC unit number.
 *      trv_data - (IN)pattern + compare,act,notify routines.  
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_alpm_warmboot_walk(int unit, _bcm_l3_trvrs_data_t *trv_data)
{
    int ipv6;                   /* IPv6 flag.                   */
    int idx;                    /* Iteration index.             */
    int tmp_idx;                /* IPv4 entries iterator.       */
    int vrf, vrf_id;            /*                              */
    int idx_end;                /*                              */
    int bkt_addr;               /*                              */
    int bkt_count;              /*                              */
    int bkt_idx = 0;            /*                              */
    int bkt_ptr = 0;            /* Bucket pointer               */
    int bank_num = 0;           /* Number of active SRAM banks  */
    int pivot_idx = 0;          /* Pivot Index                  */
    int entry_num = 0;          /* ALPM entry number in bucket  */
    int nh_ecmp_idx;            /* Next hop/Ecmp group index.   */
    int entry_count;            /* ALPM entry count in bucket   */
    int bank_count;             /* SRAM bank count              */
    int step_count;             /*                              */
    int cmp_result;             /* Test routine result.         */
    int rv = BCM_E_FAIL;        /* Operation return status.     */
    int defip_table_size = 0;   /* Defip table size.            */
    char *lpm_tbl_ptr = NULL;   /* DMA table pointer.           */
    void *alpm_entry = NULL;    /*                              */
    uint32 rval;                /*                              */
    soc_mem_t alpm_mem;         /*                              */
    _bcm_defip_cfg_t lpm_cfg;   /* Buffer to fill route info.   */
    defip_entry_t *lpm_entry;   /* Hw entry buffer.             */
    defip_alpm_ipv4_entry_t    alpm_entry_v4;
    defip_alpm_ipv6_64_entry_t alpm_entry_v6_64;
        
    /* DMA LPM table to software copy */
    BCM_IF_ERROR_RETURN
        (bcm_xgs3_l3_tbl_dma(unit, BCM_XGS3_L3_MEM(unit, defip),
                             BCM_XGS3_L3_ENT_SZ(unit, defip), "lpm_tbl",
                             &lpm_tbl_ptr, &defip_table_size));

    if (SOC_URPF_STATUS_GET(unit)) {
        defip_table_size >>= 1;
    }

    idx_end = defip_table_size;
    
    if(BCM_FAILURE(READ_L3_DEFIP_RPF_CONTROLr(unit, &rval))) {
        goto free_lpm_table;
    }

    if (soc_reg_field_get(unit, L3_DEFIP_RPF_CONTROLr, rval, LOOKUP_MODEf)) {
        /* parallel search mode */
        if (SOC_URPF_STATUS_GET(unit)) {
            bank_count = 2;
        } else {
            bank_count = 4;
        }
    } else {
        bank_count = 4;
    }
    
    /* Walk all lpm entries */
    for (idx = 0; idx < idx_end; idx++) {
        /* Calculate entry ofset */
        lpm_entry =
            soc_mem_table_idx_to_pointer(unit, BCM_XGS3_L3_MEM(unit, defip),
                                         defip_entry_t *, lpm_tbl_ptr, idx);
       
        ipv6 = soc_mem_field32_get(unit, L3_DEFIPm, lpm_entry, MODE0f);

        /* Calculate LPM table traverse step count */
        if (ipv6) {
            if (SOC_ALPM_V6_SCALE_CHECK(unit, ipv6)) {
                step_count = 2;
            } else {
                step_count = 1;
            }
        } else {
            step_count = 2;
        }
      
        /* Insert LPM entry into HASH table and init LPM trackers */ 
        if (BCM_FAILURE(soc_alpm_warmboot_lpm_reinit(unit, ipv6, 
                                                     idx, lpm_entry))) {
            goto free_lpm_table;
        }
        
        /*
         * This loop is used for two purposes
         *  1. IPv4, DEFIP entry has two IPv4 entries.
         *  2. IPv6 double wide mode, walk next bucket.
         */
        for (tmp_idx = 0; tmp_idx < step_count; tmp_idx++) {
            if (tmp_idx) {  /* If index == 1*/
                if (!ipv6) {
                    /* Copy upper half of lpm entry to lower half */
                    soc_alpm_lpm_ip4entry1_to_0(unit, lpm_entry, lpm_entry, TRUE);
               
                    /* Invalidate upper half */
                    soc_L3_DEFIPm_field32_set(unit, lpm_entry, VALID1f, 0);
                }
            }
    
            /* Make sure entry is valid. */
            if (!soc_L3_DEFIPm_field32_get(unit, lpm_entry, VALID0f)) {
                continue;
            }

            if (ipv6 && tmp_idx) {
                /* 
                 * IPv6 double wide mode,  Walk bucket entries at next bucket.
                 */
                bkt_ptr++;
            } else {
                /* Extract bucket pointer from LPM entry */
                bkt_ptr = soc_mem_field32_get(unit, L3_DEFIPm, lpm_entry, 
                                              ALG_BKT_PTR0f);

                /* Extract VRF from LPM entry*/
                if (BCM_FAILURE(soc_alpm_lpm_vrf_get
                                (unit, lpm_entry, &vrf_id, &vrf))) {
                    goto free_lpm_table;
                }

                /* VRF_OVERRIDE (Global High) entries, prefix resides in TCAM */
                if (vrf_id == SOC_L3_VRF_OVERRIDE) {
                    continue;
                }
                    
                pivot_idx = (idx << 1) + tmp_idx;

                /* TCAM pivot recovery */
                if(BCM_FAILURE(soc_alpm_warmboot_pivot_add(unit, ipv6, lpm_entry, pivot_idx,
                                                           bkt_ptr))) {
                    goto free_lpm_table;
                }
            }

            /* Set bucket bitmap */ 
            if(BCM_FAILURE(soc_alpm_warmboot_bucket_bitmap_set(unit, ipv6, bkt_ptr))) {
                goto free_lpm_table;
            }

            if (ipv6) {
                /* IPv6 */
                alpm_mem    = L3_DEFIP_ALPM_IPV6_64m;
                alpm_entry  = &alpm_entry_v6_64;
                bkt_count   = ALPM_IPV6_64_BKT_COUNT;
                entry_count = 4;
            } else {
                /* IPv4 */
                alpm_mem    = L3_DEFIP_ALPM_IPV4m;
                bkt_count   = ALPM_IPV4_BKT_COUNT;
                alpm_entry  = &alpm_entry_v4;
                entry_count = 6;
            }

            if (SOC_URPF_STATUS_GET(unit)) {
                defip_table_size >>= 1;
                if (soc_alpm_mode_get(unit)) {
                    bkt_count >>= 1;
                }
            }

            entry_num = 0;
            bank_num = 0;

            /* Get the bucket pointer from lpm entry */
            for (bkt_idx = 0; bkt_idx < bkt_count; bkt_idx++) {
                /* Calculate bucket memory address */
                /* Increment so next bucket address can be calculated */
                bkt_addr = (entry_num << 16) | (bkt_ptr << 2) | 
                           (bank_num & 0x3);
                entry_num++; 
                if (entry_num == entry_count) {
                    entry_num = 0;
                    bank_num++;
                    if (bank_num == bank_count) {
                        bank_num = 0;
                    }
                }

                /* Read entry from bucket memory */
                if (BCM_FAILURE(soc_mem_read(unit, alpm_mem, MEM_BLOCK_ANY,
                                  bkt_addr, alpm_entry))) {
                    goto free_lpm_table;
                }

                /* Check if ALPM entry is valid */
                if (!soc_mem_field32_get(unit, alpm_mem, alpm_entry, VALIDf)) {
                    continue;
                }

                /* Zero destination buffer first. */
                sal_memset(&lpm_cfg, 0, sizeof(_bcm_defip_cfg_t));

                /* Parse the entry. */
                _bcm_td2_alpm_ent_parse(unit, &lpm_cfg, &nh_ecmp_idx, lpm_entry,
                                        alpm_mem, alpm_entry);
               
                /* Execute operation routine if any. */
                if (trv_data->op_cb) {
                    rv = (*trv_data->op_cb) (unit, (void *)trv_data,
                                            (void *)&lpm_cfg,
                                            (void *)&nh_ecmp_idx, &cmp_result);
#ifdef BCM_CB_ABORT_ON_ERR
                    if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                        continue;
                    }
#endif
                }

                if(BCM_FAILURE(soc_alpm_warmboot_prefix_insert(unit, ipv6, lpm_entry,
                                         alpm_entry, pivot_idx, bkt_ptr, bkt_addr))) {
                    continue;
                }

            } /* End of bucket walk loop*/
        } /* End of lpm entry upper/lower half traversal */
    } /* End of lpm table traversal */

    if(BCM_FAILURE(soc_alpm_warmboot_lpm_reinit_done(unit))) {
        goto free_lpm_table;
    }

#ifdef ALPM_WARM_BOOT_DEBUG
    soc_alpm_lpm_sw_dump(unit);
#endif /* ALPM_WARM_BOOT_DEBUG  */

    rv = BCM_E_NONE;
free_lpm_table:
    soc_cm_sfree(unit, lpm_tbl_ptr);

    return (rv);
}

STATIC int
_bcm_td2_alpm_128_update_match(int unit, _bcm_l3_trvrs_data_t *trv_data)
{
    uint32 ipv6;                /* Iterate over ipv6 only flag. */
    int idx;                    /* Iteration index.             */
    int tmp_idx;                /* ipv4 entries iterator.       */
    char *lpm_tbl_ptr;          /* Dma table pointer.           */
    int nh_ecmp_idx;            /* Next hop/Ecmp group index.   */
    int cmp_result;             /* Test routine result.         */
    defip_pair_128_entry_t *lpm_entry;   /* Hw entry buffer.    */
    _bcm_defip_cfg_t lpm_cfg;   /* Buffer to fill route info.   */
    int defip_table_size;       /* Defip table size.            */
    int rv = BCM_E_NONE;        /* Operation return status.     */
    int idx_start = 0;
    int idx_end = 0, bkt_idx, bkt_count, bkt_ptr = 0, bkt_addr;
    int entry_num, bank_num, entry_count, bank_count;
    uint32 rval;
    defip_alpm_ipv6_128_entry_t alpm_entry_v6_128;
    void *alpm_entry;
    soc_mem_t alpm_mem, pivot_mem = L3_DEFIP_PAIR_128m;
    int step_count;
    int def_arr_idx = 0;
    int def_rte_arr_sz;
    typedef struct _alpm_def_route_info_s {
        int idx;
        int bkt_addr;
    } _alpm_def_route_info_t;
    _alpm_def_route_info_t *def_rte_arr = NULL;

    ipv6 = (trv_data->flags & BCM_L3_IP6);
    if (!ipv6) {
        return SOC_E_NONE;
    }

    /* Allocate memory to store default route meta data*/
    def_rte_arr_sz = SOC_VRF_MAX(unit) * sizeof(_alpm_def_route_info_t);
    def_rte_arr = sal_alloc(def_rte_arr_sz, "alpm_def_rte_arry"); 
    if (NULL == def_rte_arr) {
        return BCM_E_MEMORY;
    }
    sal_memset(def_rte_arr, 0, def_rte_arr_sz);

    /* Table DMA the LPM table to software copy */
    BCM_IF_ERROR_RETURN
        (bcm_xgs3_l3_tbl_dma(unit, L3_DEFIP_PAIR_128m, 
                             soc_mem_entry_words(unit, L3_DEFIP_PAIR_128m), 
                             "lpm_128_tbl", &lpm_tbl_ptr, &defip_table_size));

    alpm_mem = L3_DEFIP_ALPM_IPV6_128m;
    bkt_count = ALPM_IPV6_128_BKT_COUNT;

    if (SOC_URPF_STATUS_GET(unit)) {
        defip_table_size >>= 1;
        /* bucket size halves for parallel search mode */
        if (soc_alpm_mode_get(unit)) {
            bkt_count >>= 1;
        }
    }

    idx_start = 0;
    idx_end = defip_table_size;
    bank_num = 0;
    entry_num = 0;
    SOC_IF_ERROR_RETURN(READ_L3_DEFIP_RPF_CONTROLr(unit, &rval));
    if (soc_reg_field_get(unit, L3_DEFIP_RPF_CONTROLr, rval, LOOKUP_MODEf)) {
        /* parallel search mode */
        if (SOC_URPF_STATUS_GET(unit)) {
            bank_count = 2;
        } else {
            bank_count = 4;
            }
    } else {
        bank_count = 4;
    }

    entry_count = 2;
    alpm_entry = &alpm_entry_v6_128;

    if (SOC_ALPM_V6_SCALE_CHECK(unit, 1)) {
        step_count = 2;
    } else {
        step_count = 1;
    }

    /* if v6 loop twice once with with defip and once with defip_pair */
    for (idx = idx_start; idx < idx_end; idx++) {
        /* Calculate entry ofset. */
        lpm_entry =
            soc_mem_table_idx_to_pointer(unit, L3_DEFIP_PAIR_128m,
                                    defip_pair_128_entry_t *, lpm_tbl_ptr, idx);

        /* Each lpm entry contains upto 24 IPV4 entries. Check all */           
        for (tmp_idx = 0; tmp_idx < step_count; tmp_idx++) {

            /* Make sure entry is valid. */
            if (!soc_mem_field32_get(unit, pivot_mem, lpm_entry, VALID0_LWRf)) {
                continue;
            }

            if (tmp_idx) {
                bkt_ptr++;
            } else {
                bkt_ptr = soc_mem_field32_get(unit, pivot_mem, lpm_entry, 
                                          ALG_BKT_PTRf);
            }
            if (bkt_ptr == 0) {
                /* this could be lpm entry */
                _td2_defip_pair128_get_key(unit, (uint32 *) lpm_entry, &lpm_cfg);

                _bcm_td2_alpm_128_ent_parse(unit, pivot_mem, 
                                            (uint32 *) lpm_entry, 
                                            &lpm_cfg, &nh_ecmp_idx);
                if (trv_data->op_cb) {
                    rv = (*trv_data->op_cb) (unit, (void *)trv_data,
                                            (void *)&lpm_cfg,
                                            (void *)&nh_ecmp_idx, &cmp_result);
#ifdef BCM_CB_ABORT_ON_ERR
                    if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                        break;
                    }
#endif
                }
                continue;
            }
            /* get the bucket pointer from lpm mem  entry */
            for (bkt_idx = 0; bkt_idx < bkt_count; bkt_idx++) {
                /* calculate bucket memory address */
                /* also increment so next bucket address can be calculated */
                bkt_addr = (entry_num << 16) | (bkt_ptr << 2) | 
                           (bank_num & 0x3);
                entry_num++; 
                if (entry_num == entry_count) {
                    entry_num = 0;
                    bank_num++;
                    if (bank_num == bank_count) {
                        bank_num = 0;
                    }
                }

                /* read entry from bucket memory */
                SOC_IF_ERROR_RETURN(soc_mem_read(unit, alpm_mem, MEM_BLOCK_ANY,
                                    bkt_addr, alpm_entry));

                if (!soc_mem_field32_get(unit, alpm_mem, alpm_entry, VALIDf)) {
                    continue;
                }

                /* Zero destination buffer first. */
                sal_memset(&lpm_cfg, 0, sizeof(_bcm_defip_cfg_t));

                /* Parse  the entry. */
                /* Fill entry ip address &  subnet mask. */
                _bcm_td2_alpm_128_get_addr(unit, alpm_mem, 
                                           (uint32 *) alpm_entry, &lpm_cfg);
                
                /* get associated data */
                _bcm_td2_alpm_128_ent_parse(unit, alpm_mem, alpm_entry, 
                                            &lpm_cfg, &nh_ecmp_idx);
                /* If protocol doesn't match skip the entry. */
                if ((lpm_cfg.defip_flags & BCM_L3_IP6) != ipv6) {
                    continue;
                }

                /* Check if this is default route */
                /* Subnet length zero will indicate default route */
                if (0 == lpm_cfg.defip_sub_len) {
                    if (def_arr_idx < SOC_VRF_MAX(unit)) {
                        def_rte_arr[def_arr_idx].bkt_addr = bkt_addr;
                        def_rte_arr[def_arr_idx].idx = idx;
                        def_arr_idx ++;
                    }
                } else {
                    /* Execute operation routine if any. */
                    if (trv_data->op_cb) {
                        rv = (*trv_data->op_cb) (unit, (void *)trv_data,
                                                (void *)&lpm_cfg,
                                                (void *)&nh_ecmp_idx, &cmp_result);
#ifdef BCM_CB_ABORT_ON_ERR
                        if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                            break;
                        }
#endif
                    }
                }
#if 0 /* BCM_WARM_BOOT_SUPPORT*/
                if (SOC_WARM_BOOT(unit) && (!tmp_idx)) {
                    rv = soc_fb_lpm_reinit(unit, idx, lpm_entry);
                    if (rv < 0) {
                        soc_cm_sfree(unit, lpm_tbl_ptr);
                        return rv;
                    }
                }
#endif /* BCM_WARM_BOOT_SUPPORT */
            }
        }
    }
    
    /* Process Default routes */
    for (idx = 0; idx < def_arr_idx; idx++) {

        /* Calculate entry ofset. */
        lpm_entry =
            soc_mem_table_idx_to_pointer(unit, L3_DEFIP_PAIR_128m,
                                        defip_pair_128_entry_t *, lpm_tbl_ptr,  
                                        def_rte_arr[idx].idx);

        bkt_addr = def_rte_arr[idx].bkt_addr;
        
        /* read entry from bucket memory */
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, alpm_mem, MEM_BLOCK_ANY,
                            bkt_addr, alpm_entry));

        if (!soc_mem_field32_get(unit, alpm_mem, alpm_entry, VALIDf)) {
            continue;
        }

        /* Zero destination buffer first. */
        sal_memset(&lpm_cfg, 0, sizeof(_bcm_defip_cfg_t));

        /* Parse  the entry. */
        /* Fill entry ip address & subnet mask. */
        _bcm_td2_alpm_128_get_addr(unit, alpm_mem, alpm_entry, &lpm_cfg);

        _bcm_td2_alpm_128_ent_parse(unit, alpm_mem, alpm_entry, &lpm_cfg, 
                                    &nh_ecmp_idx);

        /* If protocol doesn't match skip the entry. */
        if ((lpm_cfg.defip_flags & BCM_L3_IP6) != ipv6) {
            continue;
        }

        /* Execute operation routine if any. */
        if (trv_data->op_cb) {
            rv = (*trv_data->op_cb) (unit, (void *)trv_data,
                                    (void *)&lpm_cfg,
                                    (void *)&nh_ecmp_idx, &cmp_result);
#ifdef BCM_CB_ABORT_ON_ERR
            if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                break;
            }
#endif
        }
    }

#if 0 /* BCM_WARM_BOOT_SUPPORT */
    if (SOC_WARM_BOOT(unit)) {
        SOC_IF_ERROR_RETURN(soc_fb_lpm_reinit_done(unit));
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    sal_free(def_rte_arr);
    soc_cm_sfree(unit, lpm_tbl_ptr);
    return (rv);
}

/*
 * Function:
 *      _rbcm_td2_alpm_update_match
 * Purpose:
 *      Update/Delete all entries in defip table matching a certain rule.
 * Parameters:
 *      unit     - (IN)SOC unit number.
 *      trv_data - (IN)Delete pattern + compare,act,notify routines.  
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_alpm_update_match(int unit, _bcm_l3_trvrs_data_t *trv_data)
{
    uint32 ipv6 = 0;            /* Iterate over ipv6 only flag. */
    int idx;                    /* Iteration index.             */
    int tmp_idx;                /* ipv4 entries iterator.       */
    char *lpm_tbl_ptr;          /* Dma table pointer.           */
    int nh_ecmp_idx;            /* Next hop/Ecmp group index.   */
    int cmp_result;             /* Test routine result.         */
    defip_entry_t *lpm_entry;   /* Hw entry buffer.             */
    _bcm_defip_cfg_t lpm_cfg;   /* Buffer to fill route info.   */
    int defip_table_size;       /* Defip table size.            */
    int rv = BCM_E_NONE;        /* Operation return status.     */
    int idx_start = 0;
    int idx_end = 0, bkt_idx, bkt_count, bkt_ptr = 0, bkt_addr;
    int entry_num, bank_num, entry_count, bank_count;
    uint32 rval;
    defip_alpm_ipv4_entry_t alpm_entry_v4;
    defip_alpm_ipv4_1_entry_t alpm_entry_v4_1;
    defip_alpm_ipv6_64_entry_t alpm_entry_v6_64;
    defip_alpm_ipv6_64_1_entry_t alpm_entry_v6_64_1;
    defip_alpm_ipv6_128_entry_t alpm_entry_v6_128;
    void *alpm_entry;
    soc_mem_t alpm_mem;
    int step_count;
    int def_arr_idx = 0;
    int def_rte_arr_sz;
    typedef struct _alpm_def_route_info_s {
        int idx;
        int bkt_addr;
    } _alpm_def_route_info_t;
       
    _alpm_def_route_info_t *def_rte_arr = NULL;
#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
         rv =_bcm_td2_alpm_warmboot_walk(unit, trv_data);
         if (rv < 0) {
             soc_cm_print("ERROR!  ALPM Warmboot recovery failed");
         }
         return (rv);
    }
#endif

    /* Allocate memory to store default route meta data*/
    def_rte_arr_sz = SOC_VRF_MAX(unit) * sizeof(_alpm_def_route_info_t);
    def_rte_arr = sal_alloc(def_rte_arr_sz, "alpm_def_rte_arry"); 
    if (NULL == def_rte_arr) {
        return BCM_E_MEMORY;
    }
    sal_memset(def_rte_arr, 0, def_rte_arr_sz);

    if (trv_data->flags & BCM_L3_IP6) {
        ipv6 = 1; /* IPv6 */
    } else {
        ipv6 = 0; /* IPv4 */       
    };

    if (ipv6 && soc_mem_index_count(unit, L3_DEFIP_PAIR_128m) != 0) {
        /* first get v6-128 entries */
        rv = _bcm_td2_alpm_128_update_match(unit, trv_data);
#ifdef BCM_CB_ABORT_ON_ERR
        if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
            return rv;
        }
#endif
    }

    /* Table DMA the LPM table to software copy */
    BCM_IF_ERROR_RETURN
        (bcm_xgs3_l3_tbl_dma(unit, BCM_XGS3_L3_MEM(unit, defip),
                             BCM_XGS3_L3_ENT_SZ(unit, defip), "lpm_tbl",
                             &lpm_tbl_ptr, &defip_table_size));

    if (!ipv6) {
        alpm_mem = L3_DEFIP_ALPM_IPV4m;
        bkt_count = ALPM_IPV4_BKT_COUNT;
    } else {
        alpm_mem = L3_DEFIP_ALPM_IPV6_64m;
        bkt_count = ALPM_IPV6_64_BKT_COUNT;
    }
    if (SOC_URPF_STATUS_GET(unit)) {
        defip_table_size >>= 1;
        if (soc_alpm_mode_get(unit)) {
            bkt_count >>= 1;
        }
    }

    idx_end = defip_table_size;
    bank_num = 0;
    entry_num = 0;
    SOC_IF_ERROR_RETURN(READ_L3_DEFIP_RPF_CONTROLr(unit, &rval));
    if (soc_reg_field_get(unit, L3_DEFIP_RPF_CONTROLr, rval, LOOKUP_MODEf)) {
        /* parallel search mode */
        if (SOC_URPF_STATUS_GET(unit)) {
            bank_count = 2;
        } else {
            bank_count = 4;
            }
    } else {
        bank_count = 4;
    }
    
    switch (alpm_mem) {
    case L3_DEFIP_ALPM_IPV4m:
        entry_count = 6;
        alpm_entry = &alpm_entry_v4;
        break;
    case L3_DEFIP_ALPM_IPV4_1m:
        entry_count = 4;
        alpm_entry = &alpm_entry_v4_1;
        break;
    case L3_DEFIP_ALPM_IPV6_64m:
        entry_count = 4;
        alpm_entry = &alpm_entry_v6_64;
        break;
    case L3_DEFIP_ALPM_IPV6_64_1m:
        entry_count = 3;
        alpm_entry = &alpm_entry_v6_64_1;
        break;
    case L3_DEFIP_ALPM_IPV6_128m:
        entry_count = 2;
        alpm_entry = &alpm_entry_v6_128;
        break;
    default:
        return SOC_E_PARAM;
    }

    if (ipv6) {
        if (SOC_ALPM_V6_SCALE_CHECK(unit, 1)) {
            step_count = 2;
        } else {
            step_count = 1;
        }
    } else {
        step_count = 2;
    }

    /* Walk all lpm entries */
    for (idx = idx_start; idx < idx_end; idx++) {
        /* Calculate entry ofset. */
        lpm_entry =
            soc_mem_table_idx_to_pointer(unit, BCM_XGS3_L3_MEM(unit, defip),
                                         defip_entry_t *, lpm_tbl_ptr, idx);
        
        if (ipv6 != (soc_mem_field32_get(unit, L3_DEFIPm, lpm_entry, MODE0f))) {
           /*
            * Function called for IPv6 LPM/ALPM walk and LPM entry is IPv4; Continue;
            */
           continue;
        }
        
        /* Each LPM index has two IPv4 entries*/
        for (tmp_idx = 0; tmp_idx < step_count; tmp_idx++) {
            if (tmp_idx) {
                if (!ipv6) {
                    /* Check second part of the entry. */
                    soc_alpm_lpm_ip4entry1_to_0(unit, lpm_entry, lpm_entry, TRUE);
                }
            }

            /* Make sure entry is valid. */
            if (!soc_L3_DEFIPm_field32_get(unit, lpm_entry, VALID0f)) {
                continue;
            }

            soc_alpm_lpm_vrf_get(unit, lpm_entry, &lpm_cfg.defip_vrf,
                                &cmp_result);
            if (lpm_cfg.defip_vrf == BCM_L3_VRF_OVERRIDE) {
                sal_memset(&lpm_cfg, 0, sizeof(_bcm_defip_cfg_t));
                _bcm_td2_lpm_ent_parse(unit, &lpm_cfg, &nh_ecmp_idx, lpm_entry);
                _bcm_td2_lpm_ent_get_key(unit, &lpm_cfg, lpm_entry);
                if (ipv6 && (lpm_cfg.defip_flags & BCM_L3_IP6)) {
                    if (trv_data->op_cb) {
                        rv = (*trv_data->op_cb) (unit, (void *)trv_data,
                                            (void *)&lpm_cfg,
                                            (void *)&nh_ecmp_idx, &cmp_result);
#ifdef BCM_CB_ABORT_ON_ERR
                        if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                            break;
                        }
#endif
                    }
                }
            }

            if (ipv6 && tmp_idx) {
                bkt_ptr++;
            } else {
                bkt_ptr = soc_mem_field32_get(unit, L3_DEFIPm, lpm_entry, 
                                          ALG_BKT_PTR0f);
            }
            
            /* Get the bucket pointer from lpm mem entry */
            for (bkt_idx = 0; bkt_idx < bkt_count; bkt_idx++) {
                /* Calculate bucket memory address */
                /* Increment so next bucket address can be calculated */
                bkt_addr = (entry_num << 16) | (bkt_ptr << 2) | 
                           (bank_num & 0x3);
                entry_num++; 
                if (entry_num == entry_count) {
                    entry_num = 0;
                    bank_num++;
                    if (bank_num == bank_count) {
                        bank_num = 0;
                    }
                }

                /* Read entry from bucket memory */
                SOC_IF_ERROR_RETURN(soc_mem_read(unit, alpm_mem, MEM_BLOCK_ANY,
                                    bkt_addr, alpm_entry));

                if (!soc_mem_field32_get(unit, alpm_mem, alpm_entry, VALIDf)) {
                    continue;
                }

                /* Zero destination buffer first. */
                sal_memset(&lpm_cfg, 0, sizeof(_bcm_defip_cfg_t));

                /* Parse the entry. */
                _bcm_td2_alpm_ent_parse(unit, &lpm_cfg, &nh_ecmp_idx, lpm_entry,
                                        alpm_mem, alpm_entry);
        
                /* If protocol doesn't match skip the entry. */
                if ((lpm_cfg.defip_flags & BCM_L3_IP6) &&  ipv6) {
                    continue;
                }

                /* Fill entry IP address and subnet mask. */
                _bcm_td2_alpm_ent_get_key(unit, &lpm_cfg, lpm_entry, 
                                          alpm_mem, alpm_entry);

                /* Check if this is default route */
                /* Subnet length zero will indicate default route */
                if (0 == lpm_cfg.defip_sub_len) {
                    if (def_arr_idx < SOC_VRF_MAX(unit)) {
                        def_rte_arr[def_arr_idx].bkt_addr = bkt_addr;
                        def_rte_arr[def_arr_idx].idx = idx;
                        def_arr_idx ++;
                    }
                } else {
                    /* Execute operation routine if any. */
                    if (trv_data->op_cb) {
                        rv = (*trv_data->op_cb) (unit, (void *)trv_data,
                                                (void *)&lpm_cfg,
                                                (void *)&nh_ecmp_idx, &cmp_result);
#ifdef BCM_CB_ABORT_ON_ERR
                        if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                            break;
                        }
#endif
                    }
                }
            }
        }
    }

    /* Process Default routes */
    for (idx = 0; idx < def_arr_idx; idx++) {

        /* Calculate entry ofset. */
        lpm_entry =
            soc_mem_table_idx_to_pointer(unit, BCM_XGS3_L3_MEM(unit, defip),
                                         defip_entry_t *, lpm_tbl_ptr,  
                                         def_rte_arr[idx].idx);

        if (ipv6 != soc_mem_field32_get(unit, L3_DEFIPm, lpm_entry, MODE0f)) {
           /*
            * Function called for IPv6 LPM/ALPM walk and LPM entry is IPv4; Continue;
            */
           continue;
        }
       
        bkt_addr = def_rte_arr[idx].bkt_addr;
        
        /* read entry from bucket memory */
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, alpm_mem, MEM_BLOCK_ANY,
                            bkt_addr, alpm_entry));

        if (!soc_mem_field32_get(unit, alpm_mem, alpm_entry, VALIDf)) {
            continue;
        }

        /* Zero destination buffer first. */
        sal_memset(&lpm_cfg, 0, sizeof(_bcm_defip_cfg_t));

        /* Parse  the entry. */
        _bcm_td2_alpm_ent_parse(unit, &lpm_cfg, &nh_ecmp_idx, lpm_entry,
                                alpm_mem, alpm_entry);

        /* If protocol doesn't match skip the entry. */
        if ((lpm_cfg.defip_flags & BCM_L3_IP6) != ipv6) {
            continue;
        }

        /* Fill entry ip address & subnet mask. */
        _bcm_td2_alpm_ent_get_key(unit, &lpm_cfg, lpm_entry, 
                                  alpm_mem, alpm_entry);

        /* Execute operation routine if any. */
        if (trv_data->op_cb) {
            rv = (*trv_data->op_cb) (unit, (void *)trv_data,
                                    (void *)&lpm_cfg,
                                    (void *)&nh_ecmp_idx, &cmp_result);
#ifdef BCM_CB_ABORT_ON_ERR
            if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                break;
            }
#endif
        }
    }

#ifdef ALPM_WARM_BOOT_DEBUG
    soc_alpm_lpm_sw_dump(unit);
#endif /* ALPM_WARM_BOOT_DEBUG  */
    sal_free(def_rte_arr);
    soc_cm_sfree(unit, lpm_tbl_ptr);

    return (rv);
}
#endif /* ALPM_ENABLE */

/*
 * Function:
 *      bcm_td2_ecmp_rh_deinit
 * Purpose:
 *      Deallocate ECMP resilient hashing internal data structures
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_xxx
 */
void
bcm_td2_ecmp_rh_deinit(int unit)
{
    if (_td2_ecmp_rh_info[unit]) {
        if (_td2_ecmp_rh_info[unit]->ecmp_rh_flowset_block_bitmap) {
            sal_free(_td2_ecmp_rh_info[unit]->ecmp_rh_flowset_block_bitmap);
            _td2_ecmp_rh_info[unit]->ecmp_rh_flowset_block_bitmap = NULL;
        }
        sal_free(_td2_ecmp_rh_info[unit]);
        _td2_ecmp_rh_info[unit] = NULL;
    }
}

/*
 * Function:
 *      bcm_td2_ecmp_rh_init
 * Purpose:
 *      Initialize ECMP resilient hashing internal data structures
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_xxx
 */
int
bcm_td2_ecmp_rh_init(int unit)
{
    int num_ecmp_rh_flowset_entries;

    bcm_td2_ecmp_rh_deinit(unit);

    _td2_ecmp_rh_info[unit] = sal_alloc(sizeof(_td2_ecmp_rh_info_t),
            "_td2_ecmp_rh_info");
    if (_td2_ecmp_rh_info[unit] == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(_td2_ecmp_rh_info[unit], 0, sizeof(_td2_ecmp_rh_info_t));

    /* The LAG and ECMP resilient hash features share the same flow set table.
     * The table can be configured in one of 3 modes:
     * - dedicated to LAG resilient hashing,
     * - dedicated to ECMP resilient hashing,
     * - split evenly between LAG and ECMP resilient hashing.
     * The mode was configured during Trident2 initialization.
     */
    num_ecmp_rh_flowset_entries = soc_mem_index_count(unit, RH_ECMP_FLOWSETm);

    /* Each bit in ecmp_rh_flowset_block_bitmap corresponds to a block of 64
     * resilient hashing flow set table entries.
     */
    _td2_ecmp_rh_info[unit]->num_ecmp_rh_flowset_blocks =
        num_ecmp_rh_flowset_entries / 64;
    if (_td2_ecmp_rh_info[unit]->num_ecmp_rh_flowset_blocks > 0) {
        _td2_ecmp_rh_info[unit]->ecmp_rh_flowset_block_bitmap = 
            sal_alloc(SHR_BITALLOCSIZE(_td2_ecmp_rh_info[unit]->
                        num_ecmp_rh_flowset_blocks),
                    "ecmp_rh_flowset_block_bitmap");
        if (_td2_ecmp_rh_info[unit]->ecmp_rh_flowset_block_bitmap == NULL) {
            bcm_td2_ecmp_rh_deinit(unit);
            return BCM_E_MEMORY;
        }
        sal_memset(_td2_ecmp_rh_info[unit]->ecmp_rh_flowset_block_bitmap, 0,
                SHR_BITALLOCSIZE(_td2_ecmp_rh_info[unit]->
                    num_ecmp_rh_flowset_blocks));
    }

    /* Set the seed for the pseudo-random number generator */
    _td2_ecmp_rh_info[unit]->ecmp_rh_rand_seed = sal_time_usecs();

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_ecmp_rh_dynamic_size_encode
 * Purpose:
 *      Encode ECMP resilient hashing flow set size.
 * Parameters:
 *      dynamic_size - (IN) Number of flow sets.
 *      encoded_value - (OUT) Encoded value.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_ecmp_rh_dynamic_size_encode(int dynamic_size, int *encoded_value)
{   
    switch (dynamic_size) {
        case 64:
            *encoded_value = 1;
            break;
        case 128:
            *encoded_value = 2;
            break;
        case 256:
            *encoded_value = 3;
            break;
        case 512:
            *encoded_value = 4;
            break;
        case 1024:
            *encoded_value = 5;
            break;
        case 2048:
            *encoded_value = 6;
            break;
        case 4096:
            *encoded_value = 7;
            break;
        case 8192:
            *encoded_value = 8;
            break;
        case 16384:
            *encoded_value = 9;
            break;
        case 32768:
            *encoded_value = 10;
            break;
        case 65536:
            *encoded_value = 11;
            break;
        default:
            return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_ecmp_rh_dynamic_size_decode
 * Purpose:
 *      Decode ECMP resilient hashing flow set size.
 * Parameters:
 *      encoded_value - (IN) Encoded value.
 *      dynamic_size - (OUT) Number of flow sets.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_ecmp_rh_dynamic_size_decode(int encoded_value, int *dynamic_size)
{   
    switch (encoded_value) {
        case 1:
            *dynamic_size = 64;
            break;
        case 2:
            *dynamic_size = 128;
            break;
        case 3:
            *dynamic_size = 256;
            break;
        case 4:
            *dynamic_size = 512;
            break;
        case 5:
            *dynamic_size = 1024;
            break;
        case 6:
            *dynamic_size = 2048;
            break;
        case 7:
            *dynamic_size = 4096;
            break;
        case 8:
            *dynamic_size = 8192;
            break;
        case 9:
            *dynamic_size = 16384;
            break;
        case 10:
            *dynamic_size = 32768;
            break;
        case 11:
            *dynamic_size = 65536;
            break;
        default:
            return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_ecmp_rh_free_resource
 * Purpose:
 *      Free resources for an ECMP resilient hashing group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      ecmp_group - (IN) ECMP group ID.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_ecmp_rh_free_resource(int unit, int ecmp_group)
{
    int rv = BCM_E_NONE;
    ecmp_count_entry_t ecmp_count_entry;
    int flow_set_size;
    int entry_base_ptr;
    int num_entries;
    int block_base_ptr;
    int num_blocks;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int index_min, index_max;

    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY,
                ecmp_group, &ecmp_count_entry));
    if (0 == soc_L3_ECMP_COUNTm_field32_get(unit,
            &ecmp_count_entry, ENHANCED_HASHING_ENABLEf)) {
        /* Resilient hashing is not enabled on this ECMP group. */
        return BCM_E_NONE;
    }
    flow_set_size = soc_L3_ECMP_COUNTm_field32_get(unit,
            &ecmp_count_entry, RH_FLOW_SET_SIZEf);
    entry_base_ptr = soc_L3_ECMP_COUNTm_field32_get(unit,
            &ecmp_count_entry, RH_FLOW_SET_BASEf);

    /* Clear resilient hashing fields */
    soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_count_entry,
            ENHANCED_HASHING_ENABLEf, 0);
    soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_count_entry,
            RH_FLOW_SET_BASEf, 0);
    soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_count_entry,
            RH_FLOW_SET_SIZEf, 0);
    SOC_IF_ERROR_RETURN(WRITE_L3_ECMP_COUNTm(unit, MEM_BLOCK_ALL,
                ecmp_group, &ecmp_count_entry));

    /* Clear flow set table entries */
    BCM_IF_ERROR_RETURN
        (_bcm_td2_ecmp_rh_dynamic_size_decode(flow_set_size, &num_entries));
    alloc_size = num_entries * sizeof(rh_ecmp_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_ECMP_FLOWSET entries");
    if (NULL == buf_ptr) {
        return BCM_E_MEMORY;
    }
    sal_memset(buf_ptr, 0, alloc_size);
    index_min = entry_base_ptr;
    index_max = index_min + num_entries - 1;
    rv = soc_mem_write_range(unit, RH_ECMP_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        soc_cm_sfree(unit, buf_ptr);
        return rv;
    }
    soc_cm_sfree(unit, buf_ptr);

    /* Free flow set table resources */
    block_base_ptr = entry_base_ptr >> 6;
    num_blocks = num_entries >> 6; 
    _BCM_ECMP_RH_FLOWSET_BLOCK_USED_CLR_RANGE(unit, block_base_ptr, num_blocks);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_ecmp_rh_dynamic_size_set
 * Purpose:
 *      Set the dynamic size for an ECMP resilient hashing group with
 *      no members.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      ecmp_group - (IN) ECMP group ID.
 *      dynamic_size - (IN) Number of flow sets.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_ecmp_rh_dynamic_size_set(int unit, int ecmp_group, int dynamic_size)
{
    ecmp_count_entry_t ecmp_count_entry;
    int flow_set_size;

    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, ecmp_group,
                &ecmp_count_entry));
    if (soc_L3_ECMP_COUNTm_field32_get(unit, &ecmp_count_entry,
                ENHANCED_HASHING_ENABLEf)) {
        /* Not expecting resilient hashing to be enabled */
        return BCM_E_INTERNAL;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_td2_ecmp_rh_dynamic_size_encode(dynamic_size, &flow_set_size));
    soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_count_entry, RH_FLOW_SET_SIZEf,
            flow_set_size);
    SOC_IF_ERROR_RETURN(WRITE_L3_ECMP_COUNTm(unit, MEM_BLOCK_ALL, ecmp_group,
                &ecmp_count_entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_ecmp_rh_rand_get
 * Purpose:
 *      Get a random number between 0 and the given max value.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      rand_max - (IN) Maximum random number.
 *      rand_num - (OUT) Random number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 *      This procedure uses the pseudo-random number generator algorithm
 *      suggested by the C standard.
 */
STATIC int
_bcm_td2_ecmp_rh_rand_get(int unit, int rand_max, int *rand_num)
{
    int modulus;
    int rand_seed_shift;

    if (rand_max < 0) {
        return BCM_E_PARAM;
    }

    if (NULL == rand_num) {
        return BCM_E_PARAM;
    }

    /* Make sure the modulus does not exceed limit. For instance,
     * if the 32-bit rand_seed is shifted to the right by 16 bits before
     * the modulo operation, the modulus should not exceed 1 << 16.
     */
    modulus = rand_max + 1;
    rand_seed_shift = 16;
    if (modulus > (1 << (32 - rand_seed_shift))) {
        return BCM_E_PARAM;
    }

    _td2_ecmp_rh_info[unit]->ecmp_rh_rand_seed =
        _td2_ecmp_rh_info[unit]->ecmp_rh_rand_seed * 1103515245 + 12345;

    *rand_num = (_td2_ecmp_rh_info[unit]->ecmp_rh_rand_seed >> rand_seed_shift) %
                modulus;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_ecmp_rh_member_choose
 * Purpose:
 *      Choose a member of the ECMP group.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      num_members - (IN) Number of members to choose from.
 *      entry_count_arr   - (IN/OUT) An array keeping track of how many
 *                                   flow set entries have been assigned
 *                                   to each member.
 *      max_entry_count   - (IN/OUT) The maximum number of flow set entries
 *                                   that can be assigned to a member. 
 *      chosen_index      - (OUT) The index of the chosen member.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_ecmp_rh_member_choose(int unit, int num_members, int *entry_count_arr,
        int *max_entry_count, int *chosen_index)
{
    int member_index;
    int next_index;

    *chosen_index = 0;

    /* Choose a random member index */
    BCM_IF_ERROR_RETURN(_bcm_td2_ecmp_rh_rand_get(unit, num_members - 1,
                &member_index));

    if (entry_count_arr[member_index] < *max_entry_count) {
        entry_count_arr[member_index]++;
        *chosen_index = member_index;
    } else {
        /* The randomly chosen member has reached the maximum
         * flow set entry count. Choose the next member that
         * has not reached the maximum entry count.
         */
        next_index = (member_index + 1) % num_members;
        while (next_index != member_index) {
            if (entry_count_arr[next_index] < *max_entry_count) {
                entry_count_arr[next_index]++;
                *chosen_index = next_index;
                break;
            } else {
                next_index = (next_index + 1) % num_members;
            }
        }
        if (next_index == member_index) {
            /* All members have reached the maximum flow set entry
             * count. This scenario occurs when dividing the number of
             * flow set entries by the number of members results
             * in a non-zero remainder. The remainder flow set entries
             * will be distributed among members, at most 1 remainder
             * entry per member.
             */
            (*max_entry_count)++;
            entry_count_arr[member_index]++;
            *chosen_index = member_index;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_ecmp_rh_set
 * Purpose:
 *      Configure an ECMP resilient hashing group.
 * Parameters:
 *      unit       - (IN) SOC unit number. 
 *      ecmp       - (IN) ECMP group info.
 *      intf_count - (IN) Number of elements in intf_array.
 *      intf_array - (IN) Array of Egress forwarding objects.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_ecmp_rh_set(int unit,
        bcm_l3_egress_ecmp_t *ecmp,
        int intf_count,
        bcm_if_t *intf_array)
{
    int rv = BCM_E_NONE;
    int ecmp_group;
    int num_blocks;
    int total_blocks;
    int max_block_base_ptr;
    int block_base_ptr;
    int entry_base_ptr;
    SHR_BITDCL occupied;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int *entry_count_arr = NULL;
    int max_entry_count;
    int chosen_index;
    int i;
    rh_ecmp_flowset_entry_t *flowset_entry;
    int offset;
    int next_hop_index;
    int index_min, index_max;
    ecmp_count_entry_t ecmp_count_entry;
    int flow_set_size;

    if (ecmp == NULL ||
            ecmp->dynamic_mode != BCM_L3_ECMP_DYNAMIC_MODE_RESILIENT) {
        return BCM_E_PARAM;
    }

    if (BCM_XGS3_L3_MPATH_EGRESS_IDX_VALID(unit, ecmp->ecmp_intf)) {
        ecmp_group = ecmp->ecmp_intf - BCM_XGS3_MPATH_EGRESS_IDX_MIN;
    } else {
        return BCM_E_PARAM;
    }

    if (intf_count > 0 && intf_array == NULL) {
        return BCM_E_PARAM;
    }

    if (intf_count == 0) {
        /* Store the dynamic_size in hardware */
        BCM_IF_ERROR_RETURN(_bcm_td2_ecmp_rh_dynamic_size_set(unit,
                    ecmp_group, ecmp->dynamic_size));
        return BCM_E_NONE;
    }

    /* Find a contiguous region of flow set table that's large
     * enough to hold the requested number of flow sets.
     * The flow set table is allocated in 64-entry blocks.
     */
    num_blocks = ecmp->dynamic_size >> 6;
    total_blocks = _td2_ecmp_rh_info[unit]->num_ecmp_rh_flowset_blocks;
    if (num_blocks > total_blocks) {
        return BCM_E_RESOURCE;
    }
    max_block_base_ptr = total_blocks - num_blocks;
    for (block_base_ptr = 0;
         block_base_ptr <= max_block_base_ptr;
         block_base_ptr++) {
        /* Check if the contiguous region of flow set table from
         * block_base_ptr to (block_base_ptr + num_blocks - 1) is free. 
         */
        _BCM_ECMP_RH_FLOWSET_BLOCK_TEST_RANGE(unit, block_base_ptr, num_blocks,
                occupied); 
        if (!occupied) {
            break;
        }
    }
    if (block_base_ptr > max_block_base_ptr) {
        /* A contiguous region of the desired size could not be found in
         * flow set table.
         */
        return BCM_E_RESOURCE;
    }

    /* Configure flow set table */
    alloc_size = ecmp->dynamic_size * sizeof(rh_ecmp_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_ECMP_FLOWSET entries");
    if (NULL == buf_ptr) {
        return BCM_E_MEMORY;
    }
    sal_memset(buf_ptr, 0, alloc_size);

    entry_count_arr = sal_alloc(sizeof(int) * intf_count,
            "ECMP RH entry count array");
    if (NULL == entry_count_arr) {
        soc_cm_sfree(unit, buf_ptr);
        return BCM_E_MEMORY;
    }
    sal_memset(entry_count_arr, 0, sizeof(int) * intf_count);
    max_entry_count = ecmp->dynamic_size / intf_count;

    for (i = 0; i < ecmp->dynamic_size; i++) {
        /* Choose a member of the ECMP */
        rv = _bcm_td2_ecmp_rh_member_choose(unit, intf_count,
                entry_count_arr, &max_entry_count, &chosen_index);
        if (BCM_FAILURE(rv)) {
            soc_cm_sfree(unit, buf_ptr);
            sal_free(entry_count_arr);
            return rv;
        }

        /* Set flow set entry */
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_ECMP_FLOWSETm, rh_ecmp_flowset_entry_t *, buf_ptr, i);
        soc_mem_field32_set(unit, RH_ECMP_FLOWSETm, flowset_entry, VALIDf, 1);
        if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit,
                    intf_array[chosen_index])) {
            offset = BCM_XGS3_EGRESS_IDX_MIN;
        } else if (BCM_XGS3_DVP_EGRESS_IDX_VALID(unit,
                    intf_array[chosen_index])) {
            offset = BCM_XGS3_DVP_EGRESS_IDX_MIN;
        } else {
            soc_cm_sfree(unit, buf_ptr);
            sal_free(entry_count_arr);
            return BCM_E_PARAM;
        }
        next_hop_index = intf_array[chosen_index] - offset;
        soc_mem_field32_set(unit, RH_ECMP_FLOWSETm, flowset_entry,
                NEXT_HOP_INDEXf, next_hop_index);
    }

    entry_base_ptr = block_base_ptr << 6;
    index_min = entry_base_ptr;
    index_max = index_min + ecmp->dynamic_size - 1;
    rv = soc_mem_write_range(unit, RH_ECMP_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        soc_cm_sfree(unit, buf_ptr);
        sal_free(entry_count_arr);
        return rv;
    }
    soc_cm_sfree(unit, buf_ptr);
    sal_free(entry_count_arr);

    _BCM_ECMP_RH_FLOWSET_BLOCK_USED_SET_RANGE(unit, block_base_ptr, num_blocks);

    /* Update ECMP group table */
    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, ecmp_group,
                &ecmp_count_entry));
    soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_count_entry,
            ENHANCED_HASHING_ENABLEf, 1);
    soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_count_entry,
            RH_FLOW_SET_BASEf, entry_base_ptr);
    BCM_IF_ERROR_RETURN(_bcm_td2_ecmp_rh_dynamic_size_encode(
                ecmp->dynamic_size, &flow_set_size));
    soc_L3_ECMP_COUNTm_field32_set(unit, &ecmp_count_entry,
            RH_FLOW_SET_SIZEf, flow_set_size);
    SOC_IF_ERROR_RETURN(WRITE_L3_ECMP_COUNTm(unit, MEM_BLOCK_ALL, ecmp_group,
                &ecmp_count_entry));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_ecmp_rh_add
 * Purpose:
 *      Add a member to an ECMP resilient hashing group.
 * Parameters:
 *      unit       - (IN) SOC unit number. 
 *      ecmp       - (IN) ECMP group info.
 *      intf_count - (IN) Number of elements in intf_array.
 *      intf_array - (IN) Array of resilient hashing eligible members,
 *                        including the member to be added.
 *      new_intf   - (IN) New member to be added.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_td2_ecmp_rh_add(int unit,
        bcm_l3_egress_ecmp_t *ecmp,
        int intf_count,
        bcm_if_t *intf_array,
        bcm_if_t new_intf)
{
    int rv = BCM_E_NONE;
    int ecmp_group;
    int offset;
    int new_next_hop_index;
    int next_hop_index;
    int num_existing_members;
    ecmp_count_entry_t ecmp_count_entry;
    int i, j;
    int entry_base_ptr;
    int flow_set_size;
    int num_entries;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int index_min, index_max;
    int *entry_count_arr = NULL;
    rh_ecmp_flowset_entry_t *flowset_entry;
    int member_index = 0;
    int lower_bound, upper_bound;
    int threshold;
    int new_member_entry_count;
    int entry_index, next_entry_index;
    int is_new_member;

    if (ecmp == NULL ||
            ecmp->dynamic_mode != BCM_L3_ECMP_DYNAMIC_MODE_RESILIENT) {
        return BCM_E_PARAM;
    }

    if (BCM_XGS3_L3_MPATH_EGRESS_IDX_VALID(unit, ecmp->ecmp_intf)) {
        ecmp_group = ecmp->ecmp_intf - BCM_XGS3_MPATH_EGRESS_IDX_MIN;
    } else {
        return BCM_E_PARAM;
    }

    if (intf_count == 0 || intf_array == NULL) {
        return BCM_E_PARAM;
    }

    if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, new_intf)) {
        offset = BCM_XGS3_EGRESS_IDX_MIN;
    } else if (BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, new_intf)) {
        offset = BCM_XGS3_DVP_EGRESS_IDX_MIN;
    } else {
        return BCM_E_PARAM;
    }
    new_next_hop_index = new_intf - offset;

    /* Check that the new member is the last element of the array of
     * resilient hashing eligible members.
     */
    if (new_intf != intf_array[intf_count - 1]) {
        return BCM_E_PARAM;
    }
    num_existing_members = intf_count - 1;

    if (intf_count == 1) {
        /* Adding the first member is the same as setting one member */
        return _bcm_td2_ecmp_rh_set(unit, ecmp, intf_count, intf_array);
    }

    /* Read all the flow set entries of this ECMP resilient hash group,
     * and compute the number of entries currently assigned to each
     * existing member.
     */
    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, ecmp_group,
                &ecmp_count_entry));
    entry_base_ptr = soc_L3_ECMP_COUNTm_field32_get(unit, &ecmp_count_entry,
            RH_FLOW_SET_BASEf);
    flow_set_size = soc_L3_ECMP_COUNTm_field32_get(unit, &ecmp_count_entry,
            RH_FLOW_SET_SIZEf);
    BCM_IF_ERROR_RETURN
        (_bcm_td2_ecmp_rh_dynamic_size_decode(flow_set_size, &num_entries));

    alloc_size = num_entries * sizeof(rh_ecmp_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_ECMP_FLOWSET entries");
    if (NULL == buf_ptr) {
        return BCM_E_MEMORY;
    }
    sal_memset(buf_ptr, 0, alloc_size);
    index_min = entry_base_ptr;
    index_max = index_min + num_entries - 1;
    rv = soc_mem_read_range(unit, RH_ECMP_FLOWSETm, MEM_BLOCK_ANY,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

    alloc_size = num_existing_members * sizeof(int);
    entry_count_arr = sal_alloc(alloc_size, "ECMP RH entry count array");
    if (NULL == entry_count_arr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(entry_count_arr, 0, alloc_size);

    for (i = 0; i < num_entries; i++) {
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_ECMP_FLOWSETm, rh_ecmp_flowset_entry_t *, buf_ptr, i);
        if (!soc_mem_field32_get(unit, RH_ECMP_FLOWSETm, flowset_entry,
                    VALIDf)) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
        next_hop_index = soc_mem_field32_get(unit, RH_ECMP_FLOWSETm,
                flowset_entry, NEXT_HOP_INDEXf);
        for (j = 0; j < num_existing_members; j++) {
            if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, intf_array[j])) {
                offset = BCM_XGS3_EGRESS_IDX_MIN;
            } else if (BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, intf_array[j])) {
                offset = BCM_XGS3_DVP_EGRESS_IDX_MIN;
            } else {
                rv = BCM_E_PARAM;
                goto cleanup;
            }
            if (next_hop_index == (intf_array[j] - offset)) {
                break;
            }
        }
        if (j == num_existing_members) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
        entry_count_arr[j]++;
    }

    /* Check that the distribution of flow set entries among existing members
     * is balanced. For instance, if the number of flow set entries is 64, and
     * the number of existing members is 6, then every member should have
     * between 10 and 11 entries. 
     */ 
    lower_bound = num_entries / num_existing_members;
    upper_bound = (num_entries % num_existing_members) ?
                  (lower_bound + 1) : lower_bound;
    for (i = 0; i < num_existing_members; i++) {
        if (entry_count_arr[i] < lower_bound ||
                entry_count_arr[i] > upper_bound) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
    }

    /* Re-balance flow set entries from existing members to the new member.
     * For example, if the number of flow set entries is 64, and the number of
     * existing members is 6, then each existing member has between 10 and 11
     * entries. The entries should be re-assigned from the 6 existing members
     * to the new member such that each member will end up with between 9 and
     * 10 entries.
     */
    lower_bound = num_entries / intf_count;
    upper_bound = (num_entries % intf_count) ?
                  (lower_bound + 1) : lower_bound;
    threshold = upper_bound;
    new_member_entry_count = 0;
    while (new_member_entry_count < lower_bound) {
        /* Pick a random entry in the flow set buffer */
        rv = _bcm_td2_ecmp_rh_rand_get(unit, num_entries - 1, &entry_index);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        flowset_entry = soc_mem_table_idx_to_pointer(unit, RH_ECMP_FLOWSETm,
                rh_ecmp_flowset_entry_t *, buf_ptr, entry_index);
        next_hop_index = soc_mem_field32_get(unit, RH_ECMP_FLOWSETm,
                flowset_entry, NEXT_HOP_INDEXf);

        /* Determine if the randomly picked entry contains the new member.
         * If not, determine the existing member index.
         */
        if (next_hop_index == new_next_hop_index) {
            is_new_member = TRUE;
        } else {
            is_new_member = FALSE;
        }
        if (!is_new_member) {
            for (i = 0; i < num_existing_members; i++) {
                if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, intf_array[i])) {
                    offset = BCM_XGS3_EGRESS_IDX_MIN;
                } else if (BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, intf_array[i])) {
                    offset = BCM_XGS3_DVP_EGRESS_IDX_MIN;
                } else {
                    rv = BCM_E_PARAM;
                    goto cleanup;
                }
                if (next_hop_index == (intf_array[i] - offset)) {
                    break;
                }
            }
            if (i == num_existing_members) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            member_index = i;
        }

        if (!is_new_member && (entry_count_arr[member_index] > threshold)) {
            soc_mem_field32_set(unit, RH_ECMP_FLOWSETm, flowset_entry,
                    NEXT_HOP_INDEXf, new_next_hop_index);
            entry_count_arr[member_index]--;
            new_member_entry_count++;
        } else {
            /* Either the member of the randomly chosen entry is
             * the same as the new member, or the member is an existing
             * member and its entry count has decreased to threshold.
             * In both cases, find the next entry that contains a
             * member that's not the new member and whose entry count
             * has not decreased to threshold.
             */
            next_entry_index = (entry_index + 1) % num_entries;
            while (next_entry_index != entry_index) {
                flowset_entry = soc_mem_table_idx_to_pointer(unit,
                        RH_ECMP_FLOWSETm, rh_ecmp_flowset_entry_t *, buf_ptr,
                        next_entry_index);
                next_hop_index = soc_mem_field32_get(unit, RH_ECMP_FLOWSETm,
                        flowset_entry, NEXT_HOP_INDEXf);

                /* Determine if the entry contains the new member.
                 * If not, determine the existing member index.
                 */
                if (next_hop_index == new_next_hop_index) {
                    is_new_member = TRUE;
                } else {
                    is_new_member = FALSE;
                }
                if (!is_new_member) {
                    for (i = 0; i < num_existing_members; i++) {
                        if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, intf_array[i])) {
                            offset = BCM_XGS3_EGRESS_IDX_MIN;
                        } else if (BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, intf_array[i])) {
                            offset = BCM_XGS3_DVP_EGRESS_IDX_MIN;
                        } else {
                            rv = BCM_E_PARAM;
                            goto cleanup;
                        }
                        if (next_hop_index == (intf_array[i] - offset)) {
                            break;
                        }
                    }
                    if (i == num_existing_members) {
                        rv = BCM_E_INTERNAL;
                        goto cleanup;
                    }
                    member_index = i;
                }

                if (!is_new_member &&
                        (entry_count_arr[member_index] > threshold)) {
                    soc_mem_field32_set(unit, RH_ECMP_FLOWSETm, flowset_entry,
                            NEXT_HOP_INDEXf, new_next_hop_index);
                    entry_count_arr[member_index]--;
                    new_member_entry_count++;
                    break;
                } else {
                    next_entry_index = (next_entry_index + 1) % num_entries;
                }
            }
            if (next_entry_index == entry_index) {
                /* The entry count of all existing members has decreased
                 * to threshold. The entry count of the new member has
                 * not yet increased to lower_bound. Lower the threshold.
                 */
                threshold--;
            }
        }
    }

    /* Write re-balanced flow set entries to hardware */
    rv = soc_mem_write_range(unit, RH_ECMP_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

cleanup:
    if (buf_ptr) {
        soc_cm_sfree(unit, buf_ptr);
    }
    if (entry_count_arr) {
        sal_free(entry_count_arr);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_td2_ecmp_rh_delete
 * Purpose:
 *      Delete a member from an ECMP resilient hashing group.
 * Parameters:
 *      unit         - (IN) SOC unit number. 
 *      ecmp         - (IN) ECMP group info.
 *      intf_count   - (IN) Number of elements in intf_array.
 *      intf_array   - (IN) Array of resilient hashing eligible members,
 *                        except the member to be deleted.
 *      leaving_intf - (IN) Member to delete.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_td2_ecmp_rh_delete(int unit,
        bcm_l3_egress_ecmp_t *ecmp,
        int intf_count,
        bcm_if_t *intf_array,
        bcm_if_t leaving_intf)
{
    int rv = BCM_E_NONE;
    int ecmp_group;
    int offset;
    int leaving_next_hop_index;
    int num_remaining_members;
    int i, j;
    ecmp_count_entry_t ecmp_count_entry;
    int entry_base_ptr;
    int flow_set_size;
    int num_entries;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int index_min, index_max;
    int leaving_member_entry_count;
    int *entry_count_arr = NULL;
    rh_ecmp_flowset_entry_t *flowset_entry;
    int next_hop_index;
    int lower_bound, upper_bound;
    int threshold;
    int chosen_index;

    if (ecmp == NULL ||
            ecmp->dynamic_mode != BCM_L3_ECMP_DYNAMIC_MODE_RESILIENT) {
        return BCM_E_PARAM;
    }

    if (BCM_XGS3_L3_MPATH_EGRESS_IDX_VALID(unit, ecmp->ecmp_intf)) {
        ecmp_group = ecmp->ecmp_intf - BCM_XGS3_MPATH_EGRESS_IDX_MIN;
    } else {
        return BCM_E_PARAM;
    }

    if (intf_count > 0 && intf_array == NULL) {
        return BCM_E_PARAM;
    }

    if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, leaving_intf)) {
        offset = BCM_XGS3_EGRESS_IDX_MIN;
    } else if (BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, leaving_intf)) {
        offset = BCM_XGS3_DVP_EGRESS_IDX_MIN;
    } else {
        return BCM_E_PARAM;
    }
    leaving_next_hop_index = leaving_intf - offset;

    if (intf_count == 0) {
        /* Deleting the last member is the same as freeing all resources */
        BCM_IF_ERROR_RETURN(_bcm_td2_ecmp_rh_free_resource(unit, ecmp_group));

        /* Store the dynamic_size in hardware */
        BCM_IF_ERROR_RETURN(_bcm_td2_ecmp_rh_dynamic_size_set(unit, ecmp_group,
                    ecmp->dynamic_size));

        return BCM_E_NONE;
    }

    /* Check that the leaving member is not a member of the array */
    for (i = 0; i < intf_count; i++) {
        if (leaving_intf == intf_array[i]) {
            return BCM_E_PARAM;
        }
    }
    num_remaining_members = intf_count;

    /* Read all the flow set entries of this ECMP resilient hash group,
     * and compute the number of entries currently assigned to each
     * member, including the leaving member.
     */
    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY, ecmp_group,
                &ecmp_count_entry));
    entry_base_ptr = soc_L3_ECMP_COUNTm_field32_get(unit, &ecmp_count_entry,
            RH_FLOW_SET_BASEf);
    flow_set_size = soc_L3_ECMP_COUNTm_field32_get(unit, &ecmp_count_entry,
            RH_FLOW_SET_SIZEf);
    BCM_IF_ERROR_RETURN
        (_bcm_td2_ecmp_rh_dynamic_size_decode(flow_set_size, &num_entries));

    alloc_size = num_entries * sizeof(rh_ecmp_flowset_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "RH_ECMP_FLOWSET entries");
    if (NULL == buf_ptr) {
        return BCM_E_MEMORY;
    }
    sal_memset(buf_ptr, 0, alloc_size);
    index_min = entry_base_ptr;
    index_max = index_min + num_entries - 1;
    rv = soc_mem_read_range(unit, RH_ECMP_FLOWSETm, MEM_BLOCK_ANY,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

    alloc_size = num_remaining_members * sizeof(int);
    entry_count_arr = sal_alloc(alloc_size, "RH entry count array");
    if (NULL == entry_count_arr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(entry_count_arr, 0, alloc_size);

    leaving_member_entry_count = 0;
    for (i = 0; i < num_entries; i++) {
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_ECMP_FLOWSETm, rh_ecmp_flowset_entry_t *, buf_ptr, i);
        if (!soc_mem_field32_get(unit, RH_ECMP_FLOWSETm, flowset_entry,
                    VALIDf)) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
        next_hop_index = soc_mem_field32_get(unit, RH_ECMP_FLOWSETm,
                flowset_entry, NEXT_HOP_INDEXf);
        if (next_hop_index == leaving_next_hop_index) {
            leaving_member_entry_count++;
        } else {
            for (j = 0; j < num_remaining_members; j++) {
                if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit, intf_array[j])) {
                    offset = BCM_XGS3_EGRESS_IDX_MIN;
                } else if (BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, intf_array[j])) {
                    offset = BCM_XGS3_DVP_EGRESS_IDX_MIN;
                } else {
                    rv = BCM_E_PARAM;
                    goto cleanup;
                }
                if (next_hop_index == (intf_array[j] - offset)) {
                    break;
                }
            }
            if (j == num_remaining_members) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            entry_count_arr[j]++;
        }
    }

    /* Check that the distribution of flow set entries among all members
     * is balanced. For instance, if the number of flow set entries is 64, and
     * the number of members is 6, then every member should have
     * between 10 and 11 entries. 
     */ 
    lower_bound = num_entries / (num_remaining_members + 1);
    upper_bound = (num_entries % (num_remaining_members + 1)) ?
                  (lower_bound + 1) : lower_bound;
    for (i = 0; i < num_remaining_members; i++) {
        if (entry_count_arr[i] < lower_bound ||
                entry_count_arr[i] > upper_bound) {
            rv = BCM_E_INTERNAL;
            goto cleanup;
        }
    }
    if (leaving_member_entry_count < lower_bound ||
            leaving_member_entry_count > upper_bound) {
        rv = BCM_E_INTERNAL;
        goto cleanup;
    }

    /* Re-balance flow set entries from the leaving member to the remaining
     * members. For example, if the number of flow set entries is 64, and
     * the number of members is 6, then each member has between 10 and 11
     * entries. The entries should be re-assigned from the leaving member to
     * the remaining 5 members such that each remaining member will end up
     * with between 12 and 13 entries.
     */
    lower_bound = num_entries / num_remaining_members;
    upper_bound = (num_entries % num_remaining_members) ?
                  (lower_bound + 1) : lower_bound;
    threshold = lower_bound;
    for (i = 0; i < num_entries; i++) {
        flowset_entry = soc_mem_table_idx_to_pointer(unit,
                RH_ECMP_FLOWSETm, rh_ecmp_flowset_entry_t *, buf_ptr, i);
        next_hop_index = soc_mem_field32_get(unit, RH_ECMP_FLOWSETm,
                flowset_entry, NEXT_HOP_INDEXf);
        if (next_hop_index != leaving_next_hop_index) {
            continue;
        }

        /* Randomly choose a member among the remaining members */
        rv = _bcm_td2_ecmp_rh_member_choose(unit, num_remaining_members,
                entry_count_arr, &threshold, &chosen_index);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
        if (BCM_XGS3_L3_EGRESS_IDX_VALID(unit,
                    intf_array[chosen_index])) {
            offset = BCM_XGS3_EGRESS_IDX_MIN;
        } else if (BCM_XGS3_DVP_EGRESS_IDX_VALID(unit,
                    intf_array[chosen_index])) {
            offset = BCM_XGS3_DVP_EGRESS_IDX_MIN;
        } else {
            rv = BCM_E_PARAM;
            goto cleanup;
        }
        soc_mem_field32_set(unit, RH_ECMP_FLOWSETm, flowset_entry,
                NEXT_HOP_INDEXf, intf_array[chosen_index] - offset);
    }

    /* Write re-balanced flow set entries to hardware */
    rv = soc_mem_write_range(unit, RH_ECMP_FLOWSETm, MEM_BLOCK_ALL,
            index_min, index_max, buf_ptr);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

cleanup:
    if (buf_ptr) {
        soc_cm_sfree(unit, buf_ptr);
    }
    if (entry_count_arr) {
        sal_free(entry_count_arr);
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_l3_egress_ecmp_rh_create 
 * Purpose:
 *      Create or modify an ECMP resilient hashing group.
 * Parameters:
 *      unit       - (IN) bcm device.
 *      ecmp       - (IN) ECMP group info.
 *      intf_count - (IN) Number of elements in intf_array.
 *      intf_array - (IN) Array of Egress forwarding objects.
 *      op         - (IN) Member operation: SET, ADD, or DELETE.
 *      intf       - (IN) Egress forwarding object to add or delete.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_td2_l3_egress_ecmp_rh_create(int unit, bcm_l3_egress_ecmp_t *ecmp,
        int intf_count, bcm_if_t *intf_array, int op, bcm_if_t intf)
{
    int dynamic_size_encode;
    int rh_enable;
    int ecmp_group;

    if (ecmp->dynamic_mode == BCM_L3_ECMP_DYNAMIC_MODE_RESILIENT) {
        /* Verify ecmp->dynamic_size */
        BCM_IF_ERROR_RETURN
            (_bcm_td2_ecmp_rh_dynamic_size_encode(ecmp->dynamic_size,
                                                  &dynamic_size_encode));
        rh_enable = 1;
    } else {
        rh_enable = 0;
    }

    if (op == BCM_L3_ECMP_MEMBER_OP_SET) {
        /* Free resilient hashing resources associated with this ecmp group */
        ecmp_group = ecmp->ecmp_intf - BCM_XGS3_MPATH_EGRESS_IDX_MIN;
        BCM_IF_ERROR_RETURN
            (_bcm_td2_ecmp_rh_free_resource(unit, ecmp_group));

        if (rh_enable) {
            /* Set resilient hashing members for this ecmp group */
            BCM_IF_ERROR_RETURN
                (_bcm_td2_ecmp_rh_set(unit, ecmp, intf_count, intf_array));
        }
    } else if (op == BCM_L3_ECMP_MEMBER_OP_ADD) {
        if (rh_enable) {
            /* Add new resilient hashing member to ecmp group */
            BCM_IF_ERROR_RETURN
                (_bcm_td2_ecmp_rh_add(unit, ecmp, intf_count, intf_array, intf));
        }
    } else if (op == BCM_L3_ECMP_MEMBER_OP_DELETE) {
        if (rh_enable) {
            /* Delete resilient hashing member from ecmp group */
            BCM_IF_ERROR_RETURN
                (_bcm_td2_ecmp_rh_delete(unit, ecmp, intf_count, intf_array, intf));
        }
    } else {
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_l3_egress_ecmp_rh_destroy
 * Purpose:
 *      Destroy an ECMP resilient hashing group.
 * Parameters:
 *      unit    - (IN) bcm device.
 *      mpintf  - (IN) L3 interface id pointing to Egress multipath object.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_td2_l3_egress_ecmp_rh_destroy(int unit, bcm_if_t mpintf) 
{
    int ecmp_group;

    if (BCM_XGS3_L3_MPATH_EGRESS_IDX_VALID(unit, mpintf)) {
        ecmp_group = mpintf - BCM_XGS3_MPATH_EGRESS_IDX_MIN;
    } else {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_td2_ecmp_rh_free_resource(unit, ecmp_group));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_l3_egress_ecmp_rh_get
 * Purpose:
 *      Get info about an ECMP resilient hashing group.
 * Parameters:
 *      unit - (IN) bcm device.
 *      ecmp - (INOUT) ECMP group info.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_td2_l3_egress_ecmp_rh_get(int unit, bcm_l3_egress_ecmp_t *ecmp)
{
    int ecmp_group;
    ecmp_count_entry_t ecmp_count_entry;
    int enhanced_hashing_enable;
    int flow_set_size;
    int num_entries;

    if (BCM_XGS3_L3_MPATH_EGRESS_IDX_VALID(unit, ecmp->ecmp_intf)) {
        ecmp_group = ecmp->ecmp_intf - BCM_XGS3_MPATH_EGRESS_IDX_MIN;
    } else {
        return BCM_E_PARAM;
    }

    /* An ECMP group can be in one of 3 resilient hashing states:
     * (1) RH not enabled: L3_ECMP_COUNT.RH_FLOW_SET_SIZE = 0.
     * (2) RH enabled but without any members:
     *     L3_ECMP_COUNT.RH_FLOW_SET_SIZE is non-zero,
     *     L3_ECMP_COUNT.ENHANCED_HASHING_ENABLE = 0.
     * (3) RH enabled with members:
     *     L3_ECMP_COUNT.RH_FLOW_SET_SIZE is non-zero,
     *     L3_ECMP_COUNT.ENHANCED_HASHING_ENABLE = 1.
     */

    SOC_IF_ERROR_RETURN(READ_L3_ECMP_COUNTm(unit, MEM_BLOCK_ANY,
                ecmp_group, &ecmp_count_entry));
    enhanced_hashing_enable = soc_L3_ECMP_COUNTm_field32_get(unit,
            &ecmp_count_entry, ENHANCED_HASHING_ENABLEf);
    flow_set_size = soc_L3_ECMP_COUNTm_field32_get(unit,
            &ecmp_count_entry, RH_FLOW_SET_SIZEf);

    if (0 == flow_set_size) {
        /* Resilient hashing is not enabled on this ECMP group. */
        return BCM_E_NONE;
    }

    ecmp->dynamic_mode = BCM_L3_ECMP_DYNAMIC_MODE_RESILIENT;
    if (enhanced_hashing_enable) {
        BCM_IF_ERROR_RETURN(_bcm_td2_ecmp_rh_dynamic_size_decode(flow_set_size,
                    &num_entries));
        ecmp->dynamic_size = num_entries;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_l3_egress_ecmp_rh_ethertype_set
 * Purpose:
 *      Set the Ethertypes that are eligible or ineligible for
 *      ECMP resilient hashing.
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (IN) BCM_L3_ECMP_DYNAMIC_ETHERTYPE_xxx flags.
 *      ethertype_count - (IN) Number of elements in ethertype_array.
 *      ethertype_array - (IN) Array of Ethertypes.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_td2_l3_egress_ecmp_rh_ethertype_set(
    int unit, 
    uint32 flags, 
    int ethertype_count, 
    int *ethertype_array)
{
    uint32 control_reg;
    int i, j;
    rh_ecmp_ethertype_eligibility_map_entry_t ethertype_entry;

    /* Input check */
    if (ethertype_count > soc_mem_index_count(unit,
                RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm)) {
        return BCM_E_RESOURCE;
    }

    /* Update ethertype eligibility control register */
    SOC_IF_ERROR_RETURN
        (READ_RH_ECMP_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, &control_reg));
    soc_reg_field_set(unit, RH_ECMP_ETHERTYPE_ELIGIBILITY_CONTROLr,
            &control_reg, ETHERTYPE_ELIGIBILITY_CONFIGf,
            flags & BCM_L3_ECMP_DYNAMIC_ETHERTYPE_ELIGIBLE ? 1 : 0);
    soc_reg_field_set(unit, RH_ECMP_ETHERTYPE_ELIGIBILITY_CONTROLr,
            &control_reg, INNER_OUTER_ETHERTYPE_SELECTIONf,
            flags & BCM_L3_ECMP_DYNAMIC_ETHERTYPE_INNER ? 1 : 0);
    SOC_IF_ERROR_RETURN
        (WRITE_RH_ECMP_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, control_reg));

    /* Update Ethertype eligibility map table */
    for (i = 0; i < ethertype_count; i++) {
        sal_memset(&ethertype_entry, 0,
                sizeof(rh_ecmp_ethertype_eligibility_map_entry_t));
        soc_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm_field32_set(unit,
                &ethertype_entry, VALIDf, 1);
        soc_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm_field32_set(unit,
                &ethertype_entry, ETHERTYPEf, ethertype_array[i]);
        SOC_IF_ERROR_RETURN(WRITE_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ALL, i, &ethertype_entry));
    }

    /* Zero out remaining entries of Ethertype eligibility map table */
    for (j = i; j < soc_mem_index_count(unit,
                RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm); j++) {
        SOC_IF_ERROR_RETURN(WRITE_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ALL, j, soc_mem_entry_null(unit,
                        RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm)));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_l3_egress_ecmp_rh_ethertype_get
 * Purpose:
 *      Get the Ethertypes that are eligible or ineligible for
 *      ECMP resilient hashing.
 * Parameters:
 *      unit - (IN) Unit number.
 *      flags - (INOUT) BCM_L3_ECMP_DYNAMIC_ETHERTYPE_xxx flags.
 *      ethertype_max - (IN) Number of elements in ethertype_array.
 *      ethertype_array - (OUT) Array of Ethertypes.
 *      ethertype_count - (OUT) Number of elements returned in ethertype_array.
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_td2_l3_egress_ecmp_rh_ethertype_get(
    int unit, 
    uint32 *flags, 
    int ethertype_max, 
    int *ethertype_array, 
    int *ethertype_count)
{
    uint32 control_reg;
    int i;
    int ethertype;
    rh_ecmp_ethertype_eligibility_map_entry_t ethertype_entry;

    *ethertype_count = 0;

    /* Get flags */
    SOC_IF_ERROR_RETURN
        (READ_RH_ECMP_ETHERTYPE_ELIGIBILITY_CONTROLr(unit, &control_reg));
    if (soc_reg_field_get(unit, RH_ECMP_ETHERTYPE_ELIGIBILITY_CONTROLr,
                control_reg, ETHERTYPE_ELIGIBILITY_CONFIGf)) {
        *flags |= BCM_L3_ECMP_DYNAMIC_ETHERTYPE_ELIGIBLE;
    }
    if (soc_reg_field_get(unit, RH_ECMP_ETHERTYPE_ELIGIBILITY_CONTROLr,
                control_reg, INNER_OUTER_ETHERTYPE_SELECTIONf)) {
        *flags |= BCM_L3_ECMP_DYNAMIC_ETHERTYPE_INNER;
    }

    /* Get Ethertypes */
    for (i = 0; i < soc_mem_index_count(unit,
                RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm); i++) {
        SOC_IF_ERROR_RETURN(READ_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm(unit,
                    MEM_BLOCK_ANY, i, &ethertype_entry));
        if (soc_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                    &ethertype_entry, VALIDf)) {
            ethertype = soc_RH_ECMP_ETHERTYPE_ELIGIBILITY_MAPm_field32_get(unit,
                    &ethertype_entry, ETHERTYPEf);
            if (NULL != ethertype_array) {
                ethertype_array[*ethertype_count] = ethertype;
            }
            (*ethertype_count)++;
            if ((ethertype_max > 0) && (*ethertype_count == ethertype_max)) {
                break;
            }
        }
    }

    return BCM_E_NONE;
}

#ifdef BCM_WARM_BOOT_SUPPORT

/*
 * Function:
 *      bcm_td2_ecmp_rh_hw_recover
 * Purpose:
 *      Recover ECMP resilient hashing internal state from hardware.
 * Parameters:
 *      unit - (IN) SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_ecmp_rh_hw_recover(int unit) 
{
    int rv = BCM_E_NONE;
    int alloc_size;
    uint32 *buf_ptr = NULL;
    int i;
    ecmp_count_entry_t *ecmp_count_entry;
    int flow_set_size;
    int enhanced_hashing_enable;
    int entry_base_ptr;
    int block_base_ptr;
    int num_entries;
    int num_blocks;

    /* Read entire L3_ECMP_COUNT table */
    alloc_size = soc_mem_index_count(unit, L3_ECMP_COUNTm) *
        sizeof(ecmp_count_entry_t);
    buf_ptr = soc_cm_salloc(unit, alloc_size, "L3_ECMP_COUNT entries");
    if (NULL == buf_ptr) {
        return BCM_E_MEMORY;
    }
    rv = soc_mem_read_range(unit, L3_ECMP_COUNTm, MEM_BLOCK_ANY,
            soc_mem_index_min(unit, L3_ECMP_COUNTm),
            soc_mem_index_max(unit, L3_ECMP_COUNTm),
            buf_ptr);
    if (SOC_FAILURE(rv)) {
        soc_cm_sfree(unit, buf_ptr);
        return rv;
    }

    /* Traverse ECMP groups to recover resilient hashing flow set
     * table usage.
     */
    for (i = 0; i < soc_mem_index_count(unit, L3_ECMP_COUNTm); i++) {

        /* An ECMP group can be in one of 3 resilient hashing states:
         * (1) RH not enabled: L3_ECMP_COUNT.RH_FLOW_SET_SIZE = 0.
         * (2) RH enabled but without any members:
         *     L3_ECMP_COUNT.RH_FLOW_SET_SIZE is non-zero,
         *     L3_ECMP_COUNT.ENHANCED_HASHING_ENABLE = 0.
         * (3) RH enabled with members:
         *     L3_ECMP_COUNT.RH_FLOW_SET_SIZE is non-zero,
         *     L3_ECMP_COUNT.ENHANCED_HASHING_ENABLE = 1.
         */

        ecmp_count_entry = soc_mem_table_idx_to_pointer(unit, L3_ECMP_COUNTm,
                ecmp_count_entry_t *, buf_ptr, i);
        flow_set_size = soc_L3_ECMP_COUNTm_field32_get(unit,
                ecmp_count_entry, RH_FLOW_SET_SIZEf);
        if (0 == flow_set_size) {
            /* Resilient hashing is not enabled on this ECMP group. */
            continue;
        }

        enhanced_hashing_enable = soc_L3_ECMP_COUNTm_field32_get(unit,
                ecmp_count_entry, ENHANCED_HASHING_ENABLEf);
        if (enhanced_hashing_enable) {
            /* Recover ECMP resilient hashing flow set block usage */
            entry_base_ptr = soc_L3_ECMP_COUNTm_field32_get(unit,
                    ecmp_count_entry, RH_FLOW_SET_BASEf);
            block_base_ptr = entry_base_ptr >> 6;
            rv = _bcm_td2_ecmp_rh_dynamic_size_decode(flow_set_size,
                    &num_entries);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, buf_ptr);
                return rv;
            }
            num_blocks = num_entries >> 6; 
            _BCM_ECMP_RH_FLOWSET_BLOCK_USED_SET_RANGE(unit, block_base_ptr,
                    num_blocks);
        }
    }

    soc_cm_sfree(unit, buf_ptr);

    return rv;
}

#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP

/*
 * Function:
 *     bcm_td2_ecmp_rh_sw_dump
 * Purpose:
 *     Displays ECMP resilient hashing state maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void bcm_td2_ecmp_rh_sw_dump(int unit)
{
    int i, j;

    soc_cm_print("  ECMP Resilient Hashing Info -\n");
    soc_cm_print("    RH Flowset Table Blocks Total: %d\n",
            _td2_ecmp_rh_info[unit]->num_ecmp_rh_flowset_blocks);

    /* Print ECMP RH flowset table usage */
    soc_cm_print("    RH Flowset Table Blocks Used:");
    j = 0;
    for (i = 0; i < _td2_ecmp_rh_info[unit]->num_ecmp_rh_flowset_blocks; i++) {
        if (_BCM_ECMP_RH_FLOWSET_BLOCK_USED_GET(unit, i)) {
            j++;
            if (j % 15 == 1) {
                soc_cm_print("\n     ");
            }
            soc_cm_print(" %4d", i);
        }
    }
    soc_cm_print("\n");

    return;
}

#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */


#endif /* INCLUDE_L3 && BCM_TRIDENT2_SUPPORT  */
