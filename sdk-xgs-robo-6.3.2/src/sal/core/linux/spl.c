/*
 * $Id: spl.c 1.13 Broadcom SDK $
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
 * Purpose:	Multi-processor safe recursive global lock
 */

#include <sal/core/spl.h>
#include "lkm.h"
#include <asm/hardirq.h>
#include <assert.h>
#include <linux/spinlock.h>
#include <asm/cache.h>

#ifdef LKM_2_6
#include <linux/preempt.h>
#else
#define preempt_disable()               do { } while (0)
#define preempt_enable()                do { } while (0)
#endif

/* The lock data structure is placed in it's own cacheline to prevent
   false sharing. */
 
struct __rec_spin_lock {
    spinlock_t spl_lock;
    unsigned long flags;
    struct task_struct *thread;
    int depth;
};

static struct _rec_spin_lock {
    spinlock_t spl_lock;
    unsigned long flags;
    struct task_struct *thread;
    int depth;
    char pad[ ((sizeof(struct __rec_spin_lock)+ L1_CACHE_BYTES-1) & ~(L1_CACHE_BYTES-1)) - sizeof(struct __rec_spin_lock)];
} __attribute__((aligned(L1_CACHE_BYTES))) rec_spin_lock = { __SPIN_LOCK_UNLOCKED(rec_spin_lock.spl_lock),0,NULL,0 };

/*
 * Function:
 *	sal_spl
 * Purpose:
 *	Set lock level. (MP safe)
 * Parameters:
 *	level - lock level to set.
 * Returns: always 0
 */

int
sal_spl(int level)
{
    assert (rec_spin_lock.thread == current);
    assert (rec_spin_lock.depth == level);

    if (likely(--rec_spin_lock.depth == 0)) {
        rec_spin_lock.thread = NULL;
        smp_wmb(); /* required on some systems */
        spin_unlock_irqrestore(&rec_spin_lock.spl_lock,rec_spin_lock.flags);
        preempt_enable();
    }
    return 0;
}

/*
 * Function:
 *	sal_splhi
 * Purpose:
 *	Increase the lock level. (MP safe)
 * Parameters:
 *	None
 * Returns:
 *	Value of new lock level.
 * Notes:
 *      Once the lock is taken interrupts are disabled and preemption
 *      cannot happen. This means only the thread holding the lock  
 *	can change rec_spin_lock.thread and rec_spin_lock.depth. 
 *      I.e. no mem barriers are required either.
 */

int
sal_splhi(void)
{
    if (likely(rec_spin_lock.thread != current)) {
        unsigned long flags;
        preempt_disable();
        spin_lock_irqsave(&rec_spin_lock.spl_lock,flags); 
        assert(rec_spin_lock.depth == 0);
        rec_spin_lock.flags = flags;
        rec_spin_lock.thread = current;
    }
    return ++rec_spin_lock.depth;
}

/*
 * Function:
 *	sal_int_locked
 * Purpose:
 *	Return TRUE if the global lock is taken.
 * Parameters:
 *	None
 * Returns:
 *	Boolean
 */

int
sal_int_locked(void)
{
    return rec_spin_lock.depth;
}

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
    return in_interrupt();
}
