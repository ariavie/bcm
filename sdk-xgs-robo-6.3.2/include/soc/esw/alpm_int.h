/*
 * $Id: 
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
 * common defines between v4 and v6-128 code 
 */

#ifndef _ESW_TRIDENT2_ALPM_INT_H
#define _ESW_TRIDENT2_ALPM_INT_H

#include <soc/esw/trie.h>

#define SOC_ALPM_MODE_PARALLEL  1

#define _MAX_KEY_LEN_   144
#define VRF_ID_LEN      (10)   

#define MAX_VRF_ID   (1 << VRF_ID_LEN)
#define _MAX_KEY_WORDS_ (BITS2WORDS(_MAX_KEY_LEN_))

#define SOC_ALPM_GET_GLOBAL_BANK_DISABLE(u, bank_disable)\
do {\
    (bank_disable) = soc_alpm_mode_get((u));\
    if ((bank_disable)) {\
        /* parallel mode. Nothing for combined mode */\
        if (SOC_URPF_STATUS_GET((u))) {\
            (bank_disable) = 0x3; /* Search in banks, B2 and B3 */\
        } else {\
            (bank_disable) = 0;\
        }\
    }\
} while (0)

#define SOC_ALPM_GET_VRF_BANK_DISABLE(u, bank_disable)\
do {\
    (bank_disable) = soc_alpm_mode_get((u));\
    if ((bank_disable)) {\
        /* parallel mode. Nothing for combined mode */\
        if (SOC_URPF_STATUS_GET((u))) {\
            (bank_disable) = 0xC; /* Search in banks, B1 and B0 */\
        } else {\
            (bank_disable) = 0;\
        }\
    }\
} while (0)

/* Bucket Management Functions */
/* The Buckets from the shared RAM are assigned to TCAM PIVOTS to store
 * prefixes. The bucket once assigned to a TCAM PIVOT will be in use till the
 * PIVOT is active (at least one Prefix is used in the bucket). If no entries
 * are in the bucket, the bucket can be treated as free. The bucket pointers
 * follow TCAM entries when the TCAM entires are moved up or down to make
 * space.
 */

/* Shared bitmap routines are used to track bucket usage */
typedef struct soc_alpm_bucket_s {
    SHR_BITDCL *alpm_bucket_bmap;
    int alpm_bucket_bmap_size;
    int bucket_count;
    int next_free;
}soc_alpm_bucket_t;

extern soc_alpm_bucket_t soc_alpm_bucket[];

#define SOC_ALPM_BUCKET_BMAP(u)  (soc_alpm_bucket[u].alpm_bucket_bmap)
#define SOC_ALPM_BUCKET_BMAP_BYTE(u,i)  (soc_alpm_bucket[u].alpm_bucket_bmap[i])
#define SOC_ALPM_BUCKET_BMAP_SIZE(u)  (soc_alpm_bucket[u].alpm_bucket_bmap_size)
#define SOC_ALPM_BUCKET_NEXT_FREE(u)  (soc_alpm_bucket[u].next_free)
#define SOC_ALPM_BUCKET_COUNT(u)      (soc_alpm_bucket[u].bucket_count)
#define SOC_ALPM_BUCKET_MAX_INDEX(u)  (soc_alpm_bucket[u].bucket_count - 1)

#define SOC_ALPM_RPF_BKT_IDX(u, bkt) \
    bkt + SOC_ALPM_BUCKET_COUNT(u) 

#define PRESERVE_HIT                TRUE

typedef struct _payload_s payload_t;
struct _payload_s {
    trie_node_t node; /*trie node */
    payload_t *next; /* list node */
    unsigned int key[BITS2WORDS(_MAX_KEY_LEN_)];
    unsigned int len;
    int index;   /* Memory location */
};

/*
 * Table Operations for ALPM
 */

/* Generic AUX operation function */
typedef enum _soc_aux_op_s {
    INSERT_PROPAGATE,
    DELETE_PROPAGATE,
    PREFIX_LOOKUP,
    HITBIT_REPLACE
}_soc_aux_op_t;

extern int _soc_alpm_aux_op(int u, _soc_aux_op_t aux_op, 
                 defip_aux_scratch_entry_t *aux_entry,
                 int *hit, int *tcam_index, int *bucket_index);
extern int _ipmask2pfx(uint32 ipv4m, int *mask_len);

extern int alpm_bucket_assign(int u, int *bucket_pointer, int v6);
extern int alpm_bucket_release(int u, int bucket_pointer, int v6);

/* Per VRF PIVOT and Prefix trie. Each VRF will host a trie based on IPv4, IPV6-64 and IPV6-128
 * This seperation reduces the complexity of trie management.
 */
typedef struct alpm_vrf_handle_s {
    trie_t *pivot_trie_ipv4;     /* IPV4 Pivot trie */
    trie_t *pivot_trie_ipv6;     /* IPV6-64 Pivot trie */
    trie_t *prefix_trie_ipv4;    /* IPV4 Pivot trie */
    trie_t *prefix_trie_ipv6;    /* IPV6-64 Pivot trie */
    trie_t *prefix_trie_ipv6_128;    /* IPV6-128 Prefix trie */
    defip_entry_t *lpm_entry;          /* IPv4 Default LPM entry */
    defip_entry_t *lpm_entry_v6;       /* IPv6 Default LPM entry */
    defip_pair_128_entry_t *lpm_entry_v6_128;   /* IPv6-128 Default LPM entry */
    int count;                   /* no. of routes for this vrf */
    int count_v6_64;
    int count_v6_128;
    int init_done;               /* Init for VRF completed */
                                 /* ready to accept route additions */
} alpm_vrf_handle_t;

extern alpm_vrf_handle_t alpm_vrf_handle[SOC_MAX_NUM_DEVICES][MAX_VRF_ID];

/* 
 * Bucket Hnadle
 */
typedef struct alpm_bucket_handle_s {
    trie_t *bucket_trie;   /* trie of Prefix within this bucket */
    int bucket_index;      /* bucket Pointer */
} alpm_bucket_handle_t;

/*
 * Pivot Structure
 */
typedef struct alpm_pivot_s {
    trie_node_t node; /*trie node */
    /* dq_t        listnode;*/ /* list node */
    alpm_bucket_handle_t *bucket;  /* Bucket trie */
    unsigned int key[BITS2WORDS(_MAX_KEY_LEN_)];  /* pivot */
    unsigned int len;                             /* pivot length */
    int tcam_index;   /* TCAM index where the pivot is inserted */
}alpm_pivot_t;


#define MAX_PIVOT_COUNT 16384

/* Array of Pivots */
extern alpm_pivot_t *tcam_pivot[SOC_MAX_NUM_DEVICES][MAX_PIVOT_COUNT];

#define ALPM_TCAM_PIVOT(u, index)    tcam_pivot[u][index]

#define PIVOT_BUCKET_HANDLE(p)    (p)->bucket
#define PIVOT_BUCKET_TRIE(p)      ((p)->bucket)->bucket_trie
#define PIVOT_BUCKET_INDEX(p)     ((p)->bucket)->bucket_index
#define PIVOT_TCAM_INDEX(p)       ((p)->tcam_index)


#define VRF_PIVOT_TRIE_IPV4(u, vrf)         \
    alpm_vrf_handle[u][vrf].pivot_trie_ipv4
#define VRF_PIVOT_TRIE_IPV6(u, vrf)         \
    alpm_vrf_handle[u][vrf].pivot_trie_ipv6
#define VRF_PREFIX_TRIE_IPV4(u, vrf)        \
    alpm_vrf_handle[u][vrf].prefix_trie_ipv4
#define VRF_PREFIX_TRIE_IPV6(u, vrf)        \
    alpm_vrf_handle[u][vrf].prefix_trie_ipv6
#define VRF_PREFIX_TRIE_IPV6_128(u, vrf)    \
    alpm_vrf_handle[u][vrf].prefix_trie_ipv6_128

#define L3_DEFIP_MODE_64    1
#define L3_DEFIP_MODE_128   2

#define VRF_TRIE_INIT_DONE(u, vrf, v6, val)      \
do {\
    alpm_vrf_handle[u][vrf].init_done &= ~(1 << (v6));\
    alpm_vrf_handle[(u)][(vrf)].init_done |= ((val) & 1) << (v6);\
} while (0) 

#define VRF_TRIE_INIT_COMPLETED(u, vrf, v6)     ((alpm_vrf_handle[u][vrf].init_done & (1 << (v6))) != 0)

#define VRF_TRIE_DEFAULT_ROUTE_IPV4(u, vrf)      alpm_vrf_handle[u][vrf].lpm_entry
#define VRF_TRIE_DEFAULT_ROUTE_IPV6(u, vrf)      alpm_vrf_handle[u][vrf].lpm_entry_v6
#define VRF_TRIE_DEFAULT_ROUTE_IPV6_128(u, vrf)      alpm_vrf_handle[u][vrf].lpm_entry_v6_128

#define VRF_TRIE_ROUTES_INC(u, vrf, v6)     \
do {\
    if (!(v6)) {\
        alpm_vrf_handle[(u)][(vrf)].count++;\
    } else if ((v6) == 1) {\
        alpm_vrf_handle[(u)][(vrf)].count_v6_64++;\
    } else {\
        alpm_vrf_handle[(u)][(vrf)].count_v6_128++;\
    }\
} while (0)

#define VRF_TRIE_ROUTES_DEC(u, vrf, v6) \
do {\
    if (!(v6)) {\
        alpm_vrf_handle[(u)][(vrf)].count--;\
    } else if ((v6) == 1) {\
        alpm_vrf_handle[(u)][(vrf)].count_v6_64--;\
    } else {\
        alpm_vrf_handle[(u)][(vrf)].count_v6_128--;\
    }\
} while (0)
 
#define VRF_TRIE_ROUTES_CNT(u, vrf, v6)  \
    (((v6) == 0) ? alpm_vrf_handle[(u)][(vrf)].count : (((v6) == 1) ? \
    alpm_vrf_handle[(u)][(vrf)].count_v6_64 : \
    alpm_vrf_handle[(u)][(vrf)].count_v6_128))

/* Used to store the list of Prefixes that need to be moved to new bucket */
#define MAX_PREFIX_PER_BUCKET 32
#define SOC_ALPM_LPM_LOCK(u)             soc_mem_lock(u, L3_DEFIPm)
#define SOC_ALPM_LPM_UNLOCK(u)           soc_mem_unlock(u, L3_DEFIPm)

#define SOC_ALPM_LPM_CACHE_FIELD_CREATE(m, f) soc_field_info_t * m##f
#define SOC_ALPM_LPM_CACHE_FIELD_ASSIGN(m_u, s, m, f) \
                (s)->m##f = soc_mem_fieldinfo_get(m_u, m, f)

typedef struct {
    payload_t *prefix[MAX_PREFIX_PER_BUCKET];
    /* indicates success or failure on bucket split move */
    int status[MAX_PREFIX_PER_BUCKET];
    int count;
}alpm_mem_prefix_array_t;

extern int _soc_alpm_rpf_entry(int u, int idx);
extern int _soc_alpm_find_in_bkt(int u, soc_mem_t mem, int bucket_index, 
                                 int bank_disable, uint32 *e, void *alpm_data, 
                                 int *key_index, int v6);
extern int alpm_mem_prefix_array_cb(trie_node_t *node, void *info);
extern int alpm_delete_node_cb(trie_node_t *node, void *info);
extern int _soc_alpm_insert_in_bkt(int u, soc_mem_t mem, int bucket_index,
                                   int bank_disable, void *alpm_data, 
                                   uint32 *e, int *key_index, int v6);
extern int _soc_alpm_delete_in_bkt(int u, soc_mem_t mem, int delete_bucket, 
                                   int bank_disable, void *bufp2, uint32 *e, 
                                   int *key_index, int v6);
extern int soc_alpm_128_init(int u);
extern int soc_alpm_128_state_clear(int u);
extern int soc_alpm_128_deinit(int u);
extern int soc_alpm_128_lpm_init(int u);
extern int soc_alpm_128_lpm_deinit(int u);
extern int _soc_alpm_aux_op(int u, _soc_aux_op_t aux_op, 
                            defip_aux_scratch_entry_t *aux_entry, int *hit, 
                            int *tcam_index, int *bucket_index);
extern int soc_alpm_physical_idx(int u, soc_mem_t mem, int index, int full);
extern int soc_alpm_logical_idx(int u, soc_mem_t mem, int index, int full);
#endif /* _ESW_TRIDENT2_ALPM_INT_H */
