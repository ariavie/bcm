/*
 * $Id: cmd_api.c 1.3 Broadcom SDK $
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
 * File:        cmd_api.c
 * Purpose:     API mode handler
 *
 */

#include "bcm/port.h"
#include "sal/core/alloc.h"
#include "sal/core/libc.h"
#include "sal/appl/io.h"
#include "shared/util.h"
#include "appl/diag/shell.h"
#include "appl/diag/system.h"
#include "appl/diag/parse.h"
#include "appl/diag/cmd_cint.h"
#include "sal/appl/editline/editline.h"
#include "shared/util.h"
#include "context.h"
#include "completion.h"
#include "api_mode.h"

#include <cint_porting.h>
#include <cint_internal.h>
#include <cint_ast.h>
#include <cint_eval_asts.h>
#include <cint_eval_ast_print.h>

#include <appl/diag/api_mode.h>

typedef enum api_mode_state_e {
    ApiModeOff,
    ApiModeOn,
    ApiModeOnly
} api_mode_state_t;

STATIC api_mode_state_t api_mode_state;

STATIC int
api_mode_initialize(void)
{
    /* init CINT */
    interp.extended_events = 1; /* enable extended event reporting */
    BCM_IF_ERROR_RETURN(cmd_cint_initialize());
    BCM_IF_ERROR_RETURN((api_mode_context_initialize()));
    BCM_IF_ERROR_RETURN((api_mode_completion_initialize()));

    return BCM_E_NONE;
}

STATIC int
api_mode_uninitialize(void)
{
    BCM_IF_ERROR_RETURN((api_mode_completion_uninitialize()));
    BCM_IF_ERROR_RETURN((api_mode_context_uninitialize()));

    return BCM_E_NONE;
}


char cmd_api_usage[] =
    "api [on|off|only]\n"
#ifndef COMPILER_STRING_CONST_LIMIT
    "\ton   - enable API mode, fall through to shell\n"
    "\toff  - disable API mode\n"
    "\tonly - enable API mode, do not fall though to shell\n"
#endif
    ;

/*
 * Function:    cmd_api
 * Purpose: Set shell mode to API
 * Returns: CMD_USAGE/CMD_FAIL/CMD_OK.
 */
cmd_result_t
cmd_api(int unit, args_t *args)
{
    cmd_result_t rv = CMD_FAIL;
    char *subcmd;

    if ((subcmd = ARG_GET(args)) == NULL) {
        sal_printf("API command mode is %s\n",
                   (api_mode_state != ApiModeOff) ? "on" : "off");
        rv = CMD_OK;
    } else if (sal_strcasecmp(subcmd, "ON") == 0) {
        if (BCM_SUCCESS(api_mode_initialize())) {
            api_mode_state = ApiModeOn;
            rv = CMD_OK;
        }
    } else if (sal_strcasecmp(subcmd, "OFF") == 0) {
        (void) api_mode_uninitialize();
        api_mode_state = ApiModeOff;
        rv = CMD_OK;
    } else if (sal_strcasecmp(subcmd, "ONLY") == 0) {
        if (BCM_SUCCESS(api_mode_initialize())) {
            api_mode_state = ApiModeOnly;
            rv = CMD_OK;
        }
    } else {
        sal_printf("API subcommand '%s' not found\n", subcmd);
        rv = CMD_USAGE;
    }
    return rv;
}

cmd_result_t
diag_api_mode_process_command(int u, char *s)
{
    int rv = API_MODE_E_FAIL;

    if (api_mode_state != ApiModeOff) {
        rv = api_mode_process_command(u, s);
        (void)var_set_integer("?", rv, TRUE, FALSE);
        if (api_mode_state == ApiModeOnly) {
            /* ignore returned rv so only API mode commands are run */
            if (API_MODE_FAILURE(rv)) {
                sal_printf("Error %d\n", rv);
            }
            rv = API_MODE_E_NONE;
        }
    }

    return (API_MODE_FAILURE(rv)) ? CMD_FAIL : CMD_OK;
}

void
diag_api_mode_completion_enable(int flag)
{
    if (api_mode_state != ApiModeOff) {
        if (flag) {
            (void)api_mode_completion_initialize();
        } else {
            (void)api_mode_completion_uninitialize();
        }
    }
}
