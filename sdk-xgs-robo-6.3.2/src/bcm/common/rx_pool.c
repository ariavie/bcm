/*
 * $Id: rx_pool.c 1.9 Broadcom SDK $
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
 * File:        rx_pool.c
 * Purpose:     Receive packet pool code
 * Requires:
 */

#include <sal/core/libc.h>

#include <soc/drv.h>
#include <soc/higig.h>

#include <bcm_int/common/rx.h>
#include <bcm_int/control.h>
#include <bcm/rx.h>
#include <bcm/stack.h>
#include <bcm/debug.h>


/****************************************************************
 *
 * RX POOL CODE:
 *
 *     Default allocation and free routines
 *
 * This code sets up a very simple pool of packet buffers for RX.
 * The buffer pool holds a fixed number of fixed size packets.
 * If the pool is exhausted, the allocation returns NULL.
 *
 ****************************************************************/

static uint8 *rxp_all_bufs;      /* Allocation pointer */
#if defined(BCM_RXP_DEBUG)
static uint8 *rxp_end_bufs;      /* Points to first byte after all bufs */
#endif
static uint8 *rxp_free_list;     /* Free list pointer */
static int rxp_pkt_size;         /* Base packet size */
static int rxp_pkt_count;        /* How many packets in pool */

static sal_mutex_t rxp_mutex;

#ifdef BCM_RX_REFILL_FROM_INTR
/* Must be interrupt-safe in this mode */
#define RXP_LOCK       RX_INTR_LOCK
#define RXP_UNLOCK     RX_INTR_UNLOCK
#else
#define RXP_LOCK       sal_mutex_take(rxp_mutex, sal_mutex_FOREVER)
#define RXP_UNLOCK     sal_mutex_give(rxp_mutex)
#endif

/*
 * Define buffer alignment in physical memory.
 *
 * This is a system-specific parameter, but it is currently not possible
 * to retrieve this information through the SAL or BDE interfaces.
 */
#ifndef BCM_CACHE_LINE_BYTES
#define BCM_CACHE_LINE_BYTES 128 /* Should be fine on most platforms */
#endif
#define RXP_ALIGN(_a) \
    ((_a + (BCM_CACHE_LINE_BYTES - 1)) & ~(BCM_CACHE_LINE_BYTES - 1))

#define RXP_INIT                                                        \
    if (rxp_mutex == NULL &&                                            \
            (rxp_mutex = sal_mutex_create("rx_pool")) == NULL)          \
        return BCM_E_MEMORY

/* Define function interfaces to RX pool memory allocator

   External functions must provide the same interface as
   sal_dma_alloc() and sal_dma_free().

   BCM_RX_POOL_MEM_ALLOC() is called once during bcm_rx_pool_setup()
   to allocate RX buffer memory.

   BCM_RX_POOL_MEM_FREE() is called once during bcm_rx_pool_cleanup()
   to release allocated RX buffer memory if memory was allocated. Once
   allocated memory is freed, BCM_RX_POOL_MEM_FREE() will not be
   called again until after the next call to bcm_rx_pool_setup().

*/
#ifndef BCM_RX_POOL_MEM_ALLOC
#define BCM_RX_POOL_MEM_ALLOC sal_dma_alloc
#endif

#ifndef BCM_RX_POOL_MEM_FREE
#define BCM_RX_POOL_MEM_FREE sal_dma_free
#endif

/* Any byte in a buffer may be used to reference that buffer when freeing */

/* Buffer index; use any byte in buffer to get proper index */
#define RXP_BUF_IDX(buf)  \
    ( ((buf) - rxp_all_bufs) / rxp_pkt_size )

/*
 * Get the start/end addresses of a buffer given the index of the pkt
 */

#define RXP_BUF_START(idx)  \
    (rxp_all_bufs + ((idx) * rxp_pkt_size))

/*
 * Get the start address of a buffer given a pointer to any byte in the
 * buffer.
 */

#define RXP_PTR_TO_START(ptr) RXP_BUF_START(RXP_BUF_IDX(ptr))

/*
 * When buffers are not allocated, the first word is used as a pointer
 * for linked lists.  Requires alignment of the buffers.
 */
#define RXP_PKT_NEXT(buf) ( ((uint8 **)(RXP_PTR_TO_START(buf)))[0])

#if defined(BCM_RXP_DEBUG)

/*
 * Does "val" point into the buffer space at all?
 */

#define RXP_IN_BUFFER_SPACE(val) \
    (((val) < rxp_end_bufs) && ((val) >= rxp_all_bufs))

/*
 * Does "val" point to the start of a buffer?
 */

#define RXP_IS_BUF_START(val) \
    (RXP_IN_BUFFER_SPACE(val) &&   \
     (((val) - rxp_all_bufs) % rxp_pkt_size == 0))

/*
 * Keep pointer to "owner" for each packet
 *
 * Buffers are filled with 0xee (empty) when on free list;
 * Filled with 0xaa when allocated
 */

typedef struct {
    int state;
    const char *owner;
    int alloc;
    int free;
} RXP_TRACK_t;

static RXP_TRACK_t *rxp_tracker;
static int rxp_alloc_count;
static int rxp_tot_alloc_count;
static int rxp_tot_free_count;
static int rxp_alloc_fail;
#endif /* BCM_RXP_DEBUG */

/*
 * Function:
 *      bcm_rx_pool_setup
 * Purpose:
 *      Setup the RX packet pool
 * Parameters:
 *      pkt_count      - How many pkts in pool.  < 0 ==> default
 *      bytes_per_pkt  - Packet allocation size
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Force alignment of the words so the pointers are safe
 *
 *      The pool cannot be setup up twice without calling cleanup
 *      to deallocate pointers.
 */

int
bcm_rx_pool_setup(int pkt_count, int bytes_per_pkt)
{
    uint8 *buf, *next_buf;
    int i;

    RXP_INIT;

    if (rxp_all_bufs != NULL) {
        return BCM_E_BUSY;
    }

    if (pkt_count < 0) {
        pkt_count = BCM_RX_POOL_COUNT_DEFAULT;
    }
    if (bytes_per_pkt < 0) {
        bytes_per_pkt = BCM_RX_POOL_BYTES_DEFAULT;
    } 

    /* Force packet alignment */
    bytes_per_pkt = RXP_ALIGN(bytes_per_pkt);
    rxp_pkt_size = bytes_per_pkt;
    rxp_pkt_count = pkt_count;
    rxp_all_bufs = BCM_RX_POOL_MEM_ALLOC(pkt_count * rxp_pkt_size, "bcm_rx_pool");
    if (rxp_all_bufs == NULL) {
        return BCM_E_MEMORY;
    }
    /* Set to all 0xee for RXP debug */
    sal_memset(rxp_all_bufs, 0xee, rxp_pkt_count * rxp_pkt_size);

    RXP_LOCK;
    buf = rxp_all_bufs;
    rxp_free_list = buf;
    for (i = 0; i < pkt_count - 1; i++) {
        next_buf = buf + rxp_pkt_size;
        RXP_PKT_NEXT(buf) = next_buf;
        buf = next_buf;
    }

    RXP_PKT_NEXT(buf) = NULL;

#if defined(BCM_RXP_DEBUG)
    {
        int bytes;
        bytes = sizeof(RXP_TRACK_t) * pkt_count;
        rxp_tracker = sal_alloc(bytes, "rx_pool_dbg");
        
        sal_memset(rxp_tracker, 0, bytes);
        rxp_alloc_count = 0;
        rxp_tot_alloc_count = 0;
        rxp_tot_free_count = 0;
        rxp_alloc_fail = 0;
        rxp_end_bufs = rxp_all_bufs + rxp_pkt_count * rxp_pkt_size;
    }
#endif
    RXP_UNLOCK;

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcm_rx_pool_setup_done
 * Purpose:
 *      Indicate if setup is complete
 * Returns:
 *      Boolean:  True if setup is done
 */

int
bcm_rx_pool_setup_done(void)
{
    return rxp_all_bufs != NULL;
}


/*
 * Function:
 *      bcm_rx_pool_cleanup
 * Purpose:
 *      Deallocate the rx buffer pool
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      No checking is done to see if pointers are outstanding.
 */

int
bcm_rx_pool_cleanup(void)
{
    if (rxp_all_bufs == NULL) {
        return BCM_E_NONE;
    }

    RXP_LOCK;

    BCM_RX_POOL_MEM_FREE(rxp_all_bufs);

    rxp_all_bufs = NULL;
    rxp_free_list = NULL;

#if defined(BCM_RXP_DEBUG)
    sal_free(rxp_tracker);
    rxp_tracker = NULL;
#endif
    RXP_UNLOCK;

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_rx_pool_alloc
 * Purpose:
 *      Allocate a buffer from a fixed pool configured
 *      by bcm_rx_pool_setup.
 * Parameters:
 *      unit          - Ignored
 *      size          - Size of buffer to allocate
 *      flags         - Ignored
 * Returns:
 *      Pointer to buffer if successful, NULL if failure.
 * Notes:
 *      Can fail for:
 *
 *      1.  One size fits all....if size > how the list was set up,
 *          then the function fails.
 *      2.  Not initialized
 *      3.  No free buffers
 *
 *      The first word of the buffer is used as a "next" pointer when
 *      the buffer is in the free list.
 */

int
bcm_rx_pool_alloc(int unit, int size, uint32 flags, void **pool)
{
    uint8 *rv;

    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(flags);

    if (rxp_mutex == NULL) {
        *pool = NULL;
        return BCM_E_TIMEOUT;
    }

    if (size > rxp_pkt_size) {
        BCM_ERR(("bcm_rx_pool_alloc: %d > %d\n", size, rxp_pkt_size));
        *pool = NULL;
        return BCM_E_MEMORY;
    }

    RXP_LOCK;
    if (rxp_free_list == NULL) {
#if defined(BCM_RXP_DEBUG)
        ++rxp_alloc_fail;
#endif
        RXP_UNLOCK;
        *pool = NULL;
        return BCM_E_MEMORY;
    }
    rv = rxp_free_list;
    rxp_free_list = RXP_PKT_NEXT(rxp_free_list);

#if defined(BCM_RXP_DEBUG)
    {
        int idx;
        static const char *track_name = "rx_pool_alloc";
        extern int bcm_rx_pool_free_buf_verify(void *buf);

        assert(RXP_IS_BUF_START(rv));
        if (rxp_free_list != NULL) {
            assert(RXP_IS_BUF_START(rxp_free_list));
        }

        idx = RXP_BUF_IDX(rv);
        rxp_tracker[idx].owner = track_name;
        rxp_tracker[idx].state = 1;
        rxp_tracker[idx].alloc++;
        if (bcm_rx_pool_free_buf_verify(rv) != BCM_E_NONE) {
            assert(0);
        }

        /* Fill allocated buffer with 0xaa */
        sal_memset(rv, 0xaa, rxp_pkt_size);
        ++rxp_alloc_count;
        ++rxp_tot_alloc_count;
    }
#endif /* BCM_RXP_DEBUG */
    RXP_UNLOCK;

    *pool = (void *)rv;
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_rx_pool_free
 * Purpose:
 *      Return a buffer allocated by bcm_rx_pool_alloc to the free list
 * Parameters:
 *      unit       - Ignored
 *      buf        - The buffer to free
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_rx_pool_free(int unit, void *buf)
{
    uint8 *start;
    uint8 *buf8;

    COMPILER_REFERENCE(unit);

    RXP_LOCK;

    if (rxp_all_bufs == NULL) { /* not running */
        RXP_UNLOCK;
        return BCM_E_MEMORY;
    }

    buf8 = (uint8 *)buf;
    start = RXP_PTR_TO_START(buf8);

#if defined(BCM_RXP_DEBUG)
    {
        int idx, i;
        uint32 *ptr;
        extern int bcm_rx_pool_free_verify_p(void);
        extern int bcm_rx_pool_free_buf_verify(void *buf);

        COMPILER_REFERENCE(ptr);
        COMPILER_REFERENCE(i);
        idx = RXP_BUF_IDX(buf8);
        assert((idx >= 0 && idx < rxp_pkt_count));
        rxp_tracker[idx].free++;
        if (rxp_tracker[idx].state == 0) {
            /* Buffers should never be freed more than once, but
               distinguish the error between a freed buffer that gets
               reused vs. one that doesn't. */
            BCM_WARN(("Double free of buffer %d\n", idx));
            if (bcm_rx_pool_free_buf_verify(buf) != BCM_E_NONE) {
                assert(0);
            }
            assert(0);
        }
        rxp_tracker[idx].state = 0;
        rxp_alloc_count--;
        /* Fill buffer with 0xee, except for first word */
        sal_memset(RXP_BUF_START(idx) + sizeof(void *), 0xee,
                   rxp_pkt_size - sizeof(void *));
#if 1 /* Turn this on to verify the entire free list */
        if (bcm_rx_pool_free_verify_p() != BCM_E_NONE) {
            BCM_WARN(("RX Pool error while checking %d\n", idx));
            assert(0);
        }
#else
        /* Check head of free list for good measure */
        if (rxp_free_list != NULL) {
            if (bcm_rx_pool_free_buf_verify(rxp_free_list) != BCM_E_NONE) {
                assert(0);
            }
        }
#endif
        ++rxp_tot_free_count;
    }
#endif /* BCM_RXP_DEBUG */

    RXP_PKT_NEXT(start) = rxp_free_list;
    rxp_free_list = start;
    RXP_UNLOCK;

    return BCM_E_NONE;
}


#if defined(BCM_RXP_DEBUG)

void
bcm_rx_pool_dump(int min, int max)
{
    int i;
    uint32 *ptr;

    if (min < 0) {
        min = 0;
    }
    if (max > rxp_pkt_count) {
        max = rxp_pkt_count;
    }
    for (i = min; i < max; i++) {
        ptr = (uint32 *)RXP_BUF_START(i);
        BCM_WARN(("Pkt %d: %p. [0] 0x%x [1] 0x%x [2] 0x%x\n", i,
                  ptr, ptr[0], ptr[1], ptr[2]));
    }
}

/* Check a buffer in the free pool */

#define FREE_STATE 0xeeeeeeee
#define ALLOC_STATE 0xaaaaaaaa

void
bcm_rx_pool_buf_dump(void *buf, int until)
{
    int i;
    uint32 *ptr;

    ptr = ((uint32 *)RXP_PTR_TO_START((uint8 *)buf)) + 1;
    for (i = 0; i < rxp_pkt_size - sizeof(void *); i += sizeof(uint32)) {
        if ((i & 31) == 0) {
            if (i > until && (*ptr == FREE_STATE || *ptr == ALLOC_STATE)) {
                break;
            }
            BCM_ERR(("\n%p:", ptr));
        }
        BCM_ERR((" %08x", *ptr));
        ptr++;
    }
    BCM_ERR(("\n"));
}


int
bcm_rx_pool_free_buf_verify(void *buf)
{
    int i;
    uint32 *ptr;

    ptr = ((uint32 *)RXP_PTR_TO_START((uint8 *)buf)) + 1;
    for (i = 0; i < rxp_pkt_size - sizeof(void *); i += sizeof(uint32)) {
        if (*ptr != FREE_STATE) {
            return BCM_E_INTERNAL;
        }
        ptr++;
    }
    return BCM_E_NONE;
}

/* Check the entire free pool and return BCM_E_NONE if OK or BCM_E_INTERNAL if there
   was some sort of inconsistency detected. */
int
bcm_rx_pool_free_verify_p(void)
{
    uint8 *buf;
    uint8 *last_buf = NULL;
    int idx;
    int rc = BCM_E_NONE;

    for (buf = rxp_free_list; buf != NULL; buf = RXP_PKT_NEXT(buf)) {
        idx = RXP_BUF_IDX(buf);
        if (!(idx >= 0 && idx < rxp_pkt_count)) {
            BCM_WARN(("RXP BAD BUFFER %p. idx %d\n", buf, idx));
            if (last_buf != NULL) {
                BCM_WARN(("RXP BAD BUFFER: Previous buf %p idx %d\n",
                          last_buf, RXP_BUF_IDX(last_buf)));
            } else {
                BCM_WARN(("RXP BAD BUFFER: First elt of free list\n"));
            }
            if (idx >= 0 && idx < rxp_pkt_count) {
                BCM_WARN(("RXP IDX %d out of range (0,%d)", idx, rxp_pkt_count));
            }
            return BCM_E_INTERNAL;
        }
        last_buf = buf;
        if ((rc=bcm_rx_pool_free_buf_verify(buf)) != BCM_E_NONE) {
            return rc;
        }
    }
    return rc;
}

/* API call */
void
bcm_rx_pool_free_verify(void)
{
    (void)bcm_rx_pool_free_verify_p();
}

void
bcm_rx_pool_own(void *buf, void *owner)
{
#if defined(LINUX) || defined(__KERNEL__)
    /* Need to fix this */
    return;
#else
    int idx;
    idx = RXP_BUF_IDX((uint8 *)buf);

#if 1  /* Turn this off to give warning rather than assert */
    assert((idx >= 0) && (idx < rxp_pkt_count));
    rxp_tracker[idx].owner = (char *)owner;
#else
    if ((idx < 0) || (idx >= rxp_pkt_count)) {
        BCM_WARN(("RXP: Buffer %p (idx %d) out of range "
                  "max %d, owner %s\n", buf, idx,
                  rxp_pkt_count, (char *)owner));
    } else {
        /*    assert((idx > 0) && (idx < rxp_pkt_count)); */
        rxp_tracker[idx].owner = (char *)owner;
    }
#endif
#endif
}

#define _ISALPHA(c) \
    ((c[0] >= 'A' && c[0] <= 'Z') || (c[0] >= 'a' && c[0] <= 'z'))

/* Check that a buffer is in use, or free and not corrupted */
static int rx_pool_buf_ok(int idx)
{
    if (rxp_tracker[idx].state == 0) {
        return bcm_rx_pool_free_buf_verify(RXP_BUF_START(idx));
    }
    return BCM_E_NONE;
}

void
bcm_rx_pool_report(int min, int max)
{
    int i, count = 0;
    const char *str;

    for (i = 0; i < rxp_pkt_count; i++) {
        if (rxp_tracker[i].state != 0) {
            count++;
        }
    }
    BCM_ERR((" #  s  alloc    free     owner\n"));
    BCM_ERR(("--- -  -------- -------- ------------\n"));
    for (i = min; i < max; i++) {
        str = rxp_tracker[i].owner;
        BCM_ERR(("%3d %1d%s %8d %8d %p %s\n",
                 i,
                 rxp_tracker[i].state,
                 rx_pool_buf_ok(i)? "?" : " ",
                 rxp_tracker[i].alloc,
                 rxp_tracker[i].free,
                 str,
                 _ISALPHA(str) ? str : "?"));
    }

    BCM_ERR(("  RXPool Used %d. Failed %d. Tot alloc %d. "
             "Tot free %d. Size %d. Count %d.\n", rxp_alloc_count, rxp_alloc_fail,
             rxp_tot_alloc_count, rxp_tot_free_count, rxp_pkt_size, rxp_pkt_count));
    if (count != rxp_alloc_count) {
        BCM_ERR(("  RXP WARNING:  Alloc %d, counted %d\n",
                 rxp_alloc_count, count));
    }
}
#undef _ISALPHA

#endif /* BCM_RXP_DEBUG */
