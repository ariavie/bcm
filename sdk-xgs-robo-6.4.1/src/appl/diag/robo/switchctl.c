/*
 * $Id: switchctl.c,v 1.4 Broadcom SDK $
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
 * Switch Control commands
 */

#include <shared/bsl.h>

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/dport.h>

#include <bcm/error.h>
#include <bcm/switch.h>

STATIC char *robo_switch_control_names[] = {
    BCM_SWITCH_CONTROL_STR
};

char cmd_robo_switch_control_usage[] =
    "\n\tSwitchControl [PortBitMap=<pbmp>] [<name> | <name>=<value>]\n";

cmd_result_t
cmd_robo_switch_control(int unit, args_t *a)
{
    parse_table_t		pt;
    int				r;
    bcm_pbmp_t			arg_pbmp;
    int				arg_pbmp_given;
    bcm_switch_control_t	type;
    int				param;
    int				retCode = CMD_OK;
    bcm_port_t			port;
    char			*spec;
    char			name[128], *value;

    if (!sh_check_attached(ARG_CMD(a), unit)) {
        return CMD_FAIL;
    }

    if (COUNTOF(robo_switch_control_names) != bcmSwitch__Count) {
	cli_out("%s: Error: NAMES ARRAY OUT OF SYNC\n", ARG_CMD(a));
	return CMD_FAIL;
    }

    arg_pbmp = PBMP_ALL(unit);

    parse_table_init(unit, &pt);
    parse_table_add(&pt, "PortBitMap", 	PQ_DFL|PQ_PBMP|PQ_BCM,
		    (void *)(0), &arg_pbmp, NULL);
    if (parse_arg_eq(a, &pt) < 0) {
	parse_arg_eq_done(&pt);
	cli_out("%s: Error: Unknown option: %s\n", ARG_CMD(a), ARG_CUR(a));
	return CMD_FAIL;
    }

    arg_pbmp_given = ((pt.pt_entries[0].pq_type & PQ_PARSED) != 0);

    parse_arg_eq_done(&pt);

    if ((spec = ARG_GET(a)) == NULL) {
	if (!arg_pbmp_given) {
	    for (type = 0; type < bcmSwitch__Count; type++) {
		cli_out("%-40s", robo_switch_control_names[type]);
		r = bcm_switch_control_get(unit, type, &param);
		if (r < 0) {
		    cli_out("%s\n", bcm_errmsg(r));
		} else {
		    cli_out("0x%x\n", (uint32)param);
		}
	    }
	} else {
            BCM_PBMP_ITER(arg_pbmp, port) {
		cli_out("%s:\n", BCM_PORT_NAME(unit, port));
		for (type = 0; type < bcmSwitch__Count; type++) {
		    cli_out("    %-40s", robo_switch_control_names[type]);
		    r = bcm_switch_control_port_get(unit, port, type, &param);
		    if (r < 0) {
			cli_out("%s\n", bcm_errmsg(r));
		    } else {
			cli_out("0x%x\n", (uint32)param);
		    }
		}
	    }
	}
    } else {
	strncpy(name, spec, sizeof (name));
	name[sizeof (name) - 1] = 0;
	if ((value = strchr(name, '=')) != NULL) {
	    *value++ = 0;
	}

	for (type = 0; type < bcmSwitch__Count; type++) {
	    if (sal_strcasecmp(name, robo_switch_control_names[type]) == 0) {
		break;
	    }
	}

	if (type == bcmSwitch__Count) {
	    cli_out("Unknown switch control: %s\n", name);
	    return CMD_FAIL;
	}

	if (value == NULL) {
	    if (!arg_pbmp_given) {
		r = bcm_switch_control_get(unit, type, &param);
		if (r < 0) {
		    cli_out("bcm_switch_control_get ERROR: %s\n",
                            bcm_errmsg(r));
		    retCode = CMD_FAIL;
		} else {
		    cli_out("0x%x\n", (uint32)param);
		}
	    } else {
                BCM_PBMP_ITER(arg_pbmp, port) {
		    cli_out("%s: ", BCM_PORT_NAME(unit, port));
		    r = bcm_switch_control_port_get(unit, port, type, &param);
		    if (r < 0) {
			cli_out("bcm_switch_control_port_get ERROR: %s\n",
                                bcm_errmsg(r));
		    } else {
			cli_out("0x%x\n", (uint32)param);
		    }
		}
	    }
	} else {
	    param = parse_integer(value);

	    if (!arg_pbmp_given) {
		r = bcm_switch_control_set(unit, type, param);
		if (r < 0) {
		    cli_out("bcm_switch_control_set ERROR: %s\n",
                            bcm_errmsg(r));
		    retCode = CMD_FAIL;
		}
	    } else {
                BCM_PBMP_ITER(arg_pbmp, port) {
		    r = bcm_switch_control_port_set(unit, port, type, param);
		    if (r < 0) {
			cli_out("bcm_switch_control_port_set ERROR: "
                                "port=%d: %s\n",
                                port, bcm_errmsg(r));
			retCode = CMD_FAIL;
		    }
		}
	    }
	}
    }

    return retCode;
}
