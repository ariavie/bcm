/* 
 * $Id: mpls.c,v 1.15 Broadcom SDK $
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
 * File:        vlan.c
 *      
 * Provides:
 *      soc_internal_mpls_hash
 *      soc_internal_mpls_dual_banks
 *      soc_internal_mpls_entry_read
 *      soc_internal_mpls_entry_write
 *      soc_internal_mpls_entry_ins
 *      soc_internal_mpls_entry_del
 *      soc_internal_mpls_entry_lkup
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

#ifdef BCM_TRIUMPH_SUPPORT

int
soc_internal_mpls_hash(pcid_info_t * pcid_info, 
                             mpls_entry_entry_t *entry, int dual)
{
    uint32          tmp_hs[SOC_MAX_MEM_WORDS];
    int             hash_sel;
    uint8           at, key[8];
    int             index, bits;
    soc_block_t     blk;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META("MPLS_ENTRY hash\n")));

    soc_internal_extended_read_reg(pcid_info, blk, at,
          soc_reg_addr_get(pcid_info->unit, MPLS_ENTRY_HASH_CONTROLr,
                       REG_PORT_ANY, 0, FALSE, &blk, &at),
          tmp_hs);
    if (dual) {
        hash_sel = soc_reg_field_get(pcid_info->unit, MPLS_ENTRY_HASH_CONTROLr,
                                     tmp_hs[0], HASH_SELECT_Bf);
    } else {
        hash_sel = soc_reg_field_get(pcid_info->unit, MPLS_ENTRY_HASH_CONTROLr,
                                     tmp_hs[0], HASH_SELECT_Af);
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META("hash_sel%s %d\n"), dual ? "(b)" : "(a)", hash_sel));

#ifdef BCM_TRIDENT2_SUPPORT
    if (SOC_IS_TD2_TT2(pcid_info->unit)) {
        bits = soc_td2_mpls_base_entry_to_key(pcid_info->unit, entry, key);
        index = soc_td2_mpls_hash(pcid_info->unit, hash_sel, bits, entry, key);
    } else
#endif /* BCM_TRIDENT2_SUPPORT */
    {
        bits = soc_tr_mpls_base_entry_to_key(pcid_info->unit, (uint32 *)entry,
                                             key);
        index = soc_tr_mpls_hash(pcid_info->unit, hash_sel, bits,
                                 (uint32 *)entry, key);
    }

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META("index %d\n"), index));

    return index;
}

void
soc_internal_mpls_dual_banks(pcid_info_t *pcid_info, uint8 banks,
                                   int *dual, int *slot_min, int *slot_max)
{
    /* Dual hash always enabled for MPLS_ENTRY */
    *dual = TRUE;

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
soc_internal_mpls_entry_read(pcid_info_t * pcid_info, uint32 addr,
                                   mpls_entry_entry_t *entry)
{
    int index = (addr & 0x3fff);
    int             blk;             
    soc_block_t     sblk;
    uint8           at;
    uint32          address;

    blk = SOC_MEM_BLOCK_ANY(pcid_info->unit, MPLS_ENTRYm);
    sblk = SOC_BLOCK2SCH(pcid_info->unit, blk);
    address = soc_mem_addr_get(pcid_info->unit, MPLS_ENTRYm, 0,
                               blk, index, &at);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META("mpls_entry read addr=0x%x\n"), addr));

    soc_internal_extended_read_mem(pcid_info, sblk, 0, address, (uint32 *) entry);

    return 0;
}

int
soc_internal_mpls_entry_write(pcid_info_t * pcid_info, uint32 addr,
                                    mpls_entry_entry_t *entry)
{
    int index = (addr & 0x3fff);
    int             blk;             
    soc_block_t     sblk;
    uint8           at;
    uint32          address;

    blk = SOC_MEM_BLOCK_ANY(pcid_info->unit, MPLS_ENTRYm);
    sblk = SOC_BLOCK2SCH(pcid_info->unit, blk);
    address = soc_mem_addr_get(pcid_info->unit, MPLS_ENTRYm, 0, 
                               blk, index, &at);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META("mpls_entry write addr=0x%x\n"), addr));

    soc_internal_extended_write_mem(pcid_info, sblk, 0, address, (uint32 *) entry);

    return 0;
}

int
soc_internal_mpls_entry_ins(pcid_info_t *pcid_info, uint8 banks,
                            mpls_entry_entry_t *entry, uint32 *result)
{
    uint32          tmp[SOC_MAX_MEM_WORDS];
    int             index = 0, bucket, slot, free_index;
    int             slot_min = 0, slot_max = 7, dual = FALSE;
    int             unit = pcid_info->unit;
    schan_genresp_t *genresp = (schan_genresp_t *)result;
    schan_genresp_v2_t *genresp_v2 = (schan_genresp_v2_t *)result;
    int             blk;
    soc_block_t     sblk;
    uint8           acc_type;
    uint32          addr;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "MPLS_ENTRY Insert\n")));

    bucket = soc_internal_mpls_hash(pcid_info, entry, FALSE);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "bucket %d\n"), bucket));

    if (soc_feature(pcid_info->unit, soc_feature_dual_hash)) {
        soc_internal_mpls_dual_banks(pcid_info, banks,
                                     &dual, &slot_min, &slot_max);
    }
    blk = SOC_MEM_BLOCK_ANY(unit, MPLS_ENTRYm);
    sblk = SOC_BLOCK2SCH(unit, blk);

    /* Check if it should overwrite an existing entry */
    free_index = -1;
    for (slot = slot_min; slot <= slot_max; slot++) {
        if (dual && (slot == 4)) {
            bucket = soc_internal_mpls_hash(pcid_info, entry, TRUE);
        }
        index = bucket * 8 + slot;
        addr = soc_mem_addr_get(unit, MPLS_ENTRYm, 0, blk, index, &acc_type);
        soc_internal_extended_read_mem(pcid_info, sblk, acc_type, addr, tmp);
        if (soc_mem_field32_get(unit, MPLS_ENTRYm, tmp, VALIDf)) {
            if (soc_mem_compare_key(unit, MPLS_ENTRYm, entry, tmp) == 0) {
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "write sblk %d acc_type %d bucket %d, "
                                        "slot %d, index %d\n"),
                             sblk, acc_type, index / 8, index % 8, index));

                /* Overwrite the existing entry */
                soc_internal_extended_write_mem(pcid_info, sblk, acc_type,
                                                addr, (uint32 *)entry);

                /* Copy old entry immediately after response word */
                memcpy(&result[1], tmp,
                       soc_mem_entry_bytes(unit, MPLS_ENTRYm));

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
            if (free_index == -1) {
                free_index = index;
            }
        }
    }

    /* Find first unused slot in bucket */
    if (free_index != -1) {
        index = free_index;
        addr = soc_mem_addr_get(unit, MPLS_ENTRYm, 0, blk, index, &acc_type);

        LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "write sblk %d acc_type %d bucket %d, slot %d, "
                                "index %d\n"),
                     sblk, acc_type, index / 8, index % 8, index));

        /* Write the new entry */
        soc_internal_extended_write_mem(pcid_info, sblk, acc_type, addr,
                                        (uint32 *)entry);

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
soc_internal_mpls_entry_del(pcid_info_t * pcid_info, uint8 banks,
                            mpls_entry_entry_t *entry, uint32 *result)
{
    uint32          tmp[SOC_MAX_MEM_WORDS];
    int             index = 0, bucket, slot;
    int             slot_min = 0, slot_max = 7, dual = FALSE;
    int             unit = pcid_info->unit;
    schan_genresp_t *genresp = (schan_genresp_t *)result;
    schan_genresp_v2_t *genresp_v2 = (schan_genresp_v2_t *)result;
    int             blk;
    soc_block_t     sblk;
    uint8           acc_type;
    uint32          addr;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "MPLS_ENTRY Delete\n")));

    bucket = soc_internal_mpls_hash(pcid_info, entry, FALSE);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "bucket %d\n"), bucket));

    if (soc_feature(pcid_info->unit, soc_feature_dual_hash)) {
        soc_internal_mpls_dual_banks(pcid_info, banks, &dual, &slot_min,
                                     &slot_max);
    }
    blk = SOC_MEM_BLOCK_ANY(unit, MPLS_ENTRYm);
    sblk = SOC_BLOCK2SCH(unit, blk);

    /* Check if entry exists */
    for (slot = slot_min; slot <= slot_max; slot++) {
        if (dual && (slot == 4)) {
            bucket = soc_internal_mpls_hash(pcid_info, entry, TRUE);
        }
        index = bucket * 8 + slot;
        addr = soc_mem_addr_get(unit, MPLS_ENTRYm, 0, blk, index, &acc_type);
        soc_internal_extended_read_mem(pcid_info, sblk, acc_type, addr, tmp);
        if (soc_mem_field32_get(unit, MPLS_ENTRYm, tmp, VALIDf)) {
            if (soc_mem_compare_key(unit, MPLS_ENTRYm, entry, tmp) == 0) {
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "found sblk %d acc_type %d bucket %d, "
                                        "slot %d, index %d\n"),
                             sblk, acc_type, bucket, slot, index));

                /* Copy entry immediately after response word */
                memcpy(&result[1], tmp,
                       soc_mem_entry_bytes(unit, MPLS_ENTRYm));

                /* Invalidate entry */
                soc_mem_field32_set(unit, MPLS_ENTRYm, tmp, VALIDf, 0);
                soc_internal_extended_write_mem(pcid_info, sblk, acc_type,
                                                addr, tmp);

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

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "soc_internal_mpls_entry_del: Not found\n")));

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

int
soc_internal_mpls_entry_lkup(pcid_info_t * pcid_info, uint8 banks,
                             mpls_entry_entry_t *entry, uint32 *result)
{
    uint32          tmp[SOC_MAX_MEM_WORDS];
    int             index = 0, bucket, slot;
    int             slot_min = 0, slot_max = 7, dual = FALSE;
    int             unit = pcid_info->unit;
    schan_genresp_t *genresp = (schan_genresp_t *)result;
    schan_genresp_v2_t *genresp_v2 = (schan_genresp_v2_t *)result;
    int             blk;
    soc_block_t     sblk;
    uint8           acc_type;
    uint32          addr;

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "MPLS_ENTRY Lookup\n")));

    bucket = soc_internal_mpls_hash(pcid_info, entry, FALSE);

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "bucket %d\n"), bucket));

    if (soc_feature(unit, soc_feature_dual_hash)) {
        soc_internal_mpls_dual_banks(pcid_info, banks, &dual, &slot_min,
                                     &slot_max);
    }
    blk = SOC_MEM_BLOCK_ANY(unit, MPLS_ENTRYm);
    sblk = SOC_BLOCK2SCH(unit, blk);

    /* Check if entry exists */
    for (slot = slot_min; slot <= slot_max; slot++) {
        if (dual && (slot == 4)) {
            bucket = soc_internal_mpls_hash(pcid_info, entry, TRUE);
        }
        index = bucket * 8 + slot;
        addr = soc_mem_addr_get(unit, MPLS_ENTRYm, 0, blk, index, &acc_type);
        soc_internal_extended_read_mem(pcid_info, sblk, acc_type, addr, tmp);
        if (soc_mem_field32_get(unit, MPLS_ENTRYm, tmp, VALIDf)) {
            if (soc_mem_compare_key(unit, MPLS_ENTRYm, entry, tmp) == 0) {
                LOG_VERBOSE(BSL_LS_SOC_COMMON,
                            (BSL_META_U(unit,
                                        "found sblk %d acc_type %d bucket %d, "
                                        "slot %d, index %d\n"),
                             sblk, acc_type, bucket, slot, index));

                /* Copy entry immediately after response word */
                memcpy(&result[1], tmp,
                       soc_mem_entry_bytes(unit, MPLS_ENTRYm));

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

    LOG_VERBOSE(BSL_LS_SOC_COMMON,
                (BSL_META_U(unit,
                            "soc_internal_mpls_entry_lkup: Not found\n")));

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
#endif /* BCM_TRIUMPH_SUPPORT */
