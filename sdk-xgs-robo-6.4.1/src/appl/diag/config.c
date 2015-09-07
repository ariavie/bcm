/*
 * $Id: config.c,v 1.15 Broadcom SDK $
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

#ifndef NO_SAL_APPL

#include <appl/diag/shell.h>
#include <appl/diag/system.h>
#include <sal/appl/config.h>
#include <shared/bsl.h>


char sh_config_usage[] =
    "Parameters: show <substring> | refresh |  \n\t"
    "                 save [filename=<filename>] [append=yes] [pattern=<substring>] | \n\t"
    "                 add [<var>=<value>] | delete <var>\n\t"
    "If no parameters are given, displays the current config vars.\n\t"
#ifndef COMPILER_STRING_CONST_LIMIT
    "  show                display current config vars. Next parameter (optional) maybe a substring to match\n\t"
    "  refresh             reload config vars from non-volatile storage\n\t"
    "  save                save config vars to non-volatile storage.\n\t"
    "                      it can optionally save current config to any given file \n\t"
    "                      providing the optional <pattern> will only save variables matching the pattern\n\t"
    "  append              append config vars to the end of the file.\n\t"
    "                      don\'t replace the old one.\n\t"
    "  <var>=<value>       change the value of a config var\n\t"
    "  add <var>=<value>   create a new config var\n\t"
    "  delete <var>        deletes a config var\n\t"
    "  clear               deletes all config vars\n\t"
    "  =                   prompt for new value for each config var\n\t"
#endif
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
    parse_table_t    pt;
    char        *name, *value;
    char        **parsed_value;
    int         i, rv, nconfig, add;
    char        *c = ARG_CUR(a);
    char        *substr;
    char        *new_fname = NULL;
    char        *pattern = NULL;
    char        *append = NULL;

    rv = CMD_OK;

    if (c == NULL || !sal_strcasecmp(c, "show")) {
        name = NULL;
        ARG_NEXT(a);
        substr = ARG_CUR(a);
        while (sal_config_get_next(&name, &value) >= 0) {
            if ((substr == NULL) || (sal_strstr(name, substr) != NULL)) {
                cli_out("    %s=%s\n", name, value);
            }
        }
        if (NULL != c) {
            ARG_NEXT(a);
        }
        return CMD_OK;
    }

    if (!sal_strcasecmp(c, "refresh")) {
        ARG_NEXT(a);
        cli_out("%s: Refreshing configuration database\n", ARG_CMD(a));
        if (sal_config_refresh()) {
            cli_out("%s: Failed to refresh configuration database\n",
                    ARG_CMD(a));
            return CMD_FAIL;
        }
        return CMD_OK;
    }

    if (!sal_strcasecmp(c, "save")) {
        parse_table_t    pt_save;
        parse_table_init(u, &pt_save);
        parse_table_add(&pt_save, "filename", PQ_STRING, NULL,
                                (void *)&new_fname, NULL);
        parse_table_add(&pt_save, "append", PQ_STRING, NULL,
                                (void *)&append, NULL);
        parse_table_add(&pt_save, "pattern", PQ_STRING, NULL,
                                (void *)&pattern, NULL);
        ARG_NEXT(a);
        if (0 > parse_arg_eq(a, &pt_save)) {
            parse_arg_eq_done(&pt_save);
            return CMD_USAGE;
        }
        if (ARG_CNT(a) > 0) {
            cli_out("%s: Invalid option: %s\n", ARG_CMD(a), ARG_CUR(a));
            return CMD_USAGE;
        }
        if (sal_strlen(new_fname)) {
            rv = sal_config_save(new_fname, pattern, sal_strcasecmp(append, "yes") ? 0 : 1);
        } else {
            rv = sal_config_flush();
        }
        parse_arg_eq_done(&pt_save);

        if (rv) {
            cli_out("%s: Warning: sal_config_flush failed\n", ARG_CMD(a));
            return CMD_FAIL;
        }
        return CMD_OK;
    }

    if (!sal_strcasecmp(c, "delete")) {
        ARG_NEXT(a);
        while ((c = ARG_GET(a)) != NULL) {
            if (sal_config_set(c, 0) != 0) {
                cli_out("%s: Variable not found: %s\n", ARG_CMD(a), c);
                rv = CMD_FAIL;
            }
        }
        return rv;
    }

    if (!sal_strcasecmp(c, "clear")) {
        ARG_NEXT(a);
        for (;;) {
            name = NULL;
            if (sal_config_get_next(&name, &value) < 0) {
                break;
            }
            if (sal_config_set(name, 0) < 0) {
                cli_out("%s: Variable not found: %s\n", ARG_CMD(a), name);
                rv = CMD_FAIL;
                break;
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

    /* count config variables first, to allocate parsed_value array */
    name = NULL;
    nconfig = 0;
    while (sal_config_get_next(&name, &value) >= 0) {
        nconfig += 1;
    }
    if (nconfig) {
        parsed_value = sal_alloc(sizeof(*parsed_value) * nconfig,
                                 "config values");
        if (parsed_value == NULL) {
            cli_out("%s: cannot allocate memory for config values\n",
                    ARG_CMD(a));
            return CMD_FAIL;
        }
        sal_memset(parsed_value, 0, sizeof(*parsed_value) * nconfig);

        name = NULL;
        parse_table_init(u, &pt);
        i = 0;
        while (sal_config_get_next(&name, &value) >= 0) {
            if (parse_table_add(&pt, name, PQ_STRING, value,
                                (void *)&parsed_value[i], NULL) < 0) {
                cli_out("Internal error in parsing\n");
                return CMD_FAIL;
            }
            i += 1;
        }

        if (0 > parse_arg_eq(a, &pt)) {
            cli_out("%s: Invalid option: %s\n", ARG_CMD(a), ARG_CUR(a));
            parse_arg_eq_done(&pt);
            return CMD_USAGE;
        }

        /* For each that is modified, set new value */

        for (i = 0; i < pt.pt_cnt; i++) {
            if (pt.pt_entries[i].pq_type & PQ_PARSED) {
                if (sal_config_set(pt.pt_entries[i].pq_s,
                                   parsed_value[i]) < 0) {
                    rv = CMD_FAIL;
                }
            }
        }

        parse_arg_eq_done(&pt);
        if (parsed_value) {
            sal_free(parsed_value);
        }
    }

    if (ARG_CNT(a)) {
        while ((c = ARG_GET(a)) != NULL) {
            char *s;
            s = sal_strchr(c, '=');
            if (s == NULL) {
                cli_out("%s: Invalid assignment: %s\n", ARG_CMD(a), c);
                rv = CMD_FAIL;
            } else if (! add) {
                cli_out("%s: Must use 'add' to create new variable: %s\n",
                        ARG_CMD(a), c);
                rv = CMD_FAIL;
            } else {
                *s++ = 0;
                if (sal_config_set(c, s) < 0) {
                    rv = CMD_FAIL;
                }
            }
        }
    }

    return rv;
}

#endif /* NO_SAL_APPL */
