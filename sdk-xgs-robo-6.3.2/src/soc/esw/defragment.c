/*
 * $Id: defragment.c 1.2 Broadcom SDK $
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
 * Provides generic routines for defragmenting member tables.
 */

#include <soc/defragment.h>
#include <soc/drv.h>

/*
 * Function:
 *      soc_defragment_block_cmp
 * Purpose:
 *      Compare two blocks of member table entries based on their starting
 *      index.
 * Parameters:
 *      a - (IN) First block. 
 *      b - (IN) Second block. 
 * Returns:
 *      -1 if a < b, 0 if a = b, 1 if a > b.
 */
STATIC int
soc_defragment_block_cmp(void *a, void *b)
{
    soc_defragment_block_t *block_a, *block_b;

    block_a = (soc_defragment_block_t *)a;
    block_b = (soc_defragment_block_t *)b;

    if (block_a->base_ptr < block_b->base_ptr) {
        return -1;
    }

    if (block_a->base_ptr == block_b->base_ptr) {
        return 0;
    }

    return 1;
}

/*
 * Function:
 *      soc_defragment_block_move
 * Purpose:
 *      Move a block of member table entries.
 * Parameters:
 *      unit         - (IN) Unit
 *      src_base_ptr - (IN) Starting index of the source block.
 *      dst_base_ptr - (IN) Starting index of the destination block.
 *      block_size   - (IN) Number of entries to move.
 *      member_op - (IN) Operations on member table entry
 *      group     - (IN) The group this block belongs to
 *      group_op  - (IN) Operations on group table entry
 * Returns:
 *      SOC_E_xxx
 */
STATIC int
soc_defragment_block_move(int unit, int src_base_ptr, int dst_base_ptr,
        int block_size, soc_defragment_member_op_t *member_op,
        int group, soc_defragment_group_op_t *group_op)
{
    int i;
    int rv;

    /* Copy source block to destination block */
    for (i = 0; i < block_size; i++) {
        rv = (*member_op->member_copy)(unit, src_base_ptr + i, dst_base_ptr + i);
        SOC_IF_ERROR_RETURN(rv);
    }

    /* Update group's base pointer to point to destination block */
    rv = (*group_op->group_base_ptr_update)(unit, group, dst_base_ptr);
    SOC_IF_ERROR_RETURN(rv);

    /* Clear source block */
    for (i = 0; i < block_size; i++) {
        rv = (*member_op->member_clear)(unit, src_base_ptr + i);
        SOC_IF_ERROR_RETURN(rv);
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      soc_defragment
 * Purpose:
 *      Defragment a member table.
 * Parameters:
 *      unit           - (IN) Unit
 *      block_count    - (IN) Number of blocks in block_array
 *      block_array    - (IN) Used blocks of member table entries
 *      reserved_block - (IN) Reserved block of member table entries used as
 *                            defragmentation buffer
 *      member_table_size - (IN) Number of entries in member table
 *      member_op - (IN) Operations on member table entry
 *      group_op  - (IN) Operations on group table entry
 * Returns:
 *      SOC_E_xxx
 */
int
soc_defragment(int unit, int block_count,
        soc_defragment_block_t *block_array,
        soc_defragment_block_t *reserved_block,
        int member_table_size,
        soc_defragment_member_op_t *member_op,
        soc_defragment_group_op_t *group_op)
{
    soc_defragment_block_t *sorted_block_array = NULL;
    int reserved_block_base_ptr, reserved_block_size;
    int free_block_base_ptr, free_block_size;
    int gap_base_ptr, gap_size;
    int block_base_ptr, block_size;
    int max_block_size;
    int group;
    int i;
    int rv;

    if (0 == block_count) {
        /* If no member entries are used, there is no need to defragment. */
        return SOC_E_NONE;
    }

    if (NULL == block_array) {
        return SOC_E_PARAM;
    }

    if (NULL == reserved_block) {
        return SOC_E_PARAM;
    }

    if (NULL == group_op) {
        return SOC_E_PARAM;
    }

    if (NULL == member_op) {
        return SOC_E_PARAM;
    }

    /* Sort the block array by block's base pointer */
    sorted_block_array = sal_alloc(block_count * sizeof(soc_defragment_block_t),
            "sorted block array");
    if (NULL == sorted_block_array) {
        return SOC_E_MEMORY;
    }
    sal_memcpy(sorted_block_array, block_array,
            block_count * sizeof(soc_defragment_block_t));
    _shr_sort(sorted_block_array, block_count, sizeof(soc_defragment_block_t),
            soc_defragment_block_cmp);

    /* Get defragmentation buffer */
    if (0 == reserved_block->size) {
        /* If a reserved block is not given, find the largest free block of
         * entries in the member table and use it as the defragmentation
         * buffer.
         */
        reserved_block_size = 0;
        reserved_block_base_ptr = 0;
        free_block_base_ptr = 0;
        for (i = 0; i < block_count; i++) {
            free_block_size = sorted_block_array[i].base_ptr -
                              free_block_base_ptr;
            if (free_block_size > reserved_block_size) {
                reserved_block_size = free_block_size;
                reserved_block_base_ptr = free_block_base_ptr;
            } 
            free_block_base_ptr = sorted_block_array[i].base_ptr +
                                  sorted_block_array[i].size;
        }

        /* Also need to compute the free block size between the end of the
         * final used block and the end of the member table.
         */
        free_block_size = member_table_size - free_block_base_ptr;
        if (free_block_size > reserved_block_size) {
            reserved_block_size = free_block_size;
            reserved_block_base_ptr = free_block_base_ptr;
        } 
    } else {
        reserved_block_size = reserved_block->size;
        reserved_block_base_ptr = reserved_block->base_ptr;
    }

    /* Find maximum block size */
    max_block_size = 0;
    for (i = 0; i < block_count; i++) {
        if (sorted_block_array[i].size > max_block_size) {
            max_block_size = sorted_block_array[i].size;
        }
    }

    /* Compress the gaps between used blocks */
    gap_base_ptr = 0;
    for (i = 0; i < block_count; i++) {
        block_base_ptr = sorted_block_array[i].base_ptr;
        block_size = sorted_block_array[i].size;
        group = sorted_block_array[i].group;

        gap_size = block_base_ptr - gap_base_ptr;
        if (block_base_ptr > reserved_block_base_ptr) {
            if (gap_base_ptr <= reserved_block_base_ptr) {
                if (0 == reserved_block->size) {
                    if (gap_size < max_block_size) {
                        /* Skip over the reserved block */
                        gap_base_ptr = reserved_block_base_ptr +
                                       reserved_block_size;
                        gap_size = block_base_ptr - gap_base_ptr;
                    }
                } else { 
                    gap_size = reserved_block_base_ptr - gap_base_ptr;
                    if (gap_size < block_size) {
                        /* Skip over the reserved block */
                        gap_base_ptr = reserved_block_base_ptr +
                                       reserved_block_size;
                        gap_size = block_base_ptr - gap_base_ptr;
                    }
                }
            }
        }

        if (gap_size == 0) {
            gap_base_ptr = block_base_ptr + block_size;
        } else if (gap_size > 0) {
            if (block_size <= gap_size) {
                /* Block fits into the gap. Move it directly into the gap. */
                rv = soc_defragment_block_move(unit, block_base_ptr,
                        gap_base_ptr, block_size, member_op, group, group_op);
                if (SOC_FAILURE(rv)) {
                    sal_free(sorted_block_array);
                    return rv;
                }

                /* Move gap_base_ptr */
                gap_base_ptr += block_size;

            } else if (block_size <= reserved_block_size) {
                /* Block is bigger than the gap but fits into the
                 * defragmentation buffer. Move it first into the
                 * defragmentation buffer, then into the gap.
                 */
                rv = soc_defragment_block_move(unit, block_base_ptr,
                        reserved_block_base_ptr, block_size, member_op,
                        group, group_op);
                if (SOC_FAILURE(rv)) {
                    sal_free(sorted_block_array);
                    return rv;
                }
                rv = soc_defragment_block_move(unit, reserved_block_base_ptr,
                        gap_base_ptr, block_size, member_op, group, group_op);
                if (SOC_FAILURE(rv)) {
                    sal_free(sorted_block_array);
                    return rv;
                }

                /* Move gap_base_ptr */
                gap_base_ptr += block_size;

            } else {
                /* Block is bigger than the gap and the defragmentation
                 * buffer. It cannot be moved. Skip over it.
                 */
                gap_base_ptr = block_base_ptr + block_size;
            }
        } else {
            /* Gap size shoud never be negative. */
            sal_free(sorted_block_array);
            return SOC_E_INTERNAL;
        }
    }

    sal_free(sorted_block_array);
    return SOC_E_NONE;
}

