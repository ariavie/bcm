/*
 * $Id: f256bc4a93d846120a45f550e4efbb0ecc5fd320 $
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
 * Profile Stats commands
 */
#include <sal/types.h>
#include <sal/core/libc.h>
#include <appl/diag/shell.h>
#include <soc/cmdebug.h>
#include <shared/shr_bprof.h>
#include <shared/bsl.h>
#include <bcm/error.h>

#ifdef BCM_BPROF_STATS
void
sh_bprof_stats(int verbose)
/*
 * Function:     sh_prof_stats
 * Purpose:    Prints profile statistics.
 * Parameters:    verbose - includes more info if TRUE
 * Returns:    Nothing
 */
{
    int i = 0;
    shr_bprof_stats_entry_t bprof_stats;

    cli_out("\tOP name   \tTime(usec):Total \t Avg \t   Max \t    Min \t    Runs \n");
    for (i = 0; i < SHR_BPROF_STATS_MAX; i++) {
        if (BCM_E_NONE == shr_bprof_stats_get(i, &bprof_stats)) {
            if (bprof_stats.runs == 1) {
                cli_out("%30s = %10d %10d %10d %10d %10d \n", 
                        shr_bprof_stats_name(i), 
                        bprof_stats.total_stime,
                        0,
                        0,
                        0,
                        bprof_stats.runs);
            } else if (bprof_stats.runs > 1) {
                cli_out("%30s = %10d %10d %10d %10d %10d \n", 
                        shr_bprof_stats_name(i), 
                        bprof_stats.total_stime,
                        (bprof_stats.total_stime/bprof_stats.runs),
                        bprof_stats.max_stime,
                        bprof_stats.min_stime,
                        bprof_stats.runs);
            }
        }
    }
    cli_out("Total boot time = %10d\n", shr_bprof_stats_time_taken());

    return;
}

char sh_prof_usage[] =
    "Parameters: none\n\t"
    "Prints current profile stats\n";

/*
 * Function:     sh_prof
 * Purpose:    Print prof stats
 * Parameters:    u - Unit number (ignored)
 *        a - pointer to arguments.
 * Returns:    CMD_USAGE/CMD_OK
 */
cmd_result_t
sh_prof(int u, args_t *a)
{
    COMPILER_REFERENCE(u);

    if (ARG_CNT(a)) {
    return(CMD_USAGE);
    }
    sh_bprof_stats(TRUE);
    return(CMD_OK);
}
#endif /* BCM_BPROF_STATS */
