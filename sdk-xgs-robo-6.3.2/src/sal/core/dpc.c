/*
 * $Id: dpc.c 1.8 Broadcom SDK $
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
 * File: 	dpc.c
 * Purpose: 	Deferred Procedure Call module
 *
 * sal_dpc_init() starts a thread to process deferred procedure calls.
 * sal_dpc_term() stops the thread.
 *
 * sal_dpc() enqueues a function that will be called by the thread ASAP.
 * sal_dpc_time() enqueues a function that will be called by the
 *		thread a specified amount of time in the future.
 */

#include <assert.h>

#include <sal/core/alloc.h>
#include <sal/core/spl.h>
#include <sal/core/sync.h>
#include <sal/core/time.h>
#include <sal/core/dpc.h>

#define	SAL_DPC_COUNT		256
#define SAL_DPC_THREAD_PRIO	50

typedef struct sal_dpc_s {
    struct sal_dpc_s	*sd_next;	/* Forward pointer */
    sal_usecs_t		sd_t;		/* Absolute time in usec */
    void		(*sd_f)(void *, void *, void *, void *, void *);
    void		*sd_owner;	/* First parameter passed to sd_f */
    void		*sd_p2;		/* Four more parameters passed */
    void		*sd_p3;
    void		*sd_p4;
    void		*sd_p5;
} sal_dpc_t;

static int		sal_dpc_count	= SAL_DPC_COUNT;
static int		sal_dpc_prio	= SAL_DPC_THREAD_PRIO;
static sal_sem_t	sal_dpc_sem	= NULL;	/* Semaphore to sleep on */
static sal_dpc_t	*sal_dpc_free	= NULL;	/* Free callout structs */
static sal_dpc_t	*sal_dpc_alloc	= NULL;	/* Original allocation */
static sal_dpc_t	*sal_dpc_q	= NULL;	/* Pending callouts */
static volatile sal_thread_t	sal_dpc_threadid = SAL_THREAD_ERROR;

/*
 * Function:
 * 	_sal_dpc_cleanup
 * Purpose:
 *	Free allocated DPC resources.
 * Parameters:
 *	None
 * Returns:
 *	Nothing.
 */

STATIC void
_sal_dpc_cleanup(void)
{
    if (sal_dpc_sem != NULL) {
	sal_sem_destroy(sal_dpc_sem);
	sal_dpc_sem = NULL;
    }

    if (sal_dpc_alloc != NULL) {
	sal_free(sal_dpc_alloc);
	sal_dpc_alloc = NULL;
    }
}

/*
 * Function:
 * 	sal_dpc_thread
 * Purpose:
 *	Background deferred procedure call thread used as context for DPCs.
 * Parameters:
 *	arg (Ignored)
 * Returns:
 *	Does not return
 */

STATIC void
sal_dpc_thread(void *arg)
{
    sal_dpc_t	*d;

    COMPILER_REFERENCE(arg);

    while (1) {
	int		il;
	sal_usecs_t	cur_time;

	/*
	 * Wait until it is time to do work, based on the top of the
	 * work queue.
	 */

	if (sal_dpc_q == NULL) {
	    (void)sal_sem_take(sal_dpc_sem, sal_sem_FOREVER);
	} else {
	    int t = SAL_USECS_SUB(sal_dpc_q->sd_t, sal_time_usecs());
	    if (t > 0) {
		(void)sal_sem_take(sal_dpc_sem, t);
	    }
	}

	/* Process all DPCs past due */

	cur_time = sal_time_usecs();

	il = sal_splhi();

	while ((d = sal_dpc_q) != NULL &&
	       SAL_USECS_SUB(d->sd_t, cur_time) <= 0) {
	    sal_dpc_q = d->sd_next;
	    sal_spl(il);
	    d->sd_f(d->sd_owner, d->sd_p2, d->sd_p3, d->sd_p4, d->sd_p5);
	    il = sal_splhi();
	    d->sd_next = sal_dpc_free;	/* Free queue entry */
	    sal_dpc_free = d;
	}

	sal_spl(il);
    }
}

/*
 * Function:
 *	sal_dpc_term
 * Purpose:
 *	Terminate DPC support.
 * Parameters:
 *	None
 * Returns:
 *	Nothing
 */

STATIC void
_sal_dpc_stop(void *owner, void *p2, void *p3, void *p4, void *p5)
{
    COMPILER_REFERENCE(owner);
    COMPILER_REFERENCE(p2);
    COMPILER_REFERENCE(p3);
    COMPILER_REFERENCE(p4);
    COMPILER_REFERENCE(p5);

    sal_dpc_threadid = SAL_THREAD_ERROR;
    sal_thread_exit(0);
}

void
sal_dpc_term(void)
{
    if (sal_dpc_threadid != SAL_THREAD_ERROR) {
	sal_dpc(_sal_dpc_stop, INT_TO_PTR(-1), 0, 0, 0, 0);

	sal_usleep(100);	/* tiny sleep first */
	while (sal_dpc_threadid != SAL_THREAD_ERROR) {
	    sal_usleep(10000);
	}
    }

    _sal_dpc_cleanup();

    sal_dpc_q = NULL;
}

/*
 * Function:
 * 	sal_dpc_init
 * Purpose:
 *	Initialize DPC support.
 * Parameters:
 *	None
 * Returns:
 *	0 on success, non-zero on failure
 * Notes:
 *	This must be called before other DPC routines can be used.
 *	It must not be called from interrupt context.
 */

int
sal_dpc_init(void)
{
    int		i;

    if (sal_dpc_threadid != SAL_THREAD_ERROR) {
	sal_dpc_term();
    }

    sal_dpc_sem = sal_sem_create("sal_dpc_sem", TRUE, 0);
    sal_dpc_alloc = sal_alloc(sizeof(sal_dpc_t) * sal_dpc_count, "sal_dpc");

    if (sal_dpc_sem == NULL ||
	sal_dpc_alloc == NULL) {
        _sal_dpc_cleanup();
	return -1;
    }

    sal_dpc_threadid =
	sal_thread_create("bcmDPC",
			  SAL_THREAD_STKSZ,
			  sal_dpc_prio,
			  sal_dpc_thread, 0);

    if (sal_dpc_threadid == SAL_THREAD_ERROR) {
        _sal_dpc_cleanup();
	return -1;
    }

    sal_dpc_free = sal_dpc_alloc;

    for (i = 0; i < sal_dpc_count - 1; i++) {
	sal_dpc_free[i].sd_next = &sal_dpc_free[i + 1];
    }

    sal_dpc_free[sal_dpc_count - 1].sd_next = NULL;

    return 0;
}

/*
 * Function:
 *	sal_dpc_config
 * Purpose:
 *	set configuration parameters and reinitialize dpc subsystem
 * Parameters:
 *	count	- number of entries (use default if <= 0)
 *	prio	- task priority (use defaults if <= 0)
 * Returns:
 *	0 on success, non-zero on failure
 */

int
sal_dpc_config(int count, int prio)
{
    if (count <= 0) {
	count = SAL_DPC_COUNT;
    }

    if (prio <= 0) {
	prio = SAL_DPC_THREAD_PRIO;
    }

    if (count == sal_dpc_count &&
	prio == sal_dpc_prio &&
	sal_dpc_threadid != SAL_THREAD_ERROR) {
	return 0;		/* already running with these parameters */
    }

    sal_dpc_count = count;
    sal_dpc_prio = prio;
    sal_dpc_term();

    return sal_dpc_init();
}

/*
 * Function:
 * 	sal_dpc_time
 * Purpose:
 *	Deferred procedure mechanism with timeout.
 * Parameters:
 *	usec - number of microseconds < 2^31 to wait before the function
 *		is called.  May be 0 to cause immediate callout.
 *	f  - function to call when timeout expires.
 *	owner, p2, p3, p4, p5 - parameters passed to callout function.
 * Returns:
 *	0 - Queued.
 *	-1 - failed.
 * Notes:
 *	May be called from interrupt context.
 */

int
sal_dpc_time(sal_usecs_t usec, sal_dpc_fn_t f,
	     void *owner, void *p2, void *p3, void *p4, void *p5)
{
    int		il;
    sal_dpc_t	*nt, *ct, *lt;		/* new/current/last timer */
    sal_usecs_t	now;
    int		need_to_give_sem;

    need_to_give_sem = 0;

    assert(sal_dpc_threadid != SAL_THREAD_ERROR);

    now = sal_time_usecs();

    il = sal_splhi();

    if ((nt = sal_dpc_free) != NULL) {
	sal_dpc_free = nt->sd_next;

	nt->sd_t = SAL_USECS_ADD(now, usec);
	nt->sd_f  = f;
	nt->sd_owner = owner;
	nt->sd_p2 = p2;
	nt->sd_p3 = p3;
	nt->sd_p4 = p4;
	nt->sd_p5 = p5;
	nt->sd_next = NULL;

	/*
	 * Find location in time queue and insert, note that this search and
	 * insert results in multiple entries for the same time being called
	 * in the order in which they were inserted.
	 */
	for (lt = NULL, ct = sal_dpc_q; ct; lt = ct, ct = ct->sd_next) {
	    if (SAL_USECS_SUB(nt->sd_t, ct->sd_t) < 0) {
		break;
	    }
	}

	/*
	 * lt is NULL if insert first entry, or points to entry after which
	 * we should insert.
	 */

	if (lt) {
	    nt->sd_next = lt->sd_next;
	    lt->sd_next = nt;
	} else {
	    nt->sd_next = sal_dpc_q;
	    sal_dpc_q	= nt;
	    need_to_give_sem = 1;
	}
    }

    sal_spl(il);

    if(need_to_give_sem) {
	sal_sem_give(sal_dpc_sem);	/* Wake up, old sleep value stale */
    }

    return (nt != NULL ? 0 : -1);
}

/*
 * Function:
 * 	sal_dpc
 * Purpose:
 *	Deferred procedure mechanism with-out timeout.
 * Parameters:
 *	f - function to call when timeout expires.
 *	owner, p2, p3, p4, p5 - parameters passed to callout function.
 * Returns:
 *	0 - Queued.
 *	-1 - failed.
 * Notes:
 *	May be called from interrupt context.
 */

int
sal_dpc(sal_dpc_fn_t f, void *owner, void *p2, void *p3, void *p4, void *p5)
{
    return (sal_dpc_time((sal_usecs_t) 0, f, owner, p2, p3, p4, p5));
}

/*
 * Function:
 *	sal_dpc_cancel
 *
 * Purpose:
 *	Cancel all DPCs belonging to a specified owner.
 *
 * Parameters:
 *	owner - first parameter to DPC calls
 *
 * Notes:
 *	Each DPC currently waiting on the queue whose first
 *	argument (owner) matches the specified value is removed
 *	from the queue and never executed.
 */
void
sal_dpc_cancel(void *owner)
{
    sal_dpc_t	**dp, *d;
    int		il;

    il = sal_splhi();

    dp = &sal_dpc_q;

    while ((d = *dp) != NULL) {
	if (d->sd_owner == owner) {
	    (*dp) = d->sd_next;
	    d->sd_next = sal_dpc_free;
	    sal_dpc_free = d;
	} else {
	    dp = &(*dp)->sd_next;
	}
    }

    sal_spl(il);
}
