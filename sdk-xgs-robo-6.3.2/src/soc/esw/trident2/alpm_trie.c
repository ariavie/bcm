/*
 * $Id: alpm_trie.c 1.10 Broadcom SDK $
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
 * File:    trie.c
 * Purpose: Custom Trie Data structure (Leveraged from Caladan3)
 * Requires:
 */

/* Implementation notes:
 * Trie is a prefix based data strucutre. It is based on modification to digital search trie.
 * This implementation is not a Path compressed Binary Trie (or) a Patricia Trie.
 * It is a custom version which represents prefix on a digital search trie as following.
 * A given node on the trie could be a Payload node or a Internal node. Each node is represented
 * by <skip address, skip length> pair. Each node represents the given prefix it represents when
 * the prefix is viewed from Right to Left. ie., Most significant bits to Least significant bits.
 * Each node has a Right & Left child which branches based on bit on that position. 
 * There can be empty split node i.e, <0,0> just to host two of its children. 
 *  
 */
#include <soc/types.h>
#include <soc/drv.h>

#ifdef ALPM_ENABLE
#ifdef BCM_TRIDENT2_SUPPORT

#ifdef ALPM_IPV6_128_SUPPORT
#include <shared/util.h>
#include <sal/appl/sal.h>
#include <sal/core/libc.h>
#include <sal/core/time.h>
#include <soc/esw/trie.h>

#define _MAX_SKIP_LEN_  (31)
#define _MAX_KEY_LEN_   (64)
#define _MAX_KEY_WORDS_ (BITS2WORDS(_MAX_KEY_LEN_))

#define SHL(data, shift, max) \
    (((shift)>=(max))?0:((data)<<(shift)))

#define SHR(data, shift, max) \
    (((shift)>=(max))?0:((data)>>(shift)))

#define MASK(len) \
    (((len)>=32 || (len)==0)?0xFFFFFFFF:((1<<(len))-1))

#define BITMASK(len) \
    (((len)>=32)?0xFFFFFFFF:((1<<(len))-1))

#define ABS(n) ((((int)(n)) < 0) ? -(n) : (n))

#define _NUM_WORD_BITS_ (32)

#define BITS2SKIPOFF(x) (((x) + _MAX_SKIP_LEN_-1) / _MAX_SKIP_LEN_)

/* key packing expetations:
 * eg., 48 bit key
 * - 10/8 -> key[0]=0, key[1]=8
 * - 0x123456789a -> key[0] = 0x12 key[1] = 0x3456789a
 * length - represents number of valid bits from farther to lower index ie., 1->0 
 */

#define KEY_BIT2IDX(x) (((BITS2WORDS(_MAX_KEY_LEN_)*32) - (x))/32)


/* (internal) Generic operation macro on bit array _a, with bit _b */
#define	_BITOP(_a, _b, _op)	\
        ((_a) _op (1U << ((_b) % _NUM_WORD_BITS_)))

/* Specific operations */
#define	_BITGET(_a, _b)	_BITOP(_a, _b, &)
#define	_BITSET(_a, _b)	_BITOP(_a, _b, |=)
#define	_BITCLR(_a, _b)	_BITOP(_a, _b, &= ~)

/* get the bit position of the LSB set in bit 0 to bit "msb" of "data"
 * (max 32 bits), "lsb" is set to -1 if no bit is set in "data".
 */
#define BITGETLSBSET(data, msb, lsb)    \
    {                                    \
	lsb = 0;                         \
	while ((lsb)<=(msb)) {		 \
	    if ((data)&(1<<(lsb)))     { \
		break;                   \
	    } else { (lsb)++;}           \
	}                                \
	lsb = ((lsb)>(msb))?-1:(lsb);    \
    }

/********************************************************/
/* Get a chunk of bits from a key MSB bit - on word0, lsb on word 1.. 
 */
unsigned int _key_get_bits(unsigned int *key, 
                           unsigned int pos /* 1based */, 
                           unsigned int len,
                           unsigned int skip_len_check)
{
    unsigned int val=0, delta=0, diff, bitpos;

    if (!key || pos <1 || pos > _MAX_KEY_LEN_ || 
        (skip_len_check==0 && len > _MAX_SKIP_LEN_)) assert(0);

    bitpos = (pos-1) % _NUM_WORD_BITS_;
    bitpos++; /* 1 based */

    if (bitpos >= len) {
        diff = bitpos - len;
        val = SHR(key[KEY_BIT2IDX(pos)], diff, _NUM_WORD_BITS_);
        val &= MASK(len);
        return val;
    } else {
        diff = len - bitpos;
        if (skip_len_check==0) assert(diff <= _MAX_SKIP_LEN_);
        val = key[KEY_BIT2IDX(pos)] & MASK(bitpos);
        val = SHL(val, diff, _NUM_WORD_BITS_);
        /* get bits from next word */
        delta = _key_get_bits(key, pos-bitpos, diff, skip_len_check);
        return (delta | val);
    }
}		


/* 
 * Assumes the layout for 
 * 0 - most significant word
 * _MAX_KEY_WORDS_ - least significant word
 * eg., for key size of 48, word0-[bits 48-32] word1-[bits31-0]
 */
int _key_shift_left(unsigned int *key, unsigned int shift)
{
    unsigned int index=0;

    if (!key || shift > _MAX_SKIP_LEN_) return SOC_E_PARAM;

    for(index=KEY_BIT2IDX(_MAX_KEY_LEN_); index < KEY_BIT2IDX(1); index++) {
        key[index] = SHL(key[index], shift,_NUM_WORD_BITS_) | \
                     SHR(key[index+1],_NUM_WORD_BITS_-shift,_NUM_WORD_BITS_);
    }

    key[index] = SHL(key[index], shift, _NUM_WORD_BITS_);

    /* mask off snippets bit on MSW */
    key[0] &= MASK(_MAX_KEY_LEN_ % _NUM_WORD_BITS_);
    return SOC_E_NONE;
}

/* 
 * Assumes the layout for 
 * 0 - most significant word
 * _MAX_KEY_WORDS_ - least significant word
 * eg., for key size of 48, word0-[bits 48-32] word1-[bits31-0]
 */
int _key_shift_right(unsigned int *key, unsigned int shift)
{
    unsigned int index=0;

    if (!key || shift > _MAX_SKIP_LEN_) return SOC_E_PARAM;

    /* To Senthil: the logic here seems is wrong, fix it based on my 
     * understanding. Otherwise index here would be 0xFFFFFFFF and 
     * would be corrutping memory.
     */
    for(index=KEY_BIT2IDX(1); index <= KEY_BIT2IDX(_MAX_KEY_LEN_); index--) {
        key[index] = SHR(key[index], shift,_NUM_WORD_BITS_) | \
                     SHL(key[index-1],_NUM_WORD_BITS_-shift,_NUM_WORD_BITS_);
    }

    key[index] = SHR(key[index], shift, _NUM_WORD_BITS_);

    /* mask off snippets bit on MSW */
    key[0] &= MASK(_MAX_KEY_LEN_ % _NUM_WORD_BITS_);
    return SOC_E_NONE;
}


/* 
 * Assumes the layout for 
 * 0 - most significant word
 * _MAX_KEY_WORDS_ - least significant word
 * eg., for key size of 48, word0-[bits 48-32] word1-[bits31-0]
 */
int _key_append(unsigned int *key, 
                unsigned int *length,
                unsigned int skip_addr,
                unsigned int skip_len)
{
    int rv=SOC_E_NONE;

    if (!key || !length || (skip_len + *length > _MAX_KEY_LEN_) || skip_len > _MAX_SKIP_LEN_ ) return SOC_E_PARAM;

    rv = _key_shift_left(key, skip_len);
    if (SOC_SUCCESS(rv)) {
        key[KEY_BIT2IDX(1)] |= skip_addr;
        *length += skip_len;
    }

    return rv;
}

int _bpm_append(unsigned int *key, 
                unsigned int *length,
                unsigned int skip_addr,
                unsigned int skip_len)
{
    int rv=SOC_E_NONE;

    if (!key || !length || (skip_len + *length > _MAX_KEY_LEN_) || skip_len > (_MAX_SKIP_LEN_+1) ) return SOC_E_PARAM;

    if (skip_len == 32) {
	key[0] = key[1];
	key[1] = skip_addr;
	*length += skip_len;
    } else {
	rv = _key_shift_left(key, skip_len);
	if (SOC_SUCCESS(rv)) {
	    key[KEY_BIT2IDX(1)] |= skip_addr;
	    *length += skip_len;
	}
    }

    return rv;
}

/*
 * Function:
 *     lcplen
 * Purpose:
 *     returns longest common prefix length provided a key & skip address
 */
unsigned int
lcplen(unsigned int *key, unsigned int len1,
       unsigned int skip_addr, unsigned int len2)
{
    unsigned int diff;
    unsigned int lcp = len1 < len2 ? len1 : len2;

    if ((len1 > _MAX_KEY_LEN_) || (len2 > _MAX_KEY_LEN_)) {
	soc_cm_print("len1 %d or len2 %d is larger than %d\n", len1, len2, _MAX_KEY_LEN_);
	assert(0);
    } 

    if (len1 == 0 || len2 == 0) return 0;

    diff = _key_get_bits(key, len1, lcp, 0);
    diff ^= (SHR(skip_addr, len2 - lcp, _MAX_SKIP_LEN_) & MASK(lcp));

    while (diff) {
        diff >>= 1;
        --lcp;
    }

    return lcp;
}

int print_trie_node(trie_node_t *trie, void *datum)
{
    if (trie != NULL) {

	soc_cm_print("trie: %p, type %s, skip_addr 0x%x skip_len %d count:%d Child[0]:%p Child[1]:%p\n",
                     trie, (trie->type == PAYLOAD)?"P":"I",
                     trie->skip_addr, trie->skip_len, 
                     trie->count, trie->child[0].child_node, trie->child[1].child_node);
    }
    return SOC_E_NONE;
}

int _trie_preorder_traverse(trie_node_t *trie, trie_callback_f cb, void *user_data)
{
    int rv = SOC_E_NONE;
    trie_node_t *tmp1, *tmp2;

    if (trie == NULL || !cb) {
	return SOC_E_NONE;
    } else {
        /* make the node delete safe */
        tmp1 = trie->child[0].child_node;
        tmp2 = trie->child[1].child_node;
        rv = cb(trie, user_data);
    }

    if (SOC_SUCCESS(rv)) {
        rv = _trie_preorder_traverse(tmp1, cb, user_data);
    }
    if (SOC_SUCCESS(rv)) {
        rv = _trie_preorder_traverse(tmp2, cb, user_data);
    }
    return rv;
}

int _trie_postorder_traverse(trie_node_t *trie, trie_callback_f cb, void *user_data)
{
    int rv = SOC_E_NONE;

    if (trie == NULL) {
	return SOC_E_NONE;
    }

    if (SOC_SUCCESS(rv)) {
        rv = _trie_postorder_traverse(trie->child[0].child_node, cb, user_data);
    }
    if (SOC_SUCCESS(rv)) {
        rv = _trie_postorder_traverse(trie->child[1].child_node, cb, user_data);
    }
    if (SOC_SUCCESS(rv)) {
        rv = cb(trie, user_data);
    }
    return rv;
}

int _trie_inorder_traverse(trie_node_t *trie, trie_callback_f cb, void *user_data)
{
    int rv = SOC_E_NONE;
    trie_node_t *tmp = NULL;

    if (trie == NULL) {
	return SOC_E_NONE;
    }

    if (SOC_SUCCESS(rv)) {
        rv = _trie_inorder_traverse(trie->child[0].child_node, cb, user_data);
    }

    
    /* make the trie pointers delete safe */
    tmp = trie->child[1].child_node;

    if (SOC_SUCCESS(rv)) {
        rv = cb(trie, user_data);
    }

    if (SOC_SUCCESS(rv)) {
        rv = _trie_inorder_traverse(tmp, cb, user_data);
    }
    return rv;
}

int _trie_traverse(trie_node_t *trie, trie_callback_f cb, 
                   void *user_data,  trie_traverse_order_e_t order)
{
    int rv = SOC_E_NONE;

    switch(order) {
    case _TRIE_PREORDER_TRAVERSE:
        rv = _trie_preorder_traverse(trie, cb, user_data);
        break;
    case _TRIE_POSTORDER_TRAVERSE:
        rv = _trie_postorder_traverse(trie, cb, user_data);
        break;
    case _TRIE_INORDER_TRAVERSE:
        rv = _trie_inorder_traverse(trie, cb, user_data);
        break;
    default:
        assert(0);
    }

    return rv;
}

/*
 * Function:
 *     trie_traverse
 * Purpose:
 *     Traverse the trie & call the application callback with user data 
 */
int trie_traverse(trie_t *trie, trie_callback_f cb, 
                  void *user_data, trie_traverse_order_e_t order)
{
    if (order < _TRIE_PREORDER_TRAVERSE ||
        order >= _TRIE_TRAVERSE_MAX || !cb) return SOC_E_PARAM;

    if (trie == NULL) {
	return SOC_E_NONE;
    } else {
        return _trie_traverse(trie->trie, cb, user_data, order);
    }
}

int _trie_preorder_iter_get_first(trie_node_t *node, trie_node_t **payload)
{
    int rv = SOC_E_NONE;

    if (!payload) return SOC_E_PARAM;

    if (*payload != NULL) return SOC_E_NONE;

    if (node == NULL) {
	return SOC_E_NONE;
    } else {
        if (node->type == PAYLOAD) {
            *payload = node;
            return rv;
        }
    }

    if (SOC_SUCCESS(rv)) {
        rv = _trie_preorder_iter_get_first(node->child[0].child_node, payload);
    }
    if (SOC_SUCCESS(rv)) {
        rv = _trie_preorder_iter_get_first(node->child[1].child_node, payload);
    }
    return rv;
}

/*
 * Function:
 *     trie_iter_get_first
 * Purpose:
 *     Traverse the trie & return pointer to first payload node
 */
int trie_iter_get_first(trie_t *trie, trie_node_t **payload)
{
    int rv = SOC_E_EMPTY;

    if (!trie || !payload) return SOC_E_PARAM;

    if (trie && trie->trie) {
        *payload = NULL;
        return _trie_preorder_iter_get_first(trie->trie, payload);
    }

    return rv;
}

int _trie_dump(trie_node_t *trie, trie_callback_f cb, 
               void *user_data, unsigned int level)
{
    if (trie == NULL) {
	return SOC_E_NONE;
    } else {
        unsigned int lvl = level;
	while(lvl) {
	    if (lvl == 1) {
		soc_cm_print("|-");
	    } else {
		soc_cm_print("| ");
	    }
	    lvl--; 
	}

        if (cb) {
            cb(trie, user_data);
        } else {
            print_trie_node(trie, NULL);
        }
    }

    _trie_dump(trie->child[0].child_node, cb, user_data, level+1);
    _trie_dump(trie->child[1].child_node, cb, user_data, level+1);
    return SOC_E_NONE;
}

/*
 * Function:
 *     trie_dump
 * Purpose:
 *     Dumps the trie pre-order [root|left|child]
 */
int trie_dump(trie_t *trie, trie_callback_f cb, void *user_data)
{
    if (trie->trie) {
        return _trie_dump(trie->trie, cb, user_data, 0);
    } else {
        return SOC_E_PARAM;
    }
}

int _trie_search(trie_node_t *trie,
                 unsigned int *key,
                 unsigned int length,
                 trie_node_t **payload,
                 unsigned int *result_key,
                 unsigned int *result_len,
                 unsigned int dump)
{
    unsigned int lcp=0;
    int bit=0, rv=SOC_E_NONE;

    if (!trie) return SOC_E_PARAM;
    if ((result_key && !result_len) || (!result_key && result_len)) return SOC_E_PARAM;

    lcp = lcplen(key, length, trie->skip_addr, trie->skip_len);

    if (dump) {
        print_trie_node(trie, (unsigned int *)1);
    }

    if (length > trie->skip_len) {
        if (lcp == trie->skip_len) {
            bit = (key[KEY_BIT2IDX(length - lcp)] & \
                   SHL(1,(length - lcp - 1) % _NUM_WORD_BITS_,_NUM_WORD_BITS_)) ? 1:0;
            if (dump) {
                soc_cm_print(" Length: %d Next-Bit[%d] \n", length, bit);
            }

            if (result_key) {
                rv = _key_append(result_key, result_len, trie->skip_addr, trie->skip_len);
                if (SOC_FAILURE(rv)) return rv;
            }

            /* based on next bit branch left or right */
            if (trie->child[bit].child_node) {

                if (result_key) {
                    rv = _key_append(result_key, result_len, bit, 1);
                    if (SOC_FAILURE(rv)) return rv;
                }

                return _trie_search(trie->child[bit].child_node, key, 
                                    length - lcp - 1, payload, 
                                    result_key, result_len, dump);
            } else {
                return SOC_E_NOT_FOUND; /* not found */
            }
        } else { 
            return SOC_E_NOT_FOUND; /* not found */
        }
    } else if (length == trie->skip_len) {
        if (lcp == length) {
            if (dump) soc_cm_print(": MATCH \n");
            *payload = trie;
            assert(trie->type == PAYLOAD);
            if (result_key) {
                rv = _key_append(result_key, result_len, trie->skip_addr, trie->skip_len);
                if (SOC_FAILURE(rv)) return rv;
            }
            return SOC_E_NONE;
        }
        else return SOC_E_NOT_FOUND;
    } else {
        return SOC_E_NOT_FOUND; /* not found */        
    }
}

/*
 * Function:
 *     trie_search
 * Purpose:
 *     Search the given trie for exact match of provided prefix/length
 *     If dump is set to 1 it traces the path as it traverses the trie 
 */
int trie_search(trie_t *trie, 
                unsigned int *key, 
                unsigned int length,
                trie_node_t **payload)
{
    if (trie->trie) {
        return _trie_search(trie->trie, key, length, payload, NULL, NULL, 0);
    } else {
        return SOC_E_NOT_FOUND;
    }
}

/*
 * Function:
 *     trie_search
 * Purpose:
 *     Search the given trie for provided prefix/length
 *     If dump is set to 1 it traces the path as it traverses the trie 
 */
int trie_search_verbose(trie_t *trie, 
                        unsigned int *key, 
                        unsigned int length,
                        trie_node_t **payload,
                        unsigned int *result_key, 
                        unsigned int *result_len)
{
    if (trie->trie) {
        return _trie_search(trie->trie, key, length, payload, result_key, result_len, 0);
    } else {
        return SOC_E_NOT_FOUND;
    }
}

/*
 * Internal function for LPM match searching.
 * callback on all payload nodes if cb != NULL.
 */
int _trie_find_lpm(trie_node_t *trie,
                   unsigned int *key,
                   unsigned int length,
                   trie_node_t **payload,
		   trie_callback_f cb,
		   void *user_data)
{
    unsigned int lcp=0;
    int bit=0, rv=SOC_E_NONE;

    if (!trie) return SOC_E_PARAM;

    lcp = lcplen(key, length, trie->skip_addr, trie->skip_len);

    if ((length > trie->skip_len) && (lcp == trie->skip_len)) {
        if (trie->type == PAYLOAD) {
	    /* lpm cases */
	    if (payload != NULL) {
		/* update lpm result */
		*payload = trie;
	    }

	    if (cb != NULL) {
		/* callback with any nodes which is shorter and matches the prefix */
		rv = cb(trie, user_data);
		if (SOC_FAILURE(rv)) {
		    /* early bailout if there is error in callback handling */
		    return rv;
		}
	    }
	}

        bit = (key[KEY_BIT2IDX(length - lcp)] & \
               SHL(1,(length - lcp - 1) % _NUM_WORD_BITS_, _NUM_WORD_BITS_)) ? 1:0;

        /* based on next bit branch left or right */
        if (trie->child[bit].child_node) {
            return _trie_find_lpm(trie->child[bit].child_node, key, length - lcp - 1,
				  payload, cb, user_data);
        } 
    } else if ((length == trie->skip_len) && (lcp == length)) {
        if (trie->type == PAYLOAD) {
	    /* exact match case */
	    if (payload != NULL) {		
		/* lpm is exact match */
		*payload = trie;
	    }

	    if (cb != NULL) {
		/* callback with the exact match node */
		rv = cb(trie, user_data);
		if (SOC_FAILURE(rv)) {
		    /* early bailout if there is error in callback handling */
		    return rv;
		}		
	    }
        }
    }
    return rv;
}

/*
 * Function:
 *     trie_find_lpm
 * Purpose:
 *     Find the longest prefix matched with given prefix 
 */
int trie_find_lpm(trie_t *trie, 
                  unsigned int *key, 
                  unsigned int length,
                  trie_node_t **payload)
{
    *payload = NULL;

    if (trie->trie) {
        return _trie_find_lpm(trie->trie, key, length, payload, NULL, NULL);
    }

    return SOC_E_NOT_FOUND;
}

/*
 * Function:
 *     trie_find_pm
 * Purpose:
 *     Find the prefix matched nodes with given prefix and callback
 *     with specified callback funtion and user data
 */
int trie_find_pm(trie_t *trie, 
		 unsigned int *key, 
		 unsigned int length,
		 trie_callback_f cb,
		 void *user_data)
{

    if (trie->trie) {
        return _trie_find_lpm(trie->trie, key, length, NULL, cb, user_data);
    }

    return SOC_E_NONE;
}

/* trie->bpm format:
 * bit 0 is for the pivot itself (longest)
 * bit skip_len is for the trie branch leading to the pivot node (shortest)
 * bits (0-skip_len) is for the routes in the parent node's bucket
 */
int _trie_find_bpm(trie_node_t *trie,
                   unsigned int *key,
                   unsigned int length,
		   int *bpm_length)
{
    unsigned int lcp=0, local_bpm_mask=0;
    int bit=0, rv=SOC_E_NONE, local_bpm=0;

    if (!trie) return SOC_E_PARAM;

    /* calculate number of matching msb bits */
    lcp = lcplen(key, length, trie->skip_addr, trie->skip_len);

    if (length > trie->skip_len) {
	if (lcp == trie->skip_len) {
	    /* fully matched and more bits to check, go down the trie */
	    bit = (key[KEY_BIT2IDX(length - lcp)] &			\
		   SHL(1,(length - lcp - 1) % _NUM_WORD_BITS_, _NUM_WORD_BITS_)) ? 1:0;
	    
	    if (trie->child[bit].child_node) {
		rv = _trie_find_bpm(trie->child[bit].child_node, key, length - lcp - 1, bpm_length);
		/* on the way back, start bpm_length accumulation when encounter first non-0 bpm */
		if (*bpm_length >= 0) {
		    /* child node has non-zero bpm, just need to accumulate skip_len and branch bit */
		    *bpm_length += (trie->skip_len+1);
		    return rv;
		} else if (trie->bpm & BITMASK(trie->skip_len+1)) {
		    /* first non-zero bmp on the way back */
		    BITGETLSBSET(trie->bpm, trie->skip_len, local_bpm);
		    if (local_bpm >= 0) {
                        *bpm_length = trie->skip_len - local_bpm;
		    }
		}
		/* on the way back, and so far all bpm are 0 */
		return rv;
	    }
	}
    }

    /* no need to go further, we find whatever bits matched and 
     * check that part of the bpm mask
     */
    local_bpm_mask = trie->bpm & (~(BITMASK(trie->skip_len-lcp)));
    if (local_bpm_mask & BITMASK(trie->skip_len+1)) {
	/* first non-zero bmp on the way back */
	BITGETLSBSET(local_bpm_mask, trie->skip_len, local_bpm);
	if (local_bpm >= 0) {
	    *bpm_length = trie->skip_len - local_bpm;
	}
    }

    return rv;
}

/*
 * Function:
 *     trie_find_prefix_bpm
 * Purpose:
 *    Given a key/length return the Best prefix match length
 *    key/bpm_pfx_len will be the BPM for the key/length
 *    using the bpm info in the trie database
 */
int trie_find_prefix_bpm(trie_t *trie, 
                         unsigned int *key, 
                         unsigned int length,
                         unsigned int *bpm_pfx_len)
{
    /* Return: SOC_E_EMPTY is not bpm bit is found */
    int rv = SOC_E_EMPTY, bpm=0;

    if (!trie || !key || !bpm_pfx_len ||
	(length > _MAX_KEY_LEN_)) {
	return SOC_E_PARAM;
    }

    bpm = -1;
    if (trie->trie) {
        rv = _trie_find_bpm(trie->trie, key, length, &bpm);
	if (SOC_SUCCESS(rv)) {
            /* all bpm bits are 0 */
            *bpm_pfx_len = (bpm < 0)? 0:(unsigned int)bpm;
        }
    }

    return rv;
}

int trie_skip_node_alloc(trie_node_t **node, 
                         unsigned int *key, 
                         /* bpm bit map if bpm management is required, passing null skips bpm management */
                         unsigned int *bpm, 
                         unsigned int msb, /* NOTE: valid msb position 1 based, 0 means skip0/0 node */
                         unsigned int skip_len,
                         trie_node_t *payload,
                         unsigned int count) /* payload count underneath - mostly 1 except some tricky cases */
{
    int lsb=0, msbpos=0, lsbpos=0, bit=0, index;
    trie_node_t *child = NULL, *skip_node = NULL;


    lsb = ((msb)? msb + 1 - skip_len : msb);

    assert(((int)msb >= 0) && (lsb >= 0));

    if (!node || !key || !payload || msb > _MAX_KEY_LEN_ || msb < skip_len) return SOC_E_PARAM;

    if (msb) {
        for (index = BITS2SKIPOFF(lsb), lsbpos = lsb - 1; index <= BITS2SKIPOFF(msb); index++) {

            if (lsbpos == lsb-1) {
                skip_node = payload;
            } else {
                skip_node = sal_alloc(sizeof(trie_node_t), "trie_node");
            }

            sal_memset(skip_node, 0, sizeof(trie_node_t));

            msbpos = index * _MAX_SKIP_LEN_ - 1;
            if (msbpos > msb-1) msbpos = msb-1;

            if (msbpos - lsbpos < _MAX_SKIP_LEN_) {
                skip_node->skip_len = msbpos - lsbpos + 1;
            } else {
                skip_node->skip_len = _MAX_SKIP_LEN_;
            }

            /* skip might be skipping bits on 2 different words 
             * if msb & lsb spawns 2 word boundary in worst case */

            if (BITS2WORDS(msbpos+1) != BITS2WORDS(lsbpos+1)) {
                /* pull snippets from the different words & fuse */
                skip_node->skip_addr = key[KEY_BIT2IDX(msbpos+1)] & MASK((msbpos+1) % _NUM_WORD_BITS_); 
                skip_node->skip_addr = SHL(skip_node->skip_addr, 
                                           skip_node->skip_len - ((msbpos+1) % _NUM_WORD_BITS_),
                                           _NUM_WORD_BITS_);
                skip_node->skip_addr |= SHR(key[KEY_BIT2IDX(lsbpos+1)],(lsbpos % _NUM_WORD_BITS_),_NUM_WORD_BITS_);
            } else {
                skip_node->skip_addr = SHR(key[KEY_BIT2IDX(msbpos+1)], (lsbpos % _NUM_WORD_BITS_),_NUM_WORD_BITS_);
            }

            if (child) { /* set child pointer */
                skip_node->child[bit].child_node = child;
            }

            bit = (skip_node->skip_addr & SHL(1, skip_node->skip_len - 1,_MAX_SKIP_LEN_)) ? 1:0;

            if (lsbpos == lsb-1) {
                skip_node->type = PAYLOAD;

            } else {
                skip_node->type = INTERNAL;
            }

            skip_node->count = count;

            /* advance lsb to next word */
            lsbpos += skip_node->skip_len;

            if (bpm) {
                skip_node->bpm = _key_get_bits(bpm, lsbpos, skip_node->skip_len, 0);
            }
            
            /* for all child nodes 0/1 is implicitly obsorbed on parent */
            if (msbpos != msb-1) skip_node->skip_len--;
            

            skip_node->skip_addr &= MASK(skip_node->skip_len);
            child = skip_node;
        } 
    } else {
        skip_node = payload;
        sal_memset(skip_node, 0, sizeof(trie_node_t));  
        skip_node->type = PAYLOAD;   
        skip_node->count = count;
        if (bpm) {
            skip_node->bpm =  _key_get_bits(bpm,1,1,0);
        }
    }

    *node = skip_node;
    return SOC_E_NONE;
}

#define _CLONE_TRIE_NODE_(dest,src) sal_memcpy((dest),(src),sizeof(trie_node_t))

int _trie_insert(trie_node_t *trie, 
                 unsigned int *key, 
                 /* bpm bit map if bpm management is required, passing null skips bpm management */
                 unsigned int *bpm, 
                 unsigned int length,
                 trie_node_t *payload, /* payload node */
                 trie_node_t **child /* child pointer if the child is modified */)
{
    unsigned int lcp;
    int rv=SOC_E_NONE, bit=0;
    trie_node_t *node = NULL;

    if (!trie || !payload || !child || length > _MAX_KEY_LEN_) return SOC_E_PARAM;

    *child = NULL;

    
    lcp = lcplen(key, length, trie->skip_addr, trie->skip_len);

    /* insert cases:
     * 1 - new key could be the parent of existing node
     * 2 - new node could become the child of a existing node
     * 3 - internal node could be inserted and the key becomes one of child 
     * 4 - internal node is converted to a payload node */

    /* if the new key qualifies as new root do the inserts here */
    if (lcp == length) { /* guaranteed: length < _MAX_SKIP_LEN_ */
        if (trie->skip_len == lcp) {
            if (trie->type != INTERNAL) {
                /* duplicate */ 
                return SOC_E_EXISTS;
            } else { 
                /* change the internal node to payload node */
                _CLONE_TRIE_NODE_(payload,trie);
                sal_free(trie);
                payload->type = PAYLOAD;
                payload->count++;
                *child = payload;

                if (bpm) {
                    /* bpm at this internal mode must be same as the inserted pivot */
                    payload->bpm |= _key_get_bits(bpm, lcp+1, lcp+1, 0);
                    /* implicity preserve the previous bpm & set bit 0 -myself bit */
                } 
                return SOC_E_NONE;
            }
        } else { /* skip length can never be less than lcp implcitly here */
            /* this node is new parent for the old trie node */
            /* lcp is the new skip length */
            _CLONE_TRIE_NODE_(payload,trie);
            *child = payload;

            bit = (trie->skip_addr & SHL(1,trie->skip_len - length - 1,_MAX_SKIP_LEN_)) ? 1 : 0;
            trie->skip_addr &= MASK(trie->skip_len - length - 1);
            trie->skip_len  -= (length + 1);   
 
            if (bpm) {
                trie->bpm &= MASK(trie->skip_len+1);   
            }

            payload->skip_addr = (length > 0) ? key[KEY_BIT2IDX(length)] : 0;
            payload->skip_addr &= MASK(length);
            payload->skip_len  = length;
            payload->child[bit].child_node = trie;
            payload->child[!bit].child_node = NULL;
            payload->type = PAYLOAD;
            payload->count++;

            if (bpm) {
                payload->bpm = SHR(payload->bpm, trie->skip_len + 1,_NUM_WORD_BITS_);
                payload->bpm |= _key_get_bits(bpm, payload->skip_len+1, payload->skip_len+1, 0);
            }
        }
    } else if (lcp == trie->skip_len) {
        /* key length is implictly greater than lcp here */
        /* decide based on key's next applicable bit */
        bit = (key[KEY_BIT2IDX(length-lcp)] & 
               SHL(1,(length - lcp - 1) % _NUM_WORD_BITS_, _NUM_WORD_BITS_)) ? 1:0;

        if (!trie->child[bit].child_node) {
            /* the key is going to be one of the child of existing node */
            /* should be the child */
            rv = trie_skip_node_alloc(&node, key, bpm,
                                      length-lcp-1, /* 0 based msbit position */
                                      length-lcp-1,
                                      payload, 1);
            if (SOC_SUCCESS(rv)) {
                trie->child[bit].child_node = node;
                trie->count++;
            } else {
                soc_cm_print("\n Error on trie skip node allocaiton [%d]!!!!\n", rv);
            }
        } else { 
            rv = _trie_insert(trie->child[bit].child_node, 
                              key, bpm, length - lcp - 1, 
                              payload, child);
            if (SOC_SUCCESS(rv)) {
                trie->count++;
                if (*child != NULL) { /* chande the old child pointer to new child */
                    trie->child[bit].child_node = *child;
                    *child = NULL;
                }
            }
        }
    } else {
        trie_node_t *newchild = NULL;

        /* need to introduce internal nodes */
        node = sal_alloc(sizeof(trie_node_t), "trie-node");
        _CLONE_TRIE_NODE_(node, trie);

        rv = trie_skip_node_alloc(&newchild, key, bpm,
                                  ((lcp)?length-lcp-1:length-1),
                                  length - lcp - 1,
                                  payload, 1);
        if (SOC_SUCCESS(rv)) {
            bit = (key[KEY_BIT2IDX(length-lcp)] & 
                   SHL(1,(length - lcp - 1) % _NUM_WORD_BITS_,_NUM_WORD_BITS_)) ? 1: 0;

            node->child[!bit].child_node = trie;
            node->child[bit].child_node = newchild;
            node->type = INTERNAL;
            node->skip_addr = SHR(trie->skip_addr,trie->skip_len - lcp,_MAX_SKIP_LEN_);
            node->skip_len = lcp;
            node->count++;
            if (bpm) {
                node->bpm = SHR(node->bpm, trie->skip_len - lcp, _MAX_SKIP_LEN_);
            }
            *child = node;
            
            trie->skip_addr &= MASK(trie->skip_len - lcp - 1);
            trie->skip_len  -= (lcp + 1); 
            if (bpm) {
                trie->bpm &= MASK(trie->skip_len+1);      
            }
        } else {
            soc_cm_print("\n Error on trie skip node allocaiton [%d]!!!!\n", rv);
            sal_free(node);
        }
    }

    return rv;
}

/*
 * Function:
 *     trie_insert
 * Purpose:
 *     Inserts provided prefix/length in to the trie
 */
int trie_insert(trie_t *trie, 
                unsigned int *key, 
                unsigned int *bpm,
                unsigned int length, 
                trie_node_t *payload)
{
    int rv = SOC_E_NONE;
    trie_node_t *child=NULL;

    if (!trie || length > _MAX_KEY_LEN_) return SOC_E_PARAM;

    if (trie->trie == NULL) {
        rv = trie_skip_node_alloc(&trie->trie, key, bpm, length, length, payload, 1);
    } else {
        rv = _trie_insert(trie->trie, key, bpm, length, payload, &child);
        if (child) { /* chande the old child pointer to new child */
            trie->trie = child;
        }
    }

    return rv;
}

int _trie_fuse_child(trie_node_t *trie, int bit)
{
    trie_node_t *child = NULL;
    int rv = SOC_E_NONE;

    if (trie->child[0].child_node && trie->child[1].child_node) {
        return SOC_E_PARAM;
    } 

    bit = (bit > 0)?1:0;
    child = trie->child[bit].child_node;

    if (child == NULL) {
        return SOC_E_PARAM;
    } else {
        if (trie->skip_len + child->skip_len + 1 <= _MAX_SKIP_LEN_) {

            if (trie->skip_len == 0) trie->skip_addr = 0; 

            if (child->skip_len < _MAX_SKIP_LEN_) {
                trie->skip_addr = SHL(trie->skip_addr,child->skip_len + 1,_MAX_SKIP_LEN_);
            }

            trie->skip_addr  |= SHL(bit,child->skip_len,_MAX_SKIP_LEN_);
            child->skip_addr |= trie->skip_addr;
            child->bpm       |= SHL(trie->bpm,child->skip_len+1,_MAX_SKIP_LEN_); 
            child->skip_len  += trie->skip_len + 1;

            /* do not free payload nodes as they are user managed */
            if (trie->type == INTERNAL) {
                sal_free(trie);
            }
        }
    }

    return rv;
}

int _trie_delete(trie_node_t *trie, 
                 unsigned int *key,
                 unsigned int length,
                 trie_node_t **payload,
                 trie_node_t **child)
{
    unsigned int lcp;
    int rv=SOC_E_NONE, bit=0;
    trie_node_t *node = NULL;

    /* our algorithm should return before the length < 0, so this means
     * something wrong with the trie structure. Internal error?
     */
    if (!trie || !payload || !child || (length > _MAX_KEY_LEN_)) {
	return SOC_E_PARAM;
    }

    *child = NULL;

    /* check a section of key, return the number of matched bits and value of next bit */
    lcp = lcplen(key, length, trie->skip_addr, trie->skip_len);

    if (length > trie->skip_len) {

        if (lcp == trie->skip_len) {

            bit = (key[KEY_BIT2IDX(length-lcp)] & 
                   SHL(1,(length - lcp -1) % _NUM_WORD_BITS_,_NUM_WORD_BITS_)) ? 1:0;

            /* based on next bit branch left or right */
            if (trie->child[bit].child_node) {

	        /* has child node, keep searching */
                rv = _trie_delete(trie->child[bit].child_node, key, length - lcp - 1, payload, child);

	        if (rv == SOC_E_BUSY) {

                    trie->child[bit].child_node = NULL; /* sal_free the child */
                    rv = SOC_E_NONE;
                    trie->count--;

                    if (trie->type == INTERNAL) {

                        bit = (bit==0)?1:0;

                        if (trie->child[bit].child_node == NULL) {
                            /* parent and child connected, sal_free the middle-node itself */
                            sal_free(trie);
                            rv = SOC_E_BUSY;
                        } else {
                            /* fuse the parent & child */
                            if (trie->skip_len + trie->child[bit].child_node->skip_len + 1 <= 
                                _MAX_SKIP_LEN_) {
                                *child = trie->child[bit].child_node;
                                rv = _trie_fuse_child(trie, bit);
                                if (rv != SOC_E_NONE) {
                                    *child = NULL;
                                }
                            }
                        }
                    }
	        } else if (SOC_SUCCESS(rv)) {
                    trie->count--;
                    /* update child pointer if applicable */
                    if (*child != NULL) {
                        trie->child[bit].child_node = *child;
                        *child = NULL;
                    }
                }
            } else {
                /* no child node case 0: not found */
                rv = SOC_E_NOT_FOUND; 
            }

        } else { 
	    /* some bits are not matching, case 0: not found */
            rv = SOC_E_NOT_FOUND;
        }
    } else if (length == trie->skip_len) {
	/* when length equal to skip_len, unless this is a payload node
	 * and it's an exact match (lcp == length), we can not found a match
	 */ 
        if (!((lcp == length) && (trie->type == PAYLOAD))) {
	    rv = SOC_E_NOT_FOUND;
	} else {
            /* payload node can be deleted */
            /* if this node has 2 children update it to internal node */
            rv = SOC_E_NONE;

            if (trie->child[0].child_node && trie->child[1].child_node ) {
                node = sal_alloc(sizeof(trie_node_t), "trie_node");
                _CLONE_TRIE_NODE_(node, trie);
                node->type = INTERNAL;
                node->count--;
                *child = node;
            } else if (trie->child[0].child_node || trie->child[1].child_node ) {
                /* if this node has 1 children fuse the children with this node */
                bit = (trie->child[0].child_node) ? 0:1;
                trie->count--;
                if (trie->skip_len + trie->child[bit].child_node->skip_len + 1 <= _MAX_SKIP_LEN_) {
                    *child = trie->child[bit].child_node;
                    rv = _trie_fuse_child(trie, bit);
                    if (rv != SOC_E_NONE) {
                        *child = NULL;
                    }
                } else {
                    node = sal_alloc(sizeof(trie_node_t), "trie_node");
                    _CLONE_TRIE_NODE_(node, trie);
                    node->type = INTERNAL;
                    node->count--;
                    *child = node;
                }
            } else {
                rv = SOC_E_BUSY;
            }

            *payload = trie;
        }
    } else {
	/* key length is shorter, no match if it's internal node,
	 * will not exact match even if this is a payload node
	 */
        rv = SOC_E_NOT_FOUND; /* case 0: not found */        
    }

    return rv;
}

/*
 * Function:
 *     trie_delete
 * Purpose:
 *     Deletes provided prefix/length in to the trie
 */
int trie_delete(trie_t *trie,
                unsigned int *key,
                unsigned int length,
                trie_node_t **payload)
{
    int rv = SOC_E_NONE;
    trie_node_t *child = NULL;

    if (trie->trie) {
        rv = _trie_delete(trie->trie, key, length, payload, &child);
        if (rv == SOC_E_BUSY) {
            /* the head node of trie was deleted, reset trie pointer to null */
            trie->trie = NULL;
            rv = SOC_E_NONE;
        } else if (rv == SOC_E_NONE && child != NULL) {
            trie->trie = child;
        }
    } else {
        rv = SOC_E_NOT_FOUND;
    }
    return rv;
}


/*
 * Function:
 *     trie_split
 * Purpose:
 *     Split the trie into 2 based on optimum pivot
 */
int _trie_split(trie_node_t  *trie,
                unsigned int *pivot,
                unsigned int *length,
                unsigned int *split_count,
                trie_node_t **split_node,
                trie_node_t **child,
                const unsigned int max_count,
                unsigned int *bpm)
{
    int bit=0, rv=SOC_E_NONE;

    if (!trie || !pivot || !length || !split_node || max_count == 0) return SOC_E_PARAM;

    if (trie->child[0].child_node && trie->child[1].child_node) {
        bit = (trie->child[0].child_node->count > 
               trie->child[1].child_node->count) ? 0:1;
    } else {
        bit = (trie->child[0].child_node)?0:1;
    }

    /* start building the pivot */
    rv = _key_append(pivot, length, trie->skip_addr, trie->skip_len);
    if (SOC_FAILURE(rv)) return rv;

    if (bpm) {
        unsigned int scratch=0;
        rv = _key_append(bpm, &scratch, trie->bpm, trie->skip_len+1);
        if (SOC_FAILURE(rv)) return rv;        
    }

    /* child is a better split point */
    if (ABS(trie->child[bit].child_node->count - 
            (max_count - trie->child[bit].child_node->count)) <=
        ABS(trie->count - (max_count - trie->count))) {

        rv = _key_append(pivot, length, bit, 1);
        if (SOC_FAILURE(rv)) return rv;

        rv = _trie_split(trie->child[bit].child_node, 
                         pivot, length,
                         split_count, split_node, 
                         child, max_count, 
                         bpm);
    } else {
        *split_node = trie;
        *split_count = trie->count;
        return SOC_E_EXISTS;
    }

    /* free up internal nodes if applicable */
    if (rv == SOC_E_EXISTS) {
        if (trie->count == *split_count) {
            /* if the split point has associate internal nodes they have to
             * be cleaned up */  
            assert(trie->type == INTERNAL);
            assert(!(trie->child[0].child_node && trie->child[1].child_node));
            sal_free(trie);
        } else {
            assert(*child == NULL);
            /* fuse with child if possible */
            trie->child[bit].child_node = NULL; 
            bit = (bit==0)?1:0;
            trie->count -= *split_count; 

            /* optimize more */
            if ((trie->type == INTERNAL) && 
                (trie->skip_len + 
                 trie->child[bit].child_node->skip_len + 1 <= _MAX_SKIP_LEN_)) {
                *child = trie->child[bit].child_node;
                rv = _trie_fuse_child(trie, bit);
                if (rv != SOC_E_NONE) {
                    *child = NULL;
                }
            }
            rv = SOC_E_BUSY;
        }
    } else if (rv == SOC_E_BUSY) {
        /* adjust parent's count */     
        assert(*split_count > 0);
        assert(trie->count >= *split_count);

        /* update the child pointer if child was pruned */
        if (*child != NULL) {
            trie->child[bit].child_node = *child;
            *child = NULL;
        } 
        trie->count -= *split_count;
    }

    return rv;
}

/*
 * Function:
 *     trie_split
 * Purpose:
 *     Split the trie into 2 based on optimum pivot
 */
int trie_split(trie_t *trie,
               unsigned int *pivot,
               unsigned int *length,
               trie_node_t **split_trie_root,
               unsigned int *bpm)
{
    int rv = SOC_E_NONE;
    unsigned int split_count=0;
    trie_node_t *child = NULL, *node=NULL, clone;

    if (!trie || !pivot || !length || !split_trie_root) return SOC_E_PARAM;

    sal_memset(pivot, 0, sizeof(unsigned int) * _MAX_KEY_WORDS_);
    if (bpm) {
        sal_memset(bpm, 0, sizeof(unsigned int) * _MAX_KEY_WORDS_);
    }
    *length = 0;

    if (trie->trie) {
        rv = _trie_split(trie->trie, pivot, length, 
                         &split_count, split_trie_root,
                         &child, trie->trie->count, bpm);
        if (rv == SOC_E_BUSY) {
            /* adjust parent's count */     
            assert(split_count > 0);
            assert(trie->trie->count >= split_count || (*split_trie_root)->count >= split_count);
            /* update the child pointer if child was pruned */
            if (child != NULL) {
                trie->trie = child;
            }

            sal_memcpy(&clone, *split_trie_root, sizeof(trie_node_t));
            child = *split_trie_root;

            /* take advantage of thie function by passing in internal or payload node whatever
             * is the new root. If internal the function assumed it as payload node & changes type.
             * But this method is efficient to reuse the last internal or payload node possible to 
             * implant the new pivot */
            rv = trie_skip_node_alloc(&node, pivot, NULL,
                                      *length, *length,
                                      child, child->count);
            if (SOC_SUCCESS(rv)) {
                if (clone.type == INTERNAL) {
                    child->type = INTERNAL; /* since skip alloc would have reset it to payload */
                }
                child->child[0].child_node = clone.child[0].child_node;
                child->child[1].child_node = clone.child[1].child_node;
                *split_trie_root = node;
            } 
        } else {
            assert(0);
        }
    } else {
        rv = SOC_E_PARAM;
    }

    return rv;
}

/*
 * Function:
 *     trie_unsplit
 * Purpose:
 *     unsplit or fuse the child trie with parent trie
 */
int trie_unsplit(trie_t *parent_trie,
                 trie_t *child_trie,
                 unsigned int *child_pivot,
                 unsigned int length)
{
    int rv=SOC_E_NONE;

    if (!parent_trie || !child_trie) return SOC_E_PARAM;

    
    assert(0);

    return rv;
}

/*
 * Function:
 *     _trie_traverse_propagate_prefix
 * Purpose:
 *     calls back applicable payload object is affected by prefix updates 
 * NOTE:
 *     propagation stops once any callback funciton return something otherthan SOC_E_NONE
 *     tcam propagation code should return !SOC_E_NONE so that callback only happen once.
 * 
 *     other propagation code should always return SOC_E_NONE so that callback will
 *     happen on all pivot.
 */
int _trie_traverse_propagate_prefix(trie_node_t *trie,
                                    trie_propagate_cb_f cb,
                                    trie_bpm_cb_info_t *cb_info,
                                    unsigned int mask)
{
    int rv = SOC_E_NONE, index=0;

    if (!trie || !cb || !cb_info) return SOC_E_PARAM;

    if ((trie->bpm & mask) == 0) {
        /* call back the payload object if applicable */
        if (PAYLOAD == trie->type) {
            rv = cb(trie, cb_info);
	    if (SOC_FAILURE(rv)) {
		/* callback stops once any callback function not returning SOC_E_NONE */
		return rv;
	    }
        }

        for (index=0; index < _MAX_CHILD_ && SOC_SUCCESS(rv); index++) {
            if (trie->child[index].child_node && trie->child[index].child_node->bpm == 0) {
                rv = _trie_traverse_propagate_prefix(trie->child[index].child_node,
                                                     cb, cb_info, MASK(32));
            }

	    if (SOC_FAILURE(rv)) {
		/* callback stops once any callback function not returning SOC_E_NONE */
		return rv;
	    }
        }
    }

    return rv;
}

/*
 * Function:
 *     _trie_propagate_prefix
 * Purpose:
 *  Propogate prefix BPM. If the propogation starts from intermediate pivot on
 *  the trie, then the prefix length has to be appropriately adjusted or else 
 *  it will end up with ill updates. 
 *  Assumption: the prefix length is adjusted as per trie node on which is starts from.
 *  If node == head node then adjust is none
 *     node == pivot, then prefix length = org len - pivot len          
 */
int _trie_propagate_prefix(trie_node_t *trie,
                           unsigned int *pfx,
                           unsigned int len,
                           unsigned int add, /* 0-del/1-add */
                           trie_propagate_cb_f cb,
                           trie_bpm_cb_info_t *cb_info)
{
    int rv = SOC_E_NONE; /*, index;*/
    unsigned int bit=0, lcp=0;

    if (!pfx || !trie || len > _MAX_KEY_LEN_ || !cb || !cb_info) return SOC_E_PARAM;

    if (len > 0) {
        /* should not hit but defensive check */
        

        /* BPM bit maps has to be updated before propagation */
        lcp = lcplen(pfx, len, trie->skip_addr, trie->skip_len);            
        /* if the lcp is less than prefix length the prefix is not applicable
         * for any propagation */
        if (lcp < ((len>trie->skip_len)?trie->skip_len:len)) {
            return SOC_E_NONE; 
        } else { 
            if (len > trie->skip_len) {
                bit = _key_get_bits(pfx, len-lcp, 1, 0);
                if (!trie->child[bit].child_node) return SOC_E_NONE;
                rv = _trie_propagate_prefix(trie->child[bit].child_node,
                                            pfx, len-lcp-1, add, cb, cb_info);
            } else {
                /* pfx is <= trie skip len */
                if (!add) { /* delete */
                    _BITCLR(trie->bpm, trie->skip_len - len);
                }
                
                /* update bit map and propagate if applicable */
                if ((trie->bpm & MASK(trie->skip_len - len)) == 0) {
                    rv = _trie_traverse_propagate_prefix(trie, cb, 
                                                         cb_info, 
                                                         MASK(trie->skip_len - len));
                    if (SOC_E_LIMIT == rv) rv = SOC_E_NONE;
                }
                
                if (add && SOC_SUCCESS(rv)) {
                    /* this is the case where child bit is the new prefix */
                    _BITSET(trie->bpm, trie->skip_len - len);
                }
            }
        }
    } else {

        if (!add) { /* delete */
            _BITCLR(trie->bpm, trie->skip_len);
        }

        if (trie->bpm == 0) {
            rv = _trie_traverse_propagate_prefix(trie, cb, cb_info, MASK(32));
            if (SOC_E_LIMIT == rv) rv = SOC_E_NONE;
        }
        
        if (add && SOC_SUCCESS(rv)) { /* add */
            /* this is the case where child bit is the new prefix */
            _BITSET(trie->bpm, trie->skip_len);
        }
    }

    return rv;
}

/*
 * Function:
 *     _trie_propagate_prefix_validate
 * Purpose:
 *  validate that the provided prefix is valid for propagation.
 *  The added prefix which was member of a shorter pivot's domain 
 *  must never be more specific than another pivot encounter if any
 *  in the path
 */
int _trie_propagate_prefix_validate(trie_node_t *trie,
                                    unsigned int *pfx,
                                    unsigned int len)
{
    unsigned int lcp=0, bit=0;

    if (!trie || !pfx) return SOC_E_PARAM;

    if (len == 0) return SOC_E_NONE;

    lcp = lcplen(pfx, len, trie->skip_addr, trie->skip_len);

    if (lcp == trie->skip_len) {
        if (PAYLOAD == trie->type) return SOC_E_PARAM;
        bit = _key_get_bits(pfx, len-lcp, 1, 0);
        if (!trie->child[bit].child_node) return SOC_E_NONE;
        return _trie_propagate_prefix_validate(trie->child[bit].child_node,
                                               pfx, len-lcp);
    } 

    return SOC_E_NONE;
}

int _trie_init_propagate_info(unsigned int *pfx,
                              unsigned int len,
                              trie_propagate_cb_f cb,
                              trie_bpm_cb_info_t *cb_info)
{
    cb_info->pfx = pfx;
    cb_info->len = len;
    return SOC_E_NONE;
}

/*
 * Function:
 *     trie_pivot_propagate_prefix
 * Purpose:
 *  Propogate prefix BPM from a given pivot.      
 */
int trie_pivot_propagate_prefix(trie_node_t *pivot,
                                unsigned int pivot_len,
                                unsigned int *pfx,
                                unsigned int len,
                                unsigned int add, /* 0-del/1-add */
                                trie_propagate_cb_f cb,
                                trie_bpm_cb_info_t *cb_info)
{
    int rv = SOC_E_NONE;

    if (!pfx || !pivot || len > _MAX_KEY_LEN_ ||
        pivot_len >  _MAX_KEY_LEN_ || len < pivot_len ||
        pivot->type != PAYLOAD || !cb || !cb_info ||
        !cb_info->pfx) return SOC_E_PARAM;

    _trie_init_propagate_info(pfx,len,cb,cb_info);
    len -= pivot_len;

    if (len > 0) {
        unsigned int bit =  _key_get_bits(pfx, len, 1, 0);

        if (pivot->child[bit].child_node) {
            /* validate if the pivot provided is correct */
            rv = _trie_propagate_prefix_validate(pivot->child[bit].child_node,
                                                     pfx, len-1);
            if (SOC_SUCCESS(rv)) {
                rv = _trie_propagate_prefix(pivot->child[bit].child_node,
                                            pfx, len-1,
                                            add, cb, cb_info);
            }
        } /* else nop, nothing to propagate on this path end */
    } else {
        /* pivot == prefix */
        rv = _trie_propagate_prefix(pivot, pfx, len,
                                    add, cb, cb_info);
    }

    return rv;
}

/*
 * Function:
 *     trie_propagate_prefix
 * Purpose:
 *  Propogate prefix BPM on a given trie.      
 */
int trie_propagate_prefix(trie_t *trie,
                          unsigned int *pfx,
                          unsigned int len,
                          unsigned int add, /* 0-del/1-add */
                          trie_propagate_cb_f cb,
                          trie_bpm_cb_info_t *cb_info)
{
    int rv=SOC_E_NONE;

    if (!pfx || !trie || !trie->trie || len > _MAX_KEY_LEN_ ||
        !cb || !cb_info || !cb_info->pfx) return SOC_E_PARAM;

    _trie_init_propagate_info(pfx,len,cb,cb_info);

    if (SOC_SUCCESS(rv)) {
        rv = _trie_propagate_prefix(trie->trie, pfx, len, add, 
                                    cb, cb_info);
    }

    return rv;
}


/*
 * Function:
 *     trie_init
 * Purpose:
 *     allocates a trie & initializes it
 */
int trie_init(trie_t **ptrie)
{
    trie_t *trie = sal_alloc(sizeof(trie_t), "trie-node");
    sal_memset(trie, 0, sizeof(trie_t));
    trie->trie = NULL; /* means nothing is on trie */
    *ptrie = trie;
    return SOC_E_NONE;
}

/*
 * Function:
 *     trie_destroy
 * Purpose:
 *     destroys a trie 
 */
int trie_destroy(trie_t *trie)
{
    
    sal_free(trie);
    return SOC_E_NONE;
}

#if 0
/*
 *
 * Function:
 *    _trie_util_get_lsb
 * Input:
 *     max_mask_size  -- number of bits in the mask
 *                      ipv4 == 48
 *                      ipv4 == 144
 *     mask  -- uint32 array head.
 *              for ipv4. Mask[0].bit15-0 is mask bits 47-32
 *                        Mask[1] is mask bits 31-0
 *              for ipv6. Mask[0].bit15-0 is mask bits 143-128
 *                        Mask[1-4] is mask bits 127-0
 *     lsb   -- -1 if no bit is set. bit position of the least
 *              significant bit that is set to 1
 * Purpose:
 *     get the bit position of the least significant bit that is set in mask.
 *     for example:
 *         for ipv4, max_mask_size == 48
 *         mask[0]=0, mask[1]=0x30 will return lsb=4
 *         mask[0]=3, mask[1]=0 will return lsb=32
 */
static int _trie_util_get_lsb(uint32 max_mask_size, uint32 *mask, int32 *lsb)
{
    int word_idx, bit_idx;

    if (!mask || !lsb) {
	return SOC_E_PARAM;
    }

    *lsb = -1;
    for (word_idx = (BITS2WORDS(max_mask_size)-1); word_idx >=0; word_idx--) {
	if (mask[word_idx]!=0) {
	    for (bit_idx=0; bit_idx<32; bit_idx++) {
		if ((mask[word_idx] & (1<<bit_idx)) &&
		    (((BITS2WORDS(max_mask_size) - 1 - word_idx) * 32 + bit_idx) < max_mask_size)) {
		    *lsb = (BITS2WORDS(max_mask_size) - 1 - word_idx) * 32 + bit_idx;
		    return SOC_E_NONE;
		}
	    }
	}
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *     trie_util_get_bpm_pfx
 * Purpose:
 *     finds best prefix match length given a bpm bitmap & key
 */
int trie_util_get_bpm_pfx(unsigned int *bpm, 
                          unsigned int key_len,
                          /* OUT */
                          unsigned int *pfx_len)
{
    int rv = SOC_E_NONE, pos=0;

    if (!bpm || !pfx_len || key_len > _MAX_KEY_LEN_) {
        return SOC_E_PARAM;
    }

    *pfx_len = 0;

    rv = _trie_util_get_lsb(_MAX_KEY_LEN_, bpm, &pos);
    if (SOC_SUCCESS(rv)) {
        *pfx_len = (pos < 0)?0:(key_len - pos);
    }
    return rv;
}
#endif

#if UNIT_TESTS
#include <soc/sbx/sbDq.h>
/**********************************************/
/****************/
/** unit tests **/
/****************/
#define _NUM_KEY_ (4 * 1024)
#define _VRF_LEN_ 16
/*#define VERBOSE 
  #define LOG*/
/* use the followign diag shell command to run this test:
 * tr c3sw test=tmu_trie_ut
 */
typedef struct _payload_s {
    trie_node_t node; /*trie node */
    dq_t        listnode; /* list node */
    union {
        trie_t      *trie;
        trie_node_t pfx_trie_node;
    } info;
    unsigned int key[BITS2WORDS(_MAX_KEY_LEN_)];
    unsigned int len;
} payload_t;

int ut_print_payload_node(trie_node_t *payload, void *datum)
{
    payload_t *pyld;

    if (payload && payload->type == PAYLOAD) {
        pyld = TRIE_ELEMENT_GET(payload_t*, payload, node);
        soc_cm_print(" key[0x%08x:0x%08x] Length:%d \n",
                     pyld->key[0], pyld->key[1], pyld->len);
    }
    return SOC_E_NONE;
}

int ut_print_prefix_payload_node(trie_node_t *payload, void *datum)
{
    payload_t *pyld;

    if (payload && payload->type == PAYLOAD) {
        pyld = TRIE_ELEMENT_GET(payload_t*, payload, info.pfx_trie_node);
        soc_cm_print(" key[0x%08x:0x%08x] Length:%d \n",
                     pyld->key[0], pyld->key[1], pyld->len);
    }
    return SOC_E_NONE;
}

int ut_check_duplicate(payload_t *pyld, int pyld_vector_size)
{
    int i=0;

    assert(pyld);

    for (i=0; i < pyld_vector_size; i++) {
        if (pyld[i].len == pyld[pyld_vector_size].len &&
            pyld[i].key[0] == pyld[pyld_vector_size].key[0] && 
            pyld[i].key[1] == pyld[pyld_vector_size].key[1]) {
            break;
        }
    }

    return ((i == pyld_vector_size)?0:1);
}


int tmu_taps_util_get_bpm_pfx_ut(void) 
{
    int rv;
    unsigned int pfx_len=0;
    unsigned int bpm[] = { 0x80, 0x00101010 }; /* 40th bit msb */

    rv = trie_util_get_bpm_pfx(&bpm[0], 48, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 48-4) return SOC_E_FAIL;

    bpm[0] = 0;
    bpm[1] = 0x00010000;
    rv = trie_util_get_bpm_pfx(&bpm[0], 48, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 48-16) return SOC_E_FAIL;

    bpm[0] = 0;
    bpm[1] = 0x80000000;
    rv = trie_util_get_bpm_pfx(&bpm[0], 48, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 48-31) return SOC_E_FAIL;

    bpm[0] = 0x1;
    bpm[1] = 0x80000000;
    rv = trie_util_get_bpm_pfx(&bpm[0], 48, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 48-31) return SOC_E_FAIL;

    bpm[0] = 0;
    bpm[1] = 0;
    rv = trie_util_get_bpm_pfx(&bpm[0], 48, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 0) return SOC_E_FAIL;

    return SOC_E_NONE;
}

int tmu_taps_kshift_ut(void) 
{
    unsigned int key[] = { 0x1234, 0x12345678 }, length=0;

    _key_shift_left(key, 8);

    if (key[0] != 0x3412 && key[1] != 0x34567800) {
        return SOC_E_FAIL;
    }

    key[0] = 0;
    key[1] = 0x12345678;
    _key_shift_left(key, 15);    

    if (key[0] != (0x12345678 >> 17) && key[1] != (0x12345678 << 15)) {
        return SOC_E_FAIL;
    }

    key[0] = 0x1234;
    key[1] = 0xdeadbeef;
    _key_shift_left(key, 0);    
    if (key[0] != 0x1234 && key[1] != 0xdeadbeef) {
        return SOC_E_FAIL;
    }

    key[0] = 0;
    key[1] = 0;
    _key_append(key, &length, 0xba5e, 16);
    if (key[0] != 0 && key[1] != 0xba5e) {
        return SOC_E_FAIL;
    }

    _key_append(key, &length, 0x3a5eba11, 31);
    if (key[0] != 0xba5e && key[1] != 0x3a5eba11) {
        return SOC_E_FAIL;
    }

    return SOC_E_NONE;
}

int tmu_trie_split_ut(unsigned int seed) 
{
    int index, rv = SOC_E_NONE, numkey=0, id=0;
    trie_t *trie, *newtrie;
    trie_node_t *newroot;
    payload_t *pyld = sal_alloc(_NUM_KEY_ * sizeof(payload_t), "unit-test");
    trie_node_t *pyldptr = NULL;
    unsigned int pivot[_MAX_KEY_WORDS_], length;

    for (id=0; id < 4; id++) {
        switch(id) {
        case 0:  /* 1:1 split */
            pyld[0].key[0] = 0; pyld[0].key[1] = 0x10; pyld[0].len = _VRF_LEN_ + 8;  /* v=0 p=0x10000000/8  */
            pyld[1].key[0] = 0; pyld[1].key[1] = 0x1000; pyld[1].len = _VRF_LEN_ + 16; /* v=0 p=0x10000000/16 */
            pyld[2].key[0] = 0; pyld[2].key[1] = 0x100000; pyld[2].len = _VRF_LEN_ + 24; /* v=0 p=0x10000000/24 */
            pyld[3].key[0] = 0; pyld[3].key[1] = 0x10000000; pyld[3].len = _VRF_LEN_ + 32; /* v=0 p=0x10000000/48 */
            numkey = 4;
            break;
        case 1: /* 1:1 split */
            pyld[0].key[0] = 0; pyld[0].key[1] = 0x10000000; pyld[0].len = _VRF_LEN_ + 32; /* v=0 p=0x10000000/32 */
            pyld[1].key[0] = 0; pyld[1].key[1] = 0x10000001; pyld[1].len = _VRF_LEN_ + 32; /* v=0 p=0x10000001/32 */
            pyld[2].key[0] = 0; pyld[2].key[1] = 0x10000002; pyld[2].len = _VRF_LEN_ + 32; /* v=0 p=0x10000002/32 */
            pyld[3].key[0] = 0; pyld[3].key[1] = 0x10000003; pyld[3].len = _VRF_LEN_ + 32; /* v=0 p=0x10000002/32 */
            pyld[4].key[0] = 0; pyld[4].key[1] = 0x10000004; pyld[4].len = _VRF_LEN_ + 32; /* v=0 p=0x10000002/32 */
            pyld[5].key[0] = 0; pyld[5].key[1] = 0x10000005; pyld[5].len = _VRF_LEN_ + 32; /* v=0 p=0x10000002/32 */
            numkey = 6;
            break;
        case 2: /* 2:5 split */
            pyld[0].key[0] = 0; pyld[0].key[1] = 0x100; pyld[0].len = _VRF_LEN_ + 12;
            pyld[1].key[0] = 0; pyld[1].key[1] = 0x1011; pyld[1].len = _VRF_LEN_ + 16;
            pyld[2].key[0] = 0; pyld[2].key[1] = 0x100000; pyld[2].len = _VRF_LEN_ + 24; 
            pyld[3].key[0] = 0; pyld[3].key[1] = 0x1000000; pyld[3].len = _VRF_LEN_ + 28;
            pyld[4].key[0] = 0; pyld[4].key[1] = 0x1001; pyld[4].len = _VRF_LEN_ + 16;
            pyld[5].key[0] = 0; pyld[5].key[1] = 0x10011; pyld[5].len = _VRF_LEN_ + 20;
            numkey = 6;
            break;

        case 3:
        {
            int dup;

            if (seed == 0) {
                seed = sal_time();
                sal_srand(seed);
            }

            index = 0;
            soc_cm_print("Random test: %d Seed: 0x%x \n", id, seed);
            do {
                do {
                    pyld[index].key[1] = (unsigned int) sal_rand();
                    pyld[index].len = (unsigned int)sal_rand() % 32;
                    pyld[index].len += _VRF_LEN_;

                    if (pyld[index].len <= 32) {
                        pyld[index].key[0] = 0;
                        pyld[index].key[1] &= MASK(pyld[index].len);
                    }

                    if (pyld[index].len > 32) {
                        pyld[index].key[0] = (unsigned int)sal_rand() % 16;                        
                        pyld[index].key[0] &= MASK(pyld[index].len-32); 
                    }

                    dup = ut_check_duplicate(pyld, index);
                    if (dup) {                    
                        soc_cm_print("\n Duplicate at index[%d]:key[0x%08x:0x%08x] Retry!!!\n", 
                                     index,pyld[index].key[0],pyld[index].key[1]);
                    }
                } while(dup > 0);
            } while(++index < _NUM_KEY_);

            numkey = index;
       }
       break;

        default:
            return SOC_E_PARAM;
        }

        trie_init(&trie);
        trie_init(&newtrie);

        for(index=0; index < numkey && rv == SOC_E_NONE; index++) {
            rv = trie_insert(trie, &pyld[index].key[0], NULL, pyld[index].len, &pyld[index].node);
        }

        rv = trie_split(trie, pivot, &length, &newroot, NULL);
        if (SOC_SUCCESS(rv)) {
            soc_cm_print("\n Split Trie Pivot: 0x%08x 0x%08x Length: %d Root: %p \n",
                         pivot[0], pivot[1], length, newroot);
            soc_cm_print(" $Payload Count Old Trie:%d New Trie:%d \n", trie->trie->count, newroot->count);

            /* set new trie */
            newtrie->trie = newroot;
#ifdef VERBOSE
            soc_cm_print("\n OLD Trie Dump ############: \n");
            trie_dump(trie, NULL, NULL);
            soc_cm_print("\n SPLIT Trie Dump ############: \n");
            trie_dump(newtrie, NULL, NULL);
#endif
            
            for(index=0; index < numkey && rv == SOC_E_NONE; index++) {
                rv = trie_search(trie, &pyld[index].key[0], pyld[index].len, &pyldptr);
                if (rv != SOC_E_NONE) {
                    rv = trie_search(newtrie, &pyld[index].key[0], pyld[index].len, &pyldptr);
                    if (rv != SOC_E_NONE) {
                        soc_cm_print("SEARCH: Key=0x%x 0x%x len %d SEARCH idx:%d failed on both trie!!!!\n", 
                                     pyld[index].key[0], pyld[index].key[1],
                                     pyld[index].len, index);
                    } else {
                        assert(pyldptr == &pyld[index].node);
                    }
                } 
            }
            
            trie_destroy(trie);
            trie_destroy(newtrie);
        }
    }

    sal_free(pyld);
    return rv;
}

int tmu_taps_trie_ut(int id, unsigned int seed)
{
    int index, rv = SOC_E_NONE, numkey=0, num_deleted=0;
    trie_t *trie;
    payload_t *pyld = sal_alloc(_NUM_KEY_ * sizeof(payload_t), "unit-test");
    trie_node_t *pyldptr = NULL;
    unsigned int result_len=0, result_key[_MAX_KEY_WORDS_];

    /* keys packed right to left (ie) most significant word starts at index 0*/

    switch(id) {
    case 0:
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x10; pyld[0].len = _VRF_LEN_ + 8;  /* v=0 p=0x10000000/8  */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x1000; pyld[1].len = _VRF_LEN_ + 16; /* v=0 p=0x10000000/16 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0x1001; pyld[2].len = _VRF_LEN_ + 16; /* v=0 p=0x10010000/16 */
        pyld[3].key[0] = 0; pyld[3].key[1] = 0x10000000; pyld[3].len = _VRF_LEN_ + 32; /* v=0 p=0x10000000/48 */
        numkey = 4;
        break;

    case 1:
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x123456; pyld[0].len  = _VRF_LEN_ + 24; /* v=0 p=0x12345678/24 */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x246; pyld[1].len = _VRF_LEN_ + 13; /* v=0 p=0x12345678/13 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0x24; pyld[2].len = _VRF_LEN_ + 9; /* v=0 p=0x12345678/9 */
        numkey = 3;
        break;

    case 2: /* dup routes on another vrf */
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x1123456; pyld[0].len = _VRF_LEN_ + 24; /* v=1 p=0x12345678/24 */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x2246; pyld[1].len = _VRF_LEN_ + 13; /* v=1 p=0x12345678/13 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0x224; pyld[2].len = _VRF_LEN_ + 9; /* v=1 p=0x12345678/9 */
        numkey = 3;
        break;

    case 3:
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x10000000; pyld[0].len = _VRF_LEN_ + 32; /* v=0 p=0x10000000/32 */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x10000001; pyld[1].len = _VRF_LEN_ + 32; /* v=0 p=0x10000001/32 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0x10000002; pyld[2].len = _VRF_LEN_ + 32; /* v=0 p=0x10000002/32 */
        numkey = 3;
        break;

    case 4:
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x12345670; pyld[0].len = _VRF_LEN_ + 32; /* v=0 p=0x12345670/32 */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x12345671; pyld[1].len = _VRF_LEN_ + 32; /* v=0 p=0x12345671/32 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0x91a2b38;  pyld[2].len = _VRF_LEN_ + 31; /* v=0 p=0x12345670/31 */
        numkey = 3;
        break;

    case 5:
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x20; pyld[0].len = _VRF_LEN_ + 8; /* v=0 p=0x20000000/8 */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x8000; pyld[1].len = _VRF_LEN_ + 16; /* v=0 p=0x80000000/16 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0; pyld[2].len = _VRF_LEN_ + 0; /* v=0 p=0/0 */
        numkey = 3;
        break;

    case 7:
        pyld[0].key[0] = 0; pyld[0].key[1] = 0; pyld[0].len = _VRF_LEN_ + 0; /* v=0 p=0x0000000/0 */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x20000000; pyld[1].len = _VRF_LEN_ + 12; /* v=0 p=0x20000000/12 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0x30000000; pyld[2].len = _VRF_LEN_ + 12; /* v=0 p=0/0 */
        numkey = 3;

    case 6:
        {
            int dup;

            if (seed == 0) {
                seed = sal_time();
                sal_srand(seed);
            }
            index = 0;
            soc_cm_print("Random test: %d Seed: 0x%x \n", id, seed);
            do {
                do {
                    pyld[index].key[1] = (unsigned int) sal_rand();
                    pyld[index].len = (unsigned int)sal_rand() % 32;
                    pyld[index].len += _VRF_LEN_;

                    if (pyld[index].len <= 32) {
                        pyld[index].key[0] = 0;
                        pyld[index].key[1] &= MASK(pyld[index].len);
                    }

                    if (pyld[index].len > 32) {
                        pyld[index].key[0] = (unsigned int)sal_rand() % 16;                        
                        pyld[index].key[0] &= MASK(pyld[index].len-32); 
                    }

                    dup = ut_check_duplicate(pyld, index);
                    if (dup) {
                            soc_cm_print("\n Duplicate at index[%d]:key[0x%08x:0x%08x] Retry!!!\n", 
                                         index,pyld[index].key[0],pyld[index].key[1]);
                    }
                } while(dup > 0);
            } while(++index < _NUM_KEY_);

            numkey = index;
        }
        break;

    default:
        return -1;
    }

    trie_init(&trie);
    soc_cm_print("\n Num keys to test= %d \n", numkey);

    for(index=0; index < numkey && rv == SOC_E_NONE; index++) {
        unsigned int vrf=0, i;
        vrf = (pyld[index].len - _VRF_LEN_ == 32) ? 0:pyld[index].key[1] >> (pyld[index].len - _VRF_LEN_);
        vrf |= pyld[index].key[0] << (32 - (pyld[index].len - _VRF_LEN_));

#ifdef LOG
        soc_cm_print("+ Inserted Key=0x%x 0x%x vpn=0x%x pfx=0x%x Len=%d idx:%d\n", 
               pyld[index].key[0], pyld[index].key[1], vrf,
               pyld[index].key[1] & MASK(pyld[index].len - _VRF_LEN_), 
               pyld[index].len, index);
#endif
        rv = trie_insert(trie, &pyld[index].key[0], NULL, pyld[index].len, &pyld[index].node);
        if (rv != SOC_E_NONE) {
            soc_cm_print("FAILED to Insert Key=0x%x 0x%x vpn=0x%x pfx=0x%x Len=%d idx:%d\n", 
                   pyld[index].key[0], pyld[index].key[1], vrf,
                   pyld[index].key[1] & MASK(pyld[index].len - _VRF_LEN_), 
                   pyld[index].len, index);
        }
#define _VERBOSE_SEARCH_
        /* search all keys & figure out breakage right away */
        for (i=0; i <= index && rv == SOC_E_NONE; i++) {
#ifdef _VERBOSE_SEARCH_
            result_key[0] = 0;
            result_key[1] = 0;
            result_len    = 0;
            rv = trie_search_verbose(trie, &pyld[index].key[0], pyld[index].len, 
                                     &pyldptr, &result_key[0], &result_len);
#else
            rv = trie_search(trie, &pyld[index].key[0], pyld[index].len, &pyldptr);
#endif
            if (rv != SOC_E_NONE) {
                soc_cm_print("SEARCH: Key=0x%x 0x%x len %d SEARCH idx:%d failed!!!!\n", 
                       pyld[index].key[0], pyld[index].key[1],
                       pyld[index].len, index);
                break;
            } else {
                assert(pyldptr == &pyld[index].node);
#ifdef _VERBOSE_SEARCH_
                if (pyld[index].key[0] != result_key[0] ||
                    pyld[index].key[1] != result_key[1] ||
                    pyld[index].len != result_len) {
                    soc_cm_print(" Found key mismatches with the expected Key !!!! \n");
                    rv = SOC_E_FAIL;
                }
#ifdef VERBOSE
                soc_cm_print("Lkup[%d] key/len: 0x%x 0x%x/%d Found Key/len: 0x%x 0x%x/%d \n",
                             index,pyld[index].key[0], pyld[index].key[1], pyld[index].len,
                             result_key[0], result_key[1], result_len);
#endif
#endif
            }
        }
    }

#ifdef VERBOSE
    soc_cm_print("\n============== TRIE DUMP ================\n");
    trie_dump(trie, NULL, NULL);
    soc_cm_print("\n=========================================\n");
#endif

    /* randomly pickup prefix & delete */
    while(num_deleted < numkey && rv == SOC_E_NONE) {
        index = sal_rand() % numkey;
        if (pyld[index].len != 0xFFFFFFFF) {
            rv = trie_search(trie, &pyld[index].key[0], pyld[index].len, &pyldptr);
            if (rv == SOC_E_NONE) {
                assert(pyldptr == &pyld[index].node);
                rv = trie_delete(trie, &pyld[index].key[0], pyld[index].len, &pyldptr);

#ifdef VERBOSE
                soc_cm_print("\n============== TRIE DUMP ================\n");
                trie_dump(trie, NULL, NULL);
#endif
                if (rv == SOC_E_NONE) {
#ifdef LOG
                    soc_cm_print("Deleted Key=0x%x 0x%x Len=%d idx:%d Num-Key:%d\n", 
                           pyld[index].key[0], pyld[index].key[1], 
                           pyld[index].len, index, num_deleted);
#endif
                    pyld[index].len = 0xFFFFFFFF;
                    num_deleted++;

                    /* search all keys & figure out breakage right away */
                    for (index=0; index < numkey; index++) {
                        if (pyld[index].len == 0xFFFFFFFF) continue;

                        rv = trie_search(trie, &pyld[index].key[0], pyld[index].len, &pyldptr);
                        if (rv != SOC_E_NONE) {
                            soc_cm_print("ALL SEARCH after delete: Key=0x%x 0x%x len %d SEARCH idx:%d failed!!!!\n", 
                                   pyld[index].key[0], pyld[index].key[1],
                                   pyld[index].len, index);
                            break;
                        } else {
                            assert(pyldptr == &pyld[index].node);
                        }
                    }
                } else {
                    soc_cm_print("Deleted Key=0x%x 0x%x Len=%d idx:%d FAILED!!!\n", 
                           pyld[index].key[0], pyld[index].key[1], 
                           pyld[index].len, index);
                    break;
                }
            } else {
                soc_cm_print("SEARCH: Key=0x%x 0x%x len %d SEARCH idx:%d failed!!!!\n", 
                       pyld[index].key[0], pyld[index].key[1],
                       pyld[index].len, index);
                break;
            }
        }
    }

    if (rv == SOC_E_NONE) soc_cm_print("\n TEST ID %d passed \n", id);
    else {  
        soc_cm_print("\n TEST ID %d Failed Num Delete:%d !!!!!!!!\n", id, num_deleted);
    }

    sal_free(pyld);
    trie_destroy(trie);
    return rv;
}

/**********************************************/
/* BPM unit tests */
/* test cases:
 * 1 - insert pivot's with bpm bit masks
 * 2 - propagate updated prefix bpm (add/del)
 * 3 - fuse node bpm verification
 * 4 - split bpm - nop
 * 5 - */

typedef struct _expect_datum_s {
    dq_t list;
    payload_t *pfx; 
    trie_t *pfx_trie;
} expect_datum_t;

int ut_bpm_build_expect_list(trie_node_t *payload, void *user_data)
{
    int rv=SOC_E_NONE;

    if (payload && payload->type == PAYLOAD) {
        trie_node_t *pyldptr;
        payload_t *pivot;
        expect_datum_t *datum = (expect_datum_t*)user_data;

        pivot = TRIE_ELEMENT_GET(payload_t*, payload, node);
        /* if the inserted prefix is a best prefix, add the pivot to expected list */
        rv = trie_find_lpm(datum->pfx_trie, &pivot->key[0], pivot->len, &pyldptr); 
        assert(rv == SOC_E_NONE);
        if (pyldptr == &datum->pfx->info.pfx_trie_node) {
            /* if pivot is not equal to prefix add to expect list */
            if (!(pivot->key[0] == datum->pfx->key[0] && 
                  pivot->key[1] == datum->pfx->key[1] &&
                  pivot->len    == datum->pfx->len)) {
                DQ_INSERT_HEAD(&datum->list, &pivot->listnode);
            }
        }
    }

    return SOC_E_NONE;
}

int ut_bpm_propagate_cb(trie_node_t *payload, trie_bpm_cb_info_t *cbinfo)
{
    if (payload && cbinfo && payload->type == PAYLOAD) {
        payload_t *pivot;
        dq_p_t elem;
        expect_datum_t *datum = (expect_datum_t*)cbinfo->user_data;

        pivot = TRIE_ELEMENT_GET(payload_t*, payload, node);
        DQ_TRAVERSE(&datum->list, elem) {
            payload_t *velem = DQ_ELEMENT_GET(payload_t*, elem, listnode); 
            if (velem == pivot) {
                DQ_REMOVE(&pivot->listnode);
                break;
            }
        } DQ_TRAVERSE_END(&datum->list, elem);
    }

    return SOC_E_NONE;
}

int ut_bpm_propagate_empty_cb(trie_node_t *payload, trie_bpm_cb_info_t *cbinfo)
{
    /* do nothing */
    return SOC_E_NONE;
}

void ut_bpm_dump_expect_list(expect_datum_t *datum, char *str)
{
    dq_p_t elem;
    if (datum) {
        /* dump expected list */
        soc_cm_print("%s \n", str);
        DQ_TRAVERSE(&datum->list, elem) {
            payload_t *velem = DQ_ELEMENT_GET(payload_t*, elem, listnode); 
            soc_cm_print(" Pivot: 0x%x 0x%x Len: %d \n", 
                         velem->key[0], velem->key[1], velem->len);
        } DQ_TRAVERSE_END(&datum->list, elem);
    }
}

#define _MAX_TEST_PIVOTS_ (10)
#define _MAX_BKT_PFX_ (20)
#define _MAX_NUM_PICK (30)

int tmu_taps_bpm_trie_ut(int id, unsigned int seed)
{
    int i, rv = SOC_E_NONE, pivot=0, pfx=0, index=0, dup=0, domain=0;
    trie_t *pfx_trie, *trie;
    payload_t *pyld = sal_alloc(_MAX_BKT_PFX_ * _MAX_TEST_PIVOTS_ * sizeof(payload_t), "bpm-unit-test");
    payload_t *pivot_pyld = sal_alloc(_MAX_TEST_PIVOTS_ * sizeof(payload_t), "bpm-unit-test");
    trie_node_t *pyldptr, *newroot;
    unsigned int bpm[BITS2WORDS(_MAX_KEY_LEN_)];
    expect_datum_t datum;
    trie_bpm_cb_info_t cbinfo;
    int num_pick, bpm_pfx_len;

    sal_memset(pyld, 0, _MAX_BKT_PFX_ * _MAX_TEST_PIVOTS_ * sizeof(payload_t));
    sal_memset(pivot_pyld, 0, _MAX_TEST_PIVOTS_ * sizeof(payload_t));

    if (seed == 0) {
        seed = sal_time();
        sal_srand(seed);
    }    

    trie_init(&trie);
    trie_init(&pfx_trie);

    /* populate a random pivot / prefix trie */
    soc_cm_print("Random test: %d Seed: 0x%x \n", id, seed);

    /* insert a vrf=0,* pivot */
    pivot = 0;
    pfx = 0;
    pivot_pyld[pivot].key[1] = 0;
    pivot_pyld[pivot].key[0] = 0;
    pivot_pyld[pivot].len    = 0;
    trie_init(&pivot_pyld[pivot].info.trie);
    sal_memset(&bpm[0], 0,  BITS2WORDS(_MAX_KEY_LEN_) * sizeof(unsigned int));
            
    do {
        rv = trie_insert(trie, &pivot_pyld[pivot].key[0], &bpm[0], 
                         pivot_pyld[pivot].len, &pivot_pyld[pivot].node);
        if (rv != SOC_E_NONE) {
            soc_cm_print("FAILED to Insert PIVOT Key=0x%x 0x%x Len=%d idx:%d\n", 
                   pivot_pyld[pivot].key[0], pivot_pyld[pivot].key[1], 
                   pivot_pyld[pivot].len, pivot);
        } else {
            if (pivot > 0) {
                /* choose a random pivot bucket to fill & split */
                domain = ((unsigned int) sal_rand()) % pivot;
            } else {
                domain = 0;
            }
            
            index = 0;
            sal_memset(&bpm[0], 0,  BITS2WORDS(_MAX_KEY_LEN_) * sizeof(unsigned int));
    
            do {
                do {
                    /* add prefix such that lpm of the prefix is the pivot to ensure
                     * it goes into specific pivot domain */
                    pyld[pfx+index].key[1] = (unsigned int) sal_rand();
                    pyld[pfx+index].len = (unsigned int)sal_rand() % 32;
                    pyld[pfx+index].len += _VRF_LEN_;

                    if (pyld[pfx+index].len <= 32) {
                        pyld[pfx+index].key[0] = 0;
                        pyld[pfx+index].key[1] &= MASK(pyld[pfx+index].len);
                    }

                    if (pyld[pfx+index].len > 32) {
                        pyld[pfx+index].key[0] = (unsigned int)sal_rand() % 16;                        
                        pyld[pfx+index].key[0] &= MASK(pyld[pfx+index].len-32); 
                    }

                    dup = ut_check_duplicate(pyld, pfx+index);
                    if (!dup) {
                        rv = trie_find_lpm(trie, &pyld[pfx+index].key[0], pyld[pfx+index].len, &pyldptr); 
                        if (SOC_FAILURE(rv)) {
                            soc_cm_print("\n !! Failed to find LPM pivot for index[%d]:key[0x%08x:0x%08x] !!!!\n",
                                         pfx,pyld[pfx+index].key[0],pyld[pfx+index].key[1]);
                        } 
                    }
                } while ((dup || (pyldptr != &pivot_pyld[domain].node)) && SOC_SUCCESS(rv));

                if (SOC_SUCCESS(rv)) {
                    rv =  trie_insert(pivot_pyld[domain].info.trie,
                                      &pyld[pfx+index].key[0], NULL, 
                                      pyld[pfx+index].len, &pyld[pfx+index].node);
                    if (SOC_FAILURE(rv)) {
                        soc_cm_print("\n !! Failed insert prefix into pivot trie"
                                     " index[%d]:key[0x%08x:0x%08x] !!!!\n",
                                     pfx+index,pyld[pfx+index].key[0],pyld[pfx+index].key[1]);
                    } else {
                        rv =  trie_insert(pfx_trie,
                                          &pyld[pfx+index].key[0], NULL, 
                                          pyld[pfx+index].len, &pyld[pfx+index].info.pfx_trie_node);     
                        if (SOC_FAILURE(rv)) {
                            soc_cm_print("\n !! Failed insert prefix into prefix trie"
                                         " index[%d]:key[0x%08x:0x%08x] !!!!\n",
                                         pfx+index,pyld[pfx+index].key[0],pyld[pfx+index].key[1]);
                        } else {
                            index++;
                        }                      
                    }
                }

            } while(index < (_MAX_BKT_PFX_/2 - 1) && SOC_SUCCESS(rv));

            /* try to populate prefix where p == v */
            if (pivot > 0) {
                /* 25% probability */
                if (((unsigned int) sal_rand() % 4) == 0) {
                    
                }
            }

#ifdef VERBOSE
            soc_cm_print("### Split Domain ID: %d \n", domain);
            for (i=0; i <= pivot; i++) {
                soc_cm_print("\n --- TRIE domain dump: Pivot: 0x%x 0x%x len=%d ----- \n",
                             pivot_pyld[i].key[0], pivot_pyld[i].key[1], pivot_pyld[i].len);
                trie_dump(pivot_pyld[i].info.trie, ut_print_payload_node, NULL);
            }
#endif

            if (SOC_SUCCESS(rv) && ++pivot < _MAX_TEST_PIVOTS_) {
                pfx += index;
                trie_init(&pivot_pyld[pivot].info.trie);
                /* split the domain & insert a new pivot */
                rv = trie_split(pivot_pyld[domain].info.trie,
                                &pivot_pyld[pivot].key[0], 
                                &pivot_pyld[pivot].len, &newroot, &bpm[0]);
                if (SOC_SUCCESS(rv)) {
                    pivot_pyld[pivot].info.trie->trie = newroot;
                    soc_cm_print("BPM for split pivot: 0x%x 0x%x / %d = [0x%x 0x%x] \n",
                                 pivot_pyld[pivot].key[0], pivot_pyld[pivot].key[1],
                                 pivot_pyld[pivot].len, bpm[0], bpm[1]);
                } else {
                    soc_cm_print("\n !!! Failed to split domain trie for domain: %d !!!\n", domain);
                }
            }
        }
    } while(pivot < _MAX_TEST_PIVOTS_ && SOC_SUCCESS(rv));

    /* pick up the root node on pivot trie & add a prefix shorter than the nearest child.
     * This is ripple & create huge propagation */
    /* insert *\/1 into the * bucket so huge propagation kicks in */
    pyld[pfx].key[1] = (unsigned int) sal_rand() % 1;
    pyld[pfx].key[0] = 0;
    pyld[pfx].len    = 1;
    do {
        dup = ut_check_duplicate(pyld, pfx);
        if (!dup) {
            rv = trie_find_lpm(trie, &pyld[pfx].key[0], pyld[pfx].len, &pyldptr); 
            if (SOC_FAILURE(rv)) {
                soc_cm_print("\n !! Failed to find LPM pivot for index[%d]:key[0x%08x:0x%08x] !!!!\n",
                             pfx,pyld[pfx].key[0],pyld[pfx].key[1]);
            } 
        } else {
            pyld[pfx].len++;    
        }
    } while(dup && SOC_SUCCESS(rv));

    if (SOC_SUCCESS(rv)) {
        rv =  trie_insert(pfx_trie,
                          &pyld[pfx].key[0], NULL, 
                          pyld[pfx].len, &pyld[pfx].info.pfx_trie_node);
        if (SOC_FAILURE(rv)) {
            soc_cm_print("\n !! Failed insert prefix into pivot trie"
                         " index[%d]:key[0x%08x:0x%08x] !!!!\n",
                         pfx,pyld[pfx].key[0],pyld[pfx].key[1]);
        } else {
            DQ_INIT(&datum.list);
            datum.pfx = &pyld[pfx];
            datum.pfx_trie = pfx_trie;
            /* create expected list of pivot to be propagated */
            trie_traverse(trie, ut_bpm_build_expect_list, &datum, _TRIE_PREORDER_TRAVERSE);

            /* dump expected list */
            ut_bpm_dump_expect_list(&datum, "-- Expected Propagation List --");
        }
    }

    sal_memset(&cbinfo, 0, sizeof(trie_bpm_cb_info_t));
    cbinfo.user_data = &datum;
    cbinfo.pfx = &pyld[pfx].key[0];
    cbinfo.len = pyld[pfx].len;
    rv = trie_pivot_propagate_prefix(pyldptr,
                               (TRIE_ELEMENT_GET(payload_t*, pyldptr, node))->len,
                               &pyld[pfx].key[0], pyld[pfx].len,
                               1, ut_bpm_propagate_cb, &cbinfo);
    if (DQ_EMPTY(&datum.list)) {
        soc_cm_print("++ Propagation Test Passed \n");
    } else {
        soc_cm_print("!!!!! Propagation Test FAILED !!!!!\n");
        rv = SOC_E_FAIL;
        ut_bpm_dump_expect_list(&datum, "!! Zombies on Propagation List !!");
        assert(0);
    }

    /* propagate a shorter prefix of an existing pivot 
     * we should find the bpm
     */
    pfx++;
    num_pick = 0;
    do {
	/* randomly pick a pivot */
	index = ((unsigned int) sal_rand()) % pivot;
	
	/* create a prefix shorter */
	pyld[pfx].len    = ((unsigned int) sal_rand()) % pivot_pyld[index].len;
	pyld[pfx].key[1] = pivot_pyld[index].key[1]>>(pivot_pyld[index].len - pyld[pfx].len);
	pyld[pfx].key[0] = 0;

	if (pyld[pfx].len >= 1) {
	    /* propagate add len=0 */
	    rv = trie_pivot_propagate_prefix(trie->trie,
				       (TRIE_ELEMENT_GET(payload_t*, trie->trie, node))->len,
				       &pyld[pfx].key[0], 0,
				       1, ut_bpm_propagate_empty_cb, 
				       (trie_bpm_cb_info_t *)NULL);

	    if (SOC_FAILURE(rv)) {
		soc_cm_print("!!!!! BPM search Test FAILED to propagate add len=0!!!!!\n");
		assert(0);
	    }

	    /* propagate add */
	    rv = trie_pivot_propagate_prefix(trie->trie,
				       (TRIE_ELEMENT_GET(payload_t*, trie->trie, node))->len,
				       &pyld[pfx].key[0], pyld[pfx].len,
				       1, ut_bpm_propagate_empty_cb, 
				       (trie_bpm_cb_info_t *)NULL);
	    if (SOC_FAILURE(rv)) {
		soc_cm_print("!!!!! BPM search Test FAILED to propagate add \n"
			     " index[%d]:key[0x%08x:0x%08x] len=%d!!!!\n",
			     pfx, pyld[pfx].key[0], pyld[pfx].key[1], pyld[pfx].len);
		assert(0);
	    }

	    /* perform bpm lookup on the pivot, we should find the pyld[pfx].len */
	    rv = trie_find_prefix_bpm(trie, (unsigned int *)&(pivot_pyld[index].key[0]),
				      pivot_pyld[index].len, (unsigned int *)&bpm_pfx_len);
	    if (SOC_FAILURE(rv) || (bpm_pfx_len != pyld[pfx].len)) {
		soc_cm_print("!!!!! BPM search Test FAILDED after propagate add !!!!!\n");
		assert(0);		
	    }

	    /* propagate delete */
	    rv = trie_pivot_propagate_prefix(trie->trie,
				       (TRIE_ELEMENT_GET(payload_t*, trie->trie, node))->len,
				       &pyld[pfx].key[0], pyld[pfx].len,
				       0, ut_bpm_propagate_empty_cb, 
				       (trie_bpm_cb_info_t *)NULL);
	    
	    if (SOC_FAILURE(rv)) {
		soc_cm_print("!!!!! BPM search Test FAILED to propagate add \n"
			     " index[%d]:key[0x%08x:0x%08x] len=%d!!!!\n",
			     pfx, pyld[pfx].key[0], pyld[pfx].key[1], pyld[pfx].len);
		assert(0);
	    }

	    /* perform bpm lookup on the pivot, we should find the len==0 */
	    rv = trie_find_prefix_bpm(trie, (unsigned int *)&(pivot_pyld[index].key[0]),
				      pivot_pyld[index].len, (unsigned int *)&bpm_pfx_len);
	    if (SOC_FAILURE(rv) || (bpm_pfx_len != 0)) {
		soc_cm_print("!!!!! BPM search Test FAILDED after propagate delete !!!!!\n");
		assert(0);		
	    }

	    num_pick = _MAX_NUM_PICK+1;
	}
	num_pick++;
    } while(num_pick<_MAX_NUM_PICK);

    if (num_pick <= _MAX_NUM_PICK) {
	soc_cm_print("!!!!! BPM search Test 2 Skipped after tried %d times!!!!!\n", _MAX_NUM_PICK);	
    } else {
	soc_cm_print("!!!!! BPM search Test 2 Passed!!!!!\n");	
    }

#ifdef VERBOSE
    soc_cm_print("\n ----- Prefix Trie dump ----- \n");
    trie_dump(pfx_trie, ut_print_prefix_payload_node, NULL);
#endif

    soc_cm_print("\n ++++++++ Trie dump ++++++++ \n");
    trie_dump(trie, ut_print_payload_node, NULL);

    /* clean up */
    for (index=0; index < pivot; index++) {
#ifdef VERBOSE
        soc_cm_print("\n ddddddd dump dddddddd \n");
        trie_dump(pivot_pyld[index].info.trie, ut_print_payload_node, NULL);
#endif
        trie_destroy(pivot_pyld[index].info.trie);
    }

    sal_free(pyld);
    sal_free(pivot_pyld);
    trie_destroy(trie);
    trie_destroy(pfx_trie);
    return rv;
}

/**********************************************/
#endif /* UNIT_TESTS */

#else /* ALPM_IPV6_128_SUPPORT */

#include <shared/util.h>
#include <sal/appl/sal.h>
#include <sal/core/libc.h>
#include <sal/core/time.h>
#include <soc/esw/trie.h>
#include <soc/esw/alpm_trie_v6.h>

#define _MAX_SKIP_LEN_  (31)
#define _MAX_KEY_LEN_   (48)

#define _MAX_KEY_WORDS_ (BITS2WORDS(_MAX_KEY_LEN_))

#define SHL(data, shift, max) \
    (((shift)>=(max))?0:((data)<<(shift)))

#define SHR(data, shift, max) \
    (((shift)>=(max))?0:((data)>>(shift)))

#define MASK(len) \
    (((len)>=32 || (len)==0)?0xFFFFFFFF:((1<<(len))-1))

#define BITMASK(len) \
    (((len)>=32)?0xFFFFFFFF:((1<<(len))-1))

#define ABS(n) ((((int)(n)) < 0) ? -(n) : (n))

#define _NUM_WORD_BITS_ (32)

#define BITS2SKIPOFF(x) (((x) + _MAX_SKIP_LEN_-1) / _MAX_SKIP_LEN_)

/* key packing expetations:
 * eg., 48 bit key
 * - 10/8 -> key[0]=0, key[1]=8
 * - 0x123456789a -> key[0] = 0x12 key[1] = 0x3456789a
 * length - represents number of valid bits from farther to lower index ie., 1->0 
 */

#define KEY_BIT2IDX(x) (((BITS2WORDS(_MAX_KEY_LEN_)*32) - (x))/32)


/* (internal) Generic operation macro on bit array _a, with bit _b */
#define	_BITOP(_a, _b, _op)	\
        ((_a) _op (1U << ((_b) % _NUM_WORD_BITS_)))

/* Specific operations */
#define	_BITGET(_a, _b)	_BITOP(_a, _b, &)
#define	_BITSET(_a, _b)	_BITOP(_a, _b, |=)
#define	_BITCLR(_a, _b)	_BITOP(_a, _b, &= ~)

/* get the bit position of the LSB set in bit 0 to bit "msb" of "data"
 * (max 32 bits), "lsb" is set to -1 if no bit is set in "data".
 */
#define BITGETLSBSET(data, msb, lsb)    \
    {                                    \
	lsb = 0;                         \
	while ((lsb)<=(msb)) {		 \
	    if ((data)&(1<<(lsb)))     { \
		break;                   \
	    } else { (lsb)++;}           \
	}                                \
	lsb = ((lsb)>(msb))?-1:(lsb);    \
    }
#define KEY_BIT2IDX(x) (((BITS2WORDS(_MAX_KEY_LEN_)*32) - (x))/32)

/********************************************************/
/* Get a chunk of bits from a key (MSB bit - on word0, lsb on word 1).. 
 */
unsigned int _key_get_bits(unsigned int *key, 
                           unsigned int pos /* 1based, msb bit position */, 
                           unsigned int len,
                           unsigned int skip_len_check)
{
    unsigned int val=0, delta=0, diff, bitpos;

    if (!key || (pos < 1) || (pos > _MAX_KEY_LEN_) || 
        ((skip_len_check == 0) && (len > _MAX_SKIP_LEN_))) assert(0);

    bitpos = (pos-1) % _NUM_WORD_BITS_;
    bitpos++; /* 1 based */

    if (bitpos >= len) {
        diff = bitpos - len;
        val = SHR(key[KEY_BIT2IDX(pos)], diff, _NUM_WORD_BITS_);
        val &= MASK(len);
        return val;
    } else {
        diff = len - bitpos;
        if (skip_len_check==0) assert(diff <= _MAX_SKIP_LEN_);
        val = key[KEY_BIT2IDX(pos)] & MASK(bitpos);
        val = SHL(val, diff, _NUM_WORD_BITS_);
        /* get bits from next word */
        delta = _key_get_bits(key, pos-bitpos, diff, skip_len_check);
        return (delta | val);
    }
}		


/* 
 * Assumes the layout for 
 * 0 - most significant word
 * _MAX_KEY_WORDS_ - least significant word
 * eg., for key size of 48, word0-[bits 48-32] word1-[bits31-0]
 */
int _key_shift_left(unsigned int *key, unsigned int shift)
{
    unsigned int index=0;

    if (!key || shift > _MAX_SKIP_LEN_) return SOC_E_PARAM;

    for(index=KEY_BIT2IDX(_MAX_KEY_LEN_); index < KEY_BIT2IDX(1); index++) {
        key[index] = SHL(key[index], shift,_NUM_WORD_BITS_) | \
                     SHR(key[index+1],_NUM_WORD_BITS_-shift,_NUM_WORD_BITS_);
    }

    key[index] = SHL(key[index], shift, _NUM_WORD_BITS_);

    /* mask off snippets bit on MSW */
    key[0] &= MASK(_MAX_KEY_LEN_ % _NUM_WORD_BITS_);
    return SOC_E_NONE;
}

/* 
 * Assumes the layout for 
 * 0 - most significant word
 * _MAX_KEY_WORDS_ - least significant word
 * eg., for key size of 48, word0-[bits 48-32] word1-[bits31-0]
 */
int _key_shift_right(unsigned int *key, unsigned int shift)
{
    unsigned int index=0;

    if (!key || shift > _MAX_SKIP_LEN_) return SOC_E_PARAM;

    for(index=KEY_BIT2IDX(1); index > KEY_BIT2IDX(_MAX_KEY_LEN_); index--) {
        key[index] = SHR(key[index], shift,_NUM_WORD_BITS_) | \
                     SHL(key[index-1],_NUM_WORD_BITS_-shift,_NUM_WORD_BITS_);
    }

    key[index] = SHR(key[index], shift, _NUM_WORD_BITS_);

    /* mask off snippets bit on MSW */
    key[0] &= MASK(_MAX_KEY_LEN_ % _NUM_WORD_BITS_);
    return SOC_E_NONE;
}


/* 
 * Assumes the layout for 
 * 0 - most significant word
 * _MAX_KEY_WORDS_ - least significant word
 * eg., for key size of 48, word0-[bits 48-32] word1-[bits31-0]
 */
int _key_append(unsigned int *key, 
                unsigned int *length,
                unsigned int skip_addr,
                unsigned int skip_len)
{
    int rv=SOC_E_NONE;

    if (!key || !length || (skip_len + *length > _MAX_KEY_LEN_) || skip_len > _MAX_SKIP_LEN_ ) return SOC_E_PARAM;

    rv = _key_shift_left(key, skip_len);
    if (SOC_SUCCESS(rv)) {
        key[KEY_BIT2IDX(1)] |= skip_addr;
        *length += skip_len;
    }

    return rv;
}

int _bpm_append(unsigned int *key, 
                unsigned int *length,
                unsigned int skip_addr,
                unsigned int skip_len)
{
    int rv=SOC_E_NONE;

    if (!key || !length || (skip_len + *length > _MAX_KEY_LEN_) || skip_len > (_MAX_SKIP_LEN_+1) ) return SOC_E_PARAM;

    if (skip_len == 32) {
	key[0] = key[1];
	key[1] = skip_addr;
	*length += skip_len;
    } else {
	rv = _key_shift_left(key, skip_len);
	if (SOC_SUCCESS(rv)) {
	    key[KEY_BIT2IDX(1)] |= skip_addr;
	    *length += skip_len;
	}
    }

    return rv;
}

/*
 * Function:
 *     lcplen
 * Purpose:
 *     returns longest common prefix length provided a key & skip address
 */
unsigned int
lcplen(unsigned int *key, unsigned int len1,
       unsigned int skip_addr, unsigned int len2)
{
    unsigned int diff;
    unsigned int lcp = len1 < len2 ? len1 : len2;

    if ((len1 > _MAX_KEY_LEN_) || (len2 > _MAX_KEY_LEN_)) {
	soc_cm_print("len1 %d or len2 %d is larger than %d\n", len1, len2, _MAX_KEY_LEN_);
	assert(0);
    } 

    if (len1 == 0 || len2 == 0) return 0;

    diff = _key_get_bits(key, len1, lcp, 0);
    diff ^= (SHR(skip_addr, len2 - lcp, _MAX_SKIP_LEN_) & MASK(lcp));

    while (diff) {
        diff >>= 1;
        --lcp;
    }

    return lcp;
}

int _print_trie_node(trie_node_t *trie, void *datum)
{
    if (trie != NULL) {

	soc_cm_print("trie: %p, type %s, skip_addr 0x%x skip_len %d count:%d bpm:0x%x Child[0]:%p Child[1]:%p\n",
                     trie, (trie->type == PAYLOAD)?"P":"I",
                     trie->skip_addr, trie->skip_len, 
                     trie->count, trie->bpm, trie->child[0].child_node, trie->child[1].child_node);
    }
    return SOC_E_NONE;
}

static int _trie_preorder_traverse(trie_node_t *trie, trie_callback_f cb, void *user_data)
{
    int rv = SOC_E_NONE;
    trie_node_t *tmp1, *tmp2;

    if (trie == NULL || !cb) {
	return SOC_E_NONE;
    } else {
        /* make the node delete safe */
        tmp1 = trie->child[0].child_node;
        tmp2 = trie->child[1].child_node;
        rv = cb(trie, user_data);
    }

    if (SOC_SUCCESS(rv)) {
        rv = _trie_preorder_traverse(tmp1, cb, user_data);
    }
    if (SOC_SUCCESS(rv)) {
        rv = _trie_preorder_traverse(tmp2, cb, user_data);
    }
    return rv;
}

static int _trie_postorder_traverse(trie_node_t *trie, trie_callback_f cb, void *user_data)
{
    int rv = SOC_E_NONE;

    if (trie == NULL) {
	return SOC_E_NONE;
    }

    if (SOC_SUCCESS(rv)) {
        rv = _trie_postorder_traverse(trie->child[0].child_node, cb, user_data);
    }
    if (SOC_SUCCESS(rv)) {
        rv = _trie_postorder_traverse(trie->child[1].child_node, cb, user_data);
    }
    if (SOC_SUCCESS(rv)) {
        rv = cb(trie, user_data);
    }
    return rv;
}

static int _trie_inorder_traverse(trie_node_t *trie, trie_callback_f cb, void *user_data)
{
    int rv = SOC_E_NONE;
    trie_node_t *tmp = NULL;

    if (trie == NULL) {
	return SOC_E_NONE;
    }

    if (SOC_SUCCESS(rv)) {
        rv = _trie_inorder_traverse(trie->child[0].child_node, cb, user_data);
    }

    /* make the trie pointers delete safe */
    tmp = trie->child[1].child_node;

    if (SOC_SUCCESS(rv)) {
        rv = cb(trie, user_data);
    }

    if (SOC_SUCCESS(rv)) {
        rv = _trie_inorder_traverse(tmp, cb, user_data);
    }
    return rv;
}

static int _trie_traverse(trie_node_t *trie, trie_callback_f cb, 
			  void *user_data,  trie_traverse_order_e_t order)
{
    int rv = SOC_E_NONE;

    switch(order) {
    case _TRIE_PREORDER_TRAVERSE:
        rv = _trie_preorder_traverse(trie, cb, user_data);
        break;
    case _TRIE_POSTORDER_TRAVERSE:
        rv = _trie_postorder_traverse(trie, cb, user_data);
        break;
    case _TRIE_INORDER_TRAVERSE:
        rv = _trie_inorder_traverse(trie, cb, user_data);
        break;
    default:
        assert(0);
    }

    return rv;
}

/*
 * Function:
 *     trie_traverse
 * Purpose:
 *     Traverse the trie & call the application callback with user data 
 */
int trie_traverse(trie_t *trie, trie_callback_f cb, 
                  void *user_data, trie_traverse_order_e_t order)
{
    if (order < _TRIE_PREORDER_TRAVERSE ||
        order >= _TRIE_TRAVERSE_MAX || !cb) return SOC_E_PARAM;

    if (trie == NULL) {
	return SOC_E_NONE;
    } else {
        return _trie_traverse(trie->trie, cb, user_data, order);
    }
}

static int _trie_preorder_iter_get_first(trie_node_t *node, trie_node_t **payload)
{
    int rv = SOC_E_NONE;

    if (!payload) return SOC_E_PARAM;

    if (*payload != NULL) return SOC_E_NONE;

    if (node == NULL) {
	return SOC_E_NONE;
    } else {
        if (node->type == PAYLOAD) {
            *payload = node;
            return rv;
        }
    }

    if (SOC_SUCCESS(rv)) {
        rv = _trie_preorder_iter_get_first(node->child[0].child_node, payload);
    }
    if (SOC_SUCCESS(rv)) {
        rv = _trie_preorder_iter_get_first(node->child[1].child_node, payload);
    }
    return rv;
}

/*
 * Function:
 *     trie_iter_get_first
 * Purpose:
 *     Traverse the trie & return pointer to first payload node
 */
int trie_iter_get_first(trie_t *trie, trie_node_t **payload)
{
    int rv = SOC_E_EMPTY;

    if (!trie || !payload) return SOC_E_PARAM;

    if (trie && trie->trie) {
        *payload = NULL;
        return _trie_preorder_iter_get_first(trie->trie, payload);
    }

    return rv;
}

static int _trie_dump(trie_node_t *trie, trie_callback_f cb, 
		      void *user_data, unsigned int level)
{
    if (trie == NULL) {
	return SOC_E_NONE;
    } else {
        unsigned int lvl = level;
	while(lvl) {
	    if (lvl == 1) {
		soc_cm_print("|-");
	    } else {
		soc_cm_print("| ");
	    }
	    lvl--; 
	}

        if (cb) {
            cb(trie, user_data);
        } else {
            _print_trie_node(trie, NULL);
        }
    }

    _trie_dump(trie->child[0].child_node, cb, user_data, level+1);
    _trie_dump(trie->child[1].child_node, cb, user_data, level+1);
    return SOC_E_NONE;
}

/*
 * Function:
 *     trie_dump
 * Purpose:
 *     Dumps the trie pre-order [root|left|child]
 */
int trie_dump(trie_t *trie, trie_callback_f cb, void *user_data)
{
    if (trie->trie) {
	return _trie_dump(trie->trie, cb, user_data, 0);
    } else {
        return SOC_E_PARAM;
    }
}

static int _trie_search(trie_node_t *trie,
			unsigned int *key,
			unsigned int length,
			trie_node_t **payload,
			unsigned int *result_key,
			unsigned int *result_len,
			unsigned int dump)
{
    unsigned int lcp=0;
    int bit=0, rv=SOC_E_NONE;

    if (!trie) return SOC_E_PARAM;
    if ((result_key && !result_len) || (!result_key && result_len)) return SOC_E_PARAM;

    lcp = lcplen(key, length, trie->skip_addr, trie->skip_len);

    if (dump) {
        _print_trie_node(trie, (unsigned int *)1);
    }

    if (length > trie->skip_len) {
        if (lcp == trie->skip_len) {
            bit = (key[KEY_BIT2IDX(length - lcp)] & \
                   SHL(1,(length - lcp - 1) % _NUM_WORD_BITS_,_NUM_WORD_BITS_)) ? 1:0;
            if (dump) {
                soc_cm_print(" Length: %d Next-Bit[%d] \n", length, bit);
            }

            if (result_key) {
                rv = _key_append(result_key, result_len, trie->skip_addr, trie->skip_len);
                if (SOC_FAILURE(rv)) return rv;
            }

            /* based on next bit branch left or right */
            if (trie->child[bit].child_node) {

                if (result_key) {
                    rv = _key_append(result_key, result_len, bit, 1);
                    if (SOC_FAILURE(rv)) return rv;
                }

                return _trie_search(trie->child[bit].child_node, key, 
                                    length - lcp - 1, payload, 
                                    result_key, result_len, dump);
            } else {
                return SOC_E_NOT_FOUND; /* not found */
            }
        } else { 
            return SOC_E_NOT_FOUND; /* not found */
        }
    } else if (length == trie->skip_len) {
        if (lcp == length) {
            if (dump) soc_cm_print(": MATCH \n");
            *payload = trie;
	    if (trie->type != PAYLOAD) {
		/* no assert here, possible during dbucket search
		 * due to 1* and 0* bucket search
		 */
		return SOC_E_NOT_FOUND;
	    }
            if (result_key) {
                rv = _key_append(result_key, result_len, trie->skip_addr, trie->skip_len);
                if (SOC_FAILURE(rv)) return rv;
            }
            return SOC_E_NONE;
        }
        else return SOC_E_NOT_FOUND;
    } else {
        return SOC_E_NOT_FOUND; /* not found */        
    }
}

/*
 * Function:
 *     trie_search
 * Purpose:
 *     Search the given trie for exact match of provided prefix/length
 *     If dump is set to 1 it traces the path as it traverses the trie 
 */
int trie_search(trie_t *trie, 
                unsigned int *key, 
                unsigned int length,
                trie_node_t **payload)
{
    if (trie->trie) {
	if (trie->v6_key) {
	    return _trie_v6_search(trie->trie, key, length, payload, NULL, NULL, 0);	    
	} else {
	    return _trie_search(trie->trie, key, length, payload, NULL, NULL, 0);
	}
    } else {
        return SOC_E_NOT_FOUND;
    }
}

/*
 * Function:
 *     trie_search_verbose
 * Purpose:
 *     Search the given trie for provided prefix/length
 *     If dump is set to 1 it traces the path as it traverses the trie 
 */
int trie_search_verbose(trie_t *trie, 
                        unsigned int *key, 
                        unsigned int length,
                        trie_node_t **payload,
                        unsigned int *result_key, 
                        unsigned int *result_len)
{
    if (trie->trie) {
	if (trie->v6_key) {
	    return _trie_v6_search(trie->trie, key, length, payload, result_key, result_len, 0);
	} else {
	    return _trie_search(trie->trie, key, length, payload, result_key, result_len, 0);
	}
    } else {
        return SOC_E_NOT_FOUND;
    }
}

/*
 * Internal function for LPM match searching.
 * callback on all payload nodes if cb != NULL.
 */
static int _trie_find_lpm(trie_node_t *trie,
			  unsigned int *key,
			  unsigned int length,
			  trie_node_t **payload,
			  trie_callback_f cb,
			  void *user_data)
{
    unsigned int lcp=0;
    int bit=0, rv=SOC_E_NONE;

    if (!trie) return SOC_E_PARAM;

    lcp = lcplen(key, length, trie->skip_addr, trie->skip_len);

    if ((length > trie->skip_len) && (lcp == trie->skip_len)) {
        if (trie->type == PAYLOAD) {
	    /* lpm cases */
	    if (payload != NULL) {
		/* update lpm result */
		*payload = trie;
	    }

	    if (cb != NULL) {
		/* callback with any nodes which is shorter and matches the prefix */
		rv = cb(trie, user_data);
		if (SOC_FAILURE(rv)) {
		    /* early bailout if there is error in callback handling */
		    return rv;
		}
	    }
	}

        bit = (key[KEY_BIT2IDX(length - lcp)] & \
               SHL(1,(length - lcp - 1) % _NUM_WORD_BITS_, _NUM_WORD_BITS_)) ? 1:0;

        /* based on next bit branch left or right */
        if (trie->child[bit].child_node) {
            return _trie_find_lpm(trie->child[bit].child_node, key, length - lcp - 1,
				  payload, cb, user_data);
        } 
    } else if ((length == trie->skip_len) && (lcp == length)) {
        if (trie->type == PAYLOAD) {
	    /* exact match case */
	    if (payload != NULL) {		
		/* lpm is exact match */
		*payload = trie;
	    }

	    if (cb != NULL) {
		/* callback with the exact match node */
		rv = cb(trie, user_data);
		if (SOC_FAILURE(rv)) {
		    /* early bailout if there is error in callback handling */
		    return rv;
		}		
	    }
        }
    }
    return rv;
}

/*
 * Function:
 *     trie_find_lpm
 * Purpose:
 *     Find the longest prefix matched with given prefix 
 */
int trie_find_lpm(trie_t *trie, 
                  unsigned int *key, 
                  unsigned int length,
                  trie_node_t **payload)
{
    *payload = NULL;

    if (trie->trie) {
	if (trie->v6_key) {
	    return _trie_v6_find_lpm(trie->trie, key, length, payload, NULL, NULL);
	} else {
	    return _trie_find_lpm(trie->trie, key, length, payload, NULL, NULL);
	}
    }

    return SOC_E_NOT_FOUND;
}

/*
 * Function:
 *     trie_find_pm
 * Purpose:
 *     Find the prefix matched nodes with given prefix and callback
 *     with specified callback funtion and user data
 */
int trie_find_pm(trie_t *trie, 
		 unsigned int *key, 
		 unsigned int length,
		 trie_callback_f cb,
		 void *user_data)
{

    if (trie->trie) {
	if (trie->v6_key) {
	    return _trie_v6_find_lpm(trie->trie, key, length, NULL, cb, user_data);	
	} else {
	    return _trie_find_lpm(trie->trie, key, length, NULL, cb, user_data);
	}
    }

    return SOC_E_NONE;
}

/* trie->bpm format:
 * bit 0 is for the pivot itself (longest)
 * bit skip_len is for the trie branch leading to the pivot node (shortest)
 * bits (0-skip_len) is for the routes in the parent node's bucket
 */
int _trie_find_bpm(trie_node_t *trie,
                   unsigned int *key,
                   unsigned int length,
		   int *bpm_length)
{
    unsigned int lcp=0, local_bpm_mask=0;
    int bit=0, rv=SOC_E_NONE, local_bpm=0;

    if (!trie || (length > _MAX_KEY_LEN_)) return SOC_E_PARAM;

    /* calculate number of matching msb bits */
    lcp = lcplen(key, length, trie->skip_addr, trie->skip_len);

    if (length > trie->skip_len) {
	if (lcp == trie->skip_len) {
	    /* fully matched and more bits to check, go down the trie */
	    bit = (key[KEY_BIT2IDX(length - lcp)] &			\
		   SHL(1,(length - lcp - 1) % _NUM_WORD_BITS_, _NUM_WORD_BITS_)) ? 1:0;
	    
	    if (trie->child[bit].child_node) {
		rv = _trie_find_bpm(trie->child[bit].child_node, key, length - lcp - 1, bpm_length);
		/* on the way back, start bpm_length accumulation when encounter first non-0 bpm */
		if (*bpm_length >= 0) {
		    /* child node has non-zero bpm, just need to accumulate skip_len and branch bit */
		    *bpm_length += (trie->skip_len+1);
		    return rv;
		} else if (trie->bpm & BITMASK(trie->skip_len+1)) {
		    /* first non-zero bmp on the way back */
		    BITGETLSBSET(trie->bpm, trie->skip_len, local_bpm);
		    if (local_bpm >= 0) {
                        *bpm_length = trie->skip_len - local_bpm;
		    }
		}
		/* on the way back, and so far all bpm are 0 */
		return rv;
	    }
	}
    }

    /* no need to go further, we find whatever bits matched and 
     * check that part of the bpm mask
     */
    local_bpm_mask = trie->bpm & (~(BITMASK(trie->skip_len-lcp)));
    if (local_bpm_mask & BITMASK(trie->skip_len+1)) {
	/* first non-zero bmp on the way back */
	BITGETLSBSET(local_bpm_mask, trie->skip_len, local_bpm);
	if (local_bpm >= 0) {
	    *bpm_length = trie->skip_len - local_bpm;
	}
    }

    return rv;
}

/*
 * Function:
 *     trie_find_prefix_bpm
 * Purpose:
 *    Given a key/length return the Best prefix match length
 *    key/bpm_pfx_len will be the BPM for the key/length
 *    using the bpm info in the trie database
 */
int trie_find_prefix_bpm(trie_t *trie, 
                         unsigned int *key, 
                         unsigned int length,
                         unsigned int *bpm_pfx_len)
{
    /* Return: SOC_E_EMPTY is not bpm bit is found */
    int rv = SOC_E_EMPTY, bpm=0;

    if (!trie || !key || !bpm_pfx_len ) {
	return SOC_E_PARAM;
    }

    bpm = -1;
    if (trie->trie) {
	if (trie->v6_key) {
	    rv = _trie_v6_find_bpm(trie->trie, key, length, &bpm);
	} else {
	    rv = _trie_find_bpm(trie->trie, key, length, &bpm);
	}

	if (SOC_SUCCESS(rv)) {
            /* all bpm bits are 0 */
            *bpm_pfx_len = (bpm < 0)? 0:(unsigned int)bpm;
        }
    }

    return rv;
}

/*
 * Function:
 *   _trie_skip_node_alloc
 * Purpose:
 *   create a chain of trie_node_t that has the payload at the end.
 *   each node in the chain can skip upto _MAX_SKIP_LEN number of bits,
 *   while the child pointer in the chain represent 1 bit. So totally
 *   each node can absorb (_MAX_SKIP_LEN+1) bits.
 * Input:
 *   key      --  
 *   bpm      --  
 *   msb      --  
 *   skip_len --  skip_len of the whole chain
 *   payload  --  payload node we want to insert
 *   count    --  child count
 * Output:
 *   node     -- return pointer of the starting node of the chain.
 */
static int _trie_skip_node_alloc(trie_node_t **node, 
				 unsigned int *key, 
				 /* bpm bit map if bpm management is required, passing null skips bpm management */
				 unsigned int *bpm, 
				 unsigned int msb, /* NOTE: valid msb position 1 based, 0 means skip0/0 node */
				 unsigned int skip_len,
				 trie_node_t *payload,
				 unsigned int count) /* payload count underneath - mostly 1 except some tricky cases */
{
    int lsb=0, msbpos=0, lsbpos=0, bit=0, index;
    trie_node_t *child = NULL, *skip_node = NULL;

    /* calculate lsb bit position, also 1 based */
    lsb = ((msb)? msb + 1 - skip_len : msb);

    assert(((int)msb >= 0) && (lsb >= 0));

    if (!node || !key || !payload || msb > _MAX_KEY_LEN_ || msb < skip_len) return SOC_E_PARAM;

    if (msb) {
        for (index = BITS2SKIPOFF(lsb), lsbpos = lsb - 1; index <= BITS2SKIPOFF(msb); index++) {
	    /* each loop process _MAX_SKIP_LEN number of bits?? */
            if (lsbpos == lsb-1) {
		/* (lsbpos == lsb-1) is only true for first node (loop) here */
                skip_node = payload;
            } else {
		/* other nodes need to be created */
                skip_node = sal_alloc(sizeof(trie_node_t), "trie_node");
            }

	    /* init memory */
            sal_memset(skip_node, 0, sizeof(trie_node_t));

	    /* calculate msb bit position of current chunk of bits we are processing */
            msbpos = index * _MAX_SKIP_LEN_ - 1;
            if (msbpos > msb-1) msbpos = msb-1;

	    /* calculate the skip_len of the created node */
            if (msbpos - lsbpos < _MAX_SKIP_LEN_) {
                skip_node->skip_len = msbpos - lsbpos + 1;
            } else {
                skip_node->skip_len = _MAX_SKIP_LEN_;
            }

            /* calculate the skip_addr (skip_length number of bits).
	     * skip might be skipping bits on 2 different words 
             * if msb & lsb spawns 2 word boundary in worst case
	     */
            if (BITS2WORDS(msbpos+1) != BITS2WORDS(lsbpos+1)) {
                /* pull snippets from the different words & fuse */
                skip_node->skip_addr = key[KEY_BIT2IDX(msbpos+1)] & MASK((msbpos+1) % _NUM_WORD_BITS_); 
                skip_node->skip_addr = SHL(skip_node->skip_addr, 
                                           skip_node->skip_len - ((msbpos+1) % _NUM_WORD_BITS_),
                                           _NUM_WORD_BITS_);
                skip_node->skip_addr |= SHR(key[KEY_BIT2IDX(lsbpos+1)],(lsbpos % _NUM_WORD_BITS_),_NUM_WORD_BITS_);
            } else {
                skip_node->skip_addr = SHR(key[KEY_BIT2IDX(msbpos+1)], (lsbpos % _NUM_WORD_BITS_),_NUM_WORD_BITS_);
            }

	    /* set up the chain of child pointer, first node has no child since "child" was inited to NULL */
            if (child) {
                skip_node->child[bit].child_node = child;
            }

	    /* calculate child pointer for next loop. NOTE: skip_addr has not been masked
	     * so we still have the child bit in the skip_addr here.
	     */
            bit = (skip_node->skip_addr & SHL(1, skip_node->skip_len - 1,_MAX_SKIP_LEN_)) ? 1:0;

	    /* calculate node type */
            if (lsbpos == lsb-1) {
		/* first node is payload */
                skip_node->type = PAYLOAD;
            } else {
		/* other nodes are internal nodes */
                skip_node->type = INTERNAL;
            }

	    /* all internal nodes will have the same "count" as the payload node */
            skip_node->count = count;

            /* advance lsb to next word */
            lsbpos += skip_node->skip_len;

	    /* calculate bpm of the skip_node */
            if (bpm) {
		if (lsbpos == _MAX_KEY_LEN_) {
		    /* parent node is 0/0, so there is no branch bit here */
		    skip_node->bpm = _key_get_bits(bpm, lsbpos, skip_node->skip_len, 1);
		} else {
		    skip_node->bpm = _key_get_bits(bpm, lsbpos+1, skip_node->skip_len+1, 1);
		}
            }
            
            /* for all child nodes 0/1 is implicitly obsorbed on parent */
            if (msbpos != msb-1) {
		/* msbpos == (msb-1) is only true for the first node */
		skip_node->skip_len--;
	    }
            

            skip_node->skip_addr &= MASK(skip_node->skip_len);
            child = skip_node;
        } 
    } else {
	/* skip_len == 0 case, create a payload node with skip_len = 0 and bpm should be 1 bits only
	 * bit 0 and bit "skip_len" are same bit (bit 0).
	 */
        skip_node = payload;
        sal_memset(skip_node, 0, sizeof(trie_node_t));  
        skip_node->type = PAYLOAD;   
        skip_node->count = count;
        if (bpm) {
            skip_node->bpm =  _key_get_bits(bpm,1,1,0);
        }
    }

    *node = skip_node;
    return SOC_E_NONE;
}

static int _trie_insert(trie_node_t *trie, 
			unsigned int *key, 
			/* bpm bit map if bpm management is required, passing null skips bpm management */
			unsigned int *bpm, 
			unsigned int length,
			trie_node_t *payload, /* payload node */
			trie_node_t **child /* child pointer if the child is modified */)
{
    unsigned int lcp;
    int rv=SOC_E_NONE, bit=0;
    trie_node_t *node = NULL;

    if (!trie || !payload || !child || (length > _MAX_KEY_LEN_)) return SOC_E_PARAM;

    *child = NULL;

    
    lcp = lcplen(key, length, trie->skip_addr, trie->skip_len);

    /* insert cases:
     * 1 - new key could be the parent of existing node
     * 2 - new node could become the child of a existing node
     * 3 - internal node could be inserted and the key becomes one of child 
     * 4 - internal node is converted to a payload node */

    /* if the new key qualifies as new root do the inserts here */
    if (lcp == length) { /* guaranteed: length < _MAX_SKIP_LEN_ */
        if (trie->skip_len == lcp) {
            if (trie->type != INTERNAL) {
                /* duplicate */ 
                return SOC_E_EXISTS;
            } else { 
                /* change the internal node to payload node */
                _CLONE_TRIE_NODE_(payload,trie);
                sal_free(trie);
                payload->type = PAYLOAD;
                payload->count++;
                *child = payload;

                if (bpm) {
                    /* bpm at this internal mode must be same as the inserted pivot */
                    payload->bpm |= _key_get_bits(bpm, lcp+1, lcp+1, 1);
                    /* implicity preserve the previous bpm & set bit 0 -myself bit */
                } 
                return SOC_E_NONE;
            }
        } else { /* skip length can never be less than lcp implcitly here */
            /* this node is new parent for the old trie node */
            /* lcp is the new skip length */
            _CLONE_TRIE_NODE_(payload,trie);
            *child = payload;

            bit = (trie->skip_addr & SHL(1,trie->skip_len - length - 1,_MAX_SKIP_LEN_)) ? 1 : 0;
            trie->skip_addr &= MASK(trie->skip_len - length - 1);
            trie->skip_len  -= (length + 1);   
 
            if (bpm) {
                trie->bpm &= MASK(trie->skip_len+1);   
            }

            payload->skip_addr = (length > 0) ? key[KEY_BIT2IDX(length)] : 0;
            payload->skip_addr &= MASK(length);
            payload->skip_len  = length;
            payload->child[bit].child_node = trie;
            payload->child[!bit].child_node = NULL;
            payload->type = PAYLOAD;
            payload->count++;

            if (bpm) {
                payload->bpm = SHR(payload->bpm, trie->skip_len + 1,_NUM_WORD_BITS_);
                payload->bpm |= _key_get_bits(bpm, payload->skip_len+1, payload->skip_len+1, 1);
            }
        }
    } else if (lcp == trie->skip_len) {
        /* key length is implictly greater than lcp here */
        /* decide based on key's next applicable bit */
        bit = (key[KEY_BIT2IDX(length-lcp)] & 
               SHL(1,(length - lcp - 1) % _NUM_WORD_BITS_, _NUM_WORD_BITS_)) ? 1:0;

        if (!trie->child[bit].child_node) {
            /* the key is going to be one of the child of existing node */
            /* should be the child */
            rv = _trie_skip_node_alloc(&node, key, bpm,
				       length-lcp-1, /* 0 based msbit position */
				       length-lcp-1,
				       payload, 1);
            if (SOC_SUCCESS(rv)) {
                trie->child[bit].child_node = node;
                trie->count++;
            } else {
                soc_cm_print("\n Error on trie skip node allocaiton [%d]!!!!\n", rv);
            }
        } else { 
            rv = _trie_insert(trie->child[bit].child_node, 
                              key, bpm, length - lcp - 1, 
                              payload, child);
            if (SOC_SUCCESS(rv)) {
                trie->count++;
                if (*child != NULL) { /* chande the old child pointer to new child */
                    trie->child[bit].child_node = *child;
                    *child = NULL;
                }
            }
        }
    } else {
        trie_node_t *newchild = NULL;

        /* need to introduce internal nodes */
        node = sal_alloc(sizeof(trie_node_t), "trie-node");
        _CLONE_TRIE_NODE_(node, trie);

        rv = _trie_skip_node_alloc(&newchild, key, bpm,
				   ((lcp)?length-lcp-1:length-1),
				   length - lcp - 1,
				   payload, 1);
        if (SOC_SUCCESS(rv)) {
            bit = (key[KEY_BIT2IDX(length-lcp)] & 
                   SHL(1,(length - lcp - 1) % _NUM_WORD_BITS_,_NUM_WORD_BITS_)) ? 1: 0;

            node->child[!bit].child_node = trie;
            node->child[bit].child_node = newchild;
            node->type = INTERNAL;
            node->skip_addr = SHR(trie->skip_addr,trie->skip_len - lcp,_MAX_SKIP_LEN_);
            node->skip_len = lcp;
            node->count++;
            if (bpm) {
                node->bpm = SHR(node->bpm, trie->skip_len - lcp, _MAX_SKIP_LEN_);
            }
            *child = node;
            
            trie->skip_addr &= MASK(trie->skip_len - lcp - 1);
            trie->skip_len  -= (lcp + 1); 
            if (bpm) {
                trie->bpm &= MASK(trie->skip_len+1);      
            }
        } else {
            soc_cm_print("\n Error on trie skip node allocaiton [%d]!!!!\n", rv);
	    sal_free(node);
        }
    }

    return rv;
}

/*
 * Function:
 *     trie_insert
 * Purpose:
 *     Inserts provided prefix/length in to the trie
 */
int trie_insert(trie_t *trie, 
                unsigned int *key, 
                unsigned int *bpm,
                unsigned int length, 
                trie_node_t *payload)
{
    int rv = SOC_E_NONE;
    trie_node_t *child=NULL;

    if (!trie) return SOC_E_PARAM;

    if (trie->trie == NULL) {
        if (trie->v6_key) {
	        rv = _trie_v6_skip_node_alloc(&trie->trie, key, bpm, length, length, payload, 1);
	    } else {
           rv = _trie_skip_node_alloc(&trie->trie, key, bpm, length, length, payload, 1);
	    }
    } else {
	   if (trie->v6_key) {
	       rv = _trie_v6_insert(trie->trie, key, bpm, length, payload, &child);
	   } else {
	       rv = _trie_insert(trie->trie, key, bpm, length, payload, &child);
	   }
       if (child) { /* chande the old child pointer to new child */
           trie->trie = child;
       }
    }

    return rv;
}

int _trie_fuse_child(trie_node_t *trie, int bit)
{
    trie_node_t *child = NULL;
    int rv = SOC_E_NONE;

    if (trie->child[0].child_node && trie->child[1].child_node) {
        return SOC_E_PARAM;
    } 

    bit = (bit > 0)?1:0;
    child = trie->child[bit].child_node;

    if (child == NULL) {
        return SOC_E_PARAM;
    } else {
        if (trie->skip_len + child->skip_len + 1 <= _MAX_SKIP_LEN_) {

            if (trie->skip_len == 0) trie->skip_addr = 0; 

            if (child->skip_len < _MAX_SKIP_LEN_) {
                trie->skip_addr = SHL(trie->skip_addr,child->skip_len + 1,_MAX_SKIP_LEN_);
            }

            trie->skip_addr  |= SHL(bit,child->skip_len,_MAX_SKIP_LEN_);
            child->skip_addr |= trie->skip_addr;
            child->bpm       |= SHL(trie->bpm,child->skip_len+1,_MAX_SKIP_LEN_); 
            child->skip_len  += trie->skip_len + 1;

            /* do not free payload nodes as they are user managed */
            if (trie->type == INTERNAL) {
                sal_free(trie);
            }
        }
    }

    return rv;
}

static int _trie_delete(trie_node_t *trie, 
			unsigned int *key,
			unsigned int length,
			trie_node_t **payload,
			trie_node_t **child)
{
    unsigned int lcp;
    int rv=SOC_E_NONE, bit=0;
    trie_node_t *node = NULL;

    /* our algorithm should return before the length < 0, so this means
     * something wrong with the trie structure. Internal error?
     */
    if (!trie || !payload || !child || (length > _MAX_KEY_LEN_)) {
	return SOC_E_PARAM;
    }

    *child = NULL;

    /* check a section of key, return the number of matched bits and value of next bit */
    lcp = lcplen(key, length, trie->skip_addr, trie->skip_len);

    if (length > trie->skip_len) {

        if (lcp == trie->skip_len) {

            bit = (key[KEY_BIT2IDX(length-lcp)] & 
                   SHL(1,(length - lcp -1) % _NUM_WORD_BITS_,_NUM_WORD_BITS_)) ? 1:0;

            /* based on next bit branch left or right */
            if (trie->child[bit].child_node) {

	        /* has child node, keep searching */
                rv = _trie_delete(trie->child[bit].child_node, key, length - lcp - 1, payload, child);

	        if (rv == SOC_E_BUSY) {

                    trie->child[bit].child_node = NULL; /* sal_free the child */
                    rv = SOC_E_NONE;
                    trie->count--;

                    if (trie->type == INTERNAL) {

                        bit = (bit==0)?1:0;

                        if (trie->child[bit].child_node == NULL) {
                            /* parent and child connected, sal_free the middle-node itself */
                            sal_free(trie);
                            rv = SOC_E_BUSY;
                        } else {
                            /* fuse the parent & child */
                            if (trie->skip_len + trie->child[bit].child_node->skip_len + 1 <= 
                                _MAX_SKIP_LEN_) {
                                *child = trie->child[bit].child_node;
                                rv = _trie_fuse_child(trie, bit);
                                if (rv != SOC_E_NONE) {
                                    *child = NULL;
                                }
                            }
                        }
                    }
	        } else if (SOC_SUCCESS(rv)) {
                    trie->count--;
                    /* update child pointer if applicable */
                    if (*child != NULL) {
                        trie->child[bit].child_node = *child;
                        *child = NULL;
                    }
                }
            } else {
                /* no child node case 0: not found */
                rv = SOC_E_NOT_FOUND; 
            }

        } else { 
	    /* some bits are not matching, case 0: not found */
            rv = SOC_E_NOT_FOUND;
        }
    } else if (length == trie->skip_len) {
	/* when length equal to skip_len, unless this is a payload node
	 * and it's an exact match (lcp == length), we can not found a match
	 */ 
        if (!((lcp == length) && (trie->type == PAYLOAD))) {
	    rv = SOC_E_NOT_FOUND;
	} else {
            /* payload node can be deleted */
            /* if this node has 2 children update it to internal node */
            rv = SOC_E_NONE;

            if (trie->child[0].child_node && trie->child[1].child_node ) {
		/* the node has 2 children, update it to internal node */
                node = sal_alloc(sizeof(trie_node_t), "trie_node");
                _CLONE_TRIE_NODE_(node, trie);
                node->type = INTERNAL;
                node->count--;
                *child = node;
            } else if (trie->child[0].child_node || trie->child[1].child_node ) {
                /* if this node has 1 children fuse the children with this node */
                bit = (trie->child[0].child_node) ? 0:1;
                trie->count--;
                if (trie->skip_len + trie->child[bit].child_node->skip_len + 1 <= _MAX_SKIP_LEN_) {
		    /* able to fuse the node with its child node */
                    *child = trie->child[bit].child_node;
                    rv = _trie_fuse_child(trie, bit);
                    if (rv != SOC_E_NONE) {
                        *child = NULL;
                    }
                } else {
		    /* convert it to internal node, we need to alloc new memory for internal nodes
		     * since the old payload node memory will be freed by caller
		     */
                    node = sal_alloc(sizeof(trie_node_t), "trie_node");
                    _CLONE_TRIE_NODE_(node, trie);
                    node->type = INTERNAL;
                    *child = node;
                }
            } else {
                rv = SOC_E_BUSY;
            }

            *payload = trie;
        }
    } else {
	/* key length is shorter, no match if it's internal node,
	 * will not exact match even if this is a payload node
	 */
        rv = SOC_E_NOT_FOUND; /* case 0: not found */        
    }

    return rv;
}

/*
 * Function:
 *     trie_delete
 * Purpose:
 *     Deletes provided prefix/length in to the trie
 */
int trie_delete(trie_t *trie,
                unsigned int *key,
                unsigned int length,
                trie_node_t **payload)
{
    int rv = SOC_E_NONE;
    trie_node_t *child = NULL;

    if (trie->trie) {
	if (trie->v6_key) {
	    rv = _trie_v6_delete(trie->trie, key, length, payload, &child);
	} else {
	    rv = _trie_delete(trie->trie, key, length, payload, &child);
	}
        if (rv == SOC_E_BUSY) {
            /* the head node of trie was deleted, reset trie pointer to null */
            trie->trie = NULL;
            rv = SOC_E_NONE;
        } else if (rv == SOC_E_NONE && child != NULL) {
            trie->trie = child;
        }
    } else {
        rv = SOC_E_NOT_FOUND;
    }
    return rv;
}

/*
 * Function:
 *     trie_split
 * Purpose:
 *     Split the trie into 2 based on optimum pivot
 * NOTE:
 *     max_split_len -- split will make sure the split point
 *                has a length shorter or equal to the max_split_len
 *                unless this will cause a no-split (all prefixs
 *                stays below the split point)
 *     split_to_pair -- used only when the split point will be
 *                used to create a pair of tries later (i.e: dbucket
 *                pair. we assume the split point itself will always be
 *                put into 0* trie if itself is a payload/prefix)
 */
static int _trie_split(trie_node_t  *trie,
		       unsigned int *pivot,
		       unsigned int *length,
		       unsigned int *split_count,
		       trie_node_t **split_node,
		       trie_node_t **child,
		       const unsigned int max_count,
		       const unsigned int max_split_len,
		       const int split_to_pair,
		       unsigned int *bpm,
		       trie_split_states_e_t *state)
{
    int bit=0, rv=SOC_E_NONE;

    if (!trie || !pivot || !length || !split_node || max_count == 0 || !state) return SOC_E_PARAM;

    if (trie->child[0].child_node && trie->child[1].child_node) {
        bit = (trie->child[0].child_node->count > 
               trie->child[1].child_node->count) ? 0:1;
    } else {
        bit = (trie->child[0].child_node)?0:1;
    }

    /* start building the pivot */
    rv = _key_append(pivot, length, trie->skip_addr, trie->skip_len);
    if (SOC_FAILURE(rv)) return rv;

    if (bpm) {
        unsigned int scratch=0;
        rv = _bpm_append(bpm, &scratch, trie->bpm, trie->skip_len+1);
        if (SOC_FAILURE(rv)) return rv;        
    }

    {
	/*
	 * split logic to make sure the split length is shorter than the
	 * requested max_split_len, unless we don't actully split the
	 * tree if we stop here.
	 * if (*length > max_split_len) && (trie->count != max_count) {
	 *    need to split at or above this node. might need to split the node in middle
	 * } else if ((ABS(child count*2 - max_count) > ABS(count*2 - max_count)) ||
	 *            ((*length == max_split_len) && (trie->count != max_count))) {
	 *    (the check above imply trie->count != max_count, so also imply *length < max_split_len)
	 *    need to split at this node.
	 * } else {
	 *    keep searching, will be better split at longer pivot.
	 * }
	 */
	if ((*length > max_split_len) && (trie->count != max_count)) {
	    /* the pivot is getting too long, we better split at this node for
	     * better bucket capacity efficiency if we can. We can split if 
	     * the trie node has a count != max_count, which means the 
	     * resulted new trie will not have all pivots (FULL)
	     */ 
	    if ((TRIE_SPLIT_STATE_PAYLOAD_SPLIT == *state) && 
		(trie->type == INTERNAL)) {
		*state = TRIE_SPLIT_STATE_PAYLOAD_SPLIT_DONE;
	    } else {
		if (((*length - max_split_len) > trie->skip_len) && (trie->skip_len == 0)) {
		    /* the length is longer than max_split_len, and the trie->skip_len is 0,
		     * so the best we can do is use the node as the split point
		     */
		    *split_node = trie;
		    *split_count = trie->count;
		    
		    *state = TRIE_SPLIT_STATE_PRUNE_NODES;
		    return rv;
		}
		
		/* we need to insert a node and use it as split point */
		*split_node = sal_alloc(sizeof(trie_node_t), "trie_node");
		sal_memset((*split_node), 0, sizeof(trie_node_t));
		(*split_node)->type = INTERNAL;
		(*split_node)->count = trie->count;
		
		if ((*length - max_split_len) > trie->skip_len) {
		    /* the length is longer than the max_split_len, and the trie->skip_len is
		     * shorter than the difference (max_split_len pivot is not covered by this 
		     * node but covered by its parent, the best we can do is split at the branch
		     * lead to this node. we insert a skip_len=0 node and use it as split point
		     */
		    (*split_node)->skip_len = 0;
		    (*split_node)->skip_addr = 0;
		    (*split_node)->bpm = (trie->bpm >> trie->skip_len);
		    
		    if (_BITGET(trie->skip_addr, (trie->skip_len-1))) {
			(*split_node)->child[1].child_node = trie;
		    } else {
			(*split_node)->child[0].child_node = trie;
		    }
		    
		    /* update the current node to reflect the node inserted */
		    trie->skip_len = trie->skip_len - 1;
		    
		    /* the split point is with length max_split_len */		
		    *length -= trie->skip_len;		
		} else {
		    /* the length is longer than the max_split_len, and the trie->skip_len is
		     * longer than the difference (max_split_len pivot is covered by this 
		     * node, we insert a node with length = max_split_len and use it as split point
		     */
		    (*split_node)->skip_len = trie->skip_len - (*length - max_split_len);
		    (*split_node)->skip_addr = (trie->skip_addr >> (*length - max_split_len));
		    (*split_node)->bpm = (trie->bpm >> (*length - max_split_len));
		    
		    if (_BITGET(trie->skip_addr, (*length-max_split_len-1))) {
			(*split_node)->child[1].child_node = trie;
		    } else {
			(*split_node)->child[0].child_node = trie;
		    }
		    
		    /* update the current node to reflect the node inserted */
		    trie->skip_len = *length - max_split_len - 1;
		    
		    /* the split point is with length max_split_len */
		    *length = max_split_len;
		}
		
		trie->skip_addr = trie->skip_addr & BITMASK(trie->skip_len);
		trie->bpm = trie->bpm & BITMASK(trie->skip_len + 1);
		
		/* there is no need to update the parent node's child_node pointer
		 * to the "trie" node since we will split here and the parent node's
		 * child_node pointer will be set to NULL later
		 */
		*split_count = trie->count;
		rv = _key_shift_right(bpm, trie->skip_len+1);
		
		if (SOC_SUCCESS(rv)) {
		    rv = _key_shift_right(pivot, trie->skip_len+1);
		}
		*state = TRIE_SPLIT_STATE_PRUNE_NODES;
		return rv;
	    }
	} else if ( ((*length == max_split_len) && (trie->count != max_count)) ||
		    (ABS(trie->child[bit].child_node->count * 2 - max_count) >
		     ABS(trie->count * 2 - max_count))) {
	    /* 
	     * (1) when the node is at the max_split_len and if used as spliting point
	     * the resulted trie will not have all pivots (FULL). we should split
	     * at this node.
	     * (2) when the node is at the max_split_len and if the resulted trie
	     * will have all pivots (FULL), we fall through to keep searching
	     * (3) when the node is shorter than the max_split_len and the node
	     * has a more even pivot distribution compare to it's child, we
	     * can split at this node .
	     * 
	     * NOTE 1:
	     *  ABS(trie->count * 2 - max_count) actually means
	     *  ABS(trie->count - (max_count - trie->count))
	     * which means the count's distance to half depth of the bucket
	     * NOTE 2:
	     *  when trie->count == max_count, the above check will be FALSE
	     *  so here it guarrantees *length < max_split_len. We don't
	     *  need to further split this node.
	     */
	    *split_node = trie;
	    *split_count = trie->count;
	    
	    if ((TRIE_SPLIT_STATE_PAYLOAD_SPLIT == *state) && 
		(trie->type == INTERNAL)) {
		*state = TRIE_SPLIT_STATE_PAYLOAD_SPLIT_DONE;
	    } else {
		*state = TRIE_SPLIT_STATE_PRUNE_NODES;
		return rv;
	    }
	} else {
	    /* we can not split at this node, keep searching, it's better to 
	     * split at longer pivot
	     */
	    rv = _key_append(pivot, length, bit, 1);
	    if (SOC_FAILURE(rv)) return rv;
	    
	    rv = _trie_split(trie->child[bit].child_node, 
			     pivot, length,
			     split_count, split_node,
			     child, max_count, max_split_len,
			     split_to_pair, bpm, state);
	}
    }

    /* free up internal nodes if applicable */
    switch(*state) {
    case TRIE_SPLIT_STATE_PAYLOAD_SPLIT_DONE:
         if (trie->type == PAYLOAD) {
            *state = TRIE_SPLIT_STATE_PRUNE_NODES;
            *split_node = trie;
            *split_count = trie->count;
        } else {
            /* shift the pivot to right to ignore this internal node */
            rv = _key_shift_right(pivot, trie->skip_len+1);
            assert(*length >= trie->skip_len + 1);
            *length -= (trie->skip_len + 1);
        }
        break;

    case TRIE_SPLIT_STATE_PRUNE_NODES:
        if (trie->count == *split_count) {
            /* if the split point has associate internal nodes they have to
             * be cleaned up */  
            assert(trie->type == INTERNAL);
            assert(!(trie->child[0].child_node && trie->child[1].child_node));
            sal_free(trie);
        } else {
            assert(*child == NULL);
            /* fuse with child if possible */
            trie->child[bit].child_node = NULL; 
            bit = (bit==0)?1:0;
            trie->count -= *split_count; 

            /* optimize more */
            if ((trie->type == INTERNAL) && 
                (trie->skip_len + 
                 trie->child[bit].child_node->skip_len + 1 <= _MAX_SKIP_LEN_)) {
                *child = trie->child[bit].child_node;
                rv = _trie_fuse_child(trie, bit);
                if (rv != SOC_E_NONE) {
                    *child = NULL;
                }
            }
            *state = TRIE_SPLIT_STATE_DONE;
        }
        break;

    case TRIE_SPLIT_STATE_DONE:
        /* adjust parent's count */     
        assert(*split_count > 0);
        assert(trie->count >= *split_count);
        
        /* update the child pointer if child was pruned */
        if (*child != NULL) {
            trie->child[bit].child_node = *child;
            *child = NULL;
        } 
        trie->count -= *split_count;
        break;
        
    default:
        break;
    }
    
    return rv;
}

/*
 * Function:
 *     trie_split
 * Purpose:
 *     Split the trie into 2 based on optimum pivot
 * Note:
 *     we need to make sure the length is shorter than
 *     the max_split_len (for capacity optimization) if 
 *     possible. We should ignore the max_split_len
 *     if that will result into trie not spliting
 */
int trie_split(trie_t *trie,
	       const unsigned int max_split_len,
	       const int split_to_pair,
               unsigned int *pivot,
               unsigned int *length,
               trie_node_t **split_trie_root,
               unsigned int *bpm,
               uint8 payload_node_split)
{
    int rv = SOC_E_NONE;
    unsigned int split_count=0, max_count=0;
    trie_node_t *child = NULL, *node=NULL, clone;
    trie_split_states_e_t state = TRIE_SPLIT_STATE_NONE;

    if (!trie || !pivot || !length || !split_trie_root) return SOC_E_PARAM;

    *length = 0;

    if (trie->trie) {

        if (payload_node_split) state = TRIE_SPLIT_STATE_PAYLOAD_SPLIT;

	max_count = trie->trie->count;

	if (trie->v6_key) {	    
	    sal_memset(pivot, 0, sizeof(unsigned int) * BITS2WORDS(_MAX_KEY_LEN_144_));
	    if (bpm) {
		sal_memset(bpm, 0, sizeof(unsigned int) * BITS2WORDS(_MAX_KEY_LEN_144_));
	    }
	    rv = _trie_v6_split(trie->trie, pivot, length, &split_count, split_trie_root,
				&child, max_count, max_split_len, split_to_pair, bpm, &state);
	} else {
	    sal_memset(pivot, 0, sizeof(unsigned int) * BITS2WORDS(_MAX_KEY_LEN_48_));
	    if (bpm) {
		sal_memset(bpm, 0, sizeof(unsigned int) * BITS2WORDS(_MAX_KEY_LEN_48_));
	    }

	    rv = _trie_split(trie->trie, pivot, length, &split_count, split_trie_root,
			     &child, max_count, max_split_len, split_to_pair, bpm, &state);
	}
        if (SOC_SUCCESS(rv) && (TRIE_SPLIT_STATE_DONE == state)) {
            /* adjust parent's count */     
            assert(split_count > 0);
            assert(trie->trie->count >= split_count || (*split_trie_root)->count >= split_count);
            /* update the child pointer if child was pruned */
            if (child != NULL) {
                trie->trie = child;
            }

            sal_memcpy(&clone, *split_trie_root, sizeof(trie_node_t));
            child = *split_trie_root;

            /* take advantage of thie function by passing in internal or payload node whatever
             * is the new root. If internal the function assumed it as payload node & changes type.
             * But this method is efficient to reuse the last internal or payload node possible to 
             * implant the new pivot */
	    if (trie->v6_key) {	    
		rv = _trie_v6_skip_node_alloc(&node, pivot, NULL,
					      *length, *length,
					      child, child->count);
	    } else {
		rv = _trie_skip_node_alloc(&node, pivot, NULL,
					   *length, *length,
					   child, child->count);
	    }

            if (SOC_SUCCESS(rv)) {
                if (clone.type == INTERNAL) {
                    child->type = INTERNAL; /* since skip alloc would have reset it to payload */
                }
                child->child[0].child_node = clone.child[0].child_node;
                child->child[1].child_node = clone.child[1].child_node;
                *split_trie_root = node;
            } 
        } else {
            soc_cm_print("!!!! Failed to split the trie error:%d state: %d !!!\n",
                         rv, state);
        }
    } else {
        rv = SOC_E_PARAM;
    }

    return rv;
}

/*
 * Function:
 *     trie_unsplit
 * Purpose:
 *     unsplit or fuse the child trie with parent trie
 */
int trie_unsplit(trie_t *parent_trie,
                 trie_t *child_trie,
                 unsigned int *child_pivot,
                 unsigned int length)
{
    int rv=SOC_E_NONE;

    if (!parent_trie || !child_trie) return SOC_E_PARAM;

    
    assert(0);

    return rv;
}

/*
 * Function:
 *     _trie_traverse_propagate_prefix
 * Purpose:
 *     calls back applicable payload object is affected by prefix updates 
 * NOTE:
 *     propagation stops once any callback funciton return something otherthan SOC_E_NONE
 *     tcam propagation code should return !SOC_E_NONE so that callback only happen once.
 * 
 *     other propagation code should always return SOC_E_NONE so that callback will
 *     happen on all pivot.
 */
int _trie_traverse_propagate_prefix(trie_node_t *trie,
                                    trie_propagate_cb_f cb,
                                    trie_bpm_cb_info_t *cb_info,
                                    unsigned int mask)
{
    int rv = SOC_E_NONE, index=0;

    if (!trie || !cb || !cb_info) return SOC_E_PARAM;

    if ((trie->bpm & mask) == 0) {
        /* call back the payload object if applicable */
        if (PAYLOAD == trie->type) {
            rv = cb(trie, cb_info);
	    if (SOC_FAILURE(rv)) {
		/* callback stops once any callback function not returning SOC_E_NONE */
		return rv;
	    }
        }

        for (index=0; index < _MAX_CHILD_ && SOC_SUCCESS(rv); index++) {
            if (trie->child[index].child_node && trie->child[index].child_node->bpm == 0) {
                rv = _trie_traverse_propagate_prefix(trie->child[index].child_node,
                                                     cb, cb_info, MASK(32));
            }

	    if (SOC_FAILURE(rv)) {
		/* callback stops once any callback function not returning SOC_E_NONE */
		return rv;
	    }
        }
    }

    return rv;
}

/*
 * Function:
 *     _trie_propagate_prefix
 * Purpose:
 *  Propogate prefix BPM. If the propogation starts from intermediate pivot on
 *  the trie, then the prefix length has to be appropriately adjusted or else 
 *  it will end up with ill updates. 
 *  Assumption: the prefix length is adjusted as per trie node on which is starts from.
 *  If node == head node then adjust is none
 *     node == pivot, then prefix length = org len - pivot len          
 */
static int _trie_propagate_prefix(trie_node_t *trie,
				  unsigned int *pfx,
				  unsigned int len,
				  unsigned int add, /* 0-del/1-add */
				  trie_propagate_cb_f cb,
				  trie_bpm_cb_info_t *cb_info)
{
    int rv = SOC_E_NONE; /*, index;*/
    unsigned int bit=0, lcp=0;

    if (!pfx || !trie || (len > _MAX_KEY_LEN_) || !cb || !cb_info) return SOC_E_PARAM;

    if (len > 0) {
        /* BPM bit maps has to be updated before propagation */
        lcp = lcplen(pfx, len, trie->skip_addr, trie->skip_len);            
        /* if the lcp is less than prefix length the prefix is not applicable
         * for any propagation */
        if (lcp < ((len>trie->skip_len)?trie->skip_len:len)) {
            return SOC_E_NONE; 
        } else { 
            if (len > trie->skip_len) {
                bit = _key_get_bits(pfx, len-lcp, 1, 0);
                if (!trie->child[bit].child_node) return SOC_E_NONE;
                rv = _trie_propagate_prefix(trie->child[bit].child_node,
                                            pfx, len-lcp-1, add, cb, cb_info);
            } else {
                /* pfx is <= trie skip len */
                if (!add) { /* delete */
                    _BITCLR(trie->bpm, trie->skip_len - len);
                }
                
                /* update bit map and propagate if applicable */
                if ((trie->bpm & BITMASK(trie->skip_len - len)) == 0) {
                    rv = _trie_traverse_propagate_prefix(trie, cb, 
                                                         cb_info, 
                                                         BITMASK(trie->skip_len - len));
                    if (SOC_E_LIMIT == rv) rv = SOC_E_NONE;
                } else if (add && (trie->bpm & BITMASK(trie->skip_len - len + 1))) {
		    /* if adding, and bpm of this node is the specified prefix
		     * also propagate. (this is really update case)
		     */
                    rv = _trie_traverse_propagate_prefix(trie, cb, 
                                                         cb_info, 
                                                         BITMASK(trie->skip_len - len));
                    if (SOC_E_LIMIT == rv) rv = SOC_E_NONE;
		}
                
                if (add && SOC_SUCCESS(rv)) {
                    /* this is the case where child bit is the new prefix */
                    _BITSET(trie->bpm, trie->skip_len - len);
                }
            }
        }
    } else {
        if (!add) { /* delete */
            _BITCLR(trie->bpm, trie->skip_len);
        }

        if ((trie->bpm == 0) || 
	    (add && ((trie->bpm & BITMASK(trie->skip_len)) == 0))) {
	    /* if adding, and bpm of this node is the specified prefix
	     * also propagate. (this is really update case)
	     */
            rv = _trie_traverse_propagate_prefix(trie, cb, cb_info, BITMASK(trie->skip_len));
            if (SOC_E_LIMIT == rv) rv = SOC_E_NONE;
        }
        
        if (add && SOC_SUCCESS(rv)) { /* add */
            /* this is the case where child bit is the new prefix */
            _BITSET(trie->bpm, trie->skip_len);
        }
    }

    return rv;
}

/*
 * Function:
 *     _trie_propagate_prefix_validate
 * Purpose:
 *  validate that the provided prefix is valid for propagation.
 *  The added prefix which was member of a shorter pivot's domain 
 *  must never be more specific than another pivot encounter if any
 *  in the path
 */
static int _trie_propagate_prefix_validate(trie_node_t *trie,
					   unsigned int *pfx,
					   unsigned int len)
{
    unsigned int lcp=0, bit=0;

    if (!trie || !pfx) return SOC_E_PARAM;

    if (len == 0) return SOC_E_NONE;

    lcp = lcplen(pfx, len, trie->skip_addr, trie->skip_len);

    if (lcp == trie->skip_len) {
        if (PAYLOAD == trie->type) return SOC_E_PARAM;
	if (len == lcp) return SOC_E_NONE;
        bit = _key_get_bits(pfx, len-lcp, 1, 0);
        if (!trie->child[bit].child_node) return SOC_E_NONE;
        return _trie_propagate_prefix_validate(trie->child[bit].child_node,
                                               pfx, len-lcp);
    } 

    return SOC_E_NONE;
}

int _trie_init_propagate_info(unsigned int *pfx,
			      unsigned int len,
			      trie_propagate_cb_f cb,
			      trie_bpm_cb_info_t *cb_info)
{
    cb_info->pfx = pfx;
    cb_info->len = len;
    return SOC_E_NONE;
}

/*
 * Function:
 *     trie_pivot_propagate_prefix
 * Purpose:
 *  Propogate prefix BPM from a given pivot.      
 */
int trie_pivot_propagate_prefix(trie_node_t *pivot,
                                unsigned int pivot_len,
                                unsigned int *pfx,
                                unsigned int len,
                                unsigned int add, /* 0-del/1-add */
                                trie_propagate_cb_f cb,
                                trie_bpm_cb_info_t *cb_info)
{
    int rv = SOC_E_NONE;

    if (!pfx || !pivot || (len > _MAX_KEY_LEN_) ||
        (pivot_len >  _MAX_KEY_LEN_) || (len < pivot_len) ||
        (pivot->type != PAYLOAD) || !cb || !cb_info ||
        !cb_info->pfx) {
	return SOC_E_PARAM;
    }

    _trie_init_propagate_info(pfx,len,cb,cb_info);
    len -= pivot_len;

    if (len > 0) {
        unsigned int bit =  _key_get_bits(pfx, len, 1, 0);

        if (pivot->child[bit].child_node) {
            /* validate if the pivot provided is correct */
            rv = _trie_propagate_prefix_validate(pivot->child[bit].child_node,
						 pfx, len-1);
            if (SOC_SUCCESS(rv)) {
                rv = _trie_propagate_prefix(pivot->child[bit].child_node,
                                            pfx, len-1,
                                            add, cb, cb_info);
            }
        } /* else nop, nothing to propagate on this path end */
    } else {
        /* pivot == prefix */
        rv = _trie_propagate_prefix(pivot, pfx, pivot->skip_len,
                                    add, cb, cb_info);
    }

    return rv;
}

/*
 * Function:
 *     trie_propagate_prefix
 * Purpose:
 *  Propogate prefix BPM on a given trie.      
 */
int trie_propagate_prefix(trie_t *trie,
                          unsigned int *pfx,
                          unsigned int len,
                          unsigned int add, /* 0-del/1-add */
                          trie_propagate_cb_f cb,
                          trie_bpm_cb_info_t *cb_info)
{
    int rv=SOC_E_NONE;

    if (!pfx || !trie || !trie->trie || !cb || !cb_info || !cb_info->pfx) {
	return SOC_E_PARAM;
    }

    _trie_init_propagate_info(pfx,len,cb,cb_info);

    if (SOC_SUCCESS(rv)) {
	if (trie->v6_key) {	    
	    rv = _trie_v6_propagate_prefix(trie->trie, pfx, len, add, 
					   cb, cb_info);
	} else {
	    rv = _trie_propagate_prefix(trie->trie, pfx, len, add, 
					cb, cb_info);
	}
    }

    return rv;
}


/*
 * Function:
 *     trie_init
 * Purpose:
 *     allocates a trie & initializes it
 */
int trie_init(unsigned int max_key_len, trie_t **ptrie)
{
    trie_t *trie = sal_alloc(sizeof(trie_t), "trie-node");
    sal_memset(trie, 0, sizeof(trie_t));

    if (max_key_len == _MAX_KEY_LEN_48_) {
        trie->v6_key = FALSE;
    } else if (max_key_len == _MAX_KEY_LEN_144_) {
        trie->v6_key = TRUE;
    } else {
        sal_free(trie);
        return SOC_E_PARAM;
    }

    trie->trie = NULL; /* means nothing is on teie */
    *ptrie = trie;
    return SOC_E_NONE;
}

/*
 * Function:
 *     trie_destroy
 * Purpose:
 *     destroys a trie 
 */
int trie_destroy(trie_t *trie)
{
    
    sal_free(trie);
    return SOC_E_NONE;
}

#if 0 
/****************/
/** unit tests **/
/****************/
#define _NUM_KEY_ (4 * 1024)
#define _VRF_LEN_ 16
/*#define VERBOSE 
  #define LOG*/
/* use the followign diag shell command to run this test:
 * tr c3sw test=tmu_trie_ut
 */
typedef struct _payload_s {
    trie_node_t node; /*trie node */
    dq_t        listnode; /* list node */
    union {
        trie_t      *trie;
        trie_node_t pfx_trie_node;
    } info;
    unsigned int key[BITS2WORDS(_MAX_KEY_LEN_)];
    unsigned int len;
} payload_t;

int ut_print_payload_node(trie_node_t *payload, void *datum)
{
    payload_t *pyld;

    if (payload && payload->type == PAYLOAD) {
        pyld = TRIE_ELEMENT_GET(payload_t*, payload, node);
        soc_cm_print(" key[0x%08x:0x%08x] Length:%d \n",
                     pyld->key[0], pyld->key[1], pyld->len);
    }
    return SOC_E_NONE;
}

int ut_print_prefix_payload_node(trie_node_t *payload, void *datum)
{
    payload_t *pyld;

    if (payload && payload->type == PAYLOAD) {
        pyld = TRIE_ELEMENT_GET(payload_t*, payload, info.pfx_trie_node);
        soc_cm_print(" key[0x%08x:0x%08x] Length:%d \n",
                     pyld->key[0], pyld->key[1], pyld->len);
    }
    return SOC_E_NONE;
}

int ut_check_duplicate(payload_t *pyld, int pyld_vector_size)
{
    int i=0;

    assert(pyld);

    for (i=0; i < pyld_vector_size; i++) {
        if (pyld[i].len == pyld[pyld_vector_size].len &&
            pyld[i].key[0] == pyld[pyld_vector_size].key[0] && 
            pyld[i].key[1] == pyld[pyld_vector_size].key[1]) {
            break;
        }
    }

    return ((i == pyld_vector_size)?0:1);
}


int tmu_taps_util_get_bpm_pfx_ut(void) 
{
    int rv;
    unsigned int pfx_len=0;
    unsigned int bpm[] = { 0x80, 0x00101010 }; /* 40th bit msb */

    /* v4 test */
    rv = taps_get_bpm_pfx(&bpm[0], 48, _MAX_KEY_LEN_48_, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 48-4) return SOC_E_FAIL;

    bpm[0] = 0;
    bpm[1] = 0x00010000;
    rv = taps_get_bpm_pfx(&bpm[0], 48, _MAX_KEY_LEN_48_, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 48-16) return SOC_E_FAIL;

    bpm[0] = 0;
    bpm[1] = 0x80000000;
    rv = taps_get_bpm_pfx(&bpm[0], 48, _MAX_KEY_LEN_48_, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 48-31) return SOC_E_FAIL;

    bpm[0] = 0x1;
    bpm[1] = 0x80000000;
    rv = taps_get_bpm_pfx(&bpm[0], 48, _MAX_KEY_LEN_48_, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 48-31) return SOC_E_FAIL;

    bpm[0] = 0;
    bpm[1] = 0;
    rv = taps_get_bpm_pfx(&bpm[0], 48, _MAX_KEY_LEN_48_, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 0) return SOC_E_FAIL;

    /* v6 test */
    bpm[0] = 0x80;
    bpm[1] = 0x00101010;
    rv = taps_get_bpm_pfx(&bpm[0], 144, _MAX_KEY_LEN_144_, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 144-4) return SOC_E_FAIL;

    bpm[0] = 0;
    bpm[1] = 0x00010000;
    rv = taps_get_bpm_pfx(&bpm[0], 144, _MAX_KEY_LEN_144_, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 144-16) return SOC_E_FAIL;

    bpm[0] = 0;
    bpm[1] = 0x80000000;
    rv = taps_get_bpm_pfx(&bpm[0], 144, _MAX_KEY_LEN_144_, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 144-31) return SOC_E_FAIL;

    bpm[0] = 0x1;
    bpm[1] = 0x80000000;
    rv = taps_get_bpm_pfx(&bpm[0], 144, _MAX_KEY_LEN_144_, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 144-31) return SOC_E_FAIL;

    bpm[0] = 0;
    bpm[1] = 0;
    rv = taps_get_bpm_pfx(&bpm[0], 144, _MAX_KEY_LEN_144_, &pfx_len);
    if (SOC_FAILURE(rv)) return SOC_E_FAIL;
    if (pfx_len != 0) return SOC_E_FAIL;

    return SOC_E_NONE;
}

int tmu_taps_kshift_ut(void) 
{
    unsigned int key[] = { 0x1234, 0x12345678 }, length=0;

    /* v4 tests */
    _key_shift_left(key, 8);
    if (key[0] != 0x3412 && key[1] != 0x34567800) {
        return SOC_E_FAIL;
    }

    key[0] = 0;
    key[1] = 0x12345678;
    _key_shift_left(key, 15);    

    if (key[0] != (0x12345678 >> 17) && key[1] != (0x12345678 << 15)) {
        return SOC_E_FAIL;
    }

    key[0] = 0x1234;
    key[1] = 0xdeadbeef;
    _key_shift_left(key, 0);    
    if (key[0] != 0x1234 && key[1] != 0xdeadbeef) {
        return SOC_E_FAIL;
    }

    key[0] = 0;
    key[1] = 0;
    _key_append(key, &length, 0xba5e, 16);
    if (key[0] != 0 && key[1] != 0xba5e) {
        return SOC_E_FAIL;
    }

    _key_append(key, &length, 0x3a5eba11, 31);
    if (key[0] != 0xba5e && key[1] != 0x3a5eba11) {
        return SOC_E_FAIL;
    }

    /* v6 tests */

    return SOC_E_NONE;
}

int tmu_trie_split_ut(unsigned int seed) 
{
    int index, rv = SOC_E_NONE, numkey=0, id=0;
    trie_t *trie, *newtrie;
    trie_node_t *newroot;
    payload_t *pyld = sal_alloc(_NUM_KEY_ * sizeof(payload_t), "unit-test");
    trie_node_t *pyldptr = NULL;
    unsigned int pivot[_MAX_KEY_WORDS_], length;

    for (id=0; id < 4; id++) {
        switch(id) {
        case 0:  /* 1:1 split */
            pyld[0].key[0] = 0; pyld[0].key[1] = 0x10; pyld[0].len = _VRF_LEN_ + 8;  /* v=0 p=0x10000000/8  */
            pyld[1].key[0] = 0; pyld[1].key[1] = 0x1000; pyld[1].len = _VRF_LEN_ + 16; /* v=0 p=0x10000000/16 */
            pyld[2].key[0] = 0; pyld[2].key[1] = 0x100000; pyld[2].len = _VRF_LEN_ + 24; /* v=0 p=0x10000000/24 */
            pyld[3].key[0] = 0; pyld[3].key[1] = 0x10000000; pyld[3].len = _VRF_LEN_ + 32; /* v=0 p=0x10000000/48 */
            numkey = 4;
            break;
        case 1: /* 1:1 split */
            pyld[0].key[0] = 0; pyld[0].key[1] = 0x10000000; pyld[0].len = _VRF_LEN_ + 32; /* v=0 p=0x10000000/32 */
            pyld[1].key[0] = 0; pyld[1].key[1] = 0x10000001; pyld[1].len = _VRF_LEN_ + 32; /* v=0 p=0x10000001/32 */
            pyld[2].key[0] = 0; pyld[2].key[1] = 0x10000002; pyld[2].len = _VRF_LEN_ + 32; /* v=0 p=0x10000002/32 */
            pyld[3].key[0] = 0; pyld[3].key[1] = 0x10000003; pyld[3].len = _VRF_LEN_ + 32; /* v=0 p=0x10000002/32 */
            pyld[4].key[0] = 0; pyld[4].key[1] = 0x10000004; pyld[4].len = _VRF_LEN_ + 32; /* v=0 p=0x10000002/32 */
            pyld[5].key[0] = 0; pyld[5].key[1] = 0x10000005; pyld[5].len = _VRF_LEN_ + 32; /* v=0 p=0x10000002/32 */
            numkey = 6;
            break;
        case 2: /* 2:5 split */
            pyld[0].key[0] = 0; pyld[0].key[1] = 0x100; pyld[0].len = _VRF_LEN_ + 12;
            pyld[1].key[0] = 0; pyld[1].key[1] = 0x1011; pyld[1].len = _VRF_LEN_ + 16;
            pyld[2].key[0] = 0; pyld[2].key[1] = 0x100000; pyld[2].len = _VRF_LEN_ + 24; 
            pyld[3].key[0] = 0; pyld[3].key[1] = 0x1000000; pyld[3].len = _VRF_LEN_ + 28;
            pyld[4].key[0] = 0; pyld[4].key[1] = 0x1001; pyld[4].len = _VRF_LEN_ + 16;
            pyld[5].key[0] = 0; pyld[5].key[1] = 0x10011; pyld[5].len = _VRF_LEN_ + 20;
            numkey = 6;
            break;

        case 3:
        {
            int dup;

            if (seed == 0) {
                seed = sal_time();
                sal_srand(seed);
            }

            index = 0;
            soc_cm_print("Random test: %d Seed: 0x%x \n", id, seed);
            do {
                do {
                    pyld[index].key[1] = (unsigned int) sal_rand();
                    pyld[index].len = (unsigned int)sal_rand() % 32;
                    pyld[index].len += _VRF_LEN_;

                    if (pyld[index].len <= 32) {
                        pyld[index].key[0] = 0;
                        pyld[index].key[1] &= MASK(pyld[index].len);
                    }

                    if (pyld[index].len > 32) {
                        pyld[index].key[0] = (unsigned int)sal_rand() % 16;                        
                        pyld[index].key[0] &= MASK(pyld[index].len-32); 
                    }

                    dup = ut_check_duplicate(pyld, index);
                    if (dup) {                    
                        soc_cm_print("\n Duplicate at index[%d]:key[0x%08x:0x%08x] Retry!!!\n", 
                                     index,pyld[index].key[0],pyld[index].key[1]);
                    }
                } while(dup > 0);
            } while(++index < _NUM_KEY_);

            numkey = index;
       }
       break;

        default:
            return SOC_E_PARAM;
        }

        trie_init(_MAX_KEY_LEN_, &trie);
        trie_init(_MAX_KEY_LEN_, &newtrie);

        for(index=0; index < numkey && rv == SOC_E_NONE; index++) {
            rv = trie_insert(trie, &pyld[index].key[0], NULL, pyld[index].len, &pyld[index].node);
        }

        rv = trie_split(trie, _MAX_KEY_LEN_, FALSE, pivot, &length, &newroot, NULL, FALSE);
        if (SOC_SUCCESS(rv)) {
            soc_cm_print("\n Split Trie Pivot: 0x%08x 0x%08x Length: %d Root: %p \n",
                         pivot[0], pivot[1], length, newroot);
            soc_cm_print(" $Payload Count Old Trie:%d New Trie:%d \n", trie->trie->count, newroot->count);

            /* set new trie */
            newtrie->trie = newroot;
            newtrie->v6_key = trie->v6_key;
#ifdef VERBOSE
            soc_cm_print("\n OLD Trie Dump ############: \n");
            trie_dump(trie, NULL, NULL);
            soc_cm_print("\n SPLIT Trie Dump ############: \n");
            trie_dump(newtrie, NULL, NULL);
#endif
            
            for(index=0; index < numkey && rv == SOC_E_NONE; index++) {
                rv = trie_search(trie, &pyld[index].key[0], pyld[index].len, &pyldptr);
                if (rv != SOC_E_NONE) {
                    rv = trie_search(newtrie, &pyld[index].key[0], pyld[index].len, &pyldptr);
                    if (rv != SOC_E_NONE) {
                        soc_cm_print("SEARCH: Key=0x%x 0x%x len %d SEARCH idx:%d failed on both trie!!!!\n", 
                                     pyld[index].key[0], pyld[index].key[1],
                                     pyld[index].len, index);
                    } else {
                        assert(pyldptr == &pyld[index].node);
                    }
                } 
            }
            
        }
    }

    trie_destroy(trie);
    trie_destroy(newtrie);
    sal_free(pyld);
    return rv;
}

int tmu_taps_trie_ut(int id, unsigned int seed)
{
    int index, rv = SOC_E_NONE, numkey=0, num_deleted=0;
    trie_t *trie;
    payload_t *pyld = sal_alloc(_NUM_KEY_ * sizeof(payload_t), "unit-test");
    trie_node_t *pyldptr = NULL;
    unsigned int result_len=0, result_key[_MAX_KEY_WORDS_];

    /* keys packed right to left (ie) most significant word starts at index 0*/

    switch(id) {
    case 0:
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x10; pyld[0].len = _VRF_LEN_ + 8;  /* v=0 p=0x10000000/8  */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x1000; pyld[1].len = _VRF_LEN_ + 16; /* v=0 p=0x10000000/16 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0x100000; pyld[2].len = _VRF_LEN_ + 24; /* v=0 p=0x10000000/24 */
        pyld[3].key[0] = 0; pyld[3].key[1] = 0x10000000; pyld[3].len = _VRF_LEN_ + 32; /* v=0 p=0x10000000/48 */
        numkey = 4;
        break;

    case 1:
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x123456; pyld[0].len  = _VRF_LEN_ + 24; /* v=0 p=0x12345678/24 */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x246; pyld[1].len = _VRF_LEN_ + 13; /* v=0 p=0x12345678/13 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0x24; pyld[2].len = _VRF_LEN_ + 9; /* v=0 p=0x12345678/9 */
        numkey = 3;
        break;

    case 2: /* dup routes on another vrf */
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x1123456; pyld[0].len = _VRF_LEN_ + 24; /* v=1 p=0x12345678/24 */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x2246; pyld[1].len = _VRF_LEN_ + 13; /* v=1 p=0x12345678/13 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0x224; pyld[2].len = _VRF_LEN_ + 9; /* v=1 p=0x12345678/9 */
        numkey = 3;
        break;

    case 3:
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x10000000; pyld[0].len = _VRF_LEN_ + 32; /* v=0 p=0x10000000/32 */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x10000001; pyld[1].len = _VRF_LEN_ + 32; /* v=0 p=0x10000001/32 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0x10000002; pyld[2].len = _VRF_LEN_ + 32; /* v=0 p=0x10000002/32 */
        numkey = 3;
        break;

    case 4:
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x12345670; pyld[0].len = _VRF_LEN_ + 32; /* v=0 p=0x12345670/32 */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x12345671; pyld[1].len = _VRF_LEN_ + 32; /* v=0 p=0x12345671/32 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0x91a2b38;  pyld[2].len = _VRF_LEN_ + 31; /* v=0 p=0x12345670/31 */
        numkey = 3;
        break;

    case 5:
        pyld[0].key[0] = 0; pyld[0].key[1] = 0x20; pyld[0].len = _VRF_LEN_ + 8; /* v=0 p=0x20000000/8 */
        pyld[1].key[0] = 0; pyld[1].key[1] = 0x8000; pyld[1].len = _VRF_LEN_ + 16; /* v=0 p=0x80000000/16 */
        pyld[2].key[0] = 0; pyld[2].key[1] = 0; pyld[2].len = _VRF_LEN_ + 0; /* v=0 p=0/0 */
        numkey = 3;
        break;

    case 6:
        {
            int dup;

            if (seed == 0) {
                seed = sal_time();
                sal_srand(seed);
            }
            index = 0;
            soc_cm_print("Random test: %d Seed: 0x%x \n", id, seed);
            do {
                do {
                    pyld[index].key[1] = (unsigned int) sal_rand();
                    pyld[index].len = (unsigned int)sal_rand() % 32;
                    pyld[index].len += _VRF_LEN_;

                    if (pyld[index].len <= 32) {
                        pyld[index].key[0] = 0;
                        pyld[index].key[1] &= MASK(pyld[index].len);
                    }

                    if (pyld[index].len > 32) {
                        pyld[index].key[0] = (unsigned int)sal_rand() % 16;                        
                        pyld[index].key[0] &= MASK(pyld[index].len-32); 
                    }

                    dup = ut_check_duplicate(pyld, index);
                    if (dup) {
                            soc_cm_print("\n Duplicate at index[%d]:key[0x%08x:0x%08x] Retry!!!\n", 
                                         index,pyld[index].key[0],pyld[index].key[1]);
                    }
                } while(dup > 0);
            } while(++index < _NUM_KEY_);

            numkey = index;
        }
        break;

    default:
        sal_free(pyld);      
        return -1;
    }

    trie_init(_MAX_KEY_LEN_, &trie);
    soc_cm_print("\n Num keys to test= %d \n", numkey);

    for(index=0; index < numkey && rv == SOC_E_NONE; index++) {
        unsigned int vrf=0, i;
        vrf = (pyld[index].len - _VRF_LEN_ == 32) ? 0:pyld[index].key[1] >> (pyld[index].len - _VRF_LEN_);
        vrf |= pyld[index].key[0] << (32 - (pyld[index].len - _VRF_LEN_));

#ifdef LOG
        soc_cm_print("+ Inserted Key=0x%x 0x%x vpn=0x%x pfx=0x%x Len=%d idx:%d\n", 
               pyld[index].key[0], pyld[index].key[1], vrf,
               pyld[index].key[1] & MASK(pyld[index].len - _VRF_LEN_), 
               pyld[index].len, index);
#endif
        rv = trie_insert(trie, &pyld[index].key[0], NULL, pyld[index].len, &pyld[index].node);
        if (rv != SOC_E_NONE) {
            soc_cm_print("FAILED to Insert Key=0x%x 0x%x vpn=0x%x pfx=0x%x Len=%d idx:%d\n", 
                   pyld[index].key[0], pyld[index].key[1], vrf,
                   pyld[index].key[1] & MASK(pyld[index].len - _VRF_LEN_), 
                   pyld[index].len, index);
        }
#define _VERBOSE_SEARCH_
        /* search all keys & figure out breakage right away */
        for (i=0; i <= index && rv == SOC_E_NONE; i++) {
#ifdef _VERBOSE_SEARCH_
            result_key[0] = 0;
            result_key[1] = 0;
            result_len    = 0;
            rv = trie_search_verbose(trie, &pyld[index].key[0], pyld[index].len, 
                                     &pyldptr, &result_key[0], &result_len);
#else
            rv = trie_search(trie, &pyld[index].key[0], pyld[index].len, &pyldptr);
#endif
            if (rv != SOC_E_NONE) {
                soc_cm_print("SEARCH: Key=0x%x 0x%x len %d SEARCH idx:%d failed!!!!\n", 
                       pyld[index].key[0], pyld[index].key[1],
                       pyld[index].len, index);
                break;
            } else {
                assert(pyldptr == &pyld[index].node);
#ifdef _VERBOSE_SEARCH_
                if (pyld[index].key[0] != result_key[0] ||
                    pyld[index].key[1] != result_key[1] ||
                    pyld[index].len != result_len) {
                    soc_cm_print(" Found key mismatches with the expected Key !!!! \n");
                    rv = SOC_E_FAIL;
                }
#ifdef VERBOSE
                soc_cm_print("Lkup[%d] key/len: 0x%x 0x%x/%d Found Key/len: 0x%x 0x%x/%d \n",
                             index,pyld[index].key[0], pyld[index].key[1], pyld[index].len,
                             result_key[0], result_key[1], result_len);
#endif
#endif
            }
        }
    }

#ifdef VERBOSE
    soc_cm_print("\n============== TRIE DUMP ================\n");
    trie_dump(trie, NULL, NULL);
    soc_cm_print("\n=========================================\n");
#endif

    /* randomly pickup prefix & delete */
    while(num_deleted < numkey && rv == SOC_E_NONE) {
        index = sal_rand() % numkey;
        if (pyld[index].len != 0xFFFFFFFF) {
            rv = trie_search(trie, &pyld[index].key[0], pyld[index].len, &pyldptr);
            if (rv == SOC_E_NONE) {
                assert(pyldptr == &pyld[index].node);
                rv = trie_delete(trie, &pyld[index].key[0], pyld[index].len, &pyldptr);

#ifdef VERBOSE
                soc_cm_print("\n============== TRIE DUMP ================\n");
                trie_dump(trie, NULL, NULL);
#endif
                if (rv == SOC_E_NONE) {
#ifdef LOG
                    soc_cm_print("Deleted Key=0x%x 0x%x Len=%d idx:%d Num-Key:%d\n", 
                           pyld[index].key[0], pyld[index].key[1], 
                           pyld[index].len, index, num_deleted);
#endif
                    pyld[index].len = 0xFFFFFFFF;
                    num_deleted++;

                    /* search all keys & figure out breakage right away */
                    for (index=0; index < numkey; index++) {
                        if (pyld[index].len == 0xFFFFFFFF) continue;

                        rv = trie_search(trie, &pyld[index].key[0], pyld[index].len, &pyldptr);
                        if (rv != SOC_E_NONE) {
                            soc_cm_print("ALL SEARCH after delete: Key=0x%x 0x%x len %d SEARCH idx:%d failed!!!!\n", 
                                   pyld[index].key[0], pyld[index].key[1],
                                   pyld[index].len, index);
                            break;
                        } else {
                            assert(pyldptr == &pyld[index].node);
                        }
                    }
                } else {
                    soc_cm_print("Deleted Key=0x%x 0x%x Len=%d idx:%d FAILED!!!\n", 
                           pyld[index].key[0], pyld[index].key[1], 
                           pyld[index].len, index);
                    break;
                }
            } else {
                soc_cm_print("SEARCH: Key=0x%x 0x%x len %d SEARCH idx:%d failed!!!!\n", 
                       pyld[index].key[0], pyld[index].key[1],
                       pyld[index].len, index);
                break;
            }
        }
    }

    if (rv == SOC_E_NONE) soc_cm_print("\n TEST ID %d passed \n", id);
    else {  
        soc_cm_print("\n TEST ID %d Failed Num Delete:%d !!!!!!!!\n", id, num_deleted);
    }

    sal_free(pyld);
    trie_destroy(trie);
    return rv;
}

/**********************************************/
/* BPM unit tests */
/* test cases:
 * 1 - insert pivot's with bpm bit masks
 * 2 - propagate updated prefix bpm (add/del)
 * 3 - fuse node bpm verification
 * 4 - split bpm - nop
 * 5 - */

typedef struct _expect_datum_s {
    dq_t list;
    payload_t *pfx; 
    trie_t *pfx_trie;
} expect_datum_t;

int ut_bpm_build_expect_list(trie_node_t *payload, void *user_data)
{
    int rv=SOC_E_NONE;

    if (payload && payload->type == PAYLOAD) {
        trie_node_t *pyldptr;
        payload_t *pivot;
        expect_datum_t *datum = (expect_datum_t*)user_data;

        pivot = TRIE_ELEMENT_GET(payload_t*, payload, node);
        /* if the inserted prefix is a best prefix, add the pivot to expected list */
        rv = trie_find_lpm(datum->pfx_trie, &pivot->key[0], pivot->len, &pyldptr); 
        assert(rv == SOC_E_NONE);
        if (pyldptr == &datum->pfx->info.pfx_trie_node) {
            /* if pivot is not equal to prefix add to expect list */
            if (!(pivot->key[0] == datum->pfx->key[0] && 
                  pivot->key[1] == datum->pfx->key[1] &&
                  pivot->len    == datum->pfx->len)) {
                DQ_INSERT_HEAD(&datum->list, &pivot->listnode);
            }
        }
    }

    return SOC_E_NONE;
}

int ut_bpm_propagate_cb(trie_node_t *payload, trie_bpm_cb_info_t *cbinfo)
{
    if (payload && cbinfo && payload->type == PAYLOAD) {
        payload_t *pivot;
        dq_p_t elem;
        expect_datum_t *datum = (expect_datum_t*)cbinfo->user_data;

        pivot = TRIE_ELEMENT_GET(payload_t*, payload, node);
        DQ_TRAVERSE(&datum->list, elem) {
            payload_t *velem = DQ_ELEMENT_GET(payload_t*, elem, listnode); 
            if (velem == pivot) {
                DQ_REMOVE(&pivot->listnode);
                break;
            }
        } DQ_TRAVERSE_END(&datum->list, elem);
    }

    return SOC_E_NONE;
}

int ut_bpm_propagate_empty_cb(trie_node_t *payload, trie_bpm_cb_info_t *cbinfo)
{
    /* do nothing */
    return SOC_E_NONE;
}

void ut_bpm_dump_expect_list(expect_datum_t *datum, char *str)
{
    dq_p_t elem;
    if (datum) {
        /* dump expected list */
        soc_cm_print("%s \n", str);
        DQ_TRAVERSE(&datum->list, elem) {
            payload_t *velem = DQ_ELEMENT_GET(payload_t*, elem, listnode); 
            soc_cm_print(" Pivot: 0x%x 0x%x Len: %d \n", 
                         velem->key[0], velem->key[1], velem->len);
        } DQ_TRAVERSE_END(&datum->list, elem);
    }
}

#define _MAX_TEST_PIVOTS_ (10)
#define _MAX_BKT_PFX_ (20)
#define _MAX_NUM_PICK (30)

int tmu_taps_bpm_trie_ut(int id, unsigned int seed)
{
    int rv = SOC_E_NONE, pivot=0, pfx=0, index=0, dup=0, domain=0;
    trie_t *pfx_trie, *trie;
    payload_t *pyld = sal_alloc(_MAX_BKT_PFX_ * _MAX_TEST_PIVOTS_ * sizeof(payload_t), "bpm-unit-test");
    payload_t *pivot_pyld = sal_alloc(_MAX_TEST_PIVOTS_ * sizeof(payload_t), "bpm-unit-test");
    trie_node_t *pyldptr = NULL, *newroot;
    unsigned int bpm[BITS2WORDS(_MAX_KEY_LEN_)];
    expect_datum_t datum;
    trie_bpm_cb_info_t cbinfo;
    int num_pick, bpm_pfx_len;

    sal_memset(pyld, 0, _MAX_BKT_PFX_ * _MAX_TEST_PIVOTS_ * sizeof(payload_t));
    sal_memset(pivot_pyld, 0, _MAX_TEST_PIVOTS_ * sizeof(payload_t));

    if (seed == 0) {
        seed = sal_time();
        sal_srand(seed);
    }    

    trie_init(_MAX_KEY_LEN_, &trie);
    trie_init(_MAX_KEY_LEN_, &pfx_trie);

    /* populate a random pivot / prefix trie */
    soc_cm_print("Random test: %d Seed: 0x%x \n", id, seed);

    /* insert a vrf=0,* pivot */
    pivot = 0;
    pfx = 0;
    pivot_pyld[pivot].key[1] = 0;
    pivot_pyld[pivot].key[0] = 0;
    pivot_pyld[pivot].len    = 0;
    trie_init(_MAX_KEY_LEN_, &pivot_pyld[pivot].info.trie);
    sal_memset(&bpm[0], 0,  BITS2WORDS(_MAX_KEY_LEN_) * sizeof(unsigned int));
            
    do {
        rv = trie_insert(trie, &pivot_pyld[pivot].key[0], &bpm[0], 
                         pivot_pyld[pivot].len, &pivot_pyld[pivot].node);
        if (rv != SOC_E_NONE) {
            soc_cm_print("FAILED to Insert PIVOT Key=0x%x 0x%x Len=%d idx:%d\n", 
                   pivot_pyld[pivot].key[0], pivot_pyld[pivot].key[1], 
                   pivot_pyld[pivot].len, pivot);
        } else {
            if (pivot > 0) {
                /* choose a random pivot bucket to fill & split */
                domain = ((unsigned int) sal_rand()) % pivot;
            } else {
                domain = 0;
            }
            
            index = 0;
            sal_memset(&bpm[0], 0,  BITS2WORDS(_MAX_KEY_LEN_) * sizeof(unsigned int));
    
            do {
                do {
                    /* add prefix such that lpm of the prefix is the pivot to ensure
                     * it goes into specific pivot domain */
                    pyld[pfx+index].key[1] = (unsigned int) sal_rand();
                    pyld[pfx+index].len = (unsigned int)sal_rand() % 32;
                    pyld[pfx+index].len += _VRF_LEN_;

                    if (pyld[pfx+index].len <= 32) {
                        pyld[pfx+index].key[0] = 0;
                        pyld[pfx+index].key[1] &= MASK(pyld[pfx+index].len);
                    }

                    if (pyld[pfx+index].len > 32) {
                        pyld[pfx+index].key[0] = (unsigned int)sal_rand() % 16;                        
                        pyld[pfx+index].key[0] &= MASK(pyld[pfx+index].len-32); 
                    }

                    dup = ut_check_duplicate(pyld, pfx+index);
                    if (!dup) {
                        rv = trie_find_lpm(trie, &pyld[pfx+index].key[0], pyld[pfx+index].len, &pyldptr); 
                        if (SOC_FAILURE(rv)) {
                            soc_cm_print("\n !! Failed to find LPM pivot for index[%d]:key[0x%08x:0x%08x] !!!!\n",
                                         pfx,pyld[pfx+index].key[0],pyld[pfx+index].key[1]);
                        } 
                    }
                } while ((dup || (pyldptr != &pivot_pyld[domain].node)) && SOC_SUCCESS(rv));

                if (SOC_SUCCESS(rv)) {
                    rv =  trie_insert(pivot_pyld[domain].info.trie,
                                      &pyld[pfx+index].key[0], NULL, 
                                      pyld[pfx+index].len, &pyld[pfx+index].node);
                    if (SOC_FAILURE(rv)) {
                        soc_cm_print("\n !! Failed insert prefix into pivot trie"
                                     " index[%d]:key[0x%08x:0x%08x] !!!!\n",
                                     pfx+index,pyld[pfx+index].key[0],pyld[pfx+index].key[1]);
                    } else {
                        rv =  trie_insert(pfx_trie,
                                          &pyld[pfx+index].key[0], NULL, 
                                          pyld[pfx+index].len, &pyld[pfx+index].info.pfx_trie_node);     
                        if (SOC_FAILURE(rv)) {
                            soc_cm_print("\n !! Failed insert prefix into prefix trie"
                                         " index[%d]:key[0x%08x:0x%08x] !!!!\n",
                                         pfx+index,pyld[pfx+index].key[0],pyld[pfx+index].key[1]);
                        } else {
                            index++;
                        }                      
                    }
                }

            } while(index < (_MAX_BKT_PFX_/2 - 1) && SOC_SUCCESS(rv));

            /* try to populate prefix where p == v */
            if (pivot > 0) {
                /* 25% probability */
                if (((unsigned int) sal_rand() % 4) == 0) {
                    
                }
            }

#ifdef VERBOSE
            soc_cm_print("### Split Domain ID: %d \n", domain);
            for (i=0; i <= pivot; i++) {
                soc_cm_print("\n --- TRIE domain dump: Pivot: 0x%x 0x%x len=%d ----- \n",
                             pivot_pyld[i].key[0], pivot_pyld[i].key[1], pivot_pyld[i].len);
                trie_dump(pivot_pyld[i].info.trie, ut_print_payload_node, NULL);
            }
#endif

            if (SOC_SUCCESS(rv) && ++pivot < _MAX_TEST_PIVOTS_) {
                pfx += index;
                trie_init(_MAX_KEY_LEN_, &pivot_pyld[pivot].info.trie);
                /* split the domain & insert a new pivot */
                rv = trie_split(pivot_pyld[domain].info.trie,
				_MAX_KEY_LEN_, FALSE,
                                &pivot_pyld[pivot].key[0], 
                                &pivot_pyld[pivot].len, &newroot, 
                                &bpm[0], FALSE);
                if (SOC_SUCCESS(rv)) {
                    pivot_pyld[pivot].info.trie->trie = newroot;
                    pivot_pyld[pivot].info.trie->v6_key = pivot_pyld[domain].info.trie->v6_key;
                    soc_cm_print("BPM for split pivot: 0x%x 0x%x / %d = [0x%x 0x%x] \n",
                                 pivot_pyld[pivot].key[0], pivot_pyld[pivot].key[1],
                                 pivot_pyld[pivot].len, bpm[0], bpm[1]);
                } else {
                    soc_cm_print("\n !!! Failed to split domain trie for domain: %d !!!\n", domain);
                }
            }
        }
    } while(pivot < _MAX_TEST_PIVOTS_ && SOC_SUCCESS(rv));

    /* pick up the root node on pivot trie & add a prefix shorter than the nearest child.
     * This is ripple & create huge propagation */
    /* insert *\/1 into the * bucket so huge propagation kicks in */
    pyld[pfx].key[1] = (unsigned int) sal_rand() % 1;
    pyld[pfx].key[0] = 0;
    pyld[pfx].len    = 1;
    do {
        dup = ut_check_duplicate(pyld, pfx);
        if (!dup) {
            rv = trie_find_lpm(trie, &pyld[pfx].key[0], pyld[pfx].len, &pyldptr); 
            if (SOC_FAILURE(rv)) {
                soc_cm_print("\n !! Failed to find LPM pivot for index[%d]:key[0x%08x:0x%08x] !!!!\n",
                             pfx,pyld[pfx].key[0],pyld[pfx].key[1]);
            } 
        } else {
            pyld[pfx].len++;    
        }
    } while(dup && SOC_SUCCESS(rv));

    if (SOC_SUCCESS(rv)) {
        rv =  trie_insert(pfx_trie,
                          &pyld[pfx].key[0], NULL, 
                          pyld[pfx].len, &pyld[pfx].info.pfx_trie_node);
        if (SOC_FAILURE(rv)) {
            soc_cm_print("\n !! Failed insert prefix into pivot trie"
                         " index[%d]:key[0x%08x:0x%08x] !!!!\n",
                         pfx,pyld[pfx].key[0],pyld[pfx].key[1]);
        } else {
            DQ_INIT(&datum.list);
            datum.pfx = &pyld[pfx];
            datum.pfx_trie = pfx_trie;
            /* create expected list of pivot to be propagated */
            trie_traverse(trie, ut_bpm_build_expect_list, &datum, _TRIE_PREORDER_TRAVERSE);

            /* dump expected list */
            ut_bpm_dump_expect_list(&datum, "-- Expected Propagation List --");
        }
    }

    sal_memset(&cbinfo, 0, sizeof(trie_bpm_cb_info_t));
    cbinfo.user_data = &datum;
    cbinfo.pfx = &pyld[pfx].key[0];
    cbinfo.len = pyld[pfx].len;
    if (pyldptr == NULL) {
        assert(0); /* check here for coverity */
    }
    rv = trie_pivot_propagate_prefix(pyldptr,
                               (TRIE_ELEMENT_GET(payload_t*, pyldptr, node))->len,
                               &pyld[pfx].key[0], pyld[pfx].len,
                               1, ut_bpm_propagate_cb, &cbinfo);
    if (DQ_EMPTY(&datum.list)) {
        soc_cm_print("++ Propagation Test Passed \n");
    } else {
        soc_cm_print("!!!!! Propagation Test FAILED !!!!!\n");
        rv = SOC_E_FAIL;
        ut_bpm_dump_expect_list(&datum, "!! Zombies on Propagation List !!");
        assert(0);
    }

    /* propagate a shorter prefix of an existing pivot 
     * we should find the bpm
     */
    pfx++;
    num_pick = 0;
    do {
	/* randomly pick a pivot */
	index = ((unsigned int) sal_rand()) % pivot;
	
	/* create a prefix shorter */
	pyld[pfx].len    = ((unsigned int) sal_rand()) % pivot_pyld[index].len;
	pyld[pfx].key[1] = pivot_pyld[index].key[1]>>(pivot_pyld[index].len - pyld[pfx].len);
	pyld[pfx].key[0] = 0;

	if (pyld[pfx].len >= 1) {
	    /* propagate add len=0 */
	    rv = trie_pivot_propagate_prefix(trie->trie,
				       (TRIE_ELEMENT_GET(payload_t*, trie->trie, node))->len,
				       &pyld[pfx].key[0], 0,
				       1, ut_bpm_propagate_empty_cb, 
				       &cbinfo);

	    if (SOC_FAILURE(rv)) {
		soc_cm_print("!!!!! BPM search Test FAILED to propagate add len=0!!!!!\n");
		assert(0);
	    }

	    /* propagate add */
	    rv = trie_pivot_propagate_prefix(trie->trie,
				       (TRIE_ELEMENT_GET(payload_t*, trie->trie, node))->len,
				       &pyld[pfx].key[0], pyld[pfx].len,
				       1, ut_bpm_propagate_empty_cb, 
				       &cbinfo);
	    if (SOC_FAILURE(rv)) {
		soc_cm_print("!!!!! BPM search Test FAILED to propagate add \n"
			     " index[%d]:key[0x%08x:0x%08x] len=%d!!!!\n",
			     pfx, pyld[pfx].key[0], pyld[pfx].key[1], pyld[pfx].len);
		assert(0);
	    }

	    /* perform bpm lookup on the pivot, we should find the pyld[pfx].len */
	    rv = trie_find_prefix_bpm(trie, (unsigned int *)&(pivot_pyld[index].key[0]),
				      pivot_pyld[index].len, (unsigned int *)&bpm_pfx_len);
	    if (SOC_FAILURE(rv) || (bpm_pfx_len != pyld[pfx].len)) {
		soc_cm_print("!!!!! BPM search Test FAILDED after propagate add !!!!!\n");
		assert(0);		
	    }

	    /* propagate delete */
	    rv = trie_pivot_propagate_prefix(trie->trie,
				       (TRIE_ELEMENT_GET(payload_t*, trie->trie, node))->len,
				       &pyld[pfx].key[0], pyld[pfx].len,
				       0, ut_bpm_propagate_empty_cb, 
				       &cbinfo);
	    
	    if (SOC_FAILURE(rv)) {
		soc_cm_print("!!!!! BPM search Test FAILED to propagate add \n"
			     " index[%d]:key[0x%08x:0x%08x] len=%d!!!!\n",
			     pfx, pyld[pfx].key[0], pyld[pfx].key[1], pyld[pfx].len);
		assert(0);
	    }

	    /* perform bpm lookup on the pivot, we should find the len==0 */
	    rv = trie_find_prefix_bpm(trie, (unsigned int *)&(pivot_pyld[index].key[0]),
				      pivot_pyld[index].len, (unsigned int *)&bpm_pfx_len);
	    if (SOC_FAILURE(rv) || (bpm_pfx_len != 0)) {
		soc_cm_print("!!!!! BPM search Test FAILDED after propagate delete !!!!!\n");
		assert(0);		
	    }

	    num_pick = _MAX_NUM_PICK+1;
	}
	num_pick++;
    } while(num_pick<_MAX_NUM_PICK);

    if (num_pick <= _MAX_NUM_PICK) {
	soc_cm_print("!!!!! BPM search Test 2 Skipped after tried %d times!!!!!\n", _MAX_NUM_PICK);	
    } else {
	soc_cm_print("!!!!! BPM search Test 2 Passed!!!!!\n");	
    }

#ifdef VERBOSE
    soc_cm_print("\n ----- Prefix Trie dump ----- \n");
    trie_dump(pfx_trie, ut_print_prefix_payload_node, NULL);
#endif

    soc_cm_print("\n ++++++++ Trie dump ++++++++ \n");
    trie_dump(trie, ut_print_payload_node, NULL);

    /* clean up */
    for (index=0; index < pivot; index++) {
#ifdef VERBOSE
        soc_cm_print("\n ddddddd dump dddddddd \n");
        trie_dump(pivot_pyld[index].info.trie, ut_print_payload_node, NULL);
#endif
        trie_destroy(pivot_pyld[index].info.trie);
    }

    sal_free(pyld);
    sal_free(pivot_pyld);
    trie_destroy(trie);
    trie_destroy(pfx_trie);
    return rv;
}

/**********************************************/
#endif 
#endif /* ALPM_IPV6_128_SUPPORT */

#endif /* BCM_TRIDENT2_SUPPORT */
#endif /* ALPM_ENABLE */

