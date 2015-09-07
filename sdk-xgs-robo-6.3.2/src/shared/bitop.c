/*
 * $Id: bitop.c 1.7 Broadcom SDK $
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
 * Bit Array routines
 */

#include <shared/bitop.h>
#include <sal/core/libc.h>

/* Same as shr_bitop_range_null, but for a single SHR_BITDCL.
   The following constraints are kept:
   a. first < SHR_BITWID
   b. first + bit_count <= SHR_BITWID
*/
STATIC INLINE int
shr_bitop_range_null_one_bitdcl(CONST SHR_BITDCL bits,
                                CONST int first,
                                CONST int bit_count)
{
    SHR_BITDCL mask = ~0;

    mask >>= (SHR_BITWID - bit_count);
    /* Move the mask to start from 'first' offset */
    mask <<= first;
    return (bits & mask) == 0;
}

/* returns 1 if the bit array is empty */
int
shr_bitop_range_null(CONST SHR_BITDCL *bits,
                     CONST int first,
                     int bit_count)
{
    CONST SHR_BITDCL *ptr;
    int woff_first, wremain;

    if(bit_count <= 0) {
        return 1;
    }

    /* Pointer to first SHR_BITDCL in 'bits' that contains 'first' */
    ptr = bits + (first / SHR_BITWID);
    
    /* Offset of 'first' bit within this SDH_BITDCL */
    woff_first = first % SHR_BITWID;

    /* Check if 'first' is SHR_BITWID aligned */
    if (woff_first != 0) {
        /*  Get remaining bits in this SDH_BITDCL */
        wremain = SHR_BITWID - woff_first;
        if (bit_count <= wremain) {
            /* All the range is in one SHR_BITDCL */
            return shr_bitop_range_null_one_bitdcl(*ptr,
                                                   woff_first,
                                                   bit_count);
        }
        /* We should check the first SHR_BITDCL, and might also continue */
        if (!shr_bitop_range_null_one_bitdcl(*ptr, woff_first, wremain)) {
            return 0;
        }
        bit_count -= wremain;
        ++ptr;
    }
    while (bit_count >= SHR_BITWID) {
        /* We're testing a full SHR_BITDCL */
        if (*(ptr++)) {
            return 0;
        }
        bit_count -= SHR_BITWID;
    }
    /* This is the last SHR_BITDCL, and it is not SHR_BITWID */
    if(bit_count > 0) {
        return shr_bitop_range_null_one_bitdcl(*ptr, 0, bit_count);
    }
    return 1;
}

/* Same as shr_bitop_range_eq, but for a single SHR_BITDCL.
   The following constraints are kept:
   a. first < SHR_BITWID
   b. first + range <= SHR_BITWID
*/
STATIC INLINE int
shr_bitop_range_eq_one_bitdcl(CONST SHR_BITDCL bits1,
                              CONST SHR_BITDCL bits2,
                              CONST int first,
                              CONST int range)
{
    SHR_BITDCL mask = ~0;
    mask >>= (SHR_BITWID - range);
    /* Move the mask to start from 'first' offset */
    mask <<= first;
    return (bits1 & mask) == (bits2 & mask);
}

/* returns 1 if the two bitmaps are equal */
int
shr_bitop_range_eq(CONST SHR_BITDCL *bits1,
                   CONST SHR_BITDCL *bits2,
                   CONST int first,
                   int range)
{
    CONST SHR_BITDCL *ptr1;
    CONST SHR_BITDCL *ptr2;
    int woff_first, wremain;

    if(range <= 0) {
        return 1;
    }

    ptr1 = bits1 + (first / SHR_BITWID);
    ptr2 = bits2 + (first / SHR_BITWID);
    
    woff_first = first % SHR_BITWID;
    
    if (woff_first != 0) {
        wremain = SHR_BITWID - woff_first;
        if (range <= wremain) {
            return shr_bitop_range_eq_one_bitdcl(*ptr1,
                                                 *ptr2,
                                                 woff_first,
                                                 range);
        }
        if (!shr_bitop_range_eq_one_bitdcl(*ptr1,
                                          *ptr2,
                                          woff_first,
                                          wremain)) {
            return 0;
        }
        range -= wremain;
        ++ptr1, ++ptr2;
    }
    while (range >= SHR_BITWID) {
        if (*(ptr1++) != *(ptr2++)) {
            return 0;
        }
        range -= SHR_BITWID;
    }
    if(range > 0) {
        return shr_bitop_range_eq_one_bitdcl(*ptr1, *ptr2, 0, range);
    }
    return 1;
}

STATIC INLINE int
shr_bitop_range_count_uchar(CONST uint8 bits)
{
    uint8 tmp_res;
 
    /* Efficient algorithm to count bits */
    tmp_res = (bits & 0x55) + ((bits & 0xaa) >> 1);
    tmp_res = (tmp_res & 0x33) + ((tmp_res & 0xcc) >> 2);
    return (int) ((tmp_res & 0xf) + ((tmp_res & 0xf0) >> 4));
} 

STATIC INLINE int
shr_bitop_range_count_bitdcl_all_bits(CONST SHR_BITDCL bits)
{
    int count = 0, i;
    for(i = 0; i < sizeof(SHR_BITWID); ++i) {
        count += shr_bitop_range_count_uchar((bits >> (8*i)) & 0xff);
    }
    return count;
}

/* Same as shr_bitop_range_count, but for a single SHR_BITDCL.
   The following constraints are kept:
   a. first < SHR_BITWID
   b. first + range <= SHR_BITWID
*/
STATIC INLINE int
shr_bitop_range_count_one_bitdcl(CONST SHR_BITDCL bits,
                                 CONST int first,
                                 CONST int range)
{
    SHR_BITDCL mask = ~0;
    mask >>= (SHR_BITWID - range);
    mask <<= first;
    return shr_bitop_range_count_bitdcl_all_bits(bits & mask);
}

/* returns the number of set bits is the specified range for the bitmap */
void
shr_bitop_range_count(CONST SHR_BITDCL *bits,
                      CONST int first,
                      int range,
                      int *count)
{
    CONST SHR_BITDCL *ptr;
    int woff_first, wremain;

    ptr = bits + (first / SHR_BITWID);
    
    woff_first = first % SHR_BITWID;
    
    *count = 0;

    if(range <= 0) {
        return;
    }

    if (woff_first != 0) {
        wremain = SHR_BITWID - woff_first;
        if (range <= wremain) {
            *count = shr_bitop_range_count_one_bitdcl(*ptr, woff_first, range);
            return;
        }
        *count += shr_bitop_range_count_one_bitdcl(*ptr, woff_first, wremain);
        range -= wremain;
        ++ptr;
    }
    while (range >= SHR_BITWID) {
        *count += shr_bitop_range_count_bitdcl_all_bits(*(ptr++));
        range -= SHR_BITWID;
    }
    if(range > 0) {
        *count += shr_bitop_range_count_one_bitdcl(*ptr, 0, range);
    }
}

/* Same as shr_bitop_range_copy, but for a single SHR_BITDCL.
   The following constraints are kept:
   a. dst_first, src_first < SHR_BITWID
   b. dst_first + range, src_first + range <= SHR_BITWID
*/
STATIC INLINE void
shr_bitop_range_copy_one_bitdcl(SHR_BITDCL *dst_ptr,
                                CONST int dst_first,
                                CONST SHR_BITDCL src,
                                CONST int src_first,
                                CONST int range)
{
    SHR_BITDCL data;
    SHR_BITDCL mask;
    /* no need to check that dst_first == 0 and src_first == 0,
       It must be becuse of the constrains */
    if ((range) == SHR_BITWID) {
        *(dst_ptr) = src;
        return;
    }
    /* get the data */
    data = src >> (src_first);
    /* align the data to the place it may be inserted */
    data <<= (dst_first);

    /* We might have bits in src_ptr above src_first + range
       that need to be cleared */
    mask = ~0;
    mask >>= SHR_BITWID - range;
    mask <<= dst_first;
    data &= mask;
    *(dst_ptr) &= ~mask;
    *(dst_ptr) |= data;
}

void
shr_bitop_range_copy(SHR_BITDCL *dst_ptr,
                     CONST int dst_first,
                     CONST SHR_BITDCL *src_ptr,
                     CONST int src_first,
                     int range)
{
    if(range <= 0) {
        return;
    }

    if ((((dst_first) % SHR_BITWID) == 0) &&
        (((src_first) % SHR_BITWID) == 0) &&
        (((range) % SHR_BITWID) == 0)) {
            sal_memcpy(&((dst_ptr)[(dst_first) / SHR_BITWID]),
                &((src_ptr)[(src_first) / SHR_BITWID]),
                SHR_BITALLOCSIZE(range));
    } else {
        SHR_BITDCL *cur_dst;
        CONST SHR_BITDCL *cur_src;

        int woff_src, woff_dst, wremain;

        cur_dst = (dst_ptr) + ((dst_first) / SHR_BITWID);
        cur_src = (src_ptr) + ((src_first) / SHR_BITWID);

        woff_src = src_first % SHR_BITWID;
        woff_dst = dst_first % SHR_BITWID;

        if (woff_dst >= woff_src) {
            wremain = SHR_BITWID - woff_dst;
        } else {
            wremain = SHR_BITWID - woff_src;
        }
        if (range <= wremain) {
            shr_bitop_range_copy_one_bitdcl(cur_dst,
                                            woff_dst,
                                            *cur_src,
                                            woff_src,
                                            range);
            return;
        }
        shr_bitop_range_copy_one_bitdcl(cur_dst,
                                        woff_dst,
                                        *cur_src,
                                        woff_src,
                                        wremain);
        range -= wremain;
        while (range >= SHR_BITWID) {
            if (woff_dst >= woff_src) {
                ++cur_dst;
                wremain = woff_dst - woff_src;
                if(wremain > 0) {
                    shr_bitop_range_copy_one_bitdcl(cur_dst,
                                                    0,
                                                    *cur_src,
                                                    SHR_BITWID - wremain,
                                                    wremain);
                }
            } else {
                ++cur_src;
                wremain = woff_src - woff_dst;
                shr_bitop_range_copy_one_bitdcl(cur_dst,
                                                SHR_BITWID - wremain,
                                                *cur_src,
                                                0,
                                                wremain);
            }
            range -= wremain;
            wremain = SHR_BITWID - wremain;
            if (woff_dst >= woff_src) {
                ++cur_src;
                shr_bitop_range_copy_one_bitdcl(cur_dst,
                                                SHR_BITWID - wremain,
                                                *cur_src,
                                                0,
                                                wremain);
            } else {
                ++cur_dst;
                shr_bitop_range_copy_one_bitdcl(cur_dst,
                                                0,
                                                *cur_src,
                                                SHR_BITWID - wremain,
                                                wremain);
            }
            range -= wremain;
        }

        if (woff_dst >= woff_src) {
            ++cur_dst;
            wremain = woff_dst - woff_src;
            if (range <= wremain) {
                if(range > 0) {
                    shr_bitop_range_copy_one_bitdcl(cur_dst,
                                                    0,
                                                    *cur_src,
                                                    SHR_BITWID - wremain,
                                                    range);
                }
                return;
            }
            shr_bitop_range_copy_one_bitdcl(cur_dst,
                                            0,
                                            *cur_src,
                                            SHR_BITWID - wremain,
                                            wremain);
        } else {
            ++cur_src;
            wremain = woff_src - woff_dst;
            if (range <= wremain) {
                if(range > 0) {
                    shr_bitop_range_copy_one_bitdcl(cur_dst,
                                                    SHR_BITWID - wremain,
                                                    *cur_src,
                                                    0,
                                                    range);
                }
                return;
            }
            shr_bitop_range_copy_one_bitdcl(cur_dst,
                                            SHR_BITWID - wremain,
                                            *cur_src,
                                            0,
                                            wremain);
        }
        range -= wremain;

        if(range > 0) {
            wremain = SHR_BITWID - wremain;
            if (woff_dst >= woff_src) {
                ++cur_src;
                shr_bitop_range_copy_one_bitdcl(cur_dst,
                                                SHR_BITWID - wremain,
                                                *cur_src,
                                                0,
                                                range);
            } else {
                ++cur_dst;
                shr_bitop_range_copy_one_bitdcl(cur_dst,
                                                0,
                                                *cur_src,
                                                SHR_BITWID - wremain,
                                                range);
            }
        }
    }
}

/* The same as _SHR_BITOP_RANGE, but for a single SHR_BITDCL.
   The following constraints are kept: 
 * a. _first < SHR_BITWID 
 * b. _first + _bit_count < SHR_BITWID.
 */
#define _SHR_BITOP_RANGE_ONE_BITDCL(_bits1,     \
                                    _bits2,     \
                                    _first,     \
                                    _bit_count, \
                                    _dest,      \
                                    _op)        \
{                                               \
    SHR_BITDCL _mask = ~0;                      \
    SHR_BITDCL _data;                           \
    _mask >>= (SHR_BITWID - (_bit_count));      \
    _mask <<=_first;                            \
    _data = ((_bits1) _op (_bits2)) & _mask;    \
    *(_dest) &= ~_mask;                         \
    *(_dest) |= _data;                          \
}

#define _SHR_BITOP_RANGE(_bits1, _bits2, _first, _bit_count, _dest, _op) \
{                                               \
    CONST SHR_BITDCL *_ptr_bits1;               \
    CONST SHR_BITDCL *_ptr_bits2;               \
    SHR_BITDCL *_ptr_dest;                      \
    int _woff_first, _wremain;                  \
                                                \
    _ptr_bits1 =                                \
        (_bits1) + ((_first) / SHR_BITWID);     \
    _ptr_bits2 =                                \
        (_bits2) + ((_first) / SHR_BITWID);     \
    _ptr_dest =                                 \
        (_dest) + ((_first) / SHR_BITWID);      \
    _woff_first = ((_first) % SHR_BITWID);      \
                                                \
    _wremain = SHR_BITWID - _woff_first;        \
    if ((_bit_count) <= _wremain) {             \
        _SHR_BITOP_RANGE_ONE_BITDCL(*_ptr_bits1,\
            *_ptr_bits2,                        \
            _woff_first,                        \
            (_bit_count),                       \
            _ptr_dest, _op);                    \
        return;                                 \
    }                                           \
    _SHR_BITOP_RANGE_ONE_BITDCL(*_ptr_bits1,    \
        *_ptr_bits2, _woff_first, _wremain,     \
        _ptr_dest, _op);                        \
        (_bit_count) -= _wremain;               \
        ++_ptr_bits1; ++_ptr_bits2; ++_ptr_dest;\
    while ((_bit_count) >= SHR_BITWID) {        \
        *_ptr_dest =                            \
            (*_ptr_bits1) _op (*_ptr_bits2);    \
        (_bit_count) -= SHR_BITWID;             \
        ++_ptr_bits1; ++_ptr_bits2; ++_ptr_dest;\
    }                                           \
    if((_bit_count) > 0) {                      \
        _SHR_BITOP_RANGE_ONE_BITDCL(            \
            *_ptr_bits1,                        \
            *_ptr_bits2,                        \
            0,                                  \
            (_bit_count),                       \
            _ptr_dest,                          \
            _op);                               \
    }                                           \
}

void
shr_bitop_range_and(CONST SHR_BITDCL *bits1,
                    CONST SHR_BITDCL *bits2,
                    CONST int first,
                    int bit_count,
                    SHR_BITDCL *dest)
{
    if(bit_count > 0) {
        _SHR_BITOP_RANGE(bits1, bits2, first, bit_count, dest, &);
    }
}

void
shr_bitop_range_or(CONST SHR_BITDCL *bits1,
                   CONST SHR_BITDCL *bits2,
                   CONST int first,
                   int bit_count,
                   SHR_BITDCL *dest)
{
    if(bit_count > 0) {
        _SHR_BITOP_RANGE(bits1, bits2, first, bit_count, dest, |);
    }
}

void
shr_bitop_range_xor(CONST SHR_BITDCL *bits1,
                    CONST SHR_BITDCL *bits2,
                    CONST int first,
                    int bit_count,
                    SHR_BITDCL *dest)
{
    if(bit_count > 0) {
        _SHR_BITOP_RANGE(bits1, bits2, first, bit_count, dest, ^);
    }
}

void
shr_bitop_range_remove(CONST SHR_BITDCL *bits1,
                       CONST SHR_BITDCL *bits2,
                       CONST int first,
                       int bit_count,
                       SHR_BITDCL *dest)
{
    if(bit_count > 0) {
        _SHR_BITOP_RANGE(bits1, bits2, first, bit_count, dest, & ~);
    }
}

/* The same as _SHR_BITNEGATE_RANGE, but for a single SHR_BITDCL.
   The following constraints are kept:
 * a. _first < SHR_BITWID 
 * b. _first + _bit_count < SHR_BITWID.
 */
#define _SHR_BITNEGATE_RANGE_ONE_BITDCL(_bits1, _first, _bit_count, _dest) \
{                                               \
    SHR_BITDCL _mask = ~0;                      \
    SHR_BITDCL _data;                           \
    _mask >>= (SHR_BITWID - (_bit_count));      \
    _mask <<=_first;                            \
    _data =  ~(_bits1) & _mask;                 \
    *(_dest) &= ~_mask;                         \
    *(_dest) |= _data;                          \
}

#define _SHR_BITNEGATE_RANGE(_bits1, _first, _bit_count, _dest) \
{                                               \
    CONST SHR_BITDCL *_ptr_bits1;               \
    SHR_BITDCL *_ptr_dest;                      \
    int _woff_first, _wremain;                  \
                                                \
    _ptr_bits1 =                                \
        (_bits1) + ((_first) / SHR_BITWID);     \
    _ptr_dest =                                 \
        (_dest) + ((_first) / SHR_BITWID);      \
    _woff_first = ((_first) % SHR_BITWID);      \
                                                \
    _wremain = SHR_BITWID - _woff_first;        \
    if ((_bit_count) <= _wremain) {             \
        _SHR_BITNEGATE_RANGE_ONE_BITDCL(        \
            *_ptr_bits1,                        \
            _woff_first,                        \
            (_bit_count),                       \
            _ptr_dest);                         \
        return;                                 \
    }                                           \
    _SHR_BITNEGATE_RANGE_ONE_BITDCL(*_ptr_bits1,\
        _woff_first, _wremain, _ptr_dest);      \
        (_bit_count) -= _wremain;               \
        ++_ptr_bits1; ++_ptr_dest;              \
    while ((_bit_count) >= SHR_BITWID) {        \
        *_ptr_dest = ~(*_ptr_bits1);            \
        (_bit_count) -= SHR_BITWID;             \
        ++_ptr_bits1; ++_ptr_dest;              \
    }                                           \
    if((_bit_count) > 0) {                      \
        _SHR_BITNEGATE_RANGE_ONE_BITDCL(        \
            *_ptr_bits1,                        \
            0,                                  \
            (_bit_count),                       \
            _ptr_dest);                         \
    }                                           \
}

void
shr_bitop_range_negate(CONST SHR_BITDCL *bits1,
                       CONST int first,
                       int bit_count,
                       SHR_BITDCL *dest)
{
    if(bit_count > 0) {
        _SHR_BITNEGATE_RANGE(bits1, first, bit_count, dest);
    }
}

/* The same as shr_bitop_range_clear, but for a single SHR_BITDCL.
   The following constraints are kept:
 * a. b < SHR_BITWID 
 * b. b + c < SHR_BITWID.
 */
STATIC INLINE void
shr_bitop_range_clear_one_bitdcl(SHR_BITDCL *a, CONST int b, CONST int c)
{
    SHR_BITDCL mask = ~0;
    mask >>= (SHR_BITWID - c);
    mask <<= b;
    *a &= ~mask;
}

void
shr_bitop_range_clear(SHR_BITDCL *a, CONST int b, int c)
{
    SHR_BITDCL *ptr;
    int woff_first, wremain;

    if(c <= 0) {
        return;
    }

    ptr = a + (b / SHR_BITWID);
    
    woff_first = b % SHR_BITWID;
    
    if (woff_first != 0) {
        wremain = SHR_BITWID - woff_first;
        if (c <= wremain) {
            shr_bitop_range_clear_one_bitdcl(ptr, woff_first, c);
            return;
        }
        shr_bitop_range_clear_one_bitdcl(ptr, woff_first, wremain);
        c -= wremain;
        ++ptr;
    }
    while (c >= SHR_BITWID) {
        *(ptr++) = 0;
        c -= SHR_BITWID;
    }

    if(c > 0) {
        shr_bitop_range_clear_one_bitdcl(ptr, 0, c);
    }
}

/* The same as shr_bitop_range_set, but for a single SHR_BITDCL.
   The following constraints are kept:
 * a. b < SHR_BITWID 
 * b. b + c < SHR_BITWID.
 */
STATIC INLINE void
shr_bitop_range_set_one_bitdcl(SHR_BITDCL *a, CONST int b, CONST int c)
{
    SHR_BITDCL mask = ~0;
    mask >>= (SHR_BITWID - c);
    mask <<= b;
    *a |= mask;
}

void
shr_bitop_range_set(SHR_BITDCL *a, CONST int b, int c)
{
    SHR_BITDCL *ptr;
    int woff_first, wremain;

    if(c <= 0) {
        return;
    }

    ptr = a + (b / SHR_BITWID);
    
    woff_first = b % SHR_BITWID;
    
    if (woff_first != 0) {
        wremain = SHR_BITWID - woff_first;
        if (c <= wremain) {
            shr_bitop_range_set_one_bitdcl(ptr, woff_first, c);
            return;
        }
        shr_bitop_range_set_one_bitdcl(ptr, woff_first, wremain);
        c -= wremain;
        ++ptr;
    }
    while (c >= SHR_BITWID) {
        *(ptr++) = ~0;
        c -= SHR_BITWID;
    }
    if(c > 0) {
        shr_bitop_range_set_one_bitdcl(ptr, 0, c);
    }
}

