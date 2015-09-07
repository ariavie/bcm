/*
 * $Id: avl.h 1.7 Broadcom SDK $
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
 * File: 	avl.h
 * Purpose: 	Defines a generic AVL tree data structure.
 */

#ifndef _SHR_AVL_H
#define _SHR_AVL_H

/*
 * NOTE:
 *
 *   sizeof (shr_avl_datum_t) is actually datum_bytes.
 *   sizeof (shr_avl_entry_t) is also correspondingly larger.
 */

typedef int shr_avl_datum_t;

typedef int (*shr_avl_compare_fn)(void *user_data,
				  shr_avl_datum_t *datum1,
				  shr_avl_datum_t *datum2);

typedef int (*shr_avl_compare_fn_lkupdata)(void *user_data,
				  shr_avl_datum_t *datum1,
				  shr_avl_datum_t *datum2,
                                  void *lkupdata);

typedef int (*shr_avl_traverse_fn)(void *user_data,
				   shr_avl_datum_t *datum,
				   void *trav_data);

typedef int (*shr_avl_datum_copy_fn)(void *user_data,
				  shr_avl_datum_t *datum1,
                                     shr_avl_datum_t *datum2);

typedef struct shr_avl_entry_s {
    struct shr_avl_entry_s	*left;
    struct shr_avl_entry_s	*right;
    int				balance;
    shr_avl_datum_t		datum;	    /* NOTE: variable size field */
} shr_avl_entry_t;

typedef struct shr_avl_s {
    /* Static data configured on tree creation */
    void			*user_data;
    int				datum_bytes;
    int				datum_max;
    int				entry_bytes;

    /* Dynamic data */
    char			*datum_base;
    shr_avl_entry_t		*root;
    shr_avl_entry_t		*free_list;
    int				count;		/* Number entries in tree */
    shr_avl_datum_copy_fn       datum_copy_fn;
} shr_avl_t;

extern int shr_avl_create(shr_avl_t **avl_ptr,
			  void *user_data,
			  int datum_bytes,
			  int datum_max);

extern int shr_avl_destroy(shr_avl_t *avl);

extern int shr_avl_insert(shr_avl_t *avl,
			  shr_avl_compare_fn cmp_fn,
			  shr_avl_datum_t *datum);

extern int shr_avl_delete(shr_avl_t *avl,
			  shr_avl_compare_fn key_cmp_fn,
			  shr_avl_datum_t *datum);

extern int shr_avl_delete_all(shr_avl_t *avl);

extern int shr_avl_count(shr_avl_t *avl);

extern int shr_avl_lookup(shr_avl_t *avl,
			  shr_avl_compare_fn key_cmp_fn,
			  shr_avl_datum_t *datum);

extern int shr_avl_lookup_lkupdata(shr_avl_t *avl,            
                                   shr_avl_compare_fn_lkupdata key_cmp_fn,
                                   shr_avl_datum_t *datum,
                                   void *userdata);    

extern int shr_avl_lookup_min(shr_avl_t *avl,
			      shr_avl_datum_t *datum);

extern int shr_avl_lookup_max(shr_avl_t *avl,
			      shr_avl_datum_t *datum);

extern int shr_avl_traverse(shr_avl_t *avl,
			    shr_avl_traverse_fn trav_fn,
			    void *trav_data);

#ifdef BROADCOM_DEBUG
extern void shr_avl_dump(shr_avl_t *avl);
#endif /* BROADCOM_DEBUG */

#endif	/* !_SHR_AVL_H */
