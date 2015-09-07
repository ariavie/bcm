/*
 * $Id: shr_res_bitmap.c 1.13 Broadcom SDK $
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
 * Indexed resource management -- simple bitmap
 */

#include <shared/alloc.h>
#include <shared/bitop.h>
#include <bcm/debug.h>
#include <bcm/error.h>
#include <soc/cm.h>
#include <shared/shr_res_bitmap.h>

/*
 *  This is a fairly brute-force implementation of bitmap, with minimal
 *  optimisations or improvements.  It could probably be enhanced somewhat by
 *  some subtleties, such as checking whether a SHR_BITDCL is all ones before
 *  scanning individual bits when looking for free space.
 */



#define RES_MESSAGE_ENABLE TRUE
#define RES_EXCESS_VERBOSITY TRUE
#if 0 
#undef BCM_DEBUG
#define BCM_DEBUG(flags, stuff)   soc_cm_print stuff
#endif 
#if RES_MESSAGE_ENABLE
#define RES_DEBUG(flags, stuff)  BCM_DEBUG(flags | BCM_DBG_API, stuff)
#else /* RES_MESSAGE_ENABLE */
#define RES_DEBUG(flags, stuff)
#endif /* RES_MESSAGE_ENABLE */
#define RES_OUT(stuff)           RES_DEBUG(BCM_DBG_API, stuff)
#define RES_WARN(stuff)          RES_DEBUG(BCM_DBG_WARN, stuff)
#define RES_ERR(stuff)           RES_DEBUG(BCM_DBG_ERR, stuff)
#define RES_VERB(stuff)          RES_DEBUG(BCM_DBG_VERBOSE, stuff)
#define RES_VVERB(stuff)         RES_DEBUG(BCM_DBG_VVERBOSE, stuff)
#if RES_EXCESS_VERBOSITY
#define RES_EVERB(stuff)         RES_DEBUG(BCM_DBG_VVERBOSE, stuff)
#else /* RES_EXCESS_VERBOSITY */
#define RES_EVERB(stuff)
#endif /* RES_EXCESS_VERBOSITY */
/* Please do not change the following seven lines */
#if defined(__GNUC__) && !defined(__PEDANTIC__)
#define RES_MSG(string) "%s[%d]%s" string, __FILE__, __LINE__, __FUNCTION__
#define RES_MSG1(string) "%s[%d]%s: " string, __FILE__, __LINE__, __FUNCTION__
#else /* defined(__GNUC__) && !defined(__PEDANTIC__) */
#define RES_MSG(string) "%s[%d]" string, __FILE__, __LINE__
#define RES_MSG1(string) "%s[%d]: " string, __FILE__, __LINE__
#endif /* defined(__GNUC__) && !defined(__PEDANTIC__) */

/*
 *  This controls certain optimisations that try to more quickly find available
 *  blocks.  These optimisations tend to improve allocation performance in many
 *  cases, but they also tend to reduce resource packing efficiency.
 *
 *  SHR_RES_BITMAP_SEARCH_RESUME: If TRUE, this module will track the first
 *  element of the last freed block and the next element after the last
 *  successful allocation, first trying a new alloc in the place of the last
 *  free, then starting its exhaustive search for available elements after the
 *  last successful alloc, wrapping around if needed.  If FALSE, this module
 *  will not check the last freed location for suitability and will always
 *  start the exhaustive search from the low element.
 */
#define SHR_RES_BITMAP_SEARCH_RESUME TRUE


/*
 *  Macros and other things that change according to settings...
 */
#if SHR_RES_BITMAP_SEARCH_RESUME
#define SHR_RES_BITMAP_FINAL_SEARCH_LIMIT (handle->nextAlloc)
#else /* SHR_RES_BITMAP_SEARCH_RESUME */
#define SHR_RES_BITMAP_FINAL_SEARCH_LIMIT (handle->count - count)
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */


typedef struct _shr_res_bitmap_list_s {
    int low;
    int count;
    int used;
#if SHR_RES_BITMAP_SEARCH_RESUME
    int lastFree;
    int nextAlloc;
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
    SHR_BITDCL data[1];
} _shr_res_bitmap_list_t;


/* inline */ static int
_shr_res_bitmap_check_all(shr_res_bitmap_handle_t handle,
                          int count,
                          int index)
{
    int offset;
    int freed = 0;
    int inuse = 0;

    /* scan the block */
    for (offset = 0; offset < count; offset++) {
        if (SHR_BITGET(handle->data, index + offset)) {
            inuse++;
        } else {
            freed++;
        }
    } /* for (offset = 0; offset < count; offset++) */
    if (inuse == count) {
        /* block is entirely in use */
        return BCM_E_FULL;
    } else if (freed == count) {
        /* block is entirely free */
        return BCM_E_EMPTY;
    } else {
        /* block is partially free and partially in use */
        return BCM_E_EXISTS;
    }
}

/* inline */ static int
_shr_res_bitmap_check_all_sparse(shr_res_bitmap_handle_t handle,
                                 uint32 pattern,
                                 int length,
                                 int repeats,
                                 int base)
{
    int index;
    int offset;
    int elem;
    int elemCount;
    int usedCount;

    for (index = 0, elem = base, elemCount = 0, usedCount = 0;
         index < repeats;
         index++) {
        for (offset = 0; offset < length; offset++, elem++) {
            if (pattern & (1 << offset)) {
                /* this element is in the pattern */
                elemCount++;
                if (SHR_BITGET(handle->data, elem)) {
                    /* this element is in use */
                    usedCount++;
                }
            } /* if (pattern & (1 << offset)) */
        } /* for (the length of the pattern) */
    } /* for (as many times as the pattern repeats) */
    if (elemCount == usedCount) {
        /* block is entirely in use */
        return BCM_E_FULL;
    } else if (0 == usedCount) {
        /* block is entirely free */
        return BCM_E_EMPTY;
    } else {
        /* block is partially free and partially in use */
        return BCM_E_EXISTS;
    }
}

int
shr_res_bitmap_create(shr_res_bitmap_handle_t *handle,
                      int low_id,
                      int count)
{
    shr_res_bitmap_handle_t desc;

    /* check arguments */
    if (0 >= count) {
        RES_ERR((RES_MSG1("must have a positive number of elements\n")));
        return BCM_E_PARAM;
    }
    if (!handle) {
        RES_ERR((RES_MSG1("obligatory out argument must not be NULL\n")));
        return BCM_E_PARAM;
    }
    /* alloc memory for descriptor & data */
    desc = sal_alloc(sizeof(*desc) +
                     SHR_BITALLOCSIZE(count) -
                     sizeof(SHR_BITDCL),
                     "bitmap resource data");
    if (!desc) {
        /* alloc failed */
        RES_ERR((RES_MSG1("unable to allocate %u bytes for data\n"),
                 (unsigned int)(sizeof(*desc) +
                                SHR_BITALLOCSIZE(count) -
                                sizeof(SHR_BITDCL))));
        return BCM_E_MEMORY;
    }
    /* init descriptor and data */
    sal_memset(desc,
               0x00,
               sizeof(*desc) +
               SHR_BITALLOCSIZE(count) -
               sizeof(SHR_BITDCL));
    desc->low = low_id;
    desc->count = count;
    *handle = desc;
    /* all's well if we got here */
    return BCM_E_NONE;
}

int
shr_res_bitmap_destroy(shr_res_bitmap_handle_t handle)
{
    if (handle) {
        sal_free(handle);
        return BCM_E_NONE;
    } else {
        RES_ERR((RES_MSG1("unable to free NULL handle\n")));
        return BCM_E_PARAM;
    }
}

int
shr_res_bitmap_alloc(shr_res_bitmap_handle_t handle,
                     uint32 flags,
                     int count,
                     int *elem)
{
    int index;
    int offset;
    int result = BCM_E_NONE;

    /* check arguments */
    if (!handle) {
        RES_ERR((RES_MSG1("unable to alloc from NULL descriptor\n")));
        return BCM_E_PARAM;
    }
    if (!elem) {
        RES_ERR((RES_MSG1("obligatory in/out argument must not be NULL\n")));
        return BCM_E_PARAM;
    }
    if (0 >= count) {
        RES_ERR((RES_MSG1("must allocate at least one element\n")));
        return BCM_E_PARAM;
    }
    if (SHR_RES_BITMAP_ALLOC_REPLACE ==
        (flags & (SHR_RES_BITMAP_ALLOC_REPLACE |
                  SHR_RES_BITMAP_ALLOC_WITH_ID))) {
        RES_ERR((RES_MSG1("must use WITH_ID when using REPLACE\n")));
        return BCM_E_PARAM;
    }

    if (flags & SHR_RES_BITMAP_ALLOC_WITH_ID) {
        /* WITH_ID, so only try the specifically requested block */
        if (*elem < handle->low) {
            /* not valid ID */
            result = BCM_E_PARAM;
        }
        index = *elem - handle->low;
        if (index + count > handle->count) {
            /* not valid ID */
            result = BCM_E_PARAM;
        }
        if (BCM_E_NONE == result) {
            result = _shr_res_bitmap_check_all(handle, count, index);
            switch (result) {
            case BCM_E_FULL:
                if (flags & SHR_RES_BITMAP_ALLOC_REPLACE) {
                    result = BCM_E_NONE;
                } else {
                    RES_ERR((RES_MSG1("proposed block %p base %d count %d"
                                      " already exists\n"),
                             (void*)handle,
                             *elem,
                             count));
                    result = BCM_E_RESOURCE;
                }
                break;
            case BCM_E_EMPTY:
                if (flags & SHR_RES_BITMAP_ALLOC_REPLACE) {
                    RES_ERR((RES_MSG1("proposed block %p base %d count %d"
                                      " does not exist\n"),
                             (void*)handle,
                             *elem,
                             count));
                    result = BCM_E_NOT_FOUND;
                } else {
                    result = BCM_E_NONE;
                }
                break;
            case BCM_E_EXISTS:
                RES_ERR((RES_MSG1("proposed block %p base %d count %d"
                                  " would merge/expand existing block(s)\n"),
                         (void*)handle,
                         *elem,
                         count));
                result = BCM_E_RESOURCE;
                break;
            default:
                /* should never see this */
                RES_ERR((RES_MSG1("unexpected result checking proposed block:"
                                  " %d (%s)\n"),
                         result,
                         _SHR_ERRMSG(result)));
                if (BCM_E_NONE == result) {
                    result = BCM_E_INTERNAL;
                }
            }
        }
        /* don't adjust last free or next alloc for WITH_ID */
    } else { /* if (flags & SHR_RES_ALLOC_WITH_ID) */
#if SHR_RES_BITMAP_SEARCH_RESUME
        /* see if there are enough elements after last free */
        index = handle->lastFree;
        if (index + count < handle->count) {
            /* it might fit */
            for (offset = 0; offset < count; offset++) {
                if (SHR_BITGET(handle->data, index + offset)) {
                    result = BCM_E_EXISTS;
                    break;
                }
            }
        } else {
            result = BCM_E_EXISTS;
        }
        if (BCM_E_NONE == result) {
            /* looks good; adjust last free to miss this block */
            handle->lastFree = index + count;
        } else { /* if (BCM_E_NONE == result) */
            /* start searching after last successful alloc */
            index = handle->nextAlloc;
            while (index <= handle->count - count) {
                while (SHR_BITGET(handle->data, index) &&
                       (index > handle->count - count)) {
                    index++;
                }
                if (index <= handle->count - count) {
                    /* have a candidate; see if block is big enough */
                    result = BCM_E_NONE;
                    for (offset = 0; offset < count; offset++) {
                        if (SHR_BITGET(handle->data, index + offset)) {
                            /* not big enough; skip this block */
                            result = BCM_E_EXISTS;
                            index += offset + 1;
                            break;
                        }
                    } /* for (offset = 0; offset < count; offset++) */
                } /* if (index <= desc->count - count) */
                if (BCM_E_NONE == result) {
                    /* found a sufficient block */
                    break;
                }
            } /* while (index <= desc->count - count) */
            if (BCM_E_NONE != result) {
                /* no space, so try space before last successful alloc */
#else /* SHR_RES_BITMAP_SEARCH_RESUME */
                result = BCM_E_RESOURCE;
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
                index = 0;
                while (index < SHR_RES_BITMAP_FINAL_SEARCH_LIMIT) {
                    while ((index < SHR_RES_BITMAP_FINAL_SEARCH_LIMIT) &&
                           SHR_BITGET(handle->data, index)) {
                        index++;
                    }
                    if (index < SHR_RES_BITMAP_FINAL_SEARCH_LIMIT) {
                        /* have a candidate; see if block is big enough */
                        result = BCM_E_NONE;
                        for (offset = 0; offset < count; offset++) {
                            if (SHR_BITGET(handle->data, index + offset)) {
                                /* not big enough; skip this block */
                                result = BCM_E_EXISTS;
                                index += offset + 1;
                                break;
                            }
                        } /* for (offset = 0; offset < count; offset++) */
                    } /* if (index < data->next_alloc) */
                    if (BCM_E_NONE == result) {
                        /* found a sufficient block */
                        break;
                    }
                } /* while (index < data->next_alloc) */
#if SHR_RES_BITMAP_SEARCH_RESUME
            } /* if (BCM_E_NONE != result) */
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
            if (BCM_E_NONE != result) {
                /* still no space; give up */
                result = BCM_E_RESOURCE;
#if SHR_RES_BITMAP_SEARCH_RESUME
            } else {
                /* got some space; update next alloc  */
                handle->nextAlloc = index + count;
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
            }
#if SHR_RES_BITMAP_SEARCH_RESUME
        } /* if (BCM_E_NONE == result) */
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
    } /* if (flags & SHR_RES_ALLOC_WITH_ID) */
    if (BCM_E_NONE == result) {
        /* return the beginning element */
        *elem = index + handle->low;
        /* mark the block as in use */
        SHR_BITSET_RANGE(handle->data, index, count);
        if (0 == (flags & SHR_RES_BITMAP_ALLOC_REPLACE)) {
            /* only adjust accounting if not replacing existing block */
            handle->used += count;
        }
    } /* if (BCM_E_NONE == result) */
    return result;
}

int
shr_res_bitmap_alloc_align(shr_res_bitmap_handle_t handle,
                           uint32 flags,
                           int align,
                           int offs,
                           int count,
                           int *elem)
{
    int index;
    int offset;
    int result = BCM_E_NONE;

    /* check arguments */
    if (!handle) {
        RES_ERR((RES_MSG1("unable to alloc from NULL descriptor\n")));
        return BCM_E_PARAM;
    }
    if (!elem) {
        RES_ERR((RES_MSG1("obligatory in/out argument must not be NULL\n")));
        return BCM_E_PARAM;
    }
    if (0 >= count) {
        RES_ERR((RES_MSG1("must allocate at least one element\n")));
        return BCM_E_PARAM;
    }
    if (SHR_RES_BITMAP_ALLOC_REPLACE ==
        (flags & (SHR_RES_BITMAP_ALLOC_REPLACE |
                  SHR_RES_BITMAP_ALLOC_WITH_ID))) {
        RES_ERR((RES_MSG1("must use WITH_ID when using REPLACE\n")));
        return BCM_E_PARAM;
    }

    if (flags & SHR_RES_BITMAP_ALLOC_WITH_ID) {
        /* WITH_ID, so only try the specifically requested block */
        if (*elem < handle->low) {
            /* not valid ID */
            result = BCM_E_PARAM;
        }
        index = *elem - handle->low;
        if (index + count > handle->count) {
            /* not valid ID */
            result = BCM_E_PARAM;
        }
        if (BCM_E_NONE == result) {
            /* make sure caller's request is valid */
            if (flags & SHR_RES_BITMAP_ALLOC_ALIGN_ZERO) {
                /* alignment is against zero */
                offset = (*elem) % align;
            } else {
                /* alignment is against low */
                offset = ((*elem) - handle->low) % align;
            }
            if (offset != offs) {
                RES_ERR((RES_MSG1("provided first element %d does not conform"
                                  " to provided align %d + offset %d values"
                                  " (actual offset = %d)\n"),
                         *elem,
                         align,
                         offset,
                         offs));
                result = BCM_E_PARAM;
            }
        }
        if (BCM_E_NONE == result) {
            result = _shr_res_bitmap_check_all(handle, count, index);
            switch (result) {
            case BCM_E_FULL:
                if (flags & SHR_RES_BITMAP_ALLOC_REPLACE) {
                    result = BCM_E_NONE;
                } else {
                    RES_ERR((RES_MSG1("proposed block %p base %d count %d"
                                      " already exists\n"),
                             (void*)handle,
                             *elem,
                             count));
                    result = BCM_E_RESOURCE;
                }
                break;
            case BCM_E_EMPTY:
                if (flags & SHR_RES_BITMAP_ALLOC_REPLACE) {
                    RES_ERR((RES_MSG1("proposed block %p base %d count %d"
                                      " does not exist\n"),
                             (void*)handle,
                             *elem,
                             count));
                    result = BCM_E_NOT_FOUND;
                } else {
                    result = BCM_E_NONE;
                }
                break;
            case BCM_E_EXISTS:
                RES_ERR((RES_MSG1("proposed block %p base %d count %d"
                                  " would merge/expand existing block(s)\n"),
                         (void*)handle,
                         *elem,
                         count));
                result = BCM_E_RESOURCE;
                break;
            default:
                /* should never see this */
                RES_ERR((RES_MSG1("unexpected result checking proposed block:"
                                  " %d (%s)\n"),
                         result,
                         _SHR_ERRMSG(result)));
                if (BCM_E_NONE == result) {
                    result = BCM_E_INTERNAL;
                }
            }
        }
        /* don't adjust last free or next alloc for WITH_ID */
    } else { /* if (flags & SHR_RES_ALLOC_WITH_ID) */
        if (flags & SHR_RES_BITMAP_ALLOC_ALIGN_ZERO) {
            /* alignment is against zero, not start of pool */
            offs += align - (handle->low % align);
        }
#if SHR_RES_BITMAP_SEARCH_RESUME
        /* see if there are enough elements after last free */
        index = (((handle->lastFree + align - 1) / align) * align) + offs;
        if (index + count < handle->count) {
            /* it might fit */
            for (offset = 0; offset < count; offset++) {
                if (SHR_BITGET(handle->data, index + offset)) {
                    result = BCM_E_EXISTS;
                    break;
                }
            }
        } else {
            result = BCM_E_EXISTS;
        }
        if (BCM_E_NONE == result) {
            /* looks good; adjust last free to miss this block */
            if (0 == offs) {
                handle->lastFree = index + count;
            }
        } else { /* if (BCM_E_NONE == result) */
            /* start searching after last successful alloc */
            index = (((handle->nextAlloc + align - 1) / align) * align) + offs;
            while (index <= handle->count - count) {
                while ((index <= handle->count - count) &&
                       SHR_BITGET(handle->data, index)) {
                    index += align;
                }
                if (index <= handle->count - count) {
                    /* have a candidate; see if block is big enough */
                    result = BCM_E_NONE;
                    for (offset = 0; offset < count; offset++) {
                        if (SHR_BITGET(handle->data, index + offset)) {
                            /* not big enough; skip this block */
                            result = BCM_E_EXISTS;
                            index = (((index + offset + align) / align) * align) + offs;
                            break;
                        }
                    } /* for (offset = 0; offset < count; offset++) */
                } /* if (index <= desc->count - count) */
                if (BCM_E_NONE == result) {
                    /* found a sufficient block */
                    break;
                }
            } /* while (index <= desc->count - count) */
            if (BCM_E_NONE != result) {
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
                /* no space, so try space before last successful alloc */
                index = offs;
                while (index < SHR_RES_BITMAP_FINAL_SEARCH_LIMIT) {
                    while ((index < SHR_RES_BITMAP_FINAL_SEARCH_LIMIT) &&
                           SHR_BITGET(handle->data, index)) {
                        index += align;
                    }
                    if (index < SHR_RES_BITMAP_FINAL_SEARCH_LIMIT) {
                        /* have a candidate; see if block is big enough */
                        result = BCM_E_NONE;
                        for (offset = 0; offset < count; offset++) {
                            if (SHR_BITGET(handle->data, index + offset)) {
                                /* not big enough; skip this block */
                                result = BCM_E_EXISTS;
                                index = (((index + offset + align) / align) * align) + offs;
                                break;
                            }
                        } /* for (offset = 0; offset < count; offset++) */
                    } /* if (index < data->next_alloc) */
                    if (BCM_E_NONE == result) {
                        /* found a sufficient block */
                        break;
                    }
                } /* while (index < data->next_alloc) */
#if SHR_RES_BITMAP_SEARCH_RESUME
            } /* if (BCM_E_NONE != result) */
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
            if (BCM_E_NONE != result) {
                /* still no space; give up */
                result = BCM_E_RESOURCE;
#if SHR_RES_BITMAP_SEARCH_RESUME
            } else {
                /* got some space; update next alloc  */
                handle->nextAlloc = index + count;
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
            }
#if SHR_RES_BITMAP_SEARCH_RESUME
        } /* if (BCM_E_NONE == result) */
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
    } /* if (flags & SHR_RES_ALLOC_WITH_ID) */
    if (BCM_E_NONE == result) {
        /* return the beginning element */
        *elem = index + handle->low;
        /* mark the block as in use */
        SHR_BITSET_RANGE(handle->data, index, count);
        if (0 == (flags & SHR_RES_BITMAP_ALLOC_REPLACE)) {
            /* only adjust accounting if not replacing existing block */
            handle->used += count;
        }
    } /* if (BCM_E_NONE == result) */
    return result;
}

int
shr_res_bitmap_alloc_align_sparse(shr_res_bitmap_handle_t handle,
                                  uint32 flags,
                                  int align,
                                  int offs,
                                  uint32 pattern,
                                  int length,
                                  int repeats,
                                  int *elem)
{
    int index;
    int offset;
    int repeat;
    int current;
    int first;
    int count;
    int result = BCM_E_NONE;

    /* check arguments */
    if (!handle) {
        RES_ERR((RES_MSG1("unable to alloc from NULL descriptor\n")));
        return BCM_E_PARAM;
    }
    if (!elem) {
        RES_ERR((RES_MSG1("obligatory in/out argument must not be NULL\n")));
        return BCM_E_PARAM;
    }
    if (0 >= length) {
        RES_ERR((RES_MSG1("pattern must be at least one long\n")));
        return BCM_E_PARAM;
    }
    if (32 < length) {
        RES_ERR((RES_MSG1("pattern must not be longer than 32\n")));
        return BCM_E_PARAM;
    }
    if (0 >= repeats) {
        RES_ERR((RES_MSG1("must allocate at least one pattern\n")));
        return BCM_E_PARAM;
    }
    if (0 == (pattern & ((1 << length) - 1))) {
        RES_ERR((RES_MSG1("pattern must contain at least one element\n")));
        return BCM_E_PARAM;
    }
    if (pattern & (~((1 << length) - 1))) {
        RES_ERR((RES_MSG1("pattern must not contain unused bits\n")));
        return BCM_E_PARAM;
    }
    if (SHR_RES_BITMAP_ALLOC_REPLACE ==
        (flags & (SHR_RES_BITMAP_ALLOC_REPLACE |
                  SHR_RES_BITMAP_ALLOC_WITH_ID))) {
        RES_ERR((RES_MSG1("must use WITH_ID when using REPLACE\n")));
        return BCM_E_PARAM;
    }
    /* find the final set bit of the repeated pattern */
    index = length;
    count = 0;
    do {
        index--;
        if (pattern & (1 << index)) {
            count = index;
            break;
        }
    } while (index > 0);
    count += (length * (repeats - 1));
    /* find the first set bit of the repeated pattern */
    for (first = 0; first < length; first++) {
        if (pattern & (1 << first)) {
            break;
        }
    }
#if 0 
    if (first) {
        RES_WARN((RES_MSG1("first element is not at zero; the returned block"
                           " will not point to the first element allocated,"
                           " but will point %d elements before it\n"),
                  first));
    }
#endif 

    if (flags & SHR_RES_BITMAP_ALLOC_WITH_ID) {
        /* WITH_ID, so only try the specifically requested block */
        if (*elem < handle->low) {
            RES_ERR((RES_MSG1("first element is too low\n")));
            result = BCM_E_PARAM;
        }
        index = *elem - handle->low;
        if (index + count > handle->count) {
            RES_ERR((RES_MSG1("final element is too high\n")));
            result = BCM_E_PARAM;
        }
        if (BCM_E_NONE == result) {
            /* make sure caller's request is valid */
            if (flags & SHR_RES_BITMAP_ALLOC_ALIGN_ZERO) {
                /* alignment is against zero */
                offset = (*elem) % align;
            } else {
                /* alignment is against low */
                offset = ((*elem) - handle->low) % align;
            }
            if (offset != offs) {
                RES_ERR((RES_MSG1("provided first element %d does not conform"
                                  " to provided align %d + offset %d values"
                                  " (actual offset = %d)\n"),
                         *elem,
                         align,
                         offset,
                         offs));
                result = BCM_E_PARAM;
            }
        } /* if (BCM_E_NONE == result) */
        if (BCM_E_NONE == result) {
            result = _shr_res_bitmap_check_all_sparse(handle,
                                                      pattern,
                                                      length,
                                                      repeats,
                                                      index);
            switch (result) {
            case BCM_E_FULL:
                if (flags & SHR_RES_BITMAP_ALLOC_REPLACE) {
                    result = BCM_E_NONE;
                } else {
                    RES_ERR((RES_MSG1("proposed block %p base %d pattern %08X"
                                      " length %d repeat %d already exists\n"),
                             (void*)handle,
                             *elem,
                             pattern,
                             length,
                             repeats));
                    result = BCM_E_RESOURCE;
                }
                break;
            case BCM_E_EMPTY:
                if (flags & SHR_RES_BITMAP_ALLOC_REPLACE) {
                    RES_ERR((RES_MSG1("proposed block %p base %d pattern %08X"
                                      " length %d repeat %d does not exist\n"),
                             (void*)handle,
                             *elem,
                             pattern,
                             length,
                             repeats));
                    result = BCM_E_NOT_FOUND;
                } else {
                    result = BCM_E_NONE;
                }
                break;
            case BCM_E_EXISTS:
                RES_ERR((RES_MSG1("proposed block %p base %d pattern %08X"
                                  " length %d repeat %d would merge/expand"
                                  " existing block(s)\n"),
                         (void*)handle,
                         *elem,
                         pattern,
                         length,
                         repeats));
                result = BCM_E_RESOURCE;
                break;
            default:
                /* should never see this */
                RES_ERR((RES_MSG1("unexpected result checking proposed block:"
                                  " %d (%s)\n"),
                         result,
                         _SHR_ERRMSG(result)));
                if (BCM_E_NONE == result) {
                    result = BCM_E_INTERNAL;
                }
            } /* switch (result) */
        } /* if (BCM_E_NONE == result) */
        /* don't adjust last free or next alloc for WITH_ID */
    } else { /* if (flags & SHR_RES_ALLOC_WITH_ID) */
        if (flags & SHR_RES_BITMAP_ALLOC_ALIGN_ZERO) {
            /* alignment is against zero, not start of pool */
            offs += align - (handle->low % align);
        }
#if SHR_RES_BITMAP_SEARCH_RESUME
        /* see if it fits after the last free */
        index = (((handle->lastFree + align - 1) / align) * align) + offs;
        if (index + count < handle->count) {
            /* it might fit */
            for (repeat = 0, current = index; repeat < repeats; repeat++) {
                for (offset = 0; offset < length; offset++, current++) {
                    if (pattern & (1 << offset)) {
                        if (SHR_BITGET(handle->data, current)) {
                            result = BCM_E_EXISTS;
                            break;
                        }
                    }
                }
            }
        } else {
            result = BCM_E_EXISTS;
        }
        if (BCM_E_NONE == result) {
            /* looks good; adjust last free to miss this block */
            if (0 == offs) {
                handle->lastFree = index + count;
            }
        } else { /* if (BCM_E_NONE == result) */
            /* start searching after last successful alloc */
            index = (((handle->nextAlloc + align - 1) / align) * align) + offs;
            while (index <= handle->count - count) {
                while ((index <= handle->count - count) &&
                       SHR_BITGET(handle->data, index + first)) {
                    index += align;
                }
                if (index <= handle->count - count) {
                    /* have a candidate; see if block is big enough */
                    result = BCM_E_NONE;
                    for (repeat = 0, current = index;
                         repeat < repeats;
                         repeat++) {
                        for (offset = 0; offset < length; offset++, current++) {
                            if (pattern & (1 << offset)) {
                                if (SHR_BITGET(handle->data, current)) {
                                    /* an element is in use */
                                    result = BCM_E_EXISTS;
                                    /* skip to next alignment point */
                                    index += align;
                                    /* start comparing again */
                                    break;
                                } /* if (this element is in use) */
                            } /* if (this element is in the pattern) */
                        } /* for (length of the pattern) */
                    } /* for (number of repetitions of the pattern) */
                } /* if (index <= desc->count - count) */
                if (BCM_E_NONE == result) {
                    /* found a sufficient block */
                    break;
                }
            } /* while (index <= desc->count - count) */
            if (BCM_E_NONE != result) {
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
                /* no space, so try space before last successful alloc */
                index = offs;
                while (index < SHR_RES_BITMAP_FINAL_SEARCH_LIMIT) {
                    while ((index < SHR_RES_BITMAP_FINAL_SEARCH_LIMIT) &&
                           SHR_BITGET(handle->data, index + first)) {
                        index += align;
                    }
                    if (index < SHR_RES_BITMAP_FINAL_SEARCH_LIMIT) {
                        /* have a candidate; see if block is big enough */
                        result = BCM_E_NONE;
                        for (repeat = 0, current = index;
                             repeat < repeats;
                             repeat++) {
                            for (offset = 0;
                                 offset < length;
                                 offset++, current++) {
                                if (pattern & (1 << offset)) {
                                    if (SHR_BITGET(handle->data, current)) {
                                        /* an element is in use */
                                        result = BCM_E_EXISTS;
                                        /* skip to next alignment point */
                                        index += align;
                                        /* start comparing again */
                                        break;
                                    } /* if (this element is in use) */
                                } /* if (this element is in the pattern) */
                            } /* for (length of the pattern) */
                        } /* for (number of repetitions of the pattern) */
                    } /* if (index < end of possible space) */
                    if (BCM_E_NONE == result) {
                        /* found a sufficient block */
                        break;
                    }
                } /* while (index < data->next_alloc) */
#if SHR_RES_BITMAP_SEARCH_RESUME
            } /* if (BCM_E_NONE != result) */
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
            if (BCM_E_NONE != result) {
                /* still no space; give up */
                result = BCM_E_RESOURCE;
#if SHR_RES_BITMAP_SEARCH_RESUME
            } else {
                /* got some space; update next alloc  */
                handle->nextAlloc = index + count;
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
            }
#if SHR_RES_BITMAP_SEARCH_RESUME
        } /* if (BCM_E_NONE == result) */
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
    } /* if (flags & SHR_RES_ALLOC_WITH_ID) */
    if (BCM_E_NONE == result) {
        /* return the beginning element */
        *elem = index + handle->low;
        /* mark the block as in use */
        for (repeat = 0, count = 0, current = index;
             repeat < repeats;
             repeat++) {
            for (offset = 0; offset < length; offset++, current++) {
                if (pattern & (1 << offset)) {
                    SHR_BITSET(handle->data, current);
                    count++;
                } /* if (this element is in the pattern) */
            } /* for (length of the pattern) */
        } /* for (number of repetitions of the pattern) */
        if (0 == (flags & SHR_RES_BITMAP_ALLOC_REPLACE)) {
            /* only adjust accounting if not replacing existing block */
            handle->used += count;
        }
    } /* if (BCM_E_NONE == result) */
    return result;
}

int
shr_res_bitmap_free(shr_res_bitmap_handle_t handle,
                    int count,
                    int elem)
{
    int index;
    int offset;
    int result = BCM_E_NONE;

    /* check arguments */
    if (!handle) {
        RES_ERR((RES_MSG1("unable to alloc from NULL descriptor\n")));
        return BCM_E_PARAM;
    }
    if (elem < handle->low) {
        /* not valid ID */
        result = BCM_E_PARAM;
    }
    if (0 >= count) {
        RES_ERR((RES_MSG1("must free at least one element\n")));
        return BCM_E_PARAM;
    }

    index = elem - handle->low;
    if (index + count > handle->count) {
        /* not valid ID */
        result = BCM_E_PARAM;
    }
    if (BCM_E_NONE == result) {
        /* check whether the block is in use */
        for (offset = 0; offset < count; offset++) {
            if (!SHR_BITGET(handle->data, index + offset)) {
                /* not entirely in use */
                result = BCM_E_NOT_FOUND;
                break;
            }
        } /* for (offset = 0; offset < count; offset++) */
    } /* if (BCM_E_NONE == result) */
    if (BCM_E_NONE == result) {
        /* looks fine, so mark the block as free */
        SHR_BITCLR_RANGE(handle->data, index, count);
        handle->used -= count;
#if SHR_RES_BITMAP_SEARCH_RESUME
        handle->lastFree = index;
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
    } /* if (BCM_E_NONE == result) */
    /* return the result */
    return result;
}

int
shr_res_bitmap_free_sparse(shr_res_bitmap_handle_t handle,
                           uint32 pattern,
                           int length,
                           int repeats,
                           int elem)
{
    int index;
    int offset;
    int final;
    int result = BCM_E_NONE;

    /* check arguments */
    if (!handle) {
        RES_ERR((RES_MSG1("unable to alloc from NULL descriptor\n")));
        return BCM_E_PARAM;
    }
    if (elem < handle->low) {
        RES_ERR((RES_MSG1("first element is too low\n")));
        result = BCM_E_PARAM;
    }
    if (0 >= length) {
        RES_ERR((RES_MSG1("pattern must be at least one long\n")));
        return BCM_E_PARAM;
    }
    if (32 < length) {
        RES_ERR((RES_MSG1("pattern must not be longer than 32\n")));
        return BCM_E_PARAM;
    }
    if (0 >= repeats) {
        RES_ERR((RES_MSG1("must check at least one pattern\n")));
        return BCM_E_PARAM;
    }
    if (0 == (pattern & ((1 << length) - 1))) {
        RES_ERR((RES_MSG1("pattern must contain at least one element\n")));
        return BCM_E_PARAM;
    }
    if (pattern & (~((1 << length) - 1))) {
        RES_ERR((RES_MSG1("pattern must not contain unused bits\n")));
        return BCM_E_PARAM;
    }
    index = length;
    final = 0;
    do {
        index--;
        if (pattern & (1 << index)) {
            final = index;
            break;
        }
    } while (index > 0);
    final += (length * (repeats - 1));

    elem -= handle->low;
    if (elem + final > handle->count) {
        RES_ERR((RES_MSG1("last element is too high\n")));
        result = BCM_E_PARAM;
    }
    if (BCM_E_NONE == result) {
        /* check whether the block is in use */
        result = _shr_res_bitmap_check_all_sparse(handle,
                                                  pattern,
                                                  length,
                                                  repeats,
                                                  elem);
        if (BCM_E_FULL == result) {
            /* block is fully in use */
#if SHR_RES_BITMAP_SEARCH_RESUME
            handle->lastFree = elem;
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
            for (index = 0; index < repeats; index++) {
                for (offset = 0; offset < length; offset++, elem++) {
                    if (pattern & (1 << offset)) {
                        SHR_BITCLR(handle->data, elem);
                        handle->used--;
                    } /* if (this element is in the pattern) */
                } /* for (pattern length) */
            } /* for (all repeats) */
            result = BCM_E_NONE;
        } else { /* if (BCM_E_FULL == result) */
            /* not entirely in use */
            result = BCM_E_NOT_FOUND;
        } /* if (BCM_E_FULL == result) */
    } /* if (BCM_E_NONE == result) */
    /* return the result */
    return result;
}

int
shr_res_bitmap_check(shr_res_bitmap_handle_t handle,
                     int count,
                     int elem)
{
    int index;
    int offset;
    int result = BCM_E_NONE;

    /* check arguments */
    if (!handle) {
        RES_ERR((RES_MSG1("unable to alloc from NULL descriptor\n")));
        return BCM_E_PARAM;
    }
    if (elem < handle->low) {
        /* not valid ID */
        result = BCM_E_PARAM;
    }
    if (0 >= count) {
        RES_ERR((RES_MSG1("must check at least one element\n")));
        return BCM_E_PARAM;
    }

    index = elem - handle->low;
    if (index + count > handle->count) {
        /* not valid ID */
        result = BCM_E_PARAM;
    }
    if (BCM_E_NONE == result) {
        /* check whether the block is in use */
        result = BCM_E_NOT_FOUND;
        for (offset = 0; offset < count; offset++) {
            if (SHR_BITGET(handle->data, index + offset)) {
                /* not entirely free */
                result = BCM_E_EXISTS;
                break;
            }
        } /* for (offset = 0; offset < count; offset++) */
    } /* if (BCM_E_NONE == result) */
    /* return the result */
    return result;
}

int
shr_res_bitmap_check_all(shr_res_bitmap_handle_t handle,
                         int count,
                         int elem)
{
    int index;
    int result = BCM_E_NONE;

    /* check arguments */
    if (!handle) {
        RES_ERR((RES_MSG1("unable to alloc from NULL descriptor\n")));
        return BCM_E_PARAM;
    }
    if (elem < handle->low) {
        /* not valid ID */
        result = BCM_E_PARAM;
    }
    if (0 >= count) {
        RES_ERR((RES_MSG1("must check at least one element\n")));
        return BCM_E_PARAM;
    }

    index = elem - handle->low;
    if (index + count > handle->count) {
        /* not valid ID */
        result = BCM_E_PARAM;
    }
    if (BCM_E_NONE == result) {
        result = _shr_res_bitmap_check_all(handle, count, index);
    }
    /* return the result */
    return result;
}

int
shr_res_bitmap_check_all_sparse(shr_res_bitmap_handle_t handle,
                                uint32 pattern,
                                int length,
                                int repeats,
                                int elem)
{
    int index;
    int final;
    int result = BCM_E_NONE;

    /* check arguments */
    if (!handle) {
        RES_ERR((RES_MSG1("unable to alloc from NULL descriptor\n")));
        return BCM_E_PARAM;
    }
    if (elem < handle->low) {
        RES_ERR((RES_MSG1("first element is too low\n")));
        result = BCM_E_PARAM;
    }
    if (0 >= length) {
        RES_ERR((RES_MSG1("pattern must be at least one long\n")));
        return BCM_E_PARAM;
    }
    if (32 < length) {
        RES_ERR((RES_MSG1("pattern must not be longer than 32\n")));
        return BCM_E_PARAM;
    }
    if (0 >= repeats) {
        RES_ERR((RES_MSG1("must check at least one pattern\n")));
        return BCM_E_PARAM;
    }
    if (0 == (pattern & ((1 << length) - 1))) {
        RES_ERR((RES_MSG1("pattern must contain at least one element\n")));
        return BCM_E_PARAM;
    }
    if (pattern & (~((1 << length) - 1))) {
        RES_ERR((RES_MSG1("pattern must not contain unused bits\n")));
        return BCM_E_PARAM;
    }
    /* find the final set bit of the repeated pattern */
    index = length;
    final = 0;
    do {
        index--;
        if (pattern & (1 << index)) {
            final = index;
            break;
        }
    } while (index > 0);
    final += (length * (repeats - 1));

    elem -= handle->low;
    if (elem + final > handle->count) {
        RES_ERR((RES_MSG1("last element is too high\n")));
        result = BCM_E_PARAM;
    }
    if (BCM_E_NONE == result) {
        result = _shr_res_bitmap_check_all_sparse(handle,
                                                  pattern,
                                                  length,
                                                  repeats,
                                                  elem);
    }
    /* return the result */
    return result;
}

int
shr_res_bitmap_dump(const shr_res_bitmap_handle_t handle)
{
    int result;
    int error = FALSE;
    int elemsUsed;
    int index;
    int offset;
    int elemOffset;
    int rowUse;

    if (!handle) {
        RES_ERR((RES_MSG1("must provide non-NULL handle\n")));
        return BCM_E_PARAM;
    }
    soc_cm_print("shr_res_bitmap at %p:\n", (const void*)handle);
    soc_cm_print("  lowest ID     = %08X\n", handle->low);
    soc_cm_print("  element count = %08X\n", handle->count);
    soc_cm_print("  used elements = %08X\n", handle->used);
#if SHR_RES_BITMAP_SEARCH_RESUME
    soc_cm_print("  last free     = %08X %s\n",
                 handle->lastFree,
                 (error |= (handle->lastFree > handle->count))?"INVALID":"");
    soc_cm_print("  next alloc    = %08X %s\n",
                 handle->nextAlloc,
                 (error |= (handle->nextAlloc > handle->count))?"INVALID":"");
#endif /* SHR_RES_BITMAP_SEARCH_RESUME */
    soc_cm_print("  element map:\n");
    soc_cm_print("    1st Elem (index)    State of elements (1 = used)\n");
    soc_cm_print("    -------- --------   --------------------------------------------------\n");
    elemsUsed = 0;
    for (index = 0; index < handle->count; /* increment in loop */) {
        soc_cm_print("    %08X %08X   ", index + handle->low, index);
        elemOffset = 0;
        rowUse = 0;
        for (offset = 0; offset < 48; offset++) {
            if ((16 == offset) || (32 == offset)) {
                soc_cm_print(" ");
            }
            if (index < handle->count) {
                if (SHR_BITGET(handle->data, index + elemOffset)) {
                    soc_cm_print("1");
                    rowUse++;
                } else {
                    soc_cm_print("0");
                }
                index++;
            }
        }
        soc_cm_print("\n");
        elemsUsed += rowUse;
    } /* for all grains */
    soc_cm_print("  counted elems = %08X %s\n",
                 elemsUsed,
                 (error |= (elemsUsed != handle->used))?"INVALID":"");
    if (error) {
        soc_cm_print("bitmap %p appears to be corrupt\n",
                     (void*)handle);
        result = BCM_E_INTERNAL;
    } else {
        result = BCM_E_NONE;
    }
    return result;
}


