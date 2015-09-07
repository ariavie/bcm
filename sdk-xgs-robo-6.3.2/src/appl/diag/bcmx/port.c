/*
 * $Id: port.c 1.21 Broadcom SDK $
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
 * File:        bcmxdiag/port.c
 * Purpose:     Implement bcmx port command
 * Requires:    
 */

#include <bcm/port.h>
#include <bcm/error.h>

#include <bcmx/port.h>
#include <bcmx/bcmx.h>

#include <appl/diag/shell.h>
#include <appl/diag/system.h>
#include <appl/diag/parse.h>

#include "bcmx.h"
#include <appl/diag/diag.h>

#define PCMD_DONE(rv) cmd_rv = (rv); goto done

char bcmx_cmd_port_usage[] =
    "    port [-b] <uport-list> [opts]    - List (briefly) port properties\n"
    "    port <uport-list> [opts]         - Set options indicated\n"
#ifndef COMPILER_STRING_CONST_LIMIT
    "    Options include:\n"
    "        AutoNeg[=on|off]                 LinkScan[=on|off|hw|sw]\n"
    "        SPeed[=10|100|1000]              FullDuplex[=true|false]\n"
    "        TxPAUse[=on|off]                 RxPAUse[=on|off]\n"
    "        STationADdr[=<macaddr>]          LeaRN[=<learnmode>]\n"
    "        DIScard[=none|untag|all]         VlanFilter[=on|off]\n"
    "        UTPRIrity[=<pri>]                UTVlan[=<vid>]\n"
    "        PortFilterMode[=<value>]         PHymaster[=Master|Slave|Auto]\n"
    "Speed, duplex, pause are the ADVERTISED or FORCED according to autoneg.\n"
    "Speed of zero indicates maximum speed.\n"
    "LinkScan enables automatic scanning for link changes.\n"
    "VlanFilter drops input packets that are not tagged with a valid VLAN\n"
    "Priority sets the priority for untagged packets.\n"
    "PortFilterMode takes a value 0/1/2 for mode A/B/C (see register ref).\n"
    "<vid> is valid VLAN identifier.  <pri> is a value from 0 to 7.\n"
    "<learnmode> flag: bit 0 --> enable learning.  \n"
    "    bit 1 --> forward to CPU.   bit 2 --> switch packet\n"
#endif
    ;

/* Note that 'unit' is left over from the bcm layer.  It should be ignored */
cmd_result_t
bcmx_cmd_port(int unit, args_t *args)
{
    char *ch;
    bcmx_lplist_t ports;
    bcm_port_info_t given;
    bcm_port_info_t info;
    bcm_port_info_t *info_p;
    bcm_port_info_t *port_info = NULL;
    int rv;
    int count;
    bcmx_lport_t lport;
    int brief = FALSE;
    parse_table_t pt;
    uint32 seen = 0;
    uint32 port_flags;
    uint32 parsed = 0;
    char uport_str[BCMX_UPORT_STR_LENGTH_DEFAULT];
    bcmx_uport_t uport;
    cmd_result_t cmd_rv = CMD_OK;

    if ((ch = ARG_GET(args)) == NULL) {
        return(CMD_USAGE);
    }

    if (!strcmp("-b", ch)) {
        brief = TRUE;
        if ((ch = ARG_GET(args)) == NULL) {
            return(CMD_USAGE);
        }
    }

    bcmx_lplist_init(&ports, 0, 0);

    if (bcmx_lplist_parse(&ports, ch) < 0) {
        soc_cm_print("%s: Error: could not parse portlist: %s\n",
               ARG_CMD(args), ch);
        PCMD_DONE(CMD_FAIL);
    }

    if (ports.lp_last < 0) {
        soc_cm_print("No ports specified.\n");
        PCMD_DONE(CMD_FAIL);
    }

    if (ARG_CNT(args) == 0) {
        seen = BCM_PORT_ATTR_ALL_MASK & ~BCM_PORT_ATTR_RATE_MASK;
    } else { /* Some arguments given..... */
        /*
         * Parse the arguments.  Determine which ones were actually given.
         */
        if (SOC_IS_ROBO(unit)){
#if defined(BCM_ROBO_SUPPORT)
            robo_port_parse_setup(0, &pt, &given);
#endif
        } else {
#if defined(BCM_ESW_SUPPORT)
        port_parse_setup(0, &pt, &given);
#endif
        }
        
        if (parse_arg_eq(args, &pt) < 0) {
            PCMD_DONE(CMD_USAGE);
        }

        if (ARG_CNT(args) > 0) {
            soc_cm_print("%s: Unknown argument %s\n",
                         ARG_CMD(args), ARG_CUR(args));
            PCMD_DONE(CMD_FAIL);
        }

        /*
         * Find out what parameters specified.  Record values specified.
         */
        if (SOC_IS_ROBO(unit)){
#if defined(BCM_ROBO_SUPPORT)
            robo_port_parse_mask_get(&pt, &seen, &parsed);
#endif
        } else {
#if defined(BCM_ESW_SUPPORT)
        port_parse_mask_get(&pt, &seen, &parsed);
#endif
        }

    }

    if (seen && parsed) {
        soc_cm_print("%s: Cannot get and set port properties in one command\n",
                     ARG_CMD(args));
        PCMD_DONE(CMD_FAIL);
    } else if (seen) { /* Show selected information */
        bcm_port_config_t config;
        int bcm_unit, bcm_port;

        /****************************************************************
         * DISPLAY PORT INFO
         ****************************************************************/
        if (brief) {
            soc_cm_print("  Uport\n");
            if (SOC_IS_ROBO(unit)){
#if defined(BCM_ROBO_SUPPORT)
            robo_brief_port_info_header(-1);
#endif
            } else {
#if defined(BCM_ESW_SUPPORT)
            brief_port_info_header(-1);
#endif
            }
        } else {
            soc_cm_print("Uport Status (* indicates PHY link up)\n");
        }

        BCMX_LPLIST_ITER(ports, lport, count) {
            uport = BCMX_LPORT_TO_UPORT(lport);
            bcmx_uport_to_str(uport, uport_str, BCMX_UPORT_STR_LENGTH_DEFAULT);
            bcmx_lport_to_unit_port(lport, &bcm_unit, &bcm_port);
            rv = bcm_port_config_get(bcm_unit, &config);

            if (BCM_FAILURE(rv)) {
                soc_cm_print("%s: Could not get port config information: %s\n",
                             ARG_CMD(args), bcm_errmsg(rv));
                PCMD_DONE(CMD_FAIL);
            }

            if (BCM_PBMP_MEMBER(config.cpu, bcm_port)) {
                if (brief) {
                    soc_cm_print("%7s  up     CPU port for unit %d\n",
                                 uport_str, bcm_unit);
                } else {
                    soc_cm_print(" *%s CPU port for unit %d\n",
                                 uport_str, bcm_unit);
                }
                continue;
            }

	    port_flags = seen;
            if (SOC_IS_ROBO(unit)){
#if defined(BCM_ROBO_SUPPORT)
                robo_port_info_init(-1, lport, &info, port_flags);
#endif
            } else {
#if defined(BCM_ESW_SUPPORT)
               port_info_init(-1, lport, &info, port_flags);
#endif
            }
            rv = bcmx_port_selective_get(lport, &info);
            if (BCM_FAILURE(rv)) {
                soc_cm_print("%s: Could not get port %s information: %s\n",
                             ARG_CMD(args), uport_str, bcm_errmsg(rv));
                PCMD_DONE(CMD_FAIL);
            }

            if (brief) {
                if (SOC_IS_ROBO(unit)){
#if defined(BCM_ROBO_SUPPORT)
                    robo_brief_port_info(uport_str, &info, port_flags);
#endif
                } else {
#if defined(BCM_ESW_SUPPORT)
                    brief_port_info(uport_str, &info, port_flags);
#endif
                }
            } else {
                if (SOC_IS_ROBO(unit)){
#if defined(BCM_ROBO_SUPPORT)
                    robo_disp_port_info(uport_str, &info, 0, port_flags);
#endif
                } else {
#if defined(BCM_ESW_SUPPORT)
                    disp_port_info(uport_str, &info, 0, port_flags);
#endif
                }
           }            
        }
        PCMD_DONE(CMD_OK);
    }

    /****************************************************************
     *
     * SET PORT INFO
     *     Allocate all the info structures
     *     Get the info for all the ports listed
     *     Adjust the structures according to input
     *     Write all the data back at once
     *
     * After allocating ports, make sure to goto done on error.
     *
     ****************************************************************/

    if (parsed & BCM_PORT_ATTR_LINKSCAN_MASK) {
        /* Map ON --> S/W, OFF--> None */
        if (given.linkscan > 2) {
            given.linkscan -= 3;
        }
    }

    port_info = sal_alloc((ports.lp_last+1) * sizeof(bcm_port_info_t),
                          "bcmx_port");
    if (!port_info) {
        soc_cm_print("Could not allocate enough memory for %d ports\n",
                     (ports.lp_last+1));
        PCMD_DONE(CMD_FAIL);
    }

    /* Get and adjust the information for the selected ports */
    BCMX_LPLIST_ITER(ports, lport, count) {
        info_p = &port_info[count];
        if (SOC_IS_ROBO(unit)){
#if defined(BCM_ROBO_SUPPORT)
                robo_port_info_init(-1, lport, info_p, parsed);
#endif
        } else {
#if defined(BCM_ESW_SUPPORT)
        port_info_init(-1, lport, info_p, parsed);
#endif
        }        

        rv = bcmx_port_selective_get(lport, info_p);
        if (BCM_FAILURE(rv)) {
            soc_cm_print("%s: ERROR: port info get for port %s returned %s\n",
                         ARG_CMD(args), bcmx_lport_to_uport_str(lport),
                         bcm_errmsg(rv));
            PCMD_DONE(CMD_FAIL);
        }
    }

    BCMX_LPLIST_ITER(ports, lport, count) {
        info_p = &port_info[count];
        /* Get max speed and abilities for the port */
        if ((rv = bcmx_port_speed_max(lport, &given.speed_max)) < 0) {
            soc_cm_print("port parse: Could not get port %s max speed: %s\n",
                         bcmx_lport_to_uport_str(lport), bcm_errmsg(rv));
            cmd_rv = CMD_FAIL;
            continue;
        }
        if ((rv = bcmx_port_speed_max(lport, &given.speed)) < 0) {
            soc_cm_print("port parse: Could not get port %s peed: %s\n",
                         bcmx_lport_to_uport_str(lport), bcm_errmsg(rv));
            cmd_rv = CMD_FAIL;
            continue;
        }
        if ((rv = bcmx_port_ability_get(lport, &given.ability)) < 0) {
            soc_cm_print("port parse: Could not get port %s ability: %s\n",
                         bcmx_lport_to_uport_str(lport), bcm_errmsg(rv));
            cmd_rv = CMD_FAIL;
            continue;
        }
        if (SOC_IS_ROBO(unit)){
#if defined(BCM_ROBO_SUPPORT)
            rv = robo_port_parse_port_info_set(parsed, &given, info_p);
            if (BCM_FAILURE(rv)) {
                soc_cm_print("%s: Error: Could not parse port %s info: %s\n",
                             ARG_CMD(args), bcmx_lport_to_uport_str(lport),
                             bcm_errmsg(rv));
                cmd_rv = CMD_FAIL;
                continue;
            }
#endif
        } else {
#if defined(BCM_ESW_SUPPORT)
            rv = port_parse_port_info_set(parsed, &given, info_p);
            if (BCM_FAILURE(rv)) {
                soc_cm_print("%s: Error: Could not parse port %s info: %s\n",
                             ARG_CMD(args), bcmx_lport_to_uport_str(lport),
                             bcm_errmsg(rv));
                cmd_rv = CMD_FAIL;
                continue;
            }
#endif
        }    
        
        /* If AN is on, do not set speed, duplex, pause */
        if (info_p->autoneg) {
            info_p->action_mask &= ~(BCM_PORT_ATTR_SPEED_MASK |
                  BCM_PORT_ATTR_DUPLEX_MASK | BCM_PORT_ATTR_PAUSE_MASK);
        }

        rv = bcmx_port_selective_set(lport, info_p);
        if (BCM_FAILURE(rv)) {
            soc_cm_print("%s: ERROR: port info get for port %s returned %s\n",
                         ARG_CMD(args), bcmx_lport_to_uport_str(lport),
                         bcm_errmsg(rv));
            cmd_rv = CMD_FAIL;
            break;
        }
    }

done:
    bcmx_lplist_free(&ports);
    if (port_info) {
        sal_free(port_info);
    }
    return cmd_rv;
}

