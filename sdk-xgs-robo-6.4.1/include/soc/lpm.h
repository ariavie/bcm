/*
 * $Id: lpm.h,v 1.15 Broadcom SDK $
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
 */

#ifndef _LPM_H_
#define _LPM_H_
#include <shared/l3.h>

/* 
 *  Reserved VRF values 
 */
#define SOC_L3_VRF_OVERRIDE  _SHR_L3_VRF_OVERRIDE /* Matches before vrf */
                                                  /* specific entries.  */
#define SOC_L3_VRF_GLOBAL    _SHR_L3_VRF_GLOBAL   /* Matches after vrf  */
                                                  /* specific entries.  */
#define SOC_L3_VRF_DEFAULT   _SHR_L3_VRF_DEFAULT  /* Default vrf id.    */ 

#define SOC_APOLLO_L3_DEFIP_URPF_SIZE (0x800)
#define SOC_APOLLO_B0_L3_DEFIP_URPF_SIZE (0xC00)

#define SOC_LPM_LOCK(u)             soc_mem_lock(u, L3_DEFIPm)
#define SOC_LPM_UNLOCK(u)           soc_mem_unlock(u, L3_DEFIPm)

#define PRESERVE_HIT                TRUE 

#define FB_LPM_HASH_INDEX_NULL  (0xFFFF)
#define FB_LPM_HASH_INDEX_MASK  (0x7FFF)
#define FB_LPM_HASH_IPV6_MASK   (0x8000)

#define _SOC_HASH_L3_VRF_GLOBAL   0
#define _SOC_HASH_L3_VRF_OVERRIDE 1
#define _SOC_HASH_L3_VRF_DEFAULT  2
#define _SOC_HASH_L3_VRF_SPECIFIC 3

#ifdef BCM_FIREBOLT_SUPPORT
#define IPV6_PFX_ZERO                33
#define IPV4_PFX_ZERO                (3 * (64 + 32 + 2 + 1)) 
#define MAX_PFX_ENTRIES              (2 * (IPV4_PFX_ZERO))
#define MAX_PFX_INDEX                (MAX_PFX_ENTRIES - 1)
#define SOC_LPM_INIT_CHECK(u)        (soc_lpm_state[(u)] != NULL)
#define SOC_LPM_STATE(u)             (soc_lpm_state[(u)])
#define SOC_LPM_STATE_START(u, pfx)  (soc_lpm_state[(u)][(pfx)].start)
#define SOC_LPM_STATE_END(u, pfx)    (soc_lpm_state[(u)][(pfx)].end)
#define SOC_LPM_STATE_PREV(u, pfx)  (soc_lpm_state[(u)][(pfx)].prev)
#define SOC_LPM_STATE_NEXT(u, pfx)  (soc_lpm_state[(u)][(pfx)].next)
#define SOC_LPM_STATE_VENT(u, pfx)  (soc_lpm_state[(u)][(pfx)].vent)
#define SOC_LPM_STATE_FENT(u, pfx)  (soc_lpm_state[(u)][(pfx)].fent)
#define SOC_LPM_PFX_IS_V4(u, pfx)   (pfx) >= IPV4_PFX_ZERO ? TRUE : FALSE  
#define SOC_LPM_PFX_IS_V6_64(u, pfx) (pfx) < IPV4_PFX_ZERO  ? TRUE : FALSE  
#define SOC_LPM128_PFX_IS_V6_128(u, pfx) \
        ((pfx) > (MAX_PFX_64_OFFSET + MAX_PFX_32_OFFSET) ? TRUE : FALSE)
#define SOC_LPM128_PFX_IS_V6_64(u, pfx) \
    (((pfx) >= MAX_PFX_32_OFFSET && ((pfx) < MAX_PFX_128_OFFSET)) ? TRUE : FALSE)
#define SOC_LPM128_PFX_IS_V4(u, pfx) ((pfx) < MAX_PFX_32_OFFSET ? TRUE : FALSE)
#define SOC_LPM_STAT_INIT_CHECK(u) (soc_lpm_stat[u] != NULL) 
#define SOC_LPM_V4_COUNT(u) (soc_lpm_stat[u]->used_v4_count)
#define SOC_LPM_64BV6_COUNT(u) (soc_lpm_stat[u]->used_64b_count)
#define SOC_LPM_128BV6_COUNT(u) (soc_lpm_stat[u]->used_128b_count)
#define SOC_LPM_MAX_V4_COUNT(u) (soc_lpm_stat[u]->max_v4_count)
#define SOC_LPM_MAX_64BV6_COUNT(u) (soc_lpm_stat[u]->max_64b_count)
#define SOC_LPM_MAX_128BV6_COUNT(u) (soc_lpm_stat[u]->max_128b_count)
#define SOC_LPM_V4_HALF_ENTRY_COUNT(u) (soc_lpm_stat[u]->half_entry_count)
#define SOC_LPM_COUNT_INC(val) (val)++
#define SOC_LPM_COUNT_DEC(val) (val)--

typedef struct soc_lpm_stat_s {
    uint16 used_v4_count;
    uint16 used_64b_count;
    uint16 used_128b_count;
    uint16 max_v4_count;
    uint16 max_64b_count;
    uint16 max_128b_count;
    uint16 half_entry_count;
} soc_lpm_stat_t, *soc_lpm_stat_p;

typedef struct soc_lpm_state_s {
    int start;  /* start index for this prefix length */
    int end;    /* End index for this prefix length */
    int prev;   /* Previous (Lo to Hi) prefix length with non zero entry count*/
    int next;   /* Next (Hi to Lo) prefix length with non zero entry count */
    int vent;   /* valid entries */
    int fent;   /* free entries */
} soc_lpm_state_t, *soc_lpm_state_p;

/*
 * The idea of these offsets is to create unique space for each prefix type
 * The precedence is as follows
 * 128BV6 > 64B V6 > V4 irrespective of whether the entry is public or private
 * i.e any prefix of V4 is lesser than min prefix of 64B V6 and any prefix of
 * 64B V6 is lesser than 128B V6 from software perspective 
 */
#define MAX_PFX_128_OFFSET              (3 * 129) 
#define MAX_PFX_64_OFFSET               (3 * 65)
#define MAX_PFX_32_OFFSET               (3 * 33)
#define MAX_PFX128_ENTRIES              MAX_PFX_128_OFFSET + \
                                        MAX_PFX_64_OFFSET + \
                                        MAX_PFX_32_OFFSET + 2
#define MAX_PFX128_INDEX                (MAX_PFX128_ENTRIES - 1)
#define SOC_LPM128_STATE_TABLE(u)       (soc_lpm128_state_table[u])
#define SOC_LPM128_STATE_TABLE_INIT_CHECK(u) \
        (soc_lpm128_state_table[(u)] != NULL)
#define SOC_LPM128_STATE_TABLE_TOTAL_COUNT(u) \
        (soc_lpm128_state_table[u]->total_count)

#define SOC_LPM128_INIT_CHECK(u)        \
        (soc_lpm128_state_table[(u)]->soc_lpm_state != NULL)
#define SOC_LPM128_STATE(u)             \
        (soc_lpm128_state_table[(u)]->soc_lpm_state)
#define SOC_LPM128_STATE_START1(u, lpm_state, pfx) (lpm_state[pfx].start1)
#define SOC_LPM128_STATE_START2(u, lpm_state, pfx) (lpm_state[pfx].start2)
#define SOC_LPM128_STATE_END1(u, lpm_state, pfx) (lpm_state[pfx].end1)
#define SOC_LPM128_STATE_END2(u, lpm_state, pfx) (lpm_state[pfx].end2)
#define SOC_LPM128_STATE_PREV(u, lpm_state, pfx) (lpm_state[pfx].prev)
#define SOC_LPM128_STATE_NEXT(u, lpm_state, pfx) (lpm_state[pfx].next)
#define SOC_LPM128_STATE_VENT(u, lpm_state, pfx) (lpm_state[pfx].vent)
#define SOC_LPM128_STATE_FENT(u, lpm_state, pfx) (lpm_state[pfx].fent)

#define SOC_LPM128_UNRESERVED_INIT_CHECK(u)        \
        (soc_lpm128_state_table[(u)]->soc_lpm_unreserved_state != NULL)
#define SOC_LPM128_UNRESERVED_STATE(u)             \
        (soc_lpm128_state_table[(u)]->soc_lpm_unreserved_state)

#define SOC_LPM128_INDEX_TO_PFX_GROUP_TABLE(u) \
        soc_lpm128_index_to_pfx_group[u]
#define SOC_LPM128_INDEX_TO_PFX_GROUP(u, index) \
        soc_lpm128_index_to_pfx_group[u][index]

#define SOC_LPM128_STAT_V4_COUNT(u) \
        (soc_lpm128_state_table[(u)]->soc_lpm_stat.used_v4_count)
#define SOC_LPM128_STAT_64BV6_COUNT(u) \
        (soc_lpm128_state_table[(u)]->soc_lpm_stat.used_64b_count)
#define SOC_LPM128_STAT_128BV6_COUNT(u) \
        (soc_lpm128_state_table[(u)]->soc_lpm_stat.used_128b_count)
#define SOC_LPM128_STAT_MAX_V4_COUNT(u) \
        (soc_lpm128_state_table[(u)]->soc_lpm_stat.max_v4_count)
#define SOC_LPM128_STAT_MAX_64BV6_COUNT(u) \
        (soc_lpm128_state_table[(u)]->soc_lpm_stat.max_64b_count)
#define SOC_LPM128_STAT_MAX_128BV6_COUNT(u) \
        (soc_lpm128_state_table[(u)]->soc_lpm_stat.max_128b_count)
#define SOC_LPM128_STAT_V4_HALF_ENTRY_COUNT(u) \
        (soc_lpm128_state_table[(u)]->soc_lpm_stat.half_entry_count)
/*
 * This structure maintains 128B V6, 64B V6, V4 prefix groups
 * Ideally all entries of same length should be contigous.
 * But for V4, the entries may not be continous because V6 entries occupy odd
 * TCAM entries as well. So if the V4 prefix is not continous, start2 and end2
 * are used to represent this split. 
 * In case of V6 entries, if end1 is crossing tcam boundary, then we add offset
 * of tcam_depth(1024) as we start V6 entry in the odd TCAM. However start2,
 * end2 are not set.
 * fent: For V6 fent of 2 represents 2 entries end1 + 1 and end1 + 1 + 1024. 
 *       For V4 prefixes fent of 2 represents entries end1/end2 + 1, 
 *       end1/end2 + 2
 * vent: For V6 or V4 if we add a entry then vent will be increased by 1.
 */

typedef struct soc_lpm128_state_s {
    int start1;  /* start index for this prefix length */
    int start2;  /* start index for this prefix length */
    int end1;    /* End index for this prefix length */
    int end2;    /* End index for this prefix length */
    int prev;   /* Previous (Lo to Hi) prefix length with non zero entry count*/
    int next;   /* Next (Hi to Lo) prefix length with non zero entry count */
    int vent;   /* valid entries */
    int fent;   /* free entries */
} soc_lpm128_state_t, *soc_lpm128_state_p;

typedef struct soc_lpm128_table_s {
    soc_lpm128_state_p soc_lpm_state;
    soc_lpm128_state_p soc_lpm_unreserved_state;
    uint16 total_count;
    soc_lpm_stat_t soc_lpm_stat;
} soc_lpm128_table_t;

typedef enum soc_lpm_entry_type_s {
    socLpmEntryTypeV4 = 0x1,
    socLpmEntryType64BV6 = 0x02,
    socLpmEntryType128BV6 = 0x04,
    socLpmEntryTypeLast = 0x08
} soc_lpm_entry_type_t;

extern soc_lpm_state_p soc_lpm_state[SOC_MAX_NUM_DEVICES];
extern soc_lpm128_table_t *soc_lpm128_state_table[SOC_MAX_NUM_DEVICES];

extern int soc_fb_lpm_init(int u);
extern int soc_fb_lpm_deinit(int u);
extern int soc_fb_lpm_insert(int unit, void *entry);
extern int soc_fb_lpm_insert_index(int unit, void *entry, int index);
extern int soc_fb_lpm_delete(int u, void *key_data);
extern int soc_fb_lpm_delete_index(int u, void *key_data, int index);
extern int soc_fb_lpm_match(int u, void *key_data, void *e, int *index_ptr);
extern int soc_fb_lpm_ipv4_delete_index(int u, int index);
extern int soc_fb_lpm_ipv6_delete_index(int u, int index);
extern int soc_fb_lpm_ip4entry0_to_0(int u, void *src, void *dst, int copy_hit);
extern int soc_fb_lpm_ip4entry1_to_1(int u, void *src, void *dst, int copy_hit);
extern int soc_fb_lpm_ip4entry0_to_1(int u, void *src, void *dst, int copy_hit);
extern int soc_fb_lpm_ip4entry1_to_0(int u, void *src, void *dst, int copy_hit);
extern int soc_fb_lpm_vrf_get(int unit, void *lpm_entry, int *vrf);
extern int soc_fb_lpm128_init(int u);
extern int soc_fb_lpm128_deinit(int u);
extern int soc_fb_get_largest_prefix(int u, int ipv6, void *e,
                              int *index, int *pfx_len, int* count);
extern int soc_fb_lpm128_insert(int u, void *hw_entry_lwr, void *hw_entry_upr,
                                int *index);
extern int soc_fb_lpm128_match(int u,
               void *key_data,
               void *key_data_upr,
               void *e,         /* return entry data if found */
               void *eupr,         /* return entry data if found */
               int *index_ptr,
               int *pfx,
               soc_lpm_entry_type_t *type);
extern int soc_fb_lpm128_delete(int u, void *key_data, void* key_data_upr);
extern int soc_fb_lpm_tcam_pair_count_get(int u, int *tcam_pair_count);
extern int soc_fb_lpm128_get_smallest_movable_prefix(int u, int ipv6, void *e,
                                                    void *eupr, int *index,
                                                    int *pfx_len, int *count);
int soc_fb_lpm128_is_v4_64b_allowed_in_paired_tcam(int unit);
int soc_fb_lpm_table_sizes_get(int unit, int *paired_table_size,
                                int *defip_table_size);
int soc_lpm_max_v4_route_get(int u, int *entries);
int soc_lpm_max_64bv6_route_get(int u, int *entries);
int soc_lpm_max_128bv6_route_get(int u, int *entries);
int soc_lpm_free_v4_route_get(int u, int *entries);
int soc_lpm_free_64bv6_route_get(int u, int *entries);
int soc_lpm_free_128bv6_route_get(int u, int *entries);
int soc_lpm_used_v4_route_get(int u, int *entries);
int soc_lpm_used_64bv6_route_get(int u, int *entries);
int soc_lpm_used_128bv6_route_get(int u, int *entries);
int soc_fb_lpm_stat_init(int u);
int soc_fb_lpm128_stat_init(int u);
void soc_lpm_stat_128b_count_update(int u, int incr);

#ifdef BCM_KATANA_SUPPORT

typedef struct soc_kt_lpm_ipv6_cfg_s {
    uint32 sip_start_offset;
    uint32 dip_start_offset;
    uint32 depth;
} soc_kt_lpm_ipv6_cfg_t;

typedef struct soc_kt_lpm_info_s {
    soc_kt_lpm_ipv6_cfg_t ipv6_128b;
    soc_kt_lpm_ipv6_cfg_t ipv6_64b;
} soc_kt_lpm_ipv6_info_t;

extern soc_kt_lpm_ipv6_info_t *soc_kt_lpm_ipv6_info_get(int unit);
#endif
#ifdef BCM_WARM_BOOT_SUPPORT
extern int soc_fb_lpm_reinit(int unit, int idx, defip_entry_t *lpm_entry);
extern int soc_fb_lpm_reinit_done(int unit, int ipv6);
extern int soc_fb_lpm128_reinit(int unit, int idx, defip_entry_t *lpm_entry,
                                defip_entry_t *lpm_entry_upr);
extern int soc_fb_lpm128_reinit_done(int unit, int ipv6);

#else
#define soc_fb_lpm_reinit(u, t, s) (SOC_E_UNAVAIL)
#define soc_fb_lpm_reinit_done(u, i) (SOC_E_UNAVAIL)
#define soc_fb_lpm128_reinit(u, i, l, l1) (SOC_E_UNAVAIL)
#define soc_fb_lpm128_reinit_done(u, i) (SOC_E_UNAVAIL)
#endif /* BCM_WARM_BOOT_SUPPORT */
  	 
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void soc_fb_lpm_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */
#endif	/* BCM_FIREBOLT_SUPPORT */

#endif	/* !_LPM_H_ */
