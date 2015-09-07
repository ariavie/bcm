/*
 * $Id: shr_res_tag_bitmap.h,v 1.5 Broadcom SDK $
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
 * Indexed resource management -- tagged bitmap
 */

#ifndef _SHR_RES_TAG_BITMAP_H_
#define _SHR_RES_TAG_BITMAP_H_

#include <sal/types.h>

/*
 *  Provide WITH_ID when allocating a block and you want to specify the initial
 *  element of that block.
 *
 *  Provide ALIGN_ZERO when allocating an aligned block and you want that block
 *  to be aligned against zero rather than against the low_id value used when
 *  creating the resource.
 *
 *  Provide REPLACE when allocating WITH_ID to indicate you want to replace an
 *  existing block.  Note this requires that the existing block be there in its
 *  entirety; it is an error to try to change the size of a block this way.
 */
#define SHR_RES_TAG_BITMAP_ALLOC_WITH_ID      0x00000001
#define SHR_RES_TAG_BITMAP_ALLOC_ALIGN_ZERO   0x00000002
#define SHR_RES_TAG_BITMAP_ALLOC_REPLACE      0x00000004

struct _shr_res_tag_bitmap_list_s;
typedef struct _shr_res_tag_bitmap_list_s *shr_res_tag_bitmap_handle_t;

/*
 *   Function
 *      shr_res_tag_bitmap_create
 *   Purpose
 *      Create a tagged bitmap resource
 *   Parameters
 *      (OUT) handle    : where to put the handle
 *      (IN) low_id     : minimum valid element ID
 *      (IN) count      : number of elements total
 *      (IN) grain_size : number of elements per grain
 *      (IN) tag_size   : number of bytes in each grain's tag
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 *      This will destroy all of the resource pools, then tear down the rest of
 *      the resource management for the instance.
 *
 *      There are count / grain_size grains, and each one has a tag associated.
 */
extern int
shr_res_tag_bitmap_create(shr_res_tag_bitmap_handle_t *handle,
                          int low_id,
                          int count,
                          int grain_size,
                          int tag_size);

/*
 *   Function
 *      shr_res_tag_bitmap_destroy
 *   Purpose
 *      Destroy a tagged bitmap resource
 *   Parameters
 *      (IN) handle : handle for the instance to access
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 */
extern int
shr_res_tag_bitmap_destroy(shr_res_tag_bitmap_handle_t handle);

/*
 *   Function
 *      shr_res_tag_bitmap_alloc
 *   Purpose
 *      Allocate an element or block of elements of a particular resource
 *   Parameters
 *      (IN) handle   : handle for the instance to access
 *      (IN) flags    : flags providing specifics of what/how to allocate
 *      (IN) count    : elements to allocate in this block
 *      (IN/OUT) elem : where to put the allocated element (block base)
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 *      The elem argument is IN if the WITH_ID flag is specified; it is OUT if
 *      the WITH_ID flag is not specified.
 *
 *      This will allocate a single block of the requested number of elements
 *      of this resource (each of which may be a number of elements taken from
 *      the underlying pool).
 *
 *      The caller must track how many elements were requested and provide that
 *      number when freeing the block.
 *
 *      Note the tag used will be all zero bytes for blocks allocated with this
 *      call (since there is no tag specified here).
 */
extern int
shr_res_tag_bitmap_alloc(shr_res_tag_bitmap_handle_t handle,
                         uint32 flags,
                         int count,
                         int *elem);

/*
 *   Function
 *      shr_res_tag_bitmap_alloc_tag
 *   Purpose
 *      Allocate an element or block of elements of a particular resource,
 *      assuring all of the elements have the same tag.
 *   Parameters
 *      (IN) handle   : handle for the instance to access
 *      (IN) flags    : flags providing specifics of what/how to allocate
 *      (IN) tag      : pointer to tag value to use
 *      (IN) count    : elements to allocate in this block
 *      (IN/OUT) elem : where to put the allocated element (block base)
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 *      The elem argument is IN if the WITH_ID flag is specified; it is OUT if
 *      the WITH_ID flag is not specified.
 *
 *      The tag pointer is a pointer to the value that will be used for tagging
 *      the requested block of elements.  Note that since tags are a number of
 *      bytes starting at the pointer, the tag must already be masked as needed
 *      and should be stored in something no larger than the number of bytes
 *      that was specified as tag size when setting up the pool.
 *
 *      This will allocate a single block of the requested number of elements
 *      of this resource (each of which may be a number of elements taken from
 *      the underlying pool).
 *
 *      If neither this nor the alloc_tag call is used, the tag will be assumed
 *      to be all zeroes.
 *
 *      Partial blocks will not be allocated.
 *
 *      The caller must track how many elements were requested and provide that
 *      number when freeing the block.
 */
extern int
shr_res_tag_bitmap_alloc_tag(shr_res_tag_bitmap_handle_t handle,
                             uint32 flags,
                             const void *tag,
                             int count,
                             int *elem);

/*
 *   Function
 *      shr_res_tag_bitmap_alloc_align
 *   Purpose
 *      Allocate an element or block of elements of a particular resource,
 *      using a base alignment and an offset.
 *   Parameters
 *      (IN) handle   : handle for the instance to access
 *      (IN) flags    : flags providing specifics of what/how to allocate
 *      (IN) align    : base alignment
 *      (IN) offset   : offest from base alignment for first element
 *      (IN) count    : elements to allocate in this block
 *      (IN/OUT) elem : where to put the allocated element (block base)
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 *      The elem argument is IN if the WITH_ID flag is specified; it is OUT if
 *      the WITH_ID flag is not specified.  If WITH_ID is specified, and the
 *      requested base element does not comply with the indicated alignment,
 *      BCM_E_PARAM will be returned.
 *
 *      This will allocate a single block of the requested number of elements
 *      of this resource (each of which may be a number of elements taken from
 *      the underlying pool).
 *
 *      The first element of the returned block will be at ((n * align) +
 *      offset), where n is some integer.  If it is not possible to allocate a
 *      block with the requested constraints, the call will fail.  Note that
 *      the alignment is within the specified range of the resource, and not
 *      specifically aligned against the absolute value zero; to request the
 *      alignment be against zero, specify the ALIGN_ZERO flag.
 *
 *      If offset >= align, BCM_E_PARAM.  If align is zero or negative, it will
 *      be treated as if it were 1.
 *
 *      Partial blocks will not be allocated.
 *
 *      The caller must track how many elements were requested and provide that
 *      number when freeing the block.
 *
 *      Note the tag used will be all zero bytes for blocks allocated with this
 *      call (since there is no tag specified here).
 */
extern int
shr_res_tag_bitmap_alloc_align(shr_res_tag_bitmap_handle_t handle,
                               uint32 flags,
                               int align,
                               int offs,
                               int count,
                               int *elem);

/*
 *   Function
 *      shr_res_tag_bitmap_alloc_align_tag
 *   Purpose
 *      Allocate an element or block of elements of a particular resource,
 *      using a base alignment and an offset, and assuring the elements all
 *      have the same tag.
 *   Parameters
 *      (IN) handle   : handle for the instance to access
 *      (IN) flags    : flags providing specifics of what/how to allocate
 *      (IN) align    : base alignment
 *      (IN) offset   : offest from base alignment for first element
 *      (IN) tag      : pointer to tag value to use
 *      (IN) count    : elements to allocate in this block
 *      (IN/OUT) elem : where to put the allocated element (block base)
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 *      The elem argument is IN if the WITH_ID flag is specified; it is OUT if
 *      the WITH_ID flag is not specified.  If WITH_ID is specified, and the
 *      requested base element does not comply with the indicated alignment,
 *      BCM_E_PARAM will be returned.
 *
 *      The tag pointer is a pointer to the value that will be used for tagging
 *      the requested block of elements.  Note that since tags are a number of
 *      bytes starting at the pointer, the tag must already be masked as needed
 *      and should be stored in something no larger than the number of bytes
 *      that was specified as tag size when setting up the pool.
 *
 *      This will allocate a single block of the requested number of elements
 *      of this resource (each of which may be a number of elements taken from
 *      the underlying pool), and ensuring all have the same tag (elements can
 *      be within the same grain as elements from other blocks only if the tag
 *      of the partial grain is equal to the tag for the new elements).
 *
 *      The first element of the returned block will be at ((n * align) +
 *      offset), where n is some integer.  If it is not possible to allocate a
 *      block with the requested constraints, the call will fail.  Note that
 *      the alignment is within the specified range of the resource, and not
 *      specifically aligned against the absolute value zero; to request the
 *      alignment be against zero, specify the ALIGN_ZERO flag.
 *
 *      If offset >= align, BCM_E_PARAM.  If align is zero or negative, it will
 *      be treated as if it were 1.
 *
 *      Partial blocks will not be allocated.
 *
 *      The caller must track how many elements were requested and provide that
 *      number when freeing the block.
 */
extern int
shr_res_tag_bitmap_alloc_align_tag(shr_res_tag_bitmap_handle_t handle,
                                   uint32 flags,
                                   int align,
                                   int offs,
                                   const void *tag,
                                   int count,
                                   int *elem);

/*
 *   Function
 *      shr_res_tag_bitmap_free
 *   Purpose
 *      Free an element or block of elements of a particular resource
 *   Parameters
 *      (IN) handle : handle for the instance to access
 *      (IN) count  : elements in the block to free
 *      (IN) elem   : the element to free (or base of the block to free)
 *   Returns
 *      BCM_E_NONE if successful
 *      BCM_E_* as appropriate otherwise
 *   Notes
 *      This will free a single block of the requested number of elements,
 *      starting at the specified element.  Some of the allocators do not deal
 *      with blocks and so must be told how large a block was, so it is
 *      obligatory that the caller be able to provide such information.
 *
 *      This should only be called with valid data (base element and element
 *      count) against known allocated blocks.  Trying to free a block that is
 *      not in use or trying to free something that spans multiple allocated
 *      blocks may not work.
 */
extern int
shr_res_tag_bitmap_free(shr_res_tag_bitmap_handle_t handle,
                        int count,
                        int elem);

/*
 *   Function
 *      shr_res_tag_bitmap_check
 *   Purpose
 *      Check the status of a specific element
 *   Parameters
 *      (IN) handle : handle for the instance to access
 *      (IN) count  : elements in the block to check
 *      (IN) elem   : the element to check (or base of the block to check)
 *   Returns
 *      BCM_E_NOT_FOUND if the element is not in use
 *      BCM_E_EXISTS if the element is in use
 *      BCM_E_PARAM if the element is not valid
 *      BCM_E_* as appropriate otherwise
 *   Notes
 *      This will check whether the requested block of the resource is
 *      allocated.  Note that if any element of the resource in the range of
 *      [elem..(elem+count-1)] (inclusive) is not free, it returns
 *      BCM_E_EXISTS; it will only return BCM_E_NOT_FOUND if all elements
 *      within the specified block are free.
 *
 *      Normally this should be called to check on a specific block (one that
 *      is thought to exist or in preparation for allocating it WITH_ID).
 */
extern int
shr_res_tag_bitmap_check(shr_res_tag_bitmap_handle_t handle,
                         int count,
                         int elem);

/*
 *   Function
 *      shr_res_tag_bitmap_check_all_tag
 *   Purpose
 *      Check the status of a block of elements
 *   Parameters
 *      (IN) handle : handle for the instance to access
 *      (IN) tag    : the tag for comparison
 *      (IN) count  : elements in the block to check
 *      (IN) elem   : the element to check (or base of the block to check)
 *   Returns
 *      BCM_E_EMPTY if none of the elements are in use
 *      BCM_E_FULL if all of the elements are in use
 *      BCM_E_CONFIG if elements are in use but block(s) do not match
 *      BCM_E_EXISTS if some of the elements are in use but not all of them
 *      BCM_E_PARAM if any of the elements is not valid
 *      BCM_E_* as appropriate otherwise
 *   Notes
 *      This will check whether the requested block of the resource is
 *      allocated.  Note that if any element of the resource in the range of
 *      [elem..(elem+count-1)] (inclusive) is not free, it returns
 *      BCM_E_EXISTS; it will only return BCM_E_NOT_FOUND if all elements
 *      within the specified block are free.
 *
 *      Normally this should be called to check on a specific block (one that
 *      is thought to exist or in preparation for allocating it WITH_ID).
 *
 *      WANRING: The tagged bitmap allocator does not track blocks internally
 *      and so it is possible that if there are two adjacent blocks with
 *      identical tags both allocated and this is called to check whether safe
 *      to 'reallocate', will falsely indicate that it can be done.  However,
 *      this will consider different tags to indicate different blocks, so will
 *      not assert false true for the case if the adjacent blocks have
 *      different tags, unless the block size is smaller than the tag size, in
 *      which case it still could falsely claim the operation is valid. Also,
 *      'reallocate' in a similar manner of a large block to a smaller block
 *      could leak underlying resources.
 */
extern int
shr_res_tag_bitmap_check_all_tag(shr_res_tag_bitmap_handle_t handle,
                                 const void *tag,
                                 int count,
                                 int elem);

/*
 *  Same as shr_res_tag_bitmap_check_all_tag except without explicit tag.
 *
 *  Unlike the other supported operations that work without explicit tag, this
 *  function assumes the first tag in the block is the desired tag.  If you
 *  want the normal default tag behaviour, provide NULL as the tag argument and
 *  call shr_res_tag_bitmap_check_all_tag instead.
 */
extern int
shr_res_tag_bitmap_check_all(shr_res_tag_bitmap_handle_t handle,
                             int count,
                             int elem);

/*
 *   Function
 *      shr_res_tag_bitmap_dump
 *   Purpose
 *      Dump the internal state of a tagged bitmap allocator
 *   Parameters
 *      (IN) handle : handle for instance to dump
 *   Returns
 *      BCM_E_PARAM if handle is clearly bogus
 *      BCM_E_INTERNAL if there is obvious corruption
 *      BCM_E_NONE usually for successful dump
 *      BCM_E_* otherwise as appropriate
 *   Notes
 *      There is very little that can be verified for corruption in the tagged
 *      bitmap allocator, particularly if the search resume feature is off, and
 *      even if it is on, still rather little.
 */
extern int
shr_res_tag_bitmap_dump(const shr_res_tag_bitmap_handle_t handle);

#endif /* ndef _SHR_RES_TAG_BITMAP_H_ */

