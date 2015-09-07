/*
 * $Id: vlan.c 1.18.2.2 Broadcom SDK $
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
 * File:    vlan.c
 * Purpose: Manages VLAN virtual port creation and deletion.
 *          Also manages addition and removal of virtual
 *          ports from VLAN. The types of virtual ports that
 *          can be added to or removed from VLAN can be VLAN
 *          VPs, NIV VPs, or Extender VPs.
 */

#include <soc/defs.h>
#include <sal/core/libc.h>

#if defined(BCM_TRIUMPH2_SUPPORT) && defined(INCLUDE_L3) 

#include <soc/mem.h>
#include <soc/hash.h>

#include <bcm/error.h>

#include <bcm_int/esw_dispatch.h>
#include <bcm_int/api_xlate_port.h>

#include <bcm_int/common/multicast.h>
#include <bcm_int/esw/multicast.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/vlan.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm_int/esw/trident.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/trunk.h>
#include <bcm_int/esw/ipmc.h>
#if defined(BCM_TRIUMPH3_SUPPORT)
#include <bcm_int/esw/triumph3.h>
#endif /* BCM_TRIUMPH3_SUPPORT*/

#if defined(BCM_TRIDENT2_SUPPORT)
#include <bcm_int/esw/trident2.h>
#endif /* BCM_TRIDENT2_SUPPORT*/

/* -------------------------------------------------------
 * Software book keeping for VLAN virtual port information
 * -------------------------------------------------------
 */

typedef struct _bcm_tr2_vlan_vp_info_s {
    bcm_vlan_port_match_t criteria;
    uint32 flags;
    bcm_vlan_t match_vlan;
    bcm_vlan_t match_inner_vlan;
    bcm_gport_t port;
} _bcm_tr2_vlan_vp_info_t;

typedef struct _bcm_tr2_vlan_virtual_bookkeeping_s {
    int vlan_virtual_initialized; /* Flag to check initialized status */
    sal_mutex_t vlan_virtual_mutex; /* VLAN virtual module lock */
    _bcm_tr2_vlan_vp_info_t *port_info; /* VP state */
} _bcm_tr2_vlan_virtual_bookkeeping_t;

STATIC _bcm_tr2_vlan_virtual_bookkeeping_t _bcm_tr2_vlan_virtual_bk_info[BCM_MAX_NUM_UNITS];

#define VLAN_VIRTUAL_INFO(unit) (&_bcm_tr2_vlan_virtual_bk_info[unit])

#define VLAN_VIRTUAL_INIT(unit)                                   \
    do {                                                          \
        if ((unit < 0) || (unit >= BCM_MAX_NUM_UNITS)) {          \
            return BCM_E_UNIT;                                    \
        }                                                         \
        if (!VLAN_VIRTUAL_INFO(unit)->vlan_virtual_initialized) { \
            return BCM_E_INIT;                                    \
        }                                                         \
    } while (0)

#define VLAN_VIRTUAL_LOCK(unit) \
        sal_mutex_take(VLAN_VIRTUAL_INFO(unit)->vlan_virtual_mutex, sal_mutex_FOREVER);

#define VLAN_VIRTUAL_UNLOCK(unit) \
        sal_mutex_give(VLAN_VIRTUAL_INFO(unit)->vlan_virtual_mutex);

#define VLAN_VP_INFO(unit, vp) (&VLAN_VIRTUAL_INFO(unit)->port_info[vp])

/* ---------------------------------------------------------------------------- 
 *
 * Routines for creation and deletion of VLAN virtual ports
 *
 * ---------------------------------------------------------------------------- 
 */

/*
 * Function:
 *      _bcm_tr2_vlan_vp_port_cnt_update
 * Purpose:
 *      Update port's VP count.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      gport - (IN) GPORT ID.
 *      vp    - (IN) Virtual port number.
 *      incr  - (IN) If TRUE, increment VP count, else decrease VP count.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr2_vlan_vp_port_cnt_update(int unit, bcm_gport_t gport,
        int vp, int incr)
{
    int mod_out, port_out, tgid_out, id_out;
    int idx;
    int mod_local;
    _bcm_port_info_t *port_info;
    bcm_port_t local_member_array[SOC_MAX_NUM_PORTS];
    int local_member_count;

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, gport, &mod_out, 
                                &port_out, &tgid_out, &id_out));
    if (-1 != id_out) {
        return BCM_E_PARAM;
    }

    /* Update the physical port's SW state. If associated with a trunk,
     * update each local physical port's SW state.
     */

    if (BCM_TRUNK_INVALID != tgid_out) {

        BCM_IF_ERROR_RETURN(_bcm_esw_trunk_local_members_get(unit, tgid_out,
                    SOC_MAX_NUM_PORTS, local_member_array, &local_member_count));

        for (idx = 0; idx < local_member_count; idx++) {
            _bcm_port_info_access(unit, local_member_array[idx], &port_info);
            if (incr) {
                port_info->vp_count++;
            } else {
                port_info->vp_count--;
            }
        }
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_modid_is_local(unit, mod_out, &mod_local));
        if (mod_local) {
            if (soc_feature(unit, soc_feature_sysport_remap)) { 
                BCM_XLATE_SYSPORT_S2P(unit, &port_out); 
            }
            _bcm_port_info_access(unit, port_out, &port_info);
            if (incr) {
                port_info->vp_count++;
            } else {
                port_info->vp_count--;
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr2_vlan_virtual_free_resources
 * Purpose:
 *      Free all allocated tables and memory
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      Nothing
 */
STATIC void
_bcm_tr2_vlan_virtual_free_resources(int unit)
{
    if (VLAN_VIRTUAL_INFO(unit)->port_info) {
        sal_free(VLAN_VIRTUAL_INFO(unit)->port_info);
        VLAN_VIRTUAL_INFO(unit)->port_info = NULL;
    }
    if (VLAN_VIRTUAL_INFO(unit)->vlan_virtual_mutex) {
        sal_mutex_destroy(VLAN_VIRTUAL_INFO(unit)->vlan_virtual_mutex);
        VLAN_VIRTUAL_INFO(unit)->vlan_virtual_mutex = NULL;
    } 
}

/*
 * Function:
 *      bcm_tr2_vlan_virtual_init
 * Purpose:
 *      Initialize the VLAN virtual port module.
 * Parameters:
 *      unit - SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_vlan_virtual_init(int unit)
{
    int num_vp;
    int rv = BCM_E_NONE;

    if (VLAN_VIRTUAL_INFO(unit)->vlan_virtual_initialized) {
        bcm_tr2_vlan_virtual_detach(unit);
    }

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    if (NULL == VLAN_VIRTUAL_INFO(unit)->port_info) {
        VLAN_VIRTUAL_INFO(unit)->port_info =
            sal_alloc(sizeof(_bcm_tr2_vlan_vp_info_t) * num_vp, "vlan_vp_info");
        if (NULL == VLAN_VIRTUAL_INFO(unit)->port_info) {
            _bcm_tr2_vlan_virtual_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(VLAN_VIRTUAL_INFO(unit)->port_info, 0,
            sizeof(_bcm_tr2_vlan_vp_info_t) * num_vp);

    if (NULL == VLAN_VIRTUAL_INFO(unit)->vlan_virtual_mutex) {
        VLAN_VIRTUAL_INFO(unit)->vlan_virtual_mutex =
            sal_mutex_create("vlan virtual mutex");
        if (NULL == VLAN_VIRTUAL_INFO(unit)->vlan_virtual_mutex) {
            _bcm_tr2_vlan_virtual_free_resources(unit);
            return BCM_E_MEMORY;
        }
    }

    VLAN_VIRTUAL_INFO(unit)->vlan_virtual_initialized = 1;

    /* Warm boot recovery of VLAN_VP_INFO depends on the completion
     * of _bcm_virtual_init, but _bcm_virtual_init is after the 
     * VLAN module in the init sequence. Hence, warm boot recovery
     * of VLAN_VP_INFO is moved to end of _bcm_virtual_init.
     */

    return rv;
}

/*
 * Function:
 *      bcm_tr2_vlan_virtual_detach
 * Purpose:
 *      Detach the VLAN virtual port module.
 * Parameters:
 *      unit - SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_vlan_virtual_detach(int unit)
{
    _bcm_tr2_vlan_virtual_free_resources(unit);

    VLAN_VIRTUAL_INFO(unit)->vlan_virtual_initialized = 0;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr2_vlan_vp_nh_info_set
 * Purpose:
 *      Get a next hop index and configure next hop tables.
 * Parameters:
 *      unit       - (IN) SOC unit number. 
 *      vlan_vp    - (IN) Pointer to VLAN virtual port structure. 
 *      vp         - (IN) Virtual port number. 
 *      drop       - (IN) Drop indication. 
 *      nh_index   - (IN/OUT) Next hop index. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr2_vlan_vp_nh_info_set(int unit, bcm_vlan_port_t *vlan_vp, int vp,
        int drop, int *nh_index)
{
    int rv;
    uint32 nh_flags;
    bcm_l3_egress_t nh_info;
    egr_l3_next_hop_entry_t egr_nh;
    uint8 egr_nh_entry_type;
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t trunk_id;
    int id;
    int ing_nh_port;
    int ing_nh_module;
    int ing_nh_trunk;
    ing_l3_next_hop_entry_t ing_nh;
    initial_ing_l3_next_hop_entry_t initial_ing_nh;

#ifdef BCM_TRIUMPH3_SUPPORT
    uint32 mtu_profile_index = 0;
#endif /* BCM_TRIUMPH3_SUPPORT */

    /* Get a next hop index */

    if (vlan_vp->flags & BCM_VLAN_PORT_REPLACE) {
        if ((*nh_index > soc_mem_index_max(unit, EGR_L3_NEXT_HOPm)) ||
                (*nh_index < soc_mem_index_min(unit, EGR_L3_NEXT_HOPm)))  {
            return BCM_E_PARAM;
        }
    } else {
        /*
         * Allocate a next-hop entry. By calling bcm_xgs3_nh_add()
         * with _BCM_L3_SHR_WRITE_DISABLE flag, a next-hop index is
         * allocated but nothing is written to hardware. The "nh_info"
         * in this case is not used, so just set to all zeros.
         */
        bcm_l3_egress_t_init(&nh_info);

        nh_flags = _BCM_L3_SHR_MATCH_DISABLE | _BCM_L3_SHR_WRITE_DISABLE;
        rv = bcm_xgs3_nh_add(unit, nh_flags, &nh_info, nh_index);
        BCM_IF_ERROR_RETURN(rv);
    }

    /* Write EGR_L3_NEXT_HOP entry */

    if (vlan_vp->flags & BCM_VLAN_PORT_REPLACE) {
        /* Read the existing egress next_hop entry */
        rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY, 
                *nh_index, &egr_nh);
        BCM_IF_ERROR_RETURN(rv);

        /* Be sure that the existing entry is programmed to SD-tag */
        egr_nh_entry_type = 
            soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh, ENTRY_TYPEf);
        if (egr_nh_entry_type != 0x2) { /* != SD-tag */
            return BCM_E_PARAM;
        }
    } else {
        egr_nh_entry_type = 0x2; /* SD-tag */
    }

    sal_memset(&egr_nh, 0, sizeof(egr_l3_next_hop_entry_t));
    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
            ENTRY_TYPEf, egr_nh_entry_type);
    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
            SD_TAG__DVPf, vp);
    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
            SD_TAG__HG_HDR_SELf, 1);
    rv = soc_mem_write(unit, EGR_L3_NEXT_HOPm,
            MEM_BLOCK_ALL, *nh_index, &egr_nh);
    if (rv < 0) {
        goto cleanup;
    }

    /* Resolve gport */

    rv = _bcm_esw_gport_resolve(unit, vlan_vp->port, &mod_out, 
            &port_out, &trunk_id, &id);
    if (rv < 0) {
        goto cleanup;
    }

    ing_nh_port = -1;
    ing_nh_module = -1;
    ing_nh_trunk = -1;

    if (BCM_GPORT_IS_TRUNK(vlan_vp->port)) {
        ing_nh_module = -1;
        ing_nh_port = -1;
        ing_nh_trunk = trunk_id;
    } else {
        ing_nh_module = mod_out;
        ing_nh_port = port_out;
        ing_nh_trunk = -1;
    }

    /* Write ING_L3_NEXT_HOP entry */

    sal_memset(&ing_nh, 0, sizeof(ing_l3_next_hop_entry_t));

    if (ing_nh_trunk == -1) {
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                &ing_nh, PORT_NUMf, ing_nh_port);
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                &ing_nh, MODULE_IDf, ing_nh_module);
    } else {    
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                &ing_nh, Tf, 1);
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                &ing_nh, TGIDf, ing_nh_trunk);
    }

    soc_mem_field32_set(unit, ING_L3_NEXT_HOPm, &ing_nh, DROPf, drop);
    soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
            &ing_nh, ENTRY_TYPEf, 0x2); /* L2 DVP */
    if (SOC_MEM_FIELD_VALID(unit, ING_L3_NEXT_HOPm, MTU_SIZEf)) {
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                &ing_nh, MTU_SIZEf, 0x3fff);
    }
#ifdef BCM_TRIUMPH3_SUPPORT
    else if (SOC_MEM_FIELD_VALID(unit, ING_L3_NEXT_HOPm, 
                 DVP_ATTRIBUTE_1_INDEXf)) {
        BCM_IF_ERROR_RETURN(
          _bcm_tr3_mtu_profile_index_get(unit, 0x3fff, &mtu_profile_index));
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm, &ing_nh, 
                      DVP_ATTRIBUTE_1_INDEXf, mtu_profile_index);
    }
#endif /*BCM_TRIUMPH3_SUPPORT */
    rv = soc_mem_write (unit, ING_L3_NEXT_HOPm,
            MEM_BLOCK_ALL, *nh_index, &ing_nh);
    if (rv < 0) {
        goto cleanup;
    }

    /* Write INITIAL_ING_L3_NEXT_HOP entry */

    sal_memset(&initial_ing_nh, 0, sizeof(initial_ing_l3_next_hop_entry_t));
    if (ing_nh_trunk == -1) {
        soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                &initial_ing_nh, PORT_NUMf, ing_nh_port);
        soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                &initial_ing_nh, MODULE_IDf, ing_nh_module);
    } else {
        soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                &initial_ing_nh, Tf, 1);
        soc_mem_field32_set(unit, INITIAL_ING_L3_NEXT_HOPm,
                &initial_ing_nh, TGIDf, ing_nh_trunk);
    }
    rv = soc_mem_write(unit, INITIAL_ING_L3_NEXT_HOPm,
            MEM_BLOCK_ALL, *nh_index, &initial_ing_nh);
    if (rv < 0) {
        goto cleanup;
    }

    return rv;

cleanup:
    if (!(vlan_vp->flags & BCM_VLAN_PORT_REPLACE)) {
        (void) bcm_xgs3_nh_del(unit, _BCM_L3_SHR_WRITE_DISABLE, *nh_index);
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr2_vlan_vp_nh_info_delete
 * Purpose:
 *      Free next hop index and clear next hop tables.
 * Parameters:
 *      unit     - (IN) SOC unit number. 
 *      nh_index - (IN) Next hop index. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr2_vlan_vp_nh_info_delete(int unit, int nh_index)
{
    egr_l3_next_hop_entry_t egr_nh;
    ing_l3_next_hop_entry_t ing_nh;
    initial_ing_l3_next_hop_entry_t initial_ing_nh;

    /* Clear EGR_L3_NEXT_HOP entry */
    sal_memset(&egr_nh, 0, sizeof(egr_l3_next_hop_entry_t));
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, EGR_L3_NEXT_HOPm,
                                   MEM_BLOCK_ALL, nh_index, &egr_nh));

    /* Clear ING_L3_NEXT_HOP entry */
    sal_memset(&ing_nh, 0, sizeof(ing_l3_next_hop_entry_t));
    BCM_IF_ERROR_RETURN(soc_mem_write (unit, ING_L3_NEXT_HOPm,
                                   MEM_BLOCK_ALL, nh_index, &ing_nh));

    /* Clear INITIAL_ING_L3_NEXT_HOP entry */
    sal_memset(&initial_ing_nh, 0, sizeof(initial_ing_l3_next_hop_entry_t));
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, INITIAL_ING_L3_NEXT_HOPm,
                                   MEM_BLOCK_ALL, nh_index, &initial_ing_nh));

    /* Free the next-hop index. */
    BCM_IF_ERROR_RETURN
        (bcm_xgs3_nh_del(unit, _BCM_L3_SHR_WRITE_DISABLE, nh_index));

    return BCM_E_NONE;
}

#if defined(BCM_TRIUMPH3_SUPPORT) 
/*
 * Function:
 *      _tr3_vlan_vp_match_add
 * Purpose:
 *      Add match criteria for VLAN VP.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan_vp - (IN) Pointer to VLAN virtual port structure. 
 *      vp - (IN) Virtual port number.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_tr3_vlan_vp_match_add(int unit, bcm_vlan_port_t *vlan_vp, int vp)
{
    vlan_xlate_entry_t vent;
    vlan_xlate_extd_entry_t vxent, old_vxent;
    int key_type = 0;
    bcm_vlan_action_set_t action;
    uint32 profile_idx;
    int rv;

    if (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_NONE) {
       return BCM_E_NONE;
    }

    if (!((vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN) ||
          (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN_STACKED) ||
          (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN16))) {
        return BCM_E_PARAM;
    }

    if ((vlan_vp->egress_vlan > BCM_VLAN_MAX) ||
        (vlan_vp->egress_inner_vlan > BCM_VLAN_MAX)) {
        return BCM_E_PARAM;
    } 

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));
    sal_memset(&vxent, 0, sizeof(vlan_xlate_extd_entry_t));

    if (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN) {
        key_type = bcmVlanTranslateKeyPortOuter;
    } else if (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN_STACKED) {
        key_type = bcmVlanTranslateKeyPortDouble;
    } else if (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN16) {
        key_type = bcmVlanTranslateKeyPortOuterTag;
    }
    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_translate_entry_assemble(unit, &vent,
                                                vlan_vp->port,
                                                key_type,
                                                vlan_vp->match_inner_vlan,
                                                vlan_vp->match_vlan));

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_vxlate2vxlate_extd(unit,&vent,&vxent));

    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, VALID_0f, 1);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, VALID_1f, 1);

    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, SOURCE_VPf, vp);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, NEW_OVIDf,
                                vlan_vp->egress_vlan);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, NEW_IVIDf,
                                vlan_vp->egress_inner_vlan);

    bcm_vlan_action_set_t_init(&action);

    if (vlan_vp->flags & BCM_VLAN_PORT_INNER_VLAN_PRESERVE) {
        action.dt_outer = bcmVlanActionReplace;
        action.dt_outer_prio = bcmVlanActionReplace;
        action.dt_inner = bcmVlanActionNone;
        action.dt_inner_prio = bcmVlanActionNone;
    } else {
        if (soc_feature(unit, soc_feature_vlan_copy_action)) {
            action.dt_outer = bcmVlanActionCopy;
            action.dt_outer_prio = bcmVlanActionCopy;
        } else {
            action.dt_outer = bcmVlanActionReplace;
            action.dt_outer_prio = bcmVlanActionReplace;
        }
        action.dt_inner = bcmVlanActionDelete;
        action.dt_inner_prio = bcmVlanActionDelete;
    }

    action.ot_outer = bcmVlanActionReplace;
    action.ot_outer_prio = bcmVlanActionReplace;
    if (vlan_vp->flags & BCM_VLAN_PORT_INNER_VLAN_ADD) {
        action.ot_inner = bcmVlanActionAdd;
    } else {
        action.ot_inner = bcmVlanActionNone;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_action_profile_entry_add(unit, &action, &profile_idx));

    soc_VLAN_XLATE_EXTDm_field32_set(unit, &vxent, TAG_ACTION_PROFILE_PTRf,
                                profile_idx);

    rv = soc_mem_insert_return_old(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL,
                                   &vxent, &old_vxent);
    if (rv == SOC_E_EXISTS) {
        /* Delete the old vlan translate profile entry */
        profile_idx = soc_VLAN_XLATE_EXTDm_field32_get(unit, &old_vxent,
                                                  TAG_ACTION_PROFILE_PTRf);       
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
    }
    return rv;
}

/*
 * Function:
 *      _tr3_vlan_vp_match_delete
 * Purpose:
 *      Delete match criteria for VLAN VP.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vp - (IN) Virtual port number.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_tr3_vlan_vp_match_delete(int unit, int vp)
{
    vlan_xlate_entry_t vent;
    vlan_xlate_extd_entry_t vxent, old_vxent;
    int key_type = 0;
    uint32 profile_idx;
    int rv;

    if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_NONE) {
        return BCM_E_NONE;
    }

    if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN) {
        key_type = bcmVlanTranslateKeyPortOuter;
    } else if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN_STACKED) {
        key_type = bcmVlanTranslateKeyPortDouble;
    } else if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN16) {
        key_type = bcmVlanTranslateKeyPortOuterTag;
    } else {
        return BCM_E_INTERNAL;
    }

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));
    sal_memset(&vxent, 0, sizeof(vlan_xlate_extd_entry_t));
    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_translate_entry_assemble(unit, &vent,
                                                VLAN_VP_INFO(unit, vp)->port,
                                                key_type,
                                                VLAN_VP_INFO(unit, vp)->match_inner_vlan,
                                                VLAN_VP_INFO(unit, vp)->match_vlan));
    BCM_IF_ERROR_RETURN
        (_bcm_tr3_vxlate2vxlate_extd(unit,&vent,&vxent));

    rv = soc_mem_delete_return_old(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL, 
                                   &vxent, &old_vxent);
    if ((rv == SOC_E_NONE) && 
        soc_VLAN_XLATE_EXTDm_field32_get(unit, &old_vxent, VALID_0f)) {
        profile_idx = soc_VLAN_XLATE_EXTDm_field32_get(unit, &old_vxent,
                                                  TAG_ACTION_PROFILE_PTRf);       
        /* Delete the old vlan action profile entry */
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
    }
    return rv;
}

/*
 * Function:
 *      _tr3_vlan_vp_match_get
 * Purpose:
 *      Get match criteria for VLAN VP.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vp - (IN) Virtual port number.
 *      vlan_vp - (OUT) Pointer to VLAN virtual port structure. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_tr3_vlan_vp_match_get(int unit, int vp, bcm_vlan_port_t *vlan_vp)
{
    vlan_xlate_entry_t vent;
    vlan_xlate_extd_entry_t vxent, vxent_out;
    int key_type = 0;
    int idx;

    vlan_vp->criteria = VLAN_VP_INFO(unit, vp)->criteria;
    vlan_vp->match_vlan = VLAN_VP_INFO(unit, vp)->match_vlan;
    vlan_vp->match_inner_vlan = VLAN_VP_INFO(unit, vp)->match_inner_vlan;
    vlan_vp->port = VLAN_VP_INFO(unit, vp)->port;

    if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_NONE) {
        return BCM_E_NONE;
    }

    if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN) {
        key_type = bcmVlanTranslateKeyPortOuter;
    } else if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN_STACKED) {
        key_type = bcmVlanTranslateKeyPortDouble;
    } else if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN16) {
        key_type = bcmVlanTranslateKeyPortOuterTag;
    } else {
        return BCM_E_INTERNAL;
    }

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));
    sal_memset(&vxent, 0, sizeof(vlan_xlate_extd_entry_t));

    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_translate_entry_assemble(unit, &vent,
                                                VLAN_VP_INFO(unit, vp)->port,
                                                key_type,
                                                VLAN_VP_INFO(unit, vp)->match_inner_vlan,
                                                VLAN_VP_INFO(unit, vp)->match_vlan));

    BCM_IF_ERROR_RETURN
        (_bcm_tr3_vxlate2vxlate_extd(unit,&vent,&vxent));

    BCM_IF_ERROR_RETURN
        (soc_mem_search(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL,
                        &idx, &vxent, &vxent_out, 0));
    vlan_vp->egress_vlan = soc_VLAN_XLATE_EXTDm_field32_get(unit, &vxent_out,
                                                       NEW_OVIDf);
    vlan_vp->egress_inner_vlan = soc_VLAN_XLATE_EXTDm_field32_get(unit, &vxent_out,
                                                       NEW_IVIDf);

    return BCM_E_NONE;
}
#endif   /* TRIUMPH3 support */

#ifdef BCM_TRIDENT2_SUPPORT

/*
 * Function:
 *      bcm_td2_vlan_vp_source_vp_lag_set
 * Purpose:
 *      Set source VP LAG for a VLAN virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) VLAN virtual port GPORT ID. 
 *      vp_lag_vp - (IN) VP representing VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_td2_vlan_vp_source_vp_lag_set(int unit, bcm_gport_t gport,
        int vp_lag_vp)
{
    int rv;

    VLAN_VIRTUAL_INIT(unit);

    VLAN_VIRTUAL_LOCK(unit);

    /* Temporarily set rv to UNAVAIL.
     * To be implemented when VLAN VP feature is ported to TD2.
     */
    rv = BCM_E_UNAVAIL;

    VLAN_VIRTUAL_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      bcm_td2_vlan_vp_source_vp_lag_clear
 * Purpose:
 *      Clear source VP LAG for a VLAN virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) VLAN virtual port GPORT ID. 
 *      vp_lag_vp - (IN) VP representing VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_td2_vlan_vp_source_vp_lag_clear(int unit, bcm_gport_t gport,
        int vp_lag_vp)
{
    int rv;

    VLAN_VIRTUAL_INIT(unit);

    VLAN_VIRTUAL_LOCK(unit);

    /* Temporarily set rv to UNAVAIL.
     * To be implemented when VLAN VP feature is ported to TD2.
     */
    rv = BCM_E_UNAVAIL;

    VLAN_VIRTUAL_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      bcm_td2_vlan_vp_source_vp_lag_get
 * Purpose:
 *      Get source VP LAG for a VLAN virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) VLAN virtual port GPORT ID. 
 *      vp_lag_vp - (OUT) VP representing VP LAG. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_td2_vlan_vp_source_vp_lag_get(int unit, bcm_gport_t gport,
        int *vp_lag_vp)
{
    int rv;

    VLAN_VIRTUAL_INIT(unit);

    VLAN_VIRTUAL_LOCK(unit);

    /* Temporarily set rv to UNAVAIL.
     * To be implemented when VLAN VP feature is ported to TD2.
     */
    rv = BCM_E_UNAVAIL;

    VLAN_VIRTUAL_UNLOCK(unit);

    return rv;
}

#endif /* BCM_TRIDENT2_SUPPORT */

/*
 * Function:
 *      _tr2_vlan_vp_match_add
 * Purpose:
 *      Add match criteria for VLAN VP.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan_vp - (IN) Pointer to VLAN virtual port structure. 
 *      vp - (IN) Virtual port number.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_tr2_vlan_vp_match_add(int unit, bcm_vlan_port_t *vlan_vp, int vp)
{
    vlan_xlate_entry_t vent, old_vent;
    int key_type = 0;
    bcm_vlan_action_set_t action;
    uint32 profile_idx;
    int rv;

    if (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_NONE) {
       return BCM_E_NONE;
    }

    if (!((vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN) ||
          (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN_STACKED) ||
          (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN16))) {
        return BCM_E_PARAM;
    }

    if ((vlan_vp->egress_vlan > BCM_VLAN_MAX) ||
        (vlan_vp->egress_inner_vlan > BCM_VLAN_MAX)) {
        return BCM_E_PARAM;
    } 

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));

    soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);

    if (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN) {
        key_type = bcmVlanTranslateKeyPortOuter;
    } else if (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN_STACKED) {
        key_type = bcmVlanTranslateKeyPortDouble;
    } else if (vlan_vp->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN16) {
        key_type = bcmVlanTranslateKeyPortOuterTag;
    }
    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_translate_entry_assemble(unit, &vent,
                                                vlan_vp->port,
                                                key_type,
                                                vlan_vp->match_inner_vlan,
                                                vlan_vp->match_vlan));

    soc_VLAN_XLATEm_field32_set(unit, &vent, MPLS_ACTIONf, 0x1); /* SVP */
    soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_VPf, vp);
    soc_VLAN_XLATEm_field32_set(unit, &vent, NEW_OVIDf,
                                vlan_vp->egress_vlan);
    soc_VLAN_XLATEm_field32_set(unit, &vent, NEW_IVIDf,
                                vlan_vp->egress_inner_vlan);

    bcm_vlan_action_set_t_init(&action);

    if (vlan_vp->flags & BCM_VLAN_PORT_INNER_VLAN_PRESERVE) {
        action.dt_outer = bcmVlanActionReplace;
        action.dt_outer_prio = bcmVlanActionReplace;
        action.dt_inner = bcmVlanActionNone;
        action.dt_inner_prio = bcmVlanActionNone;
    } else {
        if (soc_feature(unit, soc_feature_vlan_copy_action)) {
            action.dt_outer = bcmVlanActionCopy;
            action.dt_outer_prio = bcmVlanActionCopy;
        } else {
            action.dt_outer = bcmVlanActionReplace;
            action.dt_outer_prio = bcmVlanActionReplace;
        }
        action.dt_inner = bcmVlanActionDelete;
        action.dt_inner_prio = bcmVlanActionDelete;
    }

    action.ot_outer = bcmVlanActionReplace;
    action.ot_outer_prio = bcmVlanActionReplace;
    if (vlan_vp->flags & BCM_VLAN_PORT_INNER_VLAN_ADD) {
        action.ot_inner = bcmVlanActionAdd;
    } else {
        action.ot_inner = bcmVlanActionNone;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_action_profile_entry_add(unit, &action, &profile_idx));

    soc_VLAN_XLATEm_field32_set(unit, &vent, TAG_ACTION_PROFILE_PTRf,
                                profile_idx);
    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, VLAN_ACTION_VALIDf)) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, VLAN_ACTION_VALIDf, 1);
    }

    rv = soc_mem_insert_return_old(unit, VLAN_XLATEm, MEM_BLOCK_ALL,
                                   &vent, &old_vent);
    if (rv == SOC_E_EXISTS) {
        /* Delete the old vlan translate profile entry */
        profile_idx = soc_VLAN_XLATEm_field32_get(unit, &old_vent,
                                                  TAG_ACTION_PROFILE_PTRf);       
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
    }
    return rv;
}

/*
 * Function:
 *      _tr2_vlan_vp_match_delete
 * Purpose:
 *      Delete match criteria for VLAN VP.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vp - (IN) Virtual port number.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_tr2_vlan_vp_match_delete(int unit, int vp)
{
    vlan_xlate_entry_t vent, old_vent;
    int key_type = 0;
    uint32 profile_idx;
    int rv;

    if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_NONE) {
        return BCM_E_NONE;
    }

    if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN) {
        key_type = bcmVlanTranslateKeyPortOuter;
    } else if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN_STACKED) {
        key_type = bcmVlanTranslateKeyPortDouble;
    } else if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN16) {
        key_type = bcmVlanTranslateKeyPortOuterTag;
    } else {
        return BCM_E_INTERNAL;
    }

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));
    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_translate_entry_assemble(unit, &vent,
                                                VLAN_VP_INFO(unit, vp)->port,
                                                key_type,
                                                VLAN_VP_INFO(unit, vp)->match_inner_vlan,
                                                VLAN_VP_INFO(unit, vp)->match_vlan));

    rv = soc_mem_delete_return_old(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vent, &old_vent);
    if ((rv == SOC_E_NONE) && soc_VLAN_XLATEm_field32_get(unit, &old_vent, VALIDf)) {
        profile_idx = soc_VLAN_XLATEm_field32_get(unit, &old_vent,
                                                  TAG_ACTION_PROFILE_PTRf);       
        /* Delete the old vlan action profile entry */
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
    }
    return rv;
}

/*
 * Function:
 *      _tr2_vlan_vp_match_get
 * Purpose:
 *      Get match criteria for VLAN VP.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vp - (IN) Virtual port number.
 *      vlan_vp - (OUT) Pointer to VLAN virtual port structure. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_tr2_vlan_vp_match_get(int unit, int vp, bcm_vlan_port_t *vlan_vp)
{
    vlan_xlate_entry_t vent, vent_out;
    int key_type = 0;
    int idx;

    vlan_vp->criteria = VLAN_VP_INFO(unit, vp)->criteria;
    vlan_vp->match_vlan = VLAN_VP_INFO(unit, vp)->match_vlan;
    vlan_vp->match_inner_vlan = VLAN_VP_INFO(unit, vp)->match_inner_vlan;
    vlan_vp->port = VLAN_VP_INFO(unit, vp)->port;

    if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_NONE) {
        return BCM_E_NONE;
    }

    if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN) {
        key_type = bcmVlanTranslateKeyPortOuter;
    } else if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN_STACKED) {
        key_type = bcmVlanTranslateKeyPortDouble;
    } else if (VLAN_VP_INFO(unit, vp)->criteria == BCM_VLAN_PORT_MATCH_PORT_VLAN16) {
        key_type = bcmVlanTranslateKeyPortOuterTag;
    } else {
        return BCM_E_INTERNAL;
    }

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));
    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_translate_entry_assemble(unit, &vent,
                                                VLAN_VP_INFO(unit, vp)->port,
                                                key_type,
                                                VLAN_VP_INFO(unit, vp)->match_inner_vlan,
                                                VLAN_VP_INFO(unit, vp)->match_vlan));

    BCM_IF_ERROR_RETURN
        (soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ALL,
                        &idx, &vent, &vent_out, 0));
    vlan_vp->egress_vlan = soc_VLAN_XLATEm_field32_get(unit, &vent_out,
                                                       NEW_OVIDf);
    vlan_vp->egress_inner_vlan = soc_VLAN_XLATEm_field32_get(unit, &vent_out,
                                                       NEW_IVIDf);

    return BCM_E_NONE;
}

STATIC int
_bcm_tr2_vlan_vp_match_add(int unit, bcm_vlan_port_t *vlan_vp, int vp)
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _tr3_vlan_vp_match_add(unit, vlan_vp, vp);
    } else
#endif
    {
        return _tr2_vlan_vp_match_add(unit, vlan_vp, vp);
    }
}

STATIC int
_bcm_tr2_vlan_vp_match_delete(int unit, int vp)
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _tr3_vlan_vp_match_delete(unit, vp);
    } else
#endif
    {
        return _tr2_vlan_vp_match_delete(unit, vp);
    }
}

STATIC int
_bcm_tr2_vlan_vp_match_get(int unit, int vp, bcm_vlan_port_t *vlan_vp)
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        return _tr3_vlan_vp_match_get(unit, vp, vlan_vp);
    } else
#endif
    {
        return _tr2_vlan_vp_match_get(unit, vp, vlan_vp);
    }
}

/*
 * Function:
 *      _bcm_tr2_vlan_vp_create
 * Purpose:
 *      Create a VLAN virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan_vp - (IN/OUT) Pointer to VLAN virtual port structure. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr2_vlan_vp_create(int unit,
                         bcm_vlan_port_t *vlan_vp)
{
    int mode;
    int vp;
    int rv = BCM_E_NONE;
    int num_vp;
    int nh_index = 0;
    ing_dvp_table_entry_t dvp_entry;
    source_vp_entry_t svp_entry;
    int cml_default_enable=0, cml_default_new=0, cml_default_move=0;

    BCM_IF_ERROR_RETURN(bcm_xgs3_l3_egress_mode_get(unit, &mode));
    if (!mode) {
        soc_cm_debug(DK_L3, "L3 egress mode must be set first\n");
        return BCM_E_DISABLED;
    }

    if (!(vlan_vp->flags & BCM_VLAN_PORT_REPLACE)) { /* Create new VLAN VP */

        if (vlan_vp->flags & BCM_VLAN_PORT_WITH_ID) {
            if (!BCM_GPORT_IS_VLAN_PORT(vlan_vp->vlan_port_id)) {
                return BCM_E_PARAM;
            }
            vp = BCM_GPORT_VLAN_PORT_ID_GET(vlan_vp->vlan_port_id);

            if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
                return BCM_E_EXISTS;
            } else {
                rv = _bcm_vp_used_set(unit, vp, _bcmVpTypeVlan);
                if (rv < 0) {
                    return rv;
                }
            }
        } else {
            /* allocate a new VP index */
            num_vp = soc_mem_index_count(unit, SOURCE_VPm);
            rv = _bcm_vp_alloc(unit, 0, (num_vp - 1), 1, SOURCE_VPm,
                    _bcmVpTypeVlan, &vp);
            if (rv < 0) {
                return rv;
            }
        }

        /* Configure next hop tables */
        rv = _bcm_tr2_vlan_vp_nh_info_set(unit, vlan_vp, vp, 0,
                                              &nh_index);
        if (rv < 0) {
            goto cleanup;
        }

        /* Configure DVP table */
        sal_memset(&dvp_entry, 0, sizeof(ing_dvp_table_entry_t));
        soc_ING_DVP_TABLEm_field32_set(unit, &dvp_entry, NEXT_HOP_INDEXf,
                                       nh_index);
        rv = WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp, &dvp_entry);
        if (rv < 0) {
            goto cleanup;
        }

        /* Configure SVP table */
        sal_memset(&svp_entry, 0, sizeof(source_vp_entry_t));
        soc_SOURCE_VPm_field32_set(unit, &svp_entry, ENTRY_TYPEf, 3);


        /* Set the CML */
        rv = _bcm_vp_default_cml_mode_get (unit, 
                           &cml_default_enable, &cml_default_new, 
                           &cml_default_move);
         if (rv < 0) {
             goto cleanup;
         }

        if (cml_default_enable) {
            /* Set the CML to default values */
            soc_SOURCE_VPm_field32_set(unit, &svp_entry, CML_FLAGS_NEWf, cml_default_new);
            soc_SOURCE_VPm_field32_set(unit, &svp_entry, CML_FLAGS_MOVEf, cml_default_move);
        } else {
            /* Set the CML to PVP_CML_SWITCH by default (hw learn and forward) */
            soc_SOURCE_VPm_field32_set(unit, &svp_entry, CML_FLAGS_NEWf, 0x8);
            soc_SOURCE_VPm_field32_set(unit, &svp_entry, CML_FLAGS_MOVEf, 0x8);
        }


        rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp_entry);
        if (rv < 0) {
            goto cleanup;
        }

        /* Configure ingress VLAN translation table */
        rv = _bcm_tr2_vlan_vp_match_add(unit, vlan_vp, vp);
        if (rv < 0) {
            goto cleanup;
        }

        /* Increment port's VP count */
        rv = _bcm_tr2_vlan_vp_port_cnt_update(unit, vlan_vp->port, vp, TRUE);
        if (rv < 0) {
            goto cleanup;
        }

    } else { /* Replace properties of existing VLAN VP */

        if (!(vlan_vp->flags & BCM_VLAN_PORT_WITH_ID)) {
            return BCM_E_PARAM;
        }
        if (!BCM_GPORT_IS_VLAN_PORT(vlan_vp->vlan_port_id)) {
            return BCM_E_PARAM;
        }
        vp = BCM_GPORT_VLAN_PORT_ID_GET(vlan_vp->vlan_port_id);
        if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
            return BCM_E_PARAM;
        }

        /* For existing vlan vp, NH entry already exists */
        BCM_IF_ERROR_RETURN
            (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp_entry));

        nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp_entry,
                NEXT_HOP_INDEXf);

        /* Update existing next hop entries */
        BCM_IF_ERROR_RETURN
            (_bcm_tr2_vlan_vp_nh_info_set(unit, vlan_vp, vp, 0,
                                              &nh_index));

        /* Delete old ingress VLAN translation entry,
         * install new ingress VLAN translation entry
         */
        BCM_IF_ERROR_RETURN
            (_bcm_tr2_vlan_vp_match_delete(unit, vp));

        BCM_IF_ERROR_RETURN
            (_bcm_tr2_vlan_vp_match_add(unit, vlan_vp, vp));

        /* Decrement old port's VP count, increment new port's VP count */
        BCM_IF_ERROR_RETURN
            (_bcm_tr2_vlan_vp_port_cnt_update(unit,
                VLAN_VP_INFO(unit, vp)->port, vp, FALSE));

        BCM_IF_ERROR_RETURN
            (_bcm_tr2_vlan_vp_port_cnt_update(unit,
                vlan_vp->port, vp, TRUE));
    }

    /* Set VLAN VP software state */
    VLAN_VP_INFO(unit, vp)->criteria = vlan_vp->criteria;
    VLAN_VP_INFO(unit, vp)->flags = vlan_vp->flags;
    VLAN_VP_INFO(unit, vp)->match_vlan = vlan_vp->match_vlan;
    VLAN_VP_INFO(unit, vp)->match_inner_vlan = vlan_vp->match_inner_vlan;
    VLAN_VP_INFO(unit, vp)->port = vlan_vp->port;

    BCM_GPORT_VLAN_PORT_ID_SET(vlan_vp->vlan_port_id, vp);
    vlan_vp->encap_id = nh_index;

    return rv;

cleanup:
    if (!(vlan_vp->flags & BCM_VLAN_PORT_REPLACE)) {
        (void) _bcm_vp_free(unit, _bcmVpTypeVlan, 1, vp);
        _bcm_tr2_vlan_vp_nh_info_delete(unit, nh_index);

        sal_memset(&dvp_entry, 0, sizeof(ing_dvp_table_entry_t));
        (void)WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp, &dvp_entry);

        sal_memset(&svp_entry, 0, sizeof(source_vp_entry_t));
        (void)WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp_entry);

        (void) _bcm_tr2_vlan_vp_match_delete(unit, vp);
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr2_vlan_vp_create
 * Purpose:
 *      Create a VLAN virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan_vp - (IN/OUT) Pointer to VLAN virtual port structure. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_tr2_vlan_vp_create(int unit,
                         bcm_vlan_port_t *vlan_vp)
{
    int rv;

    VLAN_VIRTUAL_INIT(unit);

    VLAN_VIRTUAL_LOCK(unit);

    rv = _bcm_tr2_vlan_vp_create(unit, vlan_vp);

    VLAN_VIRTUAL_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      _bcm_tr2_vlan_vp_destroy
 * Purpose:
 *      Destroy a VLAN virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) VLAN VP GPORT ID.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr2_vlan_vp_destroy(int unit, bcm_gport_t gport)
{
    int vp;
    source_vp_entry_t svp_entry;
    int nh_index;
    ing_dvp_table_entry_t dvp_entry;

    if (!BCM_GPORT_IS_VLAN_PORT(gport)) {
        return BCM_E_PARAM;
    }

    vp = BCM_GPORT_VLAN_PORT_ID_GET(gport);
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
        return BCM_E_NOT_FOUND;
    }

#ifdef BCM_TRIDENT_SUPPORT
    if (soc_feature(unit, soc_feature_vp_group_ingress_vlan_membership) ||
        soc_feature(unit, soc_feature_vp_group_egress_vlan_membership)) { 
        /* Disable ingress and egress filter modes */
        BCM_IF_ERROR_RETURN
            (bcm_td_vp_vlan_member_set(unit, gport, 0));
    }
#endif /* BCM_TRIDENT_SUPPORT */

    /* Delete ingress VLAN translation entry */
    BCM_IF_ERROR_RETURN
        (_bcm_tr2_vlan_vp_match_delete(unit, vp));

    /* Clear SVP entry */
    sal_memset(&svp_entry, 0, sizeof(source_vp_entry_t));
    BCM_IF_ERROR_RETURN
        (WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp_entry));

    /* Clear DVP entry */
    BCM_IF_ERROR_RETURN
        (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp_entry));
    nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp_entry, NEXT_HOP_INDEXf);

    sal_memset(&dvp_entry, 0, sizeof(ing_dvp_table_entry_t));
    BCM_IF_ERROR_RETURN
        (WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp, &dvp_entry));

    /* Clear next hop entries and free next hop index */
    BCM_IF_ERROR_RETURN
        (_bcm_tr2_vlan_vp_nh_info_delete(unit, nh_index));

    /* Decrement port's VP count */
    BCM_IF_ERROR_RETURN
        (_bcm_tr2_vlan_vp_port_cnt_update(unit,
                                              VLAN_VP_INFO(unit, vp)->port,
                                              vp, FALSE));
    /* Free VP */
    BCM_IF_ERROR_RETURN
        (_bcm_vp_free(unit, _bcmVpTypeVlan, 1, vp));

    /* Clear VLAN VP software state */
    sal_memset(VLAN_VP_INFO(unit, vp), 0, sizeof(_bcm_tr2_vlan_vp_info_t));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2_vlan_vp_destroy
 * Purpose:
 *      Destroy a VLAN virtual port.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      gport - (IN) VLAN VP GPORT ID.
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_tr2_vlan_vp_destroy(int unit, bcm_gport_t gport)
{
    int rv;

    VLAN_VIRTUAL_INIT(unit);

    VLAN_VIRTUAL_LOCK(unit);

    rv = _bcm_tr2_vlan_vp_destroy(unit, gport);

    VLAN_VIRTUAL_UNLOCK(unit);

    return rv;
}

/*
 * Function:
 *      _bcm_tr2_vlan_vp_find
 * Purpose:
 *      Get VLAN virtual port info.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan_vp - (IN/OUT) Pointer to VLAN virtual port structure. 
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr2_vlan_vp_find(int unit, bcm_vlan_port_t *vlan_vp)
{
    int vp;
    ing_dvp_table_entry_t dvp_entry;
    int nh_index;

    if (!BCM_GPORT_IS_VLAN_PORT(vlan_vp->vlan_port_id)) {
        return BCM_E_PARAM;
    }

    vp = BCM_GPORT_VLAN_PORT_ID_GET(vlan_vp->vlan_port_id);
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
        return BCM_E_NOT_FOUND;
    }

    bcm_vlan_port_t_init(vlan_vp);

    BCM_IF_ERROR_RETURN(_bcm_tr2_vlan_vp_match_get(unit, vp, vlan_vp));

    vlan_vp->flags = VLAN_VP_INFO(unit, vp)->flags;

    BCM_IF_ERROR_RETURN
        (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp_entry));
    nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp_entry, NEXT_HOP_INDEXf);
    vlan_vp->encap_id = nh_index;

    BCM_GPORT_VLAN_PORT_ID_SET(vlan_vp->vlan_port_id, vp);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2_vlan_vp_find
 * Purpose:
 *      Get VLAN virtual port info.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan_vp - (IN/OUT) Pointer to VLAN virtual port structure. 
 * Returns:
 *      BCM_X_XXX
 */
int
bcm_tr2_vlan_vp_find(int unit, bcm_vlan_port_t *vlan_vp)
{
    VLAN_VIRTUAL_INIT(unit);
    return _bcm_tr2_vlan_vp_find(unit, vlan_vp);
}

/*
 * Function:
 *      _bcm_tr2_vlan_port_resolve
 * Purpose:
 *      Get the modid, port, trunk values for a VLAN virtual port
 * Parameters:
 *      unit - (IN) BCM device number
 *      gport - (IN) Global port identifier
 *      modid - (OUT) Module ID
 *      port - (OUT) Port number
 *      trunk_id - (OUT) Trunk ID
 *      id - (OUT) Virtual port ID
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr2_vlan_port_resolve(int unit, bcm_gport_t vlan_port_id,
                          bcm_module_t *modid, bcm_port_t *port,
                          bcm_trunk_t *trunk_id, int *id)

{
    int rv = BCM_E_NONE, nh_index, vp;
    ing_l3_next_hop_entry_t ing_nh;
    ing_dvp_table_entry_t dvp;

    VLAN_VIRTUAL_INIT(unit);

    if (!BCM_GPORT_IS_VLAN_PORT(vlan_port_id)) {
        return (BCM_E_BADID);
    }

    vp = BCM_GPORT_VLAN_PORT_ID_GET(vlan_port_id);
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
        return BCM_E_NOT_FOUND;
    }
    BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
    nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp,
                                              NEXT_HOP_INDEXf);
    BCM_IF_ERROR_RETURN (soc_mem_read(unit, ING_L3_NEXT_HOPm, MEM_BLOCK_ANY,
                                      nh_index, &ing_nh));

    if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, ENTRY_TYPEf) != 0x2) {
        /* Entry type is not L2 DVP */
        return BCM_E_NOT_FOUND;
    }
    if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, Tf)) {
        *trunk_id = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, TGIDf);
    } else {
        /* Only add this to replication set if destination is local */
        *modid = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, MODULE_IDf);
        *port = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, PORT_NUMf);
    }
    *id = vp;
    return rv;
}

/* ---------------------------------------------------------------------------- 
 *
 * Routines for adding/removing virtual ports to/from VLAN
 *
 * ---------------------------------------------------------------------------- 
 */

/*
 * Function:
 *      _bcm_tr2_phy_port_trunk_is_local
 * Purpose:
 *      Determine if the given physical port or trunk is local.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      gport - (IN) Physical port or trunk GPORT ID.
 *      is_local - (OUT) Indicates if gport is local.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr2_phy_port_trunk_is_local(int unit, bcm_gport_t gport, int *is_local)
{
    bcm_trunk_t trunk_id;
    int rv = BCM_E_NONE;
    int local_member_count;
    bcm_module_t mod_out;
    bcm_port_t port_out;
    int id;
    int modid_local;

    *is_local = 0;

    if (BCM_GPORT_IS_TRUNK(gport)) {
        trunk_id = BCM_GPORT_TRUNK_GET(gport);
        rv = _bcm_trunk_id_validate(unit, trunk_id);
        if (BCM_FAILURE(rv)) {
            return (BCM_E_PORT);
        }
        rv = _bcm_esw_trunk_local_members_get(unit, trunk_id,
                0, NULL, &local_member_count);
        if (BCM_FAILURE(rv)) {
            return (BCM_E_PORT);
        }   
        if (local_member_count > 0) {
            *is_local = 1;
        }
    } else {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_gport_resolve(unit, gport, &mod_out, &port_out,
                                    &trunk_id, &id)); 
        if ((trunk_id != -1) || (id != -1)) {
            return BCM_E_PARAM;
        }

        BCM_IF_ERROR_RETURN
            (_bcm_esw_modid_is_local(unit, mod_out, &modid_local));

        *is_local = modid_local;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr2_vlan_vp_untagged_add
 * Purpose:
 *      Set VLAN VP tagging/untagging status by adding
 *      a (VLAN VP, VLAN) egress VLAN translation entry.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN ID. 
 *      vp   - (IN) Virtual port number.
 *      flags - (IN) Untagging indication.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr2_vlan_vp_untagged_add(int unit, bcm_vlan_t vlan, int vp, int flags)
{
    egr_vlan_xlate_entry_t vent, old_vent;
    bcm_vlan_action_set_t action;
    uint32 profile_idx;
    int rv;

    sal_memset(&vent, 0, sizeof(egr_vlan_xlate_entry_t));

    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);

    if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, ENTRY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, ENTRY_TYPEf, 1);
    } else if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, KEY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 1);
    }
    
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, DVPf, vp);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, vlan);

    if (VLAN_VP_INFO(unit, vp)->flags & BCM_VLAN_PORT_EGRESS_VLAN16) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, NEW_OTAG_VPTAG_SELf, 1);
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, NEW_OTAG_VPTAGf,
                                        VLAN_VP_INFO(unit, vp)->match_vlan);
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, NEW_IVIDf, vlan);
    } else {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, NEW_OTAG_VPTAG_SELf, 0);
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, NEW_OVIDf,
                VLAN_VP_INFO(unit, vp)->match_vlan & 0xfff);
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, NEW_IVIDf, vlan);
    }

    bcm_vlan_action_set_t_init(&action);
    action.dt_outer = bcmVlanActionReplace;
    action.ot_outer = bcmVlanActionReplace;
    if (flags & BCM_VLAN_PORT_UNTAGGED) {
        action.dt_inner = bcmVlanActionNone;
        action.ot_inner = bcmVlanActionNone;
    } else {
        if (soc_feature(unit, soc_feature_vlan_copy_action)) {
            action.dt_inner = bcmVlanActionCopy;
            action.ot_inner = bcmVlanActionCopy;
        } else {

            action.dt_inner = bcmVlanActionReplace;
            action.ot_inner = bcmVlanActionAdd;
        }
    }
    BCM_IF_ERROR_RETURN
        (_bcm_trx_egr_vlan_action_profile_entry_add(unit, &action, &profile_idx));

    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, TAG_ACTION_PROFILE_PTRf,
                                profile_idx);

    rv = soc_mem_insert_return_old(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL,
                                   &vent, &old_vent);
    if (rv == SOC_E_EXISTS) {
        /* Delete the old vlan translate profile entry */
        profile_idx = soc_EGR_VLAN_XLATEm_field32_get(unit, &old_vent,
                                                  TAG_ACTION_PROFILE_PTRf);       
        rv = _bcm_trx_egr_vlan_action_profile_entry_delete(unit, profile_idx);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_tr2_vlan_vp_untagged_delete
 * Purpose:
 *      Delete (VLAN VP, VLAN) egress VLAN translation entry.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN ID. 
 *      vp   - (IN) Virtual port number.
 * Returns:
 *      BCM_X_XXX
 */
STATIC int
_bcm_tr2_vlan_vp_untagged_delete(int unit, bcm_vlan_t vlan, int vp)
{
    egr_vlan_xlate_entry_t vent, old_vent;
    uint32 profile_idx;
    int rv;

    sal_memset(&vent, 0, sizeof(egr_vlan_xlate_entry_t));

    if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, ENTRY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, ENTRY_TYPEf, 1);
    } else if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, KEY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 1);
    }
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, DVPf, vp);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, vlan);

    rv = soc_mem_delete_return_old(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL,
                                   &vent, &old_vent);
    if ((rv == SOC_E_NONE) &&
            soc_EGR_VLAN_XLATEm_field32_get(unit, &old_vent, VALIDf)) {
        /* Delete the old vlan translate profile entry */
        profile_idx = soc_EGR_VLAN_XLATEm_field32_get(unit, &old_vent,
                                                  TAG_ACTION_PROFILE_PTRf);       
        rv = _bcm_trx_egr_vlan_action_profile_entry_delete(unit, profile_idx);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_tr2_vlan_vp_untagged_get
 * Purpose:
 *      Get tagging/untagging status of a VLAN virtual port by
 *      reading the (VLAN VP, VLAN) egress vlan translation entry.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN to remove virtual port from.
 *      vp   - (IN) Virtual port number.
 *      flags - (OUT) Untagging status of the VLAN virtual port.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_vlan_vp_untagged_get(int unit, bcm_vlan_t vlan, int vp,
                                  int *flags)
{
    egr_vlan_xlate_entry_t vent, res_vent;
    int idx;
    uint32 profile_idx;
    int rv;
    bcm_vlan_action_set_t action;

    *flags = 0;

    sal_memset(&vent, 0, sizeof(egr_vlan_xlate_entry_t));
    if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, ENTRY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, ENTRY_TYPEf, 1);
    } else if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, KEY_TYPEf)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 1);
    }
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, DVPf, vp);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, vlan);

    rv = soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &idx,
                                   &vent, &res_vent, 0);
    if ((rv == SOC_E_NONE) &&
            soc_EGR_VLAN_XLATEm_field32_get(unit, &res_vent, VALIDf)) {
        profile_idx = soc_EGR_VLAN_XLATEm_field32_get(unit, &res_vent,
                                                  TAG_ACTION_PROFILE_PTRf);       
        _bcm_trx_egr_vlan_action_profile_entry_get(unit, &action, profile_idx);

        if (bcmVlanActionNone == action.ot_inner) {
            *flags = BCM_VLAN_PORT_UNTAGGED;
        }
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr2_update_vlan_pbmp
 * Purpose:
 *      Update VLAN table's port bitmap.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      vlan  - (IN) VLAN ID.
 *      pbmp  - (IN) VLAN port bitmap. 
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_update_vlan_pbmp(int unit, bcm_vlan_t vlan,
        bcm_pbmp_t *pbmp)
{
    int rv = BCM_E_NONE;
    vlan_tab_entry_t vtab;
    egr_vlan_entry_t egr_vtab;

    soc_mem_lock(unit, VLAN_TABm);
    
    rv = soc_mem_read(unit, VLAN_TABm, MEM_BLOCK_ANY, vlan, &vtab);
    if (rv < 0) {
        soc_mem_unlock(unit, VLAN_TABm);
        return rv;
    }

    soc_mem_pbmp_field_set(unit, VLAN_TABm, &vtab, PORT_BITMAPf, pbmp);
    if (soc_mem_field_valid(unit, VLAN_TABm, ING_PORT_BITMAPf)) {
        soc_mem_pbmp_field_set(unit, VLAN_TABm, &vtab, ING_PORT_BITMAPf, pbmp);
    }
    rv = soc_mem_write(unit, VLAN_TABm, MEM_BLOCK_ALL, vlan, &vtab);
    if (rv < 0) {
        soc_mem_unlock(unit, VLAN_TABm);
        return rv;
    }

    soc_mem_unlock(unit, VLAN_TABm);

    soc_mem_lock(unit, EGR_VLANm);

    rv = soc_mem_read(unit, EGR_VLANm, MEM_BLOCK_ANY, vlan, &egr_vtab); 
    if (rv < 0) {
        soc_mem_unlock(unit, EGR_VLANm);
        return rv;
    }

    soc_mem_pbmp_field_set(unit, EGR_VLANm, &egr_vtab, PORT_BITMAPf, pbmp);
    rv = soc_mem_write(unit, EGR_VLANm, MEM_BLOCK_ALL, vlan, &egr_vtab);
    if (rv < 0) {
        soc_mem_unlock(unit, EGR_VLANm);
        return rv;
    }

    soc_mem_unlock(unit, EGR_VLANm);

    return rv;
}

/*
 * Function:
 *      bcm_tr2_vlan_gport_add
 * Purpose:
 *      Add a VLAN/NIV/Extender virtual port to the specified vlan.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      vlan  - (IN) VLAN ID to add virtual port to as a member.
 *      gport - (IN) VP Gport ID
 *      flags - (IN) Indicates if packet should egress out of the given
 *                   VLAN virtual port untagged. Not applicable to NIV or
 *                   Extender VPs. 
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_vlan_gport_add(int unit, bcm_vlan_t vlan, bcm_gport_t gport,
                           int flags)
{
    int rv = BCM_E_NONE;
    int vp;
    _bcm_vp_type_e vp_type;
    bcm_vlan_port_t vlan_vp;
    bcm_gport_t phy_port_trunk;
    int is_local;
    vlan_tab_entry_t vtab;
    bcm_pbmp_t vlan_pbmp, vlan_ubmp, l2_pbmp, l3_pbmp, l2_l3_pbmp;
    int i, mc_idx;
    bcm_if_t encap_id;
    bcm_multicast_t group;
    int bc_idx, umc_idx, uuc_idx;
    int bc_do_not_add, umc_do_not_add, uuc_do_not_add;
    soc_field_t group_type[3] = {BC_IDXf, UMC_IDXf, UUC_IDXf};
    int egr_vt_added = FALSE;
    int ing_vp_vlan_membership_added = FALSE;
    int egr_vp_vlan_membership_added = FALSE;
    int ing_vp_group_added = FALSE;
    int eg_vp_group_added = FALSE;

    /* Check that unsupported flags are not set */
    if (flags & ~(BCM_VLAN_PORT_UNTAGGED |
                  BCM_VLAN_GPORT_ADD_VP_VLAN_MEMBERSHIP | 
                  BCM_VLAN_GPORT_ADD_INGRESS_ONLY |
                  BCM_VLAN_GPORT_ADD_EGRESS_ONLY  |
                  BCM_VLAN_GPORT_ADD_STP_MASK |
                  BCM_VLAN_GPORT_ADD_UNKNOWN_UCAST_DO_NOT_ADD |
                  BCM_VLAN_GPORT_ADD_UNKNOWN_MCAST_DO_NOT_ADD |
                  BCM_VLAN_GPORT_ADD_BCAST_DO_NOT_ADD)) {
        return BCM_E_PARAM;
    }

    /* check for specfic combination */
    if (flags & BCM_VLAN_GPORT_ADD_VP_VLAN_MEMBERSHIP) {
        if ((flags & (BCM_VLAN_GPORT_ADD_INGRESS_ONLY | 
                     BCM_VLAN_GPORT_ADD_EGRESS_ONLY)) ==
            (BCM_VLAN_GPORT_ADD_INGRESS_ONLY | 
             BCM_VLAN_GPORT_ADD_EGRESS_ONLY)) {
             return BCM_E_PARAM;
        }
    }

    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        vp = BCM_GPORT_VLAN_PORT_ID_GET(gport);
        vp_type = _bcmVpTypeVlan;

        /* Get the physical port or trunk the VP resides on */
        bcm_vlan_port_t_init(&vlan_vp);
        vlan_vp.vlan_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_tr2_vlan_vp_find(unit, &vlan_vp));
        phy_port_trunk = vlan_vp.port;
    } else
#ifdef BCM_TRIDENT_SUPPORT
    if (BCM_GPORT_IS_NIV_PORT(gport)) {
        bcm_niv_port_t niv_port;

        vp = BCM_GPORT_NIV_PORT_ID_GET(gport);
        vp_type = _bcmVpTypeNiv;

        /* Get the physical port or trunk the VP resides on */
        bcm_niv_port_t_init(&niv_port);
        niv_port.niv_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_trident_niv_port_get(unit, &niv_port));
        if (niv_port.flags & BCM_NIV_PORT_MATCH_NONE) {
            phy_port_trunk = BCM_GPORT_INVALID;
        } else {
            phy_port_trunk = niv_port.port;
        }
    } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        bcm_extender_port_t extender_port;

        vp = BCM_GPORT_EXTENDER_PORT_ID_GET(gport);
        vp_type = _bcmVpTypeExtender;

        /* Get the physical port or trunk the VP resides on */
        bcm_extender_port_t_init(&extender_port);
        extender_port.extender_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_tr3_extender_port_get(unit, &extender_port));
        phy_port_trunk = extender_port.port;
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        return BCM_E_PARAM;
    }

    if (!_bcm_vp_used_get(unit, vp, vp_type)) {
        return BCM_E_NOT_FOUND;
    }

    if (phy_port_trunk != BCM_GPORT_INVALID) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr2_phy_port_trunk_is_local(unit, phy_port_trunk, &is_local));
        if (!is_local) {
            /* VP is added to replication lists only if it resides on a local port */
            return BCM_E_PORT;
        }
    }

    sal_memset(&vtab, 0, sizeof(vlan_tab_entry_t));

    soc_mem_lock(unit, VLAN_TABm);

    rv = soc_mem_read(unit, VLAN_TABm, MEM_BLOCK_ANY, vlan, &vtab); 
    if (rv < 0) {
        soc_mem_unlock(unit, VLAN_TABm);
        return rv;
    }

    if (!soc_mem_field32_get(unit, VLAN_TABm, &vtab, VALIDf)) {
        soc_mem_unlock(unit, VLAN_TABm);
        return BCM_E_NOT_FOUND;
    }

    /* Enable VP switching on the VLAN */
    if (SOC_MEM_FIELD_VALID(unit, VLAN_TABm, VIRTUAL_PORT_ENf)) {
       if (!soc_mem_field32_get(unit, VLAN_TABm, &vtab, VIRTUAL_PORT_ENf)) {
           soc_mem_field32_set(unit, VLAN_TABm, &vtab, VIRTUAL_PORT_ENf, 1);
           rv = soc_mem_write(unit, VLAN_TABm, MEM_BLOCK_ALL, vlan, &vtab);
           if (rv < 0) {
               soc_mem_unlock(unit, VLAN_TABm);
               return rv;
           }
           soc_mem_unlock(unit, VLAN_TABm);

           /* Also need to copy the physical port members to the L2_BITMAP of
            * the IPMC entry for each group once we've gone virtual */
           rv = mbcm_driver[unit]->mbcm_vlan_port_get
               (unit, vlan, &vlan_pbmp, &vlan_ubmp, NULL);
           if (rv < 0) {
               return rv;
           }

           /* Deal with each group */
           for (i = 0; i < 3; i++) {
               mc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, group_type[i]);
               rv = _bcm_esw_multicast_ipmc_read(unit, mc_idx, &l2_pbmp, &l3_pbmp);
               if (rv < 0) {
                   return rv;
               }
               rv = _bcm_esw_multicast_ipmc_write(unit, mc_idx, vlan_pbmp,
                       l3_pbmp, TRUE);
               if (rv < 0) {
                   return rv;
               }
           }
       }
    } else { 
        soc_mem_unlock(unit, VLAN_TABm);
    }

    bc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, BC_IDXf);
    umc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, UMC_IDXf);
    uuc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, UUC_IDXf);

    if (flags & BCM_VLAN_GPORT_ADD_BCAST_DO_NOT_ADD) {
        bc_do_not_add = 1;
    } else {
        bc_do_not_add = 0;
    }
    if (flags & BCM_VLAN_GPORT_ADD_UNKNOWN_MCAST_DO_NOT_ADD) {
        umc_do_not_add = 1;
    } else {
        umc_do_not_add = 0;
    }
    if (flags & BCM_VLAN_GPORT_ADD_UNKNOWN_UCAST_DO_NOT_ADD) {
        uuc_do_not_add = 1;
    } else {
        uuc_do_not_add = 0;
    }

    if ((bc_idx == umc_idx) && (bc_do_not_add != umc_do_not_add)) {
        return BCM_E_PARAM;
    }
    if ((bc_idx == uuc_idx) && (bc_do_not_add != uuc_do_not_add)) {
        return BCM_E_PARAM;
    }
    if ((umc_idx == uuc_idx) && (umc_do_not_add != uuc_do_not_add)) {
        return BCM_E_PARAM;
    }

    if (phy_port_trunk == BCM_GPORT_INVALID) {
        if (!bc_do_not_add || !umc_do_not_add || !uuc_do_not_add) {
            /* If VP's physical port is invalid, this VP cannot be added
             * to multicast groups. User should set the DO_NOT_ADD flags.
             */
            return BCM_E_PARAM;
        }
    }

    /* Add the VP to the BC group */
    BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, bc_idx,
                &group));
    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_multicast_vlan_encap_get(unit, group, phy_port_trunk,
                                              gport, &encap_id));
    } else if (BCM_GPORT_IS_NIV_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_multicast_niv_encap_get(unit, group, phy_port_trunk,
                                             gport, &encap_id));
    } else {
        BCM_IF_ERROR_RETURN
            (bcm_esw_multicast_extender_encap_get(unit, group, phy_port_trunk,
                                             gport, &encap_id));
    }
    if (!bc_do_not_add) {
        /* coverity[stack_use_overflow : FALSE] */
        BCM_IF_ERROR_RETURN(bcm_esw_multicast_egress_add(unit, group,
                    phy_port_trunk, encap_id)); 
    }

    /* Add the VP to the UMC group, if UMC group is different from BC group */
    BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, umc_idx,
                &group));
    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_multicast_vlan_encap_get(unit, group, phy_port_trunk,
                                              gport, &encap_id));
    } else if (BCM_GPORT_IS_NIV_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_multicast_niv_encap_get(unit, group, phy_port_trunk,
                                             gport, &encap_id));
    } else {
        BCM_IF_ERROR_RETURN
            (bcm_esw_multicast_extender_encap_get(unit, group, phy_port_trunk,
                                             gport, &encap_id));
    }
    if (umc_idx != bc_idx) {
        if (!umc_do_not_add) {
            BCM_IF_ERROR_RETURN(bcm_esw_multicast_egress_add(unit, group,
                        phy_port_trunk, encap_id)); 
        }
    }

    /* Add the VP to the UUC group, if UUC group is different from BC/UMC group */
    BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, uuc_idx,
                &group));
    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_multicast_vlan_encap_get(unit, group, phy_port_trunk,
                                              gport, &encap_id));
    } else if (BCM_GPORT_IS_NIV_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (bcm_esw_multicast_niv_encap_get(unit, group, phy_port_trunk,
                                             gport, &encap_id));
    } else {
        BCM_IF_ERROR_RETURN
            (bcm_esw_multicast_extender_encap_get(unit, group, phy_port_trunk,
                                             gport, &encap_id));
    }
    if ((uuc_idx != bc_idx) && (uuc_idx != umc_idx)) {
        if (!uuc_do_not_add) {
            BCM_IF_ERROR_RETURN(bcm_esw_multicast_egress_add(unit, group,
                        phy_port_trunk, encap_id)); 
        }
    }

    /* Update the VLAN table's port bitmap to contain the BC/UMC/UUC groups' 
     * L2 and L3 bitmaps, since the VLAN table's port bitmap is used for
     * ingress and egress VLAN membership checks.
     */
    BCM_PBMP_CLEAR(l2_l3_pbmp);
    for (i = 0; i < 3; i++) {
        mc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, group_type[i]);
        BCM_IF_ERROR_RETURN
            (_bcm_esw_multicast_ipmc_read(unit, mc_idx, &l2_pbmp, &l3_pbmp));
        BCM_PBMP_OR(l2_l3_pbmp, l2_pbmp);
        BCM_PBMP_OR(l2_l3_pbmp, l3_pbmp);
    }
    BCM_IF_ERROR_RETURN(bcm_tr2_update_vlan_pbmp(unit, vlan, &l2_l3_pbmp));

    /* Add (vlan, vp) to egress vlan translation table and
     * configure the egress untagging status.
     */
    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr2_vlan_vp_untagged_add(unit, vlan, vp, flags));
        egr_vt_added = TRUE;
    }
#ifdef BCM_TRIDENT_SUPPORT
    else if (BCM_GPORT_IS_NIV_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (bcm_trident_niv_untagged_add(unit, vlan, vp, flags, &egr_vt_added));
    }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    else if (BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (bcm_tr3_extender_untagged_add(unit, vlan, vp, flags, &egr_vt_added));
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    /* Configure ingress and egress VLAN membership checks */
    if (flags & BCM_VLAN_GPORT_ADD_VP_VLAN_MEMBERSHIP) {
#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_ing_vp_vlan_membership)) { 
            if (!(flags & BCM_VLAN_GPORT_ADD_EGRESS_ONLY)) {
                BCM_IF_ERROR_RETURN(bcm_td2_ing_vp_vlan_membership_add(unit, 
                              vp, vlan, flags));
                ing_vp_vlan_membership_added = TRUE;
            }
        } else
#endif /* BCM_TRIDENT2_SUPPORT */
        {
            if (!(flags & BCM_VLAN_GPORT_ADD_EGRESS_ONLY)) {
                return BCM_E_UNAVAIL;
            }
        }

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_egr_vp_vlan_membership)) {
            if (!(flags & BCM_VLAN_GPORT_ADD_INGRESS_ONLY)) {
                BCM_IF_ERROR_RETURN(bcm_td2_egr_vp_vlan_membership_add(unit, 
                             vp, vlan, flags));
                egr_vp_vlan_membership_added = TRUE;
            }
        } else
#endif /* BCM_TRIDENT2_SUPPORT */
        {
            if (!(flags & BCM_VLAN_GPORT_ADD_INGRESS_ONLY)) {
                return BCM_E_UNAVAIL;
            }
        }
    } else {
#ifdef BCM_TRIDENT_SUPPORT
        /* Due to change in VP's VLAN membership, may need to move VP to
         * another VP group.
         */
        if (soc_feature(unit, soc_feature_vp_group_ingress_vlan_membership)) {
            uint32 ing_filter_flags;

            BCM_IF_ERROR_RETURN(bcm_esw_port_vlan_member_get(unit, gport,
                        &ing_filter_flags)); 
            if ((ing_filter_flags & BCM_PORT_VLAN_MEMBER_INGRESS) &&
                !(ing_filter_flags & BCM_PORT_VLAN_MEMBER_VP_VLAN_MEMBERSHIP)) { 
                BCM_IF_ERROR_RETURN
                    (bcm_td_ing_vp_group_move(unit, vp, vlan, TRUE));
                ing_vp_group_added = TRUE;
            }
        }     
        if (soc_feature(unit, soc_feature_vp_group_egress_vlan_membership)) {
            uint32 eg_filter_flags;

            BCM_IF_ERROR_RETURN(bcm_esw_port_vlan_member_get(unit, gport,
                        &eg_filter_flags)); 
            if ((eg_filter_flags & BCM_PORT_VLAN_MEMBER_EGRESS) &&
                !(eg_filter_flags & BCM_PORT_VLAN_MEMBER_VP_VLAN_MEMBERSHIP)) { 
                BCM_IF_ERROR_RETURN
                    (bcm_td_eg_vp_group_move(unit, vp, vlan, TRUE));
                eg_vp_group_added = TRUE;
            }
        }
#endif /* BCM_TRIDENT_SUPPORT */
    } 

    if (bc_do_not_add && umc_do_not_add && uuc_do_not_add &&
        !egr_vt_added && !ing_vp_vlan_membership_added &&
        !egr_vp_vlan_membership_added && !ing_vp_group_added &&
        !eg_vp_group_added) {
        /* The given VP has not been added to VLAN's BC, UMC, and UUC groups.
         * It also has not been added to egress VLAN translation table, to
         * VP VLAN membership check table, or to a VP group.
         * This scenario does not make sense. Moreover, bcm_vlan_gport_get_all
         * would not be able to find this VP.
         */
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2_vlan_gport_delete
 * Purpose:
 *      Remove a VLAN/NIV/Extender virtual port from the specified vlan.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      vlan  - (IN) VLAN to remove virtual port from.
 *      gport - (IN) VP Gport ID
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_vlan_gport_delete(int unit, bcm_vlan_t vlan, bcm_gport_t gport) 
{
    int rv = BCM_E_NONE;
    int vp;
    _bcm_vp_type_e vp_type;
    bcm_vlan_port_t vlan_vp;
    bcm_gport_t phy_port_trunk;
    int is_local;
    vlan_tab_entry_t vtab;
    int bc_idx, umc_idx, uuc_idx;
    bcm_multicast_t group;
    bcm_if_t encap_id;
    int i, mc_idx;
    soc_field_t group_type[3] = {BC_IDXf, UMC_IDXf, UUC_IDXf};
    bcm_pbmp_t l2_pbmp, l3_pbmp, l2_l3_pbmp;
    int bc_not_found = TRUE;
    int umc_not_found = TRUE;
    int uuc_not_found = TRUE;
    int egr_vt_found = FALSE;
    int ing_vp_vlan_membership_found = FALSE;
    int egr_vp_vlan_membership_found = FALSE;
    int ing_vp_group_found = FALSE;
    int eg_vp_group_found = FALSE;

    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        vp = BCM_GPORT_VLAN_PORT_ID_GET(gport);
        vp_type = _bcmVpTypeVlan;

        /* Get the physical port or trunk the VP resides on */
        bcm_vlan_port_t_init(&vlan_vp);
        vlan_vp.vlan_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_tr2_vlan_vp_find(unit, &vlan_vp));
        phy_port_trunk = vlan_vp.port;

    } else
#ifdef BCM_TRIDENT_SUPPORT
    if (BCM_GPORT_IS_NIV_PORT(gport)) {
        bcm_niv_port_t niv_port;

        vp = BCM_GPORT_NIV_PORT_ID_GET(gport);
        vp_type = _bcmVpTypeNiv;

        /* Get the physical port or trunk the VP resides on */
        bcm_niv_port_t_init(&niv_port);
        niv_port.niv_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_trident_niv_port_get(unit, &niv_port));
        if (niv_port.flags & BCM_NIV_PORT_MATCH_NONE) {
            phy_port_trunk = BCM_GPORT_INVALID;
        } else {
            phy_port_trunk = niv_port.port;
        }

    } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        bcm_extender_port_t extender_port;

        vp = BCM_GPORT_EXTENDER_PORT_ID_GET(gport);
        vp_type = _bcmVpTypeExtender;

        /* Get the physical port or trunk the VP resides on */
        bcm_extender_port_t_init(&extender_port);
        extender_port.extender_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_tr3_extender_port_get(unit, &extender_port));
        phy_port_trunk = extender_port.port;

    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        return BCM_E_PARAM;
    }

    if (!_bcm_vp_used_get(unit, vp, vp_type)) {
        return BCM_E_NOT_FOUND;
    }

    if (phy_port_trunk != BCM_GPORT_INVALID) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr2_phy_port_trunk_is_local(unit, phy_port_trunk, &is_local));
        if (!is_local) {
            /* VP to be deleted must reside on a local port */
            return BCM_E_PORT;
        }
    }

    sal_memset(&vtab, 0, sizeof(vlan_tab_entry_t));

    SOC_IF_ERROR_RETURN
        (soc_mem_read(unit, VLAN_TABm, MEM_BLOCK_ANY, vlan, &vtab)); 

    if (!soc_mem_field32_get(unit, VLAN_TABm, &vtab, VALIDf)) {
        return BCM_E_NOT_FOUND;
    }

    if (SOC_MEM_FIELD_VALID(unit, VLAN_TABm, VIRTUAL_PORT_ENf)) {
       if (!soc_mem_field32_get(unit, VLAN_TABm, &vtab, VIRTUAL_PORT_ENf)) {
           /* VP switching is not enabled on the VLAN */
           return BCM_E_PORT;
       }
    }

    if (phy_port_trunk != BCM_GPORT_INVALID) {
        /* Delete the VP from the BC group */
        bc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, BC_IDXf);
        BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, bc_idx,
                    &group));
        if (BCM_GPORT_IS_VLAN_PORT(gport)) {
            BCM_IF_ERROR_RETURN
                (bcm_esw_multicast_vlan_encap_get(unit, group, phy_port_trunk,
                                                  gport, &encap_id));
        } else if (BCM_GPORT_IS_NIV_PORT(gport)) {
            BCM_IF_ERROR_RETURN
                (bcm_esw_multicast_niv_encap_get(unit, group, phy_port_trunk,
                                                 gport, &encap_id));
        } else {
            BCM_IF_ERROR_RETURN
                (bcm_esw_multicast_extender_encap_get(unit, group, phy_port_trunk,
                                                      gport, &encap_id));
        }
        rv = bcm_esw_multicast_egress_delete(unit, group, phy_port_trunk, encap_id); 
        /* BCM_E_NOT_FOUND is tolerated since the VP could have been added to the
         * VLAN with the BCM_VLAN_GPORT_ADD_BCAST_DO_NOT_ADD flag.
         */
        if (BCM_SUCCESS(rv)) {
            bc_not_found = FALSE;
        } else if (rv == BCM_E_NOT_FOUND) {
            bc_not_found = TRUE;
            rv = BCM_E_NONE;
        } else {
            return rv;
        }

        /* Delete the VP from the UMC group, if UMC group is different
         * from BC group.
         */ 
        umc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, UMC_IDXf);
        BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, umc_idx,
                    &group));
        if (BCM_GPORT_IS_VLAN_PORT(gport)) {
            BCM_IF_ERROR_RETURN
                (bcm_esw_multicast_vlan_encap_get(unit, group, phy_port_trunk,
                                                  gport, &encap_id));
        } else if (BCM_GPORT_IS_NIV_PORT(gport)) {
            BCM_IF_ERROR_RETURN
                (bcm_esw_multicast_niv_encap_get(unit, group, phy_port_trunk,
                                                 gport, &encap_id));
        } else {
            BCM_IF_ERROR_RETURN
                (bcm_esw_multicast_extender_encap_get(unit, group, phy_port_trunk,
                                                      gport, &encap_id));
        }
        if (umc_idx != bc_idx) {
            rv = bcm_esw_multicast_egress_delete(unit, group, phy_port_trunk, encap_id); 
            /* BCM_E_NOT_FOUND is tolerated since the VP could have been added to
             * the VLAN with the BCM_VLAN_GPORT_ADD_UNKNOWN_MCAST_DO_NOT_ADD flag.
             */
            if (BCM_SUCCESS(rv)) {
                umc_not_found = FALSE;
            } else if (rv == BCM_E_NOT_FOUND) {
                umc_not_found = TRUE;
                rv = BCM_E_NONE;
            } else {
                return rv;
            }
        } else {
            umc_not_found = bc_not_found;
        }

        /* Delete the VP from the UUC group, if UUC group is different
         * from BC and UMC groups.
         */
        uuc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, UUC_IDXf);
        BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, uuc_idx,
                    &group));
        if (BCM_GPORT_IS_VLAN_PORT(gport)) {
            BCM_IF_ERROR_RETURN
                (bcm_esw_multicast_vlan_encap_get(unit, group, phy_port_trunk,
                                                  gport, &encap_id));
        } else if (BCM_GPORT_IS_NIV_PORT(gport)) {
            BCM_IF_ERROR_RETURN
                (bcm_esw_multicast_niv_encap_get(unit, group, phy_port_trunk,
                                                 gport, &encap_id));
        } else {
            BCM_IF_ERROR_RETURN
                (bcm_esw_multicast_extender_encap_get(unit, group, phy_port_trunk,
                                                      gport, &encap_id));
        }
        if ((uuc_idx != bc_idx) && (uuc_idx != umc_idx)) {
            rv = bcm_esw_multicast_egress_delete(unit, group, phy_port_trunk, encap_id); 
            /* BCM_E_NOT_FOUND is tolerated since the VP could have been added to
             * the VLAN with the BCM_VLAN_GPORT_ADD_UNKNOWN_UCAST_DO_NOT_ADD flag.
             */
            if (BCM_SUCCESS(rv)) {
                uuc_not_found = FALSE;
            } else if (rv == BCM_E_NOT_FOUND) {
                uuc_not_found = TRUE;
                rv = BCM_E_NONE;
            } else {
                return rv;
            }
        } else if (uuc_idx == bc_idx) {
            uuc_not_found = bc_not_found;
        } else {
            uuc_not_found = umc_not_found;
        }
    }

    /* Update the VLAN table's port bitmap to contain the BC/UMC/UUC groups' 
     * L2 and L3 bitmaps, since the VLAN table's port bitmap is used for
     * ingress and egress VLAN membership checks.
     */
    BCM_PBMP_CLEAR(l2_l3_pbmp);
    for (i = 0; i < 3; i++) {
        mc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, group_type[i]);
        rv = _bcm_esw_multicast_ipmc_read(unit, mc_idx, &l2_pbmp, &l3_pbmp);
        if (rv < 0) {
            return rv;
        }
        BCM_PBMP_OR(l2_l3_pbmp, l2_pbmp);
        BCM_PBMP_OR(l2_l3_pbmp, l3_pbmp);
    }
    BCM_IF_ERROR_RETURN(bcm_tr2_update_vlan_pbmp(unit, vlan, &l2_l3_pbmp));

    /* Delete (vlan, vp) from egress vlan translation table */
    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr2_vlan_vp_untagged_delete(unit, vlan, vp));
        egr_vt_found = TRUE;
    }
#ifdef BCM_TRIDENT_SUPPORT
    else if (BCM_GPORT_IS_NIV_PORT(gport)) {
        rv = bcm_trident_niv_untagged_delete(unit, vlan, vp);
        if (BCM_SUCCESS(rv)) {
            egr_vt_found = TRUE;
        } else if (rv == BCM_E_NOT_FOUND) {
            egr_vt_found = FALSE;
            rv = BCM_E_NONE;
        } else {
            return rv;
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    else if (BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        rv = bcm_tr3_extender_untagged_delete(unit, vlan, vp);
        if (BCM_SUCCESS(rv)) {
            egr_vt_found = TRUE;
        } else if (rv == BCM_E_NOT_FOUND) {
            egr_vt_found = FALSE;
            rv = BCM_E_NONE;
        } else {
            return rv;
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_ing_vp_vlan_membership)) {
        rv = bcm_td2_ing_vp_vlan_membership_delete(unit, vp, vlan);
        if (BCM_SUCCESS(rv)) {
            ing_vp_vlan_membership_found = TRUE;
        } else if (rv == BCM_E_NOT_FOUND) {
            ing_vp_vlan_membership_found = FALSE;
            rv = BCM_E_NONE;
        } else {
            return rv;
        }
    }
        
    if (soc_feature(unit, soc_feature_egr_vp_vlan_membership)) {
        rv = bcm_td2_egr_vp_vlan_membership_delete(unit, vp, vlan);
        if (BCM_SUCCESS(rv)) {
            egr_vp_vlan_membership_found = TRUE;
        } else if (rv == BCM_E_NOT_FOUND) {
            egr_vp_vlan_membership_found = FALSE;
            rv = BCM_E_NONE;
        } else {
            return rv;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
    /* Due to change in VP's VLAN membership, may need to move VP to
     * another VP group.
     */
    if (soc_feature(unit, soc_feature_vp_group_ingress_vlan_membership)) {
        uint32 ing_filter_flags;

        BCM_IF_ERROR_RETURN(bcm_esw_port_vlan_member_get(unit, gport,
                    &ing_filter_flags)); 
        if ((ing_filter_flags & BCM_PORT_VLAN_MEMBER_INGRESS) &&
                !(ing_filter_flags & BCM_PORT_VLAN_MEMBER_VP_VLAN_MEMBERSHIP)) { 
            BCM_IF_ERROR_RETURN
                (bcm_td_ing_vp_group_move(unit, vp, vlan, FALSE));
            ing_vp_group_found = TRUE;
        }
    } 
    if (soc_feature(unit, soc_feature_vp_group_egress_vlan_membership)) {
        uint32 eg_filter_flags;

        BCM_IF_ERROR_RETURN(bcm_esw_port_vlan_member_get(unit, gport,
                    &eg_filter_flags)); 
        if ((eg_filter_flags & BCM_PORT_VLAN_MEMBER_EGRESS) &&
            !(eg_filter_flags & BCM_PORT_VLAN_MEMBER_VP_VLAN_MEMBERSHIP)) { 
            BCM_IF_ERROR_RETURN
                (bcm_td_eg_vp_group_move(unit, vp, vlan, FALSE));
            eg_vp_group_found = TRUE;
        }
    } 
#endif /* BCM_TRIDENT_SUPPORT */

    if (bc_not_found && umc_not_found && uuc_not_found &&
            !egr_vt_found && !ing_vp_vlan_membership_found &&
            !egr_vp_vlan_membership_found && !ing_vp_group_found &&
            !eg_vp_group_found) {
        return BCM_E_NOT_FOUND;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr2_vlan_mc_group_gport_is_member
 * Purpose:
 *      Determine if the given VP is a member of the multicast group.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      group - (IN) Multicast group
 *      gport - (IN) VP Gport ID
 *      phy_port_trunk - (IN) Gport ID for physical port or trunk on which
 *                            the VP resides
 *      is_member - (OUT) Indicates if given VP belongs to VLAN.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_vlan_mc_group_gport_is_member(int unit, bcm_multicast_t group,
        bcm_gport_t gport, bcm_gport_t phy_port_trunk, int *is_member) 
{
    int rv = BCM_E_NONE;
    bcm_if_t encap_id;
    bcm_trunk_t trunk_id;
    bcm_port_t trunk_member_port[SOC_MAX_NUM_PORTS];
    int trunk_local_ports = 0;
    int if_max;
    bcm_if_t *if_array;
    int if_count, if_cur;
    int i, mc_idx;

    if (NULL == is_member) {
        return BCM_E_PARAM;
    }
    *is_member = FALSE;

    /* Get VP's multicast encap id */
    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        BCM_IF_ERROR_RETURN(bcm_esw_multicast_vlan_encap_get(unit, group,
                    phy_port_trunk, gport, &encap_id));
    } else if (BCM_GPORT_IS_NIV_PORT(gport)) {
        BCM_IF_ERROR_RETURN(bcm_esw_multicast_niv_encap_get(unit, group,
                    phy_port_trunk, gport, &encap_id));
    } else if (BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        BCM_IF_ERROR_RETURN(bcm_esw_multicast_extender_encap_get(unit, group,
                    phy_port_trunk, gport, &encap_id));
    } else {
        return BCM_E_PARAM;
    }

    /* Get all the local ports of the trunk */
    if (BCM_GPORT_IS_TRUNK(phy_port_trunk)) {
        trunk_id = BCM_GPORT_TRUNK_GET(phy_port_trunk);
        rv = _bcm_trunk_id_validate(unit, trunk_id);
        if (BCM_FAILURE(rv)) {
            return BCM_E_PORT;
        }
        rv = _bcm_esw_trunk_local_members_get(unit, trunk_id,
                SOC_MAX_NUM_PORTS, trunk_member_port, &trunk_local_ports);
        if (BCM_FAILURE(rv)) {
            return BCM_E_PORT;
        }
    } 

    /* Find out if VP is a member of the multicast group */
    if_max = soc_mem_index_count(unit, EGR_L3_NEXT_HOPm);
    if_array = sal_alloc(if_max * sizeof(bcm_if_t),
            "temp repl interface array");
    if (if_array == NULL) {
        return BCM_E_MEMORY;
    }
    mc_idx = _BCM_MULTICAST_ID_GET(group);
    if (BCM_GPORT_IS_TRUNK(phy_port_trunk)) {
        /* Iterate over all local ports in the trunk and search for
         * a match on any local port
         */
        for (i = 0; i < trunk_local_ports; i++) {
            rv = bcm_esw_ipmc_egress_intf_get(unit, mc_idx,
                    trunk_member_port[i], if_max, if_array, &if_count);
            if (BCM_FAILURE(rv)) {
                sal_free(if_array);
                return rv;
            }
            for (if_cur = 0; if_cur < if_count; if_cur++) {
                if (if_array[if_cur] == encap_id) {
                    *is_member = TRUE;
                    sal_free(if_array);
                    return BCM_E_NONE;
                }
            }
        }
    } else {
        rv = bcm_esw_ipmc_egress_intf_get(unit, mc_idx,
                phy_port_trunk, if_max, if_array, &if_count);
        if (BCM_FAILURE(rv)) {
            sal_free(if_array);
            return rv;
        }
        for (if_cur = 0; if_cur < if_count; if_cur++) {
            if (if_array[if_cur] == encap_id) {
                *is_member = TRUE;
                sal_free(if_array);
                return BCM_E_NONE;
            }
        }
    }

    sal_free(if_array);
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr2_vlan_gport_get
 * Purpose:
 *      Get untagging status of a VLAN virtual port.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      vlan  - (IN) VLAN to remove virtual port from.
 *      gport - (IN) VLAN VP Gport ID
 *      flags - (OUT) Untagging status of the VLAN virtual port.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_vlan_gport_get(int unit, bcm_vlan_t vlan, bcm_gport_t gport,
        int *flags) 
{
    int rv = BCM_E_NONE;
    int vp;
    _bcm_vp_type_e vp_type;
    bcm_vlan_port_t vlan_vp;
    bcm_gport_t phy_port_trunk;
    int is_local;
    vlan_tab_entry_t vtab;
    int bc_idx, umc_idx, uuc_idx;
    bcm_multicast_t group;
    int is_bc_member = FALSE;
    int is_umc_member = FALSE;
    int is_uuc_member = FALSE;
    int untagged_flag = 0;
    int egr_vt_found = FALSE;
    int ing_vp_vlan_membership_found = FALSE;
    int egr_vp_vlan_membership_found = FALSE;
    int ing_vp_group_found = FALSE;
    int eg_vp_group_found = FALSE;

    *flags = 0;

    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        vp = BCM_GPORT_VLAN_PORT_ID_GET(gport);
        vp_type = _bcmVpTypeVlan;

        /* Get the physical port or trunk the VP resides on */
        bcm_vlan_port_t_init(&vlan_vp);
        vlan_vp.vlan_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_tr2_vlan_vp_find(unit, &vlan_vp));
        phy_port_trunk = vlan_vp.port;

    } else
#ifdef BCM_TRIDENT_SUPPORT
    if (BCM_GPORT_IS_NIV_PORT(gport)) {
        bcm_niv_port_t niv_port;

        vp = BCM_GPORT_NIV_PORT_ID_GET(gport);
        vp_type = _bcmVpTypeNiv;

        /* Get the physical port or trunk the VP resides on */
        bcm_niv_port_t_init(&niv_port);
        niv_port.niv_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_trident_niv_port_get(unit, &niv_port));
        if (niv_port.flags & BCM_NIV_PORT_MATCH_NONE) {
            phy_port_trunk = BCM_GPORT_INVALID;
        } else {
            phy_port_trunk = niv_port.port;
        }

    } else
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    if (BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        bcm_extender_port_t extender_port;

        vp = BCM_GPORT_EXTENDER_PORT_ID_GET(gport);
        vp_type = _bcmVpTypeExtender;

        /* Get the physical port or trunk the VP resides on */
        bcm_extender_port_t_init(&extender_port);
        extender_port.extender_port_id = gport;
        BCM_IF_ERROR_RETURN(bcm_tr3_extender_port_get(unit, &extender_port));
        phy_port_trunk = extender_port.port;

    } else
#endif /* BCM_TRIDENT_SUPPORT */
    {
        return BCM_E_PARAM;
    }

    if (!_bcm_vp_used_get(unit, vp, vp_type)) {
        return BCM_E_NOT_FOUND;
    }

    if (phy_port_trunk != BCM_GPORT_INVALID) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr2_phy_port_trunk_is_local(unit, phy_port_trunk, &is_local));
        if (!is_local) {
            return BCM_E_PORT;
        }
    }

    sal_memset(&vtab, 0, sizeof(vlan_tab_entry_t));
    SOC_IF_ERROR_RETURN
        (soc_mem_read(unit, VLAN_TABm, MEM_BLOCK_ANY, vlan, &vtab));
    if (!soc_mem_field32_get(unit, VLAN_TABm, &vtab, VALIDf)) {
        return BCM_E_NOT_FOUND;
    }

    if (SOC_MEM_FIELD_VALID(unit, VLAN_TABm, VIRTUAL_PORT_ENf)) {
       if (!soc_mem_field32_get(unit, VLAN_TABm, &vtab, VIRTUAL_PORT_ENf)) {
           /* No virtual port exists for this VLAN */
           return BCM_E_NOT_FOUND;
       } 
    }

    if (phy_port_trunk != BCM_GPORT_INVALID) {
        bc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, BC_IDXf);
        BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, bc_idx,
                    &group));
        BCM_IF_ERROR_RETURN(_bcm_tr2_vlan_mc_group_gport_is_member(unit, group,
                    gport, phy_port_trunk, &is_bc_member));

        umc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, UMC_IDXf);
        if (umc_idx != bc_idx) {
            BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, umc_idx,
                        &group));
            BCM_IF_ERROR_RETURN(_bcm_tr2_vlan_mc_group_gport_is_member(unit, group,
                        gport, phy_port_trunk, &is_umc_member));
        } else {
            is_umc_member = is_bc_member;
        }

        uuc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, UUC_IDXf);
        if (uuc_idx != bc_idx && uuc_idx != umc_idx) {
            BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, uuc_idx,
                        &group));
            BCM_IF_ERROR_RETURN(_bcm_tr2_vlan_mc_group_gport_is_member(unit, group,
                        gport, phy_port_trunk, &is_uuc_member));
        } else if (uuc_idx == bc_idx) {
            is_uuc_member = is_bc_member;
        } else {
            is_uuc_member = is_umc_member;
        }
    }

    if (!is_bc_member) {
        *flags |= BCM_VLAN_GPORT_ADD_BCAST_DO_NOT_ADD;
    }
    if (!is_umc_member) {
        *flags |= BCM_VLAN_GPORT_ADD_UNKNOWN_MCAST_DO_NOT_ADD;
    }
    if (!is_uuc_member) {
        *flags |= BCM_VLAN_GPORT_ADD_UNKNOWN_UCAST_DO_NOT_ADD;
    }

    /* Get untagging status */
    if (BCM_GPORT_IS_VLAN_PORT(gport)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr2_vlan_vp_untagged_get(unit, vlan, vp, &untagged_flag));
        egr_vt_found = TRUE;
    }
#ifdef BCM_TRIDENT_SUPPORT
    else if (BCM_GPORT_IS_NIV_PORT(gport)) {
        rv = bcm_trident_niv_untagged_get(unit, vlan, vp, &untagged_flag);
        if (BCM_SUCCESS(rv)) {
            egr_vt_found = TRUE;
        } else if (rv == BCM_E_NOT_FOUND) {
            egr_vt_found = FALSE;
            rv = BCM_E_NONE;
        } else {
            return rv;
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
    else if (BCM_GPORT_IS_EXTENDER_PORT(gport)) {
        rv = bcm_tr3_extender_untagged_get(unit, vlan, vp, &untagged_flag);
        if (BCM_SUCCESS(rv)) {
            egr_vt_found = TRUE;
        } else if (rv == BCM_E_NOT_FOUND) {
            egr_vt_found = FALSE;
            rv = BCM_E_NONE;
        } else {
            return rv;
        }
    }
#endif /* BCM_TRIUMPH3_SUPPORT */

    *flags |= untagged_flag;

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_ing_vp_vlan_membership)) {
        uint32 ing_flag;

        rv = bcm_td2_ing_vp_vlan_membership_get(unit, vp, vlan, &ing_flag);
        if (BCM_SUCCESS(rv)) {
            ing_vp_vlan_membership_found = TRUE;
            *flags |= ing_flag;
            *flags |= BCM_VLAN_GPORT_ADD_VP_VLAN_MEMBERSHIP;
        } else if (rv == BCM_E_NOT_FOUND) {
            ing_vp_vlan_membership_found = FALSE;
            rv = BCM_E_NONE;
        } else {
            return rv;
        }
    }
        
    if (soc_feature(unit, soc_feature_egr_vp_vlan_membership)) {
        uint32 egr_flag;

        rv = bcm_td2_egr_vp_vlan_membership_get(unit, vp, vlan, &egr_flag);
        if (BCM_SUCCESS(rv)) {
            egr_vp_vlan_membership_found = TRUE;
            *flags |= egr_flag;
            *flags |= BCM_VLAN_GPORT_ADD_VP_VLAN_MEMBERSHIP;
        } else if (rv == BCM_E_NOT_FOUND) {
            egr_vp_vlan_membership_found = FALSE;
            rv = BCM_E_NONE;
        } else {
            return rv;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
    if (soc_feature(unit, soc_feature_vp_group_ingress_vlan_membership)) {
        uint32 ing_filter_flags;

        BCM_IF_ERROR_RETURN(bcm_esw_port_vlan_member_get(unit, gport,
                    &ing_filter_flags)); 
        if ((ing_filter_flags & BCM_PORT_VLAN_MEMBER_INGRESS) &&
                !(ing_filter_flags & BCM_PORT_VLAN_MEMBER_VP_VLAN_MEMBERSHIP)) { 
            ing_vp_group_found = TRUE;
        }
    } 
    if (soc_feature(unit, soc_feature_vp_group_egress_vlan_membership)) {
        uint32 eg_filter_flags;

        BCM_IF_ERROR_RETURN(bcm_esw_port_vlan_member_get(unit, gport,
                    &eg_filter_flags)); 
        if ((eg_filter_flags & BCM_PORT_VLAN_MEMBER_EGRESS) &&
            !(eg_filter_flags & BCM_PORT_VLAN_MEMBER_VP_VLAN_MEMBERSHIP)) { 
            eg_vp_group_found = TRUE;
        }
    } 
#endif /* BCM_TRIDENT_SUPPORT */

    if (!is_bc_member && !is_umc_member && !is_uuc_member &&
            !egr_vt_found && !ing_vp_vlan_membership_found &&
            !egr_vp_vlan_membership_found && !ing_vp_group_found &&
            !eg_vp_group_found) {
        return BCM_E_NOT_FOUND;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_tr2_mc_group_vp_get_all
 * Purpose:
 *      Get all the virtual ports of the given multicast group.
 * Parameters:
 *      unit   - (IN) SOC unit number. 
 *      mc_group - (IN) Multicast group.
 *      vp_bitmap - (OUT) Bitmap of virtual ports.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_mc_group_vp_get_all(int unit, bcm_multicast_t mc_group,
        SHR_BITDCL *vp_bitmap)
{
    int rv = BCM_E_NONE;
    int num_encap_id;
    bcm_if_t *encap_id_array = NULL;
    int i;
    int nh_index;
    egr_l3_next_hop_entry_t egr_nh;
    int vp;

    if (NULL == vp_bitmap) {
        return BCM_E_PARAM;
    }

    /* Get the number of encap IDs of the multicast group */
    BCM_IF_ERROR_RETURN
        (bcm_esw_multicast_egress_get(unit, mc_group, 0,
                                      NULL, NULL, &num_encap_id));

    /* Get all the encap IDs of the multicast group */
    encap_id_array = sal_alloc(sizeof(bcm_if_t) * num_encap_id,
            "encap_id_array");
    if (NULL == encap_id_array) {
        return BCM_E_MEMORY;
    }
    sal_memset(encap_id_array, 0, sizeof(bcm_if_t) * num_encap_id);
    rv = bcm_esw_multicast_egress_get(unit, mc_group, num_encap_id,
                                      NULL, encap_id_array, &num_encap_id);
    if (rv < 0) {
        sal_free(encap_id_array);
        return rv;
    }

    /* Convert encap IDs to virtual ports */
    for (i = 0; i < num_encap_id; i++) {
        if (BCM_IF_INVALID != encap_id_array[i]) {
            nh_index = encap_id_array[i] - BCM_XGS3_DVP_EGRESS_IDX_MIN;
            rv = READ_EGR_L3_NEXT_HOPm(unit, MEM_BLOCK_ANY, nh_index, &egr_nh); 
            if (BCM_FAILURE(rv)) {
                sal_free(encap_id_array);
                return rv;
            }
            if (2 != soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh,
                        ENTRY_TYPEf)) {
                sal_free(encap_id_array);
                return BCM_E_INTERNAL;
            }
            vp = soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh, SD_TAG__DVPf);
            SHR_BITSET(vp_bitmap, vp);
        }
    }

    sal_free(encap_id_array);
    return rv;
}

/*
 * Function:
 *      _bcm_tr2_vp_untagged_get_all
 * Purpose:
 *      Get all the given VLAN's virtual ports and untagged status from egress
 *      VLAN translation table.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN.
 *      vp_bitmap - (OUT) Bitmap of virtual ports.
 *      arr_size  - (IN) Number of elements in flags_arr.
 *      flags_arr - (OUT) Array of flags.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_vp_untagged_get_all(int unit, bcm_vlan_t vlan, SHR_BITDCL *vp_bitmap,
        int arr_size, int *flags_arr)
{
    int rv = BCM_E_NONE;
    int chunk_size, num_chunks, chunk_index;
    int alloc_size;
    uint8 *egr_vt_buf = NULL;
    soc_field_t entry_type_f;
    int i, entry_index_min, entry_index_max;
    uint32 *egr_vt_entry;
    int vp;
    int untagged_flag;

    if (NULL == vp_bitmap) {
        return BCM_E_PARAM;
    }
    if (arr_size != soc_mem_index_count(unit, SOURCE_VPm)) {
        return BCM_E_PARAM;
    } 
    if (NULL == flags_arr) {
        return BCM_E_PARAM;
    } 

    chunk_size = 1024;
    num_chunks = soc_mem_index_count(unit, EGR_VLAN_XLATEm) / chunk_size;
    if (soc_mem_index_count(unit, EGR_VLAN_XLATEm) % chunk_size) {
        num_chunks++;
    }

    alloc_size = sizeof(egr_vlan_xlate_entry_t) * chunk_size;
    egr_vt_buf = soc_cm_salloc(unit, alloc_size, "EGR_VLAN_XLATE buffer");
    if (NULL == egr_vt_buf) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }

    if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, ENTRY_TYPEf)) {
        entry_type_f = ENTRY_TYPEf;
    } else {
        entry_type_f = KEY_TYPEf;
    }

    for (chunk_index = 0; chunk_index < num_chunks; chunk_index++) {

        /* Get a chunk of entries */
        entry_index_min = chunk_index * chunk_size;
        entry_index_max = entry_index_min + chunk_size - 1;
        if (entry_index_max > soc_mem_index_max(unit, EGR_VLAN_XLATEm)) {
            entry_index_max = soc_mem_index_max(unit, EGR_VLAN_XLATEm);
        }
        rv = soc_mem_read_range(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ANY,
                entry_index_min, entry_index_max, egr_vt_buf);
        if (SOC_FAILURE(rv)) {
            goto cleanup;
        }

        /* Read each entry of the chunk to extract VP and untagged status */
        for (i = 0; i < (entry_index_max - entry_index_min + 1); i++) {
            egr_vt_entry = soc_mem_table_idx_to_pointer(unit,
                    EGR_VLAN_XLATEm, uint32 *, egr_vt_buf, i);

            if (soc_mem_field32_get(unit, EGR_VLAN_XLATEm, egr_vt_entry,
                        VALIDf) == 0) {
                continue;
            }
            if (soc_mem_field32_get(unit, EGR_VLAN_XLATEm, egr_vt_entry,
                        entry_type_f) != 1) {
                continue;
            }
            if (soc_mem_field32_get(unit, EGR_VLAN_XLATEm, egr_vt_entry,
                        OVIDf) != vlan) {
                continue;
            }

            vp = soc_mem_field32_get(unit, EGR_VLAN_XLATEm, egr_vt_entry,
                    DVPf);

            /* Get untagged status */
            if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
                rv = _bcm_tr2_vlan_vp_untagged_get(unit, vlan, vp,
                        &untagged_flag);
            }
#ifdef BCM_TRIDENT_SUPPORT
            else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv)) {
                rv = bcm_trident_niv_untagged_get(unit, vlan, vp,
                        &untagged_flag);
            }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
            else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                rv = bcm_tr3_extender_untagged_get(unit, vlan, vp,
                        &untagged_flag);
            }
#endif /* BCM_TRIUMPH3_SUPPORT */
            else {
                continue;
            }
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }

            SHR_BITSET(vp_bitmap, vp);
            flags_arr[vp] = untagged_flag;
        }
    }

cleanup:
    if (egr_vt_buf) {
        soc_cm_sfree(unit, egr_vt_buf);
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr2_vlan_gport_get_all
 * Purpose:
 *      Get all virtual ports of the given VLAN.
 * Parameters:
 *      unit      - (IN) SOC unit number. 
 *      vlan      - (IN) VLAN ID.
 *      array_max - (IN) Max number of elements in the array.
 *      gport_array - (OUT) Array of virtual ports in GPORT format.
 *      flags_array - (OUT) Array of untagging status.
 *      array_size  - (OUT) Actual number of elements in the array.
 * Returns:
 *      BCM_E_XXX
 * Notes: If array_max == 0 and gport_array == NULL, actual number of
 *        virtual ports in the VLAN will be returned in array_size.
 */
int
bcm_tr2_vlan_gport_get_all(int unit, bcm_vlan_t vlan, int array_max,
        bcm_gport_t *gport_array, int *flags_array, int *array_size) 
{
    int rv = BCM_E_NONE;
    vlan_tab_entry_t vtab;
    int num_vp;
    SHR_BITDCL *bc_vp_bitmap = NULL;
    SHR_BITDCL *umc_vp_bitmap = NULL;
    SHR_BITDCL *uuc_vp_bitmap = NULL;
    int bc_idx, umc_idx, uuc_idx;
    bcm_multicast_t mc_group;
    SHR_BITDCL *egr_vt_vp_bitmap = NULL;
    int *egr_vt_flags_arr = NULL;
    SHR_BITDCL *ing_vp_bitmap = NULL;
    int *ing_flags_arr = NULL;
    SHR_BITDCL *egr_vp_bitmap = NULL;
    int *egr_flags_arr = NULL;
    SHR_BITDCL *ing_group_vp_bitmap = NULL;
    SHR_BITDCL *eg_group_vp_bitmap = NULL;
    int i;

    if (array_max < 0) {
        return BCM_E_PARAM;
    }

    if ((array_max > 0) && (NULL == gport_array)) {
        return BCM_E_PARAM;
    }

    if (NULL == array_size) {
        return BCM_E_PARAM;
    }

    *array_size = 0;

    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, VLAN_TABm, MEM_BLOCK_ANY, (int)vlan, &vtab));

    if (SOC_MEM_FIELD_VALID(unit, VLAN_TABm, VIRTUAL_PORT_ENf)) {
       if (!soc_mem_field32_get(unit, VLAN_TABm, &vtab, VIRTUAL_PORT_ENf)) {
           /* This vlan has no virtual port members. */
           return BCM_E_NONE;
       } 
    }

    /* Get VPs from BC group */
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    bc_vp_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_vp), "bc_vp_bitmap");
    if (NULL == bc_vp_bitmap) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(bc_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    bc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, BC_IDXf);
    rv = _bcm_tr_multicast_ipmc_group_type_get(unit, bc_idx, &mc_group);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }
    rv = _bcm_tr2_mc_group_vp_get_all(unit, mc_group, bc_vp_bitmap);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    /* Get VPs from UMC group, if different from BC group */
    umc_vp_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_vp), "umc_vp_bitmap");
    if (NULL == umc_vp_bitmap) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(umc_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    umc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, UMC_IDXf);
    rv = _bcm_tr_multicast_ipmc_group_type_get(unit, umc_idx, &mc_group);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }
    if (umc_idx != bc_idx) {
        rv = _bcm_tr2_mc_group_vp_get_all(unit, mc_group, umc_vp_bitmap);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
    } else {
        sal_memcpy(umc_vp_bitmap, bc_vp_bitmap, SHR_BITALLOCSIZE(num_vp));
    }

    /* Get VPs from UUC group, if different from BC and UMC group */
    uuc_vp_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_vp), "uuc_vp_bitmap");
    if (NULL == uuc_vp_bitmap) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(uuc_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    uuc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, UUC_IDXf);
    rv = _bcm_tr_multicast_ipmc_group_type_get(unit, uuc_idx, &mc_group);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }
    if ((uuc_idx != bc_idx) && (uuc_idx != umc_idx)) {
        rv = _bcm_tr2_mc_group_vp_get_all(unit, mc_group, uuc_vp_bitmap);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
    } else if (uuc_idx == bc_idx) {
        sal_memcpy(uuc_vp_bitmap, bc_vp_bitmap, SHR_BITALLOCSIZE(num_vp));
    } else {
        sal_memcpy(uuc_vp_bitmap, umc_vp_bitmap, SHR_BITALLOCSIZE(num_vp));
    }

    /* Get VPs and untagged status from egress VLAN translation table */
    egr_vt_vp_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_vp), "egr_vt_vp_bitmap");
    if (NULL == egr_vt_vp_bitmap) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(egr_vt_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    
    egr_vt_flags_arr = sal_alloc(num_vp * sizeof(int), "egr_vt_flags_arr");
    if (NULL == egr_vt_flags_arr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(egr_vt_flags_arr, 0, num_vp * sizeof(int));

    rv = _bcm_tr2_vp_untagged_get_all(unit, vlan, egr_vt_vp_bitmap,
            num_vp, egr_vt_flags_arr); 
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    /* Get VPs and flags from ingress VP VLAN membership table */
    ing_vp_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_vp), "ing_vp_bitmap");
    if (NULL == ing_vp_bitmap) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(ing_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    
    ing_flags_arr = sal_alloc(num_vp * sizeof(int), "ing_flags_arr");
    if (NULL == ing_flags_arr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(ing_flags_arr, 0, num_vp * sizeof(int));

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_ing_vp_vlan_membership)) {
        rv = bcm_td2_ing_vp_vlan_membership_get_all(unit, vlan, ing_vp_bitmap,
                num_vp, ing_flags_arr);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */
        
    /* Get VPs and flags from egress VP VLAN membership table */
    egr_vp_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_vp), "egr_vp_bitmap");
    if (NULL == egr_vp_bitmap) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(egr_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    
    egr_flags_arr = sal_alloc(num_vp * sizeof(int), "egr_flags_arr");
    if (NULL == egr_flags_arr) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(egr_flags_arr, 0, num_vp * sizeof(int));

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_egr_vp_vlan_membership)) {
        rv = bcm_td2_egr_vp_vlan_membership_get_all(unit, vlan, egr_vp_bitmap,
                num_vp, egr_flags_arr);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    /* Get VPs from ingress VLAN VP groups */
    ing_group_vp_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_vp),
            "ing_group_vp_bitmap");
    if (NULL == ing_group_vp_bitmap) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(ing_group_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    
#ifdef BCM_TRIDENT_SUPPORT
    if (soc_feature(unit, soc_feature_vp_group_ingress_vlan_membership)) {
        rv = bcm_td_ing_vp_group_vlan_get_all(unit, vlan, ing_group_vp_bitmap);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */

    /* Get VPs from egress VLAN VP groups */
    eg_group_vp_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_vp),
            "eg_group_vp_bitmap");
    if (NULL == eg_group_vp_bitmap) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(eg_group_vp_bitmap, 0, SHR_BITALLOCSIZE(num_vp));
    
#ifdef BCM_TRIDENT_SUPPORT
    if (soc_feature(unit, soc_feature_vp_group_egress_vlan_membership)) {
        rv = bcm_td_eg_vp_group_vlan_get_all(unit, vlan, eg_group_vp_bitmap);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */

    /* Combine the VP bitmaps and the flags arrays */
    for (i = 0; i < num_vp; i++) {
        if (!SHR_BITGET(bc_vp_bitmap, i) &&
            !SHR_BITGET(umc_vp_bitmap, i) &&
            !SHR_BITGET(uuc_vp_bitmap, i) &&
            !SHR_BITGET(egr_vt_vp_bitmap, i) &&
            !SHR_BITGET(ing_vp_bitmap, i) &&
            !SHR_BITGET(egr_vp_bitmap, i) &&
            !SHR_BITGET(ing_group_vp_bitmap, i) &&
            !SHR_BITGET(eg_group_vp_bitmap, i)) {
            /* VP is not a member of this VLAN */
            continue;
        }

        if (0 == array_max) {
            (*array_size)++;
        } else {
            if (_bcm_vp_used_get(unit, i, _bcmVpTypeVlan)) {
                BCM_GPORT_VLAN_PORT_ID_SET(gport_array[*array_size], i);
            } else if (_bcm_vp_used_get(unit, i, _bcmVpTypeNiv)) {
                BCM_GPORT_NIV_PORT_ID_SET(gport_array[*array_size], i);
            } else if (_bcm_vp_used_get(unit, i, _bcmVpTypeExtender)) {
                BCM_GPORT_EXTENDER_PORT_ID_SET(gport_array[*array_size], i);
            } else {
                /* Unexpected VP type */
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }

            if (NULL != flags_array) {
                flags_array[*array_size] = egr_vt_flags_arr[i] |
                    ing_flags_arr[i] | egr_flags_arr[i];
                if (!SHR_BITGET(bc_vp_bitmap, i)) {
                    flags_array[*array_size] |= BCM_VLAN_GPORT_ADD_BCAST_DO_NOT_ADD;
                }
                if (!SHR_BITGET(umc_vp_bitmap, i)) {
                    flags_array[*array_size] |= BCM_VLAN_GPORT_ADD_UNKNOWN_MCAST_DO_NOT_ADD;
                }
                if (!SHR_BITGET(uuc_vp_bitmap, i)) {
                    flags_array[*array_size] |= BCM_VLAN_GPORT_ADD_UNKNOWN_UCAST_DO_NOT_ADD;
                }
                if (SHR_BITGET(ing_vp_bitmap, i)) {
                    flags_array[*array_size] |= BCM_VLAN_GPORT_ADD_VP_VLAN_MEMBERSHIP;
                }
                if (SHR_BITGET(egr_vp_bitmap, i)) {
                    flags_array[*array_size] |= BCM_VLAN_GPORT_ADD_VP_VLAN_MEMBERSHIP;
                }
            }

            (*array_size)++;
            if (*array_size == array_max) {
                break;
            }
        }
    }

cleanup:
    if (NULL != bc_vp_bitmap) {
        sal_free(bc_vp_bitmap);
    }
    if (NULL != umc_vp_bitmap) {
        sal_free(umc_vp_bitmap);
    }
    if (NULL != uuc_vp_bitmap) {
        sal_free(uuc_vp_bitmap);
    }
    if (NULL != egr_vt_vp_bitmap) {
        sal_free(egr_vt_vp_bitmap);
    }
    if (NULL != egr_vt_flags_arr) {
        sal_free(egr_vt_flags_arr);
    }
    if (NULL != ing_vp_bitmap) {
        sal_free(ing_vp_bitmap);
    }
    if (NULL != ing_flags_arr) {
        sal_free(ing_flags_arr);
    }
    if (NULL != egr_vp_bitmap) {
        sal_free(egr_vp_bitmap);
    }
    if (NULL != egr_flags_arr) {
        sal_free(egr_flags_arr);
    }
    if (NULL != ing_group_vp_bitmap) {
        sal_free(ing_group_vp_bitmap);
    }
    if (NULL != eg_group_vp_bitmap) {
        sal_free(eg_group_vp_bitmap);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_tr2_vp_untagged_delete_all
 * Purpose:
 *      Delete all the given VLAN's virtual ports from egress
 *      VLAN translation table.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      vlan - (IN) VLAN.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr2_vp_untagged_delete_all(int unit, bcm_vlan_t vlan)
{
    int rv = BCM_E_NONE;
    int chunk_size, num_chunks, chunk_index;
    int alloc_size;
    uint8 *egr_vt_buf = NULL;
    soc_field_t entry_type_f;
    int i, entry_index_min, entry_index_max;
    uint32 *egr_vt_entry;
    int vp;

    chunk_size = 1024;
    num_chunks = soc_mem_index_count(unit, EGR_VLAN_XLATEm) / chunk_size;
    if (soc_mem_index_count(unit, EGR_VLAN_XLATEm) % chunk_size) {
        num_chunks++;
    }

    alloc_size = sizeof(egr_vlan_xlate_entry_t) * chunk_size;
    egr_vt_buf = soc_cm_salloc(unit, alloc_size, "EGR_VLAN_XLATE buffer");
    if (NULL == egr_vt_buf) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }

    if (soc_mem_field_valid(unit, EGR_VLAN_XLATEm, ENTRY_TYPEf)) {
        entry_type_f = ENTRY_TYPEf;
    } else {
        entry_type_f = KEY_TYPEf;
    }

    for (chunk_index = 0; chunk_index < num_chunks; chunk_index++) {

        /* Get a chunk of entries */
        entry_index_min = chunk_index * chunk_size;
        entry_index_max = entry_index_min + chunk_size - 1;
        if (entry_index_max > soc_mem_index_max(unit, EGR_VLAN_XLATEm)) {
            entry_index_max = soc_mem_index_max(unit, EGR_VLAN_XLATEm);
        }
        rv = soc_mem_read_range(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ANY,
                entry_index_min, entry_index_max, egr_vt_buf);
        if (SOC_FAILURE(rv)) {
            goto cleanup;
        }

        /* Read each entry of the chunk */
        for (i = 0; i < (entry_index_max - entry_index_min + 1); i++) {
            egr_vt_entry = soc_mem_table_idx_to_pointer(unit,
                    EGR_VLAN_XLATEm, uint32 *, egr_vt_buf, i);

            if (soc_mem_field32_get(unit, EGR_VLAN_XLATEm, egr_vt_entry,
                        VALIDf) == 0) {
                continue;
            }
            if (soc_mem_field32_get(unit, EGR_VLAN_XLATEm, egr_vt_entry,
                        entry_type_f) != 1) {
                continue;
            }
            if (soc_mem_field32_get(unit, EGR_VLAN_XLATEm, egr_vt_entry,
                        OVIDf) != vlan) {
                continue;
            }

            vp = soc_mem_field32_get(unit, EGR_VLAN_XLATEm, egr_vt_entry,
                    DVPf);

            if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
                rv = _bcm_tr2_vlan_vp_untagged_delete(unit, vlan, vp);
            }
#ifdef BCM_TRIDENT_SUPPORT
            else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv)) {
                rv = bcm_trident_niv_untagged_delete(unit, vlan, vp);
            }
#endif /* BCM_TRIDENT_SUPPORT */
#ifdef BCM_TRIUMPH3_SUPPORT
            else if (_bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                rv = bcm_tr3_extender_untagged_delete(unit, vlan, vp);
            }
#endif /* BCM_TRIUMPH3_SUPPORT */
            else {
                continue;
            }
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
        }
    }

cleanup:
    if (egr_vt_buf) {
        soc_cm_sfree(unit, egr_vt_buf);
    }

    return rv;
}
/*
 * Function:
 *      bcm_tr2_vlan_gport_delete_all
 * Purpose:
 *      Remove all VLAN virtual ports from the specified vlan.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      vlan  - (IN) VLAN to remove virtual ports from.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr2_vlan_gport_delete_all(int unit, bcm_vlan_t vlan) 
{
    vlan_tab_entry_t vtab;
    int bc_idx, umc_idx, uuc_idx;
    bcm_multicast_t mc_group;

    sal_memset(&vtab, 0, sizeof(vlan_tab_entry_t));

    BCM_IF_ERROR_RETURN
        (soc_mem_read(unit, VLAN_TABm, MEM_BLOCK_ANY, vlan, &vtab)); 

    if (!soc_mem_field32_get(unit, VLAN_TABm, &vtab, VALIDf)) {
        return BCM_E_NOT_FOUND;
    }

    if (SOC_MEM_FIELD_VALID(unit, VLAN_TABm, VIRTUAL_PORT_ENf)) {
       if (!soc_mem_field32_get(unit, VLAN_TABm, &vtab, VIRTUAL_PORT_ENf)) {
           /* There are no virtual ports to remove. */
           return BCM_E_NONE;
       }
    }

    /* Delete all VPs from the BC group */
    bc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, BC_IDXf);
    BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, bc_idx,
                &mc_group));
    BCM_IF_ERROR_RETURN(bcm_esw_multicast_egress_delete_all(unit, mc_group)); 

    /* Delete all VPs from the UMC group, if UMC group is different
     * from BC group.
     */ 
    umc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, UMC_IDXf);
    if (umc_idx != bc_idx) {
        BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, umc_idx,
                    &mc_group));
        BCM_IF_ERROR_RETURN(bcm_esw_multicast_egress_delete_all(unit, mc_group)); 
    }

    /* Delete all VPs from the UUC group, if UUC group is different
     * from BC and UMC groups.
     */
    uuc_idx = soc_mem_field32_get(unit, VLAN_TABm, &vtab, UUC_IDXf);
    if ((uuc_idx != bc_idx) && (uuc_idx != umc_idx)) {
        BCM_IF_ERROR_RETURN(_bcm_tr_multicast_ipmc_group_type_get(unit, uuc_idx,
                    &mc_group));
        BCM_IF_ERROR_RETURN(bcm_esw_multicast_egress_delete_all(unit, mc_group)); 
    }

    /* Clear the VIRTUAL_PORT_EN field since there are no virtual ports in
     * this VLAN anymore.
     */
    if (SOC_MEM_FIELD_VALID(unit, VLAN_TABm, VIRTUAL_PORT_ENf)) {
       soc_mem_field32_set(unit, VLAN_TABm, &vtab, VIRTUAL_PORT_ENf, 0);
    }
    SOC_IF_ERROR_RETURN(soc_mem_write(unit, VLAN_TABm, MEM_BLOCK_ALL, vlan,
                &vtab));

    /* Delete given VLAN's (vlan, vp) entries from egress vlan translation table */
    BCM_IF_ERROR_RETURN(_bcm_tr2_vp_untagged_delete_all(unit, vlan));

#ifdef BCM_TRIDENT2_SUPPORT
    /* Delete given VLAN's (vlan, vp) entries from membership tables */
    if (soc_feature(unit, soc_feature_ing_vp_vlan_membership)) {
        BCM_IF_ERROR_RETURN
            (bcm_td2_ing_vp_vlan_membership_delete_all(unit, vlan));
    }
    if (soc_feature(unit, soc_feature_egr_vp_vlan_membership)) {
        BCM_IF_ERROR_RETURN
            (bcm_td2_egr_vp_vlan_membership_delete_all(unit, vlan));
    }
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_TRIDENT_SUPPORT
    /* Delete VP groups from VLAN */
    if (soc_feature(unit, soc_feature_vp_group_ingress_vlan_membership)) {
        BCM_IF_ERROR_RETURN(bcm_td_ing_vp_group_vlan_delete_all(unit, vlan));
    } 
    if (soc_feature(unit, soc_feature_vp_group_egress_vlan_membership)) {
        BCM_IF_ERROR_RETURN(bcm_td_eg_vp_group_vlan_delete_all(unit, vlan));
    } 
#endif /* BCM_TRIDENT_SUPPORT */

    return BCM_E_NONE;
}

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      bcm_tr2_vlan_virtual_reinit
 * Purpose:
 *      Warm boot recovery for the VLAN virtual software module
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int 
bcm_tr2_vlan_virtual_reinit(int unit)
{
    int rv = BCM_E_NONE;
    int stable_size;
    int chunk_size, num_chunks, chunk_index;
    uint8 *vt_buf = NULL;
    uint32 *vt_entry;
    int i, entry_index_min, entry_index_max;
    uint32 key_type, vp, nh_index, trunk_bit;
    source_vp_entry_t svp_entry;
    ing_dvp_table_entry_t dvp_entry;
    ing_l3_next_hop_entry_t ing_nh_entry;
    wlan_svp_table_entry_t wlan_svp_entry;
    bcm_trunk_t tgid;
    bcm_module_t modid, mod_out;
    bcm_port_t port_num, port_out;
    uint32 profile_idx;
    ing_vlan_tag_action_profile_entry_t ing_profile_entry;
    soc_mem_t vt_mem;
    int vt_entry_size;
    soc_field_t valid_f;
    soc_field_t key_type_f;
    int key_type_ovid, key_type_ivid_ovid, key_type_otag;
    soc_field_t mpls_action_f;
    soc_field_t source_vp_f;
    soc_field_t ovid_f;
    soc_field_t ivid_f;
    soc_field_t otag_f;
    soc_field_t trunk_f;
    soc_field_t tgid_f;
    soc_field_t module_id_f;
    soc_field_t port_num_f;
    soc_field_t tag_action_profile_ptr_f;

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    /* Recover VLAN virtual ports from VLAN_XLATE table */

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        vt_mem = VLAN_XLATE_EXTDm;
        vt_entry_size = sizeof(vlan_xlate_extd_entry_t);
        valid_f = VALID_0f;
        key_type_f = KEY_TYPE_0f;
        key_type_ovid = TR3_VLXLT_X_HASH_KEY_TYPE_OVID;
        key_type_ivid_ovid = TR3_VLXLT_X_HASH_KEY_TYPE_IVID_OVID;
        key_type_otag = TR3_VLXLT_X_HASH_KEY_TYPE_OTAG;
        mpls_action_f = XLATE__MPLS_ACTIONf;
        source_vp_f = XLATE__SOURCE_VPf;
        ovid_f = XLATE__OVIDf;
        ivid_f = XLATE__IVIDf;
        otag_f = XLATE__OTAGf;
        trunk_f = XLATE__Tf;
        tgid_f = XLATE__TGIDf;
        module_id_f = XLATE__MODULE_IDf;
        port_num_f = XLATE__PORT_NUMf;
        tag_action_profile_ptr_f = XLATE__TAG_ACTION_PROFILE_PTRf; 
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        vt_mem = VLAN_XLATEm;
        vt_entry_size = sizeof(vlan_xlate_entry_t);
        valid_f = VALIDf;
        key_type_f = KEY_TYPEf;
        key_type_ovid = TR_VLXLT_HASH_KEY_TYPE_OVID;
        key_type_ivid_ovid = TR_VLXLT_HASH_KEY_TYPE_IVID_OVID;
        key_type_otag = TR_VLXLT_HASH_KEY_TYPE_OTAG;
        mpls_action_f = MPLS_ACTIONf;
        source_vp_f = SOURCE_VPf;
        ovid_f = OVIDf;
        ivid_f = IVIDf;
        otag_f = OTAGf;
        trunk_f = Tf;
        tgid_f = TGIDf;
        module_id_f = MODULE_IDf;
        port_num_f = PORT_NUMf;
        tag_action_profile_ptr_f = TAG_ACTION_PROFILE_PTRf; 
    }

    chunk_size = 1024;
    num_chunks = soc_mem_index_count(unit, vt_mem) / chunk_size;
    if (soc_mem_index_count(unit, vt_mem) % chunk_size) {
        num_chunks++;
    }

    vt_buf = soc_cm_salloc(unit, vt_entry_size * chunk_size,
            "VLAN_XLATE buffer");
    if (NULL == vt_buf) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }

    for (chunk_index = 0; chunk_index < num_chunks; chunk_index++) {

        /* Get a chunk of entries */
        entry_index_min = chunk_index * chunk_size;
        entry_index_max = entry_index_min + chunk_size - 1;
        if (entry_index_max > soc_mem_index_max(unit, vt_mem)) {
            entry_index_max = soc_mem_index_max(unit, vt_mem);
        }
        rv = soc_mem_read_range(unit, vt_mem, MEM_BLOCK_ANY,
                entry_index_min, entry_index_max, vt_buf);
        if (SOC_FAILURE(rv)) {
            goto cleanup;
        }

        /* Read each entry of the chunk */
        for (i = 0; i < (entry_index_max - entry_index_min + 1); i++) {
            vt_entry = soc_mem_table_idx_to_pointer(unit,
                    vt_mem, uint32 *, vt_buf, i);

            if (soc_mem_field32_get(unit, vt_mem, vt_entry, valid_f) == 0) {
                continue;
            }

            key_type = soc_mem_field32_get(unit, vt_mem, vt_entry, key_type_f);
            if ((key_type != key_type_ovid) &&
                (key_type != key_type_ivid_ovid) &&
                (key_type != key_type_otag)) {
                continue;
            }

            if (soc_mem_field32_get(unit, vt_mem, vt_entry, mpls_action_f) != 1) {
                continue;
            }

            vp = soc_mem_field32_get(unit, vt_mem, vt_entry, source_vp_f);
            if ((stable_size == 0) || SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit)) {
                /* Determine if VP is a VLAN VP by process of elimination.
                 * Can rule out NIV and Extender VPs since they have different
                 * VIF key types in VLAN_XLATEm.
                 */

                rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp_entry);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
                /* Rule out MPLS and MiM VPs, for which ENTRY_TYPE = 1 */
                if (soc_SOURCE_VPm_field32_get(unit, &svp_entry, ENTRY_TYPEf) != 3) {
                    continue;
                }

                rv = READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp_entry);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
                /* Rule out Trill VPs, for which VP_TYPE = 1 */
                if (soc_ING_DVP_TABLEm_field32_get(unit, &dvp_entry, VP_TYPEf) != 0) {
                    continue;
                }

                nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp_entry,
                        NEXT_HOP_INDEXf);
                rv = READ_ING_L3_NEXT_HOPm(unit, MEM_BLOCK_ANY, nh_index,
                        &ing_nh_entry);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
                /* Rule out Subport VPs, for which ENTRY_TYPE = 3 */
                if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh_entry,
                            ENTRY_TYPEf) != 2) {
                    continue;
                }

                /* Rule out WLAN vp */
                if (SOC_MEM_IS_VALID(unit, WLAN_SVP_TABLEm)) {
                    if (vp > soc_mem_index_max(unit, WLAN_SVP_TABLEm)) {
                        continue;
                    }
                    rv = READ_WLAN_SVP_TABLEm(unit, MEM_BLOCK_ANY, vp,
                            &wlan_svp_entry);
                    if (BCM_FAILURE(rv)) {
                        goto cleanup;
                    }
                    if (soc_WLAN_SVP_TABLEm_field32_get(unit, &wlan_svp_entry,
                                VALIDf) == 1) { 
                        continue;
                    }
                }

                /* At this point, we are sure VP is a VLAN VP. */

                rv = _bcm_vp_used_set(unit, vp, _bcmVpTypeVlan);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
            } else {
                if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVlan)) {
                    /* VP bitmap is recovered by virtual_init */
                    continue;
                }
            }

            /* Recover VLAN_VP_INFO(unit, vp)->criteria, match_vlan, and
             * match_inner_vlan.
             */
            if (key_type == key_type_ovid) {
                VLAN_VP_INFO(unit, vp)->criteria =
                    BCM_VLAN_PORT_MATCH_PORT_VLAN;
                VLAN_VP_INFO(unit, vp)->match_vlan = 
                    soc_mem_field32_get(unit, vt_mem, vt_entry, ovid_f);
            } else if (key_type == key_type_ivid_ovid) {
                VLAN_VP_INFO(unit, vp)->criteria =
                    BCM_VLAN_PORT_MATCH_PORT_VLAN_STACKED;
                VLAN_VP_INFO(unit, vp)->match_vlan = 
                    soc_mem_field32_get(unit, vt_mem, vt_entry, ovid_f);
                VLAN_VP_INFO(unit, vp)->match_inner_vlan = 
                    soc_mem_field32_get(unit, vt_mem, vt_entry, ivid_f);
            } else if (key_type == key_type_otag) {
                VLAN_VP_INFO(unit, vp)->criteria =
                    BCM_VLAN_PORT_MATCH_PORT_VLAN16;
                VLAN_VP_INFO(unit, vp)->match_vlan = 
                    soc_mem_field32_get(unit, vt_mem, vt_entry, otag_f);
            } else {
                continue;
            }

            /* Recover VLAN_VP_INFO(unit, vp)->port */
            trunk_bit = soc_mem_field32_get(unit, vt_mem, vt_entry, trunk_f);
            if (trunk_bit) {
                tgid = soc_mem_field32_get(unit, vt_mem, vt_entry, tgid_f);
                BCM_GPORT_TRUNK_SET(VLAN_VP_INFO(unit, vp)->port, tgid);
            } else {
                modid = soc_mem_field32_get(unit, vt_mem, vt_entry, module_id_f);
                port_num = soc_mem_field32_get(unit, vt_mem, vt_entry, port_num_f);
                rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                        modid, port_num, &mod_out, &port_out);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
                BCM_GPORT_MODPORT_SET(VLAN_VP_INFO(unit, vp)->port,
                        mod_out, port_out);
            }

            /* Recover VLAN_VP_INFO(unit, vp)->flags */
            profile_idx = soc_mem_field32_get(unit, vt_mem, vt_entry,
                    tag_action_profile_ptr_f);
            rv = READ_ING_VLAN_TAG_ACTION_PROFILEm(unit, MEM_BLOCK_ANY,
                    profile_idx, &ing_profile_entry);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
            if ((soc_ING_VLAN_TAG_ACTION_PROFILEm_field32_get(unit,
                     &ing_profile_entry, DT_OTAG_ACTIONf) ==
                 bcmVlanActionReplace) &&
                (soc_ING_VLAN_TAG_ACTION_PROFILEm_field32_get(unit,
                     &ing_profile_entry, DT_ITAG_ACTIONf) ==
                 bcmVlanActionNone)) {
                VLAN_VP_INFO(unit, vp)->flags |= BCM_VLAN_PORT_INNER_VLAN_PRESERVE;
            }
            if ((soc_ING_VLAN_TAG_ACTION_PROFILEm_field32_get(unit,
                     &ing_profile_entry, SOT_OTAG_ACTIONf) ==
                 bcmVlanActionReplace) &&
                (soc_ING_VLAN_TAG_ACTION_PROFILEm_field32_get(unit,
                     &ing_profile_entry, SOT_ITAG_ACTIONf) ==
                 bcmVlanActionAdd)) {
                VLAN_VP_INFO(unit, vp)->flags |= BCM_VLAN_PORT_INNER_VLAN_ADD;
            }

            if (VLAN_VP_INFO(unit, vp)->criteria ==
                    BCM_VLAN_PORT_MATCH_PORT_VLAN16) {
                VLAN_VP_INFO(unit, vp)->flags |= BCM_VLAN_PORT_EGRESS_VLAN16;
            }

            if (stable_size == 0) {
                /* In the Port module, a port's VP count is not recovered in 
                 * level 1 Warm Boot.
                 */
                rv = _bcm_tr2_vlan_vp_port_cnt_update(unit,
                        VLAN_VP_INFO(unit, vp)->port, vp, TRUE);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
            }
        }
    }

cleanup:
    if (vt_buf) {
        soc_cm_sfree(unit, vt_buf);
    }

    if (BCM_FAILURE(rv)) {
        _bcm_tr2_vlan_virtual_free_resources(unit);
    }

    return rv;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_tr2_vlan_vp_sw_dump
 * Purpose:
 *     Displays VLAN VP information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
bcm_tr2_vlan_vp_sw_dump(int unit)
{
    int i, num_vp;

    soc_cm_print("\nSW Information VLAN VP - Unit %d\n", unit);

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    for (i = 0; i < num_vp; i++) {
        if (VLAN_VP_INFO(unit, i)->port == 0) {
            continue;
        }
        soc_cm_print("\n  VLAN vp = %d\n", i);
        soc_cm_print("  Criteria = 0x%x,", VLAN_VP_INFO(unit, i)->criteria);
        switch (VLAN_VP_INFO(unit, i)->criteria) {
            case BCM_VLAN_PORT_MATCH_PORT_VLAN:
                soc_cm_print(" port plus outer VLAN ID\n");
                break;
            case BCM_VLAN_PORT_MATCH_PORT_VLAN_STACKED:
                soc_cm_print(" port plus outer and inner VLAN IDs\n");
                break;
            case BCM_VLAN_PORT_MATCH_PORT_VLAN16:
                soc_cm_print(" port plus outer VLAN tag\n");
                break;
            default:
                soc_cm_print(" \n");
        }
        soc_cm_print("  Flags = 0x%x\n", VLAN_VP_INFO(unit, i)->flags);
        soc_cm_print("  Match VLAN = 0x%x\n", VLAN_VP_INFO(unit, i)->match_vlan);
        soc_cm_print("  Match Inner VLAN = 0x%x\n",
                VLAN_VP_INFO(unit, i)->match_inner_vlan);
        soc_cm_print("  Port = 0x%x\n", VLAN_VP_INFO(unit, i)->port);
    }

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif /* BCM_TRIUMPH2_SUPPORT && INCLUDE_L3 */
