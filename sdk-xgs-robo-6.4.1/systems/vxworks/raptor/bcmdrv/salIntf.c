/*
 * $Id: salIntf.c,v 1.4 Broadcom SDK $
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
 */
#include "vxWorks.h"
#include "sysLib.h"
#include "intLib.h"
#include "salIntf.h"
#include "semLib.h"
#include "assert.h"
#include "systemInit.h"
#include "signal.h"
#include "taskLib.h"
#include "cacheLib.h"

/*
 * Mutex and semaphore abstraction
 */

sal_mutex_t 
sal_mutex_create(char *desc) 
{
    SEM_ID      sem;

    assert(!INT_CONTEXT());

    sem = semMCreate(SEM_Q_PRIORITY |
        SEM_DELETE_SAFE |
        SEM_INVERSION_SAFE);

    return (sal_mutex_t) sem;
}


void
sal_mutex_destroy(sal_mutex_t m) 
{
    SEM_ID      sem = (SEM_ID) m;

    assert(!INT_CONTEXT());
    assert(sem);

    semDelete(sem);
}


int _sal_usec_to_ticks( uint32 usec );

int 
sal_mutex_take(sal_mutex_t m, int usec) 
{
    SEM_ID      sem = (SEM_ID) m;
    int         ticks;

    assert(!INT_CONTEXT());
    assert(sem);

    /*    ctrl_c_block();*/

    if (usec == (uint32) sal_mutex_FOREVER) {
        ticks = WAIT_FOREVER;
    }
    else {
        ticks = _sal_usec_to_ticks(usec);
    }

    if (semTake(sem, ticks) != OK) {
        /*ctrl_c_unblock();*/
        return -1;
    }

    return 0;
}


int 
sal_mutex_give(sal_mutex_t m) 
{
    SEM_ID      sem = (SEM_ID) m;
    int         err;

    assert(!INT_CONTEXT());

    err = semGive(sem);

    /*    ctrl_c_unblock();*/

    return (err == OK) ? 0 : -1;
}


static int _int_locked = 0;

int 
sal_spl(int level) 
{
    int il;
    _int_locked--;
#if VX_VERSION == 64
    intUnlock(level);
    il = 0;
#else
    il = intUnlock(level);
#endif
    return il;
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
    int il;
    il = intLock();
    _int_locked++;
    return (il);
}


#define DEFAULT_THREAD_OPTIONS VX_FP_TASK
#define SAL_THREAD_ERROR    ((sal_thread_t) -1)

sal_thread_t 
sal_thread_create(char *name, int ss, int prio, void (f)(void *), void *arg) 
{
    int     rv;
    sigset_t    new_mask, orig_mask;

#ifdef SAL_THREAD_PRIORITY
    prio = SAL_THREAD_PRIORITY;
#endif

    /* Make sure no child thread catches Control-C */

    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGINT);
    sigprocmask(SIG_BLOCK, &new_mask, &orig_mask);
    rv = taskSpawn(name, prio, DEFAULT_THREAD_OPTIONS, ss, (FUNCPTR)f,
        (uint32)(arg), 0, 0, 0, 0, 0, 0, 0, 0, 0);
    sigprocmask(SIG_SETMASK, &orig_mask, NULL);

    if (rv == ERROR) {
        return (SAL_THREAD_ERROR);
    }
    else {
        return ((sal_thread_t) (rv));
    }
}


int 
sal_thread_destroy(sal_thread_t thread) 
{
    if (taskDelete((uint32)(thread)) == ERROR) {
        return -1;
    }

    return 0;
}


sal_sem_t
sal_sem_create(char *desc, int binary, int initial_count) 
{
    SEM_ID      b;

    assert(!INT_CONTEXT());

    if (binary) {
        b = semBCreate(SEM_Q_FIFO, initial_count);
    }
    else {
        b = semCCreate(SEM_Q_FIFO, initial_count);
    }

    return (sal_sem_t) b;
}


void
sal_sem_destroy(sal_sem_t b) 
{
    SEM_ID      sem = (SEM_ID) b;

    assert(sem);
    semDelete(sem);
}


int
sal_sem_take(sal_sem_t b, int usec) 
{
    SEM_ID      sem = (SEM_ID) b;
    int         ticks;

    assert(!INT_CONTEXT());
    assert(sem);

    if (usec == (uint32) sal_sem_FOREVER) {
        ticks = WAIT_FOREVER;
    }
    else {
        ticks = _sal_usec_to_ticks(usec);
    }

    return (semTake(sem, ticks) != OK) ? -1 : 0;
}


int
sal_sem_give(sal_sem_t b) 
{
    SEM_ID      sem = (SEM_ID) b;

    assert(sem);

    return (semGive(sem) != OK) ? -1 : 0;
}


void 
sal_thread_exit(int rc) 
{
    exit(rc);
}


void 
sal_dma_flush(void *addr, int len) 
{
    CACHE_DRV_FLUSH(&cacheDmaFuncs, addr, len);
}


void 
sal_dma_inval(void *addr, int len) 
{
    CACHE_DRV_INVALIDATE(&cacheDmaFuncs, addr, len);
}
