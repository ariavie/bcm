/*
 * $Id: profile_mem.h,v 1.20 Broadcom SDK $
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
 * Routines for managing HW profile tables.
 */

#ifndef _SOC_PROFILE_MEM_H
#define _SOC_PROFILE_MEM_H

#include <soc/defs.h>
#include <soc/mem.h>

#define SOC_PROFILE_MEM_F_NO_SHARE      0x1
#define SOC_PROFILE_MEM_DMA_THRESHHOLD  8

typedef struct soc_profile_mem_entry_s {
    uint32     ref_count;       /* Reference counters */
    int        entries_per_set; /* Number of entries per set */
} soc_profile_mem_entry_t;

typedef struct soc_profile_mem_table_s {
    soc_mem_t  mem;             /* Memory id */
    int        index_min;
    int        index_max;
    int        entry_words;     /* Entry word size */
    uint32     *data_mask;      /* optional entry comparison mask */
    soc_profile_mem_entry_t *entries; /* Entry info array */
    uint32     *cache_p;        /* Pointer to concatenated cached entries */   
} soc_profile_mem_table_t;

typedef struct soc_profile_mem_s {
    soc_profile_mem_table_t *tables;  /* Table info array */
    int        table_count;           /* Number of entries in tables array */
    uint32     flags;                 /* SOC_PROFILE_MEM_F_xxx */
} soc_profile_mem_t;

typedef struct soc_profile_reg_entry_s {
    uint32     ref_count;       /* Reference counters */
    int        entries_per_set; /* Number of entries per set */
    uint64     *cache_p;        /* Pointer to cached entries */
} soc_profile_reg_entry_t;

typedef struct soc_profile_reg_s {
    soc_reg_t  *regs;                 /* Pointer to array of register id */
    int        regs_count;            /* Number of entries in regs */
    soc_profile_reg_entry_t *entries; /* Pointer to array of entries */
} soc_profile_reg_t;

extern void soc_profile_mem_t_init(soc_profile_mem_t *profile);
extern int soc_profile_mem_index_create(int unit,
                                        soc_mem_t *mem_array,
                                        int *entry_words_array,
                                        int *index_min_array,
                                        int *index_max_array,
                                        void **data_mask_array,
                                        int table_count,
                                        soc_profile_mem_t *profile);
extern int soc_profile_mem_create(int unit,
                                  soc_mem_t *mem_array,
                                  int *entry_words_array,
                                  int table_count,
                                  soc_profile_mem_t *profile);
extern int soc_profile_mem_destroy(int unit,
                                   soc_profile_mem_t *profile);
extern int soc_profile_mem_add(int unit,
                               soc_profile_mem_t *profile,
                               void **entries,
                               int entries_per_set0,
                               uint32 *index0);
extern int soc_profile_mem_single_table_add(int unit,
                                            soc_profile_mem_t *profile,
                                            void *entries,
                                            int entries_per_set,
                                            int *index);
extern int soc_profile_mem_delete(int unit,
                                  soc_profile_mem_t *profile,
                                  uint32 index);
extern int soc_profile_mem_set(int unit,
                               soc_profile_mem_t *profile,
                               void **entries_array,
                               uint32 index0);
extern int soc_profile_mem_get(int unit,
                               soc_profile_mem_t *profile,
                               int index0,
                               int count,
                               void **entries_array);
extern int soc_profile_mem_single_table_get(int unit,
                                            soc_profile_mem_t *profile,
                                            int index,
                                            int count,
                                            void *entries);
extern int soc_profile_mem_reference(int unit,
                                     soc_profile_mem_t *profile,
                                     int index0,
                                     int entries_per_set0);
extern int soc_profile_mem_ref_count_get(int unit,
                                         soc_profile_mem_t *profile,
                                         int index, int *ref_count);
extern int soc_profile_mem_fields32_modify(int unit,
                                           soc_profile_mem_t *profile,
                                           int table_id,
                                           int field_count,
                                           soc_field_t *fields,
                                           uint32 *values);
extern int soc_profile_mem_field32_modify(int unit,
                                          soc_profile_mem_t *profile,
                                          int table_id,
                                          soc_field_t field,
                                          uint32 value);

extern void soc_profile_reg_t_init(soc_profile_reg_t *profile_reg);
extern int soc_profile_reg_create(int unit,
                                  soc_reg_t *regs,
                                  int regs_count,
                                  soc_profile_reg_t *profile_reg);
extern int soc_profile_reg_destroy(int unit,
                                   soc_profile_reg_t *profile_reg);
extern int soc_profile_reg_add(int unit,
                               soc_profile_reg_t *profile_reg,
                               uint64 **entries,
                               int entries_per_set,
                               uint32 *index);
extern int soc_profile_reg_delete(int unit,
                                  soc_profile_reg_t *profile_reg,
                                  uint32 index);
extern int soc_profile_reg_get(int unit,
                               soc_profile_reg_t *profile_reg,
                               uint32 index,
                               int count,
                               uint64 **entries);
extern int soc_profile_reg_reference(int unit,
                                     soc_profile_reg_t *profile_reg,
                                     uint32 index,
                                     int entries_per_set_override);
extern int soc_profile_reg_ref_count_get(int unit,
                                         soc_profile_reg_t *profile_reg,
                                         uint32 index, int *ref_count);
extern int soc_profile_mem_sw_state_set(int unit, soc_profile_mem_t *profile,
                                        void **entries_array, int ent_per_set0,
                                        uint32 index0);

/* Macro to get a pointer to the entry at the specified index */
#define SOC_PROFILE_MEM_ENTRY(_unit, _profile_mem, _cast, _index) \
    ((_cast)&_profile_mem->tables[0].cache_p[(_index) * _profile_mem->tables[0].entry_words])

/* Macro to adjust the ref count at the specifed index.
 * Positive and negative "_count" values are accepted.
 */
#define SOC_PROFILE_MEM_REFERENCE(_unit, _profile_mem, _index, _count) \
    (_profile_mem->tables[0].entries[_index].ref_count += _count)

/* Macro to set the number of entries per set */
#define SOC_PROFILE_MEM_ENTRIES_PER_SET(_unit, _profile_mem, _index, _num) \
    (_profile_mem->tables[0].entries[_index].entries_per_set = _num)

#endif /* !_SOC_PROFILE_MEM_H */
