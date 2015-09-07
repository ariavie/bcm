/*
 * $Id: shell.h,v 1.20 Broadcom SDK $
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
 * File:    sh.h
 * Purpose:    Types and structures for command table entires and
 *        command return values for cshell commands.
 */

#ifndef _DIAG_SH_H
#define _DIAG_SH_H

#if defined(VXWORKS)
#   include <vxWorks.h>
#endif
#ifdef __KERNEL__
/* Need jmp_buf to satisfy prototypes */
#define jmp_buf int
#else
#include <setjmp.h>
#endif
#if !defined(SWIG)
#include <sal/appl/io.h>
#endif
#include <appl/diag/parse.h>

/*
 * Define:    RCLOAD_DEPTH_MAX
 * Purpose:    Maximum depth of rcload files.
 */
#define RCLOAD_DEPTH_MAX    32

/*
 * Define:    LOG_DEFAULT_FILE
 * Purpose:    Default file name for "log" command
 */
#define LOG_DEFAULT_FILE    "bcm.log"

/*
 * Define:    PUSH_CTRL_C_CNT
 * Purpose:    Defines the maximum number of pushed Control-C handlers.
 * Notes:    Must be more than RCLOAD_DEPTH_MAX.
 */
#define    PUSH_CTRL_C_CNT    (RCLOAD_DEPTH_MAX + 4)

/*
 * Typedef:     cmd_result_t
 * Purpose:    Type retured from all commands indicating success, fail, 
 *        or print usage.
 */
typedef enum cmd_result_e {
    CMD_OK   = 0,            /* Command completed successfully */
    CMD_FAIL = -1,            /* Command failed */
    CMD_USAGE= -2,            /* Command failed, print usage  */
    CMD_NFND = -3,            /* Command not found */
    CMD_EXIT = -4,            /* Exit current shell level */
    CMD_INTR = -5,            /* Command interrupted */
    CMD_NOTIMPL = -6            /* Command not implemented */
} cmd_result_t;

/*
 * Typedef:    cmd_func_t
 * Purpose:    Defines command function type
 */
typedef cmd_result_t (*cmd_func_t)(int, args_t *);

/*
 * Typedef:    cmd_t
 * Purpose:    Table command match structure.
 */
typedef struct cmd_s {
    parse_key_t    c_cmd;            /* Command string */
    cmd_func_t    c_f;            /* Function to call */
    const char     *c_usage;        /* Usage string */
    const char    *c_help;        /* Help string */
} cmd_t;

/*
 * Shell rc file load depth
 */
extern int sh_rcload_depth;


/*
 * Shell 'set' variables
 */

extern int    sh_set_rcload;
extern int    sh_set_rcerror;
extern int    sh_set_lperror;
extern int    sh_set_iferror;
extern int    sh_set_rctest;
extern int    sh_set_more_lines;
extern int    sh_set_report_time;
extern int    sh_set_report_status;


/*
 * Exported shell.c functions.
 */

cmd_result_t     sh_process(int unit, const char *, int eof);
cmd_result_t    sh_process_command(int unit, char *c);
cmd_result_t    sh_process_command_check(int unit, char *c);
int        sh_check_attached(const char *pfx, const int unit);
cmd_result_t    sh_rcload_file(int u, args_t *, char *f, int add_rc);
void        sh_swap_unit_vars(int new_unit);

/*
 * Functions for pushing/poping where to longjump on control-c.
 */
extern void    sh_push_ctrl_c(jmp_buf *);
extern void    sh_pop_ctrl_c(void);
extern void    sh_block_ctrl_c(int);
extern void    sh_ctrl_c_take(void);
extern void    sh_print_version(int verbose);

extern void    sh_bg_init(void);

#endif /* _DIAG_SH_H */
