/*
 * $Id: tdpll_esmc.c, Exp $
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
 * File: tdpll_esmc.c
 *
 * Purpose: Telecom DPLL Ethernet Synchronization Messaging Channel (ESMC) support.
 *
 * Functions:
 *      bcm_tdpll_esmc_init
 *      bcm_tdpll_esmc_rx_state_machine
 *      bcm_tdpll_esmc_holdover_event_send
 *      bcm_tdpll_esmc_switch_event_send
 *      bcm_tdpll_esmc_ql_get
 *      bcm_tdpll_esmc_ql_set
 *      bcm_tdpll_esmc_holdover_ql_get
 *      bcm_tdpll_esmc_holdover_ql_set
 *      bcm_tdpll_esmc_mac_get
 *      bcm_tdpll_esmc_mac_set
 *      bcm_tdpll_esmc_rx_enable_get
 *      bcm_tdpll_esmc_rx_enable_set
 *      bcm_tdpll_esmc_tx_enable_get
 *      bcm_tdpll_esmc_tx_enable_set
 *      bcm_tdpll_esmc_rx_portbitmap_get
 *      bcm_tdpll_esmc_rx_portbitmap_set
 *      bcm_tdpll_esmc_tx_portbitmap_get
 *      bcm_tdpll_esmc_tx_portbitmap_set
 *
 *      bcm_tdpll_esmc_info_pdu_tx
 */

#if defined(INCLUDE_PTP)

#include <bcm/ptp.h>
#include <bcm_int/common/ptp.h>
#include <bcm_int/ptp_common.h>
#include <bcm/error.h>

/* Definitions. */
#define TDPLL_ESMC_NETWORK_OPTION_DEFAULT (bcm_esmc_network_option_g781_II)
#define TDPLL_ESMC_QL_DEFAULT             (bcm_esmc_g781_II_ql_smc)
#define TDPLL_ESMC_SSM_CODE_DEFAULT       (0x0c)

#define TDPLL_ESMC_RX_ENABLE_DEFAULT                (0)
#define TDPLL_ESMC_TX_ENABLE_DEFAULT                (0)

#define TDPLL_ESMC_INFO_PDU_DPC_TIME_USEC_DEFAULT   (1000000/TDPLL_DPLL_INSTANCE_NUM_MAX)
#define TDPLL_ESMC_INFO_PDU_DPC_TIME_USEC_IDLE      (10000000)

/* Macros. */

/* Types. */

/* Constants and variables. */
static bcm_tdpll_esmc_data_t objdata;

/* Static functions. */
static void bcm_tdpll_esmc_info_pdu_tx(
    void *owner, void *arg_unit, void *arg_stack_id,
    void *arg_dpll_index, void *unused);


/*
 * Function:
 *      bcm_tdpll_esmc_init()
 * Purpose:
 *      Initialize T-DPLL Ethernet Synchronization Messaging Channel (ESMC).
 * Parameters:
 *      unit      - (IN) Unit number.
 *      stack_id  - (IN) Stack identifier index.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_tdpll_esmc_init(
    int unit,
    int stack_id)
{
    int i;

    objdata.rx_enable = TDPLL_ESMC_RX_ENABLE_DEFAULT ? 1:0;
    for (i = 0; i < TDPLL_DPLL_INSTANCE_NUM_MAX; ++i) {
        objdata.tx_enable[i] = TDPLL_ESMC_TX_ENABLE_DEFAULT ? 1:0;
    }

    objdata.network_option = TDPLL_ESMC_NETWORK_OPTION_DEFAULT;
    for (i = 0; i < TDPLL_DPLL_INSTANCE_NUM_MAX; ++i) {
        objdata.pdu_data[i].ql = TDPLL_ESMC_QL_DEFAULT;
        objdata.holdover_ql[i] = TDPLL_ESMC_QL_DEFAULT;
    }

    for (i = 0; i < TDPLL_DPLL_INSTANCE_NUM_MAX; ++i) {
        objdata.pdu_data[i].ssm_code = TDPLL_ESMC_SSM_CODE_DEFAULT;
        objdata.holdover_ssm_code[i] = TDPLL_ESMC_SSM_CODE_DEFAULT;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_rx_state_machine()
 * Purpose:
 *      Execute state machine for Rx ESMC PDU.
 * Parameters:
 *      unit          - (IN) Unit number.
 *      stack_id      - (IN) Stack identifier index.
 *      ingress_port  - (IN) Ingress port.
 *      esmc_pdu_data - (IN) ESMC PDU.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 *      State machine updates the SSM/QL of a T-DPLL input clock, transmits an
 *      ESMC event PDU upon change to SSM/QL of selected reference clock, etc.
 */
int
bcm_common_tdpll_esmc_rx_state_machine(
    int unit,
    int stack_id,
    int ingress_port,
    bcm_esmc_pdu_data_t *esmc_pdu_data)
{
    int rv;

    int dpll_index;
    int input_clock_index;
    int reference_index;

    bcm_esmc_quality_level_t input_clock_ql;
    bcm_esmc_quality_level_t esmc_pdu_ql;

    bcm_esmc_pdu_data_t pdu_data;

    /* Update networking option. */
    bcm_esmc_g781_option_get(unit, stack_id, &objdata.network_option);

    /* Look up ESMC PDU source in input clock set. */
    if (BCM_SUCCESS(rv = bcm_tdpll_input_clock_port_lookup(unit, stack_id,
            ingress_port, &input_clock_index))) {
        /* Update input clock MAC address. ESMC PDU from a known SyncE input clock. */
        bcm_tdpll_input_clock_mac_set(unit, stack_id, input_clock_index,
            &esmc_pdu_data->source_mac);
    } else if (BCM_FAILURE(rv = bcm_tdpll_input_clock_mac_lookup(unit, stack_id,
            &esmc_pdu_data->source_mac, &input_clock_index))) {
        /* Do nothing. ESMC PDU not from a known source / input clock. */
        return BCM_E_NONE;
    }

    /* Input clock SSM/QL change. */
    if (BCM_FAILURE(rv = bcm_tdpll_input_clock_ql_get(unit, stack_id,
            input_clock_index, &input_clock_ql))) {
        PTP_ERROR_FUNC("bcm_tdpll_input_clock_ql_get()");
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_esmc_SSM_QL_map(unit, objdata.network_option,
            esmc_pdu_data->ssm_code, &esmc_pdu_ql))) {
        PTP_ERROR_FUNC("bcm_esmc_SSM_QL_map()");
        return rv;
    }

    if (input_clock_ql != esmc_pdu_ql) {
        if (BCM_FAILURE(rv = bcm_tdpll_input_clock_ql_set(unit, stack_id,
                input_clock_index, esmc_pdu_ql))) {
                PTP_ERROR_FUNC("bcm_tdpll_input_clock_ql_set()");
            return rv;
        }
    } else {
        /* Do nothing. ESMC PDU from known source / input clock but SSM/QL is unchanged. */
        return BCM_E_NONE;
    }

    /* Selected reference clock SSM/QL change. */
    for (dpll_index = 0; dpll_index < TDPLL_DPLL_INSTANCE_NUM_MAX; ++dpll_index) {
        if (0 == objdata.tx_enable[dpll_index]) {
            /* ESMC Tx not enabled for DPLL. */
            continue;
        }

        if (BCM_FAILURE(rv = bcm_tdpll_input_clock_dpll_reference_get(unit, stack_id,
                dpll_index, &reference_index))) {
            /*
             * Do nothing. T-DPLL reference clock lookup failure.
             * Holdover logic shall initiate update to local node ESMC attributes and issue
             * an ESMC event PDU upon entry to holdover.
             */
            continue;
        }

        if (reference_index != input_clock_index) {
            /* Do nothing. ESMC PDU is not from T-DPLL current / active reference clock. */
            continue;
        }

        if (objdata.pdu_data[dpll_index].ssm_code != esmc_pdu_data->ssm_code) {
            /* Update local node ESMC attributes. */
            objdata.pdu_data[dpll_index].ssm_code = esmc_pdu_data->ssm_code;
            if (BCM_FAILURE(rv = bcm_esmc_SSM_QL_map(unit, objdata.network_option,
                    objdata.pdu_data[dpll_index].ssm_code, &objdata.pdu_data[dpll_index].ql))) {
                PTP_ERROR_FUNC("bcm_esmc_SSM_QL_map()");
                continue;
            }

            /* Transmit ESMC event PDU. */
            pdu_data = objdata.pdu_data[dpll_index];
            pdu_data.pdu_type = bcm_esmc_pdu_type_event;
            if (BCM_FAILURE(rv = bcm_esmc_tx(unit, stack_id, objdata.tx_pbmp[dpll_index],
                    &pdu_data))) {
                PTP_ERROR_FUNC("bcm_esmc_tx()");
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_holdover_event_send()
 * Purpose:
 *      Update ESMC and issue event PDU as result of holdover entry.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_tdpll_esmc_holdover_event_send(
    int unit,
    int stack_id,
    int dpll_index)
{
    int rv;
    bcm_esmc_pdu_data_t pdu_data;

    /* Argument checking and error handling. */
    if (dpll_index < 0 || dpll_index > TDPLL_DPLL_INSTANCE_NUM_MAX) {
        return BCM_E_PARAM;
    }

    if (objdata.pdu_data[dpll_index].ql == objdata.holdover_ql[dpll_index]) {
        /* Do nothing. ESMC QL is unchanged. */
        return BCM_E_NONE;
    }

    objdata.pdu_data[dpll_index].ql = objdata.holdover_ql[dpll_index];
    objdata.pdu_data[dpll_index].ssm_code = objdata.holdover_ssm_code[dpll_index];

    if (objdata.tx_enable[dpll_index]) {
        /* Transmit ESMC event PDU. */
        pdu_data = objdata.pdu_data[dpll_index];
        pdu_data.pdu_type = bcm_esmc_pdu_type_event;
        if (BCM_FAILURE(rv = bcm_esmc_tx(unit, stack_id, objdata.tx_pbmp[dpll_index],
                &pdu_data))) {
            PTP_ERROR_FUNC("bcm_esmc_tx()");
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_switch_event_send()
 * Purpose:
 *      Update ESMC and issue event PDU as result of reference switch.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      ql         - (IN) QL.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_tdpll_esmc_switch_event_send(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_esmc_quality_level_t ql)
{
    int rv;
    bcm_esmc_pdu_data_t pdu_data;

    /* Argument checking and error handling. */
    if (dpll_index < 0 || dpll_index > TDPLL_DPLL_INSTANCE_NUM_MAX) {
        return BCM_E_PARAM;
    }

    if (ql == objdata.pdu_data[dpll_index].ql) {
        /* Do nothing. ESMC QL is unchanged. */
        return BCM_E_NONE;
    }

    /* Update networking option. */
    bcm_esmc_g781_option_get(unit, stack_id, &objdata.network_option);

    objdata.pdu_data[dpll_index].ql = ql;
    if (BCM_FAILURE(rv = bcm_esmc_QL_SSM_map(unit, objdata.network_option,
            objdata.pdu_data[dpll_index].ql, &objdata.pdu_data[dpll_index].ssm_code))) {
        PTP_ERROR_FUNC("bcm_esmc_QL_SSM_map");
        return rv;
    }

    if (objdata.tx_enable[dpll_index]) {
        /* Transmit ESMC event PDU. */
        pdu_data = objdata.pdu_data[dpll_index];
        pdu_data.pdu_type = bcm_esmc_pdu_type_event;
        if (BCM_FAILURE(rv = bcm_esmc_tx(unit, stack_id, objdata.tx_pbmp[dpll_index],
                &pdu_data))) {
            PTP_ERROR_FUNC("bcm_esmc_tx()");
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_ql_get()
 * Purpose:
 *      Get quality level (QL) for ESMC.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      ql         - (OUT) QL.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_tdpll_esmc_ql_get(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_esmc_quality_level_t *ql)
{
    /* Argument checking and error handling. */
    if (dpll_index < 0 || dpll_index > TDPLL_DPLL_INSTANCE_NUM_MAX) {
        return BCM_E_PARAM;
    }

    *ql = objdata.pdu_data[dpll_index].ql;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_ql_set()
 * Purpose:
 *      Set quality level (QL) for ESMC.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      ql         - (IN) QL.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 *      Assignment is transient. QL shall be overwritten by subsequent
 *      ESMC PDUs received from a T-DPLL selected reference clock.
 */
int
bcm_common_tdpll_esmc_ql_set(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_esmc_quality_level_t ql)
{
    int rv;

    /* Argument checking and error handling. */
    if (dpll_index < 0 || dpll_index > TDPLL_DPLL_INSTANCE_NUM_MAX) {
        return BCM_E_PARAM;
    }

    /* Update networking option. */
    bcm_esmc_g781_option_get(unit, stack_id, &objdata.network_option);

    objdata.pdu_data[dpll_index].ql = ql;
    if (BCM_FAILURE(rv = bcm_esmc_QL_SSM_map(unit, objdata.network_option,
            objdata.pdu_data[dpll_index].ql, &objdata.pdu_data[dpll_index].ssm_code))) {
        PTP_ERROR_FUNC("bcm_esmc_QL_SSM_map");
        return rv;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_holdover_ql_get()
 * Purpose:
 *      Get quality level (QL) for ESMC during holdover.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      ql         - (OUT) QL.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_tdpll_esmc_holdover_ql_get(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_esmc_quality_level_t *ql)
{
    /* Argument checking and error handling. */
    if (dpll_index < 0 || dpll_index > TDPLL_DPLL_INSTANCE_NUM_MAX) {
        return BCM_E_PARAM;
    }

    *ql = objdata.holdover_ql[dpll_index];
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_holdover_ql_set()
 * Purpose:
 *      Set quality level (QL) for ESMC during holdover.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      ql         - (IN) QL.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_tdpll_esmc_holdover_ql_set(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_esmc_quality_level_t ql)
{
    int rv;

    /* Argument checking and error handling. */
    if (dpll_index < 0 || dpll_index > TDPLL_DPLL_INSTANCE_NUM_MAX) {
        return BCM_E_PARAM;
    }

    /* Update networking option. */
    bcm_esmc_g781_option_get(unit, stack_id, &objdata.network_option);

    objdata.holdover_ql[dpll_index] = ql;
    if (BCM_FAILURE(rv = bcm_esmc_QL_SSM_map(unit, objdata.network_option,
            objdata.holdover_ql[dpll_index], &objdata.holdover_ssm_code[dpll_index]))) {
        PTP_ERROR_FUNC("bcm_esmc_QL_SSM_map");
        return rv;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_mac_get()
 * Purpose:
 *      Get MAC address for ESMC.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      mac        - (OUT) Local port MAC address.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 *      MAC address is used as source MAC in Tx'd ESMC info and event PDUs.
 *      MAC address is used in discard-from-self logic for reflected PDUs.
 */
int
bcm_common_tdpll_esmc_mac_get(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_mac_t *mac)
{
    int rv;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_INDEXED_PROPRIETARY_MSG_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_PAYLOAD_ESMC_MAC_SIZE_OCTETS] = {0};
    int resp_len = PTP_MGMTMSG_PAYLOAD_ESMC_MAC_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    /* Argument checking and error handling. */
    if (dpll_index < 0 || dpll_index > TDPLL_DPLL_INSTANCE_NUM_MAX) {
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, stack_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, stack_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_IEEE1588_ALL_PORTS, &portid))) {
        PTP_ERROR_FUNC("bcm_common_ptp_clock_port_identity_get()");
        return rv;
    }

    /* Make indexed payload to get MAC address for specified DPLL instance. */
    sal_memcpy(payload, "BCM\0\0\0", 6);
    payload[6] = (uint8)dpll_index;
    if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, stack_id, PTP_CLOCK_NUMBER_DEFAULT,
            &portid, PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_ESMC_MAC,
            payload, PTP_MGMTMSG_PAYLOAD_INDEXED_PROPRIETARY_MSG_SIZE_OCTETS,
            resp, &resp_len))) {
        PTP_ERROR_FUNC("_bcm_ptp_management_message_send()");
        return rv;
    }

    /*
     * Parse response.
     *    Octet 0...5  : Custom management message key/identifier.
     *                   BCM<null><null><null>.
     *    Octet 6      : DPLL instance number.
     *    Octet 7      : Reserved.
     *    Octet 8...13 : ESMC MAC address.
     */
    sal_memcpy(mac, resp + 8, sizeof(bcm_mac_t));

    /* Set host-maintained ESMC MAC address. */
    sal_memcpy(objdata.pdu_data[dpll_index].source_mac, resp + 8, sizeof(bcm_mac_t));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_mac_set()
 * Purpose:
 *      Set MAC address for ESMC.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      mac        - (IN) Local port MAC address.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 *      MAC address is used as source MAC in Tx'd ESMC info and event PDUs.
 *      MAC address is used in discard-from-self logic for reflected PDUs.
 */
int
bcm_common_tdpll_esmc_mac_set(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_mac_t *mac)
{
    int rv;
    int i;
    
    uint8 payload[PTP_MGMTMSG_PAYLOAD_ESMC_MAC_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    /* Argument checking and error handling. */
    if (dpll_index < 0 || dpll_index > TDPLL_DPLL_INSTANCE_NUM_MAX) {
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, stack_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, stack_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_IEEE1588_ALL_PORTS, &portid))) {
        PTP_ERROR_FUNC("bcm_common_ptp_clock_port_identity_get()");
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0...5  : Custom management message key/identifier.
     *                   BCM<null><null><null>.
     *    Octet 6      : DPLL instance number.
     *    Octet 7      : Reserved.
     *    Octet 8...13 : ESMC MAC address.
     */
    sal_memcpy(payload, "BCM\0\0\0", 6);
    i = 6;
    payload[i++] = (uint8)dpll_index;
    payload[i++] = 0;
    sal_memcpy(payload + i, mac, sizeof(bcm_mac_t));

    if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, stack_id, PTP_CLOCK_NUMBER_DEFAULT,
            &portid, PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_ESMC_MAC,
            payload, PTP_MGMTMSG_PAYLOAD_ESMC_MAC_SIZE_OCTETS,
            resp, &resp_len))) {
        PTP_ERROR_FUNC("_bcm_ptp_management_message_send()");
        return rv;
    }

    /* Set host-maintained ESMC MAC address. */
    sal_memcpy(objdata.pdu_data[dpll_index].source_mac, mac, sizeof(bcm_mac_t));

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_rx_enable_get()
 * Purpose:
 *      Get ESMC PDU receive (Rx) enable state.
 * Parameters:
 *      unit     - (IN) Unit number.
 *      stack_id - (IN) Stack identifier index.
 *      enable   - (OUT) ESMC PDU receive enable Boolean.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_tdpll_esmc_rx_enable_get(
    int unit,
    int stack_id,
    int *enable)
{
    int rv;

    rv = bcm_esmc_tunnel_get(unit, stack_id, enable);
    objdata.rx_enable = *enable ? 1:0;

    return rv;
}

/*
 * Function:
 *      bcm_tdpll_esmc_rx_enable_set()
 * Purpose:
 *      Set ESMC PDU receive (Rx) enable state.
 * Parameters:
 *      unit     - (IN) Unit number.
 *      stack_id - (IN) Stack identifier index.
 *      enable   - (IN) ESMC PDU receive enable Boolean.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_tdpll_esmc_rx_enable_set(
    int unit,
    int stack_id,
    int enable)
{
    int rv;

    rv = bcm_esmc_tunnel_set(unit, stack_id, enable);
    objdata.rx_enable = enable ? 1:0;

    return rv;
}

/*
 * Function:
 *      bcm_tdpll_esmc_tx_enable_get()
 * Purpose:
 *      Get ESMC PDU transmit (Tx) enable state.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      enable     - (OUT) ESMC PDU transmit enable Boolean.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_tdpll_esmc_tx_enable_get(
    int unit,
    int stack_id,
    int dpll_index,
    int *enable)
{
    /* Argument checking and error handling. */
    if (dpll_index < 0 || dpll_index > TDPLL_DPLL_INSTANCE_NUM_MAX) {
        return BCM_E_PARAM;
    }

    *enable = objdata.tx_enable[dpll_index] ? 1:0;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_tx_enable_set()
 * Purpose:
 *      Set ESMC PDU transmit (Tx) enable state.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      enable     - (IN) ESMC PDU transmit enable Boolean.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_tdpll_esmc_tx_enable_set(
    int unit,
    int stack_id,
    int dpll_index,
    int enable)
{
    int i;
    int tx_esmc;

    /* Argument checking and error handling. */
    if (dpll_index < 0 || dpll_index > TDPLL_DPLL_INSTANCE_NUM_MAX) {
        return BCM_E_PARAM;
    }

    objdata.tx_enable[dpll_index] = enable ? 1:0;

    /*
     * Conditionally start DPC for periodic ESMC information PDUs.
     * NB: Cancel prior ESMC information DPC process (if any).
     */
    tx_esmc = 0;
    for (i = 0; i < TDPLL_DPLL_INSTANCE_NUM_MAX; ++i) {
        if (objdata.tx_enable[i]) {
            tx_esmc = 1;
            break;
        }
    }

    sal_dpc_cancel(INT_TO_PTR(&bcm_tdpll_esmc_info_pdu_tx));
    if (tx_esmc) {
        sal_dpc_time(TDPLL_ESMC_INFO_PDU_DPC_TIME_USEC_DEFAULT,
            &bcm_tdpll_esmc_info_pdu_tx, INT_TO_PTR(&bcm_tdpll_esmc_info_pdu_tx),
            INT_TO_PTR(unit), INT_TO_PTR(stack_id), 0, 0);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_rx_portbitmap_get()
 * Purpose:
 *      Get port bitmap for ESMC Rx.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      pbmp       - (OUT) Rx port bitmap.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_tdpll_esmc_rx_portbitmap_get(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_pbmp_t *pbmp)
{
    *pbmp = objdata.rx_pbmp[dpll_index];
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_rx_portbitmap_set()
 * Purpose:
 *      Set port bitmap for ESMC Rx.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      pbmp       - (IN) Rx port bitmap.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_tdpll_esmc_rx_portbitmap_set(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_pbmp_t pbmp)
{
    objdata.rx_pbmp[dpll_index] = pbmp;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_tx_portbitmap_get()
 * Purpose:
 *      Get port bitmap for ESMC Tx.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      pbmp       - (OUT) Tx port bitmap.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_tdpll_esmc_tx_portbitmap_get(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_pbmp_t *pbmp)
{
    *pbmp = objdata.tx_pbmp[dpll_index];
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_tx_portbitmap_set()
 * Purpose:
 *      Set port bitmap for ESMC Tx.
 * Parameters:
 *      unit       - (IN) Unit number.
 *      stack_id   - (IN) Stack identifier index.
 *      dpll_index - (IN) DPLL instance number.
 *      pbmp       - (IN) Tx port bitmap.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_tdpll_esmc_tx_portbitmap_set(
    int unit,
    int stack_id,
    int dpll_index,
    bcm_pbmp_t pbmp)
{
    objdata.tx_pbmp[dpll_index] = pbmp;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tdpll_esmc_info_pdu_tx()
 * Purpose:
 *      ESMC information PDU (as DPC).
 * Parameters:
 *      owner          - (IN) DPC owner.
 *      arg_unit       - (IN) Unit number (as void*).
 *      arg_stack_id   - (IN) Stack identifier index (as void*).
 *      arg_dpll_index - (IN) DPLL instance number (as void*).
 *      unused         - (IN) Unused.
 * Returns:
 *      None.
 * Notes:
 */
static void
bcm_tdpll_esmc_info_pdu_tx(
    void *owner,
    void *arg_unit,
    void *arg_stack_id,
    void *arg_dpll_index,
    void *unused)
{
    int rv;

    int unit = (size_t)arg_unit;
    int stack_id = (size_t)arg_stack_id;
    int dpll_index = (size_t)arg_dpll_index;

    sal_usecs_t dpc_recall_usec = TDPLL_ESMC_INFO_PDU_DPC_TIME_USEC_DEFAULT;
    bcm_esmc_pdu_data_t pdu_data;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, stack_id,
            PTP_CLOCK_NUMBER_DEFAULT, PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        dpc_recall_usec = TDPLL_ESMC_INFO_PDU_DPC_TIME_USEC_IDLE;
    }

    if (objdata.tx_enable[dpll_index]) {
        /* Transmit ESMC information PDU. */
        pdu_data = objdata.pdu_data[dpll_index];
        pdu_data.pdu_type = bcm_esmc_pdu_type_info;
        if (BCM_FAILURE(rv = bcm_esmc_tx(unit, stack_id, objdata.tx_pbmp[dpll_index],
                &pdu_data))) {
            PTP_ERROR_FUNC("bcm_esmc_tx()");
        }
    }

    ++dpll_index;
    dpll_index %= TDPLL_DPLL_INSTANCE_NUM_MAX;
    sal_dpc_time(dpc_recall_usec, &bcm_tdpll_esmc_info_pdu_tx,
                 INT_TO_PTR(&bcm_tdpll_esmc_info_pdu_tx),
                 arg_unit, arg_stack_id, INT_TO_PTR(dpll_index), 0);

    return;
}

#endif /* defined(INCLUDE_PTP) */
