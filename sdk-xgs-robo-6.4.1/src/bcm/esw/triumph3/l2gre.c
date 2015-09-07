/*
 * $Id: l2gre.c,v 1.42 Broadcom SDK $
 *
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
 * ESW L2GRE API
 */

#include <soc/defs.h>
#include <sal/core/libc.h>
#include <shared/bsl.h>
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/util.h>
#include <soc/hash.h>
#include <soc/debug.h>
#include <bcm/types.h>
#include <bcm/error.h>

#include <bcm/types.h>
#include <bcm/l3.h>
#include <soc/ism_hash.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/api_xlate_port.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/virtual.h>
#include <bcm_int/esw/trx.h>
#include <bcm_int/esw/triumph3.h>
#include <bcm_int/esw/l2gre.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/common/multicast.h>

_bcm_tr3_l2gre_bookkeeping_t   *_bcm_tr3_l2gre_bk_info[BCM_MAX_NUM_UNITS] = { 0 };

#define L2GRE_INFO(_unit_)   (_bcm_tr3_l2gre_bk_info[_unit_])
#define L3_INFO(_unit_)   (&_bcm_l3_bk_info[_unit_])

/*
 * EGR_IP_TUNNEL table usage bitmap operations
 */
#define _BCM_L2GRE_IP_TNL_USED_GET(_u_, _tnl_) \
        SHR_BITGET(L2GRE_INFO(_u_)->l2gre_ip_tnl_bitmap, (_tnl_))
#define _BCM_L2GRE_IP_TNL_USED_SET(_u_, _tnl_) \
        SHR_BITSET(L2GRE_INFO((_u_))->l2gre_ip_tnl_bitmap, (_tnl_))
#define _BCM_L2GRE_IP_TNL_USED_CLR(_u_, _tnl_) \
        SHR_BITCLR(L2GRE_INFO((_u_))->l2gre_ip_tnl_bitmap, (_tnl_))

#define _BCM_L2GRE_CLEANUP(_rv_) \
       if ( (_rv_) < 0) { \
           goto cleanup; \
       }

STATIC int _bcm_tr3_l2gre_bud_loopback_disable(int unit);
STATIC int _bcm_tr3_l2gre_sd_tag_set( int unit, bcm_l2gre_vpn_config_t *l2gre_vpn_info, 
                         bcm_l2gre_port_t *l2gre_port, 
                         _bcm_tr3_l2gre_nh_info_t  *egr_nh_info, 
                         egr_vlan_xlate_entry_t   *vxlate_entry,
                         int *tpid_index );
STATIC void _bcm_tr3_l2gre_sd_tag_get( int unit, bcm_l2gre_vpn_config_t *l2gre_vpn_info, 
                         bcm_l2gre_port_t *l2gre_port, 
                         egr_l3_next_hop_entry_t *egr_nh, 
                         egr_vlan_xlate_entry_t   *vxlate_entry,
                         int network_port_flag );
STATIC int _bcm_tr3_l2gre_egr_xlate_entry_set(int unit, egr_vlan_xlate_entry_t  *key_ent);
STATIC int _bcm_tr3_l2gre_egr_xlate_entry_reset(int unit, bcm_vpn_t vpn);
STATIC void  _bcm_tr3_l2gre_egr_xlate_entry_key_set(int unit, bcm_vpn_t vpn, 
                                  egr_vlan_xlate_entry_t   *tr_ent, int clean_flag);
int _bcm_tr3_l2gre_port_get(int unit, bcm_vpn_t vpn, int vp, bcm_l2gre_port_t  *l2gre_port);
void bcm_tr3_l2gre_match_clear (int unit, int vp);


/*
 * Function:
 *      _bcm_l2gre_check_init
 * Purpose:
 *      Check if L2GRE is initialized
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */


STATIC int 
_bcm_l2gre_check_init(int unit)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;

    if ((unit < 0) || (unit >= BCM_MAX_NUM_UNITS)) {
         return BCM_E_UNIT;
    }

    l2gre_info = L2GRE_INFO(unit);

    if ((l2gre_info == NULL) || (l2gre_info->initialized == FALSE)) { 
         return BCM_E_INIT;
    } else {
         return BCM_E_NONE;
    }
}

/*
 * Function:
 *      bcm_tr3_l2gre_lock
 * Purpose:
 *      Take L2GRE Lock Sempahore
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */


 int 
 bcm_tr3_l2gre_lock(int unit)
{
   int rv;

   rv = _bcm_l2gre_check_init(unit);
   
   if ( rv == BCM_E_NONE ) {
           sal_mutex_take(L2GRE_INFO((unit))->l2gre_mutex, sal_mutex_FOREVER);
   }
   return rv; 
}



/*
 * Function:
 *      bcm_tr3_l2gre_unlock
 * Purpose:
 *      Release  L2GRE Lock Semaphore
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */


void
bcm_tr3_l2gre_unlock(int unit)
{
    int rv;

   rv = _bcm_l2gre_check_init(unit);
    if ( rv == BCM_E_NONE ) {
         sal_mutex_give(L2GRE_INFO((unit))->l2gre_mutex);
    }
}


/*
 * Function:
 *      _bcm_tr3_l2gre_free_resource
 * Purpose:
 *      Free all allocated software resources 
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      Nothing
 */


STATIC void
_bcm_tr3_l2gre_free_resource(int unit)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info = L2GRE_INFO(unit);

    /* If software tables were not allocated we are done. */ 
    if (NULL == L2GRE_INFO(unit)) {
        return;
    }

    /* Destroy EGR_IP_TUNNEL usage bitmap */
    if (l2gre_info->l2gre_ip_tnl_bitmap) {
        sal_free(l2gre_info->l2gre_ip_tnl_bitmap);
        l2gre_info->l2gre_ip_tnl_bitmap = NULL;
    }

    if (l2gre_info->match_key) {
        sal_free(l2gre_info->match_key);
        l2gre_info->match_key = NULL;
    }

    if (l2gre_info->l2gre_tunnel_init) {
        sal_free(l2gre_info->l2gre_tunnel_init);
        l2gre_info->l2gre_tunnel_init = NULL;
    }

    if (l2gre_info->l2gre_tunnel_term) {
        sal_free(l2gre_info->l2gre_tunnel_term);
        l2gre_info->l2gre_tunnel_term = NULL;
    }

    /* Free module data. */
    sal_free(L2GRE_INFO(unit));
    L2GRE_INFO(unit) = NULL;
}

/*
 * Function:
 *      bcm_tr3_l2gre_allocate_bk
 * Purpose:
 *      Initialize L2GRe software book-kepping
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
bcm_tr3_l2gre_allocate_bk(int unit)
{
    /* Allocate/Init unit software tables. */
    if (NULL == L2GRE_INFO(unit)) {
        BCM_TR3_L2GRE_ALLOC(L2GRE_INFO(unit), sizeof(_bcm_tr3_l2gre_bookkeeping_t),
                          "l2gre_bk_module_data");
        if (NULL == L2GRE_INFO(unit)) {
            return (BCM_E_MEMORY);
        } else {
            L2GRE_INFO(unit)->initialized = FALSE;
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_hw_clear
 * Purpose:
 *     Perform hw tables clean up for L2GRE module. 
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */
 
STATIC int
_bcm_tr3_l2gre_hw_clear(int unit)
{
    int rv = BCM_E_NONE, rv_error = BCM_E_NONE;

    rv = bcm_tr3_l2gre_tunnel_terminator_destroy_all(unit);
    if (BCM_FAILURE(rv) && (BCM_E_NONE == rv_error)) {
        rv_error = rv;
    }

    rv = bcm_tr3_l2gre_tunnel_initiator_destroy_all(unit);
    if (BCM_FAILURE(rv) && (BCM_E_NONE == rv_error)) {
        rv_error = rv;
    }

    rv = bcm_tr3_l2gre_vpn_destroy_all(unit);
    if (BCM_FAILURE(rv) && (BCM_E_NONE == rv_error)) {
        rv_error = rv;
    }

    rv = _bcm_tr3_l2gre_bud_loopback_disable(unit);
    if (BCM_FAILURE(rv) && (BCM_E_NONE == rv_error)) {
        rv_error = rv;
    }

    return rv_error;
}

/*
 * Function:
 *      bcm_tr3_l2gre_cleanup
 * Purpose:
 *      DeInit  L2GRE software module
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr3_l2gre_cleanup(int unit)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int rv = BCM_E_UNAVAIL;

    if ((unit < 0) || (unit >= BCM_MAX_NUM_UNITS)) {
         return BCM_E_UNIT;
    }

    l2gre_info = L2GRE_INFO(unit);

    if (FALSE == l2gre_info->initialized) {
        return (BCM_E_NONE);
    } 

    rv = bcm_tr3_l2gre_lock (unit);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    if (0 == SOC_HW_ACCESS_DISABLE(unit)) { 
        rv = _bcm_tr3_l2gre_hw_clear(unit);
    }

    /* Mark the state as uninitialized */
    l2gre_info->initialized = FALSE;

    sal_mutex_give(l2gre_info->l2gre_mutex);

    /* Destroy protection mutex. */
    sal_mutex_destroy(l2gre_info->l2gre_mutex );

    /* Free software resources */
    (void) _bcm_tr3_l2gre_free_resource(unit);

    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_port_match_count_adjust
 * Purpose:
 *      Obtain ref-count for a L2GRE port
 * Returns:
 *      BCM_E_XXX
 */

void 
bcm_tr3_l2gre_port_match_count_adjust(int unit, int vp, int step)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info = L2GRE_INFO(unit);
 
    l2gre_info->match_key[vp].match_count += step;
}

#ifdef BCM_WARM_BOOT_SUPPORT

/*
 * Function:
 *      _bcm_tr3_l2gre_reinit
 * Purpose:
 *      Warm boot recovery for the L2GRE software module
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_tr3_l2gre_reinit(int unit)
{
    soc_mem_t mem;  
    mpls_entry_entry_t ment;
    int i, index_min, index_max;
    vlan_xlate_extd_entry_t vent;
    int port_type, vp, vxlate_flag;
    egr_ip_tunnel_entry_t egr_iptnl_entry;
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    source_trunk_map_table_entry_t stm_entry;
    uint32 trunk, tgid, mod_id, port_num, key_type;
    soc_field_t validf, keytypef;
    int keytype_ovid, keytype_ivid, keytype_iovid, keytype_cfi;

    l2gre_info = L2GRE_INFO(unit);

    /* Recover tunnel terminator endpoints */
    mem = EGR_IP_TUNNELm; 
    index_min = soc_mem_index_min(unit, mem);
    index_max = soc_mem_index_max(unit, mem);

    for (i = index_min; i <= index_max; i++) {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY,
                                             i, &egr_iptnl_entry));

        /* Looking for IPV4 tunnels of type L2GRE only */
        if (soc_EGR_IP_TUNNELm_field32_get(unit, &egr_iptnl_entry,
                                           ENTRY_TYPEf) != 0x1) {
            continue;
        }

        if (soc_EGR_IP_TUNNELm_field32_get(unit, &egr_iptnl_entry,
                                           TUNNEL_TYPEf) != 0x7) {
            continue;
        }
     
        l2gre_info->l2gre_tunnel_term[i].sip = 
            soc_EGR_IP_TUNNELm_field32_get(unit, &egr_iptnl_entry, SIPf);

        l2gre_info->l2gre_tunnel_term[i].dip = 
            soc_EGR_IP_TUNNELm_field32_get(unit, &egr_iptnl_entry, DIPf);
        
    }

    /* Recover l2gre match key */

    /* Recovery based on entry in VLAN_XLATE tbl */
    if (soc_mem_is_valid(unit, VLAN_XLATE_EXTDm)) {
        mem = VLAN_XLATE_EXTDm; 
        validf = VALID_0f;
        keytypef = KEY_TYPE_0f;
    } else if (soc_mem_is_valid(unit, VLAN_XLATEm)) {
        mem = VLAN_XLATEm; 
        validf = VALIDf;
        keytypef = KEY_TYPEf;
    } else {
        return BCM_E_INTERNAL;
    }

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TRIDENT2(unit)) {
        keytype_ovid = VLXLT_HASH_KEY_TYPE_OVID;
        keytype_ivid =  VLXLT_HASH_KEY_TYPE_IVID;
        keytype_iovid = VLXLT_HASH_KEY_TYPE_IVID_OVID;
        keytype_cfi = VLXLT_HASH_KEY_TYPE_PRI_CFI;
    } else
#endif
    {
        /*coverity[mixed_enums]*/
        keytype_ovid = TR3_VLXLT_X_HASH_KEY_TYPE_OVID;
        /*coverity[mixed_enums]*/
        keytype_ivid = TR3_VLXLT_X_HASH_KEY_TYPE_IVID;
        /*coverity[mixed_enums]*/
        keytype_iovid = TR3_VLXLT_X_HASH_KEY_TYPE_IVID_OVID;
        /*coverity[mixed_enums]*/
        keytype_cfi = TR3_VLXLT_X_HASH_KEY_TYPE_PRI_CFI;
    }

    index_min = soc_mem_index_min(unit, mem);
    index_max = soc_mem_index_max(unit, mem);

    for (i = index_min; i <= index_max; i++) {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY, i, &vent));
        if (soc_mem_field32_get(unit, mem, &vent, validf) == 0) {
            continue;
        }

        if (soc_mem_field32_get(unit, mem, &vent, XLATE__MPLS_ACTIONf) != 0x1) {
            continue;
        }    

        vp = soc_mem_field32_get(unit, mem, &vent, XLATE__SOURCE_VPf);

        /* Proceed only if this VP belongs to L2GRE */
        if (_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
            vxlate_flag = FALSE; 
            key_type = soc_mem_field32_get(unit, mem, &vent, keytypef);
            trunk = soc_mem_field32_get(unit, mem, &vent, XLATE__Tf);
            tgid = soc_mem_field32_get(unit, mem, &vent, XLATE__TGIDf);
            mod_id = soc_mem_field32_get(unit, mem, &vent, XLATE__MODULE_IDf);
            port_num = soc_mem_field32_get(unit, mem, &vent, XLATE__PORT_NUMf);

            if (keytype_ovid == key_type) {
                l2gre_info->match_key[vp].flags =
                                                _BCM_L2GRE_PORT_MATCH_TYPE_VLAN;
                l2gre_info->match_key[vp].match_vlan =
                    soc_mem_field32_get(unit, mem, &vent, XLATE__OVIDf);
                vxlate_flag = TRUE; /* recovery based on entry in vxlate tbl */
            } else if (keytype_ivid == key_type) {
                l2gre_info->match_key[vp].flags =
                                          _BCM_L2GRE_PORT_MATCH_TYPE_INNER_VLAN;
                l2gre_info->match_key[vp].match_inner_vlan =
                     soc_mem_field32_get(unit, mem, &vent, XLATE__IVIDf);
                vxlate_flag = TRUE;
            } else if (keytype_iovid == key_type) {
                l2gre_info->match_key[vp].flags =
                                        _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_STACKED;
                l2gre_info->match_key[vp].match_vlan =
                    soc_mem_field32_get(unit, mem, &vent, XLATE__OVIDf);
                l2gre_info->match_key[vp].match_inner_vlan =
                     soc_mem_field32_get(unit, mem, &vent, XLATE__IVIDf);
                vxlate_flag = TRUE;
            } else if (keytype_cfi == key_type) {
                l2gre_info->match_key[vp].flags =
                                            _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_PRI;
                l2gre_info->match_key[vp].match_vlan = 
                    soc_mem_field32_get(unit, mem, &vent, OTAGf);
                vxlate_flag = TRUE;
            }

            if (vxlate_flag) {
                if (trunk) {
                    l2gre_info->match_key[vp].trunk_id = tgid;
                } else {
                    l2gre_info->match_key[vp].port = port_num;
                    l2gre_info->match_key[vp].modid = mod_id;
                }
                bcm_tr3_l2gre_port_match_count_adjust(unit, vp, 1);
            }
        }
    }

    /* Recovery based on entry in MPLS tbl */
    mem = MPLS_ENTRYm; 
    index_min = soc_mem_index_min(unit, mem);
    index_max = soc_mem_index_max(unit, mem);

    for (i = index_min; i <= index_max; i++) {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY, i, &ment));
        if (soc_MPLS_ENTRYm_field32_get(unit, &ment, VALIDf) == 0) {
            continue;
        }

        /* Looking for entries of type l2gre only */
        key_type = soc_MPLS_ENTRYm_field32_get(unit, &ment, KEY_TYPEf);
        if ((key_type != 0x8) && (key_type != 0x6)) {
            continue;
        }    

        vp = soc_MPLS_ENTRYm_field32_get(unit, &ment, L2GRE_SIP__SVPf);
    }
    
    /* Recovery based on entry in SOURCE_TRUNK_MAP tbl */
    mem = SOURCE_TRUNK_MAP_TABLEm; 
    index_min = soc_mem_index_min(unit, mem);
    index_max = soc_mem_index_max(unit, mem);

    for (i = index_min; i <= index_max; i++) {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY, i, &stm_entry));

        vp = soc_SOURCE_TRUNK_MAP_TABLEm_field32_get(unit, &stm_entry, SOURCE_VPf);

        /* Proceed only if this VP belongs to L2GRE */
        if (_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {

            port_type = soc_SOURCE_TRUNK_MAP_TABLEm_field32_get(unit, &stm_entry,
                                                                     PORT_TYPEf);
            if (port_type) { /* trunk port */
                l2gre_info->match_key[vp].flags =_BCM_L2GRE_PORT_MATCH_TYPE_TRUNK;
                l2gre_info->match_key[vp].trunk_id = 
                 soc_SOURCE_TRUNK_MAP_TABLEm_field32_get(unit, &stm_entry, TGIDf);
            } else {
                l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_PORT;
                l2gre_info->match_key[vp].index = i;
            }
        }
    }

    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      bcm_tr3_l2gre_init
 * Purpose:
 *      Initialize the L2GRE software module
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */


int
bcm_tr3_l2gre_init(int unit)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int i, num_tnl=0, num_vp=0;
    int rv = BCM_E_NONE;

    if (!L3_INFO(unit)->l3_initialized) {
        LOG_ERROR(BSL_LS_BCM_L2GRE,
                  (BSL_META_U(unit,
                              "L3 module must be initialized prior to L2GRE Init\n")));
        return BCM_E_CONFIG;
    }

    /* Allocate BK Info */
    BCM_IF_ERROR_RETURN(bcm_tr3_l2gre_allocate_bk(unit));
    l2gre_info = L2GRE_INFO(unit);

    /*
     * allocate resources
     */
    if (l2gre_info->initialized) {
         BCM_IF_ERROR_RETURN(bcm_tr3_l2gre_cleanup(unit));
         BCM_IF_ERROR_RETURN(bcm_tr3_l2gre_allocate_bk(unit));
         l2gre_info = L2GRE_INFO(unit);
    }

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    l2gre_info->match_key =
        sal_alloc(sizeof(_bcm_l2gre_match_port_info_t) * num_vp, "match_key");
    if (l2gre_info->match_key == NULL) {
        _bcm_tr3_l2gre_free_resource(unit);
        return BCM_E_MEMORY;
    }

    sal_memset(l2gre_info->match_key, 0, 
            sizeof(_bcm_l2gre_match_port_info_t) * num_vp);
    /* Stay same after recover */
    for (i = 0; i < num_vp; i++) {
        bcm_tr3_l2gre_match_clear(unit, i);
    }

    /* Create EGR_IP_TUNNEL usage bitmap */
    num_tnl = soc_mem_index_count(unit, EGR_IP_TUNNELm);

    l2gre_info->l2gre_ip_tnl_bitmap =
        sal_alloc(SHR_BITALLOCSIZE(num_tnl), "l2gre_ip_tnl_bitmap");
    if (l2gre_info->l2gre_ip_tnl_bitmap == NULL) {
        _bcm_tr3_l2gre_free_resource(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(l2gre_info->l2gre_ip_tnl_bitmap, 0, SHR_BITALLOCSIZE(num_tnl));

    /* Create L2GRE protection mutex. */
    l2gre_info->l2gre_mutex = sal_mutex_create("l2gre_mutex");
    if (!l2gre_info->l2gre_mutex) {
         _bcm_tr3_l2gre_free_resource(unit);
         return BCM_E_MEMORY;
    }

    if (NULL == l2gre_info->l2gre_tunnel_term) {
        l2gre_info->l2gre_tunnel_term =
            sal_alloc(sizeof(_bcm_l2gre_tunnel_endpoint_t) * num_vp, "L2gre tunnel term store");
        if (l2gre_info->l2gre_tunnel_term == NULL) {
            _bcm_tr3_l2gre_free_resource(unit);
            return BCM_E_MEMORY;
        }
        sal_memset(l2gre_info->l2gre_tunnel_term, 0, 
                sizeof(_bcm_l2gre_tunnel_endpoint_t) * num_vp);
    }

    if (NULL == l2gre_info->l2gre_tunnel_init) {
        l2gre_info->l2gre_tunnel_init =
            sal_alloc(sizeof(_bcm_l2gre_tunnel_endpoint_t) * num_vp, "L2gre tunnel init store");
        if (l2gre_info->l2gre_tunnel_init == NULL) {
            _bcm_tr3_l2gre_free_resource(unit);
            return BCM_E_MEMORY;
        }
        sal_memset(l2gre_info->l2gre_tunnel_init, 0, 
                sizeof(_bcm_l2gre_tunnel_endpoint_t) * num_vp);
    }

#ifdef BCM_WARM_BOOT_SUPPORT

    if (SOC_WARM_BOOT(unit)) {
        rv = _bcm_tr3_l2gre_reinit(unit);
        if (BCM_FAILURE(rv)) {
            _bcm_tr3_l2gre_free_resource(unit);
        }
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    /* Mark the state as initialized */
    l2gre_info->initialized = TRUE;

    return rv;
}

 /*
  * Function:
  * 	 _bcm_tr3_l2gre_bud_loopback_enable
  * Purpose:
  * 	 Enable loopback for L2GRE BUD node multicast
  * Parameters:
  *   IN : unit
  * Returns:
  * 	 BCM_E_XXX
  */
 
STATIC int
_bcm_tr3_l2gre_bud_loopback_enable(int unit)
{
    soc_field_t lport_fields[] = {
        PORT_TYPEf,
        L2GRE_TERMINATION_ALLOWEDf,
        V4IPMC_ENABLEf,
        L2GRE_VPNID_LOOKUP_KEY_TYPEf,
        L2GRE_DEFAULT_SVP_ENABLEf
    };
    uint32 lport_values[] = {
        0x0,  /* PORT_TYPEf */
        0x1,  /* L2GRE_TERMINATION_ALLOWEDf */
        0x1,  /* V4IPMC_ENABLEf */
        0x0,  /* L2GRE_VPNID_LOOKUP_KEY_TYPEf */
        0x1   /*  L2GRE_DEFAULT_SVP_ENABLEf */
    };

    /* Update LPORT Profile Table */
    return _bcm_lport_profile_fields32_modify(unit, LPORT_PROFILE_LPORT_TAB,
                                              COUNTOF(lport_fields),
                                              lport_fields, lport_values);
}


 /*
  * Function:
  * 	 _bcm_tr3_l2gre_bud_loopback_disable
  * Purpose:
  * 	 Disable loopback for L2GRE BUD node multicast
  * Parameters:
  *   IN : unit
  * Returns:
  * 	 BCM_E_XXX
  */
 
STATIC int
_bcm_tr3_l2gre_bud_loopback_disable(int unit)
{
    soc_field_t lport_fields[] = {
        PORT_TYPEf,
        L2GRE_TERMINATION_ALLOWEDf,
        V4IPMC_ENABLEf,
        L2GRE_VPNID_LOOKUP_KEY_TYPEf,
        L2GRE_DEFAULT_SVP_ENABLEf
    };
    uint32 lport_values[] = {
        0x0,  /* PORT_TYPEf */
        0x0,  /* L2GRE_TERMINATION_ALLOWEDf */
        0x0,  /* V4IPMC_ENABLEf */
        0x0,   /* L2GRE_VPNID_LOOKUP_KEY_TYPEf */
        0x0   /*  L2GRE_DEFAULT_SVP_ENABLEf */
    };

    /* Update LPORT Profile Table */
    return _bcm_lport_profile_fields32_modify(unit, LPORT_PROFILE_LPORT_TAB,
                                              COUNTOF(lport_fields),
                                              lport_fields, lport_values);
}

 /* Function:
 *      _bcm_tr3_l2gre_vpn_is_valid
 * Purpose:
 *      Find if given L2GRE VPN is Valid 
 * Parameters:
 *      unit     - Device Number
 *      l2vpn   - L2GRE VPN
 * Returns:
 *      BCM_E_XXXX
 */
int
_bcm_tr3_l2gre_vpn_is_valid( int unit, bcm_vpn_t l2vpn)
{
    bcm_vpn_t l2gre_vpn_min=0;
    int vfi_index=-1, num_vfi=0;

    num_vfi = soc_mem_index_count(unit, VFIm);

    /* Check for Valid vpn */
     _BCM_L2GRE_VPN_SET(l2gre_vpn_min, _BCM_L2GRE_VPN_TYPE_ELINE, 0);
     if ( l2vpn < l2gre_vpn_min || l2vpn > (l2gre_vpn_min+num_vfi-1) ) {
        return BCM_E_PARAM;
     }

    _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELINE,  l2vpn);

    if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }
    return BCM_E_NONE;
}

 /* Function:
 *      _bcm_tr3_l2gre_vpn_is_eline
 * Purpose:
 *      Find if given L2GRE VPN is ELINE
 * Parameters:
 *      unit     - Device Number
 *      l2vpn   - L2GRE VPN
 *      isEline  - Flag 
 * Returns:
 *      BCM_E_XXXX
 * Assumes:
 *      l2vpn is valid
 */

STATIC int
_bcm_tr3_l2gre_vpn_is_eline( int unit, bcm_vpn_t l2vpn, uint8 *isEline)
{
    int vfi_index=-1;
    vfi_entry_t vfi_entry;

    BCM_IF_ERROR_RETURN(_bcm_tr3_l2gre_vpn_is_valid(unit, l2vpn));

    _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELINE,  l2vpn);

    BCM_IF_ERROR_RETURN
        (READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry));

    /* Set the returned VPN id */
    if (soc_VFIm_field32_get(unit, &vfi_entry, PT2PT_ENf)) {
        *isEline = 1;  /* ELINE */
    } else {
         *isEline = 0;  /* ELAN */
    }
    return BCM_E_NONE;
}

 /* Function:
 *      _bcm_tr3_l2gre_vp_is_eline
 * Purpose:
 *      Find if given L2GRE VP is ELINE
 * Parameters:
 *      unit     - Device Number
 *      vp   - L2GRE VP
 *      isEline  - Flag 
 * Returns:
 *      BCM_E_XXXX
 * Assumes:
 *      l2vpn is valid
 */

int
_bcm_tr3_l2gre_vp_is_eline( int unit, int vp, uint8 *isEline)
{
    source_vp_entry_t svp;
    vfi_entry_t vfi_entry;
    int rv = BCM_E_PARAM;
    int vfi_index;

    if (vp == -1) {
        return BCM_E_PARAM;
    }

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }

    rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
    if (rv < 0) {
        return rv;
    }

    vfi_index = soc_SOURCE_VPm_field32_get(unit, &svp, VFIf);
    BCM_IF_ERROR_RETURN
        (READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry));

    /* Set the returned VPN id */
    if (soc_VFIm_field32_get(unit, &vfi_entry, PT2PT_ENf)) {
       *isEline = 0x1;
    }  else {
       *isEline = 0x0;
   }
   return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_port_resolve
 * Purpose:
 *      Get the modid, port, trunk values for a L2GRE port
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr3_l2gre_port_resolve(int unit, bcm_gport_t l2gre_port_id, 
                          bcm_if_t encap_id, bcm_module_t *modid, bcm_port_t *port,
                          bcm_trunk_t *trunk_id, int *id)

{
    int rv = BCM_E_NONE;
    ing_l3_next_hop_entry_t ing_nh;
    egr_l3_next_hop_entry_t egr_nh;
    ing_dvp_table_entry_t dvp;
    int ecmp, nh_index, nh_ecmp_index, vp;
    uint32 hw_buf[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry. */
    int  idx, max_ent_count, base_idx;

    rv = _bcm_l2gre_check_init(unit);
    if (rv < 0) {
        return rv;
    }

    if (!BCM_GPORT_IS_L2GRE_PORT(l2gre_port_id)) {
        return (BCM_E_BADID);
    }

    vp = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port_id);
    if (vp == -1) {
       return BCM_E_PARAM;
    }

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }
    BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
    ecmp = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, ECMPf);
    if (!ecmp) {
        nh_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, 
                                              NEXT_HOP_INDEXf);
        BCM_IF_ERROR_RETURN (soc_mem_read(unit, ING_L3_NEXT_HOPm, MEM_BLOCK_ANY,
                                      nh_index, &ing_nh));

        if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, Tf)) {
            *trunk_id = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, TGIDf);
        } else {
            /* Only add this to replication set if destination is local */
            *modid = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, MODULE_IDf);
            *port = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, PORT_NUMf);
        }
    } else {
         /* Select the desired nhop from ECMP group - pointed by encap_id */
         nh_ecmp_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, ECMP_PTRf);
         BCM_IF_ERROR_RETURN(
              soc_mem_read(unit, L3_ECMP_COUNTm, MEM_BLOCK_ANY, 
                        nh_ecmp_index, hw_buf));

         if (soc_feature(unit, soc_feature_l3_ecmp_1k_groups)) {
              max_ent_count = soc_mem_field32_get(unit, L3_ECMP_COUNTm, 
                                                 hw_buf, COUNTf);
              base_idx = soc_mem_field32_get(unit, L3_ECMP_COUNTm, 
                                                 hw_buf, BASE_PTRf);
         }  else {
              max_ent_count = soc_mem_field32_get(unit, L3_ECMP_COUNTm, 
                                                 hw_buf, COUNT_0f);
              base_idx = soc_mem_field32_get(unit, L3_ECMP_COUNTm, 
                                                 hw_buf, BASE_PTR_0f);
         }
        max_ent_count++; /* Count is zero based. */ 

        if(encap_id == -1) {
              BCM_IF_ERROR_RETURN(
                   soc_mem_read(unit, L3_ECMPm, MEM_BLOCK_ANY, 
                        base_idx, hw_buf));
              nh_index = soc_mem_field32_get(unit, L3_ECMPm, 
                   hw_buf, NEXT_HOP_INDEXf);
              BCM_IF_ERROR_RETURN (soc_mem_read(unit, EGR_L3_NEXT_HOPm, 
                   MEM_BLOCK_ANY, nh_index, &egr_nh));
                 BCM_IF_ERROR_RETURN (soc_mem_read(unit, ING_L3_NEXT_HOPm, 
                        MEM_BLOCK_ANY, nh_index, &ing_nh));

                 if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, Tf)) {
                     *trunk_id = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, TGIDf);
                 } else {
                     /* Only add this to replication set if destination is local */
                     *modid = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, MODULE_IDf);
                     *port = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, PORT_NUMf);
                }
        } else {
            for (idx = 0; idx < max_ent_count; idx++) {
              BCM_IF_ERROR_RETURN(
                   soc_mem_read(unit, L3_ECMPm, MEM_BLOCK_ANY, 
                        (base_idx+idx), hw_buf));
              nh_index = soc_mem_field32_get(unit, L3_ECMPm, 
                   hw_buf, NEXT_HOP_INDEXf);
              BCM_IF_ERROR_RETURN (soc_mem_read(unit, EGR_L3_NEXT_HOPm, 
                   MEM_BLOCK_ANY, nh_index, &egr_nh));
              if (encap_id == soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, 
                                               &egr_nh, INTF_NUMf)) {
                 BCM_IF_ERROR_RETURN (soc_mem_read(unit, ING_L3_NEXT_HOPm, 
                        MEM_BLOCK_ANY, nh_index, &ing_nh));

                 if (soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, Tf)) {
                     *trunk_id = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, TGIDf);
                 } else {
                     /* Only add this to replication set if destination is local */
                     *modid = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, MODULE_IDf);
                     *port = soc_ING_L3_NEXT_HOPm_field32_get(unit, &ing_nh, PORT_NUMf);
                }
                break;
              }
            }
        }
    }
    *id = vp;
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_port_nh_cnt_dec
 * Purpose:
 *      Decrease L2GRE nexthop count.
 * Parameters:
 *      unit  - (IN) SOC unit number.
 *      vp    - (IN) Virtual port number.
 * Returns:
 *      BCM_E_XXX
 */  


STATIC int
_bcm_tr3_l2gre_port_nh_cnt_dec(int unit, int vp)
{
    int nh_ecmp_index = -1;
    ing_dvp_table_entry_t dvp;
    uint32 flags = 0;
    int ref_count = 0;
    int ecmp = -1;

    BCM_IF_ERROR_RETURN(
        READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));

    ecmp = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, ECMPf);
    if (ecmp) {
        nh_ecmp_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, ECMP_PTRf);
        flags = BCM_L3_MULTIPATH;
        /* Decrement reference count */
        BCM_IF_ERROR_RETURN(
             bcm_xgs3_get_ref_count_from_nhi(unit, flags, &ref_count, nh_ecmp_index));
    } else {
        nh_ecmp_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, 
                                              NEXT_HOP_INDEXf);
        if(nh_ecmp_index != 0) {
             /* Decrement reference count */
             BCM_IF_ERROR_RETURN(
                  bcm_xgs3_get_ref_count_from_nhi(unit, flags, &ref_count, nh_ecmp_index));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_port_cnt_update
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
_bcm_tr3_l2gre_port_cnt_update(int unit, bcm_gport_t gport,
        int vp, int incr_decr_flag)
{
    int mod_out=-1, port_out=-1, tgid_out=-1, vp_out=-1;
    bcm_port_t local_member_array[SOC_MAX_NUM_PORTS];
    int local_member_count=0;
    int idx=-1;
    int mod_local=-1;
    _bcm_port_info_t *port_info;

    BCM_IF_ERROR_RETURN(
       _bcm_tr3_l2gre_port_resolve(unit, gport, -1, &mod_out,
                    &port_out, &tgid_out, &vp_out));

    if (vp_out == -1) {
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
            if (incr_decr_flag) {
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
            if (incr_decr_flag) {
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
 *      bcm_tr3_l2gre_vpn_create
 * Purpose:
 *      Create a VPN instance
 * Parameters:
 *      unit  - (IN)  Device Number
 *      info  - (IN/OUT) VPN configuration info
 * Returns:
 *      BCM_E_XXX
 */

int 
bcm_tr3_l2gre_vpn_create(int unit, bcm_l2gre_vpn_config_t *info)
{
    int rv = BCM_E_PARAM;
    vfi_entry_t vfi_entry;
    int vfi_index, num_vfi;
    int bc_group=0, umc_group=0, uuc_group=0;
    int bc_group_type, umc_group_type, uuc_group_type;
    bcm_vpn_t l2gre_vpn_min=0;
    mpls_entry_entry_t ment;
    egr_vlan_xlate_entry_t   vxlate_entry;
    int tpid_index = -1;
    uint8 vpn_alloc_flag=0;

     num_vfi = soc_mem_index_count(unit, VFIm);

    if (info->flags & BCM_L2GRE_VPN_ELINE) {
        /* Allocate a VFI */
        if (info->flags & BCM_L2GRE_VPN_REPLACE) {
            _BCM_L2GRE_VPN_SET(l2gre_vpn_min, _BCM_L2GRE_VPN_TYPE_ELINE, 0);
            if ( info->vpn < l2gre_vpn_min || info->vpn > (l2gre_vpn_min+num_vfi-1) ) {
                return BCM_E_PARAM;
            }

            _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELINE,  info->vpn);
            if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
                return BCM_E_PARAM;
            }
            rv = READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry);
            if (BCM_FAILURE(rv)) {
                return rv;
            }
        } else if (info->flags & BCM_L2GRE_VPN_WITH_ID) {
            _BCM_L2GRE_VPN_SET(l2gre_vpn_min, _BCM_L2GRE_VPN_TYPE_ELINE, 0);
            if ( info->vpn < l2gre_vpn_min || info->vpn > (l2gre_vpn_min+num_vfi-1) ) {
                return BCM_E_PARAM;
            }

            _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELINE,  info->vpn);
            if (_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
                return BCM_E_EXISTS;
            } else {
                rv = _bcm_vfi_alloc_with_id(unit, VFIm, _bcmVfiTypeL2Gre, vfi_index);
                if (rv < 0) {
                    return rv;
                }
                vpn_alloc_flag = 1;
            }
            sal_memset(&vfi_entry, 0, sizeof(vfi_entry_t));
        } else {
            rv = _bcm_vfi_alloc(unit, VFIm, _bcmVfiTypeL2Gre, &vfi_index);
            if (rv < 0) {
                return rv;
            }
            vpn_alloc_flag = 1;
            sal_memset(&vfi_entry, 0, sizeof(vfi_entry_t));
        }
        soc_VFIm_field32_set(unit, &vfi_entry, PT2PT_ENf, 0x1);
        rv = WRITE_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry);
        _BCM_L2GRE_CLEANUP(rv);
        _BCM_L2GRE_VPN_SET(info->vpn, _BCM_L2GRE_VPN_TYPE_ELINE, vfi_index);
    } else if (info->flags & BCM_L2GRE_VPN_ELAN) {
        /* Check that the groups are valid. */
        bc_group_type = _BCM_MULTICAST_TYPE_GET(info->broadcast_group);
        bc_group = _BCM_MULTICAST_ID_GET(info->broadcast_group);
        umc_group_type = _BCM_MULTICAST_TYPE_GET(info->unknown_multicast_group);
        umc_group = _BCM_MULTICAST_ID_GET(info->unknown_multicast_group);
        uuc_group_type = _BCM_MULTICAST_TYPE_GET(info->unknown_unicast_group);
        uuc_group = _BCM_MULTICAST_ID_GET(info->unknown_unicast_group);

        if ((bc_group_type != _BCM_MULTICAST_TYPE_L2GRE) ||
            (umc_group_type != _BCM_MULTICAST_TYPE_L2GRE) ||
            (uuc_group_type != _BCM_MULTICAST_TYPE_L2GRE) ||
            (bc_group >= soc_mem_index_count(unit, L3_IPMCm)) ||
            (umc_group >= soc_mem_index_count(unit, L3_IPMCm)) ||
            (uuc_group >= soc_mem_index_count(unit, L3_IPMCm))) {
            return BCM_E_PARAM;
        }

        if (info->flags & BCM_L2GRE_VPN_REPLACE) {
            _BCM_L2GRE_VPN_SET(l2gre_vpn_min, _BCM_L2GRE_VPN_TYPE_ELAN, 0);
            if ( info->vpn < l2gre_vpn_min || info->vpn > (l2gre_vpn_min+num_vfi-1) ) {
                return BCM_E_PARAM;
            }

            _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELAN,  info->vpn);
            if (_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
                return BCM_E_PARAM;
            }
            rv = READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry);
            if (BCM_FAILURE(rv)) {
                return rv;
            }
        } else if (info->flags & BCM_L2GRE_VPN_WITH_ID) {
            _BCM_L2GRE_VPN_SET(l2gre_vpn_min, _BCM_L2GRE_VPN_TYPE_ELAN, 0);
            if ( info->vpn < l2gre_vpn_min || info->vpn > (l2gre_vpn_min+num_vfi-1) ) {
                return BCM_E_PARAM;
            }

            _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELAN,  info->vpn);
            if (_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
                return BCM_E_EXISTS;
            } else {
                rv = _bcm_vfi_alloc_with_id(unit, VFIm, _bcmVfiTypeL2Gre, vfi_index);
                if (rv < 0) {
                    return rv;
                }
                vpn_alloc_flag = 1;
            }
            sal_memset(&vfi_entry, 0, sizeof(vfi_entry_t));
        } else {
            rv = _bcm_vfi_alloc(unit, VFIm, _bcmVfiTypeL2Gre, &vfi_index);
            if (rv < 0) {
                return rv;
            }
            vpn_alloc_flag = 1;
            sal_memset(&vfi_entry, 0, sizeof(vfi_entry_t));
        }
        /* Commit the entry to HW */
        soc_VFIm_field32_set(unit, &vfi_entry, UMC_INDEXf, umc_group);
        soc_VFIm_field32_set(unit, &vfi_entry, UUC_INDEXf, uuc_group);
        soc_VFIm_field32_set(unit, &vfi_entry, BC_INDEXf, bc_group);
        rv = WRITE_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry);
        _BCM_L2GRE_CLEANUP(rv);
        _BCM_L2GRE_VPN_SET(info->vpn, _BCM_L2GRE_VPN_TYPE_ELAN, vfi_index);
    }

    sal_memset(&vxlate_entry, 0, sizeof(egr_vlan_xlate_entry_t));
    if (info->flags & BCM_L2GRE_VPN_WITH_VPNID) {
        if (info->vpnid == 0) {
           rv = BCM_E_PARAM;
           goto cleanup;
        }
        /* Configure Ingress VPNID */
        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));

        soc_MPLS_ENTRYm_field32_set(unit, &ment, L2GRE_VPNID__VPNIDf, 
                                    info->vpnid);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, L2GRE_VPNID__VFIf, vfi_index);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, KEY_TYPEf, _BCM_L2GRE_KEY_TYPE_VPNID_VFI);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 1);

        rv = bcm_tr3_l2gre_match_vpnid_entry_set(unit, &ment);
        _BCM_L2GRE_CLEANUP(rv);

        /* Configure Egress VPNID */
        if (SOC_IS_TRIUMPH3(unit)) {
            soc_EGR_VLAN_XLATEm_field32_set(unit, &vxlate_entry, KEY_TYPEf, _BCM_TR3_L2GRE_KEY_TYPE_LOOKUP_VFI);
        } else  if (SOC_IS_TRIDENT2(unit)) {
            soc_EGR_VLAN_XLATEm_field32_set(unit, &vxlate_entry, ENTRY_TYPEf, _BCM_TD2_L2GRE_KEY_TYPE_LOOKUP_VFI);
        }
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VALIDf, 0x1);
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vxlate_entry, L2GRE_VFI__VFIf, vfi_index);
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                L2GRE_VFI__VPNIDf,  info->vpnid);
    } else {
           rv = BCM_E_PARAM;
           goto cleanup;
    }

    if (info->flags & BCM_L2GRE_VPN_SERVICE_TAGGED) {
        /* Configure EGR_VLAN_XLATE - SD_TAG setting */
        rv = _bcm_tr3_l2gre_sd_tag_set(unit, info, NULL, NULL, &vxlate_entry, &tpid_index);
        _BCM_L2GRE_CLEANUP(rv);
    }

    if ((info->flags & BCM_L2GRE_VPN_WITH_VPNID) || 
        (info->flags & BCM_L2GRE_VPN_SERVICE_TAGGED)) {
        rv = _bcm_tr3_l2gre_egr_xlate_entry_set(unit, &vxlate_entry);
         _BCM_L2GRE_CLEANUP(rv);
    } 
    return rv;

cleanup:
    if ( vpn_alloc_flag ) {
        (void) _bcm_vfi_free(unit, _bcmVfiTypeVxlan, vfi_index);
    }
    if (tpid_index != -1) {
        (void) _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
    }
    return rv;

}

 /* Function:
 *      bcm_tr3_l2gre_vpn_vpnid_get
 * Purpose:
 *      Get VPNID per VPN instance
 * Parameters:
 *      unit     - Device Number
 *      l2vpn   - L2GRE VPN
 *      info     - L2GRE VPN Config
 * Returns:
 *      BCM_E_XXXX
 */

STATIC int 
_bcm_tr3_l2gre_vpn_vpnid_get( int unit, int vfi, uint32 *vnid)
{
   /* Indexes to iterate over memories, chunks and entries */
    int             chnk_idx=0, ent_idx=0, chnk_idx_max=0, mem_idx_max=0;
    int             buf_size=0, chunksize=0, chnk_end=0;
    uint32          *l2gre_vnid_chunk;
    mpls_entry_entry_t *ment;
    int             rv = BCM_E_NONE;
    soc_mem_t     mem;
    int    key_type=0;

    mem = MPLS_ENTRYm;

    if (!soc_mem_index_count(unit, mem)) {
        return BCM_E_NONE;
    }

    chunksize = 4096;

    buf_size = 4 * SOC_MAX_MEM_FIELD_WORDS * chunksize;
    l2gre_vnid_chunk = soc_cm_salloc(unit, buf_size, "L2Gre vpn vpnid get");
    if (NULL == l2gre_vnid_chunk) {
        return BCM_E_MEMORY;
    }

    mem_idx_max = soc_mem_index_max(unit, mem);
    for (chnk_idx = soc_mem_index_min(unit, mem); 
         chnk_idx <= mem_idx_max; chnk_idx += chunksize) {
        sal_memset((void *)l2gre_vnid_chunk, 0, buf_size);

        chnk_idx_max = 
            ((chnk_idx + chunksize) < mem_idx_max) ? 
            (chnk_idx + chunksize - 1) : mem_idx_max;

        rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ANY,
                                chnk_idx, chnk_idx_max, l2gre_vnid_chunk);
        if (SOC_FAILURE(rv)) {
            break;
        }
        chnk_end = (chnk_idx_max - chnk_idx);
        for (ent_idx = 0 ; ent_idx <= chnk_end; ent_idx ++) {
            ment = soc_mem_table_idx_to_pointer(unit, mem, mpls_entry_entry_t *, 
                                             l2gre_vnid_chunk, ent_idx);

            if (!soc_mem_field32_get(unit, mem, ment, VALIDf)) {
                continue;
            }
          
            /* Match the Key Type to L2GRE */
            key_type = soc_mem_field32_get(unit, mem, ment, KEY_TYPEf);

            if (key_type != _BCM_L2GRE_KEY_TYPE_VPNID_VFI) { 
                continue;
            }

            /* Check for Valid VFI */
            if (vfi != soc_mem_field32_get(unit, mem, ment, L2GRE_VPNID__VFIf) ) {
                continue;
            }

            *vnid = soc_mem_field32_get(unit, mem, ment, L2GRE_VPNID__VPNIDf);
        }
    }
    soc_cm_sfree(unit, l2gre_vnid_chunk);
    return rv;
}


/*
 * Function:
 *      bcm_tr3_l2gre_vpn_destroy
 * Purpose:
 *      Delete a VPN instance
 * Parameters:
 *      unit - Device Number
 *      vpn - VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr3_l2gre_vpn_destroy(int unit, bcm_vpn_t l2vpn)
{
    int vfi = 0, num_vfi;
    vfi_entry_t vfi_entry;
    int rv;
    bcm_vpn_t l2gre_vpn_min=0;
    uint8 isEline;
    mpls_entry_entry_t ment;
    uint32 vpnid = 0;

    /*
     * Valid ELINE VPN?
     */
    _BCM_L2GRE_VPN_SET(l2gre_vpn_min, _BCM_L2GRE_VPN_TYPE_ELINE, 0);
    isEline = 1;
    num_vfi = soc_mem_index_count(unit, VFIm);
    if (l2vpn < l2gre_vpn_min || l2vpn > (l2gre_vpn_min + num_vfi - 1)) {
        /*
         * Valid ELAN VPN?
         * Redundant defensive check
         * _BCM_L2GRE_VPN_TYPE_ELINE == _BCM_L2GRE_VPN_TYPE_ELAN
         */
        _BCM_L2GRE_VPN_SET(l2gre_vpn_min, _BCM_L2GRE_VPN_TYPE_ELAN, 0);
        isEline = 0;
        if (l2vpn < l2gre_vpn_min || l2vpn > (l2gre_vpn_min + num_vfi - 1)) {
            return BCM_E_PARAM;
        }
    }

    BCM_IF_ERROR_RETURN 
         (_bcm_tr3_l2gre_vpn_is_eline(unit, l2vpn, &isEline));

    if (isEline == 0x1 ) {
        _BCM_L2GRE_VPN_GET(vfi, _BCM_L2GRE_VPN_TYPE_ELINE, l2vpn);
    } else if (isEline == 0x0 ) {
        _BCM_L2GRE_VPN_GET(vfi, _BCM_L2GRE_VPN_TYPE_ELAN, l2vpn);
    }

    if (!_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }

    rv = _bcm_tr3_l2gre_vpn_vpnid_get(unit, vfi,  &vpnid);
    BCM_IF_ERROR_RETURN(rv);

    sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
    soc_MPLS_ENTRYm_field32_set(unit, &ment, L2GRE_VPNID__VPNIDf, vpnid);
    soc_MPLS_ENTRYm_field32_set(unit, &ment, KEY_TYPEf, _BCM_L2GRE_KEY_TYPE_VPNID_VFI);
    soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 1);

    rv = bcm_tr3_l2gre_match_vpnid_entry_reset(unit, &ment);
    BCM_IF_ERROR_RETURN(rv);

    /* Delete egr_vxlate entry */
    rv = _bcm_tr3_l2gre_egr_xlate_entry_reset(unit, l2vpn);
    BCM_IF_ERROR_RETURN(rv);

    /* Delete all L2GRE ports on this VPN */
    rv = bcm_tr3_l2gre_port_delete_all(unit, l2vpn);
    BCM_IF_ERROR_RETURN(rv);

    sal_memset(&vfi_entry, 0, sizeof(vfi_entry_t));
    BCM_IF_ERROR_RETURN(WRITE_VFIm(unit, MEM_BLOCK_ALL, vfi, &vfi_entry));
    (void) _bcm_vfi_free(unit, _bcmVfiTypeL2Gre, vfi);
    return BCM_E_NONE;
}

 /* Function:
 *      bcm_tr3_l2gre_vpn_id_destroy_all
 * Purpose:
 *      Delete all L2-VPN instances
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXXX
 */

int 
bcm_tr3_l2gre_vpn_destroy_all(int unit)
{
    int num_vfi, idx, rv = BCM_E_NONE;
    bcm_vpn_t l2vpn;
    vfi_entry_t vfi_entry;

    /* Destroy all L2GRE VPNs */
    num_vfi = soc_mem_index_count(unit, VFIm);
    for (idx = 0; idx < num_vfi; idx++) {
         if (_bcm_vfi_used_get(unit, idx, _bcmVfiTypeL2Gre)) {
              BCM_IF_ERROR_RETURN
                   (READ_VFIm(unit, MEM_BLOCK_ALL, idx, &vfi_entry));
              if (soc_VFIm_field32_get(unit, &vfi_entry, PT2PT_ENf)) {
                   _BCM_L2GRE_VPN_SET(l2vpn, _BCM_L2GRE_VPN_TYPE_ELINE, idx);
              } else {
                   _BCM_L2GRE_VPN_SET(l2vpn, _BCM_L2GRE_VPN_TYPE_ELAN, idx);
              }
              rv = bcm_tr3_l2gre_vpn_destroy(unit, l2vpn);
              if (rv < 0) {
                return rv;
              }
         }
    }
    return rv;
}


 /* Function:
 *      bcm_tr3_l2gre_vpn_get
 * Purpose:
 *      Get L2-VPN instance
 * Parameters:
 *      unit     - Device Number
 *      l2vpn   - L2GRE VPN
 *      info     - L2GRE VPN Config
 * Returns:
 *      BCM_E_XXXX
 */

int 
bcm_tr3_l2gre_vpn_get( int unit, bcm_vpn_t l2vpn, bcm_l2gre_vpn_config_t *info)
{
    int vfi_index = 0, num_vfi;
    vfi_entry_t vfi_entry;
    bcm_vpn_t l2gre_vpn_min=0;
    uint8 isEline;
    int rv = BCM_E_NONE;
    egr_vlan_xlate_entry_t   vxlate_entry;
    int vxlate_index=-1;

    /*
     * Valid ELINE VPN?
     */
    _BCM_L2GRE_VPN_SET(l2gre_vpn_min, _BCM_L2GRE_VPN_TYPE_ELINE, 0);
    isEline = 1;
    num_vfi = soc_mem_index_count(unit, VFIm);
    if (l2vpn < l2gre_vpn_min || l2vpn > (l2gre_vpn_min + num_vfi - 1)) {
        /*
         * Valid ELAN VPN?
         * Redundant defensive check
         * _BCM_L2GRE_VPN_TYPE_ELINE == _BCM_L2GRE_VPN_TYPE_ELAN
         */
        _BCM_L2GRE_VPN_SET(l2gre_vpn_min, _BCM_L2GRE_VPN_TYPE_ELAN, 0);
        isEline = 0;
        if (l2vpn < l2gre_vpn_min || l2vpn > (l2gre_vpn_min + num_vfi - 1)) {
            return BCM_E_PARAM;
        }
    }

    BCM_IF_ERROR_RETURN 
             (_bcm_tr3_l2gre_vpn_is_eline(unit, l2vpn, &isEline));

    if (isEline == 0x1 ) {
        _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELINE, l2vpn);
    } else if (isEline == 0x0 ) {
        _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELAN, l2vpn);
    }

    if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }

    BCM_IF_ERROR_RETURN
        (READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry));

    /* Set the returned VPN id */
    if (soc_VFIm_field32_get(unit, &vfi_entry, PT2PT_ENf)) {
        info->flags =  BCM_L2GRE_VPN_ELINE;
        _BCM_L2GRE_VPN_SET(info->vpn, _BCM_L2GRE_VPN_TYPE_ELINE, vfi_index);
    } else {
         _BCM_MULTICAST_GROUP_SET(info->broadcast_group,
                             _BCM_MULTICAST_TYPE_L2GRE,
              soc_VFIm_field32_get(unit, &vfi_entry, BC_INDEXf));
         _BCM_MULTICAST_GROUP_SET(info->unknown_unicast_group, 
                             _BCM_MULTICAST_TYPE_L2GRE,  
              soc_VFIm_field32_get(unit, &vfi_entry, UUC_INDEXf));
         _BCM_MULTICAST_GROUP_SET(info->unknown_multicast_group, 
                             _BCM_MULTICAST_TYPE_L2GRE,  
              soc_VFIm_field32_get(unit, &vfi_entry, UMC_INDEXf));

         info->flags =  BCM_L2GRE_VPN_ELAN;
         _BCM_L2GRE_VPN_SET(info->vpn, _BCM_L2GRE_VPN_TYPE_ELAN, vfi_index);
    }
    /* Get VPNID */
    info->vpnid = 0;
    rv = _bcm_tr3_l2gre_vpn_vpnid_get(unit, vfi_index,  &info->vpnid);
    if (info->vpnid != 0) {
        info->flags |= BCM_L2GRE_VPN_WITH_VPNID;
    }
    /* Get SDTAG settings */
    (void) _bcm_tr3_l2gre_egr_xlate_entry_key_set(unit,
                      l2vpn, &vxlate_entry, 1);

    rv = soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ANY, &vxlate_index,
                    &vxlate_entry, &vxlate_entry, 0);
    if (BCM_SUCCESS(rv)) {
          (void)  _bcm_tr3_l2gre_sd_tag_get( unit, info, NULL, NULL,
                            &vxlate_entry, 1);
    }

    return rv;}

 /* Function:
 *      _bcm_tr3_l2gre_vpn_get
 * Purpose:
 *      Get L2-VPN instance for L2GRE
 * Parameters:
 *      unit     - Device Number
 *      vfi_index   - vfi_index 
 *      vid     (OUT) VLAN Id (l2vpn id)
 * Returns:
 *      BCM_E_XXXX
 */
int 
_bcm_tr3_l2gre_vpn_get(int unit, int vfi_index, bcm_vlan_t *vid)
{
    vfi_entry_t vfi_entry;

    BCM_IF_ERROR_RETURN
        (READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry));

    if (soc_VFIm_field32_get(unit, &vfi_entry, PT2PT_ENf)) {
        _BCM_L2GRE_VPN_SET(*vid, _BCM_L2GRE_VPN_TYPE_ELINE, vfi_index);
    } else {
        _BCM_L2GRE_VPN_SET(*vid, _BCM_L2GRE_VPN_TYPE_ELAN, vfi_index);
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_l2gre_vpn_traverse
 * Purpose:
 *      Get information about a VPN instance
 * Parameters:
 *      unit  - (IN)  Device Number
 *      cb    - (IN)  User-provided callback
 *      info  - (IN/OUT) Cookie
 * Returns:
 *      BCM_E_XXX
 */

int bcm_tr3_l2gre_vpn_traverse(int unit, bcm_l2gre_vpn_traverse_cb cb, 
                             void *user_data)
{
    int idx, num_vfi, rv = BCM_E_NONE;
    int vpn;
    bcm_l2gre_vpn_config_t info;

    /* L2GRE VPNs */
    num_vfi = soc_mem_index_count(unit, VFIm);
    for (idx = 0; idx < num_vfi; idx++) {
         if (_bcm_vfi_used_get(unit, idx, _bcmVfiTypeL2Gre)) {
              _BCM_L2GRE_VPN_SET(vpn, _BCM_L2GRE_VPN_TYPE_ELAN, idx);
              bcm_l2gre_vpn_config_t_init(&info);
              rv = bcm_tr3_l2gre_vpn_get(unit, vpn, &info);
              if (BCM_FAILURE(rv)) {
                   return rv;
              }
              if (info.flags & BCM_L2GRE_VPN_WITH_VPNID) {
                   rv = cb(unit, &info, user_data);
                   if (BCM_FAILURE(rv)) {
                        return rv;
                   } 
              } else {
                  return BCM_E_PARAM;
              }
         }
    }
    return rv;
}

/*
 * Function:
 *		_bcm_tr3_l2gre_tunnel_terminate_entry_key_set
 * Purpose:
 *		Set L2GRE Tunnel Terminate entry key set
 * Parameters:
 *  IN :  Unit
 *  IN :  tnl_info
 *  IN(OUT) : tr_ent VLAN_XLATE entry pointer
 *  IN : clean_flag
 * Returns:
 *	BCM_E_XXX
 */

STATIC void
 _bcm_tr3_l2gre_tunnel_terminate_entry_key_set(int unit, bcm_tunnel_terminator_t *tnl_info,
                                         vlan_xlate_entry_t   *tr_ent, uint8 out_mode, int clean_flag)
 {
    if (clean_flag) {
         sal_memset(tr_ent, 0, sizeof(vlan_xlate_entry_t));
    }

    if (SOC_IS_TRIUMPH3(unit)) {
        soc_VLAN_XLATEm_field32_set(unit, tr_ent, KEY_TYPEf, 
                       _BCM_TR3_L2GRE_KEY_TYPE_LOOKUP_DIP); /* L2GRE_DIP Key */
    } else if  (SOC_IS_TRIDENT2(unit)) {
        soc_VLAN_XLATEm_field32_set(unit, tr_ent, KEY_TYPEf, 
                       _BCM_TD2_L2GRE_KEY_TYPE_LOOKUP_DIP); /* L2GRE_DIP Key */
    }

    soc_VLAN_XLATEm_field32_set(unit, tr_ent, VALIDf, 0x1);
    soc_VLAN_XLATEm_field32_set(unit, tr_ent,  
                                    L2GRE_DIP__DIPf, tnl_info->dip);
    if (out_mode == _BCM_L2GRE_MULTICAST_BUD) {
        soc_VLAN_XLATEm_field32_set(unit, tr_ent,  
                                    L2GRE_DIP__NETWORK_RECEIVERS_PRESENTf, 0x1);
        _bcm_tr3_l2gre_bud_loopback_enable(unit);
    } else if (out_mode == _BCM_L2GRE_MULTICAST_LEAF) {
        soc_VLAN_XLATEm_field32_set(unit, tr_ent,  
                                    L2GRE_DIP__NETWORK_RECEIVERS_PRESENTf, 0x0);
    }
 }

/*
 * Function:
 *          _bcm_tr3_l2gre_tunnel_terminator_reference_count
 * Purpose:
 *          Find L2GRE Tunnel terminator DIP reference count
 * Returns: BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_tunnel_terminator_reference_count(int unit, bcm_ip_t dest_ip_addr)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int tunnel_idx=0, num_vp=0, ref_count=0;

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    l2gre_info = L2GRE_INFO(unit);

      for (tunnel_idx=0; tunnel_idx< num_vp; tunnel_idx++) {
          if (l2gre_info->l2gre_tunnel_term[tunnel_idx].dip == dest_ip_addr) {
              ref_count ++;
          }
      }
    return ref_count;
}

/*
 * Function:
 *          _bcm_tr3_l2gre_multicast_tunnel_state_set
 * Purpose:
 *          Set L2GRE Tunnel terminator Multicast State
 * Returns: BCM_E_XXX
 */


STATIC void
_bcm_tr3_l2gre_multicast_tunnel_state_set (int unit, int  tunnel_idx, uint8 tunnel_state)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;

    l2gre_info = L2GRE_INFO(unit);

    if (tunnel_idx != -1) {
        l2gre_info->l2gre_tunnel_term[tunnel_idx].tunnel_state = tunnel_state;
    }
}


/*
 * Function:
 *          _bcm_tr3_l2gre_multicast_tunnel_state_index_get
 * Purpose:
 *          Get L2GRE Tunnel terminator Multicast State and Tunnel Index
 * Returns: BCM_E_XXX
 */


STATIC void
_bcm_tr3_l2gre_multicast_tunnel_state_index_get  (int unit, 
                            bcm_ip_t mc_ip_addr, bcm_ip_t  src_ip_addr, uint8 *tunnel_state, int *tunnel_index)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int idx=0, num_vp=0;

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    l2gre_info = L2GRE_INFO(unit);

      for (idx=0; idx< num_vp; idx++) {
          if (l2gre_info->l2gre_tunnel_term[idx].dip == mc_ip_addr) {
              if (l2gre_info->l2gre_tunnel_term[idx].sip == src_ip_addr) {
                    *tunnel_state = l2gre_info->l2gre_tunnel_term[idx].tunnel_state;
                    *tunnel_index = idx;
                     break;
              }
          }
      }
}

/*
 * Function:
 *          _bcm_tr3_l2gre_tunnel_terminator_state_find
 * Purpose:
 *          Find L2GRE Tunnel terminator DIP State
 * Returns: BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_tunnel_terminator_state_find(int unit, bcm_ip_t dest_ip_addr)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int tunnel_idx=0, num_vp=0, tunnel_state=0;

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    l2gre_info = L2GRE_INFO(unit);

    for (tunnel_idx=0; tunnel_idx< num_vp; tunnel_idx++) {
          if (l2gre_info->l2gre_tunnel_term[tunnel_idx].dip == dest_ip_addr) {
              if (l2gre_info->l2gre_tunnel_term[tunnel_idx].tunnel_state == _BCM_L2GRE_TUNNEL_TERM_MULTICAST_LEAF ) {
                 tunnel_state += 0;
              } else if (l2gre_info->l2gre_tunnel_term[tunnel_idx].tunnel_state == _BCM_L2GRE_TUNNEL_TERM_MULTICAST_BUD ) {
                 tunnel_state += 1;
              }
          }
    }
    return tunnel_state;
}

/*
 * Function:
 *      bcm_tr3_l2gre_multicast_leaf_entry_check
 * Purpose:
 *      To find whether LEAF-multicast entry exists for DIP?
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr3_l2gre_multicast_leaf_entry_check(int unit, bcm_ip_t mc_ip_addr, bcm_ip_t  src_ip_addr, int multicast_flag)
{
    vlan_xlate_entry_t vxlate_entry;
    int rv=BCM_E_NONE, index=0;
    int tunnel_ref_count = 0;
    uint8 tunnel_multicast_state = 0;
    int tunnel_mc_states = 0, tunnel_idx=0;

    tunnel_ref_count = _bcm_tr3_l2gre_tunnel_terminator_reference_count(unit, mc_ip_addr);

    if (tunnel_ref_count < 1) {
        /* Tunnel not created */
        return BCM_E_NONE;
    } else if (tunnel_ref_count >= 1) {
         (void ) _bcm_tr3_l2gre_multicast_tunnel_state_index_get (unit, 
                           mc_ip_addr, src_ip_addr, &tunnel_multicast_state, &tunnel_idx);

          /* Set (SIP, DIP) based Software State */
          if (multicast_flag == BCM_L2GRE_MULTICAST_TUNNEL_STATE_BUD_ENABLE) {
              if (tunnel_multicast_state == _BCM_L2GRE_TUNNEL_TERM_MULTICAST_LEAF) {
                   (void) _bcm_tr3_l2gre_multicast_tunnel_state_set(unit, 
                                 tunnel_idx, _BCM_L2GRE_TUNNEL_TERM_MULTICAST_BUD);
              }
          } else  if (multicast_flag == BCM_L2GRE_MULTICAST_TUNNEL_STATE_BUD_DISABLE){
              if (tunnel_multicast_state == _BCM_L2GRE_TUNNEL_TERM_MULTICAST_BUD) {
                   (void) _bcm_tr3_l2gre_multicast_tunnel_state_set(unit, 
                                 tunnel_idx, _BCM_L2GRE_TUNNEL_TERM_MULTICAST_LEAF);
              }
          }

          /* Set DIP-based cumulative hardware State */
         tunnel_mc_states = _bcm_tr3_l2gre_tunnel_terminator_state_find(unit, mc_ip_addr);

         sal_memset(&vxlate_entry, 0, sizeof(vlan_xlate_entry_t));
         if (SOC_IS_TRIUMPH3(unit)) {
            soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, KEY_TYPEf, 
                       _BCM_TR3_L2GRE_KEY_TYPE_LOOKUP_DIP); /* L2GRE_DIP Key */
         } else if  (SOC_IS_TRIDENT2(unit)) {
            soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, KEY_TYPEf, 
                       _BCM_TD2_L2GRE_KEY_TYPE_LOOKUP_DIP); /* L2GRE_DIP Key */
         }

         soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VALIDf, 0x1);
         soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    L2GRE_DIP__DIPf, mc_ip_addr);

         rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                        &vxlate_entry, &vxlate_entry, 0);

         if (rv == SOC_E_NONE) {
              if (tunnel_mc_states) {
                   /* Create Transit entry: LEAF to BUD */
                   soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    L2GRE_DIP__NETWORK_RECEIVERS_PRESENTf, 0x1);
                   _bcm_tr3_l2gre_bud_loopback_enable(unit);
              } else {
                   /* Delete Transit entry : BUD to LEAF */
                   soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    L2GRE_DIP__NETWORK_RECEIVERS_PRESENTf, 0x0);
              }
              rv = soc_mem_write(unit, VLAN_XLATEm,
                                           MEM_BLOCK_ALL, index,
                                           &vxlate_entry);
         }
    }
    return BCM_E_NONE;
}

/*  
 * Function:
 *      bcm_tr3_l2gre_tunnel_terminator_create
 * Purpose:
 *      Set L2GRE Tunnel terminator
 * Parameters:
 *      unit - Device Number
 *      tnl_info
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_l2gre_tunnel_terminator_create(int unit, bcm_tunnel_terminator_t *tnl_info)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int index=-1, tunnel_idx=-1;
    vlan_xlate_entry_t vxlate_entry, return_entry;
    int rv = BCM_E_NONE;

    if (tnl_info->type != bcmTunnelTypeL2Gre) {
       return BCM_E_PARAM;
    }
    l2gre_info = L2GRE_INFO(unit);

    /* Program tunnel id */
    if (tnl_info->flags & BCM_TUNNEL_TERM_TUNNEL_WITH_ID) {
        if (!BCM_GPORT_IS_TUNNEL(tnl_info->tunnel_id)) {
            return BCM_E_PARAM;
        }
        tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(tnl_info->tunnel_id);
    } else {
        /* Only BCM_TUNNEL_TERM_TUNNEL_WITH_ID supported */
        /* Use tunnel_id retuned by _create */
        return BCM_E_PARAM;
    }

    /* Add First Tunnel term-entry for given DIP to Hardware */
    if (_bcm_tr3_l2gre_tunnel_terminator_reference_count(unit, tnl_info->dip) < 1)  {

         (void) _bcm_tr3_l2gre_tunnel_terminate_entry_key_set(unit, tnl_info, &vxlate_entry, 0, 1);

         rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                        &vxlate_entry, &return_entry, 0);

         if (rv == SOC_E_NONE) {
              (void) _bcm_tr3_l2gre_tunnel_terminate_entry_key_set(unit, tnl_info, &vxlate_entry, 0, 0);
              rv = soc_mem_write(unit, VLAN_XLATEm,
                                           MEM_BLOCK_ALL, index,
                                           &return_entry);
         } else if (rv != SOC_E_NOT_FOUND) {
              return rv;
         } else {
              rv = soc_mem_insert(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vxlate_entry);
         }
    }

    if (BCM_SUCCESS(rv) && (tunnel_idx != -1)) {
       l2gre_info->l2gre_tunnel_term[tunnel_idx].sip = tnl_info->sip;
       l2gre_info->l2gre_tunnel_term[tunnel_idx].dip = tnl_info->dip;
       l2gre_info->l2gre_tunnel_term[tunnel_idx].tunnel_state = _BCM_L2GRE_TUNNEL_TERM_MULTICAST_LEAF;
    }
    return rv;
}

/*  
 * Function:
 *      bcm_tr3_l2gre_tunnel_terminator_update
 * Purpose:
 *     Update L2GRE Tunnel terminator multicast state
 * Parameters:
 *      unit - Device Number
 *      tnl_info
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_l2gre_tunnel_terminator_update(int unit, bcm_tunnel_terminator_t *tnl_info)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int rv = BCM_E_NONE;
    int tunnel_idx=-1;
    bcm_ip_t mc_ip_addr, src_ip_addr;

    l2gre_info = L2GRE_INFO(unit);

    /* Program tunnel id */
    if (tnl_info->flags & BCM_TUNNEL_TERM_TUNNEL_WITH_ID) {
        if (!BCM_GPORT_IS_TUNNEL(tnl_info->tunnel_id)) {
            return BCM_E_PARAM;
        }
        tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(tnl_info->tunnel_id);
    } else {
        /* Only BCM_TUNNEL_TERM_TUNNEL_WITH_ID supported */
        /* Use tunnel_id retuned by _create */
        return BCM_E_PARAM; 
    }

    mc_ip_addr = l2gre_info->l2gre_tunnel_term[tunnel_idx].dip;
    src_ip_addr = l2gre_info->l2gre_tunnel_term[tunnel_idx].sip;

    rv = bcm_tr3_l2gre_multicast_leaf_entry_check(unit, 
                   mc_ip_addr, src_ip_addr, tnl_info->multicast_flag);
    return rv;
}

/*
 * Function:
 *  bcm_tr3_l2gre_tunnel_terminate_destroy
 * Purpose:
 *  Destroy L2GRE  tunnel_terminate Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  l2gre_tunnel_id
 * Returns:
 *  BCM_E_XXX
 */

int
bcm_tr3_l2gre_tunnel_terminator_destroy(int unit, bcm_gport_t l2gre_tunnel_id)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int tunnel_idx, index;
    vlan_xlate_entry_t  vxlate_entry;
    uint32  tunnel_dip=0;
    int rv = BCM_E_NONE;

    l2gre_info = L2GRE_INFO(unit);

    if (!BCM_GPORT_IS_TUNNEL(l2gre_tunnel_id)) {
        return BCM_E_PARAM;
    }
    tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(l2gre_tunnel_id);

    sal_memset(&vxlate_entry, 0, sizeof(vlan_xlate_entry_t));
    tunnel_dip = l2gre_info->l2gre_tunnel_term[tunnel_idx].dip;

    /* Remove Tunnel term-entry for given DIP from Hardware based on ref-count */
    if (_bcm_tr3_l2gre_tunnel_terminator_reference_count(unit, tunnel_dip) == 1)  {

        if (SOC_IS_TRIUMPH3(unit)) {
            soc_VLAN_XLATEm_field32_set(unit,  &vxlate_entry, KEY_TYPEf, 
                           _BCM_TR3_L2GRE_KEY_TYPE_LOOKUP_DIP); /* L2GRE_DIP Key */
        } else if  (SOC_IS_TRIDENT2(unit)) {
            soc_VLAN_XLATEm_field32_set(unit,  &vxlate_entry, KEY_TYPEf, 
                           _BCM_TD2_L2GRE_KEY_TYPE_LOOKUP_DIP); /* L2GRE_DIP Key */
        }
        soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VALIDf, 0x1);
        soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    L2GRE_DIP__DIPf, tunnel_dip);
        soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    L2GRE_DIP__NETWORK_RECEIVERS_PRESENTf, 0);

        rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                             &vxlate_entry, &vxlate_entry, 0);
         if (BCM_FAILURE(rv)) {
              return rv;
         }
        /* Delete the entry from HW */
        rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vxlate_entry);
        if (BCM_FAILURE(rv)) {
            return rv;
        }
    }

    if (BCM_SUCCESS(rv) && (tunnel_idx != -1)) {
       l2gre_info->l2gre_tunnel_term[tunnel_idx].dip = 0;
       l2gre_info->l2gre_tunnel_term[tunnel_idx].tunnel_state = _BCM_L2GRE_TUNNEL_TERM_MULTICAST_LEAF;
    }
 
    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_tunnel_terminator_destroy_all
 * Purpose:
 *      Destroy all incoming L2GRE tunnel
 * Parameters:
 *      unit           - (IN) Device Number
 */
int
bcm_tr3_l2gre_tunnel_terminator_destroy_all(int unit) 
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int rv = BCM_E_UNAVAIL, i, num_entries=0,tnl_idx=0,num_vp=0;
    vlan_xlate_entry_t  vxlate_entry;

    l2gre_info = L2GRE_INFO(unit);
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    num_entries = soc_mem_index_count(unit, VLAN_XLATEm);
    for (i = 0; i < num_entries; i++) {
        rv = READ_VLAN_XLATEm(unit, MEM_BLOCK_ANY, i, &vxlate_entry);
        if (rv < 0) {
            return rv;
        }
        if (!soc_VLAN_XLATEm_field32_get(unit, &vxlate_entry, VALIDf)) {
            continue;
        }

        if (SOC_IS_TRIUMPH3(unit)) {
            if (soc_VLAN_XLATEm_field32_get(unit, &vxlate_entry,
                                        KEY_TYPEf) != _BCM_TR3_L2GRE_KEY_TYPE_LOOKUP_DIP) {
                continue;
            }
        } else if  (SOC_IS_TRIDENT2(unit)) {
            if (soc_VLAN_XLATEm_field32_get(unit, &vxlate_entry,
                                        KEY_TYPEf) != _BCM_TD2_L2GRE_KEY_TYPE_LOOKUP_DIP) {
                continue;
            }
        }

        /* Delete the entry from HW */
        rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vxlate_entry);
        if (rv < 0) {
            return rv;
        }
    }

    for (tnl_idx = 0; tnl_idx < num_vp; tnl_idx++) {
       l2gre_info->l2gre_tunnel_term[tnl_idx].dip = 0;
       l2gre_info->l2gre_tunnel_term[tnl_idx].tunnel_state = 0;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_l2gre_tunnel_initiator_get
 * Purpose:
 *      Get L2GRE  tunnel_terminate Entry
 * Parameters:
 *      unit    - (IN) Device Number
 *      info    - (IN/OUT) Tunnel terminator structure
 */
int
bcm_tr3_l2gre_tunnel_terminator_get(int unit, bcm_tunnel_terminator_t *tnl_info)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int tunnel_idx=-1, index=-1;
    vlan_xlate_entry_t  vxlate_entry;
    uint32  tunnel_dip=0;
    int rv = BCM_E_UNAVAIL;

    l2gre_info = L2GRE_INFO(unit);

    /* Program tunnel id */
    if (!BCM_GPORT_IS_TUNNEL(tnl_info->tunnel_id)) {
        return BCM_E_PARAM;
    }
    tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(tnl_info->tunnel_id);

    sal_memset(&vxlate_entry, 0, sizeof(vlan_xlate_entry_t));
    tunnel_dip = l2gre_info->l2gre_tunnel_term[tunnel_idx].dip;

    if (SOC_IS_TRIUMPH3(unit)) {
        soc_VLAN_XLATEm_field32_set(unit,  &vxlate_entry, KEY_TYPEf, 
                       _BCM_TR3_L2GRE_KEY_TYPE_LOOKUP_DIP); /* L2GRE_DIP Key */
    } else if  (SOC_IS_TRIDENT2(unit)) {
        soc_VLAN_XLATEm_field32_set(unit,  &vxlate_entry, KEY_TYPEf, 
                       _BCM_TD2_L2GRE_KEY_TYPE_LOOKUP_DIP); /* L2GRE_DIP Key */
    }
    soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VALIDf, 0x1);
    soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    L2GRE_DIP__DIPf, tunnel_dip);

    rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                             &vxlate_entry, &vxlate_entry, 0);

    if (BCM_FAILURE(rv)) {
         return rv;
    }

    tnl_info->sip = l2gre_info->l2gre_tunnel_term[tunnel_idx].sip;
    tnl_info->dip = l2gre_info->l2gre_tunnel_term[tunnel_idx].dip;
    tnl_info->type = bcmTunnelTypeL2Gre;
    if (l2gre_info->l2gre_tunnel_term[tunnel_idx].tunnel_state == _BCM_L2GRE_TUNNEL_TERM_MULTICAST_BUD) {
        tnl_info->multicast_flag = BCM_L2GRE_MULTICAST_TUNNEL_STATE_BUD_ENABLE;
    } else if (l2gre_info->l2gre_tunnel_term[tunnel_idx].tunnel_state == _BCM_L2GRE_TUNNEL_TERM_MULTICAST_LEAF){
        tnl_info->multicast_flag = BCM_L2GRE_MULTICAST_TUNNEL_STATE_BUD_DISABLE;
    } else {
        tnl_info->multicast_flag = 0;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_l2gre_tunnel_terminator_traverse
 * Purpose:
 *      Traverse L2GRE tunnel terminator entries
 * Parameters:
 *      unit    - (IN) Device Number
 *      cb    - (IN) User callback function, called once per entry
 *      user_data    - (IN) User supplied cookie used in parameter in callback function 
 */
int
bcm_tr3_l2gre_tunnel_terminator_traverse(int unit, bcm_tunnel_terminator_traverse_cb cb, void *user_data)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int num_tnl = 0, idx = 0;
    bcm_tunnel_terminator_t info;
    int rv = BCM_E_NONE;

    l2gre_info = L2GRE_INFO(unit);
    num_tnl = soc_mem_index_count(unit, SOURCE_VPm);

    /* Iterate over all the entries - search valid ones. */
    for(idx = 0; idx < num_tnl; idx++) {
        /* Check a valid entry */
        if (l2gre_info->l2gre_tunnel_term[idx].dip) {
            /* Reset buffer before read. */
            sal_memset(&info, 0, sizeof(bcm_tunnel_terminator_t));

            BCM_GPORT_TUNNEL_ID_SET(info.tunnel_id, idx);

            rv = bcm_tr3_l2gre_tunnel_terminator_get(unit,&info);

            /* search failure */
            if (BCM_FAILURE(rv)) {
                break;
            }            

            if (cb) {
                rv = (*cb) (unit, &info, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
                if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                    break;
                }
#endif
            }
        }
    }

    return rv;
}


/*
 * Function:
 *      _bcm_tr3_l2gre_tunnel_init_add
 * Purpose:
 *      Add tunnel initiator entry to hw. 
 * Parameters:
 *      unit     - (IN)SOC unit number.
 *      idx      - (IN)Index to insert tunnel initiator.
 *      info     - (IN)Tunnel initiator entry info.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr3_l2gre_tunnel_init_add(int unit, int idx, bcm_tunnel_initiator_t *info)
{
    uint32 tnl_entry[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to write hw entry */
    soc_mem_t mem;                        /* Tunnel initiator table memory */  
    int rv = BCM_E_NONE;                  /* Return value                  */
    int df_val;                           /* Don't fragment encoding       */

    mem = EGR_IP_TUNNELm; 

    /* Zero write buffer. */
    sal_memset(tnl_entry, 0, sizeof(tnl_entry));

    /* If replacing existing entry, first read the entry to get old 
       profile pointer and TPID */
    if (info->flags & BCM_TUNNEL_REPLACE) {
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, idx, tnl_entry);
        BCM_IF_ERROR_RETURN(rv);
    }

    /* Set destination address. */
    soc_mem_field_set(unit, mem, tnl_entry, DIPf, (uint32 *)&info->dip);

    /* Set source address. */
    soc_mem_field_set(unit, mem, tnl_entry, SIPf, (uint32 *)&info->sip);

    /* IP tunnel hdr DF bit for IPv4. */
    df_val = 0;
    if (info->flags & BCM_TUNNEL_INIT_USE_INNER_DF) {
        df_val |= 0x2;
    } else if (info->flags & BCM_TUNNEL_INIT_IPV4_SET_DF) { 
        df_val |= 0x1;
    }
    soc_mem_field32_set(unit, mem, tnl_entry, IPV4_DF_SELf, df_val);

    /* Set DSCP value.  */
    soc_mem_field32_set(unit, mem, tnl_entry, DSCPf, info->dscp);

    /* Tunnel header DSCP select. */
    soc_mem_field32_set(unit, mem, tnl_entry, DSCP_SELf, info->dscp_sel);

    /* Set TTL. */
    soc_mem_field32_set(unit, mem, tnl_entry, TTLf, info->ttl);

    /* Set Tunnel type = L2GRE */
    soc_mem_field32_set(unit, mem, tnl_entry, TUNNEL_TYPEf, _BCM_L2GRE_TUNNEL_TYPE);

    /* Set entry type = IPv4 */
    soc_mem_field32_set(unit, mem, tnl_entry, ENTRY_TYPEf, 0x1);

    /* Write buffer to hw. */
    rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, idx, tnl_entry);
    return rv;

}


/*  
 * Function:
 *      _bcm_tr3_l2gre_tunnel_initiator_entry_add
 * Purpose:
 *      Configure L2GRE Tunnel initiator hardware Entry
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr3_l2gre_tunnel_initiator_entry_add(int unit, uint32 flags, bcm_tunnel_initiator_t *info, int *tnl_idx)
{
   int rv = BCM_E_NONE;

   rv = bcm_xgs3_tnl_init_add(unit, flags, info, tnl_idx);
   if (BCM_FAILURE(rv)) {
         return rv;
   }
   /* Write the entry to HW */
   rv = _bcm_tr3_l2gre_tunnel_init_add(unit, *tnl_idx, info);
   if (BCM_FAILURE(rv)) {
        flags = _BCM_L3_SHR_WRITE_DISABLE;
        (void) bcm_xgs3_tnl_init_del(unit, flags, *tnl_idx);
   }
   return rv;
}

/*  
 * Function:
 *      bcm_tr3_l2gre_tunnel_initiator_create
 * Purpose:
 *      Set L2GRE Tunnel initiator
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_l2gre_tunnel_initiator_create(int unit, bcm_tunnel_initiator_t *info)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int tnl_idx=-1, rv = BCM_E_NONE;
    uint32 flags;
    int idx=0, num_tnl=0, num_vp=0;
    uint8  sip_entry_present=0;
    uint32 tnl_entry[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry.  */

    /* Input parameters check. */
    if (NULL == info) {
        return (BCM_E_PARAM);
    }   
    if (info->type != bcmTunnelTypeL2Gre)  {
        return BCM_E_PARAM;
    }
    if (!BCM_TTL_VALID(info->ttl)) {
        return BCM_E_PARAM;
    }
    if (info->dscp_sel > 2 || info->dscp_sel < 0) {
        return BCM_E_PARAM;
    }
    if (info->dscp > 63 || info->dscp < 0) {
        return BCM_E_PARAM;
    }
   
    l2gre_info = L2GRE_INFO(unit);
    num_tnl = soc_mem_index_count(unit, EGR_IP_TUNNELm);
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    /* Allocate either existing or new tunnel initiator entry */
    flags = _BCM_L3_SHR_MATCH_DISABLE | _BCM_L3_SHR_WRITE_DISABLE |
             _BCM_L3_SHR_SKIP_INDEX_ZERO;

    /* Check if Same SIP exists: MAP first 512 entries to EGR_IP_TUNNEL
        Rest of entries map to  L2GRE_TUNNEL with Same SIP, different DIP */
    for (idx=0; idx < num_tnl; idx++) {
        if (l2gre_info->l2gre_tunnel_init[idx].sip == info->sip) {
            tnl_idx = idx; /* Entry_idx within Hardware-Range */
            sip_entry_present = 1;
            break;
        }
    }

    /*  Check if Same SIP exists in Soft-state */
    if (!sip_entry_present) {
       for (idx=num_tnl; idx < num_vp; idx++) {
          if (l2gre_info->l2gre_tunnel_init[idx].sip == info->sip) {
            for (tnl_idx=0; tnl_idx < num_tnl; tnl_idx++) {
                 BCM_IF_ERROR_RETURN(soc_mem_read(unit, EGR_IP_TUNNELm, MEM_BLOCK_ANY, tnl_idx, tnl_entry));
                 if (info->sip == soc_mem_field32_get(unit, EGR_IP_TUNNELm, tnl_entry, SIPf)) {
                    /* Entry_idx within Hardware-Range */
                    BCM_GPORT_TUNNEL_ID_SET(info->tunnel_id, tnl_idx);
                    info->flags |= BCM_TUNNEL_REPLACE;
                    break;
                 }
            }           
            break;
          }
       }
    }

    if (sip_entry_present) {
         /* 2 Cases: Entry-update, (Multiple-dip, same-SIP) */
         if (l2gre_info->l2gre_tunnel_init[tnl_idx].dip != info->dip) {
             /* Obtain index to Software-State to store (SIP,DIP) */
             for (idx=num_tnl; idx < num_vp; idx++) {
                 if (l2gre_info->l2gre_tunnel_init[idx].sip == 0) {
                     tnl_idx = idx; /* Obtain Free index */
                     break;
                 }
             }
         } else {
             /* Parse tunnel replace flag */
             if (info->flags & BCM_TUNNEL_REPLACE) {
                flags |=  _BCM_L3_SHR_UPDATE | _BCM_L3_SHR_WITH_ID;
                tnl_idx = BCM_GPORT_TUNNEL_ID_GET(info->tunnel_id);
                rv = _bcm_tr3_l2gre_tunnel_initiator_entry_add(unit, flags, info, &tnl_idx);
                if (BCM_FAILURE(rv)) {
                   return rv;
                }
             }
         }
    } else {
         /* Parse tunnel replace flag */
         if (info->flags & BCM_TUNNEL_REPLACE) {
            flags |=  _BCM_L3_SHR_UPDATE | _BCM_L3_SHR_WITH_ID;
            tnl_idx = BCM_GPORT_TUNNEL_ID_GET(info->tunnel_id);
         }
         rv = _bcm_tr3_l2gre_tunnel_initiator_entry_add(unit, flags, info, &tnl_idx);
         if (BCM_FAILURE(rv)) {
             return rv;
         }
    }

    if (tnl_idx !=- 1) {
         l2gre_info->l2gre_tunnel_init[tnl_idx].sip = info->sip;
         l2gre_info->l2gre_tunnel_init[tnl_idx].dip = info->dip;
         BCM_GPORT_TUNNEL_ID_SET(info->tunnel_id, tnl_idx);
    } else {
        return BCM_E_NOT_FOUND;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_tunnel_init_get
 * Purpose:
 *      Get a tunnel initiator entry from hw. 
 * Parameters:
 *      unit     - (IN)SOC unit number.
 *      idx      - (IN/OUT)Index to read tunnel initiator.
 *      info     - (OUT)Tunnel initiator entry info.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr3_l2gre_tunnel_init_get(int unit, int *hw_idx, bcm_tunnel_initiator_t *info)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    uint32 tnl_entry[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry.  */
    soc_mem_t mem;                         /* Tunnel initiator table memory.*/  
    int df_val;                            /* Don't fragment encoded value. */
    int tnl_type;                          /* Tunnel type.                  */
    uint32 entry_type = BCM_XGS3_TUNNEL_INIT_V4;/* Entry type (IPv4/IPv6/MPLS)*/
    int num_tnl=0, tnl_idx =0, idx=0;
    bcm_ip_t sip=0;                       /* Tunnel header SIP (IPv4). */

    /* Get table memory. */
    mem = EGR_IP_TUNNELm; 
    idx = *hw_idx;

    /* Initialize the buffer. */
    sal_memset(tnl_entry, 0, sizeof(tnl_entry));
    l2gre_info = L2GRE_INFO(unit);
    num_tnl = soc_mem_index_count(unit, EGR_IP_TUNNELm);

    if (idx >= num_tnl) { /* Software State */
         sip = l2gre_info->l2gre_tunnel_init[idx].sip;
         info->dip = l2gre_info->l2gre_tunnel_init[idx].dip;

        /* Find matching entry within Hardware */
       for (tnl_idx=0; tnl_idx < num_tnl; tnl_idx++) {
            BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY, tnl_idx, tnl_entry));
            if (sip == soc_mem_field32_get(unit, mem, tnl_entry, SIPf)) {
                idx = tnl_idx; /* Entry_idx within Hardware-Range */
                break;
            }
       }
    } else {
         info->dip = l2gre_info->l2gre_tunnel_init[idx].dip;
         BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY, idx, tnl_entry));
    }

    /* Get tunnel type. */
    tnl_type = soc_mem_field32_get(unit, mem, tnl_entry, TUNNEL_TYPEf);
    if (tnl_type != _BCM_L2GRE_TUNNEL_TYPE) {
        return BCM_E_CONFIG;
    }

    /* Get entry_type. */
    entry_type = soc_mem_field32_get(unit, mem, tnl_entry, ENTRY_TYPEf); 
    if (entry_type != 0x1) {
        return BCM_E_CONFIG;
    }

    /* Parse hw buffer. */
    /* Check will not fail. entry_type already validated */
    if (BCM_XGS3_TUNNEL_INIT_V4 == entry_type) {
        /* Get destination ip. */
        info->dip = soc_mem_field32_get(unit, mem, tnl_entry, DIPf); 

        /* Get source ip. */
        info->sip = soc_mem_field32_get(unit, mem, tnl_entry, SIPf);
    } 

    /* Tunnel header DSCP select. */
    info->dscp_sel = soc_mem_field32_get(unit, mem, tnl_entry, DSCP_SELf);

    /* Tunnel header DSCP value. */
    info->dscp = soc_mem_field32_get(unit, mem, tnl_entry, DSCPf);

    /* IP tunnel hdr DF bit for IPv4 */
    df_val = soc_mem_field32_get(unit, mem, tnl_entry, IPV4_DF_SELf);
    if (0x2 <= df_val) {
        info->flags |= BCM_TUNNEL_INIT_USE_INNER_DF;  
    } else if (0x1 == df_val) {
        info->flags |= BCM_TUNNEL_INIT_IPV4_SET_DF;  
    }

    /* Get TTL. */
    info->ttl = soc_mem_field32_get(unit, mem, tnl_entry, TTLf);

    /* Translate hw tunnel type into API encoding. - bcmTunnelTypeL2Gre */
    BCM_IF_ERROR_RETURN(_bcm_trx_tnl_hw_code_to_type(unit, tnl_type,
                                                     entry_type,
                                                     &info->type));
    *hw_idx = idx;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_l2gre_tunnel_initiator_destroy
 * Purpose:
 *      Destroy outgoing L2GRE tunnel
 * Parameters:
 *      unit           - (IN) Device Number
 *      l2gre_tunnel_id - (IN) Tunnel ID (Gport)
 */
int
bcm_tr3_l2gre_tunnel_initiator_destroy(int unit, bcm_gport_t l2gre_tunnel_id) 
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int tnl_idx=-1, temp_tnl_idx=-1, rv = BCM_E_NONE, ref_count=0, idx=0;
    uint32 flags = 0;
    bcm_tunnel_initiator_t info;           /* Tunnel initiator structure */
    int num_vp=0;
    bcm_ip_t sip=0;                       /* Tunnel header SIP (IPv4). */

    l2gre_info = L2GRE_INFO(unit);
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    /* Input parameters check. */
    if (!BCM_GPORT_IS_TUNNEL(l2gre_tunnel_id)) {
        return BCM_E_PARAM;
    }
    tnl_idx = BCM_GPORT_TUNNEL_ID_GET(l2gre_tunnel_id);

    bcm_tunnel_initiator_t_init(&info);
    sip = l2gre_info->l2gre_tunnel_init[tnl_idx].sip;
    /* Remove HW TNL entry only if all matching SIP gets removed */
    for (idx=0; idx < num_vp; idx++) {
        if (l2gre_info->l2gre_tunnel_init[idx].sip == sip) {
            ref_count++;
        }
    }
    if (ref_count == 1) {
       /* Get the entry first */
       temp_tnl_idx = tnl_idx;
       rv = _bcm_tr3_l2gre_tunnel_init_get(unit, &tnl_idx, &info);
       if (BCM_FAILURE(rv)) {
          return rv;
       }
       /* Destroy the tunnel entry */
       (void) bcm_xgs3_tnl_init_del(unit, flags, tnl_idx);
       l2gre_info->l2gre_tunnel_init[temp_tnl_idx].sip = 0;
       l2gre_info->l2gre_tunnel_init[temp_tnl_idx].dip = 0;
    } else if (ref_count) {
       l2gre_info->l2gre_tunnel_init[tnl_idx].sip = 0;
       l2gre_info->l2gre_tunnel_init[tnl_idx].dip = 0;
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_tunnel_initiator_destroy_all
 * Purpose:
 *      Destroy all outgoing L2GRE tunnel
 * Parameters:
 *      unit           - (IN) Device Number
 */
int
bcm_tr3_l2gre_tunnel_initiator_destroy_all(int unit) 
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    uint32 tnl_entry[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry.  */
    int rv = BCM_E_NONE, tnl_idx, num_entries, num_vp;

    l2gre_info = L2GRE_INFO(unit);
    num_entries = soc_mem_index_count(unit, EGR_IP_TUNNELm);
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    for (tnl_idx = 0; tnl_idx < num_entries; tnl_idx++) {
        rv = READ_EGR_IP_TUNNELm(unit, MEM_BLOCK_ANY, tnl_idx, &tnl_entry);
        if (rv < 0) {
            return rv;
        }
        if (!soc_EGR_IP_TUNNELm_field32_get(unit, &tnl_entry, ENTRY_TYPEf)) {
            continue;
        }
        if (soc_EGR_IP_TUNNELm_field32_get(unit, &tnl_entry,
                                        TUNNEL_TYPEf) != _BCM_L2GRE_TUNNEL_TYPE) {
            continue;
        }

       /* Destroy the tunnel entry */
       (void) bcm_xgs3_tnl_init_del(unit, 0, tnl_idx);
       l2gre_info->l2gre_tunnel_init[tnl_idx].sip = 0;
       l2gre_info->l2gre_tunnel_init[tnl_idx].dip = 0;
    }

    for (tnl_idx = num_entries; tnl_idx < num_vp; tnl_idx++) {
         l2gre_info->l2gre_tunnel_init[tnl_idx].sip = 0;
         l2gre_info->l2gre_tunnel_init[tnl_idx].dip = 0;
    }
    return BCM_E_NONE;

}

/*
 * Function:
 *      bcm_tr3_l2gre_tunnel_initiator_get
 * Purpose:
 *      Get an outgong L2GRE tunnel info
 * Parameters:
 *      unit    - (IN) Device Number
 *      info    - (IN/OUT) Tunnel initiator structure
 */
int
bcm_tr3_l2gre_tunnel_initiator_get(int unit, bcm_tunnel_initiator_t *info)
{
    int tnl_idx, rv = BCM_E_NONE;

    /* Input parameters check. */
    if (info == NULL) {
        return BCM_E_PARAM;
    }
    if (!BCM_GPORT_IS_TUNNEL(info->tunnel_id)) {
        return BCM_E_PARAM;
    }
    tnl_idx = BCM_GPORT_TUNNEL_ID_GET(info->tunnel_id);

    /* Get the entry */
    rv = _bcm_tr3_l2gre_tunnel_init_get(unit, &tnl_idx, info);
    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_tunnel_initiator_traverse
 * Purpose:
 *      Traverse L2GRE tunnel initiator entries
 * Parameters:
 *      unit    - (IN) Device Number
 *      cb    - (IN) User callback function, called once per entry
 *      user_data    - (IN) User supplied cookie used in parameter in callback function 
 */
int
bcm_tr3_l2gre_tunnel_initiator_traverse(int unit, bcm_tunnel_initiator_traverse_cb cb, void *user_data)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int num_vp = 0, idx = 0;
    bcm_tunnel_initiator_t info;
    int rv = BCM_E_NONE;

    l2gre_info = L2GRE_INFO(unit);
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    /* Iterate over all the entries - search valid ones. */
    for(idx = 0; idx < num_vp; idx++) {
        /* Check a valid entry */
        if (l2gre_info->l2gre_tunnel_init[idx].sip != 0) {
           /* Reset buffer before read. */
           sal_memset(&info, 0, sizeof(bcm_tunnel_initiator_t));

           BCM_GPORT_TUNNEL_ID_SET(info.tunnel_id, idx);
           rv = bcm_tr3_l2gre_tunnel_initiator_get(unit, &info);

           /* search failure */
           if (BCM_FAILURE(rv)) {
               if (rv != BCM_E_CONFIG) {
                   break;
               }
               continue;
           }  

           if (cb) {
               rv = (*cb) (unit, &info, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
               if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                   break;
               }
#endif
           }
        }
    }

    /* Reset last read status. */
    if (BCM_E_CONFIG == rv) {
        rv = BCM_E_NONE;
    }

    return rv;
}

/*
 * Function:
 *     _bcm_tr3_l2gre_ingress_dvp_set
 * Purpose:
 *     Set Ingress DVP tables
 * Parameters:
 *   IN :  Unit
 *   IN :  vp
 *   IN :  l2gre_port
 * Returns:
 *   BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_ingress_dvp_set(int unit, int vp, uint32 mpath_flag,
                                    int vp_nh_ecmp_index, bcm_l2gre_port_t *l2gre_port)
{
    ing_dvp_table_entry_t dvp;
    int rv = BCM_E_NONE;

         if (l2gre_port->flags & BCM_L2GRE_PORT_REPLACE) {
            BCM_IF_ERROR_RETURN(
                READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
         } else if (l2gre_port->flags & BCM_L2GRE_PORT_WITH_ID ) {
            sal_memset(&dvp, 0, sizeof(ing_dvp_table_entry_t));
         } else {
            sal_memset(&dvp, 0, sizeof(ing_dvp_table_entry_t));
         }

         if (mpath_flag) {
              soc_ING_DVP_TABLEm_field32_set(unit, &dvp, 
                                       NEXT_HOP_INDEXf, 0x0);
              soc_ING_DVP_TABLEm_field32_set(unit, &dvp, 
                                       ECMPf, 0x1);
              soc_ING_DVP_TABLEm_field32_set(unit, &dvp, 
                                       ECMP_PTRf, vp_nh_ecmp_index);
         } else {
              soc_ING_DVP_TABLEm_field32_set(unit, &dvp, 
                                       ECMPf, 0x0);
              soc_ING_DVP_TABLEm_field32_set(unit, &dvp, 
                                       ECMP_PTRf, 0);
              soc_ING_DVP_TABLEm_field32_set(unit, &dvp, 
                                       NEXT_HOP_INDEXf, vp_nh_ecmp_index);
         }

        /* Enable DVP as Network_port */
        if (l2gre_port->flags & BCM_L2GRE_PORT_EGRESS_TUNNEL) {
             soc_ING_DVP_TABLEm_field32_set(unit, &dvp, NETWORK_PORTf, 0x1);
             soc_ING_DVP_TABLEm_field32_set(unit, &dvp, 
                           VP_TYPEf, _BCM_L2GRE_INGRESS_DEST_VP_TYPE);
        } else {
             soc_ING_DVP_TABLEm_field32_set(unit, &dvp, NETWORK_PORTf, 0x0);
             soc_ING_DVP_TABLEm_field32_set(unit, &dvp, 
                           VP_TYPEf, _BCM_L2GRE_DEST_VP_TYPE_ACCESS);
        }

        rv =  WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp, &dvp);   
        return rv;
}


/*
 * Function:
 *     _bcm_tr3_l2gre_ingress_dvp_reset
 * Purpose:
 *     Reset Ingress DVP tables
 * Parameters:
 *   IN :  Unit
 *   IN :  vp
 * Returns:
 *   BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_ingress_dvp_reset(int unit, int vp)
{
    int rv=BCM_E_UNAVAIL;
    ing_dvp_table_entry_t dvp;
  
    sal_memset(&dvp, 0, sizeof(ing_dvp_table_entry_t));
    rv =  WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp, &dvp);   
    return rv;
}

/*
 * Function:
 *     _bcm_tr3_l2gre_egress_dvp_set
 * Purpose:
 *     Set Egress DVP tables
 * Parameters:
 *   IN :  Unit
 *   IN :  vp
 *   IN :  l2gre_port
 * Returns:
 *   BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_egress_dvp_set(int unit, int vp, int drop, bcm_l2gre_port_t *l2gre_port)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int rv=BCM_E_UNAVAIL;
    egr_dvp_attribute_entry_t  egr_dvp_attribute;
    egr_dvp_attribute_1_entry_t  egr_dvp_attribute_1;
    bcm_ip_t dip=0;                       /* DIP for tunnel header */
    bcm_ip_t sip=0;                       /* SIP for tunnel header */
    int tunnel_index=-1, idx=0, num_tnl=0;


    l2gre_info = L2GRE_INFO(unit);
    num_tnl = soc_mem_index_count(unit, EGR_IP_TUNNELm);

    tunnel_index = BCM_GPORT_TUNNEL_ID_GET(l2gre_port->egress_tunnel_id);
    if (tunnel_index < num_tnl) {
        /* Obtain Tunnel DIP */
        dip = l2gre_info->l2gre_tunnel_init[tunnel_index].dip; 
    } else {
        /* Obtain Tunnel SIP */
        sip = l2gre_info->l2gre_tunnel_init[tunnel_index].sip; 
        /* Obtain Tunnel DIP */
        dip = l2gre_info->l2gre_tunnel_init[tunnel_index].dip; 
        /* Restrict tunnel_index to Hardware Range */
        for (idx=0; idx < num_tnl; idx++) {
            if (l2gre_info->l2gre_tunnel_init[idx].sip == sip) {
                tunnel_index = idx; /* Entry_idx for SIP within EGR_IP_TUNNEL */
                break;
            }
        }
    }
   
    /* Zero write buffer. */
    sal_memset(&egr_dvp_attribute, 0, sizeof(egr_dvp_attribute_entry_t));
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            VP_TYPEf, _BCM_L2GRE_EGRESS_DEST_VP_TYPE);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            L2GRE__TUNNEL_INDEXf, tunnel_index);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            L2GRE__DIPf, dip);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            L2GRE__DVP_IS_NETWORK_PORTf, 0x1);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            L2GRE__DISABLE_VP_PRUNINGf, 0x0);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            L2GRE__DELETE_VNTAGf, 0x1);

    rv = soc_mem_write(unit, EGR_DVP_ATTRIBUTEm,
                                            MEM_BLOCK_ALL, vp, &egr_dvp_attribute);
    BCM_IF_ERROR_RETURN(rv);

    /* Zero write buffer. */
    sal_memset(&egr_dvp_attribute_1, 0, sizeof(egr_dvp_attribute_1_entry_t));
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            L2GRE__CLASS_IDf, l2gre_port->if_class);
    /* L2 MTU - This is different from L3 MTU */
    if (l2gre_port->mtu > 0) {
        soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                             L2GRE__MTU_VALUEf, l2gre_port->mtu);
        soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                             L2GRE__MTU_ENABLEf, 0x1);
    }

    if (l2gre_port->flags & BCM_L2GRE_PORT_MULTICAST) {
        soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            L2GRE__UUC_DROPf, drop ? 1 : 0);
        soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            L2GRE__UMC_DROPf, drop ? 1 : 0);
        soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            L2GRE__BC_DROPf, drop ? 1 : 0);
    }
    rv = soc_mem_write(unit, EGR_DVP_ATTRIBUTE_1m,
                                            MEM_BLOCK_ALL, vp, &egr_dvp_attribute_1);
    return rv;
}


/*
 * Function:
 *     _bcm_tr3_l2gre_egress_dvp_get
 * Purpose:
 *     Get Egress DVP tables
 * Parameters:
 *   IN :  Unit
 *   IN :  vp
 *   IN :  l2gre_port
 * Returns:
 *   BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_egress_dvp_get(int unit, int vp, bcm_l2gre_port_t *l2gre_port)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info;
    int rv=BCM_E_NONE;
    egr_dvp_attribute_entry_t  egr_dvp_attribute;
    egr_dvp_attribute_1_entry_t  egr_dvp_attribute_1;
    int tunnel_index=-1;
    bcm_ip_t dip=0;                       /* DIP for tunnel header */
    int idx=0, num_vp=0;

    l2gre_info = L2GRE_INFO(unit);
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    sal_memset(&egr_dvp_attribute, 0, sizeof(egr_dvp_attribute_entry_t));
    rv = soc_mem_read(unit, EGR_DVP_ATTRIBUTEm,
                                            MEM_BLOCK_ANY, vp, &egr_dvp_attribute);
    BCM_IF_ERROR_RETURN(rv);
    dip = soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTEm, 
                                            &egr_dvp_attribute, L2GRE__DIPf);

    /* Find DIP within Softare State */
    for (idx=0; idx < num_vp; idx++) {
        if (l2gre_info->l2gre_tunnel_init[idx].dip == dip) {
            tunnel_index = idx;
            break;
        }
    }

    BCM_GPORT_TUNNEL_ID_SET(l2gre_port->egress_tunnel_id, tunnel_index);

    sal_memset(&egr_dvp_attribute_1, 0, sizeof(egr_dvp_attribute_1_entry_t));
    rv = soc_mem_read(unit, EGR_DVP_ATTRIBUTE_1m,
                                            MEM_BLOCK_ANY, vp, &egr_dvp_attribute_1);
    BCM_IF_ERROR_RETURN(rv);

    l2gre_port->if_class = soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTE_1m, 
                                            &egr_dvp_attribute_1, L2GRE__CLASS_IDf);
    if (soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTE_1m, 
                                            &egr_dvp_attribute_1, L2GRE__MTU_ENABLEf)) {
         l2gre_port->mtu = soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTE_1m, 
                                            &egr_dvp_attribute_1, L2GRE__MTU_VALUEf);
    }

    return rv;
}

/*
 * Function:
 *		_bcm_tr3_l2gre_egress_dvp_reset
 * Purpose:
 *		Reet Egress DVP tables
 * Parameters:
 *		IN :  Unit
 *           IN :  vp
 * Returns:
 *		BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_egress_dvp_reset(int unit, int vp)
{
    int rv=BCM_E_UNAVAIL;
    egr_dvp_attribute_entry_t  egr_dvp_attribute;
    egr_dvp_attribute_1_entry_t  egr_dvp_attribute_1;

    sal_memset(&egr_dvp_attribute, 0, sizeof(egr_dvp_attribute_entry_t));
    rv = soc_mem_write(unit, EGR_DVP_ATTRIBUTEm,
                                            MEM_BLOCK_ALL, vp, &egr_dvp_attribute);
    BCM_IF_ERROR_RETURN(rv);

    sal_memset(&egr_dvp_attribute_1, 0, sizeof(egr_dvp_attribute_1_entry_t));
    rv = soc_mem_write(unit, EGR_DVP_ATTRIBUTE_1m,
                                            MEM_BLOCK_ALL, vp, &egr_dvp_attribute_1);
    return rv;
}

/*
 * Function:
 *		_bcm_tr3_l2gre_learn_entry_key_set
 * Purpose:
 *		Set L2GRE egr_xlate key
 * Parameters:
 *		IN :  Unit
 *  IN :  l2gre_port
 *  IN : vpn
 *  IN : VLAN_XLATE entry pointer
 * Returns:
 *		BCM_E_XXX
 */

STATIC void
 _bcm_tr3_l2gre_egr_xlate_entry_key_set(int unit, bcm_vpn_t vpn, 
                                  egr_vlan_xlate_entry_t   *tr_ent, int clean_flag)
 {
    int vfi;

    if (clean_flag) {
         sal_memset(tr_ent, 0, sizeof(egr_vlan_xlate_entry_t));
    }
    _BCM_L2GRE_VPN_GET(vfi, _BCM_L2GRE_VPN_TYPE_ELAN,  vpn);
    if (SOC_IS_TRIUMPH3(unit)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, tr_ent, KEY_TYPEf, _BCM_TR3_L2GRE_KEY_TYPE_LOOKUP_VFI);
    } else  if (SOC_IS_TRIDENT2(unit)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, tr_ent, ENTRY_TYPEf, _BCM_TD2_L2GRE_KEY_TYPE_LOOKUP_VFI);
    }
    soc_EGR_VLAN_XLATEm_field32_set(unit, tr_ent, VALIDf, 0x1);
    soc_EGR_VLAN_XLATEm_field32_set(unit, tr_ent, L2GRE_VFI__VFIf, vfi);
 }

/*
 * Function:
 *		_bcm_td_l2gre_learn_entry_set
 * Purpose:
 *		Set L2GRE  egr_xlate Entry 
 * Parameters:
 *		IN :  Unit
 *  IN :  vlan_xlate_entry
 * Returns:
 *		BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_egr_xlate_entry_set(int unit, egr_vlan_xlate_entry_t  *key_ent)
{
    egr_vlan_xlate_entry_t  return_ent;
    int rv = BCM_E_UNAVAIL;
    int index;

    rv = soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                        key_ent, &return_ent, 0);

    if (rv == SOC_E_NONE) {
         rv = soc_mem_write(unit, EGR_VLAN_XLATEm,
                                           MEM_BLOCK_ALL, index,
                                           key_ent);
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    } else {
         rv = soc_mem_insert(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, key_ent);
    }

    return rv;
}

/*
 * Function:
 *    _bcm_tr3_l2gre_egr_xlate_entry_reset
 * Purpose:
 *   Reset L2GRE  egr_xlate Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  vpn
 * Returns:
 *    BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_egr_xlate_entry_reset(int unit, bcm_vpn_t vpn)
{
    int rv = BCM_E_UNAVAIL;
    int index;
    egr_vlan_xlate_entry_t  key_ent;
    int vfi;

    sal_memset(&key_ent, 0, sizeof(mpls_entry_entry_t));

    _BCM_L2GRE_VPN_GET(vfi, _BCM_L2GRE_VPN_TYPE_ELAN,  vpn);

    if (SOC_IS_TRIUMPH3(unit)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &key_ent, KEY_TYPEf, _BCM_TR3_L2GRE_KEY_TYPE_LOOKUP_VFI);
    } else  if (SOC_IS_TRIDENT2(unit)) {
        soc_EGR_VLAN_XLATEm_field32_set(unit, &key_ent, ENTRY_TYPEf, _BCM_TD2_L2GRE_KEY_TYPE_LOOKUP_VFI);
    }
    soc_EGR_VLAN_XLATEm_field32_set(unit, &key_ent, VALIDf, 0x1);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &key_ent, L2GRE_VFI__VFIf, vfi);

    rv = soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                             &key_ent, &key_ent, 0);
    if ( (rv != BCM_E_NOT_FOUND) && (rv != BCM_E_NONE) ) {
         return rv;
    }
    /* Delete the entry from HW */
    rv = soc_mem_delete(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &key_ent);
    
    if ( (rv != BCM_E_NOT_FOUND) && (rv != BCM_E_NONE) ) {
       return rv;
    } else {
       return BCM_E_NONE;
    }
}


/*
 * Function:
 *      bcm_tr3_l2gre_port_untagged_profile_set
 * Purpose:
 *      Set  VLan profile per Physical port entry
 * Parameters:
 *      unit   - (IN) SOC unit #
 *      port  - (IN) Physical port
 * Returns:
 *      BCM_E_XXX
 */     

int
bcm_tr3_l2gre_port_untagged_profile_set (int unit, bcm_port_t port)
{
    port_tab_entry_t ptab;
    int rv;
    bcm_vlan_action_set_t action;
    uint32 ing_profile_idx = 0xffffffff;

    bcm_vlan_action_set_t_init(&action);
    action.ut_outer = 0x0; /* No Op */
    action.ut_inner = 0x0; /* No Op */

    SOC_IF_ERROR_RETURN(soc_mem_read(unit, PORT_TABm,
                                     MEM_BLOCK_ANY, port, &ptab));

    rv = _bcm_trx_vlan_action_profile_entry_add(unit, &action, &ing_profile_idx);
    if (rv < 0) {
        return rv;
    }

    soc_PORT_TABm_field32_set(unit, &ptab, TAG_ACTION_PROFILE_PTRf,
                              ing_profile_idx);

    rv = soc_mem_write(unit, PORT_TABm,
                       MEM_BLOCK_ALL, port, &ptab);

    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_port_untagged_profile_reset
 * Purpose:
 *      Reset  VLan profile per Physical port entry
 * Parameters:
 *      unit   - (IN) SOC unit #
 *      port  - (IN) Physical port
 * Returns:
 *      BCM_E_XXX
 */     

int
bcm_tr3_l2gre_port_untagged_profile_reset(int unit, bcm_port_t port)
{
    int rv;
    port_tab_entry_t ptab;
    uint32 ing_profile_idx = 0xffffffff;

    SOC_IF_ERROR_RETURN(soc_mem_read(unit, PORT_TABm,
                                     MEM_BLOCK_ANY, port, &ptab));

    ing_profile_idx = 
            soc_PORT_TABm_field32_get(unit, &ptab, TAG_ACTION_PROFILE_PTRf);

    rv = _bcm_trx_vlan_action_profile_entry_delete(unit, ing_profile_idx);
    if (rv < 0) {
        return rv;
    }

    soc_PORT_TABm_field32_set(unit, &ptab, TAG_ACTION_PROFILE_PTRf, 0);

    rv = soc_mem_write(unit, PORT_TABm,
                                  MEM_BLOCK_ALL, port, &ptab);

    return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_sd_tag_set
 * Purpose:
 *      Program  SD_TAG settings
 * Parameters:
 *      unit           - (IN)  SOC unit #
 *      l2gre_port  - (IN)  L2GRE VP
 *      egr_nh_info  (IN/OUT) Egress NHop Info
 * Returns:
 *      BCM_E_XXX
 */     

STATIC int
_bcm_tr3_l2gre_sd_tag_set( int unit, bcm_l2gre_vpn_config_t *l2gre_vpn_info, 
                         bcm_l2gre_port_t *l2gre_port, 
                         _bcm_tr3_l2gre_nh_info_t  *egr_nh_info, 
                         egr_vlan_xlate_entry_t   *vxlate_entry,
                         int *tpid_index )
{
   soc_mem_t  hw_mem;

    if (l2gre_vpn_info != NULL ) {
        hw_mem = EGR_VLAN_XLATEm;
        if (l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_VLAN_ADD) {
            if (l2gre_vpn_info->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
            }
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_VIDf,
                                l2gre_vpn_info->egress_service_vlan);
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_NOT_PRESENTf,
                                0x1); /* ADD */
        }

        if (l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_VLAN_TPID_REPLACE) {
            if (l2gre_vpn_info->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
            }
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_VIDf,
                                l2gre_vpn_info->egress_service_vlan);
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x1); /* REPLACE_VID_TPID */
        } else if (l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_VLAN_REPLACE) {
            if (l2gre_vpn_info->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
            }
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_VIDf,
                                l2gre_vpn_info->egress_service_vlan);
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x2); /* REPLACE_VID_ONLY */
        } else if (l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_VLAN_DELETE) {
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x3); /* DELETE */
        } else if (l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_VLAN_PRI_TPID_REPLACE) {
               if (l2gre_vpn_info->egress_service_vlan >= BCM_VLAN_INVALID) {
                   return  BCM_E_PARAM;
               }
               soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_VIDf,
                                l2gre_vpn_info->egress_service_vlan);
               if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_PRIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_PRIf,
                                l2gre_vpn_info->pkt_pri);
               }
               if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_CFIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_CFIf,
                                l2gre_vpn_info->pkt_cfi);
               }

               soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x4); /* REPLACE_VID_PRI_TPID */

        } else if (l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_VLAN_PRI_REPLACE ){
                if (l2gre_vpn_info->egress_service_vlan >= BCM_VLAN_INVALID) {
                    return  BCM_E_PARAM;
                }
                soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_VIDf,
                                l2gre_vpn_info->egress_service_vlan);

                if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_PRIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_PRIf,
                                l2gre_vpn_info->pkt_pri);
                }
                if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_CFIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_CFIf,
                                l2gre_vpn_info->pkt_cfi);
                }

                soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x5); /* REPLACE_VID_PRI */
        }else if (l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_PRI_REPLACE ) {
                if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_PRIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_PRIf,
                                l2gre_vpn_info->pkt_pri);
                }
                if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_CFIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_CFIf,
                                l2gre_vpn_info->pkt_cfi);
                }

                soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x6); /* REPLACE_PRI_ONLY */
      
        } else if (l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_TPID_REPLACE ) {
                 soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x7); /* REPLACE_TPID_ONLY */
        }

        if ((l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_VLAN_ADD) ||
            (l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_VLAN_TPID_REPLACE) ||
            (l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_TPID_REPLACE) ||
            (l2gre_vpn_info->flags & BCM_L2GRE_VPN_SERVICE_VLAN_PRI_TPID_REPLACE)) {
            /* TPID value is used */
            BCM_IF_ERROR_RETURN(
                _bcm_fb2_outer_tpid_entry_add(unit, l2gre_vpn_info->egress_service_tpid, tpid_index));
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_TPID_INDEXf, *tpid_index);
        }
    } else  {

        if (l2gre_port->flags & BCM_L2GRE_PORT_EGRESS_TUNNEL) {
           hw_mem = EGR_VLAN_XLATEm;
           if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_ADD) {
              if (l2gre_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
              }
              soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_VIDf,
                                l2gre_port->egress_service_vlan);
              soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_NOT_PRESENTf,
                                0x1); /* ADD */
           }

           if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_TPID_REPLACE) {
              if (l2gre_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
              }
              soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_VIDf,
                                l2gre_port->egress_service_vlan);
              soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x1); /* REPLACE_VID_TPID */
           } else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_REPLACE) {
              if (l2gre_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
              }
              soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_VIDf,
                                l2gre_port->egress_service_vlan);
              soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x2); /* REPLACE_VID_ONLY */
           } else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_DELETE) {
              soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x3); /* DELETE */
           } else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_PRI_TPID_REPLACE) {
               if (l2gre_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                   return  BCM_E_PARAM;
               }
               soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_VIDf,
                                l2gre_port->egress_service_vlan);
               if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_PRIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_PRIf,
                                l2gre_port->pkt_pri);
               }
               if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_CFIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_CFIf,
                                l2gre_port->pkt_cfi);
               }

               soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x4); /* REPLACE_VID_PRI_TPID */

           } else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_PRI_REPLACE ) {
                if (l2gre_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                    return  BCM_E_PARAM;
                }
                soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_VIDf,
                                l2gre_port->egress_service_vlan);

                if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_PRIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_PRIf,
                                l2gre_port->pkt_pri);
                }
                if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_CFIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_CFIf,
                                l2gre_port->pkt_cfi);
                }

                soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x5); /* REPLACE_VID_PRI */
           }else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_PRI_REPLACE ) {
                if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_PRIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_PRIf,
                                l2gre_port->pkt_pri);
                }
                if (soc_mem_field_valid(unit, hw_mem,
                                       L2GRE_VFI__SD_TAG_NEW_CFIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_NEW_CFIf,
                                l2gre_port->pkt_cfi);
                }

                soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x6); /* REPLACE_PRI_ONLY */
      
           } else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_TPID_REPLACE ) {
                 soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x7); /* REPLACE_TPID_ONLY */
           }

           if ((l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_ADD) ||
               (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_TPID_REPLACE) ||
               (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_TPID_REPLACE) ||
               (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_PRI_TPID_REPLACE)) {
               /* TPID value is used */
               BCM_IF_ERROR_RETURN(
                _bcm_fb2_outer_tpid_entry_add(unit, l2gre_port->egress_service_tpid, tpid_index));
               soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, L2GRE_VFI__SD_TAG_TPID_INDEXf, *tpid_index);
           }
        } else {

           if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_ADD) {
              if (l2gre_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
              }
              egr_nh_info->sd_tag_vlan = l2gre_port->egress_service_vlan; 
              egr_nh_info->sd_tag_action_not_present = 0x1; /* ADD */
           }

           if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_TPID_REPLACE) {
              if (l2gre_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
              }
              /* REPLACE_VID_TPID */
              egr_nh_info->sd_tag_vlan = l2gre_port->egress_service_vlan; 
              egr_nh_info->sd_tag_action_present = 0x1;
           } else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_REPLACE) {

              if (l2gre_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
              }
               /* REPLACE_VID_ONLY */
              egr_nh_info->sd_tag_vlan = l2gre_port->egress_service_vlan; 
              egr_nh_info->sd_tag_action_present = 0x2;
           } else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_DELETE) {
              egr_nh_info->sd_tag_action_present = 0x3; /* DELETE */
              egr_nh_info->sd_tag_action_not_present = 0;
           } else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_PRI_TPID_REPLACE) {
                if (l2gre_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                    return  BCM_E_PARAM;
                }

                /* REPLACE_VID_PRI_TPID */
                egr_nh_info->sd_tag_vlan = l2gre_port->egress_service_vlan;
                egr_nh_info->sd_tag_pri = l2gre_port->pkt_pri;
                egr_nh_info->sd_tag_cfi = l2gre_port->pkt_cfi;
                egr_nh_info->sd_tag_action_present = 0x4;
           } else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_PRI_REPLACE ) {
                if (l2gre_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                    return  BCM_E_PARAM;
                }

                /* REPLACE_VID_PRI_ONLY */
                egr_nh_info->sd_tag_vlan = l2gre_port->egress_service_vlan;
                egr_nh_info->sd_tag_pri = l2gre_port->pkt_pri;
                egr_nh_info->sd_tag_cfi = l2gre_port->pkt_cfi;
                egr_nh_info->sd_tag_action_present = 0x5;
           } else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_PRI_REPLACE ) {
                 /* REPLACE_PRI_ONLY */
                egr_nh_info->sd_tag_pri = l2gre_port->pkt_pri;
                egr_nh_info->sd_tag_cfi = l2gre_port->pkt_cfi;
                egr_nh_info->sd_tag_action_present = 0x6;
           } else if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_TPID_REPLACE ) {
                 /* REPLACE_TPID_ONLY */
                egr_nh_info->sd_tag_action_present = 0x7;
           }

           if ((l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_ADD) ||
               (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_TPID_REPLACE) ||
               (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_TPID_REPLACE) ||
               (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_VLAN_PRI_TPID_REPLACE)) {
               /* TPID value is used */
               BCM_IF_ERROR_RETURN(_bcm_fb2_outer_tpid_entry_add(unit, l2gre_port->egress_service_tpid, tpid_index));
               egr_nh_info->tpid_index = *tpid_index;
           }
        }
    }
    return  BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_sd_tag_get
 * Purpose:
 *      Get  SD_TAG settings
 * Parameters:
 *      unit           - (IN)  SOC unit #
 *      l2gre_port  - (IN)  L2GRE VP
 *      egr_nh_info  (IN/OUT) Egress NHop Info
 * Returns:
 *      BCM_E_XXX
 */     

STATIC void
_bcm_tr3_l2gre_sd_tag_get( int unit, bcm_l2gre_vpn_config_t *l2gre_vpn_info, 
                         bcm_l2gre_port_t *l2gre_port, 
                         egr_l3_next_hop_entry_t *egr_nh, 
                         egr_vlan_xlate_entry_t   *vxlate_entry,
                         int network_port_flag )
{
    int action_present, action_not_present;

    if (network_port_flag) {
       action_present = 
            soc_EGR_VLAN_XLATEm_field32_get(unit, 
                                     vxlate_entry,
                                     L2GRE_VFI__SD_TAG_ACTION_IF_PRESENTf);
       action_not_present = 
            soc_EGR_VLAN_XLATEm_field32_get(unit, 
                                     vxlate_entry,
                                     L2GRE_VFI__SD_TAG_ACTION_IF_NOT_PRESENTf);

       switch (action_present) {
         case 0:
            if (action_not_present == 1) {
                l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_ADD;
            }
            l2gre_vpn_info->egress_service_vlan =
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, L2GRE_VFI__SD_TAG_VIDf);
            break;

         case 1:
            if (action_not_present == 1) {
                l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_ADD;
            }
            l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_TPID_REPLACE;
            l2gre_vpn_info->egress_service_vlan =
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, L2GRE_VFI__SD_TAG_VIDf);
            break;

         case 2:
            if (action_not_present == 1) {
                l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_ADD;
            }
            l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_REPLACE;
            l2gre_vpn_info->egress_service_vlan =
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, L2GRE_VFI__SD_TAG_VIDf);
            break;

         case 3:
            if (action_not_present == 1) {
                l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_ADD;
            }
            l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_DELETE;
            l2gre_vpn_info->egress_service_vlan = BCM_VLAN_INVALID;
            break;

         case 4:
            if (action_not_present == 1) {
                l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_ADD;
            }
            l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_PRI_TPID_REPLACE;
            l2gre_vpn_info->egress_service_vlan =                 
                 soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, L2GRE_VFI__SD_TAG_VIDf);
            l2gre_vpn_info->pkt_pri = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, L2GRE_VFI__SD_TAG_NEW_PRIf);
            l2gre_vpn_info->pkt_cfi = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, L2GRE_VFI__SD_TAG_NEW_CFIf);
            break;

         case 5:
            if (action_not_present == 1) {
                l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_ADD;
            }
            l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_PRI_REPLACE;
            l2gre_vpn_info->egress_service_vlan = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, L2GRE_VFI__SD_TAG_VIDf);
            l2gre_vpn_info->pkt_pri = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, L2GRE_VFI__SD_TAG_NEW_PRIf);
            l2gre_vpn_info->pkt_cfi = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, L2GRE_VFI__SD_TAG_NEW_CFIf);
            break;

         case 6:
            if (action_not_present == 1) {
                l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_ADD;
            }
            l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_PRI_REPLACE;
            l2gre_vpn_info->egress_service_vlan =   BCM_VLAN_INVALID;
            l2gre_vpn_info->pkt_pri = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, L2GRE_VFI__SD_TAG_NEW_PRIf);
            l2gre_vpn_info->pkt_cfi = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, L2GRE_VFI__SD_TAG_NEW_CFIf);
            break;

         case 7:
            if (action_not_present == 1) {
                l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_VLAN_ADD;
            }
            l2gre_vpn_info->flags |= BCM_L2GRE_VPN_SERVICE_TPID_REPLACE;
            l2gre_vpn_info->egress_service_vlan = BCM_VLAN_INVALID;
            break;

         default:
            break;
       }
    }else {
       action_present = 
            soc_EGR_L3_NEXT_HOPm_field32_get(unit, 
                                     egr_nh,
                                     SD_TAG__SD_TAG_ACTION_IF_PRESENTf);
       action_not_present = 
            soc_EGR_L3_NEXT_HOPm_field32_get(unit, 
                                     egr_nh,
                                     SD_TAG__SD_TAG_ACTION_IF_NOT_PRESENTf);

       switch (action_present) {
         case 0:
            if (action_not_present == 1) {
                l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_ADD;
            }
            l2gre_port->egress_service_vlan =
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__SD_TAG_VIDf);
            break;

         case 1:
            if (action_not_present == 1) {
                l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_ADD;
            }
            l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_TPID_REPLACE;
            l2gre_port->egress_service_vlan =
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__SD_TAG_VIDf);
            break;

         case 2:
            if (action_not_present == 1) {
                l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_ADD;
            }
            l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_REPLACE;
            l2gre_port->egress_service_vlan =
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__SD_TAG_VIDf);
            break;

         case 3:
            if (action_not_present == 1) {
                l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_ADD;
            }
            l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_DELETE;
            l2gre_port->egress_service_vlan = BCM_VLAN_INVALID;
            break;

         case 4:
            if (action_not_present == 1) {
                l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_ADD;
            }
            l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_PRI_TPID_REPLACE;
            l2gre_port->egress_service_vlan =                 
                 soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__SD_TAG_VIDf);
            l2gre_port->pkt_pri = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_PRIf);
            l2gre_port->pkt_cfi = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_CFIf);
            break;

         case 5:
            if (action_not_present == 1) {
                l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_ADD;
            }
            l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_PRI_REPLACE;
            l2gre_port->egress_service_vlan = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__SD_TAG_VIDf);
            l2gre_port->pkt_pri = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_PRIf);
            l2gre_port->pkt_cfi = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_CFIf);
            break;

         case 6:
            if (action_not_present == 1) {
                l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_ADD;
            }
            l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_PRI_REPLACE;
            l2gre_port->egress_service_vlan =   BCM_VLAN_INVALID;
            l2gre_port->pkt_pri = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_PRIf);
            l2gre_port->pkt_cfi = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_CFIf);
            break;

         case 7:
            if (action_not_present == 1) {
                l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_VLAN_ADD;
            }
            l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_TPID_REPLACE;
            l2gre_port->egress_service_vlan = BCM_VLAN_INVALID;
            break;

         default:
            break;
       }
    }
}

/*
 * Function:
 *      _bcm_tr3_l2gre_nexthop_entry_modify
 * Purpose:
 *      Modify Egress entry
 * Parameters:
 *      unit - Device Number
 *      nh_index - Next Hop Index
 *      new_entry_type - New Entry type
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_nexthop_entry_modify(int unit, int nh_index, int drop, 
              _bcm_tr3_l2gre_nh_info_t  *egr_nh_info, int new_entry_type)
{
    egr_l3_next_hop_entry_t egr_nh;
    int rv = BCM_E_NONE;
    uint32 old_entry_type=0, intf_num=0, vntag_actions=0;
    uint32 vntag_dst_vif=0, vntag_pf=0, vntag_force_lf=0;
    uint32 etag_pcp_de_sourcef=0, etag_pcpf=0, etag_dot1p_mapping_ptr=0;
    bcm_mac_t mac_addr;                 /* Next hop forwarding destination mac. */

    sal_memset(&mac_addr, 0, sizeof (mac_addr));
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ALL,
                      nh_index, &egr_nh));

    old_entry_type = soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh, ENTRY_TYPEf);

    if ((new_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_SDTAG_VIEW) &&
         (old_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_L3_VIEW)) {
            if (egr_nh_info->sd_tag_vlan != -1) {
                  soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm,
                                &egr_nh, SD_TAG__SD_TAG_VIDf, egr_nh_info->sd_tag_vlan);
            }

            if (egr_nh_info->sd_tag_action_present != -1) {
                   soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                             SD_TAG__SD_TAG_ACTION_IF_PRESENTf,
                             egr_nh_info->sd_tag_action_present);
            }

            if (egr_nh_info->sd_tag_action_not_present != -1) {
                   soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                             SD_TAG__SD_TAG_ACTION_IF_NOT_PRESENTf,
                             egr_nh_info->sd_tag_action_not_present);
            }

            if (egr_nh_info->sd_tag_pri != -1) {
                 if(soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm, SD_TAG__NEW_PRIf)) {
                       soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                             SD_TAG__NEW_PRIf,
                             egr_nh_info->sd_tag_pri);
                 }
            }

            if (egr_nh_info->sd_tag_cfi != -1) {
                   if(soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm, SD_TAG__NEW_CFIf)) {
                       soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                             SD_TAG__NEW_CFIf,
                             egr_nh_info->sd_tag_cfi);
                   }
            }

            if (egr_nh_info->tpid_index != -1) {
                  soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm,
                                &egr_nh, SD_TAG__SD_TAG_TPID_INDEXf,
                                egr_nh_info->tpid_index);
            }

            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                    SD_TAG__BC_DROPf, drop ? 1 : 0);
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                    SD_TAG__UUC_DROPf, drop ? 1 : 0);
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                    SD_TAG__UMC_DROPf, drop ? 1 : 0);
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                    SD_TAG__DVPf, egr_nh_info->dvp);
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm,
                    &egr_nh, SD_TAG__DVP_IS_NETWORK_PORTf,
                    egr_nh_info->dvp_is_network);
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                    SD_TAG__HG_LEARN_OVERRIDEf, egr_nh_info->is_eline ? 1 : 0);
            /* HG_MODIFY_ENABLE must be 0x0 for Ingress and Egress Chip */
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                    SD_TAG__HG_MODIFY_ENABLEf, 0x0);
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                    SD_TAG__HG_HDR_SELf, 1);

            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                            ENTRY_TYPEf, _BCM_L2GRE_EGR_NEXT_HOP_SDTAG_VIEW); /* SD_TAG view */
    } else if ((new_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_L3_VIEW) &&
            (old_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_SDTAG_VIEW)) {
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                            ENTRY_TYPEf, _BCM_L2GRE_EGR_NEXT_HOP_L3_VIEW); /* Normal view */
    } else if ((new_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_L3MC_VIEW) &&
         (old_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_L3_VIEW)) {

        /* Get mac address. */
        soc_mem_mac_addr_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3__MAC_ADDRESSf, mac_addr);

        /* Get int_num */
        intf_num = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3__INTF_NUMf);

        vntag_actions = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3__VNTAG_ACTIONSf);

        vntag_dst_vif = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3__VNTAG_DST_VIFf);

        vntag_pf = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__VNTAG_Pf);

        vntag_force_lf = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__VNTAG_FORCE_Lf);

        etag_pcp_de_sourcef = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__ETAG_PCP_DE_SOURCEf);

        etag_pcpf = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__ETAG_PCPf);

        etag_dot1p_mapping_ptr = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__ETAG_DOT1P_MAPPING_PTRf);

        soc_mem_mac_addr_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__MAC_ADDRESSf, mac_addr);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__INTF_NUMf, intf_num);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__DVP_VALIDf, 0x1);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__DVPf, egr_nh_info->dvp);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L2_MC_DA_DISABLEf, 0x1);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L2_MC_SA_DISABLEf, 0x1);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L2_MC_VLAN_DISABLEf, 0x1);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L2_DROPf, 0x0);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                                L3MC__L3_DROPf, drop ? 1 : 0);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh, 
                                L3MC__L3MC_USE_CONFIGURED_MACf, 0x1);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__VNTAG_ACTIONSf, vntag_actions);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__VNTAG_DST_VIFf, vntag_dst_vif);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__VNTAG_Pf, vntag_pf);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__VNTAG_FORCE_Lf, vntag_force_lf);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__ETAG_PCP_DE_SOURCEf, etag_pcp_de_sourcef);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__ETAG_PCPf, etag_pcpf);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__ETAG_DOT1P_MAPPING_PTRf, etag_dot1p_mapping_ptr);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                ENTRY_TYPEf, _BCM_L2GRE_EGR_NEXT_HOP_L3MC_VIEW);
    } else if ((new_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_L3MC_VIEW) &&
             (old_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_L3MC_VIEW)) {
        
        /* Get mac address. */
        soc_mem_mac_addr_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__MAC_ADDRESSf, mac_addr);
        
        /* Get int_num */
        intf_num = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__INTF_NUMf);
        
        vntag_actions = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__VNTAG_ACTIONSf);
        
        vntag_dst_vif = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__VNTAG_DST_VIFf);
        
        vntag_pf = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__VNTAG_Pf);
        
        vntag_force_lf = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__VNTAG_FORCE_Lf);
        
        etag_pcp_de_sourcef = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__ETAG_PCP_DE_SOURCEf);
        
        etag_pcpf = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__ETAG_PCPf);
        
        etag_dot1p_mapping_ptr = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__ETAG_DOT1P_MAPPING_PTRf);
        
        soc_mem_mac_addr_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3MC__MAC_ADDRESSf, mac_addr);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__INTF_NUMf, intf_num);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__DVP_VALIDf, 0x1);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__DVPf, egr_nh_info->dvp);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L2_MC_DA_DISABLEf, 0x1);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L2_MC_SA_DISABLEf, 0x1);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L2_MC_VLAN_DISABLEf, 0x1);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L2_DROPf, 0x0);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L3_DROPf, drop ? 1 : 0);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L3MC_USE_CONFIGURED_MACf, 0x1);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__VNTAG_ACTIONSf, vntag_actions);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__VNTAG_DST_VIFf, vntag_dst_vif);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__VNTAG_Pf, vntag_pf);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__VNTAG_FORCE_Lf, vntag_force_lf);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__ETAG_PCP_DE_SOURCEf, etag_pcp_de_sourcef);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__ETAG_PCPf, etag_pcpf);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__ETAG_DOT1P_MAPPING_PTRf, etag_dot1p_mapping_ptr);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                ENTRY_TYPEf, _BCM_L2GRE_EGR_NEXT_HOP_L3MC_VIEW);
    } else if ((new_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_L3_VIEW) &&
         (old_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_L3MC_VIEW)) {
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                            ENTRY_TYPEf, _BCM_L2GRE_EGR_NEXT_HOP_L3_VIEW);
    } else if ((new_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_SDTAG_VIEW) &&
        (old_entry_type == _BCM_L2GRE_EGR_NEXT_HOP_SDTAG_VIEW)) {

        /* Case of L2GRE Proxy Next-Hop */
        if (egr_nh_info->sd_tag_vlan != -1) {
              soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm,
                            &egr_nh, SD_TAG__SD_TAG_VIDf, egr_nh_info->sd_tag_vlan);
        }

        if (egr_nh_info->sd_tag_action_present != -1) {
               soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         SD_TAG__SD_TAG_ACTION_IF_PRESENTf,
                         egr_nh_info->sd_tag_action_present);
        }

        if (egr_nh_info->sd_tag_action_not_present != -1) {
               soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         SD_TAG__SD_TAG_ACTION_IF_NOT_PRESENTf,
                         egr_nh_info->sd_tag_action_not_present);
        }

        if (egr_nh_info->sd_tag_pri != -1) {
             if(soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm, SD_TAG__NEW_PRIf)) {
                   soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         SD_TAG__NEW_PRIf,
                         egr_nh_info->sd_tag_pri);
             }
        }

        if (egr_nh_info->sd_tag_cfi != -1) {
               if(soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm, SD_TAG__NEW_CFIf)) {
                   soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         SD_TAG__NEW_CFIf,
                         egr_nh_info->sd_tag_cfi);
               }
        }

        if (egr_nh_info->tpid_index != -1) {
              soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm,
                            &egr_nh, SD_TAG__SD_TAG_TPID_INDEXf,
                            egr_nh_info->tpid_index);
        }

        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                SD_TAG__DVPf, egr_nh_info->dvp);

        if (egr_nh_info->sd_tag_cfi != -1) {
              soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm,
                            &egr_nh, SD_TAG__DVP_IS_NETWORK_PORTf,
                            egr_nh_info->dvp_is_network);
        }


    } 

    rv = soc_mem_write(unit, EGR_L3_NEXT_HOPm,
                       MEM_BLOCK_ALL, nh_index, &egr_nh);
    return rv;
}

/*
 * Function:
 *           _bcm_tr3_l2gre_next_hop_set
 * Purpose:
 *           Set L2GRE NextHop entry
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
_bcm_tr3_l2gre_next_hop_set(int unit, int nh_index, uint32 flags)
{
    ing_l3_next_hop_entry_t ing_nh;
    int rv=BCM_E_NONE;
    bcm_port_t      port=0;
    bcm_module_t modid=0, local_modid=0;
    bcm_trunk_t tgid=0;
    int  num_local_ports=0, idx=-1;
    bcm_port_t   local_ports[SOC_MAX_NUM_PORTS];
    int  old_nh_index=-1;
    uint32 reg_val=0;
    int replace=0;

    replace = flags & BCM_L3_REPLACE;

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, ING_L3_NEXT_HOPm, 
                                  MEM_BLOCK_ANY, nh_index, &ing_nh));
    if (soc_mem_field32_get(unit, ING_L3_NEXT_HOPm, &ing_nh, Tf)) {
         tgid = soc_mem_field32_get(unit, ING_L3_NEXT_HOPm,
                                            &ing_nh, TGIDf);
         rv = _bcm_trunk_id_validate(unit, tgid);
         if (BCM_FAILURE(rv)) {
            return BCM_E_PORT;
         }
         BCM_IF_ERROR_RETURN(_bcm_esw_trunk_local_members_get(unit, tgid,
                SOC_MAX_NUM_PORTS, local_ports, &num_local_ports));
    } else {
        modid = soc_mem_field32_get(unit, ING_L3_NEXT_HOPm,
                                    &ing_nh, MODULE_IDf);
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get (unit, &local_modid));
        if (modid != local_modid) {
            /* Ignore EGR_PORT_TO_NHI_MAPPINGs */
            return BCM_E_NONE;
        }
         port = soc_mem_field32_get(unit, ING_L3_NEXT_HOPm,
                                            &ing_nh, PORT_NUMf);
        local_ports[num_local_ports++] = port;
    }

    if (!(flags & BCM_L3_IPMC)) { /* Only for L2GRE Unicast Nexthops */
       for (idx = 0; idx < num_local_ports; idx++) {
            BCM_IF_ERROR_RETURN(
                READ_EGR_PORT_TO_NHI_MAPPINGr(unit, local_ports[idx], &reg_val));
            old_nh_index = soc_reg_field_get(unit, EGR_PORT_TO_NHI_MAPPINGr, reg_val, NEXT_HOP_INDEXf);
            if (old_nh_index && !replace) {
                if (old_nh_index != nh_index) {
                    /* Limitation: For TD/TR3, 
                    Trill egress port can be configured with one NHI*/
                    return BCM_E_RESOURCE;    
                }
            } else {
                rv = soc_reg_field32_modify(unit, EGR_PORT_TO_NHI_MAPPINGr,
                                            local_ports[idx], NEXT_HOP_INDEXf, nh_index);
                if (BCM_FAILURE(rv)) {
                   return rv;
                }
            }
         }
    }
    return rv;
}

/*
 * Function:
 *           bcm_tr3_l2gre_nexthop_get
 * Purpose:
 *          Get L2GRE NextHop entry
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
bcm_tr3_l2gre_nexthop_get(int unit, int nh_index, bcm_l2gre_port_t *l2gre_port)
{
    soc_mem_t      hw_mem;
    egr_l3_next_hop_entry_t egr_nh;

    hw_mem = EGR_L3_NEXT_HOPm;
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, hw_mem, 
                                  MEM_BLOCK_ANY, nh_index, &egr_nh));
    if (0x1 == soc_mem_field32_get(unit, hw_mem, &egr_nh,
                                  L3__DVP_VALIDf)) {
         l2gre_port->flags |= BCM_L2GRE_PORT_MULTICAST;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *           bcm_tr3_l2gre_egr_nexthop_multicast_set
 * Purpose:
 *           Set L2GRE Egress NextHop Multicast
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

STATIC int
bcm_tr3_l2gre_egr_nexthop_multicast_set(int unit, int nh_index, int vp, bcm_l2gre_port_t *l2gre_port)
{
    soc_mem_t      hw_mem;
    egr_l3_next_hop_entry_t egr_nh;

    if (l2gre_port->flags & BCM_L2GRE_PORT_MULTICAST) {
        hw_mem = EGR_L3_NEXT_HOPm;
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, hw_mem, 
                                      MEM_BLOCK_ANY, nh_index, &egr_nh));
        soc_mem_field32_set(unit, hw_mem, &egr_nh,
                                  L3__DVP_VALIDf, 0x1);
        soc_mem_field32_set(unit, hw_mem, &egr_nh,
                                  L3__DVPf, vp);
        BCM_IF_ERROR_RETURN(soc_mem_write(unit, hw_mem,
                                  MEM_BLOCK_ALL, nh_index, &egr_nh));
    }
    return BCM_E_NONE;
}


/*
 * Function:
 *           bcm_tr3_l2gre_port_egress_nexthop_reset
 * Purpose:
 *           Reset L2GRE Egress NextHop 
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
bcm_tr3_l2gre_port_egress_nexthop_reset(int unit, int nh_index, int vp_type, int vp, bcm_vpn_t vpn)
{
    egr_l3_next_hop_entry_t egr_nh;
    int rv=BCM_E_NONE;
    int action_present=0, action_not_present=0, old_tpid_idx = -1;
    uint32  entry_type=0;

    if (vp_type== _BCM_L2GRE_DEST_VP_TYPE_ACCESS) {
        /* egressing into a regular port */
        entry_type = 
            soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh, ENTRY_TYPEf);
        if (entry_type != _BCM_L2GRE_EGR_NEXT_HOP_SDTAG_VIEW) {
            rv = BCM_E_PARAM;
        }

        action_present = 
             soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh,
                                         SD_TAG__SD_TAG_ACTION_IF_PRESENTf);
        action_not_present = 
             soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh,
                                         SD_TAG__SD_TAG_ACTION_IF_NOT_PRESENTf);
        if ((action_not_present == 0x1) || (action_present == 0x1)) {
            old_tpid_idx =
                  soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh,
                                         SD_TAG__SD_TAG_TPID_INDEXf);
        }

        /* Normalize Next-hop Entry -- L3 View */
        rv = _bcm_tr3_l2gre_nexthop_entry_modify(unit, nh_index, 0, 
                             NULL, _BCM_L2GRE_EGR_NEXT_HOP_L3_VIEW);

    }

    if (old_tpid_idx != -1) {
        (void) _bcm_fb2_outer_tpid_entry_delete(unit, old_tpid_idx);
    }
    return rv;
}

/*
 * Function:
 *		bcm_tr3_l2gre_nexthop_reset
 * Purpose:
 *		Reset L2GRE NextHop entry
 * Parameters:
 *		IN :  Unit
 *           IN :  nh_index
 * Returns:
 *		BCM_E_XXX
 */

STATIC int
bcm_tr3_l2gre_nexthop_reset(int unit, int nh_index)
{
    int rv=BCM_E_NONE;
    ing_l3_next_hop_entry_t ing_nh;
    bcm_port_t      port=0;
    bcm_trunk_t tgid=0;
    int  num_local_ports=0, idx=-1;
    bcm_port_t   local_ports[SOC_MAX_NUM_PORTS];
    soc_mem_t      hw_mem;
    bcm_module_t modid=0, local_modid=0;
    int  old_nh_index=-1;
    uint32 reg_val=0;


    hw_mem = ING_L3_NEXT_HOPm;
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, hw_mem, 
                                  MEM_BLOCK_ANY, nh_index, &ing_nh));
    soc_mem_field32_set(unit, hw_mem, &ing_nh,
                                            DROPf, 0x0);
    soc_mem_field32_set(unit, hw_mem, &ing_nh,
                                            ENTRY_TYPEf, 0x0);
    if (soc_mem_field_valid(unit, ING_L3_NEXT_HOPm, DVP_ATTRIBUTE_1_INDEXf) ) {
         soc_mem_field32_set(unit, hw_mem,
                        &ing_nh, DVP_ATTRIBUTE_1_INDEXf, 0);
    }
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, hw_mem,
                                            MEM_BLOCK_ALL, nh_index, &ing_nh));

    if (soc_mem_field32_get(unit, hw_mem, &ing_nh, Tf)) {
         tgid = soc_mem_field32_get(unit, hw_mem,
                                            &ing_nh, TGIDf);
         rv = _bcm_trunk_id_validate(unit, tgid);
         if (BCM_FAILURE(rv)) {
            return BCM_E_PORT;
         }
         BCM_IF_ERROR_RETURN(_bcm_esw_trunk_local_members_get(unit, tgid,
                SOC_MAX_NUM_PORTS, local_ports, &num_local_ports));
    } else {
        modid = soc_mem_field32_get(unit, ING_L3_NEXT_HOPm,
                                    &ing_nh, MODULE_IDf);
        BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get (unit, &local_modid));

        if (modid != local_modid) {
            /* Ignore reset for non local modid */
            return BCM_E_NONE;
        }
        port = soc_mem_field32_get(unit, hw_mem,
                                            &ing_nh, PORT_NUMf);
        local_ports[num_local_ports++] = port;
    }

     for (idx = 0; idx < num_local_ports; idx++) {
         BCM_IF_ERROR_RETURN(
         READ_EGR_PORT_TO_NHI_MAPPINGr(unit, local_ports[idx], &reg_val));
         old_nh_index = soc_reg_field_get(unit, EGR_PORT_TO_NHI_MAPPINGr, reg_val, NEXT_HOP_INDEXf);
         if (old_nh_index == nh_index) {
              rv = soc_reg_field32_modify(unit, EGR_PORT_TO_NHI_MAPPINGr,
                                        local_ports[idx], NEXT_HOP_INDEXf, 0x0);
              if (BCM_FAILURE(rv)) {
                  return rv;
              }
         }
     }

    return rv;
}


/*
 * Function:
 *           bcm_tr3_l2gre_egress_set
 * Purpose:
 *           Set [MTU] L2GRE Egress NextHop 
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
bcm_tr3_l2gre_egress_set(int unit, int nh_index, uint32 flags)
{
    ing_l3_next_hop_entry_t ing_nh;
    soc_mem_t      hw_mem;
    int rv=BCM_E_NONE;

    hw_mem = ING_L3_NEXT_HOPm;
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, hw_mem, 
        MEM_BLOCK_ANY, nh_index, &ing_nh));
    soc_mem_field32_set(unit, hw_mem, &ing_nh,
        ENTRY_TYPEf, _BCM_L2GRE_INGRESS_NEXT_HOP_ENTRY_TYPE); /* L2 DVP */
    if (SOC_MEM_FIELD_VALID(unit, hw_mem, MTU_SIZEf)) {
        soc_mem_field32_set(unit, hw_mem, &ing_nh, MTU_SIZEf, 0x3fff);
    }
    else if (soc_mem_field_valid(unit, ING_L3_NEXT_HOPm, 
                 DVP_ATTRIBUTE_1_INDEXf)) {
    /* Set default ING_L3_NEXT_HOP_ATTRIBUTE_1 MTU_SIZE 
       * Note: Assumption that first entry has MTU size of 0x3fff
       */
        soc_mem_field32_set(unit, hw_mem,
            &ing_nh, DVP_ATTRIBUTE_1_INDEXf, 0);
    }    
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, hw_mem,
                            MEM_BLOCK_ALL, nh_index, &ing_nh));
    
    rv = _bcm_tr3_l2gre_next_hop_set(unit, nh_index, flags);
    return rv;
}

/*
 * Function:
 *           bcm_tr3_l2gre_egress_reset
 * Purpose:
 *           Reset L2GRE Egress NextHop 
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
bcm_tr3_l2gre_egress_reset(int unit, int nh_index)
{
    ing_l3_next_hop_entry_t ing_nh;
    soc_mem_t      hw_mem;
    int rv=BCM_E_NONE;

    hw_mem = ING_L3_NEXT_HOPm;
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, hw_mem, 
                MEM_BLOCK_ANY, nh_index, &ing_nh));
    soc_mem_field32_set(unit, hw_mem, &ing_nh,
                ENTRY_TYPEf, 0x0);
    if (SOC_MEM_FIELD_VALID(unit, ING_L3_NEXT_HOPm, MTU_SIZEf)) {
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                &ing_nh, MTU_SIZEf, 0x0);
    }
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, hw_mem,
                                  MEM_BLOCK_ALL, nh_index, &ing_nh));

    rv = bcm_tr3_l2gre_nexthop_reset(unit, nh_index);
    return rv;
}


/*
 * Function:
 *           bcm_tr3_l2gre_egress_get
 * Purpose:
 *           Get L2GRE Egress NextHop 
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
bcm_tr3_l2gre_egress_get(int unit, bcm_l3_egress_t *egr, int nh_index)
{
    ing_l3_next_hop_entry_t ing_nh;

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, ING_L3_NEXT_HOPm, 
                                  MEM_BLOCK_ANY, nh_index, &ing_nh));

    if (0x2 == soc_mem_field32_get(unit, ING_L3_NEXT_HOPm, &ing_nh,
                                  ENTRY_TYPEf)) {
         egr->flags |= BCM_L3_L2GRE_ONLY;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_tr3_l2gre_port_nh_add
 * Purpose:
 *      Add L2GRE Next Hop entry
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 */     

STATIC int
_bcm_tr3_l2gre_port_nh_add(int unit, bcm_l2gre_port_t *l2gre_port, int vp,
                            bcm_vpn_t vpn, int drop)
{
    egr_l3_next_hop_entry_t egr_nh;
    egr_vlan_xlate_entry_t   vxlate_entry;
    _bcm_tr3_l2gre_nh_info_t egr_nh_info;
    int rv=BCM_E_NONE;
    int action_present, action_not_present, tpid_index = -1;
    int old_tpid_idx = -1;
    uint32 mpath_flag=0;
    int vp_nh_ecmp_index=-1;


    egr_nh_info.dvp = vp;
    egr_nh_info.entry_type = -1;
    egr_nh_info.dvp_is_network = -1;
    egr_nh_info.sd_tag_action_present = -1;
    egr_nh_info.sd_tag_action_not_present = -1;
    egr_nh_info.intf_num = -1;
    egr_nh_info.sd_tag_vlan = -1;
    egr_nh_info.sd_tag_pri = -1;
    egr_nh_info.sd_tag_cfi = -1;
    egr_nh_info.tpid_index = -1;
    egr_nh_info.is_eline = 0; /* Change to 1 for Eline */


    /* 
     * Get egress next-hop index from egress object and
     * increment egress object reference count. 
     */
    rv = bcm_xgs3_get_nh_from_egress_object(unit, l2gre_port->egress_if,
                                            &mpath_flag, 1, &vp_nh_ecmp_index);
    BCM_IF_ERROR_RETURN(rv);

    /* Read the existing egress next_hop entry */
    if (mpath_flag == 0) {
        rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY, 
                      vp_nh_ecmp_index, &egr_nh);
        BCM_IF_ERROR_RETURN(rv);
    }

    if (l2gre_port->flags & BCM_L2GRE_PORT_EGRESS_TUNNEL) {

        /* Program DVP entry - Egress */
        rv = _bcm_tr3_l2gre_egress_dvp_set(unit, vp, drop, l2gre_port);
        _BCM_L2GRE_CLEANUP(rv);


        /* Program DVP entry  - Ingress  */
        rv = _bcm_tr3_l2gre_ingress_dvp_set(unit, vp, mpath_flag, vp_nh_ecmp_index, l2gre_port);
        _BCM_L2GRE_CLEANUP(rv);

        if (l2gre_port->flags & BCM_L2GRE_PORT_MULTICAST) {
            egr_nh_info.dvp_is_network = 1; /* Enable */
            egr_nh_info.entry_type = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh, ENTRY_TYPEf);
            if (SOC_IS_TRIDENT2(unit)) {
                  rv = _bcm_tr3_l2gre_nexthop_entry_modify(unit, vp_nh_ecmp_index, drop, 
                             &egr_nh_info, _BCM_TD2_L2GRE_EGR_NEXT_HOP_L3MC_VIEW);
            } else if (SOC_IS_TRIUMPH3(unit)){
                  rv = bcm_tr3_l2gre_egr_nexthop_multicast_set(unit, vp_nh_ecmp_index, vp, l2gre_port);
            }
            _BCM_L2GRE_CLEANUP(rv);
        }
    } else {
        /* Egress into Access - Non Tunnel */
        egr_nh_info.dvp_is_network = 0; /* Disable */
        if (l2gre_port->flags & BCM_L2GRE_PORT_REPLACE) {
            action_present = 
                 soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh,
                                             SD_TAG__SD_TAG_ACTION_IF_PRESENTf);
            action_not_present = 
                 soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh,
                                             SD_TAG__SD_TAG_ACTION_IF_NOT_PRESENTf);
            if ((action_not_present == 0x1) || (action_present == 0x1)) {
                /* If SD tag action is ADD or REPLACE_VID_TPID, the tpid
                 * index of the entry getting replaced is valid. Save
                 * the old tpid index to be deleted later.
                 */
                old_tpid_idx =
                      soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh,
                                             SD_TAG__SD_TAG_TPID_INDEXf);
            }
        }

        if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_TAGGED) {
            egr_nh_info.entry_type = _BCM_L2GRE_EGR_NEXT_HOP_SDTAG_VIEW; /* SD_TAG_ACTIONS */
            /* Configure SD_TAG setting */
            rv = _bcm_tr3_l2gre_sd_tag_set(unit, NULL, l2gre_port, &egr_nh_info, &vxlate_entry, &tpid_index);
            _BCM_L2GRE_CLEANUP(rv);

           /* Configure EGR Next Hop entry -- SD_TAG view */
           rv = _bcm_tr3_l2gre_nexthop_entry_modify(unit, vp_nh_ecmp_index, drop, 
                             &egr_nh_info, _BCM_L2GRE_EGR_NEXT_HOP_SDTAG_VIEW);
           _BCM_L2GRE_CLEANUP(rv);
        }
        if (!(l2gre_port->flags & BCM_L2GRE_PORT_MULTICAST)) {
            /* Program DVP entry  with ECMP / Next Hop Index */
            rv = _bcm_tr3_l2gre_ingress_dvp_set(unit, vp, mpath_flag, vp_nh_ecmp_index, l2gre_port);
            _BCM_L2GRE_CLEANUP(rv);
        }
    }

    /* Delete old TPID, Nexthop indexes */
    if (old_tpid_idx != -1) {
        (void)_bcm_fb2_outer_tpid_entry_delete(unit, old_tpid_idx);
    }
    return rv;

cleanup:
    if (tpid_index != -1) {
        (void) _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_port_nh_delete
 * Purpose:
 *      Delete L2GRE Next Hop entry
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 */     

STATIC int
_bcm_tr3_l2gre_port_nh_delete(int unit, bcm_vpn_t vpn, int vp)
{
    int rv=BCM_E_NONE;
    int  nh_ecmp_index=-1;
    ing_dvp_table_entry_t dvp;
    uint32  vp_type=0;
    uint32  flags=0;
    int  ref_count=0;
    int ecmp =-1;

    BCM_IF_ERROR_RETURN(
        READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));

    vp_type = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, VP_TYPEf);
    ecmp = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, ECMPf);
    if (ecmp) {
        nh_ecmp_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, ECMP_PTRf);
        flags = BCM_L3_MULTIPATH;
        /* Decrement reference count */
        BCM_IF_ERROR_RETURN(
             bcm_xgs3_get_ref_count_from_nhi(unit, flags, &ref_count, nh_ecmp_index));
    } else {
        nh_ecmp_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, 
                                              NEXT_HOP_INDEXf);
        if(nh_ecmp_index != 0) {
             /* Decrement reference count */
             BCM_IF_ERROR_RETURN(
                  bcm_xgs3_get_ref_count_from_nhi(unit, flags, &ref_count, nh_ecmp_index));
        }
        
        if (ref_count == 1) {
              if (nh_ecmp_index) {
                  BCM_IF_ERROR_RETURN(
                     bcm_tr3_l2gre_port_egress_nexthop_reset
                                (unit, nh_ecmp_index, vp_type, vp, vpn));
              }
         }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_port_nh_get
 * Purpose:
 *      Get L2GRE Next Hop entry
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 */     

STATIC int
_bcm_tr3_l2gre_port_nh_get(int unit, bcm_vpn_t vpn,  int vp, bcm_l2gre_port_t *l2gre_port)
{
    egr_l3_next_hop_entry_t egr_nh;
    ing_dvp_table_entry_t dvp;
    int nh_ecmp_index=0;
    uint32 ecmp;
    int rv = BCM_E_NONE;

    /* Read the HW entries */
    BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));

    if (soc_ING_DVP_TABLEm_field32_get(unit, &dvp, VP_TYPEf) == 
                              _BCM_L2GRE_INGRESS_DEST_VP_TYPE) {
        /* Egress into L2GRE tunnel, find the tunnel_if */
        l2gre_port->flags |= BCM_L2GRE_PORT_EGRESS_TUNNEL;
        BCM_IF_ERROR_RETURN(
             READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
        ecmp = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, ECMPf);
        if (ecmp) {
            nh_ecmp_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, ECMP_PTRf);
            l2gre_port->egress_if  =  nh_ecmp_index + BCM_XGS3_MPATH_EGRESS_IDX_MIN;
        } else {
            nh_ecmp_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
            /* Extract next hop index from unipath egress object. */
            l2gre_port->egress_if  =  nh_ecmp_index + BCM_XGS3_EGRESS_IDX_MIN;
        }

        rv = bcm_tr3_l2gre_nexthop_get(unit, nh_ecmp_index, l2gre_port);
        BCM_IF_ERROR_RETURN(rv);

    } else if (soc_ING_DVP_TABLEm_field32_get(unit, &dvp, 
                                                VP_TYPEf) == _BCM_L2GRE_DEST_VP_TYPE_ACCESS) {
        /* Egress into Access-side, find the tunnel_if */
        BCM_IF_ERROR_RETURN(
            READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
        nh_ecmp_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        /* Extract next hop index from unipath egress object. */
        l2gre_port->egress_if  =  nh_ecmp_index + BCM_XGS3_EGRESS_IDX_MIN;

        BCM_IF_ERROR_RETURN (soc_mem_read(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY, 
                                      nh_ecmp_index, &egr_nh));
         if (BCM_SUCCESS(rv)) {
              (void) _bcm_tr3_l2gre_sd_tag_get( unit, NULL, l2gre_port, &egr_nh,
                         NULL, 0);
         }

    } else {
        return BCM_E_NOT_FOUND;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *		_bcm_tr3_l2gre_match_vpnid_entry_update
 * Purpose:
 *		Update Match L2GRE VPNID Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  l2gre_entry
 *  OUT :  l2gre_entry

 */

STATIC int
_bcm_tr3_l2gre_match_vpnid_entry_update(int unit,
                                             mpls_entry_entry_t *ment, 
                                             mpls_entry_entry_t *return_ment)
{
    uint32 value, key_type;

    /* Check if Key_Type identical */
    key_type = soc_MPLS_ENTRYm_field32_get (unit, ment, KEY_TYPEf);
    value = soc_MPLS_ENTRYm_field32_get (unit, return_ment, KEY_TYPEf);
    if (key_type != value) {
         return BCM_E_PARAM;
    }

    soc_MPLS_ENTRYm_field32_set(unit, return_ment, VALIDf, 1);

    value = soc_MPLS_ENTRYm_field32_get(unit, ment, L2GRE_VPNID__SIPf);
    soc_MPLS_ENTRYm_field32_set(unit, return_ment, L2GRE_VPNID__SIPf, value);
    value = soc_MPLS_ENTRYm_field32_get(unit, ment, L2GRE_VPNID__VFIf);
    soc_MPLS_ENTRYm_field32_set(unit, return_ment, L2GRE_VPNID__VFIf, value);
    value = soc_MPLS_ENTRYm_field32_get(unit, ment, L2GRE_VPNID__VPNIDf);
    soc_MPLS_ENTRYm_field32_set(unit, return_ment, L2GRE_VPNID__VPNIDf, value);

   return BCM_E_NONE;
}


/*
 * Function:
 *		bcm_tr3_mpls_match_label_entry_set
 * Purpose:
 *		Set L2GRE Match VPNID Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  mpls_entry
 * Returns:
 *		BCM_E_XXX
 */

int
bcm_tr3_l2gre_match_vpnid_entry_set(int unit, mpls_entry_entry_t *ment)
{
    mpls_entry_entry_t return_ment;
    int rv = BCM_E_UNAVAIL;
    int index;

    rv = soc_mem_search(unit, MPLS_ENTRYm, MEM_BLOCK_ANY, &index,
                        ment, &return_ment, 0);

    if (rv == SOC_E_NONE) {
         BCM_IF_ERROR_RETURN(
              _bcm_tr3_l2gre_match_vpnid_entry_update (unit, ment, &return_ment));
         rv = soc_mem_write(unit, MPLS_ENTRYm,
                                           MEM_BLOCK_ALL, index,
                                           &return_ment);
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    } else {
        rv = soc_mem_insert(unit, MPLS_ENTRYm, MEM_BLOCK_ALL, ment);
    }
    return rv;
}

/*
 * Function:
 *		_bcm_tr3_mpls_match_label_entry_reset
 * Purpose:
 *		Reset L2GRE Match VPNID Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  mpls_entry
 * Returns:
 *		BCM_E_XXX
 */

int
bcm_tr3_l2gre_match_vpnid_entry_reset(int unit, mpls_entry_entry_t *ment)
{
    mpls_entry_entry_t return_ment;
    int rv = BCM_E_UNAVAIL;
    int index=0;

    rv = soc_mem_search(unit, MPLS_ENTRYm, MEM_BLOCK_ANY, &index,
                        ment, &return_ment, 0);
    if ( (rv != BCM_E_NOT_FOUND) && (rv != BCM_E_NONE) ) {
         return rv;
    }

    sal_memset(&return_ment, 0, sizeof(mpls_entry_entry_t));
    if (rv == SOC_E_NONE) {
         rv = soc_mem_write(unit, MPLS_ENTRYm,
                                           MEM_BLOCK_ALL, index,
                                           &return_ment);
    }
    if ( (rv != BCM_E_NOT_FOUND) && (rv != BCM_E_NONE) ) {
       return rv;
    } else {
       return BCM_E_NONE;
    }
}

/*
 * Function:
 *		_bcm_tr3_l2gre_match_tunnel_entry_update
 * Purpose:
 *		Update Match L2GRE Tunnel Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  l2gre_entry
 *  OUT :  l2gre_entry

 */

STATIC int
_bcm_tr3_l2gre_match_tunnel_entry_update(int unit,
                                             mpls_entry_entry_t *ment, 
                                             mpls_entry_entry_t *return_ment)
{
    uint32 value, key_type;

    /* Check if Key_Type identical */
    key_type = soc_MPLS_ENTRYm_field32_get (unit, ment, KEY_TYPEf);
    value = soc_MPLS_ENTRYm_field32_get (unit, return_ment, KEY_TYPEf);
    if (key_type != value) {
         return BCM_E_PARAM;
    }

    soc_MPLS_ENTRYm_field32_set(unit, return_ment, VALIDf, 1);

    value = soc_MPLS_ENTRYm_field32_get(unit, ment, L2GRE_SIP__SIPf);
    soc_MPLS_ENTRYm_field32_set(unit, return_ment, L2GRE_SIP__SIPf, value);
    value = soc_MPLS_ENTRYm_field32_get(unit, ment, L2GRE_SIP__SVPf);
    soc_MPLS_ENTRYm_field32_set(unit, return_ment, L2GRE_SIP__SVPf, value);

   return BCM_E_NONE;
}


/*
 * Function:
 *		_bcm_tr3_mpls_match_tunnel_entry_set
 * Purpose:
 *		Set L2GRE Match Tunnel Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  mpls_entry
 * Returns:
 *		BCM_E_XXX
 */

int
bcm_tr3_l2gre_match_tunnel_entry_set(int unit, mpls_entry_entry_t *ment)
{
    mpls_entry_entry_t return_ment;
    int rv = BCM_E_UNAVAIL;
    int index;

    rv = soc_mem_search(unit, MPLS_ENTRYm, MEM_BLOCK_ANY, &index,
                        ment, &return_ment, 0);

    if (rv == SOC_E_NONE) {
         BCM_IF_ERROR_RETURN(
              _bcm_tr3_l2gre_match_tunnel_entry_update (unit, ment, &return_ment));
         rv = soc_mem_write(unit, MPLS_ENTRYm,
                                           MEM_BLOCK_ALL, index,
                                           &return_ment);
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    } else {
        rv = soc_mem_insert(unit, MPLS_ENTRYm, MEM_BLOCK_ALL, ment);
    }
    return rv;
}


/*
 * Function:
 *		_bcm_tr3_mpls_match_tunnel_entry_reset
 * Purpose:
 *		Reet L2GRE Match Tunnel Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  mpls_entry
 * Returns:
 *		BCM_E_XXX
 */

int
bcm_tr3_l2gre_match_tunnel_entry_reset(int unit, mpls_entry_entry_t *ment)
{
    mpls_entry_entry_t return_ment;
    int rv = BCM_E_UNAVAIL;
    int index;

    rv = soc_mem_search(unit, MPLS_ENTRYm, MEM_BLOCK_ANY, &index,
                        ment, &return_ment, 0);

    if ( (rv != BCM_E_NOT_FOUND) && (rv != BCM_E_NONE) ) {
         return rv;
    }

    sal_memset(&return_ment, 0, sizeof(mpls_entry_entry_t));

    if (rv == SOC_E_NONE) {
         rv = soc_mem_write(unit, MPLS_ENTRYm,
                                           MEM_BLOCK_ALL, index,
                                           &return_ment);
    }

    if ( (rv != BCM_E_NOT_FOUND) && (rv != BCM_E_NONE) ) {
       return rv;
    } else {
       return BCM_E_NONE;
    }
}

/*
 * Function:
 *		_bcm_tr3_mpls_match_vlan_extd_entry_update
 * Purpose:
 *		Update L2GRE Match Vlan_xlate_extd  Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  vlan_xlate_extd_entry_t
 *  OUT :  vlan_xlate_extd_entry_t

 */

STATIC int
_bcm_tr3_l2gre_match_vxlate_extd_entry_update(int unit, vlan_xlate_extd_entry_t *vent, 
                                                                     vlan_xlate_extd_entry_t *return_ent)
{
    uint32  vp, key_type, value;

    /* Check if Key_Type identical */
    key_type = soc_VLAN_XLATE_EXTDm_field32_get (unit, vent, KEY_TYPE_0f);
    value = soc_VLAN_XLATE_EXTDm_field32_get (unit, return_ent, KEY_TYPE_0f);
    if (key_type != value) {
         return BCM_E_PARAM;
    }

    /* Retain original Keys -- Update data only */
    soc_VLAN_XLATE_EXTDm_field32_set(unit, return_ent, XLATE__MPLS_ACTIONf, 0x1);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, return_ent, XLATE__DISABLE_VLAN_CHECKSf, 1);
    vp = soc_VLAN_XLATE_EXTDm_field32_get (unit, vent, XLATE__SOURCE_VPf);
    soc_VLAN_XLATE_EXTDm_field32_set(unit, return_ent, XLATE__SOURCE_VPf, vp);

   return BCM_E_NONE;
}


/*
 * Function:
 *        _bcm_tr3_l2gre_match_vxlate_extd_entry_set
 * Purpose:
 *       Set L2GRE Match Vxlate Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  l2gre_port
 *  IN :  vlan_xlate_extd_entry_t
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_match_vxlate_extd_entry_set(int unit, bcm_l2gre_port_t *l2gre_port, vlan_xlate_extd_entry_t *vent)
{
    vlan_xlate_extd_entry_t return_vent;
    int rv = BCM_E_UNAVAIL;
    int index;

    rv = soc_mem_search(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ANY, &index,
                        vent, &return_vent, 0);

    if (rv == SOC_E_NONE) {
         BCM_IF_ERROR_RETURN(
              _bcm_tr3_l2gre_match_vxlate_extd_entry_update (unit, vent, &return_vent));
         rv = soc_mem_write(unit, VLAN_XLATE_EXTDm,
                                           MEM_BLOCK_ALL, index,
                                           &return_vent);
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    } else {
         if (l2gre_port->flags & BCM_L2GRE_PORT_REPLACE) {
             return BCM_E_NOT_FOUND;
         } else {
              rv = soc_mem_insert(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ALL, vent);
         }
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_match_clear
 * Purpose:
 *      Clear L2GRE Match Software State
 * Parameters:
 *      unit    - (IN) Device Number
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      void
 */

void
bcm_tr3_l2gre_match_clear (int unit, int vp)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info = L2GRE_INFO(unit);

    l2gre_info->match_key[vp].flags = 0;
    l2gre_info->match_key[vp].match_vlan = 0;
    l2gre_info->match_key[vp].match_inner_vlan = 0;
    l2gre_info->match_key[vp].trunk_id = -1;
    l2gre_info->match_key[vp].port = 0;
    l2gre_info->match_key[vp].modid = -1;
    l2gre_info->match_key[vp].match_tunnel_index= 0;
    l2gre_info->match_key[vp].match_vpnid = 0;

    return;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_trunk_table_set
 * Purpose:
 *      Configure L2GRE Trunk port entry
 * Parameters:
 *      unit    - (IN) Device Number
 *      port   - (IN) Trunk member port
 *      tgid - (IN) Trunk group Id
 *      vp   - (IN) Virtual port
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_trunk_table_set(int unit, bcm_port_t port, bcm_trunk_t tgid, int vp)
{
    source_trunk_map_table_entry_t trunk_map_entry;/*Trunk table entry buffer.*/
    bcm_module_t my_modid;
    int    src_trk_idx = -1;
    int    port_type;
    int rv = BCM_E_NONE;

    if (tgid != BCM_TRUNK_INVALID) {
        port_type = 1; /* TRUNK_PORT_TYPE */
    } else {
        return BCM_E_PARAM;
    }

    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));

    /* Get index to source trunk map table */
    BCM_IF_ERROR_RETURN(_bcm_esw_src_mod_port_table_index_get(unit, my_modid,
                port, &src_trk_idx));

    soc_mem_lock(unit, SOURCE_TRUNK_MAP_TABLEm);

    /* Read source trunk map table. */
    BCM_IF_ERROR_RETURN (soc_mem_read(unit, SOURCE_TRUNK_MAP_TABLEm, 
                MEM_BLOCK_ANY, src_trk_idx, &trunk_map_entry));


    /* Set trunk group id. */ 
    soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm, &trunk_map_entry,
            TGIDf, tgid);

    /* Set port is part of Trunk group */
    soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm, &trunk_map_entry,
            PORT_TYPEf, port_type);

    /* Enable SVP Mode */
    if (SOC_MEM_FIELD_VALID(unit,SOURCE_TRUNK_MAP_TABLEm,SVP_VALIDf)) {
        soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm, &trunk_map_entry,
               SVP_VALIDf, 0x1);
    }

    /* Set SVP */
    soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm, &trunk_map_entry,
            SOURCE_VPf, vp);

    /* Write entry to hw. */
    rv = soc_mem_write(unit, SOURCE_TRUNK_MAP_TABLEm, MEM_BLOCK_ALL,
            src_trk_idx, &trunk_map_entry);

    soc_mem_unlock(unit, SOURCE_TRUNK_MAP_TABLEm);

    return rv; 
}


/*
 * Function:
 *      _bcm_tr3_l2gre_trunk_table_reset
 * Purpose:
 *      Reset L2GRE Trunk port entry
 * Parameters:
 *      unit    - (IN) Device Number
 *      port   - (IN) Trunk member port
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_trunk_table_reset(int unit, bcm_port_t port)
{
    source_trunk_map_table_entry_t trunk_map_entry;/*Trunk table entry buffer.*/
    bcm_module_t my_modid;
    int    src_trk_idx = -1;
    int rv = BCM_E_NONE;

    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &my_modid));

    /* Get index to source trunk map table */
    BCM_IF_ERROR_RETURN(_bcm_esw_src_mod_port_table_index_get(unit, my_modid,
                port, &src_trk_idx));

    soc_mem_lock(unit, SOURCE_TRUNK_MAP_TABLEm);

    /* Read source trunk map table. */
    BCM_IF_ERROR_RETURN (soc_mem_read(unit, SOURCE_TRUNK_MAP_TABLEm, 
                MEM_BLOCK_ANY, src_trk_idx, &trunk_map_entry));

    /* Disable SVP Mode */
    if (SOC_MEM_FIELD_VALID(unit,SOURCE_TRUNK_MAP_TABLEm,SVP_VALIDf)) {
       soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm, &trunk_map_entry,
            SVP_VALIDf, 0);
    }

    /* Set SVP */
    soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm, &trunk_map_entry,
            SOURCE_VPf, 0);

    /* Write entry to hw. */
    rv = soc_mem_write(unit, SOURCE_TRUNK_MAP_TABLEm, MEM_BLOCK_ALL,
            src_trk_idx, &trunk_map_entry);

    soc_mem_unlock(unit, SOURCE_TRUNK_MAP_TABLEm);

    return rv; 
}
/*
 * Function:
 *      bcm_tr3_l2gre_match_trunk_add
 * Purpose:
 *      Assign SVP of  L2GRE Trunk port
 * Parameters:
 *      unit    - (IN) Device Number
 *      tgid - (IN) Trunk group Id
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

int
 bcm_tr3_l2gre_match_trunk_add(int unit, bcm_trunk_t tgid, int vp)
{
     int port_idx=0;
    int local_port_out[SOC_MAX_NUM_PORTS];
    int local_member_count=0;
    int rv = BCM_E_NONE;

    BCM_IF_ERROR_RETURN(_bcm_esw_trunk_local_members_get(unit, tgid,
                SOC_MAX_NUM_PORTS, local_port_out, &local_member_count));
    for (port_idx = 0; port_idx < local_member_count; port_idx++) {
        rv = _bcm_tr3_l2gre_trunk_table_set(unit, local_port_out[port_idx], tgid, vp);
        if (BCM_FAILURE(rv)) {
            goto trunk_cleanup;
        }
        rv = soc_mem_field32_modify(unit, PORT_TABm, local_port_out[port_idx],
                                     PORT_OPERATIONf, 0x1); /* L2_SVP */
        if (BCM_FAILURE(rv)) {
            goto trunk_cleanup;
        }
    }

    return BCM_E_NONE;

 trunk_cleanup:
    for (;port_idx >= 0; port_idx--) {
        (void)soc_mem_field32_modify(unit, PORT_TABm, local_port_out[port_idx],
                                  PORT_OPERATIONf, 0x0);
        (void) _bcm_tr3_l2gre_trunk_table_reset(unit, local_port_out[port_idx]);
    }
    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_match_trunk_delete
 * Purpose:
 *      Remove SVP of L2GRE Trunk port
 * Parameters:
 *      unit    - (IN) Device Number
 *      tgid - (IN) Trunk group Id
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr3_l2gre_match_trunk_delete(int unit, bcm_trunk_t tgid, int vp)
{
    int port_idx=0;
    int local_port_out[SOC_MAX_NUM_PORTS];
    int local_member_count=0;
    int rv = BCM_E_NONE;

    BCM_IF_ERROR_RETURN(_bcm_esw_trunk_local_members_get(unit, tgid,
                SOC_MAX_NUM_PORTS, local_port_out, &local_member_count));

    for (port_idx = 0; port_idx < local_member_count; port_idx++) {
        rv = _bcm_tr3_l2gre_trunk_table_reset(unit, local_port_out[port_idx]);
        if (BCM_FAILURE(rv)) {
            goto trunk_cleanup;
        }
        rv = soc_mem_field32_modify(unit, PORT_TABm, local_port_out[port_idx],
                                     PORT_OPERATIONf, 0x0); /* L2_SVP */
        if (BCM_FAILURE(rv)) {
            goto trunk_cleanup;
        }
    }

    return BCM_E_NONE;

 trunk_cleanup:
    for (;port_idx >= 0; port_idx--) {
        (void)soc_mem_field32_modify(unit, PORT_TABm, local_port_out[port_idx],
                                  PORT_OPERATIONf, 0x1);
        (void) _bcm_tr3_l2gre_trunk_table_set(unit, local_port_out[port_idx], tgid, vp);
    }
    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_match_add
 * Purpose:
 *      Assign match criteria of an L2GRE port
 * Parameters:
 *      unit    - (IN) Device Number
 *      l2gre_port - (IN) l2gre port information
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr3_l2gre_match_add(int unit, bcm_l2gre_port_t *l2gre_port, int vp, bcm_vpn_t vpn)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info = L2GRE_INFO(unit);
    int rv = BCM_E_NONE;
    bcm_module_t mod_out = -1;
    bcm_port_t port_out = -1;
    bcm_trunk_t trunk_id = -1;
    vlan_xlate_extd_entry_t vent;
    int    mod_id_idx=0; /* Module Id */
    int    src_trk_idx=0;  /*Source Trunk table index.*/
    int    gport_id=-1;

    rv = _bcm_tr3_l2gre_port_resolve(unit, l2gre_port->l2gre_port_id, -1, &mod_out,
                    &port_out, &trunk_id, &gport_id);
    BCM_IF_ERROR_RETURN(rv);

    sal_memset(&vent, 0, sizeof(vlan_xlate_extd_entry_t));

    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &mod_id_idx));

    if (l2gre_port->criteria == BCM_L2GRE_PORT_MATCH_PORT_VLAN ) {

        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_0f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_1f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);
        if (!BCM_VLAN_VALID(l2gre_port->match_vlan)) {
             return BCM_E_PARAM;
        }
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_0f, 
                                        TR3_VLXLT_X_HASH_KEY_TYPE_OVID);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_1f, 
                                        TR3_VLXLT_X_HASH_KEY_TYPE_OVID);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__OVIDf, 
                                        l2gre_port->match_vlan);
        l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_VLAN;
        l2gre_info->match_key[vp].match_vlan = l2gre_port->match_vlan;

        if (BCM_GPORT_IS_TRUNK(l2gre_port->match_port)) {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
            l2gre_info->match_key[vp].trunk_id = trunk_id;
        } else {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
            l2gre_info->match_key[vp].port = port_out;
            l2gre_info->match_key[vp].modid = mod_out;
        }
        rv = _bcm_tr3_l2gre_match_vxlate_extd_entry_set(unit, l2gre_port, &vent);
        BCM_IF_ERROR_RETURN(rv);
        if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
              bcm_tr3_l2gre_port_match_count_adjust(unit, vp, 1);
        }
    } else if (l2gre_port->criteria ==
                            BCM_L2GRE_PORT_MATCH_PORT_INNER_VLAN) {

        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_0f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_1f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);
        if (!BCM_VLAN_VALID(l2gre_port->match_inner_vlan)) {
            return BCM_E_PARAM;
        }
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_0f,
                                    TR3_VLXLT_X_HASH_KEY_TYPE_IVID);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_1f,
                                    TR3_VLXLT_X_HASH_KEY_TYPE_IVID);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__IVIDf,
                                    l2gre_port->match_inner_vlan);
        l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_INNER_VLAN;
        l2gre_info->match_key[vp].match_inner_vlan = l2gre_port->match_inner_vlan;

        if (BCM_GPORT_IS_TRUNK(l2gre_port->match_port)) {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
            l2gre_info->match_key[vp].trunk_id = trunk_id;
        } else {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
            l2gre_info->match_key[vp].port = port_out;
            l2gre_info->match_key[vp].modid = mod_out;
        }
        rv = _bcm_tr3_l2gre_match_vxlate_extd_entry_set(unit, l2gre_port, &vent);
        BCM_IF_ERROR_RETURN(rv);
        if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
             bcm_tr3_l2gre_port_match_count_adjust(unit, vp, 1);
        }
    } else if (l2gre_port->criteria == 
                            BCM_L2GRE_PORT_MATCH_PORT_VLAN_STACKED) {

         soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_0f, 1);
         soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_1f, 1);
         soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
         soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
         soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);
         if (!BCM_VLAN_VALID(l2gre_port->match_vlan) || 
                !BCM_VLAN_VALID(l2gre_port->match_inner_vlan)) {
              return BCM_E_PARAM;
         }
         soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_0f, 
                   TR3_VLXLT_X_HASH_KEY_TYPE_IVID_OVID);
         soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_1f, 
                   TR3_VLXLT_X_HASH_KEY_TYPE_IVID_OVID);
         soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__OVIDf, 
                   l2gre_port->match_vlan);
         soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__IVIDf, 
                   l2gre_port->match_inner_vlan);
         l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_STACKED;
         l2gre_info->match_key[vp].match_vlan = l2gre_port->match_vlan;
         l2gre_info->match_key[vp].match_inner_vlan = l2gre_port->match_inner_vlan;
         if (BCM_GPORT_IS_TRUNK(l2gre_port->match_port)) {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
            l2gre_info->match_key[vp].trunk_id = trunk_id;
         } else {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
            l2gre_info->match_key[vp].port = port_out;
            l2gre_info->match_key[vp].modid = mod_out;
         }
         rv = _bcm_tr3_l2gre_match_vxlate_extd_entry_set(unit, l2gre_port, &vent);
         BCM_IF_ERROR_RETURN(rv);
         if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
             bcm_tr3_l2gre_port_match_count_adjust(unit, vp, 1);
         }
    } else if (l2gre_port->criteria ==
                                            BCM_L2GRE_PORT_MATCH_VLAN_PRI) {

              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_0f, 1);
              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_1f, 1);
              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);
              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_0f, 
                                                 TR3_VLXLT_X_HASH_KEY_TYPE_PRI_CFI);
              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_1f, 
                                                 TR3_VLXLT_X_HASH_KEY_TYPE_PRI_CFI);
              /* match_vlan : Bits 12-15 contains VLAN_PRI + CFI, vlan=BCM_E_NONE */
              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, OTAGf, 
                                                 l2gre_port->match_vlan);
              l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_PRI;
              l2gre_info->match_key[vp].match_vlan = l2gre_port->match_vlan;

              if (BCM_GPORT_IS_TRUNK(l2gre_port->match_port)) {
                   soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__Tf, 1);
                   soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
                        l2gre_info->match_key[vp].trunk_id = trunk_id;
              } else {
                   soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
                   soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
                   l2gre_info->match_key[vp].port = port_out;
                   l2gre_info->match_key[vp].modid = mod_out;
              }
              rv = _bcm_tr3_l2gre_match_vxlate_extd_entry_set(unit, l2gre_port, &vent);
              BCM_IF_ERROR_RETURN(rv);
              if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
                   bcm_tr3_l2gre_port_match_count_adjust(unit, vp, 1);
              }
    }else if (l2gre_port->criteria == BCM_L2GRE_PORT_MATCH_PORT) {
        if (BCM_GPORT_IS_TRUNK(l2gre_port->match_port)) {
             rv = bcm_tr3_l2gre_match_trunk_add(unit, trunk_id, vp);
             if (rv >= 0) {
                   l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_TRUNK;
                   l2gre_info->match_key[vp].trunk_id = trunk_id;
             }
             BCM_IF_ERROR_RETURN(rv);       
        } else {
            int is_local;

            BCM_IF_ERROR_RETURN
                ( _bcm_esw_modid_is_local(unit, mod_out, &is_local));

            /* Get index to source trunk map table */
            BCM_IF_ERROR_RETURN(
                   _bcm_esw_src_mod_port_table_index_get(unit, mod_out,
                     port_out, &src_trk_idx));

            rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm, 
                                        src_trk_idx, SOURCE_VPf, vp);
            BCM_IF_ERROR_RETURN(rv);

            if (is_local) {
                rv = soc_mem_field32_modify(unit, PORT_TABm, port_out,
                                        PORT_OPERATIONf, 0x1); /* L2_SVP */
                BCM_IF_ERROR_RETURN(rv);

                /* Set TAG_ACTION_PROFILE_PTR */
                rv = bcm_tr3_l2gre_port_untagged_profile_set(unit, port_out);
                BCM_IF_ERROR_RETURN(rv);
            }

            l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_PORT;
            l2gre_info->match_key[vp].index = src_trk_idx;
        }
    }else if (l2gre_port->criteria == BCM_L2GRE_PORT_MATCH_VPNID) {
        mpls_entry_entry_t ment;
        int tunnel_idx=-1;
        uint32 tunnel_sip;

        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
        l2gre_info->match_key[vp].flags = _BCM_L2GRE_PORT_MATCH_TYPE_VPNID;
        if (!BCM_GPORT_IS_TUNNEL(l2gre_port->match_tunnel_id)) {
            return BCM_E_PARAM;
        }

        tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(l2gre_port->match_tunnel_id);
        tunnel_sip = l2gre_info->l2gre_tunnel_term[tunnel_idx].sip;
        l2gre_info->match_key[vp].match_tunnel_index = tunnel_idx;

        if (l2gre_port->flags & BCM_L2GRE_PORT_MULTICAST) {
              return BCM_E_NONE;
        }

        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
        soc_MPLS_ENTRYm_field32_set(unit, &ment, L2GRE_SIP__SIPf, tunnel_sip);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, L2GRE_SIP__SVPf, vp);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, KEY_TYPEf, _BCM_L2GRE_KEY_TYPE_TUNNEL);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 1);

        rv = bcm_tr3_l2gre_match_tunnel_entry_set(unit, &ment);
        BCM_IF_ERROR_RETURN(rv);
    }

    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_match_delete
 * Purpose:
 *      Delete match criteria of an L2GRE port
 * Parameters:
 *      unit    - (IN) Device Number
 *      l2gre_port - (IN) L2GRE port information
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr3_l2gre_match_delete(int unit,  int vp)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info = L2GRE_INFO(unit);
    int rv=BCM_E_NONE;
    vlan_xlate_extd_entry_t vent;
    bcm_trunk_t trunk_id;
    int    src_trk_idx=0;   /*Source Trunk table index.*/
    int    mod_id_idx=0;   /* Module_Id */
    int port=0, tunnel_index = -1;
    mpls_entry_entry_t ment;

    sal_memset(&vent, 0, sizeof(vlan_xlate_extd_entry_t));

    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &mod_id_idx));

    if  (l2gre_info->match_key[vp].flags == _BCM_L2GRE_PORT_MATCH_TYPE_VLAN) {

        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_0f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_1f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_0f, 
                                    TR3_VLXLT_X_HASH_KEY_TYPE_OVID);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_1f, 
                                    TR3_VLXLT_X_HASH_KEY_TYPE_OVID);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__OVIDf,
                                    l2gre_info->match_key[vp].match_vlan);
        if (l2gre_info->match_key[vp].modid != -1) {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                        l2gre_info->match_key[vp].modid);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                        l2gre_info->match_key[vp].port);
        } else {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__TGIDf, 
                                        l2gre_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);
        
        (void) bcm_tr3_l2gre_match_clear (unit, vp);
         bcm_tr3_l2gre_port_match_count_adjust(unit, vp, -1);

    } else if  (l2gre_info->match_key[vp].flags == 
                     _BCM_L2GRE_PORT_MATCH_TYPE_INNER_VLAN) {

        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_0f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_1f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_0f, 
                                    TR3_VLXLT_X_HASH_KEY_TYPE_IVID);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_1f, 
                                    TR3_VLXLT_X_HASH_KEY_TYPE_IVID);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__IVIDf,
                                    l2gre_info->match_key[vp].match_inner_vlan);
        if (l2gre_info->match_key[vp].modid != -1) {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                        l2gre_info->match_key[vp].modid);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                        l2gre_info->match_key[vp].port);
        } else {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__TGIDf, 
                                        l2gre_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);
        
        (void) bcm_tr3_l2gre_match_clear (unit, vp);
        bcm_tr3_l2gre_port_match_count_adjust(unit, vp, -1);
    }else if (l2gre_info->match_key[vp].flags == 
                    _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_STACKED) {

        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_0f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_1f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_0f, 
                                    TR3_VLXLT_X_HASH_KEY_TYPE_IVID_OVID);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_1f, 
                                    TR3_VLXLT_X_HASH_KEY_TYPE_IVID_OVID);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__OVIDf,
                                    l2gre_info->match_key[vp].match_vlan);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__IVIDf,
                              l2gre_info->match_key[vp].match_inner_vlan);
        if (l2gre_info->match_key[vp].modid != -1) {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                        l2gre_info->match_key[vp].modid);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                        l2gre_info->match_key[vp].port);
        } else {
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__TGIDf, 
                                        l2gre_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);

        (void) bcm_tr3_l2gre_match_clear (unit, vp);
        bcm_tr3_l2gre_port_match_count_adjust(unit, vp, -1);

    } else if	(l2gre_info->match_key[vp].flags ==
                                               _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_PRI) {

        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_0f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, VALID_1f, 1);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_0f, 
                                                      TR3_VLXLT_X_HASH_KEY_TYPE_PRI_CFI);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, KEY_TYPE_1f, 
                                                      TR3_VLXLT_X_HASH_KEY_TYPE_PRI_CFI);
        soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__OTAGf,
                                                       l2gre_info->match_key[vp].match_vlan);
        if (l2gre_info->match_key[vp].modid != -1) {
              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                                      l2gre_info->match_key[vp].modid);
              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                                      l2gre_info->match_key[vp].port);
        } else {
              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__Tf, 1);
              soc_VLAN_XLATE_EXTDm_field32_set(unit, &vent, XLATE__TGIDf, 
                                                      l2gre_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATE_EXTDm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);

        (void) bcm_tr3_l2gre_match_clear (unit, vp);
        bcm_tr3_l2gre_port_match_count_adjust(unit, vp, -1);
    }else if  (l2gre_info->match_key[vp].flags == 
                    _BCM_L2GRE_PORT_MATCH_TYPE_PORT) {
         int is_local;

         src_trk_idx = l2gre_info->match_key[vp].index;
         rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm, 
                                  src_trk_idx, SOURCE_VPf, 0);
         BCM_IF_ERROR_RETURN(rv);

         BCM_IF_ERROR_RETURN(_bcm_esw_src_modid_port_get( unit, src_trk_idx,
                                                          &mod_id_idx, &port));
         BCM_IF_ERROR_RETURN
             ( _bcm_esw_modid_is_local(unit, mod_id_idx, &is_local));

         if (is_local) {
             /* Convert system ports to physical ports */ 
             if (soc_feature(unit, soc_feature_sysport_remap)) { 
                 BCM_XLATE_SYSPORT_S2P(unit, &port); 
             }

             rv = soc_mem_field32_modify(unit, PORT_TABm, port,
                                           PORT_OPERATIONf, 0x0); /* NORMAL */
             BCM_IF_ERROR_RETURN(rv);
    
             /* Reset TAG_ACTION_PROFILE_PTR */
             rv = bcm_tr3_l2gre_port_untagged_profile_reset(unit, port);
             BCM_IF_ERROR_RETURN(rv);
         }
         (void) bcm_tr3_l2gre_match_clear (unit, vp);
    } else if  (l2gre_info->match_key[vp].flags == 
                  _BCM_L2GRE_PORT_MATCH_TYPE_TRUNK) {
         trunk_id = l2gre_info->match_key[vp].trunk_id;
         rv = bcm_tr3_l2gre_match_trunk_delete(unit, trunk_id, vp);
         BCM_IF_ERROR_RETURN(rv);

        (void) bcm_tr3_l2gre_match_clear (unit, vp);
    } else if (l2gre_info->match_key[vp].flags == 
                  _BCM_L2GRE_PORT_MATCH_TYPE_VPNID) {

        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
        tunnel_index = l2gre_info->match_key[vp].match_tunnel_index;
        soc_MPLS_ENTRYm_field32_set(unit, &ment, L2GRE_VPNID__SIPf, 
                                    l2gre_info->l2gre_tunnel_term[tunnel_index].sip);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, KEY_TYPEf, _BCM_L2GRE_KEY_TYPE_TUNNEL);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 1);

        rv = bcm_tr3_l2gre_match_tunnel_entry_reset(unit, &ment);
        if (BCM_SUCCESS(rv) && (tunnel_index != -1)) {
           l2gre_info->l2gre_tunnel_term[tunnel_index].sip = 0;
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_match_get
 * Purpose:
 *      Obtain match information of an L2GRE port
 * Parameters:
 *      unit    - (IN) Device Number
 *      l2gre_port - (OUT) L2GRE port information
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_tr3_l2gre_match_get(int unit, bcm_l2gre_port_t *l2gre_port, int vp)
{
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info = L2GRE_INFO(unit);
    int rv = BCM_E_NONE;
    bcm_module_t mod_in=0, mod_out;
    bcm_port_t port_in=0, port_out;
    bcm_trunk_t trunk_id=0;
    int    src_trk_idx=0;    /*Source Trunk table index.*/
    int    tunnel_idx = -1;
    int    mode_in=0;   /* Module_Id */
    
    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &mode_in));

    if  (l2gre_info->match_key[vp].flags &  _BCM_L2GRE_PORT_MATCH_TYPE_VLAN) {

         l2gre_port->criteria = BCM_L2GRE_PORT_MATCH_PORT_VLAN;
         l2gre_port->match_vlan = l2gre_info->match_key[vp].match_vlan;

        if (l2gre_info->match_key[vp].trunk_id != -1) {
             trunk_id = l2gre_info->match_key[vp].trunk_id;
             BCM_GPORT_TRUNK_SET(l2gre_port->match_port, trunk_id);
        } else {
             port_in = l2gre_info->match_key[vp].port;
             mod_in = l2gre_info->match_key[vp].modid;
             rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                  mod_in, port_in, &mod_out, &port_out);
             BCM_GPORT_MODPORT_SET(l2gre_port->match_port, mod_out, port_out);
        }
    } else if (l2gre_info->match_key[vp].flags &  
                 _BCM_L2GRE_PORT_MATCH_TYPE_INNER_VLAN) {
         l2gre_port->criteria = BCM_L2GRE_PORT_MATCH_PORT_INNER_VLAN;
         l2gre_port->match_inner_vlan = l2gre_info->match_key[vp].match_inner_vlan;

        if (l2gre_info->match_key[vp].trunk_id != -1) {
             trunk_id = l2gre_info->match_key[vp].trunk_id;
             BCM_GPORT_TRUNK_SET(l2gre_port->match_port, trunk_id);
        } else {
             port_in = l2gre_info->match_key[vp].port;
             mod_in = l2gre_info->match_key[vp].modid;
             rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                  mod_in, port_in, &mod_out, &port_out);
             BCM_GPORT_MODPORT_SET(l2gre_port->match_port, mod_out, port_out);
        }
    }else if (l2gre_info->match_key[vp].flags &
                    _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_STACKED) {

         l2gre_port->criteria = BCM_L2GRE_PORT_MATCH_PORT_VLAN_STACKED;
         l2gre_port->match_vlan = l2gre_info->match_key[vp].match_vlan;
         l2gre_port->match_inner_vlan = l2gre_info->match_key[vp].match_inner_vlan;

         if (l2gre_info->match_key[vp].trunk_id != -1) {
              trunk_id = l2gre_info->match_key[vp].trunk_id;
              BCM_GPORT_TRUNK_SET(l2gre_port->match_port, trunk_id);
         } else {
              port_in = l2gre_info->match_key[vp].port;
              mod_in = l2gre_info->match_key[vp].modid;
              rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                             mod_in, port_in, &mod_out, &port_out);
              BCM_GPORT_MODPORT_SET(l2gre_port->match_port, mod_out, port_out);
         }
    } else if  (l2gre_info->match_key[vp].flags &  _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_PRI) {

         l2gre_port->criteria = BCM_L2GRE_PORT_MATCH_VLAN_PRI;
         l2gre_port->match_vlan = l2gre_info->match_key[vp].match_vlan;

        if (l2gre_info->match_key[vp].trunk_id != -1) {
             trunk_id = l2gre_info->match_key[vp].trunk_id;
             BCM_GPORT_TRUNK_SET(l2gre_port->match_port, trunk_id);
        } else {
             port_in = l2gre_info->match_key[vp].port;
             mod_in = l2gre_info->match_key[vp].modid;
             rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                  mod_in, port_in, &mod_out, &port_out);
             BCM_GPORT_MODPORT_SET(l2gre_port->match_port, mod_out, port_out);
        }
    } else if (l2gre_info->match_key[vp].flags &
                   _BCM_L2GRE_PORT_MATCH_TYPE_PORT) {

         src_trk_idx = l2gre_info->match_key[vp].index;
         BCM_IF_ERROR_RETURN(_bcm_esw_src_modid_port_get( unit, src_trk_idx,
                                                          &mode_in, &port_in));

         l2gre_port->criteria = BCM_L2GRE_PORT_MATCH_PORT;
         rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                               mod_in, port_in, &mod_out, &port_out);
         BCM_GPORT_MODPORT_SET(l2gre_port->match_port, mod_out, port_out);
    }else if (l2gre_info->match_key[vp].flags &
                  _BCM_L2GRE_PORT_MATCH_TYPE_TRUNK) {

         trunk_id = l2gre_info->match_key[vp].trunk_id;
         l2gre_port->criteria = BCM_L2GRE_PORT_MATCH_PORT;
         BCM_GPORT_TRUNK_SET(l2gre_port->match_port, trunk_id);
    } else if ((l2gre_info->match_key[vp].flags &
                          _BCM_L2GRE_PORT_MATCH_TYPE_VPNID)) {
        l2gre_port->criteria = BCM_L2GRE_PORT_MATCH_VPNID;

        tunnel_idx = l2gre_info->match_key[vp].match_tunnel_index;
        BCM_GPORT_TUNNEL_ID_SET(l2gre_port->match_tunnel_id, tunnel_idx);

    } else {
        l2gre_port->criteria = BCM_L2GRE_PORT_MATCH_NONE;
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_eline_vp_map_set
 * Purpose:
 *      Set L2GRE ELINE ports from VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_tr3_l2gre_eline_vp_map_set(int unit, int vfi_index, int vp1, int vp2)
{
    vfi_entry_t vfi_entry;
    int rv=BCM_E_NONE;
    int num_vp;

    if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    BCM_IF_ERROR_RETURN
        (READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry));

    if (soc_VFIm_field32_get(unit, &vfi_entry, PT2PT_ENf)) {
         if ((vp1 > 0) && (vp1 < num_vp)) {
              soc_VFIm_field32_set(unit, &vfi_entry, VP_0f, vp1);
         } else {
              return BCM_E_PARAM;
         }
         if ((vp2 > 0) && (vp2 < num_vp)) {
              soc_VFIm_field32_set(unit, &vfi_entry, VP_1f, vp2);
         } else {
              return BCM_E_PARAM;
         }
    } else {
         /* ELAN */
         return BCM_E_PARAM;
    }

    rv = WRITE_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry);
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_eline_vp_map_get
 * Purpose:
 *      Get L2GRE ELINE ports from VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_eline_vp_map_get(int unit, int vfi_index, int *vp1, int *vp2)
{
    vfi_entry_t vfi_entry;

    if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }

    BCM_IF_ERROR_RETURN
        (READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry));

    if (soc_VFIm_field32_get(unit, &vfi_entry, PT2PT_ENf)) {
        *vp1 = soc_VFIm_field32_get(unit, &vfi_entry, VP_0f);
        *vp2 = soc_VFIm_field32_get(unit, &vfi_entry, VP_1f);
         return BCM_E_NONE;
    } else {
         return BCM_E_PARAM;
    }
}

/*
 * Function:
 *      _bcm_tr3_l2gre_eline_vp_map_clear
 * Purpose:
 *      Clear L2GRE ELINE ports from VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_eline_vp_map_clear(int unit, int vfi_index, int vp1, int vp2)
{
    vfi_entry_t vfi_entry;
    int rv=BCM_E_NONE;
    int num_vp;

    if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    BCM_IF_ERROR_RETURN
        (READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry));

    if (soc_VFIm_field32_get(unit, &vfi_entry, PT2PT_ENf)) {
         if ((vp1 > 0) && (vp1 < num_vp)) {
             soc_VFIm_field32_set(unit, &vfi_entry, VP_0f, 0);
         } else if ((vp2 > 0) && (vp2 < num_vp)) {
             soc_VFIm_field32_set(unit, &vfi_entry, VP_1f, 0);
         } else {
              return BCM_E_PARAM;
         }
    } else {
         /* ELAN */
         return BCM_E_PARAM;
    }
    rv = WRITE_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry);
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_eline_vp_configure
 * Purpose:
 *      Configure L2GRE ELINE virtual port
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_tr3_l2gre_eline_vp_configure (int unit, int vfi_index, int active_vp, 
                  source_vp_entry_t *svp, int tpid_enable, bcm_l2gre_port_t  *l2gre_port)
{
    int rv = BCM_E_NONE;

    /* Set SOURCE_VP */
    soc_SOURCE_VPm_field32_set(unit, svp, CLASS_IDf,
                                                                l2gre_port->if_class);
    soc_SOURCE_VPm_field32_set(unit, svp, NETWORK_PORTf,
                        (l2gre_port->flags & BCM_L2GRE_PORT_NETWORK) ? 1 : 0);

    if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_TAGGED) {
         soc_SOURCE_VPm_field32_set(unit, svp, SD_TAG_MODEf, 1);
         soc_SOURCE_VPm_field32_set(unit, svp, TPID_ENABLEf, tpid_enable);
    } else {
         soc_SOURCE_VPm_field32_set(unit, svp, SD_TAG_MODEf, 0);
    }

     soc_SOURCE_VPm_field32_set(unit, svp, DISABLE_VLAN_CHECKSf, 1);
     soc_SOURCE_VPm_field32_set(unit, svp, 
                           ENTRY_TYPEf, _BCM_L2GRE_SOURCE_VP_TYPE_VFI); /* VFI Type */
     soc_SOURCE_VPm_field32_set(unit, svp, VFIf, vfi_index);

     BCM_IF_ERROR_RETURN
           (WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, active_vp, svp));

    return rv;
}


/*
 * Function:
 *      _bcm_tr3_l2gre_elan_vp_configure
 * Purpose:
 *      Configure L2GRE ELINE virtual port
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_tr3_l2gre_elan_vp_configure (int unit, int vfi_index, int active_vp, 
                  source_vp_entry_t *svp, int tpid_enable, bcm_l2gre_port_t  *l2gre_port)
{
    int rv = BCM_E_NONE;
    int cml_default_enable=0, cml_default_new=0, cml_default_move=0;

    /* Set SOURCE_VP */
    soc_SOURCE_VPm_field32_set(unit, svp, CLASS_IDf,
                         l2gre_port->if_class);
    soc_SOURCE_VPm_field32_set(unit, svp, NETWORK_PORTf,
                        (l2gre_port->flags & BCM_L2GRE_PORT_NETWORK) ? 1 : 0);

    if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_TAGGED) {
         soc_SOURCE_VPm_field32_set(unit, svp, SD_TAG_MODEf, 1);
         soc_SOURCE_VPm_field32_set(unit, svp, TPID_ENABLEf, tpid_enable);
    } else {
         soc_SOURCE_VPm_field32_set(unit, svp, SD_TAG_MODEf, 0);
    }
    if (vfi_index == _BCM_L2GRE_VFI_INVALID) {
        soc_SOURCE_VPm_field32_set(unit, svp, 
                                   ENTRY_TYPEf, _BCM_L2GRE_SOURCE_VP_TYPE_INVALID); /* INVALID */
    } else {
        /* Initialize VP parameters */
        soc_SOURCE_VPm_field32_set(unit, svp, 
                                   ENTRY_TYPEf, _BCM_L2GRE_SOURCE_VP_TYPE_VFI); /* VFI Type */
    }
    soc_SOURCE_VPm_field32_set(unit, svp, VFIf, vfi_index);
    rv = _bcm_vp_default_cml_mode_get (unit,
                       &cml_default_enable, &cml_default_new, 
                       &cml_default_move);
    if (rv < 0) {
         return rv;
    }
    if (cml_default_enable) {
        /* Set the CML to default values */
        soc_SOURCE_VPm_field32_set(unit, svp, CML_FLAGS_NEWf, cml_default_new);
        soc_SOURCE_VPm_field32_set(unit, svp, CML_FLAGS_MOVEf, cml_default_move);
    } else {
        /* Set the CML to PVP_CML_SWITCH by default (hw learn and forward) */
        soc_SOURCE_VPm_field32_set(unit, svp, CML_FLAGS_NEWf, 0x8);
        soc_SOURCE_VPm_field32_set(unit, svp, CML_FLAGS_MOVEf, 0x8);
    }
    if (soc_mem_field_valid(unit, SOURCE_VPm, DISABLE_VLAN_CHECKSf)) {
          soc_SOURCE_VPm_field32_set(unit, svp, DISABLE_VLAN_CHECKSf, 0x1);
    }

    BCM_IF_ERROR_RETURN
           (WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, active_vp, svp));

    return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_access_niv_pe_set
 * Purpose:
 *      Set L2GRE Access virtual port which is of NIV/PE type
 * Parameters:
 *      unit    - (IN) Device Number
 *      vp      -  (IN) Access Virtual Port
 *      vfi      -  (IN) Virtial forwarding instance
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_access_niv_pe_set (int unit, int vp, int vfi)
{
    source_vp_entry_t svp;
    int  rv = BCM_E_PARAM; 

        rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
        if (rv < 0) {
            return rv;
        }

        if (vfi == _BCM_L2GRE_VFI_INVALID) {
            soc_SOURCE_VPm_field32_set(unit, &svp, 
                                       ENTRY_TYPEf, _BCM_L2GRE_SOURCE_VP_TYPE_INVALID);
        } else {
            /* Initialize VP parameters */
            soc_SOURCE_VPm_field32_set(unit, &svp, 
                                       ENTRY_TYPEf, _BCM_L2GRE_SOURCE_VP_TYPE_VFI);
        }     
        soc_SOURCE_VPm_field32_set(unit, &svp, VFIf, vfi);
        rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
        return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_access_niv_pe_reset
 * Purpose:
 *      Reset L2GRE Access virtual port which is of NIV/PE type
 * Parameters:
 *      unit    - (IN) Device Number
 *      vp      -  (IN) Access Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_access_niv_pe_reset (int unit, int vp)
{
    source_vp_entry_t svp;
    int  rv = BCM_E_PARAM; 

        rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
        if (rv < 0) {
            return rv;
        }

        /* Initialize VP parameters */
        soc_SOURCE_VPm_field32_set(unit, &svp, 
                                       ENTRY_TYPEf, _BCM_L2GRE_SOURCE_VP_TYPE_VLAN);
        soc_SOURCE_VPm_field32_set(unit, &svp, VFIf, 0);
        rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
        return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_eline_port_add
 * Purpose:
 *      Add L2GRE ELINE port to a VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 *      l2gre_port - (IN/OUT) L2GRE port information (OUT : l2gre_port_id)
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_eline_port_add(int unit, bcm_vpn_t vpn, bcm_l2gre_port_t  *l2gre_port)
{
    int active_vp = 0; /* default VP */
    source_vp_entry_t svp1, svp2;
    uint8 vp_valid_flag = 0;
    int tpid_enable = 0, tpid_index=-1;
    int customer_drop=0;
    int  rv = BCM_E_PARAM; 
    int num_vp=0;
    int vp1=0, vp2=0, vfi_index= -1;


        if ( vpn != BCM_L2GRE_VPN_INVALID) {
            _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELINE,  vpn);
             if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
                return BCM_E_NOT_FOUND;
             }
        } else {
             vfi_index = _BCM_L2GRE_VFI_INVALID;
        }

        num_vp = soc_mem_index_count(unit, SOURCE_VPm);

        if ( vpn != BCM_L2GRE_VPN_INVALID) {

             /* ---- Read in current table values for VP1 and VP2 ----- */
             (void) _bcm_tr3_l2gre_eline_vp_map_get (unit, vfi_index,  &vp1,  &vp2);

            if ( 0 < _bcm_vp_used_get(unit, vp1, _bcmVpTypeL2Gre)) {
                BCM_IF_ERROR_RETURN (
                   READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp1, &svp1));
                if (soc_SOURCE_VPm_field32_get(unit, &svp1, ENTRY_TYPEf) == 0x1) {
                    vp_valid_flag |= 0x1;  /* -- VP1 Valid ----- */
                }
            }

            if (0 < _bcm_vp_used_get(unit, vp2, _bcmVpTypeL2Gre)) {
                BCM_IF_ERROR_RETURN (
                   READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp2, &svp2));
                if (soc_SOURCE_VPm_field32_get(unit, &svp2, ENTRY_TYPEf) == 0x1) {
                    vp_valid_flag |= 0x2;        /* -- VP2 Valid ----- */
                }
            }

            if (l2gre_port->flags & BCM_L2GRE_PORT_REPLACE) {
                active_vp = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port->l2gre_port_id);
                if (active_vp == -1) {
                   return BCM_E_PARAM;
                }
                if (!_bcm_vp_used_get(unit, active_vp, _bcmVpTypeL2Gre)) {
                     return BCM_E_NOT_FOUND;
                }

                /* Decrement old-port's nexthop count */
                rv = _bcm_tr3_l2gre_port_nh_cnt_dec(unit, active_vp);
                if(rv < 0) {
                    return rv;
                }

                /* Decrement old-port's VP count */
                rv = _bcm_tr3_l2gre_port_cnt_update(unit, l2gre_port->l2gre_port_id,
                                                     active_vp, FALSE);
                if (rv < 0) {
                    return rv;
                }
            }
        }

        switch (vp_valid_flag) {

              case 0x0: /* No VP is valid */
                             if (l2gre_port->flags & BCM_L2GRE_PORT_REPLACE) {
                                  return BCM_E_NOT_FOUND;
                             }

                             if (l2gre_port->flags & BCM_L2GRE_PORT_WITH_ID) {
                                vp1 = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port->l2gre_port_id);
                                if (vp1 == -1) {
                                     return BCM_E_PARAM;
                                }

                                if (_bcm_vp_used_get(unit, vp1, _bcmVpTypeL2Gre)) {
                                     return BCM_E_EXISTS;
                                }
                                if (vp1 >= num_vp) {
                                     return (BCM_E_BADID);
                                }
                                BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp1, _bcmVpTypeL2Gre));

                             } else {

                                /* No entries are used, allocate a new VP index for VP1 */
                                rv = _bcm_vp_alloc(unit, 0, (num_vp - 1), 1, SOURCE_VPm, _bcmVpTypeL2Gre, &vp1);
                                if (rv < 0) {
                                  return rv;
                                }
                                /* Allocate a new VP index for VP2 */
                                 rv = _bcm_vp_alloc(unit, 0, (num_vp - 1), 1, SOURCE_VPm, _bcmVpTypeL2Gre, &vp2);
                                  if (rv < 0) {
                                     (void) _bcm_vp_free(unit, _bcmVpTypeL2Gre, 1, vp1);
                                      return rv;
                                  }
                             }

                             active_vp = vp1;
                             vp_valid_flag = 1;
                             sal_memset(&svp1, 0, sizeof(source_vp_entry_t));
                             sal_memset(&svp2, 0, sizeof(source_vp_entry_t));
                             (void) _bcm_tr3_l2gre_eline_vp_map_set(unit, vfi_index, vp1, vp2);
                             break;


         case 0x1:    /* Only VP1 is valid */	
                             if (l2gre_port->flags & BCM_L2GRE_PORT_REPLACE) {
                                  if (active_vp != vp1) {
                                       return BCM_E_NOT_FOUND;
                                  }
                             } else if (l2gre_port->flags & BCM_L2GRE_PORT_WITH_ID) {
                                vp2 = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port->l2gre_port_id);
                                if (vp2 == -1) {
                                     return BCM_E_PARAM;
                                }

                                if (_bcm_vp_used_get(unit, vp2, _bcmVpTypeL2Gre)) {
                                     return BCM_E_EXISTS;
                                }
                                if (vp2 >= num_vp) {
                                     return (BCM_E_BADID);
                                }
                                BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp2, _bcmVpTypeL2Gre));

                             } else {
                                  active_vp = vp2;
                                  vp_valid_flag = 3;
                                  sal_memset(&svp2, 0, sizeof(source_vp_entry_t));
                             }

                             (void) _bcm_tr3_l2gre_eline_vp_map_set(unit, vfi_index, vp1, vp2);
                             break;



         case 0x2: /* Only VP2 is valid */
                             if (l2gre_port->flags & BCM_L2GRE_PORT_REPLACE) {
                                  if (active_vp != vp2) {
                                       return BCM_E_NOT_FOUND;
                                  }
                             } else if (l2gre_port->flags & BCM_L2GRE_PORT_WITH_ID) {
                                vp1 = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port->l2gre_port_id);
                                if (vp1 == -1) {
                                     return BCM_E_PARAM;
                                }

                                if (_bcm_vp_used_get(unit, vp1, _bcmVpTypeL2Gre)) {
                                     return BCM_E_EXISTS;
                                }
                                if (vp1 >= num_vp) {
                                     return (BCM_E_BADID);
                                }
                                BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp1, _bcmVpTypeL2Gre));

                             } else {
                                  active_vp = vp1;
                                  vp_valid_flag = 3;
                                   sal_memset(&svp1, 0, sizeof(source_vp_entry_t));
                             }

                             (void) _bcm_tr3_l2gre_eline_vp_map_set(unit, vfi_index, vp1, vp2);
                             break;


              
         case 0x3: /* VP1 and VP2 are valid */
                              if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
                                   return BCM_E_FULL;
                              }
                             break;
       }

       if (active_vp == -1) {
           return BCM_E_CONFIG;
       }

        /* Set L2GRE port ID */
        if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
            BCM_GPORT_L2GRE_PORT_ID_SET(l2gre_port->l2gre_port_id, active_vp);
        }

       /* ======== Configure Next-Hop Properties ========== */
       customer_drop = (l2gre_port->flags & BCM_L2GRE_PORT_DROP) ? 1 : 0;
       rv = _bcm_tr3_l2gre_port_nh_add(unit, l2gre_port, active_vp, vpn, customer_drop);
       if (rv < 0) {
            if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
                (void) _bcm_vp_free(unit, _bcmVfiTypeL2Gre, 1, active_vp);
            }
            return rv;
       }

       /* ======== Configure Service-Tag Properties =========== */
       if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_TAGGED) {
            rv = _bcm_fb2_outer_tpid_lkup(unit, l2gre_port->egress_service_tpid,
                                           &tpid_index);
            if (rv < 0) {
                goto l2gre_cleanup;
            }
            tpid_enable = (1 << tpid_index);
       }

       /* ======== Configure SVP Properties ========== */
       if (active_vp == vp1) {
           rv  = _bcm_esw_l2gre_eline_vp_configure (unit, vfi_index, active_vp, &svp1, 
                                                    tpid_enable, l2gre_port);
       } else if (active_vp == vp2) {
           rv  = _bcm_esw_l2gre_eline_vp_configure (unit, vfi_index, active_vp, &svp2, 
                                                    tpid_enable, l2gre_port);
       }
       if (rv < 0) {
            goto l2gre_cleanup;
       }

        rv = _bcm_esw_l2gre_match_add(unit, l2gre_port, active_vp, vpn);
        if (rv < 0) {
            goto l2gre_cleanup;
        }

        /* Increment new-port's VP count */
        rv = _bcm_tr3_l2gre_port_cnt_update(unit, l2gre_port->l2gre_port_id, active_vp, TRUE);
        if (rv < 0) {
            goto l2gre_cleanup;
        }

        /* Set L2GRE port ID */
        if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
            BCM_GPORT_L2GRE_PORT_ID_SET(l2gre_port->l2gre_port_id, active_vp);
        }

l2gre_cleanup:
        if (rv < 0) {
            if (tpid_enable) {
                (void) _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
            }
            /* Free the VP's */
            if (vp_valid_flag) {
                 (void) _bcm_vp_free(unit, _bcmVpTypeL2Gre, 1, vp1);
                 (void) _bcm_vp_free(unit, _bcmVpTypeL2Gre, 1, vp2);
            }
        }
        return rv;
}

/*
 * Function:
 *     _bcm_tr3_l2gre_elan_port_add
 * Purpose:
 *      Add L2GRE ELAN port to a VPN
 * Parameters:
 *   unit - (IN) Device Number
 *   vpn - (IN) VPN instance ID
 *   l2gre_port - (IN/OUT) l2gre port information (OUT : l2gre_port_id)
 * Returns:
 *     BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_elan_port_add(int unit, bcm_vpn_t vpn, bcm_l2gre_port_t *l2gre_port)
{
    int vp, num_vp, vfi=0;
    source_vp_entry_t svp;
    int tpid_enable = 0, tpid_index=-1;
    int drop, rv = BCM_E_PARAM;

    if ( vpn != BCM_L2GRE_VPN_INVALID) {
        _BCM_L2GRE_VPN_GET(vfi, _BCM_L2GRE_VPN_TYPE_ELAN,  vpn);
        if (!_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeL2Gre)) {
            return BCM_E_NOT_FOUND;
        }
    } else {
        vfi = _BCM_L2GRE_VFI_INVALID;
    }
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

   /* ======== Allocate/Get Virtual_Port =============== */
    if (l2gre_port->flags & BCM_L2GRE_PORT_REPLACE) {
        if (BCM_GPORT_IS_NIV_PORT(l2gre_port->l2gre_port_id) ||
            BCM_GPORT_IS_EXTENDER_PORT(l2gre_port->l2gre_port_id)) {
            if (BCM_GPORT_IS_NIV_PORT(l2gre_port->l2gre_port_id)) {
               vp = BCM_GPORT_NIV_PORT_ID_GET((l2gre_port->l2gre_port_id));
            } else if (BCM_GPORT_IS_EXTENDER_PORT(l2gre_port->l2gre_port_id)) {
               vp = BCM_GPORT_EXTENDER_PORT_ID_GET((l2gre_port->l2gre_port_id));
            }
            rv = _bcm_tr3_l2gre_access_niv_pe_set (unit, vp, vfi);
            if (BCM_SUCCESS(rv)) {
               /* Set L2GRE port ID */
               BCM_GPORT_L2GRE_PORT_ID_SET(l2gre_port->l2gre_port_id, vp);
               rv = _bcm_vp_used_set(unit, vp, _bcmVpTypeL2Gre);
            }
            return rv;
        } 

        vp = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port->l2gre_port_id);
        if (vp == -1) {
            return BCM_E_PARAM;
        }

        if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
            return BCM_E_NOT_FOUND;
        }
        rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
        if (rv < 0) {
            return rv;
        }
        /* Decrement old-port's nexthop count */
        rv = _bcm_tr3_l2gre_port_nh_cnt_dec(unit, vp);
        if(rv < 0) {
            return rv;
        }
        /* Decrement old-port's VP count */
        rv = _bcm_tr3_l2gre_port_cnt_update(unit, l2gre_port->l2gre_port_id, vp, FALSE);
        if (rv < 0) {
            return rv;
        }

    } else if (l2gre_port->flags & BCM_L2GRE_PORT_WITH_ID ) {
        if (!BCM_GPORT_IS_L2GRE_PORT(l2gre_port->l2gre_port_id)) {
            return (BCM_E_BADID);
        }

        vp = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port->l2gre_port_id);
        if (vp == -1) {
            return BCM_E_PARAM;
        }
        if (vp >= num_vp) {
            return (BCM_E_BADID);
        }
        BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp, _bcmVpTypeL2Gre));
        sal_memset(&svp, 0, sizeof(source_vp_entry_t));
    } else {
        /* allocate a new VP index */
        rv = _bcm_vp_alloc(unit, 0, (num_vp - 1), 1, SOURCE_VPm, _bcmVpTypeL2Gre, &vp);
        if (rv < 0) {
           return rv;
        }
        sal_memset(&svp, 0, sizeof(source_vp_entry_t));
        BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp, _bcmVpTypeL2Gre));
    }

    /* ======== Configure Next-Hop/DVP Properties ========== */
    drop = (l2gre_port->flags & BCM_L2GRE_PORT_DROP) ? 1 : 0;
    rv = _bcm_tr3_l2gre_port_nh_add(unit, l2gre_port, vp, vpn, drop);
    if (rv < 0) {
        if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
            (void) _bcm_vp_free(unit, _bcmVfiTypeL2Gre, 1, vp);
        }
        return rv;
    }

    /* ======== Configure Service-Tag Properties =========== */
    if (l2gre_port->flags & BCM_L2GRE_PORT_SERVICE_TAGGED) {
        rv = _bcm_fb2_outer_tpid_lkup(unit, l2gre_port->egress_service_tpid,
                                           &tpid_index);
        if (rv < 0) {
            goto l2gre_cleanup;
        }
        if (tpid_index != -1) {
            tpid_enable = (1 << tpid_index);
        }
    }

    /* ======== Configure SVP Properties ========== */
    rv  = _bcm_esw_l2gre_elan_vp_configure (unit, vfi, vp, &svp, 
                               tpid_enable, l2gre_port);
    if (rv < 0) {
        goto l2gre_cleanup;
    }

    if (rv == BCM_E_NONE) {
        /* Set L2GRE port ID */
        BCM_GPORT_L2GRE_PORT_ID_SET(l2gre_port->l2gre_port_id, vp);
    }

    /* Increment new-port's VP count */
    rv = _bcm_tr3_l2gre_port_cnt_update(unit, l2gre_port->l2gre_port_id, vp, TRUE);
    if (rv < 0) {
        goto l2gre_cleanup;
    }

    /* ======== Configure match to VP Properties ========== */
    rv = _bcm_esw_l2gre_match_add(unit, l2gre_port, vp, vpn);
    if (rv < 0) {
        goto l2gre_cleanup;
    }

  l2gre_cleanup:
    if (rv < 0) {
        if (tpid_enable) {
            (void) _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
        }
        if (!(l2gre_port->flags & BCM_L2GRE_PORT_REPLACE)) {
            (void) _bcm_vp_free(unit, _bcmVpTypeL2Gre, 1, vp);
            _bcm_tr3_l2gre_port_nh_delete(unit, vpn, vp);
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_l2gre_eline_port_delete
 * Purpose:
 *      Delete L2GRE ELINE port from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      l2gre_port_id - (IN) l2gre port information
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_tr3_l2gre_eline_port_delete(int unit, bcm_vpn_t vpn, int active_vp)
{
    source_vp_entry_t svp;
    int vp1=0, vp2=0, vfi_index= -1;
    int rv = BCM_E_UNAVAIL;
    bcm_gport_t  l2gre_port_id;

    if ( vpn != BCM_L2GRE_VPN_INVALID) {
         _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELINE,  vpn);
         if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
            return BCM_E_NOT_FOUND;
         }
    } else {
         vfi_index = _BCM_L2GRE_VFI_INVALID;
    }

    if (!_bcm_vp_used_get(unit, active_vp, _bcmVpTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }

    /* Set L2GRE port ID */
    BCM_GPORT_L2GRE_PORT_ID_SET(l2gre_port_id, active_vp);

    /* Delete the next-hop info */
    rv = _bcm_tr3_l2gre_port_nh_delete(unit, vpn, active_vp);
    if ( rv < 0 ) {
        if (rv != BCM_E_NOT_FOUND) {
            return rv;
        } else {
                rv = 0;
        }
    }

    rv = _bcm_esw_l2gre_match_delete(unit, active_vp);
    if ( rv < 0 ) {
        if (rv != BCM_E_NOT_FOUND) {
             return rv;
        } else {
             rv = BCM_E_NONE;
        }
    }

     /* ---- Read in current table values for VP1 and VP2 ----- */
     (void) _bcm_tr3_l2gre_eline_vp_map_get (unit, vfi_index,  &vp1,  &vp2);

    /* If the other port is valid, point it to itself */
    if (active_vp == vp1) {
        if (BCM_SUCCESS(rv)) {
            rv = _bcm_tr3_l2gre_eline_vp_map_clear (unit, vfi_index, active_vp, 0);
        }
    } else if (active_vp == vp2) {
        if (BCM_SUCCESS(rv)) {
            rv = _bcm_tr3_l2gre_eline_vp_map_clear (unit, vfi_index, 0, active_vp);
        }
    }

    if (rv >= 0) {

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_mem_is_valid(unit, SOURCE_VP_2m)) {
           source_vp_2_entry_t svp_2_entry;
           int network_port_flag=0;
       
            /* Check for Network-Port */
           BCM_IF_ERROR_RETURN (READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, active_vp, &svp));
           network_port_flag = soc_SOURCE_VPm_field32_get(unit, &svp, NETWORK_PORTf);
           if (!network_port_flag) {
                /* Clear SOURCE_VP_2 entry */
                sal_memset(&svp_2_entry, 0, sizeof(source_vp_2_entry_t));
                BCM_IF_ERROR_RETURN
                    (WRITE_SOURCE_VP_2m(unit, MEM_BLOCK_ALL, active_vp, &svp_2_entry));
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */

        /* Invalidate the VP being deleted */
        sal_memset(&svp, 0, sizeof(source_vp_entry_t));
        rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, active_vp, &svp);
        if (BCM_SUCCESS(rv)) {
             rv = _bcm_tr3_l2gre_egress_dvp_reset(unit, active_vp);
             if (rv < 0) {
                 return rv;
             }
             rv = _bcm_tr3_l2gre_ingress_dvp_reset(unit, active_vp);
        }
    }

    if (rv < 0) {
        return rv;
    }

    /* Decrement port's VP count */
    rv = _bcm_tr3_l2gre_port_cnt_update(unit, l2gre_port_id, active_vp, FALSE);
    if (rv < 0) {
        return rv;
    }

    /* Free the VP */
    (void) _bcm_vp_free(unit, _bcmVfiTypeL2Gre, 1, active_vp);
    return rv;

}

/*
 * Function:
 *      _bcm_tr3_l2gre_elan_port_delete
 * Purpose:
 *      Delete L2GRE ELAN port from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      l2gre_port_id - (IN) l2gre port information
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_tr3_l2gre_elan_port_delete(int unit, bcm_vpn_t vpn, int vp)
{
    source_vp_entry_t svp;
    int rv = BCM_E_UNAVAIL;
    int vfi_index= -1;
    bcm_gport_t l2gre_port_id;
    int network_port_flag=0;
    bcm_l2gre_port_t  l2gre_port;

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }

    /* Check for Network-Port */
    BCM_IF_ERROR_RETURN (READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp));
    network_port_flag = soc_SOURCE_VPm_field32_get(unit, &svp, NETWORK_PORTf);

    if ( vpn != BCM_L2GRE_VPN_INVALID) {
         if (!network_port_flag) { /* Check VPN only for Access VP */
             _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELAN,  vpn);
             if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
                return BCM_E_NOT_FOUND;
             }
             if (_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv) ||
                  _bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                   (void) _bcm_vp_free(unit, _bcmVpTypeL2Gre, 1, vp);
                   rv = _bcm_tr3_l2gre_access_niv_pe_reset (unit, vp);
                   return rv;
             }
         }
    }

    /* Set L2GRE port ID */
    BCM_GPORT_L2GRE_PORT_ID_SET(l2gre_port_id, vp);
    bcm_l2gre_port_t_init(&l2gre_port);
    BCM_IF_ERROR_RETURN(_bcm_tr3_l2gre_port_get(unit, vpn, vp, &l2gre_port));

    /* Delete the next-hop info */
    rv = _bcm_tr3_l2gre_port_nh_delete(unit, vpn, vp);

    if ( rv < 0 ) {
        if (rv != BCM_E_NOT_FOUND) {
            return rv;
        } else {
                rv = 0;
        }
    }

    rv = _bcm_esw_l2gre_match_delete(unit, vp);
    if ( rv < 0 ) {
        if (rv != BCM_E_NOT_FOUND) {
            return rv;
        }
    }

    /* Clear the SVP and DVP table entries */
    sal_memset(&svp, 0, sizeof(source_vp_entry_t));
    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
    if (rv < 0) {
        return rv;
    }

    if (!network_port_flag) {
#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_mem_is_valid(unit, SOURCE_VP_2m)) {
           source_vp_2_entry_t svp_2_entry;
       
                /* Clear SOURCE_VP_2 entry */
                sal_memset(&svp_2_entry, 0, sizeof(source_vp_2_entry_t));
                BCM_IF_ERROR_RETURN
                    (WRITE_SOURCE_VP_2m(unit, MEM_BLOCK_ALL, vp, &svp_2_entry));
        }
#endif /* BCM_TRIDENT2_SUPPORT */
    }

    rv = _bcm_tr3_l2gre_egress_dvp_reset(unit, vp);
    if (rv < 0) {
        return rv;
    }

    rv = _bcm_tr3_l2gre_ingress_dvp_reset(unit, vp);
    if (rv < 0) {
        return rv;
    }

    /* Decrement port's VP count */
    rv = _bcm_tr3_l2gre_port_cnt_update(unit, l2gre_port_id, vp, FALSE);
    if (rv < 0) {
        return rv;
    }

    /* Free the VP */
    (void) _bcm_vp_free(unit, _bcmVfiTypeL2Gre, 1, vp);
    return rv;
}


/*
 * Function:
 *      _bcm_tr3_l2gre_port_get
 * Purpose:
 *      Get L2GRE port information from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      l2gre_port_id - (IN) l2gre port information
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_tr3_l2gre_port_get(int unit, bcm_vpn_t vpn, int vp, bcm_l2gre_port_t  *l2gre_port)
{
    int i, tpid_enable = 0, rv = BCM_E_NONE;
    source_vp_entry_t svp;
    ing_dvp_table_entry_t dvp;
    int split_horizon_port_flag=0;
    int egress_tunnel_flag=0;

    /* Initialize the structure */
    bcm_l2gre_port_t_init(l2gre_port);
    BCM_GPORT_L2GRE_PORT_ID_SET(l2gre_port->l2gre_port_id, vp);

    /* Check for Network-Port */
    BCM_IF_ERROR_RETURN (READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp));
    split_horizon_port_flag = soc_SOURCE_VPm_field32_get(unit, &svp, NETWORK_PORTf);

    if ( vpn != BCM_L2GRE_VPN_INVALID) {
         if (!split_horizon_port_flag) { /* Check VPN only for Access VP */
             if (_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv) ||
                  _bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                   return BCM_E_NONE;
             }
         }
    }

    /* Get the match parameters */
    rv = _bcm_tr3_l2gre_match_get(unit, l2gre_port, vp);
    BCM_IF_ERROR_RETURN(rv);

    /* Get the next-hop parameters */
    rv = _bcm_tr3_l2gre_port_nh_get(unit, vpn, vp, l2gre_port);
    BCM_IF_ERROR_RETURN(rv);

    /* Get Tunnel index */
    rv = _bcm_tr3_l2gre_egress_dvp_get(unit, vp, l2gre_port);
    BCM_IF_ERROR_RETURN(rv);

    /* Fill in SVP parameters */
    l2gre_port->if_class = soc_SOURCE_VPm_field32_get(unit, &svp, CLASS_IDf);
    if (split_horizon_port_flag) {
        l2gre_port->flags |= BCM_L2GRE_PORT_NETWORK;
    }

    if (soc_SOURCE_VPm_field32_get(unit, &svp, SD_TAG_MODEf)) {
        tpid_enable = soc_SOURCE_VPm_field32_get(unit, &svp, TPID_ENABLEf);
        if (tpid_enable) {
            l2gre_port->flags |= BCM_L2GRE_PORT_SERVICE_TAGGED;
            for (i = 0; i < 4; i++) {
                if (tpid_enable & (1 << i)) {
                    _bcm_fb2_outer_tpid_entry_get(unit, &l2gre_port->egress_service_tpid, i);
                }
            }
        }
    }

    /* Check for Egress-Tunnel */
    BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
    egress_tunnel_flag = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NETWORK_PORTf);
    if (egress_tunnel_flag) {
        l2gre_port->flags |= BCM_L2GRE_PORT_EGRESS_TUNNEL;
    }
    return rv;
}


/*
 * Function:
 *      _bcm_tr3_l2gre_default_port_add
 * Purpose:
 *      Add L2GRE port to a VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 *      l2gre_port - (IN/OUT) l2gre_port information (OUT : l2gre_port_id)
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_tr3_l2gre_default_port_add(int unit, bcm_l2gre_port_t  *l2gre_port)
{
    source_vp_entry_t svp;
    int rv = BCM_E_PARAM, vp=0, num_vp=0;
    int cml_default_enable=0, cml_default_new=0, cml_default_move=0;

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

   /* ======== Allocate/Get Virtual_Port =============== */
    if (l2gre_port->flags & BCM_L2GRE_PORT_REPLACE) {
        vp = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port->l2gre_port_id);
        if (vp == -1) {
            return BCM_E_PARAM;
        }

        if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
            return BCM_E_NOT_FOUND;
        }
        rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
        if (rv < 0) {
            return rv;
        }

        /* Decrement old-port's nexthop count */
        rv = _bcm_tr3_l2gre_port_nh_cnt_dec(unit, vp);
        if(rv < 0) {
            return rv;
        }

        /* Decrement old-port's VP count */
        rv = _bcm_tr3_l2gre_port_cnt_update(unit, l2gre_port->l2gre_port_id, vp, FALSE);
        if (rv < 0) {
            return rv;
        }

    } else if (l2gre_port->flags & BCM_L2GRE_PORT_WITH_ID ) {
       if (!BCM_GPORT_IS_L2GRE_PORT(l2gre_port->l2gre_port_id)) {
            return (BCM_E_BADID);
        }

        vp = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port->l2gre_port_id);
        if (vp == -1) {
            return BCM_E_PARAM;
        }
        if (vp >= num_vp) {
            return (BCM_E_BADID);
        }
        BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp, _bcmVpTypeL2Gre));
        sal_memset(&svp, 0, sizeof(source_vp_entry_t));
    } else {
       rv = _bcm_vp_alloc(unit, 0, (num_vp - 1), 1, SOURCE_VPm, _bcmVpTypeL2Gre, &vp);
       if (rv < 0) {
           return rv;
       }
        sal_memset(&svp, 0, sizeof(source_vp_entry_t));
        BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp, _bcmVpTypeL2Gre));
    }

    /* ======== Configure SVP/DVP Properties ========== */
    soc_SOURCE_VPm_field32_set(unit, &svp, CLASS_IDf, 
                               l2gre_port->if_class);
    soc_SOURCE_VPm_field32_set(unit, &svp, NETWORK_PORTf,
                               (l2gre_port->flags & BCM_L2GRE_PORT_NETWORK) ? 1 : 0);

     soc_SOURCE_VPm_field32_set(unit, &svp, 
                                       ENTRY_TYPEf, _BCM_L2GRE_SOURCE_VP_TYPE_VFI);

        rv = _bcm_vp_default_cml_mode_get (unit, 
                           &cml_default_enable, &cml_default_new, 
                           &cml_default_move);
         if (rv < 0) {
            return rv;
         }
        if (cml_default_enable) {
            /* Set the CML to default values */
            soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_NEWf, cml_default_new);
            soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_MOVEf, cml_default_move);
        } else {
            /* Set the CML to PVP_CML_SWITCH by default (hw learn and forward) */
            soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_NEWf, 0x8);
            soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_MOVEf, 0x8);
        }
        if (soc_mem_field_valid(unit, SOURCE_VPm, DISABLE_VLAN_CHECKSf)) {
              soc_SOURCE_VPm_field32_set(unit, &svp, DISABLE_VLAN_CHECKSf, 1);
        }

    /* Configure L2GRE_DEFAULT_NETWORK_SVPr.SVP */
    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
    BCM_IF_ERROR_RETURN(rv);

    BCM_GPORT_L2GRE_PORT_ID_SET(l2gre_port->l2gre_port_id, vp);

    /* Increment new-port's VP count */
    rv = _bcm_tr3_l2gre_port_cnt_update(unit, l2gre_port->l2gre_port_id, vp, TRUE);
    BCM_IF_ERROR_RETURN(rv);

    return (soc_reg_field32_modify(unit, L2GRE_DEFAULT_NETWORK_SVPr, 
             REG_PORT_ANY, SVPf, vp));

}

/*
 * Function:
 *      _bcm_tr3_l2gre_default_port_delete
 * Purpose:
 *      Delete L2GRE Default port from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      l2gre_port_id - (IN) L2gre port information
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_tr3_l2gre_default_port_delete(int unit, bcm_vpn_t vpn, int vp)
{
    source_vp_entry_t svp;
    int rv = BCM_E_UNAVAIL;
    int vfi_index= -1;
    bcm_gport_t l2gre_port_id;

    if ( vpn != BCM_L2GRE_VPN_INVALID) {
         _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELAN,  vpn);
         if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
            return BCM_E_NOT_FOUND;
         }
    }

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }

    /* Set L2GRE port ID */
    BCM_GPORT_L2GRE_PORT_ID_SET(l2gre_port_id, vp);

    /* Clear the SVP and DVP table entries */
    sal_memset(&svp, 0, sizeof(source_vp_entry_t));
    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
    if (rv < 0) {
        return rv;
    }

    /* Decrement port's VP count */
    rv = _bcm_tr3_l2gre_port_cnt_update(unit, l2gre_port_id, vp, FALSE);
    if (rv < 0) {
        return rv;
    }

    /* Free the VP */
    (void) _bcm_vp_free(unit, _bcmVpTypeL2Gre, 1, vp);
    return rv;

}

/*
 * Function:
 *      bcm_tr3_l2gre_port_add
 * Purpose:
 *      Add L2GRE port to a VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 *      l2gre_port - (IN/OUT) l2gre_port information (OUT : l2gre_port_id)
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr3_l2gre_port_add(int unit, bcm_vpn_t vpn, bcm_l2gre_port_t  *l2gre_port)
{
    int mode, rv = BCM_E_PARAM; 
    uint8 isEline=0xFF;
    int vfi_index=0;

    BCM_IF_ERROR_RETURN(bcm_xgs3_l3_egress_mode_get(unit, &mode));
    if (!mode) {
        LOG_INFO(BSL_LS_BCM_L3,
                 (BSL_META_U(unit,
                             "L3 egress mode must be set first\n")));
        return BCM_E_DISABLED;
    }

    if (l2gre_port->flags & BCM_L2GRE_PORT_NETWORK) {         
         _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELINE,  vpn);
        if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
            isEline = 0x0; /* ELAN Dont care VPN Case */
        }
    }

    if ((vpn != BCM_L2GRE_VPN_INVALID) && (isEline != 0x0)) {
         BCM_IF_ERROR_RETURN 
              (_bcm_tr3_l2gre_vpn_is_eline(unit, vpn, &isEline));
    }
    
    if (l2gre_port->flags & BCM_L2GRE_PORT_DEFAULT) {
       if (soc_feature(unit, soc_feature_l2gre_default_tunnel)) {
                BCM_IF_ERROR_RETURN(
                     _bcm_tr3_l2gre_default_port_add(unit, l2gre_port));
       }
    }

    if (isEline == 0x1 ) {
        rv = _bcm_tr3_l2gre_eline_port_add(unit, vpn, l2gre_port);
    } else if (isEline == 0x0 ) {
        rv = _bcm_tr3_l2gre_elan_port_add(unit, vpn, l2gre_port);
    }

    return rv;
}


/*
 * Function:
 *      bcm_tr3_l2gre_port_delete
 * Purpose:
 *      Delete L2GRE port from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      l2gre_port_id - (IN) l2gre port information
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr3_l2gre_port_delete(int unit, bcm_vpn_t vpn, bcm_gport_t l2gre_port_id)
{
    int vp;
    int rv = BCM_E_UNAVAIL;
    uint32 reg_val=0;
    uint8 isEline=0;
    uint32 stat_counter_id;
    int num_ctr = 0; 

    /* Check for Valid Vpn */
    BCM_IF_ERROR_RETURN(_bcm_tr3_l2gre_vpn_is_valid(unit, vpn));

    vp = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port_id);
    if (vp == -1) {
        return BCM_E_PARAM;
    }

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }

    /* Check counters before delete port */
    if (bcm_esw_l2gre_stat_id_get(unit, l2gre_port_id, vpn,
                   bcmL2greOutPackets, &stat_counter_id) == BCM_E_NONE) { 
        num_ctr++;
    } 
    if (bcm_esw_l2gre_stat_id_get(unit, l2gre_port_id, vpn,
                   bcmL2greInPackets, &stat_counter_id) == BCM_E_NONE) {
        num_ctr++;
    }

    if (num_ctr != 0) {
        return BCM_E_RESOURCE;/* Need detach counters before delete port */
    }

    /* Check for L2GRE default port */
    if (soc_feature(unit, soc_feature_l2gre_default_tunnel)) {
        BCM_IF_ERROR_RETURN(READ_L2GRE_DEFAULT_NETWORK_SVPr(unit, &reg_val));
        if (vp == soc_reg_field_get(unit, L2GRE_DEFAULT_NETWORK_SVPr,
                                         reg_val, SVPf)) {
            rv = _bcm_tr3_l2gre_default_port_delete(unit, vpn, vp);
            return rv;
        }
    }

    BCM_IF_ERROR_RETURN 
         (_bcm_tr3_l2gre_vpn_is_eline(unit, vpn, &isEline));

    if (isEline == 0x1 ) {
       rv = _bcm_tr3_l2gre_eline_port_delete(unit, vpn, vp);
    } else if (isEline == 0x0 ) {
       rv = _bcm_tr3_l2gre_elan_port_delete(unit, vpn, vp);
    }
    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_port_delete_all
 * Purpose:
 *      Delete all L2GRE ports from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_l2gre_port_delete_all(int unit, bcm_vpn_t vpn)
{
    int rv = BCM_E_NONE;
    int vfi_index;
    int vp1 = 0, vp2 = 0;
    uint8 isEline=0xFF;
    bcm_gport_t l2gre_port_id;
    uint32 reg_val=0;

    BCM_IF_ERROR_RETURN 
         (_bcm_tr3_l2gre_vpn_is_eline(unit, vpn, &isEline));

    if (isEline == 0x1 ) {
         if ( vpn != BCM_L2GRE_VPN_INVALID) {
              _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELINE,  vpn);
              if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
                   return BCM_E_NOT_FOUND;
              }
         } else {
              vfi_index = _BCM_L2GRE_VFI_INVALID;
         }

         /* ---- Read in current table values for VP1 and VP2 ----- */
         (void) _bcm_tr3_l2gre_eline_vp_map_get (unit, vfi_index,  &vp1,  &vp2);
         if (vp1 != 0) {
              rv = _bcm_tr3_l2gre_eline_port_delete(unit, vpn, vp1);
              BCM_IF_ERROR_RETURN(rv);
         }
         if (vp2 != 0) {
              rv = _bcm_tr3_l2gre_eline_port_delete(unit, vpn, vp2);
              BCM_IF_ERROR_RETURN(rv);
         }
    } else if (isEline == 0x0 ) {
        uint32 vfi, vp, num_vp;
        source_vp_entry_t svp;

        _BCM_L2GRE_VPN_GET(vfi, _BCM_L2GRE_VPN_TYPE_ELAN,  vpn);
        if (!_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeL2Gre)) {
            rv =  BCM_E_NOT_FOUND;
                return rv;
        }
        num_vp = soc_mem_index_count(unit, SOURCE_VPm);
        for (vp = 0; vp < num_vp; vp++) {
            /* Check for the validity of the VP */
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
                continue;
            }
            rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
            if (rv < 0) {
                return rv;
            }
            if (soc_feature(unit, soc_feature_l2gre_default_tunnel)) {
                BCM_IF_ERROR_RETURN(READ_L2GRE_DEFAULT_NETWORK_SVPr(unit, &reg_val));
                if (vp == soc_reg_field_get(unit, L2GRE_DEFAULT_NETWORK_SVPr,
                                         reg_val, SVPf)) {
                    rv = _bcm_tr3_l2gre_default_port_delete(unit, vpn, vp);
                    if (rv < 0) {
                        return rv;
                    }   
                }
            }
            if ((soc_SOURCE_VPm_field32_get(unit, &svp, ENTRY_TYPEf) == 0x1) &&
                (vfi == soc_SOURCE_VPm_field32_get(unit, &svp, VFIf))) {
                   /* Set L2GRE port ID */
                   BCM_GPORT_L2GRE_PORT_ID_SET(l2gre_port_id, vp);
                   rv = bcm_tr3_l2gre_port_delete(unit, vpn, l2gre_port_id);
                   if (rv < 0) {
                      return rv;
                   }
            }
        }
    }
    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_port_get
 * Purpose:
 *      Get an L2GRE port from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      l2gre_port  - (IN/OUT) l2gre port information
 */
int
bcm_tr3_l2gre_port_get(int unit, bcm_vpn_t vpn, bcm_l2gre_port_t *l2gre_port)
{
    int vp;
    int rv = BCM_E_NONE;

    /* Check for Valid Vpn */
    BCM_IF_ERROR_RETURN(_bcm_tr3_l2gre_vpn_is_valid(unit, vpn));

    vp = BCM_GPORT_L2GRE_PORT_ID_GET(l2gre_port->l2gre_port_id);
    if (vp == -1) {
        return BCM_E_PARAM;
    }

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
        return BCM_E_NOT_FOUND;
    }

    rv = _bcm_tr3_l2gre_port_get(unit, vpn, vp, l2gre_port);
    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_port_get_all
 * Purpose:
 *      Get all L2GRE ports from a VPN
 * Parameters:
 *      unit     - (IN) Device Number
 *      vpn      - (IN) VPN instance ID
 *      port_max   - (IN) Maximum number of interfaces in array
 *      port_array - (OUT) Array of L2GRE ports
 *      port_count - (OUT) Number of interfaces returned in array
 *
 */
int
bcm_tr3_l2gre_port_get_all(int unit, bcm_vpn_t vpn, int port_max,
                         bcm_l2gre_port_t *port_array, int *port_count)
{
    int vp, rv = BCM_E_NONE;
    int vfi_index;
    int vp1 = 0, vp2 = 0;
    uint8 isEline=0xFF;

    BCM_IF_ERROR_RETURN 
         (_bcm_tr3_l2gre_vpn_is_eline(unit, vpn, &isEline));

    *port_count = 0;

    if (isEline == 0x1 ) {
         if ( vpn != BCM_L2GRE_VPN_INVALID) {
              _BCM_L2GRE_VPN_GET(vfi_index, _BCM_L2GRE_VPN_TYPE_ELAN,  vpn);
              if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeL2Gre)) {
                   return BCM_E_NOT_FOUND;
              }
         } else {
              vfi_index = _BCM_L2GRE_VFI_INVALID;
         }

        /* ---- Read in current table values for VP1 and VP2 ----- */
        (void) _bcm_tr3_l2gre_eline_vp_map_get (unit, vfi_index,  &vp1,  &vp2);
        vp = vp1;
        if (_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
            if (*port_count < port_max) {
                rv = _bcm_tr3_l2gre_port_get(unit, vpn, vp, 
                                           &port_array[*port_count]);
                if (rv < 0) {
                    return rv;
                }
                (*port_count)++;
            }
        }

        vp = vp2;
        if (_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
            if (*port_count < port_max) {
                rv = _bcm_tr3_l2gre_port_get(unit, vpn, vp, 
                                           &port_array[*port_count]);
                if (rv < 0) {
                    return rv;
                }
                (*port_count)++;
            }
        }
    } else if (isEline == 0x0 ) {
        uint32 vfi, entry_type;
        int num_vp;
        source_vp_entry_t svp;

        _BCM_L2GRE_VPN_GET(vfi, _BCM_L2GRE_VPN_TYPE_ELAN,  vpn);
        if (!_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeL2Gre)) {
            rv = BCM_E_NOT_FOUND;
            return rv;
        }
        num_vp = soc_mem_index_count(unit, SOURCE_VPm);
        for (vp = 0; vp < num_vp; vp++) {
            /* Check for the validity of the VP */
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeL2Gre)) {
                continue;
            }
            if (*port_count == port_max) {
                break;
            }
            rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
            if (rv < 0) {
                return rv;
            }
            entry_type = soc_SOURCE_VPm_field32_get(unit, &svp, ENTRY_TYPEf);

            if (vfi == soc_SOURCE_VPm_field32_get(unit, &svp, VFIf) && 
                 entry_type == _BCM_L2GRE_SOURCE_VP_TYPE_VFI) {

                /* Check if number of ports is requested */
                if (0 == port_max) {
                    (*port_count)++;
                    continue;
                } else if (*port_count == port_max) {
                    break;
                }

               rv = _bcm_tr3_l2gre_port_get(unit, vpn, vp,
                                           &port_array[*port_count]);
                if (rv < 0) {
                    return rv;
                }
                (*port_count)++;
            }
        }
    }
    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_trunk_member_add
 * Purpose:
 *      Activate Trunk Port-membership from 
 *      EGR_PORT_TO_NHI_MAPPING
 * Parameters:
 *     unit  - (IN) Device Number
 *     tgid - (IN)  Trunk Group ID
 *     trunk_member_count - (IN) Count of Trunk members to be added
 *     trunk_member_array - (IN) Trunk member ports to be added
*/
int
bcm_tr3_l2gre_trunk_member_add(int unit, bcm_trunk_t trunk_id, 
                int trunk_member_count, bcm_port_t *trunk_member_array) 
{ 
    int idx = 0;
    int rv = BCM_E_NONE;
    uint32 reg_val = 0;
    int nh_index = 0, old_nh_index = -1;
    bcm_l3_egress_t egr;

    rv = _bcm_trunk_id_validate(unit, trunk_id);
    if (BCM_FAILURE(rv)) {
        return BCM_E_PORT;
    }   

    /* Obtain nh_index for trunk_id */
    rv = _bcm_xgs3_trunk_nh_store_get(unit, trunk_id, &nh_index);
    if (nh_index == 0) {
        return rv;
    }
    BCM_IF_ERROR_RETURN(bcm_xgs3_nh_get(unit, nh_index, &egr));

    if (egr.flags & BCM_L3_L2GRE_ONLY) {
        for (idx = 0; idx < trunk_member_count; idx++) {
            BCM_IF_ERROR_RETURN(
                    READ_EGR_PORT_TO_NHI_MAPPINGr(unit, trunk_member_array[idx], &reg_val));
            old_nh_index = soc_reg_field_get(unit, EGR_PORT_TO_NHI_MAPPINGr, reg_val, NEXT_HOP_INDEXf);
            if (!old_nh_index) {
                /* Set EGR_PORT_TO_NHI_MAPPING */
                rv = soc_reg_field32_modify(unit, EGR_PORT_TO_NHI_MAPPINGr,
                        trunk_member_array[idx], NEXT_HOP_INDEXf, nh_index);
                if (BCM_FAILURE(rv)) {
                    return rv;
                }
            }
        }
    }
    return rv;
}

/*
 * Function:
 *      bcm_tr3_l2gre_trunk_member_delete
 * Purpose:
 *      Clear Trunk Port-membership from 
 *      EGR_PORT_TO_NHI_MAPPING
 * Parameters:
 *     unit  - (IN) Device Number
 *     tgid - (IN)  Trunk Group ID
 *     trunk_member_count - (IN) Count of Trunk members to be deleted
 *     trunk_member_array - (IN) Trunk member ports to be deleted
*/
int
bcm_tr3_l2gre_trunk_member_delete(int unit, bcm_trunk_t trunk_id, 
                int trunk_member_count, bcm_port_t *trunk_member_array) 
{ 
    int idx = 0;
    int rv = BCM_E_NONE;
    uint32 reg_val = 0;
    int old_nh_index = 0;
    int nh_index = 0;
    bcm_l3_egress_t egr;

    rv = _bcm_trunk_id_validate(unit, trunk_id);
    if (BCM_FAILURE(rv)) {
        return BCM_E_PORT;
    }

    /* Obtain nh_index for trunk_id */
    rv = _bcm_xgs3_trunk_nh_store_get(unit, trunk_id, &nh_index);
    if (nh_index == 0) {
        for (idx = 0; idx < trunk_member_count; idx++) {
            rv += _bcm_tr3_l2gre_trunk_table_reset(unit, trunk_member_array[idx]);
        }
        if (BCM_FAILURE(rv)) {
            return rv;
        }
    } else {
        BCM_IF_ERROR_RETURN(bcm_xgs3_nh_get(unit, nh_index, &egr));

        if (egr.flags & BCM_L3_L2GRE_ONLY) {
            for (idx = 0; idx < trunk_member_count; idx++) {
                BCM_IF_ERROR_RETURN(
                    READ_EGR_PORT_TO_NHI_MAPPINGr(unit, trunk_member_array[idx], &reg_val));
                old_nh_index = soc_reg_field_get(unit, EGR_PORT_TO_NHI_MAPPINGr, reg_val, NEXT_HOP_INDEXf);
                if (0 != old_nh_index) {
                    /* Reset EGR_PORT_TO_NHI_MAPPING */
                    rv = soc_reg_field32_modify(unit, EGR_PORT_TO_NHI_MAPPINGr,
                            trunk_member_array[idx], NEXT_HOP_INDEXf, 0x0);
                    if (BCM_FAILURE(rv)) {
                        return rv;
                    }
                }
            }
        }
    }
    return rv;
}

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_tr3_l2gre_sw_dump
 * Purpose:
 *     Displays TR3 L2GRE information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_tr3_l2gre_sw_dump(int unit)
{
    int i, num_vp;
    _bcm_tr3_l2gre_bookkeeping_t *l2gre_info = L2GRE_INFO(unit);

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    LOG_CLI((BSL_META_U(unit,
                        "Tunnel Initiator Endpoints:\n")));
    for (i = 0; i < num_vp; i++) {
        if (L2GRE_INFO(unit)->l2gre_tunnel_init[i].dip != 0 &&
            L2GRE_INFO(unit)->l2gre_tunnel_init[i].sip != 0 ) {
            LOG_CLI((BSL_META_U(unit,
                                "Tunnel idx:%d, sip:%x, dip:%x\n"), i,
                     l2gre_info->l2gre_tunnel_init[i].sip, 
                     l2gre_info->l2gre_tunnel_init[i].dip));
        }
    }

    LOG_CLI((BSL_META_U(unit,
                        "Tunnel Terminator Endpoints:\n")));
    for (i = 0; i < num_vp; i++) {
        if (L2GRE_INFO(unit)->l2gre_tunnel_term[i].dip != 0 &&
            L2GRE_INFO(unit)->l2gre_tunnel_term[i].sip != 0 ) {
            LOG_CLI((BSL_META_U(unit,
                                "Tunnel idx:%d, sip:%x, dip:%x\n"), i,
                     l2gre_info->l2gre_tunnel_term[i].sip, 
                     l2gre_info->l2gre_tunnel_term[i].dip));
        }
    }
    
    LOG_CLI((BSL_META_U(unit,
                        "\n  Match Info    : \n")));
    for (i = 0; i < num_vp; i++) {
        if ((l2gre_info->match_key[i].trunk_id == 0) && 
            (l2gre_info->match_key[i].modid == 0) &&
            (l2gre_info->match_key[i].port == 0) &&
            (l2gre_info->match_key[i].flags == 0)) {
            continue;
        }
        LOG_CLI((BSL_META_U(unit,
                            "\n  L2GRE port vp = %d\n"), i));
        LOG_CLI((BSL_META_U(unit,
                            "Flags = %x\n"), l2gre_info->match_key[i].flags));
        LOG_CLI((BSL_META_U(unit,
                            "Index = %x\n"), l2gre_info->match_key[i].index));
        LOG_CLI((BSL_META_U(unit,
                            "TGID = %d\n"), l2gre_info->match_key[i].trunk_id));
        LOG_CLI((BSL_META_U(unit,
                            "Modid = %d\n"), l2gre_info->match_key[i].modid));
        LOG_CLI((BSL_META_U(unit,
                            "Port = %d\n"), l2gre_info->match_key[i].port));
        LOG_CLI((BSL_META_U(unit,
                            "Match VLAN = %d\n"), 
                 l2gre_info->match_key[i].match_vlan));
        LOG_CLI((BSL_META_U(unit,
                            "Match Inner VLAN = %d\n"), 
                 l2gre_info->match_key[i].match_inner_vlan));
        LOG_CLI((BSL_META_U(unit,
                            "Match VPNid = %d\n"), 
                 l2gre_info->match_key[i].match_vpnid));
        LOG_CLI((BSL_META_U(unit,
                            "Match tunnel Index = %x\n"), 
                 l2gre_info->match_key[i].match_tunnel_index));
    }
    
    return;
}
#endif

#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */

