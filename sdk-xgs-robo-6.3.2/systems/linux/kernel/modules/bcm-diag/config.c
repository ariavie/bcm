/*
 * $Id: config.c 1.6 Broadcom SDK $
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
 * System configuration command.
 */

#include <appl/diag/shell.h>
#include <appl/diag/system.h>
#include <kconfig.h>

char sh_config_usage[] =
    "Parameters: show | refresh | save | [add] [<var>=<value>] | delete <var>\n\t"
    "If no parameters are given, displays the current config vars.\n\t"
    "  show                display current config vars\n\t"
    "  <var>=<value>       change the value of a config var\n\t"
    "  add <var>=<value>   create a new config var\n\t"
    "  delete <var>        deletes a config var\n\t"
    "  =                   prompt for new value for each config var\n\t"
    "NOTE: changes are not retained permanently unless saved\n";

cmd_result_t
sh_config(int u, args_t *a)
/*
 * Function: 	sh_config
 * Purpose:	Configure switch management information.
 * Parameters:
 * Returns:
 */
{
    int	rv, add;
    char *name, *value, *c = ARG_CUR(a);

    rv = CMD_OK;

    if (c == NULL || !sal_strcasecmp(c, "show")) {
        name = NULL;
        while (kconfig_get_next(&name, &value) >= 0) {
            sal_printf("\t%s=%s\n", name, value);
        }
        if (NULL != c) {
            ARG_NEXT(a);
        }
        return CMD_OK;
    }

    if (!sal_strcasecmp(c, "delete")) {
	ARG_NEXT(a);
	while ((c = ARG_GET(a)) != NULL) {
	    if (kconfig_set(c, 0) != 0) {
		printk("%s: Variable not found: %s\n", ARG_CMD(a), c);
		rv = CMD_FAIL;
	    }
	}
	return rv;
    }

    if (!sal_strcasecmp(c, "add")) {
	ARG_NEXT(a);
	add = 1;
    } else {
	add = 0;
    }

    if (ARG_CNT(a)) {
	while ((c = ARG_GET(a)) != NULL) {
	    char *s;
	    s = strchr(c, '=');
	    if (s == NULL) {
		printk("%s: Invalid assignment: %s\n", ARG_CMD(a), c);
		rv = CMD_FAIL;
	    } else {
                *s++ = 0;
                if (!add && kconfig_get(c) == NULL) {
                    printk("%s: Must use 'add' to create new variable: %s\n",
                           ARG_CMD(a), c);
                    rv = CMD_FAIL;
                } else {
                    if (kconfig_set(c, s) < 0) {
                        rv = CMD_FAIL;
                    }
		}
	    }
	}
    }

    return rv;
}
