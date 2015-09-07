/*
 * $Id: pscan.c,v 1.2 Broadcom SDK $
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
 * File: 	mcs.c
 * Purpose: 	Micro Controller Subsystem Support
 */

#include <assert.h>
#include <sal/core/libc.h>
#include <bcm/error.h>
#include <soc/pscan.h>
#include <soc/types.h>
#include <soc/dma.h>
#include <appl/diag/shell.h>
#include <appl/diag/system.h>

#if defined(BCM_CMICM_SUPPORT)

#include <soc/uc_msg.h>

char pscan_usage[] =
    "Parameters: \n\t"
    "Control the pscan on the Uc.\n\t"
    "init\n\t"
    "delay <usec>\n\t"
    "enable <port>\n\t"
    "disable <port>\n\t"
    "config <port> <flags>\n"
    "update\n\t"
    ;

cmd_result_t
pscan_cmd(int unit, args_t *a)
/*
 * Function: 	pscan_cmd
 * Purpose:	Control uKernel pscan interface
 * Parameters:	unit - unit
 *		a - args
 * Returns:	CMD_OK/CMD_FAIL/CMD_USAGE
 */
{
    cmd_result_t rv = CMD_OK;
    char *c;
    int count, delay, port, flags;

#ifndef NO_CTRL_C
    jmp_buf ctrl_c;
#endif    

    if (!sh_check_attached("pscan", unit)) {
        return(CMD_FAIL);
    }

    if (!soc_feature(unit, soc_feature_uc)) {
        return (CMD_FAIL);
    } 

    if (ARG_CNT(a) < 1) {
        return(CMD_USAGE);
    }

    /* check for simulation*/
    if (SAL_BOOT_BCMSIM) {
        return(rv);
    }

#ifndef NO_CTRL_C    
    if (!setjmp(ctrl_c)) {
        sh_push_ctrl_c(&ctrl_c);
#endif
        c = ARG_GET(a);
        count = ARG_CNT(a);
        if ((count == 0) && !sal_strcmp(c, "init")) {
            rv = soc_pscan_init(unit);
        }
        else if ((count == 2) && !sal_strcmp(c, "config")) {
            c = ARG_GET(a);
            port = parse_integer(c);
            c = ARG_GET(a);
            flags = parse_integer(c);
            rv = soc_pscan_port_config(unit, port, flags);
        }
        else if ((count == 1) && !sal_strcmp(c, "delay")) {
            c = ARG_GET(a);
            delay = parse_integer(c);
            rv = soc_pscan_delay(unit, delay);
        }
        else if ((count == 1) && !sal_strcmp(c, "disable")) {
            c = ARG_GET(a);
            port = parse_integer(c);
            rv = soc_pscan_port_enable(unit, port, 0);
        }
        else if ((count == 1) && !sal_strcmp(c, "enable")) {
            c = ARG_GET(a);
            port = parse_integer(c);
            rv = soc_pscan_port_enable(unit, port, 1);
        }
        else if ((count == 0) && !sal_strcmp(c, "update")) {
            rv = soc_pscan_update(unit);
        }
        else {
            rv = CMD_USAGE;
        }
#ifndef NO_CTRL_C    
    } else {
        rv = CMD_INTR;
    }

    sh_pop_ctrl_c();
#endif    

    return(rv);
}

#endif /* CMICM support */

