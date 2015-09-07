/*
 * $Id: vlan.c,v 1.186 Broadcom SDK $
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
 * File:        vlan.c
 * Purpose:     Provide low-level access to Triumph VLAN resources
 */
#include <sal/core/boot.h>

#include <soc/defs.h>
#if defined(BCM_TRX_SUPPORT)
#include <soc/drv.h>
#include <soc/hash.h>
#include <soc/mem.h>
#include <soc/profile_mem.h>
#include <shared/bsl.h>
#include <bcm/error.h>
#include <bcm/vlan.h>
#include <bcm/stg.h>
#include <bcm/port.h>
#include <bcm/trunk.h>
#include <bcm/stack.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/vlan.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/xgs3.h>
#include <bcm_int/esw/trx.h>
#if defined(BCM_TRIUMPH2_SUPPORT)
#include <bcm_int/esw/triumph2.h>
#endif /* BCM_TRIUMPH2_SUPPORT */

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
#include <bcm_int/esw/policer.h>
#endif
#include <bcm_int/esw_dispatch.h>

#if defined(BCM_TRIDENT2_SUPPORT)
#include <bcm/fcoe.h>
#include <bcm_int/esw/trident2.h>
#endif

#if defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw/katana2.h>
#endif

/* Cache of Ingress Vlan Translate Action Profile Table */
static soc_profile_mem_t *ing_action_profile[BCM_MAX_NUM_UNITS] = {NULL};
#define ING_ACTION_PROFILE_DEFAULT  0

/* Cache of Engress Vlan Translate Action Profile Table */
static soc_profile_mem_t *egr_action_profile[BCM_MAX_NUM_UNITS] = {NULL};
#define EGR_ACTION_PROFILE_DEFAULT  0

/* Cache of Ingress Vlan Range Profile Table */
static soc_profile_mem_t *vlan_range_profile[BCM_MAX_NUM_UNITS] = {NULL};
#define VLAN_RANGE_PROFILE_DEFAULT  0

typedef struct _bcm_trx_vlan_subnet_entry_s {
    bcm_ip6_t   ip;
    bcm_ip6_t   mask;
    int         prefix;
    bcm_vlan_t  ovid;
    bcm_vlan_t  ivid;
    uint8       opri;
    uint8       ocfi;
    uint8       ipri;
    uint8       icfi;
    int         profile_idx;
}_bcm_trx_vlan_subnet_entry_t;

#if defined(BCM_TRIUMPH3_SUPPORT)
typedef int (*tr3_vxlate_extd_entry_update_func_t)(int unit,
                   vlan_xlate_entry_t *vent_in,
                   void *ctxt,
                   vlan_xlate_entry_t *vent_out,
                   vlan_xlate_extd_entry_t *vxent_out,
                   int *use_extd_tbl);
#endif

STATIC int _bcm_trx_vlan_range_profile_init(int unit);

#ifdef BCM_WARM_BOOT_SUPPORT
STATIC int _bcm_trx_vlan_port_protocol_action_reinit(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#if defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function : 
 *   _bcm_tr3_vxlate_extd2vxlate
 *
 * Purpose  : 
 *   convert extended vlan translation table to regular vlan translation
 *   table for all applicable fields.  
 *
 * Parameters:
 *   vxxent  -  (IN) extended vlan translation table 
 *   vxent   -  (OUT) regular vlan translation table
 *   use_svp -  (IN) Is SVP used in vxxent
 */

int
_bcm_tr3_vxlate_extd2vxlate(int unit,
                vlan_xlate_extd_entry_t *vxxent,
                vlan_xlate_entry_t *vxent,
                int use_svp)
{
    uint32 key[2];
    uint32 value;

    value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, VALID_0f);
    soc_mem_field32_set(unit, VLAN_XLATEm, vxent, VALIDf,value);

    soc_mem_field_get(unit, VLAN_XLATE_EXTDm, 
                      (uint32 *)vxxent, XLATE__KEY_0f, key);
    soc_mem_field_set(unit, VLAN_XLATEm,
                    (uint32 *)vxent,  KEYf, key);

    /* key type value for VLAN_XLATE_EXTDm: VLAN_XLATE_1 value + 1 */
    value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, KEY_TYPE_0f);
    value--;
    soc_VLAN_XLATEm_field32_set(unit, vxent, KEY_TYPEf,value);

    value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, 
                    XLATE__TAG_ACTION_PROFILE_PTRf);
    soc_VLAN_XLATEm_field32_set(unit, vxent, 
                               TAG_ACTION_PROFILE_PTRf,value);
    value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, 
                    XLATE__NEW_OVIDf);
    soc_VLAN_XLATEm_field32_set(unit, vxent, 
                               NEW_OVIDf,value);
    value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, 
                    XLATE__NEW_OPRIf);
    soc_VLAN_XLATEm_field32_set(unit, vxent, 
                               NEW_OPRIf,value);
    value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, 
                    XLATE__NEW_OCFIf);
    soc_VLAN_XLATEm_field32_set(unit, vxent, 
                               NEW_OCFIf,value);

    /* VLAN_XLATE source_vp and new_ivid are overlay fields while
     * VLAN_XLATE_EXTD's are not
     */
    if (use_svp) {
        value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, 
                        XLATE__SOURCE_VPf);
        soc_VLAN_XLATEm_field32_set(unit, vxent, SOURCE_VPf,value);
        soc_VLAN_XLATEm_field32_set(unit, vxent, SVP_VALIDf,1);
    } else {
        soc_VLAN_XLATEm_field32_set(unit, vxent, SVP_VALIDf,0);
        value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, 
                        XLATE__NEW_IVIDf);
        soc_VLAN_XLATEm_field32_set(unit, vxent, 
                                   NEW_IVIDf,value);
        value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, 
                        XLATE__NEW_IPRIf);
        soc_VLAN_XLATEm_field32_set(unit, vxent, 
                                   NEW_IPRIf,value);
        value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, 
                        XLATE__NEW_ICFIf);
        soc_VLAN_XLATEm_field32_set(unit, vxent, 
                                   NEW_ICFIf,value);
    }

    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm,
                            ESM_SEARCH_OFFSETf)) {
        value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, 
                        XLATE__ESM_SEARCH_OFFSETf);
        soc_VLAN_XLATEm_field32_set(unit, vxent, 
                                   ESM_SEARCH_OFFSETf,value);
    }
    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm,
                            ESM_SEARCH_PRIORITYf)) {
        value = soc_VLAN_XLATE_EXTDm_field32_get(unit, vxxent, 
                        XLATE__ESM_SEARCH_PRIORITYf);
        soc_VLAN_XLATEm_field32_set(unit, vxent, 
                                   ESM_SEARCH_PRIORITYf,value);
    }

    return BCM_E_NONE;    
}

/*
 * Function :
 *   _bcm_tr3_vxlate2vxlate_extd
 *
 * Purpose  :
 *   convert the regular vlan translation table to the extended vlan translation
 *   table for all applicable fields.
 *
 * Parameters:
 *   vxent   -  (IN) regular vlan translation table
 *   vxxent  -  (OUT) extended vlan translation table
 */

int
_bcm_tr3_vxlate2vxlate_extd(int unit,
                vlan_xlate_entry_t *vxent,
                vlan_xlate_extd_entry_t *vxxent)
{
    uint32 key[2];
    uint32 value;
    int svp_valid;

    value = soc_mem_field32_get(unit, VLAN_XLATEm, vxent, VALIDf);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, VALID_0f, value);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, VALID_1f, value);

    soc_mem_field_get(unit, VLAN_XLATEm,
                    (uint32 *)vxent,  KEYf, key);
    soc_mem_field_set(unit, VLAN_XLATE_EXTDm, 
                      (uint32 *)vxxent, XLATE__KEY_0f, key);

    value = soc_VLAN_XLATEm_field32_get(unit, vxent, KEY_TYPEf);

    /* key type value for VLAN_XLATE_EXTDm: VLAN_XLATE_1 value + 1 */
    value++;    
    soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, KEY_TYPE_0f, value);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, KEY_TYPE_1f, value);

    value = soc_VLAN_XLATEm_field32_get(unit, vxent, 
                                   TAG_ACTION_PROFILE_PTRf);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, 
                        XLATE__TAG_ACTION_PROFILE_PTRf, value);
    value = soc_VLAN_XLATEm_field32_get(unit, vxent, 
                                   NEW_OVIDf);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, 
                        XLATE__NEW_OVIDf, value);
    value = soc_VLAN_XLATEm_field32_get(unit, vxent, 
                                   NEW_OPRIf);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, 
                        XLATE__NEW_OPRIf, value);
    value = soc_VLAN_XLATEm_field32_get(unit, vxent, 
                                   NEW_OCFIf);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, 
                        XLATE__NEW_OCFIf, value);
    svp_valid = soc_VLAN_XLATEm_field32_get(unit, vxent, 
                                   SVP_VALIDf);
    if (svp_valid) {
        value = soc_VLAN_XLATEm_field32_get(unit, vxent, 
                                   SOURCE_VPf);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, 
                            XLATE__SOURCE_VPf, value);
    } else {
        value = soc_VLAN_XLATEm_field32_get(unit, vxent, 
                                   NEW_IVIDf);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, 
                        XLATE__NEW_IVIDf, value);
        value = soc_VLAN_XLATEm_field32_get(unit, vxent, 
                                   NEW_IPRIf);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, 
                        XLATE__NEW_IPRIf, value);
        value = soc_VLAN_XLATEm_field32_get(unit, vxent, 
                                   NEW_ICFIf);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, 
                        XLATE__NEW_ICFIf, value);
    }

    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm,
                                    ESM_SEARCH_OFFSETf)) {
        value = soc_VLAN_XLATEm_field32_get(unit, vxent, 
                                   ESM_SEARCH_OFFSETf);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, 
                            XLATE__ESM_SEARCH_OFFSETf, value);
    }
    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm,
                                    ESM_SEARCH_PRIORITYf)) {
        value = soc_VLAN_XLATEm_field32_get(unit, vxent, 
                                   ESM_SEARCH_PRIORITYf);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, vxxent, 
                            XLATE__ESM_SEARCH_PRIORITYf, value);
    }
    return BCM_E_NONE;    
}


/*
 * Function :
 *   _bcm_tr3_vxlate_entry_add
 *
 * Purpose  :
 *   Add or update a vlan translation entry to the appropriate table. 
 *   BCM56640 has two type of vlan translation tables, a regular sized 
 *   table VLAN_XLATE and a extended sized table VLAN_XLATE_EXTD. This
 *   function takes a regular sized entry as input, add or update 
 *   either in regular sized table or extended sized table if fields 
 *   only in extended table are used.   
 *
 * Parameters:
 *  vent   -    the regular sized entry to be added/updated
 *  ctxt   -    context used by update_func 
 *  update_func - function determines whether to use regular or extd entry
 */                   
int
_bcm_tr3_vxlate_entry_add(int unit,
                vlan_xlate_entry_t *vent,
                void *ctxt,
                tr3_vxlate_extd_entry_update_func_t update_func)
{
    vlan_xlate_entry_t vent_old;
    vlan_xlate_extd_entry_t vxent;
    vlan_xlate_extd_entry_t vxent_old;
    vlan_xlate_extd_entry_t vxent_tmp;
    int idx;
    int use_extd_table = 0;
    int rv;
 
    sal_memset(&vent_old, 0, sizeof(vent_old));
    sal_memset(&vxent, 0, sizeof(vxent));
    sal_memset(&vxent_old, 0, sizeof(vxent_old));

    /*
     * first searches the regular entry table VLAN_XLATEm, if a match is found,
     * update and write it back if the entry can be added to this table. 
     * If not, delete this entry and add it to VLAN_XLATE_EXTDm. If no match is
     * found, check VLAN_XLATE_EXTDm, if a match is found, update and write it
     * back, if not, add it to either VLAN_XLATEm or VLAN_XLATE_EXTDm 
     * appropriately. 
     */ 
    
    /* look up existing entry */
    soc_mem_lock(unit, VLAN_XLATEm);
    rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &idx, vent, 
                        &vent_old, 0);
    soc_mem_unlock(unit, VLAN_XLATEm);

    /* regular entry exist */
    if (rv == SOC_E_NONE) {

        if (update_func) {
            BCM_IF_ERROR_RETURN
                (update_func(unit,vent,ctxt,&vent_old,&vxent_old,
                      &use_extd_table));

            if (use_extd_table) {
                /* remove the existing entry */
                soc_mem_lock(unit, VLAN_XLATEm);
                rv = (soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ANY, 
                                     &vent_old));
                soc_mem_unlock(unit, VLAN_XLATEm);

                if (rv != SOC_E_NONE) {
                    return rv;
                }
 
                /* search for the extd entry. It should not exist because
                 * we already found a regular one
                 */
                soc_mem_lock(unit, VLAN_XLATE_EXTDm);
                rv = soc_mem_search(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL,
                             &idx, &vxent, &vxent_tmp, 0);
                soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
    
                if (rv == SOC_E_NONE) {
                    return BCM_E_EXISTS;  /* XXX ERROR message */
                } else if (rv != SOC_E_NOT_FOUND) {
                    return rv;
                } else { 
                    soc_mem_lock(unit, VLAN_XLATE_EXTDm);

                    /* no extry exists, insert */
                    rv = soc_mem_insert(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL, 
                                    &vxent_old);    
                    soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
                    return rv;
                }
            }  /* use_extd_table */
        } /*  update_func */ 

        /* regular entry */
        soc_mem_lock(unit, VLAN_XLATEm);
        rv = soc_mem_write(unit, VLAN_XLATEm,  MEM_BLOCK_ALL, 
                    idx, update_func == NULL? vent: &vent_old);
        soc_mem_unlock(unit, VLAN_XLATEm);
        return BCM_E_NONE;
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    } else {  /* no regular entry found */
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vxlate2vxlate_extd(unit,vent,&vxent));
        soc_mem_lock(unit, VLAN_XLATE_EXTDm);
        rv = soc_mem_search(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL, &idx,
                            &vxent, &vxent_old, 0);
        soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
        if (rv == SOC_E_NONE) {
            /* found extd entry */

            if (update_func) {
                /* need update the entry before writing it back */
                BCM_IF_ERROR_RETURN
                    (update_func(unit,vent,ctxt,NULL,&vxent_old,
                                      &use_extd_table));
            }
            soc_mem_lock(unit, VLAN_XLATE_EXTDm);
            rv = soc_mem_write(unit, VLAN_XLATE_EXTDm,
                          MEM_BLOCK_ALL, idx, &vxent_old);
            soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
        } else if (rv != SOC_E_NOT_FOUND) {
            return rv;
        } else {
            /* both regular and extd table are not found */
            /* need update the entry before insert */
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_vxlate2vxlate_extd(unit,vent,&vxent));
            if (update_func) {
                BCM_IF_ERROR_RETURN
                    (update_func(unit,vent,ctxt,NULL,&vxent,
                                      &use_extd_table));
            }

            if (use_extd_table) {
                soc_mem_lock(unit, VLAN_XLATE_EXTDm);
                rv = soc_mem_insert(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL, 
                                &vxent);    
                soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
            } else {
                soc_mem_lock(unit, VLAN_XLATEm);
                rv = soc_mem_insert(unit, VLAN_XLATEm, MEM_BLOCK_ALL, 
                                vent);    
                soc_mem_unlock(unit, VLAN_XLATEm);
            }
        }
    }
    return rv;    
}
#endif /* BCM_TRIUMPH3_SUPPORT */

/*
 * Function : _bcm_trx_vlan_action_verify
 *
 * Purpose  : to validate all members of action structure
 *
 */
STATIC int 
_bcm_trx_vlan_action_verify(int unit, bcm_vlan_action_set_t *action)
{
    if (NULL == action) {
        return BCM_E_PARAM;
    }
    VLAN_CHK_ID(unit, action->new_inner_vlan);
    VLAN_CHK_ID(unit, action->new_outer_vlan);

    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        if (action->priority < -1 || action->priority > 7 ||
            action->new_inner_pkt_prio > 7 ||
            action->new_outer_cfi > 1 || action->new_inner_cfi > 1) {
            return BCM_E_PARAM;
        }
    }

    switch (action->dt_outer) {
    case bcmVlanActionNone:
    case bcmVlanActionAdd:
    case bcmVlanActionReplace:
    case bcmVlanActionDelete:
        break;
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_copy_action)) {
            return BCM_E_UNAVAIL;
        }
        break;

#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->dt_outer_prio) {
    case bcmVlanActionNone:
    case bcmVlanActionAdd:
    case bcmVlanActionReplace:
    case bcmVlanActionDelete:
        break;
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_copy_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->dt_outer_pkt_prio) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionAdd:
    case bcmVlanActionReplace:
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->dt_outer_cfi) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionAdd:
    case bcmVlanActionReplace:
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
    default:
        return BCM_E_PARAM;
    }

    switch (action->dt_inner) {
    case bcmVlanActionNone:
    case bcmVlanActionReplace:
    case bcmVlanActionDelete:
        break;
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_copy_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->dt_inner_prio) {
    case bcmVlanActionNone:
    case bcmVlanActionReplace:
    case bcmVlanActionDelete:
        break;
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_copy_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->dt_inner_pkt_prio) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionReplace:
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->dt_inner_cfi) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionReplace:
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
    default:
        return BCM_E_PARAM;
    }

    switch (action->ot_outer) {
    case bcmVlanActionNone:
    case bcmVlanActionAdd:
    case bcmVlanActionReplace:
    case bcmVlanActionDelete:
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->ot_outer_prio) {
    case bcmVlanActionNone:
    case bcmVlanActionAdd:
    case bcmVlanActionReplace:
    case bcmVlanActionDelete:
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->ot_outer_pkt_prio) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionReplace:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->ot_outer_cfi) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionReplace:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
    default:
        return BCM_E_PARAM;
    }

    switch (action->ot_inner) {
    case bcmVlanActionNone:
    case bcmVlanActionAdd:
        break;
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_copy_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->ot_inner_pkt_prio) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionAdd:
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->ot_inner_cfi) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionAdd:
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
    default:
        return BCM_E_PARAM;
    }

    switch (action->it_outer) {
    case bcmVlanActionNone:
    case bcmVlanActionAdd:
        break;
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_copy_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->it_outer_pkt_prio) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionAdd:
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->it_outer_cfi) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionAdd:
    case bcmVlanActionCopy:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
    default:
        return BCM_E_PARAM;
    }

    switch (action->it_inner) {
    case bcmVlanActionNone:
    case bcmVlanActionReplace:
    case bcmVlanActionDelete:
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->it_inner_prio) {
    case bcmVlanActionNone:
    case bcmVlanActionReplace:
    case bcmVlanActionDelete:
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->it_inner_pkt_prio) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionReplace:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->it_inner_cfi) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionReplace:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
    default:
        return BCM_E_PARAM;
    }

    switch (action->ut_outer) {
    case bcmVlanActionNone:
    case bcmVlanActionAdd:
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->ut_outer_pkt_prio) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionAdd:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->ut_outer_cfi) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionAdd:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
    default:
        return BCM_E_PARAM;
    }

    switch (action->ut_inner) {
    case bcmVlanActionNone:
    case bcmVlanActionAdd:
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->ut_inner_pkt_prio) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionAdd:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
#if defined(BCM_TRIDENT2_SUPPORT)
    case bcmVlanActionCopyVsan:
    case bcmVlanActionReplaceVsan:
        if (!soc_feature(unit, soc_feature_fcoe)) {
            return BCM_E_UNAVAIL;
        }
        break;
#endif
    default:
        return BCM_E_PARAM;
    }

    switch (action->ut_inner_cfi) {
    case bcmVlanActionNone:
        break;
    case bcmVlanActionAdd:
        if (!soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            return BCM_E_UNAVAIL;
        }
        break;
    default:
        return BCM_E_PARAM;
    }
    
    return BCM_E_NONE;
}

/*
 * Function : _bcm_trx_egr_vlan_action_verify
 *
 * Purpose  : to validate all members of action structure for egress vlan 
 *            more strict validation than ingress  
 *
 */
STATIC int 
_bcm_trx_egr_vlan_action_verify(int unit, bcm_vlan_action_set_t *action)
{

    BCM_IF_ERROR_RETURN(
        _bcm_trx_vlan_action_verify(unit, action));

    if (action->dt_outer == bcmVlanActionAdd ||
        action->dt_outer_prio == bcmVlanActionAdd ||
        action->ot_outer == bcmVlanActionAdd ||
        action->ot_outer_prio == bcmVlanActionAdd) {
        return BCM_E_PARAM;
    }

    /* Single inner tag modification is available for egress vlan actions */
    if (!soc_feature(unit, soc_feature_vlan_egr_it_inner_replace)) {
        if (action->dt_outer_prio != bcmVlanActionNone) {
            return BCM_E_PARAM;
        }
        if (action->ot_outer_prio != bcmVlanActionNone) {
            return BCM_E_PARAM;
        }
        if (action->it_outer != bcmVlanActionNone) {
            return BCM_E_PARAM;
        }
        if (action->it_inner != bcmVlanActionNone) {
            return BCM_E_PARAM;
        }
        if (action->it_inner_prio != bcmVlanActionNone) {
            return BCM_E_PARAM;
        }
        if (action->ut_inner != bcmVlanActionNone) {
            return BCM_E_PARAM;
        }
        if (action->ut_outer != bcmVlanActionNone) {
            return BCM_E_PARAM;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function : _bcm_trx_vlan_ip_verify
 *
 * Purpose  : to validate members of vlan_ip structure
 *
 */
STATIC int 
_bcm_trx_vlan_ip_verify(int unit, bcm_vlan_ip_t *vlan_ip)
{
    if (NULL == vlan_ip) {
        return BCM_E_PARAM;
    }
    VLAN_CHK_ID(unit, vlan_ip->vid);
    VLAN_CHK_PRIO(unit, vlan_ip->prio);

    return BCM_E_NONE;
}

/*
 * Function : bcm_trx_vlan_action_profile_detach
 *
 * Purpose  : to initialize hardware ING_VLAN_TAG_ACTION_PROFILE and
 *            EGR_VLAN_TAG_ACTION_PROFILE tables and allocate memory
 *            to cache hardware tables in RAM.
 *
 * Note:
 *      Allocate memory to cache the profile tables and initialize.
 *      If memory to cache the profile table is already allocated, just
 *      initialize the table.
 */
int
_bcm_trx_vlan_action_profile_detach(int unit)
{
    int rv;

    /* De-initialize the ING_VLAN_TAG_ACTION_PROFILE table */
    rv = soc_profile_mem_destroy(unit, ing_action_profile[unit]);
    BCM_IF_ERROR_RETURN(rv);

    sal_free(ing_action_profile[unit]);
    ing_action_profile[unit] = NULL;

    rv = soc_profile_mem_destroy(unit, egr_action_profile[unit]);
    BCM_IF_ERROR_RETURN(rv);

    sal_free(egr_action_profile[unit]);
    egr_action_profile[unit] = NULL;

    rv = soc_profile_mem_destroy(unit, vlan_range_profile[unit]);
    BCM_IF_ERROR_RETURN(rv);

    sal_free(vlan_range_profile[unit]);
    vlan_range_profile[unit] = NULL;    

    return BCM_E_NONE;
}

/*
 * Function : bcm_trx_vlan_action_profile_init
 *
 * Purpose  : to initialize hardware ING_VLAN_TAG_ACTION_PROFILE and
 *            EGR_VLAN_TAG_ACTION_PROFILE tables and allocate memory
 *            to cache hardware tables in RAM.
 *
 * Note:
 *      Allocate memory to cache the profile tables and initialize.
 *      If memory to cache the profile table is already allocated, just
 *      initialize the table.
 */
int
_bcm_trx_vlan_action_profile_init(int unit)
{
    int i, idx, num_ports;
    port_tab_entry_t port_entry;
    vlan_protocol_data_entry_t proto_entry;
    vlan_subnet_entry_t subnet_entry;
    vlan_xlate_entry_t xlate_entry;
    egr_vlan_xlate_entry_t egr_xlate_entry;
    ing_vlan_tag_action_profile_entry_t ing_profile_entry;
    egr_vlan_tag_action_profile_entry_t egr_profile_entry;
    uint32 rval, temp_index;
    soc_mem_t mem;
    int entry_words;
    void *entries[1];
    int key_type_value, key_type;
#if defined(BCM_TRIUMPH2_SUPPORT)
    _bcm_flex_stat_handle_t handle;
    vlan_xlate_entry_t vent;
    egr_vlan_xlate_entry_t event;
    uint32 key[2];
    int fs_idx;
#endif

    /* Initialize the ING_VLAN_TAG_ACTION_PROFILE table */

    if (ing_action_profile[unit] == NULL) {
        ing_action_profile[unit] = sal_alloc(sizeof(soc_profile_mem_t),
                                             "Ing Action Profile Mem");
        if (ing_action_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(ing_action_profile[unit]);
    }

    /* Create profile table cache (or re-init if it already exists) */
    mem = ING_VLAN_TAG_ACTION_PROFILEm;
    entry_words = sizeof(ing_vlan_tag_action_profile_entry_t) / sizeof(uint32);
    SOC_IF_ERROR_RETURN(soc_profile_mem_create(unit, &mem, &entry_words, 1,
                                               ing_action_profile[unit]));

    if (SOC_WARM_BOOT(unit)) {
        /* Increment the ref count for all ports */
        for (i = 0; i < soc_mem_index_count(unit, PORT_TABm); i++) {
            SOC_IF_ERROR_RETURN
                (READ_PORT_TABm(unit, MEM_BLOCK_ANY, i, &port_entry));
            idx = soc_mem_field32_get(unit, PORT_TABm,
                                      &port_entry, TAG_ACTION_PROFILE_PTRf);
            SOC_PROFILE_MEM_REFERENCE(unit, ing_action_profile[unit], idx, 1);
            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, ing_action_profile[unit], idx, 1);
        }

        /* Increment the ref count for all Protocol-based VLANs */
        for (i = 0; i < soc_mem_index_count(unit, VLAN_PROTOCOL_DATAm); i++) {
            SOC_IF_ERROR_RETURN
                (READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY, i, &proto_entry));
            idx = soc_mem_field32_get(unit, VLAN_PROTOCOL_DATAm,
                                      &proto_entry, TAG_ACTION_PROFILE_PTRf);
            SOC_PROFILE_MEM_REFERENCE(unit, ing_action_profile[unit], idx, 1);
            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, ing_action_profile[unit], idx, 1);
        }

        /* Increment the ref count for all Subnet-based VLANs */
        for (i = 0; i < soc_mem_index_count(unit, VLAN_SUBNETm); i++) {
            SOC_IF_ERROR_RETURN
                (READ_VLAN_SUBNETm(unit, MEM_BLOCK_ANY, i, &subnet_entry));
            if (!soc_mem_field32_get(unit, VLAN_SUBNETm, 
                                     &subnet_entry, VALIDf)) {
                continue;
            }
            idx = soc_mem_field32_get(unit, VLAN_SUBNETm,
                                      &subnet_entry, TAG_ACTION_PROFILE_PTRf);
            SOC_PROFILE_MEM_REFERENCE(unit, ing_action_profile[unit], idx, 1);
            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, ing_action_profile[unit], idx, 1);
        }

        /* Increment the ref count for all Mac-based VLANs & vlan translations */
        for (i = 0; i < soc_mem_index_count(unit, VLAN_XLATEm); i++) {
            SOC_IF_ERROR_RETURN
                (READ_VLAN_XLATEm(unit, MEM_BLOCK_ANY, i, &xlate_entry));
            if (!soc_mem_field32_get(unit, VLAN_XLATEm, &xlate_entry, VALIDf)) {
                continue;
            }
            key_type_value = soc_mem_field32_get(unit, VLAN_XLATEm,
                    &xlate_entry, KEY_TYPEf);

            /* Here return value is not being checked (that is intentional); in case
               it returns BCM_E_UNAVAIL, will proceed with else condition below */
            /* Coverity[unchecked_value] */
            _bcm_esw_vlan_xlate_key_type_get(unit, key_type_value, &key_type);
            if (key_type == VLXLT_HASH_KEY_TYPE_VIF ||
                key_type == VLXLT_HASH_KEY_TYPE_VIF_VLAN ||
                key_type == VLXLT_HASH_KEY_TYPE_VIF_CVLAN ||
                key_type == VLXLT_HASH_KEY_TYPE_VIF_OTAG ||
                key_type == VLXLT_HASH_KEY_TYPE_VIF_ITAG) {
                idx = soc_mem_field32_get(unit, VLAN_XLATEm,
                        &xlate_entry, VIF__TAG_ACTION_PROFILE_PTRf);
            } else {
                idx = soc_mem_field32_get(unit, VLAN_XLATEm,
                        &xlate_entry, TAG_ACTION_PROFILE_PTRf);
            }
            SOC_PROFILE_MEM_REFERENCE(unit, ing_action_profile[unit], idx, 1);
            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, ing_action_profile[unit], idx, 1);

#if defined(BCM_TRIUMPH2_SUPPORT)
            if (soc_feature(unit, soc_feature_gport_service_counters)) {
                fs_idx =
                    soc_VLAN_XLATEm_field32_get(unit, &xlate_entry,
                                                VINTF_CTR_IDXf);
                if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm,
                                        USE_VINTF_CTR_IDXf)) {
                    if (0 == soc_mem_field32_get(unit, VLAN_XLATEm,
                                     &xlate_entry, USE_VINTF_CTR_IDXf)) {
                        fs_idx = 0;
                    }
                }
                if (0 != fs_idx) { /* Recover flex stat counter */
                    sal_memset(&vent, 0, sizeof(vent));
                    /* Construct key-only entry, copy to FS handle */
                    soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                             soc_VLAN_XLATEm_field32_get(unit, &xlate_entry,
                                                         KEY_TYPEf));
                    soc_mem_field_get(unit, VLAN_XLATEm,
                                      (uint32 *) &xlate_entry,
                                      KEYf, (uint32 *) key);
                    soc_mem_field_set(unit, VLAN_XLATEm, (uint32 *) &vent,
                                      KEYf, (uint32 *) key);

                    _BCM_FLEX_STAT_HANDLE_CLEAR(handle);
                    _BCM_FLEX_STAT_HANDLE_COPY(handle, vent);
                    _bcm_esw_flex_stat_ext_reinit_add(unit,
                                  _bcmFlexStatTypeVxlt, fs_idx, handle);
                }
            }
#endif
        }

        /* One extra increment to preserve location
         * ING_ACTION_PROFILE_DEFAULT */
        SOC_PROFILE_MEM_REFERENCE(unit, ing_action_profile[unit], 
                              ING_ACTION_PROFILE_DEFAULT, 1);

    } else {
        /* Initialize the ING_ACTION_PROFILE_DEFAULT. For untagged and
         * inner-tagged packets, always add an outer tag.
         */
        sal_memset(&ing_profile_entry, 0, sizeof(ing_vlan_tag_action_profile_entry_t));
        soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm,
                            &ing_profile_entry, UT_OTAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(bcmVlanActionAdd));
        if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm,
                                &ing_profile_entry, UT_OPRI_ACTIONf,
                                _BCM_TRX_PRI_CFI_ACTION_ENCODE(bcmVlanActionAdd));
            soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm,
                                &ing_profile_entry, UT_OCFI_ACTIONf,
                                _BCM_TRX_PRI_CFI_ACTION_ENCODE(bcmVlanActionAdd));
        }
        soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm,
                            &ing_profile_entry, SIT_OTAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(bcmVlanActionAdd));
        if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm,
                                &ing_profile_entry, SIT_OPRI_ACTIONf,
                                _BCM_TRX_PRI_CFI_ACTION_ENCODE(bcmVlanActionAdd));
            soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm,
                                &ing_profile_entry, SIT_OCFI_ACTIONf,
                                _BCM_TRX_PRI_CFI_ACTION_ENCODE(bcmVlanActionAdd));
        }
        soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm,
                            &ing_profile_entry, SIT_PITAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(bcmVlanActionNone));
        soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm,
                            &ing_profile_entry, SOT_POTAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(bcmVlanActionReplace));
        soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm,
                            &ing_profile_entry, DT_POTAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(bcmVlanActionReplace));
        entries[0] = &ing_profile_entry;
        SOC_IF_ERROR_RETURN
            (soc_profile_mem_add(unit, ing_action_profile[unit],
                                 (void *) &entries, 1, &temp_index));

        /* Increment the ref count for all ports */
        SOC_PROFILE_MEM_REFERENCE(unit, ing_action_profile[unit], 
                                  ING_ACTION_PROFILE_DEFAULT,
                                  soc_mem_index_count(unit, PORT_TABm));

        /* Increment the ref count for all Protocol-based VLANs */
        SOC_PROFILE_MEM_REFERENCE(unit, ing_action_profile[unit], 
                                  ING_ACTION_PROFILE_DEFAULT,
                                  soc_mem_index_count(unit,
                                                      VLAN_PROTOCOL_DATAm));
    }

    /* Initialize the EGR_VLAN_TAG_ACTION_PROFILE table */
    if (egr_action_profile[unit] == NULL) {
        egr_action_profile[unit] = sal_alloc(sizeof(soc_profile_mem_t),
                                             "Egr Action Profile Mem");
        if (egr_action_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(egr_action_profile[unit]);
    }

    /* Create profile table cache (or re-init if it already exists) */
    mem = EGR_VLAN_TAG_ACTION_PROFILEm;
    entry_words = sizeof(egr_vlan_tag_action_profile_entry_t) / sizeof(uint32);
    SOC_IF_ERROR_RETURN(soc_profile_mem_create(unit, &mem, &entry_words, 1,
                                               egr_action_profile[unit]));

    SOC_PBMP_COUNT(PBMP_ALL(unit), num_ports);
    if (SOC_WARM_BOOT(unit)) {
        /* Increment the ref count for all ports */
        PBMP_ALL_ITER(unit, i) {
            SOC_IF_ERROR_RETURN
                (READ_EGR_VLAN_CONTROL_3r(unit, i, &rval));
            idx = soc_reg_field_get(unit, EGR_VLAN_CONTROL_3r,
                                    rval, TAG_ACTION_PROFILE_PTRf);
            SOC_PROFILE_MEM_REFERENCE(unit, egr_action_profile[unit], idx, 1);
            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, egr_action_profile[unit], idx, 1);
        }

        /* Increment the ref count for all egress vlan translations */
        for (i = 0; i < soc_mem_index_count(unit, EGR_VLAN_XLATEm); i++) {
            SOC_IF_ERROR_RETURN
                (READ_EGR_VLAN_XLATEm(unit, MEM_BLOCK_ANY, i, &egr_xlate_entry));
            if (!soc_mem_field32_get(unit, EGR_VLAN_XLATEm, &egr_xlate_entry, VALIDf)) {
                continue;
            }
            idx = soc_mem_field32_get(unit, EGR_VLAN_XLATEm,
                                      &egr_xlate_entry, TAG_ACTION_PROFILE_PTRf);
            SOC_PROFILE_MEM_REFERENCE(unit, egr_action_profile[unit], idx, 1);
            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, egr_action_profile[unit], idx, 1);

#if defined(BCM_TRIUMPH2_SUPPORT)
            if (soc_feature(unit, soc_feature_gport_service_counters)) {
                fs_idx =
                    soc_EGR_VLAN_XLATEm_field32_get(unit, &egr_xlate_entry,
                                                VINTF_CTR_IDXf);
                if (SOC_MEM_FIELD_VALID(unit, EGR_VLAN_XLATEm,
                                        USE_VINTF_CTR_IDXf)) {
                    if (0 == soc_mem_field32_get(unit, EGR_VLAN_XLATEm,
                                     &egr_xlate_entry, USE_VINTF_CTR_IDXf)) {
                        fs_idx = 0;
                    }
                }
                if (0 != fs_idx) { /* Recover flex stat counter */
                    sal_memset(&event, 0, sizeof(event));
                    /* Construct key-only entry, copy to FS handle */
                    soc_EGR_VLAN_XLATEm_field32_set(unit, &event,
                                                    ENTRY_TYPEf,
                             soc_EGR_VLAN_XLATEm_field32_get(unit,
                                          &egr_xlate_entry, ENTRY_TYPEf));
                    soc_mem_field_get(unit, EGR_VLAN_XLATEm,
                                      (uint32 *) &egr_xlate_entry,
                                      KEYf, (uint32 *) key);
                    soc_mem_field_set(unit, EGR_VLAN_XLATEm,
                                      (uint32 *) &event,
                                      KEYf, (uint32 *) key);

                    _BCM_FLEX_STAT_HANDLE_CLEAR(handle);
                    _BCM_FLEX_STAT_HANDLE_COPY(handle, event);
                    _bcm_esw_flex_stat_ext_reinit_add(unit,
                                  _bcmFlexStatTypeVxlt, fs_idx, handle);
                }
            }
#endif
        }

        /* One extra increment to preserve location EGR_ACTION_PROFILE_DEFAULT */
        SOC_PROFILE_MEM_REFERENCE(unit, egr_action_profile[unit], 
                              EGR_ACTION_PROFILE_DEFAULT, 1);
    } else {
        /* Initialize the EGR_ACTION_PROFILE_DEFAULT to have all
         * actions as NOP (0).
         */
        sal_memset(&egr_profile_entry, 0, sizeof(egr_vlan_tag_action_profile_entry_t));
        entries[0] = &egr_profile_entry;
        SOC_IF_ERROR_RETURN
            (soc_profile_mem_add(unit, egr_action_profile[unit],
                                 (void *) &entries, 1, &temp_index));

        /* Increment the ref count for all ports */
        SOC_PROFILE_MEM_REFERENCE(unit, egr_action_profile[unit], 
                                  EGR_ACTION_PROFILE_DEFAULT, num_ports);
    }

    /* Initialize the VLAN range profile table */
    _bcm_trx_vlan_range_profile_init(unit);

#ifdef BCM_WARM_BOOT_SUPPORT
    /* Reload VLAN protocol action data */
    if (SOC_WARM_BOOT(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_trx_vlan_port_protocol_action_reinit(unit));
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    return BCM_E_NONE;
}

/*
 * Function : _bcm_trx_vlan_action_profile_entry_add
 *
 * Purpose  : add a new entry to vlan action profile table
 *
 */
int
_bcm_trx_vlan_action_profile_entry_add(int unit,
                                      bcm_vlan_action_set_t *action,
                                      uint32 *index)
{   
    ing_vlan_tag_action_profile_entry_t entry;
    void *entries[1];

    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_verify(unit, action));

    sal_memset(&entry, 0, sizeof(ing_vlan_tag_action_profile_entry_t));

    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        DT_OTAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->dt_outer));
    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        DT_POTAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->dt_outer_prio));
    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        DT_ITAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->dt_inner));
    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        DT_PITAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->dt_inner_prio));
    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        SOT_OTAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->ot_outer));
    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        SOT_POTAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->ot_outer_prio));
    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        SOT_ITAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->ot_inner));
    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        SIT_OTAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->it_outer));
    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        SIT_ITAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->it_inner));
    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        SIT_PITAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->it_inner_prio));
    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        UT_OTAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->ut_outer));
    soc_mem_field32_set(unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry,
                        UT_ITAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->ut_inner));

    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, DT_OPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->dt_outer_pkt_prio));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, DT_OCFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->dt_outer_cfi));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, DT_IPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->dt_inner_pkt_prio));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, DT_ICFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->dt_inner_cfi));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_OPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ot_outer_pkt_prio));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_OCFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ot_outer_cfi));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_IPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ot_inner_pkt_prio));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_ICFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ot_inner_cfi));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_OPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->it_outer_pkt_prio));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_OCFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->it_outer_cfi));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_IPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->it_inner_pkt_prio));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_ICFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->it_inner_cfi));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, UT_OPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ut_outer_pkt_prio));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, UT_OCFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ut_outer_cfi));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, UT_IPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ut_inner_pkt_prio));
        soc_mem_field32_set
            (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, UT_ICFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ut_inner_cfi));

#if defined(BCM_TRIDENT2_SUPPORT)
        if (soc_feature(unit, soc_feature_fcoe)) {

            /* overwrite just fcoe related fields (pri)*/
            soc_mem_field32_set
                (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, DT_OPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                   action->dt_outer_pkt_prio));
            soc_mem_field32_set
                (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, DT_IPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                   action->dt_inner_pkt_prio));
            soc_mem_field32_set
                (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_OPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                   action->ot_outer_pkt_prio));
            soc_mem_field32_set
                (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_IPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                   action->ot_inner_pkt_prio));
            soc_mem_field32_set
                (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_OPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                   action->it_outer_pkt_prio));
            soc_mem_field32_set
                (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_IPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                   action->it_inner_pkt_prio));
            soc_mem_field32_set
                (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, UT_OPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                   action->ut_outer_pkt_prio));
            soc_mem_field32_set
                (unit, ING_VLAN_TAG_ACTION_PROFILEm, &entry, UT_IPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                   action->ut_inner_pkt_prio));
        }
#endif  /* BCM_TRIDENT2_SUPPORT */
    }

    entries[0] = &entry;
    return (soc_profile_mem_add(unit, ing_action_profile[unit],
                                (void *) &entries, 1, index));
}

#if defined(BCM_TRIDENT2_SUPPORT)

/*
 * add a new entry to ingress vlan action profile table(without modification)
 */
int
_bcm_trx_ing_vlan_action_profile_entry_no_mod_add(
                               int                                 unit, 
                               ing_vlan_tag_action_profile_entry_t *entry, 
                               uint32                              *index)
{
    void *entries[1];

    entries[0] = entry;
    return soc_profile_mem_add(unit, ing_action_profile[unit],
                               (void *) &entries, 1, index);
}

/*
 * add a new entry to egress vlan action profile table(without modification)
 */
int
_bcm_trx_egr_vlan_action_profile_entry_no_mod_add(
                               int                                 unit, 
                               egr_vlan_tag_action_profile_entry_t *entry, 
                               uint32                              *index)
{
    void *entries[1];

    entries[0] = entry;
    return soc_profile_mem_add(unit, egr_action_profile[unit],
                               (void *) &entries, 1, index);
}

#endif


/*
 * Function : _bcm_trx_vlan_action_profile_entry_increment
 *
 * Purpose  : increment the refcount for a vlan action profile table entry
 *
 */
void
_bcm_trx_vlan_action_profile_entry_increment(int unit, uint32 index)
{   
    SOC_PROFILE_MEM_REFERENCE(unit, ing_action_profile[unit], index, 1);
}

/*
 * Function : _bcm_trx_vlan_action_profile_entry_get
 *
 * Purpose  : get a copy of cached vlan action profile table
 *
 */
void
_bcm_trx_vlan_action_profile_entry_get(int unit,
                                      bcm_vlan_action_set_t *action, 
                                      uint32 index)
{
    ing_vlan_tag_action_profile_entry_t *entry;
    uint32 val;

    entry = SOC_PROFILE_MEM_ENTRY(unit, ing_action_profile[unit],
                                  ing_vlan_tag_action_profile_entry_t *,
                                  index);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              DT_OTAG_ACTIONf);
    action->dt_outer = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              DT_POTAG_ACTIONf);
    action->dt_outer_prio = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              DT_ITAG_ACTIONf);
    action->dt_inner = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              DT_PITAG_ACTIONf);
    action->dt_inner_prio = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              SOT_OTAG_ACTIONf);
    action->ot_outer = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              SOT_POTAG_ACTIONf);
    action->ot_outer_prio = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              SOT_ITAG_ACTIONf);
    action->ot_inner = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              SIT_OTAG_ACTIONf);
    action->it_outer = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              SIT_ITAG_ACTIONf);
    action->it_inner = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              SIT_PITAG_ACTIONf);
    action->it_inner_prio = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              UT_OTAG_ACTIONf);
    action->ut_outer = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                              UT_ITAG_ACTIONf);
    action->ut_inner = _BCM_TRX_TAG_ACTION_DECODE(val);

    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_OPRI_ACTIONf);
        action->dt_outer_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_OCFI_ACTIONf);
        action->dt_outer_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_IPRI_ACTIONf);
        action->dt_inner_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_ICFI_ACTIONf);
        action->dt_inner_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SOT_OPRI_ACTIONf);
        action->ot_outer_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SOT_OCFI_ACTIONf);
        action->ot_outer_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SOT_IPRI_ACTIONf);
        action->ot_inner_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SOT_ICFI_ACTIONf);
        action->ot_inner_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SIT_OPRI_ACTIONf);
        action->it_outer_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SIT_OCFI_ACTIONf);
        action->it_outer_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SIT_IPRI_ACTIONf);
        action->it_inner_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SIT_ICFI_ACTIONf);
        action->it_inner_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  UT_OPRI_ACTIONf);
        action->ut_outer_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  UT_OCFI_ACTIONf);
        action->ut_outer_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  UT_IPRI_ACTIONf);
        action->ut_inner_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                  UT_ICFI_ACTIONf);
        action->ut_inner_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

#if defined(BCM_TRIDENT2_SUPPORT)

        if (soc_feature(unit, soc_feature_fcoe)) {

            /* overwrite only fcoe related values (pri) */

            val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                      DT_OPRI_ACTIONf);
            action->dt_outer_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                      DT_IPRI_ACTIONf);
            action->dt_inner_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                      SOT_OPRI_ACTIONf);
            action->ot_outer_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                      SOT_IPRI_ACTIONf);
            action->ot_inner_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                      SIT_OPRI_ACTIONf);
            action->it_outer_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                      SIT_IPRI_ACTIONf);
            action->it_inner_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                      UT_OPRI_ACTIONf);
            action->ut_outer_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, ING_VLAN_TAG_ACTION_PROFILEm, entry,
                                      UT_IPRI_ACTIONf);
            action->ut_inner_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);
                
        }
#endif /* BCM_TRIDENT2_SUPPORT */

    } else {
        action->dt_outer_pkt_prio = bcmVlanActionNone;
        action->dt_outer_cfi = bcmVlanActionNone;
        action->dt_inner_pkt_prio = bcmVlanActionNone;
        action->dt_inner_cfi = bcmVlanActionNone;
        action->ot_outer_pkt_prio = bcmVlanActionNone;
        action->ot_outer_cfi = bcmVlanActionNone;
        action->ot_inner_pkt_prio = bcmVlanActionNone;
        action->ot_inner_cfi = bcmVlanActionNone;
        action->it_outer_pkt_prio = bcmVlanActionNone;
        action->it_outer_cfi = bcmVlanActionNone;
        action->it_inner_pkt_prio = bcmVlanActionNone;
        action->it_inner_cfi = bcmVlanActionNone;
        action->ut_outer_pkt_prio = bcmVlanActionNone;
        action->ut_outer_cfi = bcmVlanActionNone;
        action->ut_inner_pkt_prio = bcmVlanActionNone;
        action->ut_inner_cfi = bcmVlanActionNone;
    }
}

/*
 * Function : _bcm_trx_vlan_action_profile_entry_delete
 *
 * Purpose  : remove an entry from vlan action profile table
 *
 */
int
_bcm_trx_vlan_action_profile_entry_delete(int unit, uint32 index)
{
    return soc_profile_mem_delete(unit, ing_action_profile[unit], index);
}

/*
 * Function : _bcm_trx_egr_vlan_action_profile_entry_add
 *
 * Purpose  : add a new entry to egress vlan action profile table
 *
 */
int
_bcm_trx_egr_vlan_action_profile_entry_add(int unit,
                                          bcm_vlan_action_set_t *action,
                                          uint32 *index)
{   
    egr_vlan_tag_action_profile_entry_t entry;
    void *entries[1];

    sal_memset(&entry, 0, sizeof(egr_vlan_tag_action_profile_entry_t));

    soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                        DT_OTAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->dt_outer));
    soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                        DT_ITAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->dt_inner));
    soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                        DT_PITAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->dt_inner_prio));
    soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                        SOT_OTAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->ot_outer));
    soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                        SOT_ITAG_ACTIONf,
                        _BCM_TRX_TAG_ACTION_ENCODE(action->ot_inner));

#if defined(BCM_TRIDENT2_SUPPORT)

    if (soc_feature(unit, soc_feature_fcoe)) {
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            DT_OTAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE_EGR_FCOE_EXT(
                                                           action->dt_outer));
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            DT_ITAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE_EGR_FCOE_EXT(
                                                           action->dt_inner));
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            DT_PITAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE_EGR_FCOE_EXT(
                                                       action->dt_inner_prio));
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            SOT_OTAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE_EGR_FCOE_EXT(
                                                           action->ot_outer));
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            SOT_ITAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE_EGR_FCOE_EXT(
                                                           action->ot_inner));
    }

#endif  /* BCM_TRIDENT2_SUPPORT */

    if (soc_feature(unit, soc_feature_vlan_egr_it_inner_replace)) {
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            DT_POTAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(action->dt_outer_prio));
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            SOT_POTAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(action->ot_outer_prio));
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            SIT_OTAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(action->it_outer));
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            SIT_ITAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(action->it_inner));
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            SIT_PITAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(action->it_inner_prio));
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            UT_OTAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(action->ut_outer));
        soc_mem_field32_set(unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry,
                            UT_ITAG_ACTIONf,
                            _BCM_TRX_TAG_ACTION_ENCODE(action->ut_inner));
    }

    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, DT_OPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->dt_outer_pkt_prio));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, DT_OCFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->dt_outer_cfi));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, DT_IPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->dt_inner_pkt_prio));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, DT_ICFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->dt_inner_cfi));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_OPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ot_outer_pkt_prio));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_OCFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ot_outer_cfi));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_IPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ot_inner_pkt_prio));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_ICFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ot_inner_cfi));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_OPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->it_outer_pkt_prio));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_OCFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->it_outer_cfi));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_IPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->it_inner_pkt_prio));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_ICFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->it_inner_cfi));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, UT_OPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ut_outer_pkt_prio));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, UT_OCFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ut_outer_cfi));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, UT_IPRI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ut_inner_pkt_prio));
        soc_mem_field32_set
            (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, UT_ICFI_ACTIONf,
             _BCM_TRX_PRI_CFI_ACTION_ENCODE(action->ut_inner_cfi));

#if defined(BCM_TRIDENT2_SUPPORT)

        if (soc_feature(unit, soc_feature_fcoe)) {

            /* overwrite fcoe related fields (pri) */
            soc_mem_field32_set
                (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, DT_OPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                action->dt_outer_pkt_prio));
            soc_mem_field32_set
                (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, DT_IPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                action->dt_inner_pkt_prio));
            soc_mem_field32_set
                (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_OPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                action->ot_outer_pkt_prio));
            soc_mem_field32_set
                (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SOT_IPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                action->ot_inner_pkt_prio));
            soc_mem_field32_set
                (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_OPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                action->it_outer_pkt_prio));
            soc_mem_field32_set
                (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, SIT_IPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                action->it_inner_pkt_prio));
            soc_mem_field32_set
                (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, UT_OPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                action->ut_outer_pkt_prio));
            soc_mem_field32_set
                (unit, EGR_VLAN_TAG_ACTION_PROFILEm, &entry, UT_IPRI_ACTIONf,
                 _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(
                                                action->ut_inner_pkt_prio));
        }
#endif  /* BCM_TRIDENT2_SUPPORT */
    }

    entries[0] = &entry;
    return (soc_profile_mem_add(unit, egr_action_profile[unit],
                                (void *) &entries, 1, index));
}

/*
 * Function : _bcm_trx_egr_vlan_action_profile_entry_increment
 *
 * Purpose  : increment the refcount for an egress vlan action profile table entry
 *
 */
void
_bcm_trx_egr_vlan_action_profile_entry_increment(int unit, uint32 index)
{   
    SOC_PROFILE_MEM_REFERENCE(unit, egr_action_profile[unit], index, 1);
}

/*
 * Function : _bcm_trx_egr_vlan_action_profile_entry_get
 *
 * Purpose  : get a copy of cached egress vlan action profile table
 *
 */
void
_bcm_trx_egr_vlan_action_profile_entry_get(int unit,
                                          bcm_vlan_action_set_t *action, 
                                          uint32 index)
{
    egr_vlan_tag_action_profile_entry_t *entry;
    uint32 val;

    entry = SOC_PROFILE_MEM_ENTRY(unit, egr_action_profile[unit],
                                  egr_vlan_tag_action_profile_entry_t *,
                                  index);

    val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                              DT_OTAG_ACTIONf);
    action->dt_outer = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                              DT_ITAG_ACTIONf);
    action->dt_inner = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                              DT_PITAG_ACTIONf);
    action->dt_inner_prio = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                              SOT_OTAG_ACTIONf);
    action->ot_outer = _BCM_TRX_TAG_ACTION_DECODE(val);

    val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                              SOT_ITAG_ACTIONf);
    action->ot_inner = _BCM_TRX_TAG_ACTION_DECODE(val);

#if defined(BCM_TRIDENT2_SUPPORT)

    if (soc_feature(unit, soc_feature_fcoe)) {
        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_OTAG_ACTIONf);
        action->dt_outer = _BCM_TRX_TAG_ACTION_DECODE_EGR_FCOE_EXT(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_ITAG_ACTIONf);
        action->dt_inner = _BCM_TRX_TAG_ACTION_DECODE_EGR_FCOE_EXT(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_PITAG_ACTIONf);
        action->dt_inner_prio = _BCM_TRX_TAG_ACTION_DECODE_EGR_FCOE_EXT(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SOT_OTAG_ACTIONf);
        action->ot_outer = _BCM_TRX_TAG_ACTION_DECODE_EGR_FCOE_EXT(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                              SOT_ITAG_ACTIONf);
        action->ot_inner = _BCM_TRX_TAG_ACTION_DECODE_EGR_FCOE_EXT(val);
    }

#endif  /* BCM_TRIDENT2_SUPPORT */

    if (soc_feature(unit, soc_feature_vlan_egr_it_inner_replace)) {
        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_POTAG_ACTIONf);
        action->dt_outer_prio = _BCM_TRX_TAG_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SOT_POTAG_ACTIONf);
        action->ot_outer_prio = _BCM_TRX_TAG_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SIT_OTAG_ACTIONf);
        action->it_outer = _BCM_TRX_TAG_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SIT_ITAG_ACTIONf);
        action->it_inner = _BCM_TRX_TAG_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SIT_PITAG_ACTIONf);
        action->it_inner_prio = _BCM_TRX_TAG_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  UT_OTAG_ACTIONf);
        action->ut_outer = _BCM_TRX_TAG_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  UT_ITAG_ACTIONf);
        action->ut_inner = _BCM_TRX_TAG_ACTION_DECODE(val);
    } else {
        action->it_outer = bcmVlanActionNone;
        action->it_inner = bcmVlanActionNone;
        action->it_inner_prio = bcmVlanActionNone;
        action->ut_outer = bcmVlanActionNone;
        action->ut_inner = bcmVlanActionNone;
    }

    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_OPRI_ACTIONf);
        action->dt_outer_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_OCFI_ACTIONf);
        action->dt_outer_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_IPRI_ACTIONf);
        action->dt_inner_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  DT_ICFI_ACTIONf);
        action->dt_inner_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SOT_OPRI_ACTIONf);
        action->ot_outer_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SOT_OCFI_ACTIONf);
        action->ot_outer_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SOT_IPRI_ACTIONf);
        action->ot_inner_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SOT_ICFI_ACTIONf);
        action->ot_inner_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SIT_OPRI_ACTIONf);
        action->it_outer_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SIT_OCFI_ACTIONf);
        action->it_outer_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SIT_IPRI_ACTIONf);
        action->it_inner_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  SIT_ICFI_ACTIONf);
        action->it_inner_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  UT_OPRI_ACTIONf);
        action->ut_outer_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  UT_OCFI_ACTIONf);
        action->ut_outer_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  UT_IPRI_ACTIONf);
        action->ut_inner_pkt_prio = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

        val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                  UT_ICFI_ACTIONf);
        action->ut_inner_cfi = _BCM_TRX_PRI_CFI_ACTION_DECODE(val);

#if defined(BCM_TRIDENT2_SUPPORT)

        if (soc_feature(unit, soc_feature_fcoe)) {

            /* overwrite fcoe related values (pri) */
            val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                      DT_OPRI_ACTIONf);
            action->dt_outer_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                      DT_IPRI_ACTIONf);
            action->dt_inner_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                      SOT_OPRI_ACTIONf);
            action->ot_outer_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                      SOT_IPRI_ACTIONf);
            action->ot_inner_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                      SIT_OPRI_ACTIONf);
            action->it_outer_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                      SIT_IPRI_ACTIONf);
            action->it_inner_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                      UT_OPRI_ACTIONf);
            action->ut_outer_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

            val = soc_mem_field32_get(unit, EGR_VLAN_TAG_ACTION_PROFILEm, entry,
                                      UT_IPRI_ACTIONf);
            action->ut_inner_pkt_prio = 
                _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(val);

        }

#endif  /* BCM_TRIDENT2_SUPPORT */

    } else {
        action->dt_outer_pkt_prio = bcmVlanActionNone;
        action->dt_outer_cfi = bcmVlanActionNone;
        action->dt_inner_pkt_prio = bcmVlanActionNone;
        action->dt_inner_cfi = bcmVlanActionNone;
        action->ot_outer_pkt_prio = bcmVlanActionNone;
        action->ot_outer_cfi = bcmVlanActionNone;
        action->ot_inner_pkt_prio = bcmVlanActionNone;
        action->ot_inner_cfi = bcmVlanActionNone;
        action->it_outer_pkt_prio = bcmVlanActionNone;
        action->it_outer_cfi = bcmVlanActionNone;
        action->it_inner_pkt_prio = bcmVlanActionNone;
        action->it_inner_cfi = bcmVlanActionNone;
        action->ut_outer_pkt_prio = bcmVlanActionNone;
        action->ut_outer_cfi = bcmVlanActionNone;
        action->ut_inner_pkt_prio = bcmVlanActionNone;
        action->ut_inner_cfi = bcmVlanActionNone;
    }
}

/*
 * Function : _bcm_trx_egr_vlan_action_profile_entry_delete
 *
 * Purpose  : remove an entry from egress vlan action profile table
 *
 */
int
_bcm_trx_egr_vlan_action_profile_entry_delete(int unit, uint32 index)
{
    return soc_profile_mem_delete(unit, egr_action_profile[unit], index);
}

STATIC soc_field_t _tr_range_min_f[] = {VLAN_MIN_0f, VLAN_MIN_1f, 
                                        VLAN_MIN_2f, VLAN_MIN_3f,
                                        VLAN_MIN_4f, VLAN_MIN_5f, 
                                        VLAN_MIN_6f, VLAN_MIN_7f};
STATIC soc_field_t _tr_range_max_f[] = {VLAN_MAX_0f, VLAN_MAX_1f, 
                                        VLAN_MAX_2f, VLAN_MAX_3f,
                                        VLAN_MAX_4f, VLAN_MAX_5f, 
                                        VLAN_MAX_6f, VLAN_MAX_7f}; 

/*
 * Function : bcm_trx_vlan_range_profile_init
 *
 * Purpose  : to initialize hardware ING_VLAN_RANGE table
 *            allocate memory to cache hardware tables in RAM.
 *
 * Note:
 *      Allocate memory to cache the profile tables and initialize.
 *      If memory to cache the profile table is already allocated, just
 *      initialize the table.
 */
STATIC int
_bcm_trx_vlan_range_profile_init(int unit)
{
    int i, idx, inner_idx;
    source_trunk_map_table_entry_t stm_entry;
    trunk32_port_table_entry_t trunk32_entry;
    ing_vlan_range_entry_t profile_entry;
    uint32 temp_index, temp_inner_index;
    soc_mem_t mem;
    int entry_words;
    void *entries[1];
    soc_field_t field;

    /* Initialize the ING_VLAN_RANGE table */

    if (vlan_range_profile[unit] == NULL) {
        vlan_range_profile[unit] = sal_alloc(sizeof(soc_profile_mem_t),
                                             "Vlan Range Profile Mem");
        if (vlan_range_profile[unit] == NULL) {
            return BCM_E_MEMORY;
        }
        soc_profile_mem_t_init(vlan_range_profile[unit]);
    }

    /* Create profile table cache (or re-init if it already exists) */
    mem = ING_VLAN_RANGEm;
    entry_words = sizeof(ing_vlan_range_entry_t) / sizeof(uint32);
    SOC_IF_ERROR_RETURN(soc_profile_mem_create(unit, &mem, &entry_words, 1,
                                               vlan_range_profile[unit]));

    if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                OUTER_VLAN_RANGE_IDXf)) {
        field = OUTER_VLAN_RANGE_IDXf;
    } else {
        field = VLAN_RANGE_IDXf;
    }
    if (SOC_WARM_BOOT(unit)) {
        /* Increment the ref count for all ports */
        for (i = 0;
             i < soc_mem_index_count(unit, SOURCE_TRUNK_MAP_TABLEm); i++) {
            SOC_IF_ERROR_RETURN
                (READ_SOURCE_TRUNK_MAP_TABLEm(unit,
                                              MEM_BLOCK_ANY, i, &stm_entry));
       
            idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                                      &stm_entry, field);
            SOC_PROFILE_MEM_REFERENCE(unit, vlan_range_profile[unit], idx, 1);
            SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, vlan_range_profile[unit],
                                            idx, 1);
            if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                                   INNER_VLAN_RANGE_IDXf)) {
                inner_idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                                         &stm_entry, INNER_VLAN_RANGE_IDXf);
                SOC_PROFILE_MEM_REFERENCE(unit, vlan_range_profile[unit],
                                         inner_idx, 1);
                SOC_PROFILE_MEM_ENTRIES_PER_SET(unit, vlan_range_profile[unit],
                                         inner_idx, 1);
            }
        }

        if (SOC_MEM_FIELD_VALID(unit, TRUNK32_PORT_TABLEm, VLAN_RANGE_IDXf)) {
            if (!soc_property_get(unit, spn_TRUNK_EXTEND, 1)) {
                for (i = 0;
                     i < soc_mem_index_count(unit, TRUNK32_PORT_TABLEm); i++) {
                    SOC_IF_ERROR_RETURN
                        (READ_TRUNK32_PORT_TABLEm(unit,
                                            MEM_BLOCK_ANY, i, &trunk32_entry));
                    idx = soc_mem_field32_get(unit, TRUNK32_PORT_TABLEm,
                            &trunk32_entry, VLAN_RANGE_IDXf);
                    SOC_PROFILE_MEM_REFERENCE(unit,
                            vlan_range_profile[unit], idx, 1);
                    SOC_PROFILE_MEM_ENTRIES_PER_SET(unit,
                            vlan_range_profile[unit], idx, 1);
                }
            }
        }

        /* One extra increment to preserve location VLAN_RANGE_PROFILE_DEFAULT */
        SOC_PROFILE_MEM_REFERENCE(unit, vlan_range_profile[unit], 
                                  VLAN_RANGE_PROFILE_DEFAULT, 1);

    } else {
        sal_memset(&profile_entry, 0, sizeof(ing_vlan_range_entry_t));
        for (i = 0; i < 8; i++) {
            soc_mem_field32_set(unit, ING_VLAN_RANGEm,
                                &profile_entry, _tr_range_min_f[i], 1);
            soc_mem_field32_set(unit, ING_VLAN_RANGEm,
                                &profile_entry, _tr_range_max_f[i], 0);
        }

        /* Initialize the VLAN_RANGE_PROFILE_DEFAULT to have 
         * all ranges unused (min == 1, max == 0).
         */
        entries[0] = &profile_entry;
        SOC_IF_ERROR_RETURN
            (soc_profile_mem_add(unit, vlan_range_profile[unit],
                                 (void *) &entries, 1, &temp_index));

        if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                                INNER_VLAN_RANGE_IDXf)) {
            SOC_IF_ERROR_RETURN
                (soc_profile_mem_add(unit, vlan_range_profile[unit],
                                 (void *) &entries, 1, &temp_inner_index));
        }
        /* Increment the ref count for all ports */
        for (i = 0;
             i < soc_mem_index_count(unit, SOURCE_TRUNK_MAP_TABLEm); i++) {
            SOC_IF_ERROR_RETURN
                (READ_SOURCE_TRUNK_MAP_TABLEm(unit,
                                              MEM_BLOCK_ANY, i, &stm_entry));
            soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm,
                                &stm_entry, field, temp_index);

            SOC_PROFILE_MEM_REFERENCE(unit,
                    vlan_range_profile[unit], temp_index, 1);

            if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                                                  INNER_VLAN_RANGE_IDXf)) {
                soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm, &stm_entry,
                                    INNER_VLAN_RANGE_IDXf, temp_inner_index);
                SOC_PROFILE_MEM_REFERENCE(unit, vlan_range_profile[unit], 
                                    temp_inner_index, 1);
            }
            SOC_IF_ERROR_RETURN
                (WRITE_SOURCE_TRUNK_MAP_TABLEm(unit,
                                              MEM_BLOCK_ANY, i, &stm_entry));
        }

        if (SOC_MEM_FIELD_VALID(unit, TRUNK32_PORT_TABLEm, VLAN_RANGE_IDXf)) {
            if (!soc_property_get(unit, spn_TRUNK_EXTEND, 1)) {
                for (i = 0;
                     i < soc_mem_index_count(unit, TRUNK32_PORT_TABLEm); i++) {
                    if (soc_mem_field32_modify(unit, TRUNK32_PORT_TABLEm, i, 
                            VLAN_RANGE_IDXf, temp_index) != BCM_E_NONE) {
                        continue;
                    }
                    SOC_PROFILE_MEM_REFERENCE(unit, vlan_range_profile[unit],
                            temp_index, 1);
                }
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function : _bcm_trx_vlan_range_profile_entry_add
 *
 * Purpose  : add a new entry to vlan range profile table
 *
 */
int
_bcm_trx_vlan_range_profile_entry_add(int unit, bcm_vlan_t *min_vlan,
                                     bcm_vlan_t *max_vlan, uint32 *index)
{   
    ing_vlan_range_entry_t profile_entry;
    void *entries[1];
    int i;

    sal_memset(&profile_entry, 0, sizeof(ing_vlan_range_entry_t));

    for (i = 0; i < 8; i++) {
        soc_mem_field32_set(unit, ING_VLAN_RANGEm,
                            &profile_entry, _tr_range_min_f[i], min_vlan[i]);
        soc_mem_field32_set(unit, ING_VLAN_RANGEm,
                            &profile_entry, _tr_range_max_f[i], max_vlan[i]);
    }
    entries[0] = &profile_entry;
    return (soc_profile_mem_add(unit, vlan_range_profile[unit],
                                (void *) &entries, 1, index));
}

/*
 * Function : _bcm_trx_vlan_range_profile_entry_increment
 *
 * Purpose  : increment the refcount for a vlan range profile table entry
 *
 */
void
_bcm_trx_vlan_range_profile_entry_increment(int unit, uint32 index)
{   
    SOC_PROFILE_MEM_REFERENCE(unit, vlan_range_profile[unit], index, 1);
}

/*
 * Function : _bcm_trx_vlan_range_profile_entry_get
 *
 * Purpose  : get a copy of cached vlan range profile table
 *
 */
void
_bcm_trx_vlan_range_profile_entry_get(int unit, bcm_vlan_t *min_vlan, 
                                     bcm_vlan_t *max_vlan, uint32 index)
{
    int i;
    ing_vlan_range_entry_t *profile_entry;

    profile_entry = SOC_PROFILE_MEM_ENTRY(unit, vlan_range_profile[unit],
                                          ing_vlan_range_entry_t *,
                                          index);

    for (i = 0; i < 8; i++) {
        min_vlan[i] = soc_mem_field32_get(unit, ING_VLAN_RANGEm,
                                          profile_entry, _tr_range_min_f[i]);
        max_vlan[i] = soc_mem_field32_get(unit, ING_VLAN_RANGEm,
                                          profile_entry, _tr_range_max_f[i]);
    }
}

/*
 * Function : _bcm_trx_vlan_range_profile_entry_delete
 *
 * Purpose  : remove an entry from vlan range profile table
 *
 */
int
_bcm_trx_vlan_range_profile_entry_delete(int unit, uint32 index)
{
    return soc_profile_mem_delete(unit, vlan_range_profile[unit], index);
}

/*
 * On XGS3, VLAN_SUBNET.IP_ADDR stores only a top half (64 bits) of
 * the IPV6 address. Comparing it against a full length of a parameter
 * will have certainly result in a mismatch. There should be a
 * special type for a stored part of the address; meanwhile, a
 * symbolic constant is used.
 */

#define VLAN_SUBNET_IP_ADDR_LENGTH 8

/*
 * Function:
 *      _trx_vlan_ip_addr_mask_get
 * Purpose:
 *      Get IPV6 address and mask from vlan_ip structure
 * Parameters:
 *      vlan_ip -   structure specifying IP address and other info
 *      ip6addr -   IPv6 address to retrieve
 *      ip6mask -   IPv6 mask to retrieve
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_trx_vlan_ip_addr_mask_get(bcm_vlan_ip_t *vlan_ip, bcm_ip6_t ip6addr, 
                           bcm_ip6_t ip6mask)
{
    if (vlan_ip->flags & BCM_VLAN_SUBNET_IP6) {
        if (vlan_ip->prefix > 64) {
            return BCM_E_PARAM;
        }

        bcm_ip6_mask_create(ip6mask, vlan_ip->prefix); 
        sal_memcpy(ip6addr, vlan_ip->ip6, sizeof(bcm_ip6_t));
    } else { /* IPv4 entry */
        sal_memcpy(ip6addr, "\xff\xff\x00\x00", 4);
        ip6addr[4] = vlan_ip->ip4 >> 24;
        ip6addr[5] = (vlan_ip->ip4 >> 16) & 0xff;
        ip6addr[6] = (vlan_ip->ip4 >> 8) & 0xff;
        ip6addr[7] = vlan_ip->ip4 & 0xff;

        sal_memcpy(ip6mask, "\xff\xff\xff\xff", 4);
        ip6mask[4] = vlan_ip->mask >> 24;
        ip6mask[5] = (vlan_ip->mask >> 16) & 0xff;
        ip6mask[6] = (vlan_ip->mask >> 8) & 0xff;
        ip6mask[7] = vlan_ip->mask & 0xff;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _tr_vlan_subnet_mem_read
 * Purpose:
 *      Helper function to allocated memory and read by range all 
 *      VLAN_SUBNET memory
 * Parameters:
 *      unit    -       (IN) BCM Device unit
 *      vstab   -       (OUT) pointer to memory
 *      nent    -       (OUT) total number of entries in the memory
 * Returns:
 *      BCM_E_XXX
 * Note: 
 *      This routine will allocate a memory, it is up to caller responsibility
 *      to free it 
 */
STATIC int 
_tr_vlan_subnet_mem_read(int unit, vlan_subnet_entry_t **vstab, int *nent)
{
    int imin, imax, vsbytes, rv;

    imin = soc_mem_index_min(unit, VLAN_SUBNETm);
    imax = soc_mem_index_max(unit, VLAN_SUBNETm);
    *nent = soc_mem_index_count(unit, VLAN_SUBNETm);
    vsbytes = soc_mem_entry_words(unit, VLAN_SUBNETm);
    vsbytes = WORDS2BYTES(vsbytes);
    *vstab = soc_cm_salloc(unit, *nent * sizeof(vlan_subnet_entry_t), "vlan_subnet");
    if (*vstab == NULL) {
        return BCM_E_MEMORY;
    }

    rv = soc_mem_read_range(unit, VLAN_SUBNETm, MEM_BLOCK_ANY,
                            imin, imax, *vstab);
    return rv;
}

/*
 * Function:
 *      _trx_vlan_subnet_entry_parse
 * Purpose:
 *      Helper function to parse all information fields from 
 *      VLAN_SUBNET memory entry
 * Parameters:
 *      unit    -       (IN) BCM Device unit
 *      ventry   -      (IN) pointer to a vlan subnet entry 
 *      subnet_fields - (OUT) Structure contains all entry fields
 * Returns:
 *      none
 * Note: 
 *      
 */
STATIC void 
_trx_vlan_subnet_entry_parse(int unit, vlan_subnet_entry_t *ventry, 
                           _bcm_trx_vlan_subnet_entry_t *subnet_fields)
{
    bcm_ip6_t   ip6tmp;
    vlan_subnet_entry_t entry;
    uint32      fval[SOC_MAX_MEM_WORDS];

    soc_mem_ip6_addr_get(unit, VLAN_SUBNETm, ventry, IP_ADDRf,
                         subnet_fields->ip, SOC_MEM_IP6_UPPER_ONLY);

    if (soc_mem_field_valid(unit, VLAN_SUBNETm, KEYf)) {
        soc_mem_field_get(unit, VLAN_SUBNETm, (uint32 *)ventry, MASKf, fval);
        soc_mem_field_set(unit, VLAN_SUBNETm, (uint32 *)&entry, KEYf, fval);
        soc_mem_ip6_addr_get(unit, VLAN_SUBNETm, &entry, IP_ADDRf,
                             ip6tmp, SOC_MEM_IP6_UPPER_ONLY);
    } else {
        soc_mem_ip6_addr_get(unit, VLAN_SUBNETm, ventry, MASKf,
                             ip6tmp, SOC_MEM_IP6_UPPER_ONLY);
    }
    subnet_fields->prefix = bcm_ip6_mask_length(ip6tmp);

    subnet_fields->ovid = soc_VLAN_SUBNETm_field32_get(unit, ventry, OVIDf);
    subnet_fields->ivid = soc_VLAN_SUBNETm_field32_get(unit, ventry, IVIDf);
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        subnet_fields->opri =
            soc_VLAN_SUBNETm_field32_get(unit, ventry, OPRIf);
        subnet_fields->ocfi =
            soc_VLAN_SUBNETm_field32_get(unit, ventry, OCFIf);
        subnet_fields->ipri =
            soc_VLAN_SUBNETm_field32_get(unit, ventry, IPRIf);
        subnet_fields->icfi =
            soc_VLAN_SUBNETm_field32_get(unit, ventry, ICFIf);
    } else {
        subnet_fields->opri = soc_VLAN_SUBNETm_field32_get(unit, ventry, PRIf);
    }
    subnet_fields->profile_idx = soc_VLAN_SUBNETm_field32_get(unit, ventry, 
                                                    TAG_ACTION_PROFILE_PTRf);
}

/*
 * Function:
 *      _trx_vlan_subnet_entry_set
 * Purpose:
 *      Helper function to write all fields to 
 *      VLAN_SUBNET memory entry
 * Parameters:
 *      unit    -       (IN) BCM Device unit
 *      ventry   -      (IN) pointer to a vlan subnet entry 
 *      subnet_fields - (IN) Structure contains all entry fields
 * Returns:
 *      none
 * Note: 
 *      This routine assumes that all rovided fields has correct values
 */

STATIC void 
_trx_vlan_subnet_entry_set(int unit, vlan_subnet_entry_t *ventry, 
                           _bcm_trx_vlan_subnet_entry_t *subnet_fields)
{
    uint32      fval[SOC_MAX_MEM_WORDS];

    sal_memset(fval, 0, sizeof(fval));
    if (soc_mem_field_valid(unit, VLAN_SUBNETm, KEYf)) {
        soc_mem_field_set(unit, VLAN_SUBNETm, (uint32 *)ventry, KEYf, fval);
        soc_mem_ip6_addr_set(unit, VLAN_SUBNETm, ventry, IP_ADDRf,
                             subnet_fields->mask, SOC_MEM_IP6_UPPER_ONLY);
        soc_mem_field_get(unit, VLAN_SUBNETm, (uint32 *)ventry, KEYf, fval);
        soc_mem_field_set(unit, VLAN_SUBNETm, (uint32 *)ventry, MASKf, fval);
    } else {
        soc_mem_ip6_addr_set(unit, VLAN_SUBNETm, ventry, MASKf,
                             subnet_fields->mask, SOC_MEM_IP6_UPPER_ONLY);
    }
    soc_mem_ip6_addr_set(unit, VLAN_SUBNETm, ventry, IP_ADDRf,
                         subnet_fields->ip, SOC_MEM_IP6_UPPER_ONLY);
    soc_VLAN_SUBNETm_field32_set(unit, ventry, OVIDf, subnet_fields->ovid);
    soc_VLAN_SUBNETm_field32_set(unit, ventry, IVIDf, subnet_fields->ivid);
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        soc_VLAN_SUBNETm_field32_set(unit, ventry, OPRIf, subnet_fields->opri);
        soc_VLAN_SUBNETm_field32_set(unit, ventry, OCFIf, subnet_fields->ocfi);
        soc_VLAN_SUBNETm_field32_set(unit, ventry, IPRIf, subnet_fields->ipri);
        soc_VLAN_SUBNETm_field32_set(unit, ventry, ICFIf, subnet_fields->icfi);
    } else {
        soc_VLAN_SUBNETm_field32_set(unit, ventry, PRIf, subnet_fields->opri);
    }
    soc_VLAN_SUBNETm_field32_set(unit, ventry, TAG_ACTION_PROFILE_PTRf, 
                                 subnet_fields->profile_idx);
    soc_VLAN_SUBNETm_field32_set(unit, ventry, VALIDf, 1);
}

/*
 * Function:
 *      _trx_vlan_subnet_lookup
 * Purpose:
 *      Get index into table with settings matching to subnet
 * Parameters:
 *      unit            (IN) BCM Device unit
 *      vstab           (IN) pointer to memory
 *      nent            (IN) number of entries in the memory
 *      ip              (IN) IP address
 *      ip_mask         (IN) IP address mask
 *      entry_index     (OUT) target entry index (if BCM_E_NONE)
 *                            index to be used for insert or -1 when full
 *                            (if BCM_E_NOT_FOUND)
 *      free_index      (OUT) (optional) index of last used entry + 1
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_trx_vlan_subnet_lookup(int unit, vlan_subnet_entry_t *vstab, int nent,
                        bcm_ip6_t ip, bcm_ip6_t ip_mask, int *entry_index,
                        int *free_index)
{
    bcm_ip6_t           ip_key, ip6_full_mask;
    vlan_subnet_entry_t *vstabp, entry0, entry1, entry2, mask0, mask1;
    int                 index_min, index_max;
    int                 index, i, entry_words, rv;
    uint32              fval[SOC_MAX_MEM_WORDS];

    sal_memcpy(ip_key, ip, VLAN_SUBNET_IP_ADDR_LENGTH);
#ifdef BCM_TRIDENT_SUPPORT
    
    if (soc_feature(unit, soc_feature_xy_tcam)) {
        for (i = 0; i < VLAN_SUBNET_IP_ADDR_LENGTH; i++) {
            ip_key[i] &= ip_mask[i];
        }
    }
#endif /* BCM_TRIDENT_SUPPORT */

    sal_memset(&ip6_full_mask, 0xff, sizeof(ip6_full_mask));
    entry_words = soc_mem_entry_words(unit, VLAN_SUBNETm);

    /* entry2 is for checking VALID bit */
    sal_memset(&entry2, 0, sizeof(entry2));
    soc_mem_field32_set(unit, VLAN_SUBNETm, &entry2, VALIDf, 1);

    /* entry1 and mask1 are for prefix length check */
    entry1 = entry2;
    if (soc_mem_field_valid(unit, VLAN_SUBNETm, KEYf)) {
        sal_memset(&entry0, 0, sizeof(entry0));
        soc_mem_ip6_addr_set(unit, VLAN_SUBNETm, &entry0, IP_ADDRf, ip_mask,
                             SOC_MEM_IP6_UPPER_ONLY);
        soc_mem_field_get(unit, VLAN_SUBNETm, (uint32 *)&entry0, KEYf, fval);
        soc_mem_field_set(unit, VLAN_SUBNETm, (uint32 *)&entry1, MASKf, fval);
        mask1 = entry1;
        soc_mem_ip6_addr_set(unit, VLAN_SUBNETm, &entry0, IP_ADDRf,
                             ip6_full_mask, SOC_MEM_IP6_UPPER_ONLY);
        soc_mem_field_get(unit, VLAN_SUBNETm, (uint32 *)&entry0, KEYf, fval);
        soc_mem_field_set(unit, VLAN_SUBNETm, (uint32 *)&mask1, MASKf, fval);
    } else {
        soc_mem_ip6_addr_set(unit, VLAN_SUBNETm, &entry1, MASKf, ip_mask,
                             SOC_MEM_IP6_UPPER_ONLY);
        mask1 = entry1;
        soc_mem_ip6_addr_set(unit, VLAN_SUBNETm, &mask1, MASKf,
                             ip6_full_mask, SOC_MEM_IP6_UPPER_ONLY);
    }

    /* entry0 and mask0 are for target entry matching */
    entry0 = entry1;
    soc_mem_ip6_addr_set(unit, VLAN_SUBNETm, &entry0, IP_ADDRf, ip_key,
                         SOC_MEM_IP6_UPPER_ONLY);
    mask0 = mask1;
    soc_mem_field32_set(unit, VLAN_SUBNETm, &mask0, VALIDf, 1);
    soc_mem_ip6_addr_set(unit, VLAN_SUBNETm, &mask0, IP_ADDRf, ip_mask,
                         SOC_MEM_IP6_UPPER_ONLY);

    /* Do binary search to find the first entry whose prefix length
     * is shorter than or equal to the target entry */
    index_min = 0;
    index_max = nent - 1;
    while (index_min < index_max) {
        index = (index_min + index_max) / 2;
        vstabp = soc_mem_table_idx_to_pointer(unit, VLAN_SUBNETm,
                                              vlan_subnet_entry_t *,
                                              vstab, index);

        /* Check if the entry has longer prefix */
        for (i = 0; i < entry_words; i++) {
            if ((vstabp->entry_data[i] & mask1.entry_data[i]) >
                entry1.entry_data[i]) {
                break;
            }
        }
        if (i != entry_words) { /* prefix length is longer than target entry */
            index_min = index + 1;
        } else { /* prefix length is shorter than or equal to target entry */
            index_max = index;
        }
    }

    /* Match against entries with the same prefix length as target entry */
    rv = BCM_E_NOT_FOUND;
    *entry_index = -1;
    for (index = index_min; index < nent; index++) {
        vstabp = soc_mem_table_idx_to_pointer(unit, VLAN_SUBNETm,
                                              vlan_subnet_entry_t *,
                                              vstab, index);
        /* Try matching the entry */
        for (i = 0; i < entry_words; i++) {
            if ((vstabp->entry_data[i] ^ entry0.entry_data[i]) &
                mask0.entry_data[i]) {
                break;
            }
        }
        if (i == entry_words) {
            rv = BCM_E_NONE;
            *entry_index = index;
            break;
        }

        /* Find the first shorter prefix entry */
        for (i = 0; i < entry_words; i++) {
            if ((vstabp->entry_data[i] & mask1.entry_data[i]) <
                entry1.entry_data[i]) {
                break;
            }
        }
        if (i != entry_words) {
            *entry_index = index;
            break;
        }
    }


    if (free_index == NULL) {
        return rv;
    }

    /* Do binary search to find the index after the last used entry */
    index_min = index;
    index_max = nent - 1;
    while (index_min <= index_max) {
        index = (index_min + index_max) / 2;
        vstabp = soc_mem_table_idx_to_pointer(unit, VLAN_SUBNETm,
                                              vlan_subnet_entry_t *,
                                              vstab, index);

        /* Check if the entry is valid */
        for (i = 0; i < entry_words; i++) {
            if (vstabp->entry_data[i] & entry2.entry_data[i]) {
                break;
            }
        }
        if (i == entry_words) { /* free entry */
            if (index_min == index_max) {
                break;
            }
            index_max = index;
        } else { /* entry in used */
            index_min = index + 1;
        }
    }

    *free_index = index_min;
    return rv;
}

/*
 * Function:
 *      _trx_vlan_subnet_entry_add
 * Purpose:
 *      Add vlan settings for given subnet
 * Parameters:
 *      unit            - (IN) BCM Device unit
 *      entry_fields    - (IN) Entry fields to add
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_trx_vlan_subnet_entry_add(int unit,
                           _bcm_trx_vlan_subnet_entry_t *entry_fields)
{
    int                         rv, nent, i;
    vlan_subnet_entry_t         *vstab, *vstabp;
    uint32                      old_profile_idx;
    int                         entry_index, free_index;

    soc_mem_lock(unit, VLAN_SUBNETm);

    rv = _tr_vlan_subnet_mem_read(unit, &vstab, &nent);
    if (BCM_FAILURE(rv)) {
        if (NULL != vstab) {
            soc_cm_sfree(unit, vstab);
        }
        soc_mem_unlock(unit, VLAN_SUBNETm);
        return rv;
    }

    rv = _trx_vlan_subnet_lookup(unit, vstab, nent, entry_fields->ip,
                                 entry_fields->mask, &entry_index,
                                 &free_index);
    if (rv == BCM_E_NOT_FOUND && entry_index == -1) {
        rv = BCM_E_FULL;
    }

    if (BCM_SUCCESS(rv)) {   /* found an exact match */
        vstabp = soc_mem_table_idx_to_pointer(unit, VLAN_SUBNETm,
                                              vlan_subnet_entry_t *, vstab,
                                              entry_index);

        /* retrieve old vlan action profile pointer */
        old_profile_idx = soc_VLAN_SUBNETm_field32_get
            (unit, vstabp, TAG_ACTION_PROFILE_PTRf);   
        _trx_vlan_subnet_entry_set(unit, vstabp, entry_fields);
        rv = WRITE_VLAN_SUBNETm(unit, MEM_BLOCK_ALL, entry_index, vstabp);
        if (BCM_SUCCESS(rv)) {
            /* Delete the old vlan action profile entry */
            rv = _bcm_trx_vlan_action_profile_entry_delete(unit,
                                                           old_profile_idx);
        }
    } else if (rv == BCM_E_NOT_FOUND) {
        /* shift down entries starting at the insert point to create hole */
        for (i = free_index - 1; i >= entry_index; i--) {
            vstabp = soc_mem_table_idx_to_pointer(unit, VLAN_SUBNETm,
                                                  vlan_subnet_entry_t *, vstab,
                                                  i);
            rv = WRITE_VLAN_SUBNETm(unit, MEM_BLOCK_ANY, i + 1, vstabp);
            if (BCM_FAILURE(rv)) {
                break;
            }
        }
        if (i < entry_index) {
            /* insert new entry at the created hole */
            vstabp = soc_mem_table_idx_to_pointer(unit, VLAN_SUBNETm,
                                                  vlan_subnet_entry_t *, vstab,
                                                  entry_index);
            sal_memset(vstabp, 0,
                       WORDS2BYTES(soc_mem_entry_words(unit, VLAN_SUBNETm)));
            _trx_vlan_subnet_entry_set(unit, vstabp, entry_fields);
            rv = WRITE_VLAN_SUBNETm(unit, MEM_BLOCK_ALL, entry_index, vstabp);
        }
    }
    soc_mem_unlock(unit, VLAN_SUBNETm);
    soc_cm_sfree(unit, vstab);
    return rv;
}



/*
 * Function:
 *      _trx_vlan_subnet_entry_get
 * Purpose:
 *      Get vlan settings for given subnet
 * Parameters:
 *      unit -          (IN) BCM Device unit
 *      subnet_fields - (IN/OUT) Structure contains all entry fields
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_trx_vlan_subnet_entry_get(int unit,
                           _bcm_trx_vlan_subnet_entry_t *entry_fields)
{
    vlan_subnet_entry_t         *vstab, *vstabp;
    int                         entry_index, rv, nent;

    if (NULL == entry_fields) {
        return (BCM_E_PARAM);
    }
    soc_mem_lock(unit, VLAN_SUBNETm);

    rv = _tr_vlan_subnet_mem_read(unit, &vstab, &nent);
    if (BCM_FAILURE(rv)) {
        if (NULL != vstab) {
            soc_cm_sfree(unit, vstab);
        }
        soc_mem_unlock(unit, VLAN_SUBNETm);
        return rv;
    }
    
    rv = _trx_vlan_subnet_lookup(unit, vstab, nent, entry_fields->ip,
                                 entry_fields->mask, &entry_index, NULL);
    if (BCM_SUCCESS(rv)) {
        vstabp = soc_mem_table_idx_to_pointer(unit, VLAN_SUBNETm, 
                                              vlan_subnet_entry_t *, vstab,
                                              entry_index);
        _trx_vlan_subnet_entry_parse(unit, vstabp, entry_fields);
    }

    soc_cm_sfree(unit, vstab);
    soc_mem_unlock(unit, VLAN_SUBNETm);
    return (BCM_E_NONE);
}


/*
 * Function:
 *      _trx_vlan_subnet_entry_delete
 * Purpose:
 *      Delete vlan settings for given subnet
 * Parameters:
 *      unit -        (IN) BCM Device unit
 *      ip -          (IN) IP address
 *      mask -        (IN) IP mask
 * Returns:
 *      BCM_E_XXX
 */
static int
_trx_vlan_subnet_entry_delete(int unit, bcm_ip6_t ip, bcm_ip6_t mask)
{
    int                         rv, nent, i;
    vlan_subnet_entry_t         *vstab, *vstabp;
    uint32                      old_profile_idx;
    int                         entry_index, free_index;

    soc_mem_lock(unit, VLAN_SUBNETm);
    rv = _tr_vlan_subnet_mem_read(unit, &vstab, &nent);
    if (BCM_FAILURE(rv)) {
        if (NULL != vstab) {
            soc_cm_sfree(unit, vstab);
        }
        soc_mem_unlock(unit, VLAN_SUBNETm);
        return rv;
    }

    rv = _trx_vlan_subnet_lookup(unit, vstab, nent, ip, mask, &entry_index,
                                 &free_index);
    if (BCM_FAILURE(rv)) {
        soc_cm_sfree(unit, vstab);
        soc_mem_unlock(unit, VLAN_SUBNETm);
        return (rv);
    }

    /* Get the old vlan action profile pointer */
    vstabp = soc_mem_table_idx_to_pointer(unit, VLAN_SUBNETm,
                                          vlan_subnet_entry_t *, vstab,
                                          entry_index);
    old_profile_idx = soc_mem_field32_get(unit, VLAN_SUBNETm, vstabp,
                                          TAG_ACTION_PROFILE_PTRf);

    for (i = entry_index; i < free_index - 1; i++) {
        vstabp = soc_mem_table_idx_to_pointer(unit, VLAN_SUBNETm,
                                              vlan_subnet_entry_t *, vstab,
                                              (i + 1));
        rv = WRITE_VLAN_SUBNETm(unit, MEM_BLOCK_ANY, i, vstabp);
        if (BCM_FAILURE(rv)) {
            break;
        }
    }
    if (i >= free_index - 1) {
        rv = WRITE_VLAN_SUBNETm(unit, MEM_BLOCK_ANY, free_index - 1,
                                soc_mem_entry_null(unit, VLAN_SUBNETm));
        if (BCM_SUCCESS(rv)) {
            /* Delete the old vlan action profile entry */
            rv = _bcm_trx_vlan_action_profile_entry_delete(unit,
                                                           old_profile_idx);
        }
    }

    soc_mem_unlock(unit, VLAN_SUBNETm);
    soc_cm_sfree(unit, vstab);
    return rv;
}

/*
 * Function:
 *      _bcm_trx_vlan_ip_action_add
 * Purpose:
 *      Add a subnet lookup to select vlan and priority for
 *      untagged packets
 * Parameters:
 *      unit    -   device number
 *      vlan_ip -   structure specifying IP address and other info
 *      action  -   structure VLAN tag actions
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_ip_action_add(int unit, bcm_vlan_ip_t *vlan_ip,
                           bcm_vlan_action_set_t *action)
{
    int rv;
    _bcm_trx_vlan_subnet_entry_t entry_fields;

    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_verify(unit, action));

    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_ip_verify(unit, vlan_ip));

    sal_memset(&entry_fields, 0, sizeof(_bcm_trx_vlan_subnet_entry_t));
    BCM_IF_ERROR_RETURN(_trx_vlan_ip_addr_mask_get(vlan_ip, entry_fields.ip,
                                                   entry_fields.mask));

    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_profile_entry_add
                        (unit, action, (uint32 *)&entry_fields.profile_idx));

    entry_fields.ovid = action->new_outer_vlan;
    entry_fields.ivid = action->new_inner_vlan;
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        entry_fields.opri = action->priority;
        entry_fields.ocfi = action->new_outer_cfi;
        entry_fields.ipri = action->new_inner_pkt_prio;
        entry_fields.icfi = action->new_inner_cfi;
    } else {
        entry_fields.opri = action->priority;
    }

    rv = _trx_vlan_subnet_entry_add(unit, &entry_fields);
    if (BCM_FAILURE(rv)) {
        /* Add failed, back out the new profile entry */
        (void)_bcm_trx_vlan_action_profile_entry_delete
            (unit, entry_fields.profile_idx);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_trx_vlan_ip_action_get
 * Purpose:
 *      Helper funtion for the API, implementation for TRX
 * Parameters:
 *      unit    -   (IN) device number
 *      vlan_ip -   (IN) structure specifying IP address and other info
 *      action  -   (OUT) structure VLAN tag actions
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_ip_action_get(int unit, bcm_vlan_ip_t *vlan_ip,
                           bcm_vlan_action_set_t *action)
{
    _bcm_trx_vlan_subnet_entry_t    entry_fields;

    /* Parameter check */
    if (NULL == action) {
        return (BCM_E_PARAM);
    }
    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_ip_verify(unit, vlan_ip));

    sal_memset(&entry_fields, 0, sizeof(_bcm_trx_vlan_subnet_entry_t));
    BCM_IF_ERROR_RETURN(
        _trx_vlan_ip_addr_mask_get(vlan_ip, entry_fields.ip, entry_fields.mask));  
    
    BCM_IF_ERROR_RETURN(
        _trx_vlan_subnet_entry_get(unit, &entry_fields));

    action->new_outer_vlan = entry_fields.ovid; 
    action->new_inner_vlan = entry_fields.ivid;
    action->priority = entry_fields.opri;
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        action->new_outer_cfi = entry_fields.ocfi;
        action->new_inner_pkt_prio = entry_fields.ipri;
        action->new_inner_cfi = entry_fields.icfi;
    }

    _bcm_trx_vlan_action_profile_entry_get(unit, action, 
                                           entry_fields.profile_idx);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_trx_vlan_ip_action_traverse
 * Purpose:
 *      Helper funtion for the API, implementation for TRX
 * Parameters:
 *      unit    -   (IN) device number
 *      cb      -   (IN) user specified call back function
 *      user_data - (IN) pointer to user_data
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_ip_action_traverse(int unit, 
                                 bcm_vlan_ip_action_traverse_cb cb,
                                 void * user_data)
{
    vlan_subnet_entry_t         *vstab, *vstabp;
    int                         nent, rv, i;
    bcm_vlan_action_set_t       action;
    bcm_vlan_ip_t               vlan_ip;
    _bcm_trx_vlan_subnet_entry_t entry_fields;

    /* Input parameters check. */
    if (NULL == cb) {
        return (BCM_E_PARAM);
    }
    sal_memset(&entry_fields, 0, sizeof(_bcm_trx_vlan_subnet_entry_t));

    
    rv = _tr_vlan_subnet_mem_read(unit, &vstab, &nent);
    if (BCM_FAILURE(rv)) {
        if (NULL != vstab) {
            soc_cm_sfree(unit, vstab);
        }
        return rv;
    }
    
    for(i = 0; i < nent; i++) {
        sal_memset(&action, 0, sizeof(bcm_vlan_action_set_t));
        sal_memset(&vlan_ip, 0, sizeof(bcm_vlan_ip_t));
        vstabp = soc_mem_table_idx_to_pointer(unit, VLAN_SUBNETm,
                                              vlan_subnet_entry_t *, 
                                              vstab, i);
        if (0 == soc_VLAN_SUBNETm_field32_get(unit, vstabp, VALIDf)){
            continue;
        }
        _trx_vlan_subnet_entry_parse(unit, vstabp, &entry_fields);

        if (*(uint32*)entry_fields.ip == 0xffff0000) {
            /* IPv4 case */
            bcm_ip6_t   ip6tmp;

            vlan_ip.ip4 = (entry_fields.ip[4] << 24) |
                (entry_fields.ip[5] << 16) | (entry_fields.ip[6] << 8) |
                entry_fields.ip[7];
            bcm_ip6_mask_create(ip6tmp, entry_fields.prefix);
            vlan_ip.mask = (ip6tmp[4] << 24) | (ip6tmp[5] << 16) |
                (ip6tmp[6] << 8) | ip6tmp[7];
        } else {
            /* IPv6 case */
            sal_memcpy(vlan_ip.ip6, entry_fields.ip, sizeof(bcm_ip6_t));
            vlan_ip.prefix = entry_fields.prefix;
            vlan_ip.flags |= BCM_VLAN_SUBNET_IP6;
        }

        vlan_ip.prefix = entry_fields.prefix;
        vlan_ip.vid = entry_fields.ovid;
        action.new_outer_vlan = entry_fields.ovid;
        action.new_inner_vlan = entry_fields.ivid;
        action.priority = entry_fields.opri;
        if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            action.new_outer_cfi = entry_fields.ocfi;
            action.new_inner_pkt_prio = entry_fields.ipri;
            action.new_inner_cfi = entry_fields.icfi;
        }
         
         /* Read action profile data. */
        _bcm_trx_vlan_action_profile_entry_get(unit, &action, 
                                               entry_fields.profile_idx);
        /* Call traverse callback with the data. */
        rv = cb(unit, &vlan_ip, &action, user_data);
        if (BCM_FAILURE(rv)) {
            soc_cm_sfree(unit, vstab);
            return rv; 
        }
    }

    soc_cm_sfree(unit, vstab);
    return rv;

}

/*
 * Function:
 *      bcm_vlan_ip_delete
 * Purpose:
 *      Delete a subnet lookup entry.
 * Parameters:
 *      unit -          device number
 *      vlan_ip -       structure specifying IP address and other info
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_ip_delete(int unit, bcm_vlan_ip_t *vlan_ip)
{
    bcm_ip6_t   ip6addr, ip6mask;

    BCM_IF_ERROR_RETURN(
        _bcm_trx_vlan_ip_verify(unit, vlan_ip));

    BCM_IF_ERROR_RETURN(
        _trx_vlan_ip_addr_mask_get(vlan_ip, ip6addr, ip6mask));
    return _trx_vlan_subnet_entry_delete(unit, ip6addr, ip6mask); 
}

/*
 * Function:
 *      bcm_vlan_ip_delete_all
 * Purpose:
 *      Delete all subnet lookup entries.
 * Parameters:
 *      unit -          device number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_ip_delete_all(int unit)
{
    int i, nent,  rv;
    uint32 old_profile_idx;
    vlan_subnet_entry_t *vstab, *vsnull,  *vstabp;

    rv = _tr_vlan_subnet_mem_read(unit, &vstab, &nent);
    if (BCM_FAILURE(rv)) {
        if (NULL != vstab) {
            soc_cm_sfree(unit, vstab);
        }
        return rv;
    }

    vsnull = soc_mem_entry_null(unit, VLAN_SUBNETm);

    soc_mem_lock(unit, VLAN_SUBNETm);
    for(i = 0; i < nent; i++) {
        vstabp = soc_mem_table_idx_to_pointer(unit, VLAN_SUBNETm,
                        vlan_subnet_entry_t *, vstab, i);

        if (!soc_VLAN_SUBNETm_field32_get(unit, vstabp, VALIDf)) {
            continue;
        }
        old_profile_idx = soc_VLAN_SUBNETm_field32_get(unit, vstabp,
                                                       TAG_ACTION_PROFILE_PTRf);

        rv = WRITE_VLAN_SUBNETm(unit, MEM_BLOCK_ANY, i, vsnull);

        /* Increment the vlan action profile refcount for the cleared entry */
        _bcm_trx_vlan_action_profile_entry_increment(unit, ING_ACTION_PROFILE_DEFAULT);

        if (rv >= 0) {
            /* Delete the old vlan action profile entry */
            rv = _bcm_trx_vlan_action_profile_entry_delete(unit, old_profile_idx);
        }
    }

    soc_mem_unlock(unit, VLAN_SUBNETm);
    soc_cm_sfree(unit, vstab);
    return rv;
}

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int 
_bcm_tr3_vlan_mac_action_add(int                   unit,
                                 bcm_mac_t              mac,
                                 bcm_vlan_action_set_t *action) 
{
    int rv;
    uint32 profile_idx;
    vlan_xlate_entry_t vxent;
    
    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_verify(unit, action));
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        if (action->priority == -1) {
            return BCM_E_PARAM;  /* no default priority action to take*/
        }
    }
    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_action_profile_entry_add(unit, action, &profile_idx));

    sal_memset(&vxent, 0, sizeof(vxent));
    soc_mem_mac_addr_set(unit, VLAN_XLATEm, (uint32 *)(&vxent),
                       VLAN_MAC__MAC_ADDRf, mac);

    soc_VLAN_XLATEm_field32_set(unit, &vxent, KEY_TYPEf, 
             TR3_VLXLT_HASH_KEY_TYPE_VLAN_MAC);
    soc_VLAN_XLATEm_field32_set(unit, &vxent, VLAN_MAC__OVIDf, 
                                action->new_outer_vlan);
    soc_VLAN_XLATEm_field32_set(unit, &vxent, VLAN_MAC__IVIDf, 
                                action->new_inner_vlan);
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        soc_VLAN_XLATEm_field32_set(unit, &vxent, VLAN_MAC__OPRIf, 
                                    action->priority);
        soc_VLAN_XLATEm_field32_set(unit, &vxent, VLAN_MAC__OCFIf, 
                                    action->new_outer_cfi);
        soc_VLAN_XLATEm_field32_set(unit, &vxent, VLAN_MAC__IPRIf,
                                    action->new_inner_pkt_prio);
        soc_VLAN_XLATEm_field32_set(unit, &vxent, VLAN_MAC__ICFIf, 
                                    action->new_inner_cfi);
    } else {
        if (action->priority >= BCM_PRIO_MIN &&
            action->priority <= BCM_PRIO_MAX) {
            soc_VLAN_XLATEm_field32_set(unit, &vxent, VLAN_MAC__OPRIf, 
                                        action->priority);
        }
    }
    soc_VLAN_XLATEm_field32_set(unit, &vxent, VALIDf, 1);
    soc_VLAN_XLATEm_field32_set(unit, &vxent, 
                    VLAN_MAC__TAG_ACTION_PROFILE_PTRf, profile_idx);

    rv = soc_mem_insert_return_old(unit, VLAN_XLATEm, MEM_BLOCK_ALL, 
                                      &vxent, &vxent);

    if (rv == SOC_E_EXISTS) {
        /* Delete the old vlan action profile entry */
        profile_idx = soc_VLAN_XLATEm_field32_get(unit, &vxent,
                                      VLAN_MAC__TAG_ACTION_PROFILE_PTRf);
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
    }

    return rv;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

/*
 * Function:
 *      bcm_vlan_mac_action_add
 * Description   :
 *      Add association from MAC address to VLAN.
 *      If the entry already exists, update the action.
 * Parameters   :
 *      unit      (IN) BCM unit number
 *      mac       (IN) MAC address
 *      action    (IN) Action for outer and inner tag
 * Note:
 *   Program VLAN_XLATEm.
 */
int _bcm_trx_vlan_mac_action_add(int                   unit,
                                 bcm_mac_t              mac,
                                 bcm_vlan_action_set_t *action) 
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vlan_mac_action_add(unit,mac,action));
        return BCM_E_NONE;
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
    int rv;
    uint32 profile_idx;
    vlan_mac_entry_t vment;

    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_verify(unit, action));
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        if (action->priority == -1) {
            return BCM_E_PARAM;  /* no default priority action to take*/
        }
    }

    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_action_profile_entry_add(unit, action, &profile_idx));

    sal_memset(&vment, 0, sizeof(vment));
    soc_VLAN_MACm_mac_addr_set(unit, &vment, MAC_ADDRf, mac);
    soc_VLAN_MACm_field32_set(unit, &vment, KEY_TYPEf,
                              TR_VLXLT_HASH_KEY_TYPE_VLAN_MAC);
    soc_VLAN_MACm_field32_set(unit, &vment, OVIDf, action->new_outer_vlan);
    soc_VLAN_MACm_field32_set(unit, &vment, IVIDf, action->new_inner_vlan);
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        soc_VLAN_MACm_field32_set(unit, &vment, OPRIf, action->priority);
        soc_VLAN_MACm_field32_set(unit, &vment, OCFIf, action->new_outer_cfi);
        soc_VLAN_MACm_field32_set(unit, &vment, IPRIf,
                                  action->new_inner_pkt_prio);
        soc_VLAN_MACm_field32_set(unit, &vment, ICFIf, action->new_inner_cfi);
    } else {
        if (action->priority >= BCM_PRIO_MIN &&
            action->priority <= BCM_PRIO_MAX) {
            soc_VLAN_MACm_field32_set(unit, &vment, PRIf, action->priority);
        }
    }
    soc_VLAN_MACm_field32_set(unit, &vment, TAG_ACTION_PROFILE_PTRf, 
                              profile_idx);
    if (SOC_MEM_FIELD_VALID(unit, VLAN_MACm, VLAN_ACTION_VALIDf)) {
        soc_VLAN_MACm_field32_set(unit, &vment, VLAN_ACTION_VALIDf, 1);
    }
    soc_VLAN_MACm_field32_set(unit, &vment, VALIDf, 1);

    rv = soc_mem_insert_return_old(unit, VLAN_MACm, MEM_BLOCK_ALL, &vment, &vment);

    if (rv == SOC_E_EXISTS) {
        /* Delete the old vlan action profile entry */
        profile_idx = soc_VLAN_MACm_field32_get(unit, &vment,
                                                TAG_ACTION_PROFILE_PTRf);
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
    }

    return rv;
    }
}

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int 
_bcm_tr3_vlan_mac_action_get(int unit, bcm_mac_t  mac,
                             bcm_vlan_action_set_t *action) 
{
    vlan_xlate_entry_t vxent;             /* Lookup key hw buffer.      */
    vlan_xlate_entry_t res_vxent;         /* Lookup result buffer.      */
    uint32 profile_idx;                 /* Vlan action profile index. */
    int rv;                             /* Operation return status.   */
    int idx = 0;                        /* Lookup result entry index. */

    /* Input parameters check. */
    if (NULL == action) {
        return (BCM_E_PARAM);
    }

    /* Reset lookup key and result destination buffer. */
    sal_memset(&vxent, 0, sizeof(vlan_xlate_entry_t));
    sal_memset(&res_vxent, 0, sizeof(vlan_xlate_entry_t));

    /* Initialize lookup key. */
    soc_mem_mac_addr_set(unit, VLAN_XLATEm, (uint32 *)(&vxent),
                       VLAN_MAC__MAC_ADDRf, mac);

    soc_VLAN_XLATEm_field32_set(unit, &vxent, KEY_TYPEf,
                              TR3_VLXLT_HASH_KEY_TYPE_VLAN_MAC);

    /* Perform VLAN_MAC table search by mac address. */
    soc_mem_lock(unit, VLAN_XLATEm);
    rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &idx, 
                        &vxent, &res_vxent, 0);
    soc_mem_unlock(unit, VLAN_XLATEm);
    BCM_IF_ERROR_RETURN(rv);

    action->new_outer_vlan =
        soc_VLAN_XLATEm_field32_get(unit, &res_vxent, VLAN_MAC__OVIDf);
    action->new_inner_vlan =
        soc_VLAN_XLATEm_field32_get(unit, &res_vxent, VLAN_MAC__IVIDf);
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        action->priority = soc_VLAN_XLATEm_field32_get(unit, &res_vxent, 
                                    VLAN_MAC__OPRIf);
        action->new_outer_cfi =
            soc_VLAN_XLATEm_field32_get(unit, &res_vxent, VLAN_MAC__OCFIf);
        action->new_inner_pkt_prio =
            soc_VLAN_XLATEm_field32_get(unit, &res_vxent, VLAN_MAC__IPRIf);
        action->new_inner_cfi =
            soc_VLAN_XLATEm_field32_get(unit, &res_vxent, VLAN_MAC__ICFIf);
    } else {
        action->priority = soc_VLAN_XLATEm_field32_get(unit, &res_vxent, 
                                   VLAN_MAC__OPRIf);
    }

    /* Read action profile data. */
    profile_idx = soc_VLAN_XLATEm_field32_get(unit, &res_vxent, 
                                   VLAN_MAC__TAG_ACTION_PROFILE_PTRf);
    _bcm_trx_vlan_action_profile_entry_get(unit, action, profile_idx);

    return (BCM_E_NONE);
}
#endif /* BCM_TRIUMPH3_SUPPORT */

/*
 * Function:
 *      _bcm_trx_vlan_mac_action_get
 * Description   :
 *      Get association from MAC address to VLAN tag actions.
 * Parameters   :
 *      unit      (IN) BCM unit number
 *      mac       (IN) MAC address
 *      action    (OUT) Action for outer and inner tag
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_trx_vlan_mac_action_get(int unit, bcm_mac_t  mac,
                             bcm_vlan_action_set_t *action) 
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vlan_mac_action_get(unit,mac,action));
        return BCM_E_NONE;
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
    vlan_mac_entry_t vment;             /* Lookup key hw buffer.      */
    vlan_mac_entry_t res_vment;         /* Lookup result buffer.      */
    uint32 profile_idx;                 /* Vlan action profile index. */
    int rv;                             /* Operation return status.   */
    int idx = 0;                        /* Lookup result entry index. */

    /* Input parameters check. */
    if (NULL == action) {
        return (BCM_E_PARAM);
    }

    /* Reset lookup key and result destination buffer. */
    sal_memset(&vment, 0, sizeof(vlan_mac_entry_t));
    sal_memset(&res_vment, 0, sizeof(vlan_mac_entry_t));

    /* Initialize lookup key. */
    soc_VLAN_MACm_mac_addr_set(unit, &vment, MAC_ADDRf, mac);
    soc_VLAN_MACm_field32_set(unit, &vment, KEY_TYPEf,
                              TR_VLXLT_HASH_KEY_TYPE_VLAN_MAC);

    /* Perform VLAN_MAC table search by mac address. */
    soc_mem_lock(unit, VLAN_MACm);
    rv = soc_mem_search(unit, VLAN_MACm, MEM_BLOCK_ALL, &idx, 
                        &vment, &res_vment, 0);
    soc_mem_unlock(unit, VLAN_MACm);
    BCM_IF_ERROR_RETURN(rv);

    action->new_outer_vlan =
        soc_VLAN_MACm_field32_get(unit, &res_vment, OVIDf);
    action->new_inner_vlan =
        soc_VLAN_MACm_field32_get(unit, &res_vment, IVIDf);
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        action->priority = soc_VLAN_MACm_field32_get(unit, &res_vment, OPRIf);
        action->new_outer_cfi =
            soc_VLAN_MACm_field32_get(unit, &res_vment, OCFIf);
        action->new_inner_pkt_prio =
            soc_VLAN_MACm_field32_get(unit, &res_vment, IPRIf);
        action->new_inner_cfi =
            soc_VLAN_MACm_field32_get(unit, &res_vment, ICFIf);
    } else {
        action->priority = soc_VLAN_MACm_field32_get(unit, &res_vment, PRIf);
    }

    /* Read action profile data. */
    profile_idx =
        soc_VLAN_MACm_field32_get(unit, &res_vment, TAG_ACTION_PROFILE_PTRf);
    _bcm_trx_vlan_action_profile_entry_get(unit, action, profile_idx);

    return (BCM_E_NONE);
    }
}

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int
_bcm_tr3_vlan_mac_delete(int unit, bcm_mac_t mac)
{
    vlan_xlate_entry_t  vxent;
    int rv;
    uint32 profile_idx;

    sal_memset(&vxent, 0, sizeof(vxent));
    soc_mem_mac_addr_set(unit, VLAN_XLATEm, (uint32 *)(&vxent),
                       VLAN_MAC__MAC_ADDRf, mac);
    soc_VLAN_XLATEm_field32_set(unit, &vxent, KEY_TYPEf,
                              TR3_VLXLT_HASH_KEY_TYPE_VLAN_MAC);
    soc_mem_lock(unit, VLAN_XLATEm);
    rv = soc_mem_delete_return_old(unit, VLAN_XLATEm, MEM_BLOCK_ALL,
                                   &vxent, &vxent);
    soc_mem_unlock(unit, VLAN_XLATEm);
    if (rv == SOC_E_NOT_FOUND) {
        rv = SOC_E_NONE;
    } else if ((rv == SOC_E_NONE) && soc_VLAN_XLATEm_field32_get(unit, 
               &vxent, VALIDf)) {
        profile_idx = soc_VLAN_XLATEm_field32_get(unit, &vxent,
                                                TAG_ACTION_PROFILE_PTRf);
        /* Delete the old vlan action profile entry */
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
    }
    return rv;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

/*
 * Function:
 *      bcm_vlan_mac_delete
 * Purpose:
 *      Delete a vlan mac lookup entry.
 * Parameters:
 *      unit      (IN) BCM unit number
 *      mac       (IN) MAC address
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_mac_delete(int unit, bcm_mac_t mac) 
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vlan_mac_delete(unit,mac));
        return BCM_E_NONE;
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
    vlan_mac_entry_t  vment;
    int rv; 
    uint32 profile_idx;

    sal_memset(&vment, 0, sizeof(vment));
    soc_VLAN_MACm_mac_addr_set(unit, &vment, MAC_ADDRf, mac);
    soc_VLAN_MACm_field32_set(unit, &vment, KEY_TYPEf,
                              TR_VLXLT_HASH_KEY_TYPE_VLAN_MAC);
    soc_mem_lock(unit, VLAN_MACm);
    rv = soc_mem_delete_return_old(unit, VLAN_MACm, MEM_BLOCK_ALL,
                                   &vment, &vment);
    soc_mem_unlock(unit, VLAN_MACm);
    if (rv == SOC_E_NOT_FOUND) {
        rv = SOC_E_NONE;
    } else if ((rv == SOC_E_NONE) && soc_VLAN_MACm_field32_get(unit, &vment, VALIDf)) {
        profile_idx = soc_VLAN_MACm_field32_get(unit, &vment,
                                                TAG_ACTION_PROFILE_PTRf);       
        /* Delete the old vlan action profile entry */
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
    }
    return rv;
    }
}

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int
_bcm_tr3_vlan_mac_delete_all(int unit) 
{
    int i, imin, imax, nent, vmbytes, rv;
    uint32 old_profile_idx;
    vlan_xlate_entry_t *vxtab, *vmnull, *vxtabp;
    
    imin = soc_mem_index_min(unit, VLAN_XLATEm);
    imax = soc_mem_index_max(unit, VLAN_XLATEm);
    nent = soc_mem_index_count(unit, VLAN_XLATEm);
    vmbytes = soc_mem_entry_words(unit, VLAN_XLATEm);
    vmbytes = WORDS2BYTES(vmbytes);

    vxtab = soc_cm_salloc(unit, nent * vmbytes, "vlan_xlate");

    if (vxtab == NULL) {
        return BCM_E_MEMORY;
    }
    
    vmnull = soc_mem_entry_null(unit, VLAN_XLATEm);

    soc_mem_lock(unit, VLAN_XLATEm);
    rv = soc_mem_read_range(unit, VLAN_XLATEm, MEM_BLOCK_ANY,
                            imin, imax, vxtab);
    if (rv < 0) {
        soc_mem_unlock(unit, VLAN_XLATEm);
        soc_cm_sfree(unit, vxtab);
        return rv; 
    }
    
    for(i = 0; i < nent; i++) {
        vxtabp = soc_mem_table_idx_to_pointer(unit, VLAN_XLATEm,
                        vlan_xlate_entry_t *, vxtab, i);

        if (!soc_VLAN_XLATEm_field32_get(unit, vxtabp, VALIDf) ||
            (soc_VLAN_XLATEm_field32_get(unit, vxtabp, KEY_TYPEf) !=
             TR3_VLXLT_HASH_KEY_TYPE_VLAN_MAC)) {
            continue;
        }
        old_profile_idx = soc_VLAN_XLATEm_field32_get(unit, vxtabp,
                                                    TAG_ACTION_PROFILE_PTRf);

        rv = WRITE_VLAN_XLATEm(unit, MEM_BLOCK_ANY, i, vmnull);

        if (rv >= 0) {
            /* Delete the old vlan action profile entry */
            rv = _bcm_trx_vlan_action_profile_entry_delete(unit, old_profile_idx);
        }
    }
    
    soc_mem_unlock(unit, VLAN_XLATEm);
    soc_cm_sfree(unit, vxtab);
    return rv;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

/*
 * Function:
 *      bcm_vlan_mac_delete_all
 * Purpose:
 *      Delete all vlan mac lookup entries.
 * Parameters:
 *      unit      (IN) BCM unit number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_mac_delete_all(int unit) 
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vlan_mac_delete_all(unit));
        return BCM_E_NONE;
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
    int i, imin, imax, nent, vmbytes, rv;
    uint32 old_profile_idx;
    vlan_mac_entry_t *vmtab, *vmnull, *vmtabp;
    
    imin = soc_mem_index_min(unit, VLAN_MACm);
    imax = soc_mem_index_max(unit, VLAN_MACm);
    nent = soc_mem_index_count(unit, VLAN_MACm);
    vmbytes = soc_mem_entry_words(unit, VLAN_MACm);
    vmbytes = WORDS2BYTES(vmbytes);

    vmtab = soc_cm_salloc(unit, nent * vmbytes, "vlan_mac");

    if (vmtab == NULL) {
        return BCM_E_MEMORY;
    }
    
    vmnull = soc_mem_entry_null(unit, VLAN_MACm);

    soc_mem_lock(unit, VLAN_MACm);
    rv = soc_mem_read_range(unit, VLAN_MACm, MEM_BLOCK_ANY,
                            imin, imax, vmtab);
    if (rv < 0) {
        soc_mem_unlock(unit, VLAN_MACm);
        soc_cm_sfree(unit, vmtab);
        return rv; 
    }
    
    for(i = 0; i < nent; i++) {
        vmtabp = soc_mem_table_idx_to_pointer(unit, VLAN_MACm,
                        vlan_mac_entry_t *, vmtab, i);

        if (!soc_VLAN_MACm_field32_get(unit, vmtabp, VALIDf) ||
            (soc_VLAN_MACm_field32_get(unit, vmtabp, KEY_TYPEf) !=
             TR_VLXLT_HASH_KEY_TYPE_VLAN_MAC)) {
            continue;
        }
        old_profile_idx = soc_VLAN_MACm_field32_get(unit, vmtabp,
                                                    TAG_ACTION_PROFILE_PTRf);

        rv = WRITE_VLAN_MACm(unit, MEM_BLOCK_ANY, i, vmnull);

        if (rv >= 0) {
            /* Delete the old vlan action profile entry */
            rv = _bcm_trx_vlan_action_profile_entry_delete(unit, old_profile_idx);
        }
    }
    
    soc_mem_unlock(unit, VLAN_MACm);
    soc_cm_sfree(unit, vmtab);
    return rv;
    }
}

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int 
_bcm_tr3_vlan_mac_action_traverse(int unit, 
                                  bcm_vlan_mac_action_traverse_cb cb, 
                                  void *user_data)
{
    int idx, imin, imax, nent, vmbytes, rv;
    uint32 profile_idx;
    bcm_mac_t  mac;
    bcm_vlan_action_set_t action;
    vlan_xlate_entry_t * vxtab, *vxtabp;
    
    /* Input parameters check. */
    if (NULL == cb) {
        return (BCM_E_PARAM);
    }

    imin = soc_mem_index_min(unit, VLAN_XLATEm);
    imax = soc_mem_index_max(unit, VLAN_XLATEm);
    nent = soc_mem_index_count(unit, VLAN_XLATEm);
    vmbytes = soc_mem_entry_words(unit, VLAN_XLATEm);
    vmbytes = WORDS2BYTES(vmbytes);
    vxtab = soc_cm_salloc(unit, nent * sizeof(*vxtab), "vlan_xlate");

    if (vxtab == NULL) {
        return BCM_E_MEMORY;
    }
    
    soc_mem_lock(unit, VLAN_XLATEm);
    rv = soc_mem_read_range(unit, VLAN_XLATEm, MEM_BLOCK_ANY,
                            imin, imax, vxtab);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_XLATEm);
        soc_cm_sfree(unit, vxtab);
        return rv; 
    }
    
    for(idx = 0; idx < nent; idx++) {
        sal_memset(mac, 0, sizeof(bcm_mac_t));
        sal_memset(&action, 0, sizeof(bcm_vlan_action_set_t));
        vxtabp = soc_mem_table_idx_to_pointer(unit, VLAN_XLATEm,
                                              vlan_xlate_entry_t *, 
                                              vxtab, idx);

        if ((0 == soc_VLAN_XLATEm_field32_get(unit, vxtabp, VALIDf)) ||
            (TR3_VLXLT_HASH_KEY_TYPE_VLAN_MAC !=
             soc_VLAN_XLATEm_field32_get(unit, vxtabp, KEY_TYPEf))) {
            continue;
        }

        /* Get entry mac address. */
        soc_mem_mac_addr_get(unit, VLAN_XLATEm, (uint32 *)vxtabp, 
                     VLAN_MAC__MAC_ADDRf, mac);

        action.new_outer_vlan =
            soc_VLAN_XLATEm_field32_get(unit, vxtabp, VLAN_MAC__OVIDf);
        action.new_inner_vlan =
            soc_VLAN_XLATEm_field32_get(unit, vxtabp, VLAN_MAC__IVIDf);
        if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            action.priority = soc_VLAN_XLATEm_field32_get(unit, vxtabp, VLAN_MAC__OPRIf);
            action.new_outer_cfi =
                soc_VLAN_XLATEm_field32_get(unit, vxtabp, VLAN_MAC__OCFIf);
            action.new_inner_pkt_prio =
                soc_VLAN_XLATEm_field32_get(unit, vxtabp, VLAN_MAC__IPRIf);
            action.new_inner_cfi =
                soc_VLAN_XLATEm_field32_get(unit, vxtabp, VLAN_MAC__ICFIf);
        } else {
            action.priority =
                soc_VLAN_XLATEm_field32_get(unit, vxtabp, VLAN_MAC__OPRIf);
        }

        /* Read action profile data. */
        profile_idx =
            soc_VLAN_XLATEm_field32_get(unit, vxtabp, 
                                      VLAN_MAC__TAG_ACTION_PROFILE_PTRf);
        _bcm_trx_vlan_action_profile_entry_get(unit, &action, profile_idx);

        /* Call traverse callback with the data. */
        rv = cb(unit, mac, &action, user_data);
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, VLAN_XLATEm);
            soc_cm_sfree(unit, vxtab);
            return rv; 
        }
    }
    
    soc_mem_unlock(unit, VLAN_XLATEm);
    soc_cm_sfree(unit, vxtab);
    return rv;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

/*
 * Function:
 *      _bcm_trx_vlan_mac_action_traverse
 * Description   :
 *      Traverse over vlan mac actions, which are used for VLAN
 *      tag/s assignment to untagged packets.
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      cb              (IN) Call back function
 *      user_data       (IN) User provided data to pass to a call back
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_trx_vlan_mac_action_traverse(int unit, 
                                  bcm_vlan_mac_action_traverse_cb cb, 
                                  void *user_data)
{
#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vlan_mac_action_traverse(unit,cb,user_data));
        return BCM_E_NONE;
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
    int idx, imin, imax, nent, vmbytes, rv;
    uint32 profile_idx;
    bcm_mac_t  mac;
    bcm_vlan_action_set_t action;
    vlan_mac_entry_t * vmtab, *vmtabp;
    
    /* Input parameters check. */
    if (NULL == cb) {
        return (BCM_E_PARAM);
    }

    imin = soc_mem_index_min(unit, VLAN_MACm);
    imax = soc_mem_index_max(unit, VLAN_MACm);
    nent = soc_mem_index_count(unit, VLAN_MACm);
    vmbytes = soc_mem_entry_words(unit, VLAN_MACm);
    vmbytes = WORDS2BYTES(vmbytes);
    vmtab = soc_cm_salloc(unit, nent * sizeof(*vmtab), "vlan_mac");

    if (vmtab == NULL) {
        return BCM_E_MEMORY;
    }
    
    soc_mem_lock(unit, VLAN_MACm);
    rv = soc_mem_read_range(unit, VLAN_MACm, MEM_BLOCK_ANY,
                            imin, imax, vmtab);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, VLAN_MACm);
        soc_cm_sfree(unit, vmtab);
        return rv; 
    }
    
    for(idx = 0; idx < nent; idx++) {
        sal_memset(mac, 0, sizeof(bcm_mac_t));
        sal_memset(&action, 0, sizeof(bcm_vlan_action_set_t));
        vmtabp = soc_mem_table_idx_to_pointer(unit, VLAN_MACm,
                                              vlan_mac_entry_t *, 
                                              vmtab, idx);

        if ((0 == soc_VLAN_MACm_field32_get(unit, vmtabp, VALIDf)) ||
            (TR_VLXLT_HASH_KEY_TYPE_VLAN_MAC !=
             soc_VLAN_MACm_field32_get(unit, vmtabp, KEY_TYPEf))) {
            continue;
        }

        /* Get entry mac address. */
        soc_VLAN_MACm_mac_addr_get(unit, vmtabp, MAC_ADDRf, mac);

        action.new_outer_vlan =
            soc_VLAN_MACm_field32_get(unit, vmtabp, OVIDf);
        action.new_inner_vlan =
            soc_VLAN_MACm_field32_get(unit, vmtabp, IVIDf);
        if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            action.priority = soc_VLAN_MACm_field32_get(unit, vmtabp, OPRIf);
            action.new_outer_cfi =
                soc_VLAN_MACm_field32_get(unit, vmtabp, OCFIf);
            action.new_inner_pkt_prio =
                soc_VLAN_MACm_field32_get(unit, vmtabp, IPRIf);
            action.new_inner_cfi =
                soc_VLAN_MACm_field32_get(unit, vmtabp, ICFIf);
        } else {
            action.priority =
                soc_VLAN_MACm_field32_get(unit, vmtabp, PRIf);
        }

        /* Read action profile data. */
        profile_idx =
            soc_VLAN_MACm_field32_get(unit, vmtabp, TAG_ACTION_PROFILE_PTRf);
        _bcm_trx_vlan_action_profile_entry_get(unit, &action, profile_idx);

        /* Call traverse callback with the data. */
        rv = cb(unit, mac, &action, user_data);
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, VLAN_MACm);
            soc_cm_sfree(unit, vmtab);
            return rv; 
        }
    }
    
    soc_mem_unlock(unit, VLAN_MACm);
    soc_cm_sfree(unit, vmtab);
    return rv;
    }
}

/*
 * Function:
 *      _bcm_trx_vif_vlan_translate_entry_parse
 * Purpose:
 *      Parses vlan translate entry by given key_type, for inner and outer vlans
 * Parameters: 
 *      unit            (IN) BCM unit number
 *      mem             (IN) Vlan translate memory id.
 *      vent            (IN) vlan translate entry to parse
 *      action          (OUT) Action to fill
 * Returns:
 *      BCM_E_XXX
 */     
STATIC int
_bcm_trx_vif_vlan_translate_entry_parse(int unit, soc_mem_t mem, uint32 *vent,
                                   bcm_vlan_action_set_t *action)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        action->new_outer_vlan = soc_mem_field32_get(unit, VLAN_XLATEm,
                         vent, VIF__NEW_OVIDf);
        action->new_inner_vlan = soc_mem_field32_get (unit, VLAN_XLATEm,
                         vent, VIF__NEW_IVIDf);
    
        if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            action->priority = soc_mem_field32_get(unit, VLAN_XLATEm,
                                vent, VIF__NEW_OPRIf);
            action->new_outer_cfi = soc_mem_field32_get(unit, VLAN_XLATEm,
                     vent, VIF__NEW_OCFIf);
            action->new_inner_pkt_prio = soc_mem_field32_get(unit, VLAN_XLATEm,
                     vent, VIF__NEW_IPRIf);
            action->new_inner_cfi = soc_mem_field32_get(unit, VLAN_XLATEm,
                     vent, VIF__NEW_ICFIf);
        } else {
            action->priority = soc_mem_field32_get(unit, VLAN_XLATEm,
                           vent, VIF__PRIf);
        }
        return BCM_E_NONE;
    }
#endif   /* defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3) */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_trx_vlan_translate_entry_parse
 * Purpose:
 *      Parses vlan translate entry by given key_type, for inner and outer vlans
 * Parameters:
 *      unit            (IN) BCM unit number
 *      mem             (IN) Vlan translate memory id.
 *      vent            (IN) vlan translate entry to parse
 *      action          (OUT) Action to fill
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_trx_vlan_translate_entry_parse(int unit, soc_mem_t mem, uint32 *vent,
                                   bcm_vlan_action_set_t *action)
{
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    int offset = 0, index = 0;
#endif /* BCM_KATANA_SUPPORT or BCM_TRIUMPH3_SUPPORT */
    if ((NULL == vent) || (NULL == action) || (INVALIDm == mem)) {
        return BCM_E_PARAM;
    }

    action->new_outer_vlan = soc_mem_field32_get(unit, mem, vent, NEW_OVIDf);
    action->new_inner_vlan = soc_mem_field32_get (unit, mem, vent, NEW_IVIDf);

    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        action->priority = soc_mem_field32_get(unit, mem, vent, NEW_OPRIf);
        action->new_outer_cfi =
            soc_mem_field32_get(unit, mem, vent, NEW_OCFIf);
        action->new_inner_pkt_prio =
            soc_mem_field32_get(unit, mem, vent, NEW_IPRIf);
        action->new_inner_cfi =
            soc_mem_field32_get(unit, mem, vent, NEW_ICFIf);
    } else {
        action->priority = soc_mem_field32_get(unit, mem, vent, PRIf);
    }

    if (SOC_IS_TR_VL(unit) && (!SOC_IS_HURRICANEX(unit))) {
        if (SOC_MEM_FIELD_VALID(unit, mem, MPLS_ACTIONf)) {
            if (soc_mem_field32_get(unit, mem, vent, MPLS_ACTIONf) == 0x2) {
                action->ingress_if = 
                   soc_mem_field32_get(unit, mem, vent, L3_IIFf);
            }
        } 
    }
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)
    if (soc_feature(unit, soc_feature_global_meter)) {
        if (SOC_IS_KATANA(unit)) {
            if (SOC_MEM_FIELD_VALID(unit, mem, SVC_METER_OFFSET_MODEf)) {
                offset = soc_mem_field32_get(unit, mem, vent,
                                                  SVC_METER_OFFSET_MODEf);
            }
            if (SOC_MEM_FIELD_VALID(unit, mem, SVC_METER_INDEXf)) {
                index = soc_mem_field32_get(unit, mem, vent, SVC_METER_INDEXf);
            }
        } else if (SOC_IS_TRIUMPH3(unit)) {
            if (SOC_MEM_FIELD_VALID(unit, mem,
                                         XLATE__SVC_METER_OFFSET_MODEf)) {
                offset = soc_mem_field32_get(unit, mem, vent,
                                         XLATE__SVC_METER_OFFSET_MODEf);
            }
            if (SOC_MEM_FIELD_VALID(unit, mem, XLATE__SVC_METER_INDEXf)) {
                index = soc_mem_field32_get(unit, mem, vent, 
                                                  XLATE__SVC_METER_INDEXf);
            }
        }
        _bcm_esw_get_policer_id_from_index_offset(unit, index, offset, 
                                                     &action->policer_id);
     }
#endif /* BCM_KATANA_SUPPORT or BCM_TRIUMPH3_SUPPORT */

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_trx_vif_vlan_translate_entry_assemble
 * Purpose:
 *      Constructs vif vlan translate entry from given key_type, inner and outer vlans
 * Parameters:
 *      unit            (IN) BCM unit number
 *      vent            (IN/OUT) vlan translate entry to construct
 *      key_type        (IN) Key Type : bcmVlanTranslateKey*
 *      inner_vlan      (IN) inner VLAN ID
 *      outer_vlan      (IN) outer VLAN ID
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vif_vlan_translate_entry_assemble(int unit, vlan_xlate_entry_t *vent,
                                      bcm_gport_t niv_port_id,
                                      bcm_vlan_translate_key_t key_type,
                                      bcm_vlan_t inner_vlan,
                                      bcm_vlan_t outer_vlan)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        int             tmp_id;
        bcm_module_t    mod_out;
        bcm_port_t      port_out;
        bcm_trunk_t     trunk_out;
        int key_type_value;
        bcm_niv_port_t niv_port;
    
        if (!BCM_GPORT_IS_NIV_PORT(niv_port_id)) {
            return BCM_E_PORT;
        }
    
        switch (key_type) {
            case bcmVlanTranslateKeyPortOuterTag:
                key_type = VLXLT_HASH_KEY_TYPE_VIF_OTAG;
                soc_VLAN_XLATEm_field32_set(unit, vent, VIF__OTAGf, outer_vlan);
                break;
    
            case bcmVlanTranslateKeyPortInnerTag:
                key_type = VLXLT_HASH_KEY_TYPE_VIF_ITAG;
                soc_VLAN_XLATEm_field32_set(unit, vent, VIF__ITAGf, inner_vlan);
                break;
    
            case bcmVlanTranslateKeyPortOuter:
                key_type = VLXLT_HASH_KEY_TYPE_VIF_VLAN;
                soc_VLAN_XLATEm_field32_set(unit, vent, VIF__VLANf, outer_vlan);
                break;
    
            case bcmVlanTranslateKeyPortInner:
                key_type = VLXLT_HASH_KEY_TYPE_VIF_CVLAN;
                soc_VLAN_XLATEm_field32_set(unit, vent, VIF__CVLANf,inner_vlan);
                break;
    
            default:
                return BCM_E_PARAM;
        }
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                   key_type, &key_type_value));
        soc_VLAN_XLATEm_field32_set(unit, vent, KEY_TYPEf, key_type_value);
    
        niv_port.niv_port_id = niv_port_id;
        BCM_IF_ERROR_RETURN(bcm_esw_niv_port_get(unit,&niv_port));
        if (niv_port.flags & BCM_NIV_PORT_MATCH_NONE) {
            return BCM_E_PARAM;
        }
    
        soc_VLAN_XLATEm_field32_set(unit, vent, VIF__SRC_VIFf, 
                           niv_port.virtual_interface_id);
    
        if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
            soc_VLAN_XLATEm_field32_set(unit, vent, SOURCE_TYPEf, 1);
        }
        BCM_IF_ERROR_RETURN(_bcm_esw_gport_resolve(unit, niv_port.port, 
                      &mod_out, &port_out, &trunk_out, &tmp_id));
        if (BCM_GPORT_IS_TRUNK(niv_port.port)) {
            soc_VLAN_XLATEm_field32_set(unit, vent, VIF__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, vent, VIF__TGIDf, trunk_out);
        } else {
            soc_VLAN_XLATEm_field32_set(unit, vent, VIF__MODULE_IDf, mod_out);
            soc_VLAN_XLATEm_field32_set(unit, vent, VIF__PORT_NUMf, port_out);
        }
        return BCM_E_NONE;
    }
#endif   /* defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3) */
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      _bcm_trx_vlan_translate_entry_assemble
 * Purpose:
 *      Constructs vlan translate entry from given key_type, inner and outer vlans
 * Parameters:
 *      unit            (IN) BCM unit number
 *      vent            (IN/OUT) vlan translate entry to construct
 *      key_type        (IN) Key Type : bcmVlanTranslateKey*
 *      inner_vlan      (IN) inner VLAN ID
 *      outer_vlan      (IN) outer VLAN ID
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_trx_vlan_translate_entry_assemble(int unit, vlan_xlate_entry_t *vent,
                                      bcm_gport_t port,
                                      bcm_vlan_translate_key_t key_type,
                                      bcm_vlan_t inner_vlan, 
                                      bcm_vlan_t outer_vlan)
{
    int             use_glp = 1;    /* By default incoming port used in lookup key */
    int             tmp_id;
    bcm_module_t    mod_out;
    bcm_port_t      port_out;
    bcm_trunk_t     trunk_out;
    int key_type_value;

    switch (key_type) {
        case bcmVlanTranslateKeyDouble:
            use_glp = 0;
            /* Fall through */
        case bcmVlanTranslateKeyPortDouble:
            
            VLAN_CHK_ID(unit, inner_vlan);
            VLAN_CHK_ID(unit, outer_vlan);
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_IVID_OVID,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, vent, OVIDf, outer_vlan);
            soc_VLAN_XLATEm_field32_set(unit, vent, IVIDf, inner_vlan);
            break;
        case bcmVlanTranslateKeyOuterTag:
            use_glp = 0;
            /* Fall through */
        case bcmVlanTranslateKeyPortOuterTag:
            /* allow 16 bit VLAN id */
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_OTAG,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, vent, OTAGf, outer_vlan);
            break;
        case bcmVlanTranslateKeyInnerTag:
            use_glp = 0;
            /* Fall through */
        case bcmVlanTranslateKeyPortInnerTag:
            /* Allow 16 bit VLAN id */
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_ITAG,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, vent, ITAGf, inner_vlan);
            break;
        case bcmVlanTranslateKeyOuter:
            use_glp = 0;
            /* Fall through */
        case bcmVlanTranslateKeyPortOuter:
            VLAN_CHK_ID(unit, outer_vlan);
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_OVID,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, vent, OVIDf, outer_vlan);
            break;
        case bcmVlanTranslateKeyInner:
            use_glp = 0;
            /* Fall through */
        case bcmVlanTranslateKeyPortInner:
            VLAN_CHK_ID(unit, inner_vlan);
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_IVID,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, vent, IVIDf, inner_vlan);
            break;
        default:
            return BCM_E_PARAM;
    }

    if (use_glp) {
#if defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)        
        if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
            soc_VLAN_XLATEm_field32_set(unit, vent, SOURCE_TYPEf, 1);
        }
#endif
        BCM_IF_ERROR_RETURN
            (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out,
                                    &trunk_out, &tmp_id));
        if (BCM_GPORT_IS_TRUNK(port)) {
            soc_VLAN_XLATEm_field32_set(unit, vent, Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, vent, TGIDf, trunk_out);
        } else {
            soc_VLAN_XLATEm_field32_set(unit, vent, MODULE_IDf, mod_out);
            soc_VLAN_XLATEm_field32_set(unit, vent, PORT_NUMf, port_out);
        }
    } else {
        /* SOURCE_TYPE field in vent is 0 */

        /* incoming port not used in lookup key, set field to all 1's */
        soc_VLAN_XLATEm_field32_set(unit, vent, GLPf, 
                                    SOC_VLAN_XLATE_GLP_WILDCARD(unit));
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *		_bcm_trx_vlan_translate_action_entry_update
 * Purpose:
 *		Update Vlan_xlate Entry.
 *              Currently it only updates the fields by the actions
 *              specified in bcm_vlan_action_set_t.
 * Parameters:
 *  IN :  Unit
 *  IN :  vlan_xlate_entry
 *  OUT :  vlan_xlate_entry

 */

STATIC int
_bcm_trx_vlan_translate_action_entry_update(int unit, vlan_xlate_entry_t *vent, 
                                                 vlan_xlate_entry_t *return_ent)
{
    uint32  key_type, value;
    bcm_vlan_t outer_vlan, inner_vlan;
#if defined(BCM_KATANA_SUPPORT)
    bcm_policer_t  current_policer = 0;
    int rv = BCM_E_NONE;
#endif
    /* Check if Key_Type identical */
    key_type = soc_VLAN_XLATEm_field32_get(unit, vent, KEY_TYPEf);
    value = soc_VLAN_XLATEm_field32_get(unit, return_ent, KEY_TYPEf);
    if (key_type != value) {
         return BCM_E_PARAM;
    }

    /* Retain original Keys -- Update data only */
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        value = soc_VLAN_XLATEm_field32_get(unit, vent, NEW_OPRIf);
        soc_VLAN_XLATEm_field32_set(unit, return_ent, NEW_OPRIf, value);
        value = soc_VLAN_XLATEm_field32_get(unit, vent, NEW_OCFIf);
        soc_VLAN_XLATEm_field32_set(unit, return_ent, NEW_OCFIf, value);
        value = soc_VLAN_XLATEm_field32_get(unit, vent, NEW_IPRIf);
        soc_VLAN_XLATEm_field32_set(unit, return_ent, NEW_IPRIf, value);
        value = soc_VLAN_XLATEm_field32_get(unit, vent, NEW_ICFIf);
        soc_VLAN_XLATEm_field32_set(unit, return_ent, NEW_ICFIf, value);
    } else {
        value = soc_VLAN_XLATEm_field32_get(unit, vent, RPEf);
        soc_VLAN_XLATEm_field32_set(unit, return_ent, RPEf, value);
        if (value) {
            value = soc_VLAN_XLATEm_field32_get(unit, vent, PRIf);
            soc_VLAN_XLATEm_field32_set(unit, return_ent, PRIf, value);
        } 
    }

    value = soc_VLAN_XLATEm_field32_get(unit, vent, 
                                       TAG_ACTION_PROFILE_PTRf);
    soc_VLAN_XLATEm_field32_set(unit, return_ent, 
                                       TAG_ACTION_PROFILE_PTRf, value);
    inner_vlan = soc_VLAN_XLATEm_field32_get(unit, vent, NEW_IVIDf);
    soc_VLAN_XLATEm_field32_set(unit, return_ent, NEW_IVIDf, inner_vlan);
    outer_vlan = soc_VLAN_XLATEm_field32_get(unit, vent, NEW_OVIDf);
    soc_VLAN_XLATEm_field32_set(unit, return_ent, NEW_OVIDf, outer_vlan);
#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANAX(unit)) {
        rv = _bcm_esw_get_policer_from_table(unit, VLAN_XLATEm, 0, return_ent, 
                                                    &current_policer, 1);
        /* decrement current policer ref count - if any */
        if ((current_policer & BCM_POLICER_GLOBAL_METER_INDEX_MASK) > 0) {
            rv = _bcm_esw_policer_decrement_ref_count(unit, current_policer);
            BCM_IF_ERROR_RETURN(rv);
        }
        value = soc_VLAN_XLATEm_field32_get(unit, vent, SVC_METER_OFFSET_MODEf);
        soc_VLAN_XLATEm_field32_set(unit, return_ent, 
                                                SVC_METER_OFFSET_MODEf, value);
        value = soc_VLAN_XLATEm_field32_get(unit, vent, SVC_METER_INDEXf);
        soc_VLAN_XLATEm_field32_set(unit, return_ent, SVC_METER_INDEXf, value);
    }
#endif /* BCM_KATANA_SUPPORT*/

   return BCM_E_NONE;
}

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int
_bcm_tr3_vxlate_extd_action_entry_update(int unit,
               vlan_xlate_entry_t *vent_in,  /* update entry */
               void *ctxt,
               vlan_xlate_entry_t *vent_out, /* entry read from mem */
               vlan_xlate_extd_entry_t *vxent_out, /* entry read from mem or
                                   converted from vent_out if not null */
               int *use_extd_tbl) 
{
    vlan_xlate_entry_t vent_new;
    bcm_vlan_action_set_t *action;
#ifdef INCLUDE_L3
    int intf_map_mode = 0;
    int min_l3_iif;
    int max_l3_iif;
#endif

    *use_extd_tbl = FALSE;
    action = (bcm_vlan_action_set_t *)ctxt;
    sal_memset(&vent_new, 0, sizeof(vent_new));

    /* if a regular entry from memory table is given, then update it */
    if (vent_out) {
        BCM_IF_ERROR_RETURN(
            _bcm_trx_vlan_translate_action_entry_update (unit,
                    vent_in, vent_out));

        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vxlate2vxlate_extd(unit,vent_out,vxent_out));
    } else {
        /* if a regular entry isn't given instead a extd entry, then
         * first convert to a regular entry, update it and convert back
         * to a extd entry. This is to utilize the existing regular entry
         * update function 
         */
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vxlate_extd2vxlate(unit,vxent_out,&vent_new,0));
        BCM_IF_ERROR_RETURN(
            _bcm_trx_vlan_translate_action_entry_update (unit,
                    vent_in, &vent_new));
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vxlate2vxlate_extd(unit,&vent_new,vxent_out));
    }
#ifdef INCLUDE_L3
    max_l3_iif = BCM_XGS3_L3_ING_IF_TBL_SIZE(unit);

#if defined(BCM_TRIUMPH_SUPPORT)
    if (soc_feature(unit, soc_feature_l3_ingress_interface)) {
        BCM_IF_ERROR_RETURN (bcm_xgs3_l3_ingress_intf_map_get(unit,
                           &intf_map_mode));
    }
#endif

    if (intf_map_mode) {
        min_l3_iif = 0;
    } else {
        min_l3_iif = BCM_VLAN_MAX + 1;
    }

    if ((action->ingress_if >= min_l3_iif) &&
                      (action->ingress_if < max_l3_iif)) {
        *use_extd_tbl = TRUE;
        soc_VLAN_XLATE_EXTDm_field32_set(unit, vxent_out,
                                    XLATE__MPLS_ACTIONf, 0x2); /* L3_IIF */
        soc_VLAN_XLATE_EXTDm_field32_set(unit, vxent_out, XLATE__L3_IIFf,
                                    action->ingress_if);
    }
#endif  /* INCLUDE_L3 */

    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_add_policer_to_table(unit, action->policer_id,
                                     VLAN_XLATE_EXTDm, 0, vxent_out));
        if (action->policer_id > 0 ) {
            *use_extd_tbl = TRUE;
        } 
    } 
    return BCM_E_NONE;
}

/*
 * Function:
 *		_bcm_tr3_vlan_translate_action_entry_set
 * Purpose:
 *		Set Vlan Translate Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  vlan_xlate_entry
 * Returns:
 *		BCM_E_XXX
 */

STATIC int
_bcm_tr3_vlan_translate_action_entry_set(int unit, vlan_xlate_entry_t *vent,
                            bcm_vlan_action_set_t *action)
{

    BCM_IF_ERROR_RETURN(
         _bcm_tr3_vxlate_entry_add(unit, vent,(void *)action,
                _bcm_tr3_vxlate_extd_action_entry_update));
    return BCM_E_NONE;
}
#endif /* BCM_TRIUMPH3_SUPPORT */

/*
 * Function:
 *		_bcm_trx_vlan_translate_action_entry_set
 * Purpose:
 *		Set Vlan Translate Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  vlan_xlate_entry
 * Returns:
 *		BCM_E_XXX
 */

STATIC int
_bcm_trx_vlan_translate_action_entry_set(int unit, vlan_xlate_entry_t *vent)
{
    vlan_xlate_entry_t return_vent;
    int rv = BCM_E_UNAVAIL;
    int index;

    rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                        vent, &return_vent, 0);
	
    if (rv == SOC_E_NONE) {
         BCM_IF_ERROR_RETURN(
             _bcm_trx_vlan_translate_action_entry_update (unit, 
                     vent, &return_vent));
         rv = soc_mem_write(unit, VLAN_XLATEm,
                                           MEM_BLOCK_ALL, index,
                                           &return_vent);
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    } else {
         rv = soc_mem_insert(unit, VLAN_XLATEm, MEM_BLOCK_ALL, vent);
    }

    return rv;
}


/*
 * Function   :
 *      _bcm_trx_vif_vlan_translate_action_add
 * Description   :
 *      Add an entry to ingress VLAN translation table.
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      port            (IN) Ingress generic port
 *      key_type        (IN) Key Type : bcmVlanTranslateKey*
 *      outer_vlan      (IN) Packet outer VLAN ID
 *      inner_vlan      (IN) Packet inner VLAN ID
 *      action          (IN) Action for outer and inner tag
 */
STATIC int 
_bcm_trx_vif_vlan_translate_action_add(int unit,
                                  bcm_gport_t port,
                                  bcm_vlan_translate_key_t key_type,
                                  bcm_vlan_t outer_vlan,
                                  bcm_vlan_t inner_vlan,
                                  bcm_vlan_action_set_t *action)
{
#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv)) {
        uint32 profile_idx; 
        int rv = BCM_E_NONE;
        int search_rv;
        vlan_xlate_entry_t vent;
        vlan_xlate_entry_t search_vent;
        int key_type_value;
        int old_profile_idx = 0;
        int index;
    
        BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_verify(unit, action));
    
        sal_memset(&vent, 0, sizeof(vent));
        BCM_IF_ERROR_RETURN(_bcm_trx_vif_vlan_translate_entry_assemble(unit, 
                &vent, port, key_type, inner_vlan, outer_vlan));
    
        sal_memcpy(&search_vent, &vent, sizeof(vlan_xlate_entry_t));
     
        /* NIV port vxlate key type */
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_VIF, &key_type_value));
        soc_VLAN_XLATEm_field32_set(unit, &search_vent, KEY_TYPEf, 
                                          key_type_value);
        soc_VLAN_XLATEm_field32_set(unit, &search_vent, VIF__VLANf, 0); 
    
        search_rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                            &search_vent, &vent, 0);
    
        /* an entry should already exist */
        if (search_rv != SOC_E_NONE) {
            return search_rv;
        }
    
        /* re-initialize the key */
        BCM_IF_ERROR_RETURN(_bcm_trx_vif_vlan_translate_entry_assemble(unit, 
               &vent, port, key_type, inner_vlan, outer_vlan));
        sal_memcpy(&search_vent, &vent, sizeof(vlan_xlate_entry_t));
        search_rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                            &search_vent, &vent, 0);
    
        if (!(search_rv == SOC_E_NONE || search_rv == SOC_E_NOT_FOUND)) {
            return search_rv;
        }
        if (search_rv == SOC_E_NONE) {
            old_profile_idx = soc_VLAN_XLATEm_field32_get(unit, &vent, 
                                   VIF__TAG_ACTION_PROFILE_PTRf);
        }
    
        /* update or fill in the vlan xlate entry*/
        BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_profile_entry_add(unit, 
                       action, &profile_idx));
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__TAG_ACTION_PROFILE_PTRf,
                                    profile_idx);
        if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, VIF__VLAN_ACTION_VALIDf)) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__VLAN_ACTION_VALIDf, 1);
        }
    
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__NEW_IVIDf, 
                                    action->new_inner_vlan);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__NEW_OVIDf, 
                                    action->new_outer_vlan);
        if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
            if (action->priority >= BCM_PRIO_MIN &&
                action->priority <= BCM_PRIO_MAX) {
                soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__NEW_OPRIf, 
                          action->priority);
            }
            soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__NEW_OCFIf,
                                        action->new_outer_cfi);
            soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__NEW_IPRIf,
                                        action->new_inner_pkt_prio);
            soc_VLAN_XLATEm_field32_set(unit, &vent, VIF__NEW_ICFIf,
                                        action->new_inner_cfi);
        } 
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
    
        if (search_rv == SOC_E_NONE) {
            /* overrides existing one */
            rv = soc_mem_write(unit, VLAN_XLATEm,
                                           MEM_BLOCK_ALL, index,
                                           &vent);
        } else { /* case search_rv == SOC_E_NOT_FOUND */
           rv = soc_mem_insert(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vent);
        }
    
        if (BCM_FAILURE(rv)) {
            /* Delete the allocated vlan translate profile entry */
            profile_idx = soc_VLAN_XLATEm_field32_get(unit, &vent,
                                             VIF__TAG_ACTION_PROFILE_PTRf);
            (void) _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
        } else {
            /* delete t the old profile entry */
            if (search_rv == SOC_E_NONE) {
                rv = _bcm_trx_vlan_action_profile_entry_delete(unit,
                         old_profile_idx);
            }
        }
        return rv;
    }
#endif   /* defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3) */
    return BCM_E_UNAVAIL;

}

/*
 * Function   :
 *      _bcm_trx_vlan_translate_action_add
 * Description   :
 *      Add an entry to ingress VLAN translation table.
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      port            (IN) Ingress generic port
 *      key_type        (IN) Key Type : bcmVlanTranslateKey*
 *      outer_vlan      (IN) Packet outer VLAN ID
 *      inner_vlan      (IN) Packet inner VLAN ID
 *      action          (IN) Action for outer and inner tag
 */
int 
_bcm_trx_vlan_translate_action_add(int unit,
                                  bcm_gport_t port,
                                  bcm_vlan_translate_key_t key_type,
                                  bcm_vlan_t outer_vlan,
                                  bcm_vlan_t inner_vlan,
                                  bcm_vlan_action_set_t *action)
{
    uint32 profile_idx; 
    int rv;
    vlan_xlate_entry_t vent;

    /* process the NIV type of gport */
    if (BCM_GPORT_IS_NIV_PORT(port)) {
        int rv;
        rv = _bcm_trx_vif_vlan_translate_action_add(unit,
             port,key_type,outer_vlan,inner_vlan,action);
        return rv;
    }

    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_verify(unit, action));

    sal_memset(&vent, 0, sizeof(vent));
    BCM_IF_ERROR_RETURN(
        _bcm_trx_vlan_translate_entry_assemble(unit, &vent, port, key_type,
                                               inner_vlan, outer_vlan));

    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_action_profile_entry_add(unit, action, &profile_idx));
    soc_VLAN_XLATEm_field32_set(unit, &vent, TAG_ACTION_PROFILE_PTRf,
                                profile_idx);
    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, VLAN_ACTION_VALIDf)) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, VLAN_ACTION_VALIDf, 1);
    }

    soc_VLAN_XLATEm_field32_set(unit, &vent, NEW_IVIDf, 
                                action->new_inner_vlan);
    soc_VLAN_XLATEm_field32_set(unit, &vent, NEW_OVIDf, 
                                action->new_outer_vlan);
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        if (action->priority >= BCM_PRIO_MIN &&
            action->priority <= BCM_PRIO_MAX) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, NEW_OPRIf, action->priority);
        }
        soc_VLAN_XLATEm_field32_set(unit, &vent, NEW_OCFIf,
                                    action->new_outer_cfi);
        soc_VLAN_XLATEm_field32_set(unit, &vent, NEW_IPRIf,
                                    action->new_inner_pkt_prio);
        soc_VLAN_XLATEm_field32_set(unit, &vent, NEW_ICFIf,
                                    action->new_inner_cfi);
    } else {
        if (action->priority >= BCM_PRIO_MIN &&
            action->priority <= BCM_PRIO_MAX) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, RPEf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, PRIf, action->priority);
        }
    }
    soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        rv = _bcm_tr3_vlan_translate_action_entry_set(unit, &vent,action);
        if (BCM_FAILURE(rv)) {
            /* Delete the old vlan translate profile entry */
            profile_idx = soc_VLAN_XLATEm_field32_get(unit, &vent,
                                                  TAG_ACTION_PROFILE_PTRf);
            (void) _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
        }
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
#ifdef INCLUDE_L3
    if (SOC_IS_TR_VL(unit) && (!SOC_IS_HURRICANEX(unit))) {
        if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, MPLS_ACTIONf)) {
            int intf_map_mode = 0; 
            int min_l3_iif; 
            int max_l3_iif; 

            max_l3_iif = BCM_XGS3_L3_ING_IF_TBL_SIZE(unit); 
#if defined(BCM_TRIUMPH_SUPPORT)
            if (soc_feature(unit, soc_feature_l3_ingress_interface)) {
                BCM_IF_ERROR_RETURN (bcm_xgs3_l3_ingress_intf_map_get(unit,
                                   &intf_map_mode));
    }
#endif

            if (intf_map_mode) { 
                min_l3_iif = 0;
            } else {
                min_l3_iif = BCM_VLAN_MAX + 1;
            }
            
            if ((action->ingress_if >= min_l3_iif) && 
                      (action->ingress_if < max_l3_iif)) {
                soc_VLAN_XLATEm_field32_set(unit, &vent, 
                                            MPLS_ACTIONf, 0x2); /* L3_IIF */
                soc_VLAN_XLATEm_field32_set(unit, &vent, L3_IIFf, 
                                            action->ingress_if);
            } 
        }
    }
#endif /* INCLUDE_L3 */

#if defined(BCM_KATANA_SUPPORT) 
    if (SOC_IS_KATANAX(unit)) {
        rv = _bcm_esw_add_policer_to_table(unit, action->policer_id,
                                           VLAN_XLATEm,0, &vent);
        BCM_IF_ERROR_RETURN(rv);
    }
#endif /* BCM_KATANA_SUPPORT */
    rv = _bcm_trx_vlan_translate_action_entry_set(unit, &vent);

    if (BCM_FAILURE(rv)) {
        /* Delete the old vlan translate profile entry */
        profile_idx = soc_VLAN_XLATEm_field32_get(unit, &vent,
                                                  TAG_ACTION_PROFILE_PTRf);
        (void) _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
    }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_trx_vlan_translate_get
 * Purpose:
 *      Gets a vlan translate entry.
 * Parameters:
 *      unit            (IN) BCM unit number
 *      port            (IN) Ingress generic port
 *      key_type        (IN) Key Type : bcmVlanTranslateKey*
 *      old_vlan        (IN) Packet old VLAN ID
 *      new_vlan        (OUT) Packet new VLAN ID
 *      prio            (OUT) Packet prio
 */
int 
_bcm_trx_vlan_translate_action_get (int unit, 
                                   bcm_gport_t port, 
                                   bcm_vlan_translate_key_t key_type, 
                                   bcm_vlan_t outer_vid,
                                   bcm_vlan_t inner_vid,
                                   bcm_vlan_action_set_t *action)
{
    vlan_xlate_entry_t  vent, res_vent;
    int                 rv;
    int                 idx = 0;
    uint32              profile_idx = 0;

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));
    sal_memset(&res_vent, 0, sizeof(vlan_xlate_entry_t));
    if (BCM_GPORT_IS_NIV_PORT(port)) {
        BCM_IF_ERROR_RETURN(
            _bcm_trx_vif_vlan_translate_entry_assemble(unit, &vent, port, 
                         key_type,inner_vid, outer_vid));

        rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &idx, 
                         &vent, &res_vent, 0);
        BCM_IF_ERROR_RETURN(rv);
        profile_idx = soc_VLAN_XLATEm_field32_get(unit, &res_vent,
                                      VIF__TAG_ACTION_PROFILE_PTRf);       
        _bcm_trx_vlan_action_profile_entry_get(unit, action, profile_idx);
        return _bcm_trx_vif_vlan_translate_entry_parse(unit, VLAN_XLATEm,  
                                               (uint32 *)&res_vent, action);  
    } 
    BCM_IF_ERROR_RETURN(
        _bcm_trx_vlan_translate_entry_assemble(unit, &vent, port, key_type, 
                                               inner_vid, outer_vid));
    soc_mem_lock(unit, VLAN_XLATEm);
    rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &idx, 
                         &vent, &res_vent, 0);
    soc_mem_unlock(unit, VLAN_XLATEm);

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        vlan_xlate_extd_entry_t vxent;
        vlan_xlate_extd_entry_t res_vxent;

        /* check the extd table */
        if (rv == SOC_E_NOT_FOUND) {
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_vxlate2vxlate_extd(unit,&vent,&vxent));

            soc_mem_lock(unit, VLAN_XLATE_EXTDm);
            rv = soc_mem_search(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL, &idx, 
                            &vxent, &res_vxent, 0);
            soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
            BCM_IF_ERROR_RETURN(rv);
            profile_idx = soc_VLAN_XLATE_EXTDm_field32_get(unit, &res_vxent,
                                          TAG_ACTION_PROFILE_PTRf);       
            _bcm_trx_vlan_action_profile_entry_get(unit, action, profile_idx);

             return _bcm_trx_vlan_translate_entry_parse(unit, VLAN_XLATE_EXTDm,
                                               (uint32 *)&res_vxent, action);  
            
        }
    } 
#endif
    BCM_IF_ERROR_RETURN(rv);
    profile_idx = soc_VLAN_XLATEm_field32_get(unit, &res_vent,
                                              TAG_ACTION_PROFILE_PTRf);       
    _bcm_trx_vlan_action_profile_entry_get(unit, action, profile_idx);

    return _bcm_trx_vlan_translate_entry_parse(unit, VLAN_XLATEm,  
                                               (uint32 *)&res_vent, action);  
}


/*
 * Function:
 *      bcm_vlan_translate_action_delete
 * Purpose:
 *      Delete a vlan translate lookup entry.
 * Parameters:
 *      unit            (IN) BCM unit number
 *      port            (IN) Ingress generic port
 *      key_type        (IN) Key Type : bcmVlanTranslateKey*
 *      outer_vlan      (IN) Packet outer VLAN ID
 *      inner_vlan      (IN) Packet inner VLAN ID
 */
int 
_bcm_trx_vlan_translate_action_delete(int unit,
                                     bcm_gport_t port,
                                     bcm_vlan_translate_key_t key_type,
                                     bcm_vlan_t outer_vlan,
                                     bcm_vlan_t inner_vlan)
{
    vlan_xlate_entry_t vent;
    uint32 profile_idx;
    int rv;
#if defined(BCM_TRIUMPH3_SUPPORT)
    int rv_vxlate_delete;
#endif

#if defined(BCM_TRIUMPH2_SUPPORT)
    _bcm_flex_stat_handle_t handle;
    _BCM_FLEX_STAT_HANDLE_CLEAR(handle);
#endif

    sal_memset(&vent, 0, sizeof(vent));

    if (BCM_GPORT_IS_NIV_PORT(port)) {
        BCM_IF_ERROR_RETURN(
            _bcm_trx_vif_vlan_translate_entry_assemble(unit, &vent, port, 
                         key_type,inner_vlan, outer_vlan));
        rv = soc_mem_delete_return_old(unit, VLAN_XLATEm, MEM_BLOCK_ALL, 
                                    &vent, &vent);
        if ((rv == SOC_E_NONE) && 
            soc_VLAN_XLATEm_field32_get(unit, &vent, VALIDf)) {
            profile_idx = soc_VLAN_XLATEm_field32_get(unit, &vent,
                                        VIF__TAG_ACTION_PROFILE_PTRf);       
            /* Delete the old vlan action profile entry */
            rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
        }
        return rv;
    }

    BCM_IF_ERROR_RETURN(
        _bcm_trx_vlan_translate_entry_assemble(unit, &vent, port, key_type,
                                               inner_vlan, outer_vlan));

#if defined(BCM_TRIUMPH2_SUPPORT)
        if (soc_feature(unit, soc_feature_gport_service_counters)) {
            /* Record flex stat info before the returned entry
             * includes non-key info. */

            _BCM_FLEX_STAT_HANDLE_COPY(handle, vent);
        }
#endif

    rv = soc_mem_delete_return_old(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vent, &vent);
#if defined(BCM_TRIUMPH3_SUPPORT)
    rv_vxlate_delete = rv;
#endif
    if ((rv == SOC_E_NONE) && soc_VLAN_XLATEm_field32_get(unit, &vent, VALIDf)) {
        profile_idx = soc_VLAN_XLATEm_field32_get(unit, &vent,
                                                  TAG_ACTION_PROFILE_PTRf);       
        /* Delete the old vlan action profile entry */
        rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);

#if defined(BCM_TRIUMPH2_SUPPORT)
        if (soc_feature(unit, soc_feature_gport_service_counters) &&
            (0 != soc_VLAN_XLATEm_field32_get(unit, &vent, VINTF_CTR_IDXf))) {
            /* Release Service counter */
            _bcm_esw_flex_stat_ext_handle_free(unit, _bcmFlexStatTypeVxlt,
                                               handle);
        }
#endif
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        vlan_xlate_extd_entry_t vxent;

        /* check the extend table */
        sal_memset(&vxent, 0, sizeof(vxent));
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vxlate2vxlate_extd(unit,&vent,&vxent));
        rv = soc_mem_delete_return_old(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL,
                     &vxent, &vxent);
        if ((rv == SOC_E_NONE) && soc_VLAN_XLATE_EXTDm_field32_get(unit, 
                        &vxent, VALID_0f)) {
            profile_idx = soc_VLAN_XLATE_EXTDm_field32_get(unit, &vxent,
                                                  TAG_ACTION_PROFILE_PTRf); 
            /* Delete the old vlan action profile entry */
            rv = _bcm_trx_vlan_action_profile_entry_delete(unit, profile_idx);
        } else {
            /* if an entry is already found in regular table, then reset ret code*/
            if (rv == BCM_E_NOT_FOUND) {
                if (rv_vxlate_delete == BCM_E_NONE) {
                    rv = BCM_E_NONE;
                }
            }
        }
    }
#endif

    return rv;
}

#if defined(BCM_TRIUMPH3_SUPPORT)
/*
 * Function:
 *      _bcm_tr3_vxlate_action_delete_extd_tbl
 * Purpose:
 *      Delete all vlan translate lookup entries.
 * Parameters:
 *      unit      (IN) BCM unit number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr3_vxlate_action_delete_extd_tbl(int unit) 
{
    int i, imin, imax, nent, vbytes, rv, mpls;
    uint32 old_profile_idx;
    vlan_xlate_extd_entry_t * vtab, * vnull,  *vtabp;
    
    imin = soc_mem_index_min(unit, VLAN_XLATE_EXTDm);
    imax = soc_mem_index_max(unit, VLAN_XLATE_EXTDm);
    nent = soc_mem_index_count(unit, VLAN_XLATE_EXTDm);
    vbytes = soc_mem_entry_words(unit, VLAN_XLATE_EXTDm);
    vbytes = WORDS2BYTES(vbytes);
    vtab = soc_cm_salloc(unit, nent * sizeof(*vtab), "vlan_xlate_extd");

    if (vtab == NULL) {
        return BCM_E_MEMORY;
    }
    
    vnull = soc_mem_entry_null(unit, VLAN_XLATE_EXTDm);

    soc_mem_lock(unit, VLAN_XLATE_EXTDm);
    rv = soc_mem_read_range(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ANY,
                            imin, imax, vtab);
    if (rv < 0) {
        soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
        soc_cm_sfree(unit, vtab);
        return rv; 
    }
    
    for(i = 0; i < nent; i++) {
        vtabp = soc_mem_table_idx_to_pointer(unit, VLAN_XLATE_EXTDm,
                                     vlan_xlate_extd_entry_t *, vtab, i);

        mpls = soc_VLAN_XLATE_EXTDm_field32_get(unit, vtabp, 
                            XLATE__MPLS_ACTIONf);

        if (!soc_VLAN_XLATE_EXTDm_field32_get(unit, vtabp, VALID_0f) ||
            (mpls != 0)) {
            continue;
        }

        old_profile_idx = soc_VLAN_XLATE_EXTDm_field32_get(unit, vtabp,
                                                      TAG_ACTION_PROFILE_PTRf);

        rv = WRITE_VLAN_XLATE_EXTDm(unit, MEM_BLOCK_ANY, i, vnull);

        if (rv >= 0) {
            /* Delete the old vlan action profile entry */
            rv = _bcm_trx_vlan_action_profile_entry_delete(unit, 
                            old_profile_idx);
        }
    }
    
    soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
    soc_cm_sfree(unit, vtab);
    return rv;
}
#endif   /*BCM_TRIUMPH3_SUPPORT */

/*
 * Function:
 *      _bcm_trx_vlan_translate_action_delete_all
 * Purpose:
 *      Delete all vlan translate lookup entries.
 * Parameters:
 *      unit      (IN) BCM unit number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_translate_action_delete_all(int unit) 
{
    int i, imin, imax, nent, vbytes, rv, mpls;
    uint32 old_profile_idx;
    vlan_xlate_entry_t * vtab, * vnull,  *vtabp;
    int vx_key_type;
    int vx_key_value;
#if defined(BCM_TRIUMPH2_SUPPORT)
    _bcm_flex_stat_handle_t handle;
    vlan_xlate_entry_t vent;
    uint32 key[2];
#endif
    
    imin = soc_mem_index_min(unit, VLAN_XLATEm);
    imax = soc_mem_index_max(unit, VLAN_XLATEm);
    nent = soc_mem_index_count(unit, VLAN_XLATEm);
    vbytes = soc_mem_entry_words(unit, VLAN_XLATEm);
    vbytes = WORDS2BYTES(vbytes);
    vtab = soc_cm_salloc(unit, nent * sizeof(*vtab), "vlan_xlate");

    if (vtab == NULL) {
        return BCM_E_MEMORY;
    }
    
    vnull = soc_mem_entry_null(unit, VLAN_XLATEm);

    soc_mem_lock(unit, VLAN_XLATEm);
    rv = soc_mem_read_range(unit, VLAN_XLATEm, MEM_BLOCK_ANY,
                            imin, imax, vtab);
    if (rv < 0) {
        soc_mem_unlock(unit, VLAN_XLATEm);
        soc_cm_sfree(unit, vtab);
        return rv; 
    }
    
    for(i = 0; i < nent; i++) {
        vtabp = soc_mem_table_idx_to_pointer(unit, VLAN_XLATEm,
                                             vlan_xlate_entry_t *, vtab, i);

#if defined(BCM_TRIUMPH3_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            mpls = 0;
        } else
#endif
        {
            if (SOC_IS_TR_VL(unit) && (!SOC_IS_HURRICANEX(unit)) &&
                SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, MPLS_ACTIONf)) {
                mpls = soc_VLAN_XLATEm_field32_get(unit, vtabp, MPLS_ACTIONf);
            } else {
                mpls = 0;
            }
        }

        vx_key_value = soc_VLAN_XLATEm_field32_get(unit, vtabp, KEY_TYPEf); 
        rv =  _bcm_esw_vlan_xlate_key_type_get(unit,vx_key_value,&vx_key_type);
        if (rv != BCM_E_NONE) {
            /* not valid key type, must be for extend entry */
            continue;
        }

        if (!soc_VLAN_XLATEm_field32_get(unit, vtabp, VALIDf) ||
            (mpls != 0) ||
            (vx_key_type == VLXLT_HASH_KEY_TYPE_VLAN_MAC)) {
            continue;
        }

        old_profile_idx = soc_VLAN_XLATEm_field32_get(unit, vtabp,
                                                      TAG_ACTION_PROFILE_PTRf);

        rv = WRITE_VLAN_XLATEm(unit, MEM_BLOCK_ANY, i, vnull);

        if (rv >= 0) {
            /* Delete the old vlan action profile entry */
            rv = _bcm_trx_vlan_action_profile_entry_delete(unit, old_profile_idx);
        }

#if defined(BCM_TRIUMPH2_SUPPORT)
        if (soc_feature(unit, soc_feature_gport_service_counters) &&
            (0 != soc_VLAN_XLATEm_field32_get(unit, vtabp, VINTF_CTR_IDXf))) {
            sal_memset(&vent, 0, sizeof(vent));
            /* Construct key-only entry, copy to FS handle */
            soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                     soc_VLAN_XLATEm_field32_get(unit, vtabp, KEY_TYPEf));
            soc_mem_field_get(unit, VLAN_XLATEm, (uint32 *) vtabp,
                               KEYf, (uint32 *) key);
            soc_mem_field_set(unit, VLAN_XLATEm, (uint32 *) &vent,
                               KEYf, (uint32 *) key);

            _BCM_FLEX_STAT_HANDLE_CLEAR(handle);
            _BCM_FLEX_STAT_HANDLE_COPY(handle, vent);

            /* Release Service counter */
            _bcm_esw_flex_stat_ext_handle_free(unit, _bcmFlexStatTypeVxlt,
                                               handle);
        }
#endif
    }
    
    soc_mem_unlock(unit, VLAN_XLATEm);
    soc_cm_sfree(unit, vtab);

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vxlate_action_delete_extd_tbl(unit));
    }
#endif
    return rv;
}

/*
 * Function   :
 *      _bcm_trx_vlan_translate_action_range_add
 * Description   :
 *      Add a range of VLANs and an entry to ingress VLAN translation table.
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      port            (IN) Ingress generic port
 *      outer_vlan_low  (IN) Packet outer VLAN ID low 
 *      outer_vlan_high (IN) Packet outer VLAN ID high
 *      inner_vlan_low  (IN) Packet inner VLAN ID low 
 *      inner_vlan_high (IN) Packet inner VLAN ID high
 *      action          (IN) Action for outer and inner tag
 */
int
_bcm_trx_vlan_translate_action_range_add(int unit,
                                        bcm_gport_t port,
                                        bcm_vlan_t outer_vlan_low,
                                        bcm_vlan_t outer_vlan_high,
                                        bcm_vlan_t inner_vlan_low,
                                        bcm_vlan_t inner_vlan_high,
                                        bcm_vlan_action_set_t *action)
{
    int i, key_type = 0, rv, gport_id, old_idx = 0, stm_idx = 0;
    uint32 new_idx = 0;
    source_trunk_map_table_entry_t stm_entry;
    bcm_module_t mod_out;
    bcm_port_t   port_out;
    bcm_trunk_t  trunk_id, tid;
    bcm_vlan_t min_vlan[8], max_vlan[8];
    bcm_vlan_t min_inner_vlan[8], max_inner_vlan[8];
    int port_count;
    bcm_trunk_member_t *member_array = NULL;
    bcm_module_t *mod_array = NULL;
    bcm_port_t   *port_array = NULL;
    int trunk128;
    soc_field_t field;
    int old_inner_idx = 0;
    uint32 new_inner_idx = 0;

    if ((outer_vlan_low != BCM_VLAN_INVALID) &&
        (inner_vlan_low != BCM_VLAN_INVALID)) {
        if (soc_feature (unit, soc_feature_vlan_double_tag_range_compress)) {
            key_type = bcmVlanTranslateKeyPortDouble;
            VLAN_CHK_ID(unit, inner_vlan_low);
            VLAN_CHK_ID(unit, inner_vlan_high);
            VLAN_CHK_ID(unit, outer_vlan_low);
            VLAN_CHK_ID(unit, outer_vlan_high);
        } else {
            /* For an incoming double tagged packet, hardware currently supports
             * VLAN range matching for outer VLAN ID only, not for inner VLAN ID
             * Hence, outer VLAN range and inner VLAN range cannot be specified
             * simultaneously.
             */
            return BCM_E_PARAM;
        }
    } else if (outer_vlan_low != BCM_VLAN_INVALID) {
        key_type = bcmVlanTranslateKeyPortOuter;
        VLAN_CHK_ID(unit, outer_vlan_low);
        VLAN_CHK_ID(unit, outer_vlan_high);
    } else if (inner_vlan_low != BCM_VLAN_INVALID) {
        key_type = bcmVlanTranslateKeyPortInner;
        VLAN_CHK_ID(unit, inner_vlan_low);
        VLAN_CHK_ID(unit, inner_vlan_high);
    } else {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(
        _bcm_trx_vlan_action_verify(unit, action));

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out, &trunk_id,
                                &gport_id));

    if (BCM_GPORT_IS_TRUNK(port)) {
        if (BCM_TRUNK_INVALID == trunk_id) {
            return BCM_E_PORT;
        }

        BCM_IF_ERROR_RETURN
            (bcm_esw_trunk_get(unit, trunk_id, NULL, 0, NULL,
                               &port_count));

        if (port_count <= 0) {
            return BCM_E_PORT;
        }

        member_array = sal_alloc(sizeof(bcm_trunk_member_t) * port_count,
                "trunk member array");
        if (member_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(member_array, 0, sizeof(bcm_trunk_member_t) * port_count);

        rv = bcm_esw_trunk_get(unit, trunk_id, NULL, port_count,
                member_array, &port_count);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }

        mod_array = sal_alloc(sizeof(bcm_module_t) * port_count,
                "module ID array");
        if (mod_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(mod_array, 0, sizeof(bcm_module_t) * port_count);

        port_array = sal_alloc(sizeof(bcm_port_t) * port_count,
                "port ID array");
        if (port_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(port_array, 0, sizeof(bcm_port_t) * port_count);

        for (i = 0; i < port_count; i++) {
            rv = _bcm_esw_gport_resolve(unit, member_array[i].gport,
                    &mod_array[i], &port_array[i], &tid, &gport_id);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
        }
    } else {
        if ((-1 == mod_out) || (-1 == port_out)) {
            return BCM_E_PORT;
        }

        port_count = 1;

        mod_array = sal_alloc(sizeof(bcm_module_t) * port_count,
                "module ID array");
        if (mod_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        mod_array[0] = mod_out;

        port_array = sal_alloc(sizeof(bcm_port_t) * port_count,
                "port ID array");
        if (port_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        port_array[0] = port_out;
    }

    rv = _bcm_esw_src_mod_port_table_index_get(unit, mod_array[0],
            port_array[0], &stm_idx);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    soc_mem_lock(unit, ING_VLAN_RANGEm);

    rv = READ_SOURCE_TRUNK_MAP_TABLEm(unit, MEM_BLOCK_ANY, stm_idx, &stm_entry);
    if (rv < 0) {
        soc_mem_unlock(unit, ING_VLAN_RANGEm);
        goto cleanup;
    }

    /* For devices supporting vlan double tag range compression
     * and if inner_vlan_low is valid
     */
    if (inner_vlan_low != BCM_VLAN_INVALID) {
        if (soc_feature (unit, soc_feature_vlan_double_tag_range_compress)) {
            field = INNER_VLAN_RANGE_IDXf;
        } else {
            field = VLAN_RANGE_IDXf;
        } 
        old_inner_idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                                  &stm_entry, field);

        /* Get the profile table entry for this port/trunk */
        _bcm_trx_vlan_range_profile_entry_get(unit,
                                min_inner_vlan, max_inner_vlan, old_inner_idx);
    
        /* Find the first unused min/max range. Unused ranges are
         * identified by { min == 1, max == 0 }
         */
        for (i = 0 ; i < 8 ; i++) {
            if ((min_inner_vlan[i] == 1) && (max_inner_vlan[i] == 0)) {
                break;
            } else if (min_inner_vlan[i] == inner_vlan_low) {
                /* Can't have multiple ranges with the same min */
                rv = BCM_E_EXISTS;
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
        }
        if (i == 8) {
            /* All ranges are taken */
            rv = BCM_E_FULL;
            soc_mem_unlock(unit, ING_VLAN_RANGEm);
            goto cleanup;
        }

        /* Insert the new range into the table entry sorted by min VID */
        for ( ; i > 0 ; i--) {
            if (min_inner_vlan[i - 1] > inner_vlan_low) {
                /* Move existing min/max down */
                min_inner_vlan[i] = min_inner_vlan[i - 1];
                max_inner_vlan[i] = max_inner_vlan[i - 1];
            } else {
                break;
            }
        }
        min_inner_vlan[i] = inner_vlan_low;
        max_inner_vlan[i] = inner_vlan_high;

        /* Adding the new profile table entry and update profile pointer */
        for (i = 0; i < port_count; i++) {
            rv = _bcm_trx_vlan_range_profile_entry_add(unit, 
                            min_inner_vlan, max_inner_vlan, &new_inner_idx);
            if (BCM_FAILURE(rv)) {
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
            rv = _bcm_esw_src_mod_port_table_index_get(unit, mod_array[i],
                    port_array[i], &stm_idx);
            if (BCM_SUCCESS(rv)) {
                rv = soc_mem_field32_modify(unit,
                        SOURCE_TRUNK_MAP_TABLEm, stm_idx, field, new_inner_idx);
            }
            if (BCM_FAILURE(rv)) {
                _bcm_trx_vlan_range_profile_entry_delete(unit, new_inner_idx);
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
        }
    }

    /* For devices supporting vlan double tag range compression
     * and entry is for outer tag or double tag vlan range compression
     * OR
     * Other devices not supporting double tag range compression feature
     */
    if (outer_vlan_low != BCM_VLAN_INVALID) {
        field = VLAN_RANGE_IDXf;
        if (soc_feature (unit, soc_feature_vlan_double_tag_range_compress)) {
            if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm, 
                              OUTER_VLAN_RANGE_IDXf)) {  
                field = OUTER_VLAN_RANGE_IDXf;
            }
        }

        old_idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                                  &stm_entry, field);

        /* Get the profile table entry for this port/trunk */
        _bcm_trx_vlan_range_profile_entry_get(unit,
                                          min_vlan, max_vlan, old_idx);
        
        /* Find the first unused min/max range. Unused ranges are
         * identified by { min == 1, max == 0 }
         */
        for (i = 0 ; i < 8 ; i++) {
            if ((min_vlan[i] == 1) && (max_vlan[i] == 0)) {
                break;
            } else if (min_vlan[i] == outer_vlan_low) {
                /* Can't have multiple ranges with the same min */
                rv = BCM_E_EXISTS;
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
        }
        if (i == 8) {
            /* All ranges are taken */
            rv = BCM_E_FULL;
            soc_mem_unlock(unit, ING_VLAN_RANGEm);
            goto cleanup;
        }
       
        /* Insert the new range into the table entry sorted by min VID */
        for ( ; i > 0 ; i--) {
            if (min_vlan[i - 1] > outer_vlan_low) {
                /* Move existing min/max down */
                min_vlan[i] = min_vlan[i - 1];
                max_vlan[i] = max_vlan[i - 1];
            } else {
                break;
            }
        }
        min_vlan[i] = outer_vlan_low;
        max_vlan[i] = outer_vlan_high;
        
        /* Adding the new profile table entry and update profile pointer */
        for (i = 0; i < port_count; i++) {
            rv = _bcm_trx_vlan_range_profile_entry_add(unit, min_vlan, max_vlan,
                    &new_idx);
            if (BCM_FAILURE(rv)) {
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
            rv = _bcm_esw_src_mod_port_table_index_get(unit, mod_array[i],
                    port_array[i], &stm_idx);
            if (BCM_SUCCESS(rv)) {
                rv = soc_mem_field32_modify(unit,
                        SOURCE_TRUNK_MAP_TABLEm, stm_idx, field, new_idx);
            }
            if (BCM_FAILURE(rv)) {
                _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
        }
    }
   
    /* Add an entry in the vlan translate table for the low VID */
    rv = _bcm_trx_vlan_translate_action_add(unit, port, key_type,
                                           outer_vlan_low, inner_vlan_low,
                                           action);
    if (rv != BCM_E_NONE) {
        for (i = 0; i < port_count; i++) {
            if (outer_vlan_low != BCM_VLAN_INVALID) {
                _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
            }
            if (inner_vlan_low != BCM_VLAN_INVALID) {
                _bcm_trx_vlan_range_profile_entry_delete(unit, new_inner_idx);
            }
        }
        soc_mem_unlock(unit, ING_VLAN_RANGEm);
        goto cleanup;
    }

    /* Delete the old profile entry */
    for (i = 0; i < port_count; i++) {
        if (outer_vlan_low != BCM_VLAN_INVALID) {
            rv = _bcm_trx_vlan_range_profile_entry_delete(unit, old_idx);
            if (BCM_FAILURE(rv)) {
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
        }
        if (inner_vlan_low != BCM_VLAN_INVALID) {
            rv = _bcm_trx_vlan_range_profile_entry_delete(unit, old_inner_idx);
            if (BCM_FAILURE(rv)) {
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
        }
    }

    if (BCM_GPORT_IS_TRUNK(port)) {
        if (SOC_MEM_FIELD_VALID(unit, TRUNK32_PORT_TABLEm, VLAN_RANGE_IDXf)) {
            trunk128 = soc_property_get(unit, spn_TRUNK_EXTEND, 1);
            if (!trunk128) {
                rv = _bcm_trx_vlan_range_profile_entry_add(unit,
                        min_vlan, max_vlan, &new_idx);
                if (BCM_FAILURE(rv)) {
                    soc_mem_unlock(unit, ING_VLAN_RANGEm);
                    goto cleanup;
                }
                rv = soc_mem_field32_modify(unit,
                        TRUNK32_PORT_TABLEm, trunk_id,
                        VLAN_RANGE_IDXf, new_idx);
                if (BCM_FAILURE(rv)) {
                    _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
                    soc_mem_unlock(unit, ING_VLAN_RANGEm);
                    goto cleanup;
                }
                if (outer_vlan_low != BCM_VLAN_INVALID) {
                    rv = _bcm_trx_vlan_range_profile_entry_delete(unit,
                            old_idx);
                    if (BCM_FAILURE(rv)) {
                        soc_mem_unlock(unit, ING_VLAN_RANGEm);
                        goto cleanup;
                    }
                }
                if (inner_vlan_low != BCM_VLAN_INVALID) {
                    rv = _bcm_trx_vlan_range_profile_entry_delete(unit,
                            old_inner_idx);
                    if (BCM_FAILURE(rv)) {
                        soc_mem_unlock(unit, ING_VLAN_RANGEm);
                        goto cleanup;
                    }
                }
            }
        }
    }
    
    soc_mem_unlock(unit, ING_VLAN_RANGEm);

cleanup:
    if (NULL != member_array) {
        sal_free(member_array);
    }
    if (NULL != mod_array) {
        sal_free(mod_array);
    }
    if (NULL != port_array) {
        sal_free(port_array);
    }

    return rv;
}

/*
 * Function   :
 *      _bcm_trx_vlan_range_profile_index_get
 * Description   :
 *      Get the index for profile entry in the ING_VLAN_RANGE memory.
 * Parameters   :
 *      unit        (IN) BCM unit number
 *      gport       (IN) Ingress generic port
 *      profile_idx (OUT) index to profile entry
 */
int 
_bcm_trx_vlan_range_profile_index_get(int unit, bcm_gport_t gport, 
                                     int *profile_idx)
{
    int                             idx = 0, gport_id = 0;
    bcm_port_t                      port_out;
    bcm_module_t                    mod_out;
    bcm_trunk_t                     trunk_id;
    int                             stm_idx;     /* Source Trunk Map index */
    source_trunk_map_table_entry_t  stm_entry;
    bcm_trunk_member_t              trunk_member;
    int                             port_count;

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, gport, &mod_out, &port_out, &trunk_id,
                                &gport_id));

    if (BCM_GPORT_IS_TRUNK(gport)) {
        if (BCM_TRUNK_INVALID == trunk_id) {
            return BCM_E_PORT;
        }

        BCM_IF_ERROR_RETURN
            (bcm_esw_trunk_get(unit, trunk_id, NULL, 1, &trunk_member,
                                      &port_count));
        BCM_IF_ERROR_RETURN
            (_bcm_esw_gport_resolve(unit, trunk_member.gport,
                                    &mod_out, &port_out, &trunk_id, &gport_id));
    } else {
        if ((-1 == mod_out) || (-1 == port_out)) {
            return BCM_E_PORT;
        }
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_src_mod_port_table_index_get(unit, mod_out,
            port_out, &stm_idx));

    BCM_IF_ERROR_RETURN(
       READ_SOURCE_TRUNK_MAP_TABLEm(unit, MEM_BLOCK_ANY, stm_idx, &stm_entry));

    if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                                          OUTER_VLAN_RANGE_IDXf)) {
        idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                              &stm_entry, OUTER_VLAN_RANGE_IDXf);
    } else {
        idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                              &stm_entry, VLAN_RANGE_IDXf);
    }
        *profile_idx = idx;
    return BCM_E_NONE;
    }

/*
 * Function   :
 *      _bcm_trx_vlan_range_profile_inner_index_get
 * Description   :
 *      Get the inner index for profile entry in the ING_VLAN_RANGE memory.
 * Parameters   :
 *      unit        (IN) BCM unit number
 *      gport       (IN) Ingress generic port
 *      profile_idx (OUT) inner vlan range index to profile entry
 */
int 
_bcm_trx_vlan_range_profile_inner_index_get(int unit, bcm_gport_t gport, 
                                     int *profile_idx)
{
    int                             idx = 0, gport_id = 0;
    bcm_port_t                      port_out;
    bcm_module_t                    mod_out;
    bcm_trunk_t                     trunk_id;
    int                             stm_idx;     /* Source Trunk Map index */
    source_trunk_map_table_entry_t  stm_entry;
    bcm_trunk_info_t                tinfo;
    bcm_trunk_member_t              trunk_member;
    int                             port_count;

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, gport, &mod_out, &port_out, &trunk_id,
                                &gport_id));

    if (BCM_GPORT_IS_TRUNK(gport)) {
        if (BCM_TRUNK_INVALID == trunk_id) {
            return BCM_E_PORT;
        }
        BCM_IF_ERROR_RETURN(bcm_esw_trunk_get(unit, trunk_id, &tinfo, 1,
                    &trunk_member, &port_count));
        BCM_IF_ERROR_RETURN(_bcm_esw_gport_resolve(unit, trunk_member.gport,
                    &mod_out, &port_out, &trunk_id, &gport_id));
    } else {
        if ((-1 == mod_out) || (-1 == port_out)) {
            return BCM_E_PORT;
        }
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_src_mod_port_table_index_get(unit, mod_out,
            port_out, &stm_idx));

    BCM_IF_ERROR_RETURN(
       READ_SOURCE_TRUNK_MAP_TABLEm(unit, MEM_BLOCK_ANY, stm_idx, &stm_entry));

    if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                        INNER_VLAN_RANGE_IDXf)) {
        idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                          &stm_entry, INNER_VLAN_RANGE_IDXf);
        *profile_idx = idx;
    }
    return BCM_E_NONE;
}

/*
 * Function   :
 *      _bcm_trx_vlan_translate_action_range_get
 * Description   :
 *      Get a range of VLANs and an entry to ingress VLAN translation table.
 *      for Triumph
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      port            (IN) Ingress generic port
 *      outer_vlan_low  (IN) Packet outer VLAN ID low 
 *      outer_vlan_high (IN) Packet outer VLAN ID high
 *      inner_vlan_low  (IN) Packet inner VLAN ID low 
 *      inner_vlan_high (IN) Packet inner VLAN ID high
 *      action          (OUT) Action for outer and inner tag
 */
int
_bcm_trx_vlan_translate_action_range_get(int unit,
                                        bcm_gport_t port,
                                        bcm_vlan_t outer_vlan_low,
                                        bcm_vlan_t outer_vlan_high,
                                        bcm_vlan_t inner_vlan_low,
                                        bcm_vlan_t inner_vlan_high,
                                        bcm_vlan_action_set_t *action)
{
    int         key_type = 0, profile_idx, i;
    bcm_vlan_t  min_vlan[8], max_vlan[8];
    int rv;
    
    if ((outer_vlan_low != BCM_VLAN_INVALID) &&
        (inner_vlan_low != BCM_VLAN_INVALID)) {
        if (soc_feature (unit, soc_feature_vlan_double_tag_range_compress)) {
            key_type = bcmVlanTranslateKeyPortDouble;
            VLAN_CHK_ID(unit, inner_vlan_low);
            VLAN_CHK_ID(unit, inner_vlan_high);
            VLAN_CHK_ID(unit, outer_vlan_low);
            VLAN_CHK_ID(unit, outer_vlan_high);
        } else {
            /* For an incoming double tagged packet, hardware currently supports
             * VLAN range matching for outer VLAN ID only, not for inner VLAN ID
             * Hence, outer VLAN range and inner VLAN range cannot be specified
             * simultaneously.
             */
            return BCM_E_PARAM;
        }
    } else if (outer_vlan_low != BCM_VLAN_INVALID) {
        key_type = bcmVlanTranslateKeyPortOuter;
        VLAN_CHK_ID(unit, outer_vlan_low);
        VLAN_CHK_ID(unit, outer_vlan_high);
    } else if (inner_vlan_low != BCM_VLAN_INVALID) {
        key_type = bcmVlanTranslateKeyPortInner;
        VLAN_CHK_ID(unit, inner_vlan_low);
        VLAN_CHK_ID(unit, inner_vlan_high);
    } else {
        return BCM_E_PARAM;
    }

    /* For devices supporting vlan double tag range compression
     * and if inner_vlan_low is valid
     */
    if (inner_vlan_low != BCM_VLAN_INVALID) {
        if (soc_feature (unit, soc_feature_vlan_double_tag_range_compress)) {
            rv = _bcm_trx_vlan_range_profile_inner_index_get(unit,
                port, &profile_idx);
        } else {
            rv = _bcm_trx_vlan_range_profile_index_get(unit, port, &profile_idx);
        }
        BCM_IF_ERROR_RETURN(rv);

        /* Get the profile table entry for this port/trunk */
        _bcm_trx_vlan_range_profile_entry_get(unit, min_vlan, 
                max_vlan, profile_idx);
        /* Find range match */
        for (i = 0 ; i < 8 ; i++) {
            if ((min_vlan[i] == inner_vlan_low) &&
                (max_vlan[i] == inner_vlan_high)) {
                break;
            }
        }
        if (i == 8) {
            return BCM_E_NOT_FOUND;
        }
    }

    /* For devices supporting vlan double tag range compression
     * and entry is for outer tag or double tag range compression
     * OR
     * Other devices not supporting double tag range compression feature 
     */
    if (outer_vlan_low != BCM_VLAN_INVALID) {
        rv = _bcm_trx_vlan_range_profile_index_get(unit, port, &profile_idx);
        BCM_IF_ERROR_RETURN(rv);
        
        /* Get the profile table entry for this port/trunk */
        _bcm_trx_vlan_range_profile_entry_get(unit, min_vlan, 
                max_vlan, profile_idx);
        /* Find range match */
        for (i = 0 ; i < 8 ; i++) {
            if ((min_vlan[i] == outer_vlan_low) &&
                (max_vlan[i] == outer_vlan_high)) {
                break;
            }
        }
        if (i == 8) {
            return BCM_E_NOT_FOUND;
        }
    }
    /* Get an entry in the vlan translate table for the low VID */
    rv = _bcm_trx_vlan_translate_action_get(unit,port, key_type,
                                outer_vlan_low, inner_vlan_low, action);  
    if (rv == BCM_E_NOT_FOUND) {
        int mod_out;
        int port_out;
        int trunk_id;
        int gport_id;
        int    stm_idx = 0;
        source_trunk_map_table_entry_t  stm_entry;
        int port_type;

        /* check if the port belongs to a trunk group and whether 
         * a xlat entry for that TGID exist
         */
        if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, Tf)) {
            if (BCM_GPORT_IS_MODPORT(port)) {
                BCM_IF_ERROR_RETURN
                    (_bcm_esw_gport_resolve(unit, port, &mod_out, 
                          &port_out, &trunk_id, &gport_id));

                /* Get index to source trunk map table */
                BCM_IF_ERROR_RETURN
                    (_bcm_esw_src_mod_port_table_index_get(unit, 
                        mod_out, port_out, &stm_idx));

                /* read the entry */
                BCM_IF_ERROR_RETURN(
                   READ_SOURCE_TRUNK_MAP_TABLEm(unit, MEM_BLOCK_ANY, 
                                stm_idx, &stm_entry));
                trunk_id = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                                         &stm_entry, TGIDf);
                port_type = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                                         &stm_entry, PORT_TYPEf);

                if (port_type == 1) {
                    BCM_GPORT_TRUNK_SET(gport_id, trunk_id);
                    rv = _bcm_trx_vlan_translate_action_get(unit,gport_id,
                         key_type,outer_vlan_low, inner_vlan_low, action);
                }
            } 
        }
    } 
    return (rv);
}

/*
 * Function   :
 *      _bcm_trx_vlan_translate_action_range_delete
 * Description   :
 *      Delete a range of VLANs and an entry from ingress VLAN translation table
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      port            (IN) Ingress generic port
 *      outer_vlan_low  (IN) Packet outer VLAN ID low 
 *      outer_vlan_high (IN) Packet outer VLAN ID high
 *      inner_vlan_low  (IN) Packet inner VLAN ID low 
 *      inner_vlan_high (IN) Packet inner VLAN ID high
 */
int
_bcm_trx_vlan_translate_action_range_delete(int unit,
                                           bcm_gport_t port,
                                           bcm_vlan_t outer_vlan_low,
                                           bcm_vlan_t outer_vlan_high,
                                           bcm_vlan_t inner_vlan_low,
                                           bcm_vlan_t inner_vlan_high)
{
    int i, key_type = 0, rv, gport_id, old_idx = 0, stm_idx = 0;
    uint32 new_idx = 0;
    source_trunk_map_table_entry_t stm_entry;
    bcm_module_t mod_out;
    bcm_port_t   port_out;
    bcm_trunk_t  trunk_id, tid;
    bcm_vlan_t min_vlan[8], max_vlan[8];
    bcm_vlan_t min_inner_vlan[8], max_inner_vlan[8];
    int port_count;
    bcm_trunk_member_t *member_array = NULL;
    bcm_module_t *mod_array = NULL;
    bcm_port_t   *port_array = NULL;
    int trunk128;
    soc_field_t field;
    int old_inner_idx = 0;
    uint32 new_inner_idx = 0;

    if ((outer_vlan_low != BCM_VLAN_INVALID) &&
        (inner_vlan_low != BCM_VLAN_INVALID)) {
        if (soc_feature (unit, soc_feature_vlan_double_tag_range_compress)) {
            key_type = bcmVlanTranslateKeyPortDouble;
            VLAN_CHK_ID(unit, outer_vlan_low);
            VLAN_CHK_ID(unit, outer_vlan_high);
            VLAN_CHK_ID(unit, outer_vlan_low);
            VLAN_CHK_ID(unit, outer_vlan_high);
        } else {
            /* For an incoming double tagged packet, hardware currently supports
             * VLAN range matching for outer VLAN ID only, not for inner VLAN ID
             * Hence, outer VLAN range and inner VLAN range cannot be specified
             * simultaneously.
             */
            return BCM_E_PARAM;
        }
    } else if (outer_vlan_low != BCM_VLAN_INVALID) {
        key_type = bcmVlanTranslateKeyPortOuter;
        VLAN_CHK_ID(unit, outer_vlan_low);
        VLAN_CHK_ID(unit, outer_vlan_high);
    } else if (inner_vlan_low != BCM_VLAN_INVALID) {
        key_type = bcmVlanTranslateKeyPortInner;
        VLAN_CHK_ID(unit, inner_vlan_low);
        VLAN_CHK_ID(unit, inner_vlan_high);
    } else {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out, &trunk_id,
                                &gport_id));

    if (BCM_GPORT_IS_TRUNK(port)) {
       if (BCM_TRUNK_INVALID == trunk_id) {
            return BCM_E_PORT;
        }

        BCM_IF_ERROR_RETURN
            (bcm_esw_trunk_get(unit, trunk_id, NULL, 0, NULL, &port_count));

        member_array = sal_alloc(sizeof(bcm_trunk_member_t) * port_count,
                "trunk member array");
        if (member_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(member_array, 0, sizeof(bcm_trunk_member_t) * port_count);

        rv = bcm_esw_trunk_get(unit, trunk_id, NULL, port_count,
                member_array, &port_count);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }

        mod_array = sal_alloc(sizeof(bcm_module_t) * port_count,
                "module ID array");
        if (mod_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(mod_array, 0, sizeof(bcm_module_t) * port_count);

        port_array = sal_alloc(sizeof(bcm_port_t) * port_count,
                "port ID array");
        if (port_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(port_array, 0, sizeof(bcm_port_t) * port_count);

        for (i = 0; i < port_count; i++) {
            rv = _bcm_esw_gport_resolve(unit, member_array[i].gport,
                    &mod_array[i], &port_array[i], &tid, &gport_id);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
        }
    } else {
        if ((-1 == mod_out) || (-1 == port_out)) {
            return BCM_E_PORT;
        }

        port_count = 1;

        mod_array = sal_alloc(sizeof(bcm_module_t) * port_count,
                "module ID array");
        if (mod_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        mod_array[0] = mod_out;

        port_array = sal_alloc(sizeof(bcm_port_t) * port_count,
                "port ID array");
        if (port_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        port_array[0] = port_out;
    }

    rv = _bcm_esw_src_mod_port_table_index_get(unit, mod_array[0],
            port_array[0], &stm_idx);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    soc_mem_lock(unit, ING_VLAN_RANGEm);
    rv = READ_SOURCE_TRUNK_MAP_TABLEm(unit, MEM_BLOCK_ANY, stm_idx, &stm_entry);
    if (rv < 0) {
        soc_mem_unlock(unit, ING_VLAN_RANGEm);
        goto cleanup;
    }

    /* If vlan double tag range compression feature is supported and
     * entry type is for inner tag range translation */
    if (inner_vlan_low != BCM_VLAN_INVALID) {
        if (soc_feature (unit, soc_feature_vlan_double_tag_range_compress)) {
            field = INNER_VLAN_RANGE_IDXf;
        } else {
            field = VLAN_RANGE_IDXf;
        }

        old_inner_idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                                  &stm_entry, field);

        /* Get the profile table entry for this port/trunk */
        _bcm_trx_vlan_range_profile_entry_get(unit,
                                min_inner_vlan, max_inner_vlan, old_inner_idx);
    
        /* Find the min/max range. */
        for (i = 0 ; i < 8 ; i++) {
            if ((min_inner_vlan[i] == inner_vlan_low) &&
                (max_inner_vlan[i] == inner_vlan_high)) {
                break;
            }
        }
        if (i == 8) {
            rv = BCM_E_NOT_FOUND;
            soc_mem_unlock(unit, ING_VLAN_RANGEm);
            goto cleanup;
        }

        /* Remove the range from the table entry and fill in the gap */
        for ( ; i < 7 ; i++) {
            /* Move existing min/max UP */
            min_inner_vlan[i] = min_inner_vlan[i + 1];
            max_inner_vlan[i] = max_inner_vlan[i + 1];
        }
        /* Mark last min/max range as unused. Unused ranges are
         * identified by { min == 1, max == 0 }
         */
        min_inner_vlan[i] = 1;
        max_inner_vlan[i] = 0;
    
        /* Adding the new profile table entry and update profile pointer */
        for (i = 0; i < port_count; i++) {
            rv = _bcm_trx_vlan_range_profile_entry_add(unit,
                    min_inner_vlan, max_inner_vlan, &new_inner_idx);
            if (BCM_FAILURE(rv)) {
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
            rv = _bcm_esw_src_mod_port_table_index_get(unit, mod_array[i],
                    port_array[i], &stm_idx);
            if (BCM_SUCCESS(rv)) {
                rv = soc_mem_field32_modify(unit,
                        SOURCE_TRUNK_MAP_TABLEm, stm_idx,
                        field, new_inner_idx);
            }
            if (BCM_FAILURE(rv)) {
                _bcm_trx_vlan_range_profile_entry_delete(unit, new_inner_idx);
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
        }
    }
    
    /* If vlan double tag range compression feature is supported and
     * entry type is for double tag range compression */
    if (outer_vlan_low != BCM_VLAN_INVALID) {
        field = VLAN_RANGE_IDXf;
        if (soc_feature (unit, soc_feature_vlan_double_tag_range_compress)) {
            if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm, 
                              OUTER_VLAN_RANGE_IDXf)) {  
                field = OUTER_VLAN_RANGE_IDXf;
            }
        }

        old_idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                &stm_entry, field);

        /* Get the profile table entry for this port/trunk */
        _bcm_trx_vlan_range_profile_entry_get(unit,
                                              min_vlan, max_vlan, old_idx);


        /* Find the min/max range. */
        for (i = 0 ; i < 8 ; i++) {
            if ((min_vlan[i] == outer_vlan_low) &&
                (max_vlan[i] == outer_vlan_high)) {
                        break;
            }
        }
        if (i == 8) {
            rv = BCM_E_NOT_FOUND;
            soc_mem_unlock(unit, ING_VLAN_RANGEm);
            goto cleanup;
        }

        /* Remove the range from the table entry and fill in the gap */
        for ( ; i < 7 ; i++) {
            /* Move existing min/max UP */
            min_vlan[i] = min_vlan[i + 1];
            max_vlan[i] = max_vlan[i + 1];
        }
        /* Mark last min/max range as unused. Unused ranges are
         * identified by { min == 1, max == 0 }
         */
        min_vlan[i] = 1;
        max_vlan[i] = 0;

        /* Adding the new profile table entry and update profile pointer */
        for (i = 0; i < port_count; i++) {
            rv = _bcm_trx_vlan_range_profile_entry_add(unit,
                    min_vlan, max_vlan, &new_idx);
            if (BCM_FAILURE(rv)) {
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
            rv = _bcm_esw_src_mod_port_table_index_get(unit, mod_array[i],
                    port_array[i], &stm_idx);
            if (BCM_SUCCESS(rv)) {
                rv = soc_mem_field32_modify(unit,
                        SOURCE_TRUNK_MAP_TABLEm, stm_idx,
                        field, new_idx);
            }
            if (BCM_FAILURE(rv)) {
                _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
                soc_mem_unlock(unit, ING_VLAN_RANGEm);
                goto cleanup;
            }
        }
    }
    
    /* Delete the entry from the vlan translate table for the low VID */
    rv = _bcm_trx_vlan_translate_action_delete(unit, port, key_type,
                                               outer_vlan_low, inner_vlan_low);
    if ((rv != BCM_E_NONE) && (rv != BCM_E_NOT_FOUND)){
        for (i = 0; i < port_count; i++) {
            if (outer_vlan_low != BCM_VLAN_INVALID) {
                _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
            }
            if (inner_vlan_low != BCM_VLAN_INVALID) {
                _bcm_trx_vlan_range_profile_entry_delete(unit, new_inner_idx);
            }
        }
        soc_mem_unlock(unit, ING_VLAN_RANGEm);
        goto cleanup;
    }

    /* Delete the old profile entry */
    for (i = 0; i < port_count; i++) {
        if (outer_vlan_low != BCM_VLAN_INVALID) {
            _bcm_trx_vlan_range_profile_entry_delete(unit, old_idx);
        }
        if (inner_vlan_low != BCM_VLAN_INVALID) {
            _bcm_trx_vlan_range_profile_entry_delete(unit, old_inner_idx);
        }
    }

    if (BCM_GPORT_IS_TRUNK(port)) {
        if (SOC_MEM_FIELD_VALID(unit, TRUNK32_PORT_TABLEm, VLAN_RANGE_IDXf)) {
            trunk128 = soc_property_get(unit, spn_TRUNK_EXTEND, 1);
            if (!trunk128) {
                rv = _bcm_trx_vlan_range_profile_entry_add(unit,
                        min_vlan, max_vlan, &new_idx);
                if (BCM_FAILURE(rv)) {
                    soc_mem_unlock(unit, ING_VLAN_RANGEm);
                    goto cleanup;
                }
                rv = soc_mem_field32_modify(unit, TRUNK32_PORT_TABLEm, trunk_id,
                        VLAN_RANGE_IDXf, new_idx);
                if (BCM_FAILURE(rv)) {
                    _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
                    soc_mem_unlock(unit, ING_VLAN_RANGEm);
                    goto cleanup;
                }
                if (outer_vlan_low != BCM_VLAN_INVALID) {
                    _bcm_trx_vlan_range_profile_entry_delete(unit, old_idx);
                }
                if (inner_vlan_low != BCM_VLAN_INVALID) {
                    _bcm_trx_vlan_range_profile_entry_delete(unit, old_inner_idx);
                }
            }
        }
    }
    soc_mem_unlock(unit, ING_VLAN_RANGEm);
    
cleanup:
    if (NULL != member_array) {
        sal_free(member_array);
    }
    if (NULL != mod_array) {
        sal_free(mod_array);
    }
    if (NULL != port_array) {
        sal_free(port_array);
    }

    return rv;
}

/*
 * Function   :
 *      bcm_vlan_translate_action_range_delete_all
 * Description   :
 *      Delete all ranges of VLANs and entries from ingress 
 *      VLAN translation table.
 * Parameters   :
 *      unit            (IN) BCM unit number
 */
int
_bcm_trx_vlan_translate_action_range_delete_all(int unit)
{
    int rv;

    rv = _bcm_trx_vlan_range_profile_init(unit);
    if (rv == BCM_E_NONE) {
        rv = _bcm_trx_vlan_translate_action_delete_all(unit);
    }
    return rv;
}

/*
 * Function:
 *     _bcm_trx_vlan_translate_stm_index_to_gport
 * Description:
 *      Translate index of SOURCE_TRUNK_MAP_TABLE into gport
 * Parameters:
 *      unit         device number
 *      stm_index    Index to SOURCE_TRUNK_MAP_TABLE
 *      gport       (OUT) gport
 * Return:
 *     BCM_E_XXX
 */
STATIC int  
_bcm_trx_vlan_translate_stm_index_to_gport(int unit, int stm_index, 
                                          bcm_gport_t *gport)
{
    bcm_module_t    modid;
    bcm_port_t      port;
    _bcm_gport_dest_t   gport_dest;

    if (NULL == gport) {
        return BCM_E_PARAM;
    }

#if defined(BCM_TRIDENT_SUPPORT)
    if (soc_feature(unit, soc_feature_src_modid_base_index)) {
        BCM_IF_ERROR_RETURN
            (_bcm_esw_src_modid_port_get(unit, stm_index, 
                                                 &modid,&port));
    } else
#endif
    {
        modid = stm_index / (SOC_PORT_ADDR_MAX(unit) + 1) ; 
        port = stm_index - modid * (SOC_PORT_ADDR_MAX(unit) + 1);
    }

    if (!SOC_MODID_ADDRESSABLE(unit, modid)) {
        return BCM_E_PARAM;
    }

    if (!SOC_PORT_ADDRESSABLE(unit, port)) {
        return BCM_E_PARAM;
    }

    gport_dest.port = port;
    gport_dest.modid = modid;
    gport_dest.gport_type = _SHR_GPORT_TYPE_MODPORT;
    BCM_IF_ERROR_RETURN(_bcm_esw_gport_construct(unit, &gport_dest, gport));

    return BCM_E_NONE;
}

/*
 * Function   :
 *      _bcm_trx_vlan_translate_action_range_traverse
 * Description   :
 *      Traverses over range of ingress VLAN translation table.
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      trvs_info       (IN/OUT) Traverse Structure to opertae on
 */
int
_bcm_trx_vlan_translate_action_range_traverse(int unit, 
                                    _bcm_vlan_translate_traverse_t *trvs_info)
{
    /* Indexes to iterate over memories, chunks and entries */
    int                           chnk_idx, ent_idx, chnk_idx_max, mem_idx_max;
    int                             buf_size, chunksize, chnk_end;
    /* Buffer to store chunks of memory table we currently work on */
    uint32                          *stm_tbl_chnk;
    source_trunk_map_table_entry_t  *stm_ent;
    soc_mem_t                       mem;
    int                             stop, rv = BCM_E_NONE;
    /* Index to point to table of vlan ranges. */
    int                             range_idx, stm_index, i;
    bcm_gport_t                     gport;
    bcm_vlan_t                      min_vlan[8], max_vlan[8];
    soc_field_t                    field;
    bcm_vlan_t                     min_inner_vlan[8], max_inner_vlan[8];
    int                            range_inner_idx, j;

    mem = SOURCE_TRUNK_MAP_TABLEm;
    if (!soc_mem_index_count(unit, mem)) {
        return BCM_E_NONE;
    }

    chunksize = soc_property_get(unit, spn_VLANDELETE_CHUNKS,
                                 VLAN_MEM_CHUNKS_DEFAULT);

    buf_size = 4 * SOC_MAX_MEM_FIELD_WORDS * chunksize;
    stm_tbl_chnk = soc_cm_salloc(unit, buf_size, 
                                 "vlan translate range traverse");
    if (NULL == stm_tbl_chnk) {
        return BCM_E_MEMORY;
    }

    mem_idx_max = soc_mem_index_max(unit, mem);
    for (chnk_idx = soc_mem_index_min(unit, mem); 
         chnk_idx <= mem_idx_max; 
         chnk_idx += chunksize) {
        sal_memset((void *)stm_tbl_chnk, 0, buf_size);

        chnk_idx_max = 
            ((chnk_idx + chunksize) <= mem_idx_max) ? 
            chnk_idx + chunksize - 1: mem_idx_max;

        rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ANY,
                                chnk_idx, chnk_idx_max, stm_tbl_chnk);
        if (SOC_FAILURE(rv)) {
            break;
        }
        chnk_end = (chnk_idx_max - chnk_idx);

        if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                    OUTER_VLAN_RANGE_IDXf)) {
            field = OUTER_VLAN_RANGE_IDXf;
        } else {
            field = VLAN_RANGE_IDXf;
        }

        for (ent_idx = 0 ; ent_idx <= chnk_end; ent_idx ++) {
            stm_ent = soc_mem_table_idx_to_pointer(unit, mem, 
                                             source_trunk_map_table_entry_t *, 
                                             stm_tbl_chnk, ent_idx);

            if (soc_feature (unit,
                            soc_feature_vlan_double_tag_range_compress)) {
                range_idx = soc_mem_field32_get(unit, mem, stm_ent, field);
                _bcm_trx_vlan_range_profile_entry_get(unit, min_vlan, max_vlan,
                        range_idx);

                if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                            INNER_VLAN_RANGE_IDXf)) {
                    range_inner_idx = soc_mem_field32_get(unit, mem, stm_ent, 
                            INNER_VLAN_RANGE_IDXf);
                    _bcm_trx_vlan_range_profile_entry_get(unit, min_inner_vlan,
                            max_inner_vlan, range_inner_idx);
                    if ((1 == min_vlan[0]) && (0 == max_vlan[0]) &&
                        (1 == min_inner_vlan[0]) && (0 == max_inner_vlan[0])) {
                        continue;
                    }
                } else {
                    if ((1 == min_vlan[0]) && (0 == max_vlan[0])) {
                        continue;
                    }
                }
                stm_index = chnk_idx + ent_idx; 
                rv = _bcm_trx_vlan_translate_stm_index_to_gport(unit, stm_index,
                        &gport);
                if (BCM_FAILURE(rv)) {
                    break;
                }
                trvs_info->gport = gport;

                for (i = 0; i < 8; i++) {
                    if ((1 == min_vlan[i]) && (0 == max_vlan[i])) {
                        continue;
                    }
                    trvs_info->inner_vlan = BCM_VLAN_INVALID;
                    trvs_info->inner_vlan_high = BCM_VLAN_INVALID;
                    trvs_info->outer_vlan = BCM_VLAN_INVALID;
                    trvs_info->outer_vlan_high = BCM_VLAN_INVALID;
                    /* check if entry key_type is 
                       bcmVlanTranslateKeyPortOuter */
                    rv = _bcm_trx_vlan_translate_action_range_get(
                            unit, gport,
                            min_vlan[i], max_vlan[i],
                            BCM_VLAN_INVALID, BCM_VLAN_INVALID,
                            trvs_info->action);
                    if (BCM_SUCCESS (rv)) {
                        trvs_info->key_type = bcmVlanTranslateKeyPortOuter;
                        trvs_info->outer_vlan = min_vlan[i];
                        trvs_info->outer_vlan_high = max_vlan[i];
                        rv = trvs_info->int_cb(unit, trvs_info, &stop);
                        if (BCM_FAILURE(rv) || TRUE == stop) {
                            break;
                        }
                    } else {
                        if (BCM_E_NOT_FOUND == rv) {
                            if (SOC_MEM_FIELD_VALID(unit,
                                        SOURCE_TRUNK_MAP_TABLEm,
                                        INNER_VLAN_RANGE_IDXf)) {
                                for (j = 0; j < 8; j++) {
                                    if ((1 == min_inner_vlan[j]) && 
                                        (0 == max_inner_vlan[j])) {
                                        continue;
                                    }
                                    trvs_info->inner_vlan = BCM_VLAN_INVALID;
                                    trvs_info->inner_vlan_high =
                                        BCM_VLAN_INVALID;
                                    trvs_info->outer_vlan = BCM_VLAN_INVALID;
                                    trvs_info->outer_vlan_high =
                                        BCM_VLAN_INVALID;
                                    /* check if entry key_type is 
                                     * bcmVlanTranslateKeyPortDouble */
                                    rv =
                                    _bcm_trx_vlan_translate_action_range_get(
                                            unit, gport,
                                            min_vlan[i], max_vlan[i],
                                            min_inner_vlan[j],
                                            max_inner_vlan[j],
                                            trvs_info->action);
                                    if (BCM_SUCCESS (rv)) {
                                        trvs_info->key_type =
                                            bcmVlanTranslateKeyPortDouble;
                                        trvs_info->outer_vlan = min_vlan[i];
                                        trvs_info->outer_vlan_high =
                                            max_vlan[i];
                                        trvs_info->inner_vlan =
                                            min_inner_vlan[j];
                                        trvs_info->inner_vlan_high =
                                            max_inner_vlan[j];
                                        rv = trvs_info->int_cb(unit,
                                                trvs_info, &stop);
                                        if (BCM_FAILURE(rv) || TRUE == stop) {
                                            break;
                                        }
                                    } else {
                                        if (BCM_E_NOT_FOUND != rv) {
                                            soc_cm_sfree(unit, stm_tbl_chnk);
                                            return rv;
                                        }
                                    }
                                } /*endof for j=0;j<8;j++*/
                            }
                        } else {
                            soc_cm_sfree(unit, stm_tbl_chnk);
                            return rv;
                        }
                    }
                } /*endof for i=0;i<8;i++ */
                
                if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                            INNER_VLAN_RANGE_IDXf)) {
                    for (j = 0; j < 8; j++) {
                        if ((1 == min_inner_vlan[j]) && 
                            (0 == max_inner_vlan[j])) {
                            continue;
                        }
                        trvs_info->inner_vlan = BCM_VLAN_INVALID;
                        trvs_info->inner_vlan_high = BCM_VLAN_INVALID;
                        trvs_info->outer_vlan = BCM_VLAN_INVALID;
                        trvs_info->outer_vlan_high = BCM_VLAN_INVALID;
                        /* check if entry key_type is
                           bcmVlanTranslateKeyPortInner */
                        rv = _bcm_trx_vlan_translate_action_range_get(
                                unit, gport, 
                                BCM_VLAN_INVALID, BCM_VLAN_INVALID,
                                min_inner_vlan[j], max_inner_vlan[j], 
                                trvs_info->action);
                        if (BCM_SUCCESS (rv)) {
                            trvs_info->key_type = bcmVlanTranslateKeyPortInner;
                            trvs_info->inner_vlan = min_inner_vlan[j];
                            trvs_info->inner_vlan_high = max_inner_vlan[j];
                            rv = trvs_info->int_cb(unit, trvs_info, &stop);
                            if (BCM_FAILURE(rv) || TRUE == stop) {
                                break;
                            }
                        } else {
                            if (BCM_E_NOT_FOUND != rv) {
                                soc_cm_sfree(unit, stm_tbl_chnk);
                                return rv;
                            }
                        }
                    }
                }
            } else {
                range_idx = soc_mem_field32_get(unit, mem, stm_ent, field);
                _bcm_trx_vlan_range_profile_entry_get(unit, min_vlan, max_vlan,
                        range_idx);

                if ((1 == min_vlan[0]) && (0 == max_vlan[0])) {
                    continue;
                }

                stm_index = chnk_idx + ent_idx; 
                rv = _bcm_trx_vlan_translate_stm_index_to_gport(unit, stm_index,
                        &gport);
                if (BCM_FAILURE(rv)) {
                    break;
                }
                trvs_info->gport = gport;

                for (i = 0; i < 8; i++) {
                    if ((1 == min_vlan[i]) && (0 == max_vlan[i])) {
                        continue;
                    }
                    trvs_info->inner_vlan = BCM_VLAN_INVALID;
                    trvs_info->inner_vlan_high = BCM_VLAN_INVALID;
                    trvs_info->outer_vlan = BCM_VLAN_INVALID;
                    trvs_info->outer_vlan_high = BCM_VLAN_INVALID;

                    /* check if entry key_type is
                     * bcmVlanTranslateKeyPortOuter */
                    rv = _bcm_trx_vlan_translate_action_range_get(unit, 
                            gport, 
                            min_vlan[i], 
                            max_vlan[i], 
                            BCM_VLAN_INVALID,
                            BCM_VLAN_INVALID,
                            trvs_info->action);
                    if (BCM_SUCCESS (rv)) {
                        trvs_info->key_type = bcmVlanTranslateKeyPortOuter;
                        trvs_info->outer_vlan = min_vlan[i];
                        trvs_info->outer_vlan_high = max_vlan[i];
                    } else {
                        if (BCM_E_NOT_FOUND == rv) {
                            /* check if entry key_type is
                               bcmVlanTranslateKeyPortInner */
                            rv = _bcm_trx_vlan_translate_action_range_get(unit, 
                                    gport, 
                                    BCM_VLAN_INVALID,
                                    BCM_VLAN_INVALID,
                                    min_vlan[i], 
                                    max_vlan[i], 
                                    trvs_info->action);
                            if (BCM_FAILURE(rv)) {
                                soc_cm_sfree(unit, stm_tbl_chnk);
                                return rv;
                            }
                            trvs_info->key_type = bcmVlanTranslateKeyPortInner;
                            trvs_info->inner_vlan = min_vlan[i];
                            trvs_info->inner_vlan_high = max_vlan[i];
                        } else {
                            soc_cm_sfree(unit, stm_tbl_chnk);
                            return rv;
                        }
                    }

                    rv = trvs_info->int_cb(unit, trvs_info, &stop);
                    if (BCM_FAILURE(rv) || TRUE == stop) {
                        break;
                    }
                }
            }
        }
    }
    soc_cm_sfree(unit, stm_tbl_chnk);
    return rv;        

}

/*
 * Function:
 *      _bcm_trx_vlan_translate_egress_entry_assemble
 * Purpose:
 *      Constructs vlan translate egress entry from given port class,
 *      inner and outer vlans
 * Parameters:
 *      unit            (IN) BCM unit number
 *      vent            (IN/OUT) vlan translate egress entry to construct
 *      port_class      (IN) Group ID of ingress port
 *      outer_vlan      (IN) Packet outer VLAN ID
 *      inner_vlan      (IN) Packet inner VLAN ID
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_trx_vlan_translate_egress_entry_assemble(int unit,
                                              egr_vlan_xlate_entry_t *vent,
                                              int port_class,
                                              bcm_vlan_t outer_vlan,
                                              bcm_vlan_t inner_vlan)
{
#ifdef BCM_TRIUMPH2_SUPPORT
    int tunnel_id = 0;
    int dvp_id = BCM_GPORT_INVALID;
#endif

    VLAN_CHK_ID(unit, outer_vlan);
    VLAN_CHK_ID(unit, inner_vlan);

#ifdef BCM_TRIUMPH2_SUPPORT
    if (BCM_GPORT_IS_TUNNEL(port_class)) {
        if (soc_feature(unit, soc_feature_wlan)) {
            tunnel_id = BCM_GPORT_TUNNEL_ID_GET(port_class);
        } else {
            return BCM_E_PORT;
        }
    } else if (BCM_GPORT_IS_SUBPORT_PORT(port_class)) {
        if (soc_feature(unit, soc_feature_subport)) {
            dvp_id = BCM_GPORT_SUBPORT_PORT_GET(port_class);
        } else {
            return BCM_E_PORT;
        }
    } else if (BCM_GPORT_IS_MIM_PORT(port_class)) {
        if (soc_feature(unit, soc_feature_mim)) {
            dvp_id = BCM_GPORT_MIM_PORT_ID_GET(port_class);
        } else {
            return BCM_E_PORT;
        }
        
    } else if (BCM_GPORT_IS_WLAN_PORT(port_class)) {
        if (soc_feature(unit, soc_feature_wlan)) {
            dvp_id = BCM_GPORT_WLAN_PORT_ID_GET(port_class);
        } else {
            return BCM_E_PORT;
        }
    } else if (BCM_GPORT_IS_VLAN_PORT(port_class)) {
        if (soc_feature(unit, soc_feature_vlan_vp)) {
            dvp_id = BCM_GPORT_VLAN_PORT_ID_GET(port_class);
        } else {
            return BCM_E_PORT;
        }
    } else if (BCM_GPORT_IS_NIV_PORT(port_class)) {
        if (soc_feature(unit, soc_feature_niv)) {
            dvp_id = BCM_GPORT_NIV_PORT_ID_GET(port_class);
        } else {
            return BCM_E_PORT;
        }
    } else if (BCM_GPORT_IS_MPLS_PORT(port_class)) {
        if (soc_feature(unit, soc_feature_mpls)) {
            dvp_id = BCM_GPORT_MPLS_PORT_ID_GET(port_class);
        } else {
            return BCM_E_PORT;
        }
    }
#endif

    sal_memset(vent, 0, sizeof(*vent));
    soc_EGR_VLAN_XLATEm_field32_set(unit, vent, OVIDf, outer_vlan);
    soc_EGR_VLAN_XLATEm_field32_set(unit, vent, IVIDf, inner_vlan);
#ifdef BCM_TRIUMPH2_SUPPORT
    if (BCM_GPORT_IS_TUNNEL(port_class)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, vent, ENTRY_TYPEf, 2); /* WLAN */
        soc_EGR_VLAN_XLATEm_field32_set(unit, vent, TUNNEL_IDf, tunnel_id);
    } else if (BCM_GPORT_INVALID != dvp_id) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, vent, ENTRY_TYPEf, 1); /* DVP */
        soc_EGR_VLAN_XLATEm_field32_set(unit, vent, DVPf, dvp_id);
    } else 
#endif
    {
        soc_EGR_VLAN_XLATEm_field32_set(unit, vent, PORT_GROUP_IDf, port_class);
    }
    return BCM_E_NONE;
}

/*
 * Function   :
 *      bcm_vlan_translate_egress_action_add
 * Description   :
 *      Add an entry to egress VLAN translation table.
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      port_class      (IN) Group ID of ingress port
 *      outer_vlan      (IN) Packet outer VLAN ID
 *      inner_vlan      (IN) Packet inner VLAN ID
 *      action          (IN) Action for outer and inner tag
 */
int 
_bcm_trx_vlan_translate_egress_action_add(int unit, int port_class,
                                         bcm_vlan_t outer_vlan,
                                         bcm_vlan_t inner_vlan,
                                         bcm_vlan_action_set_t *action)
{
    int rv;
    uint32 profile_idx;
    uint32 old_profile_idx = 0;
    egr_vlan_xlate_entry_t vent;
    egr_vlan_xlate_entry_t vent_old;
    egr_vlan_xlate_entry_t * vent_ptr;
    int index;
    
    BCM_IF_ERROR_RETURN(_bcm_trx_egr_vlan_action_verify(unit, action));

    vent_ptr = &vent;
    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_translate_egress_entry_assemble(unit, vent_ptr,
                                                       port_class,
                                                       outer_vlan,
                                                       inner_vlan));

    soc_mem_lock(unit, EGR_VLAN_XLATEm);
    /* check if the entry with same key already exists */
    rv = soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                        vent_ptr, &vent_old, 0);

    if (rv == SOC_E_NONE) {
        /* alredy exists, update on the existing one, so the flex
         * counter related fields in TR3/Katana can be preserved.
         */
        vent_ptr = &vent_old;
        old_profile_idx = soc_EGR_VLAN_XLATEm_field32_get(unit, vent_ptr,
                                               TAG_ACTION_PROFILE_PTRf); 
    } else if (rv != SOC_E_NOT_FOUND) {
        soc_mem_unlock(unit, EGR_VLAN_XLATEm);
        return rv;  /* error condition */
    } else {
        /* doesn't exist, update the new entry */
        vent_ptr = &vent; 
    }
 
    soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, NEW_IVIDf, 
                                    action->new_inner_vlan);
    soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, NEW_OVIDf, 
                                    action->new_outer_vlan);

    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        bcm_vlan_action_set_t action1;
        sal_memcpy(&action1, action, sizeof(bcm_vlan_action_set_t));
        if (action->priority == -1) {
            /* point to the default mapping table(0) for default priority action */
            soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, OPRI_CFI_SELf, 1);
            soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, IPRI_CFI_SELf, 1);
    
#define PKT_PRI_DFLT_ACTION(_action_tag) \
   do { \
      if (((_action_tag) == bcmVlanActionReplace) || \
                ((_action_tag) == bcmVlanActionAdd) ) { \
                _action_tag ## _pkt_prio = _action_tag; \
      } \
   } while(0)
            PKT_PRI_DFLT_ACTION(action1.dt_outer);
            PKT_PRI_DFLT_ACTION(action1.dt_inner);
            PKT_PRI_DFLT_ACTION(action1.ot_outer);
            PKT_PRI_DFLT_ACTION(action1.ot_inner);
            PKT_PRI_DFLT_ACTION(action1.it_outer);
            PKT_PRI_DFLT_ACTION(action1.it_inner);
            PKT_PRI_DFLT_ACTION(action1.ut_outer);
            PKT_PRI_DFLT_ACTION(action1.ut_inner);
#undef PKT_PRI_DFLT_ACTION
        } else {
            soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, NEW_OPRIf,
                                        action->priority);
            soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, NEW_OCFIf,
                                        action->new_outer_cfi);
            soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, NEW_IPRIf,
                                        action->new_inner_pkt_prio);
            soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, NEW_ICFIf,
                                        action->new_inner_cfi);
        }
        BCM_IF_ERROR_RETURN
            (_bcm_trx_egr_vlan_action_profile_entry_add(unit, &action1,
                                                    &profile_idx));
    } else {
        if (action->priority >= BCM_PRIO_MIN &&
            action->priority <= BCM_PRIO_MAX) {
            soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, RPEf, 1);
            soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, PRIf,
                                            action->priority);
            if (bcmVlanActionNone != action->it_inner_prio ||
                bcmVlanActionNone != action->dt_inner_prio) {                                           
                soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, NEW_IRPEf, 1);
                soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, NEW_IPRIf,
                                            action->priority);
            }
        }
        BCM_IF_ERROR_RETURN
            (_bcm_trx_egr_vlan_action_profile_entry_add(unit, action,
                                                    &profile_idx));
    }
    soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, TAG_ACTION_PROFILE_PTRf,
                                    profile_idx);
    soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, NEW_OTAG_VPTAG_SELf, 0);
    soc_EGR_VLAN_XLATEm_field32_set(unit, vent_ptr, VALIDf, 1);

    if (rv == SOC_E_NONE) {
        /* write back the existing one */
        rv = soc_mem_write(unit, EGR_VLAN_XLATEm,
                             MEM_BLOCK_ALL, index, vent_ptr);
        if (rv == SOC_E_NONE) {
            rv = _bcm_trx_egr_vlan_action_profile_entry_delete(unit,
                           old_profile_idx);
        }
    } else {
        /* insert new one */
        rv = soc_mem_insert(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, vent_ptr);
    }
    soc_mem_unlock(unit, EGR_VLAN_XLATEm);

    return rv;
}

/*
 * Function:
 *      _bcm_trx_vlan_translate_egress_action_get
 * Purpose:
 *      Get an egress vlan translate entry.
 * Parameters:
 *      unit            (IN) BCM unit number
 *      port_class      (IN) Group ID of ingress port
 *      old_vid         (IN) Packet old VLAN ID to match
 *      new_vid         (OUT) Packet new VLAN ID
 *      prio            (OUT) Translation priority
 */
extern int 
_bcm_trx_vlan_translate_egress_action_get(int unit, int port_class,
                                         bcm_vlan_t outer_vlan,
                                         bcm_vlan_t inner_vlan,
                                         bcm_vlan_action_set_t *action)
{
    egr_vlan_xlate_entry_t  vent;
    egr_vlan_xlate_entry_t  res_vent;
    int                     rv;
    int                     idx = 0, profile_idx;

    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_translate_egress_entry_assemble(unit, &vent,
                                                       port_class,
                                                       outer_vlan,
                                                       inner_vlan));

    sal_memset(&res_vent, 0, sizeof(egr_vlan_xlate_entry_t));
    soc_mem_lock(unit, EGR_VLAN_XLATEm);
    rv = soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &idx, 
                        &vent, &res_vent, 0);
    soc_mem_unlock(unit, EGR_VLAN_XLATEm);
    BCM_IF_ERROR_RETURN(rv);
    profile_idx = soc_EGR_VLAN_XLATEm_field32_get(unit, &res_vent,
                                              TAG_ACTION_PROFILE_PTRf);       
    _bcm_trx_egr_vlan_action_profile_entry_get(unit, action, profile_idx);
   
    BCM_IF_ERROR_RETURN(
        _bcm_trx_vlan_translate_entry_parse(unit, EGR_VLAN_XLATEm,
                                            (uint32 *)&res_vent, action));
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_vlan_translate_egress_action_delete
 * Purpose:
 *      Delete an egress vlan translate lookup entry.
 * Parameters:
 *      unit            (IN) BCM unit number
 *      port_class      (IN) Group ID of ingress port
 *      outer_vlan      (IN) Packet outer VLAN ID
 *      inner_vlan      (IN) Packet inner VLAN ID
 */
int 
_bcm_trx_vlan_translate_egress_action_delete(int unit, int port_class,
                                            bcm_vlan_t outer_vlan,
                                            bcm_vlan_t inner_vlan)
{
    egr_vlan_xlate_entry_t vent;
    int rv; 
    uint32 profile_idx;
#if defined(BCM_TRIUMPH2_SUPPORT)
    _bcm_flex_stat_handle_t handle;
    _BCM_FLEX_STAT_HANDLE_CLEAR(handle);
#endif

    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_translate_egress_entry_assemble(unit, &vent,
                                                       port_class,
                                                       outer_vlan,
                                                       inner_vlan));

#if defined(BCM_TRIUMPH2_SUPPORT)
    if (soc_feature(unit, soc_feature_gport_service_counters)) {
        /* Record flex stat info before the returned entry
         * includes non-key info. */

        _BCM_FLEX_STAT_HANDLE_COPY(handle, vent);
    }
#endif

    rv = soc_mem_delete_return_old(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &vent, &vent);
    if ((rv == SOC_E_NONE) && soc_EGR_VLAN_XLATEm_field32_get(unit, &vent, VALIDf)) {
        profile_idx = soc_EGR_VLAN_XLATEm_field32_get(unit, &vent,
                                                      TAG_ACTION_PROFILE_PTRf);       
        /* Delete the old vlan action profile entry */
        rv = _bcm_trx_egr_vlan_action_profile_entry_delete(unit, profile_idx);

#if defined(BCM_TRIUMPH2_SUPPORT)
        if (soc_feature(unit, soc_feature_gport_service_counters) &&
            (0 != soc_EGR_VLAN_XLATEm_field32_get(unit, &vent,
                                                  VINTF_CTR_IDXf))) {
            /* Release Service counter */
            _bcm_esw_flex_stat_ext_handle_free(unit, _bcmFlexStatTypeVxlt,
                                               handle);
        }
#endif
    }
    return rv;
}

/*
 * Function:
 *      bcm_vlan_translate_egress_action_delete_all
 * Purpose:
 *      Delete all egress vlan translate lookup entries.
 * Parameters:
 *      unit      (IN) BCM unit number
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_translate_egress_action_delete_all(int unit) 
{
    int i, imin, imax, nent, vbytes, rv;
    uint32 old_profile_idx;
    egr_vlan_xlate_entry_t * vtab, * vnull,  *vtabp;
#if defined(BCM_TRIUMPH2_SUPPORT)
    _bcm_flex_stat_handle_t handle;
    egr_vlan_xlate_entry_t vent;
    uint32 key[2];
#endif
    
    imin = soc_mem_index_min(unit, EGR_VLAN_XLATEm);
    imax = soc_mem_index_max(unit, EGR_VLAN_XLATEm);
    nent = soc_mem_index_count(unit, EGR_VLAN_XLATEm);
    vbytes = soc_mem_entry_words(unit, EGR_VLAN_XLATEm);
    vbytes = WORDS2BYTES(vbytes);
    vtab = soc_cm_salloc(unit, nent * sizeof(*vtab), "egr_vlan_xlate");

    if (vtab == NULL) {
        return BCM_E_MEMORY;
    }
    
    vnull = soc_mem_entry_null(unit, EGR_VLAN_XLATEm);

    soc_mem_lock(unit, EGR_VLAN_XLATEm);
    rv = soc_mem_read_range(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ANY,
                            imin, imax, vtab);
    if (rv < 0) {
        soc_mem_unlock(unit, EGR_VLAN_XLATEm);
        soc_cm_sfree(unit, vtab);
        return rv; 
    }
    
    for(i = 0; i < nent; i++) {
        vtabp = soc_mem_table_idx_to_pointer(unit, EGR_VLAN_XLATEm,
                                             egr_vlan_xlate_entry_t *,
                                             vtab, i);

        if (!soc_EGR_VLAN_XLATEm_field32_get(unit, vtabp, VALIDf)) {
            continue;
        }
        old_profile_idx =
            soc_EGR_VLAN_XLATEm_field32_get(unit, vtabp,
                                            TAG_ACTION_PROFILE_PTRf);

        rv = WRITE_EGR_VLAN_XLATEm(unit, MEM_BLOCK_ANY, i, vnull);

        /* Delete the old vlan action profile entry */
        if (rv == SOC_E_NONE) {
            rv = _bcm_trx_egr_vlan_action_profile_entry_delete(unit, 
                                                        old_profile_idx);

#if defined(BCM_TRIUMPH2_SUPPORT)
        if (soc_feature(unit, soc_feature_gport_service_counters) &&
            (0 != soc_EGR_VLAN_XLATEm_field32_get(unit, vtabp,
                                                  VINTF_CTR_IDXf))) {
            sal_memset(&vent, 0, sizeof(vent));
            /* Construct key-only entry, copy to FS handle */
            soc_EGR_VLAN_XLATEm_field32_set(unit, &vent, ENTRY_TYPEf,
                soc_EGR_VLAN_XLATEm_field32_get(unit, vtabp, ENTRY_TYPEf));
            soc_mem_field_get(unit, EGR_VLAN_XLATEm, (uint32 *) vtabp,
                               KEYf, (uint32 *) key);
            soc_mem_field_set(unit, EGR_VLAN_XLATEm, (uint32 *) &vent,
                               KEYf, (uint32 *) key);

            _BCM_FLEX_STAT_HANDLE_CLEAR(handle);
            _BCM_FLEX_STAT_HANDLE_COPY(handle, vent);

            /* Release Service counter */
            _bcm_esw_flex_stat_ext_handle_free(unit, _bcmFlexStatTypeVxlt,
                                               handle);
        }
#endif
        }
    }
    
    soc_mem_unlock(unit, EGR_VLAN_XLATEm);
    soc_cm_sfree(unit, vtab);
    return rv;
}

/*
 * Function   :
 *      _bcm_trx_vlan_translate_old_vlan_get
 * Description   :
 *      Helper function to get an old vid from vlan translate entry 
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      mem             (IN) Vlan translate memory id.
 *      vent            (IN) Vlan translate entry
 *      key_type        (IN) Key type to determine which old vid to get
 *      outer_vlan      (OUT) outer vlan to retrieve
 *      inner_vlan      (OUT) inner vlan to retrieve
 */

STATIC int 
_bcm_trx_vlan_translate_old_vlan_get(int unit, soc_mem_t mem, 
                                     uint32 *vent, uint32 key_type,
                                     bcm_vlan_t *outer_vlan, 
                                     bcm_vlan_t *inner_vlan)
{
    bcm_vlan_t  tmp_o_vlan = BCM_VLAN_INVALID;
    bcm_vlan_t  tmp_i_vlan = BCM_VLAN_INVALID;

    /* Input parameters check. */ 
    if ((NULL  == vent) || (NULL == outer_vlan) || 
        (INVALIDm == mem) || (NULL == inner_vlan)) {
        return (BCM_E_PARAM);
    }

    switch (key_type) {
      case bcmVlanTranslateKeyDouble:
      case bcmVlanTranslateKeyPortDouble:
          tmp_o_vlan = soc_mem_field32_get(unit, mem, vent, OVIDf);
          tmp_i_vlan = soc_mem_field32_get(unit, mem, vent, IVIDf);
          break;
      case bcmVlanTranslateKeyOuterTag:
      case bcmVlanTranslateKeyPortOuterTag:
          tmp_o_vlan = soc_mem_field32_get(unit, mem, vent, OTAGf);
          break;
      case bcmVlanTranslateKeyInnerTag:
      case bcmVlanTranslateKeyPortInnerTag:
          tmp_i_vlan = soc_mem_field32_get(unit, mem, vent, ITAGf);
          break;
      case bcmVlanTranslateKeyOuter:
      case bcmVlanTranslateKeyPortOuter:
          tmp_o_vlan = soc_mem_field32_get(unit, mem, vent, OVIDf);
          break;
      case bcmVlanTranslateKeyInner:
      case bcmVlanTranslateKeyPortInner:
          tmp_i_vlan = soc_mem_field32_get(unit, mem, vent, IVIDf);
          break;
      default:
          return BCM_E_PARAM;
    }

    *outer_vlan = tmp_o_vlan;
    *inner_vlan = tmp_i_vlan;

    return BCM_E_NONE;
}

/*
 * Function   :
 *      _bcm_trx_vlan_translate_gport_get
 * Description   :
 *      Helper function to get a gport from vlan translate entry 
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      mem             (IN) Vlan translate memory id.
 *      vent            (IN) Vlan translate entry
 *      gport           (OUT) gport to retrieve
 */

STATIC int
_bcm_trx_vlan_translate_gport_get(int unit, soc_mem_t mem,
                                  uint32 *vent, bcm_gport_t *gport)
{
    bcm_gport_t     tmp_gport = 0;
    uint32          modid;
    uint32          port;
    int             glp_wildcard;


    /* Input parameters check. */
    if ((NULL == vent) || (NULL == gport) || (INVALIDm == mem)) {
        return (BCM_E_PARAM);
    }

    glp_wildcard = (soc_mem_field32_get(unit, mem, vent, GLPf) == 
                    SOC_VLAN_XLATE_GLP_WILDCARD(unit));
    if (glp_wildcard) {
        tmp_gport = BCM_GPORT_INVALID;
    } else {
        if (soc_mem_field32_get(unit, mem, vent, Tf)) {
            port = soc_mem_field32_get(unit, mem, vent, TGIDf); 
            BCM_GPORT_TRUNK_SET(tmp_gport, port); 
        } else {
            port = soc_mem_field32_get(unit, mem, vent, PORT_NUMf); 
            modid = soc_mem_field32_get(unit, mem, vent, MODULE_IDf); 
            BCM_GPORT_MODPORT_SET(tmp_gport, modid, port); 
        }
    }

    *gport = tmp_gport;
    return BCM_E_NONE;
}

/*
 * Function   :
 *      _bcm_trx_vlan_translate_key_type_get
 * Description   :
 *      Helper function to get a key_type from vlan translate entry 
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      mem             (IN) Vlan translate memory id.
 *      vent            (IN) Vlan translate entry
 *      key_type        (OUT) Key type to retrieve
 */

STATIC int 
_bcm_trx_vlan_translate_key_type_get(int unit, soc_mem_t mem, 
                                     uint32 *vent, uint32 *key_type)
{
    uint32 key_val, glp_wildcard;
    int vt_key;

    /* Input parameters check. */
    if ((NULL == vent) || (NULL == key_type) || (INVALIDm == mem)) {
        return (BCM_E_PARAM);
    }

#ifdef BCM_TRIUMPH3_SUPPORT
    if (SOC_IS_TRIUMPH3(unit)) {
        if (mem ==  VLAN_XLATE_EXTDm) {
            key_val = soc_mem_field32_get(unit, mem, vent, KEY_TYPE_0f);
            key_val &= ~1; /*remove extd entry indication */ 
        } else {
            key_val = soc_mem_field32_get(unit, mem, vent, KEY_TYPEf); 
        }
    } else
#endif /* BCM_TRIUMPH3_SUPPORT */
    {
        key_val = soc_mem_field32_get(unit, mem, vent, KEY_TYPEf);
    } 
    glp_wildcard = (soc_mem_field32_get(unit, mem, vent, GLPf) == 
                    SOC_VLAN_XLATE_GLP_WILDCARD(unit));

    BCM_IF_ERROR_RETURN
        (_bcm_esw_vlan_xlate_key_type_get(unit,key_val,&vt_key));
    switch (vt_key) {
      case VLXLT_HASH_KEY_TYPE_IVID_OVID:
          {
              if (glp_wildcard) {
                  *key_type = bcmVlanTranslateKeyDouble;
              } else {

              }   *key_type = bcmVlanTranslateKeyPortDouble;
              break;
          }
      case VLXLT_HASH_KEY_TYPE_OTAG:
          {
              if (glp_wildcard) {
                  *key_type = bcmVlanTranslateKeyOuterTag;
              } else {
                  *key_type = bcmVlanTranslateKeyPortOuterTag;
              }
              break;
          }
      case VLXLT_HASH_KEY_TYPE_ITAG:
          {
              if (glp_wildcard) {
                  *key_type = bcmVlanTranslateKeyInnerTag;
              } else {
                  *key_type = bcmVlanTranslateKeyPortInnerTag;
              }
              break;
          }
      case VLXLT_HASH_KEY_TYPE_OVID:
          {
              if (glp_wildcard) {
                  *key_type = bcmVlanTranslateKeyOuter;
              } else {
                  *key_type = bcmVlanTranslateKeyPortOuter;
              }
              break;
          }
      case VLXLT_HASH_KEY_TYPE_IVID: 
          {
              if (glp_wildcard) {
                  *key_type = bcmVlanTranslateKeyInner;
              } else {
                  *key_type = bcmVlanTranslateKeyPortInner;
              }
              break;
          }
      default:
          return BCM_E_NOT_FOUND;
          break;
    }

    return BCM_E_NONE;
}

/*
 * Function   :
 *      _bcm_trx_vlan_translate_parse
 * Description   :
 *      Helper function for an API to parse a vlan translate 
 *      entry for Triumph and call given call back.
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      mem             (IN) Vlan translation memory id.
 *      vent            (IN) HW entry.
 *      trvs_info       (IN/OUT) Traverse structure that contain all relevant info
 */
int 
_bcm_trx_vlan_translate_parse(int unit, soc_mem_t mem, uint32 *vent,
                             _bcm_vlan_translate_traverse_t *trvs_info)
{
    uint32          profile_idx = 0;     /* Vlan profile index.      */

#if defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3)
    if (soc_feature(unit, soc_feature_niv) && (mem == VLAN_XLATEm)) {
        uint32 key_val;
        int vt_key;
        int vif_hit = TRUE;
        int niv_port_id;
        int vp;

        key_val = soc_mem_field32_get(unit, mem, vent, KEY_TYPEf);
        BCM_IF_ERROR_RETURN(_bcm_esw_vlan_xlate_key_type_get(unit,
              key_val,&vt_key));
        switch (vt_key) {
            case VLXLT_HASH_KEY_TYPE_VIF_OTAG:
                trvs_info->key_type = bcmVlanTranslateKeyPortOuterTag;
                trvs_info->outer_vlan = soc_mem_field32_get(unit, mem, 
                           vent, VIF__OTAGf);
                break;

            case VLXLT_HASH_KEY_TYPE_VIF_ITAG:
                trvs_info->key_type = bcmVlanTranslateKeyPortInnerTag;
                trvs_info->inner_vlan = soc_mem_field32_get(unit, mem, 
                           vent, VIF__ITAGf);
                break;

            case VLXLT_HASH_KEY_TYPE_VIF_VLAN:
                trvs_info->key_type = bcmVlanTranslateKeyPortOuter;
                trvs_info->outer_vlan = soc_mem_field32_get(unit, mem, 
                           vent, VIF__VLANf);
                break;

            case VLXLT_HASH_KEY_TYPE_VIF_CVLAN:
                trvs_info->key_type = bcmVlanTranslateKeyPortInner;
                trvs_info->inner_vlan = soc_mem_field32_get(unit, mem, 
                           vent, VIF__CVLANf);
                break;
            default:
                vif_hit = FALSE;
                break;
        }
        if (vif_hit == TRUE) {
            vp = soc_mem_field32_get(unit, mem, vent, VIF__SOURCE_VPf);
            BCM_GPORT_NIV_PORT_ID_SET(niv_port_id, vp);
            profile_idx = soc_mem_field32_get(unit, mem, vent,
                                      VIF__TAG_ACTION_PROFILE_PTRf); 
            _bcm_trx_vlan_action_profile_entry_get(unit, trvs_info->action,
                                               profile_idx);
            trvs_info->gport = niv_port_id;
            BCM_IF_ERROR_RETURN(_bcm_trx_vif_vlan_translate_entry_parse(unit, 
                     mem, vent, trvs_info->action));
            return BCM_E_NONE;
        }
    }
#endif   /* defined(BCM_TRIDENT_SUPPORT) && defined(INCLUDE_L3) */
 
    profile_idx = soc_mem_field32_get(unit, mem, vent,
                                      TAG_ACTION_PROFILE_PTRf); 
    if (mem == EGR_VLAN_XLATEm) {
        _bcm_trx_egr_vlan_action_profile_entry_get(unit, 
                                                   trvs_info->action, 
                                                   profile_idx);
        trvs_info->port_class = 
            soc_mem_field32_get(unit, mem, vent, PORT_GROUP_IDf);
        trvs_info->gport = BCM_GPORT_INVALID;

        trvs_info->key_type = bcmVlanTranslateKeyPortDouble;
    } else {
        _bcm_trx_vlan_action_profile_entry_get(unit, trvs_info->action, 
                                               profile_idx);

        BCM_IF_ERROR_RETURN(
             _bcm_trx_vlan_translate_key_type_get(unit, mem, vent,
                                                  &(trvs_info->key_type)));
        BCM_IF_ERROR_RETURN(
            _bcm_trx_vlan_translate_gport_get(unit, mem, vent, 
                                              &(trvs_info->gport)));
        trvs_info->port_class = BCM_GPORT_INVALID;
    }

    BCM_IF_ERROR_RETURN(
        _bcm_trx_vlan_translate_entry_parse(unit, mem, vent,
                                            trvs_info->action));

    return _bcm_trx_vlan_translate_old_vlan_get(unit, mem, vent, 
                                              trvs_info->key_type,
                                              &(trvs_info->outer_vlan), 
                                              &(trvs_info->inner_vlan));
}

/*
*       VLAN PORT PROTOCOL 
*/

/*
 * Function:
 *      _bcm_trx_vlan_port_protocol_entry_parse
 * Purpose:
 *      Parses VLAN protocol entry and extracts frame type and ethertype
 * Parameters:
 *      unit        -     (IN) BCM Device unit
 *      vpe         -     (IN) VLAN protocol entry to parse
 *      frame       -     (OUT) Frame type from the entry
 *      ether       -     (IN) Ethertype from the entry
 * Returns:
 *      None
 */
STATIC void
_bcm_trx_vlan_port_protocol_entry_parse(int unit, vlan_protocol_entry_t *vpe,
                                        bcm_port_frametype_t *frame,
                                        bcm_port_ethertype_t *ether)
{
    *frame = 0;
    if (soc_VLAN_PROTOCOLm_field32_get(unit, vpe, ETHERIIf)) {
        *frame |= BCM_PORT_FRAMETYPE_ETHER2;
    }
    if (soc_VLAN_PROTOCOLm_field32_get(unit, vpe, SNAPf)) {
        *frame |= BCM_PORT_FRAMETYPE_8023;
    }
    if (soc_VLAN_PROTOCOLm_field32_get(unit, vpe, LLCf)) {
        *frame |= BCM_PORT_FRAMETYPE_LLC;
    }
    *ether = soc_VLAN_PROTOCOLm_field32_get(unit, vpe, ETHERTYPEf);
}

/*
 * Function:
 *      _bcm_trx_vlan_port_prot_match_get
 * Purpose:
 *      Get index into table with settings matching to port protocol
 * Parameters:
 *      unit        -     (IN) BCM Device unit
 *      frame       -     (IN) Frmae to match
 *      ether       -     (IN) Ethertype to match
 *      match_idx   -     (OUT) index to the table entry if match, -1 otherwise
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_trx_vlan_port_prot_match_get(int unit, bcm_port_frametype_t frame, 
                                  bcm_port_ethertype_t ether, int *match_idx)
{
    int                         i, idxmin, idxmax;
    vlan_protocol_entry_t       vpe;
    bcm_port_frametype_t        ft;
    bcm_port_ethertype_t        et;

    idxmin = soc_mem_index_min(unit, VLAN_PROTOCOLm);
    idxmax = soc_mem_index_max(unit, VLAN_PROTOCOLm);
    
    *match_idx = -1;

    for (i = idxmin; i <= idxmax; i++) {
        SOC_IF_ERROR_RETURN
            (READ_VLAN_PROTOCOLm(unit, MEM_BLOCK_ANY, i, &vpe));
        _bcm_trx_vlan_port_protocol_entry_parse(unit, &vpe, &ft, &et);
        if (ft == frame && et == ether) {
            *match_idx = i;
            break;
        }
    }

    return (*match_idx < 0) ?  BCM_E_NOT_FOUND : BCM_E_NONE ;
}

/*
 * Function:
 *      _bcm_trx_vlan_port_prot_match_get
 * Purpose:
 *      Get index into first empty entry in the table 
 * Parameters:
 *      unit        -     (IN) BCM Device unit
 *      empty_idx   -     (OUT) index to the table entry if match, -1 otherwise
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_trx_vlan_port_prot_empty_get(int unit, int *empty_idx)
{
    int                         i, idxmin, idxmax;
    vlan_protocol_entry_t       vpe;
    bcm_port_frametype_t        ft;
    
    idxmin = soc_mem_index_min(unit, VLAN_PROTOCOLm);
    idxmax = soc_mem_index_max(unit, VLAN_PROTOCOLm);
    
    *empty_idx = -1;

    for (i = idxmin; i <= idxmax; i++) {
        SOC_IF_ERROR_RETURN
            (READ_VLAN_PROTOCOLm(unit, MEM_BLOCK_ANY, i, &vpe));
        ft = 0;
        if (soc_VLAN_PROTOCOLm_field32_get(unit, &vpe, ETHERIIf)) {
            ft |= BCM_PORT_FRAMETYPE_ETHER2;
        }
        if (soc_VLAN_PROTOCOLm_field32_get(unit, &vpe, SNAPf)) {
            ft |= BCM_PORT_FRAMETYPE_8023;
        }
        if (soc_VLAN_PROTOCOLm_field32_get(unit, &vpe, LLCf)) {
            ft |= BCM_PORT_FRAMETYPE_LLC;
        }
        if (0 == ft) {
            *empty_idx = i;
            break;
        }
    }

    return (*empty_idx < 0) ?  BCM_E_FULL : BCM_E_NONE ;
}

/*
 * Function   :
 *      _bcm_trx_vlan_protocol_data_entry_set
 * Description   :
 *      Helper function to set vlan_protocol_data entry
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      ventry          (IN / OUT) Entry to setup
 *      action          (IN) vlan action to be used
 *      profile_idx     (IN) profile index to set
 */
STATIC void 
_bcm_trx_vlan_protocol_data_entry_set(int unit, 
                                      vlan_protocol_data_entry_t *ventry, 
                                      bcm_vlan_action_set_t *action,
                                      int profile_idx)
{
    soc_VLAN_PROTOCOL_DATAm_field32_set(unit, ventry, OVIDf, 
                                        action->new_outer_vlan);
    soc_VLAN_PROTOCOL_DATAm_field32_set(unit, ventry, IVIDf, 
                                        action->new_inner_vlan);

    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        soc_VLAN_PROTOCOL_DATAm_field32_set(unit, ventry, OPRIf,
                                            action->priority);
        soc_VLAN_PROTOCOL_DATAm_field32_set(unit, ventry, OCFIf,
                                            action->new_outer_cfi);
        soc_VLAN_PROTOCOL_DATAm_field32_set(unit, ventry, IPRIf,
                                            action->new_inner_pkt_prio);
        soc_VLAN_PROTOCOL_DATAm_field32_set(unit, ventry, ICFIf,
                                            action->new_inner_cfi);
    } else {
        soc_VLAN_PROTOCOL_DATAm_field32_set(unit, ventry, PRIf,
                                            action->priority);
    }
    soc_VLAN_PROTOCOL_DATAm_field32_set(unit, ventry, TAG_ACTION_PROFILE_PTRf,
                                        profile_idx);
}

/*
 * Function   :
 *      _bcm_trx_vlan_protocol_data_entry_parse
 * Description   :
 *      Helper function to parse vlan_protocol_data entry into action
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      ventry          (IN) Entry to setup
 *      action          (OUT) vlan action to be used
 *      profile_idx     (OUT) profile index 
 */
STATIC void 
_bcm_trx_vlan_protocol_data_entry_parse(int unit, 
                                        vlan_protocol_data_entry_t *ventry, 
                                        bcm_vlan_action_set_t *action,
                                        int *profile_idx)
{
    action->new_outer_vlan = 
        soc_VLAN_PROTOCOL_DATAm_field32_get(unit, ventry, OVIDf);

    action->new_inner_vlan = 
        soc_VLAN_PROTOCOL_DATAm_field32_get(unit, ventry, IVIDf); 

    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        action->priority =
            soc_VLAN_PROTOCOL_DATAm_field32_get(unit, ventry, OPRIf);
        action->new_outer_cfi =
            soc_VLAN_PROTOCOL_DATAm_field32_get(unit, ventry, OCFIf);
        action->new_inner_pkt_prio =
            soc_VLAN_PROTOCOL_DATAm_field32_get(unit, ventry, IPRIf);
        action->new_inner_cfi =
            soc_VLAN_PROTOCOL_DATAm_field32_get(unit, ventry, ICFIf);
    } else {
        action->priority =
            soc_VLAN_PROTOCOL_DATAm_field32_get(unit, ventry, PRIf);
    }
    *profile_idx = 
        soc_VLAN_PROTOCOL_DATAm_field32_get(unit, ventry, 
                                            TAG_ACTION_PROFILE_PTRf);
}

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      _bcm_trx_vlan_port_protocol_action_reinit
 * Description:
 *      Recover data for 'vd_pbvl' in port_info structure.
 * Parameters:
 *      unit            (IN) BCM unit number
 * Notes:
 *      Assumes:
 *      - 'vd_pbvl' data in port_info structure is clear
 *      - 'vlan_prot_ptr' in port_info structure is initialized
 */
STATIC int
_bcm_trx_vlan_port_protocol_action_reinit(int unit)
{
    int                         idxmin, idxmax, i, data_idx;
    vlan_protocol_entry_t       vpe;
    vlan_protocol_data_entry_t  vde;
    bcm_port_frametype_t        ft;
    bcm_port_ethertype_t        et;
    bcm_pbmp_t                  pbmp;
    bcm_port_t                  port;
    bcm_vlan_action_set_t       def_action;
    bcm_vlan_t                  def_ovid, def_ivid;
    bcm_port_config_t           pconf;
    _bcm_port_info_t            *pinfo;

    idxmin = soc_mem_index_min(unit, VLAN_PROTOCOLm);
    idxmax = soc_mem_index_max(unit, VLAN_PROTOCOLm);

    BCM_IF_ERROR_RETURN(bcm_esw_port_config_get(unit, &pconf));
    pbmp = pconf.e; 
    if (soc_feature(unit, soc_feature_cpuport_switched)) {
        BCM_PBMP_OR(pbmp, pconf.cpu);
    }
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit) && soc_feature(unit, soc_feature_flex_port)) {
        BCM_IF_ERROR_RETURN(_bcm_kt2_flexio_pbmp_update(unit, &pbmp));
    }
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        _bcm_kt2_subport_pbmp_update(unit, &pbmp);
    }
#endif

    for (i = idxmin; i <= idxmax; i++) {
        SOC_IF_ERROR_RETURN
            (READ_VLAN_PROTOCOLm(unit, MEM_BLOCK_ANY, i, &vpe));
        _bcm_trx_vlan_port_protocol_entry_parse(unit, &vpe, &ft, &et);
        if (0 == ft) {
            continue;
        }

        PBMP_ITER(pbmp, port) {
            BCM_IF_ERROR_RETURN(
                bcm_esw_vlan_port_default_action_get(unit, port, &def_action));
            BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));
            data_idx = pinfo->vlan_prot_ptr + i;
            SOC_IF_ERROR_RETURN(
                READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY, data_idx, &vde));
            def_ovid = soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, OVIDf);
            def_ivid = soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, IVIDf);
            if (def_ivid == def_action.new_inner_vlan &&
                def_ovid == def_action.new_outer_vlan) {
                continue;
            }

            _BCM_PORT_VD_PBVL_SET(pinfo, i);
        }
    }

    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */


/*
 * Function   :
 *      _bcm_trx_vlan_protocol_data_update
 * Description   :
 *      Helper function for an API to update lan port protocol data memory
 *      for specific ports.
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      update_pbm      (IN) BCM port bitmap to update
 *      prot_idx        (IN) Index to Port Protocol table
 *      action          (IN) VLAN action to be used to update all ports in pbmp
 */
STATIC int
_bcm_trx_vlan_protocol_data_update(int unit, bcm_pbmp_t update_pbm, int prot_idx,
                                  bcm_vlan_action_set_t *action)
{
    bcm_port_t                  p;
    vlan_protocol_data_entry_t  vde;
    int                         data_idx, profile_idx;
    int                         c_profile_idx, use_default = 0;
    bcm_vlan_action_set_t       def_action, *action_p;
    _bcm_port_info_t            *pinfo;
    

    /* if action is NULL then default action should be used for each port */
    if (NULL == action) {
        use_default = 1;
        action_p = &def_action;
    } else {
        BCM_IF_ERROR_RETURN(
            _bcm_trx_vlan_action_verify(unit, action));
        action_p = action;
    }

    BCM_PBMP_ITER(update_pbm, p) {
        BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, p, &pinfo));
        data_idx = pinfo->vlan_prot_ptr + prot_idx;

        if (use_default) {
        BCM_IF_ERROR_RETURN(
                bcm_esw_vlan_port_default_action_get(unit, p, action_p));
        }
        BCM_IF_ERROR_RETURN(
            _bcm_trx_vlan_action_profile_entry_add(unit, action_p, 
                                                   (uint32 *)&profile_idx));
        BCM_IF_ERROR_RETURN(
            READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY, data_idx, &vde));

        c_profile_idx =
            soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde,
                                                TAG_ACTION_PROFILE_PTRf);
        sal_memset(&vde, 0, sizeof(vde));
        _bcm_trx_vlan_protocol_data_entry_set(unit, &vde, action_p, 
                                              profile_idx);
        BCM_IF_ERROR_RETURN(
            WRITE_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ALL, data_idx, &vde));
        BCM_IF_ERROR_RETURN(
            _bcm_trx_vlan_action_profile_entry_delete(unit, c_profile_idx));
    }
    return BCM_E_NONE;
}

/*
 * Function   :
 *      _bcm_trx_vlan_protocol_data_entry_delete_by_idx
 * Description   :
 *      Helper function to delete a vlan_protocol_data entry from memory by 
 *      protocol index
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      port            (IN) Port
 *      vp_idx          (IN) Index to protcol id
 */
STATIC int 
_bcm_trx_vlan_protocol_data_entry_delete_by_idx(int unit, bcm_port_t port, 
                                                int vp_idx) 
{
    bcm_pbmp_t                  switched_pbm, update_pbm;
    bcm_port_config_t           pconf;
    _bcm_port_info_t            *pinfo;
    bcm_port_t                  p;
    int                         in_use = 0;
    vlan_protocol_entry_t       *vpnull;

    BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));
    BCM_IF_ERROR_RETURN(bcm_esw_port_config_get(unit, &pconf));

    BCM_PBMP_CLEAR(update_pbm);
    BCM_PBMP_PORT_ADD(update_pbm, port);
    
    switched_pbm = pconf.e; 
    if (soc_feature(unit, soc_feature_cpuport_switched)) {
        BCM_PBMP_OR(switched_pbm, pconf.cpu);
    }
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit) && soc_feature(unit, soc_feature_flex_port)) {
        BCM_IF_ERROR_RETURN(_bcm_kt2_flexio_pbmp_update(unit, &switched_pbm));
    }
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        _bcm_kt2_subport_pbmp_update(unit, &switched_pbm);
    }
#endif

    BCM_PBMP_ITER(switched_pbm, p) {
        if (p == port) {    /* skip the port we just added */
            continue;
        }
        BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, p, &pinfo));
        /* Check for explicit VID entry */
        if (!_BCM_PORT_VD_PBVL_IS_SET(pinfo, vp_idx)) {
            BCM_PBMP_PORT_ADD(update_pbm, p);
        } else {
            in_use = 1;
        }
    }

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        BCM_IF_ERROR_RETURN(
            _bcm_kt2_vlan_protocol_data_update(unit, update_pbm, vp_idx, NULL));
    } else
#endif
    {
        BCM_IF_ERROR_RETURN(
            _bcm_trx_vlan_protocol_data_update(unit, update_pbm, vp_idx, NULL));
    }
    if (!in_use) {
        vpnull = soc_mem_entry_null(unit, VLAN_PROTOCOLm); 
        WRITE_VLAN_PROTOCOLm(unit, MEM_BLOCK_ALL, vp_idx, vpnull);
    }

    return BCM_E_NONE;
}

/*
 * Function   :
 *      _bcm_trx_vlan_port_protocol_action_add
 * Description   :
 *      Helper function for an API to add vlan port protocol action
 *      For TRX 
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      port            (IN) BCM port number
 *      frame           (IN) Frame type
 *      ethr            (IN) Ethertype 
 *      action          (IN) vlan action to be used
 */
int
_bcm_trx_vlan_port_protocol_action_add(int unit,
                                      bcm_port_t port,
                                      bcm_port_frametype_t frame,
                                      bcm_port_ethertype_t ether,
                                      bcm_vlan_action_set_t *action)
{
    
    bcm_port_t                  p;
    vlan_protocol_entry_t       vpe;
    int                         vpentry, empty, rv_m, rv_e;
    bcm_pbmp_t                  switched_pbm, update_pbm, clear_pbm;
    _bcm_port_info_t            *pinfo;
    bcm_port_config_t           pconf;

    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_verify(unit, action));

    rv_m = _bcm_trx_vlan_port_prot_match_get(unit, frame, ether, &vpentry);
    rv_e = _bcm_trx_vlan_port_prot_empty_get(unit, &empty);

    if ((BCM_E_NOT_FOUND == rv_m)  && (BCM_E_FULL == rv_e)) {
        return BCM_E_FULL;
    }
    if (BCM_E_NOT_FOUND == rv_m) {
        sal_memset(&vpe, 0, sizeof(vpe));
        soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, ETHERTYPEf, ether);
        if (frame & BCM_PORT_FRAMETYPE_ETHER2) {
            soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, ETHERIIf, 1);
        }
        if (frame & BCM_PORT_FRAMETYPE_8023) {
            soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, SNAPf, 1);
        }
        if (frame & BCM_PORT_FRAMETYPE_LLC) {
            soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, LLCf, 1);
        }
        soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, MATCHUPPERf, 1);
        soc_VLAN_PROTOCOLm_field32_set(unit, &vpe, MATCHLOWERf, 1);
        /* Actual writing into the memory will be last in this routine */
        vpentry = empty;
    }

    /*
     * Set VLAN ID for target port. For all other ethernet ports,
     * make sure entries indexed by the matched entry in VLAN_PROTOCOL
     * have initialized values (either default or explicit VID).
     */
    BCM_PBMP_CLEAR(update_pbm);
    BCM_PBMP_CLEAR(clear_pbm);
    BCM_IF_ERROR_RETURN(
        bcm_esw_port_config_get(unit, &pconf));
    switched_pbm = pconf.e;
    if (soc_feature(unit, soc_feature_cpuport_switched)) {
        BCM_PBMP_OR(switched_pbm, pconf.cpu);
    }
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit) && soc_feature(unit, soc_feature_flex_port)) {
        BCM_IF_ERROR_RETURN(_bcm_kt2_flexio_pbmp_update(unit, &switched_pbm));
    }
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        _bcm_kt2_subport_pbmp_update(unit, &switched_pbm);
    }
#endif

    BCM_PBMP_ITER(switched_pbm, p) {
        BCM_IF_ERROR_RETURN(
            _bcm_port_info_get(unit, p, &pinfo));
        if (p == port) {
            if (_BCM_PORT_VD_PBVL_IS_SET(pinfo, vpentry)) {
                return BCM_E_EXISTS;
            }
            /* Set as explicit VID */
            _BCM_PORT_VD_PBVL_SET(pinfo, vpentry);
            BCM_PBMP_PORT_ADD(update_pbm, p);
        } else {
            if (_BCM_PORT_VD_PBVL_IS_SET(pinfo, vpentry)) {
                continue;
            }
            /* Set to default VLAN ID and vlan actions */
            BCM_PBMP_PORT_ADD(clear_pbm, p);
        }
    }

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_kt2_vlan_protocol_data_update(unit,
            clear_pbm, vpentry, NULL));
        BCM_IF_ERROR_RETURN(_bcm_kt2_vlan_protocol_data_update(unit,
            update_pbm, vpentry, action));
    } else
#endif
    {
        BCM_IF_ERROR_RETURN(_bcm_trx_vlan_protocol_data_update(unit,
            clear_pbm, vpentry, NULL));
        BCM_IF_ERROR_RETURN(_bcm_trx_vlan_protocol_data_update(unit,
            update_pbm, vpentry, action));
    }

    if (BCM_E_NOT_FOUND == rv_m) {
        BCM_IF_ERROR_RETURN(
            WRITE_VLAN_PROTOCOLm(unit, MEM_BLOCK_ALL, vpentry, &vpe));
}

    return BCM_E_NONE;
}

/*
 * Function   :
 *      _bcm_trx_vlan_port_protocol_action_get
 * Description   :
 *      Helper function for an API to get vlan port protocol action
 *      For TRX 
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      port            (IN) BCM port number
 *      frame           (IN) Frame type
 *      ethr            (IN) Ethertype 
 *      action          (OUT) vlan action to get
 */
int
_bcm_trx_vlan_port_protocol_action_get(int unit, bcm_port_t port, 
                                      bcm_port_frametype_t frame,
                                      bcm_port_ethertype_t ether,
                                      bcm_vlan_action_set_t *action)
{
    int                             match_prot_idx, match_data_idx, profile_idx; 
    vlan_protocol_data_entry_t      vde;
    _bcm_port_info_t                *pinfo;

    if (NULL == action) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(
        _bcm_trx_vlan_port_prot_match_get(unit, frame, ether, &match_prot_idx));

    BCM_IF_ERROR_RETURN(
        _bcm_port_info_get(unit, port, &pinfo));

    if (!_BCM_PORT_VD_PBVL_IS_SET(pinfo, match_prot_idx)) {
        return BCM_E_NOT_FOUND;
    }

    match_data_idx = pinfo->vlan_prot_ptr + match_prot_idx;
    
    BCM_IF_ERROR_RETURN(
        READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY, match_data_idx, &vde));

    _bcm_trx_vlan_protocol_data_entry_parse(unit, &vde, action, &profile_idx);
    _bcm_trx_vlan_action_profile_entry_get(unit, action, profile_idx); 

    return BCM_E_NONE;
}

/*
 * Function   :
 *      _bcm_trx_vlan_port_protocol_delete
 * Description   :
 *      Helper function for an API to delete vlan port protocol action
 *      For TRX 
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      port            (IN) BCM port number
 *      frame           (IN) Frame type
 *      ethr            (IN) Ethertype 
 */

int
_bcm_trx_vlan_port_protocol_delete(int unit, bcm_port_t port,
                                  bcm_port_frametype_t frame,
                                  bcm_port_ethertype_t ether)
{
    int                 vpentry, rv;
    _bcm_port_info_t    *pinfo;

    BCM_IF_ERROR_RETURN(
        _bcm_trx_vlan_port_prot_match_get(unit, frame, ether, &vpentry));

    BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));

    if (!_BCM_PORT_VD_PBVL_IS_SET(pinfo, vpentry)) {
        return BCM_E_NOT_FOUND;
    }

    rv = _bcm_trx_vlan_protocol_data_entry_delete_by_idx(unit, port, vpentry);
    if (BCM_SUCCESS(rv)) {
        _BCM_PORT_VD_PBVL_CLEAR(pinfo, vpentry);
    }

    return rv;
}


/*
 * Function   :
 *      _bcm_trx_vlan_port_protocol_delete_all
 * Description   :
 *      Helper function for an API to delete all actions for all 
 *      vlan port protocols for TRX 
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      port            (IN) BCM port number
 */
int
_bcm_trx_vlan_port_protocol_delete_all(unit, port)
{
    int      i, idxmin, idxmax, rv;
    _bcm_port_info_t    *pinfo;

    idxmin = soc_mem_index_min(unit, VLAN_PROTOCOLm);
    idxmax = soc_mem_index_max(unit, VLAN_PROTOCOLm);

    BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));
    
    rv = BCM_E_NONE;
    
    for (i = idxmin; i <= idxmax; i++) {
        if (_BCM_PORT_VD_PBVL_IS_SET(pinfo, i)) {
            rv = _bcm_trx_vlan_protocol_data_entry_delete_by_idx(unit, port, i);
            if (BCM_FAILURE(rv)) {
                break;
            } else {
                _BCM_PORT_VD_PBVL_CLEAR(pinfo, i);
            }
        }
    }

    return rv;
}


/*
 * Function   :
 *      _bcm_trx_vlan_port_protocol_action_traverse
 * Description   :
 *      Helper function for an API to traverse over all  
 *      vlan port protocols and call given callback routine for TRX 
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      cb              (IN) user callback function
 *      user_data       (IN) pointer to user data
 */
int 
_bcm_trx_vlan_port_protocol_action_traverse(int unit,
                                   bcm_vlan_port_protocol_action_traverse_cb cb,
                                           void *user_data)
{
    int                         idxmin, idxmax, i, data_idx, profile_idx;
    vlan_protocol_entry_t       vpe;
    vlan_protocol_data_entry_t  vde;
    bcm_port_frametype_t        ft;
    bcm_port_ethertype_t        et;
    bcm_pbmp_t                  pbmp;
    bcm_port_t                  port;
    bcm_vlan_action_set_t       action;
    bcm_port_config_t           pconf;
    _bcm_port_info_t            *pinfo;

    idxmin = soc_mem_index_min(unit, VLAN_PROTOCOLm);
    idxmax = soc_mem_index_max(unit, VLAN_PROTOCOLm);

    BCM_IF_ERROR_RETURN(bcm_esw_port_config_get(unit, &pconf));
    pbmp = pconf.e; 
    if (soc_feature(unit, soc_feature_cpuport_switched)) {
        BCM_PBMP_OR(pbmp, pconf.cpu);
    }
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit) && soc_feature(unit, soc_feature_flex_port)) {
        BCM_IF_ERROR_RETURN(_bcm_kt2_flexio_pbmp_update(unit, &pbmp));
    }
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        _bcm_kt2_subport_pbmp_update(unit, &pbmp);
    }
#endif

    for (i = idxmin; i <= idxmax; i++) {
        SOC_IF_ERROR_RETURN(
            READ_VLAN_PROTOCOLm(unit, MEM_BLOCK_ANY, i, &vpe));
        _bcm_trx_vlan_port_protocol_entry_parse(unit, &vpe, &ft, &et);
        if (0 == ft) {
            continue;
        }
        PBMP_ITER(pbmp, port) {
            BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));
            if (_BCM_PORT_VD_PBVL_IS_SET(pinfo, i)) {
                data_idx = pinfo->vlan_prot_ptr + i;
                SOC_IF_ERROR_RETURN(READ_VLAN_PROTOCOL_DATAm(unit,
                            MEM_BLOCK_ANY, data_idx, &vde));
                profile_idx = soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde,
                        TAG_ACTION_PROFILE_PTRf);
                action.new_inner_vlan =
                    soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, IVIDf);
                action.new_outer_vlan =
                    soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, OVIDf);
                if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
                    action.priority =
                        soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, OPRIf);
                    action.new_outer_cfi =
                        soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, OCFIf);
                    action.new_inner_pkt_prio =
                        soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, IPRIf);
                    action.new_inner_cfi =
                        soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, ICFIf);
                } else {
                    action.priority =
                        soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde, PRIf);
                }
                /* Read action profile data. */
                _bcm_trx_vlan_action_profile_entry_get(unit, &action, profile_idx);
                BCM_IF_ERROR_RETURN(cb(unit, port, ft, et, &action, user_data));
            }
        }
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_trx_vlan_port_default_action_set
 * Purpose:
 *      Set the port's default vlan tag actions
 * Parameters:
 *      unit       - (IN) BCM unit.
 *      port       - (IN) Port number.
 *      action     - (IN) Vlan tag actions
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_port_default_action_set(int unit, bcm_port_t port,
                                     bcm_vlan_action_set_t *action)
{
    uint32 profile_idx, old_profile_idx, vp_profile_idx;
    bcm_port_cfg_t pcfg;
    vlan_protocol_data_entry_t vde;
    int vlan_prot_entries, vlan_data_prot_start, i;
    _bcm_port_info_t *pinfo;

    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_verify(unit, action));
    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_action_profile_entry_add(unit, action, &profile_idx));

    BCM_IF_ERROR_RETURN
        (mbcm_driver[unit]->mbcm_port_cfg_get(unit, port, &pcfg));
    
    old_profile_idx = pcfg.pc_vlan_action;
    pcfg.pc_vlan = action->new_outer_vlan;
    pcfg.pc_ivlan = action->new_inner_vlan;
    pcfg.pc_vlan_action = profile_idx;
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        if (action->priority >= BCM_PRIO_MIN &&
            action->priority <= BCM_PRIO_MAX) {
            pcfg.pc_new_opri = action->priority;
        }
        pcfg.pc_new_ocfi = action->new_outer_cfi;
        pcfg.pc_new_ipri = action->new_inner_pkt_prio;
        pcfg.pc_new_icfi = action->new_inner_cfi;
    } else {
        if (action->priority >= BCM_PRIO_MIN &&
            action->priority <= BCM_PRIO_MAX) {
            pcfg.pc_new_opri = action->priority;
        }
    }

    BCM_IF_ERROR_RETURN
        (mbcm_driver[unit]->mbcm_port_cfg_set(unit, port, &pcfg));

    SOC_IF_ERROR_RETURN
        (_bcm_trx_vlan_action_profile_entry_delete(unit, old_profile_idx));

    /*
     * Update default VLAN ID in VLAN_PROTOCOL_DATA
     */
    vlan_prot_entries = soc_mem_index_count(unit, VLAN_PROTOCOLm);
    BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));
    vlan_data_prot_start = pinfo->vlan_prot_ptr;

    if (pinfo->p_vd_pbvl == 0) {
        /* Avoid one crash in kt2 tr 18 test case in linux environment */
        return (BCM_E_NONE);
    }

    
    for (i = 0; i < vlan_prot_entries; i++) {

        BCM_IF_ERROR_RETURN(
            READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY,
                                     vlan_data_prot_start + i, &vde));
        vp_profile_idx = soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde,
                             TAG_ACTION_PROFILE_PTRf);

        if (!_BCM_PORT_VD_PBVL_IS_SET(pinfo, i)) { 

            BCM_IF_ERROR_RETURN(
                _bcm_trx_vlan_action_profile_entry_add(unit, action, 
                                                       &profile_idx));
            _bcm_trx_vlan_protocol_data_entry_set(unit, &vde, action, 
                                                  profile_idx);

            BCM_IF_ERROR_RETURN(
                WRITE_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ALL,
                                           vlan_data_prot_start + i, &vde));
            BCM_IF_ERROR_RETURN(
                _bcm_trx_vlan_action_profile_entry_delete(unit, 
                                                          vp_profile_idx));
        }
    }
    
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_trx_vlan_port_default_action_get
 * Purpose:
 *      Get the port's default vlan tag actions
 * Parameters:
 *      unit       - (IN) BCM unit.
 *      port       - (IN) Port number.
 *      action     - (IN) Vlan tag actions
 * Returns:
 *      BCM_E_XXX
 */ 
int 
_bcm_trx_vlan_port_default_action_get(int unit, bcm_port_t port,
                                     bcm_vlan_action_set_t *action)
{
    uint32 profile_idx;
    bcm_port_cfg_t pcfg;

    BCM_IF_ERROR_RETURN
        (mbcm_driver[unit]->mbcm_port_cfg_get(unit, port, &pcfg));
    profile_idx = pcfg.pc_vlan_action;

    _bcm_trx_vlan_action_profile_entry_get(unit, action, profile_idx);
    
    action->new_outer_vlan = pcfg.pc_vlan;
    action->new_inner_vlan = pcfg.pc_ivlan;
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        action->priority = pcfg.pc_new_opri; 
        action->new_outer_cfi = pcfg.pc_new_ocfi;
        action->new_inner_pkt_prio = pcfg.pc_new_ipri;
        action->new_inner_cfi = pcfg.pc_new_icfi;
    } else {
        action->priority = pcfg.pc_new_opri;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_tr_vlan_port_default_action_delete
 * Purpose:
 *      Deletes the port's default vlan tag actions
 * Parameters:
 *      unit       - (IN) BCM unit.
 *      port       - (IN) Port number.
 * Returns:
 *      BCM_E_XXX
 */ 
int 
_bcm_trx_vlan_port_default_action_delete(int unit, bcm_port_t port)
{
    uint32                      old_profile_idx;
    uint32                      vp_profile_idx;
    bcm_port_cfg_t              pcfg;
    _bcm_port_info_t            *pinfo;
    int                         num_ent, start, i;
    vlan_protocol_data_entry_t  vde;

    BCM_IF_ERROR_RETURN
        (mbcm_driver[unit]->mbcm_port_cfg_get(unit, port, &pcfg));
    old_profile_idx = pcfg.pc_vlan_action;

    pcfg.pc_vlan = BCM_VLAN_DEFAULT;
    pcfg.pc_ivlan = 0;
    pcfg.pc_vlan_action = ING_ACTION_PROFILE_DEFAULT;
    BCM_IF_ERROR_RETURN
        (mbcm_driver[unit]->mbcm_port_cfg_set(unit, port, &pcfg));

    _bcm_trx_vlan_action_profile_entry_increment(unit,
            ING_ACTION_PROFILE_DEFAULT);
    BCM_IF_ERROR_RETURN
        (_bcm_trx_vlan_action_profile_entry_delete(unit, old_profile_idx));
    
    BCM_IF_ERROR_RETURN(
        _bcm_port_info_get(unit,port, &pinfo));

    num_ent = soc_mem_index_count(unit, VLAN_PROTOCOLm);
    start = pinfo->vlan_prot_ptr;
    for (i = 0; i < num_ent; i++) {
        if (!_BCM_PORT_VD_PBVL_IS_SET(pinfo, i)) { 
            BCM_IF_ERROR_RETURN(READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY,
                        start + i, &vde));
            vp_profile_idx = soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde,
                    TAG_ACTION_PROFILE_PTRf);

            sal_memset(&vde, 0, sizeof(vlan_protocol_data_entry_t));
            soc_VLAN_PROTOCOL_DATAm_field32_set(unit, &vde, OVIDf,
                    BCM_VLAN_DEFAULT); 
            soc_VLAN_PROTOCOL_DATAm_field32_set(unit, &vde,
                    TAG_ACTION_PROFILE_PTRf, ING_ACTION_PROFILE_DEFAULT);
            BCM_IF_ERROR_RETURN(WRITE_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ALL,
                        start + i, &vde));

            _bcm_trx_vlan_action_profile_entry_increment(unit,
                    ING_ACTION_PROFILE_DEFAULT);
            BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_profile_entry_delete(unit, 
                        vp_profile_idx));
        }
    }
    return (BCM_E_NONE);
}
/*
 * Function:
 *      _bcm_tr_vlan_port_egress_default_action_set
 * Purpose:
 *      Set the egress port's default vlan tag actions
 * Parameters:
 *      unit       - (IN) BCM unit.
 *      port       - (IN) Port number.
 *      action     - (IN) Vlan tag actions
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_port_egress_default_action_set(int unit, bcm_port_t port,
                                            bcm_vlan_action_set_t *action)
{
    int rv;
    uint32 rval, profile_idx, old_profile_idx;

    BCM_IF_ERROR_RETURN(_bcm_trx_egr_vlan_action_verify(unit, action));
    BCM_IF_ERROR_RETURN(
        _bcm_trx_egr_vlan_action_profile_entry_add(unit, action, &profile_idx));

    rv = READ_EGR_VLAN_CONTROL_2r(unit, port, &rval);
    if (rv < 0) {
        goto error;
    }

    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        if (-1 == action->priority) {
            /* point to default map profile(0) for default priority action */
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_2r, &rval, OPRI_CFI_SELf, 1);
        } else {
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_2r, &rval, OPRI_CFI_SELf, 0);
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_2r, &rval, OPRIf,
                          action->priority);
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_2r, &rval, OCFIf,
                          action->new_outer_cfi);
        }
    } else {
        /* -1 reserved value */
        if (-1 == action->priority) {
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_2r, &rval, ORPEf, 0);
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_2r, &rval, OPRIf, 0);
        } else if (action->priority <= BCM_PRIO_MAX){
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_2r, &rval, ORPEf, 1);
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_2r, &rval, OPRIf,
                              action->priority);
        } else {
            rv = BCM_E_PARAM;
            goto error;
        }
    }
    
    soc_reg_field_set(unit, EGR_VLAN_CONTROL_2r, &rval,
                      OVIDf, action->new_outer_vlan);
    rv = WRITE_EGR_VLAN_CONTROL_2r(unit, port, rval);
    if (rv < 0) {
        goto error;
    }

    rv = READ_EGR_VLAN_CONTROL_3r(unit, port, &rval);
    if (rv < 0) {
        goto error;
    }   
    old_profile_idx = soc_reg_field_get(unit, EGR_VLAN_CONTROL_3r,
                                        rval, TAG_ACTION_PROFILE_PTRf);
    soc_reg_field_set(unit, EGR_VLAN_CONTROL_3r, &rval,
                      TAG_ACTION_PROFILE_PTRf, profile_idx);
    soc_reg_field_set(unit, EGR_VLAN_CONTROL_3r, &rval,
                      IVIDf, action->new_inner_vlan);

    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        if ( -1 == action->priority) {
            /* point to default map profile(0) for default priority action */
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_3r, &rval, IPRI_CFI_SELf, 1);
        } else {
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_3r, &rval, IPRI_CFI_SELf, 0);
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_3r, &rval, IPRIf,
                          action->new_inner_pkt_prio);
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_3r, &rval, ICFIf,
                          action->new_inner_cfi);
        }
    } else {
        if ( -1 == action->priority) {
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_3r, &rval, IRPEf, 0);
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_3r, &rval, IPRIf, 0);
        } else if (action->priority <= BCM_PRIO_MAX){
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_3r, &rval, IRPEf, 1);
            soc_reg_field_set(unit, EGR_VLAN_CONTROL_3r, &rval, IPRIf,
                              action->priority);
        } else {
            rv = BCM_E_PARAM;
            goto error;
        }
    }
    
    rv = WRITE_EGR_VLAN_CONTROL_3r(unit, port, rval);
    if (rv < 0) {
        goto error;
    }

    /* Delete the old profile only if it is different the a new one. */
    if (profile_idx != old_profile_idx) {
        BCM_IF_ERROR_RETURN
            (_bcm_trx_egr_vlan_action_profile_entry_delete(unit, old_profile_idx));
    }

    return BCM_E_NONE;

error:
    /* Undo action profile entry addition */
    BCM_IF_ERROR_RETURN
        (_bcm_trx_egr_vlan_action_profile_entry_delete(unit, profile_idx));
    return rv;
}

/*
 * Function:
 *      _bcm_tr_vlan_port_egress_default_action_get
 * Purpose:
 *      Get the egress port's default vlan tag actions
 * Parameters:
 *      unit       - (IN) BCM unit.
 *      port       - (IN) Port number.
 *      action     - (IN) Vlan tag actions
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_port_egress_default_action_get(int unit, bcm_port_t port,
                                            bcm_vlan_action_set_t *action)
{
    uint32 profile_idx, rval;

    bcm_vlan_action_set_t_init(action);

    BCM_IF_ERROR_RETURN
        (READ_EGR_VLAN_CONTROL_3r(unit, port, &rval));
    profile_idx = soc_reg_field_get(unit, EGR_VLAN_CONTROL_3r,
                                    rval, TAG_ACTION_PROFILE_PTRf);
    _bcm_trx_egr_vlan_action_profile_entry_get(unit, action, profile_idx);
    action->new_inner_vlan = soc_reg_field_get(unit, EGR_VLAN_CONTROL_3r,
                                               rval, IVIDf);
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        if (!soc_reg_field_get(unit, EGR_VLAN_CONTROL_3r, rval,
                               IPRI_CFI_SELf)) {
            action->new_inner_pkt_prio =
                soc_reg_field_get(unit, EGR_VLAN_CONTROL_3r, rval, IPRIf);
            action->new_inner_cfi =
                soc_reg_field_get(unit, EGR_VLAN_CONTROL_3r, rval, ICFIf);
        }
    }

    BCM_IF_ERROR_RETURN
        (READ_EGR_VLAN_CONTROL_2r(unit, port, &rval));
    action->new_outer_vlan = soc_reg_field_get(unit, EGR_VLAN_CONTROL_2r,
                                               rval, OVIDf);
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        if (!soc_reg_field_get(unit, EGR_VLAN_CONTROL_2r, rval,
                               OPRI_CFI_SELf)) {
            action->priority =
                soc_reg_field_get(unit, EGR_VLAN_CONTROL_2r, rval, OPRIf);
            action->new_outer_cfi =
                soc_reg_field_get(unit, EGR_VLAN_CONTROL_2r, rval, OCFIf);
        }
    } else {
        if (soc_reg_field_get(unit, EGR_VLAN_CONTROL_2r, rval, ORPEf)) {
            action->priority = soc_reg_field_get(unit, EGR_VLAN_CONTROL_2r,
                                                 rval, OPRIf);
        } else {
            action->priority = -1;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr_vlan_port_egress_default_action_delete
 * Purpose:
 *      Deletes the egress port's default vlan tag actions
 * Parameters:
 *      unit       - (IN) BCM unit.
 *      port       - (IN) Port number.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_trx_vlan_port_egress_default_action_delete(int unit, bcm_port_t port)
{
    uint32                  profile_idx, rval;

    BCM_IF_ERROR_RETURN(
        READ_EGR_VLAN_CONTROL_3r(unit, port, &rval));
    profile_idx = soc_reg_field_get(unit, EGR_VLAN_CONTROL_3r,
                                    rval, TAG_ACTION_PROFILE_PTRf);
    
    BCM_IF_ERROR_RETURN(
        _bcm_trx_egr_vlan_action_profile_entry_delete(unit, profile_idx));
    sal_memset(&rval, 0, sizeof(uint32));
    BCM_IF_ERROR_RETURN(
        WRITE_EGR_VLAN_CONTROL_3r(unit, port, rval));
    BCM_IF_ERROR_RETURN(
        WRITE_EGR_VLAN_CONTROL_2r(unit, port, rval));


    return BCM_E_NONE;
}

/*
 * Function:
 *		_bcm_tr_vlan_translate_entry_update
 * Purpose:
 *		Update VLAN XLATE Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  vlan_xlate_entry
 *  OUT :  vlan_xlate_entry

 */

STATIC int
_bcm_tr_vlan_translate_entry_update(int unit, vlan_xlate_entry_t *vent, 
                                                                     vlan_xlate_entry_t *return_ent)
{
    uint32  vp, key_type, value;

    /* Check if Key_Type identical */
    key_type = soc_VLAN_XLATEm_field32_get (unit, vent, KEY_TYPEf);
    value = soc_VLAN_XLATEm_field32_get (unit, return_ent, KEY_TYPEf);
    if (key_type != value) {
         return BCM_E_PARAM;
    }

    soc_VLAN_XLATEm_field32_set(unit, return_ent, MPLS_ACTIONf, 0x1);
    soc_VLAN_XLATEm_field32_set(unit, return_ent, DISABLE_VLAN_CHECKSf, 1);
    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, SOURCE_VPf)) {
        vp = soc_VLAN_XLATEm_field32_get (unit, vent, SOURCE_VPf);
        soc_VLAN_XLATEm_field32_set(unit, return_ent, SOURCE_VPf, vp);
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *              _bcm_tr3_vlan_translate_entry_update
 * Purpose:
 *              Update VLAN XLATE Entry
 * Parameters:  
 *  IN :  Unit
 *  IN :  vlan_xlate_extd_entry
 *  OUT : vlan_xlate_extd_entry
 */

#if defined(BCM_TRIUMPH3_SUPPORT)
STATIC int
_bcm_tr3_vxlate_extd_entry_vp_update(int unit, 
               vlan_xlate_entry_t *vent_in,  /* update entry */
               void *ctxt,
               vlan_xlate_entry_t *vent_out, /* entry read from mem */
               vlan_xlate_extd_entry_t *vxent_out, /* entry read from mem or
                                      converted from vent_out if not null */
               int *use_extd_tbl)
{
    uint32  vp;
  
    vp = PTR_TO_INT(ctxt);
 
    if (vent_out) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vxlate2vxlate_extd(unit,vent_out,vxent_out));
    }

    soc_VLAN_XLATE_EXTDm_field32_set(unit, vxent_out,XLATE__MPLS_ACTIONf, 0x1);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, vxent_out, DISABLE_VLAN_CHECKSf, 1);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, vxent_out, SOURCE_VPf, vp);
    *use_extd_tbl = TRUE;
    return BCM_E_NONE;
}
#endif


int
_bcm_tr_vlan_translate_vp_add(int unit,
                               bcm_gport_t port,
                               bcm_vlan_translate_key_t key_type,
                               bcm_vlan_t outer_vlan,
                               bcm_vlan_t inner_vlan,
                               int vp, bcm_vlan_action_set_t *action)
{
    int rv, gport_id;
    vlan_xlate_entry_t vent;
    vlan_xlate_entry_t vent_old;
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t trunk_id;
    int idx;
    int key_type_value;
    uint32 vxlate_action_profile_idx;
    uint32 vxlate_action_profile_idx_old;
   
    rv = BCM_E_NONE; 
    sal_memset(&vent, 0, sizeof(vent));
    switch (key_type) {
        case bcmVlanTranslateKeyPortOuter:
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_OVID,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, outer_vlan);
            break;
        case bcmVlanTranslateKeyPortInner:
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_IVID,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, inner_vlan);
            break;
        case bcmVlanTranslateKeyPortDouble:
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_IVID_OVID,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, outer_vlan);
            soc_VLAN_XLATEm_field32_set(unit, &vent, IVIDf, inner_vlan);
            break;
        case bcmVlanTranslateKeyPortOuterPri:
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_PRI_CFI,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                        key_type_value);
            /* outer_vlan: Bits 12-15 contains VLAN_PRI + CFI, vlan=BCM_E_NONE */
            soc_VLAN_XLATEm_field32_set(unit, &vent, OTAGf, outer_vlan);
            break;
        case bcmVlanTranslateKeyPortOuterTag:
            /* allow 16 bit VLAN id */
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_OTAG,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, &vent, OTAGf, outer_vlan);
            break;
        default:
            return BCM_E_PARAM;
    }
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out, &trunk_id,
                                &gport_id));

    if (BCM_GPORT_IS_TRUNK(port)) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, Tf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, TGIDf, trunk_id);
    } else {
        soc_VLAN_XLATEm_field32_set(unit, &vent, MODULE_IDf, mod_out);
        soc_VLAN_XLATEm_field32_set(unit, &vent, PORT_NUMf, port_out);
    }
        
    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, SOURCE_VPf)) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_VPf, vp);
    }

    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
        /* set default vlan_xlate key source_type as SGLP */
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
    }

    soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);

    if (action) {
        vxlate_action_profile_idx = 0;
        BCM_IF_ERROR_RETURN(
            _bcm_trx_vlan_action_verify(unit, action));
        vxlate_action_profile_idx_old = 
         soc_VLAN_XLATEm_field32_get(unit, &vent, TAG_ACTION_PROFILE_PTRf);
        /* Add new tag action profile table ptr */
        BCM_IF_ERROR_RETURN
          (_bcm_trx_vlan_action_profile_entry_add(unit, action, 
                                  &vxlate_action_profile_idx));
        soc_VLAN_XLATEm_field32_set(unit, &vent, TAG_ACTION_PROFILE_PTRf,
                                              vxlate_action_profile_idx);
        /* Delete old tag action profile table ptr */
        BCM_IF_ERROR_RETURN
          (_bcm_trx_vlan_action_profile_entry_delete(unit,
                          vxlate_action_profile_idx_old));            
        /* Set new OVID and IVID from the 'action' specified */
        soc_VLAN_XLATEm_field32_set(unit, &vent, NEW_OVIDf,
                                    (action->new_outer_vlan));
        soc_VLAN_XLATEm_field32_set(unit, &vent, NEW_IVIDf,
                                    (action->new_inner_vlan));
    }

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_vxlate_entry_add(unit,&vent,
                          (void *)INT_TO_PTR(vp),
                          _bcm_tr3_vxlate_extd_entry_vp_update));
    } else
#endif
    {
    soc_VLAN_XLATEm_field32_set(unit, &vent, MPLS_ACTIONf, 0x1); /* SVP */
    soc_VLAN_XLATEm_field32_set(unit, &vent, DISABLE_VLAN_CHECKSf, 1);

    /* look up existing entry */
    soc_mem_lock(unit, VLAN_XLATEm);
    rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &idx, &vent, &vent_old, 0);
    if(rv == SOC_E_NONE) {
         BCM_IF_ERROR_RETURN(
              _bcm_tr_vlan_translate_entry_update (unit, &vent, &vent_old));
         rv = soc_mem_write(unit, VLAN_XLATEm,
                                       MEM_BLOCK_ALL, idx, &vent_old);
        soc_mem_unlock(unit, VLAN_XLATEm);
        return BCM_E_EXISTS;
    } else if (rv != SOC_E_NOT_FOUND) {
        soc_mem_unlock(unit, VLAN_XLATEm);
        return rv;
    } 

    rv = soc_mem_insert(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vent);    
    soc_mem_unlock(unit, VLAN_XLATEm);
    }
    return rv;
}

int
_bcm_tr_vlan_translate_vp_delete(int unit,
                                  bcm_gport_t port,
                                  bcm_vlan_translate_key_t key_type,
                                  bcm_vlan_t outer_vlan,
                                  bcm_vlan_t inner_vlan,
                                  int vp)
{
    int rv, gport_id;
    vlan_xlate_entry_t vent;
    bcm_module_t mod_out;
    bcm_port_t port_out;
    bcm_trunk_t trunk_id;
    int key_type_value;
 
    sal_memset(&vent, 0, sizeof(vent));
    switch (key_type) {
        case bcmVlanTranslateKeyPortOuter:
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_OVID,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, outer_vlan);
            break;
        case bcmVlanTranslateKeyPortInner:
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_IVID,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, inner_vlan);
            break;
        case bcmVlanTranslateKeyPortDouble:
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_IVID_OVID,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, &vent, OVIDf, outer_vlan);
            soc_VLAN_XLATEm_field32_set(unit, &vent, IVIDf, inner_vlan);
            break;
       case bcmVlanTranslateKeyPortOuterPri:
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_PRI_CFI,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, &vent, OTAGf, outer_vlan);
            break;
        case bcmVlanTranslateKeyPortOuterTag:
            /* allow 16 bit VLAN id */
            BCM_IF_ERROR_RETURN
                (_bcm_esw_vlan_xlate_key_type_value_get(unit,
                       VLXLT_HASH_KEY_TYPE_OTAG,
                       &key_type_value));
            soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                        key_type_value);
            soc_VLAN_XLATEm_field32_set(unit, &vent, OTAGf, outer_vlan);
            break;
        default:
            return BCM_E_PARAM;
    }
    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out, &trunk_id,
                                &gport_id));

    if (BCM_GPORT_IS_TRUNK(port)) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, Tf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, TGIDf, trunk_id);
    } else {
        soc_VLAN_XLATEm_field32_set(unit, &vent, MODULE_IDf, mod_out);
        soc_VLAN_XLATEm_field32_set(unit, &vent, PORT_NUMf, port_out);
    }

    soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);

    if (SOC_MEM_FIELD_VALID(unit, VLAN_XLATEm, SOURCE_TYPEf)) {
        /* set default vlan_xlate key source_type as SGLP */
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
    }

    soc_mem_lock(unit, VLAN_XLATEm);
    rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &vent);  
    soc_mem_unlock(unit, VLAN_XLATEm);

#if defined(BCM_TRIUMPH3_SUPPORT)
    if (SOC_IS_TRIUMPH3(unit)) {
        vlan_xlate_extd_entry_t vxent;

        if (rv == BCM_E_NOT_FOUND) { /* serarch for extend table */
            sal_memset(&vxent, 0, sizeof(vxent));
            BCM_IF_ERROR_RETURN
                (_bcm_tr3_vxlate2vxlate_extd(unit,&vent,&vxent));
            soc_mem_lock(unit, VLAN_XLATE_EXTDm);
            rv = soc_mem_delete(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ANY, &vxent);  
            soc_mem_unlock(unit, VLAN_XLATE_EXTDm);
            return rv;
        } 
    }
#endif
    return rv;
}

#ifdef BCM_TRIUMPH2_SUPPORT
int
_bcm_tr2_vlan_translate_range_vp_add(int unit,
                                     bcm_gport_t port,
                                     bcm_vlan_t outer_vlan_low,
                                     bcm_vlan_t outer_vlan_high,
                                     int vp)
{
    int i, key_type = 0, rv, gport_id, old_idx, stm_idx = 0;
    uint32 new_idx;
    source_trunk_map_table_entry_t stm_entry;
    bcm_module_t mod_out;
    bcm_port_t   port_out;
    bcm_trunk_t  trunk_id, tid;
    bcm_vlan_t vlan_low, vlan_high, min_vlan[8], max_vlan[8];
    int port_count;
    bcm_trunk_member_t *member_array = NULL;
    bcm_module_t *mod_array = NULL;
    bcm_port_t   *port_array = NULL;
    int trunk128;

    if (outer_vlan_low != BCM_VLAN_INVALID) {
        key_type = bcmVlanTranslateKeyPortOuter;
        vlan_low = outer_vlan_low;
        vlan_high = outer_vlan_high;
    } else {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out, &trunk_id,
                                &gport_id));

    if (BCM_GPORT_IS_TRUNK(port)) {
        if (BCM_TRUNK_INVALID == trunk_id) {
            return BCM_E_PORT;
        }

        BCM_IF_ERROR_RETURN
            (bcm_esw_trunk_get(unit, trunk_id, NULL, 0, NULL, &port_count));

        member_array = sal_alloc(sizeof(bcm_trunk_member_t) * port_count,
                "trunk member array");
        if (member_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(member_array, 0, sizeof(bcm_trunk_member_t) * port_count);

        rv = bcm_esw_trunk_get(unit, trunk_id, NULL, port_count,
                member_array, &port_count);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }

        mod_array = sal_alloc(sizeof(bcm_module_t) * port_count,
                "module ID array");
        if (mod_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(mod_array, 0, sizeof(bcm_module_t) * port_count);

        port_array = sal_alloc(sizeof(bcm_port_t) * port_count,
                "port ID array");
        if (port_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(port_array, 0, sizeof(bcm_port_t) * port_count);

        for (i = 0; i < port_count; i++) {
            rv = _bcm_esw_gport_resolve(unit, member_array[i].gport,
                    &mod_array[i], &port_array[i], &tid, &gport_id);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
        }
    } else {
        if ((-1 == mod_out) || (-1 == port_out)) {
            return BCM_E_PORT;
        }

        port_count = 1;

        mod_array = sal_alloc(sizeof(bcm_module_t) * port_count,
                "module ID array");
        if (mod_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        mod_array[0] = mod_out;

        port_array = sal_alloc(sizeof(bcm_port_t) * port_count,
                "port ID array");
        if (port_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        port_array[0] = port_out;
    }

    rv = _bcm_esw_src_mod_port_table_index_get(unit, mod_array[0],
            port_array[0], &stm_idx);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    soc_mem_lock(unit, ING_VLAN_RANGEm);

    rv = READ_SOURCE_TRUNK_MAP_TABLEm(unit, MEM_BLOCK_ANY, stm_idx, &stm_entry);
    if (rv < 0) {
        soc_mem_unlock(unit, ING_VLAN_RANGEm);
        goto cleanup;
    }
    if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                OUTER_VLAN_RANGE_IDXf)) {
        old_idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                                  &stm_entry, OUTER_VLAN_RANGE_IDXf);
    } else {
        old_idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                                  &stm_entry, VLAN_RANGE_IDXf);
    }

    /* Get the profile table entry for this port/trunk */
    _bcm_trx_vlan_range_profile_entry_get(unit, min_vlan, max_vlan, old_idx);

    /* Find the first unused min/max range. Unused ranges are
     * identified by { min == 1, max == 0 }
     */
    for (i = 0 ; i < 8 ; i++) {
        if ((min_vlan[i] == 1) && (max_vlan[i] == 0)) {
            break;
        } else if (min_vlan[i] == vlan_low) {
            /* Can't have multiple ranges with the same min */
            soc_mem_unlock(unit, ING_VLAN_RANGEm);
            rv = BCM_E_EXISTS;
            goto cleanup;
        }
    }
    if (i == 8) {
        /* All ranges are taken */
        soc_mem_unlock(unit, ING_VLAN_RANGEm);
        rv = BCM_E_FULL;
        goto cleanup;
    }

    /* Insert the new range into the table entry sorted by min VID */
    for ( ; i > 0 ; i--) {
        if (min_vlan[i - 1] > vlan_low) {
            /* Move existing min/max down */
            min_vlan[i] = min_vlan[i - 1];
            max_vlan[i] = max_vlan[i - 1];
        } else {
            break;
        }
    }
    min_vlan[i] = vlan_low;
    max_vlan[i] = vlan_high;

    /* Adding the new profile table entry and update profile pointer */
    for (i = 0; i < port_count; i++) {
        rv = _bcm_trx_vlan_range_profile_entry_add(unit, min_vlan, max_vlan,
                &new_idx);
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, ING_VLAN_RANGEm);
            goto cleanup;
        }
        rv = _bcm_esw_src_mod_port_table_index_get(unit, mod_array[i],
                port_array[i], &stm_idx);
        if (BCM_SUCCESS(rv)) {
            soc_field_t field;
            if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                        OUTER_VLAN_RANGE_IDXf)) {
                field = OUTER_VLAN_RANGE_IDXf;
            } else {
                field = VLAN_RANGE_IDXf;
            }
            rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm, stm_idx,
                    field, new_idx);
        }
        if (BCM_FAILURE(rv)) {
            _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
            soc_mem_unlock(unit, ING_VLAN_RANGEm);
            goto cleanup;
        }
    }

    if (BCM_GPORT_IS_TRUNK(port)) {
        if (SOC_MEM_FIELD_VALID(unit, TRUNK32_PORT_TABLEm, VLAN_RANGE_IDXf)) {
            trunk128 = soc_property_get(unit, spn_TRUNK_EXTEND, 1);
            if (!trunk128) {
                rv = _bcm_trx_vlan_range_profile_entry_add(unit, min_vlan, max_vlan,
                        &new_idx);
                if (BCM_FAILURE(rv)) {
                    soc_mem_unlock(unit, ING_VLAN_RANGEm);
                    goto cleanup;
                }
                rv = soc_mem_field32_modify(unit, TRUNK32_PORT_TABLEm, trunk_id, 
                        VLAN_RANGE_IDXf, new_idx);
                if (BCM_FAILURE(rv)) {
                    _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
                    soc_mem_unlock(unit, ING_VLAN_RANGEm);
                    goto cleanup;
                }
                _bcm_trx_vlan_range_profile_entry_delete(unit, old_idx);
            }
        }
    }

    /* Add an entry in the vlan translate table for the low VID */
    rv = _bcm_tr_vlan_translate_vp_add(unit, port, key_type, 
                                        outer_vlan_low, BCM_VLAN_INVALID,
                                        vp, NULL);
    if (rv != BCM_E_NONE) {
        for (i = 0; i < port_count; i++) {
            _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
        }
        soc_mem_unlock(unit, ING_VLAN_RANGEm);
        goto cleanup;
    }

    /* Delete the old profile entry */
    for (i = 0; i < port_count; i++) {
        _bcm_trx_vlan_range_profile_entry_delete(unit, old_idx);
    }
    soc_mem_unlock(unit, ING_VLAN_RANGEm);

cleanup:
    if (NULL != member_array) {
        sal_free(member_array);
    }
    if (NULL != mod_array) {
        sal_free(mod_array);
    }
    if (NULL != port_array) {
        sal_free(port_array);
    }

    return rv;
}

int
_bcm_tr2_vlan_translate_range_vp_delete(int unit,
                                        bcm_gport_t port,
                                        bcm_vlan_t outer_vlan_low,
                                        bcm_vlan_t outer_vlan_high,
                                        int vp)
{
    int i, key_type = 0, rv, gport_id, old_idx, stm_idx = 0;
    uint32 new_idx;
    source_trunk_map_table_entry_t stm_entry;
    bcm_module_t mod_out;
    bcm_port_t   port_out;
    bcm_trunk_t  trunk_id, tid;
    bcm_vlan_t vlan_low, vlan_high, min_vlan[8], max_vlan[8];
    int port_count;
    bcm_trunk_member_t *member_array = NULL;
    bcm_module_t *mod_array = NULL;
    bcm_port_t   *port_array = NULL;
    int trunk128;
    soc_field_t field;

    if (outer_vlan_low != BCM_VLAN_INVALID) {
        key_type = bcmVlanTranslateKeyPortOuter;
        vlan_low = outer_vlan_low;
        vlan_high = outer_vlan_high;
    } else {
        return BCM_E_PARAM;
    }

    VLAN_CHK_ID(unit, vlan_low);
    VLAN_CHK_ID(unit, vlan_high);

    BCM_IF_ERROR_RETURN
        (_bcm_esw_gport_resolve(unit, port, &mod_out, &port_out, &trunk_id,
                                &gport_id));

    if (BCM_GPORT_IS_TRUNK(port)) {
       if (BCM_TRUNK_INVALID == trunk_id) {
            return BCM_E_PORT;
        }

        BCM_IF_ERROR_RETURN
            (bcm_esw_trunk_get(unit, trunk_id, NULL, 0, NULL, &port_count));

        member_array = sal_alloc(sizeof(bcm_trunk_member_t) * port_count,
                "trunk member array");
        if (member_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(member_array, 0, sizeof(bcm_trunk_member_t) * port_count);

        rv = bcm_esw_trunk_get(unit, trunk_id, NULL, port_count,
                member_array, &port_count);
        if (BCM_FAILURE(rv)) {
            goto cleanup;
        }

        mod_array = sal_alloc(sizeof(bcm_module_t) * port_count,
                "module ID array");
        if (mod_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(mod_array, 0, sizeof(bcm_module_t) * port_count);

        port_array = sal_alloc(sizeof(bcm_port_t) * port_count,
                "port ID array");
        if (port_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        sal_memset(port_array, 0, sizeof(bcm_port_t) * port_count);

        for (i = 0; i < port_count; i++) {
            rv = _bcm_esw_gport_resolve(unit, member_array[i].gport,
                    &mod_array[i], &port_array[i], &tid, &gport_id);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
        }
    } else {
        if ((-1 == mod_out) || (-1 == port_out)) {
            return BCM_E_PORT;
        }

        port_count = 1;

        mod_array = sal_alloc(sizeof(bcm_module_t) * port_count,
                "module ID array");
        if (mod_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        mod_array[0] = mod_out;

        port_array = sal_alloc(sizeof(bcm_port_t) * port_count,
                "port ID array");
        if (port_array == NULL) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        port_array[0] = port_out;
    }

    rv = _bcm_esw_src_mod_port_table_index_get(unit, mod_array[0],
            port_array[0], &stm_idx);
    if (BCM_FAILURE(rv)) {
        goto cleanup;
    }

    soc_mem_lock(unit, ING_VLAN_RANGEm);
    rv = READ_SOURCE_TRUNK_MAP_TABLEm(unit, MEM_BLOCK_ANY, stm_idx, &stm_entry);
    if (rv < 0) {
        soc_mem_unlock(unit, ING_VLAN_RANGEm);
        goto cleanup;
    }
    if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                OUTER_VLAN_RANGE_IDXf)) {
        field = OUTER_VLAN_RANGE_IDXf;
    } else {
        field = VLAN_RANGE_IDXf;
    }

    old_idx = soc_mem_field32_get(unit, SOURCE_TRUNK_MAP_TABLEm,
                                  &stm_entry, field);

    /* Get the profile table entry for this port/trunk */
    _bcm_trx_vlan_range_profile_entry_get(unit, min_vlan, max_vlan, old_idx);

    /* Find the min/max range. */
    for (i = 0 ; i < 8 ; i++) {
        if ((min_vlan[i] == vlan_low) && (max_vlan[i] == vlan_high)) {
            break;
        }
    }
    if (i == 8) {
        soc_mem_unlock(unit, ING_VLAN_RANGEm);
        rv = BCM_E_NOT_FOUND;
        goto cleanup;
    }

    /* Remove the range from the table entry and fill in the gap */
    for ( ; i < 7 ; i++) {
        /* Move existing min/max UP */
        min_vlan[i] = min_vlan[i + 1];
        max_vlan[i] = max_vlan[i + 1];
    }
    /* Mark last min/max range as unused. Unused ranges are
     * identified by { min == 1, max == 0 }
     */
    min_vlan[i] = 1;
    max_vlan[i] = 0;

    /* Adding the new profile table entry and update profile pointer */
    for (i = 0; i < port_count; i++) {
        rv = _bcm_trx_vlan_range_profile_entry_add(unit, min_vlan, max_vlan,
                &new_idx);
        if (BCM_FAILURE(rv)) {
            soc_mem_unlock(unit, ING_VLAN_RANGEm);
            goto cleanup;
        }
        rv = _bcm_esw_src_mod_port_table_index_get(unit, mod_array[i],
                port_array[i], &stm_idx);
        if (BCM_SUCCESS(rv)) {
            if (SOC_MEM_FIELD_VALID(unit, SOURCE_TRUNK_MAP_TABLEm,
                OUTER_VLAN_RANGE_IDXf)) {
                field = OUTER_VLAN_RANGE_IDXf;
            } else {
                field = VLAN_RANGE_IDXf;
            }

            rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm, stm_idx,
                    field, new_idx);
        }
        if (BCM_FAILURE(rv)) {
            _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
            soc_mem_unlock(unit, ING_VLAN_RANGEm);
            goto cleanup;
        }
    }

    if (BCM_GPORT_IS_TRUNK(port)) {
        if (SOC_MEM_FIELD_VALID(unit, TRUNK32_PORT_TABLEm, VLAN_RANGE_IDXf)) {
            trunk128 = soc_property_get(unit, spn_TRUNK_EXTEND, 1);
            if (!trunk128) {
                rv = _bcm_trx_vlan_range_profile_entry_add(unit, min_vlan, max_vlan,
                        &new_idx);
                if (BCM_FAILURE(rv)) {
                    soc_mem_unlock(unit, ING_VLAN_RANGEm);
                    goto cleanup;
                }
                rv = soc_mem_field32_modify(unit, TRUNK32_PORT_TABLEm, trunk_id, 
                        VLAN_RANGE_IDXf, new_idx);
                if (BCM_FAILURE(rv)) {
                    _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
                    soc_mem_unlock(unit, ING_VLAN_RANGEm);
                    goto cleanup;
                }
                _bcm_trx_vlan_range_profile_entry_delete(unit, old_idx);
            }
        }
    }

    /* Delete the entry from the vlan translate table for the low VID */
    rv = _bcm_tr_vlan_translate_vp_delete(unit, port, key_type, 
                                           outer_vlan_low, BCM_VLAN_INVALID, vp);
    if (rv != BCM_E_NONE) {
        for (i = 0; i < port_count; i++) {
            _bcm_trx_vlan_range_profile_entry_delete(unit, new_idx);
        }
        soc_mem_unlock(unit, ING_VLAN_RANGEm);
        goto cleanup;
    }

    /* Delete the old profile entry */
    for (i = 0; i < port_count; i++) {
        _bcm_trx_vlan_range_profile_entry_delete(unit, old_idx);
    }
    soc_mem_unlock(unit, ING_VLAN_RANGEm);

cleanup:
    if (NULL != member_array) {
        sal_free(member_array);
    }
    if (NULL != mod_array) {
        sal_free(mod_array);
    }
    if (NULL != port_array) {
        sal_free(port_array);
    }

    return rv;
}
#endif

/*
 * Function:
 *     _bcm_trx_system_reserved_vlan_ing_set
 * Purpose:
 *     Set ingress system reserved VLAN
 * Parameters:
 *     unit  - Device unit number
 *     flags - Operation flags
 *     vlan  - VLAN
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_trx_system_reserved_vlan_ing_set(int unit, int arg)
{
    uint32 rval;
    int rv;
    int valid = (arg != BCM_VLAN_INVALID);
    int vid = valid ? arg : 0;

    rval = 0;
    soc_reg_field_set(unit, ING_SYS_RSVD_VIDr, &rval, VALIDf, valid);
    soc_reg_field_set(unit, ING_SYS_RSVD_VIDr, &rval, VIDf, vid);
    rv = WRITE_ING_SYS_RSVD_VIDr(unit, rval);

    return rv;
}

/*
 * Function:
 *     _bcm_trx_system_reserved_vlan_egr_set
 * Purpose:
 *     Set egress system reserved VLAN
 * Parameters:
 *     unit  - Device unit number
 *     flags - Operation flags
 *     vlan  - VLAN
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_trx_system_reserved_vlan_egr_set(int unit, int arg)
{
    uint32 rval;
    int rv;
    int vid = (arg != BCM_VLAN_INVALID) ? arg : 0;

    rval = 0;
    soc_reg_field_set(unit, EGR_SYS_RSVD_VIDr, &rval, VIDf, vid);
    rv = WRITE_EGR_SYS_RSVD_VIDr(unit, rval);

    return rv;
}

/*
 * Function:
 *     _bcm_trx_system_reserved_vlan_ing_get
 * Purpose:
 *     Get ingress system reserved VLAN
 * Parameters:
 *     unit  - Device unit number
 *     vlan  - VLAN
 * Returns:
 *     BCM_E_XXX
 */
int
_bcm_trx_system_reserved_vlan_ing_get(int unit, int *arg)
{
    uint32 rval;
    int rv = BCM_E_UNAVAIL;

    rv = READ_ING_SYS_RSVD_VIDr(unit, &rval);
    if (SOC_SUCCESS(rv)) {
        if (soc_reg_field_get(unit, ING_SYS_RSVD_VIDr, rval, VALIDf)) {
            bcm_vlan_t vlan;

            vlan = soc_reg_field_get(unit, ING_SYS_RSVD_VIDr, rval, VIDf);
            *arg = vlan;
        } else {
            *arg = BCM_VLAN_INVALID;
        }
    }
    return rv;
}


#ifdef BCM_KATANA2_SUPPORT
#define _BCM_VLAN_PROTOCOL_DATA_CHUNK    16
#define _BCM_VLAN_ACTION_PROFILE_DEFAULT  0
#define _BCM_VLAN_PROTOCOL_DATA_PROFILE_DEFAULT 0

STATIC
int
_bcm_kt2_port_vlan_protocol_data_profile_index_set(int unit,
    bcm_port_t port, int vlan_prot_offset, int action_profile_idx,
    bcm_vlan_action_set_t *action)
{
    int alloc_size, vlan_prot_idx, rv = BCM_E_NONE;
    vlan_protocol_data_entry_t *vlan_prot_data;
    void *entries[1];
    port_tab_entry_t pent;
    _bcm_port_info_t            *pinfo;


    if ((vlan_prot_offset < 0) ||
        (vlan_prot_offset >= _BCM_VLAN_PROTOCOL_DATA_CHUNK)) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));
    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_verify(unit, action));

    /* Allocate memory for DMA */
    alloc_size = _BCM_VLAN_PROTOCOL_DATA_CHUNK * 
                       sizeof(vlan_protocol_data_entry_t);
    vlan_prot_data = soc_cm_salloc(unit, alloc_size,
                               "VLAN protocol data table");
    if (NULL == vlan_prot_data) {
        return (BCM_E_MEMORY);
    }
    sal_memset(vlan_prot_data, 0, alloc_size);

    /* Read the old profile chunk */
    vlan_prot_idx = pinfo->vlan_prot_ptr;
    rv = soc_mem_read_range(unit, VLAN_PROTOCOL_DATAm, 
                            MEM_BLOCK_ANY, vlan_prot_idx, 
                            vlan_prot_idx + (_BCM_VLAN_PROTOCOL_DATA_CHUNK - 1),
                            vlan_prot_data);
    if (BCM_FAILURE(rv)) {
        soc_cm_sfree(unit, vlan_prot_data);
        return (rv);
    }

    /* Modify what's needed */
    /* coverity[uninit_use_in_call: FALSE] */
    _bcm_trx_vlan_protocol_data_entry_set(unit,
        &vlan_prot_data[vlan_prot_offset], action, action_profile_idx);

    /* Delete the old profile chunk */
    rv = _bcm_port_vlan_protocol_data_entry_delete(unit, vlan_prot_idx);
    if (BCM_FAILURE(rv)) {
        soc_cm_sfree(unit, vlan_prot_data);
        return (rv);
    }

    /* Add new chunk and store new HW index */
    entries[0] = vlan_prot_data;
    rv = _bcm_port_vlan_protocol_data_entry_add(unit, entries,
             _BCM_VLAN_PROTOCOL_DATA_CHUNK, (uint32 *)&vlan_prot_idx);
    /* Free the DMA memory */
    soc_cm_sfree(unit, vlan_prot_data);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    soc_mem_lock(unit, PORT_TABm);
    /* Set VLAN_PROTOCOL_DATA_INDEXf  in PORT_TAB. */
    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY, port, &pent);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, PORT_TABm);
        return rv;
    }

    soc_mem_field32_set(unit, PORT_TABm, &pent,
        VLAN_PROTOCOL_DATA_INDEXf, vlan_prot_idx / 16);

    rv = soc_mem_write(unit, PORT_TABm, MEM_BLOCK_ALL, port, &pent);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, PORT_TABm);
        return rv;
    }
    pinfo->vlan_prot_ptr = vlan_prot_idx;
    soc_mem_unlock(unit, PORT_TABm);

    return (rv);
}

STATIC
int
_bcm_kt2_port_vlan_protocol_data_profile_set(int unit,
    bcm_port_t port, void **entries)
{
    int               vlan_prot_idx, rv = BCM_E_NONE;
    _bcm_port_info_t  *pinfo;
    port_tab_entry_t  pent;

    /* Delete the old profile chunk */
    BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));
    BCM_IF_ERROR_RETURN(
        _bcm_port_vlan_protocol_data_entry_delete(unit, pinfo->vlan_prot_ptr));

    /* Add new chunk and store new HW index */
    BCM_IF_ERROR_RETURN(
        _bcm_port_vlan_protocol_data_entry_add(unit, entries,
             _BCM_VLAN_PROTOCOL_DATA_CHUNK, (uint32 *)&vlan_prot_idx));

    soc_mem_lock(unit, PORT_TABm);
    /* Set VLAN_PROTOCOL_DATA_INDEXf  in PORT_TAB. */
    rv = soc_mem_read(unit, PORT_TABm, MEM_BLOCK_ANY, port, &pent);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, PORT_TABm);
        return rv;
    }

    soc_mem_field32_set(unit, PORT_TABm, &pent,
        VLAN_PROTOCOL_DATA_INDEXf, vlan_prot_idx / 16);

    rv = soc_mem_write(unit, PORT_TABm, MEM_BLOCK_ALL, port, &pent);
    if (BCM_FAILURE(rv)) {
        soc_mem_unlock(unit, PORT_TABm);
        return rv;
    }
    /* Update vlan_prot_ptr in SW port info */
    pinfo->vlan_prot_ptr = vlan_prot_idx;

    soc_mem_unlock(unit, PORT_TABm);
    return (rv);

}


/*
 * Function   :
 *      _bcm_kt2_vlan_protocol_data_update
 * Description   :
 *      Helper function for an API to update vlan port protocol data memory
 *      for specific ports.
 * Parameters   :
 *      unit            (IN) BCM unit number
 *      update_pbm      (IN) BCM port bitmap to update
 *      prot_idx        (IN) Index to Port Protocol table
 *      action          (IN) VLAN action to be used to update all ports in pbmp
 */
int
_bcm_kt2_vlan_protocol_data_update(
    int unit, bcm_pbmp_t update_pbm, int prot_idx,
    bcm_vlan_action_set_t *action)
{
    bcm_port_t                  p;
    int                         data_idx;
    uint32                      action_profile_idx, old_action_profile_idx;
    int                         use_default = 0;
    bcm_vlan_action_set_t       def_action, *action_p;
    _bcm_port_info_t            *pinfo;
    vlan_protocol_data_entry_t  vde;

    /* if action is NULL then default action should be used for each port */
    if (NULL == action) {
        use_default = 1;
        action_p = &def_action;
    } else {
        BCM_IF_ERROR_RETURN(
            _bcm_trx_vlan_action_verify(unit, action));
        action_p = action;
    }

    BCM_PBMP_ITER(update_pbm, p) {
        BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, p, &pinfo));
        data_idx = pinfo->vlan_prot_ptr + prot_idx;

        if (use_default) {
            BCM_IF_ERROR_RETURN(
                bcm_esw_vlan_port_default_action_get(unit, p, action_p));
        }

        BCM_IF_ERROR_RETURN(
            READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY, data_idx, &vde));

        old_action_profile_idx =
            soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde,
                TAG_ACTION_PROFILE_PTRf);

        BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_profile_entry_add(unit,
            action_p, &action_profile_idx));

        /* Update  VLAN_PROTOCOL_DATA profile and
        * PORT.VLAN_PROTOCOL_DATA_INDEX */
        BCM_IF_ERROR_RETURN(
            _bcm_kt2_port_vlan_protocol_data_profile_index_set(unit,
                p, prot_idx, action_profile_idx, action_p));

        BCM_IF_ERROR_RETURN(
            _bcm_trx_vlan_action_profile_entry_delete(unit,
                old_action_profile_idx));
    }
    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_kt2_vlan_port_default_action_set
 * Purpose:
 *      Set the port's default vlan tag actions
 * Parameters:
 *      unit       - (IN) BCM unit.
 *      port       - (IN) Port number.
 *      action     - (IN) Vlan tag actions
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_kt2_vlan_port_default_action_set(int unit, bcm_port_t port,
                                     bcm_vlan_action_set_t *action)
{
    uint32 action_profile_idx, old_action_profile_idx;
    bcm_port_cfg_t pcfg;
    vlan_protocol_data_entry_t vde;
    int vlan_prot_entries, vlan_data_prot_start, i, rv, alloc_size;
    _bcm_port_info_t *pinfo;
    vlan_protocol_data_entry_t *vlan_prot_data;
    void *entries[1];

    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_verify(unit, action));
    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_profile_entry_add(unit,
        action, &action_profile_idx));

    BCM_IF_ERROR_RETURN
        (mbcm_driver[unit]->mbcm_port_cfg_get(unit, port, &pcfg));
    
    old_action_profile_idx = pcfg.pc_vlan_action;
    pcfg.pc_vlan = action->new_outer_vlan;
    pcfg.pc_ivlan = action->new_inner_vlan;
    pcfg.pc_vlan_action = action_profile_idx;
    if (soc_feature(unit, soc_feature_vlan_pri_cfi_action)) {
        if (action->priority >= BCM_PRIO_MIN &&
            action->priority <= BCM_PRIO_MAX) {
            pcfg.pc_new_opri = action->priority;
        }
        pcfg.pc_new_ocfi = action->new_outer_cfi;
        pcfg.pc_new_ipri = action->new_inner_pkt_prio;
        pcfg.pc_new_icfi = action->new_inner_cfi;
    } else {
        if (action->priority >= BCM_PRIO_MIN &&
            action->priority <= BCM_PRIO_MAX) {
            pcfg.pc_new_opri = action->priority;
        }
    }

    BCM_IF_ERROR_RETURN
        (mbcm_driver[unit]->mbcm_port_cfg_set(unit, port, &pcfg));

    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_profile_entry_delete(unit,
        old_action_profile_idx));

    BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));

    if (pinfo->p_vd_pbvl == 0) {
        /* Avoid one crash in kt2 tr 18 test case in linux environment */
        return (BCM_E_NONE);
    }

    /* Allocate memory for DMA */
    alloc_size = _BCM_VLAN_PROTOCOL_DATA_CHUNK * 
                       sizeof(vlan_protocol_data_entry_t);
    vlan_prot_data = soc_cm_salloc(unit, alloc_size,
                               "VLAN protocol data table");
    if (NULL == vlan_prot_data) {
        return (BCM_E_MEMORY);
    }
    sal_memset(vlan_prot_data, 0, alloc_size);

    /* Read the old profile chunk */
    vlan_data_prot_start = pinfo->vlan_prot_ptr;
    rv = soc_mem_read_range(unit, VLAN_PROTOCOL_DATAm, 
             MEM_BLOCK_ANY, vlan_data_prot_start, 
             vlan_data_prot_start + (_BCM_VLAN_PROTOCOL_DATA_CHUNK - 1),
             vlan_prot_data);
    if (BCM_FAILURE(rv)) {
        soc_cm_sfree(unit, vlan_prot_data);
        return (rv);
    }

    /* Update TAG_ACTION_PROFILE_PTR in VLAN_PROTOCOL_DATA */
    vlan_prot_entries = soc_mem_index_count(unit, VLAN_PROTOCOLm);
    for (i = 0; i < vlan_prot_entries; i++) {
        if (!_BCM_PORT_VD_PBVL_IS_SET(pinfo, i)) { 
            rv = READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY,
                     vlan_data_prot_start + i, &vde);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, vlan_prot_data);
                return (rv);
            }

            old_action_profile_idx = soc_VLAN_PROTOCOL_DATAm_field32_get(unit,
                                         &vde, TAG_ACTION_PROFILE_PTRf);

            rv = _bcm_trx_vlan_action_profile_entry_add(unit,
                    action, &action_profile_idx);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, vlan_prot_data);
                return (rv);
            }

            /* Modify what's needed */
            /* coverity[uninit_use_in_call: FALSE] */
            _bcm_trx_vlan_protocol_data_entry_set(unit,
                &vlan_prot_data[i], action, action_profile_idx);

            rv = _bcm_trx_vlan_action_profile_entry_delete(unit,
                     old_action_profile_idx);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, vlan_prot_data);
                return (rv);
            }
        }
    }
    entries[0] = vlan_prot_data;

    /* Update  VLAN_PROTOCOL_DATA profile and
    * PORT.VLAN_PROTOCOL_DATA_INDEX */
    rv  =_bcm_kt2_port_vlan_protocol_data_profile_set(unit, port, entries);
    soc_cm_sfree(unit, vlan_prot_data);
    return (rv);
}


/*
 * Function:
 *      _bcm_tr_vlan_port_default_action_delete
 * Purpose:
 *      Deletes the port's default vlan tag actions
 * Parameters:
 *      unit       - (IN) BCM unit.
 *      port       - (IN) Port number.
 * Returns:
 *      BCM_E_XXX
 */ 
int 
_bcm_kt2_vlan_port_default_action_delete(int unit, bcm_port_t port)
{
    uint32                      old_action_profile_idx;
    uint32                      action_profile_idx;
    bcm_port_cfg_t              pcfg;
    int                         num_ent, start, i;
    _bcm_port_info_t            *pinfo;
    vlan_protocol_data_entry_t  vde;

    BCM_IF_ERROR_RETURN
        (mbcm_driver[unit]->mbcm_port_cfg_get(unit, port, &pcfg));
    old_action_profile_idx = pcfg.pc_vlan_action;

    pcfg.pc_vlan = BCM_VLAN_DEFAULT;
    pcfg.pc_ivlan = 0;
    pcfg.pc_vlan_action = ING_ACTION_PROFILE_DEFAULT;
    BCM_IF_ERROR_RETURN
        (mbcm_driver[unit]->mbcm_port_cfg_set(unit, port, &pcfg));

    _bcm_trx_vlan_action_profile_entry_increment(unit,
         _BCM_VLAN_ACTION_PROFILE_DEFAULT);
    BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_profile_entry_delete(unit,
        old_action_profile_idx));

    num_ent = soc_mem_index_count(unit, VLAN_PROTOCOLm);
    BCM_IF_ERROR_RETURN(_bcm_port_info_get(unit, port, &pinfo));
    start = pinfo->vlan_prot_ptr;

    if (start == 0) {
        /* Decrease ref count of default profile and return */
        _bcm_port_vlan_protocol_data_entry_delete(unit, start);
        return BCM_E_NONE;
    }

    for (i = 0; i < num_ent; i++) {
        if (!_BCM_PORT_VD_PBVL_IS_SET(pinfo, i)) { 
            BCM_IF_ERROR_RETURN(READ_VLAN_PROTOCOL_DATAm(unit, MEM_BLOCK_ANY,
                        start + i, &vde));
            action_profile_idx = soc_VLAN_PROTOCOL_DATAm_field32_get(unit, &vde,
                    TAG_ACTION_PROFILE_PTRf);

            _bcm_trx_vlan_action_profile_entry_increment(unit,
                 _BCM_VLAN_ACTION_PROFILE_DEFAULT);
            BCM_IF_ERROR_RETURN(_bcm_trx_vlan_action_profile_entry_delete(unit, 
                        action_profile_idx));
        }
    }

    /* Delete the VLAN_PROTOCOL_DATA profile */
    BCM_IF_ERROR_RETURN(_bcm_port_vlan_protocol_data_entry_delete(unit, start));

    /* Increase ref count of default profile and return */
    BCM_IF_ERROR_RETURN(_bcm_port_vlan_protocol_data_entry_reference(unit,
        _BCM_VLAN_PROTOCOL_DATA_PROFILE_DEFAULT,
        _BCM_VLAN_PROTOCOL_DATA_CHUNK));
    pinfo->vlan_prot_ptr = _BCM_VLAN_PROTOCOL_DATA_PROFILE_DEFAULT;

    return (BCM_E_NONE);
}

#endif /*BCM_KATANA2_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_trx_vlan_sw_dump
 * Purpose:
 *     Displays VLAN information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_trx_vlan_sw_dump(int unit)
{
    int     num_entries;
    int     ref_count;
    int     entries_per_set;
    uint32  index;
    ing_vlan_tag_action_profile_entry_t *ing_entry;
    egr_vlan_tag_action_profile_entry_t *egr_entry;
    ing_vlan_range_entry_t              *range_entry;

    /* Display entries with reference count > 0 */

    if (ing_action_profile[unit] != NULL) {
        LOG_CLI((BSL_META_U(unit,
                            "\nSW Information Ingress VLAN Action Profile - "
                 "Unit %d\n"), unit));

        num_entries = soc_mem_index_count
            (unit, ing_action_profile[unit]->tables[0].mem);
        LOG_CLI((BSL_META_U(unit,
                            " Number of entries: %d\n\n"), num_entries));

        for (index = 0; index < num_entries; index ++) {
            if (SOC_FAILURE
                (soc_profile_mem_ref_count_get(unit,
                                               ing_action_profile[unit],
                                               index, &ref_count))) {
                break;
            }

            if (ref_count <= 0) {
                continue;
            }

            entries_per_set =
                ing_action_profile[unit]->tables[0].entries[index].entries_per_set;
            ing_entry =
                SOC_PROFILE_MEM_ENTRY(unit,
                                      ing_action_profile[unit],
                                      ing_vlan_tag_action_profile_entry_t *,
                                      index);
            LOG_CLI((BSL_META_U(unit,
                                " Index           = 0x%x\n"), index));
            LOG_CLI((BSL_META_U(unit,
                                " Reference count = %d\n"), ref_count));
            LOG_CLI((BSL_META_U(unit,
                                " Entries per set = %d\n"), entries_per_set));
            soc_mem_entry_dump(unit, ing_action_profile[unit]->tables[0].mem,
                               ing_entry);
            LOG_CLI((BSL_META_U(unit,
                                "\n\n")));
        }
    }

    if (egr_action_profile[unit] != NULL) {
        LOG_CLI((BSL_META_U(unit,
                            "\nSW Information Egress VLAN Action Profile - "
                 "Unit %d\n"), unit));

        num_entries = soc_mem_index_count
            (unit, egr_action_profile[unit]->tables[0].mem);
        LOG_CLI((BSL_META_U(unit,
                            " Number of entries: %d\n\n"), num_entries));

        for (index = 0; index < num_entries; index ++) {
            if (SOC_FAILURE
                (soc_profile_mem_ref_count_get(unit,
                                               egr_action_profile[unit],
                                               index, &ref_count))) {
                break;
            }

            if (ref_count <= 0) {
                continue;
            }

            entries_per_set =
                egr_action_profile[unit]->tables[0].entries[index].entries_per_set;
            egr_entry =
                SOC_PROFILE_MEM_ENTRY(unit,
                                      egr_action_profile[unit],
                                      egr_vlan_tag_action_profile_entry_t *,
                                      index);
            LOG_CLI((BSL_META_U(unit,
                                " Index           = 0x%x\n"), index));
            LOG_CLI((BSL_META_U(unit,
                                " Reference count = %d\n"), ref_count));
            LOG_CLI((BSL_META_U(unit,
                                " Entries per set = %d\n"), entries_per_set));
            soc_mem_entry_dump(unit, egr_action_profile[unit]->tables[0].mem,
                               egr_entry);
            LOG_CLI((BSL_META_U(unit,
                                "\n\n")));
        }
    }

    if (vlan_range_profile[unit] != NULL) {
        LOG_CLI((BSL_META_U(unit,
                            "\nSW Information VLAN Range Profile - Unit %d\n"),
                 unit));

        num_entries = soc_mem_index_count
            (unit, vlan_range_profile[unit]->tables[0].mem);
                                          
        LOG_CLI((BSL_META_U(unit,
                            " Number of entries: %d\n\n"), num_entries));

        for (index = 0; index < num_entries; index ++) {
            if (SOC_FAILURE
                (soc_profile_mem_ref_count_get(unit,
                                               vlan_range_profile[unit],
                                               index, &ref_count))) {
                break;
            }

            if (ref_count <= 0) {
                continue;
            }

            entries_per_set =
                vlan_range_profile[unit]->tables[0].entries[index].entries_per_set;
            range_entry =
                SOC_PROFILE_MEM_ENTRY(unit,
                                      vlan_range_profile[unit],
                                      ing_vlan_range_entry_t *,
                                      index);
            LOG_CLI((BSL_META_U(unit,
                                " Index           = 0x%x\n"), index));
            LOG_CLI((BSL_META_U(unit,
                                " Reference count = %d\n"), ref_count));
            LOG_CLI((BSL_META_U(unit,
                                " Entries per set = %d\n"), entries_per_set));
            soc_mem_entry_dump(unit, vlan_range_profile[unit]->tables[0].mem,
                               range_entry);
            LOG_CLI((BSL_META_U(unit,
                                "\n\n")));
        }
    }

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#else /* BCM_TRX_SUPPORT */
int _triumph_vlan_not_empty;
#endif  /* BCM_TRX_SUPPORT */

