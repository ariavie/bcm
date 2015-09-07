/*
 * $Id: sync.c 1.27 Broadcom SDK $
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

#include <sal/core/sync.h>
#include <sal/core/thread.h>
#include <sal/core/alloc.h>
#include <sal/core/time.h>

#include "lkm.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif

#include <linux/interrupt.h>
#include <linux/sched.h>

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
static unsigned int _sal_sem_count_curr;
static unsigned int _sal_sem_count_max;
static unsigned int _sal_mutex_count_curr;
static unsigned int _sal_mutex_count_max;
#define SAL_SEM_RESOURCE_USAGE_INCR(a_curr, a_max, ilock)               \
        a_curr++;                                                       \
        a_max = ((a_curr) > (a_max)) ? (a_curr) : (a_max)
    
#define SAL_SEM_RESOURCE_USAGE_DECR(a_curr, ilock)                      \
        a_curr--

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
        *sem_curr = _sal_sem_count_curr - _sal_mutex_count_curr;
    }
    if (sem_max != NULL) {
        *sem_max = _sal_sem_count_max - _sal_mutex_count_max;
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

#define USECS_PER_JIFFY (SECOND_USEC / HZ)
#define USEC_TO_JIFFIES(usec) ((usec + (USECS_PER_JIFFY - 1)) / USECS_PER_JIFFY)


/* See thread.c for details */
static INLINE void
thread_check_signals(void)
{
    if(signal_pending(current)) {
        sal_thread_exit(0);
    }
}

/* Emulate user mode system calls */
static INLINE void
sal_ret_from_syscall(void)
{
    if (!in_interrupt()) {
        schedule();
        thread_check_signals();
    }
}

/*
 * recursive_mutex_t
 *
 *   This is an abstract type built on the POSIX mutex that allows a
 *   mutex to be taken recursively by the same thread without deadlock.
 */

typedef struct recursive_mutex_s {
    sal_sem_t		sem;
    sal_thread_t	owner;
    int			recurse_count;
    char		*desc;
} recursive_mutex_t;

sal_mutex_t
sal_mutex_create(char *desc)
{
    recursive_mutex_t	*rm;

    if ((rm = sal_alloc(sizeof (recursive_mutex_t), desc)) == NULL) {
	return NULL;
    }

    rm->sem = sal_sem_create(desc, 1, 1);
    rm->owner = 0;
    rm->recurse_count = 0;
    rm->desc = desc;

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
        SAL_SEM_RESOURCE_USAGE_INCR(
            _sal_mutex_count_curr,
            _sal_mutex_count_max,
            ilock);
#endif
#endif

    return (sal_mutex_t) rm;
}

void
sal_mutex_destroy(sal_mutex_t m)
{
    recursive_mutex_t	*rm = (recursive_mutex_t *) m;

    sal_sem_destroy(rm->sem);

    sal_free(rm);
#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
        SAL_SEM_RESOURCE_USAGE_DECR(
            _sal_mutex_count_curr,
            ilock);
#endif
#endif
}

int
#ifdef BROADCOM_DEBUG_MUTEX
sal_mutex_take_intern(sal_mutex_t m, int usec)
#else
sal_mutex_take(sal_mutex_t m, int usec)
#endif
{
    recursive_mutex_t	*rm = (recursive_mutex_t *) m;
    sal_thread_t	myself = sal_thread_self();
    int			err;

    if (rm->owner == myself) {
	rm->recurse_count++;
        sal_ret_from_syscall();
	return 0;
    }

    err = sal_sem_take(rm->sem, usec);

    if (err) {
	return -1;
    }

    rm->owner = myself;

    return 0;
}

int
#ifdef BROADCOM_DEBUG_MUTEX
sal_mutex_give_intern(sal_mutex_t m)
#else
sal_mutex_give(sal_mutex_t m)
#endif
{
    recursive_mutex_t	*rm = (recursive_mutex_t *) m;
    int			err;

    if (rm->recurse_count > 0) {
	rm->recurse_count--;
        sal_ret_from_syscall();
	return 0;
    }

    rm->owner = 0;

    err = sal_sem_give(rm->sem);

    return err ? -1 : 0;
}

/*
 * sem_ctrl_t
 *
 *   The semaphore control type uses the binary property to implement
 *   timed semaphores with improved performance using wait queues.
 */

typedef struct sem_ctrl_s {
    struct semaphore    sem;
    int                 binary;
    int                 cnt;
    wait_queue_head_t   wq;
    atomic_t            wq_active;
} sem_ctrl_t;

sal_sem_t
sal_sem_create(char *desc, int binary, int initial_count)
{
    sem_ctrl_t *s;

    if ((s = sal_alloc(sizeof(*s), desc)) != 0) {
	sema_init(&s->sem, initial_count);
        s->binary = binary;
        if (s->binary) {
            init_waitqueue_head(&s->wq);
        }
        atomic_set(&s->wq_active, 0);
    }

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
        SAL_SEM_RESOURCE_USAGE_INCR(
            _sal_sem_count_curr,
            _sal_sem_count_max,
            ilock);
#endif
#endif

    return (sal_sem_t) s;
}

void
sal_sem_destroy(sal_sem_t b)
{
    sem_ctrl_t *s = (sem_ctrl_t *) b;

    if (s == NULL) {
	return;
    }

    /*
     * the linux kernel does not have a sema_destroy(s)
     */
    sal_free(s);

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
        SAL_SEM_RESOURCE_USAGE_DECR(
            _sal_sem_count_curr,
            ilock);
#endif
#endif
}

static int
_sal_sem_take(sal_sem_t b, int usec)
{
    sem_ctrl_t *s = (sem_ctrl_t *) b;
    int			err;

    if (usec == sal_sem_FOREVER && !in_interrupt()) {
	err = down_interruptible(&s->sem);
    } else {
	int		time_wait = 1;
        int             cnt = s->cnt;

	for (;;) {
	    if (down_trylock(&s->sem) == 0) {
		err = 0;
		break;
	    }

            if (s->binary) {

                /* Wait for event or timeout */

                if (time_wait > 1) {
                    err = -ETIMEDOUT;
                    break;
                }

                atomic_inc(&s->wq_active);

                err = wait_event_interruptible_timeout(s->wq, cnt != s->cnt, 
                                                       USEC_TO_JIFFIES(usec));
                atomic_dec(&s->wq_active);

                if (err < 0) {
                    break;
                }
                time_wait++;

            } else {

                /* Retry algorithm with exponential backoff */

                if (time_wait > usec) {
                    time_wait = usec;
                }

                sal_usleep(time_wait);
                
                usec -= time_wait;
            
                if (usec == 0) {
                    err = -ETIMEDOUT;
                    break;
                }

                if ((time_wait *= 2) > 100000) {
                    time_wait = 100000;
                }
	    }
	}
    }

    sal_ret_from_syscall();

    return err;
}

int
sal_sem_take(sal_sem_t b, int usec)
{
    int err = 0;
    int retry;

    do {
        retry = 0;
        err = _sal_sem_take(b, usec);
        /* Retry if we got interrupted prematurely */
        if (err == -ERESTARTSYS || err == -EINTR) {
            retry = 1;
#ifdef LINUX_SAL_SEM_OVERRIDE
            /* Retry done from user-mode */
            return -2;
#endif
        }
    } while(retry);

    return err ? -1 : 0;
}

int
sal_sem_give(sal_sem_t b)
{
    sem_ctrl_t *s = (sem_ctrl_t *) b;
    int wq_active = atomic_read(&s->wq_active);

    if (s->binary) {
        if (down_trylock(&s->sem));
        up(&s->sem);
        /*
         * At this point a binary semaphore may already have
         * been destroyed, so we need to make sure that we
         * do not access the semaphore anymore even if the
         * wake_up call is otherwise harmless.
         */
        if (wq_active) {
            s->cnt++;
            wake_up_interruptible(&s->wq);
        }
    } else {
        up(&s->sem);
     }
    sal_ret_from_syscall();
    return 0;
}
