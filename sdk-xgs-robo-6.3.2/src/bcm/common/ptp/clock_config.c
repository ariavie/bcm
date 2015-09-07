/*
 * $Id: clock_config.c 1.1 Broadcom SDK $
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
 * File:    clock_config.c
 *
 * Purpose: 
 *
 * Functions:
 *      bcm_common_ptp_clock_accuracy_get
 *      bcm_common_ptp_clock_accuracy_set
 *      bcm_common_ptp_clock_current_dataset_get
 *      bcm_common_ptp_clock_default_dataset_get
 *      bcm_common_ptp_clock_domain_get
 *      bcm_common_ptp_clock_domain_set
 *      bcm_common_ptp_clock_get
 *      bcm_common_ptp_clock_parent_dataset_get
 *      bcm_common_ptp_clock_port_announce_receipt_timeout_get
 *      bcm_common_ptp_clock_port_announce_receipt_timeout_set
 *      bcm_common_ptp_clock_port_configure
 *      bcm_common_ptp_clock_port_dataset_get
 *      bcm_common_ptp_clock_port_delay_mechanism_get
 *      bcm_common_ptp_clock_port_delay_mechanism_set
 *      bcm_common_ptp_clock_port_disable
 *      bcm_common_ptp_clock_port_enable
 *      bcm_common_ptp_clock_port_info_get
 *      bcm_common_ptp_clock_port_latency_get
 *      bcm_common_ptp_clock_port_latency_set
 *      bcm_common_ptp_clock_port_log_announce_interval_get
 *      bcm_common_ptp_clock_port_log_announce_interval_set
 *      bcm_common_ptp_clock_port_log_min_delay_req_interval_get
 *      bcm_common_ptp_clock_port_log_min_delay_req_interval_set
 *      bcm_common_ptp_clock_port_log_min_pdelay_req_interval_get
 *      bcm_common_ptp_clock_port_log_min_pdelay_req_interval_set
 *      bcm_common_ptp_clock_port_log_sync_interval_get
 *      bcm_common_ptp_clock_port_log_sync_interval_set
 *      bcm_common_ptp_clock_port_mac_get
 *      bcm_common_ptp_clock_port_type_get
 *      bcm_common_ptp_clock_port_version_number_get
 *      bcm_common_ptp_clock_port_version_number_set
 *      bcm_common_ptp_clock_priority1_get
 *      bcm_common_ptp_clock_priority1_set
 *      bcm_common_ptp_clock_priority2_get
 *      bcm_common_ptp_clock_priority2_set
 *      bcm_common_ptp_clock_slaveonly_get
 *      bcm_common_ptp_clock_slaveonly_set
 *      bcm_common_ptp_clock_time_get
 *      bcm_common_ptp_clock_time_properties_get
 *      bcm_common_ptp_clock_time_set
 *      bcm_common_ptp_clock_timescale_get
 *      bcm_common_ptp_clock_timescale_set
 *      bcm_common_ptp_clock_traceability_get
 *      bcm_common_ptp_clock_traceability_set
 *      bcm_common_ptp_clock_user_description_set
 *      bcm_common_ptp_clock_utc_get
 *      bcm_common_ptp_clock_utc_set
 *      bcm_common_ptp_foreign_master_dataset_get
 *
 *      _bcm_ptp_clock_port_cache_info_get
 *      _bcm_ptp_clock_port_cache_info_set
 *      _bcm_ptp_clock_cache_port_state_get
 *      _bcm_ptp_clock_cache_port_state_set
 */

#if defined(INCLUDE_PTP)

#include <soc/defs.h>
#include <soc/drv.h>

#include <sal/core/boot.h>

#include <bcm/ptp.h>
#include <bcm_int/common/ptp.h>
#include <bcm_int/ptp_common.h>
#include <bcm/error.h>

static const bcm_ptp_port_identity_t portid_all =
    {{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, PTP_IEEE1588_ALL_PORTS};

/*
 * Function:
 *      _bcm_ptp_clock_port_cache_info_get
 * Purpose:
 *      Get the clock port information cache of a PTP clock.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      port_info  - (OUT) PTP clock port information cache.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int _bcm_ptp_clock_port_cache_info_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_clock_port_info_t *port_info)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate !=
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    if ((clock_port < 1) ||
        (clock_port > _bcm_common_ptp_unit_array[unit].stack_array[unit]
                                      .clock_array[clock_num]
                                      .clock_info.num_ports)) {
        return BCM_E_PORT;
    }

    *port_info = _bcm_common_ptp_unit_array[unit].stack_array[ptp_id]
                                 .clock_array[clock_num]
                                 .port_info[clock_port-1];

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_port_cache_info_set
 * Purpose:
 *      Set the clock port information cache of a PTP clock.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      port_info  - (IN) PTP clock port information cache.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int _bcm_ptp_clock_port_cache_info_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    const bcm_ptp_clock_port_info_t port_info)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate !=
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    if ((clock_port < 1) ||
        (clock_port > _bcm_common_ptp_unit_array[unit].stack_array[unit]
                                      .clock_array[clock_num]
                                      .clock_info.num_ports)) {
        return BCM_E_PORT;
    }

    _bcm_common_ptp_unit_array[unit].stack_array[ptp_id]
                    .clock_array[clock_num]
                    .port_info[clock_port-1] = port_info;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_port_state_get
 * Purpose:
 *      Get a PTP port's portState from PTP clock information cache.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      portState  - (OUT) PTP clock port state.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_clock_cache_port_state_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    _bcm_ptp_port_state_t *portState)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate !=
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    if (clock_port < 1 ||
        clock_port > _bcm_common_ptp_unit_array[unit].stack_array[unit]
                     .clock_array[clock_num].clock_info.num_ports) {
        return BCM_E_PORT;
    }

    *portState = _bcm_common_ptp_unit_array[unit].stack_array[ptp_id]
                     .clock_array[clock_num].portState[clock_port-1];

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_cache_port_state_set
 * Purpose:
 *      Set a PTP port's portState in PTP clock information cache.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      portState  - (IN) PTP clock port state.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_clock_cache_port_state_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    const _bcm_ptp_port_state_t portState)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if ((_bcm_common_ptp_unit_array[unit].memstate != PTP_MEMSTATE_INITIALIZED) ||
        (_bcm_common_ptp_unit_array[unit].stack_array[ptp_id].memstate !=
            PTP_MEMSTATE_INITIALIZED)) {
        return BCM_E_UNAVAIL;
    }

    if (clock_port < 1 ||
        clock_port > _bcm_common_ptp_unit_array[unit].stack_array[unit]
                     .clock_array[clock_num].clock_info.num_ports) {
        return BCM_E_PORT;
    }

    _bcm_common_ptp_unit_array[unit].stack_array[ptp_id]
        .clock_array[clock_num].portState[clock_port-1] = portState;

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_get
 * Purpose:
 *      Get configuration information (data and metadata) of a PTP clock.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_info - (OUT) PTP clock information.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_common_ptp_clock_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_clock_info_t *clock_info)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv =
        _bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, clock_info))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_time_get
 * Purpose:
 *      Get local time of a PTP node.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      time      - (OUT) PTP timestamp.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapter 15.5.3.2.1.
 */
int
bcm_common_ptp_clock_time_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_timestamp_t *time)
{
    int rv = BCM_E_UNAVAIL;
    uint64 seconds;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_TIME, 0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0...9 : Timestamp. 48-bit seconds, 32-bit nanoseconds.
         *    NOTICE: bcm_ptp_timestamp_t seconds member is 64-bit value.
         */
        seconds = _bcm_ptp_uint64_read(resp);
        COMPILER_64_SHR(seconds, 16); 
        COMPILER_64_SET(time->seconds, 
                COMPILER_64_HI(seconds), COMPILER_64_LO(seconds));
        time->nanoseconds = _bcm_ptp_uint32_read(resp+6);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_time_set
 * Purpose:
 *      Set local time of a PTP node.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      time      - (IN) PTP timestamp.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Function applies to a grandmaster node only.
 *      Ref. IEEE Std. 1588-2008, Chapter 15.5.3.2.1.
 */
int
bcm_common_ptp_clock_time_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_timestamp_t *time)
{
    int rv = BCM_E_UNAVAIL;
    uint64 seconds;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_TIME_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0...9 : Timestamp. 48-bit seconds, 32-bit nanoseconds.
     *    NOTICE: bcm_ptp_timestamp_t seconds member is 64-bit value.
     */
    COMPILER_64_SET(seconds, 
                COMPILER_64_HI(time->seconds), COMPILER_64_LO(time->seconds));
    COMPILER_64_SHL(seconds, 16); 
    _bcm_ptp_uint64_write(payload, seconds);
    _bcm_ptp_uint32_write(payload+6, time->nanoseconds);

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_TIME,
            payload, PTP_MGMTMSG_PAYLOAD_TIME_SIZE_OCTETS,
            resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_user_description_set
 * Purpose:
 *      Set user description member of the default dataset.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      desc      - (IN) User description.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapter 15.5.3.1.3.
 */
int
bcm_common_ptp_clock_user_description_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 *desc)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MAX_USER_DESCRIPTION_SIZE_OCTETS] = {0};
    int desc_len;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0         : User description length (octets).
     *    Octet 1...(L-1) : User description.
     *    Octet L         : Conditional payload padding.
     */
    for (desc_len = 0;
         desc_len < (PTP_MGMTMSG_PAYLOAD_MAX_USER_DESCRIPTION_SIZE_OCTETS-1);
         ++desc_len) {
        if (desc[desc_len] == 0) {
            break;
        }
    }

    payload[0] = desc_len;
    sal_memcpy(payload+1, desc, desc_len);
    /*
     * NOTICE: Per IEEE Std. 1588-2008, Chapter 15.5.3.1.3, pad logic is
     *         used to yield an even-octet-sized management TLV data field
     *         with assumption that maximum USER_DESCRIPTION size is even.
     */
    if ((desc_len % 2) == 0) {
        ++desc_len;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_USER_DESCRIPTION,
            payload, (1+desc_len), resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_default_dataset_get
 * Purpose:
 *      Get PTP clock default dataset.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      dataset   - (OUT) Default dataset.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The default dataset is relevant to PTP ordinary and boundary clocks.
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.1 and 15.5.3.3.
 */
int
bcm_common_ptp_clock_default_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_default_dataset_t *dataset)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    int i = 0;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_DEFAULT_DATASET,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0: Two-step (TSC) and slave-only (SO) Booleans.
         *             |B7|B6|B5|B4|B3|B2|B1|B0| = |0|0|0|0|0|0|SO|TSC|.
         *    Octet 1       : Reserved.
         *    Octet 2...3   : Number of ports.
         *    Octet 4       : Priority1.
         *    Octet 5...8   : Clock quality.
         *    Octet 9       : Priority2.
         *    Octet 10...17 : Clock identity.
         *    Octet 18      : PTP domain number.
         */
        i = 0;
        dataset->flags = (0x03 & resp[i]);
        i = 2;
        dataset->number_ports = _bcm_ptp_uint16_read(resp+i);
        i += sizeof(uint16);
        dataset->priority1 = resp[i++];

        dataset->clock_quality.clock_class = resp[i++];
        dataset->clock_quality.clock_accuracy = resp[i++];
        dataset->clock_quality.offset_scaled_log_variance =
            _bcm_ptp_uint16_read(resp+i);
        i += sizeof(uint16);

        dataset->priority2 = resp[i++];
        sal_memcpy(dataset->clock_identity,
                   resp+i, sizeof(bcm_ptp_clock_identity_t));
        i += sizeof(bcm_ptp_clock_identity_t);
        dataset->domain_number = resp[i];
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_current_dataset_get
 * Purpose:
 *      Get PTP clock current dataset.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      dataset   - (OUT) Current dataset.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The current dataset is relevant to PTP ordinary and boundary clocks.
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.2 and 15.5.3.4.
 */
int
bcm_common_ptp_clock_current_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_current_dataset_t *dataset)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    int i = 0;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_CURRENT_DATASET,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0...1   : Steps removed from master.
         *    Octet 2...9   : Offset from master (scaled nanoseconds).
         *    Octet 10...17 : Mean path delay (scaled nanoseconds).
         */
        i = 0;
        dataset->steps_removed = _bcm_ptp_uint16_read(resp);
        i += sizeof(uint16);
        dataset->offset_from_master = _bcm_ptp_uint64_read(resp+i);
        i += sizeof(uint64);
        dataset->mean_path_delay = _bcm_ptp_uint64_read(resp+i);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_parent_dataset_get
 * Purpose:
 *      Get PTP clock parent dataset.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      dataset   - (OUT) Parent dataset.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The parent dataset is relevant to PTP ordinary and boundary clocks.
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.3 and 15.5.3.5.
 */
int
bcm_common_ptp_clock_parent_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_parent_dataset_t *dataset)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    int i = 0;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_PARENT_DATASET,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0...9   : Parent port identity.
         *    Octet 10      : Parent statistics (PS) Boolean.
         *                    |B7|B6|B5|B4|B3|B2|B1|B0| = |0|0|0|0|0|0|0|PS|.
         *    Octet 11      : Reserved.
         *    Octet 12...13 : Observed parent offset scaled log variance.
         *    Octet 14...17 : Observed parent clock phase change rate.
         *    Octet 18      : Grandmaster clock Priority1.
         *    Octet 19...22 : Grandmaster clock quality.
         *    Octet 23      : Grandmaster clock Priority2.
         *    Octet 24...31 : Grandmaster clock identity.
         */
        i = 0;
        sal_memcpy(dataset->parent_port_identity.clock_identity, resp,
                   sizeof(bcm_ptp_clock_identity_t));
        i += sizeof(bcm_ptp_clock_identity_t);
        dataset->parent_port_identity.port_number = _bcm_ptp_uint16_read(resp+i);
        i += sizeof(uint16);

        dataset->ps = (0x01 & resp[i]);
        i += 2;

        dataset->observed_parent_offset_scaled_log_variance =
            _bcm_ptp_uint16_read(resp+i);
        i += sizeof(uint16);

        dataset->observed_parent_clock_phase_change_rate =
            _bcm_ptp_uint32_read(resp+i);
        i += sizeof(uint32);

        dataset->grandmaster_priority1 = resp[i++];

        dataset->grandmaster_clock_quality.clock_class = resp[i++];
        dataset->grandmaster_clock_quality.clock_accuracy = resp[i++];

        dataset->grandmaster_clock_quality.offset_scaled_log_variance =
            _bcm_ptp_uint16_read(resp+i);
        i += sizeof(uint16);

        dataset->grandmaster_priority2 = resp[i++];

        sal_memcpy(dataset->grandmaster_identity, resp+i,
                   sizeof(bcm_ptp_clock_identity_t));
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_time_properties_get
 * Purpose:
 *      Get PTP clock time properties dataset.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      dataset   - (OUT) Time properties dataset.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The time properties dataset is relevant to PTP ordinary and boundary clocks.
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.4 and 15.5.3.6.
 */
int
bcm_common_ptp_clock_time_properties_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_time_properties_t *dataset)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    int i = 0;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_TIME_PROPERTIES_DATASET,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0...1 : UTC offset.
         *    Octet 2     : Leap-61 (LI-61), Leap-59 (LI-59), Valid UTC
         *                  Offset (UTCV), PTP timescale, time traceable
         *                  (TTRA), and frequency traceable (FTRA) Booleans.
         *                  |B7|B6|B5|B4|B3|B2|B1|B0| =
         *                  |0|0|FTRA|TTRA|PTP|UTCV|LI-59|LI-61|.
         *    Octet 3     : Time source.
         */
        i = 0;
        dataset->utc_info.utc_offset = _bcm_ptp_uint16_read(resp);
        i += sizeof(uint16);

        dataset->utc_info.leap61 = (0x01 & resp[i]);
        dataset->utc_info.leap59 = ((0x02 & resp[i]) >> 1);
        dataset->utc_info.utc_valid = ((0x04 & resp[i]) >> 2);

        dataset->trace_info.time_traceable = ((0x10 & resp[i]) >> 4);
        dataset->trace_info.frequency_traceable = ((0x20 & resp[i]) >> 5);

        dataset->timescale_info.ptp_timescale = ((0x08 & resp[i++]) >> 3);
        dataset->timescale_info.time_source = (bcm_ptp_time_source_t)resp[i];
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_foreign_master_dataset_get
 * Purpose:
 *      Get the foreign master dataset of a PTP clock port.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      data_set  - (OUT) Foreign master dataset.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_common_ptp_foreign_master_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_foreign_master_dataset_t *data_set)
{
    int rv = BCM_E_NONE;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    unsigned num_masters;
    unsigned offset;
    unsigned index;

    BCM_IF_ERROR_RETURN(
        _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
                                   PTP_CLOCK_PORT_NUMBER_DEFAULT));

    BCM_IF_ERROR_RETURN(
        bcm_common_ptp_clock_port_identity_get(unit, ptp_id, clock_num,
                                            port_num, &portid));

    BCM_IF_ERROR_RETURN(
        _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
                                         PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_FOREIGN_MASTER_DATASET,
                                         0, 0, resp, &resp_len));

    /*
     * Parse response.
     *    Octet  0      : Number of foreign masters.
     *    Octet  1      : Current best master index.
     *    Octet  2..9   : FM#0 clockIdentity.
     *    Octet 10..11  : FM#0 Port number.
     *    Octet 12      : FM#0 Address type.
     *    Octet 13..28  : FM#0 Address.
     *    Octet 29      : FM#0 clockClass.
     *    Octet 30      : FM#0 grandmasterPriority1.
     *    Octet 31      : FM#0 grandmasterPriority2.
     *    Octet 32      : FM#0 Minimum clockClass since last query.
     *    Octet 33      : FM#0 Maximum clockClass since last query.
     *    Octet 34      : FM#0 Number of clockClass decrease events since last query.
     *    Octet 35      : FM#0 Number of clockClass increase events since last query.
     *    Octet 36..39  : FM#0 Milliseconds since Sync (max 1 hr).
     *    Octet 40..43  : FM#0 Milliseconds since Sync/Followup TS (max 1 hr).
     *    Octet 44..47  : FM#0 Milliseconds since Delay_Resp (max 1 hr).
     *    Octet 48..49  : FM#0 Announce messages in window.
     *    Octet 50..57  : FM#0 PDV scaled Allan variance (PTP variance of MTSDs).
     *    Octet 58..113 : FM#1 (as FM#0).
     *    ...
     */

    num_masters = resp[0];
    if (num_masters > _PTP_MAX_FOREIGN_MASTER_DATASET_ENTRIES) {
        num_masters = _PTP_MAX_FOREIGN_MASTER_DATASET_ENTRIES;
        rv = BCM_E_FULL;
    }

    if (resp[1] != 0xff) {
        data_set->current_best_master = resp[1];
    } else {
        data_set->current_best_master = 0;
    }

    index = 0;
    offset = 2;

    while (index < num_masters) {
        bcm_ptp_foreign_master_entry_t *fm = &data_set->foreign_master[index];

        sal_memcpy(fm->port_identity.clock_identity, resp + offset, BCM_PTP_CLOCK_EUID_IEEE1588_SIZE);
        offset += BCM_PTP_CLOCK_EUID_IEEE1588_SIZE;

        fm->port_identity.port_number = _bcm_ptp_uint16_read(resp + offset);
        offset += 2;

        fm->address.addr_type = *(resp + offset);
        offset += 1;

        memcpy(&fm->address.address[0], resp + offset, BCM_PTP_MAX_NETW_ADDR_SIZE);
        offset += BCM_PTP_MAX_NETW_ADDR_SIZE;

        fm->clockClass = *(resp + offset++);
        fm->grandmasterPriority1 = *(resp + offset++);
        fm->grandmasterPriority2 = *(resp + offset++);

        fm->clockClass_window.min = *(resp + offset++);
        fm->clockClass_window.max = *(resp + offset++);
        fm->clockClass_window.fall_events = *(resp + offset++);
        fm->clockClass_window.rise_events = *(resp + offset++);

        fm->ms_since_sync = _bcm_ptp_uint32_read(resp + offset);
        offset += 4;

        fm->ms_since_sync_ts = _bcm_ptp_uint32_read(resp + offset);
        offset += 4;

        fm->ms_since_del_resp = _bcm_ptp_uint32_read(resp + offset);
        offset += 4;

        fm->announce_messages = _bcm_ptp_uint16_read(resp + offset);
        offset += 2;

        fm->pdv_scaled_allan_var = _bcm_ptp_uint64_read(resp + offset);
        offset += 8;

        ++index;
    }

    data_set->num_foreign_masters = num_masters;

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_priority1_get
 * Purpose:
 *      Get priority1 member of a PTP clock default dataset.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      priority1 - (OUT) PTP clock priority1.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.1 and 15.5.3.3.2.
 */
int
bcm_common_ptp_clock_priority1_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 *priority1)
{
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    BCM_IF_ERROR_RETURN(
        _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
                                   PTP_CLOCK_PORT_NUMBER_DEFAULT));

    BCM_IF_ERROR_RETURN(
        bcm_common_ptp_clock_port_identity_get(unit, ptp_id, clock_num,
                                            PTP_IEEE1588_ALL_PORTS, &portid));

    BCM_IF_ERROR_RETURN(
        _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
                                         PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_PRIORITY1,
                                         0, 0, resp, &resp_len));

    /*
     * Parse response.
     *    Octet 0 : Priority1.
     *    Octet 1 : Reserved.
     */
    *priority1 = resp[0];

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_clock_priority1_set
 * Purpose:
 *      Set priority1 member of a PTP clock default dataset.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      priority1 - (IN) PTP clock priority1.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.1 and 15.5.3.3.2.
 */
int
bcm_common_ptp_clock_priority1_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 priority1)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_clock_info_t ci;
    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    /* PTP attribute profile range checking. */
    if (BCM_FAILURE(rv =
            _bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, &ci))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    if ((priority1 < ci.priority1_minimum) ||
            (priority1 > ci.priority1_maximum)) {
        /*
         * Caller provided priority1 value is not in (min,max) range of
         * PTP profile.
         */
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : Priority1.
     *    Octet 1 : Reserved.
     */
    payload[0] = (uint8)priority1;

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_PRIORITY1,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    /* Update PTP clock cache. */
    if (rv == BCM_E_NONE) {
        ci.priority1 = (uint8)priority1;
        rv = _bcm_ptp_clock_cache_info_set(unit, ptp_id, clock_num, ci);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_priority2_get
 * Purpose:
 *      Get priority2 member of a PTP clock default dataset.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      priority2 - (OUT) PTP clock priority2.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.1 and 15.5.3.3.3.
 */
int
bcm_common_ptp_clock_priority2_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 *priority2)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_PRIORITY2,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : Priority2.
         *    Octet 1 : Reserved.
         */
        *priority2 = resp[0];
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_priority2_set
 * Purpose:
 *      Set priority2 member of a PTP clock default dataset.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      priority2 - (IN) PTP clock priority2.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.1 and 15.5.3.3.3.
 */
int
bcm_common_ptp_clock_priority2_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 priority2)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_clock_info_t ci;
    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    /* PTP attribute profile range checking. */
    if (BCM_FAILURE(rv =
            _bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, &ci))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    if ((priority2 < ci.priority2_minimum) ||
            (priority2 > ci.priority2_maximum)) {
        /*
         * Caller provided priority2 value is not in (min,max) range of
         * PTP profile.
         */
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : Priority2.
     *    Octet 1 : Reserved.
     */
    payload[0] = (uint8)priority2;

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_PRIORITY2,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    /* Update PTP clock cache. */
    if (rv == BCM_E_NONE) {
        ci.priority2 = (uint8)priority2;
        rv = _bcm_ptp_clock_cache_info_set(unit, ptp_id, clock_num, ci);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_domain_get
 * Purpose:
 *      Get PTP domain member of a PTP clock default dataset.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      domain    - (OUT) PTP clock domain.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.1 and 15.5.3.3.4.
 */
int
bcm_common_ptp_clock_domain_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 *domain)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_DOMAIN,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : PTP domain.
         *    Octet 1 : Reserved.
         */
        *domain = resp[0];
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_domain_set
 * Purpose:
 *      Set PTP domain member of a PTP clock default dataset.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      domain    - (IN) PTP clock domain.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.1 and 15.5.3.3.4.
 */
int
bcm_common_ptp_clock_domain_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 domain)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_clock_info_t ci;
    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    /* PTP attribute profile range checking. */
    if (BCM_FAILURE(rv =
            _bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, &ci))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    if ((domain < ci.domain_number_minimum) ||
            (domain > ci.domain_number_maximum)) {
        /*
         * Caller provided PTP domain value is not in (min,max) range of
         * PTP profile.
         */
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /* Make payload.
     *    Octet 0 : PTP domain.
     *    Octet 1 : Reserved.
     */
    payload[0] = (uint8)domain;

    if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id,
            clock_num, &portid, PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_DOMAIN,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS, resp, &resp_len))) {
            return rv;
    }

    /* Remove signaled slaves. */
    if (BCM_FAILURE(rv = bcm_common_ptp_signaled_unicast_slave_table_clear(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, 0))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    /* Remove static slaves. */
    if (BCM_FAILURE(rv = _bcm_ptp_unicast_slave_host_reset(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    /*
     * Register domain with PTP management framework.
     * Ensure subsequent management messages are configured with proper
     * domain.
     */
    if (BCM_FAILURE(rv = _bcm_ptp_management_domain_set(unit, ptp_id,
            clock_num, domain))) {
        return rv;
    }

    /* Update PTP clock cache. */
    ci.domain_number = (uint8)domain;
    rv = _bcm_ptp_clock_cache_info_set(unit, ptp_id, clock_num, ci);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_slaveonly_get
 * Purpose:
 *      Get slave-only (SO) flag of a PTP clock default dataset.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      slaveonly - (OUT) PTP clock slave-only (SO) flag.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.1 and 15.5.3.3.5.
 */
int
bcm_common_ptp_clock_slaveonly_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 *slaveonly)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_SLAVE_ONLY,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : Slave-only (SO) Boolean.
         *              |B7|B6|B5|B4|B3|B2|B1|B0| = |0|0|0|0|0|0|0|SO|.
         *    Octet 1 : Reserved.
         */
        *slaveonly = (0x01 & resp[0]);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_slaveonly_set
 * Purpose:
 *      Set slave-only (SO) flag of a PTP clock default dataset.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      slaveonly - (IN) PTP clock slave-only (SO) flag.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.1 and 15.5.3.3.5.
 */
int
bcm_common_ptp_clock_slaveonly_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 slaveonly)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_clock_info_t ci;
    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : Slave-only (SO) Boolean.
     *              |B7|B6|B5|B4|B3|B2|B1|B0| = |0|0|0|0|0|0|0|SO|.
     *    Octet 1 : Reserved.
     */
    payload[0] = (uint8)(1 & slaveonly);

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_SLAVE_ONLY,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    /* Update PTP clock cache. */
    if (rv == BCM_E_NONE) {
        if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_info_get(unit, ptp_id,
                clock_num, &ci))) {
            soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
            return rv;
        }

        ci.slaveonly = (uint8)(1 & slaveonly);
        rv = _bcm_ptp_clock_cache_info_set(unit, ptp_id, clock_num, ci);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_accuracy_get
 * Purpose:
 *      Get clock quality, clock accuracy member of a PTP clock default dataset.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      accuracy  - (OUT) PTP clock accuracy.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.1 and 15.5.3.2.2.
 */
int
bcm_common_ptp_clock_accuracy_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_clock_accuracy_t *accuracy)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_CLOCK_ACCURACY,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : PTP clock accuracy.
         *    Octet 1 : Reserved.
         */
        *accuracy = (bcm_ptp_clock_accuracy_t)resp[0];
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_accuracy_set
 * Purpose:
 *      Set clock quality, clock accuracy member of a PTP clock default dataset.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      accuracy  - (IN) PTP clock accuracy.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Function applies to a grandmaster node only.
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.1 and 15.5.3.2.2.
 */
int
bcm_common_ptp_clock_accuracy_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_clock_accuracy_t *accuracy)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : PTP clock accuracy.
     *    Octet 1 : Reserved.
     */
    payload[0] = (uint8)(*accuracy);

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_CLOCK_ACCURACY,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_timescale_get
 * Purpose:
 *      Get timescale members of a PTP clock time properties dataset.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      timescale - (OUT) PTP clock timescale properties.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapter 15.5.3.6.4.
 */
int
bcm_common_ptp_clock_timescale_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_timescale_t *timescale)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_TIMESCALE_PROPERTIES,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : PTP timescale Boolean.
         *              |B7|B6|B5|B4|B3|B2|B1|B0| = |0|0|0|0|PTP|0|0|0|.
         *    Octet 1 : Time source.
         */
        timescale->ptp_timescale = ((0x08 & resp[0]) >> 3);
        timescale->time_source = (bcm_ptp_time_source_t)resp[1];
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_timescale_set
 * Purpose:
 *      Set timescale members of a PTP clock time properties dataset.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      timescale - (IN) PTP clock timescale properties.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapter 15.5.3.6.4.
 */
int
bcm_common_ptp_clock_timescale_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_timescale_t *timescale)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : PTP timescale Boolean.
     *              |B7|B6|B5|B4|B3|B2|B1|B0| = |0|0|0|0|PTP|0|0|0|.
     *    Octet 1 : Time source.
     */
    payload[0] = ((0x01 & timescale->ptp_timescale) << 3);
    payload[1] = (uint8)(timescale->time_source);

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_TIMESCALE_PROPERTIES,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_traceability_get
 * Purpose:
 *      Get traceability members of a PTP clock time properties dataset.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP stack ID.
 *      clock_num - (IN)  PTP clock number.
 *      trace     - (OUT) PTP clock traceability properties.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapter 15.5.3.6.3.
 */
int
bcm_common_ptp_clock_traceability_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_trace_t *trace)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_TRACEABILITY_PROPERTIES,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : Time traceable (TTRA) and frequency traceable (FTRA) Booleans.
         *              |B7|B6|B5|B4|B3|B2|B1|B0| = |0|0|FTRA|TTRA|0|0|0|0|.
         *    Octet 1 : Reserved.
         */
        trace->time_traceable = ((0x10 & resp[0]) >> 4);
        trace->frequency_traceable = ((0x20 & resp[0]) >> 5);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_traceability_set
 * Purpose:
 *      Set traceability members of a PTP clock time properties dataset.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP stack ID.
 *      clock_num - (IN) PTP clock number.
 *      trace     - (IN) PTP clock traceability properties.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapter 15.5.3.6.3.
 */
int
bcm_common_ptp_clock_traceability_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_trace_t *trace)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : Time traceable (TTRA) and frequency traceable (FTRA) Booleans.
     *              |B7|B6|B5|B4|B3|B2|B1|B0| = |0|0|FTRA|TTRA|0|0|0|0|.
     *    Octet 1 : Reserved.
     */
    payload[0]= ((0x01 & trace->time_traceable) << 4) +
                ((0x01 & trace->frequency_traceable) << 5);

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_TRACEABILITY_PROPERTIES,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_utc_get
 * Purpose:
 *      Get UTC members of a PTP clock time properties dataset.
 * Parameters:
 *      unit      - (IN)  Unit number.
 *      ptp_id    - (IN)  PTP Stack ID
 *      clock_num - (IN)  PTP Clock Number
 *      utc       - (OUT) PTP clock coordinated universal time (UTC) properties.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapter 15.5.3.6.2.
 */
int
bcm_common_ptp_clock_utc_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_utc_t *utc)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    int i = 0;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_UTC_PROPERTIES,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0...1 : UTC offset.
         *    Octet 2     : Leap-61 (LI-61), Leap-59 (LI-59), and valid
         *                  UTC offset (UTCV) Booleans.
         *                  |B7|B6|B5|B4|B3|B2|B1|B0| =
         *                  |0|0|0|0|0|UTCV|LI-59|LI-61|.
         *    Octet 3: Reserved.
         */
        i = 0;
        utc->utc_offset = _bcm_ptp_uint16_read(resp);
        i += sizeof(uint16);

        utc->leap61 = (0x01 & resp[i]);
        utc->leap59 = ((0x02 & resp[i]) >> 1);
        utc->utc_valid = ((0x04 & resp[i]) >> 2);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_utc_set
 * Purpose:
 *      Set UTC members of a PTP clock time properties dataset.
 * Parameters:
 *      unit      - (IN) Unit number.
 *      ptp_id    - (IN) PTP Stack ID
 *      clock_num - (IN) PTP Clock Number
 *      utc       - (IN) PTP clock coordinated universal time (UTC) properties.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapter 15.5.3.6.2.
 */
int
bcm_common_ptp_clock_utc_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_utc_t *utc)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_UTC_PROPERTIES_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    int i = 0;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0...1 : UTC offset.
     *    Octet 2     : Leap-61 (LI-61), Leap-59 (LI-59), and valid UTC
     *                  offset (UTCV) Booleans.
     *                  |B7|B6|B5|B4|B3|B2|B1|B0| =
     *                  |0|0|0|0|0|UTCV|LI-59|LI-61|.
     *    Octet 3: Reserved.
     */
    i = 0;
    _bcm_ptp_uint16_write(payload, utc->utc_offset);
    i += sizeof(uint16);

    payload[i] = ((0x01 & utc->leap61)) +
                  ((0x01 & utc->leap59) << 1) +
                  ((0x01 & utc->utc_valid) << 2);

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_UTC_PROPERTIES,
            payload, PTP_MGMTMSG_PAYLOAD_UTC_PROPERTIES_SIZE_OCTETS,
            resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_type_get
 * Purpose:
 *      Get port type of a PTP clock port.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      type       - (OUT) PTP clock port type.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_common_ptp_clock_port_type_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_port_type_t *type)
{
    int rv = BCM_E_UNAVAIL;

    bcm_ptp_clock_port_info_t pi;

    if (BCM_FAILURE(rv =
        _bcm_ptp_clock_port_cache_info_get(unit, ptp_id, clock_num, clock_port, &pi))) {
        return rv;
    }

    *type = pi.port_type;

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_protocol_get
 * Purpose:
 *      Get network protocol of a PTP clock port.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      protocol   - (OUT) PTP clock port network protocol.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_common_ptp_clock_port_protocol_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_protocol_t *protocol)
{
    int rv = BCM_E_UNAVAIL;

    bcm_ptp_clock_port_info_t pi;

    if (BCM_FAILURE(rv =
        _bcm_ptp_clock_port_cache_info_get(unit, ptp_id, clock_num, clock_port, &pi))) {
        return rv;
    }

    *protocol = pi.port_address.addr_type;

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_mac_get
 * Purpose:
 *      Get MAC address of a PTP clock port.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      mac        - (OUT) PTP clock port MAC address.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_common_ptp_clock_port_mac_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_mac_t *mac)
{
    int rv = BCM_E_UNAVAIL;

    bcm_ptp_clock_port_info_t pi;

    if (BCM_FAILURE(rv =
        _bcm_ptp_clock_port_cache_info_get(unit, ptp_id, clock_num, clock_port, &pi))) {
        return rv;
    }

    sal_memcpy(mac, pi.mac, sizeof(bcm_mac_t));

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_info_get
 * Purpose:
 *      Get configuration information (data and metadata) of a PTP clock port.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      info       - (OUT) PTP clock port configuration information.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_common_ptp_clock_port_info_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_clock_port_info_t *info)
{
    int rv = BCM_E_UNAVAIL;

    if (BCM_FAILURE(rv =
        _bcm_ptp_clock_port_cache_info_get(unit, ptp_id, clock_num, clock_port, info))) {
        return rv;
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_dataset_get
 * Purpose:
 *      Get PTP clock port dataset.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      dataset    - (OUT) Port dataset.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      The port dataset is relevant to PTP ordinary and boundary clocks.
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.
 */
int
bcm_common_ptp_clock_port_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_port_dataset_t *dataset)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    int i = 0;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_PORT_DATASET,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0...9   : PTP port identity.
         *    Octet 10      : Port state.
         *    Octet 11      : Log minimum delay request interval.
         *    Octet 12...19 : Peer mean path delay (nanoseconds).
         *    Octet 20      : Log announce interval.
         *    Octet 21      : Announce receipt timeout.
         *    Octet 22      : Log synchronization interval.
         *    Octet 23      : Delay mechanism.
         *    Octet 24      : Log minimum peer delay request interval.
         *    Octet 25      : |B7|B6|B5|B4|B3|B2|B1|B0|.
         *                    |B4...B7| = Reserved.
         *                    |B0...B3| = Version number.
         */
        i = 0;
        sal_memcpy(dataset->port_identity.clock_identity, resp,
                   sizeof(bcm_ptp_clock_identity_t));
        i += sizeof(bcm_ptp_clock_identity_t);

        dataset->port_identity.port_number = _bcm_ptp_uint16_read(resp+i);
        i += sizeof(uint16);

        dataset->port_state = resp[i++];
        dataset->log_min_delay_req_interval = (int8)resp[i++];

        dataset->peer_mean_path_delay = _bcm_ptp_uint64_read(resp+i);
        i += sizeof(uint64);

        dataset->log_announce_interval = (int8)resp[i++];
        dataset->announce_receipt_timeout = resp[i++];
        dataset->log_sync_interval = (int8)resp[i++];
        dataset->delay_mechanism = resp[i++];
        dataset->log_min_pdelay_req_interval = (int8)resp[i++];

        dataset->version_number = (0x0f & resp[i]);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_delay_mechanism_get
 * Purpose:
 *      Get delay mechanism member of a PTP clock port dataset.
 * Parameters:
 *      unit            - (IN)  Unit number.
 *      ptp_id          - (IN)  PTP stack ID.
 *      clock_num       - (IN)  PTP clock number.
 *      clock_port      - (IN)  PTP clock port number.
 *      delay_mechanism - (OUT) PTP clock port delay mechanism.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.5.
 */
int
bcm_common_ptp_clock_port_delay_mechanism_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 *delay_mechanism)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_DELAY_MECHANISM,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : Delay mechanism.
         *    Octet 1 : Reserved.
         */
        *delay_mechanism = resp[0];
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_delay_mechanism_set
 * Purpose:
 *      Set delay mechanism member of a PTP clock port dataset.
 * Parameters:
 *      unit            - (IN) Unit number.
 *      ptp_id          - (IN) PTP stack ID.
 *      clock_num       - (IN) PTP clock number.
 *      clock_port      - (IN) PTP clock port number.
 *      delay_mechanism - (IN) PTP clock port delay mechanism.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.5.
 */
int
bcm_common_ptp_clock_port_delay_mechanism_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 delay_mechanism)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : Delay mechanism.
     *    Octet 1 : Reserved.
     */
    payload[0] = (uint8)delay_mechanism;

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_DELAY_MECHANISM,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_latency_get
 * Purpose:
 *      Get PTP clock port expected asymmetric latencies.
 * Parameters:
 *      unit        - (IN)  Unit number.
 *      ptp_id      - (IN)  PTP stack ID.
 *      clock_num   - (IN)  PTP clock number.
 *      clock_port  - (IN)  PTP clock port number.
 *      latency_in  - (OUT) PTP clock port inbound latency (ns).
 *      latency_out - (OUT) PTP clock port outbound latency (ns).
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Function is an implementation-specific PTP management message.
 */
int
bcm_common_ptp_clock_port_latency_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 *latency_in,
    uint32 *latency_out)
{
    
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_latency_set
 * Purpose:
 *      Set PTP clock port expected asymmetric latencies.
 * Parameters:
 *      unit        - (IN) Unit number.
 *      ptp_id      - (IN) PTP stack ID.
 *      clock_num   - (IN) PTP clock number.
 *      clock_port  - (IN) PTP clock port number.
 *      latency_in  - (IN) PTP clock port inbound latency (ns).
 *      latency_out - (IN) PTP clock port outbound latency (ns).
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Function is an implementation-specific PTP management message.
 */
int
bcm_common_ptp_clock_port_latency_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 latency_in,
    uint32 latency_out)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_PORT_LATENCY_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    uint64 latency;
    int i = 0;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0...2   : Custom management message key/identifier.
     *                    Octet 0= 'B'; Octet 1= 'C'; Octet 2= 'M'.
     *    Octet 3...5   : Reserved.
     *    Octet 6...13  : Inbound port latency.
     *                    Binary fixed-point value. Time (ns)= latency_in/(2^16).
     *    Octet 14...21 : Outbound port latency.
     *                    Binary fixed-point value. Time (ns)= latency_out/(2^16).
     */
    i = 0;
    payload[i++] = 'B';
    payload[i++] = 'C';
    payload[i++] = 'M';
    i += 3;

    COMPILER_64_SET(latency, 0, latency_in);
    COMPILER_64_SHL(latency, 16);
    _bcm_ptp_uint64_write(payload+i, latency);
    i += sizeof(uint64);
    COMPILER_64_SET(latency, 0, latency_out);
    COMPILER_64_SHL(latency, 16);
    _bcm_ptp_uint64_write(payload+i, latency);

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_PORT_LATENCY,
            payload, PTP_MGMTMSG_PAYLOAD_PORT_LATENCY_SIZE_OCTETS,
            resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_log_announce_interval_get
 * Purpose:
 *      Get log announce interval of a PTP clock port dataset.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      interval   - (OUT) PTP clock port log announce interval.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.2.
 */
int
bcm_common_ptp_clock_port_log_announce_interval_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int *interval)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_LOG_ANNOUNCE_INTERVAL,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : Log announce interval.
         *    Octet 1 : Reserved.
         */
        *interval = (int8)resp[0];
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_log_announce_interval_set
 * Purpose:
 *      Set log announce interval of a PTP clock port dataset.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      interval   - (IN) PTP clock port log announce interval.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.2.
 */
int
bcm_common_ptp_clock_port_log_announce_interval_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int interval)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_clock_info_t ci;
    bcm_ptp_clock_port_info_t pi;
    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    /* PTP attribute profile range checking. */
    if (BCM_FAILURE(rv =
            _bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, &ci))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    if ((interval < ci.log_announce_interval_minimum) ||
            (interval > ci.log_announce_interval_maximum)) {
        /*
         * Caller provided log announce interval value is not in (min,max)
         * range of PTP profile.
         */
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : Log announce interval.
     *    Octet 1 : Reserved.
     */
    payload[0] = (uint8)interval;

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_LOG_ANNOUNCE_INTERVAL,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    /* Update PTP clock port cache. */
    if (rv == BCM_E_NONE) {
        if (BCM_FAILURE(rv = _bcm_ptp_clock_port_cache_info_get(unit, ptp_id,
                clock_num, clock_port, &pi))) {
            return rv;
        }

        pi.log_announce_interval = interval;
        rv = _bcm_ptp_clock_port_cache_info_set(unit, ptp_id, clock_num,
                clock_port, pi);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_announce_receipt_timeout_get
 * Purpose:
 *      Get announce receipt timeout member of a PTP clock port dataset.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      timeout    - (OUT) PTP clock port announce receipt timeout.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.3.
 */
int
bcm_common_ptp_clock_port_announce_receipt_timeout_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 *timeout)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_ANNOUNCE_RECEIPT_TIMEOUT,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : Announce receipt timeout.
         *    Octet 1 : Reserved.
         */
        *timeout = resp[0];
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_announce_receipt_timeout_set
 * Purpose:
 *      Set announce receipt timeout member of a PTP clock port dataset.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      timeout    - (IN) PTP clock port announce receipt timeout.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.3.
 */
int
bcm_common_ptp_clock_port_announce_receipt_timeout_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 timeout)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_clock_info_t ci;
    bcm_ptp_clock_port_info_t pi;
    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    /* PTP attribute profile range checking. */
    if (BCM_FAILURE(rv =
            _bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, &ci))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    if ((timeout < ci.announce_receipt_timeout_minimum) ||
            (timeout > ci.announce_receipt_timeout_maximum)) {
        /*
         * Caller provided announce receipt timeout value is not in (min,max)
         * range of PTP profile.
         */
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : Announce receipt timeout.
     *    Octet 1 : Reserved.
     */
    payload[0] = (uint8)timeout;

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_ANNOUNCE_RECEIPT_TIMEOUT,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    /* Update PTP clock port cache. */
    if (rv == BCM_E_NONE) {
        if (BCM_FAILURE(rv = _bcm_ptp_clock_port_cache_info_get(unit, ptp_id,
                clock_num, clock_port, &pi))) {
            return rv;
        }

        pi.announce_receipt_timeout = (uint8)timeout;
        rv = _bcm_ptp_clock_port_cache_info_set(unit, ptp_id, clock_num,
                clock_port, pi);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_log_sync_interval_get
 * Purpose:
 *      Get log sync interval member of a PTP clock port dataset.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      interval   - (OUT) PTP clock port log sync interval.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.4.
 */
int
bcm_common_ptp_clock_port_log_sync_interval_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int *interval)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_LOG_SYNC_INTERVAL,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : Log sync interval.
         *    Octet 1 : Reserved.
         */
        *interval = (int8)resp[0];
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_log_sync_interval_set
 * Purpose:
 *      Set log sync interval member of a PTP clock port dataset.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      interval   - (IN) PTP clock port log sync interval.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.4.
 */
int
bcm_common_ptp_clock_port_log_sync_interval_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int interval)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_clock_info_t ci;
    bcm_ptp_clock_port_info_t pi;
    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    /* PTP attribute profile range checking. */
    if (BCM_FAILURE(rv =
            _bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, &ci))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    if ((interval < ci.log_sync_interval_minimum) ||
            (interval > ci.log_sync_interval_maximum)) {
        /*
         * Caller provided log sync interval value is not in (min,max)
         * range of PTP profile.
         */
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : Log sync interval.
     *    Octet 1 : Reserved.
     */
    payload[0] = (uint8)interval;

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_LOG_SYNC_INTERVAL,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    /* Update PTP clock port cache. */
    if (rv == BCM_E_NONE) {
        if (BCM_FAILURE(rv = _bcm_ptp_clock_port_cache_info_get(unit, ptp_id,
                clock_num, clock_port, &pi))) {
            return rv;
        }

        pi.log_sync_interval = interval;
        rv = _bcm_ptp_clock_port_cache_info_set(unit, ptp_id, clock_num,
                clock_port, pi);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_version_number_get
 * Purpose:
 *      Get PTP version number member of a PTP clock port dataset.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      version    - (OUT) PTP clock port PTP version number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.7.
 */
int
bcm_common_ptp_clock_port_version_number_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 *version)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_VERSION_NUMBER,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : PTP version number.
         *    Octet 1 : Reserved.
         */
        *version = (0x0f & resp[0]);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_version_number_set
 * Purpose:
 *      Set PTP version number member of a PTP clock port dataset.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      version    - (IN) PTP clock port PTP version number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.7.
 */
int
bcm_common_ptp_clock_port_version_number_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 version)
{
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_log_min_pdelay_req_interval_get
 * Purpose:
 *      Get log minimum peer delay (PDelay) request interval member of a PTP
 *      clock port dataset.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      interval   - (OUT) PTP clock port log minimum peer delay (PDelay)
 *                         request interval.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.6.
 */
int
bcm_common_ptp_clock_port_log_min_pdelay_req_interval_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int *interval)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_LOG_MIN_PDELAY_REQ_INTERVAL,
            0, 0, resp, &resp_len);

    if (rv == BCM_E_NONE) {
        /*
         * Parse response.
         *    Octet 0 : Log minimum peer delay request interval.
         *    Octet 1 : Reserved.
         */
        *interval = (int8)resp[0];
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_log_min_pdelay_req_interval_set
 * Purpose:
 *      Set log minimum peer delay (PDelay) request interval member of a PTP
 *      clock port dataset.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      interval   - (IN) PTP clock port log minimum peer delay (PDelay)
 *                        request interval.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapters 8.2.5 and 15.5.3.7.6.
 */
int
bcm_common_ptp_clock_port_log_min_pdelay_req_interval_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int interval)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_clock_info_t ci;
    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    /* PTP attribute profile range checking. */
    if (BCM_FAILURE(rv =
            _bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, &ci))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    if ((interval < ci.log_min_delay_req_interval_minimum) ||
            (interval > ci.log_min_delay_req_interval_maximum)) {
        /*
         * Caller provided log minimum peer delay (PDelay) request interval
         * value is not in (min,max) range of PTP profile.
         */
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : Log minimum peer delay request interval.
     *    Octet 1 : Reserved.
     */
    payload[0] = (uint8)interval;

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_LOG_MIN_PDELAY_REQ_INTERVAL,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_log_min_delay_req_interval_get
 * Purpose:
 *      Get log minimum delay request interval member of a PTP clock port
 *      dataset.
 * Parameters:
 *      unit       - (IN)  Unit number.
 *      ptp_id     - (IN)  PTP stack ID.
 *      clock_num  - (IN)  PTP clock number.
 *      clock_port - (IN)  PTP clock port number.
 *      interval   - (OUT) PTP clock port log minimum delay request interval.
 *                         request interval.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Function is an implementation-specific PTP management message.
 *      Ref. IEEE Std. 1588-2008, Chapter 8.2.5.
 */
int
bcm_common_ptp_clock_port_log_min_delay_req_interval_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int *interval)
{
    int rv = BCM_E_UNAVAIL;
    bcm_ptp_port_dataset_t dataset;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    rv = bcm_common_ptp_clock_port_dataset_get(unit, ptp_id,
            clock_num, clock_port, &dataset);

    if (rv == BCM_E_NONE) {
        *interval = dataset.log_min_delay_req_interval;
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_log_min_delay_req_interval_set
 * Purpose:
 *      Set log minimum delay request interval member of a PTP clock port
 *      dataset.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      interval   - (IN) PTP clock port log minimum delay request interval.
 *                        request interval.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Function is an implementation-specific PTP management message.
 *      Ref. IEEE Std. 1588-2008, Chapter 8.2.5.
 */
int
bcm_common_ptp_clock_port_log_min_delay_req_interval_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int interval)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_clock_info_t ci;
    bcm_ptp_clock_port_info_t pi;
    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    /* PTP attribute profile range checking. */
    if (BCM_FAILURE(rv =
            _bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, &ci))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    if ((interval < ci.log_min_delay_req_interval_minimum) ||
            (interval > ci.log_min_delay_req_interval_maximum)) {
        /*
         * Caller provided log minimum delay request interval value is
         * not in (min,max) range of PTP profile.
         */
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0 : Log minimum peer delay request interval.
     *    Octet 1 : Reserved.
     */
    payload[0] = (uint8)interval;

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_LOG_MIN_DELAY_REQ_INTERVAL,
            payload, PTP_MGMTMSG_PAYLOAD_MIN_SIZE_OCTETS,
            resp, &resp_len);

    /* Update PTP clock port cache. */
    if (rv == BCM_E_NONE) {
        if (BCM_FAILURE(rv = _bcm_ptp_clock_port_cache_info_get(unit, ptp_id,
                clock_num, clock_port, &pi))) {
            return rv;
        }

        pi.log_min_delay_req_interval = interval;
        rv = _bcm_ptp_clock_port_cache_info_set(unit, ptp_id, clock_num,
                clock_port, pi);
    }

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_enable
 * Purpose:
 *      Enable a PTP clock port of an ordinary clock (OC) or a boundary
 *      clock (BC).
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapter 15.5.3.2.3.
 */
int
bcm_common_ptp_clock_port_enable(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    /* Empty payload. */

    if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id,
            clock_num, &portid, PTP_MGMTMSG_CMD, PTP_MGMTMSG_ID_ENABLE_PORT,
            0, 0, resp, &resp_len))) {
        return rv;
    }

    /* Restore/re-subscribe static unicast slave table. */
    if (BCM_FAILURE(rv = _bcm_ptp_unicast_slave_stack_restore(unit, ptp_id,
            clock_num, clock_port))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_disable
 * Purpose:
 *      Disable a PTP clock port of an ordinary clock (OC) or a boundary
 *      clock (BC).
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Ref. IEEE Std. 1588-2008, Chapter 15.5.3.2.4.
 */
int
bcm_common_ptp_clock_port_disable(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port)
{
    int rv = BCM_E_UNAVAIL;

    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, clock_port, &portid))) {
        return rv;
    }

    /* Empty payload. */

    if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id,
            clock_num, &portid, PTP_MGMTMSG_CMD, PTP_MGMTMSG_ID_DISABLE_PORT,
            0, 0, resp, &resp_len))) {
        return rv;
    }

    /* Remove signaled slaves. */
    if (BCM_FAILURE(rv = bcm_common_ptp_signaled_unicast_slave_table_clear(unit, ptp_id,
            clock_num, clock_port, 0))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    /*
     * NOTE: Implementation.
     * Firmware clears unicast slaves, which is sufficient to prevent sending
     * PTP messages from a disabled port. Do not clear static unicast slaves,
     * so table can be restored/re-subscribed if port is re-enabled. See also
     * ENABLE_PORT management message API.
     */

    /* Remove static slaves. */
    /*
    if (BCM_FAILURE(rv = _bcm_ptp_unicast_slave_host_reset(unit, ptp_id,
            clock_num, clock_port))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }
     */

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_clock_port_configure
 * Purpose:
 *      Configure a PTP clock port.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clock_port - (IN) PTP clock port number.
 *      info       - (IN) PTP clock port configuration information.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
bcm_common_ptp_clock_port_configure(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_clock_port_info_t *info)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_CONFIGURE_CLOCK_PORT_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    int i = 0;

    bcm_ptp_clock_info_t ci;
    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            clock_port))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0...2     : Custom management message key/identifier.
     *                      Octet 0= 'B'; Octet 1= 'C'; Octet 2= 'M'.
     *    Octet 3...5     : Reserved.
     *    Octet 6         : PTP clock number.
     *    Octet 7...8     : PTP clock port number.
     *    Octet 9...14    : Port MAC address.
     *    Octet 15        : Network protocol.
     *    Octet 16...31   : Port address.
     *    Octet 32        : Port type.
     *    Octet 33        : Log minimum delay request interval.
     *    Octet 34        : Multicast L2 header size (octets).
     *    Octet 35...66   : Multicast L2 header.
     *    Octet 67        : Multicast (peer delay) L2 header size (octets).
     *    Octet 68...99   : Multicast (peer delay) L2 header.
     *    Octet 100       : Multicast Tx enable Boolean.
     *    Octet 101...102 : Port Rx packets VLAN.
     *    Octet 103...106 : Port Rx packets port mask (high 32 bits).
     *    Octet 107...110 : Port Rx packets port mask (low 32 bits).
     *    Octet 111       : Port Rx packets criteria mask.
     *    Octet 112       : Rx timestamp mechanism.
     */
    i = 0;
    payload[i++] = 'B';
    payload[i++] = 'C';
    payload[i++] = 'M';
    i += 3;

    payload[i++] = clock_num;

    _bcm_ptp_uint16_write(payload+i, clock_port);
    i += sizeof(uint16);

    sal_memcpy(payload+i, info->mac, PTP_MAC_ADDR_SIZE_BYTES);
    i += PTP_MAC_ADDR_SIZE_BYTES;

    payload[i++] = info->port_address.addr_type;

    switch (info->port_address.addr_type) {
    case (bcmPTPUDPIPv4):
        /* Ethernet/UDP/IPv4 address. */
        sal_memcpy(payload+i, info->port_address.address,
                   PTP_IPV4_ADDR_SIZE_BYTES);
        break;

    case (bcmPTPUDPIPv6):
        /* Ethernet/UDP/IPv6 address. */
        sal_memcpy(payload+i, info->port_address.address,
                   PTP_IPV6_ADDR_SIZE_BYTES);
        break;

    case (bcmPTPIEEE8023):
        /* Ethernet Layer 2. */
        sal_memcpy(payload+i, info->port_address.address,
                   PTP_MAC_ADDR_SIZE_BYTES);
        break;

    default:
        /* Invalid address type. */
        return BCM_E_UNAVAIL;
    }

    i += BCM_PTP_MAX_NETW_ADDR_SIZE;

    payload[i++] = info->port_type;
    payload[i++] = info->log_min_delay_req_interval;

    if (info->multicast_l2_size > PTP_MAX_L2_HEADER_LENGTH) {
        /* Multicast L2 header length exceeds device-compliant maximum size. */
        return BCM_E_PARAM;
    }
    payload[i++] = info->multicast_l2_size;
    sal_memcpy(payload+i, info->multicast_l2, info->multicast_l2_size);
    i += PTP_MSG_L2_HEADER_LENGTH;

    if (info->multicast_pdelay_l2_size > PTP_MAX_L2_HEADER_LENGTH) {
        /* Multicast peer-delay L2 header length exceeds device-compliant maximum size. */
        return BCM_E_PARAM;
    }
    payload[i++] = info->multicast_pdelay_l2_size;
    sal_memcpy(payload+i, info->multicast_pdelay_l2,
               info->multicast_pdelay_l2_size);
    i += PTP_MSG_L2_HEADER_LENGTH;

    payload[i++] = (1 & info->multicast_tx_enable);

    _bcm_ptp_uint16_write(payload+i, info->rx_packets_vlan);
    i += sizeof(uint16);

    _bcm_ptp_uint32_write(payload+i, info->rx_packets_port_mask_high32);
    i += sizeof(uint32);

    _bcm_ptp_uint32_write(payload+i, info->rx_packets_port_mask_low32);
    i += sizeof(uint32);

    payload[i++] = info->rx_packets_criteria_mask;
    payload[i] = info->rx_timestamp_mechanism;

    if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id,
            clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_CONFIGURE_CLOCK_PORT,
            payload, PTP_MGMTMSG_PAYLOAD_CONFIGURE_CLOCK_PORT_SIZE_OCTETS,
            resp, &resp_len))) {
        return rv;
    }

    if (BCM_FAILURE(rv =
            _bcm_ptp_clock_cache_info_get(unit, ptp_id, clock_num, &ci))) {
        soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
        return rv;
    }

    if (ci.type != bcmPTPClockTypeTransparent)
    {
        /* Set log announce interval. */
        if (BCM_FAILURE(rv =
                bcm_common_ptp_clock_port_log_announce_interval_set(unit, ptp_id,
                clock_num, clock_port, (int)(info->log_announce_interval)))) {
            return rv;
        }

        /* Set log sync interval. */
        if (BCM_FAILURE(rv =
                bcm_common_ptp_clock_port_log_sync_interval_set(unit, ptp_id,
                clock_num, clock_port, info->log_sync_interval))) {
            return rv;
        }

        /* Set announce receipt timeout. */
        if (BCM_FAILURE(rv =
                bcm_common_ptp_clock_port_announce_receipt_timeout_set(unit, ptp_id,
                clock_num, clock_port, info->announce_receipt_timeout))) {
            return rv;
        }
    }

    /* Set delay mechanism. */
    if (BCM_FAILURE(rv =
            bcm_common_ptp_clock_port_delay_mechanism_set(unit, ptp_id,
            clock_num, clock_port, info->delay_mechanism))) {
        return rv;
    }

    /* Assign PTP clock port information to cache. */
    _bcm_common_ptp_unit_array[unit].stack_array[ptp_id]
                            .clock_array[clock_num]
                            .port_info[clock_port-1] = *info;

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_clock_class_set
 * Purpose:
 *      Set the PTP clock class.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      clockClass - (IN) PTP clock class.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
_bcm_ptp_clock_class_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 clockClass)
{
    int rv = BCM_E_NONE;
    int i = 0;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_CLOCK_CLASS_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_clock_info_t ci;
    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            clock_num, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        /* On error, target message to (all clocks, all ports) portIdentity. */
        portid = portid_all;
    }

    /*
     * Make payload.
     *    Octet 0...5 : Custom management message key/identifier.
     *                  BCM<null><null><null>.
     *    Octet 6     : PTP clockClass.
     *    Octet 7     : Reserved.
     */
    sal_memcpy(payload, "BCM\0\0\0", 6);
    i = 6;
    payload[i] = clockClass;

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num,
            &portid, PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_CLOCK_CLASS,
            payload, PTP_MGMTMSG_PAYLOAD_CLOCK_CLASS_SIZE_OCTETS,
            resp, &resp_len);

    /* Update PTP clock cache. */
    if (rv == BCM_E_NONE) {
        if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_info_get(unit, ptp_id,
                clock_num, &ci))) {
            soc_cm_print("FAILED %s:%d\n", __FILE__, __LINE__);
            return rv;
        }

        ci.clock_class = clockClass;
        rv = _bcm_ptp_clock_cache_info_set(unit, ptp_id, clock_num, ci);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_system_log_configure
 * Purpose:
 *      Set the debug log level for the firmware.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      ptp_id     - (IN) PTP stack ID.
 *      clock_num  - (IN) PTP clock number.
 *      level_mask - (IN) bitmask of log levels
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_ptp_system_log_configure(int unit, bcm_ptp_stack_id_t ptp_id, int clock_num,
                              uint32 debug_mask, uint64 udp_log_mask,
                              bcm_mac_t src_mac, bcm_mac_t dest_mac,
                              uint16 tpid, uint16 vid, uint8 ttl,
                              bcm_ip_t src_addr, bcm_ip_t dest_addr,
                              uint16 udp_port)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_LOG_LEVEL_SIZE_OCTETS] = {0};
    uint8 *curs = &payload[0];
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0..5:   BCM<nul><nul><nul>
     *    Octet 6..9:  new debug mask level
     *    Octet 10..17: new log mask level
     *    Octet 18..23: src_mac for log packets
     *    Octet 24..29: dst_mac for log packets
     *    Octet 30..31: tpid for log packets
     *    Octet 32..33: vid for log packets
     *    Octet 34:     TTL for log packets
     *    Octet 35..38: src_ip for log packets
     *    Octet 39..42: dst_ip for log packets
     *    Octet 43..46: UDP port for log packets
     */
    sal_memcpy(curs, "BCM\0\0\0", 6); curs += 6;
    _bcm_ptp_uint32_write(curs, debug_mask);   curs += 4;
    _bcm_ptp_uint64_write(curs, udp_log_mask); curs += 8;
    sal_memcpy(curs, src_mac, 6);        curs += 6;
    sal_memcpy(curs, dest_mac, 6);       curs += 6;
    _bcm_ptp_uint16_write(curs, tpid); curs += 2;
    _bcm_ptp_uint16_write(curs, vid);  curs += 2;
    *curs = ttl;  curs++;
    _bcm_ptp_uint32_write(curs, src_addr);  curs += 4;
    _bcm_ptp_uint32_write(curs, dest_addr); curs += 4;
    _bcm_ptp_uint16_write(curs, udp_port);  curs += 2;

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num,
                                          &portid, PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_LOG_LEVEL,
                                          payload, curs - payload, resp, &resp_len);

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
            PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_LOG_LEVEL,
            payload, PTP_MGMTMSG_PAYLOAD_LOG_LEVEL_SIZE_OCTETS,
            resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      _bcm_ptp_tunnel_arp_set
 * Purpose:
 *      Set flag to turn on/off ARP packet tunneling.
 * Parameters:
 *      unit   - (IN) Unit number.
 *      ptp_id - (IN) PTP stack ID.
 *      flag   - (IN) Tunnel ARP packets Boolean.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 */
int
_bcm_ptp_tunnel_arp_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    uint8 flag)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_TUNNEL_ARP_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    int i = 0;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_IEEE1588_ALL_PORTS, &portid))) {
        /* On error, target message to (all clocks, all ports) portIdentity. */
        portid = portid_all;
    }

    /*
     * Parse response.
     *    Octet 0...5 : Custom management message key/identifier.
     *                  BCM<null><null><null>.
     *    Octet 6     : Tunnel ARP packets Boolean.
     *    Octet 7     : Reserved (pad).
     */
    sal_memcpy(payload, "BCM\0\0\0", 6);
    i = 6;
    payload[i] = (flag & 0x01);

    rv = _bcm_ptp_management_message_send(unit, ptp_id, PTP_CLOCK_NUMBER_DEFAULT,
            &portid, PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_TUNNEL_ARP,
            payload, PTP_MGMTMSG_PAYLOAD_TUNNEL_ARP_SIZE_OCTETS,
            resp, &resp_len);

    return rv;
}

#endif /* defined(INCLUDE_PTP)*/
