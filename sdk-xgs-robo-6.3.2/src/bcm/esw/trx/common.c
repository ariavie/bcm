/*
 * $Id: common.c 1.158.2.1 Broadcom SDK $
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
 * File:    common.c
 * Purpose: Manages resources shared across multiple modules
 */
#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/mcm/allenum.h>
#include <soc/profile_mem.h>

#include <sal/compiler.h>

#include <bcm/error.h>
#include <bcm/types.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/field.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm_int/esw/mpls.h>
#include <bcm_int/esw/mim.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw_dispatch.h>
#ifdef BCM_KATANA2_SUPPORT
#include <bcm_int/esw/katana2.h>
#endif

typedef struct _bcm_common_bookkeeping_s {
    int initialized;            /* Flag to check initialized status */
    int mac_da_profile_created; /* Flag to check MAC_DA profile creation */
    int lport_profile_created;  /* Flag to check LPORT profile creation */
    int ing_pri_cng_map_created; /* Flag to check ING_PRI_CNG_MAP creation */
    int ing_vft_pri_map_created; /* Flag to check ING_VFT_PRI_MAP creation */
    int egr_vft_pri_map_created; /* Flag to check EGR_VFT_PRI_MAP creation */
    int egr_vsan_intpri_map_created; /* Flag to check EGR_VSAN_INTPRI_MAP created */
    int egr_vft_fields_created; /* Flag to check EGR_VFT_FIELDS_PROFILE created */
    int egr_mpls_combo_map_created; /* Flag to check combo map creation */
    int dscp_table_created; /* Flag to check DSCP_TABLE creation */
    int egr_dscp_table_created; /* Flag to check EGR_DSCP_TABLE creation */
    int ing_outer_dot1p_created; /* Flag to check ING_OUTER_DOT1P_MAPPING_TABLE 
                                    creation */
#ifdef BCM_TRIUMPH3_SUPPORT
    int ing_l3_nh_attribute_created;  /* Flag to check ING_L3_NEXT_HOP_ATTRIBUTE_1
                                         creation */
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    int l3_iif_profile_created;  /* Flag to check L3_IIF_PROFILE creation */
    int l3_ip4_options_profile_created;  /* Flag to check L3_IP4_OPTIONS_PROFILE creation */
    int fc_map_profile_created;          /* Flag to check FC_MAP_PROFILE */
#endif /* BCM_TRIDENT2_SUPPORT */

    int prot_pkt_ctrl_created;
#ifdef BCM_KATANA_SUPPORT
    int ing_queue_offset_map_created; /* Flag to check ING_QUEUE_OFFSET_MAP_
                                         TABLE creation */
#endif
#ifdef BCM_KATANA2_SUPPORT
    int vlan_protocol_data_profile_created; /* Flag to check if VLAN_PROTOCOL_DATA profile creation */
#endif
    soc_profile_mem_t *mac_da_profile; /* cache of EGR_MAC_DA_PROFILE */
    soc_profile_mem_t *lport_profile;  /* cache of LPORT_TABLE (profile) */
    soc_profile_mem_t *ing_pri_cng_map;  /* cache of ING_PRI_CNG_MAP (profile) */
    soc_profile_mem_t *egr_mpls_combo_map;  /* cache of combined tables */
    soc_profile_mem_t *dscp_table;  /* cache of DSCP_TABLE (profile) */
    soc_profile_mem_t *egr_dscp_table;  /* cache of EGR_DSCP_TABLE (profile) */
    soc_profile_mem_t *ing_outer_dot1p;  /* cache of ING_OUTER_DOT1P_MAPPING_TABLE 
                                            (profile) */
#ifdef BCM_TRIUMPH3_SUPPORT
    soc_profile_mem_t *ing_l3_nh_attribute;  /* cache of ING_L3_NEXT_HOP_ATTRIBUTE_1
                                                (profile) */ 
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    soc_profile_mem_t *l3_iif_profile;  /* Cache of L3 IIF profile */
    soc_profile_mem_t *l3_ip4_options_profile;  /* Cache of L3 IP4 options profile */
    soc_profile_mem_t *fc_map_profile;
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    soc_profile_mem_t *offset_map_table; /* cache of ING_QUEUE_OFFSET_MAP_TABLE
                                            (profile) */
#endif /* BCM_KATANA_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
    /* cache of VLAN_PROTOCOL_DATA profile */
    soc_profile_mem_t *vlan_protocol_data_profile;
#endif

    soc_profile_mem_t *ing_vft_pri_map;
    soc_profile_mem_t *egr_vft_pri_map;
    soc_profile_mem_t *egr_vsan_intpri_map;
    soc_profile_mem_t *egr_vft_fields;
    soc_profile_reg_t *prot_pkt_ctrl;
    uint16      *mpls_station_hash; /* Hash values for MPLS STATION entries */
    my_station_tcam_entry_t *my_station_shadow;
    my_station_tcam_entry_t my_station_l3_mask;
    my_station_tcam_entry_t my_station_tunnel_mask;
} _bcm_common_bookkeeping_t;

STATIC _bcm_common_bookkeeping_t  _bcm_common_bk_info[BCM_MAX_NUM_UNITS] = {{ 0 }};
STATIC sal_mutex_t _common_mutex[BCM_MAX_NUM_UNITS] = {NULL};

#define COMMON_INFO(_unit_)   (&_bcm_common_bk_info[_unit_])
#define MAC_DA_PROFILE(_unit_) (_bcm_common_bk_info[_unit_].mac_da_profile)
#define LPORT_PROFILE(_unit_) (_bcm_common_bk_info[_unit_].lport_profile)
#define ING_PRI_CNG_MAP(_unit_) (_bcm_common_bk_info[_unit_].ing_pri_cng_map)
#define EGR_MPLS_COMBO_MAP(_unit_) (_bcm_common_bk_info[_unit_].egr_mpls_combo_map)
#define DSCP_TABLE(_unit_) (_bcm_common_bk_info[_unit_].dscp_table)
#define EGR_DSCP_TABLE(_unit_) (_bcm_common_bk_info[_unit_].egr_dscp_table)
#define ING_OUTER_DOT1P(_unit_) (_bcm_common_bk_info[_unit_].ing_outer_dot1p)
#define PROT_PKT_CTRL(_unit_) (_bcm_common_bk_info[_unit_].prot_pkt_ctrl)
#define ING_VFT_PRI_MAP(_unit_) (_bcm_common_bk_info[_unit_].ing_vft_pri_map)
#define EGR_VFT_PRI_MAP(_unit_) (_bcm_common_bk_info[_unit_].egr_vft_pri_map)
#define EGR_VSAN_INTPRI_MAP(_unit_) (_bcm_common_bk_info[_unit_].egr_vsan_intpri_map)
#define EGR_VFT_FIELDS(_unit_) (_bcm_common_bk_info[_unit_].egr_vft_fields)
#define MPLS_STATION_HASH(_unit_, _index_) \
        (_bcm_common_bk_info[_unit_].mpls_station_hash[_index_])

#ifdef BCM_TRIUMPH3_SUPPORT
#define ING_L3_NH_ATTRIB_PROFILE(_unit_) (_bcm_common_bk_info[_unit_].ing_l3_nh_attribute)
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
#define L3_IIF_PROFILE(_unit_) (_bcm_common_bk_info[_unit_].l3_iif_profile)
#define L3_IP4_OPTIONS_PROFILE(_unit_) (_bcm_common_bk_info[_unit_].l3_ip4_options_profile)
#define FC_MAP_PROFILE(_unit_) (_bcm_common_bk_info[_unit_].fc_map_profile)
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
#define OFFSET_MAP_TABLE(_unit_) (_bcm_common_bk_info[_unit_].offset_map_table)
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_KATANA2_SUPPORT
#define VLAN_PROTOCOL_DATA_TABLE(_unit_) \
        (_bcm_common_bk_info[_unit_].vlan_protocol_data_profile)
#endif /* BCM_KATANA2_SUPPORT */

/* 
 * Common resource lock
 */
#define COMMON_LOCK(unit) \
        sal_mutex_take(_common_mutex[unit], sal_mutex_FOREVER); 

#define COMMON_UNLOCK(unit) \
        sal_mutex_give(_common_mutex[unit]); 

/* Forward declaration */
STATIC int
_bcm_trx_mpls_station_hash_calc(int unit, bcm_mac_t mac,
                                bcm_vlan_t vlan, uint16 *hash);
#ifdef BCM_TRIUMPH2_SUPPORT
STATIC int
_bcm_esw_flex_stat_hw_set(int unit, _bcm_flex_stat_type_t type,
                          int fs_idx, _bcm_flex_stat_t stat, uint64 val);
STATIC int
_bcm_esw_flex_stat_sw_clear(int unit, _bcm_flex_stat_type_t type,
                            int fs_idx);
#endif


/*
 * Function:
 *      _bcm_common_free_resource
 * Purpose:
 *      Free all allocated tables and memory
 *      Shared by MPLS and MIM
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      Nothing
 */
STATIC void
_bcm_common_free_resource(int unit, _bcm_common_bookkeeping_t *info)
{
    if (!info) {
        return;
    }
    if (_common_mutex[unit]) {
        sal_mutex_destroy(_common_mutex[unit]);
        _common_mutex[unit] = NULL;
    } 
    if (info->mpls_station_hash) {
        sal_free(info->mpls_station_hash);
        info->mpls_station_hash = NULL;
    }
    if (info->my_station_shadow) {
        sal_free(info->my_station_shadow);
        info->my_station_shadow = NULL;
    }
    if (info->mac_da_profile) {
        if (info->mac_da_profile_created) {
            (void) soc_profile_mem_destroy(unit, info->mac_da_profile);
        }
        sal_free(info->mac_da_profile);
        info->mac_da_profile = NULL;
    }
    if (info->lport_profile) {
        if (info->lport_profile_created) {
            (void) soc_profile_mem_destroy(unit, info->lport_profile);
        }
        sal_free(info->lport_profile);
        info->lport_profile = NULL;
    }
    if (info->ing_pri_cng_map) {
        if (info->ing_pri_cng_map_created) {
            (void) soc_profile_mem_destroy(unit, info->ing_pri_cng_map);
        }
        sal_free(info->ing_pri_cng_map);
        info->ing_pri_cng_map = NULL;
    }
    if (info->ing_vft_pri_map) {
        if (info->ing_vft_pri_map_created) {
            (void) soc_profile_mem_destroy(unit, info->ing_vft_pri_map);
        }
        sal_free(info->ing_vft_pri_map);
        info->ing_vft_pri_map = NULL;
    }
    if (info->egr_vft_pri_map) {
        if (info->egr_vft_pri_map_created) {
            (void) soc_profile_mem_destroy(unit, info->egr_vft_pri_map);
        }
        sal_free(info->egr_vft_pri_map);
        info->egr_vft_pri_map = NULL;
    }
    if (info->egr_vsan_intpri_map) {
        if (info->egr_vsan_intpri_map_created) {
            (void) soc_profile_mem_destroy(unit, info->egr_vsan_intpri_map);
        }
        sal_free(info->egr_vsan_intpri_map);
        info->egr_vsan_intpri_map = NULL;
    }
    if (info->egr_vft_fields) {
        if (info->egr_vft_fields_created) {
            (void) soc_profile_mem_destroy(unit, info->egr_vft_fields);
        }
        sal_free(info->egr_vft_fields);
        info->egr_vft_fields = NULL;
    }
    if (info->egr_mpls_combo_map) {
        if (info->egr_mpls_combo_map_created) {
            (void) soc_profile_mem_destroy(unit, info->egr_mpls_combo_map);
        }
        sal_free(info->egr_mpls_combo_map);
        info->egr_mpls_combo_map = NULL;
    }
    if (info->dscp_table) {
        if (info->dscp_table_created) {
            (void) soc_profile_mem_destroy(unit, info->dscp_table);
        }
        sal_free(info->dscp_table);
        info->dscp_table = NULL;
    }
    if (info->egr_dscp_table) {
        if (info->egr_dscp_table_created) {
            (void) soc_profile_mem_destroy(unit, info->egr_dscp_table);
        }
        sal_free(info->egr_dscp_table);
        info->egr_dscp_table = NULL;
    }
    if (info->ing_outer_dot1p) {
        if (info->ing_outer_dot1p_created) {
            (void) soc_profile_mem_destroy(unit, info->ing_outer_dot1p);
        }
        sal_free(info->ing_outer_dot1p);
        info->ing_outer_dot1p = NULL;
    }
    if (info->prot_pkt_ctrl) {
        if (info->prot_pkt_ctrl_created) {
            (void) soc_profile_reg_destroy(unit, info->prot_pkt_ctrl);
        }
        sal_free(info->prot_pkt_ctrl);
        info->prot_pkt_ctrl = NULL;
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        if (info->ing_l3_nh_attribute) {
            if (info->ing_l3_nh_attribute_created) {
                (void) soc_profile_mem_destroy(unit,
                        info->ing_l3_nh_attribute);
            }
            sal_free(info->ing_l3_nh_attribute);
            info->ing_l3_nh_attribute = NULL;
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)) {
        if (info->l3_iif_profile) {
            if (info->l3_iif_profile_created) {
                (void) soc_profile_mem_destroy(unit,
                        info->l3_iif_profile);
            }
            sal_free(info->l3_iif_profile);
            info->l3_iif_profile = NULL;
        }
        if (info->l3_ip4_options_profile) {
            if (info->l3_ip4_options_profile_created) {
                (void) soc_profile_mem_destroy(unit,
                        info->l3_ip4_options_profile);
            }
            sal_free(info->l3_ip4_options_profile);
            info->l3_ip4_options_profile = NULL;
        }
        if (info->fc_map_profile) {
            if (info->fc_map_profile_created) {
                (void) soc_profile_mem_destroy(unit,
                        info->fc_map_profile);
            }
            sal_free(info->fc_map_profile);
            info->fc_map_profile = NULL;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    if (info->offset_map_table) {
        if (info->ing_queue_offset_map_created) {
            (void) soc_profile_mem_destroy(unit, info->offset_map_table);
        }
        sal_free(info->offset_map_table);
        info->offset_map_table = NULL;
    }
#endif /* BCM_KATANA_SUPPORT */

    return;
}

#ifdef BCM_WARM_BOOT_SUPPORT
STATIC int
_bcm_common_reinit(int unit)
{
    int i, num_station;
    bcm_mac_t hw_mac;
    bcm_vlan_t hw_vlan;
    mpls_station_tcam_entry_t mpls_station_entry;
    uint16 hash;

    if (SOC_MEM_IS_VALID(unit, MPLS_STATION_TCAMm)) {
        /* Compute hash for each entry */
        num_station = soc_mem_index_count(unit, MPLS_STATION_TCAMm);
        for (i = 0; i < num_station; i++) {
            BCM_IF_ERROR_RETURN
                (READ_MPLS_STATION_TCAMm(unit, MEM_BLOCK_ANY, i, &mpls_station_entry));
            soc_mem_mac_addr_get(unit, MPLS_STATION_TCAMm,
                                 &mpls_station_entry, MAC_ADDRf, hw_mac);
            hw_vlan = soc_mem_field32_get(unit, MPLS_STATION_TCAMm,
                                          &mpls_station_entry, VLAN_IDf);
            /* Get the hash value for this key */
            BCM_IF_ERROR_RETURN
                (_bcm_trx_mpls_station_hash_calc(unit, hw_mac, hw_vlan, &hash));
            MPLS_STATION_HASH(unit, i) = hash;
        }
    } else {
#ifdef BCM_TRIDENT_SUPPORT
        if (SOC_MEM_IS_VALID(unit, MY_STATION_TCAMm)) {
            _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);

            /* Compute hash for each entry */
            num_station = soc_mem_index_count(unit, MY_STATION_TCAMm);
            for (i = 0; i < num_station; i++) {
                BCM_IF_ERROR_RETURN
                    (READ_MY_STATION_TCAMm(unit, MEM_BLOCK_ANY, i,
                                           &info->my_station_shadow[i]));
            }
        }
#endif /* BCM_TRIDENT_SUPPORT */
    }

    /* Restore the CPU TDM slot cache entry in soc_control */
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) ||
        SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit)) {
        int i, alloc_size;
        uint32 port_num;
        arb_tdm_table_entry_t *arb_tdm, *arb_tdm_table;
        iarb_tdm_table_entry_t *iarb_tdm, *iarb_tdm_table;
        alloc_size = sizeof(arb_tdm_table_entry_t) * 
                     soc_mem_index_count(unit, ARB_TDM_TABLEm);
        arb_tdm_table = soc_cm_salloc(unit, alloc_size, "ARB TDM reload");
        if (arb_tdm_table == NULL) {
            return BCM_E_MEMORY;
        }
        sal_memset(arb_tdm_table, 0, alloc_size);
        alloc_size = sizeof(iarb_tdm_table_entry_t) * 
                     soc_mem_index_count(unit, IARB_TDM_TABLEm);
        iarb_tdm_table = soc_cm_salloc(unit, alloc_size, "IARB TDM reload");
        if (iarb_tdm_table == NULL) {
            soc_cm_sfree(unit, arb_tdm_table);
            return BCM_E_MEMORY;
        }
        sal_memset(iarb_tdm_table, 0, alloc_size);
        if (soc_mem_read_range(unit, ARB_TDM_TABLEm, MEM_BLOCK_ANY,
                               soc_mem_index_min(unit, ARB_TDM_TABLEm),
                               soc_mem_index_max(unit, ARB_TDM_TABLEm),
                               arb_tdm_table) < 0) {
            soc_cm_sfree(unit, arb_tdm_table);
            soc_cm_sfree(unit, iarb_tdm_table);
            return SOC_E_INTERNAL;
        }
        if (soc_mem_read_range(unit, IARB_TDM_TABLEm, MEM_BLOCK_ANY,
                               soc_mem_index_min(unit, IARB_TDM_TABLEm),
                               soc_mem_index_max(unit, IARB_TDM_TABLEm),
                               iarb_tdm_table) < 0) {
            soc_cm_sfree(unit, arb_tdm_table);
            soc_cm_sfree(unit, iarb_tdm_table);
            return SOC_E_INTERNAL;
        }
        for (i = 0; i < soc_mem_index_count(unit, ARB_TDM_TABLEm); i++) {
            arb_tdm = soc_mem_table_idx_to_pointer(unit, ARB_TDM_TABLEm,
                                                   arb_tdm_table_entry_t *,
                                                   arb_tdm_table, i);
            iarb_tdm = soc_mem_table_idx_to_pointer(unit, IARB_TDM_TABLEm,
                                                    iarb_tdm_table_entry_t *,
                                                    iarb_tdm_table, i);             
            if (soc_ARB_TDM_TABLEm_field32_get(unit, arb_tdm, WRAP_ENf)) {
                break;
            }
            port_num = soc_ARB_TDM_TABLEm_field32_get(unit, arb_tdm, PORT_NUMf);
            if (IS_CPU_PORT(unit, port_num)) {
                sal_memcpy(&(SOC_CONTROL(unit)->iarb_tdm), iarb_tdm, 
                           sizeof(iarb_tdm_table_entry_t));
                SOC_CONTROL(unit)->iarb_tdm_idx = i;
            }
        }
        if (arb_tdm_table != NULL) {
            soc_cm_sfree(unit, arb_tdm_table);
        }
        if (iarb_tdm_table != NULL) {
            soc_cm_sfree(unit, iarb_tdm_table);
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */  
    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      _bcm_common_cleanup
 * Purpose:
 *      Internal function for initializing common resource management
 *      Shared by MPLS and MIM
 * Parameters:
 *      unit    -  (IN) Device number.
 * Returns:
 *      BCM_X_XXX
 */
int _bcm_common_cleanup(int unit)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    _bcm_common_free_resource(unit, info);
    info->mac_da_profile_created = FALSE;
    info->lport_profile_created = FALSE;
    info->ing_pri_cng_map_created = FALSE;
    info->ing_vft_pri_map_created = FALSE;
    info->egr_vft_pri_map_created = FALSE;
    info->egr_vft_fields_created = FALSE;
    info->egr_mpls_combo_map_created = FALSE;
    info->dscp_table_created = FALSE;
    info->egr_dscp_table_created = FALSE;
    info->prot_pkt_ctrl_created = FALSE;
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        info->ing_l3_nh_attribute_created = FALSE;
    }
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)) {
        info->l3_iif_profile_created = FALSE;
        info->l3_ip4_options_profile_created = FALSE;
        info->fc_map_profile_created = FALSE;
    }
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    info->ing_queue_offset_map_created = FALSE;
#endif /* BCM_KATANA_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
    info->vlan_protocol_data_profile_created = FALSE;
#endif /* BCM_KATANA2_SUPPORT */

    info->initialized = FALSE;
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_common_init
 * Purpose:
 *      Internal function for initializing common resource management
 *      Shared by MPLS and MIM
 * Parameters:
 *      unit    -  (IN) Device number.
 * Returns:
 *      BCM_X_XXX
 */
int _bcm_common_init(int unit)
{
    int num_station = 0, rv = BCM_E_NONE;
    soc_mem_t mem;
    soc_mem_t mems[3];
    int mem_words[3];
    int index_min[2];
    int index_max[2];
    int mems_cnt;
    int entry_words, alloc_size;
    bcm_pbmp_t all_pbmp;
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);

    if (info->initialized) {
        BCM_IF_ERROR_RETURN(_bcm_common_cleanup(unit));
    }

    /* Create mutex */
    if (NULL == _common_mutex[unit]) {
        _common_mutex[unit] = sal_mutex_create("common mutex");
        if (_common_mutex[unit] == NULL) {
            _bcm_common_free_resource(unit, info);
            return BCM_E_MEMORY;
        }
    }

    /* MPLS station hash */
    if (SOC_MEM_IS_VALID(unit, MPLS_STATION_TCAMm)) {
        num_station = soc_mem_index_count(unit, MPLS_STATION_TCAMm);
        if (NULL == info->mpls_station_hash) {
            info->mpls_station_hash = sal_alloc(sizeof(uint16) *
                                        num_station, "mpls station hash");
            if (info->mpls_station_hash == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            sal_memset(info->mpls_station_hash, 0, 
                sizeof(uint16) * num_station);
        }
    } else if (SOC_MEM_IS_VALID(unit, MY_STATION_TCAMm)) {
        num_station = soc_mem_index_count(unit, MY_STATION_TCAMm);
        if (SOC_MEM_IS_VALID(unit, MY_STATION_TCAMm)) {
            alloc_size = sizeof(my_station_tcam_entry_t) * num_station;
            if (NULL == info->my_station_shadow) {
                info->my_station_shadow =
                    sal_alloc(alloc_size, "my station shadow");
                if (info->my_station_shadow == NULL) {
                    _bcm_common_free_resource(unit, info);
                    return BCM_E_MEMORY;
                }
                sal_memset(info->my_station_shadow, 0, alloc_size);
            }
            /* L3 flags */
            soc_mem_field32_set(unit, MY_STATION_TCAMm,
                                &info->my_station_l3_mask,
                                IPV4_TERMINATION_ALLOWEDf, 1);
            soc_mem_field32_set(unit, MY_STATION_TCAMm,
                                &info->my_station_l3_mask,
                                IPV6_TERMINATION_ALLOWEDf, 1);
            soc_mem_field32_set(unit, MY_STATION_TCAMm,
                                &info->my_station_l3_mask,
                                ARP_RARP_TERMINATION_ALLOWEDf, 1);
            /* OAM flags */
            if (SOC_IS_KATANA(unit)) {
                if (soc_mem_field_valid(unit,MY_STATION_TCAMm,
                    OAM_TERMINATION_ALLOWEDf)) {
                    soc_mem_field32_set(unit, MY_STATION_TCAMm,
                                        &info->my_station_l3_mask,
                                        OAM_TERMINATION_ALLOWEDf, 1);
                }
            }
            /* Tunnel flags */
            soc_mem_field32_set(unit, MY_STATION_TCAMm,
                                &info->my_station_tunnel_mask,
                                MIM_TERMINATION_ALLOWEDf, 1);
            soc_mem_field32_set(unit, MY_STATION_TCAMm,
                                &info->my_station_tunnel_mask,
                                MPLS_TERMINATION_ALLOWEDf, 1);
            if (soc_mem_field_valid(unit, MY_STATION_TCAMm, 
                                    TRILL_TERMINATION_ALLOWEDf)) {
                soc_mem_field32_set(unit, MY_STATION_TCAMm,
                                &info->my_station_tunnel_mask,
                                TRILL_TERMINATION_ALLOWEDf, 1);
            }
        }
    }

    /* Create profile table cache (or re-init if it already exists) */
    mem = EGR_MAC_DA_PROFILEm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->mac_da_profile) {
            info->mac_da_profile = sal_alloc(sizeof(soc_profile_mem_t),
                    "MAC DA Profile Mem");
            if (info->mac_da_profile == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->mac_da_profile);

            entry_words = sizeof(egr_mac_da_profile_entry_t) / sizeof(uint32);
            rv = soc_profile_mem_create(unit, &mem, &entry_words, 1,
                    info->mac_da_profile);
            info->mac_da_profile_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }

    /* Create profile table cache (or re-init if it already exists) */
    mem = LPORT_TABm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->lport_profile && 
                soc_feature(unit, soc_feature_lport_tab_profile)) {
            info->lport_profile = sal_alloc(sizeof(soc_profile_mem_t),
                    "LPORT_PROFILE_TABLE Mem");
            if (info->lport_profile == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->lport_profile);

            entry_words = sizeof(lport_tab_entry_t) / sizeof(uint32);
            rv = soc_profile_mem_create(unit, &mem, &entry_words, 1,
                    info->lport_profile);
            info->lport_profile_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }

            if (SOC_WARM_BOOT(unit)) {
                /*
                 * Set at least one reference count for default entry.
                 *
                 * Other modules that make a reference to the LPORT table are
                 * responsible for updating the LPORT reference count.
                 */
                rv = _bcm_lport_profile_mem_reference
                    (unit,
                     _bcm_trx_lport_tab_default_entry_index_get(unit),
                     1, 1);
            } else {
                /* coverity[stack_use_callee] */
                /* coverity[stack_use_overflow] */
                rv = _bcm_trx_lport_tab_default_entry_add(unit,
                                                          info->lport_profile);
            }
            if (BCM_FAILURE(rv)) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }

    /* Create profile table cache (or re-init if it already exists) */
    mem = ING_PRI_CNG_MAPm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->ing_pri_cng_map) {
            info->ing_pri_cng_map = sal_alloc(sizeof(soc_profile_mem_t),
                                              "ING_PRI_CNG_MAP Profile Mem");
            if (info->ing_pri_cng_map == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->ing_pri_cng_map);

            mems_cnt = 0;
            mems[mems_cnt] = mem;
            mem_words[mems_cnt] = sizeof(ing_pri_cng_map_entry_t) /
                sizeof(uint32);
            index_min[mems_cnt] = soc_mem_index_min(unit, mem);
            index_max[mems_cnt] = soc_mem_index_max(unit, mem);
            if (SOC_IS_TRIUMPH2(unit) || SOC_IS_APOLLO(unit) || 
                SOC_IS_VALKYRIE2(unit) || SOC_IS_ENDURO(unit) ||
                SOC_IS_HURRICANEX(unit) || SOC_IS_TRIUMPH3(unit)) {
                /* 63 profiles * 16 enitres for tagged packets
                 * entry 1008 to 1070 are 63 profiles for untagged packets */
                index_max[mems_cnt] = 1007;
            } else if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
                /* First 53 chunks are hard-wired to physical ports */
                /* Only the nine remaining are flexible profiles */    
                index_min[mems_cnt] = 848;
                index_max[mems_cnt] = 1007;
            }
            mems_cnt++;

            mem = ING_UNTAGGED_PHBm;
            if (SOC_MEM_IS_VALID(unit, mem)) {
                mems[mems_cnt] = mem;
                mem_words[mems_cnt] = sizeof(ing_untagged_phb_entry_t) /
                    sizeof(uint32);
                index_min[mems_cnt] = soc_mem_index_min(unit, mem);
                index_max[mems_cnt] = soc_mem_index_max(unit, mem);
                mems_cnt++;
            }
            rv = soc_profile_mem_index_create(unit, mems, mem_words,
                                              index_min, index_max, NULL,
                                              mems_cnt, info->ing_pri_cng_map);
            info->ing_pri_cng_map_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }

    mem = ING_VFT_PRI_MAPm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->ing_vft_pri_map) {
            info->ing_vft_pri_map = sal_alloc(sizeof(soc_profile_mem_t),
                                              "ING_VFT_PRI_MAP Profile Mem");
            if (info->ing_vft_pri_map == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->ing_vft_pri_map);

            mems_cnt = 0;
            mems[mems_cnt] = mem;
            mem_words[mems_cnt] = sizeof(ing_vft_pri_map_entry_t) /
                sizeof(uint32);
            index_min[mems_cnt] = soc_mem_index_min(unit, mem);
            index_max[mems_cnt] = soc_mem_index_max(unit, mem);
            mems_cnt++;

            rv = soc_profile_mem_index_create(unit, mems, mem_words,
                                              index_min, index_max, NULL,
                                              mems_cnt, info->ing_vft_pri_map);
            info->ing_vft_pri_map_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }

    mem = EGR_VFT_PRI_MAPm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->egr_vft_pri_map) {
            info->egr_vft_pri_map = sal_alloc(sizeof(soc_profile_mem_t),
                                              "EGR_VFT_PRI_MAP Profile Mem");
            if (info->egr_vft_pri_map == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->egr_vft_pri_map);

            mems_cnt = 0;
            mems[mems_cnt] = mem;
            mem_words[mems_cnt] = sizeof(egr_vft_pri_map_entry_t) /
                sizeof(uint32);
            index_min[mems_cnt] = soc_mem_index_min(unit, mem);
            index_max[mems_cnt] = soc_mem_index_max(unit, mem);
            mems_cnt++;

            rv = soc_profile_mem_index_create(unit, mems, mem_words,
                                              index_min, index_max, NULL,
                                              mems_cnt, info->egr_vft_pri_map);
            info->egr_vft_pri_map_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }

    mem = EGR_VSAN_INTPRI_MAPm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->egr_vsan_intpri_map) {
            info->egr_vsan_intpri_map = sal_alloc(sizeof(soc_profile_mem_t),
                                            "EGR_VSAN_INTPRI_MAP Profile Mem");
            if (info->egr_vsan_intpri_map == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->egr_vsan_intpri_map);

            mems_cnt = 0;
            mems[mems_cnt] = mem;
            mem_words[mems_cnt] = sizeof(egr_vsan_intpri_map_entry_t) /
                                      sizeof(uint32);
            index_min[mems_cnt] = soc_mem_index_min(unit, mem);
            index_max[mems_cnt] = soc_mem_index_max(unit, mem);
            mems_cnt++;

            rv = soc_profile_mem_index_create(unit, mems, mem_words,
                                              index_min, index_max, NULL,
                                              mems_cnt, 
                                              info->egr_vsan_intpri_map);
            info->egr_vsan_intpri_map_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }

    mem = EGR_VFT_FIELDS_PROFILEm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->egr_vft_fields) {
            info->egr_vft_fields = sal_alloc(sizeof(soc_profile_mem_t),
                                            "EGR_VFT_FIELDS_PROFILE Mem");
            if (info->egr_vft_fields == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->egr_vft_fields);

            mems_cnt = 0;
            mems[mems_cnt] = mem;
            mem_words[mems_cnt] = sizeof(egr_vft_fields_profile_entry_t) /
                                      sizeof(uint32);
            index_min[mems_cnt] = soc_mem_index_min(unit, mem);
            index_max[mems_cnt] = soc_mem_index_max(unit, mem);
            mems_cnt++;

            rv = soc_profile_mem_index_create(unit, mems, mem_words,
                                              index_min, index_max, NULL,
                                              mems_cnt, 
                                              info->egr_vft_fields);
            info->egr_vft_fields_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }

    mem = EGR_MPLS_PRI_MAPPINGm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->egr_mpls_combo_map) {
            info->egr_mpls_combo_map = sal_alloc(sizeof(soc_profile_mem_t),
                                                 "MPLS Combo Profile Mem");
            if (info->egr_mpls_combo_map == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->egr_mpls_combo_map);

            /* Prevent entries from moving around in the tables */
            info->egr_mpls_combo_map->flags = SOC_PROFILE_MEM_F_NO_SHARE;

            mems_cnt = 0;
            mems[mems_cnt] = mem;
            mem_words[mems_cnt] = sizeof(egr_mpls_pri_mapping_entry_t) /
                sizeof(uint32);
            mems_cnt++;
            if (SOC_MEM_IS_VALID(unit, EGR_MPLS_EXP_MAPPING_1m)) {
                mems[mems_cnt] = EGR_MPLS_EXP_MAPPING_1m;
                mem_words[mems_cnt] = sizeof(egr_mpls_exp_mapping_1_entry_t) /
                    sizeof(uint32);
                mems_cnt++;
            }
            if (SOC_MEM_IS_VALID(unit, EGR_MPLS_EXP_MAPPING_2m)) {
                mems[mems_cnt] = EGR_MPLS_EXP_MAPPING_2m;
                mem_words[mems_cnt] = sizeof(egr_mpls_exp_mapping_2_entry_t) /
                    sizeof(uint32);
                mems_cnt++;
            }
            rv = soc_profile_mem_create(unit, mems, mem_words, mems_cnt,
                                        info->egr_mpls_combo_map);
            info->egr_mpls_combo_map_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
            /* Create a set of default 1-to-1 internal priority to packet priority
             * mapping profiles for reference by egress objects such as egr_vlan_xlate
             * Must create first to get the 0 indexed profile. 
             */
            if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
                egr_mpls_pri_mapping_entry_t egr_mpls_pri_map[64];
                egr_mpls_exp_mapping_1_entry_t egr_mpls_exp_map1[64];
                egr_mpls_exp_mapping_2_entry_t egr_mpls_exp_map2[64];
                uint32 index;
                int int_pri;
                int cng;
                void *entries[3];

                mem = EGR_MPLS_PRI_MAPPINGm;
                mems_cnt = 0;
                if (SOC_MEM_IS_VALID(unit, mem)) {
                    sal_memset(egr_mpls_pri_map, 0, sizeof(egr_mpls_pri_map));
                    for (int_pri = 0; int_pri < 16; int_pri++) {
                        for (cng = 0; cng < 4; cng++) {
                            soc_mem_field32_set(unit, mem, 
                                (uint32 *)&egr_mpls_pri_map[(int_pri << 2) | cng],
                                NEW_PRIf, int_pri > 7? 7: int_pri);
                            soc_mem_field32_set(unit, mem, 
                                (uint32 *)&egr_mpls_pri_map[(int_pri << 2) | cng],
                                NEW_CFIf, cng > 1? 1:cng);
                        }
                    }
                    entries[mems_cnt++] = egr_mpls_pri_map;
                }

                mem = EGR_MPLS_EXP_MAPPING_1m;
                if (SOC_MEM_IS_VALID(unit, mem)) {
                    sal_memset(egr_mpls_exp_map1, 0, sizeof(egr_mpls_exp_map1));
                for (int_pri = 0; int_pri < 16; int_pri++) {
                    for (cng = 0; cng < 4; cng++) {
                 
                        soc_mem_field32_set(unit, mem, 
                            (uint32 *)&egr_mpls_exp_map1[(int_pri << 2) | cng],
                            PRIf, int_pri > 7? 7: int_pri);
                        soc_mem_field32_set(unit, mem, 
                            (uint32 *)&egr_mpls_exp_map1[(int_pri << 2) | cng],
                            CFIf, cng > 1?1:cng);
                    }
                }
                entries[mems_cnt++] = egr_mpls_exp_map1;
                }

                mem = EGR_MPLS_EXP_MAPPING_2m;
                    sal_memset(egr_mpls_exp_map2, 0, sizeof(egr_mpls_exp_map2));
                if (SOC_MEM_IS_VALID(unit, mem)) {
                for (int_pri = 0; int_pri < 16; int_pri++) {
                    for (cng = 0; cng < 4; cng++) {
                 
                        soc_mem_field32_set(unit, mem,
                            (uint32 *)&egr_mpls_exp_map2[(int_pri << 2) | cng],
                            PRIf, int_pri > 7? 7: int_pri);
                        soc_mem_field32_set(unit, mem, 
                            (uint32 *)&egr_mpls_exp_map2[(int_pri << 2) | cng],
                            CFIf, cng > 1?1:cng);
                    }
                }
                entries[mems_cnt++] = egr_mpls_exp_map2;
                }

                BCM_IF_ERROR_RETURN
                    (_bcm_egr_mpls_combo_map_entry_add(unit, entries, 64,
                                                       &index));
            }
        }
    }

    BCM_PBMP_CLEAR(all_pbmp);
    BCM_PBMP_ASSIGN(all_pbmp, PBMP_ALL(unit));
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit) && soc_feature(unit, soc_feature_flex_port)) {
        BCM_IF_ERROR_RETURN(_bcm_kt2_flexio_pbmp_update(unit, &all_pbmp));
    }
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        _bcm_kt2_subport_pbmp_update(unit, &all_pbmp);
    }
#endif

    mem = DSCP_TABLEm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->dscp_table) {
            info->dscp_table = sal_alloc(sizeof(soc_profile_mem_t),
                                         "DSCP_TABLE Profile Mem");
            if (info->dscp_table == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->dscp_table);

            entry_words = sizeof(dscp_table_entry_t) / sizeof(uint32);
            index_min[0] = soc_mem_index_min(unit, mem);
            index_max[0] = soc_mem_index_max(unit, mem);
            mems[0] = mem;
            mem_words[0] = entry_words;
            if (SOC_IS_TRIUMPH(unit) || SOC_IS_VALKYRIE(unit)) {
                /* First 53 chunks are hard-wired to physical ports */
                /* Only the nine remaining are flexible profiles */
                index_min[0] = 3392;
            }
            if (SOC_IS_KATANA(unit) || SOC_IS_TRIUMPH3(unit)) {
                index_min[0] = SOC_PORT_MAX(unit,all) * 64;
            }
            rv = soc_profile_mem_index_create(unit, mems, mem_words,
                                              index_min, index_max, NULL, 1,
                                              info->dscp_table);
            info->dscp_table_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
            if (SOC_IS_TD2_TT2(unit) || SOC_IS_KATANA2(unit)) {
                dscp_table_entry_t dscp_map_entries[64];
                int  count = 0, i = 0;
                port_tab_entry_t         pent;
                bcm_port_t port;
                void *entries_buf[1];
                uint32 dscp_index =0; 
                
                /* Create the default profile */

                sal_memset(dscp_map_entries, 0, sizeof(dscp_map_entries));

                entries_buf[0] = &dscp_map_entries;
                BCM_IF_ERROR_RETURN(soc_profile_mem_add(unit,
                    info->dscp_table, entries_buf, 64, &dscp_index));

                count = 0;
                soc_mem_lock(unit, PORT_TABm); 
                PBMP_ITER(all_pbmp, port) {
                    if (IS_LB_PORT(unit, port)) {
                        continue;
                    }

                  /* Get profile index from port table. */
                    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY,
                                        port, &pent);

                    if (BCM_FAILURE(rv)) {
                        soc_mem_unlock(unit, PORT_TABm);
                        return rv;
                    }
                    soc_mem_field32_set(unit, PORT_TABm, &pent, TRUST_DSCP_PTRf,
                            dscp_index / 64);
                    rv = soc_mem_write(unit, PORT_TABm, MEM_BLOCK_ALL,
                                        port, &pent);
 
                    if (BCM_FAILURE(rv)) {
                        soc_mem_unlock(unit, PORT_TABm);
                        return rv;
                    }
                    count++;
                }
                soc_mem_unlock(unit, PORT_TABm);
                for (i = 1; i < count; i++) {
                     soc_profile_mem_reference(unit,
                        info->dscp_table, dscp_index, 0);
                }
            }
        }
    }
    mem = EGR_DSCP_TABLEm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->egr_dscp_table) {
            info->egr_dscp_table = sal_alloc(sizeof(soc_profile_mem_t),
                                             "EGR_DSCP_TABLE Profile Mem");
            if (info->egr_dscp_table == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->egr_dscp_table);

            entry_words = sizeof(egr_dscp_table_entry_t) / sizeof(uint32);
            rv = soc_profile_mem_create(unit, &mem, &entry_words, 1,
                                        info->egr_dscp_table);
            info->egr_dscp_table_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }
    if (SOC_REG_INFO(unit, PROTOCOL_PKT_CONTROLr).regtype != soc_portreg &&
        SOC_REG_INFO(unit, IGMP_MLD_PKT_CONTROLr).regtype != soc_portreg) {
        if (info->prot_pkt_ctrl == NULL) {
            soc_reg_t reg[2];
            info->prot_pkt_ctrl = 
                sal_alloc(sizeof(soc_profile_reg_t),
                          "PROTOCOL_PKT_CONTROL Profile Reg");
            if (info->prot_pkt_ctrl == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_reg_t_init(info->prot_pkt_ctrl);

            reg[0] = PROTOCOL_PKT_CONTROLr;
            reg[1] = IGMP_MLD_PKT_CONTROLr;
            rv = soc_profile_reg_create(unit, reg, 2,
                                        info->prot_pkt_ctrl);
            info->prot_pkt_ctrl_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }
#ifdef BCM_TRIDENT_SUPPORT
    mem = ING_OUTER_DOT1P_MAPPING_TABLEm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        uint32 idx, max;
        ing_outer_dot1p_mapping_table_entry_t dot1p[16];
        void *entries[1];
        int base = 0;

        if (NULL == info->ing_outer_dot1p) {
            info->ing_outer_dot1p = sal_alloc(sizeof(soc_profile_mem_t),
                               "ING_OUTER_DOT1P_MAPPING_TABLE Profile Mem");
            if (info->ing_outer_dot1p == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->ing_outer_dot1p);

            entry_words = sizeof(ing_outer_dot1p_mapping_table_entry_t) / 
                          sizeof(uint32);
            rv = soc_profile_mem_create(unit, &mem, &entry_words, 1,
                                        info->ing_outer_dot1p);
            info->ing_outer_dot1p_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
        /* Add default profile entry - identity mapping */
        idx = soc_mem_index_min(unit, mem);
        max = 16;
        sal_memset(&dot1p, 0, sizeof(dot1p));
        while (BCM_SUCCESS(rv) && idx < max) {
            soc_mem_field32_set(unit, mem, &dot1p[idx], NEW_CFIf, idx & 1);
            soc_mem_field32_set(unit, mem, &dot1p[idx], NEW_DOT1Pf, 
                                (idx >> 1) & 7);
            idx++;
        }
        entries[0] = &dot1p;
        BCM_IF_ERROR_RETURN
            (soc_profile_mem_add(unit, ING_OUTER_DOT1P(unit), entries, 16, 
                                 (uint32 *)&base));
    }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH2_SUPPORT
    if (SOC_IS_TRIUMPH2(unit) || SOC_IS_VALKYRIE2(unit) ||
        SOC_IS_HURRICANEX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_APOLLO(unit) || SOC_IS_TD_TT(unit) || SOC_IS_KATANAX(unit) ||
        SOC_IS_ENDURO(unit)) {
        rv = _bcm_tr2_ing_pri_cng_map_default_entry_add
            (unit, info->ing_pri_cng_map);
        if (BCM_FAILURE(rv)) {
            _bcm_common_free_resource(unit, info);
            return rv;
        }
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    mem = ING_L3_NEXT_HOP_ATTRIBUTE_1m;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->ing_l3_nh_attribute) {
            info->ing_l3_nh_attribute = sal_alloc(sizeof(soc_profile_mem_t),
                                     "ING_L3_NEXT_HOP_ATTRIBUTE_1 Profile Mem");
            if (info->ing_l3_nh_attribute == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->ing_l3_nh_attribute);

            entry_words = sizeof(ing_l3_next_hop_attribute_1_entry_t) /
                                 sizeof(uint32);

            rv = soc_profile_mem_create(unit, &mem, &entry_words, 1,
                                        info->ing_l3_nh_attribute);
            info->ing_l3_nh_attribute_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
    mem = L3_IIF_PROFILEm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->l3_iif_profile) {
            info->l3_iif_profile = sal_alloc(sizeof(soc_profile_mem_t),
                                     "L3_IIF_PROFILE Mem");
            if (info->l3_iif_profile == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->l3_iif_profile);

            entry_words = soc_mem_entry_words(unit, mem);

            rv = soc_profile_mem_create(unit, &mem, &entry_words, 1,
                                        info->l3_iif_profile);
            info->l3_iif_profile_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }
    mem = IP_OPTION_CONTROL_PROFILE_TABLEm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->l3_ip4_options_profile) {
            info->l3_ip4_options_profile = sal_alloc(sizeof(soc_profile_mem_t),
                                         "IP_OPTION_CONTROL_PROFILE_TABLEm Profile Mem");
            if (info->l3_ip4_options_profile == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->l3_ip4_options_profile);

            entry_words = 1;
            index_min[0] = soc_mem_index_min(unit, mem);
            index_max[0] = soc_mem_index_max(unit, mem);
            mems[0] = mem;
            mem_words[0] = entry_words;
            rv = soc_profile_mem_index_create(unit, mems, mem_words,
                                              index_min, index_max, NULL, 1,
                                              info->l3_ip4_options_profile);
            info->l3_ip4_options_profile_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }

    mem = FC_MAP_PROFILEm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->fc_map_profile) {
            info->fc_map_profile = sal_alloc(sizeof(soc_profile_mem_t),
                                             "FC MAP PROFILE Mem");
            if (info->fc_map_profile == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->fc_map_profile);

            entry_words = 1;
            index_min[0] = soc_mem_index_min(unit, mem);
            index_max[0] = soc_mem_index_max(unit, mem);
            mems[0] = mem;
            mem_words[0] = entry_words;
            rv = soc_profile_mem_index_create(unit, mems, mem_words,
                                              index_min, index_max, NULL, 1,
                                              info->fc_map_profile);
            info->fc_map_profile_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }

#endif /* BCM_TRIDENT2_SUPPORT */

   if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
    if (soc_feature(unit, soc_feature_gport_service_counters)) {
        /* Initialize flex stats */
        BCM_IF_ERROR_RETURN
            (_bcm_esw_flex_stat_init(unit, _bcmFlexStatTypeGport));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_flex_stat_init(unit, _bcmFlexStatTypeService));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_flex_stat_init(unit, _bcmFlexStatTypeEgressGport));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_flex_stat_init(unit, _bcmFlexStatTypeEgressService));
    }
   }
#endif /* BCM_TRIUMPH2_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    mem = ING_QUEUE_OFFSET_MAPPING_TABLEm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->offset_map_table) {
            info->offset_map_table = sal_alloc(sizeof(soc_profile_mem_t),
                                         "OFFSET_MAP_TABLE Profile Mem");
            if (info->offset_map_table == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->offset_map_table);

            entry_words = 1;
            index_min[0] = soc_mem_index_min(unit, mem);
            index_max[0] = soc_mem_index_max(unit, mem);
            mems[0] = mem;
            mem_words[0] = entry_words;
            rv = soc_profile_mem_index_create(unit, mems, mem_words,
                                              index_min, index_max, NULL, 1,
                                              info->offset_map_table);
            info->ing_queue_offset_map_created = TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }
#endif /* BCM_KATANA_SUPPORT */

#ifdef BCM_KATANA2_SUPPORT
    mem = VLAN_PROTOCOL_DATAm;
    if (SOC_MEM_IS_VALID(unit, mem)) {
        if (NULL == info->vlan_protocol_data_profile) {
            info->vlan_protocol_data_profile =
                sal_alloc(sizeof(soc_profile_mem_t),
                "VLAN_PROTOCOL_DATA Profile Mem");
            if (info->vlan_protocol_data_profile == NULL) {
                _bcm_common_free_resource(unit, info);
                return BCM_E_MEMORY;
            }
            soc_profile_mem_t_init(info->vlan_protocol_data_profile);

            entry_words = sizeof(vlan_protocol_data_entry_t) / sizeof(uint32);
            index_min[0] = soc_mem_index_min(unit, mem);
            index_max[0] = soc_mem_index_max(unit, mem);
            mems[0] = mem;
            mem_words[0] = entry_words;
            rv = soc_profile_mem_index_create(unit, mems, mem_words,
                                              index_min, index_max, NULL, 1,
                                              info->vlan_protocol_data_profile);
            info->vlan_protocol_data_profile_created= TRUE;
            if (rv < 0) {
                _bcm_common_free_resource(unit, info);
                return rv;
            }
        }
    }
#endif /* BCM_KATANA2_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        rv = _bcm_common_reinit(unit);
        if (BCM_FAILURE(rv)) {
            _bcm_common_free_resource(unit, info);
            return rv;
        }
    }
#endif /* BCM_WARM_BOOT_SUPPORT */
    
    info->initialized = TRUE;
    return rv;
}

/*
 * Function:
 *      _bcm_mac_da_profile_entry_add
 * Purpose:
 *      Internal function for adding an entry to the MAC_DA_PROFILE table
 *      Adds an entry to the global shared SW copy of the MAC_DA_PROFILE table
 *      Shared by MPLS and MIM
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_mac_da_profile_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, MAC_DA_PROFILE(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_mac_da_profile_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the MAC_DA_PROFILE table
 *      Deletes an entry from the global shared SW copy of the MAC_DA_PROFILE table
 *      Shared by MPLS and MIM
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_mac_da_profile_entry_delete(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, MAC_DA_PROFILE(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_lport_profile_mem_reference
 * Purpose:
 *      Update LPORT_PROFILE_TABLE entry reference count and entries_per_set 
 *      information for given index.
 * Parameters:
 *      unit            - (IN) Device number
 *      index           - (IN) Base index for entry to update
 *      count           - (IN) Count to increment
 *      entries_per_set - (IN) Number of entries in the set
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_lport_profile_mem_reference(int unit, int index,
                                 int count, int entries_per_set)
{
    COMMON_LOCK(unit);
    SOC_PROFILE_MEM_REFERENCE(unit, LPORT_PROFILE(unit), index, count);
    SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, LPORT_PROFILE(unit), index,
                                    entries_per_set);
    COMMON_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_lport_profile_entry_add
 * Purpose:
 *      Internal function for adding an entry to the LPORT_PROFILE_TABLE.
 *      Adds an entry to the global shared SW copy of the LPORT_PROFILE_TABLE.
 *      Shared by WLAN, proxy, and SOURCE_TRUNK_MAP table.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entries allocated in HW
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_lport_profile_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, LPORT_PROFILE(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_lport_profile_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the LPORT_PROFILE_TABLE.
 *      Deletes an entry from the global shared SW copy of the
 *      LPORT_PROFILE_TABLE.
 *      Shared by WLAN, proxy, and SOURCE_TRUNK_MAP table.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be deleted
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_lport_profile_entry_delete(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, LPORT_PROFILE(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_ing_vft_pri_map_entry_add
 * Purpose:
 *      Adds an entry to the global shared SW copy of the ING_VFT_PRI_MAP table.
 * Parameters:
 *      unit            - (IN)  Device number.
 *      entries         - (IN)  Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index           - (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_ing_vft_pri_map_entry_add(int unit, void **entries,
                               int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, ING_VFT_PRI_MAP(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_egr_vft_pri_map_entry_add
 * Purpose:
 *      Adds an entry to the EGR_VFT_PRI_MAP table (both the hardware and 
 *      the global shared SW cache).
 * Parameters:
 *      unit            - (IN)  Device number.
 *      entries         - (IN)  Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index           - (OUT) Base index for the entries allocated
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_egr_vft_pri_map_entry_add(int unit, void **entries,
                               int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, EGR_VFT_PRI_MAP(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_egr_vsan_intpri_map_entry_add
 * Purpose:
 *      Adds an entry to the EGR_VSAN_INTPRI_MAP table (both the hardware and 
 *      the global shared SW cache).
 * Parameters:
 *      unit            - (IN)  Device number.
 *      entries         - (IN)  Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index           - (OUT) Base index for the entries allocated
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_egr_vsan_intpri_map_entry_add(int unit, void **entries,
                                   int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, EGR_VSAN_INTPRI_MAP(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_egr_vft_fields_entry_add
 * Purpose:
 *      Adds an entry to the EGR_VFT_FIELDS_PROFILE table (both the hardware and 
 *      the global shared SW cache).
 * Parameters:
 *      unit            - (IN)  Device number.
 *      entries         - (IN)  Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index           - (OUT) Base index for the entries allocated
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_egr_vft_fields_entry_add(int unit, void **entries, int entries_per_set,
                              uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, EGR_VFT_FIELDS(unit), entries, 
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function: 
 *     _bcm_ing_vft_pri_map_entry_del
 * Purpose:
 *      Deletes an entry from the ING_VFT_PRI_MAP table (both the hardware and
 *      the global shared SW cache).
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entries to be deleted
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_ing_vft_pri_map_entry_del(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, ING_VFT_PRI_MAP(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function: 
 *     _bcm_egr_vft_pri_map_entry_del
 * Purpose:
 *      Deletes an entry from the EGR_VFT_PRI_MAP table (both the hardware and
 *      the global shared SW cache).
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entries to be deleted
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_egr_vft_pri_map_entry_del(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, EGR_VFT_PRI_MAP(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function: 
 *     _bcm_egr_vft_fields_entry_del
 * Purpose:
 *      Deletes an entry from the EGR_VFT_FIELDS_PROFILE table (both the 
 *      hardware and the global shared SW cache).
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entries to be deleted
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_egr_vft_fields_entry_del(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, EGR_VFT_FIELDS(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function: 
 *     _bcm_egr_vsan_intpri_map_entry_del
 * Purpose:
 *      Deletes an entry from the EGR_VSAN_INTPRI_MAP table (both the hardware
 *      and the global shared SW cache).
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entries to be deleted
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_egr_vsan_intpri_map_entry_del(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, EGR_VSAN_INTPRI_MAP(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function: 
 *     _bcm_ing_vft_pri_map_entry_get
 * Purpose:
 *     Get a set of entries from the ING_VFT_PRI_MAP table.
 * Parameters:
 *     unit    - (IN)  Device number.
 *     index   - (IN)  Base index for the entries to be retrieved
 *     count   - (IN)  Number of entries in the set
 *     entries - (OUT) Array of pointer to table entries set
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_ing_vft_pri_map_entry_get(int unit, int index, int count, void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, ING_VFT_PRI_MAP(unit), index, count,
                             entries);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function: 
 *     _bcm_egr_vft_pri_map_entry_get
 * Purpose:
 *     Get a set of entries from the EGR_VFT_PRI_MAP table.
 * Parameters:
 *     unit    - (IN)  Device number.
 *     index   - (IN)  Base index for the entries to be retrieved
 *     count   - (IN)  Number of entries in the set
 *     entries - (OUT) Array of pointer to table entries set
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_egr_vft_pri_map_entry_get(int unit, int index, int count, void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, EGR_VFT_PRI_MAP(unit), index, count,
                             entries);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function: 
 *     _bcm_egr_vft_fields_entry_get
 * Purpose:
 *     Get a set of entries from the EGR_VFT_FIELDS_PROFILE table.
 * Parameters:
 *     unit    - (IN)  Device number.
 *     index   - (IN)  Base index for the entries to be retrieved
 *     count   - (IN)  Number of entries in the set
 *     entries - (OUT) Array of pointer to table entries set
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_egr_vft_fields_entry_get(int unit, int index, int count, void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, EGR_VFT_FIELDS(unit), index, count,
                             entries);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function: 
 *     _bcm_egr_vsan_intpri_map_entry_get
 * Purpose:
 *     Get a set of entries from the EGR_VSAN_INTPRI_MAP table.
 * Parameters:
 *     unit    - (IN)  Device number.
 *     index   - (IN)  Base index for the entries to be retrieved
 *     count   - (IN)  Number of entries in the set
 *     entries - (OUT) Array of pointer to table entries set
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_egr_vsan_intpri_map_entry_get(int unit, int index, int count, 
                                   void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, EGR_VSAN_INTPRI_MAP(unit), index, count,
                             entries);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_lport_profile_entry_get
 * Purpose:
 *      Internal function for getting an entry from the LPORT_PROFILE_TABLE.
 *      Gets an entry from the global shared SW copy of the
 *      LPORT_PROFILE_TABLE.
 *      Shared by WLAN and proxy, and SOURCE_TRUNK_MAP table.
 * Parameters:
 *      unit    - (IN) Device number
 *      index   - (IN) Base index to the entries in HW for table
 *      count   - (IN) Array of number of entries to retrieve
 *      entries - (OUT) Array of pointer to table entries to get
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_lport_profile_entry_get(int unit, int index, int count, void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, LPORT_PROFILE(unit), index, count,
                             entries);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_lport_profile_fields32_modify
 * Purpose:
 *      Modify the specified fields for all valid (non-zero reference count)
 *      entries in the LPORT Profile Table.
 *
 *      This is useful when a field(s) needs to be set globally
 *      in the profile table.
 * Parameters:
 *      unit        - (IN) Device number
 *      field_count - (IN) Number of fields to modify
 *      fields      - (IN) Array of fields to modify
 *      values      - (IN) Array of new field values
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_lport_profile_fields32_modify(int unit, int field_count,
                                   soc_field_t *fields, uint32 *values)
{
    int rv;
    int i;
    soc_mem_t mem = LPORT_TABm;

    /* Check all fields are valid */
    for (i = 0; i < field_count; i++) {
        if (!SOC_MEM_FIELD_VALID(unit, mem, fields[i])) {
            return BCM_E_UNAVAIL;
        }
    }

    COMMON_LOCK(unit);
    rv = soc_profile_mem_fields32_modify(unit, LPORT_PROFILE(unit), 0,
                                         field_count, fields, values);
    COMMON_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      _bcm_lport_profile_field32_modify
 * Purpose:
 *      Modify the specified field for all valid (non-zero reference count)
 *      entries in the LPORT Profile Table.
 *
 *      This is useful when a field needs to be set globally
 *      in the profile table.
 * Parameters:
 *      unit     - (IN) Device number
 *      field    - (IN) Fields to modify
 *      value    - (IN) New field value
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_lport_profile_field32_modify(int unit, soc_field_t field, uint32 value)
{
    return _bcm_lport_profile_fields32_modify(unit, 1, &field, &value);
}

/*
 * Function:
 *      _bcm_ing_pri_cng_map_entry_add
 * Purpose:
 *      Internal function for adding an entry to the ING_PRI_CNG_MAP table
 *      Adds an entry to the global shared SW copy of the ING_PRI_CNG_MAP table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_ing_pri_cng_map_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, ING_PRI_CNG_MAP(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_ing_pri_cng_map_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the ING_PRI_CNG_MAP table
 *      Deletes an entry from the global shared SW copy of the ING_PRI_CNG_MAP table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be deleted
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_ing_pri_cng_map_entry_delete(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, ING_PRI_CNG_MAP(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

int
_bcm_ing_pri_cng_map_entry_get(int unit, int index, int count, void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, ING_PRI_CNG_MAP(unit), index, count,
                             entries);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_ing_pri_cng_map_entry_reference
 * Purpose:
 *      Internal function for indicating that an entry in ING_PRI_CNG_MAP table
 *      is being used. Updates the global shared SW copy.
 * Parameters:
 *      unit    -  (IN) Device number
 *      index   -  (IN) Base index for the entry to be updated
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_ing_pri_cng_map_entry_reference(int unit, int index,
                                     int entries_per_set_override) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_reference(unit, ING_PRI_CNG_MAP(unit), index, 
                                   entries_per_set_override);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_egr_mpls_combo_map_entry_add
 * Purpose:
 *      Internal function for adding an entry to the combined
 *      EGR_MPLS_PRI_MAPPING and EGR_MPLS_EXP_MAPPING_1 table.
 *      Adds an entry to the global shared SW copy of the combo table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_egr_mpls_combo_map_entry_add(int unit, void **entries, 
                                int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, EGR_MPLS_COMBO_MAP(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_egr_mpls_combo_map_entry_update
 * Purpose:
 *      Internal function for updating an entry to the combined
 *      EGR_MPLS_PRI_MAPPING and EGR_MPLS_EXP_MAPPING_1 table.
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      index   -  (OUT) Base index to update
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_egr_mpls_combo_map_entry_update(int unit, void **entries, 
                                     uint32 index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_set(unit, EGR_MPLS_COMBO_MAP(unit), entries,
                             index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_egr_mpls_combo_map_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the combined
 *      EGR_MPLS_PRI_MAPPING and EGR_MPLS_EXP_MAPPING_1 table.
 *      Deletes an entry from the global shared SW copy of the combo table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_egr_mpls_combo_map_entry_delete(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, EGR_MPLS_COMBO_MAP(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_egr_mpls_combo_map_entry_reference
 * Purpose:
 *      Internal function for indicating an entry in the combined
 *      EGR_MPLS_PRI_MAPPING and EGR_MPLS_EXP_MAPPING_1 table
 *      is being used. Updates global shared SW copy.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be updated
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_egr_mpls_combo_map_entry_reference(int unit, int index, 
                                        int entries_per_set_override) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_reference(unit, EGR_MPLS_COMBO_MAP(unit), index, 
                                   entries_per_set_override);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_dscp_table_entry_add
 * Purpose:
 *      Internal function for adding an entry to the DSCP_TABLE table
 *      Adds an entry to the global shared SW copy of the DSCP_TABLE table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_dscp_table_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, DSCP_TABLE(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_dscp_table_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the DSCP_TABLE table
 *      Deletes an entry from the global shared SW copy of the DSCP_TABLE table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be deleted
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_dscp_table_entry_delete(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, DSCP_TABLE(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_dscp_table_entry_reference
 * Purpose:
 *      Internal function for indicating an entry in the DSCP_TABLE table
 *      is being used. Updates the global shared SW copy
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be updated
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_dscp_table_entry_reference(int unit, int index, 
                                int entries_per_set_override) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_reference(unit, DSCP_TABLE(unit), index, 
                                   entries_per_set_override);
    COMMON_UNLOCK(unit);
    return rv;
}

int
_bcm_dscp_table_entry_ref_count_get(int unit, uint32 index,
                                                int* ref_count)
{
    int rv;
    rv = soc_profile_mem_ref_count_get(unit,
                                       DSCP_TABLE(unit), index, ref_count);
    return rv;
}

int
_bcm_dscp_table_entry_get(int unit, int index, int count, void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, DSCP_TABLE(unit), index, count,
                             entries);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_egr_dscp_table_entry_add
 * Purpose:
 *      Internal function for adding an entry to the EGR_DSCP_TABLE table
 *      Adds an entry to the global shared SW copy of the EGR_DSCP_TABLE table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_egr_dscp_table_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, EGR_DSCP_TABLE(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_egr_dscp_table_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the EGR_DSCP_TABLE table
 *      Deletes an entry from the global shared SW copy of the EGR_DSCP_TABLE table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be deleted
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_egr_dscp_table_entry_delete(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, EGR_DSCP_TABLE(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_egr_dscp_table_entry_reference
 * Purpose:
 *      Internal function for indicating an entry from the EGR_DSCP_TABLE table
 *      is being used. Updates the global shared SW copy
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be updated
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_egr_dscp_table_entry_reference(int unit, int index, 
                                    int entries_per_set_override) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_reference(unit, EGR_DSCP_TABLE(unit), index, 
                                   entries_per_set_override);
    COMMON_UNLOCK(unit);
    return rv;
}

int
_bcm_prot_pkt_ctrl_add(int unit, uint32 prot_pkt_ctrl,
                       uint32 igmp_mld_pkt_ctrl, uint32 *index)
{
    int rv;
    uint64 rval64[2], *rval64s[2];

    COMPILER_64_SET(rval64[0], 0, prot_pkt_ctrl);
    COMPILER_64_SET(rval64[1], 0, igmp_mld_pkt_ctrl);
    rval64s[0] = &rval64[0];
    rval64s[1] = &rval64[1];

    COMMON_LOCK(unit);
    rv = soc_profile_reg_add(unit, PROT_PKT_CTRL(unit), rval64s, 1, index);
    COMMON_UNLOCK(unit);

    return rv;
}

int
_bcm_prot_pkt_ctrl_delete(int unit, uint32 index)
{
    int rv;

    COMMON_LOCK(unit);
    rv = soc_profile_reg_delete(unit, PROT_PKT_CTRL(unit), index);
    COMMON_UNLOCK(unit);

    return rv;
}

int
_bcm_prot_pkt_ctrl_get(int unit, uint32 index, uint32 *prot_pkt_ctrl,
                       uint32 *igmp_mld_pkt_ctrl)
{
    int rv;
    uint64 rval64[2], *rval64s[2];

    rval64s[0] = &rval64[0];
    rval64s[1] = &rval64[1];

    COMMON_LOCK(unit);
    rv = soc_profile_reg_get(unit, PROT_PKT_CTRL(unit), index, 1, rval64s);
    COMMON_UNLOCK(unit);

    if (BCM_SUCCESS(rv)) {
        *prot_pkt_ctrl = COMPILER_64_LO(rval64[0]);
        *igmp_mld_pkt_ctrl = COMPILER_64_LO(rval64[1]);
    }

    return rv;
}

int
_bcm_prot_pkt_ctrl_reference(int unit, uint32 index)
{
    int rv;

    COMMON_LOCK(unit);
    rv = soc_profile_reg_reference(unit, PROT_PKT_CTRL(unit), index, 1);
    COMMON_UNLOCK(unit);

    return rv;
}

int
_bcm_prot_pkt_ctrl_ref_count_get(int unit, uint32 index,
                                                int* ref_count)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_reg_ref_count_get(unit,
                                       PROT_PKT_CTRL(unit), index, ref_count);
    COMMON_UNLOCK(unit);

    return rv;
}


#ifdef BCM_TRIUMPH3_SUPPORT
/*
 * Function:
 *      _bcm_ing_l3_nh_attrib_entry_add
 * Purpose:
 *      Internal function for adding an entry to the ING_L3_NEXT_HOP_ATTRIBUTE_1
 *      table. Adds an entry to the global shared SW copy of the
 *      ING_L3_NEXT_HOP_ATTRIBUTE_1 table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_ing_l3_nh_attrib_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, ING_L3_NH_ATTRIB_PROFILE(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_ing_l3_nh_attrib_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the
 *      ING_L3_NEXT_HOP_ATTRIBUTE_1 table Deletes an entry from
 *      the global shared SW copy of the IN_L3_NEXT_HOP_ATTRIBUTE_1 table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be deleted
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_ing_l3_nh_attrib_entry_delete(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, ING_L3_NH_ATTRIB_PROFILE(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_ing_l3_nh_attrib_entry_reference
 * Purpose:
 *      Internal function for indicating an entry in the
 *      IN_L3_NEXT_HOP_ATTRIBUTE_1 table is being used.
 *      Updates the global shared SW copy
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be updated
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_ing_l3_nh_attrib_entry_reference(int unit, int index, 
                                int entries_per_set_override) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_reference(unit, ING_L3_NH_ATTRIB_PROFILE(unit), index, 
                                   entries_per_set_override);
    COMMON_UNLOCK(unit);
    return rv;
}
#endif /* BCM_TRIUMPH3_SUPPORT */


#ifdef BCM_TRIDENT2_SUPPORT
/*
 * Function:
 *      _bcm_l3_iif_profile_entry_add
 * Purpose:
 *      Internal function for adding an entry to the L3_IIF_PROFILE table
 *      Adds an entry to the global shared SW copy of the L3_IIF_PROFILE table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_l3_iif_profile_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, L3_IIF_PROFILE(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_l3_iif_profile_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the L3_IIF_PROFILE table
 *      Deletes an entry from the global shared SW copy of the L3_IIF_PROFILE table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be deleted
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_l3_iif_profile_entry_delete(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, L3_IIF_PROFILE(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

int
_bcm_l3_iif_profile_entry_get(int unit, int index, int count, void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, L3_IIF_PROFILE(unit), index, count,
                             entries);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_l3_iif_profile_entry_reference
 * Purpose:
 *      Internal function for indicating an entry in the
 *      L3_IIF_PROFILE table is being used.
 *      Updates the global shared SW copy
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be updated
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_l3_iif_profile_entry_reference(int unit, int index, 
                                int entries_per_set_override) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_reference(unit, L3_IIF_PROFILE(unit), index, 
                                   entries_per_set_override);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_l3_iif_profile_entry_update
 * Purpose:
 *      Internal function for updating an entry to the combined
 *      L3_IIF_PROFILE and L3_IIF_PROFILE table.
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      index   -  (IN) Base index to update
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_l3_iif_profile_entry_update(int unit, void **entries, 
                                     uint32 index)
{
    int rv;
    int ref_count;
    void *entries_get[1];
    iif_profile_entry_t l3_iif_profile;

    /* Do not overwrite the default enrty */
    if (index == 0) {
        return BCM_E_INTERNAL;
    }

    entries_get[0] = &l3_iif_profile;
    BCM_IF_ERROR_RETURN
        (_bcm_l3_iif_profile_entry_get(unit, index, 1, entries_get));

    if (!sal_memcmp(entries[0], &l3_iif_profile, sizeof(iif_profile_entry_t))) {
        /* Profile entry needs no update */
        return BCM_E_NONE;
    }

    COMMON_LOCK(unit);
   
    rv = soc_profile_mem_ref_count_get(unit, L3_IIF_PROFILE(unit), index, &ref_count);
    if (SOC_FAILURE(rv)) {
        COMMON_UNLOCK(unit);
        return rv;
    }

    if (0 == ref_count) {
       /* Profile entry does not exist */ 
       rv = BCM_E_NOT_FOUND;
    }

    if (1 == ref_count) {
        /* Profile entry exist and not shared */
        rv = soc_profile_mem_set(unit, L3_IIF_PROFILE(unit), entries,
                                 index);
    } else {
        /* Profile entry exists but referenced by other iif; return error
         * so the caller can delete existing and create new entry         */
        rv = BCM_E_BUSY;
    }

    COMMON_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      _bcm_l3_ip4_options_profile_entry_add
 * Purpose:
 *      Internal function for adding an entry to the L3_IP_OPTIONS_PROFILE table
 *      Adds an entry to the global shared SW copy of the L3_IP_OPTIONS_PROFILE table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_l3_ip4_options_profile_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, L3_IP4_OPTIONS_PROFILE(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_l3_ip4_options_profile_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the L3_IP_OPTIONS_PROFILE table
 *      Deletes an entry from the global shared SW copy of the L3_IP_OPTIONS_PROFILE table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be deleted
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_l3_ip4_options_profile_entry_delete(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, L3_IP4_OPTIONS_PROFILE(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

int
_bcm_l3_ip4_options_profile_entry_get(int unit, int index, int count, void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, L3_IP4_OPTIONS_PROFILE(unit), index, count,
                             entries);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_l3_ip4_options_profile_entry_reference
 * Purpose:
 *      Internal function for indicating an entry in the
 *      L3_IP_OPTIONS_PROFILE table is being used.
 *      Updates the global shared SW copy
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be updated
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_l3_ip4_options_profile_entry_reference(int unit, int index, 
                                int entries_per_set_override) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_reference(unit, L3_IP4_OPTIONS_PROFILE(unit), index, 
                                   entries_per_set_override);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_l3_ip4_options_profile_entry_update
 * Purpose:
 *      Internal function for updating an entry to the combined
 *      L3_IP_OPTIONS_PROFILE and L3_IP_OPTIONS_PROFILE table.
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      index   -  (OUT) Base index to update
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_l3_ip4_options_profile_entry_update(int unit, void **entries, 
                                     uint32 index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_set(unit, L3_IP4_OPTIONS_PROFILE(unit), entries,
                             index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_fc_map_profile_entry_add
 * Purpose:
 *      Internal function for adding an entry to the FC_MAP_PROFILE table
 *      Adds an entry to the global shared SW copy of the FC_MAP_PROFILE table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_fc_map_profile_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, FC_MAP_PROFILE(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_fc_profile_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the FC_MAP_PROFILE table
 *      Deletes an entry from the global shared SW copy of the FC_MAP_PROFILE
 *      table shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be deleted
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_fc_map_profile_entry_delete(int unit, int index) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, FC_MAP_PROFILE(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

int
_bcm_fc_map_profile_entry_get(int unit, int index, int count, void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, FC_MAP_PROFILE(unit), index, count,
                             entries);
    COMMON_UNLOCK(unit);
    return rv;
}

#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
/*
 * Function:
 *      _bcm_offset_map_table_entry_add
 * Purpose:
 *      Internal function for adding an entry to the 
 *      ING_QUEUE_OFFSET_MAPPING_TABLE table.
 *      Adds an entry to the global shared SW copy of the 
 *      ING_QUEUE_OFFSET_MAPPING_TABLE table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */


int
_bcm_offset_map_table_entry_add(int unit, void **entries,
                                int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, OFFSET_MAP_TABLE(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}
/*
 * Function:
 *      _bcm_offset_map_table_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the 
 *      ING_QUEUE_OFFSET_MAPPING_TABLE table
 *      Deletes an entry from the global shared SW copy of the 
 *      ING_QUEUE_OFFSET_MAPPING_TABLE table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be deleted
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_offset_map_table_entry_delete(int unit, int index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, OFFSET_MAP_TABLE(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_offset_map_table_entry_get
 * Purpose:
 *      Internal function for getting an entry from the 
 *      ING_QUEUE_OFFSET_MAPPING_TABLE table
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index  - (IN) Base index for the entry to be deleted
 *      count  - (IN) number of entries  
 *      entries - (OUT) enrtries
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_offset_map_table_entry_get(int unit, int index, int count, 
                                      void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, OFFSET_MAP_TABLE(unit), index, count,
                             entries);	
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_offset_map_table_entry_reference
 * Purpose:
 *      Internal function for indicating an entry in the 
 *      ING_QUEUE_OFFSET_MAPPING_TABLE table
 *      is being used. Updates the global shared SW copy
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be updated
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_offset_map_table_entry_reference(int unit, int index,
                                int entries_per_set_override)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_reference(unit, OFFSET_MAP_TABLE(unit), index,
                                   entries_per_set_override);
    COMMON_UNLOCK(unit);
    return rv;
}
#endif /* BCM_KATANA_SUPPORT */

#if defined(BCM_KATANA2_SUPPORT)
/*
 * Function:
 *      _bcm_port_vlan_protocol_data_entry_add
 * Purpose:
 *      Internal function for adding an entry to the 
 *      VLAN_PROTOCOL_DATA table.
 *      Adds an entry to the global shared SW copy of the 
 *      VLAN_PROTOCOL_DATA table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      entries -  (IN) Array of pointer to table entries set
 *      entries_per_set - (IN)  Number of entries in the set
 *      index   -  (OUT) Base index for the entires allocated in HW
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_port_vlan_protocol_data_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_add(unit, VLAN_PROTOCOL_DATA_TABLE(unit), entries,
                             entries_per_set, index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_port_vlan_protocol_data_entry_delete
 * Purpose:
 *      Internal function for deleting an entry from the 
 *      VLAN_PROTOCOL_DATA table
 *      Deletes an entry from the global shared SW copy of the 
 *      VLAN_PROTOCOL_DATA table
 *      Shared by several applications.
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be deleted
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_port_vlan_protocol_data_entry_delete(int unit, int index)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_delete(unit, VLAN_PROTOCOL_DATA_TABLE(unit), index);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_port_vlan_protocol_data_entry_get
 * Purpose:
 *      Internal function for getting an entry from the 
 *      VLAN_PROTOCOL_DATA table
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index  - (IN) Base index for the entry to be deleted
 *      count  - (IN) number of entries  
 *      entries - (OUT) enrtries
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_port_vlan_protocol_data_entry_get(int unit,
    int index, int count, void **entries)
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_get(unit, VLAN_PROTOCOL_DATA_TABLE(unit), index, count,
                             entries);
    COMMON_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_vlan_port_protocol_data_entry_reference
 * Purpose:
 *      Internal function for indicating an entry in the 
 *      VLAN_PROTOCOL_DATA table
 *      is being used. Updates the global shared SW copy
 * Parameters:
 *      unit    -  (IN) Device number.
 *      index   -  (IN) Base index for the entry to be updated
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_port_vlan_protocol_data_entry_reference(int unit, int index,
                                int entries_per_set_override) 
{
    int rv;
    COMMON_LOCK(unit);
    rv = soc_profile_mem_reference(unit, VLAN_PROTOCOL_DATA_TABLE(unit), index, 
                                   entries_per_set_override);
    COMMON_UNLOCK(unit);
    return rv;
}

#endif /*BCM_KATANA2_SUPPORT */

void
_bcm_common_profile_mem_ref_cnt_update(int unit, soc_mem_t mem,
                                       int profile_ptr, int count)
{
    int entries_per_set, index, i;
    soc_profile_mem_t *profile;

    switch (mem) {
    case EGR_MAC_DA_PROFILEm:
        profile = MAC_DA_PROFILE(unit);
        entries_per_set = 1;
        break;
    case ING_PRI_CNG_MAPm:
        profile = ING_PRI_CNG_MAP(unit);
        entries_per_set = 16;
        break;
    case LPORT_TABm:
        profile = LPORT_PROFILE(unit);
        entries_per_set = 1;
        break;
    case EGR_MPLS_PRI_MAPPINGm:
    case EGR_MPLS_EXP_MAPPING_1m:
        profile = EGR_MPLS_COMBO_MAP(unit);
        entries_per_set = 64;
        break;
    case DSCP_TABLEm:
        profile = DSCP_TABLE(unit);
        entries_per_set = 64;
        break;
    case EGR_DSCP_TABLEm:
        profile = EGR_DSCP_TABLE(unit);
        entries_per_set = 64;
        break;
#ifdef BCM_TRIUMPH3_SUPPORT
    case ING_L3_NEXT_HOP_ATTRIBUTE_1m:
        profile = NULL;
        entries_per_set = 0;
        if (SOC_IS_TRIUMPH3(unit)) {
            profile = ING_L3_NH_ATTRIB_PROFILE(unit);
            entries_per_set = 1;
        }
        break;
#endif /* BCM_TRIUMPH3_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
    case L3_IIF_PROFILEm:
        profile = NULL;
        entries_per_set = 0;
        if (SOC_IS_TRIDENT2(unit)) {
            profile = L3_IIF_PROFILE(unit);
            entries_per_set = 1;
        }
        break;
    case IP_OPTION_CONTROL_PROFILE_TABLEm:
        profile = NULL;
        entries_per_set = 1;
        if (SOC_IS_TRIDENT2(unit)) {
            profile = L3_IP4_OPTIONS_PROFILE(unit);
            entries_per_set = 256;
        }
        break;
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA_SUPPORT
    case ING_QUEUE_OFFSET_MAPPING_TABLEm:
        profile = OFFSET_MAP_TABLE(unit);
        entries_per_set = 16;
        break;
#endif /* BCM_KATANA_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
    case VLAN_PROTOCOL_DATAm:
        profile = VLAN_PROTOCOL_DATA_TABLE(unit);
        entries_per_set = 16;
        break;
#endif /* BCM_KATANA2_SUPPORT */

    default:
        return;
    }

    index = profile_ptr * entries_per_set;
    for (i = 0; i < entries_per_set; i++) {
        SOC_PROFILE_MEM_REFERENCE(unit, profile, index + i, count);
        SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, profile, index + i, 
                                        entries_per_set);
    }
    return;
}

/* Structure used for hash calculation */
typedef struct _bcm_trx_mpls_station_key_s {
    bcm_mac_t  mac;
    bcm_vlan_t vlan;
} _bcm_trx_mpls_station_key_t;

/*
 * Function:
 *      _bcm_trx_mpls_station_hash_calc
 * Purpose:
 *      Calculate MPLS_STATION_TCAM entry hash.
 * Parameters:
 *      unit -  (IN)  Device number.
 *      mac  -  (IN)  MAC address
 *      vlan -  (IN)  VLAN ID
 *      hash -  (OUT) Hash valu.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_trx_mpls_station_hash_calc(int unit, bcm_mac_t mac,
                                bcm_vlan_t vlan, uint16 *hash)
{
    _bcm_trx_mpls_station_key_t key;

    if (hash == NULL) {
        return (BCM_E_PARAM);
    }
    sal_memcpy(key.mac, mac, sizeof(bcm_mac_t));
    key.vlan = vlan;
    *hash = _shr_crc16(0, (uint8 *)&key, sizeof(_bcm_trx_mpls_station_key_t));

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_l2_tunnel_add
 * Purpose:
 *    Add a (MAC, VLAN) for tunnel/MPLS processing.
 *    Frames destined to (MAC, VLAN) is subjected to MPLS processing.
 *    Shared by MPLS and MIM
 * Parameters:
 *      unit - Unit number
 *      mac  - MAC address
 *      vlan - VLAN ID
 */
int
bcm_trx_metro_l2_tunnel_add(int unit, bcm_mac_t mac, bcm_vlan_t vlan)
{
    mpls_station_tcam_entry_t station_entry;
    uint16 hash;
    bcm_mac_t hw_mac;
    bcm_vlan_t hw_vlan;
    int rv, i, num_station, free_index = -1;

    /* Get the hash value for this key */
    BCM_IF_ERROR_RETURN(_bcm_trx_mpls_station_hash_calc(unit, mac, vlan, &hash));

    /* see if the entry already exists or find free entry */
    num_station = soc_mem_index_count(unit, MPLS_STATION_TCAMm);
    COMMON_LOCK(unit);
    for (i = 0; i < num_station; i++) {
        if (hash == MPLS_STATION_HASH(unit, i)) {
            /* Read HW entry to see if it's really a match */
            rv = READ_MPLS_STATION_TCAMm(unit, MEM_BLOCK_ANY,
                                         i, &station_entry);
            if (rv < 0) {
                COMMON_UNLOCK(unit);
                return rv;
            }
            soc_mem_mac_addr_get(unit, MPLS_STATION_TCAMm,
                                 &station_entry, MAC_ADDRf, hw_mac);
            hw_vlan = soc_mem_field32_get(unit, MPLS_STATION_TCAMm,
                                          &station_entry, VLAN_IDf);
            if ((vlan == hw_vlan) &&
                !sal_memcmp(mac, hw_mac, sizeof(bcm_mac_t))) {
                break;
            }
        }
        if ((free_index == -1) && (MPLS_STATION_HASH(unit, i) == 0)) {
            /* Read HW entry to see if it's unused */
            rv = READ_MPLS_STATION_TCAMm(unit, MEM_BLOCK_ANY,
                                         i, &station_entry);
            if (rv < 0) {
                COMMON_UNLOCK(unit);
                return rv;
            }
            if (!soc_mem_field32_get(unit, MPLS_STATION_TCAMm,
                                     &station_entry, VALIDf)) {
                free_index = i;
            }
        }
    }
    if (i < num_station) {
        /* Entry already exists */
        COMMON_UNLOCK(unit);
        return BCM_E_EXISTS;
    } else if (free_index == -1) {
        /* Table is full */
        COMMON_UNLOCK(unit);
        return BCM_E_FULL;
    }
    /* Commit entry to HW */
    sal_memset(&station_entry, 0, sizeof(mpls_station_tcam_entry_t));
    soc_mem_field32_set(unit, MPLS_STATION_TCAMm, &station_entry, VALIDf, 1);
    soc_mem_mac_addr_set(unit, MPLS_STATION_TCAMm, &station_entry,
                         MAC_ADDRf, mac);
    soc_mem_field32_set(unit, MPLS_STATION_TCAMm, &station_entry,
                        VLAN_IDf, vlan);
    /* set masks to all 1's */
    for (i = 0; i < 6; i++) {
        hw_mac[i] = 0xff;
    }
    hw_vlan = 0xfff;
    soc_mem_mac_addr_set(unit, MPLS_STATION_TCAMm, &station_entry,
                         MAC_ADDR_MASKf, hw_mac);
    soc_mem_field32_set(unit, MPLS_STATION_TCAMm, &station_entry,
                        VLAN_ID_MASKf, hw_vlan);
    rv = WRITE_MPLS_STATION_TCAMm(unit, MEM_BLOCK_ANY,
                                  free_index, &station_entry);
    if (rv < 0) {
        COMMON_UNLOCK(unit);
        return rv;
    }
    MPLS_STATION_HASH(unit, free_index) = hash;
    COMMON_UNLOCK(unit);
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_l2_tunnel_delete
 * Purpose:
 *    Delete a (MAC, VLAN) for tunnel/MPLS processing.
 *    Shared by MPLS and MIM
 * Parameters:
 *      unit - Unit number
 *      mac  - MAC address
 *      vlan - VLAN ID
 */
int
bcm_trx_metro_l2_tunnel_delete(int unit, bcm_mac_t mac, bcm_vlan_t vlan)
{
    mpls_station_tcam_entry_t station_entry;
    uint16 hash;
    bcm_mac_t hw_mac;
    bcm_vlan_t hw_vlan;
    int rv, i, num_station;

    /* Get the hash value for this key */
    BCM_IF_ERROR_RETURN(_bcm_trx_mpls_station_hash_calc(unit, mac, vlan, &hash));

    /* find the entry to be deleted */
    num_station = soc_mem_index_count(unit, MPLS_STATION_TCAMm);
    COMMON_LOCK(unit);
    for (i = 0; i < num_station; i++) {
        if (hash == MPLS_STATION_HASH(unit, i)) {
            /* Read HW entry to see if it's really a match */
            rv = READ_MPLS_STATION_TCAMm(unit, MEM_BLOCK_ANY,
                                         i, &station_entry);
            if (rv < 0) {
                COMMON_UNLOCK(unit);
                return rv;
            }
            soc_mem_mac_addr_get(unit, MPLS_STATION_TCAMm,
                                 &station_entry, MAC_ADDRf, hw_mac);
            hw_vlan = soc_mem_field32_get(unit, MPLS_STATION_TCAMm,
                                          &station_entry, VLAN_IDf);
            if ((vlan == hw_vlan) &&
                !sal_memcmp(mac, hw_mac, sizeof(bcm_mac_t))) {
                break;
            }
        }
    }
    if (i == num_station) {
        /* Entry not found */
        COMMON_UNLOCK(unit);
        return BCM_E_NOT_FOUND;
    }
    /* Clear the entry in HW */
    sal_memset(&station_entry, 0, sizeof(mpls_station_tcam_entry_t));
    rv = WRITE_MPLS_STATION_TCAMm(unit, MEM_BLOCK_ANY, i, &station_entry);
    if (rv < 0) {
        COMMON_UNLOCK(unit);
        return rv;
    }
    COMMON_UNLOCK(unit);
    MPLS_STATION_HASH(unit, i) = 0;
    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_l2_tunnel_delete_all
 * Purpose:
 *     Delete all (MAC, VLAN) for tunnel/MPLS processing.
 *    Shared by MPLS and MIM
 * Parameters:
 *      unit - Unit number
 */
int
bcm_trx_metro_l2_tunnel_delete_all(int unit)
{
    int rv, num_station;

    num_station = soc_mem_index_count(unit, MPLS_STATION_TCAMm);
    COMMON_LOCK(unit);
    rv = soc_mem_clear(unit, MPLS_STATION_TCAMm, COPYNO_ALL, 0);
    if (rv < 0) {
        COMMON_UNLOCK(unit);
        return rv;
    }
    sal_memset(COMMON_INFO(unit)->mpls_station_hash, 0,
               sizeof(uint16) * num_station);
    COMMON_UNLOCK(unit);
    return BCM_E_NONE;
}

#if defined(BCM_TRIUMPH2_SUPPORT)
/*
 * Flexible stats:
 * 
 * Ingress/egress packet/byte counters which may be assigned from a pool
 * to various packet categories.  There are two pools available:
 * Service interface types:  VLAN, VRF, VFI
 * Virtual interface types:  Some GPORT categories
 * The source table responsible for the packet classification holds an
 * index to the counter pool.  Otherwise, the implementation of the two
 * types is equivalent, so it is abstracted here.
 * This is implemented here because so many modules are touched by
 * this feature.
 */



static _bcm_flex_stat_info_t *fs_info[BCM_MAX_NUM_UNITS][_bcmFlexStatTypeAllHwNum];
static int _tr2_flex_stat_api_ver[BCM_MAX_NUM_UNITS] = {0};
static sal_mutex_t _flex_stat_mutex[BCM_MAX_NUM_UNITS] = {NULL};
static uint64 *_bcm_flex_stat_mem[BCM_MAX_NUM_UNITS][_bcmFlexStatTypeHwNum]
                                 [_BCM_FLEX_STAT_DIRS][_BCM_FLEX_STAT_GRANS];
static ing_service_counter_table_entry_t *_bcm_flex_stat_buff
                                 [BCM_MAX_NUM_UNITS][_bcmFlexStatTypeHwNum]
                                 [_BCM_FLEX_STAT_DIRS][_BCM_FLEX_STAT_BUFFS];
#ifdef BCM_TRIDENT_SUPPORT
static ing_service_counter_table_entry_t *_bcm_flex_stat_buff_y
                                 [BCM_MAX_NUM_UNITS][_bcmFlexStatTypeHwNum]
                                 [_BCM_FLEX_STAT_DIRS][_BCM_FLEX_STAT_BUFFS];
#endif /* BCM_TRIDENT_SUPPORT */
static uint32 _bcm_flex_stat_mem_types[] = {
    ING_SERVICE_COUNTER_TABLEm,
    EGR_SERVICE_COUNTER_TABLEm,
    ING_VINTF_COUNTER_TABLEm,
    EGR_VINTF_COUNTER_TABLEm
};
static uint8 _bcm_flex_stat_buff_toggle = 0;

static soc_memacc_t *_bcm_flex_stat_memacc[BCM_MAX_NUM_UNITS];

/* The gport, fp, MPLS label and both vxlt types are currently
 * implemented in the same HW. Here we collapse them into the same
 * info array for tracking.
 * We need the different types to distinguish the different handles.
 */
#define FS_TYPE_TO_HW_TYPE(unit, type) \
        (((type == _bcmFlexStatTypeService) || \
          (type == _bcmFlexStatTypeVrf))? _bcmFlexStatTypeService : \
	 (type == _bcmFlexStatTypeEgressService)? \
          _bcmFlexStatTypeEgressService: \
         (type == _bcmFlexStatTypeEgressGport || \
          type == _bcmFlexStatTypeEgrVxlt)? \
           _bcmFlexStatTypeEgressGport: _bcmFlexStatTypeGport)

#define FS_TYPE_TO_SVC_VINTF_HW_TYPE(unit, type) \
        (((type == _bcmFlexStatTypeService) || \
          (type == _bcmFlexStatTypeVrf) || \
	  (type == _bcmFlexStatTypeEgressService))? \
	  _bcmFlexStatTypeService : \
         _bcmFlexStatTypeGport)

#define FS_INFO(unit, hw_type) \
            fs_info[unit][hw_type]

#define FS_MEM_SIZE(unit, hw_type)     FS_INFO(unit, hw_type)->stat_mem_size
#define FS_USED_BITMAP(unit, hw_type)  FS_INFO(unit, hw_type)->stats
#define FS_HANDLE_LIST_SIZE(unit, hw_type)  \
                            FS_INFO(unit, hw_type)->handle_list_size
#define FS_HANDLE_LIST(unit, hw_type)  FS_INFO(unit, hw_type)->handle_list
#define FS_HANDLE_LIST_ELEMENT_PTR(unit, hw_type, list_index) \
            (&(FS_INFO(unit, hw_type)->handle_list[list_index]))
#define FS_HANDLE_LIST_ELEMENT_HANDLE(unit, hw_type, list_index) \
            ((FS_INFO(unit, hw_type)->handle_list[list_index].handle))
#define FS_HANDLE_LIST_ELEMENT_TYPE(unit, hw_type, list_index) \
            ((FS_INFO(unit, hw_type)->handle_list[list_index].type))
#define FS_HANDLE_LIST_ELEMENT_INDEX(unit, hw_type, list_index) \
            ((FS_INFO(unit, hw_type)->handle_list[list_index].index))
#define FS_HANDLE_LIST_ELEMENT_SIZE \
                                  sizeof(_bcm_flex_stat_handle_to_index_t)
#define FS_REF_CNT_ELEMENT(_u, _htype, _inx) \
            (FS_INFO((_u), (_htype))->ref_cnt[(_inx)])

/* Below, the macros work with the FS type */
/* type takes ingress hardware types */
#define FS_ING_MEM(type)         \
        ((FS_TYPE_TO_HW_TYPE(unit, type) == _bcmFlexStatTypeService) ? \
                ING_SERVICE_COUNTER_TABLEm : ING_VINTF_COUNTER_TABLEm)
/* type takes egress hardware types */
#define FS_EGR_MEM(type)         \
        (((FS_TYPE_TO_HW_TYPE(unit, type) == \
                        _bcmFlexStatTypeEgressService) || \
         (FS_TYPE_TO_HW_TYPE(unit, type) == _bcmFlexStatTypeService)) ? \
                EGR_SERVICE_COUNTER_TABLEm : EGR_VINTF_COUNTER_TABLEm)

/* 
 * Common resource lock
 */
#define FS_LOCK(unit) \
        sal_mutex_take(_flex_stat_mutex[unit], sal_mutex_FOREVER);

#define FS_UNLOCK(unit) \
        sal_mutex_give(_flex_stat_mutex[unit]); 

#define FS_INIT(unit, type) \
        if (FS_INFO(unit, FS_TYPE_TO_HW_TYPE(unit, type)) == NULL) { \
            FS_UNLOCK(unit); \
            return BCM_E_INIT; \
        }

#define TR2_FLEX_STAT_API_LEGACY    1
#define TR2_FLEX_STAT_API_NEW       2

/* Allocate Flex counter table SW caches and fetch buffers */
STATIC int
_bcm_esw_flex_stat_mem_init(int unit)
{
    int i /* types */, j /* dir */, k /* grans/buffs */;
    int alloc_size, rv = BCM_E_NONE; 
    soc_mem_t mem;
    for (i = 0; i < _bcmFlexStatTypeHwNum; i++) {
        for (j = 0; j < _BCM_FLEX_STAT_DIRS; j++) {
            mem = _bcm_flex_stat_mem_types[(i * _BCM_FLEX_STAT_DIRS) + j];
            for (k = 0; k < _BCM_FLEX_STAT_GRANS; k++) {
                alloc_size = SOC_MEM_SIZE(unit, mem) * sizeof(uint64);
                _bcm_flex_stat_mem[unit][i][j][k] = 
                    sal_alloc(alloc_size, "Flexible counters cache");
                if (NULL == _bcm_flex_stat_mem[unit][i][j][k]) {
                    soc_cm_print("bcm_esw_flex_stat_mem_init: Mem alloc failed - "
                                 "unit: %d, type: %d, dir: %d, gran: %d\n",
                                  unit, i, j, k);
                    return BCM_E_MEMORY;
                }
                sal_memset(_bcm_flex_stat_mem[unit][i][j][k], 0, alloc_size);
            }
        }
    }
    for (i = 0; i < _bcmFlexStatTypeHwNum; i++) {
        for (j = 0; j < _BCM_FLEX_STAT_DIRS; j++) {
            mem = _bcm_flex_stat_mem_types[(i * _BCM_FLEX_STAT_DIRS) + j];
            alloc_size = SOC_MEM_TABLE_BYTES(unit, mem);
            for (k = 0; k < _BCM_FLEX_STAT_BUFFS; k++) {
                _bcm_flex_stat_buff[unit][i][j][k] = \
                    soc_cm_salloc(unit, alloc_size, "Flexible counters buff");
                if (NULL == _bcm_flex_stat_buff[unit][i][j][k]) {
                    soc_cm_print("bcm_esw_flex_stat_mem_init: Mem alloc failed - "
                                 "unit: %d, type: %d, dir: %d, buff: %d\n",
                                  unit, i, j, k);
                    soc_cm_print("Mem alloc failed.\n");
                    return BCM_E_MEMORY;
                }
                sal_memset(_bcm_flex_stat_buff[unit][i][j][k], 0, alloc_size);
#ifdef BCM_TRIDENT_SUPPORT
                if (SOC_IS_TD_TT(unit)) {
                    _bcm_flex_stat_buff_y[unit][i][j][k] = \
                        soc_cm_salloc(unit, alloc_size, 
                                      "Flexible counters buff_y");
                    if (NULL == _bcm_flex_stat_buff_y[unit][i][j][k]) {
                            return BCM_E_MEMORY;
                    }
                    sal_memset(_bcm_flex_stat_buff_y[unit][i][j][k], 0, 
                               alloc_size);
                }
#endif /* BCM_TRIDENT_SUPPORT */
            }
        } 
    }
    return rv;
}

/* Free Flex counter table SW caches and fetch buffers */
STATIC void
_bcm_esw_flex_stat_mem_free(int unit)
{
    int i /* types */, j /* dir */, k /* grans/buffs */;
    for (i = 0; i < _bcmFlexStatTypeHwNum; i++) {
        for (j = 0; j < _BCM_FLEX_STAT_DIRS; j++) {
            for (k = 0; k < _BCM_FLEX_STAT_GRANS; k++) {
                sal_free(_bcm_flex_stat_mem[unit][i][j][k]);
            }
        }
    }
    for (i = 0; i < _bcmFlexStatTypeHwNum; i++) {
        for (j = 0; j < _BCM_FLEX_STAT_DIRS; j++) {
            for (k = 0; k < _BCM_FLEX_STAT_BUFFS; k++) {
                soc_cm_sfree(unit, _bcm_flex_stat_buff[unit][i][j][k]);
#ifdef BCM_TRIDENT_SUPPORT
                if (SOC_IS_TD_TT(unit)) {
                    soc_cm_sfree(unit, _bcm_flex_stat_buff_y[unit][i][j][k]);
                }
#endif /* BCM_TRIDENT_SUPPORT */
            }
        }
    }
}

typedef struct _bcm_fs_memacc_map_s {
    soc_mem_t mem;
    soc_field_t fld;
    int type_index;
} _bcm_fs_memacc_map_t;

static _bcm_fs_memacc_map_t _bcm_tr2_fs_memacc_map[] = {
    {ING_SERVICE_COUNTER_TABLEm, PACKET_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_ING_PKT},
    {ING_SERVICE_COUNTER_TABLEm, BYTE_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_ING_BYTE},
    {EGR_SERVICE_COUNTER_TABLEm, PACKET_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_EGR_PKT},
    {EGR_SERVICE_COUNTER_TABLEm, BYTE_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_EGR_BYTE},
    {ING_VINTF_COUNTER_TABLEm, PACKET_COUNTERf,
     _BCM_FS_MEMACC_VINTF_ING_PKT},
    {ING_VINTF_COUNTER_TABLEm, BYTE_COUNTERf,
     _BCM_FS_MEMACC_VINTF_ING_BYTE},
    {EGR_VINTF_COUNTER_TABLEm, PACKET_COUNTERf,
     _BCM_FS_MEMACC_VINTF_EGR_PKT},
    {EGR_VINTF_COUNTER_TABLEm, BYTE_COUNTERf,
     _BCM_FS_MEMACC_VINTF_EGR_BYTE},
};

#ifdef BCM_TRIDENT_SUPPORT
static _bcm_fs_memacc_map_t _bcm_td_fs_memacc_map[] = {
    {ING_SERVICE_COUNTER_TABLE_Xm, PACKET_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_ING_PKT},
    {ING_SERVICE_COUNTER_TABLE_Xm, BYTE_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_ING_BYTE},
    {EGR_SERVICE_COUNTER_TABLE_Xm, PACKET_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_EGR_PKT},
    {EGR_SERVICE_COUNTER_TABLE_Xm, BYTE_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_EGR_BYTE},
    {ING_VINTF_COUNTER_TABLE_Xm, PACKET_COUNTERf,
     _BCM_FS_MEMACC_VINTF_ING_PKT},
    {ING_VINTF_COUNTER_TABLE_Xm, BYTE_COUNTERf,
     _BCM_FS_MEMACC_VINTF_ING_BYTE},
    {EGR_VINTF_COUNTER_TABLE_Xm, PACKET_COUNTERf,
     _BCM_FS_MEMACC_VINTF_EGR_PKT},
    {EGR_VINTF_COUNTER_TABLE_Xm, BYTE_COUNTERf,
     _BCM_FS_MEMACC_VINTF_EGR_BYTE},

    {ING_SERVICE_COUNTER_TABLE_Ym, PACKET_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_ING_PKT + _BCM_FS_MEMACC_NUM},
    {ING_SERVICE_COUNTER_TABLE_Ym, BYTE_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_ING_BYTE + _BCM_FS_MEMACC_NUM},
    {EGR_SERVICE_COUNTER_TABLE_Ym, PACKET_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_EGR_PKT + _BCM_FS_MEMACC_NUM},
    {EGR_SERVICE_COUNTER_TABLE_Ym, BYTE_COUNTERf,
     _BCM_FS_MEMACC_SERVICE_EGR_BYTE + _BCM_FS_MEMACC_NUM},
    {ING_VINTF_COUNTER_TABLE_Ym, PACKET_COUNTERf,
     _BCM_FS_MEMACC_VINTF_ING_PKT + _BCM_FS_MEMACC_NUM},
    {ING_VINTF_COUNTER_TABLE_Ym, BYTE_COUNTERf,
     _BCM_FS_MEMACC_VINTF_ING_BYTE + _BCM_FS_MEMACC_NUM},
    {EGR_VINTF_COUNTER_TABLE_Ym, PACKET_COUNTERf,
     _BCM_FS_MEMACC_VINTF_EGR_PKT + _BCM_FS_MEMACC_NUM},
    {EGR_VINTF_COUNTER_TABLE_Ym, BYTE_COUNTERf,
     _BCM_FS_MEMACC_VINTF_EGR_BYTE + _BCM_FS_MEMACC_NUM},
};
#endif /*  BCM_TRIDENT_SUPPORT */

STATIC int
_bcm_esw_flex_stat_memacc_init(int unit)
{
    int alloc_size, rv = BCM_E_NONE;
    _bcm_fs_memacc_map_t *memacc_map = _bcm_tr2_fs_memacc_map;
    int mam_ix, memacc_map_size = _BCM_FS_MEMACC_NUM;

    alloc_size = (_BCM_FS_MEMACC_NUM * sizeof(soc_memacc_t));

#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        /* X & Y pipes */
        alloc_size *= 2;
        memacc_map = _bcm_td_fs_memacc_map;
        memacc_map_size = 2 * _BCM_FS_MEMACC_NUM;
    }
#endif /* BCM_TRIDENT_SUPPORT */

    _bcm_flex_stat_memacc[unit] =
        sal_alloc(alloc_size, "Flexible counters memacc data");
    if (NULL == _bcm_flex_stat_memacc[unit]) {
        rv = BCM_E_MEMORY;
    }

    for (mam_ix = 0; mam_ix < memacc_map_size; mam_ix++) {
        if (BCM_SUCCESS(rv)) {
            rv = soc_memacc_init(unit, memacc_map[mam_ix].mem,
                                 memacc_map[mam_ix].fld,
                                 &(_bcm_flex_stat_memacc[unit]
                                   [memacc_map[mam_ix].type_index]));
        }
        if (BCM_SUCCESS(rv) &&
            (0 == SOC_MEMACC_FIELD_LENGTH(&(_bcm_flex_stat_memacc[unit]
                                         [memacc_map[mam_ix].type_index])))) {
            soc_cm_debug(DK_VERBOSE, "bcm_esw_flex_stat_memacc_init: " 
                         "Invalid memacc field info.\n");
            rv = BCM_E_INTERNAL;
        }
    }
    return rv;
}

/* Callback to be invoked from counter thread */
void
_bcm_esw_flex_stat_callback(int unit)
{
    int i /* types */, j /* dir */, k /* index */, max = 0;
    soc_mem_t mem;
    ing_service_counter_table_entry_t *curr_ptr = NULL;
    ing_service_counter_table_entry_t *prev_ptr = NULL;
    ing_service_counter_table_entry_t *curr_eptr = NULL;
    ing_service_counter_table_entry_t *prev_eptr = NULL;
    uint64 *prev_pktptr, *prev_bytptr;
    int rv = BCM_E_NONE;
    soc_memacc_t *memacc_pkt = NULL;
    soc_memacc_t *memacc_byte = NULL;
    int field_length_byte = 0;
    int field_length_packet = 0;
    uint64 curr, prev, diff, size; 
    SHR_BITDCL *stat_bmp;
#ifdef BCM_TRIDENT_SUPPORT
    ing_service_counter_table_entry_t *curr_ptr_y = NULL; 
    ing_service_counter_table_entry_t *prev_ptr_y = NULL;
    ing_service_counter_table_entry_t *curr_eptr_y = NULL;
    ing_service_counter_table_entry_t *prev_eptr_y = NULL;
    soc_mem_t mem_x = 0, mem_y = 0;
    soc_memacc_t *memacc_pkt_x = NULL;
    soc_memacc_t *memacc_byte_x = NULL;
    soc_memacc_t *memacc_pkt_y = NULL;
    soc_memacc_t *memacc_byte_y = NULL;
    int field_length_byte_x = 0;
    int field_length_packet_x = 0;
    int field_length_byte_y = 0;
    int field_length_packet_y = 0;
    uint64 diff_y;
#endif /* BCM_TRIDENT_SUPPORT */

    /* For each table fetch stats from h/w */
    for (i = 0; i < _bcmFlexStatTypeHwNum; i++) {
        for (j = 0; j < _BCM_FLEX_STAT_DIRS; j++) {
            /* Take and release lock to permit other threads'
             * activity in between table scans. */
            FS_LOCK(unit);
            if (_tr2_flex_stat_api_ver[unit] == TR2_FLEX_STAT_API_NEW) {
                stat_bmp = FS_USED_BITMAP(unit,i+(_bcmFlexStatTypeHwNum * j));
            } else {
                stat_bmp = FS_USED_BITMAP(unit, i);
            }
            mem = _bcm_flex_stat_mem_types[(i * _BCM_FLEX_STAT_DIRS) + j];
            curr_ptr = _bcm_flex_stat_buff[unit][i][j]
                                          [_bcm_flex_stat_buff_toggle];
            if (NULL == curr_ptr) {
                soc_cm_debug(DK_VERBOSE, "bcm_esw_flex_stat_callback: " 
                             "Bad mem ptr - unit: %d, type: %d, dir: %d\n",
                             unit, i, j);
                /* we are in a callback; no point continuing */
                FS_UNLOCK(unit);
                return;
            }
#ifdef BCM_TRIDENT_SUPPORT
            if (SOC_IS_TD_TT(unit)) {
                curr_ptr_y = _bcm_flex_stat_buff_y[unit][i][j]
                                 [_bcm_flex_stat_buff_toggle];
                if (NULL == curr_ptr_y) {
                    soc_cm_debug(DK_VERBOSE, "bcm_esw_flex_stat_callback: " 
                                 "Bad mem ptr - unit: %d, type: %d, dir: %d\n",
                                 unit, i, j);
                    /* we are in a callback; no point continuing */
                    FS_UNLOCK(unit);
                    return;
                }
                switch (mem) {
                case ING_SERVICE_COUNTER_TABLEm:
                    mem_x = ING_SERVICE_COUNTER_TABLE_Xm;
                    memacc_pkt_x = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_ING_PKT]);
                    memacc_byte_x = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_ING_BYTE]);
                    mem_y = ING_SERVICE_COUNTER_TABLE_Ym;
                    memacc_pkt_y = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_ING_PKT +
                         _BCM_FS_MEMACC_NUM]);
                    memacc_byte_y = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_ING_BYTE +
                         _BCM_FS_MEMACC_NUM]);
                    break;
                case EGR_SERVICE_COUNTER_TABLEm:
                    mem_x = EGR_SERVICE_COUNTER_TABLE_Xm; 
                    memacc_pkt_x = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_EGR_PKT]);
                    memacc_byte_x = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_EGR_BYTE]);
                    mem_y = EGR_SERVICE_COUNTER_TABLE_Ym; 
                    memacc_pkt_y = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_EGR_PKT +
                         _BCM_FS_MEMACC_NUM]);
                    memacc_byte_y = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_EGR_BYTE +
                         _BCM_FS_MEMACC_NUM]);
                    break;
                case ING_VINTF_COUNTER_TABLEm:
                    mem_x = ING_VINTF_COUNTER_TABLE_Xm; 
                    memacc_pkt_x = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_ING_PKT]);
                    memacc_byte_x = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_ING_BYTE]);
                    mem_y = ING_VINTF_COUNTER_TABLE_Ym; 
                    memacc_pkt_y = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_ING_PKT +
                         _BCM_FS_MEMACC_NUM]);
                    memacc_byte_y = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_ING_BYTE +
                         _BCM_FS_MEMACC_NUM]);
                    break;
                case EGR_VINTF_COUNTER_TABLEm:
                    mem_x = EGR_VINTF_COUNTER_TABLE_Xm; 
                    memacc_pkt_x = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_EGR_PKT]);
                    memacc_byte_x = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_EGR_BYTE]);
                    mem_y = EGR_VINTF_COUNTER_TABLE_Ym; 
                    memacc_pkt_y = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_EGR_PKT +
                         _BCM_FS_MEMACC_NUM]);
                    memacc_byte_y = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_EGR_BYTE +
                         _BCM_FS_MEMACC_NUM]);
                    break;
                default:
                    soc_cm_debug(DK_VERBOSE, "bcm_esw_flex_stat_callback: " 
                                 "Bad mem type (%d) - unit: %d, "
                                 "type: %d, dir: %d\n", mem, unit, i, j);
                    /* we are in a callback; no point continuing */
                    FS_UNLOCK(unit);
                    return;
                }
                rv = soc_mem_read_range(unit, mem_x, MEM_BLOCK_ANY, 
                                        soc_mem_index_min(unit, mem_x), 
                                        soc_mem_index_max(unit, mem_x), 
                                        curr_ptr);
                if (rv < 0) {
                    soc_cm_debug(DK_VERBOSE, "bcm_esw_flex_stat_callback: " 
                                 "Mem read failed.\n");
                    /* we are in a callback; no point continuing */
                    FS_UNLOCK(unit);
                    return;
                } 
                rv = soc_mem_read_range(unit, mem_y, MEM_BLOCK_ANY, 
                                        soc_mem_index_min(unit, mem_y), 
                                        soc_mem_index_max(unit, mem_y), 
                                        curr_ptr_y);
                if (rv < 0) {
                    soc_cm_debug(DK_VERBOSE, "bcm_esw_flex_stat_callback: " 
                                 "Mem read failed.\n");
                    /* we are in a callback; no point continuing */
                    FS_UNLOCK(unit);
                    return;
                } 
                max = soc_mem_index_max(unit, mem_x);

                field_length_packet_x =
                    SOC_MEMACC_FIELD_LENGTH(memacc_pkt_x);
                field_length_byte_x =
                    SOC_MEMACC_FIELD_LENGTH(memacc_byte_x);
                field_length_packet_y =
                    SOC_MEMACC_FIELD_LENGTH(memacc_pkt_y);
                field_length_byte_y =
                    SOC_MEMACC_FIELD_LENGTH(memacc_byte_y);
            } else 
#endif /* BCM_TRIDENT_SUPPORT */
            {
                switch (mem) {
                case ING_SERVICE_COUNTER_TABLEm:
                    memacc_pkt = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_ING_PKT]);
                    memacc_byte = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_ING_BYTE]);
                    break;
                case EGR_SERVICE_COUNTER_TABLEm:
                    memacc_pkt = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_EGR_PKT]);
                    memacc_byte = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_SERVICE_EGR_BYTE]);
                    break;
                case ING_VINTF_COUNTER_TABLEm:
                    memacc_pkt = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_ING_PKT]);
                    memacc_byte = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_ING_BYTE]);
                    break;
                case EGR_VINTF_COUNTER_TABLEm:
                    memacc_pkt = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_EGR_PKT]);
                    memacc_byte = &(_bcm_flex_stat_memacc[unit]
                        [_BCM_FS_MEMACC_VINTF_EGR_BYTE]);
                    break;
                default:
                    soc_cm_debug(DK_VERBOSE, "bcm_esw_flex_stat_callback: " 
                                 "Bad mem type (%d) - unit: %d, "
                                 "type: %d, dir: %d\n", mem, unit, i, j);
                    /* we are in a callback; no point continuing */
                    FS_UNLOCK(unit);
                    return;
                }

                rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ANY, 
                                        soc_mem_index_min(unit, mem), 
                                        soc_mem_index_max(unit, mem), 
                                        curr_ptr);
                if (rv < 0) {
                    soc_cm_debug(DK_VERBOSE, "bcm_esw_flex_stat_callback: " 
                                 "Mem read failed.\n");
                    /* we are in a callback; no point continuing */
                    FS_UNLOCK(unit);
                    return;
                } 

                max = soc_mem_index_max(unit, mem);
                field_length_packet = SOC_MEMACC_FIELD_LENGTH(memacc_pkt);
                field_length_byte = SOC_MEMACC_FIELD_LENGTH(memacc_byte);
            }

            /* Iterate through and check/get diffs */
            prev_ptr = _bcm_flex_stat_buff[unit][i][j]
                                          [!_bcm_flex_stat_buff_toggle];
#ifdef BCM_TRIDENT_SUPPORT
            if (SOC_IS_TD_TT(unit)) {
                prev_ptr_y = _bcm_flex_stat_buff_y[unit][i][j]
                                 [!_bcm_flex_stat_buff_toggle];
            }
#endif /* BCM_TRIDENT_SUPPORT */
            prev_pktptr = _bcm_flex_stat_mem[unit][i][j][0];
            prev_bytptr = _bcm_flex_stat_mem[unit][i][j][1];
            for (k = 0; k < max; k++) {
                if (!SHR_BITGET(stat_bmp, k)) {
                    /* Stat not enabled, don't bother to collect */
                    continue;
                }

                /* DMA memory entry arithmetic */ 
                curr_eptr = soc_mem_table_idx_to_pointer(unit,
                                        ING_SERVICE_COUNTER_TABLEm,
                                        ing_service_counter_table_entry_t *,
                                                         curr_ptr, k);
                prev_eptr = soc_mem_table_idx_to_pointer(unit,
                                        ING_SERVICE_COUNTER_TABLEm,
                                        ing_service_counter_table_entry_t *,
                                                         prev_ptr, k);
#ifdef BCM_TRIDENT_SUPPORT
                if (SOC_IS_TD_TT(unit)) {
                    curr_eptr_y = soc_mem_table_idx_to_pointer(unit,
                                        ING_SERVICE_COUNTER_TABLEm,
                                        ing_service_counter_table_entry_t *,
                                            curr_ptr_y, k);
                    prev_eptr_y = soc_mem_table_idx_to_pointer(unit,
                                        ING_SERVICE_COUNTER_TABLEm,
                                        ing_service_counter_table_entry_t *,
                                            prev_ptr_y, k);
                }
#endif /* BCM_TRIDENT_SUPPORT */

                /* first packets */
#ifdef BCM_TRIDENT_SUPPORT
                if (SOC_IS_TD_TT(unit)) {
                    soc_memacc_field64_get(memacc_pkt_x,
                                           (uint32 *)curr_eptr, &curr);
                    soc_memacc_field64_get(memacc_pkt_x,
                                           (uint32 *)prev_eptr, &prev);
                    /* Take care of rollovers */
                    if (COMPILER_64_LT(curr, prev)) {
                       COMPILER_64_SET(size, 0, 1);
                       COMPILER_64_SHL(size, field_length_packet_x);
                       COMPILER_64_ADD_64(curr, size); 
                    }
                    diff = curr;
                    COMPILER_64_SUB_64(diff, prev);

                    soc_memacc_field64_get(memacc_pkt_y,
                                           (uint32 *)curr_eptr_y, &curr);
                    soc_memacc_field64_get(memacc_pkt_y,
                                           (uint32 *)prev_eptr_y, &prev);
                    /* Take care of rollovers */
                    if (COMPILER_64_LT(curr, prev)) {
                       COMPILER_64_SET(size, 0, 1);
                       COMPILER_64_SHL(size, field_length_packet_y);
                       COMPILER_64_ADD_64(curr, size); 
                    }
                    diff_y = curr;         
                    COMPILER_64_SUB_64(diff_y, prev);
                    COMPILER_64_ADD_64(diff, diff_y);
                } else
#endif /* BCM_TRIDENT_SUPPORT */
                {
                    soc_memacc_field64_get(memacc_pkt,
                                           (uint32 *)curr_eptr, &curr);
                    soc_memacc_field64_get(memacc_pkt,
                                           (uint32 *)prev_eptr, &prev);
                    /* Take care of rollovers */
                    if (COMPILER_64_LT(curr, prev)) {
                       COMPILER_64_SET(size, 0, 1);
                       COMPILER_64_SHL(size, field_length_packet);
                       COMPILER_64_ADD_64(curr, size); 
                    }
                    diff = curr;                    
                    COMPILER_64_SUB_64(diff, prev);
                }
                if (!COMPILER_64_IS_ZERO(diff)) {
                    /* Update soft copy */
                    COMPILER_64_ADD_64(prev_pktptr[k], diff);
                }

                /* then bytes */
#ifdef BCM_TRIDENT_SUPPORT
                if (SOC_IS_TD_TT(unit)) {
                    soc_memacc_field64_get(memacc_byte_x,
                                           (uint32 *)curr_eptr,
                                           &curr);
                    soc_memacc_field64_get(memacc_byte_x,
                                           (uint32 *)prev_eptr,
                                           &prev);
                    /* Take care of rollovers */
                    if (COMPILER_64_LT(curr, prev)) {
                       COMPILER_64_SET(size, 0, 1);
                       COMPILER_64_SHL(size, field_length_byte_x);
                       COMPILER_64_ADD_64(curr, size); 
                    }
                    diff = curr;
                    COMPILER_64_SUB_64(diff, prev);

                    soc_memacc_field64_get(memacc_byte_y,
                                           (uint32 *)curr_eptr_y,
                                           &curr);
                    soc_memacc_field64_get(memacc_byte_y,
                                           (uint32 *)prev_eptr_y,
                                           &prev);
                    /* Take care of rollovers */
                    if (COMPILER_64_LT(curr, prev)) {
                       COMPILER_64_SET(size, 0, 1);
                       COMPILER_64_SHL(size, field_length_byte_y);
                       COMPILER_64_ADD_64(curr, size); 
                    }
                    diff_y = curr;
                    COMPILER_64_SUB_64(diff_y, prev);
                    COMPILER_64_ADD_64(diff, diff_y);
                } else
#endif /* BCM_TRIDENT_SUPPORT */
                {
                    soc_memacc_field64_get(memacc_byte,
                                           (uint32 *)curr_eptr,
                                           &curr);
                    soc_memacc_field64_get(memacc_byte,
                                           (uint32 *)prev_eptr,
                                           &prev);
                    /* Take care of rollovers */
                    if (COMPILER_64_LT(curr, prev)) {
                       COMPILER_64_SET(size, 0, 1);
                       COMPILER_64_SHL(size, field_length_byte);
                       COMPILER_64_ADD_64(curr, size); 
                    }
                    diff = curr;                    
                    COMPILER_64_SUB_64(diff, prev);
                }
                if (!COMPILER_64_IS_ZERO(diff)) {
                    /* Update soft copy */
                    COMPILER_64_ADD_64(prev_bytptr[k], diff);
                }
            }
            FS_UNLOCK(unit);
        }
    }

    /* Flip toggle with lock */
    FS_LOCK(unit);
    _bcm_flex_stat_buff_toggle = !_bcm_flex_stat_buff_toggle;
    FS_UNLOCK(unit);
}

/*
 * Function:
 *      _bcm_esw_flex_stat_detach
 * Purpose:
 *      De-allocate memory used by flexible counters.
 * Parameters:
 *      unit - SOC unit number
 *      fs_type - flexible stat type
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_flex_stat_detach(int unit, _bcm_flex_stat_type_t fs_type)
{
    _bcm_flex_stat_info_t  *stat_info;
    _bcm_flex_stat_type_t type_idx,
        hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);
    int mutex = TRUE;

    if (fs_type >= _bcmFlexStatTypeAllHwNum) {
        /* We shouldn't be deallocating this */
        return BCM_E_INTERNAL;
    }

    if (NULL != _flex_stat_mutex[unit]) {
        FS_LOCK(unit);
    }

    stat_info = FS_INFO(unit, hw_type);

    if (stat_info != NULL) {
        if (stat_info->stats != NULL) {
            sal_free(stat_info->stats);
        }
        if (stat_info->handle_list != NULL) {
            sal_free(stat_info->handle_list);
        }
        if (stat_info->ref_cnt != NULL) {
            sal_free(stat_info->ref_cnt);
        }
        sal_free(stat_info);
        FS_INFO(unit, hw_type) = NULL;
    }

    if (NULL != _flex_stat_mutex[unit]) {
        FS_UNLOCK(unit);
        /* Check if all counter types are detached */
        for (type_idx = _bcmFlexStatTypeService;
             type_idx < _bcmFlexStatTypeAllHwNum; type_idx ++) {
            if (FS_INFO(unit, type_idx) != NULL) {
                mutex = FALSE;
                break;
            }
        }
        /* If so, unregister, free the memory and destroy the mutex also */
        if (mutex) {
            soc_counter_extra_unregister(unit, _bcm_esw_flex_stat_callback);
            _bcm_esw_flex_stat_mem_free(unit);
            if (NULL != _bcm_flex_stat_memacc[unit]) {
                sal_free(_bcm_flex_stat_memacc[unit]);
                _bcm_flex_stat_memacc[unit] = NULL;
            }
            sal_mutex_destroy(_flex_stat_mutex[unit]);
            _flex_stat_mutex[unit] = NULL;
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_init
 * Purpose:
 *      Allocate memory used by flexible counters.
 * Parameters:
 *      unit - SOC unit number
 *      fs_type - flexible stat type
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_flex_stat_init(int unit, _bcm_flex_stat_type_t fs_type)
{
    _bcm_flex_stat_info_t  *stat_info = NULL;
    int alloc_size, rv = BCM_E_NONE;
    soc_mem_t mem;
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);

    if (fs_type >= _bcmFlexStatTypeAllHwNum) {
        /* We shouldn't be allocating this */
        return BCM_E_INTERNAL;
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_flex_stat_detach(unit, fs_type));

    /* Without the lock and memory, we can't do anything */
    if (_flex_stat_mutex[unit] == NULL) {
        _flex_stat_mutex[unit] = sal_mutex_create("common mutex");
        if (_flex_stat_mutex[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        /* Allocate cache and buffer memory */
        rv = _bcm_esw_flex_stat_mem_init(unit);
        if (BCM_SUCCESS(rv)) {
            /* Predetermined memory access accelerators */
            rv = _bcm_esw_flex_stat_memacc_init(unit);
        }
        if (BCM_SUCCESS(rv)) {
            /* Register for counter callback */
            soc_counter_extra_register(unit, _bcm_esw_flex_stat_callback);
        }
    }

    FS_LOCK(unit);

    if (BCM_SUCCESS(rv)) {
        alloc_size = sizeof(_bcm_flex_stat_info_t);
        stat_info = sal_alloc(alloc_size, "Flexible counters info");
        if (NULL == stat_info) {
            rv = BCM_E_MEMORY;
        } else {
            sal_memset(stat_info, 0, alloc_size);
            mem = FS_ING_MEM(fs_type);
            stat_info->stat_mem_size = soc_mem_index_count(unit, mem);
        }
    }

    if (BCM_SUCCESS(rv)) {
        alloc_size = SHR_BITALLOCSIZE(stat_info->stat_mem_size);
        stat_info->stats =
            sal_alloc(alloc_size, "Flexible counters usage bitmap");
        if (NULL == stat_info->stats) {
            rv = BCM_E_MEMORY;
        } else {
            sal_memset(stat_info->stats, 0, alloc_size);
            SHR_BITSET(stat_info->stats, 0); /* Index 0 is invalid */
        }
    }

    if (BCM_SUCCESS(rv)) {
        alloc_size = (FS_HANDLE_LIST_ELEMENT_SIZE *
                      stat_info->stat_mem_size);
        stat_info->handle_list =
            sal_alloc(alloc_size, "Flexible counters handle list");
        if (NULL == stat_info->handle_list) {
            rv = BCM_E_MEMORY;
        } else {
            sal_memset(stat_info->handle_list, 0, alloc_size);
            stat_info->handle_list_size = 0;
        }
    }

    if (BCM_SUCCESS(rv)) {
        alloc_size = (sizeof(uint16) *
                      stat_info->stat_mem_size);
        stat_info->ref_cnt =
            sal_alloc(alloc_size, "Flexible counters reference table");
        if (NULL == stat_info->ref_cnt) {
            rv = BCM_E_MEMORY;
        } else {
            sal_memset(stat_info->ref_cnt, 0, alloc_size);
        }
    }

    if (BCM_SUCCESS(rv)) {
        FS_INFO(unit, hw_type) = stat_info;
    } else {
        /* Clean up */
        (void)_bcm_esw_flex_stat_detach(unit, fs_type);
        /* We already have an error in rv */
    }

    FS_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      _bcm_flex_stat_handle_to_index_t_compare
 * Purpose:
 *      Compare two _bcm_flex_stat_handle_to_index_t elements for 
 *      binary table sort.
 * Parameters:
 *      a - (IN) pointer to first _bcm_flex_stat_handle_to_index_t
 *      b - (IN) pointer to second _bcm_flex_stat_handle_to_index_t
 * Returns:
 *      a<=>b
 */
STATIC int
_bcm_flex_stat_handle_to_index_t_compare(void *a, void *b)
{
    _bcm_flex_stat_handle_to_index_t *first, *second;
    int word;

    first = (_bcm_flex_stat_handle_to_index_t *)a;
    second = (_bcm_flex_stat_handle_to_index_t *)b;

    if (first->type < second->type) {
        return (-1);
    } else if (first->type > second->type) {
        return (1);
    }

    /* Types equal, check handle */
    for (word = (_BCM_FLEX_STAT_HANDLE_WORDS - 1); word >= 0; word--) {
        if (_BCM_FLEX_STAT_HANDLE_WORD_GET(first->handle, word) <
            _BCM_FLEX_STAT_HANDLE_WORD_GET(second->handle, word)) {
            return (-1);
        } else if (_BCM_FLEX_STAT_HANDLE_WORD_GET(first->handle, word) >
                   _BCM_FLEX_STAT_HANDLE_WORD_GET(second->handle, word)) {
            return (1);
        }
    }

    return (0);
}

/*
 * Function:
 *      _bcm_esw_flex_stat_index
 * Purpose:
 *      Retrieve the flexible stat index for the given handle and stat type.
 * Parameters:
 *      unit - SOC unit number
 *      fs_type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 * Returns:
 *      Flexible stat index
 */
STATIC int
_bcm_esw_flex_stat_index(int unit, _bcm_flex_stat_type_t fs_type,
                         _bcm_flex_stat_handle_t handle)
{
    int list_index;
    _bcm_flex_stat_handle_to_index_t target;
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);

    _BCM_FLEX_STAT_HANDLE_ASSIGN(target.handle, handle);
    target.type = fs_type;
    list_index = _shr_bsearch(FS_HANDLE_LIST(unit, hw_type),
                              FS_HANDLE_LIST_SIZE(unit, hw_type),
                              FS_HANDLE_LIST_ELEMENT_SIZE, &target,
                              &_bcm_flex_stat_handle_to_index_t_compare);

    if ((list_index >= 0) &&
        (list_index < FS_HANDLE_LIST_SIZE(unit, hw_type))) {
        return FS_HANDLE_LIST_ELEMENT_INDEX(unit, hw_type, list_index);
    }
    return 0;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_handle_list_insert
 * Purpose:
 *      Add the flexible stat index for the given handle and stat type
 *      to the handle list.
 * Parameters:
 *      unit - SOC unit number
 *      fs_type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      index - flexible stat index
 * Returns:
 *      Flexible stat index, or 0 on resource failure
 */
STATIC int
_bcm_esw_flex_stat_handle_list_insert(int unit, _bcm_flex_stat_type_t fs_type,
                                _bcm_flex_stat_handle_t handle, uint32 index)
{
    int list_index, idx, tmp;
    _bcm_flex_stat_handle_to_index_t target;
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);

    if (FS_HANDLE_LIST_SIZE(unit, hw_type) >= FS_MEM_SIZE(unit, hw_type)) {
        
        return 0;
    }

    _BCM_FLEX_STAT_HANDLE_ASSIGN(target.handle, handle);
    target.type = fs_type;
    list_index = _shr_bsearch(FS_HANDLE_LIST(unit, hw_type),
                              FS_HANDLE_LIST_SIZE(unit, hw_type),
                              FS_HANDLE_LIST_ELEMENT_SIZE, &target,
                              &_bcm_flex_stat_handle_to_index_t_compare);


    if (list_index >= 0) {
        
        return 0;
    }

    idx = (((-1) * list_index) - 1);
    tmp = FS_HANDLE_LIST_SIZE(unit, hw_type) - 1;
    while (tmp >= idx) {
        sal_memcpy(FS_HANDLE_LIST_ELEMENT_PTR(unit, hw_type, tmp + 1),
                   FS_HANDLE_LIST_ELEMENT_PTR(unit, hw_type, tmp),
                   FS_HANDLE_LIST_ELEMENT_SIZE);
        tmp--;
    }
    _BCM_FLEX_STAT_HANDLE_ASSIGN
        (FS_HANDLE_LIST_ELEMENT_HANDLE(unit, hw_type, idx), handle);
    FS_HANDLE_LIST_ELEMENT_TYPE(unit, hw_type, idx) = fs_type;
    FS_HANDLE_LIST_ELEMENT_INDEX(unit, hw_type, idx) = index;
    FS_HANDLE_LIST_SIZE(unit, hw_type)++;
    return index;
}

STATIC int
_bcm_esw_flex_stat_index_set(int unit, _bcm_flex_stat_type_t fs_type,
                               uint32 index)
{
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);

    SHR_BITSET(FS_USED_BITMAP(unit, hw_type), index);
    return index;
}

STATIC uint32
_bcm_esw_flex_stat_index_get(int unit, _bcm_flex_stat_type_t fs_type,
                               uint32 index)
{
    uint32 ret;

    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);

    ret = SHR_BITGET(FS_USED_BITMAP(unit, hw_type), index);
    return ret;
}

STATIC int
_bcm_esw_flex_stat_ref_cnt_dec(int unit, _bcm_flex_stat_type_t fs_type,
                               uint32 fs_inx)
{
    int ret = BCM_E_NONE;
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);

    if (FS_REF_CNT_ELEMENT(unit,hw_type,fs_inx)) {
        FS_REF_CNT_ELEMENT(unit,hw_type,fs_inx) -= 1;
    } else {
        ret = BCM_E_PARAM;
    }

    return ret;
}

STATIC uint16
_bcm_esw_flex_stat_ref_cnt_get(int unit, _bcm_flex_stat_type_t fs_type,
                               uint32 fs_inx)
{
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);

    return FS_REF_CNT_ELEMENT(unit,hw_type,fs_inx);
}

STATIC int
_bcm_esw_flex_stat_ref_cnt_inc(int unit, _bcm_flex_stat_type_t fs_type,
                               uint32 fs_inx)
{
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);
    int rv = BCM_E_NONE;

    if (!FS_REF_CNT_ELEMENT(unit,hw_type,fs_inx)) {
        /* if first time, clear both hardware/software count */
        rv = _bcm_esw_flex_stat_sw_clear(unit, fs_type, fs_inx);
    }
    FS_REF_CNT_ELEMENT(unit,hw_type,fs_inx) += 1;
    return rv;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_index_insert
 * Purpose:
 *      Add the flexible stat index for the given handle and stat type
 *      to the handle list.
 * Parameters:
 *      unit - SOC unit number
 *      fs_type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      index - flexible stat index
 * Returns:
 *      Flexible stat index, or 0 on resource failure
 */
STATIC int
_bcm_esw_flex_stat_index_insert(int unit, _bcm_flex_stat_type_t fs_type,
                                _bcm_flex_stat_handle_t handle, uint32 index)
{
    int rv;

    rv = _bcm_esw_flex_stat_handle_list_insert(unit,fs_type,handle,index);
    if (!rv) {
        return 0;
    }
    return _bcm_esw_flex_stat_index_set(unit,fs_type,index);
}

/*
 * Function:
 *      _bcm_esw_flex_stat_handle_list_delete
 * Purpose:
 *      Remove the flexible stat index for the given handle and stat type
 *      from the handle list.
 * Parameters:
 *      unit - SOC unit number
 *      fs_type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      index - flexible stat index
 * Returns:
 *      Flexible stat index, or 0 on resource failure
 */
STATIC void
_bcm_esw_flex_stat_handle_list_delete(int unit, _bcm_flex_stat_type_t fs_type,
                                _bcm_flex_stat_handle_t handle)
{
    int list_index, max, tmp;
    _bcm_flex_stat_handle_to_index_t target;
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);

    _BCM_FLEX_STAT_HANDLE_ASSIGN(target.handle, handle);
    target.type = fs_type;
    list_index = _shr_bsearch(FS_HANDLE_LIST(unit, hw_type),
                              FS_HANDLE_LIST_SIZE(unit, hw_type),
                              FS_HANDLE_LIST_ELEMENT_SIZE, &target,
                              &_bcm_flex_stat_handle_to_index_t_compare);

    if (list_index < 0) {
        
        return;
    }

    tmp = list_index;
    max = FS_HANDLE_LIST_SIZE(unit, hw_type) - 1;
    while (tmp < max) {
        sal_memcpy(FS_HANDLE_LIST_ELEMENT_PTR(unit, hw_type, tmp),
                   FS_HANDLE_LIST_ELEMENT_PTR(unit, hw_type, tmp + 1),
                   FS_HANDLE_LIST_ELEMENT_SIZE);
        tmp++;
    }
    FS_HANDLE_LIST_SIZE(unit, hw_type)--;
    sal_memset(FS_HANDLE_LIST_ELEMENT_PTR(unit, hw_type,
                                      FS_HANDLE_LIST_SIZE(unit, hw_type)),
               0, FS_HANDLE_LIST_ELEMENT_SIZE);
    return;
}

STATIC void
_bcm_esw_flex_stat_index_clear(int unit, _bcm_flex_stat_type_t fs_type,
                                uint32 index)
{
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);
    SHR_BITCLR(FS_USED_BITMAP(unit, hw_type), index);
}

/*
 * Function:
 *      _bcm_esw_flex_stat_index_delete
 * Purpose:
 *      Remove the flexible stat index for the given handle and stat type
 *      from the handle list.
 * Parameters:
 *      unit - SOC unit number
 *      fs_type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      index - flexible stat index
 * Returns:
 *      Flexible stat index, or 0 on resource failure
 */
STATIC void
_bcm_esw_flex_stat_index_delete(int unit, _bcm_flex_stat_type_t fs_type,
                                _bcm_flex_stat_handle_t handle, uint32 index)
{
     _bcm_esw_flex_stat_handle_list_delete(unit,fs_type,handle);
     _bcm_esw_flex_stat_index_clear(unit,fs_type,index);
}

/*
 * Function:
 *      _bcm_esw_flex_stat_next_free_index
 * Purpose:
 *      Retrieve the next available flexible stat index for a stat type.
 * Parameters:
 *      unit - SOC unit number
 *      fs_type - flexible stat type
 * Returns:
 *      Flexible stat index
 */
STATIC int
_bcm_esw_flex_stat_next_free_index(int unit, _bcm_flex_stat_type_t fs_type)
{
    int                 ix, mem_size;
    uint32              bit;
    SHR_BITDCL          not_ptrs, *stat_bmp;
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);

    mem_size = FS_MEM_SIZE(unit, hw_type);
    stat_bmp = FS_USED_BITMAP(unit, hw_type);
    for (ix = 0; ix < _SHR_BITDCLSIZE(mem_size); ix++) {
        not_ptrs = ~stat_bmp[ix];
        if (not_ptrs) {
            for (bit = 0; bit < SHR_BITWID; bit++) {
                if (not_ptrs & (1 << bit)) {
                    return (ix * SHR_BITWID) + bit;
                }
            }
        }
    }

    return 0;
}

STATIC int
_bcm_esw_flex_stat_hw_index_set(int unit, _bcm_flex_stat_type_t fs_type,
            int fs_inx, uint64 val)
{
    int rv;
    _bcm_flex_stat_type_t htype;

    htype = FS_TYPE_TO_HW_TYPE(unit, fs_type);

    if ((htype == _bcmFlexStatTypeService) ||
        (htype == _bcmFlexStatTypeGport)) {
        rv = _bcm_esw_flex_stat_hw_set(unit, fs_type, fs_inx,
                      _bcmFlexStatIngressPackets, val);
        if (BCM_SUCCESS(rv)) {
            rv = _bcm_esw_flex_stat_hw_set(unit, fs_type, fs_inx,
                      _bcmFlexStatIngressBytes, val);
        }
    } else  {
        rv = _bcm_esw_flex_stat_hw_set(unit, fs_type, fs_inx,
                      _bcmFlexStatEgressPackets, val);
        if (BCM_SUCCESS(rv)) {
            rv = _bcm_esw_flex_stat_hw_set(unit, fs_type, fs_inx,
                      _bcmFlexStatEgressBytes, val);
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_free_index_assign
 * Purpose:
 *      get the next available flexible stat index for a stat type.
 * Parameters:
 *      unit - SOC unit number
 *      fs_type - flexible stat type
 * Returns:
 *      Flexible stat index
 */
int
_bcm_esw_flex_stat_free_index_assign(int unit, _bcm_flex_stat_type_t fs_type)
{
    int fs_inx;
    int rv;
    uint64 val64;

    COMPILER_64_SET(val64,0,1);
    FS_LOCK(unit);
    fs_inx = _bcm_esw_flex_stat_next_free_index(unit,fs_type);
    fs_inx = _bcm_esw_flex_stat_index_set(unit,fs_type,fs_inx);
    FS_UNLOCK(unit);

    /* set a initial value 1 to the hardware counter to indicate
     * the counter has been reserved, but not used yet, for warmboot
     * purpose.
     */
    rv = _bcm_esw_flex_stat_hw_index_set(unit, fs_type, fs_inx, val64);

    if (BCM_SUCCESS(rv)) {
        return fs_inx;  
    } else {
        return 0;
    }
}

/*
 * Function:
 *      _bcm_esw_flex_stat_free_index_assign
 * Purpose:
 *      get the next available flexible stat index for a stat type.
 * Parameters:
 *      unit - SOC unit number
 *      fs_type - flexible stat type
 * Returns:
 *      Flexible stat index
 */
int
_bcm_esw_flex_stat_count_index_remove(int unit, _bcm_flex_stat_type_t fs_type,
                 int fs_inx)
{
    int rv;
    uint16 ref_cnt;
    uint64 val64;

    COMPILER_64_SET(val64,0,0);
    rv = BCM_E_NONE;
    FS_LOCK(unit);
    ref_cnt = _bcm_esw_flex_stat_ref_cnt_get(unit,fs_type,fs_inx);
    if (!ref_cnt) {
        _bcm_esw_flex_stat_index_clear(unit,fs_type,fs_inx);
    }  else {
         rv = BCM_E_PARAM;
    }
    FS_UNLOCK(unit);

    /* clear the hardware counter */
    if (rv == BCM_E_NONE) {
        rv = _bcm_esw_flex_stat_hw_index_set(unit, fs_type, fs_inx, val64);
    }
    return rv;

}

/*
 * Function:
 *      _bcm_esw_flex_stat_alloc
 * Purpose:
 *      Attempt to allocate a flexible stat index for a stat type.
 * Parameters:
 *      unit - SOC unit number
 *      type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 * Returns:
 *      Success: Flexible stat index
 *      Failure: 0 (invalid stat index)
 */
STATIC int
_bcm_esw_flex_stat_alloc(int unit, _bcm_flex_stat_type_t type,
                         _bcm_flex_stat_handle_t handle)
{
    int index;

    /* Already checked for existence */
    index = _bcm_esw_flex_stat_next_free_index(unit, type);
    if (index) {
        /* Returns 0 on error */
        index = _bcm_esw_flex_stat_index_insert(unit, type, handle, index);
    }

    return index;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_release_handles
 * Purpose:
 *      Deallocate all handles of the given stat type.
 * Parameters:
 *      unit - SOC unit number
 *      fs_type - flexible stat type
 * Returns:
 *      Flexible stat index
 */
void
_bcm_esw_flex_stat_release_handles(int unit, _bcm_flex_stat_type_t fs_type)
{
    int list_index, idx, tmp, max;
    _bcm_flex_stat_handle_to_index_t target;
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, fs_type);

    /* Find start of this type */
    _BCM_FLEX_STAT_HANDLE_CLEAR(target.handle); 
    target.type = fs_type;
    
    FS_LOCK(unit);

    max = FS_HANDLE_LIST_SIZE(unit, hw_type);
    if (max == 0) {
        FS_UNLOCK(unit);
        return;
    }
    list_index = _shr_bsearch(FS_HANDLE_LIST(unit, hw_type), max,
                              FS_HANDLE_LIST_ELEMENT_SIZE, &target,
                              &_bcm_flex_stat_handle_to_index_t_compare);

    /* Starting point */
    if (list_index < 0) {
        idx = (((-1) * list_index) - 1);
    } else {
        idx = list_index;
    }

    tmp = idx;
    while (tmp < max) {
        if (FS_HANDLE_LIST_ELEMENT_TYPE(unit, hw_type, tmp) != fs_type) {
            if (tmp != idx) {
                /* Copy up entry */
                sal_memcpy(FS_HANDLE_LIST_ELEMENT_PTR(unit, hw_type, idx),
                           FS_HANDLE_LIST_ELEMENT_PTR(unit, hw_type, tmp),
                           FS_HANDLE_LIST_ELEMENT_SIZE);
                idx++;
            }
        }
        if (tmp != idx) {
            /* Blank out current position */
            sal_memset(FS_HANDLE_LIST_ELEMENT_PTR(unit, hw_type, tmp),
                       0, FS_HANDLE_LIST_ELEMENT_SIZE);
        }
        tmp++;
    }
    if (idx < tmp) {
        FS_HANDLE_LIST_SIZE(unit, hw_type) = idx + 1;
    }
    FS_UNLOCK(unit);
}

/*
 * Function:
 *      _bcm_esw_flex_stat_reinit_add
 * Purpose:
 *      Record the flexible stat information during reinitialization.
 * Parameters:
 *      unit - SOC unit number
 *      type - flexible stat type
 *      fs_index - the flexible stat index
 *      handle - the encoded handle for the type of packets to count.
 * Returns:
 *      BCM_X_XXX
 */
void
_bcm_esw_flex_stat_ext_reinit_add(int unit, _bcm_flex_stat_type_t type,
                                  int fs_index,
                                  _bcm_flex_stat_handle_t handle)
{
    FS_LOCK(unit);
    (void)_bcm_esw_flex_stat_index_insert(unit, type, handle, fs_index);
    FS_UNLOCK(unit);
    /* Ignoring insert return code */
}

/*
 * Function:
 *      _bcm_esw_vlan_flex_stat_hw_index_set
 * Purpose:
 *      Update the flexible stat pointers of HW tables for VLAN/VFI types.
 * Parameters:
 *      unit - SOC unit number
 *      handle - the encoded handle for the type of packets to count.
 *      fs_idx - the flexible stat index
 *      cookie - info to be used from original calling routine
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_vlan_flex_stat_hw_index_set(int unit, _bcm_flex_stat_handle_t fsh,
                                     int fs_idx, void *cookie)
{
    bcm_vlan_t handle = _BCM_FLEX_STAT_HANDLE_WORD_GET(fsh, 0);
    uint32 flags;
#if defined(INCLUDE_L3)
    int vfi;
#endif /* INCLUDE_L3 */

    flags = cookie==NULL? _BCM_FLEX_STAT_HW_BOTH:
            PTR_TO_INT(cookie);
#if defined(INCLUDE_L3)

    /* Is it a VPN? */
    if (_BCM_MPLS_VPN_IS_SET(handle)) {
        _BCM_MPLS_VPN_GET(vfi, _BCM_MPLS_VPN_TYPE_VPLS, handle);
        return _bcm_vfi_flex_stat_index_set(unit, vfi,
                                            _bcmVfiTypeMpls, fs_idx,flags);
    } else if (_BCM_MIM_VPN_IS_SET(handle)) {
        _BCM_MIM_VPN_GET(vfi, _BCM_MIM_VPN_TYPE_MIM, handle);
        return _bcm_vfi_flex_stat_index_set(unit, vfi,
                                            _bcmVfiTypeMim, fs_idx,flags);
    } else 
#endif /* INCLUDE_L3 */
    {
        return _bcm_esw_vlan_flex_stat_index_set(unit, handle, fs_idx,flags);
    }    
}

/*
 * Function:
 *      _bcm_esw_port_flex_stat_hw_index_set
 * Purpose:
 *      Update the flexible stat pointers of HW tables for GPORT types.
 * Parameters:
 *      unit - SOC unit number
 *      handle - the encoded handle for the type of packets to count.
 *      fs_idx - the flexible stat index
 *      cookie - info to be used from original calling routine
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_port_flex_stat_hw_index_set(int unit, _bcm_flex_stat_handle_t fsh,
                                     int fs_idx, void *cookie)
{
    uint32 flags;
    uint32 handle = _BCM_FLEX_STAT_HANDLE_WORD_GET(fsh, 0);

    COMPILER_REFERENCE(cookie);

    flags = cookie==NULL? _BCM_FLEX_STAT_HW_BOTH:
            PTR_TO_INT(cookie);

#if defined(INCLUDE_L3)
    if (BCM_GPORT_IS_MPLS_PORT(handle)) {
#if defined(BCM_MPLS_SUPPORT)
        return _bcm_esw_mpls_flex_stat_index_set(unit, handle, fs_idx,flags);
#else
        return BCM_E_UNAVAIL;
#endif /* BCM_MPLS_SUPPORT */
    } else if (BCM_GPORT_IS_SUBPORT_GROUP(handle) ||
               BCM_GPORT_IS_SUBPORT_PORT(handle)) {
        return _bcm_esw_subport_flex_stat_index_set(unit, handle, fs_idx,flags);
    } else if (BCM_GPORT_IS_MIM_PORT(handle)) {
        return _bcm_esw_mim_flex_stat_index_set(unit, handle, fs_idx,flags);
    } else if (BCM_GPORT_IS_WLAN_PORT(handle)) {
        return _bcm_esw_wlan_flex_stat_index_set(unit, handle, fs_idx,flags);
    } else 
#endif /* INCLUDE_L3 */
    {
        /* handle GPORT must be a local physical port */
        return _bcm_esw_port_flex_stat_index_set(unit, handle, fs_idx,flags);
    }
}

/*
 * Function:
 *      _bcm_esw_flex_stat_handle_free
 * Purpose:
 *      Deallocate the flexible stat for a handle that has been deleted
 *      by its controlling module.
 * Parameters:
 *      unit - SOC unit number
 *      type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 * Returns:
 *      Success: Flexible stat index
 *      Failure: 0 (invalid stat index)
 * Notes:
 *      No HW update is required because the controlling module clears
 *      the entry for the associated handle. We note that this FS index
 *      is again usable, and the FS HW counters will be reset when this
 *      index is next used.
 */
void
_bcm_esw_flex_stat_ext_handle_free(int unit, _bcm_flex_stat_type_t type,
                                   _bcm_flex_stat_handle_t handle)
{
    int fs_idx;
    _bcm_flex_stat_type_t hw_type = FS_TYPE_TO_HW_TYPE(unit, type);

    FS_LOCK(unit);
    if (FS_INFO(unit, hw_type) != NULL) {
        fs_idx = _bcm_esw_flex_stat_index(unit, type, handle);
        if (fs_idx > 0) {
            _bcm_esw_flex_stat_index_delete(unit, type, handle, fs_idx);
        }
    }
    FS_UNLOCK(unit);
}

/* Write to Flexible Counter HW */
STATIC int
_bcm_esw_flex_stat_hw_set(int unit, _bcm_flex_stat_type_t type,
                          int fs_idx, _bcm_flex_stat_t stat, uint64 val)
{
    ing_service_counter_table_entry_t flex_ctr_entry;
    soc_mem_t mem;
    soc_field_t stat_field = BYTE_COUNTERf;
    int rv;

    switch (stat) {
    case _bcmFlexStatIngressPackets:
        stat_field = PACKET_COUNTERf;
        /* Fall thru */
    case _bcmFlexStatIngressBytes:
        mem = FS_ING_MEM(type);
        break;
    case _bcmFlexStatEgressPackets:
        stat_field = PACKET_COUNTERf;
        /* Fall thru */
    case _bcmFlexStatEgressBytes:
        mem = FS_EGR_MEM(type);
        break;
    default:
        return BCM_E_PARAM;
    }

    MEM_LOCK(unit, mem);
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        soc_mem_t mem_x, mem_y;
        uint64 zero64;

        switch (mem) {
        case ING_SERVICE_COUNTER_TABLEm:
            mem_x = ING_SERVICE_COUNTER_TABLE_Xm;
            mem_y = ING_SERVICE_COUNTER_TABLE_Ym;
            break;
        case EGR_SERVICE_COUNTER_TABLEm:
            mem_x = EGR_SERVICE_COUNTER_TABLE_Xm; 
            mem_y = EGR_SERVICE_COUNTER_TABLE_Ym; 
            break;
        case ING_VINTF_COUNTER_TABLEm:
            mem_x = ING_VINTF_COUNTER_TABLE_Xm; 
            mem_y = ING_VINTF_COUNTER_TABLE_Ym; 
            break;
        case EGR_VINTF_COUNTER_TABLEm:
            mem_x = EGR_VINTF_COUNTER_TABLE_Xm; 
            mem_y = EGR_VINTF_COUNTER_TABLE_Ym; 
            break;
        /* Defensive Default */
        /* coverity[dead_error_begin] */
        default:
            return BCM_E_INTERNAL;
        }
        rv = soc_mem_read(unit, mem_x, MEM_BLOCK_ANY,
                          fs_idx, &flex_ctr_entry);
        if (BCM_SUCCESS(rv)) {
            soc_mem_field64_set(unit, mem_x, (uint32 *)&flex_ctr_entry,
                                stat_field, val);
            rv = soc_mem_write(unit, mem_x, MEM_BLOCK_ANY, fs_idx,
                               &flex_ctr_entry);
        }
        /* Dual pipe, write non-zero value to X pipe, 0 to Y pipe */
        if (BCM_SUCCESS(rv)) {
            rv = soc_mem_read(unit, mem_y, MEM_BLOCK_ANY,
                              fs_idx, &flex_ctr_entry);
        }
        if (BCM_SUCCESS(rv)) {
            COMPILER_64_ZERO(zero64);
            soc_mem_field64_set(unit, mem_y, (uint32 *)&flex_ctr_entry,
                                stat_field, zero64);
            rv = soc_mem_write(unit, mem_y, MEM_BLOCK_ANY, fs_idx,
                               &flex_ctr_entry);
        }
    } else
#endif /* BCM_TRIDENT_SUPPORT */
    {
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, fs_idx, &flex_ctr_entry);
        if (BCM_SUCCESS(rv)) {
            soc_mem_field64_set(unit, mem, (uint32 *)&flex_ctr_entry,
                                stat_field, val);
            rv = soc_mem_write(unit, mem, MEM_BLOCK_ANY, fs_idx,
                               &flex_ctr_entry);
        }
    }
    MEM_UNLOCK(unit, mem);

    return rv;
}

/* Write to Flexible Counter SW cache and HW. 
   Value is written to SW as is but is trimmed for HW */
STATIC int
_bcm_esw_flex_stat_sw_set(int unit, _bcm_flex_stat_type_t type,
                          int fs_idx, _bcm_flex_stat_t stat, uint64 val)
{
    uint8 htype, dir = 0 /* ingress */, gran = 0 /* pkts */;
    soc_mem_t mem;
    soc_field_t stat_field = PACKET_COUNTERf;
    ing_service_counter_table_entry_t *bptr, *eptr;
    uint64 *cptr;
    uint32 value[2], mask[2];

    htype = FS_TYPE_TO_SVC_VINTF_HW_TYPE(unit, type);

    if (stat > _bcmFlexStatIngressBytes) {
        /* egress */
        dir = 1;
    }
    if ((stat == _bcmFlexStatIngressBytes) || 
        (stat == _bcmFlexStatEgressBytes)) {
        /* bytes */ 
        gran = 1;
        mask[0] = 0xffffffff;
#ifdef BCM_TRIDENT_SUPPORT
        if (SOC_IS_TD_TT(unit)) {
            mask[1] = 0xf; /* 36 bits */
            stat_field = BYTE_COUNTERf;
        } else 
#endif /* BCM_TRIDENT_SUPPORT */
        {
            mask[1] = 0x7; /* 35 bits */
            stat_field = BYTE_COUNTERf;
        }
    } else {
        /* packets */
        mask[0] = 0x1fffffff; /* 29 bits */
        mask[1] = 0; 
    }
    cptr = _bcm_flex_stat_mem[unit][htype][dir][gran];
    cptr[fs_idx] = val; 
    value[0] = COMPILER_64_LO(val) & mask[0];
    value[1] = COMPILER_64_HI(val) & mask[1];
    COMPILER_64_SET(val, value[1], value[0]);
    /* Set in the 'other' HW buffer */
    bptr = _bcm_flex_stat_buff[unit][htype][dir]
                              [!_bcm_flex_stat_buff_toggle];
    mem = _bcm_flex_stat_mem_types[(htype * _BCM_FLEX_STAT_DIRS) + dir];
    eptr = soc_mem_table_idx_to_pointer(unit, mem,
                                        ing_service_counter_table_entry_t *,
                                        bptr, fs_idx);
    soc_mem_field_set(unit, mem, (uint32 *)eptr, stat_field, value);
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        /* Clear Y-Pipe 'other' HW buffer. */
        bptr = _bcm_flex_stat_buff_y[unit][htype][dir]
            [!_bcm_flex_stat_buff_toggle];
        if (NULL == bptr) {
            /* Lock released in calling routine. */
            return (BCM_E_INTERNAL);
        }
        eptr = soc_mem_table_idx_to_pointer(unit, mem,
                                        ing_service_counter_table_entry_t *,
                                            bptr, fs_idx);
        /* Clear Y-pipe buffer value. */
        value[0] = value[1] = 0;
        soc_mem_field_set(unit, mem, (uint32 *)eptr, stat_field, value);
    }
#endif /* BCM_TRIDENT_SUPPORT */

    return _bcm_esw_flex_stat_hw_set(unit, type, fs_idx, stat, val);
}

/* Read from Flexible Counters SW cache */
STATIC int
_bcm_esw_flex_stat_sw_get(int unit, _bcm_flex_stat_type_t type,
                          int fs_idx, _bcm_flex_stat_t stat, uint64 *val)
{
    int rv = BCM_E_NONE;
    uint8 htype, dir = 0 /* ingress */, gran = 0 /* pkts */;
    uint64 *cptr;
    htype = FS_TYPE_TO_SVC_VINTF_HW_TYPE(unit, type);
    if (stat > _bcmFlexStatIngressBytes) {
        /* egress */
        dir = 1;
    }
    if ((stat == _bcmFlexStatIngressBytes) || 
        (stat == _bcmFlexStatEgressBytes)) {
        /* bytes */ 
        gran = 1;
    } 
    cptr = _bcm_flex_stat_mem[unit][htype][dir][gran];
    *val = cptr[fs_idx];
    return rv;
}

/* Read from Flexible Counters HW */
int
_bcm_esw_flex_stat_hw_get(int unit, _bcm_flex_stat_type_t type,
                          int fs_idx, _bcm_flex_stat_t stat, uint64 *val)
{
    ing_service_counter_table_entry_t flex_ctr_entry;
    soc_mem_t mem;
    soc_field_t stat_field = BYTE_COUNTERf;
    int rv;

    switch (stat) {
    case _bcmFlexStatIngressPackets:
        stat_field = PACKET_COUNTERf;
        /* Fall thru */
    case _bcmFlexStatIngressBytes:
        mem = FS_ING_MEM(type);
        break;
    case _bcmFlexStatEgressPackets:
        stat_field = PACKET_COUNTERf;
        /* Fall thru */
    case _bcmFlexStatEgressBytes:
        mem = FS_EGR_MEM(type);
        break;
    default:
        return BCM_E_PARAM;
    }

    MEM_LOCK(unit, mem);
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        soc_mem_t mem_x, mem_y;
        uint64 val_y;

        switch (mem) {
        case ING_SERVICE_COUNTER_TABLEm:
            mem_x = ING_SERVICE_COUNTER_TABLE_Xm;
            mem_y = ING_SERVICE_COUNTER_TABLE_Ym;
            break;
        case EGR_SERVICE_COUNTER_TABLEm:
            mem_x = EGR_SERVICE_COUNTER_TABLE_Xm; 
            mem_y = EGR_SERVICE_COUNTER_TABLE_Ym; 
            break;
        case ING_VINTF_COUNTER_TABLEm:
            mem_x = ING_VINTF_COUNTER_TABLE_Xm; 
            mem_y = ING_VINTF_COUNTER_TABLE_Ym; 
            break;
        case EGR_VINTF_COUNTER_TABLEm:
            mem_x = EGR_VINTF_COUNTER_TABLE_Xm; 
            mem_y = EGR_VINTF_COUNTER_TABLE_Ym; 
            break;
        /* Defensive Default */
        /* coverity[dead_error_begin] */
        default:
            return BCM_E_INTERNAL;
        }
        rv = soc_mem_read(unit, mem_x, MEM_BLOCK_ANY,
                          fs_idx, &flex_ctr_entry);
        if (BCM_SUCCESS(rv)) {
            soc_mem_field64_get(unit, mem_x, (uint32 *)&flex_ctr_entry,
                                stat_field, val);
        }
        /* Dual pipe, read from X pipe and Y pipe then add */
        if (BCM_SUCCESS(rv)) {
            rv = soc_mem_read(unit, mem_y, MEM_BLOCK_ANY,
                              fs_idx, &flex_ctr_entry);
        }
        if (BCM_SUCCESS(rv)) {
            soc_mem_field64_get(unit, mem_y, (uint32 *)&flex_ctr_entry,
                                stat_field, &val_y);
            COMPILER_64_ADD_64(*val, val_y);
        }
    } else
#endif /* BCM_TRIDENT_SUPPORT */
    {
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, fs_idx, &flex_ctr_entry);

        if (BCM_SUCCESS(rv)) {
            soc_mem_field64_get(unit, mem, (uint32 *)&flex_ctr_entry,
                                stat_field, val);
        }
    }
    MEM_UNLOCK(unit, mem);

    return rv;
}

/* Clear the Flexible Counter HW */
STATIC int
_bcm_esw_flex_stat_hw_clear(int unit, _bcm_flex_stat_type_t type,
                            int fs_idx)
{
    ing_service_counter_table_entry_t flex_ctr_entry;
    soc_mem_t mem;
    int rv = BCM_E_NONE;

    /* This will work on dual-pipes because the base memory will write
     * the same value to both pipe memories. */
    sal_memset(&flex_ctr_entry, 0, sizeof(flex_ctr_entry));

    mem = FS_ING_MEM(type);
    MEM_LOCK(unit, mem);
    rv = soc_mem_write(unit, mem, MEM_BLOCK_ANY, fs_idx, &flex_ctr_entry);
    MEM_UNLOCK(unit, mem);

    if (BCM_SUCCESS(rv)) {
        mem = FS_EGR_MEM(type);
        MEM_LOCK(unit, mem);
        rv = soc_mem_write(unit, mem, MEM_BLOCK_ANY, fs_idx,
                           &flex_ctr_entry);
        MEM_UNLOCK(unit, mem);
    }

    return rv;
}

/* Clear the Flexible Counter SW and HW */
STATIC int
_bcm_esw_flex_stat_sw_clear(int unit, _bcm_flex_stat_type_t type,
                            int fs_idx)
{
    uint64 *cptr;
    ing_service_counter_table_entry_t *bptr, *eptr;
    uint8 htype = FS_TYPE_TO_SVC_VINTF_HW_TYPE(unit, type);
    /* Clear for both directions and both granularities */
    cptr = _bcm_flex_stat_mem[unit][htype][0][0];
    COMPILER_64_ZERO(cptr[fs_idx]);
    cptr = _bcm_flex_stat_mem[unit][htype][0][1];
    COMPILER_64_ZERO(cptr[fs_idx]);
    cptr = _bcm_flex_stat_mem[unit][htype][1][0];
    COMPILER_64_ZERO(cptr[fs_idx]);
    cptr = _bcm_flex_stat_mem[unit][htype][1][1];
    COMPILER_64_ZERO(cptr[fs_idx]);
    /* Clear for both directions only in the 'other' HW buffer */
    bptr = _bcm_flex_stat_buff[unit][htype][0][!_bcm_flex_stat_buff_toggle];
    eptr = soc_mem_table_idx_to_pointer(unit, ING_SERVICE_COUNTER_TABLEm,
                                        ing_service_counter_table_entry_t *,
                                        bptr, fs_idx);
    sal_memset(eptr, 0, sizeof(ing_service_counter_table_entry_t));
    bptr = _bcm_flex_stat_buff[unit][htype][1][!_bcm_flex_stat_buff_toggle];
    eptr = soc_mem_table_idx_to_pointer(unit, ING_SERVICE_COUNTER_TABLEm,
                                        ing_service_counter_table_entry_t *,
                                        bptr, fs_idx);
    sal_memset(eptr, 0, sizeof(ing_service_counter_table_entry_t));
#ifdef BCM_TRIDENT_SUPPORT
    if (SOC_IS_TD_TT(unit)) {
        bptr = _bcm_flex_stat_buff_y[unit][htype][0][!_bcm_flex_stat_buff_toggle];
        eptr = soc_mem_table_idx_to_pointer(unit, ING_SERVICE_COUNTER_TABLEm,
                                        ing_service_counter_table_entry_t *,
                                            bptr, fs_idx);
        sal_memset(eptr, 0, sizeof(ing_service_counter_table_entry_t));
        bptr = _bcm_flex_stat_buff_y[unit][htype][1][!_bcm_flex_stat_buff_toggle];
        eptr = soc_mem_table_idx_to_pointer(unit, ING_SERVICE_COUNTER_TABLEm,
                                        ing_service_counter_table_entry_t *,
                                            bptr, fs_idx);
        sal_memset(eptr, 0, sizeof(ing_service_counter_table_entry_t));
    }
#endif /* BCM_TRIDENT_SUPPORT */
    return _bcm_esw_flex_stat_hw_clear(unit, type, fs_idx);
}

/*
 * Function:
 *      _bcm_esw_flex_stat_enable_set
 * Purpose:
 *      Attach/detach a flexible statistic resource to a given packet
 *      lookup table hit.
 * Parameters:
 *      unit - SOC unit number
 *      type - flexible stat type
 *      fs_set_f - callback function to write index into HW table
 *      cookie - cookie for callback function
 *      handle - the encoded handle for the type of packets to count.
 *      enable - TRUE: configure flexible stat collection for the handle.
 *               FALSE: stop flexible stat collection for the handle.
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_flex_stat_ext_enable_set(int unit, _bcm_flex_stat_type_t type,
                                  _bcm_flex_stat_src_index_set_f fs_set_f,
                                  void *cookie,
                                  _bcm_flex_stat_handle_t handle, int enable,
                                  int cnt_inx)
                       
{
    int fs_idx, rv = BCM_E_NONE;

    FS_LOCK(unit);
    FS_INIT(unit, type);

    /* non-zero cnt_inx,  used by the new flex count APIs
     * indicates a counter index is allocated outside of this routine. 
     * It doesn't have one-to-one mapping with a handle. Several handles
     * may be associated with a same counter index. For delete operation, 
     * cnt_inx should be ignore. A entry is removed from the handle list 
     * based on the handle.
     *
     * zero cnt_inx
     * Implies the legacy implementation. The counter index is automatically 
     * allocated with the handle entry, and has one-to-one mapping with a
     * handle.
     */
    if (cnt_inx) {
        /* legacy and new APIs cannot be mixed. Validate it first */
        if (_tr2_flex_stat_api_ver[unit]) {
            /* must be new APIs */
            if (_tr2_flex_stat_api_ver[unit] != TR2_FLEX_STAT_API_NEW) {
                soc_cm_print("ERROR: _bcm_esw_flex_stat_ext_enable_set: "
                  "cannot mix the new API with legacy API\n");
                return BCM_E_PARAM;
            }
        } else {  /* first time */
            _tr2_flex_stat_api_ver[unit] = TR2_FLEX_STAT_API_NEW;
        }

        /* check if stat counter is already created */
        if (!_bcm_esw_flex_stat_index_get(unit, type, cnt_inx)) {
            FS_UNLOCK(unit);
            return BCM_E_PARAM;
        }
        fs_idx = _bcm_esw_flex_stat_index(unit, type, handle);
        if (enable) {
            if (fs_idx > 0) {
                rv = BCM_E_EXISTS;
            } else {
                _bcm_esw_flex_stat_handle_list_insert(unit,type,handle,
                                                      cnt_inx);
                rv = (*fs_set_f)(unit, handle, cnt_inx, cookie);
                if (BCM_FAILURE(rv)) {
                    _bcm_esw_flex_stat_handle_list_delete(unit, type, handle);
                } else {
                    rv = _bcm_esw_flex_stat_ref_cnt_inc(unit,type,cnt_inx);
                }
            }
        } else {  /* enable FALSE */
            if (fs_idx) { 
                _bcm_esw_flex_stat_handle_list_delete(unit,type,handle);
                rv = (*fs_set_f)(unit, handle, 0, cookie);
                if (BCM_SUCCESS(rv)) {
                    rv = _bcm_esw_flex_stat_ref_cnt_dec(unit,type,fs_idx);
                } 
            } else {
                rv = BCM_E_NOT_FOUND;
            }
        }
        FS_UNLOCK(unit);
        return rv;
    }

    /* legacy API is used */
    if (_tr2_flex_stat_api_ver[unit]) {    /* must be legacy APIs */
        if (_tr2_flex_stat_api_ver[unit] != TR2_FLEX_STAT_API_LEGACY) {
            soc_cm_print("ERROR _bcm_esw_flex_stat_ext_enable_set: cannot mix "
                  "the new API with legacy API\n");
            return BCM_E_PARAM;
        }
    } else {  /* first time */
        _tr2_flex_stat_api_ver[unit] = TR2_FLEX_STAT_API_LEGACY;
    }

    fs_idx = _bcm_esw_flex_stat_index(unit, type, handle);
    if (fs_idx > 0) {
        if (enable) {
            rv = BCM_E_EXISTS;
        } else {
            _bcm_esw_flex_stat_index_delete(unit, type, handle, fs_idx);
            rv = (*fs_set_f)(unit, handle, 0, cookie);
        }
    } else {
        if (!enable) {
            rv = BCM_E_NOT_FOUND;
        } else {
            fs_idx = _bcm_esw_flex_stat_alloc(unit, type, handle);
            if (fs_idx) {
                rv = (*fs_set_f)(unit, handle, fs_idx, cookie);
                if (BCM_FAILURE(rv)) {
                    _bcm_esw_flex_stat_index_delete(unit, type,
                                                    handle, fs_idx);
                }
            } else {
                rv = BCM_E_RESOURCE;
            }
            /* Start with cleared stats for this index */
            if (BCM_SUCCESS(rv)) {
                rv = _bcm_esw_flex_stat_sw_clear(unit, type, fs_idx);
            }
        }
    }
    FS_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_get
 * Purpose:
 *      Retrieve a flexible stat for a handle 
 * Parameters:
 *      unit - SOC unit number
 *      type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      stat - (IN) Type of the flexible statistic to retrieve.
 *      val - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_flex_stat_ext_get(int unit, _bcm_flex_stat_type_t type,
                           _bcm_flex_stat_handle_t handle,
                           _bcm_flex_stat_t stat, uint64 *val)
{
    int fs_idx, rv = BCM_E_NONE;

    if (NULL == val) {
        return (BCM_E_PARAM);
    }

    if ((stat < _bcmFlexStatIngressPackets) ||
        (stat >= _bcmFlexStatNum)) {
        return (BCM_E_PARAM);
    }

    FS_LOCK(unit);
    FS_INIT(unit, type);

    fs_idx = _bcm_esw_flex_stat_index(unit, type, handle);
    if (fs_idx > 0) {
        rv = _bcm_esw_flex_stat_sw_get(unit, type, fs_idx, stat, val);
    } else {
        rv = BCM_E_NOT_FOUND;
    }
    FS_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_get32
 * Purpose:
 *      Retrieve a flexible stat for a handle in 32-bit form 
 * Parameters:
 *      unit - SOC unit number
 *      type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      stat - (IN) Type of the flexible statistic to retrieve.
 *      val - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_flex_stat_ext_get32(int unit, _bcm_flex_stat_type_t type,
                             _bcm_flex_stat_handle_t handle,
                             _bcm_flex_stat_t stat, uint32 *val)
{
    uint64 val64;            /* 64 bit counter value.    */
    int rv;

    if (NULL == val) {
        return (BCM_E_PARAM);
    }

    rv = _bcm_esw_flex_stat_ext_get(unit, type, handle, stat, &val64);

    if (BCM_SUCCESS(rv)) {
        *val = COMPILER_64_LO(val64);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_set
 * Purpose:
 *      Assign a flexible stat for a handle 
 * Parameters:
 *      unit - SOC unit number
 *      type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      stat - (IN) Type of the flexible statistic to retrieve.
 *      val - (IN) Counter value.
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_flex_stat_ext_set(int unit, _bcm_flex_stat_type_t type,
                           _bcm_flex_stat_handle_t handle,
                           _bcm_flex_stat_t stat, uint64 val)
{
    int fs_idx, rv = BCM_E_NONE;

    if ((stat < _bcmFlexStatIngressPackets) ||
        (stat >= _bcmFlexStatNum)) {
        return (BCM_E_PARAM);
    }

    FS_LOCK(unit);
    FS_INIT(unit, type);

    fs_idx = _bcm_esw_flex_stat_index(unit, type, handle);
    if (fs_idx > 0) {
        rv = _bcm_esw_flex_stat_sw_set(unit, type, fs_idx, stat, val);
    } else {
        rv = BCM_E_NOT_FOUND;
    }
    FS_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_set32
 * Purpose:
 *      Assign a flexible stat for a handle in 32-bit form 
 * Parameters:
 *      unit - SOC unit number
 *      type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      stat - (IN) Type of the flexible statistic to retrieve.
 *      val - (OUT) Pointer to a counter value.
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_esw_flex_stat_ext_set32(int unit, _bcm_flex_stat_type_t type,
                             _bcm_flex_stat_handle_t handle,
                             _bcm_flex_stat_t stat, uint32 val)
{
    uint64 val64;            /* 64 bit counter value.    */

    COMPILER_64_SET(val64, 0, val);
    return _bcm_esw_flex_stat_ext_set(unit, type, handle, stat, val64);
}

/*
 * Function:
 *      _bcm_esw_flex_stat_multi_get
 * Purpose:
 *      Get 64-bit counter value for multiple flexible statistic types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Collected statistics descriptors array
 *      value_arr - (OUT) Collected counters values
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
_bcm_esw_flex_stat_ext_multi_get(int unit, _bcm_flex_stat_type_t type,
                                 _bcm_flex_stat_handle_t handle, int nstat,
                                 _bcm_flex_stat_t *stat_arr,
                                 uint64 *value_arr)
{
    int                 rv = BCM_E_NONE; /* Operation return status.    */
    int                 idx;             /* Statistics iteration index. */
    int                 fs_idx;          /* Flexible stat mem index.    */

    FS_LOCK(unit);
    FS_INIT(unit, type);

    fs_idx = _bcm_esw_flex_stat_index(unit, type, handle);
    if (fs_idx > 0) {
        /* Read individual statistics. */
        for (idx = 0; idx < nstat; idx++) {
            if ((stat_arr[idx] < _bcmFlexStatIngressPackets) ||
                (stat_arr[idx] >= _bcmFlexStatNum)) {
                rv = BCM_E_PARAM;
                break;
            }
            rv = _bcm_esw_flex_stat_sw_get(unit, type, fs_idx,
                                           stat_arr[idx], value_arr + idx);
            if (BCM_FAILURE(rv)) {
                break;
            }
        }
    } else {
        rv = BCM_E_NOT_FOUND;
    }
    FS_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_multi_get32
 * Purpose:
 *      Get 32-bit counter value for multiple flexible statistic types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Collected statistics descriptors array
 *      value_arr - (OUT) Collected counters values
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
_bcm_esw_flex_stat_ext_multi_get32(int unit, _bcm_flex_stat_type_t type,
                                   _bcm_flex_stat_handle_t handle, int nstat,
                                   _bcm_flex_stat_t *stat_arr,
                                   uint32 *value_arr)
{
    uint64              value;           /* 64 bit counter value.       */
    int                 rv = BCM_E_NONE; /* Operation return status.    */
    int                 idx;             /* Statistics iteration index. */
    int                 fs_idx;          /* Flexible stat mem index.    */

    FS_LOCK(unit);
    FS_INIT(unit, type);

    fs_idx = _bcm_esw_flex_stat_index(unit, type, handle);
    if (fs_idx > 0) {
        /* Read individual statistics. */
        for (idx = 0; idx < nstat; idx++) {
            if ((stat_arr[idx] < _bcmFlexStatIngressPackets) ||
                (stat_arr[idx] >= _bcmFlexStatNum)) {
                rv = BCM_E_PARAM;
                break;
            }
            rv = _bcm_esw_flex_stat_sw_get(unit, type, fs_idx,
                                           stat_arr[idx], &value);
            if (BCM_FAILURE(rv)) {
                break;
            }
            value_arr[idx] = COMPILER_64_LO(value);
        }
    } else {
        rv = BCM_E_NOT_FOUND;
    }
    FS_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_multi_set
 * Purpose:
 *      Assign 64-bit counter value for multiple flexible statistic types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Collected statistics descriptors array
 *      value_arr - (IN) Collected counters values
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
_bcm_esw_flex_stat_ext_multi_set(int unit, _bcm_flex_stat_type_t type,
                                 _bcm_flex_stat_handle_t handle, int nstat,
                                 _bcm_flex_stat_t *stat_arr, 
                                 uint64 *value_arr)
{
    int                 rv = BCM_E_NONE; /* Operation return status.    */
    int                 idx;             /* Statistics iteration index. */
    int                 fs_idx;          /* Flexible stat mem index.    */

    FS_LOCK(unit);
    FS_INIT(unit, type);

    fs_idx = _bcm_esw_flex_stat_index(unit, type, handle);
    if (fs_idx > 0) {
        /* Read individual statistics. */
        for (idx = 0; idx < nstat; idx++) {
            if ((stat_arr[idx] < _bcmFlexStatIngressPackets) ||
                (stat_arr[idx] >= _bcmFlexStatNum)) {
                rv = BCM_E_PARAM;
                break;
            }
            rv = _bcm_esw_flex_stat_sw_set(unit, type, fs_idx,
                                           stat_arr[idx], value_arr[idx]);
            if (BCM_FAILURE(rv)) {
                break;
            }
        }
    } else {
        rv = BCM_E_NOT_FOUND;
    }
    FS_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      _bcm_esw_flex_stat_multi_set32
 * Purpose:
 *      Assign 32-bit counter value for multiple flexible statistic types.
 * Parameters:
 *      unit - (IN) Unit number.
 *      type - flexible stat type
 *      handle - the encoded handle for the type of packets to count.
 *      nstat - (IN) Number of elements in stat array
 *      stat_arr - (IN) Collected statistics descriptors array
 *      value_arr - (IN) Collected counters values
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
_bcm_esw_flex_stat_ext_multi_set32(int unit, _bcm_flex_stat_type_t type,
                                   _bcm_flex_stat_handle_t handle, int nstat,
                                   _bcm_flex_stat_t *stat_arr, 
                                   uint32 *value_arr)
{
    uint64              value;           /* 64 bit counter value.       */
    int                 rv = BCM_E_NONE; /* Operation return status.    */
    int                 idx;             /* Statistics iteration index. */
    int                 fs_idx;          /* Flexible stat mem index.    */

    FS_LOCK(unit);
    FS_INIT(unit, type);

    fs_idx = _bcm_esw_flex_stat_index(unit, type, handle);
    if (fs_idx > 0) {
        /* Read individual statistics. */
        for (idx = 0; idx < nstat; idx++) {
            if ((stat_arr[idx] < _bcmFlexStatIngressPackets) ||
                (stat_arr[idx] >= _bcmFlexStatNum)) {
                rv = BCM_E_PARAM;
                break;
            }
            COMPILER_64_SET(value, 0, value_arr[idx]);
            rv = _bcm_esw_flex_stat_sw_set(unit, type, fs_idx,
                                           stat_arr[idx], value);
            if (BCM_FAILURE(rv)) {
                break;
            }
        }
    } else {
        rv = BCM_E_NOT_FOUND;
    }
    FS_UNLOCK(unit);

    return rv;
}

/* Functions for legacy uint32 handles to newer extended handles */
void
_bcm_esw_flex_stat_reinit_add(int unit, _bcm_flex_stat_type_t type,
                              int fs_index, uint32 handle)
{
    _bcm_flex_stat_handle_t fsh;
    _BCM_FLEX_STAT_HANDLE_CLEAR(fsh);
    _BCM_FLEX_STAT_HANDLE_WORD_SET(fsh, 0, handle);

    _bcm_esw_flex_stat_ext_reinit_add(unit, type, fs_index, fsh);
}

void
_bcm_esw_flex_stat_handle_free(int unit, _bcm_flex_stat_type_t type,
                               uint32 handle)
{
    _bcm_flex_stat_handle_t fsh;
    _BCM_FLEX_STAT_HANDLE_CLEAR(fsh);
    _BCM_FLEX_STAT_HANDLE_WORD_SET(fsh, 0, handle);

    _bcm_esw_flex_stat_ext_handle_free(unit, type, fsh);
}

int
_bcm_esw_flex_stat_enable_set(int unit, _bcm_flex_stat_type_t type,
                              _bcm_flex_stat_src_index_set_f fs_set_f,
                              void *cookie, uint32 handle, int enable,
           int cnt_inx /* count index, 0 for legacy API, auto allocated */
           )
{
    _bcm_flex_stat_handle_t fsh;
    _BCM_FLEX_STAT_HANDLE_CLEAR(fsh);
    _BCM_FLEX_STAT_HANDLE_WORD_SET(fsh, 0, handle);

    return _bcm_esw_flex_stat_ext_enable_set(unit, type, fs_set_f,
                                             cookie, fsh, enable,cnt_inx);
}

int
_bcm_esw_flex_stat_get(int unit, _bcm_flex_stat_type_t type,
                       uint32 handle, _bcm_flex_stat_t stat, uint64 *val)
{
    _bcm_flex_stat_handle_t fsh;
    _BCM_FLEX_STAT_HANDLE_CLEAR(fsh);
    _BCM_FLEX_STAT_HANDLE_WORD_SET(fsh, 0, handle);

    return _bcm_esw_flex_stat_ext_get(unit, type, fsh, stat, val);
}

int
_bcm_esw_flex_stat_get32(int unit, _bcm_flex_stat_type_t type,
                         uint32 handle, _bcm_flex_stat_t stat, uint32 *val)
{
    _bcm_flex_stat_handle_t fsh;
    _BCM_FLEX_STAT_HANDLE_CLEAR(fsh);
    _BCM_FLEX_STAT_HANDLE_WORD_SET(fsh, 0, handle);

    return _bcm_esw_flex_stat_ext_get32(unit, type, fsh, stat, val);
}

int
_bcm_esw_flex_stat_set(int unit, _bcm_flex_stat_type_t type,
                       uint32 handle, _bcm_flex_stat_t stat, uint64 val)
{
    _bcm_flex_stat_handle_t fsh;
    _BCM_FLEX_STAT_HANDLE_CLEAR(fsh);
    _BCM_FLEX_STAT_HANDLE_WORD_SET(fsh, 0, handle);

    return _bcm_esw_flex_stat_ext_set(unit, type, fsh, stat, val);
}

int
_bcm_esw_flex_stat_set32(int unit, _bcm_flex_stat_type_t type,
                         uint32 handle, _bcm_flex_stat_t stat, uint32 val)
{
    _bcm_flex_stat_handle_t fsh;
    _BCM_FLEX_STAT_HANDLE_CLEAR(fsh);
    _BCM_FLEX_STAT_HANDLE_WORD_SET(fsh, 0, handle);

    return _bcm_esw_flex_stat_ext_set32(unit, type, fsh, stat, val);
}

int 
_bcm_esw_flex_stat_multi_get(int unit, _bcm_flex_stat_type_t type,
                             uint32 handle, int nstat,
                             _bcm_flex_stat_t *stat_arr,
                             uint64 *value_arr)
{
    _bcm_flex_stat_handle_t fsh;
    _BCM_FLEX_STAT_HANDLE_CLEAR(fsh);
    _BCM_FLEX_STAT_HANDLE_WORD_SET(fsh, 0, handle);

    return _bcm_esw_flex_stat_ext_multi_get(unit, type, fsh,
                                            nstat, stat_arr, value_arr);
}

int 
_bcm_esw_flex_stat_multi_get32(int unit, _bcm_flex_stat_type_t type,
                               uint32 handle, int nstat,
                               _bcm_flex_stat_t *stat_arr,
                               uint32 *value_arr)
{
    _bcm_flex_stat_handle_t fsh;
    _BCM_FLEX_STAT_HANDLE_CLEAR(fsh);
    _BCM_FLEX_STAT_HANDLE_WORD_SET(fsh, 0, handle);

    return _bcm_esw_flex_stat_ext_multi_get32(unit, type, fsh,
                                              nstat, stat_arr, value_arr);
}

int 
_bcm_esw_flex_stat_multi_set(int unit, _bcm_flex_stat_type_t type,
                             uint32 handle, int nstat,
                             _bcm_flex_stat_t *stat_arr, 
                             uint64 *value_arr)
{
    _bcm_flex_stat_handle_t fsh;
    _BCM_FLEX_STAT_HANDLE_CLEAR(fsh);
    _BCM_FLEX_STAT_HANDLE_WORD_SET(fsh, 0, handle);

    return _bcm_esw_flex_stat_ext_multi_set(unit, type, fsh,
                                            nstat, stat_arr, value_arr);
}

int 
_bcm_esw_flex_stat_multi_set32(int unit, _bcm_flex_stat_type_t type,
                               uint32 handle, int nstat,
                               _bcm_flex_stat_t *stat_arr, 
                               uint32 *value_arr)
{
    _bcm_flex_stat_handle_t fsh;
    _BCM_FLEX_STAT_HANDLE_CLEAR(fsh);
    _BCM_FLEX_STAT_HANDLE_WORD_SET(fsh, 0, handle);

    return _bcm_esw_flex_stat_ext_multi_set32(unit, type, fsh,
                                              nstat, stat_arr, value_arr);
}
#endif /* BCM_TRIUMPH2_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
/*
 * Function:
 *     _bcm_td_l2cache_from_my_station
 * Purpose:
 *     Convert a MY_STATION_TCAM entry to an L2cache API data structure
 * Parameters:
 *     unit              Unit number
 *     l2caddr           L2 cache API data structure
 *     entry             MY_STATION_TCAM entry
 */
STATIC void
_bcm_td_l2cache_from_my_station(int unit, bcm_l2_cache_addr_t *l2caddr,
                                my_station_tcam_entry_t *entry)
{
    sal_memset(l2caddr, 0, sizeof(*l2caddr));

    /* Valid bit is ignored here; entry is assumed valid */

    soc_mem_mac_addr_get(unit, MY_STATION_TCAMm, entry, MAC_ADDRf,
                         l2caddr->mac);
    soc_mem_mac_addr_get(unit, MY_STATION_TCAMm, entry, MAC_ADDR_MASKf,
                         l2caddr->mac_mask);
    l2caddr->vlan = soc_mem_field32_get(unit, MY_STATION_TCAMm, entry,
                                        VLAN_IDf);
    l2caddr->vlan_mask = soc_mem_field32_get(unit, MY_STATION_TCAMm, entry,
                                             VLAN_ID_MASKf);
    l2caddr->src_port = soc_mem_field32_get(unit, MY_STATION_TCAMm,
                                           entry, ING_PORT_NUMf);
    l2caddr->src_port_mask = soc_mem_field32_get(unit, MY_STATION_TCAMm,
                                                 entry, ING_PORT_NUM_MASKf);

    l2caddr->flags |= BCM_L2_CACHE_L3;
    if (soc_mem_field32_get(unit, MY_STATION_TCAMm, entry, DISCARDf)) {
        l2caddr->flags |= BCM_L2_CACHE_DISCARD;
    }
    if (soc_mem_field32_get(unit, MY_STATION_TCAMm, entry, COPY_TO_CPUf)) {
        l2caddr->flags |= BCM_L2_CACHE_CPU;
    }
}

/*
 * Function:
 *      _bcm_td_l2cache_to_my_station
 * Purpose:
 *     Convert an L2 API data structure to a MY_STATION_TCAM entry
 * Parameters:
 *     unit              Unit number
 *     entry             MY_STATION_TCAM entry
 *     l2caddr           L2 cache API data structure
 */
STATIC void
_bcm_td_l2cache_to_my_station(int unit, my_station_tcam_entry_t *entry,
                              bcm_l2_cache_addr_t *l2caddr)
{
    int i, entry_words;
    uint32 *l3_mask;

    l3_mask = COMMON_INFO(unit)->my_station_l3_mask.entry_data;
    entry_words = soc_mem_entry_words(unit, MY_STATION_TCAMm);

    soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, VALIDf, 1);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, VLAN_IDf,
                        l2caddr->vlan);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, VLAN_ID_MASKf,
                        l2caddr->vlan_mask);
    soc_mem_mac_addr_set(unit, MY_STATION_TCAMm, entry, MAC_ADDRf,
                         l2caddr->mac);
    soc_mem_mac_addr_set(unit, MY_STATION_TCAMm, entry, MAC_ADDR_MASKf,
                         l2caddr->mac_mask);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, ING_PORT_NUMf,
                        l2caddr->src_port);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, ING_PORT_NUM_MASKf,
                        l2caddr->src_port_mask);

    for (i = 0; i < entry_words; i++) {
        entry->entry_data[i] |= l3_mask[i];
    }

    if (l2caddr->flags & BCM_L2_CACHE_DISCARD) {
        soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, DISCARDf, 1);
    }
    if (l2caddr->flags & BCM_L2_CACHE_CPU) {
        soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, COPY_TO_CPUf, 1);
    }
}

/*
 * Function:
 *     _bcm_td_l2_from_my_station
 * Purpose:
 *     Convert a MY_STATION_TCAM entry to an L2 API data structure
 * Parameters:
 *     unit              Unit number
 *     l2addr            L2 API data structure
 *     entry             MY_STATION_TCAM entry
 */
STATIC void
_bcm_td_l2_from_my_station(int unit, bcm_l2_addr_t *l2addr,
                           my_station_tcam_entry_t *entry)
{
    sal_memset(l2addr, 0, sizeof(*l2addr));

    /* Valid bit is ignored here; entry is assumed valid */

    soc_mem_mac_addr_get(unit, MY_STATION_TCAMm, entry, MAC_ADDRf,
                         l2addr->mac);
    l2addr->vid = soc_mem_field32_get(unit, MY_STATION_TCAMm, entry, VLAN_IDf);

    if (soc_mem_field32_get(unit, MY_STATION_TCAMm, entry,
                            IPV4_TERMINATION_ALLOWEDf)) {
        l2addr->flags |= BCM_L2_L3LOOKUP;
    }
    if (soc_mem_field32_get(unit, MY_STATION_TCAMm, entry, DISCARDf)) {
        l2addr->flags |= BCM_L2_DISCARD_DST;
    }
    if (soc_mem_field32_get(unit, MY_STATION_TCAMm, entry, COPY_TO_CPUf)) {
        l2addr->flags |= BCM_L2_COPY_TO_CPU;
    }
    l2addr->flags |= BCM_L2_LEARN_LIMIT_EXEMPT;
}

/*
 * Function:
 *     _bcm_td_l2_to_my_station
 * Purpose:
 *     Convert an L2 API data structure to a MY_STATION_TCAM entry
 * Parameters:
 *     unit              Unit number
 *     entry             MY_STATION_TCAM entry
 *     l2addr            L2 API data structure
 *     incl_key_mask     Update key and mask fields as well
 */
STATIC void
_bcm_td_l2_to_my_station(int unit, my_station_tcam_entry_t *entry,
                         bcm_l2_addr_t *l2addr, int incl_key_mask)
{
    bcm_mac_t mac_mask;
    uint32 fval;
    int i, entry_words;
    uint32 *l3_mask;

    l3_mask = COMMON_INFO(unit)->my_station_l3_mask.entry_data;
    entry_words = soc_mem_entry_words(unit, MY_STATION_TCAMm);

    if (incl_key_mask) {
        soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, VALIDf, 1);
        soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, VLAN_IDf,
                            l2addr->vid);
        soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, VLAN_ID_MASKf,
                            0xfff);
        soc_mem_mac_addr_set(unit, MY_STATION_TCAMm, entry, MAC_ADDRf,
                             l2addr->mac);
        sal_memset(&mac_mask, 0xff, sizeof(mac_mask));
        soc_mem_mac_addr_set(unit, MY_STATION_TCAMm, entry, MAC_ADDR_MASKf,
                             mac_mask);
    }

    for (i = 0; i < entry_words; i++) {
        entry->entry_data[i] |= l3_mask[i];
    }

    fval = l2addr->flags & BCM_L2_DISCARD_DST ? 1 : 0;
    soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, DISCARDf, fval);

    fval = l2addr->flags & BCM_L2_COPY_TO_CPU ? 1 : 0;
    soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, COPY_TO_CPUf, fval);
}

/*
 * Function:
 *     _bcm_td_my_station_lookup
 * Purpose:
 *     Perform following exact matched in MY_STATION_TCAM table:
 *     - target entry lookup:
 *       match the entry specified by vlan/mac and optional port
 *     - (optional) alternate entry lookup: (when alt_index != NULL)
 *       match the possible entry having the same vlan/mac as the target entry
 * Parameters:
 *     unit              Unit number
 *     mac               MAC address key
 *     vlan              VLAN id key
 *     port              Source port number key (-1 means to match any)
 *     index_to_be_skip  The index to be omit during lookup
 *     entry_index       (OUT) target entry index (if BCM_E_NONE)
 *                             available entry index or -1 (if BCM_E_NOT_FOUND)
 *     alt_index         (OUT) alternate entry index or -1 (if BCM_E_NOT_FOUND)
 *                       if alt_index is NULL, that means lookup only
 * Returns:
 *      BCM_E_NONE - target entry found
 *      BCM_E_NOT_FOUND - target entry not found
 * Notes:
 *     if (target entry is found) {
 *         entry_index: index for the matched target entry
 *         alt_index: N/A
 *     } else if (alternate entry is found) {
 *         entry_index: index for the free entry to be used (-1 if table full)
 *         alt_index: index for the matched alternate entry
 *         *** caller needs to compare both indices, move the entry if needed
 *     } else {
 *         entry_index: index for the free entry to be used (-1 if table full)
 *         alt_index: -1
 *     }
 */
STATIC int
_bcm_td_my_station_lookup(int unit, bcm_mac_t mac, bcm_vlan_t vlan,
                          bcm_port_t port, int index_to_skip,
                          int *entry_index, int *alt_index)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    my_station_tcam_entry_t *entry, entry0, entry1, entry2, mask0, mask1;
    int index0, index1, free_index, i, entry_words;
    int start, end, step;
    bcm_mac_t mac_mask;
    uint32 port_mask;

    BCM_DEBUG(BCM_DBG_L2,
              ("_bcm_td_my_station_lookup: unit=%d "
               "mac=%02x:%02x:%02x:%02x:%02x:%02x vlan=%d port=%d "
               "index_to_skip=%d\n",
               unit, vlan, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
               port, index_to_skip));

    sal_memset(&mac_mask, 0xff, sizeof(mac_mask));
    port_mask =
        (1 << soc_mem_field_length(unit, MY_STATION_TCAMm, ING_PORT_NUMf)) - 1;

    entry_words = soc_mem_entry_words(unit, MY_STATION_TCAMm);

    /*
     * entry0, mask0 are for target entry
     * entry1, mask1 are for alternate entry
     * if (port == -1)
     *     entry0 is: vlan/mac/00/fff/ffffffffffff/ff
     *     entry1 is: vlan/mac/xx/fff/ffffffffffff/ff
     *     start from last entry (free index needs to have larger index than
     *     any alternate entry)
     * else
     *     entry0 is: vlan/mac/port/fff/ffffffffffff/ff
     *     entry1 is: vlan/mac/00/fff/ffffffffffff/ff
     *     start from first entry (free index needs to have smaller index than
     *     the alternate entry)
     */
    sal_memset(&entry0, 0, sizeof(entry0));
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &entry0, VALIDf, 1);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &entry0, VLAN_IDf, vlan);
    soc_mem_mac_addr_set(unit, MY_STATION_TCAMm, &entry0, MAC_ADDRf, mac);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &entry0, VLAN_ID_MASKf, 0xfff);
    soc_mem_mac_addr_set(unit, MY_STATION_TCAMm, &entry0, MAC_ADDR_MASKf,
                         mac_mask);
    mask0 = entry0;
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &mask0, VLAN_IDf, 0xfff);
    soc_mem_mac_addr_set(unit, MY_STATION_TCAMm, &mask0, MAC_ADDRf, mac_mask);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &mask0, ING_PORT_NUMf,
                        port_mask);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &mask0, ING_PORT_NUM_MASKf,
                        port_mask);
    if (alt_index != NULL) {
        entry1 = entry0;
        mask1 = mask0;
    }

    if (port == -1) {
        start = soc_mem_index_max(unit, MY_STATION_TCAMm);
        end = -1;
        step = -1;
    } else {
        soc_mem_field32_set(unit, MY_STATION_TCAMm, &entry0, ING_PORT_NUMf,
                            port);
        soc_mem_field32_set(unit, MY_STATION_TCAMm, &entry0,
                            ING_PORT_NUM_MASKf, port_mask);
        start = 0;
        end = soc_mem_index_count(unit, MY_STATION_TCAMm);
        step = 1;
    }

    if (alt_index != NULL) {
        if (port == -1) {
            soc_mem_field32_set(unit, MY_STATION_TCAMm, &mask1, ING_PORT_NUMf,
                                0);
        }

        /* entry2 is for checking VALID bit */
        sal_memset(&entry2, 0, sizeof(entry2));
        soc_mem_field32_set(unit, MY_STATION_TCAMm, &entry2, VALIDf, 1);
    }

    index1 = -1;
    free_index = -1;
    for (index0 = start; index0 != end; index0 += step) {
        /* Skip the entry that we want to move (we know it matches) */
        if (index0 == index_to_skip) {
            continue;
        }

        entry = &info->my_station_shadow[index0];

        /* Try matching target entry */
        for (i = 0; i < entry_words; i++) {
            if ((entry->entry_data[i] ^ entry0.entry_data[i]) &
                mask0.entry_data[i]) {
                break;
            }
        }
        if (i == entry_words) {
            /* Target entry is found, no more action needed */
            *entry_index = index0;
            BCM_DEBUG(BCM_DBG_L2,
                      ("_bcm_td_my_station_lookup: found entry_index=%d\n",
                       *entry_index));
            return BCM_E_NONE;
        }

        if (alt_index == NULL) { /* lookup only */
            continue;
        }

        /* Keep track index of the first free entry */
        if (free_index == -1) {
            for (i = 0; i < entry_words; i++) {
                if (entry->entry_data[i] & entry2.entry_data[i]) {
                    break;
                }
            }
            if (i == entry_words) {
                if (index1 != -1) {
                    /* Both free entry and alternate entry are found */
                    *entry_index = index0;
                    *alt_index = index1;
                    BCM_DEBUG(BCM_DBG_L2,
                              ("_bcm_td_my_station_lookup: not found "
                               "entry_index=%d alt_index=%d\n",
                               *entry_index, *alt_index));
                    return BCM_E_NOT_FOUND;
                } else {
                    free_index = index0;
                    continue;
                }
            }
        }

        /* Try matching alternate entry. If alternate entry is found, it
         * implies target entry is not in the table */
        for (i = 0; i < entry_words; i++) {
            if ((entry->entry_data[i] ^ entry1.entry_data[i]) &
                mask1.entry_data[i]) {
                break;
            }
        }
        if (i == entry_words) {
            if (free_index != -1) {
                /* both free entry and alternate entry are found */
                *entry_index = free_index;
                *alt_index = index0;
                BCM_DEBUG(BCM_DBG_L2,
                          ("_bcm_td_my_station_lookup: not found "
                           "entry_index=%d alt_index=%d\n",
                           *entry_index, *alt_index));
                return BCM_E_NOT_FOUND;
            } else {
                index1 = index0;
                continue;
            }
        }
    }

    *entry_index = free_index;
    if (alt_index != NULL) {
        *alt_index = index1;
    }
    BCM_DEBUG(BCM_DBG_L2,
              ("_bcm_td_my_station_lookup: not found "
                               "entry_index=%d alt_index=%d\n",
               *entry_index, alt_index != NULL ? *alt_index : -100));

    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *     bcm_td_l2_myStation_add
 * Purpose:
 *     Add MY_STATION_TCAM entry by L2 API
 * Parameters:
 *     unit              Unit number
 *     l2addr            L2 API data structure
 */
int
bcm_td_l2_myStation_add(int unit, bcm_l2_addr_t *l2addr)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    int rv, index, alt_index;
    my_station_tcam_entry_t *entry;
    l2u_entry_t l2u_entry;

    VLAN_CHK_ID(unit, l2addr->vid);

    soc_mem_lock(unit, MY_STATION_TCAMm);

    rv = _bcm_td_my_station_lookup(unit, l2addr->mac, l2addr->vid, -1, -1,
                                   &index, &alt_index);
    if (BCM_SUCCESS(rv)) {
#if defined(BCM_TRIDENT_SUPPORT)
        if(SOC_IS_TRIDENT(unit)) {
            /* Check if the entry is not added by l2cache API */
            SOC_IF_ERROR_RETURN(soc_l2u_get(unit, &l2u_entry, index));
            if ((SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, L3f) &&
                 soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry, L3f)) ||
                (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, RESERVED_0f) &&
                 soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry,
                                 RESERVED_0f))) {
                soc_mem_unlock(unit, MY_STATION_TCAMm);
                return BCM_E_EXISTS;
            }
        }
#endif
        /* Entry exist, update the entry */
        entry = &info->my_station_shadow[index];
        _bcm_td_l2_to_my_station(unit, entry, l2addr, FALSE);
        rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index,
                           entry);
    } else if (rv == BCM_E_NOT_FOUND && index == -1) {
        /* No free entry available for insertion */
        rv = BCM_E_FULL;
    }
    if (rv != BCM_E_NOT_FOUND) {
        soc_mem_unlock(unit, MY_STATION_TCAMm);
        return rv;
    }

    if (alt_index > index) {
        /* Free entry is available for new entry insertion and alternate entry
         * exists. However need to swap these 2 entries to maintain proper
         * lookup precedence */
        info->my_station_shadow[index] = info->my_station_shadow[alt_index];
        rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index,
                           &info->my_station_shadow[index]);
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, MY_STATION_TCAMm);
            return rv;
        }
        index = alt_index;
    }

    /* Add the new entry */
    entry = &info->my_station_shadow[index];
    sal_memset(entry, 0, sizeof(*entry));
    _bcm_td_l2_to_my_station(unit, entry, l2addr, TRUE);
    rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index, entry);

    soc_mem_unlock(unit, MY_STATION_TCAMm);
    return rv;
}

/*
 * Function:
 *     bcm_td_l2_myStation_delete
 * Purpose:
 *     Delete MY_STATION_TCAM entry added by L2 API
 * Parameters:
 *     unit              Unit number
 *     mac               MAC address
 *     vlan              VLAN ID
 */
int
bcm_td_l2_myStation_delete(int unit, bcm_mac_t mac, bcm_vlan_t vlan,
                           int *l2_index)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    int rv, index, i, entry_words;
    my_station_tcam_entry_t *entry;
    l2u_entry_t l2u_entry;
    uint32 *l3_mask, *tunnel_mask;

    VLAN_CHK_ID(unit, vlan);

    l3_mask = info->my_station_l3_mask.entry_data;
    tunnel_mask = info->my_station_tunnel_mask.entry_data;
    entry_words = soc_mem_entry_words(unit, MY_STATION_TCAMm);

    soc_mem_lock(unit, MY_STATION_TCAMm);

    rv = _bcm_td_my_station_lookup(unit, mac, vlan, -1, -1, &index, NULL);
    if (BCM_SUCCESS(rv)) {
        entry = &info->my_station_shadow[index];

        /* Check if the entry has L3 related flag enabled */
        for (i = 0; i < entry_words; i++) {
            if (entry->entry_data[i] & l3_mask[i]) {
                break;
            }
        }
        if (i == entry_words) {
            soc_mem_unlock(unit, MY_STATION_TCAMm);
            return BCM_E_NOT_FOUND;
        }

#if defined(BCM_TRIDENT_SUPPORT)
        if(SOC_IS_TRIDENT(unit)) {
            /* Check if the entry is not added by l2cache API */
            SOC_IF_ERROR_RETURN(soc_l2u_get(unit, &l2u_entry, index));
            if ((SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, L3f) &&
                 soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry, L3f)) ||
                (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, RESERVED_0f) &&
                 soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry,
                                 RESERVED_0f))) {
                soc_mem_unlock(unit, MY_STATION_TCAMm);
                return BCM_E_NOT_FOUND;
            }
        }
#endif
        /* Check to see if we need to delete or modify the entry */
        for (i = 0; i < entry_words; i++) {
            if (entry->entry_data[i] & tunnel_mask[i]) {
                break;
            }
        }
        if (i == entry_words) {
            /* Delete the entry if no tunnel related flag is enabled */
            sal_memset(entry, 0, sizeof(*entry));
        } else {
            /* Modify the entry if any tunnel related flag is enabled */
            for (i = 0; i < entry_words; i++) {
                entry->entry_data[i] &= ~l3_mask[i];
            }
            soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, DISCARDf, 0);
            soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, COPY_TO_CPUf,
                                0);
        }
        rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index,
                           entry);
    }

    soc_mem_unlock(unit, MY_STATION_TCAMm);
    return rv;
}

/*
 * Function:
 *     bcm_td_l2_myStation_get
 * Purpose:
 *     Get MY_STATION_TCAM entry added by L2 API
 * Parameters:
 *     unit              Unit number
 *     mac               MAC address
 *     vlan              VLAN ID
 */
int
bcm_td_l2_myStation_get(int unit, bcm_mac_t mac, bcm_vlan_t vlan,
                        bcm_l2_addr_t *l2addr)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    int index, i, entry_words;
    my_station_tcam_entry_t *entry;
    l2u_entry_t l2u_entry;
    uint32 *l3_mask;

    VLAN_CHK_ID(unit, vlan);

    l3_mask = info->my_station_l3_mask.entry_data;
    entry_words = soc_mem_entry_words(unit, MY_STATION_TCAMm);

    BCM_IF_ERROR_RETURN
        (_bcm_td_my_station_lookup(unit, mac, vlan, -1, -1, &index, NULL));

    entry = &info->my_station_shadow[index];

    /* Check if the entry has L3 related flag enabled */
    for (i = 0; i < entry_words; i++) {
        if (entry->entry_data[i] & l3_mask[i]) {
            break;
        }
    }
    if (i == entry_words) {
        return BCM_E_NOT_FOUND;
    }

#if defined(BCM_TRIDENT_SUPPORT)
    if(SOC_IS_TRIDENT(unit)) {
        /* Check if the entry is not added by l2cache API */
        SOC_IF_ERROR_RETURN(soc_l2u_get(unit, &l2u_entry, index));
        if ((SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, L3f) &&
             soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry, L3f)) ||
            (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, RESERVED_0f) &&
             soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry,
                             RESERVED_0f))) {
            return BCM_E_NOT_FOUND;
        }
    }
#endif

    _bcm_td_l2_from_my_station(unit, l2addr, entry);

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_td_l2cache_myStation_set
 * Purpose:
 *     Add MY_STATION_TCAM entry from L2 cache API
 * Parameters:
 *     unit              Unit number
 *     index             Index
 *     l2caddr           L2 cache API data structure
 */
int
bcm_td_l2cache_myStation_set(int unit, int index, bcm_l2_cache_addr_t *l2caddr)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    int rv, entry_index, alt_index;
    my_station_tcam_entry_t *entry;
    l2u_entry_t l2u_entry;
    bcm_vlan_t vlan;
    bcm_mac_t mac;
    bcm_port_t port;

    VLAN_CHK_ID(unit, l2caddr->vlan);

    soc_mem_lock(unit, MY_STATION_TCAMm);

    entry_index = -1;
    entry = &info->my_station_shadow[index];
    if (!soc_mem_field32_get(unit, MY_STATION_TCAMm, entry, VALIDf)) {
        /* Entry is not in used */
        entry_index = index;
    } else {
#if defined(BCM_TRIDENT_SUPPORT)
        if(SOC_IS_TRIDENT(unit)) {
            /* Check if the existing entry is added by l2cache API */
            rv = soc_l2u_get(unit, &l2u_entry, index);
            if (BCM_SUCCESS(rv) &&
                ((SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, L3f) &&
                  soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry, L3f)) ||
                 (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, RESERVED_0f) &&
                  soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry,
                                  RESERVED_0f)))) {
                entry_index = index;
            }
        }
#endif
    }
    if (entry_index != -1) {
        /* The entry is not used by API other than l2cache */
        sal_memset(entry, 0, sizeof(*entry));
        _bcm_td_l2cache_to_my_station(unit, entry, l2caddr);
        rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index,
                           entry);
        soc_mem_unlock(unit, MY_STATION_TCAMm);
        return rv;
    }

    /* Need to move the current entry */
    vlan = soc_mem_field32_get(unit, MY_STATION_TCAMm, entry, VLAN_IDf);
    soc_mem_mac_addr_get(unit, MY_STATION_TCAMm, entry, MAC_ADDRf, mac);
    if (soc_mem_field32_get(unit, MY_STATION_TCAMm, entry,
                            ING_PORT_NUM_MASKf) == 0) {
        port = -1;
    } else {
        port = soc_mem_field32_get(unit, MY_STATION_TCAMm, entry,
                                   ING_PORT_NUMf);
    }
    rv = _bcm_td_my_station_lookup(unit, mac, vlan, port, index, &entry_index,
                                   &alt_index);
    if (rv == BCM_E_NOT_FOUND && entry_index == -1) {
        /* No free entry available for insertion */
        rv = BCM_E_FULL;
    }
    if (rv != BCM_E_NOT_FOUND) {
        soc_mem_unlock(unit, MY_STATION_TCAMm);
        return rv;
    }

    if (alt_index != -1 &&
        ((port == -1 && alt_index > entry_index) ||
         (port != -1 && alt_index < entry_index))) {
        /* Free entry is available for moving the target entry and alternate
         * entry exists. However need to swap these 2 entries to maintain
         * proper lookup precedence */
        info->my_station_shadow[entry_index] =
            info->my_station_shadow[alt_index];
        rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, entry_index,
                           &info->my_station_shadow[entry_index]);
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, MY_STATION_TCAMm);
            return rv;
        }
        entry_index = alt_index;
    }

    /* Move the entry at specified index to the available free location */
    info->my_station_shadow[entry_index] = *entry;
    rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, entry_index,
                       entry);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, MY_STATION_TCAMm);
        return rv;
    }

    /* Add the new entry */
    sal_memset(entry, 0, sizeof(*entry));
    _bcm_td_l2cache_to_my_station(unit, entry, l2caddr);
    rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index,
                       entry);

    soc_mem_unlock(unit, MY_STATION_TCAMm);
    return rv;
}

/*
 * Function:
 *     bcm_td_l2cache_myStation_get
 * Purpose:
 *     Get MY_STATION_TCAM entry added by L2 cache API
 * Parameters:
 *     unit              Unit number
 *     index             Index
 *     l2caddr           L2 cache API data structure
 */
int
bcm_td_l2cache_myStation_get(int unit, int index, bcm_l2_cache_addr_t *l2caddr)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    int i, entry_words;
    my_station_tcam_entry_t *entry;
    l2u_entry_t l2u_entry;
    uint32 *l3_mask;

    l3_mask = info->my_station_l3_mask.entry_data;
    entry_words = soc_mem_entry_words(unit, MY_STATION_TCAMm);

    entry = &info->my_station_shadow[index];

    /* Check if the entry has L3 related flag enabled */
    for (i = 0; i < entry_words; i++) {
        if (entry->entry_data[i] & l3_mask[i]) {
            break;
        }
    }
    if (i == entry_words) {
        return BCM_E_NOT_FOUND;
    }

    /* Check if the entry is valid */
    if (!soc_mem_field32_get(unit, MY_STATION_TCAMm, entry, VALIDf)) {
        return BCM_E_NOT_FOUND;
    }

#if defined(BCM_TRIDENT_SUPPORT)
    if(SOC_IS_TRIDENT(unit)) {
        /* Check if the entry is added by l2cache API */
        SOC_IF_ERROR_RETURN(soc_l2u_get(unit, &l2u_entry, index));
        if (!((SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, L3f) &&
               soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry, L3f)) ||
              (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, RESERVED_0f) &&
               soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry,    
                               RESERVED_0f)))) {
            return BCM_E_NOT_FOUND;
        }
    }
#endif

    _bcm_td_l2cache_from_my_station(unit, l2caddr, entry);

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_td_l2cache_myStation_delete
 * Purpose:
 *     Delete MY_STATION_TCAM entry added by L2 cache API
 * Parameters:
 *     unit              Unit number
 *     index             Index
 */
int
bcm_td_l2cache_myStation_delete(int unit, int index)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    int rv, i, entry_words;
    my_station_tcam_entry_t *entry;
    l2u_entry_t l2u_entry;
    uint32 *l3_mask, *tunnel_mask;

    l3_mask = info->my_station_l3_mask.entry_data;
    tunnel_mask = info->my_station_tunnel_mask.entry_data;
    entry_words = soc_mem_entry_words(unit, MY_STATION_TCAMm);

    soc_mem_lock(unit, MY_STATION_TCAMm);

    entry = &info->my_station_shadow[index];

    /* Check if the entry has L3 related flag enabled */
    for (i = 0; i < entry_words; i++) {
        if (entry->entry_data[i] & l3_mask[i]) {
            break;
        }
    }
    if (i == entry_words) {
        soc_mem_unlock(unit, MY_STATION_TCAMm);
        return BCM_E_NOT_FOUND;
    }

    /* Check if the entry is valid */
    if (!soc_mem_field32_get(unit, MY_STATION_TCAMm, entry, VALIDf)) {
        soc_mem_unlock(unit, MY_STATION_TCAMm);
        return BCM_E_NOT_FOUND;
    }

#if defined(BCM_TRIDENT_SUPPORT)
    if(SOC_IS_TRIDENT(unit)) {
        /* Check if the entry is added by l2cache API */
        SOC_IF_ERROR_RETURN(soc_l2u_get(unit, &l2u_entry, index));
        if (!((SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, L3f) &&
               soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry, L3f)) ||
              (SOC_MEM_FIELD_VALID(unit, L2_USER_ENTRYm, RESERVED_0f) &&
               soc_mem_field32_get(unit, L2_USER_ENTRYm, &l2u_entry,
                               RESERVED_0f)))) {
            soc_mem_unlock(unit, MY_STATION_TCAMm);
            return BCM_E_NOT_FOUND;
        }
    }
#endif
    /* Check to see if we need to delete or modify the entry */
    for (i = 0; i < entry_words; i++) {
        if (entry->entry_data[i] & tunnel_mask[i]) {
            break;
        }
    }
    if (i == entry_words) {
        /* Delete the entry if no tunnel related flag is enabled */
        sal_memset(entry, 0, sizeof(*entry));
    } else {
        /* Modify the entry if any tunnel related flag is enabled */
        for (i = 0; i < entry_words; i++) {
            entry->entry_data[i] &= ~l3_mask[i];
        }
        soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, DISCARDf, 0);
        soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, COPY_TO_CPUf, 0);
    }
    rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index, entry);

    soc_mem_unlock(unit, MY_STATION_TCAMm);
    return rv;
}

/*
 * Function:
 *     bcm_td_l2cache_myStation_lookup
 * Purpose:
 *     Search MY_STATION_TCAM for entry added by L2 cache API
 * Parameters:
 *     unit              Unit number
 *     l2caddr           L2 cache API data structure
 *     result_index      Index
 */
int
bcm_td_l2cache_myStation_lookup(int unit, bcm_l2_cache_addr_t *l2caddr,
                             int *result_index)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    int index, i, num_entries, entry_words;
    my_station_tcam_entry_t *entry, entry0, mask0;
    uint32 *l3_mask;
    bcm_mac_t mac_mask;
    uint32 port_mask;

    sal_memset(&mac_mask, 0xff, sizeof(mac_mask));
    port_mask =
        (1 << soc_mem_field_length(unit, MY_STATION_TCAMm, ING_PORT_NUMf)) - 1;

    l3_mask = info->my_station_l3_mask.entry_data;
    num_entries = soc_mem_index_count(unit, MY_STATION_TCAMm);
    entry_words = soc_mem_entry_words(unit, MY_STATION_TCAMm);

    sal_memset(&entry0, 0, sizeof(entry0));
    _bcm_td_l2cache_to_my_station(unit, &entry0, l2caddr);
    sal_memset(&mask0, 0, sizeof(mask0));
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &mask0, VALIDf, 1);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &mask0, VLAN_IDf,
                        l2caddr->vlan_mask);
    soc_mem_mac_addr_set(unit, MY_STATION_TCAMm, &mask0, MAC_ADDRf,
                         l2caddr->mac_mask);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &mask0, ING_PORT_NUMf,
                        l2caddr->src_port_mask);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &mask0, VLAN_ID_MASKf, 0xfff);
    soc_mem_mac_addr_set(unit, MY_STATION_TCAMm, &mask0, MAC_ADDR_MASKf,
                         mac_mask);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &mask0, ING_PORT_NUM_MASKf,
                        port_mask);
    for (i = 0; i < entry_words; i++) {
        mask0.entry_data[i] |= l3_mask[i];
    }

    for (index = 0; index < num_entries; index++) {
        entry = &info->my_station_shadow[index];

        /* Try matching the entry */
        for (i = 0; i < entry_words; i++) {
            if ((entry->entry_data[i] ^ entry0.entry_data[i]) &
                mask0.entry_data[i]) {
                break;
            }
        }
        if (i == entry_words) {
            *result_index = index;
            return BCM_E_NONE;
        }
    }

    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *      bcm_td_metro_myStation_add
 * Purpose:
 *     Add a MY_STATION_TCAM entry from L2 tunnel API
 *     Shared by MPLS, MIM and TRILL
 * Parameters:
 *     unit              Unit number
 *     mac               MAC address
 *     vlan              VLAN ID
 *     port              Source port number
 */
int
bcm_td_metro_myStation_add(int unit, bcm_mac_t mac, bcm_vlan_t vlan,
                           bcm_port_t port)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    int rv, index, alt_index, i, entry_words;
    my_station_tcam_entry_t *entry;
    uint32 *tunnel_mask;
    bcm_mac_t mac_mask;
    uint32 port_mask;

    tunnel_mask = info->my_station_tunnel_mask.entry_data;
    entry_words = soc_mem_entry_words(unit, MY_STATION_TCAMm);

    soc_mem_lock(unit, MY_STATION_TCAMm);

    rv = _bcm_td_my_station_lookup(unit, mac, vlan, port, -1, &index,
                                   &alt_index);
    if (BCM_SUCCESS(rv)) {
        /* Entry exist, update the entry */
        entry = &info->my_station_shadow[index];
        for (i = 0; i < entry_words; i++) {
            entry->entry_data[i] |= tunnel_mask[i];
        }
        rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index,
                           entry);
    } else if (rv == BCM_E_NOT_FOUND && index == -1) {
        /* No free entry available for insertion */
        rv = BCM_E_FULL;
    }
    if (rv != BCM_E_NOT_FOUND) {
        soc_mem_unlock(unit, MY_STATION_TCAMm);
        return rv;
    }

    if (alt_index != -1 &&
        ((port == -1 && alt_index > index) ||
         (port != -1 && alt_index < index))) {
        /* Free entry is available for new entry insertion and alternate entry
         * exists. However need to swap these 2 entries to maintain proper
         * lookup precedence */
        rv = soc_mem_read(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, alt_index,
                          &info->my_station_shadow[index]);
        if (BCM_SUCCESS(rv)) {
            rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index,
                               &info->my_station_shadow[index]);
        }
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, MY_STATION_TCAMm);
            return rv;
        }
        index = alt_index;
    }

    /* Add the new entry */
    entry = &info->my_station_shadow[index];
    sal_memset(entry, 0, sizeof(*entry));
    soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, VALIDf, 1);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, VLAN_IDf, vlan);
    soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, VLAN_ID_MASKf, 0xfff);
    soc_mem_mac_addr_set(unit, MY_STATION_TCAMm, entry, MAC_ADDRf, mac);
    sal_memset(&mac_mask, 0xff, sizeof(mac_mask));
    soc_mem_mac_addr_set(unit, MY_STATION_TCAMm, entry, MAC_ADDR_MASKf,
                         mac_mask);
    if (port != -1) {
        soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, ING_PORT_NUMf,
                            port);
        port_mask = (1 << soc_mem_field_length(unit, MY_STATION_TCAMm,
                                               ING_PORT_NUMf)) - 1;
        soc_mem_field32_set(unit, MY_STATION_TCAMm, entry, ING_PORT_NUM_MASKf,
                            port_mask);
    }
    for (i = 0; i < entry_words; i++) {
        entry->entry_data[i] |= tunnel_mask[i];
    }
    rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index, entry);

    soc_mem_unlock(unit, MY_STATION_TCAMm);
    return rv;
}

/*
 * Function:
 *    bcm_td_metro_myStation_delete
 * Purpose:
 *     Delete a MY_STATION_TCAM entry added by L2 tunnel API
 *     Shared by MPLS, MIM and TRILL
 * Parameters:
 *     unit              Unit number
 *     mac               MAC address
 *     vlan              VLAN ID
 *     port              Source port number
 */
int
bcm_td_metro_myStation_delete(int unit, bcm_mac_t mac, bcm_vlan_t vlan,
                              bcm_port_t port)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    int rv, index, i, entry_words;
    my_station_tcam_entry_t *entry;
    uint32 *l3_mask, *tunnel_mask;

    l3_mask = info->my_station_l3_mask.entry_data;
    tunnel_mask = info->my_station_tunnel_mask.entry_data;
    entry_words = soc_mem_entry_words(unit, MY_STATION_TCAMm);

    soc_mem_lock(unit, MY_STATION_TCAMm);

    rv = _bcm_td_my_station_lookup(unit, mac, vlan, port, -1, &index, NULL);
    if (BCM_SUCCESS(rv)) {
        entry = &info->my_station_shadow[index];

        /* Check if the entry has tunnel related flag enabled */
        for (i = 0; i < entry_words; i++) {
            if (entry->entry_data[i] & tunnel_mask[i]) {
                break;
            }
        }
        if (i == entry_words) {
            soc_mem_unlock(unit, MY_STATION_TCAMm);
            return BCM_E_NOT_FOUND;
        }

        /* Check to see if we need to delete or modify the entry */
        for (i = 0; i < entry_words; i++) {
            if (entry->entry_data[i] & l3_mask[i]) {
                break;
            }
        }
        if (i == entry_words) {
            /* Delete the entry if no L3 related flag is enabled */
            sal_memset(entry, 0, sizeof(*entry));
        } else {
            /* Modify the entry if any L3 related flag is enabled */
            for (i = 0; i < entry_words; i++) {
                entry->entry_data[i] &= ~tunnel_mask[i];
            }
        }
        rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index,
                           entry);
    }

    soc_mem_unlock(unit, MY_STATION_TCAMm);
    return rv;
}

/*
 * Function:
 *     bcm_td_metro_myStation_delete_all
 * Purpose:
 *     Delete all MY_STATION_TCAM entries added by L2 tunnel API
 *     Shared by MPLS, MIM and TRILL
 * Parameters:
 *     unit              Unit number
 */
int
bcm_td_metro_myStation_delete_all(int unit)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    int rv, index, i, num_entries, entry_words;
    my_station_tcam_entry_t *entry, valid_mask;
    uint32 *l3_mask, *tunnel_mask;

    sal_memset(&valid_mask, 0, sizeof(valid_mask));
    soc_mem_field32_set(unit, MY_STATION_TCAMm, &valid_mask, VALIDf, 1);

    l3_mask = info->my_station_l3_mask.entry_data;
    tunnel_mask = info->my_station_tunnel_mask.entry_data;
    num_entries = soc_mem_index_count(unit, MY_STATION_TCAMm);
    entry_words = soc_mem_entry_words(unit, MY_STATION_TCAMm);

    soc_mem_lock(unit, MY_STATION_TCAMm);

    rv = BCM_E_NONE;
    for (index = 0; index < num_entries; index++) {
        entry = &info->my_station_shadow[index];

        /* Check if the entry has tunnel related flag enabled */
        for (i = 0; i < entry_words; i++) {
            if (entry->entry_data[i] & tunnel_mask[i]) {
                break;
            }
        }
        if (i == entry_words) {
            continue;
        }

        /* Check if the entry is valid */
        for (i = 0; i < entry_words; i++) {
            if (entry->entry_data[i] & valid_mask.entry_data[i]) {
                break;
            }
        }
        if (i == entry_words) {
            continue;
        }

        /* Check to see if we need to delete or modify the entry */
        for (i = 0; i < entry_words; i++) {
            if (entry->entry_data[i] & l3_mask[i]) {
                break;
            }
        }
        if (i == entry_words) {
            /* Delete the entry if no L3 related flag is enabled */
            sal_memset(entry, 0, sizeof(*entry));
        } else {
            /* Modify the entry if any L3 related flag is enabled */
            for (i = 0; i < entry_words; i++) {
                entry->entry_data[i] &= ~tunnel_mask[i];
            }
        }
        rv = soc_mem_write(unit, MY_STATION_TCAMm, MEM_BLOCK_ALL, index,
                           entry);
    }

    soc_mem_unlock(unit, MY_STATION_TCAMm);
    return rv;
}
#endif /* BCM_TRIDENT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_common_sw_dump
 * Purpose:
 *     Displays common data structure information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 * Note:
 */
void
_bcm_common_sw_dump(int unit)
{
    _bcm_common_bookkeeping_t *info = COMMON_INFO(unit);
    soc_mem_t        mem;
    int              i, j, rv;
    int              num_entries;
    int              ref_count;

    soc_cm_print("\nSW Information Common - Unit %d\n", unit);

    if (!info->initialized) {
        soc_cm_print("  Common data structures uninitialized!\n");
    }

    /* MPLS station hash */
    if (NULL != info->mpls_station_hash) {
        if (SOC_MEM_IS_VALID(unit, MY_STATION_TCAMm)) {
            num_entries = soc_mem_index_count(unit, MY_STATION_TCAMm);
        } else {
            num_entries = soc_mem_index_count(unit, MPLS_STATION_TCAMm);
        }

        soc_cm_print("\n  MPLS station index hashes (%d)\n", num_entries);
        soc_cm_print("    Index     Hash\n");
        ref_count = 0;
        for (i = 0; i < num_entries; i++) {
            if (0 != MPLS_STATION_HASH(unit, i)){
                soc_cm_print("  %3d      0x%04x\n",
                             i, MPLS_STATION_HASH(unit, i));
                ref_count++;
            }
        }
        if (0 == ref_count) {
            soc_cm_print("    All indexes hash to zero.\n");
        }
        soc_cm_print("\n");
    }

    /* MAC_DA_PROFILE table */
    if (NULL != info->mac_da_profile) {
        egr_mac_da_profile_entry_t *mac_da_entry_p;
        bcm_mac_t        l2mac;

        mem = EGR_MAC_DA_PROFILEm;
        num_entries = soc_mem_index_count(unit, mem);
        soc_cm_print("  MAC DA Profile Mem\n");
        soc_cm_print("    Number of entries: %d\n", num_entries);
        soc_cm_print("    Index RefCount - MAC address\n");

        for (i = 0; i < num_entries; i++) {
            rv = soc_profile_mem_ref_count_get(unit,
                                               info->mac_da_profile,
                                               i, &ref_count);
            if (SOC_FAILURE(rv)) {
                soc_cm_print
                    (" *** Error retrieving profile reference: %d ***\n",
                     rv);
                break;
            }

            if (ref_count <= 0) {
                continue;
            }

            mac_da_entry_p =
                SOC_PROFILE_MEM_ENTRY(unit, info->mac_da_profile,
                                      egr_mac_da_profile_entry_t *, i);

            soc_mem_mac_addr_get(unit, mem, mac_da_entry_p,
                                 MAC_ADDRESSf, l2mac);
            soc_cm_print("  %5d %8d    0x%02x%02x%02x%02x%02x%02x\n",
                         i, ref_count,
                         l2mac[0], l2mac[1], l2mac[2],
                         l2mac[3], l2mac[4], l2mac[5]);
        }
        soc_cm_print("\n");
    }

    /* LPORT_PROFILE table */
    if (NULL != info->lport_profile) {
        lport_tab_entry_t *lport_entry_p;

        mem = LPORT_TABm;
        num_entries = soc_mem_index_count(unit, mem);
        soc_cm_print("  LPORT_PROFILE_TABLE Mem\n");
        soc_cm_print("    Number of entries: %d\n", num_entries);
        soc_cm_print("    Index RefCount - LPORT_TAB entry\n");

        for (i = 0; i < num_entries; i++) {
            rv = soc_profile_mem_ref_count_get(unit,
                                               info->lport_profile,
                                               i, &ref_count);
            if (SOC_FAILURE(rv)) {
                soc_cm_print
                    (" *** Error retrieving profile reference: %d ***\n",
                     rv);
                break;
            }

            if (ref_count <= 0) {
                continue;
            }

            lport_entry_p =
                SOC_PROFILE_MEM_ENTRY(unit, info->lport_profile,
                                      lport_tab_entry_t *, i);

            soc_cm_print("  %5d %8d\n", i, ref_count);
            soc_mem_entry_dump(unit, mem, lport_entry_p);
            soc_cm_print("\n");
        }
        soc_cm_print("\n");
    }

    /* ING_PRI_CNG_MAP table */
    if (NULL != info->ing_pri_cng_map) {
        ing_pri_cng_map_entry_t *ing_pri_cng_map_entry_p;
        int pri, cng;
        int entries_per_set;

        mem = ING_PRI_CNG_MAPm;
        num_entries = soc_mem_index_count(unit, mem);
        soc_cm_print("  ING_PRI_CNG_MAP Mem\n");
        soc_cm_print("    Number of entries: %d\n", num_entries);
        soc_cm_print("    Index RefCount EntriesPerSet - PRI CNG\n");

        for (i = 0; i < num_entries; i += _BCM_TR2_PRI_CNG_MAP_SIZE) {
            rv = soc_profile_mem_ref_count_get(unit,
                                               info->ing_pri_cng_map,
                                               i, &ref_count);
            if (SOC_FAILURE(rv)) {
                soc_cm_print
                    (" *** Error retrieving profile reference: %d ***\n",
                     rv);
                break;
            }

            if (ref_count <= 0) {
                continue;
            }

            for (j = 0; j < _BCM_TR2_PRI_CNG_MAP_SIZE; j++) {
                entries_per_set =
                    info->ing_pri_cng_map->tables[0].entries[i + j].entries_per_set;
                ing_pri_cng_map_entry_p =
                    SOC_PROFILE_MEM_ENTRY(unit, info->ing_pri_cng_map,
                                          ing_pri_cng_map_entry_t *,
                                          i + j);
                pri = soc_mem_field32_get(unit, mem,
                                          ing_pri_cng_map_entry_p,
                                          PRIf);
                cng = soc_mem_field32_get(unit, mem,
                                          ing_pri_cng_map_entry_p,
                                          CNGf);

                soc_cm_print("  %5d %8d %13d     %2d  %2d\n",
                             i + j, ref_count, entries_per_set,
                             pri, cng);
            }
        }
        soc_cm_print("\n");
    }

#define _BCM_QOS_PROFILE_ENTRIES_PER_SET 64

    /* EGR_MPLS_PRI_MAPPING & EGR_MPLS_EXP_MAPPING_1 tables */
    if (NULL != info->egr_mpls_combo_map) {
        egr_mpls_pri_mapping_entry_t
            egr_mpls_pri_map_entry[_BCM_QOS_PROFILE_ENTRIES_PER_SET];
        egr_mpls_exp_mapping_1_entry_t
            egr_mpls_exp_map_entry[_BCM_QOS_PROFILE_ENTRIES_PER_SET];
        int pri, new_cfi, cfi, exp;
        int entries_per_set;
        void *entries[2];

        mem = EGR_MPLS_PRI_MAPPINGm;
        num_entries = soc_mem_index_count(unit, mem);
        soc_cm_print
            ("  EGR_MPLS_PRI_MAPPING & EGR_MPLS_EXP_MAP Combo Mem\n");
        soc_cm_print("    Number of entries: %d\n", num_entries);
        soc_cm_print("    Index RefCount EntriesPerSet - PRI CFI MPLS_EXP\n");

        for (i = 0; i < num_entries;
             i += _BCM_QOS_PROFILE_ENTRIES_PER_SET) {
            rv = soc_profile_mem_ref_count_get(unit,
                                               info->egr_mpls_combo_map,
                                               i, &ref_count);
            if (SOC_FAILURE(rv)) {
                soc_cm_print
                    (" *** Error retrieving profile reference: %d ***\n",
                     rv);
                break;
            }

            if (ref_count <= 0) {
                continue;
            }

            entries[0] = &egr_mpls_pri_map_entry;
            entries[1] = &egr_mpls_exp_map_entry;

            rv = soc_profile_mem_get(unit, info->egr_mpls_combo_map,
                                     i, _BCM_QOS_PROFILE_ENTRIES_PER_SET, entries);
            if (SOC_FAILURE(rv)) {
                soc_cm_print
                    (" *** Error retrieving profile data: %d ***\n",
                     rv);
                break;
            }

            for (j = 0; j < _BCM_QOS_PROFILE_ENTRIES_PER_SET; j++) {
                entries_per_set =
                    info->egr_mpls_combo_map->tables[0].entries[i + j].entries_per_set;

                pri = soc_mem_field32_get(unit, mem,
                                          &(egr_mpls_pri_map_entry[j]),
                                          NEW_PRIf);
                new_cfi = soc_mem_field32_get(unit, mem,
                                              &(egr_mpls_pri_map_entry[j]),
                                              NEW_CFIf);
                cfi = soc_mem_field32_get(unit, EGR_MPLS_EXP_MAPPING_1m,
                                          &(egr_mpls_exp_map_entry[j]),
                                          CFIf);
                exp = soc_mem_field32_get(unit, EGR_MPLS_EXP_MAPPING_1m,
                                          &(egr_mpls_exp_map_entry[j]),
                                          MPLS_EXPf);

                soc_cm_print
                    ("  %5d %8d %13d     %2d  %2d    %2d",
                     i + j, ref_count, entries_per_set,
                     pri, cfi, exp);
                if (new_cfi != cfi) {
                    soc_cm_print("   CFI inconsistent!");
                }
                soc_cm_print("\n");
            }
        }
        soc_cm_print("\n");
    }

    /* DSCP_TABLE table */
    if (NULL != info->dscp_table) {
        dscp_table_entry_t *dscp_table_entry_p;
        int cng, pri, dscp;
        int entries_per_set;

        mem = DSCP_TABLEm;
        num_entries = soc_mem_index_count(unit, mem);
        soc_cm_print("  DSCP_TABLE Mem\n");
        soc_cm_print("    Number of entries: %d\n", num_entries);
        soc_cm_print("    Index RefCount EntriesPerSet - CNG PRI DSCP\n");

        for (i = 0; i < num_entries;
             i += _BCM_QOS_PROFILE_ENTRIES_PER_SET) {
            rv = soc_profile_mem_ref_count_get(unit,
                                               info->dscp_table,
                                               i, &ref_count);
            if (SOC_FAILURE(rv)) {
                soc_cm_print
                    (" *** Error retrieving profile reference: %d ***\n",
                     rv);
                break;
            }

            if (ref_count <= 0) {
                continue;
            }

            for (j = 0; j < _BCM_QOS_PROFILE_ENTRIES_PER_SET; j++) {
                entries_per_set =
                    info->dscp_table->tables[0].entries[i + j].entries_per_set;
                dscp_table_entry_p =
                    SOC_PROFILE_MEM_ENTRY(unit, info->dscp_table,
                                          dscp_table_entry_t *,
                                          i + j);
                cng = soc_mem_field32_get(unit, mem,
                                          dscp_table_entry_p,
                                          CNGf);
                pri = soc_mem_field32_get(unit, mem,
                                          dscp_table_entry_p,
                                          PRIf);
                dscp = soc_mem_field32_get(unit, mem,
                                           dscp_table_entry_p,
                                           DSCPf);

                soc_cm_print("  %5d %8d %13d     %2d  %2d  %2d\n",
                             i + j, ref_count, entries_per_set,
                             cng, pri, dscp);
            }
        }
        soc_cm_print("\n");
    }

    /* EGR_DSCP_TABLE table */
    if (NULL != info->egr_dscp_table) {
        egr_dscp_table_entry_t *egr_dscp_table_entry_p;
        int entries_per_set;
        int dscp;

        mem = EGR_DSCP_TABLEm;
        num_entries = soc_mem_index_count(unit, mem);
        soc_cm_print("  EGR_DSCP_TABLE Mem\n");
        soc_cm_print("    Number of entries: %d\n", num_entries);
        soc_cm_print("    Index RefCount EntriesPerSet - DSCP\n");

        for (i = 0; i < num_entries;
             i += _BCM_QOS_PROFILE_ENTRIES_PER_SET) {
            rv = soc_profile_mem_ref_count_get(unit,
                                               info->egr_dscp_table,
                                               i, &ref_count);
            if (SOC_FAILURE(rv)) {
                soc_cm_print
                    (" *** Error retrieving profile reference: %d ***\n",
                     rv);
                break;
            }

            if (ref_count <= 0) {
                continue;
            }

            for (j = 0; j < _BCM_QOS_PROFILE_ENTRIES_PER_SET; j++) {
                entries_per_set =
                    info->egr_dscp_table->tables[0].entries[i + j].entries_per_set;
                egr_dscp_table_entry_p =
                    SOC_PROFILE_MEM_ENTRY(unit, info->egr_dscp_table,
                                          egr_dscp_table_entry_t *,
                                          i + j);
                dscp = soc_mem_field32_get(unit, mem,
                                           egr_dscp_table_entry_p,
                                           DSCPf);

                soc_cm_print("  %5d %8d %13d     %2d\n",
                             i + j, ref_count, entries_per_set,
                             dscp);
            }
        }
        soc_cm_print("\n");
    }

    if (NULL != info->prot_pkt_ctrl) {
        uint32 prot_pkt_ctrl;
        uint32 igmp_mld_pkt_ctrl;

        num_entries = SOC_REG_NUMELS(unit, PROTOCOL_PKT_CONTROLr);
        soc_cm_print("  PROTOCOL_PKT_CONTROL & IGMP_MLD_PKT_CONTROL\n");
        soc_cm_print("    Number of entries: %d\n", num_entries);
        soc_cm_print("    Index RefCount -  Protocol    IGMP_MLD\n");

        for (i = 0; i < num_entries; i++) {
            rv = soc_profile_reg_ref_count_get(unit,
                                               info->prot_pkt_ctrl,
                                               i, &ref_count);
            if (SOC_FAILURE(rv)) {
                soc_cm_print
                    (" *** Error retrieving profile reference: %d ***\n",
                     rv);
                break;
            }
            if (ref_count <= 0) {
                continue;
            }

            prot_pkt_ctrl = 0;
            igmp_mld_pkt_ctrl = 0;
            rv = _bcm_prot_pkt_ctrl_get(unit, i, &prot_pkt_ctrl,
                                        &igmp_mld_pkt_ctrl);
            if (SOC_FAILURE(rv)) {
                soc_cm_print
                    (" *** Error retrieving profile register data: %d ***\n",
                     rv);
                break;
            }

            soc_cm_print("  %5d %8d      0x%08x  0x%08x\n",
                         i, ref_count, prot_pkt_ctrl, igmp_mld_pkt_ctrl);
        }
        soc_cm_print("\n");
    }

#ifdef BCM_TRIUMPH2_SUPPORT
    if (soc_feature(unit, soc_feature_gport_service_counters)) {
        _bcm_flex_stat_type_t fs_type, hw_type;
        uint32 fs_index;
        _bcm_flex_stat_handle_t fs_handle;
        int list_index, list_size;
        const char *fs_type_string[_bcmFlexStatTypeNum] = {
            "Service",
            "Gport",
            "Field",
            "VRF",
            "Vxlt",
            "EgrVxlt"
        };

        soc_cm_print("  Flexible statistics\n");
        for (hw_type = _bcmFlexStatTypeService;
             hw_type < _bcmFlexStatTypeAllHwNum; hw_type++) {
            
            list_size = FS_HANDLE_LIST_SIZE(unit, hw_type);

            soc_cm_print("    %s type, Mem size %5d, List size %5d\n",
                         fs_type_string[hw_type],
                         FS_MEM_SIZE(unit, hw_type),
                         list_size);
            if (0 == list_size) {
                continue;
            }
            soc_cm_print("       Handle                  Index      Type\n");
            for (list_index = 0; list_index < list_size; list_index++) {
                _BCM_FLEX_STAT_HANDLE_ASSIGN(fs_handle,
                          FS_HANDLE_LIST_ELEMENT_HANDLE(unit, hw_type,
                                                        list_index));
                fs_index = FS_HANDLE_LIST_ELEMENT_INDEX(unit, hw_type,
                                                        list_index);
                fs_type = FS_HANDLE_LIST_ELEMENT_TYPE(unit, hw_type,
                                                      list_index);

                if (fs_type >= _bcmFlexStatTypeNum) {
                    continue;
                }
                soc_cm_print("       0x%08x 0x%08x %5d        %s\n",
                             _BCM_FLEX_STAT_HANDLE_WORD_GET(fs_handle, 1),
                             _BCM_FLEX_STAT_HANDLE_WORD_GET(fs_handle, 0),
                             fs_index, fs_type_string[fs_type]);
            }
        }
    }
#endif /* BCM_TRIUMPH2_SUPPORT */

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */
