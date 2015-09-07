/* 
 * $Id: l2x.c,v 1.41 Broadcom SDK $
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
 * File:        l2x.c
 *      
 * Provides:
 *      soc_internal_l2x_read
 *      soc_internal_l2x_write
 *      soc_internal_l2x_lkup
 *      soc_internal_l2x_init
 *      soc_internal_l2x_del
 *      soc_internal_l2x_ins
 *      
 * Requires:    
 */

#include <shared/bsl.h>

#include <soc/mem.h>
#include <soc/hash.h>
#include <soc/drv.h>
#include <soc/l2x.h>
#ifdef BCM_TRIDENT2_SUPPORT
#include <soc/trident2.h>
#endif

#include "pcid.h"
#include "mem.h"
#include "cmicsim.h"

#ifdef BCM_XGS_SWITCH_SUPPORT
#ifdef BCM_FIREBOLT_SUPPORT
int
soc_internal_l2x2_hash(pcid_info_t * pcid_info, l2x_entry_t *entry, int dual)
{
    uint32          tmp_hs[SOC_MAX_MEM_WORDS];
    int             hash_sel;
    uint8           at, key[8];
    int             index;
    soc_block_t     blk;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META("L2X2 hash\n")));

    if (dual) {
        soc_internal_extended_read_reg(pcid_info, blk, at,
                soc_reg_addr_get(pcid_info->unit, L2_AUX_HASH_CONTROLr,
                               REG_PORT_ANY, 0, FALSE, &blk, &at),
                tmp_hs);
        hash_sel = soc_reg_field_get(pcid_info->unit, L2_AUX_HASH_CONTROLr,
                                     tmp_hs[0], HASH_SELECTf);
    } else {
        soc_internal_extended_read_reg(pcid_info, blk, at,
                soc_reg_addr_get(pcid_info->unit, HASH_CONTROLr, REG_PORT_ANY, 
                               0, FALSE, &blk, &at), 
                tmp_hs);
        hash_sel = soc_reg_field_get(pcid_info->unit, HASH_CONTROLr,
                                     tmp_hs[0], L2_AND_VLAN_MAC_HASH_SELECTf);
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META("hash_sel%s %d\n"), dual ? "" : "(aux)", hash_sel));

    soc_draco_l2x_base_entry_to_key(pcid_info->unit, entry, key);

    index = soc_fb_l2_hash(pcid_info->unit, hash_sel, key);

    if (SOC_IS_FIREBOLT2(pcid_info->unit)) {
        uint32 addr_mask;
        soc_internal_read_reg(pcid_info,
              soc_reg_addr(pcid_info->unit, L2_ENTRY_ADDR_MASKr,
                           REG_PORT_ANY, 0),
              tmp_hs);
        addr_mask = soc_reg_field_get(pcid_info->unit, L2_ENTRY_ADDR_MASKr,
                                      tmp_hs[0], MASKf);
        index &= addr_mask;
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META("index %d\n"), index));

    return index;
}

void
soc_internal_l2x2_dual_banks(pcid_info_t *pcid_info, uint8 banks,
                             int *dual, int *slot_min, int *slot_max)
{
    uint32          tmp[SOC_MAX_MEM_WORDS];
    soc_block_t     blk;
    uint8           at;

    soc_internal_extended_read_reg(pcid_info, blk, at,
                                   soc_reg_addr_get(pcid_info->unit, 
                                    L2_AUX_HASH_CONTROLr,
                                    REG_PORT_ANY, 0, FALSE, &blk, &at),
                                   tmp);
    *dual = soc_reg_field_get(pcid_info->unit, L2_AUX_HASH_CONTROLr,
                              tmp[0], ENABLEf);

    if (*dual) {
        switch (banks) {
        case 0:
            return; /* Nothing to do here */
        case 1:
            *slot_min = 4;
            *slot_max = 7;
            break;
        case 2:
            *slot_min = 0;
            *slot_max = 3;
            break;
        default:
            *slot_min = 0;
            *slot_max = -1;
            break;
        }
    }

    return;
}

int
soc_internal_l2x2_entry_ins(pcid_info_t *pcid_info, uint8 banks,
                l2x_entry_t *entry, uint32 *result)
{
    uint32 tmp[SOC_MAX_MEM_WORDS];
    int    index = 0, bucket, slot, free_index;
    int    slot_min = 0, slot_max = 7, dual = FALSE;
    int    unit = pcid_info->unit; 
    int    op_fail_pos;
    uint8  at; 

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "l2_entry Insert\n")));

    bucket = soc_internal_l2x2_hash(pcid_info, entry, FALSE);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "bucket %d\n"), bucket));

    if (soc_feature(pcid_info->unit, soc_feature_dual_hash)) {
        soc_internal_l2x2_dual_banks(pcid_info, banks,
                                     &dual, &slot_min, &slot_max);
    }

    /* Check if it should overwrite an existing entry */
    free_index = -1;
    for (slot = slot_min; slot <= slot_max; slot++) {
        if (dual && (slot == 4)) {
            bucket = soc_internal_l2x2_hash(pcid_info, entry, TRUE);
        }
        index = bucket * 8 + slot;
        soc_internal_extended_read_mem(pcid_info, 
                                       SOC_MEM_BLOCK_ANY(unit, L2Xm), at, 
                                       soc_mem_addr_get(unit, L2Xm, 0,
                                        SOC_MEM_BLOCK_ANY(unit, L2Xm),
                                        index, &at), 
                                       tmp);
        if (soc_mem_field32_get(unit, L2Xm, tmp, VALIDf)) {
            if (soc_mem_compare_key(unit, L2Xm, entry, tmp) == 0) {
                /* Overwrite the existing entry */
                PCIM(pcid_info, CMIC_SCHAN_CTRL) &= ~SC_MSG_NAK_TST;

                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "write bucket %d, slot %d, index %d\n"),
                             index / 8, index % 8, index));
                soc_internal_extended_write_mem(pcid_info, 
                                                SOC_MEM_BLOCK_ANY(unit, L2Xm), at,
                                                soc_mem_addr_get(unit, L2Xm, 0, 
                                                 SOC_MEM_BLOCK_ANY(unit, L2Xm),
                                                 index, &at), 
                                                (uint32 *)entry);

                result[3] = 0x000;
                return 0;
            }
        } else {
            if (free_index == -1) {
                free_index = index;
            }
        }
    }

    /* Find first unused slot in bucket */
    if (free_index != -1) {
        index = free_index;

        /* Write the existing entry */
        result[3] = 0x000;
        PCIM(pcid_info, CMIC_SCHAN_CTRL) &= ~SC_MSG_NAK_TST;

        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "write bucket %d, slot %d, index %d\n"),
                     index / 8, index % 8, index));
        soc_internal_extended_write_mem(pcid_info, 
                                        SOC_MEM_BLOCK_ANY(unit, L2Xm), at, 
                                        soc_mem_addr_get(unit, L2Xm, 0, 
                                         SOC_MEM_BLOCK_ANY(unit, L2Xm),
                                         index, &at), 
                                        (uint32 *)entry);
        return 0;
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "Bucket full\n")));

    /*
     * =============================================================
     * | PERR_PBM | MFIFO_FULL | OP_FAIL | Index  | L2x entry data |
     * =============================================================
     */
    op_fail_pos = SOC_L2X_OP_FAIL_POS(unit);

    result[3] = (1 << op_fail_pos);
    PCIM(pcid_info, CMIC_SCHAN_CTRL) |= SC_MSG_NAK_TST;
    return 0;
}

int
soc_internal_l2x2_entry_del(pcid_info_t * pcid_info, uint8 banks,
                l2x_entry_t *entry, uint32 *result)
{
    uint32          tmp[SOC_MAX_MEM_WORDS];
    int             op_fail_pos;
    int             index = 0, bucket, slot;
    int             slot_min = 0, slot_max = 7, dual = FALSE;
    int             unit = pcid_info->unit;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "l2_entry Delete\n")));

    bucket = soc_internal_l2x2_hash(pcid_info, entry, FALSE);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "bucket %d\n"), bucket));

    /*
     * =============================================================
     * | PERR_PBM | MFIFO_FULL | OP_FAIL | Index  | L2x entry data |
     * =============================================================
     */
    op_fail_pos = SOC_L2X_OP_FAIL_POS(unit);

    result[3] = (1 << op_fail_pos);

    if (soc_feature(pcid_info->unit, soc_feature_dual_hash)) {
        soc_internal_l2x2_dual_banks(pcid_info, banks,
                                     &dual, &slot_min, &slot_max);
    }

    /* Check if entry exists */
    for (slot = slot_min; slot <= slot_max; slot++) {
        if (dual && (slot == 4)) {
            bucket = soc_internal_l2x2_hash(pcid_info, entry, TRUE);
        }
        index = bucket * 8 + slot;
        soc_internal_read_mem(pcid_info, soc_mem_addr(unit, L2Xm, 0, 
                                         SOC_MEM_BLOCK_ANY(unit, L2Xm),
                                         index), tmp);
        if (soc_mem_field32_get(unit, L2Xm, tmp, VALIDf)) {
            if (soc_mem_compare_key(unit, L2Xm, entry, tmp) == 0) {
                result[3] = 0x000;

                /* Invalidate entry */
                soc_mem_field32_set(unit, L2Xm, tmp, VALIDf, 0);
                soc_internal_write_mem(pcid_info,
                                       soc_mem_addr(unit, L2Xm, 0, 
                                       SOC_MEM_BLOCK_ANY(unit, L2Xm),
                                       index), (uint32 *)tmp);
            }
        } 
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "Not found\n")));

    return 0;
}

/* 
 * soc_internal_l2x2_entry_lkup
 *
 *   Note: entry_lookup and entry_result may overlap.
 */

int
soc_internal_l2x2_entry_lkup(pcid_info_t * pcid_info, uint8 banks,
                             l2x_entry_t *entry_lookup, uint32 *result)
{
    uint32 tmp[SOC_MAX_MEM_WORDS];
    int    op_fail_pos;
    int    index = 0, bucket, slot;
    int    slot_min = 0, slot_max = 7, dual = FALSE;
    int    unit = pcid_info->unit; 
    uint8  at;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "l2_entry Lookup\n")));

    bucket = soc_internal_l2x2_hash(pcid_info, entry_lookup, FALSE);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "bucket %d\n"), bucket));

    /*
     * =============================================================
     * | PERR_PBM | MFIFO_FULL | OP_FAIL | Index  | L2x entry data |
     * =============================================================
     */
    op_fail_pos = SOC_L2X_OP_FAIL_POS(unit);

    result[3] = (1 << op_fail_pos);
   

    if (soc_feature(unit, soc_feature_dual_hash)) {
        soc_internal_l2x2_dual_banks(pcid_info, banks,
                                     &dual, &slot_min, &slot_max);
    }

    /* Check if entry exists */
    for (slot = slot_min; slot <= slot_max; slot++) {
        if (dual && (slot == 4)) {
            bucket = soc_internal_l2x2_hash(pcid_info, entry_lookup, TRUE);
        }
        index = bucket * 8 + slot;
        soc_internal_extended_read_mem(pcid_info,
                                       SOC_MEM_BLOCK_ANY(unit, L2Xm), at, 
                                       soc_mem_addr_get(unit, L2Xm, 0, 
                                        SOC_MEM_BLOCK_ANY(unit, L2Xm),
                                        index, &at), 
                                       tmp);
        if (soc_mem_field32_get(unit, L2Xm, tmp, VALIDf)) {
            if (soc_mem_compare_key(unit, L2Xm, entry_lookup, tmp) == 0) {
                int nbits; /* Data Bits in last word */

                nbits = soc_mem_entry_bits(unit, L2Xm) % 32;

                result[0] = tmp[0];
                result[1] = tmp[1];
                result[2] = (tmp[2] & ((1 << nbits) - 1)) |
                            ((index & ((1 << (32 - nbits)) - 1)) << nbits);
                result[3] = (index >> (32 - nbits)) &
                            ((soc_mem_index_max(unit, L2Xm)) >> (32 - nbits));
                return 0;
            }
        } 
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "Not found\n")));

    return 0;
}
#endif /* BCM_FIREBOLT_SUPPORT */

#ifdef BCM_TRX_SUPPORT
STATIC int
soc_internal_l2_tr_bucket_get(pcid_info_t *pcid_info, int bank, void *entry)
{
    uint32  rval[SOC_MAX_MEM_WORDS];
    int     bucket, hash_sel;
    int     unit = pcid_info->unit;
    int     blk;
    uint8   acc_type;
    uint32  addr;

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(unit)) {
        int hash_offset, use_lsb;
        soc_reg_t reg;
        static const soc_field_t fields[] = {
            BANK0_HASH_OFFSETf, BANK1_HASH_OFFSETf, BANK2_HASH_OFFSETf,
            BANK3_HASH_OFFSETf, BANK4_HASH_OFFSETf, BANK5_HASH_OFFSETf
        };

        reg = L2_TABLE_HASH_CONTROLr;
        addr = soc_reg_addr_get(unit, reg, REG_PORT_ANY, 0, FALSE, &blk, &acc_type);
        soc_internal_extended_read_reg(pcid_info, blk, acc_type, addr, rval);
        use_lsb = soc_reg_field_get(unit, reg, rval[0], HASH_ZERO_OR_LSBf);

        if (bank > 1) {
            reg = SHARED_TABLE_HASH_CONTROLr;
            addr = soc_reg_addr_get(unit, reg, REG_PORT_ANY, 0, FALSE, &blk,
                                    &acc_type);
            soc_internal_extended_read_reg(pcid_info, blk, acc_type, addr,
                                           rval);
        }
        hash_offset = soc_reg_field_get(unit, reg, rval[0], fields[bank]);

        bucket = soc_td2_l2x_entry_hash(unit, bank, hash_offset, use_lsb,
                                        entry);
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        hash_sel = -1;
        if (bank == 1) {
            addr = soc_reg_addr_get(unit, L2_AUX_HASH_CONTROLr, REG_PORT_ANY,
                                    0, FALSE, &blk, &acc_type);
            soc_internal_extended_read_reg(pcid_info, blk, acc_type, addr,
                                           rval);
            if (soc_reg_field_get(unit, L2_AUX_HASH_CONTROLr, rval[0],
                                  ENABLEf)) {
                hash_sel = soc_reg_field_get(unit, L2_AUX_HASH_CONTROLr,
                                             rval[0], HASH_SELECTf);
            }
        }
        if (hash_sel == -1) {
            addr = soc_reg_addr_get(unit, HASH_CONTROLr, REG_PORT_ANY, 0, FALSE, &blk,
                                    &acc_type);
            soc_internal_extended_read_reg(pcid_info, blk, acc_type, addr,
                                           rval);
            hash_sel = soc_reg_field_get(unit, HASH_CONTROLr, rval[0],
                                         L2_AND_VLAN_MAC_HASH_SELECTf);
        }

        bucket = soc_tr_l2x_entry_hash(unit, hash_sel, entry);
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "bank %d bucket %d\n"), bank, bucket));

    return bucket;
}

STATIC void
soc_internal_l2_tr_bank_map_get(pcid_info_t *pcid_info, uint32 *bank_map,
                                int *bank_start, int *bank_inc,
                                int *do_first_fit)
{
    uint32 rval[SOC_MAX_MEM_WORDS];
    int    unit = pcid_info->unit;
    int    blk;
    uint8  acc_type;
    uint32 addr;

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(pcid_info->unit)) {
        int    mode;

        addr = soc_reg_addr_get(unit, L2_TABLE_HASH_CONTROLr, REG_PORT_ANY, 0,
                                FALSE, &blk, &acc_type);
        soc_internal_extended_read_reg(pcid_info, blk, acc_type, addr, rval);
        mode = soc_reg_field_get(unit, L2_TABLE_HASH_CONTROLr, rval[0], MODEf);
        switch (mode) {
        case 0:
            *bank_map = 0x3f;
            break;
        case 1:
            *bank_map = 0x1f;
            break;
        case 2:
            *bank_map = 0xf;
            break;
        case 3:
            *bank_map = 0x7;
            break;
        default:
            *bank_map = 0x3;
        }
        *bank_start = 0;
        *bank_inc = 1;
        if (do_first_fit != NULL) {
            *do_first_fit = FALSE;
        }
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        *bank_map = 0x3;
        *bank_start = 0;
        *bank_inc = 1;

        addr = soc_reg_addr_get(unit, L2_AUX_HASH_CONTROLr, REG_PORT_ANY, 0,
                                FALSE, &blk, &acc_type);
        soc_internal_extended_read_reg(pcid_info, blk, acc_type, addr, rval);
        if (do_first_fit != NULL) {
            *do_first_fit =
                soc_reg_field_get(unit, L2_AUX_HASH_CONTROLr, rval[0],
                                  INSERT_LEAST_FULL_HALFf) ? FALSE : TRUE;
        }
    }
}

STATIC void
soc_internal_l2_tr_bank_size_get(pcid_info_t *pcid_info, int bank,
                                 int *entries_per_bank, int *entries_per_row,
                                 int *entries_per_bucket, int *bank_base,
                                 int *bucket_offset)
{
    /*
     * entry index =
     *     bank_base + bucket * entries_per_row + bucket_offest + slot;
     */

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(pcid_info->unit)) {
        (void)soc_trident2_hash_bank_info_get(pcid_info->unit, L2Xm,
                                              bank, entries_per_bank,
                                              entries_per_row,
                                              entries_per_bucket,
                                              bank_base, bucket_offset);
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        /*
         * 8 entries per bucket
         * first half of entries (slot 0) in each bucket are for bank 0
         * the other half of entries (slot 1) in each bucket are for bank 1
         */
        if (entries_per_bank != NULL) {
            *entries_per_bank = soc_mem_index_count(pcid_info->unit, L2Xm) / 2;
        }
        *entries_per_row = 8;
        *entries_per_bucket = *entries_per_row / 2;
        *bank_base = 0;
        *bucket_offset = bank * *entries_per_bucket;
    }
}

int
soc_internal_l2_tr_entry_ins(pcid_info_t *pcid_info, uint32 inv_bank_map,
                             void *entry, uint32 *result)
{
    uint32          tmp[SOC_MAX_MEM_WORDS];
    uint32          bank_map;
    int             bank, entries_per_bank, bank_start, bank_inc, free_bank;
    int             bucket, entries_per_row, bank_base, free_bucket;
    int             slot, entries_per_bucket, bucket_offset, free_slot;
    int             index, free_index;
    int             first_free_slot, free_slot_count, do_first_fit;
    int             best_free_slot_count, best_entries_per_bank;
    int             unit = pcid_info->unit;
    schan_genresp_t *genresp = (schan_genresp_t *) result;
    schan_genresp_v2_t *genresp_v2 = (schan_genresp_v2_t *) result;
    int             blk;
    soc_block_t     sblk;
    uint8           acc_type;
    uint32          addr;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "L2X Insert\n")));

    blk = SOC_MEM_BLOCK_ANY(unit, L2Xm);
    sblk = SOC_BLOCK2SCH(unit, blk);

    free_index = -1;

    soc_internal_l2_tr_bank_map_get(pcid_info, &bank_map, &bank_start,
                                    &bank_inc, &do_first_fit);
    bank_map &= ~inv_bank_map;

    best_free_slot_count = 0;
    for (bank = bank_start; bank_map != 0; bank += bank_inc) {
        if (!(bank_map & (1 << bank))) {
            continue;
        }
        bank_map &= ~(1 << bank);

        soc_internal_l2_tr_bank_size_get(pcid_info, bank, &entries_per_bank,
                                         &entries_per_row, &entries_per_bucket,
                                         &bank_base, &bucket_offset);
        bucket = soc_internal_l2_tr_bucket_get(pcid_info, bank, entry);
        free_slot_count = 0;
        first_free_slot = -1;
        for (slot = 0; slot < entries_per_bucket; slot++) {
            index = bank_base + bucket * entries_per_row + bucket_offset +
                slot;
            addr = soc_mem_addr_get(unit, L2Xm, 0, blk, index, &acc_type);
            soc_internal_extended_read_mem(pcid_info, sblk, acc_type, addr,
                                           tmp);
            if (soc_mem_field32_get(unit, L2Xm, tmp, VALIDf)) {
                if (soc_mem_compare_key(unit, L2Xm, entry, tmp) == 0) {
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "write sblk %d acc_type %d bank %d "
                                            "bucket %d, slot %d, index %d\n"),
                                 sblk, acc_type, bank, bucket, slot, index));

                    /* Overwrite the existing entry */
                    soc_internal_extended_write_mem(pcid_info, sblk, acc_type,
                                                    addr, (uint32 *)entry);

                    /* Copy old entry immediately after response word */
                    memcpy(&result[1], tmp, soc_mem_entry_bytes(unit, L2Xm));

                    result[0] = 0;
                    if (soc_feature(unit, soc_feature_new_sbus_format) &&
                        !soc_feature(unit, soc_feature_new_sbus_old_resp) ) {
                        genresp_v2->type = SCHAN_GEN_RESP_TYPE_REPLACED;
                        genresp_v2->index = index;
                    } else {
                        genresp->type = SCHAN_GEN_RESP_TYPE_REPLACED;
                        genresp->index = index;
                    }
                    PCIM(pcid_info, CMIC_SCHAN_CTRL) &= ~SC_MSG_NAK_TST;

                    return 0;
                }
            } else {
                free_slot_count++;
                if (first_free_slot == -1) {
                    first_free_slot = slot;
                }
            }
        }
        if (free_slot_count > 0) {
            if (do_first_fit) {
                if (free_index != -1) {
                    continue;
                }
            } else {
                if (free_slot_count < best_free_slot_count) {
                    continue;
                } else if (free_slot_count == best_free_slot_count) {
                    if (entries_per_bank < best_entries_per_bank) {
                        continue;
                    } else if (entries_per_bank == best_entries_per_bank) {
                        if (free_index != -1 && bank > free_bank) {
                            continue;
                        }
                    }
                }
                best_free_slot_count = free_slot_count;
                best_entries_per_bank = entries_per_bank;
            }
            free_index = index + 1 - entries_per_bucket + first_free_slot;
            free_bank = bank;
            free_bucket = bucket;
            free_slot = first_free_slot;
        }
    }

    /* Find first unused slot in bucket */
    if (free_index != -1) {
        index = free_index;
        addr = soc_mem_addr_get(unit, L2Xm, 0, blk, index, &acc_type);

        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "write sblk %d acc_type %d bank %d bucket %d, "
                                "slot %d, index %d\n"),
                     sblk, acc_type, free_bank, free_bucket, free_slot, index));

        /* Write the entry */
        soc_internal_extended_write_mem(pcid_info, sblk, acc_type, addr,
                                        entry);

        result[0] = 0;
        if (soc_feature(unit, soc_feature_new_sbus_format) &&
            !soc_feature(unit, soc_feature_new_sbus_old_resp) ) {
            genresp_v2->type = SCHAN_GEN_RESP_TYPE_INSERTED;
            genresp_v2->index = index;
        } else {
            genresp->type = SCHAN_GEN_RESP_TYPE_INSERTED;
            genresp->index = index;
        }
        PCIM(pcid_info, CMIC_SCHAN_CTRL) &= ~SC_MSG_NAK_TST;

        return 0;
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "Bucket full\n")));
    result[0] = 0;
    if (soc_feature(unit, soc_feature_new_sbus_format) &&
        !soc_feature(unit, soc_feature_new_sbus_old_resp) ) {
        genresp_v2->type = SCHAN_GEN_RESP_TYPE_FULL;
    } else {
        genresp->type = SCHAN_GEN_RESP_TYPE_FULL;
    }
    PCIM(pcid_info, CMIC_SCHAN_CTRL) |= SC_MSG_NAK_TST;

    return 0;
}

int
soc_internal_l2_tr_entry_del(pcid_info_t *pcid_info, uint32 inv_bank_map,
                             void *entry, uint32 *result)
{
    uint32          tmp[SOC_MAX_MEM_WORDS];
    uint32          bank_map;
    int             bank, bank_start, bank_inc, bank_base;
    int             bucket, entries_per_row;
    int             slot, entries_per_bucket, bucket_offset;
    int             index;
    int             unit = pcid_info->unit;
    schan_genresp_t *genresp = (schan_genresp_t *) result;
    schan_genresp_v2_t *genresp_v2 = (schan_genresp_v2_t *) result;
    int             blk;
    soc_block_t     sblk;
    uint8           acc_type;
    uint32          addr;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "L2X Delete\n")));

    blk = SOC_MEM_BLOCK_ANY(unit, L2Xm);
    sblk = SOC_BLOCK2SCH(unit, blk);

    soc_internal_l2_tr_bank_map_get(pcid_info, &bank_map, &bank_start,
                                    &bank_inc, NULL);
    bank_map &= ~inv_bank_map;

    for (bank = bank_start; bank_map != 0; bank += bank_inc) {
        if (!(bank_map & (1 << bank))) {
            continue;
        }
        bank_map &= ~(1 << bank);

        soc_internal_l2_tr_bank_size_get(pcid_info, bank, NULL,
                                         &entries_per_row, &entries_per_bucket,
                                         &bank_base, &bucket_offset);
        bucket = soc_internal_l2_tr_bucket_get(pcid_info, bank, entry);

        for (slot = 0; slot < entries_per_bucket; slot++) {
            index = bank_base + bucket * entries_per_row + bucket_offset +
                slot;
            addr = soc_mem_addr_get(unit, L2Xm, 0, blk, index, &acc_type);
            soc_internal_extended_read_mem(pcid_info, sblk, acc_type, addr,
                                           tmp);
            if (soc_mem_field32_get(unit, L2Xm, tmp, VALIDf)) {
                if (soc_mem_compare_key(unit, L2Xm, entry, tmp) == 0) {
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "delete sblk %d acc_type %d bank %d "
                                            "bucket %d, slot %d, index %d\n"),
                                 sblk, acc_type, bank, bucket, slot, index));

                    /* Copy entry immediately after response word */
                    memcpy(&result[1], tmp, soc_mem_entry_bytes(unit, L2Xm));

                    /* Invalidate entry */
                    soc_mem_field32_set(unit, L2Xm, tmp, VALIDf, 0);
                    soc_internal_extended_write_mem(pcid_info, sblk, acc_type,
                                                    addr, (uint32 *)tmp);

                    result[0] = 0;
                    if (soc_feature(unit, soc_feature_new_sbus_format) &&
                        !soc_feature(unit, soc_feature_new_sbus_old_resp) ) {
                        genresp_v2->type = SCHAN_GEN_RESP_TYPE_DELETED;
                        genresp_v2->index = index;
                    } else {
                        genresp->type = SCHAN_GEN_RESP_TYPE_DELETED;
                        genresp->index = index;
                    }
                    PCIM(pcid_info, CMIC_SCHAN_CTRL) &= ~SC_MSG_NAK_TST;

                    return 0;
                }
            }
        }
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "soc_internal_l2_tr_entry_del: Not found\n")));

    result[0] = 0;
    if (soc_feature(unit, soc_feature_new_sbus_format) &&
        !soc_feature(unit, soc_feature_new_sbus_old_resp) ) {
        genresp_v2->type = SCHAN_GEN_RESP_TYPE_NOT_FOUND;
    } else {
        genresp->type = SCHAN_GEN_RESP_TYPE_NOT_FOUND;
    }
    PCIM(pcid_info, CMIC_SCHAN_CTRL) |= SC_MSG_NAK_TST;

    return 0;
}

/*
 * soc_internal_l2_tr_entry_lkup
 *
 *   Note: entry_lookup and entry_result may overlap.
 */

int
soc_internal_l2_tr_entry_lkup(pcid_info_t *pcid_info, uint32 inv_bank_map,
                              void *entry, uint32 *result)
{
    uint32          tmp[SOC_MAX_MEM_WORDS];
    uint32          bank_map;
    int             bank, bank_start, bank_inc, bank_base;
    int             bucket, entries_per_row;
    int             slot, entries_per_bucket, bucket_offset;
    int             index;
    int             unit = pcid_info->unit;
    schan_genresp_t *genresp = (schan_genresp_t *) result;
    schan_genresp_v2_t *genresp_v2 = (schan_genresp_v2_t *) result;
    int             blk;
    soc_block_t     sblk;
    uint8           acc_type;
    uint32          addr;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "L2X Lookup\n")));

    blk = SOC_MEM_BLOCK_ANY(unit, L2Xm);
    sblk = SOC_BLOCK2SCH(unit, blk);

    soc_internal_l2_tr_bank_map_get(pcid_info, &bank_map, &bank_start,
                                    &bank_inc, NULL);
    bank_map &= ~inv_bank_map;

    for (bank = bank_start; bank_map != 0; bank += bank_inc) {
        if (!(bank_map & (1 << bank))) {
            continue;
        }
        bank_map &= ~(1 << bank);

        soc_internal_l2_tr_bank_size_get(pcid_info, bank, NULL,
                                         &entries_per_row, &entries_per_bucket,
                                         &bank_base, &bucket_offset);
        bucket = soc_internal_l2_tr_bucket_get(pcid_info, bank, entry);

        for (slot = 0; slot < entries_per_bucket; slot++) {
            index = bank_base + bucket * entries_per_row + bucket_offset +
                slot;
            addr = soc_mem_addr_get(unit, L2Xm, 0, blk, index, &acc_type);
            soc_internal_extended_read_mem(pcid_info, sblk, acc_type, addr,
                                           tmp);
            if (soc_mem_field32_get(unit, L2Xm, tmp, VALIDf)) {
                if (soc_mem_compare_key(unit, L2Xm, entry, tmp) == 0) {
                    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                                (BSL_META_U(unit,
                                            "found sblk %d acc_type %d bank %d "
                                            "bucket %d, slot %d, index %d\n"),
                                 sblk, acc_type, bank, bucket, slot, index));

                    /* Copy entry immediately after response word */
                    memcpy(&result[1], tmp, soc_mem_entry_bytes(unit, L2Xm));

                    result[0] = 0;
                    if (soc_feature(unit, soc_feature_new_sbus_format) &&
                        !soc_feature(unit, soc_feature_new_sbus_old_resp) ) {
                        genresp_v2->type = SCHAN_GEN_RESP_TYPE_FOUND;
                        genresp_v2->index = index;
                    } else {
                        genresp->type = SCHAN_GEN_RESP_TYPE_FOUND;
                        genresp->index = index;
                    }
                    PCIM(pcid_info, CMIC_SCHAN_CTRL) &= ~SC_MSG_NAK_TST;

                    return 0;
                }
            }
        }
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "soc_internal_l2_tr_entry_lkup: Not found\n")));

    result[0] = 0;
    if (soc_feature(unit, soc_feature_new_sbus_format) &&
        !soc_feature(unit, soc_feature_new_sbus_old_resp) ) {
        genresp_v2->type = SCHAN_GEN_RESP_TYPE_NOT_FOUND;
    } else {
        genresp->type = SCHAN_GEN_RESP_TYPE_NOT_FOUND;
    }
    PCIM(pcid_info, CMIC_SCHAN_CTRL) |= SC_MSG_NAK_TST;

    return 0;
}
#endif /* BCM_TRX_SUPPORT */
#endif
