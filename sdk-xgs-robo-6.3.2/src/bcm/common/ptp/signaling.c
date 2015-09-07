/*
 * $Id: signaling.c 1.1 Broadcom SDK $
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
 * File:    signaling.c
 *
 * Purpose: 
 *
 * Functions:
 *      bcm_common_ptp_signaled_unicast_master_add
 *      bcm_common_ptp_signaled_unicast_master_remove
 *      bcm_common_ptp_signaled_unicast_slave_list
 *      bcm_common_ptp_signaled_unicast_slave_table_clear
 *      bcm_common_ptp_unicast_request_duration_get
 *      bcm_common_ptp_unicast_request_duration_set
 *      bcm_common_ptp_unicast_request_duration_min_get
 *      bcm_common_ptp_unicast_request_duration_min_set
 *      bcm_common_ptp_unicast_request_duration_max_get
 *      bcm_common_ptp_unicast_request_duration_max_set
 *
 *      _bcm_ptp_register_signaling_arbiter
 *      _bcm_ptp_unregister_signaling_arbiter
 *      _bcm_ptp_signaling_init
 *      _bcm_ptp_signaling_manager
 *      _bcm_ptp_signaled_unicast_master_table_init
 *      _bcm_ptp_signaled_unicast_master_table_reset
 *      _bcm_ptp_signaled_unicast_slave_table_init
 *      _bcm_ptp_signaled_unicast_slave_table_reset
 *      _bcm_ptp_signaled_unicast_slave_exists
 *      _bcm_ptp_signaled_unicast_slave_add
 *      _bcm_ptp_signaled_unicast_slave_remove
 *      _bcm_ptp_signaled_unicast_slave_expire
 *      _bcm_ptp_signaled_unicast_slave_table_update
 *      send_unicast_request
 *      _bcm_ptp_master_active_service_cancel
 *      _bcm_ptp_master_active_service_ack_cancel
 *      _bcm_ptp_slave_signaling_send
 *      send_pending_signaling_to_masters
 *      _bcm_ptp_slave_signaling_request_deny
 *      _bcm_ptp_slave_signaling_gateway
 *      _bcm_ptp_parse_signaling_message
 *      _bcm_ptp_make_peer_from_signaling_msg
 *      _bcm_ptp_process_incoming_signaling_msg
 *      delayed_master_table_update
 */

#if defined(INCLUDE_PTP)

#ifdef BCM_HIDE_DISPATCHABLE
#undef BCM_HIDE_DISPATCHABLE
#endif

#include <soc/defs.h>
#include <soc/drv.h>

#include <sal/appl/io.h>
#include <sal/core/libc.h>
#include <sal/core/alloc.h>
#include <sal/core/time.h>
#include <sal/core/dpc.h>
#include <shared/bitop.h>
#include <bcm/ptp.h>
#include <bcm_int/common/ptp.h>
#include <bcm_int/ptp_common.h>

/* Parameters and constants. */
#define BCM_PTP_SIGNAL_REQUEST_DENY_DURATION_SEC     (0)

#define BCM_PTP_SIGNAL_GRANT_UNICAST_RENEWAL_INVITED (1)
#define BCM_PTP_SIGNAL_TELECOM_GRANT_UNICAST_RENEWAL_INVITED (0)

#define BCM_PTP_SIGNAL_MIN_SLAVE_DPC_TIME_USEC       (50)
#define BCM_PTP_SIGNAL_DEFAULT_SLAVE_DPC_TIME_USEC   (1000000)
#define BCM_PTP_SIGNAL_SLAVE_MUTEX_TIMEOUT_USEC      (50000)

#define BCM_PTP_SIGNAL_REQUEST_UC_MINIMUM_DURATION_SEC (10)
#define BCM_PTP_SIGNAL_REQUEST_UC_DEFAULT_DURATION_SEC (300)
#define BCM_PTP_SIGNAL_REQUEST_UC_MAXIMUM_DURATION_SEC (1000)

#define BCM_PTP_SIGNAL_TELECOM_REQUEST_UC_MINIMUM_DURATION_SEC (60)
#define BCM_PTP_SIGNAL_TELECOM_REQUEST_UC_DEFAULT_DURATION_SEC (300)
#define BCM_PTP_SIGNAL_TELECOM_REQUEST_UC_MAXIMUM_DURATION_SEC (1000)

#define BCM_PTP_SIGNAL_TELECOM_UNMET_REQUEST_MAX                (3)
#define BCM_PTP_SIGNAL_TELECOM_UNMET_REQUEST_QUERY_INTERVAL_SEC (60)

#define BCM_PTP_SIGNAL_MIN_MASTER_DPC_TIME_USEC      (50)
#define BCM_PTP_SIGNAL_DEFAULT_MASTER_DPC_TIME_USEC  (1000000)

#define BCM_PTP_SIGNAL_TLV_OFFSET_OCTETS             (44)
#define BCM_PTP_SIGNAL_MIN_TLV_SIZE_OCTETS           (6)
#define BCM_PTP_SIGNAL_MAX_TLV_SIZE_OCTETS           (12)

#define BCM_PTP_SIGNAL_MAX_TLV_PER_MESSAGE           (3)
#define BCM_PTP_SIGNAL_MAX_MULTI_TLV_SIZE_OCTETS     (BCM_PTP_SIGNAL_MAX_TLV_SIZE_OCTETS * \
                                                      BCM_PTP_SIGNAL_MAX_TLV_PER_MESSAGE)

#define BCM_PTP_SIGNAL_MIN_UDP_PAYLOAD_SIZE_OCTETS   (BCM_PTP_SIGNAL_TLV_OFFSET_OCTETS + \
                                                      BCM_PTP_SIGNAL_MIN_TLV_SIZE_OCTETS)
#define BCM_PTP_SIGNAL_MAX_UDP_PAYLOAD_SIZE_OCTETS   (BCM_PTP_SIGNAL_TLV_OFFSET_OCTETS + \
                                                      BCM_PTP_SIGNAL_MAX_MULTI_TLV_SIZE_OCTETS)
#define BCM_PTP_SIGNAL_MAX_PKT_SIZE_OCTETS           (PTP_MSG_L2_HEADER_LENGTH + 40 + \
                                                      PTP_UDPHDR_SIZE_OCTETS + \
                                                      BCM_PTP_SIGNAL_MAX_UDP_PAYLOAD_SIZE_OCTETS)

/* Macros. */
#define BCM_PTP_SLAVETABLE_MUTEX_TAKE() \
    PTP_MUTEX_TAKE(slaveTable_mutex, BCM_PTP_SIGNAL_SLAVE_MUTEX_TIMEOUT_USEC)

#define BCM_PTP_SLAVETABLE_MUTEX_RELEASE_RETURN(__rv__) \
    PTP_MUTEX_RELEASE_RETURN(slaveTable_mutex, __rv__)

/* Types. */
typedef struct _bcm_ptp_msg_services_s {
    uint8 announce;
    uint8 sync;
    uint8 delayresp;
} _bcm_ptp_msg_services_t;

typedef struct signaled_master_s {
    sal_time_t announce_refresh_time, sync_refresh_time, delay_refresh_time;
    int8 log_announce_interval, log_sync_interval, log_delay_interval;
    int8 granted_log_sync_interval, granted_log_delay_interval;
    uint32 query_interval_sec;
    uint32 durationField;
    uint16 sequenceId;

    _bcm_ptp_msg_services_t service_enable;
    _bcm_ptp_msg_services_t service_grants;
    _bcm_ptp_msg_services_t to_ack_cancel;
    _bcm_ptp_msg_services_t unmet_requests;

    bcm_ptp_clock_peer_t master;
} signaled_master_t;

typedef struct _bcm_ptp_signaled_slave_s {
    sal_time_t announce_request_time;
    sal_time_t sync_request_time;
    sal_time_t delay_request_time;

    sal_time_t announce_grant_time;
    sal_time_t sync_grant_time;
    sal_time_t delay_grant_time;

    sal_time_t announce_cancel_time;
    sal_time_t sync_cancel_time;
    sal_time_t delay_cancel_time;

#if defined(BCM_PTP_SIGNAL_TIMESTAMP)
    sal_time_t announce_ack_time;
    sal_time_t sync_ack_time;
    sal_time_t delay_ack_time;

    sal_time_t announce_expire_time;
    sal_time_t sync_expire_time;
    sal_time_t delay_expire_time;
#endif

    uint8 announce_service_active;
    uint8 sync_service_active;
    uint8 delay_service_active;

    uint8 queue_announce_grant;
    uint8 queue_sync_grant;
    uint8 queue_delay_grant;

    uint8 announce_service_changed;
    uint8 sync_service_changed;
    uint8 delay_service_changed;

    uint8 queue_announce_cancel;
    uint8 queue_sync_cancel;
    uint8 queue_delay_cancel;

    uint8 queue_announce_expire;
    uint8 queue_sync_expire;
    uint8 queue_delay_expire;

    uint32 announce_duration;
    uint32 sync_duration;
    uint32 delay_duration;

    uint16 sequenceId;

    bcm_ptp_clock_peer_t slave;
} _bcm_ptp_signaled_slave_t;

typedef struct _bcm_ptp_signaled_slave_table_s {
    uint16 num_entries;
    _bcm_ptp_signaled_slave_t entry[PTP_MAX_UNICAST_SLAVE_TABLE_ENTRIES];
} _bcm_ptp_signaled_slave_table_t;

typedef struct unit_sig_info_s {
    bcm_ptp_signaling_arbiter_t arbiter;
    void * arbiter_user_data;
} unit_sig_info_t;

/* Variables. */
static int signaling_initialized = 0;
static int signaling_initialized_slave = 0;

static unit_sig_info_t unit_sig_array[BCM_MAX_NUM_UNITS] = {{0}};

static signaled_master_t signaled_master[PTP_MAX_UNICAST_MASTER_TABLE_ENTRIES] = {{0}};   
static volatile int num_signaled_masters = 0;  

static _bcm_ptp_signaled_slave_t signaled_slave_entry_default = {0};
static _bcm_ptp_signaled_slave_table_t signaled_slave_table;    
static _bcm_ptp_mutex_t slaveTable_mutex = 0x0;     

static bcm_ptp_clock_identity_t BCM_PTP_ALL_CLOCKS = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
static uint16 BCM_PTP_ANY_PORT = 0xFFFF;  /* From the standard: port 65535 => any/every port */

static uint32 request_uc_durationField = BCM_PTP_SIGNAL_REQUEST_UC_DEFAULT_DURATION_SEC;
static uint32 request_uc_durationField_min = BCM_PTP_SIGNAL_REQUEST_UC_MINIMUM_DURATION_SEC;
static uint32 request_uc_durationField_max = BCM_PTP_SIGNAL_REQUEST_UC_MAXIMUM_DURATION_SEC;

static int renewal_invited = BCM_PTP_SIGNAL_GRANT_UNICAST_RENEWAL_INVITED;

/* Static functions. */
static int send_unicast_request(int unit, bcm_ptp_stack_id_t ptp_id, int clock_num,
    int num_tlvs, int *type, int8 *log_inter_message_period, uint32 durationField,
    uint16 sequenceId, bcm_ptp_clock_peer_t *master);
static int _bcm_ptp_master_active_service_cancel(int unit, bcm_ptp_stack_id_t ptp_id,
    int clock_num, signaled_master_t *entry, bcm_ptp_message_type_t messageType);
static int _bcm_ptp_master_active_service_ack_cancel(int unit, bcm_ptp_stack_id_t ptp_id,
    int clock_num, signaled_master_t *entry, bcm_ptp_message_type_t messageType);
static void send_pending_signaling_to_masters(void *owner, void *arg_unit, void *arg_ptp_id,
                                              void *arg_clock_num, void *unused);
static void delayed_master_table_update(void *owner, void *arg_unit, void *arg_ptp_id,
                                        void *arg_clock_num, void *unused);

static int _bcm_ptp_slave_signaling_send(int unit, bcm_ptp_stack_id_t ptp_id,
    int clock_num, bcm_ptp_tlv_type_t tlv_type, bcm_ptp_message_type_t msg_type,
    int8 log_intermessage_period, uint32 duration, uint16 sequenceId, bcm_ptp_clock_peer_t *slave);
static int _bcm_ptp_slave_signaling_request_deny(int unit, bcm_ptp_stack_id_t ptp_id,
    int clock_num, bcm_ptp_message_type_t msg_type, int8 log_intermessage_period,
    bcm_ptp_clock_peer_t *slave);
static void _bcm_ptp_slave_signaling_gateway(void *owner, void *callback, void *arg_unit,
                                             void *arg_ptp_id, void *arg_clock_num);


/*
 * Function:
 *      _bcm_ptp_register_signaling_arbiter
 * Purpose:
 *      Register a callback function to accept or reject PTP signaling messages.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      arb       - (IN) PTP signaling message arbiter function pointer.
 *      user_data - (IN) User data.
 * Returns:
 *      BCM_E_XXX - Function status.
 */
int
_bcm_ptp_register_signaling_arbiter(
    int unit,
    bcm_ptp_signaling_arbiter_t arb,
    void *user_data)
{
    int rv = BCM_E_NONE;

    unit_sig_array[unit].arbiter = arb;
    unit_sig_array[unit].arbiter_user_data = user_data;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_unregister_signaling_arbiter
 * Purpose:
 *      Unregister a PTP signaling message arbiter.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX - Function status.
 */
int
_bcm_ptp_unregister_signaling_arbiter(
    int unit)
{
    int rv = BCM_E_NONE;

    unit_sig_array[unit].arbiter = NULL;
    unit_sig_array[unit].arbiter_user_data = NULL;

    return rv;
}


/*
 * Function:
 *      _bcm_ptp_signaling_init
 * Purpose:
 *      Initialize the BCM PTP signaling message framework.
 * Parameters:
 *      unit   - (IN) Unit number.
 *      ptp_id - (IN) PTP stack ID.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
_bcm_ptp_signaling_init(
    int unit,
    bcm_ptp_stack_id_t ptp_id)
{
    int rv;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_signaled_unicast_master_table_init(unit, ptp_id))) {
        PTP_ERROR_FUNC("_bcm_ptp_signaled_unicast_master_table_init()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_signaled_unicast_master_table_reset(unit, ptp_id))) {
        PTP_ERROR_FUNC("_bcm_ptp_signaled_unicast_master_table_reset()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_signaled_unicast_slave_table_init(unit, ptp_id))) {
        PTP_ERROR_FUNC("_bcm_ptp_signaled_unicast_slave_table_init()");
        return rv;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_signaling_manager
 * Purpose:
 *      Manage configurable attributes based on active PTP profile.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      en_telecom - (IN) Telecom profile enabled Boolean.
 * Returns:
 *      BCM_E_XXX - Function status.
 */
int
_bcm_ptp_signaling_manager(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int en_telecom)
{
    int rv;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    
    if (0 == en_telecom) {
        /* Standard profile. */
        request_uc_durationField = BCM_PTP_SIGNAL_REQUEST_UC_DEFAULT_DURATION_SEC;
        request_uc_durationField_min = BCM_PTP_SIGNAL_REQUEST_UC_MINIMUM_DURATION_SEC;
        request_uc_durationField_max = BCM_PTP_SIGNAL_REQUEST_UC_MAXIMUM_DURATION_SEC;

        renewal_invited = BCM_PTP_SIGNAL_GRANT_UNICAST_RENEWAL_INVITED;
    } else {
        /* Telecom profile. */
        request_uc_durationField = BCM_PTP_SIGNAL_TELECOM_REQUEST_UC_DEFAULT_DURATION_SEC;
        request_uc_durationField_min = BCM_PTP_SIGNAL_TELECOM_REQUEST_UC_MINIMUM_DURATION_SEC;
        request_uc_durationField_max = BCM_PTP_SIGNAL_TELECOM_REQUEST_UC_MAXIMUM_DURATION_SEC;

        renewal_invited = BCM_PTP_SIGNAL_TELECOM_GRANT_UNICAST_RENEWAL_INVITED;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_signaled_unicast_master_table_init
 * Purpose:
 *      Initialize the periodic check to see if signaling messages need to be sent to masters
 * Parameters:
 *      unit   - (IN) Unit number.
 *      ptp_id - (IN) PTP stack ID.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
_bcm_ptp_signaled_unicast_master_table_init(
    int unit,
    bcm_ptp_stack_id_t ptp_id)
{
    sal_dpc_cancel(INT_TO_PTR(&send_pending_signaling_to_masters));
    sal_dpc_time(BCM_PTP_SIGNAL_DEFAULT_MASTER_DPC_TIME_USEC,
                 &send_pending_signaling_to_masters, INT_TO_PTR(&send_pending_signaling_to_masters),
                 INT_TO_PTR(unit), INT_TO_PTR(ptp_id), INT_TO_PTR(PTP_CLOCK_NUMBER_DEFAULT), 0);

    request_uc_durationField = BCM_PTP_SIGNAL_REQUEST_UC_DEFAULT_DURATION_SEC;
    request_uc_durationField_min = BCM_PTP_SIGNAL_REQUEST_UC_MINIMUM_DURATION_SEC;
    request_uc_durationField_max = BCM_PTP_SIGNAL_REQUEST_UC_MAXIMUM_DURATION_SEC;

    renewal_invited = BCM_PTP_SIGNAL_GRANT_UNICAST_RENEWAL_INVITED;

    signaling_initialized = 1;
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_signaled_unicast_master_table_reset
 * Purpose:
 *      Reset signaled unicast master table.
 * Parameters:
 *      unit   - (IN) Unit number.
 *      ptp_id - (IN) PTP stack ID.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
_bcm_ptp_signaled_unicast_master_table_reset(
    int unit,
    bcm_ptp_stack_id_t ptp_id)
{
    num_signaled_masters = 0;
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_signaled_unicast_slave_table_init
 * Purpose:
 *      Initialize the signaled unicast slave table and framework to manage
 *      signaling messages sent to slaves.
 * Parameters:
 *      unit   - (IN) Unit number.
 *      ptp_id - (IN) PTP stack ID.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
_bcm_ptp_signaled_unicast_slave_table_init(
    int unit,
    bcm_ptp_stack_id_t ptp_id)
{
    sal_dpc_cancel(INT_TO_PTR(&_bcm_ptp_slave_signaling_gateway));
    sal_dpc_time(BCM_PTP_SIGNAL_DEFAULT_SLAVE_DPC_TIME_USEC,
                 &_bcm_ptp_slave_signaling_gateway, INT_TO_PTR(&_bcm_ptp_slave_signaling_gateway),
                 0, INT_TO_PTR(unit), INT_TO_PTR(ptp_id), INT_TO_PTR(PTP_CLOCK_NUMBER_DEFAULT));

    if (!slaveTable_mutex) {
        slaveTable_mutex = _bcm_ptp_mutex_create("signaled_slave_table");
    }

    if (signaling_initialized_slave) {
        /* Reset signaled unicast slave table. */
        int rv;

        if (BCM_FAILURE(rv = _bcm_ptp_signaled_unicast_slave_table_reset(unit, ptp_id))) {
            PTP_ERROR_FUNC("_bcm_ptp_signaled_unicast_slave_table_reset()");
            return rv;
        }
    }

    signaling_initialized_slave = 1;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_signaled_unicast_slave_table_reset
 * Purpose:
 *      Reset signaled unicast slave table.
 * Parameters:
 *      unit   - (IN) Unit number.
 *      ptp_id - (IN) PTP stack ID.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
_bcm_ptp_signaled_unicast_slave_table_reset(
    int unit,
    bcm_ptp_stack_id_t ptp_id)
{
    int i;

    signaled_slave_table.num_entries = 0;
    for (i = 0; i < PTP_MAX_UNICAST_SLAVE_TABLE_ENTRIES; ++i) {
        signaled_slave_table.entry[i] = signaled_slave_entry_default;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_common_ptp_signaled_unicast_master_add
 * Purpose:
 *      Add an entry to the signaled unicast master table.
 * Parameters:
 *      unit               - (IN) Unit number.
 *      ptp_id             - (IN) PTP stack ID.
 *      clock_num          - (IN) PTP clock number.
 *      port_num           - (IN) PTP clock port number.
 *      master_info        - (IN) Unicast master.
 *      flags              - (IN) Signaled unicast master configuration flags.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_signaled_unicast_master_add(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_unicast_master_t *master_info,
    uint32 flags)
{
    int rv;
    int idx;
    sal_time_t t;

    _bcm_ptp_port_state_t portState;

    SHR_BITDCL reqmask = flags;

    /* Telecom profile support. */
    bcm_ptp_clock_port_address_t address;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (!signaling_initialized) {
        _bcm_ptp_signaled_unicast_master_table_init(unit, ptp_id);
    }

    /* Do not add unicast master if portState is DISABLED. */
    if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_port_state_get(unit,
            ptp_id, clock_num, port_num, &portState))) {
        return rv;
    } else if (portState == _bcm_ptp_state_disabled) {
        return BCM_E_DISABLED;
    }
    
    /*
     * Scan signaled unicast master table for an entry with matching address.
     *    If match found, indexing logic shall replace existing entry.
     *    Otherwise, indexing logic shall append a new entry to table.
     */
    for (idx = 0; idx < num_signaled_masters; ++idx) {
        if (_bcm_ptp_peer_address_compare(&signaled_master[idx].master.peer_address,
                &master_info->address)) {
            break;
        }
    }

    if (idx == PTP_MAX_UNICAST_MASTER_TABLE_ENTRIES) {
        return BCM_E_FULL;
    }

    if (BCM_FAILURE(rv = bcm_ptp_static_unicast_master_add(unit, ptp_id, clock_num,
            port_num, master_info))) {
        return rv;
    }

    signaled_master[idx].master.peer_address = master_info->address;
    signaled_master[idx].master.local_port_number = port_num;
    signaled_master[idx].master.remote_port_number = BCM_PTP_ANY_PORT;
    sal_memcpy(signaled_master[idx].master.clock_identity,
               BCM_PTP_ALL_CLOCKS, sizeof(bcm_ptp_clock_identity_t));

    signaled_master[idx].log_announce_interval = master_info->log_announce_interval;
    signaled_master[idx].log_sync_interval = master_info->log_sync_interval;
    signaled_master[idx].log_delay_interval = master_info->log_min_delay_request_interval;

    /* Register PTP message services. */
    if (SHR_BITGET(&reqmask, PTP_SIGNALING_ANNOUNCE_SERVICE_BIT)) {
        signaled_master[idx].service_enable.announce = 1;
    } else {
        signaled_master[idx].service_enable.announce = 0;
    }
    if (SHR_BITGET(&reqmask, PTP_SIGNALING_SYNC_SERVICE_BIT)) {
        signaled_master[idx].service_enable.sync = 1;
    } else {
        signaled_master[idx].service_enable.sync = 0;
    }
    if (SHR_BITGET(&reqmask, PTP_SIGNALING_DELAYRESP_SERVICE_BIT)) {
        signaled_master[idx].service_enable.delayresp = 1;
    } else {
        signaled_master[idx].service_enable.delayresp = 0;
    }

    signaled_master[idx].service_grants.announce = 0;
    signaled_master[idx].service_grants.sync = 0;
    signaled_master[idx].service_grants.delayresp = 0;

    signaled_master[idx].to_ack_cancel.announce = 0;
    signaled_master[idx].to_ack_cancel.sync = 0;
    signaled_master[idx].to_ack_cancel.delayresp = 0;

    signaled_master[idx].unmet_requests.announce = 0;
    signaled_master[idx].unmet_requests.sync = 0;
    signaled_master[idx].unmet_requests.delayresp = 0;

    /* 2 seconds: plenty long to get signaling response */
    signaled_master[idx].granted_log_sync_interval = 1;
    signaled_master[idx].granted_log_delay_interval = 1;

    t = _bcm_ptp_monotonic_time();
    signaled_master[idx].announce_refresh_time = t;
    signaled_master[idx].sync_refresh_time = t;
    signaled_master[idx].delay_refresh_time = t;

    if (master_info->log_query_interval < 0) {
        signaled_master[idx].query_interval_sec = 1;
    } else {
        signaled_master[idx].query_interval_sec = (1 << master_info->log_query_interval);
    }

    signaled_master[idx].durationField = request_uc_durationField;
    signaled_master[idx].sequenceId = 0;

    if (idx == num_signaled_masters) {
        num_signaled_masters++;
    }

    /*
     * Telecom profile support.
     * Register signaled unicast master in packet master array.
     */
    _bcm_ptp_peer_address_convert(&master_info->address, &address);
    if (BCM_FAILURE(rv = bcm_ptp_telecom_g8265_packet_master_add(unit, ptp_id,
            clock_num, port_num, &address))) {
        return rv;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_common_ptp_signaled_unicast_master_remove
 * Purpose:
 *      Remove an entry from the signaled unicast master table.
 * Parameters:
 *      unit        - (IN) Unit number.
 *      ptp_id      - (IN) PTP stack ID.
 *      clock_num   - (IN) PTP clock number.
 *      port_num    - (IN) PTP clock port number.
 *      master_info - (IN) Unicast master.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_signaled_unicast_master_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_peer_address_t *address)
{
    int status;
    int idx = 0;
    int rv;

    /* Telecom profile support. */
    bcm_ptp_clock_port_address_t profile_address;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, port_num))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    status = bcm_ptp_static_unicast_master_remove(unit, ptp_id, clock_num, port_num, address);
    if (status != BCM_E_NONE) {
        return status;
    }

    while (idx < num_signaled_masters) {
        if (_bcm_ptp_peer_address_compare(&signaled_master[idx].master.peer_address,
                address)) {
            /* Cancel active unicast message services. */
            _bcm_ptp_master_active_service_cancel(unit, ptp_id, clock_num,
                &signaled_master[idx], bcmPTP_MESSAGE_TYPE_ANNOUNCE);
            _bcm_ptp_master_active_service_cancel(unit, ptp_id, clock_num,
                &signaled_master[idx], bcmPTP_MESSAGE_TYPE_SYNC);
            _bcm_ptp_master_active_service_cancel(unit, ptp_id, clock_num,
                &signaled_master[idx], bcmPTP_MESSAGE_TYPE_DELAY_RESP);

            /* Swap the last entry into this place, then decrement total number.  Could be in place... */
            signaled_master[idx] = signaled_master[--num_signaled_masters];
        } else {
            ++idx;
        }
    }

    /*
     * Telecom profile support.
     * Unregister signaled unicast master in packet master array.
     */
    _bcm_ptp_peer_address_convert(address, &profile_address);
    if (BCM_FAILURE(rv = bcm_ptp_telecom_g8265_packet_master_remove(unit, ptp_id,
            clock_num, port_num, &profile_address))) {
        return rv;
    }

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_ptp_signaled_unicast_slave_list
 * Purpose:
 *      List the signaled unicast slaves.
 * Parameters:
 *      unit           - (IN)  Unit number.
 *      ptp_id         - (IN)  PTP stack ID.
 *      clock_num      - (IN)  PTP clock number.
 *      port_num       - (IN)  PTP clock port number.
 *      max_num_slaves - (IN)  Maximum number of signaled unicast slaves.
 *      num_slaves     - (OUT) Number of signaled unicast slaves.
 *      slave_table    - (OUT) Signaled unicast slave table.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_signaled_unicast_slave_list (
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    int max_num_slaves,
    int *num_slaves,
    bcm_ptp_clock_peer_t *slave_table)
{
    int i;
    int rv;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, port_num))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    BCM_PTP_SLAVETABLE_MUTEX_TAKE();

    if (signaled_slave_table.num_entries > max_num_slaves) {
        /* Insufficient caller-provided table size for result. */
        *num_slaves = 0;
        BCM_PTP_SLAVETABLE_MUTEX_RELEASE_RETURN(BCM_E_RESOURCE);
    }

    *num_slaves = signaled_slave_table.num_entries;

    for (i = 0; i < (*num_slaves); ++i) {
        slave_table[i] = signaled_slave_table.entry[i].slave;
    }

    BCM_PTP_SLAVETABLE_MUTEX_RELEASE_RETURN(rv);
}


/*
 * Function:
 */
static int
_bcm_ptp_signaled_unicast_slave_exists(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    bcm_ptp_clock_peer_t *slave,
    int *index)
{
    int i;
    for (i = 0; i < signaled_slave_table.num_entries; ++i) {
        if (_bcm_ptp_peer_address_compare(&signaled_slave_table.entry[i].slave.peer_address,
                &slave->peer_address)) {
            /* Entry exists. */
            *index = i;
            return BCM_E_NONE;
        }
    }

    *index = -1;
    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 */
static int
_bcm_ptp_signaled_unicast_slave_add(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    bcm_ptp_clock_peer_t *slave)
{
    int i;
    int rv;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (signaled_slave_table.num_entries == PTP_MAX_UNICAST_SLAVE_TABLE_ENTRIES) {
        /* Signaled unicast slave table full. */
        return BCM_E_FULL;
    }

    /* Add entry. */
    i = signaled_slave_table.num_entries++;
    signaled_slave_table.entry[i] = signaled_slave_entry_default;
    signaled_slave_table.entry[i].slave = *slave;

    return BCM_E_NONE;
}

/*
 * Function:
 */
static int
_bcm_ptp_signaled_unicast_slave_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    bcm_ptp_clock_peer_t *slave)
{
    int i = 0;
    int k = 0;
    int rv;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    while (i < signaled_slave_table.num_entries) {
        if (_bcm_ptp_peer_address_compare(&slave->peer_address,
                &signaled_slave_table.entry[i].slave.peer_address)) {
            /*
             * Remove entry.
             * Move last entry into this slot, and decrement number of entries.
             */
            signaled_slave_table.entry[i] =
                signaled_slave_table.entry[--signaled_slave_table.num_entries];

            /* Reset unused entry. */
            signaled_slave_table.entry[signaled_slave_table.num_entries] =
                signaled_slave_entry_default;

            ++k;
        } else {
            ++i;
        }
    }

    if (k > 0) {
        return BCM_E_NONE;
    } else {
        /* No matching peer address(es) in signaled unicast slave table. */
        return BCM_E_NOT_FOUND;
    }
}

/*
 * Function:
 */
static int
_bcm_ptp_signaled_unicast_slave_expire(
    int unit,
    bcm_ptp_stack_id_t ptp_id)
{
    int i;
    int rv;
    sal_time_t t = _bcm_ptp_monotonic_time();

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    /*
     * Tag signaled slave announce, sync, and delay response services that
     * are to expire.
     */
    for (i = 0; i < signaled_slave_table.num_entries; ++i) {
        if ((!signaled_slave_table.entry[i].announce_service_active) ||
            (signaled_slave_table.entry[i].queue_announce_grant) ||
            (signaled_slave_table.entry[i].announce_grant_time +
             signaled_slave_table.entry[i].announce_duration >= t)) {
            signaled_slave_table.entry[i].queue_announce_expire = 0;
        } else {
            signaled_slave_table.entry[i].queue_announce_expire = 1;
        }

        if ((!signaled_slave_table.entry[i].sync_service_active) ||
            (signaled_slave_table.entry[i].queue_sync_grant) ||
            (signaled_slave_table.entry[i].sync_grant_time +
             signaled_slave_table.entry[i].sync_duration >= t)) {
            signaled_slave_table.entry[i].queue_sync_expire = 0;
        } else {
            signaled_slave_table.entry[i].queue_sync_expire = 1;
        }

        if ((!signaled_slave_table.entry[i].delay_service_active) ||
            (signaled_slave_table.entry[i].queue_delay_grant) ||
            (signaled_slave_table.entry[i].delay_grant_time +
             signaled_slave_table.entry[i].delay_duration >= t)) {
            signaled_slave_table.entry[i].queue_delay_expire = 0;
        } else {
            signaled_slave_table.entry[i].queue_delay_expire = 1;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 */
static int
_bcm_ptp_signaled_unicast_slave_table_update(
    int unit,
    bcm_ptp_stack_id_t ptp_id)
{
    int i;
    int rv;
    int num_entries;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    /*
     * Remove inactive/expired slaves from signaled unicast slave table.
     * NOTICE: Process table in reverse given logic in removal function
     *         _bcm_ptp_signaled_unicast_slave_remove() to reorder entries.
     */
    num_entries = signaled_slave_table.num_entries;
    for (i = (num_entries-1); i >= 0; --i) {
        if ((signaled_slave_table.entry[i].queue_announce_grant == 0) &&
            (signaled_slave_table.entry[i].queue_sync_grant == 0) &&
            (signaled_slave_table.entry[i].queue_delay_grant == 0) &&
            (signaled_slave_table.entry[i].queue_announce_cancel == 0) &&
            (signaled_slave_table.entry[i].queue_sync_cancel == 0) &&
            (signaled_slave_table.entry[i].queue_delay_cancel == 0) &&
            (signaled_slave_table.entry[i].queue_announce_expire == 0) &&
            (signaled_slave_table.entry[i].queue_sync_expire == 0) &&
            (signaled_slave_table.entry[i].queue_delay_expire == 0) &&
            (signaled_slave_table.entry[i].announce_service_active == 0) &&
            (signaled_slave_table.entry[i].sync_service_active == 0) &&
            (signaled_slave_table.entry[i].delay_service_active == 0)) {
            /*
             * Remove slave.
             *    No pending unicast grant, cancel, or acknowledge cancel TLVs to send.
             *    All slave announce, sync, and delay services are inactive or expired.
             */
            bcm_ptp_clock_peer_t slave = signaled_slave_table.entry[i].slave;
            _bcm_ptp_signaled_unicast_slave_remove(unit, ptp_id, &slave);
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 */
static int
send_unicast_request(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int num_tlvs,
    int *type,
    int8 *log_inter_message_period,
    uint32 durationField,
    uint16 sequenceId,
    bcm_ptp_clock_peer_t *master)
{
    int rv = BCM_E_NONE;
    int i;

    bcm_ptp_port_identity_t local_port;
    bcm_ptp_clock_port_address_t master_addr;
    bcm_ptp_clock_info_t ci;
    bcm_ptp_clock_port_info_t pi;

    int tlvLen;
    int udpLen;
    int packet_len;
    uint8 udp[BCM_PTP_SIGNAL_MAX_UDP_PAYLOAD_SIZE_OCTETS] = {0};
    uint8 packet[BCM_PTP_SIGNAL_MAX_PKT_SIZE_OCTETS] = {0};
    uint8 tlv[BCM_PTP_SIGNAL_MAX_TLV_SIZE_OCTETS] = {0};
    uint8 multi_tlv_en = 0;
    uint8 domain;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (BCM_SUCCESS(rv = bcm_ptp_clock_get(unit, ptp_id, clock_num, &ci))) {
        multi_tlv_en = (SHR_BITGET(&ci.flags, PTP_CLOCK_FLAGS_SIGNALING_MULTI_TLV_BIT) ||
                        SHR_BITGET(&ci.flags, PTP_CLOCK_FLAGS_TELECOM_PROFILE_BIT));
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, master->local_port_number, &local_port))) {
        PTP_ERROR_FUNC("bcm_common_ptp_clock_port_identity_get()");
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_ptp_clock_port_info_get(unit, ptp_id, clock_num,
            local_port.port_number, &pi))) {
        PTP_ERROR_FUNC("bcm_ptp_clock_port_info_get()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_management_domain_get(unit, ptp_id, clock_num,
            &domain))) {
        PTP_ERROR_FUNC("_bcm_ptp_management_domain_get()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_construct_signaling_msg(&local_port,
            master->clock_identity, master->remote_port_number,
            domain, sequenceId, 0, 0, udp, &udpLen))) {
        PTP_ERROR_FUNC("_bcm_ptp_construct_signaling_msg()");
        return rv;
    }

    if (num_tlvs > 1 && multi_tlv_en == 0) {
        SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
            "%s() failed %s\n", __func__, "Multiple TLV support not enabled.\n"));
        return BCM_E_CONFIG;
    }

    for (i = 0; i < num_tlvs; ++i) {
        if (BCM_FAILURE(rv = _bcm_ptp_construct_signaling_request_tlv(type[i],
                log_inter_message_period[i], durationField, tlv, &tlvLen))) {
            PTP_ERROR_FUNC("_bcm_ptp_construct_signaling_request_tlv()");
            return rv;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_signaling_message_append_tlv(udp, &udpLen,
                tlv, tlvLen))) {
            PTP_ERROR_FUNC("_bcm_ptp_signaling_message_append_tlv()");
            return rv;
        }
    }

    if (BCM_FAILURE(rv = _bcm_ptp_construct_udp_packet(master, &pi.port_address,
            udp, udpLen, packet, &packet_len))) {
        PTP_ERROR_FUNC("_bcm_ptp_construct_udp_packet()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_peer_address_convert(&master->peer_address, &master_addr))) {
        PTP_ERROR_FUNC("_bcm_ptp_peer_address_convert()");
        return rv;
    }
    if (BCM_FAILURE(rv = _bcm_ptp_update_peer_counts(master->local_port_number, 0,
            &master_addr, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0))) {
        PTP_ERROR_FUNC("_bcm_ptp_update_peer_counts()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_tunnel_message_to_world(unit, ptp_id,
            packet_len, packet))) {
        PTP_ERROR_FUNC("_bcm_ptp_tunnel_message_to_world()");
        return rv;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_master_active_service_cancel
 * Purpose:
 *      Cancel unicast message service (|Announce|Sync|Delay_Resp|) of unicast master.
 * Parameters:
 *      unit        - (IN)     Unit number.
 *      ptp_id      - (IN)     PTP stack ID.
 *      clock_num   - (IN)     PTP clock number.
 *      entry       - (IN/OUT) Signaled unicast master table entry.
 *      messageType - (IN)     Unicast message service to cancel.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_master_active_service_cancel(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    signaled_master_t *entry,
    bcm_ptp_message_type_t messageType)
{
    int rv;

    bcm_ptp_port_identity_t local_port;
    bcm_ptp_clock_port_address_t master_addr;
    bcm_ptp_clock_port_info_t pi;
    uint8 domainNumber;

    uint8 tlv[BCM_PTP_SIGNAL_MAX_TLV_SIZE_OCTETS] = {0};
    uint8 udp[BCM_PTP_SIGNAL_MAX_UDP_PAYLOAD_SIZE_OCTETS] = {0};
    uint8 packet[BCM_PTP_SIGNAL_MAX_PKT_SIZE_OCTETS] = {0};
    int tlvLen;
    int udpLen;
    int packet_len;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, entry->master.local_port_number, &local_port))) {
        PTP_ERROR_FUNC("bcm_common_ptp_clock_port_identity_get()");
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_ptp_clock_port_info_get(unit, ptp_id, clock_num,
            local_port.port_number, &pi))) {
        PTP_ERROR_FUNC("bcm_ptp_clock_port_info_get()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_management_domain_get(unit, ptp_id, clock_num,
            &domainNumber))) {
        PTP_ERROR_FUNC("_bcm_ptp_management_domain_get()");
        return rv;
    }

    switch (messageType) {
    case bcmPTP_MESSAGE_TYPE_ANNOUNCE:
        if (0 == entry->service_grants.announce) {
            /* Do nothing. Announce service is not active. */
            return BCM_E_NONE;
        }
        entry->service_grants.announce = 0;
        break;
    case bcmPTP_MESSAGE_TYPE_SYNC:
        if (0 == entry->service_grants.sync) {
            /* Do nothing. Sync service is not active. */
            return BCM_E_NONE;
        }
        entry->service_grants.sync = 0;
        break;
    case bcmPTP_MESSAGE_TYPE_DELAY_RESP:
        if (0 == entry->service_grants.delayresp) {
            /* Do nothing. Delay_Resp service is not active. */
            return BCM_E_NONE;
        }
        entry->service_grants.delayresp = 0;
        break;
    default:
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_construct_signaling_cancel_tlv(messageType,
            tlv, &tlvLen))) {
        PTP_ERROR_FUNC("_bcm_ptp_construct_signaling_cancel_tlv()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_construct_signaling_msg(&local_port,
            entry->master.clock_identity, entry->master.remote_port_number,
            domainNumber, ++entry->sequenceId, tlv, tlvLen, udp, &udpLen))) {
        PTP_ERROR_FUNC("_bcm_ptp_construct_signaling_msg()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_construct_udp_packet(&entry->master, &pi.port_address,
            udp, udpLen, packet, &packet_len))) {
        PTP_ERROR_FUNC("_bcm_ptp_construct_udp_packet()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_peer_address_convert(&entry->master.peer_address, &master_addr))) {
        PTP_ERROR_FUNC("_bcm_ptp_peer_address_convert()");
        return rv;
    }
    if (BCM_FAILURE(rv = _bcm_ptp_update_peer_counts(entry->master.local_port_number, 0,
            &master_addr, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0))) {
        PTP_ERROR_FUNC("_bcm_ptp_update_peer_counts()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_tunnel_message_to_world(unit, ptp_id,
            packet_len, packet))) {
        PTP_ERROR_FUNC("_bcm_ptp_tunnel_message_to_world()");
        return rv;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_master_active_service_ack_cancel
 * Purpose:
 *      Acknowledge cancellation of (|Announce|Sync|Delay_Resp|) message service by unicast master.
 * Parameters:
 *      unit        - (IN)     Unit number.
 *      ptp_id      - (IN)     PTP stack ID.
 *      clock_num   - (IN)     PTP clock number.
 *      entry       - (IN/OUT) Signaled unicast master table entry.
 *      messageType - (IN)     Unicast message service to acknowledge cancellation.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_master_active_service_ack_cancel(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    signaled_master_t *entry,
    bcm_ptp_message_type_t messageType)
{
    int rv;

    bcm_ptp_port_identity_t local_port;
    bcm_ptp_clock_port_address_t master_addr;
    bcm_ptp_clock_port_info_t pi;
    uint8 domainNumber;

    uint8 tlv[BCM_PTP_SIGNAL_MAX_TLV_SIZE_OCTETS] = {0};
    uint8 udp[BCM_PTP_SIGNAL_MAX_UDP_PAYLOAD_SIZE_OCTETS] = {0};
    uint8 packet[BCM_PTP_SIGNAL_MAX_PKT_SIZE_OCTETS] = {0};
    int tlvLen;
    int udpLen;
    int packet_len;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, entry->master.local_port_number, &local_port))) {
        PTP_ERROR_FUNC("bcm_common_ptp_clock_port_identity_get()");
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_ptp_clock_port_info_get(unit, ptp_id, clock_num,
            local_port.port_number, &pi))) {
        PTP_ERROR_FUNC("bcm_ptp_clock_port_info_get()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_management_domain_get(unit, ptp_id, clock_num,
            &domainNumber))) {
        PTP_ERROR_FUNC("_bcm_ptp_management_domain_get()");
        return rv;
    }

    switch (messageType) {
    case bcmPTP_MESSAGE_TYPE_ANNOUNCE:
        if (0 == entry->to_ack_cancel.announce) {
            /* Do nothing. Announce service was not cancelled by master. */
            return BCM_E_NONE;
        }
        entry->to_ack_cancel.announce = 0;
        break;
    case bcmPTP_MESSAGE_TYPE_SYNC:
        if (0 == entry->to_ack_cancel.sync) {
            /* Do nothing. Sync service was not cancelled by master. */
            return BCM_E_NONE;
        }
        entry->to_ack_cancel.sync = 0;
        break;
    case bcmPTP_MESSAGE_TYPE_DELAY_RESP:
        if (0 == entry->to_ack_cancel.delayresp) {
            /* Do nothing. Delay_Resp service was not cancelled by master. */
            return BCM_E_NONE;
        }
        entry->to_ack_cancel.delayresp = 0;
        break;
    default:
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_construct_signaling_ack_cancel_tlv(messageType,
            tlv, &tlvLen))) {
        PTP_ERROR_FUNC("_bcm_ptp_construct_signaling_ack_cancel_tlv()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_construct_signaling_msg(&local_port,
            entry->master.clock_identity, entry->master.remote_port_number,
            domainNumber, ++entry->sequenceId, tlv, tlvLen, udp, &udpLen))) {
        PTP_ERROR_FUNC("_bcm_ptp_construct_signaling_msg()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_construct_udp_packet(&entry->master, &pi.port_address,
            udp, udpLen, packet, &packet_len))) {
        PTP_ERROR_FUNC("_bcm_ptp_construct_udp_packet()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_peer_address_convert(&entry->master.peer_address, &master_addr))) {
        PTP_ERROR_FUNC("_bcm_ptp_peer_address_convert()");
        return rv;
    }
    if (BCM_FAILURE(rv = _bcm_ptp_update_peer_counts(entry->master.local_port_number, 0,
            &master_addr, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0))) {
        PTP_ERROR_FUNC("_bcm_ptp_update_peer_counts()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_tunnel_message_to_world(unit, ptp_id,
            packet_len, packet))) {
        PTP_ERROR_FUNC("_bcm_ptp_tunnel_message_to_world()");
        return rv;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 */
static int
_bcm_ptp_slave_signaling_send(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_tlv_type_t tlv_type,
    bcm_ptp_message_type_t msg_type,
    int8 log_intermessage_period,
    uint32 duration,
    uint16 sequenceId,
    bcm_ptp_clock_peer_t *slave)
{
    bcm_ptp_port_identity_t local_port_id;
    bcm_ptp_clock_port_address_t slave_addr;
    bcm_ptp_clock_port_address_t local_port_addr;
    uint8 tlv[BCM_PTP_SIGNAL_MAX_TLV_SIZE_OCTETS];
    uint8 udp[BCM_PTP_SIGNAL_MAX_UDP_PAYLOAD_SIZE_OCTETS];
    uint8 packet[BCM_PTP_SIGNAL_MAX_PKT_SIZE_OCTETS];
    int packet_len;
    int udpLen;
    int tlvLen;
    int rv;
    uint8 domain;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, slave->local_port_number))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (BCM_FAILURE(bcm_common_ptp_clock_port_identity_get(unit, ptp_id, clock_num,
                                                        slave->local_port_number, &local_port_id))) {
        return BCM_E_PARAM;
    }

    local_port_addr = (_bcm_common_ptp_unit_array[unit]
                       .stack_array[ptp_id]
                       .clock_array[clock_num]
                       .port_info[slave->local_port_number - 1]
                       .port_address);

    switch (tlv_type) {
    case bcmPTPTlvTypeGrantUnicastTransmission:
        _bcm_ptp_construct_signaling_grant_tlv(msg_type, log_intermessage_period,
            duration, renewal_invited, tlv, &tlvLen);
        break;

    case bcmPTPTlvTypeCancelUnicastTransmission:
        _bcm_ptp_construct_signaling_cancel_tlv(msg_type, tlv, &tlvLen);
        break;

    case bcmPTPTlvTypeAckCancelUnicastTransmission:
        _bcm_ptp_construct_signaling_ack_cancel_tlv(msg_type, tlv, &tlvLen);
        break;

    default:
        SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
            "%s() failed %s\n", __func__, "Unknown or unsupported TLV type.\n"));
        return BCM_E_PARAM;
    }

    _bcm_ptp_management_domain_get(unit, ptp_id, clock_num, &domain);
    _bcm_ptp_construct_signaling_msg(&local_port_id, slave->clock_identity,
        slave->remote_port_number, domain, sequenceId, tlv, tlvLen, udp, &udpLen);

    _bcm_ptp_construct_udp_packet(slave, &local_port_addr, udp, udpLen, packet, &packet_len);

    if (BCM_FAILURE(rv = _bcm_ptp_peer_address_convert(&slave->peer_address, &slave_addr))) {
        PTP_ERROR_FUNC("_bcm_ptp_peer_address_convert()");
        return rv;
    }
    if (BCM_FAILURE(rv = _bcm_ptp_update_peer_counts(slave->local_port_number, 0, &slave_addr,
            0, 1, 0, 0, 0, 0, 0, 0, 1, 0))) {
        PTP_ERROR_FUNC("_bcm_ptp_update_peer_counts()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_tunnel_message_to_world(unit, ptp_id, packet_len, packet)))
    {
        SOC_DEBUG_PRINT((DK_ERR, "In %s, _bcm_ptp_tunnel_message_to_world failed\n", __func__));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      send_pending_signaling_to_masters
 * Purpose:
 *      Send any necessary signaling messages to masters (as DPC).
 * Parameters:
 *      owner         - (IN) DPC owner.
 *      arg_unit      - (IN) Unit number (as void*).
 *      arg_ptp_id    - (IN) PTP stack ID (as void*).
 *      arg_clock_num - (IN) PTP clock number (as void*).
 *      unused        - (IN) Unused.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static void
send_pending_signaling_to_masters(
    void *owner,
    void *arg_unit,
    void *arg_ptp_id,
    void *arg_clock_num,
    void *unused)
{
    int i;
    int rv;

    int unit = (size_t)arg_unit;
    bcm_ptp_stack_id_t ptp_id = (bcm_ptp_stack_id_t)(size_t)arg_ptp_id;
    int clock_num = (size_t)arg_clock_num;

    sal_time_t ref_time;
    int messageType[BCM_PTP_SIGNAL_MAX_TLV_PER_MESSAGE] = {0};
    int8 logInterMessagePeriod[BCM_PTP_SIGNAL_MAX_TLV_PER_MESSAGE] = {0};
    int num_tlvs = 0;
    uint8 multi_tlv_en = 0;
    uint8 telecom_en = 0;
    uint8 ack_cancel = 0;

    bcm_ptp_clock_info_t ci;
    _bcm_ptp_port_state_t portState;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");

        /* On error, retry at default DPC interval. */
        sal_dpc_time(BCM_PTP_SIGNAL_DEFAULT_MASTER_DPC_TIME_USEC,
                     &send_pending_signaling_to_masters,
                     INT_TO_PTR(&send_pending_signaling_to_masters),
                     arg_unit, arg_ptp_id, arg_clock_num, 0);

        return;
    }

    if (BCM_SUCCESS(rv = bcm_ptp_clock_get(unit, ptp_id, clock_num, &ci))) {
        telecom_en = SHR_BITGET(&ci.flags, PTP_CLOCK_FLAGS_TELECOM_PROFILE_BIT);
        multi_tlv_en = (SHR_BITGET(&ci.flags, PTP_CLOCK_FLAGS_SIGNALING_MULTI_TLV_BIT) ||
                        telecom_en);
    }

    /* Acknowledge any unanswered cancellations from master(s). */
    for (i = 0; i < num_signaled_masters; ++i) {
        if (signaled_master[i].to_ack_cancel.announce) {
            _bcm_ptp_master_active_service_ack_cancel(unit, ptp_id, clock_num,
                &signaled_master[i], bcmPTP_MESSAGE_TYPE_ANNOUNCE);
            ack_cancel = 1;
        } else if (signaled_master[i].to_ack_cancel.sync) {
            _bcm_ptp_master_active_service_ack_cancel(unit, ptp_id, clock_num,
                &signaled_master[i], bcmPTP_MESSAGE_TYPE_SYNC);
            ack_cancel = 1;
        } else if (signaled_master[i].to_ack_cancel.delayresp) {
            _bcm_ptp_master_active_service_ack_cancel(unit, ptp_id, clock_num,
                &signaled_master[i], bcmPTP_MESSAGE_TYPE_DELAY_RESP);
            ack_cancel = 1;
        }

        if (ack_cancel) {
            /*
             * Anti-flooding.
             * Re-call on a short interval to check for others.
             */
            sal_dpc_time(BCM_PTP_SIGNAL_MIN_MASTER_DPC_TIME_USEC,
                         &send_pending_signaling_to_masters,
                         INT_TO_PTR(&send_pending_signaling_to_masters),
                         arg_unit, arg_ptp_id, arg_clock_num, 0);

            return;
        }
    }

    for (i = 0; i < num_signaled_masters; ++i) {
        ref_time = _bcm_ptp_monotonic_time();

        /* Check local port state; do not send signaling messages if portState is DISABLED. */
        if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_port_state_get(unit, ptp_id, clock_num,
                signaled_master[i].master.local_port_number, &portState)) ||
                portState == _bcm_ptp_state_disabled) {
            continue;
        }

        if (signaled_master[i].service_enable.announce &&
            signaled_master[i].announce_refresh_time &&
            signaled_master[i].announce_refresh_time < ref_time) {
            messageType[num_tlvs] = bcmPTP_MESSAGE_TYPE_ANNOUNCE;
            logInterMessagePeriod[num_tlvs++] = signaled_master[i].log_announce_interval;

            /*
             * Increment counter of unanswered/denied Announce service requests.
             * Do not rollover.
             */
            if (signaled_master[i].unmet_requests.announce < ((uint8)-1)) {
                ++signaled_master[i].unmet_requests.announce;
            }

            /*
             * Set the time to send the next signaling message.
             * Will be overwritten if we get a master response.
             */
            if ((telecom_en == 0) ||
                (signaled_master[i].unmet_requests.announce <
                 BCM_PTP_SIGNAL_TELECOM_UNMET_REQUEST_MAX)) {
                signaled_master[i].announce_refresh_time = ref_time +
                    signaled_master[i].query_interval_sec;
            } else {
                /*
                 * Telecom profile support.
                 * Retry at 60 sec. intervals if three (3) unanswered or denied requests.
                 * Ref. ITU-T G.8265.1, Clause 6.6.
                 */
                signaled_master[i].announce_refresh_time = ref_time +
                    BCM_PTP_SIGNAL_TELECOM_UNMET_REQUEST_QUERY_INTERVAL_SEC;
            }

            if (multi_tlv_en == 0) {
                goto process_tlvs;
            }

        }

        if (signaled_master[i].service_grants.announce &&
            signaled_master[i].service_enable.sync &&
            signaled_master[i].sync_refresh_time &&
            signaled_master[i].sync_refresh_time < ref_time) {
            messageType[num_tlvs] = bcmPTP_MESSAGE_TYPE_SYNC;
            logInterMessagePeriod[num_tlvs++] = signaled_master[i].log_sync_interval;

            /*
             * Increment counter of unanswered/denied Sync service requests.
             * Do not rollover.
             */
            if (signaled_master[i].unmet_requests.sync < ((uint8)-1)) {
                ++signaled_master[i].unmet_requests.sync;
            }

            /*
             * Set the time to send the next signaling message.
             * Will be overwritten if we get a master response.
             */
            if ((telecom_en == 0) ||
                (signaled_master[i].unmet_requests.sync <
                 BCM_PTP_SIGNAL_TELECOM_UNMET_REQUEST_MAX)) {
                signaled_master[i].sync_refresh_time = ref_time +
                    signaled_master[i].query_interval_sec;
            } else {
                /*
                 * Telecom profile support.
                 * Retry at 60 sec. intervals if three (3) unanswered or denied requests.
                 * Ref. ITU-T G.8265.1, Clause 6.6.
                 */
                signaled_master[i].sync_refresh_time = ref_time +
                    BCM_PTP_SIGNAL_TELECOM_UNMET_REQUEST_QUERY_INTERVAL_SEC;
            }

            if (multi_tlv_en == 0) {
                goto process_tlvs;
            }

        }

        if (signaled_master[i].service_grants.announce &&
            signaled_master[i].service_enable.delayresp &&
            signaled_master[i].delay_refresh_time &&
            signaled_master[i].delay_refresh_time < ref_time) {
            messageType[num_tlvs] = bcmPTP_MESSAGE_TYPE_DELAY_RESP;
            logInterMessagePeriod[num_tlvs++] = signaled_master[i].log_delay_interval;

            /*
             * Increment counter of unanswered/denied Delay_Resp service requests.
             * Do not rollover.
             */
            if (signaled_master[i].unmet_requests.delayresp < ((uint8)-1)) {
                ++signaled_master[i].unmet_requests.delayresp;
            }

            /*
             * Set the time to send the next signaling message.
             * Will be overwritten if we get a master response.
             */
            if ((telecom_en == 0) ||
                (signaled_master[i].unmet_requests.delayresp <
                 BCM_PTP_SIGNAL_TELECOM_UNMET_REQUEST_MAX)) {
                signaled_master[i].delay_refresh_time = ref_time +
                    signaled_master[i].query_interval_sec;
            } else {
                /*
                 * Telecom profile support.
                 * Retry at 60 sec. intervals if three (3) unanswered or denied requests.
                 * Ref. ITU-T G.8265.1, Clause 6.6.
                 */
                signaled_master[i].delay_refresh_time = ref_time +
                    BCM_PTP_SIGNAL_TELECOM_UNMET_REQUEST_QUERY_INTERVAL_SEC;
            }
        }

process_tlvs:
        if (num_tlvs) {
            /* Pending signaling message(s) to send to a master. */
            send_unicast_request(unit, ptp_id, clock_num, num_tlvs, messageType,
                logInterMessagePeriod, signaled_master[i].durationField,
                ++signaled_master[i].sequenceId, &signaled_master[i].master);

            /*
             * Anti-flooding.
             * Re-call on a short interval to check for others.
             */
            sal_dpc_time(BCM_PTP_SIGNAL_MIN_MASTER_DPC_TIME_USEC,
                         &send_pending_signaling_to_masters,
                         INT_TO_PTR(&send_pending_signaling_to_masters),
                         arg_unit, arg_ptp_id, arg_clock_num, 0);

            return;
        }
    }

    /* No pending signaling messages; try again in 1 second. */
    sal_dpc_time(BCM_PTP_SIGNAL_DEFAULT_MASTER_DPC_TIME_USEC,
                 &send_pending_signaling_to_masters,
                 INT_TO_PTR(&send_pending_signaling_to_masters),
                 arg_unit, arg_ptp_id, arg_clock_num, 0);

    return;
}

/*
 * Function:
 *      _bcm_ptp_slave_signaling_request_deny
 * Purpose:
 *      Encapsulate system behavior to deny slave's request for PTP message
 *      services.
 * Parameters:
 *      unit                    - (IN) Unit number.
 *      ptp_id                  - (IN) PTP stack ID.
 *      clock_num               - (IN) PTP clock number.
 *      msg_type                - (IN) PTP message type (|Announce|Sync|DelayResp|).
 *      log_intermessage_period - (IN) logInterMessagePeriod.
 *      slave                   - (IN) Slave data.
 * Returns:
 *      BCM_E_XXX - Function status.
 */
static int
_bcm_ptp_slave_signaling_request_deny(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_message_type_t msg_type,
    int8 log_intermessage_period,
    bcm_ptp_clock_peer_t *slave)
{
    int rv;
    int i;

    BCM_PTP_SLAVETABLE_MUTEX_TAKE();

    /* Signaled unicast slave lookup. */
    if (BCM_FAILURE(_bcm_ptp_signaled_unicast_slave_exists(unit, ptp_id, slave, &i))) {
        /*
         * Unknown slave.
         * Send zero duration GRANT_UNICAST_TRANSMISSION to deny request.
         * Ref. IEEE Std. 1588-2008, Sect. 16.1.4.2.5.
         */
        if (BCM_FAILURE(rv = _bcm_ptp_slave_signaling_send(unit, ptp_id, clock_num,
                bcmPTPTlvTypeGrantUnicastTransmission, msg_type,
                log_intermessage_period, BCM_PTP_SIGNAL_REQUEST_DENY_DURATION_SEC,
                1, slave))) {
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                            "%s() failed %s\n", __func__,
                            "Zero duration GRANT_UNICAST_TRANSMISSION error.\n"));
            BCM_PTP_SLAVETABLE_MUTEX_RELEASE_RETURN(rv);
        }

        /* Return. No matching entry in signaled unicast slave table. */
        BCM_PTP_SLAVETABLE_MUTEX_RELEASE_RETURN(BCM_E_NONE);
    }

    /*
     * Send zero duration GRANT_UNICAST_TRANSMISSION to deny request.
     * Ref. IEEE Std. 1588-2008, Sect. 16.1.4.2.5.
     */
    if (BCM_FAILURE(rv = _bcm_ptp_slave_signaling_send(unit, ptp_id, clock_num,
            bcmPTPTlvTypeGrantUnicastTransmission, msg_type,
            log_intermessage_period, BCM_PTP_SIGNAL_REQUEST_DENY_DURATION_SEC,
            ++signaled_slave_table.entry[i].sequenceId, slave))) {
        SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                        "%s() failed %s\n", __func__,
                        "Zero duration GRANT_UNICAST_TRANSMISSION error.\n"));
        BCM_PTP_SLAVETABLE_MUTEX_RELEASE_RETURN(rv);
    }

    /*
     * Signaled unicast slave table management.
     * Configure table to cancel corresponding PTP message service if slave
     * is currently subscribed to the service.
     */
    switch (msg_type) {
    case bcmPTP_MESSAGE_TYPE_ANNOUNCE:
        if (signaled_slave_table.entry[i].announce_service_active) {
            signaled_slave_table.entry[i].queue_announce_grant = 0;
            signaled_slave_table.entry[i].queue_announce_expire = 1;
        }
        if (signaled_slave_table.entry[i].sync_service_active) {
            signaled_slave_table.entry[i].queue_sync_grant = 0;
            signaled_slave_table.entry[i].queue_sync_expire = 1;
        }
        if (signaled_slave_table.entry[i].delay_service_active) {
            signaled_slave_table.entry[i].queue_delay_grant = 0;
            signaled_slave_table.entry[i].queue_delay_expire = 1;
        }
        break;

    case bcmPTP_MESSAGE_TYPE_SYNC:
        if (signaled_slave_table.entry[i].sync_service_active) {
            signaled_slave_table.entry[i].queue_sync_grant = 0;
            signaled_slave_table.entry[i].queue_sync_expire = 1;
        }
        break;

    case bcmPTP_MESSAGE_TYPE_DELAY_RESP:
        if (signaled_slave_table.entry[i].delay_service_active) {
            signaled_slave_table.entry[i].queue_delay_grant = 0;
            signaled_slave_table.entry[i].queue_delay_expire = 1;
        }
        break;

    default:
        ;
    }

    _bcm_ptp_mutex_give(slaveTable_mutex);

    /* Call signaled-slave DPC so changes take place immediately. */
    sal_dpc_cancel(INT_TO_PTR(&_bcm_ptp_slave_signaling_gateway));
    sal_dpc_time(0, &_bcm_ptp_slave_signaling_gateway,
                 INT_TO_PTR(&_bcm_ptp_slave_signaling_gateway),
                 0, INT_TO_PTR(unit), INT_TO_PTR(ptp_id), INT_TO_PTR(clock_num));

    return rv;
}

/*
 * Function:
 */
static void
_bcm_ptp_slave_signaling_gateway(
    void *owner,
    void *callback,
    void *arg_unit,
    void *arg_ptp_id,
    void *arg_clock_num)
{
    int rv = BCM_E_NONE;
    int i;
    int isqueued = 0;
    bcm_ptp_tlv_type_t tlvType = 0;
    bcm_ptp_tlv_type_t tlvTypeResp = 0;
    bcm_ptp_message_type_t messageType = 0;
    int8 logInterMessagePeriod = 0;
    uint32 durationField = 0;
    bcm_ptp_clock_peer_t slave;
    int top_needs_updating = 0;
    int unit = (size_t)arg_unit;
    int clock_num = (size_t)arg_clock_num;
    bcm_ptp_stack_id_t ptp_id = (bcm_ptp_stack_id_t)(size_t)arg_ptp_id;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        /* Retry at default DPC interval. */
        sal_dpc_time(BCM_PTP_SIGNAL_DEFAULT_SLAVE_DPC_TIME_USEC,
                     &_bcm_ptp_slave_signaling_gateway, INT_TO_PTR(&_bcm_ptp_slave_signaling_gateway),
                     0, INT_TO_PTR(unit), INT_TO_PTR(ptp_id), INT_TO_PTR(clock_num));
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return;
    }

    if ((rv = _bcm_ptp_mutex_take(slaveTable_mutex, BCM_PTP_SIGNAL_SLAVE_MUTEX_TIMEOUT_USEC))) {
        /* Retry at default DPC interval. */
        sal_dpc_time(BCM_PTP_SIGNAL_DEFAULT_SLAVE_DPC_TIME_USEC,
                     &_bcm_ptp_slave_signaling_gateway, INT_TO_PTR(&_bcm_ptp_slave_signaling_gateway),
                     0, INT_TO_PTR(unit), INT_TO_PTR(ptp_id), INT_TO_PTR(clock_num));
        PTP_ERROR_FUNC("_bcm_ptp_mutex_take()");
        return;
    }

    for (i = 0; i < signaled_slave_table.num_entries; ++i)
    {
        /*
         * Service one signaling message per DPC to avoid flooding.
         * Configure DPC with short pause to send next signaling message, if any.
         */
        if (signaled_slave_table.entry[i].queue_announce_grant) {
            /*
             * Defer processing of announce grant iff pending cancel
             * announce TLV precedes request announce TLV.
             */
            if ((signaled_slave_table.entry[i].announce_request_time <=
                 signaled_slave_table.entry[i].announce_cancel_time) ||
                (!signaled_slave_table.entry[i].queue_announce_cancel)) {

                top_needs_updating = signaled_slave_table.entry[i].announce_service_changed;
                signaled_slave_table.entry[i].announce_service_changed = 0;

                signaled_slave_table.entry[i].queue_announce_grant = 0;
                signaled_slave_table.entry[i].announce_service_active = 1;
                signaled_slave_table.entry[i].announce_grant_time = _bcm_ptp_monotonic_time();

                /* Select message to service. */
                tlvType = bcmPTPTlvTypeRequestUnicastTransmission;
                tlvTypeResp = bcmPTPTlvTypeGrantUnicastTransmission;
                messageType = bcmPTP_MESSAGE_TYPE_ANNOUNCE;
                logInterMessagePeriod = signaled_slave_table.entry[i].slave.log_announce_interval;
                durationField = signaled_slave_table.entry[i].announce_duration;
                slave = signaled_slave_table.entry[i].slave;

                isqueued = 1;
                break;
            }
        }

        if (signaled_slave_table.entry[i].queue_sync_grant) {
            /*
             * Defer processing of sync grant iff pending cancel
             * sync TLV precedes request sync TLV.
             */
            if ((signaled_slave_table.entry[i].sync_request_time <=
                 signaled_slave_table.entry[i].sync_cancel_time) ||
                (!signaled_slave_table.entry[i].queue_sync_cancel)) {

                top_needs_updating = signaled_slave_table.entry[i].sync_service_changed;
                signaled_slave_table.entry[i].sync_service_changed = 0;

                signaled_slave_table.entry[i].queue_sync_grant = 0;
                signaled_slave_table.entry[i].sync_service_active = 1;
                signaled_slave_table.entry[i].sync_grant_time = _bcm_ptp_monotonic_time();

                /* Select message to service. */
                tlvType = bcmPTPTlvTypeRequestUnicastTransmission;
                tlvTypeResp = bcmPTPTlvTypeGrantUnicastTransmission;
                messageType = bcmPTP_MESSAGE_TYPE_SYNC;
                logInterMessagePeriod = signaled_slave_table.entry[i].slave.log_sync_interval;
                durationField = signaled_slave_table.entry[i].sync_duration;
                slave = signaled_slave_table.entry[i].slave;

                isqueued = 1;
                break;
            }
        }

        if (signaled_slave_table.entry[i].queue_delay_grant) {
            /*
             * Defer processing of delay response grant iff pending cancel
             * delay response TLV precedes request delay response TLV.
             */
            if ((signaled_slave_table.entry[i].delay_request_time <=
                 signaled_slave_table.entry[i].delay_cancel_time) ||
                (!signaled_slave_table.entry[i].queue_delay_cancel)) {

                top_needs_updating = signaled_slave_table.entry[i].delay_service_changed;
                signaled_slave_table.entry[i].delay_service_changed = 0;

                signaled_slave_table.entry[i].queue_delay_grant = 0;
                signaled_slave_table.entry[i].delay_service_active = 1;
                signaled_slave_table.entry[i].delay_grant_time = _bcm_ptp_monotonic_time();

                /* Select message to service. */
                tlvType = bcmPTPTlvTypeRequestUnicastTransmission;
                tlvTypeResp = bcmPTPTlvTypeGrantUnicastTransmission;
                messageType = bcmPTP_MESSAGE_TYPE_DELAY_RESP;
                logInterMessagePeriod = signaled_slave_table.entry[i].slave.log_delay_request_interval;
                durationField = signaled_slave_table.entry[i].delay_duration;
                slave = signaled_slave_table.entry[i].slave;

                isqueued = 1;
                break;
            }
        }

        if (signaled_slave_table.entry[i].queue_announce_cancel) {
            signaled_slave_table.entry[i].queue_announce_cancel = 0;
            signaled_slave_table.entry[i].announce_service_active = 0;
            signaled_slave_table.entry[i].slave.log_announce_interval = 99;
#if defined(BCM_PTP_SIGNAL_TIMESTAMP)
            signaled_slave_table.entry[i].announce_ack_time = _bcm_ptp_monotonic_time();
#endif

            top_needs_updating = 1;

            /* Select message to service. */
            tlvType = bcmPTPTlvTypeCancelUnicastTransmission;
            tlvTypeResp = bcmPTPTlvTypeAckCancelUnicastTransmission;
            messageType = bcmPTP_MESSAGE_TYPE_ANNOUNCE;
            logInterMessagePeriod = signaled_slave_table.entry[i].slave.log_announce_interval;
            durationField = signaled_slave_table.entry[i].announce_duration;
            slave = signaled_slave_table.entry[i].slave;

            isqueued = 1;
            break;
        }

        if (signaled_slave_table.entry[i].queue_sync_cancel) {
            signaled_slave_table.entry[i].queue_sync_cancel = 0;
            signaled_slave_table.entry[i].sync_service_active = 0;
            signaled_slave_table.entry[i].slave.log_sync_interval = 99;
#if defined(BCM_PTP_SIGNAL_TIMESTAMP)
            signaled_slave_table.entry[i].sync_ack_time = _bcm_ptp_monotonic_time();
#endif

            top_needs_updating = 1;

            /* Select message to service. */
            tlvType = bcmPTPTlvTypeCancelUnicastTransmission;
            tlvTypeResp = bcmPTPTlvTypeAckCancelUnicastTransmission;
            messageType = bcmPTP_MESSAGE_TYPE_SYNC;
            logInterMessagePeriod = signaled_slave_table.entry[i].slave.log_sync_interval;
            durationField = signaled_slave_table.entry[i].sync_duration;
            slave = signaled_slave_table.entry[i].slave;

            isqueued = 1;
            break;
        }

        if (signaled_slave_table.entry[i].queue_delay_cancel) {
            signaled_slave_table.entry[i].queue_delay_cancel = 0;
            signaled_slave_table.entry[i].delay_service_active = 0;
            signaled_slave_table.entry[i].slave.log_delay_request_interval = 99;
#if defined(BCM_PTP_SIGNAL_TIMESTAMP)
            signaled_slave_table.entry[i].delay_ack_time = _bcm_ptp_monotonic_time();
#endif

            top_needs_updating = 1;

            /* Select message to service. */
            tlvType = bcmPTPTlvTypeCancelUnicastTransmission;
            tlvTypeResp = bcmPTPTlvTypeAckCancelUnicastTransmission;
            messageType = bcmPTP_MESSAGE_TYPE_DELAY_RESP;
            logInterMessagePeriod = signaled_slave_table.entry[i].slave.log_delay_request_interval;
            durationField = signaled_slave_table.entry[i].delay_duration;
            slave = signaled_slave_table.entry[i].slave;

            isqueued = 1;
            break;
        }

        if (signaled_slave_table.entry[i].queue_announce_expire) {
            signaled_slave_table.entry[i].queue_announce_expire = 0;
            signaled_slave_table.entry[i].announce_service_active = 0;
            signaled_slave_table.entry[i].slave.log_announce_interval = 99;
#if defined(BCM_PTP_SIGNAL_TIMESTAMP)
            signaled_slave_table.entry[i].announce_expire_time = _bcm_ptp_monotonic_time();
#endif

            top_needs_updating = 1;

            /* Select message to service. */
            tlvType = bcmPTPTlvTypeCancelUnicastTransmission;
            tlvTypeResp = bcmPTPTlvTypeCancelUnicastTransmission;
            messageType = bcmPTP_MESSAGE_TYPE_ANNOUNCE;
            logInterMessagePeriod = signaled_slave_table.entry[i].slave.log_announce_interval;
            durationField = signaled_slave_table.entry[i].announce_duration;
            slave = signaled_slave_table.entry[i].slave;

            isqueued = 1;
            break;
        }

        if (signaled_slave_table.entry[i].queue_sync_expire) {
            signaled_slave_table.entry[i].queue_sync_expire = 0;
            signaled_slave_table.entry[i].sync_service_active = 0;
            signaled_slave_table.entry[i].slave.log_sync_interval = 99;
#if defined(BCM_PTP_SIGNAL_TIMESTAMP)
            signaled_slave_table.entry[i].sync_expire_time = _bcm_ptp_monotonic_time();
#endif

            top_needs_updating = 1;

            /* Select message to service. */
            tlvType = bcmPTPTlvTypeCancelUnicastTransmission;
            tlvTypeResp = bcmPTPTlvTypeCancelUnicastTransmission;
            messageType = bcmPTP_MESSAGE_TYPE_SYNC;
            logInterMessagePeriod = signaled_slave_table.entry[i].slave.log_sync_interval;
            durationField = signaled_slave_table.entry[i].sync_duration;
            slave = signaled_slave_table.entry[i].slave;

            isqueued = 1;
            break;
        }

        if (signaled_slave_table.entry[i].queue_delay_expire) {
            signaled_slave_table.entry[i].queue_delay_expire = 0;
            signaled_slave_table.entry[i].delay_service_active = 0;
            signaled_slave_table.entry[i].slave.log_delay_request_interval = 99;
#if defined(BCM_PTP_SIGNAL_TIMESTAMP)
            signaled_slave_table.entry[i].delay_expire_time = _bcm_ptp_monotonic_time();
#endif

            top_needs_updating = 1;

            /* Select message to service. */
            tlvType = bcmPTPTlvTypeCancelUnicastTransmission;
            tlvTypeResp = bcmPTPTlvTypeCancelUnicastTransmission;
            messageType = bcmPTP_MESSAGE_TYPE_DELAY_RESP;
            logInterMessagePeriod = signaled_slave_table.entry[i].slave.log_delay_request_interval;
            durationField = signaled_slave_table.entry[i].delay_duration;
            slave = signaled_slave_table.entry[i].slave;

            isqueued = 1;
            break;
        }
    }

    if (isqueued) {
        /* Queued signaling message(s). */

        /*
         * Release mutex.
         * No additional DPC modifications to signaled unicast slave table.
         */
        _bcm_ptp_mutex_give(slaveTable_mutex);

        /* Respond to (grant, cancel, acknowledge cancel) selected message. */
        if (BCM_FAILURE(rv = _bcm_ptp_slave_signaling_send(unit, ptp_id, clock_num, tlvTypeResp,
                messageType, logInterMessagePeriod, durationField,
                ++signaled_slave_table.entry[i].sequenceId,&slave))) {
            PTP_ERROR_FUNC("_bcm_ptp_slave_signaling_send()");
        }

        if (top_needs_updating) {
            /* Subscribe slave. */
            if (BCM_FAILURE(rv = _bcm_ptp_unicast_slave_subscribe(unit, ptp_id, clock_num,
                    BCM_PTP_ANY_PORT, &slave, messageType, tlvType, logInterMessagePeriod))) {
                PTP_ERROR_FUNC("_bcm_ptp_unicast_slave_subscribe()");
            }
        }

        /* Re-call gateway at minimum DPC interval to service next message, if any. */
        sal_dpc_time(BCM_PTP_SIGNAL_MIN_SLAVE_DPC_TIME_USEC,
                     &_bcm_ptp_slave_signaling_gateway, INT_TO_PTR(&_bcm_ptp_slave_signaling_gateway),
                     0, INT_TO_PTR(unit), INT_TO_PTR(ptp_id), INT_TO_PTR(clock_num));

    } else {
        /* No queued signaling messages. */

        /* Update indicators of expired slave services. */
        if (BCM_FAILURE(rv = _bcm_ptp_signaled_unicast_slave_expire(unit, ptp_id))) {
            PTP_ERROR_FUNC("_bcm_ptp_signaled_unicast_slave_expire()");
        }

        /* Update signaled unicast slave table to remove inactive/expired slaves. */
        if (BCM_FAILURE(rv = _bcm_ptp_signaled_unicast_slave_table_update(unit, ptp_id))) {
            PTP_ERROR_FUNC("_bcm_ptp_signaled_unicast_slave_table_update()");
        }

        /*
         * Release mutex.
         * No additional DPC modifications to signaled unicast slave table.
         */
        _bcm_ptp_mutex_give(slaveTable_mutex);

        /* Retry at default DPC interval. */
        sal_dpc_time(BCM_PTP_SIGNAL_DEFAULT_SLAVE_DPC_TIME_USEC,
                     &_bcm_ptp_slave_signaling_gateway, INT_TO_PTR(&_bcm_ptp_slave_signaling_gateway),
                     0, INT_TO_PTR(unit), INT_TO_PTR(ptp_id), INT_TO_PTR(clock_num));
    }

    return;
}

/*
 * Function:
 *      bcm_common_ptp_signaled_unicast_slave_table_clear()
 * Purpose:
 *      Cancel active unicast services for slaves and clear signaled unicast
 *      slave table entries that correspond to caller-specified master port.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      port_num  - (IN) Local (master) PTP clock port number.
 *      callstack - (IN) Boolean to unsubscribe/unregister unicast services.
 *                       TRUE : Cancellation message to slave and PTP stack.
 *                       FALSE: Cancellation message tunneled to slave only.
 * Returns:
 *      BCM_E_XXX - Function status.
 */
int
bcm_common_ptp_signaled_unicast_slave_table_clear(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    int callstack)
{
    int rv = BCM_E_NONE;
    int i;
    bcm_ptp_tlv_type_t tlvType;
    bcm_ptp_message_type_t messageType;
    int8 logInterMessagePeriod;
    uint32 durationField;
    bcm_ptp_clock_peer_t slave;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    BCM_PTP_SLAVETABLE_MUTEX_TAKE();

    for (i = 0; i < signaled_slave_table.num_entries; ++i)
    {
       tlvType = bcmPTPTlvTypeCancelUnicastTransmission;
       slave = signaled_slave_table.entry[i].slave;

       /* Remove slaves associated with specified master port. */
       if (port_num == slave.local_port_number ||
           port_num == PTP_IEEE1588_ALL_PORTS) {
           if (signaled_slave_table.entry[i].announce_service_active) {
                /* Select message to service. */
                messageType = bcmPTP_MESSAGE_TYPE_ANNOUNCE;
                logInterMessagePeriod =  signaled_slave_table.entry[i].slave.log_announce_interval;
                durationField =  signaled_slave_table.entry[i].announce_duration;

                /* Cancel message. */
                if (BCM_FAILURE(rv = _bcm_ptp_slave_signaling_send(unit, ptp_id, clock_num, tlvType,
                        messageType, logInterMessagePeriod, durationField,
                        ++signaled_slave_table.entry[i].sequenceId, &slave))) {
                    PTP_ERROR_FUNC("_bcm_ptp_slave_signaling_send()");
                }

                /* Unsubscribe slave. */
                if (callstack &&
                    BCM_FAILURE(rv = _bcm_ptp_unicast_slave_subscribe(unit, ptp_id,
                       clock_num, slave.local_port_number, &slave, messageType, tlvType,
                       logInterMessagePeriod))) {
                    PTP_ERROR_FUNC("_bcm_ptp_unicast_slave_subscribe()");
                }
            }

            if (signaled_slave_table.entry[i].sync_service_active) {
                /* Select message to service. */
                messageType = bcmPTP_MESSAGE_TYPE_SYNC;
                logInterMessagePeriod =  signaled_slave_table.entry[i].slave.log_sync_interval;
                durationField =  signaled_slave_table.entry[i].sync_duration;

                /* Cancel message. */
                if (BCM_FAILURE(rv = _bcm_ptp_slave_signaling_send(unit, ptp_id, clock_num, tlvType,
                        messageType, logInterMessagePeriod, durationField,
                        ++signaled_slave_table.entry[i].sequenceId, &slave))) {
                    PTP_ERROR_FUNC("_bcm_ptp_slave_signaling_send()");
                }

                /* Unsubscribe slave. */
                if (callstack &&
                    BCM_FAILURE(rv = _bcm_ptp_unicast_slave_subscribe(unit, ptp_id,
                        clock_num, slave.local_port_number, &slave, messageType, tlvType,
                        logInterMessagePeriod))) {
                    PTP_ERROR_FUNC("_bcm_ptp_unicast_slave_subscribe()");
                }
            }

            if (signaled_slave_table.entry[i].delay_service_active) {
                /* Select message to service. */
                messageType = bcmPTP_MESSAGE_TYPE_DELAY_RESP;
                logInterMessagePeriod =  signaled_slave_table.entry[i].slave.log_delay_request_interval;
                durationField =  signaled_slave_table.entry[i].delay_duration;

                /* Cancel message. */
                if (BCM_FAILURE(rv = _bcm_ptp_slave_signaling_send(unit, ptp_id, clock_num, tlvType,
                        messageType, logInterMessagePeriod, durationField,
                        ++signaled_slave_table.entry[i].sequenceId, &slave))) {
                    PTP_ERROR_FUNC("_bcm_ptp_slave_signaling_send()");
                }

                /* Unsubscribe slave. */
                if (callstack &&
                    BCM_FAILURE(rv = _bcm_ptp_unicast_slave_subscribe(unit, ptp_id,
                        clock_num, slave.local_port_number, &slave, messageType, tlvType,
                        logInterMessagePeriod))) {
                    PTP_ERROR_FUNC("_bcm_ptp_unicast_slave_subscribe()");
                }
            }

            /*
             * Set slave entry.
             *    Reset message attributes to defaults to render inactive/expired.
             *    Preserve slave address.
             */
            signaled_slave_table.entry[i] = signaled_slave_entry_default;
            signaled_slave_table.entry[i].slave = slave;

        }
    }

    /* Update unicast slave table to remove inactive/expired slaves. */
    if (BCM_FAILURE(rv = _bcm_ptp_signaled_unicast_slave_table_update(unit, ptp_id))) {
        PTP_ERROR_FUNC("_bcm_ptp_signaled_unicast_slave_table_update()");
    }

    BCM_PTP_SLAVETABLE_MUTEX_RELEASE_RETURN(rv);
}

/* Basic parsing of a signaling message: checks clock/port numbers and gets TLV info */

int _bcm_ptp_parse_signaling_message(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    unsigned data_length,
    uint8 * data,
    int ptp_msg_offset,
    bcm_ptp_tlv_type_t *tlvType,
    bcm_ptp_message_type_t *messageType,
    int8 *logInterMessagePeriod,
    uint32 *durationField,
    int *num_tlvs)
{
    int rv;
    uint8 *targetClock;
    int cursor = 0;
    uint8 *ptp_tlv;
    uint16 lengthField;
    int lengthRemaining;
    bcm_ptp_clock_info_t ci;

    /* Initialize number of TLVs processed. */
    *num_tlvs = 0;

    /* Retrieve clock instance create-time data and metadata. */
    if (BCM_FAILURE(rv = bcm_ptp_clock_get(unit, ptp_id, clock_num, &ci))) {
        PTP_ERROR_FUNC("_bcm_ptp_clock_get()");
        /* Reject signaling messages if a PTP clock is not available. */
        return BCM_E_FAIL;
    }

    /* Check target clock identity. Either broadcast, or must match that of clock_num  */
    targetClock = data + ptp_msg_offset + PTP_PTPHDR_SIZE_OCTETS;
    if (sal_memcmp(targetClock, BCM_PTP_ALL_CLOCKS, sizeof(bcm_ptp_clock_identity_t)) != 0) {
        if (sal_memcmp(targetClock, ci.clock_identity, sizeof(bcm_ptp_clock_identity_t)) != 0) {
            return BCM_E_FAIL;
        }
    }

    /*
     * Dimension consistency check(s).
     * Ensure bogus or malformed messages do not cause range errors.
     */
    if (data_length - ptp_msg_offset <
        BCM_PTP_SIGNAL_MIN_UDP_PAYLOAD_SIZE_OCTETS) {
        return BCM_E_FAIL;
    }

    ptp_tlv = data + ptp_msg_offset + BCM_PTP_SIGNAL_TLV_OFFSET_OCTETS;
    lengthRemaining = data_length - ptp_msg_offset - BCM_PTP_SIGNAL_TLV_OFFSET_OCTETS;

    while (lengthRemaining > 0) {
        tlvType[*num_tlvs] = _bcm_ptp_uint16_read(ptp_tlv);
        cursor += sizeof(uint16);

        lengthField = _bcm_ptp_uint16_read(ptp_tlv + cursor);
        cursor += sizeof(uint16);

        /* Get message type from TLV. */
        if (lengthField < 1) {
            return BCM_E_FAIL;
        }
        messageType[*num_tlvs] = (ptp_tlv[cursor++] >> 4);

        /* If the message has space for them, parse the interval and duration */
        if (lengthField >= 5) {
            logInterMessagePeriod[*num_tlvs] = ptp_tlv[cursor++];
            durationField[*num_tlvs] = _bcm_ptp_uint32_read(ptp_tlv + cursor);
        } else {
            logInterMessagePeriod[*num_tlvs] = 0;
            durationField[*num_tlvs] = 0;
        }

        /* Advance to next TLV, if any. */
        ++(*num_tlvs);
        ptp_tlv += lengthField + 4;
        cursor = 0;

        /* Adjust remaining length. */
        lengthRemaining -= (lengthField + 4);

        /* Exit if maximum number of TLVs has been processed. */
        if (*num_tlvs >= BCM_PTP_SIGNAL_MAX_TLV_PER_MESSAGE) {
            lengthRemaining = 0;
        }
    }

    return BCM_E_NONE;
}

STATIC int
_bcm_ptp_make_peer_from_signaling_msg(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    unsigned data_length,
    uint8 *data,
    bcm_ptp_protocol_t protocol,
    int src_addr_offset,
    int ptp_msg_offset,
    bcm_ptp_clock_peer_t *peer)
{
    int any_port = 0xffff;
    int rv = BCM_E_NONE;
    int cursor;

    int port_num;
    bcm_ptp_clock_info_t ci;
    bcm_ptp_clock_port_info_t pi;

    bcm_mac_t src_mac;
    bcm_mac_t dest_mac;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    /* Parse signaling message to construct peer data structure. */
    switch ((bcm_ptp_protocol_t)protocol) {
    case bcmPTPUDPIPv4:
        /* Ethernet/UDP/IPv4 address. */
        peer->peer_address.raw_l2_header_length = src_addr_offset -
            PTP_IPV4HDR_SRC_IPADDR_OFFSET_OCTETS;

        if (peer->peer_address.raw_l2_header_length > PTP_MAX_L2_HEADER_LENGTH) {
            peer->peer_address.raw_l2_header_length = 0;
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "%s() failed %s\n", __func__, "Incompatible L2 header size.\n"));
            return BCM_E_CONFIG;
        }

        /*
         * Configure raw L2 header.
         * Reference implementation: Swap source and destination MACs from
         * incoming signaling message.
         */
        sal_memcpy(dest_mac, data, sizeof(bcm_mac_t));
        sal_memcpy(src_mac, data + sizeof(bcm_mac_t), sizeof(bcm_mac_t));

        sal_memset(peer->peer_address.raw_l2_header, 0, PTP_MSG_L2_HEADER_LENGTH);
        sal_memcpy(peer->peer_address.raw_l2_header, data,
            peer->peer_address.raw_l2_header_length);

        sal_memcpy(peer->peer_address.raw_l2_header, src_mac, sizeof(bcm_mac_t));
        sal_memcpy(peer->peer_address.raw_l2_header + sizeof(bcm_mac_t), dest_mac,
            sizeof(bcm_mac_t));

        /* Set address. */
        peer->peer_address.addr_type = bcmPTPUDPIPv4;
        peer->peer_address.ipv4_addr = _bcm_ptp_uint32_read(data + src_addr_offset);
        sal_memset(peer->peer_address.ipv6_addr, 0, PTP_IPV6_ADDR_SIZE_BYTES);

        /* Extract req'd remote (source) port information. */
        cursor = ptp_msg_offset + PTP_PTPHDR_SRCPORT_OFFSET_OCTETS;
        sal_memcpy(peer->clock_identity,
            data + cursor, sizeof(bcm_ptp_clock_identity_t));

        cursor += sizeof(bcm_ptp_clock_identity_t);
        peer->remote_port_number = _bcm_ptp_uint16_read(data + cursor);

        /* Extract req'd local (target) port information. */
        cursor = ptp_msg_offset + PTP_PTPHDR_SIZE_OCTETS + sizeof(bcm_ptp_clock_identity_t);
        peer->local_port_number = _bcm_ptp_uint16_read(data + cursor);

        /*
         * Process target port.
         * Reference implementation: Assign "all ports" target to port with
         *                           matching protocol address.
         * Reference implementation: Reject message if associated port is
         *                           unavailable as determined by the port
         *                           address accessor.
         * Reference implementation: Reject message if destination address
         *                           does not match local port address.
         */
        if (peer->local_port_number == any_port) {
            if (BCM_FAILURE(rv = bcm_ptp_clock_get(unit, ptp_id, clock_num, &ci))) {
                PTP_ERROR_FUNC("_bcm_ptp_clock_get()");
                return BCM_E_FAIL;
            }

            for (port_num = 1; port_num <= ci.num_ports; ++port_num) {
                if (BCM_FAILURE(bcm_ptp_clock_port_info_get(unit, ptp_id, clock_num, port_num, &pi))) {
                    PTP_ERROR_FUNC("_bcm_ptp_clock_port_info_get()");
                    return BCM_E_FAIL;
                }

                if (protocol == pi.port_address.addr_type &&
                    !sal_memcmp(data + src_addr_offset + PTP_IPV4_ADDR_SIZE_BYTES,
                                pi.port_address.address, PTP_IPV4_ADDR_SIZE_BYTES)) {
                    break;
                }
            }

            peer->local_port_number = port_num;
            if (peer->local_port_number > ci.num_ports) {
                peer->local_port_number = 1;
                PTP_WARN("Destination IP address mismatch (IPv4).\n");
                return BCM_E_PARAM;
            }
        } else {
            if (BCM_FAILURE(rv = bcm_ptp_clock_get(unit, ptp_id, clock_num, &ci))) {
                PTP_ERROR_FUNC("_bcm_ptp_clock_get()");
                return BCM_E_FAIL;
            }

            if (BCM_FAILURE(bcm_ptp_clock_port_info_get(unit, ptp_id, clock_num,
                                                        peer->local_port_number, &pi))) {
                PTP_ERROR_FUNC("_bcm_ptp_clock_port_info_get()");
                return BCM_E_FAIL;
            }

            if (protocol != pi.port_address.addr_type ||
                sal_memcmp(data + src_addr_offset + PTP_IPV4_ADDR_SIZE_BYTES,
                           pi.port_address.address, PTP_IPV4_ADDR_SIZE_BYTES)) {
                PTP_WARN("Destination IP address mismatch (IPv4).\n");
                return BCM_E_PARAM;
            }
        }

        break;

    case bcmPTPUDPIPv6:
        /* Ethernet/UDP/IPv6 address. */
        peer->peer_address.raw_l2_header_length = src_addr_offset - 8;

        if (peer->peer_address.raw_l2_header_length > PTP_MAX_L2_HEADER_LENGTH) {
            peer->peer_address.raw_l2_header_length = 0;
            SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
                "%s() failed %s\n", __func__, "Incompatible L2 header size.\n"));
            return BCM_E_CONFIG;
        }

        /*
         * Configure raw L2 header.
         * Reference implementation: Swap source and destination MACs from
         * incoming signaling message.
         */
        sal_memcpy(dest_mac, data, sizeof(bcm_mac_t));
        sal_memcpy(src_mac, data + sizeof(bcm_mac_t), sizeof(bcm_mac_t));

        sal_memset(peer->peer_address.raw_l2_header, 0, PTP_MSG_L2_HEADER_LENGTH);
        sal_memcpy(peer->peer_address.raw_l2_header, data,
            peer->peer_address.raw_l2_header_length);

        sal_memcpy(peer->peer_address.raw_l2_header, src_mac, sizeof(bcm_mac_t));
        sal_memcpy(peer->peer_address.raw_l2_header + sizeof(bcm_mac_t), dest_mac,
            sizeof(bcm_mac_t));

        /* Set address. */
        peer->peer_address.addr_type = bcmPTPUDPIPv6;
        peer->peer_address.ipv4_addr = 0;
        sal_memcpy(peer->peer_address.ipv6_addr, data + src_addr_offset,
            PTP_IPV6_ADDR_SIZE_BYTES);

        /* Extract req'd remote (source) port information. */
        cursor = ptp_msg_offset + PTP_PTPHDR_SRCPORT_OFFSET_OCTETS;
        sal_memcpy(peer->clock_identity,
            data + cursor, sizeof(bcm_ptp_clock_identity_t));

        cursor += sizeof(bcm_ptp_clock_identity_t);
        peer->remote_port_number = _bcm_ptp_uint16_read(data + cursor);

        /* Extract req'd local (target) port information. */
        cursor = ptp_msg_offset + PTP_PTPHDR_SIZE_OCTETS + sizeof(bcm_ptp_clock_identity_t);
        peer->local_port_number = _bcm_ptp_uint16_read(data + cursor);

        /*
         * Process target port.
         * Reference implementation: Assign "all ports" target to port with
         *                           matching protocol address.
         * Reference implementation: Reject message if associated port is
         *                           unavailable as determined by the port
         *                           address accessor.
         * Reference implementation: Reject message if destination address
         *                           does not match local port address.
         */
        if (peer->local_port_number == any_port) {
            if (BCM_FAILURE(rv = bcm_ptp_clock_get(unit, ptp_id, clock_num, &ci))) {
                PTP_ERROR_FUNC("_bcm_ptp_clock_get()");
                return BCM_E_FAIL;
            }

            for (port_num = 1; port_num <= ci.num_ports; ++port_num) {
                if (BCM_FAILURE(bcm_ptp_clock_port_info_get(unit, ptp_id, clock_num, port_num, &pi))) {
                    PTP_ERROR_FUNC("_bcm_ptp_clock_port_info_get()");
                    return BCM_E_FAIL;
                }

                if (protocol == pi.port_address.addr_type &&
                    !sal_memcmp(data + src_addr_offset + PTP_IPV6_ADDR_SIZE_BYTES,
                                pi.port_address.address, PTP_IPV6_ADDR_SIZE_BYTES)) {
                    break;
                }
            }

            peer->local_port_number = port_num;
            if (peer->local_port_number > ci.num_ports) {
                peer->local_port_number = 1;
                PTP_WARN("Destination IP address mismatch (IPv6).\n");
                return BCM_E_PARAM;
            }
        } else {
            if (BCM_FAILURE(bcm_ptp_clock_port_info_get(unit, ptp_id, clock_num,
                                                        peer->local_port_number, &pi))) {
                PTP_ERROR_FUNC("_bcm_ptp_clock_port_info_get()");
                return BCM_E_FAIL;
            }

            if (protocol != pi.port_address.addr_type ||
                sal_memcmp(data + src_addr_offset + PTP_IPV6_ADDR_SIZE_BYTES,
                           pi.port_address.address, PTP_IPV6_ADDR_SIZE_BYTES)) {
                PTP_WARN("Destination IP address mismatch (IPv6).\n");
                return BCM_E_PARAM;
            }
        }

        break;

    default:
        /* Unknown or unsupported address type. */
        SOC_DEBUG_PRINT((DK_ERR | DK_VERBOSE,
            "%s() failed %s\n", __func__, "Unknown or unsupported address type.\n"));
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_process_incoming_signaling_msg()
 * Purpose:
 *      Reference implementation to process an incoming PTP signaling message.
 * Parameters:
 *      unit            - (IN) Unit number.
 *      ptp_id          - (IN) PTP stack ID.
 *      clock_num       - (IN) PTP clock number.
 *      port_num        - (IN) PTP clock port number.
 *      protocol        - (IN) Network protocol.
 *      src_addr_offset - (IN) Source address offset in message.
 *      ptp_msg_offset  - (IN) PTP offset (PTP common header) in message.
 *      data_length     - (IN) Signaling message size.
 *      data            - (IN) Signaling message data.
 * Returns:
 *      bcmPTPCallbackAccept - Criteria met; accept PTP signaling message.
 *      bcmPTPCallbackReject - Criteria not met; reject PTP signaling message.
 */
bcm_ptp_callback_t
_bcm_ptp_process_incoming_signaling_msg(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_protocol_t protocol,
    int src_addr_offset,
    int ptp_msg_offset,
    unsigned data_length,
    uint8 *data)
{
    int rv;

    int num_tlvs;
    int tlv_num;

    bcm_ptp_tlv_type_t tlvType;
    bcm_ptp_tlv_type_t tlvTypes[BCM_PTP_SIGNAL_MAX_TLV_PER_MESSAGE];

    bcm_ptp_message_type_t messageType;
    bcm_ptp_message_type_t messageTypes[BCM_PTP_SIGNAL_MAX_TLV_PER_MESSAGE];

    int logInterMessagePeriodAsInt;
    int8 logInterMessagePeriod;
    int8 logInterMessagePeriods[BCM_PTP_SIGNAL_MAX_TLV_PER_MESSAGE];

    uint32 durationField;
    uint32 durationFields[BCM_PTP_SIGNAL_MAX_TLV_PER_MESSAGE];

    bcm_ptp_clock_peer_t peer;
    bcm_ptp_callback_t rvcb;
    bcm_ptp_cb_msg_t msg;
    _bcm_ptp_clock_cache_t *clock;
    _bcm_ptp_port_state_t portState;

    bcm_ptp_clock_info_t ci;
    uint8 domainNumber;

    bcm_ptp_signaling_arbiter_t arbiter = unit_sig_array[unit].arbiter;
    bcm_ptp_cb_signaling_arbiter_msg_t amsg;
    void *user_data = unit_sig_array[unit].arbiter_user_data;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return bcmPTPCallbackReject;
    }

    /*
     * Check domainNumber in message common header against local clock instance.
     * Reference implementation: Reject (ignore) PTP signaling messages that are
     *                           not in local clock's domain.
     */
    domainNumber = *(data + ptp_msg_offset + PTP_PTPHDR_DOMAIN_OFFSET_OCTETS);

    if (BCM_FAILURE(_bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, &ci)) ||
            domainNumber != ci.domain_number) {
        return bcmPTPCallbackReject;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_parse_signaling_message(unit, ptp_id, clock_num,
            port_num, data_length, data, ptp_msg_offset, tlvTypes, messageTypes,
            logInterMessagePeriods, durationFields, &num_tlvs))) {
        return bcmPTPCallbackReject;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_make_peer_from_signaling_msg(unit, ptp_id, clock_num,
            data_length, data, protocol, src_addr_offset, ptp_msg_offset, &peer))) {
        return bcmPTPCallbackReject;
    }

    /*
     * Check portState of target port.
     * Reference implementation: Reject (ignore) PTP signaling messages if
     *                           the portState of target port is DISABLED.
     */
    if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_port_state_get(unit,
            ptp_id, clock_num, peer.local_port_number, &portState)) ||
            portState == _bcm_ptp_state_disabled) {
        return bcmPTPCallbackReject;
    }

    for (tlv_num = 0; tlv_num < num_tlvs; ++tlv_num) {
        tlvType = tlvTypes[tlv_num];
        messageType = messageTypes[tlv_num];
        logInterMessagePeriod = logInterMessagePeriods[tlv_num];
        logInterMessagePeriodAsInt = (int)logInterMessagePeriod;
        durationField = durationFields[tlv_num];

        if (tlvType == bcmPTPTlvTypeRequestUnicastTransmission) {
            if (!arbiter) {
                return bcmPTPCallbackReject;
            }
            /*
             * Use caller-provided, application-specific registered interpreter/arbiter
             * callback to determine whether to accept or reject PTP signaling message.
             */
            clock = &_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].clock_array[clock_num];

            sal_memset(&amsg, 0, sizeof(bcm_ptp_cb_signaling_arbiter_msg_t));
            sal_memset(&msg, 0, sizeof(bcm_ptp_cb_msg_t));

            amsg.ptp_id = ptp_id;
            amsg.clock_num = clock_num;
            amsg.port_num = port_num;
            msg.length = data_length;
            msg.data = data;
            msg.protocol = protocol;
            msg.src_addr_offset = src_addr_offset;
            msg.msg_offset = ptp_msg_offset;
            amsg.clock_info = &clock->clock_info;
            amsg.tlv_type = &tlvType;
            amsg.messageType = &messageType;
            amsg.logInterMessagePeriod = &logInterMessagePeriodAsInt;
            amsg.durationField = &durationField;
            amsg.msg = &msg;
            amsg.slave = &peer;

            rvcb = arbiter(unit, &amsg, user_data);

            if (bcmPTPCallbackReject == rvcb) {
                /*
                 * Deny request for PTP message service.
                 * NOTE : Use as-provided messageType and logInterMessagePeriod
                 *        in the event a custom arbiter modifies the parameters.
                 */
                if (BCM_FAILURE(rv = _bcm_ptp_slave_signaling_request_deny(
                        unit, ptp_id, clock_num, messageTypes[tlv_num],
                        logInterMessagePeriods[tlv_num], &peer))) {
                    PTP_ERROR_FUNC("_bcm_ptp_slave_signaling_request_deny()");
                }
                continue;
            }
        }

        if ((rv = _bcm_ptp_mutex_take(slaveTable_mutex, BCM_PTP_SIGNAL_SLAVE_MUTEX_TIMEOUT_USEC))) {
            PTP_ERROR_FUNC("_bcm_ptp_mutex_take()");
            return bcmPTPCallbackReject;
        }

        switch (tlvType) {
        case bcmPTPTlvTypeRequestUnicastTransmission:
            {
                /*
                 * Signaled unicast slave table management.
                 *    Locate slave in signaled unicast slave table.
                 *    Add an entry if corresponding slave does not exist yet.
                 */
                int rv;
                int i;

                if (BCM_FAILURE(rv = _bcm_ptp_signaled_unicast_slave_exists(unit, ptp_id, &peer, &i))) {
                    if (BCM_FAILURE(rv = _bcm_ptp_signaled_unicast_slave_add(unit, ptp_id, &peer))) {
                        PTP_ERROR_FUNC("_bcm_ptp_signaled_unicast_slave_add()");
                        goto release_return;
                    }
                    i = signaled_slave_table.num_entries - 1;
                    signaled_slave_table.entry[i].slave.log_announce_interval = 99;
                    signaled_slave_table.entry[i].slave.log_sync_interval = 99;
                    signaled_slave_table.entry[i].slave.log_delay_request_interval = 99;

                    signaled_slave_table.entry[i].slave.announce_receive_timeout =
                        PTP_CLOCK_PRESETS_ANNOUNCE_RECEIPT_TIMEOUT_DEFAULT;
                    signaled_slave_table.entry[i].slave.delay_mechanism =
                        bcmPTPDelayMechanismEnd2End;
                    signaled_slave_table.entry[i].slave.tx_timestamp_mech =
                        peer.tx_timestamp_mech;
                }

                switch (messageType) {
                case bcmPTP_MESSAGE_TYPE_ANNOUNCE:
                    if ((signaled_slave_table.entry[i].slave.log_announce_interval != logInterMessagePeriod) ||
                        (signaled_slave_table.entry[i].announce_service_active == 0)) {
                        signaled_slave_table.entry[i].announce_service_changed = 1;
                    }

                    signaled_slave_table.entry[i].slave.log_announce_interval = logInterMessagePeriod;
                    signaled_slave_table.entry[i].queue_announce_grant = 1;
                    signaled_slave_table.entry[i].announce_duration = durationField;
                    signaled_slave_table.entry[i].announce_request_time = _bcm_ptp_monotonic_time();

                    goto release_return;

                case bcmPTP_MESSAGE_TYPE_SYNC:
                    if ((signaled_slave_table.entry[i].slave.log_sync_interval != logInterMessagePeriod) ||
                        (signaled_slave_table.entry[i].sync_service_active == 0)) {
                        signaled_slave_table.entry[i].sync_service_changed = 1;
                    }

                    signaled_slave_table.entry[i].slave.log_sync_interval = logInterMessagePeriod;
                    signaled_slave_table.entry[i].queue_sync_grant = 1;
                    signaled_slave_table.entry[i].sync_duration = durationField;
                    signaled_slave_table.entry[i].sync_request_time = _bcm_ptp_monotonic_time();

                    goto release_return;

                case bcmPTP_MESSAGE_TYPE_DELAY_RESP:
                    if ((signaled_slave_table.entry[i].slave.log_delay_request_interval != logInterMessagePeriod) ||
                        (signaled_slave_table.entry[i].delay_service_active == 0)) {
                        signaled_slave_table.entry[i].delay_service_changed = 1;
                    }

                    signaled_slave_table.entry[i].slave.log_delay_request_interval = logInterMessagePeriod;
                    signaled_slave_table.entry[i].queue_delay_grant = 1;
                    signaled_slave_table.entry[i].delay_duration = durationField;
                    signaled_slave_table.entry[i].delay_request_time = _bcm_ptp_monotonic_time();

                    goto release_return;

                default:
                    goto release_return;
                }
            }

            break;

        case bcmPTPTlvTypeGrantUnicastTransmission:
            {
                /* find the index of the master in our list: */
                int idx;

                for (idx = 0; idx < num_signaled_masters; ++idx) {
                    if (_bcm_ptp_peer_address_raw_compare(&signaled_master[idx].master.peer_address,
                            data + src_addr_offset, protocol)) {
                        switch (messageType) {
                        case bcmPTP_MESSAGE_TYPE_ANNOUNCE:
                            if (durationField) {
                                signaled_master[idx].announce_refresh_time = _bcm_ptp_monotonic_time() + durationField / 2 + 1;
                                signaled_master[idx].service_grants.announce = 1;
                                signaled_master[idx].unmet_requests.announce = 0;
                            } else {
                                /* Zero duration GRANT_UNICAST_TRANSMISSION equals request denial. */
                                signaled_master[idx].service_grants.announce = 0;
                            }
                            goto release_return;

                        case bcmPTP_MESSAGE_TYPE_SYNC:
                            if (durationField) {
                                signaled_master[idx].sync_refresh_time = _bcm_ptp_monotonic_time() + durationField / 2 + 1;
                                signaled_master[idx].service_grants.sync = 1;
                                signaled_master[idx].unmet_requests.sync = 0;
                            } else {
                                /* Zero duration GRANT_UNICAST_TRANSMISSION equals request denial. */
                                signaled_master[idx].service_grants.sync = 0;
                            }

                            if (signaled_master[idx].granted_log_sync_interval != logInterMessagePeriod) {
                                signaled_master[idx].granted_log_sync_interval = logInterMessagePeriod;
                                sal_dpc(delayed_master_table_update, INT_TO_PTR(delayed_master_table_update),
                                        INT_TO_PTR(idx), INT_TO_PTR(unit), INT_TO_PTR(ptp_id),0);
                            }
                            goto release_return;

                        case bcmPTP_MESSAGE_TYPE_DELAY_RESP:
                            if (durationField) {
                                signaled_master[idx].delay_refresh_time = _bcm_ptp_monotonic_time() + durationField / 2 + 1;
                                signaled_master[idx].service_grants.delayresp = 1;
                                signaled_master[idx].unmet_requests.delayresp = 0;
                            } else {
                                /* Zero duration GRANT_UNICAST_TRANSMISSION equals request denial. */
                                signaled_master[idx].service_grants.delayresp = 0;
                            }

                            if (signaled_master[idx].granted_log_delay_interval != logInterMessagePeriod) {
                                signaled_master[idx].granted_log_delay_interval = logInterMessagePeriod;
                                sal_dpc(delayed_master_table_update, INT_TO_PTR(delayed_master_table_update),
                                        INT_TO_PTR(idx), INT_TO_PTR(unit), INT_TO_PTR(ptp_id),0);
                            }
                            goto release_return;

                        default:
                            goto release_return;
                        }
                    }
                }
            }
            break;

        case bcmPTPTlvTypeCancelUnicastTransmission:
            {
                /* This could either be from a slave, indicating that we can stop sending
                 * or from a master, indicating that we no longer have a grant
                 * See if it matches a master from our list: */
                int idx;
                int rv;
                int i;

                for (idx = 0; idx < num_signaled_masters; ++idx) {
                    if (_bcm_ptp_peer_address_raw_compare(&signaled_master[idx].master.peer_address,
                            data + src_addr_offset, protocol)) {
                        switch (messageType) {
                        case bcmPTP_MESSAGE_TYPE_ANNOUNCE:
                            signaled_master[idx].announce_refresh_time =
                                _bcm_ptp_monotonic_time() + signaled_master[idx].query_interval_sec;
                            signaled_master[idx].service_grants.announce = 0;
                            signaled_master[idx].to_ack_cancel.announce = 1;
                            goto release_return;

                        case bcmPTP_MESSAGE_TYPE_SYNC:
                            signaled_master[idx].sync_refresh_time =
                                _bcm_ptp_monotonic_time() + signaled_master[idx].query_interval_sec;
                            signaled_master[idx].service_grants.sync = 0;
                            signaled_master[idx].to_ack_cancel.sync = 1;
                            goto release_return;

                        case bcmPTP_MESSAGE_TYPE_DELAY_RESP:
                            signaled_master[idx].delay_refresh_time =
                                _bcm_ptp_monotonic_time() + signaled_master[idx].query_interval_sec;
                            signaled_master[idx].service_grants.delayresp = 0;
                            signaled_master[idx].to_ack_cancel.delayresp = 1;
                            goto release_return;

                        default:
                            goto release_return;
                        }
                    }
                }

                /* Signaled unicast slave lookup. */
                if (BCM_FAILURE(rv = _bcm_ptp_signaled_unicast_slave_exists(unit, ptp_id, &peer, &i))) {
                    /* No matching entry in signaled unicast slave table. */
                    goto release_return;
                }

                switch (messageType) {
                case bcmPTP_MESSAGE_TYPE_ANNOUNCE:
                    signaled_slave_table.entry[i].announce_cancel_time = _bcm_ptp_monotonic_time();
                    signaled_slave_table.entry[i].queue_announce_cancel = 1;
                    goto release_return;

                case bcmPTP_MESSAGE_TYPE_SYNC:
                    signaled_slave_table.entry[i].sync_cancel_time = _bcm_ptp_monotonic_time();
                    signaled_slave_table.entry[i].queue_sync_cancel = 1;
                    goto release_return;

                case bcmPTP_MESSAGE_TYPE_DELAY_RESP:
                    signaled_slave_table.entry[i].delay_cancel_time = _bcm_ptp_monotonic_time();
                    signaled_slave_table.entry[i].queue_delay_cancel = 1;
                    goto release_return;

                default:
                    goto release_return;
                }
            }
            break;

        case bcmPTPTlvTypeAckCancelUnicastTransmission:
            /* Do nothing. */
            break;
        default:
            ;
        }

release_return:
        _bcm_ptp_mutex_give(slaveTable_mutex);
    }

    return bcmPTPCallbackAccept;
}


/*
 * Function:
 *      bcm_common_ptp_unicast_request_duration_get
 * Purpose:
 *      Get durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit        - (IN)  Unit number.
 *      ptp_id      - (IN)  PTP stack ID.
 *      clock_num   - (IN)  PTP clock number.
 *      port_num    - (IN) PTP clock port number.
 *      duration - (OUT) durationField (sec.)
 * Returns:
 *      BCM_E_XXX - Function status.
 */
int
bcm_common_ptp_unicast_request_duration_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 *duration)
{
    *duration = request_uc_durationField;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_unicast_request_duration_set
 * Purpose:
 *      Set durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit        - (IN)  Unit number.
 *      ptp_id      - (IN)  PTP stack ID.
 *      clock_num   - (IN)  PTP clock number.
 *      port_num    - (IN) PTP clock port number.
 *      duration - (IN) durationField (sec.)
 * Returns:
 *      BCM_E_XXX - Function status.
 */
int
bcm_common_ptp_unicast_request_duration_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 duration)
{
    if (duration < request_uc_durationField_min) {
        request_uc_durationField = request_uc_durationField_min;
    } else if (duration > request_uc_durationField_max) {
        request_uc_durationField = request_uc_durationField_max;
    } else {
        request_uc_durationField = duration;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_unicast_request_duration_min_get
 * Purpose:
 *      Get minimum durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit         - (IN)  Unit number.
 *      ptp_id       - (IN)  PTP stack ID.
 *      clock_num    - (IN)  PTP clock number.
 *      port_num     - (IN) PTP clock port number.
 *      duration_min - (OUT) minimum durationField (sec.)
 * Returns:
 *      BCM_E_XXX - Function status.
 */
int
bcm_common_ptp_unicast_request_duration_min_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 *duration_min)
{
    *duration_min = request_uc_durationField_min;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_unicast_request_duration_min_set
 * Purpose:
 *      Set minimum durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit         - (IN)  Unit number.
 *      ptp_id       - (IN)  PTP stack ID.
 *      clock_num    - (IN)  PTP clock number.
 *      port_num     - (IN) PTP clock port number.
 *      duration_min - (IN) minimum durationField (sec.)
 * Returns:
 *      BCM_E_XXX - Function status.
 */
int
bcm_common_ptp_unicast_request_duration_min_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 duration_min)
{
    if (duration_min > request_uc_durationField_max) {
        return BCM_E_PARAM;
    }

    request_uc_durationField_min = duration_min;

    /* durationField in-range enforcement. */
    if (request_uc_durationField < duration_min) {
        request_uc_durationField = duration_min;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_unicast_request_duration_max_get
 * Purpose:
 *      Get maximum durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit         - (IN)  Unit number.
 *      ptp_id       - (IN)  PTP stack ID.
 *      clock_num    - (IN)  PTP clock number.
 *      port_num     - (IN) PTP clock port number.
 *      duration_max - (OUT) maximum durationField (sec.)
 * Returns:
 *      BCM_E_XXX - Function status.
 */
int
bcm_common_ptp_unicast_request_duration_max_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 *duration_max)
{
    *duration_max = request_uc_durationField_max;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_unicast_request_duration_max_set
 * Purpose:
 *      Set maximum durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit         - (IN)  Unit number.
 *      ptp_id       - (IN)  PTP stack ID.
 *      clock_num    - (IN)  PTP clock number.
 *      port_num     - (IN) PTP clock port number.
 *      duration_max - (IN) maximum durationField (sec.)
 * Returns:
 *      BCM_E_XXX - Function status.
 */
int
bcm_common_ptp_unicast_request_duration_max_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 duration_max)
{
    if (duration_max < request_uc_durationField_min) {
        return BCM_E_PARAM;
    }

    request_uc_durationField_max = duration_max;

    /* durationField in-range enforcement. */
    if (request_uc_durationField > duration_max) {
        request_uc_durationField = duration_max;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      delayed_master_table_update
 * Purpose:
 *      DPC-called function to update the ToP's master table
 */
static void
delayed_master_table_update(
    void *owner,
    void *arg_idx,
    void *arg_unit,
    void *arg_ptp_id,
    void *arg_clock_num)
{
    int idx = (size_t)arg_idx;
    int unit = (size_t)arg_unit;
    bcm_ptp_stack_id_t ptp_id = (bcm_ptp_stack_id_t)(size_t)arg_ptp_id;
    int clock_num = (size_t) arg_clock_num;
    bcm_ptp_clock_unicast_master_t master_info;
    int rv;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return;
    }

    master_info.address = signaled_master[idx].master.peer_address;
    master_info.log_sync_interval = signaled_master[idx].log_sync_interval;
    master_info.log_min_delay_request_interval = signaled_master[idx].log_delay_interval;

    bcm_ptp_static_unicast_master_add(unit, ptp_id, clock_num,
        signaled_master[idx].master.local_port_number,
        &master_info);
}

#endif /* INCLUDE_PTP */
