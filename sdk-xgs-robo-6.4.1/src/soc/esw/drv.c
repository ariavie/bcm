/*
 * $Id: drv.c,v 1.1112 Broadcom SDK $
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
 * StrataSwitch driver
 */

#include <shared/bsl.h>

#include <stddef.h>
#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>
#include <sal/core/sync.h>
#include <sal/core/boot.h>
#include <sal/core/dpc.h>
#ifdef PLISIM
#include <sal/appl/config.h>
#endif

#include <soc/drv.h>
#include <soc/mcm/driver.h>     /* soc_base_driver_table */
#include <soc/cm.h>
#include <soc/mem.h>
#include <soc/error.h>
#include <soc/l2x.h>
#include <soc/l2u.h>
#include <soc/dma.h>
#include <soc/i2c.h>
#include <soc/counter.h>
#include <soc/hercules.h>
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#include <soc/uc_msg.h>
#endif
#ifdef BCM_IPROC_SUPPORT
#include <soc/iproc.h>
#endif
#if defined(BCM_FIREBOLT_SUPPORT)
#include <soc/firebolt.h>
#endif /* BCM_FIREBOLT_SUPPORT */
#if defined(BCM_BRADLEY_SUPPORT)
#include <soc/bradley.h>
#endif /* BCM_BRADLEY_SUPPORT */
#if defined(BCM_TRIUMPH_SUPPORT)
#include <soc/triumph.h>
#include <soc/er_tcam.h>
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_ENDURO_SUPPORT)
#include <soc/enduro.h>
#endif /* BCM_ENDURO_SUPPORT */
#if defined(BCM_HURRICANE_SUPPORT)
#include <soc/hurricane.h>
#endif /* BCM_HURRICANE_SUPPORT */
#if defined(BCM_HURRICANE2_SUPPORT)
#include <soc/hurricane2.h>
#endif /* BCM_HURRICANE2_SUPPORT */
#if defined(BCM_GREYHOUND_SUPPORT)
#include <soc/greyhound.h>
#endif /* BCM_GREYHOUND_SUPPORT */
#if defined(BCM_VALKYRIE_SUPPORT)
#include <soc/valkyrie.h>
#endif /* BCM_VALKYRIE_SUPPORT */
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <soc/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
#include <soc/trident.h>
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
#include <soc/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <soc/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_HELIX4_SUPPORT)
#include <soc/helix4.h>
#endif /* BCM_HELIX4_SUPPORT */
#if defined(BCM_SIRIUS_SUPPORT)
#include <soc/sbx/sbx_drv.h>
#endif
#if defined(BCM_SHADOW_SUPPORT)
#include <soc/shadow.h>
#endif /* BCM_SHADOW_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
#include <soc/katana.h>
#endif /* BCM_KATANA_SUPPORT */
#if defined(BCM_KATANA2_SUPPORT)
#include <soc/katana2.h>
#include <soc/uc.h>
#endif /* BCM_KATANA2_SUPPORT */
#include <soc/mmuerr.h>
#include <soc/higig.h>
#include <soc/ipoll.h>
#include <soc/phyctrl.h>
#include <soc/dport.h>
#include <soc/rcpu.h>
#ifdef INCLUDE_KNET
#include <soc/knet.h>
#endif

#ifdef BCM_KATANA_SUPPORT
#define   BCM_SABER_MAX_COUNTER_DIRECTION 2
#define   BCM_SABER_MAX_COUNTER_POOL      8
#endif

#ifdef BCM_ESW_SUPPORT
/* Macro to swap name to the mapped-to (actually-used) mem state name */
#define _SOC_DRV_MEM_REUSE_MEM_STATE(unit, mem, lock_mem) \
{\
    switch(mem) {\
    case VLAN_XLATE_1m: mem = VLAN_XLATEm; break;\
    case EP_VLAN_XLATE_1m: mem = EGR_VLAN_XLATEm; break;\
    case MPLS_ENTRY_1m: mem = MPLS_ENTRYm; break;\
    case VLAN_MACm: \
        if (SOC_IS_TRX(unit) && !soc_feature(unit, soc_feature_ism_memory)) {\
            mem = VLAN_XLATEm;\
        }\
        break;\
    case EGR_VLANm: \
        if (SOC_IS_KATANA2(unit)) {\
            mem = VLAN_TABm;\
        }\
        break;\
    default: break;\
    }\
    lock_mem = mem;\
    switch(mem) {\
    case L3_ENTRY_ONLYm: \
    case L3_ENTRY_IPV4_UNICASTm: \
    case L3_ENTRY_IPV6_UNICASTm: \
    case L3_ENTRY_IPV6_MULTICASTm: \
        if (soc_feature(unit, soc_feature_shared_hash_mem)) {\
            lock_mem = L3_ENTRY_IPV4_MULTICASTm;\
        }\
        break;\
    default: break;\
    }\
}
/* Routine to check if this is the mem whose state is being (mapped-to) actually-used by others */
int _SOC_DRV_MEM_IS_REUSED_MEM(int unit, soc_mem_t mem) {\
    switch(mem) {\
    case EGR_VLAN_XLATEm: return TRUE;\
    case MPLS_ENTRYm: return TRUE;\
    case VLAN_XLATEm:\
        if (SOC_IS_TRX(unit)) {\
            return TRUE;\
        }\
        break;\
    case VLAN_TABm:\
        if (SOC_IS_KATANA2(unit)) {\
            return TRUE;\
        }\
        break;\
    default: break;\
    }\
    return FALSE;\
}
#endif /* BCM_ESW_SUPPORT */

/*
 * Function:
 *      soc_mmu_init
 * Purpose:
 *      Initialize MMU registers
 * Parameters:
 *      unit - unit number
 * Returns:
 *      SOC_E_XXX
 */
int
soc_mmu_init(int unit)
{
    int                 rv;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "soc_mmu_init\n")));

    if (!soc_attached(unit)) {
        return SOC_E_INIT;
    }

    if (SOC_IS_RCPU_ONLY(unit)) {
        return SOC_E_NONE;
    }

    /* Perform Chip-Dependent Initializations */

    rv = SOC_FUNCTIONS(unit)->soc_mmu_init(unit);

    return rv;
}

/*
 * Function:
 *      soc_chip_driver_find
 * Purpose:
 *      Return the soc_driver for a chip if found.
 *      If not found, return NULL.
 * Parameters:
 *      pci_dev_id - PCI dev ID to find (exact match)
 *      pci_rev_id - PCI dev ID to find (exact match)
 * Returns:
 *      Pointer to static driver structure
 */
soc_driver_t *
soc_chip_driver_find(uint16 pci_dev_id, uint8 pci_rev_id)
{
    int                 i;
    soc_driver_t        *d;
    uint16              driver_dev_id;
    uint8               driver_rev_id;

    if (soc_cm_get_id_driver(pci_dev_id, pci_rev_id,
                             &driver_dev_id, &driver_rev_id) < 0) {
        return NULL;
    }

    /*
     * Find driver in table.  In theory any IDs returned by
     * soc_cm_id_to_driver_id() should have a driver in the table.
     */

    for (i = 0; i < SOC_NUM_SUPPORTED_CHIPS; i++) {
        d = soc_base_driver_table[i];
        if ((d != NULL) &&
            (d->block_info != NULL) &&
            (d->pci_device == driver_dev_id) &&
            (d->pci_revision == driver_rev_id)) {
            return d;
        }
    }

    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META("soc_chip_driver_find: driver in devid table "
                        "not in soc_base_driver_table\n")));

    return NULL;
}

#if defined(PLISIM) && defined(BCM_KATANA2_SUPPORT)
STATIC char *
_dummy_sysconf_get_property(const char *property)
{
#ifndef NO_SAL_APPL
    return sal_config_get(property);
#else
    return NULL;
#endif
}
STATIC char *
_dummy_config_var_get(soc_cm_dev_t *dev, const char *property)
{
    char  str[SAL_CONFIG_STR_MAX];
    int i=0;
    COMPILER_REFERENCE(dev);
    for (i=0; (property[i] != '.') && (property[i] != '\0');i++) {
         str[i]=property[i];
    }
    str[i]='\0';
    return _dummy_sysconf_get_property(str);
}
#endif /* PLISIM && BCM_KATANA2_SUPPORT */

/*
 * Function:
 *      soc_info_config
 * Parameters:
 *      unit - Unit number
 *      soc - soc_control_t associated with this unit
 * Purpose:
 *      Fill in soc_info structure for a newly attached unit.
 *      Generates bitmaps and various arrays based on block and
 *      ports that the hardware has enabled.
 *
 *      This isn't STATIC so that pcid can get at it.
 */

int
soc_info_config(int unit, soc_control_t *soc)
{
    soc_info_t          *si;
    soc_pbmp_t          pbmp_valid;
    soc_pbmp_t          pbmp_xport_xe;
    soc_pbmp_t          pbmp_xport_ge;
    soc_pbmp_t          pbmp_gport_fe;
    soc_pbmp_t          pbmp_gport_stack;   /* GE port in Higig Lite mode */
    soc_pbmp_t          pbmp_gx25g;         /* GX port 24 Gbps capable */
    soc_pbmp_t          pbmp_gx21g;         /* GX port 21 Gbps capable */
    soc_pbmp_t          pbmp_gx20g;         /* GX port 20 Gbps capable */
    soc_pbmp_t          pbmp_gx16g;         /* GX port 16 Gbps capable */
    soc_pbmp_t          pbmp_gx13g;         /* GX port 13 Gbps capable */
    soc_pbmp_t          pbmp_gx12g;         /* GX port 12 Gbps capable */
    soc_pbmp_t          pbmp_mxq;           /* MXQ port */
    soc_pbmp_t          pbmp_mxq1g;         /* MXQ port 1 Gbps capable */
    soc_pbmp_t          pbmp_mxq2p5g;       /* MXQ port 2.5 Gbps capable */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    soc_pbmp_t          pbmp_mxq10g;        /* MXQ port 10 Gbps capable */
#endif /* (BCM_KATANA_SUPPORT) || (BCM_KATANA2_SUPPORT) */
    soc_pbmp_t          pbmp_mxq13g;        /* MXQ port 13 Gbps capable */
    soc_pbmp_t          pbmp_mxq21g;        /* MXQ port 20,21 Gbps capable */
#ifdef BCM_SHADOW_SUPPORT
    soc_pbmp_t          pbmp_xl;            /* XL port */
    soc_pbmp_t          pbmp_xl40g;         /* XL port 40 Gbps capable */
    soc_pbmp_t          pbmp_xl10g;         /* XL port 10 Gbps capable */
    soc_pbmp_t          pbmp_xl12g;         /* XL port 12 Gbps capable (SCH) */
#endif /* BCM_SHADOW_SUPPORT */
    soc_pbmp_t          pbmp_xg2p5g;        /* XG port 2.5 Gbps capable */
    soc_pbmp_t          pbmp_xq10g;         /* XQ port 10 Gbps capable */
    soc_pbmp_t          pbmp_xq12g;         /* XQ port 12 Gbps capable */
    soc_pbmp_t          pbmp_xq13g;         /* XQ port 13 Gbps capable */
    soc_pbmp_t          pbmp_xq2p5g;        /* XQ port 2.5 Gbps capable */
    soc_pbmp_t          pbmp_xq;            /* XQ port (any type) */
    soc_pbmp_t          pbmp_hyplite;       /* Hyperlite port */
    soc_pbmp_t          pbmp_tmp;
    soc_pbmp_t          pbmp_disabled;      /* forcefully disabled ports */
    soc_pbmp_t          pbmp_pp;            /* Packet Processing Port */
    uint16              dev_id;
    uint8               rev_id;
    uint16              drv_dev_id;
    uint8               drv_rev_id;
    int                 port, phy_port, max_port, blk, bindex, pno=0, mem;
    char                *bname, *old_bname;
    int                 blktype, old_blktype, port_idx, instance;
    char                instance_string[3];
#ifdef BCM_HURRICANE_SUPPORT
    int                 dxgxs = 0;
    uint32              regval;
#endif /* BCM_HURRICANE_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    int                 kt_mxqport_index=0;
    int                 kt_mxqport2[4]={27,32,33,34};
#endif /* BCM_KATANA_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
    int                 mxq7_blk=0; 
/* Just keeping below for informatory for short duration */
#if 0
    int                 kt2_mxqport_num=0;
    int                 kt2_mxqport_index=0;
    int                 kt2_mxqports_list[10][4]={
                        { 1, 2, 3, 4},  /* MXQPORT0 */
                        { 5, 6, 7, 8},  /* MXQPORT1 */
                        { 9,10,11,12},  /* MXQPORT2 */
                        {13,14,15,16},  /* MXQPORT3 */
                        {17,18,19,20},  /* MXQPORT4 */
                        {21,22,23,24},  /* MXQPORT5 */
                        {25,35,36,37},  /* MXQPORT6 */
                        {26,38,39,40},  /* MXQPORT7 */
                        {27,32,33,34},  /* MXQPORT8 */
                        {28,29,30,31}   /* MXQPORT9 */
                        };
#endif
    kt2_pbmp_t kt2_pbmp={0};
#endif /* BCM_KATANA2_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) \
    || defined(BCM_TRIDENT2_SUPPORT)
    int                 rv;
#endif

    si = &soc->info;
    sal_memset((void *)si, 0, sizeof(soc_info_t));

    soc_cm_get_id(unit, &dev_id, &rev_id);
    soc_cm_get_id_driver(dev_id, rev_id, &drv_dev_id, &drv_rev_id);

    si->driver_type = soc->chip_driver->type;
    si->driver_group = soc_chip_type_map[si->driver_type];
    si->num_cpu_cosq = 8;
    si->num_mmu_base_cosq = 0;
    si->modid_count = 1;
    si->modid_max = 31;    /* See SOC_MODID_MAX(unit) */
    si->modport_max = 31;
    si->trunk_bit_pos = 5; /* 6th bit from right. See SOC_TRUNK_BIT_POS(unit) */
    si->port_addr_max = 31;
    si->vlan_xlate_glp_wildcard = 0x3fff; /* For most chips it is 3fff but can vary */
    si->cpu_hg_index = -1;
    si->cpu_hg_pp_port_index = -1;
    si->port_class_max = 64;    /* By default 64 classes are supported */
    si->num_flex_ingress_pools = 0;
    si->num_flex_egress_pools = 0;
    si->size_flex_ingress_pool = 0;
    si->size_flex_egress_pool = 0;
    si->pcie_phy_addr = 0;
#ifdef BCM_SBUSDMA_SUPPORT
    if (!soc->max_sbusdma_channels) {
        /* 
         * only assign init value one time, the value is fixed for each chip 
         * assign them when the first call of if soc_info_config() 
         * by soc_attach() 
         */
        soc->max_sbusdma_channels = 0;
        soc->tdma_ch = -1;
        soc->tslam_ch = -1;
        soc->desc_ch = -1;
    }
#endif
    SOC_PBMP_CLEAR(pbmp_xport_xe);
    SOC_PBMP_CLEAR(pbmp_xport_ge);
    SOC_PBMP_CLEAR(pbmp_gport_fe);
    SOC_PBMP_CLEAR(pbmp_gport_stack);
    SOC_PBMP_CLEAR(pbmp_disabled);
    SOC_PBMP_CLEAR(pbmp_pp);
    SOC_PBMP_CLEAR(si->linkphy_pbm);
    SOC_PBMP_CLEAR(si->lp.bitmap);
    SOC_PBMP_CLEAR(si->subtag_pbm);
    SOC_PBMP_CLEAR(si->subtag.bitmap);
    SOC_PBMP_CLEAR(si->subtag_allowed_pbm);
    SOC_PBMP_CLEAR(si->linkphy_pp_port_pbm);
    SOC_PBMP_CLEAR(si->subtag_pp_port_pbm);
    si->subtag_enabled = 0;
    si->linkphy_enabled = 0;

    /* XGPORT is set to 1G if it is not included in one of the pbmp below */
    SOC_PBMP_CLEAR(pbmp_xg2p5g);

    /* XQPORT is set to 1G if it is not included in one of the pbmp below */
    SOC_PBMP_CLEAR(pbmp_xq2p5g);
    SOC_PBMP_CLEAR(pbmp_xq10g);
    SOC_PBMP_CLEAR(pbmp_xq12g);
    SOC_PBMP_CLEAR(pbmp_xq13g);
    SOC_PBMP_CLEAR(pbmp_xq);

#ifdef BCM_SHADOW_SUPPORT
    /* XLPORT is set to one of the pbmp below */
    SOC_PBMP_CLEAR(pbmp_xl);
    SOC_PBMP_CLEAR(pbmp_xl10g);
    SOC_PBMP_CLEAR(pbmp_xl12g);
    SOC_PBMP_CLEAR(pbmp_xl40g);
#endif /* BCM_SHADOW_SUPPORT */

    /* GXPORT is set to 10G if it is not included in one of the pbmp below */
    SOC_PBMP_CLEAR(pbmp_gx12g);
    SOC_PBMP_CLEAR(pbmp_gx13g);
    SOC_PBMP_CLEAR(pbmp_gx16g);
    SOC_PBMP_CLEAR(pbmp_gx20g);
    SOC_PBMP_CLEAR(pbmp_gx21g);
    SOC_PBMP_CLEAR(pbmp_gx25g);
    SOC_PBMP_CLEAR(pbmp_mxq);
    SOC_PBMP_CLEAR(pbmp_mxq1g);
    SOC_PBMP_CLEAR(pbmp_mxq2p5g);
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    SOC_PBMP_CLEAR(pbmp_mxq10g);
#endif /* defined(BCM_KATANA_SUPPORT) || (BCM_KATANA2_SUPPORT) */
    SOC_PBMP_CLEAR(pbmp_mxq13g);
    SOC_PBMP_CLEAR(pbmp_mxq21g);

    SOC_PBMP_CLEAR(si->s_pbm);  /* 10/100/1000/2500 Mbps comboserdes */
    SOC_PBMP_CLEAR(si->gmii_pbm);
    SOC_PBMP_CLEAR(si->lb_pbm);
    SOC_PBMP_CLEAR(si->mmu_pbm);
    SOC_PBMP_CLEAR(pbmp_hyplite);

    SOC_PBMP_CLEAR(si->pbm_ext_mem);
    for (port = 0; port < SOC_MAX_NUM_PORTS; port++) {
        si->max_port_p2m_mapping[port] = -1;
    }

    /*
     * Used to implement the SOC_IS_*(unit) macros
     */
    switch (drv_dev_id) {
#if defined(BCM_HERCULES_SUPPORT)
    case BCM5675_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_HERCULES15;
        soc->base_modid = -1;
        si->modid_count = 0;
        break;
#endif /* BCM_HERCULES_SUPPORT */
    case BCM56102_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_FHX;
        si->chip_type = SOC_INFO_CHIP_TYPE_FELIX;
        SOC_PBMP_WORD_SET(pbmp_gport_fe, 0, 0x00ffffff);
        si->modid_max = 63;
        si->hg_offset = 24;     /* SOC_HG_OFFSET */
        break;
    case BCM56304_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_FHX;
        si->chip_type = SOC_INFO_CHIP_TYPE_HELIX;
        si->modid_max = 63;
        si->hg_offset = 24;     /* SOC_HG_OFFSET */
        break;
    case BCM56112_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_FHX;
        si->chip_type = SOC_INFO_CHIP_TYPE_FELIX15;
        SOC_PBMP_WORD_SET(pbmp_gport_fe, 0, 0x00ffffff);
        si->modid_max = 63;
        si->hg_offset = 24;     /* SOC_HG_OFFSET */
        break;
    case BCM56314_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_FHX;
        si->chip_type = SOC_INFO_CHIP_TYPE_HELIX15;
        si->modid_max = 63;
        si->hg_offset = 24;     /* SOC_HG_OFFSET */
        break;
    case BCM56504_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_FIREBOLT;
        si->modid_max = 63;
        si->hg_offset = 24;     /* SOC_HG_OFFSET */
        break;
    case BCM56800_DEVICE_ID:
        switch (dev_id) {
        case BCM56580_DEVICE_ID:
            si->chip = SOC_INFO_CHIP_GOLDWING;
            SOC_CHIP_STRING(unit) = "goldwing";
            break;
        case BCM56700_DEVICE_ID:
        case BCM56701_DEVICE_ID:
            si->chip = SOC_INFO_CHIP_HUMV;
            SOC_CHIP_STRING(unit) = "humv";
            break;
        default:
            si->chip = SOC_INFO_CHIP_BRADLEY;
            break;
        }
        si->modid_max = 127;
        si->num_cpu_cosq = 16;
        break;
#if defined(BCM_RAPTOR_SUPPORT)
    case BCM56218_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_RAPTOR;
        SOC_PBMP_WORD_SET(pbmp_gport_stack, 0, 0x00000006);
        SOC_PBMP_WORD_SET(si->s_pbm, 0, 0x00000006);
        SOC_PBMP_WORD_SET(si->hg2_pbm, 0, 0x00000006);
        SOC_PBMP_WORD_SET(si->gmii_pbm, 0, 0x00000008);
        si->modid_max = 15;     /* See SOC_MODID_MAX(unit) */
        /* 7th bit from right. See SOC_TRUNK_BIT_POS(unit)*/
    si->trunk_bit_pos = 6;
        si->port_addr_max = 63;
        si->hg_offset = 0;      /* SOC_HG_OFFSET */
        break;
    case BCM56224_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_RAVEN;
        SOC_PBMP_WORD_SET(pbmp_gport_stack, 0, 0x00000036);
        SOC_PBMP_WORD_SET(si->s_pbm, 0, 0x00000036);
        SOC_PBMP_WORD_SET(si->hg2_pbm, 0, 0x00000036);
        SOC_PBMP_WORD_SET(si->gmii_pbm, 0, 0x00000008);
        si->modid_max = 15;     /* See SOC_MODID_MAX(unit) */
        /* 7th bit from right. See SOC_TRUNK_BIT_POS(unit)*/
        si->trunk_bit_pos = 6;
        si->port_addr_max = 63;
        si->hg_offset = 0;      /* SOC_HG_OFFSET */
        break;
    case BCM53312_DEVICE_ID:
    case BCM53313_DEVICE_ID:
    case BCM53314_DEVICE_ID:
    case BCM53322_DEVICE_ID:
    case BCM53323_DEVICE_ID:
    case BCM53324_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_HAWKEYE;
        si->modid_max = 0;     /* See SOC_MODID_MAX(unit) */
        /* 7th bit from right. See SOC_TRUNK_BIT_POS(unit)*/
        si->trunk_bit_pos = 6;
        si->port_addr_max = 31;
        si->hg_offset = 0;      /* SOC_HG_OFFSET */
        break;
#endif /* BCM_RAPTOR_SUPPORT */
    case BCM56514_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_FIREBOLT2;
        si->modid_max = 63;
        si->hg_offset = 24;     /* SOC_HG_OFFSET */
        break;
    case BCM56624_DEVICE_ID:
        SOC_PBMP_WORD_SET(si->s_pbm, 0, 0x00000002);
        si->chip = SOC_INFO_CHIP_TRIUMPH;
        si->modid_max = 127;    /* See SOC_MODID_MAX(unit) */
        si->hg_offset = 28;     /* SOC_HG_OFFSET */
        /*
         * 14th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
    si->trunk_bit_pos = 13;
        si->port_addr_max = 63;
        si->num_cpu_cosq = 48;
        if (soc_property_get(unit, spn_MODULE_64PORTS, 0)) {
            si->modport_max = 63;
        } else {
            /* Adjust modid count based on dual-modid config parameter settings.
             * By default, dual-modid is enabled (32-port module mode) to allow
             * compatibility with other devices.
             */
            si->modid_count = 2;
        }
        break;
    case BCM56680_DEVICE_ID:
        SOC_PBMP_WORD_SET(si->s_pbm, 0, 0x00000002);
        si->chip = SOC_INFO_CHIP_VALKYRIE;
        si->modid_max = 127;    /* See SOC_MODID_MAX(unit) */
        si->hg_offset = 28;     /* SOC_HG_OFFSET */
        /*
         * 14th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
        si->trunk_bit_pos = 13;
        si->port_addr_max = 63;
        si->num_cpu_cosq = 48;
        if (soc_property_get(unit, spn_MODULE_64PORTS, 0)) {
            si->modport_max = 63;
        } else {
            /* Adjust modid count based on dual-modid config parameter settings.
             * By default, dual-modid is enabled (32-port module mode) to allow
             * compatibility with other devices.
             */
            si->modid_count = 2;
        }
        break;
    case BCM56820_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_SCORPION;
        si->modid_max = 255;
        si->port_addr_max = 63;
        si->num_cpu_cosq = 32;
        si->trunk_bit_pos = 14;
        si->vlan_xlate_glp_wildcard = 0x7fff; /* glp wildcard value */
        /* SOC_HG_OFFSET = 0, default */
        break;
    case BCM56725_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_CONQUEROR;
        si->modid_max = 255;
        si->port_addr_max = 63;
        si->num_cpu_cosq = 32;
        si->trunk_bit_pos = 14;
        si->vlan_xlate_glp_wildcard = 0x7fff; /* glp wildcard value */
        /* SOC_HG_OFFSET = 0, default */
        break;
    case BCM56634_DEVICE_ID:
        SOC_PBMP_WORD_SET(si->s_pbm, 0, 0x00000002);
        SOC_PBMP_WORD_SET(si->lb_pbm, 1, 0x00400000);
        si->lb_port = 54;
        SOC_PBMP_WORD_SET(si->mmu_pbm, 1, 0x01800000);
        si->chip = SOC_INFO_CHIP_TRIUMPH2;
        si->modid_max = 127;    /* See SOC_MODID_MAX(unit) */
        si->hg_offset = 26;     /* SOC_HG_OFFSET */
        /*
         * 14th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
    si->trunk_bit_pos = 13;
        si->port_addr_max = 63;
        si->num_cpu_cosq = 48;
        if (soc_property_get(unit, spn_MODULE_64PORTS, 0)) {
            si->modport_max = 63;
        } else {
            /* Adjust modid count based on dual-modid config parameter settings.
             * By default, dual-modid is enabled (32-port module mode) to allow
             * compatibility with other devices.
             */
            si->modid_count = 2;
        }
        si->num_time_interface = 1;
        switch (dev_id) {
        case BCM56634_DEVICE_ID:
        case BCM56538_DEVICE_ID:
            if (soc_property_get(unit, spn_BCM56634_48G_4X12, 0) ||
                soc_property_get(unit, spn_BCM56634_48G_2X24, 0) ||
                soc_property_get(unit, spn_BCM56538_48G_4X12, 0) ||
                soc_property_get(unit, spn_BCM56538_48G_2X24, 0)) {
                si->internal_loopback = 1;
            } else {
                si->internal_loopback = 0;
            }
            break;
        case BCM56636_DEVICE_ID:
            if (soc_property_get(unit, spn_BCM56636_24G_6X12, 0) ||
                soc_property_get(unit, spn_BCM56636_2X12_2X24, 0)) {
                si->internal_loopback = 1;
            } else {
                si->internal_loopback = 0;
            }
            break;
        case BCM56638_DEVICE_ID:
            if (soc_property_get(unit, spn_BCM56638_8X12, 0) ||
                soc_property_get(unit, spn_BCM56638_4X12_2X24, 0)) {
                si->internal_loopback = 1;
            } else {
                si->internal_loopback = 0;
            }
            break;
        default:
            si->internal_loopback = 1;
            break;
        }
        break;
    case BCM56524_DEVICE_ID:
        SOC_PBMP_WORD_SET(si->lb_pbm, 1, 0x00400000);
        si->lb_port = 54;
        SOC_PBMP_WORD_SET(si->mmu_pbm, 1, 0x01800000);
        si->chip = SOC_INFO_CHIP_APOLLO;
        si->modid_max = 127;    /* See SOC_MODID_MAX(unit) */
        si->hg_offset = 26;     /* SOC_HG_OFFSET */
        /*
         * 14th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
    si->trunk_bit_pos = 13;
        si->port_addr_max = 63;
        si->num_cpu_cosq = 48;
        if (soc_property_get(unit, spn_MODULE_64PORTS, 0)) {
            si->modport_max = 63;
        } else {
            /* Adjust modid count based on dual-modid config parameter settings.
             * By default, dual-modid is enabled (32-port module mode) to allow
             * compatibility with other devices.
             */
            si->modid_count = 2;
        }
        si->num_time_interface = 1;
        si->internal_loopback = 1;
        break;
    case BCM56685_DEVICE_ID:
        SOC_PBMP_WORD_SET(si->lb_pbm, 1, 0x00400000);
        si->lb_port = 54;
        SOC_PBMP_WORD_SET(si->mmu_pbm, 1, 0x01800000);
        si->chip = SOC_INFO_CHIP_VALKYRIE2;
        si->modid_max = 127;    /* See SOC_MODID_MAX(unit) */
        si->hg_offset = 26;     /* SOC_HG_OFFSET */
        /*
         * 14th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
    si->trunk_bit_pos = 13;
        si->port_addr_max = 63;
        si->num_cpu_cosq = 48;
        if (soc_property_get(unit, spn_MODULE_64PORTS, 0)) {
            si->modport_max = 63;
        } else {
            /* Adjust modid count based on dual-modid config parameter settings.
             * By default, dual-modid is enabled (32-port module mode) to allow
             * compatibility with other devices.
             */
            si->modid_count = 2;
        }
        si->num_time_interface = 1;
        si->internal_loopback = 1;
        break;
#if defined(BCM_ENDURO_SUPPORT)
    case BCM56132_DEVICE_ID:
    case BCM56134_DEVICE_ID:
    case BCM56331_DEVICE_ID:
    case BCM56333_DEVICE_ID:
    case BCM56334_DEVICE_ID:
    case BCM56320_DEVICE_ID:
    case BCM56321_DEVICE_ID:
    case BCM56338_DEVICE_ID:
    case BCM56230_DEVICE_ID:
    case BCM56231_DEVICE_ID:
        if(dev_id == BCM56338_DEVICE_ID) {
            SOC_PBMP_WORD_SET(pbmp_gport_stack, 0, 0xc000000);
        } else if(dev_id == BCM56132_DEVICE_ID) {
            SOC_PBMP_WORD_SET(pbmp_gport_stack, 0, 0x30000000);
        } else if((dev_id == BCM56334_DEVICE_ID) || (dev_id == BCM56320_DEVICE_ID) ||
                  (dev_id == BCM56321_DEVICE_ID)){
            SOC_PBMP_WORD_SET(pbmp_gport_stack, 0, 0x3c000000);
        }
        /* SOC_PBMP_WORD_SET(si->s_pbm, 0, 0x00000002);
        SOC_PBMP_WORD_SET(si->mmu_pbm, 1, 0x01800000); */
        SOC_PBMP_WORD_SET(si->lb_pbm, 0, 0x00000002);
        si->chip = SOC_INFO_CHIP_ENDURO;
        si->modid_max = 63;    /* See SOC_MODID_MAX(unit) */
        si->hg_offset = 26;     /* SOC_HG_OFFSET */
        /*
         * 14th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
        si->trunk_bit_pos = 13;
        /*
         * Note that maximum port number for enduro is 32. But port_addr_max
         *  is defined as 63 to make enduro compatible to other TRX families.
         */
        si->port_addr_max = 63;
        si->num_cpu_cosq = 48;
        si->internal_loopback = 1;
        si->num_time_interface = 1;
        break;
#endif /* BCM_ENDURO_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
    case BCM88732_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_SHADOW;
        si->modid_max = 255;
        si->port_addr_max = 63;
        si->num_cpu_cosq = 32;
        si->trunk_bit_pos = 14;
        si->pcie_phy_addr = 0x9a;
        break;
#endif /* BCM_SHAODOW_SUPPORT */

#if defined(BCM_HURRICANE_SUPPORT)
    case BCM56140_DEVICE_ID:
    case BCM56142_DEVICE_ID:
    case BCM56143_DEVICE_ID:
    case BCM56144_DEVICE_ID:
    case BCM56146_DEVICE_ID:
    case BCM56147_DEVICE_ID:
    case BCM56149_DEVICE_ID:
        /* Read DXGXS input */
        if (SAL_BOOT_PLISIM) {
            dxgxs=1;
        } else {
            /* At this point, Endianness may or may not be configured. Compensate for it */
            soc_pci_getreg(unit, soc_reg_addr(unit, CMIC_MISC_STATUSr, 0, 0), &regval);
            if ((CMVEC(unit).big_endian_pio ? 1 : 0) !=
                ((soc_pci_read(unit, CMIC_ENDIAN_SELECT) & ES_BIG_ENDIAN_PIO) ? 1 : 0) )
            {
                dxgxs = (regval & 0x01000000) ? 1 : 0;
            } else {
                dxgxs = (regval & 0x01) ? 1 : 0;
            }
        }
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        if(dev_id == BCM56142_DEVICE_ID) {
            if (soc_property_get(unit, spn_BCM5614X_CONFIG, 1) == 0) {
                /* Full Capacity (24x1G + 4xHGd) for testing */
                SOC_PBMP_WORD_SET(pbmp_gport_stack, 0, 0x3c000000);
            } else if (soc_property_get(unit, spn_BCM5614X_CONFIG, 1) == 2) {
                /* 24x1G + 2xHG[13] */
                SOC_PBMP_WORD_SET(pbmp_gport_stack, 0, 0x14000000);
            } else {
                SOC_PBMP_WORD_SET(pbmp_gport_stack, 0, dxgxs ? 0x34000000 : 0x1c000000);
            }
        } else if(dev_id == BCM56143_DEVICE_ID) {
            if (!soc_property_get(unit, spn_BCM5614X_CONFIG, 0)) {
                SOC_PBMP_WORD_SET(pbmp_gport_stack, 0, dxgxs ? 0x0c000000 : 0x30000000);
            }
        }

        si->chip = SOC_INFO_CHIP_HURRICANE;
        si->modid_max = 63;    /* See SOC_MODID_MAX(unit) */
        si->hg_offset = 26;     /* SOC_HG_OFFSET */
        /*
         * 14th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
        si->trunk_bit_pos = 13;
        /*
         * Note that maximum port number for hurricane is 32. But port_addr_max
         *  is defined as 63 to make hurricane compatible to other TRX families.
         */
        si->port_addr_max = 63;
        si->num_cpu_cosq = 8;
        si->internal_loopback = 0;
        si->num_time_interface = 1;
        break;
#endif /* BCM_HURRICANE_SUPPORT */

#if defined(BCM_HURRICANE2_SUPPORT)
    case BCM56150_DEVICE_ID:
    case BCM56151_DEVICE_ID:
    case BCM56152_DEVICE_ID:
    case BCM53342_DEVICE_ID:
    case BCM53343_DEVICE_ID:
    case BCM53344_DEVICE_ID:
    case BCM53346_DEVICE_ID:
    case BCM53347_DEVICE_ID:
    case BCM53333_DEVICE_ID:
    case BCM53334_DEVICE_ID:
    case BCM53393_DEVICE_ID:
    case BCM53394_DEVICE_ID:
        
        si->chip = SOC_INFO_CHIP_HURRICANE2;
        si->modid_max = 63;    /* See SOC_MODID_MAX(unit) */
        si->hg_offset = 26;     /* SOC_HG_OFFSET */
        /*
         * 14th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
        si->trunk_bit_pos = 13;
        /*
         * Note that maximum port number for hurricane is 32. But port_addr_max
         *  is defined as 63 to make hurricane compatible to other TRX families.
         */
        si->port_addr_max = 63;
        si->num_cpu_cosq = 8;
        si->internal_loopback = 0;
        si->num_time_interface = 1;
        si->num_flex_ingress_pools = 8;
        si->num_flex_egress_pools = 8;
        si->pcie_phy_addr = 0x2;
#ifdef BCM_SBUSDMA_SUPPORT
        soc->max_sbusdma_channels = 3;
        soc->tdma_ch = 0;
        soc->tslam_ch = 1;
        soc->desc_ch = 2;
#endif
        break;
#endif /* BCM_HURRICANE2_SUPPORT */

#if defined(BCM_GREYHOUND_SUPPORT)
    case BCM53400_DEVICE_ID:
        
        si->chip_type = SOC_INFO_CHIP_TYPE_GREYHOUND;

        si->modid_max = 63;    /* See SOC_MODID_MAX(unit) */
        si->hg_offset = 0;  /* SOC_HG_OFFSET */
        /*
         * 14th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
        si->trunk_bit_pos = 13;
        /*
         * Note that maximum port number for hurricane is 32. But port_addr_max
         *  is defined as 63 to make hurricane compatible to other TRX families.
         */
        si->port_addr_max = 63;
        si->num_cpu_cosq = 8;
        si->internal_loopback = 0;
        si->num_time_interface = 1;
        si->nof_pmqs = 1;
#ifdef BCM_SBUSDMA_SUPPORT
        soc->max_sbusdma_channels = 3;
        soc->tdma_ch = 0;
        soc->tslam_ch = 1;
        soc->desc_ch = 2;
#endif
        if ((dev_id & 0xF000) == 0x8000){
            dev_id &= 0xFF0F;
        }
        break;
#endif /* BCM_GREYHOUND_SUPPORT */


    case BCM56840_DEVICE_ID:
        if (dev_id == BCM56745_DEVICE_ID || dev_id == BCM56743_DEVICE_ID ||
            dev_id == BCM56746_DEVICE_ID || dev_id == BCM56744_DEVICE_ID) {
            si->chip = SOC_INFO_CHIP_TITAN;
        } else {
            si->chip = SOC_INFO_CHIP_TRIDENT;
        }
        si->port_class_max = 128;  /* egr_vlan_xlate port class max*/
        si->modid_max = 255;    /* See SOC_MODID_MAX(unit) */
        /* si->hg_offset = 0;    SOC_HG_OFFSET */
        /*
         * 16th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
        si->trunk_bit_pos = 15;
        /* port_addr_max has to be 2^n -1 */
        si->port_addr_max = 127;
        si->num_cpu_cosq = 48;
        si->pcie_phy_addr = 0xfa;
        if (soc_property_get(unit, spn_MODULE_64PORTS, 0)) {
            /* Use this property as single modid indicator for now */
            si->modport_max = 127;
        } else {
            /* By default, dual-modid is enabled (32-port module mode) to allow
             * compatibility with other devices. In this mode, port 64 will
             * not be globally addressable, however still allocate 3 modid
             * to allow using mod/port gport to address all ports.
             */
            si->modid_count = 3;
        }
        si->num_time_interface = 1;
        si->vlan_xlate_glp_wildcard = 0xffff; /* glp wildcard value */
        break;

#ifdef BCM_HELIX4_SUPPORT
    case BCM56548_DEVICE_ID:
    case BCM56547_DEVICE_ID:
    case BCM56347_DEVICE_ID:
    case BCM56346_DEVICE_ID:
    case BCM56344_DEVICE_ID:
    case BCM56342_DEVICE_ID:
    case BCM56340_DEVICE_ID:
    case BCM56049_DEVICE_ID:
    case BCM56048_DEVICE_ID:
    case BCM56047_DEVICE_ID:
    case BCM56042_DEVICE_ID:
    case BCM56041_DEVICE_ID:
    case BCM56040_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_HELIX4;
        si->chip_type = SOC_INFO_CHIP_TYPE_HELIX4;
        si->modid_max = 255;    /* See SOC_MODID_MAX(unit) */
        /* si->hg_offset = 0;    SOC_HG_OFFSET */
        /*
         * 16th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *      Note that it is not together with port/trunk-id.
         */
        si->trunk_bit_pos = 15;
        si->port_addr_max = 127; /* port_addr_max has to be 2^n -1 */
        si->num_cpu_cosq = 44;
        /* add multiple modid */
        si->num_time_interface = 1;
        si->vlan_xlate_glp_wildcard = 0xffff; /* glp wildcard value */
        si->pcie_phy_addr = 0x5;
#ifdef BCM_SBUSDMA_SUPPORT
        soc->max_sbusdma_channels = 3;
        soc->tdma_ch = 0;
        soc->tslam_ch = 1;
        soc->desc_ch = 2;
        soc_hx4_blk_counter_config(unit);
#endif /* BCM_SBUSDMA_SUPPORT */
        si->global_meter_pools = 8;
        si->global_meter_size_of_pool = 4096;
        si->num_flex_ingress_pools = 8;
        si->num_flex_egress_pools = 8;
        break;
#endif /* BCM_HELIX4_SUPPORT */

    case BCM56640_DEVICE_ID:
    case BCM56643_DEVICE_ID:
    case BCM56644_DEVICE_ID:
    case BCM56648_DEVICE_ID:
    case BCM56649_DEVICE_ID:
    case BCM56540_DEVICE_ID:
    case BCM56541_DEVICE_ID:
    case BCM56542_DEVICE_ID:
    case BCM56543_DEVICE_ID:
    case BCM56544_DEVICE_ID:
    case BCM56545_DEVICE_ID:
    case BCM56546_DEVICE_ID:
    case BCM56044_DEVICE_ID:
    case BCM56045_DEVICE_ID:
    case BCM56046_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_TRIUMPH3;
        si->modid_max = 255;    /* See SOC_MODID_MAX(unit) */
        si->port_class_max = 128;  /* egr_vlan_xlate port class max*/
        /* si->hg_offset = 0;    SOC_HG_OFFSET */
        /*
         * 16th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
        si->trunk_bit_pos = 15;
        si->port_addr_max = 127; /* port_addr_max has to be 2^n -1 */
        si->num_cpu_cosq = (rev_id >= BCM56640_B0_REV_ID) ? 45 : 48;
        /* add multiple modid */
        si->num_time_interface = 1;
        si->vlan_xlate_glp_wildcard = 0xffff; /* glp wildcard value */
        si->pcie_phy_addr = 0x9e;
#ifdef BCM_SBUSDMA_SUPPORT
        soc->max_sbusdma_channels = 3;
        soc->tdma_ch = 0;
        soc->tslam_ch = 1;
        soc->desc_ch = 2;
#ifdef BCM_TRIUMPH3_SUPPORT
        soc_tr3_blk_counter_config(unit);
#endif /* BCM_TRIUMPH3_SUPPORT */
#endif /* BCM_SBUSDMA_SUPPORT */
        si->global_meter_pools = 8;
        si->global_meter_size_of_pool = 4096;
        si->global_meter_max_size_of_pool = 4096;
        if (dev_id == BCM56540_DEVICE_ID ||
            dev_id == BCM56541_DEVICE_ID ||
            dev_id == BCM56542_DEVICE_ID ||
            dev_id == BCM56543_DEVICE_ID ||
            dev_id == BCM56544_DEVICE_ID ||
            dev_id == BCM56545_DEVICE_ID ||
            dev_id == BCM56546_DEVICE_ID) {
            si->global_meter_size_of_pool = 2048;
        }
        si->global_meter_action_size = 8192;
        if (soc_property_get(unit, spn_MODULE_64PORTS, 1)) { 
            si->modport_max = 63;
        } else { 
            /* Adjust modid count based on dual-modid config parameter settings.
             * By default, dual-modid is enabled (32-port module mode) to allow
             * compatibility with other devices.
             */
            si->modid_count = 2;
        }

        break;

#if defined(BCM_KATANA_SUPPORT)
    case BCM56440_DEVICE_ID:
    case BCM56441_DEVICE_ID:
    case BCM56442_DEVICE_ID:
    case BCM56443_DEVICE_ID:
    case BCM56445_DEVICE_ID:
    case BCM56446_DEVICE_ID:
    case BCM56447_DEVICE_ID:
    case BCM56448_DEVICE_ID:
    case BCM56449_DEVICE_ID:
    case BCM56240_DEVICE_ID:
    case BCM56241_DEVICE_ID:
    case BCM56242_DEVICE_ID:
    case BCM56243_DEVICE_ID:
    case BCM56245_DEVICE_ID:
    case BCM56246_DEVICE_ID:
    case BCM55440_DEVICE_ID:
    case BCM55441_DEVICE_ID:    
        si->chip = SOC_INFO_CHIP_KATANA;
        si->port_class_max = 128;  /* egr_vlan_xlate port class max*/
        si->modid_max = 255;    /* See SOC_MODID_MAX(unit) */
        si->hg_offset = 26;   /* SOC_HG_OFFSET */
        si->trunk_bit_pos = 15;
        si->port_addr_max = 127;
        si->num_cpu_cosq = 48;
        si->internal_loopback = 1;
        si->num_time_interface = 1;
        si->num_flex_ingress_pools = 8;
        si->num_flex_egress_pools = 8;
        si->pcie_phy_addr = 0x9d;
        if ((dev_id >= BCM56240_DEVICE_ID) &&
            (dev_id <= BCM56246_DEVICE_ID)) {
           si->size_flex_ingress_pool = 512;
           si->size_flex_egress_pool = 128;
        } else {
           si->size_flex_ingress_pool = 2048;
           si->size_flex_egress_pool = 1024;
        }
        si->vlan_xlate_glp_wildcard = 0xffff; /* glp wildcard value */
        si->global_meter_pools = 8;
        si->global_meter_size_of_pool = 1024;
        si->global_meter_max_size_of_pool = 1024;
        if ((dev_id >= BCM56240_DEVICE_ID) &&
            (dev_id <= BCM56246_DEVICE_ID)) {
            si->global_meter_size_of_pool = 256;
        }
        si->global_meter_action_size = 8192;
        /* default DDR3 Configuration */
        /* 16k Col * 1k Row * 8 Banks * 16 bits = 2Gb */
#if 0 /* Katana Only supports 1k Cols and 8 banks */
        soc->ddr3_num_columns = soc_property_get(unit,spn_EXT_RAM_COLUMNS, 1024);
        soc->ddr3_num_banks = soc_property_get(unit,spn_EXT_RAM_BANKS, 8);
#else
        soc->ddr3_num_columns = 1024;
        soc->ddr3_num_banks = 8;
#endif
        soc->ddr3_num_memories = soc_property_get(unit,spn_EXT_RAM_PRESENT, 3);
        soc->ddr3_num_rows = soc_property_get(unit,spn_EXT_RAM_ROWS, 8192);
        soc->ddr3_clock_mhz = soc_property_get(unit, spn_DDR3_CLOCK_MHZ, 800);
        soc->ddr3_mem_grade = soc_property_get(unit, spn_DDR3_MEM_GRADE, 0);

        break;
#endif /* BCM_KATANA_SUPPORT */

#if defined(BCM_KATANA2_SUPPORT)
    case BCM55450_DEVICE_ID:
    case BCM55455_DEVICE_ID:
    case BCM56248_DEVICE_ID:
    case BCM56450_DEVICE_ID:
    case BCM56452_DEVICE_ID:
    case BCM56454_DEVICE_ID:
    case BCM56455_DEVICE_ID:
    case BCM56456_DEVICE_ID:
    case BCM56457_DEVICE_ID:
    case BCM56458_DEVICE_ID:
        si->chip = SOC_INFO_CHIP_KATANA2;
        si->modid_max = 255;    /* See SOC_MODID_MAX(unit) */
        si->modport_max = 127;
        si->hg_offset = 26;   /* SOC_HG_OFFSET */
        si->trunk_bit_pos = 15; /* 16th bit from right. See SOC_TRUNK_BIT_POS(unit)*/
        si->port_addr_max = 127;
        si->num_cpu_cosq = 48;
        si->internal_loopback = 1;
        si->num_time_interface = 1;
        si->num_flex_ingress_pools = 8;
        si->num_flex_egress_pools = 8;
        si->size_flex_ingress_pool = 2048;
        si->size_flex_egress_pool = 1024;
        si->vlan_xlate_glp_wildcard = 0xffff; /* glp wildcard value */
        si->global_meter_pools = 8;
        si->global_meter_size_of_pool = 1024;
        si->global_meter_max_size_of_pool = 1024;
        si->pcie_phy_addr = 0x5;
        if ((dev_id >= BCM56240_DEVICE_ID) &&
            (dev_id <= BCM56246_DEVICE_ID)) {
            si->global_meter_size_of_pool = 256;
        }

        si->global_meter_action_size = 8192;
#ifdef BCM_SBUSDMA_SUPPORT
        soc->max_sbusdma_channels = 3;
        soc->tdma_ch = 0;
        soc->tslam_ch = 1;
        soc->desc_ch = 2;
        soc_kt2_blk_counter_config(unit);
#endif
#ifdef PLISIM
        if (SOC_IS_KATANA2(unit) && (sal_boot_flags_get() & BOOT_F_PLISIM)) {
            /* case with pcid.sim or bcm5*_bcmsim.linux */
            if (CMVEC(unit).config_var_get == NULL) { 
                LOG_CLI((BSL_META_U(unit,
                                    "Assigning dummy config var get \n"))); 
                CMVEC(unit).config_var_get = _dummy_config_var_get;
            }
        }
#endif
        si->olp_port[0] = 0;

        break;
#endif /* BCM_KATANA2_SUPPORT */

    case BCM56850_DEVICE_ID:
        if (dev_id == BCM56750_DEVICE_ID) {
            si->chip = SOC_INFO_CHIP_TITAN2;
        } else {
            si->chip = SOC_INFO_CHIP_TRIDENT2;
        }
        si->port_class_max = 128;  /* egr_vlan_xlate port class max*/
        si->modid_max = 255;    /* See SOC_MODID_MAX(unit) */
        /* si->hg_offset = 0;    SOC_HG_OFFSET */
        /*
         * 16th bit from right. See SOC_TRUNK_BIT_POS(unit)
         *     Note that it is not together with port/trunk-id.
         */
        si->trunk_bit_pos = 15;
        /* port_addr_max has to be 2^n -1 */
        si->port_addr_max = 127;
        si->num_cpu_cosq = (rev_id >= BCM56850_A1_REV_ID) ? 44 : 48;
        si->modid_count =
            soc_property_get(unit, spn_MODULE_NUM_MODIDS, 1) == 2 ? 2 : 1;
        si->modport_max = si->modid_count == 1 ? 127 : 63;
        si->num_time_interface = 1;
        si->pcie_phy_addr = 0x1da;
#ifdef BCM_SBUSDMA_SUPPORT
        soc->max_sbusdma_channels = 3;
        soc->tdma_ch = 0;
        soc->tslam_ch = 1;
        soc->desc_ch = 2;
#endif
        si->vlan_xlate_glp_wildcard = 0xffff; /* glp wildcard value */
        break;

    default:
        si->chip = 0;
        si->modid_count = 0;
        LOG_WARN(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "soc_info_config: driver device %04x unexpected\n"),
                  drv_dev_id));
        break;
    }
    
    if (si->internal_loopback) {
        SOC_FEATURE_SET(unit, soc_feature_wlan);
        SOC_FEATURE_SET(unit, soc_feature_egr_mirror_true);
        SOC_FEATURE_SET(unit, soc_feature_mim);
        SOC_FEATURE_SET(unit, soc_feature_internal_loopback);
        SOC_FEATURE_SET(unit, soc_feature_mmu_virtual_ports);
    } else {
        SOC_PBMP_CLEAR(si->lb_pbm);
    }

    /*
     * pbmp_valid is a bitmap of all ports that exist on the unit.  Any
     * port not in this bitmap is thoroughly disregarded by the driver.
     * This is useful when some switch ports are unused (e.g. the IPIC
     * for 5691).
     */
    pbmp_valid = soc_property_get_pbmp(unit, spn_PBMP_VALID, 1);
    if (soc_property_get_str(unit, spn_PBMP_GPORT_STACK) != NULL) {
        pbmp_gport_stack = soc_property_get_pbmp(unit, spn_PBMP_GPORT_STACK, 0);
    }
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_KATANA_SUPPORT)
    if(SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit)) {
        if (soc_property_get_str(unit, spn_PBMP_LOOPBACK) != NULL) {
            si->lb_pbm = soc_property_get_pbmp(unit, spn_PBMP_LOOPBACK, 0);
        }
    }
#endif
#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANAX(unit)) {
        if (soc_property_get_str(unit, spn_PBMP_EXT_MEM) != NULL) {
            si->pbm_ext_mem = soc_property_get_pbmp(unit, spn_PBMP_EXT_MEM, 0);
        }
    }
#endif

    /* Defaults for the xport settings */
    SOC_PBMP_WORD_SET(pbmp_xport_ge, 0, 0);
    SOC_PBMP_WORD_SET(pbmp_xport_xe, 0, 0);
    switch (dev_id) {
    case BCM56580_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_xport_ge, 0, 0x0000ffff);
        break;
    case BCM56800_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_xport_xe, 0, 0x000fffff);
        break;
    case BCM56801_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_xport_xe, 0, 0x000003ff);
        break;
    case BCM56820_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_xport_xe, 0, 0x01fffffe);
        break;
    case BCM56821_DEVICE_ID:
        if (!soc_property_get(unit, spn_BCM56821_20X12, 0)) {
            SOC_PBMP_WORD_SET(pbmp_xport_xe, 0, 0x001fffe0);
        }
        break;
    case BCM56822_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_xport_xe, 0, 0x01ffe000);
        break;
    case BCM56825_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_xport_xe, 0, 0x01fe01fe);
        break;
    case BCM56629_DEVICE_ID:
        if (soc_feature(unit, soc_feature_xgport_one_xe_six_ge)) {
            if (!soc_property_get(unit, spn_BCM56629_40GE, 0)) {
                SOC_PBMP_WORD_SET(pbmp_xport_xe, 0, 0x0c004004);
            }
        }
        break;
#ifdef BCM_SHADOW_SUPPORT
    case BCM88732_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_xport_xe, 0, 0x1fe);
        break;
#endif /* BCM_SHADOW_SUPPORT */
    default:
        break;
    }

    pbmp_xport_xe = soc_property_get_pbmp_default(unit, spn_PBMP_XPORT_XE,
                                 pbmp_xport_xe);
    pbmp_xport_ge = soc_property_get_pbmp_default(unit, spn_PBMP_XPORT_GE,
                                 pbmp_xport_ge);


    switch (dev_id) {
#if defined(BCM_HERCULES_SUPPORT)
    case BCM5676_DEVICE_ID:
        /* 5676 (Herc4) has only ports 1, 3, 6, and 8 (only 0x14b) */
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 2);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 4);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 5);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 7);
        break;
#endif /* BCM_HERCULES_SUPPORT */
#ifdef BCM_FIREBOLT_SUPPORT
    case BCM53301_DEVICE_ID:
        for (port = 16; port <= 27; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        break;
    case BCM53300_DEVICE_ID:
    case BCM56500_DEVICE_ID:
    case BCM56300_DEVICE_ID:
    case BCM56505_DEVICE_ID:
    case BCM56305_DEVICE_ID:
    case BCM56310_DEVICE_ID:
    case BCM56315_DEVICE_ID:
    case BCM56510_DEVICE_ID:
        /*
         * Remove all HG/XE ports
         */
        for (port = 24; port <= 27; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        break;
    case BCM56301_DEVICE_ID:    /* Helix Only */
    case BCM56306_DEVICE_ID:    /* Helix Only */
    case BCM56311_DEVICE_ID:    /* Helix 1.5 Only */
    case BCM56316_DEVICE_ID:    /* Helix 1.5 Only */
    case BCM56501_DEVICE_ID:
    case BCM56506_DEVICE_ID:
    case BCM56511_DEVICE_ID:
    case BCM56516_DEVICE_ID:
        /*
         * Remove all GE ports
         */
        for (port = 0; port <= 23; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        break;
    case BCM56502_DEVICE_ID:
    case BCM56302_DEVICE_ID:
    case BCM56507_DEVICE_ID:
    case BCM56307_DEVICE_ID:
    case BCM56100_DEVICE_ID:
    case BCM56105_DEVICE_ID:
    case BCM56110_DEVICE_ID:
    case BCM56115_DEVICE_ID:
    case BCM56312_DEVICE_ID:
    case BCM56317_DEVICE_ID:
    case BCM56512_DEVICE_ID:
    case BCM56517_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 26);
        /* passthru */
        /* coverity[fallthrough: FALSE] */
    case BCM56503_DEVICE_ID:
    case BCM56303_DEVICE_ID:
    case BCM56508_DEVICE_ID:
    case BCM56308_DEVICE_ID:
    case BCM56101_DEVICE_ID:
    case BCM56106_DEVICE_ID:
    case BCM56111_DEVICE_ID:
    case BCM56116_DEVICE_ID:
    case BCM56313_DEVICE_ID:
    case BCM56318_DEVICE_ID:
    case BCM56513_DEVICE_ID:
    case BCM56518_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 27);
        break;
    case BCM53302_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 24);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 25);
        break;
#endif /* BCM_FIREBOLT_SUPPORT */
#if defined(BCM_RAPTOR_SUPPORT)
    case BCM56218R_DEVICE_ID:
    case BCM56219R_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_valid,
                          0,
                          SOC_PBMP_WORD_GET(pbmp_valid, 0) & 0xffffffc9);
        break;

    case BCM56214R_DEVICE_ID:
    case BCM56215R_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_valid,
                          0,
                          SOC_PBMP_WORD_GET(pbmp_valid, 0) & 0xffffffc9);
        /* passthru */
        /* coverity[fallthrough: FALSE] */
    case BCM56214_DEVICE_ID:
    case BCM56215_DEVICE_ID:
    SOC_PBMP_WORD_SET(pbmp_valid, 1, 0x0);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 30);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 31);
        break;

    case BCM56216_DEVICE_ID:
    case BCM56217_DEVICE_ID:
    SOC_PBMP_WORD_SET(pbmp_valid, 1, 0x0);
        SOC_PBMP_WORD_SET(pbmp_valid,
                          0,
                          SOC_PBMP_WORD_GET(pbmp_valid, 0) & 0x3fffff);
        break;

    case BCM56212_DEVICE_ID:
    case BCM56213_DEVICE_ID:
    SOC_PBMP_WORD_SET(pbmp_valid, 1, 0x0);
        SOC_PBMP_WORD_SET(pbmp_valid,
                          0,
                          SOC_PBMP_WORD_GET(pbmp_valid, 0) & 0x3fff);
        break;

    case BCM53716_DEVICE_ID:
    SOC_PBMP_WORD_SET(pbmp_valid, 1, 0x0);
        SOC_PBMP_WORD_SET(pbmp_valid,
                          0,
                          SOC_PBMP_WORD_GET(pbmp_valid, 0) & 0x3fffc9);
        break;

    case BCM53714_DEVICE_ID:
    SOC_PBMP_WORD_SET(pbmp_valid, 1, 0x0);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 30);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 31);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 1);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 2);
    break;

    case BCM53724_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 1);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 2);
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        break;
    case BCM53726_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_valid,
                          0,
                          SOC_PBMP_WORD_GET(pbmp_valid, 0) & 0x3ffff9);
        SOC_PBMP_CLEAR(pbmp_gport_stack);
    break;

    case BCM53312_DEVICE_ID:
    case BCM53322_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_valid,
                          0,
                          SOC_PBMP_WORD_GET(pbmp_valid, 0) & 0x2223ff);
        break;

    case BCM53313_DEVICE_ID:
    case BCM53323_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_valid,
                          0,
                          SOC_PBMP_WORD_GET(pbmp_valid, 0) & 0x23ffff);
        break;

    case BCM53314_DEVICE_ID:
    case BCM53324_DEVICE_ID:
        break;

    case BCM53718_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 1);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 2);
        break;

    case BCM56014_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_valid, 1, 0x0);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 30);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 31);
        /* passthru */
        /* coverity[fallthrough: FALSE] */
    case BCM56018_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_gport_fe, 0, 0xffffffc0);
        SOC_PBMP_WORD_SET(pbmp_gport_fe, 1, 0x003fffff);
        break;

    case BCM56024_DEVICE_ID:
    case BCM56025_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_gport_fe, 0, 0xffffffc0);
        break;

    case BCM56226_DEVICE_ID:
    case BCM56227_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_valid,
                          0,
                          SOC_PBMP_WORD_GET(pbmp_valid, 0) & 0x3fffff);
        break;

    case BCM56228_DEVICE_ID:
    case BCM56229_DEVICE_ID:
        SOC_PBMP_WORD_SET(pbmp_valid,
                          0,
                          SOC_PBMP_WORD_GET(pbmp_valid, 0) & 0x3fff);
        break;

#endif /* BCM_RAPTOR_SUPPORT */
    case BCM56580_DEVICE_ID:
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x0000ffff);
        SOC_PBMP_OR(pbmp_xport_ge, pbmp_tmp);
        break;
    case BCM56701_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 14);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 15);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 16);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 17);
        /* Fall through */
    case BCM56700_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 8);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 9);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 18);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 19);
        SOC_PBMP_CLEAR(pbmp_xport_xe);
        SOC_PBMP_ASSIGN(pbmp_gx13g, pbmp_valid);
        break;
    case BCM56803_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 14);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 15);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 16);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 17);
        /* Fall through */
    case BCM56802_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 8);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 9);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 18);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 19);
        SOC_PBMP_ASSIGN(pbmp_gx13g, pbmp_valid);
        break;
    case BCM56801_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 18);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 19);
        SOC_PBMP_ASSIGN(pbmp_gx13g, pbmp_valid);
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x000ffc00);
        SOC_PBMP_AND(pbmp_gx13g, pbmp_tmp);
        break;
    case BCM56630_DEVICE_ID:
        for (port = 1; port <= 25; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        if (soc_property_get(unit, spn_BCM56630_2X12_2X24, 0)) {
            /* 28x1GE + 2x12HG + 2x24HG + loopback */
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_ASSIGN(pbmp_gx25g, pbmp_tmp);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            SOC_PBMP_ASSIGN(pbmp_gx13g, pbmp_tmp);
        } else {
            /* 24x1GE + 4x20HG + loopback */
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            if (soc_property_get(unit, spn_HIGIG_MAX_SPEED, 0) == 12000) {
                SOC_PBMP_ASSIGN(pbmp_gx12g, pbmp_tmp);
            } else {
                SOC_PBMP_ASSIGN(pbmp_gx20g, pbmp_tmp);
            }
        }
        for (port = 30; port <= 53; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_AND(pbmp_xq, pbmp_valid);
        SOC_PBMP_ASSIGN(pbmp_hyplite, pbmp_xq);
        break;
    case BCM56634_DEVICE_ID:
    /* Fall through */
    case BCM56538_DEVICE_ID:
        SOC_PBMP_CLEAR(pbmp_tmp);
        if (soc_property_get(unit, spn_BCM56634_48G_4X12, 0) || 
            soc_property_get(unit, spn_BCM56538_48G_4X12, 0)) {
            /* 48x1GE + 4x12HG + 1x12G(loopback) */
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            SOC_PBMP_ASSIGN(pbmp_gx13g, pbmp_tmp);
        } else if (soc_property_get(unit, spn_BCM56634_48G_2X24, 0) || 
                   soc_property_get(unit, spn_BCM56538_48G_2X24, 0)) {
            /* 48x1GE + 2x24HG + 1x12G(loopback) */
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_ASSIGN(pbmp_gx25g, pbmp_tmp);
            SOC_PBMP_PORT_REMOVE(pbmp_valid, 28);
            SOC_PBMP_PORT_REMOVE(pbmp_valid, 29);
        } else {
            /* 48x1GE + 4x16HG */
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            SOC_PBMP_ASSIGN(pbmp_gx16g, pbmp_tmp);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 30; port <= 53; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_AND(pbmp_xq, pbmp_valid);
        SOC_PBMP_ASSIGN(pbmp_hyplite, pbmp_xq);
        break;
    case BCM56636_DEVICE_ID:
        for (port = 14; port <= 25; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 43; port <= 49; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 51; port <= 53; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 42);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 50);
        if (soc_property_get(unit, spn_BCM56636_2X12_2X24, 0)) {
            SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);
            SOC_PBMP_PORT_REMOVE(pbmp_valid, 28);
            SOC_PBMP_PORT_REMOVE(pbmp_valid, 29);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_ASSIGN(pbmp_gx25g, pbmp_tmp);
        } else if (soc_property_get(unit, spn_BCM56636_24G_6X12, 0)) {
            SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            SOC_PBMP_ASSIGN(pbmp_gx13g, pbmp_tmp);
        } else {
            SOC_PBMP_ASSIGN(pbmp_xq12g, pbmp_tmp);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            SOC_PBMP_ASSIGN(pbmp_gx16g, pbmp_tmp);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 30; port <= 53; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_AND(pbmp_xq, pbmp_valid);
        SOC_PBMP_ASSIGN(pbmp_hyplite, pbmp_xq);

    /* Flex-port support */

    /* For each possible controlling port, ... */
    for (port = 30; port <= 50; port += 4) {
        if (!SOC_PBMP_MEMBER(pbmp_valid, port))  continue;
      
        /* Controlling port is valid port
           => Add corresponding subsidiary ports as valid
        */
      
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 1);
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 2);
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 3);
        
        if (SOC_PBMP_MEMBER(pbmp_xq10g, port)
        || SOC_PBMP_MEMBER(pbmp_xq12g, port)
        || SOC_PBMP_MEMBER(pbmp_xq13g, port)
        ) {
            /* Controlling port not in 1G mode
           => Mark corresponding subsidiary ports as disabled
        */
          
            SOC_PBMP_PORT_ADD(pbmp_disabled, port + 1);
        SOC_PBMP_PORT_ADD(pbmp_disabled, port + 2);
        SOC_PBMP_PORT_ADD(pbmp_disabled, port + 3);
        }
    }

        break;
    case BCM56638_DEVICE_ID:
        for (port = 2; port <= 25; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 31; port <= 37; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 39; port <= 41; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 43; port <= 49; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 51; port <= 53; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 30);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 38);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 42);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 50);
        if (soc_property_get(unit, spn_BCM56638_4X12_2X24, 0)) {
            SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);
            SOC_PBMP_PORT_REMOVE(pbmp_valid, 28);
            SOC_PBMP_PORT_REMOVE(pbmp_valid, 29);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_ASSIGN(pbmp_gx25g, pbmp_tmp);
        } else if (soc_property_get(unit, spn_BCM56638_8X12, 0)) {
            SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            SOC_PBMP_ASSIGN(pbmp_gx13g, pbmp_tmp);
        } else {
            SOC_PBMP_ASSIGN(pbmp_xq12g, pbmp_tmp);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            SOC_PBMP_ASSIGN(pbmp_gx16g, pbmp_tmp);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 30; port <= 53; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_AND(pbmp_xq, pbmp_valid);
        SOC_PBMP_ASSIGN(pbmp_hyplite, pbmp_xq);

    /* Flex-port support */

    /* For each possible controlling port, ... */
    for (port = 30; port <= 50; port += 4) {
        if (!SOC_PBMP_MEMBER(pbmp_valid, port))  continue;
      
        /* Controlling port is valid port
           => Add corresponding subsidiary ports as valid
        */
      
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 1);
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 2);
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 3);
        
        if (SOC_PBMP_MEMBER(pbmp_xq10g, port)
        || SOC_PBMP_MEMBER(pbmp_xq12g, port)
        || SOC_PBMP_MEMBER(pbmp_xq13g, port)
        ) {
            /* Controlling port not in 1G mode
           => Mark corresponding subsidiary ports as disabled
        */
          
            SOC_PBMP_PORT_ADD(pbmp_disabled, port + 1);
        SOC_PBMP_PORT_ADD(pbmp_disabled, port + 2);
        SOC_PBMP_PORT_ADD(pbmp_disabled, port + 3);
        }
    }

        break;

    case BCM56639_DEVICE_ID:
        for (port = 34; port <= 37; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 46; port <= 49; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 30);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 38);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 42);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 50);
        SOC_PBMP_ASSIGN(pbmp_xq10g, pbmp_tmp);
        if (soc_property_get(unit, spn_BCM56639_28G_7X10, 0)) {
            /* 28x1GE + 7x10G + loopback */
            SOC_PBMP_PORT_REMOVE(pbmp_valid, 26);
            SOC_PBMP_PORT_ADD(pbmp_valid, 46);
            SOC_PBMP_PORT_ADD(pbmp_valid, 47);
            SOC_PBMP_PORT_ADD(pbmp_valid, 48);
            SOC_PBMP_PORT_ADD(pbmp_valid, 49);
        }

        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 30; port <= 53; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_AND(pbmp_xq, pbmp_valid);
        SOC_PBMP_ASSIGN(pbmp_hyplite, pbmp_xq);

    /* Flex-port support */

    /* For each possible controlling port, ... */
    for (port = 30; port <= 50; port += 4) {
        if (!SOC_PBMP_MEMBER(pbmp_valid, port))  continue;
      
        /* Controlling port is valid port
           => Add corresponding subsidiary ports as valid
        */
      
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 1);
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 2);
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 3);
        
        if (SOC_PBMP_MEMBER(pbmp_xq10g, port)
        || SOC_PBMP_MEMBER(pbmp_xq12g, port)
        || SOC_PBMP_MEMBER(pbmp_xq13g, port)
        ) {
            /* Controlling port not in 1G mode
           => Mark corresponding subsidiary ports as disabled
        */
          
            SOC_PBMP_PORT_ADD(pbmp_disabled, port + 1);
        SOC_PBMP_PORT_ADD(pbmp_disabled, port + 2);
        SOC_PBMP_PORT_ADD(pbmp_disabled, port + 3);
        }
    }

        break;
    case BCM56689_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 1);
        /* Fall through */
    case BCM56685_DEVICE_ID:
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
        SOC_PBMP_ASSIGN(pbmp_gx12g, pbmp_tmp);
        SOC_PBMP_ASSIGN(pbmp_xq2p5g, pbmp_valid);
        SOC_PBMP_REMOVE(pbmp_xq2p5g, pbmp_gx12g);
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 30; port <= 53; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_AND(pbmp_xq, pbmp_valid);
        SOC_PBMP_ASSIGN(pbmp_hyplite, pbmp_xq);
        break;
    case BCM56520_DEVICE_ID:
        for (port = 1; port <= 29; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 30; port <= 53; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_AND(pbmp_xq, pbmp_valid);
        SOC_PBMP_ASSIGN(pbmp_hyplite, pbmp_xq);
        break;
    case BCM56521_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 1);
        for (port = 30; port <= 53; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        if (soc_property_get(unit, spn_BCM56521_2X12_2X24, 0)) {
            /* 28x1GE + 2x12HG + 2x24HG + loopback */
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_ASSIGN(pbmp_gx25g, pbmp_tmp);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            SOC_PBMP_ASSIGN(pbmp_gx13g, pbmp_tmp);
        } else {
            /* 24x1GE + 4x20HG + loopback */
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            if (soc_property_get(unit, spn_HIGIG_MAX_SPEED, 0) == 12000) {
                SOC_PBMP_ASSIGN(pbmp_gx12g, pbmp_tmp);
            } else {
                SOC_PBMP_ASSIGN(pbmp_gx20g, pbmp_tmp);
            }
        }
        break;
    case BCM56522_DEVICE_ID:
        for (port = 1; port <= 25; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 28; port <= 29; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
        if (soc_property_get(unit, spn_HIGIG_MAX_SPEED, 0) == 12000) {
            SOC_PBMP_ASSIGN(pbmp_gx12g, pbmp_tmp);
        } else {
            SOC_PBMP_ASSIGN(pbmp_gx25g, pbmp_tmp);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 30; port <= 53; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_AND(pbmp_xq, pbmp_valid);
        SOC_PBMP_ASSIGN(pbmp_hyplite, pbmp_xq);
        break;
    case BCM56524_DEVICE_ID:
    /* Fall through */
    case BCM56534_DEVICE_ID:
        for (port = 1; port <= 25; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        if (soc_property_get(unit, spn_BCM56524_2X12_2X24, 0) || 
            soc_property_get(unit, spn_BCM56534_2X12_2X24, 0)) {
            /* 28x1GE + 2x12HG + 2x24HG + loopback */
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_ASSIGN(pbmp_gx25g, pbmp_tmp);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            SOC_PBMP_ASSIGN(pbmp_gx13g, pbmp_tmp);
        } else {
            /* 24x1GE + 4x20HG + loopback */
            SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
            if (soc_property_get(unit, spn_HIGIG_MAX_SPEED, 0) == 12000) {
                SOC_PBMP_ASSIGN(pbmp_gx12g, pbmp_tmp);
            } else {
                SOC_PBMP_ASSIGN(pbmp_gx20g, pbmp_tmp);
            }
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 30; port <= 53; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_AND(pbmp_xq, pbmp_valid);
        SOC_PBMP_ASSIGN(pbmp_hyplite, pbmp_xq);
        break;
    case BCM56526_DEVICE_ID:
        for (port = 30; port <= 37; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 42; port <= 45; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 47; port <= 49; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 51; port <= 53; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 26);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 27);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 28);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 29);
        if (soc_property_get(unit, spn_BCM56526_2X12_4X16, 0)) {
            SOC_PBMP_ASSIGN(pbmp_gx16g, pbmp_tmp);
            for (port = 38; port <= 41; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 46);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 50);
            SOC_PBMP_ASSIGN(pbmp_xq12g, pbmp_tmp);
        } else {
            SOC_PBMP_ASSIGN(pbmp_gx13g, pbmp_tmp);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 46);
            SOC_PBMP_PORT_ADD(pbmp_tmp, 50);
            SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 30; port <= 53; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_AND(pbmp_xq, pbmp_valid);
        SOC_PBMP_ASSIGN(pbmp_hyplite, pbmp_xq);

    /* Flex-port support */

    /* For each possible controlling port, ... */
    for (port = 30; port <= 50; port += 4) {
        if (!SOC_PBMP_MEMBER(pbmp_valid, port))  continue;
      
        /* Controlling port is valid port
           => Add corresponding subsidiary ports as valid
        */
      
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 1);
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 2);
        SOC_PBMP_PORT_ADD(pbmp_valid, port + 3);
        
        if (SOC_PBMP_MEMBER(pbmp_xq10g, port)
        || SOC_PBMP_MEMBER(pbmp_xq12g, port)
        || SOC_PBMP_MEMBER(pbmp_xq13g, port)
        ) {
            /* Controlling port not in 1G mode
           => Mark corresponding subsidiary ports as disabled
        */
          
            SOC_PBMP_PORT_ADD(pbmp_disabled, port + 1);
        SOC_PBMP_PORT_ADD(pbmp_disabled, port + 2);
        SOC_PBMP_PORT_ADD(pbmp_disabled, port + 3);
        }
    }

        break;

#if defined(BCM_ENDURO_SUPPORT)
    case BCM56132_DEVICE_ID:
        /* 24 + 2 + 2 ports */
        for (port = 30; port <= 31; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        if(dev_id == BCM56132_DEVICE_ID) {            
            SOC_PBMP_CLEAR(pbmp_tmp);
            for (port = 2; port <= 25; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
            }
            SOC_PBMP_ASSIGN(pbmp_gport_fe, pbmp_tmp);
        }      
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 26; port <= 27; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq2p5g, pbmp_tmp);
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 28; port <= 29; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 26; port <= 29; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);

        SOC_PBMP_PORT_ADD(pbmp_tmp, 1);
        SOC_PBMP_AND(si->lb_pbm, pbmp_tmp);
        break;
    case BCM56134_DEVICE_ID:
    case BCM56331_DEVICE_ID:
        /* 24 + 4 ports */
        for (port = 30; port <= 31; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        if(dev_id == BCM56134_DEVICE_ID) {
            SOC_PBMP_CLEAR(pbmp_tmp);
            for (port = 2; port <= 25; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
            }
            SOC_PBMP_ASSIGN(pbmp_gport_fe, pbmp_tmp);
        }      
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 26; port <= 29; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq2p5g, pbmp_tmp);
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);

        SOC_PBMP_PORT_ADD(pbmp_tmp, 1);
        SOC_PBMP_AND(si->lb_pbm, pbmp_tmp);
        break;
    case BCM56334_DEVICE_ID:
    case BCM56320_DEVICE_ID:
    case BCM56321_DEVICE_ID:
        /* 24 + 4 ports */
        for (port = 30; port <= 31; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 26; port <= 29; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);

        SOC_PBMP_PORT_ADD(pbmp_tmp, 1);
        SOC_PBMP_AND(si->lb_pbm, pbmp_tmp);
        break;
    case BCM56333_DEVICE_ID:
        /* 16 ports */
        for (port = 18; port <= 31; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 1);
        SOC_PBMP_AND(si->lb_pbm, pbmp_tmp);
        break;
    case BCM56338_DEVICE_ID:
        /* 8 + 2 ports */
        for (port = 10; port <= 25; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 28; port <= 31; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 26; port <= 27; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);

        SOC_PBMP_PORT_ADD(pbmp_tmp, 1);
        SOC_PBMP_AND(si->lb_pbm, pbmp_tmp);
        break;
    case BCM56230_DEVICE_ID:
        /* 12 ports */
        for (port = 14; port <= 31; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 1);
        SOC_PBMP_AND(si->lb_pbm, pbmp_tmp);
        break;
    case BCM56231_DEVICE_ID:
        /* 6 ports */
        for (port = 8; port <= 31; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_PORT_ADD(pbmp_tmp, 1);
        SOC_PBMP_AND(si->lb_pbm, pbmp_tmp);
        break;
#endif /* BCM_ENDURO_SUPPORT */

#if defined(BCM_HURRICANE2_SUPPORT)
    case BCM56150_DEVICE_ID:
    case BCM56151_DEVICE_ID:
    case BCM56152_DEVICE_ID:
    case BCM53342_DEVICE_ID:
    case BCM53343_DEVICE_ID:
    case BCM53344_DEVICE_ID:
    case BCM53346_DEVICE_ID:
    case BCM53347_DEVICE_ID:
    case BCM53333_DEVICE_ID:
    case BCM53334_DEVICE_ID:
    case BCM53393_DEVICE_ID:
    case BCM53394_DEVICE_ID:
        SOC_FEATURE_SET(unit, soc_feature_logical_port_num);
        if (soc_hu2_port_config_init(unit, dev_id) < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Port config error !!\n")));
            return SOC_E_CONFIG;
        }
        SOC_PBMP_CLEAR(si->lb_pbm);
        break;
#endif /* BCM_HURRICANE2_SUPPORT */

#if defined(BCM_GREYHOUND_SUPPORT)
    case BCM53400_DEVICE_ID:
    case BCM53401_DEVICE_ID:
    case BCM53402_DEVICE_ID:
    case BCM53403_DEVICE_ID:
    case BCM53404_DEVICE_ID:
    case BCM53405_DEVICE_ID:
    case BCM53406_DEVICE_ID:
    case BCM53408_DEVICE_ID:
    case BCM56060_DEVICE_ID:
    case BCM56062_DEVICE_ID:
    case BCM56063_DEVICE_ID:        
    case BCM56064_DEVICE_ID:        
        SOC_FEATURE_SET(unit, soc_feature_logical_port_num);
        if (soc_greyhound_port_config_init(unit, dev_id) < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Port config error !!")));
            return SOC_E_CONFIG;
        }
        SOC_PBMP_CLEAR(si->lb_pbm);
        break;
#endif /* BCM_GREYHOUND_SUPPORT */
#if defined(BCM_HURRICANE_SUPPORT)
    case BCM56140_DEVICE_ID:
        if (!soc_property_get(unit, spn_BCM5614X_CONFIG, 0)) {
            for (port = 2; port <= 25; port++) {
                if ((port-2)%4) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
            }
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 26; port <= 29; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
            si->port_speed_max[port] = 2500;
        }
        if (!SOC_PBMP_MEMBER(pbmp_valid, 27)) {
            /* Make Port 26 as HG[13] */
            SOC_PBMP_PORT_REMOVE(pbmp_tmp, 26);
            SOC_PBMP_PORT_REMOVE(pbmp_tmp, 27);
            SOC_PBMP_PORT_ADD(pbmp_xq13g, 26);
            si->port_speed_max[26] = 13000;
        }
        if (!SOC_PBMP_MEMBER(pbmp_valid, 29)) {
            /* Make Port 28 as HG[13] */
            SOC_PBMP_PORT_REMOVE(pbmp_tmp, 28);
            SOC_PBMP_PORT_REMOVE(pbmp_tmp, 29);
            SOC_PBMP_PORT_ADD(pbmp_xq13g, 28);
            si->port_speed_max[28] = 13000;
        }
        SOC_PBMP_ASSIGN(pbmp_xq2p5g, pbmp_tmp);
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_OR(pbmp_xq, pbmp_xq13g);
        SOC_PBMP_CLEAR(si->lb_pbm);
        break;
    case BCM56142_DEVICE_ID:
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 26; port <= 29; port++) {
            /* Full Capacity (24x1G + 4xHGd) for testing */
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        if (soc_property_get(unit, spn_BCM5614X_CONFIG, 1) == 2) {
              /* 24x1G + 2HG */
            SOC_PBMP_PORT_REMOVE(pbmp_valid, 27);
            SOC_PBMP_PORT_REMOVE(pbmp_valid, 29);
            SOC_PBMP_PORT_REMOVE(pbmp_tmp, 27);
            SOC_PBMP_PORT_REMOVE(pbmp_tmp, 29);
        } else if (soc_property_get(unit, spn_BCM5614X_CONFIG, 1) != 0) {
             /* 24x1G + 1xHG + 2xHGd (dxgxs=1) or
                24x1G + 2xHGd + 1xHG (dxgxs=0)*/
            SOC_PBMP_PORT_REMOVE(pbmp_valid, dxgxs ? 27 : 29);
            SOC_PBMP_PORT_REMOVE(pbmp_tmp, dxgxs ? 27 : 29);
        }
        
        SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);

        /* For HGd ports, 12.768 GB is set as 12G */
        if (dxgxs) {
            si->port_speed_max[28] = 12000;
            si->port_speed_max[29] = 12000;
        } else {
            si->port_speed_max[26] = 12000;
            si->port_speed_max[27] = 12000;
        }

        if (soc_property_get(unit, spn_BCM5614X_CONFIG, 1) == 2) {
            if (SOC_PBMP_MEMBER(pbmp_valid, 26)) {
                si->port_speed_max[26] = 13000;
            }
            if (SOC_PBMP_MEMBER(pbmp_valid, 28)) {
                si->port_speed_max[28] = 13000;
            }
        } else {
            if (SOC_PBMP_MEMBER(pbmp_valid, 27)) {
                si->port_speed_max[26] = 12000;
                si->port_speed_max[27] = 12000;
            } else {
                si->port_speed_max[26] = 13000;
            }
            if (SOC_PBMP_MEMBER(pbmp_valid, 29)) {
                si->port_speed_max[28] = 12000;
                si->port_speed_max[29] = 12000;
            } else {
                si->port_speed_max[28] = 13000;
            }
        }

        for (port = 30; port <= 31; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(si->lb_pbm);
        break;
    case BCM56143_DEVICE_ID:
        if (!soc_property_get(unit, spn_BCM5614X_CONFIG, 0)) {
            /* 24x1G + 2xHGdG + 2x2.5G (dxgxs=1) or
               24x1G + 2x2.5G + 2xHGd (dxgxs=0) */
            SOC_PBMP_CLEAR(pbmp_tmp);
            for (port = 26; port <= 27; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
            }
            if(dxgxs) {
                SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);
            } else {
                SOC_PBMP_ASSIGN(pbmp_xq2p5g, pbmp_tmp);
            }
            SOC_PBMP_CLEAR(pbmp_tmp);
            for (port = 28; port <= 29; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
            }
            if(dxgxs) {
                SOC_PBMP_ASSIGN(pbmp_xq2p5g, pbmp_tmp);
            } else {
                SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);
            }
            SOC_PBMP_CLEAR(pbmp_tmp);
            for (port = 26; port <= 29; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
            }
            SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);

            for (port = 30; port <= 31; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PBMP_CLEAR(si->lb_pbm);
        } else {
            /* 24x1G + 4xG/4x2.5G */
            SOC_PBMP_CLEAR(pbmp_tmp);
            for (port = 26; port <= 29; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
            }
            SOC_PBMP_ASSIGN(pbmp_xq2p5g, pbmp_tmp);
            SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
            si->port_speed_max[26] = 2500;
            si->port_speed_max[27] = 2500;
            si->port_speed_max[28] = 2500;
            si->port_speed_max[29] = 2500;
        }
        break;
    case BCM56144_DEVICE_ID:
        /* 16x1G + 2x2.5G + 2x2.5G */
        for (port = 18; port <= 25; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 26; port <= 29; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq2p5g, pbmp_tmp);
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_CLEAR(si->lb_pbm);
        si->port_speed_max[26] = 2500;
        si->port_speed_max[27] = 2500;
        si->port_speed_max[28] = 2500;
        si->port_speed_max[29] = 2500;
        break;
    case BCM56146_DEVICE_ID:
        /* 24xFE + 2x2.5G + 2x2.5G */
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 2; port <= 25; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
            si->port_speed_max[port] = 100;
        }
        SOC_PBMP_ASSIGN(pbmp_gport_fe, pbmp_tmp);

        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 26; port <= 29; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
            si->port_speed_max[port] = 2500;
        }
        SOC_PBMP_ASSIGN(pbmp_xq2p5g, pbmp_tmp);
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_CLEAR(si->lb_pbm);
        break;
    case BCM56147_DEVICE_ID:
        /* 24xFE + 1xHG + 2x2.5G */
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 2; port <= 25; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
            si->port_speed_max[port] = 100;
        }
        SOC_PBMP_ASSIGN(pbmp_gport_fe, pbmp_tmp);

        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_CLEAR(pbmp_xq13g);
        for (port = 26; port <= 29; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
            si->port_speed_max[port] = 2500;
        }
        if (!soc_property_get(unit, spn_BCM5614X_CONFIG, 0)) {
            if (dxgxs) {
                /* Make Port 26 as HG[13] */
                SOC_PBMP_PORT_REMOVE(pbmp_tmp, 26);
                SOC_PBMP_PORT_REMOVE(pbmp_tmp, 27);
                SOC_PBMP_PORT_REMOVE(pbmp_valid, 27);
                SOC_PBMP_PORT_ADD(pbmp_xq13g, 26);
                si->port_speed_max[26] = 13000;
            } else {
                /* Make Port 28 as HG[13] */
                SOC_PBMP_PORT_REMOVE(pbmp_tmp, 28);
                SOC_PBMP_PORT_REMOVE(pbmp_tmp, 29);
                SOC_PBMP_PORT_REMOVE(pbmp_valid, 29);
                SOC_PBMP_PORT_ADD(pbmp_xq13g, 28);
                si->port_speed_max[28] = 13000;
            }
        }
        SOC_PBMP_ASSIGN(pbmp_xq2p5g, pbmp_tmp);
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_OR(pbmp_xq, pbmp_xq13g);
        SOC_PBMP_CLEAR(si->lb_pbm);
        break;
    case BCM56149_DEVICE_ID:
        /* 24x1G + 2xXAUI + 2xHGd  or
           24x1G + 2xHgd + 2xXAUI */                 
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 26; port <= 29; port++) {
            SOC_PBMP_PORT_ADD(pbmp_tmp, port);
        }
        SOC_PBMP_ASSIGN(pbmp_xq, pbmp_tmp);
        SOC_PBMP_ASSIGN(pbmp_xq13g, pbmp_tmp);

        SOC_PBMP_CLEAR(pbmp_tmp);
        if (dxgxs) {
            /* 24x1G + 2xHgd + 2xXAUI */
            if (!SOC_PBMP_MEMBER(pbmp_xport_xe, 26) &&
                !SOC_PBMP_MEMBER(pbmp_xport_xe, 27)) {
                si->port_speed_max[26] = 12000;
                si->port_speed_max[27] = 12000;
                SOC_PBMP_WORD_SET(pbmp_gport_stack, 0, 0x0c000000);
            }
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 28);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 29);
        } else {
            /* 24x1G + 2xXAUI + 2xHGd */
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 26);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 27);
            if (!SOC_PBMP_MEMBER(pbmp_xport_xe, 28) &&
                !SOC_PBMP_MEMBER(pbmp_xport_xe, 29)) {
                si->port_speed_max[28] = 12000;
                si->port_speed_max[29] = 12000;
                SOC_PBMP_WORD_SET(pbmp_gport_stack, 0, 0x30000000);
            }
        }

        for (port = 30; port <= 31; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(si->lb_pbm);
        break;
#endif /* BCM_HURRICANE_SUPPORT */

#ifdef BCM_TRIUMPH_SUPPORT
    case BCM56624_DEVICE_ID:
        SOC_PORT_SPEED_MAX_SET(unit, 28, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 29, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 30, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 31, 16000);
        for (port = 2; port <= 7; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 14; port <= 19; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 26; port <= 27; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 32; port <= 36; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 43; port <= 47; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        break;
    case BCM56626_DEVICE_ID:
        for (port = 32; port <= 53; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PORT_SPEED_MAX_SET(unit, 26, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 27, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 28, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 29, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 30, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 31, 16000);
        for (port = 2; port <= 7; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 14; port <= 19; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 26; port <= 27; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        break;
    case BCM56628_DEVICE_ID:
        for (port = 3; port <= 13; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 15; port <= 25; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 32; port <= 53; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PORT_SPEED_MAX_SET(unit, 2, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 14, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 26, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 27, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 28, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 29, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 30, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 31, 16000);
        for (port = 14; port <= 19; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 26; port <= 27; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        break;
    case BCM56629_DEVICE_ID:
        if (soc_feature(unit, soc_feature_xgport_one_xe_six_ge)) {
            if (soc_property_get(unit, spn_BCM56629_40GE, 0)) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, 6);
                SOC_PBMP_PORT_REMOVE(pbmp_valid, 7);
                SOC_PBMP_PORT_REMOVE(pbmp_valid, 18);
                SOC_PBMP_PORT_REMOVE(pbmp_valid, 19);
                SOC_PBMP_PORT_REMOVE(pbmp_valid, 35);
                SOC_PBMP_PORT_REMOVE(pbmp_valid, 36);
                SOC_PBMP_PORT_REMOVE(pbmp_valid, 46);
                SOC_PBMP_PORT_REMOVE(pbmp_valid, 47);
            } else {
                for (port = 3; port <= 7; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                for (port = 15; port <= 19; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                for (port = 32; port <= 36; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                for (port = 43; port <= 47; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                SOC_PORT_SPEED_MAX_SET(unit, 2, 10000);
                SOC_PORT_SPEED_MAX_SET(unit, 14, 10000);
                SOC_PORT_SPEED_MAX_SET(unit, 26, 10000);
                SOC_PORT_SPEED_MAX_SET(unit, 27, 10000);
            }
        } else {
            for (port = 32; port <= 53; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PORT_SPEED_MAX_SET(unit, 26, 12000);
            SOC_PORT_SPEED_MAX_SET(unit, 27, 12000);
        }
        for (port = 2; port <= 7; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 14; port <= 19; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 26; port <= 27; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 32; port <= 36; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 43; port <= 47; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        SOC_PORT_SPEED_MAX_SET(unit, 28, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 29, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 30, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 31, 12000);
        break;
    case BCM56620_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 1);
        SOC_PORT_SPEED_MAX_SET(unit, 28, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 29, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 30, 16000);
        SOC_PORT_SPEED_MAX_SET(unit, 31, 16000);
        for (port = 2; port <= 7; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 14; port <= 19; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 26; port <= 27; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 32; port <= 36; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 43; port <= 47; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        break;
    case BCM56684_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 1);
        /* Fall through */
    case BCM56680_DEVICE_ID:
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PORT_SPEED_MAX_SET(unit, 28, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 29, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 30, 12000);
        SOC_PORT_SPEED_MAX_SET(unit, 31, 12000);
        SOC_PBMP_ASSIGN(pbmp_xg2p5g, pbmp_valid);
        SOC_PBMP_PORT_REMOVE(pbmp_xg2p5g, 28);
        SOC_PBMP_PORT_REMOVE(pbmp_xg2p5g, 29);
        SOC_PBMP_PORT_REMOVE(pbmp_xg2p5g, 30);
        SOC_PBMP_PORT_REMOVE(pbmp_xg2p5g, 31);
        for (port = 2; port <= 7; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 14; port <= 19; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 26; port <= 27; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 32; port <= 36; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 43; port <= 47; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        break;
    case BCM56686_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 1);
        for (port = 3; port <= 13; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 14; port <= 26; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 32; port <= 36; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 43; port <= 47; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PORT_SPEED_MAX_SET(unit, 2, 10000);
        SOC_PORT_SPEED_MAX_SET(unit, 27, 10000);
        for (port = 2; port <= 7; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        for (port = 26; port <= 27; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        break;
#endif /* BCM_TRIUMPH_SUPPORT */
    case BCM56820_DEVICE_ID:
        for (port = 9; port <= 16; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        break;
    case BCM56821_DEVICE_ID:
        for (port = 11; port <= 14; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        if (soc_property_get(unit, spn_BCM56821_20X12, 0)) {
            SOC_PBMP_ASSIGN(pbmp_gx12g, pbmp_valid);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x01ff87fe);
            SOC_PBMP_ASSIGN(pbmp_gx12g, pbmp_tmp);
        } else {
            SOC_PBMP_ASSIGN(pbmp_gx16g, pbmp_valid);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x01e0001e);
            SOC_PBMP_AND(pbmp_gx16g, pbmp_tmp);
        }
        for (port = 9; port <= 16; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        break;
    case BCM56822_DEVICE_ID:
        for (port = 1; port <= 4; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        if (soc_property_get(unit, spn_BCM56822_8X16, 0)) {
            SOC_PBMP_ASSIGN(pbmp_gx16g, pbmp_valid);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x00001fe0);
            SOC_PBMP_AND(pbmp_gx16g, pbmp_tmp);
        } else {
            for (port = 5; port <= 6; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PBMP_ASSIGN(pbmp_gx21g, pbmp_valid);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x00001e00);
            SOC_PBMP_AND(pbmp_gx21g, pbmp_tmp);
            SOC_PBMP_ASSIGN(pbmp_gx16g, pbmp_valid);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x00000180);
            SOC_PBMP_AND(pbmp_gx16g, pbmp_tmp);
        }
        for (port = 9; port <= 16; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        break;
    case BCM56725_DEVICE_ID:
        for (port = 25; port <= 28; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        if (soc_property_get(unit, spn_BCM56725_16X16, 0)) {
            for (port = 1; port <= 4; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            for (port = 21; port <= 24; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PBMP_ASSIGN(pbmp_gx16g, pbmp_valid);
            SOC_PBMP_CLEAR(pbmp_tmp);
            SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x001fffe0);
            SOC_PBMP_AND(pbmp_gx16g, pbmp_tmp);
            break;
        }
        /* Fall through */
    case BCM56823_DEVICE_ID:
        for (port = 1; port <= 6; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 19; port <= 24; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_ASSIGN(pbmp_gx21g, pbmp_valid);
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x0001fe00);
        SOC_PBMP_AND(pbmp_gx21g, pbmp_tmp);
        SOC_PBMP_ASSIGN(pbmp_gx16g, pbmp_valid);
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x00060180);
        SOC_PBMP_AND(pbmp_gx16g, pbmp_tmp);
        for (port = 9; port <= 16; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        break;
    case BCM56825_DEVICE_ID:
        for (port = 26; port <= 28; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_ASSIGN(pbmp_gx21g, pbmp_valid);
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x0001fe00);
        SOC_PBMP_AND(pbmp_gx21g, pbmp_tmp);
        for (port = 9; port <= 16; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        break;
    case BCM56721_DEVICE_ID:
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 5);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 6);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 19);
        SOC_PBMP_PORT_REMOVE(pbmp_valid, 20);
        /* Fall through */
    case BCM56720_DEVICE_ID:
        for (port = 1; port <= 4; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 21; port <= 28; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_ASSIGN(pbmp_gx16g, pbmp_valid);
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_WORD_SET(pbmp_tmp, 0, 0x001fffe0);
        SOC_PBMP_AND(pbmp_gx16g, pbmp_tmp);
        for (port = 9; port <= 16; port++) {
            SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
        }
        SOC_PBMP_AND(pbmp_hyplite, pbmp_valid);
        break;
#ifdef BCM_TRIDENT_SUPPORT
    case BCM56840_DEVICE_ID:
    case BCM56841_DEVICE_ID:
    case BCM56843_DEVICE_ID:
    case BCM56845_DEVICE_ID:
    case BCM56847_DEVICE_ID:        
    case BCM56743_DEVICE_ID:
    case BCM56745_DEVICE_ID:
    case BCM56842_DEVICE_ID:
    case BCM56844_DEVICE_ID:
    case BCM56846_DEVICE_ID:
    case BCM56549_DEVICE_ID:
    case BCM56053_DEVICE_ID:
    case BCM56831_DEVICE_ID:
    case BCM56835_DEVICE_ID:
    case BCM56838_DEVICE_ID:
    case BCM56849_DEVICE_ID:        
    case BCM56744_DEVICE_ID:
    case BCM56746_DEVICE_ID:
        SOC_FEATURE_SET(unit, soc_feature_logical_port_num);
        rv = soc_trident_port_config_init(unit, dev_id);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Port config error !!\n")));
            return SOC_E_CONFIG;
        }
        for (port = 66; port <= 73; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        si->cpu_hg_index = 66;
        break;
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
    case BCM56850_DEVICE_ID:
    case BCM56851_DEVICE_ID:
    case BCM56852_DEVICE_ID:
    case BCM56853_DEVICE_ID:
    case BCM56854_DEVICE_ID:
    case BCM56855_DEVICE_ID:
    case BCM56834_DEVICE_ID:          
    case BCM56750_DEVICE_ID:
    case BCM56830_DEVICE_ID:
        SOC_FEATURE_SET(unit, soc_feature_logical_port_num);
        if (rev_id >= BCM56850_A1_REV_ID) {
            SOC_FEATURE_SET(unit, soc_feature_cmic_reserved_queues);
        }
        rv = soc_trident2_port_config_init(unit, dev_id, rev_id);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Port config error !!\n")));
        }
        for (port = 108; port <= 129; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        si->cpu_hg_index = 106;
        if (dev_id == BCM56830_DEVICE_ID) {
            SOC_PBMP_ASSIGN(pbmp_xport_xe,pbmp_valid);
        }
        /* Flex counter pools */
        si->num_flex_ingress_pools = 8;
        si->num_flex_egress_pools = 4;
        si->size_flex_ingress_pool = 4096;
        si->size_flex_egress_pool = 4096;
        break;
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
    case BCM88732_DEVICE_ID:

        if (soc_property_get(unit, spn_BCM88732_2X40_1X40, 0)) {
            /* 2x40G port-link + 1x40G interlaken */
            SOC_PBMP_PORT_ADD(pbmp_xl, 1);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 1);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 1);
            for (port = 2; port <= 4; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PBMP_PORT_ADD(pbmp_xl, 5);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 5);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 5);
            for (port = 6; port <= 8; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PBMP_PORT_ADD(pbmp_xl, 9);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 9);
            SOC_PBMP_CLEAR(pbmp_tmp);
            for (port = 10; port <= 16; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
            }
            SOC_PBMP_ASSIGN(pbmp_disabled, pbmp_tmp);
        } else if (soc_property_get(unit, spn_BCM88732_2X40_2X40, 0)) {
            /* 2x40G port-link + 2x40G interlaken */
            SOC_PBMP_PORT_ADD(pbmp_xl, 1);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 1);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 1);
            for (port = 2; port <= 4; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PBMP_PORT_ADD(pbmp_xl, 5);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 5);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 5);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 5);
            for (port = 6; port <= 8; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PBMP_PORT_ADD(pbmp_xl, 9);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 9);
            SOC_PBMP_CLEAR(pbmp_tmp);
            for (port = 10; port <= 12; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
            }
            SOC_PBMP_PORT_ADD(pbmp_xl, 13);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 13);
            for (port = 14; port <= 16; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
            }
            /* SOC_PBMP_ASSIGN(pbmp_disabled, pbmp_tmp); */
        } else if (soc_property_get(unit, spn_BCM88732_2X40_2X40E, 0)) {
            /* 2x40G port-link + 2x40G Ethernet */
            SOC_PBMP_PORT_ADD(pbmp_xl, 1);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 1);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 1);
            for (port = 2; port <= 4; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PBMP_PORT_ADD(pbmp_xl, 5);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 5);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 5);
            for (port = 6; port <= 8; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PBMP_PORT_ADD(pbmp_xl, 9);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 9);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 9);
            for (port = 10; port <= 12; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
            SOC_PBMP_PORT_ADD(pbmp_xl, 13);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 13);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 13);
            for (port = 14; port <= 16; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
        } else if (soc_property_get(unit, spn_BCM88732_2X40_8X10, 0)) {
            /* 2x40G port-link + 8x10G XFI */
            SOC_PBMP_PORT_ADD(pbmp_xl, 1);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 1);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 1);

            /* Mark ports as disabled*/
            SOC_PBMP_CLEAR(pbmp_tmp);
            for (port = 2; port <= 4; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
                SOC_PBMP_PORT_ADD(pbmp_xl40g, 1);
            }
            for (port = 6; port <= 8; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
            }
            SOC_PBMP_ASSIGN(pbmp_disabled, pbmp_tmp);


#if 0
            for (port = 2; port <= 4; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
#endif
            SOC_PBMP_PORT_ADD(pbmp_xl, 5);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 5);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 5);
#if 0
            for (port = 6; port <= 8; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
#endif
            for (port = 9; port <= 16; port++) {
                SOC_PBMP_PORT_ADD(pbmp_xport_xe, port);
                SOC_PBMP_PORT_ADD(pbmp_xl10g, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
            }
        } else if (soc_property_get(unit, spn_BCM88732_8X10_8X10, 0)) {
            /* 8x10G port-link + 8x10G XFI */
            for (port = 1; port <= 16; port++) {
                SOC_PBMP_PORT_ADD(pbmp_xport_xe, port);
                SOC_PBMP_PORT_ADD(pbmp_xl10g, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
            }
        } else if (soc_property_get(unit, spn_BCM88732_8X10_1X40, 0)) {
            /* 8x10G port-link + 1x40G interlaken */
            for (port = 1; port <= 8; port++) {
                SOC_PBMP_PORT_ADD(pbmp_xport_xe, port);
                SOC_PBMP_PORT_ADD(pbmp_xl10g, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
            }
            SOC_PBMP_PORT_ADD(pbmp_xl, 9);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 9);
            SOC_PBMP_CLEAR(pbmp_tmp);
            for (port = 10; port <= 16; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
            }
            SOC_PBMP_ASSIGN(pbmp_disabled, pbmp_tmp);
        } else if (soc_property_get(unit, spn_BCM88732_1X40_4X10, 0)) {
            /* TDM 18 */
            /* 1x40G port-link 4X10G XAUI Switch link*/
            SOC_PBMP_PORT_ADD(pbmp_xl, 1);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 1);
            SOC_PBMP_PORT_ADD(pbmp_xport_xe, 1);
            for (port = 2; port <= 8; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }

            /* Switch Panel ports */
            for (port = 9; port < 16; port+=2) {
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
                SOC_PBMP_PORT_ADD(pbmp_xport_xe, port);
                SOC_PBMP_PORT_ADD(pbmp_xl10g, port);
            } 
            /* Remove the even ports from the valid ports */
            for (port = 10; port <= 16; port+=2) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            } 

        } else if (soc_property_get(unit, spn_BCM88732_4X10_4X10, 0)) {
            /* TDM 19 */
            for (port = 1; port <= 4; port++) {
                SOC_PBMP_PORT_ADD(pbmp_xport_xe, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
                SOC_PBMP_PORT_ADD(pbmp_xl10g, port);
            }
            for (port = 5; port <= 8; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }

            /* Switch Panel ports */
            for (port = 9; port < 16; port+=2) {
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
                SOC_PBMP_PORT_ADD(pbmp_xport_xe, port);
                SOC_PBMP_PORT_ADD(pbmp_xl10g, port);
            } 
            /* Remove the even ports from the valid ports */
            for (port = 10; port <= 16; port+=2) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            } 
        } else {
            /* 8x10G port-link + 2x40G interlaken */
            for (port = 1; port <= 8; port++) {
                SOC_PBMP_PORT_ADD(pbmp_xport_xe, port);
                SOC_PBMP_PORT_ADD(pbmp_xl10g, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
            }
            SOC_PBMP_PORT_ADD(pbmp_xl, 9);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 9);
            SOC_PBMP_CLEAR(pbmp_tmp);
            for (port = 10; port <= 12; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
            }
            SOC_PBMP_PORT_ADD(pbmp_xl, 13);
            SOC_PBMP_PORT_ADD(pbmp_xl40g, 13);
            for (port = 14; port <= 16; port++) {
                SOC_PBMP_PORT_ADD(pbmp_tmp, port);
                SOC_PBMP_PORT_ADD(pbmp_xl, port);
            }
            /* SOC_PBMP_ASSIGN(pbmp_disabled, pbmp_tmp); */
        }
        break;
#endif /* BCM_SHADOW_SUPPORT */

#ifdef BCM_HELIX4_SUPPORT
    case BCM56548_DEVICE_ID:
    case BCM56547_DEVICE_ID:
    case BCM56347_DEVICE_ID:
    case BCM56346_DEVICE_ID:
    case BCM56344_DEVICE_ID:
    case BCM56342_DEVICE_ID:
    case BCM56340_DEVICE_ID:
    case BCM56049_DEVICE_ID:
    case BCM56048_DEVICE_ID:
    case BCM56047_DEVICE_ID:
    case BCM56042_DEVICE_ID:
    case BCM56041_DEVICE_ID:
    case BCM56040_DEVICE_ID:

        SOC_FEATURE_SET(unit, soc_feature_logical_port_num);
        rv = soc_tr3_port_config_init(unit, dev_id);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Port config error !!\n")));
            return SOC_E_CONFIG;
        }
        for (port = 62; port <= 66; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 58; port < 62; port++) {
            SOC_PBMP_PORT_ADD(si->lb_pbm, port);
        }
        
        AXP_PORT(unit, SOC_AXP_NLF_PASSTHRU) = si->lb_port = 61;
        AXP_PORT(unit, SOC_AXP_NLF_SM) = 60;
        AXP_PORT(unit, SOC_AXP_NLF_WLAN_ENCAP) = 58;
        AXP_PORT(unit, SOC_AXP_NLF_WLAN_DECAP) = 59;
        si->cpu_hg_index = 63;
        si->num_flex_ingress_pools = 8; 
        si->num_flex_egress_pools = 8;
        si->size_flex_ingress_pool = 1024;
        si->size_flex_egress_pool = 1024;
        si->global_meter_action_size = 8192;
        break;
#endif

#ifdef BCM_TRIUMPH3_SUPPORT
    case BCM56640_DEVICE_ID:
    case BCM56643_DEVICE_ID:
    case BCM56644_DEVICE_ID:
    case BCM56648_DEVICE_ID:
    case BCM56649_DEVICE_ID:
    case BCM56540_DEVICE_ID:
    case BCM56541_DEVICE_ID:
    case BCM56542_DEVICE_ID:
    case BCM56543_DEVICE_ID:
    case BCM56544_DEVICE_ID:
    case BCM56545_DEVICE_ID:
    case BCM56546_DEVICE_ID:
    case BCM56044_DEVICE_ID:
    case BCM56045_DEVICE_ID:
    case BCM56046_DEVICE_ID:
        SOC_FEATURE_SET(unit, soc_feature_logical_port_num);
        rv = soc_tr3_port_config_init(unit, dev_id);
        SOC_PBMP_CLEAR(pbmp_xport_xe);
        for (port = 0; port <= 57; port++) {
            if (10000 == si->port_speed_max[port]) {
                SOC_PBMP_PORT_ADD(pbmp_xport_xe, port);
            }
        }
        pbmp_xport_xe = soc_property_get_pbmp_default(unit,
                                                      spn_PBMP_XPORT_XE,
                                                      pbmp_xport_xe);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Port config error !!\n")));
            return SOC_E_CONFIG;
        }
        for (port = 62; port <= 66; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 58; port < 62; port++) {
            SOC_PBMP_PORT_ADD(si->lb_pbm, port);
            SOC_PBMP_PORT_ADD(pbmp_valid, port);
        }
        
        AXP_PORT(unit, SOC_AXP_NLF_PASSTHRU) = si->lb_port = 61;
        AXP_PORT(unit, SOC_AXP_NLF_SM) = 60;
        AXP_PORT(unit, SOC_AXP_NLF_WLAN_ENCAP) = 58;
        AXP_PORT(unit, SOC_AXP_NLF_WLAN_DECAP) = 59;
        si->cpu_hg_index = 63;
        si->num_flex_ingress_pools = 16;
        si->size_flex_ingress_pool = 8192;
        si->num_flex_egress_pools = 8;
        si->size_flex_egress_pool = 2048;
        if (dev_id == BCM56540_DEVICE_ID ||
            dev_id == BCM56541_DEVICE_ID ||
            dev_id == BCM56542_DEVICE_ID ||
            dev_id == BCM56543_DEVICE_ID ||
            dev_id == BCM56544_DEVICE_ID ||
            dev_id == BCM56545_DEVICE_ID ||
            dev_id == BCM56546_DEVICE_ID ||
            dev_id == BCM56044_DEVICE_ID ||
            dev_id == BCM56045_DEVICE_ID ||
            dev_id == BCM56046_DEVICE_ID) {
            si->num_flex_ingress_pools = 8;
            si->size_flex_ingress_pool = 2048;
            if (dev_id == BCM56044_DEVICE_ID ||
                dev_id == BCM56045_DEVICE_ID ||
                dev_id == BCM56046_DEVICE_ID) {
                si->size_flex_egress_pool = 64;
                si->size_flex_ingress_pool = 256;
            }
        }
        break;
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    case BCM56440_DEVICE_ID:
    case BCM55440_DEVICE_ID:
    case BCM55441_DEVICE_ID:    
    case BCM56445_DEVICE_ID:
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        SOC_PBMP_CLEAR(pbmp_mxq13g);
        SOC_PBMP_CLEAR(pbmp_mxq2p5g);
        SOC_PBMP_CLEAR(pbmp_tmp);
        switch (soc_property_get(unit, spn_BCM5644X_CONFIG, 0)) {
            case 1: /* 24x1G + 2xHG + 8x2.5GE */
                for (port = 25; port <= 26; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq13g, port);
                    SOC_PBMP_PORT_ADD(pbmp_tmp, port);
                }
                for (port = 27; port <= 34; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                    SOC_PBMP_PORT_ADD(pbmp_tmp, port);
                }
                SOC_PBMP_ASSIGN(pbmp_mxq, pbmp_tmp);
                break;

            case 2: /* 24x1G + 4x2.5HGL */
                for (port = 25; port <= 28; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                    SOC_PBMP_PORT_ADD(pbmp_gport_stack, port);
                }
                for (port = 29; port <= 34; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                break;
            case 3: /* 24x1G + 3xHG + 4x2.5GE */
                for (port = 25; port <= 27; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq13g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                }
                for (port = 28; port <= 31; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                }
                for (port = 32; port <= 34; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                break;

            case 4: /* 24x1G + 2xHG + 4x2.5GE + 1xHG*/
                for (port = 25; port <= 26; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq13g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                }
                SOC_PBMP_PORT_ADD(pbmp_mxq13g, 28);
                SOC_PBMP_PORT_ADD(pbmp_mxq, 28);
                for (port = 29; port <= 31; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, 27);
                SOC_PBMP_PORT_ADD(pbmp_mxq, 27);
                for (port = 32; port <= 34; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                }
                break;


            case 0: /* 24x1G + 4xHG */
            default:
                for (port = 25; port <= 28; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq13g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                    SOC_PBMP_PORT_ADD(pbmp_gport_stack, port);
                }
                for (port = 29; port <= 34; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                break;
        }
        for (port = 36; port <= 38; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        if (drv_dev_id == BCM55440_DEVICE_ID) {
            /* Only 17 GE ports */
            for (port = 18; port <= 24; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
        }
        SOC_PBMP_CLEAR(si->tdm_pbm);
        SOC_PBMP_CLEAR(si->lb_pbm);
        SOC_PBMP_PORT_ADD(si->lb_pbm, 35);
        si->lb_port = 35;
        si->cpu_hg_index = 39;
        break;
    case BCM56441_DEVICE_ID:
    case BCM56446_DEVICE_ID:
        /* 8x1G + 2xHG */
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        SOC_PBMP_CLEAR(pbmp_hyplite);
        SOC_PBMP_CLEAR(pbmp_mxq13g);
        SOC_PBMP_CLEAR(pbmp_mxq2p5g);
        SOC_PBMP_CLEAR(pbmp_mxq);

        /*
         * For 56441/2/3 devices port 1 is always included. The OTP settings
         * are checked in _soc_katana_misc_init and if CES is not enabled then
         * the port is removed there.
         */
        switch (soc_property_get(unit, spn_BCM5644X_CONFIG, 0)) {
            case 2: /* 4x1G + 4x2.5HGL + 1xHG */
                if (dev_id == BCM56441_DEVICE_ID) {
                    /* Port 0 is used for CES */    
                    for (port = 2; port <= 8; port++) {
                        SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                    }
                } else if (dev_id == BCM56446_DEVICE_ID) {
                    for (port = 5; port <= 12; port++) { 
                        SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                    }
                }
                for (port = 13; port <= 26; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                SOC_PBMP_PORT_ADD(pbmp_mxq13g, 27);
                SOC_PBMP_PORT_ADD(pbmp_mxq, 27);
                SOC_PBMP_PORT_ADD(pbmp_gport_stack, 27);
                for (port = 28; port <= 31; port++) {
                     SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                     SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                     SOC_PBMP_PORT_ADD(pbmp_gport_stack, port);
                }
                for (port = 32; port <= 34; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                break;
            case 1:
                /* (Frontpanel)8xSGMII + (BackPanel) 4*2.5HGL + 4*2.5GE */
                /* 56441 : GPORT1 - 9 to 16 */
                /* 56446 : GPORT0 - 1 to 8  */
                if (dev_id == BCM56441_DEVICE_ID) {
                /* Port 0 is used for CES */
                for (port = 2; port <= 8; port++) {
                         SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                    }
                } else if (dev_id == BCM56446_DEVICE_ID) {
                    for (port = 9; port <= 16; port++) {
                         SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                    }
                }
                for (port = 17; port <= 26; port++) {
                     SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                /* MXQPORT2 - 27,32,33,34 */
                for (kt_mxqport_index = 0 ;
                     kt_mxqport_index < 4 ; 
                     kt_mxqport_index++) {
                     port = kt_mxqport2[kt_mxqport_index]; /* 27,32,33,34 */
                     SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                     SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                }
                /*
                    MXQPORT3 - 28,29,30,31
                 */
                for (port = 28; port <= 31; port++) {
                     SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                     SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                     SOC_PBMP_PORT_ADD(pbmp_gport_stack, port);
                }
                break;
            case 0: /* 8x1G + 2xHG */
            default:  
                /* (Frontpanel)8xSGMII + (BackPanel)2HG[13] */
                /* 56441 : GPORT1 - 9 to 16 */
                /* 56446 : GPORT0 - 1 to 8 */
 
                if (dev_id == BCM56441_DEVICE_ID) {
                /* Port 0 is used for CES */
                for (port = 2; port <= 8; port++) {
                         SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                    }
                } else if (dev_id == BCM56446_DEVICE_ID) {
                    for (port = 9; port <= 16; port++) {
                         SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                    }
                }
                for (port = 17; port <= 26; port++) {
                     SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                /*
                    MXQPORT2 - 27
                    MXQPORT3 - 28
                */
                for (port = 27; port <= 28; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq13g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                    SOC_PBMP_PORT_ADD(pbmp_gport_stack, port);
                }
                for (port = 29; port <= 34; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                break;
        }
        for (port = 36; port <= 38; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(si->tdm_pbm);
        SOC_PBMP_CLEAR(si->lb_pbm);
        SOC_PBMP_PORT_ADD(si->lb_pbm, 35);
        si->lb_port = 35;
        si->cpu_hg_index = 39;
        break;
    case BCM56442_DEVICE_ID:
    case BCM56447_DEVICE_ID:
        /* 16x1G */
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        if (dev_id == BCM56442_DEVICE_ID) {
            for (port = 2; port <= 8; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
        } else if (dev_id == BCM56447_DEVICE_ID) {
            for (port = 17; port <= 24; port++) {
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
        }
        for (port = 25; port <= 34; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 36; port <= 38; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(si->lb_pbm);
        SOC_PBMP_PORT_ADD(si->lb_pbm, 35);
        si->lb_port = 35;
        si->cpu_hg_index = 39;
        break;
    case BCM56443_DEVICE_ID:
        /* 8x1G + 2xHG + 8x2.5G*/
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        SOC_PBMP_CLEAR(pbmp_hyplite);
        SOC_PBMP_CLEAR(pbmp_mxq13g);
        SOC_PBMP_CLEAR(pbmp_mxq2p5g);
        SOC_PBMP_CLEAR(pbmp_mxq);
        for (port = 2; port <= 8; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 17; port <= 24; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        switch (soc_property_get(unit, spn_BCM5644X_CONFIG, 0)) {
            case 0: /* 8x1G + 2xHG + 8x2.5G */
                for (port = 25; port <= 26; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq13g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                }
                for (port = 27; port <= 34; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                }
                break;
            case 1: /* 8x1G + 2x2.5HGL + 8x2.5G */
                for (port = 25; port <= 26; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                    SOC_PBMP_PORT_ADD(pbmp_gport_stack, port);
                    SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
                }
                for (port = 27; port <= 34; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                }
                break;
            default:
                break;
        }
        for (port = 36; port <= 38; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(si->lb_pbm);
        SOC_PBMP_PORT_ADD(si->lb_pbm, 35);
        si->lb_port = 35;
        si->cpu_hg_index = 39;
        break;
    case BCM56449_DEVICE_ID:
        /* 2xHG + 2x2.5HGL */
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        SOC_PBMP_CLEAR(pbmp_mxq13g);
        SOC_PBMP_CLEAR(pbmp_mxq2p5g);
        SOC_PBMP_CLEAR(pbmp_tmp);
        SOC_PBMP_CLEAR(pbmp_hyplite);

        for (port = 1; port <= 24; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        switch (soc_property_get(unit, spn_BCM5644X_CONFIG, 0)) {
            case 0: /* 2xHG + 2x2.5HGL */
                for (port = 27; port <= 28; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                    SOC_PBMP_PORT_ADD(pbmp_gport_stack, port);
                    SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
                }
                for (port = 25; port <= 26; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq13g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                }
                for (port = 29; port <= 34; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                break;
            case 1: /* 4x2.5HGL */
                for (port = 25; port <= 28; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                    SOC_PBMP_PORT_ADD(pbmp_gport_stack, port);
                    SOC_PBMP_PORT_ADD(pbmp_hyplite, port);
                }
                for (port = 29; port <= 34; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                break;
           default:
                break;
        }
        for (port = 36; port <= 38; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(si->tdm_pbm);
        SOC_PBMP_CLEAR(si->lb_pbm);
        SOC_PBMP_PORT_ADD(si->lb_pbm, 35);
        si->lb_port = 35;
        si->cpu_hg_index = 39;
        break;
    case BCM56448_DEVICE_ID:
        /* 24x1G + 4x2.5HGL */
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        SOC_PBMP_CLEAR(pbmp_mxq13g);
        SOC_PBMP_CLEAR(pbmp_mxq2p5g);
        SOC_PBMP_CLEAR(pbmp_tmp);
        for (port = 25; port <= 28; port++) {
            SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
            SOC_PBMP_PORT_ADD(pbmp_mxq, port);
            SOC_PBMP_PORT_ADD(pbmp_gport_stack, port);
        }
        for (port = 29; port <= 34; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 36; port <= 38; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(si->tdm_pbm);
        SOC_PBMP_CLEAR(si->lb_pbm);
        SOC_PBMP_PORT_ADD(si->lb_pbm, 35);
        si->lb_port = 35;
        si->cpu_hg_index = 39;
        break;
    case BCM56241_DEVICE_ID:
        /* 6xGE + 2x2.5G*/
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        SOC_PBMP_CLEAR(pbmp_hyplite);
        SOC_PBMP_CLEAR(pbmp_mxq13g);
        SOC_PBMP_CLEAR(pbmp_mxq2p5g);
        SOC_PBMP_CLEAR(pbmp_mxq);
        for (port = 1; port <= 26; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }

        for (port = 27; port <= 34; port++) {
            SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
            SOC_PBMP_PORT_ADD(pbmp_mxq, port);
            if ((port >= 30) && (port <= 31)) {
                si->port_speed_max[port] = 2500;
            } else {
                si->port_speed_max[port] =1000;
            }
        }

        for (port = 36; port <= 38; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(si->lb_pbm);
        SOC_PBMP_PORT_ADD(si->lb_pbm, 35);
        si->lb_port = 35;
        si->cpu_hg_index = 39;
        break;
    case BCM56240_DEVICE_ID:
    case BCM56245_DEVICE_ID:
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        SOC_PBMP_CLEAR(pbmp_hyplite);
        SOC_PBMP_CLEAR(pbmp_mxq13g);
        SOC_PBMP_CLEAR(pbmp_mxq10g);
        SOC_PBMP_CLEAR(pbmp_mxq2p5g);
        SOC_PBMP_CLEAR(pbmp_mxq);
        for (port = 1; port <= 24; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }

        switch (soc_property_get(unit, spn_BCM5644X_CONFIG, 0)) {
            case 0: /* 2x10G + 2xHG[13] */
                SOC_PBMP_PORT_ADD(pbmp_mxq13g, 25);
                SOC_PBMP_PORT_ADD(pbmp_mxq13g, 26);
                SOC_PBMP_PORT_ADD(pbmp_mxq10g, 27);
                SOC_PBMP_PORT_ADD(pbmp_mxq10g, 28);
                for (port = 25; port <= 28; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                }
                for (port = 29; port <= 34; port++) {
                    SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
                }
                break;
            case 1: /* 8x2.5G + 2xHG[13] */
                SOC_PBMP_PORT_ADD(pbmp_mxq13g, 25);
                SOC_PBMP_PORT_ADD(pbmp_mxq, 25);
                SOC_PBMP_PORT_ADD(pbmp_mxq13g, 26);
                SOC_PBMP_PORT_ADD(pbmp_mxq, 26);
                for (port = 27; port <= 34; port++) {
                    SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
                    SOC_PBMP_PORT_ADD(pbmp_mxq, port);
                }
                break;
            default:
                break;
        }
        for (port = 36; port <= 38; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(si->lb_pbm);
        SOC_PBMP_PORT_ADD(si->lb_pbm, 35);
        si->lb_port = 35;
        si->cpu_hg_index = 39;
        break;
    case BCM56242_DEVICE_ID:
    case BCM56246_DEVICE_ID:
        /* 10x2.5G */
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        SOC_PBMP_CLEAR(pbmp_hyplite);
        SOC_PBMP_CLEAR(pbmp_mxq13g);
        SOC_PBMP_CLEAR(pbmp_mxq10g);
        SOC_PBMP_CLEAR(pbmp_mxq2p5g);
        SOC_PBMP_CLEAR(pbmp_mxq);
        for (port = 1; port <= 24; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 25; port <= 34; port++) {
            SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
            SOC_PBMP_PORT_ADD(pbmp_mxq, port);
        }
        for (port = 36; port <= 38; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(si->lb_pbm);
        SOC_PBMP_PORT_ADD(si->lb_pbm, 35);
        si->lb_port = 35;
        si->cpu_hg_index = 39;
        break;
    case BCM56243_DEVICE_ID:
        /* 4x2.5G */
        SOC_PBMP_CLEAR(pbmp_gport_stack);
        SOC_PBMP_CLEAR(pbmp_hyplite);
        SOC_PBMP_CLEAR(pbmp_mxq13g);
        SOC_PBMP_CLEAR(pbmp_mxq10g);
        SOC_PBMP_CLEAR(pbmp_mxq2p5g);
        SOC_PBMP_CLEAR(pbmp_mxq);
        for (port = 1; port <= 27; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 28; port <= 31; port++) {
            SOC_PBMP_PORT_ADD(pbmp_mxq2p5g, port);
            SOC_PBMP_PORT_ADD(pbmp_mxq, port);
        }
        for (port = 32; port <= 34; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        for (port = 36; port <= 38; port++) {
            SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
        }
        SOC_PBMP_CLEAR(si->lb_pbm);
        SOC_PBMP_PORT_ADD(si->lb_pbm, 35);
        si->lb_port = 35;
        si->cpu_hg_index = 39;
        break;
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_KATANA2_SUPPORT
    case BCM55450_DEVICE_ID:
    case BCM55455_DEVICE_ID:
    case BCM56248_DEVICE_ID:
    case BCM56450_DEVICE_ID:
    case BCM56452_DEVICE_ID:
    case BCM56454_DEVICE_ID:
    case BCM56455_DEVICE_ID:
    case BCM56456_DEVICE_ID:
    case BCM56457_DEVICE_ID:
    case BCM56458_DEVICE_ID:
        /* Init kt2_pbmp structure with local variable's addresses */
        kt2_pbmp.pbmp_gport_stack=&pbmp_gport_stack;
        kt2_pbmp.pbmp_mxq=&pbmp_mxq;
        kt2_pbmp.pbmp_mxq1g=&pbmp_mxq1g;
        kt2_pbmp.pbmp_mxq2p5g=&pbmp_mxq2p5g;
        kt2_pbmp.pbmp_mxq10g=&pbmp_mxq10g;
        kt2_pbmp.pbmp_mxq13g=&pbmp_mxq13g;
        kt2_pbmp.pbmp_mxq21g=&pbmp_mxq21g;
        kt2_pbmp.pbmp_xport_xe=&pbmp_xport_xe;
        kt2_pbmp.pbmp_valid=&pbmp_valid;
        kt2_pbmp.pbmp_pp=&pbmp_pp;
        kt2_pbmp.pbmp_linkphy=&(si->linkphy_pbm);
        /* Init KT2 related pbmp now */
        soc_katana2_pbmp_init(unit,kt2_pbmp);

        SOC_PBMP_CLEAR(si->tdm_pbm);
        SOC_PBMP_CLEAR(si->lb_pbm);
        SOC_PBMP_PORT_ADD(si->lb_pbm, 41);
        si->lb_port = 41;
        si->cpu_hg_index = 42;
        si->cpu_hg_pp_port_index = 170;
        /* Initialize subport related pbmp linkphy_pbm and subtag_pbm */
        soc_katana2_subport_init(unit);

        break;
#endif /* BCM_KATANA2_SUPPORT */

    default:
        break;
    }

    /* No emulation/debug only regs when a real chip or plisim */
    if (!SAL_BOOT_RTLSIM && !soc_property_get(unit, spn_EMULATION_REGS, 0)) {
        soc->disabled_reg_flags |= SOC_REG_FLAG_EMULATION;
    }

    si->ipic_port = -1;
    si->ipic_block = -1;
    si->cmic_port = -1;
    si->cmic_block = -1;
    si->fe.min = si->fe.max = -1;
    si->ge.min = si->ge.max = -1;
    si->xe.min = si->xe.max = -1;
    si->ce.min = si->ce.max = -1;
    si->hg.min = si->hg.max = -1;
    si->hl.min = si->hl.max = -1;
    si->st.min = si->st.max = -1;
    si->lp.min = si->lp.max = -1;
    si->cl.min = si->cl.max = -1;
    si->c.min = si->c.max = -1;
    si->mmu.min = si->mmu.max = -1;
    si->tdm.min = si->tdm.max = -1;
    si->ether.min = si->ether.max = -1;
    si->port.min = si->port.max = -1;
    si->all.min = si->all.max = -1;
#ifdef BCM_SHADOW_SUPPORT 
    si->ms_isec_block[0] = -1;
    si->ms_isec_block[1] = -1;
    si->ms_esec_block[0] = -1;
    si->ms_esec_block[1] = -1;
#endif /*BCM_SHADOW_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT 
    si->rxlp_block[0] = -1;
    si->rxlp_block[1] = -1;
    si->txlp_block[0] = -1;
    si->txlp_block[1] = -1;
    si->olp_block = -1;
#endif /*BCM_KATANA2_SUPPORT */

    for (blk = 0; blk < SOC_MAX_NUM_BLKS; blk++) {
        si->block_port[blk] = REG_PORT_ANY;
#if defined(BCM_KATANA2_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT)
        if ((SOC_IS_KATANA2(unit) || SOC_IS_HURRICANE2(unit))) {
            if ((SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_RXLP) && 
                (SOC_BLOCK_INFO(unit, blk).number == 0)) {
                si->rxlp_block[0] = blk;
            }
            if ((SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_RXLP) && 
                (SOC_BLOCK_INFO(unit, blk).number == 1)) {
                si->rxlp_block[1] = blk;
            }
            if ((SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_TXLP) && 
                (SOC_BLOCK_INFO(unit, blk).number == 0)) {
                si->txlp_block[0] = blk;
            }
            if ((SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_TXLP) && 
                (SOC_BLOCK_INFO(unit, blk).number == 1)) {
                si->txlp_block[1] = blk;
            }
#if defined(BCM_KATANA2_SUPPORT)
            if ((SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_MXQPORT) && 
                (SOC_BLOCK_INFO(unit, blk).number == 10)) {
                si->olp_block = blk;
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "OLP blk:%d \n"),si->olp_block ));
            }
            if ((SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_MXQPORT) && 
                (SOC_BLOCK_INFO(unit, blk).number == 7)) {
                mxq7_blk = blk;
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "OLP blk:%d \n"),si->olp_block ));
            }
#endif /*BCM_KATANA2_SUPPORT */
        }
#endif
    }

#define ADD_PORT(ptype, port_num) \
            si->ptype.port[si->ptype.num++] = port_num; \
            if (si->ptype.min > port_num || si->ptype.min < 0) { \
                si->ptype.min = port_num; \
            } \
            if (si->ptype.max < port_num) {     \
                si->ptype.max = port_num; \
            } \
            SOC_PBMP_PORT_ADD(si->ptype.bitmap, port_num);

    max_port = -1;
    for (phy_port = 0; ; phy_port++) {
        blk = SOC_PORT_IDX_BLOCK(unit, phy_port, 0);
        bindex = SOC_PORT_IDX_BINDEX(unit, phy_port, 0);
        if (blk < 0 && bindex < 0) { /* end of regsfile port list */
            break;
        }
        if (soc_feature(unit, soc_feature_logical_port_num)) {
            port = si->port_p2l_mapping[phy_port];
            if (port < 0) { /* not used in user config */
                continue;
            }
            if (port > max_port) {
                max_port = port;
            }
            if (si->flex_port_p2l_mapping[phy_port] == -1) {
                SOC_PBMP_PORT_ADD(si->port.disabled_bitmap, port);
                SOC_PBMP_PORT_ADD(si->all.disabled_bitmap, port);
            }
        } else {
            port = phy_port;
            max_port = phy_port;
        }

        if (!SOC_PBMP_MEMBER(pbmp_valid, port)) {   /* disabled port */
            int blk_list[2];
            blk_list[0] = SOC_BLOCK_INFO(unit, blk).type;
            blk_list[1] = SOC_BLOCK_TYPE_INVALID;
            if (SOC_BLOCK_IN_LIST(blk_list, SOC_BLK_CPU)) {
                LOG_WARN(BSL_LS_SOC_COMMON,
                         (BSL_META_U(unit,
                                     "soc_info_config: "
                                     "cannot disable cpu port\n")));
            } else {
                blk = -1; /* mark as disabled port */
            }
        }
#if defined(BCM_ENDURO_SUPPORT) || defined(BCM_KATANA_SUPPORT)
        if(SOC_IS_ENDURO(unit) || SOC_IS_KATANAX(unit)) {
            if (IS_LB_PORT(unit, port)) {
                blk = -1; /* mark as disabled port */
            }
        }
#endif
#if defined(BCM_SHADOW_SUPPORT)
        if(SOC_IS_SHADOW(unit)) {
            if (SOC_PBMP_MEMBER(pbmp_disabled, port) && !IS_IL_PORT(unit, port)) {   /* disabled port */
                SOC_PBMP_PORT_REMOVE(pbmp_valid, port);
            }
        }
#endif
        if (blk < 0) { /* disabled port */
            sal_snprintf(si->port_name[port], sizeof(si->port_name[port]),
                         "?%d", port);
            si->port_offset[port] = port;
            continue;
        }

        blktype = -1;
        bname = "?";
        for (port_idx = 0; port_idx < SOC_DRIVER(unit)->port_num_blktype;
             port_idx++) {
            int skip_blk_port = 0;             
            blk = SOC_PORT_IDX_BLOCK(unit, phy_port, port_idx);
            if (SOC_IS_GREYHOUND(unit) && 
                ((SOC_BLOCK_INFO(unit, blk).number & 0xf00) == 0xf00)){
                if(phy_port == 5){
                    SOC_BLOCK_INFO(unit, blk).number &= ~0xf00;
                }
                skip_blk_port = 1;
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "skip the phy_port %d blk %d\n"),
                                        phy_port,blk));
            }
            if (blk < 0) { /* end of block list of each port */
                break;
            }

            old_blktype = blktype;
            old_bname = bname;

            blktype = SOC_BLOCK_INFO(unit, blk).type;
            bname = soc_block_port_name_lookup_ext(blktype, unit);
            switch (blktype) {
            case SOC_BLK_MXQPORT:
                if (!si->port_speed_max[port]) {
                    if (SOC_PBMP_MEMBER(pbmp_mxq21g, port)) {
                        si->port_speed_max[port] = 21000;
                    } else if (SOC_PBMP_MEMBER(pbmp_mxq13g, port)) {
                        si->port_speed_max[port] = 13000;
                    } else if (SOC_PBMP_MEMBER(pbmp_mxq2p5g, port)) {
                        si->port_speed_max[port] = 2500;
                    } else if (SOC_PBMP_MEMBER(pbmp_mxq1g, port)) {
                        si->port_speed_max[port] = 1000;
                    } else {
                        si->port_speed_max[port] = 10000;
                    }
                }
#if defined(BCM_KATANA2_SUPPORT)
                if (soc_feature(unit, soc_feature_linkphy_coe) &&
                    SOC_PBMP_MEMBER(si->linkphy_pbm, port)) {
                    pno = si->lp.num;
                    ADD_PORT(lp, port);
                }
                if (soc_feature(unit, soc_feature_subtag_coe) &&
                    SOC_PBMP_MEMBER(si->subtag_pbm, port)) {
                    pno = si->subtag.num;
                    ADD_PORT(subtag, port);
                }
#endif
                if (si->port_speed_max[port] < 10000) { /* GE */
                    pno = si->ge.num;
                    ADD_PORT(ge, port);
                    if (SOC_PBMP_MEMBER(pbmp_gport_stack, port)) {
                        /* GE in Higig Lite Mode. Higig Like stack capable
                         * port */
                        ADD_PORT(st, port);
                        ADD_PORT(hl, port);
                    } else {
                        ADD_PORT(ether, port);
                    }
                } else if (SOC_PBMP_MEMBER(pbmp_xport_xe, port)) { /* XE */
                    pno = si->xe.num;
                    ADD_PORT(xe, port);
                    ADD_PORT(ether, port);
                    bname = "xe";
                } else { /* HG */
                    pno = si->hg.num;
                    ADD_PORT(hg, port);
                    ADD_PORT(st, port);
                }

                ADD_PORT(port, port);
                if (SOC_PBMP_MEMBER(pbmp_mxq, port)) {
                    ADD_PORT(mxq, port);
                }
                if (SOC_PBMP_MEMBER(pbmp_hyplite, port)) {
                    ADD_PORT(hyplite, port);
                }
                ADD_PORT(all, port);
                break;
            case SOC_BLK_EPIC:
                pno = si->fe.num;
                ADD_PORT(fe, port);
                ADD_PORT(ether, port);
                ADD_PORT(port, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_GPIC:
            case SOC_BLK_XTPORT:
            case SOC_BLK_QGPORT:
            case SOC_BLK_GPORT:
                if (SOC_PBMP_MEMBER(pbmp_gport_fe, port)) { /* GE  vs FE SMII  */
                    pno = si->fe.num;
                    bname = "fe";
                    ADD_PORT(fe, port);
                } else {
                    if (SOC_IS_TRIUMPH3(unit)) {
                        if (si->port_speed_max[port] == 10000) {
                            pno = si->xe.num;
                            ADD_PORT(xe, port);
                        } else {
                            pno = si->ge.num;
                            ADD_PORT(ge, port);
                        } 
                    } else {
                        pno = si->ge.num;
                        ADD_PORT(ge, port);
                    }
                }
                if (SOC_PBMP_MEMBER(pbmp_gport_stack, port)) {
                    /* GE in Higig Lite Mode. Higig Like stack capable port */
                    ADD_PORT(st, port);
                    ADD_PORT(hl, port);
                } else {
                    ADD_PORT(ether, port);
                }
                if (SOC_IS_TRIUMPH3(unit)) {
                    ADD_PORT(xt, port);
                }
                ADD_PORT(port, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_XWPORT:
                if (si->port_speed_max[port] && si->port_speed_max[port] < 10000) { /* GE */
                    pno = si->ge.num;
                    ADD_PORT(ge, port);
                    ADD_PORT(ether, port);
                    bname = "ge";
                } else if (SOC_PBMP_MEMBER(pbmp_xport_xe, port) ||
                           (si->port_speed_max[port] == 10000)) { /* XE */
                    pno = si->xe.num;
                    ADD_PORT(xe, port);
                    ADD_PORT(ether, port);
                    bname = "xe";
                } else { /* HG */
                    pno = si->hg.num;
                    ADD_PORT(hg, port);
                    ADD_PORT(st, port);
                }
                ADD_PORT(xw, port);
                ADD_PORT(port, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_XPIC:
                pno = si->xe.num;
                ADD_PORT(xe, port);
                ADD_PORT(ether, port);
                ADD_PORT(port, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_IPIC:
                si->ipic_port = port;
                si->ipic_block = blk;
                /* FALLTHROUGH */
            case SOC_BLK_HPIC:
                pno = si->hg.num;
                ADD_PORT(hg, port);
                ADD_PORT(st, port);
                ADD_PORT(port, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_XPORT:
                if (SOC_PBMP_MEMBER(pbmp_xport_xe, port)) { /* XE  vs HG  */
                    pno = si->xe.num;
                    ADD_PORT(xe, port);
                    ADD_PORT(ether, port);
                    bname = "xe";
                } else {
                    pno = si->hg.num;
                    ADD_PORT(hg, port);
                    ADD_PORT(st, port);
                }
                si->port_speed_max[port] = 12000;
                ADD_PORT(port, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_CPORT:            
                if (si->port_speed_max[port] < 100000) {
                    /* don't use the name from this block */
                    blktype = old_blktype;
                    bname = old_bname;
                    break;
                }
                ADD_PORT(c, port);
                if (si->port_speed_max[port] == 100000) { /* CE */
                    pno = si->c.num;
                    ADD_PORT(ce, port);
                    ADD_PORT(ether, port);
                    /* name is default to ce by lookup above */
                } else { /* HG */
                    pno = si->hg.num;
                    ADD_PORT(hg, port);
                    ADD_PORT(st, port);
                    bname = "hg";
                }
                ADD_PORT(port, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_CLPORT:            
                ADD_PORT(cl, port);
                if (si->port_speed_max[port] >= 100000) {
                    /* Catch both CE and HG ports */
                    ADD_PORT(c, port);
                }
                if (si->port_speed_max[port] == 100000) { /* CE */
                    pno = si->ce.num;
                    ADD_PORT(ce, port);
                    ADD_PORT(ether, port);
                    bname = "ce";
                    ADD_PORT(port, port);
                    ADD_PORT(all, port);
                    break;
                }
                /* fall through to case for SOC_BLK_XLPORT */
                /* coverity[fallthrough: FALSE] */
            case SOC_BLK_XLPORT:                
                if(SOC_IS_GREYHOUND(unit) && (skip_blk_port == 1)){
                    si->block_valid[blk] += 1;
                    continue;
                }
                if ((SOC_BLK_XLPORT == blktype) &&
                    (si->port_speed_max[port] >= 100000)) {
                    /* don't use the name from this block */
                    blktype = old_blktype;
                    bname = old_bname;
                    break;
                }
                if (!si->port_speed_max[port]) {
                    if (SOC_PBMP_MEMBER(pbmp_xport_ge, port)) {
                        si->port_speed_max[port] = 2500;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx25g, port)) {
                        si->port_speed_max[port] = 25000;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx21g, port)) {
                        si->port_speed_max[port] = 21000;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx20g, port)) {
                        si->port_speed_max[port] = 20000;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx16g, port)) {
                        si->port_speed_max[port] = 16000;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx13g, port)) {
                        si->port_speed_max[port] = 13000;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx12g, port)) {
                        si->port_speed_max[port] = 12000;
                    } else {
                        si->port_speed_max[port] = 10000;
                    }
#ifdef BCM_SHADOW_SUPPORT
                    if (SOC_IS_SHADOW(unit)) {
                        if SOC_PBMP_MEMBER(pbmp_xl12g, port) {
                            si->port_speed_max[port] = 12000;
                        } else if (SOC_PBMP_MEMBER(pbmp_xl40g, port)) {
                            si->port_speed_max[port] = 40000;
                        }
                    }
#endif
                }
                if (si->port_speed_max[port] < 10000) { /* GE */
#ifdef BCM_GREYHOUND_SUPPORT
                    if (SOC_IS_GREYHOUND(unit) &&
                        si->port_speed_max[port] == 5000) {
                        if (SOC_PBMP_MEMBER(pbmp_xport_ge, port)) { /* GE */
                            pno = si->ge.num;
                            si->port_speed_max[port] = 2500;
                            ADD_PORT(ge, port);
                            ADD_PORT(ether, port);
                            bname = "ge";
                        } else if (SOC_PBMP_MEMBER(pbmp_xport_xe, port)) { /* XE */
                            pno = si->xe.num;
                            ADD_PORT(xe, port);
                            ADD_PORT(ether, port);
                            bname = "xe";
                        } else { /* HG */
                            pno = si->hg.num;
                            ADD_PORT(hg, port);
                            ADD_PORT(st, port);
                        }
                    } else
#endif
                    {
                    pno = si->ge.num;
                    ADD_PORT(ge, port);
                    if (SOC_PBMP_MEMBER(pbmp_gport_stack, port)) {
                        /* GE in Higig Lite Mode. Higig Like stack capable
                         * port */
                        ADD_PORT(st, port);
                        ADD_PORT(hl, port);
                    } else {
                        ADD_PORT(ether, port);
                    }
                    }
                } else if (SOC_PBMP_MEMBER(pbmp_xport_xe, port)) { /* XE */
                    pno = si->xe.num;
                    ADD_PORT(xe, port);
                    ADD_PORT(ether, port);
                    bname = "xe";
#ifdef BCM_TRIUMPH3_SUPPORT
                    if (SOC_IS_TRIUMPH3(unit)) {
                        if (si->port_speed_max[port] == 42000) {
                            si->port_speed_max[port] = 40000;
                        }
                    }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_SHADOW_SUPPORT
                    if (SOC_IS_SHADOW(unit) &&
                        (
                      soc_property_get(unit, spn_BCM88732_2X40_8X10, 0) ||
                      soc_property_get(unit, spn_BCM88732_1X40_4X10, 0) ||
                      soc_property_get(unit, spn_BCM88732_4X10_4X10, 0))) {
                      /* TDM16 and TDM 17 */
                        if (SOC_PBMP_MEMBER(pbmp_disabled, port)) {
                            SOC_PBMP_PORT_ADD(si->xe.disabled_bitmap, port);
                            SOC_PBMP_PORT_ADD(si->port.disabled_bitmap, port);
                            SOC_PBMP_PORT_ADD(si->all.disabled_bitmap, port);
                        }
                    }
                } else if (soc_property_get(unit, spn_BCM88732_2X40_2X40, 0) ||
                    soc_property_get(unit, spn_BCM88732_2X40_1X40, 0) ||
                    soc_property_get(unit, spn_BCM88732_8X10_1X40, 0) ||
                    soc_property_get(unit, spn_BCM88732_8X10_2X40, 0)) { 
                    /* Interlaken  Modes */
                    pno = si->il.num;
                    ADD_PORT(il, port);
                    bname = "il";
                    if (SOC_PBMP_MEMBER(pbmp_disabled, port)) {
                        SOC_PBMP_PORT_ADD(si->il.disabled_bitmap, port);
                        SOC_PBMP_PORT_ADD(si->port.disabled_bitmap, port);
                        SOC_PBMP_PORT_ADD(si->all.disabled_bitmap, port);
                    }
#endif /* BCM_SHADOW_SUPPORT */
                } 
#ifdef BCM_HURRICANE2_SUPPORT
                else if ((SOC_IS_HURRICANE2(unit)||SOC_IS_GREYHOUND(unit)) &&
                    (SOC_PBMP_MEMBER(pbmp_xport_ge, port))) { /* GE */
                    pno = si->ge.num;
                    si->port_speed_max[port] = 1000;
#ifdef BCM_GREYHOUND_SUPPORT
                    if ((SOC_IS_GREYHOUND(unit)) &&
                        ((dev_id == BCM53402_DEVICE_ID) ||
                         (dev_id == BCM53405_DEVICE_ID) ||
                         (dev_id == BCM53406_DEVICE_ID))) {
                        si->port_speed_max[port] = 2500;
                    }
#endif
                    ADD_PORT(ge, port);
                    ADD_PORT(ether, port);
                    bname = "ge";
                }
#endif
#ifdef BCM_TRIUMPH3_SUPPORT
                else if (SOC_IS_TRIUMPH3(unit) && (si->port_speed_max[port] == 40000)) {
                    pno = si->xe.num;
                    ADD_PORT(xe, port);
                    ADD_PORT(ether, port);
                    bname = "xe";
                }
#endif /* BCM_TRIUMPH3_SUPPORT */
                else { /* HG */
                    pno = si->hg.num;
                    ADD_PORT(hg, port);
                    ADD_PORT(st, port);
                    /* name is default to hg by lookup above */
                }
#if defined(BCM_SHADOW_SUPPORT)
                if (SOC_IS_SHADOW(unit)) {
                    if (SOC_PBMP_MEMBER(pbmp_xl, port)) {
                        ADD_PORT(xl, port);
                    }
                } else
#endif /* BCM_SHADOW_SUPPORT */
                {
                    if (!IS_CL_PORT(unit, port)) {
                        ADD_PORT(xl, port);
                    }
                }
                if (!SOC_IS_TRIUMPH3(unit)) {
                    
                    ADD_PORT(gx, port);
                }
                ADD_PORT(port, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_GXPORT:
                if(SOC_IS_GREYHOUND(unit)&& (skip_blk_port == 1)){
                    si->block_valid[blk] += 1;
                    continue;
                }
                if (!si->port_speed_max[port]) {
                    if (SOC_PBMP_MEMBER(pbmp_xport_ge, port)) {
                        si->port_speed_max[port] = 2500;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx25g, port)) {
                        si->port_speed_max[port] = 25000;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx21g, port)) {
                        si->port_speed_max[port] = 21000;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx20g, port)) {
                        si->port_speed_max[port] = 20000;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx16g, port)) {
                        si->port_speed_max[port] = 16000;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx13g, port)) {
                        si->port_speed_max[port] = 13000;
                    } else if (SOC_PBMP_MEMBER(pbmp_gx12g, port)) {
                        si->port_speed_max[port] = 12000;
                    } else {
                        si->port_speed_max[port] = 10000;
                    }
                }
                if (si->port_speed_max[port] < 10000) { /* GE */
                    pno = si->ge.num;
                    ADD_PORT(ge, port);
                    ADD_PORT(ether, port);
                    bname = "ge";
                } else if (SOC_PBMP_MEMBER(pbmp_xport_xe, port)) { /* XE */
                    pno = si->xe.num;
                    ADD_PORT(xe, port);
                    ADD_PORT(ether, port);
                    bname = "xe";
                } else { /* HG */
                    pno = si->hg.num;
                    ADD_PORT(hg, port);
                    ADD_PORT(st, port);
                    /* name is default to hg by lookup above */
                }
                if (!SOC_IS_GREYHOUND(unit)) {      
                    /* no xe capability in Greyhound's SOC_BLK_GXPORT */
                    ADD_PORT(gx, port);
                }
                if (SOC_PBMP_MEMBER(pbmp_hyplite, port)) {
                    ADD_PORT(hyplite, port);
                }
                ADD_PORT(port, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_XQPORT:
            case SOC_BLK_XGPORT:
                if (SOC_PBMP_MEMBER(pbmp_xport_ge, port)) { /* GE */
                    pno = si->ge.num;
                    ADD_PORT(ge, port);
                    ADD_PORT(ether, port);
                    bname = "ge";
                } else {
                    if (!si->port_speed_max[port]) {
                        if (SOC_PBMP_MEMBER(pbmp_xq12g, port)) {
                            si->port_speed_max[port] = 12000;
                        } else if (SOC_PBMP_MEMBER(pbmp_xq13g, port)) {
                            si->port_speed_max[port] = 13000;
                        } else if (SOC_PBMP_MEMBER(pbmp_xq10g, port)) {
                            si->port_speed_max[port] = 10000;
                        } else if (SOC_PBMP_MEMBER(pbmp_xg2p5g, port) || 
                                   SOC_PBMP_MEMBER(pbmp_xq2p5g, port)) {
                            si->port_speed_max[port] = 2500;
                        } else {
                            si->port_speed_max[port] = 1000;
                        }
                    }
                    if (SOC_PBMP_MEMBER(pbmp_xq12g, port) || 
                        SOC_PBMP_MEMBER(pbmp_xq13g, port) ||
                        SOC_PBMP_MEMBER(pbmp_xq10g, port)) {
                        ADD_PORT(gx, port);
                    }
                    if (si->port_speed_max[port] >= 10000) {
                        if (SOC_PBMP_MEMBER(pbmp_xport_xe, port)) { /* XE */
                            pno = si->xe.num;
                            ADD_PORT(xe, port);
                            ADD_PORT(ether, port);
                            bname = "xe";
                        } else { /* HG */
                            pno = si->hg.num;
                            ADD_PORT(hg, port);
                            ADD_PORT(st, port);
                            bname = "hg";
                        }
                    } else {
                        pno = si->ge.num;
                        ADD_PORT(ge, port);
                        if (SOC_PBMP_MEMBER(pbmp_gport_stack, port)) {
                            /* GE in Higig Lite Mode. Higig Like stack capable
                             * port */
                            ADD_PORT(st, port);
                            ADD_PORT(hl, port);
                        } else {
                            ADD_PORT(ether, port);
                        }
                    }
                }
                if (SOC_PBMP_MEMBER(pbmp_disabled, port)) {
                    SOC_PBMP_PORT_ADD(si->ge.disabled_bitmap, port);
                    SOC_PBMP_PORT_ADD(si->ether.disabled_bitmap, port);
                    SOC_PBMP_PORT_ADD(si->port.disabled_bitmap, port);
                    SOC_PBMP_PORT_ADD(si->all.disabled_bitmap, port);
                }
                if (SOC_PBMP_MEMBER(pbmp_xq, port)) {
                    ADD_PORT(xq, port);
                } else {
                    ADD_PORT(xg, port);
                }
                if (SOC_PBMP_MEMBER(pbmp_hyplite, port)) {
                    ADD_PORT(hyplite, port);
                }
                ADD_PORT(port, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_SPORT:
                pno = si->ge.num;
                ADD_PORT(ge, port);
                ADD_PORT(ether, port);
                ADD_PORT(port, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_LBPORT:
                pno = 0;
                si->lb_port = port;
                SOC_PBMP_PORT_ADD(si->lb_pbm, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_AXP:
                pno = si->axp.num;
                ADD_PORT(axp, port);
                ADD_PORT(all, port);
                bname = "axp";
                break;
            case SOC_BLK_CPIC:
            case SOC_BLK_CMIC:
                pno = 0;
                si->cmic_port = port;
                si->cmic_block = blk;
                SOC_PBMP_PORT_ADD(si->cmic_bitmap, port);
                ADD_PORT(all, port);
                break;
            case SOC_BLK_PGW_CL:
                /* don't use the name from this block */
                blktype = old_blktype;
                bname = old_bname;
                break;
            default:
                pno = 0;
                break;
            }

            si->block_valid[blk] += 1;
            if (si->block_port[blk] < 0) {
                si->block_port[blk] = port;
            }
            SOC_PBMP_PORT_ADD(si->block_bitmap[blk], port);
        }

        if (bname[0] == '?') {
            pno = port;
        }
        sal_snprintf(si->port_name[port], sizeof(si->port_name[port]),
                     "%s%d", bname, pno);
        si->port_type[port] = blktype;
        si->port_offset[port] = pno;
#ifdef BCM_SHADOW_SUPPORT
        if (SOC_IS_SHADOW(unit)) {
            /* Add ports 1 through 8 to the MACSEC blocks */
            /* Add ports 9 and 13 to the IL blocks */
            blk = 0;
            while (SOC_BLOCK_INFO(unit, blk).type != -1) {
                if ((SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_MS_ISEC) || 
                    (SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_MS_ESEC)) {
                    if (((SOC_BLOCK_INFO(unit, blk).number == 0) && (port >= 1) 
                        && (port <= 4)) || 
                        ((SOC_BLOCK_INFO(unit, blk).number == 1) && (port >= 5) 
                        && (port <= 8))) {
                        si->block_valid[blk] += 1;
                        if (si->block_port[blk] < 0) {
                            si->block_port[blk] = port;
                        }
                        SOC_PBMP_PORT_ADD(si->block_bitmap[blk], port);
                    }
                    if ((SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_MS_ISEC) && 
                        (SOC_BLOCK_INFO(unit, blk).number == 0)) {
                        si->ms_isec_block[0] = blk;
                    }
                    if ((SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_MS_ISEC) && 
                        (SOC_BLOCK_INFO(unit, blk).number == 1)) {
                        si->ms_isec_block[1] = blk;
                    }
                    if ((SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_MS_ESEC) && 
                        (SOC_BLOCK_INFO(unit, blk).number == 0)) {
                        si->ms_esec_block[0] = blk;
                    }
                    if ((SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_MS_ESEC) && 
                        (SOC_BLOCK_INFO(unit, blk).number == 1)) {
                        si->ms_esec_block[1] = blk;
                    }
                }
                if (SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_IL) {
                    if (((SOC_BLOCK_INFO(unit, blk).number == 0) && (port == 9))
                        || ((SOC_BLOCK_INFO(unit, blk).number == 1) && 
                        (port == 13))) {
                        if (!SOC_PBMP_MEMBER(si->il.disabled_bitmap, port)) {
                            si->block_valid[blk] += 1;
                        }
                        if (si->block_port[blk] < 0) {
                            si->block_port[blk] = port;
                        }
                        SOC_PBMP_PORT_ADD(si->block_bitmap[blk], port);
                    }
                    if ((SOC_BLOCK_INFO(unit, blk).number == 0)) {
                        si->il_block[0] = blk;
                    }
                    if ((SOC_BLOCK_INFO(unit, blk).number == 1)) {
                        si->il_block[1] = blk;
                    }
                }
                if (SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_CW) {
                    si->block_valid[blk] += 1;
                    if ((SOC_BLOCK_INFO(unit, blk).number == 0)) {
                        si->cw = blk;
                    }
                }
                blk++;
            }
        }
#endif /* BCM_SHADOW_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
        if (SOC_IS_KATANA2(unit)) {
            if (soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                SOC_BLK_TXLP, &blk, NULL) == SOC_E_NONE) {
                /* Is a TXLP Port */
                si->block_valid[blk] += 1;
                if (si->block_port[blk] < 0) {
                    si->block_port[blk] = port;
                }
                SOC_PBMP_PORT_ADD(si->block_bitmap[blk], port);
            }
            if (soc_kt2_linkphy_port_reg_blk_idx_get(unit, port,
                SOC_BLK_RXLP, &blk, NULL) == SOC_E_NONE) {
                /* Is a RXLP Port */
                si->block_valid[blk] += 1;
                if (si->block_port[blk] < 0) {
                    si->block_port[blk] = port;
                }
                SOC_PBMP_PORT_ADD(si->block_bitmap[blk], port);
            }
            if (port == KT2_OLP_PORT) {
                if (si->olp_port[0] == 1) {
                    si->block_valid[si->olp_block] = 1;
                    SOC_DRIVER(unit)->port_info[KT2_OLP_PORT].blk = 
                                                              si->olp_block;
                    SOC_DRIVER(unit)->port_info[KT2_OLP_PORT].bindex = 0;
                    si->block_port[si->olp_block] = port; 
                    SOC_PBMP_PORT_REMOVE(si->block_bitmap[mxq7_blk], port);
                    SOC_PBMP_PORT_ADD(si->block_bitmap[si->olp_block], port);
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "OLP Port Configuration blk=%d"
                                            "schain=%d cmic=%d\n"),
                                 SOC_DRIVER(unit)->port_info[KT2_OLP_PORT].blk,
                                 SOC_DRIVER(unit)->block_info[si->olp_block].
                                 schan,
                                 SOC_DRIVER(unit)->block_info[si->olp_block].
                                 cmic));
                } else {
                    si->block_valid[mxq7_blk] = 1;
                    SOC_DRIVER(unit)->port_info[KT2_OLP_PORT].blk = mxq7_blk;
                    SOC_DRIVER(unit)->port_info[KT2_OLP_PORT].bindex = 3;
                    si->block_port[mxq7_blk] = port; 
                    SOC_PBMP_PORT_ADD(si->block_bitmap[mxq7_blk], port);
                    SOC_PBMP_PORT_REMOVE(si->block_bitmap[si->olp_block], port);
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "OLP Port Configuration blk=%d"
                                            "schain=%d cmic=%d\n"),
                                 SOC_DRIVER(unit)->port_info[KT2_OLP_PORT].blk,
                                 SOC_DRIVER(unit)->block_info[mxq7_blk].schan,
                                 SOC_DRIVER(unit)->block_info[mxq7_blk].cmic));
               }
            }
        }
#endif /* BCM_KATANA2_SUPPORT */
    }
    port = max_port + 1;

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        int hp, hgc = 0;
        PBMP_HG_ITER(unit, hp) {
            if (!SOC_PBMP_MEMBER(SOC_PORT_DISABLED_BITMAP(unit, port), hp)) {
                hgc++;
            }
        }
        if (hgc > _SOC_TR3_MAX_HG_PORT_COUNT) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Port config error (Higig port count[%d] exceeded) !!\n"),
                                  hgc));
            return SOC_E_CONFIG;
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
    
    /* The last ports are the loopback port in the internal MMU ports*/
    if (si->internal_loopback) {
#if defined(BCM_ENDURO_SUPPORT)
        if(SOC_IS_ENDURO(unit)) {
            int port_temp;
            port_temp = port;
            for (port = 1; port <= 29; port++) {
                /* The LB port in Enduro could be port 1 or HG ports, not the last port */
                if (IS_LB_PORT(unit, port)) {
                    ADD_PORT(all, port);
                    si->port_type[port] = SOC_BLK_EPIPE;
                    si->port_offset[port] = 0;
                    sal_snprintf(si->port_name[port], sizeof(si->port_name[port]),
                                 "%s%d", "lb", 0);
                }
            }
            /* Since the LB port in Enduro is port 1, not the last port.
                 recover the port to previous value */
            port = port_temp;
        } else
#endif /* BCM_ENDURO_SUPPORT */
#if defined(BCM_KATANA_SUPPORT)
        if(SOC_IS_KATANAX(unit)) {

            int port_temp;
            port_temp = port;
            for (port = 35; port <= 41; port++) {
                if (IS_LB_PORT(unit, port)) {
                    ADD_PORT(all, port);
                    si->port_type[port] = SOC_BLK_EPIPE;
                    si->port_offset[port] = 0;
                    sal_snprintf(si->port_name[port], sizeof(si->port_name[port]),
                                 "%s%d", "lb", (port - 35));
                }
            }
            port = port_temp;
        } else
#endif /* BCM_KATANA_SUPPORT */
        {
            ADD_PORT(all, port);
            si->port_type[port] = SOC_BLK_EPIPE;
            si->port_offset[port] = 0;
            sal_snprintf(si->port_name[port], sizeof(si->port_name[port]),
                         "%s%d", "lb", 0);
            /* Loopback port is the last port. */
            port++;
        }
    }
    if(!SOC_IS_ENDURO(unit) && !SOC_IS_KATANAX(unit)) {
        if (si->internal_loopback) {
            ADD_PORT(mmu, port);
            si->port_type[port] = SOC_BLK_MMU;
            si->port_offset[port] = 0;
            sal_snprintf(si->port_name[port], sizeof(si->port_name[port]),
                         "%s%d", "mmu", 0);
            port++;
            ADD_PORT(mmu, port);
            si->port_type[port] = SOC_BLK_MMU;
            si->port_offset[port] = 1;
            sal_snprintf(si->port_name[port], sizeof(si->port_name[port]),
                         "%s%d", "mmu", 1);
            port++;
        }
    }
     /* Cache the port count from the above loop.
        NOTE: for future code changes: 
              Do not modify the 'port' variable before the below assignment */
    si->port_num = port;

    for (port = 0; port < SOC_MAX_NUM_PP_PORTS; port++) {
        if (SOC_PBMP_MEMBER(pbmp_pp, port)) {
            ADD_PORT(pp, port);
        }
    }
#undef  ADD_PORT

    /* Generate logical to physical and mmu to physical port mapping */
    if (soc_feature(unit, soc_feature_logical_port_num)) {
        for (port = 0; port < phy_port; port++) {
            si->port_l2p_mapping[port] = -1;
            si->port_m2p_mapping[port] = -1;
        }
        for (port = 0; port < phy_port; port++) {
            if (si->port_p2l_mapping[port] != -1) {
                si->port_l2p_mapping[si->port_p2l_mapping[port]] = port;
            }
            if (si->port_p2m_mapping[port] != -1) {
                si->port_m2p_mapping[si->port_p2m_mapping[port]] = port;
            }
        }
        /* Cleanup */
        for (port = 0; port < phy_port; port++) {
            if (si->flex_port_p2l_mapping[port] == -1) {
                si->port_speed_max[si->port_p2l_mapping[port]] = 0;
            }
        }
    }


    /* some things need to be found in the block table */
    si->arl_block = -1;
    si->mmu_block = -1;
    si->mcu_block = -1;

    si->ipipe_block = -1;
    si->ipipe_hi_block = -1;
    si->epipe_block = -1;
    si->epipe_hi_block = -1;
    si->bsafe_block = -1;
    si->esm_block = -1;
    si->otpc_block = -1;
    si->axp_block = -1;
    si->top_block = -1;
    si->ism_block = -1;
    si->etu_block = -1;
    si->ibod_block = -1;

    si->igr_block = -1;
    si->egr_block = -1;
    si->bse_block = -1;
    si->cse_block = -1;
    si->hse_block = -1;
    si->iproc_block = -1;

    sal_memset(si->has_block, 0, sizeof(soc_block_t) * COUNTOF(si->has_block));

    for (blk = 0; blk < SOC_MAX_NUM_CI_BLKS; blk++) {
        si->ci_block[blk] = -1;
    }

    for (blk = 0; SOC_BLOCK_INFO(unit, blk).type >= 0; blk++) {
        blktype = SOC_BLOCK_INFO(unit, blk).type;
        instance = SOC_BLOCK_INFO(unit, blk).number;
        si->has_block[blk] = blktype;
        sal_snprintf(instance_string, sizeof(instance_string), "%d",
                     SOC_BLOCK_INFO(unit, blk).number);
        switch (blktype) {
        case SOC_BLK_ARL:
            si->arl_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_MMU:
            si->mmu_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_MCU:
            si->mcu_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_IPIPE:
            si->ipipe_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_CI:
            si->ci_block[instance] = blk;
            si->block_valid[blk] += 1;
            si->block_port[blk] = (SOC_REG_ADDR_INSTANCE_MASK | instance);
            SOC_PBMP_PORT_ADD(si->block_bitmap[blk], instance);
            break;
        case SOC_BLK_IPIPE_HI:
            si->ipipe_hi_block = blk;
            si->block_valid[blk] += 1;
            SOC_PBMP_OR(si->block_bitmap[blk],PBMP_ST_ALL(unit));
            SOC_PBMP_OR(si->block_bitmap[blk],PBMP_HG_ALL(unit));
            SOC_PBMP_OR(si->block_bitmap[blk],PBMP_CMIC(unit));
            break;
        case SOC_BLK_EPIPE:
            si->epipe_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_EPIPE_HI:
            si->epipe_hi_block = blk;
            si->block_valid[blk] += 1;
            SOC_PBMP_OR(si->block_bitmap[blk],PBMP_ST_ALL(unit));
            SOC_PBMP_OR(si->block_bitmap[blk],PBMP_HG_ALL(unit));
            SOC_PBMP_OR(si->block_bitmap[blk],PBMP_CMIC(unit));
            break;
#ifdef BCM_SHADOW_SUPPORT
        case SOC_BLK_BSE: /* Aliased with IL */
            sal_strcpy(si->block_name[blk], "il"); 
            break;
       case SOC_BLK_CSE: /* Aliased with MS_ISEC */
           sal_strcpy(si->block_name[blk], "ms_isec"); 
            break;
        case SOC_BLK_HSE: /* Aliased with MS_ESEC */
            sal_strcpy(si->block_name[blk], "ms_esec");
            break;
        case SOC_BLK_SYS: /* Aliased with CW */
            sal_strcpy(si->block_name[blk], "cw");
            break;
#endif /* BCM_SHADOW_SUPPORT */
        case SOC_BLK_BSAFE:
            si->bsafe_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_OTPC:
            si->otpc_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_ESM:
            si->esm_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_PGW_CL:
            si->pgw_cl_block[instance] = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_PORT_GROUP4:
            si->pg4_block[instance] = blk;
            sal_strcpy(instance_string, instance ? "_y" : "_x");
            si->block_port[blk] = (SOC_REG_ADDR_INSTANCE_MASK | instance);
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_PORT_GROUP5:
            si->pg5_block[instance] = blk;
            sal_strcpy(instance_string, instance ? "_y" : "_x");
            si->block_port[blk] = (SOC_REG_ADDR_INSTANCE_MASK | instance);
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_TOP:
            si->top_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_SER:
            si->ser_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_AXP:
            si->axp_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_ISM:
            si->ism_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_ETU:
            si->etu_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_IBOD:
            si->ibod_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_LLS:
            si->lls_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_CES:
            si->ces_block = blk;
            si->block_valid[blk] += 1;
            break;
         case SOC_BLK_IPROC:
            si->iproc_block = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_PMQ:
            if(SOC_IS_GREYHOUND(unit)){
                if (si->port_p2l_mapping[2]== -1){
                /* TSC0Q is powered down in some skus,
                it could be tell by checking the valid of physical port 2. */
                    break;
                }
            }
            si->pmq_block[instance] = blk;
            si->block_valid[blk] += 1;
            break;
        case SOC_BLK_AVS:
            si->avs_block = blk;
            si->block_valid[blk] += 1;
        }
        sal_snprintf(si->block_name[blk], sizeof(si->block_name[blk]),
                     "%s%s",
                     soc_block_name_lookup_ext(blktype, unit),
                     instance_string);
    }
    si->block_num = blk;

    /*
     * Calculate the mem_block_any array for this configuration
     * The "any" block is just the first one enabled
     */
    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
        si->mem_block_any[mem] = -1;
        if (!SOC_MEM_IS_VALID(unit, mem)) {
            continue;
        }
        SOC_MEM_BLOCK_ITER(unit, mem, blk) {
            si->mem_block_any[mem] = blk;
            break;
        }
    }

    switch (drv_dev_id) {
#ifdef BCM_TRIDENT_SUPPORT
    case BCM56840_DEVICE_ID:
        soc_trident_num_cosq_init(unit);
        break;
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    case BCM56850_DEVICE_ID:
        soc_trident2_num_cosq_init(unit);
        break;
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    case BCM56440_DEVICE_ID:
    case BCM56441_DEVICE_ID:
    case BCM56442_DEVICE_ID:
    case BCM56443_DEVICE_ID:
    case BCM56445_DEVICE_ID:
    case BCM56446_DEVICE_ID:
    case BCM56447_DEVICE_ID:
    case BCM56448_DEVICE_ID:
    case BCM56449_DEVICE_ID:
    case BCM56240_DEVICE_ID:
    case BCM56241_DEVICE_ID:
    case BCM56242_DEVICE_ID:
    case BCM56243_DEVICE_ID:
    case BCM56245_DEVICE_ID:
    case BCM56246_DEVICE_ID:
    case BCM55440_DEVICE_ID:
    case BCM55441_DEVICE_ID:    
        soc_katana_num_cosq_init(unit);
        break;
#endif /* BCM_KATANA_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
    case BCM55450_DEVICE_ID:
    case BCM55455_DEVICE_ID:
    case BCM56248_DEVICE_ID:
    case BCM56450_DEVICE_ID:
    case BCM56452_DEVICE_ID:
    case BCM56454_DEVICE_ID:
    case BCM56455_DEVICE_ID:
    case BCM56456_DEVICE_ID:
    case BCM56457_DEVICE_ID:
    case BCM56458_DEVICE_ID:
        SOC_IF_ERROR_RETURN(soc_katana2_num_cosq_init(unit));
        break;
#endif /* BCM_KATANA2_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
    case BCM56640_DEVICE_ID:
    case BCM56643_DEVICE_ID:
    case BCM56644_DEVICE_ID:
    case BCM56648_DEVICE_ID:
    case BCM56649_DEVICE_ID:
    case BCM56540_DEVICE_ID:
    case BCM56541_DEVICE_ID:
    case BCM56542_DEVICE_ID:
    case BCM56543_DEVICE_ID:
    case BCM56544_DEVICE_ID:
    case BCM56545_DEVICE_ID:
    case BCM56546_DEVICE_ID:
    case BCM56044_DEVICE_ID:
    case BCM56045_DEVICE_ID:
    case BCM56046_DEVICE_ID:
        soc_triumph3_init_num_cosq(unit);
        break;
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_HELIX4_SUPPORT
    case BCM56548_DEVICE_ID:
    case BCM56547_DEVICE_ID:
    case BCM56347_DEVICE_ID:
    case BCM56346_DEVICE_ID:
    case BCM56344_DEVICE_ID:
    case BCM56342_DEVICE_ID:
    case BCM56340_DEVICE_ID:
    case BCM56049_DEVICE_ID:
    case BCM56048_DEVICE_ID:
    case BCM56047_DEVICE_ID:
    case BCM56042_DEVICE_ID:
    case BCM56041_DEVICE_ID:
    case BCM56040_DEVICE_ID:
        soc_hx4_init_num_cosq(unit);
        break;
#endif
    default:
        break;
    }

    soc_esw_dport_init(unit);

#ifdef INCLUDE_RCPU
    if (SOC_IS_RCPU_ONLY(unit)) {
        soc_rcpu_cfg_t cfg;
        if (SOC_E_NONE == soc_cm_get_rcpu_cfg(unit, &cfg)) {
            si->rcpu_port = cfg.port;
        } else {
            si->rcpu_port = -1;
        }
    }
#endif /* INCLUDE_RCPU */
    /* Allocate MMU LLS usage map for all valid ports.
     * The code below is moved from soc_attach() to
     * soc_info_config() as soc_attach() is not invoked
     * during subsequent "rc" after first initialization is complete.
     * The move is done to ensure that if the number of ports change
     * due to any config then the MMU LLS usage bmap
     * allocation is accounted for new ports.
     */
#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANA(unit)) {
        soc_kt_lls_bmap_alloc(unit);
    }
#endif /* BCM_KATANA_SUPPORT */

#if defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_KATANA2(unit)) {
        soc_kt2_lls_bmap_alloc(unit);
    }
#endif /* BCM_KATANA2_SUPPORT */
    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_max_supported_vrf_get
 * Purpose:
 *      Utility routine for reading maximum supported  vrf value for the device.
 * Parameters:
 *      unit - (IN) Unit number.
 *      value  - (OUT) Max vrf value.
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
soc_max_supported_vrf_get(int unit, uint16 *value)
{
    int bit_num = 0;
#if defined(BCM_TRIUMPH_SUPPORT)
    uint16              dev_id;
    uint8               rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);
#endif /* BCM_TRIUMPH_SUPPORT */

    if (NULL == value) {
        return (SOC_E_PARAM);
    }
#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_TRIUMPH2(unit)) {
        if (soc_mem_index_count(unit, EXT_IPV6_64_DEFIPm)) {
            bit_num = soc_mem_field_length(unit, EXT_IPV6_64_DEFIPm, VRF_LOf);
        } else {
            bit_num = soc_mem_field_length(unit, L3_IIFm, VRFf);
        }
    } else if (SOC_IS_TRIUMPH3(unit)) {
        bit_num = soc_mem_field_length(unit, L3_IIFm, VRFf);
    } else if (SOC_IS_VALKYRIE(unit)) {
        bit_num = 8; /* Hardcoded to 256 VRFs */
    } else if (SOC_IS_ENDURO(unit)) {
        if ((dev_id == BCM56230_DEVICE_ID) || (dev_id == BCM56231_DEVICE_ID)) {
            bit_num = 5; /* Hardcoded to 32 VRFs for Dagger */            
        } else {
            bit_num = 7; /* Hardcoded to 128 VRFs */
        }
    } else if (SOC_IS_APOLLO(unit) || SOC_IS_VALKYRIE2(unit)) {
        bit_num = soc_mem_field_length(unit, L3_IIFm, VRFf);
    } else if (SOC_IS_TD_TT(unit) || (SOC_IS_KATANAX(unit))) {
        bit_num = soc_mem_field_length(unit, L3_IIFm, VRFf);
    }
#endif /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_FIREBOLT2_SUPPORT) || defined(BCM_RAPTOR_SUPPORT) || \
    defined(BCM_SCORPION_SUPPORT)
    if (SOC_IS_FIREBOLT2(unit) || SOC_IS_RAVEN(unit) || 
        SOC_IS_SC_CQ(unit)) {
         bit_num = soc_mem_field_length(unit, VLAN_TABm, VRF_IDf);
    }
#endif  /* BCM_FIREBOLT2_SUPPORT || BCM_RAVEN_SUPPORT || BCM_SCORPION_SUPPORT */
    *value = (( 1 << bit_num ) - 1);
    return (SOC_E_NONE);
}


/*
 * Function:
 *      soc_max_supported_addr_class_get
 * Purpose:
 *      Utility routine for reading maximum supported  address
 *      class for the device.
 * Parameters:
 *      unit   - (IN) Unit number.
 *      value  - (OUT) Max address class value.
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
soc_max_supported_addr_class_get(int unit, uint16 *value)
{
    int bit_num = 0;

    if (NULL == value) {
        return (SOC_E_PARAM);
    }
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
      /* fill in for SHADOW */
    } else 
#endif /* BCM_SHADOW_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
    if (SOC_IS_TRX(unit)) {
        if (soc_feature(unit, soc_feature_ism_memory)) {
            bit_num = soc_mem_field_length(unit, L2_ENTRY_1m, CLASS_IDf);
        } else {
            bit_num = soc_mem_field_length(unit, L2Xm, CLASS_IDf);
        }
    } else
#endif  /* BCM_TRX_SUPPORT */
#ifdef BCM_SIRIUS_SUPPORT
    if (SOC_IS_SIRIUS(unit)) {
      /* fill in for sirius */
    } else 
#endif /* BCM_SIRIUS_SUPPORT */
#ifdef BCM_ESW_SUPPORT
    if (SOC_MEM_FIELD_VALID(unit, L2Xm, MAC_BLOCK_INDEXf)) {
         bit_num = soc_mem_field_length(unit, L2Xm, MAC_BLOCK_INDEXf);
    }
#endif /* BCM_ESW_SUPPORT */
    *value = (( 1 << bit_num ) - 1);

    return (SOC_E_NONE);
}

/*
 * Function:
 *      soc_max_supported_overlaid_addr_class_get
 * Purpose:
 *      Utility routine for reading maximum supported address
 *      class with overlay for the device.
 * Parameters:
 *      unit   - (IN) Unit number.
 *      value  - (OUT) Max address class value.
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
soc_max_supported_overlaid_addr_class_get(int unit, uint16 *value)
{
    int bit_num = 0;

    if (NULL == value) {
        return (SOC_E_PARAM);
    }

#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_MEM_FIELD_VALID(unit, L2Xm, CLASS_IDf)) {
        bit_num = soc_mem_field_length(unit, L2Xm, CLASS_IDf);
        if (SOC_MEM_FIELD_VALID(unit, L2Xm, PRIf)) {
            bit_num += soc_mem_field_length(unit, L2Xm, PRIf);
        }
    }
#endif  /* BCM_TRX_SUPPORT */

    *value = (( 1 << bit_num ) - 1);

    return (SOC_E_NONE);
}

/*
 * Function:
 *      soc_max_supported_extended_addr_class_get
 * Purpose:
 *      Utility routine for reading maximum supported extended address
 *      class for the device.
 * Parameters:
 *      unit   - (IN) Unit number.
 *      value  - (OUT) Max address class value.
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
soc_max_supported_extended_addr_class_get(int unit, uint16 *value)
{
    int bit_num = 0;

    if (NULL == value) {
        return (SOC_E_PARAM);
    }
    if (SOC_MEM_FIELD_VALID(unit, L3_ENTRY_1m, CLASS_IDf)) {
        bit_num = soc_mem_field_length(unit, L3_ENTRY_1m, CLASS_IDf);
    }

    *value = (( 1 << bit_num ) - 1);
    return (SOC_E_NONE);
}

/*
 * Function:
 *      soc_max_supported_intf_class_get
 * Purpose:
 *      Utility routine for reading maximum supported  interface
 *      class for the device.
 * Parameters:
 *      unit   - (IN) Unit number.
 *      value  - (OUT) Maximum  interface class value.
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
soc_max_supported_intf_class_get(int unit, uint16 *value)
{
    int bit_num = 0;

    if (NULL == value) {
        return (SOC_E_PARAM);
    }

#if defined(BCM_TRIUMPH_SUPPORT)
    if (SOC_IS_TR_VL(unit)) {
         bit_num = soc_mem_field_length(unit, VLAN_TABm, VLAN_CLASS_IDf);
    }
#endif  /* BCM_TRIUMPH_SUPPORT */
#if defined(BCM_SCORPION_SUPPORT)
    if (SOC_IS_SC_CQ(unit)) {
         bit_num = soc_mem_field_length(unit, VLAN_TABm, CLASS_IDf);
    }
#endif  /* BCM_SCORPION_SUPPORT */
#if defined(BCM_FIREBOLT2_SUPPORT)
    if (SOC_IS_FIREBOLT2(unit)) {
         bit_num = soc_mem_field_length(unit, PORT_TABm, VFP_PORT_GROUP_IDf);
    }
#endif  /* BCM_FIREBOLT2_SUPPORT */
    *value = (( 1 << bit_num ) - 1);
    return (SOC_E_NONE);
}


/*
 * Function:
 *      soc_max_vrf_set
 * Purpose:
 *      Utility routine for configuring max vrf value for the device.
 * Parameters:
 *      unit   - (IN) Unit number.
 *      value  - (IN) Max vrf value.
 * NOTE:  Value must be equal or less than maximum supported vrf id,
 *        otherwise maximum supported vrf id is used.
 *        Value must be power of two - 1, otherwise routine calculates closest
 *        power of 2 - 1 value less than requested one.
 * Returns:
       BCM_E_NONE
 */
int
soc_max_vrf_set(int unit, int value)
{
    uint16 max_supported;   /* Maximum supported vrf id.           */
    uint16 max_vrf_id;      /* Adjusted max vrf value.             */
    uint8  bit_number;      /* Number of bits in configured value. */


    SOC_IF_ERROR_RETURN(soc_max_supported_vrf_get(unit, &max_supported));

    if (value > max_supported) {
       return (SOC_E_PARAM);
    }

    /* Find number of bits required to cover requested value. */
    for (bit_number = 0; bit_number < 16; bit_number++) {
        max_vrf_id = ((1 << bit_number) - 1);
        if (value == max_vrf_id) {
            break;
        }
    }

    if (max_vrf_id != value) {
        return (SOC_E_PARAM);
    }

    SOC_VRF_MAX_SET(unit, max_vrf_id);
    return (SOC_E_NONE);
}

/*
 * Function:
 *      soc_xport_type_verify
 * Purpose:
 *      Check all 10G ports to see if the encapsulation is changed from
 *      the configured selection.  Update the SOC port data structures to
 *      match the HW encapsulation selection.
 * Parameters:
 *      unit - XGS unit #.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      WARM_BOOT mode only
 */
STATIC int
soc_xport_type_verify(int unit)
{
    soc_pbmp_t pbmp_10g_plus;
    soc_port_t port;
    int        mode;
    mac_driver_t *macdp;
#ifdef  BCM_GREYHOUND_SUPPORT
    uint32 hg2_mode = 0;
#endif

    if (!soc_feature(unit, soc_feature_xport_convertible)) {
        /* Nothing to do */
        return SOC_E_NONE;
    }

    SOC_PBMP_ASSIGN(pbmp_10g_plus, PBMP_XE_ALL(unit));
    SOC_PBMP_OR(pbmp_10g_plus, PBMP_HG_ALL(unit));
    
    SOC_PBMP_ITER(pbmp_10g_plus, port) {
        SOC_IF_ERROR_RETURN(soc_mac_probe(unit, port, &macdp));
        SOC_IF_ERROR_RETURN(MAC_ENCAP_GET(macdp, unit, port, &mode));
#ifdef  BCM_GREYHOUND_SUPPORT
        if (SOC_IS_GREYHOUND(unit)){
            /* Need more check for MAC mode at IEEE encap mode if the port is 
             * actually at HG2-LITE encap mode.
             */
            if (mode == SOC_ENCAP_IEEE){
                SOC_IF_ERROR_RETURN(soc_greyhound_pgw_encap_field_get(unit, 
                    port, HIGIG2_MODEf, &hg2_mode));
                mode = (hg2_mode) ? SOC_ENCAP_HIGIG2_LITE : mode;
            }

            if ((IS_XE_PORT(unit, port) && (mode != SOC_ENCAP_IEEE)) || 
                    (IS_HG_PORT(unit, port) && (mode == SOC_ENCAP_IEEE))) {
                soc_gh_xport_type_update(unit, port, mode);
            }
            if (mode == SOC_ENCAP_HIGIG2 || mode == SOC_ENCAP_HIGIG2_LITE) {
                SOC_HG2_ENABLED_PORT_ADD(unit, port);
            }
        } else 
#endif  /* GREYHOUND */
        {
            if (IS_XE_PORT(unit, port) && (mode != SOC_ENCAP_IEEE)) {
                soc_xport_type_update(unit, port, TRUE);
            } else if (IS_HG_PORT(unit, port) && (mode == SOC_ENCAP_IEEE)) {
                soc_xport_type_update(unit, port, FALSE);
            }

            if (mode == SOC_ENCAP_HIGIG2) {
                if (IS_HG_PORT(unit, port)) {
                    SOC_HG2_ENABLED_PORT_ADD(unit, port);
                } else {
                    /* This should not happen */
                    return SOC_E_INTERNAL;
                }
            }
        }
    }

    return SOC_E_NONE;
}

#if defined(BCM_RAVEN_SUPPORT) || defined (BCM_TRIUMPH2_SUPPORT)
/*
 * Function:
 *      soc_stport_type_verify
 * Purpose:
 *      Check all stackable ports to see if the encapsulation is
 *      changed from the configured selection.  Update the SOC port
 *      data structures to match the HW encapsulation selection.
 * Parameters:
 *      unit - XGS unit #.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      WARM_BOOT mode only
 */
STATIC int
soc_stport_type_verify(int unit)
{
    port_tab_entry_t ptab;
    soc_port_t       port;
    soc_info_t       *si = &SOC_INFO(unit);
    soc_reg_t        ehg_rx_reg;
    soc_field_t      port_type;
    uint32           ehg_rx_ctrl = 0;
    int              to_higig = FALSE;
    soc_mem_t        mem;
    ing_physical_port_table_entry_t ptab_entry;
    void             *pentry;
 
    if (SOC_IS_GREYHOUND(unit)) {
        /* GH's port type verify will be done in soc_xport_type_verify */
        return SOC_E_NONE;
    }

    /* Select correct port table field for the Higig2 configuration. */
    port_type = (SOC_IS_ENDURO(unit) || SOC_IS_TRIUMPH2(unit) || 
                 SOC_IS_APOLLO(unit) || SOC_IS_VALKYRIE2(unit)  ||
                 SOC_IS_HURRICANEX(unit) || SOC_IS_TD_TT(unit) ||
                 SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit)) ? 
                 PORT_TYPEf : HIGIG_PACKETf;

    ehg_rx_reg =
        SOC_IS_TRIUMPH3(unit) ? PORT_EHG_RX_CONTROLr : EHG_RX_CONTROLr;

    PBMP_PORT_ITER(unit, port) {
        /* Handle embedded Higig cases */
        if (SOC_REG_IS_VALID(unit, ehg_rx_reg)) {
            SOC_IF_ERROR_RETURN
                (soc_reg32_get(unit, ehg_rx_reg, port, 0, &ehg_rx_ctrl));

            if (2 == soc_reg_field_get(unit, ehg_rx_reg,
                                       ehg_rx_ctrl, MODEf)) {
                /* Raptor-style stacking -- HIGIG2_LITE */
                if (IS_HG_PORT(unit, port)) {
                    SOC_HG2_ENABLED_PORT_REMOVE(unit, port);
                } else {
                    SOC_PBMP_PORT_ADD(si->hl.bitmap, port);
                }
            }

            if (0 != soc_reg_field_get(unit, ehg_rx_reg,
                                       ehg_rx_ctrl, MODEf)) {
                /* Embedded Higig */
                SOC_PBMP_PORT_ADD(si->st.bitmap, port);
                SOC_PBMP_PORT_REMOVE(si->ether.bitmap, port);
                continue;
            }
            /* Else, fall through to below */
        }
        if (SOC_IS_KATANA2(unit)) {
            mem = ING_PHYSICAL_PORT_TABLEm;
            sal_memset(&ptab_entry, 0, sizeof(ptab_entry));
            pentry = (void *)&ptab_entry;
        } else {
            mem = PORT_TABm;
            sal_memset(&ptab, 0, sizeof(ptab));
            pentry = (void *)&ptab;
        }

        if (!IS_HG_PORT(unit, port)) {
            SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem,
                                             MEM_BLOCK_ANY, port, pentry));
            to_higig =
                (0 != soc_mem_field32_get(unit, mem, pentry, port_type));

            /* ST port settings */
            if (to_higig) {
                SOC_PBMP_PORT_ADD(si->st.bitmap, port);
                SOC_PBMP_PORT_ADD(si->hl.bitmap, port);  
                SOC_PBMP_PORT_REMOVE(si->ether.bitmap, port);
                if (!IS_XE_PORT(unit, port)) {
                    SOC_HG2_ENABLED_PORT_ADD(unit, port);
                }
            } else {
                SOC_PBMP_PORT_REMOVE(si->st.bitmap, port);
                SOC_PBMP_PORT_REMOVE(si->hl.bitmap, port);
                SOC_PBMP_PORT_ADD(si->ether.bitmap, port);
                if (!IS_XE_PORT(unit, port)) {
                    SOC_HG2_ENABLED_PORT_REMOVE(unit, port);
                }
            }
        }
    }

    return SOC_E_NONE;
}
#endif /* defined(BCM_RAVEN_SUPPORT) || defined (BCM_TRIUMPH2_SUPPORT) */

/*
 * Function:
 *      soc_port_type_verify
 * Purpose:
 *      Check all ports to see if the encapsulation is changed from
 *      the configured selection.  Update the SOC port data structures to
 *      match the HW encapsulation selection.
 * Parameters:
 *      unit - XGS unit #.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      WARM_BOOT mode only
 */
int
soc_port_type_verify(int unit)
{
    SOC_IF_ERROR_RETURN(soc_xport_type_verify(unit));

#if defined(BCM_RAVEN_SUPPORT) || defined (BCM_TRIUMPH2_SUPPORT)
    /* Raven HG or embedded HG versions here */
    SOC_IF_ERROR_RETURN(soc_stport_type_verify(unit));
#endif /* defined(BCM_RAVEN_SUPPORT) || defined (BCM_TRIUMPH2_SUPPORT) */

    return SOC_E_NONE;
}

#ifdef BCM_CMICM_SUPPORT
int
cmicm_pcie_deemphasis_set(int unit, uint16 phy_addr) {
    uint16  phy_reg_val;
    uint32  cmic_reg_val;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "Changing the CMICm PCIe SerDes deemphasis on unit %d\n"), unit));
    
    /* Change the transmitter setting */
    SOC_IF_ERROR_RETURN(soc_miim_write(unit, phy_addr, 0x1f, 0x8610));
    SOC_IF_ERROR_RETURN(soc_miim_read(unit, phy_addr, 0x17, &phy_reg_val));
    phy_reg_val = (phy_reg_val & 0xf0ff) | 0x700;
    SOC_IF_ERROR_RETURN(soc_miim_write(unit, phy_addr, 0x17, phy_reg_val));
    
    /* Force the PCIe link to retrain */
    cmic_reg_val = 0x1004;
    SOC_IF_ERROR_RETURN(WRITE_CMIC_PCIE_CFG_ADDRESSr(unit, cmic_reg_val));
    SOC_IF_ERROR_RETURN(READ_CMIC_PCIE_CFG_READ_DATAr(unit, &cmic_reg_val));
    cmic_reg_val &= ~0x4000;
    SOC_IF_ERROR_RETURN(WRITE_CMIC_PCIE_CFG_WRITE_DATAr(unit, cmic_reg_val));        
    cmic_reg_val |= 0x4000;
    SOC_IF_ERROR_RETURN(WRITE_CMIC_PCIE_CFG_WRITE_DATAr(unit, cmic_reg_val));        
    cmic_reg_val &= ~0x4000;
    SOC_IF_ERROR_RETURN(WRITE_CMIC_PCIE_CFG_WRITE_DATAr(unit, cmic_reg_val));        

    /* Wait a short while for the retraining to complete */
    sal_usleep(1000);
    
    return SOC_E_NONE;
}
#endif /* BCM_CMICM_SUPPORT */

int cmic_pcie_cdr_bw_adj(int unit, uint16 phy_addr) {

    SOC_IF_ERROR_RETURN(soc_miim_write(unit, phy_addr, 0x1f, 0x8630));
    SOC_IF_ERROR_RETURN(soc_miim_write(unit, phy_addr, 0x13, 0x190));
    SOC_IF_ERROR_RETURN(soc_miim_write(unit, phy_addr, 0x19, 0x191));

    return SOC_E_NONE;
}

int
cmice_pcie_deemphasis_set(int unit, uint16 phy_addr) {
    uint16  phy_reg_val;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "Changing the CMICe PCIe SerDes deemphasis on unit %d\n"), unit));
    
    /* Change the transmitter setting */
    SOC_IF_ERROR_RETURN(soc_miim_write(unit, phy_addr, 0x1f, 0x8610));
    SOC_IF_ERROR_RETURN(soc_miim_read(unit, phy_addr, 0x17, &phy_reg_val));
    phy_reg_val = (phy_reg_val & 0xf0ff) | 0x700;
    SOC_IF_ERROR_RETURN(soc_miim_write(unit, phy_addr, 0x17, phy_reg_val));
    
    /* Wait a short while */
    sal_usleep(1000);
    
    return SOC_E_NONE;
}

#ifdef BCM_IPROC_SUPPORT
int iproc_pcie_cdr_bw_adj(int unit, uint16 phy_addr) {
    uint32  cmic_reg_val;

    /* Enable the iProc internal MDIO interface */
    cmic_reg_val = 0;
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_CONTROLr, &cmic_reg_val, 
                      MDCDIVf, 0x7f);
    SOC_IF_ERROR_RETURN(WRITE_CHIPCOMMONB_MII_MANAGEMENT_CONTROLr(unit, 
                      cmic_reg_val));
    sal_usleep(10000);

    /* Set Block */
    cmic_reg_val = 0;
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, SBf, 0x1);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, TAf, 0x2);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, OPf, 0x1);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, PAf, phy_addr);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, RAf, 0x1f);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, DATAf, 0x8630);
    SOC_IF_ERROR_RETURN(WRITE_CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr(unit, 
                      cmic_reg_val));
    sal_usleep(10000);

    /* Write reg 0x13 */
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, RAf, 0x13);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, DATAf, 0x190);
    SOC_IF_ERROR_RETURN(WRITE_CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr(unit, 
                      cmic_reg_val));
    sal_usleep(10000);

    /* Write reg 0x19 */
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, RAf, 0x19);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, DATAf, 0x191);
    SOC_IF_ERROR_RETURN(WRITE_CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr(unit, 
                      cmic_reg_val));
    sal_usleep(10000);

    return SOC_E_NONE;
}

int
iproc_pcie_deemphasis_set(int unit, uint16 phy_addr) {
    uint16  phy_reg_val;
    uint32  cmic_reg_val;

#ifdef BCM_HURRICANE2_SUPPORT
    /* DeEmphasis not needed for Hurricane2 */
    if (SOC_IS_HURRICANE2(unit)) {
        return SOC_E_NONE;
    }
#endif

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "Changing the iProc PCIe SerDes deemphasis on unit %d\n"), unit));

    /* Enable the iProc internal MDIO interface */
    cmic_reg_val = 0;
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_CONTROLr, &cmic_reg_val, 
                      MDCDIVf, 0x7f);
    SOC_IF_ERROR_RETURN(WRITE_CHIPCOMMONB_MII_MANAGEMENT_CONTROLr(unit, 
                      cmic_reg_val));
    sal_usleep(10000);
    
    /* Change the transmitter setting */
    cmic_reg_val = 0;
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, SBf, 0x1);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, TAf, 0x2);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, OPf, 0x1);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, PAf, phy_addr);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, RAf, 0x1f);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, DATAf, 0x8610);
    SOC_IF_ERROR_RETURN(WRITE_CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr(unit, 
                      cmic_reg_val));
    sal_usleep(10000);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, OPf, 0x2);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr,
                      &cmic_reg_val, RAf, 0x17);
    SOC_IF_ERROR_RETURN(WRITE_CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr(unit,
                      cmic_reg_val));
    sal_usleep(10000);
    SOC_IF_ERROR_RETURN(READ_CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr(unit,
                      &cmic_reg_val));
    sal_usleep(10000);
    phy_reg_val = soc_reg_field_get(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr,
                      cmic_reg_val, DATAf);
    phy_reg_val = (phy_reg_val & 0xf0ff) | 0x700;
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, SBf, 0x1);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, TAf, 0x2);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, OPf, 0x1);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, PAf, phy_addr);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, RAf, 0x17);
    soc_reg_field_set(unit, CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr, 
                      &cmic_reg_val, DATAf, phy_reg_val);
    SOC_IF_ERROR_RETURN(WRITE_CHIPCOMMONB_MII_MANAGEMENT_COMMAND_DATAr(unit, 
                      cmic_reg_val));

    /* Force the PCIe link to retrain */
    if (soc_cm_get_bus_type(unit) & SOC_DEV_BUS_ALT) {
        /* PCI-1 */
        cmic_reg_val = 0;
        soc_reg_field_set(unit, PAXB_1_CONFIG_IND_ADDRr, &cmic_reg_val,
                          PROTOCOL_LAYERf, 0x2);
        soc_reg_field_set(unit, PAXB_1_CONFIG_IND_ADDRr, &cmic_reg_val,
                          ADDRESSf, 0x4);
        SOC_IF_ERROR_RETURN(WRITE_PAXB_1_CONFIG_IND_ADDRr(unit, cmic_reg_val));
        SOC_IF_ERROR_RETURN(READ_PAXB_1_CONFIG_IND_DATAr(unit, &cmic_reg_val));
        cmic_reg_val &= ~0x4000;
        SOC_IF_ERROR_RETURN(WRITE_PAXB_1_CONFIG_IND_ADDRr(unit, cmic_reg_val));        
        cmic_reg_val |= 0x4000;
        SOC_IF_ERROR_RETURN(WRITE_PAXB_1_CONFIG_IND_ADDRr(unit, cmic_reg_val));        
        cmic_reg_val &= ~0x4000;
        SOC_IF_ERROR_RETURN(WRITE_PAXB_1_CONFIG_IND_ADDRr(unit, cmic_reg_val));        
    } else {
        /* PCI-0 */
        cmic_reg_val = 0;
        soc_reg_field_set(unit, PAXB_0_CONFIG_IND_ADDRr, &cmic_reg_val,
                          PROTOCOL_LAYERf, 0x2);
        soc_reg_field_set(unit, PAXB_0_CONFIG_IND_ADDRr, &cmic_reg_val,
                          ADDRESSf, 0x4);
        SOC_IF_ERROR_RETURN(WRITE_PAXB_0_CONFIG_IND_ADDRr(unit, cmic_reg_val));
        SOC_IF_ERROR_RETURN(READ_PAXB_0_CONFIG_IND_DATAr(unit, &cmic_reg_val));
        cmic_reg_val &= ~0x4000;
        SOC_IF_ERROR_RETURN(WRITE_PAXB_0_CONFIG_IND_ADDRr(unit, cmic_reg_val));        
        cmic_reg_val |= 0x4000;
        SOC_IF_ERROR_RETURN(WRITE_PAXB_0_CONFIG_IND_ADDRr(unit, cmic_reg_val));        
        cmic_reg_val &= ~0x4000;
        SOC_IF_ERROR_RETURN(WRITE_PAXB_0_CONFIG_IND_ADDRr(unit, cmic_reg_val));        
    }
    /* Wait a short while for the retraining to complete */
    sal_usleep(1000);
    
    return SOC_E_NONE;
}
#endif /* BCM_IPROC_SUPPORT */

/*
 * Function:
 *      soc_do_init
 * Purpose:
 *      Optionally reset, and initialize a StrataSwitch.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      reset - Boolean, if TRUE, device is reset.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Resets the SOC chip using the SCP reset provided by CMIC.
 *      Fields in soc_control_t are initialized to correspond to a SOC
 *      post-reset.  SOC interrupts are configured for normal chip
 *      operation.
 *
 *      This routine may be called after a device is attached
 *      or whenever a chip reset is required.
 *
 * IMPORTANT NOTE:
 *      Only the quickest, most basic things should be initialized here.
 *      This routine may NOT do anything time consuming like scanning ports,
 *      clearing counters, etc.  Such things should be done as commands in
 *      the rc script.  Otherwise, Verilog and Quickturn simulations take
 *      too long.
 */
int
soc_do_init(int unit, int reset)
{
    soc_control_t       *soc;
    soc_persist_t       *sop;
    soc_mem_t           mem;
    uint32              reg;
    uint16              max_class;
    uint16              mem_clear_chunk_size;
    int                 rv, blk, cache;
    int                 copyno = COPYNO_ALL;
#ifdef BCM_WARM_BOOT_SUPPORT
    int                 offset = 0;
    int                 l3_defip_pair_128_index_count = 0;
    int                 num_ipv6_128b_entries = 0;
    uint32              rval = 0;
#endif
#if !defined(SOC_NO_NAMES)
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || \
    defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
    uint8               chk_skip_cache = 0;
#endif
#endif

    if (!SOC_UNIT_VALID(unit)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_init: unit %d not valid\n"), unit));
        return SOC_E_UNIT;
    }

    soc = SOC_CONTROL(unit);
    sop = SOC_PERSIST(unit);

    if (!(soc->soc_flags & SOC_F_ATTACHED)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_init: unit %d not attached\n"), unit));
        return(SOC_E_UNIT);
    }

#if defined(BCM_RCPU_SUPPORT) && defined(BCM_XGS3_SWITCH_SUPPORT)    
    if (SOC_IS_RCPU_ONLY(unit)) {
        soc_rcpu_init(unit);
    }
#endif

    /***********************************************************************/
    /* If the device has already been initialized before, perform some     */
    /* de-initialization to avoid stomping on existing activities.         */
    /***********************************************************************/
    if (SOC_WARM_BOOT(unit)) {
        pbmp_t empty_pbm;
        int rv;

        /* Cleanup old PCIe link failure based issues */
#if defined(BCM_CMICM_SUPPORT) && defined(BCM_SBUSDMA_SUPPORT)
        if (soc_feature(unit, soc_feature_cmicm) && 
            soc_feature(unit, soc_feature_sbusdma)) {
            int cmc = SOC_PCI_CMC(unit);
            reg = soc_pci_read(unit, CMIC_CMCx_IRQ_STAT1_OFFSET(cmc));
            if (soc_reg_field_get(unit, CMIC_CMC0_IRQ_STAT1r, reg, 
                                  PCIEINTF_NEEDS_CLEANUPf) || 
                soc_reg_field_get(unit, CMIC_CMC0_IRQ_STAT1r, reg, 
                                  PCIE_ECRC_ERR_INTRf)) {
                SOC_IF_ERROR_RETURN
                    (READ_CMIC_PCIE_USERIF_STATUS_CLRr(unit, &reg));
                soc_reg_field_set(unit, CMIC_PCIE_USERIF_STATUS_CLRr, &reg,
                                  USERIF_TIMEDOUTf, 1);
                soc_reg_field_set(unit, CMIC_PCIE_USERIF_STATUS_CLRr, &reg,
                                  USERIF_RESETf, 1);
                SOC_IF_ERROR_RETURN
                    (WRITE_CMIC_PCIE_USERIF_STATUS_CLRr(unit, reg));
                reg = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, 0));
                soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &reg,
                                  ABORTf, 1);
                soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, 0), reg);
                reg = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, 1));
                soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &reg,
                                  ABORTf, 1);
                soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, 1), reg);
                reg = soc_pci_read(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, 2));
                soc_reg_field_set(unit, CMIC_CMC0_SBUSDMA_CH0_CONTROLr, &reg,
                                  ABORTf, 1);
                soc_pci_write(unit, CMIC_CMCx_SBUSDMA_CHy_CONTROL(cmc, 2), reg);

                SOC_IF_ERROR_RETURN(_soc_mem_sbus_fifo_dma_stop(unit, 0));
                SOC_IF_ERROR_RETURN(_soc_mem_sbus_fifo_dma_stop(unit, 1));
                SOC_IF_ERROR_RETURN(_soc_mem_sbus_fifo_dma_stop(unit, 2));
                SOC_IF_ERROR_RETURN(_soc_mem_sbus_fifo_dma_stop(unit, 3));
            }            
            SOC_IF_ERROR_RETURN(READ_CMIC_PCIE_USERIF_PURGE_STATUSr(unit, &reg));
            if (reg) {
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "Unit %d: PCIE purge clear 0x%08x.\n"), unit, reg));
                SOC_IF_ERROR_RETURN(WRITE_CMIC_PCIE_USERIF_PURGE_CONTROLr(unit, 0));
            }
        }
#endif /* BCM_CMICM_SUPPORT && BCM_SBUSDMA_SUPPORT */

        if (!SOC_IS_RCPU_ONLY(unit)) {
            rv = soc_dma_abort(unit);

            if (SOC_FAILURE(rv)) {
                SOC_IF_ERROR_RETURN(soc_event_generate(unit, 
                                SOC_SWITCH_EVENT_UNCONTROLLED_SHUTDOWN, 
                                0, 0, 0));
            }
        }
        /* Assume HW linkscan is running, shut it down to allow 
         * PHY probes.  Linkscan will turn it back on when needed. */

        /* Set linkscan enable flag, it will be turned off by the
         * empty bitmap config call. */
        soc->soc_flags |= SOC_F_LSE;
        SOC_PBMP_CLEAR(empty_pbm);

        SOC_IF_ERROR_RETURN
            (soc_linkscan_config(unit, empty_pbm, empty_pbm));
    } else {
           /* Cold Boot case */
#ifdef BCM_WARM_BOOT_SUPPORT
#ifdef BCM_KATANA2_SUPPORT
           if (SOC_IS_KATANA2(unit)) {
               SOC_IF_ERROR_RETURN(_soc_katana2_flexio_scache_allocate(unit));
           }
#endif
#endif
    }
    if (soc->soc_flags & SOC_F_INITED) {
        if (!SOC_IS_RCPU_ONLY(unit)) {
            SOC_IF_ERROR_RETURN(soc_dma_abort(unit)); /* Turns off/clean up DMA */
        }
        SOC_IF_ERROR_RETURN(soc_counter_stop(unit)); /* Stop counter collection */
#ifdef INCLUDE_MEM_SCAN
        SOC_IF_ERROR_RETURN(soc_mem_scan_stop(unit)); /* Stop memory scanner */
#endif

#ifdef  INCLUDE_I2C
        SOC_IF_ERROR_RETURN(soc_i2c_detach(unit)); /* Free up I2C driver mem */
#endif

#ifdef BCM_XGS_SWITCH_SUPPORT
        if (soc_feature(unit, soc_feature_arl_hashed)) {
            /* Stop L2X thread */
            SOC_IF_ERROR_RETURN(soc_l2x_stop(unit));
        }
#ifdef BCM_TRIUMPH3_SUPPORT
        if SOC_IS_TRIUMPH3(unit) {
            SOC_IF_ERROR_RETURN(soc_tr3_l2_bulk_age_stop(unit));
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
        if SOC_IS_TRIDENT2(unit) {
            SOC_IF_ERROR_RETURN(soc_td2_l2_bulk_age_stop(unit));
        }
#endif /* BCM_TRIDENT2_SUPPORT */
#endif /* BCM_XGS_SWITCH_SUPPORT */

        if (soc->arlShadow != NULL) {
            sal_mutex_take(soc->arlShadowMutex, sal_mutex_FOREVER);
            shr_avl_delete_all(soc->arlShadow);
            sal_mutex_give(soc->arlShadowMutex);
        }

        /* Destroy egress metering mutex. */
        if (soc->egressMeteringMutex != NULL) {
            sal_mutex_destroy(soc->egressMeteringMutex);
            soc->egressMeteringMutex = NULL;
        }

#ifdef BCM_CMICM_SUPPORT
        if (soc_feature(unit, soc_feature_cmicm)) {
            soc_cmic_uc_msg_stop(unit);         /* Stop the UC msging */
        }
#endif /* CMICM Support */

        soc->soc_flags &= ~SOC_F_INITED;
    }
#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        rv = soc_scache_recover(unit);
        if (SOC_FAILURE(rv)) {
            /* Fall back to Level 1 Warm Boot recovery */
            SOC_IF_ERROR_RETURN(soc_stable_size_set(unit, 0));
            SOC_IF_ERROR_RETURN
                (soc_stable_set(unit, _SHR_SWITCH_STABLE_NONE, 0));
            /* Error report */
            SOC_IF_ERROR_RETURN
                (soc_event_generate(unit, SOC_SWITCH_EVENT_STABLE_ERROR, 
                                    SOC_STABLE_CORRUPT,
                                    SOC_STABLE_FLAGS(unit), 0));
            LOG_WARN(BSL_LS_SOC_COMMON,
                     (BSL_META_U(unit,
                                 "Unit %d: Corrupt stable cache.\n"),
                      unit));
        } else {
            SOC_IF_ERROR_RETURN(soc_mem_cache_scache_get(unit));
#ifdef BCM_KATANA2_SUPPORT
            if (SOC_IS_KATANA2(unit)) {
                SOC_IF_ERROR_RETURN(_soc_katana2_flexio_scache_retrieve(unit));
            }
#endif
        }
    }
#endif


    /* Set bitmaps according to which type of device it is */
    SOC_IF_ERROR_RETURN(soc_info_config(unit, soc));

    if (SOC_WARM_BOOT(unit)) {
        SOC_IF_ERROR_RETURN(soc_port_type_verify(unit));
    }

    /* Set feature cache */

    soc_feature_init(unit);
    soc_dcb_unit_init(unit);

#ifdef BCM_CB_ABORT_ON_ERR
    SOC_CB_ABORT_ON_ERR(unit) = soc_property_get(unit, spn_CB_ABORT_ON_ERR, 0);
#endif

    /***********************************************************************/
    /* Always be sure device has correct endian configuration before       */
    /* touching registers - device may not have been configured yet.       */
    /***********************************************************************/

    if (!SOC_WARM_BOOT(unit) && !SOC_IS_RCPU_ONLY(unit)) {
        soc_endian_config(unit);
        soc_pci_ep_config(unit, -1);
    }

    if (!SOC_IS_RCPU_ONLY(unit) && !(soc->soc_flags & SOC_F_INITED) &&
        (soc_cm_get_bus_type(unit) & SOC_PCI_DEV_TYPE) &&
        !(SAL_BOOT_PLISIM || SAL_BOOT_BCMSIM)) {
        uint32 cfg_dw = 0;
        uint8  cap_id = 0, base_ptr = 0;
        soc_info_t *si = &SOC_INFO(unit);
        uint16 pcie_phy_addr = si->pcie_phy_addr;

        /* Device has not been initialized... */
        
        /*********************************************************************/
        /* PCIe SerDes Gen1/Gen2 CDR Track Bandwidth Adjustment              */
        /* for Better Jitter Tolerance.                                      */
        /*********************************************************************/
        if (pcie_phy_addr != 0) {
#ifdef BCM_IPROC_SUPPORT
            if (soc_feature(unit, soc_feature_iproc)) {
                SOC_IF_ERROR_RETURN(iproc_pcie_cdr_bw_adj(unit, pcie_phy_addr));
            } else 
#endif
            {
                SOC_IF_ERROR_RETURN(cmic_pcie_cdr_bw_adj(unit, pcie_phy_addr));
            }
        }

        /*********************************************************************/
        /* Check to see if the PCIe SerDes deemphasis needs to be changed    */
        /* based on the advertisement from the root complex                  */
        /*********************************************************************/
        /*Find PCIe capability base */
        base_ptr = soc_pci_conf_read(unit, 0x34) & 0xff;
        while ((base_ptr >= 0x40) && (base_ptr < 0xff)) {
            cfg_dw = soc_pci_conf_read(unit, base_ptr);
            cap_id = cfg_dw & 0xff;
            if (cap_id == 0x10) break;
            base_ptr = (cfg_dw >> 8) & 0xff;
        }
        /* Check for PCIe Gen2 -3.5dB advertisement */
        if ((cap_id == 0x10) &&  
            ((soc_pci_conf_read(unit, base_ptr + 0x10) & 0xf0000) == 0x20000) &&
            (soc_pci_conf_read(unit, base_ptr + 0x30) & 0x00010000)) {
            /* Device is operating at Gen2 speeds and RC requested -3.5dB */

            if (pcie_phy_addr != 0) {
#ifdef BCM_IPROC_SUPPORT
                if (soc_feature(unit, soc_feature_iproc)) {
                    SOC_IF_ERROR_RETURN(iproc_pcie_deemphasis_set(unit, pcie_phy_addr));
                } else 
#endif
#ifdef BCM_CMICM_SUPPORT
                if (soc_feature(unit, soc_feature_cmicm)) {
                    SOC_IF_ERROR_RETURN(cmicm_pcie_deemphasis_set(unit, pcie_phy_addr));
                } else
#endif
                {
                    SOC_IF_ERROR_RETURN(cmice_pcie_deemphasis_set(unit, pcie_phy_addr));
                }
            }
        }
    }

    /***********************************************************************/
    /* Always enable bursting before doing any more reads or writes        */
    /***********************************************************************/

    if (!SOC_WARM_BOOT(unit) && !SOC_IS_RCPU_ONLY(unit)) {
        soc_pci_burst_enable(unit);
    }


    /***********************************************************************/
    /* Begin initialization from a known state (reset).                    */
    /***********************************************************************/

    /* Attach DMA */

    if (!SOC_IS_RCPU_ONLY(unit)) {
        if ((rv = soc_dma_attach(unit, reset)) < 0) {
            return SOC_E_INTERNAL;
        }
    }

    /*
     * PHY drivers and ID map
     */
    SOC_IF_ERROR_RETURN(soc_phyctrl_software_init(unit));
    /* NB:  The PHY init must be before fusioncore reset, which is next. */

    if (reset && !SOC_WARM_BOOT(unit) && !SOC_IS_RCPU_ONLY(unit)) {
        SOC_IF_ERROR_RETURN(soc_reset(unit));
#if defined(BCM_CMICM_SUPPORT) && defined(BCM_SBUSDMA_SUPPORT)
        if (soc_feature(unit, soc_feature_cmicm) && 
            soc_feature(unit, soc_feature_sbusdma)) {
            /* Enable PCIe purge on error */
            SOC_IF_ERROR_RETURN(READ_CMIC_PCIE_USERIF_PURGE_CONTROLr(unit, &reg));
            soc_reg_field_set(unit, CMIC_PCIE_USERIF_PURGE_CONTROLr, &reg,
                              ENABLE_PURGE_IF_USERIF_TIMESOUTf, 1);
            soc_reg_field_set(unit, CMIC_PCIE_USERIF_PURGE_CONTROLr, &reg,
                              ENABLE_PURGE_IF_USERIF_RESETf, 1);
            soc_reg_field_set(unit, CMIC_PCIE_USERIF_PURGE_CONTROLr, &reg,
                              ENABLE_PIO_PURGE_IF_USERIF_RESETf, 1);
            SOC_IF_ERROR_RETURN(WRITE_CMIC_PCIE_USERIF_PURGE_CONTROLr(unit, reg));
        }
#endif /* BCM_CMICM_SUPPORT && BCM_SBUSDMA_SUPPORT */
    }

    if (SOC_WARM_BOOT(unit) && soc_feature(unit, soc_feature_esm_support)) {
#ifdef BCM_TRIUMPH_SUPPORT
        SOC_IF_ERROR_RETURN(soc_triumph_esm_init_read_config(unit));
#endif
    }

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit) && SOC_IS_HELIX4(unit)) {
        SOC_IF_ERROR_RETURN(READ_TOP_SWITCH_FEATURE_ENABLEr(unit, &rval));
        if (!(rval & 0x8)) {
            /* Disabling MPLS feature during warm boot for HX4, as it is 
               disabled in cold boot by default */
            SOC_FEATURE_CLEAR(unit, soc_feature_mpls);
        }
    }
#endif

    /*
     * Update saved chip state to reflect values after reset.
     */
#ifdef BCM_XGS_SUPPORT
    if (SOC_IS_XGS(unit)) {
        SOC_MEM_CLEAR_HW_ACC_SET(unit, 1);
    }
#endif /* BCM_XGS_SUPPORT */
    soc->soc_flags &= (SOC_F_RESET | SOC_F_RCPU_ONLY);

    sop->debugMode = 0;
    soc->pciParityDPC = 0;
    soc->pciFatalDPC = 0;
    soc->stat.ser_err_int = 0;
    soc->stat.ser_err_fifo = 0;
    soc->stat.ser_err_tcam = 0;
    soc->stat.ser_err_nak = 0;
    soc->stat.ser_err_stat = 0;
    soc->stat.ser_err_ecc = 0;
    soc->stat.ser_err_corr = 0;
    soc->stat.ser_err_clear = 0;
    soc->stat.ser_err_restor = 0;
    soc->stat.ser_err_spe = 0;
    soc->stat.ser_err_reg = 0;
    soc->stat.ser_err_mem = 0;
    soc->stat.ser_err_sw = 0;

#ifdef BCM_WARM_BOOT_SUPPORT
    /* Recover stored Level 2 Warm Boot cache */
    /* The stable and stable size must be selected first */
    if (SOC_WARM_BOOT(unit)) {
        /* Set urpf mode early for mem cache reload to work properly */
        if (soc_feature(unit, soc_feature_urpf) && 
            (soc_feature(unit, soc_feature_l3_defip_map) || 
             soc_feature(unit, soc_feature_l3_defip_hole))) {
            SOC_IF_ERROR_RETURN(READ_L3_DEFIP_RPF_CONTROLr(unit, &reg));
            if (soc_reg_field_get(unit, L3_DEFIP_RPF_CONTROLr,
                                  reg, DEFIP_RPF_ENABLEf)) {
                SOC_CONTROL(unit)->l3_defip_urpf = TRUE;
            }
        }
    }
    if (soc_feature(unit, soc_feature_l3_shared_defip_table) &&
        (SOC_IS_TRIDENT2(unit) || SOC_IS_TRIUMPH3(unit))){
        SOC_IF_ERROR_RETURN(soc_control_defip_scache_init(unit));
    }

    if (SOC_WARM_BOOT(unit)) {
        if (soc_feature(unit, soc_feature_l3_shared_defip_table) &&
            (SOC_IS_TRIDENT2(unit) || SOC_IS_TRIUMPH3(unit))) {
            rv = soc_control_defip_scache_load(unit,
                                &num_ipv6_128b_entries);
            if (SOC_SUCCESS(rv) && SOC_MEM_IS_VALID(unit, L3_DEFIP_PAIR_128m)) {
                l3_defip_pair_128_index_count = soc_mem_index_count(unit,
                                                      L3_DEFIP_PAIR_128m);
                if (num_ipv6_128b_entries != l3_defip_pair_128_index_count) {
                    /* Taking lock is not required as we are in soc init */
                    soc_defip_tables_resize(unit, num_ipv6_128b_entries);
                }
            }
        }
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    /*
     * Initialize memory table status.
     */
    cache = soc_feature(unit, soc_feature_mem_cache) && 
            soc_property_get(unit, spn_MEM_CACHE_ENABLE, 
                             (SAL_BOOT_RTLSIM || SAL_BOOT_XGSSIM) ? 0 : 1);
#if !defined(SOC_NO_NAMES)
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || \
    defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)

    chk_skip_cache = soc_property_get(unit, spn_MEM_CHECK_NOCACHE_OVERRIDE, 0);
#endif
#endif
    soc->stat.mem_cache_count = 0;
    soc->stat.mem_cache_size = 0;
    soc->stat.mem_cache_vmap_size = 0;
#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_HELIX4_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) || \
    defined(BCM_GREYHOUND_SUPPORT)
    soc->stat.tcam_corrupt_map_size = 0;
#endif
    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
        uint8 skip_mem = 0;
        soc_mem_t act_mem = mem;
#ifdef BCM_ESW_SUPPORT        
        soc_mem_t lock_mem = mem;
#endif
        
        if (!SOC_MEM_IS_VALID(unit, mem)) {
            continue;
        }
        if (soc_mem_index_count(unit, mem) == 0) {
            continue;
        }
        sal_memset(sop->memState[mem].count, 0,
                   sizeof (sop->memState[mem].count));
#ifdef BCM_ESW_SUPPORT
        _SOC_DRV_MEM_REUSE_MEM_STATE(unit, act_mem, lock_mem);
        /* Avoid complier warning */
        assert(lock_mem >= 0);
#endif
        if (act_mem != mem) {
            continue;
        }
        SOC_MEM_BLOCK_ITER(unit, mem, blk) {
            copyno = blk;
            break;
        }
        if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM)) {
            SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_CACHABLE;
            skip_mem = 1;
        }
        /* Skip IESMIF mems when ESM is not present */
        else if (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_IPIPE) {
#ifdef BCM_TRIUMPH3_SUPPORT
            if (SOC_IS_TRIUMPH3(unit) && /* Needs update per chip */
                SOC_MEM_ADDR_STAGE_EXTENDED(SOC_MEM_BASE(unit, mem)) == 0x9) {
                if (!soc_feature(unit, soc_feature_esm_support) ||
                     SAL_BOOT_QUICKTURN) {
                    SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_CACHABLE;
                    skip_mem = 1;
                }
            }
#endif /* BCM_TRIUMPH3_SUPPORT */
        }
#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            /* Force cache L2 mem */
            if (mem == L2_ENTRY_1m || mem == L2_ENTRY_2m) {
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE;
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_SER_SPECIAL; 
            }
            if (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE) {
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_CACHABLE;
            }
            /* Skip caching unused views */
            if (mem == ESM_PKT_TYPE_ID_ONLYm || mem == ESM_PKT_TYPE_ID_DATA_ONLYm) {
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_CACHABLE;
                skip_mem = 1;
            }
            if (mem == MODPORT_MAP_M0m || mem == MODPORT_MAP_M1m ||
                mem == MODPORT_MAP_M2m || mem == MODPORT_MAP_M3m) {
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_CACHABLE;
                skip_mem = 1;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT_SUPPORT
        if (SOC_IS_TD_TT(unit) || SOC_IS_TD2_TT2(unit)) {
            int i;
            static soc_mem_t mskip[] = {
                L3_ENTRY_IPV4_MULTICAST_Xm,
                L3_ENTRY_IPV4_MULTICAST_Ym,
                L3_ENTRY_IPV6_UNICAST_Xm,
                L3_ENTRY_IPV6_UNICAST_Ym
            };
            for (i=0; i<COUNTOF(mskip); i++) {
                if (mem == mskip[i]) {
                    SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_CACHABLE;
                    skip_mem = 1;
                    break;
                }
            }
            /* Force cache L2 mem */
            if (mem == L2Xm) {
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE;
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_SER_ENTRY_CLEAR;
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_SER_SPECIAL; 
            }
            if (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE) {
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_CACHABLE;
            }
            /* Skip caching unused views */
            if (mem == ING_SNAT_ONLYm || mem == ING_SNAT_DATA_ONLYm ||
                mem == FP_GLOBAL_MASK_TCAMm ||
                mem == FP_GLOBAL_MASK_TCAM_Xm ||
                mem == FP_GLOBAL_MASK_TCAM_Ym ||
                mem == L2_ENTRY_LPm || mem == L3_ENTRY_LPm ||
                mem == VLAN_XLATE_LPm || mem == EGR_VLAN_XLATE_LPm ||
                mem == VLAN_SUBNET_DATA_ONLYm) {
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_CACHABLE;
                skip_mem = 1;
            }
            if (mem == MODPORT_MAP_M0m || mem == MODPORT_MAP_M1m ||
                mem == MODPORT_MAP_M2m || mem == MODPORT_MAP_M3m) {
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_CACHABLE;
                skip_mem = 1;
            }
        }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
        if (SOC_IS_TD2_TT2(unit)) {
            if (mem == L3_DEFIP_ALPM_IPV4m || mem == L3_DEFIP_ALPM_IPV4_1m ||
                mem == L3_DEFIP_ALPM_IPV6_64m || mem == L3_DEFIP_ALPM_IPV6_64_1m ||
                mem == L3_DEFIP_ALPM_IPV6_128m || mem == L3_DEFIP_PAIR_128m ||
                mem == L3_DEFIPm) {
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE;
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_SER_CACHE_RESTORE;
                skip_mem = 1;
            }
            if (mem == L3_DEFIP_AUX_TABLEm) {
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_CACHABLE;
                skip_mem = 1;
            }
            if (mem == MMU_CTR_MC_DROP_MEM0m || mem == MMU_CTR_MC_DROP_MEM1m ||
            	  mem == MMU_CTR_COLOR_DROP_MEMm || mem == MMU_CTR_ING_DROP_MEMm ||
            	  mem == MMU_CTR_MTRI_DROP_MEMm || mem == MMU_CTR_UC_DROP_MEMm) {
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_SER_ENTRY_CLEAR;
                skip_mem = 1;
            }
        }
        if (SOC_IS_TRIDENT2(unit)) {
            if (mem == MMU_INTFI_XPIPE_FC_MAP_TBL0m || 
                mem == MMU_INTFI_XPIPE_FC_MAP_TBL1m ||
                mem == MMU_INTFI_YPIPE_FC_MAP_TBL0m ||
                mem == MMU_INTFI_YPIPE_FC_MAP_TBL1m) {
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_CACHABLE;
                skip_mem = 1;
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_FIREBOLT2_SUPPORT)
        if (SOC_IS_FIREBOLT2(unit)) {
            switch(mem) {
            case L2Xm:
            case L2_ENTRY_ONLYm:
            case L3_ENTRY_ONLYm:
            case L3_DEFIP_DATA_ONLYm:
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_CACHABLE; 
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_SER_CACHE_RESTORE;
                break;
            case MMU_XQ0m:
            case MMU_XQ1m:
            case MMU_XQ2m:
            case MMU_XQ3m:
            case MMU_XQ4m:
            case MMU_XQ5m:
            case MMU_XQ6m:
            case MMU_XQ7m:
            case MMU_XQ8m:
            case MMU_XQ9m:
            case MMU_XQ10m:
            case MMU_XQ11m:
            case MMU_XQ12m:
            case MMU_XQ13m:
            case MMU_XQ14m:
            case MMU_XQ15m:
            case MMU_XQ16m:
            case MMU_XQ17m:
            case MMU_XQ18m:
            case MMU_XQ19m:
            case MMU_XQ20m:
            case MMU_XQ21m:
            case MMU_XQ22m:
            case MMU_XQ23m:
            case MMU_XQ24m:
            case MMU_XQ25m:
            case MMU_XQ26m:
            case MMU_XQ27m:
            case MMU_XQ28m:
            case MMU_CCPm:
            case MMU_CFAPm:
            case MMU_CBPPKTHEADER0m:
            case MMU_CBPPKTHEADER1m:
            case MMU_CBPCELLHEADERm:
            case MMU_CBPDATA0m:
            case MMU_CBPDATA1m:
            case MMU_CBPDATA2m:
            case MMU_CBPDATA3m:
            case MMU_CBPDATA4m:
            case MMU_CBPDATA5m:
            case MMU_CBPDATA6m:
            case MMU_CBPDATA7m:
            case MMU_CBPDATA8m:
            case MMU_CBPDATA9m:
            case MMU_CBPDATA10m:
            case MMU_CBPDATA11m:
            case MMU_CBPDATA12m:
            case MMU_CBPDATA13m:
            case MMU_CBPDATA14m:
            case MMU_CBPDATA15m:
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_SER_ENTRY_CLEAR; 
                break;
            default:
                SOC_MEM_INFO(unit, mem).flags &= ~(SOC_MEM_FLAG_CACHABLE | SOC_MEM_SER_FLAGS);
                skip_mem = 1;
            }
        }
#endif /* BCM_FIREBOLT2_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
        if (SOC_IS_TRX(unit)) {
            /* Force cache L2 mem */
            if (mem == L2Xm) {
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE;
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_SER_ENTRY_CLEAR;
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_SER_SPECIAL; 
            }
            if (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE) {
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_CACHABLE;
            }
            /* Skip caching unused views */
            if (mem == MY_STATION_TCAM_DATA_ONLYm || mem == L2_USER_ENTRY_DATA_ONLYm ||
                mem == L3_DEFIP_DATA_ONLYm || mem == L3_DEFIP_PAIR_128_DATA_ONLYm ||
                mem == L3_DEFIP_128_DATA_ONLYm || mem == L3_DEFIP_128_ONLYm ||
                mem == L3_DEFIP_ONLYm || mem == L3_DEFIP_PAIR_128_ONLYm ||
                mem == L3_ENTRY_ONLYm || 
                mem == MODPORT_MAP_SWm) {
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_CACHABLE;
                skip_mem = 1;
            }
        }
#endif /* BCM_TRX_SUPPORT */
#if defined(BCM_ENDURO_SUPPORT)
        if(SOC_IS_ENDURO(unit)) {
            if (mem == FP_METER_TABLEm) {
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE;
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_SER_ENTRY_CLEAR;
            }
            if (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE) {
                SOC_MEM_INFO(unit, mem).flags |= SOC_MEM_FLAG_CACHABLE;
            }
        }
#endif
#if !defined(SOC_NO_NAMES)
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || \
    defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
        if (chk_skip_cache) {
            char mem_name[128];
            char *mptr;

            sal_strcpy(mem_name, "mem_nocache_");
            mptr = &mem_name[sal_strlen(mem_name)];
            sal_strcpy(mptr, SOC_MEM_NAME(unit, mem));
            if (soc_property_get(unit, mem_name, 0)) {
                SOC_MEM_INFO(unit, mem).flags &= ~SOC_MEM_FLAG_CACHABLE;
                skip_mem = 1;
            }
        }
#endif /* !SOC_NO_NAMES */
#endif
        /* Enable cache only if all criteria's are met */
        if (cache && !skip_mem &&
            ((SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_SER_CACHE_RESTORE) ||
             (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_SER_WRITE_CACHE_RESTORE) ||
             ((SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CACHABLE) && 
              ((SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_SER_ECC_CORRECTABLE) ||
               (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_SER_SPECIAL))))) {
            SOC_IF_ERROR_RETURN(soc_mem_cache_set(unit, mem, MEM_BLOCK_ALL, TRUE));
#ifdef BCM_WARM_BOOT_SUPPORT
            SOC_IF_ERROR_RETURN(soc_mem_cache_scache_load(unit, mem, &offset));
#endif
        } else {
            SOC_IF_ERROR_RETURN(soc_mem_cache_set(unit, mem, MEM_BLOCK_ALL, FALSE));
        }
    }
#if defined(BCM_XGS_SUPPORT)
    if (soc_feature(unit, soc_feature_regs_as_mem) && 
        soc_property_get(unit, spn_PARITY_ENABLE, TRUE)) {
        SOC_IF_ERROR_RETURN(soc_ser_reg_cache_init(unit));
    }
#endif /* BCM_XGS_SUPPORT */

    /***********************************************************************/
    /* Configure CMIC PCI registers correctly for driver operation.        */
    /*                                                                     */
    /* NOTE:  When interrupt driven, the internal SOC registers cannot     */
    /*        be accessed until the CMIC interrupts are enabled.           */
    /***********************************************************************/

    if (!SAL_BOOT_PLISIM && !SOC_WARM_BOOT(unit) && !SOC_IS_RCPU_UNIT(unit)) {
        /*
         * Check that PCI memory space is mapped correctly by running a
         * quick diagnostic on the S-Channel message buffer.
         */

        SOC_IF_ERROR_RETURN(soc_pci_test(unit));
    }

    if (!SOC_IS_RCPU_ONLY(unit)) {
    /*
     * Adjust the CMIC CONFIG register
     */
#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm)) {
        if (SAL_BOOT_PLISIM) {
            /* Set interrupt polarity to active high */
        }
    } else
#endif
    {
        reg = soc_pci_read(unit, CMIC_CONFIG);

        /*
         * Enable enhanced DMA modes:
         *  Scatter/gather, reload, and unaligned transfers
         *
         * Enable read and write bursts.
         *  Note: very fast CPUs (above ~500 MHz) may combine multiple
         *  memory operations into bursts.  The CMIC will hang if burst
         *  operations are not enabled.
         */

        reg |= (CC_SG_OPN_EN | CC_RLD_OPN_EN | CC_ALN_OPN_EN |
                CC_RD_BRST_EN | CC_WR_BRST_EN);

        if (SAL_BOOT_PLISIM) {
            /* Set interrupt polarity to active high */
            reg &= ~CC_ACT_LOW_INT;
        }

#ifdef BCM_XGS_SWITCH_SUPPORT
        if (SOC_IS_XGS_SWITCH(unit) &&
            !soc_property_get(unit, spn_MDIO_EXTERNAL_MASTER, 0)) {
            reg |= CC_EXT_MDIO_MSTR_DIS;
        }
#endif

        soc_pci_write(unit, CMIC_CONFIG, reg);
    }

    /*
     * Configure DMA channels.
     */
    if (soc_dma_init(unit) != 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_init: unit %d DMA initialization failed\n"),
                   unit));
        return SOC_E_INTERNAL;
    }

    } /* !SOC_IS_RCPU_ONLY(unit) */

#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        int                 i, j;
        int                 numq = 0, cmc_numq;

        
        /* Enable PCI Bus Error and Parity Error Interrupts */
        /* soc_intr_enable(unit, IRQ_PCI_PARITY_ERR | IRQ_PCI_FATAL_ERR); */
        /* S-Channel Error Interrupt */
        /* soc_intr_enable(unit, IRQ_SCHAN_ERR); */
        /* Link status updates */
        if (!SOC_IS_RCPU_ONLY(unit)) {
            soc_cmicm_intr1_enable(unit, IRQ_CMCx_LINK_STAT_MOD);
        }

        soc_cmic_uc_msg_init(unit);     /* Init the uC area */

        for (i = 0; i < SOC_CMCS_NUM(unit); i++) {
            /* Clear the CPU & ARM queues */
            SHR_BITCLR_RANGE(CPU_ARM_QUEUE_BITMAP(unit, i),
                             0, NUM_CPU_COSQ(unit));
            if (i == SOC_PCI_CMC(unit)) {
                NUM_CPU_ARM_COSQ(unit, i) =
                    soc_property_uc_get(unit, 0, spn_NUM_QUEUES,
                                        NUM_CPU_COSQ(unit));
            } else {
                /* Properties presume it is PCI for first CMC, then the UC's */
                j = (i < SOC_PCI_CMC(unit)) ? (i + 1) : i;
                NUM_CPU_ARM_COSQ(unit, i) =
                    soc_property_uc_get(unit, j, spn_NUM_QUEUES, 0);
            }

            cmc_numq = NUM_CPU_ARM_COSQ(unit, i);
            SHR_BITSET_RANGE(CPU_ARM_QUEUE_BITMAP(unit, i),
                             numq, cmc_numq);
            numq += cmc_numq;

            if (numq > NUM_CPU_COSQ(unit)) {
                LOG_WARN(BSL_LS_SOC_COMMON,
                         (BSL_META_U(unit,
                                     "soc_do_init: total cpu and arm cosq %04x unexpected\n"),
                          numq));
            }
        }
    } else 
#endif /* CMICM Support */
    {
        if (!SOC_IS_RCPU_ONLY(unit)) {

        /*
         * PCI Bus Error and Parity Error Interrupts
         */
        soc_intr_enable(unit, IRQ_PCI_PARITY_ERR | IRQ_PCI_FATAL_ERR);

        /*
         * S-Channel Error Interrupt
         */
        soc_intr_enable(unit, IRQ_SCHAN_ERR);

        /*
         * S-Channel Operation Complete Interrupt.
         * This interrupt is enabled in soc_schan_op().
         *
         * The SCH_MSG_DONE bit must be cleared after chip reset, since it
         * defaults to 1.  Otherwise, the interrupt will occur as soon as
         * it's enabled.
         */
        soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MSG_DONE_CLR);
        /*
         * MIIM Operation Complete Interrupt
         * This interrupt is enabled in cmic_miim_read/write().
         *
         * The SCH_MIIM_OP_DONE bit must be cleared after chip reset, since
         * it defaults to 1.  Otherwise, the interrupt will occur as soon as
         * it's enabled.
         */
        soc_pci_write(unit, CMIC_SCHAN_CTRL, SC_MIIM_OP_DONE_CLR);

        /*
         * Link status updates
         */

        soc_intr_enable(unit, IRQ_LINK_STAT_MOD);

        } /* !SOC_IS_RCPU_ONLY(unit) */
    }

    soc->soc_flags |= SOC_F_INITED;

    if (!SOC_IS_RCPU_ONLY(unit)) {

    /***********************************************************************/
    /* It is legal to access SOC internal registers beyond this point.     */
    /*                                                                     */
    /* HOWEVER:                                                            */
    /*                                                                     */
    /* This init routine should perform the utter minimum necessary.       */
    /* soc_misc_init() and soc_mmu_init() are used for further init.       */
    /***********************************************************************/

#ifdef BCM_TRIUMPH_SUPPORT
     if (SOC_IS_TRIUMPH(unit) || SOC_IS_TRIUMPH2(unit)) {
        /* BP needs to be disabled for register tests to pass */
        SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, AUX_ARB_CONTROL_2r,
                            REG_PORT_ANY, ESM_BP_ENABLEf, 0));
    }
#endif
#ifdef BCM_TRIUMPH2_SUPPORT
    /* Make BIGMAC registers accesible for Triumph2, Apollo and Enduro */
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) || SOC_IS_HURRICANE(unit)) {
        soc_port_t port;
        uint64              mac_ctrl;
        PBMP_PORT_ITER(unit, port) {
            if (!IS_HG_PORT(unit, port) && !IS_XE_PORT(unit, port)) {
                continue;
            }
            SOC_IF_ERROR_RETURN(READ_MAC_CTRLr(unit, port, &mac_ctrl));
            soc_reg64_field32_set(unit, MAC_CTRLr, &mac_ctrl, TXRESETf, 0);
            soc_reg64_field32_set(unit, MAC_CTRLr, &mac_ctrl, RXRESETf, 0);
            SOC_IF_ERROR_RETURN(WRITE_MAC_CTRLr(unit, port, mac_ctrl));
        }
    }
#endif
#ifdef BCM_SHADOW_SUPPORT
    /* Make sure registers which are actually memories in HW are reset */
    /* Also make the XMAC registers accesible */
    if (SOC_IS_SHADOW(unit)) {
        uint32 rval = 0;
        soc_port_t port;
        PBMP_PORT_ITER(unit, port) {
            if (!IS_HG_PORT(unit, port) && !IS_XE_PORT(unit, port) &&
                !IS_SCH_PORT(unit,port)) {
                continue;
            }
            SOC_IF_ERROR_RETURN(READ_XLPORT_XMAC_CONTROLr(unit, port, &rval));
            soc_reg_field_set(unit, XLPORT_XMAC_CONTROLr, &rval, XMAC_RESETf, 0);
            SOC_IF_ERROR_RETURN(WRITE_XLPORT_XMAC_CONTROLr(unit, port, rval));
        }
        rval = 0;
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_IP_STATS_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_COS_BYTES_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_COS_PKTS_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_VLAN_STATS_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_HIGIG_CMD_STATS_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_DROP_BYTES_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_DROP_PKTS_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_L2_BYTES_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_L2_PKTS_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_SKIP_STOP_STATS_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_DROP_AGG_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT1r, &rval, RCV_IPHCKS_INITf, 1);
        SOC_IF_ERROR_RETURN(WRITE_ISW2_MEM_INIT1r(unit, rval));

        rval = 0;
        soc_reg_field_set(unit, ISW2_MEM_INIT2r, &rval, RDBGC0_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT2r, &rval, RDBGC1_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT2r, &rval, RDBGC2_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT2r, &rval, RDBGC3_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT2r, &rval, RDBGC4_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT2r, &rval, RDBGC5_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT2r, &rval, RDBGC6_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT2r, &rval, RDBGC7_INITf, 1);
        soc_reg_field_set(unit, ISW2_MEM_INIT2r, &rval, RDBGC8_INITf, 1);
        SOC_IF_ERROR_RETURN(WRITE_ISW2_MEM_INIT2r(unit, rval));
    }
#endif

    } /* !SOC_IS_RCPU_ONLY */

    if (soc_property_get(unit, spn_L2XMSG_SHADOW_HIT_BITS, 1)) {
        soc->l2x_shadow_hit_bits |= L2X_SHADOW_HIT_BITS;
    } else {
        soc->l2x_shadow_hit_bits = 0;
    }
    if (soc_property_get(unit, spn_L2XMSG_SHADOW_HIT_SRC, 0)) {
        soc->l2x_shadow_hit_bits |= L2X_SHADOW_HIT_SRC;
    }
    if (soc_property_get(unit, spn_L2XMSG_SHADOW_HIT_DST, 0)) {
        soc->l2x_shadow_hit_bits |= L2X_SHADOW_HIT_DST;
    }

#ifdef BCM_XGS_SWITCH_SUPPORT
    if (soc_feature(unit, soc_feature_dual_hash)) {
        SOC_DUAL_HASH_MOVE_MAX(unit) = soc_property_get(unit,
                                       spn_DUAL_HASH_RECURSE_DEPTH,
                                       SOC_MEM_DUAL_HASH_RECURSE_DEPTH);
        SOC_DUAL_HASH_MOVE_MAX_L2X(unit) = soc_property_get(unit,
                                   spn_DUAL_HASH_RECURSE_DEPTH_L2X,
                                   soc_dual_hash_recurse_depth_get(unit, L2Xm));
        SOC_DUAL_HASH_MOVE_MAX_MPLS(unit) = soc_property_get(unit,
                            spn_DUAL_HASH_RECURSE_DEPTH_MPLS,
                            soc_dual_hash_recurse_depth_get(unit, MPLS_ENTRYm));
        SOC_DUAL_HASH_MOVE_MAX_VLAN(unit) = soc_property_get(unit,
                            spn_DUAL_HASH_RECURSE_DEPTH_VLAN,
                            soc_dual_hash_recurse_depth_get(unit, VLAN_XLATEm));
        SOC_DUAL_HASH_MOVE_MAX_EGRESS_VLAN(unit) = soc_property_get(unit,
                        spn_DUAL_HASH_RECURSE_DEPTH_EGRESS_VLAN,
                        soc_dual_hash_recurse_depth_get(unit, EGR_VLAN_XLATEm));
#if defined(INCLUDE_L3)
        SOC_DUAL_HASH_MOVE_MAX_L3X(unit) = soc_property_get(unit,
                       spn_DUAL_HASH_RECURSE_DEPTH_L3X,
                       soc_dual_hash_recurse_depth_get(unit, L3_ENTRY_ONLYm));
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
        SOC_DUAL_HASH_MOVE_MAX_ING_DNAT_ADDRESS_TYPE(unit) = soc_property_get(unit,
                spn_DUAL_HASH_RECURSE_DEPTH_DNAT_POOL,
                soc_dual_hash_recurse_depth_get(unit, ING_DNAT_ADDRESS_TYPEm));
#endif /* BCM_TRIDENT2_SUPPORT */
    }
    if (soc_feature(unit, soc_feature_ism_memory) || soc_feature(unit, soc_feature_shared_hash_mem)) {
        SOC_MULTI_HASH_MOVE_MAX(unit) = soc_property_get(unit,
                                        spn_MULTI_HASH_RECURSE_DEPTH,
                                        SOC_MEM_MULTI_HASH_RECURSE_DEPTH);
        SOC_MULTI_HASH_MOVE_MAX_L2(unit) = soc_property_get(unit,
                           spn_MULTI_HASH_RECURSE_DEPTH_L2,
                           soc_multi_hash_recurse_depth_get(unit, L2_ENTRY_1m));
        SOC_MULTI_HASH_MOVE_MAX_MPLS(unit) = soc_property_get(unit,
                           spn_MULTI_HASH_RECURSE_DEPTH_MPLS,
                           soc_multi_hash_recurse_depth_get(unit, MPLS_ENTRYm));
        SOC_MULTI_HASH_MOVE_MAX_VLAN(unit) = soc_property_get(unit,
                           spn_MULTI_HASH_RECURSE_DEPTH_VLAN,
                           soc_multi_hash_recurse_depth_get(unit, VLAN_XLATEm));
        SOC_MULTI_HASH_MOVE_MAX_EGRESS_VLAN(unit) = soc_property_get(unit,
                       spn_MULTI_HASH_RECURSE_DEPTH_EGRESS_VLAN,
                       soc_multi_hash_recurse_depth_get(unit, EGR_VLAN_XLATEm));
        SOC_MULTI_HASH_MOVE_MAX_L3(unit) = soc_property_get(unit,
                           spn_MULTI_HASH_RECURSE_DEPTH_L3,
                           soc_multi_hash_recurse_depth_get(unit, L3_ENTRY_1m));
    }
    if (soc_feature(unit, soc_feature_shared_hash_mem)) {
        SOC_MULTI_HASH_MOVE_ALGORITHM(unit) = soc_property_get(unit,
                           "multi_hash_move_algorithm",
                           MULTI_HASH_MOVE_ALGO_BREADTH);
    }
#endif /* BCM_XGS_SWITCH_SUPPORT */

    /* Backwards compatible default */
    SOC_MCAST_ADD_ALL_ROUTER_PORTS(unit) = 1;

#ifdef BCM_XGS_SWITCH_SUPPORT
    if (soc_feature(unit, soc_feature_arl_hashed)) {
        SOC_IF_ERROR_RETURN(soc_l2x_init(unit));
    }
#endif

    /* Set maximum supported vrf id for the device. */
    SOC_IF_ERROR_RETURN(soc_max_supported_vrf_get(unit, &max_class));
    SOC_VRF_MAX_SET(unit, max_class);

    /* Set maximum supported address class for the device. */
    SOC_IF_ERROR_RETURN(soc_max_supported_addr_class_get(unit, &max_class));
    SOC_ADDR_CLASS_MAX_SET(unit, max_class);

   /* If the feature is supported, set the maximum supported over-laid
      address class for the device. */
    if (soc_feature(unit, soc_feature_overlaid_address_class)) {
        SOC_IF_ERROR_RETURN
            (soc_max_supported_overlaid_addr_class_get(unit, &max_class));
        SOC_OVERLAID_ADDR_CLASS_MAX_SET(unit, max_class);
    }

    /* For the TR3 family of devices, set the extended classID */
    if (SOC_IS_TRIUMPH3(unit)) {
        SOC_IF_ERROR_RETURN
            (soc_max_supported_extended_addr_class_get(unit, &max_class));
        SOC_EXT_ADDR_CLASS_MAX_SET(unit, max_class);
    }

    if (soc_feature(unit, soc_feature_src_mac_group)) {
        SOC_L2X_GROUP_ENABLE_SET(unit, TRUE);
    } else {
        SOC_L2X_GROUP_ENABLE_SET(unit, FALSE);
    }

    /* Set maximum supported address class for the device. */
    SOC_IF_ERROR_RETURN(soc_max_supported_intf_class_get(unit, &max_class));
    SOC_INTF_CLASS_MAX_SET(unit, max_class);

    /* Create egress metering mutex. */
    if ((soc->egressMeteringMutex =
         sal_mutex_create("port_rate_egress_lock")) == NULL) {
        return (SOC_E_MEMORY);
    }

    if(soc_property_get(unit, spn_MEM_CLEAR_HW_ACCELERATION, 1)) {
        SOC_MEM_CLEAR_USE_DMA_SET(unit, TRUE);
    }

    mem_clear_chunk_size =
        soc_property_get(unit, spn_MEM_CLEAR_CHUNK_SIZE, 4096);
    SOC_MEM_CLEAR_CHUNK_SIZE_SET(unit, mem_clear_chunk_size);

    if (soc_property_get(unit, spn_GPORT, FALSE)) {
          SOC_USE_GPORT_SET(unit, TRUE);
    }
#ifdef BCM_ASSERT_EVENT_ENABLE
    sal_assert_set(soc_event_assert);
#endif /* BCM_ASSERT_EVENT_ENABLE */

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
#ifdef BCM_ISM_SUPPORT
        /* Retreive hash config data */
        if (soc_feature(unit, soc_feature_ism_memory)) {
            uint8 hcfg;
            uint8 offsets[_SOC_ISM_MAX_BANKS], count;
            
            rv = soc_ism_table_hash_config_get(unit, SOC_ISM_MEM_VLAN_XLATE,
                                               &hcfg);
            rv |= soc_ism_table_hash_config_get(unit, SOC_ISM_MEM_L2_ENTRY,
                                                &hcfg);
            rv |= soc_ism_table_hash_config_get(unit, SOC_ISM_MEM_L3_ENTRY,
                                                &hcfg);
            rv |= soc_ism_table_hash_config_get(unit, SOC_ISM_MEM_EP_VLAN_XLATE,
                                                &hcfg);
            rv |= soc_ism_table_hash_config_get(unit, SOC_ISM_MEM_MPLS, &hcfg);
            if (rv != SOC_E_NONE) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "Error retreiving ISM hash zero_lsb(s) !!\n")));
                return SOC_E_INTERNAL;
            }
            rv = soc_ism_hash_table_offset_config_get(unit, SOC_ISM_MEM_VLAN_XLATE,
                                                      offsets, &count);
            rv |= soc_ism_hash_table_offset_config_get(unit, SOC_ISM_MEM_L2_ENTRY,
                                                       offsets, &count);
            rv |= soc_ism_hash_table_offset_config_get(unit, SOC_ISM_MEM_L3_ENTRY,
                                                       offsets, &count);
            rv |= soc_ism_hash_table_offset_config_get(unit, SOC_ISM_MEM_EP_VLAN_XLATE,
                                                       offsets, &count);
            rv |= soc_ism_hash_table_offset_config_get(unit, SOC_ISM_MEM_MPLS,
                                                       offsets, &count);
            if (rv != SOC_E_NONE) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "Error retreiving ISM hash offset(s) !!\n")));
                return SOC_E_INTERNAL;
            }
        }
#endif /* BCM_ISM_SUPPORT */
    }    
#endif /* BCM_WARM_BOOT_SUPPORT */
    
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_static_repl_head_alloc)) {
        soc->repl_eligible_pbmp = soc_property_get_pbmp_default(unit,
                spn_REPLICATION_ELIGIBLE_PBMP, PBMP_ALL(unit));
    }
#endif

    return(SOC_E_NONE);
}



#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      soc_shutdown
 * Purpose:
 *      Free up SOC resources without touching hardware
 * Parameters:
 *      unit - StrataSwitch unit #
 * Returns:
 *      SOC_E_XXX
 */
int
soc_shutdown(int unit)
{
    soc_control_t       *soc;
    soc_mem_t           mem, act_mem, lock_mem;
    int                 ix;
    
    if (!SOC_UNIT_VALID(unit)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_shutdown: unit %d not valid\n"), unit));
        return SOC_E_UNIT;
    }

    soc = SOC_CONTROL(unit);

    if (!(soc->soc_flags & SOC_F_ATTACHED)) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_shutdown: unit %d not attached\n"), unit));
        return(SOC_E_UNIT);
    }

    if (soc->soc_flags & SOC_F_INITED) {
#ifdef BCM_HERCULES_SUPPORT
        /* Dump the MMU error stats */
        soc_mmu_error_done(unit);

        if (soc->lla_cells_good != NULL) {
            sal_free(soc->lla_cells_good);
            soc->lla_cells_good = NULL;
        }

        if (soc->lla_map != NULL) {
            int port;
            PBMP_PORT_ITER(unit, port) {
               if (soc->lla_map[port] != NULL) {
                   sal_free(soc->lla_map[port]);
               }
            }
            sal_free(soc->lla_map);
            soc->lla_map = NULL;
        }

        if (soc->sbe_disable != NULL) {
            sal_free(soc->sbe_disable);
            soc->sbe_disable = NULL;
        }
#endif

#ifdef  INCLUDE_I2C
        SOC_IF_ERROR_RETURN(soc_i2c_detach(unit)); /* Free up I2C driver mem */
#endif

        if (!SOC_IS_RCPU_ONLY(unit)) {
            /* Free up DMA memory */
            SOC_IF_ERROR_RETURN(soc_dma_detach(unit));
        }

        /* Clear all outstanding DPCs owned by this unit */
        sal_dpc_cancel(INT_TO_PTR(unit));

        if (soc->arlShadow != NULL) {
            sal_mutex_take(soc->arlShadowMutex, sal_mutex_FOREVER);
            shr_avl_delete_all(soc->arlShadow);
            sal_mutex_give(soc->arlShadowMutex);
        }

        /* Terminate counter module; frees allocated space */
        soc_counter_detach(unit);

#ifdef BCM_XGS_SWITCH_SUPPORT
        if (soc_feature(unit, soc_feature_arl_hashed)) {
            /* Stop L2X thread */
            SOC_IF_ERROR_RETURN(soc_l2x_stop(unit));
            SOC_IF_ERROR_RETURN(soc_l2x_detach(unit));
        }
#ifdef BCM_TRIUMPH3_SUPPORT
        if SOC_IS_TRIUMPH3(unit) {
            SOC_IF_ERROR_RETURN(soc_tr3_l2_bulk_age_stop(unit));
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
        if SOC_IS_TRIDENT2(unit) {
            SOC_IF_ERROR_RETURN(soc_td2_l2_bulk_age_stop(unit));
        }
#endif /* BCM_TRIDENT2_SUPPORT */
        if (soc_property_get(unit, spn_PARITY_ENABLE, TRUE) && 
            SOC_CONTROL(unit)->mem_scache_ptr == NULL) {
            SOC_IF_ERROR_RETURN(soc_system_scrub(unit));
        }
#ifdef BCM_SBUSDMA_SUPPORT
        if (SOC_SBUSDMA_DM_INFO(unit)) {
            SOC_IF_ERROR_RETURN(soc_sbusdma_desc_detach(unit));
        }
#endif /* BCM_SBUSDMA_SUPPORT */
#endif /* BCM_XGS_SWITCH_SUPPORT */

        soc->soc_flags &= ~SOC_F_INITED;
#ifdef BCM_CMICM_SUPPORT
        if (soc_feature(unit, soc_feature_cmicm)) {
            soc_cmicm_intr0_disable(unit, ~0);
            soc_cmicm_intr1_disable(unit, ~0);
            soc_cmicm_intr2_disable(unit, ~0);
        } else
#endif
        {
            soc_intr_disable(unit, ~0);
        }

        /* Detach interrupt handler, if we installed one */
        /* unit # is ISR arg */
        if (soc_cm_interrupt_disconnect(unit) < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "soc_shutdown: could not disconnect interrupt line\n")));
            return SOC_E_INTERNAL;
        }

        /*
         * When detaching, take care to free up only resources that were
         * actually allocated, in case we are cleaning up after an attach
         * that failed part way through.
         */
        if (soc->arlBuf != NULL) {
            soc_cm_sfree(unit, (void *)soc->arlBuf);
            soc->arlBuf = NULL;
        }

        if (soc->arlShadow != NULL) {
            shr_avl_destroy(soc->arlShadow);
            soc->arlShadow = NULL;
        }

        if (soc->arlShadowMutex != NULL) {
            sal_mutex_destroy(soc->arlShadowMutex);
            soc->arlShadowMutex = NULL;
        }

#ifdef INCLUDE_MEM_SCAN
        SOC_IF_ERROR_RETURN(soc_mem_scan_stop(unit)); /* Stop memory scanner */
        if (soc->mem_scan_notify) {
            sal_sem_destroy(soc->mem_scan_notify);
        }
#endif
        for (mem = 0; mem < NUM_SOC_MEM; mem++) {
            if (!SOC_MEM_IS_VALID(unit, mem)) {
                continue;
            }

            act_mem = mem;
#ifdef BCM_ESW_SUPPORT
            _SOC_DRV_MEM_REUSE_MEM_STATE(unit, act_mem, lock_mem);
#endif
            /* Deallocate table cache memory, if caching enabled */
            SOC_IF_ERROR_RETURN(soc_mem_cache_set(unit, act_mem, COPYNO_ALL, FALSE));
        }
        
        /* LOCK used in cache_set, so destroy later */
        for (mem = 0; mem < NUM_SOC_MEM; mem++) {
            if (!SOC_MEM_IS_VALID(unit, mem)) {
                continue;
            }

            act_mem = mem;
            lock_mem = mem;
#ifdef BCM_ESW_SUPPORT
            _SOC_DRV_MEM_REUSE_MEM_STATE(unit, act_mem, lock_mem);
#endif
            if (soc->memState[lock_mem].lock != NULL) {
                sal_mutex_destroy(soc->memState[lock_mem].lock);
                soc->memState[lock_mem].lock = NULL;
            }
        }        

        if (soc->schanMutex) {
            sal_mutex_destroy(soc->schanMutex);
            soc->schanMutex = NULL;
        }

#ifdef BCM_CMICM_SUPPORT
        if (soc->fschanMutex) {
            sal_mutex_destroy(soc->fschanMutex);
            soc->fschanMutex = NULL;
        }

        if (soc->ccmDmaIntr) {
            sal_sem_destroy(soc->ccmDmaIntr);
            soc->ccmDmaIntr = NULL;
        }

        if (soc_feature(unit, soc_feature_cmicm)) {
            soc_cmic_uc_msg_stop(unit);
        }
#endif

        if (soc->schanIntr) {
            sal_sem_destroy(soc->schanIntr);
            soc->schanIntr = NULL;
        }

        if (soc->miimMutex) {
            sal_mutex_destroy(soc->miimMutex);
            soc->miimMutex = NULL;
        }

        if (soc->tableDmaMutex) {
            sal_mutex_destroy(soc->tableDmaMutex);
            soc->tableDmaMutex = NULL;
        }

        if (soc->tslamDmaMutex) {
            sal_mutex_destroy(soc->tslamDmaMutex);
            soc->tslamDmaMutex = NULL;
        }

        if (soc->tableDmaIntr) {
            sal_sem_destroy(soc->tableDmaIntr);
            soc->tableDmaIntr = NULL;
        }

        if (soc->tslamDmaIntr) {
            sal_sem_destroy(soc->tslamDmaIntr);
            soc->tslamDmaIntr = NULL;
        }

        if (soc->tslamDmaIntr) {
            sal_sem_destroy(soc->tslamDmaIntr);
            soc->tslamDmaIntr = NULL;
        }

        for (ix = 0; ix < 3; ix++) {
             if (soc->memCmdIntr[ix]) {
                 sal_sem_destroy(soc->memCmdIntr[ix]);
                 soc->memCmdIntr[ix] = NULL;
             }
        }

        if (soc->arl_notify) {
            sal_sem_destroy(soc->arl_notify);
            soc->arl_notify = NULL;
        }

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (soc_feature(unit, soc_feature_esm_correction)) {
            SOC_IF_ERROR_RETURN(soc_tr3_esm_recovery_stop(unit));
            if (soc->esm_recovery_notify) {
                sal_sem_destroy(soc->esm_recovery_notify);
                soc->esm_recovery_notify = NULL;
            }
            if (soc->esm_lock) {
                sal_mutex_destroy(soc->esm_lock);
                soc->esm_lock = NULL;
            }
        }
#endif

#ifdef BCM_XGS_SWITCH_SUPPORT
        if (NULL != soc->l2x_notify) {
            sal_sem_destroy(soc->l2x_notify);
            soc->l2x_notify = NULL;
        }
        if (NULL != soc->l2x_del_sync) {
            sal_mutex_destroy(soc->l2x_del_sync);
            soc->l2x_del_sync = NULL;
        }
        if (NULL != soc->l2x_lock) {
            sal_sem_destroy(soc->l2x_lock);
            soc->l2x_lock = NULL;
        }
        soc->l2x_pid = SAL_THREAD_ERROR;
        soc->l2x_interval = 0;
#ifdef BCM_TRIUMPH3_SUPPORT
        if SOC_IS_TRIUMPH3(unit) {
            if (NULL != soc->l2x_age_notify) {
                sal_sem_destroy(soc->l2x_age_notify);
                soc->l2x_age_notify = NULL;
            }
            soc->l2x_age_pid = SAL_THREAD_ERROR;
            soc->l2x_age_interval = 0;
            soc->l2x_agetime_adjust_usec = 0;
            soc->l2x_agetime_curr_timeblk = 0;
            soc->l2x_agetime_curr_timeblk_usec = 0;
            soc->l2x_prev_age_timeout = 0;
        } 
#endif
#ifdef BCM_TRIDENT2_SUPPORT
        if SOC_IS_TRIDENT2(unit) {
            if (NULL != soc->l2x_age_notify) {
                sal_sem_destroy(soc->l2x_age_notify);
                soc->l2x_age_notify = NULL;
            }
            if (NULL != soc->l2x_age_sync) {
                sal_sem_destroy(soc->l2x_age_sync);
                soc->l2x_age_sync = NULL;
            }
            soc->l2x_age_pid = SAL_THREAD_ERROR;
            soc->l2x_age_interval = 0;
            soc->l2x_age_enable = 0;
            soc->l2x_sw_aging = 0;
        }
#endif
#endif /* BCM_XGS_SWITCH_SUPPORT */
        /* Destroy L3 module protection mutex. */
        if (NULL != soc->l3x_lock) {
            sal_mutex_destroy(soc->l3x_lock);
            soc->l3x_lock = NULL;
        }

        if (soc->ipfixIntr) {
            sal_sem_destroy(soc->ipfixIntr);
            soc->ipfixIntr = NULL;
        }

#if defined(INCLUDE_REGEX)
        if (soc->ftreportIntr) {
            sal_sem_destroy(soc->ftreportIntr);
            soc->ftreportIntr = NULL;
        }
#endif

        /* Destroy egress metering mutex. */
        if (soc->egressMeteringMutex != NULL) {
            sal_mutex_destroy(soc->egressMeteringMutex);
            soc->egressMeteringMutex = NULL;
        }

        sal_free(SOC_PERSIST(unit));
        SOC_PERSIST(unit) = NULL;

        sal_free(SOC_CONTROL(unit));
        SOC_CONTROL(unit) = NULL;

        if (--soc_ndev_attached == 0) {
            /* Work done after the last SOC device is detached. */
            /* (currently nothing) */
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "soc_shutdown: all units detached\n")));
        }
    }

#ifdef INCLUDE_KNET
    soc_knet_cleanup();
#endif

    /* Free up the Level 2 warm boot cache */
    soc_scache_detach(unit);

    return SOC_E_NONE;
}

int
soc_system_scrub(int unit)
{
    int mem, blk, copyno = COPYNO_ALL;
    uint32 *table = NULL;
    soc_mem_t act_mem;
#ifdef BCM_ESW_SUPPORT
    soc_mem_t lock_mem = 0;
#endif
    int c = 0, entry_dw, table_size, tmax = 0;
    int index_cnt, index_min, index_max;

    /* NOTE: This is a temporary way of enabling system scrub case by case on 
     *       different devices as support is being implemented.
     *       Eventually this check should go away or be replaced by a feature.
     */
    if (!(SOC_IS_TD_TT(unit) || SOC_IS_ENDURO(unit) ||
         (SOC_IS_TRIUMPH3(unit))) ||
        soc_property_get(unit, "skip_system_scrub", 0)) {
        return SOC_E_NONE;
    }
    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
        act_mem = mem;
        
        if (!SOC_MEM_IS_VALID(unit, mem)) {
            continue;
        }
        if (soc_mem_index_count(unit, mem) == 0) {
            continue;
        }
#ifdef BCM_ESW_SUPPORT
        _SOC_DRV_MEM_REUSE_MEM_STATE(unit, act_mem, lock_mem);
        /* Avoid compiler warning */
        assert(lock_mem >= 0);
#endif
        if (act_mem != mem) {
            continue;
        }
        if (SOC_MEM_SER_CORRECTION_TYPE(unit, mem) == 0) {
            continue;
        }
#ifdef BCM_SBUSDMA_SUPPORT
        if (soc_feature(unit, soc_feature_sbusdma)) {
            if (mem == ING_SER_FIFOm || mem == EGR_SER_FIFOm ||
                mem == ISM_SER_FIFOm) {
                continue;
            }
        }
#endif /* BCM_SBUSDMA_SUPPORT */
        SOC_MEM_BLOCK_ITER(unit, mem, blk) {
            copyno = blk;
            break;
        }
        if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU) ||
            (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ESM)) {
            continue;
        }
        /* Skip IESMIF mems when ESM is not present */
        else if (SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_IPIPE) {
#ifdef BCM_TRIUMPH3_SUPPORT
            if (SOC_IS_TRIUMPH3(unit) && /* Needs update per chip */
                SOC_MEM_ADDR_STAGE_EXTENDED(SOC_MEM_BASE(unit, mem)) == 0x9) {
                if (!soc_feature(unit, soc_feature_esm_support) ||
                     SAL_BOOT_QUICKTURN) {
                    continue;
                }
            }
#endif /* BCM_TRIUMPH3_SUPPORT */
        }
#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit)) {
            /* Skip unused views */
            if (mem == ESM_PKT_TYPE_ID_ONLYm || mem == ESM_PKT_TYPE_ID_DATA_ONLYm) {
                continue;
            }
            if (mem == MODPORT_MAP_M0m || mem == MODPORT_MAP_M1m ||
                mem == MODPORT_MAP_M2m || mem == MODPORT_MAP_M3m) {
                continue;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT_SUPPORT
        if (SOC_IS_TD_TT(unit) || SOC_IS_TD2_TT2(unit)) {
            int i, skip;
            static soc_mem_t mskip[] = {
                L3_ENTRY_IPV4_MULTICAST_Xm,
                L3_ENTRY_IPV4_MULTICAST_Ym,
                L3_ENTRY_IPV6_UNICAST_Xm,
                L3_ENTRY_IPV6_UNICAST_Ym
            };
            skip = 0;
            for (i=0; i<COUNTOF(mskip); i++) {
                if (mem == mskip[i]) {
                    skip = 1;
                    break;
                }
            }
            if (skip) {
                continue;
            }
                        /* Skip unused views */
            if (mem == ING_SNAT_ONLYm || mem == ING_SNAT_DATA_ONLYm ||
                mem == FP_GLOBAL_MASK_TCAMm ||
                mem == L2_ENTRY_LPm || mem == L3_ENTRY_LPm ||
                mem == VLAN_XLATE_LPm || mem == EGR_VLAN_XLATE_LPm ||
                mem == VLAN_SUBNET_DATA_ONLYm || mem == MODPORT_MAP_M0m || 
                mem == MODPORT_MAP_M1m || mem == MODPORT_MAP_M2m || 
                mem == MODPORT_MAP_M3m || mem == L3_DEFIP_ALPM_RAWm) {
                continue;
            }
        }
#if defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TD2_TT2(unit)) {
            if (mem == L3_DEFIP_ALPM_IPV4m || mem == L3_DEFIP_ALPM_IPV4_1m ||
                mem == L3_DEFIP_ALPM_IPV6_64m || mem == L3_DEFIP_ALPM_IPV6_64_1m ||
                mem == L3_DEFIP_ALPM_IPV6_128m) {
                /* We call a seperate traverse routine to only access the 
                   memory with the right view */                
                continue;
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
        if (SOC_IS_TRX(unit)) {
            /* Skip unused views */
            if (mem == MY_STATION_TCAM_DATA_ONLYm || mem == L2_USER_ENTRY_DATA_ONLYm ||
                mem == L3_DEFIP_DATA_ONLYm || mem == L3_DEFIP_PAIR_128_DATA_ONLYm ||
                mem == L3_DEFIP_128_DATA_ONLYm || mem == L3_DEFIP_128_ONLYm ||
                mem == L3_DEFIP_ONLYm || mem == L3_DEFIP_PAIR_128_ONLYm ||
                mem == L3_ENTRY_ONLYm || 
                mem == MODPORT_MAP_SWm) {
                continue;
            }
        }
#endif /* BCM_TRX_SUPPORT */
        entry_dw = soc_mem_entry_words(unit, mem);
        index_cnt = soc_mem_index_count(unit, mem);
        table_size = index_cnt * entry_dw * 4;
        index_max = index_cnt - 1;
        index_min = soc_mem_index_min(unit, mem);
        if (table == NULL) {
            table = soc_cm_salloc(unit, table_size, "scrub");
            if (table == NULL) {
                return SOC_E_MEMORY;
            }
            tmax = table_size;
        } else {
            if (table_size > tmax) {
                soc_cm_sfree(unit, table);
                table = soc_cm_salloc(unit, table_size, "scrub");
                if (table == NULL) {
                    return SOC_E_MEMORY;
                }
                tmax = table_size;
            }
        }
        SOC_MEM_BLOCK_ITER(unit, mem, blk) {
#if defined(BCM_TRIUMPH3_SUPPORT)
            if (SOC_IS_TRIUMPH3(unit)) {
                if (mem == PORT_EHG_RX_TUNNEL_DATAm ||
                    mem == PORT_EHG_RX_TUNNEL_MASKm ||
                    mem == PORT_EHG_TX_TUNNEL_DATAm) {
                    if (!_soc_tr3_port_mem_blk_enabled(unit, blk)) {
                        continue;
                    }
                }
            }
#endif /* BCM_TRIUMPH3_SUPPORT */
            if (soc_mem_read_range(unit, mem, blk, index_min, index_max,
                                   &table[index_min * entry_dw]) >= 0) {
                /* Do nothing */
                c++;
#if !defined(SOC_NO_NAMES)
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || \
    defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
                LOG_INFO(BSL_LS_SOC_SOCMEM,
                         (BSL_META_U(unit,
                                     "Scrub: %s\n"), SOC_MEM_NAME(unit, mem)));
#endif
#else
                LOG_INFO(BSL_LS_SOC_SOCMEM,
                         (BSL_META_U(unit,
                                     "Scrub: %d\n"), mem));
#endif
            }
        }
    }
    if (table) {
        soc_cm_sfree(unit, table);
        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "Mem scrub count: %d tmax: %d\n"), c, tmax));
    }
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
#ifdef ALPM_ENABLE
        SOC_IF_ERROR_RETURN(soc_trident2_alpm_scrub(unit));
#endif /* ALPM_ENABLE */
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    return soc_ser_reg_load_scrub(unit, 0);
}
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef  BCM_HERCULES15_SUPPORT
/*
 * The 5675A0 has some latches in a test path that runs between all
 * of the XAUI macs.  To ensure that the test path is initialized
 * correctly, all macs must be enabled, and a low frequency reference
 * clock must be given to unused ports.
 */
STATIC void
soc_reset_bcm5675_a0(int unit, uint16 dev_id)
{
    uint64      xctrl;
    int         spl;

    COMPILER_REFERENCE(dev_id);

    /*
     * Enable core_clk to all macs.
     * This does not take effect until the next soft reset.
     */
    WRITE_CMIC_CLK_ENABLEr(unit, 0x1ff);

    /* Block interrupts while resetting */
    spl = sal_splhi();
    SOC_CONTROL(unit)->soc_flags |= SOC_F_BUSY;
    soc_pci_write(unit, CMIC_CONFIG,
                  soc_pci_read(unit, CMIC_CONFIG) | CC_RESET_CPS);
    /* Unblock interrupts */
    sal_spl(spl);

    if (SAL_BOOT_QUICKTURN) {
        sal_usleep(10 * MILLISECOND_USEC);
    } else {
        sal_usleep(1 * MILLISECOND_USEC);
    }

    soc_endian_config(unit);    /* reset cleared endian settings */
    /* Block interrupts */
    spl = sal_splhi();
    SOC_CONTROL(unit)->soc_flags &= ~SOC_F_BUSY;
    /* Unblock interrupts */
    sal_spl(spl);

    /*
     * Force all ports (even disabled ones) to run for a little
     * while by clearing iddq and pwrdwn.  The definition of little
     * while depends on the refclock given to external ports, which
     * is assumed here to be at least 100KHz or so
     */
    /* defaults except for iddq=0 pwrdwn=0 */
    COMPILER_64_SET(xctrl, 0, 0x181);
    soc_reg64_write(unit, 0x500201, xctrl);     /* MAC_XGXS_CTRLr */
    soc_reg64_write(unit, 0x600201, xctrl);
    soc_reg64_write(unit, 0x700201, xctrl);
    soc_reg64_write(unit, 0x800201, xctrl);
    soc_reg64_write(unit, 0x100201, xctrl);
    soc_reg64_write(unit, 0x200201, xctrl);
    soc_reg64_write(unit, 0x300201, xctrl);
    soc_reg64_write(unit, 0x400201, xctrl);

    sal_usleep(60);

    /* back to reset defaults: iddq=1 pwrdwn=1 */
    COMPILER_64_SET(xctrl, 0, 0x1e1);
    soc_reg64_write(unit, 0x500201, xctrl);
    soc_reg64_write(unit, 0x600201, xctrl);
    soc_reg64_write(unit, 0x700201, xctrl);
    soc_reg64_write(unit, 0x800201, xctrl);
    soc_reg64_write(unit, 0x100201, xctrl);
    soc_reg64_write(unit, 0x200201, xctrl);
    soc_reg64_write(unit, 0x300201, xctrl);
    soc_reg64_write(unit, 0x400201, xctrl);
}
#endif  /* BCM_HERCULES15_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
/*
 * Reset VCO sequencer.
 */
void
soc_xgxs_lcpll_reset(int unit)
{
    if (soc_feature(unit, soc_feature_xgxs_lcpll)) {
        uint32 val;

        READ_CMIC_XGXS_PLL_CONTROL_1r(unit, &val);
#if defined(BCM_RAPTOR_SUPPORT)
        if (soc_reg_field_valid
                (unit, CMIC_XGXS_PLL_CONTROL_1r, SEL_DIFF_CLOCKf)) {
            uint32 xtal_sel;
            if (soc_property_get(unit, spn_XGXS_LCPLL_XTAL_REFCLK, 0) == 0) {
                xtal_sel = 1;
            } else {
                xtal_sel = 0;
            }
            soc_reg_field_set
                (unit, CMIC_XGXS_PLL_CONTROL_1r,
                 &val, SEL_DIFF_CLOCKf, xtal_sel);
        }
#endif /* BCM_RAPTOR_SUPPORT */

        soc_reg_field_set(unit, CMIC_XGXS_PLL_CONTROL_1r, &val, RESETf, 1);
        WRITE_CMIC_XGXS_PLL_CONTROL_1r(unit, val);
        soc_reg_field_set(unit, CMIC_XGXS_PLL_CONTROL_1r, &val, RESETf, 0);
        sal_usleep(50);
        WRITE_CMIC_XGXS_PLL_CONTROL_1r(unit, val);
        sal_usleep(50);
    }
}

/*
 * Check for PLL lock for Bradley
 */
void
_bradley_lcpll_lock_check(int unit)
{
    uint32 val;
    int pll_lock_usec;
    int locked = 0;
    int retry = 3;
    soc_timeout_t to;

    /* Check if first LCPLL locked */
    while (!locked && retry--) {
        READ_CMIC_XGXS0_PLL_CONTROL_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_XGXS0_PLL_CONTROL_1r, &val,
                          PLL_SEQSTARTf, 1);
        WRITE_CMIC_XGXS0_PLL_CONTROL_1r(unit, val);
        sal_usleep(100);

        READ_CMIC_XGXS0_PLL_CONTROL_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_XGXS0_PLL_CONTROL_1r, &val,
                          PLLFORCECAPDONE_ENf, 1);
        soc_reg_field_set(unit, CMIC_XGXS0_PLL_CONTROL_1r, &val,
                          PLLFORCECAPDONEf, 1);
        soc_reg_field_set(unit, CMIC_XGXS0_PLL_CONTROL_1r, &val,
                          PLLFORCECAPPASS_ENf, 1);
        soc_reg_field_set(unit, CMIC_XGXS0_PLL_CONTROL_1r, &val,
                          PLLFORCECAPPASSf, 1);
        WRITE_CMIC_XGXS0_PLL_CONTROL_1r(unit, val);
        sal_usleep(100);

        READ_CMIC_XGXS0_PLL_CONTROL_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_XGXS0_PLL_CONTROL_1r, &val,
                          PLL_SEQSTARTf, 0);
        WRITE_CMIC_XGXS0_PLL_CONTROL_1r(unit, val);
        sal_usleep(50);

        pll_lock_usec = 500000;
        soc_timeout_init(&to, pll_lock_usec, 0);
        while (!soc_timeout_check(&to)) {
            READ_CMIC_XGXS0_PLL_CONTROL_2r(unit, &val);
            locked = soc_reg_field_get(unit, CMIC_XGXS0_PLL_CONTROL_2r,
                                       val, CMIC_XGPLL_LOCKf);
            if (locked) {
                break;
            }
        }
    }

    if (!locked) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "bradley_lcpll_lock_check: LCPLL0 not locked on unit %d "
                              "status = 0x%08x\n"),
                   unit, val));
    }

    /* Now check for second LCPLL */
    locked = 0;
    retry = 3;
    while (!locked && retry--) {
        READ_CMIC_XGXS1_PLL_CONTROL_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_XGXS1_PLL_CONTROL_1r, &val,
                          PLL_SEQSTARTf, 1);
        WRITE_CMIC_XGXS1_PLL_CONTROL_1r(unit, val);
        sal_usleep(100);

        READ_CMIC_XGXS1_PLL_CONTROL_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_XGXS1_PLL_CONTROL_1r, &val,
                          PLLFORCECAPDONE_ENf, 1);
        soc_reg_field_set(unit, CMIC_XGXS1_PLL_CONTROL_1r, &val,
                          PLLFORCECAPDONEf, 1);
        soc_reg_field_set(unit, CMIC_XGXS1_PLL_CONTROL_1r, &val,
                          PLLFORCECAPPASS_ENf, 1);
        soc_reg_field_set(unit, CMIC_XGXS1_PLL_CONTROL_1r, &val,
                          PLLFORCECAPPASSf, 1);
        WRITE_CMIC_XGXS1_PLL_CONTROL_1r(unit, val);
        sal_usleep(100);

        READ_CMIC_XGXS1_PLL_CONTROL_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_XGXS1_PLL_CONTROL_1r, &val,
                          PLL_SEQSTARTf, 0);
        WRITE_CMIC_XGXS1_PLL_CONTROL_1r(unit, val);
        sal_usleep(50);

        pll_lock_usec = 500000;
        soc_timeout_init(&to, pll_lock_usec, 0);
        while (!soc_timeout_check(&to)) {
            READ_CMIC_XGXS1_PLL_CONTROL_2r(unit, &val);
            locked = soc_reg_field_get(unit, CMIC_XGXS1_PLL_CONTROL_2r,
                                            val, CMIC_XGPLL_LOCKf);
            if (locked) {
                break;
            }
        }
    }

    if (!locked) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "bradley_lcpll_lock_check: LCPLL1 not locked on unit %d "
                              "status = 0x%08x\n"),
                   unit, val));
    }

}

#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_BIGMAC_SUPPORT
#ifdef BCM_TRIUMPH_SUPPORT
static const struct {
    soc_reg_t mdio_conf_reg;
    int hc_chan;
} tr_xgxs_map[] = {
    {                 INVALIDr, -1, }, /* port  0 */
    {                 INVALIDr, -1, }, /* port  1 */
    { CMIC_XGXS_MDIO_CONFIG_0r,  0, }, /* port  2 */
    { CMIC_XGXS_MDIO_CONFIG_0r,  1, }, /* port  3 */
    { CMIC_XGXS_MDIO_CONFIG_0r,  2, }, /* port  4 */
    { CMIC_XGXS_MDIO_CONFIG_0r,  3, }, /* port  5 */
    { CMIC_XGXS_MDIO_CONFIG_1r,  0, }, /* port  6 */
    { CMIC_XGXS_MDIO_CONFIG_1r,  1, }, /* port  7 */
    {                 INVALIDr, -1, }, /* port  8 */
    {                 INVALIDr, -1, }, /* port  9 */
    {                 INVALIDr, -1, }, /* port 10 */
    {                 INVALIDr, -1, }, /* port 11 */
    {                 INVALIDr, -1, }, /* port 12 */
    {                 INVALIDr, -1, }, /* port 13 */
    { CMIC_XGXS_MDIO_CONFIG_2r,  0, }, /* port 14 */
    { CMIC_XGXS_MDIO_CONFIG_2r,  1, }, /* port 15 */
    { CMIC_XGXS_MDIO_CONFIG_2r,  2, }, /* port 16 */
    { CMIC_XGXS_MDIO_CONFIG_2r,  3, }, /* port 17 */
    { CMIC_XGXS_MDIO_CONFIG_1r,  2, }, /* port 18 */
    { CMIC_XGXS_MDIO_CONFIG_1r,  3, }, /* port 19 */
    {                 INVALIDr, -1, }, /* port 20 */
    {                 INVALIDr, -1, }, /* port 21 */
    {                 INVALIDr, -1, }, /* port 22 */
    {                 INVALIDr, -1, }, /* port 23 */
    {                 INVALIDr, -1, }, /* port 24 */
    {                 INVALIDr, -1, }, /* port 25 */
    { CMIC_XGXS_MDIO_CONFIG_3r,  0, }, /* port 26 */
    { CMIC_XGXS_MDIO_CONFIG_5r,  0, }, /* port 27 */
    { CMIC_XGXS_MDIO_CONFIG_6r, -1, }, /* port 28 */
    { CMIC_XGXS_MDIO_CONFIG_7r, -1, }, /* port 29 */
    { CMIC_XGXS_MDIO_CONFIG_8r, -1, }, /* port 30 */
    { CMIC_XGXS_MDIO_CONFIG_9r, -1, }, /* port 31 */
    { CMIC_XGXS_MDIO_CONFIG_3r,  1, }, /* port 32 */
    { CMIC_XGXS_MDIO_CONFIG_3r,  2, }, /* port 33 */
    { CMIC_XGXS_MDIO_CONFIG_3r,  3, }, /* port 34 */
    { CMIC_XGXS_MDIO_CONFIG_4r,  0, }, /* port 35 */
    { CMIC_XGXS_MDIO_CONFIG_4r,  1, }, /* port 36 */
    {                 INVALIDr, -1, }, /* port 37 */
    {                 INVALIDr, -1, }, /* port 38 */
    {                 INVALIDr, -1, }, /* port 39 */
    {                 INVALIDr, -1, }, /* port 40 */
    {                 INVALIDr, -1, }, /* port 41 */
    {                 INVALIDr, -1, }, /* port 42 */
    { CMIC_XGXS_MDIO_CONFIG_5r,  1, }, /* port 43 */
    { CMIC_XGXS_MDIO_CONFIG_5r,  2, }, /* port 44 */
    { CMIC_XGXS_MDIO_CONFIG_5r,  3, }, /* port 45 */
    { CMIC_XGXS_MDIO_CONFIG_4r,  2, }, /* port 46 */
    { CMIC_XGXS_MDIO_CONFIG_4r,  3, }, /* port 47 */
    {                 INVALIDr, -1, }, /* port 48 */
    {                 INVALIDr, -1, }, /* port 49 */
    {                 INVALIDr, -1, }, /* port 50 */
    {                 INVALIDr, -1, }, /* port 51 */
    {                 INVALIDr, -1, }, /* port 52 */
    {                 INVALIDr, -1, }, /* port 53 */
};
#endif /* BCM_TRIUMPH_SUPPORT */

STATIC int
_soc_xgxs_mdio_setup(int unit, int port)
{
    soc_reg_t reg = INVALIDr;
    uint32 rval;
    uint32 devad = 0;
    int skip = FALSE;

    if (!SOC_PORT_VALID(unit, port))
    {
        return SOC_E_PORT;
    }

    if (SOC_IS_FB_FX_HX(unit)) {
        switch(port) {
        case 24: reg = CMIC_XGXS_MDIO_CONFIG_0r; break;
        case 25: reg = CMIC_XGXS_MDIO_CONFIG_1r; break;
        case 26: reg = CMIC_XGXS_MDIO_CONFIG_2r; break;
        case 27: reg = CMIC_XGXS_MDIO_CONFIG_3r; break;
        default: return SOC_E_PARAM;
        }
    }

#ifdef BCM_GXPORT_SUPPORT
    if (SOC_IS_HB_GW(unit)) {
        switch(port) {
        case  0: reg = CMIC_XGXS_MDIO_CONFIG_0r; break;
        case  1: reg = CMIC_XGXS_MDIO_CONFIG_1r; break;
        case  2: reg = CMIC_XGXS_MDIO_CONFIG_2r; break;
        case  3: reg = CMIC_XGXS_MDIO_CONFIG_3r; break;
        case  4: reg = CMIC_XGXS_MDIO_CONFIG_4r; break;
        case  5: reg = CMIC_XGXS_MDIO_CONFIG_5r; break;
        case  6: reg = CMIC_XGXS_MDIO_CONFIG_6r; break;
        case  7: reg = CMIC_XGXS_MDIO_CONFIG_7r; break;
        case  8: reg = CMIC_XGXS_MDIO_CONFIG_8r; break;
        case  9: reg = CMIC_XGXS_MDIO_CONFIG_9r; break;
        case 10: reg = CMIC_XGXS_MDIO_CONFIG_10r; break;
        case 11: reg = CMIC_XGXS_MDIO_CONFIG_11r; break;
        case 12: reg = CMIC_XGXS_MDIO_CONFIG_12r; break;
        case 13: reg = CMIC_XGXS_MDIO_CONFIG_13r; break;
        case 14: reg = CMIC_XGXS_MDIO_CONFIG_14r; break;
        case 15: reg = CMIC_XGXS_MDIO_CONFIG_15r; break;
        case 16: reg = CMIC_XGXS_MDIO_CONFIG_16r; break;
        case 17: reg = CMIC_XGXS_MDIO_CONFIG_17r; break;
        case 18: reg = CMIC_XGXS_MDIO_CONFIG_18r; break;
        case 19: reg = CMIC_XGXS_MDIO_CONFIG_19r; break;
        default: return SOC_E_PARAM;
        }
    }
#endif

#ifdef BCM_SCORPION_SUPPORT
    if (SOC_IS_SC_CQ(unit)) {
        devad = 5;
        switch(port) {
        case  1: reg = CMIC_XGXS_MDIO_CONFIG_0r; break;
        case  2: reg = CMIC_XGXS_MDIO_CONFIG_1r; break;
        case  3: reg = CMIC_XGXS_MDIO_CONFIG_2r; break;
        case  4: reg = CMIC_XGXS_MDIO_CONFIG_3r; break;
        case  5: reg = CMIC_XGXS_MDIO_CONFIG_4r; break;
        case  6: reg = CMIC_XGXS_MDIO_CONFIG_5r; break;
        case  7: reg = CMIC_XGXS_MDIO_CONFIG_6r; break;
        case  8: reg = CMIC_XGXS_MDIO_CONFIG_7r; break;
        case  9: reg = CMIC_XGXS_MDIO_CONFIG_8r; break;
        case 10: reg = CMIC_XGXS_MDIO_CONFIG_9r; break;
        case 11: reg = CMIC_XGXS_MDIO_CONFIG_10r; break;
        case 12: reg = CMIC_XGXS_MDIO_CONFIG_11r; break;
        case 13: reg = CMIC_XGXS_MDIO_CONFIG_12r; break;
        case 14: reg = CMIC_XGXS_MDIO_CONFIG_13r; break;
        case 15: reg = CMIC_XGXS_MDIO_CONFIG_14r; break;
        case 16: reg = CMIC_XGXS_MDIO_CONFIG_15r; break;
        case 17: reg = CMIC_XGXS_MDIO_CONFIG_16r; break;
        case 18: reg = CMIC_XGXS_MDIO_CONFIG_17r; break;
        case 19: reg = CMIC_XGXS_MDIO_CONFIG_18r; break;
        case 20: reg = CMIC_XGXS_MDIO_CONFIG_19r; break;
        case 21: reg = CMIC_XGXS_MDIO_CONFIG_20r; break;
        case 22: reg = CMIC_XGXS_MDIO_CONFIG_21r; break;
        case 23: reg = CMIC_XGXS_MDIO_CONFIG_22r; break;
        case 24: reg = CMIC_XGXS_MDIO_CONFIG_23r; break;
        case 25:
        case 26:
        case 27:
        case 28:
            reg = CMIC_XGXS_MDIO_CONFIG_24r;
            devad = 0;
            break;
        default: return SOC_E_PARAM;
        }
    }
#endif

#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        reg = tr_xgxs_map[port].mdio_conf_reg;
        devad = 5;
        if (reg == INVALIDr) {
            return SOC_E_PARAM;
        }
    }
#endif

#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
        SOC_IS_VALKYRIE2(unit)) {
        devad = 5;
        switch(port) {
        case 30: case 31: case 32: case 33:
            reg = CMIC_XGXS_MDIO_CONFIG_0r;
            break;
        case 34: case 35: case 36: case 37:
            reg = CMIC_XGXS_MDIO_CONFIG_1r;
            break;
        case 38: case 39: case 40: case 41:
            reg = CMIC_XGXS_MDIO_CONFIG_2r;
            break;
        case 42: case 43: case 44: case 45:
            reg = CMIC_XGXS_MDIO_CONFIG_3r;
            break;
        case 46: case 47: case 48: case 49:
            reg = CMIC_XGXS_MDIO_CONFIG_4r;
            break;
        case 50: case 51: case 52: case 53:
            reg = CMIC_XGXS_MDIO_CONFIG_5r;
            break;
        case 26: reg = CMIC_XGXS_MDIO_CONFIG_6r; break;
        case 27: reg = CMIC_XGXS_MDIO_CONFIG_7r; break;
        case 28: reg = CMIC_XGXS_MDIO_CONFIG_8r; break;
        case 29: reg = CMIC_XGXS_MDIO_CONFIG_9r; break;
        default: return SOC_E_PARAM;
        }
    }
#endif
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        /*
         * MD_DEVADf and IEEE_DEVICES_IN_PKGf are not used
         * MD_ST uses reset value (clause 22)
         */
        skip = TRUE;
    }
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        int     blk_idx;

        blk_idx = SOC_PORT_BLOCK(unit, port);
        switch (blk_idx) {
            case 0: reg = CMIC_XGXS_MDIO_CONFIG_0r; break; 
            case 1: reg = CMIC_XGXS_MDIO_CONFIG_1r; break; 
            case 2: reg = CMIC_XGXS_MDIO_CONFIG_2r; break; 
            case 3: reg = CMIC_XGXS_MDIO_CONFIG_3r; break; 
            case 4: reg = CMIC_XGXS_MDIO_CONFIG_4r; break; 
            case 5: reg = CMIC_XGXS_MDIO_CONFIG_5r; break; 
            default: return SOC_E_PARAM;
        }
    }
#endif /* BCM_SHADOW_SUPPORT */

#ifdef BCM_ENDURO_SUPPORT
    if (SOC_IS_ENDURO(unit)) {
        devad = 5;
        switch(port) {
        case 26: reg = CMIC_XGXS_MDIO_CONFIG_0r; break;
        case 27: reg = CMIC_XGXS_MDIO_CONFIG_1r; break;
        case 28: reg = CMIC_XGXS_MDIO_CONFIG_2r; break;
        case 29: reg = CMIC_XGXS_MDIO_CONFIG_3r; break;
        default: return SOC_E_PARAM;
        }
    }
#endif /* BCM_ENDURO_SUPPORT */

#ifdef BCM_HURRICANE_SUPPORT
    if (SOC_IS_HURRICANE(unit)) {
        devad = 5;
        switch(port) {
        case 26: reg = CMIC_XGXS_MDIO_CONFIG_0r; break;
        case 27: reg = CMIC_XGXS_MDIO_CONFIG_1r; break;
        case 28: reg = CMIC_XGXS_MDIO_CONFIG_2r; break;
        case 29: reg = CMIC_XGXS_MDIO_CONFIG_3r; break;
        default: return SOC_E_PARAM;
        }
    }
#endif /* BCM_HURRICANE_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANAX(unit)) {
        devad = 5;
        switch(port) {
        case 25: reg = TOP_XGXS_MDIO_CONFIG_0r; break;
        case 26: reg = TOP_XGXS_MDIO_CONFIG_1r; break;
        case 27: 
        case 32: 
        case 33: 
        case 34: reg = TOP_XGXS_MDIO_CONFIG_2r; break;
        case 28: 
        case 29: 
        case 30: 
        case 31: reg = TOP_XGXS_MDIO_CONFIG_3r; break;
        default: return SOC_E_PARAM;
        }
     }
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        /*
         * MD_DEVADf and IEEE_DEVICES_IN_PKGf are not used
         * MD_ST uses reset value (clause 22)
         */
        skip = TRUE;
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    if (!skip) {
        if (reg == INVALIDr) {
            return SOC_E_UNAVAIL;
        }
#ifdef BCM_CMICM_SUPPORT
        if (soc_feature(unit, soc_feature_cmicm)) {
            /* In CMICm these regs are accessed via SCHAN */
            SOC_IF_ERROR_RETURN
                (soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &rval));
            soc_reg_field_set(unit, reg, &rval, MD_DEVADf, devad);
            soc_reg_field_set(unit, reg, &rval, IEEE_DEVICES_IN_PKGf,
                              IS_HG_PORT(unit, port) ? 0x03 : 0x15);
            SOC_IF_ERROR_RETURN
                (soc_reg32_set(unit, reg, REG_PORT_ANY, 0, rval));
        } else
#endif /* CMICm Support */
        {
            rval = soc_pci_read
                (unit, soc_reg_addr(unit, reg, REG_PORT_ANY, 0));
            soc_reg_field_set(unit, reg, &rval, MD_DEVADf, devad);
            soc_reg_field_set(unit, reg, &rval, IEEE_DEVICES_IN_PKGf,
                              IS_HG_PORT(unit, port) ? 0x03 : 0x15);
            soc_pci_write
                (unit, soc_reg_addr(unit, reg, REG_PORT_ANY, 0), rval);
        }
    }
    if (SAL_BOOT_QUICKTURN) {
#ifdef  BCM_FIREBOLT_SUPPORT
        /* Kick the QT phy model */
        if (SOC_IS_FBX(unit)) {
            uint16 rv, kv;
            int phy_addr;

            rv = 0x0800; /* Reset */
            if (IS_HG_PORT(unit,port)) {
                kv = 0x0000; /* Configure for HG mode */
            } else {
                kv = 0x0002; /* Configure for XE mode */
            }
            phy_addr = port + 0x41;
            if (SOC_IS_SCORPION(unit)) {
                /* Skip over CMIC at port 0, use bus 1 instead of 2 */
                phy_addr -= 0x21;
            }
            SOC_IF_ERROR_RETURN(soc_miim_write(unit, phy_addr, 0x00, rv));
            SOC_IF_ERROR_RETURN(soc_miim_write(unit, phy_addr, 0x00, kv));
        } else
#endif /* BCM_FIREBOLT_SUPPORT */
        {
            SOC_IF_ERROR_RETURN(soc_miim_write(unit, port + 1, 0x00, 0x0000));
        }
    }

    return SOC_E_NONE;
}

#endif /* BCM_BIGMAC_SUPPORT */

#if defined(BCM_HELIX15_SUPPORT) || defined(BCM_FELIX15_SUPPORT) || \
        defined(BCM_FIREBOLT2_SUPPORT)
/*
 *      Get LMD Enable status and update the SOC lmd_pbm.
 */
STATIC int
_xmac_lmd_enable_status_update(int unit)
{

    SOC_PBMP_CLEAR(SOC_LMD_PBM(unit));

    if (soc_feature(unit, soc_feature_lmd)) {
        uint32      dev_cfg;
        uint32      lmd_enable = 0;

        if (SOC_IS_FIREBOLT2(unit)) {
#ifndef EXCLUDE_BCM56324
            
#endif /* EXCLUDE_BCM56324 */
            lmd_enable = soc_property_get(unit, spn_LMD_ENABLE_PBMP,
                                          0);

        } else if (!SAL_BOOT_QUICKTURN) {
            SOC_IF_ERROR_RETURN(READ_BSAFE_GLB_DEV_STATUSr(unit, &dev_cfg));
            if (soc_reg_field_get(unit,
                                  BSAFE_GLB_DEV_STATUSr,
                                  dev_cfg,
                                  PROD_CFG_VLDf)) {

                SOC_IF_ERROR_RETURN
                    (READ_BSAFE_GLB_PROD_CFGr(unit, &dev_cfg));
                lmd_enable = soc_reg_field_get(unit,
                                               BSAFE_GLB_PROD_CFGr,
                                               dev_cfg,
                                               LMD_ENABLEf);
            }
        }

        lmd_enable <<= SOC_HG_OFFSET(unit);
        SOC_PBMP_WORD_SET(SOC_LMD_PBM(unit), 0, lmd_enable);
    }

    return SOC_E_NONE;
}
#endif /* BCM_HELIX15_SUPPORT || BCM_FELIX15_SUPPORT ||
          BCM_FIREBOLT2_SUPPORT */

#ifdef BCM_FIREBOLT_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56504_a0
 * Purpose:
 *      Special reset sequencing for BCM56504
 *      Setup SBUS block mapping.
 */

STATIC void
soc_reset_bcm56504_a0(int unit)
{
    uint32              val, sbus_val, to_usec;
    val = 0;

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);

    /* Reset all blocks */
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);


    /* bring the blocks out of reset */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG_PLL_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val,
                      CMIC_BSAFE_CLKGEN_RST_Lf,1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(50);
    soc_xgxs_lcpll_reset(unit);
    sal_usleep(to_usec);

#if defined(BCM_HELIX_SUPPORT) || defined(BCM_FELIX_SUPPORT)
    /* Felix/Helix only */
    if (SOC_IS_FX_HX(unit)) {
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GXP_RST_Lf, 1);
    }
#endif /* BCM_FELIX_SUPPORT || BCM_HELIX_SUPPORT */
    if (soc_reg_field_valid(unit, CMIC_SOFT_RESET_REGr, CMIC_BSAFE_RST_Lf)) {
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_BSAFE_RST_Lf, 1);
    }
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    /*
     * Firebolt/Helix/Felix
     * ring0 [00] : IPIPE[7] -> IPIPE_HI[8]
     * ring1 [01] : EPIPE[9] -> EPIPE_HI[10]
     * ring2 [10] : gport0[0] ->  gport1[1] -> xport0[2] ->
     *              xport1[3] ->  xport2[4] -> xport3[5] -> MMU[6]
     *              gport[12] ->  gport[13] (Felix)
     * ring3 [11] : bsafe[11]
     *
     * 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1 0
     * 0000__1010__1101__0100__0010__1010__1010__1010
     */

    sbus_val = 0x0ad42aaa;
    WRITE_CMIC_SBUS_RING_MAPr(unit, sbus_val);
    /*
     * Min 20 msec delay after LCPLL reset before checking for lock.
     * Lock checking required only if HG/XE ports present.
     */
    if (SOC_PBMP_NOT_NULL(PBMP_XE_ALL(unit)) ||
        SOC_PBMP_NOT_NULL(PBMP_HG_ALL(unit))) {
        soc_xgxs_lcpll_lock_check(unit);
    }

#if defined(BCM_HELIX_SUPPORT) || defined(BCM_FELIX_SUPPORT)
    /*
     * Verify Serdes base address is valid before removing Gigabit reset
     */
    if (!SAL_BOOT_QUICKTURN && SOC_IS_FX_HX(unit)) {
        soc_timeout_t   to;
        uint32          dev_cfg = 0;
        int             rv = SOC_E_NONE;

        soc_timeout_init(&to, 1000000, 1); /* One second timeout */

        for (;;) {
            if ((rv = READ_BSAFE_GLB_DEV_STATUSr(unit, &dev_cfg)) < 0) {
                break;
            }
            if (soc_reg_field_get(unit,
                                  BSAFE_GLB_DEV_STATUSr,
                                  dev_cfg,
                                  PROD_CFG_VLDf) == 1) {
                rv = SOC_E_NONE;
                break;
            }
            if (soc_reg_field_get(unit,
                                  BSAFE_GLB_DEV_STATUSr,
                                  dev_cfg,
                                  INIT_DONEf) == 1) {
                if ((soc_reg_field_get(unit,
                                       BSAFE_GLB_DEV_STATUSr,
                                       dev_cfg, INIT_KEYf) == 0) &&
                    (soc_reg_field_get(unit,
                                       BSAFE_GLB_DEV_STATUSr,
                                       dev_cfg, BUSYf) == 0)) {
                    rv = SOC_E_NONE;
                    break;
                }
                
            }
            if (soc_timeout_check(&to)) {
                rv = SOC_E_TIMEOUT;
                break;
            }
            sal_usleep(to_usec);
        }
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "soc_reset_bcm56504_a0: Device status valid check failed on unit %d\n"),
                       unit));
        }
    }
#endif /* BCM_HELIX_SUPPORT || BCM_FELIX_SUPPORT */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX4_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);

#if defined(BCM_HELIX15_SUPPORT) || defined(BCM_FELIX15_SUPPORT) || \
        defined(BCM_FIREBOLT2_SUPPORT)
    /*
     * Extract the XPORT LMD operation mode.
     */
    if (_xmac_lmd_enable_status_update(unit) < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_reset_bcm56504_a0: LMD status update failed on unit %d\n"),
                   unit));
    }
#endif /* BCM_HELIX15_SUPPORT || BCM_FELIX15_SUPPORT ||
          BCM_FIREBOLT2_SUPPORT */
}
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_RAPTOR_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56218_a0
 * Purpose:
 *      Special reset sequencing for BCM56218
 *      Setup SBUS block mapping.
 */

STATIC void
soc_reset_bcm56218_a0(int unit)
{
    uint32              val, to_usec;
    val = 0;

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);

    /* Reset all blocks */
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);


    /* bring the blocks out of reset */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG_PLL_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_G2P51_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_G2P50_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX12_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX2_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(50);
    soc_xgxs_lcpll_reset(unit);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_FP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf, 1);

    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    /*
     * Raptor
     * ring0 [00] : IPIPE[7] -> IPIPE_HI[8]
     * ring1 [01] : EPIPE[9] -> EPIPE_HI[10]
     * ring2 [10] : gport4[1] -> gport0[2] ->
     *              gport1[3] ->  gport2[4] -> gport3[5] -> MMU[6]
     * ring3 [11] : bsafe[11]
     *
     * 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1 0
     * 0000__XXXX__1101__0100__0010__1010__1010__10XX
     */

    val = 0x0ad42aaa;
    WRITE_CMIC_SBUS_RING_MAPr(unit, val);
    /*
     * Min 20 msec delay after LCPLL reset before checking for lock.
     */
    soc_xgxs_lcpll_lock_check(unit);
}
#endif /* BCM_RAPTOR_SUPPORT */

#if defined(BCM_RAVEN_SUPPORT)
STATIC void
soc_mdio_setup(int unit)
{
    uint32 val;

    READ_CMIC_GFPORT_CLOCK_CONFIGr(unit, &val);
    soc_reg_field_set(unit, CMIC_GFPORT_CLOCK_CONFIGr, &val, 
                      SEL_DOZEN_SERDES_0_REF_CLK_SRCf, 3);
    soc_reg_field_set(unit, CMIC_GFPORT_CLOCK_CONFIGr, &val,
                      SEL_DOZEN_SERDES_1_REF_CLK_SRCf, 3);
    soc_reg_field_set(unit, CMIC_GFPORT_CLOCK_CONFIGr, &val,
                      PROG_DOZEN_SERDES_PLLDIV_CTRL_DEFf, 3);                    
    WRITE_CMIC_GFPORT_CLOCK_CONFIGr(unit, val);

    READ_CMIC_GFPORT_CLOCK_CONFIGr(unit, &val);
    soc_reg_field_set(unit, CMIC_GFPORT_CLOCK_CONFIGr, &val, 
                      SEL_DOZEN_SERDES_0_REF_CLK_SRCf, 1);
    soc_reg_field_set(unit, CMIC_GFPORT_CLOCK_CONFIGr, &val,
                      SEL_DOZEN_SERDES_1_REF_CLK_SRCf, 1);
    soc_reg_field_set(unit, CMIC_GFPORT_CLOCK_CONFIGr, &val,
                      PROG_DOZEN_SERDES_PLLDIV_CTRL_DEFf, 0);                    
    WRITE_CMIC_GFPORT_CLOCK_CONFIGr(unit, val);

    return;
}

/*
 * Function:
 *      soc_reset_bcm56224_a0
 * Purpose:
 *      Special reset sequencing for BCM56224
 *      Setup SBUS block mapping.
 */

STATIC void
soc_reset_bcm56224_a0(int unit)
{
    uint32              val, to_usec;
    val = 0;

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);

    /* Reset all blocks */
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);


    /* bring the blocks out of reset */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG_PLL_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_G2P51_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_G2P50_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_G2P52_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_G2P53_RST_Lf, 1);
    if (soc_feature(unit, soc_feature_mdio_setup)) {
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX12_RST_Lf, 0);
    } else {
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX12_RST_Lf, 1);
    }
    /*soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX2_RST_Lf, 1);*/
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(50);
    soc_xgxs_lcpll_reset(unit);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_FP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf, 1);

    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);
    /*
     * Raptor
     * ring0 [00] : IPIPE[7] -> IPIPE_HI[8]
     * ring1 [01] : EPIPE[9] -> EPIPE_HI[10]
     * ring2 [10] : gport4[1] -> gport0[2] ->
     *              gport1[3] ->  gport2[4] -> gport3[5] -> MMU[6]
     * ring3 [11] : bsafe[11]
     *
     * 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1 0
     * 0000__XXXX__1101__0100__0010__1010__1010__10XX
     */

    val = 0x0ad42aaa;
    WRITE_CMIC_SBUS_RING_MAPr(unit, val);
    /*
     * Min 20 msec delay after LCPLL reset before checking for lock.
     */
    soc_xgxs_lcpll_lock_check(unit);
    if (soc_feature(unit, soc_feature_mdio_setup)) {
        soc_mdio_setup(unit);
    }
}
#endif /* BCM_RAVEN_SUPPORT */

#if defined(BCM_HAWKEYE_SUPPORT)
/*
 * Function:
 *      soc_reset_bcm53314_a0
 * Purpose:
 *      Special reset sequencing for BCM53314
 *      Setup SBUS block mapping.
 */

STATIC void
soc_reset_bcm53314_a0(int unit)
{
    uint32  val, rval, to_usec;
    int i, retry = 3, locked = 0;

    to_usec = SAL_BOOT_QUICKTURN ? (500 * MILLISECOND_USEC) :
                                   (25 * MILLISECOND_USEC);

    /* Reset all blocks */
    val = 0;
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);


    /* Bring LCPLL out of reset */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG_PLL_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);
    /* Check lock status */
    if ((!SAL_BOOT_PLISIM) && (!SAL_BOOT_QUICKTURN)) {
        for (i = 0; i < retry ; i++) {
            READ_CMIC_XGXS_PLL_STATUSr(unit, &rval);
            locked = soc_reg_field_get(unit, CMIC_XGXS_PLL_STATUSr,
                                            rval, CMIC_XG_PLL_LOCKf);
            if (locked)
                break;
            sal_usleep(to_usec);
        }
        if (!locked) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL not locked on unit %d "
                                  "status = 0x%08x\n"),
                       unit, rval));
        }
    }

    /* Bring qsgmii/qgphy out of reset */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_QSGMII2X0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_QSGMII2X1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_QGPHY0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_QGPHY1_RST_Lf, 1);

    /* Wait for qsgmii/qgphy PLL lock */
    sal_usleep(to_usec);

    /* Bring gport/ip/ep/mmu out of reset */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);

    sal_usleep(to_usec);
    /*
     * Raptor
     * ring0 [00] : IPIPE[7] -> IPIPE_HI[8]
     * ring1 [01] : EPIPE[9] -> EPIPE_HI[10]
     * ring2 [10] : gport4[1] -> gport0[2] ->
     *              gport1[3] ->  gport2[4] -> gport3[5] -> MMU[6]
     * ring3 [11] : bsafe[11]
     *
     * 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1 0
     * 0000__XXXX__1101__0100__0010__1010__1010__10XX
     */

    val = 0x0ad42aaa;
    WRITE_CMIC_SBUS_RING_MAPr(unit, val);

    /* set UNMANAGED_MODE of CMIC_CHIP_MODE_CONTROL to 0 in the init sequence */
    val = 0x0;
    WRITE_CMIC_CHIP_MODE_CONTROLr(unit, val);

    /* set CMIC_INTR_WAIT_CYCLES to 0 in the init sequence */
    val = 0x0;
    WRITE_CMIC_INTR_WAIT_CYCLESr(unit, val);

    READ_CMIC_QGPHY_QSGMII_CONTROLr(unit, &val);
    soc_reg_field_set(unit, CMIC_QGPHY_QSGMII_CONTROLr, &val, SEL_LEDRAM_SERIAL_DATAf, 1);
    WRITE_CMIC_QGPHY_QSGMII_CONTROLr(unit, val);
}
#endif /* BCM_HAWKEYE_SUPPORT */

#if defined (BCM_BRADLEY_SUPPORT)
/*
 * Function:
 *      soc_reset_bcm56800_a0
 * Purpose:
 *      Special reset sequencing for BCM56800
 *      Setup SBUS block mapping.
 */

STATIC void
soc_reset_bcm56800_a0(int unit)
{
    uint32              val, to_usec;


    val = 0;
    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (20 * MILLISECOND_USEC);

    /* Reset all blocks */
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);

    /* Bring LCPLL blocks out of reset */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val,
                      CMIC_XG_PLL0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val,
                      CMIC_XG_PLL1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val,
                      CMIC_BSAFE_CLKGEN_RST_Lf,1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    /*
     * Min 20 msec delay after LCPLL reset before checking for lock.
     */
    if ((!SAL_BOOT_PLISIM) && (!SAL_BOOT_QUICKTURN)) {
        _bradley_lcpll_lock_check(unit);
    }

    /* Bring GX4 block out of reset after 1 msec after PLLs */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX4_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    /*
     * NOTE
     * The remaining blocks will be brought out of
     * reset after Unicore reset.
     */

    /*
     * ring0 [00] : ipipe[1] -> ipipe_x[2] -> ipipe_y[3] ->
     *              epipe[4] -> epipe_x[5] -> epipe_y[6]
     * ring1 [01] : mmu[13]
     * ring2 [10] : bsafe[14]
     * ring3 [11] : gxport[0]
     *
     * 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1  0
     * 0010__0100__0000__0000__0000__0000__0000__0011
     */

    val = 0x24000003;
    WRITE_CMIC_SBUS_RING_MAPr(unit, val);
}
#endif /* BCM_BRADLEY_SUPPORT */

#ifdef BCM_TRIUMPH_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56624_a0
 * Purpose:
 *      Special reset sequencing for BCM56624
 *      Setup SBUS block mapping.
 */

STATIC void
soc_reset_bcm56624_a0(int unit)
{
    soc_control_t *soc = SOC_CONTROL(unit);
    soc_tcam_info_t *tcam_info;
    uint32 val, to_usec;

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);


    /* bring the blocks out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG_PLL0_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG_PLL1_RST_Lf,
                      1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX8_0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX8_1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX8_2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX9_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XGP0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XGP1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XGP2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XGP3_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XP0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XP1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XP2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XP3_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_SP_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    if (!soc_feature(unit, soc_feature_ignore_cmic_xgxs_pll_status) &&
        !SAL_BOOT_PLISIM && !SAL_BOOT_QUICKTURN) {
        /* LCPLL 0 is for Hyperlite and serdes x8/x9 */
        READ_CMIC_XGXS0_PLL_STATUSr(unit, &val);
        if (!soc_reg_field_get(unit, CMIC_XGXS0_PLL_STATUSr, val,
                               CMIC_XGPLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 0 not locked on unit %d "
                                  "status = 0x%08x\n"), unit, val));
        }

        /* LCPLL 1 is for Unicore */
        READ_CMIC_XGXS1_PLL_STATUSr(unit, &val);
        if (!soc_reg_field_get(unit, CMIC_XGXS1_PLL_STATUSr, val,
                               CMIC_XGPLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 1 not locked on unit %d "
                                  "status = 0x%08x\n"), unit, val));
        }
    }

    if (soc_feature(unit, soc_feature_esm_support)) {
        soc_triumph_esm_init_read_config(unit);
        tcam_info = soc->tcam_info;

        /* Assert reset pin of external TCAM, esm_init will de-assert it */
        READ_CMIC_SOFT_RESET_REGr(unit, &val);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, EXT_TCAM_RSTf, 1);
        WRITE_CMIC_SOFT_RESET_REGr(unit, val);

        /* Program PLL for DDR */
        READ_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_1r, &val,
                          EN_CMLBUF1f, 1);
        WRITE_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_1r(unit, val);

        READ_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r, &val,
                          BYPASS_SDMODf, 1);
        WRITE_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, val);

        if (tcam_info != NULL) {
            soc_triumph_esm_init_set_sram_freq(unit, tcam_info->sram_freq);
        }

        /* Program PLL for TCAM */
        READ_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_1r, &val,
                          EN_CMLBUF1f, 1);
        WRITE_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_1r(unit, val);

        READ_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r, &val,
                          BYPASS_SDMODf, 1);
        WRITE_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, val);

        if (tcam_info != NULL) {
            soc_triumph_esm_init_set_tcam_freq(unit, tcam_info->tcam_freq);
        }
    }

    /*
     * Triumph
     * ring0 [00] : IPIPE[10] -> EPIPE[11]
     * ring1 [01] : MMU[12]
     * ring2 [10] : OTPC[13] -> ESM[14]
     * ring3 [11] : XGPORT0[2] -> XGPORT1[3] -> XGPORT2[4] -> XGPORT3[5] ->
     *              GXPORT0[6] -> GXPORT1[7] -> GXPORT2[8] -> GXPORT3[9] ->
     *              SPORT[1]
     *
     * 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1 0
     * 0010__1001__0000__1111__1111__1111__1111__11XX
     */

    val = 0x290ffffe;
    WRITE_CMIC_SBUS_RING_MAPr(unit, val);

    val = 0x7d0;
    WRITE_CMIC_SBUS_TIMEOUTr(unit, val);

    /* Bring IP, EP, and MMU blocks out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    if (soc_feature(unit, soc_feature_esm_support)) {
        /* Bring PVT out of reset */
        READ_CMIC_SOFT_RESET_REGr(unit, &val);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_PVT_RST_Lf,
                          1);
        WRITE_CMIC_SOFT_RESET_REGr(unit, val);

        /* Bring DDR PLL out of reset */
        READ_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r, &val,
                          ARESETf, 0);
        WRITE_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, val);
        sal_usleep(to_usec);
        soc_reg_field_set(unit, CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r, &val,
                          DRESETf, 0);
        WRITE_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, val);
        sal_usleep(to_usec);
        if (soc->tcam_info != NULL &&
            (!SAL_BOOT_QUICKTURN && !SAL_BOOT_PLISIM && !SAL_BOOT_BCMSIM)) {
            /* Get DDR PLL lock status (bit 1) */
            READ_CMIC_MISC_STATUSr(unit, &val);
            if (!(val & 0x2)) {
                LOG_WARN(BSL_LS_SOC_COMMON,
                         (BSL_META_U(unit,
                                     "%s: DDR PLL not locked.\n"),
                                     FUNCTION_NAME()));
            }
        }

        /* Bring TCAM PLL out of reset */
        READ_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r, &val,
                          ARESETf, 0);
        WRITE_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, val);
        sal_usleep(to_usec);
        soc_reg_field_set(unit, CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r, &val,
                          DRESETf, 0);
        WRITE_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, val);
        sal_usleep(to_usec);
        if (soc->tcam_info != NULL &&
            (!SAL_BOOT_QUICKTURN && !SAL_BOOT_PLISIM && !SAL_BOOT_BCMSIM)) {
            /* Get TCAM PLL lock status (bit 2) */
            READ_CMIC_MISC_STATUSr(unit, &val);
            if (!(val & 0x4)) {
                LOG_WARN(BSL_LS_SOC_COMMON,
                         (BSL_META_U(unit,
                                     "%s: TCAM PLL not locked.\n"),
                                     FUNCTION_NAME()));
            }
        }

        /* Bring ESM block out of reset */
        READ_CMIC_SOFT_RESET_REGr(unit, &val);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_ESM_RST_Lf,
                          1);
        WRITE_CMIC_SOFT_RESET_REGr(unit, val);
        sal_usleep(to_usec);
    }
}
#endif  /* BCM_TRIUMPH_SUPPORT */

#ifdef BCM_TRIUMPH2_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56634_a0
 * Purpose:
 *      Special reset sequencing for BCM56634
 *      Setup SBUS block mapping.
 */

STATIC void
soc_reset_bcm56634_a0(int unit)
{
    soc_control_t *soc = SOC_CONTROL(unit);
    soc_tcam_info_t *tcam_info;
    uint32 val, to_usec, val2 = 0;

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);


    /* bring the blocks out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG_PLL0_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG_PLL1_RST_Lf,
                      1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX8_0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX8_1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX8_2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GX9_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ3_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ4_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ5_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XP0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XP1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XP2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XP3_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_SP_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &val2, 
                      XQ0_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &val2, 
                      XQ1_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &val2, 
                      XQ2_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &val2, 
                      XQ3_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &val2, 
                      XQ4_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &val2, 
                      XQ5_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &val2, NS_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REG_2r(unit, val2);
    sal_usleep(to_usec);

    if (soc_feature(unit, soc_feature_esm_support) || SOC_IS_TRIUMPH2(unit)) {
        soc_triumph_esm_init_read_config(unit);
    }
    tcam_info = soc->tcam_info;

    if (tcam_info != NULL) {
        /* Assert reset pin of external TCAM, esm_init will de-assert it */
        READ_CMIC_SOFT_RESET_REGr(unit, &val);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, EXT_TCAM_RSTf, 1);
        WRITE_CMIC_SOFT_RESET_REGr(unit, val);

        /* Program PLL for DDR */
        READ_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_1r, &val,
                          EN_CMLBUF1f, 1);
        WRITE_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_1r(unit, val);

        READ_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r, &val,
                          BYPASS_SDMODf, 1);
        WRITE_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, val);

        soc_triumph_esm_init_set_sram_freq(unit, tcam_info->sram_freq);

        /* Program PLL for TCAM */
        READ_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_1r, &val,
                          EN_CMLBUF1f, 1);
        WRITE_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_1r(unit, val);

        READ_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r, &val,
                          BYPASS_SDMODf, 1);
        WRITE_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, val);

        soc_triumph_esm_init_set_tcam_freq(unit, tcam_info->tcam_freq);
    }


    /*
     * Triumph2
     *
     * map_0 includes blocks 0-7
     * map_1 includes blocks 8-15
     * map_2 includes blocks 16-23
     * map_3 includes blocks 24-31
     *
     */

    val = 0x33022140;
    WRITE_CMIC_SBUS_RING_MAP_0r(unit, val);

    val = 0x00033333;
    WRITE_CMIC_SBUS_RING_MAP_1r(unit, val);

    val = 0x00333333;
    WRITE_CMIC_SBUS_RING_MAP_2r(unit, val);

    val = 0x0;
    WRITE_CMIC_SBUS_RING_MAP_3r(unit, val);

    val = 0x2000;
    WRITE_CMIC_SBUS_TIMEOUTr(unit, val);

    /* Bring IP, EP, and MMU blocks out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    if (soc_feature(unit, soc_feature_esm_support)) {
        /* Bring PVT out of reset */
        READ_CMIC_SOFT_RESET_REGr(unit, &val);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_PVT_RST_Lf,
                          1);
        WRITE_CMIC_SOFT_RESET_REGr(unit, val);

        /* Bring DDR PLL out of reset */
        READ_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r, &val,
                          ARESETf, 0);
        WRITE_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, val);
        sal_usleep(to_usec);
        soc_reg_field_set(unit, CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r, &val,
                          DRESETf, 0);
        WRITE_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, val);
        sal_usleep(to_usec);
        if (soc->tcam_info != NULL &&
            (!SAL_BOOT_QUICKTURN && !SAL_BOOT_PLISIM && !SAL_BOOT_BCMSIM)) {
            /* Get DDR PLL lock status (bit 1) */
            READ_CMIC_MISC_STATUSr(unit, &val);
            if (!(val & 0x2)) {
                LOG_WARN(BSL_LS_SOC_COMMON,
                         (BSL_META_U(unit,
                                     "%s: DDR PLL not locked.\n"),
                                     FUNCTION_NAME()));
            }
        }

        /* Bring TCAM PLL out of reset */
        READ_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r, &val,
                          ARESETf, 0);
        WRITE_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, val);
        sal_usleep(to_usec);
        soc_reg_field_set(unit, CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r, &val,
                          DRESETf, 0);
        WRITE_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, val);
        sal_usleep(to_usec);
        if (soc->tcam_info != NULL &&
            (!SAL_BOOT_QUICKTURN && !SAL_BOOT_PLISIM && !SAL_BOOT_BCMSIM)) {
            /* Get TCAM PLL lock status (bit 2) */
            READ_CMIC_MISC_STATUSr(unit, &val);
            if (!(val & 0x4)) {
                LOG_WARN(BSL_LS_SOC_COMMON,
                         (BSL_META_U(unit,
                                     "%s: TCAM PLL not locked.\n"),
                                     FUNCTION_NAME()));
            }
        }

        /* Bring ESM block out of reset */
        READ_CMIC_SOFT_RESET_REGr(unit, &val);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_ESM_RST_Lf,
                          1);
        WRITE_CMIC_SOFT_RESET_REGr(unit, val);
        sal_usleep(to_usec);
    }

    if (soc_feature(unit, soc_feature_gmii_clkout)) {
        val2 = 0;
        /* select GMII 25MHz clock output */
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &val2, 
                          CLK_RECOVERY_PRI_SELECTf, 1);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &val2, 
                          CLK_RECOVERY_BKUP_SELECTf, 1);
        WRITE_CMIC_MISC_CONTROLr(unit, val2);

        /* Select primary and secondary ports */
        val2 = 0;
        soc_reg_field_set(unit, EGR_L1_CLK_RECOVERY_CTRLr, &val2, 
                          PRI_PORT_SELf, soc_property_get(unit, 
                          "L1_primary_clk_recovery_port", 0x0));
        soc_reg_field_set(unit, EGR_L1_CLK_RECOVERY_CTRLr, &val2, 
                          BKUP_PORT_SELf, soc_property_get(unit, 
                          "L1_backup_clk_recovery_port", 0x0));
        if (WRITE_EGR_L1_CLK_RECOVERY_CTRLr(unit, val2) != SOC_E_NONE) {
            LOG_CLI((BSL_META_U(unit,
                                "Selecting L1 clock recovery ports failed !!\n")));
        }
    }
}
#endif  /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_ENDURO_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56334_a0
 * Purpose:
 *      Special reset sequencing for BCM56334
 *      Setup SBUS block mapping.
 */

STATIC void
soc_reset_bcm56334_a0(int unit)
{
    uint32 val, to_usec;
    uint16              dev_id;
    uint8               rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);


    if (soc_feature(unit, soc_feature_delay_core_pll_lock)) {
        /* put Configuring core-clk to 100MHz */
        READ_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_1r, &val,
                          TEST_SELf, 5);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_1r, &val,
                          TEST_ENABLEf, 1);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_1r(unit, val);
        READ_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_2r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_2r, &val,
                          M1DIVf, 10);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_2r(unit, val);
        READ_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 40);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
        sal_usleep(to_usec);
    }

    /* bring the blocks out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG1_PLL_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG0_PLL_RST_Lf,
                      1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                            CMIC_GX8_SERDES_0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                            CMIC_GX8_SERDES_1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                            CMIC_GX8_SERDES_2_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XQP0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XQP1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XQP2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XQP3_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ0_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ1_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ2_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ3_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, NS_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_OTPC_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, THERMAL_MON_RESETf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, THERMAL_MON_RESETf, 0);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    val = 0x11122200;
    WRITE_CMIC_SBUS_RING_MAP_0r(unit, val);
    val = 0x00330001;
    WRITE_CMIC_SBUS_RING_MAP_1r(unit, val);
    val = 0x0;
    WRITE_CMIC_SBUS_RING_MAP_2r(unit, val);
    WRITE_CMIC_SBUS_RING_MAP_3r(unit, val);
    WRITE_CMIC_SBUS_RING_MAP_4r(unit, val);
    WRITE_CMIC_SBUS_RING_MAP_5r(unit, val);
    WRITE_CMIC_SBUS_RING_MAP_6r(unit, val);
    WRITE_CMIC_SBUS_RING_MAP_7r(unit, val);

    val = 0x7d0;
    WRITE_CMIC_SBUS_TIMEOUTr(unit, val);

    /* Bring IP, EP, and MMU blocks out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    if (soc_feature(unit, soc_feature_delay_core_pll_lock)) {
        /* assert the active bits of cmic_soft_reset_reg then de-assert  */
        READ_CMIC_SOFT_RESET_REGr(unit, &val);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP0_RST_Lf, 0);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP1_RST_Lf, 0);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP2_RST_Lf, 0);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, 
                                &val, CMIC_XQP0_RST_Lf, 0);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, 
                                &val, CMIC_XQP1_RST_Lf, 0);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, 
                                &val, CMIC_XQP2_RST_Lf, 0);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, 
                                &val, CMIC_XQP3_RST_Lf, 0);     
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 0);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 0);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf, 0);
        WRITE_CMIC_SOFT_RESET_REGr(unit, val);
        sal_usleep(to_usec);

        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP0_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP1_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP2_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, 
                                &val, CMIC_XQP0_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, 
                                &val, CMIC_XQP1_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, 
                                &val, CMIC_XQP2_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, 
                                &val, CMIC_XQP3_RST_Lf, 1);     
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf, 1);
        WRITE_CMIC_SOFT_RESET_REGr(unit, val);
        sal_usleep(to_usec);        
    }
    
    if (soc_feature(unit, soc_feature_gmii_clkout)) {
        uint32 rval;
        static int16 _L1_port_val_map[] = {
            0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
            4, 5, 6, 7, 8, 9, 10, 11, 0, 0, 12, 12, 13, 13
        }; /* Note: 24, 25 are invalid, but we are just setting 0 in those cases */

        val = soc_property_get(unit, "L1_primary_clk_recovery_port", 0x0);
        if (val >= COUNTOF(_L1_port_val_map)) {
            return;
        }  
        READ_CMIC_MISC_CONTROLr(unit, &rval);
        if ((val <= 3) || ((val >= 8) && (val <= 15)) || 
            (val == 26) || (val == 28)) {
            soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                              L1_CLK0_RECOVERY_MUXf, _L1_port_val_map[val]);
        } else {
            soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                              L1_CLK1_RECOVERY_MUXf, _L1_port_val_map[val]);
        }
        
        val = soc_property_get(unit, "L1_backup_clk_recovery_port", 0x0);
        if (val >= COUNTOF(_L1_port_val_map)) {
            return;
        }  
        if ((val <= 3) || ((val >= 8) && (val <= 15)) || 
            (val == 26) || (val == 28)) {
            soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                              L1_CLK0_RECOVERY_MUXf, _L1_port_val_map[val]);
        } else {
            soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                              L1_CLK1_RECOVERY_MUXf, _L1_port_val_map[val]);
        }
        WRITE_CMIC_MISC_CONTROLr(unit, rval);
    }
}
#endif  /* BCM_ENDURO_SUPPORT */

#ifdef BCM_SHADOW_SUPPORT
/*
 * Function:
 *      soc_reset_bcm88732_a0
 * Purpose:
 *      Special reset sequencing for BCM88732
 *      Setup SBUS block mapping.
 */

STATIC void
soc_reset_bcm88732_a0(int unit)
{
    uint32              val, val2, to_usec;
    uint32              rval, addr;
    uint16              dev_id;
    uint8               rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    val = 0;
    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);

    /* Reset all blocks */
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);

    
    /* Set NDIV=140 and PDIV=7 (NDIV_INT<< 3 |  PDIV)*/
    addr = soc_reg_addr(unit, CMIC_XGXS0_PLL_CONTROL_3r, REG_PORT_ANY, 0);
    soc_pci_getreg(unit, addr, &rval);
    soc_reg_field_set(unit, CMIC_XGXS0_PLL_CONTROL_3r, &rval, PLLCTRL95_64f, (140 << 3 | 7));
    soc_pci_write(unit, addr, rval);

    addr = soc_reg_addr(unit, CMIC_XGXS1_PLL_CONTROL_3r, REG_PORT_ANY, 0);
    soc_pci_getreg(unit, addr, &rval);
    soc_reg_field_set(unit, CMIC_XGXS1_PLL_CONTROL_3r, &rval, PLLCTRL95_64f, (140 << 3 | 7));
    soc_pci_write(unit, addr, rval);


    /* Bring the PLLs out of reset */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, LCPLL2_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, LCPLL1_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, LCPLL0_SYS_RESET_Nf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    /* Miscellaneous other resets */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, TEMP_MON_PEAK_RESET_Nf,
                      1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    /*
     * Min 20 msec delay after LCPLL reset before checking for lock.
     */
    if (!soc_feature(unit, soc_feature_ignore_cmic_xgxs_pll_status) &&
        !SAL_BOOT_PLISIM && !SAL_BOOT_QUICKTURN) {
        uint32 val;

        READ_CMIC_XGXS0_PLL_STATUSr(unit, &val);
        if (!(soc_reg_field_get(unit, CMIC_XGXS0_PLL_STATUSr,
                                val, CMIC_XGPLL_LOCKf))) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 0 not locked on unit %d status = 0x%08x\n"),
                       unit, val));
        }

        READ_CMIC_XGXS1_PLL_STATUSr(unit, &val);
        if (!(soc_reg_field_get(unit, CMIC_XGXS1_PLL_STATUSr,
                                val, CMIC_XGPLL_LOCKf))) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 1 not locked on unit %d status = 0x%08x\n"),
                       unit, val));
        }

        READ_CMIC_XGXS2_PLL_STATUSr(unit, &val);
        if (!(soc_reg_field_get(unit, CMIC_XGXS2_PLL_STATUSr,
                                val, CMIC_XGPLL_LOCKf))) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 2 not locked on unit %d status = 0x%08x\n"),
                       unit, val));
        }

    }


    /* Bring port blocks out of reset after 1 msec after PLLs */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT0_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT1_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT2_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT3_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT4_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT5_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT0_LOGIC_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT1_LOGIC_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT2_LOGIC_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT3_LOGIC_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT4_LOGIC_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      XLPORT5_LOGIC_RESET_Nf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    val2 = 0;
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &val2, 
                      NS_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REG_2r(unit, val2);

    /*
     * Shadow - ring map programming information taken from the regs file.
     */
    val = 0x00321100;
    WRITE_CMIC_SBUS_RING_MAP_0r(unit, val);

    val = 0x10320411;
    WRITE_CMIC_SBUS_RING_MAP_1r(unit, val);

    val = 0x00000532;
    WRITE_CMIC_SBUS_RING_MAP_2r(unit, val);

    val = 0x0;
    WRITE_CMIC_SBUS_RING_MAP_3r(unit, val);

    /* Bring MACSEC and Interlaken blocks out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, MS0_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, MS1_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, IL0_LOGIC_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, IL0_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, IL1_LOGIC_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, IL1_SYS_RESET_Nf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    /* Bring PE, PD, and MMU blocks out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, PE0_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, PE1_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, PE0_LOGIC_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, PD_SYS_RESET_Nf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, MMU0_SYS_RESET_Nf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);

    /* Bring temperature monitor out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                      TEMP_MON_PEAK_RESET_Nf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);
}
#endif  /* BCM_SHADOW_SUPPORT */


#ifdef BCM_HURRICANE_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56142_a0
 * Purpose:
 *      Special reset sequencing for BCM56142
 *      Setup SBUS block mapping.
 */

STATIC void
soc_reset_bcm56142_a0(int unit)
{
    uint32 val, to_usec;
    uint16              dev_id;
    uint8               rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);


    /* bring the blocks out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG0_PLL_RST_Lf,
                      1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                            CMIC_QSGMII2X_SERDES_0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                            CMIC_QSGMII2X_SERDES_1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, 
                            CMIC_QSGMII2X_SERDES_2_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_GP2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XQP0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XQP1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XQP2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XQP3_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ0_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ1_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ2_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XQ3_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, THERMAL_MON_RESETf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, THERMAL_MON_RESETf, 0);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

   
   val = 0x11122200;
   WRITE_CMIC_SBUS_RING_MAP_0r(unit, val);
   val = 0x00330001;
   WRITE_CMIC_SBUS_RING_MAP_1r(unit, val);
   val = 0x0;
   WRITE_CMIC_SBUS_RING_MAP_2r(unit, val);
   WRITE_CMIC_SBUS_RING_MAP_3r(unit, val);
   WRITE_CMIC_SBUS_RING_MAP_4r(unit, val);
   WRITE_CMIC_SBUS_RING_MAP_5r(unit, val);
   WRITE_CMIC_SBUS_RING_MAP_6r(unit, val);
   WRITE_CMIC_SBUS_RING_MAP_7r(unit, val);

    val = 0x7d0;
    WRITE_CMIC_SBUS_TIMEOUTr(unit, val);

    /* Bring IP, EP, and MMU blocks out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &val);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, NS_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

}
#endif  /* BCM_HURRICANE_SUPPORT */

#ifdef BCM_HURRICANE2_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56150_a0
 * Purpose:
 *      Special reset sequencing for BCM56150
 */

STATIC int
soc_reset_bcm56150_a0(int unit)
{
    uint32 rval, to_usec;
    uint16 dev_id;
    uint8 rev_id;
    soc_reg_t reg;
    uint32 addr;
    int index;
    static const soc_reg_t pll_ctrl1_reg[] = {
        CMIC_XGXS0_PLL_CONTROL_1r, CMIC_XGXS1_PLL_CONTROL_1r,
        CMIC_XGXS2_PLL_CONTROL_1r, CMIC_XGXS3_PLL_CONTROL_1r
    };
    static const soc_reg_t pll_ctrl2_reg[] = {
        CMIC_XGXS0_PLL_CONTROL_2r, CMIC_XGXS1_PLL_CONTROL_2r,
        CMIC_XGXS2_PLL_CONTROL_2r, CMIC_XGXS3_PLL_CONTROL_2r
    };
    static const soc_reg_t pll_ctrl3_reg[] = {
        CMIC_XGXS0_PLL_CONTROL_3r, CMIC_XGXS1_PLL_CONTROL_3r,
        CMIC_XGXS2_PLL_CONTROL_3r, CMIC_XGXS3_PLL_CONTROL_3r
    };

    soc_cm_get_id(unit, &dev_id, &rev_id);

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);
 


    /* Use 156.25Mhz reference clock for LCPLL */
    if (soc_property_get(unit, spn_XGXS_LCPLL_XTAL_REFCLK, 0)) {
        for (index = 0; index < 4; index++) {
            reg = pll_ctrl1_reg[index];
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            soc_pci_getreg(unit, addr, &rval);
            soc_reg_field_set(unit, reg, &rval, CH0_MDIVf, 20);
            soc_reg_field_set(unit, reg, &rval, CH3_MDIVf, 25);
            soc_reg_field_set(unit, reg, &rval, CH4_MDIVf, 125);
            soc_reg_field_set(unit, reg, &rval, CH5_MDIVf, 25);
            soc_pci_write(unit, addr, rval);

            reg = pll_ctrl3_reg[index];
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            soc_pci_getreg(unit, addr, &rval);
            soc_reg_field_set(unit, reg, &rval, NDIV_INTf, 40);
            soc_pci_write(unit, addr, rval);

            reg = pll_ctrl2_reg[index];
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            soc_pci_getreg(unit, addr, &rval);
            soc_reg_field_set(unit, reg, &rval, PDIVf, 2);
            soc_pci_write(unit, addr, rval);
        }

        READ_CMIC_MISC_CONTROLr(unit, &rval);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL0_SW_OVWRf, 1);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL1_SW_OVWRf, 1);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL2_SW_OVWRf, 1);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL3_SW_OVWRf, 1);
        WRITE_CMIC_MISC_CONTROLr(unit, rval);
    }


    /*
     * SBUS ring and block number:
     * Ring0: cmic -> ip-ep -> cmic
     * Ring1: cmic -> xqport0 -> xqport1-> xqport2 -> xqport3-> cmic
     * Ring2: cmic -> gport0 -> gport1 -> gport2 -> cmic
     * Ring3: cmic -> otpc -> mmu -> cmic
     * Ring 4,5,6,7 unused
     */
    WRITE_CMIC_SBUS_RING_MAP_0_7r(unit, 0x11122200); /* block 7  - 0 */
    WRITE_CMIC_SBUS_RING_MAP_8_15r(unit, 0x00430000); /* block 15 - 8 */
    WRITE_CMIC_SBUS_RING_MAP_16_23r(unit, 0x00005004); /* block 23 - 16 */
    WRITE_CMIC_SBUS_RING_MAP_24_31r(unit, 0x00000000); /* block 31 - 24 */

    /* Bring LCPLL out of reset */
    
    sal_usleep(to_usec);


    /* KT-1367: Timesync PLL 250MHz TS_CLK from 25MHz refclk */ 
    /* NDIV= 100(0x64) MDIV=10(0x1)            */   

    /* Configure TS PLL */
    
    /* 250Mhz TS PLL implies 4ns resolution */
    SOC_TIMESYNC_PLL_CLOCK_NS(unit) = (1/250 * 1000); /* clock period in nanoseconds */

    /* De-assert LCPLL's post reset */

    /*
     * Bring port blocks out of reset
     */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XLP0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XLP1_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_QSGMII2X2_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_QSGMII2X1_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_QSGMII2X0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_QSGMII2X2_FIFO_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_QSGMII2X1_FIFO_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_QSGMII2X0_FIFO_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_GP2_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_GP1_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_GP0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_QGPHY_RST_Lf, 0xf); 
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, rval));
    sal_usleep(to_usec);
    
    /* Bring network sync out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_TS_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_SPARE_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, rval));

    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_2r(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_TS_PLL_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));
    
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_TS_PLL_POST_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));

    sal_usleep(to_usec);
    
    SOC_IF_ERROR_RETURN(WRITE_CMIC_SBUS_TIMEOUTr(unit, 0x7d0));

    /* Bring IP, EP, and MMU blocks out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_EP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_IP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MMU_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, rval));
    sal_usleep(to_usec);

    /* reset PVTMON */
    SOC_IF_ERROR_RETURN(READ_TOP_PVTMON_CTRL_0r(unit, &rval));
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_0r, &rval, FUNC_MODE_SELf, 0);
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_0r, &rval, MEASUREMENT_CALLIBRATIONf, 5);
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_0r, &rval, BG_ADJf, 3);
    SOC_IF_ERROR_RETURN(WRITE_TOP_PVTMON_CTRL_0r(unit, rval));

    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_2r(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval,
                      TOP_TEMP_MON_PEAK_RST_Lf, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval,
                      TOP_TEMP_MON_PEAK_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));

    SOC_IF_ERROR_RETURN(READ_TOP_PVTMON_CTRL_1r(unit, &rval));
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_1r, &rval, PVTMON_SELECTf, 0);
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_1r, &rval, PVTMON_RESET_Nf, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_PVTMON_CTRL_1r(unit, rval));
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_1r, &rval, PVTMON_RESET_Nf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_PVTMON_CTRL_1r(unit, rval));

    /* LCPLL CML optimization */
    if ((dev_id == BCM56150_DEVICE_ID) || (dev_id == BCM56152_DEVICE_ID) || \
       (dev_id == BCM53346_DEVICE_ID) || (dev_id == BCM53344_DEVICE_ID) || \
       (dev_id == BCM53343_DEVICE_ID) || (dev_id == BCM53342_DEVICE_ID) || \
       (dev_id == BCM53333_DEVICE_ID) || (dev_id == BCM53334_DEVICE_ID)) {
        SOC_IF_ERROR_RETURN(READ_TOP_XG_PLL0_CTRL_3r(unit, &rval));
        soc_reg_field_set(unit, TOP_XG_PLL0_CTRL_3r, &rval, CML_2ED_OUT_ENf, 0);
        SOC_IF_ERROR_RETURN(WRITE_TOP_XG_PLL0_CTRL_3r(unit, rval));

        SOC_IF_ERROR_RETURN(READ_TOP_XG_PLL1_CTRL_3r(unit, &rval));
        soc_reg_field_set(unit, TOP_XG_PLL1_CTRL_3r, &rval, CML_2ED_OUT_ENf, 0);
        SOC_IF_ERROR_RETURN(WRITE_TOP_XG_PLL1_CTRL_3r(unit, rval));

        SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL0_CTRL_3r(unit, &rval));
        soc_reg_field_set(unit, TOP_BS_PLL0_CTRL_3r, &rval, CML_2ED_OUT_ENf, 0);
        SOC_IF_ERROR_RETURN(WRITE_TOP_BS_PLL0_CTRL_3r(unit, rval));

        SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL1_CTRL_3r(unit, &rval));
        soc_reg_field_set(unit, TOP_BS_PLL1_CTRL_3r, &rval, CML_2ED_OUT_ENf, 0);
        SOC_IF_ERROR_RETURN(WRITE_TOP_BS_PLL1_CTRL_3r(unit, rval));

        SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_1r(unit, &rval));
        soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, CMIC_TO_XG_PLL0_SW_OVWRf, 1);
        soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, CMIC_TO_XG_PLL1_SW_OVWRf, 1);
        soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, CMIC_TO_BS_PLL0_SW_OVWRf, 1);
        soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, CMIC_TO_BS_PLL1_SW_OVWRf, 1);
        SOC_IF_ERROR_RETURN(WRITE_TOP_MISC_CONTROL_1r(unit, rval));
    }

    if ((dev_id == BCM56151_DEVICE_ID) || (dev_id == BCM53393_DEVICE_ID) || \
       (dev_id == BCM53394_DEVICE_ID) || (dev_id == BCM53347_DEVICE_ID)) {
        SOC_IF_ERROR_RETURN(READ_TOP_XG_PLL1_CTRL_3r(unit, &rval));
        soc_reg_field_set(unit, TOP_XG_PLL1_CTRL_3r, &rval, CML_2ED_OUT_ENf, 0);
        SOC_IF_ERROR_RETURN(WRITE_TOP_XG_PLL1_CTRL_3r(unit, rval));

        SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL0_CTRL_3r(unit, &rval));
        soc_reg_field_set(unit, TOP_BS_PLL0_CTRL_3r, &rval, CML_2ED_OUT_ENf, 0);
        SOC_IF_ERROR_RETURN(WRITE_TOP_BS_PLL0_CTRL_3r(unit, rval));

        SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL1_CTRL_3r(unit, &rval));
        soc_reg_field_set(unit, TOP_BS_PLL1_CTRL_3r, &rval, CML_2ED_OUT_ENf, 0);
        SOC_IF_ERROR_RETURN(WRITE_TOP_BS_PLL1_CTRL_3r(unit, rval));

        SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_1r(unit, &rval));
        soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, CMIC_TO_XG_PLL1_SW_OVWRf, 1);
        soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, CMIC_TO_BS_PLL0_SW_OVWRf, 1);
        soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, CMIC_TO_BS_PLL1_SW_OVWRf, 1);
        SOC_IF_ERROR_RETURN(WRITE_TOP_MISC_CONTROL_1r(unit, rval));
    }

    /* For 53343, power down unused qsgmii */
    if (dev_id == BCM53343_DEVICE_ID) {
        SOC_IF_ERROR_RETURN(READ_TOP_QSGMII2X_CTRLr(unit, &rval));
        soc_reg_field_set(unit, TOP_QSGMII2X_CTRLr, &rval, IDDQf, 0x7);
        SOC_IF_ERROR_RETURN(WRITE_TOP_QSGMII2X_CTRLr(unit, rval));
    }
    /* For 53342, power down unused qsgmii and unused qgphy */
    if (dev_id == BCM53342_DEVICE_ID) {
        SOC_IF_ERROR_RETURN(READ_TOP_QGPHY_CTRL_0r(unit, &rval));
        soc_reg_field_set(unit, TOP_QGPHY_CTRL_0r, &rval, IDDQ_BIASf, 0xc);
        soc_reg_field_set(unit, TOP_QGPHY_CTRL_0r, &rval, EXT_PWRDOWNf, 0xff00);
        SOC_IF_ERROR_RETURN(WRITE_TOP_QGPHY_CTRL_0r(unit, rval));

        SOC_IF_ERROR_RETURN(READ_TOP_QSGMII2X_CTRLr(unit, &rval));
        soc_reg_field_set(unit, TOP_QSGMII2X_CTRLr, &rval, IDDQf, 0x3);
        SOC_IF_ERROR_RETURN(WRITE_TOP_QSGMII2X_CTRLr(unit, rval));
    }

    /* For 53343, power down unused qsgmii */
    if (dev_id == BCM53343_DEVICE_ID) {
        SOC_IF_ERROR_RETURN(READ_TOP_QSGMII2X_CTRLr(unit, &rval));
        soc_reg_field_set(unit, TOP_QSGMII2X_CTRLr, &rval, IDDQf, 0x3);
        SOC_IF_ERROR_RETURN(WRITE_TOP_QSGMII2X_CTRLr(unit, rval));
    }
    /* For 53342, power down unused qsgmii and unused qgphy */
    if (dev_id == BCM53342_DEVICE_ID) {
        SOC_IF_ERROR_RETURN(READ_TOP_QGPHY_CTRL_0r(unit, &rval));
        soc_reg_field_set(unit, TOP_QGPHY_CTRL_0r, &rval, IDDQ_BIASf, 0xc);
        soc_reg_field_set(unit, TOP_QGPHY_CTRL_0r, &rval, EXT_PWRDOWNf, 0xff00);
        SOC_IF_ERROR_RETURN(WRITE_TOP_QGPHY_CTRL_0r(unit, rval));

        SOC_IF_ERROR_RETURN(READ_TOP_QSGMII2X_CTRLr(unit, &rval));
        soc_reg_field_set(unit, TOP_QSGMII2X_CTRLr, &rval, IDDQf, 0x3);
        SOC_IF_ERROR_RETURN(WRITE_TOP_QSGMII2X_CTRLr(unit, rval));
    }

    return SOC_E_NONE;
}

#endif  /* BCM_HURRICANE2_SUPPORT */

#ifdef BCM_SCORPION_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56820_a0
 * Purpose:
 *      Special reset sequencing for BCM56820
 *      Setup SBUS block mapping.
 */

STATIC void
soc_reset_bcm56820_a0(int unit)
{
    uint32              val, to_usec;
    uint16              dev_id;
    uint8               rev_id;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    val = 0;
    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);

    /* Reset all blocks */
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);


    if (soc_feature(unit, soc_feature_delay_core_pll_lock) &&
        soc_property_get(unit, "delay_core_pll_lock", 0)) {
        /* Core clock lock */
        READ_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 80);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 76);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 72);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 68);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 66);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 64);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);

        READ_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_0r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_0r, &val,
                          ICPXf, 0xe);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_0r(unit, val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_0r, &val,
                          KVCO_XSf, 0x4);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_0r(unit, val);

        READ_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_1r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_1r, &val,
                          VCO_RNGf, 0);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_1r(unit, val);

        READ_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 60);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 56);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 52);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 48);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r, &val,
                          NDIV_INTf, 44);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);

        READ_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_2r(unit, &val);
        soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_2r, &val,
                          M1DIVf, 5);
        WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_2r(unit, val);

        if (dev_id != BCM56820_DEVICE_ID) {
            READ_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, &val);
            soc_reg_field_set(unit, CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r,
                              &val, NDIV_INTf, 45);
            WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
            if ((dev_id == BCM56822_DEVICE_ID) ||
                (dev_id == BCM56823_DEVICE_ID) ||
                (dev_id == BCM56725_DEVICE_ID)) {
                soc_reg_field_set(unit,
                                  CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r,
                                  &val, NDIV_INTf, 49);
                WRITE_CMIC_CORE_PLL0_CTRL_STATUS_REGISTER_3r(unit, val);
            }
        }
        sal_usleep(to_usec);
        if (!SAL_BOOT_QUICKTURN && !SAL_BOOT_PLISIM && !SAL_BOOT_BCMSIM) {
            /* Get Core PLL lock status (bit 0) */
            READ_CMIC_MISC_STATUSr(unit, &val);
            if (!(val & 0x1)) {
                LOG_WARN(BSL_LS_SOC_COMMON,
                         (BSL_META_U(unit,
                                     "%s: Core PLL not locked.\n"),
                                     FUNCTION_NAME()));
            }
        }
    }

    /* Bring IP2 clock doubler out of reset */
    READ_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, &val);
    soc_reg_field_set(unit, CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r, &val,
                      ARESETf, 0);
    WRITE_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, val);
    sal_usleep(to_usec);
    soc_reg_field_set(unit, CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r, &val,
                      DRESETf, 0);
    WRITE_CMIC_CORE_PLL1_CTRL_STATUS_REGISTER_2r(unit, val);
    sal_usleep(to_usec);
    if (!SAL_BOOT_QUICKTURN && !SAL_BOOT_PLISIM && !SAL_BOOT_BCMSIM) {
        /* Get DDR PLL lock status (bit 1) */
        READ_CMIC_MISC_STATUSr(unit, &val);
        if (!(val & 0x2)) {
            LOG_WARN(BSL_LS_SOC_COMMON,
                     (BSL_META_U(unit,
                                 "%s: IP2 clock doubler not locked.\n"),
                      FUNCTION_NAME()));
        }
    }

    /* Bring MMU0 clock doubler out of reset */
    READ_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, &val);
    soc_reg_field_set(unit, CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r, &val,
                      ARESETf, 0);
    WRITE_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, val);
    sal_usleep(to_usec);
    soc_reg_field_set(unit, CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r, &val,
                      DRESETf, 0);
    WRITE_CMIC_CORE_PLL2_CTRL_STATUS_REGISTER_2r(unit, val);
    sal_usleep(to_usec);
    if (!SAL_BOOT_QUICKTURN && !SAL_BOOT_PLISIM && !SAL_BOOT_BCMSIM) {
        /* Get DDR PLL lock status (bit 2) */
        READ_CMIC_MISC_STATUSr(unit, &val);
        if (!(val & 0x4)) {
            LOG_WARN(BSL_LS_SOC_COMMON,
                     (BSL_META_U(unit,
                                 "%s: MMU0 clock doubler not locked.\n"),
                      FUNCTION_NAME()));
        }
    }

    /* Bring MMU1 clock doubler out of reset */
    READ_CMIC_CORE_PLL3_CTRL_STATUS_REGISTER_2r(unit, &val);
    soc_reg_field_set(unit, CMIC_CORE_PLL3_CTRL_STATUS_REGISTER_2r, &val,
                      ARESETf, 0);
    WRITE_CMIC_CORE_PLL3_CTRL_STATUS_REGISTER_2r(unit, val);
    sal_usleep(to_usec);
    soc_reg_field_set(unit, CMIC_CORE_PLL3_CTRL_STATUS_REGISTER_2r, &val,
                      DRESETf, 0);
    WRITE_CMIC_CORE_PLL3_CTRL_STATUS_REGISTER_2r(unit, val);
    sal_usleep(to_usec);
    if (!SAL_BOOT_QUICKTURN && !SAL_BOOT_PLISIM && !SAL_BOOT_BCMSIM) {
        /* Get DDR PLL lock status (bit 3) */
        READ_CMIC_MISC_STATUSr(unit, &val);
        if (!(val & 0x8)) {
            LOG_WARN(BSL_LS_SOC_COMMON,
                     (BSL_META_U(unit,
                                 "%s: MMU1 clock doubler not locked.\n"),
                      FUNCTION_NAME()));
        }
    }

    /* bring the blocks out of reset */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, XG_PLL2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG_PLL1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XG_PLL0_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    /*
     * Min 20 msec delay after LCPLL reset before checking for lock.
     */
    if (!soc_feature(unit, soc_feature_ignore_cmic_xgxs_pll_status) &&
        !SAL_BOOT_PLISIM && !SAL_BOOT_QUICKTURN) {
        uint32 val;

        READ_CMIC_XGXS0_PLL_STATUSr(unit, &val);
        if (!(soc_reg_field_get(unit, CMIC_XGXS0_PLL_STATUSr,
                                val, CMIC_XGPLL_LOCKf))) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 0 not locked on unit %d status = 0x%08x\n"),
                       unit, val));
        }

        READ_CMIC_XGXS1_PLL_STATUSr(unit, &val);
        if (!(soc_reg_field_get(unit, CMIC_XGXS1_PLL_STATUSr,
                                val, CMIC_XGPLL_LOCKf))) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 1 not locked on unit %d status = 0x%08x\n"),
                       unit, val));
        }

        READ_CMIC_XGXS2_PLL_STATUSr(unit, &val);
        if (!(soc_reg_field_get(unit, CMIC_XGXS2_PLL_STATUSr,
                                val, CMIC_XGPLL_LOCKf))) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 2 not locked on unit %d status = 0x%08x\n"),
                       unit, val));
        }
    }

    /* Bring blocks out of reset after 1 msec after PLLs */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, QGP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XGP0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XGP1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XGP2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XGP3_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, val);
    sal_usleep(to_usec);

    /*
     * NOTE
     * The remaining blocks will be brought out of
     * reset after Unicore reset.
     */

    /*
     * Scorpion
     * ring0 [00] : IPIPE[1] -> EPIPE[4]
     * ring1 [01] : MMU[13]
     * ring2 [10] : [14]
     * ring3 [11] : gxport[0] -> qgport[2]
     *
     * 15 14 13 12 11 10 9  8  7  6  5  4  3  2  1 0
     * XX10__01XX__XXXX__XXXX__XXXX__XX00__XX11__0011
     */

    val = 0x24000033;
    WRITE_CMIC_SBUS_RING_MAPr(unit, val);
}
#endif  /* BCM_SCORPION_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56840_a0
 * Purpose:
 *      Special reset sequencing for BCM56840
 *      Setup SBUS block mapping.
 */
STATIC int
soc_reset_bcm56840_a0(int unit)
{
    uint32 rval, to_usec;
    uint16 dev_id, drv_dev_id;
    uint8 rev_id, drv_rev_id;
    soc_reg_t reg;
    uint32 addr;
    int index;
    static const soc_reg_t pll_ctrl1_reg[] = {
        CMIC_XGXS0_PLL_CONTROL_1r, CMIC_XGXS1_PLL_CONTROL_1r,
        CMIC_XGXS2_PLL_CONTROL_1r, CMIC_XGXS3_PLL_CONTROL_1r
    };
    static const soc_reg_t pll_ctrl2_reg[] = {
        CMIC_XGXS0_PLL_CONTROL_2r, CMIC_XGXS1_PLL_CONTROL_2r,
        CMIC_XGXS2_PLL_CONTROL_2r, CMIC_XGXS3_PLL_CONTROL_2r
    };
    static const soc_reg_t pll_ctrl3_reg[] = {
        CMIC_XGXS0_PLL_CONTROL_3r, CMIC_XGXS1_PLL_CONTROL_3r,
        CMIC_XGXS2_PLL_CONTROL_3r, CMIC_XGXS3_PLL_CONTROL_3r
    };
    static const soc_reg_t pll_ctrl4_reg[] = {
        CMIC_XGXS0_PLL_CONTROL_4r, CMIC_XGXS1_PLL_CONTROL_4r,
        CMIC_XGXS2_PLL_CONTROL_4r, CMIC_XGXS3_PLL_CONTROL_4r
    };
    static const soc_reg_t pll_status_reg[] = {
        CMIC_XGXS0_PLL_STATUSr, CMIC_XGXS1_PLL_STATUSr,
        CMIC_XGXS2_PLL_STATUSr, CMIC_XGXS3_PLL_STATUSr
    };

    soc_cm_get_id(unit, &dev_id, &rev_id);
    soc_cm_get_id_driver(dev_id, rev_id, &drv_dev_id, &drv_rev_id);

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);


    /* Use 156.25Mhz reference clock for LCPLL */
    if (soc_property_get(unit, spn_XGXS_LCPLL_XTAL_REFCLK, 0)) {
        for (index = 0; index < 4; index++) {
            reg = pll_ctrl1_reg[index];
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            soc_pci_getreg(unit, addr, &rval);
            soc_reg_field_set(unit, reg, &rval, CH0_MDIVf, 20);
            soc_reg_field_set(unit, reg, &rval, CH3_MDIVf, 25);
            soc_reg_field_set(unit, reg, &rval, CH4_MDIVf, 125);
            soc_reg_field_set(unit, reg, &rval, CH5_MDIVf, 25);
            soc_pci_write(unit, addr, rval);

            reg = pll_ctrl3_reg[index];
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            soc_pci_getreg(unit, addr, &rval);
            soc_reg_field_set(unit, reg, &rval, NDIV_INTf, 140);
            soc_pci_write(unit, addr, rval);

            reg = pll_ctrl2_reg[index];
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            soc_pci_getreg(unit, addr, &rval);
            soc_reg_field_set(unit, reg, &rval, KAf, 4);
            soc_reg_field_set(unit, reg, &rval, KIf, 1);
            soc_reg_field_set(unit, reg, &rval, KPf, 9);
            soc_reg_field_set(unit, reg, &rval, PDIVf, 7);
            soc_pci_write(unit, addr, rval);

            reg = pll_ctrl4_reg[index];
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            soc_pci_getreg(unit, addr, &rval);
            soc_reg_field_set(unit, reg, &rval, CML_BYP_ENf, 1);
            soc_reg_field_set(unit, reg, &rval, TESTOUT_ENf, 0);
            soc_reg_field_set(unit, reg, &rval, CML_2ED_OUT_ENf, 0);
            soc_reg_field_set(unit, reg, &rval, TESTOUT2_ENf, 0);
            soc_pci_write(unit, addr, rval);
        }

        READ_CMIC_MISC_CONTROLr(unit, &rval);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL0_SW_OVWRf, 1);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL1_SW_OVWRf, 1);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL2_SW_OVWRf, 1);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL3_SW_OVWRf, 1);
        WRITE_CMIC_MISC_CONTROLr(unit, rval);
    }

    /* Bring LCPLL out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &rval);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval, CMIC_XG_PLL0_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval, CMIC_XG_PLL1_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval, CMIC_XG_PLL2_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval, CMIC_XG_PLL3_RST_Lf,
                      1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, rval);

    sal_usleep(to_usec);

    /* Wait for LCPLL lock */
    if (!SAL_BOOT_SIMULATION) {
        for (index = 0; index < 4; index++) {
            reg = pll_status_reg[index];
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            soc_pci_getreg(unit, addr, &rval);
            if (!soc_reg_field_get(unit, reg, rval, CMIC_XGPLL_LOCKf)) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "LCPLL %d not locked on unit %d "
                                      "status = 0x%08x\n"), index, unit, rval));
            }
        }
    }

    /* De-assert LCPLL's post reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &rval);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval,
                      CMIC_XG_PLL0_POST_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval,
                      CMIC_XG_PLL1_POST_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval,
                      CMIC_XG_PLL2_POST_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval,
                      CMIC_XG_PLL3_POST_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, rval);

    sal_usleep(to_usec);

    /*
     * Bring port blocks out of reset
     * PG0: PORT_GROUP5_X, XLPORT0, XLPORT1, XLPORT2, XLPORT3, XLPORT4
     * PG1: PORT_GROUP4_X, XLPORT5, XLPORT6, XLPORT7, XLPORT8
     * PG2: PORT_GROUP5_Y, XLPORT9, XLPORT10, XLPORT11, XLPORT12, XLPORT13
     * PG3: PORT_GROUP4_Y, XLPORT14, XLPORT15, XLPORT16, XLPORT17
     */
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval, CMIC_PG0_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval, CMIC_PG1_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval, CMIC_PG2_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval, CMIC_PG3_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval,
                      CMIC_TEMP_MON_PEAK_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, rval);

    sal_usleep(to_usec);

    /* Bring network sync out of reset */
    rval = 0;
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &rval, XQ0_HOTSWAP_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &rval, XQ1_HOTSWAP_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &rval, XQ2_HOTSWAP_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &rval, XQ3_HOTSWAP_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &rval, XQ4_HOTSWAP_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &rval, XQ5_HOTSWAP_RST_Lf,
                      1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REG_2r, &rval, NS_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REG_2r(unit, rval);

    sal_usleep(to_usec);

    /*
     * SBUS ring and block number:
     * ring 0: IP(1)
     * ring 1: EP(2)
     * ring 2: MMU(3)
     * ring 3: PORT_GROUP4_X(6), PORT_GROUP5_X(8), XLPORT0(10)...XLPORT8(18),
     *         LBPORT0(28)
     * ring 4: PORT_GROUP4_Y(7), PORT_GROUP5_Y(9), XLPORT9(19)...XLPORT17(27)
     * ring 5: OTPC(4)
     */
    WRITE_CMIC_SBUS_RING_MAP_0r(unit, 0x43052100); /* block 7  - 0 */
    WRITE_CMIC_SBUS_RING_MAP_1r(unit, 0x33333343); /* block 15 - 8 */
    WRITE_CMIC_SBUS_RING_MAP_2r(unit, 0x44444333); /* block 23 - 16 */
    WRITE_CMIC_SBUS_RING_MAP_3r(unit, 0x00034444); /* block 31 - 24 */

    WRITE_CMIC_SBUS_TIMEOUTr(unit, 0x7d0);

    /* Bring IP, EP, and MMU blocks out of reset */
    READ_CMIC_SOFT_RESET_REGr(unit, &rval);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval, CMIC_EP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval, CMIC_IP_RST_Lf, 1);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval, CMIC_MMU_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, rval);

    sal_usleep(to_usec);

    READ_CMIC_THERMAL_MON_CTRLr(unit, &rval);
    soc_reg_field_set(unit, CMIC_THERMAL_MON_CTRLr, &rval, BG_ADJf, 1);
    soc_reg_field_set(unit, CMIC_THERMAL_MON_CTRLr, &rval, VTMON_RSTBf, 1);
    WRITE_CMIC_THERMAL_MON_CTRLr(unit, rval);

    sal_usleep(1000);

    READ_CMIC_SOFT_RESET_REGr(unit, &rval);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval,
                      CMIC_TEMP_MON_PEAK_RST_Lf, 0);
    WRITE_CMIC_SOFT_RESET_REGr(unit, rval);
    soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &rval,
                      CMIC_TEMP_MON_PEAK_RST_Lf, 1);
    WRITE_CMIC_SOFT_RESET_REGr(unit, rval);

    if (!(drv_rev_id == BCM56840_A0_REV_ID && rev_id < BCM56840_B0_REV_ID)) {
        READ_CMIC_CORE_PLL3_CTRL_STATUS_REGISTER_3r(unit, &rval);
        soc_reg_field_set(unit, CMIC_CORE_PLL3_CTRL_STATUS_REGISTER_3r, &rval,
                          PVTMON_RESET_Nf, 1);
        WRITE_CMIC_CORE_PLL3_CTRL_STATUS_REGISTER_3r(unit, rval);
    }

    if (dev_id == BCM56846_DEVICE_ID || dev_id == BCM56844_DEVICE_ID ||
        dev_id == BCM56842_DEVICE_ID || dev_id == BCM56746_DEVICE_ID ||
        dev_id == BCM56744_DEVICE_ID || dev_id == BCM56549_DEVICE_ID ||
        dev_id == BCM56053_DEVICE_ID || dev_id == BCM56838_DEVICE_ID ||
        dev_id == BCM56831_DEVICE_ID || dev_id == BCM56835_DEVICE_ID ||
        dev_id == BCM56847_DEVICE_ID || dev_id == BCM56849_DEVICE_ID) {

        rval = 0;
        soc_reg_field_set(unit, ING_CONFIG_2r, &rval,
                          USE_VLAN_ING_PORT_BITMAPf, 1);
        soc_reg_field_set(unit, ING_CONFIG_2r, &rval, FCOE_IFP_KEY_MODEf, 1);
        soc_reg_field_set(unit, ING_CONFIG_2r, &rval,
                          PROTOCOL_PKT_INDEX_PRECEDENCE_MODEf, 1);
        soc_reg_field_set(unit, ING_CONFIG_2r, &rval, WESP_DRAFT_11f, 1);
        soc_reg_field_set(unit, ING_CONFIG_2r, &rval, ECMP_HASH_16BITSf, 1);
        soc_reg_field_set(unit, ING_CONFIG_2r, &rval,
                          TRILL_TRANSIT_MTU_CHECK_ENHANCEDf, 1);
        soc_reg_field_set(unit, ING_CONFIG_2r, &rval, L2_IS_IS_PARSE_MODEf, 1);
        soc_reg_field_set(unit, ING_CONFIG_2r, &rval,
                          DISABLE_INVALID_RBRIDGE_NICKNAMESf, 1);
        soc_reg_field_set(unit, ING_CONFIG_2r, &rval,
                          TRILL_ALL_ESADI_PARSE_MODEf, 1);
        SOC_IF_ERROR_RETURN(WRITE_ING_CONFIG_2r(unit, rval));

        rval = 0;
        soc_reg_field_set(unit, EGR_CONFIG_2r, &rval, DO_NOT_MOD_TPIDf, 1);
        soc_reg_field_set(unit, EGR_CONFIG_2r, &rval,
                          QCN_CNM_MESSAGE_DRAFT_24f, 1);
        soc_reg_field_set(unit, EGR_CONFIG_2r, &rval,
                          USE_RBRIDGES_NICKNAMES_TABLEf, 1);
        soc_reg_field_set(unit, EGR_CONFIG_2r, &rval, WESP_DRAFT_11f, 1);
        soc_reg_field_set(unit, EGR_CONFIG_2r, &rval,
                          TRILL_L2_IS_IS_PARSE_MODEf, 1);
        SOC_IF_ERROR_RETURN(WRITE_EGR_CONFIG_2r(unit, rval));

        rval = 0;
        soc_reg_field_set(unit, ING_Q_BEGINr, &rval, SW_ENC_DEC_TCAM_KEY_MASKf,
                          1);
        soc_reg_field_set(unit, ING_Q_BEGINr, &rval, SUPPORT_8K_VPf, 1);
        soc_reg_field_set(unit, ING_Q_BEGINr, &rval, DISABLE_CELL_COMPRESSIONf,
                          1);
        SOC_IF_ERROR_RETURN(WRITE_ING_Q_BEGINr(unit, rval));

        rval = 0;
        soc_reg_field_set(unit, EGR_Q_BEGINr, &rval, SW_ENC_DEC_TCAM_KEY_MASKf,
                          1);
        SOC_IF_ERROR_RETURN(WRITE_EGR_Q_BEGINr(unit, rval));
    }
    return SOC_E_NONE;
}
#endif  /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
/*
 * Function:
 *      soc_amac_init
 * Purpose:
 *      Bring iproc amac out of reset. 
 *      Common function for Helix4 and Katana2 devices
 */
STATIC int 
soc_amac_init(int unit)
{
    uint32 rval = 0;
    uint32 reg_index = 0;
    soc_reg_t amac_idm_reg[2]={ AMAC_IDM0_IDM_RESET_CONTROLr,
                                AMAC_IDM1_IDM_RESET_CONTROLr };
    soc_reg_t gmac_serdes_reg[2]={ GMAC0_SERDESCONTROLr, 
                                   GMAC1_SERDESCONTROLr };

    for (reg_index = 0;  reg_index < COUNTOF(amac_idm_reg); reg_index++) {
         SOC_IF_ERROR_RETURN(soc_iproc_getreg(unit, 
                             soc_reg_addr(unit, amac_idm_reg[reg_index], 
                             REG_PORT_ANY, 0), &rval));
         if (soc_reg_field_get(unit, amac_idm_reg[reg_index],
                               rval, RESETf) != 0) {
             soc_reg_field_set(unit, AMAC_IDM0_IDM_RESET_CONTROLr,
                               &rval, RESETf, 0);
         }
         SOC_IF_ERROR_RETURN(soc_iproc_setreg(unit, 
                             soc_reg_addr(unit, amac_idm_reg[reg_index], 
                             REG_PORT_ANY, 0), rval));

         SOC_IF_ERROR_RETURN(soc_iproc_getreg(unit, soc_reg_addr(unit,  
                             gmac_serdes_reg[reg_index], 
                             REG_PORT_ANY, 0), &rval));
         if (soc_reg_field_get(unit, gmac_serdes_reg[reg_index],
                               rval, REF_TERM_SELf) == 0) {
             soc_reg_field_set(unit, gmac_serdes_reg[reg_index],
                               &rval, REF_TERM_SELf, 1);
         }
         if (soc_reg_field_get(unit, gmac_serdes_reg[reg_index],
                               rval, REFSELf) == 0) {
             soc_reg_field_set(unit, GMAC0_SERDESCONTROLr,
                               &rval, REFSELf, 1);
         }
         SOC_IF_ERROR_RETURN(soc_iproc_setreg(unit, soc_reg_addr(unit,  
                             gmac_serdes_reg[reg_index], 
                             REG_PORT_ANY, 0), rval));
    }
    return SOC_E_NONE;
}
#endif

#ifdef BCM_HELIX4_SUPPORT

/*
 * Function:
 *      soc_reset_bcm56340_a0
 * Purpose:
 *      Special reset sequencing for BCM56340
 *      Setup SBUS block mapping.
 */
STATIC int
soc_reset_bcm56340_a0(int unit)
{
    int rv, val, index;
    uint32 rval, to_usec;
    soc_persist_t *sop = SOC_PERSIST(unit);
    uint16 dev_id;
    uint8 rev_id;
    static const soc_reg_t pll_ctrl0_reg[] = {
        TOP_XG_PLL0_CTRL_0r, TOP_XG_PLL1_CTRL_0r,
        TOP_XG_PLL2_CTRL_0r, TOP_XG_PLL3_CTRL_0r
    };
    static const soc_reg_t pll_ctrl4_reg[] = {
        TOP_XG_PLL0_CTRL_4r, TOP_XG_PLL1_CTRL_4r
    };

    uint16 soc_dev;
    uint8 soc_rev;

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (100 * MILLISECOND_USEC);

    /*
     * SBUS ring and block number:
     * ring 0: IP(1)
     * ring 1: EP(2)
     * ring 2: MMU(3)
     * ring 3: ISM(16)
     * ring 4: SER(17)
     * ring 5: AXP(4)
     * ring 6: XTP0(8)..XTP3(11), XLP0(12), IBOD(18),
               XWP0(13).., XWP2(15)
     * ring 7: OTPC(5), TOP_REGS(6)
     */
    WRITE_CMIC_SBUS_RING_MAP_0_7r(unit, 0x07752100);   /* block 7  - 0 */
    WRITE_CMIC_SBUS_RING_MAP_8_15r(unit, 0x66666666);  /* block 15 - 8 */
    WRITE_CMIC_SBUS_RING_MAP_16_23r(unit, 0x00000643); /* block 23 - 16 */
    WRITE_CMIC_SBUS_RING_MAP_24_31r(unit, 0x00000000); /* block 31 - 24 */


    /* Reconfig XMAC cclk - necessary for Helix4 XL port XMAC FIFO UR */
    soc_cm_get_id(unit, &soc_dev, &soc_rev);
    if ( (BCM56340_DEVICE_ID == soc_dev) || (BCM56041_DEVICE_ID == soc_dev) ||
         (BCM56042_DEVICE_ID == soc_dev) ) {
        if (SOC_E_NONE != (rv = _soc_hx4_xmac_cclk_config(unit))) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Unit: %d, Helix4 XMAC cclk config failed !!\n"),
                       unit));
            return rv;
        }
    }

    if (soc_property_get(unit, spn_XGXS_LCPLL_XTAL_REFCLK, 1)) {
        rval = 0x14;    /* 156.25Mhz */
    } else {
        rval = 0x7d;    /* 25Mhz */
    }
    for (index = 0; index < 2; index++) {
        rv = soc_reg_field32_modify(unit, pll_ctrl4_reg[index], 
                                    REG_PORT_ANY, NDIV_INTf, rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Modifying TOP_XG_PLL%d_CTRL_4 failed !!\n"),
                       index));
            return rv;                
        }
    }    
   
    if (soc_property_get(unit, spn_XGXS_QGPLL_XTAL_REFCLK, 1)) {
        rval = 0x14; /* 156.25Mhz */
    } else {
        rval = 0x7d; /* 25Mhz */
    }
    rv = soc_reg_field32_modify(unit, pll_ctrl4_reg[0],
                                REG_PORT_ANY, NDIV_INTf, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Modifying TOP_XG_PLL0_CTRL_4:QG_PLL failed !!\n")));
        return rv;
    }

    if (soc_property_get(unit, spn_XGXS_WCPLL_XTAL_REFCLK, 1)) {
        rval = 0x14; /* 156.25Mhz */
    } else {
        rval = 0x7d; /* 25Mhz */
    }
    rv = soc_reg_field32_modify(unit, pll_ctrl4_reg[1],
                                REG_PORT_ANY, NDIV_INTf, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Modifying TOP_XG_PLL1_CTRL_4:WC_PLL failed !!\n")));
        return rv;
    }

    for (index = 0; index < 4; index++) {
        rv = soc_reg_field32_modify(unit, pll_ctrl0_reg[index],
                                    REG_PORT_ANY, CH3_MDIVf, 25);
            if (rv < 0) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "Modifying TOP_XG_PLL%d_CTRL_0 failed !!\n"),
                           index));
                return rv;
            }
    }

    rv = READ_TOP_MISC_CONTROL_1r(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_MISC_CONTROL_1 failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval,
                      CMIC_TO_XG_PLL0_SW_OVWRf, 1);
    soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval,
                      CMIC_TO_XG_PLL1_SW_OVWRf, 1);
    soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval,
                      CMIC_TO_XG_PLL2_SW_OVWRf, 1);
    soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval,
                      CMIC_TO_XG_PLL3_SW_OVWRf, 1);
    soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval,
                      IO_HYS_EN_CTRLf, 1);
    rv = WRITE_TOP_MISC_CONTROL_1r(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_MISC_CONTROL_1 failed !!\n")));
        return rv;
    }

    /* Bring LCPLL out of reset */
    rv = READ_TOP_SOFT_RESET_REG_2r(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG_PLL0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG_PLL1_RST_Lf, 1);
    rv = WRITE_TOP_SOFT_RESET_REG_2r(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    sal_usleep(to_usec);

    /* Wait for LCPLL lock */
    if (!SAL_BOOT_SIMULATION) {
        rv = READ_TOP_XG_PLL0_STATUSr(unit, &rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Reading TOP_XG_PLL0_STATUS failed !!\n")));
            return rv;
        }
        if (!soc_reg_field_get(unit, TOP_XG_PLL0_STATUSr, rval, TOP_XGPLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 0 not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }
        rv = READ_TOP_XG_PLL1_STATUSr(unit, &rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Reading TOP_XG_PLL1_STATUS failed !!\n")));
            return rv;
        }
        if (!soc_reg_field_get(unit, TOP_XG_PLL1_STATUSr, rval, TOP_XGPLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 1 not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }
    }

    /* De-assert LCPLL's post reset */
    rv = READ_TOP_SOFT_RESET_REG_2r(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG_PLL0_POST_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG_PLL1_POST_RST_Lf, 1);
    rv = WRITE_TOP_SOFT_RESET_REG_2r(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    sal_usleep(to_usec);


    /* Configure TS PLL */
    SOC_IF_ERROR_RETURN(READ_TOP_TS_PLL_CTRL_2r(unit,&rval));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_2r, &rval, PDIVf,
                      soc_property_get(unit, spn_PTP_TS_PLL_PDIV, 1));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_2r, &rval, CH0_MDIVf,
                      soc_property_get(unit, spn_PTP_TS_PLL_MNDIV, 5));
    WRITE_TOP_TS_PLL_CTRL_2r(unit, rval);

    SOC_IF_ERROR_RETURN(READ_TOP_TS_PLL_CTRL_3r(unit,&rval));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_3r, &rval, NDIV_INTf,
                      soc_property_get(unit, spn_PTP_TS_PLL_N, 100));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_3r, &rval, NDIV_FRACf, 0);
    WRITE_TOP_TS_PLL_CTRL_3r(unit, rval);

    SOC_IF_ERROR_RETURN(READ_TOP_TS_PLL_CTRL_4r(unit,&rval));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_4r, &rval, KAf,
                      soc_property_get(unit, spn_PTP_TS_KA, 2));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_4r, &rval, KIf,
                      soc_property_get(unit, spn_PTP_TS_KI, 2));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_4r, &rval, KPf,
                      soc_property_get(unit, spn_PTP_TS_KP, 6));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_4r, &rval, REFCLK_SELf, 1);
    WRITE_TOP_TS_PLL_CTRL_4r(unit, rval);

    /* Set 250Mhz (implies 4ns resolution) default timesync clock to 
       calculate assymentric delays */
     SOC_TIMESYNC_PLL_CLOCK_NS(unit) = (1/250 * 1000); /* clock period in nanoseconds */

    /* put broadsync pll in reset */
    rv = READ_TOP_SOFT_RESET_REG_2r(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_BS_PLL0_RST_Lf, 0);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_BS_PLL0_POST_RST_Lf, 0);
    rv = WRITE_TOP_SOFT_RESET_REG_2r(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }

    /* Configure BS PLL */ 
    SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL0_CTRL_0r(unit,&rval));
    soc_reg_field_set(unit, TOP_BS_PLL0_CTRL_0r, &rval, VCO_DIV2f,
                      0);  /* soc_property_get(unit, spn_BROAD_SYNC_VCO_DIV2, 0)); */
    WRITE_TOP_BS_PLL0_CTRL_0r(unit, rval);

    /* assumption, 25MHz is from internal, not external, source */
    if (soc_property_get(unit, spn_PTP_BS_FREF, 25000000) == 25000000) {
        SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL0_CTRL_4r(unit,&rval));
        soc_reg_field_set(unit, TOP_BS_PLL0_CTRL_4r, &rval, REFCLK_SELf, 0);
        WRITE_TOP_BS_PLL0_CTRL_4r(unit, rval);

        SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL0_CTRL_2r(unit,&rval));
        soc_reg_field_set(unit, TOP_BS_PLL0_CTRL_2r, &rval, CH0_MDIVf,
                          soc_property_get(unit, spn_PTP_BS_MNDIV, 125));
        WRITE_TOP_BS_PLL0_CTRL_2r(unit, rval);

        SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL0_CTRL_3r(unit,&rval));
        soc_reg_field_set(unit, TOP_BS_PLL0_CTRL_3r, &rval, NDIV_INTf,
                          soc_property_get(unit, spn_PTP_BS_NDIV_INT, 100));
        WRITE_TOP_BS_PLL0_CTRL_3r(unit, rval);
    } else {
        SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL0_CTRL_4r(unit,&rval));
        soc_reg_field_set(unit, TOP_BS_PLL0_CTRL_4r, &rval, REFCLK_SELf, 1);
        WRITE_TOP_BS_PLL0_CTRL_4r(unit, rval);

        SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL0_CTRL_2r(unit,&rval));
        soc_reg_field_set(unit, TOP_BS_PLL0_CTRL_2r, &rval, CH0_MDIVf,
                          soc_property_get(unit, spn_PTP_BS_MNDIV, 120));
        WRITE_TOP_BS_PLL0_CTRL_2r(unit, rval);

        SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL0_CTRL_3r(unit,&rval));
        soc_reg_field_set(unit, TOP_BS_PLL0_CTRL_3r, &rval, NDIV_INTf,
                          soc_property_get(unit, spn_PTP_BS_NDIV_INT, 96));
        WRITE_TOP_BS_PLL0_CTRL_3r(unit, rval);
    }

    /* Timesync and broadsync out of reset */
    rv = READ_TOP_SOFT_RESET_REG_2r(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_TS_PLL_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_BS_PLL0_RST_Lf, 1);
    rv = WRITE_TOP_SOFT_RESET_REG_2r(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    sal_usleep(to_usec);

    /* Wait for the BS/TS pll to lock */
    if (!SAL_BOOT_SIMULATION) {

        rv = READ_TOP_TS_PLL_STATUSr(unit, &rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Reading TOP_TS_PLL_STATUS failed !!\n")));
            return rv;
        }
        if (!soc_reg_field_get(unit, TOP_TS_PLL_STATUSr, rval, PLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "TimeSync PLL not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }

        rv = READ_TOP_BS_PLL0_STATUSr(unit, &rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Reading TOP_BS_PLL0_STATUS failed !!\n")));
            return rv;
        }
        if (!soc_reg_field_get(unit, TOP_BS_PLL0_STATUSr, rval, PLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "BroadSync PLL0 not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }
    }

    /* Deassert Post reset */
    rv = READ_TOP_SOFT_RESET_REG_2r(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_TS_PLL_POST_RST_Lf,
                      1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_BS_PLL0_POST_RST_Lf,
                      1);
    rv = WRITE_TOP_SOFT_RESET_REG_2r(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }

    soc_cm_get_id(unit, &dev_id, &rev_id);
    val = soc_property_get(unit, "iproc_shutdown", 0);
    if (val) {
        uint32 cpu_mask = 0x1;
        /* Spiral SKUs have a single core, all other SKUs have dual cores */
        if (!(dev_id == BCM56047_DEVICE_ID || dev_id == BCM56048_DEVICE_ID ||
              dev_id == BCM56049_DEVICE_ID)) {
            cpu_mask |= 0x2;
        }
        rv = soc_iproc_shutdown(unit, cpu_mask, val-1);
        if (SOC_FAILURE(rv)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Putting iproc in shutdown failed !!\n")));
        }
    }

    /* Check for M, A and Plain versions */
    SOC_IF_ERROR_RETURN(READ_TOP_SWITCH_FEATURE_ENABLEr(unit, &rval));
    if (rval & 0x1) {
        /* BOND_1588_ENABLE */
        SOC_FEATURE_SET(unit, soc_feature_timesync_support);
    }
    if (!(rval & 0x8)) {
        /* BOND_MPLS_ENABLE */
        SOC_FEATURE_CLEAR(unit, soc_feature_mpls);
    }
    if (!(rval & 0x10)) {
        /* BOND_IP_ENABLE */
        SOC_FEATURE_CLEAR(unit, soc_feature_lpm_tcam);
        SOC_FEATURE_CLEAR(unit, soc_feature_ip_mcast);
        SOC_FEATURE_CLEAR(unit, soc_feature_ip_mcast_repl);
        SOC_FEATURE_CLEAR(unit, soc_feature_l3);
        SOC_FEATURE_CLEAR(unit, soc_feature_l3_ip6);
        SOC_FEATURE_CLEAR(unit, soc_feature_l3_lookup_cmd);
        SOC_FEATURE_CLEAR(unit, soc_feature_l3_sgv);
        /* Following features depend on IP feature */
        SOC_FEATURE_CLEAR(unit, soc_feature_subport);
        SOC_FEATURE_CLEAR(unit, soc_feature_subport_enhanced);
        SOC_FEATURE_CLEAR(unit, soc_feature_trill);
        SOC_FEATURE_CLEAR(unit, soc_feature_l2gre);
    }
    /* Adjust memory sizes for the A variants - addendum to soc_hx4_mem_config */
    if (!(rval & 0x10)) {
        /* BOND_IP_ENABLE */
        switch (dev_id) {
            case BCM56347_DEVICE_ID:
            case BCM56346_DEVICE_ID:
            case BCM56344_DEVICE_ID:
            case BCM56342_DEVICE_ID:
            case BCM56340_DEVICE_ID:
                sop->memState[MPLS_ENTRYm].index_max = -1;
                sop->memState[MPLS_ENTRY_1m].index_max = -1;
                sop->memState[MPLS_ENTRY_EXTDm].index_max = -1;
                sop->memState[L3_ENTRY_1m].index_max = -1;
                sop->memState[L3_ENTRY_2m].index_max = -1;
                sop->memState[L3_ENTRY_4m].index_max = -1;
                sop->memState[EGR_IPMCm].index_max = -1;
                sop->memState[EGR_L3_NEXT_HOPm].index_max = -1;
                sop->memState[ING_L3_NEXT_HOPm].index_max = -1;
                sop->memState[INITIAL_ING_L3_NEXT_HOPm].index_max = -1;
                sop->memState[INITIAL_PROT_NHI_TABLEm].index_max = -1;
                sop->memState[L3_IPMCm].index_max = 1024;
                sop->memState[L3_IPMC_1m].index_max = 1024;
                sop->memState[L3_IPMC_REMAPm].index_max = 1024;
                sop->memState[MMU_REPL_GROUPm].index_max = 1024;
                sop->memState[VRFm].index_max = 128;  
                sop->memState[L3_DEFIPm].index_max = -1;
                sop->memState[L3_DEFIP_ONLYm].index_max = -1;
                sop->memState[L3_DEFIP_PAIR_128m].index_max = -1;
                break;
            default:
                break;
        }
    }

    /*
     * Bring port blocks out of reset
     */
    rv = READ_TOP_SOFT_RESET_REGr(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XTP0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XTP1_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XTP2_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XTP3_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XLP0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XWP0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XWP1_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XWP2_RST_Lf, 1);

    rv = WRITE_TOP_SOFT_RESET_REGr(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }

    sal_usleep(to_usec);

    /* Bring network sync out of reset */
    rv = READ_TOP_SOFT_RESET_REGr(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_NS_RST_Lf, 1);
    rv = WRITE_TOP_SOFT_RESET_REGr(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }

    sal_usleep(to_usec);

    WRITE_CMIC_SBUS_TIMEOUTr(unit, 0x7d0);

    /* Bring IP, EP, ISM, ETU, AXP, and MMU blocks out of reset */
    rv = READ_TOP_SOFT_RESET_REGr(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_EP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_IP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MMU_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_ISM_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_AXP_RST_Lf, 1);
    rv = WRITE_TOP_SOFT_RESET_REGr(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }

    sal_usleep(to_usec);

    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_2r(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval,
                      TOP_TEMP_MON_PEAK_RST_Lf, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval,
                      TOP_TEMP_MON_PEAK_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));

    SOC_IF_ERROR_RETURN(READ_TOP_PVTMON_CTRL_1r(unit, &rval));
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_1r, &rval, PVTMON_RESET_Nf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_PVTMON_CTRL_1r(unit, rval));
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_1r, &rval, PVTMON_RESET_Nf, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_PVTMON_CTRL_1r(unit, rval));
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_1r, &rval, PVTMON_RESET_Nf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_PVTMON_CTRL_1r(unit, rval));

    
    WRITE_CMIC_SBUS_TIMEOUTr(unit, 0x7A120);

    /* Set MDIO IO PAD voltage */
    if (0 != soc_property_get(unit, spn_MDIO_IO_VOLTAGE, 0)) {
        rv = READ_TOP_MISC_CONTROL_1r(unit, &rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Reading TOP_MISC_CONTROL_1 failed !!\n")));
            return rv;
        }
        soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval,
                          XG_MDIO0_CL_SELf, 1);
        rv = WRITE_TOP_MISC_CONTROL_1r(unit, rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Writing TOP_MISC_CONTROL_1 failed !!\n")));
            return rv;
        }
    }
    /* Bring AMAC out of reset */
    soc_amac_init(unit);

    return SOC_E_NONE;
}
#endif

#ifdef BCM_TRIUMPH3_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56640_a0
 * Purpose:
 *      Special reset sequencing for BCM56640
 *      Setup SBUS block mapping.
 */
STATIC int
soc_reset_bcm56640_a0(int unit)
{
    uint32 rval, to_usec;
    soc_reg_t reg;
    int index, rv;
    
    static const soc_reg_t pll_ctrl0_reg[] = {
        TOP_XG_PLL0_CTRL_0r, TOP_XG_PLL1_CTRL_0r,
        TOP_XG_PLL2_CTRL_0r, TOP_XG_PLL3_CTRL_0r
    };
    static const soc_reg_t pll_ctrl1_reg[] = {
        TOP_XG_PLL0_CTRL_1r, TOP_XG_PLL1_CTRL_1r,
        TOP_XG_PLL2_CTRL_1r, TOP_XG_PLL3_CTRL_1r
    };
    static const soc_reg_t pll_ctrl3_reg[] = {
        TOP_XG_PLL0_CTRL_3r, TOP_XG_PLL1_CTRL_3r,
        TOP_XG_PLL2_CTRL_3r, TOP_XG_PLL3_CTRL_3r
    };
    static const soc_reg_t pll_ctrl4_reg[] = {
        TOP_XG_PLL0_CTRL_4r, TOP_XG_PLL1_CTRL_4r,
        TOP_XG_PLL2_CTRL_4r, TOP_XG_PLL3_CTRL_4r
    };

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);

    /*
     * SBUS ring and block number:
     * ring 0: IP(1)
     * ring 1: EP(2)
     * ring 2: MMU(3)
     * ring 3: ISM(16)
     * ring 4: ETU(17)
     * ring 5: AXP(4)
     * ring 6: XTP0(8)..XTP2(10), XLP0(11), XTP3(12), IBOD(18), 
               CLP0(13), XLP1(14), CLP1(15)
     * ring 7: OTPC(5), TOP_REGS(6)
     */
    WRITE_CMIC_SBUS_RING_MAP_0_7r(unit, 0x07752100);   /* block 7  - 0 */
    WRITE_CMIC_SBUS_RING_MAP_8_15r(unit, 0x66666666);  /* block 15 - 8 */
    WRITE_CMIC_SBUS_RING_MAP_16_23r(unit, 0x00000643); /* block 23 - 16 */
    WRITE_CMIC_SBUS_RING_MAP_24_31r(unit, 0x00000000); /* block 31 - 24 */

    
    /* Note that only PLL0 & PLL1 are used by TR3 */
    /* Set the OOBFC clock speed to 125MHz */     
    for (index = 0; index < 4; index++) {
        rv = soc_reg_field32_modify(unit, pll_ctrl0_reg[index],
                                    REG_PORT_ANY, CH3_MDIVf, 25);
            if (rv < 0) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "Modifying TOP_PLL%d_CTRL_0 failed !!\n"),
                           index));
                return rv;
            }
    }

    /* Use 156.25Mhz reference clock for LCPLL */
    if (soc_property_get(unit, spn_XGXS_LCPLL_XTAL_REFCLK, 0)) {
        for (index = 0; index < 4; index++) {
            reg = pll_ctrl0_reg[index];
            rv = soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &rval);
            if (rv < 0) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "Reading TOP_PLL%d_CTRL_0 failed !!\n"),
                           index));
                return rv;
            }
            soc_reg_field_set(unit, reg, &rval, CH0_MDIVf, 20);
            soc_reg_field_set(unit, reg, &rval, CH4_MDIVf, 125);
            soc_reg_field_set(unit, reg, &rval, CH5_MDIVf, 25);
            rv = soc_reg32_set(unit, reg, REG_PORT_ANY, 0, rval);
            if (rv < 0) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "Writing TOP_PLL%d_CTRL_0 failed !!\n"),
                           index));
                return rv;
            }

            rv = soc_reg_field32_modify(unit, pll_ctrl4_reg[index],
                                        REG_PORT_ANY, NDIV_INTf, 40);
            if (rv < 0) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "Modifying TOP_PLL%d_CTRL_4 failed !!\n"),
                           index));
                return rv;
            }

            rv = soc_reg_field32_modify(unit, pll_ctrl1_reg[index],
                                        REG_PORT_ANY, PDIVf, 2);
            if (rv < 0) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "Modifying TOP_PLL%d_CTRL_1 failed !!\n"),
                           index));
                return rv;
            }

            rv = soc_reg_field32_modify(unit, pll_ctrl3_reg[index],
                                        REG_PORT_ANY, CML_BYP_ENf, 1);
            if (rv < 0) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "Modifying TOP_PLL%d_CTRL_3 failed !!\n"),
                           index));
                return rv;
            }
        }
    }

    rv = READ_TOP_MISC_CONTROLr(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_MISC_CONTROL failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                      CMIC_TO_XG_PLL0_SW_OVWRf, 1);
    soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                      CMIC_TO_XG_PLL1_SW_OVWRf, 1);
    soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                      CMIC_TO_XG_PLL2_SW_OVWRf, 1);
    soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                      CMIC_TO_XG_PLL3_SW_OVWRf, 1);
    rv = WRITE_TOP_MISC_CONTROLr(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_MISC_CONTROL failed !!\n")));
        return rv;
    }

    /* Bring LCPLL out of reset */
    rv = READ_TOP_SOFT_RESET_REG_2r(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG_PLL0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG_PLL1_RST_Lf, 1);
    rv = WRITE_TOP_SOFT_RESET_REG_2r(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    sal_usleep(to_usec);

    /* Wait for LCPLL lock */
    if (!SAL_BOOT_SIMULATION) {
        rv = READ_TOP_XG_PLL0_STATUSr(unit, &rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Reading TOP_XG_PLL0_STATUS failed !!\n")));
            return rv;
        }
        if (!soc_reg_field_get(unit, TOP_XG_PLL0_STATUSr, rval, TOP_XGPLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 0 not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }
        rv = READ_TOP_XG_PLL1_STATUSr(unit, &rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Reading TOP_XG_PLL1_STATUS failed !!\n")));
            return rv;
        }
        if (!soc_reg_field_get(unit, TOP_XG_PLL1_STATUSr, rval, TOP_XGPLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 1 not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }
    }

    /* De-assert LCPLL's post reset */
    rv = READ_TOP_SOFT_RESET_REG_2r(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG_PLL0_POST_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG_PLL1_POST_RST_Lf, 1);
    rv = WRITE_TOP_SOFT_RESET_REG_2r(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    sal_usleep(to_usec);

    
    /* Configure TS PLL */ 
    SOC_IF_ERROR_RETURN(READ_TOP_TS_PLL_CTRL_2r(unit,&rval));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_2r, &rval, PDIVf,
                      soc_property_get(unit, spn_PTP_TS_PLL_PDIV, 1));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_2r, &rval, CH0_MDIVf,
                      soc_property_get(unit, spn_PTP_TS_PLL_MNDIV, 10));
    WRITE_TOP_TS_PLL_CTRL_2r(unit, rval);
    
    SOC_IF_ERROR_RETURN(READ_TOP_TS_PLL_CTRL_3r(unit,&rval));    
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_3r, &rval, NDIV_INTf,
                      soc_property_get(unit, spn_PTP_TS_PLL_N, 100));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_3r, &rval, NDIV_FRACf, 0);
    WRITE_TOP_TS_PLL_CTRL_3r(unit, rval);
    
    SOC_IF_ERROR_RETURN(READ_TOP_TS_PLL_CTRL_4r(unit,&rval));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_4r, &rval, KAf,
                      soc_property_get(unit, spn_PTP_TS_KA, 2));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_4r, &rval, KIf,
                      soc_property_get(unit, spn_PTP_TS_KI, 2));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_4r, &rval, KPf,
                      soc_property_get(unit, spn_PTP_TS_KP, 6));
    soc_reg_field_set(unit, TOP_TS_PLL_CTRL_4r, &rval, REFCLK_SELf, 1);
    WRITE_TOP_TS_PLL_CTRL_4r(unit, rval);
    
    /* Set 250Mhz (implies 4ns resolution) default timesync clock to 
       calculate assymentric delays */
     SOC_TIMESYNC_PLL_CLOCK_NS(unit) = (1/250 * 1000); /* clock period in nanoseconds */
     
    /* Configure BS PLL */
    SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL_CTRL_0r(unit,&rval));
    soc_reg_field_set(unit, TOP_BS_PLL_CTRL_0r, &rval, VCO_DIV2f,
                      1);  /* soc_property_get(unit, spn_BROAD_SYNC_VCO_DIV2, 1)); */
    WRITE_TOP_BS_PLL_CTRL_0r(unit, rval);

    SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL_CTRL_2r(unit,&rval));
    soc_reg_field_set(unit, TOP_BS_PLL_CTRL_2r, &rval, PDIVf,
                      soc_property_get(unit, spn_PTP_BS_PDIV, 2));
    soc_reg_field_set(unit, TOP_BS_PLL_CTRL_2r, &rval, CH0_MDIVf,
                      soc_property_get(unit, spn_PTP_BS_MNDIV, 200));
    WRITE_TOP_BS_PLL_CTRL_2r(unit, rval);

    SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL_CTRL_3r(unit,&rval));
    soc_reg_field_set(unit, TOP_BS_PLL_CTRL_3r, &rval, NDIV_INTf,
                      soc_property_get(unit, spn_PTP_BS_NDIV_INT, 320));
    soc_reg_field_set(unit, TOP_BS_PLL_CTRL_3r, &rval, NDIV_FRACf,
                      0);
    WRITE_TOP_BS_PLL_CTRL_3r(unit, rval);

    SOC_IF_ERROR_RETURN(READ_TOP_BS_PLL_CTRL_4r(unit,&rval));
    soc_reg_field_set(unit, TOP_BS_PLL_CTRL_4r, &rval, KAf,
                      soc_property_get(unit, spn_PTP_BS_KA, 2));
    soc_reg_field_set(unit, TOP_BS_PLL_CTRL_4r, &rval, KIf,
                      soc_property_get(unit, spn_PTP_BS_KI, 4));
    soc_reg_field_set(unit, TOP_BS_PLL_CTRL_4r, &rval, KPf,
                      soc_property_get(unit, spn_PTP_BS_KP, 9));
    /* assumption, 25MHz is from internal, not external, source */
    if (soc_property_get(unit, spn_PTP_BS_FREF, 25000000) == 25000000) {
        soc_reg_field_set(unit, TOP_BS_PLL_CTRL_4r, &rval, REFCLK_SELf, 0);
    } else {
        soc_reg_field_set(unit, TOP_BS_PLL_CTRL_4r, &rval, REFCLK_SELf, 1);
    }
    WRITE_TOP_BS_PLL_CTRL_4r(unit, rval);

    /* Timesync and broadsync out of reset */
    rv = READ_TOP_SOFT_RESET_REG_2r(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_TS_PLL_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_BS_PLL_RST_Lf, 1);
    rv = WRITE_TOP_SOFT_RESET_REG_2r(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    sal_usleep(to_usec);

    /* Wait for the BS/TS pll to lock */
    if (!SAL_BOOT_SIMULATION) {
    
        rv = READ_TOP_TS_PLL_STATUSr(unit, &rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Reading TOP_TS_PLL_STATUS failed !!\n")));
            return rv;
        }
        if (!soc_reg_field_get(unit, TOP_TS_PLL_STATUSr, rval, PLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "TimeSync PLL not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }

        rv = READ_TOP_BS_PLL_STATUSr(unit, &rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Reading TOP_BS_PLL_STATUS failed !!\n")));
            return rv;
        }
        if (!soc_reg_field_get(unit, TOP_BS_PLL_STATUSr, rval, PLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "BroadSync PLL not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }
    }

    /* Deassert Post reset */
    rv = READ_TOP_SOFT_RESET_REG_2r(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_TS_PLL_POST_RST_Lf,
                      1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_BS_PLL_POST_RST_Lf,
                      1);
    rv = WRITE_TOP_SOFT_RESET_REG_2r(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG_2 failed !!\n")));
        return rv;
    }
    
    SOC_IF_ERROR_RETURN(READ_TOP_SWITCH_FEATURE_ENABLEr(unit, &rval));
    if (rval & 0x1) {
        /* BOND_1588_ENABLE */
        SOC_FEATURE_SET(unit, soc_feature_timesync_support);
    }
    
    /*
     * Bring port blocks out of reset
     */
    rv = READ_TOP_SOFT_RESET_REGr(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XTP0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XTP1_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XTP2_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XTP3_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XLP0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_XLP1_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_CLP0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_CLP1_RST_Lf, 1);
    rv = WRITE_TOP_SOFT_RESET_REGr(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }
    
    sal_usleep(to_usec);
    
    /* Bring network sync out of reset */
    rv = READ_TOP_SOFT_RESET_REGr(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_NS_RST_Lf, 1);
    rv = WRITE_TOP_SOFT_RESET_REGr(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }
    
    sal_usleep(to_usec);

    WRITE_CMIC_SBUS_TIMEOUTr(unit, 0x7d0);

    /* Bring IP, EP, ISM, ETU, AXP, and MMU blocks out of reset */
    rv = READ_TOP_SOFT_RESET_REGr(unit, &rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Reading TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_EP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_IP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MMU_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_ISM_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_AXP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_ETU_RST_Lf, 1);
    rv = WRITE_TOP_SOFT_RESET_REGr(unit, rval);
    if (rv < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Writing TOP_SOFT_RESET_REG failed !!\n")));
        return rv;
    }
    
    sal_usleep(to_usec);
    
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_2r(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval,
                      TOP_TEMP_MON_PEAK_RST_Lf, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval,
                      TOP_TEMP_MON_PEAK_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));
    
    SOC_IF_ERROR_RETURN(READ_TOP_PVTMON_CTRL_1r(unit, &rval));
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_1r, &rval, PVTMON_RESET_Nf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_PVTMON_CTRL_1r(unit, rval));
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_1r, &rval, PVTMON_RESET_Nf, 0);
    SOC_IF_ERROR_RETURN(WRITE_TOP_PVTMON_CTRL_1r(unit, rval));
    soc_reg_field_set(unit, TOP_PVTMON_CTRL_1r, &rval, PVTMON_RESET_Nf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_PVTMON_CTRL_1r(unit, rval));

    /* need higher timeout for access to 480 bit tcam entries */
    WRITE_CMIC_SBUS_TIMEOUTr(unit, 0x7A120);
    /* Enable access to ESM regs */
    if (soc_feature(unit, soc_feature_esm_support)) {
        to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                                (10 * MILLISECOND_USEC);

        rv = READ_ESMIF_INIT_CONFIGr(unit, &rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Reading ESMIF_INIT_CONFIG failed !!\n")));
            return rv;
        }
        soc_reg_field_set(unit, ESMIF_INIT_CONFIGr, &rval, LATENCYf, 870);
        rv = WRITE_ESMIF_INIT_CONFIGr(unit, rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Writing ESMIF_INIT_CONFIG failed !!\n")));
            return rv;
        }
        soc_reg_field_set(unit, ESMIF_INIT_CONFIGr, &rval, WR_ENABLEf, 1);
        sal_usleep(to_usec);
        soc_reg_field_set(unit, ESMIF_INIT_CONFIGr, &rval, 
                        IESMIF_BYPASS_ENABLEf, 0);
        rv = WRITE_ESMIF_INIT_CONFIGr(unit, rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Writing ESMIF_INIT_CONFIG failed !!\n")));
            return rv;
        }
    }
    /* Set MDIO IO PAD voltage */
    if (0 != soc_property_get(unit, spn_MDIO_IO_VOLTAGE, 0)) {
        rv = READ_TOP_MISC_CONTROLr(unit, &rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Reading TOP_MISC_CONTROL failed !!\n")));
            return rv;
        }
        soc_reg_field_set(unit, TOP_MISC_CONTROLr, &rval,
                          XG_MDIO0_CL_SELf, 1);
        rv = WRITE_TOP_MISC_CONTROLr(unit, rval);
        if (rv < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Writing TOP_MISC_CONTROL failed !!\n")));
            return rv;
        }
    }
    return SOC_E_NONE;
}
#endif  /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56440_a0
 * Purpose:
 *      Special reset sequencing for BCM56440
 *      Setup SBUS block mapping.
 */
STATIC int
soc_reset_bcm56440_a0(int unit)
{
    uint32 rval, to_usec;
    uint16 dev_id;
    uint8 rev_id;

    /* TS PLL settings for 250MHZ output */
    unsigned ts_ref_freq = 0;
    unsigned ts_idx=0;

    /* values for 250 MHz TSPLL output */
    static const soc_pll_param_t katana_ts_pll[] = {
    /*     Fref, Ndiv_int, Ndiv_frac,  Pdiv, Mdiv, Ka, Ki, Kp, VCO_div2 */
      {12800000,      310,         0,     1,   16,  1,  2,  7,        1},
      {20000000,      200,         0,     1,   16,  2,  3,  8,        1},
      {25000000,      160,         0,     1,   16,  2,  4,  9,        1},
      {32000000,      125,         0,     1,   16,  2,  4,  9,        1},
      {       0,      160,         0,     1,   16,  2,  4,  9,        1} /*
      25MHZ from internal reference */
    };

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);
 
    /*
     * SBUS ring and block number:
     * Ring0: cmic -> ip-ep -> cmic
     * Ring1: cmic -> mxqport0 -> mxqport1-> mxqport2 -> mxqport3-> cmic
     * Ring2: cmic -> gport0 -> gport1 -> gport2 -> cmic
     * Ring3: cmic -> otpc -> top -> cmic
     * Ring4: cmic -> mmu0 -> mmu1 -> cmic
     * Ring5: cmic -> ci0 -> ci1 -> ci2 -> lls -> cmic
     * Ring6: cmic -> ces -> cmic
     * Ring7: unused
     */
    WRITE_CMIC_SBUS_RING_MAP_0_7r(unit, 0x22034000); /* block 7  - 0 */
    WRITE_CMIC_SBUS_RING_MAP_8_15r(unit, 0x55311112); /* block 15 - 8 */
    WRITE_CMIC_SBUS_RING_MAP_16_23r(unit, 0x00000655); /* block 23 - 16 */
    WRITE_CMIC_SBUS_RING_MAP_24_31r(unit, 0x00000000); /* block 31 - 24 */



    soc_cm_get_id(unit, &dev_id, &rev_id);

    switch (dev_id) {
        case BCM56440_DEVICE_ID: /* 135 MHz Operation */
        case BCM55440_DEVICE_ID:
        case BCM55441_DEVICE_ID:    
        case BCM56445_DEVICE_ID:
        case BCM56448_DEVICE_ID:
        case BCM56449_DEVICE_ID:
            SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL1_CTRL1r(unit, &rval));
            soc_reg_field_set(unit, TOP_MMU_PLL1_CTRL1r, &rval, KPf, 10);
            SOC_IF_ERROR_RETURN(WRITE_TOP_MMU_PLL1_CTRL1r(unit, rval));
            SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL2_CTRL1r(unit, &rval));
            soc_reg_field_set(unit, TOP_MMU_PLL2_CTRL1r, &rval, KPf, 10);
            SOC_IF_ERROR_RETURN(WRITE_TOP_MMU_PLL2_CTRL1r(unit, rval));
            SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL3_CTRL1r(unit, &rval));
            soc_reg_field_set(unit, TOP_MMU_PLL3_CTRL1r, &rval, KPf, 10);
            SOC_IF_ERROR_RETURN(WRITE_TOP_MMU_PLL3_CTRL1r(unit, rval));
            SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL1_CTRL3r(unit, &rval));
            soc_reg_field_set(unit, TOP_MMU_PLL1_CTRL3r, &rval, PLL_CTRL_VCO_DIV2f, 1);
            SOC_IF_ERROR_RETURN(WRITE_TOP_MMU_PLL1_CTRL3r(unit, rval));
            SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL2_CTRL3r(unit, &rval));
            soc_reg_field_set(unit, TOP_MMU_PLL2_CTRL3r, &rval, PLL_CTRL_VCO_DIV2f, 1);
            SOC_IF_ERROR_RETURN(WRITE_TOP_MMU_PLL2_CTRL3r(unit, rval));
            SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL3_CTRL3r(unit, &rval));
            soc_reg_field_set(unit, TOP_MMU_PLL3_CTRL3r, &rval, PLL_CTRL_VCO_DIV2f, 1);
            SOC_IF_ERROR_RETURN(WRITE_TOP_MMU_PLL3_CTRL3r(unit, rval));
            break;
        default: /* 90 MHz Operation */
            SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL1_CTRL3r(unit, &rval));
            soc_reg_field_set(unit, TOP_MMU_PLL1_CTRL3r, &rval, PLL_CTRL_PWM_RATEf, 0);
            SOC_IF_ERROR_RETURN(WRITE_TOP_MMU_PLL1_CTRL3r(unit, rval));
            SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL2_CTRL3r(unit, &rval));
            soc_reg_field_set(unit, TOP_MMU_PLL2_CTRL3r, &rval, PLL_CTRL_PWM_RATEf, 0);
            SOC_IF_ERROR_RETURN(WRITE_TOP_MMU_PLL2_CTRL3r(unit, rval));
            SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL3_CTRL3r(unit, &rval));
            soc_reg_field_set(unit, TOP_MMU_PLL3_CTRL3r, &rval, PLL_CTRL_PWM_RATEf, 0);
            SOC_IF_ERROR_RETURN(WRITE_TOP_MMU_PLL3_CTRL3r(unit, rval));
            break;
    }

    SOC_IF_ERROR_RETURN(READ_TOP_SWITCH_FEATURE_ENABLE_2r(unit, &rval));
    if (soc_reg_field_get(unit, TOP_SWITCH_FEATURE_ENABLE_2r, rval, BOND_1588_ENABLEf)) {
        SOC_FEATURE_SET(unit, soc_feature_timesync_support);
    }

    if (soc_reg_field_get(unit, TOP_SWITCH_FEATURE_ENABLE_2r, rval,
                          BOND_CES_ENABLEf)) {
        if ((rev_id != BCM56440_A0_REV_ID) &&
            (soc_property_get(unit, spn_CES_DISABLE, 0))) {
            /* Disable CES */
            SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_2r(unit, &rval));
            soc_reg_field_set(unit, TOP_MISC_CONTROL_2r, &rval,
                              BOND_CES_ENABLE_OVERRIDEf, 1);
            WRITE_TOP_MISC_CONTROL_2r(unit, rval);
        } else {
#if defined(INCLUDE_CES)
            SOC_FEATURE_SET(unit, soc_feature_ces);
#else
            if (rev_id != BCM56440_A0_REV_ID) {
                /* Disable CES */
                SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_2r(unit, &rval));
                soc_reg_field_set(unit, TOP_MISC_CONTROL_2r, &rval,
                                  BOND_CES_ENABLE_OVERRIDEf, 1);
                WRITE_TOP_MISC_CONTROL_2r(unit, rval);
            }
#endif
        }
    }

    /* Bring LCPLL out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_2r(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG0_PLL_RST_Lf,
                      1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG1_PLL_RST_Lf,
                      1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));

    sal_usleep(to_usec);

    /* Wait for LCPLL lock */
    if (!SAL_BOOT_SIMULATION) {
        SOC_IF_ERROR_RETURN(READ_TOP_XGXS0_PLL_STATUSr(unit, &rval));
        
        if (!soc_reg_field_get(unit, TOP_XGXS0_PLL_STATUSr, rval, TOP_XGPLL0_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "LCPLL 0 not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }
        if ((dev_id != BCM56240_DEVICE_ID) && (dev_id != BCM56241_DEVICE_ID) &&
            (dev_id != BCM56242_DEVICE_ID) && (dev_id != BCM56243_DEVICE_ID) &&
            (dev_id != BCM56245_DEVICE_ID) && (dev_id != BCM56246_DEVICE_ID) &&
            (dev_id != BCM56449_DEVICE_ID)) {
            /* Variants that don't use LCPLL-1 */
            SOC_IF_ERROR_RETURN(READ_TOP_XGXS1_PLL_STATUSr(unit, &rval));
            if (!soc_reg_field_get(unit, TOP_XGXS1_PLL_STATUSr, rval, TOP_XGPLL1_LOCKf)) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "LCPLL 1 not locked on unit %d "
                                      "status = 0x%08x\n"), unit, rval));
            }
         }
    }

    /* KT-1367: Timesync PLL 250MHz TS_CLK from 25MHz refclk */ 
    /* NDIV= 100(0x64) MDIV=10(0x1)            */   

    /* TSPLL configuration: 250MHz TS_CLK from 12.8/20/25/32MHz refclk */

    ts_ref_freq =  soc_property_get(unit, spn_PTP_TS_PLL_FREF, 0);

    for (ts_idx =0; ts_idx < (sizeof(katana_ts_pll)/sizeof(katana_ts_pll[0]));
        ++ts_idx) {
        if (katana_ts_pll[ts_idx].ref_freq == ts_ref_freq) {
            break;
        };
    }

    if (ts_idx >= (sizeof(katana_ts_pll)/sizeof(katana_ts_pll[0]))) {
        ts_idx = 4; /* wrong config, for now use internal 25MHZ reference */
        ts_ref_freq = 0;
    }


    /* Configure TS PLL */
    /* Set TS_PLL_CLK_IN_SEL based on reference frequency.  If it is 0, use the
     * internal reference */
    SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_2r(unit,&rval));
    soc_reg_field_set(unit, TOP_MISC_CONTROL_2r, &rval, TS_PLL_CLK_IN_SELf,
                      (ts_ref_freq != 0));
    WRITE_TOP_MISC_CONTROL_2r(unit, rval);
    
    SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_0r(unit,&rval));
    soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_0r, &rval, VCO_DIV2f,
                      1);  /* soc_property_get(unit, spn_TIME_SYNC_VCO_DIV2, 0)); */
    WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_0r(unit, rval);
    
    SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r(unit,&rval));
    soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r, &rval, KAf,
                      soc_property_get(unit, spn_PTP_TS_KA, katana_ts_pll[ts_idx].ka));
    soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r, &rval, KIf,
                      soc_property_get(unit, spn_PTP_TS_KI, katana_ts_pll[ts_idx].ki));
    soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r, &rval, KPf,
                      soc_property_get(unit, spn_PTP_TS_KP, katana_ts_pll[ts_idx].kp));
    WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r(unit, rval);
    
    SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r(unit,&rval));
    soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r, &rval, PDIVf,
                      soc_property_get(unit, spn_PTP_TS_PLL_PDIV,
                      katana_ts_pll[ts_idx].pdiv));
    WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r(unit, rval);
    SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r(unit,&rval));
    
    soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r, &rval, NDIV_INTf,
                      soc_property_get(unit, spn_PTP_TS_PLL_N,
                      katana_ts_pll[ts_idx].ndiv_int));
    soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r, &rval, NDIV_FRACf,
                      katana_ts_pll[ts_idx].ndiv_frac);
    WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r(unit, rval);
    
    SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_4r(unit,&rval));
    soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_4r, &rval, CH0_MDIVf,
                      soc_property_get(unit, spn_PTP_TS_PLL_MNDIV,
                      katana_ts_pll[ts_idx].mdiv));
    WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_4r(unit, rval);
    
    /* 250Mhz TS PLL implies 4ns resolution */
     SOC_TIMESYNC_PLL_CLOCK_NS(unit) = (1/250 * 1000); /* clock period in nanoseconds */

    /* Configure BS PLL */
    SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_2r(unit,&rval));

    /* assumption, 25MHz is from internal, not external, source */
    if (soc_property_get(unit, spn_PTP_BS_FREF, 25000000) == 250000000) {
        soc_reg_field_set(unit, TOP_MISC_CONTROL_2r, &rval, BS_PLL_CLK_IN_SELf,
                          0);
    } else {
        soc_reg_field_set(unit, TOP_MISC_CONTROL_2r, &rval, BS_PLL_CLK_IN_SELf,
                          1);
    }
    WRITE_TOP_MISC_CONTROL_2r(unit, rval);

    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC_PLL_CTRL_REGISTER_0r(unit,&rval));
    soc_reg_field_set(unit, TOP_BROAD_SYNC_PLL_CTRL_REGISTER_0r, &rval, VCO_DIV2f,
                      1);  /* soc_property_get(unit, spn_BROAD_SYNC_VCO_DIV2, 1)); */
    WRITE_TOP_BROAD_SYNC_PLL_CTRL_REGISTER_0r(unit, rval);

    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC_PLL_CTRL_REGISTER_1r(unit,&rval));
    soc_reg_field_set(unit, TOP_BROAD_SYNC_PLL_CTRL_REGISTER_1r, &rval, KAf,
                      soc_property_get(unit, spn_PTP_BS_KA, 2));
    soc_reg_field_set(unit, TOP_BROAD_SYNC_PLL_CTRL_REGISTER_1r, &rval, KIf,
                      soc_property_get(unit, spn_PTP_BS_KI, 4));
    soc_reg_field_set(unit, TOP_BROAD_SYNC_PLL_CTRL_REGISTER_1r, &rval, KPf,
                      soc_property_get(unit, spn_PTP_BS_KP, 9));
    WRITE_TOP_BROAD_SYNC_PLL_CTRL_REGISTER_1r(unit, rval);

    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC_PLL_CTRL_REGISTER_2r(unit,&rval));
    soc_reg_field_set(unit, TOP_BROAD_SYNC_PLL_CTRL_REGISTER_2r, &rval, PDIVf,
                      soc_property_get(unit, spn_PTP_BS_PDIV, 2));
    WRITE_TOP_BROAD_SYNC_PLL_CTRL_REGISTER_2r(unit, rval);

    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC_PLL_CTRL_REGISTER_3r(unit,&rval));
    soc_reg_field_set(unit, TOP_BROAD_SYNC_PLL_CTRL_REGISTER_3r, &rval, NDIV_INTf,
                      soc_property_get(unit, spn_PTP_BS_NDIV_INT, 320));
    soc_reg_field_set(unit, TOP_BROAD_SYNC_PLL_CTRL_REGISTER_3r, &rval, NDIV_FRACf,
                      0);
    WRITE_TOP_BROAD_SYNC_PLL_CTRL_REGISTER_3r(unit, rval);

    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC_PLL_CTRL_REGISTER_4r(unit,&rval));
    soc_reg_field_set(unit, TOP_BROAD_SYNC_PLL_CTRL_REGISTER_4r, &rval, CH0_MDIVf,
                      soc_property_get(unit, spn_PTP_BS_MNDIV, 200));
    WRITE_TOP_BROAD_SYNC_PLL_CTRL_REGISTER_4r(unit, rval);

    /* CES SETUP for external OCXO */
    if (soc_property_get(unit, spn_CES_PLL_REFERENCE_CLOCK_RATE, 25000000)==
    12800000){

      /* Configure CES PLL */
      SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_2r(unit,&rval));
      soc_reg_field_set(unit, TOP_MISC_CONTROL_2r, &rval, CES_PLL_CLK_IN_SELf,
            1);
      WRITE_TOP_MISC_CONTROL_2r(unit, rval);

      SOC_IF_ERROR_RETURN(READ_TOP_CES_PLL_CTRL_REGISTER_0r(unit,&rval));
      soc_reg_field_set(unit, TOP_CES_PLL_CTRL_REGISTER_0r, &rval, VCO_DIV2f,
            1);  
      WRITE_TOP_CES_PLL_CTRL_REGISTER_0r(unit, rval);

      SOC_IF_ERROR_RETURN(READ_TOP_CES_PLL_CTRL_REGISTER_1r(unit,&rval));
      soc_reg_field_set(unit, TOP_CES_PLL_CTRL_REGISTER_1r, &rval, KAf,
            1);
      soc_reg_field_set(unit, TOP_CES_PLL_CTRL_REGISTER_1r, &rval, KIf,
            1);
      soc_reg_field_set(unit, TOP_CES_PLL_CTRL_REGISTER_1r, &rval, KPf,
            5);
      WRITE_TOP_CES_PLL_CTRL_REGISTER_1r(unit, rval);


      SOC_IF_ERROR_RETURN(READ_TOP_CES_PLL_CTRL_REGISTER_2r(unit,&rval));
      soc_reg_field_set(unit, TOP_CES_PLL_CTRL_REGISTER_2r, &rval, PDIVf,
            4);
      WRITE_TOP_CES_PLL_CTRL_REGISTER_2r(unit, rval);

      SOC_IF_ERROR_RETURN(READ_TOP_CES_PLL_CTRL_REGISTER_3r(unit,&rval));
      soc_reg_field_set(unit, TOP_CES_PLL_CTRL_REGISTER_3r, &rval, NDIV_INTf,
            625);
      soc_reg_field_set(unit, TOP_CES_PLL_CTRL_REGISTER_3r, &rval, NDIV_FRACf,
            0);
      WRITE_TOP_CES_PLL_CTRL_REGISTER_3r(unit, rval);

      SOC_IF_ERROR_RETURN(READ_TOP_CES_PLL_CTRL_REGISTER_4r(unit,&rval));
      soc_reg_field_set(unit, TOP_CES_PLL_CTRL_REGISTER_4r, &rval, CH0_MDIVf,
            80);
      soc_reg_field_set(unit, TOP_CES_PLL_CTRL_REGISTER_4r, &rval, CH1_MDIVf,
            16);
      
      WRITE_TOP_CES_PLL_CTRL_REGISTER_4r(unit, rval);

      
    }
    



    /* De-assert LCPLL's post reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_2r(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG0_PLL_POST_RST_Lf,
                      1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_XG1_PLL_POST_RST_Lf,
                      1);
    if (soc_feature(unit, soc_feature_ces)) {
        soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_CES_PLL_RST_Lf,
                          1);
    }

    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_TS_PLL_RST_Lf,
                      1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_BS_PLL_RST_Lf,
                      1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));

    sal_usleep(to_usec);

    if (!SAL_BOOT_SIMULATION) {
        if (soc_feature(unit, soc_feature_ces)) {
            SOC_IF_ERROR_RETURN(READ_TOP_CES_PLL_STATUSr(unit, &rval));
            if (!soc_reg_field_get(unit, TOP_CES_PLL_STATUSr, rval, TOP_CES_PLL_LOCKf)) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "CES PLL not locked on unit %d "
                                      "status = 0x%08x\n"), unit, rval));
            }
        }
        SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_STATUSr(unit, &rval));
        if (!soc_reg_field_get(unit, TOP_TIME_SYNC_PLL_STATUSr, rval, 
                               TOP_TIME_SYNC_PLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "TimeSync PLL not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }

        SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC_PLL_STATUSr(unit, &rval));
        if (!soc_reg_field_get(unit, TOP_BROAD_SYNC_PLL_STATUSr, rval, 
                               TOP_BROAD_SYNC_PLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "BroadSync PLL not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }
    }

    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_2r(unit, &rval));
    if (soc_feature(unit, soc_feature_ces)) {
        soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_CES_PLL_POST_RST_Lf,
                          1);
    }
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_TS_PLL_POST_RST_Lf,
                      1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval, TOP_BS_PLL_POST_RST_Lf,
                      1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));

    sal_usleep(to_usec);
    
    /*
     * Bring port blocks out of reset
     */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ1_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ2_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ3_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_GP0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_GP1_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_GP2_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, rval));
    
    sal_usleep(to_usec);
    
    /* Bring network sync out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ0_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ1_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ2_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ3_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_NS_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, rval));
    
    sal_usleep(to_usec);
    
    SOC_IF_ERROR_RETURN(WRITE_CMIC_SBUS_TIMEOUTr(unit, 0x7d0));
    
    /* Bring IP, EP, and MMU blocks out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_EP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_IP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MMU_RST_Lf, 1);
    if (soc_feature(unit, soc_feature_ces)) {
        soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_CES_RST_Lf, 1);
    }
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, rval));
    
    sal_usleep(to_usec);

    if (((dev_id == BCM56445_DEVICE_ID) && (rev_id == BCM56445_A0_REV_ID)) ||
        ((dev_id == BCM56446_DEVICE_ID) && (rev_id == BCM56446_A0_REV_ID)) ||
        ((dev_id == BCM56447_DEVICE_ID) && (rev_id == BCM56447_A0_REV_ID)) ||
        ((dev_id == BCM56448_DEVICE_ID) && (rev_id == BCM56448_A0_REV_ID)) ) {
        _kt_lls_workaround(unit);
    }
    return SOC_E_NONE;
}
#endif  /* BCM_KATANA_SUPPORT */

#ifdef BCM_KATANA2_SUPPORT
/*
 * Function:
 *      soc_reset_bcm56450_a0
 * Purpose:
 *      Special reset sequencing for BCM56450
 *      Setup SBUS block mapping.
 */
STATIC int
soc_reset_bcm56450_a0(int unit)
{
    uint32 rval, to_usec;
    uint16 dev_id;
    uint8 rev_id;
    soc_reg_t reg;
    uint32 addr;
    int index;
    static const soc_reg_t pll_ctrl1_reg[] = {
        CMIC_XGXS0_PLL_CONTROL_1r, CMIC_XGXS1_PLL_CONTROL_1r,
        CMIC_XGXS2_PLL_CONTROL_1r, CMIC_XGXS3_PLL_CONTROL_1r
    };
    static const soc_reg_t pll_ctrl2_reg[] = {
        CMIC_XGXS0_PLL_CONTROL_2r, CMIC_XGXS1_PLL_CONTROL_2r,
        CMIC_XGXS2_PLL_CONTROL_2r, CMIC_XGXS3_PLL_CONTROL_2r
    };
    static const soc_reg_t pll_ctrl3_reg[] = {
        CMIC_XGXS0_PLL_CONTROL_3r, CMIC_XGXS1_PLL_CONTROL_3r,
        CMIC_XGXS2_PLL_CONTROL_3r, CMIC_XGXS3_PLL_CONTROL_3r
    };

    /* JIRA SDK-55492
     * New PLL settings will let VxWorks/KT2 crash when using "init all" cmd under BCM.0>
     */

    unsigned ts_ref_freq;
    unsigned ts_idx;
    static const soc_pll_param_t ts_pll[] = {  /* values for 500MHz TSPLL output */
      /*     Fref,  Ndiv_int, Ndiv_frac,  Pdiv, Mdiv, Ka, Ki, Kp, VCO_div2 */
        {12800000,       312,   0x80000,     1,    8,  1,  2,  7,        1},
        {20000000,       200,         0,     1,    8,  2,  3,  8,        1},
        {25000000,       160,         0,     1,    8,  2,  4,  9,        1},
        {32000000,       125,         0,     1,    8,  2,  4,  9,        1},
        {50000000,        80,         0,     1,    8,  3,  4, 10,        1},
        {       0,       160,         0,     1,    8,  2,  4,  9,        1}  /* 25MHz, from internal reference */
    };
    unsigned bs_ref_freq;
    unsigned bs_idx;
    uint32 bs_ndiv_high, bs_ndiv_low;
    static const soc_pll_param_t bs_pll[] = {  /* values for 20MHz BSPLL output */
      /*     Fref,  Ndiv_int,  Ndiv_frac,  Pdiv, Mdiv, Ka, Ki, Kp, VCO_div2 */
        {12800000,       250,          0,     1,  160,  1,  2,  7,        0},
        {20000000,       175,          0,     1,  175,  2,  3,  8,        0},
        {25000000,       140,          0,     1,  175,  2,  4,  9,        0},
        {32000000,       108, 0xc0000000,     1,  174,  2,  4,  9,        0}, /* Ndiv=108.75 */
        {50000000,        70,          0,     1,  175,  3,  4, 10,        0},
        {       0,       140,          0,     1,  175,  2,  4,  9,        0}  /* 25MHz, from internal reference */
    };
#if defined(BCM_HITLESS_RESET_SUPPORT)
    int initialize_broadsync;
    const int broadsync_default_ka = 4, broadsync_default_ki = 1, broadsync_default_kp = 8;
#endif /* BCM_HITLESS_RESET_SUPPORT */

    soc_cm_get_id(unit, &dev_id, &rev_id);

    to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                   (10 * MILLISECOND_USEC);

    /* Use 156.25Mhz reference clock for LCPLL */
    if (soc_property_get(unit, spn_XGXS_LCPLL_XTAL_REFCLK, 0)) {
        for (index = 0; index < 4; index++) {
            reg = pll_ctrl1_reg[index];
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            soc_pci_getreg(unit, addr, &rval);
            soc_reg_field_set(unit, reg, &rval, CH0_MDIVf, 20);
            soc_reg_field_set(unit, reg, &rval, CH3_MDIVf, 25);
            soc_reg_field_set(unit, reg, &rval, CH4_MDIVf, 125);
            soc_reg_field_set(unit, reg, &rval, CH5_MDIVf, 25);
            soc_pci_write(unit, addr, rval);

            reg = pll_ctrl3_reg[index];
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            soc_pci_getreg(unit, addr, &rval);
            soc_reg_field_set(unit, reg, &rval, NDIV_INTf, 40);
            soc_pci_write(unit, addr, rval);

            reg = pll_ctrl2_reg[index];
            addr = soc_reg_addr(unit, reg, REG_PORT_ANY, 0);
            soc_pci_getreg(unit, addr, &rval);
            soc_reg_field_set(unit, reg, &rval, PDIVf, 2);
            soc_pci_write(unit, addr, rval);
        }

        READ_CMIC_MISC_CONTROLr(unit, &rval);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL0_SW_OVWRf, 1);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL1_SW_OVWRf, 1);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL2_SW_OVWRf, 1);
        soc_reg_field_set(unit, CMIC_MISC_CONTROLr, &rval,
                          CMIC_TO_XG_PLL3_SW_OVWRf, 1);
        WRITE_CMIC_MISC_CONTROLr(unit, rval);
    }


    /*
     * SBUS ring and block number:
     * Ring0: cmic -> ip-ep -> cmic
     * Ring1: cmic -> mxqport0 -> mxqport1-> mxqport2 -> mxqport3-> cmic
     * Ring2: cmic -> gport0 -> gport1 -> gport2 -> cmic
     * Ring3: cmic -> otpc -> top -> cmic
     * Ring4: cmic -> mmu0 -> mmu1 -> cmic
     * Ring5: cmic -> ci0 -> ci1 -> ci2 -> lls -> cmic
     * Ring6: cmic -> ces -> cmic
     * Ring7: unused
     */
    WRITE_CMIC_SBUS_RING_MAP_0_7r(unit, 0x66034000); /* block 7  - 0 */
    WRITE_CMIC_SBUS_RING_MAP_8_15r(unit, 0x11112222); /* block 15 - 8 */
    WRITE_CMIC_SBUS_RING_MAP_16_23r(unit, 0x55555553); /* block 23 - 16 */
    WRITE_CMIC_SBUS_RING_MAP_24_31r(unit, 0x00637777); /* block 31 - 24 */

    /* Bring MMUPLL out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL_INITr(unit, &rval));
    soc_reg_field_set(unit, TOP_MMU_PLL_INITr, &rval,
                      PLL1_RESETBf, 1);
    soc_reg_field_set(unit, TOP_MMU_PLL_INITr, &rval,
                      PLL2_RESETBf, 1);
    soc_reg_field_set(unit, TOP_MMU_PLL_INITr, &rval,
                      PLL3_RESETBf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_MMU_PLL_INITr(unit, rval));

    sal_usleep(to_usec);

    /* Wait for MMUPLL lock */
    if (!SAL_BOOT_SIMULATION) {
        SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL_STATUS0r(unit, &rval));
        if (!soc_reg_field_get(unit, TOP_MMU_PLL_STATUS0r, rval, MMU_PLL1_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "MMUPLL 1 not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }
        if (!soc_reg_field_get(unit, TOP_MMU_PLL_STATUS0r, rval, MMU_PLL2_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "MMUPLL 2 not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }
        SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL_STATUS1r(unit, &rval));
        if (!soc_reg_field_get(unit, TOP_MMU_PLL_STATUS1r, rval, MMU_PLL3_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "MMUPLL 3 not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }
    }
    SOC_IF_ERROR_RETURN(READ_TOP_MMU_PLL_INITr(unit, &rval));
    soc_reg_field_set(unit, TOP_MMU_PLL_INITr, &rval,
                      PLL1_POST_RESETBf, 1);
    soc_reg_field_set(unit, TOP_MMU_PLL_INITr, &rval,
                      PLL2_POST_RESETBf, 1);
    soc_reg_field_set(unit, TOP_MMU_PLL_INITr, &rval,
                      PLL3_POST_RESETBf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_MMU_PLL_INITr(unit, rval));

    /* TSPLL configuration: 500MHz TS_CLK from 20/25/32/50MHz refclk */
    /* CMICd divides by 2 on input, so this is a 250MHz clock to TS logic */

    ts_ref_freq = soc_property_get(unit, spn_PTP_TS_PLL_FREF, 50000000);  /* 50MHz is default.  0->internal reference */
    /* Set TS_PLL_CLK_IN_SEL based on reference frequency.  If it is 0, use the internal reference */
    SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_1r(unit,&rval));
    soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, TS_PLL_CLK_IN_SELf,
                      (ts_ref_freq != 0));
    WRITE_TOP_MISC_CONTROL_1r(unit, rval);

    for (ts_idx = 0; ts_idx < sizeof(ts_pll)/sizeof(ts_pll[0]); ++ts_idx) {
        if (ts_pll[ts_idx].ref_freq == ts_ref_freq) {
            break;
        }
    }
    if (ts_idx == sizeof(ts_pll)/sizeof(ts_pll[0])) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "Invalid value for PTP_TS_PLL_REF (%u).  No default PLL params.\n"), ts_ref_freq));
        /* Could check for existence of SOC properties for VCO_DIV2, KA, KI, KP, NDIV_INT, NDIV_FRAC, MDIV, PDIV, and if
           all exist, use them.  For now, just fail.
        */
    } else {
        SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_0r(unit,&rval));
        soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_0r, &rval, VCO_DIV2f,
                          soc_property_get(unit, spn_PTP_TS_VCO_DIV2, ts_pll[ts_idx].vco_div2));
        WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_0r(unit, rval);
    
        SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r(unit,&rval));
        soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r, &rval, KAf,
                          soc_property_get(unit, spn_PTP_TS_KA, ts_pll[ts_idx].ka));
        soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r, &rval, KIf,
                          soc_property_get(unit, spn_PTP_TS_KI, ts_pll[ts_idx].ki));
        soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r, &rval, KPf,
                          soc_property_get(unit, spn_PTP_TS_KP, ts_pll[ts_idx].kp));
        WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_1r(unit, rval);
    
        SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r(unit,&rval));
        soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r, &rval, PDIVf,
                          soc_property_get(unit, spn_PTP_TS_PLL_PDIV, ts_pll[ts_idx].pdiv));
        WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_2r(unit, rval);

        SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r(unit,&rval));
        soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r, &rval, NDIV_INTf,
                          soc_property_get(unit, spn_PTP_TS_PLL_N, ts_pll[ts_idx].ndiv_int));
        soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r, &rval, NDIV_FRACf,
                          ts_pll[ts_idx].ndiv_frac);
        WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r(unit, rval);
    
        SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_4r(unit,&rval));
        soc_reg_field_set(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_4r, &rval, CH0_MDIVf,
                          soc_property_get(unit, spn_PTP_TS_PLL_MNDIV, ts_pll[ts_idx].mdiv));
        WRITE_TOP_TIME_SYNC_PLL_CTRL_REGISTER_4r(unit, rval);
    }

    /* 500Mhz TSPLL -> 250MHz ref at timestamper, implies 4ns resolution */
    SOC_TIMESYNC_PLL_CLOCK_NS(unit) = (1/250 * 1000); /* clock period in nanoseconds */

    /* Check to see if the BSPLLs have been configured yet.  If so, we leave them alone,
       for hitless reset support.
       Note: On KT2 A0/B0, the BroadSync Bitclock/Heartbeat regs get reset, so we still
       initialize all of BroadSync.
    */
    SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r(unit,&rval));

#if defined(BCM_HITLESS_RESET_SUPPORT)
    initialize_broadsync =
        (soc_reg_field_get(unit, TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r,
                           rval, KAf) == broadsync_default_ka
         && soc_reg_field_get(unit, TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r,
                              rval, KIf) == broadsync_default_ki
         && soc_reg_field_get(unit, TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r,
                              rval, KPf) == broadsync_default_kp);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "BSPLL regs:  %s\n"), initialize_broadsync ? "Default" : "Configured"));

    if (!initialize_broadsync) {
        /* JIRA CMICD-110
         * On A0/B0, BroadSync Bitclock/Heartbeat divisors/enable registers get reset,
         * though the outputs themselves do not if the registers were written to an even
         * number of times.  Workaround: when the registers are changed, the register
         * should be written twice, and the same value stored in SRAM.  This code will
         * restore the register from SRAM.
         */

        /* BS0 */
        uint32 reg_cache_base = soc_cmic_bs_reg_cache(unit, 0);

        uint32 config = soc_uc_mem_read(unit,
             reg_cache_base + offsetof(mos_msg_bs_reg_cache_t, config));
        uint32 clk_ctrl = soc_uc_mem_read(unit,
             reg_cache_base + offsetof(mos_msg_bs_reg_cache_t, clk_ctrl));
        uint32 heartbeat_ctrl = soc_uc_mem_read(unit,
             reg_cache_base + offsetof(mos_msg_bs_reg_cache_t, heartbeat_ctrl));
        uint32 heartbeat_down = soc_uc_mem_read(unit,
             reg_cache_base + offsetof(mos_msg_bs_reg_cache_t, heartbeat_down_duration));
        uint32 heartbeat_up = soc_uc_mem_read(unit,
             reg_cache_base + offsetof(mos_msg_bs_reg_cache_t, heartbeat_up_duration));

        if (config == 0 && clk_ctrl == 0 && heartbeat_ctrl == 0 && heartbeat_down == 0 && heartbeat_up == 0) {
            /* PLL was initialized, but there is no saved state.  Reset BroadSync. */
            initialize_broadsync = 1;
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "BSPLL Configured, but no BS state to restore\n")));
        } else {
            /* Double-writes, per CMICD-110 WAR */
            WRITE_CMIC_BS0_CONFIGr(unit, config);
            WRITE_CMIC_BS0_CONFIGr(unit, config);
            WRITE_CMIC_BS0_CLK_CTRLr(unit, clk_ctrl);
            WRITE_CMIC_BS0_CLK_CTRLr(unit, clk_ctrl);
            WRITE_CMIC_BS0_HEARTBEAT_CTRLr(unit, heartbeat_ctrl);
            WRITE_CMIC_BS0_HEARTBEAT_CTRLr(unit, heartbeat_ctrl);
            WRITE_CMIC_BS0_HEARTBEAT_DOWN_DURATIONr(unit, heartbeat_down);
            WRITE_CMIC_BS0_HEARTBEAT_DOWN_DURATIONr(unit, heartbeat_down);
            WRITE_CMIC_BS0_HEARTBEAT_UP_DURATIONr(unit, heartbeat_up);
            WRITE_CMIC_BS0_HEARTBEAT_UP_DURATIONr(unit, heartbeat_up);

            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "Restored BS0 regs:\n"
                                    "  config: %08x  clk_ctrl: %08x hb_ctrl: %08x\n"
                                    "  hb_up: %08x hb_down: %08x\n"),
                         config, clk_ctrl, heartbeat_ctrl,
                         heartbeat_down, heartbeat_up));

            /* same, but for BS1 */
            reg_cache_base = soc_cmic_bs_reg_cache(unit, 0);

            config = soc_uc_mem_read(unit,
                 reg_cache_base + offsetof(mos_msg_bs_reg_cache_t, config));
            clk_ctrl = soc_uc_mem_read(unit,
                 reg_cache_base + offsetof(mos_msg_bs_reg_cache_t, clk_ctrl));
            heartbeat_ctrl = soc_uc_mem_read(unit,
                 reg_cache_base + offsetof(mos_msg_bs_reg_cache_t, heartbeat_ctrl));
            heartbeat_down = soc_uc_mem_read(unit,
                 reg_cache_base + offsetof(mos_msg_bs_reg_cache_t, heartbeat_down_duration));
            heartbeat_up = soc_uc_mem_read(unit,
                 reg_cache_base + offsetof(mos_msg_bs_reg_cache_t, heartbeat_up_duration));

            WRITE_CMIC_BS1_CONFIGr(unit, config);
            WRITE_CMIC_BS1_CONFIGr(unit, config);
            WRITE_CMIC_BS1_CLK_CTRLr(unit, clk_ctrl);
            WRITE_CMIC_BS1_CLK_CTRLr(unit, clk_ctrl);
            WRITE_CMIC_BS1_HEARTBEAT_CTRLr(unit, heartbeat_ctrl);
            WRITE_CMIC_BS1_HEARTBEAT_CTRLr(unit, heartbeat_ctrl);
            WRITE_CMIC_BS1_HEARTBEAT_DOWN_DURATIONr(unit, heartbeat_down);
            WRITE_CMIC_BS1_HEARTBEAT_DOWN_DURATIONr(unit, heartbeat_down);
            WRITE_CMIC_BS1_HEARTBEAT_UP_DURATIONr(unit, heartbeat_up);
            WRITE_CMIC_BS1_HEARTBEAT_UP_DURATIONr(unit, heartbeat_up);

            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "Restored BS1 regs:\n"
                                    "  config: %08x  clk_ctrl: %08x hb_ctrl: %08x\n"
                                    "  hb_up: %08x hb_down: %08x\n"),
                         config, clk_ctrl, heartbeat_ctrl,
                         heartbeat_down, heartbeat_up));
        }
    }

    if (initialize_broadsync)
#endif  /* BCM_HITLESS_RESET_SUPPORT */
    {
        /* as a signal to upper-level code that the BroadSync is newly initialized
         * disable BroadSync0/1 bitclock output.  Checked in time.c / 1588 firmware    */
        SOC_IF_ERROR_RETURN(READ_CMIC_BS0_CONFIGr(unit, &rval));
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &rval, BS_CLK_OUTPUT_ENABLEf, 0);
        WRITE_CMIC_BS0_CONFIGr(unit, rval);

        SOC_IF_ERROR_RETURN(READ_CMIC_BS1_CONFIGr(unit, &rval));
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &rval, BS_CLK_OUTPUT_ENABLEf, 0);
        WRITE_CMIC_BS1_CONFIGr(unit, rval);

        /* BSPLL0 has not been configured, so reset/configure both BSPLL0 and BSPLL1 */
        SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_3r(unit, &rval));
        soc_reg_field_set(unit, TOP_SOFT_RESET_REG_3r, &rval, TOP_BS0_PLL_RST_Lf, 0);
        soc_reg_field_set(unit, TOP_SOFT_RESET_REG_3r, &rval, TOP_BS1_PLL_RST_Lf, 0);
        soc_reg_field_set(unit, TOP_SOFT_RESET_REG_3r, &rval, TOP_BS0_PLL_POST_RST_Lf, 0);
        soc_reg_field_set(unit, TOP_SOFT_RESET_REG_3r, &rval, TOP_BS1_PLL_POST_RST_Lf, 0);
        WRITE_TOP_SOFT_RESET_REG_3r(unit, rval);

        /* Both BSPLLs configured the same, for 20MHz output by default */

        bs_ref_freq = soc_property_get(unit, spn_PTP_BS_FREF, 50000000);  /* 50MHz is default.  0->internal reference */
        /* Set BS_PLL_CLK_IN_SEL based on reference frequency.  If it is 0, use the internal reference */
        SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_1r(unit,&rval));
        soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, BS0_PLL_CLK_IN_SELf,
                          (bs_ref_freq != 0));
        soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, BS1_PLL_CLK_IN_SELf,
                          (bs_ref_freq != 0));
        WRITE_TOP_MISC_CONTROL_1r(unit, rval);

        for (bs_idx = 0; bs_idx < sizeof(bs_pll)/sizeof(bs_pll[0]); ++bs_idx) {
            if (bs_pll[bs_idx].ref_freq == bs_ref_freq) {
                break;
            }
        }
        if (bs_idx == sizeof(bs_pll)/sizeof(bs_pll[0])) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "Invalid value for PTP_BS_REF (%u).  No default PLL params.\n"), bs_ref_freq));
            /* Could check for existence of SOC properties for VCO_DIV2, KA, KI, KP, NDIV_INT,
               NDIV_FRAC, MDIV, PDIV, and if all exist, use them.  For now, just fail.
            */
        } else {
            SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_3r(unit,&rval));
            soc_reg_field_set(unit, TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_3r, &rval, VCODIV2f,
                              soc_property_get(unit, spn_PTP_BS_VCO_DIV2, bs_pll[bs_idx].vco_div2));
            WRITE_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_3r(unit, rval);

            SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_3r(unit,&rval));
            soc_reg_field_set(unit, TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_3r, &rval, VCODIV2f,
                              soc_property_get(unit, spn_PTP_BS_VCO_DIV2, bs_pll[bs_idx].vco_div2));
            WRITE_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_3r(unit, rval);

            SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r(unit,&rval));
            soc_reg_field_set(unit, TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r, &rval, KAf,
                              soc_property_get(unit, spn_PTP_BS_KA, bs_pll[bs_idx].ka));
            soc_reg_field_set(unit, TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r, &rval, KIf,
                              soc_property_get(unit, spn_PTP_BS_KI, bs_pll[bs_idx].ki));
            soc_reg_field_set(unit, TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r, &rval, KPf,
                              soc_property_get(unit, spn_PTP_BS_KP, bs_pll[bs_idx].kp));
            soc_reg_field_set(unit, TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r, &rval, PDIVf,
                              soc_property_get(unit, spn_PTP_BS_PDIV, bs_pll[bs_idx].pdiv));
            WRITE_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_1r(unit, rval);

            SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_1r(unit,&rval));
            soc_reg_field_set(unit, TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_1r, &rval, KAf,
                              soc_property_get(unit, spn_PTP_BS_KA, bs_pll[bs_idx].ka));
            soc_reg_field_set(unit, TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_1r, &rval, KIf,
                              soc_property_get(unit, spn_PTP_BS_KI, bs_pll[bs_idx].ki));
            soc_reg_field_set(unit, TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_1r, &rval, KPf,
                              soc_property_get(unit, spn_PTP_BS_KP, bs_pll[bs_idx].kp));
            soc_reg_field_set(unit, TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_1r, &rval, PDIVf,
                              soc_property_get(unit, spn_PTP_BS_PDIV, bs_pll[bs_idx].pdiv));
            WRITE_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_1r(unit, rval);

            bs_ndiv_high = ((soc_property_get(unit, spn_PTP_BS_NDIV_INT, bs_pll[bs_idx].ndiv_int) << 8) |
                            ((soc_property_get(unit, spn_PTP_BS_NDIV_FRAC, bs_pll[bs_idx].ndiv_frac) >> 24) & 0xff));
            bs_ndiv_low = (soc_property_get(unit, spn_PTP_BS_NDIV_FRAC, bs_pll[bs_idx].ndiv_frac) << 8);

            SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r(unit,&rval));
            soc_reg_field_set(unit, TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r, &rval, BROAD_SYNC0_LCPLL_FBDIV_1f, bs_ndiv_high);
            WRITE_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_1r(unit, rval);

            SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_0r(unit,&rval));
            soc_reg_field_set(unit, TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_0r, &rval, BROAD_SYNC0_LCPLL_FBDIV_0f, bs_ndiv_low);
            WRITE_TOP_BROAD_SYNC0_LCPLL_FBDIV_CTRL_0r(unit, rval);

            SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r(unit,&rval));
            soc_reg_field_set(unit, TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r, &rval, BROAD_SYNC1_LCPLL_FBDIV_1f, bs_ndiv_high);
            WRITE_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_1r(unit, rval);

            SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_0r(unit,&rval));
            soc_reg_field_set(unit, TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_0r, &rval, BROAD_SYNC1_LCPLL_FBDIV_0f, bs_ndiv_low);
            WRITE_TOP_BROAD_SYNC1_LCPLL_FBDIV_CTRL_0r(unit, rval);

            SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_0r(unit,&rval));
            soc_reg_field_set(unit, TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_0r, &rval, CH1_MDIVf,
                              soc_property_get(unit, spn_PTP_BS_MNDIV, bs_pll[bs_idx].mdiv));
            WRITE_TOP_BROAD_SYNC0_PLL_CTRL_REGISTER_0r(unit, rval);

            SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_0r(unit,&rval));
            soc_reg_field_set(unit, TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_0r, &rval, CH1_MDIVf,
                              soc_property_get(unit, spn_PTP_BS_MNDIV, bs_pll[bs_idx].mdiv));
            WRITE_TOP_BROAD_SYNC1_PLL_CTRL_REGISTER_0r(unit, rval);
        }

        SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_3r(unit, &rval));
        soc_reg_field_set(unit, TOP_SOFT_RESET_REG_3r, &rval, TOP_BS0_PLL_RST_Lf, 1);
        soc_reg_field_set(unit, TOP_SOFT_RESET_REG_3r, &rval, TOP_BS1_PLL_RST_Lf, 1);
        WRITE_TOP_SOFT_RESET_REG_3r(unit, rval);

        /* JIRA CMICD-110:  See above logic to restore BroadSync Bitclock/Heartbeat. */
        /* Store default values in the SRAM, so they will be restored correctly */
        /* even if SRAM is not used */
#if defined(BCM_HITLESS_RESET_SUPPORT)
        {
            /* store reset values in SRAM for BS0 */
            uint32 reg_cache_base = soc_cmic_bs_reg_cache(unit, 0);
            READ_CMIC_BS0_CONFIGr(unit, &rval);
            soc_uc_mem_write(unit, reg_cache_base +
                             offsetof(mos_msg_bs_reg_cache_t, config), rval);
            READ_CMIC_BS0_CLK_CTRLr(unit, &rval);
            soc_uc_mem_write(unit, reg_cache_base +
                             offsetof(mos_msg_bs_reg_cache_t, clk_ctrl), rval);
            READ_CMIC_BS0_HEARTBEAT_CTRLr(unit, &rval);
            soc_uc_mem_write(unit, reg_cache_base +
                             offsetof(mos_msg_bs_reg_cache_t, heartbeat_ctrl), rval);
            READ_CMIC_BS0_HEARTBEAT_DOWN_DURATIONr(unit, &rval);
            soc_uc_mem_write(unit, reg_cache_base +
                             offsetof(mos_msg_bs_reg_cache_t, heartbeat_down_duration), rval);
            READ_CMIC_BS0_HEARTBEAT_UP_DURATIONr(unit, &rval);
            soc_uc_mem_write(unit, reg_cache_base +
                             offsetof(mos_msg_bs_reg_cache_t, heartbeat_up_duration), rval);

            /* store reset values in SRAM for BS1 */
            reg_cache_base = soc_cmic_bs_reg_cache(unit, 1);
            READ_CMIC_BS1_CONFIGr(unit, &rval);
            soc_uc_mem_write(unit, reg_cache_base +
                             offsetof(mos_msg_bs_reg_cache_t, config), rval);
            READ_CMIC_BS1_CLK_CTRLr(unit, &rval);
            soc_uc_mem_write(unit, reg_cache_base +
                             offsetof(mos_msg_bs_reg_cache_t, clk_ctrl), rval);
            READ_CMIC_BS1_HEARTBEAT_CTRLr(unit, &rval);
            soc_uc_mem_write(unit, reg_cache_base +
                             offsetof(mos_msg_bs_reg_cache_t, heartbeat_ctrl), rval);
            READ_CMIC_BS1_HEARTBEAT_DOWN_DURATIONr(unit, &rval);
            soc_uc_mem_write(unit, reg_cache_base +
                             offsetof(mos_msg_bs_reg_cache_t, heartbeat_down_duration), rval);
            READ_CMIC_BS1_HEARTBEAT_UP_DURATIONr(unit, &rval);
            soc_uc_mem_write(unit, reg_cache_base +
                             offsetof(mos_msg_bs_reg_cache_t, heartbeat_up_duration), rval);
        }
    } else {
        LOG_CLI((BSL_META_U(unit,
                            "Leaving BroadSync in holdover\n")));

        if (rev_id == BCM56450_A0_REV_ID) {
            /* WAR for reset of TOP_MISC_CONTROL_1.BSn_PLL_CLK_IN_SEL registers */
            /* Note that on A0 there will still be a "hit" if an external reference is used */
            /* But this prevents the wrong value from being persisted after a reset */
            bs_ref_freq = soc_property_get(unit, spn_PTP_BS_FREF, 50000000);  /* 50MHz is default.  0->internal reference */
            /* Reference frequency != 0 => use external reference */
            if (bs_ref_freq != 0) {
                SOC_IF_ERROR_RETURN(READ_TOP_MISC_CONTROL_1r(unit,&rval));
                soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, BS0_PLL_CLK_IN_SELf, 1);
                soc_reg_field_set(unit, TOP_MISC_CONTROL_1r, &rval, BS1_PLL_CLK_IN_SELf, 1);
                WRITE_TOP_MISC_CONTROL_1r(unit, rval);
            }
        }
#endif  /* BCM_HITLESS_RESET_SUPPORT */
    }

    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_2r(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval,
                      TOP_TS_PLL_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval,
                      TOP_DDR_PLL0_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));

    sal_usleep(to_usec);

    if (!SAL_BOOT_SIMULATION) {
        SOC_IF_ERROR_RETURN(READ_TOP_TIME_SYNC_PLL_STATUSr(unit, &rval));
        if (!soc_reg_field_get(unit, TOP_TIME_SYNC_PLL_STATUSr, rval, 
                               TOP_TIME_SYNC_PLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "TimeSync PLL not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }

        SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC0_PLL_STATUSr(unit, &rval));
        if (!soc_reg_field_get(unit, TOP_BROAD_SYNC0_PLL_STATUSr, rval,
                               TOP_BROAD_SYNC0_PLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "BroadSync0 PLL not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }

        SOC_IF_ERROR_RETURN(READ_TOP_BROAD_SYNC1_PLL_STATUSr(unit, &rval));
        if (!soc_reg_field_get(unit, TOP_BROAD_SYNC1_PLL_STATUSr, rval,
                               TOP_BROAD_SYNC1_PLL_LOCKf)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "BroadSync1 PLL not locked on unit %d "
                                  "status = 0x%08x\n"), unit, rval));
        }

        if (dev_id != BCM56456_DEVICE_ID) {
            SOC_IF_ERROR_RETURN(READ_TOP_DDR_PLL0_STATUSr(unit, &rval));
            if (!soc_reg_field_get(unit, TOP_DDR_PLL0_STATUSr, rval, 
                                   DDR_PLL0_LOCKf)) {
                LOG_ERROR(BSL_LS_SOC_COMMON,
                          (BSL_META_U(unit,
                                      "DDR PLL not locked on unit %d "
                                      "status = 0x%08x\n"), unit, rval));
            }
        }
    }

#if defined(BCM_HITLESS_RESET_SUPPORT)
    if (initialize_broadsync)
#endif  /* BCM_HITLESS_RESET_SUPPORT */
    {
        SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_3r(unit, &rval));
        soc_reg_field_set(unit, TOP_SOFT_RESET_REG_3r, &rval, TOP_BS0_PLL_POST_RST_Lf, 1);
        soc_reg_field_set(unit, TOP_SOFT_RESET_REG_3r, &rval, TOP_BS1_PLL_POST_RST_Lf, 1);
        WRITE_TOP_SOFT_RESET_REG_3r(unit, rval);
    }

    /* Check for M, A and Plain versions */
    SOC_IF_ERROR_RETURN(READ_TOP_SWITCH_FEATURE_ENABLE_1r(unit, &rval));
    if (rval & 0x10000000) {
        /* BOND_1588_ENABLE */
        SOC_FEATURE_SET(unit, soc_feature_timesync_support);
        LOG_CLI((BSL_META_U(unit,
                            "BOND_1588_ENABLE : "
                            "enabling soc_feature_timesync_support\n")));
    }
	
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REG_2r(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval,
                      TOP_TS_PLL_POST_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &rval,
                      TOP_DDR_PLL0_POST_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REG_2r(unit, rval));

    sal_usleep(to_usec);
    LOG_CLI((BSL_META_U(unit,
                        "###################################################\n")));
    if (SOC_INFO(unit).olp_port[0]) {
        LOG_CLI((BSL_META_U(unit,
                            "OLP PORT IS *USED* \n")));
    } else {
        LOG_CLI((BSL_META_U(unit,
                            "OLP PORT IS *NOT USED* \n")));
        SOC_IF_ERROR_RETURN(READ_TOP_SW_BOND_OVRD_CTRL0r(unit, &rval));
        soc_reg_field_set(unit,TOP_SW_BOND_OVRD_CTRL0r,&rval,
                          OLP_ENABLEf,0);
        SOC_IF_ERROR_RETURN(WRITE_TOP_SW_BOND_OVRD_CTRL0r(unit, rval));
    }
    LOG_CLI((BSL_META_U(unit,
                        "###################################################\n")));
    
    /*
     * Bring port blocks out of reset
     */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ0_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ1_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ2_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ3_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ4_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ5_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ6_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ7_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ8_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ9_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ10_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, rval));
    
    sal_usleep(to_usec);
    
    /* Bring network sync out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ0_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ1_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ2_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ3_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ4_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ5_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ6_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ7_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ8_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ9_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MXQ10_HOTSWAP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_NS_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, rval));
    
    sal_usleep(to_usec);
    
    SOC_IF_ERROR_RETURN(WRITE_CMIC_SBUS_TIMEOUTr(unit, 0x7d0));

    /* enable packing mode based on soc propoerty before mmu is out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SW_BOND_OVRD_CTRL1r(unit, &rval));
    soc_reg_field_set(unit, TOP_SW_BOND_OVRD_CTRL1r, &rval, MMU_PACKING_ENABLEf,
                      soc_property_get(unit, spn_MMU_MULTI_PACKETS_PER_CELL, 0));
    SOC_IF_ERROR_RETURN(WRITE_TOP_SW_BOND_OVRD_CTRL1r(unit, rval));

    /* Bring IP, EP, and MMU blocks out of reset */
    SOC_IF_ERROR_RETURN(READ_TOP_SOFT_RESET_REGr(unit, &rval));
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_EP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_IP_RST_Lf, 1);
    soc_reg_field_set(unit, TOP_SOFT_RESET_REGr, &rval, TOP_MMU_RST_Lf, 1);
    SOC_IF_ERROR_RETURN(WRITE_TOP_SOFT_RESET_REGr(unit, rval));
    
    /* Bring AMAC out of reset */
    soc_amac_init(unit);

    sal_usleep(to_usec);
    return SOC_E_NONE;
}
#endif  /* BCM_KATANA2_SUPPORT */


/*
 * Function:
 *      soc_reset
 * Purpose:
 *      Reset the unit via the CMIC CPS reset function.
 */

int
soc_reset(int unit)
{
    uint16      dev_id;
    uint8       rev_id;
    int         spl;

    if (SOC_IS_RCPU_ONLY(unit)) {
        /* Skip reset process for rcpu only unit */
        return SOC_E_NONE;
    }

    /*
     * Configure endian mode in case the system just came out of reset.
     */

    soc_endian_config(unit);
    soc_pci_ep_config(unit, -1);

    /*
     * Configure bursting in case the system just came out of reset.
     */

    soc_pci_burst_enable(unit);


    /*
     * After setting the reset bit, StrataSwitch PCI registers cannot be
     * accessed for 300 cycles or the CMIC will hang.  This is mostly of
     * concern on the Quickturn simulation but we leave plenty of time.
     */

    soc_cm_get_id(unit, &dev_id, &rev_id);

    /* Suspend IRQ polling while resetting */
    soc_ipoll_pause(unit);

    if (soc_property_get(unit, spn_SOC_SKIP_RESET, 0)) {
        LOG_WARN(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "soc_init: skipping hard reset\n")));
    }
    else
    {
        /* CPS reset problematic on PLI, the amount of time
         * to wait is not constant and may vary from machine
         * to machine. So, don't do it.
         */
        if (!SAL_BOOT_RTLSIM) {

#ifdef BCM_HERCULES_SUPPORT
#ifdef BCM_HERCULES15_SUPPORT
            /* special processing on 5675/6A0 with disabled ports */
            if (SOC_IS_HERCULES15(unit) &&
                rev_id == BCM5675_A0_REV_ID &&
                !SAL_BOOT_QUICKTURN &&
                SOC_PBMP_WORD_GET(PBMP_ALL(unit), 0) != 0x1ff) {
                soc_reset_bcm5675_a0(unit, dev_id);
            }
#endif
            if (SOC_IS_HERCULES(unit)) {
                /* Set clock enable on active ports before soft reset */
                WRITE_CMIC_CLK_ENABLEr(unit,
                        SOC_PBMP_WORD_GET(PBMP_ALL(unit), 0));
            }
#endif
            /* Block interrupts while setting the busy flag */
            spl = sal_splhi();
            SOC_CONTROL(unit)->soc_flags |= SOC_F_BUSY;

            /* Unblock interrupts */
            sal_spl(spl);

#ifdef BCM_CMICM_SUPPORT
            if (soc_feature(unit, soc_feature_cmicm)) {
                soc_pci_write(unit, CMIC_CPS_RESET_OFFSET, 1);
            } else
#endif
            {
                soc_pci_write(unit, CMIC_CONFIG,
                              soc_pci_read(unit, CMIC_CONFIG) | CC_RESET_CPS);
            }

            if (soc_feature(unit, soc_feature_reset_delay) && !SAL_BOOT_QUICKTURN) {
                sal_usleep(1000000);
            } else {
                if (SAL_BOOT_QUICKTURN) {
                    sal_usleep(10 * MILLISECOND_USEC);
                } else {
                    sal_usleep(1 * MILLISECOND_USEC);
                }
            }
        }
    }

    /*
     * Sleep some extra time to allow the StrataSwitch chip to finish
     * initializing the ARL tables, which takes 60000 to 70000 cycles.
     * On PLI, sleeping 10us won't make a difference.
     */
    if (SAL_BOOT_QUICKTURN) {
        sal_usleep(250 * MILLISECOND_USEC);
    } else {
        sal_usleep(10 * MILLISECOND_USEC);
    }

    /* On some boards that run at lower freq, reading the CMIC_CONFIG
     * register for the first time after CPS reset returns 0. The SW
     * workaround is to perform an extra read.
     * Eventhough Hu is the only (first) board with this issue, this
     * could be a common Workaround..
     */
    if (!soc_feature(unit, soc_feature_cmicm)) {
        soc_pci_read(unit, CMIC_CONFIG);
        sal_usleep(1000);
    }

    /* Restore endian mode since the reset cleared it. */

    soc_endian_config(unit);
    soc_pci_ep_config(unit, -1);

    /* Restore bursting */

    soc_pci_burst_enable(unit);


    /* Synchronize cached interrupt mask */
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        uint32 addr, rval;

        addr = CMIC_CMCx_PCIE_MISCEL_OFFSET(SOC_PCI_CMC(unit));
        rval = soc_pci_read(unit, addr);
        
#ifdef BCM958525
        /* Disable MSI for RSX cpu boards */
        soc_reg_field_set(unit, CMIC_CMC0_PCIE_MISCELr,
                            &rval, ENABLE_MSIf, 0);
#else
        soc_reg_field_set(unit, CMIC_CMC0_PCIE_MISCELr,
                            &rval, ENABLE_MSIf, 1);
#endif

        soc_pci_write(unit, addr, rval);

        soc_cmicm_intr0_disable(unit, ~0);
        soc_cmicm_intr1_disable(unit, ~0);
        soc_cmicm_intr2_disable(unit, ~0);
    } else
#endif
    {
        soc_intr_disable(unit, ~0);
    }

    /* Block interrupts */
    spl = sal_splhi();
    SOC_CONTROL(unit)->soc_flags &= ~SOC_F_BUSY;
    /* Unblock interrupts */
    sal_spl(spl);

    /* Resume IRQ polling if active */
    soc_ipoll_continue(unit);

#ifdef BCM_FIREBOLT_SUPPORT
#ifdef BCM_RAPTOR_SUPPORT
    if (SOC_IS_RAPTOR(unit)) {
        soc_reset_bcm56218_a0(unit);
    } else
#endif /* BCM_RAPTOR_SUPPORT */
#ifdef BCM_RAVEN_SUPPORT
    if (SOC_IS_RAVEN(unit)) {
        soc_reset_bcm56224_a0(unit);
    } else
#endif
#ifdef BCM_HAWKEYE_SUPPORT
    if (SOC_IS_HAWKEYE(unit)) {
        soc_reset_bcm53314_a0(unit);
    } else
#endif
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
        SOC_IS_VALKYRIE2(unit)) {
        soc_reset_bcm56634_a0(unit);
    } else
#endif
#ifdef BCM_ENDURO_SUPPORT
    if (SOC_IS_ENDURO(unit)) {
        soc_reset_bcm56334_a0(unit);
    } else
#endif
#ifdef BCM_HURRICANE_SUPPORT
    if (SOC_IS_HURRICANE(unit)) {
        soc_reset_bcm56142_a0(unit);
    } else
#endif
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        SOC_IF_ERROR_RETURN(soc_reset_bcm56150_a0(unit));
    } else
#endif
#ifdef BCM_GREYHOUND_SUPPORT
    if (SOC_IS_GREYHOUND(unit)) {
        SOC_IF_ERROR_RETURN(soc_greyhound_chip_reset(unit));
    } else
#endif
#ifdef BCM_HELIX4_SUPPORT
    if (SOC_IS_HELIX4(unit)) {
        SOC_IF_ERROR_RETURN(soc_reset_bcm56340_a0(unit));
    } else
#endif
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        SOC_IF_ERROR_RETURN(soc_reset_bcm56640_a0(unit));
    } else
#endif
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        SOC_IF_ERROR_RETURN(soc_trident2_chip_reset(unit));
    } else
#endif
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        SOC_IF_ERROR_RETURN(soc_reset_bcm56840_a0(unit));
    } else
#endif
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(unit)) {
        SOC_IF_ERROR_RETURN(soc_reset_bcm56440_a0(unit));
    } else
#endif
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        SOC_IF_ERROR_RETURN(soc_reset_bcm56450_a0(unit));
    } else
#endif
#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_IS_TR_VL(unit)) {
        soc_reset_bcm56624_a0(unit);
    } else
#endif
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        uint32 rval = 0;
        soc_reg_field_set(unit, CMIC_EB3_VLI_CONFIG_REGISTERr, &rval, 
                          EB3_DATA_PARITY_ENABLEf, 1);
        soc_reg_field_set(unit, CMIC_EB3_VLI_CONFIG_REGISTERr, &rval, 
                          EB3_ADDR_PARITY_ENABLEf, 1);
        WRITE_CMIC_EB3_VLI_CONFIG_REGISTERr(unit, rval);  
        soc_reset_bcm88732_a0(unit);
    } else
#endif
#ifdef BCM_SCORPION_SUPPORT
    if (SOC_IS_SC_CQ(unit)) {
        soc_reset_bcm56820_a0(unit);
    } else
#endif
    if (SOC_IS_FB_FX_HX(unit)) {
        soc_reset_bcm56504_a0(unit);
    } else
#if defined (BCM_BRADLEY_SUPPORT)
    if (SOC_IS_HB_GW(unit)) {
        soc_reset_bcm56800_a0(unit);
    }
#endif /* BCM_BRADLEY_SUPPORT */
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        soc_port_t port;
        /*
         * For H/W linkscan
         */
#if defined (BCM_CMICM_SUPPORT)
        if (soc_feature(unit, soc_feature_cmicm) &&
            !soc_property_get(unit, spn_MDIO_EXTERNAL_MASTER, 0)) {
            uint32 rval;
            READ_CMIC_MIIM_SCAN_CTRLr(unit, &rval);
            soc_reg_field_set(unit, CMIC_MIIM_SCAN_CTRLr, &rval,
                              OVER_RIDE_EXT_MDIO_MSTR_CNTRLf, 1);
            WRITE_CMIC_MIIM_SCAN_CTRLr(unit, rval);
        }
#endif
        if (!SOC_IS_SC_CQ(unit) && !SOC_IS_TD_TT(unit) &&
            !soc_feature(unit, soc_feature_cmicm)) {
            WRITE_CMIC_MIIM_PORT_TYPE_MAPr(unit,
                        SOC_PBMP_WORD_GET(PBMP_HG_ALL(unit), 0) |
                        SOC_PBMP_WORD_GET(PBMP_XE_ALL(unit), 0));
        }
        if (SAL_BOOT_QUICKTURN) {
            /* Kick the QT phy model */
            PBMP_GE_ITER(unit, port) {
                if (SOC_IS_SCORPION(unit)) {
                    SOC_IF_ERROR_RETURN
                        (soc_miim_write(unit, port - 24, 0x00, 0x140));
                } else {
                    if (SOC_IS_HELIX4(unit)) {
                        SOC_IF_ERROR_RETURN
                            (soc_miim_write(unit, port, 0x00, 0xFFFF));
                    } else {
                        SOC_IF_ERROR_RETURN
                            (soc_miim_write(unit, port, 0x00, 0x140));
                    }
                }
            }
            PBMP_FE_ITER(unit, port) {
                SOC_IF_ERROR_RETURN(soc_miim_write(unit, port, 0x00, 0x2100));
            }
        }
    }
#endif /* BCM_XGS3_SWITCH_SUPPORT */

#ifdef BCM_BIGMAC_SUPPORT
#ifdef BCM_SCORPION_SUPPORT
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SC_CQ(unit) && (!SOC_IS_SHADOW(unit))) {
#else /* BCM_SHADOW_SUPPORT */
    if (SOC_IS_SC_CQ(unit)) {
#endif
        soc_port_t port;
        soc_pbmp_t qg_pbmp;
        uint32 val, to_usec;

        to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                       (10 * MILLISECOND_USEC);

        /* Bring Unicore out of reset */
        PBMP_GX_ITER(unit, port) {
            _soc_xgxs_mdio_setup(unit, port);
            SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));
            val = 0;
            soc_reg_field_set(unit, XPORT_XGXS_NEWCTL_REGr,
                              &val, TXD1G_FIFO_RSTBf, 0xf);
            SOC_IF_ERROR_RETURN
                (WRITE_XPORT_XGXS_NEWCTL_REGr(unit, port, val));
        }
        SOC_PBMP_ASSIGN(qg_pbmp, PBMP_GE_ALL(unit));
        SOC_PBMP_REMOVE(qg_pbmp, PBMP_GX_ALL(unit));
        SOC_PBMP_ITER(qg_pbmp, port) {
            _soc_xgxs_mdio_setup(unit, port);
            SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));
            break;
        }

        /* Bring remaining blocks out of reset */
        READ_CMIC_SOFT_RESET_REGr(unit, &val);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf,
                          1);
        WRITE_CMIC_SOFT_RESET_REGr(unit, val);
        sal_usleep(to_usec);
    } else
#endif /* BCM_SCORPION_SUPPORT */
#ifdef BCM_TRIUMPH_SUPPORT
    if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
        soc_reg_t reg;
        int port, hc_idx;
        int hc_port[6]; /* any port in the hyperlite block */
        uint32 txd1g_map[6], val;

        /* bring unicore out of reset */
        PBMP_GX_ITER(unit, port) {
            _soc_xgxs_mdio_setup(unit, port);
            SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));
            val = 0;
            soc_reg_field_set(unit, XPORT_XGXS_NEWCTL_REGr,
                              &val, TXD1G_FIFO_RSTBf, 0xf);
            SOC_IF_ERROR_RETURN
                (WRITE_XPORT_XGXS_NEWCTL_REGr(unit, port, val));
        }

        /* bring hyperlite out of reset */
        for (hc_idx = 0; hc_idx < 6; hc_idx++) {
            hc_port[hc_idx] = -1;
            txd1g_map[hc_idx] = 0;
        }
        PBMP_XG_ITER(unit, port) {
            switch (tr_xgxs_map[port].mdio_conf_reg) {
            case CMIC_XGXS_MDIO_CONFIG_0r: hc_idx = 0; break;
            case CMIC_XGXS_MDIO_CONFIG_1r: hc_idx = 1; break;
            case CMIC_XGXS_MDIO_CONFIG_2r: hc_idx = 2; break;
            case CMIC_XGXS_MDIO_CONFIG_3r: hc_idx = 3; break;
            case CMIC_XGXS_MDIO_CONFIG_4r: hc_idx = 4; break;
            case CMIC_XGXS_MDIO_CONFIG_5r: hc_idx = 5; break;
            default: continue;
            }
            if (hc_port[hc_idx] == -1) {
                hc_port[hc_idx] = port;
            }
            if (!IS_XE_PORT(unit, port) && !IS_HG_PORT(unit, port)) {
                txd1g_map[hc_idx] |= 1 << tr_xgxs_map[port].hc_chan;
            } else {
                txd1g_map[hc_idx] = 0x0f;
            }
        }
        for (hc_idx = 0; hc_idx < 6; hc_idx++) {
            if (hc_port[hc_idx] == -1) {
                continue;
            }
            port = hc_port[hc_idx];
            _soc_xgxs_mdio_setup(unit, port);
            SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));
            reg = hc_idx != 1 && hc_idx != 4 ? XGPORT_XGXS_NEWCTL_REGr :
                XGPORT_EXTRA_XGXS_NEWCTL_REGr;
            val = 0;
            soc_reg_field_set(unit, reg, &val, TXD1G_FIFO_RSTBf,
                              txd1g_map[hc_idx]);
            SOC_IF_ERROR_RETURN(soc_reg32_set(unit, reg, port, 0, val));
        }
    } else
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
        SOC_IS_VALKYRIE2(unit)) {
        int port, hc_idx;
        int hc_port[6]; /* any port in the hyperlite block */
        uint32 txd1g_map[6], val;

        /* bring unicore out of reset */
        PBMP_GX_ITER(unit, port) {
            if (IS_XQ_PORT(unit, port)) {
                /* Register does not exist for XQ ports */
                continue;
            }
            _soc_xgxs_mdio_setup(unit, port);
            SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));
            val = 0;
            soc_reg_field_set(unit, XPORT_XGXS_NEWCTL_REGr,
                              &val, TXD1G_FIFO_RSTBf, 0xf);
            SOC_IF_ERROR_RETURN
                (WRITE_XPORT_XGXS_NEWCTL_REGr(unit, port, val));
        }

        /* bring hyperlite out of reset */
        for (hc_idx = 0; hc_idx < 6; hc_idx++) {
            hc_port[hc_idx] = -1;
            txd1g_map[hc_idx] = 0;
        }
        PBMP_XQ_ITER(unit, port) {
            hc_idx = (port - 30) / 4;

            if (hc_port[hc_idx] == -1) {
                hc_port[hc_idx] = port;
            }
            if (!IS_XE_PORT(unit, port) && !IS_HG_PORT(unit, port)) {
                txd1g_map[hc_idx] |= 1 << ((port - 30) & 0x3);
            } else {
                txd1g_map[hc_idx] = 0x0f;
            }
        }
        for (hc_idx = 0; hc_idx < 6; hc_idx++) {
            if (hc_port[hc_idx] == -1) {
                continue;
            }
            port = hc_port[hc_idx];
            _soc_xgxs_mdio_setup(unit, port);
            SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));
            val = 0;
            soc_reg_field_set(unit, XQPORT_XGXS_NEWCTL_REGr, &val,
                              TXD1G_FIFO_RSTBf, txd1g_map[hc_idx]);
            SOC_IF_ERROR_RETURN(WRITE_XQPORT_XGXS_NEWCTL_REGr(unit, port,
                                                              val));
        }
    } else
#endif /* BCM_TRIUMPH2_SUPPORT */
#ifdef BCM_BRADLEY_SUPPORT
    if (SOC_IS_HB_GW(unit)) {
        soc_port_t port;
        uint32 val, to_usec;

        to_usec = SAL_BOOT_QUICKTURN ? (250 * MILLISECOND_USEC) :
                                       (10 * MILLISECOND_USEC);

        /* Bring Unicore out of reset */
        PBMP_GX_ITER(unit, port) {
            SOC_IF_ERROR_RETURN(soc_unicore_reset(unit, port));
        }

        /* Bring remaining blocks out of reset */
        READ_CMIC_SOFT_RESET_REGr(unit, &val);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_BSAFE_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_IP_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_EP_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_MMU_RST_Lf, 1);
        soc_reg_field_set(unit, CMIC_SOFT_RESET_REGr, &val, CMIC_XP_RST_Lf, 1);
        WRITE_CMIC_SOFT_RESET_REGr(unit, val);
        sal_usleep(to_usec);
    } else
#endif /* BCM_BRADLEY_SUPPORT */
#ifdef BCM_ENDURO_SUPPORT
    if (SOC_IS_ENDURO(unit)) {
        soc_port_t port;
        uint32 val;

        /* bring unicore out of reset */        
        PBMP_XQ_ITER(unit, port) {
            _soc_xgxs_mdio_setup(unit, port);
            SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));
            val = 0;
            soc_reg_field_set(unit, XQPORT_XGXS_NEWCTL_REGr, &val,
                              TXD1G_FIFO_RSTBf, 0xf);
            SOC_IF_ERROR_RETURN(WRITE_XQPORT_XGXS_NEWCTL_REGr(unit, port, val));
        }
    } else
#endif /* BCM_ENDURO_SUPPORT */
#ifdef BCM_SHADOW_SUPPORT
    if (SOC_IS_SHADOW(unit)) {
        /* setup xgxs mdio */
        /* reset xgxs */
        soc_info_t *si;
        soc_port_t  port;
        int         blk;
        uint32 rval;

        si = &SOC_INFO(unit);
        SOC_BLOCK_ITER(unit, blk, SOC_BLK_XLPORT) {
            port = SOC_BLOCK_PORT(unit, blk);
            _soc_xgxs_mdio_setup(unit, port);
            SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));

            /* Bring XMAC out of reset */
            if (si->port_speed_max[port] < 10000) {
                continue;
            }
            SOC_IF_ERROR_RETURN(READ_XLPORT_XMAC_CONTROLr(unit, port, &rval));
            soc_reg_field_set(unit, XLPORT_XMAC_CONTROLr, &rval, XMAC_RESETf,
                              1);
            SOC_IF_ERROR_RETURN(WRITE_XLPORT_XMAC_CONTROLr(unit, port, rval));
            sal_udelay(10);

            soc_reg_field_set(unit, XLPORT_XMAC_CONTROLr, &rval, XMAC_RESETf,
                              0);
            SOC_IF_ERROR_RETURN(WRITE_XLPORT_XMAC_CONTROLr(unit, port, rval));
        }
    } else
#endif /* BCM_SHADOW_SUPPORT */
#ifdef BCM_HURRICANE_SUPPORT
    if (SOC_IS_HURRICANE(unit)) {
        soc_port_t port;
        uint32 val;

        /* bring unicore out of reset */        
        PBMP_XQ_ITER(unit, port) {
            _soc_xgxs_mdio_setup(unit, port);
            SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));
            val = 0;
            soc_reg_field_set(unit, XQPORT_XGXS_NEWCTL_REGr, &val,
                              TXD1G_FIFO_RSTBf, 0xf);
            SOC_IF_ERROR_RETURN(WRITE_XQPORT_XGXS_NEWCTL_REGr(unit, port, val));
        }
    } else
#endif /* BCM_HURRICANE_SUPPORT */
#ifdef BCM_HURRICANE2_SUPPORT
    if (SOC_IS_HURRICANE2(unit)) {
        SOC_IF_ERROR_RETURN(soc_hurricane2_tsc_reset(unit));
    } else
#endif /* BCM_HURRICANE2_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        SOC_IF_ERROR_RETURN(soc_trident2_tsc_reset(unit));
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        soc_info_t *si;
        int rv, blk, port, phy_port, saved_phy_port, saved_port_type;
        uint32 rval;

        si = &SOC_INFO(unit);

        /* Toggle unused Warpcore */
        port = 1; /* borrow logical port 1 for the unused block */
        saved_phy_port = si->port_l2p_mapping[port];
        saved_port_type = si->port_type[port];
        si->port_type[port] = SOC_BLK_XLPORT;
        for (blk = 0; SOC_BLOCK_TYPE(unit, blk) >= 0; blk++) {
            if (SOC_BLOCK_TYPE(unit, blk) != SOC_BLK_XLPORT) {
                continue;
            }
            if (SOC_INFO(unit).block_valid[blk]) {
                continue;
            }
            for (phy_port = 0; ; phy_port++) {
                if (blk == SOC_PORT_BLOCK(unit, phy_port)) {
                    break;
                }
                if (SOC_PORT_BLOCK(unit, phy_port) < 0 &&
                    SOC_PORT_BINDEX(unit, phy_port) < 0) {
                    phy_port = -1;
                    break;
                }
            }
            if (phy_port >= 0) {
                si->port_l2p_mapping[port] = phy_port;
                rv = soc_wc_xgxs_reset(unit, port, 0);
                if (SOC_FAILURE(rv)) {
                    break;
                }
            }
        }
        for (blk = 0; SOC_BLOCK_TYPE(unit, blk) >= 0; blk++) {
            if (SOC_BLOCK_TYPE(unit, blk) != SOC_BLK_XLPORT) {
                continue;
            }
            if (SOC_INFO(unit).block_valid[blk]) {
                continue;
            }
            for (phy_port = 0; ; phy_port++) {
                if (blk == SOC_PORT_BLOCK(unit, phy_port)) {
                    break;
                }
                if (SOC_PORT_BLOCK(unit, phy_port) < 0 &&
                    SOC_PORT_BINDEX(unit, phy_port) < 0) {
                    phy_port = -1;
                    break;
                }
            }
            if (phy_port >= 0) {
                si->port_l2p_mapping[port] = phy_port;
                rv = soc_wc_xgxs_power_down(unit, port, 0);
                if (SOC_FAILURE(rv)) {
                    break;
                }
            }
        }
        si->port_l2p_mapping[port] = saved_phy_port;
        si->port_type[port] = saved_port_type;

        SOC_BLOCK_ITER(unit, blk, SOC_BLK_XLPORT) {
            port = SOC_BLOCK_PORT(unit, blk);
            _soc_xgxs_mdio_setup(unit, port);
            SOC_IF_ERROR_RETURN(soc_wc_xgxs_reset(unit, port, 0));
        }
        SOC_BLOCK_ITER(unit, blk, SOC_BLK_XLPORT) {
            port = SOC_BLOCK_PORT(unit, blk);
            SOC_IF_ERROR_RETURN(soc_wc_xgxs_pll_check(unit, port, 0));
        }

        SOC_BLOCK_ITER(unit, blk, SOC_BLK_XLPORT) {
            port = SOC_BLOCK_PORT(unit, blk);
            /* Bring XMAC out of reset */
            if (si->port_speed_max[port] < 10000) {
                continue;
            }
            SOC_IF_ERROR_RETURN(READ_XLPORT_XMAC_CONTROLr(unit, port, &rval));
            soc_reg_field_set(unit, XLPORT_XMAC_CONTROLr, &rval, XMAC_RESETf,
                              1);
            SOC_IF_ERROR_RETURN(WRITE_XLPORT_XMAC_CONTROLr(unit, port, rval));
            sal_udelay(10);

            soc_reg_field_set(unit, XLPORT_XMAC_CONTROLr, &rval, XMAC_RESETf,
                              0);
            SOC_IF_ERROR_RETURN(WRITE_XLPORT_XMAC_CONTROLr(unit, port, rval));
        }
    } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANAX(unit)) {
        soc_port_t port;
        int blk;
        uint32 rval;

        SOC_BLOCK_ITER(unit, blk, SOC_BLK_MXQPORT) {
            port = SOC_BLOCK_PORT(unit, blk);
            _soc_xgxs_mdio_setup(unit, port);
            SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));
            /* Bring XMAC out of reset */
            if (SOC_IS_KATANA(unit)) {
                SOC_IF_ERROR_RETURN(READ_XPORT_XGXS_NEWCTL_REGr(unit,port,&rval));
                soc_reg_field_set(unit, XPORT_XGXS_NEWCTL_REGr, &rval, TXD1G_FIFO_RSTBf,
                                  0xf);
                WRITE_XPORT_XGXS_NEWCTL_REGr(unit,port,rval);
            } 
            SOC_IF_ERROR_RETURN(READ_XPORT_XMAC_CONTROLr(unit, port, &rval));
            soc_reg_field_set(unit, XPORT_XMAC_CONTROLr, &rval, XMAC_RESETf,
                              1);
            SOC_IF_ERROR_RETURN(WRITE_XPORT_XMAC_CONTROLr(unit, port, rval));
            sal_udelay(10);

            soc_reg_field_set(unit, XPORT_XMAC_CONTROLr, &rval, XMAC_RESETf,
                              0);
            SOC_IF_ERROR_RETURN(WRITE_XPORT_XMAC_CONTROLr(unit, port, rval));
        }
        SOC_BLOCK_ITER(unit, blk, SOC_BLK_GPORT) {
            port = SOC_BLOCK_PORT(unit, blk);
            SOC_IF_ERROR_RETURN(READ_QUAD0_SERDES_CTRLr(unit, port, &rval));
            soc_reg_field_set(unit, QUAD0_SERDES_CTRLr, &rval, RSTB_PLLf, 1);
            SOC_IF_ERROR_RETURN(WRITE_QUAD0_SERDES_CTRLr(unit, port, rval));
        if (SOC_PORT_VALID(unit, port+4)) {
        SOC_IF_ERROR_RETURN(WRITE_QUAD1_SERDES_CTRLr(unit, port+4, rval));
        }
            sal_usleep(100000);
            SOC_IF_ERROR_RETURN(READ_QUAD0_SERDES_CTRLr(unit, port, &rval));
            soc_reg_field_set(unit, QUAD0_SERDES_CTRLr, &rval, HW_RSTLf, 1);
            SOC_IF_ERROR_RETURN(WRITE_QUAD0_SERDES_CTRLr(unit, port, rval));
        if (SOC_PORT_VALID(unit, port+4)) {
        SOC_IF_ERROR_RETURN(WRITE_QUAD1_SERDES_CTRLr(unit, port+4, rval));
        }
            sal_usleep(10000);
            soc_reg_field_set(unit, QUAD0_SERDES_CTRLr, &rval, TXFIFO_RSTLf, 0xf);
            soc_reg_field_set(unit, QUAD0_SERDES_CTRLr, &rval, RSTB_PLLf, 1);
            soc_reg_field_set(unit, QUAD0_SERDES_CTRLr, &rval, RSTB_MDIOREGSf, 1);
            SOC_IF_ERROR_RETURN(WRITE_QUAD0_SERDES_CTRLr(unit, port, rval));
        if (SOC_PORT_VALID(unit, port+4)) {
        SOC_IF_ERROR_RETURN(WRITE_QUAD1_SERDES_CTRLr(unit, port+4, rval));
        }
            rval = 0xff; /* Set UMAC0_RESETf..UMAC7_RESETf */
            SOC_IF_ERROR_RETURN(WRITE_GPORT_UMAC_CONTROLr(unit, port, rval));
            sal_udelay(10);
            rval = 0; /* Reset UMAC0_RESETf..UMAC7_RESETf */
            SOC_IF_ERROR_RETURN(WRITE_GPORT_UMAC_CONTROLr(unit, port, rval));
        }
    } else
#endif /* BCM_KATANA_SUPPORT */ 
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        soc_info_t *si;
        soc_port_t port;
        int phy, lane, blk, block_has_ports, high_speed_port, xgxs0_reset;
        uint32 rval;

        si = &SOC_INFO(unit);
        SOC_BLOCK_ITER(unit, blk, SOC_BLK_CLPORT) {
            block_has_ports = FALSE;
            xgxs0_reset = FALSE;
            high_speed_port = -1;
            for (phy = 0; phy < _SOC_TR3_PHY_PER_PORT_BLOCK; phy++) {
                for (lane = 0; lane < _SOC_TR3_LANES_PER_PHY; lane++) {
                    port = si->port_p2l_mapping[si->blk_fpp[blk] +
                               ((phy * _SOC_TR3_LANES_PER_PHY) + lane)];
                    if (SOC_PORT_VALID(unit, port) &&
                        si->port_speed_max[port] >= 100000) {
                        /* Save the 100G+ speed port becuase we must
                         * init the other two PHYs, even though they don't
                         * match a valid port. */
                        high_speed_port = port;
                    } else if (high_speed_port >= 0) {
                        port = high_speed_port;
                    }
                    if (SOC_PORT_VALID(unit, port)) {
                        SOC_IF_ERROR_RETURN
                            (soc_wc_xgxs_reset(unit, port, phy));
                        block_has_ports |= 1 << phy;
                        if ((0 != phy) && !xgxs0_reset) {
                            /* We must always reset the first WC
                             * in the block */
                            SOC_IF_ERROR_RETURN
                                (soc_wc_xgxs_reset(unit, port, 0));
                        }
                        xgxs0_reset = TRUE;
                        break; /* Only need to reset PHY once */
                    }
                }
            }
            if (!block_has_ports) {
                continue; /* All ports disabled */
            }
            port = SOC_BLOCK_PORT(unit, blk);
            SOC_IF_ERROR_RETURN(READ_PORT_MAC_CONTROLr(unit, port, &rval));
            if (((dev_id == BCM56640_DEVICE_ID) || (dev_id == BCM56044_DEVICE_ID)) && high_speed_port >= 0) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  CMAC_RESETf, 1);
            }
            if (block_has_ports & 1) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC0_RESETf, 1);
            }
            if (block_has_ports & 2) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC1_RESETf, 1);
            }
            if (block_has_ports & 4) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC2_RESETf, 1);
            }
            SOC_IF_ERROR_RETURN(WRITE_PORT_MAC_CONTROLr(unit, port, rval));
            sal_udelay(10);
            if (((dev_id == BCM56640_DEVICE_ID) || (dev_id == BCM56044_DEVICE_ID)) && high_speed_port >= 0) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  CMAC_RESETf, 0);
            }
            if (block_has_ports & 1) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC0_RESETf, 0);
            }
            if (block_has_ports & 2) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC1_RESETf, 0);
            }
            if (block_has_ports & 4) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC2_RESETf, 0);
            }
            SOC_IF_ERROR_RETURN(WRITE_PORT_MAC_CONTROLr(unit, port, rval));

            /* Take MLD out of reset */
            SOC_IF_ERROR_RETURN(READ_PORT_MLD_CTRL_REGr(unit, port, &rval));
            soc_reg_field_set(unit, PORT_MLD_CTRL_REGr, &rval, 
                              RSTB_MDIOREGSf, 1);
            soc_reg_field_set(unit, PORT_MLD_CTRL_REGr, &rval, 
                              RSTB_HWf, 1);
            SOC_IF_ERROR_RETURN(WRITE_PORT_MLD_CTRL_REGr(unit, port, rval));
        }
        SOC_BLOCK_ITER(unit, blk, SOC_BLK_XTPORT) {
            block_has_ports = FALSE;
            for (phy = 0; phy < _SOC_TR3_PHY_PER_PORT_BLOCK; phy++) {
                for (lane = 0; lane < _SOC_TR3_LANES_PER_PHY; lane++) {
                    port = si->port_p2l_mapping[si->blk_fpp[blk] +
                               ((phy * _SOC_TR3_LANES_PER_PHY) + lane)];
                    if (SOC_PORT_VALID(unit, port)) {
                        SOC_IF_ERROR_RETURN
                            (soc_wc_xgxs_reset(unit, port, phy));
                        block_has_ports |= 1 << phy;
                        break; /* Only need to reset PHY once */
                    }
                }
            }
            if (!block_has_ports) {
                continue; /* All ports disabled */
            }
            port = SOC_BLOCK_PORT(unit, blk);
            SOC_IF_ERROR_RETURN(READ_PORT_MAC_CONTROLr(unit, port, &rval));
            if (block_has_ports & 1) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC0_RESETf, 1);
            }
            if (block_has_ports & 2) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC1_RESETf, 1);
            }
            if (block_has_ports & 4) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC2_RESETf, 1);
            }
            SOC_IF_ERROR_RETURN(WRITE_PORT_MAC_CONTROLr(unit, port, rval));
            sal_udelay(10);
            if (block_has_ports & 1) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC0_RESETf, 0);
            }
            if (block_has_ports & 2) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC1_RESETf, 0);                              
            }
            if (block_has_ports & 4) {
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC2_RESETf, 0);
            }
            SOC_IF_ERROR_RETURN(WRITE_PORT_MAC_CONTROLr(unit, port, rval));
        }
        SOC_BLOCK_ITER(unit, blk, SOC_BLK_XLPORT) {
            block_has_ports = FALSE;
            /* Only one PHY per XLPORT */
            for (lane = 0; lane < _SOC_TR3_LANES_PER_PHY; lane++) {
                port = si->port_p2l_mapping[si->blk_fpp[blk] + lane];
                if (SOC_PORT_VALID(unit, port)) {
                    block_has_ports = TRUE;
                    break;
                }
            }
            if (!block_has_ports) {
                continue; /* All ports disabled */
            }
            port = SOC_BLOCK_PORT(unit, blk);
            SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));
            SOC_IF_ERROR_RETURN(READ_PORT_MAC_CONTROLr(unit, port, &rval));
            soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                              XMAC0_RESETf, 1);
            SOC_IF_ERROR_RETURN(WRITE_PORT_MAC_CONTROLr(unit, port, rval));
            sal_udelay(10);
            soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                              XMAC0_RESETf, 0);
            SOC_IF_ERROR_RETURN(WRITE_PORT_MAC_CONTROLr(unit, port, rval));
        } 
#ifdef BCM_HELIX4_SUPPORT
        if (SOC_IS_HELIX4(unit)) {
            SOC_BLOCK_ITER(unit, blk, SOC_BLK_XWPORT) {
                block_has_ports = FALSE;
                /* Only one PHY per XWPORT */
                for (lane = 0; lane < _SOC_TR3_LANES_PER_PHY; lane++) {
                    port = si->port_p2l_mapping[si->blk_fpp[blk] + lane];
                    if (SOC_PORT_VALID(unit, port)) {
                        block_has_ports = TRUE;
                        break;
                    }
                }
                if (!block_has_ports) {
                    continue; /* All ports disabled */
                }
                port = SOC_BLOCK_PORT(unit, blk);
                SOC_IF_ERROR_RETURN(soc_xgxs_reset(unit, port));
                SOC_IF_ERROR_RETURN(READ_PORT_MAC_CONTROLr(unit, port, &rval));
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC0_RESETf, 1);
                SOC_IF_ERROR_RETURN(WRITE_PORT_MAC_CONTROLr(unit, port, rval));
                sal_udelay(10);
                soc_reg_field_set(unit, PORT_MAC_CONTROLr, &rval, 
                                  XMAC0_RESETf, 0);
                SOC_IF_ERROR_RETURN(WRITE_PORT_MAC_CONTROLr(unit, port, rval));
            }
        }
#endif /* BCM_HELIX4_SUPPORT */
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_GREYHOUND_SUPPORT
        if (SOC_IS_GREYHOUND(unit)) {
            SOC_IF_ERROR_RETURN(soc_greyhound_tsc_reset(unit));
        } else
#endif /* BCM_TRIDENT2_SUPPORT */

    {
        soc_port_t port;

        /* Reset the fusion core for HiGig ports. Does nothing in sim */
        /* Fusioncore must be out of reset before we probe for XGXS PHY.
         * Otherwise, XGXS PHY will not respond.
         */
        PBMP_HG_ITER(unit, port) {
            _soc_xgxs_mdio_setup(unit, port);
            soc_fusioncore_reset(unit, port);
        }
        PBMP_XE_ITER(unit, port) {
            _soc_xgxs_mdio_setup(unit, port);
            soc_fusioncore_reset(unit, port);
        }
    }
#endif /* BCM_BIGMAC_SUPPORT */
    return 0;
}

/*
 * Function:
 *      soc_detach
 * Purpose:
 *      Detach a SOC device and deallocate all resources allocated.
 * Notes:
 *      The chip is reset prior to detaching in order to quiesce
 *      any DMA, etc. operations that may be in progress.
 */

int
soc_detach(int unit)
{
    soc_control_t   *soc;
    soc_mem_t       mem, act_mem, lock_mem;
    int             ix;
    
    LOG_INFO(BSL_LS_SOC_PCI,
             (BSL_META_U(unit,
                         "Detaching\n")));

    soc = SOC_CONTROL(unit);

    if (NULL == soc) {
        return SOC_E_NONE;
    }

    if (0 == (soc->soc_flags & SOC_F_ATTACHED)) {
        return SOC_E_NONE;
    }

#ifdef BCM_HERCULES_SUPPORT
    /* Dump the MMU error stats */
    soc_mmu_error_done(unit);

    if (soc->lla_cells_good != NULL) {
        sal_free(soc->lla_cells_good);
        soc->lla_cells_good = NULL;
    }

    if (soc->lla_map != NULL) {
        int port;
        PBMP_PORT_ITER(unit, port) {
            if (soc->lla_map[port] != NULL) {
                sal_free(soc->lla_map[port]);
            }
        }
        sal_free(soc->lla_map);
        soc->lla_map = NULL;
    }

    if (soc->sbe_disable != NULL) {
        sal_free(soc->sbe_disable);
        soc->sbe_disable = NULL;
    }
#endif

    /* Free up any memory used by the I2C driver */

#ifdef  INCLUDE_I2C
    SOC_IF_ERROR_RETURN(soc_i2c_detach(unit));
#endif

    /* Clear all outstanding DPCs owned by this unit */

    sal_dpc_cancel(INT_TO_PTR(unit));

    /*
     * Call soc_init to cancel link scan task, counter DMA task,
     * outstanding DMAs, interrupt generation, and anything else the
     * driver or chip may be doing.
     */

#if !defined(PLISIM)
    soc_reset_init(unit);
#endif

    /*
     * PHY drivers and ID map
     */
    SOC_IF_ERROR_RETURN(soc_phyctrl_software_deinit(unit));

    if (!SOC_IS_RCPU_ONLY(unit)) {
        /* Free up DMA memory */
        SOC_IF_ERROR_RETURN(soc_dma_detach(unit));
    }

    /* Shutdown polled interrupt mode if active */
    soc_ipoll_disconnect(unit);
    soc->soc_flags &= ~SOC_F_POLLED;

    /* Detach interrupt handler, if we installed one */
    /* unit # is ISR arg */
    if (soc_cm_interrupt_disconnect(unit) < 0) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_detach: could not disconnect interrupt line\n")));
    }

    /*
     * When detaching, take care to free up only resources that were
     * actually allocated, in case we are cleaning up after an attach
     * that failed part way through.
     */

    if (soc->arlBuf != NULL) {
        soc_cm_sfree(unit, (void *)soc->arlBuf);
        soc->arlBuf = NULL;
    }

    if (soc->arlShadow != NULL) {
        shr_avl_destroy(soc->arlShadow);
        soc->arlShadow = NULL;
    }

    if (soc->arlShadowMutex != NULL) {
        sal_mutex_destroy(soc->arlShadowMutex);
        soc->arlShadowMutex = NULL;
    }

    if (soc->ipArbiterMutex != NULL) {
        sal_mutex_destroy(soc->ipArbiterMutex);
        soc->ipArbiterMutex = NULL;
    }

    /* Terminate counter module; frees allocated space */

    soc_counter_detach(unit);

    if (soc->counterMutex) {
        sal_mutex_destroy(soc->counterMutex);
        soc->counterMutex = NULL;
    }

    /* Deallocate L2X module and shadow table resources */

#ifdef BCM_XGS_SWITCH_SUPPORT
    if (soc_feature(unit, soc_feature_arl_hashed)) {
        SOC_IF_ERROR_RETURN(soc_l2x_detach(unit));
    }
#ifdef BCM_TRIUMPH3_SUPPORT
    if SOC_IS_TRIUMPH3(unit) {
        SOC_IF_ERROR_RETURN(soc_tr3_l2_bulk_age_stop(unit));
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    if SOC_IS_TRIDENT2(unit) {
        SOC_IF_ERROR_RETURN(soc_td2_l2_bulk_age_stop(unit));
    }
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_SBUSDMA_SUPPORT
    if (SOC_SBUSDMA_DM_INFO(unit)) {
        SOC_IF_ERROR_RETURN(soc_sbusdma_desc_detach(unit));
    }
#endif /* BCM_SBUSDMA_SUPPORT */
#endif /* BCM_XGS_SWITCH_SUPPORT */

#ifdef INCLUDE_MEM_SCAN
    SOC_IF_ERROR_RETURN(soc_mem_scan_stop(unit)); /* Stop memory scanner */
    if (soc->mem_scan_notify) {
        sal_sem_destroy(soc->mem_scan_notify);
    }
#endif
    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
        if (!SOC_MEM_IS_VALID(unit, mem)) {
            continue;
        }
        act_mem = mem;
#ifdef BCM_ESW_SUPPORT
        _SOC_DRV_MEM_REUSE_MEM_STATE(unit, act_mem, lock_mem);
#endif
        /* Deallocate table cache memory, if caching enabled */
        SOC_IF_ERROR_RETURN(soc_mem_cache_set(unit, act_mem, COPYNO_ALL, FALSE));
    }

    /* LOCK used in cache_set, so destroy later */
    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
        if (!SOC_MEM_IS_VALID(unit, mem)) {
            continue;
        }
        act_mem = mem;
        lock_mem = mem;
#ifdef BCM_ESW_SUPPORT
        _SOC_DRV_MEM_REUSE_MEM_STATE(unit, act_mem, lock_mem);
#endif
        if (soc->memState[lock_mem].lock != NULL) {
            sal_mutex_destroy(soc->memState[lock_mem].lock);
            soc->memState[lock_mem].lock = NULL;
        }
    }

    if (soc->schanMutex) {
        sal_mutex_destroy(soc->schanMutex);
        soc->schanMutex = NULL;
    }

#ifdef BCM_CMICM_SUPPORT
    if (soc->fschanMutex) {
        sal_mutex_destroy(soc->fschanMutex);
        soc->fschanMutex = NULL;
    }

    if (soc->ccmDmaIntr) {
        sal_sem_destroy(soc->ccmDmaIntr);
        soc->ccmDmaIntr = NULL;
    }

    if (soc_feature(unit, soc_feature_cmicm)) {
        soc_cmic_uc_msg_stop(unit);
    }
#endif

    if (soc->schanIntr) {
        sal_sem_destroy(soc->schanIntr);
        soc->schanIntr = NULL;
    }

    if (soc->miimMutex) {
        sal_mutex_destroy(soc->miimMutex);
        soc->miimMutex = NULL;
    }

    if (soc->tableDmaMutex) {
        sal_mutex_destroy(soc->tableDmaMutex);
        soc->tableDmaMutex = NULL;
    }

    if (soc->tslamDmaMutex) {
        sal_mutex_destroy(soc->tslamDmaMutex);
        soc->tslamDmaMutex = NULL;
    }

    if (soc->tableDmaIntr) {
        sal_sem_destroy(soc->tableDmaIntr);
        soc->tableDmaIntr = NULL;
    }

    if (soc->tslamDmaIntr) {
        sal_sem_destroy(soc->tslamDmaIntr);
        soc->tslamDmaIntr = NULL;
    }

    if (soc->miimIntr) {
        sal_sem_destroy(soc->miimIntr);
        soc->miimIntr = NULL;
    }

    for (ix = 0; ix < 3; ix++) {
        if (soc->memCmdIntr[ix]) {
            sal_sem_destroy(soc->memCmdIntr[ix]);
            soc->memCmdIntr[ix] = NULL;
        }
    }

    if (soc->arl_notify) {
        sal_sem_destroy(soc->arl_notify);
        soc->arl_notify = NULL;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_esm_correction)) {
        SOC_IF_ERROR_RETURN(soc_tr3_esm_recovery_stop(unit));
        if (soc->esm_recovery_notify) {
            sal_sem_destroy(soc->esm_recovery_notify);
            soc->esm_recovery_notify = NULL;
        }
        if (soc->esm_lock) {
                sal_mutex_destroy(soc->esm_lock);
                soc->esm_lock = NULL;
            }
    } 
#endif

#ifdef BCM_XGS_SWITCH_SUPPORT
    if (NULL != soc->l2x_notify) {
        sal_sem_destroy(soc->l2x_notify);
        soc->l2x_notify = NULL;
    }
    if (NULL != soc->l2x_del_sync) {
        sal_mutex_destroy(soc->l2x_del_sync);
        soc->l2x_del_sync = NULL;
    }
    if (NULL != soc->l2x_lock) {
        sal_sem_destroy(soc->l2x_lock);
        soc->l2x_lock = NULL;
    }
    soc->l2x_pid = SAL_THREAD_ERROR;
    soc->l2x_interval = 0;
#ifdef BCM_TRIUMPH3_SUPPORT
    if SOC_IS_TRIUMPH3(unit) {
        if (NULL != soc->l2x_age_notify) {
            sal_sem_destroy(soc->l2x_age_notify);
            soc->l2x_age_notify = NULL;
        }
        soc->l2x_age_pid = SAL_THREAD_ERROR;
        soc->l2x_age_interval = 0;
        soc->l2x_agetime_adjust_usec = 0;
        soc->l2x_agetime_curr_timeblk = 0;
        soc->l2x_agetime_curr_timeblk_usec = 0;
        soc->l2x_prev_age_timeout = 0;
    } 
#endif
#ifdef BCM_TRIDENT2_SUPPORT
    if SOC_IS_TRIDENT2(unit) {
        if (NULL != soc->l2x_age_notify) {
            sal_sem_destroy(soc->l2x_age_notify);
            soc->l2x_age_notify = NULL;
        }
        if (NULL != soc->l2x_age_sync) {
            sal_sem_destroy(soc->l2x_age_sync);
            soc->l2x_age_sync = NULL;
        }
        soc->l2x_age_pid = SAL_THREAD_ERROR;
        soc->l2x_age_interval = 0;
        soc->l2x_age_enable = 0;
        soc->l2x_sw_aging = 0;
    } 
#endif
#endif /* BCM_XGS_SWITCH_SUPPORT */
    /* Destroy L3 module protection mutex. */
    if (NULL != soc->l3x_lock) {
        sal_mutex_destroy(soc->l3x_lock);
        soc->l3x_lock = NULL;
    }

    if (soc->ipfixIntr) {
        sal_sem_destroy(soc->ipfixIntr);
        soc->ipfixIntr = NULL;
    }

#if defined(INCLUDE_REGEX)
        if (soc->ftreportIntr) {
            sal_sem_destroy(soc->ftreportIntr);
            soc->ftreportIntr = NULL;
        }
#endif

    if (soc->socControlMutex) {
        sal_mutex_destroy(soc->socControlMutex);
        soc->socControlMutex = NULL;
    }

    if (soc->egressMeteringMutex) {
        sal_mutex_destroy(soc->egressMeteringMutex);
        soc->egressMeteringMutex = NULL;
    }

    if (soc->tcam_info) {
        sal_free(soc->tcam_info);
    }

    /* MMU LLS usage map */
    for (ix = 0; ix < SOC_MAX_NUM_PORTS; ix++) {
#if defined(BCM_KATANA2_SUPPORT)
        if (soc->port_lls_s0_bmap[ix] != NULL) {
            sal_free(soc->port_lls_s0_bmap[ix]);
        }
        if (soc->port_lls_s1_bmap[ix] != NULL) {
            sal_free(soc->port_lls_s1_bmap[ix]);
        }
#endif /* BCM_KATANA2_SUPPORT */
        if (soc->port_lls_l0_bmap[ix] != NULL) {
            sal_free(soc->port_lls_l0_bmap[ix]);
        }
        if (soc->port_lls_l1_bmap[ix] != NULL) {
            sal_free(soc->port_lls_l1_bmap[ix]);
        }
        if (soc->port_lls_l2_bmap[ix] != NULL) {
        sal_free(soc->port_lls_l2_bmap[ix]);
        }
    }

#ifdef BCM_TRIUMPH_SUPPORT
    if (soc->ext_l2_ppa_info != NULL) {
        sal_free(soc->ext_l2_ppa_info);
        soc->ext_l2_ppa_info = NULL;
    }
    if (soc->ext_l2_ppa_vlan != NULL) {
        sal_free(soc->ext_l2_ppa_vlan);
        soc->ext_l2_ppa_vlan = NULL;
    }
#endif /* BCM_TRIUMPH_SUPPORT */

#if defined(BCM_RCPU_SUPPORT) && defined(BCM_XGS3_SWITCH_SUPPORT)    
    if (SOC_IS_RCPU_ONLY(unit)) {
        soc_rcpu_deinit(unit);
    }
#endif /* !defined(PLISIM) */

    sal_free(SOC_PERSIST(unit));
    SOC_PERSIST(unit) = NULL;

    sal_free(SOC_CONTROL(unit));
    SOC_CONTROL(unit) = NULL;

#ifdef BCM_WARM_BOOT_SUPPORT
    /* Second times detach cause SOC_E_CONFIG so not CheckingFalseReturn value*/
    soc_scache_detach(unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

    if (--soc_ndev_attached == 0) {
        /* Work done after the last SOC device is detached. */
        /* (currently nothing) */
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      _soc_functions_find
 * Purpose:
 *      Return function array corresponding to the driver
 * Returns:
 *      Pointer to static function array soc_functions_t
 */

STATIC soc_functions_t *
_soc_functions_find(soc_driver_t *drv)
{
    switch (drv->type) {

#ifdef BCM_HERCULES15_SUPPORT
    case SOC_CHIP_BCM5675_A0:  return &soc_hercules15_drv_funs;
#endif

#ifdef BCM_FIREBOLT_SUPPORT
    case SOC_CHIP_BCM56504_A0:  return &soc_firebolt_drv_funs;
    case SOC_CHIP_BCM56504_B0:  return &soc_firebolt_drv_funs;
    case SOC_CHIP_BCM56102_A0:  return &soc_firebolt_drv_funs;
    case SOC_CHIP_BCM56304_B0:  return &soc_firebolt_drv_funs;
    case SOC_CHIP_BCM56112_A0:  return &soc_firebolt_drv_funs;
    case SOC_CHIP_BCM56314_A0:  return &soc_firebolt_drv_funs;
    case SOC_CHIP_BCM56514_A0:  return &soc_firebolt_drv_funs;
#ifdef BCM_RAPTOR_SUPPORT
    case SOC_CHIP_BCM56218_A0:  return &soc_firebolt_drv_funs;
    case SOC_CHIP_BCM56224_A0:  return &soc_firebolt_drv_funs;
    case SOC_CHIP_BCM56224_B0:  return &soc_firebolt_drv_funs;
    case SOC_CHIP_BCM53314_A0:  return &soc_firebolt_drv_funs;
    case SOC_CHIP_BCM53324_A0:  return &soc_firebolt_drv_funs;
#endif
#ifdef BCM_BRADLEY_SUPPORT
    case SOC_CHIP_BCM56800_A0:  return &soc_bradley_drv_funs;
    case SOC_CHIP_BCM56820_A0:  return &soc_bradley_drv_funs;
    case SOC_CHIP_BCM56725_A0:  return &soc_bradley_drv_funs;
#endif
#ifdef BCM_SHADOW_SUPPORT
    case SOC_CHIP_BCM88732_A0:  return &soc_shadow_drv_funs;
#endif
#ifdef BCM_TRIUMPH_SUPPORT
    case SOC_CHIP_BCM56624_A0:  return &soc_triumph_drv_funs;
    case SOC_CHIP_BCM56624_B0:  return &soc_triumph_drv_funs;
#endif
#ifdef BCM_VALKYRIE_SUPPORT
    case SOC_CHIP_BCM56680_A0:  return &soc_valkyrie_drv_funs;
    case SOC_CHIP_BCM56680_B0:  return &soc_valkyrie_drv_funs;
#endif
#ifdef BCM_TRIUMPH2_SUPPORT
    case SOC_CHIP_BCM56634_A0:  return &soc_triumph2_drv_funs;
    case SOC_CHIP_BCM56634_B0:  return &soc_triumph2_drv_funs;
    case SOC_CHIP_BCM56524_A0:  return &soc_triumph2_drv_funs;
    case SOC_CHIP_BCM56524_B0:  return &soc_triumph2_drv_funs;
    case SOC_CHIP_BCM56685_A0:  return &soc_triumph2_drv_funs;
    case SOC_CHIP_BCM56685_B0:  return &soc_triumph2_drv_funs;
#endif
#ifdef BCM_ENDURO_SUPPORT
    case SOC_CHIP_BCM56334_A0:  return &soc_enduro_drv_funs;
    case SOC_CHIP_BCM56334_B0:  return &soc_enduro_drv_funs;
#endif
#ifdef BCM_HURRICANE_SUPPORT
    case SOC_CHIP_BCM56142_A0:  return &soc_hurricane_drv_funs;
#endif
#ifdef BCM_HURRICANE2_SUPPORT
    case SOC_CHIP_BCM56150_A0:  return &soc_hurricane2_drv_funs;
#endif
#ifdef BCM_TRIDENT_SUPPORT
    case SOC_CHIP_BCM56840_A0:  return &soc_trident_drv_funs;
    case SOC_CHIP_BCM56840_B0:  return &soc_trident_drv_funs;
#endif
#ifdef BCM_TRIDENT2_SUPPORT
    case SOC_CHIP_BCM56850_A0:  return &soc_trident2_drv_funs;
#endif
#ifdef BCM_TRIUMPH3_SUPPORT
    case SOC_CHIP_BCM56640_A0:  return &soc_triumph3_drv_funs;
    case SOC_CHIP_BCM56640_B0:  return &soc_triumph3_drv_funs;
#endif
#ifdef BCM_HELIX4_SUPPORT
    case SOC_CHIP_BCM56340_A0:  return &soc_helix4_drv_funs;
#endif
#ifdef BCM_KATANA_SUPPORT
    case SOC_CHIP_BCM56440_A0:  return &soc_katana_drv_funs;
    case SOC_CHIP_BCM56440_B0:  return &soc_katana_drv_funs;
#endif
#ifdef BCM_KATANA2_SUPPORT
    case SOC_CHIP_BCM56450_A0:  return &soc_katana2_drv_funs;
    case SOC_CHIP_BCM56450_B0:  return &soc_katana2_drv_funs;
#endif
#ifdef BCM_GREYHOUND_SUPPORT
    case SOC_CHIP_BCM53400_A0:  return &soc_greyhound_drv_funs;
#endif

#endif
    default:                    return NULL;
    }
}

/*
 * Function:
 *      soc_attach
 * Purpose:
 *      Initialize the soc_control_t structure for a device,
 *      allocating all memory and semaphores required.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      detach - Callback function called on detach.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      No chip initialization is done other than masking all interrupts,
 *      see soc_init or soc_reset_init.
 */
int
soc_attach(int unit)
{
    soc_control_t       *soc;
    soc_persist_t       *sop;
    int                 rv = SOC_E_MEMORY;
    soc_mem_t           mem, act_mem, lock_mem;
    uint16              dev_id;
    uint8               rev_id;
    soc_port_t          port;
    int                 ix;
    ipoll_handler_t     iph = soc_intr;
    uint32              table_dma_enable=0;
    uint32              tslam_dma_enable=0;
    int                 enable;
#ifdef BCM_KATANA_SUPPORT
    uint32              saber_pool_size[BCM_SABER_MAX_COUNTER_DIRECTION]=
                        {511,127};
    soc_mem_t           saber_counter_table[BCM_SABER_MAX_COUNTER_DIRECTION]
                                           [BCM_SABER_MAX_COUNTER_POOL]={
                        {ING_FLEX_CTR_COUNTER_TABLE_0m,
                         ING_FLEX_CTR_COUNTER_TABLE_1m,
                         ING_FLEX_CTR_COUNTER_TABLE_2m,
                         ING_FLEX_CTR_COUNTER_TABLE_3m,
                         ING_FLEX_CTR_COUNTER_TABLE_4m,
                         ING_FLEX_CTR_COUNTER_TABLE_5m,
                         ING_FLEX_CTR_COUNTER_TABLE_6m,
                         ING_FLEX_CTR_COUNTER_TABLE_7m},
                        {EGR_FLEX_CTR_COUNTER_TABLE_0m,
                         EGR_FLEX_CTR_COUNTER_TABLE_1m,
                         EGR_FLEX_CTR_COUNTER_TABLE_2m,
                         EGR_FLEX_CTR_COUNTER_TABLE_3m,
                         EGR_FLEX_CTR_COUNTER_TABLE_4m,
                         EGR_FLEX_CTR_COUNTER_TABLE_5m,
                         EGR_FLEX_CTR_COUNTER_TABLE_6m,
                         EGR_FLEX_CTR_COUNTER_TABLE_7m}};
    uint32              saber_counter_direction=0;
    uint32              saber_counter_pool=0;
#endif
    soc_pbmp_t          pbmp_all;

    LOG_INFO(BSL_LS_SOC_PCI,
             (BSL_META_U(unit,
                         "soc_attach: unit %d\n"), unit));

    /*
     * Allocate soc_control and soc_persist if not already.
     */

    if (SOC_CONTROL(unit) == NULL) {
        SOC_CONTROL(unit) =
            sal_alloc(sizeof (soc_control_t), "soc_control");
        if (SOC_CONTROL(unit) == NULL) {
            return SOC_E_MEMORY;
        }
        sal_memset(SOC_CONTROL(unit), 0, sizeof (soc_control_t));
    }

    SOC_PERSIST(unit) = sal_alloc(sizeof (soc_persist_t), "soc_persist");
    if (NULL == SOC_PERSIST(unit)) {
        return SOC_E_MEMORY;
    }

    sal_memset(SOC_PERSIST(unit), 0, sizeof (soc_persist_t));

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_IF_ERROR_RETURN(soc_stable_attach(unit));
#endif /* BCM_WARM_BOOT_SUPPORT */

    soc = SOC_CONTROL(unit);
    sop = SOC_PERSIST(unit);
    sop->version = 1;

    soc_cm_get_id(unit, &dev_id, &rev_id);

    /*
     * Instantiate the driver -- Verify chip revision matches driver
     * compilation revision.
     */
    soc->chip_driver = soc_chip_driver_find(dev_id, rev_id);

    if (soc->chip_driver == NULL) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_attach: unit %d has no driver "
                              "(device 0x%04x rev 0x%02x)\n"),
                   unit, dev_id, rev_id));
        return SOC_E_UNAVAIL;
    }

    soc->soc_functions = _soc_functions_find(soc->chip_driver);

    if (soc->soc_functions == NULL) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_attach: unit %d has no functions\n"),
                              unit));
        return SOC_E_INTERNAL;
    }
#ifdef INCLUDE_RCPU
    if (soc_property_get(unit, spn_PCI2EB_OVERRIDE, 0)) {
        soc->remote_cpu = TRUE;
    } else {
        soc->remote_cpu = FALSE;
    }

    /* Is this an RCPU Only unit? */
    if (soc_property_get(unit, spn_RCPU_ONLY, 0) ||
        (soc_cm_get_bus_type(unit) & SOC_RCPU_DEV_TYPE)) {
        soc->remote_cpu = TRUE;
        soc->soc_flags |= SOC_F_RCPU_ONLY;
    }
#endif /* INCLUDE_RCPU */

    soc->soc_rcpu_schan_op = NULL;

    soc->ev_cb_head = NULL;

    /* Set feature cache, since used by mutex creation */
    soc_feature_init(unit);

    /* Install the Interrupt Handler */
    /* Make sure interrupts are masked before connecting line. */
#ifdef BCM_CMICM_SUPPORT
    if (soc_feature(unit, soc_feature_cmicm)) {
        int arm;

        /* Initialize PCI/iProc based host irq offset */
        if (soc_cm_get_bus_type(unit) & SOC_AXI_DEV_TYPE) {
            soc_cmicm_ihost_irq_offset_set(unit);
        } else {
            soc_cmicm_ihost_irq_offset_reset(unit);
        }

        soc_cmicm_intr0_disable(unit, ~0);
        soc_cmicm_intr1_disable(unit, ~0);
        soc_cmicm_intr2_disable(unit, ~0);
        SOC_CMCS_NUM(unit) = 3; 
        SOC_PCI_CMC(unit) = soc_property_uc_get(unit, 0, spn_CMC, 0);
        if (SOC_PCI_CMC(unit) >= SOC_CMCS_NUM(unit)) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "soc_attach: "
                                  "specified PCI CMC %d exceeds maximum of %d\n"),
                       SOC_PCI_CMC(unit), SOC_CMCS_NUM(unit)));
            return SOC_E_PARAM;
        }
        arm = 0;
        for (ix = 0; ix < SOC_CMCS_NUM(unit); ix++) {
            
            if (SOC_PCI_CMC(unit) != ix) {
                SOC_ARM_CMC(unit, arm) = ix;
                arm++;
            }
        }
    } else
#endif
    {
        soc_intr_disable(unit, ~0);
    }
#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm)) {
        iph = soc_cmicm_intr;
    }
#endif
    if (soc_property_get(unit, spn_POLLED_IRQ_MODE, 0)) {
        if (soc_ipoll_connect(unit, iph, INT_TO_PTR(unit)) < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "soc_attach: "
                                  "error initializing polled interrupt mode\n")));
            return SOC_E_INTERNAL;
        }
        soc->soc_flags |= SOC_F_POLLED;
    } else {
        /* unit # is ISR arg */
        if (soc_cm_interrupt_connect(unit, iph, INT_TO_PTR(unit)) < 0) {
            LOG_ERROR(BSL_LS_SOC_COMMON,
                      (BSL_META_U(unit,
                                  "soc_attach: could not connect interrupt line\n")));
            return SOC_E_INTERNAL;
        }
    }

    if (soc->soc_flags & SOC_F_ATTACHED) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META_U(unit,
                              "soc_attach: unit %d already attached\n"),
                              unit));
        return(SOC_E_NONE);
    }

    /*
     * Create mutexes first as these are needed even if initialization fails.
     */

    if ((soc->socControlMutex = sal_mutex_create("SOC_CONTROL")) == NULL) {
        goto error;
    }

    if ((soc->schanMutex = sal_mutex_create("SCHAN")) == NULL) {
        goto error;
    }

    if ((soc->ipArbiterMutex = sal_mutex_create("IPARBITER")) == NULL) {
        goto error;
    }

#ifdef BCM_CMICM_SUPPORT
    if (soc_property_get(unit, spn_FSCHAN_ENABLE, 0)) {
        if ((soc->fschanMutex = sal_mutex_create("FSCHAN")) == NULL) {
            goto error;
        }
    } else {
        soc->fschanMutex = NULL;
    }
#endif

    if ((soc->miimMutex = sal_mutex_create("MIIM")) == NULL) {
        goto error;
    }

    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
        /*
         * should only create mutexes for valid memories. 
         */
        if (!SOC_MEM_IS_VALID(unit, mem)) {
            continue;
        }
        act_mem = mem;
        lock_mem = mem;
#ifdef BCM_ESW_SUPPORT
        _SOC_DRV_MEM_REUSE_MEM_STATE(unit, act_mem, lock_mem);
#endif
        /* Handle many to one state sharing */
        if (((act_mem == mem && lock_mem == mem)
#ifdef BCM_ESW_SUPPORT
             || _SOC_DRV_MEM_IS_REUSED_MEM(unit, act_mem)
#endif
            ) && (soc->memState[lock_mem].lock == NULL)) {
            char mem_name[128];
#if !defined(SOC_NO_NAMES)
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || \
    defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
            sal_strncpy(mem_name, SOC_MEM_NAME(unit, act_mem),
                        sizeof(mem_name)-1);
#else
            sal_snprintf(mem_name, sizeof(mem_name), "%s%d",
                         "mem_mutex_", act_mem);
#endif
#else
            sal_snprintf(mem_name, sizeof(mem_name), "%s%d",
                         "mem_mutex_", act_mem);
#endif
            if ((soc->memState[lock_mem].lock =
                 sal_mutex_create(mem_name)) == NULL) {
                goto error;
            }
            /* Set cache copy pointers to NULL */
            sal_memset(soc->memState[act_mem].cache,
                       0, sizeof (soc->memState[act_mem].cache));
        }
        if ((act_mem != mem || lock_mem != mem)
#ifdef BCM_ESW_SUPPORT
            && !_SOC_DRV_MEM_IS_REUSED_MEM(unit, mem)
#endif
            ) {
            /* Share the lock */
            soc->memState[mem].lock = soc->memState[lock_mem].lock;
        }
    }

#ifdef INCLUDE_MEM_SCAN
    if ((soc->mem_scan_notify =
         sal_sem_create("memscan timer", sal_sem_BINARY, 0)) == NULL) {
        goto error;
    }

    soc->mem_scan_pid = SAL_THREAD_ERROR;
    soc->mem_scan_interval = 0;
#endif


#ifdef BCM_CMICM_SUPPORT
    soc->ccmDmaMutex = NULL;
    soc->ccmDmaIntr = NULL;
    if (soc_feature(unit, soc_feature_cmicm) &&
        soc_property_get(unit, spn_CCM_DMA_ENABLE, 0)) {
        if (SAL_BOOT_QUICKTURN) {
            soc->ccmDmaTimeout = CCMDMA_TIMEOUT_QT;
        } else {
            soc->ccmDmaTimeout = CCMDMA_TIMEOUT;
        }
        soc->ccmDmaTimeout = soc_property_get(unit, spn_CCMDMA_TIMEOUT_USEC,
                                                soc->ccmDmaTimeout);
        if (soc->ccmDmaTimeout) {
            soc->ccmDmaMutex = sal_mutex_create("ccmDMA");
            if (soc->ccmDmaMutex == NULL) {
                goto error;
            }
            soc->ccmDmaIntr = sal_sem_create("CCMDMA interrupt",
                                               sal_sem_BINARY, 0);
            if (soc->ccmDmaIntr == NULL) {
                goto error;
            }
            soc->ccmDmaIntrEnb = soc_property_get(unit,
                                                    spn_CCMDMA_INTR_ENABLE, 1);
        }
    }

    /* Cross-CPU communications with UCs */
    if (soc_feature(unit, soc_feature_cmicm)) {
        soc_cmic_uc_msg_start(unit);
    }

#endif /* CMICM Support */
    if (soc_feature(unit, soc_feature_mem_cmd)) {
        soc->memCmdTimeout = soc_property_get(unit,
                                                spn_MEMCMD_TIMEOUT_USEC,
                                                1000000);
        soc->memCmdIntrEnb = soc_property_get(unit,
                                                spn_MEMCMD_INTR_ENABLE, 0);
        for (ix = 0; ix < 3; ix++) {
            if ((soc->memCmdIntr[ix] =
                 sal_sem_create("MemCmd interrupt", sal_sem_BINARY, 0)) == NULL) {
                goto error;
            }
        }
    } else {
        for (ix = 0; ix < 3; ix++) {
            soc->memCmdIntr[ix] = 0;
        }
    }

    if (soc_feature(unit, soc_feature_ipfix)) {
        soc->ipfixIntrEnb =
            soc_property_get(unit, spn_IPFIX_INTR_ENABLE,
                             soc_feature(unit, soc_feature_fifo_dma) ? 1 : 0);
    }

    if (soc_feature(unit, soc_feature_regex)) {
        soc->ftreportIntrEnb =
            soc_property_get(unit, spn_FLOW_TRACKER_INTR_ENABLE, 1);
    }

    if (soc_feature(unit, soc_feature_fifo_dma)) {
        soc->l2modDmaIntrEnb =
            soc_property_get(unit, spn_L2MOD_DMA_INTR_ENABLE, 1);
    }

    if (soc_feature(unit, soc_feature_multi_sbus_cmds)) {
        soc->sbusCmdSpacing =
            soc_property_get(unit, "multi_sbus_cmds_spacing", -1);
    }

    if ((soc->counterMutex = sal_mutex_create("Counter")) == NULL) {
        goto error;
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if ((soc->esm_recovery_notify = sal_sem_create("etu recovery", 
        	                                         sal_sem_BINARY, 0)) == NULL) {
        goto error;
    }
    if ((soc->esm_lock = sal_mutex_create("esm")) == NULL) {
        goto error;
    }
    soc->esm_recovery_pid = SAL_THREAD_ERROR;
    soc->esm_recovery_enable = FALSE;
#endif

    soc->schan_override = 0;

    /*
     * Attached flag must be true during initialization.
     * If initialization fails, the flag is cleared by soc_detach (below).
     */

    soc->soc_flags |= SOC_F_ATTACHED;

    if (soc_ndev_attached++ == 0) {
        int                     chip;

        /* Work to be done before the first SOC device is attached. */
        for (chip = 0; chip < SOC_NUM_SUPPORTED_CHIPS; chip++) {
            /* Call each chip driver's init function */
            if (soc_base_driver_table[chip]->init) {
                (soc_base_driver_table[chip]->init)();
            }
        }
    }

    /*
     * Set up port bitmaps.  They are also set up on each soc_init so
     * they can be changed from a CLI without rebooting.
     */

    SOC_IF_ERROR_RETURN(soc_info_config(unit, soc));

    soc_dcb_unit_init(unit);

    soc->tableDmaMutex = NULL; /* marker for soc_mem_dmaable */
    soc->tableDmaIntr = NULL;
    table_dma_enable = soc_property_get(unit, spn_TABLE_DMA_ENABLE, 1);
    tslam_dma_enable = soc_property_get(unit, spn_TSLAM_DMA_ENABLE, 1);
#ifdef PLISIM
#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANAX(unit) && (SAL_BOOT_PLISIM || SAL_BOOT_BCMSIM)) {
       LOG_ERROR(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "WARNING:TABLE DMA is currently not working"
                             " so ignoring its setting %d\n"), table_dma_enable));
       LOG_ERROR(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "WARNING:TSLAM DMA is currently not working"
                             " so ignoring its setting %d\n"), tslam_dma_enable));
       table_dma_enable = 0;
       tslam_dma_enable = 0;
     }
#else
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "INFO:TABLE DMA setting %d \n"),table_dma_enable));
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "INFO:TSLAM DMA setting %d \n"),tslam_dma_enable));
#endif
#endif

#ifdef INCLUDE_RCPU
    if (SOC_IS_RCPU_ONLY(unit)) {
        table_dma_enable = 0;
        tslam_dma_enable = 0;
    }
#endif /* INCLUDE_RCPU */

    if (soc_feature(unit, soc_feature_table_dma) &&
        table_dma_enable) {
        if (SAL_BOOT_QUICKTURN) {
            soc->tableDmaTimeout = TDMA_TIMEOUT_QT;
        } else {
            soc->tableDmaTimeout = TDMA_TIMEOUT;
        }
        soc->tableDmaTimeout = soc_property_get(unit, spn_TDMA_TIMEOUT_USEC,
                                                soc->tableDmaTimeout);
        if (soc->tableDmaTimeout) {
            soc->tableDmaMutex = sal_mutex_create("TblDMA");
            if (soc->tableDmaMutex == NULL) {
                goto error;
            }
            soc->tableDmaIntr = sal_sem_create("TDMA interrupt",
                                               sal_sem_BINARY, 0);
            if (soc->tableDmaIntr == NULL) {
                goto error;
            }
            soc->tableDmaIntrEnb = soc_property_get(unit,
                                                    spn_TDMA_INTR_ENABLE, 1);
        }
    }

    soc->tslamDmaMutex = NULL; /* marker for soc_mem_write_dmaable */
    soc->tslamDmaIntr = NULL;
    if (soc_feature(unit, soc_feature_tslam_dma) &&
        tslam_dma_enable) {
        if (SAL_BOOT_QUICKTURN) {
            soc->tslamDmaTimeout = TSLAM_TIMEOUT_QT;
        } else {
            soc->tslamDmaTimeout = TSLAM_TIMEOUT;
        }
        soc->tslamDmaTimeout = soc_property_get(unit, spn_TSLAM_TIMEOUT_USEC,
                                                soc->tslamDmaTimeout);
        if (soc->tslamDmaTimeout) {
            soc->tslamDmaMutex = sal_mutex_create("TSlamDMA");
            if (soc->tslamDmaMutex == NULL) {
                goto error;
            }
            soc->tslamDmaIntr = sal_sem_create("TSLAM interrupt",
                                               sal_sem_BINARY, 0);
            if (soc->tslamDmaIntr == NULL) {
                goto error;
            }
            soc->tslamDmaIntrEnb = soc_property_get(unit,
                                                    spn_TSLAM_INTR_ENABLE, 1);
        }
    }

#ifdef BROADCOM_DEBUG
    /* must be after soc_info_config, due to use of data structures */
    if (!SOC_IS_HELIX4(unit)) {
        _soc_counter_verify(unit);
    }
#endif /* BROADCOM_DEBUG */

    soc_cm_get_id(unit, &dev_id, &rev_id);

    /*
     * Initialize memory index_maxes. Chip specific overrides follow.
     */
    SOC_CONTROL(unit)->tcam_protect_write =
        soc_property_get(unit, "tcam_protect_write", FALSE);
    enable = soc_property_get(unit, "force_read_through", FALSE);
    SOC_MEM_FORCE_READ_THROUGH_SET(unit, enable);
    enable = soc_property_get(unit, "cache_coherency_check", TRUE);
    SOC_MEM_CACHE_COHERENCY_CHECK_SET(unit, enable);
    SOC_CONTROL(unit)->dma_from_mem_cache =
        soc_property_get(unit, "dma_from_cache", TRUE);
    enable = soc_property_get(unit, "reg_ser_error_assert", FALSE);
    SOC_REG_RETURN_SER_ERROR_SET(unit, enable);
    SOC_SER_CORRECTION_SUPPORT(unit) =
        soc_property_get(unit, spn_PARITY_CORRECTION, 
                         (SAL_BOOT_RTLSIM || SAL_BOOT_XGSSIM) ? 0 : 1);
    SOC_SER_COUNTER_CORRECTION(unit) =
        soc_property_get(unit, spn_PARITY_COUNTER_CLEAR, 1) ? 0 : 1;
    SOC_CONTROL(unit)->l2e_ppa = soc_property_get(unit, "tr3_l2e_ppa", 1);
    for (mem = 0; mem < NUM_SOC_MEM; mem++) {
        if (!SOC_MEM_IS_VALID(unit, mem)) {
            sop->memState[mem].index_max = -1;
            continue;
        }
        sop->memState[mem].index_max = SOC_MEM_INFO(unit, mem).index_max;
        if (soc_feature(unit, soc_feature_xy_tcam) &&
            SOC_CONTROL(unit)->tcam_protect_write &&
            (SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_CAM) &&
            (!(SOC_MEM_INFO(unit, mem).flags & SOC_MEM_FLAG_EXT_CAM))) {
            sop->memState[mem].index_max--;
        }
#ifdef BCM_TRIUMPH3_SUPPORT
        if (SOC_IS_TRIUMPH3(unit) && 
            !((dev_id == BCM56640_DEVICE_ID) ||
             (dev_id == BCM56643_DEVICE_ID))) {
            int blk, copyno = COPYNO_ALL;
            SOC_MEM_BLOCK_ITER(unit, mem, blk) {
                copyno = blk;
                break;
            }
            /* Disable unavailable ESM and IESMIF mems */
            if ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_ETU) ||
                ((SOC_BLOCK_TYPE(unit, copyno) == SOC_BLK_IPIPE) &&
                 (SOC_MEM_ADDR_STAGE_EXTENDED(SOC_MEM_BASE(unit, mem))
                    == 0x9))) {
                sop->memState[mem].index_max = -1;
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */            
    }

    /* Need to know the bypass mode before resolving
     * which memories and features are enabled in this configuration. */
    SOC_SWITCH_BYPASS_MODE(unit) = SOC_IS_SC_CQ(unit) || SOC_IS_TD_TT(unit) ?
        soc_property_get(unit, spn_SWITCH_BYPASS_MODE,
                         SOC_SWITCH_BYPASS_MODE_NONE) :
                         SOC_SWITCH_BYPASS_MODE_NONE;

    /* Initialize double-tagged mode to FALSE */
    SOC_DT_MODE(unit) = 0;

#ifdef BCM_GREYHOUND_SUPPORT
    if (SOC_IS_GREYHOUND(unit) && ((dev_id & 0xF000) == 0x8000)){
        dev_id &= 0xFF0F;
    }
#endif

    switch (dev_id) {
#ifdef BCM_FIREBOLT_SUPPORT
    case BCM56404_DEVICE_ID:
        soc_helix_mem_config(unit);
        break;
    case BCM53300_DEVICE_ID:
    case BCM53301_DEVICE_ID:
    case BCM53302_DEVICE_ID:
        soc_bcm53300_mem_config(unit);
        break;
#endif /* BCM_FIREBOLT_SUPPORT */
#ifdef BCM_RAPTOR_SUPPORT
     case BCM53716_DEVICE_ID:
        sop->memState[FP_TCAMm].index_max = 255;
        sop->memState[FP_POLICY_TABLEm].index_max = 255;
        sop->memState[FP_METER_TABLEm].index_max = 255;
        sop->memState[FP_COUNTER_TABLEm].index_max = 255;
        break;
     case BCM53718_DEVICE_ID:
     case BCM53714_DEVICE_ID:
        sop->memState[FP_TCAMm].index_max = 511;
        sop->memState[FP_POLICY_TABLEm].index_max = 511;
        sop->memState[FP_METER_TABLEm].index_max = 511;
        sop->memState[FP_COUNTER_TABLEm].index_max = 511;
        break;
     case BCM53724_DEVICE_ID:
     case BCM53726_DEVICE_ID:
        soc_bcm53724_mem_config(unit);
        break;
     case BCM56225_DEVICE_ID:
     case BCM56227_DEVICE_ID:
     case BCM56229_DEVICE_ID:
        soc_bcm56225_mem_config(unit);
        break;
#endif /* BCM_RAPTOR_SUPPORT */
#ifdef BCM_FIREBOLT2_SUPPORT
#ifndef EXCLUDE_BCM56324
    case BCM56322_DEVICE_ID:
    case BCM56324_DEVICE_ID:
#endif /* EXCLUDE_BCM56324 */
    case BCM56510_DEVICE_ID:
    case BCM56511_DEVICE_ID:
    case BCM56512_DEVICE_ID:
    case BCM56513_DEVICE_ID:
    case BCM56514_DEVICE_ID:
    case BCM56516_DEVICE_ID:
    case BCM56517_DEVICE_ID:
    case BCM56518_DEVICE_ID:
    case BCM56519_DEVICE_ID:
        soc_firebolt2_mem_config(unit);
        break;
#endif /* BCM_FIREBOLT2_SUPPORT */
#ifdef BCM_TRIUMPH_SUPPORT
    case BCM56620_DEVICE_ID:
    case BCM56624_DEVICE_ID:
    case BCM56626_DEVICE_ID:
    case BCM56628_DEVICE_ID:
    case BCM56629_DEVICE_ID:
        soc_triumph_esm_init_mem_config(unit);
        break;
#ifdef BCM_TRIUMPH2_SUPPORT
    case BCM56634_DEVICE_ID:
    case BCM56636_DEVICE_ID:
    case BCM56638_DEVICE_ID:
    case BCM56630_DEVICE_ID:
    case BCM56639_DEVICE_ID:
    case BCM56538_DEVICE_ID:
        soc_triumph_esm_init_mem_config(unit);
        soc_triumph2_mem_config(unit);
        break;
#endif /* BCM_TRIUMPH2_SUPPORT */
#if defined(BCM_APOLLO_SUPPORT)
    case BCM56526_DEVICE_ID:
    case BCM56521_DEVICE_ID:
    case BCM56520_DEVICE_ID:
    case BCM56522_DEVICE_ID:
    case BCM56524_DEVICE_ID:
    case BCM56534_DEVICE_ID:
        soc_triumph2_mem_config(unit);
        break;
#endif /* BCM_APOLLO_SUPPORT */
#ifdef BCM_VALKYRIE2_SUPPORT
    case BCM56685_DEVICE_ID:
    case BCM56689_DEVICE_ID:
        soc_triumph2_mem_config(unit);
        break;
#endif /* BCM_VALKYRIE2_SUPPORT */
    case BCM56686_DEVICE_ID:
        sop->memState[FP_TCAMm].index_max = 2047;
        sop->memState[FP_POLICY_TABLEm].index_max = 2047;
        sop->memState[FP_METER_TABLEm].index_max = 2047;
        sop->memState[FP_COUNTER_TABLEm].index_max = 2047;
        sop->memState[FP_GLOBAL_MASK_TCAMm].index_max = 2047;

        sop->memState[EFP_TCAMm].index_max = 255;
        sop->memState[EFP_POLICY_TABLEm].index_max = 255;
        sop->memState[EFP_METER_TABLEm].index_max = 255;
        sop->memState[EFP_COUNTER_TABLEm].index_max = 255;

        sop->memState[VFP_TCAMm].index_max = 255;
        sop->memState[VFP_POLICY_TABLEm].index_max = 255;
        break;
#endif /* BCM_TRIUMPH_SUPPORT */
#ifdef BCM_SCORPION_SUPPORT
    case BCM56820_DEVICE_ID:
    case BCM56821_DEVICE_ID:
    case BCM56822_DEVICE_ID:
    case BCM56823_DEVICE_ID:
    case BCM56825_DEVICE_ID:
    case BCM56720_DEVICE_ID:
    case BCM56721_DEVICE_ID:
    case BCM56725_DEVICE_ID:
#ifdef BCM_SHADOW_SUPPORT
    case BCM88732_DEVICE_ID:
#endif /* BCM_SHADOW_SUPPORT */
        soc_scorpion_mem_config(unit);
        break;
#endif /* BCM_SCORPION_SUPPORT */
#ifdef BCM_ENDURO_SUPPORT
    case BCM56320_DEVICE_ID:
    case BCM56321_DEVICE_ID:
    case BCM56230_DEVICE_ID:
    case BCM56231_DEVICE_ID:
        soc_enduro_mem_config(unit);
        break;
#endif /* BCM_ENDURO_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    case BCM56240_DEVICE_ID:
    case BCM56241_DEVICE_ID:
    case BCM56242_DEVICE_ID:
    case BCM56243_DEVICE_ID:
    case BCM56245_DEVICE_ID:
    case BCM56246_DEVICE_ID:
        sop->memState[L2Xm].index_max = 16383;
        sop->memState[L2_ENTRY_ONLYm].index_max = 16383;
        sop->memState[L2_HITDA_ONLYm].index_max = 2047;
        sop->memState[L3_ENTRY_ONLYm].index_max = 4095;
        sop->memState[L3_ENTRY_HIT_ONLYm].index_max = 255;
        sop->memState[MPLS_ENTRYm].index_max = 1023;
        sop->memState[VLAN_XLATEm].index_max = 4095;
        sop->memState[VLAN_MACm].index_max = 4095;
        sop->memState[FP_COUNTER_TABLEm].index_max = 2047;
        sop->memState[FP_GLOBAL_MASK_TCAMm].index_max = 2047;
        sop->memState[FP_METER_TABLEm].index_max = 2047;
        sop->memState[FP_POLICY_TABLEm].index_max = 2047;
        sop->memState[FP_TCAMm].index_max = 2047;
        sop->memState[MMU_CBPI_0m].index_max = 2751;
        sop->memState[MMU_CBPI_1m].index_max = 2751;
        sop->memState[MMU_CBPI_2m].index_max = 2751;
        sop->memState[MMU_CBPI_3m].index_max = 2751;
        sop->memState[MMU_CBPI_4m].index_max = 2751;
        sop->memState[MMU_CBPI_5m].index_max = 2751;
        sop->memState[MMU_CBPI_6m].index_max = 2751;
        sop->memState[MMU_CBPI_7m].index_max = 2751;
        sop->memState[MMU_CBPI_8m].index_max = 2751;
        sop->memState[MMU_CBPI_9m].index_max = 2751;
        sop->memState[MMU_CBPI_10m].index_max = 2751;
        sop->memState[MMU_CBPI_11m].index_max = 2751;
        sop->memState[LLS_L0_CONFIGm].index_max = 127;
        sop->memState[LLS_L1_CONFIGm].index_max = 255;
        sop->memState[LLS_L2_PARENTm].index_max = 511;
        for(saber_counter_direction=0;
            saber_counter_direction<BCM_SABER_MAX_COUNTER_DIRECTION;
            saber_counter_direction++) {
            for(saber_counter_pool=0;
                saber_counter_pool<BCM_SABER_MAX_COUNTER_POOL;
                saber_counter_pool++) {
                sop->memState[saber_counter_table
                              [saber_counter_direction][saber_counter_pool]].
                              index_max = saber_pool_size
                                          [saber_counter_direction];
            }
        }
        break;    
    case BCM55441_DEVICE_ID:
        sop->memState[FP_COUNTER_TABLEm].index_max = 2047;
        sop->memState[FP_GLOBAL_MASK_TCAMm].index_max = 2047;
        sop->memState[FP_METER_TABLEm].index_max = 2047;
        sop->memState[FP_POLICY_TABLEm].index_max = 2047;
        sop->memState[FP_TCAMm].index_max = 2047;
        sop->memState[LLS_L0_CONFIGm].index_max = 255;
        sop->memState[LLS_L1_CONFIGm].index_max = 1023;
        sop->memState[LLS_L2_PARENTm].index_max = 1023;
        break;
#endif /* BCM_KATANA_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
    case BCM55450_DEVICE_ID:
    case BCM55455_DEVICE_ID:
    case BCM56248_DEVICE_ID:
    case BCM56450_DEVICE_ID:
    case BCM56452_DEVICE_ID:
    case BCM56454_DEVICE_ID:
    case BCM56455_DEVICE_ID:
    case BCM56456_DEVICE_ID:
    case BCM56457_DEVICE_ID:
    case BCM56458_DEVICE_ID:
        if (soc_kt2_mem_config(unit, dev_id) != SOC_E_NONE) {
            goto error;
        }
        break;
#endif
#ifdef BCM_HURRICANE2_SUPPORT
    case BCM56150_DEVICE_ID:
    case BCM56151_DEVICE_ID:
    case BCM53347_DEVICE_ID:
    case BCM53346_DEVICE_ID:
    case BCM53344_DEVICE_ID:
    case BCM56152_DEVICE_ID:
    case BCM53333_DEVICE_ID:
    case BCM53334_DEVICE_ID:
    case BCM53342_DEVICE_ID:
    case BCM53343_DEVICE_ID:
    case BCM53393_DEVICE_ID: 
    case BCM53394_DEVICE_ID:
        if (soc_hu2_mem_config(unit, dev_id) != SOC_E_NONE) {
            goto error;
        }
        break;
#endif
#ifdef BCM_HELIX4_SUPPORT
    case BCM56548_DEVICE_ID:
    case BCM56547_DEVICE_ID:
    case BCM56347_DEVICE_ID:
    case BCM56346_DEVICE_ID:
    case BCM56344_DEVICE_ID:
    case BCM56342_DEVICE_ID:
    case BCM56340_DEVICE_ID:
    case BCM56049_DEVICE_ID:
    case BCM56048_DEVICE_ID:
    case BCM56047_DEVICE_ID:
    case BCM56042_DEVICE_ID:
    case BCM56041_DEVICE_ID:
    case BCM56040_DEVICE_ID:
        if (soc_hx4_mem_config(unit, dev_id) != SOC_E_NONE) {
            goto error;
        }
        break;
#endif
#ifdef BCM_TRIUMPH3_SUPPORT
    case BCM56540_DEVICE_ID:
    case BCM56541_DEVICE_ID:
    case BCM56542_DEVICE_ID:
    case BCM56543_DEVICE_ID:
    case BCM56544_DEVICE_ID:
    case BCM56545_DEVICE_ID:
    case BCM56546_DEVICE_ID:
    case BCM56648_DEVICE_ID:
    case BCM56649_DEVICE_ID:
    case BCM56640_DEVICE_ID:
    case BCM56643_DEVICE_ID:
    case BCM56644_DEVICE_ID:
    case BCM56044_DEVICE_ID:
    case BCM56045_DEVICE_ID:
    case BCM56046_DEVICE_ID:
        soc_triumph3_esm_init_mem_config(unit);
        if (soc_tr3_mem_config(unit, dev_id, rev_id) != SOC_E_NONE) {
            goto error;
        }
        break;
#endif /* BCM_TRIUMPH3_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
    case BCM56838_DEVICE_ID:
    case BCM56847_DEVICE_ID:
    case BCM56835_DEVICE_ID:   
    case BCM56831_DEVICE_ID:          
        if (soc_trident_mem_config(unit) < 0) {
            goto error;
        }
        break;
#endif /* BCM_TRIDENT_SUPPORT */
#if defined(BCM_TRIDENT2_SUPPORT)
    case BCM56850_DEVICE_ID:
    case BCM56851_DEVICE_ID:
    case BCM56852_DEVICE_ID:
    case BCM56853_DEVICE_ID:
    case BCM56854_DEVICE_ID:
    case BCM56855_DEVICE_ID:
    case BCM56834_DEVICE_ID:           
    case BCM56750_DEVICE_ID:
    case BCM56830_DEVICE_ID:
        if (soc_trident2_mem_config(unit) < 0) {
            goto error;
        }
        break;
#endif /* BCM_TRIDENT2_SUPPORT */
#if defined(BCM_GREYHOUND_SUPPORT)
    case BCM53400_DEVICE_ID:
    case BCM53401_DEVICE_ID:
    case BCM53402_DEVICE_ID:
    case BCM53403_DEVICE_ID:
    case BCM53404_DEVICE_ID:
    case BCM53405_DEVICE_ID:
    case BCM53406_DEVICE_ID:
    case BCM53408_DEVICE_ID:
    case BCM56060_DEVICE_ID:
    case BCM56062_DEVICE_ID:
    case BCM56063_DEVICE_ID:
    case BCM56064_DEVICE_ID:
        if (soc_greyhound_mem_config(unit, dev_id) < 0) {
            goto error;
        }
        break;
#endif /* BCM_TRIDENT2_SUPPORT */

    default:
        break;
    }
#ifdef BCM_ESW_SUPPORT
    if (soc_feature(unit, soc_feature_ipmc_reduced_table_size)) {
        sop->memState[L3_IPMCm].index_max = 2047;
    }   
#endif /* BCM_ESW_SUPPORT */
#if defined(BCM_ESW_SUPPORT) || defined(BCM_SBX_SUPPORT) || \
    defined(BCM_PETRA_SUPPORT) || defined(BCM_DFE_SUPPORT)
#if !defined(SOC_NO_NAMES)
    if (soc_property_get(unit, spn_MEM_CHECK_MAX_OVERRIDE, 0)) {
        for (mem = 0; mem < NUM_SOC_MEM; mem++) {
            uint32      max;
            uint32      max_adj;
            char        mem_name[128];
            char        *mptr;
 
            if (!SOC_MEM_IS_VALID(unit, mem)) {
                continue;
            }
            max = sop->memState[mem].index_max;
 
            sal_strcpy(mem_name, "memmax_");
            mptr = &mem_name[sal_strlen(mem_name)];
            sal_strcpy(mptr, SOC_MEM_NAME(unit, mem));
            max_adj = soc_property_get(unit, mem_name, max);
            if (max_adj == max) {
                sal_strcpy(mptr, SOC_MEM_UFNAME(unit, mem));
                max_adj = soc_property_get(unit, mem_name, max);
            }
            if (max_adj == max) {
                sal_strcpy(mptr, SOC_MEM_UFALIAS(unit, mem));
                max_adj = soc_property_get(unit, mem_name, max);
            }
            sop->memState[mem].index_max = max_adj;
            
        }
    }
#endif /* !SOC_NO_NAMES */
#endif
    /* Allocate counter module resources */
    if (soc_counter_attach(unit)) {
        goto error;
    }

    /* Allocate MMU LLS usage map */
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) {
        if (soc_trident2_lls_bmap_alloc(unit) < 0) {
            goto error;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Allocate L2X module and shadow table resources */

#ifdef BCM_XGS_SWITCH_SUPPORT
    if (soc_feature(unit, soc_feature_arl_hashed) &&
        soc_l2x_attach(unit) < 0) {
        goto error;
    }
#endif

    /*
     * Create binary semaphores for interrupt signals, initially empty
     * making us block when we try to "take" on them.  In soc_intr(),
     * when we receive the interrupt, a "give" is performed, which will
     * wake us back up.
     */

    if ((soc->schanIntr =
         sal_sem_create("SCHAN interrupt", sal_sem_BINARY, 0)) == NULL) {
        goto error;
    }

    if ((soc->miimIntr =
         sal_sem_create("MIIM interrupt", sal_sem_BINARY, 0)) == NULL) {
        goto error;
    }

    /* Initialize target device */

    if (SAL_BOOT_QUICKTURN) {
        soc->schanTimeout = SCHAN_TIMEOUT_QT;
        soc->miimTimeout = MIIM_TIMEOUT_QT;
        soc->bistTimeout = BIST_TIMEOUT_QT;
    } else if (SAL_BOOT_PLISIM) {
        soc->schanTimeout = SCHAN_TIMEOUT_PLI;
        soc->miimTimeout = MIIM_TIMEOUT_PLI;
        soc->bistTimeout = BIST_TIMEOUT_PLI;
    } else {
        soc->schanTimeout = SCHAN_TIMEOUT;
        soc->miimTimeout = MIIM_TIMEOUT;
        soc->bistTimeout = BIST_TIMEOUT;
    }

    soc->schanTimeout = soc_property_get(unit, spn_SCHAN_TIMEOUT_USEC,
                                         soc->schanTimeout);
    soc->miimTimeout = soc_property_get(unit, spn_MIIM_TIMEOUT_USEC,
                                        soc->miimTimeout);
    soc->bistTimeout = soc_property_get(unit, spn_BIST_TIMEOUT_MSEC,
                                        soc->bistTimeout);

    soc->schanIntrEnb = soc_property_get(unit, spn_SCHAN_INTR_ENABLE, 1);
    soc->schanIntrBlk = soc_property_get(unit, spn_SCHAN_ERROR_BLOCK_USEC,
                                         250000);

#ifdef INCLUDE_RCPU
    if (SOC_IS_RCPU_ONLY(unit)) {
        /* 
         * Use polling mode for RCPU by defalut. If remote interrupt packet is
         * enabled for RCPU MIIM access, the register CMIC_INTR_PKT_PACING_DELAY
         * should be programmed properly to allow continuous generation of
         * interrupt packets. 
         */
        soc->miimTimeout = MIIM_TIMEOUT_RCPU;
        soc->miimIntrEnb = soc_property_get(unit, spn_MIIM_INTR_ENABLE, 0);
    } else
#endif /* INCLUDE_RCPU */
    {
        /* use polling mode as default during init. Expect to save time */
        soc->miimIntrEnb = soc_property_get(unit, spn_MIIM_INTR_ENABLE, 0);
    }

    /* arl_notify is also used for Table DMA on 5690 */

    if ((soc->arl_notify =
        sal_sem_create("ARL interrupt", sal_sem_BINARY, 0)) == NULL) {
        goto error;
    }

    soc->arl_notified = 0;

    soc->arlDropWarn = 0;

    soc->arlResyncDelay =
        soc_property_get(unit, spn_ARL_RESYNC_DELAY, 3000000);

#ifdef BCM_XGS_SWITCH_SUPPORT
    if ((soc->l2x_notify =
        sal_sem_create("l2xmsg timer", sal_sem_BINARY, 0)) == NULL) {
        goto error;
    }
    if ((soc->l2x_del_sync =
        sal_mutex_create("l2xmsg delete sync")) == NULL) {
        goto error;
    }
    if ((soc->l2x_lock =
        sal_sem_create("l2xmsg lock", sal_sem_BINARY, 1)) == NULL) {
        goto error;
    }
    soc->l2x_pid = SAL_THREAD_ERROR;
    soc->l2x_interval = 0;
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit) || SOC_IS_KATANA2(unit)) {
        if ((soc->l2x_age_notify =
            sal_sem_create("l2xAge timer", sal_sem_BINARY, 0)) == NULL) {
            goto error;
        }
        soc->l2x_age_pid = SAL_THREAD_ERROR;
        soc->l2x_age_interval = 0;
        soc->l2x_agetime_adjust_usec = 0;
        soc->l2x_agetime_curr_timeblk = 0;
        soc->l2x_agetime_curr_timeblk_usec = 0;
        soc->l2x_prev_age_timeout= 0;
    }
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TRIDENT2(unit)) {
        if ((soc->l2x_age_notify =
            sal_sem_create("l2xAge timer", sal_sem_BINARY, 0)) == NULL) {
            goto error;
        }
        if ((soc->l2x_age_sync =
            sal_sem_create("l2xAge sync", sal_sem_BINARY, 1)) == NULL) {
            goto error;
        }
        soc->l2x_age_pid = SAL_THREAD_ERROR;
        soc->l2x_age_interval = 0;
        soc->l2x_age_enable = 0;
        if (soc_property_get(unit, spn_RUN_L2_SW_AGING, 0)) {
            soc->l2x_sw_aging = 1;
        }
    }
#endif
#endif

    /* Create L3 module protection mutex. */
    if ((soc->l3x_lock = sal_mutex_create("L3 module mutex")) == NULL) {
        goto error;
    }

#ifdef BCM_SBUSDMA_SUPPORT
    if (soc_feature(unit, soc_feature_sbusdma)) {
        if (soc_sbusdma_desc_init(unit, soc_property_get(unit, spn_DMA_DESC_TIMEOUT_USEC, 0),
                                  soc_property_get(unit, spn_DMA_DESC_INTR_ENABLE, 0))) {
            goto error;
        }
    }
#endif

    soc->arl_pid = SAL_THREAD_ERROR;
    soc->arl_dma_cnt0 = 0;
    soc->arl_dma_xfer = 0;
    soc->arl_mbuf_done = 0;
    soc->arl_msg_drop = 0;
    soc->arl_exit = 1;
    soc->arlNextBuf = 0;
    soc->arlRateLimit = soc_property_get(unit, spn_ARL_RATE_LIMIT, 3000);

    if ((soc->ipfixIntr =
         sal_sem_create("IPFIX interrupt", sal_sem_BINARY, 0)) == NULL) {
        goto error;
    }

#if defined(INCLUDE_REGEX)
    if ((soc->ftreportIntr =
         sal_sem_create("Regex interrupt", sal_sem_BINARY, 0)) == NULL) {
        goto error;
    }
#endif

#ifdef BCM_HERCULES_SUPPORT
    soc->mmu_errors = NULL;

    if (SOC_IS_HERCULES(unit)) {
        int port, entry_count;
        if ((soc->lla_cells_good =
             sal_alloc(SOC_MAX_NUM_PORTS * sizeof(uint32),
                       "LLA good cells count")) == NULL) {
            goto error;
        }

        if ((soc->lla_map =
             sal_alloc(SOC_MAX_NUM_PORTS * sizeof(SHR_BITDCL *),
                       "LLA map cache pointers")) == NULL) {
            goto error;
        }

        if ((soc->sbe_disable =
             sal_alloc(SOC_MAX_NUM_PORTS * sizeof(int),
                       "PP SBE intr port disable")) == NULL) {
            goto error;
        }

        entry_count = soc_mem_index_count(unit, MEM_LLAm);
        PBMP_ALL_ITER(unit, port) {
            soc->lla_cells_good[port] = entry_count;
            soc->lla_map[port] = NULL;
            soc->sbe_disable[port] = FALSE;
        }
    } else {
        soc->lla_cells_good = NULL;
        soc->lla_map = NULL;
        soc->sbe_disable = NULL;
    }
#endif

    /* Clear statistics */

    sal_memset(&soc->stat, 0, sizeof (soc->stat));

    /* Initialize link forwarding bitmaps */

    SOC_PBMP_CLEAR(sop->link_fwd);

#ifdef INCLUDE_RCPU
    if (SOC_IS_RCPU_ONLY(unit)) {
        SOC_PBMP_PORT_ADD(sop->link_fwd, CMIC_PORT(unit));
        if (SOC_PORT_VALID(unit, RCPU_PORT(unit))) {
            SOC_PBMP_PORT_ADD(sop->link_fwd, RCPU_PORT(unit));
        }
    }
#endif /* INCLUDE_RCPU */

    soc->link_mask2 = PBMP_ALL(unit);

    soc->soc_link_pause = 0;

    /*
     * Configure nominal IPG register settings.
     * By default the IPG should be 96 bit-times.
     */
    if (SOC_IS_KATANA2(unit)) {
        /* SOC_PBMP_ASSIGN(pbmp_all,PBMP_ALL(unit)); */
        for (port =  SOC_INFO(unit).cmic_port;
             port <= SOC_INFO(unit).lb_port;
             port++) {
             SOC_PBMP_PORT_ADD(pbmp_all,port);
        } 
    } else {
        SOC_PBMP_ASSIGN(pbmp_all,PBMP_ALL(unit));
    }
    /* PBMP_ALL_ITER(unit, port) { */
    SOC_PBMP_ITER(pbmp_all, port) { 
        sop->ipg[port].hd_10    = 96;
        sop->ipg[port].hd_100   = 96;
        sop->ipg[port].hd_1000  = 96;
        sop->ipg[port].hd_2500  = 96;

        sop->ipg[port].fd_10    = 96;
        sop->ipg[port].fd_100   = 96;
        sop->ipg[port].fd_1000  = 96;
        sop->ipg[port].fd_2500  = 96;
        sop->ipg[port].fd_10000 = 96;
        sop->ipg[port].fd_xe    = 96;
        sop->ipg[port].fd_hg    = 64;
    }

    /*
     * Initialize CES control structs
     */
#if defined(BCM_KATANA_SUPPORT)
    soc->ces_ctrl = (void*)NULL;
    soc->tdm_ctrl = (void*)NULL;
#endif

    LOG_CLI((BSL_META_U(unit,
                        "SOC unit %d attached to %s device %s\n"),
             unit, (soc->soc_flags & SOC_F_RCPU_ONLY) ? "RCPU" : "PCI",
             soc_dev_name(unit)));

    return(SOC_E_NONE);

 error:
    LOG_ERROR(BSL_LS_SOC_COMMON,
              (BSL_META_U(unit,
                          "soc_attach: unit %d failed\n"), unit));

    soc_detach(unit);           /* Perform necessary clean-ups on error */
    return rv;
}

/*
 * Function:
 *      soc_bpdu_addr_set
 * Purpose:
 *      Set BPDU address
 * Parameters:
 *      unit - StrataSwitch unit #
 *      index - bpdu index
 *      addr - mac address
 * Returns:
 *      SOC_E_XXX - on error
 *      SOC_E_NONE - success
 */
int
soc_bpdu_addr_set(int unit, int index, sal_mac_addr_t addr)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        l2u_entry_t entry;
        uint32 mask[3];
        int     skip_l2u;

        skip_l2u = soc_property_get(unit, spn_SKIP_L2_USER_ENTRY, 0);
        if (skip_l2u) {
            return SOC_E_UNAVAIL;
        }

        if ((index < 0) ||
            (index >= soc_mem_index_count(unit, L2_USER_ENTRYm))) {
            return SOC_E_PARAM;
        }
        sal_memset(&entry, 0, sizeof(entry));
        soc_L2_USER_ENTRYm_field32_set(unit, &entry, VALIDf, 1);
        soc_L2_USER_ENTRYm_field32_set(unit, &entry, CPUf, 1);
        soc_L2_USER_ENTRYm_field32_set(unit, &entry, BPDUf, 1);
        soc_mem_mac_addr_set(unit, L2_USER_ENTRYm, &entry, MAC_ADDRf, addr);
        mask[0] = 0xffffffff;
        mask[1] = 0x0000ffff;
        mask[2] = 0;
        soc_L2_USER_ENTRYm_field_set(unit, (uint32 *)&entry, MASKf, mask);
#if defined(BCM_RAVEN_SUPPORT)
        if (soc_mem_field_valid(unit, L2_USER_ENTRYm, RESERVED_MASKf)) {
            soc_mem_field32_set(unit, L2_USER_ENTRYm, &entry, RESERVED_MASKf, 0);
        }
        if (soc_mem_field_valid(unit, L2_USER_ENTRYm, RESERVED_KEYf)) {
            soc_mem_field32_set(unit, L2_USER_ENTRYm, &entry, RESERVED_KEYf, 0);
        }
#endif /* BCM_RAVEN_SUPPORT */
        SOC_IF_ERROR_RETURN(soc_l2u_insert(unit, &entry, index, &index));
        return SOC_E_NONE;
    }
#endif
    return SOC_E_UNAVAIL;
}

int
soc_bpdu_addr_get(int unit, int index, sal_mac_addr_t *addr)
{
#ifdef BCM_XGS3_SWITCH_SUPPORT
    if (SOC_IS_XGS3_SWITCH(unit)) {
        l2u_entry_t entry;
        int     skip_l2u;

        skip_l2u = soc_property_get(unit, spn_SKIP_L2_USER_ENTRY, 0);
        if (skip_l2u) {
            return SOC_E_UNAVAIL;
        }

        if ((index < 0) ||
            (index >= soc_mem_index_count(unit, L2_USER_ENTRYm))) {
            return SOC_E_PARAM;
        }
        SOC_IF_ERROR_RETURN(soc_l2u_get(unit, &entry, index));
        if ((0 == soc_L2_USER_ENTRYm_field32_get(unit, &entry, VALIDf)) ||
             (0 == soc_L2_USER_ENTRYm_field32_get(unit, &entry, BPDUf))) {
            /* Not a valid BPDU entry */
            return SOC_E_NOT_FOUND;
        }
        soc_mem_mac_addr_get(unit, L2_USER_ENTRYm, &entry, MAC_ADDRf, *addr);
        return SOC_E_NONE;
    }
#endif
    return SOC_E_UNAVAIL;
}

/*
 * Function:
 *      soc_esw_hw_qnum_get
 * Purpose:
 *      Get queue to use for PBSMH packet transmission.
 * Parameters:
 *      unit - SOC unit #
 *      port - SOC port #
 *      cos - CoS (from Tx packet)
 *      qnum - (OUT) H/W queue to use
 * Returns:
 *      SOC_E_XXX
 */
int
soc_esw_hw_qnum_get(int unit, int port, int cos, int *qnum)
{
    switch (SOC_DCB_TYPE(unit)) {
#ifdef BCM_KATANA_SUPPORT
    case 24:
    case 29:
        *qnum = SOC_INFO(unit).port_cosq_base[port] + cos;
        break;
#endif
#ifdef BCM_TRIUMPH3_SUPPORT
    case 23:
        *qnum = SOC_INFO(unit).port_uc_cosq_base[port] + cos;
        break;
#endif
#ifdef BCM_TRIDENT2_SUPPORT
    case 26:
        *qnum = SOC_INFO(unit).port_uc_cosq_base[port] + cos;
        *qnum = soc_td2_logical_qnum_hw_qnum(unit, port, *qnum, 1);
        break;
#endif
    default:
        *qnum = cos;
        break;
    }
    return SOC_E_NONE;
}

#ifdef INCLUDE_RCPU
/*
 * Function:
 *      soc_rcpu_schan_op_register
 * Purpose:
 *      Registers SCHAN operation routine over RCPU mechanism
 * Parameters:
 *      unit - SOC unit #
 *      f    - (IN) Function to be called for SCHAN operations over RCPU
 * Returns:
 *      SOC_E_XXX
 */
int
soc_rcpu_schan_op_register(int unit, soc_rcpu_schan_op_cb f)
{
    if (!SOC_IS_RCPU_UNIT(unit) && !SOC_IS_RCPU_ONLY(unit)) {
        return SOC_E_UNAVAIL;
    }

    if (NULL == f) {
        return SOC_E_PARAM;
    }

    SOC_CONTROL(unit)->soc_rcpu_schan_op = f;

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_rcpu_schan_op_unregister
 * Purpose:
 *      Unregisters SCHAN operation routine over RCPU mechanism
 *      Schan will be performed via normal flow (PCI or EB)
 * Parameters:
 *      unit - SOC unit #
 * Returns:
 *      SOC_E_XXX
 */
int
soc_rcpu_schan_op_unregister(int unit)
{
    if (!SOC_IS_RCPU_UNIT(unit) && !SOC_IS_RCPU_ONLY(unit)) {
        return SOC_E_UNAVAIL;
    }

    SOC_CONTROL(unit)->soc_rcpu_schan_op = NULL;

    return SOC_E_NONE;
}

#endif /* INCLUDE_RCPU */

#ifdef BCM_COSQ_HIGIG_MAP_DISABLE
static const soc_field_t cos_field[8] = {
    COS0f,  COS1f,  COS2f,  COS3f,  COS4f,  COS5f,  COS6f,  COS7f
};

/*
 * Function:
 *      soc_cosq_higig_map_disable
 * Purpose:
 *      Initialize the identity mapping for ipics
 * Parameters:
 *      unit - SOC unit #
 * Returns:
 *      SOC_E_XXX
 */
int
soc_cosq_stack_port_map_disable(int unit)
{
    soc_port_t  port;
    uint64  val64;
    int         prio;

    COMPILER_64_ZERO(val64);
    for (prio = 0; prio < 8; prio++) {
        soc_reg64_field32_set(unit, ICOS_SELr, &val64,
                              cos_field[prio], prio);
    }
    PBMP_ST_ITER(unit, port) {
        /* map prio0->cos0, prio1->cos1, ... , prio7->cos7 */
        SOC_IF_ERROR_RETURN(WRITE_ICOS_SELr(unit, port, val64));
    }
    if (SOC_IS_XGS3_SWITCH(unit)) {
        SOC_IF_ERROR_RETURN(WRITE_ICOS_SELr(unit, (CMIC_PORT(unit)), val64));
    }
    return SOC_E_NONE;
}
#endif /* BCM_COSQ_HIGIG_MAP_DISABLE */

#ifdef BCM_XGS3_SWITCH_SUPPORT
static const soc_field_t prio_field[8] = {
    PRI0f,  PRI1f,  PRI2f,  PRI3f,  PRI4f,  PRI5f,  PRI6f,  PRI7f
};

/*
 * Function:
 *      soc_cpu_priority_mapping_init
 * Purpose:
 *      Initialize the identity mapping for ipics
 * Parameters:
 *      unit - SOC unit #
 * Returns:
 *      SOC_E_XXX
 */
int
soc_cpu_priority_mapping_init(int unit)
{
    int         prio;
    uint32  val;

    val = 0;
    for (prio = 0; prio < 8; prio++) {
        soc_reg_field_set(unit, CPU_PRIORITY_SELr, &val,
                              prio_field[prio], prio);
    }
    SOC_IF_ERROR_RETURN(WRITE_CPU_PRIORITY_SELr(unit, val));
    return SOC_E_NONE;
}
#endif /* BCM_XGS3_SWITCH_SUPPORT */

int soc_esw_div64(uint64 x, uint32 y, uint32 *result)
{
    uint64 rem;
    uint64 b;
    uint64 res, d;
    uint32 high;

    COMPILER_64_SET(rem, COMPILER_64_HI(x), COMPILER_64_LO(x));
    COMPILER_64_SET(b, 0, y);
    COMPILER_64_SET(d, 0, 1);

    high = COMPILER_64_HI(rem);

    COMPILER_64_ZERO(res);
    if (high >= y) {
        /* NOTE: Follow code is used to handle 64bits result
         *  high /= y;
         *  res = (uint64_t) (high << 32);
         *  rem -= (uint64_t)((high * y) << 32);
         */
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("soc_sbx_div64: result > 32bits\n")));
        return SOC_E_PARAM;
    }

    while ((!COMPILER_64_BITTEST(b, 63)) &&
           (COMPILER_64_LT(b, rem)) ) {
        COMPILER_64_ADD_64(b,b);
        COMPILER_64_ADD_64(d,d);
    }

    do {
        if (COMPILER_64_GE(rem, b)) {
            COMPILER_64_SUB_64(rem, b);
            COMPILER_64_ADD_64(res, d);
        }
        COMPILER_64_SHR(b, 1);
        COMPILER_64_SHR(d, 1);
    } while (!COMPILER_64_IS_ZERO(d));

    *result = COMPILER_64_LO(res);

    /*
    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "%s: divisor 0x%x%8.8x dividor 0x%x result 0x%x\n"),
                 FUNCTION_NAME(), high, COMPILER_64_LO(x), y, *result));
    */
    return 0;
}


/*
 * Function:
 *      soc_warpcore_firmware_set
 * Purpose:
 *      Load Warpcore PHY firmware using hardware acceleration
 * Parameters:
 *      unit - unit number
 *      port - device port number
 *      array - (IN) Warpcore firmware data stream pointer
 *      datalen - length of firmware data stream
 *      wc_instance - number of the Warpcore in this port block
 *      wc_ucmem_data - Device specific memory for loading WC firmware
 *      wc_ucmem_ctrl - Device specific register for selecting WC port
 * Returns:
 *      SOC_E_XXX
 */
int
soc_warpcore_firmware_set(int unit, int port,
                          uint8 *array, int datalen, int wc_instance,
                          soc_mem_t wc_ucmem_data, soc_reg_t wc_ucmem_ctrl)
{
    soc_control_t       *soc;
    int entry_bytes, entry_num;
    int blk;
    int count, extra_bytes, i, j;
    uint8 *array_ptr, *dma_buf_ptr;
    int arr_pos_le[3] [16]
        = {{15, 14, 13, 12, 11, 10,  9,  8, 7, 6, 5, 4, 3, 2, 1, 0},
           { 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7},
           { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};
    int arr_pos_be[3] [16]
        = {{12, 13, 14, 15,  8,  9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3},
            {11, 10,  9,  8, 15, 14, 13, 12, 3, 2, 1, 0, 7, 6, 5, 4},
            {3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12}};
    int * arr_pos;
    int pio, packet, other;

    soc = SOC_CONTROL(unit);

    entry_bytes = soc_mem_entry_bytes(unit, wc_ucmem_data);
    entry_num = soc_mem_index_count(unit, wc_ucmem_data);
    if (datalen > (entry_bytes * entry_num)) {
        /* Can't fit all of the firmware into the device load table. */
        return SOC_E_RESOURCE;
    }

    soc_cm_get_endian(unit, &pio, &packet, &other);
    if(other) {
        if (SOC_IS_GREYHOUND(unit)){
            arr_pos = arr_pos_be[2];
        } else if (SOC_IS_TD2_TT2(unit)) {
            arr_pos = arr_pos_be[1];
        } else {
            arr_pos = arr_pos_be[0];
        }
    } else {
        if (SOC_IS_GREYHOUND(unit)){
            arr_pos = arr_pos_le[2];
        } else if (SOC_IS_TD2_TT2(unit)) {
            arr_pos = arr_pos_le[1];
        } else {
            arr_pos = arr_pos_le[0];
        }
    }
    if (soc->soc_wc_ucode_dma_buf == NULL) {
        count = datalen / entry_bytes;
        extra_bytes = datalen % entry_bytes;
        soc->soc_wc_ucode_alloc_size = datalen;
        if (extra_bytes != 0) {
            soc->soc_wc_ucode_alloc_size += entry_bytes - extra_bytes;
        }
        soc->soc_wc_ucode_dma_buf =
            soc_cm_salloc(unit, soc->soc_wc_ucode_alloc_size,
                          "WC ucode DMA buffer");
        if (soc->soc_wc_ucode_dma_buf == NULL) {
            return SOC_E_MEMORY;
        }

        /*
         * The byte order in each 128-bit *_WC_UCMEM_DATA entry is:
         * bit position: (bit 31-0)  - (bit 63-32) - (bit 95-64) - (bit 127-96)
         * byte offset:  0c 0d 0e 0f - 08 09 0a 0b - 04 05 06 07 - 00 01 02 03
         *
         * For Trident2:
         * bit position: (bit 31-0)  - (bit 63-32) - (bit 95-64) - (bit 127-96)
         * byte offset:  0b 0a 09 08 - 0f 0e 0d 0c - 03 02 01 00 - 07 06 05 04
         */
        array_ptr = array;
        dma_buf_ptr = soc->soc_wc_ucode_dma_buf;
        for (i = 0; i < count; i++) {
            for (j = 0; j < 16; j++) {
                dma_buf_ptr[arr_pos[j]] = array_ptr[j];
            }
            array_ptr += entry_bytes;
            dma_buf_ptr += entry_bytes;
        }
        if (extra_bytes != 0) {
            sal_memset(dma_buf_ptr, 0, entry_bytes);
            for (j = 0; j < extra_bytes; j++) {
                dma_buf_ptr[arr_pos[j]] = array_ptr[j];
            }
        }
    }

    /* enable parallel bus access */
    SOC_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, wc_ucmem_ctrl, port,
                                ACCESS_MODEf, 1));
    if (soc_reg_field_valid(unit, wc_ucmem_ctrl, INST_SELECTf)) {
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, wc_ucmem_ctrl, port,
                                    INST_SELECTf, wc_instance));
    }
    if (soc_reg_field_valid(unit, wc_ucmem_ctrl, WR_BROADCASTf)) {
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, wc_ucmem_ctrl, port,
                                    WR_BROADCASTf,
                                    IS_C_PORT(unit, port) ? 1 : 0));
    }

#if defined(BCM_GREYHOUND_SUPPORT)
    if (SOC_IS_GREYHOUND(unit)){
        int phy_port;

        phy_port = SOC_INFO(unit).port_l2p_mapping[port];
        SOC_IF_ERROR_RETURN(_soc_gh_sbus_tsc_block(unit, 
                        phy_port, wc_ucmem_data, &blk));
    } else 
#endif /* BCM_GREYHOUND_SUPPORT */
    {
    blk = SOC_PORT_BLOCK(unit, SOC_INFO(unit).port_l2p_mapping[port]);
    }

    SOC_IF_ERROR_RETURN
        (soc_mem_write_range(unit, wc_ucmem_data, blk, 0,
                             soc->soc_wc_ucode_alloc_size / entry_bytes - 1,
                             soc->soc_wc_ucode_dma_buf));

    /* disable parallel bus access, and enable MDIO access */
    SOC_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, wc_ucmem_ctrl, port,
                                ACCESS_MODEf, 0));

    return SOC_E_NONE;
}

STATIC uint32
soc_sbus_mdio_addr(uint32 phy_addr, uint32 phy_reg)
{
    int devad;
    uint32 sbus_mdio_addr;

    /* Extract lane and DEVAD from address extension */
    devad = (phy_reg >> 27) & 0x1f;

    /* Build WC/TSC PHY register address */
    sbus_mdio_addr = (phy_reg & 0xffff);
    sbus_mdio_addr |= ((phy_addr & 0x1f) << 19);
    sbus_mdio_addr |= (devad << 27);

    return sbus_mdio_addr;
}

STATIC uint32
soc_sbus_mdio_lane(uint32 phy_reg)
{
    /* Extract lane from address extension */
    return (phy_reg >> 16) & 0x7;
}

int
soc_sbus_mdio_reg_read(int unit, int port, int blk, int wc_instance,
                       uint32 phy_addr, uint32 phy_reg, uint32 *phy_data,
                       soc_mem_t wc_ucmem_data, soc_reg_t wc_ucmem_ctrl)
{
    int rv = SOC_E_NONE;
    int entry_bytes;
    uint32 ucmem_entry[16];

    entry_bytes = soc_mem_entry_bytes(unit, wc_ucmem_data);
    if (entry_bytes > sizeof(ucmem_entry)) {
        return SOC_E_PARAM;
    }

    sal_memset(ucmem_entry, 0, sizeof(ucmem_entry));

    soc_mem_lock(unit, wc_ucmem_data);

    if (soc_reg_field_valid(unit, wc_ucmem_ctrl, INST_SELECTf)) {
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, wc_ucmem_ctrl, port,
                                    INST_SELECTf, wc_instance));
    }

    /* Write AER register */
    ucmem_entry[0] = soc_sbus_mdio_addr(phy_addr, 0xffde);
    ucmem_entry[1] = (soc_sbus_mdio_lane(phy_reg) << 16);
    ucmem_entry[2] = 1; /* Write register */
    rv = soc_mem_write(unit, wc_ucmem_data, blk, 0, &ucmem_entry);

    /* Setup PHY register address */
    ucmem_entry[0] = soc_sbus_mdio_addr(phy_addr, phy_reg);
    ucmem_entry[2] = 0; /* Read register */
    if (SOC_SUCCESS(rv)) {
        rv = soc_mem_write(unit, wc_ucmem_data, blk, 0, &ucmem_entry);
    }

    /* Read PHY register contents */
    if (SOC_SUCCESS(rv)) {
        rv = soc_mem_read(unit, wc_ucmem_data, blk, 0, &ucmem_entry);
    }

    soc_mem_unlock(unit, wc_ucmem_data);

    *phy_data = ucmem_entry[0];

    LOG_INFO(BSL_LS_SOC_MII,
             (BSL_META_U(unit,
                         "soc_sbus_mdio_reg_read[%d]: "
                         "addr=0x%x reg=0x%08x data=0x%04x (%d/%d/%d/%d)\n"),
              unit, phy_addr, phy_reg, *phy_data,
              port, blk, wc_instance, rv));

    return rv;
}

int
soc_sbus_mdio_reg_write(int unit, int port, int blk, int wc_instance,
                        uint32 phy_addr, uint32 phy_reg, uint32 phy_data,
                        soc_mem_t wc_ucmem_data, soc_reg_t wc_ucmem_ctrl)
{
    int rv = SOC_E_NONE;
    int entry_bytes;
    uint32 ucmem_entry[16];

    entry_bytes = soc_mem_entry_bytes(unit, wc_ucmem_data);
    if (entry_bytes > sizeof(ucmem_entry)) {
        return SOC_E_PARAM;
    }

    /* If write mask (upper 16 bits) is empty, add full mask */
    if ((phy_data & 0xffff0000) == 0) {
        phy_data |= 0xffff0000;
    }
    sal_memset(ucmem_entry, 0, sizeof(ucmem_entry));

    soc_mem_lock(unit, wc_ucmem_data);

    if (soc_reg_field_valid(unit, wc_ucmem_ctrl, INST_SELECTf)) {
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, wc_ucmem_ctrl, port,
                                    INST_SELECTf, wc_instance));
    }

    /* Write AER register */
    ucmem_entry[0] = soc_sbus_mdio_addr(phy_addr, 0xffde);
    ucmem_entry[1] = (soc_sbus_mdio_lane(phy_reg) << 16);
    ucmem_entry[2] = 1; /* Write register */
    rv = soc_mem_write(unit, wc_ucmem_data, blk, 0, &ucmem_entry);

    /* Write PHY register */
    if (SOC_SUCCESS(rv)) {
        ucmem_entry[0] = soc_sbus_mdio_addr(phy_addr, phy_reg);
        ucmem_entry[1] = (phy_data << 16);
        rv = soc_mem_write(unit, wc_ucmem_data, blk, 0, &ucmem_entry);
    }

    soc_mem_unlock(unit, wc_ucmem_data);

    LOG_INFO(BSL_LS_SOC_MII,
             (BSL_META_U(unit,
                         "soc_sbus_mdio_reg_write[%d]: "
                         "addr=0x%x reg=0x%08x data=0x%04x (%d/%d/%d/%d)\n"),
              unit, phy_addr, phy_reg, phy_data,
              port, blk, wc_instance, rv));

    return rv;
}

#if defined(BROADCOM_DEBUG)

/*
 * Function:
 *      soc_dump
 * Purpose:
 *      Dump useful information from the soc structure.
 * Parameters:
 *      unit - unit number to dump
 *      pfx - character string to prefix output line.
 * Returns:
 *      SOC_E_XXX
 */

int
soc_dump(int unit, const char *pfx)
{
    soc_control_t       *soc;
    soc_persist_t       *sop;
    soc_stat_t          *stat;
    int                 i;
    uint16              dev_id;
    uint8               rev_id;

    if (!SOC_UNIT_VALID(unit)) {
        return(SOC_E_UNIT);
    }

    soc = SOC_CONTROL(unit);
    sop = SOC_PERSIST(unit);

    stat = SOC_STAT(unit);

    LOG_CLI((BSL_META_U(unit,
                        "%sUnit %d Driver Control Structure:\n"), pfx, unit));

    soc_cm_get_id(unit, &dev_id, &rev_id);

    LOG_CLI((BSL_META_U(unit,
                        "%sChip=%s Rev=0x%02x Driver=%s\n"),
             pfx,
             soc_dev_name(unit),
             rev_id,
             SOC_CHIP_NAME(soc->chip_driver->type)));
    LOG_CLI((BSL_META_U(unit,
                        "%sFlags=0x%x:"),
             pfx, soc->soc_flags));
    if (soc->soc_flags & SOC_F_ATTACHED) {
        LOG_CLI((BSL_META_U(unit,
                            " attached")));
    }
    if (soc->soc_flags & SOC_F_INITED) {
        LOG_CLI((BSL_META_U(unit,
                            " initialized")));
    }
    if (soc->soc_flags & SOC_F_LSE) {
        LOG_CLI((BSL_META_U(unit,
                            " link-scan")));
    }
    if (soc->soc_flags & SOC_F_SL_MODE) {
        LOG_CLI((BSL_META_U(unit,
                            " sl-mode")));
    }
    if (soc->soc_flags & SOC_F_POLLED) {
        LOG_CLI((BSL_META_U(unit,
                            " polled")));
    }
    if (soc->soc_flags & SOC_F_URPF_ENABLED) {
        LOG_CLI((BSL_META_U(unit,
                            " urpf")));
    }
    if (soc->soc_flags & SOC_F_MEM_CLEAR_USE_DMA) {
        LOG_CLI((BSL_META_U(unit,
                            " mem-clear-use-dma")));
    }
    if (soc->soc_flags & SOC_F_IPMCREPLSHR) {
        LOG_CLI((BSL_META_U(unit,
                            " ipmc-repl-shared")));
    }
    if (soc->remote_cpu) {
        LOG_CLI((BSL_META_U(unit,
                            " rcpu")));
    }
    LOG_CLI((BSL_META_U(unit,
                        "; board type 0x%x"), soc->board_type));
    LOG_CLI((BSL_META_U(unit,
                        "\n")));
    LOG_CLI((BSL_META_U(unit,
                        "%s"), pfx));
    soc_cm_dump(unit);

    LOG_CLI((BSL_META_U(unit,
                        "%sDisabled: reg_flags=0x%x mem_flags=0x%x\n"),
             pfx,
             soc->disabled_reg_flags, soc->disabled_mem_flags));
    LOG_CLI((BSL_META_U(unit,
                        "%sSchanOps=%d MMUdbg=%d LinkPause=%d\n"),
             pfx,
             stat->schan_op,
             sop->debugMode, soc->soc_link_pause));
    LOG_CLI((BSL_META_U(unit,
                        "%sCounter: int=%dus per=%dus dmaBuf=%p\n"),
             pfx,
             soc->counter_interval,
             SAL_USECS_SUB(soc->counter_coll_cur, soc->counter_coll_prev),
             (void *)soc->counter_buf32));
    LOG_CLI((BSL_META_U(unit,
                        "%sTimeout: Schan=%d(%dus) MIIM=%d(%dus)\n"),
             pfx,
             stat->err_sc_tmo, soc->schanTimeout,
             stat->err_mii_tmo, soc->miimTimeout));
    LOG_CLI((BSL_META_U(unit,
                        "%sIntr: Total=%d Sc=%d ScErr=%d MMU/ARLErr=%d\n"
                        "%s      LinkStat=%d PCIfatal=%d PCIparity=%d\n"
                        "%s      ARLdrop=%d ARLmbuf=%d ARLxfer=%d ARLcnt0=%d\n"
                        "%s      TableDMA=%d TSLAM-DMA=%d"
                        " CCM-DMA=%d SW=%d"
                        "\n%s      MemCmd[BSE]=%d MemCmd[CSE]=%d MemCmd[HSE]=%d\n"
                        "%s      ChipFunc[0]=%d ChipFunc[1]=%d ChipFunc[2]=%d\n"
                        "%s      ChipFunc[3]=%d ChipFunc[4]=%d\n"
                        "%s      FifoDma[0]=%d FifoDma[1]=%d FifoDma[2]=%d FifoDma[3]=%d\n"
                        "%s      I2C=%d MII=%d StatsDMA=%d Desc=%d Chain=%d\n"),
             pfx, stat->intr, stat->intr_sc, stat->intr_sce, stat->intr_mmu,
             pfx, stat->intr_ls, 
             stat->intr_pci_fe, stat->intr_pci_pe,
             pfx, stat->intr_arl_d, stat->intr_arl_m,
             stat->intr_arl_x, stat->intr_arl_0,
             pfx, stat->intr_tdma, stat->intr_tslam,
             stat->intr_ccmdma, stat->intr_sw,
             pfx, stat->intr_mem_cmd[0],
             stat->intr_mem_cmd[1], stat->intr_mem_cmd[2],
             pfx, stat->intr_chip_func[0], stat->intr_chip_func[1],
             stat->intr_chip_func[2],
             pfx, stat->intr_chip_func[3], stat->intr_chip_func[4],
             pfx, stat->intr_fifo_dma[0], stat->intr_fifo_dma[1],
             stat->intr_fifo_dma[2], stat->intr_fifo_dma[3],
             pfx, stat->intr_i2c, stat->intr_mii, stat->intr_stats,
             stat->intr_desc, stat->intr_chain));
    LOG_CLI((BSL_META_U(unit,
                        "%sError: SDRAM=%d CFAP=%d Fcell=%d MmuSR=%d\n"),
             pfx,
             stat->err_sdram, stat->err_cfap,
             stat->err_fcell, stat->err_sr));
    if (soc_property_get(unit, spn_PARITY_ENABLE, TRUE)) {
        LOG_CLI((BSL_META_U(unit,
                            "%sSER events(mem=%d reg=%d nak=%d stat=%d ecc=%d "
                            "direct=%d fifo=%d tcam=%d)\n"),
                 pfx,
                 stat->ser_err_mem, stat->ser_err_reg,
                 stat->ser_err_nak, stat->ser_err_stat,
                 stat->ser_err_ecc, stat->ser_err_int,
                 stat->ser_err_fifo, stat->ser_err_tcam));
        if (SOC_SER_CORRECTION_SUPPORT(unit)) {
            LOG_CLI((BSL_META_U(unit,
                                "%sSER corrections(fix=%d clear=%d "
                                "restore=%d special=%d err:%d)\n"),
                     pfx, stat->ser_err_corr, stat->ser_err_clear, 
                     stat->ser_err_restor, stat->ser_err_spe,
                     stat->ser_err_sw));
        }
    }
    LOG_CLI((BSL_META_U(unit,
                        "%sPKT DMA: dcb=t%d tpkt=%u tbyt=%u rpkt=%u rbyt=%u\n"),
             pfx,
             SOC_DCB_TYPE(unit),
             stat->dma_tpkt, stat->dma_tbyt,
             stat->dma_rpkt, stat->dma_rbyt));
    LOG_CLI((BSL_META_U(unit,
                        "%sDV: List: max-q=%d cur-tq=%d cur-rq=%d dv-size=%d\n"),
             pfx,
             soc->soc_dv_cnt, soc->soc_dv_tx_free_cnt,
             soc->soc_dv_rx_free_cnt, soc->soc_dv_size));
    LOG_CLI((BSL_META_U(unit,
                        "%sDV: Statistics: allocs=%d frees=%d alloc-q=%d\n"),
             pfx, stat->dv_alloc, stat->dv_free, stat->dv_alloc_q));
    if (soc_feature(unit, soc_feature_mem_cache) &&
        soc_property_get(unit, spn_MEM_CACHE_ENABLE, TRUE)) {
#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_HELIX4_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) || \
    defined(BCM_GREYHOUND_SUPPORT)
        LOG_CLI((BSL_META_U(unit,
                            "%sMem cache (count=%d size=%d vmap size=%d "
                            "errmap size=%d)\n"),
                 pfx,
                 stat->mem_cache_count, stat->mem_cache_size,
                 stat->mem_cache_vmap_size, stat->tcam_corrupt_map_size));
#else
        LOG_CLI((BSL_META_U(unit,
                            "%sMem cache (count=%d size=%d vmap size=%d)\n"), pfx,
                 stat->mem_cache_count, stat->mem_cache_size,
                 stat->mem_cache_vmap_size));
#endif
    }
#if defined(BCM_XGS_SUPPORT)
    if (soc_feature(unit, soc_feature_regs_as_mem)) {
        int count, size;
        soc_ser_reg_cache_info(unit, &count, &size);
        LOG_CLI((BSL_META_U(unit,
                            "%sReg cache (count=%d size=%d)\n"),
                 pfx, count, size));
    }
#endif /* BCM_XGS_SUPPORT */
    for (i = 0; i < soc->soc_max_channels; i++) {
        sdc_t *sdc = &soc->soc_channels[i];
        char *type;
        switch(sdc->sc_type) {
        case DV_NONE:           type = "--";            break;
        case DV_TX:             type = "TX";            break;
        case DV_RX:             type = "RX";            break;
        default:                type = "*INVALID*";     break;
        }
        LOG_CLI((BSL_META_U(unit,
                            "%sdma-ch-%d %s %s Queue=%d (%p)%s%s%s%s\n"),
                 pfx, i, type,
                 sdc->sc_dv_active ? "Active" : "Idle  ",
                 sdc->sc_q_cnt, (void *)sdc->sc_q,
                 (sdc->sc_flags & SOC_DMA_F_DEFAULT) ? " default" : "",
                 (sdc->sc_flags & SOC_DMA_F_POLL) ? " polled" : " intr",
                 (sdc->sc_flags & SOC_DMA_F_MBM) ? " mbm" : " no-mbm",
                 (sdc->sc_flags & SOC_DMA_F_TX_DROP) ? " tx-drop" : ""));
    }

    return(0);
}

/*
 * Function:
 *      soc_chip_dump
 * Purpose:
 *      Display driver and chip information
 * Notes:
 *      Pass unit -1 to avoid referencing unit number.
 */

void
soc_chip_dump(int unit, soc_driver_t *d)
{
    soc_info_t          *si;
    int                 i, count = 0;
    soc_port_t          port;
    soc_port_t          port1;
    char                pfmt[SOC_PBMP_FMT_LEN];
    uint16              dev_id;
    uint8               rev_id;
    int                 blk, bindex;
    char                instance_string[3], block_name[14];
#if defined(BCM_KATANA2_SUPPORT)
    soc_field_t wc_xfi_mode_sel_fld[]={WC0_8_XFI_MODE_SELf,WC1_8_XFI_MODE_SELf};
    uint32      wc_xfi_mode_sel_val[2]={0};
    uint32      top_misc_control_1_val = 0;
#endif


    if (d == NULL) {
        LOG_CLI((BSL_META_U(unit,
                            "unit %d: no driver attached\n"), unit));
        return;
    }

    LOG_CLI((BSL_META_U(unit,
                        "driver %s (%s)\n"), SOC_CHIP_NAME(d->type), d->chip_string));
    LOG_CLI((BSL_META_U(unit,
                        "\tregsfile\t\t%s\n"), d->origin));
    LOG_CLI((BSL_META_U(unit,
                        "\tpci identifier\t\tvendor 0x%04x device 0x%04x rev 0x%02x\n"),
             d->pci_vendor, d->pci_device, d->pci_revision));
    LOG_CLI((BSL_META_U(unit,
                        "\tclasses of service\t%d\n"), d->num_cos));
    LOG_CLI((BSL_META_U(unit,
                        "\tmaximums\t\tblock %d ports %d mem_bytes %d\n"),
             SOC_MAX_NUM_BLKS, SOC_MAX_NUM_PORTS, SOC_MAX_MEM_BYTES));

    if (unit < 0) {
        return;
    }
    si = &SOC_INFO(unit);
    for (blk = 0; d->block_info[blk].type >= 0; blk++) {
        sal_snprintf(instance_string, sizeof(instance_string), "%d",
                     d->block_info[blk].number);
        if (d->block_info[blk].type == SOC_BLK_PORT_GROUP4 ||
            d->block_info[blk].type == SOC_BLK_PORT_GROUP5) {
            sal_strcpy(instance_string,
                       d->block_info[blk].number ? "_y" : "_x");
        }
        sal_snprintf(block_name, sizeof(block_name), "%s%s",
                     soc_block_name_lookup_ext(d->block_info[blk].type, unit),
                     instance_string);
        LOG_CLI((BSL_META_U(unit,
                            "\tblk %d\t\t%-14s schan %d cmic %d\n"),
                 blk,
                 block_name,
                 d->block_info[blk].schan,
                 d->block_info[blk].cmic));
    }
    bindex = -1;
    for (port = 0; ; port++) {
        for (i = 0; i < d->port_num_blktype; i++) {
            blk = d->port_info[port * d->port_num_blktype + i].blk;
            bindex = d->port_info[port * d->port_num_blktype + i].bindex;
            if(blk < 0) {
                break;
            }
            if (i == 0) {
                LOG_CLI((BSL_META_U(unit,
                                    "\tport %-3d\t"),
                         soc_feature(unit, soc_feature_logical_port_num) ?
                         si->port_p2l_mapping[port] : port));
            } else {
                LOG_CLI((BSL_META_U(unit,
                                    "\t        \t")));
            }
            LOG_CLI((BSL_META_U(unit,
                                "%s\tblk %d %s%d.%d\n"),
                     soc_block_port_name_lookup_ext(d->block_info[blk].type,
                     unit),
                     blk,
                     soc_block_name_lookup_ext(d->block_info[blk].type, unit),
                     d->block_info[blk].number,
                     bindex));
        }
        if (bindex < 0) {            /* end of list */
            break;
        }
    }

    soc_cm_get_id(unit, &dev_id, &rev_id);
    LOG_CLI((BSL_META_U(unit,
                        "unit %d:\n"), unit));
    LOG_CLI((BSL_META_U(unit,
                        "\tpci\t\t\tdevice %04x rev %02x\n"), dev_id, rev_id));
#ifdef BCM_CMICM_SUPPORT
    LOG_CLI((BSL_META_U(unit,
                        "\tcmc used by pci\t\tCMC%d\n"), SOC_PCI_CMC(unit)));
#endif
    LOG_CLI((BSL_META_U(unit,
                        "\tdriver\t\t\ttype %d (%s) group %d (%s)\n"),
             si->driver_type, SOC_CHIP_NAME(si->driver_type),
             si->driver_group, soc_chip_group_names[si->driver_group]));
    LOG_CLI((BSL_META_U(unit,
                        "\tchip\t\t\t%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s"
                        "%s%s%s%s%s%s%s%s%s%s%s%s%s\n"),
             SOC_IS_HERCULES15(unit) ? "hercules15 " : "",
             SOC_IS_FIREBOLT(unit) ? "firebolt " : "",
             SOC_IS_FIREBOLT2(unit) ? "firebolt2 " : "",
             SOC_IS_TRIUMPH3(unit) ? "triumph3 " : "",
             SOC_IS_HELIX4(unit) ? "helix4 " : "",
             SOC_IS_KATANA(unit) ? "katana " : "",
             SOC_IS_KATANA2(unit) ? "katana2 " : "",
             SOC_IS_HELIX1(unit) ? "helix " : "",
             SOC_IS_FELIX1(unit) ? "felix " : "",
             SOC_IS_HELIX15(unit) ? "helix15 " : "",
             SOC_IS_FELIX15(unit) ? "felix15 " : "",
             SOC_IS_RAPTOR(unit) ? "raptor " : "",
             SOC_IS_TRIUMPH(unit) ? "triumph " : "",
             SOC_IS_RAVEN(unit) ? "raven " : "",
             SOC_IS_HAWKEYE(unit) ? "hawkeye " : "",
             SOC_IS_SCORPION(unit) ? "scorpion " : "",
             SOC_IS_VALKYRIE(unit) ? "valkyrie " : "",
             SOC_IS_CONQUEROR(unit) ? "conqueror " : "",
             SOC_IS_TRIUMPH2(unit) ? "triumph2 " : "",
             SOC_IS_ENDURO(unit) ? "enduro " : "",
             SOC_IS_TRIDENT(unit) ? "trident " : "",
             SOC_IS_TITAN(unit) ? "titan " : "",
             SOC_IS_TRIDENT2(unit) ? "trident2 " : "",
             SOC_IS_TITAN2(unit) ? "titan2 " : "",
             SOC_IS_XGS(unit) ? "xgs " : "",
             SOC_IS_XGS_FABRIC(unit) ? "xgs_fabric " : "",
             SOC_IS_XGS3_SWITCH(unit) ? "xgs3_switch " : "",
             SOC_IS_XGS_SWITCH(unit) ? "xgs_switch " : ""));
    LOG_CLI((BSL_META_U(unit,
                        "\tmax modid\t\t%d\n"), si->modid_max));
    LOG_CLI((BSL_META_U(unit,
                        "\tnum ports\t\t%d\n"), si->port_num));
    LOG_CLI((BSL_META_U(unit,
                        "\tnum modids\t\t%d\n"), si->modid_count));
    LOG_CLI((BSL_META_U(unit,
                        "\tnum blocks\t\t%d\n"), si->block_num));
    LOG_CLI((BSL_META_U(unit,
                        "\tFE ports\t%d\t%s (%d:%d)\n"),
             si->fe.num, SOC_PBMP_FMT(si->fe.bitmap, pfmt),
             si->fe.min, si->fe.max));
    LOG_CLI((BSL_META_U(unit,
                        "\tGE ports\t%d\t%s (%d:%d)\n"),
             si->ge.num, SOC_PBMP_FMT(si->ge.bitmap, pfmt),
             si->ge.min, si->ge.max));
    LOG_CLI((BSL_META_U(unit,
                        "\tXE ports\t%d\t%s (%d:%d)\n"),
             si->xe.num, SOC_PBMP_FMT(si->xe.bitmap, pfmt),
             si->xe.min, si->xe.max));
    LOG_CLI((BSL_META_U(unit,
                        "\tCE ports\t%d\t%s (%d:%d)\n"),
             si->ce.num, SOC_PBMP_FMT(si->ce.bitmap, pfmt),
             si->ce.min, si->ce.max));
    LOG_CLI((BSL_META_U(unit,
                        "\tHG ports\t%d\t%s (%d:%d)\n"),
             si->hg.num, SOC_PBMP_FMT(si->hg.bitmap, pfmt),
             si->hg.min, si->hg.max));
    LOG_CLI((BSL_META_U(unit,
                        "\tST ports\t%d\t%s (%d:%d)\n"),
             si->st.num, SOC_PBMP_FMT(si->st.bitmap, pfmt),
             si->st.min, si->st.max));
    LOG_CLI((BSL_META_U(unit,
                        "\tETHER ports\t%d\t%s (%d:%d)\n"),
             si->ether.num, SOC_PBMP_FMT(si->ether.bitmap, pfmt),
             si->ether.min, si->ether.max));
    LOG_CLI((BSL_META_U(unit,
                        "\tPORT ports\t%d\t%s (%d:%d)\n"),
             si->port.num, SOC_PBMP_FMT(si->port.bitmap, pfmt),
             si->port.min, si->port.max));
    LOG_CLI((BSL_META_U(unit,
                        "\tALL ports\t%d\t%s (%d:%d)\n"),
             si->all.num, SOC_PBMP_FMT(si->all.bitmap, pfmt),
             si->all.min, si->all.max));
    LOG_CLI((BSL_META_U(unit,
                        "\tIPIC port\t%d\tblock %d\n"),
             si->ipic_port, si->ipic_block));
    LOG_CLI((BSL_META_U(unit,
                        "\tCMIC port\t%d\t%s block %d\n"),
             si->cmic_port,
             SOC_PBMP_FMT(si->cmic_bitmap, pfmt), si->cmic_block));
    LOG_CLI((BSL_META_U(unit,
                        "\tother blocks\t\tARL %d MMU %d MCU %d\n"),
             si->arl_block, si->mmu_block, si->mcu_block));
    LOG_CLI((BSL_META_U(unit,
                        "\t            \t\tIPIPE %d IPIPE_HI %d EPIPE %d EPIPE_HI %d "
                        "BSAFE %d ESM %d OTPC %d\n"),
             si->ipipe_block, si->ipipe_hi_block,
             si->epipe_block, si->epipe_hi_block,
             si->bsafe_block, si->esm_block, si->otpc_block));

    for (i = 0; i < COUNTOF(si->has_block); i++) {
        if (si->has_block[i]) {
            count++;
        }
    }
    LOG_CLI((BSL_META_U(unit,
                        "\thas blocks\t%d\t"), count));
    for (i = 0; i < COUNTOF(si->has_block); i++) {
        if (si->has_block[i]) {
            LOG_CLI((BSL_META_U(unit,
                                "%s "),
                     soc_block_name_lookup_ext(si->has_block[i], unit)));
            if ((i) && !(i%6)) {
                LOG_CLI((BSL_META_U(unit,
                                    "\n\t\t\t\t")));
            }
        }
    }
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            READ_TOP_MISC_CONTROL_1r(unit,&top_misc_control_1_val);
            wc_xfi_mode_sel_val[0] = soc_reg_field_get(
                                       unit, TOP_MISC_CONTROL_1r,
                                       top_misc_control_1_val,
                                       wc_xfi_mode_sel_fld[0]);
            if (wc_xfi_mode_sel_val[0]) {
                LOG_CLI((BSL_META_U(unit,
                                    "\n=========================================")));
                LOG_CLI((BSL_META_U(unit,
                                    "\nATTN: KT2 Device's block MXQ8 in XFI MODE")));
                LOG_CLI((BSL_META_U(unit,
                                    "\nPort 25 mapped to  32 and 36 mapped to 34")));
                LOG_CLI((BSL_META_U(unit,
                                    "\n=========================================\n")));
            }
            wc_xfi_mode_sel_val[1] = soc_reg_field_get(
                                       unit, TOP_MISC_CONTROL_1r,
                                       top_misc_control_1_val,
                                       wc_xfi_mode_sel_fld[1]);
            if (wc_xfi_mode_sel_val[1]) {
                LOG_CLI((BSL_META_U(unit,
                                    "\n=========================================")));
                LOG_CLI((BSL_META_U(unit,
                                    "\nATTN: KT2 Device's block MXQ9 in XFI MODE")));
                LOG_CLI((BSL_META_U(unit,
                                    "\nPort 26 mapped to  29 and 39 mapped to 31")));
                LOG_CLI((BSL_META_U(unit,
                                    "\n=========================================\n")));
            }
        }
#endif
    LOG_CLI((BSL_META_U(unit,
                        "\n")));
    LOG_CLI((BSL_META_U(unit,
                        "\tport names\t\t")));
    for (port = 0; port < si->port_num; port++) {
        if (port > 0 && (port % 5) == 0) {
            LOG_CLI((BSL_META_U(unit,
                                "\n\t\t\t\t")));
        }
        port1 = port; 
#if defined(BCM_KATANA2_SUPPORT)
            if (SOC_IS_KATANA2(unit)) {
                switch(port) {
                case 25:
                case 36:
                    if (wc_xfi_mode_sel_val[0]) {
                        port1 =  (port == 25) ? 32 : 34;
                    }
                    break;
                case 26:
                case 39:
                    if (wc_xfi_mode_sel_val[1]) {
                        port1 =  (port == 26) ? 29 : 31;
                    }
                    break;
                default:
                    break;
                }
            }
#endif
        LOG_CLI((BSL_META_U(unit,
                            "%d=%s\t"),
                 port1, si->port_name[port]));
    }
    LOG_CLI((BSL_META_U(unit,
                        "\n")));
    i = 0;
    for (blk = 0; SOC_BLOCK_INFO(unit, blk).type >= 0; blk++) {
        if (SOC_PBMP_IS_NULL(si->block_bitmap[blk])) {
            continue;
        }
        if (++i == 1) {
            LOG_CLI((BSL_META_U(unit,
                                "\tblock bitmap\t")));
        } else {
            LOG_CLI((BSL_META_U(unit,
                                "\n\t\t\t")));
        }
        LOG_CLI((BSL_META_U(unit,
                            "%-2d  %-14s %s (%d ports)"),
                 blk,
                 si->block_name[blk],
                 SOC_PBMP_FMT(si->block_bitmap[blk], pfmt),
                 si->block_valid[blk]));
    }
    if (i > 0) {
        LOG_CLI((BSL_META_U(unit,
                            "\n")));
    }

    {
        soc_feature_t f;

        LOG_CLI((BSL_META_U(unit,
                            "\tfeatures\t")));
        i = 0;
        for (f = 0; f < soc_feature_count; f++) {
            if (soc_feature(unit, f)) {
                if (++i > 3) {
                    LOG_CLI((BSL_META_U(unit,
                                        "\n\t\t\t")));
                    i = 1;
                }
                LOG_CLI((BSL_META_U(unit,
                                    "%s "), soc_feature_name[f]));
            }
        }
        LOG_CLI((BSL_META_U(unit,
                            "\n")));
    }
}

#endif /* BROADCOM_DEBUG */
