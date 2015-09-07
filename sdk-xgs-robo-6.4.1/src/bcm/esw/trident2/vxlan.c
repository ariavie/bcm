/*
 * $Id: vxlan.c,v 1.94 Broadcom SDK $
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
 * VXLAN API
 */

#include <soc/defs.h>
#include <sal/core/libc.h>
#include <shared/bsl.h>
#if defined(BCM_TRIDENT2_SUPPORT) && defined(INCLUDE_L3)
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
#include <bcm_int/esw/trident2.h>
#include <bcm_int/esw/vxlan.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/common/multicast.h>

_bcm_td2_vxlan_bookkeeping_t   *_bcm_td2_vxlan_bk_info[BCM_MAX_NUM_UNITS] = { 0 };

#define VXLAN_INFO(_unit_)   (_bcm_td2_vxlan_bk_info[_unit_])
#define L3_INFO(_unit_)   (&_bcm_l3_bk_info[_unit_])

/*
 * EGR_IP_TUNNEL table usage bitmap operations
 */
#define _BCM_VXLAN_IP_TNL_USED_GET(_u_, _tnl_) \
        SHR_BITGET(VXLAN_INFO(_u_)->vxlan_ip_tnl_bitmap, (_tnl_))
#define _BCM_VXLAN_IP_TNL_USED_SET(_u_, _tnl_) \
        SHR_BITSET(VXLAN_INFO((_u_))->vxlan_ip_tnl_bitmap, (_tnl_))
#define _BCM_VXLAN_IP_TNL_USED_CLR(_u_, _tnl_) \
        SHR_BITCLR(VXLAN_INFO((_u_))->vxlan_ip_tnl_bitmap, (_tnl_))

#define _BCM_VXLAN_CLEANUP(_rv_) \
       if ( (_rv_) < 0) { \
           goto cleanup; \
       }

STATIC int _bcm_td2_vxlan_match_vnid_entry_reset(int unit, uint32 vnid);
STATIC int _bcm_td2_vxlan_bud_loopback_disable(int unit);
STATIC int _bcm_td2_vxlan_sd_tag_set( int unit, bcm_vxlan_vpn_config_t *vxlan_vpn_info, 
                         bcm_vxlan_port_t *vxlan_port, 
                         _bcm_td2_vxlan_nh_info_t  *egr_nh_info, 
                         egr_vlan_xlate_entry_t   *vxlate_entry,
                         int *tpid_index );
STATIC int _bcm_td2_vxlan_egr_xlate_entry_reset(int unit, bcm_vpn_t vpn);
STATIC int _bcm_td2_vxlan_egr_xlate_entry_get( int unit, int vfi, egr_vlan_xlate_entry_t *vxlate_entry);
STATIC void _bcm_td2_vxlan_sd_tag_get( int unit, bcm_vxlan_vpn_config_t *vxlan_vpn_info, 
                         bcm_vxlan_port_t *vxlan_port, 
                         egr_l3_next_hop_entry_t *egr_nh, 
                         egr_vlan_xlate_entry_t   *vxlate_entry,
                         int network_port_flag );

void bcm_td2_vxlan_match_clear (int unit, int vp);

/*
 * Function:
 *      _bcm_vxlan_check_init
 * Purpose:
 *      Check if VXLAN is initialized
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */


STATIC int 
_bcm_vxlan_check_init(int unit)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;

    if ((unit < 0) || (unit >= BCM_MAX_NUM_UNITS)) {
         return BCM_E_UNIT;
    }

    vxlan_info = VXLAN_INFO(unit);

    if ((vxlan_info == NULL) || (vxlan_info->initialized == FALSE)) { 
         return BCM_E_INIT;
    } else {
         return BCM_E_NONE;
    }
}

/*
 * Function:
 *      bcm_td2_vxlan_lock
 * Purpose:
 *      Take VXLAN Lock Sempahore
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */


 int 
 bcm_td2_vxlan_lock(int unit)
{
   int rv=BCM_E_NONE;

   rv = _bcm_vxlan_check_init(unit);
   
   if ( rv == BCM_E_NONE ) {
           sal_mutex_take(VXLAN_INFO((unit))->vxlan_mutex, sal_mutex_FOREVER);
   }
   return rv; 
}



/*
 * Function:
 *      bcm_td2_vxlan_unlock
 * Purpose:
 *      Release  VXLAN Lock Semaphore
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */


void
bcm_td2_vxlan_unlock(int unit)
{
   int rv=BCM_E_NONE;

   rv = _bcm_vxlan_check_init(unit);
    if ( rv == BCM_E_NONE ) {
         sal_mutex_give(VXLAN_INFO((unit))->vxlan_mutex);
    }
}

/*
 * Function:
 *      bcm_td2_vxlan_udpDestPort_set
 * Purpose:
 *      Set UDP Dest port for VXLAN
 * Parameters:
 *      unit - SOC unit number.
 *      UDP Dest port  -  Non-zero value
 * Returns:
 *      BCM_E_XXX.
 */
int
bcm_td2_vxlan_udpDestPort_set(int unit, int udpDestPort)
{
    int rv = BCM_E_NONE;
    uint64 reg64;

    COMPILER_64_ZERO(reg64);

    if ((udpDestPort < 0) || (udpDestPort > 0xFFFF) ) {
         return BCM_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(READ_VXLAN_CONTROLr(unit, &reg64));
    if (SOC_REG_FIELD_VALID(unit, VXLAN_CONTROLr, UDP_DEST_PORTf)){
         soc_reg64_field32_set(unit, VXLAN_CONTROLr, &reg64,
                      UDP_DEST_PORTf, udpDestPort);
    }
    SOC_IF_ERROR_RETURN(WRITE_VXLAN_CONTROLr(unit, reg64));
    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_udpSourcePort_set
 * Purpose:
 *      Enable UDP Source port based HASH for VXLAN
 * Parameters:
 *      unit - SOC unit number.
 *      hashEnable - Enable Hash for UDP SourcePort
 * Returns:
 *      BCM_E_XXX.
 */
int
bcm_td2_vxlan_udpSourcePort_set(int unit, int hashEnable)
{
    int rv = BCM_E_NONE;
    uint64 reg64;

    COMPILER_64_ZERO(reg64);

    if ((hashEnable < 0) || (hashEnable > 1) ) {
         return BCM_E_PARAM;
    }

    if (SOC_REG_FIELD_VALID(unit, EGR_VXLAN_CONTROLr, USE_SOURCE_PORT_SELf)){
         soc_reg64_field32_set(unit, EGR_VXLAN_CONTROLr, &reg64,
                      USE_SOURCE_PORT_SELf, hashEnable);
    }
    soc_reg64_field32_set(unit, EGR_VXLAN_CONTROLr, &reg64,
                      VXLAN_FLAGSf, 0x8);

    SOC_IF_ERROR_RETURN(WRITE_EGR_VXLAN_CONTROLr(unit, reg64));
    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_udpDestPort_get
 * Purpose:
 *      Get UDP Dest port for VXLAN
 * Parameters:
 *      unit - SOC unit number.
 *      UDP Dest port  -  Non-zero value
 * Returns:
 *      BCM_E_XXX.
 */
int
bcm_td2_vxlan_udpDestPort_get(int unit, int *udpDestPort)
{
    int rv = BCM_E_NONE;
    uint64 reg64;

    COMPILER_64_ZERO(reg64);

    SOC_IF_ERROR_RETURN(READ_VXLAN_CONTROLr(unit, &reg64));
    *udpDestPort = soc_reg64_field32_get(unit, VXLAN_CONTROLr, 
                                              reg64, UDP_DEST_PORTf);
    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_udpSourcePort_get
 * Purpose:
 *      Get UDP Source port based HASH for VXLAN
 * Parameters:
 *      unit - SOC unit number.
 *      hashEnable - Enable Hash for UDP SourcePort
 * Returns:
 *      BCM_E_XXX.
 */
int
bcm_td2_vxlan_udpSourcePort_get(int unit, int *hashEnable)
{
    int rv = BCM_E_NONE;
    uint64 reg64;

    COMPILER_64_ZERO(reg64);

    SOC_IF_ERROR_RETURN(READ_EGR_VXLAN_CONTROLr(unit, &reg64));
    *hashEnable = soc_reg64_field32_get(unit, EGR_VXLAN_CONTROLr, 
                                              reg64, USE_SOURCE_PORT_SELf);
    return rv;
}


/*
 * Function:
 *      _bcm_td2_vxlan_free_resource
 * Purpose:
 *      Free all allocated software resources 
 * Parameters:
 *      unit - SOC unit number
 * Returns:
 *      Nothing
 */


STATIC void
_bcm_td2_vxlan_free_resource(int unit)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info = VXLAN_INFO(unit);

    /* If software tables were not allocated we are done. */ 
    if (NULL == VXLAN_INFO(unit)) {
        return;
    }

    /* Destroy EGR_IP_TUNNEL usage bitmap */
    if (vxlan_info->vxlan_ip_tnl_bitmap) {
        sal_free(vxlan_info->vxlan_ip_tnl_bitmap);
        vxlan_info->vxlan_ip_tnl_bitmap = NULL;
    }

    if (vxlan_info->match_key) {
        sal_free(vxlan_info->match_key);
        vxlan_info->match_key = NULL;
    }

    if (vxlan_info->vxlan_tunnel_init) {
        sal_free(vxlan_info->vxlan_tunnel_init);
        vxlan_info->vxlan_tunnel_init = NULL;
    }

    if (vxlan_info->vxlan_tunnel_term) {
        sal_free(vxlan_info->vxlan_tunnel_term);
        vxlan_info->vxlan_tunnel_term = NULL;
    }

    /* Free module data. */
    sal_free(VXLAN_INFO(unit));
    VXLAN_INFO(unit) = NULL;
}

/*
 * Function:
 *      bcm_td2_vxlan_allocate_bk
 * Purpose:
 *      Initialize VXLAN software book-kepping
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
bcm_td2_vxlan_allocate_bk(int unit)
{
    /* Allocate/Init unit software tables. */
    if (NULL == VXLAN_INFO(unit)) {
        BCM_TD2_VXLAN_ALLOC(VXLAN_INFO(unit), sizeof(_bcm_td2_vxlan_bookkeeping_t),
                          "vxlan_bk_module_data");
        if (NULL == VXLAN_INFO(unit)) {
            return (BCM_E_MEMORY);
        } else {
            VXLAN_INFO(unit)->initialized = FALSE;
        }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_vxlan_hw_clear
 * Purpose:
 *     Perform hw tables clean up for VXLAN module. 
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */
 
STATIC int
_bcm_td2_vxlan_hw_clear(int unit)
{
    int rv = BCM_E_NONE, rv_error = BCM_E_NONE;

    rv = bcm_td2_vxlan_tunnel_terminator_destroy_all(unit);
    if (BCM_FAILURE(rv) && (BCM_E_NONE == rv_error)) {
        rv_error = rv;
    }

    rv = bcm_td2_vxlan_tunnel_initiator_destroy_all(unit);
    if (BCM_FAILURE(rv) && (BCM_E_NONE == rv_error)) {
        rv_error = rv;
    }

    rv = bcm_td2_vxlan_vpn_destroy_all(unit);
    if (BCM_FAILURE(rv) && (BCM_E_NONE == rv_error)) {
        rv_error = rv;
    }

    rv = _bcm_td2_vxlan_bud_loopback_disable(unit);
    if (BCM_FAILURE(rv) && (BCM_E_NONE == rv_error)) {
        rv_error = rv;
    }

    return rv_error;
}

/*
 * Function:
 *      bcm_td2_vxlan_cleanup
 * Purpose:
 *      DeInit  VXLAN software module
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_td2_vxlan_cleanup(int unit)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int rv = BCM_E_UNAVAIL;

    if ((unit < 0) || (unit >= BCM_MAX_NUM_UNITS)) {
         return BCM_E_UNIT;
    }

    vxlan_info = VXLAN_INFO(unit);

    if (FALSE == vxlan_info->initialized) {
        return (BCM_E_NONE);
    } 

    rv = bcm_td2_vxlan_lock (unit);
    if (BCM_FAILURE(rv)) {
        return rv;
    }

    if (0 == SOC_HW_ACCESS_DISABLE(unit)) { 
        rv = _bcm_td2_vxlan_hw_clear(unit);
    }

    /* Mark the state as uninitialized */
    vxlan_info->initialized = FALSE;

    sal_mutex_give(vxlan_info->vxlan_mutex);

    /* Destroy protection mutex. */
    sal_mutex_destroy(vxlan_info->vxlan_mutex );

    /* Free software resources */
    (void) _bcm_td2_vxlan_free_resource(unit);

    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_port_match_count_adjust
 * Purpose:
 *      Obtain ref-count for a VXLAN port
 * Returns:
 *      BCM_E_XXX
 */

STATIC void 
bcm_td2_vxlan_port_match_count_adjust(int unit, int vp, int step)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info = VXLAN_INFO(unit);
 
    vxlan_info->match_key[vp].match_count += step;
}

#ifdef BCM_WARM_BOOT_SUPPORT

#define BCM_WB_VERSION_1_0                SOC_SCACHE_VERSION(1, 0)
#define BCM_WB_VERSION_1_1                SOC_SCACHE_VERSION(1, 1)
#define BCM_WB_VERSION_1_2                SOC_SCACHE_VERSION(1, 2)
#define BCM_WB_DEFAULT_VERSION            BCM_WB_VERSION_1_2

STATIC int _bcm_td2_vxlan_wb_alloc(int unit);

/*
 * Function:
 *      _bcm_td2_vxlan_wb_recover
 *
 * Purpose:
 *      Recover VXLAN module info for Level 2 Warm Boot from persisitent memory
 *
 * Warm Boot Version Map:
 *      see _bcm_esw_vxlan_sync definition
 *
 * Parameters:
 *      unit - (IN) Device Unit Number.
 *
 * Returns:
 *      BCM_E_XXX
 */
 
STATIC int
_bcm_td2_vxlan_wb_recover(int unit)
{
    int i, sz = 0, rv = BCM_E_NONE;
    int num_tnl = 0, num_vp = 0;
    int stable_size;
    int additional_scache_size = 0;
    uint16 recovered_ver = 0;
    uint8 *vxlan_state = NULL;
    soc_scache_handle_t scache_handle;
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;

    vxlan_info = VXLAN_INFO(unit);

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    /* Requires extended scache support level-2 warmboot */
    if ((stable_size == 0) || (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit))) {
        return BCM_E_NONE;
    }

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_VXLAN, 0);
    rv = _bcm_esw_scache_ptr_get(unit, scache_handle, FALSE,
                                 0, &vxlan_state, 
                                 BCM_WB_DEFAULT_VERSION, &recovered_ver);
    if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
        return rv;
    } else if (rv == BCM_E_NOT_FOUND) {
        return _bcm_td2_vxlan_wb_alloc(unit);
    }

    if (vxlan_state != NULL) {
        num_vp = soc_mem_index_count(unit, SOURCE_VPm);
        num_tnl = soc_mem_index_count(unit, EGR_IP_TUNNELm);

        if (recovered_ver >= BCM_WB_VERSION_1_2) {
            sz = num_vp * sizeof(_bcm_vxlan_tunnel_endpoint_t);
        } else if (recovered_ver >= BCM_WB_VERSION_1_1) {
            /* Warm boot upgrade case, activate_flag was added.*/
            sz = num_vp * (sizeof(_bcm_vxlan_tunnel_endpoint_t) - sizeof(int));
            additional_scache_size =
                     ((num_vp * sizeof(_bcm_vxlan_tunnel_endpoint_t)) - sz) * 2;
        } else {
            /* Warm boot upgrade case, in the previous version (SDK 6.3.4)
                tunnel_state was uint8, now changed to uint16 */
            sz = num_vp * (sizeof(_bcm_vxlan_tunnel_endpoint_t) - 1 - sizeof(int));
            additional_scache_size =
                      ((num_vp * sizeof(_bcm_vxlan_tunnel_endpoint_t)) - sz) * 2;
        } 
     
        /* Recover each entry of Vxlan Tunnel Terminator (SIP, DIP, Tunnel State) */
        sal_memcpy(vxlan_info->vxlan_tunnel_term, vxlan_state, sz);
        vxlan_state += sz;
    
        /* Recover each entry of Vxlan Tunnel Initiator (SIP, DIP, Tunnel State) */
        sal_memcpy(vxlan_info->vxlan_tunnel_init, vxlan_state, sz);
        vxlan_state += sz;
    
        /* Recover the BITMAP of Vxlan Tunnel usage */
        sal_memcpy(vxlan_info->vxlan_ip_tnl_bitmap, vxlan_state, 
                    SHR_BITALLOCSIZE(num_tnl));
        vxlan_state += SHR_BITALLOCSIZE(num_tnl);

        /* Recover the flags & match_tunnel_index of each Match Key */
        for (i = 0; i < num_vp; i++) {
            sal_memcpy(&(vxlan_info->match_key[i].flags), vxlan_state, 
                        sizeof(uint32));
            vxlan_state += sizeof(uint32);        
            sal_memcpy(&(vxlan_info->match_key[i].match_tunnel_index), vxlan_state, 
                        sizeof(uint32));
            vxlan_state += sizeof(uint32);
        }
        /* Reallocate additional scache size required for tunnel_state in 
           Vxlan Tunnel Terminator and Vxlan Tunnel Initiator */ 
        if (additional_scache_size > 0) {
            rv = soc_scache_realloc(unit,scache_handle,additional_scache_size);
            if(BCM_FAILURE(rv)) {
               return rv;
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_vxlan_wb_alloc
 *
 * Purpose:
 *      Alloc persisitent memory for Level 2 Warm Boot scache.
 *
 * Parameters:
 *      unit - (IN) Device Unit Number.
 *
 * Returns:
 *      BCM_E_XXX
 */
 
STATIC int
_bcm_td2_vxlan_wb_alloc(int unit)
{
    int alloc_sz = 0, rv = BCM_E_NONE;
    soc_scache_handle_t scache_handle;
    int num_tnl = 0, num_vp = 0;

    uint8 *vxlan_state;
    int stable_size;

    SOC_IF_ERROR_RETURN(soc_stable_size_get(unit, &stable_size));

    if ((stable_size == 0) || (SOC_WARM_BOOT_SCACHE_IS_LIMITED(unit))) {
        return BCM_E_NONE;
    }

    /* Size of Tunnel Terminator & Initiator of all VP */
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    alloc_sz += num_vp * sizeof(_bcm_vxlan_tunnel_endpoint_t) * 2;

    /* Size of EGR_IP_TUNNEL index bitmap */
    num_tnl = soc_mem_index_count(unit, EGR_IP_TUNNELm);
    alloc_sz += SHR_BITALLOCSIZE(num_tnl);

    /* Size of All match_key's flags & match_tunnel_index */
    alloc_sz += num_vp * sizeof(uint32) * 2;

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_VXLAN, 0);
    rv = _bcm_esw_scache_ptr_get(unit, scache_handle, TRUE,
                                 alloc_sz, (uint8**)&vxlan_state, 
                                 BCM_WB_DEFAULT_VERSION, NULL);
    if (BCM_FAILURE(rv) && (rv != BCM_E_NOT_FOUND)) {
        return rv;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_td2_vxlan_reinit
 * Purpose:
 *      Warm boot recovery for the VXLAN software module
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_td2_vxlan_reinit(int unit)
{
    soc_mem_t mem;  
    int i, index_min, index_max, buf_size;
    int vp;
    int rv = BCM_E_NONE;
    vlan_xlate_entry_t *vent;
    mpls_entry_entry_t *ment;
    source_trunk_map_table_entry_t *sent;
    uint32 trunk, tgid, mod_id, port_num, key_type;
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    uint8 *trunk_buf = NULL;
    uint8 *xlate_buf = NULL;
    uint8 *mpls_buf = NULL;   

    vxlan_info = VXLAN_INFO(unit);

    mem = SOURCE_TRUNK_MAP_TABLEm;
    index_min = soc_mem_index_min(unit, mem);
    index_max = soc_mem_index_max(unit, mem);

    /* We check SOURCE_TRUNK_MAP_TABLE table outside to avoid the HUGE loop.
     * Set each match_key[vp].index first, and override it to zero later if 
     *  it's not needed.
     */
    buf_size = SOC_MEM_TABLE_BYTES(unit, mem);
    trunk_buf = soc_cm_salloc(unit, buf_size, "SOURCE_TRUNK_MAP_TABLE buffer");
    if (NULL == trunk_buf) {
        rv = BCM_E_MEMORY;
        goto clean_up;
    }
    rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ANY, index_min, index_max, trunk_buf);
    if (SOC_FAILURE(rv)) {
        goto clean_up;
    } 
    
    for (i = index_min; i <= index_max; i++) {
        bcm_trunk_t tgid = -1;
        int port_type = 0;
        sent = soc_mem_table_idx_to_pointer(unit, mem, source_trunk_map_table_entry_t *, 
                                         trunk_buf, i);     
        if (soc_mem_field_valid(unit, mem, SVP_VALIDf)) {
            if (soc_mem_field32_get(unit, mem, sent, SVP_VALIDf) == 0) {
                continue;
            }
        }

        port_type = soc_mem_field32_get(unit, mem, sent, PORT_TYPEf);
        if (port_type == 1) {                   /* Trunk */
            tgid = soc_mem_field32_get(unit, mem, sent, TGIDf);
        }

        vp = soc_mem_field32_get(unit, mem, sent, SOURCE_VPf);
        if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
            if (port_type == 1) {               /* Trunk */
                vxlan_info->match_key[vp].flags = _BCM_VXLAN_PORT_MATCH_TYPE_TRUNK;
                vxlan_info->match_key[vp].trunk_id = tgid; 
                vxlan_info->match_key[vp].modid = -1;
            } else {
                vxlan_info->match_key[vp].index = i; /* SRC Trunk table index */
                vxlan_info->match_key[vp].flags = _BCM_VXLAN_PORT_MATCH_TYPE_PORT;
                vxlan_info->match_key[vp].trunk_id = -1; 
                vxlan_info->match_key[vp].modid = -1;            
            }
        }
    }    

    /* 
     * For Trident2 device currently
     *
     * foreach valid Source VP used by VXLAN of VLAN_XLANTE table
     *   build the inner/outer 802.1Q tag info, trunk/port info, flag of each SVP.
     *   base on hash key type and trunk.
     * 
     * Sets the following fields:
     *   
     *   VXLAN_INFO[unit].match_key[SVP].flags
     *   VXLAN_INFO[unit].match_key[SVP].match_vlan
     *   VXLAN_INFO[unit].match_key[SVP].match_inner_vlan
     *   VXLAN_INFO[unit].match_key[SVP].trunk_id
     *   VXLAN_INFO[unit].match_key[SVP].port
     *   VXLAN_INFO[unit].match_key[SVP].modid     
     *   VXLAN_INFO[unit].match_key[SVP].index 
     *
     */

    mem = VLAN_XLATEm;
    index_min = soc_mem_index_min(unit, mem);
    index_max = soc_mem_index_max(unit, mem);

    buf_size = SOC_MEM_TABLE_BYTES(unit, mem);
    xlate_buf = soc_cm_salloc(unit, buf_size, "VLAN_XLATE buffer");
    if (NULL == xlate_buf) {
        rv = BCM_E_MEMORY;
        goto clean_up;
    }
    rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ANY, index_min, index_max, xlate_buf);
    if (SOC_FAILURE(rv)) {
        goto clean_up;
    } 
    
    for (i = index_min; i <= index_max; i++) {
        vent = soc_mem_table_idx_to_pointer(unit, mem, vlan_xlate_entry_t *, 
                                         xlate_buf, i);
        
        if (soc_mem_field32_get(unit, mem, vent, VALIDf) == 0) {
            continue;
        }

        if (soc_mem_field32_get(unit, mem, vent, XLATE__MPLS_ACTIONf) != 0x1) {
            continue;
        }
        
        vp = soc_mem_field32_get(unit, mem, vent, XLATE__SOURCE_VPf);

        if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
            key_type = soc_mem_field32_get(unit, mem, vent, KEY_TYPEf);
            trunk = soc_mem_field32_get(unit, mem, vent, XLATE__Tf);
            tgid = soc_mem_field32_get(unit, mem, vent, XLATE__TGIDf);
            mod_id = soc_mem_field32_get(unit, mem, vent, XLATE__MODULE_IDf);
            port_num = soc_mem_field32_get(unit, mem, vent, XLATE__PORT_NUMf);
            if (TR_VLXLT_HASH_KEY_TYPE_OVID == key_type) {
                vxlan_info->match_key[vp].flags =
                                        _BCM_VXLAN_PORT_MATCH_TYPE_VLAN;
                vxlan_info->match_key[vp].match_vlan =
                    soc_mem_field32_get(unit, mem, vent, XLATE__OVIDf);
            } else if (TR_VLXLT_HASH_KEY_TYPE_IVID == key_type) {
                vxlan_info->match_key[vp].flags =
                                      _BCM_VXLAN_PORT_MATCH_TYPE_INNER_VLAN;
                vxlan_info->match_key[vp].match_inner_vlan =
                     soc_mem_field32_get(unit, mem, vent, XLATE__IVIDf);
            } else if (TR_VLXLT_HASH_KEY_TYPE_IVID_OVID == key_type) {
                vxlan_info->match_key[vp].flags =
                                    _BCM_VXLAN_PORT_MATCH_TYPE_VLAN_STACKED;
                vxlan_info->match_key[vp].match_vlan =
                    soc_mem_field32_get(unit, mem, vent, XLATE__OVIDf);
                vxlan_info->match_key[vp].match_inner_vlan =
                     soc_mem_field32_get(unit, mem, vent, XLATE__IVIDf);
            } else if (TR_VLXLT_HASH_KEY_TYPE_PRI_CFI == key_type) {
                vxlan_info->match_key[vp].flags =
                                        _BCM_VXLAN_PORT_MATCH_TYPE_VLAN_PRI;
                vxlan_info->match_key[vp].match_vlan = 
                    soc_mem_field32_get(unit, mem, vent, OTAGf);
            }

            if (trunk) {
                vxlan_info->match_key[vp].trunk_id = tgid;
                vxlan_info->match_key[vp].modid = -1;
                vxlan_info->match_key[vp].index = 0;
            } else {
                vxlan_info->match_key[vp].port = port_num;
                vxlan_info->match_key[vp].modid = mod_id;
                vxlan_info->match_key[vp].trunk_id = -1;
                vxlan_info->match_key[vp].index = 0;
            }
            bcm_td2_vxlan_port_match_count_adjust(unit, vp, 1);            
        }
    }

    /* Recovery based on entry in MPLS tbl */
    mem = MPLS_ENTRYm; 
    index_min = soc_mem_index_min(unit, mem);
    index_max = soc_mem_index_max(unit, mem);

    /* 
     * foreach valid entry with key_type TUNNEL/VFI of MPLS_ENTRY table
     *   recover the tunnel sip info and flag of each SVP used by VXLAN.
     * 
     * Sets the following fields:
     *   
     *   VXLAN_INFO[unit].match_key[SVP].flags
     *   VXLAN_INFO[unit].match_key[SVP].match_tunnel_index
     *
     */

    buf_size = SOC_MEM_TABLE_BYTES(unit, mem);
    mpls_buf = soc_cm_salloc(unit, buf_size, "MPLS_ENTRY buffer");
    if (NULL == mpls_buf) {
        rv = BCM_E_MEMORY;
        goto clean_up;
    }
    rv = soc_mem_read_range(unit, mem, MEM_BLOCK_ANY, index_min, index_max, mpls_buf);
    if (SOC_FAILURE(rv)) {
        goto clean_up;
    } 
    
    for (i = index_min; i <= index_max; i++) {
        ment = soc_mem_table_idx_to_pointer(unit, mem, mpls_entry_entry_t *, 
                                         mpls_buf, i);        
        if (soc_MPLS_ENTRYm_field32_get(unit, ment, VALIDf) == 0) {
            continue;
        }

        /* Looking for entries of type vxlan only */
        key_type = soc_MPLS_ENTRYm_field32_get(unit, ment, KEY_TYPEf);
        if ((key_type != _BCM_VXLAN_KEY_TYPE_TUNNEL) &&
            (key_type != _BCM_VXLAN_KEY_TYPE_VNID_VFI)) {
            continue;
        }    

        vp = soc_MPLS_ENTRYm_field32_get(unit, ment, VXLAN_SIP__SVPf);
        if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
            vxlan_info->match_key[vp].flags = _BCM_VXLAN_PORT_MATCH_TYPE_VNID;
        }
    }

    /* Recover L2 scache */
    rv = _bcm_td2_vxlan_wb_recover(unit);

clean_up:
    if (NULL != trunk_buf) {
        soc_cm_sfree(unit, trunk_buf);
    }
    if (NULL != xlate_buf) {
        soc_cm_sfree(unit, xlate_buf);
    }
    if (NULL != mpls_buf) {
        soc_cm_sfree(unit, mpls_buf);
    }

    return rv;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      bcm_td2_vxlan_init
 * Purpose:
 *      Initialize the VXLAN software module
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */


int
bcm_td2_vxlan_init(int unit)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int i, num_tnl=0, num_vp=0;
    int rv = BCM_E_NONE;

    if (!L3_INFO(unit)->l3_initialized) {
        LOG_ERROR(BSL_LS_BCM_VXLAN,
                  (BSL_META_U(unit,
                              "L3 module must be initialized prior to VXLAN Init\n")));
        return BCM_E_CONFIG;
    }

    /* Allocate BK Info */
    BCM_IF_ERROR_RETURN(bcm_td2_vxlan_allocate_bk(unit));
    vxlan_info = VXLAN_INFO(unit);

    /*
     * allocate resources
     */
    if (vxlan_info->initialized) {
         BCM_IF_ERROR_RETURN(bcm_td2_vxlan_cleanup(unit));
         BCM_IF_ERROR_RETURN(bcm_td2_vxlan_allocate_bk(unit));
         vxlan_info = VXLAN_INFO(unit);
    }

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    vxlan_info->match_key =
        sal_alloc(sizeof(_bcm_vxlan_match_port_info_t) * num_vp, "match_key");
    if (vxlan_info->match_key == NULL) {
        _bcm_td2_vxlan_free_resource(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(vxlan_info->match_key, 0, 
            sizeof(_bcm_vxlan_match_port_info_t) * num_vp);
    /* Stay same after recover */
    for (i = 0; i < num_vp; i++) {
        bcm_td2_vxlan_match_clear(unit, i);
    }

    /* Create EGR_IP_TUNNEL usage bitmap */
    num_tnl = soc_mem_index_count(unit, EGR_IP_TUNNELm);

    vxlan_info->vxlan_ip_tnl_bitmap =
        sal_alloc(SHR_BITALLOCSIZE(num_tnl), "vxlan_ip_tnl_bitmap");
    if (vxlan_info->vxlan_ip_tnl_bitmap == NULL) {
        _bcm_td2_vxlan_free_resource(unit);
        return BCM_E_MEMORY;
    }
    sal_memset(vxlan_info->vxlan_ip_tnl_bitmap, 0, SHR_BITALLOCSIZE(num_tnl));

    /* Create VXLAN protection mutex. */
    vxlan_info->vxlan_mutex = sal_mutex_create("vxlan_mutex");
    if (!vxlan_info->vxlan_mutex) {
         _bcm_td2_vxlan_free_resource(unit);
         return BCM_E_MEMORY;
    }

    if (NULL == vxlan_info->vxlan_tunnel_term) {
        vxlan_info->vxlan_tunnel_term =
            sal_alloc(sizeof(_bcm_vxlan_tunnel_endpoint_t) * num_vp, "vxlan tunnel term store");
        if (vxlan_info->vxlan_tunnel_term == NULL) {
            _bcm_td2_vxlan_free_resource(unit);
            return BCM_E_MEMORY;
        }
        sal_memset(vxlan_info->vxlan_tunnel_term, 0, 
                sizeof(_bcm_vxlan_tunnel_endpoint_t) * num_vp);
    }

    if (NULL == vxlan_info->vxlan_tunnel_init) {
        vxlan_info->vxlan_tunnel_init =
            sal_alloc(sizeof(_bcm_vxlan_tunnel_endpoint_t) * num_vp, "vxlan tunnel init store");
        if (vxlan_info->vxlan_tunnel_init == NULL) {
            _bcm_td2_vxlan_free_resource(unit);
            return BCM_E_MEMORY;
        }
        sal_memset(vxlan_info->vxlan_tunnel_init, 0, 
                sizeof(_bcm_vxlan_tunnel_endpoint_t) * num_vp);
    }

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        rv = _bcm_td2_vxlan_reinit(unit);
        if (BCM_FAILURE(rv)) {
            _bcm_td2_vxlan_free_resource(unit);
        }
    } else {
        rv = _bcm_td2_vxlan_wb_alloc(unit);    
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    /* Mark the state as initialized */
    vxlan_info->initialized = TRUE;

    return rv;
}

 /*
  * Function:
  *      _bcm_td2_vxlan_bud_loopback_enable
  * Purpose:
  *      Enable loopback for VXLAN BUD node multicast
  * Parameters:
  *   IN : unit
  * Returns:
  *      BCM_E_XXX
  */
 
STATIC int
_bcm_td2_vxlan_bud_loopback_enable(int unit)
{
    soc_field_t lport_fields[] = {
        PORT_TYPEf,
        VXLAN_TERMINATION_ALLOWEDf,
        V4IPMC_ENABLEf,
        VXLAN_VN_ID_LOOKUP_KEY_TYPEf,
        VXLAN_DEFAULT_SVP_ENABLEf
    };
    uint32 lport_values[] = {
        0x0,  /* PORT_TYPEf */
        0x1,  /* VXLAN_TERMINATION_ALLOWEDf */
        0x1,  /* V4IPMC_ENABLEf */
        0x0,  /* VXLAN_VN_ID_LOOKUP_KEY_TYPEf */
        0x1   /*  VXLAN_DEFAULT_SVP_ENABLEf */
    };

    /* Update LPORT Profile Table */
    return _bcm_lport_profile_fields32_modify(unit, LPORT_PROFILE_LPORT_TAB,
                                              COUNTOF(lport_fields),
                                              lport_fields, lport_values);
}

 /*
  * Function:
  *      _bcm_td2_vxlan_bud_loopback_disable
  * Purpose:
  *      Disable loopback for VXLAN BUD node multicast
  * Parameters:
  *   IN : unit
  * Returns:
  *      BCM_E_XXX
  */
 
STATIC int
_bcm_td2_vxlan_bud_loopback_disable(int unit)
{
    soc_field_t lport_fields[] = {
        PORT_TYPEf,
        VXLAN_TERMINATION_ALLOWEDf,
        V4IPMC_ENABLEf,
        VXLAN_VN_ID_LOOKUP_KEY_TYPEf,
        VXLAN_DEFAULT_SVP_ENABLEf
    };
    uint32 lport_values[] = {
        0x0,  /* PORT_TYPEf */
        0x0,  /* VXLAN_TERMINATION_ALLOWEDf */
        0x0,  /* V4IPMC_ENABLEf */
        0x0,  /* VXLAN_VN_ID_LOOKUP_KEY_TYPEf */
        0x0   /*  VXLAN_DEFAULT_SVP_ENABLEf */
    };
    
    /* Update LPORT Profile Table */
    return _bcm_lport_profile_fields32_modify(unit, LPORT_PROFILE_LPORT_TAB,
                                              COUNTOF(lport_fields),
                                              lport_fields, lport_values);
}

 /* Function:
 *      _bcm_td2_vxlan_vpn_is_valid
 * Purpose:
 *      Find if given VXLAN VPN is Valid 
 * Parameters:
 *      unit     - Device Number
 *      l2vpn   - VXLAN VPN
 * Returns:
 *      BCM_E_XXXX
 */
int
_bcm_td2_vxlan_vpn_is_valid( int unit, bcm_vpn_t l2vpn)
{
    bcm_vpn_t vxlan_vpn_min=0;
    int vfi_index=-1, num_vfi=0;

    num_vfi = soc_mem_index_count(unit, VFIm);

    /* Check for Valid vpn */
    _BCM_VXLAN_VPN_SET(vxlan_vpn_min, _BCM_VPN_TYPE_VFI, 0);
    if ( l2vpn < vxlan_vpn_min || l2vpn > (vxlan_vpn_min+num_vfi-1) ) {
        return BCM_E_PARAM;
    }

    _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VPN_TYPE_VFI,  l2vpn);

    if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeVxlan)) {
        return BCM_E_NOT_FOUND;
    }
    return BCM_E_NONE;
}

 /* Function:
 *      _bcm_td2_vxlan_vpn_is_eline
 * Purpose:
 *      Find if given VXLAN VPN is ELINE
 * Parameters:
 *      unit     - Device Number
 *      l2vpn   - VXLAN VPN
 *      isEline  - Flag 
 * Returns:
 *      BCM_E_XXXX
 * Assumes:
 *      l2vpn is valid
 */

int
_bcm_td2_vxlan_vpn_is_eline( int unit, bcm_vpn_t l2vpn, uint8 *isEline)
{
    int vfi_index=-1;
    vfi_entry_t vfi_entry;

    BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_vpn_is_valid(unit, l2vpn));

    _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VPN_TYPE_VFI,  l2vpn);
    BCM_IF_ERROR_RETURN(READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry));

    /* Set the returned VPN id */
    if (soc_VFIm_field32_get(unit, &vfi_entry, PT2PT_ENf)) {
        *isEline = 1;  /* ELINE */
    } else {
        *isEline = 0;  /* ELAN */
    }
    return BCM_E_NONE;
}

 /* Function:
 *      _bcm_td2_vxlan_vp_is_eline
 * Purpose:
 *      Find if given VXLAN VP is ELINE
 * Parameters:
 *      unit     - Device Number
 *      vp   - VXLAN VP
 *      isEline  - Flag 
 * Returns:
 *      BCM_E_XXXX
 * Assumes:
 *      l2vpn is valid
 */

int
_bcm_td2_vxlan_vp_is_eline( int unit, int vp, uint8 *isEline)
{
    source_vp_entry_t svp;
    vfi_entry_t vfi_entry;
    int rv = BCM_E_PARAM;
    int vfi_index;

    if (vp == -1) {
        return BCM_E_PARAM;
    }

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
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
 *      _bcm_td2_vxlan_port_resolve
 * Purpose:
 *      Get the modid, port, trunk values for a VXLAN port
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_td2_vxlan_port_resolve(int unit, bcm_gport_t vxlan_port_id, 
                          bcm_if_t encap_id, bcm_module_t *modid, bcm_port_t *port,
                          bcm_trunk_t *trunk_id, int *id)

{
    int rv = BCM_E_NONE;
    ing_l3_next_hop_entry_t ing_nh;
    egr_l3_next_hop_entry_t egr_nh;
    ing_dvp_table_entry_t dvp;
    int ecmp=0, nh_index=-1, nh_ecmp_index=-1, vp=-1;
    uint32 hw_buf[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry. */
    int  idx, max_ent_count, base_idx;

    rv = _bcm_vxlan_check_init(unit);
    if (rv < 0) {
        return rv;
    }

    if (!BCM_GPORT_IS_VXLAN_PORT(vxlan_port_id)) {
        return (BCM_E_BADID);
    }

    vp = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port_id);
    if (vp == -1) {
       return BCM_E_PARAM;
    }

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
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
 *      _bcm_td2_vxlan_port_nh_cnt_dec
 * Purpose:
 *      Decrease vxlan port's nexthop count.
 * Parameters:
 *      unit  - (IN) SOC unit number.
 *      vp    - (IN) Virtual port number.
 * Returns:
 *      BCM_E_XXX
 */     

STATIC int
_bcm_td2_vxlan_port_nh_cnt_dec(int unit, int vp)
{
    int nh_ecmp_index = -1;
    ing_dvp_table_entry_t dvp;
    uint32  flags = 0;
    int  ref_count = 0;
    int ecmp = 0;

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
 *      _bcm_td2_vxlan_port_cnt_update
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
_bcm_td2_vxlan_port_cnt_update(int unit, bcm_gport_t gport,
        int vp, int incr_decr_flag)
{
    int mod_out=-1, port_out=-1, tgid_out=-1, vp_out=-1;
    bcm_port_t local_member_array[SOC_MAX_NUM_PORTS];
    int local_member_count=0;
    int idx=-1;
    int mod_local=-1;
    _bcm_port_info_t *port_info;

    BCM_IF_ERROR_RETURN(
       _bcm_td2_vxlan_port_resolve(unit, gport, -1, &mod_out,
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
 *      bcm_td2_vxlan_vpn_create
 * Purpose:
 *      Create a VPN instance
 * Parameters:
 *      unit  - (IN)  Device Number
 *      info  - (IN/OUT) VPN configuration info
 * Returns:
 *      BCM_E_XXX
 */

int 
bcm_td2_vxlan_vpn_create(int unit, bcm_vxlan_vpn_config_t *info)
{
    int rv = BCM_E_PARAM;
    vfi_entry_t vfi_entry;
    vfi_entry_t old_vfi_entry;
    int vfi_index=-1;
    int bc_group=0, umc_group=0, uuc_group=0;
    int bc_group_type=0, umc_group_type=0, uuc_group_type=0;
    uint32 vnid = 0;
    mpls_entry_entry_t ment;
    egr_vlan_xlate_entry_t vxlate_entry;
    egr_vlan_xlate_entry_t old_vxlate_entry;
    int tpid_index = -1;
    int action_present=0, action_not_present=0;
    uint8 vpn_alloc_flag=0;
    uint8 vnid_alloc_flag = 0;
    int proto_pkt_idx = 0;

     /*Allocate VFI*/
     if (info->flags & BCM_VXLAN_VPN_REPLACE) {
         BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_vpn_is_valid(unit, info->vpn));
     } else if (info->flags & BCM_VXLAN_VPN_WITH_ID) {
         rv = _bcm_td2_vxlan_vpn_is_valid(unit, info->vpn);
         if (BCM_E_NONE == rv) {
             return BCM_E_EXISTS;
         } else if (BCM_E_NOT_FOUND != rv) {
             return rv;
         }
         _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VPN_TYPE_VFI, info->vpn);
         BCM_IF_ERROR_RETURN(_bcm_vfi_alloc_with_id(unit, VFIm, _bcmVfiTypeVxlan, vfi_index));
         vpn_alloc_flag = 1;
     } else {
         BCM_IF_ERROR_RETURN(_bcm_vfi_alloc(unit, VFIm, _bcmVfiTypeVxlan, &vfi_index));
         _BCM_VXLAN_VPN_SET(info->vpn, _BCM_VPN_TYPE_VFI, vfi_index);
         vpn_alloc_flag = 1;
     }

    /*Initial and configure VFI*/
    sal_memset(&vfi_entry, 0, sizeof(vfi_entry_t));
    if (info->flags & BCM_VXLAN_VPN_ELINE) {
        soc_VFIm_field32_set(unit, &vfi_entry, PT2PT_ENf, 0x1);
    } else if (info->flags & BCM_VXLAN_VPN_ELAN) {
        /* Check that the groups are valid. */
        bc_group_type = _BCM_MULTICAST_TYPE_GET(info->broadcast_group);
        bc_group = _BCM_MULTICAST_ID_GET(info->broadcast_group);
        umc_group_type = _BCM_MULTICAST_TYPE_GET(info->unknown_multicast_group);
        umc_group = _BCM_MULTICAST_ID_GET(info->unknown_multicast_group);
        uuc_group_type = _BCM_MULTICAST_TYPE_GET(info->unknown_unicast_group);
        uuc_group = _BCM_MULTICAST_ID_GET(info->unknown_unicast_group);

        if ((bc_group_type != _BCM_MULTICAST_TYPE_VXLAN) ||
            (umc_group_type != _BCM_MULTICAST_TYPE_VXLAN) ||
            (uuc_group_type != _BCM_MULTICAST_TYPE_VXLAN) ||
            (bc_group >= soc_mem_index_count(unit, L3_IPMCm)) ||
            (umc_group >= soc_mem_index_count(unit, L3_IPMCm)) ||
            (uuc_group >= soc_mem_index_count(unit, L3_IPMCm))) {
            rv = BCM_E_PARAM;
            _BCM_VXLAN_CLEANUP(rv)
        }

        /* Commit the entry to HW */
        soc_VFIm_field32_set(unit, &vfi_entry, UMC_INDEXf, umc_group);
        soc_VFIm_field32_set(unit, &vfi_entry, UUC_INDEXf, uuc_group);
        soc_VFIm_field32_set(unit, &vfi_entry, BC_INDEXf, bc_group);
    }

    /* Configure protocol packet control */
    rv = _bcm_xgs3_protocol_packet_actions_validate(unit, &info->protocol_pkt);
    _BCM_VXLAN_CLEANUP(rv)
    if (info->flags & BCM_VXLAN_VPN_REPLACE) {
        _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VPN_TYPE_VFI, info->vpn);
        rv = READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &old_vfi_entry);
        _BCM_VXLAN_CLEANUP(rv)
        proto_pkt_idx = soc_VFIm_field32_get(unit, &old_vfi_entry, PROTOCOL_PKT_INDEXf);
    }
    rv = _bcm_xgs3_protocol_pkt_ctrl_set(unit, proto_pkt_idx, &info->protocol_pkt, &proto_pkt_idx);
    _BCM_VXLAN_CLEANUP(rv)
    soc_VFIm_field32_set(unit, &vfi_entry, PROTOCOL_PKT_INDEXf, proto_pkt_idx);

    /*Write VFI to ASIC*/
    rv = WRITE_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry);
    _BCM_VXLAN_CLEANUP(rv)

    if (info->flags & BCM_VXLAN_VPN_WITH_VPNID) {
        if ((info->vnid == 0) || (info->vnid > 0xFFFFFF)) {
           rv = BCM_E_PARAM;
           _BCM_VXLAN_CLEANUP(rv)
        }
        /* Configure Ingress VNID */
        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VXLAN_VN_ID__VFIf, vfi_index);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, KEY_TYPEf, _BCM_VXLAN_KEY_TYPE_VNID_VFI);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 1);
        if (info->flags & BCM_VXLAN_VPN_REPLACE) {
            /*Delete old match vnid entry*/
            rv = _bcm_td2_vxlan_egr_xlate_entry_get(unit, vfi_index,  &old_vxlate_entry);
            if (rv == BCM_E_NONE) {
                vnid = soc_EGR_VLAN_XLATEm_field32_get(unit, &old_vxlate_entry, VXLAN_VFI__VN_IDf);
                soc_MPLS_ENTRYm_field32_set(unit, &ment, VXLAN_VN_ID__VN_IDf, vnid);
                rv = soc_mem_delete(unit, MPLS_ENTRYm, MEM_BLOCK_ALL, &ment); 
                _BCM_VXLAN_CLEANUP(rv)
            } else if (rv != BCM_E_NOT_FOUND) {
                _BCM_VXLAN_CLEANUP(rv)
            }            
        }
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VXLAN_VN_ID__VN_IDf, info->vnid);
        rv = soc_mem_insert(unit, MPLS_ENTRYm, MEM_BLOCK_ALL, &ment);
        _BCM_VXLAN_CLEANUP(rv)
        vnid_alloc_flag = 1;

        /* Configure Egress VNID */
        sal_memset(&vxlate_entry, 0, sizeof(egr_vlan_xlate_entry_t));
        soc_EGR_VLAN_XLATEm_field32_set(
            unit, &vxlate_entry, ENTRY_TYPEf, _BCM_VXLAN_KEY_TYPE_LOOKUP_VFI);
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VALIDf, 0x1);
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VXLAN_VFI__VFIf, vfi_index);
        soc_EGR_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VXLAN_VFI__VN_IDf, info->vnid);

        /* Configure EGR_VLAN_XLATE - SD_TAG setting */
        if (info->flags & BCM_VXLAN_VPN_SERVICE_TAGGED) {
            if (info->flags & BCM_VXLAN_VPN_REPLACE) {
                /*If old tpid exist, delete it*/
                action_present = soc_EGR_VLAN_XLATEm_field32_get(unit, &old_vxlate_entry,
                                                             VXLAN_VFI__SD_TAG_ACTION_IF_PRESENTf);
                action_not_present = soc_EGR_VLAN_XLATEm_field32_get(unit, &old_vxlate_entry,
                                                         VXLAN_VFI__SD_TAG_ACTION_IF_NOT_PRESENTf);
                if (1 == action_present || 4 == action_present ||
                    7 == action_present || 1 == action_not_present) {
                    tpid_index = soc_EGR_VLAN_XLATEm_field32_get(unit,
                                     &old_vxlate_entry, VXLAN_VFI__SD_TAG_TPID_INDEXf);
                    rv = _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
                    if ((rv != BCM_E_NONE) && (rv != BCM_E_NOT_FOUND)) {
                        _BCM_VXLAN_CLEANUP(rv)
                    }            
                }
            }

            rv = _bcm_td2_vxlan_sd_tag_set(unit, info, NULL, NULL, &vxlate_entry, &tpid_index);
            _BCM_VXLAN_CLEANUP(rv)
        }

        if (info->flags & BCM_VXLAN_VPN_REPLACE) {
            rv = soc_mem_delete(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &old_vxlate_entry);
            if ((rv != BCM_E_NONE) && (rv != BCM_E_NOT_FOUND)) {
                _BCM_VXLAN_CLEANUP(rv)
            }            

        }
        rv = soc_mem_insert(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &vxlate_entry);
        _BCM_VXLAN_CLEANUP(rv)
        
    } else {

        sal_memset(&vxlate_entry, 0, sizeof(egr_vlan_xlate_entry_t));

        /* Get vfi index */
        _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VPN_TYPE_VFI, info->vpn);

        /* Reset match vnid table */
        rv = _bcm_td2_vxlan_egr_xlate_entry_get(unit, vfi_index, &vxlate_entry);
        if (rv == BCM_E_NONE) {         
            vnid = soc_EGR_VLAN_XLATEm_field32_get(unit, &vxlate_entry, VXLAN_VFI__VN_IDf);
            /* Delete mpls_entry */
            rv = _bcm_td2_vxlan_match_vnid_entry_reset(unit, vnid);
            if ((rv != BCM_E_NONE) && (rv != BCM_E_NOT_FOUND)) {
                return rv;    
            }
            /* Delete egr_vxlate entry */
            rv = _bcm_td2_vxlan_egr_xlate_entry_reset(unit, info->vpn);            
            if ((rv != BCM_E_NONE) && (rv != BCM_E_NOT_FOUND)) {
                return rv;    
            }            
        } else if (rv != BCM_E_NOT_FOUND) {
            return rv;
        }

    }

    return BCM_E_NONE;

cleanup:
    if (tpid_index != -1) {
        (void) _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
    }
    if (vnid_alloc_flag) {
        soc_mem_delete(unit, MPLS_ENTRYm, MEM_BLOCK_ALL, &ment);
    }
    if ( vpn_alloc_flag ) {
        (void) _bcm_vfi_free(unit, _bcmVfiTypeVxlan, vfi_index);
    }
    return rv;
}

 /* Function:
 *      bcm_td2_vxlan_vpn_vnid_get
 * Purpose:
 *      Get VNID per VPN instance
 * Parameters:
 *      unit     - Device Number
 *      l2vpn   - VXLAN VPN
 *      info     - VXLAN VPN Config
 * Returns:
 *      BCM_E_XXXX
 */

STATIC int 
_bcm_td2_vxlan_egr_xlate_entry_get( int unit, int vfi, egr_vlan_xlate_entry_t *vxlate_entry)
{
    egr_vlan_xlate_entry_t   key_vxlate;
    int index=0;

    if (!soc_mem_index_count(unit, EGR_VLAN_XLATEm)) {
        return BCM_E_NOT_FOUND;
    }

    sal_memset(&key_vxlate, 0, sizeof(egr_vlan_xlate_entry_t));

    soc_EGR_VLAN_XLATEm_field32_set(
        unit, &key_vxlate, ENTRY_TYPEf, _BCM_VXLAN_KEY_TYPE_LOOKUP_VFI);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &key_vxlate, VALIDf, 1);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &key_vxlate, VXLAN_VFI__VFIf, vfi);

    return soc_mem_search(unit, EGR_VLAN_XLATEm,
                          MEM_BLOCK_ANY, &index, &key_vxlate, vxlate_entry, 0);
}


/*
 * Function:
 *      bcm_td2_vxlan_vpn_destroy
 * Purpose:
 *      Delete a VPN instance
 * Parameters:
 *      unit - Device Number
 *      vpn - VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_td2_vxlan_vpn_destroy(int unit, bcm_vpn_t l2vpn)
{
    int vfi = 0;
    vfi_entry_t vfi_entry;
    uint8 isEline = 0;
    uint32 vnid = 0;
    uint32 stat_counter_id;
    int num_ctr = 0; 
    int ref_count;
    int proto_pkt_inx;
    egr_vlan_xlate_entry_t vxlate_entry;
    int rv = 0;

    /* Get vfi index */
    BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_vpn_is_eline(unit, l2vpn, &isEline));
    if (isEline == 0x1 ) {
        _BCM_VXLAN_VPN_GET(vfi, _BCM_VXLAN_VPN_TYPE_ELINE, l2vpn);
    } else if (isEline == 0x0 ) {
        _BCM_VXLAN_VPN_GET(vfi, _BCM_VXLAN_VPN_TYPE_ELAN, l2vpn);
    }

    /* Reset match vnid table */
    rv = _bcm_td2_vxlan_egr_xlate_entry_get(unit, vfi, &vxlate_entry);
    if (rv == BCM_E_NONE) {         
        vnid = soc_EGR_VLAN_XLATEm_field32_get(unit, &vxlate_entry, VXLAN_VFI__VN_IDf);
        /* Delete mpls_entry */
        rv = _bcm_td2_vxlan_match_vnid_entry_reset(unit, vnid);
        if ((rv != BCM_E_NONE) && (rv != BCM_E_NOT_FOUND)) {
            return rv;    
        }
        /* Delete egr_vxlate entry */
        rv = _bcm_td2_vxlan_egr_xlate_entry_reset(unit, l2vpn);            
        if ((rv != BCM_E_NONE) && (rv != BCM_E_NOT_FOUND)) {
            return rv;    
        }            
    } else if (rv != BCM_E_NOT_FOUND) {
        return rv;
    }

    /* Delete all VXLAN ports on this VPN */
    BCM_IF_ERROR_RETURN(bcm_td2_vxlan_port_delete_all(unit, l2vpn));

    /* Check counters before delete vpn */
    if (bcm_esw_vxlan_stat_id_get(unit, BCM_GPORT_INVALID, l2vpn,
                   bcmVxlanOutPackets, &stat_counter_id) == BCM_E_NONE) { 
        num_ctr++;
    } 
    if (bcm_esw_vxlan_stat_id_get(unit, BCM_GPORT_INVALID, l2vpn,
                   bcmVxlanInPackets, &stat_counter_id) == BCM_E_NONE) {
        num_ctr++;
    }

    if (num_ctr != 0) {
        BCM_IF_ERROR_RETURN(
           bcm_esw_vxlan_stat_detach(unit, BCM_GPORT_INVALID, l2vpn));  
    }

    /* Delete protocol packet control */
    sal_memset(&vfi_entry, 0, sizeof(vfi_entry_t));
    BCM_IF_ERROR_RETURN(READ_VFIm(unit, MEM_BLOCK_ALL, vfi, &vfi_entry));
    proto_pkt_inx = soc_VFIm_field32_get(unit, &vfi_entry, PROTOCOL_PKT_INDEXf);
    BCM_IF_ERROR_RETURN(
        _bcm_prot_pkt_ctrl_ref_count_get(unit, proto_pkt_inx, &ref_count));
    if (ref_count > 0) {
        BCM_IF_ERROR_RETURN(_bcm_prot_pkt_ctrl_delete(unit,proto_pkt_inx));
    }

    /* Reset VFI table */
    sal_memset(&vfi_entry, 0, sizeof(vfi_entry_t));
    BCM_IF_ERROR_RETURN(WRITE_VFIm(unit, MEM_BLOCK_ALL, vfi, &vfi_entry));
    (void) _bcm_vfi_free(unit, _bcmVfiTypeVxlan, vfi);

    return BCM_E_NONE;
}

 /* Function:
 *      bcm_td2_vxlan_vpn_id_destroy_all
 * Purpose:
 *      Delete all L2-VPN instances
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXXX
 */

int 
bcm_td2_vxlan_vpn_destroy_all(int unit)
{
    int num_vfi = 0, idx = 0;
    bcm_vpn_t l2vpn = 0;

    /* Destroy all VXLAN VPNs */
    num_vfi = soc_mem_index_count(unit, VFIm);
    for (idx = 0; idx < num_vfi; idx++) {
        if (_bcm_vfi_used_get(unit, idx, _bcmVfiTypeVxlan)) {
            _BCM_VXLAN_VPN_SET(l2vpn, _BCM_VPN_TYPE_VFI, idx);
            BCM_IF_ERROR_RETURN(bcm_td2_vxlan_vpn_destroy(unit, l2vpn));
        }
    }
    return BCM_E_NONE;
}

 /* Function:
 *      bcm_td2_vxlan_vpn_get
 * Purpose:
 *      Get L2-VPN instance
 * Parameters:
 *      unit     - Device Number
 *      l2vpn   - VXLAN VPN
 *      info     - VXLAN VPN Config
 * Returns:
 *      BCM_E_XXXX
 */

int 
bcm_td2_vxlan_vpn_get( int unit, bcm_vpn_t l2vpn, bcm_vxlan_vpn_config_t *info)
{
    int vfi_index = -1;
    vfi_entry_t vfi_entry;
    uint8 isEline=0;
    int rv = BCM_E_NONE;
    egr_vlan_xlate_entry_t   vxlate_entry;
    int proto_pkt_inx;

    /*Get vfi table*/
    BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_vpn_is_eline(unit, l2vpn, &isEline));
    if (isEline == 0x1 ) {
        _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VXLAN_VPN_TYPE_ELINE, l2vpn);
    } else if (isEline == 0x0 ) {
        _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VXLAN_VPN_TYPE_ELAN, l2vpn);
    }
    BCM_IF_ERROR_RETURN(READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry));

    /* Get info associated with vfi table */
    if (isEline) {
        info->flags =  BCM_VXLAN_VPN_ELINE;
    } else {
         info->flags =  BCM_VXLAN_VPN_ELAN;
         
         _BCM_MULTICAST_GROUP_SET(info->broadcast_group,
                             _BCM_MULTICAST_TYPE_VXLAN,
              soc_VFIm_field32_get(unit, &vfi_entry, BC_INDEXf));
         _BCM_MULTICAST_GROUP_SET(info->unknown_unicast_group, 
                             _BCM_MULTICAST_TYPE_VXLAN,  
              soc_VFIm_field32_get(unit, &vfi_entry, UUC_INDEXf));
         _BCM_MULTICAST_GROUP_SET(info->unknown_multicast_group, 
                             _BCM_MULTICAST_TYPE_VXLAN,  
              soc_VFIm_field32_get(unit, &vfi_entry, UMC_INDEXf));
    }
    _BCM_VXLAN_VPN_SET(info->vpn, _BCM_VPN_TYPE_VFI, vfi_index);
    /*Get protocol packet control*/
    proto_pkt_inx = soc_VFIm_field32_get(unit, &vfi_entry, PROTOCOL_PKT_INDEXf);
    BCM_IF_ERROR_RETURN(_bcm_xgs3_protocol_pkt_ctrl_get(unit,proto_pkt_inx, &info->protocol_pkt));

    /* Get VNID */
    rv = _bcm_td2_vxlan_egr_xlate_entry_get(unit, vfi_index,  &vxlate_entry);
    if (rv == BCM_E_NONE) {
        info->flags |= BCM_VXLAN_VPN_WITH_VPNID;
        info->vnid = soc_EGR_VLAN_XLATEm_field32_get(unit, &vxlate_entry, VXLAN_VFI__VN_IDf);
        /* Get SDTAG settings */
        (void)_bcm_td2_vxlan_sd_tag_get( unit, info, NULL, NULL, &vxlate_entry, 1);
    } else if (rv == BCM_E_NOT_FOUND) {
        rv = BCM_E_NONE;
    }

    return rv;
}

 /* Function:
 *      _bcm_td2_vxlan_vpn_get
 * Purpose:
 *      Get L2-VPN instance for VXLAN
 * Parameters:
 *      unit     - Device Number
 *      vfi_index   - vfi_index 
 *      vid     (OUT) VLAN Id (l2vpn id)

 * Returns:
 *      BCM_E_XXXX
 */
int 
_bcm_td2_vxlan_vpn_get(int unit, int vfi_index, bcm_vlan_t *vid)
{
    vfi_entry_t vfi_entry;

    BCM_IF_ERROR_RETURN
        (READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry));

    if (soc_VFIm_field32_get(unit, &vfi_entry, PT2PT_ENf)) {
        _BCM_VXLAN_VPN_SET(*vid, _BCM_VXLAN_VPN_TYPE_ELINE, vfi_index);
    } else {
        _BCM_VXLAN_VPN_SET(*vid, _BCM_VXLAN_VPN_TYPE_ELAN, vfi_index);
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_vxlan_vpn_traverse
 * Purpose:
 *      Get information about a VPN instance
 * Parameters:
 *      unit  - (IN)  Device Number
 *      cb    - (IN)  User-provided callback
 *      info  - (IN/OUT) Cookie
 * Returns:
 *      BCM_E_XXX
 */

int bcm_td2_vxlan_vpn_traverse(int unit, bcm_vxlan_vpn_traverse_cb cb, 
                             void *user_data)
{
    int idx, num_vfi;
    int vpn;
    bcm_vxlan_vpn_config_t info;

    if (cb == NULL) {
         return BCM_E_PARAM;
    }

    /* VXLAN VPNs */
    num_vfi = soc_mem_index_count(unit, VFIm);
    for (idx = 0; idx < num_vfi; idx++) {
         if (_bcm_vfi_used_get(unit, idx, _bcmVfiTypeVxlan)) {
              _BCM_VXLAN_VPN_SET(vpn, _BCM_VPN_TYPE_VFI, idx);
              bcm_vxlan_vpn_config_t_init(&info);
              BCM_IF_ERROR_RETURN(bcm_td2_vxlan_vpn_get(unit, vpn, &info));
              BCM_IF_ERROR_RETURN(cb(unit, &info, user_data));
         }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *          _bcm_td2_vxlan_tunnel_terminate_entry_key_set
 * Purpose:
 *          Set VXLAN Tunnel Terminate entry key set
 * Parameters:
 *  IN :  Unit
 *  IN :  tnl_info
 *  IN(OUT) : tr_ent VLAN_XLATE entry pointer
 *  IN : clean_flag
 * Returns: BCM_E_XXX
 */

STATIC void
_bcm_td2_vxlan_tunnel_terminate_entry_key_set(int unit, bcm_tunnel_terminator_t *tnl_info,
                                         vlan_xlate_entry_t   *tr_ent, uint8 out_mode, int clean_flag)
{
    if (clean_flag) {
         sal_memset(tr_ent, 0, sizeof(vlan_xlate_entry_t));
    }

    soc_VLAN_XLATEm_field32_set(unit, tr_ent, KEY_TYPEf, 
                                    _BCM_VXLAN_KEY_TYPE_LOOKUP_DIP);
    soc_VLAN_XLATEm_field32_set(unit, tr_ent, VALIDf, 0x1);
    soc_VLAN_XLATEm_field32_set(unit, tr_ent,  
                                    VXLAN_DIP__DIPf, tnl_info->dip);
    if (out_mode == _BCM_VXLAN_MULTICAST_BUD) {
        soc_VLAN_XLATEm_field32_set(unit, tr_ent,  
                                    VXLAN_DIP__NETWORK_RECEIVERS_PRESENTf, 0x1);
        _bcm_td2_vxlan_bud_loopback_enable(unit);
    } else if (out_mode == _BCM_VXLAN_MULTICAST_LEAF) {
        soc_VLAN_XLATEm_field32_set(unit, tr_ent,  
                                    VXLAN_DIP__NETWORK_RECEIVERS_PRESENTf, 0x0);
    }

    soc_VLAN_XLATEm_field32_set(unit, tr_ent,
                                VXLAN_DIP__IGNORE_UDP_CHECKSUMf,
           (tnl_info->flags & BCM_TUNNEL_TERM_UDP_CHECKSUM_ENABLE) ? 0x0 : 0x1);
}

/*
 * Function:
 *          _bcm_td2_vxlan_tunnel_terminator_reference_count
 * Purpose:
 *          Find VXLAN Tunnel terminator DIP reference count
 * Returns: BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_tunnel_terminator_reference_count(int unit, bcm_ip_t dest_ip_addr)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int tunnel_idx=0, num_vp=0, ref_count=0;

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    vxlan_info = VXLAN_INFO(unit);

      for (tunnel_idx=0; tunnel_idx< num_vp; tunnel_idx++) {
          if (vxlan_info->vxlan_tunnel_term[tunnel_idx].dip == dest_ip_addr) {
              ref_count ++;
          }
      }
    return ref_count;
}

/*
 * Function:
 *          _bcm_td2_vxlan_multicast_tunnel_state_set
 * Purpose:
 *          Set VXLAN Tunnel terminator Multicast State
 * Returns: BCM_E_XXX
 */


STATIC void
_bcm_td2_vxlan_multicast_tunnel_state_set (int unit, int  tunnel_idx, uint16 tunnel_state)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;

    vxlan_info = VXLAN_INFO(unit);

    if (tunnel_idx != -1) {
        vxlan_info->vxlan_tunnel_term[tunnel_idx].tunnel_state = tunnel_state;
    }
}

/*
 * Function:
 *          _bcm_td2_vxlan_multicast_tunnel_state_index_get
 * Purpose:
 *          Get VXLAN Tunnel terminator Multicast State and Tunnel Index
 * Returns: BCM_E_XXX
 */


STATIC void
_bcm_td2_vxlan_multicast_tunnel_state_index_get (int unit, 
                            bcm_ip_t mc_ip_addr, bcm_ip_t  src_ip_addr, uint16 *tunnel_state, int *tunnel_index)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int idx=0, num_vp=0;

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    vxlan_info = VXLAN_INFO(unit);

      for (idx=0; idx< num_vp; idx++) {
          if (vxlan_info->vxlan_tunnel_term[idx].dip == mc_ip_addr) {
              if (vxlan_info->vxlan_tunnel_term[idx].sip == src_ip_addr) {
                    *tunnel_state = vxlan_info->vxlan_tunnel_term[idx].tunnel_state;
                    *tunnel_index = idx;
                     break;
              }
          }
      }
}

/*
 * Function:
 *          _bcm_td2_vxlan_tunnel_terminator_state_find
 * Purpose:
 *          Find VXLAN Tunnel terminator DIP State
 * Returns: BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_tunnel_terminator_state_find(int unit, bcm_ip_t dest_ip_addr)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int tunnel_idx=0, num_vp=0, tunnel_state=0;

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);
    vxlan_info = VXLAN_INFO(unit);

    for (tunnel_idx=0; tunnel_idx< num_vp; tunnel_idx++) {
          if (vxlan_info->vxlan_tunnel_term[tunnel_idx].dip == dest_ip_addr) {
              if (vxlan_info->vxlan_tunnel_term[tunnel_idx].tunnel_state == _BCM_VXLAN_TUNNEL_TERM_MULTICAST_LEAF ) {
                 tunnel_state += 0;
              } else if (vxlan_info->vxlan_tunnel_term[tunnel_idx].tunnel_state == _BCM_VXLAN_TUNNEL_TERM_MULTICAST_BUD ) {
                 tunnel_state += 1;
              }
          }
    }
    return tunnel_state;
}

/*
 * Function:
 *      bcm_td2_vxlan_multicast_leaf_entry_check
 * Purpose:
 *      To find whether LEAF-multicast entry exists for DIP?
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_td2_vxlan_multicast_leaf_entry_check(int unit, bcm_ip_t mc_ip_addr, bcm_ip_t  src_ip_addr, int multicast_flag)
{
    vlan_xlate_entry_t vxlate_entry;
    int rv=BCM_E_NONE, index=0;
    int tunnel_ref_count = 0;
    uint16 tunnel_multicast_state = 0;
    int tunnel_mc_states = 0, tunnel_idx=0;

    tunnel_ref_count = _bcm_td2_vxlan_tunnel_terminator_reference_count(unit, mc_ip_addr);
    if (tunnel_ref_count < 1) {
        /* Tunnel not created */
        return BCM_E_NONE;
    } else if (tunnel_ref_count >= 1) {
         (void ) _bcm_td2_vxlan_multicast_tunnel_state_index_get (unit, 
                           mc_ip_addr, src_ip_addr, &tunnel_multicast_state, &tunnel_idx);

          /* Set (SIP, DIP) based Software State */
          if (multicast_flag == BCM_VXLAN_MULTICAST_TUNNEL_STATE_BUD_ENABLE) {
              if (tunnel_multicast_state == _BCM_VXLAN_TUNNEL_TERM_MULTICAST_LEAF) {
                   (void) _bcm_td2_vxlan_multicast_tunnel_state_set(unit, 
                                 tunnel_idx, _BCM_VXLAN_TUNNEL_TERM_MULTICAST_BUD);
              }
          } else  if (multicast_flag == BCM_VXLAN_MULTICAST_TUNNEL_STATE_BUD_DISABLE){
              if (tunnel_multicast_state == _BCM_VXLAN_TUNNEL_TERM_MULTICAST_BUD) {
                   (void) _bcm_td2_vxlan_multicast_tunnel_state_set(unit, 
                                 tunnel_idx, _BCM_VXLAN_TUNNEL_TERM_MULTICAST_LEAF);
              }
          }

          /* Set DIP-based cumulative hardware State */
         tunnel_mc_states = _bcm_td2_vxlan_tunnel_terminator_state_find(unit, mc_ip_addr);

         sal_memset(&vxlate_entry, 0, sizeof(vlan_xlate_entry_t));
         soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, KEY_TYPEf, 
                                    _BCM_VXLAN_KEY_TYPE_LOOKUP_DIP); 
         soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VALIDf, 0x1);
         soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    VXLAN_DIP__DIPf, mc_ip_addr);

         rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                        &vxlate_entry, &vxlate_entry, 0);

         if (rv == SOC_E_NONE) {
              if (tunnel_mc_states) {
                   /* Create Transit entry: LEAF to BUD */
                   soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    VXLAN_DIP__NETWORK_RECEIVERS_PRESENTf, 0x1);
                   _bcm_td2_vxlan_bud_loopback_enable(unit);
              } else {
                   /* Delete Transit entry : BUD to LEAF */
                   soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    VXLAN_DIP__NETWORK_RECEIVERS_PRESENTf, 0x0);
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
 *      bcm_td2_vxlan_tunnel_terminator_create
 * Purpose:
 *      Set VXLAN Tunnel terminator
 * Parameters:
 *      unit - Device Number
 *      tnl_info
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_vxlan_tunnel_terminator_create(int unit, bcm_tunnel_terminator_t *tnl_info)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int index=-1, tunnel_idx=-1;
    vlan_xlate_entry_t vxlate_entry, return_entry;
    int rv = BCM_E_NONE;

    if (tnl_info->type != bcmTunnelTypeVxlan) {
       return BCM_E_PARAM;
    }

    vxlan_info = VXLAN_INFO(unit);

    /* Program tunnel id */
    if (tnl_info->flags & BCM_TUNNEL_TERM_TUNNEL_WITH_ID) {
        if (!BCM_GPORT_IS_TUNNEL(tnl_info->tunnel_id)) {
            return BCM_E_PARAM;
        }
        if ((0 == tnl_info->dip) || (0 == tnl_info->sip)) {
            return BCM_E_PARAM; 
        }
        tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(tnl_info->tunnel_id);
        if ((tunnel_idx <= 0) || (tunnel_idx >= soc_mem_index_count(unit, SOURCE_VPm))) {
            return BCM_E_BADID; 
        }  
    } else {
        /* Only BCM_TUNNEL_TERM_TUNNEL_WITH_ID supported */
        /* Use tunnel_id retuned by _create */
        return BCM_E_PARAM; 
    }
    if(( 0 != vxlan_info->vxlan_tunnel_term[tunnel_idx].dip) && 
        (vxlan_info->vxlan_tunnel_term[tunnel_idx].dip != tnl_info->dip)) {
        /* Entry exists with the same idx*/
        return BCM_E_EXISTS;
    }

    /* Add First Tunnel term-entry for given DIP to Hardware */
    if (_bcm_td2_vxlan_tunnel_terminator_reference_count(unit, tnl_info->dip) < 1)  {

         (void) _bcm_td2_vxlan_tunnel_terminate_entry_key_set(unit, tnl_info, &vxlate_entry, 0, 1);

         rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                        &vxlate_entry, &return_entry, 0);

         if (rv == SOC_E_NONE) {
              (void) _bcm_td2_vxlan_tunnel_terminate_entry_key_set(unit, tnl_info, &vxlate_entry, 0, 0);
              rv = soc_mem_write(unit, VLAN_XLATEm,
                                           MEM_BLOCK_ALL, index,
                                           &return_entry);
         } else if (rv != SOC_E_NOT_FOUND) {
              return rv;
         } else {
              rv = soc_mem_insert(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vxlate_entry);
         }
    }

    memset(&(vxlan_info->vxlan_tunnel_term[tunnel_idx]), 0, sizeof(_bcm_vxlan_tunnel_endpoint_t));
    if (BCM_SUCCESS(rv) && (tunnel_idx != -1)) {
       vxlan_info->vxlan_tunnel_term[tunnel_idx].sip = tnl_info->sip;
       vxlan_info->vxlan_tunnel_term[tunnel_idx].dip = tnl_info->dip;
       vxlan_info->vxlan_tunnel_term[tunnel_idx].tunnel_state = _BCM_VXLAN_TUNNEL_TERM_MULTICAST_LEAF;
       vxlan_info->vxlan_tunnel_term[tunnel_idx].activate_flag |= _BCM_VXLAN_TUNNEL_TERM_ENABLE;
       if (tnl_info->flags & BCM_TUNNEL_TERM_UDP_CHECKSUM_ENABLE) {    
           vxlan_info->vxlan_tunnel_term[tunnel_idx].activate_flag |= 
                _BCM_VXLAN_TUNNEL_TERM_UDP_CHECKSUM_ENABLE;
       }
    }
    return rv;
}

/*  
 * Function:
 *      bcm_td2_vxlan_tunnel_terminator_update
 * Purpose:
 *     Update VXLAN Tunnel terminator multicast state
 * Parameters:
 *      unit - Device Number
 *      tnl_info
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_vxlan_tunnel_terminator_update(int unit, bcm_tunnel_terminator_t *tnl_info)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int rv = BCM_E_NONE;
    int tunnel_idx=-1, index=-1;
    bcm_ip_t mc_ip_addr, src_ip_addr;
    uint32  tunnel_dip=0;
    vlan_xlate_entry_t  vxlate_entry;

    vxlan_info = VXLAN_INFO(unit);

    /* Program tunnel id */
    if (tnl_info->flags & BCM_TUNNEL_TERM_TUNNEL_WITH_ID) {
        if (!BCM_GPORT_IS_TUNNEL(tnl_info->tunnel_id)) {
            return BCM_E_PARAM;
        }
        tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(tnl_info->tunnel_id);
        if ((tunnel_idx <= 0) || (tunnel_idx >= soc_mem_index_count(unit, SOURCE_VPm))) {
            return BCM_E_BADID; 
        }  
    } else {
        /* Only BCM_TUNNEL_TERM_TUNNEL_WITH_ID supported */
        /* Use tunnel_id retuned by _create */
        return BCM_E_PARAM; 
    }


    tunnel_dip = vxlan_info->vxlan_tunnel_term[tunnel_idx].dip;

    sal_memset(&vxlate_entry, 0, sizeof(vlan_xlate_entry_t));    
    soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, KEY_TYPEf, 
                                    _BCM_VXLAN_KEY_TYPE_LOOKUP_DIP); 
    soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VALIDf, 0x1);
    soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    VXLAN_DIP__DIPf, tunnel_dip);

    if (tnl_info->flags & BCM_TUNNEL_TERM_TUNNEL_DEACTIVATE) {        
        rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                                 &vxlate_entry, &vxlate_entry, 0);
        if (rv == BCM_E_NONE) {
            rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vxlate_entry);
            if (BCM_FAILURE(rv)) {
                 return rv;
            }
        } else if (rv != BCM_E_NOT_FOUND) {
            return rv;
        }  
        vxlan_info->vxlan_tunnel_term[tunnel_idx].activate_flag &= (~_BCM_VXLAN_TUNNEL_TERM_ENABLE);
        return BCM_E_NONE;
    } else if (vxlan_info->vxlan_tunnel_term[tunnel_idx].activate_flag == _BCM_VXLAN_TUNNEL_TERM_DISABLE) {    
        if (vxlan_info->vxlan_tunnel_term[tunnel_idx].dip != 0) {
            soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,
                                        VXLAN_DIP__IGNORE_UDP_CHECKSUMf,
                   (vxlan_info->vxlan_tunnel_term[tunnel_idx].activate_flag 
                   & _BCM_VXLAN_TUNNEL_TERM_UDP_CHECKSUM_ENABLE) ? 0x0 : 0x1);
				   
            rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                                     &vxlate_entry, &vxlate_entry, 0);        
            if (rv == BCM_E_NOT_FOUND) {
                rv = soc_mem_insert(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vxlate_entry);
                if (BCM_FAILURE(rv)) {
                     return rv;
                }
            } else if (rv != BCM_E_NONE) {
                return rv;
            }
            vxlan_info->vxlan_tunnel_term[tunnel_idx].activate_flag |= _BCM_VXLAN_TUNNEL_TERM_ENABLE;
        }
    }

    mc_ip_addr = vxlan_info->vxlan_tunnel_term[tunnel_idx].dip;
    src_ip_addr = vxlan_info->vxlan_tunnel_term[tunnel_idx].sip;


    rv = bcm_td2_vxlan_multicast_leaf_entry_check(unit, 
                   mc_ip_addr, src_ip_addr, tnl_info->multicast_flag);
    return rv;
}

/*
 * Function:
 *  bcm_td2_vxlan_tunnel_terminate_destroy
 * Purpose:
 *  Destroy VXLAN  tunnel_terminate Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  vxlan_tunnel_id
 * Returns:
 *  BCM_E_XXX
 */

int
bcm_td2_vxlan_tunnel_terminator_destroy(int unit, bcm_gport_t vxlan_tunnel_id)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int tunnel_idx=-1, index=-1;
    vlan_xlate_entry_t  vxlate_entry;
    uint32  tunnel_dip=0;
    int rv = BCM_E_NONE;

    vxlan_info = VXLAN_INFO(unit);

    if (!BCM_GPORT_IS_TUNNEL(vxlan_tunnel_id)) {
        return BCM_E_PARAM;
    }
    tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(vxlan_tunnel_id);

    if ((tunnel_idx <= 0) || (tunnel_idx >= soc_mem_index_count(unit, SOURCE_VPm))) {
        return BCM_E_BADID; 
    }  
    sal_memset(&vxlate_entry, 0, sizeof(vlan_xlate_entry_t));
    tunnel_dip = vxlan_info->vxlan_tunnel_term[tunnel_idx].dip;
    if ((0 != tunnel_dip ) &&
        (!(_BCM_VXLAN_TUNNEL_TERM_ENABLE & vxlan_info->vxlan_tunnel_term[tunnel_idx].activate_flag))) {
        /* Deactivated tunnel terminator can not be destroyed */
        return BCM_E_DISABLED;
    }

    /* Remove Tunnel term-entry for given DIP from Hardware based on ref-count */
    if (_bcm_td2_vxlan_tunnel_terminator_reference_count(unit, tunnel_dip) == 1)  {

         soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, KEY_TYPEf, 
                                    _BCM_VXLAN_KEY_TYPE_LOOKUP_DIP); 
         soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VALIDf, 0x1);
         soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    VXLAN_DIP__DIPf, tunnel_dip);
         soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    VXLAN_DIP__NETWORK_RECEIVERS_PRESENTf, 0);

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

    /* Activate the entry , then Delete the entry */
    if (BCM_SUCCESS(rv) && (tunnel_idx != -1) && 
        (vxlan_info->vxlan_tunnel_term[tunnel_idx].activate_flag & _BCM_VXLAN_TUNNEL_TERM_ENABLE)) {
       vxlan_info->vxlan_tunnel_term[tunnel_idx].dip = 0;
       vxlan_info->vxlan_tunnel_term[tunnel_idx].tunnel_state = _BCM_VXLAN_TUNNEL_TERM_MULTICAST_LEAF;       
       vxlan_info->vxlan_tunnel_term[tunnel_idx].activate_flag = 0;
    }
 
    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_tunnel_terminator_destroy_all
 * Purpose:
 *      Destroy all incoming VXLAN tunnel
 * Parameters:
 *      unit           - (IN) Device Number
 */
int
bcm_td2_vxlan_tunnel_terminator_destroy_all(int unit) 
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int rv = BCM_E_UNAVAIL, i, num_entries=0,tnl_idx=0,num_vp=0;
    vlan_xlate_entry_t  vxlate_entry;

    vxlan_info = VXLAN_INFO(unit);

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
        if (soc_VLAN_XLATEm_field32_get(unit, &vxlate_entry,
                                        KEY_TYPEf) != _BCM_VXLAN_KEY_TYPE_LOOKUP_DIP) {
            continue;
        }
        /* Delete the entry from HW */
        rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ALL, &vxlate_entry);
        if (rv < 0) {
            return rv;
        }
    }

    for (tnl_idx = 0; tnl_idx < num_vp; tnl_idx++) {
       vxlan_info->vxlan_tunnel_term[tnl_idx].dip = 0;
       vxlan_info->vxlan_tunnel_term[tnl_idx].tunnel_state = 0;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_vxlan_tunnel_initiator_get
 * Purpose:
 *      Get VXLAN  tunnel_terminate Entry
 * Parameters:
 *      unit    - (IN) Device Number
 *      info    - (IN/OUT) Tunnel terminator structure
 */
int
bcm_td2_vxlan_tunnel_terminator_get(int unit, bcm_tunnel_terminator_t *tnl_info)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int tunnel_idx=-1, index=-1;
    vlan_xlate_entry_t  vxlate_entry;
    uint32  tunnel_dip=0;
    int rv = BCM_E_UNAVAIL;

    vxlan_info = VXLAN_INFO(unit);

    /* Program tunnel id */
    if (!BCM_GPORT_IS_TUNNEL(tnl_info->tunnel_id)) {
        return BCM_E_PARAM;
    }
    tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(tnl_info->tunnel_id);

    if ((tunnel_idx <= 0) || (tunnel_idx >= soc_mem_index_count(unit, SOURCE_VPm))) {
        return BCM_E_BADID; 
    }  
    sal_memset(&vxlate_entry, 0, sizeof(vlan_xlate_entry_t));
    tunnel_dip = vxlan_info->vxlan_tunnel_term[tunnel_idx].dip;

    soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, KEY_TYPEf, 
                                    _BCM_VXLAN_KEY_TYPE_LOOKUP_DIP); 
    soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VALIDf, 0x1);
    soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    VXLAN_DIP__DIPf, tunnel_dip);

    rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                             &vxlate_entry, &vxlate_entry, 0);
    
    /* not exist or  De-activated */
    if (rv == BCM_E_NOT_FOUND) {
        if (!(vxlan_info->vxlan_tunnel_term[tunnel_idx].activate_flag & _BCM_VXLAN_TUNNEL_TERM_ENABLE) && 
           (vxlan_info->vxlan_tunnel_term[tunnel_idx].dip != 0)) {
            tnl_info->flags |= BCM_TUNNEL_TERM_TUNNEL_DEACTIVATE;
        } else {
            return rv; 
        }
    } else if (rv != BCM_E_NONE) {
        return rv;    
    }

    tnl_info->sip = vxlan_info->vxlan_tunnel_term[tunnel_idx].sip;
    tnl_info->dip = vxlan_info->vxlan_tunnel_term[tunnel_idx].dip;
    tnl_info->type = bcmTunnelTypeVxlan;
    if (vxlan_info->vxlan_tunnel_term[tunnel_idx].tunnel_state == _BCM_VXLAN_TUNNEL_TERM_MULTICAST_BUD) {
        tnl_info->multicast_flag = BCM_VXLAN_MULTICAST_TUNNEL_STATE_BUD_ENABLE;
    } else if (vxlan_info->vxlan_tunnel_term[tunnel_idx].tunnel_state == _BCM_VXLAN_TUNNEL_TERM_MULTICAST_LEAF){
        tnl_info->multicast_flag = BCM_VXLAN_MULTICAST_TUNNEL_STATE_BUD_DISABLE;
    } else {
        tnl_info->multicast_flag = 0;
    }

    if (vxlan_info->vxlan_tunnel_term[tunnel_idx].activate_flag & _BCM_VXLAN_TUNNEL_TERM_UDP_CHECKSUM_ENABLE) {
        tnl_info->flags |= BCM_TUNNEL_TERM_UDP_CHECKSUM_ENABLE;
    }
    
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_vxlan_tunnel_terminator_traverse
 * Purpose:
 *      Traverse VXLAN tunnel terminator entries
 * Parameters:
 *      unit    - (IN) Device Number
 *      cb    - (IN) User callback function, called once per entry
 *      user_data    - (IN) User supplied cookie used in parameter in callback function 
 */
int
bcm_td2_vxlan_tunnel_terminator_traverse(int unit, bcm_tunnel_terminator_traverse_cb cb, void *user_data)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int num_tnl = 0, idx = 0;
    bcm_tunnel_terminator_t info;
    int rv = BCM_E_NONE;

    vxlan_info = VXLAN_INFO(unit);
    num_tnl = soc_mem_index_count(unit, SOURCE_VPm);

    /* Iterate over all the entries - search valid ones. */
    for(idx = 0; idx < num_tnl; idx++) {
        /* Check a valid entry */
        if ((vxlan_info->vxlan_tunnel_term[idx].dip) 
            || (vxlan_info->vxlan_tunnel_term[idx].sip)){
            /* Reset buffer before read. */
            sal_memset(&info, 0, sizeof(bcm_tunnel_terminator_t));

            BCM_GPORT_TUNNEL_ID_SET(info.tunnel_id, idx);

            rv = bcm_td2_vxlan_tunnel_terminator_get(unit,&info);

            /* search failure */
            if (BCM_FAILURE(rv)) {
                if (rv != BCM_E_NOT_FOUND) {
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
    if (BCM_E_NOT_FOUND == rv) {
        rv = BCM_E_NONE;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_tunnel_init_add
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
_bcm_td2_vxlan_tunnel_init_add(int unit, int idx, bcm_tunnel_initiator_t *info)
{
    uint32 tnl_entry[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to write hw entry */
    soc_mem_t mem;                        /* Tunnel initiator table memory */  
    int rv = BCM_E_NONE;                  /* Return value                  */
    int df_val=0;                           /* Don't fragment encoding       */

    mem = EGR_IP_TUNNELm; 

    /* Zero write buffer. */
    sal_memset(tnl_entry, 0, sizeof(tnl_entry));

    /* If replacing existing entry, first read the entry to get old 
       profile pointer and TPID */
    if (info->flags & BCM_TUNNEL_REPLACE) {
        rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, idx, tnl_entry);
        BCM_IF_ERROR_RETURN(rv);
    }

    /* Set source address only within Tunnel Table */
    soc_mem_field_set(unit, mem, tnl_entry, SIPf, (uint32 *)&info->sip);

    /* IP tunnel hdr DF bit for IPv4. */
    df_val = 0;
    if (info->flags & BCM_TUNNEL_INIT_USE_INNER_DF) {
        df_val |= 0x2;
    } else if (info->flags & BCM_TUNNEL_INIT_IPV4_SET_DF) { 
        df_val |= 0x1;
    }
    soc_mem_field32_set(unit, mem, tnl_entry, IPV4_DF_SELf, df_val);

    /* IP tunnel hdr DF bit for IPv6. */
    if (info->flags & BCM_TUNNEL_INIT_IPV6_SET_DF) { 
        soc_mem_field32_set(unit, mem, tnl_entry, IPV6_DF_SELf, 1);
    }

    /* Set DSCP value.  */
    soc_mem_field32_set(unit, mem, tnl_entry, DSCPf, info->dscp);

    /* Tunnel header DSCP select. */
    soc_mem_field32_set(unit, mem, tnl_entry, DSCP_SELf, info->dscp_sel);

    /* Set TTL. */
    soc_mem_field32_set(unit, mem, tnl_entry, TTLf, info->ttl);

    /* Set UDP Dest Port */
    soc_mem_field32_set(unit, mem, tnl_entry, L4_DEST_PORTf, info->udp_dst_port);

    /* Set UDP Src Port */
    soc_mem_field32_set(unit, mem, tnl_entry, L4_SRC_PORTf, info->udp_src_port);

    /* Set Tunnel type = VXLAN */
    soc_mem_field32_set(unit, mem, tnl_entry, TUNNEL_TYPEf, _BCM_VXLAN_TUNNEL_TYPE);

    /* Set entry type = IPv4 */
    soc_mem_field32_set(unit, mem, tnl_entry, ENTRY_TYPEf, 0x1);

    /* Write buffer to hw. */
    rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, idx, tnl_entry);
    return rv;

}

/*  
 * Function:
 *      _bcm_td2_vxlan_tunnel_initiator_entry_add
 * Purpose:
 *      Configure VXLAN Tunnel initiator hardware Entry
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_vxlan_tunnel_initiator_entry_add(int unit, uint32 flags, bcm_tunnel_initiator_t *info, int *tnl_idx)
{
   int rv = BCM_E_NONE;

   rv = bcm_xgs3_tnl_init_add(unit, flags, info, tnl_idx);
   if (BCM_FAILURE(rv)) {
         return rv;
   }
   /* Write the entry to HW */
   rv = _bcm_td2_vxlan_tunnel_init_add(unit, *tnl_idx, info);
   if (BCM_FAILURE(rv)) {
        flags = _BCM_L3_SHR_WRITE_DISABLE;
        (void) bcm_xgs3_tnl_init_del(unit, flags, *tnl_idx);
   }
   return rv;
}

/*  
 * Function:
 *      bcm_td2_vxlan_tunnel_initiator_create
 * Purpose:
 *      Set VXLAN Tunnel initiator
 * Parameters:
 *      unit - Device Number
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_vxlan_tunnel_initiator_create(int unit, bcm_tunnel_initiator_t *info)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int tnl_idx=-1, rv = BCM_E_NONE,  ref_count=0;
    uint32 flags=0;
    int idx=0, num_tnl=0, num_vp=0;
    uint8  match_entry_present=0;
    uint32 tnl_entry[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry.  */
    bcm_ip_t sip=0;                       /* Tunnel header SIP (IPv4). */
    uint16 udp_dst_port;                /* Destination UDP port */

    /* Input parameters check. */
    if (NULL == info) {
        return (BCM_E_PARAM);
    }   
    if (info->type != bcmTunnelTypeVxlan)  {
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
    if ((0 == info->dip) || (0 == info->sip)) {
        return BCM_E_PARAM; 
    }
   
    vxlan_info = VXLAN_INFO(unit);
    num_tnl = soc_mem_index_count(unit, EGR_IP_TUNNELm);
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    if (info->flags & BCM_TUNNEL_REPLACE) {
        int tunnel_idx = -1;
        tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(info->tunnel_id);
        if ((tunnel_idx <= 0) || (tunnel_idx >= num_vp)) {
            return BCM_E_BADID; 
        }
        if(info->sip != vxlan_info->vxlan_tunnel_init[tunnel_idx].sip 
            || info->udp_dst_port != 
                vxlan_info->vxlan_tunnel_init[tunnel_idx].tunnel_state
            || info->dip != vxlan_info->vxlan_tunnel_init[tunnel_idx].dip) {
            return BCM_E_UNAVAIL;
        }    
    }

    /* Allocate either existing or new tunnel initiator entry */
    flags = _BCM_L3_SHR_MATCH_DISABLE | _BCM_L3_SHR_WRITE_DISABLE |
             _BCM_L3_SHR_SKIP_INDEX_ZERO;

    /* Check if Same SIP exists: MAP first 512 entries to EGR_IP_TUNNEL
        Rest of entries map to  VXLAN_TUNNEL with Same SIP, different DIP */
    for (idx=0; idx < num_tnl; idx++) {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, EGR_IP_TUNNELm, MEM_BLOCK_ANY, idx, tnl_entry));
         if (info->sip == soc_mem_field32_get(unit, EGR_IP_TUNNELm, tnl_entry, SIPf) &&
             info->udp_dst_port == 
             soc_mem_field32_get(unit, EGR_IP_TUNNELm, tnl_entry, L4_DEST_PORTf)) {
                tnl_idx = idx; /* Entry_idx within Hardware-Range */
                match_entry_present = 1;
                break;
        }
    }


    if (match_entry_present) {
         /* 2 Cases: Entry-update, (Multiple-dip, same-SIP) */
         if (vxlan_info->vxlan_tunnel_init[tnl_idx].dip != info->dip) {
             /* Obtain index to Software-State to store (SIP,DIP) */
             if (vxlan_info->vxlan_tunnel_init[tnl_idx].sip != 0) {
                 int free_index = -1;
                 int free_entry_found = 0;
                 int existed_entry_found = 0;
                 for (idx=num_tnl; idx < num_vp; idx++) {
                     if (vxlan_info->vxlan_tunnel_init[idx].sip == info->sip &&
                         vxlan_info->vxlan_tunnel_init[idx].tunnel_state ==
                         info->udp_dst_port &&
                         vxlan_info->vxlan_tunnel_init[idx].dip == info->dip) {
                         tnl_idx = idx; /* Obtain existed index */
                         existed_entry_found = 1;
                         break;
                     }
                     if (vxlan_info->vxlan_tunnel_init[idx].sip == 0 && !free_entry_found) {
                         free_index = idx; /* Obtain Free index */
                         free_entry_found = 1;
                     }
                 }
                 if (!existed_entry_found) {
                     if(free_entry_found) {
                        if (info->flags & BCM_TUNNEL_REPLACE) {
                            return BCM_E_UNAVAIL;
                        }
                        tnl_idx = free_index;
                     }
                     else {
                        /* No free resource */
                        return BCM_E_RESOURCE;
                     }
                 }
             }
         } else {
             /* Parse tunnel replace flag */
             if (info->flags & BCM_TUNNEL_REPLACE) {
                flags |=  _BCM_L3_SHR_UPDATE | _BCM_L3_SHR_WITH_ID;
                tnl_idx = BCM_GPORT_TUNNEL_ID_GET(info->tunnel_id);
                /* Check for Ref-count in (SIP, multi-DIP) situation */
                sip = vxlan_info->vxlan_tunnel_init[tnl_idx].sip;
                udp_dst_port = vxlan_info->vxlan_tunnel_init[tnl_idx].tunnel_state;
                for (idx=0; idx < num_vp; idx++) {
                    if ((vxlan_info->vxlan_tunnel_init[idx].sip == sip) &&
                         (vxlan_info->vxlan_tunnel_init[idx].tunnel_state == udp_dst_port)) {
                             ref_count++;
                    }
                }
                if (ref_count > 1) {
                   return BCM_E_RESOURCE;
                }

                rv = _bcm_td2_vxlan_tunnel_initiator_entry_add(unit, flags, info, &tnl_idx);
                if (BCM_FAILURE(rv)) {
                   return rv;
                }
             }
         }
    } else {
         /* Parse tunnel replace flag */
         if (info->flags & BCM_TUNNEL_REPLACE) {
             /* replace an inexistent tunnel initiator */
             return BCM_E_NOT_FOUND;
         }
         rv = _bcm_td2_vxlan_tunnel_initiator_entry_add(unit, flags, info, &tnl_idx);
         if (BCM_FAILURE(rv)) {
             return rv;
         }
    }

    if (tnl_idx !=- 1) {
         vxlan_info->vxlan_tunnel_init[tnl_idx].sip = info->sip;
         vxlan_info->vxlan_tunnel_init[tnl_idx].dip = info->dip;
         vxlan_info->vxlan_tunnel_init[tnl_idx].tunnel_state = info->udp_dst_port;
         BCM_GPORT_TUNNEL_ID_SET(info->tunnel_id, tnl_idx);
    } else {
        return BCM_E_NOT_FOUND;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_tunnel_init_get
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
_bcm_td2_vxlan_tunnel_init_get(int unit, int *hw_idx, bcm_tunnel_initiator_t *info)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    uint32 tnl_entry[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry.  */
    soc_mem_t mem;                         /* Tunnel initiator table memory.*/  
    int df_val=0;                            /* Don't fragment encoded value. */
    int tnl_type=0;                          /* Tunnel type.                  */
    uint32 entry_type = BCM_XGS3_TUNNEL_INIT_V4;/* Entry type (IPv4/IPv6/MPLS)*/
    int num_tnl=0, tnl_idx =0, idx=0;
    bcm_ip_t sip=0;                       /* Tunnel header SIP (IPv4). */
    uint16 udp_dst_port;                /* Destination UDP port */

    /* Get table memory. */
    mem = EGR_IP_TUNNELm; 
    idx = *hw_idx;

    /* Initialize the buffer. */
    sal_memset(tnl_entry, 0, sizeof(tnl_entry));
    vxlan_info = VXLAN_INFO(unit);
    num_tnl = soc_mem_index_count(unit, EGR_IP_TUNNELm);
    if (idx >= num_tnl) { /* Software State */
         sip = vxlan_info->vxlan_tunnel_init[idx].sip;
         udp_dst_port =  vxlan_info->vxlan_tunnel_init[idx].tunnel_state;
         info->dip = vxlan_info->vxlan_tunnel_init[idx].dip;

        /* Find matching entry within Hardware */
       for (tnl_idx=0; tnl_idx < num_tnl; tnl_idx++) {
            BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY, tnl_idx, tnl_entry));
            if (sip == soc_mem_field32_get(unit, mem, tnl_entry, SIPf) &&
                udp_dst_port == 
                     soc_mem_field32_get(unit, mem, tnl_entry, L4_DEST_PORTf)) {
                idx = tnl_idx; /* Entry_idx within Hardware-Range */
                break;
            }
       }
    } else {
         info->dip = vxlan_info->vxlan_tunnel_init[idx].dip;
         BCM_IF_ERROR_RETURN(soc_mem_read(unit, mem, MEM_BLOCK_ANY, idx, tnl_entry));
    }

    /* Get tunnel type. */
    tnl_type = soc_mem_field32_get(unit, mem, tnl_entry, TUNNEL_TYPEf);
    if (tnl_type != _BCM_VXLAN_TUNNEL_TYPE) {
        return BCM_E_NOT_FOUND;
    }

    /* Get entry_type. */
    entry_type = soc_mem_field32_get(unit, mem, tnl_entry, ENTRY_TYPEf); 
    if (entry_type != 0x1) {
        return BCM_E_NOT_FOUND;
    }

    /* Parse hw buffer. */
    /* Check will not fail. entry_type already validated */
    if (BCM_XGS3_TUNNEL_INIT_V4 == entry_type) {
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

    df_val = soc_mem_field32_get(unit, mem, tnl_entry, IPV6_DF_SELf);
    if (0x1 == df_val) {
        info->flags |= BCM_TUNNEL_INIT_IPV6_SET_DF;  
    }

    /* Get TTL. */
    info->ttl = soc_mem_field32_get(unit, mem, tnl_entry, TTLf);

    /* Get UDP Dest Port */
    info->udp_dst_port = soc_mem_field32_get(unit, mem, tnl_entry, L4_DEST_PORTf);

    /* Get UDP Src Port */
    info->udp_src_port = soc_mem_field32_get(unit, mem, tnl_entry, L4_SRC_PORTf);

    /* Translate hw tunnel type into API encoding. - bcmTunnelTypeVxlan */
    BCM_IF_ERROR_RETURN(_bcm_trx_tnl_hw_code_to_type(unit, tnl_type,
                                                     entry_type,
                                                     &info->type));
    *hw_idx = idx;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_td2_vxlan_tunnel_initiator_destroy
 * Purpose:
 *      Destroy outgoing VXLAN tunnel
 * Parameters:
 *      unit           - (IN) Device Number
 *      vxlan_tunnel_id - (IN) Tunnel ID (Gport)
 */
int
bcm_td2_vxlan_tunnel_initiator_destroy(int unit, bcm_gport_t vxlan_tunnel_id) 
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int tnl_idx=-1, temp_tnl_idx=-1, rv = BCM_E_NONE, ref_count=0, idx=0;
    uint32 flags = 0;
    bcm_tunnel_initiator_t info;           /* Tunnel initiator structure */
    int num_vp=0;
    bcm_ip_t sip=0;                       /* Tunnel header SIP (IPv4). */
    uint16 udp_dst_port;                /* Destination UDP port */

    vxlan_info = VXLAN_INFO(unit);
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    /* Input parameters check. */
    if (!BCM_GPORT_IS_TUNNEL(vxlan_tunnel_id)) {
        return BCM_E_PARAM;
    }
    tnl_idx = BCM_GPORT_TUNNEL_ID_GET(vxlan_tunnel_id);

    bcm_tunnel_initiator_t_init(&info);
    sip = vxlan_info->vxlan_tunnel_init[tnl_idx].sip;
    udp_dst_port =  vxlan_info->vxlan_tunnel_init[tnl_idx].tunnel_state;

    /* Remove HW TNL entry only if all matching SIP, UDP_DEST_PORT gets removed */
    for (idx=0; idx < num_vp; idx++) {
        if ((vxlan_info->vxlan_tunnel_init[idx].sip == sip) &&
             (vxlan_info->vxlan_tunnel_init[idx].tunnel_state == udp_dst_port)) {
                ref_count++;
        }
    }

    if (ref_count == 1) {
       /* Get the entry first */
       temp_tnl_idx = tnl_idx;
       rv = _bcm_td2_vxlan_tunnel_init_get(unit, &tnl_idx, &info);
       if (BCM_FAILURE(rv)) {
          return rv;
       }
       /* Destroy the tunnel entry */
       (void) bcm_xgs3_tnl_init_del(unit, flags, tnl_idx);
       vxlan_info->vxlan_tunnel_init[temp_tnl_idx].sip = 0;
       vxlan_info->vxlan_tunnel_init[temp_tnl_idx].dip = 0;
       vxlan_info->vxlan_tunnel_init[tnl_idx].tunnel_state = 0;
    } else if (ref_count) {
       vxlan_info->vxlan_tunnel_init[tnl_idx].sip = 0;
       vxlan_info->vxlan_tunnel_init[tnl_idx].dip = 0;
       vxlan_info->vxlan_tunnel_init[tnl_idx].tunnel_state = 0;
    }
    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_tunnel_initiator_destroy_all
 * Purpose:
 *      Destroy all outgoing VXLAN tunnel
 * Parameters:
 *      unit           - (IN) Device Number
 */
int
bcm_td2_vxlan_tunnel_initiator_destroy_all(int unit) 
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;

    uint32 tnl_entry[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry.  */
    int rv = BCM_E_NONE, tnl_idx, num_entries, num_vp;

    vxlan_info = VXLAN_INFO(unit);
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
                                        TUNNEL_TYPEf) != _BCM_VXLAN_TUNNEL_TYPE) {
            continue;
        }

        /* Delete the entry from HW */
       /* Destroy the tunnel entry */
       (void) bcm_xgs3_tnl_init_del(unit, 0, tnl_idx);
       vxlan_info->vxlan_tunnel_init[tnl_idx].sip = 0;
       vxlan_info->vxlan_tunnel_init[tnl_idx].dip = 0;
       vxlan_info->vxlan_tunnel_init[tnl_idx].tunnel_state = 0;
    }

    for (tnl_idx = num_entries; tnl_idx < num_vp; tnl_idx++) {
         vxlan_info->vxlan_tunnel_init[tnl_idx].sip = 0;
         vxlan_info->vxlan_tunnel_init[tnl_idx].dip = 0;
         vxlan_info->vxlan_tunnel_init[tnl_idx].tunnel_state = 0;
    }
    return BCM_E_NONE;

}

/*
 * Function:
 *      bcm_td2_vxlan_tunnel_initiator_get
 * Purpose:
 *      Get an outgong VXLAN tunnel info
 * Parameters:
 *      unit    - (IN) Device Number
 *      info    - (IN/OUT) Tunnel initiator structure
 */
int
bcm_td2_vxlan_tunnel_initiator_get(int unit, bcm_tunnel_initiator_t *info)
{
    int tnl_idx=-1, rv = BCM_E_NONE;

    /* Input parameters check. */
    if (info == NULL) {
        return BCM_E_PARAM;
    }
    if (!BCM_GPORT_IS_TUNNEL(info->tunnel_id)) {
        return BCM_E_PARAM;
    }
    tnl_idx = BCM_GPORT_TUNNEL_ID_GET(info->tunnel_id);

    /* Get the entry */
    rv = _bcm_td2_vxlan_tunnel_init_get(unit, &tnl_idx, info);
    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_tunnel_initiator_traverse
 * Purpose:
 *      Traverse VXLAN tunnel initiator entries
 * Parameters:
 *      unit    - (IN) Device Number
 *      cb    - (IN) User callback function, called once per entry
 *      user_data    - (IN) User supplied cookie used in parameter in callback function 
 */
int
bcm_td2_vxlan_tunnel_initiator_traverse(int unit, bcm_tunnel_initiator_traverse_cb cb, void *user_data)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int num_vp = 0, idx = 0;
    bcm_tunnel_initiator_t info;
    int rv = BCM_E_NONE;

    vxlan_info = VXLAN_INFO(unit);
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    /* Iterate over all the entries - search valid ones. */
    for(idx = 0; idx < num_vp; idx++) {
        /* Check a valid entry */
        if (vxlan_info->vxlan_tunnel_init[idx].sip != 0) {
            /* Reset buffer before read. */
            sal_memset(&info, 0, sizeof(bcm_tunnel_initiator_t));

            BCM_GPORT_TUNNEL_ID_SET(info.tunnel_id, idx);
            rv = bcm_td2_vxlan_tunnel_initiator_get(unit, &info);

            /* search failure */
            if (BCM_FAILURE(rv)) {
                if (rv != BCM_E_NOT_FOUND) {
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
    if (BCM_E_NOT_FOUND == rv) {
        rv = BCM_E_NONE;
    }

    return rv;
}

/*
 * Function:
 *     _bcm_td2_vxlan_ingress_dvp_set
 * Purpose:
 *     Set Ingress DVP tables
 * Parameters:
 *   IN :  Unit
 *   IN :  vp
 *   IN :  vxlan_port
 * Returns:
 *   BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_ingress_dvp_set(int unit, int vp, uint32 mpath_flag,
                                    int vp_nh_ecmp_index, bcm_vxlan_port_t *vxlan_port)
{
    ing_dvp_table_entry_t dvp;
    int rv = BCM_E_NONE;

         if (vxlan_port->flags & BCM_VXLAN_PORT_REPLACE) {
            BCM_IF_ERROR_RETURN(
                READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
         } else if (vxlan_port->flags & BCM_VXLAN_PORT_WITH_ID ) {
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
        if (vxlan_port->flags & BCM_VXLAN_PORT_EGRESS_TUNNEL) {
             soc_ING_DVP_TABLEm_field32_set(unit, &dvp, NETWORK_PORTf, 0x1);
             soc_ING_DVP_TABLEm_field32_set(unit, &dvp, 
                           VP_TYPEf, _BCM_VXLAN_INGRESS_DEST_VP_TYPE);
        } else {
             soc_ING_DVP_TABLEm_field32_set(unit, &dvp, NETWORK_PORTf, 0x0);
             soc_ING_DVP_TABLEm_field32_set(unit, &dvp, 
                           VP_TYPEf, _BCM_VXLAN_DEST_VP_TYPE_ACCESS);
        }

        rv =  WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp, &dvp);   
        return rv;
}


/*
 * Function:
 *     _bcm_td2_vxlan_ingress_dvp_reset
 * Purpose:
 *     Reset Ingress DVP tables
 * Parameters:
 *   IN :  Unit
 *   IN :  vp
 * Returns:
 *   BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_ingress_dvp_reset(int unit, int vp)
{
    int rv=BCM_E_UNAVAIL;
    ing_dvp_table_entry_t dvp;
  
    sal_memset(&dvp, 0, sizeof(ing_dvp_table_entry_t));
    rv =  WRITE_ING_DVP_TABLEm(unit, MEM_BLOCK_ALL, vp, &dvp);   
    return rv;
}

/*
 * Function:
 *     _bcm_td2_vxlan_egress_access_dvp_set
 * Purpose:
 *     Set Egress DVP tables - For access virtual ports
 * Parameters:
 *   IN :  Unit
 *   IN :  vp
 *   IN :  drop (unused for now)
 *   IN :  vxlan_port
 * Returns:
 *   BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_egress_access_dvp_set(int unit, int vp, int drop,
                                     bcm_vxlan_port_t *vxlan_port)
{
    int rv = BCM_E_UNAVAIL;
    soc_mem_t mem = EGR_DVP_ATTRIBUTEm;
    egr_dvp_attribute_entry_t  egr_dvp_attribute;

    /* Access ports only */
    if (vxlan_port->flags & BCM_VXLAN_PORT_NETWORK) {
        return BCM_E_PARAM;
    }

    /* Read entry and then update */
    sal_memset(&egr_dvp_attribute, 0, sizeof(egr_dvp_attribute_entry_t));
    rv = soc_mem_read(unit, mem, MEM_BLOCK_ALL, vp, &egr_dvp_attribute);

    soc_mem_field32_set(unit, mem, &egr_dvp_attribute, VP_TYPEf,
                        _BCM_VXLAN_DEST_VP_TYPE_ACCESS);

    /* L2 MTU - This is different from L3 MTU */
    if (vxlan_port->mtu > 0) {
        soc_mem_field32_set(unit, mem, &egr_dvp_attribute,
                            COMMON__MTU_VALUEf, vxlan_port->mtu);
        soc_mem_field32_set(unit, mem, &egr_dvp_attribute,
                            COMMON__MTU_ENABLEf, 0x1);
    } else {
        /* Disable mtu checking if mtu = 0 */
        soc_mem_field32_set(unit, mem, &egr_dvp_attribute,
                            COMMON__MTU_ENABLEf, 0x0);
    }


    rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, vp, &egr_dvp_attribute);
    BCM_IF_ERROR_RETURN(rv);

    return rv;

}

/*
 * Function:
 *     _bcm_td2_vxlan_egress_dvp_set
 * Purpose:
 *     Set Egress DVP tables
 * Parameters:
 *   IN :  Unit
 *   IN :  vp
 *   IN :  vxlan_port
 * Returns:
 *   BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_egress_dvp_set(int unit, int vp, int drop, bcm_vxlan_port_t *vxlan_port)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int rv=BCM_E_UNAVAIL;
    egr_dvp_attribute_entry_t  egr_dvp_attribute;
    egr_dvp_attribute_1_entry_t  egr_dvp_attribute_1;
    bcm_ip_t dip=0;                       /* DIP for tunnel header */
    bcm_ip_t sip=0;                       /* SIP for tunnel header */
    int tunnel_index=-1, idx=0, num_tnl=0;
    uint32 tnl_entry[SOC_MAX_MEM_FIELD_WORDS]; /* Buffer to read hw entry. */
    int local_drop = 0;

    vxlan_info = VXLAN_INFO(unit);
    num_tnl = soc_mem_index_count(unit, EGR_IP_TUNNELm);

    tunnel_index = BCM_GPORT_TUNNEL_ID_GET(vxlan_port->egress_tunnel_id);
    if (tunnel_index < num_tnl) {
        /* Obtain Tunnel DIP */
        dip = vxlan_info->vxlan_tunnel_init[tunnel_index].dip; 
    } else {
        /* Obtain Tunnel SIP */
        sip = vxlan_info->vxlan_tunnel_init[tunnel_index].sip; 
        /* Obtain Tunnel DIP */
        dip = vxlan_info->vxlan_tunnel_init[tunnel_index].dip; 
        /* Restrict tunnel_index to Hardware Range */
        for (idx=0; idx < num_tnl; idx++) {
            BCM_IF_ERROR_RETURN(soc_mem_read(unit, EGR_IP_TUNNELm, 
                                                                MEM_BLOCK_ANY, idx, tnl_entry));
            if (sip == soc_mem_field32_get(unit, EGR_IP_TUNNELm, tnl_entry, SIPf)) {
                tunnel_index = idx; /* Entry_idx for SIP within EGR_IP_TUNNEL */
                break;
            }
        }
    }
   
    /* Zero write buffer. */
    sal_memset(&egr_dvp_attribute, 0, sizeof(egr_dvp_attribute_entry_t));

    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            VP_TYPEf, _BCM_VXLAN_EGRESS_DEST_VP_TYPE);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            VXLAN__TUNNEL_INDEXf, tunnel_index);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            VXLAN__DIPf, dip);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            VXLAN__DVP_IS_NETWORK_PORTf, 0x1);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            VXLAN__DISABLE_VP_PRUNINGf, 0x0);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTEm, &egr_dvp_attribute,
                                            VXLAN__DELETE_VNTAGf, 0x1);

    rv = soc_mem_write(unit, EGR_DVP_ATTRIBUTEm,
                                            MEM_BLOCK_ALL, vp, &egr_dvp_attribute);
    BCM_IF_ERROR_RETURN(rv);

    if (vxlan_port->flags & BCM_VXLAN_PORT_REPLACE) {
        rv = soc_mem_read(unit, EGR_DVP_ATTRIBUTE_1m,
                                            MEM_BLOCK_ANY, vp, &egr_dvp_attribute_1);
        BCM_IF_ERROR_RETURN(rv);
    } else {
        /* Zero write buffer. */
        sal_memset(&egr_dvp_attribute_1, 0, sizeof(egr_dvp_attribute_1_entry_t));
    }

    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            VXLAN__CLASS_IDf, vxlan_port->if_class);
    /* L2 MTU - This is different from L3 MTU */
    if (vxlan_port->mtu > 0) {
        soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            VXLAN__MTU_VALUEf, vxlan_port->mtu);
        soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            VXLAN__MTU_ENABLEf, 0x1);
    } else {
        soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            VXLAN__MTU_VALUEf, 0x0);
        soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            VXLAN__MTU_ENABLEf, 0x0);
    }
    if (vxlan_port->flags & BCM_VXLAN_PORT_MULTICAST) {
        local_drop = drop ? 1 : 0;
    }
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            VXLAN__UUC_DROPf, local_drop);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            VXLAN__UMC_DROPf, local_drop);
    soc_mem_field32_set(unit, EGR_DVP_ATTRIBUTE_1m, &egr_dvp_attribute_1,
                                            VXLAN__BC_DROPf, local_drop);
    rv = soc_mem_write(unit, EGR_DVP_ATTRIBUTE_1m,
                                            MEM_BLOCK_ALL, vp, &egr_dvp_attribute_1);

    return rv;

}


/*
 * Function:
 *     _bcm_td2_vxlan_egress_dvp_get
 * Purpose:
 *     Get Egress DVP tables
 * Parameters:
 *   IN :  Unit
 *   IN :  vp
 *   IN :  vxlan_port
 * Returns:
 *   BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_egress_dvp_get(int unit, int vp, bcm_vxlan_port_t *vxlan_port)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info;
    int rv=BCM_E_NONE;
    egr_dvp_attribute_entry_t  egr_dvp_attribute;
    egr_dvp_attribute_1_entry_t  egr_dvp_attribute_1;
    int tunnel_index=-1;
    bcm_ip_t dip=0;                       /* DIP for tunnel header */
    int idx=0, num_vp=0;
    int vp_type = 0; /* access / tunnel vp type*/

    vxlan_info = VXLAN_INFO(unit);
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    sal_memset(&egr_dvp_attribute, 0, sizeof(egr_dvp_attribute_entry_t));
    rv = soc_mem_read(unit, EGR_DVP_ATTRIBUTEm,
                                            MEM_BLOCK_ANY, vp, &egr_dvp_attribute);
    BCM_IF_ERROR_RETURN(rv);

    vp_type = soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTEm,
                        &egr_dvp_attribute, VP_TYPEf);

    /* Only mtu field is valid for access DVPs */
    if (vp_type == 0) {
        if (soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTEm,
                                &egr_dvp_attribute, MTU_ENABLEf)) {
            vxlan_port->mtu = soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTEm,
                                  &egr_dvp_attribute, MTU_VALUEf);
        }
        return rv;
    }

    /* Extract Network port details */
    dip = soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTEm, 
                                            &egr_dvp_attribute, VXLAN__DIPf);

    /* Find DIP within Softare State */
    for (idx=0; idx < num_vp; idx++) {
        if (vxlan_info->vxlan_tunnel_init[idx].dip == dip) {
            tunnel_index = idx;
            break;
        }
    }

    BCM_GPORT_TUNNEL_ID_SET(vxlan_port->egress_tunnel_id, tunnel_index);

    sal_memset(&egr_dvp_attribute_1, 0, sizeof(egr_dvp_attribute_1_entry_t));
    rv = soc_mem_read(unit, EGR_DVP_ATTRIBUTE_1m,
                                            MEM_BLOCK_ANY, vp, &egr_dvp_attribute_1);
    BCM_IF_ERROR_RETURN(rv);

    vxlan_port->if_class = soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTE_1m, 
                                            &egr_dvp_attribute_1, VXLAN__CLASS_IDf);
    if (soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTE_1m, 
                                            &egr_dvp_attribute_1, VXLAN__MTU_ENABLEf)) {
         vxlan_port->mtu = soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTE_1m, 
                                            &egr_dvp_attribute_1, VXLAN__MTU_VALUEf);
    }
    if (soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTE_1m, 
                            &egr_dvp_attribute_1, VXLAN__BC_DROPf)
        || soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTE_1m, 
                               &egr_dvp_attribute_1, VXLAN__UUC_DROPf)
        || soc_mem_field32_get(unit, EGR_DVP_ATTRIBUTE_1m, 
                               &egr_dvp_attribute_1, VXLAN__UMC_DROPf)) {
         vxlan_port->flags |= BCM_VXLAN_PORT_DROP;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_egress_dvp_reset
 * Purpose:
 *      Reet Egress DVP tables
 * Parameters:
 *      IN :  Unit
 *           IN :  vp
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_egress_dvp_reset(int unit, int vp)
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
 *      _bcm_td2_vxlan_egr_xlate_entry_reset
 * Purpose:
 *      Reset VXLAN  egr_xlate Entry 
 * Parameters:
 *      IN :  Unit
 *      IN :  vpn id
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_egr_xlate_entry_reset(int unit, bcm_vpn_t vpn)
{
    int index = 0;
    int vfi = 0;
    int tpid_index = -1;
    egr_vlan_xlate_entry_t  key_ent;
    int action_present = 0, action_not_present = 0;

    sal_memset(&key_ent, 0, sizeof(egr_vlan_xlate_entry_t));
    _BCM_VXLAN_VPN_GET(vfi, _BCM_VPN_TYPE_VFI, vpn);
    soc_EGR_VLAN_XLATEm_field32_set(
        unit, &key_ent, ENTRY_TYPEf, _BCM_VXLAN_KEY_TYPE_LOOKUP_VFI);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &key_ent, VALIDf, 0x1);
    soc_EGR_VLAN_XLATEm_field32_set(unit, &key_ent, VXLAN_VFI__VFIf, vfi);

    BCM_IF_ERROR_RETURN(soc_mem_search(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ANY,
                                       &index, &key_ent, &key_ent, 0));

    /* If tpid entry exist, delete it */
    action_present = soc_EGR_VLAN_XLATEm_field32_get(unit,
                &key_ent, VXLAN_VFI__SD_TAG_ACTION_IF_PRESENTf);
    action_not_present = soc_EGR_VLAN_XLATEm_field32_get(unit,
            &key_ent, VXLAN_VFI__SD_TAG_ACTION_IF_NOT_PRESENTf);
    if (1 == action_present || 4 == action_present ||
        7 == action_present || 1 == action_not_present) {
        tpid_index = soc_EGR_VLAN_XLATEm_field32_get(
            unit, &key_ent, VXLAN_VFI__SD_TAG_TPID_INDEXf);
        BCM_IF_ERROR_RETURN(_bcm_fb2_outer_tpid_entry_delete(unit, tpid_index));
    }

    /* Delete the entry from HW */
    BCM_IF_ERROR_RETURN(
        soc_mem_delete(unit, EGR_VLAN_XLATEm, MEM_BLOCK_ALL, &key_ent));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_vxlan_port_untagged_profile_set
 * Purpose:
 *      Set  VLan profile per Physical port entry
 * Parameters:
 *      unit   - (IN) SOC unit #
 *      port  - (IN) Physical port
 * Returns:
 *      BCM_E_XXX
 */     

STATIC int
_bcm_td2_vxlan_port_untagged_profile_set (int unit, bcm_port_t port)
{
    port_tab_entry_t ptab;
    int rv=BCM_E_NONE;
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
 *      _bcm_td2_vxlan_port_untagged_profile_reset
 * Purpose:
 *      Reset  VLan profile per Physical port entry
 * Parameters:
 *      unit   - (IN) SOC unit #
 *      port  - (IN) Physical port
 * Returns:
 *      BCM_E_XXX
 */     

STATIC int
_bcm_td2_vxlan_port_untagged_profile_reset(int unit, bcm_port_t port)
{
    int rv=BCM_E_NONE;
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
 *      _bcm_td2_vxlan_sd_tag_set
 * Purpose:
 *      Program  SD_TAG settings
 * Parameters:
 *      unit           - (IN)  SOC unit #
 *      vxlan_port  - (IN)  VXLAN VP
 *      egr_nh_info  (IN/OUT) Egress NHop Info
 * Returns:
 *      BCM_E_XXX
 */     

STATIC int
_bcm_td2_vxlan_sd_tag_set( int unit, bcm_vxlan_vpn_config_t *vxlan_vpn_info, 
                         bcm_vxlan_port_t *vxlan_port, 
                         _bcm_td2_vxlan_nh_info_t  *egr_nh_info, 
                         egr_vlan_xlate_entry_t   *vxlate_entry,
                         int *tpid_index )
{
    soc_mem_t  hw_mem;

    if (vxlan_vpn_info != NULL ) {
        hw_mem = EGR_VLAN_XLATEm;
        if (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_ADD) {
            if (vxlan_vpn_info->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
            }
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_VIDf,
                                vxlan_vpn_info->egress_service_vlan);
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_ACTION_IF_NOT_PRESENTf,
                                0x1); /* ADD */
        }

        if (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_TPID_REPLACE) {
            if (vxlan_vpn_info->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
            }
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_VIDf,
                                vxlan_vpn_info->egress_service_vlan);
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x1); /* REPLACE_VID_TPID */
        } else if (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_REPLACE) {
            if (vxlan_vpn_info->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
            }
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_VIDf,
                                vxlan_vpn_info->egress_service_vlan);
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x2); /* REPLACE_VID_ONLY */
        } else if (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_DELETE) {
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x3); /* DELETE */
        } else if (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_PRI_TPID_REPLACE) {
               if (vxlan_vpn_info->egress_service_vlan >= BCM_VLAN_INVALID) {
                   return  BCM_E_PARAM;
               }
               if (vxlan_vpn_info->pkt_pri > BCM_PRIO_MAX) {
                   return  BCM_E_PARAM;
               }
               if (vxlan_vpn_info->pkt_cfi > 1) {
                   return  BCM_E_PARAM;
               }
               soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_VIDf,
                                vxlan_vpn_info->egress_service_vlan);
               if (soc_mem_field_valid(unit, hw_mem,
                                       VXLAN_VFI__SD_TAG_NEW_PRIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_NEW_PRIf,
                                vxlan_vpn_info->pkt_pri);
               }
               if (soc_mem_field_valid(unit, hw_mem,
                                       VXLAN_VFI__SD_TAG_NEW_CFIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_NEW_CFIf,
                                vxlan_vpn_info->pkt_cfi);
               }

               soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x4); /* REPLACE_VID_PRI_TPID */

        } else if (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_PRI_REPLACE ){
                if (vxlan_vpn_info->egress_service_vlan >= BCM_VLAN_INVALID) {
                    return  BCM_E_PARAM;
                }
                if (vxlan_vpn_info->pkt_pri > BCM_PRIO_MAX) {
                    return  BCM_E_PARAM;
                }
                if (vxlan_vpn_info->pkt_cfi > 1) {
                    return  BCM_E_PARAM;
                }
                soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_VIDf,
                                vxlan_vpn_info->egress_service_vlan);

                if (soc_mem_field_valid(unit, hw_mem,
                                       VXLAN_VFI__SD_TAG_NEW_PRIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_NEW_PRIf,
                                vxlan_vpn_info->pkt_pri);
                }
                if (soc_mem_field_valid(unit, hw_mem,
                                       VXLAN_VFI__SD_TAG_NEW_CFIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_NEW_CFIf,
                                vxlan_vpn_info->pkt_cfi);
                }

                soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x5); /* REPLACE_VID_PRI */
        }else if (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_PRI_REPLACE ) {
                if (vxlan_vpn_info->pkt_pri > BCM_PRIO_MAX) {
                    return  BCM_E_PARAM;
                }
                if (vxlan_vpn_info->pkt_cfi > 1) {
                    return  BCM_E_PARAM;
                }
                if (soc_mem_field_valid(unit, hw_mem,
                                       VXLAN_VFI__SD_TAG_NEW_PRIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_NEW_PRIf,
                                vxlan_vpn_info->pkt_pri);
                }
                if (soc_mem_field_valid(unit, hw_mem,
                                       VXLAN_VFI__SD_TAG_NEW_CFIf)) {
                   soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_NEW_CFIf,
                                vxlan_vpn_info->pkt_cfi);
                }

                soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x6); /* REPLACE_PRI_ONLY */
      
        } else if (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_TPID_REPLACE ) {
                 soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_ACTION_IF_PRESENTf,
                                0x7); /* REPLACE_TPID_ONLY */
        }

        if ((vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_ADD) ||
            (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_TPID_REPLACE) ||
            (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_TPID_REPLACE) ||
            (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_PRI_TPID_REPLACE)) {
            /* TPID value is used */
            BCM_IF_ERROR_RETURN(
                _bcm_fb2_outer_tpid_entry_add(unit, vxlan_vpn_info->egress_service_tpid, tpid_index));
            soc_mem_field32_set(unit, hw_mem,
                                vxlate_entry, VXLAN_VFI__SD_TAG_TPID_INDEXf, *tpid_index);
        }
    } else  {

        if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_ADD) {
            if (vxlan_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
            }
            egr_nh_info->sd_tag_vlan = vxlan_port->egress_service_vlan; 
            egr_nh_info->sd_tag_action_not_present = 0x1; /* ADD */
        }

        if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_TPID_REPLACE) {
            if (vxlan_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
            }
            /* REPLACE_VID_TPID */
            egr_nh_info->sd_tag_vlan = vxlan_port->egress_service_vlan; 
            egr_nh_info->sd_tag_action_present = 0x1;
        } else if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_REPLACE) {

            if (vxlan_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                return  BCM_E_PARAM;
            }
            /* REPLACE_VID_ONLY */
            egr_nh_info->sd_tag_vlan = vxlan_port->egress_service_vlan; 
            egr_nh_info->sd_tag_action_present = 0x2;
        } else if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_DELETE) {
            egr_nh_info->sd_tag_action_present = 0x3; /* DELETE */
            egr_nh_info->sd_tag_action_not_present = 0;
        } else if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_PRI_TPID_REPLACE) {
                if (vxlan_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                    return  BCM_E_PARAM;
                }
                if (vxlan_port->pkt_pri > BCM_PRIO_MAX) {
                    return  BCM_E_PARAM;
                }
                if (vxlan_port->pkt_cfi > 1) {
                    return  BCM_E_PARAM;
                }

                /* REPLACE_VID_PRI_TPID */
                egr_nh_info->sd_tag_vlan = vxlan_port->egress_service_vlan;
                egr_nh_info->sd_tag_pri = vxlan_port->pkt_pri;
                egr_nh_info->sd_tag_cfi = vxlan_port->pkt_cfi;
                egr_nh_info->sd_tag_action_present = 0x4;
        } else if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_PRI_REPLACE ) {
                if (vxlan_port->egress_service_vlan >= BCM_VLAN_INVALID) {
                    return  BCM_E_PARAM;
                }
                if (vxlan_port->pkt_pri > BCM_PRIO_MAX) {
                    return  BCM_E_PARAM;
                }
                if (vxlan_port->pkt_cfi > 1) {
                    return  BCM_E_PARAM;
                }

                /* REPLACE_VID_PRI_ONLY */
                egr_nh_info->sd_tag_vlan = vxlan_port->egress_service_vlan;
                egr_nh_info->sd_tag_pri = vxlan_port->pkt_pri;
                egr_nh_info->sd_tag_cfi = vxlan_port->pkt_cfi;
                egr_nh_info->sd_tag_action_present = 0x5;
        } else if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_PRI_REPLACE ) {
                if (vxlan_port->pkt_pri > BCM_PRIO_MAX) {
                    return  BCM_E_PARAM;
                }
                if (vxlan_port->pkt_cfi > 1) {
                    return  BCM_E_PARAM;
                }

                 /* REPLACE_PRI_ONLY */
                egr_nh_info->sd_tag_pri = vxlan_port->pkt_pri;
                egr_nh_info->sd_tag_cfi = vxlan_port->pkt_cfi;
                egr_nh_info->sd_tag_action_present = 0x6;
        } else if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_TPID_REPLACE ) {
                 /* REPLACE_TPID_ONLY */
                egr_nh_info->sd_tag_action_present = 0x7;
        }

        if ((vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_ADD) ||
            (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_TPID_REPLACE) ||
            (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_TPID_REPLACE) ||
            (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_PRI_TPID_REPLACE)) {
            /* TPID value is used */
            BCM_IF_ERROR_RETURN(_bcm_fb2_outer_tpid_entry_add(unit, vxlan_port->egress_service_tpid, tpid_index));
            egr_nh_info->tpid_index = *tpid_index;
        }
    } 
    return  BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_vxlan_sd_tag_get
 * Purpose:
 *      Get  SD_TAG settings
 * Parameters:
 *      unit           - (IN)  SOC unit #
 *      vxlan_port  - (IN)  VXLAN VP
 *      egr_nh_info  (IN/OUT) Egress NHop Info
 * Returns:
 *      BCM_E_XXX
 */     

STATIC void
_bcm_td2_vxlan_sd_tag_get( int unit, bcm_vxlan_vpn_config_t *vxlan_vpn_info, 
                         bcm_vxlan_port_t *vxlan_port, 
                         egr_l3_next_hop_entry_t *egr_nh, 
                         egr_vlan_xlate_entry_t   *vxlate_entry,
                         int network_port_flag )
{
    int action_present=0, action_not_present=0, tpid_index = 0;

    if (network_port_flag) {
       action_present = 
            soc_EGR_VLAN_XLATEm_field32_get(unit, 
                                     vxlate_entry,
                                     VXLAN_VFI__SD_TAG_ACTION_IF_PRESENTf);
       action_not_present = 
            soc_EGR_VLAN_XLATEm_field32_get(unit, 
                                     vxlate_entry,
                                     VXLAN_VFI__SD_TAG_ACTION_IF_NOT_PRESENTf);

       if (action_not_present == 1) {
            vxlan_vpn_info->flags |= BCM_VXLAN_VPN_SERVICE_TAGGED;
            vxlan_vpn_info->flags |= BCM_VXLAN_VPN_SERVICE_VLAN_ADD;
            vxlan_vpn_info->egress_service_vlan =
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_VIDf);
       }

       if (action_present) {
            vxlan_vpn_info->flags |= BCM_VXLAN_VPN_SERVICE_TAGGED;
       }
       switch (action_present) {
         case 1:
            vxlan_vpn_info->flags |= BCM_VXLAN_VPN_SERVICE_VLAN_TPID_REPLACE;
            vxlan_vpn_info->egress_service_vlan =
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_VIDf);
            break;

         case 2:
            vxlan_vpn_info->flags |= BCM_VXLAN_VPN_SERVICE_VLAN_REPLACE;
            vxlan_vpn_info->egress_service_vlan =
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_VIDf);
            break;

         case 3:
            vxlan_vpn_info->flags |= BCM_VXLAN_VPN_SERVICE_VLAN_DELETE;
            vxlan_vpn_info->egress_service_vlan = BCM_VLAN_INVALID;
            break;

         case 4:
            vxlan_vpn_info->flags |= BCM_VXLAN_VPN_SERVICE_VLAN_PRI_TPID_REPLACE;
            vxlan_vpn_info->egress_service_vlan =                 
                 soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_VIDf);
            vxlan_vpn_info->pkt_pri = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_NEW_PRIf);
            vxlan_vpn_info->pkt_cfi = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_NEW_CFIf);
            break;

         case 5:
            vxlan_vpn_info->flags |= BCM_VXLAN_VPN_SERVICE_VLAN_PRI_REPLACE;
            vxlan_vpn_info->egress_service_vlan = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_VIDf);
            vxlan_vpn_info->pkt_pri = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_NEW_PRIf);
            vxlan_vpn_info->pkt_cfi = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_NEW_CFIf);
            break;

         case 6:
            vxlan_vpn_info->flags |= BCM_VXLAN_VPN_SERVICE_PRI_REPLACE;
            vxlan_vpn_info->egress_service_vlan =   BCM_VLAN_INVALID;
            vxlan_vpn_info->pkt_pri = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_NEW_PRIf);
            vxlan_vpn_info->pkt_cfi = 
                soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_NEW_CFIf);
            break;

         case 7:
            vxlan_vpn_info->flags |= BCM_VXLAN_VPN_SERVICE_TPID_REPLACE;
            vxlan_vpn_info->egress_service_vlan = BCM_VLAN_INVALID;
            break;

         default:
            break;
       }

       if ((vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_ADD) ||
            (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_TPID_REPLACE) ||
            (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_TPID_REPLACE) ||
            (vxlan_vpn_info->flags & BCM_VXLAN_VPN_SERVICE_VLAN_PRI_TPID_REPLACE)) {
            tpid_index = soc_EGR_VLAN_XLATEm_field32_get(unit, vxlate_entry, VXLAN_VFI__SD_TAG_TPID_INDEXf);
            _bcm_fb2_outer_tpid_entry_get(unit, &vxlan_vpn_info->egress_service_tpid, tpid_index);
       }
    } else {
       action_present = 
            soc_EGR_L3_NEXT_HOPm_field32_get(unit, 
                                     egr_nh,
                                     SD_TAG__SD_TAG_ACTION_IF_PRESENTf);
       action_not_present = 
            soc_EGR_L3_NEXT_HOPm_field32_get(unit, 
                                     egr_nh,
                                     SD_TAG__SD_TAG_ACTION_IF_NOT_PRESENTf);

       if (action_not_present == 1) {
            vxlan_port->flags |= BCM_VXLAN_PORT_SERVICE_TAGGED;
            vxlan_port->flags |= BCM_VXLAN_PORT_SERVICE_VLAN_ADD;
            vxlan_port->egress_service_vlan =
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__SD_TAG_VIDf);
       }

       if (action_present) {
            vxlan_port->flags |= BCM_VXLAN_PORT_SERVICE_TAGGED;
       }
       switch (action_present) {
         case 1:
            vxlan_port->flags |= BCM_VXLAN_PORT_SERVICE_VLAN_TPID_REPLACE;
            vxlan_port->egress_service_vlan =
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__SD_TAG_VIDf);
            break;

         case 2:
            vxlan_port->flags |= BCM_VXLAN_PORT_SERVICE_VLAN_REPLACE;
            vxlan_port->egress_service_vlan =
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__SD_TAG_VIDf);
            break;

         case 3:
            vxlan_port->flags |= BCM_VXLAN_PORT_SERVICE_VLAN_DELETE;
            vxlan_port->egress_service_vlan = BCM_VLAN_INVALID;
            break;

         case 4:
            vxlan_port->flags |= BCM_VXLAN_PORT_SERVICE_VLAN_PRI_TPID_REPLACE;
            vxlan_port->egress_service_vlan =                 
                 soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__SD_TAG_VIDf);
            vxlan_port->pkt_pri = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_PRIf);
            vxlan_port->pkt_cfi = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_CFIf);
            break;

         case 5:
            vxlan_port->flags |= BCM_VXLAN_PORT_SERVICE_VLAN_PRI_REPLACE;
            vxlan_port->egress_service_vlan = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__SD_TAG_VIDf);
            vxlan_port->pkt_pri = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_PRIf);
            vxlan_port->pkt_cfi = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_CFIf);
            break;

         case 6:
            vxlan_port->flags |= BCM_VXLAN_PORT_SERVICE_PRI_REPLACE;
            vxlan_port->egress_service_vlan =   BCM_VLAN_INVALID;
            vxlan_port->pkt_pri = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_PRIf);
            vxlan_port->pkt_cfi = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__NEW_CFIf);
            break;

         case 7:
            vxlan_port->flags |= BCM_VXLAN_PORT_SERVICE_TPID_REPLACE;
            vxlan_port->egress_service_vlan = BCM_VLAN_INVALID;
            break;

         default:
            break;
       }

       if ((vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_ADD) ||
           (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_TPID_REPLACE) ||
           (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_TPID_REPLACE) ||
           (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_VLAN_PRI_TPID_REPLACE)) {
           /* TPID value is used */
           tpid_index = soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh, SD_TAG__SD_TAG_TPID_INDEXf);
           _bcm_fb2_outer_tpid_entry_get(unit, &vxlan_port->egress_service_tpid, tpid_index);
       }
    }
}

/*
 * Function:
 *      _bcm_td2_vxlan_nexthop_entry_modify
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
_bcm_td2_vxlan_nexthop_entry_modify(int unit, int nh_index, int drop, 
              _bcm_td2_vxlan_nh_info_t  *egr_nh_info, int new_entry_type)
{
    egr_l3_next_hop_entry_t egr_nh;
    int rv = BCM_E_NONE;
    uint32 old_entry_type=0, intf_num=0, vntag_actions=0;
    uint32 vntag_dst_vif=0, vntag_pf=0, vntag_force_lf=0;
    uint32 etag_pcp_de_sourcef=0, etag_pcpf=0, etag_dot1p_mapping_ptr=0, etag_def=0;
    bcm_mac_t mac_addr;                 /* Next hop forwarding destination mac. */

    sal_memset(&mac_addr, 0, sizeof (mac_addr));
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ALL,
                      nh_index, &egr_nh));

    old_entry_type = soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh, ENTRY_TYPEf);

    if ((new_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_SDTAG_VIEW) &&
        (old_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_L3_VIEW)) {
            /*Zero buffers.*/
            sal_memset(&egr_nh, 0, sizeof(egr_l3_next_hop_entry_t));
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
                            ENTRY_TYPEf, _BCM_VXLAN_EGR_NEXT_HOP_SDTAG_VIEW);
    } else if ((new_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_L3_VIEW) &&
        (old_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_SDTAG_VIEW)) {

            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                            ENTRY_TYPEf, _BCM_VXLAN_EGR_NEXT_HOP_L3_VIEW);

    } else if ((new_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_L3MC_VIEW) &&
         (old_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_L3_VIEW)) {

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
                         L3__VNTAG_Pf);

        vntag_force_lf = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3__VNTAG_FORCE_Lf);

        etag_pcp_de_sourcef = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3__ETAG_PCP_DE_SOURCEf);

        etag_pcpf = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3__ETAG_PCPf);

        etag_dot1p_mapping_ptr = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3__ETAG_DOT1P_MAPPING_PTRf);

        etag_def = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                         L3__ETAG_DEf);
        /*Zero buffers.*/
        sal_memset(&egr_nh, 0, sizeof(egr_l3_next_hop_entry_t));

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
                                L3MC__ETAG_DEf, etag_def);
        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                ENTRY_TYPEf, _BCM_VXLAN_EGR_NEXT_HOP_L3MC_VIEW);

    } else if ((new_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_L3MC_VIEW) &&
        (old_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_L3MC_VIEW)) {
        
        /* Update entry with DVP info if it is already IPMC entry */
        
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
        
    } else if ((new_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_L3_VIEW) &&
         (old_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_L3MC_VIEW)) {
            soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                            ENTRY_TYPEf, _BCM_VXLAN_EGR_NEXT_HOP_L3_VIEW);
    } else if ((new_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_SDTAG_VIEW) &&
        (old_entry_type == _BCM_VXLAN_EGR_NEXT_HOP_SDTAG_VIEW)) {
        /* Case of VXLAN Proxy Next-Hop */
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
 *           bcm_td2_vxlan_next_hop_set
 * Purpose:
 *           Set VXLAN NextHop Entry
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
_bcm_td2_vxlan_next_hop_set(int unit, int nh_index, uint32 flags)
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

    if (!(flags & BCM_L3_IPMC)) { /* Only for VXLAN Unicast Nexthops */
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
 *           bcm_td2_vxlan_nexthop_get
 * Purpose:
 *          Get VXLAN NextHop entry
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
bcm_td2_vxlan_nexthop_get(int unit, int nh_index, bcm_vxlan_port_t *vxlan_port)
{
    soc_mem_t      hw_mem;
    egr_l3_next_hop_entry_t egr_nh;
    uint32 entry_type;

    hw_mem = EGR_L3_NEXT_HOPm;
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, hw_mem, 
                                  MEM_BLOCK_ANY, nh_index, &egr_nh));
    if (0x1 == soc_mem_field32_get(unit, hw_mem, &egr_nh,
                                  L3MC__DVP_VALIDf)) {
         vxlan_port->flags |= BCM_VXLAN_PORT_MULTICAST;
    }

    entry_type = soc_mem_field32_get(unit, hw_mem, &egr_nh, ENTRY_TYPEf);

    if (entry_type == _BCM_VXLAN_EGR_NEXT_HOP_SDTAG_VIEW) {
        if (soc_mem_field32_get(unit, hw_mem, &egr_nh, SD_TAG__BC_DROPf)
            || soc_mem_field32_get(unit, hw_mem, &egr_nh, SD_TAG__UUC_DROPf)
            || soc_mem_field32_get(unit, hw_mem, &egr_nh, SD_TAG__UMC_DROPf)) {
            vxlan_port->flags |= BCM_VXLAN_PORT_DROP;
        }
    } else if (entry_type == _BCM_VXLAN_EGR_NEXT_HOP_L3MC_VIEW) {
        if (soc_mem_field32_get(unit, hw_mem, &egr_nh, L3MC__L3_DROPf)) {
            vxlan_port->flags |= BCM_VXLAN_PORT_DROP;
        }
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *           bcm_td2_vxlan_port_egress_nexthop_reset
 * Purpose:
 *           Reset VXLAN Egress NextHop 
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
bcm_td2_vxlan_port_egress_nexthop_reset(int unit, int nh_index, int vp_type, int vp, bcm_vpn_t vpn)
{
    egr_l3_next_hop_entry_t egr_nh;
    int rv=BCM_E_NONE;
    int action_present=0, action_not_present=0, old_tpid_idx = -1;
    uint32  entry_type=0;
    bcm_vxlan_port_t  vxlan_port;

    BCM_IF_ERROR_RETURN(
        _bcm_td2_vxlan_port_get(unit, vpn, vp, &vxlan_port));

    if (vp_type== _BCM_VXLAN_DEST_VP_TYPE_ACCESS) {
        BCM_IF_ERROR_RETURN(soc_mem_read(unit, EGR_L3_NEXT_HOPm,
                                MEM_BLOCK_ANY, nh_index, &egr_nh));
        /* egressing into a regular port */
        entry_type = 
            soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh, ENTRY_TYPEf);
        if (entry_type != _BCM_VXLAN_EGR_NEXT_HOP_SDTAG_VIEW) { 
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
        rv = _bcm_td2_vxlan_nexthop_entry_modify(unit, nh_index, 0, 
                             NULL, _BCM_VXLAN_EGR_NEXT_HOP_L3_VIEW);

    }

    if (old_tpid_idx != -1) {
        (void) _bcm_fb2_outer_tpid_entry_delete(unit, old_tpid_idx);
    }
    return rv;
}


/*
 * Function:
 *      bcm_td2_vxlan_nexthop_reset
 * Purpose:
 *      Reset VXLAN NextHop entry
 * Parameters:
 *      IN :  Unit
 *           IN :  nh_index
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_nexthop_reset(int unit, int nh_index)
{
    int rv=BCM_E_NONE;
    ing_l3_next_hop_entry_t ing_nh;
    egr_l3_next_hop_entry_t egr_nh;
    bcm_port_t      port=0;
    bcm_trunk_t tgid=0;
    int  num_local_ports=0, idx=-1;
    bcm_port_t   local_ports[SOC_MAX_NUM_PORTS];
    soc_mem_t      hw_mem;
    bcm_module_t modid=0, local_modid=0;
    uint8 multicast_entry=0;
    int  old_nh_index=-1;
    uint32 reg_val=0;

    hw_mem = EGR_L3_NEXT_HOPm;
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, hw_mem, 
                                  MEM_BLOCK_ANY, nh_index, &egr_nh));
    if (0x1 == soc_mem_field32_get(unit, hw_mem, &egr_nh,
                                  L3MC__DVP_VALIDf)) {
         multicast_entry = 1; /* Multicast NextHop */
    }

    hw_mem = ING_L3_NEXT_HOPm;
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, hw_mem, 
                                  MEM_BLOCK_ANY, nh_index, &ing_nh));
    soc_mem_field32_set(unit, hw_mem, &ing_nh,
                                            DROPf, 0x0);
    soc_mem_field32_set(unit, hw_mem, &ing_nh,
                                            ENTRY_TYPEf, 0x0);
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
            /* Ignore EGR_PORT_TO_NHI_MAPPINGs */
            return BCM_E_NONE;
        }
        port = soc_mem_field32_get(unit, hw_mem,
                                            &ing_nh, PORT_NUMf);
        local_ports[num_local_ports++] = port;
    }

    if (!multicast_entry) { /* Only for VXLAN Unicast Nexthops */
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
    }

    return rv;
}

/*
 * Function:
 *           bcm_td2_vxlan_egress_set
 * Purpose:
 *           Set VXLAN Egress NextHop 
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
bcm_td2_vxlan_egress_set(int unit, int nh_index, uint32 flags)
{
    ing_l3_next_hop_entry_t ing_nh;
    soc_mem_t      hw_mem;
    int rv=BCM_E_NONE;

    hw_mem = ING_L3_NEXT_HOPm;
    BCM_IF_ERROR_RETURN(soc_mem_read(unit, hw_mem, 
                MEM_BLOCK_ANY, nh_index, &ing_nh));
    soc_mem_field32_set(unit, hw_mem, &ing_nh,
                ENTRY_TYPEf, _BCM_VXLAN_INGRESS_NEXT_HOP_ENTRY_TYPE);
    if (SOC_MEM_FIELD_VALID(unit, ING_L3_NEXT_HOPm, MTU_SIZEf)) {
        soc_mem_field32_set(unit, ING_L3_NEXT_HOPm,
                &ing_nh, MTU_SIZEf, 0x3fff);
    }
    BCM_IF_ERROR_RETURN(soc_mem_write(unit, hw_mem,
                                  MEM_BLOCK_ALL, nh_index, &ing_nh));

    rv = _bcm_td2_vxlan_next_hop_set(unit, nh_index, flags);
    return rv;
}

/*
 * Function:
 *           bcm_td2_vxlan_egress_reset
 * Purpose:
 *           Reset VXLAN Egress NextHop 
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
bcm_td2_vxlan_egress_reset(int unit, int nh_index)
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

    rv = _bcm_td2_vxlan_nexthop_reset(unit, nh_index);
    return rv;
}

/*
 * Function:
 *           bcm_td2_vxlan_egress_get
 * Purpose:
 *           Get VXLAN Egress NextHop 
 * Parameters:
 *           IN :  Unit
 *           IN :  nh_index
 * Returns:
 *           BCM_E_XXX
 */

int
bcm_td2_vxlan_egress_get(int unit, bcm_l3_egress_t *egr, int nh_index)
{
    ing_l3_next_hop_entry_t ing_nh;

    BCM_IF_ERROR_RETURN(soc_mem_read(unit, ING_L3_NEXT_HOPm, 
                                  MEM_BLOCK_ANY, nh_index, &ing_nh));

    if (0x2 == soc_mem_field32_get(unit, ING_L3_NEXT_HOPm, &ing_nh,
                                  ENTRY_TYPEf)) {
         egr->flags |= BCM_L3_VXLAN_ONLY;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_vxlan_port_nh_add
 * Purpose:
 *      Add VXLAN Next Hop entry
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 */     

STATIC int
_bcm_td2_vxlan_port_nh_add(int unit, bcm_vxlan_port_t *vxlan_port, int vp,
                            bcm_vpn_t vpn, int drop)
{
    egr_l3_next_hop_entry_t egr_nh;
    egr_vlan_xlate_entry_t   vxlate_entry;
    _bcm_td2_vxlan_nh_info_t egr_nh_info;
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
    rv = bcm_xgs3_get_nh_from_egress_object(unit, vxlan_port->egress_if,
                                            &mpath_flag, 1, &vp_nh_ecmp_index);
    BCM_IF_ERROR_RETURN(rv);

    /* Read the existing egress next_hop entry */
    if (mpath_flag == 0) {
       rv = soc_mem_read(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY, 
                      vp_nh_ecmp_index, &egr_nh);
       BCM_IF_ERROR_RETURN(rv);
    }

    if (vxlan_port->flags & BCM_VXLAN_PORT_EGRESS_TUNNEL) {

        /* Program DVP entry - Egress */
        rv = _bcm_td2_vxlan_egress_dvp_set(unit, vp, drop, vxlan_port);
        _BCM_VXLAN_CLEANUP(rv)


        /* Program DVP entry  - Ingress  */
        rv = _bcm_td2_vxlan_ingress_dvp_set(unit, vp, mpath_flag, vp_nh_ecmp_index, vxlan_port);
        _BCM_VXLAN_CLEANUP(rv)

        if (vxlan_port->flags & BCM_VXLAN_PORT_MULTICAST) {
            /* Egress into Network - Tunnel */
            egr_nh_info.dvp_is_network = 1; /* Enable */
            egr_nh_info.entry_type = 
                soc_EGR_L3_NEXT_HOPm_field32_get(unit, &egr_nh, ENTRY_TYPEf);
            rv = _bcm_td2_vxlan_nexthop_entry_modify(unit, vp_nh_ecmp_index, drop, 
                             &egr_nh_info, _BCM_VXLAN_EGR_NEXT_HOP_L3MC_VIEW);
            _BCM_VXLAN_CLEANUP(rv)
        }
    } else {
        /* Program DVP entry - Egress */
        rv = _bcm_td2_vxlan_egress_access_dvp_set(unit, vp, drop, vxlan_port);
        _BCM_VXLAN_CLEANUP(rv)

        /* Egress into Access - Non Tunnel */
        egr_nh_info.dvp_is_network = 0; /* Disable */
        if (vxlan_port->flags & BCM_VXLAN_PORT_REPLACE) {
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

        if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_TAGGED) {
            egr_nh_info.entry_type = _BCM_VXLAN_EGR_NEXT_HOP_SDTAG_VIEW;
            /* Configure SD_TAG setting */
            rv = _bcm_td2_vxlan_sd_tag_set(unit, NULL, vxlan_port, &egr_nh_info, &vxlate_entry, &tpid_index);
            _BCM_VXLAN_CLEANUP(rv)

           /* Configure EGR Next Hop entry -- SD_TAG view */
           rv = _bcm_td2_vxlan_nexthop_entry_modify(unit, vp_nh_ecmp_index, drop, 
                             &egr_nh_info, _BCM_VXLAN_EGR_NEXT_HOP_SDTAG_VIEW);
           _BCM_VXLAN_CLEANUP(rv)
        }
        if (!(vxlan_port->flags & BCM_VXLAN_PORT_MULTICAST)) {
            /* Program DVP entry  with ECMP / Next Hop Index */
            rv = _bcm_td2_vxlan_ingress_dvp_set(unit, vp, mpath_flag, vp_nh_ecmp_index, vxlan_port);
            _BCM_VXLAN_CLEANUP(rv)
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
 *      _bcm_td2_vxlan_port_nh_delete
 * Purpose:
 *      Delete VXLAN Next Hop entry
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 */     

STATIC int
_bcm_td2_vxlan_port_nh_delete(int unit, bcm_vpn_t vpn, int vp)
{
    int rv=BCM_E_NONE;
    int nh_ecmp_index=-1;
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
                     bcm_td2_vxlan_port_egress_nexthop_reset
                                (unit, nh_ecmp_index, vp_type, vp, vpn));
              }
         }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_port_nh_get
 * Purpose:
 *      Get VXLAN Next Hop entry
 * Parameters:
 * Returns:
 *      BCM_E_XXX
 */     

STATIC int
_bcm_td2_vxlan_port_nh_get(int unit, bcm_vpn_t vpn,  int vp, bcm_vxlan_port_t *vxlan_port)
{
    egr_l3_next_hop_entry_t egr_nh;
    ing_dvp_table_entry_t dvp;
    int nh_ecmp_index=0;
    uint32 ecmp=0;
    int rv = BCM_E_NONE;

    /* Read the HW entries */
    BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));

    if (soc_ING_DVP_TABLEm_field32_get(unit, &dvp, VP_TYPEf) == 
                              _BCM_VXLAN_INGRESS_DEST_VP_TYPE) {
        /* Egress into VXLAN tunnel, find the tunnel_if */
        vxlan_port->flags |= BCM_VXLAN_PORT_EGRESS_TUNNEL;
        BCM_IF_ERROR_RETURN(
             READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
        ecmp = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, ECMPf);
        if (ecmp) {
            nh_ecmp_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, ECMP_PTRf);
            vxlan_port->egress_if  =  nh_ecmp_index + BCM_XGS3_MPATH_EGRESS_IDX_MIN;
        } else {
            nh_ecmp_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
            /* Extract next hop index from unipath egress object. */
            vxlan_port->egress_if  =  nh_ecmp_index + BCM_XGS3_EGRESS_IDX_MIN;
        }

        rv = bcm_td2_vxlan_nexthop_get(unit, nh_ecmp_index, vxlan_port);
        BCM_IF_ERROR_RETURN(rv);

    } else if (soc_ING_DVP_TABLEm_field32_get(unit, &dvp, 
                                                VP_TYPEf) == _BCM_VXLAN_DEST_VP_TYPE_ACCESS) {
        /* Egress into Access-side, find the tunnel_if */
        BCM_IF_ERROR_RETURN(
            READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
        nh_ecmp_index = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NEXT_HOP_INDEXf);
        /* Extract next hop index from unipath egress object. */
        vxlan_port->egress_if  =  nh_ecmp_index + BCM_XGS3_EGRESS_IDX_MIN;

        BCM_IF_ERROR_RETURN (soc_mem_read(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY, 
                                      nh_ecmp_index, &egr_nh));
         if (BCM_SUCCESS(rv)) {
              (void) _bcm_td2_vxlan_sd_tag_get( unit, NULL, vxlan_port, &egr_nh,
                         NULL, 0);
         }
    } else {
        return BCM_E_NOT_FOUND;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_vxlan_match_vnid_entry_reset
 * Purpose:
 *      Reset VXLAN Match VNID Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  Vnid
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_match_vnid_entry_reset(int unit, uint32 vnid)
{
    mpls_entry_entry_t ment;

    sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
    soc_MPLS_ENTRYm_field32_set(unit, &ment, VXLAN_VN_ID__VN_IDf, vnid);
    soc_MPLS_ENTRYm_field32_set(
        unit, &ment, KEY_TYPEf, _BCM_VXLAN_KEY_TYPE_VNID_VFI);
    soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 1);

    BCM_IF_ERROR_RETURN(soc_mem_delete(unit, MPLS_ENTRYm, MEM_BLOCK_ALL, &ment));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_vxlan_match_tunnel_entry_update
 * Purpose:
 *      Update Match VXLAN Tunnel Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  vxlan_entry
 *  OUT :  vxlan_entry

 */

STATIC int
_bcm_td2_vxlan_match_tunnel_entry_update(int unit,
                                             mpls_entry_entry_t *ment, 
                                             mpls_entry_entry_t *return_ment)
{
    uint32 value=0, key_type=0;

    /* Check if Key_Type identical */
    key_type = soc_MPLS_ENTRYm_field32_get (unit, ment, KEY_TYPEf);
    value = soc_MPLS_ENTRYm_field32_get (unit, return_ment, KEY_TYPEf);
    if (key_type != value) {
         return BCM_E_PARAM;
    }

    soc_MPLS_ENTRYm_field32_set(unit, return_ment, VALIDf, 1);

    value = soc_MPLS_ENTRYm_field32_get(unit, ment, VXLAN_SIP__SIPf);
    soc_MPLS_ENTRYm_field32_set(unit, return_ment, VXLAN_SIP__SIPf, value);
    value = soc_MPLS_ENTRYm_field32_get(unit, ment, VXLAN_SIP__SVPf);
    soc_MPLS_ENTRYm_field32_set(unit, return_ment, VXLAN_SIP__SVPf, value);

   return BCM_E_NONE;
}


/*
 * Function:
 *      _bcm_td2_vxlan_match_tunnel_entry_set
 * Purpose:
 *      Set VXLAN Match Tunnel Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  mpls_entry
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_match_tunnel_entry_set(int unit, mpls_entry_entry_t *ment)
{
    mpls_entry_entry_t return_ment;
    int rv = BCM_E_UNAVAIL;
    int index=0;

    rv = soc_mem_search(unit, MPLS_ENTRYm, MEM_BLOCK_ANY, &index,
                        ment, &return_ment, 0);

    if (rv == SOC_E_NONE) {
         BCM_IF_ERROR_RETURN(
              _bcm_td2_vxlan_match_tunnel_entry_update (unit, ment, &return_ment));
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
 *      _bcm_td2_vxlan_match_tunnel_entry_reset
 * Purpose:
 *      Reset VXLAN Match Tunnel Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  mpls_entry
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_match_tunnel_entry_reset(int unit, mpls_entry_entry_t *ment)
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
 *      _bcm_td2_vxlan_match_vxlate_entry_update
 * Purpose:
 *      Update VXLAN Match Vlan_xlate  Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  vlan_xlate_entry_t
 *  OUT :  vlan_xlate_entry_t

 */

STATIC int
_bcm_td2_vxlan_match_vxlate_entry_update(int unit, vlan_xlate_entry_t *vent, 
                                         vlan_xlate_entry_t *return_ent)
{
    uint32  vp=0, key_type=0, value=0;

    /* Check if Key_Type identical */
    key_type = soc_VLAN_XLATEm_field32_get (unit, vent, KEY_TYPEf);
    value = soc_VLAN_XLATEm_field32_get (unit, return_ent, KEY_TYPEf);
    if (key_type != value) {
         return BCM_E_PARAM;
    }

    /* Retain original Keys -- Update data only */
    soc_VLAN_XLATEm_field32_set(unit, return_ent, XLATE__MPLS_ACTIONf, 0x1);
    soc_VLAN_XLATEm_field32_set(unit, return_ent, XLATE__DISABLE_VLAN_CHECKSf, 1);
    vp = soc_VLAN_XLATEm_field32_get (unit, vent, XLATE__SOURCE_VPf);
    soc_VLAN_XLATEm_field32_set(unit, return_ent, XLATE__SOURCE_VPf, vp);

   return BCM_E_NONE;
}


/*
 * Function:
 *        _bcm_td2_vxlan_match_vxlate_extd_entry_set
 * Purpose:
 *       Set VXLAN Match Vxlate Entry 
 * Parameters:
 *  IN :  Unit
 *  IN :  vxlan_port
 *  IN :  vlan_xlate_extd_entry_t
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_match_vxlate_entry_set(int unit, bcm_vxlan_port_t *vxlan_port, vlan_xlate_entry_t *vent)
{
    vlan_xlate_entry_t return_vent;
    int rv = BCM_E_UNAVAIL;
    int index=0;

    rv = soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &index,
                        vent, &return_vent, 0);

    if (rv == SOC_E_NONE) {
         BCM_IF_ERROR_RETURN(
              _bcm_td2_vxlan_match_vxlate_entry_update (unit, vent, &return_vent));
         
         if (vxlan_port->flags & BCM_VXLAN_PORT_ENABLE_VLAN_CHECKS){
             soc_VLAN_XLATEm_field32_set(unit, &return_vent, XLATE__VLAN_ACTION_VALIDf, 1);
         } else {
             soc_VLAN_XLATEm_field32_set(unit, &return_vent, XLATE__VLAN_ACTION_VALIDf, 0);
         }
 
         rv = soc_mem_write(unit, VLAN_XLATEm,
                                           MEM_BLOCK_ALL, index,
                                           &return_vent);
    } else if (rv != SOC_E_NOT_FOUND) {
        return rv;
    } else {
         if (vxlan_port->flags & BCM_VXLAN_PORT_REPLACE) {
             return BCM_E_NOT_FOUND;
         } else {
              rv = soc_mem_insert(unit, VLAN_XLATEm, MEM_BLOCK_ALL, vent);
         }
    }

    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_match_clear
 * Purpose:
 *      Clear VXLAN Match Software State
 * Parameters:
 *      unit    - (IN) Device Number
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      void
 */

void
bcm_td2_vxlan_match_clear (int unit, int vp)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info = VXLAN_INFO(unit);

    vxlan_info->match_key[vp].flags = 0;
    vxlan_info->match_key[vp].match_vlan = 0;
    vxlan_info->match_key[vp].match_inner_vlan = 0;
    vxlan_info->match_key[vp].trunk_id = -1;
    vxlan_info->match_key[vp].port = 0;
    vxlan_info->match_key[vp].modid = -1;
    vxlan_info->match_key[vp].match_tunnel_index = 0;

    return;
}

/*
 * Function:
 *      _bcm_td2_vxlan_trunk_table_set
 * Purpose:
 *      Configure VXLAN Trunk port entry
 * Parameters:
 *      unit    - (IN) Device Number
 *      port   - (IN) Trunk member port
 *      tgid - (IN) Trunk group Id
 *      vp   - (IN) Virtual port
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_trunk_table_set(int unit, bcm_port_t port, bcm_trunk_t tgid, int vp)
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
    soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm, &trunk_map_entry,
            SVP_VALIDf, 0x1);

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
 *      _bcm_td2_vxlan_trunk_table_reset
 * Purpose:
 *      Reset VXLAN Trunk port entry
 * Parameters:
 *      unit    - (IN) Device Number
 *      port   - (IN) Trunk member port
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_trunk_table_reset(int unit, bcm_port_t port)
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

    /* Enable SVP Mode */
    soc_mem_field32_set(unit, SOURCE_TRUNK_MAP_TABLEm, &trunk_map_entry,
            SVP_VALIDf, 0);

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
 *      bcm_td2_vxlan_match_trunk_add
 * Purpose:
 *      Assign SVP of  VXLAN Trunk port
 * Parameters:
 *      unit    - (IN) Device Number
 *      tgid - (IN) Trunk group Id
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
 bcm_td2_vxlan_match_trunk_add(int unit, bcm_trunk_t tgid, int vp)
{
    int port_idx=0;
    int local_port_out[SOC_MAX_NUM_PORTS];
    int local_member_count=0;
    int rv = BCM_E_NONE;

    BCM_IF_ERROR_RETURN(_bcm_esw_trunk_local_members_get(unit, tgid,
                SOC_MAX_NUM_PORTS, local_port_out, &local_member_count));
    for (port_idx = 0; port_idx < local_member_count; port_idx++) {
        rv = _bcm_td2_vxlan_trunk_table_set(unit, local_port_out[port_idx], tgid, vp);
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
        (void) _bcm_td2_vxlan_trunk_table_reset(unit, local_port_out[port_idx]);
    }
    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_match_trunk_delete
 * Purpose:
 *      Remove SVP of VXLAN Trunk port
 * Parameters:
 *      unit    - (IN) Device Number
 *      tgid - (IN) Trunk group Id
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
bcm_td2_vxlan_match_trunk_delete(int unit, bcm_trunk_t tgid, int vp)
{
    int port_idx=0;
    int local_port_out[SOC_MAX_NUM_PORTS];
    int local_member_count=0;
    int rv = BCM_E_NONE;

    BCM_IF_ERROR_RETURN(_bcm_esw_trunk_local_members_get(unit, tgid,
                SOC_MAX_NUM_PORTS, local_port_out, &local_member_count));

    for (port_idx = 0; port_idx < local_member_count; port_idx++) {
        rv = _bcm_td2_vxlan_trunk_table_reset(unit, local_port_out[port_idx]);
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
        (void) _bcm_td2_vxlan_trunk_table_set(unit, local_port_out[port_idx], tgid, vp);
    }
    return rv;
}


/*
 * Function:
 *      _bcm_td2_vxlan_match_add
 * Purpose:
 *      Assign match criteria of an VXLAN port
 * Parameters:
 *      unit    - (IN) Device Number
 *      vxlan_port - (IN) vxlan port information
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_td2_vxlan_match_add(int unit, bcm_vxlan_port_t *vxlan_port, int vp, bcm_vpn_t vpn)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info = VXLAN_INFO(unit);
    int rv = BCM_E_NONE;
    bcm_module_t mod_out = -1;
    bcm_port_t port_out = -1;
    bcm_trunk_t trunk_id = -1;
    vlan_xlate_entry_t vent;
    int    mod_id_idx=0; /* Module Id */
    int    src_trk_idx=0;  /*Source Trunk table index.*/
    int    gport_id=-1;

    rv = _bcm_td2_vxlan_port_resolve(unit, vxlan_port->vxlan_port_id, -1, &mod_out,
                    &port_out, &trunk_id, &gport_id);
    BCM_IF_ERROR_RETURN(rv);

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));

    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &mod_id_idx));

    if (vxlan_port->criteria == BCM_VXLAN_PORT_MATCH_PORT_VLAN ) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);

        if (vxlan_port->flags & BCM_VXLAN_PORT_ENABLE_VLAN_CHECKS){
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__VLAN_ACTION_VALIDf, 1);
        }
        
        if (!BCM_VLAN_VALID(vxlan_port->match_vlan)) {
             return BCM_E_PARAM;
        }
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                        TR_VLXLT_HASH_KEY_TYPE_OVID);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__OVIDf, 
                                        vxlan_port->match_vlan);
        vxlan_info->match_key[vp].flags = _BCM_VXLAN_PORT_MATCH_TYPE_VLAN;
        vxlan_info->match_key[vp].match_vlan = vxlan_port->match_vlan;

        if (BCM_GPORT_IS_TRUNK(vxlan_port->match_port)) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
            vxlan_info->match_key[vp].trunk_id = trunk_id;
        } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
            vxlan_info->match_key[vp].port = port_out;
            vxlan_info->match_key[vp].modid = mod_out;
        }
        rv = _bcm_td2_vxlan_match_vxlate_entry_set(unit, vxlan_port, &vent);
        BCM_IF_ERROR_RETURN(rv);
        if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
              bcm_td2_vxlan_port_match_count_adjust(unit, vp, 1);
        }
    } else if (vxlan_port->criteria ==
                            BCM_VXLAN_PORT_MATCH_PORT_INNER_VLAN) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);
        if (vxlan_port->flags & BCM_VXLAN_PORT_ENABLE_VLAN_CHECKS){
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__VLAN_ACTION_VALIDf, 1);
        }
        
        if (!BCM_VLAN_VALID(vxlan_port->match_inner_vlan)) {
            return BCM_E_PARAM;
        }
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf,
                                    TR_VLXLT_HASH_KEY_TYPE_IVID);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__IVIDf,
                                    vxlan_port->match_inner_vlan);
        vxlan_info->match_key[vp].flags = _BCM_VXLAN_PORT_MATCH_TYPE_INNER_VLAN;
        vxlan_info->match_key[vp].match_inner_vlan = vxlan_port->match_inner_vlan;

        if (BCM_GPORT_IS_TRUNK(vxlan_port->match_port)) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
            vxlan_info->match_key[vp].trunk_id = trunk_id;
        } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
            vxlan_info->match_key[vp].port = port_out;
            vxlan_info->match_key[vp].modid = mod_out;
        }
        rv = _bcm_td2_vxlan_match_vxlate_entry_set(unit, vxlan_port, &vent);
        BCM_IF_ERROR_RETURN(rv);
        if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
             bcm_td2_vxlan_port_match_count_adjust(unit, vp, 1);
        }
    } else if (vxlan_port->criteria == 
                            BCM_VXLAN_PORT_MATCH_PORT_VLAN_STACKED) {
         soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
         soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
         soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
         soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
         soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);
         if (vxlan_port->flags & BCM_VXLAN_PORT_ENABLE_VLAN_CHECKS){
             soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__VLAN_ACTION_VALIDf, 1);
         }
         
         if (!BCM_VLAN_VALID(vxlan_port->match_vlan) || 
                !BCM_VLAN_VALID(vxlan_port->match_inner_vlan)) {
              return BCM_E_PARAM;
         }
         soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                   TR_VLXLT_HASH_KEY_TYPE_IVID_OVID);
         soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__OVIDf, 
                   vxlan_port->match_vlan);
         soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__IVIDf, 
                   vxlan_port->match_inner_vlan);
         vxlan_info->match_key[vp].flags = _BCM_VXLAN_PORT_MATCH_TYPE_VLAN_STACKED;
         vxlan_info->match_key[vp].match_vlan = vxlan_port->match_vlan;
         vxlan_info->match_key[vp].match_inner_vlan = vxlan_port->match_inner_vlan;
         if (BCM_GPORT_IS_TRUNK(vxlan_port->match_port)) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
            vxlan_info->match_key[vp].trunk_id = trunk_id;
         } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
            vxlan_info->match_key[vp].port = port_out;
            vxlan_info->match_key[vp].modid = mod_out;
         }
         rv = _bcm_td2_vxlan_match_vxlate_entry_set(unit, vxlan_port, &vent);
         BCM_IF_ERROR_RETURN(rv);
         if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
             bcm_td2_vxlan_port_match_count_adjust(unit, vp, 1);
         }
    } else if (vxlan_port->criteria == BCM_VXLAN_PORT_MATCH_VLAN_PRI) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MPLS_ACTIONf, 0x1); /* SVP */
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__DISABLE_VLAN_CHECKSf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__SOURCE_VPf, vp);
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                         TR_VLXLT_HASH_KEY_TYPE_PRI_CFI);
        /* match_vlan : Bits 12-15 contains VLAN_PRI + CFI, vlan=BCM_E_NONE */
        soc_VLAN_XLATEm_field32_set(unit, &vent, OTAGf, 
                                         vxlan_port->match_vlan);
        
        if (vxlan_port->flags & BCM_VXLAN_PORT_ENABLE_VLAN_CHECKS){
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__VLAN_ACTION_VALIDf, 1);
        }
       
        vxlan_info->match_key[vp].flags = _BCM_VXLAN_PORT_MATCH_TYPE_VLAN_PRI;
        vxlan_info->match_key[vp].match_vlan = vxlan_port->match_vlan;

        if (BCM_GPORT_IS_TRUNK(vxlan_port->match_port)) {
           soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
           soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, trunk_id);
                vxlan_info->match_key[vp].trunk_id = trunk_id;
        } else {
           soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, mod_out);
           soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, port_out);
           vxlan_info->match_key[vp].port = port_out;
           vxlan_info->match_key[vp].modid = mod_out;
        }
        rv = _bcm_td2_vxlan_match_vxlate_entry_set(unit, vxlan_port, &vent);
        BCM_IF_ERROR_RETURN(rv);
        if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
           bcm_td2_vxlan_port_match_count_adjust(unit, vp, 1);
        }
    } else if (vxlan_port->criteria == BCM_VXLAN_PORT_MATCH_PORT) {
        if (BCM_GPORT_IS_TRUNK(vxlan_port->match_port)) {
            rv = bcm_td2_vxlan_match_trunk_add(unit, trunk_id, vp);
            if (rv >= 0) {
                vxlan_info->match_key[vp].flags = _BCM_VXLAN_PORT_MATCH_TYPE_TRUNK;
                vxlan_info->match_key[vp].trunk_id = trunk_id;
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

            /* Enable SVP Mode */
            if (soc_mem_field_valid(unit, SOURCE_TRUNK_MAP_TABLEm, SVP_VALIDf) ) {
                rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm,
                                        src_trk_idx, SVP_VALIDf, 0x1);
                BCM_IF_ERROR_RETURN(rv);
            }
            rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm, 
                                        src_trk_idx, SOURCE_VPf, vp);
            BCM_IF_ERROR_RETURN(rv);

            if (is_local) {
                rv = soc_mem_field32_modify(unit, PORT_TABm, port_out,
                                        PORT_OPERATIONf, 0x1); /* L2_SVP */
                BCM_IF_ERROR_RETURN(rv);

                /* Set TAG_ACTION_PROFILE_PTR */
                rv = _bcm_td2_vxlan_port_untagged_profile_set(unit, port_out);
                BCM_IF_ERROR_RETURN(rv);
            }
            vxlan_info->match_key[vp].flags = _BCM_VXLAN_PORT_MATCH_TYPE_PORT;
            vxlan_info->match_key[vp].index = src_trk_idx;
        }
    } else if (vxlan_port->criteria == BCM_VXLAN_PORT_MATCH_VN_ID) {
        mpls_entry_entry_t ment;
        int tunnel_idx=-1;
        uint32 tunnel_sip;

        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
        vxlan_info->match_key[vp].flags = _BCM_VXLAN_PORT_MATCH_TYPE_VNID;
        if (!BCM_GPORT_IS_TUNNEL(vxlan_port->match_tunnel_id)) {
            return BCM_E_PARAM;
        }

        tunnel_idx = BCM_GPORT_TUNNEL_ID_GET(vxlan_port->match_tunnel_id);
        tunnel_sip = vxlan_info->vxlan_tunnel_term[tunnel_idx].sip;
        vxlan_info->match_key[vp].match_tunnel_index = tunnel_idx;

        if (vxlan_port->flags & BCM_VXLAN_PORT_MULTICAST) {
              return BCM_E_NONE;
        }

        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VXLAN_SIP__SIPf, tunnel_sip);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VXLAN_SIP__SVPf, vp);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, KEY_TYPEf, _BCM_VXLAN_KEY_TYPE_TUNNEL);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 1);

        rv = _bcm_td2_vxlan_match_tunnel_entry_set(unit, &ment);
        BCM_IF_ERROR_RETURN(rv);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_match_delete
 * Purpose:
 *      Delete match criteria of an VXLAN port
 * Parameters:
 *      unit    - (IN) Device Number
 *      vxlan_port - (IN) VXLAN port information
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_td2_vxlan_match_delete(int unit,  int vp)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info = VXLAN_INFO(unit);
    int rv=BCM_E_NONE;
    vlan_xlate_entry_t vent;
    bcm_trunk_t trunk_id;
    int    src_trk_idx=0;   /*Source Trunk table index.*/
    int    mod_id_idx=0;   /* Module_Id */
    int port=0, tunnel_index = -1;
    mpls_entry_entry_t ment;

    sal_memset(&vent, 0, sizeof(vlan_xlate_entry_t));

    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &mod_id_idx));

    if  (vxlan_info->match_key[vp].flags == _BCM_VXLAN_PORT_MATCH_TYPE_VLAN) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                    TR_VLXLT_HASH_KEY_TYPE_OVID);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__OVIDf,
                                    vxlan_info->match_key[vp].match_vlan);
        if (vxlan_info->match_key[vp].modid != -1) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                        vxlan_info->match_key[vp].modid);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                        vxlan_info->match_key[vp].port);
        } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, 
                                        vxlan_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);
        
        (void) bcm_td2_vxlan_match_clear (unit, vp);
         bcm_td2_vxlan_port_match_count_adjust(unit, vp, -1);

    } else if  (vxlan_info->match_key[vp].flags == 
                     _BCM_VXLAN_PORT_MATCH_TYPE_INNER_VLAN) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                    TR_VLXLT_HASH_KEY_TYPE_IVID);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__IVIDf,
                                    vxlan_info->match_key[vp].match_inner_vlan);
        if (vxlan_info->match_key[vp].modid != -1) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                        vxlan_info->match_key[vp].modid);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                        vxlan_info->match_key[vp].port);
        } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, 
                                        vxlan_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);
        
        (void) bcm_td2_vxlan_match_clear (unit, vp);
        bcm_td2_vxlan_port_match_count_adjust(unit, vp, -1);
    }else if (vxlan_info->match_key[vp].flags == 
                    _BCM_VXLAN_PORT_MATCH_TYPE_VLAN_STACKED) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                    TR_VLXLT_HASH_KEY_TYPE_IVID_OVID);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__OVIDf,
                                    vxlan_info->match_key[vp].match_vlan);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__IVIDf,
                              vxlan_info->match_key[vp].match_inner_vlan);
        if (vxlan_info->match_key[vp].modid != -1) {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                        vxlan_info->match_key[vp].modid);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                        vxlan_info->match_key[vp].port);
        } else {
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
            soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, 
                                        vxlan_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);

        (void) bcm_td2_vxlan_match_clear (unit, vp);
        bcm_td2_vxlan_port_match_count_adjust(unit, vp, -1);

    } else if   (vxlan_info->match_key[vp].flags ==
                                               _BCM_VXLAN_PORT_MATCH_TYPE_VLAN_PRI) {
        soc_VLAN_XLATEm_field32_set(unit, &vent, SOURCE_TYPEf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, VALIDf, 1);
        soc_VLAN_XLATEm_field32_set(unit, &vent, KEY_TYPEf, 
                                                      TR_VLXLT_HASH_KEY_TYPE_PRI_CFI);
        soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__OTAGf,
                                                       vxlan_info->match_key[vp].match_vlan);
        if (vxlan_info->match_key[vp].modid != -1) {
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__MODULE_IDf, 
                                                      vxlan_info->match_key[vp].modid);
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__PORT_NUMf, 
                                                      vxlan_info->match_key[vp].port);
        } else {
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__Tf, 1);
              soc_VLAN_XLATEm_field32_set(unit, &vent, XLATE__TGIDf, 
                                                      vxlan_info->match_key[vp].trunk_id);
        }
        rv = soc_mem_delete(unit, VLAN_XLATEm, MEM_BLOCK_ANY, &vent);
        BCM_IF_ERROR_RETURN(rv);

        (void) bcm_td2_vxlan_match_clear (unit, vp);
        bcm_td2_vxlan_port_match_count_adjust(unit, vp, -1);
    }else if  (vxlan_info->match_key[vp].flags == 
                    _BCM_VXLAN_PORT_MATCH_TYPE_PORT) {
         int is_local;

         src_trk_idx = vxlan_info->match_key[vp].index;
         rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm, 
                                  src_trk_idx, SOURCE_VPf, 0);
         BCM_IF_ERROR_RETURN(rv);

         /* Disable SVP Mode */
         if (soc_mem_field_valid(unit, SOURCE_TRUNK_MAP_TABLEm, SVP_VALIDf) ) {
                rv = soc_mem_field32_modify(unit, SOURCE_TRUNK_MAP_TABLEm,
                                        src_trk_idx, SVP_VALIDf, 0x0);
                BCM_IF_ERROR_RETURN(rv);
         }

         BCM_IF_ERROR_RETURN(_bcm_esw_src_modid_port_get( unit, src_trk_idx,
                                                          &mod_id_idx, &port));
         BCM_IF_ERROR_RETURN
             ( _bcm_esw_modid_is_local(unit, mod_id_idx, &is_local));

         if (is_local) {
            rv = soc_mem_field32_modify(unit, PORT_TABm, port,
                                       PORT_OPERATIONf, 0x0); /* NORMAL */
            BCM_IF_ERROR_RETURN(rv);

            /* Reset TAG_ACTION_PROFILE_PTR */
            rv = _bcm_td2_vxlan_port_untagged_profile_reset(unit, port);
            BCM_IF_ERROR_RETURN(rv);
         }

        (void) bcm_td2_vxlan_match_clear (unit, vp);
    } else if  (vxlan_info->match_key[vp].flags == 
                  _BCM_VXLAN_PORT_MATCH_TYPE_TRUNK) {
         trunk_id = vxlan_info->match_key[vp].trunk_id;
         rv = bcm_td2_vxlan_match_trunk_delete(unit, trunk_id, vp);
         BCM_IF_ERROR_RETURN(rv);

        (void) bcm_td2_vxlan_match_clear (unit, vp);
    } else if (vxlan_info->match_key[vp].flags == 
                  _BCM_VXLAN_PORT_MATCH_TYPE_VNID) {

        sal_memset(&ment, 0, sizeof(mpls_entry_entry_t));

        tunnel_index = vxlan_info->match_key[vp].match_tunnel_index;
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VXLAN_SIP__SIPf, 
                                    vxlan_info->vxlan_tunnel_term[tunnel_index].sip);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, KEY_TYPEf, _BCM_VXLAN_KEY_TYPE_TUNNEL);
        soc_MPLS_ENTRYm_field32_set(unit, &ment, VALIDf, 1);

        rv = _bcm_td2_vxlan_match_tunnel_entry_reset(unit, &ment);
        if (BCM_SUCCESS(rv) && (tunnel_index != -1)) {
           vxlan_info->vxlan_tunnel_term[tunnel_index].sip = 0;
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_match_get
 * Purpose:
 *      Obtain match information of an VXLAN port
 * Parameters:
 *      unit    - (IN) Device Number
 *      vxlan_port - (OUT) VXLAN port information
 *      vp  - (IN) Source Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_td2_vxlan_match_get(int unit, bcm_vxlan_port_t *vxlan_port, int vp)
{
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info = VXLAN_INFO(unit);
    int rv = BCM_E_NONE;
    bcm_module_t mod_in=0, mod_out=0;
    bcm_port_t port_in=0, port_out=0;
    bcm_trunk_t trunk_id=0;
    int    src_trk_idx=0;    /*Source Trunk table index.*/
    int    tunnel_idx = -1;
    int    mode_in=0;   /* Module_Id */
    
    /* Get module id for unit. */
    BCM_IF_ERROR_RETURN(bcm_esw_stk_my_modid_get(unit, &mode_in));

    if  (vxlan_info->match_key[vp].flags &  _BCM_VXLAN_PORT_MATCH_TYPE_VLAN) {

         vxlan_port->criteria = BCM_VXLAN_PORT_MATCH_PORT_VLAN;
         vxlan_port->match_vlan = vxlan_info->match_key[vp].match_vlan;

        if (vxlan_info->match_key[vp].trunk_id != -1) {
             trunk_id = vxlan_info->match_key[vp].trunk_id;
             BCM_GPORT_TRUNK_SET(vxlan_port->match_port, trunk_id);
        } else {
             port_in = vxlan_info->match_key[vp].port;
             mod_in = vxlan_info->match_key[vp].modid;
             rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                  mod_in, port_in, &mod_out, &port_out);
             BCM_GPORT_MODPORT_SET(vxlan_port->match_port, mod_out, port_out);
        }
    } else if (vxlan_info->match_key[vp].flags &  
                 _BCM_VXLAN_PORT_MATCH_TYPE_INNER_VLAN) {
         vxlan_port->criteria = BCM_VXLAN_PORT_MATCH_PORT_INNER_VLAN;
         vxlan_port->match_inner_vlan = vxlan_info->match_key[vp].match_inner_vlan;

        if (vxlan_info->match_key[vp].trunk_id != -1) {
             trunk_id = vxlan_info->match_key[vp].trunk_id;
             BCM_GPORT_TRUNK_SET(vxlan_port->match_port, trunk_id);
        } else {
             port_in = vxlan_info->match_key[vp].port;
             mod_in = vxlan_info->match_key[vp].modid;
             rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                  mod_in, port_in, &mod_out, &port_out);
             BCM_GPORT_MODPORT_SET(vxlan_port->match_port, mod_out, port_out);
        }
    }else if (vxlan_info->match_key[vp].flags &
                    _BCM_VXLAN_PORT_MATCH_TYPE_VLAN_STACKED) {

         vxlan_port->criteria = BCM_VXLAN_PORT_MATCH_PORT_VLAN_STACKED;
         vxlan_port->match_vlan = vxlan_info->match_key[vp].match_vlan;
         vxlan_port->match_inner_vlan = vxlan_info->match_key[vp].match_inner_vlan;

         if (vxlan_info->match_key[vp].trunk_id != -1) {
              trunk_id = vxlan_info->match_key[vp].trunk_id;
              BCM_GPORT_TRUNK_SET(vxlan_port->match_port, trunk_id);
         } else {
              port_in = vxlan_info->match_key[vp].port;
              mod_in = vxlan_info->match_key[vp].modid;
              rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                             mod_in, port_in, &mod_out, &port_out);
              BCM_GPORT_MODPORT_SET(vxlan_port->match_port, mod_out, port_out);
         }
    } else if  (vxlan_info->match_key[vp].flags &  _BCM_VXLAN_PORT_MATCH_TYPE_VLAN_PRI) {

         vxlan_port->criteria = BCM_VXLAN_PORT_MATCH_VLAN_PRI;
         vxlan_port->match_vlan = vxlan_info->match_key[vp].match_vlan;

        if (vxlan_info->match_key[vp].trunk_id != -1) {
             trunk_id = vxlan_info->match_key[vp].trunk_id;
             BCM_GPORT_TRUNK_SET(vxlan_port->match_port, trunk_id);
        } else {
             port_in = vxlan_info->match_key[vp].port;
             mod_in = vxlan_info->match_key[vp].modid;
             rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                                  mod_in, port_in, &mod_out, &port_out);
             BCM_GPORT_MODPORT_SET(vxlan_port->match_port, mod_out, port_out);
        }
    } else if (vxlan_info->match_key[vp].flags &
                   _BCM_VXLAN_PORT_MATCH_TYPE_PORT) {

         src_trk_idx = vxlan_info->match_key[vp].index;
         BCM_IF_ERROR_RETURN(_bcm_esw_src_modid_port_get( unit, src_trk_idx,
                                                          &mode_in, &port_in));

         vxlan_port->criteria = BCM_VXLAN_PORT_MATCH_PORT;
         rv = _bcm_esw_stk_modmap_map(unit, BCM_STK_MODMAP_GET,
                               mod_in, port_in, &mod_out, &port_out);
         BCM_GPORT_MODPORT_SET(vxlan_port->match_port, mod_out, port_out);
    }else if (vxlan_info->match_key[vp].flags &
                  _BCM_VXLAN_PORT_MATCH_TYPE_TRUNK) {

         trunk_id = vxlan_info->match_key[vp].trunk_id;
         vxlan_port->criteria = BCM_VXLAN_PORT_MATCH_PORT;
         BCM_GPORT_TRUNK_SET(vxlan_port->match_port, trunk_id);
    } else if ((vxlan_info->match_key[vp].flags &
                          _BCM_VXLAN_PORT_MATCH_TYPE_VNID)) {
        vxlan_port->criteria = BCM_VXLAN_PORT_MATCH_VN_ID;

        tunnel_idx = vxlan_info->match_key[vp].match_tunnel_index;
        BCM_GPORT_TUNNEL_ID_SET(vxlan_port->match_tunnel_id, tunnel_idx);

    } else {
        vxlan_port->criteria = BCM_VXLAN_PORT_MATCH_NONE;
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_eline_vp_map_set
 * Purpose:
 *      Set VXLAN ELINE ports from VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_td2_vxlan_eline_vp_map_set(int unit, int vfi_index, int vp1, int vp2)
{
    vfi_entry_t vfi_entry;
    int rv=BCM_E_NONE;
    int num_vp=0;

    if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeVxlan)) {
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
 *      _bcm_td2_vxlan_eline_vp_map_get
 * Purpose:
 *      Get VXLAN ELINE ports from VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_eline_vp_map_get(int unit, int vfi_index, int *vp1, int *vp2)
{
    vfi_entry_t vfi_entry;

    if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeVxlan)) {
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
 *      _bcm_td2_vxlan_eline_vp_map_clear
 * Purpose:
 *      Clear VXLAN ELINE ports from VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_eline_vp_map_clear(int unit, int vfi_index, int vp1, int vp2)
{
    vfi_entry_t vfi_entry;
    int rv=BCM_E_NONE;
    int num_vp=0;

    if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeVxlan)) {
        return BCM_E_NOT_FOUND;
    }

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    BCM_IF_ERROR_RETURN
        (READ_VFIm(unit, MEM_BLOCK_ALL, vfi_index, &vfi_entry));

    if (soc_VFIm_field32_get(unit, &vfi_entry, PT2PT_ENf)) {
        if ((vp1 > 0) && (vp1 < num_vp)) {
            soc_VFIm_field32_set(unit, &vfi_entry, VP_0f, 0);
        }
        if ((vp2 > 0) && (vp2 < num_vp)) {
            soc_VFIm_field32_set(unit, &vfi_entry, VP_1f, 0);
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
 *      _bcm_td2_vxlan_eline_vp_configure
 * Purpose:
 *      Configure VXLAN ELINE virtual port
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_eline_vp_configure (int unit, int vfi_index, int active_vp, 
                  source_vp_entry_t *svp, int tpid_enable, bcm_vxlan_port_t  *vxlan_port)
{
    int rv = BCM_E_NONE;
    source_vp_2_entry_t svp_2_entry;

    /* Set SOURCE_VP */
    soc_SOURCE_VPm_field32_set(unit, svp, CLASS_IDf,
                                vxlan_port->if_class);

    if (vxlan_port->flags & BCM_VXLAN_PORT_NETWORK) {
         soc_SOURCE_VPm_field32_set(unit, svp, NETWORK_PORTf, 1);
         soc_SOURCE_VPm_field32_set(unit, svp, TPID_SOURCEf, _BCM_VXLAN_TPID_SVP_BASED);
    } else {
         soc_SOURCE_VPm_field32_set(unit, svp, NETWORK_PORTf, 0);
         soc_SOURCE_VPm_field32_set(unit, svp, TPID_SOURCEf, _BCM_VXLAN_TPID_SGLP_BASED);
         /*  Configure IPARS Parser to signal to IMPLS Parser - Applicable only for HG2 PPD2 packets 
               to reconstruct TPID state based on HG header information - Only for VXLAN Access ports */
         sal_memset(&svp_2_entry, 0, sizeof(source_vp_2_entry_t));
         soc_SOURCE_VP_2m_field32_set(unit, &svp_2_entry, PARSE_USING_SGLP_TPIDf, 1);
         rv = WRITE_SOURCE_VP_2m(unit, MEM_BLOCK_ALL, active_vp, &svp_2_entry);
         if (rv < 0) {
             return rv;
         }
    }

    if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_TAGGED) {
         soc_SOURCE_VPm_field32_set(unit, svp, SD_TAG_MODEf, 1);
         soc_SOURCE_VPm_field32_set(unit, svp, TPID_ENABLEf, tpid_enable);
    } else {
         soc_SOURCE_VPm_field32_set(unit, svp, SD_TAG_MODEf, 0);
    }

    soc_SOURCE_VPm_field32_set(unit, svp, DISABLE_VLAN_CHECKSf, 1);
    soc_SOURCE_VPm_field32_set(unit, svp, 
                           ENTRY_TYPEf, _BCM_VXLAN_SOURCE_VP_TYPE_VFI); /* VFI Type */
    soc_SOURCE_VPm_field32_set(unit, svp, VFIf, vfi_index);

    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, active_vp, svp);
    return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_access_niv_pe_set
 * Purpose:
 *      Set VXLAN Access virtual port which is of NIV/PE type
 * Parameters:
 *      unit    - (IN) Device Number
 *      vp      -  (IN) Access Virtual Port
 *      vfi      -  (IN) Virtial forwarding instance
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_access_niv_pe_set (int unit, int vp, int vfi)
{
    source_vp_entry_t svp;
    int  rv = BCM_E_PARAM; 

        rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
        if (rv < 0) {
            return rv;
        }

        if (vfi == _BCM_VXLAN_VFI_INVALID) {
            soc_SOURCE_VPm_field32_set(unit, &svp, 
                                       ENTRY_TYPEf, _BCM_VXLAN_SOURCE_VP_TYPE_INVALID);
        } else {
            /* Initialize VP parameters */
            soc_SOURCE_VPm_field32_set(unit, &svp, 
                                       ENTRY_TYPEf, _BCM_VXLAN_SOURCE_VP_TYPE_VFI);
        }     
        soc_SOURCE_VPm_field32_set(unit, &svp, VFIf, vfi);
        rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
        return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_access_niv_pe_reset
 * Purpose:
 *      Reset VXLAN Access virtual port which is of NIV/PE type
 * Parameters:
 *      unit    - (IN) Device Number
 *      vp      -  (IN) Access Virtual Port
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_access_niv_pe_reset (int unit, int vp)
{
    source_vp_entry_t svp;
    int  rv = BCM_E_PARAM; 

        rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
        if (rv < 0) {
            return rv;
        }

        /* Initialize VP parameters */
        soc_SOURCE_VPm_field32_set(unit, &svp, 
                                       ENTRY_TYPEf, _BCM_VXLAN_SOURCE_VP_TYPE_VLAN);
        soc_SOURCE_VPm_field32_set(unit, &svp, VFIf, 0);
        rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
        return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_eline_port_add
 * Purpose:
 *      Add VXLAN ELINE port to a VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 *      vxlan_port - (IN/OUT) VXLAN port information (OUT : vxlan_port_id)
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_eline_port_add(int unit, bcm_vpn_t vpn, bcm_vxlan_port_t  *vxlan_port)
{
    int active_vp = 0; /* default VP */
    source_vp_entry_t svp1, svp2;
    uint8 vp_valid_flag = 0;
    int tpid_enable = 0, tpid_index=-1;
    int customer_drop=0;
    int  rv = BCM_E_PARAM; 
    int num_vp=0;
    int vp1=0, vp2=0, vfi_index= -1;


        if ( vpn != BCM_VXLAN_VPN_INVALID) {
            _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VXLAN_VPN_TYPE_ELINE,  vpn);
             if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeVxlan)) {
                return BCM_E_NOT_FOUND;
             }
        } else {
             vfi_index = _BCM_VXLAN_VFI_INVALID;
        }

        num_vp = soc_mem_index_count(unit, SOURCE_VPm);
        if ( vpn != BCM_VXLAN_VPN_INVALID) {

             /* ---- Read in current table values for VP1 and VP2 ----- */
             (void) _bcm_td2_vxlan_eline_vp_map_get (unit, vfi_index,  &vp1,  &vp2);

            if ( 0 < _bcm_vp_used_get(unit, vp1, _bcmVpTypeVxlan)) {
                BCM_IF_ERROR_RETURN (
                   READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp1, &svp1));
                if (soc_SOURCE_VPm_field32_get(unit, &svp1, ENTRY_TYPEf) == 0x1) {
                    vp_valid_flag |= 0x1;  /* -- VP1 Valid ----- */
                }
            }

            if (0 < _bcm_vp_used_get(unit, vp2, _bcmVpTypeVxlan)) {
                BCM_IF_ERROR_RETURN (
                   READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp2, &svp2));
                if (soc_SOURCE_VPm_field32_get(unit, &svp2, ENTRY_TYPEf) == 0x1) {
                    vp_valid_flag |= 0x2;        /* -- VP2 Valid ----- */
                }
            }

            if (vxlan_port->flags & BCM_VXLAN_PORT_REPLACE) {
                active_vp = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port->vxlan_port_id);
                if (active_vp == -1) {
                   return BCM_E_PARAM;
                }
                if (!_bcm_vp_used_get(unit, active_vp, _bcmVpTypeVxlan)) {
                     return BCM_E_NOT_FOUND;
                }
                /* Decrement old-port's nexthop count */
                rv = _bcm_td2_vxlan_port_nh_cnt_dec(unit, active_vp);
                if (rv < 0) {
                    return rv;
                }
                /* Decrement old-port's VP count */
                rv = _bcm_td2_vxlan_port_cnt_update(unit, vxlan_port->vxlan_port_id,
                                                     active_vp, FALSE);
                if (rv < 0) {
                    return rv;
                }
            }
        }

        switch (vp_valid_flag) {

              case 0x0: /* No VP is valid */
                             if (vxlan_port->flags & BCM_VXLAN_PORT_REPLACE) {
                                  return BCM_E_NOT_FOUND;
                             }

                             if (vxlan_port->flags & BCM_VXLAN_PORT_WITH_ID) {
                                vp1 = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port->vxlan_port_id);
                                if (vp1 == -1) {
                                     return BCM_E_PARAM;
                                }

                                if (_bcm_vp_used_get(unit, vp1, _bcmVpTypeVxlan)) {
                                     return BCM_E_EXISTS;
                                }
                                if (vp1 >= num_vp) {
                                     return (BCM_E_BADID);
                                }
                                BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp1, _bcmVpTypeVxlan));

                             } else {

                                /* No entries are used, allocate a new VP index for VP1 */
                                rv = _bcm_vp_alloc(unit, 0, (num_vp - 1), 1, SOURCE_VPm, _bcmVpTypeVxlan, &vp1);
                                if (rv < 0) {
                                  return rv;
                                }
                                /* Allocate a new VP index for VP2 */
                                 rv = _bcm_vp_alloc(unit, 0, (num_vp - 1), 1, SOURCE_VPm, _bcmVpTypeVxlan, &vp2);
                                  if (rv < 0) {
                                     (void) _bcm_vp_free(unit, _bcmVpTypeVxlan, 1, vp1);
                                      return rv;
                                  }
                             }

                             active_vp = vp1;
                             vp_valid_flag = 1;
                             sal_memset(&svp1, 0, sizeof(source_vp_entry_t));
                             sal_memset(&svp2, 0, sizeof(source_vp_entry_t));
                             (void) _bcm_td2_vxlan_eline_vp_map_set(unit, vfi_index, vp1, vp2);
                             break;


         case 0x1:    /* Only VP1 is valid */   
                             if (vxlan_port->flags & BCM_VXLAN_PORT_REPLACE) {
                                  if (active_vp != vp1) {
                                       return BCM_E_NOT_FOUND;
                                  }
                             } else if (vxlan_port->flags & BCM_VXLAN_PORT_WITH_ID) {
                                vp2 = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port->vxlan_port_id);
                                if (vp2 == -1) {
                                     return BCM_E_PARAM;
                                }

                                if (_bcm_vp_used_get(unit, vp2, _bcmVpTypeVxlan)) {
                                     return BCM_E_EXISTS;
                                }
                                if (vp2 >= num_vp) {
                                     return (BCM_E_BADID);
                                }
                                BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp2, _bcmVpTypeVxlan));

                             } else {
                                  active_vp = vp2;
                                  vp_valid_flag = 3;
                                  sal_memset(&svp2, 0, sizeof(source_vp_entry_t));
                             }

                             (void) _bcm_td2_vxlan_eline_vp_map_set(unit, vfi_index, vp1, vp2);
                             break;



         case 0x2: /* Only VP2 is valid */
                             if (vxlan_port->flags & BCM_VXLAN_PORT_REPLACE) {
                                  if (active_vp != vp2) {
                                       return BCM_E_NOT_FOUND;
                                  }
                             } else if (vxlan_port->flags & BCM_VXLAN_PORT_WITH_ID) {
                                vp1 = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port->vxlan_port_id);
                                if (vp1 == -1) {
                                     return BCM_E_PARAM;
                                }

                                if (_bcm_vp_used_get(unit, vp1, _bcmVpTypeVxlan)) {
                                     return BCM_E_EXISTS;
                                }
                                if (vp1 >= num_vp) {
                                     return (BCM_E_BADID);
                                }
                                BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp1, _bcmVpTypeVxlan));

                             } else {
                                  active_vp = vp1;
                                  vp_valid_flag = 3;
                                   sal_memset(&svp1, 0, sizeof(source_vp_entry_t));
                             }

                             (void) _bcm_td2_vxlan_eline_vp_map_set(unit, vfi_index, vp1, vp2);
                             break;


              
         case 0x3: /* VP1 and VP2 are valid */
                              if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
                                   return BCM_E_FULL;
                              }
                             break;
       }

       if (active_vp == -1) {
           return BCM_E_CONFIG;
       }

        /* Set VXLAN port ID */
        if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
            BCM_GPORT_VXLAN_PORT_ID_SET(vxlan_port->vxlan_port_id, active_vp);
        }
       /* ======== Configure Next-Hop Properties ========== */
       customer_drop = (vxlan_port->flags & BCM_VXLAN_PORT_DROP) ? 1 : 0;
       rv = _bcm_td2_vxlan_port_nh_add(unit, vxlan_port, active_vp, vpn, customer_drop);
       if (rv < 0) {
            if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
                (void) _bcm_vp_free(unit, _bcmVfiTypeVxlan, 1, active_vp);
            }
            return rv;
       }

       /* ======== Configure Service-Tag Properties =========== */
       if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_TAGGED) {
            rv = _bcm_fb2_outer_tpid_lkup(unit, vxlan_port->egress_service_tpid,
                                           &tpid_index);
            if (rv < 0) {
                goto vxlan_cleanup;
            }
            tpid_enable = (1 << tpid_index);
       }

       /* ======== Configure SVP Property ========== */
       if (active_vp == vp1) {
           rv  = _bcm_td2_vxlan_eline_vp_configure (unit, vfi_index, active_vp, &svp1, 
                                                    tpid_enable, vxlan_port);
       } else if (active_vp == vp2) {
           rv  = _bcm_td2_vxlan_eline_vp_configure (unit, vfi_index, active_vp, &svp2, 
                                                    tpid_enable, vxlan_port);
       }
       if (rv < 0) {
            goto vxlan_cleanup;
       }

        rv = _bcm_td2_vxlan_match_add(unit, vxlan_port, active_vp, vpn);
        if (rv < 0) {
            goto vxlan_cleanup;
        }

        /* Increment new-port's VP count */
        rv = _bcm_td2_vxlan_port_cnt_update(unit, vxlan_port->vxlan_port_id, active_vp, TRUE);
        if (rv < 0) {
            goto vxlan_cleanup;
        }

        /* Set VXLAN port ID */
        if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
            BCM_GPORT_VXLAN_PORT_ID_SET(vxlan_port->vxlan_port_id, active_vp);
        }

vxlan_cleanup:
        if (rv < 0) {
            if (tpid_enable) {
                (void) _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
            }
            /* Free the VP's */
            if (vp_valid_flag) {
                 (void) _bcm_vp_free(unit, _bcmVpTypeVxlan, 1, vp1);
                 (void) _bcm_vp_free(unit, _bcmVpTypeVxlan, 1, vp2);
            }
        }
        return rv;
}

/*
 * Function:
 *     _bcm_td2_vxlan_elan_port_add
 * Purpose:
 *      Add VXLAN ELAN port to a VPN
 * Parameters:
 *   unit - (IN) Device Number
 *   vpn - (IN) VPN instance ID
 *   vxlan_port - (IN/OUT) vxlan port information (OUT : vxlan_port_id)
 * Returns:
 *     BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_elan_port_add(int unit, bcm_vpn_t vpn, bcm_vxlan_port_t *vxlan_port)
{
    int vp, num_vp, vfi=0;
    source_vp_entry_t svp;
    source_vp_2_entry_t svp_2_entry;
    int tpid_enable = 0, tpid_index=-1;
    int drop=0, rv = BCM_E_PARAM;
    int cml_default_enable=0, cml_default_new=0, cml_default_move=0;

    if (vpn != BCM_VXLAN_VPN_INVALID) {
        _BCM_VXLAN_VPN_GET(vfi, _BCM_VXLAN_VPN_TYPE_ELAN,  vpn);
        if (!_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeVxlan)) {
            return BCM_E_NOT_FOUND;
        }
    } else {
        vfi = _BCM_VXLAN_VFI_INVALID;
    }
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

   /* ======== Allocate/Get Virtual_Port =============== */
    if (vxlan_port->flags & BCM_VXLAN_PORT_REPLACE) {
        if (BCM_GPORT_IS_NIV_PORT(vxlan_port->vxlan_port_id) ||
            BCM_GPORT_IS_EXTENDER_PORT(vxlan_port->vxlan_port_id)) {
            if (BCM_GPORT_IS_NIV_PORT(vxlan_port->vxlan_port_id)) {
               vp = BCM_GPORT_NIV_PORT_ID_GET((vxlan_port->vxlan_port_id));
            } else if (BCM_GPORT_IS_EXTENDER_PORT(vxlan_port->vxlan_port_id)) {
               vp = BCM_GPORT_EXTENDER_PORT_ID_GET((vxlan_port->vxlan_port_id));
            }
            rv = _bcm_td2_vxlan_access_niv_pe_set (unit, vp, vfi);
            if (BCM_SUCCESS(rv)) {
               /* Set VXLAN port ID */
               BCM_GPORT_VXLAN_PORT_ID_SET(vxlan_port->vxlan_port_id, vp);
               rv = _bcm_vp_used_set(unit, vp, _bcmVpTypeVxlan);
            }
            return rv;
        } 

        vp = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port->vxlan_port_id);
        if (vp == -1) {
            return BCM_E_PARAM;
        }

        if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
            return BCM_E_NOT_FOUND;
        }
        rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
        if (rv < 0) {
            return rv;
        }

        /* Decrement old-port's nexthop count */
        rv = _bcm_td2_vxlan_port_nh_cnt_dec(unit, vp);
        if (rv < 0) {
            return rv;
        }

        /* Decrement old-port's VP count */
        rv = _bcm_td2_vxlan_port_cnt_update(unit, vxlan_port->vxlan_port_id, vp, FALSE);
        if (rv < 0) {
            return rv;
        }

    } else if (vxlan_port->flags & BCM_VXLAN_PORT_WITH_ID ) {
        if (BCM_GPORT_IS_NIV_PORT(vxlan_port->vxlan_port_id) ||
            BCM_GPORT_IS_EXTENDER_PORT(vxlan_port->vxlan_port_id)) {
            if (BCM_GPORT_IS_NIV_PORT(vxlan_port->vxlan_port_id)) {
               vp = BCM_GPORT_NIV_PORT_ID_GET((vxlan_port->vxlan_port_id));
            } else if (BCM_GPORT_IS_EXTENDER_PORT(vxlan_port->vxlan_port_id)) {
               vp = BCM_GPORT_EXTENDER_PORT_ID_GET((vxlan_port->vxlan_port_id));
            }
            rv = _bcm_td2_vxlan_access_niv_pe_set (unit, vp, vfi);
            if (BCM_SUCCESS(rv)) {
               /* Set VXLAN port ID */
               BCM_GPORT_VXLAN_PORT_ID_SET(vxlan_port->vxlan_port_id, vp);
               rv = _bcm_vp_used_set(unit, vp, _bcmVpTypeVxlan);
            }
            return rv;
        } 
 
       if (!BCM_GPORT_IS_VXLAN_PORT(vxlan_port->vxlan_port_id)) {
            return (BCM_E_BADID);
        }

        vp = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port->vxlan_port_id);
        if (vp == -1) {
            return BCM_E_PARAM;
        }
        /* Vp index zero is reserved in function _bcm_virtual_init because of 
         * HW restriction. 
         */
        if (vp >= num_vp || vp < 1) {
            return BCM_E_BADID;
        }
        if (_bcm_vp_used_get(unit, vp, _bcmVpTypeAny)) {
            return BCM_E_EXISTS;
        }
        BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp, _bcmVpTypeVxlan));
        sal_memset(&svp, 0, sizeof(source_vp_entry_t));
    } else {
        /* allocate a new VP index */
        rv = _bcm_vp_alloc(unit, 0, (num_vp - 1), 1, SOURCE_VPm, _bcmVpTypeVxlan, &vp);
        if (rv < 0) {
           return rv;
        }
        sal_memset(&svp, 0, sizeof(source_vp_entry_t));
        BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp, _bcmVpTypeVxlan));
    }

    /* ======== Configure Next-Hop Properties ========== */
    drop = (vxlan_port->flags & BCM_VXLAN_PORT_DROP) ? 1 : 0;
    rv = _bcm_td2_vxlan_port_nh_add(unit, vxlan_port, vp, vpn, drop);
    if (rv < 0) {
        if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
            (void) _bcm_vp_free(unit, _bcmVfiTypeVxlan, 1, vp);
        }
        return rv;
    }

    /* ======== Configure Service-Tag Properties =========== */
    if (vxlan_port->flags & BCM_VXLAN_PORT_SERVICE_TAGGED) {
        rv = _bcm_fb2_outer_tpid_lkup(unit, vxlan_port->egress_service_tpid,
                                           &tpid_index);
        if (rv < 0) {
            goto vxlan_cleanup;
        }
        tpid_enable = (1 << tpid_index);

        soc_SOURCE_VPm_field32_set(unit, &svp, SD_TAG_MODEf, 1);
        soc_SOURCE_VPm_field32_set(unit, &svp, TPID_ENABLEf, tpid_enable);
    } else {
        soc_SOURCE_VPm_field32_set(unit, &svp, SD_TAG_MODEf, 0);
    }

    /* ======== Configure SVP/DVP Properties ========== */
    soc_SOURCE_VPm_field32_set(unit, &svp, CLASS_IDf, 
                               vxlan_port->if_class);
        if (vpn == BCM_VXLAN_VPN_INVALID) {
            soc_SOURCE_VPm_field32_set(unit, &svp, 
                                       ENTRY_TYPEf, _BCM_VXLAN_SOURCE_VP_TYPE_INVALID);
        } else {
            /* Initialize VP parameters */
            soc_SOURCE_VPm_field32_set(unit, &svp, 
                                       ENTRY_TYPEf, _BCM_VXLAN_SOURCE_VP_TYPE_VFI);
        }
      
        if (vxlan_port->flags & BCM_VXLAN_PORT_NETWORK) {
               soc_SOURCE_VPm_field32_set(unit, &svp, NETWORK_PORTf, 1);
               soc_SOURCE_VPm_field32_set(unit, &svp, TPID_SOURCEf, _BCM_VXLAN_TPID_SVP_BASED);
               soc_SOURCE_VPm_field32_set(unit, &svp, VFIf, vfi);
        } else {
               soc_SOURCE_VPm_field32_set(unit, &svp, VFIf, vfi);
               soc_SOURCE_VPm_field32_set(unit, &svp, NETWORK_PORTf, 0);
               soc_SOURCE_VPm_field32_set(unit, &svp, TPID_SOURCEf, _BCM_VXLAN_TPID_SGLP_BASED);
               /*  Configure IPARS Parser to signal to IMPLS Parser - Applicable only for HG2 PPD2 packets 
                    to reconstruct TPID state based on HG header information - Only for VXLAN Access ports */
               sal_memset(&svp_2_entry, 0, sizeof(source_vp_2_entry_t));
               soc_SOURCE_VP_2m_field32_set(unit, &svp_2_entry, PARSE_USING_SGLP_TPIDf, 1);
               rv = WRITE_SOURCE_VP_2m(unit, MEM_BLOCK_ALL, vp, &svp_2_entry);
               if (rv < 0) {
                   goto vxlan_cleanup;
               }
        }
        /* Keep CML in the replace operation. It may be set by bcm_td2_vxlan_port_learn_set before */
        if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
            rv = _bcm_vp_default_cml_mode_get (unit, 
                               &cml_default_enable, &cml_default_new, 
                               &cml_default_move);
            if (rv < 0) {
                goto vxlan_cleanup;
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
        }
        if (soc_mem_field_valid(unit, SOURCE_VPm, DISABLE_VLAN_CHECKSf)) {
              soc_SOURCE_VPm_field32_set(unit, &svp, DISABLE_VLAN_CHECKSf, 1);
        }    

    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
    if (rv < 0) {
        goto vxlan_cleanup;
    }


    if (rv == BCM_E_NONE) {
        /* Set VXLAN port ID */
        BCM_GPORT_VXLAN_PORT_ID_SET(vxlan_port->vxlan_port_id, vp);
    }

    /* Increment new-port's VP count */
    rv = _bcm_td2_vxlan_port_cnt_update(unit, vxlan_port->vxlan_port_id, vp, TRUE);
    if (rv < 0) {
        goto vxlan_cleanup;
    }

    /* ======== Configure match to VP Properties ========== */
    rv = _bcm_td2_vxlan_match_add(unit, vxlan_port, vp, vpn);
    if (rv < 0) {
        goto vxlan_cleanup;
    }

  vxlan_cleanup:
    if (rv < 0) {
        if (tpid_enable) {
            (void) _bcm_fb2_outer_tpid_entry_delete(unit, tpid_index);
        }
        if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
            (void) _bcm_vp_free(unit, _bcmVpTypeVxlan, 1, vp);
            _bcm_td2_vxlan_port_nh_delete(unit, vpn, vp);
        }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_eline_port_delete
 * Purpose:
 *      Delete VXLAN ELINE port from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      vxlan_port_id - (IN) vxlan port information
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_td2_vxlan_eline_port_delete(int unit, bcm_vpn_t vpn, int active_vp)
{
    source_vp_entry_t svp;
    source_vp_2_entry_t svp_2_entry;
    int network_port_flag=0;
    int vp1=0, vp2=0, vfi_index= -1;
    int rv = BCM_E_UNAVAIL;
    bcm_gport_t  vxlan_port_id;

    if ( vpn != BCM_VXLAN_VPN_INVALID) {
         _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VXLAN_VPN_TYPE_ELINE,  vpn);
         if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeVxlan)) {
            return BCM_E_NOT_FOUND;
         }
    } else {
         vfi_index = _BCM_VXLAN_VFI_INVALID;
    }

    if (!_bcm_vp_used_get(unit, active_vp, _bcmVpTypeVxlan)) {
        return BCM_E_NOT_FOUND;
    }

    /* Set VXLAN port ID */
    BCM_GPORT_VXLAN_PORT_ID_SET(vxlan_port_id, active_vp);

    /* Delete the next-hop info */
    rv = _bcm_td2_vxlan_port_nh_delete(unit, vpn, active_vp);
    if ( rv < 0 ) {
        if (rv != BCM_E_NOT_FOUND) {
            return rv;
        } else {
                rv = 0;
        }
    }

     /* ---- Read in current table values for VP1 and VP2 ----- */
     (void) _bcm_td2_vxlan_eline_vp_map_get (unit, vfi_index,  &vp1,  &vp2);

    rv = _bcm_td2_vxlan_match_delete(unit, active_vp);
    if ( rv < 0 ) {
        if (rv != BCM_E_NOT_FOUND) {
             return rv;
        } else {
             rv = BCM_E_NONE;
        }
    }

    /* If the other port is valid, point it to itself */
    if (active_vp == vp1) {
        if (BCM_SUCCESS(rv)) {
            rv = _bcm_td2_vxlan_eline_vp_map_clear (unit, vfi_index, active_vp, 0);
        }
    } else if (active_vp == vp2) {
        if (BCM_SUCCESS(rv)) {
            rv = _bcm_td2_vxlan_eline_vp_map_clear (unit, vfi_index, 0, active_vp);
        }
    }

    if (rv >= 0) {

        /* Check for Network-Port */
        BCM_IF_ERROR_RETURN (READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, active_vp, &svp));
        network_port_flag = soc_SOURCE_VPm_field32_get(unit, &svp, NETWORK_PORTf);
        if (!network_port_flag) {
            sal_memset(&svp_2_entry, 0, sizeof(source_vp_2_entry_t));
            BCM_IF_ERROR_RETURN
                (WRITE_SOURCE_VP_2m(unit, MEM_BLOCK_ALL, active_vp, &svp_2_entry));
        }

        /* Invalidate the VP being deleted */
        sal_memset(&svp, 0, sizeof(source_vp_entry_t));
        rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, active_vp, &svp);
        if (BCM_SUCCESS(rv)) {
             rv = _bcm_td2_vxlan_egress_dvp_reset(unit, active_vp);
             if (rv < 0) {
                 return rv;
             }
             rv = _bcm_td2_vxlan_ingress_dvp_reset(unit, active_vp);
        }
    }

    if (rv < 0) {
        return rv;
    }

    /* Decrement port's VP count */
    rv = _bcm_td2_vxlan_port_cnt_update(unit, vxlan_port_id, active_vp, FALSE);
    if (rv < 0) {
        return rv;
    }

    /* Free the VP */
    (void) _bcm_vp_free(unit, _bcmVpTypeVxlan, 1, active_vp);
    return rv;

}

/*
 * Function:
 *      _bcm_td2_vxlan_elan_port_delete
 * Purpose:
 *      Delete VXLAN ELAN port from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      vxlan_port_id - (IN) vxlan port information
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_td2_vxlan_elan_port_delete(int unit, bcm_vpn_t vpn, int vp)
{
    source_vp_entry_t svp;
    source_vp_2_entry_t svp_2_entry;
    int network_port_flag=0;
    int rv = BCM_E_UNAVAIL;
    int vfi_index= -1;
    bcm_gport_t vxlan_port_id;
    bcm_vxlan_port_t  vxlan_port;

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
        return BCM_E_NOT_FOUND;
    }

    /* Check for Network-Port */
    BCM_IF_ERROR_RETURN (READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp));
    network_port_flag = soc_SOURCE_VPm_field32_get(unit, &svp, NETWORK_PORTf);

    if ( vpn != BCM_VXLAN_VPN_INVALID) {
         if (!network_port_flag) { /* Check VPN only for Access VP */
             _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VXLAN_VPN_TYPE_ELAN,  vpn);
             if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeVxlan)) {
                return BCM_E_NOT_FOUND;
             }
             if (_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv) ||
                  _bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                   (void) _bcm_vp_free(unit, _bcmVpTypeVxlan, 1, vp);
                   rv = _bcm_td2_vxlan_access_niv_pe_reset (unit, vp);
                   return rv;
             }
         }
    }

    /* Set VXLAN port ID */
    BCM_GPORT_VXLAN_PORT_ID_SET(vxlan_port_id, vp);
    bcm_vxlan_port_t_init(&vxlan_port);
    BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_port_get(unit, vpn, vp, &vxlan_port));

    /* Delete the next-hop info */
    rv = _bcm_td2_vxlan_port_nh_delete(unit, vpn, vp);

    if ( rv < 0 ) {
        if (rv != BCM_E_NOT_FOUND) {
            return rv;
        } else {
                rv = 0;
        }
    }

    rv = _bcm_td2_vxlan_match_delete(unit, vp);
    if ( rv < 0 ) {
        if (rv != BCM_E_NOT_FOUND) {
            return rv;
        }
    }

    /* Clear the SVP table entries */
    sal_memset(&svp, 0, sizeof(source_vp_entry_t));
    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
    if (rv < 0) {
        return rv;
    }

    if (!network_port_flag) {
        sal_memset(&svp_2_entry, 0, sizeof(source_vp_2_entry_t));
        BCM_IF_ERROR_RETURN
            (WRITE_SOURCE_VP_2m(unit, MEM_BLOCK_ALL, vp, &svp_2_entry));
    }

    /* Clear the DVP table entries */
    rv = _bcm_td2_vxlan_egress_dvp_reset(unit, vp);
    if (rv < 0) {
        return rv;
    }

    rv = _bcm_td2_vxlan_ingress_dvp_reset(unit, vp);
    if (rv < 0) {
        return rv;
    }

    /* Decrement port's VP count */
    rv = _bcm_td2_vxlan_port_cnt_update(unit, vxlan_port_id, vp, FALSE);
    if (rv < 0) {
        return rv;
    }

    /* Free the VP */
    (void) _bcm_vp_free(unit, _bcmVpTypeVxlan, 1, vp);
    return rv;
}


/*
 * Function:
 *      _bcm_td2_vxlan_port_get
 * Purpose:
 *      Get VXLAN port information from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      vxlan_port_id - (IN) vxlan port information
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_td2_vxlan_port_get(int unit, bcm_vpn_t vpn, int vp, bcm_vxlan_port_t  *vxlan_port)
{
    int i, tpid_enable = 0, rv = BCM_E_NONE;
    source_vp_entry_t svp;
    ing_dvp_table_entry_t dvp;
    int split_horizon_port_flag=0;
    int egress_tunnel_flag=0;

    /* Initialize the structure */
    bcm_vxlan_port_t_init(vxlan_port);
    BCM_GPORT_VXLAN_PORT_ID_SET(vxlan_port->vxlan_port_id, vp);

    /* Check for Network-Port */
    BCM_IF_ERROR_RETURN (READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp));
    split_horizon_port_flag = soc_SOURCE_VPm_field32_get(unit, &svp, NETWORK_PORTf);

    if ( vpn != BCM_VXLAN_VPN_INVALID) {
         if (!split_horizon_port_flag) { /* Check VPN only for Access VP */
             if (_bcm_vp_used_get(unit, vp, _bcmVpTypeNiv) ||
                  _bcm_vp_used_get(unit, vp, _bcmVpTypeExtender)) {
                   return BCM_E_NONE;
             }
         }
    }

    /* Get the match parameters */
    rv = _bcm_td2_vxlan_match_get(unit, vxlan_port, vp);
    BCM_IF_ERROR_RETURN(rv);

    /* Get the next-hop parameters */
    rv = _bcm_td2_vxlan_port_nh_get(unit, vpn, vp, vxlan_port);
    BCM_IF_ERROR_RETURN(rv);

    /* Get Tunnel index */
    rv = _bcm_td2_vxlan_egress_dvp_get(unit, vp, vxlan_port);
    BCM_IF_ERROR_RETURN(rv);

    /* Fill in SVP parameters */
    vxlan_port->if_class = soc_SOURCE_VPm_field32_get(unit, &svp, CLASS_IDf);
    if (split_horizon_port_flag) {
        vxlan_port->flags |= BCM_VXLAN_PORT_NETWORK;
    }

    if (soc_SOURCE_VPm_field32_get(unit, &svp, SD_TAG_MODEf)) {
        tpid_enable = soc_SOURCE_VPm_field32_get(unit, &svp, TPID_ENABLEf);
        if (tpid_enable) {
            vxlan_port->flags |= BCM_VXLAN_PORT_SERVICE_TAGGED;
            for (i = 0; i < 4; i++) {
                if (tpid_enable & (1 << i)) {
                    _bcm_fb2_outer_tpid_entry_get(unit, &vxlan_port->egress_service_tpid, i);
                }
            }
        }
    }

    /* Check for Egress-Tunnel */
    BCM_IF_ERROR_RETURN (READ_ING_DVP_TABLEm(unit, MEM_BLOCK_ANY, vp, &dvp));
    egress_tunnel_flag = soc_ING_DVP_TABLEm_field32_get(unit, &dvp, NETWORK_PORTf);
    if (egress_tunnel_flag) {
        vxlan_port->flags |= BCM_VXLAN_PORT_EGRESS_TUNNEL;
    }
    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_port_add
 * Purpose:
 *      Add VXLAN port to a VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 *      vxlan_port - (IN/OUT) vxlan_port information (OUT : vxlan_port_id)
 * Returns:
 *      BCM_E_XXX
 */

STATIC int
_bcm_td2_vxlan_default_port_add(int unit, bcm_vxlan_port_t  *vxlan_port)
{
    source_vp_entry_t svp;
    int rv = BCM_E_PARAM, vp=0, num_vp=0;
    int cml_default_enable=0, cml_default_new=0, cml_default_move=0;

    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

   /* ======== Allocate/Get Virtual_Port =============== */
    if (vxlan_port->flags & BCM_VXLAN_PORT_REPLACE) {
        vp = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port->vxlan_port_id);
        if (vp == -1) {
            return BCM_E_PARAM;
        }

        if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
            return BCM_E_NOT_FOUND;
        }
        rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
        if (rv < 0) {
            return rv;
        }

        /* Decrement old-port's nexthop count */
        rv = _bcm_td2_vxlan_port_nh_cnt_dec(unit, vp);
        if (rv < 0) {
            return rv;
        }

        /* Decrement old-port's VP count */
        rv = _bcm_td2_vxlan_port_cnt_update(unit, vxlan_port->vxlan_port_id, vp, FALSE);
        if (rv < 0) {
            return rv;
        }

    } else if (vxlan_port->flags & BCM_VXLAN_PORT_WITH_ID ) {
       if (!BCM_GPORT_IS_VXLAN_PORT(vxlan_port->vxlan_port_id)) {
            return (BCM_E_BADID);
        }

        vp = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port->vxlan_port_id);
        if (vp == -1) {
            return BCM_E_PARAM;
        }
        /* Vp index zero is reserved in function _bcm_virtual_init because of 
         * HW restriction. 
         */
        if (vp >= num_vp || vp < 1) {
            return BCM_E_BADID;
        } 
        if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) { 
            return BCM_E_EXISTS;
        }
        BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp, _bcmVpTypeVxlan));
        sal_memset(&svp, 0, sizeof(source_vp_entry_t));
    } else {
       rv = _bcm_vp_alloc(unit, 0, (num_vp - 1), 1, SOURCE_VPm, _bcmVpTypeVxlan, &vp);
       if (rv < 0) {
           return rv;
       }
        sal_memset(&svp, 0, sizeof(source_vp_entry_t));
        BCM_IF_ERROR_RETURN(_bcm_vp_used_set(unit, vp, _bcmVpTypeVxlan));
    }

    /* ======== Configure SVP/DVP Properties ========== */
    soc_SOURCE_VPm_field32_set(unit, &svp, CLASS_IDf, 
                               vxlan_port->if_class);
    soc_SOURCE_VPm_field32_set(unit, &svp, NETWORK_PORTf,
                               (vxlan_port->flags & BCM_VXLAN_PORT_NETWORK) ? 1 : 0);

    soc_SOURCE_VPm_field32_set(unit, &svp, 
                                      ENTRY_TYPEf, _BCM_VXLAN_SOURCE_VP_TYPE_VFI);

    /* Keep CML in the replace operation. It may be set by bcm_td2_vxlan_port_learn_set before */
    if (!(vxlan_port->flags & BCM_VXLAN_PORT_REPLACE)) {
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
    }
    if (soc_mem_field_valid(unit, SOURCE_VPm, DISABLE_VLAN_CHECKSf)) {
        soc_SOURCE_VPm_field32_set(unit, &svp, DISABLE_VLAN_CHECKSf, 1);
    }

    /* Configure VXLAN_DEFAULT_NETWORK_SVPr.SVP */
    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
    BCM_IF_ERROR_RETURN(rv);

    BCM_GPORT_VXLAN_PORT_ID_SET(vxlan_port->vxlan_port_id, vp);

    /* Increment new-port's VP count */
    rv = _bcm_td2_vxlan_port_cnt_update(unit, vxlan_port->vxlan_port_id, vp, TRUE);
    BCM_IF_ERROR_RETURN(rv);

    return (soc_reg_field32_modify(unit, VXLAN_DEFAULT_NETWORK_SVPr, 
             REG_PORT_ANY, SVPf, vp));

}

/*
 * Function:
 *      _bcm_td2_vxlan_default_port_delete
 * Purpose:
 *      Delete VXLAN Default port from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      vxlan_port_id - (IN) vxlan port information
 * Returns:
 *      BCM_E_XXX
 */

int
_bcm_td2_vxlan_default_port_delete(int unit, bcm_vpn_t vpn, int vp)
{
    source_vp_entry_t svp;
    int rv = BCM_E_UNAVAIL;
    int vfi_index= -1;
    bcm_gport_t vxlan_port_id;

    if ( vpn != BCM_VXLAN_VPN_INVALID) {
         _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VXLAN_VPN_TYPE_ELAN,  vpn);
         if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeVxlan)) {
            return BCM_E_NOT_FOUND;
         }
    }

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
        return BCM_E_NOT_FOUND;
    }

    /* Set VXLAN port ID */
    BCM_GPORT_VXLAN_PORT_ID_SET(vxlan_port_id, vp);

    /* Clear the SVP and DVP table entries */
    sal_memset(&svp, 0, sizeof(source_vp_entry_t));
    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
    if (rv < 0) {
        return rv;
    }

    /* Decrement port's VP count */
    rv = _bcm_td2_vxlan_port_cnt_update(unit, vxlan_port_id, vp, FALSE);
    if (rv < 0) {
        return rv;
    }

    /* Free the VP */
    (void) _bcm_vp_free(unit, _bcmVpTypeVxlan, 1, vp);
    return rv;

}

/*
 * Function:
 *      bcm_td2_vxlan_port_add
 * Purpose:
 *      Add VXLAN port to a VPN
 * Parameters:
 *      unit    - (IN) Device Number
 *      vpn     - (IN) VPN instance ID
 *      vxlan_port - (IN/OUT) vxlan_port information (OUT : vxlan_port_id)
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_td2_vxlan_port_add(int unit, bcm_vpn_t vpn, bcm_vxlan_port_t  *vxlan_port)
{
    int mode=0, rv = BCM_E_PARAM; 
    uint8 isEline=0xFF;
    int vfi_index=0;

    BCM_IF_ERROR_RETURN(bcm_xgs3_l3_egress_mode_get(unit, &mode));
    if (!mode) {
        LOG_INFO(BSL_LS_BCM_L3,
                 (BSL_META_U(unit,
                             "L3 egress mode must be set first\n")));
        return BCM_E_DISABLED;
    }

    if (vxlan_port->flags & BCM_VXLAN_PORT_NETWORK) {         
         _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VXLAN_VPN_TYPE_ELINE,  vpn);
        if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeVxlan)) {
            isEline = 0x0; /* ELAN Dont care VPN Case */
        }
    }

    if ((vpn != BCM_VXLAN_VPN_INVALID) && (isEline != 0x0)) {
         BCM_IF_ERROR_RETURN 
              (_bcm_td2_vxlan_vpn_is_eline(unit, vpn, &isEline));
    }
    
    if (vxlan_port->flags & BCM_VXLAN_PORT_DEFAULT) {
                BCM_IF_ERROR_RETURN(
                     _bcm_td2_vxlan_default_port_add(unit, vxlan_port));
    }

    if (isEline == 0x1 ) {
        rv = _bcm_td2_vxlan_eline_port_add(unit, vpn, vxlan_port);
    } else if (isEline == 0x0 ) {
        rv = _bcm_td2_vxlan_elan_port_add(unit, vpn, vxlan_port);
    }

    return rv;
}


/*
 * Function:
 *      bcm_td2_vxlan_port_delete
 * Purpose:
 *      Delete VXLAN port from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      vxlan_port_id - (IN) vxlan port information
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_td2_vxlan_port_delete(int unit, bcm_vpn_t vpn, bcm_gport_t vxlan_port_id)
{
    int vp=0;
    int rv = BCM_E_UNAVAIL;
    uint32 reg_val=0;
    uint8 isEline=0;
    uint32 stat_counter_id;
    int num_ctr = 0; 
    
    /* Check for Valid Vpn */
    BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_vpn_is_valid(unit, vpn));

    vp = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port_id);
    if (vp == -1) {
        return BCM_E_PARAM;
    }

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
        return BCM_E_NOT_FOUND;
    }

    /* Check counters before delete port */
    if (bcm_esw_vxlan_stat_id_get(unit, vxlan_port_id, BCM_VXLAN_VPN_INVALID,
                   bcmVxlanOutPackets, &stat_counter_id) == BCM_E_NONE) { 
        num_ctr++;
    } 
    if (bcm_esw_vxlan_stat_id_get(unit, vxlan_port_id, BCM_VXLAN_VPN_INVALID,
                   bcmVxlanInPackets, &stat_counter_id) == BCM_E_NONE) {
        num_ctr++;
    }

    if (num_ctr != 0) {
        BCM_IF_ERROR_RETURN(
           bcm_esw_vxlan_stat_detach(unit, vxlan_port_id, BCM_VXLAN_VPN_INVALID));  
    }

    /* Check for VXLAN default port */
    BCM_IF_ERROR_RETURN(READ_VXLAN_DEFAULT_NETWORK_SVPr(unit, &reg_val));
    if (vp == soc_reg_field_get(unit, VXLAN_DEFAULT_NETWORK_SVPr,
                                         reg_val, SVPf)) {
        rv = _bcm_td2_vxlan_default_port_delete(unit, vpn, vp);
        return rv;
    }
 
    BCM_IF_ERROR_RETURN 
         (_bcm_td2_vxlan_vp_is_eline(unit, vp, &isEline));

    if (isEline == 0x1 ) {
       rv = _bcm_td2_vxlan_eline_port_delete(unit, vpn, vp);
    } else if (isEline == 0x0 ) {
       rv = _bcm_td2_vxlan_elan_port_delete(unit, vpn, vp);
    }
    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_port_delete_all
 * Purpose:
 *      Delete all VXLAN ports from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_vxlan_port_delete_all(int unit, bcm_vpn_t vpn)
{
    int rv = BCM_E_NONE;
    int vfi_index=0;
    int vp1 = 0, vp2 = 0;
    uint8 isEline=0xFF;
    bcm_gport_t vxlan_port_id;
    uint32 reg_val=0;
    uint32 vp=0;


    BCM_IF_ERROR_RETURN 
         (_bcm_td2_vxlan_vpn_is_eline(unit, vpn, &isEline));

    BCM_IF_ERROR_RETURN(READ_VXLAN_DEFAULT_NETWORK_SVPr(unit, &reg_val));

    if (isEline == 0x1 ) {
         if ( vpn != BCM_VXLAN_VPN_INVALID) {
              _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VXLAN_VPN_TYPE_ELINE,  vpn);
              if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeVxlan)) {
                   return BCM_E_NOT_FOUND;
              }
         } else {
              vfi_index = _BCM_VXLAN_VFI_INVALID;
         }

         /* ---- Read in current table values for VP1 and VP2 ----- */
         (void) _bcm_td2_vxlan_eline_vp_map_get (unit, vfi_index,  &vp1,  &vp2);
         if (vp1 != 0) {
              rv = _bcm_td2_vxlan_eline_port_delete(unit, vpn, vp1);
              BCM_IF_ERROR_RETURN(rv);
         }
         if (vp2 != 0) {
              rv = _bcm_td2_vxlan_eline_port_delete(unit, vpn, vp2);
              BCM_IF_ERROR_RETURN(rv);
         }
    } else if (isEline == 0x0 ) {
        uint32 vfi, num_vp;
        source_vp_entry_t svp;

        _BCM_VXLAN_VPN_GET(vfi, _BCM_VXLAN_VPN_TYPE_ELAN,  vpn);
        if (!_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeVxlan)) {
            rv =  BCM_E_NOT_FOUND;
                return rv;
        }
        num_vp = soc_mem_index_count(unit, SOURCE_VPm);
        for (vp = 0; vp < num_vp; vp++) {
            /* Check for the validity of the VP */
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
                continue;
            }
            rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
            if (rv < 0) {
                return rv;
            }
            if (vp == soc_reg_field_get(unit, VXLAN_DEFAULT_NETWORK_SVPr,
                                         reg_val, SVPf)) {
                 rv = _bcm_td2_vxlan_default_port_delete(unit, vpn, vp);
                 if (rv < 0) {
                    return rv;
                 }   
            }

            if ((soc_SOURCE_VPm_field32_get(unit, &svp, ENTRY_TYPEf) == 
                                                    _BCM_VXLAN_SOURCE_VP_TYPE_VFI) &&
                (vfi == soc_SOURCE_VPm_field32_get(unit, &svp, VFIf))) {
                   /* Set VXLAN port ID */
                   BCM_GPORT_VXLAN_PORT_ID_SET(vxlan_port_id, vp);
                   rv = bcm_td2_vxlan_port_delete(unit, vpn, vxlan_port_id);
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
 *      bcm_td2_vxlan_port_get
 * Purpose:
 *      Get an VXLAN port from a VPN
 * Parameters:
 *      unit       - (IN) Device Number
 *      vpn        - (IN) VPN instance ID
 *      vxlan_port  - (IN/OUT) VXLAN port information
 */
int
bcm_td2_vxlan_port_get(int unit, bcm_vpn_t vpn, bcm_vxlan_port_t *vxlan_port)
{
    int vp=0;
    int rv = BCM_E_NONE;

    /* Check for Valid Vpn */
    BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_vpn_is_valid(unit, vpn));

    vp = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port->vxlan_port_id);
    if (vp == -1) {
        return BCM_E_PARAM;
    }

    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
        return BCM_E_NOT_FOUND;
    }

    rv = _bcm_td2_vxlan_port_get(unit, vpn, vp, vxlan_port);
    return rv;
}

/*
 * Function:
 *      bcm_td2_vxlan_port_get_all
 * Purpose:
 *      Get all VXLAN ports from a VPN
 * Parameters:
 *      unit     - (IN) Device Number
 *      vpn      - (IN) VPN instance ID
 *      port_max   - (IN) Maximum number of interfaces in array
 *      port_array - (OUT) Array of VXLAN ports
 *      port_count - (OUT) Number of interfaces returned in array
 *
 */
int
bcm_td2_vxlan_port_get_all(int unit, bcm_vpn_t vpn, int port_max,
                         bcm_vxlan_port_t *port_array, int *port_count)
{
    int vp, rv = BCM_E_NONE;
    int vfi_index=-1;
    int vp1 = 0, vp2 = 0;
    uint8 isEline=0xFF;

    BCM_IF_ERROR_RETURN 
         (_bcm_td2_vxlan_vpn_is_eline(unit, vpn, &isEline));

    *port_count = 0;

    if (isEline == 0x1 ) {
         if ( vpn != BCM_VXLAN_VPN_INVALID) {
              _BCM_VXLAN_VPN_GET(vfi_index, _BCM_VXLAN_VPN_TYPE_ELAN,  vpn);
              if (!_bcm_vfi_used_get(unit, vfi_index, _bcmVfiTypeVxlan)) {
                   return BCM_E_NOT_FOUND;
              }
         } else {
              vfi_index = _BCM_VXLAN_VFI_INVALID;
         }

        /* ---- Read in current table values for VP1 and VP2 ----- */
        (void) _bcm_td2_vxlan_eline_vp_map_get (unit, vfi_index,  &vp1,  &vp2);
        vp = vp1;
        if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
            if (0 == port_max) {
                (*port_count)++;

            } else if (*port_count < port_max) {
                rv = _bcm_td2_vxlan_port_get(unit, vpn, vp, 
                                           &port_array[*port_count]);
                if (rv < 0) {
                    return rv;
                }
                (*port_count)++;
            }
        }

        vp = vp2;
        if (_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
            if (0 == port_max) {
                (*port_count)++;

            } else if (*port_count < port_max) {
                rv = _bcm_td2_vxlan_port_get(unit, vpn, vp, 
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

        _BCM_VXLAN_VPN_GET(vfi, _BCM_VXLAN_VPN_TYPE_ELAN,  vpn);
        if (!_bcm_vfi_used_get(unit, vfi, _bcmVfiTypeVxlan)) {
            rv = BCM_E_NOT_FOUND;
            return rv;
        }
        num_vp = soc_mem_index_count(unit, SOURCE_VPm);
        for (vp = 0; vp < num_vp; vp++) {
            /* Check for the validity of the VP */
            if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
                continue;
            }

            rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
            if (rv < 0) {
                return rv;
            }
            entry_type = soc_SOURCE_VPm_field32_get(unit, &svp, ENTRY_TYPEf);

            if (vfi == soc_SOURCE_VPm_field32_get(unit, &svp, VFIf) && 
                 entry_type == _BCM_VXLAN_SOURCE_VP_TYPE_VFI) {
                
                /* Check if number of ports is requested */
                if (0 == port_max) {
                    (*port_count)++;
                    continue;
                } else if (*port_count == port_max) {
                    break;
                }

               rv = _bcm_td2_vxlan_port_get(unit, vpn, vp,
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
 *      bcm_td2_vxlan_port_learn_set
 * Purpose:
 *      Set the CML bits for a vxlan port.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_td2_vxlan_port_learn_set(int unit, bcm_gport_t vxlan_port_id, 
                                  uint32 flags)
{
    int vp, cml = 0, rv = BCM_E_NONE, entry_type;
    source_vp_entry_t svp;

    rv = _bcm_vxlan_check_init(unit);

    if (rv != BCM_E_NONE){
        return rv;
    }

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

    /* Get the VP index from the gport */
    vp = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port_id);

    MEM_LOCK(unit, SOURCE_VPm);
    /* Be sure the entry is used and is set for VxLAN */
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
        MEM_UNLOCK(unit, SOURCE_VPm);
        return BCM_E_NOT_FOUND;
    }
    rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
    if (rv < 0) {
        MEM_UNLOCK(unit, SOURCE_VPm);
        return rv;
    }

    /* Ensure that the entry_type is 1 for VxLAN*/
    entry_type = soc_SOURCE_VPm_field32_get(unit, &svp, ENTRY_TYPEf);
    if (entry_type != 1){
        return BCM_E_NOT_FOUND;
    }

    soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_MOVEf, cml);
    soc_SOURCE_VPm_field32_set(unit, &svp, CML_FLAGS_NEWf, cml);
    rv = WRITE_SOURCE_VPm(unit, MEM_BLOCK_ALL, vp, &svp);
    MEM_UNLOCK(unit, SOURCE_VPm);
    return rv;
}

/*
 * Function:    
 *      bcm_td2_vxlan_port_learn_get
 * Purpose:
 *      Get the CML bits for a VxLAN port
 * Returns: 
 *      BCM_E_XXX
 */     
int     
bcm_td2_vxlan_port_learn_get(int unit, bcm_gport_t vxlan_port_id,
                                  uint32 *flags)
{
    int rv, vp, cml = 0, entry_type;
    source_vp_entry_t svp;
    
    rv = _bcm_vxlan_check_init(unit);

    if (rv != BCM_E_NONE){
        return rv;
    }

    /* Get the VP index from the gport */
    vp = BCM_GPORT_VXLAN_PORT_ID_GET(vxlan_port_id);
    if (vp == -1) {
       return BCM_E_PARAM;
    }

    /* Be sure the entry is used and is set for VxLAN */
    if (!_bcm_vp_used_get(unit, vp, _bcmVpTypeVxlan)) {
        return BCM_E_NOT_FOUND;
    }
    rv = READ_SOURCE_VPm(unit, MEM_BLOCK_ANY, vp, &svp);
    if (rv < 0) {
        return rv;
    }


    /* Ensure that the entry_type is 1 for VxLAN*/
    entry_type = soc_SOURCE_VPm_field32_get(unit, &svp, ENTRY_TYPEf);
    if (entry_type != 1){
        return BCM_E_NOT_FOUND;
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

/*
 * Function:
 *      bcm_td2_vxlan_trunk_member_add
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
bcm_td2_vxlan_trunk_member_add(int unit, bcm_trunk_t trunk_id, 
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

    if (egr.flags & BCM_L3_VXLAN_ONLY) {
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
 *      bcm_td2_vxlan_trunk_member_delete
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
bcm_td2_vxlan_trunk_member_delete(int unit, bcm_trunk_t trunk_id, 
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
            rv += _bcm_td2_vxlan_trunk_table_reset(unit, trunk_member_array[idx]);
        }
        if (BCM_FAILURE(rv)) {
            return rv;
        }
    } else {
        BCM_IF_ERROR_RETURN(bcm_xgs3_nh_get(unit, nh_index, &egr));

        if (egr.flags & BCM_L3_VXLAN_ONLY) {
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
 *     _bcm_td2_vxlan_sw_dump
 * Purpose:
 *     Displays VXLAN information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_td2_vxlan_sw_dump(int unit)
{
    int i, num_vp;
    _bcm_td2_vxlan_bookkeeping_t *vxlan_info = VXLAN_INFO(unit);

    LOG_CLI((BSL_META_U(unit,
                        "Tunnel Initiator Endpoints:\n")));
    for (i = 0; i < _BCM_MAX_NUM_VXLAN_TUNNEL; i++) {
        if (VXLAN_INFO(unit)->vxlan_tunnel_init[i].dip != 0 &&
            VXLAN_INFO(unit)->vxlan_tunnel_init[i].sip != 0 ) {
            LOG_CLI((BSL_META_U(unit,
                                "Tunnel idx:%d, sip:%x, dip:%x\n"), i,
                     vxlan_info->vxlan_tunnel_init[i].sip, 
                     vxlan_info->vxlan_tunnel_init[i].dip));
        }
    }

    LOG_CLI((BSL_META_U(unit,
                        "\nTunnel Terminator Endpoints:\n")));
    for (i = 0; i < _BCM_MAX_NUM_VXLAN_TUNNEL; i++) {
        if (VXLAN_INFO(unit)->vxlan_tunnel_term[i].dip != 0 &&
            VXLAN_INFO(unit)->vxlan_tunnel_term[i].sip != 0 ) {
            LOG_CLI((BSL_META_U(unit,
                                "Tunnel idx:%d, sip:%x, dip:%x\n"), i,
                     vxlan_info->vxlan_tunnel_term[i].sip, 
                     vxlan_info->vxlan_tunnel_term[i].dip));
        }
    }
    
    num_vp = soc_mem_index_count(unit, SOURCE_VPm);

    LOG_CLI((BSL_META_U(unit,
                        "\n  Match Info    : \n")));
    for (i = 0; i < num_vp; i++) {
        if ((vxlan_info->match_key[i].trunk_id == 0 || 
             vxlan_info->match_key[i].trunk_id == -1) && 
            (vxlan_info->match_key[i].modid == 0 || 
             vxlan_info->match_key[i].modid == -1) &&
            (vxlan_info->match_key[i].port == 0) &&
            (vxlan_info->match_key[i].flags == 0)) {
            continue;
        }
        LOG_CLI((BSL_META_U(unit,
                            "\n  VXLAN port vp = %d\n"), i));
        LOG_CLI((BSL_META_U(unit,
                            "Flags = %x\n"), vxlan_info->match_key[i].flags));
        LOG_CLI((BSL_META_U(unit,
                            "Index = %x\n"), vxlan_info->match_key[i].index));
        LOG_CLI((BSL_META_U(unit,
                            "TGID = %d\n"), vxlan_info->match_key[i].trunk_id));
        LOG_CLI((BSL_META_U(unit,
                            "Modid = %d\n"), vxlan_info->match_key[i].modid));
        LOG_CLI((BSL_META_U(unit,
                            "Port = %d\n"), vxlan_info->match_key[i].port));
        LOG_CLI((BSL_META_U(unit,
                            "Match VLAN = %d\n"), 
                 vxlan_info->match_key[i].match_vlan));
        LOG_CLI((BSL_META_U(unit,
                            "Match Inner VLAN = %d\n"), 
                 vxlan_info->match_key[i].match_inner_vlan));
        LOG_CLI((BSL_META_U(unit,
                            "Match tunnel Index = %x\n"), 
                 vxlan_info->match_key[i].match_tunnel_index));
    }
    
    return;
}
#endif

/*
 * Function:
 *      _bcm_td2_vxlan_dip_entry_idx_get
 * Purpose:
 *      Get TD2 vxlan_dip_entry index by VxLAN DIP.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      vxlan_dip - (IN)VxLAN DIP.
 *      index     - (OUT)vxlan_dip_entry index.
 * Returns:
 *    BCM_E_XXX
 */
int
_bcm_td2_vxlan_dip_entry_idx_get(int unit, 
                                 bcm_ip_t vxlan_dip,
                                 int *index)
{
    vlan_xlate_entry_t  vxlate_entry;
    
    sal_memset(&vxlate_entry, 0, sizeof(vlan_xlate_entry_t));

    soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, KEY_TYPEf, 
                                    _BCM_VXLAN_KEY_TYPE_LOOKUP_DIP); 
    soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry, VALIDf, 0x1);
    soc_VLAN_XLATEm_field32_set(unit, &vxlate_entry,  
                                    VXLAN_DIP__DIPf, vxlan_dip);

    return soc_mem_search(unit, VLAN_XLATEm, MEM_BLOCK_ANY, index,
                             &vxlate_entry, &vxlate_entry, 0);   
      
}


/*
 * Function:
 *      _bcm_esw_vxlan_dip_stat_get_table_info
 * Description:
 *      Provides relevant flex table information(table-name,index with
 *      direction)  for given ingress interface.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vxlan_dip     - (IN) vxlan dip
 *      num_of_tables    - (OUT) Number of flex counter tables
 *      table_info       - (OUT) Flex counter tables information
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

static 
bcm_error_t _bcm_td2_vxlan_dip_stat_get_table_info(
            int                        unit,
            bcm_ip_t                   vxlan_dip,
            uint32                     *num_of_tables,
            bcm_stat_flex_table_info_t *table_info)
{

    int index;

    if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return BCM_E_UNAVAIL;
    }

    /* Parameter validation checks */
    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    BCM_IF_ERROR_RETURN
        (_bcm_td2_vxlan_dip_entry_idx_get(unit, vxlan_dip, &index)); 
    
    if (vxlan_dip != 0) { 
        table_info[*num_of_tables].table = VLAN_XLATEm;
        table_info[*num_of_tables].index = index;
        table_info[*num_of_tables].direction = bcmStatFlexDirectionIngress;
        (*num_of_tables)++;
    }

    if (*num_of_tables == 0) {
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_vxlan_dip_stat_counter_get
 * Description:
 *      Get counter statistic values for specific vxlan dip
 *      if sync_mode is set, sync the sw accumulated count
 *      with hw count value first, else return sw count.  
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) hwcount is to be synced to sw count 
 *      vxlan_dip        - (IN) vxlan dip information
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
_bcm_td2_vxlan_dip_stat_counter_get(
            int unit,
            int sync_mode,
            bcm_ip_t vxlan_dip,
            bcm_vxlan_dip_stat_t stat,
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

    if ((stat == bcmVxlanDipInPackets) ||
        (stat == bcmVxlanDipInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         /* direction = bcmStatFlexDirectionEgress; */
         return BCM_E_PARAM;
    }
    if (stat == bcmVxlanDipInPackets) {
        byte_flag=0;
    } else {
        byte_flag=1; 
    }

    BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_dip_stat_get_table_info(
                   unit, vxlan_dip,&num_of_tables,&table_info[0]));

    for (table_count=0; table_count < num_of_tables; table_count++) {
      if (table_info[table_count].direction == direction) {
          for (index_count=0; index_count < num_entries ; index_count++) {
               /*ctr_offset_info.offset_index = counter_indexes[index_count];*/
            BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_get(
                         unit, sync_mode,
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
 *      _bcm_td2_vxlan_dip_stat_multi_get
 *
 * Description:
 *  Get Multiple vxlan dip counter value for specified IPMC group index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vxlan_dip     - (IN) vxlan dip
 *      nstat            - (IN) Number of elements in stat array
 *      stat_arr         - (IN) Collected statistics descriptors array
 *      value_arr        - (OUT) Collected counters values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
 bcm_error_t  _bcm_td2_vxlan_dip_stat_multi_get(
                            int                   unit, 
                            bcm_ip_t              vxlan_dip,
                            int                   nstat, 
                            bcm_vxlan_dip_stat_t  *stat_arr,
                            uint64                *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};
    int              sync_mode = 0; 

    /* Iterate of all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat; idx++) {
         BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_dip_stat_counter_get( 
                             unit, sync_mode, vxlan_dip, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
         if ((stat_arr[idx] == bcmVxlanDipInPackets)) {
             COMPILER_64_SET(value_arr[idx],
                             COMPILER_64_HI(counter_values.packets64),
                             COMPILER_64_LO(counter_values.packets64));
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
 *      _bcm_td2_vxlan_dip_stat_multi_get32
 *
 * Description:
 *  Get 32bit vxlan dip counter value for specified IPMC group index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vxlan_dip     - (IN) vxlan dip
 *      nstat            - (IN) Number of elements in stat array
 *      stat_arr         - (IN) Collected statistics descriptors array
 *      value_arr        - (OUT) Collected counters values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t  _bcm_td2_vxlan_dip_stat_multi_get32(
                            int                  unit, 
                            bcm_ip_t             vxlan_dip,
                            int                  nstat, 
                            bcm_vxlan_dip_stat_t *stat_arr,
                            uint32               *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};
    int              sync_mode = 0; 

    /* Iterate of all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat; idx++) {
         BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_dip_stat_counter_get( 
                             unit, sync_mode, vxlan_dip, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
         if ((stat_arr[idx] == bcmVxlanDipInPackets)) {
                value_arr[idx] = counter_values.packets;
         } else {
             value_arr[idx] = COMPILER_64_LO(counter_values.bytes);
         }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_dip_stat_counter_set
 * Description:
 *      Set counter statistic values for specific vxlan dip
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vxlan_dip     - (IN) vxlan dip
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
_bcm_td2_vxlan_dip_stat_counter_set(
            int unit,
            bcm_ip_t vxlan_dip,
            bcm_vxlan_dip_stat_t stat,
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
    
    if ((stat == bcmVxlanDipInPackets) ||
        (stat == bcmVxlanDipInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         /* direction = bcmStatFlexDirectionEgress; */
         return BCM_E_PARAM;
    }
    if (stat == bcmVxlanDipInPackets) {
        byte_flag=0;
    } else {
        byte_flag=1; 
    }

    BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_dip_stat_get_table_info(
                   unit, vxlan_dip, &num_of_tables, &table_info[0]));

    for (table_count=0; table_count < num_of_tables; table_count++) {
      if (table_info[table_count].direction == direction) {
          for (index_count=0; index_count < num_entries; index_count++) {
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
 *      _bcm_esw_vxlan_dip_stat_counter_get
 * Description:
 *      Get counter statistic values for specific vxlan dip
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) if 1 sync sw count value with hw count
 *                              before retieving count else get
 *                              sw accumulated count
 *      vxlan_dip      - (IN) vxlan dip
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
_bcm_esw_vxlan_dip_stat_counter_get(
                               int                unit,
                               int                sync_mode,
                               bcm_ip_t           vxlan_dip,
                               bcm_vxlan_dip_stat_t  stat,
                               uint32             num_entries,
                               uint32             *counter_indexes,
                               bcm_stat_value_t   *counter_values)
{
#if defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_TD2_TT2(unit)) { 
        return _bcm_td2_vxlan_dip_stat_counter_get(unit, sync_mode, vxlan_dip, stat, 
                    num_entries, counter_indexes, counter_values);
    } else
#endif
    {
        return BCM_E_UNAVAIL;
    }
}


/*
 * Function:
 *     _bcm_td2_vxlan_dip_stat_multi_set
 *
 * Description:
 *  Set vxlan dip counter value for specified IPMC group index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vxlan_dip    - (IN) vxlan dip
 *      stat             - (IN) IPMC group counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t  _bcm_td2_vxlan_dip_stat_multi_set(
                            int                   unit, 
                            bcm_ip_t              vxlan_dip,
                            int                   nstat, 
                            bcm_vxlan_dip_stat_t  *stat_arr,
                            uint64                *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};

    /* Iterate f all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat; idx++) {
         if ((stat_arr[idx] == bcmVxlanDipInPackets)) {
              counter_values.packets = COMPILER_64_LO(value_arr[idx]);
         } else {
              COMPILER_64_SET(counter_values.bytes,
                              COMPILER_64_HI(value_arr[idx]),
                              COMPILER_64_LO(value_arr[idx]));
         }
          BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_dip_stat_counter_set( 
                             unit, vxlan_dip, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
    }
    return rv;

}





/*
 * Function:
 *      _bcm_td2_vxlan_dip_stat_multi_set32
 *
 * Description:
 *  Set 32biv xlan dip counter value for specified IPMC group index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *     vxlan_dip     - (IN)vxlan dip
 *      stat             - (IN) IPMC group counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

 bcm_error_t  _bcm_td2_vxlan_dip_stat_multi_set32(
                            int                   unit, 
                            bcm_ip_t              vxlan_dip,
                            int                   nstat, 
                            bcm_vxlan_dip_stat_t  *stat_arr,
                            uint32                *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};

    /* Iterate of all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat; idx++) {
         if ((stat_arr[idx] == bcmVxlanDipInPackets)) {
             counter_values.packets = value_arr[idx];
         } else {
             COMPILER_64_SET(counter_values.bytes,0,value_arr[idx]);
         }

         BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_dip_stat_counter_set( 
                             unit, vxlan_dip, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_vxlan_dip_stat_id_get
 * Description:
 *   Retrieve associated stat counter for given VXLAN DIP
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vxlan_dip      - (IN) vxlan ip
 *      stat             - (IN) Type of the counter to retrieve
 *                              I.e. ingress/egress byte/packet)
 *      Stat_counter_id  - (OUT) Stat Counter ID
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t 
_bcm_td2_vxlan_dip_stat_id_get(
            int unit,
            bcm_ip_t vxlan_dip,
            bcm_vxlan_dip_stat_t stat,
            uint32 *stat_counter_id)
{
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32                     index=0;
    uint32                     num_stat_counter_ids=0;

    if ((stat == bcmVxlanDipInPackets) ||
        (stat == bcmVxlanDipInBytes)) {
         direction = bcmStatFlexDirectionIngress;
    } else {
         /* direction = bcmStatFlexDirectionEgress; */
         return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_dip_stat_get_table_info(
                        unit,vxlan_dip,&num_of_tables,&table_info[0]));
    for (index=0; index < num_of_tables; index++) {
         if (table_info[index].direction == direction)
             return _bcm_esw_stat_flex_get_counter_id(
                                  unit, 1, &table_info[index],
                                  &num_stat_counter_ids,stat_counter_id);
    }
    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *      _bcm_td2_vxlan_dip_stat_detach
 * Description:
 *      Detach counters entries to the given vxlan dip
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vxlan_dip     - (IN) vxlan dip
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
bcm_error_t
_bcm_td2_vxlan_dip_stat_detach(
            int unit,
            bcm_ip_t vxlan_dip)
{
    uint32                     count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    bcm_error_t                rv[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] =
                                  {BCM_E_FAIL};
    uint32                     flag[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] = {0};

    BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_dip_stat_get_table_info(
                   unit, vxlan_dip,&num_of_tables,&table_info[0]));

    for (count=0; count < num_of_tables; count++) {
      if (table_info[count].direction == bcmStatFlexDirectionIngress) {
           rv[bcmStatFlexDirectionIngress]=
                    _bcm_esw_stat_flex_detach_ingress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index);
           flag[bcmStatFlexDirectionIngress] = 1;
      } else {
         return BCM_E_INTERNAL;
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
 *      _bcm_td2_vxlan_dip_stat_attach
 * Description:
 *      Attach counters entries to the given vxlan dip
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      vxlan_dip      - (IN) vxlan dip 
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 */
bcm_error_t
_bcm_td2_vxlan_dip_stat_attach(
            int unit,
            bcm_ip_t vxlan_dip,
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

    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_object(unit,object,&direction));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_group(unit,group_mode));

    BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_get_table_info(
                        unit,object,4,&actual_num_tables,&table[0],&direction));

    BCM_IF_ERROR_RETURN(_bcm_td2_vxlan_dip_stat_get_table_info(
                   unit, vxlan_dip,&num_of_tables,&table_info[0]));
    for (count=0; count < num_of_tables; count++) {
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
                      return BCM_E_INTERNAL;
                  }
              }
         }
    }
    return BCM_E_NOT_FOUND;
}

#endif /* BCM_TRIDENT2_SUPPORT && INCLUDE_L3 */

