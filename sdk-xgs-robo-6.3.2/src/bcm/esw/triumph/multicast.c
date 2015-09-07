/*
 * $Id: multicast.c 1.105 Broadcom SDK $
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
 * File:    multicast.c
 * Purpose: Manages multicast functions
 */

#include <soc/defs.h>
#include <sal/core/libc.h>

#if defined(BCM_TRX_SUPPORT) && defined(INCLUDE_L3)

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/debug.h>
#include <soc/triumph.h>

#include <bcm/error.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/l3.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/triumph.h>
#ifdef BCM_TRIUMPH2_SUPPORT
#include <bcm_int/esw/triumph2.h>
#endif
#include <bcm_int/esw/xgs3.h>
#include <bcm_int/common/multicast.h>
#include <bcm_int/esw/multicast.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/ipmc.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/switch.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/api_xlate_port.h>
#include <bcm_int/esw_dispatch.h>
#ifdef BCM_TRIDENT_SUPPORT
#include <bcm_int/esw/trill.h>
#include <bcm_int/esw/trident.h>
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIDENT2_SUPPORT
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT */
#include <bcm_int/esw/failover.h>
#include <bcm_int/esw/mpls.h>

uint8 *_multicast_ipmc_group_types[BCM_MAX_NUM_UNITS];

int
_bcm_tr_multicast_ipmc_group_type_set(int unit, bcm_multicast_t group)
{
    uint8 group_type = _BCM_MULTICAST_TYPE_GET(group);
    uint32 ipmc_id = _BCM_MULTICAST_ID_GET(group);

    /* Don't check if MULTICAST_GROUP_IS_SET because we also want to
     * be able to clear a group. */

    if ((_BCM_MULTICAST_TYPE_L2 != group_type)) {
        if ((ipmc_id < soc_mem_index_min(unit, L3_IPMCm)) ||
            (ipmc_id > soc_mem_index_max(unit, L3_IPMCm))) {
            return BCM_E_PARAM;
        }

        /* It's an IPMC type group */
        _multicast_ipmc_group_types[unit][ipmc_id] = group_type;

#ifdef BCM_WARM_BOOT_SUPPORT
        SOC_CONTROL_LOCK(unit);
        SOC_CONTROL(unit)->scache_dirty = 1;
        SOC_CONTROL_UNLOCK(unit);
#endif
    }

    return BCM_E_NONE;
}

int
_bcm_tr_multicast_ipmc_group_type_get(int unit, uint32 ipmc_id,
                                      bcm_multicast_t *group)
{
    if (SOC_IS_XGS3_FABRIC(unit)) {
        /* _multicast_ipmc_group_types not initialized for fabric chips. */
        *group = 0;
        return BCM_E_NOT_FOUND;
    }

    if ((NULL != _multicast_ipmc_group_types[unit]) && /* Verify init */
        (0 != _multicast_ipmc_group_types[unit][ipmc_id])) {
        _BCM_MULTICAST_GROUP_SET(*group,
                                 _multicast_ipmc_group_types[unit][ipmc_id],
                                 ipmc_id);
        return BCM_E_NONE;
    } else {
        *group = 0;
        return BCM_E_NOT_FOUND;
    }
}

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_0

/*
 * Function:
 *      _bcm_trx_multicast_sync
 * Purpose:
 *      Record Multicast module persisitent info for Level 2 Warm Boot
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_multicast_sync(int unit)
{
    int ipmc_mem_size, alloc_size;
    soc_scache_handle_t scache_handle;
    uint8               *multicast_scache;

    if (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
        return BCM_E_NONE;
    }

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_MULTICAST, 0);
    BCM_IF_ERROR_RETURN
        (_bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 0,
                                 &multicast_scache, 
                                 BCM_WB_DEFAULT_VERSION, NULL));
    ipmc_mem_size = soc_mem_index_count(unit, L3_IPMCm);
    alloc_size = ipmc_mem_size * sizeof(uint8);

    if (_multicast_ipmc_group_types[unit] != NULL) {
        sal_memcpy(multicast_scache, _multicast_ipmc_group_types[unit],
                   alloc_size);
    }
    
    return BCM_E_NONE;
}

typedef struct _bcm_esw_multicast_cb_info_s {
    uint32 flags;
    SHR_BITDCL *ipmc_groups_bits;
    int stale_scache;
} _bcm_multicast_cb_info_t;

/*
 * Function:
 *      _bcm_trx_multicast_reinit_group
 * Purpose:
 *      Parse multicast group type and reinitialize the internal structure.
 * Parameters:
 *      unit      - (IN) Device Number
 *      group     - (IN) Multicast group ID
 *      mc_cb_info - (IN) data structure pointer passed via the traverse
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_trx_multicast_reinit_group(int unit, bcm_multicast_t group,
                                _bcm_multicast_cb_info_t *mc_cb_info)
{
    int rv, mc_index;
    uint32 group_flags;
    uint8 group_type;
    bcm_multicast_t scache_group;

    mc_index = _BCM_MULTICAST_ID_GET(group);

    if (SHR_BITGET(mc_cb_info->ipmc_groups_bits, mc_index)) {
        /* Already did this group index, skip */
        return BCM_E_NONE;
    }

    group_type = _BCM_MULTICAST_TYPE_GET(group);

    if (_BCM_MULTICAST_TYPE_L2 == group_type) {
        /* This shouldn't happen */
        return BCM_E_INTERNAL;
    }

    rv  = _bcm_tr_multicast_ipmc_group_type_get(unit, mc_index,
                                                &scache_group);

    if (BCM_E_NOT_FOUND == rv) {
        /* Normal operation at UNCACHED Warm Boot */
    } else if (BCM_FAILURE(rv)) {
        return rv;
    } else if (scache_group != group) {
        if (FALSE == mc_cb_info->stale_scache) {
            if (_BCM_MULTICAST_TYPE_L3 != group_type) {
                /* We have Level 2 info which tells us it isn't,
                 * L3 type, so go with the saved info. */
            } else {
                /* Stale scache for Warm Boot */
                /* We haven't yet complained about the stale scache */
                SOC_IF_ERROR_RETURN
                    (soc_event_generate(unit, SOC_SWITCH_EVENT_STABLE_ERROR, 
                                        SOC_STABLE_STALE,
                                        SOC_STABLE_MULTICAST, 0));
                mc_cb_info->stale_scache = TRUE;
            }
        } /* Else, record this as L3 type */
    } else {
        /* Synchronized scache from Warm Boot Level 2.  Nothing to do */
        return BCM_E_NONE;
    }

    group_flags = _bcm_esw_multicast_group_type_to_flags(group_type);
    /* We record the used group ID's to determine
     * which ones are pure L3. */
    SHR_BITSET(mc_cb_info->ipmc_groups_bits, mc_index);

    if (mc_cb_info->flags & group_flags) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr_multicast_ipmc_group_type_set(unit, group));
    } else {
        /* We shouldn't have this type of group for this device! */
        return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trx_multicast_l2_traverse
 * Purpose:
 *      Callback for L2 traverse to parse multicast groups.
 * Parameters:
 *      unit      - (IN) Device Number
 *      info      - (IN) L2 addr data structure, including multicast group
 *      user_data - (IN) data structure pointer passed via the traverse
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_trx_multicast_l2_traverse(int unit, bcm_l2_addr_t *info, 
                               void *user_data)
{
    _bcm_multicast_cb_info_t *mc_cb_info =
        (_bcm_multicast_cb_info_t *)user_data;
    
    if (!_BCM_MULTICAST_IS_SET(info->l2mc_index)) {
        /* Not interested */
        return BCM_E_NONE;
    }

    return _bcm_trx_multicast_reinit_group(unit, info->l2mc_index,
                                           mc_cb_info);
}

#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
/*
 * Function:
 *      _bcm_trx_multicast_vlan_traverse
 * Purpose:
 *      Iterate over VLANs and check for multicast groups.
 * Parameters:
 *      unit      - (IN) Device Number
 *      mc_cb_info - (IN) data structure pointer passed via the traverse
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_trx_multicast_vlan_traverse(int unit,
                                  _bcm_multicast_cb_info_t *mc_cb_info)
{
    bcm_vlan_t vlan;
    int rv;
    bcm_vlan_control_vlan_t control;

    for (vlan = BCM_VLAN_MIN; vlan < BCM_VLAN_COUNT; vlan++) {
        rv = bcm_esw_vlan_control_vlan_get(unit, vlan, &control);
        if (BCM_E_PARAM == rv) {
            /* VLAN isn't created, skip onward */
            continue;
        } else if (BCM_FAILURE(rv)) {
            return rv;
        } else {
            /* Found a VLAN, check it's parameters */
            if (_BCM_MULTICAST_IS_SET(control.broadcast_group)) {
                BCM_IF_ERROR_RETURN
                    (_bcm_trx_multicast_reinit_group(unit,
                              control.broadcast_group, mc_cb_info));
            }
            if (_BCM_MULTICAST_IS_SET(control.unknown_multicast_group)) {
                BCM_IF_ERROR_RETURN
                    (_bcm_trx_multicast_reinit_group(unit,
                              control.unknown_multicast_group, mc_cb_info));
            }
            if (_BCM_MULTICAST_IS_SET(control.broadcast_group)) {
                BCM_IF_ERROR_RETURN
                    (_bcm_trx_multicast_reinit_group(unit,
                              control.unknown_unicast_group, mc_cb_info));
            }
        }
    }

    return BCM_E_NONE;
}
#endif

/*
 * Function:
 *      _bcm_trx_multicast_vfi_traverse
 * Purpose:
 *      Iterate over VFIs and check for multicast groups.
 * Parameters:
 *      unit      - (IN) Device Number
 *      mc_cb_info - (IN) data structure pointer passed via the traverse
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_trx_multicast_vfi_traverse(int unit,
                                _bcm_multicast_cb_info_t *mc_cb_info)
{
    int vfi, num_vfi;
    int mc_index;
    uint32 multicast_type;
    vfi_entry_t vfi_entry;
    bcm_multicast_t group;

    if (!SOC_MEM_IS_VALID(unit,VFIm)) {
       /* if (SOC_IS_HURRICANE(unit))  */
       return BCM_E_NONE;
    }

    num_vfi = soc_mem_index_count(unit, VFIm);
    for (vfi = 0; vfi < num_vfi; vfi++) {
        if (_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeMpls)) {
            multicast_type = _BCM_MULTICAST_TYPE_VPLS;
#ifdef BCM_TRIUMPH2_SUPPORT
        } else if (_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeMim)) {
            multicast_type = _BCM_MULTICAST_TYPE_MIM;
#endif
        } else {
            /* Not a VFI we're interested in */
            continue;
        }

        BCM_IF_ERROR_RETURN
            (READ_VFIm(unit, MEM_BLOCK_ALL, vfi, &vfi_entry));
        
#ifdef BCM_TRIUMPH2_SUPPORT
        if (soc_feature(unit, soc_feature_mpls_enhanced)) {
            mc_index = soc_VFIm_field32_get(unit, &vfi_entry, BC_INDEXf);
            _BCM_MULTICAST_GROUP_SET(group, multicast_type, mc_index);
            BCM_IF_ERROR_RETURN
                (_bcm_trx_multicast_reinit_group(unit,
                                                    group, mc_cb_info));

            mc_index = soc_VFIm_field32_get(unit, &vfi_entry, UUC_INDEXf);
            _BCM_MULTICAST_GROUP_SET(group, multicast_type, mc_index);
            BCM_IF_ERROR_RETURN
                (_bcm_trx_multicast_reinit_group(unit,
                                                    group, mc_cb_info));

            mc_index = soc_VFIm_field32_get(unit, &vfi_entry, UMC_INDEXf);
            _BCM_MULTICAST_GROUP_SET(group, multicast_type, mc_index);
            BCM_IF_ERROR_RETURN
                (_bcm_trx_multicast_reinit_group(unit,
                                                    group, mc_cb_info));
        } else
#endif
        {
            mc_index = soc_VFIm_field32_get(unit, &vfi_entry, L3MC_INDEXf);
            _BCM_MULTICAST_GROUP_SET(group, multicast_type, mc_index);
            BCM_IF_ERROR_RETURN
                (_bcm_trx_multicast_reinit_group(unit,
                                                    group, mc_cb_info));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trx_multicast_reinit
 * Purpose:
 *      Recover Multicast module state for Warm Boot
 * Parameters:
 *      unit - StrataSwitch unit number.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_trx_multicast_reinit(int unit)
{
    int mem_size=0, mc_index, is_set, rv = BCM_E_NONE;
    int alloc_size;
    soc_scache_handle_t scache_handle;
    bcm_multicast_t group, scache_group;
    SHR_BITDCL *ipmc_groups_bits = NULL;
    uint8               *multicast_scache;
    _bcm_multicast_cb_info_t mc_cb_info;
    int num_ipmc_groups;

    /* L3 IPMC info may use scache info */
    mem_size = soc_mem_index_count(unit, L3_IPMCm);

    alloc_size = mem_size * sizeof(uint8);
    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_MULTICAST, 0);
    rv = _bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 0, &multicast_scache, 
                                 BCM_WB_DEFAULT_VERSION, NULL);

    if (BCM_E_NOT_FOUND == rv) {
        multicast_scache = NULL;
    } else if (BCM_FAILURE(rv)) {
        return rv;
    } else {
        sal_memcpy(_multicast_ipmc_group_types[unit], multicast_scache,
                   alloc_size);
    }

    /* Traversing multiple next hop types, set up IPMC repl logging */
    alloc_size = SHR_BITALLOCSIZE(mem_size);
    ipmc_groups_bits = sal_alloc(alloc_size, "IPMC groups referenced");
    if (ipmc_groups_bits == NULL) {
        bcm_fb_ipmc_repl_detach(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(ipmc_groups_bits, 0, alloc_size);    

    /* Store info for callback functions */
    mc_cb_info.ipmc_groups_bits = ipmc_groups_bits;        
    mc_cb_info.stale_scache = FALSE;

    mc_cb_info.flags = BCM_MULTICAST_TYPE_L3;

    if (soc_feature(unit, soc_feature_mpls)) {
        mc_cb_info.flags |= BCM_MULTICAST_TYPE_VPLS;
    }
    if (soc_feature(unit, soc_feature_subport)) {
        mc_cb_info.flags |= BCM_MULTICAST_TYPE_SUBPORT;
    }
    if (soc_feature(unit, soc_feature_mim)) {
        mc_cb_info.flags |= BCM_MULTICAST_TYPE_MIM;
    }
    if (soc_feature(unit, soc_feature_wlan)) {
        mc_cb_info.flags |= BCM_MULTICAST_TYPE_WLAN;
    }
    if (soc_feature(unit, soc_feature_vlan_vp)) {
        mc_cb_info.flags |= BCM_MULTICAST_TYPE_VLAN;
    }
    if (soc_feature(unit, soc_feature_trill)) {
        mc_cb_info.flags |= BCM_MULTICAST_TYPE_TRILL;
    }
    if (soc_feature(unit, soc_feature_niv)) {
        mc_cb_info.flags |= BCM_MULTICAST_TYPE_NIV;
    }
    if (soc_feature(unit, soc_feature_mpls)) {
        mc_cb_info.flags |= BCM_MULTICAST_TYPE_EGRESS_OBJECT;
    }
    if (soc_feature(unit, soc_feature_port_extension)) {
        mc_cb_info.flags |= BCM_MULTICAST_TYPE_EXTENDER;
    }

    /* Traverse L2 table for multicast groups */
    rv = bcm_esw_l2_traverse(unit, &_bcm_trx_multicast_l2_traverse,
                             &mc_cb_info);

#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3)
    /* Traverse VLAN MC groups */
    if (BCM_SUCCESS(rv) && (soc_feature(unit, soc_feature_wlan) ||
                soc_feature(unit, soc_feature_vlan_vp) ||
                soc_feature(unit, soc_feature_trill) ||
                soc_feature(unit, soc_feature_niv) ||
                soc_feature(unit, soc_feature_port_extension))) {
        rv = _bcm_trx_multicast_vlan_traverse(unit, &mc_cb_info);
    }
#endif

    /* Traverse VFI groups */
    if (BCM_SUCCESS(rv)) {
        if (soc_feature(unit, soc_feature_virtual_switching)) {
            rv = _bcm_trx_multicast_vfi_traverse(unit, &mc_cb_info);
        }
    }

    if (BCM_SUCCESS(rv)) {

        num_ipmc_groups = mem_size;
#if defined(BCM_TRIUMPH3_SUPPORT) 
        if (soc_feature(unit, soc_feature_static_repl_head_alloc)) {
            soc_info_t *si;
            int member_count;
            int port, phy_port, mmu_port;
            /* Each replication group is statically allocated a region
             * in REPL_HEAD table. The size of the region depends on the
             * maximum number of valid ports. Thus, the max number of
             * replication groups is limited to number of REPL_HEAD entries
             * divided by the max number of valid ports.
             */
            si = &SOC_INFO(unit);
            member_count = 0;
            PBMP_ITER(SOC_CONTROL(unit)->repl_eligible_pbmp, port) {
                phy_port = si->port_l2p_mapping[port];
                mmu_port = si->port_p2m_mapping[phy_port];
                if ((mmu_port == 57) || (mmu_port == 59) || (mmu_port == 62)) {
                    /* No replication on MMU ports 57, 59, and 62 */
                    continue;
                }
                member_count++;
            }
            if (member_count > 0) {
                num_ipmc_groups = soc_mem_index_count(unit, MMU_REPL_HEAD_TBLm) /
                    member_count;
                if (num_ipmc_groups > mem_size) {
                    num_ipmc_groups = mem_size;
                }
            }
        }
#endif /* BCM_TRIUMPH3_SUPPORT */

        for (mc_index = 0; mc_index < num_ipmc_groups; mc_index++) {
            if (SHR_BITGET(ipmc_groups_bits, mc_index)) {
                /* Already did this group index, skip */
                continue;
            }

            rv = bcm_xgs3_ipmc_id_is_set(unit, mc_index, &is_set);
            if (BCM_E_INIT == rv) {
                
                rv = BCM_E_NONE;
                break;
            } else if (BCM_FAILURE(rv)) {
                break;
            } else if (is_set) {
                if (NULL != multicast_scache) {
                    /* If scache was used, check for staleness */
                    rv  = _bcm_tr_multicast_ipmc_group_type_get(unit,
                                            mc_index, &scache_group);

                    if (BCM_E_NOT_FOUND == rv) {
#ifdef BCM_KATANA_SUPPORT
                        if (SOC_IS_KATANA(unit) &&
                            ((mc_index == 0) || (mc_index == 1))) {
                            continue; 
                        }
#endif /* BCM_KATANA_SUPPORT */

                        /* We expect a group from the HW state, but
                         * it isn't in the scache info */
                        if (FALSE == mc_cb_info.stale_scache) {
                            /* We have yet to complain about stale scache */
                            rv = soc_event_generate(unit,
                                           BCM_SWITCH_EVENT_STABLE_ERROR, 
                                                    SOC_STABLE_STALE,
                                                    SOC_STABLE_MULTICAST, 0);
                            if (BCM_FAILURE(rv)) {
                                break;
                            }
                            mc_cb_info.stale_scache = TRUE;
                        }
                    } else if (BCM_FAILURE(rv)) {
                        return rv;
                    } /* Keep the cached group info and continue */
                } else {
                    /* Best guess is that this is an L3 group */
                    _BCM_MULTICAST_GROUP_SET(group, 
                                             _BCM_MULTICAST_TYPE_L3,
                                             mc_index);
                    rv = _bcm_trx_multicast_reinit_group(unit, group,
                                                         &mc_cb_info);
                    if (BCM_FAILURE(rv)) {
                        break;
                    }
                }
            } else if (NULL != multicast_scache) {
                /* If scache was used, check for staleness */
                rv  = _bcm_tr_multicast_ipmc_group_type_get(unit, mc_index,
                                                            &scache_group);

                if (BCM_E_NOT_FOUND == rv) {
                    /* In sync */
                    rv = BCM_E_NONE;
                } else if (BCM_FAILURE(rv)) {
                    return rv;
                } else if (0 != scache_group) {
                    if (!_BCM_MULTICAST_IS_L3(scache_group)) {
                        /* We expect a group from the scache info, but
                         * it wasn't recovered from HW. */
                        if (FALSE == mc_cb_info.stale_scache) {
                            /* We have yet to complain about stale scache */
                            rv = soc_event_generate(unit,
                                           BCM_SWITCH_EVENT_STABLE_ERROR, 
                                                    SOC_STABLE_STALE,
                                                    SOC_STABLE_MULTICAST, 0);
                            if (BCM_FAILURE(rv)) {
                                break;
                            }
                            mc_cb_info.stale_scache = TRUE;
                        }
                    } else {
                        /* This is an L3 group that was created but
                         * not installed, so there is no HW sign of it.
                         * We need to mark it recovered in the IPMC area.
                         */
                        rv = bcm_xgs3_ipmc_id_alloc(unit, mc_index);
                        if (BCM_FAILURE(rv)) {
                            break;
                        }
                    }
                }
            }
        }
    }

    sal_free(ipmc_groups_bits);

    return rv;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      bcm_trx_multicast_init
 * Purpose:
 *      Initialize the multicast module.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_trx_multicast_init(int unit)
{
    int ipmc_mem_size=0, alloc_size;
#ifdef BCM_WARM_BOOT_SUPPORT
    int rv = BCM_E_NONE;
    soc_scache_handle_t scache_handle;
    uint8               *multicast_scache;
#endif /* BCM_WARM_BOOT_SUPPORT */

    ipmc_mem_size = soc_mem_index_count(unit, L3_IPMCm);
    alloc_size = ipmc_mem_size * sizeof(uint8);
#ifdef BCM_WARM_BOOT_SUPPORT
    if (!SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
        SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_MULTICAST, 0);
        rv = _bcm_esw_scache_ptr_get(unit, scache_handle,
                                     (0 == SOC_WARM_BOOT(unit)),
                                     alloc_size,
                                     &multicast_scache, 
                                     BCM_WB_DEFAULT_VERSION, NULL);
        if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
            return rv;
        }
    } /* Else, not enough memory, don't allocate scache */
#endif /* BCM_WARM_BOOT_SUPPORT */

    if (NULL == _multicast_ipmc_group_types[unit]) {
        _multicast_ipmc_group_types[unit] =
            sal_alloc(alloc_size, "multicast_group_types");
        if (NULL == _multicast_ipmc_group_types[unit]) {
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_multicast_ipmc_group_types[unit], 0, alloc_size);

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_virtual_port_routing)) {
        BCM_IF_ERROR_RETURN(bcm_td2_multicast_l3_vp_init(unit));
    }
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        return (_bcm_trx_multicast_reinit(unit));
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_trx_multicast_detach
 * Purpose:
 *      Shut down (uninitialize) the multicast module.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_trx_multicast_detach(int unit)
{

#ifdef BCM_WARM_BOOT_SUPPORT
    int autosync;
    BCM_IF_ERROR_RETURN(
        bcm_esw_switch_control_get(unit, bcmSwitchControlAutoSync, &autosync));
    if (autosync) {
        BCM_IF_ERROR_RETURN(_bcm_trx_multicast_sync(unit));
    }
#endif /* BCM_WARM_BOOT_SUPPORT */
    
    if (NULL != _multicast_ipmc_group_types[unit]) {
        sal_free(_multicast_ipmc_group_types[unit]);
        _multicast_ipmc_group_types[unit] = NULL;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_multicast_vpls_encap_get
 * Purpose:
 *      Get the Encap ID for a MPLS port.
 * Parameters:
 *      unit         - (IN) Unit number.
 *      group        - (IN) Multicast group ID.
 *      port         - (IN) Physical port.
 *      mpls_port_id - (IN) MPLS port ID.
 *      encap_id     - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_tr_multicast_vpls_encap_get(int unit, bcm_multicast_t group,
                                bcm_gport_t port,
                                bcm_gport_t mpls_port_id, bcm_if_t *encap_id)
{
#if defined(BCM_MPLS_SUPPORT) 
    int vp;
    ing_dvp_table_entry_t dvp;
    int vpless_failover_port = FALSE;
    int rv = BCM_E_NONE;

    if (!BCM_GPORT_IS_MPLS_PORT(mpls_port_id)) {
        return BCM_E_PARAM;
    }
    if (_BCM_MPLS_GPORT_FAILOVER_VPLESS_GET(mpls_port_id)) {
        vpless_failover_port = TRUE;
    }

    vp = BCM_GPORT_MPLS_PORT_ID_GET(mpls_port_id); 
    if (vp >= soc_mem_index_count(unit, SOURCE_VPm)) {
        return BCM_E_PARAM;
    }
    BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));

    /* Next-hop index is used for multicast replication */
    *encap_id = (bcm_if_t) soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
    if (vpless_failover_port == TRUE) {
        int failover_nh_index=0;
        int failover_id =0;
        int failover_mc_group=0;

        rv = _bcm_esw_failover_prot_nhi_get (unit, *encap_id,
              &failover_id, &failover_nh_index, &failover_mc_group);
        if (BCM_SUCCESS(rv)) {
            if (failover_nh_index) {
            *encap_id = failover_nh_index;
            }
        } else if (rv == BCM_E_UNAVAIL) {
            if (soc_feature(unit, soc_feature_mpls_software_failover)) {
                rv = _bcm_tr_mpls_vpless_failover_nh_index_find(unit,vp,
                               *encap_id, &failover_nh_index);
                if (BCM_SUCCESS(rv)) {
                    *encap_id = failover_nh_index;
                }
            }
        }
    }
    return rv;
#else
    return BCM_E_UNAVAIL;
#endif
}

/*
 * Function:
 *      bcm_multicast_subport_encap_get
 * Purpose:
 *      Get the Encap ID for a subport.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      group     - (IN) Multicast group ID.
 *      port      - (IN) Physical port.
 *      subport   - (IN) Subport ID.
 *      encap_id  - (OUT) Encap ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_tr_multicast_subport_encap_get(int unit, bcm_multicast_t group,
                                   bcm_gport_t port,
                                   bcm_gport_t subport, bcm_if_t *encap_id)
{

    if (!BCM_GPORT_IS_SUBPORT_PORT(subport)) {
        return BCM_E_PARAM;
    }

    /* Egress L3 interface is used for multicast replication */
    *encap_id = (bcm_if_t) BCM_GPORT_SUBPORT_PORT_GET(subport);
    if (*encap_id >= BCM_XGS3_L3_IF_TBL_SIZE(unit)) {
        return (BCM_E_PARAM);
    }
    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_trx_multicast_group_traverse
 * Purpose:
 *      Iterate over the defined multicast groups of the type
 *      specified in 'flags'.  If all types are desired, use
 *      MULTICAST_TYPE_MASK.
 * Parameters:
 *      unit - (IN) Unit number.
 *      trav_fn - (IN) Callback function.
 *      flags - (IN) BCM_MULTICAST_*
 *      user_data - (IN) User data to be passed to callback function.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_trx_multicast_group_traverse(int unit,
                                 bcm_multicast_group_traverse_cb_t trav_fn, 
                                 uint32 flags, void *user_data)
{
    int l2mc_size, l3mc_size, mc_index, is_set, rv = BCM_E_NONE;
    uint32 group_flags, flags_mask;
    bcm_multicast_t group;

    flags_mask = BCM_MULTICAST_TYPE_L2 | BCM_MULTICAST_TYPE_L3;

    if (soc_feature(unit, soc_feature_mpls)) {
        flags_mask |= BCM_MULTICAST_TYPE_VPLS;
    }
    if (soc_feature(unit, soc_feature_subport)) {
        flags_mask |= BCM_MULTICAST_TYPE_SUBPORT;
    }
    if (soc_feature(unit, soc_feature_mim)) {
        flags_mask |= BCM_MULTICAST_TYPE_MIM;
    }
    if (soc_feature(unit, soc_feature_wlan)) {
        flags_mask |= BCM_MULTICAST_TYPE_WLAN;
    }
    if (soc_feature(unit, soc_feature_vlan_vp)) {
        flags_mask |= BCM_MULTICAST_TYPE_VLAN;
    }
    if (soc_feature(unit, soc_feature_trill)) {
        flags_mask |= BCM_MULTICAST_TYPE_TRILL;
    }
    if (soc_feature(unit, soc_feature_niv)) {
        flags_mask |= BCM_MULTICAST_TYPE_NIV;
    }
    if (soc_feature(unit, soc_feature_mpls)) {
        flags_mask |= BCM_MULTICAST_TYPE_EGRESS_OBJECT;
    }
    if (soc_feature(unit, soc_feature_port_extension)) {
        flags_mask |= BCM_MULTICAST_TYPE_EXTENDER;
    }
    if (soc_feature(unit, soc_feature_vxlan)) {
        flags_mask |= BCM_MULTICAST_TYPE_VXLAN;
    }

    if (0 == (flags & flags_mask)) {
        /* No recognized multicast types to traverse */
        return BCM_E_PARAM;
    }

    flags &= flags_mask;

    /* Replace the L2 MC logic */
    if (flags & BCM_MULTICAST_TYPE_L2) {
        BCM_IF_ERROR_RETURN(_bcm_xgs3_l2mc_size_get(unit, &l2mc_size));
        group_flags = BCM_MULTICAST_TYPE_L2 | BCM_MULTICAST_WITH_ID;

        for (mc_index = 0; mc_index < l2mc_size; mc_index++) {
            rv = _bcm_xgs3_l2mc_index_is_set(unit, mc_index, &is_set);
            if (BCM_FAILURE(rv)) {
                return rv;
            } else if (is_set) {
                _BCM_MULTICAST_GROUP_SET(group, _BCM_MULTICAST_TYPE_L2,
                                         mc_index);
                BCM_IF_ERROR_RETURN
                    ((*trav_fn)(unit, group, group_flags, user_data));
            }
        }
    }

    if (flags & ~(BCM_MULTICAST_TYPE_L2)) {
        l3mc_size = soc_mem_index_count(unit, L3_IPMCm);
        for (mc_index = 0; mc_index < l3mc_size; mc_index++) {
            rv = _bcm_tr_multicast_ipmc_group_type_get(unit, mc_index,
                                                       &group);

            if (BCM_E_NOT_FOUND == rv) {
                continue;
            } else if (BCM_FAILURE(rv)) {
                return rv;
            } else {
                group_flags =
                    _bcm_esw_multicast_group_type_to_flags(
                         _BCM_MULTICAST_TYPE_GET(group)) |
                    BCM_MULTICAST_WITH_ID;

                if (group_flags & flags) {
                    BCM_IF_ERROR_RETURN
                        ((*trav_fn)(unit, group, group_flags, user_data));
                }
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_trx_multicast_group_get
 * Purpose:
 *      Retrieve the flags for a multicast group, if it exists.
 * Parameters:
 *      unit       - (IN)   Device Number
 *      group      - (IN)  Group ID
 *      flags      - (OUT)   BCM_MULTICAST_*
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_trx_multicast_group_get(int unit, bcm_multicast_t group, uint32 *flags)
{
    int rv = BCM_E_NONE, mc_index, is_set = FALSE;
    bcm_multicast_t get_group;

    mc_index = _BCM_MULTICAST_ID_GET(group);
    if (_BCM_MULTICAST_IS_L2(group)) {
        BCM_IF_ERROR_RETURN
            (_bcm_xgs3_l2mc_index_is_set(unit, mc_index, &is_set));
        if (is_set) {
            *flags |= BCM_MULTICAST_TYPE_L2 | BCM_MULTICAST_WITH_ID;
        } else {
            *flags = 0;
            return BCM_E_NOT_FOUND;
        }
    } else {
        if (!_BCM_MULTICAST_IS_L3(group) &&
            !_BCM_MULTICAST_IS_VPLS(group) && 
            !_BCM_MULTICAST_IS_MIM(group) &&
            !_BCM_MULTICAST_IS_SUBPORT(group) &&
            !_BCM_MULTICAST_IS_WLAN(group) &&
            !_BCM_MULTICAST_IS_VLAN(group) &&
            !_BCM_MULTICAST_IS_TRILL(group) &&
            !_BCM_MULTICAST_IS_NIV(group) &&
            !_BCM_MULTICAST_IS_L2GRE(group) &&
            !_BCM_MULTICAST_IS_VXLAN(group) &&
            !_BCM_MULTICAST_IS_EGRESS_OBJECT(group) &&
            !_BCM_MULTICAST_IS_EXTENDER(group)) {
            return BCM_E_PARAM;
        }

        if (_BCM_MULTICAST_IS_VPLS(group) &&
            !soc_feature(unit, soc_feature_mpls)) {
            return BCM_E_UNAVAIL;
        }
        if (_BCM_MULTICAST_IS_SUBPORT(group) &&
            !soc_feature(unit, soc_feature_subport)) {
            return BCM_E_UNAVAIL;
        }
        if (_BCM_MULTICAST_IS_MIM(group) &&
            !soc_feature(unit, soc_feature_mim)) {
            return BCM_E_UNAVAIL;
        }
        if (_BCM_MULTICAST_IS_WLAN(group) &&
            !soc_feature(unit, soc_feature_wlan)) {
            return BCM_E_UNAVAIL;
        }
        if (_BCM_MULTICAST_IS_VLAN(group) &&
            !soc_feature(unit, soc_feature_vlan_vp)) {
            return BCM_E_UNAVAIL;
        }
        if (_BCM_MULTICAST_IS_TRILL(group) &&
            !soc_feature(unit, soc_feature_trill)) {
            return BCM_E_UNAVAIL;
        }
        if (_BCM_MULTICAST_IS_NIV(group) &&
            !soc_feature(unit, soc_feature_niv)) {
            return BCM_E_UNAVAIL;
        }
        if (_BCM_MULTICAST_IS_EGRESS_OBJECT(group) &&
            !soc_feature(unit, soc_feature_mpls)) {
            return BCM_E_UNAVAIL;
        }
        if (_BCM_MULTICAST_IS_L2GRE(group) &&
            !soc_feature(unit, soc_feature_l2gre)) {
            return BCM_E_UNAVAIL;
        }
        if (_BCM_MULTICAST_IS_VXLAN(group) &&
            !soc_feature(unit, soc_feature_vxlan)) {
            return BCM_E_UNAVAIL;
        }
        if (_BCM_MULTICAST_IS_EXTENDER(group) &&
            !soc_feature(unit, soc_feature_port_extension)) {
            return BCM_E_UNAVAIL;
        }

        rv = _bcm_tr_multicast_ipmc_group_type_get(unit, mc_index,
                                                   &get_group);

        if (BCM_E_NOT_FOUND == rv) {
            *flags = 0;
        } else if (BCM_FAILURE(rv)) {
            return rv;
        } else if (get_group != group) {
            /* IPMC index is allocated, but not to the same group type */
            *flags = 0;
            return BCM_E_NOT_FOUND;
        } else {
            *flags =
                _bcm_esw_multicast_group_type_to_flags(
                     _BCM_MULTICAST_TYPE_GET(group)) |
                BCM_MULTICAST_WITH_ID;
        }
    }

    return rv;
}

#else  /* INCLUDE_L3 && BCM_TRX_SUPPORT */
int bcm_esw_triumph_multicast_not_empty;
#endif /* INCLUDE_L3 && BCM_TRX_SUPPORT */
