/*
 * $Id: clock_init.c,v 1.1 Broadcom SDK $
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
 * File:    clock_init.c
 *
 * Purpose: 
 *
 * Functions:
 *      bcm_common_ptp_clock_port_identity_get
 *      bcm_common_ptp_clock_create
 */

#if defined(INCLUDE_PTP)

#include <shared/bsl.h>

#include <soc/defs.h>
#include <soc/drv.h>

#include <bcm/ptp.h>
#include <bcm_int/common/ptp.h>
#include <bcm_int/ptp_common.h>
#include <bcm/error.h>

/*
 * Function:
 *      bcm_common_ptp_clock_port_identity_get
 * Purpose:
 *      Get port identity of a PTP clock port.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      identity   - (OUT) PTP port identity.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_common_ptp_clock_port_identity_get(
    int unit, 
    bcm_ptp_stack_id_t ptp_id, 
    int clock_num, 
    uint32 clock_port, 
    bcm_ptp_port_identity_t *identity)
{
    int rv = BCM_E_UNAVAIL;
    
    bcm_ptp_clock_info_t ci;
    
    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;   
    }
        
    if (BCM_FAILURE(rv = 
            _bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, &ci))) {
        return rv;
    }

    sal_memcpy(identity->clock_identity, 
               ci.clock_identity, 
               sizeof(bcm_ptp_clock_identity_t));
    
    identity->port_number = (uint16)clock_port; 
    
    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_create
 * Purpose:
 *      Create a PTP clock.
 * Parameters:
 *      unit       - (IN)     Unit number.
 *      ptp_id     - (IN)     PTP stack ID.
 *      clock_info - (IN/OUT) PTP clock information.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int 
bcm_common_ptp_clock_create(
    int unit, 
    bcm_ptp_stack_id_t ptp_id, 
    bcm_ptp_clock_info_t *clock_info)
{
    int rv = BCM_E_UNAVAIL;
    
    uint8 payload[PTP_MGMTMSG_PAYLOAD_CLOCK_INSTANCE_SIZE_OCTETS]= {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len= PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    int i= 0;
   
    _bcm_ptp_info_t *ptp_info_p;
    bcm_ptp_port_identity_t portid;
    _bcm_ptp_clock_cache_t *clock_cache;
    int clock_max_num_ports;

    bcm_ptp_telecom_g8265_network_option_t telecom_network_option =
        bcm_ptp_telecom_g8265_network_option_disable;

    sal_sleep(1);  

    if (soc_feature(unit, soc_feature_ptp)) {
        rv = BCM_E_NONE;

        rv = bcm_common_ptp_lock(unit);
        if (BCM_FAILURE(rv)) {
           return rv;
        }

        SET_PTP_INFO;

        if (ptp_info_p->memstate != PTP_MEMSTATE_INITIALIZED) {
            return BCM_E_PARAM;
        }

        
#if 0
        if (clock_info->flags & BCM_PTP_CLOCK_WITH_ID) {
            if (clock_info->clock_num  > PTP_MAX_CLOCKS_PER_STACK) {
                return BCM_E_PARAM;
            }
            
        }else {
            if (clock_info->clock_num  >= PTP_MAX_CLOCKS_PER_STACK) {
                return BCM_E_PARAM;
            }
        }
#else
        clock_info->clock_num = 0;
#endif
 
        /* Clock type and port number checks. */
        switch (clock_info->type) {
        case bcmPTPClockTypeOrdinary:
            if (clock_info->num_ports != 1) {
                /* Too few / too many ports. */
                return BCM_E_PARAM; 
            }
            break;
            
        case bcmPTPClockTypeBoundary:
            if ((clock_info->num_ports < 1) ||
                    (clock_info->num_ports > PTP_MAX_CLOCK_INSTANCE_PORTS)) {
                /* Too few / too many ports. */
                return BCM_E_PARAM; 
            }
            break;
            
        case bcmPTPClockTypeTransparent:
            /* Fall through (v1.2.0 firmware does not support transparent clocks). */            
        default:
            /* Invalid/unknown clock type. */
            return BCM_E_UNAVAIL; 
        }

        /* Parameter extrema checking. */
        if ((clock_info->announce_receipt_timeout_minimum < 
                PTP_CLOCK_PRESETS_ANNOUNCE_RECEIPT_TIMEOUT_MINIMUM) ||
                (clock_info->announce_receipt_timeout_maximum > 
                PTP_CLOCK_PRESETS_ANNOUNCE_RECEIPT_TIMEOUT_MAXIMUM)) {
            /* Caller provided (min,max) exceed allowable value(s). */
            return BCM_E_PARAM; 
        }

        if (SHR_BITGET(&(clock_info->flags), PTP_CLOCK_FLAGS_TELECOM_PROFILE_BIT)) {
            /* Telecom profile. */
            if ((clock_info->log_announce_interval_minimum <
                    PTP_CLOCK_PRESETS_TELECOM_LOG_ANNOUNCE_INTERVAL_MINIMUM) ||
                    (clock_info->log_announce_interval_maximum > 
                    PTP_CLOCK_PRESETS_TELECOM_LOG_ANNOUNCE_INTERVAL_MAXIMUM)) {
                /* Caller provided (min,max) exceed allowable value(s). */
                return BCM_E_PARAM; 
            }

            if ((clock_info->log_sync_interval_minimum <
                    PTP_CLOCK_PRESETS_TELECOM_LOG_SYNC_INTERVAL_MINIMUM) ||
                    (clock_info->log_sync_interval_maximum > 
                    PTP_CLOCK_PRESETS_TELECOM_LOG_SYNC_INTERVAL_MAXIMUM)) {
                /* Caller provided (min,max) exceed allowable value(s). */
                return BCM_E_PARAM; 
            }

            if ((clock_info->log_min_delay_req_interval_minimum <
                     PTP_CLOCK_PRESETS_TELECOM_LOG_MIN_DELAY_REQ_INTERVAL_MINIMUM) ||
                     (clock_info->log_min_delay_req_interval_maximum > 
                     PTP_CLOCK_PRESETS_TELECOM_LOG_MIN_DELAY_REQ_INTERVAL_MAXIMUM)) {
                /* Caller provided (min,max) exceed allowable value(s). */
                return BCM_E_PARAM; 
            }

            if ((clock_info->domain_number_minimum <
                     PTP_CLOCK_PRESETS_TELECOM_DOMAIN_NUMBER_MINIMUM) ||
                     (clock_info->domain_number_maximum > 
                     PTP_CLOCK_PRESETS_TELECOM_DOMAIN_NUMBER_MAXIMUM)) {
                /* Caller provided (min,max) exceed allowable value(s). */
                return BCM_E_PARAM; 
            }
        } else {
            /* Standard profile. */
            if ((clock_info->log_announce_interval_minimum <
                    PTP_CLOCK_PRESETS_LOG_ANNOUNCE_INTERVAL_MINIMUM) ||
                    (clock_info->log_announce_interval_maximum > 
                    PTP_CLOCK_PRESETS_LOG_ANNOUNCE_INTERVAL_MAXIMUM)) {
                /* Caller provided (min,max) exceed allowable value(s). */
                return BCM_E_PARAM; 
            }

            if ((clock_info->log_sync_interval_minimum <
                    PTP_CLOCK_PRESETS_LOG_SYNC_INTERVAL_MINIMUM) ||
                    (clock_info->log_sync_interval_maximum > 
                    PTP_CLOCK_PRESETS_LOG_SYNC_INTERVAL_MAXIMUM)) {
                /* Caller provided (min,max) exceed allowable value(s). */
                return BCM_E_PARAM; 
            }

            if ((clock_info->log_min_delay_req_interval_minimum <
                     PTP_CLOCK_PRESETS_LOG_MIN_DELAY_REQ_INTERVAL_MINIMUM) ||
                     (clock_info->log_min_delay_req_interval_maximum > 
                     PTP_CLOCK_PRESETS_LOG_MIN_DELAY_REQ_INTERVAL_MAXIMUM)) {
                /* Caller provided (min,max) exceed allowable value(s). */
                return BCM_E_PARAM; 
            }
        }

        /* DEFAULT clock configuration PTP attribute checking. */
        if ((clock_info->announce_receipt_timeout_default < 
                 clock_info->announce_receipt_timeout_minimum) ||
                (clock_info->announce_receipt_timeout_default> 
                 clock_info->announce_receipt_timeout_maximum)) {
             /* Caller provided default value is not in (min,max) range. */
            return BCM_E_PARAM;
        }

        if ((clock_info->log_announce_interval_default < 
                 clock_info->log_announce_interval_minimum) ||
                (clock_info->log_announce_interval_default > 
                 clock_info->log_announce_interval_maximum)) {
            /* Caller provided default value is not in (min,max) range. */
            return BCM_E_PARAM; 
        }

        if ((clock_info->log_sync_interval_default < 
                 clock_info->log_sync_interval_minimum) ||
                (clock_info->log_sync_interval_default > 
                 clock_info->log_sync_interval_maximum)) {
            /* Caller provided default value is not in (min,max) range. */
            return BCM_E_PARAM; 
        }

        if ((clock_info->log_min_delay_req_interval_default < 
                 clock_info->log_min_delay_req_interval_minimum) ||
                (clock_info->log_min_delay_req_interval_default > 
                 clock_info->log_min_delay_req_interval_maximum)) {
            /* Caller provided default value is not in (min,max) range. */
            return BCM_E_PARAM; 
        }

        if ((clock_info->domain_number_default < 
                 clock_info->domain_number_minimum) ||
                (clock_info->domain_number_default > 
                 clock_info->domain_number_maximum)) 
        {
            /* Caller provided default value is not in (min,max) range. */
            return BCM_E_PARAM; 
        }

        if ((clock_info->priority1_default < clock_info->priority1_minimum) ||
                (clock_info->priority1_default > clock_info->priority1_maximum)) {
            /* Caller provided default value is not in (min,max) range. */
            return BCM_E_PARAM; 
        }

        if ((clock_info->priority2_default < clock_info->priority2_minimum) ||
                (clock_info->priority2_default > clock_info->priority2_maximum)) {
            /* Caller provided default value is not in (min,max) range. */
            return BCM_E_PARAM; 
        }

        /* CURRENT clock configuration PTP attribute checking. */
        if ((clock_info->domain_number < clock_info->domain_number_minimum) ||
                (clock_info->domain_number > clock_info->domain_number_maximum)) {
            /* Caller provided current value is not in (min,max) range. */
            return BCM_E_PARAM; 
        }
        
        if ((clock_info->priority1< clock_info->priority1_minimum) ||
                (clock_info->priority1> clock_info->priority1_maximum)) {
            /* Caller provided current value is not in (min,max) range. */
            return BCM_E_PARAM; 
        }
        
        if ((clock_info->priority2< clock_info->priority2_minimum) ||
                (clock_info->priority2> clock_info->priority2_maximum)) {
            /* Caller provided current value is not in (min,max) range. */
            return BCM_E_PARAM; 
        }    

        /* Set clock flags based on active PTP profile. */
        if (SHR_BITGET(&(clock_info->flags), PTP_CLOCK_FLAGS_TELECOM_PROFILE_BIT)) {
            SHR_BITSET(&(clock_info->flags), PTP_CLOCK_FLAGS_DELREQ_ALTMASTER_BIT);
        }

        /* If this is the first initialization, the passed-in num ports will be the max for this clock */
        clock_max_num_ports = clock_info->num_ports;

        /* Assign PTP clock information to cache. */
        clock_cache = &_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].clock_array[clock_info->clock_num];

        if (clock_cache->in_use) {
            /* reconfiguration.  Don't allow the number of ports to exceed the initial config */
            if (clock_info->num_ports > clock_cache->max_num_ports) {
                LOG_ERROR(BSL_LS_BCM_COMMON,
                          (BSL_META_U(unit,
                                      "Clock reconfiguration with %d ports exceeds initial number (%d)\n"),
                           clock_info->num_ports, clock_cache->max_num_ports));
                return BCM_E_PARAM;
            }

            /* Keep the original number of ports as the max */
            clock_max_num_ports = clock_cache->max_num_ports;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_info_create(
                                  unit, ptp_id, clock_info->clock_num))) {
            return rv;
        }

        /* Set initially configured number of ports, as the upper limit for reconfiguration */
        clock_cache->max_num_ports = clock_max_num_ports;
        clock_cache->in_use = 1;

        clock_cache->clock_info = *clock_info;

        if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id, 
                clock_info->clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
            return rv;
        }
        
        /* Create the PTP management data for the PTP clock. */
        if (BCM_FAILURE(rv = _bcm_ptp_management_clock_create(unit, ptp_id, clock_info->clock_num))) {
            return rv;
        }

        /*
         * Make payload.
         *    Octet 0...2   : Custom management message key/identifier.
         *                   Octet 0= 'B'; Octet 1= 'C'; Octet 2= 'M'.
         *    Octet 3...5   : Reserved.
         *    Octet 6       : Instance.
         *    Octet 7       : Clock type.
         *    Octet 8...9   : Number of ports.
         *    Octet 10      : Clock class.
         *    Octet 11      : PTP domain.
         *    Octet 12...13 : Scaled log variance.
         *    Octet 14      : Priority1.
         *    Octet 15      : Priority2.
         *    Octet 16      : Slave-Only (SO) Boolean.
         *    Octet 17      : Transparent clock primary domain.
         *    Octet 18      : Transparent clock delay mechanism.
         *    Octet 19      : Log announce interval (MINIMUM).
         *    Octet 20      : Log announce interval (DEFAULT).
         *    Octet 21      : Log announce interval (MAXIMUM).
         *    Octet 22      : Announce receipt timeout (MINIMUM).
         *    Octet 23      : Announce receipt timeout (DEFAULT).
         *    Octet 24      : Announce receipt timeout (MAXIMUM).
         *    Octet 25      : Log sync interval (MINIMUM).
         *    Octet 26      : Log sync interval (DEFAULT).
         *    Octet 27      : Log sync interval (MAXIMUM).
         *    Octet 28      : Log min PDelay request interval (MININUM) or
         *                    log min delay request interval (MINIMUM).
         *    Octet 29      : Log min PDelay request interval (DEFAULT) or
         *                    log min delay request interval (DEFAULT).
         *    Octet 30      : Log min PDelay request interval (MAXIMUM) or
         *                    log min delay request interval (MAXIMUM).
         *    Octet 31      : PTP domain number (MINIMUM).
         *    Octet 32      : PTP domain number (DEFAULT).
         *    Octet 33      : PTP domain number (MAXIMUM).
         *    Octet 34      : Priority1 (MINIMUM).
         *    Octet 35      : Priority1 (DEFAULT).
         *    Octet 36      : Priority1 (MAXIMUM).
         *    Octet 37      : Priority2 (MINIMUM).
         *    Octet 38      : Priority2 (DEFAULT).
         *    Octet 39      : Priority2 (MAXIMUM).
         *    Octet 40      : Flags
         *    Octet 41-42   : Max Unicast Master table size.
         *    Octet 43-44   : Max Acceptable Master table size.
         *    Octet 45-46   : Max Unicast Slave table size.
         *    Octet 47-48   : Max MC slaves (for peer data)
         */
        i = 0;
        payload[i++] = 'B';
        payload[i++] = 'C';
        payload[i++] = 'M';
        i += 3;
       
        payload[i++] = clock_info->clock_num;
        payload[i++] = clock_info->type;
       
        _bcm_ptp_uint16_write(payload+i, clock_info->num_ports);
        i += sizeof(uint16);
       
        payload[i++] = clock_info->clock_class;
        payload[i++] = clock_info->domain_number;
       
        _bcm_ptp_uint16_write(payload+i, clock_info->scaled_log_variance);
        i += sizeof(uint16);
       
        payload[i++] = clock_info->priority1;
        payload[i++] = clock_info->priority2;
        payload[i++] = clock_info->slaveonly & 1;
       
        payload[i++] = clock_info->tc_primary_domain;
        payload[i++] = clock_info->tc_delay_mechanism;
       
        payload[i++] = clock_info->log_announce_interval_minimum;
        payload[i++] = clock_info->log_announce_interval_default;
        payload[i++] = clock_info->log_announce_interval_maximum;
       
        payload[i++] = clock_info->announce_receipt_timeout_minimum;
        payload[i++] = clock_info->announce_receipt_timeout_default;
        payload[i++] = clock_info->announce_receipt_timeout_maximum;
       
        payload[i++] = clock_info->log_sync_interval_minimum;
        payload[i++] = clock_info->log_sync_interval_default;
        payload[i++] = clock_info->log_sync_interval_maximum;
       
        payload[i++] = clock_info->log_min_delay_req_interval_minimum;
        payload[i++] = clock_info->log_min_delay_req_interval_default;
        payload[i++] = clock_info->log_min_delay_req_interval_maximum;
       
        payload[i++] = clock_info->domain_number_minimum;
        payload[i++] = clock_info->domain_number_default;
        payload[i++] = clock_info->domain_number_maximum;
        
        payload[i++] = clock_info->priority1_minimum;
        payload[i++] = clock_info->priority1_default;
        payload[i++] = clock_info->priority1_maximum;
       
        payload[i++] = clock_info->priority2_minimum;
        payload[i++] = clock_info->priority2_default;
        payload[i++] = clock_info->priority2_maximum;

        {
            /* Note: this relies on the firmware using bits 0-6 to represent
               the same as 1-7 of the passed-in structure.  Is correct for now,
               but may need more logic in future */
            uint8 passed_flags = (clock_info->flags >> 1);
            payload[i++] = passed_flags;
        }

        _bcm_ptp_uint16_write(payload+i, ptp_info_p->stack_info->unicast_master_table_size);
        i += sizeof(uint16);

        _bcm_ptp_uint16_write(payload+i, ptp_info_p->stack_info->acceptable_master_table_size);
        i += sizeof(uint16);

        _bcm_ptp_uint16_write(payload+i, ptp_info_p->stack_info->unicast_slave_table_size);
        i += sizeof(uint16);

        _bcm_ptp_uint16_write(payload+i, ptp_info_p->stack_info->multicast_slave_stats_size);
        i += sizeof(uint16);

        if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id, 
                clock_info->clock_num, &portid,
                PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_CREATE_CLOCK_INSTANCE,
                payload, PTP_MGMTMSG_PAYLOAD_CLOCK_INSTANCE_SIZE_OCTETS, 
                resp, &resp_len))) {

            return rv;    
        }

        /*
         * Register domain with PTP management framework.
         * Ensure subsequent management messages are configured with proper 
         * domain.
         */
        if (BCM_FAILURE(rv = _bcm_ptp_management_domain_set(unit, ptp_id, 
                clock_info->clock_num, clock_info->domain_number))) {
            return rv;
        }
      
        /* Create a unicast master table for the PTP clock. */
        if (BCM_FAILURE(rv = _bcm_ptp_unicast_master_table_clock_create(unit,
                ptp_id, clock_info->clock_num))) {
            return rv;
        }

        /* Create unicast slave tables for the PTP clock ports. */
        if (BCM_FAILURE(rv = _bcm_ptp_unicast_slave_table_clock_create(unit,
                ptp_id, clock_info->clock_num))) {
            return rv;
        }

        /* Create an acceptable master table for the PTP clock. */
        if (BCM_FAILURE(rv = _bcm_ptp_acceptable_master_table_clock_create(unit,
                ptp_id, clock_info->clock_num))) {
            return rv;
        }

        /*
         * Telecom profile support.
         * Enable/disable host-side telecom profile.
         */
        if (SHR_BITGET(&(clock_info->flags), PTP_CLOCK_FLAGS_TELECOM_PROFILE_BIT)) {
            telecom_network_option = PTP_TELECOM_NETWORKING_OPTION_DEFAULT;
        } else {
            telecom_network_option = bcm_ptp_telecom_g8265_network_option_disable;
        }

        if (BCM_FAILURE(rv = bcm_common_ptp_telecom_g8265_network_option_set(unit, ptp_id,
                clock_info->clock_num, telecom_network_option))) {
            return rv;
        }

        /* On Katana, there is a default 1PPS output on TSGPIO1, so set that up: */
        if (SOC_HAS_PTP_INTERNAL_STACK_SUPPORT(unit)) {
            bcm_ptp_signal_output_t signal;
            int signal_id = 0;
            signal.pin = 1;
            signal.frequency = 1;
            signal.phase_lock = 1;
            signal.pulse_width_ns = 100000000;
            signal.pulse_offset_ns = 0;

            if (BCM_FAILURE(rv = bcm_common_ptp_signal_output_set(unit, ptp_id,
                                                               clock_info->clock_num,
                                                               &signal_id, &signal))) {
                return rv;
            }
        }
        
        BCM_IF_ERROR_RETURN(bcm_common_ptp_unlock(unit));
    }

    return rv;
}
#endif /* defined(INCLUDE_PTP)*/

