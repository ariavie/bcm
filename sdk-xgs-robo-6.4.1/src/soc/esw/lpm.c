/* $Id: lpm.c,v 1.57 Broadcom SDK $
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
 * Firebolt LPM TCAM table insert/delete/lookup routines
 * soc_fb_lpm_init - Called from bcm_l3_init
 * soc_fb_lpm_insert - Insert/Update an IPv4/IPV6 route entry into LPM table
 * soc_fb_lpm_delete - Delete an IPv4/IPV6 route entry from LPM table
 * soc_fb_lpm_match  - (Exact match for the key. Will match both IP address
 *                      and mask)
 * soc_fb_lpm_lookup (LPM search) - Not Implemented
 *
 * Hit bit preservation - May loose HIT bit state when entries are moved
 * around due to race condition. This happens if the HIT bit gets set in
 * hardware after reading the entry to be moved and before the move is
 * completed. If the HIT bit for an entry is already set when the move is
 * initiated then it is preserved.
 *
 */

#include <shared/bsl.h>

#include <soc/mem.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/lpm.h>
#include <shared/util.h>
#include <shared/avl.h>
#include <shared/l3.h>
#include <shared/bsl.h>
#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_L3)
static soc_kt_lpm_ipv6_info_t *kt_lpm_ipv6_info[SOC_MAX_NUM_DEVICES];
#define L3_DEFIP_TCAM_SIZE 1024
#endif
#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
/* Matches with _BCM_TR3_DEFIP_TCAM_DEPTH in triumph3.h */
#define _SOC_TR3_DEFIP_TCAM_DEPTH 1024 
#endif
#ifdef BCM_FIREBOLT_SUPPORT
#if defined(INCLUDE_L3)

/*
 * TCAM based LPM implementation. Each table entry can hold two IPV4 entries or
 * one IPV6 entry. VRF independent routes placed at the beginning or 
 * at the end of table based on application provided entry vrf id 
 * (SOC_L3_VRF_OVERRIDE/SOC_L3_VRF_GLOBAL).   
 *
 *              MAX_PFX_INDEX
 * lpm_prefix_index[98].begin ---> ===============================
 *                                 ==                           ==
 *                                 ==    0                      ==
 * lpm_prefix_index[98].end   ---> ===============================
 *
 * lpm_prefix_index[97].begin ---> ===============================
 *                                 ==                           ==
 *                                 ==    IPV6  Prefix Len = 64  ==
 * lpm_prefix_index[97].end   ---> ===============================
 *
 *
 *
 * lpm_prefix_index[x].begin --->  ===============================
 *                                 ==                           ==
 *                                 ==                           ==
 * lpm_prefix_index[x].end   --->  ===============================
 *
 *
 *              IPV6_PFX_ZERO
 * lpm_prefix_index[33].begin ---> ===============================
 *                                 ==                           ==
 *                                 ==    IPV6  Prefix Len = 0   ==
 * lpm_prefix_index[33].end   ---> ===============================
 *
 *
 * lpm_prefix_index[32].begin ---> ===============================
 *                                 ==                           ==
 *                                 ==    IPV4  Prefix Len = 32  ==
 * lpm_prefix_index[32].end   ---> ===============================
 *
 *
 *
 * lpm_prefix_index[0].begin --->  ===============================
 *                                 ==                           ==
 *                                 ==    IPV4  Prefix Len = 0   ==
 * lpm_prefix_index[0].end   --->  ===============================
 *
 *
 */

#if defined(FB_LPM_HASH_SUPPORT)
#undef FB_LPM_AVL_SUPPORT
#elif defined(FB_LPM_AVL_SUPPORT)
#else
#define FB_LPM_HASH_SUPPORT 1
#endif

#ifdef FB_LPM_CHECKER_ENABLE
#define FB_LPM_AVL_SUPPORT 1
#define FB_LPM_HASH_SUPPORT 1
#endif

#define SOC_MEM_COMPARE_RETURN(a, b) {          \
        if ((a) < (b)) { return -1; }           \
        if ((a) > (b)) { return  1; }           \
}

/* Can move to SOC Control structures */
soc_lpm_state_p soc_lpm_state[SOC_MAX_NUM_DEVICES];
soc_lpm128_table_t *soc_lpm128_state_table[SOC_MAX_NUM_DEVICES];
int *soc_lpm128_index_to_pfx_group[SOC_MAX_NUM_DEVICES];
soc_lpm_stat_p soc_lpm_stat[SOC_MAX_NUM_DEVICES];

#define SOC_LPM_CACHE_FIELD_CREATE(m, f) soc_field_info_t * m##f
#define SOC_LPM_CACHE_FIELD_ASSIGN(m_u, s, m, f) \
                (s)->m##f = soc_mem_fieldinfo_get(m_u, m, f)
typedef struct soc_lpm_field_cache_s {
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, CLASS_ID0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, CLASS_ID1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, DST_DISCARD0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, DST_DISCARD1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, ECMP0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, ECMP1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, ECMP_COUNT0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, ECMP_COUNT1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, ECMP_PTR0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, ECMP_PTR1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, GLOBAL_ROUTE0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, GLOBAL_ROUTE1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, HIT0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, HIT1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, IP_ADDR0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, IP_ADDR1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, IP_ADDR_MASK0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, IP_ADDR_MASK1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, MODE0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, MODE1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, MODE_MASK0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, MODE_MASK1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, NEXT_HOP_INDEX0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, NEXT_HOP_INDEX1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, PRI0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, PRI1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, RPE0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, RPE1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, VALID0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, VALID1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, VRF_ID_0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, VRF_ID_1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, VRF_ID_MASK0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, VRF_ID_MASK1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, ENTRY_TYPE0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, ENTRY_TYPE1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, ENTRY_TYPE_MASK0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, ENTRY_TYPE_MASK1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, D_ID0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, D_ID1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, D_ID_MASK0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, D_ID_MASK1f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, DEFAULTROUTE0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, DEFAULTROUTE1f);
} soc_lpm_field_cache_t, *soc_lpm_field_cache_p;
static soc_lpm_field_cache_p soc_lpm_field_cache_state[SOC_MAX_NUM_DEVICES];

#define SOC_MEM_OPT_F32_GET(m_unit, m_mem, m_entry_data, m_field) \
        soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(m_unit, m_mem)), (m_entry_data), (soc_lpm_field_cache_state[(m_unit)]->m_mem##m_field))

#define SOC_MEM_OPT_F32_SET(m_unit, m_mem, m_entry_data, m_field, m_val) \
        soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(m_unit, m_mem)), (m_entry_data), (soc_lpm_field_cache_state[(m_unit)]->m_mem##m_field), (m_val))


#define SOC_MEM_OPT_FIELD_VALID(m_unit, m_mem, m_field) \
                ((soc_lpm_field_cache_state[(m_unit)]->m_mem##m_field) != NULL)

#if defined(FB_LPM_AVL_SUPPORT)
typedef struct soc_lpm_avl_state_s {
    uint32      mask0;
    uint32      mask1;
    uint32      ip0;
    uint32      ip1;
    uint32      vrf_id;
    uint32      index;
} soc_lpm_avl_state_t, *soc_lpm_avl_state_p;

static shr_avl_t   *avl[SOC_MAX_NUM_DEVICES];   /* AVL tree to track entries */

#define SOC_LPM_STATE_AVL(u)         (avl[(u)])
/*
 * Function:
 *      soc_fb_lpm_avl_compare_key
 * Purpose:
 *      Comparison function for AVL shadow table operations.
 */
int
soc_fb_lpm_avl_compare_key(void *user_data,
                          shr_avl_datum_t *datum1,
                          shr_avl_datum_t *datum2)
{
    soc_lpm_avl_state_p d1, d2;
    
    /* COMPILER_REFERENCE(user_data);*/
    d1 = (soc_lpm_avl_state_p)datum1;
    d2 = (soc_lpm_avl_state_p)datum2;

    SOC_MEM_COMPARE_RETURN(d1->vrf_id, d2->vrf_id);
    SOC_MEM_COMPARE_RETURN(d1->mask1, d2->mask1);
    SOC_MEM_COMPARE_RETURN(d1->mask0, d2->mask0);
    SOC_MEM_COMPARE_RETURN(d1->ip1, d2->ip1);
    SOC_MEM_COMPARE_RETURN(d1->ip0, d2->ip0);

    return(0);
}

#define LPM_AVL_INSERT_IPV6(u, entry_data, tab_index)                        \
{                                                                            \
    soc_lpm_avl_state_t     d;                                               \
    d.ip0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR0f);        \
    d.ip1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR1f);        \
    d.mask0 =                                                                \
        SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR_MASK0f);       \
    d.mask1 =                                                                \
        SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR_MASK1f);       \
    if ((!(SOC_IS_HURRICANE(u))) && \
	 (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_0f))) {                      \
        d.vrf_id = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VRF_ID_0f); \
    } else {                                                                 \
        d.vrf_id = 0;                                                        \
    }                                                                        \
    d.index = tab_index;                                                     \
    if (shr_avl_insert(SOC_LPM_STATE_AVL(u),                                 \
                       soc_fb_lpm_avl_compare_key,                           \
                       (shr_avl_datum_t *)&d)) {                             \
    }                                                                        \
}

#define LPM_AVL_INSERT_IPV4_0(u, entry_data, tab_index)                      \
{                                                                            \
    soc_lpm_avl_state_t     d;                                               \
    d.ip0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR0f);        \
    d.mask0 =                                                                \
        SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR_MASK0f);       \
    d.ip1 = 0;                                                               \
    d.mask1 = 0x80000001;                                                    \
    if ((!(SOC_IS_HURRICANE(u))) && \
	 (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_0f))) {                      \
        d.vrf_id = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VRF_ID_0f); \
    } else {                                                                 \
        d.vrf_id = 0;                                                        \
    }                                                                        \
    d.index = (tab_index << 1);                                              \
    if (shr_avl_insert(SOC_LPM_STATE_AVL(u),                                 \
                       soc_fb_lpm_avl_compare_key,                           \
                       (shr_avl_datum_t *)&d)) {                             \
    }                                                                        \
}

#define LPM_AVL_INSERT_IPV4_1(u, entry_data, tab_index)                      \
{                                                                            \
    soc_lpm_avl_state_t     d;                                               \
    d.ip0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR1f);        \
    d.mask0 =                                                                \
        SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR_MASK1f);       \
    d.ip1 = 0;                                                               \
    d.mask1 = 0x80000001;                                                    \
    if ((!(SOC_IS_HURRICANE(u))) && \
	 (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_1f))) {                      \
        d.vrf_id = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VRF_ID_1f); \
    } else {                                                                 \
        d.vrf_id = 0;                                                        \
    }                                                                        \
    d.index = (tab_index << 1) + 1;                                          \
    if (shr_avl_insert(SOC_LPM_STATE_AVL(u),                                 \
                       soc_fb_lpm_avl_compare_key,                           \
                       (shr_avl_datum_t *)&d)) {                             \
    }                                                                        \
}

#define LPM_AVL_INSERT(u, entry_data, tab_index)                        \
        soc_fb_lpm_avl_insert(u, entry_data, tab_index)
#define LPM_AVL_DELETE(u, entry_data, tab_index)                        \
        soc_fb_lpm_avl_delete(u, entry_data)
#define LPM_AVL_LOOKUP(u, key_data, pfx, tab_index)                     \
        soc_fb_lpm_avl_lookup(u, key_data, tab_index)


void soc_fb_lpm_avl_insert(int u, void *entry_data, uint32 tab_index)
{
    if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, MODE0f)) {        
        /* IPV6 entry */                                                
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VALID1f) &&   
            SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VALID0f)) {   
            LPM_AVL_INSERT_IPV6(u, entry_data, tab_index)               
        }                                                               
    } else {                                                            
        /* IPV4 entry */                                                
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VALID1f)) {   
            LPM_AVL_INSERT_IPV4_1(u, entry_data, tab_index)            
        }                                                               
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VALID0f)) {   
            LPM_AVL_INSERT_IPV4_0(u, entry_data, tab_index)             
        }                                                               
    }
}

void soc_fb_lpm_avl_delete(int u, void *key_data)
{
    soc_lpm_avl_state_t     d;

    d.ip0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, IP_ADDR0f);
    d.mask0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, IP_ADDR_MASK0f);

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_0f)) {
        d.vrf_id = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, VRF_ID_0f); 
    } else {
        d.vrf_id = 0;
    }                                                                        

    if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, MODE0f)) {
        d.ip1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, IP_ADDR1f);
        d.mask1 =
            SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, IP_ADDR_MASK1f);
    } else {
        d.ip1 = 0;
        d.mask1 = 0x80000001;
    }
    if (shr_avl_delete(SOC_LPM_STATE_AVL(u),
                       soc_fb_lpm_avl_compare_key,
                       (shr_avl_datum_t *)&d)) {
    }
}

int soc_fb_lpm_avl_lookup(int u, void *key_data, int *tab_index)
{
    soc_lpm_avl_state_t     d;

    d.ip0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, IP_ADDR0f);
    d.mask0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm,  key_data, IP_ADDR_MASK0f);

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_0f)) {
        d.vrf_id = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, VRF_ID_0f); 
    } else {
        d.vrf_id = 0;
    }                                                                        

    /* Entry Type: IPv4/IPv6 */
    if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, MODE0f)) {
        d.ip1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm,  key_data, IP_ADDR1f);
        d.mask1 =
            SOC_MEM_OPT_F32_GET(u, L3_DEFIPm,  key_data, IP_ADDR_MASK1f);
    } else {
        d.ip1 = 0;
        d.mask1 = 0x80000001;
    }
    if (shr_avl_lookup(SOC_LPM_STATE_AVL(u),
                       soc_fb_lpm_avl_compare_key,
                       (shr_avl_datum_t *)&d)) {
        *tab_index = d.index;
    } else {
        return SOC_E_NOT_FOUND;
    }
    return SOC_E_NONE;
}
#else
#define LPM_AVL_INSERT(u, entry_data, tab_index)
#define LPM_AVL_DELETE(u, entry_data, tab_index)
#define LPM_AVL_LOOKUP(u, key_data, pfx, tab_index)
#endif /* FB_LPM_AVL_SUPPORT */

#ifdef FB_LPM_HASH_SUPPORT
#define _SOC_LPM128_HASH_KEY_LEN_IN_WORDS 10
typedef struct _soc_fb_lpm_hash_s {
    int         unit;
    int         entry_count;    /* Number entries in hash table */
    int         index_count;    /* Hash index max value + 1 */
    uint16      *table;         /* Hash table with 16 bit index */
    uint16      *link_table;    /* To handle collisions */
} _soc_fb_lpm_hash_t;

typedef uint32 _soc_fb_lpm_hash_entry_t[6];
typedef uint32 _soc_fb_lpm128_hash_entry_t[_SOC_LPM128_HASH_KEY_LEN_IN_WORDS];
typedef int (*_soc_fb_lpm_hash_compare_fn)(_soc_fb_lpm_hash_entry_t key1,
                                           _soc_fb_lpm_hash_entry_t key2);
typedef int (*_soc_fb_lpm128_hash_compare_fn)(_soc_fb_lpm128_hash_entry_t key1,
                                           _soc_fb_lpm128_hash_entry_t key2);
static _soc_fb_lpm_hash_t *_fb_lpm_hash_tab[SOC_MAX_NUM_DEVICES];
static _soc_fb_lpm_hash_t *_fb_lpm128_hash_tab[SOC_MAX_NUM_DEVICES];

static uint32 fb_lpm_hash_index_mask = 0;

#define SOC_LPM_STATE_HASH(u)           (_fb_lpm_hash_tab[(u)])
#define SOC_LPM128_STATE_HASH(u)           (_fb_lpm128_hash_tab[(u)])
#define SOC_FB_LPM128_HASH_ENTRY_IPV6_GET(u, entry_data, entry_data_upr, odata)\
    odata[0] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data_upr, IP_ADDR1f);   \
    odata[1] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data_upr, IP_ADDR_MASK1f);   \
    odata[2] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data_upr, IP_ADDR0f);   \
    odata[3] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data_upr, IP_ADDR_MASK0f);   \
    odata[4] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR1f);  \
    odata[5] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR_MASK1f);  \
    odata[6] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR0f);   \
    odata[7] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR_MASK0f);   \
    if ( (!(SOC_IS_HURRICANE(u))) &&   \
        (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_0f))) {                \
        odata[8] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VRF_ID_0f); \
        soc_fb_lpm_hash_vrf_0_get(u, entry_data, &odata[9]);                 \
    } else {                                                                 \
        odata[8] = 0;                                                        \
        odata[9] = 0;                                                        \
    }

#define SOC_FB_LPM_HASH_ENTRY_IPV6_GET(u, entry_data, odata)                 \
    odata[0] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR0f);     \
    odata[1] =                                                               \
        SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR_MASK0f);       \
    odata[2] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR1f);     \
    odata[3] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR_MASK1f);\
    if ( (!(SOC_IS_HURRICANE(u))) &&   \
        (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_0f))) {                \
        odata[4] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VRF_ID_0f); \
        soc_fb_lpm_hash_vrf_0_get(u, entry_data, &odata[5]);                 \
    } else {                                                                 \
        odata[4] = 0;                                                        \
        odata[5] = 0;                                                        \
    }

#define SOC_FB_LPM_HASH_ENTRY_IPV4_0_GET(u, entry_data, odata)               \
    odata[0] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR0f);     \
    odata[1] =                                                               \
        SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR_MASK0f);       \
    odata[2] = 0;                                                            \
    odata[3] = 0x80000001;                                                   \
    if ((!(SOC_IS_HURRICANE(u))) && \
        (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_0f))) {                \
        odata[4] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VRF_ID_0f); \
        soc_fb_lpm_hash_vrf_0_get(u, entry_data, &odata[5]); \
    } else {                                                                 \
        odata[4] = 0;                                                        \
        odata[5] = 0;                                                        \
    }

#define SOC_FB_LPM_HASH_ENTRY_IPV4_1_GET(u, entry_data, odata)               \
    odata[0] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR1f);     \
    odata[1] =                                                               \
        SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, IP_ADDR_MASK1f);       \
    odata[2] = 0;                                                            \
    odata[3] = 0x80000001;                                                   \
    if ((!(SOC_IS_HURRICANE(u))) &&  \
        (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_1f))) {                \
        odata[4] = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VRF_ID_1f); \
        soc_fb_lpm_hash_vrf_1_get(u, entry_data, &odata[5]);                 \
    } else {                                                                 \
        odata[4] = 0;                                                        \
        odata[5] = 0;                                                        \
    }


#define INDEX_ADD(hash, hash_idx, new_idx)                      \
    hash->link_table[new_idx & FB_LPM_HASH_INDEX_MASK] =        \
        hash->table[hash_idx];                                  \
    hash->table[hash_idx] = new_idx

#define INDEX_ADD_LINK(hash, t_index, new_idx)                  \
    hash->link_table[new_idx & FB_LPM_HASH_INDEX_MASK] =        \
        hash->link_table[t_index & FB_LPM_HASH_INDEX_MASK];     \
    hash->link_table[t_index & FB_LPM_HASH_INDEX_MASK] = new_idx

#define INDEX_UPDATE(hash, hash_idx, old_idx, new_idx)          \
    hash->table[hash_idx] = new_idx;                            \
    hash->link_table[new_idx & FB_LPM_HASH_INDEX_MASK] =        \
        hash->link_table[old_idx & FB_LPM_HASH_INDEX_MASK];     \
    hash->link_table[old_idx & FB_LPM_HASH_INDEX_MASK] = FB_LPM_HASH_INDEX_NULL

#define INDEX_UPDATE_LINK(hash, prev_idx, old_idx, new_idx)             \
    hash->link_table[prev_idx & FB_LPM_HASH_INDEX_MASK] = new_idx;      \
    hash->link_table[new_idx & FB_LPM_HASH_INDEX_MASK] =                \
        hash->link_table[old_idx & FB_LPM_HASH_INDEX_MASK];             \
    hash->link_table[old_idx & FB_LPM_HASH_INDEX_MASK] = FB_LPM_HASH_INDEX_NULL

#define INDEX_DELETE(hash, hash_idx, del_idx)                   \
    hash->table[hash_idx] =                                     \
        hash->link_table[del_idx & FB_LPM_HASH_INDEX_MASK];     \
    hash->link_table[del_idx & FB_LPM_HASH_INDEX_MASK] =        \
        FB_LPM_HASH_INDEX_NULL

#define INDEX_DELETE_LINK(hash, prev_idx, del_idx)              \
    hash->link_table[prev_idx & FB_LPM_HASH_INDEX_MASK] =       \
        hash->link_table[del_idx & FB_LPM_HASH_INDEX_MASK];     \
    hash->link_table[del_idx & FB_LPM_HASH_INDEX_MASK] =        \
        FB_LPM_HASH_INDEX_NULL

/*
 * Function:
 *      soc_fb_lpm_hash_vrf_0_get and soc_fb_lpm_hash_vrf_1_get
 * Purpose:
 *      Service routine used to determine the vrf type 
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      lpm_entry - (IN)Route info buffer from hw.
 *      vrf_id    - (OUT)Virtual router id.
 * Returns:
 *      BCM_E_XXX
 */
STATIC void
soc_fb_lpm_hash_vrf_0_get(int unit, void *lpm_entry, uint32 *vrf)
{
    int vrf_id;

     
    /* Get Virtual Router id if supported. */
    if (SOC_MEM_OPT_FIELD_VALID(unit, L3_DEFIPm, VRF_ID_MASK0f)){
        vrf_id = SOC_MEM_OPT_F32_GET(unit, L3_DEFIPm, lpm_entry, VRF_ID_0f);

        /* Special vrf's handling. */
        if (SOC_MEM_OPT_F32_GET(unit, L3_DEFIPm, lpm_entry, VRF_ID_MASK0f)) {
            *vrf = _SOC_HASH_L3_VRF_SPECIFIC;    
        } else if (SOC_VRF_MAX(unit) == vrf_id) {
            *vrf = _SOC_HASH_L3_VRF_GLOBAL;
        } else {
            *vrf = _SOC_HASH_L3_VRF_OVERRIDE;    
            if (SOC_MEM_OPT_FIELD_VALID(unit, L3_DEFIPm, GLOBAL_ROUTE0f)) {
                if (SOC_MEM_OPT_F32_GET(unit, L3_DEFIPm, lpm_entry, GLOBAL_ROUTE0f)) {
                    *vrf = _SOC_HASH_L3_VRF_GLOBAL; 
                }
            }
        }
    } else {
        /* No vrf support on this device. */
        *vrf = _SOC_HASH_L3_VRF_DEFAULT;
    }
}

STATIC void
soc_fb_lpm_hash_vrf_1_get(int unit, void *lpm_entry, uint32 *vrf)
{
    int vrf_id;

    /* Get Virtual Router id if supported. */
    if (SOC_MEM_OPT_FIELD_VALID(unit, L3_DEFIPm, VRF_ID_MASK1f)){
        vrf_id = SOC_MEM_OPT_F32_GET(unit, L3_DEFIPm, lpm_entry, VRF_ID_1f);

        /* Special vrf's handling. */
        if (SOC_MEM_OPT_F32_GET(unit, L3_DEFIPm, lpm_entry, VRF_ID_MASK1f)) {
            *vrf = _SOC_HASH_L3_VRF_SPECIFIC;    
        } else if (SOC_VRF_MAX(unit) == vrf_id) {
            *vrf = _SOC_HASH_L3_VRF_GLOBAL;
        } else {
            *vrf = _SOC_HASH_L3_VRF_OVERRIDE;    
            if (SOC_MEM_OPT_FIELD_VALID(unit, L3_DEFIPm, GLOBAL_ROUTE1f)) {
                if (SOC_MEM_OPT_F32_GET(unit, L3_DEFIPm, lpm_entry, GLOBAL_ROUTE1f)) {
                    *vrf = _SOC_HASH_L3_VRF_GLOBAL; 
                }
            }
        }
    } else {
        /* No vrf support on this device. */
        *vrf = _SOC_HASH_L3_VRF_DEFAULT;
    }
}


#define SOC_FB_LPM_HASH_ENTRY_GET _soc_fb_lpm_hash_entry_get
#define SOC_FB_LPM128_HASH_ENTRY_GET _soc_fb_lpm128_hash_entry_get

static
void _soc_fb_lpm_hash_entry_get(int u, void *e,
                                int index, _soc_fb_lpm_hash_entry_t r_entry);
static
uint16 _soc_fb_lpm_hash_compute(uint8 *data, int data_nbits);
static
int _soc_fb_lpm_hash_create(int unit,
                            int entry_count,
                            int index_count,
                            _soc_fb_lpm_hash_t **fb_lpm_hash_ptr);
static
int _soc_fb_lpm_hash_destroy(_soc_fb_lpm_hash_t *fb_lpm_hash);
static
int _soc_fb_lpm_hash_lookup(_soc_fb_lpm_hash_t          *hash,
                            _soc_fb_lpm_hash_compare_fn key_cmp_fn,
                            _soc_fb_lpm_hash_entry_t    entry,
                            int                         pfx,
                            uint16                      *key_index);
static
int _soc_fb_lpm_hash_insert(_soc_fb_lpm_hash_t *hash,
                            _soc_fb_lpm_hash_compare_fn key_cmp_fn,
                            _soc_fb_lpm_hash_entry_t entry,
                            int    pfx,
                            uint16 old_index,
                            uint16 new_index);
static
int _soc_fb_lpm128_hash_insert(_soc_fb_lpm_hash_t *hash,
                            _soc_fb_lpm_hash_compare_fn key_cmp_fn,
                            _soc_fb_lpm128_hash_entry_t entry,
                            int    pfx,
                            uint16 old_index,
                            uint16 new_index);
static
int _soc_fb_lpm_hash_delete(_soc_fb_lpm_hash_t *hash,
                            _soc_fb_lpm_hash_compare_fn key_cmp_fn,
                            _soc_fb_lpm_hash_entry_t entry,
                            int    pfx,
                            uint16 delete_index);

/*
 *      Extract key data from an entry at the given index.
 */
static
void _soc_fb_lpm_hash_entry_get(int u, void *e,
                                int index, _soc_fb_lpm_hash_entry_t r_entry)
{
    if (index & FB_LPM_HASH_IPV6_MASK) {
        SOC_FB_LPM_HASH_ENTRY_IPV6_GET(u, e, r_entry);
    } else {
        if (index & 0x1) {
            SOC_FB_LPM_HASH_ENTRY_IPV4_1_GET(u, e, r_entry);
        } else {
            SOC_FB_LPM_HASH_ENTRY_IPV4_0_GET(u, e, r_entry);
        }
    }
}


/*
 * Function:
 *      _soc_fb_lpm_hash_compare_key
 * Purpose:
 *      Comparison function for AVL shadow table operations.
 */
static
int _soc_fb_lpm_hash_compare_key(_soc_fb_lpm_hash_entry_t key1,
                                 _soc_fb_lpm_hash_entry_t key2)
{
    int idx;

    for (idx = 0; idx < 6; idx++) { 
        SOC_MEM_COMPARE_RETURN(key1[idx], key2[idx]);
    }
    return (0);
}

/*
 * Function:
 *      _soc_fb_lpm_hash_compare_key
 * Purpose:
 *      Comparison function for AVL shadow table operations.
 */
static
int _soc_fb_lpm128_hash_compare_key(_soc_fb_lpm128_hash_entry_t key1,
                                 _soc_fb_lpm128_hash_entry_t key2)
{
    int idx;

    for (idx = 0; idx < _SOC_LPM128_HASH_KEY_LEN_IN_WORDS; idx++) { 
        SOC_MEM_COMPARE_RETURN(key1[idx], key2[idx]);
    }
    return (0);
}


/* #define FB_LPM_DEBUG*/
#ifdef FB_LPM_DEBUG      
#define H_INDEX_MATCH(str, tab_index, match_index)      \
    LOG_ERROR(BSL_LS_SOC_LPM, \
              (BSL_META("%s index: H %d A %d\n"),                  \
               str, (int)tab_index, match_index)
#else
#define H_INDEX_MATCH(str, tab_index, match_index)
#endif

#define LPM_NO_MATCH_INDEX 0x8000
#define LPM_HASH_INSERT(u, entry_data, tab_index)       \
    soc_fb_lpm_hash_insert(u, entry_data, tab_index, LPM_NO_MATCH_INDEX, 0)

#define LPM_HASH_DELETE(u, key_data, tab_index)         \
    soc_fb_lpm_hash_delete(u, key_data, tab_index)

#define LPM_HASH_LOOKUP(u, key_data, pfx, tab_index)    \
    soc_fb_lpm_hash_lookup(u, key_data, pfx, tab_index)

void soc_fb_lpm_hash_insert(int u, void *entry_data, uint32 tab_index,
                            uint32 old_index, int pfx)
{
    _soc_fb_lpm_hash_entry_t    key_hash;

    if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, MODE0f)) {
        /* IPV6 entry */
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VALID1f) &&
            SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VALID0f)) {
            SOC_FB_LPM_HASH_ENTRY_IPV6_GET(u, entry_data, key_hash);
            _soc_fb_lpm_hash_insert(
                        SOC_LPM_STATE_HASH(u),
                        _soc_fb_lpm_hash_compare_key,
                        key_hash,
                        pfx,
                        old_index,
                        ((uint16)tab_index << 1) | FB_LPM_HASH_IPV6_MASK);
        }
    } else {
        /* IPV4 entry */
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VALID0f)) {
            SOC_FB_LPM_HASH_ENTRY_IPV4_0_GET(u, entry_data, key_hash);
            _soc_fb_lpm_hash_insert(SOC_LPM_STATE_HASH(u),
                                    _soc_fb_lpm_hash_compare_key,
                                    key_hash,
                                    pfx,
                                    old_index,
                                    ((uint16)tab_index << 1));
        }
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VALID1f)) {
            SOC_FB_LPM_HASH_ENTRY_IPV4_1_GET(u, entry_data, key_hash);
            _soc_fb_lpm_hash_insert(SOC_LPM_STATE_HASH(u),
                                    _soc_fb_lpm_hash_compare_key,
                                    key_hash,
                                    pfx,
                                    old_index,
                                    (((uint16)tab_index << 1) + 1));
        }
    }
}

void soc_fb_lpm_hash_delete(int u, void *key_data, uint32 tab_index)
{
    _soc_fb_lpm_hash_entry_t    key_hash;
    int                         pfx = -1;
    int                         rv;
    uint16                      index;

    if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, MODE0f)) {
        SOC_FB_LPM_HASH_ENTRY_IPV6_GET(u, key_data, key_hash);
        index = (tab_index << 1) | FB_LPM_HASH_IPV6_MASK;
    } else {
        SOC_FB_LPM_HASH_ENTRY_IPV4_0_GET(u, key_data, key_hash);
        index = tab_index;
    }

    rv = _soc_fb_lpm_hash_delete(SOC_LPM_STATE_HASH(u),
                                 _soc_fb_lpm_hash_compare_key,
                                 key_hash, pfx, index);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "\ndel  index: H %d error %d\n"), index, rv));
    }
}

int soc_fb_lpm_hash_lookup(int u, void *key_data, int pfx, int *key_index)
{
    _soc_fb_lpm_hash_entry_t    key_hash;
    int                         is_ipv6;
    int                         rv;
    uint16                      index = FB_LPM_HASH_INDEX_NULL;

    is_ipv6 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, MODE0f);
    if (is_ipv6) {
        SOC_FB_LPM_HASH_ENTRY_IPV6_GET(u, key_data, key_hash);
    } else {
        SOC_FB_LPM_HASH_ENTRY_IPV4_0_GET(u, key_data, key_hash);
    }

    rv = _soc_fb_lpm_hash_lookup(SOC_LPM_STATE_HASH(u),
                                 _soc_fb_lpm_hash_compare_key,
                                 key_hash, pfx, &index);
    if (SOC_FAILURE(rv)) {
        *key_index = 0xFFFFFFFF;
        return(rv);
    }

    *key_index = index;

    return(SOC_E_NONE);
}

/* 
 * Function:
 *      _soc_fb_lpm_hash_compute
 * Purpose:
 *      Compute CRC hash for key data.
 * Parameters:
 *      data - Key data
 *      data_nbits - Number of data bits
 * Returns:
 *      Computed 16 bit hash
 */
static
uint16 _soc_fb_lpm_hash_compute(uint8 *data, int data_nbits)
{
    return (_shr_crc16b(0, data, data_nbits));
}

/* 
 * Function:
 *      _soc_fb_lpm_hash_create
 * Purpose:
 *      Create an empty hash table
 * Parameters:
 *      unit  - Device unit
 *      entry_count - Limit for number of entries in table
 *      index_count - Hash index max + 1. (index_count <= count)
 *      fb_lpm_hash_ptr - Return pointer (handle) to new Hash Table
 * Returns:
 *      SOC_E_NONE       Success
 *      SOC_E_MEMORY     Out of memory (system allocator)
 */

static
int _soc_fb_lpm_hash_create(int unit,
                            int entry_count,
                            int index_count,
                            _soc_fb_lpm_hash_t **fb_lpm_hash_ptr)
{
    _soc_fb_lpm_hash_t  *hash;
    int                 index;

    if (index_count > entry_count) {
        return SOC_E_MEMORY;
    }
    hash = sal_alloc(sizeof (_soc_fb_lpm_hash_t), "lpm_hash");
    if (hash == NULL) {
        return SOC_E_MEMORY;
    }

    sal_memset(hash, 0, sizeof (*hash));

    hash->unit = unit;
    hash->entry_count = entry_count;
    hash->index_count = index_count;

    /*
     * Pre-allocate the hash table storage.
     */
    hash->table = sal_alloc(hash->index_count * sizeof(*(hash->table)),
                            "hash_table");

    if (hash->table == NULL) {
        sal_free(hash);
        return SOC_E_MEMORY;
    }
    /*
     * In case where all the entries should hash into the same bucket
     * this will prevent the hash table overflow
     */
    hash->link_table = sal_alloc(
                            hash->entry_count * sizeof(*(hash->link_table)),
                            "link_table");
    if (hash->link_table == NULL) {
        sal_free(hash->table);
        sal_free(hash);
        return SOC_E_MEMORY;
    }

    /*
     * Set the entries in the hash table to FB_LPM_HASH_INDEX_NULL
     * Link the entries beyond hash->index_max for handling collisions
     */
    for(index = 0; index < hash->index_count; index++) {
        hash->table[index] = FB_LPM_HASH_INDEX_NULL;
    }
    for(index = 0; index < hash->entry_count; index++) {
        hash->link_table[index] = FB_LPM_HASH_INDEX_NULL;
    }
    *fb_lpm_hash_ptr = hash;
    return SOC_E_NONE;
}

/* 
 * Function:
 *      _soc_fb_lpm_hash_destroy
 * Purpose:
 *      Destroy the hash table
 * Parameters:
 *      fb_lpm_hash - Pointer (handle) to Hash Table
 * Returns:
 *      SOC_E_NONE       Success
 */
static
int _soc_fb_lpm_hash_destroy(_soc_fb_lpm_hash_t *fb_lpm_hash)
{
    if (fb_lpm_hash != NULL) {
        sal_free(fb_lpm_hash->table);
        sal_free(fb_lpm_hash->link_table);
        sal_free(fb_lpm_hash);
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      _soc_fb_lpm_hash_lookup
 * Purpose:
 *      Look up a key in the hash table
 * Parameters:
 *      hash - Pointer (handle) to Hash Table
 *      key_cmp_fn - Compare function which should compare key
 *      entry   - The key to lookup
 *      pfx     - Prefix length for lookup acceleration.
 *      key_index - (OUT)       Index where the key was found.
 * Returns:
 *      SOC_E_NONE      Key found
 *      SOC_E_NOT_FOUND Key not found
 */

static
int _soc_fb_lpm_hash_lookup(_soc_fb_lpm_hash_t          *hash,
                            _soc_fb_lpm_hash_compare_fn key_cmp_fn,
                            _soc_fb_lpm_hash_entry_t    entry,
                            int                         pfx,
                            uint16                      *key_index)
{
    int u = hash->unit;

    uint16 hash_val;
    uint16 index;

    hash_val = _soc_fb_lpm_hash_compute((uint8 *)entry,
                                        (32 * 6)) % hash->index_count;
    index = hash->table[hash_val];
    H_INDEX_MATCH("lhash", entry[0], hash_val);
    H_INDEX_MATCH("lkup ", entry[0], index);
    while(index != FB_LPM_HASH_INDEX_NULL) {
        uint32  e[SOC_MAX_MEM_FIELD_WORDS];
        _soc_fb_lpm_hash_entry_t  r_entry;
        int     rindex;

        rindex = (index & fb_lpm_hash_index_mask) >> 1;
        /*
         * Check prefix length and skip index if not valid for given length
        if ((SOC_LPM_STATE_START(u, pfx) <= rindex) &&
            (SOC_LPM_STATE_END(u, pfx) >= rindex)) {
         */
        SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, rindex, e));
        SOC_FB_LPM_HASH_ENTRY_GET(u, e, index, r_entry);
        if ((*key_cmp_fn)(entry, r_entry) == 0) {
            *key_index = (index & fb_lpm_hash_index_mask) >> 
                            ((index & FB_LPM_HASH_IPV6_MASK) ? 1 : 0);
            H_INDEX_MATCH("found", entry[0], index);
            return(SOC_E_NONE);
        }
        /*
        }
        */
        index = hash->link_table[index & fb_lpm_hash_index_mask];
        H_INDEX_MATCH("lkup1", entry[0], index);
    }
    H_INDEX_MATCH("not_found", entry[0], index);
    return(SOC_E_NOT_FOUND);
}

/*
 * Function:
 *      _soc_fb_lpm_hash_insert
 * Purpose:
 *      Insert/Update a key index in the hash table
 * Parameters:
 *      hash - Pointer (handle) to Hash Table
 *      key_cmp_fn - Compare function which should compare key
 *      entry   - The key to lookup
 *      pfx     - Prefix length for lookup acceleration.
 *      old_index - Index where the key was moved from.
 *                  FB_LPM_HASH_INDEX_NULL if new entry.
 *      new_index - Index where the key was moved to.
 * Returns:
 *      SOC_E_NONE      Key found
 */
/*
 *      Should be caled before updating the LPM table so that the
 *      data in the hash table is consistent with the LPM table
 */
static
int _soc_fb_lpm_hash_insert(_soc_fb_lpm_hash_t *hash,
                            _soc_fb_lpm_hash_compare_fn key_cmp_fn,
                            _soc_fb_lpm_hash_entry_t entry,
                            int    pfx,
                            uint16 old_index,
                            uint16 new_index)
{
    int u = hash->unit;
    uint16 hash_val;
    uint16 index;
    uint16 prev_index;

    hash_val = _soc_fb_lpm_hash_compute((uint8 *)entry,
                                        (32 * 6)) % hash->index_count;
    index = hash->table[hash_val];
    H_INDEX_MATCH("ihash", entry[0], hash_val);
    H_INDEX_MATCH("ins  ", entry[0], new_index);
    H_INDEX_MATCH("ins1 ", index, new_index);
    prev_index = FB_LPM_HASH_INDEX_NULL;
    if (old_index != FB_LPM_HASH_INDEX_NULL) {
        while(index != FB_LPM_HASH_INDEX_NULL) {
            uint32  e[SOC_MAX_MEM_FIELD_WORDS];
            _soc_fb_lpm_hash_entry_t  r_entry;
            int     rindex;
            
            rindex = (index & fb_lpm_hash_index_mask) >> 1;

            /*
             * Check prefix length and skip index if not valid for given length
            if ((SOC_LPM_STATE_START(u, pfx) <= rindex) &&
                (SOC_LPM_STATE_END(u, pfx) >= rindex)) {
             */
            SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, rindex, e));
            SOC_FB_LPM_HASH_ENTRY_GET(u, e, index, r_entry);
            if ((*key_cmp_fn)(entry, r_entry) == 0) {
                /* assert(old_index == index);*/
                if (new_index != index) {
                    H_INDEX_MATCH("imove", prev_index, new_index);
                    if (prev_index == FB_LPM_HASH_INDEX_NULL) {
                        INDEX_UPDATE(hash, hash_val, index, new_index);
                    } else {
                        INDEX_UPDATE_LINK(hash, prev_index, index, new_index);
                    }
                }
                H_INDEX_MATCH("imtch", index, new_index);
                return(SOC_E_NONE);
            }
            /*
            }
            */
            prev_index = index;
            index = hash->link_table[index & fb_lpm_hash_index_mask];
            H_INDEX_MATCH("ins2 ", index, new_index);
        }
    }
    INDEX_ADD(hash, hash_val, new_index);  /* new entry */
    return(SOC_E_NONE);
}

/*
 * Function:
 *      _soc_fb_lpm_hash_delete
 * Purpose:
 *      Delete a key index in the hash table
 * Parameters:
 *      hash - Pointer (handle) to Hash Table
 *      key_cmp_fn - Compare function which should compare key
 *      entry   - The key to delete
 *      pfx     - Prefix length for lookup acceleration.
 *      delete_index - Index to delete.
 * Returns:
 *      SOC_E_NONE      Success
 *      SOC_E_NOT_FOUND Key index not found.
 */
static
int _soc_fb_lpm_hash_delete(_soc_fb_lpm_hash_t *hash,
                            _soc_fb_lpm_hash_compare_fn key_cmp_fn,
                            _soc_fb_lpm_hash_entry_t entry,
                            int    pfx,
                            uint16 delete_index)
{
    uint16 hash_val;
    uint16 index;
    uint16 prev_index;

    hash_val = _soc_fb_lpm_hash_compute((uint8 *)entry,
                                        (32 * 6)) % hash->index_count;
    index = hash->table[hash_val];
    H_INDEX_MATCH("dhash", entry[0], hash_val);
    H_INDEX_MATCH("del  ", entry[0], index);
    prev_index = FB_LPM_HASH_INDEX_NULL;
    while(index != FB_LPM_HASH_INDEX_NULL) {
        if (delete_index == index) {
            H_INDEX_MATCH("dfoun", entry[0], index);
            if (prev_index == FB_LPM_HASH_INDEX_NULL) {
                INDEX_DELETE(hash, hash_val, delete_index);
            } else {
                INDEX_DELETE_LINK(hash, prev_index, delete_index);
            }
            return(SOC_E_NONE);
        }
        prev_index = index;
        index = hash->link_table[index & fb_lpm_hash_index_mask];
        H_INDEX_MATCH("del1 ", entry[0], index);
    }
    return(SOC_E_NOT_FOUND);
}

/*
 * Function:
 *      _soc_fb_lpm128_hash_delete
 * Purpose:
 *      Delete a key index in the hash table
 * Parameters:
 *      hash - Pointer (handle) to Hash Table
 *      key_cmp_fn - Compare function which should compare key
 *      entry   - The key to delete
 *      pfx     - Prefix length for lookup acceleration.
 *      delete_index - Index to delete.
 * Returns:
 *      SOC_E_NONE      Success
 *      SOC_E_NOT_FOUND Key index not found.
 */
static
int _soc_fb_lpm128_hash_delete(_soc_fb_lpm_hash_t *hash,
                            _soc_fb_lpm128_hash_compare_fn key_cmp_fn,
                            _soc_fb_lpm128_hash_entry_t entry,
                            int    pfx,
                            uint16 delete_index)
{
    uint16 hash_val;
    uint16 index;
    uint16 prev_index;

    hash_val = _soc_fb_lpm_hash_compute((uint8 *)entry,
               (32 * _SOC_LPM128_HASH_KEY_LEN_IN_WORDS)) % hash->index_count;
    index = hash->table[hash_val];
    H_INDEX_MATCH("dhash", entry[0], hash_val);
    H_INDEX_MATCH("del  ", entry[0], index);
    prev_index = FB_LPM_HASH_INDEX_NULL;
    while(index != FB_LPM_HASH_INDEX_NULL) {
        if (delete_index == index) {
            H_INDEX_MATCH("dfoun", entry[0], index);
            if (prev_index == FB_LPM_HASH_INDEX_NULL) {
                INDEX_DELETE(hash, hash_val, delete_index);
            } else {
                INDEX_DELETE_LINK(hash, prev_index, delete_index);
            }
            return(SOC_E_NONE);
        }
        prev_index = index;
        index = hash->link_table[index & fb_lpm_hash_index_mask];
        H_INDEX_MATCH("del1 ", entry[0], index);
    }
    return(SOC_E_NOT_FOUND);
}
#else
#define LPM_HASH_INSERT(u, entry_data, tab_index)
#define LPM_HASH_DELETE(u, key_data, tab_index)
#define LPM_HASH_LOOKUP(u, key_data, pfx, tab_index)
#endif /* FB_LPM_HASH_SUPPORT */

static
int
_ipmask2pfx(uint32 ipv4m, int *mask_len)
{
    *mask_len = 0;
    while (ipv4m & (1 << 31)) {
        *mask_len += 1;
        ipv4m <<= 1;
    }

    return ((ipv4m) ? SOC_E_PARAM : SOC_E_NONE);
}

/* src and dst can be same */
int
soc_fb_lpm_ip4entry0_to_0(int u, void *src, void *dst, int copy_hit)
{
    uint32      ipv4a;

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VALID0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VALID0f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, MODE0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, MODE0f, ipv4a);
#if defined(BCM_TRX_SUPPORT) || defined(BCM_RAVEN_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, MODE_MASK0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, MODE_MASK0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, MODE_MASK0f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT || BCM_RAVEN_SUPPORT */

#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, GLOBAL_ROUTE0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, GLOBAL_ROUTE0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, GLOBAL_ROUTE0f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT */

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, IP_ADDR0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, IP_ADDR0f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, IP_ADDR_MASK0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, IP_ADDR_MASK0f, ipv4a);

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, ECMP0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP0f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, ECMP_COUNT0f)) {
            ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP_COUNT0f);
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP_COUNT0f, ipv4a);

            ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP_PTR0f);
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP_PTR0f, ipv4a);
    } else {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, NEXT_HOP_INDEX0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, NEXT_HOP_INDEX0f, ipv4a);
    }

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, PRI0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, PRI0f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, RPE0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, RPE0f, ipv4a);

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, DEFAULTROUTE0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src,DEFAULTROUTE0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, DEFAULTROUTE0f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VRF_ID_0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VRF_ID_0f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VRF_ID_MASK0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VRF_ID_MASK0f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, ENTRY_TYPE0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ENTRY_TYPE0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ENTRY_TYPE0f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ENTRY_TYPE_MASK0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ENTRY_TYPE_MASK0f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, D_ID0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, D_ID0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, D_ID0f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, D_ID_MASK0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, D_ID_MASK0f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, DST_DISCARD0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, DST_DISCARD0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, DST_DISCARD0f, ipv4a);
    }
#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, CLASS_ID0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, CLASS_ID0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, CLASS_ID0f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT */
    if (copy_hit) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, HIT0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, HIT0f, ipv4a);
    }

    return(SOC_E_NONE);
}

int
soc_fb_lpm_ip4entry1_to_1(int u, void *src, void *dst, int copy_hit)
{
    uint32      ipv4a;

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VALID1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VALID1f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, MODE1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, MODE1f, ipv4a);
#if defined(BCM_TRX_SUPPORT) || defined(BCM_RAVEN_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, MODE_MASK1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, MODE_MASK1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, MODE_MASK1f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT || BCM_RAVEN_SUPPORT */

#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, GLOBAL_ROUTE1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, GLOBAL_ROUTE1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, GLOBAL_ROUTE1f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT */

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, IP_ADDR1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, IP_ADDR1f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, IP_ADDR_MASK1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, IP_ADDR_MASK1f, ipv4a);

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, ECMP1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP1f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, ECMP_COUNT1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP_COUNT1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP_COUNT1f, ipv4a);

        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP_PTR1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP_PTR1f, ipv4a);
    } else {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, NEXT_HOP_INDEX1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, NEXT_HOP_INDEX1f, ipv4a);
    }

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, PRI1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, PRI1f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, RPE1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, RPE1f, ipv4a);

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, DEFAULTROUTE1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src,DEFAULTROUTE1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, DEFAULTROUTE1f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VRF_ID_1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VRF_ID_1f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VRF_ID_MASK1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VRF_ID_MASK1f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, ENTRY_TYPE1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ENTRY_TYPE1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ENTRY_TYPE1f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ENTRY_TYPE_MASK1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ENTRY_TYPE_MASK1f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, D_ID1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, D_ID1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, D_ID1f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, D_ID_MASK1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, D_ID_MASK1f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, DST_DISCARD1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, DST_DISCARD1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, DST_DISCARD1f, ipv4a);
    }
#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, CLASS_ID1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, CLASS_ID1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, CLASS_ID1f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT */
    if (copy_hit) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, HIT1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, HIT1f, ipv4a);
    }

    return(SOC_E_NONE);
}

/* src and dst can be same */
int
soc_fb_lpm_ip4entry0_to_1(int u, void *src, void *dst, int copy_hit)
{
    uint32      ipv4a;

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VALID0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VALID1f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, MODE0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, MODE1f, ipv4a);
#if defined(BCM_TRX_SUPPORT) || defined(BCM_RAVEN_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, MODE_MASK0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, MODE_MASK0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, MODE_MASK1f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT || BCM_RAVEN_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, GLOBAL_ROUTE0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, GLOBAL_ROUTE0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, GLOBAL_ROUTE1f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT */

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, IP_ADDR0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, IP_ADDR1f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, IP_ADDR_MASK0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, IP_ADDR_MASK1f, ipv4a);

	if (!SOC_IS_HURRICANE(u)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP1f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, ECMP_COUNT0f)) {
            ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP_COUNT0f);
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP_COUNT1f, ipv4a);

            ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP_PTR0f);
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP_PTR1f, ipv4a);
    } else {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, NEXT_HOP_INDEX0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, NEXT_HOP_INDEX1f, ipv4a);
    }

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, PRI0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, PRI1f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, RPE0f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, RPE1f, ipv4a);

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, DEFAULTROUTE0f) &&
        SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, DEFAULTROUTE1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src,DEFAULTROUTE0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, DEFAULTROUTE1f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VRF_ID_0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VRF_ID_1f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VRF_ID_MASK0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VRF_ID_MASK1f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, ENTRY_TYPE0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ENTRY_TYPE0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ENTRY_TYPE1f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ENTRY_TYPE_MASK0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ENTRY_TYPE_MASK1f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, D_ID0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, D_ID0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, D_ID1f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, D_ID_MASK0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, D_ID_MASK1f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, DST_DISCARD0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, DST_DISCARD0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, DST_DISCARD1f, ipv4a);
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, CLASS_ID0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, CLASS_ID0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, CLASS_ID1f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT */
    if (copy_hit) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, HIT0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, HIT1f, ipv4a);
    }

    return(SOC_E_NONE);
}

/* src and dst can be same */
int
soc_fb_lpm_ip4entry1_to_0(int u, void *src, void *dst, int copy_hit)
{
    uint32      ipv4a;

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VALID1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VALID0f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, MODE1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, MODE0f, ipv4a);
#if defined(BCM_TRX_SUPPORT) || defined(BCM_RAVEN_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, MODE_MASK0f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, MODE_MASK1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, MODE_MASK0f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT || BCM_RAVEN_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, GLOBAL_ROUTE1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, GLOBAL_ROUTE1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, GLOBAL_ROUTE0f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT */

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, IP_ADDR1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, IP_ADDR0f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, IP_ADDR_MASK1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, IP_ADDR_MASK0f, ipv4a);

	if (!SOC_IS_HURRICANE(u)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP0f, ipv4a);
	}

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, ECMP_COUNT1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP_COUNT1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP_COUNT0f, ipv4a);

        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ECMP_PTR1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ECMP_PTR0f, ipv4a);
    } else {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, NEXT_HOP_INDEX1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, NEXT_HOP_INDEX0f, ipv4a);
    }

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, PRI1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, PRI0f, ipv4a);

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, RPE1f);
    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, RPE0f, ipv4a);

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, DEFAULTROUTE0f) &&
        SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, DEFAULTROUTE1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src,DEFAULTROUTE1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, DEFAULTROUTE0f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, VRF_ID_1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VRF_ID_1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VRF_ID_0f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, VRF_ID_MASK1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, VRF_ID_MASK0f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, ENTRY_TYPE1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ENTRY_TYPE1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ENTRY_TYPE0f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, ENTRY_TYPE_MASK1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, ENTRY_TYPE_MASK0f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, D_ID1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, D_ID1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, D_ID0f, ipv4a);
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, D_ID_MASK1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, D_ID_MASK0f, ipv4a);
    }

    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, DST_DISCARD1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, DST_DISCARD1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, DST_DISCARD0f, ipv4a);
    }

#if defined(BCM_TRX_SUPPORT)
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, CLASS_ID1f)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, CLASS_ID1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, CLASS_ID0f, ipv4a);
    }
#endif /* BCM_TRX_SUPPORT */
    if (copy_hit) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src, HIT1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, HIT0f, ipv4a);
    }

    return(SOC_E_NONE);
}

static
int _lpm_ip4entry_swap(int u, void *e)
{
    return(SOC_E_NONE);
}

void
soc_fb_lpm_state_dump(int u)
{
    int i;
    int max_pfx_len;
    max_pfx_len = MAX_PFX_INDEX;

    if (!bsl_check(bslLayerSoc, bslSourceLpm, bslSeverityVerbose, u)) {
        return;
    }
    for(i = max_pfx_len; i >= 0 ; i--) {
        if ((i != MAX_PFX_INDEX) && (SOC_LPM_STATE_START(u, i) == -1)) {
            continue;
        }
        LOG_VERBOSE(BSL_LS_SOC_LPM,
                    (BSL_META_U(u,
                                "PFX = %d P = %d N = %d START = %d "
                                "END = %d VENT = %d FENT = %d\n"),
                     i,
                     SOC_LPM_STATE_PREV(u, i),
                     SOC_LPM_STATE_NEXT(u, i),
                     SOC_LPM_STATE_START(u, i),
                     SOC_LPM_STATE_END(u, i),
                     SOC_LPM_STATE_VENT(u, i),
                     SOC_LPM_STATE_FENT(u, i)));
    }
    COMPILER_REFERENCE(_lpm_ip4entry_swap);
}

/*
 *      Replicate entry to the second half of the tcam if URPF check is ON. 
 */
static 
int _lpm_fb_urpf_entry_replicate(int u, int index, uint32 *e)
{
    int src_tcam_offset;  /* Defip memory size/2 urpf source lookup offset */
    int ipv6;             /* IPv6 entry.                                   */
    uint32 mask0;         /* Mask 0 field value.                           */
    uint32 mask1;         /* Mask 1 field value.                           */
    int def_gw_flag;      /* Entry is default gateway.                     */ 

    if(!SOC_URPF_STATUS_GET(u)) {
        return (SOC_E_NONE);
    }

    if (soc_feature(u, soc_feature_l3_defip_hole)) {
         src_tcam_offset = (soc_mem_index_count(u, L3_DEFIPm) >> 1);
    } else if (SOC_IS_APOLLO(u)) {
         src_tcam_offset = (soc_mem_index_count(u, L3_DEFIPm) >> 1) + 0x0400;
    } 
    else {
         src_tcam_offset = (soc_mem_index_count(u, L3_DEFIPm) >> 1);
    }

    ipv6 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, MODE0f);

    #if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANA(u)) {
        if (soc_property_get(u, spn_IPV6_LPM_128B_ENABLE, 0) == 1) {
            /* When tcams are paired ipv4 lpm and ipv6 64b lpm
             * starts at index of tcam 4. We have to add size of the
             * tcam used for DIP lookup to get the urpf index */
            src_tcam_offset = kt_lpm_ipv6_info[u]->ipv6_64b.depth;
        }
    }
    #endif

    /* Reset destination discard bit. */
    if (SOC_MEM_OPT_FIELD_VALID(u, L3_DEFIPm, DST_DISCARD0f)) {
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, DST_DISCARD0f, 0);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, DST_DISCARD1f, 0);
    }

    /* Set/Reset default gateway flag based on ip mask value. */
    mask0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, IP_ADDR_MASK0f);
    mask1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, IP_ADDR_MASK1f);

    if (!ipv6) {
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f)) {
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, RPE0f, (!mask0) ? 1 : 0);
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e,
                DEFAULTROUTE0f, (!mask0) ? 1 : 0);
        }
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f)) {
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, RPE1f, (!mask1) ? 1 : 0);
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e,
                DEFAULTROUTE1f, (!mask1) ? 1 : 0);
        }
    } else {
        def_gw_flag = ((!mask0) &&  (!mask1)) ? 1 : 0;
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, RPE0f, def_gw_flag);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, RPE1f, def_gw_flag);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, DEFAULTROUTE0f, def_gw_flag);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, DEFAULTROUTE1f, def_gw_flag);
    }

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using only 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF (For ex: Katana2).
     */
    if (soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        return WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, index, e);
    }

    /* Write entry to the second half of the tcam. */
    return WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, (index + src_tcam_offset), e);
}

/*
 *      Create a slot for the new entry rippling the entries if required
 */
static
int _lpm_fb_entry_shift(int u, int from_ent, int to_ent)
{
    uint32      e[SOC_MAX_MEM_FIELD_WORDS];

#ifdef FB_LPM_TABLE_CACHED
    SOC_IF_ERROR_RETURN(soc_mem_cache_invalidate(u, L3_DEFIPm,
                                       MEM_BLOCK_ANY, from_ent));
#endif /* FB_LPM_TABLE_CACHED */

    SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e));
    LPM_HASH_INSERT(u, e, to_ent);
    SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, to_ent, e));
    SOC_IF_ERROR_RETURN(_lpm_fb_urpf_entry_replicate(u, to_ent, e));
    LPM_AVL_INSERT(u, e, to_ent);
    return (SOC_E_NONE);
}


/*
 *      Shift prefix entries 1 entry UP, while preserving  
 *      last half empty IPv4 entry if any.
 */
static
int _lpm_fb_shift_pfx_up(int u, int pfx, int ipv6)
{
    uint32      e[SOC_MAX_MEM_FIELD_WORDS];
    int         from_ent;
    int         to_ent;
    uint32      v0, v1;

    to_ent = SOC_LPM_STATE_END(u, pfx) + 1;

    if (!ipv6) {
        from_ent = SOC_LPM_STATE_END(u, pfx);
#ifdef FB_LPM_TABLE_CACHED
        SOC_IF_ERROR_RETURN(soc_mem_cache_invalidate(u, L3_DEFIPm,
                                                     MEM_BLOCK_ANY, from_ent));
#endif /* FB_LPM_TABLE_CACHED */
        SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e));
        v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
        v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);

        if ((v0 == 0) || (v1 == 0)) {
            /* Last entry is half full -> keep it last. */
            LPM_HASH_INSERT(u, e, to_ent);
            SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, to_ent, e));
            SOC_IF_ERROR_RETURN(_lpm_fb_urpf_entry_replicate(u, to_ent, e));
            LPM_AVL_INSERT(u, e, to_ent);
            to_ent--;
        }
    }

    from_ent = SOC_LPM_STATE_START(u, pfx);
    if(from_ent != to_ent) {
        SOC_IF_ERROR_RETURN(_lpm_fb_entry_shift(u, from_ent, to_ent));
    } 
    SOC_LPM_STATE_START(u, pfx) += 1;
    SOC_LPM_STATE_END(u, pfx) += 1;
    return (SOC_E_NONE);
}

/*
 *      Shift prefix entries 1 entry DOWN, while preserving  
 *      last half empty IPv4 entry if any.
 */
static
int _lpm_fb_shift_pfx_down(int u, int pfx, int ipv6)
{
    uint32      e[SOC_MAX_MEM_FIELD_WORDS];
    int         from_ent;
    int         to_ent;
    int         prev_ent;
    uint32      v0, v1;

    to_ent = SOC_LPM_STATE_START(u, pfx) - 1;

    /* Don't move empty prefix . */ 
    if (SOC_LPM_STATE_VENT(u, pfx) == 0) {
        SOC_LPM_STATE_START(u, pfx) = to_ent;
        SOC_LPM_STATE_END(u, pfx) = to_ent - 1;
        return (SOC_E_NONE);
    }

    if ((!ipv6) && (SOC_LPM_STATE_END(u, pfx) != SOC_LPM_STATE_START(u, pfx))) {

        from_ent = SOC_LPM_STATE_END(u, pfx);

#ifdef FB_LPM_TABLE_CACHED
        SOC_IF_ERROR_RETURN(soc_mem_cache_invalidate(u, L3_DEFIPm,
                                                     MEM_BLOCK_ANY, from_ent));
#endif /* FB_LPM_TABLE_CACHED */
        SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e));
        v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
        v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);

        if ((v0 == 0) || (v1 == 0)) {
            /* Last entry is half full -> keep it last. */
            /* Shift entry before last to start - 1 position. */
            prev_ent = from_ent - 1;
            SOC_IF_ERROR_RETURN(_lpm_fb_entry_shift(u, prev_ent, to_ent));

            LPM_HASH_INSERT(u, e, prev_ent);
            SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, prev_ent , e));
            SOC_IF_ERROR_RETURN(_lpm_fb_urpf_entry_replicate(u, prev_ent, e));
            LPM_AVL_INSERT(u, e, prev_ent);
        } else {
            /* Last entry is full -> just shift it to start - 1  position. */
            LPM_HASH_INSERT(u, e, to_ent);
            SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, to_ent, e));
            SOC_IF_ERROR_RETURN(_lpm_fb_urpf_entry_replicate(u, to_ent, e));
            LPM_AVL_INSERT(u, e, to_ent);
        }

    } else  {

        from_ent = SOC_LPM_STATE_END(u, pfx);
        SOC_IF_ERROR_RETURN(_lpm_fb_entry_shift(u, from_ent, to_ent));

    }

    SOC_LPM_STATE_START(u, pfx) -= 1;
    SOC_LPM_STATE_END(u, pfx) -= 1;

    return (SOC_E_NONE);
}

/*
 * @name  - _lpm_entry_in_paired_tcam
 * @param - unit - unit number
 * @param - index - logical index into L3_DEFIP
 *
 * @purpose - Check if the physical index corresponding to the logical index
 *            is in paired tcam 
 * @returns - TRUE/FALSE
 */
STATIC int
_lpm_entry_in_paired_tcam(int unit, int index) 
{
    int num_tcams = 0;
    int tcam_size = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit);
    int new_index;
    int num_ipv6_128b_entries = SOC_L3_DEFIP_INDEX_REMAP_GET(unit);

    if (index >= soc_mem_index_count(unit, L3_DEFIPm)) {
        return FALSE; 
    }

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using only 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF (For ex: Katana2).
     */
    if (SOC_URPF_STATUS_GET(unit) &&
        !soc_feature(unit, soc_feature_l3_defip_advanced_lookup)) {
        new_index = soc_l3_defip_urpf_index_map(unit, 0, index); 
        num_ipv6_128b_entries /= 2;
    } else {
        new_index = soc_l3_defip_index_map(unit, 0, index); 
    } 

    num_tcams = (num_ipv6_128b_entries /  tcam_size) +
                ((num_ipv6_128b_entries % tcam_size) ? 1 : 0);

    if (new_index < num_tcams * tcam_size * 2) {
        return TRUE;
    }

    return FALSE;
}


/*
 *      Create a slot for the new entry rippling the entries if required
 */
static
int _lpm_free_slot_create(int u, int pfx, int ipv6, void *e, int *free_slot)
{
    int         prev_pfx;
    int         next_pfx;
    int         free_pfx;
    int         curr_pfx;
    int         from_ent;
    uint32      v0, v1;
    int         rv;
    int         v6_index_in_paired_tcam = 0;
    int         start_index = 0;
    int         to_ent;
    int         unpaired_start = -1;
    int         occupied_index = -1;
    uint32      entry[SOC_MAX_MEM_FIELD_WORDS];
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int num_ipv6_128b_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);

    if (SOC_LPM_STATE_VENT(u, pfx) == 0) {
        /*
         * Find the  prefix position. Only prefix with valid
         * entries are in the list.
         * next -> high to low prefix. low to high index
         * prev -> low to high prefix. high to low index
         * Unused prefix length MAX_PFX_INDEX is the head of the
         * list and is node corresponding to this is always
         * present.
         */
        curr_pfx = MAX_PFX_INDEX;
        while (SOC_LPM_STATE_NEXT(u, curr_pfx) > pfx) {
            curr_pfx = SOC_LPM_STATE_NEXT(u, curr_pfx);
        }
        /* Insert the new prefix */
        next_pfx = SOC_LPM_STATE_NEXT(u, curr_pfx);
        if (next_pfx != -1) {
            SOC_LPM_STATE_PREV(u, next_pfx) = pfx;
        }
        SOC_LPM_STATE_NEXT(u, pfx) = SOC_LPM_STATE_NEXT(u, curr_pfx);
        SOC_LPM_STATE_PREV(u, pfx) = curr_pfx;
        SOC_LPM_STATE_NEXT(u, curr_pfx) = pfx;

        SOC_LPM_STATE_START(u, pfx) =  SOC_LPM_STATE_END(u, curr_pfx) +
                                   (SOC_LPM_STATE_FENT(u, curr_pfx) / 2) + 1;
        SOC_LPM_STATE_END(u, pfx) = SOC_LPM_STATE_START(u, pfx) - 1;
        SOC_LPM_STATE_VENT(u, pfx) = 0;

        if (soc_feature(u, soc_feature_l3_shared_defip_table)) {
            /* if v6, dont allow the entry to be added in paired tcam */
            start_index = SOC_LPM_STATE_START(u, pfx);
            if ((ipv6 != 0) && _lpm_entry_in_paired_tcam(u, start_index)) {
                /* entry location is paired tcam. 
                 * so find a new slot for this prefix. 
                 */
                v6_index_in_paired_tcam = 1;
                SOC_LPM_STATE_START(u, pfx) = -1;
                SOC_LPM_STATE_END(u, pfx) = -1;
                SOC_LPM_STATE_FENT(u, pfx) = 0;
                if (SOC_LPM_STATE_FENT(u, curr_pfx)) {
                    /* fent = 0, index in paired tcam for V6, if prev has
                     * fent, then figureout the start of unpaired tcam
                     * and try to get that index
                     */
                    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
                     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
                     * L3_DEFIP table is not divided into 2 to support URPF.
                     */
                    if (SOC_URPF_STATUS_GET(u) &&
                        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
                        num_ipv6_128b_entries >>= 1;
                    }
                    unpaired_start = (tcam_depth -
                                     (num_ipv6_128b_entries % tcam_depth)) << 1;
                    /* Need not check if num_ipv6_128b_entries % tcam_depth == 0
                     * as _lpm_entry_in_paired_tcam will be true only if it is 
                     * not zero. 
                     */
                    if (unpaired_start <= soc_mem_index_max(u, L3_DEFIPm)) {
                        occupied_index = SOC_LPM_STATE_END(u, curr_pfx) +
                                         SOC_LPM_STATE_FENT(u, curr_pfx);
                        if ((SOC_LPM_STATE_END(u, curr_pfx) < unpaired_start) &&
                            (occupied_index - unpaired_start >= 0)) {
                            SOC_LPM_STATE_FENT(u, pfx) = occupied_index -
                                                    unpaired_start + 1;
                            SOC_LPM_STATE_FENT(u, curr_pfx) -=
                                SOC_LPM_STATE_FENT(u, pfx);
                            SOC_LPM_STATE_START(u, pfx) = unpaired_start;
                            SOC_LPM_STATE_END(u, pfx) = unpaired_start - 1;
                            v6_index_in_paired_tcam = 0;
                        }
                    }
                }
            } else {
                SOC_LPM_STATE_FENT(u, pfx) =  (SOC_LPM_STATE_FENT(u, curr_pfx) + 1) / 2;
                SOC_LPM_STATE_FENT(u, curr_pfx) -= SOC_LPM_STATE_FENT(u, pfx);
            }
        } else {
            SOC_LPM_STATE_FENT(u, pfx) =  (SOC_LPM_STATE_FENT(u, curr_pfx) + 1) / 2;
            SOC_LPM_STATE_FENT(u, curr_pfx) -= SOC_LPM_STATE_FENT(u, pfx);
        }
    } else if (!ipv6) {
        /* For IPv4 Check if alternate entry is free */
        from_ent = SOC_LPM_STATE_START(u, pfx);
#ifdef FB_LPM_TABLE_CACHED
        if ((rv = soc_mem_cache_invalidate(u, L3_DEFIPm,
                                           MEM_BLOCK_ANY, from_ent)) < 0) {
            return rv;
        }
#endif /* FB_LPM_TABLE_CACHED */
        if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e)) < 0) {
            return rv;
        }
        v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
        v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);

        if ((v0 == 0) || (v1 == 0)) {
            *free_slot = (from_ent << 1) + ((v1 == 0) ? 1 : 0);
            return(SOC_E_NONE);
        }

        from_ent = SOC_LPM_STATE_END(u, pfx);
#ifdef FB_LPM_TABLE_CACHED
        if ((rv = soc_mem_cache_invalidate(u, L3_DEFIPm,
                                           MEM_BLOCK_ANY, from_ent)) < 0) {
            return rv;
        }
#endif /* FB_LPM_TABLE_CACHED */
        if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e)) < 0) {
            return rv;
        }
        v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
        v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);

        if ((v0 == 0) || (v1 == 0)) {
            *free_slot = (from_ent << 1) + ((v1 == 0) ? 1 : 0);
            return(SOC_E_NONE);
        }
    }

    free_pfx = pfx;
    while(SOC_LPM_STATE_FENT(u, free_pfx) == 0) {
        free_pfx = SOC_LPM_STATE_NEXT(u, free_pfx);
        if (free_pfx == -1) {
            /* No free entries on this side try the other side */
            free_pfx = pfx;
            break;
        }
    }

    while (v6_index_in_paired_tcam == 0 && 
           SOC_LPM_STATE_FENT(u, free_pfx) == 0) {
        free_pfx = SOC_LPM_STATE_PREV(u, free_pfx);
        if (free_pfx == -1) {
            if (SOC_LPM_STATE_VENT(u, pfx) == 0) {
                /* failed to allocate entries for a newly allocated prefix.*/
                prev_pfx = SOC_LPM_STATE_PREV(u, pfx);
                next_pfx = SOC_LPM_STATE_NEXT(u, pfx);
                if (-1 != prev_pfx) {
                    SOC_LPM_STATE_NEXT(u, prev_pfx) = next_pfx;
                }
                if (-1 != next_pfx) {
                    SOC_LPM_STATE_PREV(u, next_pfx) = prev_pfx;
                }
            }
            return(SOC_E_FULL);
        }
    }

    /*
     * Ripple entries to create free space
     */
    while (free_pfx > pfx) {
        next_pfx = SOC_LPM_STATE_NEXT(u, free_pfx); 
        if (soc_feature(u, soc_feature_l3_shared_defip_table)) {
            /*
             * If the next_pfx is of type 64b V6 and if the free_pfx is in 
             * paired_tcam space, we should not move the entry.
             * We will move the entry from the next prefix. 
             * The assumption here is order of V4 and V6 entries are independent
             */
            if (SOC_LPM_PFX_IS_V6_64(u, next_pfx) == TRUE) {
                to_ent = SOC_LPM_STATE_START(u, next_pfx) - 1;
                if (_lpm_entry_in_paired_tcam(u, to_ent)) {

                    /* encountered V6 prefix which cannot be moved to 
                     * paired tcam. When we move a pfx, the from entry will
                     * not be removed assuming that the location will be used
                     * by next prefix. In this case we did not write the V6 
                     * entry. so we need to delete the from entry. 
                     * Ideally we should delete hash and set valid bits to 0 
                     * in _lpm_fb_entry_shift for the from_entry. But in most 
                     * of the cases, the from entry will be used by next prefix
                     * so we can reduce one read/write of L3_DEFIP if we dont 
                     * delete the entry in _lpm_fb_entry_shift
                     * free_pfx contains the previous pfx from which entry was 
                     * moved 
                     */
                     
                    from_ent = SOC_LPM_STATE_END(u, free_pfx) + 1;
                    SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, 
                                                       from_ent, entry));
                    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, entry, VALID0f, 0);
                    SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, entry, VALID1f, 0);
                    SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, 
                                                        from_ent, entry));
                    SOC_IF_ERROR_RETURN(_lpm_fb_urpf_entry_replicate(u, 
                                        from_ent, entry));
                    if (SOC_LPM_STATE_VENT(u, pfx) == 0) {
                        /* failed to allocate entries for a newly allocated prefix.*/
                        prev_pfx = SOC_LPM_STATE_PREV(u, pfx);
                        next_pfx = SOC_LPM_STATE_NEXT(u, pfx);
                        if (-1 != prev_pfx) {
                            SOC_LPM_STATE_NEXT(u, prev_pfx) = next_pfx;
                        }
                        if (-1 != next_pfx) {
                            SOC_LPM_STATE_PREV(u, next_pfx) = prev_pfx;
                        }
                    }
                    return (SOC_E_FULL);
                }
            }
        }
        SOC_IF_ERROR_RETURN(_lpm_fb_shift_pfx_down(u, next_pfx, ipv6));
        SOC_LPM_STATE_FENT(u, free_pfx) -= 1;
        SOC_LPM_STATE_FENT(u, next_pfx) += 1;
        free_pfx = next_pfx;
    }

    while (free_pfx < pfx) {
        SOC_IF_ERROR_RETURN(_lpm_fb_shift_pfx_up(u, free_pfx, ipv6));
        SOC_LPM_STATE_FENT(u, free_pfx) -= 1;
        prev_pfx = SOC_LPM_STATE_PREV(u, free_pfx); 
        SOC_LPM_STATE_FENT(u, prev_pfx) += 1;
        free_pfx = prev_pfx;
    }

    if (soc_feature(u, soc_feature_l3_shared_defip_table)) { 
        if (v6_index_in_paired_tcam != 0 && free_pfx == pfx) {
            /* could not find a entry in  unpaired tcam */
            if (SOC_LPM_STATE_VENT(u, pfx) == 0) {
                prev_pfx = SOC_LPM_STATE_PREV(u, pfx);
                next_pfx = SOC_LPM_STATE_NEXT(u, pfx);
                if (SOC_LPM_STATE_FENT(u, pfx)) {
                    SOC_LPM_STATE_START(u, pfx) =
                        SOC_LPM_STATE_START(u, next_pfx) - 1;
                    SOC_LPM_STATE_END(u, pfx) = SOC_LPM_STATE_START(u, pfx) - 1;
                } else {
                    /* failed to allocate entries for a newly allocated prefix.*/
                    if (-1 != prev_pfx) {
                        SOC_LPM_STATE_NEXT(u, prev_pfx) = next_pfx;
                    }
                    if (-1 != next_pfx) {
                        SOC_LPM_STATE_PREV(u, next_pfx) = prev_pfx;
                    }
                    return SOC_E_FULL;
                }
            }
        }
    }

    SOC_LPM_STATE_VENT(u, pfx) += 1;
    SOC_LPM_STATE_FENT(u, pfx) -= 1;
    SOC_LPM_STATE_END(u, pfx) += 1;
    *free_slot = SOC_LPM_STATE_END(u, pfx) <<  ((ipv6) ? 0 : 1);
    sal_memcpy(e, soc_mem_entry_null(u, L3_DEFIPm),
               soc_mem_entry_words(u,L3_DEFIPm) * 4);

    return(SOC_E_NONE);
}

/*
 *      Delete a slot and adjust entry pointers if required.
 *      e - has the contents of entry at slot(useful for IPV4 only)
 */
static
int _lpm_free_slot_delete(int u, int pfx, int ipv6, void *e, int slot)
{
    int         prev_pfx;
    int         next_pfx;
    int         from_ent;
    int         to_ent;
    uint32      ef[SOC_MAX_MEM_FIELD_WORDS];
    void        *et;
    int         rv;

    from_ent = SOC_LPM_STATE_END(u, pfx);
    to_ent = slot;
    if (!ipv6) { /* IPV4 */
        to_ent >>= 1;
#ifdef FB_LPM_TABLE_CACHED
        if ((rv = soc_mem_cache_invalidate(u, L3_DEFIPm,
                                           MEM_BLOCK_ANY, from_ent)) < 0) {
            return rv;
        }
#endif /* FB_LPM_TABLE_CACHED */
        if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, ef)) < 0) {
            return rv;
        }
        et =  (to_ent == from_ent) ? ef : e;
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, ef, VALID1f)) {
            if (slot & 1) {
                rv = soc_fb_lpm_ip4entry1_to_1(u, ef, et, PRESERVE_HIT);
            } else {
                rv = soc_fb_lpm_ip4entry1_to_0(u, ef, et, PRESERVE_HIT);
            }
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, ef, VALID1f, 0);
            /* first half of full entry is deleted, generating half entry.*/
            if (soc_feature(u, soc_feature_l3_shared_defip_table)) {
                SOC_LPM_COUNT_INC(SOC_LPM_V4_HALF_ENTRY_COUNT(u));
            } 
        } else {
            if (slot & 1) {
                rv = soc_fb_lpm_ip4entry0_to_1(u, ef, et, PRESERVE_HIT);
            } else {
                rv = soc_fb_lpm_ip4entry0_to_0(u, ef, et, PRESERVE_HIT);
            }
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, ef, VALID0f, 0);
            /* Remaining half entry got deleted. Decrement half entry count.*/ 
            if (soc_feature(u, soc_feature_l3_shared_defip_table)) {
                SOC_LPM_COUNT_DEC(SOC_LPM_V4_HALF_ENTRY_COUNT(u));
            } 
            SOC_LPM_STATE_VENT(u, pfx) -= 1;
            SOC_LPM_STATE_FENT(u, pfx) += 1;
            SOC_LPM_STATE_END(u, pfx) -= 1;
        }

        if (to_ent != from_ent) {
            LPM_HASH_INSERT(u, et, to_ent);
            if ((rv = WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, to_ent, et)) < 0) {
                return rv;
            }
            if ((rv = _lpm_fb_urpf_entry_replicate(u, to_ent, et)) < 0) {
                return rv;
            }
            LPM_AVL_INSERT(u, et, to_ent);
        }
        LPM_HASH_INSERT(u, ef, from_ent);
        if ((rv = WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, ef)) < 0) {
            return rv;
        }
        if ((rv = _lpm_fb_urpf_entry_replicate(u, from_ent, ef)) < 0) {
            return rv;
        }
        LPM_AVL_INSERT(u, ef, from_ent);
    } else { /* IPV6 */
        SOC_LPM_STATE_VENT(u, pfx) -= 1;
        SOC_LPM_STATE_FENT(u, pfx) += 1;
        SOC_LPM_STATE_END(u, pfx) -= 1;
        if (to_ent != from_ent) {
#ifdef FB_LPM_TABLE_CACHED
            if ((rv = soc_mem_cache_invalidate(u, L3_DEFIPm,
                                               MEM_BLOCK_ANY, from_ent)) < 0) {
                return rv;
            }
#endif /* FB_LPM_TABLE_CACHED */
            if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, ef)) < 0) {
                return rv;
            }
            LPM_HASH_INSERT(u, ef, to_ent);
            if ((rv = WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, to_ent, ef)) < 0) {
                return rv;
            }
            if ((rv = _lpm_fb_urpf_entry_replicate(u, to_ent, ef)) < 0) {
                return rv;
            }
            LPM_AVL_INSERT(u, ef, to_ent);
        }

        sal_memcpy(ef, soc_mem_entry_null(u, L3_DEFIPm),
                   soc_mem_entry_words(u,L3_DEFIPm) * 4);
        LPM_HASH_INSERT(u, ef, from_ent);
        if ((rv = WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, ef)) < 0) {
            return rv;
        }
        if ((rv = _lpm_fb_urpf_entry_replicate(u, from_ent, ef)) < 0) {
            return rv;
        }
        LPM_AVL_INSERT(u, ef, from_ent);
    }

    if (SOC_LPM_STATE_VENT(u, pfx) == 0) {
        /* remove from the list */
        prev_pfx = SOC_LPM_STATE_PREV(u, pfx); /* Always present */
        assert(prev_pfx != -1);
        next_pfx = SOC_LPM_STATE_NEXT(u, pfx);
        SOC_LPM_STATE_NEXT(u, prev_pfx) = next_pfx;
        SOC_LPM_STATE_FENT(u, prev_pfx) += SOC_LPM_STATE_FENT(u, pfx);
        SOC_LPM_STATE_FENT(u, pfx) = 0;
        if (next_pfx != -1) {
            SOC_LPM_STATE_PREV(u, next_pfx) = prev_pfx;
        }
        SOC_LPM_STATE_NEXT(u, pfx) = -1;
        SOC_LPM_STATE_PREV(u, pfx) = -1;
        SOC_LPM_STATE_START(u, pfx) = -1;
        SOC_LPM_STATE_END(u, pfx) = -1;
    }

    return(rv);
}


/*
 * Function:
 *      _soc_fb_lpm_vrf_get
 * Purpose:
 *      Service routine used to translate hw specific vrf id to API format.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      lpm_entry - (IN)Route info buffer from hw.
 *      vrf_id    - (OUT)Virtual router id.
 * Returns:
 *      BCM_E_XXX
 */
int
soc_fb_lpm_vrf_get(int unit, void *lpm_entry, int *vrf)
{
    int vrf_id;

    /* Get Virtual Router id if supported. */
    if (SOC_MEM_OPT_FIELD_VALID(unit, L3_DEFIPm, VRF_ID_MASK0f)){
        vrf_id = SOC_MEM_OPT_F32_GET(unit, L3_DEFIPm, lpm_entry, VRF_ID_0f);

        /* Special vrf's handling. */
        if (SOC_MEM_OPT_F32_GET(unit, L3_DEFIPm, lpm_entry, VRF_ID_MASK0f)) {
            *vrf = vrf_id;    
        } else if (SOC_VRF_MAX(unit) == vrf_id) {
            *vrf = SOC_L3_VRF_GLOBAL;
        } else {
            *vrf = SOC_L3_VRF_OVERRIDE;    
            if (SOC_MEM_OPT_FIELD_VALID(unit, L3_DEFIPm, GLOBAL_ROUTE0f)) {
                if (SOC_MEM_OPT_F32_GET(unit, L3_DEFIPm, lpm_entry, GLOBAL_ROUTE0f)) {
                    *vrf = SOC_L3_VRF_GLOBAL; 
                }
            }
        }
    } else {
        /* No vrf support on this device. */
        *vrf = SOC_L3_VRF_DEFAULT;
    }
    return (SOC_E_NONE);
}

/*
 * _soc_fb_lpm_prefix_length_get (Extract vrf weighted  prefix lenght from the 
 * hw entry based on ip, mask & vrf)
 */
static int
_soc_fb_lpm_prefix_length_get(int u, void *entry, int *pfx_len) 
{
    int         pfx;
    int         rv;
    int         ipv6;
    uint32      ipv4a;
    int         vrf_id;

    ipv6 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry, MODE0f);

    if (ipv6) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry, IP_ADDR_MASK0f);
        if ((rv = _ipmask2pfx(ipv4a, &pfx)) < 0) {
            return(rv);
        }
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry, IP_ADDR_MASK1f);
        if (pfx) {
            if (ipv4a != 0xffffffff)  {
                return(SOC_E_PARAM);
            }
            pfx += 32;
        } else {
            if ((rv = _ipmask2pfx(ipv4a, &pfx)) < 0) {
                return(rv);
            }
        }
        if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
            /*
             * if using shared defip table, 64B V6 addresses have lower
             * prefix length than V4 address. This is because, V4 entries 
             * can be added to the paired tcam but not 64B V6 addresses.
             * So V4 entries need to occupy the lower addresses to be contigous 
             * in software.
             */     
         
            pfx += IPV6_PFX_ZERO; /* First 33 are for IPV4 addresses */
        }
    } else {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry, IP_ADDR_MASK0f);
        if ((rv = _ipmask2pfx(ipv4a, &pfx)) < 0) {
            return(rv);
        }

        if (soc_feature(u, soc_feature_l3_shared_defip_table)) {
            /*
             * V4 prefixes should be at lower indices that 64B prefixes
             * as V4 entries can be added to the paired tcam as well.
             * The max V4 prefix length is 295.
             * Max 64B prefix length is 262.
             */
            pfx += IPV4_PFX_ZERO;
        } 
    }

    SOC_IF_ERROR_RETURN(soc_fb_lpm_vrf_get(u, entry, &vrf_id));

    switch (vrf_id) { 
      case SOC_L3_VRF_GLOBAL:
          *pfx_len = pfx;
          break;
      case SOC_L3_VRF_OVERRIDE:
          *pfx_len = pfx + 2 * (MAX_PFX_ENTRIES / 6);
          break;
      default:   
          *pfx_len = pfx +  (MAX_PFX_ENTRIES / 6);
          break;
    }
    return (SOC_E_NONE);
}


/*
 * _soc_fb_lpm_match (Exact match for the key. Will match both IP address
 * and mask)
 */
static
int
_soc_fb_lpm_match(int u,
               void *key_data,
               void *e,         /* return entry data if found */
               int *index_ptr,  /* return key location */
               int *pfx_len,    /* Key prefix length. vrf + 32 + prefix len for IPV6*/
               int *ipv6)       /* Entry is ipv6. */
{
    int         rv;
    int         v6;
    int         key_index;
    int         pfx = 0;

    v6 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, MODE0f);

    if (v6) {
        if (!(v6 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, MODE1f))) {
            return (SOC_E_PARAM);
        }
    }
    *ipv6 = v6;

    /* Calculate vrf weighted prefix lengh. */
    _soc_fb_lpm_prefix_length_get(u, key_data, &pfx); 
    *pfx_len = pfx; 

#ifdef FB_LPM_HASH_SUPPORT
    if (LPM_HASH_LOOKUP(u, key_data, pfx, &key_index) == SOC_E_NONE) {
        *index_ptr = key_index;
        if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY,
                         (*ipv6) ? *index_ptr : (*index_ptr >> 1), e)) < 0) {
            return rv;
        }
#ifndef  FB_LPM_CHECKER_ENABLE
        return(SOC_E_NONE);
#endif
    } else {
#ifndef  FB_LPM_CHECKER_ENABLE
        return(SOC_E_NOT_FOUND);
#endif
    }
#endif /* FB_LPM_HASH_SUPPORT */

#if defined(FB_LPM_AVL_SUPPORT)
    if (LPM_AVL_LOOKUP(u, key_data, pfx, &key_index) == SOC_E_NONE) {
#ifdef  FB_LPM_CHECKER_ENABLE
        if (*index_ptr != key_index) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "\nsoc_fb_lpm_locate: HASH %d AVL %d\n"), *index_ptr, key_index));
        }
        assert(*index_ptr == key_index);
#endif
        *index_ptr = key_index;
        if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY,
                         (*ipv6) ? *index_ptr : (*index_ptr >> 1), e)) < 0) {
            return rv;
        }
        return(SOC_E_NONE);
    } else {
        return(SOC_E_NOT_FOUND);
    }
#endif /* FB_LPM_AVL_SUPPORT */
}

int soc_fb_lpm_tcam_pair_count_get(int u, int *tcam_pair_count)
{
    int num_ipv6_128b_entries;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);

    if (tcam_pair_count == NULL) {
        return SOC_E_PARAM;
    }

    num_ipv6_128b_entries =  SOC_L3_DEFIP_MAX_128B_ENTRIES(u);
    if (num_ipv6_128b_entries) {
        *tcam_pair_count = (num_ipv6_128b_entries / tcam_depth) +
                          ((num_ipv6_128b_entries % tcam_depth) ? 1 : 0);
    } else {
        *tcam_pair_count = 0;
    }
    
    return SOC_E_NONE;
}

STATIC int 
_soc_lpm_max_v4_route_get(int u, int paired, uint16 *entries)
{
    int is_reserved = 0;
    int paired_table_size = 0;
    int defip_table_size = 0;
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    if (!paired && !soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
         * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
         * L3_DEFIP table is not divided into 2 to support URPF.
         */
        if (SOC_URPF_STATUS_GET(u) &&
            !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
            *entries = (soc_mem_index_count(u, L3_DEFIPm));
        } else {
            *entries = (soc_mem_index_count(u, L3_DEFIPm) << 1);
        }
        return SOC_E_NONE;
    }

    if (paired && !soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        *entries = 0;
        return SOC_E_NONE;
    }

    SOC_IF_ERROR_RETURN(soc_fb_lpm_table_sizes_get(u, &paired_table_size, 
                                                   &defip_table_size));
    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        max_v6_entries >>= 1;
    }
    if (paired) {
        if (is_reserved) {
            *entries = paired_table_size - (max_v6_entries << 1);
        } else {
            *entries = paired_table_size;
        }
    } else {
        *entries = defip_table_size;
    }
    *entries <<= 1;
    return SOC_E_NONE;
}

STATIC int 
_soc_lpm_max_64bv6_route_get(int u, int paired, uint16 *entries)
{
    int is_reserved = 0;
    int paired_table_size = 0;
    int defip_table_size = 0;
    int tcam_pair_count = 0;
    int max_pair_count = 0;
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int max_tcams = SOC_L3_DEFIP_MAX_TCAMS_GET(u);

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    if (!paired) {
        /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
         * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
         * L3_DEFIP table is not divided into 2 to support URPF.
         */
        if (SOC_URPF_STATUS_GET(u) &&
            !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
            max_v6_entries >>= 1;
            tcam_pair_count = (max_v6_entries / tcam_depth) + 
                              (max_v6_entries % tcam_depth ? 1 : 0);
            max_pair_count = max_tcams >> 2;
            *entries = ((max_pair_count - tcam_pair_count) * tcam_depth);
        } else {
            tcam_pair_count = (max_v6_entries / tcam_depth) + 
                              (max_v6_entries % tcam_depth ? 1 : 0);
            max_pair_count = max_tcams >> 1;
            *entries = ((max_pair_count - tcam_pair_count) * tcam_depth);
        }
        *entries <<= 1;
        return SOC_E_NONE;
    }

    if (paired && soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        SOC_IF_ERROR_RETURN(soc_fb_lpm_table_sizes_get(u, &paired_table_size, 
                                                       &defip_table_size));
        /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
         * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
         * L3_DEFIP table is not divided into 2 to support URPF.
         */
        if (SOC_URPF_STATUS_GET(u) &&
            !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
            max_v6_entries >>= 1;
        }
        *entries = (paired_table_size >> 1);
        if (is_reserved) {
            *entries -= (max_v6_entries);
        }
        return SOC_E_NONE;
    }
    *entries = 0;
    return SOC_E_NONE;
}

STATIC int
_soc_lpm_max_128bv6_route_get(int u, uint16 *entries)
{
    int is_reserved = 0;
    int paired_table_size = 0;
    int defip_table_size = 0;
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    if (!soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
         * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
         * L3_DEFIP table is not divided into 2 to support URPF.
         */
        if (SOC_URPF_STATUS_GET(u) &&
            !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
            max_v6_entries >>= 1;
        }
        *entries = max_v6_entries;
        return SOC_E_NONE;
    }

    SOC_IF_ERROR_RETURN(soc_fb_lpm_table_sizes_get(u, &paired_table_size, 
                                                   &defip_table_size));
    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        max_v6_entries >>= 1;
    }
    if (is_reserved) {
        *entries = max_v6_entries;
    } else {
        *entries = paired_table_size >> 1;
    }
    return SOC_E_NONE;
}

int
soc_fb_lpm_stat_init(int u)
{
    if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
        return SOC_E_NONE;
    }

    if (SOC_LPM_STAT_INIT_CHECK(u)) {
        sal_free(soc_lpm_stat[u]);
        soc_lpm_stat[u] = NULL;
    }

    soc_lpm_stat[u] = sal_alloc(sizeof(soc_lpm_stat_t), "LPM STATS");
    if (soc_lpm_stat[u] == NULL) {
        return SOC_E_MEMORY;
    }

    SOC_LPM_V4_COUNT(u) = 0;
    SOC_LPM_64BV6_COUNT(u) = 0;
    SOC_LPM_128BV6_COUNT(u) = 0;
    SOC_LPM_V4_HALF_ENTRY_COUNT(u) = 0;
    SOC_IF_ERROR_RETURN(_soc_lpm_max_v4_route_get(u, 0,
                                           &SOC_LPM_MAX_V4_COUNT(u))); 
    SOC_IF_ERROR_RETURN(_soc_lpm_max_64bv6_route_get(u, 0,
                                           &SOC_LPM_MAX_64BV6_COUNT(u))); 
    if (!soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        SOC_IF_ERROR_RETURN(_soc_lpm_max_128bv6_route_get(u,
                                           &SOC_LPM_MAX_128BV6_COUNT(u)));
    } else {
        SOC_LPM_MAX_128BV6_COUNT(u) = 0;
    }
    return SOC_E_NONE;
}

int
soc_fb_lpm128_stat_init(int u)
{
    if (SOC_LPM128_STATE(u) == NULL) {
        return SOC_E_INTERNAL;
    }
    SOC_LPM128_STAT_V4_COUNT(u) = 0;
    SOC_LPM128_STAT_64BV6_COUNT(u) = 0;
    SOC_LPM128_STAT_128BV6_COUNT(u) = 0;
    SOC_LPM128_STAT_V4_HALF_ENTRY_COUNT(u) = 0;
    /* Now setup max values */
    SOC_IF_ERROR_RETURN(_soc_lpm_max_v4_route_get(u, 1,
                                       &SOC_LPM128_STAT_MAX_V4_COUNT(u)));
    SOC_IF_ERROR_RETURN(_soc_lpm_max_64bv6_route_get(u, 1,
                                      &SOC_LPM128_STAT_MAX_64BV6_COUNT(u)));
    SOC_IF_ERROR_RETURN(_soc_lpm_max_128bv6_route_get(u,
                                     &SOC_LPM128_STAT_MAX_128BV6_COUNT(u)));
    return SOC_E_NONE;
}

/*
 * Initialize the start/end tracking pointers for each prefix length
 */
int
soc_fb_lpm_init(int u)
{
    int max_pfx_len;
    int defip_table_size = 0;
    int pfx_state_size;
    int i;    
#if defined(BCM_KATANA_SUPPORT)
    uint32 l3_defip_tbl_size;
    int ipv6_lpm_128b_enable;
#endif
    int tcam_pair_count = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int start_index = 0;
    int hash_table_size = 0;


    if (! soc_feature(u, soc_feature_lpm_tcam)) {
        return(SOC_E_UNAVAIL);
    }

    max_pfx_len = MAX_PFX_ENTRIES;
    pfx_state_size = sizeof(soc_lpm_state_t) * (max_pfx_len);

    if (! SOC_LPM_INIT_CHECK(u)) {
        soc_lpm_field_cache_state[u] = 
                sal_alloc(sizeof(soc_lpm_field_cache_t), "lpm_field_state");
        if (NULL == soc_lpm_field_cache_state[u]) {
            return (SOC_E_MEMORY);
        }
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, CLASS_ID0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, CLASS_ID1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, DST_DISCARD0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, DST_DISCARD1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, ECMP0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, ECMP1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, ECMP_COUNT0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, ECMP_COUNT1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, ECMP_PTR0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, ECMP_PTR1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, GLOBAL_ROUTE0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, GLOBAL_ROUTE1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, HIT0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, HIT1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, IP_ADDR0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, IP_ADDR1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, IP_ADDR_MASK0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, IP_ADDR_MASK1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, MODE0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, MODE1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, MODE_MASK0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, MODE_MASK1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, NEXT_HOP_INDEX0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, NEXT_HOP_INDEX1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, PRI0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, PRI1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, RPE0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, RPE1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, VALID0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, VALID1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, VRF_ID_0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, VRF_ID_1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, VRF_ID_MASK0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, VRF_ID_MASK1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, ENTRY_TYPE0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, ENTRY_TYPE1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, ENTRY_TYPE_MASK0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, ENTRY_TYPE_MASK1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, D_ID0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, D_ID1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, D_ID_MASK0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, D_ID_MASK1f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, DEFAULTROUTE0f);
        SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, DEFAULTROUTE1f);
        SOC_LPM_STATE(u) = sal_alloc(pfx_state_size, "LPM prefix info");
        if (NULL == SOC_LPM_STATE(u)) {
            sal_free(soc_lpm_field_cache_state[u]);
            soc_lpm_field_cache_state[u] = NULL;
            return (SOC_E_MEMORY);
        }
    }

#ifdef FB_LPM_TABLE_CACHED
    SOC_IF_ERROR_RETURN (soc_mem_cache_set(u, L3_DEFIPm, MEM_BLOCK_ALL, TRUE));
#endif /* FB_LPM_TABLE_CACHED */
    SOC_LPM_LOCK(u);

    sal_memset(SOC_LPM_STATE(u), 0, pfx_state_size);
    for(i = 0; i < max_pfx_len; i++) {
        SOC_LPM_STATE_START(u, i) = -1;
        SOC_LPM_STATE_END(u, i) = -1;
        SOC_LPM_STATE_PREV(u, i) = -1;
        SOC_LPM_STATE_NEXT(u, i) = -1;
        SOC_LPM_STATE_VENT(u, i) = 0;
        SOC_LPM_STATE_FENT(u, i) = 0;
    }

    defip_table_size = soc_mem_index_count(u, L3_DEFIPm);
#ifdef BCM_KATANA2_SUPPORT
    if (SOC_IS_KATANA2(u)) {
          fb_lpm_hash_index_mask = 0x7FFF;
    } else {
          fb_lpm_hash_index_mask = 0x3FFF;
    }
#else
    fb_lpm_hash_index_mask = 0x3FFF;
#endif
        
    if (SOC_URPF_STATUS_GET(u)) {
        if (soc_feature(u, soc_feature_l3_defip_hole)) {
              defip_table_size = SOC_APOLLO_B0_L3_DEFIP_URPF_SIZE;
        } else if (SOC_IS_APOLLO(u)) {
            defip_table_size = SOC_APOLLO_L3_DEFIP_URPF_SIZE;
        } else {
            /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
             * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
             * L3_DEFIP table is not divided into 2 to support URPF.
             */
            if (!soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
                defip_table_size >>= 1;
            }
        }
    }

    if (soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        SOC_IF_ERROR_RETURN(soc_fb_lpm_tcam_pair_count_get(u, &tcam_pair_count));
        /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
         * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
         * L3_DEFIP table is not divided into 2 to support URPF.
         */
        if (SOC_URPF_STATUS_GET(u) &&
            !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
            switch(tcam_pair_count) {
                case 1:
                case 2:
                    defip_table_size -= (2 * tcam_depth);
                    start_index = 2 * tcam_depth;
                    break;
                case 3: 
                case 4:
                    defip_table_size = 0;
                    /* Set it to max */
                    start_index = 4 * tcam_depth;
                    break;
                default:
                    break;
                
            }
        } else {
            defip_table_size -= (tcam_pair_count * tcam_depth * 2);
            start_index = (2 * tcam_pair_count * tcam_depth);
        }
    } else {
        start_index = 0;
    }

    SOC_LPM_STATE_FENT(u, (MAX_PFX_INDEX)) = defip_table_size;
    SOC_LPM_STATE_START(u, MAX_PFX_INDEX) = start_index;
    SOC_LPM_STATE_END(u, (MAX_PFX_INDEX)) = start_index - 1;

#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANA(u)) {
        l3_defip_tbl_size = soc_mem_index_count(u, L3_DEFIPm);
        
        ipv6_lpm_128b_enable = soc_property_get(u, spn_IPV6_LPM_128B_ENABLE, 0);
    
        /* IPv6 128b and IPv6 64b entries are stored in mutually exclusive
         * entries in L3_DEFIP table. IPv6 128b are stored in L3_DEFIP_PAIR_128
         * table which is an alias of the L3_DEFIP TCAM pairs. While storing the
         * IPv6 64b entriesin L3_DEFIP table it should be taken care not to
         * overlap with the  entry locations reserved for IPv6 128b for a given
         * 128b pairing mode */

        /* De-allocate any existing kt_lpm_ipv6_info structures. */
        if (kt_lpm_ipv6_info[u] != NULL) {
            kt_lpm_ipv6_info[u] = NULL;
            sal_free(kt_lpm_ipv6_info[u]);
        }

        /* Allocate kt_lpm_ipv6_info structure. */
        kt_lpm_ipv6_info[u] = sal_alloc(sizeof(soc_kt_lpm_ipv6_info_t),
                                        "kt_lpm_ipv6_info");
        if (kt_lpm_ipv6_info[u] == NULL) {
            return (SOC_E_MEMORY);
        }

        /* Reset kt_lpm_ipv6_info structure. */
        sal_memset(kt_lpm_ipv6_info[u], 0, sizeof(soc_kt_lpm_ipv6_info_t));

        if (SOC_URPF_STATUS_GET(u)) {
            /* urpf check enabled */
            if (ipv6_lpm_128b_enable == 1) {
                /* C6 :
                 * L3_DEFIP_KEY_SEL=101100_011 ( 0x163 )
                 * TCAMs 0-1 paired for DIP IPv6 LPM 128b
                 * TCAMs 2-3 paired for SIP Ipv6 LPM 128b*/
                WRITE_L3_DEFIP_KEY_SELr(u, 0x163);
                kt_lpm_ipv6_info[u]->ipv6_128b.depth = L3_DEFIP_TCAM_SIZE;
                /* TCAM 4 is used for DIP IPv6 LPM 64b
                 * TCAM 5 is used for SIP IPv6 LPM 64b */
                kt_lpm_ipv6_info[u]->ipv6_64b.dip_start_offset =
                    L3_DEFIP_TCAM_SIZE * 4;
                kt_lpm_ipv6_info[u]->ipv6_64b.depth = L3_DEFIP_TCAM_SIZE;
                
            } else {
                /* C5 :
                 * L3_DEFIP_KEY_SEL=111000_000 ( 0x1c0 )
                 * No TCAM pairing - No IPv6 LPM 128b entries */
                WRITE_L3_DEFIP_KEY_SELr(u, 0x1c0);
                /* TCAMs 0-2 are shared for DIP IPv4 LPM and IPv6 LPM 64b
                 * TCAMs 3-5 are shared for SIP IPv4 LPM and IPv6 LPM 64b*/
                kt_lpm_ipv6_info[u]->ipv6_64b.depth = l3_defip_tbl_size / 2;
            }            
            kt_lpm_ipv6_info[u]->ipv6_128b.sip_start_offset =
                kt_lpm_ipv6_info[u]->ipv6_128b.dip_start_offset +
                kt_lpm_ipv6_info[u]->ipv6_128b.depth;
            kt_lpm_ipv6_info[u]->ipv6_64b.sip_start_offset =
                kt_lpm_ipv6_info[u]->ipv6_64b.dip_start_offset +
                kt_lpm_ipv6_info[u]->ipv6_64b.depth;
        } else {
            /* urpf check disabled */
            if (ipv6_lpm_128b_enable == 1) {
                /* C3:
                 * L3_DEFIP_KEY_SEL = 000000_011
                 * TCAMs 0-3 paired for DIP IPv6 LPM 128b */
                WRITE_L3_DEFIP_KEY_SELr(u, 0x3);
                kt_lpm_ipv6_info[u]->ipv6_128b.depth = L3_DEFIP_TCAM_SIZE * 2;
                /* TCAMs 4-5 is used for DIP IPv6 LPM 64b */
                kt_lpm_ipv6_info[u]->ipv6_64b.dip_start_offset =
                    L3_DEFIP_TCAM_SIZE * 4;
                kt_lpm_ipv6_info[u]->ipv6_64b.depth = L3_DEFIP_TCAM_SIZE * 2;
            } else {
                /* C1:
                 * L3_DEFIP_KEY_SEL = 000000_000
                 * No TCAM pairing - No Ipv6 LPM 128b entries */
                WRITE_L3_DEFIP_KEY_SELr(u, 0x0);
                /* TCAMs 0-6 are used for DIP IPv6 LPM 64b */
                kt_lpm_ipv6_info[u]->ipv6_64b.depth = l3_defip_tbl_size;
            };
        }
        SOC_LPM_STATE_FENT(u, (MAX_PFX_INDEX)) =
                kt_lpm_ipv6_info[u]->ipv6_64b.depth;
        SOC_LPM_STATE_END(u, (MAX_PFX_INDEX)) =
                kt_lpm_ipv6_info[u]->ipv6_64b.dip_start_offset - 1;
    }
    #endif


#if defined(FB_LPM_AVL_SUPPORT)
    if (SOC_LPM_STATE_AVL(u) != NULL) {
        if (shr_avl_destroy(SOC_LPM_STATE_AVL(u)) < 0) {
            SOC_LPM_UNLOCK(u);
            return SOC_E_INTERNAL;
        }
        SOC_LPM_STATE_AVL(u) = NULL;
    }
    if (shr_avl_create(&SOC_LPM_STATE_AVL(u), INT_TO_PTR(u),
                       sizeof(soc_lpm_avl_state_t), defip_table_size * 2) < 0) {
        SOC_LPM_UNLOCK(u);
        return SOC_E_MEMORY;
    }
#endif /* FB_LPM_AVL_SUPPORT */

#ifdef FB_LPM_HASH_SUPPORT
    if (SOC_LPM_STATE_HASH(u) != NULL) {
        if (_soc_fb_lpm_hash_destroy(SOC_LPM_STATE_HASH(u)) < 0) {
            SOC_LPM_UNLOCK(u);
            return SOC_E_INTERNAL;
        }
        SOC_LPM_STATE_HASH(u) = NULL;
    }

    hash_table_size = soc_mem_index_count(u, L3_DEFIPm);
    if (_soc_fb_lpm_hash_create(u, hash_table_size * 2, hash_table_size,
                                &SOC_LPM_STATE_HASH(u)) < 0) {
        SOC_LPM_UNLOCK(u);
        return SOC_E_MEMORY;
    }
#endif
    if (soc_feature(u, soc_feature_l3_shared_defip_table)) {
        if (soc_fb_lpm_stat_init(u) < 0) {
       /* COVERITY
        * soc_fb_lpm_deinit is called in error condition.
        * so irrespective of its return value we are going to release lpm lock
        * and return E_INTERNAL. Hence we can ignore the return value
        */
        /* coverity[unchecked_value] */
            soc_fb_lpm_deinit(u);
            SOC_LPM_UNLOCK(u);
            return SOC_E_INTERNAL;
        }
    }
    SOC_LPM_UNLOCK(u);

    return(SOC_E_NONE);
}
/*
 * De-initialize the start/end tracking pointers for each prefix length
 */
int
soc_fb_lpm_deinit(int u)
{
    if (!soc_feature(u, soc_feature_lpm_tcam)) {
        return(SOC_E_UNAVAIL);
    }

    SOC_LPM_LOCK(u);

#if defined(FB_LPM_AVL_SUPPORT)
    if (SOC_LPM_STATE_AVL(u) != NULL) {
        shr_avl_destroy(SOC_LPM_STATE_AVL(u));
        SOC_LPM_STATE_AVL(u) = NULL;
    }
#endif /* FB_LPM_AVL_SUPPORT */

#ifdef FB_LPM_HASH_SUPPORT
    if (SOC_LPM_STATE_HASH(u) != NULL) {
        _soc_fb_lpm_hash_destroy(SOC_LPM_STATE_HASH(u));
        SOC_LPM_STATE_HASH(u) = NULL;
    }
#endif

#ifdef BCM_KATANA_SUPPORT
    if (SOC_IS_KATANA(u)) {
        /* De-allocate any existing kt_lpm_ipv6_info structures. */
        if (kt_lpm_ipv6_info[u] != NULL) {
            kt_lpm_ipv6_info[u] = NULL;
            sal_free(kt_lpm_ipv6_info[u]);
        }
    }
#endif

    if (SOC_LPM_INIT_CHECK(u)) {
        sal_free(soc_lpm_field_cache_state[u]);
        soc_lpm_field_cache_state[u] = NULL;
        sal_free(SOC_LPM_STATE(u));
        SOC_LPM_STATE(u) = NULL;
    }

    if (SOC_LPM_STAT_INIT_CHECK(u)) {
        sal_free(soc_lpm_stat[u]);
        soc_lpm_stat[u] = NULL;
    }

    SOC_LPM_UNLOCK(u);

    return(SOC_E_NONE);
}

#ifdef BCM_WARM_BOOT_SUPPORT
int
soc_fb_lpm_reinit_done(int unit, int ipv6)
{
    int idx;
    int prev_idx = MAX_PFX_INDEX;
    uint32      v0 = 0, v1 = 0;
    int from_ent = -1;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS] = {0};

    int defip_table_size = soc_mem_index_count(unit, L3_DEFIPm);

    if (SOC_IS_HAWKEYE(unit)) {
        return SOC_E_NONE;
    }

    if (SOC_URPF_STATUS_GET(unit)) {
        if (soc_feature(unit, soc_feature_l3_defip_hole)) {
              defip_table_size = SOC_APOLLO_B0_L3_DEFIP_URPF_SIZE;
        } else if (SOC_IS_APOLLO(unit)) {
            defip_table_size = SOC_APOLLO_L3_DEFIP_URPF_SIZE;
        } else {
            /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
             * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
             * L3_DEFIP table is not divided into 2 to support URPF.
             */
            if (!soc_feature(unit, soc_feature_l3_defip_advanced_lookup)) {
                defip_table_size >>= 1;
            }
        }
    } 

    SOC_LPM_STATE_PREV(unit, MAX_PFX_INDEX) = -1;

    for (idx = MAX_PFX_INDEX; idx > 0 ; idx--) {
        if (-1 == SOC_LPM_STATE_START(unit, idx)) {
            continue;
        }

        if (prev_idx != idx) {
            SOC_LPM_STATE_PREV(unit, idx) = prev_idx;
            SOC_LPM_STATE_NEXT(unit, prev_idx) = idx;
        }
        SOC_LPM_STATE_FENT(unit, prev_idx) =                    \
                          SOC_LPM_STATE_START(unit, idx) -      \
                          SOC_LPM_STATE_END(unit, prev_idx) - 1;
        prev_idx = idx;
        
        if (idx == MAX_PFX_INDEX || (ipv6 && SOC_LPM_PFX_IS_V4(unit, idx)) ||
           (!ipv6 && SOC_LPM_PFX_IS_V6_64(unit, idx))) {
            continue;
        }

        if (!soc_feature(unit, soc_feature_l3_shared_defip_table)) {
            continue;
        }

        if (SOC_LPM_PFX_IS_V4(unit, idx)) {
            from_ent = SOC_LPM_STATE_END(unit, idx);
#ifdef FB_LPM_TABLE_CACHED
        SOC_IF_ERROR_RETURN(soc_mem_cache_invalidate(unit, L3_DEFIPm,
                                                     MEM_BLOCK_ANY, from_ent));
#endif /* FB_LPM_TABLE_CACHED */
            SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(unit, MEM_BLOCK_ANY, from_ent, e));
            v0 = SOC_MEM_OPT_F32_GET(unit, L3_DEFIPm, e, VALID0f);
            v1 = SOC_MEM_OPT_F32_GET(unit, L3_DEFIPm, e, VALID1f);
            SOC_LPM_V4_COUNT(unit) += (SOC_LPM_STATE_VENT(unit, idx) << 1);
            if ((v0 == 0) || (v1 == 0)) {
                SOC_LPM_V4_COUNT(unit) -= 1;
            }
            if ((v0 && (!(v1))) || (!(v0) && v1)) {
                SOC_LPM_COUNT_INC(SOC_LPM_V4_HALF_ENTRY_COUNT(unit));
            } 
        } else {
            SOC_LPM_64BV6_COUNT(unit) += SOC_LPM_STATE_VENT(unit, idx);
        }
    }

    SOC_LPM_STATE_NEXT(unit, prev_idx) = -1;
    SOC_LPM_STATE_FENT(unit, prev_idx) =                       \
                          defip_table_size -                   \
                          SOC_LPM_STATE_END(unit, prev_idx) - 1;

#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANA(unit)) {
        SOC_LPM_STATE_FENT(unit, MAX_PFX_INDEX) =
                kt_lpm_ipv6_info[unit]->ipv6_64b.depth;
    }
#endif

    return (SOC_E_NONE);
}

int
soc_fb_lpm_reinit(int unit, int idx, defip_entry_t *lpm_entry)
{
    int pfx_len;

    if (SOC_IS_HAWKEYE(unit)) {
        return SOC_E_NONE;
    }

    SOC_IF_ERROR_RETURN
        (_soc_fb_lpm_prefix_length_get(unit, lpm_entry, &pfx_len));

    if (SOC_LPM_STATE_VENT(unit, pfx_len) == 0) {
        SOC_LPM_STATE_START(unit, pfx_len) = idx;
        SOC_LPM_STATE_END(unit, pfx_len) = idx;
    } else {
        SOC_LPM_STATE_END(unit, pfx_len) = idx;
    }

    SOC_LPM_STATE_VENT(unit, pfx_len)++;
    LPM_HASH_INSERT(unit, lpm_entry, idx);
    LPM_AVL_INSERT(unit, lpm_entry, idx);

    return (SOC_E_NONE);
}


#endif /* BCM_WARM_BOOT_SUPPORT */


/*
 * Function:
 *     soc_fb_lpm_sw_dump
 * Purpose:
 *     Displays FB LPM information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
soc_fb_lpm_sw_dump(int unit)
{
    soc_lpm_state_p  lpm_state;
    int              i;

    LOG_CLI((BSL_META_U(unit,
                        "\n    FB LPM State -\n")));
    LOG_CLI((BSL_META_U(unit,
                        "        Prefix entries : %d\n"), MAX_PFX_ENTRIES));

    lpm_state = soc_lpm_state[unit];
    if (lpm_state == NULL) {
        return;
    }

    for (i = 0; i < MAX_PFX_INDEX; i++) {
        if (lpm_state[i].vent != 0 ) {
            LOG_CLI((BSL_META_U(unit,
                                "      Prefix %d\n"), i));
            LOG_CLI((BSL_META_U(unit,
                                "        Start : %d\n"), lpm_state[i].start));
            LOG_CLI((BSL_META_U(unit,
                                "        End   : %d\n"), lpm_state[i].end));
            LOG_CLI((BSL_META_U(unit,
                                "        Prev  : %d\n"), lpm_state[i].prev));
            LOG_CLI((BSL_META_U(unit,
                                "        Next  : %d\n"), lpm_state[i].next));
            LOG_CLI((BSL_META_U(unit,
                                "        Valid Entries : %d\n"), lpm_state[i].vent));
            LOG_CLI((BSL_META_U(unit,
                                "        Free  Entries : %d\n"), lpm_state[i].fent));
            if (soc_feature(unit, soc_feature_l3_shared_defip_table)) {
                if (SOC_LPM_PFX_IS_V4(unit, i)) {
                    LOG_CLI((BSL_META_U(unit,
                                        "        Type  : IPV4\n")));
                } else if (SOC_LPM_PFX_IS_V6_64(unit, i)) {
                    LOG_CLI((BSL_META_U(unit,
                                        "        Type  : 64B IPV6\n")));
                }
            }
        }
    }
    
    LOG_CLI((BSL_META_U(unit,
                        "      Prefix %d\n"), i));
    LOG_CLI((BSL_META_U(unit,
                        "        Start : %d\n"), lpm_state[i].start));
    LOG_CLI((BSL_META_U(unit,
                        "        End   : %d\n"), lpm_state[i].end));
    LOG_CLI((BSL_META_U(unit,
                        "        Prev  : %d\n"), lpm_state[i].prev));
    LOG_CLI((BSL_META_U(unit,
                        "        Next  : %d\n"), lpm_state[i].next));
    LOG_CLI((BSL_META_U(unit,
                        "        Valid Entries : %d\n"), lpm_state[i].vent));
    LOG_CLI((BSL_META_U(unit,
                        "        Free  Entries : %d\n"), lpm_state[i].fent));
    if (soc_feature(unit, soc_feature_l3_shared_defip_table)) {
        LOG_CLI((BSL_META_U(unit,
                            "        Type  : MAX\n")));
    }
    return;
}

/*
 * Implementation using soc_mem_read/write using entry rippling technique
 * Advantage: A completely sorted table is not required. Lookups can be slow
 * as it will perform a linear search on the entries for a given prefix length.
 * No device access necessary for the search if the table is cached. Auxiliary
 * hash table can be maintained to speed up search(Not implemented) instead of
 * performing a linear search.
 * Small number of entries need to be moved around (97 worst case)
 * for performing insert/update/delete. However CPU needs to do all
 * the work to move the entries.
 */

/*
 * soc_fb_lpm_insert
 * For IPV4 assume only both IP_ADDR0 is valid
 * Moving multiple entries around in h/w vs  doing a linear search in s/w
 */
int
soc_fb_lpm_insert(int u, void *entry_data)
{
    int         pfx;
    int         index, copy_index;
    int         ipv6;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS];
    int         rv = SOC_E_NONE;
    int         found = 0;

    sal_memcpy(e, soc_mem_entry_null(u, L3_DEFIPm),
               soc_mem_entry_words(u,L3_DEFIPm) * 4);

    SOC_LPM_LOCK(u);
    rv = _soc_fb_lpm_match(u, entry_data, e, &index, &pfx, &ipv6);
    if (rv == SOC_E_NOT_FOUND) {
        rv = _lpm_free_slot_create(u, pfx, ipv6, e, &index);
        if (rv < 0) {
            SOC_LPM_UNLOCK(u);
            return(rv);
        }
    } else {
        found = 1;
    }

    copy_index = index;
    if (rv == SOC_E_NONE) {
        /* Entry already present. Update the entry */
        if (!ipv6) {
            /* IPV4 entry */
            if (index & 1) {
                rv = soc_fb_lpm_ip4entry0_to_1(u, entry_data, e, PRESERVE_HIT);
            } else {
                rv = soc_fb_lpm_ip4entry0_to_0(u, entry_data, e, PRESERVE_HIT);
            }

            if (rv < 0) {
                SOC_LPM_UNLOCK(u);
                return(rv);
            }

            entry_data = (void *)e;
            index >>= 1;
        }
        soc_fb_lpm_state_dump(u);
        LOG_INFO(BSL_LS_SOC_LPM,
                 (BSL_META_U(u,
                             "\nsoc_fb_lpm_insert: %d %d\n"),
                             index, pfx));
        if (!found) {
            LPM_HASH_INSERT(u, entry_data, index);
            if (soc_feature(u, soc_feature_l3_shared_defip_table)) {
                if (ipv6) {
                    SOC_LPM_COUNT_INC(SOC_LPM_64BV6_COUNT(u));
                } else {
                    SOC_LPM_COUNT_INC(SOC_LPM_V4_COUNT(u));
                    /* half entry counts should change for new entry only */
                    if (copy_index & 1) {
                        SOC_LPM_COUNT_DEC(SOC_LPM_V4_HALF_ENTRY_COUNT(u));
                    } else {
                        SOC_LPM_COUNT_INC(SOC_LPM_V4_HALF_ENTRY_COUNT(u));
                    }
                }
            }
        }
        rv = WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, index, entry_data);
        if (rv >= 0) {
            rv = _lpm_fb_urpf_entry_replicate(u, index, entry_data);
        }
        LPM_AVL_INSERT(u, entry_data, index);
    }

    SOC_LPM_UNLOCK(u);
    return(rv);
}

/*
 * soc_fb_lpm_insert_index
 */
int
soc_fb_lpm_insert_index(int u, void *entry_data, int index)
{
    int         pfx;
    int         ipv6;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS];
    int         rv = SOC_E_NONE;
    int         new_add = 0;
    int         copy_index;

    if (index == -2) {
        return soc_fb_lpm_insert(u, entry_data);
    }

    SOC_LPM_LOCK(u);
    ipv6 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, MODE0f);
    if (ipv6) {
        if (!(ipv6 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, MODE1f))) {
            SOC_LPM_UNLOCK(u);
            return (SOC_E_PARAM);
        }
    }
    /* Calculate vrf weighted prefix lengh. */
    _soc_fb_lpm_prefix_length_get(u, entry_data, &pfx); 

    if (index == -1) {
        sal_memcpy(e, soc_mem_entry_null(u, L3_DEFIPm),
                   soc_mem_entry_words(u,L3_DEFIPm) * 4);
        rv = _lpm_free_slot_create(u, pfx, ipv6, e, &index);
        new_add = 1;
    } else {
        rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY,
                         ipv6 ? index : (index >> 1), e);
    }

    copy_index = index;
    if (rv == SOC_E_NONE) {
        /* Entry already present. Update the entry */
        if (!ipv6) {
            /* IPV4 entry */
            if (index & 1) {
                rv = soc_fb_lpm_ip4entry0_to_1(u, entry_data, e, PRESERVE_HIT);
            } else {
                rv = soc_fb_lpm_ip4entry0_to_0(u, entry_data, e, PRESERVE_HIT);
            }

            if (rv < 0) {
                SOC_LPM_UNLOCK(u);
                return(rv);
            }

            entry_data = (void *)e;
            index >>= 1;
        }
        soc_fb_lpm_state_dump(u);
        LOG_INFO(BSL_LS_SOC_LPM,
                 (BSL_META_U(u,
                             "\nsoc_fb_lpm_insert_index: %d %d\n"),
                             index, pfx));
        if (new_add) {
            LPM_HASH_INSERT(u, entry_data, index);
            if (soc_feature(u, soc_feature_l3_shared_defip_table)) {
                if (ipv6) {
                    SOC_LPM_COUNT_INC(SOC_LPM_64BV6_COUNT(u));
                } else {
                    SOC_LPM_COUNT_INC(SOC_LPM_V4_COUNT(u));
                    if (copy_index & 1) {
                        SOC_LPM_COUNT_DEC(SOC_LPM_V4_HALF_ENTRY_COUNT(u));
                    } else {
                        SOC_LPM_COUNT_INC(SOC_LPM_V4_HALF_ENTRY_COUNT(u));
                    }
                }
            }
        }
        rv = WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, index, entry_data);
        if (rv >= 0) {
            rv = _lpm_fb_urpf_entry_replicate(u, index, entry_data);
        }
        LPM_AVL_INSERT(u, entry_data, index);
    }

    SOC_LPM_UNLOCK(u);
    return(rv);
}

/*
 * soc_fb_lpm_delete
 */
int
soc_fb_lpm_delete(int u, void *key_data)
{
    int         pfx;
    int         index;
    int         ipv6;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS];
    int         rv = SOC_E_NONE;

    SOC_LPM_LOCK(u);
    rv = _soc_fb_lpm_match(u, key_data, e, &index, &pfx, &ipv6);
    if (rv == SOC_E_NONE) {
        LOG_INFO(BSL_LS_SOC_LPM,
                 (BSL_META_U(u,
                             "\nsoc_fb_lpm_delete: %d %d\n"),
                             index, pfx));
        LPM_HASH_DELETE(u, key_data, index);
        rv = _lpm_free_slot_delete(u, pfx, ipv6, e, index);
        LPM_AVL_DELETE(u, key_data, index);
        if (soc_feature(u, soc_feature_l3_shared_defip_table)) {
            if (ipv6) {
                SOC_LPM_COUNT_DEC(SOC_LPM_64BV6_COUNT(u));
            } else {
                SOC_LPM_COUNT_DEC(SOC_LPM_V4_COUNT(u));
            }
        }
    }
    soc_fb_lpm_state_dump(u);
    SOC_LPM_UNLOCK(u);
    return(rv);
}

/*
 * soc_fb_lpm_delete_index
 */
int
soc_fb_lpm_delete_index(int u, void *key_data, int index)
{
    int         pfx;
    int         ipv6;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS];
    int         rv = SOC_E_NONE;

    if (index == -1) {
        return soc_fb_lpm_delete(u, key_data);
    } 

    SOC_LPM_LOCK(u);
    ipv6 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, MODE0f);
    /* Calculate vrf weighted prefix lengh. */
    _soc_fb_lpm_prefix_length_get(u, key_data, &pfx); 
    rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY,
                     ipv6 ? index : (index >> 1), e);

    if (rv == SOC_E_NONE) {
        LOG_INFO(BSL_LS_SOC_LPM,
                 (BSL_META_U(u,
                             "\nsoc_fb_lpm_delete_index: %d %d\n"),
                             index, pfx));
        LPM_HASH_DELETE(u, key_data, index);
        rv = _lpm_free_slot_delete(u, pfx, ipv6, e, index);
        LPM_AVL_DELETE(u, key_data, index);
        if (soc_feature(u, soc_feature_l3_shared_defip_table)) {
            if (ipv6) {
                SOC_LPM_COUNT_DEC(SOC_LPM_64BV6_COUNT(u));
            } else {
                SOC_LPM_COUNT_DEC(SOC_LPM_V4_COUNT(u));
            }
        }
    }
    soc_fb_lpm_state_dump(u);
    SOC_LPM_UNLOCK(u);
    return(rv);
}

/*
 * soc_fb_lpm_match (Exact match for the key. Will match both IP
 *                   address and mask)
 */
int
soc_fb_lpm_match(int u,
               void *key_data,
               void *e,         /* return entry data if found */
               int *index_ptr)  /* return key location */
{
    int        pfx;
    int        rv;
    int        ipv6;

    SOC_LPM_LOCK(u);
    rv = _soc_fb_lpm_match(u, key_data, e, index_ptr, &pfx, &ipv6);
    SOC_LPM_UNLOCK(u);
    return(rv);
}

/*
 * soc_fb_lpm_lookup (LPM search). Masked match.
 */
int
soc_fb_lpm_lookup(int u,
               void *key_data,
               void *entry_data,
               int *index_ptr)
{
    return(SOC_E_UNAVAIL);
}

/*
 * Function:
 *	soc_fb_lpm_ipv4_delete_index
 * Purpose:
 *	Delete an entry from a LPM table based on index,
 * Parameters:
 *	u - StrataSwitch unit #
 *	Index for IPv4 entry is 0 .. 2 * max_index + 1. 
 *      If entry appears at offset N and entry 0 of the table
 *      then index = 2 * N + 0.
 *      If entry appears at offset N and entry 1 of the table
 *      then index = 2 * N + 1.
 *                            =========================================
 *     Offset N - 1           =  IPV4 Entry 1    =  IPV4 Entry 0      =
 *                            =========================================
 *     Offset N               =  IPV4 Entry 1    =  IPV4 Entry 0      =
 *                            =========================================
 *     Offset N + 1           =  IPV4 Entry 1    =  IPV4 Entry 0      =
 *                            =========================================
 */
int
soc_fb_lpm_ipv4_delete_index(int u, int index)
{
    int         pfx;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS];
    int         rv = SOC_E_NONE;

    SOC_LPM_LOCK(u);
    rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, index / 2, e); 
    if (rv == SOC_E_NONE) {
        if ((soc_mem_field32_get(u, L3_DEFIPm, e,
                                (index % 2) ? VALID1f : VALID0f)) &&
            (soc_mem_field32_get(u, L3_DEFIPm, e,
                                (index % 2) ? MODE1f : MODE0f) == 0)) {
            uint32      ipv4a;
            ipv4a = soc_mem_field32_get(u, L3_DEFIPm, e,
                                        (index % 2) ?
                                        IP_ADDR_MASK1f : IP_ADDR_MASK0f);
            rv = _ipmask2pfx(ipv4a, &pfx);
        } else {
            rv = SOC_E_PARAM;
        }
        if (rv == SOC_E_NONE) {
            LOG_INFO(BSL_LS_SOC_LPM,
                     (BSL_META_U(u,
                                 "\nsoc_fb_lpm_ipv4_delete_index: %d %d\n"),
                      index, pfx));
            LPM_HASH_DELETE(u, e, index);
            rv = _lpm_free_slot_delete(u, pfx, FALSE, e, index);
            LPM_AVL_DELETE(u, e, index);
        }
        soc_fb_lpm_state_dump(u);
    }
    SOC_LPM_UNLOCK(u);
    return(rv);
}

/*
 * Function:
 *	soc_fb_lpm_ipv6_delete_index
 * Purpose:
 *	Delete an entry from a LPM table based on index,
 * Parameters:
 *	u - StrataSwitch unit #
 *	Index for IPv6 entry is 0 .. max_index. 
 *                            =========================================
 *     Offset N - 1           =  IPV6 Entry                           =
 *                            =========================================
 *     Offset N               =  IPV6 Entry                           =
 *                            =========================================
 *     Offset N + 1           =  IPV6 Entry                           =
 *                            =========================================
 */
int
soc_fb_lpm_ipv6_delete_index(int u, int index)
{
    int         pfx;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS];
    int         rv = SOC_E_NONE;

    SOC_LPM_LOCK(u);
    rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, index , e); 
    if (rv == SOC_E_NONE) {
        if ((SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f)) &&
            (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, MODE0f)) &&
            (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, MODE1f)) &&
            (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f))) {
            uint32      ipv4a;
 
            ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, IP_ADDR_MASK0f);
            if ((rv = _ipmask2pfx(ipv4a, &pfx)) == SOC_E_NONE) {
                ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, IP_ADDR_MASK1f);
                if (pfx) {
                    if (ipv4a != 0xffffffff)  {
                        rv =SOC_E_PARAM;
                    }
                    pfx += 32;
                } else {
                    rv = _ipmask2pfx(ipv4a, &pfx);
                }
            }
        } else {
            rv = SOC_E_PARAM;
        }
        if (rv == SOC_E_NONE) {
            LOG_INFO(BSL_LS_SOC_LPM,
                     (BSL_META_U(u,
                                 "\nsoc_fb_lpm_ipv4_delete_index: %d %d\n"),
                      index, pfx));
            LPM_HASH_DELETE(u, e, index);
            rv = _lpm_free_slot_delete(u, pfx, TRUE,e, index);
            LPM_AVL_DELETE(u, e, index);
        }
        soc_fb_lpm_state_dump(u);
    }
    SOC_LPM_UNLOCK(u);
    return(rv);
}

int soc_fb_get_largest_prefix(int u, int ipv6, void *e, 
                              int *index, int *pfx_len, int* count)
{
    int curr_pfx = MAX_PFX_INDEX;
    int next_pfx = SOC_LPM_STATE_NEXT(u, curr_pfx);
    int rv = SOC_E_NOT_FOUND;
    uint32 v1;

    while (next_pfx != -1) {
        if (!ipv6 && SOC_LPM_PFX_IS_V4(u, next_pfx)) {
            break; 
        } else if(ipv6 && SOC_LPM_PFX_IS_V6_64(u, next_pfx)) {
           break;
        } 
        next_pfx = SOC_LPM_STATE_NEXT(u, next_pfx);  
    }

    if (next_pfx == -1) {
        return rv;
    }

    *index = SOC_LPM_STATE_END(u, next_pfx);
    *pfx_len = next_pfx;

#ifdef FB_LPM_TABLE_CACHED
    if ((rv = soc_mem_cache_invalidate(u, L3_DEFIPm,
                                       MEM_BLOCK_ANY, *index)) < 0) {
        return rv;
    }
#endif /* FB_LPM_TABLE_CACHED */
    if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, *index, e)) < 0) {
        return rv;
    }

    v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);
   
    *count = 1;
    if (!ipv6 && v1) {
       *count = 2;    
    }
    return rv;
}


                        /* LPM 128 code starts here */

#define LPM128_HASH_INSERT(u, entry_data, entry_data_upr, tab_index)       \
    soc_fb_lpm128_hash_insert(u, entry_data, entry_data_upr, tab_index, LPM_NO_MATCH_INDEX, 0)

#define LPM128_HASH_DELETE(u, key_data, key_data_upr, tab_index)         \
    soc_fb_lpm128_hash_delete(u, key_data, key_data_upr, tab_index)

#define LPM128_HASH_LOOKUP(u, key_data, key_data_upr, pfx, tab_index)    \
    soc_fb_lpm128_hash_lookup(u, key_data, key_data_upr, pfx, tab_index)

STATIC int
_lpm128_free_slot_move_up(int u, int pfx, int free_pfx,
                          soc_lpm128_state_p lpm_state_ptr);
STATIC int
_lpm128_free_slot_move_down(int u, int pfx, int free_pfx,
                            soc_lpm128_state_p lpm_state_ptr, 
                            int erase);

STATIC
void _soc_fb_lpm128_hash_entry_get(int u, void *e, void *eupr,
                                int index, _soc_fb_lpm128_hash_entry_t r_entry)
{
    if (index & FB_LPM_HASH_IPV6_MASK) {
        SOC_FB_LPM128_HASH_ENTRY_IPV6_GET(u, e, eupr, r_entry);
    } else {
        if (index & 0x1) {
            SOC_FB_LPM_HASH_ENTRY_IPV4_1_GET(u, e, r_entry);
        } else {
            SOC_FB_LPM_HASH_ENTRY_IPV4_0_GET(u, e, r_entry);
        }
    }
}

/*
 * Function:
 *      _soc_fb_lpm128_hash_insert
 * Purpose:
 *      Insert/Update a key index in the hash table
 * Parameters:
 *      hash - Pointer (handle) to Hash Table
 *      key_cmp_fn - Compare function which should compare key
 *      entry   - The key to lookup
 *      pfx     - Prefix length for lookup acceleration.
 *      old_index - Index where the key was moved from.
 *                  FB_LPM_HASH_INDEX_NULL if new entry.
 *      new_index - Index where the key was moved to.
 * Returns:
 *      SOC_E_NONE      Key found
 */
/*
 *      Should be caled before updating the LPM table so that the
 *      data in the hash table is consistent with the LPM table
 */
static
int _soc_fb_lpm128_hash_insert(_soc_fb_lpm_hash_t *hash,
                            _soc_fb_lpm128_hash_compare_fn key_cmp_fn,
                            _soc_fb_lpm128_hash_entry_t entry,
                            int    pfx,
                            uint16 old_index,
                            uint16 new_index)
{
    int u = hash->unit;
    uint16 hash_val;
    uint16 index;
    uint16 prev_index;

    hash_val = _soc_fb_lpm_hash_compute((uint8 *)entry,
               (32 * _SOC_LPM128_HASH_KEY_LEN_IN_WORDS)) % hash->index_count;
    index = hash->table[hash_val];
    H_INDEX_MATCH("ihash", entry[0], hash_val);
    H_INDEX_MATCH("ins  ", entry[0], new_index);
    H_INDEX_MATCH("ins1 ", index, new_index);
    prev_index = FB_LPM_HASH_INDEX_NULL;
    if (old_index != FB_LPM_HASH_INDEX_NULL) {
        while(index != FB_LPM_HASH_INDEX_NULL) {
            uint32  e[SOC_MAX_MEM_FIELD_WORDS];
            uint32  eupr[SOC_MAX_MEM_FIELD_WORDS];
            _soc_fb_lpm128_hash_entry_t  r_entry = {0};
            int     rindex;
            
            rindex = (index & FB_LPM_HASH_INDEX_MASK) >> 1;

            /*
             * Check prefix length and skip index if not valid for given length
            if ((SOC_LPM_STATE_START(u, pfx) <= rindex) &&
                (SOC_LPM_STATE_END(u, pfx) >= rindex)) {
             */
            SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, rindex, e));
            if (index & FB_LPM_HASH_IPV6_MASK) {
                SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, rindex + 1024, eupr));
                SOC_FB_LPM128_HASH_ENTRY_GET(u, e, eupr, index, r_entry);
            } else { 
                SOC_FB_LPM128_HASH_ENTRY_GET(u, e, NULL, index, r_entry);
            }
            if ((*key_cmp_fn)(entry, r_entry) == 0) {
                /* assert(old_index == index);*/
                if (new_index != index) {
                    H_INDEX_MATCH("imove", prev_index, new_index);
                    if (prev_index == FB_LPM_HASH_INDEX_NULL) {
                        INDEX_UPDATE(hash, hash_val, index, new_index);
                    } else {
                        INDEX_UPDATE_LINK(hash, prev_index, index, new_index);
                    }
                }
                H_INDEX_MATCH("imtch", index, new_index);
                return(SOC_E_NONE);
            }
            /*
            }
            */
            prev_index = index;
            index = hash->link_table[index & FB_LPM_HASH_INDEX_MASK];
            H_INDEX_MATCH("ins2 ", index, new_index);
        }
    }
    INDEX_ADD(hash, hash_val, new_index);  /* new entry */
    return(SOC_E_NONE);
}

/*
 * Function:
 *      _soc_fb_lpm128_hash_lookup
 * Purpose:
 *      Look up a key in the hash table
 * Parameters:
 *      hash - Pointer (handle) to Hash Table
 *      key_cmp_fn - Compare function which should compare key
 *      entry   - The key to lookup
 *      pfx     - Prefix length for lookup acceleration.
 *      key_index - (OUT)       Index where the key was found.
 * Returns:
 *      SOC_E_NONE      Key found
 *      SOC_E_NOT_FOUND Key not found
 */

static
int _soc_fb_lpm128_hash_lookup(_soc_fb_lpm_hash_t          *hash,
                            _soc_fb_lpm128_hash_compare_fn key_cmp_fn,
                            _soc_fb_lpm128_hash_entry_t    entry,
                            int                         pfx,
                            uint16                      *key_index)
{
    int u = hash->unit;

    uint16 hash_val;
    uint16 index;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);

    hash_val = _soc_fb_lpm_hash_compute((uint8 *)entry,
               (32 * _SOC_LPM128_HASH_KEY_LEN_IN_WORDS)) % hash->index_count;
    index = hash->table[hash_val];
    H_INDEX_MATCH("lhash", entry[0], hash_val);
    H_INDEX_MATCH("lkup ", entry[0], index);
    while(index != FB_LPM_HASH_INDEX_NULL) {
        uint32  e[SOC_MAX_MEM_FIELD_WORDS];
        uint32  eupr[SOC_MAX_MEM_FIELD_WORDS];
        _soc_fb_lpm128_hash_entry_t  r_entry = {0};
        int     rindex;

        rindex = (index & FB_LPM_HASH_INDEX_MASK) >> 1;
        /*
         * Check prefix length and skip index if not valid for given length
        if ((SOC_LPM_STATE_START(u, pfx) <= rindex) &&
            (SOC_LPM_STATE_END(u, pfx) >= rindex)) {
         */
        SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, rindex, e));
        if (index & FB_LPM_HASH_IPV6_MASK) {
            SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, 
                                               rindex + tcam_depth, eupr));
            SOC_FB_LPM128_HASH_ENTRY_GET(u, e, eupr, index, r_entry);
        } else {
            SOC_FB_LPM128_HASH_ENTRY_GET(u, e, NULL, index, r_entry);
        }
        if ((*key_cmp_fn)(entry, r_entry) == 0) {
            *key_index = (index & FB_LPM_HASH_INDEX_MASK) >> 
                            ((index & FB_LPM_HASH_IPV6_MASK) ? 1 : 0);
            H_INDEX_MATCH("found", entry[0], index);
            return(SOC_E_NONE);
        }
        /*
        }
        */
        index = hash->link_table[index & FB_LPM_HASH_INDEX_MASK];
        H_INDEX_MATCH("lkup1", entry[0], index);
    }
    H_INDEX_MATCH("not_found", entry[0], index);
    return(SOC_E_NOT_FOUND);
}

void soc_fb_lpm128_hash_insert(int u, void *entry_data, void *entry_data_upr, 
                               uint32 tab_index, uint32 old_index, int pfx)
{
    _soc_fb_lpm128_hash_entry_t    key_hash = {0};
    int mode = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, MODE0f);
 
    if (mode && mode != 3) {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "Invalid mode while inserting lpm128 hash for pfx - %d\n"), pfx));
        return; 
    } 
    if (mode) {
        /* IPV6 entry */
        if (entry_data_upr == NULL) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "upper data is NULL for pfx - %d\n"), pfx));
            return;
        }
        SOC_FB_LPM128_HASH_ENTRY_IPV6_GET(u, entry_data, entry_data_upr, 
                                          key_hash);
        _soc_fb_lpm128_hash_insert(
                    SOC_LPM128_STATE_HASH(u),
                    _soc_fb_lpm128_hash_compare_key,
                    key_hash,
                    pfx,
                    old_index,
                    ((uint16)tab_index << 1) | FB_LPM_HASH_IPV6_MASK);
    } else {
        /* IPV4 entry */
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VALID0f)) {
            SOC_FB_LPM_HASH_ENTRY_IPV4_0_GET(u, entry_data, key_hash);
            _soc_fb_lpm128_hash_insert(SOC_LPM128_STATE_HASH(u),
                                    _soc_fb_lpm128_hash_compare_key,
                                    key_hash,
                                    pfx,
                                    old_index,
                                    ((uint16)tab_index << 1));
        }
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_data, VALID1f)) {
            SOC_FB_LPM_HASH_ENTRY_IPV4_1_GET(u, entry_data, key_hash);
            _soc_fb_lpm128_hash_insert(SOC_LPM128_STATE_HASH(u),
                                    _soc_fb_lpm128_hash_compare_key,
                                    key_hash,
                                    pfx,
                                    old_index,
                                    (((uint16)tab_index << 1) + 1));
        }
    }
}

void soc_fb_lpm128_hash_delete(int u, void *key_data, void *key_data_upr, 
                               uint32 tab_index)
{
    _soc_fb_lpm128_hash_entry_t    key_hash = {0};
    int                         pfx = -1;
    int                         rv;
    uint16                      index;

    if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, MODE0f)) {
        SOC_FB_LPM128_HASH_ENTRY_IPV6_GET(u, key_data, key_data_upr, key_hash);
        index = (tab_index << 1) | FB_LPM_HASH_IPV6_MASK;
    } else {
        SOC_FB_LPM_HASH_ENTRY_IPV4_0_GET(u, key_data, key_hash);
        index = tab_index;
    }

    rv = _soc_fb_lpm128_hash_delete(SOC_LPM128_STATE_HASH(u),
                                 _soc_fb_lpm128_hash_compare_key,
                                 key_hash, pfx, index);
    if (SOC_FAILURE(rv)) {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "\ndel  index: H %d error %d\n"), index, rv));
    }
}

int soc_fb_lpm128_hash_lookup(int u, void *key_data, void *key_data_upr, 
                              int pfx, int *key_index)
{
    _soc_fb_lpm128_hash_entry_t    key_hash = {0};
    int                         is_ipv6;
    int                         rv;
    uint16                      index = FB_LPM_HASH_INDEX_NULL;

    is_ipv6 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, key_data, MODE0f);
    if (is_ipv6 == 3) {
        SOC_FB_LPM128_HASH_ENTRY_IPV6_GET(u, key_data, key_data_upr, key_hash);
    } else if(is_ipv6 == 0) {
        SOC_FB_LPM_HASH_ENTRY_IPV4_0_GET(u, key_data, key_hash);
    } else {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "LPM128 hash lookup for pfx - %d failed\n"), pfx));
        return SOC_E_PARAM;
    }

    rv = _soc_fb_lpm128_hash_lookup(SOC_LPM128_STATE_HASH(u),
                                 _soc_fb_lpm128_hash_compare_key,
                                 key_hash, pfx, &index);
    if (SOC_FAILURE(rv)) {
        *key_index = 0xFFFFFFFF;
        return(rv);
    }

    *key_index = index;
    return(SOC_E_NONE);
}


/*
 * _soc_fb_lpm_prefix_length_get (Extract vrf weighted  prefix lenght from the 
 * hw entry based on ip, mask & vrf)
 */
static int
_soc_fb_lpm128_prefix_length_get(int u, void *entry, void* entry_upr, 
                                 int *pfx_len)
{
    int         pfx_len1 = 0;
    int         pfx_len2 = 0;
    int         pfx_len3 = 0;
    int         pfx_len4 = 0;
    int         rv;
    int         mode;
    uint32      ipv4a;
    int         vrf_id;
    int pfx;
    int offset = 0;
    int value;

    mode = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry, MODE0f);

    if (mode && (mode != 3)) {
       return SOC_E_PARAM;
    }

    ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry, IP_ADDR_MASK0f);
    if (ipv4a) {
        if ((rv = _ipmask2pfx(ipv4a, &pfx_len1)) < 0) {
            return(rv);
        }
    }
    if (mode) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry, IP_ADDR_MASK1f);
        if (ipv4a) {
            if ((rv = _ipmask2pfx(ipv4a, &pfx_len2)) < 0) {
                return(rv);
            }
        }

        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_upr, IP_ADDR_MASK0f);
        if (ipv4a) {
            if ((rv = _ipmask2pfx(ipv4a, &pfx_len3)) < 0) {
                return(rv);
            }
        }

        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, entry_upr, IP_ADDR_MASK1f);
        if (ipv4a) {
            if ((rv = _ipmask2pfx(ipv4a, &pfx_len4)) < 0) {
                return(rv);
            }
        }
    }

    pfx = pfx_len1 + pfx_len2 + pfx_len3 + pfx_len4;

    if (mode == 0) {
        offset = 0;
    } else {
        offset = (pfx > 64) ? (MAX_PFX_64_OFFSET + MAX_PFX_32_OFFSET) :
                 MAX_PFX_32_OFFSET;
    }

    pfx += offset;

    SOC_IF_ERROR_RETURN(soc_fb_lpm_vrf_get(u, entry, &vrf_id));

    switch(mode) {
        case 0:
            value = MAX_PFX_32_OFFSET;
            break; 
        case 3:
            if (offset > MAX_PFX_32_OFFSET) {
                value = MAX_PFX_128_OFFSET;
            } else {
                value = MAX_PFX_64_OFFSET;
            }  
            break;
        /* COVERITY
         * Default switch case
         */
        /* coverity[dead_error_begin] */
        default:
            return SOC_E_INTERNAL;
    }

    switch (vrf_id) { 
      case SOC_L3_VRF_GLOBAL:
          *pfx_len = pfx;
          break;
      case SOC_L3_VRF_OVERRIDE:
          *pfx_len = pfx +  2 * (value / 3) ;
          break;
      default:   
          *pfx_len = pfx +  (value / 3);
          break;
    }
    return (SOC_E_NONE);
}
/*
 * _soc_fb_lpm_match (Exact match for the key. Will match both IP address
 * and mask)
 */

STATIC int
_soc_fb_lpm128_match(int u,
               void *key_data,
               void *key_data_upr,
               void *e,         /* return entry data if found */
               void *eupr,         /* return entry data if found */
               int *index_ptr,  /* return key location */
               int *pfx_len,    /* Key prefix length. vrf + 32 + prefix len for IPV6*/
               soc_lpm_entry_type_t *type_out)
{
    int         rv;
    int         key_index;
    int         pfx = 0;
    int         tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    soc_lpm_entry_type_t type;

    /* Calculate vrf weighted prefix lengh. */
    _soc_fb_lpm128_prefix_length_get(u, key_data, key_data_upr, &pfx);
    *pfx_len = pfx;

     if (SOC_LPM128_PFX_IS_V6_128(u, pfx)) {
         type = socLpmEntryType128BV6;
     } else if (SOC_LPM128_PFX_IS_V6_64(u, pfx)) {
         type = socLpmEntryType64BV6;
     } else {
         type = socLpmEntryTypeV4;
     }

#ifdef FB_LPM_HASH_SUPPORT
    rv = LPM128_HASH_LOOKUP(u, key_data, key_data_upr, pfx, &key_index);
    if (rv == SOC_E_NONE) {
        *index_ptr = key_index;
        if (type == socLpmEntryTypeV4) {
            key_index = key_index >> 1; 

        }      
        if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, key_index, e)) < 0) {
            return rv;
        }
        if (type == socLpmEntryType128BV6) {
            if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, 
                                     (key_index + tcam_depth),
                      eupr)) < 0) {
                return rv;
            }
        }

    }
#endif /* FB_LPM_HASH_SUPPORT */
    if (type_out != NULL) {
       *type_out = type;
    }
    return rv;
}

/*
 * Initialize the start/end tracking pointers for each prefix length
 */

int
soc_fb_lpm128_init(int u)
{
    int max_pfx_len;
    int defip_table_size = 0;
    int pfx_state_size;
    int i;
    int num_ipv6_128b_entries;
    int tcam_pair_count = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int is_reserved = 0;
    int unreserved_start_offset = 0; 
    soc_lpm128_state_p lpm_state_ptr = NULL;
    int hash_table_size = 0;

    if (!soc_feature(u, soc_feature_lpm_tcam)) {
        return(SOC_E_UNAVAIL);
    }

    if (!soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        return SOC_E_UNAVAIL;
    }

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    num_ipv6_128b_entries =  SOC_L3_DEFIP_MAX_128B_ENTRIES(u);

    SOC_IF_ERROR_RETURN(soc_fb_lpm_tcam_pair_count_get(u, &tcam_pair_count));

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (tcam_pair_count && SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        switch(tcam_pair_count) {
            case 1:
            case 2:
                defip_table_size = 2 * tcam_depth;
                break; 
            case 3:
            case 4:
                defip_table_size = 4 * tcam_depth;
                break;
            default:
                defip_table_size = 0;
               break;
        }
    } else {
        defip_table_size = (tcam_pair_count * tcam_depth * 2);
    }

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        num_ipv6_128b_entries >>= 1;
    }

    if(SOC_LPM128_INDEX_TO_PFX_GROUP_TABLE(u)) {
        /* defip table size might have changed */
        sal_free(SOC_LPM128_INDEX_TO_PFX_GROUP_TABLE(u));
        SOC_LPM128_INDEX_TO_PFX_GROUP_TABLE(u) = NULL;
    }

    SOC_LPM128_INDEX_TO_PFX_GROUP_TABLE(u) =
        sal_alloc(defip_table_size * sizeof(int), 
                  "SOC LPM128 GROUP TO PFX");
    if (NULL == SOC_LPM128_INDEX_TO_PFX_GROUP_TABLE(u)) {
        return SOC_E_MEMORY;
    }
    
    sal_memset(SOC_LPM128_INDEX_TO_PFX_GROUP_TABLE(u), -1,
               defip_table_size * sizeof(int));

    if (!SOC_LPM128_STATE_TABLE_INIT_CHECK(u)) {
        SOC_LPM128_STATE_TABLE(u) = sal_alloc(sizeof(soc_lpm128_table_t),
                                              "soc LPM STATE table");
        if (NULL == SOC_LPM128_STATE_TABLE(u)) {
            soc_fb_lpm128_deinit(u);
            return SOC_E_MEMORY;
        }
    }
    sal_memset(SOC_LPM128_STATE_TABLE(u), 0x0, sizeof(soc_lpm128_table_t));

    max_pfx_len = MAX_PFX128_ENTRIES;
    pfx_state_size = sizeof(soc_lpm128_state_t) * (max_pfx_len);

    if (!SOC_LPM128_INIT_CHECK(u)) {
        SOC_LPM128_STATE(u) = sal_alloc(pfx_state_size, "LPM prefix info");
        if (NULL == SOC_LPM128_STATE(u)) {
            soc_fb_lpm128_deinit(u);
            return (SOC_E_MEMORY);
        }
    }

    if (!SOC_LPM128_UNRESERVED_INIT_CHECK(u)) {
        if (is_reserved && (num_ipv6_128b_entries % tcam_depth) != 0) {
            SOC_LPM128_UNRESERVED_STATE(u) = sal_alloc(pfx_state_size,
                                                       "LPM prefix info");
            if (NULL == SOC_LPM128_UNRESERVED_STATE(u)) {
                soc_fb_lpm128_deinit(u);
                return (SOC_E_MEMORY);
            }
        } else {
            SOC_LPM128_UNRESERVED_STATE(u) = NULL;
        }
    } else {
        if (is_reserved && (num_ipv6_128b_entries % tcam_depth) == 0) {
           sal_free(SOC_LPM128_UNRESERVED_STATE(u));
           SOC_LPM128_UNRESERVED_STATE(u) = NULL;
        }
    }

    SOC_LPM_LOCK(u);

    SOC_LPM128_STATE_TABLE_TOTAL_COUNT(u) = defip_table_size;

    lpm_state_ptr = SOC_LPM128_STATE(u);
    for(i = 0; i < max_pfx_len; i++) {
        SOC_LPM128_STATE_START1(u, lpm_state_ptr, i) = -1;
        SOC_LPM128_STATE_START2(u, lpm_state_ptr, i) = -1;
        SOC_LPM128_STATE_END1(u, lpm_state_ptr, i) = -1;
        SOC_LPM128_STATE_END2(u, lpm_state_ptr, i) = -1;
        SOC_LPM128_STATE_PREV(u, lpm_state_ptr, i) = -1;
        SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, i) = -1;
        SOC_LPM128_STATE_VENT(u, lpm_state_ptr, i) = 0;
        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, i) = 0;
    }

    if (is_reserved) {
        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, (MAX_PFX128_INDEX)) = 
                              2 * num_ipv6_128b_entries;
    } else {
        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, (MAX_PFX128_INDEX)) = 
                              defip_table_size;
    }
    unreserved_start_offset = 0; 
    if (is_reserved && ((num_ipv6_128b_entries % tcam_depth) != 0)) {
        unreserved_start_offset = 
            ((num_ipv6_128b_entries / tcam_depth) * tcam_depth * 2) +
            (num_ipv6_128b_entries % tcam_depth);
    }

    if (unreserved_start_offset) {
        lpm_state_ptr = SOC_LPM128_UNRESERVED_STATE(u);
        for(i = 0; i < max_pfx_len; i++) {
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, i) = -1;
            SOC_LPM128_STATE_START2(u, lpm_state_ptr, i) = -1;
            SOC_LPM128_STATE_END1(u, lpm_state_ptr, i) = -1;
            SOC_LPM128_STATE_END2(u, lpm_state_ptr, i) = -1;
            SOC_LPM128_STATE_PREV(u, lpm_state_ptr, i) = -1;
            SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, i) = -1;
            SOC_LPM128_STATE_VENT(u, lpm_state_ptr, i) = 0;
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, i) = 0;
        }
        /* Assign all fent entries to FENT only */
       SOC_LPM128_STATE_FENT(u, lpm_state_ptr, (MAX_PFX128_INDEX)) = 
           defip_table_size - (2 * num_ipv6_128b_entries);
       
       SOC_LPM128_STATE_START1(u, lpm_state_ptr, (MAX_PFX128_INDEX)) =
           unreserved_start_offset;
       SOC_LPM128_STATE_END1(u, lpm_state_ptr, (MAX_PFX128_INDEX)) =
           unreserved_start_offset - 1;
   }

#ifdef FB_LPM_HASH_SUPPORT
    if (SOC_LPM128_STATE_HASH(u) != NULL) {
        if (_soc_fb_lpm_hash_destroy(SOC_LPM128_STATE_HASH(u)) < 0) {
            SOC_LPM_UNLOCK(u);
            soc_fb_lpm128_deinit(u);
            return SOC_E_INTERNAL;
        }
        SOC_LPM128_STATE_HASH(u) = NULL;
    }

    hash_table_size = soc_mem_index_count(0, L3_DEFIPm);
    if (_soc_fb_lpm_hash_create(u, hash_table_size * 2, hash_table_size,
                                &SOC_LPM128_STATE_HASH(u)) < 0) {
        soc_fb_lpm128_deinit(u);
        SOC_LPM_UNLOCK(u);
        return SOC_E_MEMORY;
    }
#endif
    if (soc_fb_lpm128_stat_init(u) < 0) {
        soc_fb_lpm128_deinit(u);
        SOC_LPM_UNLOCK(u);
        return SOC_E_INTERNAL;
    }
    SOC_LPM_UNLOCK(u);

    return(SOC_E_NONE);
}

/*
 * De-initialize the start/end tracking pointers for each prefix length
 */
int
soc_fb_lpm128_deinit(int u)
{
    if (!soc_feature(u, soc_feature_lpm_tcam)) {
        return(SOC_E_UNAVAIL);
    }

    if (!soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        return SOC_E_UNAVAIL;
    }

    SOC_LPM_LOCK(u);
    if (SOC_LPM128_INDEX_TO_PFX_GROUP_TABLE(u) != NULL) {
        sal_free(SOC_LPM128_INDEX_TO_PFX_GROUP_TABLE(u));
        SOC_LPM128_INDEX_TO_PFX_GROUP_TABLE(u) = NULL;
    }

    if (SOC_LPM128_STATE_TABLE_INIT_CHECK(u) && SOC_LPM128_INIT_CHECK(u)) {
        sal_free(SOC_LPM128_STATE(u));
        SOC_LPM128_STATE(u) = NULL;
    }

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved) && 
        SOC_LPM128_STATE_TABLE_INIT_CHECK(u) && 
        SOC_LPM128_UNRESERVED_INIT_CHECK(u)) {
        sal_free(SOC_LPM128_UNRESERVED_STATE(u));
        SOC_LPM128_UNRESERVED_STATE(u) = NULL;
    }

    if (SOC_LPM128_STATE_TABLE_INIT_CHECK(u)) {
        sal_free(SOC_LPM128_STATE_TABLE(u));
        SOC_LPM128_STATE_TABLE(u) = NULL;  
    }
#ifdef FB_LPM_HASH_SUPPORT
    if (SOC_LPM128_STATE_HASH(u) != NULL) {
        _soc_fb_lpm_hash_destroy(SOC_LPM128_STATE_HASH(u));
        SOC_LPM128_STATE_HASH(u) = NULL;
    }
#endif

    SOC_LPM_UNLOCK(u);

    return(SOC_E_NONE);
}

STATIC INLINE int
_lpm128_find_pfx_type(int u, int pfx, soc_lpm_entry_type_t *type)
{
    if (pfx == -1) {
        return SOC_E_INTERNAL;
    }

    if (SOC_LPM128_PFX_IS_V4(u, pfx)) {
        *type = socLpmEntryTypeV4; 
    } else if(SOC_LPM128_PFX_IS_V6_64(u, pfx)) {
        *type = socLpmEntryType64BV6; 
    } else {
        *type = socLpmEntryType128BV6; 
    }
   
    return SOC_E_NONE;
}

STATIC INLINE int
_lpm128_if_conflicting_type(int u, soc_lpm_entry_type_t t1, 
                                   soc_lpm_entry_type_t t2) 
{
    if (t1 == socLpmEntryTypeV4 && (t2 != socLpmEntryTypeV4)) {
        return TRUE; 
    }

    if (t2 == socLpmEntryTypeV4 && (t1 != socLpmEntryTypeV4)) {
        return TRUE;
    }

    return FALSE;
}

STATIC int 
_lpm128_fb_urpf_entry_replicate(int u, int index, uint32 *e, uint32 *eupr)
{
    int src_tcam_offset;  /* Defip memory size/2 urpf source lookup offset */
    int new_index = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int max_tcams = SOC_L3_DEFIP_MAX_TCAMS_GET(u); 

    if(!SOC_URPF_STATUS_GET(u)) {
        return (SOC_E_NONE);
    }

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        return (SOC_E_NONE);
    }

    src_tcam_offset = (max_tcams * tcam_depth) / 2;
    new_index = index + src_tcam_offset;
    /* Write entry to the second half of the tcam. */
    SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, new_index, e));
    if (eupr) {
        SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, 
                                            new_index + tcam_depth, eupr));
    }
    return SOC_E_NONE;
}

void
soc_lpm128_group_index_map_table_dump(int u) 
{
    int i = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int defip_table_size;
    int tcam_pair_count = 0;

    if (soc_fb_lpm_tcam_pair_count_get(u, &tcam_pair_count) < 0) {
        return;
    }

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        switch(tcam_pair_count) {
            case 1:
            case 2:
                defip_table_size = (2 * tcam_depth);
                break;
            case 3:
            case 4:
                defip_table_size = 4 * tcam_depth;
                break;
            case 0:
            default:
               defip_table_size = 0;
                break;
        }
    } else {
        defip_table_size = (tcam_pair_count * tcam_depth * 2);
    }

    for (i = 0; i < defip_table_size; i++) {
        if ( (i + 1) % 5 == 0) {
            LOG_CLI((BSL_META_U(u,
                                "\n")));
        }
        LOG_CLI((BSL_META_U(u,
                            "[%05d, %05d]  "), i, SOC_LPM128_INDEX_TO_PFX_GROUP(u, i)));
    }
    LOG_CLI((BSL_META_U(u,
                        "\n")));
}

void print_pfx_info(int u, soc_lpm128_state_p lpm_state_ptr, int pfx) 
{
    int start1 = -1;
    int start2 = -1;
    int end1 = -1;
    int end2 = -1;
    int fent = -1;
    int vent = -1;
    int prev = -1;
    int next = -1;
    soc_lpm_entry_type_t type = socLpmEntryType128BV6;

    if (_lpm128_find_pfx_type(u, pfx, &type) < 0) {
        return;
    }

    start1 = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx);
    start2 = SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx);
    end1   = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
    end2   = SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx);
    fent   = SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx);
    vent   = SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx);
    prev   = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, pfx);
    next   = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, pfx);

    if (type == socLpmEntryTypeV4) {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "------------V4 pfx - %d ---------------------\n"), 
                              pfx));
    } else if(type == socLpmEntryType64BV6) {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "------------ 64B V6 pfx - %d ----------------\n"), 
                              pfx));
    } else {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "------------ 128B V6 pfx - %d ---------------\n"), 
                              pfx));
    }
    LOG_ERROR(BSL_LS_SOC_LPM,
              (BSL_META_U(u,
                          "Start1: %d End1: %d\n"), start1, end1));
    LOG_ERROR(BSL_LS_SOC_LPM,
              (BSL_META_U(u,
                          "Start2: %d End2: %d\n"), start2, end2));
    LOG_ERROR(BSL_LS_SOC_LPM,
              (BSL_META_U(u,
                          "Fent: %d Vent: %d\n"), fent, vent));
    LOG_ERROR(BSL_LS_SOC_LPM,
              (BSL_META_U(u,
                          "Prev: %d Next: %d\n"), prev, next));
    LOG_ERROR(BSL_LS_SOC_LPM,
              (BSL_META_U(u,
                          "------------ END ---------------------\n")));
}

/* This works only if there are no free entries */
int test_lpm128_verify_algorithm(int u)
{
    int i = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int defip_table_size = 0;
    int tcam_pair_count = 0;
    int free_count = 0;
    soc_lpm128_state_p lpm_state_ptr = SOC_LPM128_STATE(u);
    int32 curr_pfx = MAX_PFX128_INDEX;
    int32 *pfxmap = NULL;
    int start1, start2;
    int end1, end2;
    int vent = 0;
    int vent_count = 0;
    int split_count = 0;
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);
    int is_reserved = 0;
    int next_pfx = -1;
    soc_lpm_entry_type_t type;
    int prev_split_pfx = -1;
    int max_entries = 0;
    int end_idx = 0;
    int fent = 0;

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        max_v6_entries >>= 1;
    }

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    SOC_IF_ERROR_RETURN(soc_fb_lpm_tcam_pair_count_get(u, &tcam_pair_count));

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        switch(tcam_pair_count) {
            case 1:
            case 2:
                defip_table_size = 2 * tcam_depth;
                break;
            case 3:
            case 4:
                defip_table_size = 4 * tcam_depth;
                break;
            case 0:
            default:
               defip_table_size = 0;
                break;
        }
    } else {
        defip_table_size = (tcam_pair_count * tcam_depth * 2);
    }

    if (is_reserved) {
        max_entries = defip_table_size - (2 * max_v6_entries);
    } else {
       max_entries = defip_table_size;
    }

    if (is_reserved) {
        lpm_state_ptr = SOC_LPM128_UNRESERVED_STATE(u);
        if (NULL == lpm_state_ptr) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "reserved and unreserved state ptr is NULL\n")));
            return SOC_E_MEMORY;
        }
    }

    if (lpm_state_ptr == NULL) {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "lpm128 state ptr is NULL, just return\n")));
        return SOC_E_MEMORY; 
    }

    pfxmap = sal_alloc(defip_table_size * sizeof(int), "debug pfxmap");
    if (pfxmap == NULL) {
        return SOC_E_MEMORY;             
    } 

    sal_memset(pfxmap, -1, sizeof(int) * defip_table_size);

    free_count = SOC_LPM128_STATE_FENT(u, lpm_state_ptr, MAX_PFX128_INDEX);

    while (curr_pfx != -1) {
        if (_lpm128_find_pfx_type(u, curr_pfx, &type) < 0) {
            sal_free(pfxmap);
            return SOC_E_INTERNAL;
        }
        next_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, curr_pfx);
        if (curr_pfx == MAX_PFX128_INDEX) {
            curr_pfx = next_pfx;
            continue;
        }

        start1 = SOC_LPM128_STATE_START1(u, lpm_state_ptr, curr_pfx);
        end1 = SOC_LPM128_STATE_END1(u, lpm_state_ptr, curr_pfx);
        if (end1 == -1) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "pfx: %d end1 is -1\n"), curr_pfx));
            print_pfx_info(u, lpm_state_ptr, curr_pfx);
        }

        fent = SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx);

        start2 = SOC_LPM128_STATE_START2(u, lpm_state_ptr, curr_pfx);
        end2 = SOC_LPM128_STATE_END2(u, lpm_state_ptr, curr_pfx);

        if (type != socLpmEntryTypeV4) {
            end_idx = end1;
        } else {
            if (start2 != -1) {
                end_idx = end2;
            } else {
                end_idx = end1;
            }
        }

        if (start2 != -1) {
            if (split_count > 1) {
                 LOG_ERROR(BSL_LS_SOC_LPM,
                           (BSL_META_U(u,
                                       "More than one split. pfx: %d prev split pfx: %d\n"),
                            curr_pfx, prev_split_pfx));
                print_pfx_info(u, lpm_state_ptr, curr_pfx);
                print_pfx_info(u, lpm_state_ptr, prev_split_pfx);
            } else {
                split_count++;
                prev_split_pfx = curr_pfx;
            }
            if (end2 == -1) {
                LOG_ERROR(BSL_LS_SOC_LPM,
                          (BSL_META_U(u,
                                      "pfx: %d end2 is -1\n"), curr_pfx));
                print_pfx_info(u, lpm_state_ptr, curr_pfx);
            }

            for (i = start2; i <= end_idx; i++) {
                if (pfxmap[i] != -1) {
                    /* There is a overwrite */
                    LOG_ERROR(BSL_LS_SOC_LPM,
                              (BSL_META_U(u,
                                          "S2 onflict for pfx: %d with pfx: %d\n"),
                                          curr_pfx, pfxmap[i]));
                     
                    print_pfx_info(u, lpm_state_ptr, curr_pfx);
                    print_pfx_info(u, lpm_state_ptr, pfxmap[i]);
                } else {
                    pfxmap[i] = curr_pfx;
                }
            }
        }

        if (start2 != -1) {
            end_idx = end1;
        } else {
            end_idx = end1 + fent;
        }

        for (i = start1; i <= end_idx; i++) {
            if (type != socLpmEntryTypeV4) {
                if (((i / tcam_depth) & 1) && ((i % tcam_depth) == 0)) {
                    i += tcam_depth;
                }
            } 
            if (pfxmap[i] != -1) {
                /* There is a overwrite */
                LOG_ERROR(BSL_LS_SOC_LPM,
                          (BSL_META_U(u,
                                      "Conflict for pfx: %d with pfx: %d\n"),
                                      curr_pfx, pfxmap[i]));
                 
                print_pfx_info(u, lpm_state_ptr, curr_pfx);
                print_pfx_info(u, lpm_state_ptr, pfxmap[i]);
            } else {
                pfxmap[i] = curr_pfx;
                if (type != socLpmEntryTypeV4) {
                    pfxmap[i + 1024] = curr_pfx;
                }
            }
        }

        vent = SOC_LPM128_STATE_VENT(u, lpm_state_ptr, curr_pfx);
        if (type == socLpmEntryTypeV4) {
            vent_count += vent;
        } else {
            vent_count += (vent * 2);
        }
        free_count += SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx);
        curr_pfx = next_pfx;
    }

    if ((free_count + vent_count) != max_entries) {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "count mismatch!!! free: %d vent: %d tablesize: %d\n"),
                   free_count, vent_count, defip_table_size));
    }

    for (i = 0; i < is_reserved && max_v6_entries; i++) {
        if (pfxmap[i] != -1) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "overflow into reserved space pfx: %d\n"), 
                                  pfxmap[i]));
             
            print_pfx_info(u, lpm_state_ptr, pfxmap[i]);
        }
    }
    sal_free(pfxmap);

    return SOC_E_NONE;
}

void
dump_lpm128_prefix_info(int u)
{
    soc_lpm128_state_p lpm_state_ptr = SOC_LPM128_STATE(u);
    int start1;
    int start2;
    int end1;
    int end2;
    int vent1;
    int fent1;
    int prev;
    int next;
    int total_prefixes = 0;
    int curr_pfx = MAX_PFX128_INDEX;
 
    lpm_state_ptr = SOC_LPM128_UNRESERVED_STATE(u); 
    
    while (lpm_state_ptr && curr_pfx > 0) {
        start1 = SOC_LPM128_STATE_START1(u, lpm_state_ptr, curr_pfx);
        start2 = SOC_LPM128_STATE_START2(u, lpm_state_ptr, curr_pfx);
        end1 = SOC_LPM128_STATE_END1(u, lpm_state_ptr, curr_pfx);
        end2 = SOC_LPM128_STATE_END2(u, lpm_state_ptr, curr_pfx);
        vent1 = SOC_LPM128_STATE_VENT(u, lpm_state_ptr, curr_pfx);
        fent1 = SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx);
        prev = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, curr_pfx);
        next = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, curr_pfx);
        LOG_CLI((BSL_META_U(u,
                            "pfx: %d start1: %d start2: %d end1: %d end2: %d \n"),
                 curr_pfx, start1, start2, end1, end2));
        LOG_CLI((BSL_META_U(u,
                            "vent1: %d fent1: %d  prev: %d next: %d \n"),
                 vent1, fent1, prev, next));
        curr_pfx = next;
        total_prefixes += vent1; 
    }
    if (lpm_state_ptr) {
        LOG_CLI((BSL_META_U(u,
                            "Total number of prefixes - %d\n"), total_prefixes));
    }

    lpm_state_ptr = SOC_LPM128_STATE(u);
    if (lpm_state_ptr == NULL) {
        return;
    }  

    LOG_CLI((BSL_META_U(u,
                        "reserved prefixes=====================\n")));
    curr_pfx = MAX_PFX128_INDEX;
    while (lpm_state_ptr && curr_pfx > 0) {
        start1 = SOC_LPM128_STATE_START1(u, lpm_state_ptr, curr_pfx);
        start2 = SOC_LPM128_STATE_START2(u, lpm_state_ptr, curr_pfx);
        end1 = SOC_LPM128_STATE_END1(u, lpm_state_ptr, curr_pfx);
        end2 = SOC_LPM128_STATE_END2(u, lpm_state_ptr, curr_pfx);
        vent1 = SOC_LPM128_STATE_VENT(u, lpm_state_ptr, curr_pfx);
        fent1 = SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx);
        prev = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, curr_pfx);
        next = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, curr_pfx);
        LOG_CLI((BSL_META_U(u,
                            "pfx: %d start1: %d start2: %d end1: %d end2: %d \n"),
                 curr_pfx, start1, start2, end1, end2));
        LOG_CLI((BSL_META_U(u,
                            "vent: %d fent: %d prev: %d next: %d \n"),
                 vent1, fent1, prev, next ));
        curr_pfx = next;
        total_prefixes += vent1;
    }
    LOG_CLI((BSL_META_U(u,
                        "Total number of prefixes - %d\n"), total_prefixes));
}

STATIC
int _lpm128_v6_vent_in_curr_tcam(int u, int pfx, 
                                soc_lpm128_state_p lpm_state_ptr, int *vent)
{
    int start = 0;
    int end = 0;
    int start_tcam_number = 0;
    int end_tcam_number = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int is_reserved = 0;

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    start = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx);
    end = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
    start_tcam_number = start / tcam_depth;
    end_tcam_number =   end / tcam_depth;
    if (start_tcam_number == end_tcam_number) {
        *vent = SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx);
    } else {
        if (is_reserved) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "finding V6 vent: reserved and existing pfx %d crossed"\
                                  " TCAM boundaries\n"), pfx));
            return SOC_E_INTERNAL;
        }
        *vent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) -
                        (end_tcam_number * tcam_depth) + 1;
    }

    return SOC_E_NONE;
}

STATIC int
_lpm128_fb_entry_shift(int u, soc_lpm128_state_p lpm_state_ptr,
                       int pfx, int from_ent, int to_ent, int erase);

STATIC INLINE int
_lpm128_assign_free_fent_from_v6_to_v4(int u, int curr_pfx, int pfx, 
                                       soc_lpm128_state_p lpm_state_ptr)
{
    int is_reserved = 0;
    int start_index;
    int tcam_number = 0;
    int paired_tcam_size = 0;
    int fent_entries = 0;
    int dest_pfx = -1;
    int search_index = -1;
    int other_index = -1;
    int other_tcam_number = 0;
    int end_index = -1;
    int prev_index = -1;
    int entry_count = 0;
    int move_count = 0;
    int iter = 0;
    int rem = 0;
    int tcam_count = 0;
    int  fent_in_last_tcam = 0;
    int original_other_index = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    soc_fb_lpm_table_sizes_get(u, &paired_tcam_size, NULL);
    if (!is_reserved) {
        end_index = SOC_LPM128_STATE_END1(u, lpm_state_ptr, curr_pfx);
        fent_entries = SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx);
        SOC_IF_ERROR_RETURN(soc_fb_lpm_table_sizes_get(u, 
                                 &paired_tcam_size, NULL));

        rem = tcam_depth - (end_index + 1) % tcam_depth;
        if (tcam_depth == rem) {
            rem  = 0;
        }
        if (fent_entries >= 2  * rem) {
            fent_entries -= (2 * rem);
            tcam_count = (end_index + rem + 1) / tcam_depth;;
            if (tcam_count % 2) {
                tcam_count++;
            }

            fent_in_last_tcam = (fent_entries / 2) % tcam_depth;
            fent_entries -= (2 * fent_in_last_tcam);
            tcam_count += (fent_entries / tcam_depth);
            other_index = ((tcam_count + 1) * tcam_depth) + fent_in_last_tcam;
        } else {
            other_index = end_index + tcam_depth + (fent_entries / 2) + 1;
            fent_in_last_tcam = (fent_entries / 2);
        }

        if (other_index >= paired_tcam_size) {
            LOG_ERROR(BSL_LS_SOC_LPM, \
                      (BSL_META_U(u, \
                                  "finding up index: to_index: %d"\
                                  " paired_tcam_size: %d pfx: %d\n"), other_index, 
                       paired_tcam_size, curr_pfx));
            return SOC_E_INTERNAL;
        }
        start_index = other_index - tcam_depth;
        other_index -= fent_in_last_tcam;
        tcam_number = other_index / tcam_depth;
        search_index = tcam_number * tcam_depth - 1;
        dest_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, search_index);
        for (; dest_pfx == -1 && search_index > start_index;
             search_index--) {
            dest_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, search_index);
            if (dest_pfx == -1) {
                continue;
            }
            break;
        }

        if (dest_pfx == -1) {
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) =
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx);
        } else {
            fent_entries = SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx);
            fent_entries -= fent_in_last_tcam;
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) = fent_entries;
            move_count = fent_in_last_tcam;
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, dest_pfx) += 
                fent_in_last_tcam;
            /* if dest_pfx has start2 , then the fent given to it
             * will be above start2. So move end2 to start2 - 1 */
            start_index = SOC_LPM128_STATE_START2(u, lpm_state_ptr, 
                                                  dest_pfx);
            if (start_index != -1) {
                end_index = SOC_LPM128_STATE_END2(u, lpm_state_ptr,
                                                  dest_pfx);
                entry_count = (end_index - start_index) + 1;
                if (entry_count == 1) {
                    prev_index = end_index;
                } else {
                    prev_index = end_index - 1;
                }
                iter = move_count > entry_count ? 
                       entry_count : move_count;
                original_other_index = other_index;
                while(iter) {
                    if (prev_index < start_index) {
                        break;
                    }   
                    SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, 
                                        lpm_state_ptr, dest_pfx, 
                                        prev_index, other_index, 1));
                    other_index++;
                    iter--;
                    prev_index--;
                }
                if (entry_count > 1) {
                    if (move_count >= entry_count) {
                        SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u,
                                            lpm_state_ptr, dest_pfx, 
                                            end_index, other_index, 1));
                        SOC_LPM128_STATE_END2(u, lpm_state_ptr, 
                                              dest_pfx) = other_index;
                    } else {
                        prev_index++; 
                        SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u,
                                            lpm_state_ptr, dest_pfx, 
                                            end_index, prev_index, 1));
                        SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx) = prev_index;
                    }
                }
                if (original_other_index == SOC_LPM128_STATE_END1(u, 
                                        lpm_state_ptr, dest_pfx) + 1) {
                    SOC_LPM128_STATE_END1(u, lpm_state_ptr, dest_pfx) =
                    SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx);
                    SOC_LPM128_STATE_START2(u, lpm_state_ptr, dest_pfx) = -1;
                    SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx) = -1;
                } else {
                    SOC_LPM128_STATE_START2(u, lpm_state_ptr, dest_pfx) = 
                                            original_other_index;
                }
            }
        }
    } else {
        start_index = SOC_LPM128_STATE_END1(u, lpm_state_ptr, 
                                            curr_pfx) + 1;
        tcam_number = start_index / tcam_depth;
        if (tcam_number & 1) {
            /* falling in odd tcam */
            /*  Need not handle the reserved case, because if 
             *  V6 reservation is on, then we should not enter
             *  here
             */
            start_index += tcam_depth;
            tcam_number = start_index / tcam_depth;
        }
        other_index = start_index + tcam_depth;
        other_tcam_number = other_index / tcam_depth;
        if (other_tcam_number & 1) {
            search_index = (other_tcam_number * tcam_depth) - 1;
        } else {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "creating new pfx group: other_index: %d not"\
                                  " in odd tcam pfx: %d curr_pfx: %d\n"), 
                       other_index, pfx, curr_pfx));
            return SOC_E_INTERNAL;
        }

        dest_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, search_index);
        for (; dest_pfx == -1 && search_index > start_index;
             search_index--) {
            dest_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, search_index);
            if (dest_pfx == -1) {
                continue;
            }
            break;
        }

        if (dest_pfx == -1) {
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) =
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx);
        } else {
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) = 
            (SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx) / 2);
            move_count = (SOC_LPM128_STATE_FENT(u, lpm_state_ptr,
                                                curr_pfx) / 2);
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, dest_pfx) += 
            (SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx) / 2);
            /* if dest_pfx has start2 , then the fent given to it
             * will be above start2. So move end2 to start2 - 1 */
            start_index = SOC_LPM128_STATE_START2(u, lpm_state_ptr, 
                                                  dest_pfx);
            if (start_index != -1) {
                end_index = SOC_LPM128_STATE_END2(u, lpm_state_ptr,
                                                  dest_pfx);
                entry_count = (end_index - start_index) + 1;
                if (entry_count == 1) {
                    prev_index = end_index;
                } else {
                    prev_index = end_index - 1;
                }
                iter = move_count > entry_count ? 
                       entry_count : move_count;
                while(iter) {
                    if (prev_index < start_index) {
                        break;
                    }   
                    SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, 
                                        lpm_state_ptr, dest_pfx, 
                                       prev_index, other_index, 1));
                    other_index++;
                    iter--;
                    prev_index--;
                }
                if (entry_count > 1) {
                    if (move_count >= entry_count) {
                        SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u,
                                            lpm_state_ptr, dest_pfx, 
                                            end_index, other_index, 1));
                    SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx) = other_index;
                    } else {
                        prev_index++; 
                        SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u,
                                            lpm_state_ptr, dest_pfx, 
                                            end_index, prev_index, 1));
                        SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx) = prev_index;
                    }
                }
                SOC_LPM128_STATE_START2(u, lpm_state_ptr, dest_pfx) = 
                SOC_LPM128_STATE_END1(u, lpm_state_ptr, curr_pfx) + 
                tcam_depth + 1;
            }
        }
    }
    return SOC_E_NONE;
}

STATIC INLINE int
_lpm128_create_new_pfx_group(int u, int pfx, soc_lpm_entry_type_t type, 
                             soc_lpm128_state_p lpm_state_ptr)
{
    int is_reserved = 0;
    int existing_pfx = -1;    
    int start_index = -1;
    int next_pfx = -1;
    int curr_pfx = -1; 
    int tcam_number = 0;
    int paired_tcam_size = 0;
    int fent_entries = 0;
    int existing_vent = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);
    soc_lpm_entry_type_t curr_pfx_type = socLpmEntryType128BV6;

    /*
     * Find the  prefix position. Only prefix with valid
     * entries are in the list.
     * next -> high to low prefix. low to high index
     * prev -> low to high prefix. high to low index
     * Unused prefix length MAX_PFX_INDEX is the head of the
     * list and is node corresponding to this is always
     * present.
     */
    if (lpm_state_ptr == NULL) {
        return SOC_E_PARAM;
    }
  
    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        max_v6_entries >>= 1;
    }

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }
  
    soc_fb_lpm_table_sizes_get(u, &paired_tcam_size, NULL); 
    curr_pfx = MAX_PFX128_INDEX;

    while (SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, curr_pfx) > pfx) {
        curr_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, curr_pfx);
    }
    /* Insert the new prefix */
    next_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, curr_pfx);
    if (next_pfx != -1) {
        SOC_LPM128_STATE_PREV(u, lpm_state_ptr, next_pfx) = pfx;
    }
    /* Setup the entry in the linked list */
    SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, pfx) =
        SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, curr_pfx);
    SOC_LPM128_STATE_PREV(u, lpm_state_ptr, pfx) = curr_pfx;
    SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, curr_pfx) = pfx;
    /* If the curr_pfx is V4, then check if it is split. 
     * If yes, then the pfx which is lesser than this one must also be V4
     * It must reside after the second range of the curr_pfx 
     */
    if (type == socLpmEntryTypeV4) {
        SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, curr_pfx, &curr_pfx_type));
        if (curr_pfx == MAX_PFX128_INDEX || 
            !_lpm128_if_conflicting_type(u, type, curr_pfx_type)) {
        
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) =
                SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx);
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx) = 0;
        } else {
            /* If curr_pfx is V6, and has free entries then do the following 
             * find the prefix which can take half of the entries.
             * If there is one and if it falls in the same tcam as pfx, then
             * split entries among both. Otherwise give all free entries to
             * pfx.
             */
            if (SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx)) {
                if (next_pfx == -1) {
                    SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) =
                        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx);
                } else {
                    _lpm128_assign_free_fent_from_v6_to_v4(u, curr_pfx, pfx, 
                                                           lpm_state_ptr);
                }
            }
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx) = 0;
        }

        if (SOC_LPM128_STATE_START2(u, lpm_state_ptr, curr_pfx) != -1) {
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) =
                SOC_LPM128_STATE_END2(u, lpm_state_ptr, curr_pfx) + 1;
        } else {    
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) =
                SOC_LPM128_STATE_END1(u, lpm_state_ptr, curr_pfx) + 1;
        }         
 
        /*
         * V4 prefixes: There are 2 cases where start of a new prefix group has 
         * to be adjusted. 
         * a) when 128B V6 space is reserved
         *    Solution: set the offset by max_v6_entries 
         * b) when the start falls into the odd tcam and the other 
              corresponding entry has 128B V6 address. 

         * Case 1: start address of pfx is in reserved space.
         * This is possible only if the start index is multiple of 1024. 
         * Otherwise the previous prefix group was also in reserved space.
         * Solution is to add max 128B entries to start 
         */
        start_index =  SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx);
        tcam_number = start_index / tcam_depth;
        if ((tcam_number & 1) && (start_index % tcam_depth == 0)) {
           if (is_reserved) { 
               start_index += (max_v6_entries % tcam_depth);
           }
           /*
            * case 2: At the start address index, there is some other V6 prefix.
            * Solution: get the prefix, simply add vent number of entries.   
            */ 
           if (start_index < paired_tcam_size) {
               existing_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, start_index);
           }
            while ((start_index < paired_tcam_size) && (existing_pfx > pfx)) {
                /*
                 * existing_pfx > pfx check is required because if there is a lower 
                 * prefix, then it would be moved down during move operation.
                 */
                if (SOC_LPM128_PFX_IS_V4(u, existing_pfx)) {
                    break;
                }
                fent_entries = SOC_LPM128_STATE_FENT(u, lpm_state_ptr, 
                                                       existing_pfx);
                _lpm128_v6_vent_in_curr_tcam(u, existing_pfx, lpm_state_ptr,
                                             &existing_vent);
                start_index += existing_vent + (fent_entries / 2);
                if (start_index < paired_tcam_size)  {
                    existing_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, start_index);
                }
            }
            
            if (start_index > paired_tcam_size) {
                LOG_ERROR(BSL_LS_SOC_LPM, \
                          (BSL_META_U(u, \
                                      "create new group, start_index(%d) "\
                                      "greater than paired_table_size(%d) for pfx %d\n"),
                           start_index, paired_tcam_size, pfx));
                return SOC_E_INTERNAL;
            }
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = start_index;
        }
        SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) =
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) - 1;
    } else {
        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) = 
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx);
        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, curr_pfx) = 0;
        SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) =
            SOC_LPM128_STATE_END1(u, lpm_state_ptr, curr_pfx) + 1;

        start_index = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx);
        /*
         * since we are managing indices interms on unpaired TCAM, 
         * it is possible that the start we got could be in odd tcam 
         * which is already in use. so if the start falls in odd tcam, 
         * skip the tcam. It is safe to add tcam_depth to start because 
         * there is no known case where the start will not be the first 
         * index of the odd tcam. 
         */
        tcam_number = start_index / tcam_depth;
        if (tcam_number & 1) {
            if (type == socLpmEntryType64BV6 && is_reserved) {
                if (SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx)) {
                    LOG_ERROR(BSL_LS_SOC_LPM,
                              (BSL_META_U(u,
                                          "create new pfx group: pfx: %d crossing tcam and"\
                                          " still has fent\n"), pfx));
                    return SOC_E_INTERNAL;
                }
            } else {
                SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = 
                                        (tcam_number + 1) * tcam_depth;
            }
        }

        SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) =
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) - 1;
        SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) = 0;
    }
    return SOC_E_NONE;
}


/*
 *      Create a slot for the new entry rippling the entries if required
 */
STATIC int
_lpm128_fb_entry_shift(int u, soc_lpm128_state_p lpm_state_ptr, 
                       int pfx, int from_ent, int to_ent, int erase)
{
    uint32 v0 = 0;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS] = {0};
    uint32      eupr[SOC_MAX_MEM_FIELD_WORDS] = {0};
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);

#ifdef FB_LPM_TABLE_CACHED
    SOC_IF_ERROR_RETURN(soc_mem_cache_invalidate(u, L3_DEFIPm,
                                       MEM_BLOCK_ANY, from_ent));
#endif /* FB_LPM_TABLE_CACHED */
    
    SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e));
    if (!SOC_LPM128_PFX_IS_V4(u, pfx)) {
        SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, 
                                           from_ent + tcam_depth, eupr));
        LPM128_HASH_INSERT(u, e, eupr, to_ent);
        /* Disable valid bit at to_ent + tcam_depth so that packets dont
         * get forwared to some transient addresses
         */
        v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, eupr, VALID0f);
        if (v0) {
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, eupr, VALID0f, 0);
            SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY,
                                                to_ent + tcam_depth, eupr));
        }
    } else {
        LPM128_HASH_INSERT(u, e, NULL, to_ent);
    }
    SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, to_ent, e));
    SOC_LPM128_INDEX_TO_PFX_GROUP(u, to_ent) = pfx;
    if (!SOC_LPM128_PFX_IS_V4(u, pfx)) {
        if (v0) {
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, eupr, VALID0f, v0);
        }
        SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, 
                                            to_ent + tcam_depth, eupr));
        SOC_LPM128_INDEX_TO_PFX_GROUP(u, to_ent + tcam_depth) = pfx;
        SOC_IF_ERROR_RETURN(_lpm128_fb_urpf_entry_replicate(u, to_ent, e, 
                                                            eupr));
    } else {
        SOC_IF_ERROR_RETURN(_lpm128_fb_urpf_entry_replicate(u, to_ent, e, NULL));
    }

    if (erase) {
        sal_memcpy(e, soc_mem_entry_null(u, L3_DEFIPm),
                   soc_mem_entry_words(u,L3_DEFIPm) * 4);
        SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e));
        SOC_LPM128_INDEX_TO_PFX_GROUP(u, from_ent) = -1;
        if (!SOC_LPM128_PFX_IS_V4(u, pfx)) {
            SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, 
                                from_ent + tcam_depth, e));
            SOC_LPM128_INDEX_TO_PFX_GROUP(u, from_ent + tcam_depth) = -1;
            LPM128_HASH_INSERT(u, e, e, from_ent);
            SOC_IF_ERROR_RETURN(_lpm128_fb_urpf_entry_replicate(u, from_ent, e, 
                                                                e));
        } else {
            LPM128_HASH_INSERT(u, e, NULL, from_ent);
            SOC_IF_ERROR_RETURN(_lpm128_fb_urpf_entry_replicate(u, from_ent, e, 
                                                                NULL));
        }
    }
    return (SOC_E_NONE);
}


STATIC int
_lpm128_fb_v4_entry_shift_up(int u, soc_lpm128_state_p lpm_state_ptr,
                          int pfx, int start_ent, int end_ent, int to_ent)
{
    uint32      e[SOC_MAX_MEM_FIELD_WORDS] = {0};
    uint32      v0 = 0, v1 = 0;
    int from_ent = end_ent;

#ifdef FB_LPM_TABLE_CACHED
    SOC_IF_ERROR_RETURN(soc_mem_cache_invalidate(u, L3_DEFIPm,
                                                 MEM_BLOCK_ANY, from_ent));
#endif /* FB_LPM_TABLE_CACHED */
    SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e));
    v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
    v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);

    if (v0 == 0 || v1 == 0) {
       /* Last entry is half full -> keep it last. */
        LPM128_HASH_INSERT(u, e, NULL, to_ent);
        SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, to_ent, e));
        SOC_IF_ERROR_RETURN(_lpm128_fb_urpf_entry_replicate(u, to_ent, e, NULL));
        SOC_LPM128_INDEX_TO_PFX_GROUP(u,to_ent) = pfx;
        to_ent = from_ent;
    }

    from_ent = start_ent;

    if (to_ent != from_ent) {
        SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr, pfx,
                                                   from_ent, to_ent, 0));
    }

    return SOC_E_NONE;
}

STATIC int
_lpm128_fb_v4_entry_shift_down(int u, soc_lpm128_state_p lpm_state_ptr,
                          int pfx, int prev_ent, int end_ent, int to_ent, 
                          int entry_count, int erase)
{
    uint32      e[SOC_MAX_MEM_FIELD_WORDS] = {0};
    uint32      v0 = 0, v1 = 0;
    int from_ent = end_ent;

#ifdef FB_LPM_TABLE_CACHED
    SOC_IF_ERROR_RETURN(soc_mem_cache_invalidate(u, L3_DEFIPm,
                                                 MEM_BLOCK_ANY, from_ent));
#endif /* FB_LPM_TABLE_CACHED */
    SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e));
    v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
    v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);

    if (v0 == 0 || v1 == 0) {
       /* Last entry is half full -> keep it last. */
        if (entry_count > 1) {
            SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr, 
                                                   pfx, prev_ent, to_ent, 0));
            LPM128_HASH_INSERT(u, e, NULL, prev_ent); 
            SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, prev_ent, e));
            SOC_IF_ERROR_RETURN(_lpm128_fb_urpf_entry_replicate(u, prev_ent, 
                                                                e, NULL));
            SOC_LPM128_INDEX_TO_PFX_GROUP(u,prev_ent) = pfx;
        } else {
            SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr, 
                                               pfx, from_ent, to_ent, erase));
        } 
    } else {
        SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr,
                                               pfx, from_ent, to_ent, erase));
    }
    
    return SOC_E_NONE;
}

STATIC int
soc_fb_lpm128_get_smallest_v6_prefix(int u,
                                         soc_lpm128_state_p lpm_state_ptr,
                                         int *pfx_id)
{
    int pfx = MAX_PFX128_INDEX;
    int found = 0;

    while(pfx != -1) {
        pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, pfx);
        if ((pfx == -1) || SOC_LPM128_PFX_IS_V4(u, pfx)) {
            break;
        }
        *pfx_id = pfx;
        found = 1;
    }
    if (found == 0) {
       return SOC_E_NOT_FOUND;
    }
    return SOC_E_NONE;
}

STATIC int
_lpm128_find_new_index_from_v6_group(int u, int pfx, 
                                     soc_lpm128_state_p lpm_state_ptr, 
                                     int *index)
{
    int pfx_id = -1;
    int rv = SOC_E_NONE;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int paired_tcam_size = 0;
    int fent = 0;
    int end = 0;
    int to_index = 0;
    int tcam_count = 0;
    int rem = 0;
    int fent_in_last_tcam = 0;
    int to_ent_tcam_number = 0;
    int to_index_tcam_number = 0;

    rv = soc_fb_lpm128_get_smallest_v6_prefix(u, lpm_state_ptr, &pfx_id);
    if (rv == SOC_E_NOT_FOUND) {
        return SOC_E_NONE;
    }
    
    end = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx_id);
    fent = SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx_id);

    SOC_IF_ERROR_RETURN(soc_fb_lpm_table_sizes_get(u, &paired_tcam_size, NULL));

    rem = tcam_depth - (end + 1) % tcam_depth;
    if (tcam_depth == rem) {
        rem  = 0;
    }
    if (fent >= 2  * rem) {
        fent -= (2 * rem);
        tcam_count = (end + rem + 1) / tcam_depth;;
        if (tcam_count % 2) {
            tcam_count++;
        }

        fent_in_last_tcam = (fent / 2) % tcam_depth;
        fent -= (2 * fent_in_last_tcam);
        tcam_count += (fent / tcam_depth);
        to_index = ((tcam_count + 1) * tcam_depth) + fent_in_last_tcam;
    } else {
        to_index = end + tcam_depth + (fent / 2) + 1;
    }

    if (to_index >= paired_tcam_size) {
        LOG_ERROR(BSL_LS_SOC_LPM, \
                  (BSL_META_U(u, \
                              "finding up index: to_index: %d"\
                              " paired_tcam_size: %d pfx: %d\n"), to_index, paired_tcam_size, pfx));
        return SOC_E_INTERNAL;
    }

    to_ent_tcam_number = (*index) / tcam_depth;
    to_index_tcam_number = to_index / tcam_depth;

    if (to_ent_tcam_number == to_index_tcam_number) {
        *index = to_index;
    }

   if (to_index_tcam_number > to_ent_tcam_number) {
       LOG_ERROR(BSL_LS_SOC_LPM, \
                 (BSL_META_U(u, \
                             "finding up index: lowest v6 is at higher index "\
                             "than pfx: %d to_index: %d index: %d\n"), pfx, to_index, *index));
       return SOC_E_INTERNAL;
   }
   return SOC_E_NONE;
}

STATIC int
_lpm128_get_next_up_index(int u, int pfx, soc_lpm128_state_p lpm_state_ptr, 
                       int *index)
{

    int is_reserved = 0;
    int existing_pfx = -1;
    int paired_tcam_size = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);
    int existing_vent = 0;
    int to_ent = 0;
    int old_to_ent = 0;
    soc_lpm_entry_type_t type = socLpmEntryTypeV4;

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        max_v6_entries >>= 1;
    }

    SOC_IF_ERROR_RETURN(soc_fb_lpm_table_sizes_get(u, &paired_tcam_size, NULL));
    SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, pfx, &type));

    to_ent = *index;
    to_ent++;
    old_to_ent = to_ent;
    if (((to_ent / tcam_depth) & 1) && (to_ent % tcam_depth == 0)) {
        if (type == socLpmEntryType64BV6) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "finding next index: FATAL 64B crossing boundary pfx: %d\n"), pfx));
            return SOC_E_INTERNAL;
        }
        if ((type == socLpmEntryTypeV4) && is_reserved) {
            to_ent += (max_v6_entries % tcam_depth);
        }
        if (type == socLpmEntryType128BV6) {
            to_ent += tcam_depth;
            *index = to_ent;
            return SOC_E_NONE;
        }
     
        existing_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, to_ent);
        while ((existing_pfx != -1) && (to_ent < paired_tcam_size)) {
            if (SOC_LPM128_PFX_IS_V4(u, existing_pfx) && (existing_pfx > pfx)) {
                LOG_ERROR(BSL_LS_SOC_LPM,
                          (BSL_META_U(u,
                                      "finding next index: FATAL existing_pfx: %d pfx: %d\n"),
                           existing_pfx, pfx));
                return SOC_E_INTERNAL;
            } else if (SOC_LPM128_PFX_IS_V4(u, existing_pfx)) {
                break;
            }

            /*
             * Here are 2 different cases
             * a) start and end fall in same tcam=> just add vent
             * b) start and end in different tcam=> add number of entries in the
             *    current tcam
             */
            _lpm128_v6_vent_in_curr_tcam(u, existing_pfx, lpm_state_ptr,
                                         &existing_vent);
            to_ent += existing_vent + (SOC_LPM128_STATE_FENT(u,
                                        lpm_state_ptr, existing_pfx)) / 2;
            existing_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, to_ent);
        }

        if (to_ent >= paired_tcam_size) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "finding next index: did not find free entries during move for"\
                                  " pfx: %d existing_pfx: %d!!!\n"), pfx, existing_pfx));
            return SOC_E_INTERNAL;
        }
        if (old_to_ent == to_ent) {
            _lpm128_find_new_index_from_v6_group(u, pfx, lpm_state_ptr, 
                                                 &to_ent);
        }
    }
    *index = to_ent;

    return SOC_E_NONE;
}

STATIC int
_lpm128_fb_shift_v4_pfx_up(int u, soc_lpm128_state_p lpm_state_ptr, int pfx)
{
    uint32      e[SOC_MAX_MEM_FIELD_WORDS] = {0};
    int         from_ent = -1;
    int         to_ent = -1;
    uint32      v0 = 0, v1 = 0;
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u); 
    int entry_count = 0;
    int from_end2 = 0;
    int crossed_boundary = 0;
    int to_ent_actual = 0;
    int paired_tcam_size = 0;
    int tcam_pair_count = 0;

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        max_v6_entries >>= 1;
    }

    SOC_IF_ERROR_RETURN(soc_fb_lpm_tcam_pair_count_get(u, &tcam_pair_count));
    soc_fb_lpm_table_sizes_get(u, &paired_tcam_size, NULL);

    if (SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) != -1) {
        from_ent = SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx);
        from_end2 = 1;
    } else {
        from_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
        from_end2 = 0;
    }
    entry_count = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) -
                  SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) + 1;
    to_ent = from_ent;
    SOC_IF_ERROR_RETURN(_lpm128_get_next_up_index(u, pfx, lpm_state_ptr, 
                                                  &to_ent));
    if (to_ent - from_ent > 1) {
        crossed_boundary = 1;
    }
    
    if (from_end2 && crossed_boundary) {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "V4 move up: end2 crossed boundary for pfx: %d from_ent: %d"\
                              " to_ent: %d\n"), pfx, from_ent, to_ent));
        return SOC_E_INTERNAL; 
    }
#ifdef FB_LPM_TABLE_CACHED
    SOC_IF_ERROR_RETURN(soc_mem_cache_invalidate(u, L3_DEFIPm,
                                                 MEM_BLOCK_ANY, from_ent));
#endif /* FB_LPM_TABLE_CACHED */
    SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e));
    v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
    v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);

    to_ent_actual = to_ent;
    if ((v0 == 0) || (v1 == 0)) {
        /* Last entry is half full -> keep it last. */
        LPM128_HASH_INSERT(u, e, NULL, to_ent);
        SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, to_ent, e));
        SOC_IF_ERROR_RETURN(_lpm128_fb_urpf_entry_replicate(u, to_ent, e, NULL));
        SOC_LPM128_INDEX_TO_PFX_GROUP(u,to_ent) = pfx;   
        to_ent = from_ent;
    }

    from_ent = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx);

    if (from_ent < 0) {
       LOG_ERROR(BSL_LS_SOC_LPM,
                 (BSL_META_U(u,
                             "V4 move up: Invalid start start1: %d pfx: %d\n"), 
                             from_ent, pfx));
       return SOC_E_INTERNAL; 
    }
   
    if (to_ent != from_ent) {
        SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr, pfx, 
                                                   from_ent, to_ent, 0));
    }
   
    if (from_end2) {
        SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) += 1;
        if (entry_count == 1) {
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = 
            SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx);
            SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = 
            SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx);
            SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = -1;
            SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = -1;
        } else {
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) += 1;
        }
    } else {
        if (crossed_boundary) {
            if (entry_count == 1) {
               SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) =
                                                                  to_ent_actual;
                SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) =
                                                                  to_ent_actual;
            } else {
               SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) =
                                                                  to_ent_actual;
                SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) =
                                                                  to_ent_actual;
                SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) += 1;
            }
        } else {
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) += 1;
            SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) += 1;
        }
    }
    return SOC_E_NONE;
}

/*
 *      Shift prefix entries 1 entry UP, while preserving
 *      last half empty IPv4 entry if any.
 */
STATIC int 
_lpm128_fb_shift_pfx_up(int u, soc_lpm128_state_p lpm_state_ptr, int pfx)
{
    int         from_ent;
    int         to_ent;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int tcam_number = 0;
    int is_reserved = 0;
    
    if (SOC_LPM128_PFX_IS_V4(u, pfx)) {
        return _lpm128_fb_shift_v4_pfx_up(u, lpm_state_ptr, pfx);   
    }

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }
  
    to_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) + 1;
    /*
     * since we are managing indices interms on unpaired TCAM,
     * it is possible that the start we got could be in odd tcam
     * which is already in use. so if the start falls in odd tcam,
     * skip the tcam. It is safe to add tcam_depth to start because
     * there is no known case where the start will not be the first
     * index of the odd tcam.
     */
    tcam_number = to_ent / tcam_depth;
    if ((tcam_number & 1) && (to_ent % tcam_depth == 0)) {
        if (!is_reserved || SOC_LPM128_PFX_IS_V6_128(u, pfx)) {
            to_ent += tcam_depth;
        } else if(SOC_LPM128_PFX_IS_V6_64(u, pfx)) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "moving entries up: type 64B, crossing tcam boundary for"\
                                  " pfx - %d\n"), pfx));  
            return SOC_E_INTERNAL;
        }
    }

    from_ent = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx);
    if(from_ent != to_ent) {
        SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr,
                                          pfx, from_ent, to_ent, 0));
    }
    SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) += 1;
    tcam_number = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) / 
                      tcam_depth; 
    if (tcam_number & 1) {
        if (!is_reserved || SOC_LPM128_PFX_IS_V6_128(u, pfx)) {
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) += tcam_depth;
        } else if (SOC_LPM128_PFX_IS_V6_64(u, pfx)) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "%s: 64B START in odd tcam in reserved state for pfx - %d\n"), 
                       FUNCTION_NAME(), pfx));
            return SOC_E_INTERNAL;
        }
    }
    SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = to_ent;
    return (SOC_E_NONE);
}


STATIC int
_lpm128_fb_shift_v4_pfx_down(int u, soc_lpm128_state_p lpm_state_ptr, 
                             int pfx, int erase) 
{
    int from_ent;
    int from_end2;
    int prev_ent;
    int v0, v1;
    int is_reserved = 0;
    int prev_pfx = 0;
    int crossed_boundary = 0;
    int entry_count = 0;
    int tcam_number = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);
    int prev_end = -1;
    int prev_tcam_number = -1;
    int traverse_index = -1;
    int old_traverse_index = -1;
    int dest_pfx = -1;
    int existing_vent = 0;
    int to_ent = -1;
    soc_lpm_entry_type_t prev_pfx_type;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS] = {0};
  
    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        max_v6_entries >>= 1;
    }
    prev_pfx = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, pfx);
    if (prev_pfx == -1) {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "v4 move down: previous pfx is -1 for pfx: %d\n"), pfx));
        return SOC_E_INTERNAL;
    }
    
    to_ent = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) - 1;
    tcam_number = to_ent / tcam_depth;  
    if (tcam_number & 1) {
        /* This is odd TCAM. Take care of the following cases
         * Check if we need to adjust the to_ent
         * 1. prev_pfx is V6, then do to_ent -= tcam_depth to get to_ent
         * 2. if prev_pfx is v4, then get find start1 + 1024. If to_ent less
              than or equal to prev_start1 + 1024, we then no need to change  
         *    to_ent.
         * 3. if prev_pfx is in lower index tcam, then end1 = tcam * tcam_depth -1,
         * then        
         */
        if (is_reserved && 
               ((to_ent % tcam_depth) < (max_v6_entries % tcam_depth))) {
               to_ent -= (max_v6_entries % tcam_depth);
        } else if (SOC_LPM128_PFX_IS_V4(u, prev_pfx)) {
            if (SOC_LPM128_STATE_START2(u, lpm_state_ptr, prev_pfx) == -1) {
               prev_end = SOC_LPM128_STATE_END1(u, lpm_state_ptr, prev_pfx);
               prev_tcam_number = prev_end / tcam_depth; 
                
               if (prev_tcam_number < tcam_number) {
                   traverse_index = tcam_number * tcam_depth;
                   old_traverse_index = traverse_index;
                   if (is_reserved) {
                      traverse_index += (max_v6_entries % tcam_depth);
                   }
                   while (traverse_index < (to_ent + 1)) {
                       dest_pfx =  SOC_LPM128_INDEX_TO_PFX_GROUP(u, 
                                                      traverse_index);
                       if (dest_pfx == -1) {
                           break;
                       }
                       if (!SOC_LPM128_PFX_IS_V4(u, dest_pfx)) {
                           _lpm128_v6_vent_in_curr_tcam(u, dest_pfx, 
                                                        lpm_state_ptr, 
                                                        &existing_vent);
                           traverse_index += existing_vent +
                                (SOC_LPM128_STATE_FENT(u, lpm_state_ptr, 
                                                          dest_pfx) / 2);
                                continue;
                       }
                       break;
                   }
                   if (old_traverse_index == traverse_index) {
                       _lpm128_find_new_index_from_v6_group(u, pfx, 
                                                            lpm_state_ptr, 
                                                            &traverse_index);
                   }
                   if (traverse_index > (to_ent + 1)) {
                      LOG_ERROR(BSL_LS_SOC_LPM,
                                (BSL_META_U(u,
                                            "v4 moving down, to_ent(%d) not free pfx: %d"\
                                            " prev_pfx: %d\n"), to_ent, pfx, prev_pfx));
                      return SOC_E_INTERNAL; 
                   }
                   if (traverse_index == to_ent + 1) {
                        to_ent = (tcam_number * tcam_depth) - 1;
                   }
               }
            }
        } else {
            to_ent = (tcam_number * tcam_depth) - 1;
        }
    }

    if (to_ent != SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) - 1) {
        crossed_boundary = 1;
    }

    if (SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) != -1) {
        from_end2 = 1;
        from_ent = SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx);
        entry_count = from_ent -
                      SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) + 1;
        crossed_boundary = 1;
    } else {
        from_end2 = 0;
        from_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
        entry_count = from_ent -
                      SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) + 1;
         
    }
    
    /* Check if vent is 0. Then no need to move, just update start and end */
    /* This case is hit if the prefix group is created just now */ 
    if (SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) == 0) {
        SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = to_ent;
        SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = to_ent - 1;
        SOC_LPM128_INDEX_TO_PFX_GROUP(u, to_ent) = pfx;
        return SOC_E_NONE;
    }

#ifdef FB_LPM_TABLE_CACHED
    SOC_IF_ERROR_RETURN(soc_mem_cache_invalidate(u, L3_DEFIPm,
                                                 MEM_BLOCK_ANY, from_ent));
#endif /* FB_LPM_TABLE_CACHED */
    SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e));
    v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
    v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);

    if ((v0 == 0) || (v1 == 0)) {
        /* Last entry is half full -> keep it last. */
        /* Shift entry before last to start - 1 position. */
        if (from_end2) {
            if(entry_count == 1) {
                prev_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
            } else {
                prev_ent = SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) - 1;
            }
        } else {
            if (entry_count == 1) {
                prev_ent = from_ent;
            } else {
                prev_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) - 1;
            }
        }

        SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr,
                                           pfx,
                                           prev_ent,
                                           to_ent, 
                                           prev_ent == from_ent ? erase : 0));

        if (prev_ent != from_ent) {
            SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr,
                                                   pfx,
                                                   from_ent,
                                                   prev_ent, erase));
        }
    } else {
        /* Last entry is full -> just shift it to start - 1  position. */
        SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr, 
                                             pfx,
                                             from_ent, 
                                             to_ent, erase));
    }

    /*
     * if pfx is V4 and prev_pfx is V6, then V6 needs to give other index
     * to another V4 pfx group. If that group is this pfx, then it will have
     * more than 1 fent after this move. So in next move, the next pfx writes
     * to last fent. but there is duplicate at end + 1. So delete it here
     */

    prev_pfx = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, pfx); 
    if (prev_pfx != -1) {
        SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, prev_pfx, &prev_pfx_type));
        if (_lpm128_if_conflicting_type(u, socLpmEntryTypeV4, prev_pfx_type)) {
            sal_memcpy(e, soc_mem_entry_null(u, L3_DEFIPm),
                       soc_mem_entry_words(u,L3_DEFIPm) * 4);
            LPM128_HASH_INSERT(u, e, NULL, from_ent);
            SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e));
            SOC_LPM128_INDEX_TO_PFX_GROUP(u, from_ent) = -1;
        }
    }

    if (from_end2) {
        SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) -= 1;
        if (entry_count == 1) { 
            SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = -1;
            SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = -1;
        } else { 
            SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) -= 1;
        }
    } else {
        if (crossed_boundary) {
            if (entry_count == 1) {
                SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = to_ent;
                SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = to_ent;
            } else {
                SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) =
                SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx);  
                SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = to_ent;   
                SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) =
                SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) - 1;
                SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = to_ent;  
            } 
        } else {
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) -= 1;
            SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) -= 1;
        }
    }
    return SOC_E_NONE;
}

/*
 *      Shift prefix entries 1 entry DOWN, while preserving
 *      last half empty IPv4 entry if any.
 */
STATIC int 
_lpm128_fb_shift_pfx_down(int u, soc_lpm128_state_p lpm_state_ptr, int pfx, 
                          int erase)
{
    int         from_ent;
    int         to_ent  = -1;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int tcam_number = 0;
    int is_reserved = 0;

    if (SOC_LPM128_PFX_IS_V4(u, pfx)) {
        return _lpm128_fb_shift_v4_pfx_down(u, lpm_state_ptr, pfx, 
                                            erase);
    } 

    /*
     * if you are moving V6 entries down, we are sure that all the above 
     * entries are V6 only 
     */
    to_ent = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) - 1;
    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    tcam_number = to_ent / tcam_depth;
    if (tcam_number & 1) {
        if ((!is_reserved) || SOC_LPM128_PFX_IS_V6_128(u, pfx)) {
            to_ent -= tcam_depth;
        } else if (SOC_LPM128_PFX_IS_V6_64(u, pfx)) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "moving down: 64BV6 falling in odd TCAM  pfx: %d\n"), pfx)); 
            return SOC_E_INTERNAL;
        }
    }

    /* Don't move empty prefix . 
     * This condition is hit if pfx is created for the first time 
     */
    if (SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) == 0) {
        SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = to_ent;
        SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = to_ent - 1;
        SOC_LPM128_INDEX_TO_PFX_GROUP(u, to_ent) = pfx;
        return (SOC_E_NONE);
    }

    from_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
    SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr,
                                               pfx, from_ent,
                                               to_ent, erase));

    SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = to_ent;
    
    SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) -= 1;
    tcam_number = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) / tcam_depth;
    if (tcam_number & 1) {
        if (!is_reserved || SOC_LPM128_PFX_IS_V6_128(u, pfx)) {
            SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) -= tcam_depth;
        } else if (SOC_LPM128_PFX_IS_V6_64(u, pfx)) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "%s: END TCAM boundary crossing for 64B entries for pfx - %d\n"),
                       FUNCTION_NAME(), pfx));
            return SOC_E_INTERNAL;
        }
    }
   
    return (SOC_E_NONE);
}

STATIC int
_lpm128_v4_half_entry_available(int u, soc_lpm128_state_p lpm_state_ptr, 
                                int pfx, void *e, int *free_slot)
{
    int from_ent;
    uint32 v0, v1;
    int rv;

    if (SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) != -1) {
        from_ent = SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx);
    } else {
        from_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
    }

    if (from_ent == -1) {
        return SOC_E_INTERNAL; 
    }

#ifdef FB_LPM_TABLE_CACHED
    if ((rv = soc_mem_cache_invalidate(u, L3_DEFIPm,
                                       MEM_BLOCK_ANY, from_ent)) < 0) {
        return rv;
    }
#endif /* FB_LPM_TABLE_CACHED */
    if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e)) < 0) {
        return rv;
    }
    v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
    v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);

    if ((v0 == 0) || (v1 == 0)) {
        *free_slot = (from_ent << 1) + ((v1 == 0) ? 1 : 0);
        return(SOC_E_NONE);
    }
    return SOC_E_FULL; /* To indicate that both the entries are in use*/
}


STATIC int
_lpm128_free_slot_move_down(int u, int pfx, int free_pfx, 
                            soc_lpm128_state_p lpm_state_ptr,
                            int erase)
{
    int next_pfx = -1;
    soc_lpm_entry_type_t next_pfx_type = socLpmEntryTypeV4;
    soc_lpm_entry_type_t free_pfx_type = socLpmEntryTypeV4;
    int tcam_number = 0;
    int search_index = 0;
    int dest_pfx = 0;
    int fent_incr = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int entry_count = 0;
    int start_index = -1;
    int other_index = -1;
    int end_index = -1;
    int prev_index = -1;

    while (free_pfx > pfx) {
        /* copy start of prev_pfx to start - 1 */  
        next_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, free_pfx);
        SOC_IF_ERROR_RETURN(_lpm128_fb_shift_pfx_down(u, lpm_state_ptr,
                                                      next_pfx, erase));
        
        SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, next_pfx, &next_pfx_type));
        SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, free_pfx, &free_pfx_type));

        /* V6 to V6 case  or V4 to V4 case */
        if (next_pfx_type != socLpmEntryTypeV4) {
            fent_incr = 2;
        } else {
            fent_incr = 1;
        }

        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, free_pfx) -= fent_incr;
        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, next_pfx) += fent_incr;

        if (SOC_LPM128_STATE_FENT(u, lpm_state_ptr, free_pfx) && 
            _lpm128_if_conflicting_type(u, free_pfx_type, next_pfx_type)) {
            if (free_pfx_type == socLpmEntryTypeV4) {
                LOG_ERROR(BSL_LS_SOC_LPM,
                          (BSL_META_U(u,
                                      "moving entries down: wrong move sequence free_pfx: %d "\
                                      "next_pfx: %d pfx: %d\n"), free_pfx, next_pfx, pfx));
                return SOC_E_INTERNAL;
            }
            /* next_pfx is v4 and got a free index for V6.
             * Since each V6 uses 2 slots, give away the other slot
             *
             * starting from search_index, till start_index search to get the 
             * dest_pfx. Following cases are possible
             * 1. if the dest_pfx has start2 then other_index should be equal
             *    to start2 - 1. then simply copy end2 to start2 -1 and 
             *    decrement start2 and end2 , increase fent for dest_pfx 
             * 2. if start2 for dest_pfx is -1. Then simply increase fent 
                  for dest pfx  
             */
         
            start_index = SOC_LPM128_STATE_START1(u, lpm_state_ptr,
                                                     next_pfx);
            end_index = SOC_LPM128_STATE_END1(u, lpm_state_ptr, next_pfx);  
            other_index = start_index + tcam_depth;
            tcam_number = other_index / tcam_depth;
            if (tcam_number & 1) {
                search_index = (tcam_number  * tcam_depth) - 1;
            } else {
                LOG_ERROR(BSL_LS_SOC_LPM,
                          (BSL_META_U(u,
                                      "moving entries down: other_index: %d not in odd tcam"\
                                      " pfx: %d next_pfx: %d\n"), other_index, pfx, next_pfx));
                return SOC_E_INTERNAL;
            }
            dest_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, search_index);
            for (; dest_pfx == -1 && search_index >= end_index; 
                 search_index--) { 
                dest_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, search_index);
                if (dest_pfx == -1) {
                    continue;
                }
                break;    
            }
              
            if (search_index < end_index) {
                LOG_ERROR(BSL_LS_SOC_LPM,
                          (BSL_META_U(u,
                                      "moving entries down: could not find pfx for other_index: "\
                                      "%d next_pfx: %d pfx: %d free_pfx: %d\n"),
                           other_index, next_pfx, pfx, free_pfx));
                return SOC_E_INTERNAL;
            }
            start_index = SOC_LPM128_STATE_START2(u, lpm_state_ptr,
                                                  dest_pfx);
            if (start_index != -1) {
                if (other_index != start_index - 1) {
                    LOG_ERROR(BSL_LS_SOC_LPM,
                              (BSL_META_U(u,
                                          "Moving entries down, start2 of dest_pfx: %d does not "\
                                          "match other_index: %d free_pfx: %d, pfx: %d next_pfx: "\
                                          "%d\n"), dest_pfx, other_index, free_pfx, pfx, next_pfx));
                    return SOC_E_INTERNAL;
                }
                end_index = SOC_LPM128_STATE_END2(u, lpm_state_ptr, 
                                                  dest_pfx);
                entry_count = (end_index - start_index) + 1;
                if (next_pfx == dest_pfx) {
                    erase = 1;
                }
                if (entry_count == 1) {
                    prev_index = SOC_LPM128_STATE_END1(u, lpm_state_ptr, 
                                                       dest_pfx);
                } else {
                    prev_index = end_index - 1;
                }
                SOC_IF_ERROR_RETURN(_lpm128_fb_v4_entry_shift_down(u,
                   lpm_state_ptr, dest_pfx, prev_index, end_index, other_index, 
                   entry_count, erase));
                if (SOC_LPM128_STATE_END1(u, lpm_state_ptr, dest_pfx) + 1 ==
                                                                  other_index) {
                    SOC_LPM128_STATE_END1(u, lpm_state_ptr, dest_pfx) = 
                    SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx) - 1;   
                    SOC_LPM128_STATE_START2(u, lpm_state_ptr, dest_pfx) = -1;   
                    SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx) = -1;   
                } else {
                    SOC_LPM128_STATE_START2(u, lpm_state_ptr, dest_pfx) -= 1;   
                    SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx) -= 1;   
                }
            }
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, dest_pfx) += 1;
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, free_pfx) -= 1;
            if (!erase) {
                SOC_LPM128_INDEX_TO_PFX_GROUP(u, other_index) = -1;
            }
        }
        free_pfx = next_pfx;
    }
    return SOC_E_NONE;
}
/*
 * curr_pfx - V6 prefix
 * dest_pfx - V4 prefix in which the other_index falls
 */

STATIC int
_lpm128_move_v4_entry_for_v6(int u, int curr_pfx, int dest_pfx, 
                             int other_index, 
                             soc_lpm128_state_p lpm_state_ptr) 
{
    int free_pfx = -1;
    int to_ent = -1;
    int start_ent = -1;
    int end_ent = -1;
    int tcam_number = -1;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int from_end2 = 0;
    int start_tcam_number = 0;
    int end_tcam_number = 0;
    int continous = 0;

    /* other_index must be either start1 or start2 of some V4 index
     * It can be end or some other index.
     * or it must be -1 i.e empty
     */
    
    /*
     * if other_index is -1, then simply decrement the fent
     */

    if (SOC_LPM128_INDEX_TO_PFX_GROUP(u, other_index) == -1) {
        /* Simply decrement fent of dest_pfx */
        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, dest_pfx) -= 1;
        return SOC_E_NONE;
    }

    free_pfx = dest_pfx;
    while (SOC_LPM128_STATE_FENT(u, lpm_state_ptr, free_pfx) == 0) {
        free_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, free_pfx); 
        if (free_pfx == -1) {
            free_pfx = dest_pfx;
            break;  
        }
    }

    while (SOC_LPM128_STATE_FENT(u, lpm_state_ptr, free_pfx) == 0) {
       free_pfx = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, free_pfx);
       if (free_pfx == -1) {
           free_pfx = dest_pfx;
           return SOC_E_FULL;
       }
    }

    if (free_pfx >= curr_pfx) {
        LOG_ERROR(BSL_LS_SOC_LPM,
                  (BSL_META_U(u,
                              "moving v4 entry to create space for V6: curr_pfx: %d "\
                              "free_pfx: %d dest_pfx: %d\n"), curr_pfx, free_pfx, 
                   dest_pfx));
        return SOC_E_FULL;
    }

    if (free_pfx > dest_pfx) {
        SOC_IF_ERROR_RETURN(_lpm128_free_slot_move_down(u, dest_pfx, free_pfx,
                                                        lpm_state_ptr, 0));
    }

    if (free_pfx < dest_pfx) {
        SOC_IF_ERROR_RETURN(_lpm128_free_slot_move_up(u, dest_pfx, free_pfx,
                                                        lpm_state_ptr));
    }

    /*
     * case1: other_index == start2 || other_index == start1
     * solution: move other_index to end + 1 and change start1, end1 or 
     * start2, end2
     * case 2: other_index != start1 && other_index != start2
     * a) This case occurs if the prefix group moved above or below completely
     *                          or
     * b) The prefix group was continous between 2 tcams
     */
    if (SOC_LPM128_STATE_START2(u, lpm_state_ptr, dest_pfx) != -1) {
        start_ent = SOC_LPM128_STATE_START2(u, lpm_state_ptr, dest_pfx);
        end_ent = SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx);
        to_ent = SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx) + 1;     
        from_end2 = 1;
    } else {
        start_ent = SOC_LPM128_STATE_START1(u, lpm_state_ptr, dest_pfx);
        end_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, dest_pfx);
        to_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, dest_pfx) + 1;     
        from_end2 = 0;
    }

    if (start_ent == other_index) {
        /* move the start end to end + 1. if end_ent is half, then
         * move end to end + 1 and start to  end */ 

        SOC_IF_ERROR_RETURN(_lpm128_fb_v4_entry_shift_up(u, lpm_state_ptr, 
                            dest_pfx, start_ent, end_ent, to_ent));
        if (from_end2) {
            SOC_LPM128_STATE_START2(u, lpm_state_ptr, dest_pfx) += 1;
            SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx) += 1;
        } else {
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, dest_pfx) += 1;
            SOC_LPM128_STATE_END1(u, lpm_state_ptr, dest_pfx) += 1;
        }  
    } else {
         if (from_end2) {
             LOG_ERROR(BSL_LS_SOC_LPM,
                       (BSL_META_U(u,
                                   "moving v4 entry to create space for V6:: from end2 "\
                                   "end2(%d)!=other_index(%d) pfx: %d\n"), end_ent, other_index,
                        dest_pfx));
             return SOC_E_INTERNAL;    
         }

         start_tcam_number = (SOC_LPM128_STATE_START1(u, lpm_state_ptr, 
                                                     dest_pfx)) / tcam_depth;
         end_tcam_number = (SOC_LPM128_STATE_END1(u, lpm_state_ptr, 
                                                  dest_pfx)) / tcam_depth;
         if (start_tcam_number != end_tcam_number) {
             continous = 1;
         }

         if (continous) {
             /* if the v4 prefix is continous between the tcams
              * copy other index to end1 + 1 and decrement fent
              * also split the prefix into 2 parts 
              */
             start_ent = other_index;
             end_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, dest_pfx);  
             to_ent = end_ent + 1;
             SOC_IF_ERROR_RETURN(_lpm128_fb_v4_entry_shift_up(u, 
                                 lpm_state_ptr, dest_pfx, start_ent, 
                                 end_ent, to_ent));

             /* split the group */
             tcam_number = SOC_LPM128_STATE_START1(u, lpm_state_ptr, 
                                                   dest_pfx) / tcam_depth;
             SOC_LPM128_STATE_END2(u, lpm_state_ptr, dest_pfx) = to_ent; 
             SOC_LPM128_STATE_START2(u, lpm_state_ptr, dest_pfx) = 
                                                                other_index + 1;
             SOC_LPM128_STATE_END1(u, lpm_state_ptr, dest_pfx) =
                                       (tcam_number + 1) * tcam_depth - 1;
         }
    }

    SOC_LPM128_STATE_FENT(u, lpm_state_ptr, dest_pfx) -= 1;

    return SOC_E_NONE;
}

 
STATIC int
_lpm128_free_slot_move_up(int u, int pfx, int free_pfx, 
                          soc_lpm128_state_p lpm_state_ptr)
{
    soc_lpm_entry_type_t prev_pfx_type;
    soc_lpm_entry_type_t free_pfx_type;
    int prev_pfx = -1;
    int start_index = -1;
    int tcam_number;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int other_index = -1;
    int dest_pfx = -1;
    int trav_index = -1;
    int fent_incr = 0;

    while (free_pfx < pfx) {
        SOC_IF_ERROR_RETURN(_lpm128_fb_shift_pfx_up(u, lpm_state_ptr,
                                                    free_pfx));
        prev_pfx = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, free_pfx);


        SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, prev_pfx, &prev_pfx_type));
        SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, free_pfx, &free_pfx_type));

        /* V6 to V6 case  or V4 to V4 case */
        if (free_pfx_type != socLpmEntryTypeV4) {
            fent_incr = 2;
        } else {
            fent_incr = 1;
        } 
        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, free_pfx) -= fent_incr;
        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, prev_pfx) += fent_incr;

        if (_lpm128_if_conflicting_type(u, prev_pfx_type, free_pfx_type)) {
            /*
             * prev_pfx is V6 and free_pfx is V4.
             * At this stage there could be 1 entry free in V4. But we need
             * 2 entries to move V6 at index n and n + 1024. 
             * n is already free. so now find the prefix of n + 1024 and move 
             * entries of that prefix to find free space  
             */
            start_index = 
                  SOC_LPM128_STATE_START1(u, lpm_state_ptr, free_pfx) - 1;
            tcam_number = start_index / tcam_depth;
            if (tcam_number & 1) {
                start_index = (tcam_number * tcam_depth) - 1;   
            }
            other_index = start_index + tcam_depth;   
            if (!((other_index / tcam_depth) & 1)) {
                LOG_ERROR(BSL_LS_SOC_LPM,
                          (BSL_META_U(u,
                                      "moving entries up: other idx: %d is in even tcam "\
                                      " prev_pfx: %d free_pfx: %d pfx: %d\n"), 
                           other_index, prev_pfx, free_pfx, pfx));
                return SOC_E_INTERNAL;
            }

            for (trav_index = other_index;
                 trav_index != start_index + 1; trav_index--) {
                dest_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, trav_index);
                if (dest_pfx == -1) {
                    continue;
                }
                if (!SOC_LPM128_PFX_IS_V4(u, dest_pfx)) {
                    continue;
                }
                break;  
            }

            if (trav_index == start_index + 1) {
                dest_pfx = free_pfx;
            }

            /* We could get the other pfx. now move the entry */
            SOC_IF_ERROR_RETURN(_lpm128_move_v4_entry_for_v6(u, prev_pfx, 
                                dest_pfx, other_index, lpm_state_ptr));
            /* dest_pfx fent is already decreased */ 
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, prev_pfx) += 1;
        } else if (prev_pfx_type == socLpmEntryTypeV4 &&
                   free_pfx_type != prev_pfx_type) {
            LOG_ERROR(BSL_LS_SOC_LPM,
                      (BSL_META_U(u,
                                  "moving entries up: out of order prefxies free_pfx: %d "\
                                  "prev_pfx: %d pfx: %d\n"), free_pfx, prev_pfx, pfx));
            return SOC_E_INTERNAL;
        }
        free_pfx = prev_pfx;
    }
    return SOC_E_NONE;
}


/*
 *      Create a slot for the new entry rippling the entries if required
 *      
 */
STATIC int 
_lpm128_free_slot_create(int u, int pfx, soc_lpm_entry_type_t type, 
                             void *e, void *eupr, int *free_slot)
{
    int         prev_pfx;
    int         next_pfx;
    int         free_pfx;
    int  is_reserved = 0;
    int tcam_number;
    int fent_incr = 1;
    int rv;
    int v6_pfx_exists = 0;
    int existing_pfx = -1;
    int to_ent = -1;
    int old_to_ent = -1;
    int other_index = -1;
    int trav_index = -1;
    int dest_pfx = -1;
    int existing_vent = -1;
    int offset = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);
    soc_lpm128_state_p lpm_state_ptr = SOC_LPM128_STATE(u);
    soc_lpm_entry_type_t existing_pfx_type = socLpmEntryTypeV4;

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        max_v6_entries >>= 1;
    }

    if (type != socLpmEntryTypeV4) {
        fent_incr = 2;
    }
    
    if (type == socLpmEntryType128BV6 || is_reserved == 0) {
        lpm_state_ptr = SOC_LPM128_STATE(u);
    } else {
        lpm_state_ptr = SOC_LPM128_UNRESERVED_STATE(u);
    }

    if (SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) == 0) {
        SOC_IF_ERROR_RETURN(_lpm128_create_new_pfx_group(u, pfx, type,
                            lpm_state_ptr));
    } else if (type == socLpmEntryTypeV4) {
        /* Check if half of the entry is free. if yes add to it */
        rv = _lpm128_v4_half_entry_available(u, lpm_state_ptr,
                                                    pfx, e, free_slot);
        if (SOC_SUCCESS(rv)) {
            return SOC_E_NONE;  
        } else if (rv != SOC_E_FULL) {
            return rv;  
        }
    }
     
    free_pfx = pfx;
    while (type == socLpmEntryTypeV4 && 
           SOC_LPM128_STATE_FENT(u, lpm_state_ptr, free_pfx) == 0) {
        free_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, free_pfx);
        if (free_pfx == -1) {
            /* No free entries on  right side try the other side */
            free_pfx = pfx;
            break;
        }
    }

    while (SOC_LPM128_STATE_FENT(u, lpm_state_ptr, free_pfx) == 0) {
        free_pfx = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, free_pfx);
        if (free_pfx == -1) {
            /* No free entries on  left side as well */
            if (type == socLpmEntryTypeV4) {
                if (SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) == 0) {
                   /* failed to allocate entries for a newly allocated prefix.*/
                    prev_pfx = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, pfx);
                    next_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, pfx);
                    if (-1 != prev_pfx) {
                        SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, prev_pfx) = 
                            next_pfx;
                    }
                    if (-1 != next_pfx) {
                        SOC_LPM128_STATE_PREV(u, lpm_state_ptr, next_pfx) = 
                        prev_pfx;
                    }
                }
                return(SOC_E_FULL);
            }
            free_pfx = pfx; 
            break;  
        }
    }

    while (type != socLpmEntryTypeV4 && 
           SOC_LPM128_STATE_FENT(u, lpm_state_ptr, free_pfx) == 0) {
        free_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, free_pfx);
        if (free_pfx == -1) {
            free_pfx = pfx;
            /* No free entries on  right side as well */
            if (SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) == 0) {
                /* failed to allocate entries for a newly allocated prefix.*/
                prev_pfx = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, pfx);
                next_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, pfx);
                if (-1 != prev_pfx) {
                    SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, prev_pfx) = 
                        next_pfx;
                }
                if (-1 != next_pfx) {
                    SOC_LPM128_STATE_PREV(u, lpm_state_ptr, next_pfx) = 
                    prev_pfx;
                }
            }
            return(SOC_E_FULL);
        }
    }

    /*
     * Ripple entries to create free space
     */
    if (free_pfx > pfx) {
        SOC_IF_ERROR_RETURN(_lpm128_free_slot_move_down(u, pfx, free_pfx, 
                                                        lpm_state_ptr, 0));
    }

    if (free_pfx < pfx) {
        SOC_IF_ERROR_RETURN(_lpm128_free_slot_move_up(u, pfx, free_pfx, 
                                                        lpm_state_ptr));
    }

    if (type == socLpmEntryTypeV4) {
        /* if start2 is not -1, then increase end2 */ 
        if (SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) != -1) {
            SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) += 1;
        } else {
            /* if end1 + 1 falls in odd tcam and 
             * there are V6 prefixes at index 0 then split the group
             * if there are no V6 prefixes, just increment end1
             */
             to_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) + 1;
             old_to_ent = to_ent;
             tcam_number = to_ent / tcam_depth;
             if (tcam_number & 1 && (to_ent % tcam_depth == 0)) {
                 if (is_reserved) {
                     to_ent += (max_v6_entries % tcam_depth);
                 }
                 existing_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, to_ent);
                 while(existing_pfx != -1) {
                     SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, existing_pfx, 
                                         &existing_pfx_type));
                     if (existing_pfx_type == socLpmEntryTypeV4) {
                         if (existing_pfx > pfx) {
                             LOG_ERROR(BSL_LS_SOC_LPM,
                                       (BSL_META_U(u,
                                                   "creating free slot, wrong order of prefixes "\
                                                   "existing_pfx: %d pfx: %d\n"),
                                        existing_pfx, pfx));
                             return SOC_E_INTERNAL;
                         }
                         break;    
                     }
                     v6_pfx_exists = 1;
                     _lpm128_v6_vent_in_curr_tcam(u, existing_pfx, 
                                                  lpm_state_ptr, 
                                                  &existing_vent);
                     offset = existing_vent + (SOC_LPM128_STATE_FENT(u, 
                                lpm_state_ptr, existing_pfx) / 2);
                     to_ent += offset;
                     existing_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, to_ent);
                 }

                 if (!v6_pfx_exists && !is_reserved) {
                     if (old_to_ent == to_ent) {
                         _lpm128_find_new_index_from_v6_group(u, pfx, 
                                                              lpm_state_ptr,
                                                              &to_ent);
                     }
                     if (old_to_ent == to_ent) {
                         SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = to_ent;
                     } else {
                         /* split the prefix into to */
                         SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = to_ent;
                         SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = to_ent;
                     }
                 } else {
                     /* split the prefix into to */
                     SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = to_ent;
                     SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = to_ent;
                 }
             } else {
                SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = to_ent;
             }
        }
    } else {
        to_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) + 1;
        tcam_number = to_ent / tcam_depth;
        if (tcam_number & 1) {
            if (!is_reserved || (type == socLpmEntryType128BV6)) {
                to_ent += tcam_depth;
            } else if(type == socLpmEntryType64BV6) {
                LOG_ERROR(BSL_LS_SOC_LPM,
                          (BSL_META_U(u,
                                      "creating free slot: END of 64BV6 entries crossing TCAM"\
                                      " boundary for pfx - %d\n"), pfx));
                return SOC_E_INTERNAL;
            }
            if (!SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx)) {
                if (SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) != 
                                                                   to_ent) {
                    SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) +=
                        (max_v6_entries % tcam_depth);
                }
            }
        }

        if (SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) < 2) {
            other_index = to_ent + tcam_depth;
            if (!((other_index / tcam_depth) & 1)) {
                LOG_ERROR(BSL_LS_SOC_LPM,
                          (BSL_META_U(u,
                                      "creating free slot: other_idx: %d is in even tcam "\
                                      " pfx: %d\n"),
                           other_index, pfx));
                return SOC_E_INTERNAL;
            }

            for (trav_index = other_index;
                 trav_index != to_ent; trav_index--) {
                dest_pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(u, trav_index);
                if (dest_pfx == -1) {
                    continue;
                }
                if (!SOC_LPM128_PFX_IS_V4(u, dest_pfx)) {
                    continue;
                }
                break;
            }

            if (trav_index == to_ent) {
                LOG_ERROR(BSL_LS_SOC_LPM,
                          (BSL_META_U(u,
                                      "creating free slot: Did not "\
                                      "find dest_pfx to move entry from for pfx: %d"), pfx));
                return SOC_E_INTERNAL;
            }

            /* We could get the other pfx. now move the entry */
            SOC_IF_ERROR_RETURN(_lpm128_move_v4_entry_for_v6(u, pfx,
                                dest_pfx, other_index, lpm_state_ptr));
            /* dest_pfx fent is already decreased */
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) += 1;
        }
        SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = to_ent;
    }
    SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) += 1;
    SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) -= fent_incr;

    if (type == socLpmEntryTypeV4) {
        if (SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) != -1) {
            *free_slot = SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) << 1;
        } else {
            *free_slot = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) << 1;
        }
        sal_memcpy(e, soc_mem_entry_null(u, L3_DEFIPm),
                   soc_mem_entry_words(u,L3_DEFIPm) * 4);

    } else {
        *free_slot = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
    }
    return(SOC_E_NONE);
}


STATIC int
_lpm128_move_v4_entry_down_for_v6(int u, int free_cnt,
                                  int start_index, int other_pfx,
                                  soc_lpm128_state_p lpm_state_ptr,
                                  int free_pfx)
{
    int other_from_end2 = 0;
    int iter = 0;
    int from_ent = -1;
    int start_ent = start_index;
    int prev_ent = -1;
    int entry_count = 0;

    if (SOC_LPM128_STATE_END2(u, lpm_state_ptr, other_pfx) != -1) {
        from_ent = SOC_LPM128_STATE_END2(u, lpm_state_ptr, other_pfx);
        other_from_end2 = 1;
        entry_count = SOC_LPM128_STATE_END2(u, lpm_state_ptr, other_pfx) -
            SOC_LPM128_STATE_START2(u, lpm_state_ptr, other_pfx) + 1;
    } else {
        from_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, other_pfx);
        other_from_end2 = 0;
        entry_count = SOC_LPM128_STATE_END1(u, lpm_state_ptr, other_pfx) -
            SOC_LPM128_STATE_START1(u, lpm_state_ptr, other_pfx) + 1;
    }

    if (entry_count ==1) {
        prev_ent = from_ent;
        iter = 1;
    } else {
        prev_ent = from_ent - 1;
        iter = (entry_count - 1 > free_cnt) ? free_cnt: (entry_count - 1);
    }

    while (iter) {
       if (prev_ent != start_ent) {
           SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr,
                               other_pfx, prev_ent, start_ent, 1));
       }
       iter--;
       prev_ent--;
       start_ent++;
    }

    if (entry_count!=1) {
        if (free_cnt >= entry_count-1) {
            SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr,
                                other_pfx, from_ent, start_ent, 1));
        } else {
            prev_ent++;
            SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr,
                                other_pfx, from_ent, prev_ent, 1));
        }
    }

    iter = (entry_count > free_cnt) ? free_cnt : entry_count;
    if (other_from_end2) {
        SOC_LPM128_STATE_START2(u, lpm_state_ptr, other_pfx) -= iter;
        SOC_LPM128_STATE_END2(u, lpm_state_ptr, other_pfx) -= iter;
    } else {
        SOC_LPM128_STATE_START1(u, lpm_state_ptr, other_pfx) -= iter;
        SOC_LPM128_STATE_END1(u, lpm_state_ptr, other_pfx) -= iter;
    }

    return SOC_E_NONE;
}

STATIC int
_lpm128_move_next_pfx_down_during_delete(int u, int pfx, int prev_pfx, 
                                         soc_lpm128_state_p lpm_state_ptr,
                                         int start_index, int count)
{
    int iter = 0;
    int from_ent = -1;
    int from_end2 = 0;
    int half_entry = 0;
    uint32 v0, v1;
    int rv;
    int entry_count = 0;
    int start = -1;
    int prev_ent = -1;
    int end1_index = -1;
    int end2_index = -1;
    int start2_index = -1;
    int vent = SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx);
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    uint32      e[SOC_MAX_MEM_FIELD_WORDS] = {0};
    soc_lpm_entry_type_t prev_pfx_type;
    soc_lpm_entry_type_t pfx_type;

    if (!SOC_LPM128_PFX_IS_V4(u, prev_pfx)) {
        count >>= 1;
    }

    SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, prev_pfx, &prev_pfx_type));
    SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, pfx, &pfx_type));

    /* take care of deleting 64BV6 prefix group previous to V4 prefix group */
    if (socLpmEntryType64BV6 == prev_pfx_type && 
        socLpmEntryTypeV4 == pfx_type ){

        int other_start  = -1;
        int other_pfx    = pfx;
        int bottom_pfx   = pfx;
        int other_start_v4 = -1;

        other_start_v4 = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) +
                         tcam_depth;
        while (other_pfx != -1) {
            if (SOC_LPM128_STATE_START1(u, lpm_state_ptr, other_pfx) >= 
                other_start_v4 ||
                SOC_LPM128_STATE_START2(u, lpm_state_ptr, other_pfx) >= 
                other_start_v4) {
                break;
            }
            bottom_pfx = other_pfx;
            other_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, other_pfx);
        }

        if (other_pfx == -1 ||
            SOC_LPM128_STATE_END2(u, lpm_state_ptr, other_pfx) == -1) {
            /* no other_pfx present or other_pfx is not split,
               just give free entries to previous bottom prefix */

            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, bottom_pfx) +=
                        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, prev_pfx) >> 1;

            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, prev_pfx) >>= 1;
        } else {
            /* other_pfx is split, move other_pfx to make contiguous */
            other_start = start_index + tcam_depth;
            _lpm128_move_v4_entry_down_for_v6(u, count, other_start,
                                          other_pfx, lpm_state_ptr, prev_pfx);

            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, other_pfx) +=
                        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, prev_pfx) >> 1;

            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, prev_pfx) >>= 1;
        }
    }

    if (SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) != -1) {
        from_ent = SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx);
        from_end2 = 1;
        entry_count = SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) -
                      SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) + 1;
        start = SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx);
    } else {
        from_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
        from_end2 = 0;
        entry_count = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) -
                      SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) + 1;
        start = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx);
    }

#ifdef FB_LPM_TABLE_CACHED
    if ((rv = soc_mem_cache_invalidate(u, L3_DEFIPm,
                                       MEM_BLOCK_ANY, from_ent)) < 0) {
        return rv;
    }
#endif /* FB_LPM_TABLE_CACHED */
    if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e)) < 0) {
        return rv;
    }
    v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
    v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);

    if ((v0 == 0) || (v1 == 0)) {
        half_entry = 1;
        if (from_end2) {
            if (entry_count == 1) {
                prev_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
                start = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx);
            } else {
                prev_ent = SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) - 1;
            }
        } else {
            if (entry_count == 1) {
                prev_ent = from_ent;
                half_entry = 0;
            } else {
                prev_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) - 1;
            }
        }
    } else {
        prev_ent = from_ent;
    }

    iter = count > vent ? vent : count;
    while (iter) {
       if (prev_ent < start) {
           if (from_end2) {
               if (start == SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx)) {
                   break;
               }
               start = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx);
               prev_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
           } else {
               break;
           }
       }
       if (prev_ent != start_index) {
           SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr, pfx,
                                                     prev_ent, start_index, 1));
       }
       iter--;
       prev_ent--;

       if (((prev_ent / tcam_depth) & 1) && !SOC_LPM128_PFX_IS_V4(u, pfx)) {
           prev_ent -= tcam_depth;
       }
       if (start2_index != -1) {
           end2_index = start_index;
       } else {
           end1_index = start_index;
       }
       _lpm128_get_next_up_index(u, pfx, lpm_state_ptr, &start_index);
       if ((SOC_LPM128_PFX_IS_V4(u, pfx) && (start_index - end1_index > 1) &&
           start2_index == -1)) {
          start2_index = start_index;
       }
    }

    if ((prev_ent < start) &&
        (start == SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx))) {
        prev_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
    }

    if (half_entry) {
        if (count >= vent) {
            SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr, pfx, 
                                                   from_ent, start_index, 1));
            if (start2_index != -1) {
                end2_index = start_index;
            } else {
                end1_index = start_index;
            }
        } else {
            _lpm128_get_next_up_index(u, pfx, lpm_state_ptr, &prev_ent);
            SOC_IF_ERROR_RETURN(_lpm128_fb_entry_shift(u, lpm_state_ptr, pfx, 
                                                   from_ent, prev_ent, 1));
        }
    }

    if (count >= vent) {
        SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = end1_index;
        SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = start2_index;
        SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = end2_index;
    } else {
        iter = count > vent ? vent : count;
        if (from_end2) {
            if ((entry_count - iter) > 0) {
                SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = prev_ent;
            } else {
                SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = prev_ent;
                SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = -1;
                SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = -1;
            }
        } else {
            if (start2_index != -1) {
                SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = prev_ent;
                SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = end1_index;
            } else {
                SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = prev_ent;
            }   
            SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = start2_index;
        }
    }
    SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = SOC_LPM128_STATE_START1(u, 
                                                       lpm_state_ptr, prev_pfx);
    SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) += SOC_LPM128_STATE_FENT(u, 
                                                    lpm_state_ptr, prev_pfx);
    SOC_LPM128_STATE_FENT(u, lpm_state_ptr, prev_pfx) = 0;
    return SOC_E_NONE;
}

STATIC int
_lpm128_v4_free_slot_delete(int u, int pfx, 
                            soc_lpm128_state_p lpm_state_ptr, 
                            void *e, int slot, int *is_deleted)
{
    int         prev_pfx;
    int         next_pfx;
    int         from_ent;
    int         to_ent;
    uint32      ef[SOC_MAX_MEM_FIELD_WORDS];
    void        *et;
    int         rv;
    int from_end2;
    int entry_count = 0;
    int end_index;
    int tcam_number;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int is_reserved = 0;
    int moved_down = 0;
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);
    soc_lpm_entry_type_t prev_pfx_type = socLpmEntryType128BV6;
    soc_lpm_entry_type_t pfx_type = socLpmEntryType128BV6;

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        max_v6_entries >>= 1;
    }

    to_ent = slot;
    to_ent >>= 1;

    if (SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) != -1) {
        from_ent = SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx);
        from_end2 = 1;
        entry_count = SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) -
                     SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) + 1;
    } else {
        from_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
        from_end2 = 0;
    }

#ifdef FB_LPM_TABLE_CACHED
    if ((rv = soc_mem_cache_invalidate(u, L3_DEFIPm,
                                       MEM_BLOCK_ANY, from_ent)) < 0) {
        return rv;
    }
#endif /* FB_LPM_TABLE_CACHED */
    if ((rv = READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, ef)) < 0) {
        return rv;
    }
    et =  (to_ent == from_ent) ? ef : e;
    if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, ef, VALID1f)) {
        if (slot & 1) {
            rv = soc_fb_lpm_ip4entry1_to_1(u, ef, et, PRESERVE_HIT);
        } else {
            rv = soc_fb_lpm_ip4entry1_to_0(u, ef, et, PRESERVE_HIT);
        }
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, ef, VALID1f, 0);
        /* first half of full entry is deleted, generating half entry.*/
        SOC_LPM_COUNT_INC(SOC_LPM128_STAT_V4_HALF_ENTRY_COUNT(u));
    } else {
        if (slot & 1) {
            rv = soc_fb_lpm_ip4entry0_to_1(u, ef, et, PRESERVE_HIT);
        } else {
            rv = soc_fb_lpm_ip4entry0_to_0(u, ef, et, PRESERVE_HIT);
        }
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, ef, VALID0f, 0);
        /* Remaining half entry got deleted. Decrement half entry count.*/ 
        SOC_LPM_COUNT_DEC(SOC_LPM128_STAT_V4_HALF_ENTRY_COUNT(u));
        SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) -= 1;
        SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) += 1;
        SOC_LPM128_INDEX_TO_PFX_GROUP(u, from_ent) = -1;
        *is_deleted = 1;
        if (from_end2) {
            if (entry_count == 1) {
                SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = -1;
                SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = -1;
            } else {
                SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) -= 1;
            }
        } else {
            SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) -= 1;
            end_index = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
            tcam_number = end_index / tcam_depth;
            if (tcam_number & 1) {
                if ((is_reserved) && ((end_index % tcam_depth) < 
                                     (max_v6_entries % tcam_depth))) {
                    end_index -= (max_v6_entries % tcam_depth);
                    SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = end_index;
                }
            }
        }
    }

    if (to_ent != from_ent) {
        LPM128_HASH_INSERT(u, et, NULL, to_ent);
        if ((rv = WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, to_ent, et)) < 0) {
            return rv;
        }
        if ((rv = _lpm128_fb_urpf_entry_replicate(u, to_ent, et, NULL)) < 0) {
            return rv;
        }
    }
    LPM128_HASH_INSERT(u, ef, NULL, from_ent);
    if ((rv = WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, ef)) < 0) {
        return rv;
    }
    if ((rv = _lpm128_fb_urpf_entry_replicate(u, from_ent, ef, NULL)) < 0) {
        return rv;
    }

    if (SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) == 0) {
        /* remove from the list */
        prev_pfx = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, pfx); 
        /* prev_pfx Always present */
        assert(prev_pfx != -1);
        SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, prev_pfx, &prev_pfx_type));
        SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, pfx, &pfx_type));
        next_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, pfx);

        if ((_lpm128_if_conflicting_type(u, pfx_type, prev_pfx_type) &&
            next_pfx != -1) || 
            (prev_pfx == MAX_PFX128_INDEX && next_pfx != -1)) {
            _lpm128_move_next_pfx_down_during_delete(u, next_pfx, pfx, 
                                                   lpm_state_ptr,
                SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx),
                SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx));
            moved_down = 1;
        }

        if (next_pfx != -1) {
            SOC_LPM128_STATE_PREV(u, lpm_state_ptr, next_pfx) = prev_pfx;
        }
        SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, prev_pfx) = next_pfx;
        if (!moved_down) {
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, prev_pfx) += 
                SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx);
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) = 0;
        }
        SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_PREV(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = -1;
    }

    return(rv);
}


/*
 *      Delete a slot and adjust entry pointers if required.
 *      e - has the contents of entry at slot(useful for IPV4 only)
 */
STATIC int 
_lpm128_free_slot_delete(int u, int pfx, soc_lpm_entry_type_t type, 
                         void *e, int slot, int *is_deleted)
{
    int         prev_pfx;
    int         next_pfx;
    int         from_ent;
    int         to_ent;
    uint32      ef[SOC_MAX_MEM_FIELD_WORDS];
    uint32      efupr[SOC_MAX_MEM_FIELD_WORDS];
    int         rv = SOC_E_NONE;
    int  is_reserved = 0;
    int fent_incr = 1;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int tcam_number = 0;
    soc_lpm128_state_p lpm_state_ptr = SOC_LPM128_STATE(u);
    int moved_down = 0;
    soc_lpm_entry_type_t prev_pfx_type = socLpmEntryType128BV6;
    soc_lpm_entry_type_t pfx_type = socLpmEntryType128BV6;

   if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    if (type == socLpmEntryType128BV6 || is_reserved == 0) {
       lpm_state_ptr = SOC_LPM128_STATE(u);
    } else {
       lpm_state_ptr = SOC_LPM128_UNRESERVED_STATE(u);
    }

    if (type == socLpmEntryTypeV4) {
        return _lpm128_v4_free_slot_delete(u, pfx, lpm_state_ptr, e, slot, 
                                           is_deleted);
    }

    fent_incr = 2;    
    from_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx);
    to_ent = slot;
    SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) -= 1;
    SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) += fent_incr;
    SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) -= 1;
    tcam_number = SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) / 
                  tcam_depth;
    if (tcam_number & 1) {
        if (!is_reserved || type == socLpmEntryType128BV6) { 
            SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) -= tcam_depth;
        }
    }

    if (to_ent != from_ent) {
        SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, ef));
        SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, 
                                           from_ent + tcam_depth, efupr));
        LPM128_HASH_INSERT(u, ef, efupr, to_ent);

        SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, to_ent, ef));
        SOC_LPM128_INDEX_TO_PFX_GROUP(u, to_ent) = pfx;
        SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, 
                                            to_ent + tcam_depth, efupr));
        SOC_LPM128_INDEX_TO_PFX_GROUP(u, to_ent + tcam_depth) = pfx;
        SOC_IF_ERROR_RETURN(_lpm128_fb_urpf_entry_replicate(u, to_ent, ef, 
                                                            efupr));
    }

    sal_memcpy(ef, soc_mem_entry_null(u, L3_DEFIPm),
               soc_mem_entry_words(u,L3_DEFIPm) * 4);
    sal_memcpy(efupr, soc_mem_entry_null(u, L3_DEFIPm),
               soc_mem_entry_words(u,L3_DEFIPm) * 4);
    LPM128_HASH_INSERT(u, ef, efupr, from_ent);
    SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, ef));
    SOC_LPM128_INDEX_TO_PFX_GROUP(u, from_ent) = -1;
    SOC_IF_ERROR_RETURN(WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent + tcam_depth,
                                        efupr));
    SOC_LPM128_INDEX_TO_PFX_GROUP(u, from_ent + tcam_depth) = -1;
    SOC_IF_ERROR_RETURN(_lpm128_fb_urpf_entry_replicate(u, from_ent, ef, 
                                                        efupr));
    *is_deleted = 1;
    if (SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) == 0) {
        /* remove from the list */
        /* Always present */
        prev_pfx = SOC_LPM128_STATE_PREV(u, lpm_state_ptr, pfx); 
        assert(prev_pfx != -1);
        next_pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, pfx);
        SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, prev_pfx, &prev_pfx_type));
        SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, pfx, &pfx_type));

        if ((_lpm128_if_conflicting_type(u, pfx_type, prev_pfx_type) &&
            next_pfx != -1) ||
            (prev_pfx == MAX_PFX128_INDEX && next_pfx != -1)) {
            _lpm128_move_next_pfx_down_during_delete(u, next_pfx, pfx,
                                                     lpm_state_ptr,
                SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx),
                SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx));
            moved_down = 1;
        }

        if (!moved_down) {
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, prev_pfx) +=
                SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx);
            SOC_LPM128_STATE_FENT(u, lpm_state_ptr, pfx) = 0;
        }
        if (next_pfx != -1) {
            SOC_LPM128_STATE_PREV(u, lpm_state_ptr, next_pfx) = prev_pfx;
        }
        SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, prev_pfx) = next_pfx;
        SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_PREV(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = -1;
        SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = -1;
    }

    return(rv);
}

STATIC int
soc_fb_lpm128_can_insert_entry(int u, soc_lpm_entry_type_t type)
{
    int free_count = 0;
    
    if (type == socLpmEntryTypeV4) {
        SOC_IF_ERROR_RETURN(soc_lpm_free_v4_route_get(u, &free_count));
    } else if (type == socLpmEntryType64BV6) {
        SOC_IF_ERROR_RETURN(soc_lpm_free_64bv6_route_get(u, &free_count));
    } else if (type == socLpmEntryType128BV6) {
        SOC_IF_ERROR_RETURN(soc_lpm_free_128bv6_route_get(u, &free_count));
    }
    if (free_count == 0) {
        return FALSE;
    }
 
    return TRUE;
}

/*
 * Implementation using soc_mem_read/write using entry rippling technique
 * Advantage: A completely sorted table is not required. Lookups can be slow
 * as it will perform a linear search on the entries for a given prefix length.
 * No device access necessary for the search if the table is cached. Auxiliary
 * hash table can be maintained to speed up search(Not implemented) instead of
 * performing a linear search.
 * Small number of entries need to be moved around (97 worst case)
 * for performing insert/update/delete. However CPU needs to do all
 * the work to move the entries.
 */

/*
 * soc_fb_lpm128_insert
 * For IPV4 assume only both IP_ADDR0 is valid
 * Moving multiple entries around in h/w vs  doing a linear search in s/w
 */
int
soc_fb_lpm128_insert(int u, void *entry_data, void *entry_data1,
                     int *hw_index)
{
    int         pfx;
    int         index, copy_index;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS] =  {0};
    uint32      e1[SOC_MAX_MEM_FIELD_WORDS] = {0};
    int         rv = SOC_E_NONE;
    int         found = 0;
    soc_lpm_entry_type_t type = socLpmEntryType128BV6;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);

    sal_memcpy(e, soc_mem_entry_null(u, L3_DEFIPm),
               soc_mem_entry_words(u,L3_DEFIPm) * 4);

  
    SOC_LPM_LOCK(u);
    rv = _soc_fb_lpm128_match(u, entry_data, entry_data1, e, e1, 
                              &index, &pfx, &type);
     
    if (rv == SOC_E_NOT_FOUND) {
        if (!soc_fb_lpm128_can_insert_entry(u, type)) {
            SOC_LPM_UNLOCK(u);
            return SOC_E_FULL; 
        }
        rv = _lpm128_free_slot_create(u, pfx, type, e, e1, &index);
        if (rv < 0) {
            SOC_LPM_UNLOCK(u);
            return(rv);
        }
    } else {
        found = 1;
    }

    copy_index = index;
    if (rv == SOC_E_NONE) {
        /* Entry already present. Update the entry */
        if (type == socLpmEntryTypeV4) {
            /* IPV4 entry */
            if (index & 1) {
                rv = soc_fb_lpm_ip4entry0_to_1(u, entry_data, e, PRESERVE_HIT);
            } else {
                rv = soc_fb_lpm_ip4entry0_to_0(u, entry_data, e, PRESERVE_HIT);
            }

            if (rv < 0) {
                SOC_LPM_UNLOCK(u);
                return(rv);
            }

            entry_data = (void *)e;
            index >>= 1;
        }
 
        if (!found) {
            LPM128_HASH_INSERT(u, entry_data, entry_data1, index);
            if (type == socLpmEntryTypeV4) {
                SOC_LPM_COUNT_INC(SOC_LPM128_STAT_V4_COUNT(u));
                /* indexes should change for new entry only */
                if (copy_index & 1) {
                    SOC_LPM_COUNT_DEC(SOC_LPM128_STAT_V4_HALF_ENTRY_COUNT(u));
                } else {
                    SOC_LPM_COUNT_INC(SOC_LPM128_STAT_V4_HALF_ENTRY_COUNT(u));
                }
            }

            if (type == socLpmEntryType64BV6) {
                SOC_LPM_COUNT_INC(SOC_LPM128_STAT_64BV6_COUNT(u));
            }

            if (type == socLpmEntryType128BV6) {
                SOC_LPM_COUNT_INC(SOC_LPM128_STAT_128BV6_COUNT(u));
            }
        } else {
            LOG_INFO(BSL_LS_SOC_LPM,
                     (BSL_META_U(u,
                                 "\nsoc_fb_lpm128_insert: %d %d ENTRY ALREADY PRESENT\n"),
                      index, pfx));
        }
        rv = WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, index, entry_data);
        if (rv >= 0) {
            SOC_LPM128_INDEX_TO_PFX_GROUP(u, index) = pfx;
            if (type != socLpmEntryTypeV4) {
                rv = WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, index + tcam_depth, 
                                     entry_data1);
                SOC_LPM128_INDEX_TO_PFX_GROUP(u, index + tcam_depth) = pfx;
                rv = _lpm128_fb_urpf_entry_replicate(u, index, entry_data, 
                                                     entry_data1);
            } else {
                rv = _lpm128_fb_urpf_entry_replicate(u, index, entry_data, NULL);
            }
        }
    }

    SOC_LPM_UNLOCK(u);

    return(rv);
}

/*
 * soc_fb_lpm_delete
 */
int
soc_fb_lpm128_delete(int u, void *key_data, void* key_data_upr)
{
    int         pfx;
    int         index;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS];
    uint32      eupr[SOC_MAX_MEM_FIELD_WORDS];
    int         rv = SOC_E_NONE;
    soc_lpm_entry_type_t type;
    int is_deleted = 0;
 
    SOC_LPM_LOCK(u);
    rv = _soc_fb_lpm128_match(u, key_data, key_data_upr, e, eupr, 
                              &index, &pfx, &type);
    if (rv == SOC_E_NONE) {
        LOG_INFO(BSL_LS_SOC_LPM,
                 (BSL_META_U(u,
                             "\nsoc_fb_lpm128_delete: %d %d\n"),
                             index, pfx));
        if (type != socLpmEntryTypeV4) {
            LPM128_HASH_DELETE(u, key_data, key_data_upr, index);
        } else {
            LPM128_HASH_DELETE(u, key_data, NULL, index);
        }
        rv = _lpm128_free_slot_delete(u, pfx, type, e, index, &is_deleted);
        if (rv >= 0) {
            if (type == socLpmEntryTypeV4) {
                SOC_LPM_COUNT_DEC(SOC_LPM128_STAT_V4_COUNT(u));
            }

            if (type == socLpmEntryType64BV6) {
                SOC_LPM_COUNT_DEC(SOC_LPM128_STAT_64BV6_COUNT(u));
            }

            if (type == socLpmEntryType128BV6) {
                SOC_LPM_COUNT_DEC(SOC_LPM128_STAT_128BV6_COUNT(u));
            }
        }

    }
    SOC_LPM_UNLOCK(u);
    return(rv);
}

/*
 * soc_fb_lpm_match (Exact match for the key. Will match both IP
 *                   address and mask)
 */
int
soc_fb_lpm128_match(int u,
               void *key_data,
               void *key_data_upr,
               void *e,         /* return entry data if found */
               void *eupr,         /* return entry data if found */
               int *index_ptr,
               int *pfx,
               soc_lpm_entry_type_t *type)
{
    int        rv;

    SOC_LPM_LOCK(u);
    rv = _soc_fb_lpm128_match(u, key_data, key_data_upr, e, eupr, index_ptr,
                              pfx, type);
    SOC_LPM_UNLOCK(u);
    return(rv);
}

/*
 * soc_fb_lpm_lookup (LPM search). Masked match.
 */
int
soc_fb_lpm128_lookup(int u,
               void *key_data,
               void *entry_data,
               int *index_ptr)
{
    return(SOC_E_UNAVAIL);
}


int soc_fb_lpm128_get_smallest_movable_prefix(int u, int ipv6, void *e,
                                             void *eupr, int *index,
                                             int *pfx_len, int *count)
{
    soc_lpm128_state_p lpm_state_ptr = SOC_LPM128_STATE(u);
    int pfx = MAX_PFX128_INDEX;
    int small_pfx = -1;
    soc_lpm_entry_type_t type;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    int v0, v1;

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        lpm_state_ptr = SOC_LPM128_UNRESERVED_STATE(u);
    }

    if (pfx_len == NULL) {
        return SOC_E_PARAM;
    }

    if (lpm_state_ptr == NULL) {
        return SOC_E_NOT_FOUND;
    }

    while (pfx != -1) {
        SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, pfx, &type));
        if (ipv6 && type == socLpmEntryType64BV6) {
            small_pfx = pfx;
        }
        if (!ipv6 && type == socLpmEntryTypeV4) {
           small_pfx = pfx;
        }
        pfx = SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, pfx);
    }
   
    if (small_pfx == -1) {
       return SOC_E_NOT_FOUND;
    }

    if (ipv6) {
        if (index != NULL && e != NULL && eupr != NULL && count != NULL) { 
            *index = SOC_LPM128_STATE_END1(u, lpm_state_ptr, small_pfx);
            SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, *index, e));
            SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, 
                                               (*index + tcam_depth), eupr));
            *count = 1;
        }
    } else {
        if (index != NULL && e != NULL && count != NULL) {
            if (SOC_LPM128_STATE_END2(u, lpm_state_ptr, small_pfx) != -1) {
                *index = SOC_LPM128_STATE_END2(u, lpm_state_ptr, small_pfx);
            } else {
                *index = SOC_LPM128_STATE_END1(u, lpm_state_ptr, small_pfx);
            }
            SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, *index, e));
            v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
            v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);

            if (v0 == 0 || v1 == 0) {
                *count = 1;
            } else {
                *count = 2;
            }
        }
    }

    *pfx_len = small_pfx;

    return SOC_E_NONE;
} 

int soc_fb_lpm128_is_v4_64b_allowed_in_paired_tcam(int unit)
{
    if (soc_feature(unit, soc_feature_l3_lpm_128b_entries_reserved)) {
        return (SOC_LPM128_UNRESERVED_STATE(unit) != NULL) ? 1 : 0;
    }

    return (SOC_LPM128_STATE(unit) != NULL) ? 1 : 0;
}

int
soc_fb_lpm_table_sizes_get(int unit, int *paired_table_size, 
                           int *defip_table_size)
{
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit);
    int max_paired_count = (SOC_L3_DEFIP_MAX_TCAMS_GET(unit)) / 2;
    int tcam_pair_count = 0;
    int paired_size = 0;
    int unpaired_size = 0;

    if (!soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        return SOC_E_UNAVAIL;
    }

    SOC_IF_ERROR_RETURN(soc_fb_lpm_tcam_pair_count_get(unit, &tcam_pair_count));
    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(unit) &&
        !soc_feature(unit, soc_feature_l3_defip_advanced_lookup)) {
        switch(tcam_pair_count) {
            case 1:
            case 2:
                unpaired_size = 2 * tcam_depth;
                paired_size = 2 * tcam_depth;
                break;
            case 3:
            case 4:
                unpaired_size = 0;
                paired_size = 4 * tcam_depth;
                break;
            default:
                unpaired_size = 4 * tcam_depth;
                paired_size = 0;
               break;
        }
    } else {
        unpaired_size = (max_paired_count - tcam_pair_count) * tcam_depth;
        unpaired_size <<= 1;
        paired_size = tcam_pair_count * tcam_depth;
        paired_size <<= 1;
    }

    if (paired_table_size != NULL) {
       *paired_table_size = paired_size;
    }
 
    if (defip_table_size != NULL) {
       *defip_table_size = unpaired_size;
    }

    return SOC_E_NONE;
}

int soc_lpm_max_v4_route_get(int u, int *entries)
{
    if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
        return SOC_E_UNAVAIL;
    }
    *entries = SOC_LPM_MAX_V4_COUNT(u);
    if (soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        *entries += SOC_LPM128_STAT_MAX_V4_COUNT(u);
    }
    return SOC_E_NONE;
}

int soc_lpm_max_64bv6_route_get(int u, int *entries)
{

    if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
        return SOC_E_UNAVAIL;
    }

    *entries = SOC_LPM_MAX_64BV6_COUNT(u);
    if (soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        *entries += SOC_LPM128_STAT_MAX_64BV6_COUNT(u);
    }
    return SOC_E_NONE;
}

int soc_lpm_max_128bv6_route_get(int u, int *entries)
{
    if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
        return SOC_E_UNAVAIL;
    }

    *entries = SOC_LPM_MAX_128BV6_COUNT(u);
    if (soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        *entries += SOC_LPM128_STAT_MAX_128BV6_COUNT(u);
    }
    return SOC_E_NONE;
}

int
soc_lpm_free_v4_route_get(int u, int *entries)
{
    int v4_count = 0;
    int b64_count = 0;
    int b128_count = 0;
    int is_reserved = 0;
    int v4_max_count = 0;

    if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
        return SOC_E_UNAVAIL;
    }

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    SOC_IF_ERROR_RETURN(soc_lpm_max_v4_route_get(u, &v4_max_count));
    SOC_IF_ERROR_RETURN(soc_lpm_used_v4_route_get(u, &v4_count));
    b64_count = SOC_LPM_64BV6_COUNT(u) << 1;
    if (soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        b64_count += (SOC_LPM128_STAT_64BV6_COUNT(u) << 2);
        if (!is_reserved) {
            SOC_IF_ERROR_RETURN(soc_lpm_used_128bv6_route_get(u, &b128_count));
            b128_count <<= 2;
        }
    }
    *entries = v4_max_count - (b64_count + b128_count) - v4_count;
    return SOC_E_NONE;
}

int
soc_lpm_free_64bv6_route_get(int u, int *entries)
{
    int v4_count = 0;
    int b64_count = 0;
    int b128_count = 0;
    int is_reserved = 0;
    int v64_max_count = 0;
    int paired_v64_max_count = 0;
    int paired_v4_count = 0;
    int paired_tcam_size = 0;
    int paired_v64_count = 0;
    int paired_count = 0;
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);

    if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
        return SOC_E_UNAVAIL;
    }

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(u) &&
        !soc_feature(u, soc_feature_l3_defip_advanced_lookup)) {
        max_v6_entries >>= 1;
    }
    v64_max_count = SOC_LPM_MAX_64BV6_COUNT(u);
    /*
     * V4 routes may have some half entries. These entries have only
     * one v4 route and other side is empty. For Half entry V4 
     * routes, one v4 route occupies one defip index.
     * No need to divide v4 routes in half entries by half. 
     */   
    v4_count = SOC_LPM_V4_COUNT(u) - SOC_LPM_V4_HALF_ENTRY_COUNT(u);
    v4_count = ((v4_count + 1) >> 1) + SOC_LPM_V4_HALF_ENTRY_COUNT(u);
    b64_count = SOC_LPM_64BV6_COUNT(u);
    if (soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        SOC_IF_ERROR_RETURN(soc_fb_lpm_table_sizes_get(u, &paired_tcam_size, 
                                                                      NULL));
        paired_v4_count = SOC_LPM128_STAT_V4_COUNT(u) - SOC_LPM128_STAT_V4_HALF_ENTRY_COUNT(u);
        paired_v4_count = ((paired_v4_count + 1) >> 1) + SOC_LPM128_STAT_V4_HALF_ENTRY_COUNT(u);
        paired_v64_count = SOC_LPM128_STAT_64BV6_COUNT(u) << 1;
        paired_v64_max_count = paired_tcam_size;
        if (!is_reserved) {
            SOC_IF_ERROR_RETURN(soc_lpm_used_128bv6_route_get(u, &b128_count));
            b128_count <<= 1;
            paired_count = paired_v64_max_count - paired_v64_count - 
                           paired_v4_count - b128_count;
        } else {
            paired_v64_max_count -= (max_v6_entries << 1);
            paired_count = paired_v64_max_count - paired_v4_count -
                           paired_v64_count;
        }
        paired_count >>= 1;
    } else {
        if (max_v6_entries % tcam_depth == 0) {
            paired_v4_count = 0;
        } else {
            paired_v4_count = (tcam_depth - (max_v6_entries % tcam_depth)) << 1;
        }
        if (v4_count > paired_v4_count) {
            v4_count -= paired_v4_count;
        } else {
            v4_count = 0;
        }
    }
    *entries = v64_max_count - b64_count - v4_count + paired_count;
    return SOC_E_NONE;
}

int
soc_lpm_free_128bv6_route_get(int u, int *entries)
{
    int v4_count = 0;
    int b64_count = 0;
    int b128_count = 0;
    int is_reserved = 0;
    int v128_max_count = 0;

    if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
        return SOC_E_UNAVAIL;
    }

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    SOC_IF_ERROR_RETURN(soc_lpm_max_128bv6_route_get(u, &v128_max_count));
    SOC_IF_ERROR_RETURN(soc_lpm_used_128bv6_route_get(u, &b128_count));
    if (soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        if (!is_reserved) {
            /*
             * V4 routes may have some half entries. These entries have only
             * one v4 route and other side is empty. For Half entry V4 
             * routes, one v4 route occupies one defip index.
             * No need to divide v4 routes in half entries by half. 
             */   
            v4_count = SOC_LPM128_STAT_V4_COUNT(u) - SOC_LPM128_STAT_V4_HALF_ENTRY_COUNT(u);
            v4_count = ((v4_count + 1) >> 1) + SOC_LPM128_STAT_V4_HALF_ENTRY_COUNT(u);
            b64_count = SOC_LPM128_STAT_64BV6_COUNT(u);
        }
    }        
    *entries = (v128_max_count << 1) - ((b64_count + b128_count) << 1) - 
               v4_count;
    *entries >>= 1;
    return SOC_E_NONE;
}

int
soc_lpm_used_v4_route_get(int u, int *entries)
{
    if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
        return SOC_E_UNAVAIL;
    }

    *entries = SOC_LPM_V4_COUNT(u);
    if (soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        *entries += SOC_LPM128_STAT_V4_COUNT(u);
    }
    return SOC_E_NONE;
}

int
soc_lpm_used_64bv6_route_get(int u, int *entries)
{
    if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
        return SOC_E_UNAVAIL;
    }

    *entries = SOC_LPM_64BV6_COUNT(u);
    if (soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        *entries += SOC_LPM128_STAT_64BV6_COUNT(u);
    }
    return SOC_E_NONE;
}

int
soc_lpm_used_128bv6_route_get(int u, int *entries)
{
    if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
        return SOC_E_UNAVAIL;
    }

    *entries = SOC_LPM_128BV6_COUNT(u);
    if (soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        *entries += SOC_LPM128_STAT_128BV6_COUNT(u);
    }
    return SOC_E_NONE;
}

void
soc_lpm_stat_128b_count_update(int u, int incr)
{
    if (!soc_feature(u, soc_feature_l3_shared_defip_table)) {
        return; 
    }

    if (soc_feature(u, soc_feature_l3_shared_defip_table)) {
        if (incr) {
            SOC_LPM_COUNT_INC(SOC_LPM_128BV6_COUNT(u));
        } else {
            SOC_LPM_COUNT_DEC(SOC_LPM_128BV6_COUNT(u));
        }
    }
}

#ifdef BCM_WARM_BOOT_SUPPORT
int
soc_fb_lpm128_reinit(int u, int idx, defip_entry_t *lpm_entry, 
                     defip_entry_t *lpm_entry_upr)
{
    int pfx = 0;
    int is_reserved = 0;
    soc_lpm128_state_p lpm_state_ptr = SOC_LPM128_STATE(u);
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(u);
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(u);
    soc_lpm_entry_type_t type = socLpmEntryType128BV6;

    if (!soc_feature(u, soc_feature_l3_lpm_scaling_enable)) {
        return SOC_E_NONE;
    }

    if (max_v6_entries == 0) {
        return SOC_E_NONE;
    }

    if (soc_feature(u, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    SOC_IF_ERROR_RETURN
        (_soc_fb_lpm128_prefix_length_get(u, lpm_entry, lpm_entry_upr, 
                                          &pfx));

    SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(u, pfx, &type));
    if (!is_reserved || type == socLpmEntryType128BV6) {
        lpm_state_ptr = SOC_LPM128_STATE(u);
    } else {
        lpm_state_ptr = SOC_LPM128_UNRESERVED_STATE(u);
    }

    if (SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) == 0) {
        SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx) = idx;
        SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = idx;
    } else {
        if (SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) != -1) { 
            SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = idx;
        } else {
            if ((type != socLpmEntryTypeV4) && ((idx / tcam_depth) & 1)) {
                return SOC_E_NONE;
            }
            if ((SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) + 1) != idx) {
                SOC_LPM128_STATE_START2(u, lpm_state_ptr, pfx) = idx;
                SOC_LPM128_STATE_END2(u, lpm_state_ptr, pfx) = idx;
            } else {
                SOC_LPM128_STATE_END1(u, lpm_state_ptr, pfx) = idx;
            }
        }
    }

    SOC_LPM128_STATE_VENT(u, lpm_state_ptr, pfx) += 1;
    LPM128_HASH_INSERT(u, lpm_entry, lpm_entry_upr, idx);
    SOC_LPM128_INDEX_TO_PFX_GROUP(u, idx) = pfx;
    if (type != socLpmEntryTypeV4) {
        SOC_LPM128_INDEX_TO_PFX_GROUP(u, idx + tcam_depth) = pfx;
    }

    return (SOC_E_NONE);
}

STATIC int
_soc_fb_lpm128_setup_pfx_state(int u, soc_lpm128_state_p lpm_state_ptr)
{
    int prev_idx = MAX_PFX128_INDEX;
    int idx = 0;
    int from_ent = -1;
    int v0 = 0, v1 = 0;
    uint32      e[SOC_MAX_MEM_FIELD_WORDS] = {0};

    if (NULL == lpm_state_ptr) {
       return SOC_E_INTERNAL;
    }

    SOC_LPM128_STATE_PREV(u, lpm_state_ptr, MAX_PFX128_INDEX) = -1;

    for (idx = MAX_PFX128_INDEX; idx > 0; idx--) {
       if (SOC_LPM128_STATE_START1(u, lpm_state_ptr, idx) == -1) {
           continue;
       }
       
       SOC_LPM128_STATE_PREV(u, lpm_state_ptr, idx) = prev_idx;
       SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, prev_idx) = idx;
    
       prev_idx = idx;
       if (SOC_LPM128_PFX_IS_V6_64(u, idx)) {
           SOC_LPM128_STAT_64BV6_COUNT(u) += SOC_LPM128_STATE_VENT(u, 
                                                           lpm_state_ptr, idx);
       }

       if (SOC_LPM128_PFX_IS_V6_128(u, idx)) {
           SOC_LPM128_STAT_128BV6_COUNT(u) += SOC_LPM128_STATE_VENT(u, 
                                                            lpm_state_ptr, idx);
       }

       if (SOC_LPM128_PFX_IS_V4(u, idx)) {
           if (SOC_LPM128_STATE_START2(u, lpm_state_ptr, idx) != -1) {
               from_ent = SOC_LPM128_STATE_END2(u, lpm_state_ptr, idx);
           } else {
               from_ent = SOC_LPM128_STATE_END1(u, lpm_state_ptr, idx);
           }
#ifdef FB_LPM_TABLE_CACHED
           SOC_IF_ERROR_RETURN(soc_mem_cache_invalidate(u, L3_DEFIPm,
                                                     MEM_BLOCK_ANY, from_ent));
#endif /* FB_LPM_TABLE_CACHED */
            SOC_IF_ERROR_RETURN(READ_L3_DEFIPm(u, MEM_BLOCK_ANY, from_ent, e));
            v0 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID0f);
            v1 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f);
            SOC_LPM128_STAT_V4_COUNT(u) += (SOC_LPM128_STATE_VENT(u, 
                                                      lpm_state_ptr, idx)) << 1;
            if ((v0 == 0) || (v1 == 0)) {
                SOC_LPM128_STAT_V4_COUNT(u) -= 1;
            }
            if ((v0 && (!(v1))) || ((!(v0) && v1))) {
                SOC_LPM_COUNT_INC(SOC_LPM128_STAT_V4_HALF_ENTRY_COUNT(u));
            }  
        }
    }

    SOC_LPM128_STATE_NEXT(u, lpm_state_ptr, prev_idx) = -1;

    return SOC_E_NONE;
}

int
soc_fb_lpm128_reinit_done(int unit, int ipv6)
{
    int paired_table_size = 0;
    int defip_table_size = 0;
    int i = 0;
    int pfx;
    int is_reserved = 0;
    int start_idx = -1;
    int end_idx = -1;
    int fent_incr = 0;
    int found = 0;
    int first_v4_index = -1;
    int split_done = 0;
    int next_pfx = -1;
    soc_lpm_entry_type_t type = socLpmEntryTypeV4;
    soc_lpm128_state_p lpm_state_ptr = SOC_LPM128_STATE(unit);
    int max_v6_entries = SOC_L3_DEFIP_MAX_128B_ENTRIES(unit);
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit);

    if (!soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        return SOC_E_NONE;
    }

    if (!ipv6) {
        return SOC_E_NONE;
    }

    if (max_v6_entries == 0) {
        return SOC_E_NONE;
    }

    if (soc_feature(unit, soc_feature_l3_lpm_128b_entries_reserved)) {
        is_reserved = 1;
    }

    SOC_IF_ERROR_RETURN(soc_fb_lpm_table_sizes_get(unit, &paired_table_size, 
                                                   &defip_table_size));
    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF.
     */
    if (SOC_URPF_STATUS_GET(unit) &&
        !soc_feature(unit, soc_feature_l3_defip_advanced_lookup)) {
        max_v6_entries >>= 1;
    }

    lpm_state_ptr = SOC_LPM128_STATE(unit);
    if (lpm_state_ptr == NULL) {
        return SOC_E_NONE;
    }
    _soc_fb_lpm128_setup_pfx_state(unit, lpm_state_ptr);

    pfx = MAX_PFX128_INDEX;
    next_pfx = SOC_LPM128_STATE_NEXT(unit, lpm_state_ptr, pfx);
    start_idx = 0;
    if (!is_reserved) {
        end_idx = paired_table_size;
        if (next_pfx == -1) {
            SOC_LPM128_STATE_FENT(unit, lpm_state_ptr, pfx) = end_idx;
        }
    } else {
        end_idx = max_v6_entries;
        if (next_pfx == -1) {
            SOC_LPM128_STATE_FENT(unit, lpm_state_ptr, pfx) = end_idx << 1;
        }
    }

    for (i = start_idx; (next_pfx != -1) && i < end_idx; i++) {
        if (((i / tcam_depth) & 1) && (i % tcam_depth == 0)) {
            if (is_reserved) {
                i += max_v6_entries;
            }
            if (first_v4_index != -1 && !split_done) {
                i = first_v4_index + tcam_depth;
                split_done = 1;
            }
        }
        if (SOC_LPM128_INDEX_TO_PFX_GROUP(unit, i) != -1) {
            pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(unit, i);
            SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(unit, pfx, &type));
            if (type == socLpmEntryTypeV4) {
                if (first_v4_index == -1) {
                   first_v4_index = SOC_LPM128_STATE_START1(u, lpm_state_ptr, pfx); 
                }
                fent_incr = 1;
            } else {
                fent_incr = 2;
            }
            if (pfx != MAX_PFX128_INDEX) {
                found = 1;
            }
        } else {
            if (type != socLpmEntryTypeV4 && ((i / tcam_depth) & 1)) {
                continue;
            }
            SOC_LPM128_STATE_FENT(unit, lpm_state_ptr, pfx) += fent_incr;
        }
    }

    if (found) {
        SOC_LPM128_STATE_FENT(unit, lpm_state_ptr, MAX_PFX128_INDEX) = 0;
    }
    lpm_state_ptr = SOC_LPM128_UNRESERVED_STATE(unit);
    if (lpm_state_ptr == NULL) {
       return SOC_E_NONE;
    }
    _soc_fb_lpm128_setup_pfx_state(unit, lpm_state_ptr);
    pfx = MAX_PFX128_INDEX;
    found = 0;
    start_idx = ((max_v6_entries / tcam_depth) * tcam_depth * 2) + 
                max_v6_entries % tcam_depth;
    next_pfx = SOC_LPM128_STATE_NEXT(unit, lpm_state_ptr, pfx);
    if (next_pfx == -1) {
        SOC_LPM128_STATE_FENT(unit, lpm_state_ptr, pfx) = (paired_table_size - 
                                                         (max_v6_entries << 1));
    }
    for (i = start_idx; (next_pfx != -1) && i < paired_table_size; i++) {
        if (((i / tcam_depth) & 1) && (i % tcam_depth == 0)) {
            i += (max_v6_entries % tcam_depth);
        }
        if (SOC_LPM128_INDEX_TO_PFX_GROUP(unit, i) != -1) {
            pfx = SOC_LPM128_INDEX_TO_PFX_GROUP(unit, i);
            SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(unit, pfx, &type));
            if ((type != socLpmEntryTypeV4) && ((i / tcam_depth) & 1)) {
            }
            SOC_IF_ERROR_RETURN(_lpm128_find_pfx_type(unit, pfx, &type));
            if (type == socLpmEntryTypeV4) {
                fent_incr = 1;
            } else {
                fent_incr = 2;
            }
            if (pfx != MAX_PFX128_INDEX) {
                found = 1;
            }
        } else {
            SOC_LPM128_STATE_FENT(unit, lpm_state_ptr, pfx) += fent_incr;
        }
    }

    if (found) {
        SOC_LPM128_STATE_FENT(unit, lpm_state_ptr, MAX_PFX128_INDEX) = 0;
    }

    return SOC_E_NONE;
}
#endif
#endif
#endif /* BCM_FIREBOLT_SUPPORT */
#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_L3)
soc_kt_lpm_ipv6_info_t *
soc_kt_lpm_ipv6_info_get(int unit)
{
    return kt_lpm_ipv6_info[unit];
}
#endif


