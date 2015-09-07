/*
 * $Id: alloc.c,v 1.31 Broadcom SDK $
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
 * File: 	alloc.c
 * Purpose:	Defines sal routines for memory allocation
 */

#include <sys/types.h>
#include <stdlib.h>
#include <assert.h>

#include <sal/types.h>

#include <sal/core/memlog.h>

#ifndef USE_EXTERNAL_MEM_CHECKING
#define USE_EXTERNAL_MEM_CHECKING 0
#endif
#if USE_EXTERNAL_MEM_CHECKING
#define EXT_DEBUG_ALLOC(_sz) do { return malloc(_sz); } while(0)
#define EXT_DEBUG_FREE(_addr) do { free(_addr); return; } while(0)
#else
#define EXT_DEBUG_ALLOC(_sz)
#define EXT_DEBUG_FREE(_addr)
#endif

#ifndef AGGRESSIVE_ALLOC_DEBUG_TESTING
#define AGGRESSIVE_ALLOC_DEBUG_TESTING 0
#endif
#if AGGRESSIVE_ALLOC_DEBUG_TESTING
#if USE_EXTERNAL_MEM_CHECKING
#error "USE_EXTERNAL_MEM_CHECKING and AGGRESSIVE_ALLOC_DEBUG_TESTING can\'t be set both."
#endif
#ifndef AGGRESSIVE_ALLOC_DEBUG_TESTING_KEEP_ORDER
/* The allocations stored in array.
 * When freeing allocation in the middle of the array,
 * the last entry in the array replace it.
 * if this flags is set, all the entries next to the free allocation will be pushed down,
 * and hence, the order will be kept.
 */
#define AGGRESSIVE_ALLOC_DEBUG_TESTING_KEEP_ORDER 0
#endif
#include "alloc_debug.h"
#define AGGR_DEBUG_ALLOC(_p, _sz, _s) sal_alloc_debug_alloc(_p, _sz, _s)
#define AGGR_DEBUG_FREE(_addr) sal_alloc_debug_free(_addr)
#define AGGR_DEBUG_PRINT_BAD_ADDR(_addr) sal_alloc_debug_print_bad_addr(_addr)
#else
#define AGGR_DEBUG_ALLOC(_p, _sz, _s)
#define AGGR_DEBUG_FREE(_addr)
#define AGGR_DEBUG_PRINT_BAD_ADDR(addr)
#endif /* AGGRESSIVE_ALLOC_DEBUG_TESTING */


/* To do: use real data segment limits for bad pointer detection */
#define BAD_PTR(p)						\
        (PTR_TO_UINTPTR(p) < 0x1000UL ||			\
         PTR_TO_UINTPTR(p) > ~0xfffUL)
#define CORRUPT(p)						\
	(p[-1] != 0xaaaaaaaa ||					\
	 p[p[-2]] != 0xbbbbbbbb)

static unsigned long	sal_alloc_bytes;
static unsigned long	sal_alloc_calls;
static unsigned long	sal_free_bytes;
static unsigned long	sal_free_calls;

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
static unsigned int _sal_alloc_max;
static unsigned int _sal_alloc_curr;

#define SAL_ALLOC_RESOURCE_USAGE_INCR(a_curr, a_max, a_size, ilock)     \
        a_curr += (a_size);                                             \
        a_max = ((a_curr) > (a_max)) ? (a_curr) : (a_max)

#define SAL_ALLOC_RESOURCE_USAGE_DECR(a_curr, a_size, ilock)            \
        a_curr -= (a_size)                                              \

/*
 * Function:
 *      sal_alloc_resource_usage_get
 * Purpose:
 *      Provides Current/Maximum memory allocation.
 * Parameters:
 *      alloc_curr - Current memory usage.
 *      alloc_max - Memory usage high water mark
 */

void 
sal_alloc_resource_usage_get(uint32 *alloc_curr, uint32 *alloc_max)
{
    if (alloc_curr != NULL) {
        *alloc_curr = _sal_alloc_curr;
    }
    if (alloc_max != NULL) {
        *alloc_max = _sal_alloc_max;
    }
}
static unsigned int _sal_dma_alloc_max;
static unsigned int _sal_dma_alloc_curr;

#define SAL_DMA_ALLOC_RESOURCE_USAGE_INCR(a_curr, a_max, a_size, ilock) \
        a_curr += (a_size);                                             \
        a_max = ((a_curr) > (a_max)) ? (a_curr) : (a_max)

#define SAL_DMA_ALLOC_RESOURCE_USAGE_DECR(a_curr, a_size, ilock)        \
        a_curr -= (a_size)

/*
 * Function:
 *      sal_dma_alloc_resource_usage_get
 * Purpose:
 *      Provides Current/Maximum memory allocation.
 * Parameters:
 *      alloc_curr - Current memory usage.
 *      alloc_max - Memory usage high water mark
 */

void 
sal_dma_alloc_resource_usage_get(uint32 *alloc_curr, uint32 *alloc_max)
{
    if (alloc_curr != NULL) {
        *alloc_curr = _sal_dma_alloc_curr;
    }
    if (alloc_max != NULL) {
        *alloc_max = _sal_dma_alloc_max;
    }
}
#endif
/*
 * Function:
 *      sal_alloc_stat
 * Purpose:
 *      Dump the current allocations
 * Parameters:
 *      param - integer used to change behavior.
 * Notes:
 *      If the parameter is non-zero, the routine will not
 *      display successive entries with the same description.
 */
void
sal_alloc_stat(void *param)
{
}
#endif /* BROADCOM_DEBUG */

/*
 * Function:
 *	sal_alloc
 * Purpose:
 *	Allocate general purpose system memory.
 * Parameters:
 *	sz - size of memory block to allocate.
 *	s - optional user description of memory block for debugging.
 * Returns:
 *	Pointer to memory block
 * Notes:
 *	Memory allocated by this routine is not guaranteed to be safe
 *	for hardware DMA read/write.
 */

void *
sal_alloc(unsigned int sz, char *s)
{
    unsigned int orig_sz, alloc_sz;
    uint32	*p;

    EXT_DEBUG_ALLOC(sz);

    /*
     * Round up size to accommodate corruption detection sentinels.
     * Place sentinels at the beginning and end of the data area to
     * detect memory corruption.  These are verified on free.
     */

    orig_sz = sz;

    sz = (sz + 3) & ~3;

    /* Check for wrap caused by bad input */
    alloc_sz = sz + 12;
    if (alloc_sz < orig_sz) {
        return NULL;
    }

    sal_alloc_calls += 1;

    if ((p = malloc(alloc_sz)) == 0) {
	return p;
    }

    assert(UINTPTR_TO_PTR(PTR_TO_UINTPTR(p)) == p);

    sal_alloc_bytes += sz;

    p[0] = sz / 4;
    p[1] = 0xaaaaaaaa;
    p[2 + sz / 4] = 0xbbbbbbbb;

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
    SAL_ALLOC_RESOURCE_USAGE_INCR(
        _sal_alloc_curr,
        _sal_alloc_max,
        (sz),
        ilock);
#endif
#endif /* BROADCOM_DEBUG */

    AGGR_DEBUG_ALLOC(p, sz, s);

    MEMLOG_ALLOC("sal_alloc", (void *)&p[0], orig_sz, s);

    return (void *) &p[2];
}

/*
 * Function:
 *	sal_free
 * Purpose:
 *	Free memory block allocate by sal_alloc
 * Parameters:
 *	addr - Address returned by sal_alloc
 */

void 
sal_free(void *addr)
{
    uint32	*p	= (uint32 *)addr;
    uint32  *ap = p; /* Originally Allocated Pointer */

    EXT_DEBUG_FREE(addr);

    /*
     * Verify sentinels on free.  If this assertion fails, it means that
     * memory corruption was detected.
     */

#ifdef SAL_FREE_NULL_IGNORE
    if (addr == NULL)
    {
        return;
    }
#endif

    AGGR_DEBUG_FREE(addr);

    if (BAD_PTR(p) || CORRUPT(p)) {
        AGGR_DEBUG_PRINT_BAD_ADDR(addr);
        assert(!BAD_PTR(p));	/* Use macro to beautify assert message */
        assert(!CORRUPT(p));	/* Use macro to beautify assert message */
    }

    /* Adjust for Corruption detection Sentinels */
    ap--;
    ap--;

    sal_free_calls += 1;
    sal_free_bytes += ap[0] * 4;

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
    SAL_ALLOC_RESOURCE_USAGE_DECR(
        _sal_alloc_curr,
        (ap[0] * 4),
        ilock);
#endif
#endif /* BROADCOM_DEBUG */

    MEMLOG_FREE("sal_free", (void *)&p[-2]);

    ap[1] = 0;			/* Detect redundant frees */
    /*    coverity[address_free : FALSE]    */
    free(ap);
}

#ifndef LINUX_SAL_DMA_ALLOC_OVERRIDE

/*
 * Function:
 *	sal_dma_alloc
 * Purpose:
 *	Allocate memory that can be DMA'd into/out of.
 * Parameters:
 *	sz - number of bytes to allocate
 *	s - string associated with allocate
 * Returns:
 *	Pointer to allocated memory or NULL if out of memory.
 * Notes:
 *	Memory allocated by this routine is not guaranteed to be safe
 *	for hardware DMA read/write. This is for use only on sim platform.
 */

void *
sal_dma_alloc(size_t sz, char *s)
{
    uint32	*p;

    /*
     * Round up size to accommodate corruption detection sentinels.
     * Place sentinels at the beginning and end of the data area to
     * detect memory corruption.  These are verified on free.
     */
    sz = (sz + 3) & ~3;

    if ((p = malloc(sz + 12)) == 0) {
	return p;
    }

    assert(INT_TO_PTR(PTR_TO_INT(p)) == p);

    p[0] = sz / 4;
    p[1] = 0xaaaaaaaa;
    p[2 + sz / 4] = 0xbbbbbbbb;
#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
    SAL_DMA_ALLOC_RESOURCE_USAGE_INCR(
        _sal_dma_alloc_curr,
        _sal_dma_alloc_max,
        (sz),
        ilock);
#endif
#endif /* BROADCOM_DEBUG */

    MEMLOG_ALLOC("sal_dma_alloc", &p[0], orig_sz, s);

    return (void *) &p[2];
}

/*
 * Function:
 *	sal_dma_free
 * Purpose:
 *	Free memory allocated by sal_dma_alloc
 * Parameters:
 *	addr - pointer to memory to free.
 * Returns:
 *	Nothing.
 */

void
sal_dma_free(void *addr)
{
    uint32	*p	= (uint32 *)addr;
    uint32  *ap = p; /* Originally Allocated Pointer */

    /*
     * Verify sentinels on free.  If this assertion fails, it means that
     * memory corruption was detected.
     */

    /*    coverity[conditional (1): FALSE]    */
    /*    coverity[conditional (2): FALSE]    */
    assert(!BAD_PTR(p));	/* Use macro to beautify assert message */
    /*    coverity[conditional (3): FALSE]    */
    /*    coverity[conditional (4): FALSE]    */
    assert(!CORRUPT(p));	/* Use macro to beautify assert message */


    /* Adjust for Corruption detection Sentinels */
    ap--;
    ap--;

#ifdef BROADCOM_DEBUG
#ifdef INCLUDE_BCM_SAL_PROFILE
    SAL_DMA_ALLOC_RESOURCE_USAGE_DECR(
        _sal_dma_alloc_curr,
        (ap[0] * 4),
        ilock);
#endif
#endif /* BROADCOM_DEBUG */

    MEMLOG_FREE("sal_dma_free", ap);

    ap[1] = 0;			/* Detect redundant frees */
    /*    coverity[address_free : FALSE]    */
    free(ap);
}

#endif /* LINUX_SAL_DMA_ALLOC_OVERRIDE */

/*
 * Function:
 *	sal_dma_flush
 * Purpose:
 *	Ensure modified cache is written out to memory.
 * Parameters:
 *	addr - beginning of address region
 *	len - size of address region
 * Notes:
 *	A region of memory should always be flushed before telling
 *	hardware to start a DMA read from that memory.
 */

void
sal_dma_flush(void *addr, int len)
{
    COMPILER_REFERENCE(addr);
    COMPILER_REFERENCE(len);
}

/*
 * Function:
 *	sal_dma_inval
 * Purpose:
 *	Ensure cache memory is discarded and not written out to memory.
 * Parameters:
 *	addr - beginning of address region
 *	len - size of address region
 * Notes:
 *	A region of memory should always be invalidated before telling
 *	hardware to start a DMA write into that memory.
 */

void
sal_dma_inval(void *addr, int len)
{
    COMPILER_REFERENCE(addr);
    COMPILER_REFERENCE(len);
}

/*
 * Function:
 *	sal_dma_vtop
 * Purpose:
 *	Convert a virtual memory address to physical.
 * Parameters:
 *	addr - address to convert
 * Returns:
 *	Physical address
 */

void *
sal_dma_vtop(void *addr)
{
    return addr;
}

/*
 * Function:
 *	sal_dma_ptov
 * Purpose:
 *	Convert a physical memory address to virtual.
 * Parameters:
 *	addr - address to convert
 * Returns:
 *	Virtual address
 */

void *
sal_dma_ptov(void *addr)
{
    return addr;
}

/*
 * Function:
 *	sal_memory_check
 * Purpose:
 *	Check an address in memory for existence and writability.
 * Parameters:
 *	addr - address to check
 * Returns:
 *	0 if writable, 1 if not
 * Notes:
 *	Should be written to catch SIGBUS and SIGSEGV.
 *	For now, just returns success.
 */

int
sal_memory_check(uint32 addr)
{
    return 0;
}
