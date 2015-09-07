/*
 * $Id: time-mbox.c 1.6 Broadcom SDK $
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
 * Time - Broadcom Time BroadSync API - shared functionality
 */

#include <shared/util.h>

#include <bcm/time.h>
#include <bcm/error.h>

#include <bcm_int/common/time.h>
#include <bcm_int/common/time-mbox.h>
#include <bcm_int/common/mbox.h>

#include <bcm_int/esw/mbcm.h>


#define FREQ_OFFSET_CMD_LEN  (1 + 4)
#define FREQ_OFFSET_RESP_LEN (1 + 1)
int
_bcm_time_bs_frequency_offset_set(int unit, bcm_time_spec_t new_offset)
{
    uint8  command[FREQ_OFFSET_CMD_LEN]  = {_BCM_TIME_FREQ_OFFSET_SET};
    int command_len = FREQ_OFFSET_CMD_LEN;

    uint8  response[FREQ_OFFSET_RESP_LEN] = {0};
    int response_len = FREQ_OFFSET_RESP_LEN;

    int offset = (new_offset.nanoseconds * 1000);

    /* The maximum drift allowed is 1ms per second */
    if ( (!COMPILER_64_IS_ZERO(new_offset.seconds)) || (new_offset.nanoseconds > 1000000) ) {
        return BCM_E_PARAM;
    }
                                           
    /* units in message to ToP are ppt, nanoseconds per second drift is ppb so convert */
    if (new_offset.isnegative) {
        offset = -offset;
    }

    _shr_uint32_write(&command[1], (uint32) offset);

    _bcm_mbox_txrx(unit, 0, _BCM_MBOX_MESSAGE, command, command_len, response, &response_len);

    if (response_len != 2) { return BCM_E_INTERNAL; }
    if (response[0] != command[0]) { return BCM_E_INTERNAL; }
    if (response[1] != 0x0) { return BCM_E_FAIL; }

    return BCM_E_NONE;
}


#define PHASE_OFFSET_CMD_LEN  (1 + 1 + 8 + 4)
#define PHASE_OFFSET_RESP_LEN (1 + 1)
int
_bcm_time_bs_phase_offset_set(int unit, bcm_time_spec_t new_offset)
{
    uint8  command[PHASE_OFFSET_CMD_LEN]  = {_BCM_TIME_PHASE_OFFSET_SET};
    int command_len = PHASE_OFFSET_CMD_LEN;

    uint8  response[PHASE_OFFSET_RESP_LEN] = {0};
    int response_len = PHASE_OFFSET_RESP_LEN;

    command[1] = new_offset.isnegative;
    _shr_uint64_write(&command[2], new_offset.seconds);
    _shr_uint32_write(&command[10], new_offset.nanoseconds);

    _bcm_mbox_txrx(unit, 0, _BCM_MBOX_MESSAGE, command, command_len, response, &response_len);

    if (response_len != 2) { return BCM_E_INTERNAL; }
    if (response[0] != command[0]) { return BCM_E_INTERNAL; }
    if (response[1] != 0x0) { return BCM_E_FAIL; }

    return BCM_E_NONE;
}


#define DEBUG_1PPS_CMD_LEN  (1 + 1)
#define DEBUG_1PPS_RESP_LEN (1 + 1)
int
_bcm_time_bs_debug_1pps_set(int unit, uint8 enableOutput)
{
    uint8  command[DEBUG_1PPS_CMD_LEN]  = {_BCM_TIME_DEBUG_1PPS_SET};
    int command_len = DEBUG_1PPS_CMD_LEN;

    uint8  response[DEBUG_1PPS_RESP_LEN] = {0};
    int response_len = DEBUG_1PPS_RESP_LEN;

    command[1] = enableOutput;

    _bcm_mbox_txrx(unit, 0, _BCM_MBOX_MESSAGE, command, command_len, response, &response_len);

    if (response_len != 2) { return BCM_E_INTERNAL; }
    if (response[0] != command[0]) { return BCM_E_INTERNAL; }
    if (response[1] != 0x0) { return BCM_E_FAIL; }

    return BCM_E_NONE;
}


#define STATUS_GET_CMD_LEN  (1)
#define STATUS_GET_RESP_LEN (2)
int
_bcm_time_bs_status_get(int unit, int *lock_status)
{
    uint8  command[STATUS_GET_CMD_LEN]  = {_BCM_TIME_STATUS_GET};
    int command_len = STATUS_GET_CMD_LEN;

    uint8  response[STATUS_GET_RESP_LEN] = {0};
    int response_len = STATUS_GET_RESP_LEN;

    _bcm_mbox_txrx(unit, 0, _BCM_MBOX_MESSAGE, command, command_len, response, &response_len);

    if (response_len != 2) { return BCM_E_INTERNAL; }
    if (response[0] != command[0]) { return BCM_E_INTERNAL; }

    /* Looks valid, return the second byte */
    *lock_status = (int)response[1];

    return BCM_E_NONE;
}


int
_bcm_time_bs_debug(uint32 flags)
{
    return _bcm_mbox_debug_flag_set(flags);
}


#define LOG_SET_CMD_LEN  (40)
#define LOG_SET_RESP_LEN (40)
/*
 * Function:
 *      _bcm_time_bs_log_configure
 * Purpose:
 *      Set the debug level and UDP log info for the firmware.
 * Parameters:
 *      unit         - (IN) Unit number.
 *      debug_mask   - (IN) bitmask of debug levels (textual debug)
 *      udp_log_mask - (IN) bitmask of UDP log levels
 *      src_mac      - (IN) SRC MAC for UDP log packets
 *      dest_mac     - (IN) DEST MAC for UDP log packets
 *      tpid         - (IN) TPID for UDP log packets
 *      vid          - (IN) VID for UDP log packets
 *      ttl          - (IN) TTL for UDP log packets
 *      src_addr     - (IN) IPv4 SRC Address for UDP log packets
 *      dest_addr    - (IN) IPv4 DEST Address for UDP log packets
 *      udp_port     - (IN) UDP port for UDP log packets
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_time_bs_log_configure(int unit,
                           uint32 debug_mask, uint64 udp_log_mask,
                           bcm_mac_t src_mac, bcm_mac_t dest_mac,
                           uint16 tpid, uint16 vid, uint8 ttl,
                           bcm_ip_t src_addr, bcm_ip_t dest_addr,
                           uint16 udp_port)
{
    int rv = BCM_E_UNAVAIL;

    uint8 payload[LOG_SET_CMD_LEN] = {0};
    uint8 *curs = &payload[0];
    uint8 resp[LOG_SET_RESP_LEN];
    int resp_len = LOG_SET_RESP_LEN;


    /*
     * Make payload.
     *    Octet 0:      command
     *    Octet 1..4:   new debug mask level
     *    Octet 5..12:  new log mask level
     *    Octet 13..18: src_mac for log packets
     *    Octet 19..24: dst_mac for log packets
     *    Octet 25..26: tpid for log packets
     *    Octet 27..28: vid for log packets
     *    Octet 29:     TTL for log packets
     *    Octet 30..33: src_ip for log packets
     *    Octet 34..37: dst_ip for log packets
     *    Octet 38..39: UDP port for log packets
     */
    *curs = _BCM_TIME_LOG_SET; curs++;
    soc_htonl_store(curs, debug_mask);   curs += 4;
    soc_htonl_store(curs, COMPILER_64_HI(udp_log_mask)); curs += 4;
    soc_htonl_store(curs, COMPILER_64_LO(udp_log_mask)); curs += 4;
    sal_memcpy(curs, src_mac, 6);        curs += 6;
    sal_memcpy(curs, dest_mac, 6);       curs += 6;
    soc_htons_store(curs, tpid); curs += 2;
    soc_htons_store(curs, vid);  curs += 2;
    *curs = ttl;  curs++;
    soc_htonl_store(curs, src_addr);  curs += 4;
    soc_htonl_store(curs, dest_addr); curs += 4;
    soc_htons_store(curs, udp_port);  curs += 2;

    rv = _bcm_mbox_txrx(unit, 0, _BCM_MBOX_MESSAGE, payload, LOG_SET_CMD_LEN, resp, &resp_len);
    
    if (resp_len != 2) { return BCM_E_INTERNAL; }
    if (resp[0] != payload[0]) { return BCM_E_INTERNAL; }
    if (resp[1] != 0x0) { return BCM_E_FAIL; }

    return rv;
}
