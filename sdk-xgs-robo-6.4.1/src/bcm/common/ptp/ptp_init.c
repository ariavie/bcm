/*
 * $Id: ptp_init.c,v 1.5 Broadcom SDK $
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
 * File:    ptp_init.c
 *
 * Purpose: 
 *
 * Functions:
 *      bcm_common_ptp_detach
 *      bcm_common_ptp_lock
 *      bcm_common_ptp_unlock
 *      bcm_common_ptp_init
 *      bcm_common_ptp_stack_create
 *      bcm_common_ptp_time_format_set
 *      bcm_common_ptp_cb_register
 *      bcm_common_ptp_cb_unregister
 *
 *      _bcm_ptp_clock_cache_init
 *      _bcm_ptp_clock_cache_detach
 *      _bcm_ptp_clock_cache_stack_create
 *      _bcm_ptp_clock_cache_info_create
 *      _bcm_ptp_clock_cache_info_get
 *      _bcm_ptp_clock_cache_info_set
 *      _bcm_ptp_clock_cache_signal_get
 *      _bcm_ptp_clock_cache_signal_set
 *      _bcm_ptp_clock_cache_tod_input_get
 *      _bcm_ptp_clock_cache_tod_input_set
 *      _bcm_ptp_clock_cache_tod_output_get
 *      _bcm_ptp_clock_cache_tod_output_set
 */

#if defined(INCLUDE_PTP)

#include <shared/bsl.h>

#include <soc/defs.h>
#include <soc/drv.h>
#include <shared/bsl.h>
#include <sal/core/dpc.h>

#include <bcm/ptp.h>
#include <bcm_int/common/ptp.h>
#include <bcm_int/ptp_common.h>
#include <bcm/error.h>


static const _bcm_ptp_clock_cache_t cache_default;
static int next_stack_id[BCM_MAX_NUM_UNITS] = {0};  /* per-unit ID of next stack */
_bcm_ptp_unit_cache_t _bcm_common_ptp_unit_array[BCM_MAX_NUM_UNITS] = {{0}};
_bcm_ptp_info_t _bcm_common_ptp_info[BCM_MAX_NUM_UNITS];

/*
 * Function:
 *      bcm_common_ptp_lock
 * Purpose:
 *      Locks the unit mutex
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_INIT
 * Notes:
 */
int bcm_common_ptp_lock(int unit)
{
    if (_bcm_common_ptp_info[unit].mutex == NULL) {
        return BCM_E_INIT;
    }

    return _bcm_ptp_mutex_take(_bcm_common_ptp_info[unit].mutex, 100);
}

/*
 * Function:
 *      bcm_common_ptp_unlock
 * Purpose:
 *      Unlocks the unit mutex
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_INTERNAL
 * Notes:
 */
int bcm_common_ptp_unlock(int unit)
{
    if (_bcm_ptp_mutex_give(_bcm_common_ptp_info[unit].mutex) != 0) {
        return BCM_E_INTERNAL;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_init
 * Purpose:
 *      Initialize the PTP subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_common_ptp_init(
    int unit)
{
    int rv = BCM_E_NONE;
    _bcm_ptp_info_t *ptp_info_p;

    if (soc_feature(unit, soc_feature_ptp)) {
        if (_bcm_common_ptp_info[unit].mutex == NULL) {
            _bcm_common_ptp_info[unit].mutex = _bcm_ptp_mutex_create("ptp.mutex");
            if (_bcm_common_ptp_info[unit].mutex == NULL) {
                rv = BCM_E_MEMORY;
            }
        }
        SET_PTP_INFO;

#if defined(BCM_PTP_INTERNAL_STACK_SUPPORT)
        /* Initialize an SOC with internal PTP stack */
        if (SOC_HAS_PTP_INTERNAL_STACK_SUPPORT(unit)) {
            if (ptp_info_p->memstate == PTP_MEMSTATE_INITIALIZED) {
                
            }
            ptp_info_p->memstate = PTP_MEMSTATE_INITIALIZED;
        }
#endif

        if (SOC_HAS_PTP_EXTERNAL_STACK_SUPPORT(unit)) {
        /* Initialize an SOC with external PTP stack */
            if (ptp_info_p->memstate == PTP_MEMSTATE_INITIALIZED) {
                
            }
            ptp_info_p->memstate = PTP_MEMSTATE_INITIALIZED;
        }

        if (ptp_info_p->memstate != PTP_MEMSTATE_INITIALIZED) {
            rv = BCM_E_UNAVAIL;
            goto done;
        }

        if (ptp_info_p->stack_info == NULL){ 
            ptp_info_p->stack_info = soc_cm_salloc(unit, 
                    sizeof(_bcm_ptp_stack_info_t)*PTP_MAX_STACKS_PER_UNIT, "ptp_stack_info"); 
            sal_memset(ptp_info_p->stack_info, 0, sizeof(_bcm_ptp_stack_info_t)*PTP_MAX_STACKS_PER_UNIT); 
        }

        if (BCM_FAILURE(rv = _bcm_ptp_rx_init(unit))) {
            PTP_ERROR_FUNC("_bcm_ptp_rx_init()");
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_init(unit))) {
            PTP_ERROR_FUNC("_bcm_ptp_clock_cache_init()");
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_management_init(unit))) {
            PTP_ERROR_FUNC("_bcm_ptp_management_init()");
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_unicast_master_table_init(unit))) {
            PTP_ERROR_FUNC("_bcm_ptp_unicast_master_table_init()");
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_unicast_slave_table_init(unit))) {
            PTP_ERROR_FUNC("_bcm_ptp_unicast_slave_table_init()");
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_acceptable_master_table_init(unit))) {
            PTP_ERROR_FUNC("_bcm_ptp_acceptable_master_table_init()");
            goto done;
        }
    }
done:    
    if (BCM_FAILURE(rv)) {
       _bcm_ptp_mutex_destroy(_bcm_common_ptp_info[unit].mutex);
       _bcm_common_ptp_info[unit].mutex = NULL;
    }
    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_detach
 * Purpose:
 *      Shut down the PTP subsystem
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_common_ptp_detach(
    int unit)
{
    int rv = BCM_E_NONE;
    
    
    if (BCM_FAILURE(rv = _bcm_ptp_rx_detach(unit))) {
        PTP_ERROR_FUNC("_bcm_ptp_rx_detach()");
    }

    if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_detach(unit))) {
        PTP_ERROR_FUNC("_bcm_ptp_clock_cache_detach()");
    }

    if (BCM_FAILURE(rv = _bcm_ptp_management_detach(unit))) {
        PTP_ERROR_FUNC("_bcm_ptp_management_detach()");
    }

    if (BCM_FAILURE(rv = _bcm_ptp_unicast_master_table_detach(unit))) {
        PTP_ERROR_FUNC("_bcm_ptp_unicast_master_table_detach()");
    }

    if (BCM_FAILURE(rv = _bcm_ptp_unicast_slave_table_detach(unit))) {
        PTP_ERROR_FUNC("_bcm_ptp_unicast_slave_table_detach()");
    }

    if (BCM_FAILURE(rv = _bcm_ptp_acceptable_master_table_detach(unit))) {
        PTP_ERROR_FUNC("_bcm_ptp_acceptable_master_table_detach()");
    }
    
    if (soc_feature(unit, soc_feature_ptp)) {
        if (_bcm_common_ptp_info[unit].mutex == NULL) {
            return BCM_E_NONE;
        }

        _bcm_ptp_mutex_destroy(_bcm_common_ptp_info[unit].mutex);

        _bcm_common_ptp_info[unit].mutex = NULL;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_stack_create
 * Purpose:
 *      Create a PTP stack instance
 * Parameters:
 *      unit - (IN) Unit number.
 *      ptp_info - (IN/OUT) Pointer to an PTP Stack Info structure
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_common_ptp_stack_create(
    int unit,
    bcm_ptp_stack_info_t *info)
{
    int rv = BCM_E_NONE;
    _bcm_ptp_stack_info_t *stack_p;
    _bcm_ptp_info_t *ptp_info_p;
    pbmp_t pbmp = {{0x03008000}};
    int i;

    SET_PTP_INFO;
    if (soc_feature(unit, soc_feature_ptp)) {
        rv = bcm_common_ptp_lock(unit);
        if (BCM_FAILURE(rv)) {
           return rv;
        }

        if ((info->flags & BCM_PTP_STACK_WITH_ID) == 0) {
            info->id = (bcm_ptp_stack_id_t) next_stack_id[unit]++;
        }

        if (info->id >= PTP_MAX_STACKS_PER_UNIT) {
            /* Zero-based stack ID exceeds max. */
            rv = BCM_E_PARAM;
            goto done;
        }

        /* Check if stack is in use. */
        stack_p = &ptp_info_p->stack_info[info->id];
        if (stack_p->in_use) {
            rv = BCM_E_EXISTS;
            goto done;
        }

        stack_p->stack_id = info->id;
        stack_p->unit = unit;
        stack_p->in_use = 1;
        stack_p->flags = info->flags;

        stack_p->unicast_master_table_size = PTP_MAX_UNICAST_MASTER_TABLE_ENTRIES;
        stack_p->acceptable_master_table_size = PTP_MAX_ACCEPTABLE_MASTER_TABLE_ENTRIES;
        stack_p->unicast_slave_table_size = PTP_MAX_UNICAST_SLAVE_TABLE_ENTRIES;
        stack_p->multicast_slave_stats_size = PTP_MAX_MULTICAST_SLAVE_STATS_ENTRIES;

        /* Check if the stack supports Keystone */
        if (info->flags & BCM_PTP_STACK_EXTERNAL_TOP) {
#if defined(BCM_PTP_EXTERNAL_STACK_SUPPORT)
            if (BCM_FAILURE(rv = _bcm_ptp_external_stack_create(unit, info, stack_p->stack_id))) {
                PTP_ERROR_FUNC("_bcm_ptp_external_stack_create()");
                goto done;
            }
            stack_p->memstate = PTP_MEMSTATE_INITIALIZED;
#else
            rv = BCM_E_PARAM;
            goto done;
#endif
        } else {
#if defined(BCM_PTP_INTERNAL_STACK_SUPPORT)
            if (BCM_FAILURE(rv = _bcm_ptp_internal_stack_create(unit, info, stack_p->stack_id))) {
                goto done;
            }
            stack_p->memstate = PTP_MEMSTATE_INITIALIZED;
#else
            rv = BCM_E_UNAVAIL;
            goto done;
#endif
        }
        if (stack_p->memstate != PTP_MEMSTATE_INITIALIZED) {
            LOG_CLI((BSL_META_U(unit,
                                "Found no supported PTP platforms\n")));
            rv = BCM_E_FAIL;
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_rx_stack_create(                  /* call with blank MACs for Katana */
                unit, stack_p->stack_id, &stack_p->ext_info.host_mac, 
                &stack_p->ext_info.top_mac, stack_p->ext_info.tpid,
                (stack_p->ext_info.vlan | ((int)(stack_p->ext_info.vlan_pri) << 5))))) {
            PTP_ERROR_FUNC("_bcm_ptp_rx_stack_create()");
            goto done;
        }

        for (i = 0; i < PTP_MAX_CLOCKS_PER_STACK; ++i) {
            if (BCM_FAILURE(rv = _bcm_ptp_rx_clock_create(
                    unit, stack_p->stack_id, i))) {
                PTP_ERROR_FUNC("_bcm_ptp_rx_clock_create()");
                goto done;
            }   
        }
        
        if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_stack_create(
            unit,stack_p->stack_id))) {
            PTP_ERROR_FUNC("_bcm_ptp_clock_cache_stack_create()");
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_management_stack_create(
                unit, stack_p->stack_id, &stack_p->ext_info.host_mac, 
                &stack_p->ext_info.top_mac, stack_p->ext_info.host_ip_addr, 
                stack_p->ext_info.top_ip_addr, stack_p->ext_info.vlan, 
                stack_p->ext_info.vlan_pri, pbmp))) {
            PTP_ERROR_FUNC("_bcm_ptp_management_stack_create()");
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_unicast_master_table_stack_create(
               unit, stack_p->stack_id))) {
            PTP_ERROR_FUNC("_bcm_ptp_unicast_master_table_stack_create()");
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_unicast_slave_table_stack_create(
                unit, stack_p->stack_id))) {
            PTP_ERROR_FUNC("_bcm_ptp_unicast_slave_table_stack_create()");
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_acceptable_master_table_stack_create(
                unit, stack_p->stack_id))) {
            PTP_ERROR_FUNC("_bcm_ptp_acceptable_master_table_stack_create()");
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_signaling_init(
                unit, stack_p->stack_id))) {
            PTP_ERROR_FUNC("_bcm_ptp_unicast_signaling_init()");
            goto done;
        }

        if (BCM_FAILURE(rv = bcm_common_ptp_telecom_g8265_init(
                unit, stack_p->stack_id, PTP_CLOCK_NUMBER_DEFAULT))) {
            PTP_ERROR_FUNC("bcm_common_ptp_telecom_g8265_init()");
            goto done;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_ctdev_init(unit, stack_p->stack_id,
                PTP_CLOCK_NUMBER_DEFAULT, 0))) {
            PTP_ERROR_FUNC("_bcm_ptp_ctdev_init()");
            goto done;
        }

        /* T-DPLL support. */
        if (BCM_FAILURE(rv = bcm_tdpll_dpll_instance_init(unit, stack_p->stack_id))) {
            PTP_ERROR_FUNC("bcm_tdpll_dpll_instance_init()");
            goto done;
        }

        if (BCM_FAILURE(rv = bcm_tdpll_input_clock_init(unit, stack_p->stack_id))) {
            PTP_ERROR_FUNC("bcm_tdpll_input_clock_init()");
            goto done;
        }

        if (BCM_FAILURE(rv = bcm_tdpll_output_clock_init(unit, stack_p->stack_id))) {
            PTP_ERROR_FUNC("bcm_tdpll_output_clock_init()");
            goto done;
        }

        if (BCM_FAILURE(rv = bcm_esmc_init(unit, stack_p->stack_id))) {
            PTP_ERROR_FUNC("bcm_esmc_init()");
            goto done;
        }
        if (BCM_FAILURE(rv = bcm_tdpll_esmc_init(unit, stack_p->stack_id))) {
            PTP_ERROR_FUNC("bcm_tdpll_esmc_init()");
            goto done;
        }

done:
        BCM_IF_ERROR_RETURN(bcm_common_ptp_unlock(unit));
        if (rv == BCM_E_NONE) {
            LOG_CLI((BSL_META_U(unit,
                                "PTP stack created\n")));
        }
    } else {
        rv = BCM_E_UNAVAIL;
        LOG_VERBOSE(BSL_LS_BCM_COMMON,
                    (BSL_META_U(unit,
                                "%s() failed %s\n"), FUNCTION_NAME(), "PTP not supported on this unit"));
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_time_format_set
 * Purpose:
 *      Set Time Of Day format for PTP stack instance.
 * Parameters:
 *      unit - (IN) Unit number.
 *      ptp_info - (IN) PTP Stack ID
 *      format - (IN) Time of Day format
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_common_ptp_time_format_set(
    int unit, 
    bcm_ptp_stack_id_t ptp_id, 
    bcm_ptp_time_type_t type)
{
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_common_ptp_cb_register
 * Purpose:
 *      Register a callback for handling PTP events
 * Parameters:
 *      unit - (IN) Unit number.
 *      cb_types - (IN) The set of PTP callbacks types for which the specified callback should be called
 *      cb - (IN) A pointer to the callback function to call for the specified PTP events
 *      user_data - (IN) Pointer to user data to supply in the callback
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_common_ptp_cb_register(
    int unit, 
    bcm_ptp_cb_types_t cb_types, 
    bcm_ptp_cb cb, 
    void *user_data)
{
    int rv = BCM_E_UNAVAIL;

    if (soc_feature(unit, soc_feature_ptp)) {
        rv = bcm_common_ptp_lock(unit);
        if (BCM_FAILURE(rv)) {
           return rv;
        }

        if (SHR_BITGET(cb_types.w, bcmPTPCallbackTypeManagement)) {
            rv = _bcm_ptp_register_management_callback(unit, cb, user_data);
        }

        if (SHR_BITGET(cb_types.w, bcmPTPCallbackTypeEvent)) {
            rv = _bcm_ptp_register_event_callback(unit, cb, user_data);
        }

        if (SHR_BITGET(cb_types.w, bcmPTPCallbackTypeFault)) {
            rv = _bcm_ptp_register_fault_callback(unit, cb, user_data);
        }

        if (SHR_BITGET(cb_types.w, bcmPTPCallbackTypeSignal)) {
            rv = _bcm_ptp_register_signal_callback(unit, cb, user_data);
        }

        if (SHR_BITGET(cb_types.w, bcmPTPCallbackTypeCheckPeers)) {
            rv = _bcm_ptp_register_peers_callback(unit, cb, user_data);
        }

        BCM_IF_ERROR_RETURN(bcm_common_ptp_unlock(unit));
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_cb_unregister
 * Purpose:
 *      Unregister a callback for handling PTP events
 * Parameters:
 *      unit - (IN) Unit number.
 *      event_types - (IN) The set of PTP events to unregister for the specified callback
 *      cb - (IN) A pointer to the callback function to unregister from the specified PTP events
 * Returns:
 *      BCM_E_xxx
 * Notes:
 */
int 
bcm_common_ptp_cb_unregister(
    int unit, 
    bcm_ptp_cb_types_t cb_types, 
    bcm_ptp_cb cb)
{
    int rv = BCM_E_UNAVAIL;

    if (soc_feature(unit, soc_feature_ptp)) {
        rv = bcm_common_ptp_lock(unit);
        if (BCM_FAILURE(rv)) {
           return rv;
        }

        if (SHR_BITGET(cb_types.w, bcmPTPCallbackTypeManagement)) {
            rv = _bcm_ptp_unregister_management_callback(unit);
        }

        if (SHR_BITGET(cb_types.w, bcmPTPCallbackTypeEvent)) {
            rv = _bcm_ptp_unregister_event_callback(unit);
        }

        if (SHR_BITGET(cb_types.w, bcmPTPCallbackTypeFault)) {
            rv = _bcm_ptp_unregister_fault_callback(unit);
        }

        if (SHR_BITGET(cb_types.w, bcmPTPCallbackTypeSignal)) {
            rv = _bcm_ptp_unregister_signal_callback(unit);
        }

        if (SHR_BITGET(cb_types.w, bcmPTPCallbackTypeCheckPeers)) {
            rv = _bcm_ptp_unregister_peers_callback(unit);
        }

        BCM_IF_ERROR_RETURN(bcm_common_ptp_unlock(unit));
    }

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_init
 * Purpose:
 *      Initialize the PTP clock information caches a unit.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
_bcm_ptp_clock_cache_init(
    int unit)
{
    int rv = BCM_E_UNAVAIL;
    
    _bcm_ptp_stack_cache_t *stack_p;
    
    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, PTP_STACK_ID_DEFAULT,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");  
        return rv;
    }
        
    
    stack_p = sal_alloc(PTP_MAX_STACKS_PER_UNIT*
                        sizeof(_bcm_ptp_stack_cache_t),"Unit PTP Clock Information Caches");

    if (!stack_p) {
        _bcm_common_ptp_unit_array[unit].memstate = PTP_MEMSTATE_FAILURE;
        return BCM_E_MEMORY;
    }

    _bcm_common_ptp_unit_array[unit].stack_array = stack_p;
    _bcm_common_ptp_unit_array[unit].memstate = PTP_MEMSTATE_INITIALIZED;
    
    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_detach
 * Purpose:
 *      Shut down the PTP clock information caches a unit.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
_bcm_ptp_clock_cache_detach(
    int unit)
{
    if(_bcm_common_ptp_unit_array[unit].stack_array) {
        sal_free(_bcm_common_ptp_unit_array[unit].stack_array);
        _bcm_common_ptp_unit_array[unit].stack_array = NULL;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_stack_create
 * Purpose:
 *      Create the PTP clock information caches of a PTP stack.
 * Parameters:
 *      unit    - (IN) Unit number.
 *      ptp_id  - (IN) PTP stack ID.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
_bcm_ptp_clock_cache_stack_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id)
{
    int rv = BCM_E_UNAVAIL;
    int i;
    _bcm_ptp_clock_cache_t *clock_p;
    
    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");  
        return rv;
    }
        
    if (_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) {
        return BCM_E_UNAVAIL;
    }

    
    clock_p = sal_alloc(PTP_MAX_CLOCK_INSTANCES*sizeof(_bcm_ptp_clock_cache_t),
                        "Stack PTP Information Caches");

    if (!clock_p) {
        _bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate = 
            PTP_MEMSTATE_FAILURE;
        return BCM_E_MEMORY;
    }

    /* Mark the clocks in the newly-created clock array as not-in-use */
    for (i = 0; i < PTP_MAX_CLOCK_INSTANCES; ++i) {
        clock_p[i].in_use = 0;
    }

    _bcm_common_ptp_unit_array[unit].stack_array[ptp_id].clock_array = clock_p;
    _bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate = 
        PTP_MEMSTATE_INITIALIZED;
    
    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_info_create
 * Purpose:
 *      Create the information cache of a PTP clock.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
_bcm_ptp_clock_cache_info_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num)
{
    int rv = BCM_E_UNAVAIL;
    int i;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");  
        return rv;
    }
        
    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate != 
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    
    _bcm_common_ptp_unit_array[unit].stack_array[ptp_id].clock_array[clock_num] = 
        cache_default;

    /* Initialize port states. */
    for (i = 0; i < PTP_MAX_CLOCK_INSTANCE_PORTS; ++i) {
        _bcm_common_ptp_unit_array[unit].stack_array[ptp_id].clock_array[clock_num]
                                     .portState[i] = _bcm_ptp_state_initializing;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_info_get
 * Purpose:
 *      Get the clock information cache of a PTP clock.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_info - (OUT) PTP clock information cache.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int _bcm_ptp_clock_cache_info_get(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_clock_info_t *clock_info)
{
    int rv = BCM_E_UNAVAIL;
    
    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");  
        return rv;
    }
        
    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate != 
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }        

    *clock_info = _bcm_common_ptp_unit_array[unit].stack_array[ptp_id]
                                  .clock_array[clock_num].clock_info;
    
    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_info_set
 * Purpose:
 *      Set the clock information cache of a PTP clock.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_info - (IN) PTP clock information cache.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int _bcm_ptp_clock_cache_info_set(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    const bcm_ptp_clock_info_t clock_info)
{
    int rv = BCM_E_UNAVAIL;
         
    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");  
        return rv;
    }
        
    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate != 
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }        

    _bcm_common_ptp_unit_array[unit].stack_array[ptp_id]
                    .clock_array[clock_num].clock_info = clock_info;
    
    return rv;
}

int _bcm_ptp_clock_cache_signal_get(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int signal_num,
    bcm_ptp_signal_output_t *signal)
{
    int rv = BCM_E_UNAVAIL;
    
    if (signal_num >= PTP_MAX_OUTPUT_SIGNALS || signal_num < 0) {
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");  
        return rv;
    }
        
    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate != 
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }        

    *signal = _bcm_common_ptp_unit_array[unit].stack_array[ptp_id].clock_array[clock_num].signal_output[signal_num];
    
    return rv;
}


int _bcm_ptp_clock_cache_signal_set(
    int unit, 
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int signal_num,
    const bcm_ptp_signal_output_t *signal)
{
    int rv = BCM_E_UNAVAIL;
    
    if (signal_num >= PTP_MAX_OUTPUT_SIGNALS || signal_num < 0) {
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");  
        return rv;
    }
        
    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate != 
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }        

    _bcm_common_ptp_unit_array[unit].stack_array[ptp_id].clock_array[clock_num].signal_output[signal_num] = *signal;
    
    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_tod_input_get
 * Purpose:
 *      Get a ToD input configuration from PTP clock information cache.
 * Parameters:
 *      unit          - (IN)  Unit number.
 *      ptp_id        - (IN)  PTP stack ID.
 *      clock_num     - (IN)  PTP clock number.
 *      tod_input_num - (IN)  ToD input configuration number.
 *      tod_input     - (OUT) ToD input configuration.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int _bcm_ptp_clock_cache_tod_input_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int tod_input_num,
    bcm_ptp_tod_input_t *tod_input)
{
    int rv = BCM_E_UNAVAIL;

    if (tod_input_num >= PTP_MAX_TOD_INPUTS || tod_input_num < 0) {
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate !=
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    *tod_input = _bcm_common_ptp_unit_array[unit].stack_array[ptp_id]
                                              .clock_array[clock_num]
                                              .tod_input[tod_input_num];

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_tod_input_set
 * Purpose:
 *      Set a ToD input configuration in PTP clock information cache.
 * Parameters:
 *      unit          - (IN) Unit number.
 *      ptp_id        - (IN) PTP stack ID.
 *      clock_num     - (IN) PTP clock number.
 *      tod_input_num - (IN) ToD input configuration number.
 *      tod_input     - (IN) ToD input configuration.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int _bcm_ptp_clock_cache_tod_input_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int tod_input_num,
    const bcm_ptp_tod_input_t *tod_input)
{
    int rv = BCM_E_UNAVAIL;

    if (tod_input_num >= PTP_MAX_TOD_INPUTS || tod_input_num < 0) {
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate != 
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    _bcm_common_ptp_unit_array[unit].stack_array[ptp_id]
                                 .clock_array[clock_num]
                                 .tod_input[tod_input_num] = *tod_input;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_tod_output_get
 * Purpose:
 *      Get a ToD output configuration from PTP clock information cache.
 * Parameters:
 *      unit           - (IN)  Unit number.
 *      ptp_id         - (IN)  PTP stack ID.
 *      clock_num      - (IN)  PTP clock number.
 *      tod_output_num - (IN)  ToD output configuration number.
 *      tod_output     - (OUT) ToD output configuration.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int _bcm_ptp_clock_cache_tod_output_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int tod_output_num,
    bcm_ptp_tod_output_t *tod_output)
{
    int rv = BCM_E_UNAVAIL;

    if (tod_output_num >= PTP_MAX_TOD_OUTPUTS || tod_output_num < 0) {
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate != 
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    *tod_output = _bcm_common_ptp_unit_array[unit].stack_array[ptp_id]
                                               .clock_array[clock_num]
                                               .tod_output[tod_output_num];

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_tod_output_set
 * Purpose:
 *      Set a ToD output configuration in PTP clock information cache.
 * Parameters:
 *      unit           - (IN) Unit number.
 *      ptp_id         - (IN) PTP stack ID.
 *      clock_num      - (IN) PTP clock number.
 *      tod_output_num - (IN) ToD output configuration number.
 *      tod_output     - (IN) ToD output configuration.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int _bcm_ptp_clock_cache_tod_output_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int tod_output_num,
    const bcm_ptp_tod_output_t *tod_output)
{
    int rv = BCM_E_UNAVAIL;

    if (tod_output_num >= PTP_MAX_TOD_OUTPUTS || tod_output_num < 0) {
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate != 
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    _bcm_common_ptp_unit_array[unit].stack_array[ptp_id]
                                 .clock_array[clock_num]
                                 .tod_output[tod_output_num] = *tod_output;

    return rv;
}


extern int _bcm_ptp_set_max_unicast_masters(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int max_masters)
{
    _bcm_ptp_info_t *ptp_info_p;
    if (soc_feature(unit, soc_feature_ptp) == 0) {
        return BCM_E_UNAVAIL;
    }

    SET_PTP_INFO;
    if (ptp_info_p->memstate != PTP_MEMSTATE_INITIALIZED) {
        return BCM_E_PARAM;
    }

    ptp_info_p->stack_info->unicast_master_table_size = max_masters;
    return BCM_E_NONE;
}


extern int _bcm_ptp_set_max_acceptable_masters(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int max_masters)
{
    _bcm_ptp_info_t *ptp_info_p;
    if (soc_feature(unit, soc_feature_ptp) == 0) {
        return BCM_E_UNAVAIL;
    }

    SET_PTP_INFO;
    if (ptp_info_p->memstate != PTP_MEMSTATE_INITIALIZED) {
        return BCM_E_PARAM;
    }

    ptp_info_p->stack_info->acceptable_master_table_size = max_masters;
    return BCM_E_NONE;
}


extern int _bcm_ptp_set_max_unicast_slaves(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int max_slaves)
{
    _bcm_ptp_info_t *ptp_info_p;
    if (soc_feature(unit, soc_feature_ptp) == 0) {
        return BCM_E_UNAVAIL;
    }

    SET_PTP_INFO;
    if (ptp_info_p->memstate != PTP_MEMSTATE_INITIALIZED) {
        return BCM_E_PARAM;
    }

    ptp_info_p->stack_info->unicast_slave_table_size = max_slaves;
    return BCM_E_NONE;
}

extern int _bcm_ptp_set_max_multicast_slave_stats(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int max_entries)
{
    _bcm_ptp_info_t *ptp_info_p;
    if (soc_feature(unit, soc_feature_ptp) == 0) {
        return BCM_E_UNAVAIL;
    }

    SET_PTP_INFO;
    if (ptp_info_p->memstate != PTP_MEMSTATE_INITIALIZED) {
        return BCM_E_PARAM;
    }

    ptp_info_p->stack_info->multicast_slave_stats_size = max_entries;
    return BCM_E_NONE;
}


#endif /* defined(INCLUDE_PTP)*/
