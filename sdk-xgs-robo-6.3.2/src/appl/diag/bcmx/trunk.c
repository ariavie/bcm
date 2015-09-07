/*
 * $Id: trunk.c 1.11 Broadcom SDK $
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
 * File:     trunk.c
 * Purpose:  BCMX trunking commands
 */

#include <sal/appl/io.h>

#include <bcm/trunk.h>

#include <bcm/error.h>
#include <bcmx/bcmx.h>
#include <bcmx/trunk.h>

#include <appl/diag/shell.h>
#include <appl/diag/system.h>
#include <appl/diag/parse.h>

#include "bcmx.h"
#include <appl/diag/diag.h>

char bcmx_cmd_trunk_usage[] =
    "BCMX Trunk Usages:\n"
    "    trunk create <tid>               - Create a trunk\n"
    "    trunk destroy <tid>              - Destroy a trunk\n"
    "    trunk set <tid> <uport-list>     - Add ports to a trunk\n"
    "    trunk show <tid>                 - Display trunk information\n";

cmd_result_t
bcmx_cmd_trunk(int unit, args_t *args)
{
    char *subcmd, *ch;
    bcmx_trunk_add_info_t tai;
    int lplist_given, tid;
    int rv = BCM_E_NONE;

    if ((subcmd = ARG_GET(args)) == NULL) {
        sal_printf("%s: Subcommand required\n", ARG_CMD(args));
        return CMD_USAGE;
    }

    if ((ch = ARG_GET(args)) == NULL) {
        sal_printf("%s: trunk ID not specified\n", ARG_CMD(args));
        return CMD_FAIL;
    } else {
        tid = parse_integer(ch);
    }

    bcmx_trunk_add_info_t_init(&tai);

    if ((ch = ARG_GET(args)) == NULL) {
        lplist_given = 0;
    } else {
        if (bcmx_lplist_parse(&tai.ports, ch) < 0) {
            sal_printf("%s: could not parse plist: %s\n", ARG_CMD(args), ch);
            bcmx_trunk_add_info_t_free(&tai);
            return CMD_FAIL;
        }
	lplist_given = 1;
    }

    if (sal_strcasecmp(subcmd, "create") == 0) {
	if (tid < 0) {
	    rv = bcmx_trunk_create(&tid);
	    if (rv >= 0) {
		sal_printf("%s: trunk %d created\n", ARG_CMD(args), tid);
	    }
	} else {
	    rv = bcmx_trunk_create_id(tid);
	}
        goto done;
    }

    if (sal_strcasecmp(subcmd, "destroy") == 0) {
        rv = bcmx_trunk_destroy(tid);
        goto done;
    }

    if (sal_strcasecmp(subcmd, "set") == 0) {
        if (!lplist_given) {
            sal_printf("%s: Bad port list or not specified\n", ARG_CMD(args));
            bcmx_trunk_add_info_t_free(&tai);
            return CMD_USAGE;
        }

	/* default lots of things, lets bcm trunk code choose */
        tai.psc = -1;
        tai.dlf_port = BCMX_LPORT_INVALID;
        tai.mc_port = BCMX_LPORT_INVALID;
        tai.ipmc_port = BCMX_LPORT_INVALID;

        rv = bcmx_trunk_set(tid, &tai);

        goto done;
    }

    if (sal_strcasecmp(subcmd, "show") == 0) {
        bcmx_lplist_free(&tai.ports);

        if ((rv = bcmx_trunk_get(tid, &tai)) == BCM_E_NONE) {
            bcmx_lport_t  lport;
            int           count;

            sal_printf("trunk %d psc=%d ", tid, tai.psc);
            sal_printf("dlf=%s ", bcmx_lport_to_uport_str(tai.dlf_port));
            sal_printf("mc=%s ", bcmx_lport_to_uport_str(tai.mc_port));
            sal_printf("ipmc=%s ", bcmx_lport_to_uport_str(tai.ipmc_port));

            sal_printf("uports=");
            BCMX_LPLIST_ITER(tai.ports, lport, count) {
                sal_printf("%s%s", count == 0 ? "" : ",",
                           bcmx_lport_to_uport_str(lport));
            }
            if (count == 0) {
                sal_printf("none\n");
            } else {
                sal_printf("\n");
            }
        }
        goto done;
    }

    sal_printf("%s: ERROR: unknown subcommand: %s\n", ARG_CMD(args), subcmd);
    bcmx_lplist_free(&tai.ports);
    return CMD_USAGE;
	
 done:
    if (rv < 0) {
        sal_printf("%s: ERROR: %s\n", ARG_CMD(args), bcm_errmsg(rv));
        bcmx_trunk_add_info_t_free(&tai);
	    return CMD_FAIL;
    }

    bcmx_trunk_add_info_t_free(&tai);

    return CMD_OK;
}
