/*
 * $Id: timesource.c 1.4 Broadcom SDK $
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
 * File:    timesource.c
 *
 * Purpose: 
 *
 * Functions:
 *      bcm_common_ptp_tod_input_sources_set
 *      bcm_common_ptp_tod_input_sources_get
 *      bcm_common_ptp_tod_output_set
 *      bcm_common_ptp_tod_output_remove
 *      bcm_common_ptp_tod_output_get
 *      bcm_common_ptp_timesource_input_status_get
 *      bcm_common_ptp_input_channel_precedence_mode_set
 *      bcm_common_ptp_input_channel_switching_mode_set
 *      bcm_common_ptp_input_channels_get
 *      bcm_common_ptp_input_channels_set
 *      bcm_common_ptp_synce_output_set
 *      bcm_common_ptp_synce_output_get
 *      bcm_common_ptp_signal_output_get
 *      bcm_common_ptp_signal_output_remove
 *      bcm_common_ptp_signal_output_set
 *      bcm_common_ptp_sync_phy
 */

#if defined(INCLUDE_PTP)

#include <soc/defs.h>
#include <soc/drv.h>

#include <sal/core/dpc.h>

#include <bcm/ptp.h>
#include <bcm_int/common/ptp.h>
#include <bcm_int/ptp_common.h>
#include <bcm/error.h>

/* Constants and variables. */
#define PTP_TIMESOURCE_USE_MANAGEMENT_MSG                               (1)
#define PTP_TIMESOURCE_TODIN_MGMTMSG_FORMAT_LENGTH                     (32)
#define PTP_TIMESOURCE_TODIN_MGMTMSG_MASK_LENGTH (BCM_PTP_MAX_TOD_FORMAT_STRING)
#define PTP_TIMESOURCE_SIGNAL_OUTPUT_DATA_SIZE_OCTETS                  (17)


static const bcm_ptp_port_identity_t portid_all =
    {{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}, PTP_IEEE1588_ALL_PORTS};

static int _bcm_ptp_tod_out_enabled;

/*
 * Function:
 *      _bcm_common_ptp_configure_serial_tod
 * Purpose:
 *      Configure TOD for serial output.
 * Parameters:
 *      unit           - (IN) Unit number.
 *      ptp_id         - (IN) PTP stack ID.
 *      clock_num      - (IN) PTP clock number.
 *      uart_num       - (IN) UART number.
 *      offset         - (IN) ToD offset (nsec).
 *      delay          - (IN) ToD delay (nsec).
 *      fw_format_type - (IN) ToD format type selector (per firmware definition)
 *      format_str_len - (IN) ToD format string length.
 *      format_str     - (IN) ToD format string.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
_bcm_common_ptp_configure_serial_tod(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int uart_num,
    int32 offset_sec,
    int32 offset_ns,
    uint32 delay,
    int fw_format_type,
    int format_str_len,
    const char *format_str,
    uint8 l2_header_length,
    const uint8 l2_header[BCM_PTP_MAX_L2_HEADER_LENGTH])
{
    int rv = BCM_E_UNAVAIL;

    bcm_ptp_port_identity_t portid;
    uint8 payload[PTP_MGMTMSG_PAYLOAD_CONFIGURE_TOD_OUT_SIZE_OCTETS] = {0};
    uint8 *curs = &payload[0];
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;
    int copy_len;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    /* soc_cm_print("configure_serial_tod() uart_num:%d\n", uart_num); */

    /* Make payload. */
    sal_memcpy(curs, "BCM\0\0\0", 6);                 curs += 6;
    *curs++ = uart_num;
    _bcm_ptp_uint32_write(curs, (uint32)offset_sec);  curs += sizeof(uint32);
    _bcm_ptp_uint32_write(curs, (uint32)offset_ns);   curs += sizeof(uint32);
    _bcm_ptp_uint32_write(curs, delay);               curs += sizeof(uint32);
    *curs++ = fw_format_type;

    copy_len = format_str_len;
    if (copy_len > BCM_PTP_MAX_TOD_FORMAT_STRING) {
        copy_len = BCM_PTP_MAX_TOD_FORMAT_STRING;
    } else if (copy_len < 0) {
        copy_len = 0;
    }
    *curs++ = copy_len;

    if (format_str) {
        sal_memcpy(curs, format_str, copy_len);
    }
    curs += BCM_PTP_MAX_TOD_FORMAT_STRING;

    sal_memset(curs, 0, 18);
    if (l2_header_length <= 18) {
        if (l2_header) {
            sal_memcpy(curs, l2_header, l2_header_length);
        }
    } else {
        SOC_DEBUG_PRINT((DK_ERR, "Invalid L2 length (must be <= 18)"));
        return BCM_E_PARAM;
    }
    
    curs += 18;

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        /* On error, target message to (all clocks, all ports) portIdentity. */
        portid = portid_all;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
             PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_CONFIGURE_TOD_OUT,
             payload, curs - payload, resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_tod_input_sources_set
 * Purpose:
 *      Set PTP TOD input(s).
 * Parameters:
 *      unit            - (IN) Unit number.
 *      ptp_id          - (IN) PTP stack ID.
 *      clock_num       - (IN) PTP clock number.
 *      num_tod_sources - (IN) Number of ToD inputs.
 *      tod_sources     - (IN) PTP ToD input configuration.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_tod_input_sources_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int num_tod_sources,
    bcm_ptp_tod_input_t *tod_sources)
{
    int rv = BCM_E_UNAVAIL;

    bcm_ptp_port_identity_t portid;
    uint8 payload[PTP_MGMTMSG_PAYLOAD_CONFIGURE_TOD_IN_SIZE_OCTETS*2] = {0};
    uint8 *curs = &payload[0];
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    int fw_format_type = 0;
    uint8 *format_str = 0;
    uint8 *mask_str = 0;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    

    switch (num_tod_sources) {
    case 0:
        /* already have values set to have no TOD in */
        break;
    case 1:
        fw_format_type = tod_sources[0].format + 1;  /* ordering must be same on fw side */
        format_str = tod_sources[0].format_str;
        mask_str = tod_sources[0].mask_str;
        break;
    default:
        /* only currently support a single TOD-in */
        return BCM_E_PARAM;
    }

    /* Make payload. */
    sal_memcpy(curs, "BCM\0\0\0", 6);                            curs += 6;
    *curs++ = tod_sources[0].source;
    _bcm_ptp_uint32_write(curs, (uint32)tod_sources[0].tod_offset_sec);   curs += 4;
    _bcm_ptp_uint32_write(curs, (uint32)tod_sources[0].tod_offset_ns);    curs += 4;
    *curs++ = fw_format_type;

    *curs++ = (format_str) ? strlen((char*)format_str) : 0;

    if (format_str) {
        sal_memcpy(curs, format_str, PTP_TIMESOURCE_TODIN_MGMTMSG_FORMAT_LENGTH);
    }
    curs += PTP_TIMESOURCE_TODIN_MGMTMSG_FORMAT_LENGTH;

    *curs++ = (mask_str) ? strlen((char*)mask_str) : 0;

    if (mask_str) {
        sal_memcpy(curs, mask_str, PTP_TIMESOURCE_TODIN_MGMTMSG_MASK_LENGTH);
    }
    curs += PTP_TIMESOURCE_TODIN_MGMTMSG_MASK_LENGTH;

    if (tod_sources[0].peer_address.raw_l2_header_length == 18) {
        sal_memcpy(curs, tod_sources[0].peer_address.raw_l2_header + 6, 6); /* pick out src MAC */
        curs += 6;
        sal_memcpy(curs, tod_sources[0].peer_address.raw_l2_header + 14, 2); /* pick out VLAN */
        curs += 2;
        sal_memcpy(curs, tod_sources[0].peer_address.raw_l2_header + 16, 2); /* pick out EtherType */
        curs += 2;
    } else {
        curs += 6 + 2 + 2;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        /* On error, target message to (all clocks, all ports) portIdentity. */
        portid = portid_all;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num,
            &portid, PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_CONFIGURE_TOD_IN,
            payload, curs - payload, resp, &resp_len))) {
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_tod_input_set(unit, ptp_id, clock_num,
            num_tod_sources - 1, tod_sources))) {
        return rv;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_tod_input_sources_get
 * Purpose:
 *      Get PTP ToD input source(s).
 * Parameters:
 *      unit            - (IN)  Unit number.
 *      ptp_id          - (IN)  PTP stack ID.
 *      clock_num       - (IN)  PTP clock number.
 *      num_tod_sources - (IN/OUT)  Number of ToD inputs. (max on input)
 *      tod_sources     - (OUT) Array of ToD inputs.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_tod_input_sources_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *num_tod_sources,
    bcm_ptp_tod_input_t *tod_sources)
{
    int rv = BCM_E_UNAVAIL;
    int i;

    bcm_ptp_port_identity_t portid;
    uint8 resp[PTP_MGMTMSG_PAYLOAD_CONFIGURE_TOD_IN_SIZE_OCTETS] = {0};
    int resp_len = PTP_MGMTMSG_PAYLOAD_CONFIGURE_TOD_IN_SIZE_OCTETS;

    uint8 fmtlen = 0;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (*num_tod_sources >= PTP_MAX_TOD_INPUTS) {
        *num_tod_sources = PTP_MAX_TOD_INPUTS;
    }

    if (PTP_TIMESOURCE_USE_MANAGEMENT_MSG) {
        if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
                clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
            return rv;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num,
                &portid, PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_CONFIGURE_TOD_IN, 0, 0,
                resp, &resp_len))) {
            return rv;
        }

        /*
         * Parse response.
         *    Octet 0...5    : Custom management message key/identifier.
         *                     BCM<null><null><null>.
         *    Octet 6        : UART number.
         *    Octet 7..10    : ToD-In offset seconds
         *    Octet 11..14   : ToD-In offset nanoseconds
         *    Octet 15       : ToD-In format.
         *    Octet 16       : ToD-In format string length.
         *    Octet 17...48  : ToD-In format string.
         *    Octet 49       : ToD-In mask string length.
         *    Octet 50...177 : ToD-In mask string.
         */
        tod_sources[0].source = bcmPTPTODSourceSerial; 

        i = 7;
        tod_sources[0].tod_offset_sec = (int32) _bcm_ptp_uint32_read(resp+i);  i += 4;
        tod_sources[0].tod_offset_ns = (int32) _bcm_ptp_uint32_read(resp+i);    i += 4;

        /*
         * ToD format enumeration.
         * Account for mapping to firmware definition.
         */
        tod_sources[0].format = resp[i++] - 1;

        fmtlen = resp[i++];
        sal_memset(tod_sources[0].format_str, 0, BCM_PTP_MAX_TOD_FORMAT_STRING);
        sal_memcpy(tod_sources[0].format_str, resp + i, fmtlen);
        i += PTP_TIMESOURCE_TODIN_MGMTMSG_FORMAT_LENGTH;

        fmtlen = resp[i++];
        sal_memset(tod_sources[0].mask_str, 0, BCM_PTP_MAX_TOD_FORMAT_STRING);
        sal_memcpy(tod_sources[0].mask_str, resp + i, fmtlen);

    } else {
        for (i = 0; i < *num_tod_sources; ++i) {
            if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_tod_input_get(unit, ptp_id,
                    clock_num, i, &tod_sources[i]))) {
                return rv;
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_timesource_input_status_get
 * Purpose:
 *      Get time source status.
 * Parameters:
 *      unit        - (IN)  Unit number.
 *      ptp_id      - (IN)  PTP stack ID.
 *      clock_num   - (IN)  PTP clock number.
 *      status      - (OUT) Time source status.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_timesource_input_status_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_timesource_status_t *status)
{
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_common_ptp_input_channels_set
 * Purpose:
 *      Set PTP input channels.
 * Parameters:
 *      unit         - (IN) Unit number.
 *      ptp_id       - (IN) PTP stack ID.
 *      clock_num    - (IN) PTP clock number.
 *      num_channels - (IN) Number of channels (time sources).
 *      channels     - (IN) Channels (time sources).
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_input_channels_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int num_channels,
    bcm_ptp_channel_t *channels)
{
    int rv = BCM_E_UNAVAIL;
    int i;

    bcm_ptp_port_identity_t portid;
    uint8 payload[PTP_MGMTMSG_PAYLOAD_CHANNELS_SIZE_OCTETS] = {0};
    uint8 *curs = &payload[0];
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    if (num_channels >= PTP_MAX_TIME_SOURCES) {
        num_channels = PTP_MAX_TIME_SOURCES;
    } else if (num_channels < 1) {
        num_channels = 1;
    }

    /* Make payload. */
    _bcm_ptp_uint16_write(curs, num_channels);  curs += sizeof(uint16);

    for (i = 0; i < num_channels; ++i) {
        _bcm_ptp_uint16_write(curs, channels[i].type);         curs += sizeof(uint16);
        _bcm_ptp_uint32_write(curs, channels[i].source);       curs += sizeof(uint32);
        _bcm_ptp_uint32_write(curs, channels[i].frequency);    curs += sizeof(uint32);

        *curs++ = channels[i].tod_index;

        *curs++ = channels[i].freq_priority;
        *curs++ = channels[i].freq_enabled;
        *curs++ = channels[i].time_prio;
        *curs++ = channels[i].time_enabled;

        *curs++ = channels[i].freq_assumed_QL;
        *curs++ = channels[i].time_assumed_QL;
        *curs++ = channels[i].assumed_QL_enabled;

        _bcm_ptp_uint32_write(curs, channels[i].resolution);   curs += sizeof(uint32);

        if (SOC_IS_KATANA(unit) || SOC_IS_TRIUMPH3(unit) || SOC_IS_TRIDENT2(unit))
        {
           *curs++ = channels[i].synce_input_port;
        }
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
             PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_CHANNELS,
             payload, curs - payload, resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_input_channels_get
 * Purpose:
 *      Set PTP input channels.
 * Parameters:
 *      unit         - (IN)  Unit number.
 *      ptp_id       - (IN)  PTP stack ID.
 *      clock_num    - (IN)  PTP clock number.
 *      num_channels - (IN/  Max number of channels (time sources) /
 *                      OUT) Number of returned channels (time sources).
 *      channels     - (OUT) Channels (time sources).
 * Returns:
 *      BCM_E_XXX - Function status;
 * Notes:
 *      Function is not part of external API at this point.
 */
int
bcm_common_ptp_input_channels_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *num_channels,
    bcm_ptp_channel_t *channels)
{
    int rv = BCM_E_UNAVAIL;
    int i;

    bcm_ptp_port_identity_t portid;
    int max_num_channels = *num_channels;

    uint8 resp[PTP_MGMTMSG_PAYLOAD_CHANNELS_SIZE_OCTETS] = {0};
    uint8 *curs = &resp[0];
    int resp_len = PTP_MGMTMSG_PAYLOAD_CHANNELS_SIZE_OCTETS;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
             PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_CHANNELS, 0, 0, resp, &resp_len);

    /* Parse response. */
    *num_channels = _bcm_ptp_uint16_read(curs);  curs += sizeof(uint16);

    if (*num_channels > max_num_channels) {
        *num_channels = max_num_channels;
    }

    for (i = 0; i < *num_channels; ++i) {
        channels[i].type = _bcm_ptp_uint16_read(curs);         curs += sizeof(uint16);
        channels[i].source = _bcm_ptp_uint32_read(curs);       curs += sizeof(uint32);
        channels[i].frequency = _bcm_ptp_uint32_read(curs);    curs += sizeof(uint32);
        channels[i].tod_index = *curs++;
        channels[i].freq_priority = *curs++;
        channels[i].freq_enabled  = *curs++;
        channels[i].time_prio = *curs++;
        channels[i].time_enabled = *curs++;
        channels[i].freq_assumed_QL = *curs++;
        channels[i].time_assumed_QL = *curs++;
        channels[i].assumed_QL_enabled = *curs++;
        channels[i].resolution = _bcm_ptp_uint32_read(curs);   curs += sizeof(uint32);
        channels[i].synce_input_port = *curs++;
    }

    return rv;
}

/*
 * Function:
 *      _bcm_common_ptp_input_channels_status_get
 * Purpose:
 *      Get status of channels
 * Parameters:
 *      unit        - (IN)     Unit number.
 *      ptp_id      - (IN)     PTP stack ID.
 *      clock_num   - (IN)     PTP clock number.
 *      num_chan    - (IN/OUT) Number of channels to return / returned
 *      chan_status - (OUT)    Array of channel statuses
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
_bcm_common_ptp_input_channels_status_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *num_chan,
    _bcm_ptp_channel_status_t *chan_status)
{
    int rv = BCM_E_UNAVAIL;
    int i;
    int num_returned_channels = 0;

    uint8 resp[PTP_MGMTMSG_PAYLOAD_CHANNEL_STATUS_SIZE_OCTETS] = {0};
    uint8 *curs = &resp[6];
    int resp_len = PTP_MGMTMSG_PAYLOAD_CHANNEL_STATUS_SIZE_OCTETS;

    bcm_ptp_port_identity_t portid;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (*num_chan < 1) {
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    if (BCM_FAILURE (rv = _bcm_ptp_management_message_send(unit, ptp_id,
            clock_num, &portid, PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_CHANNEL_STATUS,
            0, 0, resp, &resp_len))) {
        return rv;
    }

    /* Parse response. */
    num_returned_channels = _bcm_ptp_uint16_read(curs);  curs += sizeof(uint16);

    if (num_returned_channels < *num_chan) {
        *num_chan = num_returned_channels;
    }

    for (i = 0; i < *num_chan; ++i) {
        chan_status[i].freq_status = _bcm_ptp_uint32_read(curs);
        curs += sizeof(uint32);
        chan_status[i].time_status = _bcm_ptp_uint32_read(curs);
        curs += sizeof(uint32);
        chan_status[i].freq_weight = _bcm_ptp_uint32_read(curs);
        curs += sizeof(uint32);
        chan_status[i].time_weight = _bcm_ptp_uint32_read(curs);
        curs += sizeof(uint32);
        chan_status[i].freq_QL  = _bcm_ptp_uint32_read(curs);
        curs += sizeof(uint32);
        chan_status[i].time_QL = _bcm_ptp_uint32_read(curs);
        curs += sizeof(uint32);
        chan_status[i].ql_read_externally = _bcm_ptp_uint32_read(curs);
        curs += sizeof(uint32);
        chan_status[i].fault_map = _bcm_ptp_uint32_read(curs);
        curs += sizeof(uint32);
    }

    return BCM_E_NONE;
}



/*
 * Function:
 *      bcm_common_ptp_input_channel_precedence_mode_set
 * Purpose:
 *      Set PTP input channels precedence mode.
 * Parameters:
 *      unit                - (IN) Unit number.
 *      ptp_id              - (IN) PTP stack ID.
 *      clock_num           - (IN) PTP clock number.
 *      channel_select_mode - (IN) Input channel precedence mode selector.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_input_channel_precedence_mode_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int channel_select_mode)
{
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_common_ptp_input_channel_switching_mode_set
 * Purpose:
 *      Set PTP input channels switching mode.
 * Parameters:
 *      unit                   - (IN) Unit number.
 *      ptp_id                 - (IN) PTP stack ID.
 *      clock_num              - (IN) PTP clock number.
 *      channel_switching_mode - (IN) Channel switching mode selector.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_input_channel_switching_mode_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int channel_switching_mode)
{
    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *      bcm_common_ptp_tod_output_set
 * Purpose:
 *      Set PTP ToD output.
 * Parameters:
 *      unit          - (IN)  Unit number.
 *      ptp_id        - (IN)  PTP stack ID.
 *      clock_num     - (IN)  PTP clock number.
 *      tod_output_id - (OUT) ToD output ID.
 *      output_info   - (IN)  ToD output configuration.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_tod_output_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *tod_output_id,
    bcm_ptp_tod_output_t *output_info)
{
    int rv = BCM_E_UNAVAIL;
    int fw_format_type = output_info->format + 1;  /* relies on firmware format type having the same ordering */

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (BCM_FAILURE(rv = _bcm_common_ptp_configure_serial_tod(unit, ptp_id, clock_num,
            output_info->source, output_info->tod_offset_src, output_info->tod_offset_ns, output_info->tod_delay_ns,
            fw_format_type, output_info->format_str_len, (char *)output_info->format_str,
            output_info->peer_address.raw_l2_header_length, output_info->peer_address.raw_l2_header ))) {
        return rv;
    }

    *tod_output_id = 1;
    _bcm_ptp_tod_out_enabled = 1;

    if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_tod_output_set(unit, ptp_id, clock_num,
            *tod_output_id - 1, output_info))) {
        return rv;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_tod_output_get
 * Purpose:
 *      Get PTP TOD output(s).
 * Parameters:
 *      unit             - (IN)  Unit number.
 *      ptp_id           - (IN)  PTP stack ID.
 *      clock_num        - (IN)  PTP clock number.
 *      tod_output_count - (IN/  Max number of ToD outputs /
 *                          OUT) Number ToD outputs returned.
 *      tod_outputs    - (OUT) Array of ToD outputs.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_tod_output_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *tod_output_count,
    bcm_ptp_tod_output_t *tod_outputs)
{
    int rv = BCM_E_UNAVAIL;
    int i;

    bcm_ptp_port_identity_t portid;
    uint8 resp[PTP_MGMTMSG_PAYLOAD_CONFIGURE_TOD_OUT_SIZE_OCTETS] = {0};
    int resp_len = PTP_MGMTMSG_PAYLOAD_CONFIGURE_TOD_OUT_SIZE_OCTETS;

    

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (*tod_output_count >= PTP_MAX_TOD_OUTPUTS) {
        *tod_output_count = PTP_MAX_TOD_OUTPUTS;
    }

    if (PTP_TIMESOURCE_USE_MANAGEMENT_MSG) {
        if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
                clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
            return rv;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num,
                &portid, PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_CONFIGURE_TOD_OUT, 0, 0,
                resp, &resp_len))) {
            return rv;
        }

        /*
         * Parse response.
         *    Octet 0...5    : Custom management message key/identifier.
         *                     BCM<null><null><null>.
         *    Octet 6        : UART number.
         *    Octet 7...14   : ToD-Out offset (nsec).
         *    Octet 15...18  : ToD-Out delay (nsec).
         *    Octet 19       : ToD-Out format.
         *    Octet 20       : ToD-Out format string length.
         *    Octet 21...148 : ToD-Out format string.
         */
        tod_outputs[0].source = bcmPTPTODSourceSerial; 
        tod_outputs[0].frequency = 1;                  

        i = 7;

        tod_outputs[0].tod_offset_src = _bcm_ptp_uint32_read(resp+i);
        i += sizeof(uint32);
        tod_outputs[0].tod_offset_ns = _bcm_ptp_uint32_read(resp+i+4);
        i += sizeof(uint32);

        tod_outputs[0].tod_delay_ns = _bcm_ptp_uint32_read(resp+i);
        i += sizeof(uint32);

        /*
         * ToD format enumeration.
         * Account for mapping to firmware definition.
         */
        tod_outputs[0].format = resp[i++] - 1;
        
        if (tod_outputs[0].format < bcmPTPTODFormatString) {
            tod_outputs[0].format = bcmPTPTODFormatString;
        }

        tod_outputs[0].format_str_len = resp[i++];
        sal_memset(tod_outputs[0].format_str, 0, BCM_PTP_MAX_TOD_FORMAT_STRING);
        sal_memcpy(tod_outputs[0].format_str, resp + i, tod_outputs[0].format_str_len);

    } else {
        for (i = 0; i < *tod_output_count; ++i) {
            if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_tod_output_get(unit, ptp_id,
                    clock_num, i, &tod_outputs[i]))) {
                return rv;
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_tod_output_remove
 * Purpose:
 *      Remove a ToD output.
 * Parameters:
 *      unit          - (IN) Unit number.
 *      ptp_id        - (IN) PTP stack ID.
 *      clock_num     - (IN) PTP clock number.
 *      tod_output_id - (IN) ToD output ID.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_tod_output_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int tod_output_id)
{
    int rv = BCM_E_UNAVAIL;
    int fw_format_type = 0;

    if (!_bcm_ptp_tod_out_enabled) {
        return BCM_E_PARAM;
    }

    if (BCM_FAILURE(rv = _bcm_common_ptp_configure_serial_tod(unit, ptp_id, clock_num,
            0, 0, 0, 0, fw_format_type, 0, 0, 0, 0))) {
        return rv;
    }

    _bcm_ptp_tod_out_enabled = 0;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_synce_output_set
 * Purpose:
 *      Set SyncE output control on/off
 * Parameters:
 *      unit             - (IN)  Unit number.
 *      ptp_id           - (IN)  PTP stack ID.
 *      clock_num        - (IN)  PTP clock number.
 *      synce_port       - (IN)  PTP output L1 clock port
 *      state            - (IN)  Non-zero to indicate that SyncE output should be controlled by TOP
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_synce_output_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int synce_port,
    int state)
{
    int rv = BCM_E_UNAVAIL;

    bcm_ptp_port_identity_t portid;
    uint8 payload[PTP_MGMTMSG_PAYLOAD_OUTPUT_SIGNALS_SIZE_OCTETS] = {0};
    uint8 *curs = &payload[0];
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    _bcm_ptp_uint32_write(curs, state);
    curs += sizeof(uint32);
    *curs++ = synce_port;
    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
             PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_OUTPUT_SYNCE,
             payload, curs - payload, resp, &resp_len);

    return rv;
}


/*
 * Function:
 *      bcm_common_ptp_synce_output_get
 * Purpose:
 *      Gets SyncE output control state
 * Parameters:
 *      unit             - (IN)  Unit number.
 *      ptp_id           - (IN)  PTP stack ID.
 *      clock_num        - (IN)  PTP clock number.
 *      state            - (OUT) Non-zero to indicate that SyncE output should be controlled by TOP
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_synce_output_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *state)
{
    int rv = BCM_E_UNAVAIL;

    bcm_ptp_port_identity_t portid;
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
             PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_OUTPUT_SYNCE,
             0, 0,
             resp, &resp_len);

    if (resp_len < PTP_MGMTMSG_PAYLOAD_OUTPUT_SYNCE_SIZE_OCTETS) {
        /* Internal error : message too short to contain correct data. */
        return BCM_E_INTERNAL;
    }

    *state = _bcm_ptp_uint32_read(resp);

    return rv;
}


/*
 * Function:
 *      bcm_common_ptp_signal_output_set
 * Purpose:
 *      Set PTP signal output.
 * Parameters:
 *      unit             - (IN)  Unit number.
 *      ptp_id           - (IN)  PTP stack ID.
 *      clock_num        - (IN)  PTP clock number.
 *      signal_output_id - (OUT) ID of signal.
 *      output_info      - (IN)  Signal information.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_signal_output_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *signal_output_id,
    bcm_ptp_signal_output_t *output)
{
    int rv = BCM_E_UNAVAIL;
    unsigned idx;

    bcm_ptp_signal_output_t signal;
    bcm_ptp_port_identity_t portid;

    uint8 payload[PTP_MGMTMSG_PAYLOAD_OUTPUT_SIGNALS_SIZE_OCTETS] = {0};
    uint8 *curs = &payload[0];
    uint8 resp[PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS];
    int resp_len = PTP_MGMTMSG_RESP_MAX_SIZE_OCTETS;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    idx = output->pin;
    if (idx >= PTP_MAX_OUTPUT_SIGNALS) {
        return BCM_E_PARAM;
    }

    /* use the pin # as the signal_output_id, since they're unique anyhow  */
    /* this makes the "sparse" problem immediately apparent if the pin# is */
    /* not zero, but the problem would occur anyhow if any but the first   */
    /* signal were removed                                                 */

    *signal_output_id = output->pin;

    /* Special case for SyncE: use the SyncE output set message */
    if (output->pin == PTP_OUTPUT_SIGNAL_SYNCE) {
        *signal_output_id = output->pin;
        return bcm_common_ptp_synce_output_set(unit, ptp_id, clock_num, output->synce_output_port, output->frequency);
    }

    if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_signal_set(unit, ptp_id, clock_num,
            idx, output))) {
        return rv;
    }

    if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
            clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
        return rv;
    }

    /* Make payload. */
    if (PTP_TIMESOURCE_USE_MANAGEMENT_MSG) {
        /* Get PTP signal output configurations from firmware. */
        if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id,
             clock_num, &portid, PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_OUTPUT_SIGNALS,
             0, 0, resp, &resp_len))) {
            return rv;
        }
        /* Insert updated configuration for chosen PTP signal output. */
        sal_memcpy(payload, resp, PTP_MGMTMSG_PAYLOAD_OUTPUT_SIGNALS_SIZE_OCTETS);
        /* Advance cursor. */
        curs += (output->pin*PTP_TIMESOURCE_SIGNAL_OUTPUT_DATA_SIZE_OCTETS);
        _bcm_ptp_uint32_write(curs, output->pin);
        curs += sizeof(uint32);
        _bcm_ptp_uint32_write(curs, output->frequency);
        curs += sizeof(uint32);
        *curs++ = output->phase_lock;
        _bcm_ptp_uint32_write(curs, output->pulse_width_ns);
        curs += sizeof(uint32);
        _bcm_ptp_uint32_write(curs, output->pulse_offset_ns);
    } else {
        /* Get PTP signal output configurations from SDK cache. */
        for (idx = 0; idx < PTP_GPIO_OUTPUT_SIGNALS; ++idx) {
            if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_signal_get(unit, ptp_id,
                    clock_num, idx, &signal))) {
                return rv;
            }
            _bcm_ptp_uint32_write(curs, signal.pin);               curs += sizeof(uint32);
            _bcm_ptp_uint32_write(curs, signal.frequency);         curs += sizeof(uint32);
            *curs++ = signal.phase_lock;
            _bcm_ptp_uint32_write(curs, signal.pulse_width_ns);    curs += sizeof(uint32);
            _bcm_ptp_uint32_write(curs, signal.pulse_offset_ns);   curs += sizeof(uint32);
        }
    }

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
             PTP_MGMTMSG_SET, PTP_MGMTMSG_ID_OUTPUT_SIGNALS,
             payload, PTP_MGMTMSG_PAYLOAD_OUTPUT_SIGNALS_SIZE_OCTETS,
             resp, &resp_len);

    return rv;
}

/*
 * Function:
 *      bcm_common_ptp_signal_output_get
 * Purpose:
 *      Get PTP signal output.
 * Parameters:
 *      unit                - (IN)  Unit number.
 *      ptp_id              - (IN)  PTP stack ID.
 *      clock_num           - (IN)  PTP clock number.
 *      signal_output_count - (IN/  Max number of signal outputs /
 *                             OUT) Number signal outputs returned.
 *      signal_output       - (OUT) Array of signal outputs.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 *      This function specification does not provide the ID of each signal,
 *      so the signal ID equals the position in the array, i.e. there is no
 *      mechanism to make it sparse. Invalid/inactive outputs are indicated
 *      by a zero (0) frequency.
 */
int
bcm_common_ptp_signal_output_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *signal_output_count,
    bcm_ptp_signal_output_t *signal_output)
{
    int rv = BCM_E_UNAVAIL;
    int i;

    bcm_ptp_port_identity_t portid;
    uint8 resp[PTP_MGMTMSG_PAYLOAD_OUTPUT_SIGNALS_SIZE_OCTETS] = {0};
    uint8 *cursor = &resp[0];
    int resp_len = PTP_MGMTMSG_PAYLOAD_OUTPUT_SIGNALS_SIZE_OCTETS;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
            PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    *signal_output_count = 0;

    if (PTP_TIMESOURCE_USE_MANAGEMENT_MSG) {
        if (BCM_FAILURE(rv = bcm_common_ptp_clock_port_identity_get(unit, ptp_id,
                clock_num, PTP_IEEE1588_ALL_PORTS, &portid))) {
            return rv;
        }

        if (BCM_FAILURE(rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num,
                &portid, PTP_MGMTMSG_GET, PTP_MGMTMSG_ID_OUTPUT_SIGNALS,
                0, 0, resp, &resp_len))) {
            return rv;
        }

        /*
         * Parse response.
         *    Per Output Signal : # = Output Signal Number, I = 17(#-1)
         *    Octet I...I+3     : Output Signal # pin.
         *    Octet I+4...I+7   : Output Signal # frequency.
         *    Octet I+8         : Output Signal # phase-lock Boolean.
         *    Octet I+9...I+12  : Output Signal # pulse width (nsec).
         *    Octet I+13...I+16 : Output Signal # pulse offset (nsec).
         */
        for (i = 0; i < PTP_GPIO_OUTPUT_SIGNALS; ++i) {
            if (cursor >= (resp_len + &resp[0]))
            {
                return BCM_E_NONE;
            }
            signal_output[i].pin = _bcm_ptp_uint32_read(cursor);
            cursor += sizeof(uint32);
            signal_output[i].frequency = _bcm_ptp_uint32_read(cursor);
            cursor += sizeof(uint32);

            signal_output[i].phase_lock = *cursor++;

            signal_output[i].pulse_width_ns = _bcm_ptp_uint32_read(cursor);
            cursor += sizeof(uint32);
            signal_output[i].pulse_offset_ns = _bcm_ptp_uint32_read(cursor);
            cursor += sizeof(uint32);
            if (signal_output[i].frequency)
            {
                *signal_output_count = (*signal_output_count) + 1;
            }
        }

        if (PTP_OUTPUT_SIGNAL_SYNCE > 0) {
            if (BCM_FAILURE(rv = bcm_common_ptp_synce_output_get(unit, ptp_id, clock_num,
                                    &signal_output[PTP_OUTPUT_SIGNAL_SYNCE].frequency))) {
                return rv;
            }
            if (signal_output[PTP_OUTPUT_SIGNAL_SYNCE].frequency) {
                *signal_output_count = (*signal_output_count) + 1;
            }
            signal_output[PTP_OUTPUT_SIGNAL_SYNCE].pin = PTP_OUTPUT_SIGNAL_SYNCE;
            signal_output[PTP_OUTPUT_SIGNAL_SYNCE].phase_lock = 0;
            signal_output[PTP_OUTPUT_SIGNAL_SYNCE].pulse_width_ns = 0;
            signal_output[PTP_OUTPUT_SIGNAL_SYNCE].pulse_offset_ns = 0;
        }
    } else {
        for (i = 0; i < *signal_output_count; ++i) {
            if (BCM_FAILURE(rv = _bcm_ptp_clock_cache_signal_get(unit, ptp_id,
                    clock_num, i, signal_output))) {
                return rv;
            }

            ++signal_output;
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_common_ptp_signal_output_remove
 * Purpose:
 *      Remove PTP signal output.
 * Parameters:
 *      unit             - (IN) Unit number.
 *      ptp_id           - (IN) PTP stack ID.
 *      clock_num        - (IN) PTP clock number.
 *      signal_output_id - (IN) Signal to remove/invalidate.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_signal_output_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int signal_output_id)
{
    if (SOC_HAS_PTP_INTERNAL_STACK_SUPPORT(unit)) {
        bcm_ptp_signal_output_t signal_output = {0};
        signal_output.pin = signal_output_id;

        return bcm_common_ptp_signal_output_set(unit, ptp_id, clock_num, &signal_output_id, &signal_output);
    }

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcm_common_ptp_sync_phy
 * Purpose:
 *      Instruct TOP to send a PHY Sync pulse, and mark the time that the PHYs are now synced to
 * Parameters:
 *      unit             - (IN) Unit number.
 *      ptp_id           - (IN) PTP stack ID.
 *      clock_num        - (IN) PTP clock number.
 *      bcm_ptp_sync_phy_input_t (IN) PTP PHY SYNC inputs.
 * Returns:
 *      BCM_E_XXX - Function status.
 * Notes:
 */
int
bcm_common_ptp_sync_phy(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_sync_phy_input_t sync_input)
{
    int rv = BCM_E_UNAVAIL;

    bcm_ptp_port_identity_t portid = portid_all;
    uint8 payload[PTP_MGMTMSG_PAYLOAD_SYNC_PHY_SIZE_OCTETS] = {0};
    uint8 resp[PTP_MGMTMSG_PAYLOAD_MAX_SIZE_OCTETS] = {0};
    int resp_len = PTP_MGMTMSG_PAYLOAD_MAX_SIZE_OCTETS;
    uint8 *curs = payload;

    if (BCM_FAILURE(rv = _bcm_ptp_function_precheck(unit, ptp_id, clock_num,
                                                        PTP_CLOCK_PORT_NUMBER_DEFAULT))) {
        PTP_ERROR_FUNC("_bcm_ptp_function_precheck()");
        return rv;
    }

    /*
     * Make payload.
     *    Octet 0...2   : Custom management message key/identifier.
     *                   Octet 0= 'B'; Octet 1= 'C'; Octet 2= 'M'.
     *    Octet 3...5   : Reserved.
     *    Octet 6       : Framesync Pin #
     *    Octet 7-14    : Reference time
     *    Octet 15      : Freq Pin #
     */

    sal_memcpy(curs, "BCM\0\0\0", 6);     curs += 6;
    *curs++ = sync_input.framesync_pin;
    _bcm_ptp_uint64_write(curs, sync_input.reference_time);   curs += 8;
    *curs++ = sync_input.freq_pin;

    portid = portid_all;

    rv = _bcm_ptp_management_message_send(unit, ptp_id, clock_num, &portid,
                                          PTP_MGMTMSG_CMD, PTP_MGMTMSG_ID_SYNC_PHY,
                                          payload, curs - payload, resp, &resp_len);

    return rv;

}

#endif /* defined(INCLUDE_PTP)*/
