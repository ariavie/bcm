/*
 * $Id: stat.c,v 1.7 Broadcom SDK $
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
 * File:        stat.c
 * Purpose:	BCMX diagnostic stat command
 */

#ifdef	INCLUDE_BCMX

#include <bcm/types.h>
#include <bcm/error.h>
#include <bcm/stat.h>
#include <bcmx/stat.h>
#include <bcmx/lplist.h>

#include <appl/diag/parse.h>
#include <appl/diag/shell.h>
#include <appl/diag/system.h>
#include "bcmx.h"

char bcmx_cmd_stat_usage[] =
	"\n"
	"\tstat init\n"
	"\tstat show [all] <user-port-list>\n"
	"\tstat clear [<user-port-list>]\n";

cmd_result_t
bcmx_cmd_stat(int unit, args_t *args)
{
    char		*cmd, *subcmd, *s;
    int			rv, all, count;
    bcmx_lport_t	lport;
    bcmx_lplist_t	lplist;
    bcm_stat_val_t	stype;
    uint64		sval;
    char		*sname;
    char        *_stat_names[] = BCM_STAT_NAME_INITIALIZER;

    cmd = ARG_CMD(args);
    subcmd = ARG_GET(args);
    if (subcmd == NULL) {
	subcmd = "show";
    }

    if (sal_strcasecmp(subcmd, "init") == 0) {
	rv = bcmx_stat_init();
	if (rv < 0) {
	    sal_printf("%s: ERROR: %s failed: %s\n",
		       cmd, subcmd, bcm_errmsg(rv));
	    return CMD_FAIL;
	}
	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "show") == 0) {
	all = 0;
	s = ARG_GET(args);
	if (s != NULL && sal_strcasecmp(s, "all") == 0) {
	    all = 1;
	    s = ARG_GET(args);
	}
	if (s == NULL) {
	    s = "*";
	}
	if (bcmx_lplist_init(&lplist, 0, 0) < 0) {
	    sal_printf("%s: ERROR: could not init port list\n", cmd);
	    return CMD_FAIL;
	}
        if (bcmx_lplist_parse(&lplist, s) < 0) {
            sal_printf("%s: ERROR: could not parse port list: %s\n",
                         cmd, s);
            return CMD_FAIL;
        }

	BCMX_LPLIST_ITER(lplist, lport, count) {
        sal_printf("%s: Statistics for port %s\n",
                   cmd, bcmx_lport_to_uport_str(lport));

	    for (stype = (bcm_stat_val_t)0; stype < snmpValCount; stype++) {
		sname = _stat_names[stype];
		rv = bcmx_stat_get(lport, stype, &sval);
		if (rv < 0) {
		    sal_printf("\t%18s\t%s (stat %d): %s\n",
			       "-", sname, stype, bcm_errmsg(rv));
		    continue;
		}
		if (all == 0 && COMPILER_64_IS_ZERO(sval)) {
		    continue;
		}
		if (COMPILER_64_HI(sval) == 0) {
		    sal_printf("\t%18u\t%s (stat %d)\n",
			       COMPILER_64_LO(sval), sname, stype);
		} else {
		    sal_printf("\t0x%08x%08x\t%s (stat %d)\n",
			       COMPILER_64_HI(sval),
			       COMPILER_64_LO(sval),
			       sname, stype);
		}

	    }
	}
        bcmx_lplist_free(&lplist);
	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "clear") == 0) {
	s = ARG_GET(args);
	if (s == NULL) {
	    s = "*";
	}
	if (bcmx_lplist_init(&lplist, 0, 0) < 0) {
	    sal_printf("%s: ERROR: could not init port list\n", cmd);
	    return CMD_FAIL;
	}
        if (bcmx_lplist_parse(&lplist, s) < 0) {
            sal_printf("%s: ERROR: could not parse port list: %s\n",
                         cmd, s);
            return CMD_FAIL;
        }

	BCMX_LPLIST_ITER(lplist, lport, count) {
	    rv = bcmx_stat_clear(lport);
	    if (rv < 0) {
            sal_printf("%s: ERROR: %s on port %s failed: %s\n",
                       cmd, subcmd,
                       bcmx_lport_to_uport_str(lport), bcm_errmsg(rv));
            return CMD_FAIL;
	    }
	}
        bcmx_lplist_free(&lplist);
	return CMD_OK;
    }

    sal_printf("%s: unknown subcommand '%s'\n", ARG_CMD(args), subcmd);
    return CMD_USAGE;
}

#endif	/* INCLUDE_BCMX */
