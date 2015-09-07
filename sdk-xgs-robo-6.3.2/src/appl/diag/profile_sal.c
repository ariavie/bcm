/*
 * $Id: profile_sal.c 1.9 Broadcom SDK $
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
 * File: 	profile_sal.c
 * Purpose: 	SAL resource usage profiler
 */

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE

#include <sal/types.h>
#include <shared/alloc.h>
#include <sal/core/sync.h>
#include <sal/core/thread.h>

#include <sal/appl/sal.h>

#include <appl/diag/shell.h>
#include <appl/diag/cmdlist.h>

char cmd_sal_profile_usage[] =
    "Parameters: None\n\t"
    "Display current SAL resource usage\n";

static void
_cmd_sal_profile_display(void)
{
    unsigned int res0_curr, res0_max;
    unsigned int res1_curr, res1_max;

    /*
     * appl/sal resource usage
     */
    sal_dma_alloc_resource_usage_get(&res0_curr, &res0_max);
    if (res0_max != 0) {
        printk("DMA Memory Allocation Current / Maximum   %d / %d Bytes\n",
                res0_curr,res0_max);
    }

    /*
     * core/sal resource usage
     */
    sal_alloc_resource_usage_get(&res0_curr, &res0_max);
    printk("CPU Memory Allocation Current / Maximum   %d / %d Bytes\n",
            res0_curr,res0_max);

    sal_sem_resource_usage_get(&res0_curr, &res0_max);
    if (res0_max != 0) {
        printk("Semaphore Count Current / Maximum         %d / %d\n",
                res0_curr,res0_max);
    }

    sal_mutex_resource_usage_get(&res0_curr, &res0_max);
    printk("Mutex Count Current / Maximum             %d / %d\n",
            res0_curr,res0_max);

    sal_thread_resource_usage_get(&res0_curr, &res1_curr, &res0_max, &res1_max);
    printk("Thread Count Current / Maximum            %d / %d\n",
            res0_curr,res0_max);

    if (res1_max != 0) {
        printk("Thread Stack Allocation Current / Maximum %d / %d Bytes\n",
                res1_curr,res1_max);
    }
}

cmd_result_t
cmd_sal_profile(int u, args_t *a)
/*
 * Function: 	cmd_sal_profile
 * Purpose:	Displays current SAL resource usage
 * Parameters:	None
 * Returns:	CMD_OK/CMD_USAGE
 */
{
    int  arg_count;

    COMPILER_REFERENCE(u);

    arg_count = ARG_CNT(a);

    if (arg_count != 0) {
        return(CMD_USAGE);
    } else {
        _cmd_sal_profile_display();
        return(CMD_OK);
    }
}
#endif
#endif
int _profile_sal_c_not_empty;
