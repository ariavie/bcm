/*
 * $Id: hash.c,v 1.8 Broadcom SDK $
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
 * File:        hash.c
 * Purpose:     Trident2 hash table calculation routines
 * Requires:
 */

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/hash.h>
#include <shared/bsl.h>
#if defined(BCM_TRIDENT2_SUPPORT)

#include <soc/trident2.h>

/* Get number of banks for the hash table according to the config */
int
soc_trident2_hash_bank_count_get(int unit, soc_mem_t mem, int *num_banks)
{
    int count;

    switch (mem) {
    case L2Xm:
        /*
         * 16k entries in each of 2 dedicated L2 bank
         * 64k entries in each shared bank
         */
        count = soc_mem_index_count(unit, L2Xm);
        *num_banks = 2 + (count - 2 * 16 * 1024) / (64 * 1024);
        break;
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        /*
         * 4k entries in each of 4 dedicated L3 bank
         * 64k entries in each shared bank
         */
        count = soc_mem_index_count(unit, L3_ENTRY_ONLYm);
        *num_banks = 4 + (count - 4 * 4 * 1024) / (64 * 1024);
        break;
    case MPLS_ENTRYm:
    case VLAN_XLATEm:
    case VLAN_MACm:
    case EGR_VLAN_XLATEm:
    case ING_VP_VLAN_MEMBERSHIPm:
    case EGR_VP_VLAN_MEMBERSHIPm:
    case ING_DNAT_ADDRESS_TYPEm:
    case L2_ENDPOINT_IDm:
    case ENDPOINT_QUEUE_MAPm:
        *num_banks = 2;
        break;
    default:
        return SOC_E_INTERNAL;
    }

    return SOC_E_NONE;
}

/* Get bank bitmap for the hash table according to the config */
int
soc_trident2_hash_bank_bitmap_get(int unit, soc_mem_t mem, uint32 *bitmap)
{
    int count;

    SOC_IF_ERROR_RETURN(soc_trident2_hash_bank_count_get(unit, mem, &count));

    switch (mem) {
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        *bitmap = ((1 << count) - 1) << (10 - count);
        break;
    default:
        *bitmap = (1 << count) - 1;
        break;
    }

    return SOC_E_NONE;
}

/*
 * Map bank sequence number to bank number.
 *     Sequence number: 0 .. (number of bank - 1)
 *     bank number: bank id in hardware
 * This is only useful for L3 table which has non-sequential bank number,
 * sequence number equals to bank number for all other tables.
 */
int
soc_trident2_hash_bank_number_get(int unit, soc_mem_t mem, int seq_num,
                                  int *bank_num)
{
    int count;

    SOC_IF_ERROR_RETURN(soc_trident2_hash_bank_count_get(unit, mem, &count));
    if (seq_num < 0 || seq_num >= count) {
        return SOC_E_INTERNAL;
    }

    switch (mem) {
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        if (seq_num < 4) {
            *bank_num = 6 + seq_num;
        } else {
            *bank_num = 9 - seq_num;
        }
        break;
    default:
        *bank_num = seq_num;
        break;
    }

    return SOC_E_NONE;
}

/* 
 * Map bank bank number to sequence number.
 *     Sequence number: 0 .. (number of bank - 1)
 *     bank number: bank id in hardware
 * This is only useful for L3 table which has non-sequential bank number,
 *
 * L3 seq_num   bank_num
 *          0          6
 *          1          7
 *          2          8
 *          3          9
 *          4          5
 *          5          4 
 */

int
soc_trident2_hash_seq_number_get(int unit, soc_mem_t mem, int bank_num, 
                                 int *seq_num)
{
    int count;

    switch (mem) {
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        if (bank_num < 6) {
            *seq_num = 9 - bank_num;
        } else {
            *seq_num = bank_num - 6;
        }
        break;
    default:
        *seq_num = bank_num;
        break;        
    }

    SOC_IF_ERROR_RETURN(soc_trident2_hash_bank_count_get(unit, mem, &count));

    if (*seq_num < 0 || *seq_num >= count) {
        return SOC_E_INTERNAL;
    }

    return SOC_E_NONE;
    
}


int
soc_trident2_hash_bank_info_get(int unit, soc_mem_t mem, int bank,
                                int *entries_per_bank, int *entries_per_row,
                                int *entries_per_bucket, int *bank_base,
                                int *bucket_offset)
{
    int bank_size, row_size, bucket_size, base, offset;

    /*
     * entry index for bucket b, entry e =
     *     bank_base + b * entries_per_row + bucket_offset + e;
     */
    switch (mem) {
    case L2Xm:
        /*
         * 4 entries per bucket (1 bucket per row)
         * bank 0: index      0- 16383 (16k entries in dedicated bank)
         * bank 1: index  16384- 32767 (16k entries in dedicated bank)
         * bank 2: index  32768- 98303 (64k entries in shared bank)
         * bank 3: index  98304-163839 (64k entries in shared bank)
         * bank 4: index 163840-229375 (64k entries in shared bank)
         * bank 5: index 229376-294911 (64k entries in shared bank)
         */
        row_size = 4;
        bucket_size = 4;
        offset = 0;
        if (bank < 0 || bank > 5) {
            return SOC_E_INTERNAL;
        } else if (bank < 2) { /* 4k buckets for each dedicated bank */
            bank_size = 16384;
            base = bank * 16384;
        } else { /* 16k buckets for each shared bank */
            bank_size = 65536;
            base = 32768 + (bank - 2) * 65536;
        }
        break;
    case L3_ENTRY_ONLYm:
        /*
         * 4 entries per bucket (1 bucket per row)
         * bank 3: index 147456-212991 (64k entries in shared bank)
         * bank 4: index  81920-147455 (64k entries in shared bank)
         * bank 5: index  16384- 81919 (64k entries in shared bank)
         * bank 6: index      0-  4095 (4k entries in dedicated bank)
         * bank 7: index   4096-  8191 (4k entries in dedicated bank)
         * bank 8: index   8192- 12287 (4k entries in dedicated bank)
         * bank 9: index  12288- 16383 (4k entries in dedicated bank)
         */
        row_size = 4;
        bucket_size = 4;
        offset = 0;
        if (bank < 3 || bank > 9) {
            return SOC_E_INTERNAL;
        } else if (bank > 5) { /* 1k buckets for each dedicated bank */
            bank_size = 4096;
            base = (bank - 6) * 4096;
        } else { /* 16k buckets for each shared bank */
            bank_size = 65536;
            base = 16384 + (5 - bank) * 65536;
        }
        break;
    case MPLS_ENTRYm:
    case VLAN_XLATEm:
    case VLAN_MACm:
    case EGR_VLAN_XLATEm:
    case ING_VP_VLAN_MEMBERSHIPm:
    case EGR_VP_VLAN_MEMBERSHIPm:
    case ING_DNAT_ADDRESS_TYPEm:
    case L2_ENDPOINT_IDm:
    case ENDPOINT_QUEUE_MAPm:
        /*
         * first half of entries in each row are for bank 0
         * the other half of entries in each row are for bank 1
         */
        row_size = 8;
        bucket_size = 4;
        bank_size = soc_mem_index_count(unit, mem) / 2;
        base = 0;
        offset = bank * bucket_size;
        break;
    default:
        return SOC_E_INTERNAL;
    }

    if (entries_per_bank != NULL) {
        *entries_per_bank = bank_size;
    }
    if (entries_per_row != NULL) {
        *entries_per_row = row_size;
    }
    if (entries_per_bucket != NULL) {
        *entries_per_bucket = bucket_size;
    }
    if (bank_base != NULL) {
        *bank_base = base;
    }
    if (bucket_offset != NULL) {
        *bucket_offset = offset;
    }
    return SOC_E_NONE;
}

STATIC int
_soc_td2_hash_entry_to_key(int unit, void *entry, uint8 *key, soc_mem_t mem,
                           soc_field_t *field_list)
{
    soc_field_t field;
    int         index, key_index, val_index, fval_index;
    int         right_shift_count, left_shift_count;
    uint32      val[SOC_MAX_MEM_WORDS], fval[SOC_MAX_MEM_WORDS];
    int         bits, val_bits, fval_bits;
    int8        field_length[16];

    val_bits = 0;
    for (index = 0; field_list[index] != INVALIDf; index++) {
        field = field_list[index];
        field_length[index] = soc_mem_field_length(unit, mem, field);
        val_bits += field_length[index];
    }

    switch (mem) {
    case L2Xm:
        val_bits = soc_mem_field_length(unit, L2Xm,
                                        TRILL_NONUC_NETWORK_LONG__KEYf);
        break;
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
        val_bits = soc_mem_field_length(unit, L3_ENTRY_IPV6_MULTICASTm,
                                        IPV6MC__KEY_0f) +
            soc_mem_field_length(unit, L3_ENTRY_IPV6_MULTICASTm,
                                 IPV6MC__KEY_1f) +
            soc_mem_field_length(unit, L3_ENTRY_IPV6_MULTICASTm,
                                 IPV6MC__KEY_2f) +
            soc_mem_field_length(unit, L3_ENTRY_IPV6_MULTICASTm,
                                 IPV6MC__KEY_3f);
        break;
    case VLAN_XLATEm:
    case VLAN_MACm:
        val_bits = soc_mem_field_length(unit, VLAN_MACm, MAC_PORT__KEYf);
        break;
    case EGR_VLAN_XLATEm:
        val_bits = soc_mem_field_length(unit, EGR_VLAN_XLATEm, XLATE__KEYf);
        break;
    case MPLS_ENTRYm:
        val_bits = soc_mem_field_length(unit, MPLS_ENTRYm, L2GRE_VPNID__KEYf);
        break;
    case L2_ENDPOINT_IDm:
        val_bits = soc_mem_field_length(unit, L2_ENDPOINT_IDm, L2__KEYf);
        break;
    default:
        break;
    }

    bits = (val_bits + 7) & ~0x7;
    sal_memset(val, 0, sizeof(val));
    val_bits = bits - val_bits;
    for (index = 0; field_list[index] != INVALIDf; index++) {
        field = field_list[index];
        soc_mem_field_get(unit, mem, entry, field, fval);
        fval_bits = field_length[index];

        val_index = val_bits >> 5;
        fval_index = 0;
        left_shift_count = val_bits & 0x1f;
        right_shift_count = 32 - left_shift_count;
        val_bits += fval_bits;

        if (left_shift_count) {
            for (; fval_bits > 0; fval_bits -= 32) {
                val[val_index++] |= fval[fval_index] << left_shift_count;
                val[val_index] |= fval[fval_index++] >> right_shift_count;
            }
        } else {
            for (; fval_bits > 0; fval_bits -= 32) {
                val[val_index++] = fval[fval_index++];
            }
        }
    }

    key_index = 0;
    for (val_index = 0; val_bits > 0; val_index++) {
        for (right_shift_count = 0; right_shift_count < 32;
             right_shift_count += 8) {
            if (val_bits <= 0) {
                break;
            }
            key[key_index++] = (val[val_index] >> right_shift_count) & 0xff;
            val_bits -= 8;
        }
    }

    if ((bits + 7) / 8 > key_index) {
        sal_memset(&key[key_index], 0, (bits + 7) / 8 - key_index);
    }

    return bits;
}

/* For non-UFT style hash (all tables except L2 and L3) */
int
soc_td2_hash_sel_get(int unit, soc_mem_t mem, int bank, int *hash_sel)
{
    soc_reg_t reg;
    soc_field_t field;
    uint32 rval;

    field = bank > 0 ? HASH_SELECT_Bf : HASH_SELECT_Af;
    switch (mem) {
    case VLAN_XLATEm:
    case VLAN_MACm:
        reg = VLAN_XLATE_HASH_CONTROLr;
        break;
    case EGR_VLAN_XLATEm:
        reg = EGR_VLAN_XLATE_HASH_CONTROLr;
        break;
    case MPLS_ENTRYm:
        reg = MPLS_ENTRY_HASH_CONTROLr;
        break;
    case ING_VP_VLAN_MEMBERSHIPm:
        reg = ING_VP_VLAN_MEMBERSHIP_HASH_CONTROLr;
        break;
    case EGR_VP_VLAN_MEMBERSHIPm:
        reg = EGR_VP_VLAN_MEMBERSHIP_HASH_CONTROLr;
        break;
    case ING_DNAT_ADDRESS_TYPEm:
        reg = ING_DNAT_ADDRESS_TYPE_HASH_CONTROLr;
        break;
    case L2_ENDPOINT_IDm:
        reg = L2_ENDPOINT_ID_HASH_CONTROLr;
        break;
    case ENDPOINT_QUEUE_MAPm:
        reg = ENDPOINT_QUEUE_MAP_HASH_CONTROLr;
        break;
    default:
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &rval));
    *hash_sel = soc_reg_field_get(unit, reg, rval, field);

    return SOC_E_NONE;
}

/* For UFT style hash (L2 and L3 table) */
int
soc_td2_hash_offset_get(int unit, soc_mem_t mem, int bank, int *hash_offset,
                        int *use_lsb)
{
    uint32 rval;
    soc_reg_t reg;
    static const soc_field_t fields[] = {
        BANK0_HASH_OFFSETf, BANK1_HASH_OFFSETf, BANK2_HASH_OFFSETf,
        BANK3_HASH_OFFSETf, BANK4_HASH_OFFSETf, BANK5_HASH_OFFSETf,
        BANK6_HASH_OFFSETf, BANK7_HASH_OFFSETf, BANK8_HASH_OFFSETf,
        BANK9_HASH_OFFSETf
    };

    if (mem == L2Xm) {
        if (bank < 0 || bank > 5) {
            return SOC_E_PARAM;
        }
        reg = L2_TABLE_HASH_CONTROLr;
    } else if (mem == L3_ENTRY_ONLYm) {
        if (bank < 3 || bank > 9) {
            return SOC_E_PARAM;
        }
        reg = L3_TABLE_HASH_CONTROLr;
    } else {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &rval));
    *use_lsb = soc_reg_field_get(unit, reg, rval, HASH_ZERO_OR_LSBf);

    if (bank > 1 && bank < 6) {
        reg = SHARED_TABLE_HASH_CONTROLr;
        SOC_IF_ERROR_RETURN(soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &rval));
    }

    *hash_offset = soc_reg_field_get(unit, reg, rval, fields[bank]);

    return SOC_E_NONE;
}

int
soc_td2_hash_offset_set(int unit, soc_mem_t mem, int bank, int hash_offset,
                        int use_lsb)
{
    uint32 rval;
    soc_reg_t reg;
    static const soc_field_t fields[] = {
        BANK0_HASH_OFFSETf, BANK1_HASH_OFFSETf, BANK2_HASH_OFFSETf,
        BANK3_HASH_OFFSETf, BANK4_HASH_OFFSETf, BANK5_HASH_OFFSETf,
        BANK6_HASH_OFFSETf, BANK7_HASH_OFFSETf, BANK8_HASH_OFFSETf,
        BANK9_HASH_OFFSETf
    };

    if (mem == L2Xm) {
        if (bank < 0 || bank > 5) {
            return SOC_E_PARAM;
        }
        reg = L2_TABLE_HASH_CONTROLr;
    } else if (mem == L3_ENTRY_ONLYm) {
        if (bank < 3 || bank > 9) {
            return SOC_E_PARAM;
        }
        reg = L3_TABLE_HASH_CONTROLr;
    } else {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &rval));
    soc_reg_field_set(unit, reg, &rval, HASH_ZERO_OR_LSBf, use_lsb);
    if (bank < 2 || bank > 5) {
        soc_reg_field_set(unit, reg, &rval, fields[bank], hash_offset);
    }
    SOC_IF_ERROR_RETURN(soc_reg32_set(unit, reg, REG_PORT_ANY, 0, rval));

    if (bank > 1 && bank < 6) {
        reg = SHARED_TABLE_HASH_CONTROLr;
        SOC_IF_ERROR_RETURN(soc_reg32_get(unit, reg, REG_PORT_ANY, 0, &rval));
        soc_reg_field_set(unit, reg, &rval, fields[bank], hash_offset);
        SOC_IF_ERROR_RETURN(soc_reg32_set(unit, reg, REG_PORT_ANY, 0, rval));
    }    
    return SOC_E_NONE;
}

STATIC int
_soc_td2_shared_hash(int unit, int hash_offset, uint32 key_nbits, uint8 *key,
                     uint32 result_mask, uint16 lsb)
{
    uint32 crc_hi, crc_lo;

    if (hash_offset >= 48) {
        if (hash_offset > 48) {
            lsb >>= hash_offset - 48;
        }
        return lsb & result_mask;
    }

    crc_hi = soc_crc16b(key, key_nbits) | ((uint32)lsb << 16);
    if (hash_offset >= 32) {
        if (hash_offset > 32) {
            crc_hi >>= hash_offset - 32;
        }
        return crc_hi & result_mask;
    }

    crc_lo = soc_crc32b(key, key_nbits);
    if (hash_offset > 0) {
        crc_lo >>= hash_offset;
        crc_lo |= crc_hi << (32 - hash_offset);
    }
    return crc_lo & result_mask;
}

uint32
soc_td2_l2x_hash(int unit, int bank, int hash_offset, int use_lsb,
                 int key_nbits, void *base_entry, uint8 *key)
{
    uint32 lsb_val;
    uint32 hash_mask;
    uint32 hash_bits;

    if (bank < 2) {
        hash_mask = 0x0fff; /* 4k buckets per dedicated L2 bank */
        hash_bits = 12;
    } else {
        hash_mask = 0x3fff; /* 16k buckets per shared bank */
        hash_bits = 14;
    }

    if (use_lsb && (hash_offset + hash_bits > 48)) {
        switch (soc_mem_field32_get(unit, L2Xm, base_entry, KEY_TYPEf)) {
        case TD2_L2_HASH_KEY_TYPE_BRIDGE:
        case TD2_L2_HASH_KEY_TYPE_VFI:
        case TD2_L2_HASH_KEY_TYPE_TRILL_NONUC_ACCESS:
            lsb_val = soc_mem_field32_get(unit, L2Xm, base_entry,
                                          L2__HASH_LSBf);
            break;
        case TD2_L2_HASH_KEY_TYPE_SINGLE_CROSS_CONNECT:
        case TD2_L2_HASH_KEY_TYPE_DOUBLE_CROSS_CONNECT:
            lsb_val = soc_mem_field32_get(unit, L2Xm, base_entry,
                                          VLAN__HASH_LSBf);
            break;
        case TD2_L2_HASH_KEY_TYPE_VIF:
            lsb_val = soc_mem_field32_get(unit, L2Xm, base_entry,
                                          VIF__HASH_LSBf);
            break;
        case TD2_L2_HASH_KEY_TYPE_TRILL_NONUC_NETWORK_LONG:
            lsb_val = soc_mem_field32_get(unit, L2Xm, base_entry,
                                          TRILL_NONUC_NETWORK_LONG__HASH_LSBf);
            break;
        case TD2_L2_HASH_KEY_TYPE_TRILL_NONUC_NETWORK_SHORT:
            lsb_val = soc_mem_field32_get
                (unit, L2Xm, base_entry, TRILL_NONUC_NETWORK_SHORT__HASH_LSBf);
            break;
        case TD2_L2_HASH_KEY_TYPE_BFD:
            lsb_val = soc_mem_field32_get(unit, L2Xm, base_entry,
                                          BFD__HASH_LSBf);
            break;
        case TD2_L2_HASH_KEY_TYPE_PE_VID:
            lsb_val = soc_mem_field32_get(unit, L2Xm, base_entry,
                                          PE_VID__HASH_LSBf);
            break;
        case TD2_L2_HASH_KEY_TYPE_FCOE_ZONE:
            lsb_val = soc_mem_field32_get(unit, L2Xm, base_entry,
                                          FCOE_ZONE__HASH_LSBf);
            break;
        default:
            lsb_val = 0;
            break;
        }
    } else {
        lsb_val = 0;
    }

    return _soc_td2_shared_hash(unit, hash_offset, key_nbits, key, hash_mask,
                                lsb_val);
}

int
soc_td2_l2x_base_entry_to_key(int unit, uint32 *entry, uint8 *key)
{
    soc_field_t field_list[2];

    switch (soc_mem_field32_get(unit, L2Xm, entry, KEY_TYPEf)) {
    case TD2_L2_HASH_KEY_TYPE_BRIDGE:
    case TD2_L2_HASH_KEY_TYPE_VFI:
    case TD2_L2_HASH_KEY_TYPE_TRILL_NONUC_ACCESS:
        field_list[0] = L2__KEYf;
        break;
    case TD2_L2_HASH_KEY_TYPE_SINGLE_CROSS_CONNECT:
    case TD2_L2_HASH_KEY_TYPE_DOUBLE_CROSS_CONNECT:
        field_list[0] = VLAN__KEYf;
        break;
    case TD2_L2_HASH_KEY_TYPE_VIF:
        field_list[0] = VIF__KEYf;
        break;
    case TD2_L2_HASH_KEY_TYPE_TRILL_NONUC_NETWORK_LONG:
        field_list[0] = TRILL_NONUC_NETWORK_LONG__KEYf;
        break;
    case TD2_L2_HASH_KEY_TYPE_TRILL_NONUC_NETWORK_SHORT:
        field_list[0] = TRILL_NONUC_NETWORK_SHORT__KEYf;
        break;
    case TD2_L2_HASH_KEY_TYPE_BFD:
        field_list[0] = BFD__KEYf;
        break;
    case TD2_L2_HASH_KEY_TYPE_PE_VID:
        field_list[0] = PE_VID__KEYf;
        break;
    case TD2_L2_HASH_KEY_TYPE_FCOE_ZONE:
        field_list[0] = FCOE_ZONE__KEYf;
        break;
    default:
        return 0;
    }
    field_list[1] = INVALIDf;
    return _soc_td2_hash_entry_to_key(unit, entry, key, L2Xm, field_list);
}

uint32
soc_td2_l2x_entry_hash(int unit, int bank, int hash_offset, int use_lsb,
                       uint32 *entry)
{
    int             key_nbits;
    uint8           key[SOC_MAX_MEM_WORDS * 4];

    key_nbits = soc_td2_l2x_base_entry_to_key(unit, entry, key);
    return soc_td2_l2x_hash(unit, bank, hash_offset, use_lsb, key_nbits, entry,
                            key);
}

uint32
soc_td2_l2x_bank_entry_hash(int unit, int bank, uint32 *entry)
{
    int rv, hash_offset, use_lsb;

    rv = soc_td2_hash_offset_get(unit, L2Xm, bank, &hash_offset, &use_lsb);
    if (SOC_FAILURE(rv)) {
        return 0;
    }

    return soc_td2_l2x_entry_hash(unit, bank, hash_offset, use_lsb, entry);
}

uint32
soc_td2_l3x_hash(int unit, int bank, int hash_offset, int use_lsb,
                 int key_nbits, void *base_entry, uint8 *key)
{
    uint32 lsb_val;
    uint32 hash_mask;

    if (bank > 5) {
        hash_mask = 0x3ff;  /* 1k buckets per dedicated L3 bank */
    } else {
        hash_mask = 0x3fff; /* 16k buckets per shared bank */
    }

    if (use_lsb && (hash_offset + SOC_CONTROL(unit)->hash_bits_l3x > 48)) {
        switch (soc_mem_field32_get(unit, L3_ENTRY_ONLYm, base_entry,
                                    KEY_TYPEf)) {
        case TD2_L3_HASH_KEY_TYPE_V4UC:
        case TD2_L3_HASH_KEY_TYPE_V4UC_EXT:
        case TD2_L3_HASH_KEY_TYPE_V4MC:
        case TD2_L3_HASH_KEY_TYPE_V6UC:
        case TD2_L3_HASH_KEY_TYPE_V6UC_EXT:
        case TD2_L3_HASH_KEY_TYPE_V6MC:
            lsb_val = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm,
                                          base_entry, IPV4UC__HASH_LSBf);
            break;
        case TD2_L3_HASH_KEY_TYPE_TRILL:
            lsb_val = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm,
                                          base_entry, TRILL__HASH_LSBf);
            break;
        case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN:
        case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN_EXT:
        case TD2_L3_HASH_KEY_TYPE_FCOE_HOST:
        case TD2_L3_HASH_KEY_TYPE_FCOE_HOST_EXT:
        case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP:
        case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP_EXT:
            lsb_val = soc_mem_field32_get(unit, L3_ENTRY_IPV4_UNICASTm,
                                          base_entry, FCOE__HASH_LSBf);
            break;
        case TD2_L3_HASH_KEY_TYPE_DST_NAT:
        case TD2_L3_HASH_KEY_TYPE_DST_NAPT:
            lsb_val = soc_mem_field32_get(unit, L3_ENTRY_IPV4_MULTICASTm,
                                          base_entry, NAT__HASH_LSBf);
            break;
        default:
            lsb_val = 0;
            break;
        }
    } else {
        lsb_val = 0;
    }

    return _soc_td2_shared_hash(unit, hash_offset, key_nbits, key, hash_mask,
                                lsb_val);
}

int
soc_td2_l3x_base_entry_to_key(int unit, uint32 *entry, uint8 *key)
{
    soc_mem_t mem;
    soc_field_t field_list[5];
    uint32 sanitized_entry[SOC_MAX_MEM_WORDS];
    void *ptr;

    ptr = entry;

    switch (soc_mem_field32_get(unit, L3_ENTRY_ONLYm, entry, KEY_TYPEf)) {
    case TD2_L3_HASH_KEY_TYPE_V4UC_EXT:
        sal_memcpy(&sanitized_entry, entry, sizeof(sanitized_entry));
        ptr = &sanitized_entry;
        soc_mem_field32_set(unit, L3_ENTRY_IPV4_UNICASTm, ptr, KEY_TYPEf,
                            TD2_L3_HASH_KEY_TYPE_V4UC);
        /* fall through */
    case TD2_L3_HASH_KEY_TYPE_V4UC:
        mem = L3_ENTRY_IPV4_UNICASTm;
        field_list[0] = IPV4UC__KEYf;
        field_list[1] = INVALIDf;
        break;
    case TD2_L3_HASH_KEY_TYPE_V6UC_EXT:
        sal_memcpy(&sanitized_entry, entry, sizeof(sanitized_entry));
        ptr = &sanitized_entry;
        soc_mem_field32_set(unit, L3_ENTRY_IPV6_UNICASTm, ptr, KEY_TYPE_0f,
                            TD2_L3_HASH_KEY_TYPE_V6UC);
        /* fall through */
    case TD2_L3_HASH_KEY_TYPE_V6UC:
        mem = L3_ENTRY_IPV6_UNICASTm;
        field_list[0] = IPV6UC__KEY_0f;
        field_list[1] = IPV6UC__KEY_1f;
        field_list[2] = INVALIDf;
        break;
    case TD2_L3_HASH_KEY_TYPE_V4MC:
        mem = L3_ENTRY_IPV4_MULTICASTm;
        field_list[0] = IPV4MC__KEY_0f;
        field_list[1] = IPV4MC__KEY_1f;
        field_list[2] = INVALIDf;
        break;
    case TD2_L3_HASH_KEY_TYPE_V6MC:
        mem = L3_ENTRY_IPV6_MULTICASTm;
        field_list[0] = IPV6MC__KEY_0f;
        field_list[1] = IPV6MC__KEY_1f;
        field_list[2] = IPV6MC__KEY_2f;
        field_list[3] = IPV6MC__KEY_3f;
        field_list[4] = INVALIDf;
        break;
    case TD2_L3_HASH_KEY_TYPE_TRILL:
        mem = L3_ENTRY_IPV4_UNICASTm;
        field_list[0] = TRILL__KEYf;
        field_list[1] = INVALIDf;
        break;
    case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN_EXT:
        sal_memcpy(&sanitized_entry, entry, sizeof(sanitized_entry));
        ptr = &sanitized_entry;
        soc_mem_field32_set(unit, L3_ENTRY_IPV4_UNICASTm, ptr, KEY_TYPEf,
                            TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN);
        /* fall through */
    case TD2_L3_HASH_KEY_TYPE_FCOE_DOMAIN:
        mem = L3_ENTRY_IPV4_UNICASTm;
        field_list[0] = FCOE__KEYf;
        field_list[1] = INVALIDf;
        break;
    case TD2_L3_HASH_KEY_TYPE_FCOE_HOST_EXT:
        sal_memcpy(&sanitized_entry, entry, sizeof(sanitized_entry));
        ptr = &sanitized_entry;
        soc_mem_field32_set(unit, L3_ENTRY_IPV4_UNICASTm, ptr, KEY_TYPEf,
                            TD2_L3_HASH_KEY_TYPE_FCOE_HOST);
        /* fall through */
    case TD2_L3_HASH_KEY_TYPE_FCOE_HOST:
        mem = L3_ENTRY_IPV4_UNICASTm;
        field_list[0] = FCOE__KEYf;
        field_list[1] = INVALIDf;
        break;
    case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP_EXT:
        sal_memcpy(&sanitized_entry, entry, sizeof(sanitized_entry));
        ptr = &sanitized_entry;
        soc_mem_field32_set(unit, L3_ENTRY_IPV4_UNICASTm, ptr, KEY_TYPEf,
                            TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP);
        /* fall through */
    case TD2_L3_HASH_KEY_TYPE_FCOE_SRC_MAP:
        mem = L3_ENTRY_IPV4_UNICASTm;
        field_list[0] = FCOE__KEYf;
        field_list[1] = INVALIDf;
        break;

    case TD2_L3_HASH_KEY_TYPE_DST_NAT:
    case TD2_L3_HASH_KEY_TYPE_DST_NAPT:
        mem = L3_ENTRY_IPV4_MULTICASTm;
        field_list[0] = NAT__KEY_0f;
        field_list[1] = INVALIDf;
        break;
    default:
        return 0;
    }
    return _soc_td2_hash_entry_to_key(unit, ptr, key, mem, field_list);
}

uint32
soc_td2_l3x_entry_hash(int unit, int bank, int hash_offset, int use_lsb,
                       uint32 *entry)
{
    int             key_nbits;
    uint8           key[SOC_MAX_MEM_WORDS * 4];

    key_nbits = soc_td2_l3x_base_entry_to_key(unit, entry, key);
    return soc_td2_l3x_hash(unit, bank, hash_offset, use_lsb, key_nbits,
                            entry, key);
}

uint32
soc_td2_l3x_bank_entry_hash(int unit, int bank, uint32 *entry)
{
    int rv, hash_offset, use_lsb;

    rv = soc_td2_hash_offset_get(unit, L3_ENTRY_ONLYm, bank, &hash_offset,
                                 &use_lsb);
    if (SOC_FAILURE(rv)) {
        return 0;
    }

    return soc_td2_l3x_entry_hash(unit, bank, hash_offset, use_lsb, entry);
}

uint32
soc_td2_mpls_hash(int unit, int hash_sel, int key_nbits, void *base_entry,
                  uint8 *key)
{
    uint32 rv = 0;

    /*
     * Cache bucket mask and shift amount for upper crc
     */
    if (SOC_CONTROL(unit)->hash_mask_mpls == 0) {
        uint32  mask;
        int     bits;

        /* 8 Entries per bucket */
        mask = soc_mem_index_max(unit, MPLS_ENTRYm) >> 3;
        bits = 0;
        rv = 1;
        while (rv && (mask & rv)) {
            bits += 1;
            rv <<= 1;
        }
        SOC_CONTROL(unit)->hash_mask_mpls = mask;
        SOC_CONTROL(unit)->hash_bits_mpls = bits;
    }

    switch (hash_sel) {
    case FB_HASH_CRC16_UPPER:
        rv = soc_crc16b(key, key_nbits);
        rv >>= 16 - SOC_CONTROL(unit)->hash_bits_mpls;
        break;

    case FB_HASH_CRC16_LOWER:
        rv = soc_crc16b(key, key_nbits);
        break;

    case FB_HASH_LSB:
        if (key_nbits == 0) {
            return 0;
        }
        switch (soc_mem_field32_get(unit, MPLS_ENTRYm, base_entry,
                                    KEY_TYPEf)) {
        case TD2_MPLS_HASH_KEY_TYPE_MPLS:
            rv = soc_mem_field32_get(unit, MPLS_ENTRYm, base_entry,
                                     MPLS__HASH_LSBf);
            break;
        case TD2_MPLS_HASH_KEY_TYPE_MIM_NVP:
            rv = soc_mem_field32_get(unit, MPLS_ENTRYm, base_entry,
                                     MIM_NVP__HASH_LSBf);
            break;
        case TD2_MPLS_HASH_KEY_TYPE_MIM_ISID:
        case TD2_MPLS_HASH_KEY_TYPE_MIM_ISID_SVP:
            rv = soc_mem_field32_get(unit, MPLS_ENTRYm, base_entry,
                                     MIM_ISID__HASH_LSBf);
            break;
        case TD2_MPLS_HASH_KEY_TYPE_TRILL:
            rv = soc_mem_field32_get(unit, MPLS_ENTRYm, base_entry,
                                     TRILL__HASH_LSBf);
            break;
        case TD2_MPLS_HASH_KEY_TYPE_L2GRE_SIP:
            rv = soc_mem_field32_get(unit, MPLS_ENTRYm, base_entry,
                                     L2GRE_SIP__HASH_LSBf);
        break;
        case TD2_MPLS_HASH_KEY_TYPE_L2GRE_VPNID_SIP:
        case TD2_MPLS_HASH_KEY_TYPE_L2GRE_VPNID:
            rv = soc_mem_field32_get(unit, MPLS_ENTRYm, base_entry,
                                     L2GRE_VPNID__HASH_LSBf);
            break;
        case TD2_MPLS_HASH_KEY_TYPE_VXLAN_SIP:
            rv = soc_mem_field32_get(unit, MPLS_ENTRYm, base_entry,
                                     VXLAN_SIP__HASH_LSBf);
            break;
        case TD2_MPLS_HASH_KEY_TYPE_VXLAN_VPNID:
        case TD2_MPLS_HASH_KEY_TYPE_VXLAN_VPNID_SIP:
            rv = soc_mem_field32_get(unit, MPLS_ENTRYm, base_entry,
                                     VXLAN_VN_ID__HASH_LSBf);
            break;
        default:
            rv = 0;
            break;
        }
        break;

    case FB_HASH_ZERO:
        rv = 0;
        break;

    case FB_HASH_CRC32_UPPER:
        rv = soc_crc32b(key, key_nbits);
        rv >>= 32 - SOC_CONTROL(unit)->hash_bits_mpls;
        break;

    case FB_HASH_CRC32_LOWER:
        rv = soc_crc32b(key, key_nbits);
        break;

    default:
        LOG_ERROR(BSL_LS_SOC_HASH,
                  (BSL_META_U(unit,
                              "soc_td2_mpls_hash: invalid hash_sel %d\n"),
                   hash_sel));
        rv = 0;
        break;
    }

    return rv & SOC_CONTROL(unit)->hash_mask_mpls;
}

int
soc_td2_mpls_base_entry_to_key(int unit, void *entry, uint8 *key)
{
    soc_field_t field_list[2];

    switch (soc_mem_field32_get(unit, MPLS_ENTRYm, entry, KEY_TYPEf)) {
    case TD2_MPLS_HASH_KEY_TYPE_MPLS:
        field_list[0] = MPLS__KEYf;
        break;
    case TD2_MPLS_HASH_KEY_TYPE_MIM_NVP:
        field_list[0] = MIM_NVP__KEYf;
        break;
    case TD2_MPLS_HASH_KEY_TYPE_MIM_ISID:
    case TD2_MPLS_HASH_KEY_TYPE_MIM_ISID_SVP:
        field_list[0] = MIM_ISID__KEYf;
        break;
    case TD2_MPLS_HASH_KEY_TYPE_TRILL:
        field_list[0] = TRILL__KEYf;
        break;
    case TD2_MPLS_HASH_KEY_TYPE_L2GRE_SIP:
        field_list[0] = L2GRE_SIP__KEYf;
        break;
    case TD2_MPLS_HASH_KEY_TYPE_L2GRE_VPNID_SIP:
    case TD2_MPLS_HASH_KEY_TYPE_L2GRE_VPNID:
        field_list[0] = L2GRE_VPNID__KEYf;
        break;
    case TD2_MPLS_HASH_KEY_TYPE_VXLAN_SIP:
        field_list[0] = VXLAN_SIP__KEYf;
        break;
    case TD2_MPLS_HASH_KEY_TYPE_VXLAN_VPNID:
    case TD2_MPLS_HASH_KEY_TYPE_VXLAN_VPNID_SIP:
        field_list[0] = VXLAN_VN_ID__KEYf;
        break;
    default:
        return 0;
    }
    field_list[1] = INVALIDf;
    return _soc_td2_hash_entry_to_key(unit, entry, key, MPLS_ENTRYm,
                                      field_list);
}

uint32
soc_td2_mpls_entry_hash(int unit, int hash_sel, uint32 *entry)
{
    int             key_nbits;
    uint8           key[SOC_MAX_MEM_WORDS * 4];
    uint32          index;

    key_nbits = soc_td2_mpls_base_entry_to_key(unit, entry, key);
    index = soc_td2_mpls_hash(unit, hash_sel, key_nbits, entry, key);

    return index;
}

uint32
soc_td2_mpls_bank_entry_hash(int unit, int bank, uint32 *entry)
{
    int rv, hash_sel;

    rv = soc_td2_hash_sel_get(unit, MPLS_ENTRYm, bank, &hash_sel);
    if (SOC_FAILURE(rv)) {
        return 0;
    }

    return soc_td2_mpls_entry_hash(unit, hash_sel, entry);
}

uint32
soc_td2_vlan_xlate_hash(int unit, int hash_sel, int key_nbits,
                        void *base_entry, uint8 *key)
{
    uint32 rv = 0;

    /*
     * Cache bucket mask and shift amount for upper crc
     */
    if (SOC_CONTROL(unit)->hash_mask_vlan_mac == 0) {
        uint32  mask;
        int     bits;

        /* 8 Entries per bucket */
        mask = soc_mem_index_max(unit, VLAN_MACm) >> 3;
        bits = 0;
        rv = 1;
        while (rv && (mask & rv)) {
            bits += 1;
            rv <<= 1;
        }
        SOC_CONTROL(unit)->hash_mask_vlan_mac = mask;
        SOC_CONTROL(unit)->hash_bits_vlan_mac = bits;
    }

    switch (hash_sel) {
    case FB_HASH_CRC16_UPPER:
        rv = soc_crc16b(key, key_nbits);
        rv >>= 16 - SOC_CONTROL(unit)->hash_bits_vlan_mac;
        break;

    case FB_HASH_CRC16_LOWER:
        rv = soc_crc16b(key, key_nbits);
        break;

    case FB_HASH_LSB:
        if (key_nbits == 0) {
            return 0;
        }
        switch (soc_mem_field32_get(unit, VLAN_XLATEm, base_entry,
                                    KEY_TYPEf)) {
        case TD2_VLXLT_HASH_KEY_TYPE_IVID_OVID:
        case TD2_VLXLT_HASH_KEY_TYPE_OTAG:
        case TD2_VLXLT_HASH_KEY_TYPE_ITAG:
        case TD2_VLXLT_HASH_KEY_TYPE_OVID:
        case TD2_VLXLT_HASH_KEY_TYPE_IVID:
        case TD2_VLXLT_HASH_KEY_TYPE_PRI_CFI:
        case TD2_VLXLT_HASH_KEY_TYPE_IVID_OVID_VSAN:
        case TD2_VLXLT_HASH_KEY_TYPE_IVID_VSAN:
        case TD2_VLXLT_HASH_KEY_TYPE_OVID_VSAN:
            rv = soc_mem_field32_get(unit, VLAN_XLATEm, base_entry,
                                     XLATE__HASH_LSBf);
            break;
        case TD2_VLXLT_HASH_KEY_TYPE_VLAN_MAC:
            rv = soc_mem_field32_get(unit, VLAN_MACm, base_entry,
                                     MAC__HASH_LSBf);
            break;
        case TD2_VLXLT_HASH_KEY_TYPE_HPAE:
            rv = soc_mem_field32_get(unit, VLAN_MACm, base_entry,
                                     MAC_IP_BIND__HASH_LSBf);
            break;
        case TD2_VLXLT_HASH_KEY_TYPE_VIF:
        case TD2_VLXLT_HASH_KEY_TYPE_VIF_VLAN:
        case TD2_VLXLT_HASH_KEY_TYPE_VIF_CVLAN:
        case TD2_VLXLT_HASH_KEY_TYPE_VIF_OTAG:
        case TD2_VLXLT_HASH_KEY_TYPE_VIF_ITAG:
            rv = soc_mem_field32_get(unit, VLAN_XLATEm, base_entry,
                                     VIF__HASH_LSBf);
            break;
        case TD2_VLXLT_HASH_KEY_TYPE_VLAN_MAC_PORT:
            rv = soc_mem_field32_get(unit, VLAN_MACm, base_entry,
                                     MAC_PORT__HASH_LSBf);
            break;
        case TD2_VLXLT_HASH_KEY_TYPE_L2GRE_DIP:
            rv = soc_mem_field32_get(unit, VLAN_XLATEm, base_entry,
                                     L2GRE_DIP__HASH_LSBf);
            break;
        case TD2_VLXLT_HASH_KEY_TYPE_VXLAN_DIP:
            rv = soc_mem_field32_get(unit, VLAN_XLATEm, base_entry,
                                     VXLAN_DIP__HASH_LSBf);
            break;
        default:
            rv = 0;
            break;
        }
        break;

    case FB_HASH_ZERO:
        rv = 0;
        break;

    case FB_HASH_CRC32_UPPER:
        rv = soc_crc32b(key, key_nbits);
        rv >>= 32 - SOC_CONTROL(unit)->hash_bits_vlan_mac;
        break;

    case FB_HASH_CRC32_LOWER:
        rv = soc_crc32b(key, key_nbits);
        break;

    default:
        LOG_ERROR(BSL_LS_SOC_HASH,
                  (BSL_META_U(unit,
                              "soc_td2_vlan_xlate_hash: invalid hash_sel %d\n"),
                   hash_sel));
        rv = 0;
        break;
    }

    return rv & SOC_CONTROL(unit)->hash_mask_vlan_mac;
}

int
soc_td2_vlan_xlate_base_entry_to_key(int unit, void *entry, uint8 *key)
{
    soc_mem_t mem;
    soc_field_t field_list[2];

    switch (soc_mem_field32_get(unit, VLAN_XLATEm, entry, KEY_TYPEf)) {
    case TD2_VLXLT_HASH_KEY_TYPE_IVID_OVID:
    case TD2_VLXLT_HASH_KEY_TYPE_OTAG:
    case TD2_VLXLT_HASH_KEY_TYPE_ITAG:
    case TD2_VLXLT_HASH_KEY_TYPE_OVID:
    case TD2_VLXLT_HASH_KEY_TYPE_IVID:
    case TD2_VLXLT_HASH_KEY_TYPE_PRI_CFI:
    case TD2_VLXLT_HASH_KEY_TYPE_IVID_OVID_VSAN:
    case TD2_VLXLT_HASH_KEY_TYPE_IVID_VSAN:
    case TD2_VLXLT_HASH_KEY_TYPE_OVID_VSAN:
        mem = VLAN_XLATEm;
        field_list[0] = XLATE__KEYf;
        break;
    case TD2_VLXLT_HASH_KEY_TYPE_VLAN_MAC:
        mem = VLAN_MACm;
        field_list[0] = MAC__KEYf;
        break;
    case TD2_VLXLT_HASH_KEY_TYPE_HPAE:
        mem = VLAN_MACm;
        field_list[0] = MAC_IP_BIND__KEYf;
        break;
    case TD2_VLXLT_HASH_KEY_TYPE_VIF:
    case TD2_VLXLT_HASH_KEY_TYPE_VIF_VLAN:
    case TD2_VLXLT_HASH_KEY_TYPE_VIF_CVLAN:
    case TD2_VLXLT_HASH_KEY_TYPE_VIF_OTAG:
    case TD2_VLXLT_HASH_KEY_TYPE_VIF_ITAG:
        mem = VLAN_XLATEm;
        field_list[0] = VIF__KEYf;
        break;
    case TD2_VLXLT_HASH_KEY_TYPE_VLAN_MAC_PORT:
        mem = VLAN_MACm;
        field_list[0] = MAC_PORT__KEYf;
        break;
    case TD2_VLXLT_HASH_KEY_TYPE_L2GRE_DIP:
        mem = VLAN_XLATEm;
        field_list[0] = L2GRE_DIP__KEYf;
        break;
    case TD2_VLXLT_HASH_KEY_TYPE_VXLAN_DIP:
        mem = VLAN_XLATEm;
        field_list[0] = VXLAN_DIP__KEYf;
        break;
    default:
        return 0;
    }
    field_list[1] = INVALIDf;
    return _soc_td2_hash_entry_to_key(unit, entry, key, mem, field_list);
}

uint32
soc_td2_vlan_xlate_entry_hash(int unit, int hash_sel, uint32 *entry)
{
    int             key_nbits;
    uint8           key[SOC_MAX_MEM_WORDS * 4];
    uint32          index;

    key_nbits = soc_td2_vlan_xlate_base_entry_to_key(unit, entry, key);
    index = soc_td2_vlan_xlate_hash(unit, hash_sel, key_nbits, entry, key);

    return index;
}

uint32
soc_td2_vlan_xlate_bank_entry_hash(int unit, int bank, uint32 *entry)
{
    int rv, hash_sel;

    rv = soc_td2_hash_sel_get(unit, VLAN_XLATEm, bank, &hash_sel);
    if (SOC_FAILURE(rv)) {
        return 0;
    }

    return soc_td2_vlan_xlate_entry_hash(unit, hash_sel, entry);
}

uint32
soc_td2_egr_vlan_xlate_hash(int unit, int hash_sel, int key_nbits,
                            void *base_entry, uint8 *key)
{
    uint32 rv;

    /*
     * Cache bucket mask and shift amount for upper crc
     */
    if (SOC_CONTROL(unit)->hash_mask_egr_vlan_xlate == 0) {
        uint32  mask;
        int     bits;

        /* 8 Entries per bucket */
        mask = soc_mem_index_max(unit, EGR_VLAN_XLATEm) >> 3;
        bits = 0;
        rv = 1;
        while (rv && (mask & rv)) {
            bits += 1;
            rv <<= 1;
        }
        SOC_CONTROL(unit)->hash_mask_egr_vlan_xlate = mask;
        SOC_CONTROL(unit)->hash_bits_egr_vlan_xlate = bits;
    }

    switch (hash_sel) {
    case FB_HASH_CRC16_UPPER:
        rv = soc_crc16b(key, key_nbits);
        rv >>= 16 - SOC_CONTROL(unit)->hash_bits_egr_vlan_xlate;
        break;

    case FB_HASH_CRC16_LOWER:
        rv = soc_crc16b(key, key_nbits);
        break;

    case FB_HASH_LSB:
        if (key_nbits == 0) {
            return 0;
        }
        switch (soc_mem_field32_get(unit, EGR_VLAN_XLATEm, base_entry,
                                    ENTRY_TYPEf)) {
        case TD2_EVLXLT_HASH_KEY_TYPE_VLAN_XLATE:
        case TD2_EVLXLT_HASH_KEY_TYPE_VLAN_XLATE_DVP:
        case TD2_EVLXLT_HASH_KEY_TYPE_FCOE_XLATE:
        case TD2_EVLXLT_HASH_KEY_TYPE_FCOE_XLATE_DVP:
            rv = soc_mem_field32_get(unit, EGR_VLAN_XLATEm, base_entry,
                                     XLATE__HASH_LSBf);
            break;
        case TD2_EVLXLT_HASH_KEY_TYPE_ISID_XLATE:
        case TD2_EVLXLT_HASH_KEY_TYPE_ISID_DVP_XLATE:
            rv = soc_mem_field32_get(unit, EGR_VLAN_XLATEm, base_entry,
                                     MIM_ISID__HASH_LSBf);
            break;
        case TD2_EVLXLT_HASH_KEY_TYPE_L2GRE_VFI:
        case TD2_EVLXLT_HASH_KEY_TYPE_L2GRE_VFI_DVP:
            rv = soc_mem_field32_get(unit, EGR_VLAN_XLATEm, base_entry,
                                     L2GRE_VFI__HASH_LSBf);
            break;
        case TD2_EVLXLT_HASH_KEY_TYPE_VXLAN_VFI:
        case TD2_EVLXLT_HASH_KEY_TYPE_VXLAN_VFI_DVP:
            rv = soc_mem_field32_get(unit, EGR_VLAN_XLATEm, base_entry,
                                     VXLAN_VFI__HASH_LSBf);
            break;
        default:
            rv = 0;
            break;
        }
        break;

    case FB_HASH_ZERO:
        rv = 0;
        break;

    case FB_HASH_CRC32_UPPER:
        rv = soc_crc32b(key, key_nbits);
        rv >>= 32 - SOC_CONTROL(unit)->hash_bits_egr_vlan_xlate;
        break;

    case FB_HASH_CRC32_LOWER:
        rv = soc_crc32b(key, key_nbits);
        break;

    default:
        LOG_ERROR(BSL_LS_SOC_HASH,
                  (BSL_META_U(unit,
                              "soc_td2_egr_vlan_xlate_hash: invalid hash_sel %d\n"),
                   hash_sel));
        rv = 0;
        break;
    }

    return rv & SOC_CONTROL(unit)->hash_mask_egr_vlan_xlate;
}

int
soc_td2_egr_vlan_xlate_base_entry_to_key(int unit, void *entry, uint8 *key)
{
    soc_field_t field_list[2];

    switch (soc_mem_field32_get(unit, EGR_VLAN_XLATEm, entry, ENTRY_TYPEf)) {
    case TD2_EVLXLT_HASH_KEY_TYPE_VLAN_XLATE:
    case TD2_EVLXLT_HASH_KEY_TYPE_VLAN_XLATE_DVP:
    case TD2_EVLXLT_HASH_KEY_TYPE_FCOE_XLATE:
    case TD2_EVLXLT_HASH_KEY_TYPE_FCOE_XLATE_DVP:
        field_list[0] = XLATE__KEYf;
        break;
    case TD2_EVLXLT_HASH_KEY_TYPE_ISID_XLATE:
    case TD2_EVLXLT_HASH_KEY_TYPE_ISID_DVP_XLATE:
        field_list[0] = MIM_ISID__KEYf;
        break;
    case TD2_EVLXLT_HASH_KEY_TYPE_L2GRE_VFI:
    case TD2_EVLXLT_HASH_KEY_TYPE_L2GRE_VFI_DVP:
        field_list[0] = L2GRE_VFI__KEYf;
        break;
    case TD2_EVLXLT_HASH_KEY_TYPE_VXLAN_VFI:
    case TD2_EVLXLT_HASH_KEY_TYPE_VXLAN_VFI_DVP:
        field_list[0] = VXLAN_VFI__KEYf;
        break;
    default:
        return 0;
    }
    field_list[1] = INVALIDf;
    return _soc_td2_hash_entry_to_key(unit, entry, key, EGR_VLAN_XLATEm,
                                      field_list);
}

uint32
soc_td2_egr_vlan_xlate_entry_hash(int unit, int hash_sel, uint32 *entry)
{
    int             key_nbits;
    uint8           key[SOC_MAX_MEM_WORDS * 4];
    uint32          index;

    key_nbits = soc_td2_egr_vlan_xlate_base_entry_to_key(unit, entry, key);
    index = soc_td2_egr_vlan_xlate_hash(unit, hash_sel, key_nbits, entry, key);

    return index;
}

uint32
soc_td2_egr_vlan_xlate_bank_entry_hash(int unit, int bank, uint32 *entry)
{
    int rv, hash_sel;

    rv = soc_td2_hash_sel_get(unit, EGR_VLAN_XLATEm, bank, &hash_sel);
    if (SOC_FAILURE(rv)) {
        return 0;
    }

    return soc_td2_egr_vlan_xlate_entry_hash(unit, hash_sel, entry);
}

uint32
soc_td2_ing_vp_vlan_member_hash(int unit, int hash_sel, int key_nbits,
                                void *base_entry, uint8 *key)
{
    uint32 rv;

    /*
     * Cache bucket mask and shift amount for upper crc
     */
    if (SOC_CONTROL(unit)->hash_mask_ing_vp_vlan_member == 0) {
        uint32  mask;
        int     bits;

        /* 8 Entries per bucket */
        mask = soc_mem_index_max(unit, ING_VP_VLAN_MEMBERSHIPm) >> 3;
        bits = 0;
        rv = 1;
        while (rv && (mask & rv)) {
            bits += 1;
            rv <<= 1;
        }
        SOC_CONTROL(unit)->hash_mask_ing_vp_vlan_member = mask;
        SOC_CONTROL(unit)->hash_bits_ing_vp_vlan_member = bits;
    }

    switch (hash_sel) {
    case FB_HASH_CRC16_UPPER:
        rv = soc_crc16b(key, key_nbits);
        rv >>= 16 - SOC_CONTROL(unit)->hash_bits_ing_vp_vlan_member;
        break;

    case FB_HASH_CRC16_LOWER:
        rv = soc_crc16b(key, key_nbits);
        break;

    case FB_HASH_LSB:
        if (key_nbits == 0) {
            return 0;
        }
        rv = soc_mem_field32_get(unit, ING_VP_VLAN_MEMBERSHIPm, base_entry,
                                 HASH_LSBf);
        break;

    case FB_HASH_ZERO:
        rv = 0;
        break;

    case FB_HASH_CRC32_UPPER:
        rv = soc_crc32b(key, key_nbits);
        rv >>= 32 - SOC_CONTROL(unit)->hash_bits_ing_vp_vlan_member;
        break;

    case FB_HASH_CRC32_LOWER:
        rv = soc_crc32b(key, key_nbits);
        break;

    default:
        LOG_ERROR(BSL_LS_SOC_HASH,
                  (BSL_META_U(unit,
                              "soc_td2_inv_vp_vlan_member_hash: invalid hash_sel %d\n"),
                   hash_sel));
        rv = 0;
        break;
    }

    return rv & SOC_CONTROL(unit)->hash_mask_ing_vp_vlan_member;
}

int
soc_td2_ing_vp_vlan_member_base_entry_to_key(int unit, void *entry, uint8 *key)
{
    static soc_field_t field_list[] = {
        KEYf,
        INVALIDf
    };

    return _soc_td2_hash_entry_to_key(unit, entry, key,
                                      ING_VP_VLAN_MEMBERSHIPm, field_list);
}

uint32
soc_td2_ing_vp_vlan_member_entry_hash(int unit, int hash_sel, uint32 *entry)
{
    int             key_nbits;
    uint8           key[SOC_MAX_MEM_WORDS * 4];
    uint32          index;

    key_nbits = soc_td2_ing_vp_vlan_member_base_entry_to_key(unit, entry, key);
    index = soc_td2_ing_vp_vlan_member_hash(unit, hash_sel, key_nbits, entry,
                                            key);

    return index;
}

uint32
soc_td2_ing_vp_vlan_member_bank_entry_hash(int unit, int bank, uint32 *entry)
{
    int rv, hash_sel;

    rv = soc_td2_hash_sel_get(unit, ING_VP_VLAN_MEMBERSHIPm, bank, &hash_sel);
    if (SOC_FAILURE(rv)) {
        return 0;
    }

    return soc_td2_ing_vp_vlan_member_entry_hash(unit, hash_sel, entry);
}

uint32
soc_td2_egr_vp_vlan_member_hash(int unit, int hash_sel, int key_nbits,
                                void *base_entry, uint8 *key)
{
    uint32 rv;

    /*
     * Cache bucket mask and shift amount for upper crc
     */
    if (SOC_CONTROL(unit)->hash_mask_egr_vp_vlan_member == 0) {
        uint32  mask;
        int     bits;

        /* 8 Entries per bucket */
        mask = soc_mem_index_max(unit, EGR_VP_VLAN_MEMBERSHIPm) >> 3;
        bits = 0;
        rv = 1;
        while (rv && (mask & rv)) {
            bits += 1;
            rv <<= 1;
        }
        SOC_CONTROL(unit)->hash_mask_egr_vp_vlan_member = mask;
        SOC_CONTROL(unit)->hash_bits_egr_vp_vlan_member = bits;
    }

    switch (hash_sel) {
    case FB_HASH_CRC16_UPPER:
        rv = soc_crc16b(key, key_nbits);
        rv >>= 16 - SOC_CONTROL(unit)->hash_bits_egr_vp_vlan_member;
        break;

    case FB_HASH_CRC16_LOWER:
        rv = soc_crc16b(key, key_nbits);
        break;

    case FB_HASH_LSB:
        if (key_nbits == 0) {
            return 0;
        }
        rv = soc_mem_field32_get(unit, EGR_VP_VLAN_MEMBERSHIPm, base_entry,
                                 HASH_LSBf);
        break;

    case FB_HASH_ZERO:
        rv = 0;
        break;

    case FB_HASH_CRC32_UPPER:
        rv = soc_crc32b(key, key_nbits);
        rv >>= 32 - SOC_CONTROL(unit)->hash_bits_egr_vp_vlan_member;
        break;

    case FB_HASH_CRC32_LOWER:
        rv = soc_crc32b(key, key_nbits);
        break;

    default:
        LOG_ERROR(BSL_LS_SOC_HASH,
                  (BSL_META_U(unit,
                              "soc_td2_inv_vp_vlan_member_hash: invalid hash_sel %d\n"),
                   hash_sel));
        rv = 0;
        break;
    }

    return rv & SOC_CONTROL(unit)->hash_mask_egr_vp_vlan_member;
}

int
soc_td2_egr_vp_vlan_member_base_entry_to_key(int unit, void *entry, uint8 *key)
{
    static soc_field_t field_list[] = {
        KEYf,
        INVALIDf
    };

    return _soc_td2_hash_entry_to_key(unit, entry, key,
                                      EGR_VP_VLAN_MEMBERSHIPm, field_list);
}

uint32
soc_td2_egr_vp_vlan_member_entry_hash(int unit, int hash_sel, uint32 *entry)
{
    int             key_nbits;
    uint8           key[SOC_MAX_MEM_WORDS * 4];
    uint32          index;

    key_nbits = soc_td2_egr_vp_vlan_member_base_entry_to_key(unit, entry, key);
    index = soc_td2_egr_vp_vlan_member_hash(unit, hash_sel, key_nbits, entry,
                                           key);

    return index;
}

uint32
soc_td2_egr_vp_vlan_member_bank_entry_hash(int unit, int bank, uint32 *entry)
{
    int rv, hash_sel;

    rv = soc_td2_hash_sel_get(unit, EGR_VP_VLAN_MEMBERSHIPm, bank, &hash_sel);
    if (SOC_FAILURE(rv)) {
        return 0;
    }

    return soc_td2_egr_vp_vlan_member_entry_hash(unit, hash_sel, entry);
}

uint32
soc_td2_ing_dnat_address_type_hash(int unit, int hash_sel, int key_nbits,
                                   void *base_entry, uint8 *key)
{
    uint32 rv;

    /*
     * Cache bucket mask and shift amount for upper crc
     */
    if (SOC_CONTROL(unit)->hash_mask_ing_dnat_address_type == 0) {
        uint32  mask;
        int     bits;

        /* 8 Entries per bucket */
        mask = soc_mem_index_max(unit, ING_DNAT_ADDRESS_TYPEm) >> 3;
        bits = 0;
        rv = 1;
        while (rv && (mask & rv)) {
            bits += 1;
            rv <<= 1;
        }
        SOC_CONTROL(unit)->hash_mask_ing_dnat_address_type = mask;
        SOC_CONTROL(unit)->hash_bits_ing_dnat_address_type = bits;
    }

    switch (hash_sel) {
    case FB_HASH_CRC16_UPPER:
        rv = soc_crc16b(key, key_nbits);
        rv >>= 16 - SOC_CONTROL(unit)->hash_bits_ing_dnat_address_type;
        break;

    case FB_HASH_CRC16_LOWER:
        rv = soc_crc16b(key, key_nbits);
        break;

    case FB_HASH_LSB:
        if (key_nbits == 0) {
            return 0;
        }
        rv = soc_mem_field32_get(unit, ING_DNAT_ADDRESS_TYPEm, base_entry,
                                 HASH_LSBf);
        break;

    case FB_HASH_ZERO:
        rv = 0;
        break;

    case FB_HASH_CRC32_UPPER:
        rv = soc_crc32b(key, key_nbits);
        rv >>= 32 - SOC_CONTROL(unit)->hash_bits_ing_dnat_address_type;
        break;

    case FB_HASH_CRC32_LOWER:
        rv = soc_crc32b(key, key_nbits);
        break;

    default:
        LOG_ERROR(BSL_LS_SOC_HASH,
                  (BSL_META_U(unit,
                              "soc_td2_inv_vp_vlan_member_hash: invalid hash_sel %d\n"),
                   hash_sel));
        rv = 0;
        break;
    }

    return rv & SOC_CONTROL(unit)->hash_mask_ing_dnat_address_type;
}

int
soc_td2_ing_dnat_address_type_base_entry_to_key(int unit, void *entry,
                                                uint8 *key)
{
    static soc_field_t field_list[] = {
        KEYf,
        INVALIDf
    };

    return _soc_td2_hash_entry_to_key(unit, entry, key, ING_DNAT_ADDRESS_TYPEm,
                                      field_list);
}

uint32
soc_td2_ing_dnat_address_type_entry_hash(int unit, int hash_sel, uint32 *entry)
{
    int             key_nbits;
    uint8           key[SOC_MAX_MEM_WORDS * 4];
    uint32          index;

    key_nbits = soc_td2_ing_dnat_address_type_base_entry_to_key(unit, entry,
                                                                key);
    index = soc_td2_ing_dnat_address_type_hash(unit, hash_sel, key_nbits,
                                               entry, key);

    return index;
}

uint32
soc_td2_ing_dnat_address_type_bank_entry_hash(int unit, int bank,
                                              uint32 *entry)
{
    int rv, hash_sel;

    rv = soc_td2_hash_sel_get(unit, ING_DNAT_ADDRESS_TYPEm, bank, &hash_sel);
    if (SOC_FAILURE(rv)) {
        return 0;
    }

    return soc_td2_ing_dnat_address_type_entry_hash(unit, hash_sel, entry);
}

uint32
soc_td2_l2_endpoint_id_hash(int unit, int hash_sel, int key_nbits,
                            void *base_entry, uint8 *key)
{
    uint32 rv;

    /*
     * Cache bucket mask and shift amount for upper crc
     */
    if (SOC_CONTROL(unit)->hash_mask_l2_endpoint_id == 0) {
        uint32  mask;
        int     bits;

        /* 8 Entries per bucket */
        mask = soc_mem_index_max(unit, L2_ENDPOINT_IDm) >> 3;
        bits = 0;
        rv = 1;
        while (rv && (mask & rv)) {
            bits += 1;
            rv <<= 1;
        }
        SOC_CONTROL(unit)->hash_mask_l2_endpoint_id = mask;
        SOC_CONTROL(unit)->hash_bits_l2_endpoint_id = bits;
    }

    switch (hash_sel) {
    case FB_HASH_CRC16_UPPER:
        rv = soc_crc16b(key, key_nbits);
        rv >>= 16 - SOC_CONTROL(unit)->hash_bits_l2_endpoint_id;
        break;

    case FB_HASH_CRC16_LOWER:
    rv = soc_crc16b(key, key_nbits);
        break;

    case FB_HASH_LSB:
        if (key_nbits == 0) {
            return 0;
        }
        switch (soc_mem_field32_get(unit, L2_ENDPOINT_IDm, base_entry,
                                    KEY_TYPEf)) {
        case TD2_L2_HASH_KEY_TYPE_BRIDGE:
        case TD2_L2_HASH_KEY_TYPE_VFI:
            rv = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, base_entry,
                                     L2__HASH_LSBf);
            break;
        case TD2_L2_HASH_KEY_TYPE_VIF:
            rv = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, base_entry,
                                     VIF__HASH_LSBf);
            break;
        case TD2_L2_HASH_KEY_TYPE_PE_VID:
            rv = soc_mem_field32_get(unit, L2_ENDPOINT_IDm, base_entry,
                                     PE_VID__HASH_LSBf);
            break;
        default:
            rv = 0;
        }
        break;

    case FB_HASH_ZERO:
        rv = 0;
        break;

    case FB_HASH_CRC32_UPPER:
        rv = soc_crc32b(key, key_nbits);
        rv >>= 32 - SOC_CONTROL(unit)->hash_bits_l2_endpoint_id;
        break;

    case FB_HASH_CRC32_LOWER:
        rv = soc_crc32b(key, key_nbits);
        break;

    default:
        LOG_ERROR(BSL_LS_SOC_HASH,
                  (BSL_META_U(unit,
                              "soc_td2_l2_endpoint_id_hash: invalid hash_sel %d\n"),
                   hash_sel));
        rv = 0;
        break;
    }

    return rv & SOC_CONTROL(unit)->hash_mask_l2_endpoint_id;
}

int
soc_td2_l2_endpoint_id_base_entry_to_key(int unit, void *entry, uint8 *key)
{
    soc_field_t field_list[2];

    switch (soc_mem_field32_get(unit, L2_ENDPOINT_IDm, entry, KEY_TYPEf)) {
    case TD2_L2_HASH_KEY_TYPE_BRIDGE:
    case TD2_L2_HASH_KEY_TYPE_VFI:
        field_list[0] = L2__KEYf;
        break;
    case TD2_L2_HASH_KEY_TYPE_VIF:
        field_list[0] = VIF__KEYf;
        break;
    case TD2_L2_HASH_KEY_TYPE_PE_VID:
        field_list[0] = PE_VID__KEYf;
        break;
    default:
        return 0;
    }
    field_list[1] = INVALIDf;
    return _soc_td2_hash_entry_to_key(unit, entry, key, L2_ENDPOINT_IDm,
                                      field_list);
}

uint32
soc_td2_l2_endpoint_id_entry_hash(int unit, int hash_sel, uint32 *entry)
{
    int             key_nbits;
    uint8           key[SOC_MAX_MEM_WORDS * 4];
    uint32          index;

    key_nbits = soc_td2_l2_endpoint_id_base_entry_to_key(unit, entry, key);
    index = soc_td2_l2_endpoint_id_hash(unit, hash_sel, key_nbits, entry, key);

    return index;
}

uint32
soc_td2_l2_endpoint_id_bank_entry_hash(int unit, int bank, uint32 *entry)
{
    int rv, hash_sel;

    rv = soc_td2_hash_sel_get(unit, L2_ENDPOINT_IDm, bank, &hash_sel);
    if (SOC_FAILURE(rv)) {
        return 0;
    }

    return soc_td2_l2_endpoint_id_entry_hash(unit, hash_sel, entry);
}

uint32
soc_td2_endpoint_queue_map_hash(int unit, int hash_sel, int key_nbits,
                                void *base_entry, uint8 *key)
{
    uint32 rv;

    /*
     * Cache bucket mask and shift amount for upper crc
     */
    if (SOC_CONTROL(unit)->hash_mask_endpoint_queue_map == 0) {
        uint32  mask;
        int     bits;

        /* 8 Entries per bucket */
        mask = soc_mem_index_max(unit, ENDPOINT_QUEUE_MAPm) >> 3;
        bits = 0;
        rv = 1;
        while (rv && (mask & rv)) {
            bits += 1;
            rv <<= 1;
        }
        SOC_CONTROL(unit)->hash_mask_endpoint_queue_map = mask;
        SOC_CONTROL(unit)->hash_bits_endpoint_queue_map = bits;
    }

    switch (hash_sel) {
    case FB_HASH_CRC16_UPPER:
        rv = soc_crc16b(key, key_nbits);
        rv >>= 16 - SOC_CONTROL(unit)->hash_bits_endpoint_queue_map;
        break;

    case FB_HASH_CRC16_LOWER:
    rv = soc_crc16b(key, key_nbits);
        break;

    case FB_HASH_LSB:
        if (key_nbits == 0) {
            return 0;
        }
        switch (soc_mem_field32_get(unit, ENDPOINT_QUEUE_MAPm, base_entry,
                                    KEY_TYPEf)) {
        case 0:
            rv = soc_mem_field32_get(unit, ENDPOINT_QUEUE_MAPm, base_entry,
                                     HASH_LSBf);
            break;
        default:
            rv = 0;
        }
        break;

    case FB_HASH_ZERO:
        rv = 0;
        break;

    case FB_HASH_CRC32_UPPER:
        rv = soc_crc32b(key, key_nbits);
        rv >>= 32 - SOC_CONTROL(unit)->hash_bits_endpoint_queue_map;
        break;

    case FB_HASH_CRC32_LOWER:
        rv = soc_crc32b(key, key_nbits);
        break;

    default:
        LOG_ERROR(BSL_LS_SOC_HASH,
                  (BSL_META_U(unit,
                              "soc_td2_endpoint_queue_map_hash: invalid hash_sel %d\n"),
                   hash_sel));
        rv = 0;
        break;
    }

    return rv & SOC_CONTROL(unit)->hash_mask_endpoint_queue_map;
}

int
soc_td2_endpoint_queue_map_base_entry_to_key(int unit, void *entry, uint8 *key)
{
    static soc_field_t field_list[] = {
        KEYf,
        INVALIDf
    };

    return _soc_td2_hash_entry_to_key(unit, entry, key, ENDPOINT_QUEUE_MAPm,
                                      field_list);
}

uint32
soc_td2_endpoint_queue_map_entry_hash(int unit, int hash_sel, uint32 *entry)
{
    int             key_nbits;
    uint8           key[SOC_MAX_MEM_WORDS * 4];
    uint32          index;

    key_nbits = soc_td2_endpoint_queue_map_base_entry_to_key(unit, entry, key);
    index = soc_td2_endpoint_queue_map_hash(unit, hash_sel, key_nbits, entry,
                                            key);

    return index;
}

uint32
soc_td2_endpoint_queue_map_bank_entry_hash(int unit, int bank, uint32 *entry)
{
    int rv, hash_sel;

    rv = soc_td2_hash_sel_get(unit, ENDPOINT_QUEUE_MAPm, bank, &hash_sel);
    if (SOC_FAILURE(rv)) {
        return 0;
    }

    return soc_td2_endpoint_queue_map_entry_hash(unit, hash_sel, entry);
}

int
soc_hash_bucket_get(int unit, int mem, int bank, uint32 *entry, uint32 *bucket)
{
    switch(mem) {
    case L2Xm:
        *bucket = soc_td2_l2x_bank_entry_hash(unit, bank, entry);
        return SOC_E_NONE;
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        *bucket = soc_td2_l3x_bank_entry_hash(unit, bank, entry);
        return SOC_E_NONE;
    default:
        return SOC_E_PARAM;
    }
}

int
soc_hash_index_get(int unit, int mem, int bank, int bucket)
{
    int rv;
    int index;
    int entries_per_row, bank_base, bucket_offset;

    switch(mem) {
    case L2Xm:
        rv = soc_trident2_hash_bank_info_get(unit, mem, bank, NULL,
                                             &entries_per_row, NULL,
                                             &bank_base, &bucket_offset);
        if (SOC_SUCCESS(rv)) {
            return bank_base + bucket * entries_per_row + bucket_offset;
        }
        break;
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        rv = soc_trident2_hash_bank_info_get(unit, L3_ENTRY_ONLYm, bank, NULL,
                                             &entries_per_row, NULL,
                                             &bank_base, &bucket_offset);
        if (SOC_SUCCESS(rv)) {
            index = bank_base + bucket * entries_per_row + bucket_offset;
            if (mem == L3_ENTRY_IPV4_MULTICASTm || mem == L3_ENTRY_IPV6_UNICASTm) {
                return index / 2;
            } else if (mem == L3_ENTRY_IPV6_MULTICASTm) {
                return index / 4;
            }
            return index;
        }
        break;
    default:
        break;
    }

    return 0;
}

#endif /* BCM_TRIDENT2_SUPPORT */
