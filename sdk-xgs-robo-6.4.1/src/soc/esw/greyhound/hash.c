/*
 * $Id: hash.c,v 1.1.8.3 Broadcom SDK $
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
 * Purpose:     Greyhound hash table calculation routines
 * Requires:
 */

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/hash.h>

#if defined(BCM_GREYHOUND_SUPPORT)

#include <soc/greyhound.h>

STATIC int
_soc_greyhound_hash_generic_entry_to_key(int unit, void *entry, uint8 *key,
                                   soc_mem_t mem, soc_field_t *field_list)
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
        val_bits = 63;
        break;
    case L3_ENTRY_ONLYm:
    case L3_ENTRY_IPV4_UNICASTm:
    case L3_ENTRY_IPV6_UNICASTm:
    case L3_ENTRY_IPV4_MULTICASTm:
    case L3_ENTRY_IPV6_MULTICASTm:
        val_bits = 275;
        break;
    case VLAN_XLATEm:
        val_bits = 41;
        break;
    case VLAN_MACm:
        val_bits = 51;
        break;
    case EGR_VLAN_XLATEm:
        val_bits = 40;
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

int
soc_greyhound_l2x_base_entry_to_key(int unit, uint32 *entry, uint8 *key)
{
    int i;
    soc_field_t field_list[4];

    for (i=0; i < 4 ; i++)
    field_list[i] = INVALIDf;
    
    switch (soc_mem_field32_get(unit, L2Xm, entry, KEY_TYPEf)) {
    case GH_L2_HASH_KEY_TYPE_BRIDGE:
        field_list[0] = KEY_TYPEf;
        field_list[1] = VLAN_IDf;
        field_list[2] = MAC_ADDRf;        
        break;
    case GH_L2_HASH_KEY_TYPE_SINGLE_CROSS_CONNECT:
        field_list[0] = KEY_TYPEf;
        field_list[1] = OVIDf;
        break;        
    case GH_L2_HASH_KEY_TYPE_DOUBLE_CROSS_CONNECT:
        field_list[0] = KEY_TYPEf;
        field_list[1] = OVIDf;
        field_list[2] = IVIDf;
        break;
    case GH_L2_HASH_KEY_TYPE_VIF:
        field_list[0] = VIF__KEYf;
        break;
    case GH_L2_HASH_KEY_TYPE_PE_VID:
        field_list[0] = PE_VID__KEYf;
        break;
    default:
        return 0;
    }
    
    return _soc_greyhound_hash_generic_entry_to_key(unit, entry, key, L2Xm,
                                              field_list);
}


#endif /* BCM_GREYHOUND_SUPPORT */
