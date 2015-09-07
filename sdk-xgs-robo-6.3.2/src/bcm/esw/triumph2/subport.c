/*
 * $Id: subport.c 1.34.30.1 Broadcom SDK $
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
 * File:    subport.c
 * Purpose: Triumph SUBPORT functions
 */

#include <soc/defs.h>
#include <sal/core/libc.h>

#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)

#include <soc/drv.h>
#include <soc/hash.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/debug.h>

#include <bcm/error.h>
#include <bcm/subport.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm_int/esw/subport.h>

#include <bcm_int/esw_dispatch.h>

/* Usage bitmap of subport groups (1 group = 8 VPs) */
STATIC SHR_BITDCL *_tr2_group_bitmap[BCM_MAX_NUM_UNITS] = { 0 };

/* Array of L3_INTF IDs per VP (0xffff means unused) */
STATIC uint16 *_tr2_subport_id[BCM_MAX_NUM_UNITS] = { 0 };

/* Per-port count of all subports added */
STATIC int _tr2_subport_group_count[BCM_MAX_NUM_UNITS][SOC_MAX_NUM_PORTS] = {{ 0 }};

/*
 * Limited to 4K VPs (instead of actual table size of 8K)
 * since the IVID field (12-bits) in EGR_VLAN_XLATE table is 
 * used as the DVP value.
 */
#define _TR2_SUBPORT_NUM_VP          (4096)
#define _TR2_VP_PER_SUBPORT_GROUP    (8)
#define _TR2_SUBPORT_NUM_GROUP (_TR2_SUBPORT_NUM_VP / _TR2_VP_PER_SUBPORT_GROUP)

#define _TR2_SUBPORT_VP_MEM_LOCK(unit) \
   do { \
       if (SOC_IS_TRIUMPH2(unit) || SOC_MEM_IS_VALID(unit, SOURCE_VPm)) { \
           MEM_LOCK(unit, SOURCE_VPm); \
       } else { \
           sal_mutex_take(_tr2_subport_vp_mutex[unit], sal_mutex_FOREVER); \
       } \
     } while ((0))

#define _TR2_SUBPORT_VP_MEM_UNLOCK(unit) \
   do { \
       if (SOC_IS_TRIUMPH2(unit) || SOC_MEM_IS_VALID(unit, SOURCE_VPm)) { \
          MEM_UNLOCK(unit, SOURCE_VPm); \
       } else { \
          sal_mutex_give(_tr2_subport_vp_mutex[unit]); \
       } \
      } while ((0))


STATIC sal_mutex_t _tr2_subport_vp_mutex[BCM_MAX_NUM_UNITS] = {NULL};

#define _TR2_SUBPORT_CHECK_INIT(_unit_) \
        if (!_tr2_group_bitmap[_unit_]) \
            return BCM_E_INIT

STATIC int _bcm_tr2_subport_port_delete(int unit, int l3_idx, int vp);
STATIC int _bcm_tr2_subport_group_destroy(int unit, int vp_base);

/*
 * Subport group usage bitmap operations
 */
#define _BCM_TR2_GROUP_USED_GET(_u_, _group_) \
        SHR_BITGET(_tr2_group_bitmap[_u_], (_group_))
#define _BCM_TR2_GROUP_USED_SET(_u_, _group_) \
        SHR_BITSET(_tr2_group_bitmap[_u_], (_group_))
#define _BCM_TR2_GROUP_USED_CLR(_u_, _group_) \
        SHR_BITCLR(_tr2_group_bitmap[_u_], (_group_))

/*
 * Subport port usage bitmap operations
 */
#define _BCM_TR2_SUBPORT_ID_GET(_u_, _subport_) \
        (_tr2_subport_id[_u_][_subport_])
#define _BCM_TR2_SUBPORT_ID_SET(_u_, _subport_, _id_) \
        (_tr2_subport_id[_u_][_subport_] = _id_)
#define _BCM_TR2_SUBPORT_ID_CLR(_u_, _subport_) \
        (_tr2_subport_id[_u_][_subport_] = 0xffff)




/*
 * Function:
 *      _bcm_tr_subport_gport_used
 * Purpose:
 *      Internal function for testing if a gport is a configured subport type
 * Parameters:
 *      unit    -  (IN) Device number.
 *      port -  (IN) Gport
 * Returns:
 *      BCM_X_XXX
 */
int
_bcm_tr2_subport_gport_used(int unit, bcm_gport_t port)
{
    int vp;

    if (BCM_GPORT_IS_SUBPORT_GROUP(port)) {
        vp = BCM_GPORT_SUBPORT_GROUP_GET(port);
        if (vp >= _TR2_SUBPORT_NUM_GROUP) {
            return BCM_E_PARAM;
        }
        if (!_BCM_TR2_GROUP_USED_GET(unit, (vp / 8))) {
            return BCM_E_NOT_FOUND;
        }
    } else if (BCM_GPORT_IS_SUBPORT_PORT(port)) {
        vp = BCM_GPORT_SUBPORT_PORT_GET(port);
        if (vp >= _TR2_SUBPORT_NUM_VP) {
            return BCM_E_PARAM;
        }
        if (_BCM_TR2_SUBPORT_ID_GET(unit, vp) == 0xffff) {
            return BCM_E_NOT_FOUND;
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_subport_vp_alloc
 * Purpose:
 *      Internal function for allocating a group of VPs
 * Parameters:
 *      unit    -  (IN) Device number.
 *      base_vp -  (OUT) Base VP index
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr2_subport_vp_alloc(int unit, int *base_vp)
{
    int i;
/*
 * VP allocation is normally done by the MPLS module. However,
 * for builds without MPLS support, the VP allocation is done here.
 */
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
       int rv;
       /* 
        * Allocate a group of 8 consecutive VPs since 3-bit prio
        * is added to DVP-group to get the final DVP in the L2 lookup.
        * Only allocate from the 0 - 4k range since only 12-bits
        * are used in the EGR_VLAN_XLATE table key.
        * (the IVID field is used as the DVP in EGR_VLAN_XLATE)
        */
        rv = _bcm_vp_alloc(unit, 0, 4095, 8, SOURCE_VPm, _bcmVpTypeSubport, base_vp);
        if (rv < 0) {
            return rv;
        }
        _BCM_TR2_GROUP_USED_SET(unit, *base_vp / 8);
        return BCM_E_NONE;
   }

    _TR2_SUBPORT_VP_MEM_LOCK(unit);
    for (i = 0; i < _TR2_SUBPORT_NUM_GROUP; i++) {
        if (!_BCM_TR2_GROUP_USED_GET(unit, i)) {
            break;
        }
    }
    if (i == _TR2_SUBPORT_NUM_GROUP) {
        _TR2_SUBPORT_VP_MEM_UNLOCK(unit);
        return BCM_E_RESOURCE;
    }
    *base_vp = i * 8;

    _BCM_TR2_GROUP_USED_SET(unit, i);
    _TR2_SUBPORT_VP_MEM_UNLOCK(unit);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_subport_vp_free
 * Purpose:
 *      Internal function for freeing a group of VPs
 * Parameters:
 *      unit    -  (IN) Device number.
 *      base_vp -  (IN) Base VP index
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr2_subport_vp_free(int unit, int base_vp)
{
/*
 * VP freeing is normally done by the MPLS module. However,
 * for builds without MPLS support, the VP free is done here.
 */
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        int rv;

        rv = _bcm_vp_free(unit, _bcmVpTypeSubport, 8, base_vp);
        BCM_IF_ERROR_RETURN (rv);
    }

    _BCM_TR2_GROUP_USED_CLR(unit, base_vp / 8);

    return BCM_E_NONE;
}

STATIC void
_bcm_tr2_subport_free_resource(int unit)
{
    if (_tr2_group_bitmap[unit]) {
        sal_free(_tr2_group_bitmap[unit]);
        _tr2_group_bitmap[unit] = NULL;
    }
    if (_tr2_subport_id[unit]) {
        sal_free(_tr2_subport_id[unit]);
        _tr2_subport_id[unit] = NULL;
    }

    if (_tr2_subport_vp_mutex[unit] != NULL) {
        sal_mutex_destroy(_tr2_subport_vp_mutex[unit]);
        _tr2_subport_vp_mutex[unit] = NULL;
    }
}

#ifdef BCM_WARM_BOOT_SUPPORT
/* 
 * Function:
 *     _bcm_tr2_subport_reinit
 * Purpose:
 *     Reinit for warm boot.
 * Parameters:
 *      unit    : (IN) Device Unit Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_tr2_subport_reinit(int unit)
{
    int vpbase_gprtid, stable_size;
    int idx, grp_base_idx = 0, vp_count = 0, vp = 0;
    int num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    int index_min, index_max, fs_index, tpid_enable, index;
    source_vp_entry_t svp_entry;
    egr_l3_intf_entry_t egr_l3_entry;

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    /* Reconstruct s/w state: Subport IDs */
    index_min = soc_mem_index_min(unit, EGR_L3_INTFm);
    index_max = soc_mem_index_max(unit, EGR_L3_INTFm);
                                 
    for (idx = index_min; idx <= index_max; idx++) {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, EGR_L3_INTFm, MEM_BLOCK_ANY,
                                      idx, &egr_l3_entry));
        if (soc_mem_field_valid(unit, EGR_L3_INTFm, IVID_VALIDf)) {
            if (0 == soc_EGR_L3_INTFm_field32_get(unit, &egr_l3_entry,
                                                  IVID_VALIDf)) {
                continue;
            }
        } else {
            if (1 != soc_EGR_L3_INTFm_field32_get(unit, &egr_l3_entry,
                                                  IVID_PRESENT_ACTIONf) &&
                1 != soc_EGR_L3_INTFm_field32_get(unit, &egr_l3_entry,
                                                  IVID_ABSENT_ACTIONf)) {
                continue;
            }
        }
        if ((vp = soc_EGR_L3_INTFm_field32_get(unit, &egr_l3_entry, IVIDf)) == 0){
            continue;
        }
        if ((stable_size == 0) || SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
            BCM_IF_ERROR_RETURN(
                _bcm_vp_used_set(unit, vp, _bcmVpTypeSubport));
            _BCM_TR2_SUBPORT_ID_SET(unit, vp, idx);
            BCM_L3_INTF_USED_SET(unit, idx);
        } else {
        /* Extended scache based (Level 2) recovery */
            if (_bcm_vp_used_get(unit, vp, _bcmVpTypeSubport)) {
                _BCM_TR2_SUBPORT_ID_SET(unit, vp, idx);
                BCM_L3_INTF_USED_SET(unit, idx);
            }
        }
    }

    /* Reconstruct s/w state: Subport Groups */
    index_min = soc_mem_index_min(unit, SOURCE_VPm);
    index_max = soc_mem_index_max(unit, SOURCE_VPm);
    for (idx = 0; idx < num_vp ; idx++) {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, SOURCE_VPm, MEM_BLOCK_ANY,
                                      idx, &svp_entry));

        if (0x3 != soc_SOURCE_VPm_field32_get(unit, &svp_entry, ENTRY_TYPEf)) {
            continue;
        }

        /* A Subport group consists of a contiguous chunk of eight VPs */
        if (0 == grp_base_idx) {
            grp_base_idx = idx;
        }

        if (_TR2_VP_PER_SUBPORT_GROUP == ++vp_count) {
            if((grp_base_idx + _TR2_VP_PER_SUBPORT_GROUP) ==
                (idx + 1)) {/* if VP is the base VP of group */
                _BCM_TR2_GROUP_USED_SET(unit, (idx/_TR2_VP_PER_SUBPORT_GROUP));
                /* Update flex stats */
                if (soc_feature(unit, soc_feature_gport_service_counters)) {
                    if (soc_mem_field_valid(unit, SOURCE_VPm, VINTF_CTR_IDXf)) {
                        fs_index = soc_SOURCE_VPm_field32_get(unit, &svp_entry,
                                                                VINTF_CTR_IDXf);
                        if (fs_index) {
                            BCM_GPORT_SUBPORT_GROUP_SET(vpbase_gprtid,
                                                         grp_base_idx);
                            _bcm_esw_flex_stat_reinit_add(unit,
                                _bcmFlexStatTypeGport, fs_index, vpbase_gprtid);
                        }
                    }
                }
                grp_base_idx = 0; vp_count = 0; /* Reset for next group */
            }
        }

        /* Recover TPID ref count */
        if (soc_SOURCE_VPm_field32_get(unit, &svp_entry, SD_TAG_MODEf) == 1) {
            tpid_enable = soc_SOURCE_VPm_field32_get(unit, &svp_entry,
                                                                  TPID_ENABLEf);
            for (index = 0; index < 4; index++) {
                if (tpid_enable & (1 << index)) {
                    BCM_IF_ERROR_RETURN(_bcm_fb2_outer_tpid_tab_ref_count_add(
                                                               unit, index, 1));
                    break;
                }
            }
        }
    }

    return BCM_E_NONE; 
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      bcm_subport_init
 * Purpose:
 *      Initialize the SUBPORT software module
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_subport_init(int unit)
{
    int i;
    int rv = BCM_E_NONE;

    /*  Make sure module was initialized. */
    if (!BCM_XGS3_L3_INITIALIZED(unit)) {
        return (BCM_E_INIT);
    }

    /* 
     * Include hook to flex stats
     */

    if (_tr2_subport_id[unit] == NULL) {
        _tr2_subport_id[unit] = 
            sal_alloc(_TR2_SUBPORT_NUM_VP * sizeof(uint16), "subport_bitmap");
        if (_tr2_subport_id[unit] == NULL) {
            return BCM_E_MEMORY;
        }
    }
    for (i = 0; i < _TR2_SUBPORT_NUM_VP; i++) {
        _BCM_TR2_SUBPORT_ID_CLR(unit, i);
    }

    if (_tr2_group_bitmap[unit] == NULL) {
        _tr2_group_bitmap[unit] = 
            sal_alloc(SHR_BITALLOCSIZE(_TR2_SUBPORT_NUM_GROUP), "subport_group_bitmap");
        if (_tr2_group_bitmap[unit] == NULL) {
            _bcm_tr2_subport_free_resource(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_tr2_group_bitmap[unit], 0, SHR_BITALLOCSIZE(_TR2_SUBPORT_NUM_GROUP));
    /* Don't use VP 0 (HW restriction) : mark group 0 as used */
    _BCM_TR2_GROUP_USED_SET(unit, 0);

    for (i = 0; i < SOC_MAX_NUM_PORTS; i++) {
        _tr2_subport_group_count[unit][i] = 0;
    }

    if (!SOC_MEM_IS_VALID(unit, SOURCE_VPm)) { 
        if (NULL == _tr2_subport_vp_mutex[unit]) {
            _tr2_subport_vp_mutex[unit] = sal_mutex_create("subport vp mutex");
            if (_tr2_subport_vp_mutex[unit] == NULL) {
                _bcm_tr2_subport_free_resource(unit);
                return BCM_E_MEMORY;
            }
        }
    } 
#ifdef BCM_WARM_BOOT_SUPPORT
    if(SOC_WARM_BOOT(unit)) {
        rv = _bcm_tr2_subport_reinit(unit);
        if (rv != BCM_E_NONE) {
            _bcm_tr2_subport_free_resource(unit);
        }
    }
#endif /* BCM_WARM_BOOT_SUPPORT */
    return rv;
}

/*
 * Function:
 *      bcm_tr2_subport_hw_clear
 * Purpose:
 *      Cleanup the SUBPORT hw module.
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_subport_hw_clear(int unit)
{
    int i, rv, vp, l3_idx;

    
    /* _bcm_tr2_subport_port_delete() is taking VLAN_XLATEm lock. */
    _TR2_SUBPORT_VP_MEM_LOCK(unit);
    /* Delete all existing subports */
    for (vp = 0; vp < _TR2_SUBPORT_NUM_VP; vp++) {
        l3_idx = _BCM_TR2_SUBPORT_ID_GET(unit, vp);
        if (l3_idx != 0xffff) {
            rv = _bcm_tr2_subport_port_delete(unit, l3_idx, vp);
            if (rv < 0) {
                _TR2_SUBPORT_VP_MEM_UNLOCK(unit);
                return rv;
            }
        }
    }

    /* Delete all existing groups (Group 0 reserved).*/
    for (i = 1; i < _TR2_SUBPORT_NUM_GROUP; i++) {
        if (_BCM_TR2_GROUP_USED_GET(unit, i)) {
            rv = _bcm_tr2_subport_group_destroy(unit, (i * 8));
            if (rv < 0) {
                _TR2_SUBPORT_VP_MEM_UNLOCK(unit);
                return rv;
            }
        }
    }
    _TR2_SUBPORT_VP_MEM_UNLOCK(unit);
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2_subport_cleanup
 * Purpose:
 *      Cleanup the SUBPORT software module
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_subport_cleanup(int unit)
{
    int rv; /* Operation return status. */

    if (NULL == _tr2_group_bitmap[unit]) {
        return (BCM_E_NONE);
    }

     if (0 == SOC_HW_ACCESS_DISABLE(unit)) {
        rv = _bcm_tr2_subport_hw_clear(unit);
        BCM_IF_ERROR_RETURN(rv);
    }

    _bcm_tr2_subport_free_resource(unit);
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_subport_group_create
 * Purpose:
 *      Create a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      config - (IN) Subport group config information
 *      group  - (OUT) GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_subport_group_create(int unit, bcm_subport_group_config_t *config,
                             bcm_gport_t *group)
{
    int gport_id, rv, vp_base = -1, nh_index = -1, priority;
    initial_ing_l3_next_hop_entry_t initial_ing_nh;
    ing_l3_next_hop_entry_t ing_nh;
    egr_l3_next_hop_entry_t egr_nh;
    source_vp_entry_t svp;
    ing_dvp_table_entry_t dvp;
    bcm_l3_egress_t nh_info;
    uint32 nh_flags;
    bcm_module_t mod_out, my_modid;
    bcm_port_t port_out;
    bcm_trunk_t trunk_id;

    _TR2_SUBPORT_CHECK_INIT(unit);

    if ((config == NULL) || (group == NULL) || (config->vlan > BCM_VLAN_MAX)) {
        return BCM_E_PARAM;
    }
    rv = _bcm_esw_gport_resolve(unit, config->port, &mod_out, &port_out, &trunk_id,
                                &gport_id);
    BCM_IF_ERROR_RETURN(rv);

    BCM_IF_ERROR_RETURN (bcm_esw_stk_my_modid_get(unit, &my_modid));

    if (config->flags & BCM_SUBPORT_GROUP_WITH_ID) {
        vp_base = BCM_GPORT_SUBPORT_GROUP_GET(*group);
        _TR2_SUBPORT_VP_MEM_LOCK(unit);
        if ((vp_base < 0) || (vp_base > (4096 - 8))) {
            rv = BCM_E_PARAM;
        } else if (_BCM_TR2_GROUP_USED_GET(unit, (vp_base / 8))) {
            rv = BCM_E_EXISTS;
        } else {
            _BCM_TR2_GROUP_USED_SET(unit, vp_base / 8);
        }
        _TR2_SUBPORT_VP_MEM_UNLOCK(unit);
    } else {
        /* 
         * Allocate a group of 8 consecutive VPs since 3-bit prio
         * is added to DVP-group to get the final DVP in the L2 lookup.
         * Only allocate from the 0 - 4k range since only 12-bits
         * are used in the EGR_VLAN_XLATE table key.
         * (the IVID field is used as the DVP in EGR_VLAN_XLATE)
         */
        rv = _bcm_tr2_subport_vp_alloc(unit, &vp_base);
    }
    BCM_IF_ERROR_RETURN (rv);

    for (priority=0; priority < 8; priority++) {
         /* 
           * Allocate a next-hop entry. By calling bcm_xgs3_nh_add()
           * with _BCM_L3_SHR_WRITE_DISABLE flag, a next-hop index is
           * allocated but nothing is written to hardware. The "nh_info"
           * in this case is not used, so just set to all zeros.
          */
         sal_memset(&nh_info, 0, sizeof(nh_info));
         nh_flags = _BCM_L3_SHR_MATCH_DISABLE | _BCM_L3_SHR_WRITE_DISABLE;
         rv = bcm_xgs3_nh_add(unit, nh_flags, &nh_info, &nh_index);
         if (rv < 0) {
              goto cleanup;
         } 

         /* 
           * The EGR_L3_NEXT_HOP is not actually used in the datapath.
           * Instead, the DVP value is passed through the MMU in the
           * IVID field. At the egress, the IVID (DVP) is used in the
           * egress vlan translate table key. Here we use the 
           * EGR_L3_NEXT_HOP just to store the subport group's vlan
           * for warm reboot purposes for Triumph.
          */

         if (soc_feature(unit, soc_feature_subport_enhanced)) {
              /* Write EGR_L3_NEXT_HOP entry */
             sal_memset(&egr_nh, 0, sizeof(egr_l3_next_hop_entry_t));
             soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, OVIDf, 
                            config->vlan);
             rv = soc_mem_write(unit, EGR_L3_NEXT_HOPm,
                           MEM_BLOCK_ALL, nh_index, &egr_nh);
             if (rv < 0) {
                 goto cleanup;
             }
         } 

         /* Write ING_L3_NEXT_HOP entry */
         sal_memset(&ing_nh, 0, sizeof(ing_l3_next_hop_entry_t));
         if (BCM_GPORT_IS_TRUNK(config->port)) {
             soc_mem_field32_set(unit, ING_L3_NEXT_HOPm, &ing_nh, Tf, 1);
             soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                            &ing_nh, TGIDf, trunk_id);
         } else {
              soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                            &ing_nh, PORT_NUMf, port_out);
              soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                            &ing_nh, MODULE_IDf, mod_out);
         }
         /* Use all 8 DVPs in the group. */
         soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                       &ing_nh, DVP_RES_INFOf, 0x7f);

         if (soc_feature(unit, soc_feature_subport_enhanced)) {
             soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                            &ing_nh, ENTRY_TYPEf, 0x3); /* GPON DVP */
         }

         rv = soc_mem_write (unit, ING_L3_NEXT_HOPm,
                       MEM_BLOCK_ALL, nh_index, &ing_nh);
         if (rv < 0) {
            goto cleanup;
         }

         /* Write INITIAL_ING_L3_NEXT_HOP entry */
         sal_memset(&initial_ing_nh, 0, sizeof(initial_ing_l3_next_hop_entry_t));
         if (BCM_GPORT_IS_TRUNK(config->port)) {
             soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                            &initial_ing_nh, Tf, 1);
             soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                            &initial_ing_nh, TGIDf, trunk_id);
         } else {
             soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                            &initial_ing_nh, PORT_NUMf, port_out);
             soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                            &initial_ing_nh, MODULE_IDf, mod_out);
         }
         rv = soc_mem_write(unit, INITIAL_ING_L3_NEXT_HOPm,
                       MEM_BLOCK_ALL, nh_index, &initial_ing_nh);
         if (rv < 0) {
             goto cleanup;
         }

         if (soc_feature(unit, soc_feature_subport_enhanced)) {
             /* Write ING_DVP_TABLE entry */
             sal_memset(&dvp, 0, sizeof(ing_dvp_table_entry_t));
             soc_ING_DVP_TABLEm_field32_set(unit, &dvp, NEXT_HOP_INDEXf, nh_index);
             rv = WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, (vp_base+priority), &dvp);
             if (rv < 0) {
                 goto cleanup;
             }

             /* Write SOURCE_VP table entry */
            sal_memset(&svp, 0, sizeof(svp));
            soc_SOURCE_VPm_field32_set(unit, &svp, ENTRY_TYPEf, 0x3); /* GPON */
            soc_SOURCE_VPm_field32_set(unit, &svp, CLASS_IDf, config->if_class);
            soc_SOURCE_VPm_field32_set(unit, &svp, DVPf, vp_base); /* DVP = vp_base */

            /* Set the CML to PVP_CML_SWITCH by default (hw learn and forward) */
            
            soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_NEWf, 0x8);
            soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_MOVEf, 0x8);
            rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, (vp_base+priority), &svp);
            if (rv < 0) {
                goto cleanup;
            }
        }
    }

    if (mod_out == my_modid) {
        if (_tr2_subport_group_count[unit][port_out]++ == 0) {
            SOC_IF_ERROR_RETURN
                (soc_reg_field32_modify(unit, EGR_IPMC_CFG2r, port_out,
                                        DISABLE_TTL_CHECKf, 1));
        }
    }
    BCM_GPORT_SUBPORT_GROUP_SET(*group, vp_base);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return BCM_E_NONE;

cleanup:

    for (;priority >= 0; priority--) {
        (void) READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, (vp_base+priority), &dvp);
        nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        if (nh_index != -1) {
            nh_flags = _BCM_L3_SHR_WRITE_DISABLE;
            (void) bcm_xgs3_nh_del(unit, nh_flags, nh_index);
        }
    }
    if (vp_base != -1) {
        (void) _bcm_tr2_subport_vp_free(unit, vp_base);
    }
    return rv;
}

STATIC int
_bcm_tr2_subport_group_destroy(int unit, int vp_base)
{
    int i, nh_index=-1, rv;
    initial_ing_l3_next_hop_entry_t initial_ing_nh;
    ing_l3_next_hop_entry_t ing_nh;
    egr_l3_next_hop_entry_t egr_nh;
    ing_dvp_table_entry_t dvp;
    source_vp_entry_t svp;
    bcm_port_t port;
    bcm_module_t my_modid, modid;

    BCM_IF_ERROR_RETURN (bcm_esw_stk_my_modid_get(unit, &my_modid));

    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        /* Get the next-hop index from the base VP */
        rv = READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp_base, &dvp);
        BCM_IF_ERROR_RETURN(rv);

        nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        BCM_IF_ERROR_RETURN (soc_mem_read(unit, ING_L3_NEXT_HOPm, MEM_BLOCK_ANY,
                                          nh_index, &ing_nh));

        if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, ENTRY_TYPEf) != 0x3) {
            /* Entry type is not GPON DVP */
            return BCM_E_NOT_FOUND;
        }
     }

    if (soc_feature(unit, soc_feature_subport_enhanced)) {

        for (i = 0; i < 8; i++) {

           /* Obtain nh_index from DVP_TABLE entry */
           rv = READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, (vp_base + i), &dvp);
           BCM_IF_ERROR_RETURN(rv);		   
           nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);

           /* Clear EGR_L3_NEXT_HOP entry */
           sal_memset(&egr_nh, 0, sizeof(egr_l3_next_hop_entry_t));
           rv = soc_mem_write(unit, EGR_L3_NEXT_HOPm,
                                  MEM_BLOCK_ALL, nh_index, &egr_nh);
           BCM_IF_ERROR_RETURN(rv);
	   
           /* Get the modid/port number from ING_L3_NEXT_HOP entry */
           rv = soc_mem_read(unit, ING_L3_NEXT_HOPm,
                                  MEM_BLOCK_ALL, nh_index, &ing_nh);
           BCM_IF_ERROR_RETURN(rv);
           modid = soc_mem_field32_get(unit, ING_L3_NEXT_HOPm,
                                  &ing_nh, MODULE_IDf);
           port = soc_mem_field32_get(unit, ING_L3_NEXT_HOPm,
                                  &ing_nh, PORT_NUMf);
		   
           if (modid == my_modid) {
              if (--_tr2_subport_group_count[unit][port] == 0) {
                   SOC_IF_ERROR_RETURN
                        (soc_reg_field32_modify(unit, EGR_IPMC_CFG2r, port,
                                            DISABLE_TTL_CHECKf, 0));
              }
           }
		   
           /* Clear ING_L3_NEXT_HOP entry */
           sal_memset(&ing_nh, 0, sizeof(ing_l3_next_hop_entry_t));
           rv = soc_mem_write (unit, ING_L3_NEXT_HOPm,
                                  MEM_BLOCK_ALL, nh_index, &ing_nh);
           BCM_IF_ERROR_RETURN(rv);
		   
           /* Clear INITIAL_ING_L3_NEXT_HOP entry */
           sal_memset(&initial_ing_nh, 0, sizeof(initial_ing_l3_next_hop_entry_t));
           rv = soc_mem_write(unit, INITIAL_ING_L3_NEXT_HOPm,
                                  MEM_BLOCK_ALL, nh_index, &initial_ing_nh);
           BCM_IF_ERROR_RETURN(rv);

		   		   
           /* Free the next-hop index */
           rv = bcm_xgs3_nh_del(unit, _BCM_L3_SHR_WRITE_DISABLE, nh_index);
           BCM_IF_ERROR_RETURN(rv);

           /* Clear SOURCE_VP table entry */
           sal_memset(&svp, 0, sizeof(svp));
           rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, (vp_base+i), &svp);
           BCM_IF_ERROR_RETURN(rv);

           /* Clear ING_DVP_TABLE entry */
           sal_memset(&dvp, 0, sizeof(dvp));
           rv = WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, (vp_base+i), &dvp);
           BCM_IF_ERROR_RETURN(rv);
        }
    }

    if (soc_feature(unit, soc_feature_gport_service_counters)) {
        bcm_gport_t gport;

        /* Release Service counter, if any */
        BCM_GPORT_SUBPORT_GROUP_SET(gport, vp_base);
        _bcm_esw_flex_stat_handle_free(unit, _bcmFlexStatTypeGport, gport);
    }

    /* Free the VP */
    rv = _bcm_tr2_subport_vp_free(unit, vp_base);
    BCM_IF_ERROR_RETURN(rv);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return rv;
}

/*
 * Function:
 *      bcm_subport_group_destroy
 * Purpose:
 *      Destroy a subport group
 * Parameters:
 *      unit  - (IN) Device Number
 *      group - (IN) GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_subport_group_destroy(int unit, bcm_gport_t group)
{   
    int vp_base;

    _TR2_SUBPORT_CHECK_INIT(unit);

    vp_base = BCM_GPORT_SUBPORT_GROUP_GET(group); 
    return _bcm_tr2_subport_group_destroy(unit, vp_base);
}

/*
 * Function:
 *      bcm_subport_group_get
 * Purpose:
 *      Get a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      group  - (IN) GPORT (generic port) identifier
 *      config - (OUT) Subport group config information
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_subport_group_get(int unit, bcm_gport_t group,
                         bcm_subport_group_config_t *config)
{
    int rv = BCM_E_NONE; 
    egr_l3_next_hop_entry_t egr_nh;
    ing_l3_next_hop_entry_t ing_nh;
    ing_dvp_table_entry_t dvp;
    source_vp_entry_t svp;
    int nh_index = -1, vp_base;
    bcm_port_t port_in, port_out;
    bcm_module_t mod_in, mod_out;
    bcm_trunk_t trunk_id;

    _TR2_SUBPORT_CHECK_INIT(unit);

    if (config == NULL) {
        return BCM_E_PARAM;
    }

    vp_base = BCM_GPORT_SUBPORT_GROUP_GET(group);
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        rv = READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp_base, &dvp);
        BCM_IF_ERROR_RETURN(rv);
    
        nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        BCM_IF_ERROR_RETURN (soc_mem_read(unit, ING_L3_NEXT_HOPm, MEM_BLOCK_ANY,
                                          nh_index, &ing_nh));

        if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, ENTRY_TYPEf) != 0x3) {
           /* Entry type is not GPON DVP */
           return BCM_E_NOT_FOUND;
        }
    }

    /* Get the physical gport information from the ingress next-hop entry */
    if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, Tf)) {
        trunk_id = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, TGIDf);
        BCM_GPORT_TRUNK_SET(config->port, trunk_id);
    } else {
        mod_in = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, MODULE_IDf);
        port_in = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, PORT_NUMf);
        BCM_IF_ERROR_RETURN(_bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                                   mod_in, port_in,
                                                   &mod_out, &port_out));
        BCM_GPORT_MODPORT_SET(config->port, mod_out, port_out);
    }

    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        /* Get the group's OVID from the group's egress next-hop entry */
        rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm,
                          MEM_BLOCK_ALL, nh_index, &egr_nh);
        BCM_IF_ERROR_RETURN(rv);
        config->vlan = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh, OVIDf);
    } 

    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        /* Get the class-ID from the SOURCE_VP table entry */
        rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp_base, &svp);
        BCM_IF_ERROR_RETURN(rv);
        config->if_class = soc_SOURCE_VPm_field32_get(unit, &svp, CLASS_IDf);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_subport_port_add
 * Purpose:
 *      Add a subport to a subport group
 * Parameters:
 *      unit   - (IN) Device Number
 *      config - (IN) Subport config information
 *      port   - (OUT) GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_subport_port_add(int unit, bcm_subport_config_t *config,
                         bcm_gport_t *port)
{
    int rv, l3_idx = -1, nh_index=-1, vp_base, vp;
    ing_dvp_table_entry_t dvp;
    egr_l3_next_hop_entry_t egr_nh;
    ing_l3_next_hop_entry_t ing_nh;
    egr_l3_intf_entry_t l3_intf;
    egr_vlan_xlate_entry_t egr_vent;
    vlan_xlate_entry_t ing_vent;
    bcm_vlan_t ovid = 0;
    bcm_port_t local_port = 0;
    bcm_module_t my_modid, modid = 0;
    bcm_vlan_action_set_t action;
    uint32 egr_profile_idx = 0xffffffff; 
    uint32 ing_profile_idx = 0xffffffff;


    _TR2_SUBPORT_CHECK_INIT(unit);

    if ((config->int_pri < 0) || (config->int_pri > 7)) {
        return BCM_E_PARAM;
    }

    /* Check Vlan range */
    if ((config->pkt_vlan >= BCM_VLAN_INVALID) || ( config->pkt_vlan == BCM_VLAN_DEFAULT)) {
         return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN (bcm_esw_stk_my_modid_get(unit, &my_modid));

    /* Make sure the group exists */
    vp_base = BCM_GPORT_SUBPORT_GROUP_GET(config->group);
    if (!_BCM_TR2_GROUP_USED_GET(unit, vp_base / 8)) {
        return BCM_E_NOT_FOUND;
    }

    /* See if this entry already exists */
    vp = vp_base + config->int_pri;
    if (_BCM_TR2_SUBPORT_ID_GET(unit, vp) != 0xffff) {
        return BCM_E_EXISTS;
    }


    /* ***************************************************** */
    /* --- Get the group's NHI from ING_DVP_TABLE--------------- */ 
    /* ***************************************************** */
    
      
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        rv = READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp_base, &dvp);
        BCM_IF_ERROR_RETURN(rv);
        nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
    }

    /* Get the modid and port_class for the destination port */
    rv = soc_mem_read(unit, ING_L3_NEXT_HOPm,
                      MEM_BLOCK_ALL, nh_index, &ing_nh);
    if (rv < 0) {
        goto cleanup;
    }
	
    if (!soc_mem_field32_get(unit, ING_L3_NEXT_HOPm, &ing_nh, Tf)) {
        modid = soc_mem_field32_get(unit, ING_L3_NEXT_HOPm, &ing_nh, MODULE_IDf);
        if (modid != my_modid) {
            /* No local state is required for groups on remoted devices */
            return BCM_E_NONE;
        }
        local_port = soc_mem_field32_get(unit, ING_L3_NEXT_HOPm,
                                         &ing_nh, PORT_NUMf);
        /* TBD:  Algorithm to compute DVP_RES_INFO */
    }
        
   /* *********************************************************** */
   /* ---- For each Priority, Update the fields within EGR_L3_NEXT_HOP ---- */
   /* *********************************************************** */
   
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
       /* Obtain the nh_index from DVP_Table for each Priority */
       rv = READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, (vp_base + config->int_pri), &dvp);
       BCM_IF_ERROR_RETURN(rv);
       nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);

        rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm,
                          MEM_BLOCK_ALL, nh_index, &egr_nh);
        BCM_IF_ERROR_RETURN(rv);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm,
                        &egr_nh, ENTRY_TYPEf, 0x2);
        if (soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm, DVPf)) {
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm,
                                &egr_nh, DVPf, (vp_base + config->int_pri));
        } else { /* use SD_TAG view becuase the ENTRY_TYPE is 2 */
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm,
                                &egr_nh, SD_TAG__DVPf,
                                (vp_base + config->int_pri));
        }
        if (soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm, HG_HDR_SELf)) {
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, HG_HDR_SELf,
                                1);
        } else { /* use SD_TAG view becuase the ENTRY_TYPE is 2 */
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                SD_TAG__HG_HDR_SELf, 1);
        }

        if (soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm, SD_TAG_VIDf)) {
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                                SD_TAG_VIDf, config->pkt_vlan);
        }
        rv = soc_mem_write(unit, EGR_L3_NEXT_HOPm,
                        MEM_BLOCK_ALL, nh_index, &egr_nh);
        if (rv < 0) {
             goto cleanup;
        }
        /* Get the group's OVID from the group's egress next-hop entry */
        ovid = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh, OVIDf);
    }
	
    /*
     * For each subport, we need to allocate an L3_INTF entry.
     * This is needed for multicast replication. MMU replicates
     * multicast packets to each subport by sending a copy to 
     * the subport's L3_INTF index.
     */

    /* Search for unused index. */
    MEM_LOCK(unit, EGR_L3_INTFm);
    for (l3_idx = 0; l3_idx < BCM_XGS3_L3_IF_TBL_SIZE(unit); l3_idx++) {
        /* Check if interface index is used. */
        if (!BCM_L3_INTF_USED_GET(unit, l3_idx)) {
            /* Free index found. */
            BCM_L3_INTF_USED_SET(unit, l3_idx);
            break;
        }
    }
    if (l3_idx == BCM_XGS3_L3_IF_TBL_SIZE(unit)) {
        MEM_UNLOCK(unit, EGR_L3_INTFm);
        return BCM_E_FULL;
    }

    /* Write EGR_L3_INTF entry */
    sal_memset(&l3_intf, 0, sizeof(egr_l3_intf_entry_t));
    soc_mem_field32_set(unit, EGR_L3_INTFm, &l3_intf, OVIDf, ovid);

    /* IVID field is used as the DVP */
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        soc_mem_field32_set(unit, EGR_L3_INTFm, &l3_intf, IVIDf, vp);
    }

    if (soc_mem_field_valid(unit, EGR_L3_INTFm, IVID_VALIDf)) {
        soc_mem_field32_set(unit, EGR_L3_INTFm, &l3_intf, IVID_VALIDf, 1);
    } else {
        soc_mem_field32_set(unit, EGR_L3_INTFm, &l3_intf, IVID_PRESENT_ACTIONf,
                            1);
        soc_mem_field32_set(unit, EGR_L3_INTFm, &l3_intf, IVID_ABSENT_ACTIONf,
                            1);
    }
    rv = soc_mem_write(unit, EGR_L3_INTFm, MEM_BLOCK_ALL, l3_idx, &l3_intf);
    if (rv < 0) {
        MEM_UNLOCK(unit, EGR_L3_INTFm);
        goto cleanup;
    }
    MEM_UNLOCK(unit, EGR_L3_INTFm);

    /* ************************* */
    /* Write EGR_VLAN_XLATE table  */
    /* ************************* */
	
    bcm_vlan_action_set_t_init(&action);
    /* Create a profile entry that replaces the outer vid
     * with the new 16-bit vlan tag.
     */
    action.ot_outer = bcmVlanActionReplace;
    action.dt_outer = bcmVlanActionReplace;
    action.dt_inner = bcmVlanActionDelete;
    rv = _bcm_trx_egr_vlan_action_profile_entry_add(unit, &action, &egr_profile_idx);
    if (rv < 0) {
        goto cleanup;
    }
    sal_memset(&egr_vent, 0, sizeof(egr_vent));
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, VALIDf, 1);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, OVIDf, ovid);
    soc_mem_field32_set(unit, EGR_VLAN_XLATEm, &egr_vent, DVPf, (vp_base + config->int_pri));
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, KEY_TYPEf, 0x01);
    } else 
#endif
    {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, ENTRY_TYPEf, 0x01);
    }
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, TAG_ACTION_PROFILE_PTRf, egr_profile_idx);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, NEW_OTAG_VPTAGf, config->pkt_vlan);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, NEW_OTAG_VPTAG_SELf, 1);

    rv = soc_mem_insert_return_old(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, 
                                   &egr_vent, &egr_vent);
    if (rv == SOC_E_EXISTS) {
        /* Delete the old vlan translate profile entry */
        egr_profile_idx = soc_EGR_VLAN_XLATEm_field32_get(unit, &egr_vent,
                                                          TAG_ACTION_PROFILE_PTRf);
        rv = _bcm_trx_egr_vlan_action_profile_entry_delete(unit, egr_profile_idx);
    }
    if (rv < 0) {
        goto cleanup;
    }

    /* **************************   */
    /* Write Ingress VLAN_XLATE table  */
    /* **************************   */
    bcm_vlan_action_set_t_init(&action);
    action.ot_outer = bcmVlanActionReplace;
    action.ot_outer_prio = bcmVlanActionReplace;
    action.dt_outer = bcmVlanActionReplace;
    action.dt_outer_prio = bcmVlanActionReplace;
    rv = _bcm_trx_vlan_action_profile_entry_add(unit, &action, &ing_profile_idx);
    if (rv < 0) {
        goto cleanup;
    }
    sal_memset(&ing_vent, 0, sizeof(ing_vent));
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, VALIDf, 1);
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, KEY_TYPEf,
                                TR_VLXLT_HASH_KEY_TYPE_OTAG);
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, OTAGf, config->pkt_vlan);
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, MODULE_IDf, modid);
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, PORT_NUMf, local_port);
    if (SOC_MEM_FIELD_VALID(unit, EGR_VLAN_XLATEm, RPEf)) {
        soc_VLAN_XLATEm_field32_set(unit, &ing_vent, RPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &ing_vent, PRIf, config->int_pri);
    }
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, TAG_ACTION_PROFILE_PTRf, ing_profile_idx);
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, NEW_OVIDf, ovid);
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, SOURCE_VPf, vp_base); /* Group VP */

    /* TD2 type of device indicating Source Field as SGLP */
    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
        soc_VLAN_XLATEm_field32_set(unit, &ing_vent, SOURCE_TYPEf, 1);
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        soc_VLAN_XLATEm_field32_set(unit, &ing_vent, SVP_VALIDf, 1); /* L2 SVP */
    } else 
#endif
    {
        soc_VLAN_XLATEm_field32_set(unit, &ing_vent, MPLS_ACTIONf, 1); /* L2 SVP */
    }

    rv = soc_mem_insert_return_old(unit, VLAN_XLATEm, MEM_BLOCK_ALL, 
                                   &ing_vent, &ing_vent);
    if (rv == SOC_E_EXISTS) {
        /* Delete the old vlan translate profile entry */
        ing_profile_idx = soc_VLAN_XLATEm_field32_get(unit, &ing_vent,
                                                      TAG_ACTION_PROFILE_PTRf);
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, ing_profile_idx);
    }
    if (rv < 0) {
        goto cleanup;
    }

    _BCM_TR2_SUBPORT_ID_SET(unit, vp, l3_idx);
    BCM_GPORT_SUBPORT_PORT_SET(*port, ((my_modid << 12) | l3_idx));
    return BCM_E_NONE;
cleanup:
    if (l3_idx != -1) {
        BCM_L3_INTF_USED_CLR(unit, l3_idx);
    }

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return rv;
}

STATIC int
_bcm_tr2_subport_port_delete(int unit, int l3_idx, int vp)
{
    int rv, vp_base, vt_index, nh_index=-1;
    egr_l3_intf_entry_t l3_intf;
    ing_l3_next_hop_entry_t ing_nh;
    egr_l3_next_hop_entry_t egr_nh;
    egr_vlan_xlate_entry_t egr_vent;
    vlan_xlate_entry_t ing_vent;
    ing_dvp_table_entry_t dvp;
    bcm_port_t port = 0;
    bcm_module_t modid = 0;
    bcm_vlan_t pkt_vlan, ovid = 0;
    uint32 egr_profile_idx, ing_profile_idx, port_class = 0;

    _TR2_SUBPORT_CHECK_INIT(unit);

    rv = soc_mem_read(unit, EGR_L3_INTFm, MEM_BLOCK_ALL, l3_idx, &l3_intf);
    BCM_IF_ERROR_RETURN(rv);

    /* Get the base group VP */
    vp_base = vp & ~(0x7);

    if (soc_feature(unit, soc_feature_subport_enhanced)) {
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(unit)) {
            vp_base = vp;
        }
#endif /* defined(BCM_KATANA2_SUPPORT) */        
        /* Get the group's next-hop index */
        rv = READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp_base, &dvp);
        BCM_IF_ERROR_RETURN(rv);
        nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        /* Get the group's OVID from the group's egress next-hop entry */
        rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm,
                          MEM_BLOCK_ALL, nh_index, &egr_nh);
        BCM_IF_ERROR_RETURN(rv);
        ovid = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh, OVIDf);
    }

    /* Get the egress port class */
    BCM_IF_ERROR_RETURN (soc_mem_read(unit, ING_L3_NEXT_HOPm, MEM_BLOCK_ANY,
                                      nh_index, &ing_nh));
    if (!soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, Tf)) {
        modid = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, MODULE_IDf);
        port = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, PORT_NUMf);
        rv = bcm_esw_port_class_get(unit, port, bcmPortClassVlanTranslateEgress,
                                    &port_class);
        BCM_IF_ERROR_RETURN(rv);
    } 

    /* Find the egress vlan translate entry */
    sal_memset(&egr_vent, 0, sizeof(egr_vent));
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, VALIDf, 1);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, OVIDf, ovid);
    soc_mem_field32_set(unit, EGR_VLAN_XLATEm, &egr_vent, DVPf, vp);
#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, KEY_TYPEf, 0x01);
    } else
#endif
    {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, ENTRY_TYPEf, 0x01);
    }
    MEM_LOCK(unit, EGR_VLAN_XLATEm);
    rv = soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &vt_index, 
                        &egr_vent, &egr_vent, 0);
    if (rv < 0) {
        MEM_UNLOCK(unit, EGR_VLAN_XLATEm);
        return rv;
    }

    /* Get the old vlan translate profile entry */
    egr_profile_idx = soc_EGR_VLAN_XLATEm_field32_get(unit, &egr_vent,
                                                      TAG_ACTION_PROFILE_PTRf);

    /* Delete the egress vlan translate entry */
    rv = soc_mem_delete(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &egr_vent);
    MEM_UNLOCK(unit, EGR_VLAN_XLATEm);
    BCM_IF_ERROR_RETURN(rv);

    /* Delete the old vlan translate profile entry */
    rv = _bcm_trx_egr_vlan_action_profile_entry_delete(unit, egr_profile_idx);
    BCM_IF_ERROR_RETURN(rv);

    /* Find vlan translate entry */
    pkt_vlan = soc_EGR_VLAN_XLATEm_field32_get(unit, &egr_vent, NEW_OTAG_VPTAGf);
    sal_memset(&ing_vent, 0, sizeof(ing_vent));
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, VALIDf, 1);
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, KEY_TYPEf,
                                TR_VLXLT_HASH_KEY_TYPE_OTAG);
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, OTAGf, pkt_vlan);
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, MODULE_IDf, modid);
    soc_VLAN_XLATEm_field32_set(unit, &ing_vent, PORT_NUMf, port);

    /* TD2 type of device indicating Source Field as SGLP */
    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
        soc_VLAN_XLATEm_field32_set(unit, &ing_vent, SOURCE_TYPEf, 1);
    }

    MEM_LOCK(unit, VLAN_XLATEm);
    
    rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vt_index,
                        &ing_vent, &ing_vent, 0);
    if (rv < 0) {
        MEM_UNLOCK(unit, VLAN_XLATEm);
        return rv;
    }

    /* Get the old vlan translate profile entry */
    ing_profile_idx = soc_VLAN_XLATEm_field32_get(unit, &ing_vent,
                                                  TAG_ACTION_PROFILE_PTRf);

    /* Delete the ingress vlan translate entry */
    rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &ing_vent);
    MEM_UNLOCK(unit, VLAN_XLATEm);
    BCM_IF_ERROR_RETURN(rv);

    /* Delete the old vlan translate profile entry */
    rv = _bcm_trx_vlan_action_profile_entry_delete(unit, ing_profile_idx);
    BCM_IF_ERROR_RETURN(rv);

    /* Clear the EGR_L3_INTF entry */
    sal_memset(&l3_intf, 0, sizeof(egr_l3_intf_entry_t));
    rv = soc_mem_write(unit, EGR_L3_INTFm, MEM_BLOCK_ALL, l3_idx, &l3_intf);

    /* Free the L3_INTF index */
    BCM_L3_INTF_USED_CLR(unit, l3_idx);

    if (soc_feature(unit, soc_feature_gport_service_counters)) {
        bcm_gport_t gport;

        /* Release Service counter, if any */
        BCM_GPORT_SUBPORT_PORT_SET(gport, vp);
        _bcm_esw_flex_stat_handle_free(unit, _bcmFlexStatTypeGport, gport);
    }

    /* Clear the subport usage SW state */
    _BCM_TR2_SUBPORT_ID_CLR(unit, vp);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_CONTROL_LOCK(unit);
    SOC_CONTROL(unit)->scache_dirty = 1;
    SOC_CONTROL_UNLOCK(unit);
#endif

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_subport_port_delete
 * Purpose:
 *      Delete a subport 
 * Parameters:
 *      unit   - (IN) Device Number
 *      port   - (IN) GPORT (generic port) identifier
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_subport_port_delete(int unit, bcm_gport_t port)
{
    egr_l3_intf_entry_t l3_intf;
    int l3_idx, vp=-1, rv;
    bcm_module_t modid, my_modid;

    _TR2_SUBPORT_CHECK_INIT(unit);

    BCM_IF_ERROR_RETURN (bcm_esw_stk_my_modid_get(unit, &my_modid));
    modid = (BCM_GPORT_SUBPORT_PORT_GET(port) >> 12) & SOC_MODID_MAX(unit);
    if (modid != my_modid) {
        return BCM_E_PORT;
    }

    l3_idx = BCM_GPORT_SUBPORT_PORT_GET(port) & 0xfff;
    if (l3_idx >= BCM_XGS3_L3_IF_TBL_SIZE(unit)) { 
        return (BCM_E_PARAM);
    }

    rv = soc_mem_read(unit, EGR_L3_INTFm, MEM_BLOCK_ALL, l3_idx, &l3_intf);
    BCM_IF_ERROR_RETURN(rv);

    
    vp = soc_mem_field32_get(unit, EGR_L3_INTFm, &l3_intf, IVIDf);
    if (_BCM_TR2_SUBPORT_ID_GET(unit, vp) != l3_idx) {
        return BCM_E_NOT_FOUND;
    }


    if (vp == -1) {
        return BCM_E_INTERNAL;
    }

    return _bcm_tr2_subport_port_delete(unit, l3_idx, vp);
}

STATIC int
_bcm_tr2_subport_port_get(int unit, int l3_idx,
                         bcm_subport_config_t *config)
{
    int vp=-1, rv, vp_base, nh_index=-1;
    egr_l3_intf_entry_t l3_intf;
    egr_l3_next_hop_entry_t egr_nh;
    ing_dvp_table_entry_t dvp;
    source_vp_entry_t svp;

    rv = soc_mem_read(unit, EGR_L3_INTFm, MEM_BLOCK_ALL, l3_idx, &l3_intf);
    BCM_IF_ERROR_RETURN(rv);

    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        vp = soc_mem_field32_get(unit, EGR_L3_INTFm, &l3_intf, IVIDf);
        if (_BCM_TR2_SUBPORT_ID_GET(unit, vp) != l3_idx) {
            return BCM_E_NOT_FOUND;
        }
    }

    /* Get the base group VP */
    rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
    BCM_IF_ERROR_RETURN(rv);
    vp_base = soc_SOURCE_VPm_field32_get(unit, &svp, DVPf);

    /* Set the config data to be returned */
    config->int_pri = vp - vp_base;

    /* Get the group's next-hop index */
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        rv = READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp_base, &dvp);
        BCM_IF_ERROR_RETURN(rv);
        nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        /* Get the group's OVID from the group's egress next-hop entry */
        rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm,
                          MEM_BLOCK_ALL, nh_index, &egr_nh);
        BCM_IF_ERROR_RETURN(rv);
        if (soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm, SD_TAG_VIDf)) {
            config->pkt_vlan = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh, SD_TAG_VIDf);
        }
    }
    /* Get the subport group ID */
    BCM_GPORT_SUBPORT_GROUP_SET(config->group, vp_base); 

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_subport_port_get
 * Purpose:
 *      Get a subport
 * Parameters:
 *      unit   - (IN) Device Number
 *      port   - (IN) GPORT (generic port) identifier
 *      config - (OUT) Subport config information
 * Returns:
 *      BCM_E_XXX
 */     
int
bcm_tr2_subport_port_get(int unit, bcm_gport_t port,
                        bcm_subport_config_t *config)
{
    int l3_idx;
    bcm_module_t modid, my_modid;

    _TR2_SUBPORT_CHECK_INIT(unit);

    if (config == NULL) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));
    modid = (BCM_GPORT_SUBPORT_PORT_GET(port) >> 12) & SOC_MODID_MAX(unit);
    if (modid != my_modid) {
        return BCM_E_PORT;
    }

    l3_idx = BCM_GPORT_SUBPORT_PORT_GET(port) & 0xfff;
    if (l3_idx >= BCM_XGS3_L3_IF_TBL_SIZE(unit)) {
        return (BCM_E_PARAM);
    }

    return _bcm_tr2_subport_port_get(unit, l3_idx, config);
}

/*
 * Function:
 *      bcm_subport_port_traverse
 * Purpose:
 *      Traverse all valid subports and call the
 *      supplied callback routine.
 * Parameters:
 *      unit      - Device Number
 *      cb        - User callback function, called once per subport.
 *      user_data - cookie
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_subport_port_traverse(int unit,
                             bcm_subport_port_traverse_cb cb,
                             void *user_data)
{
    int rv, vp, l3_idx;
    bcm_subport_config_t config;
    bcm_gport_t gport;
    bcm_module_t my_modid;

    _TR2_SUBPORT_CHECK_INIT(unit);

    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));

    for (vp = 0; vp < _TR2_SUBPORT_NUM_VP; vp++) {
        l3_idx = _BCM_TR2_SUBPORT_ID_GET(unit, vp);
        if (l3_idx != 0xffff) {
            rv = _bcm_tr2_subport_port_get(unit, l3_idx, &config);
            BCM_IF_ERROR_RETURN(rv);

            BCM_GPORT_SUBPORT_PORT_SET(gport, ((my_modid << 12) | l3_idx));
            rv = cb(unit, gport, &config, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
            if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                return rv;
            }
#endif
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_subport_group_resolve
 * Purpose:
 *      Get the modid, port, trunk values for a subport group GPORT
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_subport_group_resolve(int unit, bcm_gport_t gport,
                              bcm_module_t *modid, bcm_port_t *port,
                              bcm_trunk_t *trunk_id, int *id)
{
    int rv = BCM_E_NONE; 
    ing_l3_next_hop_entry_t ing_nh;
    ing_dvp_table_entry_t dvp;
    int nh_index, vp_base;

    _TR2_SUBPORT_CHECK_INIT(unit);
    
    vp_base = BCM_GPORT_SUBPORT_GROUP_GET(gport);
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        rv = READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp_base, &dvp);
        BCM_IF_ERROR_RETURN(rv);
        nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        BCM_IF_ERROR_RETURN (soc_mem_read(unit, ING_L3_NEXT_HOPm, MEM_BLOCK_ANY,
                                      nh_index, &ing_nh));
        if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, ENTRY_TYPEf) != 0x3) {
            /* Entry type is not GPON DVP */
            return BCM_E_NOT_FOUND;
        }
    }

    if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, Tf)) {
        *trunk_id = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, TGIDf);
    } else {
        *modid = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, MODULE_IDf);
        *port = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, PORT_NUMf);
    }
    *id = vp_base;
    return rv;
}

/*
 * Function:
 *      _bcm_tr_subport_port_resolve
 * Purpose:
 *      Get the modid, port, trunk values for a subport port GPORT
 * Returns:                  
 *      BCM_E_XXX
 */                          
int 
_bcm_tr2_subport_port_resolve(int unit, bcm_gport_t gport,
                             bcm_module_t *modid, bcm_port_t *port,
                             bcm_trunk_t *trunk_id, int *id)
{
    int rv = BCM_E_NONE, l3_idx; 
    ing_l3_next_hop_entry_t ing_nh;
    egr_l3_intf_entry_t l3_intf;
    ing_dvp_table_entry_t dvp;
    int nh_index, vp, vp_base;
    bcm_module_t my_modid;

    _TR2_SUBPORT_CHECK_INIT(unit);

    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));

    *id = BCM_GPORT_SUBPORT_PORT_GET(gport);
    *modid = (*id >> 12) & SOC_MODID_MAX(unit);
    if (*modid != my_modid) {
        return (BCM_E_PORT);
    }

    l3_idx = BCM_GPORT_SUBPORT_PORT_GET(gport) & 0xfff;
    if (l3_idx >= BCM_XGS3_L3_IF_TBL_SIZE(unit)) {
        return (BCM_E_PARAM);
    }
    rv = soc_mem_read(unit, EGR_L3_INTFm, MEM_BLOCK_ALL, l3_idx, &l3_intf);
    BCM_IF_ERROR_RETURN(rv);

    
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        vp = soc_mem_field32_get(unit, EGR_L3_INTFm, &l3_intf, IVIDf);
        vp_base = vp & ~(0x7);
        rv = READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp_base, &dvp);
        BCM_IF_ERROR_RETURN(rv);
        nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        BCM_IF_ERROR_RETURN (soc_mem_read(unit, ING_L3_NEXT_HOPm, MEM_BLOCK_ANY,
                                      nh_index, &ing_nh));
        if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, ENTRY_TYPEf) != 0x3) {
            /* Entry type is not GPON DVP */
            return BCM_E_NOT_FOUND;
        }
    }

    if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, Tf)) {
        *trunk_id = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, TGIDf);
    } else {
        *port = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, PORT_NUMf);
    }
    return rv;
}

/*
 * Function:
 *      bcm_tr_subport_learn_set
 * Purpose:
 *      Set the CML bits for a subport group.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_subport_learn_set(int unit, bcm_gport_t group, uint32 flags)
{
    int vp_base, cml = 0, rv = BCM_E_NONE;
    source_vp_entry_t svp;

    _TR2_SUBPORT_CHECK_INIT(unit);

    cml = 0;
    if (!(flags & BCM_PORT_LEARN_FWD)) {
       cml |= (1 << 0);
    }
    if (flags & BCM_PORT_LEARN_CPU) {
       cml |= (1 << 1);
    }
    if (flags & BCM_PORT_LEARN_PENDING) {
       cml |= (1 << 2);
    }
    if (flags & BCM_PORT_LEARN_ARL) {
       cml |= (1 << 3);
    }

    vp_base = BCM_GPORT_SUBPORT_GROUP_GET(group); 
    if (!_BCM_TR2_GROUP_USED_GET(unit, vp_base / 8)) {
        return BCM_E_NOT_FOUND;
    }
    MEM_LOCK(unit, SOURCE_VPm);
    rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp_base, &svp);
    if (soc_SOURCE_VPm_field32_get(unit, &svp, ENTRY_TYPEf) != 0x3) {
        MEM_UNLOCK(unit, SOURCE_VPm);
        return BCM_E_INTERNAL;
    }
    soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_MOVEf, cml);
    soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_NEWf, cml);
    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp_base, &svp);
    MEM_UNLOCK(unit, SOURCE_VPm);
    return rv;
}

/*
 * Function:
 *      bcm_tr_subport_learn_get
 * Purpose:
 *      Get the CML bits for a subport group.
 * Returns: 
 *      BCM_E_XXX
 */     
int
bcm_tr2_subport_learn_get(int unit, bcm_gport_t group, uint32 *flags)
{
    int vp_base, cml;
    source_vp_entry_t svp; 
    
    _TR2_SUBPORT_CHECK_INIT(unit);

    vp_base = BCM_GPORT_SUBPORT_GROUP_GET(group);
    if (!_BCM_TR2_GROUP_USED_GET(unit, vp_base / 8)) {
        return BCM_E_NOT_FOUND;
    }
    BCM_IF_ERROR_RETURN
        (READ_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp_base, &svp));
    if (soc_SOURCE_VPm_field32_get(unit, &svp, ENTRY_TYPEf) != 0x3) {
        return BCM_E_INTERNAL;
    }
    cml = soc_SOURCE_VPm_field32_get(unit, &svp, CML_FLAGS_NEWf);

    *flags = 0;
    if (!(cml & (1 << 0))) {
       *flags |= BCM_PORT_LEARN_FWD;
    }
    if (cml & (1 << 1)) {
       *flags |= BCM_PORT_LEARN_CPU;
    }
    if (cml & (1 << 2)) {
       *flags |= BCM_PORT_LEARN_PENDING;
    }
    if (cml & (1 << 3)) {
       *flags |= BCM_PORT_LEARN_ARL;
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_esw_subport_port_flex_stat_index_set(int unit, bcm_gport_t gport,
                                          int fs_idx, uint32 flags)
{
    int rv, vp, vp_base, vt_index, nh_index=-1;
    ing_l3_next_hop_entry_t ing_nh;
    egr_l3_next_hop_entry_t egr_nh;
    egr_vlan_xlate_entry_t egr_vent;
    ing_dvp_table_entry_t dvp;
    bcm_port_t port = 0;
    bcm_vlan_t ovid = 0;
    uint32 port_class = 0;

    /* Subport ports only make sense on egress */
    if (!(flags & _BCM_FLEX_STAT_HW_EGRESS)) {
        return BCM_E_PARAM;
    }

    vp = BCM_GPORT_SUBPORT_PORT_GET(gport);

    /* Get the base group VP */
    vp_base = vp & ~(0x7);

    /* Get the group's next-hop index */
    rv = READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp_base, &dvp);
    BCM_IF_ERROR_RETURN(rv);
    nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
    /* Get the group's OVID from the group's egress next-hop entry */
    rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm,
                      MEM_BLOCK_ALL, nh_index, &egr_nh);
    BCM_IF_ERROR_RETURN(rv);
    ovid = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh, OVIDf);

    /* Get the egress port class */
    BCM_IF_ERROR_RETURN (soc_mem_read(unit, ING_L3_NEXT_HOPm, MEM_BLOCK_ANY,
                                      nh_index, &ing_nh));
    if (!soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, Tf)) {
        port = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, PORT_NUMf);
        rv = bcm_esw_port_class_get(unit, port, bcmPortClassVlanTranslateEgress,
                                    &port_class);
        BCM_IF_ERROR_RETURN(rv);
    } 

    /* Find the egress vlan translate entry */
    sal_memset(&egr_vent, 0, sizeof(egr_vent));
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, VALIDf, 1);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, OVIDf, ovid);
    if (soc_feature(unit, soc_feature_subport_enhanced)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, IVIDf, vp);
    }

    soc_EGR_VLAN_XLATEm_field32_set(unit, &egr_vent, PORT_GROUP_IDf, port_class);

    MEM_LOCK(unit, EGR_VLAN_XLATEm);
    rv = soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &vt_index, 
                        &egr_vent, &egr_vent, 0);
    if (rv < 0) {
        MEM_UNLOCK(unit, EGR_VLAN_XLATEm);
        return rv;
    }

    /* Subport ports only make sense on egress */
    if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, USE_VINTF_CTR_IDXf)) {
        soc_mem_field32_set(unit, EGR_VLAN_XLATEm, &egr_vent,
                            USE_VINTF_CTR_IDXf, fs_idx > 0 ? 1 : 0);
    }
    soc_mem_field32_set(unit, EGR_VLAN_XLATEm, &egr_vent, VINTF_CTR_IDXf,
                        fs_idx);
    rv = soc_mem_write(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, vt_index,
                       &egr_vent);
    MEM_UNLOCK(unit, EGR_VLAN_XLATEm);
    return rv;
}

STATIC int
_bcm_esw_subport_group_flex_stat_index_set(int unit, bcm_gport_t port,
                                          int fs_idx, uint32 flags)
{
    int vp_base, rv;
    source_vp_entry_t vp_ent;

    vp_base = BCM_GPORT_SUBPORT_GROUP_GET(port);
    if (!_BCM_TR2_GROUP_USED_GET(unit, (vp_base / 8))) {
        return BCM_E_NOT_FOUND;
    }

    if (!(flags & _BCM_FLEX_STAT_HW_INGRESS)) {
        return BCM_E_PARAM;
    }

    /* Subport groups only make sense on ingress */
    /* Write stat index to SOURCE_VP table entry */
    rv = soc_mem_read(unit, SOURCE_VPm, MEM_BLOCK_ALL, vp_base, &vp_ent);
    if (BCM_SUCCESS(rv)) {
        if (soc_mem_field_valid(unit, SOURCE_VPm, USE_VINTF_CTR_IDXf)) {
            soc_mem_field32_set(unit, SOURCE_VPm, &vp_ent, USE_VINTF_CTR_IDXf,
                                fs_idx > 0 ? 1 : 0);
        }
        soc_mem_field32_set(unit, SOURCE_VPm, &vp_ent, VINTF_CTR_IDXf, fs_idx);
        rv = soc_mem_write(unit, SOURCE_VPm, MEM_BLOCK_ALL, vp_base, &vp_ent);
    }
    return rv;
}

int
_bcm_esw_subport_flex_stat_index_set(int unit, bcm_gport_t port,
                                          int fs_idx,uint32 flags)
{
    int rv = BCM_E_NONE;

    

    _TR2_SUBPORT_VP_MEM_LOCK(unit);

    if (BCM_GPORT_IS_SUBPORT_PORT(port)) {
        rv = _bcm_esw_subport_port_flex_stat_index_set(unit, port, fs_idx,flags);
    } else if (BCM_GPORT_IS_SUBPORT_GROUP(port)) {
        rv = _bcm_esw_subport_group_flex_stat_index_set(unit, port, fs_idx,flags);
    }

    _TR2_SUBPORT_VP_MEM_UNLOCK(unit);
    return rv;
}

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *      _bcm_tr2_subport_sw_dump
 * Purpose:
 *      Print Subport module s/w state
 * Returns: 
 *      void
 */     
void
_bcm_tr2_subport_sw_dump(int unit)
{
    int idx, sub_prt;
    soc_cm_print("SOC Feature: Subport Enhanced.\n");
    soc_cm_print("Subport Groups:\n");
    for(idx = 0; idx < _TR2_SUBPORT_NUM_GROUP; idx++) {
        if(_BCM_TR2_GROUP_USED_GET(unit, idx)) {
            soc_cm_print("%d ", idx);
        }
    }

    soc_cm_print("\n----\n");

    soc_cm_print("Subport IDs used:\n");
    for(idx = 0; idx < _TR2_SUBPORT_NUM_VP; idx++) {
        if((sub_prt = _BCM_TR2_SUBPORT_ID_GET(unit, idx)) != 0xffff) {
            soc_cm_print("Subport ID=%d, VP=%d \n", sub_prt, idx);
        }
    }
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#else  /* INCLUDE_L3 && BCM_TRIUMPH2_SUPPORT */
int bcm_esw_triumph2_subport_not_empty;
#endif /* INCLUDE_L3 && BCM_TRIUMPH2_SUPPORT */
