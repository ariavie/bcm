/*
 * $Id: hash_tbl.h,v 1.4 Broadcom SDK $
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
 * File: 	shr_hash.h
 * Purpose: 	Defines a generic hash table for key/value pairs.
 *
 * Overview:
 * Provides a generic hash table interface with configurable and default hash
 * and key compare functions.  The interface allows for complex key types, but
 * the caller must implement a _cast_ method to transform the complex type into
 * a string of bytes for the hash function.  Alternatively, the caller may
 * replace the hash function itself, in which case, the default _cast_ method
 * may be used to return the complex key and complex key size. 
 *
 * Memory is allocated on-demand in blocks for hash entries, and freed in 
 * blocks when the free pool becomes large.
 *
 * Collisions are handled simply by creating a linked list per hash index and
 * a linear search is performed within the list to find an entry.  The list is
 * not sorted.  (future upgrade?)
 *
 * The caller defined KEY is *copied* and stored in the variable sized hash 
 * table entry for comparison.  A *pointer* to the caller defined DATA is 
 * stored.  The caller is reponsible for managing the memory where DATA points
 * if any.  Callbacks are provided upon hash destruction to free any allocated
 * memory.  The hash table module itself does not explitly free any DATA
 * pointer at any time.
 */

#ifndef _HASH_TBL_H_
#define _HASH_TBL_H_

#include <sal/types.h>

typedef void*  shr_htb_key_t;
typedef void*  shr_htb_data_t;

typedef struct hash_table_s* shr_htb_hash_table_t;


typedef uint32 (*shr_htb_hash_f)(uint8* key_bytes, uint32 length);
typedef void (*shr_htb_cast_key_f)(shr_htb_key_t key,
                                   uint8  **key_bytes, 
                                   uint32  *key_size);
typedef int (*shr_htb_key_cmp_f)(shr_htb_key_t a,
                                 shr_htb_key_t b,
                                 uint32 size);
typedef void (*shr_htb_data_free_f)(shr_htb_data_t data);


int
shr_htb_create(shr_htb_hash_table_t *ht, int max_num_entries, int key_size,
               char* tbl_name);

int
shr_htb_destroy(shr_htb_hash_table_t *ht, shr_htb_data_free_f cb);

int
shr_htb_find(shr_htb_hash_table_t ht, shr_htb_key_t key, shr_htb_data_t *data,
             int remove);

int
shr_htb_insert(shr_htb_hash_table_t ht, shr_htb_key_t key, shr_htb_data_t data);


typedef int (*shr_htb_cb_t)(int unit, shr_htb_key_t key, shr_htb_data_t data);

int
shr_htb_iterate(int unit, shr_htb_hash_table_t ht, shr_htb_cb_t restore_cb);


/* Configuration routines */
void
shr_htb_cast_key_func_set(shr_htb_hash_table_t ht, 
                          shr_htb_cast_key_f func);
void 
shr_htb_hash_func_set(shr_htb_hash_table_t ht, 
                      shr_htb_hash_f func);

void 
shr_htb_key_cmp_func_set(shr_htb_hash_table_t ht,
                         shr_htb_key_cmp_f func);



#endif /* _HASH_TBL_H_ */

