/*
 * $Id: telecom.c,v 1.1 Broadcom SDK $
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
 * File:    telecom.c
 *
 * Purpose: 
 *
 * Functions:
 *      bcm_common_ptp_telecom_g8265_init
 *      bcm_common_ptp_telecom_g8265_shutdown
 *      bcm_common_ptp_telecom_g8265_network_option_get
 *      bcm_common_ptp_telecom_g8265_network_option_set
 *      bcm_common_ptp_telecom_g8265_quality_level_set
 *      bcm_common_ptp_telecom_g8265_receipt_timeout_get
 *      bcm_common_ptp_telecom_g8265_receipt_timeout_set
 *      bcm_common_ptp_telecom_g8265_pktstats_thresholds_get
 *      bcm_common_ptp_telecom_g8265_pktstats_thresholds_set
 *      bcm_common_ptp_telecom_g8265_packet_master_list
 *      bcm_common_ptp_telecom_g8265_packet_master_best_get
 *      bcm_common_ptp_telecom_g8265_packet_master_get
 *      bcm_common_ptp_telecom_g8265_packet_master_add
 *      bcm_common_ptp_telecom_g8265_packet_master_remove
 *      bcm_common_ptp_telecom_g8265_packet_master_lockout_set
 *      bcm_common_ptp_telecom_g8265_packet_master_non_reversion_set
 *      bcm_common_ptp_telecom_g8265_packet_master_wait_duration_set
 *      bcm_common_ptp_telecom_g8265_packet_master_priority_override
 *      bcm_common_ptp_telecom_g8265_packet_master_priority_set
 *
 *      _bcm_ptp_telecom_g8265_map_clockClass_QL
 *      _bcm_ptp_telecom_g8265_verbose_level_set
 *      _bcm_ptp_telecom_g8265_profile_enabled_set
 *      _bcm_ptp_telecom_g8265_controller
 *      _bcm_ptp_telecom_g8265_AMT_set
 *      _bcm_ptp_telecom_g8265_master_selector
 *      _bcm_ptp_telecom_g8265_PTSF_set
 *      _bcm_ptp_telecom_g8265_packet_master_PTSF_set
 *      _bcm_ptp_telecom_g8265_degrade_ql_set
 *      _bcm_ptp_telecom_g8265_packet_master_degrade_ql_set
 *      _bcm_ptp_telecom_g8265_wait_to_restore_set
 *      _bcm_ptp_telecom_g8265_packet_master_wait_to_restore_set
 *      _bcm_ptp_telecom_g8265_packet_master_fmds_processor
 *      _bcm_ptp_telecom_g8265_packet_master_priority_calculator
 *      _bcm_ptp_telecom_g8265_packet_master_manager
 *      _bcm_ptp_telecom_g8265_packet_master_lookup
 *      _bcm_ptp_telecom_g8265_packet_master_make
 *      _bcm_ptp_telecom_g8265_packet_master_printout
 *      _bcm_ptp_telecom_g8265_event_description
 */

#if defined(INCLUDE_PTP)

#include <bcm/ptp.h>
#include <bcm_int/common/ptp.h>
#include <bcm_int/ptp_common.h>
#include <bcm/error.h>
#include <shared/bsl.h>
#include <sal/core/dpc.h>
#include <sal/core/time.h>

/* Definitions. */
#define PTP_TELECOM_CONTROLLER_DPC_TIME_USEC_DEFAULT              (1000000)
#define PTP_TELECOM_CONTROLLER_DPC_TIME_USEC_IDLE                 (3000000)
#define PTP_TELECOM_MUTEX_TIMEOUT_USEC                             (100000)

#define PTP_TELECOM_MESSAGE_ELAPSED_TIME_MSEC_MAX                 (3600000)
#define PTP_TELECOM_MESSAGE_ELAPSED_TIME_MSEC_UNAVAIL          ((uint32)-1)

#define PACKET_MASTER(index)                            (telecom.gm[index])
#define PACKET_MASTER_BEST               PACKET_MASTER(telecom.selected_gm)

/* Macros. */
#define PTP_TELECOM_MUTEX_TAKE() \
    PTP_MUTEX_TAKE(telecom_g8265_mutex, PTP_TELECOM_MUTEX_TIMEOUT_USEC)

#define PTP_TELECOM_MUTEX_RELEASE_RETURN(__rv__) \
    PTP_MUTEX_RELEASE_RETURN(telecom_g8265_mutex, __rv__)

#define PTP_TELECOM_EVENT_REGISTER(__GM__,__event__)                      \
    do {                                                                  \
        if (++telecom_fifo.head >= PTP_TELECOM_EVENT_FIFO_SIZE ||         \
            telecom_fifo.head <= 0) {                                     \
            telecom_fifo.head = 0;                                        \
        }                                                                 \
        telecom_fifo.events[telecom_fifo.head].timestamp = sal_time();    \
        telecom_fifo.events[telecom_fifo.head].pktmaster = *__GM__;       \
        telecom_fifo.events[telecom_fifo.head].type = 0;                  \
        telecom_fifo.events[telecom_fifo.head].type |= (1u << __event__); \
    } while (0)

#define PTP_TELECOM_EVENT_HEAD_PRINT()                                     \
    do {                                                                   \
        if (telecom_g8265_verbose_level) {                                 \
            uint32 lcl_type = telecom_fifo.events[telecom_fifo.head].type; \
            bcm_ptp_telecom_g8265_pktmaster_t *lcl_gm =                    \
                &telecom_fifo.events[telecom_fifo.head].pktmaster;         \
                                                                           \
            LOG_CLI((BSL_META("Telecom Event: %s @ %u\n"),                         \
                     _bcm_ptp_telecom_g8265_event_description(lcl_type),        \
                     (uint32)telecom_fifo.events[telecom_fifo.head].timestamp)); \
            _bcm_ptp_telecom_g8265_packet_master_printout(lcl_gm, 0);      \
        }                                                                  \
    } while (0)

/* Constants and variables. */
static int telecom_g8265_init = 0;
static uint32 telecom_g8265_verbose_level = 0;
static _bcm_ptp_mutex_t telecom_g8265_mutex = 0x0;

static const bcm_ptp_telecom_g8265_pktmaster_t pktmaster_zero;
static _bcm_ptp_telecom_g8265_event_fifo_t telecom_fifo;
static bcm_ptp_telecom_g8265_profile_t telecom;

/* Static functions. */
static int _bcm_ptp_telecom_g8265_profile_enabled_set(
    int unit, bcm_ptp_stack_id_t ptp_id, int clock_num, uint8 enabled);

static void _bcm_ptp_telecom_g8265_controller(
    void *owner, void *arg_unit, void *arg_ptp_id,
    void *arg_clock_num, void *unused);

static int _bcm_ptp_telecom_g8265_AMT_set(
    int unit, bcm_ptp_stack_id_t ptp_id, int clock_num, uint32 clock_port,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster);

static int _bcm_ptp_telecom_g8265_master_selector(
    void);

static int _bcm_ptp_telecom_g8265_PTSF_set(
    void);

static int _bcm_ptp_telecom_g8265_packet_master_PTSF_set(
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster);

static int _bcm_ptp_telecom_g8265_degrade_ql_set(
    void);

static int _bcm_ptp_telecom_g8265_packet_master_degrade_ql_set(
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster);

static int _bcm_ptp_telecom_g8265_wait_to_restore_set(
    void);

static int _bcm_ptp_telecom_g8265_packet_master_wait_to_restore_set(
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster);

static int _bcm_ptp_telecom_g8265_packet_master_fmds_processor(
    int unit, bcm_ptp_stack_id_t ptp_id, int clock_num,
    bcm_ptp_foreign_master_dataset_t *FMds);

static int _bcm_ptp_telecom_g8265_packet_master_priority_calculator(
    uint8 gmPriority1, uint8 gmPriority2,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster);

static int _bcm_ptp_telecom_g8265_packet_master_manager(
    bcm_ptp_clock_port_address_t *address, int *index);

static int _bcm_ptp_telecom_g8265_packet_master_lookup(
    bcm_ptp_clock_port_address_t *address, int *index);

static int _bcm_ptp_telecom_g8265_packet_master_make(
    bcm_ptp_clock_port_address_t *address,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster);

static int _bcm_ptp_telecom_g8265_packet_master_printout(
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster, uint8 flags);

static const char* _bcm_ptp_telecom_g8265_event_description(
    uint32 type);

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_init
 * Purpose:
 *      Initialize the telecom profile.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_init(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num)
{
    int rv;
    int el;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (!telecom_g8265_mutex) {
        telecom_g8265_mutex = _bcm_ptp_mutex_create("telecom_packet_master_array");
    }

    /* Set ITU-T G.781 synchronization networking option. */
    telecom.network_option = bcm_ptp_telecom_g8265_network_option_disable; 

    /*
     * Set PTP message receipt timeouts.
     * NOTE : announceReceiptTimeout is not referenced in current implementation.
     *        PTSF lossAnnounce is handled in the firmware foreignMasterDS logic.
     */
    telecom.receipt_timeouts.announce = PTP_TELECOM_ANNOUNCE_RECEIPT_TIMEOUT_MSEC_DEFAULT;
    telecom.receipt_timeouts.sync = PTP_TELECOM_SYNC_RECEIPT_TIMEOUT_MSEC_DEFAULT;
    telecom.receipt_timeouts.delay_resp = PTP_TELECOM_DELAYRESP_RECEIPT_TIMEOUT_MSEC_DEFAULT;

    /* Set packet statistics thresholds. */
    COMPILER_64_SET(telecom.thresholds.pdv_scaled_allan_var, 0, 
        PTP_TELECOM_PDV_SCALED_ALLAN_VARIANCE_THRESHOLD_NSECSQ_DEFAULT);

    /* Set selected packet master. */
    telecom.previous_gm = -1;
    telecom.selected_gm = 0;

    /* Initialize packet master array entries. */
    for (el = 0; el < PTP_TELECOM_MAX_NUMBER_PKTMASTERS; ++el) {
        PACKET_MASTER(el) = pktmaster_zero;
        PACKET_MASTER(el).selector.non_reversion = PTP_TELECOM_NON_REVERSION_MODE_DEFAULT;
        COMPILER_64_SET(PACKET_MASTER(el).selector.wait_sec, 0, PTP_TELECOM_WAIT_TO_RESTORE_SEC_DEFAULT);
        PACKET_MASTER(el).state = bcm_ptp_telecom_g8265_pktmaster_state_unused;
    }

    /* Initialize event FIFO. */
    telecom_fifo.head = -1;

    telecom_g8265_init = 1;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_shutdown
 * Purpose:
 *      Shut down the telecom profile.
 * Parameters:
 *      unit - (IN) Unit number.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_shutdown(
    int unit)
{
    /* Cancel telecom controller delayed procedure call. */
    sal_dpc_cancel(INT_TO_PTR(&_bcm_ptp_telecom_g8265_controller));
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_network_option_get
 * Purpose:
 *      Get telecom profile ITU-T G.781 network option.
 * Parameters:
 *      unit           - (IN)  Unit number.
 *      ptp_id         - (IN)  PTP stack ID.
 *      clock_num      - (IN)  PTP clock number.
 *      network_option - (OUT) Telecom profile ITU-T G.781 network option.
 *                             |Disable |Option I |Option II |Option III |
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_network_option_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_network_option_t *network_option)
{
    *network_option = telecom.network_option;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_network_option_set
 * Purpose:
 *      Set telecom profile ITU-T G.781 network option.
 * Parameters:
 *      unit           - (IN) Unit number.
 *      ptp_id         - (IN) PTP stack ID.
 *      clock_num      - (IN) PTP clock number.
 *      network_option - (IN) Telecom profile ITU-T G.781 network option.
 *                            |Disable |Option I |Option II |Option III |
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_network_option_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_network_option_t network_option)
{
    int rv;
    uint8 enabled;

    telecom.network_option = network_option;

    enabled = (bcm_ptp_telecom_g8265_network_option_disable == network_option) ? (0):(1);
    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_profile_enabled_set(unit, ptp_id,
            clock_num, enabled))){
        return rv;
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_quality_level_set
 * Purpose:
 *      Set the ITU-T G.781 quality level (QL) of local clock.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      QL         - (IN) Quality level.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 *      Ref. ITU-T G.781 and ITU-T G.8265.1.
 *      Quality level is mapped to PTP clockClass.
 *      Quality level is used to infer ITU-T G.781 synchronization network
 *      option. ITU-T G.781 synchronization network option must be uniform
 *      among slave and its packet masters.
 */
int
bcm_common_ptp_telecom_g8265_quality_level_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_quality_level_t QL)
{
    int rv = BCM_E_NONE;
    uint8 clockClass;
    bcm_ptp_clock_info_t ci;

    if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_info_get(unit, ptp_id,
            clock_num, &ci))) {
        return rv;
    }

    if (ci.slaveonly) {
        /* Packet slave clock. Supersede caller-provided QL. */
        QL = bcm_ptp_telecom_g8265_ql_na_slv;
    } else {
        /* Set ITU-T G.781 synchronization networking option. */
        if (telecom.network_option != (QL /0x10)) {
            LOG_CLI((BSL_META_U(unit,
                                "ITU-T G.781 synchronization networking "
                                "option changed.\n")));
        }
        telecom.network_option = QL / 0x10;
    }

    /* Set PTP clockClass. */
    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_map_QL_clockClass(QL, &clockClass))) {
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_clock_class_set(unit, ptp_id, clock_num, clockClass))) {
        return rv;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_receipt_timeout_get
 * Purpose:
 *      Get PTP |Announce|Sync|Delay_Resp| message receipt timeout req'd for
 *      packet timing signal fail (PTSF) analysis.
 * Parameters:
 *      unit           - (IN)  Unit number.
 *      ptp_id         - (IN)  PTP stack ID.
 *      clock_num      - (IN)  PTP clock number.
 *      messageType    - (IN)  PTP message type.
 *      receiptTimeout - (OUT) PTP |Announce|Sync|Delay_Resp| message receipt timeout.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_receipt_timeout_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_message_type_t messageType,
    uint32 *receiptTimeout)
{
    switch (messageType) {
    case bcmPTP_MESSAGE_TYPE_ANNOUNCE:
        *receiptTimeout = telecom.receipt_timeouts.announce;
        break;

    case bcmPTP_MESSAGE_TYPE_SYNC:
        *receiptTimeout = telecom.receipt_timeouts.sync;
        break;

    case bcmPTP_MESSAGE_TYPE_DELAY_RESP:
        *receiptTimeout = telecom.receipt_timeouts.delay_resp;
        break;

    default:
        *receiptTimeout = 0;
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_receipt_timeout_set
 * Purpose:
 *      Set PTP |Announce|Sync|Delay_Resp| message receipt timeout req'd for
 *      packet timing signal fail (PTSF) analysis.
 * Parameters:
 *      unit           - (IN) Unit number.
 *      ptp_id         - (IN) PTP stack ID.
 *      clock_num      - (IN) PTP clock number.
 *      messageType    - (IN) PTP message type.
 *      receiptTimeout - (IN) PTP |Announce|Sync|Delay_Resp| message receipt timeout.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_receipt_timeout_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_message_type_t messageType,
    uint32 receiptTimeout)
{
    switch (messageType) {
    case bcmPTP_MESSAGE_TYPE_ANNOUNCE:
        telecom.receipt_timeouts.announce = receiptTimeout;
        break;

    case bcmPTP_MESSAGE_TYPE_SYNC:
        telecom.receipt_timeouts.sync = receiptTimeout;
        break;

    case bcmPTP_MESSAGE_TYPE_DELAY_RESP:
        telecom.receipt_timeouts.delay_resp = receiptTimeout;
        break;

    default:
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/* Function:
 *      bcm_common_ptp_telecom_g8265_pktstats_thresholds_get
 * Purpose:
 *      Get packet timing and PDV statistics thresholds req'd for packet timing
 *      signal fail (PTSF) analysis.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      thresholds - (OUT) Packet timing and PDV statistics thresholds.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_pktstats_thresholds_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_pktstats_t *thresholds)
{
    COMPILER_64_SET(thresholds->pdv_scaled_allan_var,
        COMPILER_64_HI(telecom.thresholds.pdv_scaled_allan_var),
        COMPILER_64_LO(telecom.thresholds.pdv_scaled_allan_var));

    return BCM_E_NONE;
}

/* Function:
 *      bcm_common_ptp_telecom_g8265_pktstats_thresholds_set
 * Purpose:
 *      Set packet timing and PDV statistics thresholds req'd for packet timing
 *      signal fail (PTSF) analysis.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      thresholds - (IN) Packet timing and PDV statistics thresholds.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_pktstats_thresholds_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_pktstats_t thresholds)
{
    COMPILER_64_SET(telecom.thresholds.pdv_scaled_allan_var,
        COMPILER_64_HI(thresholds.pdv_scaled_allan_var),
        COMPILER_64_LO(thresholds.pdv_scaled_allan_var));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_packet_master_list
 * Purpose:
 *      Get list of in-use packet masters.
 * Parameters:
 *      unit            - (IN)  Unit number.
 *      ptp_id          - (IN)  PTP stack ID.
 *      clock_num       - (IN)  PTP clock number.
 *      max_num_masters - (IN)  Maximum number of packet masters.
 *      num_masters     - (OUT) Number of in-use packet masters.
 *      best_master     - (OUT) Best packet master list index.
 *      pktmaster       - (OUT) Packet masters.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_packet_master_list(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int max_num_masters,
    int *num_masters,
    int *best_master,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    int el;

    *num_masters = 0;
    *best_master = -1;

    PTP_TELECOM_MUTEX_TAKE();

    for (el = 0; el < PTP_TELECOM_MAX_NUMBER_PKTMASTERS; ++el) {
        if (bcm_ptp_telecom_g8265_pktmaster_state_unused != PACKET_MASTER(el).state) {
            if (*num_masters == max_num_masters) {
                /* Insufficient caller-provided table size for result. */
                *num_masters = 0;
                PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_RESOURCE);
            }
            if (telecom.selected_gm == el) {
                *best_master = *num_masters;
            }
            pktmaster[(*num_masters)++] = PACKET_MASTER(el);
        }
    }

    PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_NONE);
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_packet_master_best_get
 * Purpose:
 *      Get current best packet master clock as defined by telecom profile
 *      master selection logic. 
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      pktmaster - (OUT) Best packet master clock.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_packet_master_best_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    PTP_TELECOM_MUTEX_TAKE();

    if (bcm_ptp_telecom_g8265_pktmaster_state_unused != PACKET_MASTER_BEST.state) {
        *pktmaster = PACKET_MASTER_BEST;
        PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_NONE);
    }

    *pktmaster = pktmaster_zero;
    PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_NOT_FOUND);
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_packet_master_get
 * Purpose:
 *      Get packet master by address. 
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      address   - (IN)  Packet master address.
 *      pktmaster - (OUT) Packet master.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_packet_master_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_clock_port_address_t *address,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    int rv;
    int el;

    PTP_TELECOM_MUTEX_TAKE();

    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_lookup(address, &el))) {
        *pktmaster = pktmaster_zero;
        PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_packet_master_lookup()");
        PTP_TELECOM_MUTEX_RELEASE_RETURN(rv);
    }

    *pktmaster = PACKET_MASTER(el);
    PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_NONE);
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_packet_master_add
 * Purpose:
 *      Add a packet master.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      port_num  - (IN) PTP clock port number.
 *      address   - (IN) Packet master address.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_packet_master_add(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_port_address_t *address)
{
    int rv;
    int el;

    PTP_TELECOM_MUTEX_TAKE();

    
    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_manager(address, &el))) {
        PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_packet_master_manager()");
        PTP_TELECOM_MUTEX_RELEASE_RETURN(rv);
    }

    /* Initialize packet master data. */
    _bcm_ptp_telecom_g8265_packet_master_make(address, &PACKET_MASTER(el));

    /* Register event. */
    PTP_TELECOM_EVENT_REGISTER(&PACKET_MASTER(el), PTP_TELECOM_EVENT_GM_ADDED_BIT);
    PTP_TELECOM_EVENT_HEAD_PRINT();

    PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_NONE);
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_packet_master_remove
 * Purpose:
 *      Remove a packet master.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      port_num  - (IN) PTP clock port number.
 *      address   - (IN) Packet master address.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_packet_master_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_port_address_t *address)
{
    int rv;
    int el;

    PTP_TELECOM_MUTEX_TAKE();

    
    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_manager(address, &el))) {
        PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_packet_master_manager()");
        PTP_TELECOM_MUTEX_RELEASE_RETURN(rv);
    }

    /* Register event. */
    PTP_TELECOM_EVENT_REGISTER(&PACKET_MASTER(el), PTP_TELECOM_EVENT_GM_REMOVED_BIT);
    PTP_TELECOM_EVENT_HEAD_PRINT();

    /* Clear packet master data. */
    PACKET_MASTER(el) = pktmaster_zero;
    PACKET_MASTER(el).selector.non_reversion = PTP_TELECOM_NON_REVERSION_MODE_DEFAULT;
    COMPILER_64_SET(PACKET_MASTER(el).selector.wait_sec, 0, PTP_TELECOM_WAIT_TO_RESTORE_SEC_DEFAULT);
    PACKET_MASTER(el).state = bcm_ptp_telecom_g8265_pktmaster_state_unused;

    /* Call telecom controller (so changes take effect immediately). */
    if (telecom.network_option != bcm_ptp_telecom_g8265_network_option_disable) {
        sal_dpc_cancel(INT_TO_PTR(&_bcm_ptp_telecom_g8265_controller));
        sal_dpc_time(0, &_bcm_ptp_telecom_g8265_controller, INT_TO_PTR(&_bcm_ptp_telecom_g8265_controller),
                     INT_TO_PTR(unit), INT_TO_PTR(ptp_id), INT_TO_PTR(clock_num), 0);
    }

    PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_NONE);
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_packet_master_lockout_set
 * Purpose:
 *      Set packet master lockout to exclude from master selection process.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      lockout   - (IN) Packet master lockout Boolean.
 *      address   - (IN) Packet master address.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_packet_master_lockout_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 lockout,
    bcm_ptp_clock_port_address_t *address)
{
    int rv;
    int el;

    PTP_TELECOM_MUTEX_TAKE();

    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_lookup(address, &el))) {
        PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_packet_master_lookup()");
        PTP_TELECOM_MUTEX_RELEASE_RETURN(rv);
    }
    PACKET_MASTER(el).selector.lockout = lockout ? 1:0;

    PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_NONE);
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_packet_master_non_reversion_set
 * Purpose:
 *      Set packet master non-reversion to control master selection process
 *      if master fails or is unavailable.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      nonrev    - (IN) Packet master non-reversion Boolean.
 *      address   - (IN) Packet master address.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_packet_master_non_reversion_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 nonrev,
    bcm_ptp_clock_port_address_t *address)
{
    int rv;
    int el;

    PTP_TELECOM_MUTEX_TAKE();

    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_lookup(address, &el))) {
        PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_packet_master_lookup()");
        PTP_TELECOM_MUTEX_RELEASE_RETURN(rv);
    }
    PACKET_MASTER(el).selector.non_reversion = nonrev ? 1:0;

    if (nonrev) {
        /*
         * Clear historical information so non-revertive behavior applies
         * from this point forward.
         */
        PACKET_MASTER(el).ptsf.counter = 0;
        COMPILER_64_ZERO(PACKET_MASTER(el).ptsf.timestamp);

        PACKET_MASTER(el).hql.counter = 0;
        COMPILER_64_ZERO(PACKET_MASTER(el).hql.timestamp);
    }

    PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_NONE);
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_packet_master_wait_duration_set
 * Purpose:
 *      Set packet master wait-to-restore duration to control master selection
 *      process if master fails or is unavailable.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      wait_sec  - (IN) Packet master wait-to-restore duration.
 *      address   - (IN) Packet master address.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_telecom_g8265_packet_master_wait_duration_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint64 wait_sec,
    bcm_ptp_clock_port_address_t *address)
{
    int rv;
    int el;

    PTP_TELECOM_MUTEX_TAKE();

    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_lookup(address, &el))) {
        PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_packet_master_lookup()");
        PTP_TELECOM_MUTEX_RELEASE_RETURN(rv);
    }
    PACKET_MASTER(el).selector.wait_sec = wait_sec;

    PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_NONE);
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_packet_master_priority_override
 * Purpose:
 *      Set priority override of packet master.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      override  - (IN) Packet master priority override Boolean.
 *      address   - (IN) Packet master address.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 *      Priority override controls whether a packet master's priority value
 *      is set automatically using grandmaster priority1 / priority2 in PTP
 *      announce message (i.e., override equals TRUE) or via host API calls
 *      (i.e., override equals FALSE).
 */
int
bcm_common_ptp_telecom_g8265_packet_master_priority_override(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 override,
    bcm_ptp_clock_port_address_t *address)
{
    int rv;
    int el;

    PTP_TELECOM_MUTEX_TAKE();

    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_lookup(address, &el))) {
        PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_packet_master_lookup()");
        PTP_TELECOM_MUTEX_RELEASE_RETURN(rv);
    }
    PACKET_MASTER(el).priority.override = override;

    PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_NONE);
}

/*
 * Function:
 *      bcm_common_ptp_telecom_g8265_packet_master_priority_set
 * Purpose:
 *      Set priority of packet master.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      priority   - (IN) Packet master priority.
 *      address    - (IN) Packet master address.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 *      Ref. ITU-T G.8265.1. 
 */
int
bcm_common_ptp_telecom_g8265_packet_master_priority_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint16 priority,
    bcm_ptp_clock_port_address_t *address)
{
    int rv;
    int el;

    PTP_TELECOM_MUTEX_TAKE();

    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_lookup(address, &el))) {
        PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_packet_master_lookup()");
        PTP_TELECOM_MUTEX_RELEASE_RETURN(rv);
    }
    PACKET_MASTER(el).priority.value = priority;

    PTP_TELECOM_MUTEX_RELEASE_RETURN(BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_map_QL_clockClass
 * Purpose:
 *      Map ITU-T G.781 quality level to PTP clock class.
 * Parameters:
 *      QL         - (IN)  Quality level.
 *      clockClass - (OUT) PTP clock class.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 *      Ref. ITU-T G.8265.1, Clause 6.7.3.1. 
 */
int
_bcm_ptp_telecom_g8265_map_QL_clockClass(
    bcm_ptp_telecom_g8265_quality_level_t QL,
    uint8 *clockClass)
{
    switch (QL) {
    case bcm_ptp_telecom_g8265_ql_II_prs:
        *clockClass = 80;
        break;
    case bcm_ptp_telecom_g8265_ql_II_stu:
        /* Fall through. */
    case bcm_ptp_telecom_g8265_ql_III_unk:
        *clockClass = 82;
        break;
    case bcm_ptp_telecom_g8265_ql_I_prc:
        *clockClass = 84;
        break;
    case bcm_ptp_telecom_g8265_ql_II_st2:
        *clockClass = 86;
        break;
    case bcm_ptp_telecom_g8265_ql_I_ssua:
        /* Fall through. */
    case bcm_ptp_telecom_g8265_ql_II_tnc:
        *clockClass = 90;
        break;
    case bcm_ptp_telecom_g8265_ql_I_ssub:
        *clockClass = 96;
        break;
    case bcm_ptp_telecom_g8265_ql_II_st3e:
        *clockClass = 100;
        break;
    case bcm_ptp_telecom_g8265_ql_II_st3:
        *clockClass = 102;
        break;
    case bcm_ptp_telecom_g8265_ql_I_sec:
        /* Fall through. */
    case bcm_ptp_telecom_g8265_ql_III_sec:
        *clockClass = 104;
        break;
    case bcm_ptp_telecom_g8265_ql_II_smc:
        *clockClass = 106;
        break;
    case bcm_ptp_telecom_g8265_ql_II_prov:
        *clockClass = 108;
        break;
    case bcm_ptp_telecom_g8265_ql_I_dnu:
        /* Fall through. */
    case bcm_ptp_telecom_g8265_ql_II_dus:
        *clockClass = 110;
        break;
    case bcm_ptp_telecom_g8265_ql_na_slv:
        /* Not applicable (packet slave). */
        *clockClass = 255;
        break;
    default:
        /*
         * Mapping unsuccessful.
         * Maximum alternate PTP profile clockClass value in range [68,122]
         * used by telecom profile. Ref. IEEE Std. 1588-2008, Clause 7.6.2.4.
         */
        *clockClass = 122;
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_map_clockClass_QL
 * Purpose:
 *      Map PTP clock class to ITU-T G.781 quality level.
 * Parameters:
 *      clockClass - (IN)  PTP clock class.
 *      QL         - (OUT) Quality level.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 *      Transformation requires ITU-T G.781 network option.
 */
int
_bcm_ptp_telecom_g8265_map_clockClass_QL(
    uint8 clockClass,
    bcm_ptp_telecom_g8265_quality_level_t *QL)
{
    if (clockClass == 255) {
        *QL = bcm_ptp_telecom_g8265_ql_na_slv;
        return BCM_E_NONE;
    }

    switch (telecom.network_option) {
    case (bcm_ptp_telecom_g8265_network_option_I):
        switch (clockClass) {
        case 84:
            *QL = bcm_ptp_telecom_g8265_ql_I_prc;
            break;
        case 90:
            *QL = bcm_ptp_telecom_g8265_ql_I_ssua;
            break;
        case 96:
            *QL = bcm_ptp_telecom_g8265_ql_I_ssub;
            break;
        case 104:
            *QL = bcm_ptp_telecom_g8265_ql_I_sec;
            break;
        case 110:
            *QL = bcm_ptp_telecom_g8265_ql_I_dnu;
            break;
        default:
            *QL = bcm_ptp_telecom_g8265_ql_invalid;
            return BCM_E_PARAM;
        }
        break;
    case (bcm_ptp_telecom_g8265_network_option_II):
        switch (clockClass) {
        case 80:
            *QL = bcm_ptp_telecom_g8265_ql_II_prs;
            break;
        case 82:
            *QL = bcm_ptp_telecom_g8265_ql_II_stu;
            break;
        case 86:
            *QL = bcm_ptp_telecom_g8265_ql_II_st2;
            break;
        case 90:
            *QL = bcm_ptp_telecom_g8265_ql_II_tnc;
            break;
        case 100:
            *QL = bcm_ptp_telecom_g8265_ql_II_st3e;
            break;
        case 102:
            *QL = bcm_ptp_telecom_g8265_ql_II_st3;
            break;
        case 106:
            *QL = bcm_ptp_telecom_g8265_ql_II_smc;
            break;
        case 108:
            *QL = bcm_ptp_telecom_g8265_ql_II_prov;
            break;
        case 110:
            *QL = bcm_ptp_telecom_g8265_ql_II_dus;
            break;
        default:
            *QL = bcm_ptp_telecom_g8265_ql_invalid;
            return BCM_E_PARAM;
        }
        break;
    case (bcm_ptp_telecom_g8265_network_option_III):
        switch (clockClass) {
        case 82:
            *QL = bcm_ptp_telecom_g8265_ql_III_unk;
            break;
        case 104:
            *QL = bcm_ptp_telecom_g8265_ql_III_sec;
            break;
        default:
            *QL = bcm_ptp_telecom_g8265_ql_invalid;
            return BCM_E_PARAM;
        }
        break;
    default:
        *QL = bcm_ptp_telecom_g8265_ql_invalid;
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_verbose_level_set
 * Purpose:
 *      Set flags to control verbosity of status, error, and event messages.
 * Parameters:
 *      flags - (IN) Telecom profile message control flags.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
_bcm_ptp_telecom_g8265_verbose_level_set(
    uint32 flags)
{
    telecom_g8265_verbose_level = flags;
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_profile_enabled_set
 * Purpose:
 *      Set telecom profile enabled Boolean.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      enabled   - (IN) Telecom profile enabled Boolean.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_profile_enabled_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 enabled)
{
    int rv;
    int i;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_TELECOM_PROFILE_ENABLED_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;
    bcm_ptp_clock_info_t ci;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0...5 : Custom management message key/identifier.
     *                  BCM<null><null><null>.
     *    Octet 6     : Telecom profile enabled Boolean.
     *    Octet 7     : Reserved.
     */
    sal_memcpy(payload, "BCM\0\0\0", 6);
    i = 6;
    payload[i] = enabled ? 1:0;

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        PTP_ERROR_FUNC("bcm_common_ptp_clock_port_identity_get()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num,
            &portid, PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_TELECOM_PROFILE_ENABLED,
            payload, PTP_MGMTMSG_PAYLOAD_TELECOM_PROFILE_ENABLED_SIZE_OCTETS,
            resp, &resp_len))) {
        PTP_ERROR_FUNC("_bcm_ptp_management_message_send()");
        return rv;
    }

    /*
     * Start telecom controller DPC iff telecom profile is enabled.
     * NB: Cancel prior telecom controller DPC process (if any).
     */
    sal_dpc_cancel(INT_TO_PTR(&_bcm_ptp_telecom_g8265_controller));
    if (enabled) {
        sal_dpc_time(PTP_TELECOM_CONTROLLER_DPC_TIME_USEC_DEFAULT,
                     &_bcm_ptp_telecom_g8265_controller, INT_TO_PTR(&_bcm_ptp_telecom_g8265_controller),
                     INT_TO_PTR(unit), INT_TO_PTR(ptp_id), INT_TO_PTR(clock_num), 0);
    }

    /* Update clock cache. */
    if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_info_get(unit, ptp_id,
            clock_num, &ci))) {
        PTP_ERROR_FUNC("_bcm_ptp_clock_cache_info_get()");
        return rv;
    }

    if (enabled) {
        SHR_BITSET(&ci.flags, PTP_CLOCK_FLAGS_TELECOM_PROFILE_BIT);
    } else {
        SHR_BITCLR(&ci.flags, PTP_CLOCK_FLAGS_TELECOM_PROFILE_BIT);
    }

    if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_info_set(unit, ptp_id,
            clock_num, ci))) {
        PTP_ERROR_FUNC("_bcm_ptp_clock_cache_info_set()");
        return rv;
    }

    /*
     * Manage signaling module behavior(s) and configurable PTP attributes
     * based on the active PTP profile.
     */
    if (BCM_FAILURE(rv = _bcm_ptp_signaling_manager(unit, ptp_id, clock_num,
            SHR_BITGET(&ci.flags, PTP_CLOCK_FLAGS_TELECOM_PROFILE_BIT)))) {
        PTP_ERROR_FUNC("_bcm_ptp_signaling_manager()");
        return rv;
    }

    

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_controller
 * Purpose:
 *      Run telecom profile controller (as DPC).
 * Parameters:
 *      owner         - (IN) DPC owner.
 *      arg_unit      - (IN) Unit number (as void*).
 *      arg_ptp_id    - (IN) PTP stack ID (as void*).
 *      arg_clock_num - (IN) PTP clock number (as void*).
 *      unused        - (IN) Unused.
 * Returns:
 *      None.
 * Notes:
 */
static void
_bcm_ptp_telecom_g8265_controller(
    void *owner,
    void *arg_unit,
    void *arg_ptp_id,
    void *arg_clock_num,
    void *unused)
{
    int rv;

    int unit = (size_t)arg_unit;
    bcm_ptp_stack_id_t ptp_id = (bcm_ptp_stack_id_t)(size_t)arg_ptp_id;
    int clock_num = (size_t)arg_clock_num;
    bcm_ptp_clock_info_t ci;

    bcm_ptp_foreign_master_dataset_t FMds;
    bcm_ptp_telecom_g8265_pktmaster_t pktmaster_amt;
    int isnew_amt = 0;

    sal_usecs_t dpc_recall_usec = PTP_TELECOM_CONTROLLER_DPC_TIME_USEC_DEFAULT;

    if (telecom.network_option == bcm_ptp_telecom_g8265_network_option_disable) {
        /*
         * Telecom controller DPC should not be running.
         * (Shutdown with data consistency enforcement.)
         */
        if (BCM_FAILURE(rv = bcm_common_ptp_telecom_g8265_network_option_set(unit, ptp_id, clock_num,
                bcm_ptp_telecom_g8265_network_option_disable))) {
            PTP_ERROR_FUNC("bcm_common_ptp_telecom_g8265_network_option_set()");
        }
        return;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        dpc_recall_usec = PTP_TELECOM_CONTROLLER_DPC_TIME_USEC_IDLE;
        goto telecom_dpc_recall;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_info_get(unit, ptp_id,
            clock_num, &ci))) {
        PTP_ERROR_FUNC("_bcm_ptp_clock_cache_info_get()");
        dpc_recall_usec = PTP_TELECOM_CONTROLLER_DPC_TIME_USEC_IDLE;
        goto telecom_dpc_recall;
    }

    if (0 == ci.slaveonly) {
        dpc_recall_usec = PTP_TELECOM_CONTROLLER_DPC_TIME_USEC_IDLE;
        goto telecom_dpc_recall;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_foreign_master_dataset_get(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT, &FMds))) {
        PTP_ERROR_FUNC("bcm_common_ptp_foreign_master_dataset_get()");
        dpc_recall_usec = PTP_TELECOM_CONTROLLER_DPC_TIME_USEC_IDLE;
        goto telecom_dpc_recall;
    }

    if ((rv = _bcm_ptp_mutex_take(telecom_g8265_mutex, PTP_TELECOM_MUTEX_TIMEOUT_USEC))) {
        PTP_ERROR_FUNC("_bcm_ptp_mutex_take()");
        goto telecom_dpc_recall;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_fmds_processor(unit,
            ptp_id, clock_num, &FMds))) {
        PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_packet_master_fmds_processor()");
        goto telecom_dpc_release;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_master_selector())) {
        /* PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_master_selector()"); */
        goto telecom_dpc_release;
    }

    if (telecom.previous_gm != telecom.selected_gm) {
        /* GM changed. */
        isnew_amt = 1;
        pktmaster_amt = PACKET_MASTER_BEST;

        telecom.previous_gm = telecom.selected_gm;

        /* Register event. */
        PTP_TELECOM_EVENT_REGISTER(&PACKET_MASTER_BEST, PTP_TELECOM_EVENT_GM_SELECTED_BIT);
        PTP_TELECOM_EVENT_HEAD_PRINT();
    }

telecom_dpc_release:
    _bcm_ptp_mutex_give(telecom_g8265_mutex);

    if (1 == isnew_amt) {
        /* GM changed - update ToP acceptable master table (AMT). */
        if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_AMT_set(unit, ptp_id, clock_num,
                PTP_CLOCK_PORT_NUMBER_DEFAULT, &pktmaster_amt))) {
            PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_AMT_set()");
        }
    }

telecom_dpc_recall:
    sal_dpc_time(dpc_recall_usec, &_bcm_ptp_telecom_g8265_controller,
                 INT_TO_PTR(&_bcm_ptp_telecom_g8265_controller),
                 arg_unit, arg_ptp_id, arg_clock_num, 0);

    return;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_AMT_set
 * Purpose:
 *      Set acceptable master table (AMT), which provides the mechanism to use
 *      the best packet master for frequency synchronization.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      pktmaster  - (IN) Packet master.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_AMT_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    int rv;
    int i;

    uint8 payload[128] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS; 

    bcm_ptp_port_identity_t portid;
    int addr_len = 0;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    

    /* Set AMT. */

    /*
     * Make payload.
     *    0...1   : Number of acceptable master table entries.
     *    2...3   : AMT#0 network protocol.
     *    4...5   : AMT#0 address length.
     *    6...N-1 : AMT#0 address.
     *    N       : AMT#0 alternatePriority1.
     *    N+1     : Reserved.
     */
    i = 0;
    _bcm_ptp_uint16_write(payload, 1);
    i += sizeof(uint16);

    _bcm_ptp_uint16_write(payload + i, pktmaster->port_address.addr_type);
    i += sizeof(uint16);

    addr_len = _bcm_ptp_addr_len((int)pktmaster->port_address.addr_type);
    if (0 == addr_len) {
        return BCM_E_PARAM;
    }

    _bcm_ptp_uint16_write(payload + i, addr_len);
    i += sizeof(uint16);
    sal_memcpy(payload + i, pktmaster->port_address.address, addr_len);
    i += addr_len;

    payload[i++] = pktmaster->grandmaster_priority1;
    payload[i++] = 0x00;

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id, 
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        PTP_ERROR_FUNC("bcm_common_ptp_clock_port_identity_get()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num,
            &portid, PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_ACCEPTABLE_MASTER_TABLE,
            payload, i, resp, &resp_len))) {
        return rv;
    }

    /* Enable AMT. */

    /*
     * Make payload.
     *    Octet 0 : Acceptable master table enabled (EN) Boolean.
     *              |B7|B6|B5|B4|B3|B2|B1|B0| = |0|0|0|0|0|0|0|EN|.
     *    Octet 1 : Reserved.
     */
    payload[0] = 0x01;
    payload[1] = 0x00;

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id, 
            clock_num, clock_port, &portid))) {
        PTP_ERROR_FUNC("bcm_common_ptp_clock_port_identity_get()");
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_ACCEPTABLE_MASTER_TABLE_ENABLED,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_master_selector
 * Purpose:
 *      Choose a master clock via telecom profile master selection process.
 * Parameters:
 *      NONE
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 *      Ref. ITU-T G.8265.1, Clause 6.7.3.
 */
static int
_bcm_ptp_telecom_g8265_master_selector(
    void)
{
    int el;
    uint8 unknown_gm = 0;

    /* Update PTSF properties. */
    _bcm_ptp_telecom_g8265_PTSF_set();

    /* Update QL-degradation event properties. */
    _bcm_ptp_telecom_g8265_degrade_ql_set();

    /* Update wait-to-restore. */
    _bcm_ptp_telecom_g8265_wait_to_restore_set();

    if (bcm_ptp_telecom_g8265_pktmaster_state_unused == PACKET_MASTER_BEST.state ||
        1 == PACKET_MASTER_BEST.selector.lockout ||
        1 == PACKET_MASTER_BEST.selector.wait_to_restore ||
        1 == PACKET_MASTER_BEST.ptsf.loss_announce ||
        1 == PACKET_MASTER_BEST.ptsf.loss_sync ||
        1 == PACKET_MASTER_BEST.ptsf.unusable ||
        1 == PACKET_MASTER_BEST.hql.degrade_ql) {
        /* Prior best master is no longer valid. */
        unknown_gm = 1;
    }

    for (el = 0; el < PTP_TELECOM_MAX_NUMBER_PKTMASTERS; ++el) {
        if (bcm_ptp_telecom_g8265_pktmaster_state_unused == PACKET_MASTER(el).state ||
            1 == PACKET_MASTER(el).selector.lockout ||
            1 == PACKET_MASTER(el).selector.wait_to_restore ||
            1 == PACKET_MASTER(el).ptsf.loss_announce ||
            1 == PACKET_MASTER(el).ptsf.loss_sync ||
            1 == PACKET_MASTER(el).ptsf.unusable ||
            1 == PACKET_MASTER(el).hql.degrade_ql) {
            /* Packet master does not meet master selector criteria. */
            continue;
        }

        if (1 == unknown_gm) {
            unknown_gm = 0;
            telecom.selected_gm = el;
        }

        if (PACKET_MASTER(el).clock_class < PACKET_MASTER_BEST.clock_class) {
            /*
             * Higher quality level (QL) than currently selected packet master.
             * NOTE : ITU-T G.781 quality level (QL) is inversely proportional
             *        to PTP clockClass attribute. Ref. ITU-T G.8265.1, Clause
             *        6.7.3.1, Table 1.
             */
            telecom.selected_gm = el;
        } else if (PACKET_MASTER(el).clock_class == PACKET_MASTER_BEST.clock_class) {
            if (PACKET_MASTER(el).priority.value <
                PACKET_MASTER_BEST.priority.value) {
                /* Higher priority (lower numerical value) than currently selected packet master. */
                telecom.selected_gm = el;
            }
        }

    }

    if (1 == unknown_gm) {
        return BCM_E_UNAVAIL;
    } else {
        return BCM_E_NONE;
    }

}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_PTSF_set
 * Purpose:
 *      Set packet timing signal fail (PTSF) properties of in-use packet
 *      master array entries.
 * Parameters:
 *      NONE
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_PTSF_set(
    void)
{
    int rv;
    int el;

    for (el = 0; el < PTP_TELECOM_MAX_NUMBER_PKTMASTERS; ++el) {
        if (bcm_ptp_telecom_g8265_pktmaster_state_unused == PACKET_MASTER(el).state) {
            continue;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_PTSF_set(
                &PACKET_MASTER(el)))) {
            return rv;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_packet_master_PTSF_set
 * Purpose:
 *      Set packet timing signal fail (PTSF) properties.
 * Parameters:
 *      pktmaster - (IN/OUT) Packet master.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_packet_master_PTSF_set(
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    if (NULL == pktmaster ||
        bcm_ptp_telecom_g8265_pktmaster_state_unused == pktmaster->state) {
        return BCM_E_UNAVAIL;
    }

    /*
     * PTSF lossAnnounce?
     * Test ITU-T G.8265.1 criteria on non-receipt of PTP announce messages,
     * indicating loss of channel traceability information. 
     */
    pktmaster->ptsf.loss_announce =
        (pktmaster->elapsed_times.announce > telecom.receipt_timeouts.announce) ? 1:0;

    /*
     * PTSF lossSync?
     * Test ITU-T G.8265.1 criteria on non-receipt of PTP timing messages,
     * indicating loss of packet timing signal.
     */
    pktmaster->ptsf.loss_timing_sync =
        (pktmaster->elapsed_times.sync > telecom.receipt_timeouts.sync) ? 1:0;

    pktmaster->ptsf.loss_timing_sync_ts =
        (pktmaster->elapsed_times.sync_ts > telecom.receipt_timeouts.sync) ? 1:0;

    pktmaster->ptsf.loss_timing_delay =
        (pktmaster->elapsed_times.delay_resp > telecom.receipt_timeouts.delay_resp) ? 1:0;

    /*
     * One-way operation.
     * NOTE: Bypass PTSF analysis of Delay_Resp messages.
     * Delay request and delay response messages are not exchanged.
     * Firmware overwrites time since last Delay_Resp with all-ones.
     */
    if (pktmaster->elapsed_times.delay_resp == PTP_TELECOM_MESSAGE_ELAPSED_TIME_MSEC_UNAVAIL) {
        pktmaster->ptsf.loss_timing_delay = 0;
    }

    pktmaster->ptsf.loss_sync = pktmaster->ptsf.loss_timing_sync |
                                pktmaster->ptsf.loss_timing_sync_ts |
                                pktmaster->ptsf.loss_timing_delay;

    /*
     * PTSF unusable?
     * Reference implementation: apply packet timing statistics criteria,
     * incl. thresholds on packet delay variation (PDV) statistical metrics.
     */
    pktmaster->ptsf.unusable = COMPILER_64_GT(pktmaster->pktstats.pdv_scaled_allan_var,
                                              telecom.thresholds.pdv_scaled_allan_var) ? 1:0;

    if (0 == pktmaster->fmds_counter) {
        /*
         * Startup/discovery dynamics.
         * Bypass PTSF criteria for wait-to-restore/non-reversion classifications
         * until a packet master is recognized in foreignMasterDS.
         */
         COMPILER_64_ZERO(pktmaster->ptsf.timestamp);
         pktmaster->ptsf.counter = 0;
    } else if (pktmaster->ptsf.loss_announce || pktmaster->ptsf.loss_sync || pktmaster->ptsf.unusable) {
        /*
         * PTSF criteria met?
         * Set PTSF timestamp and PTSF occurence counter.
         */
        COMPILER_64_SET(pktmaster->ptsf.timestamp, 0, sal_time());
        if (pktmaster->ptsf.counter < ((uint32)-1)) {
            ++pktmaster->ptsf.counter;
        }

        /* Register event(s). */
        if (pktmaster->ptsf.loss_announce) {
            PTP_TELECOM_EVENT_REGISTER(pktmaster, PTP_TELECOM_EVENT_PTSF_LOSS_ANNOUNCE_BIT);
            PTP_TELECOM_EVENT_HEAD_PRINT();
        }
        if (pktmaster->ptsf.loss_timing_sync) {
            PTP_TELECOM_EVENT_REGISTER(pktmaster, PTP_TELECOM_EVENT_PTSF_LOSS_SYNC_BIT);
            PTP_TELECOM_EVENT_HEAD_PRINT();
        }
        if (pktmaster->ptsf.loss_timing_sync_ts) {
            PTP_TELECOM_EVENT_REGISTER(pktmaster, PTP_TELECOM_EVENT_PTSF_LOSS_SYNC_TS_BIT);
            PTP_TELECOM_EVENT_HEAD_PRINT();
        }
        if (pktmaster->ptsf.loss_timing_delay) {
            PTP_TELECOM_EVENT_REGISTER(pktmaster, PTP_TELECOM_EVENT_PTSF_LOSS_DELAY_RESP_BIT);
            PTP_TELECOM_EVENT_HEAD_PRINT();
        }
        if (pktmaster->ptsf.unusable) {
            PTP_TELECOM_EVENT_REGISTER(pktmaster, PTP_TELECOM_EVENT_PTSF_UNUSABLE_BIT);
            PTP_TELECOM_EVENT_HEAD_PRINT();
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_degrade_ql_set
 * Purpose:
 *      Set QL-degradation event properties of in-use packet master array entries.
 * Parameters:
 *      NONE
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_degrade_ql_set(
    void)
{
    int rv;
    int el;

    for (el = 0; el < PTP_TELECOM_MAX_NUMBER_PKTMASTERS; ++el) {
        if (bcm_ptp_telecom_g8265_pktmaster_state_unused == PACKET_MASTER(el).state) {
            continue;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_degrade_ql_set(
                &PACKET_MASTER(el)))) {
            return rv;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_packet_master_degrade_ql_set
 * Purpose:
 *      Set QL-degradation event properties.
 * Parameters:
 *      pktmaster - (IN/OUT) Packet master.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_packet_master_degrade_ql_set(
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    if (NULL == pktmaster ||
        bcm_ptp_telecom_g8265_pktmaster_state_unused == pktmaster->state) {
        return BCM_E_UNAVAIL;
    }

    /* 
     * QL degradation?
     * NOTE: An increase of PTP clockClass over consecutive queries of foreign
     *       master dataset is only condition presently considered. Additional
     *       criteria might be worthwhile in subsequent implementations.
     */
    pktmaster->hql.degrade_ql =
        (pktmaster->hql.clock_class_window.rise_events) ? 1:0;

    if (0 == pktmaster->fmds_counter) {
        /*
         * Startup/discovery dynamics.
         * Bypass QL degradation criteria for wait-to-restore/non-reversion classifications
         * until a packet master is recognized in foreignMasterDS.
         */
        COMPILER_64_ZERO(pktmaster->hql.timestamp);
        pktmaster->hql.counter = 0;
    } else if (pktmaster->hql.degrade_ql) {
        /*
         * QL degradation criteria met?
         * Set QL-degradation event timestamp and occurence counter.
         */
        COMPILER_64_SET(pktmaster->hql.timestamp, 0, sal_time());
        if (pktmaster->hql.counter < ((uint32)-1)) {
            ++pktmaster->hql.counter;
        }

        /* Register event. */
        PTP_TELECOM_EVENT_REGISTER(pktmaster, PTP_TELECOM_EVENT_QL_DEGRADED_BIT);
        PTP_TELECOM_EVENT_HEAD_PRINT();
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_wait_to_restore_set
 * Purpose:
 *      Set wait-to-restore attribute of in-use packet master array entries.
 * Parameters:
 *      NONE
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_wait_to_restore_set(
    void)
{
    int rv;
    int el;

    for (el = 0; el < PTP_TELECOM_MAX_NUMBER_PKTMASTERS; ++el) {
        if (bcm_ptp_telecom_g8265_pktmaster_state_unused == PACKET_MASTER(el).state) {
            continue;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_wait_to_restore_set(
                &PACKET_MASTER(el)))) {
            return rv;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_packet_master_wait_to_restore_set
 * Purpose:
 *      Set wait-to-restore attribute.
 * Parameters:
 *      pktmaster - (IN/OUT) Packet master.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_packet_master_wait_to_restore_set(
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    sal_time_t t = sal_time();
    uint64 t_pass;

    if (NULL == pktmaster || bcm_ptp_telecom_g8265_pktmaster_state_unused == pktmaster->state) {
        return BCM_E_UNAVAIL;
    }

    COMPILER_64_SET(t_pass, 0, t);
    COMPILER_64_SUB_64(t_pass, pktmaster->selector.wait_sec);

    if (pktmaster->selector.non_reversion && (pktmaster->ptsf.counter || pktmaster->hql.counter)) {
        /*
         * Non-reversion mode.
         * Packet-master PTSF or QL degradation has occurred at least once.
         * Prevent selection of master via TRUE wait-to-restore setting.
         */
        pktmaster->selector.wait_to_restore = 1;
    } else if (COMPILER_64_LT(t_pass, pktmaster->ptsf.timestamp) ||
               COMPILER_64_LT(t_pass, pktmaster->hql.timestamp)) {
        /* Elapsed-time criterion.
         * Time since most recent PTSF or QL-degradation event is less than
         * prescribed wait-to-restore duration. Prevent selection of master
         * via TRUE wait-to-restore setting.
         */
        pktmaster->selector.wait_to_restore = 1;
    } else {
        pktmaster->selector.wait_to_restore = 0;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_packet_master_fmds_processor
 * Purpose:
 *      Query foreign master dataset and assign attributes, data, and derived
 *      quantities to corresponding packet master entries.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      FMds      - (IN) Foreign master dataset.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_packet_master_fmds_processor(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_foreign_master_dataset_t *FMds)
{
    int rv;
    int i, el;

    bcm_ptp_foreign_master_entry_t *fm;
    _bcm_ptp_port_state_t portState;

    /*
     * "Clear" packet master state, message receipt times, etc. to account for
     * the foreignMasterDS firmware logic to dynamically remove foreign master
     * entries.
     */
    for (el = 0; el < PTP_TELECOM_MAX_NUMBER_PKTMASTERS; ++el) {
        if (bcm_ptp_telecom_g8265_pktmaster_state_unused != PACKET_MASTER(el).state) {
            PACKET_MASTER(el).state = bcm_ptp_telecom_g8265_pktmaster_state_init;

            PACKET_MASTER(el).elapsed_times.announce = PTP_TELECOM_MESSAGE_ELAPSED_TIME_MSEC_MAX;
            PACKET_MASTER(el).elapsed_times.sync = PTP_TELECOM_MESSAGE_ELAPSED_TIME_MSEC_MAX;
            PACKET_MASTER(el).elapsed_times.sync_ts = PTP_TELECOM_MESSAGE_ELAPSED_TIME_MSEC_MAX;
            PACKET_MASTER(el).elapsed_times.delay_resp = PTP_TELECOM_MESSAGE_ELAPSED_TIME_MSEC_MAX;

            COMPILER_64_ALLONES(PACKET_MASTER(el).pktstats.pdv_scaled_allan_var);
        }
    }

    /* Update packet master attributes. */
    for (i = 0; i < FMds->num_foreign_masters; ++i) {
        fm = &FMds->foreign_master[i]; 
        if (BCM_FAILURE(rv = _bcm_ptp_telecom_g8265_packet_master_lookup(&fm->address, &el))) {
            PTP_ERROR_FUNC("_bcm_ptp_telecom_g8265_packet_master_lookup()");
            continue;
        }

        /*
         * Update foreignMasterDS entry counter for this packet master to keep
         * track of number of times it has shown up in foreignMasterDS queries.
         * Do not rollover.
         */
        if (PACKET_MASTER(el).fmds_counter < ((uint32)-1)) {
            ++PACKET_MASTER(el).fmds_counter;
        }

        PACKET_MASTER(el).port_identity = fm->port_identity;

        PACKET_MASTER(el).clock_class = fm->clockClass;
        _bcm_ptp_telecom_g8265_map_clockClass_QL(PACKET_MASTER(el).clock_class, &PACKET_MASTER(el).quality_level);

        _bcm_ptp_telecom_g8265_packet_master_priority_calculator(
            fm->grandmasterPriority1, fm->grandmasterPriority2, &PACKET_MASTER(el));

        /*
         * Extract basic clockClass statistics accummulated over foreignMasterDs
         * query window to facilitate an appraisal of reduction in QL conditions.
         */
        PACKET_MASTER(el).hql.clock_class_window = fm->clockClass_window;

        /*
         * Extract elapsed times since receipt of messages from foreignMasterDS.
         * NOTE: A master in foreignMasterDS implies PTSF lossAnnounce is false.
         *       Set elapsed time of last announce message to zero to guarantee.
         */
        PACKET_MASTER(el).elapsed_times.announce = 0;
        PACKET_MASTER(el).elapsed_times.sync = fm->ms_since_sync;
        PACKET_MASTER(el).elapsed_times.sync_ts = fm->ms_since_sync_ts;
        PACKET_MASTER(el).elapsed_times.delay_resp = fm->ms_since_del_resp;

        /*
         * Zero elapsed times since receipt of Sync, Follow_Up (Sync timestamp), and Delay_Resp
         * if portState is neither slave nor uncalibrated for consistency with firmware Rx/Tx.
         */
        if (BCM_SUCCESS(_bcm_ptp_clock_cache_port_state_get(unit, ptp_id, clock_num,
                PTP_CLOCK_PORT_NUMBER_DEFAULT, &portState)) &&
                (portState != _bcm_ptp_state_uncalibrated) && (portState != _bcm_ptp_state_slave)) {
            PACKET_MASTER(el).elapsed_times.sync = 0;
            PACKET_MASTER(el).elapsed_times.sync_ts = 0;
            PACKET_MASTER(el).elapsed_times.delay_resp = 0;
        }

        COMPILER_64_SET(PACKET_MASTER(el).pktstats.pdv_scaled_allan_var,
            COMPILER_64_HI(fm->pdv_scaled_allan_var),
            COMPILER_64_LO(fm->pdv_scaled_allan_var));

        PACKET_MASTER(el).state = bcm_ptp_telecom_g8265_pktmaster_state_valid;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_packet_master_priority_calculator
 * Purpose:
 *      Calculate packet master priority based on associated grandmaster's
 *      priority1 and priority2 attributes.
 * Parameters:
 *      gmPriority1 - (IN) PTP grandmaster clock priority1.
 *      gmPriority2 - (IN) PTP grandmaster clock priority2.
 *      pktmaster   - (IN/OUT) Packet master clock.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_packet_master_priority_calculator(
    uint8 gmPriority1,
    uint8 gmPriority2,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    pktmaster->grandmaster_priority1 = gmPriority1;
    pktmaster->grandmaster_priority2 = gmPriority2;

    if (1 == pktmaster->priority.override) {
        /*
         * Packet master priority calculated automatically from PTP priority1
         * and priority2 attributes, which are available in Announce messages
         * for example. Otherwise (i.e., override equals FALSE), the priority
         * is prescribed via host API calls.
         */
        pktmaster->priority.value = (gmPriority1 << 8) + gmPriority2;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_packet_master_manager
 * Purpose:
 *      Manage/assign packet master array resources and identify/lookup the
 *      entry with matching address.
 * Parameters:
 *      address - (IN)  Packet master address.
 *      index   - (OUT) Packet master index.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_packet_master_manager(
    bcm_ptp_clock_port_address_t *address,
    int *index)
{
    int rv;
    int el;

    /* Existing packet master array entry. */
    if (BCM_SUCCESS(rv = _bcm_ptp_telecom_g8265_packet_master_lookup(address, index))) {
        return BCM_E_NONE;
    }

    /* New packet master array entry. */
    for (el = 0; el < PTP_TELECOM_MAX_NUMBER_PKTMASTERS; ++el) {
        if (bcm_ptp_telecom_g8265_pktmaster_state_unused == PACKET_MASTER(el).state) {
            *index = el;
            return BCM_E_NONE;
        }
    }

    /* No available packet master array entries. */
    *index = -1;
    return BCM_E_FULL;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_packet_master_lookup
 * Purpose:
 *      Look up a packet master in the packet master array based on address.
 * Parameters:
 *      address - (IN)  Packet master address.
 *      index   - (OUT) Packet master array index.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_packet_master_lookup(
    bcm_ptp_clock_port_address_t *address,
    int *index)
{
    int el;

    /* Existing packet master array entry. */
    for (el = 0; el < PTP_TELECOM_MAX_NUMBER_PKTMASTERS; ++el) {
        if (bcm_ptp_telecom_g8265_pktmaster_state_unused != PACKET_MASTER(el).state) {
            /* Scan packet master array for entry with matching address. */
            if (_bcm_ptp_port_address_compare(address, &PACKET_MASTER(el).port_address)) {
                /* Select corresponding packet master. */
                *index = el;
                return BCM_E_NONE;
            }
        }
    }

    return BCM_E_NOT_FOUND;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_packet_master_make
 * Purpose:
 *      Initialize new packet master.
 * Parameters:
 *      address   - (IN)  Packet master address.
 *      pktmaster - (OUT) Packet master.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_packet_master_make(
    bcm_ptp_clock_port_address_t *address,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    if (NULL == pktmaster) {
        return BCM_E_FAIL;
    }

    /* Set address. */
    pktmaster->port_address = *address;

    /* Set state. */
    pktmaster->state = bcm_ptp_telecom_g8265_pktmaster_state_init;

    /* Set attributes. */
    pktmaster->priority.override = 1;
    _bcm_ptp_telecom_g8265_packet_master_priority_calculator(
        PTP_CLOCK_PRESETS_PRIORITY1_MAXIMUM, PTP_CLOCK_PRESETS_PRIORITY2_MAXIMUM,
        pktmaster);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_packet_master_printout
 * Purpose:
 *      Print packet master information.
 * Parameters:
 *      pktmaster - (IN) Packet master.
 *      flags     - (IN) Printout control flags.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
static int
_bcm_ptp_telecom_g8265_packet_master_printout(
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster,
    uint8 flags)
{
    if (NULL == pktmaster) {
        return BCM_E_FAIL;
    }

    

    switch (pktmaster->port_address.addr_type) {
    case bcmPTPUDPIPv4:
        LOG_CLI((BSL_META("Master IP Address (IPv4): %d.%d.%d.%d\n"),
                 pktmaster->port_address.address[0],
                 pktmaster->port_address.address[1],
                 pktmaster->port_address.address[2],
                 pktmaster->port_address.address[3]));
        break;
    case bcmPTPUDPIPv6:
        LOG_CLI((BSL_META("Master IP Address (IPv6): %02x%02x:%02x%02x:%02x%02x:"
                          "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n"),
                 pktmaster->port_address.address[0],
                 pktmaster->port_address.address[1],
                 pktmaster->port_address.address[2],
                 pktmaster->port_address.address[3],
                 pktmaster->port_address.address[4],
                 pktmaster->port_address.address[5],
                 pktmaster->port_address.address[6],
                 pktmaster->port_address.address[7],
                 pktmaster->port_address.address[8],
                 pktmaster->port_address.address[9],
                 pktmaster->port_address.address[10],
                 pktmaster->port_address.address[11],
                 pktmaster->port_address.address[12],
                 pktmaster->port_address.address[13],
                 pktmaster->port_address.address[14],
                 pktmaster->port_address.address[15]));
        break;
    default:
        LOG_CLI((BSL_META("Master IP Address:\n")));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_ptp_telecom_g8265_event_description
 * Purpose:
 *      Interpret telecom event(s).
 * Parameters:
 *      type - (IN) Telecom event(s).
 * Returns:
 *      Telecom event description.
 * Notes:
 */
static const char*
_bcm_ptp_telecom_g8265_event_description(
    uint32 type)
{
    if (type & (1u << PTP_TELECOM_EVENT_PTSF_LOSS_ANNOUNCE_BIT)) {
        return "PTSF lossAnnounce";
    } else if (type & (1u << PTP_TELECOM_EVENT_PTSF_LOSS_SYNC_BIT)) {
        return "PTSF lossSync (lossTimingSync)";
    } else if (type & (1u << PTP_TELECOM_EVENT_PTSF_LOSS_SYNC_TS_BIT)) {
        return "PTSF lossSync (lossTimingSyncTS)";
    } else if (type & (1u << PTP_TELECOM_EVENT_PTSF_LOSS_DELAY_RESP_BIT)) {
        return "PTSF lossSync (lossTimingDelay)";
    } else if (type & (1u << PTP_TELECOM_EVENT_PTSF_UNUSABLE_BIT)) {
        return "PTSF unusable";
    } else if (type & (1u << PTP_TELECOM_EVENT_QL_DEGRADED_BIT)) {
        return "QL Degraded";
    } else if (type & (1u << PTP_TELECOM_EVENT_GM_ADDED_BIT)) {
        return "MASTER Added";
    } else if (type & (1u << PTP_TELECOM_EVENT_GM_REMOVED_BIT)) {
        return "MASTER Removed";
    } else if (type & (1u << PTP_TELECOM_EVENT_GM_SELECTED_BIT)) {
        return "MASTER Selected";
    } else {
        return "UNKNOWN";
    }
}

#endif /* defined(INCLUDE_PTP) */
