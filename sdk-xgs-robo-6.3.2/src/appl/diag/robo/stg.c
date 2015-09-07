/*
 * $Id: stg.c 1.10 Broadcom SDK $
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

#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/diag.h>

#include <bcm/error.h>
#include <bcm/stg.h>
#include <bcm/debug.h>


STATIC int
do_robo_show_stg_vlans(int unit, bcm_stg_t stg)
{
    vlan_id_t *list;
    int count, i, r;
    int first = 1;

    if ((r = bcm_stg_vlan_list(unit, stg, &list, &count)) < 0) {
        printk("Error listing STG %d: %s\n", stg, bcm_errmsg(r));
        return r;
    }

    printk("STG %d: contains %d VLAN%s%s",
       stg, count, (count == 1) ? "" : "s", (count == 0) ? "" : " (");

    for (i = 0; i < count; i++) {
        int span;

        printk("%s%d", first ? "" : ",", list[i]);
        first = 0;

        for (span = 1; i < count - 1 && list[i + 1] == list[i] + 1; span++) {
            i++;
        }

        if (span > 1) {
            printk("-%d", list[i]);
        }
    }

    printk("%s\n", (count == 0) ? "" : ")");

    bcm_stg_vlan_list_destroy(unit, list, count);

    return BCM_E_NONE;
}

STATIC int
do_robo_show_stg_stp(int unit, bcm_stg_t stg)
{
    pbmp_t pbmps[BCM_STG_STP_COUNT];
    char buf[256];
    int state, r;
    soc_port_t port;

    memset(pbmps, 0, sizeof (pbmps));

    PBMP_PORT_ITER(unit, port) {
        if ((r = bcm_stg_stp_get(unit, stg, port, &state)) < 0) {
            return r;
        }

        SOC_PBMP_PORT_ADD(pbmps[state], port);
    }

    /* In current chips, LISTEN is not distinguished from BLOCK. */

    for (state = 0; state < BCM_STG_STP_COUNT; state++) {
        if (!(BCM_PBMP_IS_NULL(pbmps[state]))) {
            format_pbmp(unit, buf, sizeof (buf), pbmps[state]);
            printk("  %7s: %s\n", ROBO_FORWARD_MODE(state), buf);
        }
    }

    return BCM_E_NONE;
}
        
cmd_result_t
if_robo_stg(int unit, args_t *a)
{
    char *subcmd, *c;
    int r;
    bcm_stg_t stg = BCM_STG_INVALID;
    vlan_id_t vid;
    int state;
    pbmp_t pbmp;
    soc_port_t port;

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
            printk("Created spanning tree group %d\n", stg);
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

        bcm_stg_t *list;
        int count, i;

        if ((c = ARG_GET(a)) != NULL) {
            stg = parse_integer(c);
        }

        if (stg != BCM_STG_INVALID) {
            return do_robo_show_stg_vlans(unit, stg);
        }

        if ((r = bcm_stg_list(unit, &list, &count)) < 0) {
            goto bcm_err;
        }

        for (i = 0; i < count; i++) {
            if ((r = do_robo_show_stg_vlans(unit, list[i])) < 0) {
                bcm_stg_list_destroy(unit, list, count);
                goto bcm_err;
            }

            if ((r = do_robo_show_stg_stp(unit, list[i])) < 0) {
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
    if (sal_strcasecmp(subcmd, "clear") == 0) {

        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }
        stg = parse_integer(c);
        if ((r = bcm_stg_vlan_remove_all(unit, stg)) < 0) {
            goto bcm_err;
        }
        return CMD_OK;
    }
    if (sal_strcasecmp(subcmd, "stp") == 0) {
        if ((c = ARG_GET(a)) == NULL) {
            bcm_stg_t *list;
            int count, i;

            if ((r = bcm_stg_list(unit, &list, &count)) < 0) {
                goto bcm_err;
            }

            for (i = 0; i < count; i++) {
                printk("STG %d:\n", list[i]);

                if ((r = do_robo_show_stg_stp(unit, list[i])) < 0) {
                    bcm_stg_list_destroy(unit, list, count);
                    goto bcm_err;
                }
            }

            bcm_stg_list_destroy(unit, list, count);

            return CMD_OK;
        }

        stg = parse_integer(c);

        if ((c = ARG_GET(a)) == NULL) {
            printk("STG %d:\n", stg);

            if ((r = do_robo_show_stg_stp(unit, stg)) < 0) {
                goto bcm_err;
            }

            return CMD_OK;
        }

        if (parse_pbmp(unit, c, &pbmp) < 0) {
            return CMD_USAGE;
        }

        if ((c = ARG_GET(a)) == NULL) {
            return CMD_USAGE;
        }

        for (state = 0; state < BCM_STG_STP_COUNT; state++) {
            if (parse_cmp(robo_forward_mode[state], c, '\0')) {
                break;
            }
        }

        if (state == BCM_STG_STP_COUNT) {
            return CMD_USAGE;
        }

        SOC_PBMP_AND(pbmp, PBMP_ALL(unit));

        PBMP_ITER(pbmp, port) {
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

            printk("Default STG is %d\n", stg);
        } else {
            if ((r = bcm_stg_default_set(unit, stg)) < 0) {
                goto bcm_err;
            }

            printk("Default STG set to %d\n", stg);
        }

        return CMD_OK;
    }

    return CMD_USAGE;

 bcm_err:
    printk("%s: ERROR: %s\n", ARG_CMD(a), bcm_errmsg(r));
    return CMD_FAIL;
}

