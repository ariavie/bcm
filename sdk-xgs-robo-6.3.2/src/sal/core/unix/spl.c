/*
 * $Id: spl.c 1.10 Broadcom SDK $
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
 * File: 	spl.c
 * Purpose:	Interrupt Blocking
 */

#include <sys/types.h>
#include <signal.h>

#include <assert.h>
#include <sal/core/sync.h>

#ifdef SAL_SPL_NO_PREEMPT
#include <pthread.h>
#endif

static sal_mutex_t	spl_mutex;
static int		spl_level;

/*
 * Function:
 *	sal_spl_init
 * Purpose:
 *	Initialize the synchronization portion of the SAL
 * Returns:
 *	0 on success, -1 on failure
 */

int
sal_spl_init(void)
{
    /* Initialize spl */

    spl_mutex = sal_mutex_create("spl mutex");
    spl_level = 0;

    return 0;
}

/*
 * Function:
 *	sal_spl
 * Purpose:
 *	Set interrupt level.
 * Parameters:
 *	level - interrupt level to set.
 * Returns:
 *	Value of interrupt level in effect prior to the call.
 * Notes:
 *	Used most often to restore interrupts blocked by sal_splhi.
 */

int
sal_spl(int level)
{
#ifdef SAL_SPL_NO_PREEMPT
    struct sched_param param;
    int policy;

    if (pthread_getschedparam(pthread_self(), &policy, &param) == 0) {
        /* Interrupt thread uses SCHED_RR and should be left alone */
        if (policy != SCHED_RR) {
            /* setting sched_priority to 0 will only change the real time priority
               of the thread. For a non real time thread it should be 0. This is
               very Linux specific.
             */  
            param.sched_priority = 0;
            pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
        }
    }
#endif
    assert(level == spl_level);
    spl_level--;
    sal_mutex_give(spl_mutex);
    return 0;
}

/*
 * Function:
 *	sal_splhi
 * Purpose:
 *	Set interrupt mask to highest level.
 * Parameters:
 *	None
 * Returns:
 *	Value of interrupt level in effect prior to the call.
 */

int
sal_splhi(void)
{
#ifdef SAL_SPL_NO_PREEMPT
    struct sched_param param;
    int policy;

    if (pthread_getschedparam(pthread_self(), &policy, &param) == 0) {
        /* Interrupt thread uses SCHED_RR and should be left alone */
        if (policy != SCHED_RR) {
            param.sched_priority = 90;
            pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
        }
    }
#endif
    sal_mutex_take(spl_mutex, sal_mutex_FOREVER);
    return ++spl_level;
}

#if defined(IPROC_CMICD)
/* Must be provided by link check with undefined reference for Northstar's compiler */
static int
intr_int_context(void)
{
    return 0;
}
#endif

/*
 * Function:
 *	sal_int_context
 * Purpose:
 *	Return TRUE if running in interrupt context.
 * Parameters:
 *	None
 * Returns:
 *	Boolean
 */

int
sal_int_context(void)
{
    /* Must be provided by the virtual interrupt controller */
    extern int intr_int_context(); 

    return intr_int_context();
}
