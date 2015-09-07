/*
 * $Id: stg.c,v 1.8 Broadcom SDK $
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
 * STG CLI commands
 */

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/diag.h>
#include <appl/diag/dport.h>
#include <shared/bsl.h>
#include <bcm/error.h>
#include <bcm/stg.h>
#include <bcm/debug.h>


STATIC int
do_show_stg_vlans(int unit, bcm_stg_t stg)
{
    vlan_id_t		*list;
    int			count, i, r;
    int			first = 1;

    if ((r = bcm_stg_vlan_list(unit, stg, &list, &count)) < 0) {
	cli_out("Error listing STG %d: %s\n", stg, bcm_errmsg(r));
	return r;
    }

    cli_out("STG %d: contains %d VLAN%s%s",
            stg, count, (count == 1) ? "" : "s", (count == 0) ? "" : " (");

    for (i = 0; i < count; i++) {
	int		span;

	cli_out("%s%d", first ? "" : ",", list[i]);
	first = 0;

	for (span = 1; i < count - 1 && list[i + 1] == list[i] + 1; span++) {
	    i++;
	}

	if (span > 1) {
	    cli_out("-%d", list[i]);
	}
    }

    cli_out("%s\n", (count == 0) ? "" : ")");

    bcm_stg_vlan_list_destroy(unit, list, count);

    return BCM_E_NONE;
}

STATIC int
do_show_stg_stp(int unit, bcm_stg_t stg)
{
    bcm_pbmp_t		pbmps[BCM_STG_STP_COUNT];
    char		buf[FORMAT_PBMP_MAX];
    int			state, r;
    soc_port_t		port, dport;
    bcm_port_config_t   pcfg;

    sal_memset(pbmps, 0, sizeof (pbmps));

    BCM_IF_ERROR_RETURN(bcm_port_config_get(unit, &pcfg));

    DPORT_BCM_PBMP_ITER(unit, pcfg.port, dport, port) {
	if ((r = bcm_stg_stp_get(unit, stg, port, &state)) < 0) {
	    return r;
	}

	BCM_PBMP_PORT_ADD(pbmps[state], port);
    }

    /* In current chips, LISTEN is not distinguished from BLOCK. */

    for (state = 0; state < BCM_STG_STP_COUNT; state++) {
	if (!(BCM_PBMP_IS_NULL(pbmps[state]))) {
	    format_bcm_pbmp(unit, buf, sizeof (buf), pbmps[state]);
	    cli_out("  %7s: %s\n", FORWARD_MODE(state), buf);
	}
    }

    return BCM_E_NONE;
}

cmd_result_t
if_esw_stg(int unit, args_t *a)
{
    char		*subcmd, *c;
    int			r;
    bcm_stg_t		stg = BCM_STG_INVALID;
    vlan_id_t		vid;
    int			state;
    pbmp_t		pbmp;
    bcm_port_config_t   pcfg;
    soc_port_t		port, dport;

    if (! sh_check_attached(ARG_CMD(a), unit)) {
	return CMD_FAIL;
    }

    if ((subcmd = ARG_GET(a)) == NULL) {
	return CMD_USAGE;
    }

    if (sal_strcasecmp(subcmd, "create") == 0) {

	if ((c = ARG_GET(a)) != NULL) {
	    stg = parse_integer(c);
	}

	if (stg == BCM_STG_INVALID) {
	    r = bcm_stg_create(unit, &stg);
	    cli_out("Created spanning tree group %d\n", stg);
	} else {
	    r = bcm_stg_create_id(unit, stg);
	}

	if (r < 0) {
	    goto bcm_err;
	}

	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "destroy") == 0) {

	if ((c = ARG_GET(a)) == NULL) {
	    return CMD_USAGE;
	}

	stg = parse_integer(c);

	if ((r = bcm_stg_destroy(unit, stg)) < 0) {
	    goto bcm_err;
	}

	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "show") == 0) {

	bcm_stg_t		*list;
	int			count, i;

	if ((c = ARG_GET(a)) != NULL) {
	    stg = parse_integer(c);
	}

	if (stg != BCM_STG_INVALID) {
	    return do_show_stg_vlans(unit, stg);
	}

	if ((r = bcm_stg_list(unit, &list, &count)) < 0) {
	    goto bcm_err;
	}

        /* Force print STG 0 */
        do_show_stg_vlans(unit, 0);
        do_show_stg_stp(unit, 0);

	for (i = 0; i < count; i++) {
	    if ((r = do_show_stg_vlans(unit, list[i])) < 0) {
		bcm_stg_list_destroy(unit, list, count);
		goto bcm_err;
	    }

	    if ((r = do_show_stg_stp(unit, list[i])) < 0) {
		bcm_stg_list_destroy(unit, list, count);
		goto bcm_err;
	    }
	}

	bcm_stg_list_destroy(unit, list, count);

	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "add") == 0) {

	if ((c = ARG_GET(a)) == NULL) {
	    return CMD_USAGE;
	}

	stg = parse_integer(c);

	while ((c = ARG_GET(a)) != NULL) {
	    vid = parse_integer(c);

	    if ((r = bcm_stg_vlan_add(unit, stg, vid)) < 0) {
		goto bcm_err;
	    }
	}

	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "remove") == 0) {

	if ((c = ARG_GET(a)) == NULL) {
	    return CMD_USAGE;
	}

	stg = parse_integer(c);

	while ((c = ARG_GET(a)) != NULL) {
	    vid = parse_integer(c);

	    if ((r = bcm_stg_vlan_remove(unit, stg, vid)) < 0) {
		goto bcm_err;
	    }
	}

	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "stp") == 0) {
	if ((c = ARG_GET(a)) == NULL) {
	    bcm_stg_t		*list;
	    int			count, i;

	    if ((r = bcm_stg_list(unit, &list, &count)) < 0) {
		goto bcm_err;
	    }

	    for (i = 0; i < count; i++) {
		cli_out("STG %d:\n", list[i]);

		if ((r = do_show_stg_stp(unit, list[i])) < 0) {
		    bcm_stg_list_destroy(unit, list, count);
		    goto bcm_err;
		}
	    }

	    bcm_stg_list_destroy(unit, list, count);

	    return CMD_OK;
	}

	stg = parse_integer(c);

	if ((c = ARG_GET(a)) == NULL) {
	    cli_out("STG %d:\n", stg);

	    if ((r = do_show_stg_stp(unit, stg)) < 0) {
		goto bcm_err;
	    }

	    return CMD_OK;
	}

	if (parse_bcm_pbmp(unit, c, &pbmp) < 0) {
	    return CMD_USAGE;
	}

	if ((c = ARG_GET(a)) == NULL) {
	    return CMD_USAGE;
	}

	for (state = 0; state < BCM_STG_STP_COUNT; state++) {
	    if (parse_cmp(forward_mode[state], c, '\0')) {
		break;
	    }
	}

	if (state == BCM_STG_STP_COUNT) {
	    return CMD_USAGE;
	}

        BCM_IF_ERROR_RETURN(bcm_port_config_get(unit, &pcfg));

	BCM_PBMP_AND(pbmp, pcfg.port);

        DPORT_BCM_PBMP_ITER(unit, pbmp, dport, port) {
	    if ((r = bcm_stg_stp_set(unit, stg, port, state)) < 0) {
		goto bcm_err;
	    }
	}

	return CMD_OK;
    }

    if (sal_strcasecmp(subcmd, "default") == 0) {

	if ((c = ARG_GET(a)) != NULL) {
	    stg = parse_integer(c);
	}

	if (stg == BCM_STG_INVALID) {
	    if ((r = bcm_stg_default_get(unit, &stg)) < 0) {
		goto bcm_err;
	    }

	    cli_out("Default STG is %d\n", stg);
	} else {
	    if ((r = bcm_stg_default_set(unit, stg)) < 0) {
		goto bcm_err;
	    }

	    cli_out("Default STG set to %d\n", stg);
	}

	return CMD_OK;
    }

    return CMD_USAGE;

 bcm_err:
    cli_out("%s: ERROR: %s\n", ARG_CMD(a), bcm_errmsg(r));
    return CMD_FAIL;
}
