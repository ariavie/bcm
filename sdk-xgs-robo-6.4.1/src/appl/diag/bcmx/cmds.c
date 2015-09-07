/*
 * $Id: cmds.c,v 1.19 Broadcom SDK $
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
 * File:     cmds.c
 * Purpose:  Other BCMX CLI commands
 */

#include <sal/appl/io.h>
#include <sal/appl/sal.h>

#include <bcm/rate.h>

#include <bcm/error.h>
#include <bcmx/bcmx.h>
#include <bcmx/rate.h>
#include <bcmx/link.h>

#include <appl/diag/shell.h>
#include <appl/diag/system.h>
#include <appl/diag/parse.h>

#include "bcmx.h"
#include <appl/diag/diag.h>

STATIC void
_bcmx_link_callback(bcmx_lport_t lport, bcm_port_info_t *info)
{
    if (info->linkstatus) {
        sal_printf("BCMX:  Link change UP, uport %s, %dMb %s.\n",
                   bcmx_lport_to_uport_str(lport), info->speed,
                   info->duplex ? "Full Duplex" : "Half Duplex");
    } else {
        sal_printf("BCMX:  Link change DOWN, uport %s.\n",
                   bcmx_lport_to_uport_str(lport));
    }
}


char bcmx_cmd_link_usage[] =
    "LINKscan [check | reg | unreg | <usecs>]\n"
    "    Simple linkscan control command.\n"
    "    With no arguments, does a simple enable-get\n"
    "    check    - Enable-get with consistency check\n"
    "    reg      - Register local handler\n"
    "    unreg    - Unregister local handler\n"
    "    <usecs>  - Enable set with the given time; 0 disables\n";

cmd_result_t
bcmx_cmd_link(int unit, args_t *args)
{
    int us, consistent;
    char *param;
    int rv;
    static int reg_count = 0;

    if ((param = ARG_GET(args)) == NULL) {
        rv = bcmx_linkscan_enable_get(&us, NULL);
        if (rv < 0) {
            sal_printf("ERROR getting bcmx link status %d: %s\n",
                       rv, bcm_errmsg(rv));
            return CMD_FAIL;
        }
        sal_printf("BCMX linkscan time %d: %s\n", us,
                   us > 0 ? "enabled" : "disabled");
        sal_printf("BCMX register count is %d\n", reg_count);
        return CMD_OK;
    }

    if (!sal_strcasecmp(param, "check")) {
        rv = bcmx_linkscan_enable_get(&us, &consistent);
        if (rv < 0) {
            sal_printf("ERROR getting bcmx link status %d: %s\n",
                       rv, bcm_errmsg(rv));
            return CMD_FAIL;
        }
        sal_printf("BCMX linkscan time %d: %s.  Units are %sconsistent\n",
                   us, us > 0 ? "enabled" : "disabled",
                   consistent ? "" : "not ");
        sal_printf("BCMX register count is %d\n", reg_count);
    } else if (!sal_strcasecmp(param, "reg")) {
        rv = bcmx_linkscan_register(_bcmx_link_callback);
        ++reg_count;
        sal_printf("BCMX linkscan register returns %d: %s\n",
                   rv, bcm_errmsg(rv));
    } else if (!sal_strcasecmp(param, "unreg")) {
        rv = bcmx_linkscan_unregister(_bcmx_link_callback);
        --reg_count;
        sal_printf("BCMX linkscan register returns %d: %s\n",
                   rv, bcm_errmsg(rv));
    } else if (isdigit((unsigned) *param) || *param == '-') {
        us = parse_integer(param);
        rv = bcmx_linkscan_enable_set(us);
        sal_printf("Set BCMX linkscan time to %d.  Result %d: %s\n", us,
                   rv, bcm_errmsg(rv));
    } else {
        sal_printf("Unknown subcommand %s.\n", param);
        return CMD_USAGE;
    }

    return CMD_OK;
}

char bcmx_cmd_rate_usage[] =
    "BCMX RATE Usages:\n"
    "    rate <Limit=val> [Bcast=val] [Mcast=val] [Dlf=val]\n"
    "                                - Set specified packet rate limit\n\n"
    "         limit : packets per second\n"
    "         bcast : Enable broadcast rate control\n"
    "         mcast : Enable multicast rate control\n"
    "         dlf   : Enable DLF flooding rate control\n"
    "         If no flags are given, displays the current rate settings\n";

cmd_result_t
bcmx_cmd_rate(int unit, args_t *args)
{
    parse_table_t pt;
    int limit;
    int bc_limit;
    int mc_limit;
    int dlf_limit;
    int flags = 0;
    int rv = BCM_E_NONE;

    if ((rv = bcmx_rate_get(&limit, &flags)) < 0) {
        sal_printf("ERROR: could not get rate: %s\n", bcm_errmsg(rv));
        return (CMD_FAIL);
    }

    bc_limit = ((flags & BCM_RATE_BCAST) ? TRUE : FALSE);
    mc_limit = ((flags & BCM_RATE_MCAST) ? TRUE : FALSE);
    dlf_limit = ((flags & BCM_RATE_DLF) ? TRUE : FALSE);

    parse_table_init(0, &pt);
    parse_table_add(&pt, "Limit", PQ_DFL|PQ_HEX, INT_TO_PTR(limit) , &limit, 0);
    parse_table_add(&pt, "Bcast", PQ_DFL|PQ_BOOL, (void *)0, &bc_limit, 0);
    parse_table_add(&pt, "Mcast", PQ_BOOL, INT_TO_PTR(mc_limit), &mc_limit, 0);
    parse_table_add(&pt, "Dlf", PQ_DFL|PQ_BOOL, INT_TO_PTR(dlf_limit), &dlf_limit, 0);

    if (ARG_GET(args) == NULL) {
        sal_printf("Current settings:\n");
        parse_eq_format(&pt);
        parse_arg_eq_done(&pt);

        return(CMD_OK);
    }

    if (parse_arg_eq(args, &pt) < 0) {
        sal_printf("Error: Unknown option: %s\n", ARG_CUR(args));
        parse_arg_eq_done(&pt);

        return(CMD_FAIL);
    }

    if (bc_limit) {
        flags |= (BCM_RATE_BCAST);
    } else {
        flags &= (~(BCM_RATE_BCAST));
    }

    if (mc_limit) {
        flags |= (BCM_RATE_MCAST);
    } else {
        flags &= (~(BCM_RATE_MCAST));
    }

    if (dlf_limit) {
        flags |= (BCM_RATE_DLF);
    } else {
        flags &= (~(BCM_RATE_DLF));
    }

    if ((rv = bcmx_rate_set(limit, flags)) < 0) {
        sal_printf("ERROR: could not set rate: %s\n", bcm_errmsg(rv));
        parse_arg_eq_done(&pt);

        return (CMD_FAIL);
    }

    parse_arg_eq_done(&pt);

    return CMD_OK;
}


