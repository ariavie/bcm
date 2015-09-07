/*
 * $Id: ipmc.c,v 1.53 Broadcom SDK $
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
 * File:        ipmc.c
 * Purpose:     Triumph3 IPMC implementation.
 */

#include <soc/defs.h>
#include <sal/core/libc.h>
#include <shared/bsl.h>
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA2_SUPPORT) 
#if defined(INCLUDE_L3)

#include <soc/drv.h>
#include <soc/mem.h>

#include <bcm/error.h>
#include <bcm/l3.h>
#include <bcm/types.h>

#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/ipmc.h>
#include <bcm_int/esw/firebolt.h>

#ifdef BCM_KATANA2_SUPPORT
#include <bcm_int/esw/katana2.h>
#include <bcm_int/esw/subport.h>
#endif /* BCM_KATANA2_SUPPORT */

#ifdef BCM_KATANA_SUPPORT
#include <bcm_int/esw/katana.h>
#endif /* BCM_KATANA_SUPPORT */
/* Structures for managing REPL_HEAD table resources */

typedef struct _tr3_repl_head_free_block_s {
    int index; /* Starting index of a free block of REPL_HEAD table entries */
    int size;  /* Number of entries in the free block */
    struct _tr3_repl_head_free_block_s *next; /* Pointer to next free block */
} _tr3_repl_head_free_block_t;

typedef struct _tr3_repl_head_info_s {
    _tr3_repl_head_free_block_t **free_list_array;
                              /* Array of lists of free blocks */
    int array_size;           /* Number of lists in the array */
} _tr3_repl_head_info_t;

typedef struct _kt2_repl_grp_info_s {
   int mb_index;
   int lb_index;
   soc_mem_t mb_mem;
   soc_mem_t lb_mem;
   soc_field_t mbmp;
   soc_field_t lbmp;
} _kt2_repl_grp_info_t;

typedef struct _tr3_if_updated_s {
    bcm_if_t *if_array_del;   /* the interface array to be deleted */   
    int       if_count_del;   /* size of the interface array to be deleted */
    bcm_if_t *if_array_new;   /* the new interface array to be added */
    int       if_count_new;   /* size of the new interface array to be added */
} _tr3_if_updated_t;
static _tr3_repl_head_info_t *_tr3_repl_head_info[BCM_MAX_NUM_UNITS];

#define REPL_HEAD_FREE_LIST(_unit_, _index_) \
    _tr3_repl_head_info[_unit_]->free_list_array[_index_]

/* Structures for managing REPL_LIST table resources */

typedef struct _tr3_repl_list_entry_info_s {
    SHR_BITDCL *bitmap_entries_used; /* A bitmap indicating which REPL_LIST
                                        entries are used */
    int num_entries; /* Total number of entries in REPL_LIST table */
} _tr3_repl_list_entry_info_t;

static _tr3_repl_list_entry_info_t \
           *_tr3_repl_list_entry_info[BCM_MAX_NUM_UNITS];

static uint8 reserve_multicast_resources = 0;

#define REPL_LIST_ENTRY_USED_GET(_u_, _i_) \
        SHR_BITGET(_tr3_repl_list_entry_info[_u_]->bitmap_entries_used, _i_)
#define REPL_LIST_ENTRY_USED_SET(_u_, _i_) \
        SHR_BITSET(_tr3_repl_list_entry_info[_u_]->bitmap_entries_used, _i_)
#define REPL_LIST_ENTRY_USED_CLR(_u_, _i_) \
        SHR_BITCLR(_tr3_repl_list_entry_info[_u_]->bitmap_entries_used, _i_)

/* Structures containing replication info */

typedef struct _tr3_repl_port_info_s {
    int *intf_count; /* Array of interface counts, one per replication group */
} _tr3_repl_port_info_t;

typedef struct _tr3_repl_info_s {
    int num_repl_groups; /* Number of replication groups on this device */
    uint32 num_intf;     /* Number of interfaces on this device */
    _bcm_repl_list_info_t *repl_list_info;
                         /* Linked list of replication lists */
#if defined(BCM_ESW_SUPPORT) && (SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS)
    _tr3_repl_port_info_t *port_info[SOC_MAX_NUM_PP_PORTS];
#else
    _tr3_repl_port_info_t *port_info[SOC_MAX_NUM_PORTS];
#endif
                         /* Per port replication info */
    int *l3_intf_next_hop_ipmc; /* Array of next hop indices, one for each
                            L3 interface that's added to an IPMC group */ 
    int *l3_intf_next_hop_trill; /* Array of next hop indices, one for each
                            L3 interface that's added to a Trill group */ 
} _tr3_repl_info_t;

static _tr3_repl_info_t *_tr3_repl_info[BCM_MAX_NUM_UNITS];

#define REPL_LOCK(_u_)                         \
    {                                          \
        soc_mem_lock(_u_, MMU_REPL_LIST_TBLm); \
    }
#define REPL_UNLOCK(_u_)                         \
    {                                            \
        soc_mem_unlock(_u_, MMU_REPL_LIST_TBLm); \
    }

#define REPL_INIT(unit) \
        if (_tr3_repl_info[unit] == NULL) { return BCM_E_INIT; }

#define REPL_GROUP_ID(_u_, _id_) \
        if ((_id_ < 0) || (_id_ >= _tr3_repl_info[_u_]->num_repl_groups)) \
            { return BCM_E_PARAM; }

#define REPL_INTF_TOTAL(_u_) _tr3_repl_info[_u_]->num_intf

#define REPL_LIST_INFO(_u_) _tr3_repl_info[_u_]->repl_list_info

#define REPL_PORT_GROUP_INTF_COUNT(_u_, _p_, _group_id_) \
    _tr3_repl_info[_u_]->port_info[_p_]->intf_count[_group_id_]

#define REPL_L3_INTF_NEXT_HOP_IPMC(_u_, _intf_) \
    _tr3_repl_info[_u_]->l3_intf_next_hop_ipmc[_intf_]
#define REPL_L3_INTF_NEXT_HOP_TRILL(_u_, _intf_) \
    _tr3_repl_info[_u_]->l3_intf_next_hop_trill[_intf_]

#if defined(BCM_KATANA2_SUPPORT)
#define REPL_PORT_CHECK(unit, port) \
    { \
        if (SOC_IS_KATANA2(unit)) { \
            if (!IS_PORT(unit, port) && \
                !IS_AXP_PORT(unit, port) && \
                !(port >= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MIN) && \
                !(port <= _BCM_KT2_SUBPORT_PP_PORT_INDEX_MAX)) { \
                return BCM_E_PARAM; \
            } \
        }  else { \
            if (!IS_PORT(unit, port) && !IS_AXP_PORT(unit, port)) { \
                return BCM_E_PARAM; \
            } \
        } \
    }
#else
#define REPL_PORT_CHECK(unit, port) \
    { \
        if (!IS_PORT(unit, port) && !IS_AXP_PORT(unit, port)) { \
            return BCM_E_PARAM; \
        } \
    }
#endif

#define REPL_NH_INDEX_UNALLOCATED -1
#define REPL_NH_INDEX_L3_EGRESS_ALLOCATED -2

/* --------------------------------------------------------------
 * The following set of routines manage REPL_HEAD table resource.
 * --------------------------------------------------------------
 */

/*
 * Function:
 *      _bcm_tr3_repl_head_info_deinit
 * Purpose:
 *      De-initialize replication head info.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      None
 */
STATIC void 
_bcm_tr3_repl_head_info_deinit(int unit)
{
    int i;
    _tr3_repl_head_free_block_t *block_ptr;
    _tr3_repl_head_free_block_t *next_block_ptr;

    if (NULL != _tr3_repl_head_info[unit]) {
        if (NULL != _tr3_repl_head_info[unit]->free_list_array) {
            for (i = 0; i < _tr3_repl_head_info[unit]->array_size; i++) {
                block_ptr = REPL_HEAD_FREE_LIST(unit, i);
                while (NULL != block_ptr) {
                    next_block_ptr = block_ptr->next;
                    sal_free(block_ptr);
                    block_ptr = next_block_ptr;
                }
                REPL_HEAD_FREE_LIST(unit, i) = NULL;
            }
            sal_free(_tr3_repl_head_info[unit]->free_list_array);
            _tr3_repl_head_info[unit]->free_list_array = NULL;
            _tr3_repl_head_info[unit]->array_size = 0;
        }
        sal_free(_tr3_repl_head_info[unit]);
        _tr3_repl_head_info[unit] = NULL;
    }
}

/*
 * Function:
 *      _bcm_tr3_repl_head_info_init
 * Purpose:
 *      Initialize replication head info.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_tr3_repl_head_info_init(int unit)
{
    int max_array_index = 0;
    int num_of_ports = 0;

    if (NULL == _tr3_repl_head_info[unit]) {
        _tr3_repl_head_info[unit] = sal_alloc(sizeof(_tr3_repl_head_info_t),
                "repl_head_info");
        if (NULL == _tr3_repl_head_info[unit]) {
            _bcm_tr3_repl_head_info_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_tr3_repl_head_info[unit], 0, sizeof(_tr3_repl_head_info_t));

    if (NULL == _tr3_repl_head_info[unit]->free_list_array) {
        /* Each element of the array is a linked list of free blocks.
         * Array element N is a linked list of free blocks of size N.
         * When allocating a block of REPL_HEAD table entries, the max number
         * of entries needed is equal to the max number of members in a
         * replication group. This will also be the max index of the array. 
         * Of course, the REPL_HEAD table may contain bigger blocks of free
         * entries. Array element 0 wil be a linked list of free blocks with 
         * size greater than the max number of members in a replication group.
         */ 
        if (soc_mem_field_valid(unit, MMU_REPL_GROUP_INFO0m, PIPE_MEMBER_BMPf) &&
            soc_mem_field_valid(unit, MMU_REPL_GROUP_INFO1m, PIPE_MEMBER_BMPf)) {
            max_array_index =
                soc_mem_field_length(unit, MMU_REPL_GROUP_INFO0m,
                        PIPE_MEMBER_BMPf) +
                soc_mem_field_length(unit, MMU_REPL_GROUP_INFO1m,
                        PIPE_MEMBER_BMPf);
        } else {       
            if (SOC_IS_KATANA2(unit)) {
                
                max_array_index =
                (soc_mem_field_length(unit, MMU_REPL_GRP_TBL0m,
                                      MEMBER_BMP_PORTS_119_0f) +
                 soc_mem_field_length(unit, MMU_REPL_GRP_TBL1m,
                                       MEMBER_BMP_PORTS_169_120f));

            } else {  
                max_array_index = soc_mem_field_length(unit, MMU_REPL_GROUPm,
                        MEMBER_BITMAPf);
            }
        }
        _tr3_repl_head_info[unit]->free_list_array =
            sal_alloc(sizeof(_tr3_repl_head_free_block_t *) *
                      (max_array_index + 1), "repl head free list array");
        if (NULL == _tr3_repl_head_info[unit]->free_list_array) {
            _bcm_tr3_repl_head_info_deinit(unit);
            return BCM_E_MEMORY;
        }
        _tr3_repl_head_info[unit]->array_size = max_array_index + 1;
    }
    sal_memset(_tr3_repl_head_info[unit]->free_list_array, 0,
            sizeof(_tr3_repl_head_free_block_t *) *
            _tr3_repl_head_info[unit]->array_size);

    if (soc_feature(unit, soc_feature_static_repl_head_alloc)) {
        if (!SOC_WARM_BOOT(unit)) {
            /* Clear replication head table */
            SOC_IF_ERROR_RETURN
                (soc_mem_clear(unit, MMU_REPL_HEAD_TBLm, MEM_BLOCK_ALL, 0));
        }
    } else {
        REPL_HEAD_FREE_LIST(unit, 0) = 
            sal_alloc(sizeof(_tr3_repl_head_free_block_t), "repl head free block");
        if (NULL == REPL_HEAD_FREE_LIST(unit, 0)) {
            _bcm_tr3_repl_head_info_deinit(unit);
            return BCM_E_MEMORY;
        }
        
        /* Initially, there is only one free block, starting at entry 
         * (1 + max_array_index) of the REPL_HEAD table and ending 
         * at the last entry. Entry 0 is reserved.
         * Entries from (soc_mem_index_max(unit,MMU_REPL_HEAD_TBLm) - 
         * count + 1) to  soc_mem_index_max(unit,MMU_REPL_HEAD_TBLm) 
         * are reserved for swapping the REPL_HEADm of a replication group 
         * when there is no resource for deleting a port 
         * from the replication group.
         */
        reserve_multicast_resources = 
            soc_property_get(unit, spn_RESERVE_MULTICAST_RESOURCES, 0) ? 1 : 0;
        if (reserve_multicast_resources) {
            BCM_PBMP_COUNT(PBMP_ALL(unit), num_of_ports);
        }
        REPL_HEAD_FREE_LIST(unit, 0)->index = 1;
        REPL_HEAD_FREE_LIST(unit, 0)->size = soc_mem_index_max(unit,
                MMU_REPL_HEAD_TBLm) - num_of_ports;
        REPL_HEAD_FREE_LIST(unit, 0)->next = NULL;

        /* Clear entry 0 of REPL_HEAD table */
        SOC_IF_ERROR_RETURN(soc_mem_write(unit, MMU_REPL_HEAD_TBLm, MEM_BLOCK_ALL,
                    0, soc_mem_entry_null(unit, MMU_REPL_HEAD_TBLm)));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_repl_head_block_free
 * Purpose:
 *      Free a block of REPL_HEAD table entries.
 * Parameters:
 *      unit  - (IN) SOC unit number. 
 *      index - (IN) Index of the first entry of the block.
 *      size  - (IN) Size of the block.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_tr3_repl_head_block_free(int unit, int index, int size)
{
    int i;
    int block_index, block_size;
    int coalesced_index, coalesced_size;
    _tr3_repl_head_free_block_t *block_ptr;
    _tr3_repl_head_free_block_t *prev_block_ptr;
    _tr3_repl_head_free_block_t *next_block_ptr;
    _tr3_repl_head_free_block_t *coalesced_block_ptr;

    if (size <= 0) {
        return BCM_E_INTERNAL;
    }

    /* First, coalesce the block with any existing free blocks
     * that are contiguous with the block.
     */ 
    coalesced_index = index;
    coalesced_size = size;
    for (i = 0; i < _tr3_repl_head_info[unit]->array_size; i++) {
        block_ptr = REPL_HEAD_FREE_LIST(unit, i);
        prev_block_ptr = NULL;
        while (NULL != block_ptr) {
            block_index = block_ptr->index;
            block_size = block_ptr->size;
            next_block_ptr = block_ptr->next;
            if ((block_index + block_size) == coalesced_index) {
                coalesced_index = block_index;
                coalesced_size += block_size;
                if (block_ptr == REPL_HEAD_FREE_LIST(unit, i)) {
                    REPL_HEAD_FREE_LIST(unit, i) = next_block_ptr;
                } else {
                    /* 
                     * In the following line of code, Coverity thinks the
                     * prev_block_ptr may still be NULL when dereferenced.
                     * This situation will never occur because 
                     * if block_ptr is not pointing to the head of the 
                     * linked list, prev_block_ptr would not be NULL.
                     */
                    /* coverity[var_deref_op : FALSE] */
                    prev_block_ptr->next = next_block_ptr;
                } 
                sal_free(block_ptr);
            } else if ((coalesced_index + coalesced_size) == block_index) {
                coalesced_size += block_size;
                if (block_ptr == REPL_HEAD_FREE_LIST(unit, i)) {
                    REPL_HEAD_FREE_LIST(unit, i) = next_block_ptr;
                } else {
                    /* 
                     * In the following line of code, Coverity thinks the
                     * prev_block_ptr may still be NULL when dereferenced.
                     * This situation will never occur because 
                     * if block_ptr is not pointing to the head of the 
                     * linked list, prev_block_ptr would not be NULL.
                     */
                    /* coverity[var_deref_op : FALSE] */
                    prev_block_ptr->next = next_block_ptr;
                } 
                sal_free(block_ptr);
            } else {
                prev_block_ptr = block_ptr;
            }
            block_ptr = next_block_ptr;
        }
    }

    /* Insert coalesced free block */
    coalesced_block_ptr = sal_alloc(sizeof(_tr3_repl_head_free_block_t),
            "coalesced repl head free block");
    if (NULL == coalesced_block_ptr) {
        return BCM_E_MEMORY;
    }
    coalesced_block_ptr->index = coalesced_index;
    coalesced_block_ptr->size = coalesced_size;
    if (coalesced_size > (_tr3_repl_head_info[unit]->array_size - 1)) {
        /* Insert into free list 0 */
        coalesced_block_ptr->next = REPL_HEAD_FREE_LIST(unit, 0);
        REPL_HEAD_FREE_LIST(unit, 0) = coalesced_block_ptr;
    } else {
        coalesced_block_ptr->next = REPL_HEAD_FREE_LIST(unit, coalesced_size);
        REPL_HEAD_FREE_LIST(unit, coalesced_size) = coalesced_block_ptr;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_repl_head_block_alloc
 * Purpose:
 *      Allocate a free block of REPL_HEAD table entries.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      size - (IN) Size of free block requested.
 *      index - (OUT) Index of the first entry of the free block.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_tr3_repl_head_block_alloc(int unit, int size, int *index)
{
    int max_array_index;
    int i;
    int block_index, block_size;
    _tr3_repl_head_free_block_t *next_block_ptr;

    if (size == 0) {
        return BCM_E_PARAM;
    }
    if (NULL == index) {
        return BCM_E_PARAM;
    }

    max_array_index = _tr3_repl_head_info[unit]->array_size - 1;
    for (i = size; i <= max_array_index; i++) {
        if (NULL != REPL_HEAD_FREE_LIST(unit, i)) {
           block_index = REPL_HEAD_FREE_LIST(unit, i)->index; 
           block_size = REPL_HEAD_FREE_LIST(unit, i)->size; 
           next_block_ptr = REPL_HEAD_FREE_LIST(unit, i)->next; 
           sal_free(REPL_HEAD_FREE_LIST(unit, i));
           REPL_HEAD_FREE_LIST(unit, i) = next_block_ptr;

           /* If the obtained free block contains more entries
            * than requested, insert the remainder back into
            * the free list array.
            */
           if (block_size > size) {
               BCM_IF_ERROR_RETURN(_bcm_tr3_repl_head_block_free(unit,
                           block_index + size, block_size - size));
           }

           *index = block_index;
           return BCM_E_NONE;
        }
    }

    /* Get free block from free list 0 */
    if (NULL != REPL_HEAD_FREE_LIST(unit, 0)) {
        block_index = REPL_HEAD_FREE_LIST(unit, 0)->index;
        block_size = REPL_HEAD_FREE_LIST(unit, 0)->size;
        next_block_ptr = REPL_HEAD_FREE_LIST(unit, 0)->next;
        if (block_size < size) {
            /* Free blocks on list 0 should never be
             * smaller than requested size.
             */
            return BCM_E_INTERNAL;
        } 

        sal_free(REPL_HEAD_FREE_LIST(unit, 0));
        REPL_HEAD_FREE_LIST(unit, 0) = next_block_ptr;

        /* If the obtained free block contains more entries
         * than requested, insert the remainder back into
         * the free list array.
         */
        if (block_size > size) {
            BCM_IF_ERROR_RETURN(_bcm_tr3_repl_head_block_free(unit,
                        block_index + size, block_size - size));
        }

        *index = block_index;
        return BCM_E_NONE;
    }

    /* No free block of sufficient size can be found */
    return BCM_E_RESOURCE;
}

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      _bcm_tr3_repl_head_block_insert
 * Purpose:
 *      Insert a free block of REPL_HEAD table entries into free array list.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      index - (IN) Index of the first entry of the block.
 *      size - (IN) Size of block.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_tr3_repl_head_block_insert(int unit, int index, int size)
{
    _tr3_repl_head_free_block_t *block_ptr;

    block_ptr = sal_alloc(sizeof(_tr3_repl_head_free_block_t),
            "repl head free block");
    if (NULL == block_ptr) {
        return BCM_E_MEMORY;
    }
    block_ptr->index = index;
    block_ptr->size = size;
    if (size > (_tr3_repl_head_info[unit]->array_size - 1)) {
        /* Insert into free list 0 */
        block_ptr->next = REPL_HEAD_FREE_LIST(unit, 0);
        REPL_HEAD_FREE_LIST(unit, 0) = block_ptr;
    } else {
        block_ptr->next = REPL_HEAD_FREE_LIST(unit, size);
        REPL_HEAD_FREE_LIST(unit, size) = block_ptr;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_repl_head_block_used_set
 * Purpose:
 *      Mark a block of REPL_HEAD table entries as used.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      index - (IN) Index of the first entry of the block.
 *      size - (IN) Size of block.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_tr3_repl_head_block_used_set(int unit, int index, int size)
{
    int i;
    _tr3_repl_head_free_block_t *prev_block_ptr;
    _tr3_repl_head_free_block_t *block_ptr;
    int block_index, block_size, sub_block_size;

    for (i = 0; i < _tr3_repl_head_info[unit]->array_size; i++) {
        block_ptr = REPL_HEAD_FREE_LIST(unit, i);
        prev_block_ptr = NULL;
        while (NULL != block_ptr) {
            block_index = block_ptr->index;
            block_size = block_ptr->size;
            if ((index >= block_index) &&
                    ((index + size) <= (block_index + block_size))) {

                /* This free block contains the block to be marked as used.
                 * Remove this free block from linked list.
                 */
                if (block_ptr == REPL_HEAD_FREE_LIST(unit, i)) {
                    REPL_HEAD_FREE_LIST(unit, i) = block_ptr->next;
                } else {
                    /* 
                     * In the following line of code, Coverity thinks the
                     * prev_block_ptr may still be NULL when dereferenced.
                     * This situation will never occur because 
                     * if block_ptr is not pointing to the head of the 
                     * linked list, prev_block_ptr would not be NULL.
                     */
                    /* coverity[var_deref_op : FALSE] */
                    prev_block_ptr->next = block_ptr->next;
                }
                sal_free(block_ptr);

                /* This free block contains up to 3 sub-blocks: the sub-block
                 * to be marked as used, and the sub-blocks before and after
                 * it. The sub-blocks before and after the used sub-block
                 * need to be inserted back into the free list array.
                 */

                /* Insert the sub-block before the used sub-block back into
                 * the free list array.
                 */
                sub_block_size = index - block_index;
                if (sub_block_size > 0) {
                    BCM_IF_ERROR_RETURN
                        (_bcm_tr3_repl_head_block_insert(unit, block_index,
                                                         sub_block_size));
                }

                /* Insert the sub-block after the used sub-block back into
                 * the free list array.
                 */
                sub_block_size = (block_index + block_size) - (index + size);
                if (sub_block_size > 0) {
                    BCM_IF_ERROR_RETURN
                        (_bcm_tr3_repl_head_block_insert(unit, index + size,
                                                         sub_block_size));
                }

                return BCM_E_NONE;
            }

            prev_block_ptr = block_ptr;
            block_ptr = block_ptr->next;
        }
    }

    /* The block to be marked used is not found among the free blocks. */
    return BCM_E_NOT_FOUND;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      _bcm_tr3_repl_head_entry_info_get
 * Purpose:
 *      Get replication head table size and number of entries in the free block.
 * Parameters:
 *      unit - (IN) SOC unit number. 
 *      size - (out) Replication head table size.
 *      free - (out) Number of entries in the Replication head table.
 * Returns:
 *      BCM_E_xxx
 */
int
_bcm_tr3_repl_head_entry_info_get(int unit, int *free)
{
    int temp_free = 0;
    _tr3_repl_head_free_block_t *free_block;
    int i = 0;

    if (NULL == free) {
        return BCM_E_PARAM;
    } 

    IPMC_LOCK(unit);  
    if (_tr3_repl_head_info[unit] != NULL) {
        if (_tr3_repl_head_info[unit]->free_list_array != NULL) {
            for (i = 0; i < _tr3_repl_head_info[unit]->array_size; i++) {
                free_block = REPL_HEAD_FREE_LIST(unit, i);
                if (NULL != free_block){
                    temp_free = temp_free + free_block->size;
                }
            }
        }
    }
    IPMC_UNLOCK(unit);

    if (0 > temp_free){        
        return BCM_E_INTERNAL;
    }

    *free = temp_free;
    
    return BCM_E_NONE;
}


/* --------------------------------------------------------------
 * The following set of routines manage REPL_LIST table resource.
 * --------------------------------------------------------------
 */

/*
 * Function:
 *      _bcm_tr3_repl_list_entry_info_deinit
 * Purpose:
 *      De-initialize replication list entry info.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      None
 */
STATIC void 
_bcm_tr3_repl_list_entry_info_deinit(int unit)
{
    if (NULL != _tr3_repl_list_entry_info[unit]) {
        if (NULL != _tr3_repl_list_entry_info[unit]->bitmap_entries_used) {
            sal_free(_tr3_repl_list_entry_info[unit]->bitmap_entries_used);
            _tr3_repl_list_entry_info[unit]->bitmap_entries_used = NULL;
        }
        sal_free(_tr3_repl_list_entry_info[unit]);
        _tr3_repl_list_entry_info[unit] = NULL;
    }
}

/*
 * Function:
 *      _bcm_tr3_repl_list_entry_info_init
 * Purpose:
 *      Initialize replication list entry info.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_tr3_repl_list_entry_info_init(int unit)
{
    int alloc_size;

    if (NULL == _tr3_repl_list_entry_info[unit]) {
        _tr3_repl_list_entry_info[unit] =
            sal_alloc(sizeof(_tr3_repl_list_entry_info_t),
                    "repl_list_entry_info");
        if (NULL == _tr3_repl_list_entry_info[unit]) {
            _bcm_tr3_repl_list_entry_info_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_tr3_repl_list_entry_info[unit], 0,
            sizeof(_tr3_repl_list_entry_info_t));

    _tr3_repl_list_entry_info[unit]->num_entries = soc_mem_index_count(unit,
                MMU_REPL_LIST_TBLm);
    alloc_size = SHR_BITALLOCSIZE(_tr3_repl_list_entry_info[unit]->num_entries);
    if (NULL == _tr3_repl_list_entry_info[unit]->bitmap_entries_used) {
        _tr3_repl_list_entry_info[unit]->bitmap_entries_used =
            sal_alloc(alloc_size, "repl list bitmap_entries_used");
        if (NULL == _tr3_repl_list_entry_info[unit]->bitmap_entries_used) {
            _bcm_tr3_repl_list_entry_info_deinit(unit);
            return BCM_E_MEMORY;
        }
    }
    sal_memset(_tr3_repl_list_entry_info[unit]->bitmap_entries_used, 0,
            alloc_size);

    /* Reserve REPL_LIST table entry 0 */
    REPL_LIST_ENTRY_USED_SET(unit, 0);

    /* Clear entry 0 of REPL_LIST table */
    SOC_IF_ERROR_RETURN(soc_mem_write(unit, MMU_REPL_LIST_TBLm, MEM_BLOCK_ALL,
                0, soc_mem_entry_null(unit, MMU_REPL_LIST_TBLm)));

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_repl_list_entry_free
 * Purpose:
 *      Free a REPL_LIST table entry.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 *      entry_index - (IN)Index of the entry to be freed.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_tr3_repl_list_entry_free(int unit, int entry_index)
{
    if (!REPL_LIST_ENTRY_USED_GET(unit, entry_index)) {
        return BCM_E_INTERNAL;
    }

    REPL_LIST_ENTRY_USED_CLR(unit, entry_index);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_repl_list_entry_alloc
 * Purpose:
 *      Allocate a free entry from REPL_LIST table.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 *      entry_index - (OUT)Index of a free entry.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int 
_bcm_tr3_repl_list_entry_alloc(int unit, int *entry_index)
{
    int i;

    for (i = 0; i < _tr3_repl_list_entry_info[unit]->num_entries; i++) {
        if (!REPL_LIST_ENTRY_USED_GET(unit, i)) {
            *entry_index = i;
            REPL_LIST_ENTRY_USED_SET(unit, i);
            return BCM_E_NONE;
        }
    }

    *entry_index = -1;
    return BCM_E_RESOURCE;
}

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      _bcm_tr3_repl_list_entry_used_set
 * Purpose:
 *      Mark the REPL_LIST table entry as used.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 *      entry_index - (IN)Index of the entry to be marked.
 * Returns:
 *      BCM_E_xxx
 */
STATIC int
_bcm_tr3_repl_list_entry_used_set(int unit, int entry_index)
{
    REPL_LIST_ENTRY_USED_SET(unit, entry_index);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_repl_list_entry_used_get
 * Purpose:
 *      Get used status of a REPL_LIST table entry.
 * Parameters:
 *      unit - (IN)SOC unit number. 
 *      entry_index - (IN)Index of the entry.
 *      used - (OUT)Used status.
 * Returns:
 *      TRUE if used, else FALSE.
 */
STATIC int
_bcm_tr3_repl_list_entry_used_get(int unit, int entry_index)
{
    if (REPL_LIST_ENTRY_USED_GET(unit, entry_index)) {
        return TRUE;
    }
    return FALSE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */


#ifdef BCM_KATANA2_SUPPORT

/* Function:
 *      _bcm_kt2_repl_list_tbl_ptr_get
 * Purpose:
 *      Get the appropriate MMU replication tables and intended 
 *      member bitmap last bitmap fields for  for port
 * Parameters:
 *      unit       - StrataSwitch PCI device unit number.
 *      port       - (IN) port. 
 *      info       - (OUT) MMU group table info 
*                     mem, pbmp, index corresponding to the port.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_kt2_repl_list_tbl_ptr_get(int unit, bcm_port_t port,
                               _kt2_repl_grp_info_t *info)
{

    if (info == NULL)
    {
       return BCM_E_PARAM;
    }
    if ((port >= 0) && (port < 70))
    {
        info->mb_mem =  MMU_REPL_GRP_TBL0m;
        info->mbmp = MEMBER_BMP_PORTS_119_0f;
        info->lb_mem = MMU_REPL_GRP_TBL1m;
        info->lbmp =  LAST_BMP_PORTS_69_0f;
        info->mb_index = port;
        info->lb_index = port;
    } else if ((port >= 70) && (port < 120)) {
        info->mb_mem =  MMU_REPL_GRP_TBL0m;
        info->mbmp = MEMBER_BMP_PORTS_119_0f;
        info->lb_mem = MMU_REPL_GRP_TBL2m;
        info->lbmp =  LAST_BMP_PORTS_169_70f;
        info->mb_index = port;
        info->lb_index = port - 70;
    } else if ((port >= 120) && (port < 170)) {
        info->mb_mem =  MMU_REPL_GRP_TBL1m;
        info->mbmp = MEMBER_BMP_PORTS_169_120f;
        info->lb_mem = MMU_REPL_GRP_TBL2m;
        info->lbmp =  LAST_BMP_PORTS_169_70f;
        info->mb_index = port - 120;
        info->lb_index = port - 70;
    }

    return BCM_E_NONE;
}

/* Function:
 *      _bcm_kt2_repl_group_info_set
 * Purpose:
 *      Set replication group info.
 * Parameters:
 *      unit       - StrataSwitch PCI device unit number.
 *      repl_group - (IN) The replication group number.
 *      port       - (IN) port.
 *      mbmp       - (IN) Member bitmap.
 *      lbmp       - (IN) Last Member bitmap.
 *      head_index - (IN) Base pointer to REPL_HEAD table.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_kt2_repl_group_info_set(int unit, int repl_group, bcm_port_t port,
        soc_pbmp_t mbmp, soc_pbmp_t lbmp, int head_index)
{

    mmu_repl_grp_tbl0_entry_t repl_grp_tbl0_entry;
    mmu_repl_grp_tbl1_entry_t repl_grp_tbl1_entry;
    mmu_repl_grp_tbl2_entry_t repl_grp_tbl2_entry;
    _kt2_repl_grp_info_t kt2_repl_grp_info;
    int member_bitmap_width0;
    int member_bitmap_width1;
    soc_pbmp_t member_bitmap0, member_bitmap1;
    uint32 fldbuf[SOC_PBMP_WORD_MAX];
    int i;

    sal_memset(&repl_grp_tbl0_entry, 0, sizeof(repl_grp_tbl0_entry));
    sal_memset(&repl_grp_tbl1_entry, 0, sizeof(repl_grp_tbl1_entry));
    sal_memset(&repl_grp_tbl2_entry, 0, sizeof(repl_grp_tbl2_entry));
    sal_memset(&kt2_repl_grp_info, 0, sizeof(kt2_repl_grp_info));

    BCM_IF_ERROR_RETURN(_bcm_kt2_repl_list_tbl_ptr_get (unit, port,
               &kt2_repl_grp_info));
     
    member_bitmap_width0 =  soc_mem_field_length(unit,
                MMU_REPL_GRP_TBL0m, MEMBER_BMP_PORTS_119_0f);

    member_bitmap_width1 =  soc_mem_field_length(unit,
                MMU_REPL_GRP_TBL1m, MEMBER_BMP_PORTS_169_120f);

    sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));

    for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
        fldbuf[i] = SOC_PBMP_WORD_GET(lbmp, i);
    }
    if (port >=70) {
        soc_mem_field_set(unit, kt2_repl_grp_info.lb_mem,
           (uint32 *)&repl_grp_tbl2_entry, kt2_repl_grp_info.lbmp, fldbuf);
    } else {
        soc_mem_field_set(unit, kt2_repl_grp_info.lb_mem,
            (uint32 *)&repl_grp_tbl1_entry, kt2_repl_grp_info.lbmp, fldbuf);
    }

    soc_MMU_REPL_GRP_TBL2m_field32_set(unit, &repl_grp_tbl2_entry,
                        BASE_PTRf, head_index);

    SOC_PBMP_CLEAR(member_bitmap0);
    for (i = 0; i < member_bitmap_width0; i++) {
        if (SOC_PBMP_MEMBER(mbmp, i)) {
            SOC_PBMP_PORT_ADD(member_bitmap0, i);
        }
    }

    sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
    for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
        fldbuf[i] = SOC_PBMP_WORD_GET(member_bitmap0, i);
    }

    soc_MMU_REPL_GRP_TBL0m_field_set(unit, &repl_grp_tbl0_entry,
            MEMBER_BMP_PORTS_119_0f, fldbuf);

    SOC_PBMP_CLEAR(member_bitmap1);
    for (i = member_bitmap_width0;
         i < (member_bitmap_width0 + member_bitmap_width1);
         i++) {
        if (SOC_PBMP_MEMBER(mbmp, i)) {
            SOC_PBMP_PORT_ADD(member_bitmap1, (i - member_bitmap_width0));
        }
    }

    sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
    for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
        fldbuf[i] = SOC_PBMP_WORD_GET(member_bitmap1, i);
    }

    soc_MMU_REPL_GRP_TBL1m_field_set(unit, &repl_grp_tbl1_entry,
            MEMBER_BMP_PORTS_169_120f, fldbuf);

    SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_GRP_TBL0m(unit,
                    MEM_BLOCK_ALL, repl_group, &repl_grp_tbl0_entry));
    SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_GRP_TBL1m(unit,
                    MEM_BLOCK_ALL, repl_group, &repl_grp_tbl1_entry));
    SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_GRP_TBL2m(unit,
                    MEM_BLOCK_ALL, repl_group, &repl_grp_tbl2_entry));

    return BCM_E_NONE;
}

#endif /* BCM_KATANA2_SUPPORT*/

/* --------------------------------------------------------------
 * The following set of routines manage replication lists.
 * --------------------------------------------------------------
 */
/*
 * Function:
 *      bcm_tr3_ipmc_repl_detach
 * Purpose:
 *      Deinitialize replication.
 * Parameters:
 *      unit - SOC unit #
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_repl_detach(int unit)
{
    bcm_port_t          port;
    _bcm_repl_list_info_t *rli_current, *rli_free;
    bcm_pbmp_t            pbmp_all;

    BCM_PBMP_CLEAR(pbmp_all);
    BCM_PBMP_ASSIGN(pbmp_all, PBMP_ALL(unit));

#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        _bcm_kt2_subport_pbmp_update(unit, &pbmp_all);
    }
    if (SOC_IS_KATANA2(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_kt2_flexio_pbmp_update (unit, &pbmp_all));
    }
#endif

    if (_tr3_repl_info[unit] != NULL) {
        PBMP_ITER(pbmp_all, port) {
            if (_tr3_repl_info[unit]->port_info[port] != NULL) {
                if (_tr3_repl_info[unit]->
                        port_info[port]->intf_count != NULL) {
                    sal_free(_tr3_repl_info[unit]->
                            port_info[port]->intf_count);
                }
                sal_free(_tr3_repl_info[unit]->port_info[port]);
            }
        }

        if (_tr3_repl_info[unit]->repl_list_info != NULL) {
            rli_current = _tr3_repl_info[unit]->repl_list_info;
            while (rli_current != NULL) {
                rli_free = rli_current;
                rli_current = rli_current->next;
                sal_free(rli_free);
            }
        }

        if (_tr3_repl_info[unit]->l3_intf_next_hop_ipmc != NULL) {
            sal_free(_tr3_repl_info[unit]->l3_intf_next_hop_ipmc);
        }
        if (_tr3_repl_info[unit]->l3_intf_next_hop_trill != NULL) {
            sal_free(_tr3_repl_info[unit]->l3_intf_next_hop_trill);
        }

        sal_free(_tr3_repl_info[unit]);
        _tr3_repl_info[unit] = NULL;
    }

    _bcm_tr3_repl_head_info_deinit(unit);
    _bcm_tr3_repl_list_entry_info_deinit(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_ipmc_repl_init
 * Purpose:
 *      Initialize replication.
 * Parameters:
 *      unit - SOC unit #
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_repl_init(int unit)
{
    soc_info_t *si;
    int member_count;
    int phy_port, mmu_port;
    bcm_port_t port;
    int alloc_size;
    int i;
    int rv = BCM_E_NONE;
    soc_pbmp_t member_bitmap;
    int member_bitmap_index;
    uint32 fldbuf[SOC_PBMP_WORD_MAX];
    mmu_repl_group_entry_t repl_group_entry;
    uint32 regval;
    int num_lanes, count_width;
#ifdef BCM_KATANA2_SUPPORT 
    mmu_repl_map_tbl_entry_t map_entry;
    uint32 pp_port = 0;
#endif
    bcm_pbmp_t          pbmp_all;

    BCM_PBMP_CLEAR (pbmp_all);
    BCM_PBMP_ASSIGN(pbmp_all, PBMP_ALL(unit));

#if defined(BCM_KATANA2_SUPPORT)
    if (soc_feature(unit, soc_feature_linkphy_coe) ||
        soc_feature(unit, soc_feature_subtag_coe)) {
        _bcm_kt2_subport_pbmp_update(unit, &pbmp_all);
    }
    if (SOC_IS_KATANA2(unit)) {
        BCM_IF_ERROR_RETURN(_bcm_kt2_flexio_pbmp_update (unit, &pbmp_all));
    }
#endif

    bcm_tr3_ipmc_repl_detach(unit);

    if (NULL == _tr3_repl_info[unit]) {
       _tr3_repl_info[unit] = sal_alloc(sizeof(_tr3_repl_info_t), "repl info");
       if (NULL == _tr3_repl_info[unit]) {
           bcm_tr3_ipmc_repl_detach(unit);
           return BCM_E_MEMORY;
       }
    }
    sal_memset(_tr3_repl_info[unit], 0, sizeof(_tr3_repl_info_t));

    _tr3_repl_info[unit]->num_repl_groups = soc_mem_index_count(unit, L3_IPMCm);

    if (soc_feature(unit, soc_feature_static_repl_head_alloc)) {
        /* Each replication group is statically allocated a region
         * in REPL_HEAD table. The size of the region depends on the
         * maximum number of valid ports. Thus, the max number of
         * replication groups is limited to number of REPL_HEAD entries
         * divided by the max number of valid ports.
         */
        si = &SOC_INFO(unit);
        member_count = 0;
        PBMP_ITER(SOC_CONTROL(unit)->repl_eligible_pbmp, port) {
        if (SOC_IS_KATANA2(unit)) {
            phy_port = port;
            mmu_port = port;
        } else {
            phy_port = si->port_l2p_mapping[port];
            mmu_port = si->port_p2m_mapping[phy_port];
        }
            if ((!SOC_IS_KATANA2(unit)) && 
                ((mmu_port == 57) || (mmu_port == 59) || 
                 (mmu_port == 61) || (mmu_port == 62))) {
                /* No replication on MMU ports 57, 59, 61 and 62 */
                continue;
            }
            member_count++;
        }
        if (member_count > 0) {
            _tr3_repl_info[unit]->num_repl_groups =
                soc_mem_index_count(unit, MMU_REPL_HEAD_TBLm) / member_count;
            if (_tr3_repl_info[unit]->num_repl_groups > 
                    soc_mem_index_count(unit, L3_IPMCm)) {
                _tr3_repl_info[unit]->num_repl_groups =
                    soc_mem_index_count(unit, L3_IPMCm);
            }
        }
    }

    /* The total number of replication interfaces equals to the total
     * number of next hops. Unlike previous XGS3 devices, each L3 inteface
     * will be associated with a next hop index.
     */
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit))
    {
        _tr3_repl_info[unit]->num_intf = 
            (soc_mem_index_count(unit, EGR_L3_NEXT_HOPm) + 
             soc_mem_index_count(unit, EGR_L3_INTFm));
        rv = bcm_kt2_ipmc_repl_init(unit);
        if (BCM_FAILURE(rv)) {
            bcm_tr3_ipmc_repl_detach(unit);
            return rv;
        }              
    } else 
#endif /* BCM_KATANA2_SUPPORT */
    {
        _tr3_repl_info[unit]->num_intf = soc_mem_index_count(unit,
            EGR_L3_NEXT_HOPm);

        if (NULL == _tr3_repl_info[unit]->l3_intf_next_hop_ipmc) {
            _tr3_repl_info[unit]->l3_intf_next_hop_ipmc =
                sal_alloc(sizeof(int) * soc_mem_index_count(unit, EGR_L3_INTFm),
                        "l3_intf_next_hop_ipmc");
            if (NULL == _tr3_repl_info[unit]->l3_intf_next_hop_ipmc) {
                bcm_tr3_ipmc_repl_detach(unit);
                return BCM_E_MEMORY;
            }
        }
        for (i = 0; i < soc_mem_index_count(unit, EGR_L3_INTFm); i++) {
            _tr3_repl_info[unit]->l3_intf_next_hop_ipmc[i] = REPL_NH_INDEX_UNALLOCATED;
        }

        if (NULL == _tr3_repl_info[unit]->l3_intf_next_hop_trill) {
            _tr3_repl_info[unit]->l3_intf_next_hop_trill =
                sal_alloc(sizeof(int) * soc_mem_index_count(unit, EGR_L3_INTFm),
                        "l3_intf_next_hop_trill");
            if (NULL == _tr3_repl_info[unit]->l3_intf_next_hop_trill) {
                bcm_tr3_ipmc_repl_detach(unit);
                return BCM_E_MEMORY;
            }
        }
        for (i = 0; i < soc_mem_index_count(unit, EGR_L3_INTFm); i++) {
            _tr3_repl_info[unit]->l3_intf_next_hop_trill[i] = -1;
        }

    }

    /* PBMP_ITER(PBMP_ALL(unit), port)  */
    SOC_PBMP_ITER(pbmp_all, port) { 
        if (NULL == _tr3_repl_info[unit]->port_info[port]) {
            _tr3_repl_info[unit]->port_info[port] =
                sal_alloc(sizeof(_tr3_repl_port_info_t), "repl port info");
            if (NULL == _tr3_repl_info[unit]->port_info[port]) {
                bcm_tr3_ipmc_repl_detach(unit);
                return BCM_E_MEMORY;
            }
        }
        sal_memset(_tr3_repl_info[unit]->port_info[port], 0,
                sizeof(_tr3_repl_port_info_t));

        alloc_size = sizeof(int) * _tr3_repl_info[unit]->num_repl_groups;
        if (NULL == _tr3_repl_info[unit]->port_info[port]->intf_count) {
            _tr3_repl_info[unit]->port_info[port]->intf_count = 
                sal_alloc(alloc_size, "repl port intf count");
            if (NULL == _tr3_repl_info[unit]->port_info[port]->intf_count) {
                bcm_tr3_ipmc_repl_detach(unit);
                return BCM_E_MEMORY;
            }
        }
        sal_memset(_tr3_repl_info[unit]->port_info[port]->intf_count, 0,
                alloc_size);
    }


    rv = _bcm_tr3_repl_head_info_init(unit);
    if (BCM_FAILURE(rv)) {
        bcm_tr3_ipmc_repl_detach(unit);
        return rv;
    }

    rv = _bcm_tr3_repl_list_entry_info_init(unit);
    if (BCM_FAILURE(rv)) {
        bcm_tr3_ipmc_repl_detach(unit);
        return rv;
    }

    if (!SOC_WARM_BOOT(unit)) {
        if (soc_feature(unit, soc_feature_static_repl_head_alloc)) {

            /* Set MEMBER_BITMAP and LAST_BITMAP fields */
            si = &SOC_INFO(unit);
            SOC_PBMP_CLEAR(member_bitmap);
            member_count = 0;
            PBMP_ITER(SOC_CONTROL(unit)->repl_eligible_pbmp, port) {
                phy_port = si->port_l2p_mapping[port];
                mmu_port = si->port_p2m_mapping[phy_port];

                if ((mmu_port == 57) || (mmu_port == 59) ||
                    (mmu_port == 61) || (mmu_port == 62)) {
                    /* No replication on MMU ports 57, 59, 61, and 62 */
                    continue;
                } else if (mmu_port == 60) {
                    member_bitmap_index = 59;
                } else {
                    member_bitmap_index = mmu_port;
                }

                SOC_PBMP_PORT_ADD(member_bitmap, member_bitmap_index);
                member_count++;
            }

 
            for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
                fldbuf[i] = SOC_PBMP_WORD_GET(member_bitmap, i);
            }
 
            sal_memset(&repl_group_entry, 0, sizeof(mmu_repl_group_entry_t));
            soc_MMU_REPL_GROUPm_field_set(unit, &repl_group_entry,
                      MEMBER_BITMAPf, fldbuf);
            soc_MMU_REPL_GROUPm_field_set(unit, &repl_group_entry,
                      LAST_BITMAPf, fldbuf);

            /* Set HEAD_INDEXf field */
            for (i = 0; i < _tr3_repl_info[unit]->num_repl_groups; i++) {
                soc_MMU_REPL_GROUPm_field32_set(unit, &repl_group_entry,
                          HEAD_INDEXf, i * member_count);
                SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_GROUPm(unit, MEM_BLOCK_ALL,
                              i, &repl_group_entry));
            }

        } else {
            /* Clear replication group table */
            if (soc_mem_is_valid(unit, MMU_REPL_GROUP_INFO0m) &&
                soc_mem_is_valid(unit, MMU_REPL_GROUP_INFO1m)) {
                SOC_IF_ERROR_RETURN
                    (soc_mem_clear(unit, MMU_REPL_GROUP_INFO0m, MEM_BLOCK_ALL,
                                   0));
                SOC_IF_ERROR_RETURN
                    (soc_mem_clear(unit, MMU_REPL_GROUP_INFO1m, MEM_BLOCK_ALL,
                                   0));
            } else {
#ifdef  BCM_KATANA2_SUPPORT 
                if (SOC_IS_KATANA2(unit)) {
                    SOC_IF_ERROR_RETURN(soc_mem_clear(unit,
                                        MMU_REPL_GRP_TBL0m, MEM_BLOCK_ALL, 0));
                    SOC_IF_ERROR_RETURN(soc_mem_clear(unit,
                                        MMU_REPL_GRP_TBL1m, MEM_BLOCK_ALL, 0));
                    SOC_IF_ERROR_RETURN(soc_mem_clear(unit, 
                                        MMU_REPL_GRP_TBL2m, MEM_BLOCK_ALL, 0));

                    sal_memset(&map_entry, 0, sizeof(mmu_repl_map_tbl_entry_t));
                    /* MMU_REPL_MAP_TBL table is programmed to map 256 - 297 
                     * indices to pp ports 1-41 
                     */
                    for (i = 256; i < 298; i++) {
                        pp_port = i - 256;
                        soc_MMU_REPL_MAP_TBLm_field_set(unit, &map_entry, 
                                                       DEST_PPPf, &pp_port);
                        SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_MAP_TBLm(unit,
                                   MEM_BLOCK_ALL, i, &map_entry));
                    }  
                } else 
#endif /* BCM_KATANA2_SUPPORT */
                { 
                    SOC_IF_ERROR_RETURN
                        (soc_mem_clear(unit, MMU_REPL_GROUPm, MEM_BLOCK_ALL, 0));

                }
            }
 
            /* Clear replication group initial count table */
            if (soc_mem_is_valid(unit, MMU_REPL_GROUP_INITIAL_COPY_COUNTm)) {
                SOC_IF_ERROR_RETURN
                    (soc_mem_clear(unit, MMU_REPL_GROUP_INITIAL_COPY_COUNTm,
                                   MEM_BLOCK_ALL, 0));
            }

            /* Initialize PORT_INITIAL_COPY_COUNT_WIDTH registers */
            if (SOC_REG_IS_VALID(unit, PORT_INITIAL_COPY_COUNT_WIDTHr)) {
                PBMP_ITER(PBMP_ALL(unit), port) {
                    if (IS_CPU_PORT(unit, port) || IS_LB_PORT(unit, port)) {
                        count_width = 2;
                    } else {
                        num_lanes = SOC_INFO(unit).port_num_lanes[port];
                        switch (num_lanes) {
                            case 1: count_width = 1;
                                    break;
                            case 2: count_width = 2;
                                    break;
                            case 4: count_width = 3;
                                    break;
                            default: count_width = 0;
                                     break;
                        }
                    }
                    regval = 0;
                    soc_reg_field_set(unit, PORT_INITIAL_COPY_COUNT_WIDTHr,
                            &regval, BIT_WIDTHf,
                            count_width ? (count_width - 1) : 0);
                    SOC_IF_ERROR_RETURN
                        (WRITE_PORT_INITIAL_COPY_COUNT_WIDTHr(unit, port,
                                                              regval));
                }
            }
        }
    } 

    return rv;
}

/* Function:
 *	_bcm_tr3_repl_head_ptr_replace
 * Purpose:
 *      Replace REPL_HEAD pointers in MMU multicast queues.
 * Parameters:
 *	unit       - StrataSwitch PCI device unit number.
 *	old_head_index    - Starting index of old REPL_HEAD block.
 *	old_member_bitmap - Old replication group member bitmap.
 *	new_head_index    - Starting index of new REPL_HEAD block.
 *	new_member_bitmap - New replication group member bitmap.
 *	new_last_bitmap   - New replication group last indication bitmap.
 * Returns:
 *	BCM_E_XXX
 */
STATIC int
_bcm_tr3_repl_head_ptr_replace(int unit, int old_head_index,
        soc_pbmp_t old_member_bitmap, int new_head_index,
        soc_pbmp_t new_member_bitmap, soc_pbmp_t new_last_bitmap)
{
    int member_bitmap_width;
    int old_member_id, new_member_id;
    int i;
    int old_repl_head_ptr, new_repl_head_ptr;
    int new_l3_last;
    int mmu_port;
    uint64 regval_64;
    int timeout_usec;
    soc_timeout_t to;
    int replace_done;
    soc_info_t *si;
    bcm_port_t phy_port, port;


    si = &SOC_INFO(unit);

    member_bitmap_width = soc_mem_field_length(unit, MMU_REPL_GROUPm,
            MEMBER_BITMAPf);

    old_member_id = 0;
    new_member_id = 0;
    for (i = 0; i < member_bitmap_width; i++) {
        if (SOC_PBMP_MEMBER(old_member_bitmap, i)) {

            /* Compute index to old block of REPL_HEAD entries */
            old_repl_head_ptr = old_head_index + old_member_id;

            if (SOC_PBMP_MEMBER(new_member_bitmap, i)) {
                /* Compute index to new block of REPL_HEAD entries */
                new_repl_head_ptr = new_head_index + new_member_id;
                new_l3_last = SOC_PBMP_MEMBER(new_last_bitmap, i) ? 1 : 0;
            } else {
                new_repl_head_ptr = 0;
                new_l3_last = 1;
            }

            /* Convert member bitmap index to MMU port */
            if (i == 59) {
                mmu_port = 60;
            } else {
                mmu_port = i;
            }

            /* Convert MMU port to logical port */
            phy_port = si->port_m2p_mapping[mmu_port];
            port = si->port_p2l_mapping[phy_port];

            /* Replace old_repl_head_ptr with new_repl_head_ptr
             * in multicast queues.
             */
            COMPILER_64_ZERO(regval_64);
            soc_reg64_field32_set(unit, REPL_HEAD_PTR_REPLACEr, &regval_64,
                    OLD_REPL_HEAD_PTRf, old_repl_head_ptr);
            soc_reg64_field32_set(unit, REPL_HEAD_PTR_REPLACEr, &regval_64,
                    NEW_REPL_HEAD_PTRf, new_repl_head_ptr);
            soc_reg64_field32_set(unit, REPL_HEAD_PTR_REPLACEr, &regval_64,
                    NEW_L3_LASTf, new_l3_last);
            soc_reg64_field32_set(unit, REPL_HEAD_PTR_REPLACEr, &regval_64,
                    REPLACE_ENf, 1);
            SOC_IF_ERROR_RETURN(soc_reg_set(unit, REPL_HEAD_PTR_REPLACEr,
                        port, 0, regval_64));

            /* Wait for replacement done */
            timeout_usec = 1000000;
            soc_timeout_init(&to, timeout_usec, 0);
            while (TRUE) {
                SOC_IF_ERROR_RETURN(soc_reg_get(unit, REPL_HEAD_PTR_REPLACEr,
                            port, 0, &regval_64));
                replace_done = soc_reg64_field32_get(unit,
                        REPL_HEAD_PTR_REPLACEr, regval_64, REPLACE_DONEf);
                if (replace_done) {
                    break;
                }

                if (soc_timeout_check(&to)) {
                    return BCM_E_TIMEOUT;
                }
            }

            old_member_id++;
        }

        if (SOC_PBMP_MEMBER(new_member_bitmap, i)) {
            new_member_id++;
        }
    }

    return BCM_E_NONE;
}

#ifdef BCM_TRIDENT2_SUPPORT

/* Function:
 *	_bcm_td2_repl_initial_copy_count_set
 * Purpose:
 *      Set replication initial copy count.
 * Parameters:
 *	unit       - StrataSwitch PCI device unit number.
 *	repl_group - The replication group number.
 *	port       - Port.
 *	intf_count - Replication list's interface count.
 * Returns:
 *	BCM_E_XXX
 */
STATIC int
_bcm_td2_repl_initial_copy_count_set(int unit, int repl_group, bcm_port_t port,
                int intf_count)
{
    soc_info_t *si;
    bcm_port_t phy_port;
    uint32 regval;
    int count_width, copy_count;
    mmu_repl_group_initial_copy_count_entry_t initial_copy_count_entry;
    uint32 count_mask, count_shift;
    uint32 copy_count_buf[2];

    si = &SOC_INFO(unit);
    if (SOC_IS_KATANA2(unit)) {
        phy_port = port;
    } else {
        phy_port = si->port_l2p_mapping[port];
    }

    SOC_IF_ERROR_RETURN(READ_PORT_INITIAL_COPY_COUNT_WIDTHr(unit,
                port, &regval));
    count_width = 1 + soc_reg_field_get(unit,
            PORT_INITIAL_COPY_COUNT_WIDTHr, regval, BIT_WIDTHf);
    copy_count = 0;
    switch (count_width) {
        case 1:
            if (intf_count <= 1) {
                copy_count = intf_count;
            } 
            break;
        case 2:
            if (intf_count <= 3) {
                copy_count = intf_count;
            }
            break;
        case 3:
            if (intf_count <= 5) {
                copy_count = intf_count;
            }
            break;
        default:
            return BCM_E_INTERNAL;
            break;
    }

    SOC_IF_ERROR_RETURN(READ_MMU_REPL_GROUP_INITIAL_COPY_COUNTm(unit,
                MEM_BLOCK_ANY, repl_group, &initial_copy_count_entry));
    if (IS_LB_PORT(unit, port)) {
        soc_MMU_REPL_GROUP_INITIAL_COPY_COUNTm_field32_set(unit,
                &initial_copy_count_entry, ICC_BIT_131_130f, copy_count);
    } else if (IS_CPU_PORT(unit, port)) {
        soc_MMU_REPL_GROUP_INITIAL_COPY_COUNTm_field32_set(unit,
                &initial_copy_count_entry, ICC_BIT_129_128f, copy_count);
    } else if ((phy_port <= 128) && (phy_port >= 65)) {
        soc_MMU_REPL_GROUP_INITIAL_COPY_COUNTm_field_get(unit,
                &initial_copy_count_entry, ICC_BIT_127_64f, copy_count_buf);
        count_mask = (1 << count_width) - 1;
        count_shift = (phy_port - 65) % 32;
        copy_count_buf[(phy_port - 65) / 32] &= ~(count_mask << count_shift);
        copy_count_buf[(phy_port - 65) / 32] |= (copy_count << count_shift);
        soc_MMU_REPL_GROUP_INITIAL_COPY_COUNTm_field_set(unit,
                &initial_copy_count_entry, ICC_BIT_127_64f, copy_count_buf);
    } else if (phy_port <= 64) {
        soc_MMU_REPL_GROUP_INITIAL_COPY_COUNTm_field_get(unit,
                &initial_copy_count_entry, ICC_BIT_63_0f, copy_count_buf);
        count_mask = (1 << count_width) - 1;
        count_shift = (phy_port - 1) % 32;
        copy_count_buf[(phy_port - 1)/32] &= ~(count_mask << count_shift);
        copy_count_buf[(phy_port - 1)/32] |= (copy_count << count_shift);
        soc_MMU_REPL_GROUP_INITIAL_COPY_COUNTm_field_set(unit,
                &initial_copy_count_entry, ICC_BIT_63_0f, copy_count_buf);
    } else {
        return BCM_E_PORT;
    }

    SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_GROUP_INITIAL_COPY_COUNTm(unit,
                MEM_BLOCK_ALL, repl_group, &initial_copy_count_entry));

    return BCM_E_NONE;
}

/* Function:
 *	_bcm_td2_repl_group_info_set
 * Purpose:
 *      Set replication group info.
 * Parameters:
 *	unit       - StrataSwitch PCI device unit number.
 *	repl_group - The replication group number.
 *	member_bitmap - Member bitmap.
 *	head_index    - Base pointer to REPL_HEAD table.
 * Returns:
 *	BCM_E_XXX
 */
STATIC int
_bcm_td2_repl_group_info_set(int unit, int repl_group,
        soc_pbmp_t member_bitmap, int head_index)
{
    int rv = BCM_E_NONE;
    soc_pbmp_t xpipe_member_bitmap, ypipe_member_bitmap;
    int xpipe_member_bitmap_width, ypipe_member_bitmap_width;
    int xpipe_member_count, ypipe_member_count;
    int i;
    uint32 fldbuf[SOC_PBMP_WORD_MAX];
    mmu_repl_group_info0_entry_t repl_group_info0_entry;
    mmu_repl_group_info1_entry_t repl_group_info1_entry;
    soc_info_t *si;
    soc_pbmp_t l3_pbmp;
    bcm_port_t mmu_port, phy_port, port;
    int cut_through_eligible;
    ipmc_entry_t l3_ipmc_entry;

    /* Configure X-pipe REPL_GROUP table */

    SOC_PBMP_CLEAR(xpipe_member_bitmap);
    xpipe_member_bitmap_width = soc_mem_field_length(unit,
                MMU_REPL_GROUP_INFO0m, PIPE_MEMBER_BMPf);
    xpipe_member_count = 0;
    for (i = 0; i < xpipe_member_bitmap_width; i++) {
        if (SOC_PBMP_MEMBER(member_bitmap, i)) {
            SOC_PBMP_PORT_ADD(xpipe_member_bitmap, i);
            xpipe_member_count++;
        }
    }
    for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
        fldbuf[i] = SOC_PBMP_WORD_GET(xpipe_member_bitmap, i);
    }
    sal_memset(&repl_group_info0_entry, 0, sizeof(repl_group_info0_entry));
    soc_MMU_REPL_GROUP_INFO0m_field_set(unit, &repl_group_info0_entry,
            PIPE_MEMBER_BMPf, fldbuf);
    soc_MMU_REPL_GROUP_INFO0m_field32_set(unit, &repl_group_info0_entry,
            PIPE_BASE_PTRf, head_index);
    SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_GROUP_INFO0m(unit, MEM_BLOCK_ALL,
                repl_group, &repl_group_info0_entry));

    /* Configure Y-pipe REPL_GROUP table */

    SOC_PBMP_CLEAR(ypipe_member_bitmap);
    ypipe_member_bitmap_width = soc_mem_field_length(unit,
                MMU_REPL_GROUP_INFO1m, PIPE_MEMBER_BMPf);
    ypipe_member_count = 0;
    for (i = xpipe_member_bitmap_width;
            i < (xpipe_member_bitmap_width + ypipe_member_bitmap_width);
            i++) {
        if (SOC_PBMP_MEMBER(member_bitmap, i)) {
            SOC_PBMP_PORT_ADD(ypipe_member_bitmap,
                    (i - xpipe_member_bitmap_width));
            ypipe_member_count++;
        }
    }
    for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
        fldbuf[i] = SOC_PBMP_WORD_GET(ypipe_member_bitmap, i);
    }
    sal_memset(&repl_group_info1_entry, 0, sizeof(repl_group_info1_entry));
    soc_MMU_REPL_GROUP_INFO1m_field_set(unit, &repl_group_info1_entry,
            PIPE_MEMBER_BMPf, fldbuf);
    if (ypipe_member_count > 0) {
        soc_MMU_REPL_GROUP_INFO1m_field32_set(unit, &repl_group_info1_entry,
                PIPE_BASE_PTRf, (head_index + xpipe_member_count));
    } else {
        soc_MMU_REPL_GROUP_INFO1m_field32_set(unit, &repl_group_info1_entry,
                PIPE_BASE_PTRf, 0);
    }
    SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_GROUP_INFO1m(unit, MEM_BLOCK_ALL,
                repl_group, &repl_group_info1_entry));

    /* When MMU is in cut-through mode, REPL_GROUP table is not read.
     * Instead, MMU gets the REPL_HEAD base pointer and L3 bitmap from
     * the L3_IPMC table in ingress pipeline.
     */
    if (soc_mem_field_valid(unit, L3_IPMCm, REPL_HEAD_BASE_PTRf)) {
        /* Convert MMU member bitmap to logical port bitmap.
         * X-pipe contains mmu ports 0-52. Y-pipe contains mmu ports 64-116.
         */  
        si = &SOC_INFO(unit);
        SOC_PBMP_CLEAR(l3_pbmp);
        for (i = 0; i < xpipe_member_bitmap_width; i++) {
            if (SOC_PBMP_MEMBER(xpipe_member_bitmap, i)) {
                mmu_port = i;
                phy_port = si->port_m2p_mapping[mmu_port];
                port = si->port_p2l_mapping[phy_port];
                SOC_PBMP_PORT_ADD(l3_pbmp, port);
            }
        }
        for (i = 0; i < ypipe_member_bitmap_width; i++) {
            if (SOC_PBMP_MEMBER(ypipe_member_bitmap, i)) {
                mmu_port = i + 64;
                phy_port = si->port_m2p_mapping[mmu_port];
                port = si->port_p2l_mapping[phy_port];
                SOC_PBMP_PORT_ADD(l3_pbmp, port);
            }
        }

        /* Determine cut-through eligibility. An IPMC group is eligible
         * for cut-through only if there is a single replication per
         * L3 port.
         */
        cut_through_eligible = TRUE;
        SOC_PBMP_ITER(l3_pbmp, port) {
            if (REPL_PORT_GROUP_INTF_COUNT(unit, port, repl_group) > 1) {
                cut_through_eligible = FALSE;
                break;
            }
        }

        /* Write to L3_IPMC table */
        soc_mem_lock(unit, L3_IPMCm);
        rv = READ_L3_IPMCm(unit, MEM_BLOCK_ANY, repl_group, &l3_ipmc_entry);
        if (BCM_SUCCESS(rv)) {
            if (!soc_L3_IPMCm_field32_get(unit, &l3_ipmc_entry, VALIDf)) {
                sal_memset(&l3_ipmc_entry, 0, sizeof(ipmc_entry_t));
                soc_L3_IPMCm_field32_set(unit, &l3_ipmc_entry, VALIDf, 1);
            }
            soc_L3_IPMCm_field32_set(unit, &l3_ipmc_entry, REPL_HEAD_BASE_PTRf,
                    head_index);
            soc_mem_pbmp_field_set(unit, L3_IPMCm, &l3_ipmc_entry, L3_BITMAPf,
                    &l3_pbmp);
            soc_L3_IPMCm_field32_set(unit, &l3_ipmc_entry, DO_NOT_CUT_THROUGHf,
                    !cut_through_eligible);
            rv = WRITE_L3_IPMCm(unit, MEM_BLOCK_ALL, repl_group, &l3_ipmc_entry);
        }
        soc_mem_unlock(unit, L3_IPMCm);
    }

    return rv;
}

#endif /* BCM_TRIDENT2_SUPPORT */

/* Function:
 *	_bcm_tr3_repl_list_start_ptr_set
 * Purpose:
 *      Set replication list start pointer for given (repl_group, port).
 * Parameters:
 *	unit       - StrataSwitch PCI device unit number.
 *	repl_group - The replication group number.
 *	port       - Port.
 *	start_ptr  - Replication list's start pointer.
 *	intf_count - Replication list's interface count.
 * Returns:
 *	BCM_E_XXX
 */
STATIC int
_bcm_tr3_repl_list_start_ptr_set(int unit, int repl_group, bcm_port_t port,
                int start_ptr, int intf_count)
{
    int add_member;
    soc_info_t *si;
    bcm_port_t phy_port, mmu_port;
    int member_bitmap_index = 0;
    mmu_repl_group_entry_t repl_group_entry;
    uint32 fldbuf[SOC_PBMP_WORD_MAX];
    soc_pbmp_t old_member_bitmap, old_last_bitmap;
    soc_pbmp_t new_member_bitmap, new_last_bitmap;
    int i;
    int old_member_count, new_member_count;
    int member_id, member, old_member_id, new_member_id;
    int old_head_index, new_head_index;
    mmu_repl_head_tbl_entry_t repl_head_entry, old_repl_head_entry;
#ifdef BCM_KATANA2_SUPPORT
    mmu_repl_grp_tbl0_entry_t repl_grp_tbl0_entry;
    mmu_repl_grp_tbl1_entry_t repl_grp_tbl1_entry;
    mmu_repl_grp_tbl2_entry_t repl_grp_tbl2_entry;
    _kt2_repl_grp_info_t kt2_repl_grp_info;    
    int last_member_bitmap_index = 0;
    soc_pbmp_t member_bitmap0, member_bitmap1;
    int member_bitmap_width0 = 0;
    int remap_grp_id = 0;
#endif
    int rv = BCM_E_NONE;
    int reserved_resources_used = 0, loop, num_of_ports = 0;

    if (start_ptr > 0) {
        add_member = TRUE;
    } else {
        add_member = FALSE;
    }
   
    si = &SOC_INFO(unit);
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {

        sal_memset(&repl_grp_tbl0_entry, 0, sizeof(mmu_repl_grp_tbl0_entry_t));
        sal_memset(&repl_grp_tbl1_entry, 0, sizeof(mmu_repl_grp_tbl1_entry_t));
        sal_memset(&repl_grp_tbl2_entry, 0, sizeof(mmu_repl_grp_tbl2_entry_t));
        sal_memset(&kt2_repl_grp_info, 0, sizeof(kt2_repl_grp_info));

        phy_port = port;
        mmu_port = port;
        /* Get the REMAP ID for IPMC index if any */
        BCM_IF_ERROR_RETURN
        (_bcm_kt_ipmc_mmu_mc_remap_ptr(unit, repl_group, &remap_grp_id, FALSE));


        BCM_IF_ERROR_RETURN(_bcm_kt2_repl_list_tbl_ptr_get (unit, mmu_port,
               &kt2_repl_grp_info));      

        member_bitmap_index = mmu_port;
        last_member_bitmap_index = kt2_repl_grp_info.lb_index;

        member_bitmap_width0 =  soc_mem_field_length(unit,
                MMU_REPL_GRP_TBL0m, MEMBER_BMP_PORTS_119_0f);

    } else 
#endif /*BCM_KATANA2_SUPPORT */
    {
        phy_port = si->port_l2p_mapping[port];
        mmu_port = si->port_p2m_mapping[phy_port];
    }
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_split_repl_group_table)) {
        int xpipe_member_bitmap_width;
        mmu_repl_group_info0_entry_t repl_group_info0_entry;
        mmu_repl_group_info1_entry_t repl_group_info1_entry;
        soc_pbmp_t xpipe_member_bitmap, ypipe_member_bitmap;

        /* In Trident2, REPL_GROUP table is split into 2 halves,
         * one for each pipeline. The X-pipe REPL_GROUP table's
         * member bitmap contains MMU ports 0-52. The Y-pipe REPL_GROUP
         * table's member bitmap contains MMU ports 64-116.
         * These two member bitmaps will be concatenated to form a
         * single 106-bit member bitmap.
         */
        xpipe_member_bitmap_width = soc_mem_field_length(unit,
                MMU_REPL_GROUP_INFO0m, PIPE_MEMBER_BMPf);
        if (mmu_port >= 64) {
            member_bitmap_index = mmu_port - 64 + xpipe_member_bitmap_width;
        } else {
            member_bitmap_index = mmu_port;
        }

        SOC_IF_ERROR_RETURN(READ_MMU_REPL_GROUP_INFO0m(unit, MEM_BLOCK_ANY,
                    repl_group, &repl_group_info0_entry));
        sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
        soc_MMU_REPL_GROUP_INFO0m_field_get(unit, &repl_group_info0_entry,
                PIPE_MEMBER_BMPf, fldbuf);
        for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
            SOC_PBMP_WORD_SET(xpipe_member_bitmap, i, fldbuf[i]);
        }
        old_head_index = soc_MMU_REPL_GROUP_INFO0m_field32_get(unit,
                &repl_group_info0_entry, PIPE_BASE_PTRf);

        SOC_IF_ERROR_RETURN(READ_MMU_REPL_GROUP_INFO1m(unit, MEM_BLOCK_ANY,
                    repl_group, &repl_group_info1_entry));
        sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
        soc_MMU_REPL_GROUP_INFO1m_field_get(unit, &repl_group_info1_entry,
                PIPE_MEMBER_BMPf, fldbuf);
        for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
            SOC_PBMP_WORD_SET(ypipe_member_bitmap, i, fldbuf[i]);
        }

        SOC_PBMP_CLEAR(old_member_bitmap);
        SOC_PBMP_OR(old_member_bitmap, xpipe_member_bitmap);
        SOC_PBMP_ITER(ypipe_member_bitmap, member) {
            SOC_PBMP_PORT_ADD(old_member_bitmap,
                    (member + xpipe_member_bitmap_width));
        }

        SOC_PBMP_ASSIGN(new_member_bitmap, old_member_bitmap);
        SOC_PBMP_COUNT(old_member_bitmap, old_member_count);

    } else
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {

        /* Read all the group Tables 
         * read all the member_bitmap and concatenate to get 
         *  the member_bitmap for all the ports
         */

        SOC_IF_ERROR_RETURN(READ_MMU_REPL_GRP_TBL0m(unit, MEM_BLOCK_ANY,
                        remap_grp_id, &repl_grp_tbl0_entry));
       
        SOC_IF_ERROR_RETURN(READ_MMU_REPL_GRP_TBL1m(unit, MEM_BLOCK_ANY,
                        remap_grp_id, &repl_grp_tbl1_entry));
 
        SOC_IF_ERROR_RETURN(READ_MMU_REPL_GRP_TBL2m(unit, MEM_BLOCK_ANY,
                        remap_grp_id, &repl_grp_tbl2_entry));

        sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));            
        soc_MMU_REPL_GRP_TBL0m_field_get(unit, &repl_grp_tbl0_entry,
                MEMBER_BMP_PORTS_119_0f, fldbuf);
        for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
            SOC_PBMP_WORD_SET(member_bitmap0, i, fldbuf[i]);
        }

        sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
        soc_MMU_REPL_GRP_TBL1m_field_get(unit, &repl_grp_tbl1_entry,
                MEMBER_BMP_PORTS_169_120f, fldbuf);
        for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
            SOC_PBMP_WORD_SET(member_bitmap1, i, fldbuf[i]);
        }
   
        SOC_PBMP_CLEAR(old_member_bitmap);
        SOC_PBMP_OR(old_member_bitmap, member_bitmap0);
        SOC_PBMP_ITER(member_bitmap1, member) {
            SOC_PBMP_PORT_ADD(old_member_bitmap,
                    (member + member_bitmap_width0));
        }

        SOC_PBMP_ASSIGN(new_member_bitmap, old_member_bitmap);
        SOC_PBMP_COUNT(old_member_bitmap, old_member_count); 

        sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
        if (mmu_port >= 70)
        {
            soc_mem_field_get(unit, kt2_repl_grp_info.lb_mem, 
               (uint32 *)&repl_grp_tbl2_entry, kt2_repl_grp_info.lbmp, fldbuf);
        } else {
            soc_mem_field_get(unit, kt2_repl_grp_info.lb_mem,
               (uint32 *)&repl_grp_tbl1_entry, kt2_repl_grp_info.lbmp, fldbuf);
        }
    
        for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
            SOC_PBMP_WORD_SET(old_last_bitmap, i, fldbuf[i]);
        }

        SOC_PBMP_ASSIGN(new_last_bitmap, old_last_bitmap);

        old_head_index = soc_MMU_REPL_GRP_TBL2m_field32_get(unit,
                    &repl_grp_tbl2_entry, BASE_PTRf);

    } else
#endif /* BCM_KATANA2_SUPPORT*/
    {
         /* Triumph3 MMU does not support replication on
          * ports 57, 59, 61 and 62 */
        if ((mmu_port == 57) || (mmu_port == 59) || 
            (mmu_port == 61) || (mmu_port == 62)) {
            return BCM_E_PORT;
        } else if (mmu_port == 60) {
            member_bitmap_index = 59;
        } else {
            member_bitmap_index = mmu_port;
        } 
        SOC_IF_ERROR_RETURN(READ_MMU_REPL_GROUPm(unit, MEM_BLOCK_ANY, 
                        repl_group,&repl_group_entry));

        sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
        soc_MMU_REPL_GROUPm_field_get(unit, &repl_group_entry, MEMBER_BITMAPf,
                    fldbuf);
        for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
            SOC_PBMP_WORD_SET(old_member_bitmap, i, fldbuf[i]);
        }
        SOC_PBMP_ASSIGN(new_member_bitmap, old_member_bitmap);
        SOC_PBMP_COUNT(old_member_bitmap, old_member_count);

        sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
        soc_MMU_REPL_GROUPm_field_get(unit, &repl_group_entry, LAST_BITMAPf,
                    fldbuf);
        for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
            SOC_PBMP_WORD_SET(old_last_bitmap, i, fldbuf[i]);
        }
        SOC_PBMP_ASSIGN(new_last_bitmap, old_last_bitmap);
        old_head_index = soc_MMU_REPL_GROUPm_field32_get(unit, 
                    &repl_group_entry, HEAD_INDEXf);
    }

    if (add_member) {

        /* Update REPL_HEAD table */

        sal_memset(&repl_head_entry, 0, sizeof(mmu_repl_head_tbl_entry_t));
        soc_MMU_REPL_HEAD_TBLm_field32_set(unit, &repl_head_entry,
                HEAD_PTRf, start_ptr);
        if (soc_mem_field_valid(unit, MMU_REPL_HEAD_TBLm, ADDL_REPSf)) {
            if (intf_count > 4) {
                soc_MMU_REPL_HEAD_TBLm_field32_set(unit, &repl_head_entry,
                        ADDL_REPSf, 3);
            } else if (intf_count > 1) {
                soc_MMU_REPL_HEAD_TBLm_field32_set(unit, &repl_head_entry,
                        ADDL_REPSf, intf_count - 2);
            } else {
                soc_MMU_REPL_HEAD_TBLm_field32_set(unit, &repl_head_entry,
                        ADDL_REPSf, 0);
            }
        }

        if (SOC_PBMP_MEMBER(old_member_bitmap, member_bitmap_index)) {
            /* Port is already a member of the group */
            member_id = 0; 
            SOC_PBMP_ITER(old_member_bitmap, member) {
                if (member == member_bitmap_index) {
                    break;
                }
                member_id++;
            }
            SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_HEAD_TBLm(unit, MEM_BLOCK_ALL,
                        old_head_index + member_id, &repl_head_entry));
            new_head_index = old_head_index;
        } else {
            if (soc_feature(unit, soc_feature_static_repl_head_alloc)) {
                /* If REPL_HEAD entries are statically allocated to
                 * replication groups, the port should already be set
                 * in member_bitmap.
                 */
                return BCM_E_PORT;
            }
            SOC_PBMP_PORT_ADD(new_member_bitmap, member_bitmap_index);
            new_member_count = old_member_count + 1;
            BCM_IF_ERROR_RETURN(_bcm_tr3_repl_head_block_alloc(unit,
                        new_member_count, &new_head_index));
            old_member_id = 0;
            new_member_id = 0;
            SOC_PBMP_ITER(new_member_bitmap, member) {
                if (member == member_bitmap_index) {
                   SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_HEAD_TBLm(unit,
                                MEM_BLOCK_ALL, new_head_index + new_member_id,
                                &repl_head_entry));
                } else {
                    SOC_IF_ERROR_RETURN(READ_MMU_REPL_HEAD_TBLm(unit,
                                MEM_BLOCK_ANY, old_head_index + old_member_id,
                                &old_repl_head_entry));
                    SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_HEAD_TBLm(unit,
                                MEM_BLOCK_ALL, new_head_index + new_member_id,
                                &old_repl_head_entry));
                    old_member_id++;
                }
                new_member_id++;
            }           
        }
 
        /* Update REPL_GROUP entry */

#ifdef BCM_TRIDENT2_SUPPORT
        if (soc_feature(unit, soc_feature_split_repl_group_table)) {
            BCM_IF_ERROR_RETURN(_bcm_td2_repl_initial_copy_count_set(unit,
                        repl_group, port, intf_count));
            BCM_IF_ERROR_RETURN(_bcm_td2_repl_group_info_set(unit,
                        repl_group, new_member_bitmap, new_head_index));
        } else
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
        if (SOC_IS_KATANA2(unit)) {

            if (intf_count == 1) {
                SOC_PBMP_PORT_ADD(new_last_bitmap, last_member_bitmap_index);
            } else {
                SOC_PBMP_PORT_REMOVE(new_last_bitmap, last_member_bitmap_index);
            }
            
            BCM_IF_ERROR_RETURN(_bcm_kt2_repl_group_info_set(unit, 
                         remap_grp_id, port, new_member_bitmap,
                         new_last_bitmap, new_head_index)); 

       } else 
#endif
        {
            for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
                fldbuf[i] = SOC_PBMP_WORD_GET(new_member_bitmap, i);
            }
            soc_MMU_REPL_GROUPm_field_set(unit, &repl_group_entry,
                        MEMBER_BITMAPf, fldbuf);
           
            if (intf_count == 1) {
                SOC_PBMP_PORT_ADD(new_last_bitmap, member_bitmap_index);
            } else {
                SOC_PBMP_PORT_REMOVE(new_last_bitmap, member_bitmap_index);
            }

            for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
                fldbuf[i] = SOC_PBMP_WORD_GET(new_last_bitmap, i);
            }
            soc_MMU_REPL_GROUPm_field_set(unit, &repl_group_entry,
                        LAST_BITMAPf, fldbuf);

            soc_MMU_REPL_GROUPm_field32_set(unit, &repl_group_entry,
                        HEAD_INDEXf, new_head_index);

            SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_GROUPm(unit, MEM_BLOCK_ALL,
                            repl_group, &repl_group_entry));
        }

        /* Release old block of REPL_HEAD entries */
        if (old_member_count > 0 && old_head_index != new_head_index) {
            if (soc_feature(unit, soc_feature_repl_head_ptr_replace)) {
                BCM_IF_ERROR_RETURN(_bcm_tr3_repl_head_ptr_replace(unit,
                            old_head_index, old_member_bitmap,
                            new_head_index, new_member_bitmap,
                            new_last_bitmap));
            }
            BCM_IF_ERROR_RETURN(_bcm_tr3_repl_head_block_free(unit,
                        old_head_index, old_member_count));
        }

    } else {
        /* Remove member from replication group */

        if (soc_feature(unit, soc_feature_static_repl_head_alloc)) {
            /* Clear REPL_HEAD entry */
            member_id = 0; 
            SOC_PBMP_ITER(old_member_bitmap, member) {
                if (member == member_bitmap_index) {
                    break;
                }
                member_id++;
            }
            SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_HEAD_TBLm(unit,
                        MEM_BLOCK_ALL, old_head_index + member_id,
                        soc_mem_entry_null(unit, MMU_REPL_HEAD_TBLm)));

            /* Set bit in LAST_BITMAP */
            SOC_PBMP_PORT_ADD(new_last_bitmap, member_bitmap_index);
            for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
                fldbuf[i] = SOC_PBMP_WORD_GET(new_last_bitmap, i);
            }

            soc_MMU_REPL_GROUPm_field_set(unit, &repl_group_entry,
                        LAST_BITMAPf, fldbuf);
            SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_GROUPm(unit, MEM_BLOCK_ALL,
                            repl_group, &repl_group_entry));
            return BCM_E_NONE;
        }

        if (!SOC_PBMP_MEMBER(old_member_bitmap, member_bitmap_index)) {
            /* Port is not a member. Nothing more to do. */
            return BCM_E_NONE;
        }

        /* Update REPL_HEAD table */

        new_member_count = old_member_count - 1;
        for (loop = 0; loop < 2; loop++) {
            if (new_member_count > 0) {
                rv = _bcm_tr3_repl_head_block_alloc(unit,
                                                    new_member_count,
                                                    &new_head_index);
                if (BCM_FAILURE(rv)) {
                    if (!reserve_multicast_resources) {
                        return rv; 
                    }
                    BCM_PBMP_COUNT(PBMP_ALL(unit), num_of_ports);
                    new_head_index = 
                        soc_mem_index_max(unit, MMU_REPL_HEAD_TBLm)
                                     - num_of_ports + 1;
                    reserved_resources_used = 1;
                }
                old_member_id = 0;
                new_member_id = 0;
                SOC_PBMP_ITER(old_member_bitmap, member) {
                    if (member != member_bitmap_index) {
                        SOC_IF_ERROR_RETURN(READ_MMU_REPL_HEAD_TBLm(unit,
                                MEM_BLOCK_ANY, old_head_index + old_member_id,
                                &old_repl_head_entry));
                        SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_HEAD_TBLm(unit,
                                MEM_BLOCK_ALL, new_head_index + new_member_id,
                                &old_repl_head_entry));
                        new_member_id++;
                    }
                    old_member_id++;
                }
            } else {
                new_head_index = 0;
            }

            if (loop == 0) {
                SOC_PBMP_PORT_REMOVE(new_member_bitmap, member_bitmap_index);
            }

        /* Update REPL_GROUP entry */

#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_feature(unit, soc_feature_split_repl_group_table)) {
                BCM_IF_ERROR_RETURN(_bcm_td2_repl_initial_copy_count_set(unit,
                            repl_group, port, 0));
                BCM_IF_ERROR_RETURN(_bcm_td2_repl_group_info_set(unit,
                            repl_group, new_member_bitmap, new_head_index));
            } else
#endif /* BCM_TRIDENT2_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
            if (SOC_IS_KATANA2(unit)) {
                if (loop == 0) {
                    SOC_PBMP_PORT_REMOVE(new_last_bitmap, 
                                              last_member_bitmap_index);
                }
                
                BCM_IF_ERROR_RETURN(_bcm_kt2_repl_group_info_set(unit,
                             remap_grp_id, port, new_member_bitmap,
                             new_last_bitmap, new_head_index));
            } else 
#endif /*BCM_KATANA2_SUPPORT */

            {
                for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
                    fldbuf[i] = SOC_PBMP_WORD_GET(new_member_bitmap, i);
                }
                soc_MMU_REPL_GROUPm_field_set(unit, &repl_group_entry,
                         MEMBER_BITMAPf, fldbuf);

                SOC_PBMP_PORT_REMOVE(new_last_bitmap, member_bitmap_index);
                for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
                    fldbuf[i] = SOC_PBMP_WORD_GET(new_last_bitmap, i);
                }
                soc_MMU_REPL_GROUPm_field_set(unit, &repl_group_entry,
                            LAST_BITMAPf, fldbuf);

                soc_MMU_REPL_GROUPm_field32_set(unit, &repl_group_entry,
                            HEAD_INDEXf, new_head_index);

                SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_GROUPm(unit, MEM_BLOCK_ALL,
                                repl_group, &repl_group_entry));
            }

        /* Release old block of REPL_HEAD entries */
            if (soc_feature(unit, soc_feature_repl_head_ptr_replace)) {
                BCM_IF_ERROR_RETURN(_bcm_tr3_repl_head_ptr_replace(unit,
                            old_head_index, old_member_bitmap,
                            new_head_index, new_member_bitmap,
                            new_last_bitmap));
            }

            if (loop == 0) {
                BCM_IF_ERROR_RETURN(_bcm_tr3_repl_head_block_free(unit,
                            old_head_index, old_member_count));
            }

            if (!reserved_resources_used) {
                break;
            }

            old_head_index = new_head_index;
            BCM_PBMP_ASSIGN(old_member_bitmap, new_member_bitmap);            
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_tr3_repl_list_start_ptr_get
 * Purpose:
 *      Get replication list start pointer for given (repl_group, port).
 * Parameters:
 *	unit       - StrataSwitch PCI device unit number.
 *	repl_group - The replication group number.
 *	port       - Port.
 *	start_ptr  - (OUT) Replication list's start pointer to
 *	             REPL_LIST table.
 * Returns:
 *	BCM_E_XXX
 */
STATIC int
_bcm_tr3_repl_list_start_ptr_get(int unit, int repl_group, bcm_port_t port,
        int *start_ptr)
{
    soc_info_t *si;
    bcm_port_t phy_port, mmu_port;
    int member_bitmap_index;
    soc_mem_t repl_group_table;
    soc_field_t member_bitmap_f, head_index_f; 
    uint32 repl_group_entry[SOC_MAX_MEM_FIELD_WORDS];
    uint32 fldbuf[SOC_PBMP_WORD_MAX];
    soc_pbmp_t member_bitmap;
    int i;
    int member_id, member;
    int head_index;
    mmu_repl_head_tbl_entry_t repl_head_entry;
#ifdef BCM_KATANA2_SUPPORT
    _kt2_repl_grp_info_t kt2_repl_grp_info;
    mmu_repl_grp_tbl2_entry_t repl_group_base_ptr_entry;
    int remap_grp_id = 0;
#endif /* BCM_KATANA2_SUPPORT */

    si = &SOC_INFO(unit);
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        phy_port = port;
        mmu_port = port;
        BCM_IF_ERROR_RETURN
        (_bcm_kt_ipmc_mmu_mc_remap_ptr(unit, repl_group, &remap_grp_id, FALSE));
    } else 
#endif /* BCM_KATANA2_SUPPORT */
    {
        phy_port = si->port_l2p_mapping[port];
        mmu_port = si->port_p2m_mapping[phy_port];
    }
    member_bitmap_index = mmu_port;
#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_split_repl_group_table)) {
        /* Trident2 has 108 MMU ports: port 0-53 in X pipe, 64-117 in Y pipe.
         * MMU ports 53 and 117 are purge ports.
         */
        if (mmu_port >= 64) {
            member_bitmap_index = mmu_port - 64;
            repl_group_table = MMU_REPL_GROUP_INFO1m;
        } else {
            repl_group_table = MMU_REPL_GROUP_INFO0m;
        }
        member_bitmap_f = PIPE_MEMBER_BMPf;
        head_index_f = PIPE_BASE_PTRf;
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
#ifdef BCM_KATANA2_SUPPORT
        if (SOC_IS_KATANA2(unit)) {
            sal_memset(&kt2_repl_grp_info, 0, sizeof(kt2_repl_grp_info));
            BCM_IF_ERROR_RETURN(_bcm_kt2_repl_list_tbl_ptr_get (unit, mmu_port,
               &kt2_repl_grp_info));
            repl_group_table  = kt2_repl_grp_info.mb_mem;
            member_bitmap_f = kt2_repl_grp_info.mbmp;
            head_index_f = BASE_PTRf;
            if (mmu_port >= 120) {
                member_bitmap_index = mmu_port - 120;
            }
        } else 
#endif /* BCM_KATANA2_SUPPORT */
        { 
            repl_group_table = MMU_REPL_GROUPm;
            member_bitmap_f = MEMBER_BITMAPf;
            head_index_f = HEAD_INDEXf;
 
            /* Triumph3 MMU does not support replication on ports 57, 59, 61 and 62 */
            if ((mmu_port == 57) || (mmu_port == 59) || (mmu_port == 61) || (mmu_port == 62)) {
                return BCM_E_PORT;
            } else if (mmu_port == 60) {
                member_bitmap_index = 59;
            }   
        }
    }

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, repl_group_table, MEM_BLOCK_ANY,
                remap_grp_id, repl_group_entry));
    } else
#endif /* BCM_KATANA2_SUPPORT */
    {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, repl_group_table, MEM_BLOCK_ANY,
                repl_group, repl_group_entry));
    } 

    sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
    soc_mem_field_get(unit, repl_group_table, repl_group_entry,
            member_bitmap_f, fldbuf);
    for (i = 0; i < SOC_PBMP_WORD_MAX; i++) {
        SOC_PBMP_WORD_SET(member_bitmap, i, fldbuf[i]);
    }
    if (!SOC_PBMP_MEMBER(member_bitmap, member_bitmap_index)) {
        /* Port is not set in member_bitmap */
        *start_ptr = 0;
        return BCM_E_NONE;
    }

    member_id = 0; 
    SOC_PBMP_ITER(member_bitmap, member) {
        if (member == member_bitmap_index) {
            break;
        }
        member_id++;
    }
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit, MMU_REPL_GRP_TBL2m, MEM_BLOCK_ANY,
                    remap_grp_id, &repl_group_base_ptr_entry));
        head_index = member_id + soc_mem_field32_get(unit, MMU_REPL_GRP_TBL2m,
                &repl_group_base_ptr_entry, head_index_f);
    } else
#endif /* BCM_KATANA2_SUPPORT */
    {
        head_index = member_id + soc_mem_field32_get(unit, repl_group_table,
                repl_group_entry, head_index_f);
    }
    SOC_IF_ERROR_RETURN(READ_MMU_REPL_HEAD_TBLm(unit, MEM_BLOCK_ANY, head_index,
                &repl_head_entry));
    *start_ptr = soc_MMU_REPL_HEAD_TBLm_field32_get(unit, &repl_head_entry,
            HEAD_PTRf);

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_tr3_repl_list_free
 * Purpose:
 *	Free the REPL_LIST entries in the HW list starting at start_ptr.
 * Parameters:
 *	unit      - StrataSwitch PCI device unit number.
 *	start_ptr - Replication list's start pointer to
 *	            REPL_LIST table.
 * Returns:
 *	BCM_E_XXX
 */
STATIC int
_bcm_tr3_repl_list_free(int unit, int start_ptr)
{
    mmu_repl_list_tbl_entry_t repl_list_entry;
    int	prev_repl_entry_ptr, repl_entry_ptr;

    prev_repl_entry_ptr = -1;
    repl_entry_ptr = start_ptr;

    while (repl_entry_ptr != prev_repl_entry_ptr) {
        SOC_IF_ERROR_RETURN
            (READ_MMU_REPL_LIST_TBLm(unit, MEM_BLOCK_ANY,
                                     repl_entry_ptr, &repl_list_entry));
        BCM_IF_ERROR_RETURN
            (_bcm_tr3_repl_list_entry_free(unit, repl_entry_ptr)); 
        prev_repl_entry_ptr = repl_entry_ptr;
        repl_entry_ptr = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                &repl_list_entry, NEXTPTRf);
    }

    return BCM_E_NONE;
}

#ifdef BCM_KATANA2_SUPPORT
/*
 * Function:
 *      _bcm_kt2_repl_list_write
 * Purpose:
 *      Write the replication list contained in intf_vec into REPL_LIST table.
 *      Return the start_ptr and total interface count.
 * Parameters:
 *      unit       - StrataSwitch PCI device unit number.
 *      intf_vec   - (IN) Vector of interfaces.
 *      nh_count   - (IN) Next hop count.
 *      start_ptr  - (OUT) Replication list's start pointer to
 *                   REPL_LIST table.
 *      intf_count - (OUT) Interface count.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_kt2_repl_list_write(int unit, SHR_BITDCL *intf_vec, 
                         uint32 nh_count, int *start_ptr, int *intf_count)
{
    int i;
    int remaining_count;
    int prev_repl_entry_ptr, repl_entry_ptr;
    uint32 msb_max, msb;
    uint32 ls_bits[2];
    int rv = BCM_E_NONE;
    int no_more_free_repl_entries;
    mmu_repl_list_tbl_entry_t repl_list_entry;
    int if_cnt, nh_cnt, offset;
    uint32 ms_bit;
    uint32 repl_type =0;

    if_cnt = soc_mem_index_count(unit, EGR_L3_INTFm);
    nh_cnt = soc_mem_index_count(unit, EGR_L3_NEXT_HOPm);
    *intf_count = 0;

    for (i = 0; i < _SHR_BITDCLSIZE(REPL_INTF_TOTAL(unit)); i++) {
        *intf_count += _shr_popcount(intf_vec[i]);
    }

    if (*intf_count == 0) {
        return BCM_E_NONE;
    }

    if (nh_count > 0) {
        repl_type = 1;
        msb_max = _SHR_BITDCLSIZE(nh_cnt) / 2; /* 32 -> 64 */
    } else {
        msb_max = _SHR_BITDCLSIZE(if_cnt) / 2; /* 32 -> 64 */
    }

    offset = (repl_type == 0)? 0 : _SHR_BITDCLSIZE(if_cnt);

    remaining_count = *intf_count;
    prev_repl_entry_ptr = -1;


    for (msb = 0; msb < msb_max; msb++) {

        ls_bits[0] = intf_vec[offset + 2 * msb + 0];
        ls_bits[1] = intf_vec[offset + 2 * msb + 1];
        if ((ls_bits[0] != 0) || (ls_bits[1] != 0)) {
            rv = _bcm_tr3_repl_list_entry_alloc(unit, &repl_entry_ptr);
            if (rv == BCM_E_RESOURCE) {
                no_more_free_repl_entries = TRUE;
            } else if (BCM_FAILURE(rv)) {
                return rv;
            } else {
                no_more_free_repl_entries = FALSE;
            }
            if (prev_repl_entry_ptr == -1) {
                if (no_more_free_repl_entries) {
                    return BCM_E_RESOURCE;
                }
                *start_ptr = repl_entry_ptr;
            } else {
                if (no_more_free_repl_entries) {
                    /* Terminate replication list */
                    soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                            NEXTPTRf, prev_repl_entry_ptr);
                } else {
                    soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                            NEXTPTRf, repl_entry_ptr);
                }
                SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_LIST_TBLm(unit,
                            MEM_BLOCK_ALL, prev_repl_entry_ptr,
                            &repl_list_entry));
                if (no_more_free_repl_entries) {
                    /* Free the list already written */
                    _bcm_tr3_repl_list_free(unit, *start_ptr);
                    return BCM_E_RESOURCE;
                }
            }
            prev_repl_entry_ptr = repl_entry_ptr;

            sal_memset(&repl_list_entry, 0, sizeof(repl_list_entry));
            if (repl_type == 1) {
                /* Replication is over next hops */
                ms_bit = msb + (1 << (soc_mem_field_length(unit, 
                                      MMU_REPL_LIST_TBLm, MSB_VLANf) - 1));
            } else {
                /* Replication is over interfaces */
                ms_bit = msb;
            }
            soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                    MSB_VLANf, ms_bit);
            soc_MMU_REPL_LIST_TBLm_field_set(unit, &repl_list_entry,
                    LSB_VLAN_BMf, ls_bits);

            remaining_count -= (_shr_popcount(ls_bits[0]) +
                    _shr_popcount(ls_bits[1]));
             if (remaining_count > 4) {
               /* Set the RMNG_REPS field to all ones */
                soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                        RMNG_REPSf, (1 << soc_mem_field_length(unit,
                        MMU_REPL_LIST_TBLm, RMNG_REPSf)) - 1);
             } else if (remaining_count > 0) {
                soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                            RMNG_REPSf, remaining_count - 1);

             } else { /* No more intefaces left to be written */
                break;
             }
        }
    }

    if (prev_repl_entry_ptr > 0) {
        /* Write final entry */
        soc_MMU_REPL_LIST_TBLm_field32_set(unit,
                          &repl_list_entry, NEXTPTRf, prev_repl_entry_ptr);
        SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_LIST_TBLm(unit,
                   MEM_BLOCK_ALL, prev_repl_entry_ptr, &repl_list_entry));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_kt2_repl_list_compare
 * Purpose:
 *      Compare replication list starting at given pointer to
 *      the interface list contained in intf_vec.
 * Parameters:
 *      unit      - StrataSwitch PCI device unit number.
 *      start_ptr - (IN) Replication list's start pointer to REPL_LIST table.
 *      intf_vec  - (IN) Vector of interfaces.
 *      nh_count  - (IN) next hops count
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_kt2_repl_list_compare(int unit, int start_ptr,
                          SHR_BITDCL *intf_vec, uint32 nh_count)
{
    uint32              msb, msb_val, hw_msb, msb_max;
    uint32              ls_bits[2], hw_ls_bits[2];
    mmu_repl_list_tbl_entry_t repl_list_entry;
    int                 repl_entry_ptr, prev_repl_entry_ptr;
    uint32   offset, if_cnt, nh_cnt;
    uint32 repl_type =0;
    prev_repl_entry_ptr = -1;
    repl_entry_ptr = start_ptr;

    if_cnt = soc_mem_index_count(unit, EGR_L3_INTFm);
    nh_cnt = soc_mem_index_count(unit, EGR_L3_NEXT_HOPm);


    if (nh_count > 0) {
        repl_type = 1;
        msb_max = _SHR_BITDCLSIZE(nh_cnt) / 2; /* 32 -> 64 */
    } else {
        msb_max = _SHR_BITDCLSIZE(if_cnt) / 2; /* 32 -> 64 */
    }

    offset = (repl_type == 0)? 0 : _SHR_BITDCLSIZE(if_cnt);

    for (msb = 0; msb < msb_max; msb++) {
        ls_bits[0] = intf_vec[offset + 2 * msb + 0];
        ls_bits[1] = intf_vec[offset + 2 * msb + 1];
        if ((ls_bits[0] != 0) || (ls_bits[1] != 0)) {
            if (repl_entry_ptr == prev_repl_entry_ptr) { /* HW list end */
                return BCM_E_NOT_FOUND;
            }
            SOC_IF_ERROR_RETURN(READ_MMU_REPL_LIST_TBLm(unit, MEM_BLOCK_ANY,
                        repl_entry_ptr, &repl_list_entry));
            hw_msb = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                    &repl_list_entry, MSB_VLANf);
            soc_MMU_REPL_LIST_TBLm_field_get(unit, &repl_list_entry,
                    LSB_VLAN_BMf, hw_ls_bits);

            if (repl_type == 1) {
                msb_val = msb + (1 << (soc_mem_field_length(unit,
                                      MMU_REPL_LIST_TBLm, MSB_VLANf) - 1)); 
            } else {
                msb_val = msb;
            }
            if ((hw_msb != msb_val) || (ls_bits[0] != hw_ls_bits[0]) ||
                    (ls_bits[1] != hw_ls_bits[1])) {
                return BCM_E_NOT_FOUND;
            }
            prev_repl_entry_ptr = repl_entry_ptr;
            repl_entry_ptr = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                    &repl_list_entry, NEXTPTRf);
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_kt2_repl_intf_vec_construct
 * Purpose:
 *      Construct replication interface vector.
 * Parameters:
 *      unit       - StrataSwitch PCI device unit number.
 *      port       - (IN) Port.
 *      if_count   - (IN) Number of interfaces in replication list.
 *      if_array   - (IN) List of interface numbers.
 *      is_l3      - (IN) Indicates if interfaces are IPMC interfaces.
 *      check_port - (IN) If if_array consists of L3 interfaces, this parameter
 *                   controls whether to check the given port is a member
 *                   in each L3 interface's VLAN. This check should be
 *                   disabled when not needed, in order to improve
 *                   performance.
 *      intf_vec   - (OUT) Vector of interfaces.
 *      nh_count   - (OUT) Next hop Count
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_kt2_repl_intf_vec_construct(int unit, bcm_port_t port, int if_count,
        bcm_if_t *if_array, int is_l3, int check_port, SHR_BITDCL *intf_vec,
        uint32 *nh_count)
{
    int if_num;
    bcm_l3_intf_t l3_intf;
    uint32 partition;
    uint32 nextHop = 0;

    partition =  soc_mem_index_count(unit, EGR_L3_INTFm);

    for (if_num = 0; if_num < if_count; if_num++) {
        if (!BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, if_array[if_num])) {
            /* L3 interface is used */

            if (if_array[if_num] > soc_mem_index_max(unit, EGR_L3_INTFm)) {
                return BCM_E_PARAM;
            }

            if (check_port) {
                bcm_l3_intf_t_init(&l3_intf);
                l3_intf.l3a_intf_id = if_array[if_num];
                BCM_IF_ERROR_RETURN(bcm_esw_l3_intf_get(unit, &l3_intf));
            }

            /* Get a next hop index if the L3 inteface is not already
             * associated with one.
             */
            if (is_l3) { /* L3 interface is being added to IPMC group */
                SHR_BITSET(intf_vec, if_array[if_num]);
            }
        } else {
            /* Next hop is used */
            SHR_BITSET(intf_vec, partition + if_array[if_num] -
                       BCM_XGS3_DVP_EGRESS_IDX_MIN);
            nextHop++;
        }
    }
    *nh_count = nextHop;

    return BCM_E_NONE;
}

#endif /* BCM_KATANA2_SUPPORT */
/*
 * Function:
 *	_bcm_tr3_repl_list_write
 * Purpose:
 *	Write the replication list contained in intf_vec into REPL_LIST table.
 *      Return the start_ptr and total interface count.
 * Parameters:
 *	unit       - StrataSwitch PCI device unit number.
 *	start_ptr  - (OUT) Replication list's start pointer to
 *	             REPL_LIST table.
 *      intf_count - (OUT) Interface count.
 *	intf_vec   - (IN) Vector of interfaces.
 * Returns:
 *	BCM_E_XXX
 */
STATIC int
_bcm_tr3_repl_list_write(int unit, int *start_ptr, int *intf_count,
                        SHR_BITDCL *intf_vec)
{
    int i;
    int remaining_count;
    int prev_repl_entry_ptr, repl_entry_ptr;
    uint32 msb_max, msb;
    uint32 ls_bits[2];
    int rv = BCM_E_NONE;
    int no_more_free_repl_entries;
    mmu_repl_list_tbl_entry_t repl_list_entry;

    *intf_count = 0;
    for (i = 0; i < _SHR_BITDCLSIZE(REPL_INTF_TOTAL(unit)); i++) {
        *intf_count += _shr_popcount(intf_vec[i]);
    }

    if (*intf_count == 0) {
        return BCM_E_NONE;
    }

    remaining_count = *intf_count;
    prev_repl_entry_ptr = -1;
    msb_max = _SHR_BITDCLSIZE(REPL_INTF_TOTAL(unit)) / 2; /* 32 -> 64 */
    for (msb = 0; msb < msb_max; msb++) {
        ls_bits[0] = intf_vec[2 * msb + 0];
        ls_bits[1] = intf_vec[2 * msb + 1];
        if ((ls_bits[0] != 0) || (ls_bits[1] != 0)) {
            rv = _bcm_tr3_repl_list_entry_alloc(unit, &repl_entry_ptr);
            if (rv == BCM_E_RESOURCE) {
                no_more_free_repl_entries = TRUE;
            } else if (BCM_FAILURE(rv)) {
                return rv;
            } else {
                no_more_free_repl_entries = FALSE;
            }
            if (prev_repl_entry_ptr == -1) {
                if (no_more_free_repl_entries) {
                    return BCM_E_RESOURCE;
                }
                *start_ptr = repl_entry_ptr;
            } else {
                if (no_more_free_repl_entries) {
                    /* Terminate replication list */
                    soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                            NEXTPTRf, prev_repl_entry_ptr);
                } else {
                    soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                            NEXTPTRf, repl_entry_ptr);
                }
                SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_LIST_TBLm(unit,
                            MEM_BLOCK_ALL, prev_repl_entry_ptr,
                            &repl_list_entry));
                if (no_more_free_repl_entries) {
                    /* Free the list already written */
                    _bcm_tr3_repl_list_free(unit, *start_ptr);
                    return BCM_E_RESOURCE;
                }
            } 
            prev_repl_entry_ptr = repl_entry_ptr;

            sal_memset(&repl_list_entry, 0, sizeof(repl_list_entry));
            soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                    MSB_VLANf, msb);
            soc_MMU_REPL_LIST_TBLm_field_set(unit, &repl_list_entry,
                    LSB_VLAN_BMf, ls_bits);

            remaining_count -= (_shr_popcount(ls_bits[0]) +
                    _shr_popcount(ls_bits[1]));
#ifdef BCM_TRIDENT2_SUPPORT
            if (SOC_IS_TRIDENT2(unit)) {
                if (remaining_count > 5) {
                    /* Set the RMNG_REPS field to all zeroes */
                    soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                            RMNG_REPSf, 0);
                } else if (remaining_count > 0) {
                    soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                            RMNG_REPSf, remaining_count);
                } else { /* No more intefaces left to be written */
                    break;
                }
            } else
#endif /* BCM_TRIDENT2_SUPPORT */
            {
                if (remaining_count > 4) {
                    /* Set the RMNG_REPS field to all ones */
                    soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                            RMNG_REPSf, (1 << soc_mem_field_length(unit,
                                    MMU_REPL_LIST_TBLm, RMNG_REPSf)) - 1);
                } else if (remaining_count > 0) {
                    soc_MMU_REPL_LIST_TBLm_field32_set(unit, &repl_list_entry,
                            RMNG_REPSf, remaining_count - 1);

                } else { /* No more intefaces left to be written */
                    break;
                }
            }
        }
    }

    if (prev_repl_entry_ptr > 0) {
        /* Write final entry */
        soc_MMU_REPL_LIST_TBLm_field32_set(unit,
                          &repl_list_entry, NEXTPTRf, prev_repl_entry_ptr);
        SOC_IF_ERROR_RETURN(WRITE_MMU_REPL_LIST_TBLm(unit,
                   MEM_BLOCK_ALL, prev_repl_entry_ptr, &repl_list_entry));
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_tr3_repl_list_compare
 * Purpose:
 *	Compare replication list starting at given pointer to
 *	the interface list contained in intf_vec.
 * Parameters:
 *	unit - StrataSwitch PCI device unit number.
 *	start_ptr - Replication list's start pointer to REPL_LIST table.
 *	intf_vec - Vector of interfaces.
 * Returns:
 *	BCM_E_XXX
 */
STATIC int
_bcm_tr3_repl_list_compare(int unit, int start_ptr,
                          SHR_BITDCL *intf_vec)
{
    uint32		msb, hw_msb, msb_max;
    uint32		ls_bits[2], hw_ls_bits[2];
    mmu_repl_list_tbl_entry_t repl_list_entry;
    int	                repl_entry_ptr, prev_repl_entry_ptr;

    prev_repl_entry_ptr = -1;
    repl_entry_ptr = start_ptr;
    msb_max = _SHR_BITDCLSIZE(REPL_INTF_TOTAL(unit)) / 2; /* 32 -> 64 */

    for (msb = 0; msb < msb_max; msb++) {
        ls_bits[0] = intf_vec[2 * msb + 0];
        ls_bits[1] = intf_vec[2 * msb + 1];
        if ((ls_bits[0] != 0) || (ls_bits[1] != 0)) {
            if (repl_entry_ptr == prev_repl_entry_ptr) { /* HW list end */
                return BCM_E_NOT_FOUND;
            }
            SOC_IF_ERROR_RETURN(READ_MMU_REPL_LIST_TBLm(unit, MEM_BLOCK_ANY,
                        repl_entry_ptr, &repl_list_entry));
            hw_msb = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                    &repl_list_entry, MSB_VLANf);
            soc_MMU_REPL_LIST_TBLm_field_get(unit, &repl_list_entry,
                    LSB_VLAN_BMf, hw_ls_bits);
            if ((hw_msb != msb) || (ls_bits[0] != hw_ls_bits[0]) ||
                    (ls_bits[1] != hw_ls_bits[1])) {
                return BCM_E_NOT_FOUND;
            }
            prev_repl_entry_ptr = repl_entry_ptr;
            repl_entry_ptr = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                    &repl_list_entry, NEXTPTRf);
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_tr3_repl_intf_vec_construct
 * Purpose:
 *	Construct replication interface vector.
 * Parameters:
 *	unit       - StrataSwitch PCI device unit number.
 *	port       - Port.
 *	if_count   - Number of interfaces in replication list.
 *  if_array   - (IN) List of interface numbers.
 *  if_updated - (IN) interfaces info to be updated including
 *                    interfaces which is new to the group and port
 *                    and interfaces to be deleted.
 *  is_l3      - (IN) Indicates if interfaces are IPMC interfaces.
 *  check_port - If if_array consists of L3 interfaces, this parameter 
 *                   controls whether to check the given port is a member
 *                   in each L3 interface's VLAN. This check should be  
 *                   disabled when not needed, in order to improve
 *                   performance.
 *  intf_vec   - (OUT) Vector of interfaces.
 * Returns:
 *	BCM_E_XXX
 */
STATIC int
_bcm_tr3_repl_intf_vec_construct(
     int unit, bcm_port_t port, int if_count, bcm_if_t *if_array,
     _tr3_if_updated_t *if_updated, int is_l3, int check_port,
     SHR_BITDCL *intf_vec)
{
    int if_num;
    bcm_l3_intf_t l3_intf;
    uint32 nh_flags;
    bcm_l3_egress_t nh_info;
    int nh_index;
    egr_l3_next_hop_entry_t egr_nh;
    int mac_oui, mac_non_oui;
    bcm_mac_t rbridge_mac;

    if (is_l3) {
        bcm_if_t intf;
        int ref_count = 0;
        for (if_num = 0; if_num < if_updated->if_count_del; if_num++) {
            intf = if_updated->if_array_del[if_num];
            if (BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, intf)) {
                continue;
            }

            if (REPL_L3_INTF_NEXT_HOP_IPMC(unit, intf) == 
                REPL_NH_INDEX_L3_EGRESS_ALLOCATED) {
                /* This next hop attached to the intf is allocated 
                 * by bcm_l3_egress_create. 
                 */
                continue;
            }
            
            if (intf > soc_mem_index_max(unit, EGR_L3_INTFm)) {
                return BCM_E_PARAM;
            }
            /* Decrease the NH ref count when the intf is deleted
             * from <group, port>. 
             */
            nh_index = REPL_L3_INTF_NEXT_HOP_IPMC(unit, intf);
            BCM_IF_ERROR_RETURN(bcm_xgs3_nh_del(unit, 0, nh_index));
            BCM_IF_ERROR_RETURN(
                bcm_xgs3_nh_ref_count_get(unit, nh_index, &ref_count));
            if (ref_count == 0) {
                REPL_L3_INTF_NEXT_HOP_IPMC(unit, intf) = 
                    REPL_NH_INDEX_UNALLOCATED;
            }                
        }
    }
    
    for (if_num = 0; if_num < if_count; if_num++) {
        if (!BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, if_array[if_num])) {
            /* L3 interface is used */

            if (if_array[if_num] > soc_mem_index_max(unit, EGR_L3_INTFm)) {
                return BCM_E_PARAM;
            }

            if (check_port) {
                bcm_l3_intf_t_init(&l3_intf);
                l3_intf.l3a_intf_id = if_array[if_num];
                BCM_IF_ERROR_RETURN(bcm_esw_l3_intf_get(unit, &l3_intf));
            }

            if (is_l3) { /* L3 interface is being added to IPMC group */
                if (REPL_L3_INTF_NEXT_HOP_IPMC(unit, if_array[if_num]) ==
                        REPL_NH_INDEX_UNALLOCATED) {
                    /* A next hop index has not been allocated for the L3
                     * inteface yet. Allocate a new next hop index.
                     */
                    bcm_l3_egress_t_init(&nh_info);
                    nh_flags = _BCM_L3_SHR_MATCH_DISABLE |
                        _BCM_L3_SHR_WRITE_DISABLE;
                    BCM_IF_ERROR_RETURN
                        (bcm_xgs3_nh_add(unit, nh_flags, &nh_info, &nh_index));

                    sal_memset(&egr_nh, 0, sizeof(egr_l3_next_hop_entry_t));
                    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                            ENTRY_TYPEf, 7);
                    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                            L3MC__INTF_NUMf, if_array[if_num]);
                    if (soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm,
                                L3MC__L2_MC_DA_DISABLEf)) {
                        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L2_MC_DA_DISABLEf, 1);
                    }
                    if (soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm,
                                L3MC__L2_MC_SA_DISABLEf)) {
                        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L2_MC_SA_DISABLEf, 1);
                    }
                    if (soc_mem_field_valid(unit, EGR_L3_NEXT_HOPm,
                                L3MC__L2_MC_VLAN_DISABLEf)) {
                        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                L3MC__L2_MC_VLAN_DISABLEf, 1);
                    }
                    SOC_IF_ERROR_RETURN(soc_mem_write(unit, EGR_L3_NEXT_HOPm,
                                MEM_BLOCK_ALL, nh_index, &egr_nh));

                    REPL_L3_INTF_NEXT_HOP_IPMC(unit, if_array[if_num]) =
                        nh_index;
                } else if (REPL_L3_INTF_NEXT_HOP_IPMC(unit, if_array[if_num]) ==
                        REPL_NH_INDEX_L3_EGRESS_ALLOCATED) {
                    /* A next hop index has already been allocated for this
                     * L3 interface by the bcm_l3_egress_create API. Adding
                     * such L3 interface directly to the IPMC group is not
                     * allowed. To add the L3 egress object to an IPMC group,
                     * first call bcm_multicast_egress_object_encap_get to get
                     * an encap ID, then add the encap ID to the group.
                     */
                    return BCM_E_CONFIG;
                } else {
                    /* IPMC module had already silently allocated a next hop
                     * index for this L3 interface.
                     * But the next hop reference count should be increased
                     * if the intf is new to the <group, port>.
                     */
                    int if_num_new;
                    for (if_num_new = 0; if_num_new < if_updated->if_count_new;
                         if_num_new++) {
                        if (if_array[if_num] == 
                            if_updated->if_array_new[if_num_new]) {
                            nh_index = 
                                REPL_L3_INTF_NEXT_HOP_IPMC(unit,
                                                           if_array[if_num]);
                            _bcm_xgs3_nh_ref_cnt_incr(unit, nh_index);
                            break;
                        }
                    }
                }

                SHR_BITSET(intf_vec,
                    REPL_L3_INTF_NEXT_HOP_IPMC(unit, if_array[if_num]));

            } else { /* L3 interface is being added to Trill group */
                if (REPL_L3_INTF_NEXT_HOP_TRILL(unit, if_array[if_num]) == -1) {
                    bcm_l3_egress_t_init(&nh_info);
                    nh_flags = _BCM_L3_SHR_MATCH_DISABLE |
                        _BCM_L3_SHR_WRITE_DISABLE;
                    BCM_IF_ERROR_RETURN
                        (bcm_xgs3_nh_add(unit, nh_flags, &nh_info, &nh_index));

                    sal_memset(&egr_nh, 0, sizeof(egr_l3_next_hop_entry_t));
#ifdef BCM_TRIDENT2_SUPPORT
                    if (SOC_IS_TRIDENT2(unit)) {
                        /*
                         * TD2 needs to set the entry_type to 0x7 (L3MC) 
                         * in order to use trill_all_rbridges_macda as the DA 
                         * in TRILL header 
                         */
                        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                ENTRY_TYPEf, 7);
                    } else
#endif /* BCM_TRIDENT2_SUPPORT */                        
                    {
                        soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                                ENTRY_TYPEf, 0);
                    }
                    soc_mem_field32_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                            L3__INTF_NUMf, if_array[if_num]);

                    /* Get and Set rbridge mac */
                    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                            bcmSwitchTrillBroadcastDestMacOui, &mac_oui)); 
                    BCM_IF_ERROR_RETURN(bcm_esw_switch_control_get(unit,
                            bcmSwitchTrillBroadcastDestMacNonOui, &mac_non_oui)); 
                    rbridge_mac[0] = (mac_oui >> 16) & 0xff;
                    rbridge_mac[1] = (mac_oui >> 8) & 0xff;
                    rbridge_mac[2] = mac_oui & 0xff;
                    rbridge_mac[3] = (mac_non_oui >> 16) & 0xff;
                    rbridge_mac[4] = (mac_non_oui >> 8) & 0xff;
                    rbridge_mac[5] = mac_non_oui & 0xff;
                    soc_mem_mac_addr_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                            L3__MAC_ADDRESSf, rbridge_mac);

                    SOC_IF_ERROR_RETURN(soc_mem_write(unit, EGR_L3_NEXT_HOPm,
                                MEM_BLOCK_ALL, nh_index, &egr_nh));

                    REPL_L3_INTF_NEXT_HOP_TRILL(unit, if_array[if_num]) =
                        nh_index;
                }
                SHR_BITSET(intf_vec,
                    REPL_L3_INTF_NEXT_HOP_TRILL(unit, if_array[if_num]));
            }
        } else {
            /* Next hop is used */
            SHR_BITSET(intf_vec,
                    if_array[if_num] - BCM_XGS3_DVP_EGRESS_IDX_MIN);
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *	_bcm_tr3_ipmc_egress_intf_set
 * Purpose:
 *	Assign set of egress interfaces to port's replication list for chosen
 *	replication group.
 * Parameters:
 *	unit       - StrataSwitch PCI device unit number.
 *	repl_group - The replication group number.
 *	port       - Port to assign replication list.
 *	if_count   - Number of interfaces in replication list.
 *  if_array   - (IN) List of interface numbers.
 *  if_updated - (IN) interfaces info to be updated including
 *                    interfaces which is new to the group and port
 *                    and interfaces to be deleted.
 *  is_l3      - (IN) Indicates if multicast group is of type IPMC.
 *  check_port - (IN) If if_array consists of L3 interfaces, this parameter 
 *                        controls whether to check the given port is a member
 *                        in each L3 interface's VLAN. This check should be  
 *                        disabled when not needed, in order to improve
 *                        performance.
 * Returns:
 *	BCM_E_XXX
 */
STATIC int
_bcm_tr3_ipmc_egress_intf_set(
    int unit, int repl_group, bcm_port_t port, int if_count,
    bcm_if_t *if_array, _tr3_if_updated_t *if_updated,
    int is_l3, int check_port)
{
    int rv = BCM_E_NONE;
    int set_repl_list;
    int prev_start_ptr, list_start_ptr=0;
    int alloc_size;
    SHR_BITDCL *intf_vec = NULL;
    int repl_hash;
    _bcm_repl_list_info_t *rli_current, *rli_prev;
    int intf_count;
    int new_repl_list = FALSE;
#ifdef BCM_KATANA2_SUPPORT
    uint32 nh_count =0;
#endif /* BCM_KATANA2_SUPPORT */
    int repl_intf_total;
    int intf_cnt_old = 0;

    REPL_INIT(unit);
    REPL_GROUP_ID(unit, repl_group);
    /* Replication is allowed on CPU ports, only on TD2 */
    if (IS_CPU_PORT(unit, port)) {
        if (!SOC_IS_TRIDENT2(unit)) {
            return BCM_E_PARAM;
        }
    } else {   
        REPL_PORT_CHECK(unit, port);
    }

    repl_intf_total = REPL_INTF_TOTAL(unit);
    if (if_count > repl_intf_total) {
        return BCM_E_PARAM;
    } else if (if_count > 0) {
        set_repl_list = TRUE;
    } else {
        set_repl_list = FALSE;
    }

    REPL_LOCK(unit);

    /* Get current replication list start pointer of (repl_group, port) */
    rv = _bcm_tr3_repl_list_start_ptr_get(unit, repl_group, port,
            &prev_start_ptr);
    if (BCM_FAILURE(rv)) {
        goto intf_set_done;
    }

    intf_cnt_old = REPL_PORT_GROUP_INTF_COUNT(unit, port, repl_group);
    if (set_repl_list) {
        /* Set new replication list for given (repl_group, port) */

        alloc_size = SHR_BITALLOCSIZE(repl_intf_total);
        intf_vec = sal_alloc(alloc_size, "Repl interface vector");
        if (intf_vec == NULL) {
            rv = BCM_E_MEMORY;
            goto intf_set_done;
        }
        sal_memset(intf_vec, 0, alloc_size);

        /* Interface validation and vector construction */
#ifdef BCM_KATANA2_SUPPORT
        if (SOC_IS_KATANA2(unit)) {
            rv = _bcm_kt2_repl_intf_vec_construct(unit, port, if_count, 
                      if_array, is_l3, check_port, intf_vec, &nh_count);
        }
        else
#endif /* BCM_KATANA2_SUPPORT */
        {
            rv = _bcm_tr3_repl_intf_vec_construct(unit, port, if_count,
                                                  if_array, if_updated,
                                                  is_l3, check_port, intf_vec);
        }
        if (BCM_FAILURE(rv)) {
            goto intf_set_done;
        }

        /* Search for matching replication list */
        if (if_count < 3 && repl_intf_total >= SHR_BITWID) {
            /* To acclerate in most cases, 3 is an experience value */
            repl_hash = _shr_crc32b(0, (uint8 *)intf_vec, SHR_BITWID);
        } else {
            repl_hash = _shr_crc32b(0, (uint8 *)intf_vec, repl_intf_total);
        }
        for (rli_current = REPL_LIST_INFO(unit); rli_current != NULL;
                rli_current = rli_current->next) {
            if (repl_hash == rli_current->hash) {
#ifdef BCM_KATANA2_SUPPORT
                if (SOC_IS_KATANA2(unit)) {
                    rv = _bcm_kt2_repl_list_compare(unit, rli_current->index,
                          intf_vec, nh_count);
                } else 
#endif /* BCM_KATANA2_SUPPORT */
                {    
                    rv = _bcm_tr3_repl_list_compare(unit, rli_current->index,
                        intf_vec);
                }
                if (rv == BCM_E_NOT_FOUND) {
                    continue; /* Not a match */
                } else if (BCM_FAILURE(rv)) {
                    goto intf_set_done; 
                } else {
                    break; /* Match */
                }
            }
        }

        if (rli_current != NULL) {
            /* Found a match */
            if (prev_start_ptr == rli_current->index) {
                /* (repl_group, port) already points to this list, so done */
                rv = BCM_E_NONE;
                goto intf_set_done;
            } else {
                list_start_ptr = rli_current->index;
                intf_count = rli_current->list_size;
            }
        } else {
            /* Not a match, make a new chain */
#ifdef BCM_KATANA2_SUPPORT
            if (SOC_IS_KATANA2(unit)) {
               rv = _bcm_kt2_repl_list_write(unit,
                     intf_vec, nh_count, &list_start_ptr, &intf_count);
            } else 
#endif /* BCM_KATANA2_SUPPORT */
            {
               rv = _bcm_tr3_repl_list_write(unit, &list_start_ptr,
                     &intf_count, intf_vec);
            }
            if (BCM_FAILURE(rv)) {
                goto intf_set_done;
            }

            if (intf_count > 0) {
                /* Update data structures */
                alloc_size = sizeof(_bcm_repl_list_info_t);
                rli_current = sal_alloc(alloc_size, "IPMC repl list info");
                if (rli_current == NULL) {
                    /* Release list */
                    _bcm_tr3_repl_list_free(unit, list_start_ptr);
                    rv = BCM_E_MEMORY;
                    goto intf_set_done;
                }
                sal_memset(rli_current, 0, alloc_size);
                rli_current->index = list_start_ptr;
                rli_current->hash = repl_hash;
                rli_current->list_size = intf_count;
                rli_current->next = REPL_LIST_INFO(unit);
                REPL_LIST_INFO(unit) = rli_current;
                new_repl_list = TRUE;
            } else {
                rv = BCM_E_INTERNAL;
                goto intf_set_done;
            }
        }

        /* Update the interface count before updating replication list
         * start pointer, since _bcm_tr3_repl_list_start_ptr_set
         * depends on correct interface count.
         */
        REPL_PORT_GROUP_INTF_COUNT(unit, port, repl_group) = intf_count;

        /* Update replication list start pointer */
        rv = _bcm_tr3_repl_list_start_ptr_set(unit, repl_group, port,
                list_start_ptr, intf_count);
        if (BCM_FAILURE(rv)) {
            if (new_repl_list) {
                _bcm_tr3_repl_list_free(unit, list_start_ptr);
                REPL_LIST_INFO(unit) = rli_current->next;
                sal_free(rli_current);
            }
            REPL_PORT_GROUP_INTF_COUNT(unit, port, repl_group) = intf_cnt_old;
            goto intf_set_done;
        }
        (rli_current->refcount)++;
       

    } else { 
        if (!SOC_IS_KATANA2(unit)) {
            rv = _bcm_tr3_repl_intf_vec_construct(unit, port, if_count,
                                                  if_array, if_updated,
                                                  is_l3, check_port, NULL);
        }
        /* Delete replication list for given (repl_group, port) */
        REPL_PORT_GROUP_INTF_COUNT(unit, port, repl_group) = 0;
        if (prev_start_ptr != 0) {
            rv = _bcm_tr3_repl_list_start_ptr_set(unit, repl_group, port, 0, 0);
            if (BCM_FAILURE(rv)) {
                REPL_PORT_GROUP_INTF_COUNT(unit, port,
                                           repl_group) = intf_cnt_old;
                goto intf_set_done;
            }
        }        
    }

    /* Delete old replication list */
    if (prev_start_ptr != 0) {
        rli_prev = NULL;
        for (rli_current = REPL_LIST_INFO(unit); rli_current != NULL;
                rli_current = rli_current->next) {
            if (prev_start_ptr == rli_current->index) {
                (rli_current->refcount)--;
                if (rli_current->refcount == 0) {
                    /* Free these linked list entries */
                    _bcm_tr3_repl_list_free(unit, prev_start_ptr);
                    if (rli_prev == NULL) {
                        REPL_LIST_INFO(unit) = rli_current->next;
                    } else {
                        rli_prev->next = rli_current->next;
                    }
                    sal_free(rli_current);
                }
                break;
            }
            rli_prev = rli_current;
        }
    }

intf_set_done:

    REPL_UNLOCK(unit);

    if (intf_vec != NULL) {
        sal_free(intf_vec);
    }

    return rv;
}
#ifdef BCM_KATANA2_SUPPORT
/*
 * Function:
 *      _bcm_kt2_init_check
 * Purpose:
 *      Katana2 initialization 
 *      unit         - StrataSwitch PCI device unit number.
 *      ipmc_id      - The replication group number.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_kt2_init_check(int unit, int ipmc_id)
{
   REPL_INIT(unit);
   REPL_GROUP_ID(unit, ipmc_id);
   return BCM_E_NONE;

}

/*
 * Function:
 *	_bcm_kt2_ipmc_egress_intf_set
 * Purpose:
 *	Assign set of egress interfaces to port's replication list for chosen
 *	replication group.
 * Parameters:
 *	unit         - StrataSwitch PCI device unit number.
 *	repl_group   - The replication group number.
 *	port         - Port to assign replication list.
 *      ipmc_ptr     - 
 *	if_count     - Number of interfaces in replication list.
 *      if_array     - (IN) List of interface numbers.
 *      new_ipmc_ptr - (OUT) repl_ptr 
 *      last_ipmc_flag - (OUT) flag indicating last replication
 *      check_port - (IN) If if_array consists of L3 interfaces, this parameter 
 *                        controls whether to check the given port is a member
 *                        in each L3 interface's VLAN. This check should be  
 *                        disabled when not needed, in order to improve
 *                        performance.
 * Returns:
 *	BCM_E_XXX
 */
int
_bcm_kt2_ipmc_egress_intf_set(int unit, int repl_group, bcm_port_t port,
                             int ipmc_ptr, int if_count, bcm_if_t *if_array, 
                             int *new_ipmc_ptr, int *last_ipmc_flag,
                             int check_port)
{
    int rv = BCM_E_NONE;
    int prev_start_ptr, list_start_ptr=0;
    int alloc_size;
    SHR_BITDCL *intf_vec = NULL;
    int repl_hash;
    _bcm_repl_list_info_t *rli_current, *rli_prev;
    int intf_count = 0;
    int last_flag;
    uint32 nh_count = 0;
    int remap_id;
    int if_num, partition;
    bcm_l3_intf_t       l3_intf;
    pbmp_t              pbmp, ubmp;
    int set_repl_list;


    REPL_INIT(unit);
    REPL_GROUP_ID(unit, repl_group);
    REPL_PORT_CHECK(unit, port);

    if (if_count > REPL_INTF_TOTAL(unit)) {
        return BCM_E_PARAM;
    } else if (if_count > 0) {
        set_repl_list = TRUE;
    } else {
        set_repl_list = FALSE;
    }

    REPL_LOCK(unit);

    if ((rv = _bcm_kt_ipmc_mmu_mc_remap_ptr(unit,
                      0, &remap_id, FALSE)) < 0) {
       goto intf_set_done;
    }

    prev_start_ptr = ipmc_ptr;

    if (set_repl_list == FALSE) {
        /* update the intf count of remapped  multicast group index */
        REPL_PORT_GROUP_INTF_COUNT(unit, port, remap_id) = 0;
        *new_ipmc_ptr = 0;
    } else {

        partition = soc_mem_index_count(unit, EGR_L3_INTFm);

        alloc_size = SHR_BITALLOCSIZE(REPL_INTF_TOTAL(unit));
        intf_vec = sal_alloc(alloc_size, "Repl interface vector");
        if (intf_vec == NULL) {
            rv = BCM_E_MEMORY;
            goto intf_set_done;
        }
        sal_memset(intf_vec, 0, alloc_size);

        /* Interface validation and vector construction */
        for (if_num = 0; if_num < if_count; if_num++) {
            if (!BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, if_array[if_num]) &&
                (uint32)(if_array[if_num]) > partition) {
                rv = BCM_E_PARAM;
                goto intf_set_done;
            }
            if (!BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, if_array[if_num])) {
                /* L3 interface is used */
                if (check_port) {
                    bcm_l3_intf_t_init(&l3_intf);
                    l3_intf.l3a_intf_id = if_array[if_num];
                    if ((rv = bcm_esw_l3_intf_get(unit, &l3_intf)) < 0) {
                        goto intf_set_done;
                    }
                    if ((rv = bcm_esw_vlan_port_get(unit, l3_intf.l3a_vid,
                                    &pbmp, &ubmp)) < 0) {
                        goto intf_set_done;
                    }
                    if (!BCM_PBMP_MEMBER(pbmp, port)) {
                        rv = BCM_E_PARAM;
                        goto intf_set_done;
                    }
                }
                SHR_BITSET(intf_vec, if_array[if_num]);
                intf_count++;
            } else {
                /* Next hop is used */
                SHR_BITSET(intf_vec, partition + if_array[if_num] -
                           BCM_XGS3_DVP_EGRESS_IDX_MIN);
                nh_count++;
            }
        }

        /* Search for list already in table */
        repl_hash = _shr_crc32b(0, (uint8 *)intf_vec, REPL_INTF_TOTAL(unit));
        for (rli_current = REPL_LIST_INFO(unit); rli_current != NULL;
            rli_current = rli_current->next) {
            if (repl_hash == rli_current->hash) {
                rv = _bcm_kt2_repl_list_compare(unit, rli_current->index,
                          intf_vec, nh_count);
                if (rv == BCM_E_NOT_FOUND) {
                    continue; /* Not a match */
                } else if (BCM_FAILURE(rv)) {
                    goto intf_set_done;
                } else {
                    break; /* Match */
                }
            }
        }

        if (rli_current != NULL) {
            /* Found a match */
            if (prev_start_ptr == rli_current->index) {
                /* (repl_group, port) already points to this list, so done */
                rv = BCM_E_NONE;
                goto intf_set_done;
            } else {
                list_start_ptr = rli_current->index;
                intf_count = rli_current->list_size;
            }
        } else {
            /* Not a match, make a new chain */
            rv = _bcm_kt2_repl_list_write(unit,
                     intf_vec, nh_count, &list_start_ptr, &intf_count);
            if (BCM_FAILURE(rv)) {
                goto intf_set_done;
            }

            if (intf_count > 0) {
                /* Update data structures */
                alloc_size = sizeof(_bcm_repl_list_info_t);
                rli_current = sal_alloc(alloc_size, "IPMC repl list info");
                if (rli_current == NULL) {
                    /* Release list */
                    _bcm_tr3_repl_list_free(unit, list_start_ptr);
                    rv = BCM_E_MEMORY;
                    goto intf_set_done;
                }
                sal_memset(rli_current, 0, alloc_size);
                rli_current->index = list_start_ptr;
                rli_current->hash = repl_hash;
                rli_current->list_size = intf_count;
                rli_current->next = REPL_LIST_INFO(unit);
                REPL_LIST_INFO(unit) = rli_current;
            } else {
                rv = BCM_E_INTERNAL;
                goto intf_set_done;
            }
        }

        last_flag = (intf_count == 1);
        *last_ipmc_flag = last_flag;
        if (intf_count > 0) {
            (rli_current->refcount)++;
        } else if (prev_start_ptr != 0) {
            list_start_ptr = 0;
        }
        *new_ipmc_ptr = list_start_ptr;

        /* Update the interface count before updating replication list
         * start pointer, since _bcm_tr3_repl_list_start_ptr_set
         * depends on correct interface count.
         */
        REPL_PORT_GROUP_INTF_COUNT(unit, port, remap_id) = intf_count;
    }

    /* Delete old replication list */
    if (prev_start_ptr != 0) {
        rli_prev = NULL;
        for (rli_current = REPL_LIST_INFO(unit); rli_current != NULL;
                rli_current = rli_current->next) {
            if (prev_start_ptr == rli_current->index) {
                (rli_current->refcount)--;
                if (rli_current->refcount == 0) {
                    /* Free these linked list entries */
                    _bcm_tr3_repl_list_free(unit, prev_start_ptr);
                    if (rli_prev == NULL) {
                        REPL_LIST_INFO(unit) = rli_current->next;
                    } else {
                        rli_prev->next = rli_current->next;
                    }
                    sal_free(rli_current);
                }
                break;
            }
            rli_prev = rli_current;
        }
    }

intf_set_done:

    REPL_UNLOCK(unit);

    if (intf_vec != NULL) {
        sal_free(intf_vec);
    }

    return rv;
}

/*
 * Function:
 *      _bcm_kt2_ipmc_egress_intf_add
 * Purpose:
 *      Add L3 interface to selected ports' replication list for chosen
 *      IPMC group.
 * Parameters:
 *      unit         - StrataSwitch PCI device unit number.
 *      ipmc_id      - IPMC group number.
 *      port         - port to add.
 *      ipmc_ptr     - replication pointer.
 *      id           - encap_id.
 *      new_ipmc_ptr - (OUT) repl_ptr
 *      last_ipmc_flag - (OUT) flag indicating last replication
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_kt2_ipmc_egress_intf_add(int unit, int ipmc_id, bcm_port_t port,
                              int ipmc_ptr, int id, int  *new_ipmc_ptr,
                              int *last_flag)
{
    bcm_if_t *if_array = NULL;
    int  intf_num, intf_max, alloc_size, rv = BCM_E_NONE;
    pbmp_t pbmp, ubmp;
    int                 partition;
    bcm_l3_intf_t       l3_intf;


    REPL_PORT_CHECK(unit, port);
    BCM_IF_ERROR_RETURN(_bcm_kt2_init_check(unit, ipmc_id));

    intf_max = REPL_INTF_TOTAL(unit);
    alloc_size = intf_max * sizeof(bcm_if_t);
    if_array = sal_alloc(alloc_size, "IPMC repl interface array");
    if (if_array == NULL) {
        return BCM_E_MEMORY;
    }

    REPL_LOCK(unit);
    rv = _bcm_kt2_ipmc_egress_intf_get(unit, ipmc_id, ipmc_ptr,
                                       if_array, &intf_num);
    if (BCM_SUCCESS(rv)) {
        if (intf_num < intf_max) {
            if_array[intf_num++] = id;

            /* For IPMC, check port is a member of the L3 interface's VLAN.
             * Performing this check here is more efficient than performing
             * it in bcm_tr2_ipmc_egress_intf_set.
             */
            if (!BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, id)) {
                /* L3 interface is used */
                partition = soc_mem_index_count(unit, EGR_L3_INTFm);
                if (id > partition) {
                    rv = BCM_E_PARAM;
                    goto intf_add_done;
                }

                bcm_l3_intf_t_init(&l3_intf);
                l3_intf.l3a_intf_id = id;
                if ((rv = bcm_esw_l3_intf_get(unit, &l3_intf)) < 0) {
                    goto intf_add_done;
                }
                if ((rv = bcm_esw_vlan_port_get(unit, l3_intf.l3a_vid,
                                &pbmp, &ubmp)) < 0) {
                    goto intf_add_done;
                }
                if (!BCM_PBMP_MEMBER(pbmp, port)) {
                    rv = BCM_E_PARAM;
                    goto intf_add_done;
                }
            }

            rv = _bcm_kt2_ipmc_egress_intf_set(unit, ipmc_id, port, 
                                               ipmc_ptr, intf_num, if_array, 
                                               new_ipmc_ptr, last_flag, FALSE);
        } else {
            rv = BCM_E_EXISTS;
        }
    }

intf_add_done:
    REPL_UNLOCK(unit);
    sal_free(if_array);
    return rv;
}

/*
 * Function:
 *      bcm_kt2_ipmc_egress_intf_get
 * Purpose:
 *      Retrieve set of egress interfaces to port's replication list for chosen
 *      replication list.
 * Parameters:
 *      unit       - StrataSwitch PCI device unit number.
 *      repl_group - The replication group number.
 *      ipmc_ptr   - Relication pointer
 *      if_array   - (OUT) List of interface numbers.
 *      if_count   - (OUT) Number of interfaces in replication list.
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If the input parameter if_max = 0, return in the output parameter
 *      if_count the total number of interfaces in the specified multicast
 *      group's replication list.
 */
int 
_bcm_kt2_ipmc_egress_intf_get(int unit, int repl_group, 
                             int ipmc_ptr, bcm_if_t *if_array, 
                             int *if_count)
{
    int rv = BCM_E_NONE;
    int prev_repl_entry_ptr, repl_entry_ptr;
    int intf_count;
    int intf_base;
    int ls_pos;
    uint32 ls_bits[2];
    mmu_repl_list_tbl_entry_t repl_list_entry;
    uint32 nh_offset = 0;
    uint32  is_len = 0;
    int if_max =0;


    BCM_IF_ERROR_RETURN(_bcm_kt2_init_check(unit, repl_group));

    REPL_LOCK(unit);

    is_len = soc_mem_field_length(unit, MMU_REPL_LIST_TBLm, LSB_VLAN_BMf);
    nh_offset = (1 << (soc_mem_field_length(unit, MMU_REPL_LIST_TBLm,
                                                MSB_VLANf) - 1)) * is_len;
    if_max = REPL_INTF_TOTAL(unit);

    if (if_max < 0) {
        return BCM_E_PARAM;
    } else if (if_max > 0 && NULL == if_array) {
        return BCM_E_PARAM;
    }

    if (NULL == if_count) {
        return BCM_E_PARAM;
    }

    prev_repl_entry_ptr = -1;
    intf_count = 0;
    repl_entry_ptr = ipmc_ptr;

    while (repl_entry_ptr != prev_repl_entry_ptr) {
        rv = READ_MMU_REPL_LIST_TBLm(unit, MEM_BLOCK_ANY,
                repl_entry_ptr, &repl_list_entry);
        if (BCM_FAILURE(rv)) {
            goto intf_get_done;
        }
        /* Each MSB represents 64 entries in LSB bitmap */
        intf_base = soc_MMU_REPL_LIST_TBLm_field32_get(unit, &repl_list_entry,
                MSB_VLANf) * 64;
        soc_MMU_REPL_LIST_TBLm_field_get(unit, &repl_list_entry,
                LSB_VLAN_BMf, ls_bits);

        for (ls_pos = 0; ls_pos < 64; ls_pos++) {
            if (ls_bits[ls_pos / 32] & (1 << (ls_pos % 32))) {
                if (if_max == 0) {
                    intf_count++;
                } else {
                        /* check if L3 interface or nexthop */
                        if (intf_base >= nh_offset) {
                            /* Entry contains next hops */
                            if_array[intf_count] = intf_base - nh_offset +
                                ls_pos + BCM_XGS3_DVP_EGRESS_IDX_MIN;
                        } else {
                            /* Entry contains interfaces */
                            if_array[intf_count] = intf_base + ls_pos;
                        }
                    intf_count++;
                    if (intf_count == if_max) {
                        *if_count = intf_count;
                        goto intf_get_done;
                    }
                }
            } 
        } 
        prev_repl_entry_ptr = repl_entry_ptr;
        repl_entry_ptr = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                &repl_list_entry, NEXTPTRf);
    }

    *if_count = intf_count;

intf_get_done:
    REPL_UNLOCK(unit);
    return rv;

}

/*
 * Function:
 *      bcm_kt2_ipmc_egress_intf_delete
 * Purpose:
 *      Remove L3 interface from selected ports' replication list for chosen
 *      Relication list.
 * Parameters:
 *      unit     - StrataSwitch PCI device unit number.
 *      ipmc_id  - IPMC group number.
 *      ipmc_ptr     - replication pointer.
 *      id           - encap_id.
 *      new_ipmc_ptr - (OUT) repl_ptr
 *      last_ipmc_flag - (OUT) flag indicating last replication
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_kt2_ipmc_egress_intf_delete(int unit, int ipmc_id, bcm_port_t port,
                                int ipmc_ptr, int id, int  *new_ipmc_ptr,
                                int *last_flag)
{
    bcm_if_t *if_array = NULL;
    int alloc_size, intf_max, if_count, if_cur, match, rv = BCM_E_NONE;

    REPL_INIT(unit);
    REPL_GROUP_ID(unit, ipmc_id);
    REPL_PORT_CHECK(unit, port);

    if (!REPL_PORT_GROUP_INTF_COUNT(unit, port, ipmc_id)) {
        return BCM_E_NOT_FOUND;
    }

    intf_max = REPL_INTF_TOTAL(unit);
    alloc_size = intf_max * sizeof(bcm_if_t);
    if_array = sal_alloc(alloc_size, "IPMC repl interface array");
    if (if_array == NULL) {
        return BCM_E_MEMORY;
    }

    REPL_LOCK(unit);
    rv = _bcm_kt2_ipmc_egress_intf_get(unit, ipmc_id, ipmc_ptr,
                                       if_array, &if_count);
    if (BCM_SUCCESS(rv)) {
        match = FALSE;
        for (if_cur = 0; if_cur < if_count; if_cur++) {
            if (match) {
                if_array[if_cur - 1] = if_array[if_cur];
            } else {
                if (if_array[if_cur] == id) {
                    match = TRUE;
                }
            }
        }
        if (match) {
            if_count--;
            rv = _bcm_kt2_ipmc_egress_intf_set(unit, ipmc_id, port,
                                             ipmc_ptr, if_count, if_array,
                                            new_ipmc_ptr, last_flag, FALSE);
        } else {
            rv = BCM_E_NOT_FOUND;
        }
    }

    REPL_UNLOCK(unit);
    sal_free(if_array);
    return rv;
}

#endif
/*
 * Function:
 *	bcm_tr3_ipmc_egress_intf_get
 * Purpose:
 *	Retrieve set of egress interfaces to port's replication list for chosen
 *	replication group.
 * Parameters:
 *	unit       - StrataSwitch PCI device unit number.
 *	repl_group - The replication group number.
 *	port       - Port from which to get replication list.
 *	if_max     - Maximum number of interfaces in replication list.
 *      if_array   - (OUT) List of interface numbers.
 *	if_count   - (OUT) Number of interfaces in replication list.
 * Returns:
 *	BCM_E_XXX
 * Notes:
 *      If the input parameter if_max = 0, return in the output parameter
 *      if_count the total number of interfaces in the specified multicast 
 *      group's replication list.
 */
int
bcm_tr3_ipmc_egress_intf_get(int unit, int repl_group, bcm_port_t port,
                            int if_max, bcm_if_t *if_array, int *if_count)
{
    int rv = BCM_E_NONE;
    int prev_repl_entry_ptr, repl_entry_ptr;
    int intf_count;
    int l3_intf, intf_base;
    int ls_pos;
    uint32 ls_bits[2];
    mmu_repl_list_tbl_entry_t repl_list_entry;
    int next_hop_index;
    int entry_type;
    egr_l3_next_hop_entry_t egr_nh;
#if defined (BCM_KATANA2_SUPPORT)
    uint32 nh_offset = 0;
    uint32  is_len = 0;
#endif


    REPL_INIT(unit);
    REPL_GROUP_ID(unit, repl_group);
    /* Replication is allowed on CPU ports, only on TD2 */
    if(IS_CPU_PORT(unit, port)) {
        if(!SOC_IS_TRIDENT2(unit)) {
            return BCM_E_PARAM;
        }
    }
    else
    {   
        REPL_PORT_CHECK(unit, port);
    }

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit)) {
        is_len = soc_mem_field_length(unit, MMU_REPL_LIST_TBLm, LSB_VLAN_BMf);
        nh_offset = (1 << (soc_mem_field_length(unit, MMU_REPL_LIST_TBLm,
                                                MSB_VLANf) - 1)) * is_len;
    }
#endif

    if (if_max < 0) {
        return BCM_E_PARAM;
    } else if (if_max > 0 && NULL == if_array) {
        return BCM_E_PARAM;
    }

    if (NULL == if_count) {
        return BCM_E_PARAM;
    }


    REPL_LOCK(unit);

    if (REPL_PORT_GROUP_INTF_COUNT(unit, port, repl_group) == 0) {
        *if_count = 0;
        REPL_UNLOCK(unit);
        return BCM_E_NONE;
    }

    rv = _bcm_tr3_repl_list_start_ptr_get(unit, repl_group, port,
            &repl_entry_ptr);
    if (BCM_FAILURE(rv)) {
        goto intf_get_done;
    }

    prev_repl_entry_ptr = -1;
    intf_count = 0;
    while (repl_entry_ptr != prev_repl_entry_ptr) {
        rv = READ_MMU_REPL_LIST_TBLm(unit, MEM_BLOCK_ANY,
                repl_entry_ptr, &repl_list_entry);
        if (BCM_FAILURE(rv)) {
            goto intf_get_done;
        }
        /* Each MSB represents 64 entries in LSB bitmap */
        intf_base = soc_MMU_REPL_LIST_TBLm_field32_get(unit, &repl_list_entry,
                MSB_VLANf) * 64;
        soc_MMU_REPL_LIST_TBLm_field_get(unit, &repl_list_entry,
                LSB_VLAN_BMf, ls_bits);

        for (ls_pos = 0; ls_pos < 64; ls_pos++) {
            if (ls_bits[ls_pos / 32] & (1 << (ls_pos % 32))) {
                if (if_max == 0) {
                    intf_count++;
                } else {
#if defined(BCM_KATANA2_SUPPORT)
                    if (SOC_IS_KATANA2(unit))
                    {
                        /* check if L3 interface or nexthop */
                        if (intf_base >= nh_offset) {
                            /* Entry contains next hops */
                            if_array[intf_count] = intf_base - nh_offset +
                                ls_pos + BCM_XGS3_DVP_EGRESS_IDX_MIN;
                        } else {
                            /* Entry contains interfaces */
                            if_array[intf_count] = intf_base + ls_pos;
                        }
                    }
                    else
#endif
                    {

                        /* Check if L3 interfaces or next hop */
                    
                        next_hop_index = intf_base + ls_pos;
                        if_array[intf_count] = next_hop_index +
                         BCM_XGS3_DVP_EGRESS_IDX_MIN;                

                        /* Check if next hop index corresponds to
                         * an IPMC or Trill interface.
                         */
                        rv = READ_EGR_L3_NEXT_HOPm(unit, MEM_BLOCK_ANY,
                             next_hop_index, &egr_nh);
                        if (BCM_FAILURE(rv)) {
                            goto intf_get_done;
                        }
                        entry_type = soc_EGR_L3_NEXT_HOPm_field32_get(unit,
                                     &egr_nh, ENTRY_TYPEf);
                        if (entry_type == 0) {
                            l3_intf = soc_EGR_L3_NEXT_HOPm_field32_get(unit,
                                &egr_nh, L3__INTF_NUMf);
                            if (REPL_L3_INTF_NEXT_HOP_TRILL(unit, l3_intf) ==
                                next_hop_index) {
                                if_array[intf_count] = l3_intf;
                            } 
                        }
                        else if (entry_type == 7) {
                            l3_intf = soc_EGR_L3_NEXT_HOPm_field32_get(unit,
                                  &egr_nh, L3MC__INTF_NUMf);
                            if (REPL_L3_INTF_NEXT_HOP_IPMC(unit, l3_intf) ==
                                  next_hop_index) {
                               if_array[intf_count] = l3_intf;
                            } 
                        }
                    } /* end - if not KATANA2*/
                    intf_count++;
                    if (intf_count == if_max) {
                        *if_count = intf_count;
                        goto intf_get_done;
                    }
                }
            } 
        } 
        prev_repl_entry_ptr = repl_entry_ptr;
        repl_entry_ptr = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                &repl_list_entry, NEXTPTRf);
        if (intf_count >= REPL_PORT_GROUP_INTF_COUNT(unit, port, repl_group)) {
            break;
        }
    }

    *if_count = intf_count;

intf_get_done:
    REPL_UNLOCK(unit);
    return rv;
}


/*
 * Function:
 *	bcm_tr3_ipmc_egress_intf_set
 * Purpose:
 *	Assign set of egress interfaces to port's replication list for chosen
 *	replication group.
 * Parameters:
 *	unit       - StrataSwitch PCI device unit number.
 *	repl_group - The replication group number.
 *	port       - Port to assign replication list.
 *	if_count   - Number of interfaces in replication list.
 *  if_array   - (IN) List of interface numbers.
 *  is_l3      - (IN) Indicates if multicast group is of type IPMC.
 *  check_port - (IN) If if_array consists of L3 interfaces, this parameter 
 *                        controls whether to check the given port is a member
 *                        in each L3 interface's VLAN. This check should be  
 *                        disabled when not needed, in order to improve
 *                        performance.
 * Returns:
 *	BCM_E_XXX
 */
int 
bcm_tr3_ipmc_egress_intf_set(
    int unit, int repl_group, bcm_port_t port,
    int if_count, bcm_if_t *if_array, int is_l3, int check_port)
{
    int rv = BCM_E_NONE;
    _tr3_if_updated_t  if_updated;
    bcm_if_t *if_array_cur = NULL;
    int intf_num_cur, intf_max, alloc_size;
    int if_index, if_cur_index;
    
    sal_memset(&if_updated, 0 , sizeof(if_updated));
    
    intf_max = REPL_INTF_TOTAL(unit);
    alloc_size = intf_max * sizeof(bcm_if_t);
    
    if_array_cur = sal_alloc(alloc_size, "IPMC intf array current");
    if (if_array_cur == NULL) {
        rv = BCM_E_MEMORY;
        goto intf_cleanup;
    }
    sal_memset(if_array_cur, 0, alloc_size);

    if_updated.if_array_new = sal_alloc(if_count * sizeof(bcm_if_t),
                                        "IPMC intf array new");
    if (if_updated.if_array_new == NULL) {
        rv = BCM_E_MEMORY;   
        goto intf_cleanup;
    }
    sal_memset(if_updated.if_array_new, 0, if_count * sizeof(bcm_if_t));

    REPL_LOCK(unit);
    rv = bcm_tr3_ipmc_egress_intf_get(unit, repl_group, port,
                                      intf_max, if_array_cur, &intf_num_cur);
    if (BCM_FAILURE(rv)) {
        REPL_UNLOCK(unit);
        goto intf_cleanup;
    }
    
    if_updated.if_array_del = sal_alloc(intf_num_cur * sizeof(bcm_if_t),
                                        "IPMC intf array del");
    if (if_updated.if_array_del == NULL) {
        REPL_UNLOCK(unit);
        rv = BCM_E_MEMORY;
        goto intf_cleanup;
    }
    sal_memset(if_updated.if_array_del, 0, intf_num_cur * sizeof(bcm_if_t));

    /* Get the new if array for <group, port> and 
     * the if array deleted from <group, port>.
     */
    for (if_index = 0; if_index < if_count; if_index++) {
        for (if_cur_index = 0; if_cur_index < intf_num_cur; if_cur_index++) {
            if (if_array[if_index] == if_array_cur[if_cur_index]) {
                /* The if_array[if_index] has been already
                 * attached to <group, port>. 
                 */ 
                if_array_cur[if_cur_index] = -1;
                break;
            }
        }
        
        if (if_cur_index == intf_num_cur) {
            /* The if_array[if_index] is new for <group, port>. */
            if_updated.if_array_new[if_updated.if_count_new++] = 
                if_array[if_index];
        }        
    }

    for (if_cur_index = 0; if_cur_index < intf_num_cur; if_cur_index++) {
        if (if_array_cur[if_cur_index] == -1) {
            continue;
        }
        if_updated.if_array_del[if_updated.if_count_del++] = 
            if_array_cur[if_cur_index];
    }

    rv = _bcm_tr3_ipmc_egress_intf_set(unit, repl_group, port,
                                       if_count,if_array, &if_updated,
                                       is_l3, check_port);
    REPL_UNLOCK(unit);
intf_cleanup:
    sal_free(if_array_cur);
    sal_free(if_updated.if_array_new);
    sal_free(if_updated.if_array_del);
    return rv;
}


/*
 * Function:
 *      bcm_tr3_ipmc_egress_intf_add
 * Purpose:
 *      Add L3 interface to selected ports' replication list for chosen
 *      IPMC group.
 * Parameters:
 *      unit     - StrataSwitch PCI device unit number.
 *      ipmc_id  - IPMC group number.
 *      port     - port to add.
 *      l3_intf  - L3 interface to replicate.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_egress_intf_add(int unit, int ipmc_id, bcm_port_t port,
                            bcm_l3_intf_t *l3_intf)
{
    bcm_if_t *if_array = NULL;
    int  intf_num, intf_max, alloc_size, rv = BCM_E_NONE;
    pbmp_t pbmp, ubmp;
    bcm_if_t intf_add;
    _tr3_if_updated_t if_updated;

    REPL_INIT(unit);
    REPL_GROUP_ID(unit, ipmc_id);
    REPL_PORT_CHECK(unit, port);

    /* Check if port belongs to this VLAN */
    BCM_IF_ERROR_RETURN
        (bcm_esw_vlan_port_get(unit, l3_intf->l3a_vid, &pbmp, &ubmp));
    if (!SOC_PBMP_MEMBER(pbmp, port)) {
        return BCM_E_PARAM;
    }

    intf_max = REPL_INTF_TOTAL(unit);
    alloc_size = intf_max * sizeof(bcm_if_t);
    if_array = sal_alloc(alloc_size, "IPMC repl interface array");
    if (if_array == NULL) {
        return BCM_E_MEMORY;
    }

    REPL_LOCK(unit);
    rv = bcm_tr3_ipmc_egress_intf_get(unit, ipmc_id, port, intf_max,
                                     if_array, &intf_num);
    if (BCM_SUCCESS(rv)) {
        if (intf_num < intf_max) {
            if_array[intf_num++] = l3_intf->l3a_intf_id;
            intf_add = l3_intf->l3a_intf_id;
            if_updated.if_array_new = &intf_add;
            if_updated.if_count_new = 1;            
            rv = _bcm_tr3_ipmc_egress_intf_set(unit, ipmc_id,
                                               port,intf_num,if_array,
                                               &if_updated, TRUE, FALSE);
        } else {
            rv = BCM_E_EXISTS;
        }
    }

    REPL_UNLOCK(unit);
    sal_free(if_array);
    return rv;
}

/*
 * Function:
 *      bcm_tr3_ipmc_egress_intf_delete
 * Purpose:
 *      Remove L3 interface from selected ports' replication list for chosen
 *      IPMC group.
 * Parameters:
 *      unit     - StrataSwitch PCI device unit number.
 *      ipmc_id  - IPMC group number.
 *      port     - port to remove.
 *      l3_intf  - L3 interface to delete from replication.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_egress_intf_delete(int unit, int ipmc_id, bcm_port_t port,
                               bcm_l3_intf_t *l3_intf)
{
    bcm_if_t *if_array = NULL;
    int alloc_size, intf_max, if_count, if_cur, match, rv = BCM_E_NONE;
    _tr3_if_updated_t if_updated;
    bcm_if_t intf_del;
    
    REPL_INIT(unit);
    REPL_GROUP_ID(unit, ipmc_id);
    /* Replication is allowed on CPU ports, only on TD2 */
    if(IS_CPU_PORT(unit, port)) {
        if(!SOC_IS_TRIDENT2(unit)) {
            return BCM_E_PARAM;
        }
    }
    else
    {   
        REPL_PORT_CHECK(unit, port);
    }

    if (!REPL_PORT_GROUP_INTF_COUNT(unit, port, ipmc_id)) {
        return BCM_E_NOT_FOUND;
    }

    intf_max = REPL_INTF_TOTAL(unit);
    alloc_size = intf_max * sizeof(bcm_if_t);
    if_array = sal_alloc(alloc_size, "IPMC repl interface array");
    if (if_array == NULL) {
        return BCM_E_MEMORY;
    }

    REPL_LOCK(unit);
    rv = bcm_tr3_ipmc_egress_intf_get(unit, ipmc_id, port, intf_max,
                                     if_array, &if_count);
    if (BCM_SUCCESS(rv)) {
        match = FALSE;
        for (if_cur = 0; if_cur < if_count; if_cur++) {
            if (match) {
                if_array[if_cur - 1] = if_array[if_cur];
            } else {
                if (if_array[if_cur] == l3_intf->l3a_intf_id) {
                    match = TRUE;
                }
            }
        }
        if (match) {
            if_count--;
            intf_del = l3_intf->l3a_intf_id;
            if_updated.if_count_del = 1;
            if_updated.if_array_del = &intf_del;
            rv = _bcm_tr3_ipmc_egress_intf_set(unit, ipmc_id, port,
                                               if_count, if_array,
                                               &if_updated,TRUE, FALSE);
        } else {
            rv = BCM_E_NOT_FOUND;
        }
    }

    REPL_UNLOCK(unit);
    sal_free(if_array);
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_ipmc_repl_get
 * Purpose:
 *      Return set of VLANs selected for port's replication list for chosen
 *      IPMC group.
 * Parameters:
 *      unit     - StrataSwitch PCI device unit number.
 *      ipmc_id  - IPMC group number.
 *      port     - port to list.
 *      vlan_vec - (OUT) vector of replicated VLANs common to selected ports.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_repl_get(int unit, int ipmc_id, bcm_port_t port,
                        bcm_vlan_vector_t vlan_vec)
{
    int rv = BCM_E_NONE;
    uint32 ls_bits[2];
    int prev_repl_entry_ptr, repl_entry_ptr;
    int intf_base;
    int ls_pos;
    mmu_repl_list_tbl_entry_t repl_list_entry;
    int next_hop_index;
    int entry_type;
    bcm_l3_intf_t l3_intf;
    bcm_l3_egress_t egress_object;
    bcm_if_t egr_if;
    int vlan_count;
    egr_l3_next_hop_entry_t egr_nh;
#ifdef BCM_KATANA2_SUPPORT
    uint32 nh_offset = 0;
    uint32 ls_len = 0;
#endif /* BCM_KATANA2_SUPPORT*/

    REPL_INIT(unit);
    REPL_GROUP_ID(unit, ipmc_id);
    REPL_PORT_CHECK(unit, port);

    BCM_VLAN_VEC_ZERO(vlan_vec);

#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(unit))
    {
        ls_len  = soc_mem_field_length(unit, MMU_REPL_LIST_TBLm, LSB_VLAN_BMf);
        nh_offset = (1 << (soc_mem_field_length(unit, MMU_REPL_LIST_TBLm,
                                                MSB_VLANf) - 1)) * ls_len;
    }
#endif

    REPL_LOCK(unit);
    if (REPL_PORT_GROUP_INTF_COUNT(unit, port, ipmc_id) == 0) {
        REPL_UNLOCK(unit);
        return BCM_E_NONE;
    }

    rv = _bcm_tr3_repl_list_start_ptr_get(unit, ipmc_id, port,
            &repl_entry_ptr);
    if (BCM_FAILURE(rv)) {
        goto vlan_get_done;
    }

    prev_repl_entry_ptr = -1;
    vlan_count = 0;
    while (repl_entry_ptr != prev_repl_entry_ptr) {
        rv = READ_MMU_REPL_LIST_TBLm(unit, MEM_BLOCK_ANY,
                repl_entry_ptr, &repl_list_entry);
        if (BCM_FAILURE(rv)) {
            goto vlan_get_done;
        }
        /* Each MSB represents 64 entries in LSB bitmap */
        intf_base = soc_MMU_REPL_LIST_TBLm_field32_get(unit, &repl_list_entry,
                MSB_VLANf) * 64;
        soc_MMU_REPL_LIST_TBLm_field_get(unit, &repl_list_entry,
                LSB_VLAN_BMf, ls_bits);
        for (ls_pos = 0; ls_pos < 64; ls_pos++) {
            if (ls_bits[ls_pos / 32] & (1 << (ls_pos % 32))) {
#ifdef BCM_KATANA2_SUPPORT
                if (SOC_IS_KATANA2(unit)) {
                    bcm_l3_intf_t_init(&l3_intf);
                    if (intf_base >= nh_offset) {
                        /* Next hop */
                        next_hop_index = intf_base - nh_offset + ls_pos +
                        BCM_XGS3_DVP_EGRESS_IDX_MIN;
                        /* Coverity: egress_object.intf is initialized above
                         * by bcm_esw_l3_egress_get. */
                        /* coverity[uninit_use : FALSE] */
                        rv = bcm_esw_l3_egress_get(unit, next_hop_index, 
                                                   &egress_object);
                        if (BCM_FAILURE(rv)) {
                            goto vlan_get_done;
                        }
                        l3_intf.l3a_intf_id = egress_object.intf;

                    } else {
                        /* L3 Interface */
                        l3_intf.l3a_intf_id  = intf_base + ls_pos;
                    }                    
                } else 
#endif
                {
                    next_hop_index = intf_base + ls_pos;
                    rv = READ_EGR_L3_NEXT_HOPm(unit, MEM_BLOCK_ANY,
                          next_hop_index, &egr_nh);
                    if (BCM_FAILURE(rv)) {
                        goto vlan_get_done;
                    }
                    entry_type = soc_EGR_L3_NEXT_HOPm_field32_get(unit,
                          &egr_nh, ENTRY_TYPEf);
                    bcm_l3_intf_t_init(&l3_intf);
                    if (entry_type == 7) {
                        l3_intf.l3a_intf_id = 
                        soc_EGR_L3_NEXT_HOPm_field32_get(unit,
                               &egr_nh, L3MC__INTF_NUMf);
                    } else {
                        egr_if = next_hop_index + BCM_XGS3_DVP_EGRESS_IDX_MIN;
                        rv = bcm_esw_l3_egress_get(unit, 
                               egr_if, &egress_object);
                        if (BCM_FAILURE(rv)) {
                            goto vlan_get_done;
                        }
                        l3_intf.l3a_intf_id = egress_object.intf;
                    }
                } /* END of if not KATANA2 */
                rv = bcm_esw_l3_intf_get(unit, &l3_intf);
                if (BCM_FAILURE(rv)) {
                    goto vlan_get_done;
                }
                BCM_VLAN_VEC_SET(vlan_vec, l3_intf.l3a_vid);
                vlan_count++;
               
            }
        }
        prev_repl_entry_ptr = repl_entry_ptr;
        repl_entry_ptr = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                &repl_list_entry, NEXTPTRf);
        if (vlan_count >= REPL_PORT_GROUP_INTF_COUNT(unit, port, ipmc_id)) {
            break;
        }
    }

vlan_get_done:
    REPL_UNLOCK(unit);
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_ipmc_repl_add
 * Purpose:
 *      Add VLAN to selected ports' replication list for chosen
 *      IPMC group.
 *Parameters:
 *      unit     - StrataSwitch PCI device unit number.
 *      ipmc_id  - IPMC group number.
 *      port     - port to add.
 *      vlan     - VLAN to replicate.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_repl_add(int unit, int ipmc_id, bcm_port_t port,
                     bcm_vlan_t vlan)
{
    int alloc_size, intf_max, if_count, rv = BCM_E_NONE;
    bcm_if_t *if_array = NULL;
    pbmp_t pbmp, ubmp;
    bcm_l3_intf_t l3_intf;
    bcm_if_t intf_add;
    _tr3_if_updated_t if_updated;
    
    REPL_INIT(unit);
    REPL_GROUP_ID(unit, ipmc_id);
    REPL_PORT_CHECK(unit, port);

    /* Check if port belongs to this VLAN */
    BCM_IF_ERROR_RETURN
        (bcm_esw_vlan_port_get(unit, vlan, &pbmp, &ubmp));
    if (!SOC_PBMP_MEMBER(pbmp, port)) {
        return BCM_E_PARAM;
    }

    bcm_l3_intf_t_init(&l3_intf);
    l3_intf.l3a_vid = vlan;
    if (bcm_esw_l3_intf_find_vlan(unit, &l3_intf) < 0) {
        return BCM_E_PARAM;
    }

    REPL_LOCK(unit);

    intf_max = 1 + REPL_PORT_GROUP_INTF_COUNT(unit, port, ipmc_id);
    alloc_size = intf_max * sizeof(bcm_if_t);
    if_array = sal_alloc(alloc_size, "IPMC repl interface array");
    if (if_array == NULL) {
        REPL_UNLOCK(unit);
        return BCM_E_MEMORY;
    }

    rv = bcm_tr3_ipmc_egress_intf_get(unit, ipmc_id, port, intf_max,
                                     if_array, &if_count);
    if (BCM_SUCCESS(rv)) {
        if (if_count < intf_max) {
            if_array[if_count++] = l3_intf.l3a_intf_id;
            intf_add = l3_intf.l3a_intf_id;
            if_updated.if_array_new = &intf_add;
            if_updated.if_count_new = 1;
            rv = _bcm_tr3_ipmc_egress_intf_set(unit, ipmc_id, port,
                                               if_count, if_array,
                                               &if_updated,TRUE, FALSE);
        } else {
            rv = BCM_E_EXISTS;
        }
    }

    REPL_UNLOCK(unit);
    sal_free(if_array);
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_ipmc_repl_delete
 * Purpose:
 *      Remove VLAN from selected ports' replication list for chosen
 *      IPMC group.
 * Parameters:
 *      unit     - StrataSwitch PCI device unit number.
 *      ipmc_id  - IPMC group number.
 *      port     - port to remove.
 *      vlan     - VLAN to delete from replication.
 * Returns:
 *      BCM_E_XXX
 */

int
bcm_tr3_ipmc_repl_delete(int unit, int ipmc_id, bcm_port_t port,
                        bcm_vlan_t vlan)
{
    int alloc_size, intf_max, if_count, if_cur, match, rv = BCM_E_NONE;
    bcm_if_t *if_array = NULL;
    bcm_l3_intf_t l3_intf;
    bcm_if_t intf_del;
    _tr3_if_updated_t if_updated;

    REPL_INIT(unit);
    REPL_GROUP_ID(unit, ipmc_id);
    REPL_PORT_CHECK(unit, port);

    if (!REPL_PORT_GROUP_INTF_COUNT(unit, port, ipmc_id)) {
        return BCM_E_NOT_FOUND;
    }

    bcm_l3_intf_t_init(&l3_intf);
    l3_intf.l3a_vid = vlan;
    if (bcm_esw_l3_intf_find_vlan(unit, &l3_intf) < 0) {
        return BCM_E_PARAM;
    }

    REPL_LOCK(unit);
    intf_max = REPL_PORT_GROUP_INTF_COUNT(unit, port, ipmc_id);
    alloc_size = intf_max * sizeof(bcm_if_t);
    if_array = sal_alloc(alloc_size, "IPMC repl interface array");
    if (if_array == NULL) {
        REPL_UNLOCK(unit);
        return BCM_E_MEMORY;
    }

    rv = bcm_tr3_ipmc_egress_intf_get(unit, ipmc_id, port, intf_max,
                                     if_array, &if_count);
    if (BCM_SUCCESS(rv)) {
        match = FALSE;
        for (if_cur = 0; if_cur < if_count; if_cur++) {
            if (match) {
                if_array[if_cur - 1] = if_array[if_cur];
            } else {
                if (if_array[if_cur] == l3_intf.l3a_intf_id) {
                    match = TRUE;
                }
            }
        }
        if (match) {
            if_count--;
            intf_del = l3_intf.l3a_intf_id;
            if_updated.if_count_del = 1;
            if_updated.if_array_del = &intf_del;
            rv = _bcm_tr3_ipmc_egress_intf_set(unit, ipmc_id, port,
                                               if_count, if_array,
                                               &if_updated, TRUE, FALSE);
        } else {
            rv = BCM_E_NOT_FOUND;
        }
    }
    REPL_UNLOCK(unit);
    sal_free(if_array);
    return rv;
}

/*
 * Function:
 *      bcm_tr3_ipmc_repl_delete_all
 * Purpose:
 *      Remove all VLANs from selected ports' replication list for chosen
 *      IPMC group.
 * Parameters:
 *      unit     - StrataSwitch PCI device unit number.
 *      ipmc_id  - The MC index number.
 *      port     - port from which to remove VLANs.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_repl_delete_all(int unit, int ipmc_id, bcm_port_t port)
{
    REPL_INIT(unit);
    REPL_GROUP_ID(unit, ipmc_id);
    REPL_PORT_CHECK(unit, port);

    if (!REPL_PORT_GROUP_INTF_COUNT(unit, port, ipmc_id)) {
        /* Nothing to do */
        return BCM_E_NONE;
    }

    return bcm_tr3_ipmc_egress_intf_set(unit, ipmc_id, port, 0, NULL,
                                        TRUE, FALSE);
}

/*
 * Function:
 *      bcm_tr3_ipmc_repl_set
 * Purpose:
 *      Assign set of VLANs provided to port's replication list for chosen
 *      IPMC group.
 * Parameters:
 *      unit     - StrataSwitch PCI device unit number.
 *      ipmc_id  - The index number.
 *      port     - port to list.
 *      vlan_vec - (IN) vector of replicated VLANs common to selected ports.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_repl_set(int unit, int ipmc_id, bcm_port_t port,
                     bcm_vlan_vector_t vlan_vec)
{
    int rv = BCM_E_NONE;
    bcm_if_t *if_array = NULL;
    bcm_l3_intf_t l3_intf;
    pbmp_t pbmp, ubmp;
    int  intf_num, intf_max, alloc_size, vid;

    REPL_INIT(unit);
    REPL_GROUP_ID(unit, ipmc_id);
    REPL_PORT_CHECK(unit, port);

    intf_max = BCM_VLAN_MAX - BCM_VLAN_MIN + 1;
    alloc_size = intf_max * sizeof(bcm_if_t);
    if_array = sal_alloc(alloc_size, "IPMC repl interface array");
    if (if_array == NULL) {
        return BCM_E_MEMORY;
    }

    sal_memset(if_array, 0, alloc_size);
    intf_num = 0;
    for (vid = BCM_VLAN_MIN; vid < BCM_VLAN_MAX; vid++) {
        if (BCM_VLAN_VEC_GET(vlan_vec, vid)) {
            rv = bcm_esw_vlan_port_get(unit, vid, &pbmp, &ubmp);
            if (BCM_FAILURE(rv)) {
                sal_free(if_array);
                return rv;
            }
            if (!BCM_PBMP_MEMBER(pbmp, port)) {
                sal_free(if_array);
                return BCM_E_PARAM;
            }
            bcm_l3_intf_t_init(&l3_intf);
            l3_intf.l3a_vid = vid;
            if ((rv = bcm_esw_l3_intf_find_vlan(unit, &l3_intf)) < 0) {
                sal_free(if_array);
                return rv;
            }
            if_array[intf_num++] = l3_intf.l3a_intf_id;
        }
    }

    rv = bcm_tr3_ipmc_egress_intf_set(unit, ipmc_id, port,
                                     intf_num, if_array, TRUE, FALSE);

    sal_free(if_array);
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_ipmc_egress_intf_add
 * Purpose:
 *      Add encap ID to selected ports' replication list for chosen
 *      IPMC group.
 * Parameters:
 *      unit     - StrataSwitch PCI device unit number.
 *      ipmc_id  - IPMC group number.
 *      port     - port to add.
 *      encap_id - Encap ID.
 *      is_l3    - Indicates if multicast group type is IPMC.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr3_ipmc_egress_intf_add(int unit, int ipmc_id, bcm_port_t port,
                            int encap_id, int is_l3)
{
    bcm_if_t *if_array = NULL;
    int  intf_num, intf_max, alloc_size, rv = BCM_E_NONE;
    bcm_l3_intf_t l3_intf;
    _tr3_if_updated_t if_updated;
    bcm_if_t intf_add;

    REPL_INIT(unit);
    REPL_GROUP_ID(unit, ipmc_id);

    /* Replication is allowed on CPU ports, only on TD2 */
    if(IS_CPU_PORT(unit, port)) {
        if(!SOC_IS_TRIDENT2(unit)) {
            return BCM_E_PARAM;
        }
    }
    else
    {   
        REPL_PORT_CHECK(unit, port);
    }

    intf_max = REPL_INTF_TOTAL(unit);
    alloc_size = intf_max * sizeof(bcm_if_t);
    if_array = sal_alloc(alloc_size, "IPMC repl interface array");
    if (if_array == NULL) {
        return BCM_E_MEMORY;
    }

    REPL_LOCK(unit);
    rv = bcm_tr3_ipmc_egress_intf_get(unit, ipmc_id, port, intf_max,
                                     if_array, &intf_num);
    if (BCM_SUCCESS(rv)) {
        if (intf_num < intf_max) {
            if_array[intf_num++] = encap_id;

            /* For IPMC and Trill, check port is a member of the L3 interface's 
             * VLAN. Performing this check here is more efficient than doing 
             * it in bcm_tr3_ipmc_egress_intf_set.
             */ 
            if (!BCM_XGS3_DVP_EGRESS_IDX_VALID(unit, encap_id)) {
                /* L3 interface is used */

                if (encap_id > soc_mem_index_max(unit, EGR_L3_INTFm)) {
                    rv = BCM_E_PARAM;
                    goto intf_add_done;
                }

                bcm_l3_intf_t_init(&l3_intf);
                l3_intf.l3a_intf_id = encap_id;
                rv = bcm_esw_l3_intf_get(unit, &l3_intf);
                if (BCM_FAILURE(rv)) {
                    goto intf_add_done;
                }
            }
            
            sal_memset(&if_updated, 0, sizeof(if_updated));
            intf_add = encap_id;
            if_updated.if_array_new = &intf_add;
            if_updated.if_count_new = 1;
            rv = _bcm_tr3_ipmc_egress_intf_set(unit, ipmc_id,
                                               port, intf_num, if_array,
                                               &if_updated, is_l3, FALSE);
        } else {
            rv = BCM_E_EXISTS;
        }
    }

intf_add_done:

    REPL_UNLOCK(unit);
    sal_free(if_array);
    return rv;
}

/*
 * Function:
 *      _bcm_tr3_ipmc_egress_intf_delete
 * Purpose:
 *      Remove encap ID from selected ports' replication list for chosen
 *      IPMC group.
 * Parameters:
 *      unit     - StrataSwitch PCI device unit number.
 *      ipmc_id  - IPMC group number.
 *      port     - port to remove.
 *      if_max   - Maximal interface.
 *      encap_id - Encap ID to delete from replication.
 *      is_l3    - Indicates if multicast group type is IPMC.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr3_ipmc_egress_intf_delete(int unit, int ipmc_id, bcm_port_t port,
                               int if_max, int encap_id, int is_l3)
{
    bcm_if_t *if_array = NULL;
    int alloc_size, if_count, if_cur, match, rv = BCM_E_NONE;
    _tr3_if_updated_t if_updated;
    bcm_if_t intf_del;
    
    REPL_INIT(unit);
    REPL_GROUP_ID(unit, ipmc_id);

    /* Replication is allowed on CPU ports, only on TD2 */
    if(IS_CPU_PORT(unit, port)) {
        if(!SOC_IS_TRIDENT2(unit)) {
            return BCM_E_PARAM;
        }
    }
    else
    {   
        REPL_PORT_CHECK(unit, port);
    }

    if (!REPL_PORT_GROUP_INTF_COUNT(unit, port, ipmc_id)) {
        return BCM_E_NOT_FOUND;
    }

    if ((if_max <= 0) || ((uint32)if_max > REPL_INTF_TOTAL(unit))) {
        return BCM_E_PARAM;
    }

    alloc_size = if_max * sizeof(bcm_if_t);
    if_array = sal_alloc(alloc_size, "IPMC repl interface array");
    if (if_array == NULL) {
        return BCM_E_MEMORY;
    }

    REPL_LOCK(unit);
    rv = bcm_tr3_ipmc_egress_intf_get(unit, ipmc_id, port, if_max,
                                     if_array, &if_count);
    if (BCM_SUCCESS(rv)) {
        match = FALSE;
        for (if_cur = 0; if_cur < if_count; if_cur++) {
            if (match) {
                if_array[if_cur - 1] = if_array[if_cur];
            } else {
                if (if_array[if_cur] == encap_id) {
                    match = TRUE;
                }
            }
        }
        if (match) {
            if_count--;
            sal_memset(&if_updated, 0, sizeof(if_updated));
            intf_del = encap_id;
            if_updated.if_count_del = 1;
            if_updated.if_array_del = &intf_del;
            rv = _bcm_tr3_ipmc_egress_intf_set(unit, ipmc_id, port, 
                                               if_count, if_array,
                                               &if_updated, is_l3, FALSE);
        } else {
            rv = BCM_E_NOT_FOUND;
        }
    }

    REPL_UNLOCK(unit);
    sal_free(if_array);
    return rv;
}

/*
 * Function:
 *      bcm_tr3_ipmc_trill_mac_update
 * Purpose:
 *      Update the Trill MAC address in next hop entries that are
 *      in Trill multicast replication list. 
 * Parameters:
 *      unit      - (IN) SOC unit #
 *      mac_field - (IN) MAC address
 *      flag      - (IN) 0 = Set the lower 24 bits of MAC address,
 *                       1 = Set the higher 24 bits of MAC address.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_trill_mac_update(int unit, uint32 mac_field, uint8 flag)
{
    int intf, nh_index;
    egr_l3_next_hop_entry_t egr_nh;
    uint32 entry_type;
    bcm_mac_t mac;

    for (intf = 0; intf < soc_mem_index_count(unit, EGR_L3_INTFm); intf++) {
        nh_index = REPL_L3_INTF_NEXT_HOP_TRILL(unit, intf);
        if (nh_index >= 0) {
            SOC_IF_ERROR_RETURN(soc_mem_read(unit, EGR_L3_NEXT_HOPm,
                        MEM_BLOCK_ANY, nh_index, &egr_nh));
            entry_type = soc_mem_field32_get(unit, EGR_L3_NEXT_HOPm,
                    &egr_nh, ENTRY_TYPEf);
            if (entry_type == 0) { /* normal entry type */
                soc_mem_mac_addr_get(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                        L3__MAC_ADDRESSf, mac);
                if (!flag) {
                    /* Set the low-order bytes */
                    mac[3] = (uint8) (mac_field >> 16 & 0xff);
                    mac[4] = (uint8) (mac_field >> 8 & 0xff);
                    mac[5] = (uint8) (mac_field & 0xff);
                } else {
                    /* Set the High-order bytes */
                    mac[0] = (uint8) (mac_field >> 16 & 0xff);
                    mac[1] = (uint8) (mac_field >> 8 & 0xff);
                    mac[2] = (uint8) (mac_field & 0xff);
                }
                soc_mem_mac_addr_set(unit, EGR_L3_NEXT_HOPm, &egr_nh,
                        L3__MAC_ADDRESSf, mac);
            }
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_ipmc_l3_intf_next_hop_free
 * Purpose:
 *      Free the next hop index associated with the given L3 interface.
 * Parameters:
 *      unit - (IN) SOC unit #
 *      intf - (IN) L3 interface ID
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_l3_intf_next_hop_free(int unit, int intf)
{
    int nh_index;

    if (_tr3_repl_info[unit] == NULL) {
        /* IPMC replication is not initialized. There is no next hop
         * index to free.
         */
        return BCM_E_NONE;
    }

    nh_index = REPL_L3_INTF_NEXT_HOP_TRILL(unit, intf);
    if (nh_index >= 0) {
        SOC_IF_ERROR_RETURN
            (WRITE_EGR_L3_NEXT_HOPm(unit, MEM_BLOCK_ALL, nh_index,
                                    soc_mem_entry_null(unit, EGR_L3_NEXT_HOPm)));
        BCM_IF_ERROR_RETURN
            (bcm_xgs3_nh_del(unit, _BCM_L3_SHR_WRITE_DISABLE, nh_index));
        REPL_L3_INTF_NEXT_HOP_TRILL(unit, intf) = -1;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_ipmc_l3_intf_next_hop_get
 * Purpose:
 *      Get the next hop index that was silently allocated to the L3 interface.
 * Parameters:
 *      unit - (IN) SOC unit #
 *      intf - (IN) L3 interface ID
 *      nh_index - (OUT) Next hop index
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_l3_intf_next_hop_get(int unit, int intf, int *nh_index)
{
    *nh_index = REPL_L3_INTF_NEXT_HOP_IPMC(unit, intf);

    if (*nh_index == REPL_NH_INDEX_UNALLOCATED ||
        *nh_index == REPL_NH_INDEX_L3_EGRESS_ALLOCATED) {
        return BCM_E_NOT_FOUND;
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_ipmc_l3_intf_next_hop_l3_egress_set
 * Purpose:
 *      Inform the IPMC module that a next hop index has been allocated
 *      for the given L3 interface by the bcm_l3_egress_create API.
 * Parameters:
 *      unit - (IN) SOC unit #
 *      intf - (IN) L3 interface ID
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_l3_intf_next_hop_l3_egress_set(int unit, int intf)
{
    int nh_index;

    nh_index = REPL_L3_INTF_NEXT_HOP_IPMC(unit, intf);

    if (nh_index == REPL_NH_INDEX_UNALLOCATED) {
        REPL_L3_INTF_NEXT_HOP_IPMC(unit, intf) =
            REPL_NH_INDEX_L3_EGRESS_ALLOCATED;
    } else if (nh_index == REPL_NH_INDEX_L3_EGRESS_ALLOCATED) {
        /* Do nothing. Users are allowed to create multiple L3
         * egress objects based on the same L3 interface.
         */
    } else {
        /* IPMC module had already silently allocated a next hop index
         * for this L3 interface.
         */
        return BCM_E_CONFIG;
    }
        
    return BCM_E_NONE;
}
#ifdef BCM_WARM_BOOT_SUPPORT

/*
 * Function:
 *      bcm_tr3_ipmc_repl_l3_intf_scache_size_get
 * Purpose:
 *      Get the required scache size for storing a bitmap
 *      of L3 interfaces.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      size - (OUT) Number of bytes
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_tr3_ipmc_repl_l3_intf_scache_size_get(
    int unit, 
    uint32 *size)
{
    int num_l3_intf;

    *size = 0;
    num_l3_intf = soc_mem_index_count(unit, EGR_L3_INTFm);
    *size += SHR_BITALLOCSIZE(num_l3_intf);

    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_ipmc_repl_l3_intf_sync
 * Purpose:
 *      Record L3 interface bitmap into the scache.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      scache_ptr - (IN/OUT) Scache pointer
 * Returns:
 *      BCM_E_xxx
 */
int 
bcm_tr3_ipmc_repl_l3_intf_sync(
    int unit, 
    uint8 **scache_ptr)
{
    int num_l3_intf, i;
    SHR_BITDCL *l3_intf_bitmap = NULL;

    /* Create a bitmap of L3 interfaces */
    num_l3_intf = soc_mem_index_count(unit, EGR_L3_INTFm);
    l3_intf_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_l3_intf),
            "L3 interface bitmap");
    if (NULL == l3_intf_bitmap) {
        return BCM_E_MEMORY;
    }
    sal_memset(l3_intf_bitmap, 0, SHR_BITALLOCSIZE(num_l3_intf));

    /* For the L3 interfaces that were allocated next hop indices by
     * bcm_l3_egress_create API, set the bit in the bitmap.
     */
    for (i = 0; i < num_l3_intf; i++) {
        if (REPL_L3_INTF_NEXT_HOP_IPMC(unit, i) ==
                REPL_NH_INDEX_L3_EGRESS_ALLOCATED) {
            SHR_BITSET(l3_intf_bitmap, i);
        }
    }

    /* Store the bitmap into scache */
    sal_memcpy((*scache_ptr), l3_intf_bitmap, SHR_BITALLOCSIZE(num_l3_intf));
    (*scache_ptr) += SHR_BITALLOCSIZE(num_l3_intf);

    sal_free(l3_intf_bitmap);
    return BCM_E_NONE;
}

/*
 * Function:
 *      bcm_tr3_ipmc_repl_l3_intf_scache_recover
 * Purpose:
 *      Recover L3 interface bitmap from scache.
 * Parameters:
 *      unit - (IN) SOC unit number.
 *      scache_ptr - (IN/OUT) Scache pointer
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_tr3_ipmc_repl_l3_intf_scache_recover(int unit, uint8 **scache_ptr) 
{
    int num_l3_intf, i;
    SHR_BITDCL *l3_intf_bitmap = NULL;

    /* Recover L3 interface bitmap */
    num_l3_intf = soc_mem_index_count(unit, EGR_L3_INTFm);
    l3_intf_bitmap = sal_alloc(SHR_BITALLOCSIZE(num_l3_intf),
            "L3 interface bitmap");
    if (NULL == l3_intf_bitmap) {
        return BCM_E_MEMORY;
    }
    sal_memcpy(l3_intf_bitmap, (*scache_ptr), SHR_BITALLOCSIZE(num_l3_intf));
    (*scache_ptr) += SHR_BITALLOCSIZE(num_l3_intf);

    for (i = 0; i < num_l3_intf; i++) {
        if (SHR_BITGET(l3_intf_bitmap, i)) {
            REPL_L3_INTF_NEXT_HOP_IPMC(unit, i) =
                REPL_NH_INDEX_L3_EGRESS_ALLOCATED;
        }
    }

    sal_free(l3_intf_bitmap);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_tr3_ipmc_repl_reload
 * Purpose:
 *      Re-Initialize replication software to state consistent with
 *      hardware.
 * Parameters:
 *      unit - SOC unit #
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_tr3_ipmc_repl_reload(int unit)
{
    uint8 *repl_group_buf = NULL;
    uint8 *repl_group_buf2 = NULL;
    int index_min, index_max;
    int rv = BCM_E_NONE;
    int intf_vec_alloc_size;
    SHR_BITDCL *intf_vec = NULL;
    int i, j;
    uint32 *repl_group_entry;
    int head_index;
    uint32 fldbuf[SOC_PBMP_WORD_MAX];
    int member_count;
    soc_info_t *si;
    int member_id, member;
    bcm_port_t mmu_port, phy_port, port;
    mmu_repl_head_tbl_entry_t repl_head_entry;
    int start_ptr;
    _bcm_repl_list_info_t *rli_current;
    int prev_repl_entry_ptr, repl_entry_ptr;
    mmu_repl_list_tbl_entry_t repl_list_entry;
    int msb;
    uint32 ls_bits[2];
    uint8 *egr_nh_buf = NULL;
    egr_l3_next_hop_entry_t *egr_nh_entry;
    int entry_type;
    int l3_intf;
    uint8 flags;
    soc_pbmp_t member_bitmap;
    soc_mem_t repl_group_table;
    soc_field_t base_idx_f;
    soc_field_t member_bitmap_f;
#ifdef BCM_TRIDENT2_SUPPORT
    soc_mem_t repl_group_table2 = MMU_REPL_GROUP_INFO1m;
    int member_bitmap2_offset = 0;
#endif

    /* Initialize internal data structures */
    BCM_IF_ERROR_RETURN(bcm_tr3_ipmc_repl_init(unit));

    /* Recover internal state by traversing replication lists */

#ifdef BCM_TRIDENT2_SUPPORT
    if (soc_feature(unit, soc_feature_split_repl_group_table)) {
        repl_group_table = MMU_REPL_GROUP_INFO0m;
        repl_group_table2 = MMU_REPL_GROUP_INFO1m;
        base_idx_f = PIPE_BASE_PTRf;
        member_bitmap_f = PIPE_MEMBER_BMPf;
        member_bitmap2_offset = soc_mem_field_length(unit,
                repl_group_table, member_bitmap_f);
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        repl_group_table = MMU_REPL_GROUPm;
        base_idx_f = HEAD_INDEXf;
        member_bitmap_f = MEMBER_BITMAPf;
    }

    /* Read replication group table */
    repl_group_buf = soc_cm_salloc(unit,
            SOC_MEM_TABLE_BYTES(unit, repl_group_table), "repl group buf");
    if (NULL == repl_group_buf) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    index_min = soc_mem_index_min(unit, repl_group_table);
    index_max = soc_mem_index_max(unit, repl_group_table);
    rv = soc_mem_read_range(unit, repl_group_table, MEM_BLOCK_ANY,
            index_min, index_max, repl_group_buf);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }


#ifdef BCM_TRIDENT2_SUPPORT
    /* Read replication group table of Y-pipe */
    else if (soc_feature(unit, soc_feature_split_repl_group_table)) {
        repl_group_buf2 = soc_cm_salloc(unit,
                SOC_MEM_TABLE_BYTES(unit, repl_group_table2), "repl group buf2");
        if (NULL == repl_group_buf2) {
            rv = BCM_E_MEMORY;
            goto cleanup;
        }
        index_min = soc_mem_index_min(unit, repl_group_table2);
        index_max = soc_mem_index_max(unit, repl_group_table2);
        rv = soc_mem_read_range(unit, repl_group_table2, MEM_BLOCK_ANY,
                index_min, index_max, repl_group_buf2);
        if (SOC_FAILURE(rv)) {
            goto cleanup;
        }
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    intf_vec_alloc_size = SHR_BITALLOCSIZE(REPL_INTF_TOTAL(unit));
    intf_vec = sal_alloc(intf_vec_alloc_size, "Repl interface vector");
    if (intf_vec == NULL) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(intf_vec, 0, intf_vec_alloc_size);

    for (i = index_min; i <= index_max; i++) {
        repl_group_entry = soc_mem_table_idx_to_pointer(unit,
                repl_group_table, uint32 *, repl_group_buf, i);

        /* Get the head index */
        head_index = soc_mem_field32_get(unit, repl_group_table,
                    repl_group_entry, base_idx_f);
        if (0 == head_index &&
                !soc_feature(unit, soc_feature_static_repl_head_alloc)) {
            continue; /* with next replication group */
        }

        /* Get the member bitmap */
        sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
        soc_mem_field_get(unit, repl_group_table, repl_group_entry,
                    member_bitmap_f, fldbuf);
        for (j = 0; j < SOC_PBMP_WORD_MAX; j++) {
            SOC_PBMP_WORD_SET(member_bitmap, j, fldbuf[j]);
        }
#ifdef BCM_TRIDENT2_SUPPORT
        /* Get the member bitmap of Y-pipe */
        if (soc_feature(unit, soc_feature_split_repl_group_table)) {
            uint32 *repl_group_entry2;
            soc_pbmp_t member_bitmap2;

            if (NULL != repl_group_buf2) {
                repl_group_entry2 = soc_mem_table_idx_to_pointer(unit,
                        repl_group_table2, uint32 *, repl_group_buf2, i);
                soc_mem_field_get(unit, repl_group_table2, repl_group_entry2,
                        member_bitmap_f, fldbuf);
                for (j = 0; j < SOC_PBMP_WORD_MAX; j++) {
                    SOC_PBMP_WORD_SET(member_bitmap2, j, fldbuf[j]);
                }
                SOC_PBMP_ITER(member_bitmap2, member) {
                    SOC_PBMP_PORT_ADD(member_bitmap,
                            (member + member_bitmap2_offset));
                }
            }
        }
#endif /* BCM_TRIDENT2_SUPPORT */
        SOC_PBMP_COUNT(member_bitmap, member_count);
        if (0 == member_count) {
            continue; /* with next replication group */
        }

        member_id = 0;
        SOC_PBMP_ITER(member_bitmap, member) {
            /* Convert member to mmu port, then to physical port, and
             * finally to logical port */
#ifdef BCM_TRIDENT2_SUPPORT
            if (soc_feature(unit, soc_feature_split_repl_group_table)) {
                if (member_bitmap2_offset > 0 && member >= member_bitmap2_offset) {
                    mmu_port = member - member_bitmap2_offset + 64;
                } else {
                    mmu_port = member;
                }
            } else
#endif /* BCM_TRIDENT2_SUPPORT */
            {
                /* In Triumph3, mmu port 60 is mapped to member 59 */
                if (59 == member) {
                    mmu_port = 60;
                } else {
                    mmu_port = member;
                }
            }
            /* Add code to check MMU port is less than SOC_MAX_NUM_PORTS,
             * in order to prevent false Coverity warning that the
             * si->port_m2p_mapping array may be overrun.
             */
            if (mmu_port >= SOC_MAX_NUM_PORTS) {
                rv = BCM_E_INTERNAL;
                goto cleanup;
            }
            si = &SOC_INFO(unit);
            phy_port = si->port_m2p_mapping[mmu_port];
            port = si->port_p2l_mapping[phy_port];

            /* Get replication list start pointer */
            rv = READ_MMU_REPL_HEAD_TBLm(unit, MEM_BLOCK_ANY,
                    head_index + member_id, &repl_head_entry);
            if (SOC_FAILURE(rv)) {
                goto cleanup;
            }
            member_id++;
            start_ptr = soc_MMU_REPL_HEAD_TBLm_field32_get(unit,
                    &repl_head_entry, HEAD_PTRf);
            if (0 == start_ptr) {
                continue; /* with next member */
            }

            if (_bcm_tr3_repl_list_entry_used_get(unit, start_ptr)) {
                /* Already traversed this replication list */
                for (rli_current = REPL_LIST_INFO(unit); rli_current != NULL;
                        rli_current = rli_current->next) {
                    if (rli_current->index == start_ptr) {
                        (rli_current->refcount)++;
                        REPL_PORT_GROUP_INTF_COUNT(unit, port, i) =
                            rli_current->list_size;
                        break;
                    }
                }
                if (rli_current == NULL) {
                    /* Unexpected */
                    rv = BCM_E_INTERNAL;
                    goto cleanup;
                } else {
                    continue; /* with next member */
                }
            }

            /* Traverse the replication list table */
            sal_memset(intf_vec, 0, intf_vec_alloc_size);
            prev_repl_entry_ptr = -1;
            repl_entry_ptr = start_ptr;
            while (repl_entry_ptr != prev_repl_entry_ptr) {
                rv = READ_MMU_REPL_LIST_TBLm(unit, MEM_BLOCK_ANY,
                        repl_entry_ptr, &repl_list_entry);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
                msb = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                        &repl_list_entry, MSB_VLANf);
                soc_MMU_REPL_LIST_TBLm_field_get(unit, &repl_list_entry,
                        LSB_VLAN_BMf, ls_bits);
                intf_vec[2 * msb + 0] = ls_bits[0];
                intf_vec[2 * msb + 1] = ls_bits[1];
                REPL_PORT_GROUP_INTF_COUNT(unit, port, i) +=
                    _shr_popcount(ls_bits[0]) + _shr_popcount(ls_bits[1]);
                rv = _bcm_tr3_repl_list_entry_used_set(unit, repl_entry_ptr);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
                prev_repl_entry_ptr = repl_entry_ptr;
                repl_entry_ptr = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                        &repl_list_entry, NEXTPTRf);
            }

            /* Insert a new replication list into linked list */
            rli_current = sal_alloc(sizeof(_bcm_repl_list_info_t),
                    "repl list info");
            if (rli_current == NULL) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }
            sal_memset(rli_current, 0, sizeof(_bcm_repl_list_info_t));
            rli_current->index = start_ptr;
            rli_current->hash = _shr_crc32b(0, (uint8 *)intf_vec,
                    REPL_INTF_TOTAL(unit));
            rli_current->list_size = REPL_PORT_GROUP_INTF_COUNT(unit, port, i);
            (rli_current->refcount)++;
            rli_current->next = REPL_LIST_INFO(unit);
            REPL_LIST_INFO(unit) = rli_current;
        }

        /* Update REPL_HEAD table usage */
        if (!soc_feature(unit, soc_feature_static_repl_head_alloc)) {
            rv = _bcm_tr3_repl_head_block_used_set(unit, head_index, member_count);
            if (BCM_FAILURE(rv)) {
                goto cleanup;
            }
        }
    }
    soc_cm_sfree(unit, repl_group_buf);
    repl_group_buf = NULL;
    if (repl_group_buf2) {
        soc_cm_sfree(unit, repl_group_buf2);
        repl_group_buf2 = NULL;
    }
    sal_free(intf_vec);
    intf_vec = NULL;

    /* Recover array of next hop indices for L3 interfaces */
    egr_nh_buf = soc_cm_salloc(unit,
            SOC_MEM_TABLE_BYTES(unit, EGR_L3_NEXT_HOPm), "egr nh buf");
    if (NULL == egr_nh_buf) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    index_min = soc_mem_index_min(unit, EGR_L3_NEXT_HOPm);
    index_max = soc_mem_index_max(unit, EGR_L3_NEXT_HOPm);
    rv = soc_mem_read_range(unit, EGR_L3_NEXT_HOPm, MEM_BLOCK_ANY,
            index_min, index_max, egr_nh_buf);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

    for (i = index_min; i <= index_max; i++) {
        egr_nh_entry = soc_mem_table_idx_to_pointer
            (unit, EGR_L3_NEXT_HOPm, egr_l3_next_hop_entry_t *,
             egr_nh_buf, i);

        entry_type = soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh_entry,
               ENTRY_TYPEf);
        if (entry_type == 0) {
            l3_intf = soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh_entry,
                L3__INTF_NUMf);
            REPL_L3_INTF_NEXT_HOP_TRILL(unit, l3_intf) = i;
        } else if (entry_type == 7) {
            l3_intf = soc_EGR_L3_NEXT_HOPm_field32_get(unit, egr_nh_entry,
                L3MC__INTF_NUMf);
            REPL_L3_INTF_NEXT_HOP_IPMC(unit, l3_intf) = i;
        } else {
            continue;
        }
    }
    soc_cm_sfree(unit, egr_nh_buf);
    egr_nh_buf = NULL;

    /* Recover REPL list mode from HW cache */
    rv = _bcm_esw_ipmc_repl_wb_flags_get(unit,
                                         _BCM_IPMC_WB_REPL_LIST, &flags);
    if (flags) {
        SOC_IPMCREPLSHR_SET(unit, 1);
    }

cleanup:
    if (repl_group_buf) {
        soc_cm_sfree(unit, repl_group_buf);
    }
    if (repl_group_buf2) {
        soc_cm_sfree(unit, repl_group_buf2);
    }
    if (intf_vec) {
        sal_free(intf_vec);
    }
    if (egr_nh_buf) {
        soc_cm_sfree(unit, egr_nh_buf);
    }

    if (BCM_FAILURE(rv)) {
        bcm_tr3_ipmc_repl_detach(unit);
    }

    return rv;
}


#ifdef BCM_KATANA2_SUPPORT
/*
 * Function:
 *      _bcm_kt2_ipmc_repl_reload
 * Purpose:
 *      Re-Initialize replication software to state consistent with
 *      hardware.
 * Parameters:
 *      unit - SOC unit #
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_kt2_ipmc_repl_reload(int unit)
{
    uint8 *repl_group_buf0 = NULL;
    uint8 *repl_group_buf1 = NULL;
    uint8 *repl_group_base_ptr_buf = NULL;
    int index_min, index_max;
    int rv = BCM_E_NONE;
    int intf_vec_alloc_size;
    SHR_BITDCL *intf_vec = NULL;
    int i, j;
    uint32 *repl_group_entry0;
    uint32 *repl_group_entry1;
    uint32 *repl_group_base_ptr_entry;
    int head_index;
    uint32 fldbuf[SOC_PBMP_WORD_MAX];
    int member_count;
    int member_id, member;
    bcm_port_t mmu_port, phy_port, port;
    mmu_repl_head_tbl_entry_t repl_head_entry;
    int start_ptr;
    _bcm_repl_list_info_t *rli_current;
    int prev_repl_entry_ptr, repl_entry_ptr;
    mmu_repl_list_tbl_entry_t repl_list_entry;
    int msb;
    uint32 ls_bits[2];
    uint8 flags;
    soc_pbmp_t member_bitmap;
    soc_pbmp_t member_bitmap0;
    soc_pbmp_t member_bitmap1;
    soc_mem_t repl_group_table0;
    soc_mem_t repl_group_table1;
    soc_mem_t repl_group_base_ptr_table = INVALIDm;
    soc_field_t base_idx_f;
    uint32 nh_offset , is_len, offset;
    soc_field_t mbmp0, mbmp1;
    int intf_base;
    int member_bitmap_width0;

    /* Initialize internal data structures */
    BCM_IF_ERROR_RETURN(bcm_tr3_ipmc_repl_init(unit));
 
    is_len = soc_mem_field_length(unit, MMU_REPL_LIST_TBLm, LSB_VLAN_BMf);
    nh_offset = (1 << (soc_mem_field_length(unit, MMU_REPL_LIST_TBLm,
                                                MSB_VLANf) - 1)) * is_len;
    member_bitmap_width0 =  soc_mem_field_length(unit,
                MMU_REPL_GRP_TBL0m, MEMBER_BMP_PORTS_119_0f);
    /* Recover internal state by traversing replication lists */
    repl_group_table0 = MMU_REPL_GRP_TBL0m;
    repl_group_table1 = MMU_REPL_GRP_TBL1m;
    repl_group_base_ptr_table = MMU_REPL_GRP_TBL2m;
    base_idx_f = BASE_PTRf;
    mbmp0 = MEMBER_BMP_PORTS_119_0f;
    mbmp1 = MEMBER_BMP_PORTS_169_120f;

    /* Read replication group table MMU_REPL_GRP_TBL0 */
    repl_group_buf0 = soc_cm_salloc(unit,
            SOC_MEM_TABLE_BYTES(unit, repl_group_table0), "repl group buf");

    if (NULL == repl_group_buf0) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }

    /* Read replication group table MMU_REPL_GRP_TBL1 */
    repl_group_buf1 = soc_cm_salloc(unit,
            SOC_MEM_TABLE_BYTES(unit, repl_group_table1), "repl group buf");

    if (NULL == repl_group_buf1) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }

    repl_group_base_ptr_buf = soc_cm_salloc(unit,
          SOC_MEM_TABLE_BYTES(unit, repl_group_base_ptr_table), 
                              "repl group base ptr buf");

    if (NULL == repl_group_base_ptr_buf) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }

    index_min = soc_mem_index_min(unit, repl_group_table0);
    index_max = soc_mem_index_max(unit, repl_group_table0);
    rv = soc_mem_read_range(unit, repl_group_table0, MEM_BLOCK_ANY,
            index_min, index_max, repl_group_buf0);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

    index_min = soc_mem_index_min(unit, repl_group_table1);
    index_max = soc_mem_index_max(unit, repl_group_table1);
    rv = soc_mem_read_range(unit, repl_group_table1, MEM_BLOCK_ANY,
            index_min, index_max, repl_group_buf1);
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }


    index_min = soc_mem_index_min(unit, repl_group_base_ptr_table);
    index_max = soc_mem_index_max(unit, repl_group_base_ptr_table);
    rv = soc_mem_read_range(unit, repl_group_base_ptr_table, MEM_BLOCK_ANY,
                index_min, index_max, repl_group_base_ptr_buf); 
    if (SOC_FAILURE(rv)) {
        goto cleanup;
    }

    intf_vec_alloc_size = SHR_BITALLOCSIZE(REPL_INTF_TOTAL(unit));
    intf_vec = sal_alloc(intf_vec_alloc_size * sizeof(SHR_BITDCL),
                         "Repl interface vector");
    if (intf_vec == NULL) {
        rv = BCM_E_MEMORY;
        goto cleanup;
    }
    sal_memset(intf_vec, 0, intf_vec_alloc_size);

    for (i = index_min; i <= index_max; i++) {
        repl_group_entry0 = soc_mem_table_idx_to_pointer(unit,
                repl_group_table0, uint32 *, repl_group_buf0, i);

        repl_group_entry1 = soc_mem_table_idx_to_pointer(unit,
                repl_group_table1, uint32 *, repl_group_buf1, i);
 
        /* Get the head index */
        repl_group_base_ptr_entry = soc_mem_table_idx_to_pointer(unit,
                    repl_group_base_ptr_table, uint32 *,
                    repl_group_base_ptr_buf, i);

        head_index = soc_mem_field32_get(unit, repl_group_base_ptr_table,
                    repl_group_base_ptr_entry, base_idx_f);

        /* Get the member bitmap */
        sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
        soc_mem_field_get(unit, repl_group_table0, repl_group_entry0,
                    mbmp0, fldbuf);
        for (j = 0; j < SOC_PBMP_WORD_MAX; j++) {
            SOC_PBMP_WORD_SET(member_bitmap0, j, fldbuf[j]);
        }
         
        sal_memset(fldbuf, 0, SOC_PBMP_WORD_MAX * sizeof(uint32));
        soc_mem_field_get(unit, repl_group_table1, repl_group_entry1,
                          mbmp1, fldbuf);
        for (j = 0; j < SOC_PBMP_WORD_MAX; j++) {
            SOC_PBMP_WORD_SET(member_bitmap1, j, fldbuf[j]);
        }

        SOC_PBMP_CLEAR(member_bitmap);
        SOC_PBMP_OR(member_bitmap, member_bitmap0);

        SOC_PBMP_ITER(member_bitmap1, member) {
           SOC_PBMP_PORT_ADD(member_bitmap,
                    (member + member_bitmap_width0));
        }

        SOC_PBMP_COUNT(member_bitmap, member_count);

        if (0 == member_count) {
            continue; /* with next replication group */
        }

        member_id = 0;
        SOC_PBMP_ITER(member_bitmap, member) {
            /* Convert member to mmu port, then to physical port, and
             * finally to logical port */

            mmu_port = member;
            phy_port = mmu_port;
            port = phy_port;

            /* Get replication list start pointer */
            rv = READ_MMU_REPL_HEAD_TBLm(unit, MEM_BLOCK_ANY,
                    head_index + member_id, &repl_head_entry);
            if (SOC_FAILURE(rv)) {
                goto cleanup;
            }
            member_id++;

            start_ptr = soc_MMU_REPL_HEAD_TBLm_field32_get(unit,
                    &repl_head_entry, HEAD_PTRf);
            if (0 == start_ptr) {
                continue; /* with next member */
            }

            if (_bcm_tr3_repl_list_entry_used_get(unit, start_ptr)) {
                /* Already traversed this replication list */
                for (rli_current = REPL_LIST_INFO(unit); rli_current != NULL;
                        rli_current = rli_current->next) {
                    if (rli_current->index == start_ptr) {
                        (rli_current->refcount)++;
                        REPL_PORT_GROUP_INTF_COUNT(unit, port, i) =
                            rli_current->list_size;
                        break;
                    }
                }
                if (rli_current == NULL) {
                    /* Unexpected */
                    rv = BCM_E_INTERNAL;
                    goto cleanup;
                } else {
                    continue; /* with next member */
                }
            }

            /* Traverse the replication list table */
            sal_memset(intf_vec, 0, intf_vec_alloc_size);
            prev_repl_entry_ptr = -1;
            repl_entry_ptr = start_ptr;
            while (repl_entry_ptr != prev_repl_entry_ptr) {
                rv = READ_MMU_REPL_LIST_TBLm(unit, MEM_BLOCK_ANY,
                        repl_entry_ptr, &repl_list_entry);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
       
                msb = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                        &repl_list_entry, MSB_VLANf);
                soc_MMU_REPL_LIST_TBLm_field_get(unit, &repl_list_entry,
                        LSB_VLAN_BMf, ls_bits);

                intf_base = msb *64;

                if (intf_base >= nh_offset)
                {
                    /* replication on next hops  */
                    offset = _SHR_BITDCLSIZE(soc_mem_index_count(unit, 
                                             EGR_L3_INTFm)); 
                } else {
                    /* replication on L3 interface */
                    offset = 0;
                }
                intf_vec[offset + 2 * msb + 0] = ls_bits[0];
                intf_vec[offset + 2 * msb + 1] = ls_bits[1];

                REPL_PORT_GROUP_INTF_COUNT(unit, port, i) +=
                    _shr_popcount(ls_bits[0]) + _shr_popcount(ls_bits[1]);
                rv = _bcm_tr3_repl_list_entry_used_set(unit, repl_entry_ptr);
                if (BCM_FAILURE(rv)) {
                    goto cleanup;
                }
                prev_repl_entry_ptr = repl_entry_ptr;
                repl_entry_ptr = soc_MMU_REPL_LIST_TBLm_field32_get(unit,
                        &repl_list_entry, NEXTPTRf);
            }

            /* Insert a new replication list into linked list */
            rli_current = sal_alloc(sizeof(_bcm_repl_list_info_t),
                    "repl list info");
            if (rli_current == NULL) {
                rv = BCM_E_MEMORY;
                goto cleanup;
            }
            sal_memset(rli_current, 0, sizeof(_bcm_repl_list_info_t));
            rli_current->index = start_ptr;
            rli_current->hash = _shr_crc32b(0, (uint8 *)intf_vec,
                    REPL_INTF_TOTAL(unit));
            rli_current->list_size = REPL_PORT_GROUP_INTF_COUNT(unit, port, i);
            (rli_current->refcount)++;
            rli_current->next = REPL_LIST_INFO(unit);
            REPL_LIST_INFO(unit) = rli_current;
        }

    }
    soc_cm_sfree(unit, repl_group_buf0);
    repl_group_buf0 = NULL;
    
    soc_cm_sfree(unit, repl_group_buf1);
    repl_group_buf1 = NULL;
   
   
    soc_cm_sfree(unit, repl_group_base_ptr_buf);
    repl_group_base_ptr_buf = NULL;
    
    sal_free(intf_vec);
    intf_vec = NULL;

    /* Recover REPL list mode from HW cache */
    rv = _bcm_esw_ipmc_repl_wb_flags_get(unit,
                                         _BCM_IPMC_WB_REPL_LIST, &flags);
    if (flags) {
        SOC_IPMCREPLSHR_SET(unit, 1);
    }

cleanup:
    if (repl_group_buf0) {
        soc_cm_sfree(unit, repl_group_buf0);
    }
    if (repl_group_buf1) {
        soc_cm_sfree(unit, repl_group_buf1);
    }
    if (repl_group_base_ptr_buf) {
        soc_cm_sfree(unit, repl_group_base_ptr_buf);
    }
    if (intf_vec) {
        sal_free(intf_vec);
    }
    if (BCM_FAILURE(rv)) {
        bcm_tr3_ipmc_repl_detach(unit);
    }

    return rv;
}
#endif /* BCM_KATANA2_SUPPORT*/

#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_tr3_ipmc_repl_sw_dump
 * Purpose:
 *     Displays IPMC replication information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_tr3_ipmc_repl_sw_dump(int unit)
{
    _tr3_repl_info_t *repl_info;
    bcm_port_t port;
    _tr3_repl_port_info_t *port_info;
    int i, j;
    _bcm_repl_list_info_t *rli_start, *rli_current;
    SHR_BITDCL *bitmap;
    _tr3_repl_head_free_block_t *free_block;

    LOG_CLI((BSL_META_U(unit,
                        "  IPMC REPL Info -\n")));
    repl_info = _tr3_repl_info[unit];
    if (repl_info != NULL) {
        LOG_CLI((BSL_META_U(unit,
                            "    Replication Group Size : %d\n"),
                 repl_info->num_repl_groups));
        LOG_CLI((BSL_META_U(unit,
                            "    Replication Intf Size  : %d\n"),
                 repl_info->num_intf));

        LOG_CLI((BSL_META_U(unit,
                            "    Port Info    -\n")));
        LOG_CLI((BSL_META_U(unit,
                            "    port (index:intf-count) :\n")));
        for (port = 0; port < ((SOC_MAX_NUM_PP_PORTS > SOC_MAX_NUM_PORTS) ? 
            SOC_MAX_NUM_PP_PORTS : SOC_MAX_NUM_PORTS); port++) {    
            port_info = repl_info->port_info[port];
            LOG_CLI((BSL_META_U(unit,
                                "    %3d -"), port));
            if (port_info == NULL) {
                LOG_CLI((BSL_META_U(unit,
                                    " null\n")));
                continue;
            }
            for (i = 0, j = 0; i < repl_info->num_repl_groups; i++) {
                /* If zero, skip print */
                if (port_info->intf_count[i] == 0) {
                    continue;
                }
                if ((j > 0) && !(j % 4)) {
                    LOG_CLI((BSL_META_U(unit,
                                        "\n         ")));
                }
                LOG_CLI((BSL_META_U(unit,
                                    " %5d:%-5d"), i, port_info->intf_count[i]));
                j++;
            }
            LOG_CLI((BSL_META_U(unit,
                                "\n")));
        }

        rli_start = REPL_LIST_INFO(unit);
        LOG_CLI((BSL_META_U(unit,
                            "    List Info    -\n")));
        for (rli_current = rli_start; rli_current != NULL;
             rli_current = rli_current->next) {
            LOG_CLI((BSL_META_U(unit,
                                "    Hash:  0x%08x\n"), rli_current->hash));
            LOG_CLI((BSL_META_U(unit,
                                "    Index: %4d\n"), rli_current->index));
            LOG_CLI((BSL_META_U(unit,
                                "    Size:  %4d\n"), rli_current->list_size));
            LOG_CLI((BSL_META_U(unit,
                                "    Refs:  %4d\n"), rli_current->refcount));
        }

      if (repl_info->l3_intf_next_hop_ipmc != NULL) {
        LOG_CLI((BSL_META_U(unit,
                            "    L3 Interface Next Hop IPMC Info -\n")));
        for (i = 0; i < soc_mem_index_count(unit, EGR_L3_INTFm); i++) {
           if (repl_info->l3_intf_next_hop_ipmc[i] ==
                   REPL_NH_INDEX_UNALLOCATED) {
              continue;
           }
           if (repl_info->l3_intf_next_hop_ipmc[i] ==
                   REPL_NH_INDEX_L3_EGRESS_ALLOCATED) {
              continue;
           }
          LOG_CLI((BSL_META_U(unit,
                              "      L3 Interface %4d: Next Hop Index %5d\n"), i,
                   repl_info->l3_intf_next_hop_ipmc[i]));
        }
      }

     if (repl_info->l3_intf_next_hop_trill != NULL) {

        LOG_CLI((BSL_META_U(unit,
                            "    L3 Interface Next Hop Trill Info -\n")));
        for (i = 0; i < soc_mem_index_count(unit, EGR_L3_INTFm); i++) {
           if (repl_info->l3_intf_next_hop_trill[i] == -1) {
              continue;
           }
          LOG_CLI((BSL_META_U(unit,
                              "      L3 Interface %4d: Next Hop Index %5d\n"), i,
                   repl_info->l3_intf_next_hop_trill[i]));
        }
     }
    }

    if (_tr3_repl_list_entry_info[unit] != NULL) {
        LOG_CLI((BSL_META_U(unit,
                            "    Replication List Table Size : %d\n"),
                 _tr3_repl_list_entry_info[unit]->num_entries));
        LOG_CLI((BSL_META_U(unit,
                            "    Replication List Table Usage Bitmap (index:value-hex) :")));
        if (_tr3_repl_list_entry_info[unit]->bitmap_entries_used != NULL) {
            bitmap = _tr3_repl_list_entry_info[unit]->bitmap_entries_used;
            for (i = 0, j = 0;
                    i < _SHR_BITDCLSIZE(_tr3_repl_list_entry_info[unit]->num_entries);
                    i++) {
                /* If zero, skip print */
                if (bitmap[i] == 0) {
                    continue;
                }
                if (!(j % 4)) {
                    LOG_CLI((BSL_META_U(unit,
                                        "\n    ")));
                }
                LOG_CLI((BSL_META_U(unit,
                                    "  %5d:%8.8x"), i, bitmap[i]));
                j++;
            }
        }
        LOG_CLI((BSL_META_U(unit,
                            "\n")));
    }

    if (_tr3_repl_head_info[unit] != NULL) {
        LOG_CLI((BSL_META_U(unit,
                            "    Replication Head Table Size : %d\n"),
                 soc_mem_index_count(unit, MMU_REPL_HEAD_TBLm)));
        LOG_CLI((BSL_META_U(unit,
                            "    Replication Head Free List Array:\n")));
        if (_tr3_repl_head_info[unit]->free_list_array != NULL) {
            for (i = 0; i < _tr3_repl_head_info[unit]->array_size; i++) {
                LOG_CLI((BSL_META_U(unit,
                                    "      Free List %2d:"), i));
                free_block = REPL_HEAD_FREE_LIST(unit, i);
                j = 0;
                while (NULL != free_block) {
                    if (j > 0 && !(j % 4)) {
                        LOG_CLI((BSL_META_U(unit,
                                            "\n                   ")));
                    }
                    LOG_CLI((BSL_META_U(unit,
                                        "  %7d:%-7d"), free_block->index,
                             free_block->size));
                    free_block = free_block->next;
                    j++;
                }
                LOG_CLI((BSL_META_U(unit,
                                    "\n")));
            }
        }
    }

#ifdef BCM_KATANA2_SUPPORT
    /*
     * _kt_extq_repl_info
     */
    if (SOC_IS_KATANA2(unit) && (repl_info != NULL)) { 
        bcm_kt2_ipmc_subscriber_sw_dump(unit);
    }
#endif

    return;
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif /* BCM_TRIUMPH3_SUPPORT && INCLUDE_L3 */
#endif
