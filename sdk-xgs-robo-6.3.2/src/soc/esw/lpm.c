/* $Id: lpm.c 1.57 Broadcom SDK $
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

#include <soc/mem.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/lpm.h>
#include <shared/util.h>
#include <shared/avl.h>
#include <shared/l3.h>

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
#if defined(BCM_KATANA2_SUPPORT)
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, DEFAULTROUTE0f);
    SOC_LPM_CACHE_FIELD_CREATE(L3_DEFIPm, DEFAULTROUTE1f);
#endif 
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
typedef struct _soc_fb_lpm_hash_s {
    int         unit;
    int         entry_count;    /* Number entries in hash table */
    int         index_count;    /* Hash index max value + 1 */
    uint16      *table;         /* Hash table with 16 bit index */
    uint16      *link_table;    /* To handle collisions */
} _soc_fb_lpm_hash_t;

typedef uint32 _soc_fb_lpm_hash_entry_t[6];
typedef int (*_soc_fb_lpm_hash_compare_fn)(_soc_fb_lpm_hash_entry_t key1,
                                           _soc_fb_lpm_hash_entry_t key2);
static _soc_fb_lpm_hash_t *_fb_lpm_hash_tab[SOC_MAX_NUM_DEVICES];

static uint32 fb_lpm_hash_index_mask = 0;

#define SOC_LPM_STATE_HASH(u)           (_fb_lpm_hash_tab[(u)])

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


/* #define FB_LPM_DEBUG*/
#ifdef FB_LPM_DEBUG      
#define H_INDEX_MATCH(str, tab_index, match_index)      \
        soc_cm_debug(DK_ERR, "%s index: H %d A %d\n",   \
                     str, (int)tab_index, match_index)
#else
#define H_INDEX_MATCH(str, tab_index, match_index)
#endif

#define LPM_NO_MATCH_INDEX 0x4000
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
        soc_cm_debug(DK_ERR, "\ndel  index: H %d error %d\n", index, rv);
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

#define INDEX_ADD(hash, hash_idx, new_idx)                      \
    hash->link_table[new_idx & fb_lpm_hash_index_mask] =        \
        hash->table[hash_idx];                                  \
    hash->table[hash_idx] = new_idx

#define INDEX_ADD_LINK(hash, t_index, new_idx)                  \
    hash->link_table[new_idx & fb_lpm_hash_index_mask] =        \
        hash->link_table[t_index & fb_lpm_hash_index_mask];     \
    hash->link_table[t_index & fb_lpm_hash_index_mask] = new_idx

#define INDEX_UPDATE(hash, hash_idx, old_idx, new_idx)          \
    hash->table[hash_idx] = new_idx;                            \
    hash->link_table[new_idx & fb_lpm_hash_index_mask] =        \
        hash->link_table[old_idx & fb_lpm_hash_index_mask];     \
    hash->link_table[old_idx & fb_lpm_hash_index_mask] = FB_LPM_HASH_INDEX_NULL

#define INDEX_UPDATE_LINK(hash, prev_idx, old_idx, new_idx)             \
    hash->link_table[prev_idx & fb_lpm_hash_index_mask] = new_idx;      \
    hash->link_table[new_idx & fb_lpm_hash_index_mask] =                \
        hash->link_table[old_idx & fb_lpm_hash_index_mask];             \
    hash->link_table[old_idx & fb_lpm_hash_index_mask] = FB_LPM_HASH_INDEX_NULL


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
#define INDEX_DELETE(hash, hash_idx, del_idx)                   \
    hash->table[hash_idx] =                                     \
        hash->link_table[del_idx & fb_lpm_hash_index_mask];     \
    hash->link_table[del_idx & fb_lpm_hash_index_mask] =        \
        FB_LPM_HASH_INDEX_NULL

#define INDEX_DELETE_LINK(hash, prev_idx, del_idx)              \
    hash->link_table[prev_idx & fb_lpm_hash_index_mask] =       \
        hash->link_table[del_idx & fb_lpm_hash_index_mask];     \
    hash->link_table[del_idx & fb_lpm_hash_index_mask] =        \
        FB_LPM_HASH_INDEX_NULL

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

#if defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_KATANA2(u)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src,DEFAULTROUTE0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, DEFAULTROUTE0f, ipv4a);
    }
#endif


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

#if defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_KATANA2(u)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src,DEFAULTROUTE0f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, DEFAULTROUTE1f, ipv4a);
    }
#endif

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
    
#if defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_KATANA2(u)) {
        ipv4a = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, src,DEFAULTROUTE1f);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, dst, DEFAULTROUTE0f, ipv4a);
    }
#endif


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

    if (! soc_cm_debug_check(DK_L3 | DK_SOCMEM | DK_VERBOSE)) {
        return;
    }
    for(i = max_pfx_len; i >= 0 ; i--) {
        if ((i != MAX_PFX_INDEX) && (SOC_LPM_STATE_START(u, i) == -1)) {
            continue;
        }
        soc_cm_debug(DK_L3 | DK_SOCMEM | DK_VERBOSE,
        "PFX = %d P = %d N = %d START = %d END = %d VENT = %d FENT = %d\n",
        i,
        SOC_LPM_STATE_PREV(u, i),
        SOC_LPM_STATE_NEXT(u, i),
        SOC_LPM_STATE_START(u, i),
        SOC_LPM_STATE_END(u, i),
        SOC_LPM_STATE_VENT(u, i),
        SOC_LPM_STATE_FENT(u, i));
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
#if defined(BCM_TRIUMPH3_SUPPORT)
      else if (SOC_IS_TRIUMPH3(u) && 
               soc_feature(u, soc_feature_l3_reduced_defip_table) && 
               soc_property_get(u, spn_IPV6_LPM_128B_ENABLE, 1)) {  
         src_tcam_offset = _SOC_TR3_DEFIP_TCAM_DEPTH;
    } 
#endif
    else {
         src_tcam_offset = (soc_mem_index_count(u, L3_DEFIPm) >> 1);
    }

    ipv6 = SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, MODE0f);

    #if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANAX(u)) {
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
#if defined(BCM_KATANA2_SUPPORT)
            if (SOC_IS_KATANA2(u)) {
                SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, DEFAULTROUTE0f, (!mask0) ? 1 : 0);
            }
#endif
        }
        if (SOC_MEM_OPT_F32_GET(u, L3_DEFIPm, e, VALID1f)) {
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, RPE1f, (!mask1) ? 1 : 0); 
#if defined(BCM_KATANA2_SUPPORT)
            if (SOC_IS_KATANA2(u)) {
                SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, DEFAULTROUTE1f, (!mask0) ? 1 : 0);
            }
#endif 
        }
    } else {
        def_gw_flag = ((!mask0) &&  (!mask1)) ? 1 : 0;
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, RPE0f, def_gw_flag);
        SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, RPE1f, def_gw_flag); 
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(u)) {
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, DEFAULTROUTE0f, def_gw_flag);
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, e, DEFAULTROUTE1f, def_gw_flag);
        }
#endif
    }
    /* Write entry to the second half of the tcam. */
#if defined(BCM_KATANA2_SUPPORT)
    if (SOC_IS_KATANA2(u)) {
        return WRITE_L3_DEFIPm(u, MEM_BLOCK_ANY, (index), e);
    }
#endif
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

    if (SOC_URPF_STATUS_GET(unit)) {
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
    uint32      entry[SOC_MAX_MEM_FIELD_WORDS];

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

        SOC_LPM_STATE_FENT(u, pfx) =  (SOC_LPM_STATE_FENT(u, curr_pfx) + 1) / 2;
        SOC_LPM_STATE_FENT(u, curr_pfx) -= SOC_LPM_STATE_FENT(u, pfx);
        SOC_LPM_STATE_START(u, pfx) =  SOC_LPM_STATE_END(u, curr_pfx) +
                                       SOC_LPM_STATE_FENT(u, curr_pfx) + 1;
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
                SOC_LPM_STATE_FENT(u, curr_pfx) += SOC_LPM_STATE_FENT(u, pfx);
                SOC_LPM_STATE_FENT(u, pfx) = 0;
                SOC_LPM_STATE_START(u, pfx) = -1;
                SOC_LPM_STATE_END(u, pfx) = -1;
            }
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
            return SOC_E_FULL;
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
        } else {
            if (slot & 1) {
                rv = soc_fb_lpm_ip4entry0_to_1(u, ef, et, PRESERVE_HIT);
            } else {
                rv = soc_fb_lpm_ip4entry0_to_0(u, ef, et, PRESERVE_HIT);
            }
            SOC_MEM_OPT_F32_SET(u, L3_DEFIPm, ef, VALID0f, 0);
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
            soc_cm_debug(DK_ERR , "\nsoc_fb_lpm_locate: HASH %d AVL %d\n", *index_ptr, key_index);
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


/*
 * Initialize the start/end tracking pointers for each prefix length
 */
int
soc_fb_lpm_init(int u)
{
    int max_pfx_len;
    int defip_table_size;
    int pfx_state_size;
    int i;    
#if defined(BCM_KATANA_SUPPORT)
    uint32 l3_defip_tbl_size;
    int ipv6_lpm_128b_enable;
#endif

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
#if defined(BCM_KATANA2_SUPPORT)
        if (SOC_IS_KATANA2(u)) {
            SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, DEFAULTROUTE0f);
            SOC_LPM_CACHE_FIELD_ASSIGN(u, soc_lpm_field_cache_state[u] ,L3_DEFIPm, DEFAULTROUTE1f);
        }
#endif
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
            defip_table_size >>= 1;
        }
    }

    SOC_LPM_STATE_FENT(u, (MAX_PFX_INDEX)) = defip_table_size;
#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANAX(u)) {
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

    if (_soc_fb_lpm_hash_create(u, defip_table_size * 2, defip_table_size,
                                &SOC_LPM_STATE_HASH(u)) < 0) {
        SOC_LPM_UNLOCK(u);
        return SOC_E_MEMORY;
    }
#endif

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
    if (SOC_IS_KATANAX(u)) {
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

    SOC_LPM_UNLOCK(u);

    return(SOC_E_NONE);
}

#ifdef BCM_WARM_BOOT_SUPPORT


int
soc_fb_lpm_reinit_done(int unit)
{
    int idx;
    int prev_idx = MAX_PFX_INDEX;


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
            defip_table_size >>= 1;
        }
    } 

    SOC_LPM_STATE_PREV(unit, MAX_PFX_INDEX) = -1;

    for (idx = MAX_PFX_INDEX; idx > 0 ; idx--) {
        if (-1 == SOC_LPM_STATE_START(unit, idx)) {
            continue;
        }

        SOC_LPM_STATE_PREV(unit, idx) = prev_idx;
        SOC_LPM_STATE_NEXT(unit, prev_idx) = idx;

        SOC_LPM_STATE_FENT(unit, prev_idx) =                    \
                          SOC_LPM_STATE_START(unit, idx) -      \
                          SOC_LPM_STATE_END(unit, prev_idx) - 1;
        prev_idx = idx;
        
    }

    SOC_LPM_STATE_NEXT(unit, prev_idx) = -1;
    SOC_LPM_STATE_FENT(unit, prev_idx) =                       \
                          defip_table_size -                   \
                          SOC_LPM_STATE_END(unit, prev_idx) - 1;

#if defined(BCM_KATANA_SUPPORT)
    if (SOC_IS_KATANAX(unit)) {
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

    soc_cm_print("\n    FB LPM State -\n");
    soc_cm_print("        Prefix entries : %d\n", MAX_PFX_ENTRIES);

    lpm_state = soc_lpm_state[unit];
    if (lpm_state == NULL) {
        return;
    }

    for (i = 0; i < MAX_PFX_ENTRIES; i++) {
        if (lpm_state[i].vent != 0 ) {
            soc_cm_print("      Prefix %d\n", i);
            soc_cm_print("        Start : %d\n", lpm_state[i].start);
            soc_cm_print("        End   : %d\n", lpm_state[i].end);
            soc_cm_print("        Prev  : %d\n", lpm_state[i].prev);
            soc_cm_print("        Next  : %d\n", lpm_state[i].next);
            soc_cm_print("        Valid Entries : %d\n", lpm_state[i].vent);
            soc_cm_print("        Free  Entries : %d\n", lpm_state[i].fent);
            if (SOC_LPM_PFX_IS_V4(unit, i)) {
                soc_cm_print("        Type  : IPV4\n");
            } else if (SOC_LPM_PFX_IS_V6_64(unit, i)) {
                soc_cm_print("        Type  : 64B IPV6\n");
            } else {
                soc_cm_print("        Type  : MAX\n");
            }    
        }
    }
    
    soc_cm_print("      Prefix %d\n", i);
    soc_cm_print("        Start : %d\n", lpm_state[i].start);
    soc_cm_print("        End   : %d\n", lpm_state[i].end);
    soc_cm_print("        Prev  : %d\n", lpm_state[i].prev);
    soc_cm_print("        Next  : %d\n", lpm_state[i].next);
    soc_cm_print("        Valid Entries : %d\n", lpm_state[i].vent);
    soc_cm_print("        Free  Entries : %d\n", lpm_state[i].fent);
    if (SOC_LPM_PFX_IS_V4(unit, i)) {
         soc_cm_print("        Type  : IPV4\n");
    } else if (SOC_LPM_PFX_IS_V6_64(unit, i)) {
         soc_cm_print("        Type  : 64B IPV6\n");
    } else {
         soc_cm_print("        Type  : MAX\n");
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
    int         index;
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
        soc_cm_debug(DK_L3 | DK_SOCMEM, "\nsoc_fb_lpm_insert: %d %d\n",
                     index, pfx);
        if (!found) {
            LPM_HASH_INSERT(u, entry_data, index);
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
        soc_cm_debug(DK_L3 | DK_SOCMEM, "\nsoc_fb_lpm_delete: %d %d\n",
                     index, pfx);
        LPM_HASH_DELETE(u, key_data, index);
        rv = _lpm_free_slot_delete(u, pfx, ipv6, e, index);
        LPM_AVL_DELETE(u, key_data, index);
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
            soc_cm_debug(DK_L3 | DK_SOCMEM,
                         "\nsoc_fb_lpm_ipv4_delete_index: %d %d\n",
                         index, pfx);
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
            soc_cm_debug(DK_L3 | DK_SOCMEM,
                         "\nsoc_fb_lpm_ipv4_delete_index: %d %d\n",
                         index, pfx);
            LPM_HASH_DELETE(u, e, index);
            rv = _lpm_free_slot_delete(u, pfx, TRUE,e, index);
            LPM_AVL_DELETE(u, e, index);
        }
        soc_fb_lpm_state_dump(u);
    }
    SOC_LPM_UNLOCK(u);
    return(rv);
}
#endif
#endif /* BCM_FIREBOLT_SUPPORT */
#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_L3)
soc_kt_lpm_ipv6_info_t *
soc_kt_lpm_ipv6_info_get(int unit)
{
    return kt_lpm_ipv6_info[unit];
}
#endif


