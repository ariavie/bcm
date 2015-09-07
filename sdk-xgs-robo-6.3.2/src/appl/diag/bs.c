/*
 * $Id: bs.c 1.8 Broadcom SDK $
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
 */

#include <bcm/error.h>
#include <bcm/time.h>

#include <appl/diag/system.h>
#include <appl/diag/parse.h>

#ifdef BCM_CMICM_SUPPORT

#include <bcm_int/esw/port.h>
#include <bcm_int/common/mbox.h>
#include <bcm_int/common/time.h>
#include <bcm_int/common/time-mbox.h>


/* A sentinel value to indicate that no valid value was passed in */
/* static int64 ILLEGAL_OFFSET_LL = COMPILER_64_INIT(0x7fffffff, 0xffffffff); */
static uint32 ILLEGAL_OFFSET = 0x7fffffff;

#define BS_NANOSECONDS_PER_SECOND (1000000000)

#define CLEAN_UP_AND_RETURN(_result)            \
    parse_arg_eq_done(&parse_table);            \
    return (_result);

#define _isprintable(_c) (((_c) > 32) && ((_c) < 127))

char cmd_broadsync_usage[] =
#ifdef COMPILER_STRING_CONST_LIMIT
    "Usage: BroadSync <option> [args...]\n"
#else
    "\n"
    "  BroadSync CONFig [Master] [BitClock=<hz>] [HeartBeat=<hz>] [Freq=<ppb offset>] [PhaseSec=<seconds>] [PhaseNSec]=<nanoseconds>\n"
    "    Initialize or reconfigure.  Default is slave, if 'Master' is not specified\n"
    "  BroadSync STATus\n"
    "    Show DPLL lock state\n"
    "  BroadSync 1PPS [ON|OFF]\n"
    "    No option: print 1PPS state.  ON/OFF: enable/disable 1PPS output\n"
    "  BroadSync DEBUG [ON|OFF]\n"
    "    No option: print debug state.  ON/OFF: enable/disable debug output\n"
    "  BroadSync LOG [LogDebug] [LogPLL] [Vlan=<vlan>] [DestMAC=<mac>] [DestAddr=<addr>] [SrcMAC=<mac>] [SrcAddr=<addr>] [UDPPort=<udp port>]\n"
    "    Sets UDP logging state\n"
#endif
    ;

cmd_result_t cmd_broadsync(int unit, args_t *args)
{
    char *arg_string_p = NULL;
    int result;

    static bcm_time_if_t bs_id[BCM_MAX_NUM_UNITS] = {0};
    static int bs_initialized[BCM_MAX_NUM_UNITS] = {0};

    int arg_is_master = 0;
    
    arg_string_p = ARG_GET(args);

    if (arg_string_p == NULL) {
        return CMD_USAGE;
    }

    if (!sh_check_attached(ARG_CMD(args), unit)) {
        return CMD_FAIL;
    }

    if (parse_cmp("CONFig", arg_string_p, 0)) {
        parse_table_t parse_table;
        int n_args = 0;
        bcm_time_interface_t intf;
        int32 freq = ILLEGAL_OFFSET;
        int32 phase_sec = ILLEGAL_OFFSET;
        int32 phase_nsec = ILLEGAL_OFFSET;

        if (bs_initialized[unit]) {
            intf.id = bs_id[unit];
            intf.flags = BCM_TIME_OFFSET | BCM_TIME_DRIFT |
                BCM_TIME_REF_CLOCK | BCM_TIME_HEARTBEAT;
            bcm_time_interface_get(unit, &intf);

            intf.flags = BCM_TIME_WITH_ID | BCM_TIME_REPLACE |
                BCM_TIME_ENABLE | BCM_TIME_SYNC_STAMPER |
                BCM_TIME_REF_CLOCK | BCM_TIME_HEARTBEAT;
        } else {
            intf.flags = BCM_TIME_ENABLE | BCM_TIME_SYNC_STAMPER |
                BCM_TIME_REF_CLOCK | BCM_TIME_HEARTBEAT;

            intf.id = 0;                                    /* Time Interface Identifier */
            intf.drift.isnegative = 0;                      /* Drift amount per 1 Sec */
            COMPILER_64_ZERO(intf.drift.seconds);
            intf.drift.nanoseconds = 0;
            intf.offset.isnegative = 0;                     /* Offset */
            COMPILER_64_ZERO(intf.offset.seconds);
            intf.offset.nanoseconds = 0;
            intf.accuracy.isnegative = 0;                   /* Accuracy */
            COMPILER_64_ZERO(intf.accuracy.seconds);
            intf.accuracy.nanoseconds = 0;
            intf.clk_resolution = 1;                        /* Reference clock resolution in nsecs */

            intf.bitclock_hz = 10000000;  /* 10MHz default bitclock */
            intf.heartbeat_hz = 4000;     /* 4KHz default heartbeat */
        }

        intf.flags &= ~(BCM_TIME_DRIFT);
        intf.flags &= ~(BCM_TIME_OFFSET);

        parse_table_init(unit, &parse_table);

        parse_table_add(&parse_table, "Master", PQ_DFL | PQ_BOOL | PQ_NO_EQ_OPT, (void*)0, &arg_is_master, NULL);
        parse_table_add(&parse_table, "BitClock", PQ_DFL | PQ_INT, NULL, &intf.bitclock_hz, NULL);
        parse_table_add(&parse_table, "HeartBeat", PQ_DFL | PQ_INT, NULL, &intf.heartbeat_hz, NULL);
        parse_table_add(&parse_table, "Freq", PQ_DFL | PQ_INT, NULL, &freq, NULL);
        parse_table_add(&parse_table, "PhaseSec", PQ_DFL | PQ_INT, NULL, &phase_sec, NULL);
        parse_table_add(&parse_table, "PhaseNSec", PQ_DFL | PQ_INT, NULL, &phase_nsec, NULL);

        n_args = parse_arg_eq(args, &parse_table);

        if ((n_args < 0) || (ARG_CNT(args) > 0)) {
            printk("Invalid option: %s\n", ARG_CUR(args));
            CLEAN_UP_AND_RETURN(CMD_USAGE);
        }

        if (!arg_is_master) {
            intf.flags |= BCM_TIME_INPUT;
        }

        if (freq != ILLEGAL_OFFSET) {
            intf.flags |= BCM_TIME_DRIFT;
            intf.drift.isnegative = (freq < 0) ? 1 : 0;
            COMPILER_64_ZERO(intf.drift.seconds);
            freq = (freq < 0) ? (-freq) : (freq);
            intf.drift.nanoseconds = (uint32) freq;
        }

        if ((phase_sec != ILLEGAL_OFFSET) || (phase_nsec != ILLEGAL_OFFSET)) {
            /* Valid seconds or nanoseconds component of phase offset. */
            intf.flags |= BCM_TIME_OFFSET;

            phase_sec = (phase_sec == ILLEGAL_OFFSET) ? 0 : phase_sec;
            phase_nsec = (phase_nsec == ILLEGAL_OFFSET) ? 0 : phase_nsec; 

            /* Discard integer seconds, if any,  from nanoseconds term. */
            while (phase_nsec >= BS_NANOSECONDS_PER_SECOND) {
                phase_nsec -= BS_NANOSECONDS_PER_SECOND;
            } 
            while (phase_nsec <= -BS_NANOSECONDS_PER_SECOND) {
                phase_nsec += BS_NANOSECONDS_PER_SECOND;
            } 

            /* Reconcile different signs in seconds and nanoseconds terms. */
            if (phase_sec > 0 && phase_nsec < 0) {
                --phase_sec;
                phase_nsec += BS_NANOSECONDS_PER_SECOND;
            } else if (phase_sec < 0 && phase_nsec > 0) {
                ++phase_sec;
                phase_nsec -= BS_NANOSECONDS_PER_SECOND;
            }

            intf.offset.isnegative = ((phase_sec < 0) || (phase_nsec < 0)) ? 1 : 0;
            phase_sec = (intf.offset.isnegative) ? (-phase_sec) : (phase_sec);
            phase_nsec = (intf.offset.isnegative) ? (-phase_nsec) : (phase_nsec);

            COMPILER_64_SET(intf.offset.seconds, 0, ((uint32)phase_sec));
            intf.offset.nanoseconds = (uint32) phase_nsec;
        } else {
            /* Invalid seconds and nanoseconds component of phase offset. */
            intf.flags &= ~(BCM_TIME_OFFSET);
        }

        result = bcm_time_interface_add(unit, &intf);
        if (BCM_FAILURE(result)) {
            printk("Command failed. %s\n", bcm_errmsg(result));
            CLEAN_UP_AND_RETURN(CMD_FAIL);
        }
                    
        bs_initialized[unit] = 1;
        bs_id[unit] = intf.id;

        CLEAN_UP_AND_RETURN(CMD_OK);

    } else if (parse_cmp("STATus", arg_string_p, 0)) {
        int status;

        if (!bs_initialized[unit]) {
            printk("BroadSync not initialized on unit %d\n", unit);
            return CMD_FAIL;
        }

        arg_string_p = ARG_GET(args);

        if (arg_string_p != NULL) {
            return CMD_USAGE;
        }

        _bcm_time_bs_status_get(0, &status);

        printk("Status = %d\n", status);
    } else if (parse_cmp("1PPS", arg_string_p, 0)) {
        if (!bs_initialized[unit]) {
            printk("BroadSync not initialized on unit %d\n", unit);
            return CMD_FAIL;
        }

        arg_string_p = ARG_GET(args);

        if (arg_string_p == NULL) {
            int enabled;

            if (BCM_FAILURE(bcm_time_heartbeat_enable_get(unit, bs_id[unit], &enabled))) {
                printk("Error getting heartbeat (1pps) enable status\n");
                return CMD_FAIL;
            } else {
                printk("1PPS is %s\n", (enabled) ? "Enabled" : "Disabled");
                return CMD_OK;
            }
        }

        if (parse_cmp(arg_string_p, "ON", 0)) {
            result = bcm_time_heartbeat_enable_set(unit, bs_id[unit], 1);
        } else if (parse_cmp(arg_string_p, "OFF", 0)) {
            result = bcm_time_heartbeat_enable_set(unit, bs_id[unit], 0);
        } else {
            return CMD_USAGE;
        }

        if (BCM_FAILURE(result)) {
            printk("Command failed. %s\n", bcm_errmsg(result));
            return CMD_FAIL;
        }

    } else if (parse_cmp("DEBUG", arg_string_p, 0)) {
        arg_string_p = ARG_GET(args);

        if (arg_string_p == NULL) {
            uint32 flags;
            if (BCM_FAILURE(_bcm_mbox_debug_flag_get(&flags))) {
                printk("Error getting debug status\n");
            } else {
                printk("Debug output is %s\n", (flags) ? "On" : "Off");
                return CMD_OK;
            }
        }

        if (parse_cmp("ON", arg_string_p, 0)) {
            _bcm_mbox_debug_flag_set(0xff);
        } else if (parse_cmp(arg_string_p, "OFF", 0)) {
            _bcm_mbox_debug_flag_set(0);
        } else {
            return CMD_USAGE;
        }

    } else if (parse_cmp("LOG", arg_string_p, 0)) {
        parse_table_t parse_table;
        int arg_log_debug = 0;
        int arg_log_pll = 0;
        bcm_mac_t dest_mac = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; /* default to broadcast */
        bcm_mac_t src_mac = {0x00, 0x10, 0x18, 0x00, 0x00, 0x01};
        bcm_ip_t dest_ip = 0xffffffff; /* default to broadcast */
        bcm_ip_t src_ip = (192L << 24) + (168L << 16) +(0L << 8) + 90;
        int udp_port = 0x4455;
        int tpid = 0x8100;
        int vlan = 1;
        int ttl = 1;
        uint64 udp_log_mask = COMPILER_64_INIT(0,0);
        uint64 log_debug_bit = COMPILER_64_INIT(0,0x01);
        uint64 log_pll_bit = COMPILER_64_INIT(0,0x40);

        int rv = 0;

        parse_table_init(unit, &parse_table);

        parse_table_add(&parse_table, "LogDebug", PQ_DFL | PQ_BOOL | PQ_NO_EQ_OPT, (void*)0, &arg_log_debug, NULL);
        parse_table_add(&parse_table, "LogPLL", PQ_DFL | PQ_BOOL | PQ_NO_EQ_OPT, (void*)0, &arg_log_pll, NULL);
        parse_table_add(&parse_table, "DestMAC", PQ_DFL | PQ_MAC, NULL, dest_mac, NULL);
        parse_table_add(&parse_table, "DestAddr", PQ_DFL | PQ_IP, NULL, &dest_ip, NULL);
        parse_table_add(&parse_table, "SrcMAC", PQ_DFL | PQ_MAC, NULL, src_mac, NULL);
        parse_table_add(&parse_table, "SrcAddr", PQ_DFL | PQ_IP, NULL, &src_ip, NULL);
        parse_table_add(&parse_table, "UDPPort", PQ_DFL | PQ_INT, NULL, &udp_port, NULL);
        parse_table_add(&parse_table, "Vlan", PQ_DFL | PQ_INT, NULL, &vlan, NULL);

        if (parse_arg_eq(args, &parse_table) < 0) {
            printk("%s: Invalid option %s\n", ARG_CMD(args), ARG_CUR(args));
        }

        if (arg_log_debug) {
            COMPILER_64_OR(udp_log_mask, log_debug_bit);
        }
        if (arg_log_pll) {
            COMPILER_64_OR(udp_log_mask, log_pll_bit);
        }

        rv = _bcm_time_bs_log_configure(unit, 0xff, udp_log_mask,
                                        src_mac, dest_mac, tpid, vlan, ttl,
                                        src_ip, dest_ip, udp_port);
        
        if (rv != BCM_E_NONE) {
            printk("Failed setting log configuration: %s\n", bcm_errmsg(rv));
            return CMD_FAIL;
        }

    } else {
        return CMD_USAGE;
    }

    return CMD_OK;
}

#endif
