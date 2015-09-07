/*
 * $Id: mpool.c 1.19 Broadcom SDK $
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

#include <mpool.h>

#ifdef __KERNEL__

/* 
 * Abstractions used when compiling for Linux kernel mode. 
 */

#include <lkm.h>

/*
 * We cannot use the linux kernel SAL for MALLOC/FREE because 
 * the current implementation of sal_alloc() allocates memory 
 * out of an mpool created by this module...
 */
#define MALLOC(x) kmalloc(x, GFP_ATOMIC)
#define FREE(x) kfree(x)

static spinlock_t _mpool_lock;
#define MPOOL_LOCK_INIT() spin_lock_init(&_mpool_lock)
#define MPOOL_LOCK() unsigned long flags; spin_lock_irqsave(&_mpool_lock, flags)
#define MPOOL_UNLOCK() spin_unlock_irqrestore(&_mpool_lock, flags)

#else /* !__KERNEL__*/

/* 
 * Abstractions used when compiling for Linux user mode. 
 */

#include <stdlib.h>
#include <sal/core/sync.h>

#define MALLOC(x) malloc(x)
#define FREE(x) free(x)

static sal_sem_t _mpool_lock;
#define MPOOL_LOCK_INIT() _mpool_lock = sal_sem_create("mpool_lock", 1, 1)
#define MPOOL_LOCK() sal_sem_take(_mpool_lock, sal_sem_FOREVER)
#define MPOOL_UNLOCK() sal_sem_give(_mpool_lock)

#endif /* __KERNEL__ */

/* Allow external override for system cache line size */
#ifndef BCM_CACHE_LINE_BYTES
#ifdef L1_CACHE_BYTES
#define BCM_CACHE_LINE_BYTES L1_CACHE_BYTES
#else
#define BCM_CACHE_LINE_BYTES 128 /* Should be fine on most platforms */
#endif
#endif

typedef struct mpool_mem_s {
    unsigned char *address;
    int size;
    struct mpool_mem_s *next;
} mpool_mem_t;

/*
 * Function: mpool_init
 *
 * Purpose:
 *    Initialize mpool lock.
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
int 
mpool_init(void)
{
    MPOOL_LOCK_INIT();
    return 0;
}

#ifdef TRACK_DMA_USAGE
static int _dma_mem_used = 0;
#endif

/*
 * Function: mpool_alloc
 *
 * Purpose:
 *    Allocate memory block from mpool.
 * Parameters:
 *    pool - mpool handle (from mpool_create)
 *    size - size of memory block to allocate
 * Returns:
 *    Pointer to allocated memory block or NULL if allocation fails.
 */
void *
mpool_alloc(mpool_handle_t pool, int size)
{
    mpool_mem_t *ptr = pool, *newptr = NULL;
    int mod;

    MPOOL_LOCK();

    mod = size & (BCM_CACHE_LINE_BYTES - 1);
    if (mod != 0 ) {
        size += (BCM_CACHE_LINE_BYTES - mod);
    }
    while (ptr && ptr->next) {
        if (ptr->next->address - (ptr->address + ptr->size) >= size) {
            break;
        }
        ptr = ptr->next;
    }
  
    if (!(ptr && ptr->next)) {
        MPOOL_UNLOCK();
        return NULL;
    }
    newptr = MALLOC(sizeof(mpool_mem_t));
    if (!newptr) {
        MPOOL_UNLOCK();
        return NULL;
    }
  
    newptr->address = ptr->address + ptr->size;
    newptr->size = size;
    newptr->next = ptr->next;
    ptr->next = newptr;
#ifdef TRACK_DMA_USAGE
    _dma_mem_used += size;
#endif
    MPOOL_UNLOCK();

    return newptr->address;
}


/*
 * Function: mpool_free
 *
 * Purpose:
 *    Free memory block allocated from mpool..
 * Parameters:
 *    pool - mpool handle (from mpool_create)
 *    addr - address of memory block to free
 * Returns:
 *    Nothing
 */
void 
mpool_free(mpool_handle_t pool, void *addr)
{
    unsigned char *address = (unsigned char *)addr;  
    mpool_mem_t *ptr = pool, *prev = NULL;

    MPOOL_LOCK();
  
    while (ptr && ptr->next) {
        if (ptr->next->address == address) {
#ifdef TRACK_DMA_USAGE
            _dma_mem_used -= ptr->next->size;
#endif
            break;
        }
        ptr = ptr->next;
    }
  
    if (ptr && ptr->next) {
        prev = ptr;
        ptr = ptr->next;
        prev->next = ptr->next;
        FREE(ptr);
    }

    MPOOL_UNLOCK();
}

/*
 * Function: mpool_create
 *
 * Purpose:
 *    Create and initialize mpool control structures.
 * Parameters:
 *    base_ptr - pointer to mpool memory block
 *    size - total size of mpool memory block
 * Returns:
 *    mpool handle
 * Notes
 *    The mpool handle returned must be used for subsequent
 *    memory allocations from the mpool.
 */
mpool_handle_t
mpool_create(void *base_ptr, int size)
{
    mpool_mem_t *head, *tail;
    int mod = (int)(((unsigned long)base_ptr) & (BCM_CACHE_LINE_BYTES - 1));

    MPOOL_LOCK();

    if (mod) {
        base_ptr = (char*)base_ptr + (BCM_CACHE_LINE_BYTES - mod);
        size -= (BCM_CACHE_LINE_BYTES - mod);
    }
    size &= ~(BCM_CACHE_LINE_BYTES - 1);
  

    head = (mpool_mem_t *)MALLOC(sizeof(mpool_mem_t));
    if (head == NULL) {
        return NULL;
    }
    tail = (mpool_mem_t *)MALLOC(sizeof(mpool_mem_t));
    if (tail == NULL) {
        FREE(head);
        return NULL;
    }
  
    head->size = tail->size = 0;
    head->address = base_ptr;
    tail->address = head->address + size;
    head->next = tail;
    tail->next = NULL;

    MPOOL_UNLOCK();

    return head;
}

/*
 * Function: mpool_destroy
 *
 * Purpose:
 *    Free mpool control structures.
 * Parameters:
 *    pool - mpool handle (from mpool_create)
 * Returns:
 *    Always 0
 */
int
mpool_destroy(mpool_handle_t pool)
{
    mpool_mem_t *ptr, *next;
  
    MPOOL_LOCK();

    for (ptr = pool; ptr; ptr = next) {
        next = ptr->next;
        FREE(ptr);
    }

    MPOOL_UNLOCK();

    return 0;
}

/*
 * Function: mpool_usage
 *
 * Purpose:
 *    Report total sum of allocated mpool memory.
 * Parameters:
 *    pool - mpool handle (from mpool_create)
 * Returns:
 *    Number of bytes currently allocated using mpool_alloc.
 */
int
mpool_usage(mpool_handle_t pool)
{
    int usage = 0;
    mpool_mem_t *ptr;

    MPOOL_LOCK();

    for (ptr = pool; ptr; ptr = ptr->next) {
	usage += ptr->size;
    }

    MPOOL_UNLOCK();

    return usage;
}
