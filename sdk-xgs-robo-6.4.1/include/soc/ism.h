/*
 * $Id: ism.h,v 1.29 Broadcom SDK $
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
 * ISM management routines
 */

#ifndef _SOC_ISM_H
#define _SOC_ISM_H

#include <shared/bitop.h>
#include <soc/types.h>
#include <soc/mem.h>

#ifdef BCM_ISM_SUPPORT

#define _ISM_BANK_SIZE_HALF     1
#define _ISM_BANK_SIZE_QUARTER  2
#define _ISM_BANK_SIZE_DISABLED 3

#define _ISM_SIZE_MODE_512      0
#define _ISM_SIZE_MODE_256      1
#define _ISM_SIZE_MODE_176      2
#define _ISM_SIZE_MODE_96       3
#define _ISM_SIZE_MODE_80       4
#define _ISM_SIZE_MODE_64       5

#define _CRITERIA_ENTRY         0
#define _CRITERIA_STAGE         1
#define _CRITERIA_BANK          2

typedef struct {
    uint8 entry;
    uint8 stage;
    uint8 bank;
    int8 mode;
    uint32 index;
    uint8 bidx;
} _soc_ism_sbo_t;

typedef enum soc_ism_mem_type_e {
    SOC_ISM_MEM_VLAN_XLATE = 1,
    SOC_ISM_MEM_L2_ENTRY,
    SOC_ISM_MEM_L3_ENTRY,
    SOC_ISM_MEM_EP_VLAN_XLATE,
    SOC_ISM_MEM_MPLS,
    SOC_ISM_MEM_ESM_L2,
    SOC_ISM_MEM_ESM_L3,
    SOC_ISM_MEM_ESM_ACL,
    SOC_ISM_MEM_MAX = SOC_ISM_MEM_ESM_L2,
    SOC_ISM_MEM_TOTAL = SOC_ISM_MEM_ESM_ACL
} soc_ism_mem_type_t;

typedef enum soc_mem_set_type_e {
    SOC_MEM_SET_VLAN_XLATE = SOC_ISM_MEM_VLAN_XLATE,
    SOC_MEM_SET_L2_ENTRY = SOC_ISM_MEM_L2_ENTRY,
    SOC_MEM_SET_L3_ENTRY = SOC_ISM_MEM_L3_ENTRY,
    SOC_MEM_SET_EP_VLAN_XLATE = SOC_ISM_MEM_EP_VLAN_XLATE,
    SOC_MEM_SET_MPLS = SOC_ISM_MEM_MPLS,
    SOC_MEM_SET_MAX = SOC_MEM_SET_MPLS
} soc_mem_set_type_t;

/* VLAN_XLATE/VLAN_MAC, L2_ENTRY, L3_ENTRY, EP_VLAN_XLATE, MPLS */
#define _SOC_ISM_ENTRY_SIZE_QUANTA   1024
#define _SOC_ISM_ENTRIES_PER_BKT     4
#define _SOC_ISM_MAX_TABLES          5
#define _SOC_ISM_MAX_STAGES          4
#define _SOC_ISM_BANKS_PER_STAGE     5
#define _SOC_ISM_MAX_BANKS           20
#define _SOC_ISM_TOTAL_BANKS         64
#define _SOC_ISM_MAX_RAW_BANKS       8
#define _SOC_ISM_MAX_ISM_MEMS        10
#define _SOC_ISM_ENTRY_BITS          108

#define _SOC_ISM_BANKS_PER_STAGE_176 4
#define _SOC_ISM_BANKS_PER_STAGE_96  4
#define _SOC_ISM_BANKS_PER_STAGE_80  4
#define _SOC_ISM_MAX_BANKS_176       16
#define _SOC_ISM_MAX_BANKS_96        16
#define _SOC_ISM_MAX_BANKS_80        16
#define _SOC_ISM_TOTAL_BANKS_176     44
#define _SOC_ISM_TOTAL_BANKS_96      24
#define _SOC_ISM_TOTAL_BANKS_80      20

#define _SOC_ISM_BANK0_START 0
#define _SOC_ISM_BANK0_SIZE (1024*16)
#define _SOC_ISM_BANK1_START (_SOC_ISM_BANK0_START + _SOC_ISM_BANK0_SIZE) 
#define _SOC_ISM_BANK1_SIZE (1024*8)
#define _SOC_ISM_BANK2_START (_SOC_ISM_BANK1_START + _SOC_ISM_BANK1_SIZE )
#define _SOC_ISM_BANK2_SIZE (1024*4)
#define _SOC_ISM_BANK3_START (_SOC_ISM_BANK2_START + _SOC_ISM_BANK2_SIZE)
#define _SOC_ISM_BANK3_SIZE (1024*2)
#define _SOC_ISM_BANK4_START (_SOC_ISM_BANK3_START + _SOC_ISM_BANK3_SIZE)
#define _SOC_ISM_BANK4_SIZE (1024*2)
#define _SOC_ISM_STAGE_SIZE (_SOC_ISM_BANK4_START + _SOC_ISM_BANK4_SIZE)
#define _SOC_ISM_SIZE (_SOC_ISM_BANK0_SIZE + _SOC_ISM_BANK1_SIZE + \
                       _SOC_ISM_BANK2_SIZE + _SOC_ISM_BANK3_SIZE + \
                       _SOC_ISM_BANK4_SIZE) * _SOC_ISM_MAX_STAGES

#define _SOC_ISM_256_BANK0_START 0
#define _SOC_ISM_256_BANK0_SIZE (1024*8)
#define _SOC_ISM_256_BANK1_START (_SOC_ISM_256_BANK0_START + _SOC_ISM_256_BANK0_SIZE) 
#define _SOC_ISM_256_BANK1_SIZE (1024*4)
#define _SOC_ISM_256_BANK2_START (_SOC_ISM_256_BANK1_START + _SOC_ISM_256_BANK1_SIZE )
#define _SOC_ISM_256_BANK2_SIZE (1024*2)
#define _SOC_ISM_256_BANK3_START (_SOC_ISM_256_BANK2_START + _SOC_ISM_256_BANK2_SIZE)
#define _SOC_ISM_256_BANK3_SIZE (1024*1)
#define _SOC_ISM_256_BANK4_START (_SOC_ISM_256_BANK3_START + _SOC_ISM_256_BANK3_SIZE)
#define _SOC_ISM_256_BANK4_SIZE (1024*1)
#define _SOC_ISM_256_STAGE_SIZE (_SOC_ISM_256_BANK4_START + _SOC_ISM_256_BANK4_SIZE)
#define _SOC_ISM_256_SIZE (_SOC_ISM_256_BANK0_SIZE + _SOC_ISM_256_BANK1_SIZE + \
                           _SOC_ISM_256_BANK2_SIZE + _SOC_ISM_256_BANK3_SIZE + \
                           _SOC_ISM_256_BANK4_SIZE) * _SOC_ISM_MAX_STAGES

#define _SOC_ISM_256_176_BANK0_START 0
#define _SOC_ISM_256_176_BANK0_SIZE (1024*4)
#define _SOC_ISM_256_176_BANK1_START (_SOC_ISM_256_176_BANK0_START + _SOC_ISM_256_176_BANK0_SIZE) 
#define _SOC_ISM_256_176_BANK1_SIZE (1024*4)
#define _SOC_ISM_256_176_BANK2_START (_SOC_ISM_256_176_BANK1_START + _SOC_ISM_256_176_BANK1_SIZE )
#define _SOC_ISM_256_176_BANK2_SIZE (1024*2)
#define _SOC_ISM_256_176_BANK3_START (_SOC_ISM_256_176_BANK2_START + _SOC_ISM_256_176_BANK2_SIZE)
#define _SOC_ISM_256_176_BANK3_SIZE (1024*1)
#define _SOC_ISM_256_176_STAGE_SIZE (_SOC_ISM_256_176_BANK3_START + _SOC_ISM_256_176_BANK3_SIZE)
#define _SOC_ISM_256_176_SIZE (_SOC_ISM_256_176_BANK0_SIZE + _SOC_ISM_256_176_BANK1_SIZE + \
                               _SOC_ISM_256_176_BANK2_SIZE + _SOC_ISM_256_176_BANK3_SIZE) * \
                               _SOC_ISM_MAX_STAGES

#define _SOC_ISM_256_96_BANK1_START 0
#define _SOC_ISM_256_96_BANK1_SIZE (1024*2)
#define _SOC_ISM_256_96_BANK2_START (_SOC_ISM_256_96_BANK1_START + _SOC_ISM_256_96_BANK1_SIZE )
#define _SOC_ISM_256_96_BANK2_SIZE (1024*2)
#define _SOC_ISM_256_96_BANK3_START (_SOC_ISM_256_96_BANK2_START + _SOC_ISM_256_96_BANK2_SIZE)
#define _SOC_ISM_256_96_BANK3_SIZE (1024*1)
#define _SOC_ISM_256_96_BANK4_START (_SOC_ISM_256_96_BANK3_START + _SOC_ISM_256_96_BANK3_SIZE)
#define _SOC_ISM_256_96_BANK4_SIZE (1024*1)
#define _SOC_ISM_256_96_STAGE_SIZE (_SOC_ISM_256_96_BANK4_START + _SOC_ISM_256_96_BANK4_SIZE)
#define _SOC_ISM_256_96_SIZE (_SOC_ISM_256_96_BANK1_SIZE + _SOC_ISM_256_96_BANK2_SIZE + \
                               _SOC_ISM_256_96_BANK3_SIZE + _SOC_ISM_256_96_BANK4_SIZE) * \
                               _SOC_ISM_MAX_STAGES

#define _SOC_ISM_256_80_BANK1_START 0
#define _SOC_ISM_256_80_BANK1_SIZE (1024*2)
#define _SOC_ISM_256_80_BANK2_START (_SOC_ISM_256_80_BANK1_START + _SOC_ISM_256_80_BANK1_SIZE )
#define _SOC_ISM_256_80_BANK2_SIZE (1024*1)
#define _SOC_ISM_256_80_BANK3_START (_SOC_ISM_256_80_BANK2_START + _SOC_ISM_256_80_BANK2_SIZE)
#define _SOC_ISM_256_80_BANK3_SIZE (1024*1)
#define _SOC_ISM_256_80_BANK4_START (_SOC_ISM_256_80_BANK3_START + _SOC_ISM_256_80_BANK3_SIZE)
#define _SOC_ISM_256_80_BANK4_SIZE (1024*1)
#define _SOC_ISM_256_80_STAGE_SIZE (_SOC_ISM_256_80_BANK4_START + _SOC_ISM_256_80_BANK4_SIZE)
#define _SOC_ISM_256_80_SIZE (_SOC_ISM_256_80_BANK1_SIZE + _SOC_ISM_256_80_BANK2_SIZE + \
                               _SOC_ISM_256_80_BANK3_SIZE + _SOC_ISM_256_80_BANK4_SIZE) * \
                               _SOC_ISM_MAX_STAGES

typedef struct {
    int count;
    uint8 index[_SOC_ISM_MAX_RAW_BANKS];
} _soc_ism_real_bank_map_t;

typedef struct {
    char name[80];
    uint32 mem; /* Logical set */
    uint8 index;
    uint8 epb; /* largest entries per bucket */
} _soc_ism_table_index_t;

typedef struct {
    uint32 mem;
    uint32 size; /* Number of widest entries in 1k units */
} soc_ism_mem_size_config_t;

typedef struct _soc_ism_s {
    uint8 ism_mode;
    uint32 *bank_raw_sizes;
    uint32 total_entries;
    uint8 banks_per_stage;
    uint8 max_banks;
    uint8 total_banks;
    uint8 max_raw_banks;
    _soc_ism_real_bank_map_t *real_bank_map;
} _soc_ism_t;

typedef struct {
    uint8 banks[_SOC_ISM_MAX_BANKS]; 
    uint32 bank_size[_SOC_ISM_MAX_BANKS];
    uint8 count;
} _soc_ism_mem_banks_t;

typedef struct _soc_ism_pd_rf_s {
    soc_reg_t pdr;
    soc_field_t pdf[5];
} _soc_ism_pd_rf_t;

typedef struct _soc_ism_pd_s {
    _soc_ism_pd_rf_t reg_field[7];
    _soc_ism_pd_rf_t hit_reg_field[3];
} _soc_ism_pd_t;

extern uint32 _soc_ism_bank_raw_sizes[];
extern uint32 _soc_ism_bank_raw_sizes_256[];
extern uint32 _soc_ism_bank_raw_sizes_256_176[];
extern uint32 _soc_ism_bank_raw_sizes_256_96[];
extern uint32 _soc_ism_bank_raw_sizes_256_80[];
extern _soc_ism_real_bank_map_t _soc_ism_real_bank_map[];
extern _soc_ism_real_bank_map_t _soc_ism_real_bank_map_176[];
extern _soc_ism_real_bank_map_t _soc_ism_real_bank_map_96[];
extern _soc_ism_real_bank_map_t _soc_ism_real_bank_map_80[];
extern uint32 _soc_ism_bank_avail[SOC_MAX_NUM_DEVICES][_SOC_ISM_MAX_BANKS]; 
extern int soc_ism_get_hash_mem_idx(int unit, soc_mem_t mem);
extern char *soc_ism_table_to_name(uint32 mem);
extern int soc_ism_mem_config(int unit, 
                              soc_ism_mem_size_config_t *mem_cfg,
                              int count);
extern int soc_ism_hash_init(int unit);
extern int soc_ism_hw_config(int unit);
extern uint32 _soc_ism_bank_total(int unit);
/* Note: offset_config can only be called after hw_config */
extern int soc_ism_hash_offset_config(int unit, uint8 bank, uint8 offset);
extern int soc_ism_hash_offset_config_get(int unit, uint8 bank, uint8 *offset);
extern int soc_ism_table_hash_config(int unit, soc_ism_mem_type_t table, 
                                     uint8 zero_lsb);
extern int soc_ism_mem_hash_config(int unit, soc_mem_t mem, uint8 zero_lsb);
extern int soc_ism_table_hash_config_get(int unit, soc_ism_mem_type_t table, 
                                         uint8 *zero_lsb);
extern int soc_ism_mem_hash_config_get(int unit, soc_mem_t mem, uint8 *zero_lsb);
extern int soc_ism_mem_max_key_bits_get(int unit, soc_mem_t mem);
extern int soc_ism_hash_max_offset_get(int unit, int *offset);
extern int soc_ism_hash_table_offset_config_get(int unit, soc_ism_mem_type_t table, 
                                                uint8 *offset, uint8 *count);
extern int soc_ism_hash_mem_offset_config_get(int unit, soc_mem_t mem, uint8 *offset, 
                                              uint8 *count);
extern int soc_ism_get_total_banks(int unit, int *count);
extern uint32 soc_ism_get_phy_bank_mask(int unit, uint32 bank_mask);
extern int soc_ism_get_banks(int unit, soc_ism_mem_type_t table, uint8 *banks, 
                             uint32 *bank_size, uint8 *count);
extern int soc_ism_get_banks_for_mem(int unit, soc_mem_t mem, uint8 *banks, 
                                     uint32 *bank_size, uint8 *count);
extern int soc_ism_get_hash_bucket_mask(int buckets);
extern int soc_ism_get_hash_bits(int buckets);
extern void soc_ism_gen_key_from_keyfields(int unit, soc_mem_t mem, void *entry, 
                                           soc_field_t *keyflds, uint8 *key, 
                                           uint8 num_flds);
extern void soc_ism_gen_crc_key_from_keyfields(int unit, soc_mem_t mem, void *entry, 
                                               soc_field_t *keyflds, uint8 *key, 
                                               uint8 num_flds, uint16 *bit_count);
extern void soc_ism_resolve_entry_index(_soc_ism_sbo_t *sbo, uint8 num_banks);
extern int soc_gen_entry_from_key(int unit, soc_mem_t mem, uint8 *key, void *entry);
extern int soc_gen_key_from_entry(int unit, soc_mem_t mem, void *entry, void *key);
extern int soc_generic_gen_hash(int unit, uint32 zero_lsb, uint32 num_bits, 
                                uint32 offset, uint32 mask, uint8 *key, 
                                uint16 lsb);

extern int soc_generic_hash(int unit, soc_mem_t mem, void *entry, int32 banks, 
                            uint8 op, int *index, uint32 *result, uint32 *base_idx,
                            uint8 *num_entries);
extern int soc_generic_get_hash_key(int unit, soc_mem_t mem, void *entry, 
                                    soc_field_t *keyf, soc_field_t *lsbf, 
                                    uint8 *num_flds);
extern int soc_mem_multi_hash_move(int unit, soc_mem_t mem, int32 banks, int copyno,
                                   void *entry, SHR_BITDCL *bucket_trace, 
                                   _soc_ism_mem_banks_t *banks_info, int recurse_depth);
extern uint8 soc_ism_get_bucket_offset(int unit, soc_mem_t mem, int8 midx, void *new_entry, 
                                       void *existing_entry);
#endif /* BCM_ISM_SUPPORT */

#endif  /* _SOC_ISM_H */
