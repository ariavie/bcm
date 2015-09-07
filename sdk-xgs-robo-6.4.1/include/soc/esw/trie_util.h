/*
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
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.
 * BROADCOM SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: trie_util.h,v 1.3 Broadcom SDK $
 *
 * TAPS utility defines/functions
 *
 *-----------------------------------------------------------------------------
 */
#ifndef _ESW_TRIDENT2_TRIE_UTIL_H_
#define _ESW_TRIDENT2_TRIE_UTIL_H_

#ifdef ALPM_ENABLE
#ifndef ALPM_IPV6_128_SUPPORT
#include <sal/types.h>

/* loop up key size */
#define TAPS_IPV4_MAX_VRF_SIZE (16)
#define TAPS_IPV4_PFX_SIZE (32)
#define TAPS_IPV4_KEY_SIZE (TAPS_IPV4_MAX_VRF_SIZE + TAPS_IPV4_PFX_SIZE)
#define TAPS_IPV4_KEY_SIZE_WORDS (((TAPS_IPV4_KEY_SIZE)+31)/32)

#define TAPS_IPV6_MAX_VRF_SIZE (16)
#define TAPS_IPV6_PFX_SIZE (128)
#define TAPS_IPV6_KEY_SIZE (TAPS_IPV6_MAX_VRF_SIZE + TAPS_IPV6_PFX_SIZE)
#define TAPS_IPV6_KEY_SIZE_WORDS (((TAPS_IPV6_KEY_SIZE)+31)/32)
#define TAPS_MAX_KEY_SIZE       (TAPS_IPV6_KEY_SIZE)
#define TAPS_MAX_KEY_SIZE_WORDS (((TAPS_MAX_KEY_SIZE)+31)/32)




#define TP_MASK(len) \
  (((len)>=32)?0xffffFFFF:((1U<<(len))-1))

#define TP_SHL(data, shift) \
  (((shift)>=32)?0:((data)<<(shift)))

#define TP_SHR(data, shift) \
  (((shift)>=32)?0:((data)>>(shift)))

/* get bit at bit_position from uint32 key array of maximum length of max_len 
 * assuming the big-endian word order. bit_pos is 0 based. for example, assuming
 * the max_len is 48 bits, then key[0] has bits 47-32, and key[1] has bits 31-0.
 * use _TAPS_GET_KEY_BIT(key, 0, 48) to get bit 0.
 */
#define TP_BITS2IDX(bit_pos, max_len) \
  ((BITS2WORDS(max_len) - 1) - (bit_pos)/32)

#define _TAPS_GET_KEY_BIT(key, bit_pos, max_len) \
  (((key)[TP_BITS2IDX(bit_pos, max_len)] & (1<<((bit_pos)%32))) >> ((bit_pos)%32))

#define _TAPS_SET_KEY_BIT(key, bit_pos, max_len) \
  ((key)[TP_BITS2IDX(bit_pos, max_len)] |= (1<<((bit_pos)%32)))

/* Get "len" number of bits start with lsb bit postion from an unsigned int array
 * the array is assumed to be with format described above in _TAPS_GET_KEY_BIT
 * NOTE: len must be <= 32 bits
 */
#define _TAPS_GET_KEY_BITS(key, lsb, len, max_len)			  \
  ((TP_SHR((key)[TP_BITS2IDX(lsb, max_len)], ((lsb)%32)) |	\
    ((TP_BITS2IDX(lsb, max_len)<1)?0:(TP_SHL((key)[TP_BITS2IDX(lsb, max_len)-1], 32-((lsb)%32))))) & \
   (TP_MASK((len))))

typedef struct taps_ipv4_prefix_s {
    uint32 length:8;
    uint32 vrf:16;
    uint32 key;
} taps_ipv4_prefix_t;

typedef struct taps_ipv6_prefix_s {
    uint32 length:8;
    uint32 vrf:16;
    uint32 key[4];
} taps_ipv6_prefix_t;

extern int taps_show_prefix(uint32 max_key_size, uint32 *key, uint32 length);

extern int taps_key_shift(uint32 max_key_size, uint32 *key, uint32 length, int32 shift);

extern int taps_key_match(uint32 max_key_size, uint32 *key1, uint32 length1,
			  uint32 *key2, uint32 length2);

extern int taps_get_lsb(uint32 max_mask_size, uint32 *mask, int32 *lsb);

extern int taps_get_bpm_pfx(unsigned int *bpm, unsigned int key_len,
			    unsigned int max_key_len, unsigned int *pfx_len);

#endif /* ALPM_IPV6_128_SUPPORT */
#endif /* ALPM_ENABLE */

#endif /* _ESW_TRIDENT2_TRIE_UTIL_H_ */
