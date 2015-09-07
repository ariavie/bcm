/*
 * $Id: trie.h,v 1.3 Broadcom SDK $
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
 * trie data structure
 *
 *-----------------------------------------------------------------------------*/
#ifndef _ESW_TRIDENT2_TRIE_H_
#define _ESW_TRIDENT2_TRIE_H_

#ifdef ALPM_ENABLE
#ifdef ALPM_IPV6_128_SUPPORT
typedef struct trie_node_s trie_node_t;

typedef enum _node_type_e {
    INTERNAL, PAYLOAD, MAX
} node_type_e_t;

typedef struct child_node_s {
    trie_node_t *child_node;
} child_node_t;

struct trie_node_s {
    trie_node_t *trie_node;
#define _MAX_CHILD_     (2)
    child_node_t child[_MAX_CHILD_];
    unsigned int skip_len;
    unsigned int skip_addr;
    node_type_e_t type;
    unsigned int count; /* number of payload node counts */
    unsigned int bpm; /* best prefix match bit map - 32 bits */
};

typedef struct trie_s {
    trie_node_t *trie; /* trie pointer */
} trie_t;

typedef int (*trie_callback_f)(trie_node_t *trie, void *datum);

typedef struct trie_bpm_cb_info_s {
    unsigned int *pfx; /* prefix buffer pointer from caller space */
    unsigned int  len;  /* prefix length */
    void         *user_data;
    /* when a prefix is deleted & propagated, the next BPM has to be updated.
     * Since the library walks down, the next bpm length is determined and 
     * returned back on call back */
    unsigned int  del_bpm_len; 
} trie_bpm_cb_info_t;

typedef int (*trie_propagate_cb_f)(trie_node_t *trie, 
                                   trie_bpm_cb_info_t *info);

/*
 * This macro is a tidy way of performing subtraction to move from a
 * pointer within an object to a pointer to the object.
 *
 * Arguments are:
 *    type of object to recover 
 *    pointer to object from which to recover element pointer 
 *    pointer to an object of type t
 *    name of the trie node field in t through which the object is linked on trie
 * Returns:
 *    a pointer to the object, of type t
 */
#define TRIE_ELEMENT(t, p, ep, f) \
  ((t) (((char *) (p)) - (((char *) &((ep)->f)) - ((char *) (ep)))))

/*
 * TRIE_ELEMENT_GET performs the same function as TRIE_ELEMENT, but does not
 * require a pointer of type (t).  This form is preferred as TRIE_ELEMENT
 * typically generate Coverity errors, and the (ep) argument is unnecessary.
 *
 * Arguments are:
 *    type of object to recover 
 *    pointer to object from which to recover element pointer
 *    name of the trie node field in t through which the object is linked on trie
 * Returns:
 *    a pointer to the object, of type t
 */
#define TRIE_ELEMENT_GET(t, p, f) \
  ((t) (((char *) (p)) - (((char *) &(((t)(0))->f)))))


/*
 * Function:
 *     trie_init
 * Purpose:
 *     allocates a trie & initializes it
 */
extern int trie_init(trie_t **ptrie);

/*
 * Function:
 *     trie_destroy
 * Purpose:
 *     destroys a trie 
 */
extern int trie_destroy(trie_t *trie);

/*
 * Function:
 *     trie_insert
 * Purpose:
 *     Inserts provided prefix/length in to the trie
 */
extern int trie_insert(trie_t *trie,
                       unsigned int *key, 
                       /* bpm bit map if bpm management is required, 
                          passing null skips bpm management */
                       unsigned int *bpm, 
                       unsigned int length,
                       trie_node_t *payload);

/*
 * Function:
 *     trie_delete
 * Purpose:
 *     Deletes provided prefix/length in to the trie
 */
extern int trie_delete(trie_t *trie,
                       unsigned int *key,
                       unsigned int length,
                       trie_node_t **payload);

/*
 * Function:
 *     trie_search
 * Purpose:
 *     Search the given trie for provided prefix/length
 */
extern int trie_search(trie_t *trie, 
                       unsigned int *key, 
                       unsigned int length,
                       trie_node_t **payload);

/*
 * Function:
 *     trie_dump
 * Purpose:
 *     Dumps the trie pre-order [root|left|child]
 */
extern int trie_dump(trie_t *trie, trie_callback_f cb, void *user_data);

/*
 * Function:
 *     trie_find_lpm
 * Purpose:
 *     Find the longest prefix matched with iven prefix 
 */
extern int trie_find_lpm(trie_t *trie, 
                         unsigned int *key, 
                         unsigned int length,
                         trie_node_t **payload);

/*
 * Function:
 *     trie_find_prefix_bpm
 * Purpose:
 *    Given a key/length return the Best prefix match length
 *    key/bpm_pfx_len will be the BPM for the key/length
 */
extern int trie_find_prefix_bpm(trie_t *trie, 
                                unsigned int *key, 
                                unsigned int length,
                                unsigned int *bpm_pfx_len);

/*
 * Function:
 *     trie_split
 * Purpose:
 *     Split the trie into 2 based on optimum pivot
 */
extern int trie_split(trie_t *trie,
                      unsigned int *pivot,
                      unsigned int *length,
                      trie_node_t **split_trie_root,
                      unsigned int *bpm);

/*
 * Function:
 *     trie_unsplit
 * Purpose:
 *     unsplit or fuse the child trie with parent trie
 */
extern int trie_unsplit(trie_t *parent_trie,
                        trie_t *child_trie,
                        unsigned int *child_pivot,
                        unsigned int length);

typedef enum _trie_traverse_order_e_s {
    _TRIE_PREORDER_TRAVERSE,  /* root, left, right */
    _TRIE_INORDER_TRAVERSE,   /* left, root, right */
    _TRIE_POSTORDER_TRAVERSE, /* left, right, root */
    _TRIE_TRAVERSE_MAX
} trie_traverse_order_e_t;


/*
 * Function:
 *     trie_traverse
 * Purpose:
 *     Traverse the trie & call the application callback with user data 
 */
extern int trie_traverse(trie_t *trie, 
                         trie_callback_f cb,
                         void *user_data,
                         trie_traverse_order_e_t order);

/*
 * Function:
 *     trie_pivot_propagate_prefix
 * Purpose:
 *   Propogate prefix BPM from a given pivot.      
 */
extern int trie_pivot_propagate_prefix(trie_node_t *pivot,
                                       unsigned int pivot_len,
                                       unsigned int *pfx,
                                       unsigned int len,
                                       unsigned int add, /* 0-del/1-add */
                                       trie_propagate_cb_f cb,
                                       trie_bpm_cb_info_t *cb_info);

/*
 * Function:
 *     trie_propagate_prefix
 * Purpose:
 *  Propogate prefix BPM on a given trie.      
 */
extern int trie_propagate_prefix(trie_t *trie,
                                 unsigned int *pfx,
                                 unsigned int len,
                                 unsigned int add, /* 0-del/1-add */
                                 trie_propagate_cb_f cb,
                                 trie_bpm_cb_info_t *cb_info);

/*
 * Function:
 *     trie_util_get_bpm_pfx
 * Purpose:
 *     finds best prefix match given a bpm bitmap & key
 */
extern int trie_util_get_bpm_pfx(unsigned int *bpm, 
                                 unsigned int key_len,
                                 /* OUT */
                                 unsigned int *pfx_len);

#else /* ALPM_IPV6_128_SUPPORT */

typedef struct trie_node_s trie_node_t;

typedef enum _node_type_e {
    INTERNAL, PAYLOAD, MAX
} node_type_e_t;

typedef struct child_node_s {
    trie_node_t *child_node;
} child_node_t;

struct trie_node_s {
    trie_node_t *trie_node;
#define _MAX_CHILD_     (2)
    child_node_t child[_MAX_CHILD_];
    unsigned int skip_len;
    unsigned int skip_addr;
    node_type_e_t type;
    unsigned int count; /* number of payload node counts */
    unsigned int bpm; /* best prefix match bit map - 32 bits */
};

typedef struct trie_s {
    trie_node_t *trie;         /* trie root pointer */
    unsigned int v6_key;       /* support 144 bits key, otherwise expect 48 bits key */
} trie_t;

typedef int (*trie_callback_f)(trie_node_t *trie, void *datum);

typedef struct trie_bpm_cb_info_s {
    unsigned int *pfx; /* prefix buffer pointer from caller space */
    unsigned int  len;  /* prefix length */
    void         *user_data;
} trie_bpm_cb_info_t;

typedef int (*trie_propagate_cb_f)(trie_node_t *trie, 
                                   trie_bpm_cb_info_t *info);

/*
 * This macro is a tidy way of performing subtraction to move from a
 * pointer within an object to a pointer to the object.
 *
 * Arguments are:
 *    type of object to recover 
 *    pointer to object from which to recover element pointer 
 *    pointer to an object of type t
 *    name of the trie node field in t through which the object is linked on trie
 * Returns:
 *    a pointer to the object, of type t
 */
#define TRIE_ELEMENT(t, p, ep, f) \
  ((t) (((char *) (p)) - (((char *) &((ep)->f)) - ((char *) (ep)))))

/*
 * TRIE_ELEMENT_GET performs the same function as TRIE_ELEMENT, but does not
 * require a pointer of type (t).  This form is preferred as TRIE_ELEMENT
 * typically generate Coverity errors, and the (ep) argument is unnecessary.
 *
 * Arguments are:
 *    type of object to recover 
 *    pointer to object from which to recover element pointer
 *    name of the trie node field in t through which the object is linked on trie
 * Returns:
 *    a pointer to the object, of type t
 */
#define TRIE_ELEMENT_GET(t, p, f) \
  ((t) (((char *) (p)) - (((char *) &(((t)(0))->f)))))


#define _CLONE_TRIE_NODE_(dest,src) \
    sal_memcpy((dest),(src),sizeof(trie_node_t))

/*
 * Function:
 *     trie_init
 * Purpose:
 *     allocates a trie & initializes it
 */
extern int trie_init(unsigned int max_key_len, trie_t **ptrie);

/*
 * Function:
 *     trie_destroy
 * Purpose:
 *     destroys a trie 
 */
extern int trie_destroy(trie_t *trie);

/*
 * Function:
 *     trie_insert
 * Purpose:
 *     Inserts provided prefix/length in to the trie
 */
extern int trie_insert(trie_t *trie,
                       unsigned int *key, 
                       /* bpm bit map if bpm management is required, 
                          passing null skips bpm management */
                       unsigned int *bpm, 
                       unsigned int length,
                       trie_node_t *payload);

/*
 * Function:
 *     trie_delete
 * Purpose:
 *     Deletes provided prefix/length in to the trie
 */
extern int trie_delete(trie_t *trie,
                       unsigned int *key,
                       unsigned int length,
                       trie_node_t **payload);

/*
 * Function:
 *     trie_search
 * Purpose:
 *     Search the given trie for provided prefix/length
 */
extern int trie_search(trie_t *trie, 
                       unsigned int *key, 
                       unsigned int length,
                       trie_node_t **payload);

/*
 * Function:
 *     trie_search_verbose
 * Purpose:
 *     Search the given trie for provided prefix/length,
 *     return the matched prefix/length
 */
extern int trie_search_verbose(trie_t *trie, 
			       unsigned int *key, 
			       unsigned int length,
			       trie_node_t **payload,
			       unsigned int *result_key, 
			       unsigned int *result_len);
/*
 * Function:
 *     trie_dump
 * Purpose:
 *     Dumps the trie pre-order [root|left|child]
 */
extern int trie_dump(trie_t *trie, trie_callback_f cb, void *user_data);

/*
 * Function:
 *     trie_find_lpm
 * Purpose:
 *     Find the longest prefix matched with iven prefix 
 */
extern int trie_find_lpm(trie_t *trie, 
                         unsigned int *key, 
                         unsigned int length,
                         trie_node_t **payload);

/*
 * Function:
 *     trie_find_pm
 * Purpose:
 *     Find the prefix matched nodes with given prefix and callback
 *     with specified callback funtion and user data
 */
extern int trie_find_pm(trie_t *trie, 
			unsigned int *key, 
			unsigned int length,
			trie_callback_f cb,
			void *user_data);

/*
 * Function:
 *     trie_find_prefix_bpm
 * Purpose:
 *    Given a key/length return the Best prefix match length
 *    key/bpm_pfx_len will be the BPM for the key/length
 */
extern int trie_find_prefix_bpm(trie_t *trie, 
                                unsigned int *key, 
                                unsigned int length,
                                unsigned int *bpm_pfx_len);

/*
 * Function:
 *     trie_split
 * Purpose:
 *     Split the trie into 2 based on optimum pivot
 */
extern int trie_split(trie_t *trie,
		      const unsigned int max_split_len,		      
		      const int split_to_pair,
                      unsigned int *pivot,
                      unsigned int *length,
                      trie_node_t **split_trie_root,
                      unsigned int *bpm,
                      /* if set split will strictly split only on payload nodes
                       * if not set splits at optimal point on the trie */
                      uint8 payload_node_split);

/*
 * Function:
 *     trie_unsplit
 * Purpose:
 *     unsplit or fuse the child trie with parent trie
 */
extern int trie_unsplit(trie_t *parent_trie,
                        trie_t *child_trie,
                        unsigned int *child_pivot,
                        unsigned int length);

typedef enum _trie_traverse_order_e_s {
    _TRIE_PREORDER_TRAVERSE,  /* root, left, right */
    _TRIE_INORDER_TRAVERSE,   /* left, root, right */
    _TRIE_POSTORDER_TRAVERSE, /* left, right, root */
    _TRIE_TRAVERSE_MAX
} trie_traverse_order_e_t;

/*
 * Function:
 *     trie_traverse
 * Purpose:
 *     Traverse the trie & call the application callback with user data 
 */
extern int trie_traverse(trie_t *trie, 
                         trie_callback_f cb,
                         void *user_data,
                         trie_traverse_order_e_t order);

/*
 * Function:
 *     trie_pivot_propagate_prefix
 * Purpose:
 *   Propogate prefix BPM from a given pivot.      
 */
extern int trie_pivot_propagate_prefix(trie_node_t *pivot,
                                       unsigned int pivot_len,
                                       unsigned int *pfx,
                                       unsigned int len,
                                       unsigned int add, /* 0-del/1-add */
                                       trie_propagate_cb_f cb,
                                       trie_bpm_cb_info_t *cb_info);

/*
 * Function:
 *     trie_propagate_prefix
 * Purpose:
 *  Propogate prefix BPM on a given trie.      
 */
extern int trie_propagate_prefix(trie_t *trie,
                                 unsigned int *pfx,
                                 unsigned int len,
                                 unsigned int add, /* 0-del/1-add */
                                 trie_propagate_cb_f cb,
                                 trie_bpm_cb_info_t *cb_info);

/*
 * Function:
 *     trie_util_get_bpm_pfx
 * Purpose:
 *     finds best prefix match given a bpm bitmap & key
 */
extern int trie_util_get_bpm_pfx(unsigned int *bpm, 
                                 unsigned int key_len,
                                 /* OUT */
                                 unsigned int *pfx_len);

/*
 * Function:
 *     trie_iter_get_first
 * Purpose:
 *     Traverse the trie & return pointer to first payload node
 */
extern int trie_iter_get_first(trie_t *trie,
                               trie_node_t **payload);

/*=================================
 * Used by internal functions only
 *================================*/
#define _MAX_KEY_LEN_48_  (48)
#define _MAX_KEY_LEN_144_ (144)

typedef enum _trie_split_states_e_s {
    TRIE_SPLIT_STATE_NONE,
    TRIE_SPLIT_STATE_PAYLOAD_SPLIT,
    TRIE_SPLIT_STATE_PAYLOAD_SPLIT_DONE,
    TRIE_SPLIT_STATE_PRUNE_NODES,
    TRIE_SPLIT_STATE_DONE,
    TRIE_SPLIT_STATE_MAX
} trie_split_states_e_t;

#define _MAX_SKIP_LEN_  (31)

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

/*
 * bit 0                                  - 0
 * bit [1, _MAX_SKIP_LEN]                 - 1
 * bit [_MAX_SKIP_LEN+1, 2*_MAX_SKIP_LEN] - 2...
 */
#define BITS2SKIPOFF(x) (((x) + _MAX_SKIP_LEN_-1) / _MAX_SKIP_LEN_)

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

extern int _trie_fuse_child(trie_node_t *trie, int bit);

extern int _print_trie_node(trie_node_t *trie, void *datum);

extern int _trie_init_propagate_info(unsigned int *pfx,
				     unsigned int len,
				     trie_propagate_cb_f cb,
				     trie_bpm_cb_info_t *cb_info);

extern int _trie_traverse_propagate_prefix(trie_node_t *trie,
					   trie_propagate_cb_f cb,
					   trie_bpm_cb_info_t *cb_info,
					   unsigned int mask);

#endif /* ALPM_IPV6_128_SUPPORT */
#endif /* ALPM_ENABLE */

#endif /* _ESW_TRIDENT2_TRIE_H_ */
