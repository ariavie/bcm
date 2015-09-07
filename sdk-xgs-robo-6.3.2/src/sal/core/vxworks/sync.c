/*
 * $Id: sync.c 1.11 Broadcom SDK $
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
 * File: 	sync.c
 * Purpose:	Defines SAL routines for mutexes and semaphores
 *
 * Mutex and Binary Semaphore abstraction
 */

#include <sys/types.h>
#include <sal/types.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include <vxWorks.h>
#include <semLib.h>
#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
#include <intLib.h>
#endif
#endif

#include <assert.h>
#include <sal/core/sync.h>
#include <sal/core/spl.h>
#include <sal/core/thread.h>

extern int _sal_usec_to_ticks(uint32 usec);	/* time.c */

#define SAL_INT_CONTEXT()	sal_int_context()

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
static unsigned int _sal_sem_count_curr;
static unsigned int _sal_sem_count_max;
static unsigned int _sal_mutex_count_curr;
static unsigned int _sal_mutex_count_max;
#define SAL_SEM_RESOURCE_USAGE_INCR(a_curr, a_max, ilock)               \
        ilock = intLock();                                              \
        a_curr++;                                                       \
        a_max = ((a_curr) > (a_max)) ? (a_curr) : (a_max);              \
        intUnlock(ilock)
    
#define SAL_SEM_RESOURCE_USAGE_DECR(a_curr, ilock)                      \
        ilock = intLock();                                              \
        a_curr--;                                                       \
        intUnlock(ilock)

/*
 * Function:
 *      sal_sem_resource_usage_get
 * Purpose:
 *      Provides count of active sem and maximum sem allocation
 * Parameters:
 *      sem_curr - Current semaphore allocation.
 *      sem_max - Maximum semaphore allocation.
 */

void
sal_sem_resource_usage_get(unsigned int *sem_curr, unsigned int *sem_max)
{
    if (sem_curr != NULL) {
        *sem_curr = _sal_sem_count_curr;
    }
    if (sem_max != NULL) {
        *sem_max = _sal_sem_count_max;
    }
}

/*
 * Function:
 *      sal_mutex_resource_usage_get
 * Purpose:
 *      Provides count of active mutex and maximum mutex allocation
 * Parameters:
 *      mutex_curr - Current mutex allocation.
 *      mutex_max - Maximum mutex allocation.
 */

void
sal_mutex_resource_usage_get(unsigned int *mutex_curr, unsigned int *mutex_max)
{
    if (mutex_curr != NULL) {
        *mutex_curr = _sal_mutex_count_curr;
    }
    if (mutex_max != NULL) {
        *mutex_max = _sal_mutex_count_max;
    }
}
#endif
#endif

/*
 * Keyboard interrupt protection
 *
 *   When a thread is running on a console, the user could Control-C
 *   while a mutex is held by the thread.  Control-C results in a signal
 *   that longjmp's somewhere else.  We prevent this from happening by
 *   blocking Control-C signals while any mutex is held.
 */

#ifndef NO_CONTROL_C
static int ctrl_c_depth = 0;
#endif

static void
ctrl_c_block(void)
{
    assert(!SAL_INT_CONTEXT());

#ifndef NO_CONTROL_C
    if (sal_thread_self() == sal_thread_main_get()) {
	if (ctrl_c_depth++ == 0) {
	    sigset_t set;
	    sigemptyset(&set);
	    sigaddset(&set, SIGINT);
	    sigprocmask(SIG_BLOCK, &set, NULL);
	}
    }
#endif
}

static void
ctrl_c_unblock(void)
{
    assert(!SAL_INT_CONTEXT());

#ifndef NO_CONTROL_C
    if (sal_thread_self() == sal_thread_main_get()) {
	assert(ctrl_c_depth > 0);
	if (--ctrl_c_depth == 0) {
	    sigset_t set;
	    sigemptyset(&set);
	    sigaddset(&set, SIGINT);
	    sigprocmask(SIG_UNBLOCK, &set, NULL);
	}
    }
#endif
}

/*
 * Mutex and semaphore abstraction
 */

sal_mutex_t
sal_mutex_create(char *desc)
{
#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
    int         ilock;
#endif
#endif

    SEM_ID		sem;

    assert(!SAL_INT_CONTEXT());

    sem = semMCreate(SEM_Q_PRIORITY |
		     SEM_DELETE_SAFE |
		     SEM_INVERSION_SAFE);

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
        SAL_SEM_RESOURCE_USAGE_INCR(
            _sal_mutex_count_curr,
            _sal_mutex_count_max,
            ilock);
#endif
#endif
    return (sal_mutex_t) sem;
}

void
sal_mutex_destroy(sal_mutex_t m)
{
    SEM_ID		sem = (SEM_ID) m;
#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
    int         ilock;
#endif
#endif

    assert(!SAL_INT_CONTEXT());
    assert(sem);

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
        SAL_SEM_RESOURCE_USAGE_DECR(
            _sal_mutex_count_curr,
            ilock);
#endif
#endif

    semDelete(sem);
}

int
sal_mutex_take(sal_mutex_t m, int usec)
{
    SEM_ID		sem = (SEM_ID) m;
    int			ticks;

    assert(!SAL_INT_CONTEXT());
    assert(sem);

    ctrl_c_block();

    if (usec == sal_mutex_FOREVER) {
	ticks = WAIT_FOREVER;
    } else {
	ticks = _sal_usec_to_ticks(usec);
    }

    if (semTake(sem, ticks) != OK) {
	ctrl_c_unblock();
	return -1;
    }

    return 0;
}

int
sal_mutex_give(sal_mutex_t m)
{
    SEM_ID		sem = (SEM_ID) m;
    int			err;

    assert(!SAL_INT_CONTEXT());

    err = semGive(sem);

    ctrl_c_unblock();

    return (err == OK) ? 0 : -1;
}

sal_sem_t
sal_sem_create(char *desc, int binary, int initial_count)
{
    SEM_ID		b;
#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
    int         ilock;
#endif
#endif

    assert(!SAL_INT_CONTEXT());

    if (binary) {
	b = semBCreate(SEM_Q_FIFO, initial_count);
    } else {
	b = semCCreate(SEM_Q_FIFO, initial_count);
    }

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
        SAL_SEM_RESOURCE_USAGE_INCR(
            _sal_sem_count_curr,
            _sal_sem_count_max,
            ilock);
#endif
#endif

    return (sal_sem_t) b;
}

void
sal_sem_destroy(sal_sem_t b)
{
    SEM_ID		sem = (SEM_ID) b;
#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
    int         ilock;
#endif
#endif

    assert(sem);
    semDelete(sem);

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
        SAL_SEM_RESOURCE_USAGE_DECR(
            _sal_sem_count_curr,
            ilock);
#endif
#endif
}

int
sal_sem_take(sal_sem_t b, int usec)
{
    SEM_ID		sem = (SEM_ID) b;
    int			ticks;

    assert(!SAL_INT_CONTEXT());
    assert(sem);

    if (usec == sal_sem_FOREVER) {
	ticks = WAIT_FOREVER;
    } else {
	ticks = _sal_usec_to_ticks(usec);
    }

    return (semTake(sem, ticks) != OK) ? -1 : 0;
}

int
sal_sem_give(sal_sem_t b)
{
    SEM_ID		sem = (SEM_ID) b;

    assert(sem);

    return (semGive(sem) != OK) ? -1 : 0;
}
