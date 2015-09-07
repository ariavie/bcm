/*
 * $Id: mem_avl.h 1.6 Broadcom SDK $
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
 * File: 	mem_avl.h
 * Purpose: 	Defines a generic memory manager using AVL tree and DDL List.
 */

#ifndef _SHR_MEM_AVL_H
#define _SHR_MEM_AVL_H

#include <shared/avl.h>

typedef struct shr_mem_avl_entry_s {
    int size;
    unsigned int addr;
    int used;
    struct shr_mem_avl_entry_s *next;
    struct shr_mem_avl_entry_s *self; /* This entry is copied in avl entry datum. self points to the node in DLL */
    struct shr_mem_avl_entry_s *prev;
} shr_mem_avl_entry_t, *shr_mem_avl_entry_pt;

typedef struct shr_mem_avl_st{
    shr_avl_t *tree;                        /* AVL Tree of free node */
    shr_mem_avl_entry_t *mem_list;          /* DLL Element List (all nodes) */
}shr_mem_avl_t;

extern int shr_mem_avl_create(shr_mem_avl_t **mem_avl_ptr,
                              int mem_size,
                              int mem_base,
                              int max_blocks);

extern int shr_mem_avl_destroy(shr_mem_avl_t *mem_avl);

extern int shr_mem_avl_malloc(shr_mem_avl_t *mem_avl, int size, unsigned int *addr);
extern int shr_mem_avl_realloc(shr_mem_avl_t *mem_avl, int size, unsigned int addr);
extern int shr_mem_avl_free(shr_mem_avl_t *mem_avl, unsigned int addr);
extern int shr_mem_avl_free_tree_list(shr_mem_avl_t *mem_avl);
extern int shr_mem_avl_list_output(shr_mem_avl_t *mem_avl);

extern int shr_mem_avl_free_count(shr_mem_avl_t *mem_avl, int size, int *count);
#endif /* _SHR_MEM_AVL_H */
