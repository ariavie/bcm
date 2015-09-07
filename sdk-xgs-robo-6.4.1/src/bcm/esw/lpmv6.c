/*
 * $Id: lpmv6.c,v 1.12 Broadcom SDK $
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
 * File:    lpv6.c
 * Purpose: L3 LPM V6 function implementations
 */

#include <soc/defs.h>
#include <shared/bsl.h>
#ifdef INCLUDE_L3  

#include <soc/drv.h>
#include <bcm/vlan.h>
#include <bcm/error.h>

#include <bcm/l3.h>
#include <soc/l3x.h>
#include <soc/lpm.h>
#ifdef ALPM_ENABLE
#include <soc/alpm.h>
#endif
#include <bcm/tunnel.h>
#include <bcm/debug.h>
#include <bcm/error.h>
#include <bcm/stack.h>
#include <soc/trident2.h>

#include <bcm_int/esw/trident2.h>
#include <bcm_int/esw/lpmv6.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/triumph.h>
#include <bcm_int/esw/triumph2.h>
#include <bcm_int/esw/stack.h>
#include <bcm_int/esw/flex_ctr.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/flex_ctr.h>
#include <bcm_int/esw/virtual.h>

#ifdef BCM_KATANA2_SUPPORT
#include <bcm_int/esw/katana2.h>
#endif /* BCM_KATANA2_SUPPORT */

#if defined(BCM_TRX_SUPPORT) 
#include <bcm_int/esw/trx.h>
#endif /* BCM_TRX_SUPPORT */

#define _BCM_L3_MEM_BANKS_ALL     (-1)
#define _BCM_HOST_ENTRY_NOT_FOUND (-1)

/* L3 LPM state */
_bcm_defip_pair128_table_t *l3_defip_pair128[BCM_MAX_NUM_UNITS];

#define SOC_LPM_128_LPM_CACHE_FIELD_CREATE(m, f) soc_field_info_t * m##f
#define SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(m_u, s, m, f) \
                (s)->m##f = soc_mem_fieldinfo_get(m_u, m, f)
typedef struct soc_lpm_128_field_cache_s {
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, CLASS_IDf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, DST_DISCARDf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, ECMPf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, ECMP_COUNTf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, ECMP_PTRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, GLOBAL_ROUTEf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, HITf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, IP_ADDR0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, IP_ADDR1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, IP_ADDR0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, IP_ADDR1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, IP_ADDR_MASK0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, IP_ADDR_MASK1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, IP_ADDR_MASK0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, IP_ADDR_MASK1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, MODE0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, MODE1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, MODE0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, MODE1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, MODE_MASK0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, MODE_MASK1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, MODE_MASK0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, MODE_MASK1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, NEXT_HOP_INDEXf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, PRIf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, RPEf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VALID0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VALID1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VALID0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VALID1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VRF_ID_0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VRF_ID_1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VRF_ID_0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VRF_ID_1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VRF_ID_MASK0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VRF_ID_MASK1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VRF_ID_MASK0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, VRF_ID_MASK1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, GLOBAL_HIGHf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, ALG_HIT_IDXf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, ALG_BKT_PTRf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, DEFAULT_MISSf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, FLEX_CTR_BASE_COUNTER_IDXf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, FLEX_CTR_POOL_NUMBERf);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, SRC_DISCARDf);    
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, DEFAULTROUTEf);    
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, HIT0f);
    SOC_LPM_128_LPM_CACHE_FIELD_CREATE(L3_DEFIP_PAIR_128m, HIT1f);
} soc_lpm_128_field_cache_t, *soc_lpm_128_field_cache_p;

static soc_lpm_128_field_cache_p 
                        soc_lpm_128_field_cache_state[SOC_MAX_NUM_DEVICES];

#define SOC_MEM_OPT_FLD32_GET(m_unit, m_mem, m_entry_data, m_field) \
        soc_meminfo_fieldinfo_field32_get((&SOC_MEM_INFO(m_unit, m_mem)), \
        (m_entry_data), (soc_lpm_128_field_cache_state[(m_unit)]->m_mem##m_field))

#define SOC_MEM_OPT_FLD32_SET(m_unit, m_mem, m_entry_data, m_field, m_val) \
        soc_meminfo_fieldinfo_field32_set((&SOC_MEM_INFO(m_unit, m_mem)), \
        (m_entry_data), (soc_lpm_128_field_cache_state[(m_unit)]->m_mem##m_field), (m_val))

#define SOC_MEM_OPT_FLD_GET(m_unit, m_mem, m_entry_data, m_field, m_pval) \
        soc_meminfo_fieldinfo_field_get((m_entry_data), \
        (&SOC_MEM_INFO(m_unit, m_mem)), \
        (soc_lpm_128_field_cache_state[(m_unit)]->m_mem##m_field), (m_pval))

#define SOC_MEM_OPT_FLD_SET(m_unit, m_mem, m_entry_data, m_field, m_pval) \
        soc_meminfo_fieldinfo_field_set((m_entry_data), \
        (&SOC_MEM_INFO(m_unit, m_mem)), \
        (soc_lpm_128_field_cache_state[(m_unit)]->m_mem##m_field), (m_pval))

#define SOC_MEM_OPT_FLD_VALID(m_unit, m_mem, m_field) \
        ((soc_lpm_128_field_cache_state[(m_unit)]->m_mem##m_field) != NULL)

#define SOC_MEM_OPT_FLD_LENGTH(m_unit, m_mem, m_field) \
        ((soc_lpm_128_field_cache_state[(m_unit)]->m_mem##m_field)->len)


/*
 * Function:
 *      _bcm_l3_defip_mem_get
 * Purpose:
 *      Resolve route table memory for a given route entry.
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      flags      - (IN)IPv6/IPv4 route.
 *      plen       - (IN)Prefix length.
 *      mem        - (OUT)Route table memory.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_l3_defip_mem_get(int unit, uint32 flags, int plen, soc_mem_t *mem)
{
    /* Default value */
    *mem = L3_DEFIPm;
    if ((flags & BCM_L3_IP6) && (plen > 64)) {
        *mem = L3_DEFIP_PAIR_128m;
    }
    
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_defip_pair128_hash
 * Purpose:
 *      Calculate an entry 16 bit hash.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 *      entry_hash  - (OUT)Entry hash
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_defip_pair128_hash(int unit, _bcm_defip_cfg_t *lpm_cfg, uint16 *hash) 
{
    uint32 key[BCM_DEFIP_PAIR128_HASH_SZ];

    if (hash == NULL) {
        return (BCM_E_PARAM);
    }

    if (lpm_cfg->defip_flags & BCM_L3_IP6) {
        SAL_IP6_ADDR_TO_UINT32(lpm_cfg->defip_ip6_addr, key);
        key[4] = lpm_cfg->defip_sub_len;
        key[5] = lpm_cfg->defip_vrf;

        *hash = _shr_crc16b(0, (void *)key,
                         WORDS2BITS((BCM_DEFIP_PAIR128_HASH_SZ)));
    } else {
        /* Call V4 hash function */
    }   
    return (BCM_E_NONE);
}

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *      _bcm_defip_pair128_reinit
 * Purpose:
 *      Re-Initialize sw image for L3_DEFIP_PAIR_128 internal tcam.
 *      during warm boot. 
 * Parameters:
 *      unit    - (IN)SOC unit number.
 *      idx     - (IN)Entry index   
 *      lpm_cfg - (IN)Inserted entry. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_defip_pair128_reinit(int unit, int idx, _bcm_defip_cfg_t *lpm_cfg)
{
    int   lkup_plen;         /* Vrf weighted lookup prefix. */
    uint16 hash = 0;         /* Entry hash value.           */

    /* Calculate vrf weighted prefix length. */
    lkup_plen = lpm_cfg->defip_sub_len * 
        ((BCM_L3_VRF_OVERRIDE == lpm_cfg->defip_vrf) ? 2 : 1);

    /* Calculate entry lookup hash. */
    BCM_IF_ERROR_RETURN(_bcm_defip_pair128_hash(unit, lpm_cfg, &hash));

    /* Set software structure info. */
    BCM_DEFIP_PAIR128_ENTRY_SET(unit, idx, lkup_plen, hash);

    /* Update software usage counter. */
    BCM_DEFIP_PAIR128_USED_COUNT(unit)++;
    soc_lpm_stat_128b_count_update(unit, 1);

    return (BCM_E_NONE);
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/*
 * Function:
 *      _bcm_defip_pair128_deinit
 * Purpose:
 *      De-initialize sw image for L3_DEFIP_PAIR_128 internal tcam.
 * Parameters:
 *      unit - (IN)SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_defip_pair128_deinit(int unit)
{

    if (BCM_DEFIP_PAIR128(unit) == NULL) {
        return (BCM_E_NONE);
    }

    /* De-allocate any existing structures. */
    if (soc_lpm_128_field_cache_state[unit] != NULL) {
        sal_free(soc_lpm_128_field_cache_state[unit]);
        soc_lpm_128_field_cache_state[unit] = NULL;
    }
    
    if (BCM_DEFIP_PAIR128_ARR(unit) != NULL) {
        sal_free(BCM_DEFIP_PAIR128_ARR(unit));
        BCM_DEFIP_PAIR128_ARR(unit) = NULL;
    }
    sal_free(BCM_DEFIP_PAIR128(unit));
    BCM_DEFIP_PAIR128(unit) = NULL;

    return (BCM_E_NONE);
}

int
_bcm_defip_pair128_field_cache_init(int unit)
{
    /* De-allocate existing structures. */
    if (soc_lpm_128_field_cache_state[unit] != NULL) {
        sal_free(soc_lpm_128_field_cache_state[unit]);
    }

    soc_lpm_128_field_cache_state[unit] = 
           sal_alloc(sizeof(soc_lpm_128_field_cache_t), "lpm_128_field_state");
    if (NULL == soc_lpm_128_field_cache_state[unit]) {
        return (BCM_E_MEMORY);
    }
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, CLASS_IDf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, DST_DISCARDf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, ECMPf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, ECMP_COUNTf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, ECMP_PTRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, GLOBAL_ROUTEf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, HITf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, IP_ADDR0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, IP_ADDR0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, IP_ADDR1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, IP_ADDR1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, IP_ADDR_MASK0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, IP_ADDR_MASK0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, IP_ADDR_MASK1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, IP_ADDR_MASK1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, MODE0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, MODE0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, MODE1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, MODE1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, MODE_MASK0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, MODE_MASK0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, MODE_MASK1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, MODE_MASK1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, NEXT_HOP_INDEXf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, PRIf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, RPEf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VALID0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VALID0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VALID1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VALID1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VRF_ID_0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VRF_ID_0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VRF_ID_1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VRF_ID_1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VRF_ID_MASK0_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VRF_ID_MASK0_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VRF_ID_MASK1_LWRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, VRF_ID_MASK1_UPRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, GLOBAL_HIGHf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, ALG_HIT_IDXf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, ALG_BKT_PTRf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, DEFAULT_MISSf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, 
                                    FLEX_CTR_BASE_COUNTER_IDXf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, 
                                    FLEX_CTR_POOL_NUMBERf);    
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, 
                                    SRC_DISCARDf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, DEFAULTROUTEf);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, HIT0f);
    SOC_LPM_128_LPM_CACHE_FIELD_ASSIGN(unit, soc_lpm_128_field_cache_state[unit],
                                    L3_DEFIP_PAIR_128m, HIT1f);
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_defip_pair128_init
 * Purpose:
 *      Initialize sw image for L3_DEFIP_PAIR_128 internal tcam.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_defip_pair128_init(int unit)
{
    int rv;
    int mem_sz;   
    int ipv6_128_depth;
 
    /* De-allocate any existing structures. */
    if (BCM_DEFIP_PAIR128(unit) != NULL) {
        rv = _bcm_defip_pair128_deinit(unit);
        BCM_IF_ERROR_RETURN(rv);
    }

    /* Allocate cam control structure. */
    mem_sz = sizeof(_bcm_defip_pair128_table_t);
    BCM_DEFIP_PAIR128(unit) = sal_alloc(mem_sz, "td2_l3_defip_pair128");
    if (BCM_DEFIP_PAIR128(unit) == NULL) {
        return (BCM_E_MEMORY);
    }

    /* Reset cam control structure. */
    sal_memset(BCM_DEFIP_PAIR128(unit), 0, mem_sz);

    ipv6_128_depth = SOC_L3_DEFIP_MAX_128B_ENTRIES(unit);
    BCM_DEFIP_PAIR128_TOTAL(unit) = ipv6_128_depth;
    BCM_DEFIP_PAIR128_URPF_OFFSET(unit) = 0;
    BCM_DEFIP_PAIR128_IDX_MAX(unit) = ipv6_128_depth - 1;
    if (ipv6_128_depth) {
        mem_sz = ipv6_128_depth * sizeof(_bcm_defip_pair128_entry_t);
        BCM_DEFIP_PAIR128_ARR(unit) = 
                              sal_alloc(mem_sz, 
                                        "td2_l3_defip_pair128_entry_array");
        if (BCM_DEFIP_PAIR128_ARR(unit) == NULL) {
            BCM_IF_ERROR_RETURN(_bcm_defip_pair128_deinit(unit));
            return (BCM_E_MEMORY);
        }
        sal_memset(BCM_DEFIP_PAIR128_ARR(unit), 0, mem_sz);
    }

    rv = _bcm_defip_pair128_field_cache_init(unit);
    if (BCM_FAILURE(rv)) {
        BCM_IF_ERROR_RETURN(_bcm_defip_pair128_deinit(unit));
        return rv;
    }
    
    return (BCM_E_NONE);
}

/*
 * Function:    
 *     _bcm_defip_pair128_ip6_mask_set
 * Purpose:  
 *     Set IP6 mask field in memory from ip6 addr type. 
 * Parameters: 
 *     unit  - (IN) BCM device number. 
 *     mem   - (IN) Memory id.
 *     entry - (IN) HW entry buffer.
 *     ip6   - (IN) SW ip6 address buffer.
 * Returns:      void
 */
STATIC void
_bcm_defip_pair128_ip6_mask_set(int unit, soc_mem_t mem, uint32 *entry, 
                                const ip6_addr_t ip6)
{
    uint32              ip6_field[4];
    
    assert(mem == L3_DEFIP_PAIR_128m);

    ip6_field[3] = ((ip6[12] << 24)| (ip6[13] << 16) |
                    (ip6[14] << 8) | (ip6[15] << 0));
    SOC_MEM_OPT_FLD_SET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR_MASK0_LWRf, &ip6_field[3]);
    ip6_field[2] = ((ip6[8] << 24) | (ip6[9] << 16) |
                    (ip6[10] << 8) | (ip6[11] << 0));
    SOC_MEM_OPT_FLD_SET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR_MASK1_LWRf, &ip6_field[2]);
    ip6_field[1] = ((ip6[4] << 24) | (ip6[5] << 16) |
                    (ip6[6] << 8)  | (ip6[7] << 0));
    SOC_MEM_OPT_FLD_SET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR_MASK0_UPRf, &ip6_field[1]);
    ip6_field[0] = ((ip6[0] << 24) | (ip6[1] << 16) |
                    (ip6[2] << 8)  | (ip6[3] << 0));
    SOC_MEM_OPT_FLD_SET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR_MASK1_UPRf, ip6_field);
}

/*
 * Function:    
 *     _bcm_defip_pair128_ip6_mask_get
 * Purpose:  
 *     Read IP6 mask field from memory field to ip6_addr_t buffer. 
 * Parameters: 
 *     unit  - (IN) BCM device number. 
 *     mem   - (IN) Memory id.
 *     entry - (IN) HW entry buffer.
 *     ip6   - (OUT) SW ip6 address buffer.
 * Returns:      void
 */
STATIC void
_bcm_defip_pair128_ip6_mask_get(int unit, soc_mem_t mem, const void *entry, 
                                ip6_addr_t ip6)
{
    uint32              ip6_field[4];

    assert(mem == L3_DEFIP_PAIR_128m);

    SOC_MEM_OPT_FLD_GET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR_MASK0_LWRf, (uint32 *)&ip6_field[3]);
    ip6[12] = (uint8) (ip6_field[3] >> 24);
    ip6[13] = (uint8) (ip6_field[3] >> 16 & 0xff);
    ip6[14] = (uint8) (ip6_field[3] >> 8 & 0xff);
    ip6[15] = (uint8) (ip6_field[3] & 0xff);
    SOC_MEM_OPT_FLD_GET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR_MASK1_LWRf, (uint32 *)&ip6_field[2]);
    ip6[8] = (uint8) (ip6_field[2] >> 24);
    ip6[9] = (uint8) (ip6_field[2] >> 16 & 0xff);
    ip6[10] = (uint8) (ip6_field[2] >> 8 & 0xff);
    ip6[11] = (uint8) (ip6_field[2] & 0xff);
    SOC_MEM_OPT_FLD_GET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR_MASK0_UPRf, (uint32 *)&ip6_field[1]);
    ip6[4] = (uint8) (ip6_field[1] >> 24);
    ip6[5] = (uint8) (ip6_field[1] >> 16 & 0xff);
    ip6[6] =(uint8) (ip6_field[1] >> 8 & 0xff);
    ip6[7] =(uint8) (ip6_field[1] & 0xff);
    SOC_MEM_OPT_FLD_GET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR_MASK1_UPRf, ip6_field);
    ip6[0] =(uint8) (ip6_field[0] >> 24);
    ip6[1] =(uint8) (ip6_field[0] >> 16 & 0xff);
    ip6[2] =(uint8) (ip6_field[0] >> 8 & 0xff);
    ip6[3] =(uint8) (ip6_field[0] & 0xff);
}

/*
 * Function:    
 *     _bcm_defip_pair128_ip6_addr_set
 * Purpose:  
 *     Set IP6 address field in memory from ip6 addr type. 
 * Parameters: 
 *     unit  - (IN) BCM device number. 
 *     mem   - (IN) Memory id.
 *     entry - (IN) HW entry buffer.
 *     ip6   - (IN) SW ip6 address buffer.
 * Returns:      void
 */
STATIC void
_bcm_defip_pair128_ip6_addr_set(int unit, soc_mem_t mem, uint32 *entry, 
                                const ip6_addr_t ip6)
{
    uint32              ip6_field[4];

    assert(mem == L3_DEFIP_PAIR_128m);

    ip6_field[3] = ((ip6[12] << 24)| (ip6[13] << 16) |
                    (ip6[14] << 8) | (ip6[15] << 0));
    SOC_MEM_OPT_FLD_SET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR0_LWRf, &ip6_field[3]);
    ip6_field[2] = ((ip6[8] << 24) | (ip6[9] << 16) |
                    (ip6[10] << 8) | (ip6[11] << 0));
    SOC_MEM_OPT_FLD_SET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR1_LWRf, &ip6_field[2]);
    ip6_field[1] = ((ip6[4] << 24) | (ip6[5] << 16) |
                    (ip6[6] << 8)  | (ip6[7] << 0));
    SOC_MEM_OPT_FLD_SET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR0_UPRf, &ip6_field[1]);
    ip6_field[0] = ((ip6[0] << 24) | (ip6[1] << 16) |
                    (ip6[2] << 8)  | (ip6[3] << 0));
    SOC_MEM_OPT_FLD_SET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR1_UPRf, ip6_field);

}

/*
 * Function:    
 *     _bcm_defip_pair128_ip6_addr_get
 * Purpose:  
 *     Read IP6 address field from memory field to ip6_addr_t buffer. 
 * Parameters: 
 *     unit  - (IN) BCM device number. 
 *     mem   - (IN) Memory id.
 *     entry - (IN) HW entry buffer.
 *     ip6   - (OUT) SW ip6 address buffer.
 * Returns:      void
 */
STATIC void
_bcm_defip_pair128_ip6_addr_get(int unit, soc_mem_t mem, const void *entry, 
                                ip6_addr_t ip6)
{
    uint32              ip6_field[4];

    assert(mem == L3_DEFIP_PAIR_128m);

    SOC_MEM_OPT_FLD_GET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR0_LWRf, (uint32 *)&ip6_field[3]);
    ip6[12] = (uint8) (ip6_field[3] >> 24);
    ip6[13] = (uint8) (ip6_field[3] >> 16 & 0xff);
    ip6[14] = (uint8) (ip6_field[3] >> 8 & 0xff);
    ip6[15] = (uint8) (ip6_field[3] & 0xff);
    SOC_MEM_OPT_FLD_GET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR1_LWRf, (uint32 *)&ip6_field[2]);
    ip6[8] = (uint8) (ip6_field[2] >> 24);
    ip6[9] = (uint8) (ip6_field[2] >> 16 & 0xff);
    ip6[10] = (uint8) (ip6_field[2] >> 8 & 0xff);
    ip6[11] = (uint8) (ip6_field[2] & 0xff);
    SOC_MEM_OPT_FLD_GET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR0_UPRf, (uint32 *)&ip6_field[1]);
    ip6[4] = (uint8) (ip6_field[1] >> 24);
    ip6[5] = (uint8) (ip6_field[1] >> 16 & 0xff);
    ip6[6] =(uint8) (ip6_field[1] >> 8 & 0xff);
    ip6[7] =(uint8) (ip6_field[1] & 0xff);
    SOC_MEM_OPT_FLD_GET(unit, L3_DEFIP_PAIR_128m, entry, IP_ADDR1_UPRf, ip6_field);
    ip6[0] =(uint8) (ip6_field[0] >> 24);
    ip6[1] =(uint8) (ip6_field[0] >> 16 & 0xff);
    ip6[2] =(uint8) (ip6_field[0] >> 8 & 0xff);
    ip6[3] =(uint8) (ip6_field[0] & 0xff);
    
}

/*
 * Function:
 *      _bcm_defip_pair128_get_key
 * Purpose:
 *      Parse route entry key from L3_DEFIP_PAIR_128 table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      entry       - (IN)Hw entry buffer. 
 *      lpm_cfg     - (IN/OUT)Buffer to fill defip information. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_defip_pair128_get_key(int unit, uint32 *hw_entry, _bcm_defip_cfg_t *lpm_cfg)
{
    bcm_ip6_t mask;                      /* Subnet mask.        */

    /* Input parameters check. */
    if ((lpm_cfg == NULL) || (hw_entry == NULL)) {
        return (BCM_E_PARAM);
    }

    /* Extract ip6 address. */
    _bcm_defip_pair128_ip6_addr_get(unit, L3_DEFIP_PAIR_128m, hw_entry, lpm_cfg->defip_ip6_addr);

    /* Extract subnet mask. */
    _bcm_defip_pair128_ip6_mask_get(unit, L3_DEFIP_PAIR_128m, hw_entry, mask);
    lpm_cfg->defip_sub_len = bcm_ip6_mask_length(mask);

    /* Extract vrf id & vrf mask. */
    if (!SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_MASK0_LWRf)) {
        lpm_cfg->defip_vrf = BCM_L3_VRF_OVERRIDE;
    } else {
        lpm_cfg->defip_vrf = SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_0_LWRf);
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_defip_pair128_match
 * Purpose:
 *      Lookup route entry from L3_DEFIP_PAIR_128 table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 *      hw_entry    - (OUT)Matching hw entry. 
 *      hw_index    - (OUT)HW table index. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_defip_pair128_match(int unit, _bcm_defip_cfg_t *lpm_cfg, 
                         uint32 *hw_entry, int *hw_index)
{
    _bcm_defip_cfg_t candidate;        /* Match condidate.            */
    int   lkup_plen;                   /* Vrf weighted lookup prefix. */
    int   index;                       /* Table iteration index.      */
    uint16 hash = 0;                   /* Entry hash value.           */
    int   rv = BCM_E_NONE;             /* Internal operation status.  */


    /* Initialization */
    sal_memset(&candidate, 0, sizeof(_bcm_defip_cfg_t));
    
    /* Calculate vrf weighted prefix length. */
    lkup_plen = lpm_cfg->defip_sub_len * 
        ((BCM_L3_VRF_OVERRIDE == lpm_cfg->defip_vrf) ? 2 : 1);

    /* Calculate entry lookup hash. */
    BCM_IF_ERROR_RETURN(_bcm_defip_pair128_hash(unit, lpm_cfg, &hash));

    /*    coverity[assignment : FALSE]    */
    for (index = 0; index <= BCM_DEFIP_PAIR128_IDX_MAX(unit); index++) {

        /* Route prefix length comparison. */
        if (lkup_plen != BCM_DEFIP_PAIR128_ARR(unit)[index].prefix_len) {
            continue;
        }

        /* Route lookup key hash comparison. */
        if (hash != BCM_DEFIP_PAIR128_ARR(unit)[index].entry_hash) {
            continue;
        }

        /* Hash & prefix length match -> read and compare hw entry itself. */
        rv = BCM_XGS3_MEM_READ(unit, L3_DEFIP_PAIR_128m, index, hw_entry);
        if (BCM_FAILURE(rv)) {
            break;
        }

        /* Make sure entry is valid.  */
        if ((!SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, VALID0_LWRf)) ||
            (!SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, VALID1_LWRf)) ||
            (!SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, VALID0_UPRf)) ||
            (!SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, VALID1_UPRf))
            ) {
            /* Too bad sw image doesn't match hardware. */
            continue;
        }

        /* Extract entry address vrf */
        rv =  _bcm_defip_pair128_get_key(unit, hw_entry, &candidate);
        if (BCM_FAILURE(rv)) {
            break;
        }


        /* Address & Vrf comparison. */
        if (lpm_cfg->defip_vrf != candidate.defip_vrf) {
            continue;
        }

        /* Subnet mask comparison. */
        if (lpm_cfg->defip_sub_len != candidate.defip_sub_len) {
            continue;
        }


        if (sal_memcmp(lpm_cfg->defip_ip6_addr, candidate.defip_ip6_addr,
                       sizeof(bcm_ip6_t))) {
            continue;
        }

        /* Matching entry found. */
        break;

    }

    BCM_IF_ERROR_RETURN(rv);

    if (index > BCM_DEFIP_PAIR128_IDX_MAX(unit))  {
        return (BCM_E_NOT_FOUND);
    }

    *hw_index = index;
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_defip_pair128_entry_clear
 * Purpose:
 *      Clear/Reset a single route entry. 
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      hw_index  - (IN)Entry index in the tcam. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_defip_pair128_entry_clear(int unit, int hw_index)  
{
    soc_mem_t mem = L3_DEFIP_PAIR_128m;
    uint32 hw_entry[SOC_MAX_MEM_FIELD_WORDS];
    soc_field_t key_field[4], mask_field[4];
    int field_count, i;
    uint64 val64;
    key_field[0] = KEY0_UPRf;
    key_field[1] = KEY1_UPRf;
    key_field[2] = KEY0_LWRf;
    key_field[3] = KEY1_LWRf;
    mask_field[0] = MASK0_UPRf;
    mask_field[1] = MASK1_UPRf;
    mask_field[2] = MASK0_LWRf;
    mask_field[3] = MASK1_LWRf;
    field_count = 4;
    sal_memset(hw_entry, 0, WORDS2BYTES(SOC_MAX_MEM_FIELD_WORDS));
    COMPILER_64_ZERO(val64);
    for (i = 0; i < field_count; i++) {
        soc_mem_field64_set(unit, mem, hw_entry, key_field[i], val64);
        soc_mem_field64_set(unit, mem, hw_entry, mask_field[i], val64);
    }

    /* Reset hw entry. */
    BCM_IF_ERROR_RETURN
        (BCM_XGS3_MEM_WRITE(unit, L3_DEFIP_PAIR_128m, hw_index,
                            &soc_mem_entry_null(unit, L3_DEFIP_PAIR_128m)));
    BCM_IF_ERROR_RETURN
        (BCM_XGS3_MEM_WRITE(unit, L3_DEFIP_PAIR_128m, hw_index, hw_entry));

    /* Reset sw entry. */
    BCM_DEFIP_PAIR128_ENTRY_SET(unit, hw_index, 0, 0);

    /* Reset urpf source lookup entry. */
    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF (Ex: KT2).
     */
    if (SOC_URPF_STATUS_GET(unit) &&
        !soc_feature(unit, soc_feature_l3_defip_advanced_lookup)) {
        BCM_IF_ERROR_RETURN
            (BCM_XGS3_MEM_WRITE(unit, L3_DEFIP_PAIR_128m, 
                            (hw_index + BCM_DEFIP_PAIR128_URPF_OFFSET(unit)),
                            &soc_mem_entry_null(unit, L3_DEFIP_PAIR_128m)));
        BCM_IF_ERROR_RETURN
            (BCM_XGS3_MEM_WRITE(unit, L3_DEFIP_PAIR_128m, 
                 (hw_index+ BCM_DEFIP_PAIR128_URPF_OFFSET(unit)), hw_entry));
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_defip_pair128_parse
 * Purpose:
 *      Parse route entry from L3_DEFIP_PAIR_128 table to sw structure.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      hw_entry    - (IN)Hw entry buffer. 
 *      lpm_cfg     - (IN/OUT)Buffer to fill defip information. 
 *      nh_ecmp_idx - (OUT)Next hop ecmp table index. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC INLINE int 
_bcm_defip_pair128_parse(int unit, uint32 *hw_entry,
                         _bcm_defip_cfg_t *lpm_cfg, int *nh_ecmp_idx)
{
    /* Input parameters check. */
    if ((lpm_cfg == NULL) || (hw_entry == NULL)) {
        return (BCM_E_PARAM);
    }

    /* Reset entry flags first. */
    lpm_cfg->defip_flags = 0;

    /* Check if entry points to ecmp group. */
    if (SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, ECMPf)) {
        /* Mark entry as ecmp */
        lpm_cfg->defip_ecmp = TRUE;
        lpm_cfg->defip_flags |= BCM_L3_MULTIPATH;

        /* Get ecmp group id. */
        if (nh_ecmp_idx != NULL) {
            *nh_ecmp_idx = SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, ECMP_PTRf);
        }
    } else {
        /* Mark entry as non-ecmp. */
        lpm_cfg->defip_ecmp = 0;

        /* Reset ecmp group next hop count. */
        lpm_cfg->defip_ecmp_count = 0;

        /* Get next hop index. */
        if (nh_ecmp_idx != NULL) {
            *nh_ecmp_idx =
                SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, NEXT_HOP_INDEXf);
        }
    }

    /* Mark entry as IPv6 */
    lpm_cfg->defip_flags |= BCM_L3_IP6;

    /* Get entry priority. */
    lpm_cfg->defip_prio = SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, PRIf);

    /* Get classification group id. */
    lpm_cfg->defip_lookup_class = 
        SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, CLASS_IDf);

    /* Get hit bit. */
    if (SOC_MEM_OPT_FLD_VALID(unit, L3_DEFIP_PAIR_128m, HITf)) {
        if (SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, HITf)) {
            lpm_cfg->defip_flags |= BCM_L3_HIT;
        }
    }

    if (SOC_MEM_OPT_FLD_VALID(unit, L3_DEFIP_PAIR_128m, HIT0f)) {
        if (SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, HIT0f)) {
            lpm_cfg->defip_flags |= BCM_L3_HIT;
        }
    }

    /* Get priority override bit. */
    if (SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, RPEf))  {
        lpm_cfg->defip_flags |= BCM_L3_RPE;
    }

    /* Get destination discard field. */
    if (SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, DST_DISCARDf)) {
        lpm_cfg->defip_flags |= BCM_L3_DST_DISCARD;
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_defip_pair128_shift
 * Purpose:
 *      Shift entries in the tcam. 
 * Parameters:
 *      unit    - (IN)SOC unit number.
 *      idx_src     - (IN)Entry source. 
 *      idx_dest    - (IN)Entry destination.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_defip_pair128_shift(int unit, int idx_src, int idx_dest)  
{
    uint32 hw_entry[SOC_MAX_MEM_FIELD_WORDS]; /* HW entry buffer. */

    /* Don't copy uninstalled entries. */
    if (0 != BCM_DEFIP_PAIR128_ARR(unit)[idx_src].prefix_len) {
        /* Read entry from source index. */
        BCM_IF_ERROR_RETURN
            (BCM_XGS3_MEM_READ(unit, L3_DEFIP_PAIR_128m, idx_src, hw_entry));

        /* Write entry to the destination index. */
        BCM_IF_ERROR_RETURN
            (BCM_XGS3_MEM_WRITE(unit, L3_DEFIP_PAIR_128m, idx_dest, hw_entry));

        /* Shift corresponding  URPF/source lookup entries. */
        /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
         * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
         * L3_DEFIP table is not divided into 2 to support URPF (Ex: KT2).
         */
        if (SOC_URPF_STATUS_GET(unit) &&
            !soc_feature(unit, soc_feature_l3_defip_advanced_lookup)) {
            /* Read entry from source index + half tcam. */
            BCM_IF_ERROR_RETURN
                (BCM_XGS3_MEM_READ(unit, L3_DEFIP_PAIR_128m, 
                              idx_src + BCM_DEFIP_PAIR128_URPF_OFFSET(unit),
                              hw_entry));

            /* Write entry to the destination index + half tcam. */
            BCM_IF_ERROR_RETURN
                (BCM_XGS3_MEM_WRITE(unit, L3_DEFIP_PAIR_128m, 
                            idx_dest +  BCM_DEFIP_PAIR128_URPF_OFFSET(unit),
                            hw_entry));
        }
    }

    /* Copy software portion of the entry. */ 
    sal_memcpy (&BCM_DEFIP_PAIR128_ARR(unit)[idx_dest],
                &BCM_DEFIP_PAIR128_ARR(unit)[idx_src], 
                sizeof(_bcm_defip_pair128_entry_t));

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_defip_pair128_shift_range
 * Purpose:
 *      Shift entries in the tcam. 
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      idx_start   - (IN)Start index
 *      idx_end     - (IN)End index
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_defip_pair128_shift_range(int unit, int idx_start, int idx_end)  
{
    int idx;     /* Iteration index. */
    int rv = BCM_E_NONE;   /* Operaton status. */ 

    /* Shift entries down (1 index up) */
    if (idx_end > idx_start) {
        for (idx = idx_end - 1; idx >= idx_start; idx--) {
            rv = _bcm_defip_pair128_shift(unit, idx, idx + 1);  
            if (BCM_FAILURE(rv)) { 
                break;
            }
        }
        /* Delete last entry from hardware & software. */
        rv = _bcm_defip_pair128_entry_clear(unit, idx_start);
    }

    /* Shift entries up (1 index down) */
    if (idx_end < idx_start) {
        for (idx = idx_end + 1; idx <= idx_start; idx++) {
            rv = _bcm_defip_pair128_shift(unit, idx, idx - 1);  
            if (BCM_FAILURE(rv)) { 
                break;
            }
        }
        /* Delete last entry from hardware & software. */
        rv = _bcm_defip_pair128_entry_clear(unit, idx_start);
    }

    return (rv);
}

/*
 * Function:
 *      _bcm_defip_pair128_idx_alloc
 * Purpose:
 *      Shuffle the tcam in order to insert a new entry 
 *      based on vrf weighted prefix length.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Route info 
 *      hw_index    - (OUT)Allocated hw index. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_defip_pair128_idx_alloc(int unit, _bcm_defip_cfg_t *lpm_cfg, int *hw_index,
                             int nh_ecmp_idx)
{
    int  lkup_plen;                   /* Vrf weighted lookup prefix.    */
    uint16 hash = 0;                  /* Entry hash value.              */
    int  idx;                         /* Table iteration index.         */
    int  rv;                          /* Operation return status.       */
    int  shift_min = 0;               /* Minimum entry shift required.  */
    int  range_start = 0;             /* First valid location for pfx.  */
    int  range_end = BCM_DEFIP_PAIR128_IDX_MAX(unit);
                                      /* Last valid location for pfx.   */
    int  free_idx_before = -1;        /* Free index before range_start. */
    int  free_idx_after  = -1;        /* Free index after range_end.    */

    /* Input parameters check. */
    if ((lpm_cfg == NULL) || (hw_index == NULL)) {
        return (BCM_E_PARAM);
    }

    /* Calculate vrf weighted prefix length. */
    lkup_plen = lpm_cfg->defip_sub_len * 
        ((BCM_L3_VRF_OVERRIDE == lpm_cfg->defip_vrf) ? 2 : 1);

    /* Calculate entry hash. */
    BCM_IF_ERROR_RETURN(_bcm_defip_pair128_hash(unit, lpm_cfg, &hash));

    /* Make sure at least one slot is available. */
    if (BCM_DEFIP_PAIR128_USED_COUNT(unit) ==
                      BCM_DEFIP_PAIR128_TOTAL(unit)) {
        return (BCM_E_FULL);
    }

    /* 
     *  Find first valid index for the prefix;
     */ 
    for (idx = 0; idx <= BCM_DEFIP_PAIR128_IDX_MAX(unit); idx++) {
        /* Remember free entry before installation location. */
        if (0 == BCM_DEFIP_PAIR128_ARR(unit)[idx].prefix_len) {
            free_idx_before = idx;
            continue;
        }
        /* Route prefix length comparison. */
        if (lkup_plen < BCM_DEFIP_PAIR128_ARR(unit)[idx].prefix_len) {
            range_start = idx;
            continue;
        }
        break;
    }

    /* 
     *  Find last valid index for the prefix;
     */ 
    for (idx = BCM_DEFIP_PAIR128_IDX_MAX(unit); idx >= 0; idx--) {

        /* Remeber free entry after installation location. */
        if (0 == BCM_DEFIP_PAIR128_ARR(unit)[idx].prefix_len) {
            free_idx_after = idx;
            continue;
        }
        /* Route prefix length comparison. */
        if (lkup_plen >  BCM_DEFIP_PAIR128_ARR(unit)[idx].prefix_len) {
            range_end = idx;
            continue;
        }
        break;
    }

    /* Check if entry should be inserted at the end. */
    if (range_start == BCM_DEFIP_PAIR128_IDX_MAX(unit)) {
        /* Shift all entries from free_idx_before till the end 1 up. */   
        rv = _bcm_defip_pair128_shift_range(unit, BCM_DEFIP_PAIR128_IDX_MAX(unit), free_idx_before);

        if (BCM_SUCCESS(rv)) { 
            /* Allocated index is last TCAM entry. */
            *hw_index = BCM_DEFIP_PAIR128_IDX_MAX(unit);
            BCM_DEFIP_PAIR128_ENTRY_SET(unit, *hw_index, lkup_plen, hash);
        }
        return (rv);
    }

    /* Check if entry should be inserted at the beginning. */
    if (range_end < 0) {
        /* Shift all entries from 0 till free_idx_after 1 down. */
        rv = _bcm_defip_pair128_shift_range(unit, 0, free_idx_after);

        if (BCM_SUCCESS(rv)) {
            /* Allocated index is first TCAM entry. */
            *hw_index = 0;
            BCM_DEFIP_PAIR128_ENTRY_SET(unit, 0, lkup_plen, hash);
        }
        return (rv);
    }

    /* Find if there is any free index in range. */
    for (idx = range_start; idx <= range_end; idx++) {
        if (0 == BCM_DEFIP_PAIR128_ARR(unit)[idx].prefix_len) {
            *hw_index = idx;
            BCM_DEFIP_PAIR128_ENTRY_SET(unit, idx, lkup_plen, hash);
            return (BCM_E_NONE);
        }
    }

    /* Check if entry should be inserted at the end of range. */
    if ((free_idx_after > range_end) && (free_idx_after != -1))  {
        shift_min = free_idx_after - range_end;
    } 

    /* Check if entry should be inserted in the beginning. */
    if ((free_idx_before < range_start) && (free_idx_before != -1))  {
        if((shift_min > (range_start - free_idx_before)) || (free_idx_after == -1)) {
            shift_min = -1;
        }
    }

    if (shift_min <= 0) {
        rv = _bcm_defip_pair128_shift_range(unit, range_start, free_idx_before);
        if (BCM_SUCCESS(rv)) {
            *hw_index= range_start;
            BCM_DEFIP_PAIR128_ENTRY_SET(unit, range_start, lkup_plen, hash);
        }
    } else {
        rv = _bcm_defip_pair128_shift_range(unit, range_end, free_idx_after);
        if (BCM_SUCCESS(rv)) {
            *hw_index = range_end;
            BCM_DEFIP_PAIR128_ENTRY_SET(unit, range_end, lkup_plen, hash);
        }
    }
    return (rv);
}

/*
 * Function:
 *      _bcm_l3_defip_pair128_get
 * Purpose:
 *      Get an entry from L3_DEFIP_PAIR_128 table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 *      nh_ecmp_idx - (IN)Next hop or ecmp group index
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_l3_defip_pair128_get(int unit, _bcm_defip_cfg_t *lpm_cfg, 
                              int *nh_ecmp_idx) 
{
    uint32 hw_entry[SOC_MAX_MEM_FIELD_WORDS];/* Hw entry buffer.          */
    bcm_ip6_t mask;                          /* Subnet mask.              */
    int clear_hit;                           /* Clear entry hit bit flag. */
    int hw_index;                            /* Entry offset in the TCAM. */ 
    int rv;                                  /* Internal operation status.*/

    /* Input parameters check. */
    if (lpm_cfg == NULL) {
        return (BCM_E_PARAM);
    }

    /* Check if clear hit bit required. */
    clear_hit = lpm_cfg->defip_flags & BCM_L3_HIT_CLEAR;

    /* Create mask from prefix length. */
    bcm_ip6_mask_create(mask, lpm_cfg->defip_sub_len);

    /* Apply mask on address. */
    bcm_xgs3_l3_mask6_apply(mask, lpm_cfg->defip_ip6_addr);

    /* Search for matching entry in the cam. */
    rv =  _bcm_defip_pair128_match(unit, lpm_cfg, hw_entry, &hw_index);
    BCM_IF_ERROR_RETURN(rv);

    lpm_cfg->defip_index = hw_index;

    /* Parse matched entry. */
    rv = _bcm_defip_pair128_parse(unit, hw_entry, lpm_cfg, nh_ecmp_idx);
    BCM_IF_ERROR_RETURN(rv);

    /* Clear the HIT bit */
    if (clear_hit) {
        if (SOC_MEM_OPT_FLD_VALID(unit, L3_DEFIP_PAIR_128m, HITf)) {
            SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, HITf, 0);
        }

        if (SOC_MEM_OPT_FLD_VALID(unit, L3_DEFIP_PAIR_128m, HIT0f)) {
            SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, HIT0f, 0);
        }

        if (SOC_MEM_OPT_FLD_VALID(unit, L3_DEFIP_PAIR_128m, HIT1f)) {
            SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, HIT1f, 0);
        }

        rv = BCM_XGS3_MEM_WRITE(unit, L3_DEFIP_PAIR_128m, hw_index, hw_entry);
    }

    return (rv);
}

/*
 * Function:
 *      _bcm_l3_defip_pair128_add
 * Purpose:
 *      Add an entry to internal L3_DEFIP_PAIR_128 table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 *      nh_ecmp_idx - (IN)Next hop or ecmp group index.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_l3_defip_pair128_add(int unit, _bcm_defip_cfg_t *lpm_cfg,
                              int nh_ecmp_idx) 
{
    uint32 hw_entry[SOC_MAX_MEM_FIELD_WORDS];   /* New entry hw buffer.      */
    bcm_ip6_t mask;                             /* Subnet mask.              */
    int hw_index = 0;                           /* Entry offset in the TCAM. */ 
    int rv = BCM_E_NONE;                        /* Operation return status.  */

    uint32 max_v6_entries = BCM_XGS3_L3_IP6_MAX_128B_ENTRIES(unit);
    uint32 used_cnt = BCM_DEFIP_PAIR128_USED_COUNT(unit);

    /* Input parameters check. */
    if (lpm_cfg == NULL) {
        return (BCM_E_PARAM);
    }

    if ((used_cnt >= max_v6_entries) && 
        !(lpm_cfg->defip_flags & BCM_L3_REPLACE)) {
        return BCM_E_FULL;     
    }

    /* No support for routes matchin AFTER vrf specific. */
    if (BCM_L3_VRF_GLOBAL == lpm_cfg->defip_vrf) {
        return (BCM_E_UNAVAIL);
    }

    /* Zero buffers. */
    sal_memset(hw_entry, 0, WORDS2BYTES(SOC_MAX_MEM_FIELD_WORDS));

    /* Create mask from prefix length. */
    bcm_ip6_mask_create(mask, lpm_cfg->defip_sub_len);

    /* Apply mask on address. */
    bcm_xgs3_l3_mask6_apply(mask, lpm_cfg->defip_ip6_addr);

    /* Allocate a new index for inserted entry. */
    if (BCM_XGS3_L3_INVALID_INDEX == lpm_cfg->defip_index) {
        rv = _bcm_defip_pair128_idx_alloc(unit, lpm_cfg, &hw_index, nh_ecmp_idx);
        BCM_IF_ERROR_RETURN(rv);
    } else {
        hw_index = lpm_cfg->defip_index;
    }

    /* Set hit bit. */
    if (lpm_cfg->defip_flags & BCM_L3_HIT) {
        if (SOC_MEM_OPT_FLD_VALID(unit, L3_DEFIP_PAIR_128m, HITf)) {
            SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, HITf, 1);
        }
        if (SOC_MEM_OPT_FLD_VALID(unit, L3_DEFIP_PAIR_128m, HIT0f)) {
            SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, HIT0f, 1);
        }
        if (SOC_MEM_OPT_FLD_VALID(unit, L3_DEFIP_PAIR_128m, HIT1f)) {
            SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, HIT1f, 1);
        }
    }

    /* Set priority override bit. */
    if (lpm_cfg->defip_flags & BCM_L3_RPE) {
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, RPEf, 1);
    }

    /* Set classification group id. */
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, CLASS_IDf,
            lpm_cfg->defip_lookup_class);

    /* Write priority field. */
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, PRIf, lpm_cfg->defip_prio);


    /* Fill next hop information. */
    if (lpm_cfg->defip_flags & BCM_L3_MULTIPATH) {
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, ECMP_PTRf, nh_ecmp_idx);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, ECMPf, 1);
    } else {
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, NEXT_HOP_INDEXf, nh_ecmp_idx);
    }

    /* Set destination discard flag. */
    if (lpm_cfg->defip_flags & BCM_L3_DST_DISCARD) {
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, DST_DISCARDf, 1);
    }

    /* Set mode bits to indicate IP v6 128b pair mode */
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, MODE1_UPRf, 0x3);
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, MODE0_UPRf, 0x3);
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, MODE1_LWRf, 0x3);
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, MODE0_LWRf, 0x3);

    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, MODE_MASK1_UPRf, 0x3);
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, MODE_MASK0_UPRf, 0x3);
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, MODE_MASK1_LWRf, 0x3);
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, MODE_MASK0_LWRf, 0x3);


    /* Set valid bits. */
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VALID1_UPRf, 1);
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VALID0_UPRf, 1);
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VALID1_LWRf, 1);
    SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VALID0_LWRf, 1);

    /* Set destination/source ip address. */
    _bcm_defip_pair128_ip6_addr_set(unit, L3_DEFIP_PAIR_128m, hw_entry, lpm_cfg->defip_ip6_addr);

    /* Set ip mask. */
    _bcm_defip_pair128_ip6_mask_set(unit, L3_DEFIP_PAIR_128m, hw_entry, mask);

    /* Set vrf id & vrf mask. */
    if (BCM_L3_VRF_OVERRIDE != lpm_cfg->defip_vrf) {
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_0_LWRf, lpm_cfg->defip_vrf);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_1_LWRf, lpm_cfg->defip_vrf);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_0_UPRf, lpm_cfg->defip_vrf);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_1_UPRf, lpm_cfg->defip_vrf);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_MASK0_LWRf,
                            (1 << SOC_MEM_OPT_FLD_LENGTH(unit, L3_DEFIP_PAIR_128m,
                                                       VRF_ID_MASK0_LWRf)) - 1);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_MASK1_LWRf,
                            (1 << SOC_MEM_OPT_FLD_LENGTH(unit, L3_DEFIP_PAIR_128m,
                                                       VRF_ID_MASK1_LWRf)) - 1);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_MASK0_UPRf,
                            (1 << SOC_MEM_OPT_FLD_LENGTH(unit, L3_DEFIP_PAIR_128m,
                                                       VRF_ID_MASK0_UPRf)) - 1);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_MASK1_LWRf,
                            (1 << SOC_MEM_OPT_FLD_LENGTH(unit, L3_DEFIP_PAIR_128m,
                                                       VRF_ID_MASK1_UPRf)) - 1);
        if (SOC_MEM_OPT_FLD_VALID(unit, L3_DEFIP_PAIR_128m, GLOBAL_ROUTEf)) {
            SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, GLOBAL_ROUTEf, 0);
        }
    } else {
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_0_LWRf, 0);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_1_LWRf, 0);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_0_UPRf, 0);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_1_UPRf, 0);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_MASK0_LWRf, 0);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_MASK1_LWRf, 0);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_MASK0_UPRf, 0);
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, VRF_ID_MASK1_UPRf, 0);
    }

    /* Configure entry for SIP lookup if URPF is enabled. This => SIP is default route.*/
    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF (Ex: KT2).
     */
    if (soc_feature(unit, soc_feature_l3_defip_advanced_lookup)) {
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, DEFAULTROUTEf,
            (SOC_URPF_STATUS_GET(unit) ? 1 : 0));
    }

    /* Write buffer to hw. */
    rv = BCM_XGS3_MEM_WRITE(unit, L3_DEFIP_PAIR_128m, hw_index, hw_entry);
    if (BCM_FAILURE(rv)) {
        BCM_DEFIP_PAIR128_ENTRY_SET(unit, hw_index, 0, 0);
        return (rv);
    }

    /* Write source lookup portion of the entry. */
    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF (Ex: KT2).
     */
    if (SOC_URPF_STATUS_GET(unit) &&
        !soc_feature(unit, soc_feature_l3_defip_advanced_lookup)) {
        SOC_MEM_OPT_FLD32_SET(unit, L3_DEFIP_PAIR_128m, hw_entry, SRC_DISCARDf, 0);
        rv = BCM_XGS3_MEM_WRITE(unit, L3_DEFIP_PAIR_128m, 
                                (hw_index + BCM_DEFIP_PAIR128_URPF_OFFSET(unit)),
                                hw_entry);
        if (BCM_FAILURE(rv)) {
            _bcm_defip_pair128_entry_clear(unit, hw_index);
            return (rv);
        }
    }

    /* If new route added increment total number of routes.  */
    /* Lack of index indicates a new route. */
    if (BCM_XGS3_L3_INVALID_INDEX == lpm_cfg->defip_index) {
        BCM_XGS3_L3_DEFIP_CNT_INC(unit, TRUE);
        BCM_DEFIP_PAIR128_USED_COUNT(unit)++;
        soc_lpm_stat_128b_count_update(unit, 1);
    }

    return rv;

}

/*
 * Function:
 *      _bcm_l3_defip_pair128_del
 * Purpose:
 *      Delete an entry from L3_DEFIP_PAIR_128 table.
 * Parameters:
 *      unit    - (IN)SOC unit number.
 *      lpm_cfg - (IN)Buffer to fill defip information. 
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_l3_defip_pair128_del(int unit, _bcm_defip_cfg_t *lpm_cfg) 
{
    uint32 hw_entry[SOC_MAX_MEM_FIELD_WORDS];/* Hw entry buffer.          */
    uint32 temp_hw_entry[SOC_MAX_MEM_FIELD_WORDS];/* Hw entry buffer.          */
    bcm_ip6_t mask;                          /* Subnet mask.              */
    int hw_index;                            /* Entry offset in the TCAM. */ 
    int rv;    
    _bcm_defip_cfg_t temp_lpm;
    int temp_nh_ecmp_idx, idx;

    /* Input parameters check. */
    if (lpm_cfg == NULL) {
        return (BCM_E_PARAM);
    }

    /* Create mask from prefix length. */
    bcm_ip6_mask_create(mask, lpm_cfg->defip_sub_len);

    /* Apply mask on address. */
    bcm_xgs3_l3_mask6_apply(mask, lpm_cfg->defip_ip6_addr);
    /* Search for matching entry in theL3_DEFIP_PAIR_128  cam.  */
    rv =  _bcm_defip_pair128_match(unit, lpm_cfg, hw_entry, &hw_index);
    if (BCM_FAILURE(rv)) {
        return (rv);
    }

    /* Delete entry from hardware & software (L3_DEFIP_PAIR_128m). */
    rv = _bcm_defip_pair128_entry_clear(unit, hw_index);
    if (BCM_SUCCESS(rv)) {
        BCM_XGS3_L3_DEFIP_CNT_DEC(unit, TRUE);
        BCM_DEFIP_PAIR128_USED_COUNT(unit)--;
        soc_lpm_stat_128b_count_update(unit, 0);
        /* Delete valid entry at highest index (or lowest LPM) in 
         * L3_DEFIP_128m table if available
         * and add it to L3_DEFIP_PAIR_128m */
        hw_index = BCM_DEFIP_PAIR128_IDX_MAX(unit);
        for (idx = BCM_DEFIP_PAIR128_IDX_MAX(unit); idx >=0; idx--) {
            if (0 == BCM_DEFIP_PAIR128_ARR(unit)[idx].prefix_len) {
                hw_index =  idx - 1;
                continue;
            }
            break;
        }

        rv = BCM_XGS3_MEM_READ(unit, L3_DEFIP_128m, hw_index, &temp_hw_entry);
        if (rv != BCM_E_NONE && rv != BCM_E_UNAVAIL) {  
            return rv;
        }
        if (rv != BCM_E_UNAVAIL) {
            sal_memset(&temp_lpm, 0, sizeof(_bcm_defip_cfg_t));
            /* Parse entry. */
            rv = _bcm_defip_pair128_parse(unit, temp_hw_entry,
                                                &temp_lpm, &temp_nh_ecmp_idx);
            BCM_IF_ERROR_RETURN(rv);
        
            /* Extract entry address vrf */
            rv =  _bcm_defip_pair128_get_key(unit, temp_hw_entry, &temp_lpm);
            BCM_IF_ERROR_RETURN(rv);

            /* Add the copied entry to L3_DEFIP_PAIR_128 table */
            temp_lpm.defip_index = BCM_XGS3_L3_INVALID_INDEX;
            rv = _bcm_l3_defip_pair128_add(unit, &temp_lpm, temp_nh_ecmp_idx);
            BCM_IF_ERROR_RETURN(rv);

            
            /* Delete entry from L3_DEFIP_128 table. */
            temp_lpm.defip_index = hw_index;
            rv = BCM_E_UNAVAIL;
            rv = _bcm_l3_defip_pair128_del(unit, &temp_lpm);
            return rv;
        }
        return BCM_E_NONE;  
    }
    return (rv);
}

/*
 * Function:
 *      _bcm_l3_defip_pair128_update_match
 * Purpose:
 *      Update/Delete all entries in defip table matching a certain rule.
 * Parameters:
 *      unit     - (IN)SOC unit number.
 *      trv_data - (IN)Delete pattern + compare,act,notify routines.  
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_l3_defip_pair128_update_match(int unit, _bcm_l3_trvrs_data_t *trv_data)
{
    int idx;                                 /* Iteration index.           */
    int rv;                                  /* Operation return status.   */
    int cmp_result;                          /* Test routine result.       */
    int nh_ecmp_idx;                         /* Next hop/Ecmp group index. */
    uint32 *hw_entry;                        /* Hw entry buffer.           */
    char *lpm_tbl_ptr;                       /* Dma table pointer.         */
    int defip_pair128_tbl_size;                    /* Defip table size.          */
    _bcm_defip_cfg_t lpm_cfg;                /* Buffer to fill route info. */
    _bcm_trvs_range_t *range_info;
    int max_index,start_index = 0;               /* START/MAX index to traverse on Entries. */

    /* Table DMA the LPM table to software copy */
    BCM_IF_ERROR_RETURN
        (bcm_xgs3_l3_tbl_dma(unit, L3_DEFIP_PAIR_128m, BCM_L3_MEM_ENT_SIZE(unit, L3_DEFIP_PAIR_128m),
                             "lpm_tbl", &lpm_tbl_ptr, &defip_pair128_tbl_size));

    /* If soc_feature_l3_defip_advanced_lookup is TRUE then 
     * both DIP and SIP lookup is done using 1 L3_DEFIP entry.
     * L3_DEFIP table is not divided into 2 to support URPF (Ex: KT2).
     */
    if (SOC_URPF_STATUS_GET(unit) &&
        !soc_feature(unit, soc_feature_l3_defip_advanced_lookup)) {
        defip_pair128_tbl_size >>= 1;
    } 

    max_index = defip_pair128_tbl_size;
    range_info = trv_data->pattern;
    if ((range_info != NULL) && (defip_pair128_tbl_size > range_info->end)) {
         max_index = range_info->end;
         start_index = range_info->start;
    } 

    for (idx = start_index ; idx < max_index; idx++) {
        /* Calculate entry ofset. */
        hw_entry = soc_mem_table_idx_to_pointer(unit, L3_DEFIP_PAIR_128m, uint32 *,
                                                lpm_tbl_ptr, idx);

        /* Make sure entry is valid. */
        if (!SOC_MEM_OPT_FLD32_GET(unit, L3_DEFIP_PAIR_128m, hw_entry, VALID0_LWRf)) {
            continue;
        }

        /* Zero destination buffer first. */
        sal_memset(&lpm_cfg, 0, sizeof(_bcm_defip_cfg_t));

        /* Parse  the entry. */
        _bcm_defip_pair128_parse(unit, hw_entry, &lpm_cfg, &nh_ecmp_idx);
       lpm_cfg.defip_index = idx;

        /* Fill entry ip address &  subnet mask. */
        _bcm_defip_pair128_get_key(unit, hw_entry, &lpm_cfg);

        /* Execute operation routine if any. */
        if (trv_data->op_cb) {
            rv = (*trv_data->op_cb) (unit, (void *)trv_data,
                                     (void *)&lpm_cfg,
                                     (void *)&nh_ecmp_idx, &cmp_result);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, lpm_tbl_ptr);
                return rv;
            }
        }
#ifdef BCM_WARM_BOOT_SUPPORT
        if (SOC_WARM_BOOT(unit)) {
            rv = _bcm_defip_pair128_reinit(unit, idx, &lpm_cfg);
            if (BCM_FAILURE(rv)) {
                soc_cm_sfree(unit, lpm_tbl_ptr);
                return rv;
            }
        }
#endif /* BCM_WARM_BOOT_SUPPORT */
    }
    
#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        SOC_IF_ERROR_RETURN(soc_fb_lpm_reinit_done(unit, 1));
    }
#endif /* BCM_WARM_BOOT_SUPPORT */
    soc_cm_sfree(unit, lpm_tbl_ptr);
    return (BCM_E_NONE);
}


/*
 * Function:
 *      _bcm_l3_defip_init
 * Purpose:
 *      Configure TD2 internal route table as per soc properties.
 *      (tcam pairing for IPv4, 64/128 bit IPV6 prefixes)
 * Parameters:
 *      unit     - (IN)SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_l3_defip_init(int unit)
{
    int num_ipv6_128b_entries;
    uint32 defip_key_sel_val = 0;
    int mem_v4, mem_v6, mem_v6_128; /* Route table memories */
    int ipv6_64_depth = 0;
    int tcam_pair_count = 0; 
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit);

    num_ipv6_128b_entries =  SOC_L3_DEFIP_MAX_128B_ENTRIES(unit);
    BCM_XGS3_L3_IP6_MAX_128B_ENTRIES(unit) = num_ipv6_128b_entries;
#ifdef ALPM_ENABLE
    /* ALPM Mode */
    if (soc_property_get(unit, spn_L3_ALPM_ENABLE, 0)) {
        int rv = BCM_E_NONE;
        rv = soc_alpm_init(unit);
        if (BCM_SUCCESS(rv)) {
            LOG_VERBOSE(BSL_LS_SOC_COMMON,
                        (BSL_META_U(unit,
                                    "ALPM mode initialized\n")));
        } else {
            LOG_ERROR(BSL_LS_BCM_ALPM,
                      (BSL_META_U(unit,
                                  "ALPM mode initialization failed, "
                                  "retVal = %d\n"), rv));
        }
        return rv;
    }
#else /* ALPM_ENABLE */
    if (soc_property_get(unit, spn_L3_ALPM_ENABLE, 0)) {
        LOG_WARN(BSL_LS_BCM_ALPM,
                 (BSL_META_U(unit,
                             "ALPM mode support is not compiled. Please, "
                             "recompile the SDK with ALPM_ENABLE proprocessor variable "
                             "defined\n"))); 
    }
#endif /* ALPM_ENABLE */

    SOC_IF_ERROR_RETURN(soc_fb_lpm_tcam_pair_count_get(unit, &tcam_pair_count));

    switch (tcam_pair_count) {
        case 1:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                              V6_KEY_SEL_CAM0_1f, 0x1);
            break; 

        case 2:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                              V6_KEY_SEL_CAM0_1f, 0x1);
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                              V6_KEY_SEL_CAM2_3f, 0x1);
            break; 

        case 3:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                              V6_KEY_SEL_CAM0_1f, 0x1);
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                              V6_KEY_SEL_CAM2_3f, 0x1);
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                              V6_KEY_SEL_CAM4_5f, 0x1);
            break; 

        case 4:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                              V6_KEY_SEL_CAM0_1f, 0x1);
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                              V6_KEY_SEL_CAM2_3f, 0x1);
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                              V6_KEY_SEL_CAM4_5f, 0x1);
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                              V6_KEY_SEL_CAM6_7f, 0x1);
            break; 

        default:
            break;
    }

    ipv6_64_depth =  soc_mem_index_count(unit, L3_DEFIPm);
    BCM_IF_ERROR_RETURN(WRITE_L3_DEFIP_KEY_SELr(unit, defip_key_sel_val));

    /* Get memory for IPv4 entries. */
    BCM_IF_ERROR_RETURN(_bcm_l3_defip_mem_get(unit, 0, 0, &mem_v4));

    /* Initialize IPv4 entries lookup engine. */
    BCM_IF_ERROR_RETURN(soc_fb_lpm_init(unit));

    /* Overriding defaults set during lpm init */

    if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        BCM_IF_ERROR_RETURN(soc_fb_lpm128_init(unit));
        ipv6_64_depth -= (tcam_pair_count * tcam_depth * 2);
        SOC_LPM_STATE_START(unit, (MAX_PFX_INDEX)) =
            (tcam_pair_count * tcam_depth * 2);
        SOC_LPM_STATE_END(unit, (MAX_PFX_INDEX)) =
                  SOC_LPM_STATE_START(unit, (MAX_PFX_INDEX))  - 1;
    } else {
        SOC_LPM_STATE_START(unit, (MAX_PFX_INDEX)) = -1;
        SOC_LPM_STATE_END(unit, (MAX_PFX_INDEX))   = - 1;
    }

    SOC_LPM_STATE_FENT(unit, (MAX_PFX_INDEX))  = ipv6_64_depth;
    /* Get memory for IPv6 entries. */
    BCM_IF_ERROR_RETURN
        (_bcm_l3_defip_mem_get(unit, BCM_L3_IP6, 0, &mem_v6));

    /* Get memory for IPv6 entries with prefix length > 64. */
    BCM_IF_ERROR_RETURN
        (_bcm_l3_defip_mem_get(unit, BCM_L3_IP6,
                                  BCM_XGS3_L3_IPV6_PREFIX_LEN,
                                  &mem_v6_128));

    /* Initialize IPv6 entries lookup engine. */
    if (mem_v6 != mem_v6_128) {
        if (!soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
            BCM_IF_ERROR_RETURN(_bcm_defip_pair128_init(unit));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_l3_defip_deinit
 * Purpose:
 *      De-initialize route table for trident2 devices.
 * Parameters:
 *      unit - (IN)  SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_l3_defip_deinit(unit)
{
    soc_mem_t mem_v4;      /* IPv4 Route table memory.             */
    soc_mem_t mem_v6;      /* IPv6 Route table memory.             */
    soc_mem_t mem_v6_128;  /* IPv6 full prefix route table memory. */

    /* Get memory for IPv4 entries. */
    BCM_IF_ERROR_RETURN(_bcm_l3_defip_mem_get(unit, 0, 0, &mem_v4));

    /* Initialize IPv4 entries lookup engine. */
    BCM_IF_ERROR_RETURN(soc_fb_lpm_deinit(unit));

    /* Get memory for IPv6 entries. */
    BCM_IF_ERROR_RETURN
        (_bcm_l3_defip_mem_get(unit, BCM_L3_IP6, 0, &mem_v6));

    /* Initialize IPv6 entries lookup engine. */
    if (mem_v4 != mem_v6) {
        BCM_IF_ERROR_RETURN(soc_fb_lpm_deinit(unit));
    }

    /* Get memory for IPv6 entries with prefix length > 64. */
    BCM_IF_ERROR_RETURN
        (_bcm_l3_defip_mem_get(unit, BCM_L3_IP6,
                                  BCM_XGS3_L3_IPV6_PREFIX_LEN,
                                  &mem_v6_128));

    /* Initialize IPv6 entries lookup engine. */
    if (mem_v6 != mem_v6_128) {
         if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {   
             BCM_IF_ERROR_RETURN(soc_fb_lpm128_deinit(unit));   
         } else {
             BCM_IF_ERROR_RETURN(_bcm_defip_pair128_deinit(unit));
         } 
    }
    return (BCM_E_NONE);
}

STATIC int
_bcm_l3_lpm_table_sizes_get(int unit, int *paired_table_size, 
                            int *defip_table_size)
{
    return soc_fb_lpm_table_sizes_get(unit, paired_table_size, 
                                      defip_table_size);
}

int 
_bcm_l3_is_v4_64b_allowed_in_paired_tcam(int unit)
{
    return soc_fb_lpm128_is_v4_64b_allowed_in_paired_tcam(unit);
}

int
_bcm_l3_scaled_lpm_get(int unit, _bcm_defip_cfg_t *lpm_cfg, int *nh_ecmp_idx)
{
    int rv = BCM_E_NOT_FOUND;
    int defip_table_size = 0;
    int paired_table_size = 0;
 
    if (!soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
       return BCM_E_UNAVAIL;
    }

    BCM_IF_ERROR_RETURN(_bcm_l3_lpm_table_sizes_get(unit, &paired_table_size,
                                                    &defip_table_size));

    if (paired_table_size == 0 && lpm_cfg->defip_sub_len > 64) {
        return BCM_E_NOT_FOUND;
    }

    if ((paired_table_size != 0) && 
        (lpm_cfg->defip_sub_len > 64 || defip_table_size == 0)) {
        return  _bcm_fb_lpm128_get(unit, lpm_cfg, nh_ecmp_idx);
    }
  
    rv = _bcm_fb_lpm_get(unit, lpm_cfg, nh_ecmp_idx);
    if ((rv == BCM_E_NOT_FOUND) && 
         _bcm_l3_is_v4_64b_allowed_in_paired_tcam(unit)) {
        rv =  _bcm_fb_lpm128_get(unit, lpm_cfg, nh_ecmp_idx);
        if (BCM_FAILURE(rv)) {
            lpm_cfg->defip_flags_high &= ~BCM_XGS3_L3_ENTRY_IN_DEFIP_PAIR;
        }
    }
 
    return rv;
}

/*
 * Function:
 *      _bcm_l3_lpm_get
 * Purpose:
 *      Get an entry from the route table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 *      nh_ecmp_idx - (IN)Next hop or ecmp group index
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_l3_lpm_get(int unit, _bcm_defip_cfg_t *lpm_cfg, int *nh_ecmp_idx)
{
    soc_mem_t mem = L3_DEFIPm;
    uint32 max_v6_entries = BCM_XGS3_L3_IP6_MAX_128B_ENTRIES(unit);
  
    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_l3_defip_mem_get(unit, lpm_cfg->defip_flags,
                                              lpm_cfg->defip_sub_len, &mem));

    if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        return _bcm_l3_scaled_lpm_get(unit, lpm_cfg, nh_ecmp_idx);
    }

    switch (mem) {
        case L3_DEFIP_PAIR_128m:
            if (max_v6_entries > 0) {    
                return _bcm_l3_defip_pair128_get(unit, lpm_cfg, nh_ecmp_idx);
            }
            break;  
        default:
            if (soc_mem_index_count(unit, L3_DEFIPm) > 0) {
                return _bcm_fb_lpm_get(unit, lpm_cfg, nh_ecmp_idx);
            }
            break;
    }

    return (BCM_E_NOT_FOUND);
}

void
_bcm_defip_cfg_t_dump(_bcm_defip_cfg_t *lpm_cfg)
{
    int i = 0;

    if (lpm_cfg) {
        LOG_CLI((BSL_META("flags: 0x%x  vrf: %d\n"), lpm_cfg->defip_flags, lpm_cfg->defip_vrf));
        if (lpm_cfg->defip_flags & BCM_L3_IP6) {
            LOG_CLI((BSL_META("defip_ip6_addr - ")));
            for (i = 0; i < sizeof(bcm_ip6_t); i++) {
                LOG_CLI((BSL_META("0x%x:"), lpm_cfg->defip_ip6_addr[i]));
            }
            LOG_CLI((BSL_META("\n")));
        } else {
            LOG_CLI((BSL_META("defip_ip_addr: 0x%x\n"), lpm_cfg->defip_ip_addr));
        }
        LOG_CLI((BSL_META("defip_sub_len: %d defip_index: %d\n"), 
                 lpm_cfg->defip_sub_len, lpm_cfg->defip_index));
        for(i = 0; i < sizeof(bcm_mac_t); i++) {
            LOG_CLI((BSL_META("defip_mac_addr - ")));
            LOG_CLI((BSL_META("0x%x:"), lpm_cfg->defip_mac_addr[i]));
        }

        if (lpm_cfg->defip_flags & BCM_L3_IP6) {
            LOG_CLI((BSL_META("\ndefip_nexthop_ip6 - ")));
            for (i = 0; i < sizeof(bcm_ip6_t); i++) {
                LOG_CLI((BSL_META("0x%x:"), lpm_cfg->defip_nexthop_ip6[i])); 
            }
            LOG_CLI((BSL_META("\n")));
        } else {
            LOG_CLI((BSL_META("defip_nexthop_ip: 0x%x\n"), lpm_cfg->defip_nexthop_ip));
        }

        LOG_CLI((BSL_META("defip_tunnel: %d defip_prio: %d\n"), 
                 lpm_cfg->defip_tunnel, lpm_cfg->defip_prio));

        LOG_CLI((BSL_META("defip_intf: %d defip_port_tgid: %d\n"), 
                 lpm_cfg->defip_intf, lpm_cfg->defip_port_tgid));

        LOG_CLI((BSL_META("defip_stack_port: %d defip_modid: %d\n"), 
                 lpm_cfg->defip_stack_port, lpm_cfg->defip_modid));

        LOG_CLI((BSL_META("defip_vid: %d defip_ecmp: %d\n"), 
                 lpm_cfg->defip_vid, lpm_cfg->defip_ecmp));

        LOG_CLI((BSL_META("defip_ecmp_count: %d defip_ecmp_index: %d\n"), 
                 lpm_cfg->defip_ecmp_count, lpm_cfg->defip_ecmp_index));

        LOG_CLI((BSL_META("defip_l3hw_index: %d defip_tunnel_option: %d\n"), 
                 lpm_cfg->defip_l3hw_index, lpm_cfg->defip_tunnel_option));

        LOG_CLI((BSL_META("defip_mpls_label: %d defip_lookup_class: %d\n"),
                 lpm_cfg->defip_mpls_label, lpm_cfg->defip_lookup_class));
    }
}

/* When a entry is delete in unpaired TCAM, then check if you can move
 * 64B V6 or V4 entries back to unpaired TCAM. This is required to keep
 * the entries in order
 */
int _bcm_l3_lpm128_ripple_entries(int unit)
{
    _bcm_defip_cfg_t lpm_cfg_array[2];
    int nh_ecmp_idx[2];
    uint32 e[SOC_MAX_MEM_FIELD_WORDS];
    uint32 eupr[SOC_MAX_MEM_FIELD_WORDS];
    int index   = 0;
    int pfx_len = 0;
    int count   = 0;
    int ipv6    = 0;
    int rv = BCM_E_NONE;
    int idx = 0;

    /* Do the following 
     * 1. Find the smallest 64BV6 prefix
     * 2. if 64BV6 entry not found, find the smallest V4 prefix
     * 3. Try to add it to the unpaired tcam
     * 4. If adding to the unpaired tcam is successful, 
     *    then delete the entry from paired TCAM
     */
    sal_memcpy(e, soc_mem_entry_null(unit, L3_DEFIPm),
               soc_mem_entry_words(unit,L3_DEFIPm) * 4);
    sal_memcpy(eupr, soc_mem_entry_null(unit, L3_DEFIPm),
               soc_mem_entry_words(unit,L3_DEFIPm) * 4);
    ipv6 = 1; 
    rv = _bcm_fb_lpm128_get_smallest_movable_prefix(unit, ipv6, e, eupr, &index,
                                                    &pfx_len, &count); 
    if (BCM_FAILURE(rv)) {
        if (rv == BCM_E_NOT_FOUND) {
            ipv6 = 0; 
            rv = _bcm_fb_lpm128_get_smallest_movable_prefix(unit, ipv6, e,eupr,
                                                            &index, &pfx_len,
                                                            &count);
            if (BCM_FAILURE(rv)) {
                return BCM_E_NONE;
            }
        } else {
            return rv;
        }
    }
    
    if (ipv6) {
        BCM_IF_ERROR_RETURN(_bcm_fb_lpm128_defip_cfg_get(unit, e, eupr,
                                                      lpm_cfg_array,
                                                      nh_ecmp_idx));
    } else {
        BCM_IF_ERROR_RETURN(_bcm_fb_lpm_defip_cfg_get(unit, ipv6, e,
                                                      lpm_cfg_array,
                                                      nh_ecmp_idx));
    }
   for (idx = 0; idx < count; idx++) {
       lpm_cfg_array[idx].defip_index = BCM_XGS3_L3_INVALID_INDEX;
       lpm_cfg_array[idx].defip_flags_high = 0;
       rv = _bcm_fb_lpm_add(unit, &lpm_cfg_array[idx], 
                                      nh_ecmp_idx[idx]);
       if (BCM_FAILURE(rv)) {
           return BCM_E_NONE;
       }
       BCM_IF_ERROR_RETURN(_bcm_fb_lpm128_del(unit, &lpm_cfg_array[idx]));
   }

   return BCM_E_NONE;
}

int
_bcm_l3_lpm_ripple_entries(int unit, _bcm_defip_cfg_t *lpm_cfg,
                           int new_nh_ecmp_idx)
{

    _bcm_defip_cfg_t lpm_cfg_array[2];
    int nh_ecmp_idx[2];
    uint32 e[SOC_MAX_MEM_FIELD_WORDS];
    int index   = 0;
    int pfx_len = 0;
    int count   = 0;
    int ipv6    = 0;
    int rv = BCM_E_NONE;
    int idx = 0;

    /* Do the following 
     * 1. Find the largest V4 prefix
     * 2. if V4 entry not found, find the largest 64B V6 prefix
     * 3. Try to add it to the paired tcam
     * 4. If adding to the paired tcam is successful, 
     *    then delete the entry
     * 5. Add this entry again
     */
    sal_memcpy(e, soc_mem_entry_null(unit, L3_DEFIPm),
               soc_mem_entry_words(unit,L3_DEFIPm) * 4);
    sal_memset(&lpm_cfg_array[0], 0x0, sizeof(_bcm_defip_cfg_t));
    sal_memset(&lpm_cfg_array[1], 0x0, sizeof(_bcm_defip_cfg_t));

    ipv6 = 0; 
    rv = _bcm_fb_get_largest_prefix(unit, ipv6, e, &index, 
                                   &pfx_len, &count); 
    if (BCM_FAILURE(rv)) {
        if (rv == BCM_E_NOT_FOUND) {
            ipv6 = 1; 
            rv = _bcm_fb_get_largest_prefix(unit, ipv6, e,
                                           &index,
                                           &pfx_len, &count);
            if (BCM_FAILURE(rv)) {
                /* Failure here indicates something fatal */
                return BCM_E_INTERNAL; 
            }
        } else {
            return BCM_E_FULL;
        }
    }
    BCM_IF_ERROR_RETURN(_bcm_fb_lpm_defip_cfg_get(unit, ipv6, e, lpm_cfg_array,
                                                 nh_ecmp_idx));
    if (!(lpm_cfg->defip_flags & BCM_L3_IP6) && 
        lpm_cfg_array[0].defip_flags & BCM_L3_IP6) {
        return _bcm_fb_lpm128_add(unit, lpm_cfg, new_nh_ecmp_idx);
        
    }

    if (((lpm_cfg->defip_flags & BCM_L3_IP6) &&
         !(lpm_cfg_array[0].defip_flags & BCM_L3_IP6)) ||
         lpm_cfg_array[0].defip_sub_len > lpm_cfg->defip_sub_len) {
        for (idx = 0; idx < count; idx++) {
            lpm_cfg_array[idx].defip_index = BCM_XGS3_L3_INVALID_INDEX;
            rv = _bcm_fb_lpm128_add(unit, &lpm_cfg_array[idx], 
                                           nh_ecmp_idx[idx]);
            lpm_cfg_array[idx].defip_flags_high = 0x0;
            if (BCM_FAILURE(rv)) {
                return rv;
            }
            BCM_IF_ERROR_RETURN(_bcm_fb_lpm_del(unit, &lpm_cfg_array[idx]));
        }
        return _bcm_fb_lpm_add(unit, lpm_cfg, new_nh_ecmp_idx);
    } else {
        return _bcm_fb_lpm128_add(unit, lpm_cfg, new_nh_ecmp_idx);
    }

   return BCM_E_NONE;
}

int
_bcm_l3_scaled_lpm_add(int unit, _bcm_defip_cfg_t *lpm_cfg, int nh_ecmp_idx)
{
    int rv = BCM_E_NONE;  
    int defip_table_size = 0;
    int paired_table_size = 0;

    if (!soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        return BCM_E_UNAVAIL;
    }

    BCM_IF_ERROR_RETURN(_bcm_l3_lpm_table_sizes_get(unit, &paired_table_size, 
                                                    &defip_table_size));

    if (paired_table_size == 0 && lpm_cfg->defip_sub_len > 64) {
        return BCM_E_FULL;
    }

    if ((paired_table_size != 0 && lpm_cfg->defip_sub_len > 64) || 
        (defip_table_size == 0)) {
        return _bcm_fb_lpm128_add(unit, lpm_cfg, nh_ecmp_idx);
    }

    rv = _bcm_fb_lpm_add(unit, lpm_cfg, nh_ecmp_idx);
    if ((rv == BCM_E_FULL) && _bcm_l3_is_v4_64b_allowed_in_paired_tcam(unit)) {
        return _bcm_l3_lpm_ripple_entries(unit, lpm_cfg,
                                          nh_ecmp_idx);
    }
    return rv;
}

/*
 * Function:
 *      _bcm_l3_lpm_add
 * Purpose:
 *      Add an entry to TD2 route table table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 frtu[]*      nh_ecmp_idx - (IN)Next hop or ecmp group index.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_l3_lpm_add(int unit, _bcm_defip_cfg_t *lpm_cfg, int nh_ecmp_idx)
{
    soc_mem_t mem = L3_DEFIPm;

    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_l3_defip_mem_get(unit, lpm_cfg->defip_flags,
                                              lpm_cfg->defip_sub_len, &mem));

    if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {    
        return _bcm_l3_scaled_lpm_add(unit, lpm_cfg, nh_ecmp_idx);
    }
    
    switch (mem) {
        case L3_DEFIP_PAIR_128m:
            if (soc_mem_index_count(unit, L3_DEFIP_PAIR_128m) > 0) { 
                return _bcm_l3_defip_pair128_add(unit, lpm_cfg, nh_ecmp_idx);
            }
            break; 
        default:
            if (soc_mem_index_count(unit, L3_DEFIPm) > 0) {
                return _bcm_fb_lpm_add(unit, lpm_cfg, nh_ecmp_idx);
            }
            break; 
    }
    return (BCM_E_FULL);
}

int
_bcm_l3_scaled_lpm_del(int unit, _bcm_defip_cfg_t *lpm_cfg)
{
    int rv = BCM_E_NONE;
    int defip_table_size = 0;
    int paired_table_size = 0;

    if (!soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        return BCM_E_UNAVAIL;
    }

   BCM_IF_ERROR_RETURN(_bcm_l3_lpm_table_sizes_get(unit, &paired_table_size,
                                                   &defip_table_size));

    if ((paired_table_size != 0) && (lpm_cfg->defip_sub_len > 64 ||
        (defip_table_size == 0) ||
        lpm_cfg->defip_flags_high & BCM_XGS3_L3_ENTRY_IN_DEFIP_PAIR)) {
        return _bcm_fb_lpm128_del(unit, lpm_cfg);
    }

    if (paired_table_size == 0 && lpm_cfg->defip_sub_len > 64) {
        return BCM_E_NOT_FOUND;
    }

    lpm_cfg->defip_flags_high = 0;

    rv = _bcm_fb_lpm_del(unit, lpm_cfg);
    if (BCM_SUCCESS(rv) && _bcm_l3_is_v4_64b_allowed_in_paired_tcam(unit)) {
            return _bcm_l3_lpm128_ripple_entries(unit);
    }
    return rv;
}

/*
 * Function:
 *      _bcm_l3_lpm_del
 * Purpose:
 *      Delete an entry from the route table.
 * Parameters:
 *      unit    - (IN)SOC unit number.
 *      lpm_cfg - (IN)Buffer to fill defip information. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_l3_lpm_del(int unit, _bcm_defip_cfg_t *lpm_cfg)
{
    soc_mem_t mem = L3_DEFIPm;
    uint32 max_v6_entries = BCM_XGS3_L3_IP6_MAX_128B_ENTRIES(unit); 
    int rv = BCM_E_NONE;

    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_l3_defip_mem_get(unit, lpm_cfg->defip_flags,
                                              lpm_cfg->defip_sub_len, &mem));

    if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        return _bcm_l3_scaled_lpm_del(unit, lpm_cfg);
    }

    switch (mem) {
        case L3_DEFIP_PAIR_128m:
            if (max_v6_entries > 0) {    
                return _bcm_l3_defip_pair128_del(unit, lpm_cfg);
            }   
            break;  
        default: 
            if (soc_mem_index_count(unit, L3_DEFIPm) > 0) {
                return _bcm_fb_lpm_del(unit, lpm_cfg);
            } 
            break;  
    }

    return (rv);
}

int _bcm_l3_scaled_lpm_update_match(int unit, _bcm_l3_trvrs_data_t *trv_data)
{
    int rv1 = BCM_E_NONE;
    int rv2 = BCM_E_NONE;
    int defip_table_size = 0;
    int paired_table_size = 0;

    if (!soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        return BCM_E_UNAVAIL;
    }

    BCM_IF_ERROR_RETURN(_bcm_l3_lpm_table_sizes_get(unit, &paired_table_size,
                                                    &defip_table_size));
    
    if (paired_table_size != 0) {
        rv1 = _bcm_fb_lpm128_update_match(unit, trv_data);
    }
   
    if (defip_table_size != 0) {
        rv2 = _bcm_fb_lpm_update_match(unit, trv_data);
    }
   
    BCM_IF_ERROR_RETURN(rv1);
    BCM_IF_ERROR_RETURN(rv2);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_l3_lpm_update_match
 * Purpose:
 *      Update/Delete all entries in defip table matching a certain rule.
 * Parameters:
 *      unit     - (IN)SOC unit number.
 *      trv_data - (IN)Delete pattern + compare,act,notify routines.  
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_l3_lpm_update_match(int unit, _bcm_l3_trvrs_data_t *trv_data)
{
    int rv1 = BCM_E_NONE; 
    int rv2 = BCM_E_NONE; 
    uint32 max_v6_entries = BCM_XGS3_L3_IP6_MAX_128B_ENTRIES(unit);

    if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        return _bcm_l3_scaled_lpm_update_match(unit, trv_data);
    }

    if ((trv_data->flags & BCM_L3_IP6) && (max_v6_entries > 0)) {
        rv2 = _bcm_l3_defip_pair128_update_match(unit, trv_data);
    }

    if (soc_mem_index_count(unit, L3_DEFIPm) > 0) {
        rv1 = _bcm_fb_lpm_update_match(unit, trv_data);
    }
    BCM_IF_ERROR_RETURN(rv1);
    BCM_IF_ERROR_RETURN(rv2);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_l3_defip_urpf_enable
 * Purpose:
 *      Configure TD2 internal route table for URPF mode
 * Parameters:
 *      unit     - (IN)SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_l3_defip_urpf_enable(int unit, int enable)
{
    uint32 defip_key_sel_val = 0;
    int ipv6_64_depth = 0;
    int ipv6_128_dip_offset = 0, ipv6_128_depth = 0;
    int num_ipv6_128b_entries;
    int tcam_pair_count = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit);
    int start_index = 0;
#ifdef ALPM_ENABLE
    if (soc_property_get(unit, spn_L3_ALPM_ENABLE, 0)) {
        return SOC_E_NONE;
    }
#endif
    num_ipv6_128b_entries =  SOC_L3_DEFIP_MAX_128B_ENTRIES(unit);
    SOC_IF_ERROR_RETURN(soc_fb_lpm_tcam_pair_count_get(unit, &tcam_pair_count));

    if (enable) {
        /* All CAMs in URPF mode */
        soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                          URPF_LOOKUP_CAM4f, 0x1);
        soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                          URPF_LOOKUP_CAM5f, 0x1);
        soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                          URPF_LOOKUP_CAM6f, 0x1);
        soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                          URPF_LOOKUP_CAM7f, 0x1);

        switch (tcam_pair_count) {
            case 0: 
               /* No pairing of tcams */
               ipv6_64_depth = 4 * tcam_depth;
               break;

            case 1:
            case 2:    
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM0_1f, 0x1);
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM4_5f, 0x1);
                if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
                    ipv6_64_depth = 2 * tcam_depth;
                }
                break; 

              default:  
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM0_1f, 0x1);
                 soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM2_3f, 0x1);
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM4_5f, 0x1);
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM6_7f, 0x1);

                if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
                    ipv6_64_depth = 0;
                }
                break; 
        }

        if (!soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
            ipv6_128_dip_offset = num_ipv6_128b_entries / 2;
            ipv6_128_depth      = num_ipv6_128b_entries / 2;
            ipv6_64_depth =  soc_mem_index_count(unit, L3_DEFIPm) / 2;
        } 
    } else { /* Disable case */
        defip_key_sel_val = 0; /* reset */
        switch (tcam_pair_count) {
            case 1:
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM0_1f, 0x1);
                break; 

            case 2:
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM0_1f, 0x1);
                 soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM2_3f, 0x1);
                  break; 

            case 3:
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM0_1f, 0x1);
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM2_3f, 0x1);
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM4_5f, 0x1);
                break; 

            case 4:
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM0_1f, 0x1);
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM2_3f, 0x1);
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM4_5f, 0x1);
                soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val, 
                                  V6_KEY_SEL_CAM6_7f, 0x1);
                 break; 

            default:
                break;
 
        }
        ipv6_64_depth =  soc_mem_index_count(unit, L3_DEFIPm);
        ipv6_128_dip_offset = 0;
        ipv6_128_depth      = num_ipv6_128b_entries;
        if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
            ipv6_64_depth -= (tcam_pair_count * tcam_depth * 2);
        }

    }  

    SOC_LPM_STATE_FENT(unit, (MAX_PFX_INDEX))  = ipv6_64_depth;
    if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        if (!enable) {
           start_index = (tcam_pair_count * tcam_depth * 2);
        } else {
            switch (tcam_pair_count) {
               case 1:
               case 2:
                   start_index = 2 * tcam_depth;
                   break;
               case 3:
               case 4:
                   start_index = 4 * tcam_depth;
                   break;
               default:
                  start_index = 0;
                  break;
            }
        }
        /* reinit the lpm128 logic to get the correct size for table */ 
        SOC_IF_ERROR_RETURN(soc_fb_lpm_stat_init(unit));
        SOC_IF_ERROR_RETURN(soc_fb_lpm128_deinit(unit));
        SOC_IF_ERROR_RETURN(soc_fb_lpm128_init(unit));
    } else {
        BCM_DEFIP_PAIR128_URPF_OFFSET(unit)    = ipv6_128_dip_offset;
        BCM_DEFIP_PAIR128_IDX_MAX(unit)        = ipv6_128_depth - 1;
        BCM_DEFIP_PAIR128_TOTAL(unit)          = ipv6_128_depth;
        SOC_IF_ERROR_RETURN(soc_fb_lpm_stat_init(unit));
    }
    SOC_LPM_STATE_START(unit, (MAX_PFX_INDEX)) = start_index;
    SOC_LPM_STATE_END(unit, (MAX_PFX_INDEX))   = start_index - 1;

    BCM_IF_ERROR_RETURN(WRITE_L3_DEFIP_KEY_SELr(unit, defip_key_sel_val));
    return BCM_E_NONE;
}

#if defined(BCM_KATANA2_SUPPORT) 

#define URPF_OFFSET 0
/*
 * Function:
 *      _bcm_kt2_l3_defip_mem_get
 * Purpose:
 *      Resolve route table memory for a given route entry.
 * Parameters:
 *      unit       - (IN)SOC unit number.
 *      flags      - (IN)IPv6/IPv4 route.
 *      plen       - (IN)Prefix length.
 *      mem        - (OUT)Route table memory.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_kt2_l3_defip_mem_get(int unit, uint32 flags, int plen, soc_mem_t *mem)
{
    *mem = ((flags & BCM_L3_IP6) && (plen > 64)) ?
               L3_DEFIP_PAIR_128m : L3_DEFIPm ;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_kt2_defip_pair128_init
 * Purpose:
 *      Initialize sw image for L3_DEFIP_PAIR_128 internal tcam.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_kt2_defip_pair128_init(int unit)
{
    int rv;
    int mem_sz;
    int defip_config;
    int ipv6_128_depth;

    /* De-allocate any existing structures. */
    if (BCM_DEFIP_PAIR128(unit) != NULL) {
        BCM_IF_ERROR_RETURN(_bcm_defip_pair128_deinit(unit));
    }

    /* Allocate CAM control structure. */
    mem_sz = sizeof(_bcm_defip_pair128_table_t);
    BCM_DEFIP_PAIR128(unit) = sal_alloc(mem_sz, "kt2_l3_defip_pair128");
    if (BCM_DEFIP_PAIR128(unit) == NULL) {
        return (BCM_E_MEMORY);
    }

    /* Reset CAM control structure. */
    sal_memset(BCM_DEFIP_PAIR128(unit), 0, mem_sz);

    defip_config = soc_property_get(unit, spn_IPV6_LPM_128B_ENABLE,1);
    ipv6_128_depth = soc_property_get(unit, spn_NUM_IPV6_LPM_128B_ENTRIES,
                                      (defip_config ? 1024 : 0));

    BCM_DEFIP_PAIR128_TOTAL(unit) = ipv6_128_depth;
    BCM_DEFIP_PAIR128_URPF_OFFSET(unit) = URPF_OFFSET;
    BCM_DEFIP_PAIR128_IDX_MAX(unit) = ipv6_128_depth - 1;
    BCM_XGS3_L3_IP6_MAX_128B_ENTRIES(unit) = ipv6_128_depth;
    if (ipv6_128_depth) {
        mem_sz = ipv6_128_depth * sizeof(_bcm_defip_pair128_entry_t);
        BCM_DEFIP_PAIR128_ARR(unit) = 
            sal_alloc(mem_sz, "kt2_l3_defip_pair128_entry_array");
        if (BCM_DEFIP_PAIR128_ARR(unit) == NULL) {
            BCM_IF_ERROR_RETURN(_bcm_defip_pair128_deinit(unit));
            return (BCM_E_MEMORY);
        }
        sal_memset(BCM_DEFIP_PAIR128_ARR(unit), 0, mem_sz);
    }
    rv = _bcm_defip_pair128_field_cache_init(unit);
    if (BCM_FAILURE(rv)) {
        BCM_IF_ERROR_RETURN(_bcm_defip_pair128_deinit(unit));
        return rv;
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_kt2_l3_defip_init
 * Purpose:
 *      Configure KT2 internal route table as per soc properties.
 *      (TCAM pairing for IPv4, 64/128 bit IPV6 prefixes)
 * Parameters:
 *      unit     - (IN)SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_kt2_l3_defip_init(int unit)
{
    int defip_config;
    uint32 defip_key_sel_val = 0;
    int mem_v4, mem_v6, mem_v6_128; /* Route table memories */
    int ipv6_64_depth = 0;
    int num_ipv6_128b_entries;
    int tcam_pair_count = 0;
    int tcam_depth = SOC_L3_DEFIP_TCAM_DEPTH_GET(unit);

    defip_config = soc_property_get(unit, spn_IPV6_LPM_128B_ENABLE, 1);
    num_ipv6_128b_entries =
        soc_property_get(unit, spn_NUM_IPV6_LPM_128B_ENTRIES,
            (defip_config ? 1024 : 0));

    if (num_ipv6_128b_entries) {
        tcam_pair_count = (num_ipv6_128b_entries / tcam_depth) +
                          ((num_ipv6_128b_entries % tcam_depth) ? 1 : 0);
    }

    BCM_XGS3_L3_IP6_MAX_128B_ENTRIES(unit) = 
                                       SOC_L3_DEFIP_MAX_128B_ENTRIES(unit);

    switch (tcam_pair_count) {
        case 8:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val,
                              V6_KEY_SEL_CAM14_15f, 0x1);
        case 7:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val,
                              V6_KEY_SEL_CAM12_13f, 0x1);
        case 6:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val,
                              V6_KEY_SEL_CAM10_11f, 0x1);
        case 5:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val,
                              V6_KEY_SEL_CAM8_9f, 0x1);
        case 4:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val,
                              V6_KEY_SEL_CAM6_7f, 0x1);
        case 3:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val,
                              V6_KEY_SEL_CAM4_5f, 0x1);
        case 2:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val,
                              V6_KEY_SEL_CAM2_3f, 0x1);
        case 1:
            soc_reg_field_set(unit, L3_DEFIP_KEY_SELr, &defip_key_sel_val,
                              V6_KEY_SEL_CAM0_1f, 0x1);
            break;
        default:
            break;
    }
    ipv6_64_depth = soc_mem_index_count(unit, L3_DEFIPm); 

    BCM_IF_ERROR_RETURN(WRITE_L3_DEFIP_KEY_SELr(unit, defip_key_sel_val));

    /* Get memory for IPv4 entries. */
    BCM_IF_ERROR_RETURN(_bcm_kt2_l3_defip_mem_get(unit, 0, 0, &mem_v4));

    /* Initialize IPv4 entries lookup engine. */
    BCM_IF_ERROR_RETURN(soc_fb_lpm_init(unit));

    /* Get memory for IPv6 entries. */
    BCM_IF_ERROR_RETURN
        (_bcm_kt2_l3_defip_mem_get(unit, BCM_L3_IP6, 0, &mem_v6));

    /* Initialize IPv6 entries lookup engine. */
    if (mem_v4 != mem_v6) {
        BCM_IF_ERROR_RETURN(soc_fb_lpm_init(unit));
    }
    /* Overriding defaults set during lpm init */
    if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        BCM_IF_ERROR_RETURN(soc_fb_lpm128_init(unit));
        ipv6_64_depth -= (tcam_pair_count * tcam_depth * 2);
        SOC_LPM_STATE_START(unit, (MAX_PFX_INDEX)) =
            (tcam_pair_count * tcam_depth * 2);
        SOC_LPM_STATE_END(unit, (MAX_PFX_INDEX)) =
                  SOC_LPM_STATE_START(unit, (MAX_PFX_INDEX))  - 1;
    } else {
        SOC_LPM_STATE_START(unit, (MAX_PFX_INDEX)) = -1;
        SOC_LPM_STATE_END(unit, (MAX_PFX_INDEX))   = -1;
    }

    SOC_LPM_STATE_FENT(unit, (MAX_PFX_INDEX)) = ipv6_64_depth;

    /* Get memory for IPv6 entries with prefix length > 64. */
    BCM_IF_ERROR_RETURN
        (_bcm_kt2_l3_defip_mem_get(unit, BCM_L3_IP6,
                                  BCM_XGS3_L3_IPV6_PREFIX_LEN,
                                  &mem_v6_128));

    /* Initialize IPv6 entries lookup engine. */
    if (mem_v6 != mem_v6_128) {
        if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
            return soc_fb_lpm128_init(unit);
        } else {
            BCM_IF_ERROR_RETURN(_bcm_kt2_defip_pair128_init(unit));
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_kt2_l3_defip_deinit
 * Purpose:
 *      De-initialize route table for katana2 devices.
 * Parameters:
 *      unit - (IN)  SOC unit number.
 * Returns:
 *      BCM_E_XXX
 */
int 
_bcm_kt2_l3_defip_deinit(int unit)
{
    soc_mem_t mem_v4;      /* IPv4 Route table memory.             */
    soc_mem_t mem_v6;      /* IPv6 Route table memory.             */
    soc_mem_t mem_v6_128;  /* IPv6 full prefix route table memory. */

    /* Get memory for IPv4 entries. */
    BCM_IF_ERROR_RETURN(_bcm_kt2_l3_defip_mem_get(unit, 0, 0, &mem_v4));

    /* Deinitialize IPv4 entries lookup engine. */
    BCM_IF_ERROR_RETURN(soc_fb_lpm_deinit(unit));

    /* Get memory for IPv6 entries. */
    BCM_IF_ERROR_RETURN
        (_bcm_kt2_l3_defip_mem_get(unit, BCM_L3_IP6, 0, &mem_v6));

    /* Deinitialize IPv6 entries lookup engine. */
    if (mem_v4 != mem_v6) {
        BCM_IF_ERROR_RETURN(soc_fb_lpm_deinit(unit));
    }

    /* Get memory for IPv6 entries with prefix length > 64. */
    BCM_IF_ERROR_RETURN
        (_bcm_kt2_l3_defip_mem_get(unit, BCM_L3_IP6,
                                  BCM_XGS3_L3_IPV6_PREFIX_LEN,
                                  &mem_v6_128));

    /* Deinitialize IPv6 entries lookup engine. */
    if (mem_v6 != mem_v6_128) {
         if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
             BCM_IF_ERROR_RETURN(soc_fb_lpm128_deinit(unit));
         } else {
             BCM_IF_ERROR_RETURN(_bcm_defip_pair128_deinit(unit));
         }
    }
    return (BCM_E_NONE);

}

/*
 * Function:
 *      _bcm_kt2_l3_lpm_get
 * Purpose:
 *      Get an entry from the route table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 *      nh_ecmp_idx - (IN)Next hop or ecmp group index
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_kt2_l3_lpm_get(int unit, _bcm_defip_cfg_t *lpm_cfg, int *nh_ecmp_idx)
{
    soc_mem_t mem = L3_DEFIPm;
    uint32 max_v6_entries = BCM_XGS3_L3_IP6_MAX_128B_ENTRIES(unit);

    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_kt2_l3_defip_mem_get(unit, lpm_cfg->defip_flags,
                                              lpm_cfg->defip_sub_len, &mem));

    if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        if (mem == L3_DEFIPm || mem == L3_DEFIP_PAIR_128m) {
            return _bcm_l3_scaled_lpm_get(unit, lpm_cfg, nh_ecmp_idx);
        }
    }

    switch (mem) {
        case L3_DEFIP_PAIR_128m:
            if (max_v6_entries > 0) {
                return _bcm_l3_defip_pair128_get(unit, lpm_cfg, nh_ecmp_idx);
            }
            break;
        default:
            if (soc_mem_index_count(unit, L3_DEFIPm) > 0) {
                return _bcm_fb_lpm_get(unit, lpm_cfg, nh_ecmp_idx);
            }
            break;
    }

    return (BCM_E_NOT_FOUND);
}

/*
 * Function:
 *      _bcm_kt2_l3_lpm_add
 * Purpose:
 *      Add an entry to KT2 route table table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 *      nh_ecmp_idx - (IN)Next hop or ecmp group index.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_kt2_l3_lpm_add(int unit, _bcm_defip_cfg_t *lpm_cfg, int nh_ecmp_idx)
{
    soc_mem_t mem = L3_DEFIPm;

    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_kt2_l3_defip_mem_get(unit, lpm_cfg->defip_flags,
                                              lpm_cfg->defip_sub_len, &mem));

    if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        if (mem == L3_DEFIPm || mem == L3_DEFIP_PAIR_128m) {
            return _bcm_l3_scaled_lpm_add(unit, lpm_cfg, nh_ecmp_idx);
        }
    }

    switch (mem) {
        case L3_DEFIP_PAIR_128m:
            if (soc_mem_index_count(unit, L3_DEFIP_PAIR_128m) > 0) {
                return _bcm_l3_defip_pair128_add(unit, lpm_cfg, nh_ecmp_idx);
            }
            break;
        default:
            if (soc_mem_index_count(unit, L3_DEFIPm) > 0) {
                return _bcm_fb_lpm_add(unit, lpm_cfg, nh_ecmp_idx);
            }
            break;
    }

    return (BCM_E_FULL);
}

/*
 * Function:
 *      _bcm_kt2_l3_lpm_del
 * Purpose:
 *      Delete an entry from the route table.
 * Parameters:
 *      unit    - (IN)SOC unit number.
 *      lpm_cfg - (IN)Buffer to fill defip information. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_kt2_l3_lpm_del(int unit, _bcm_defip_cfg_t *lpm_cfg)
{
    soc_mem_t mem = L3_DEFIPm;
    uint32 max_v6_entries = BCM_XGS3_L3_IP6_MAX_128B_ENTRIES(unit);

    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    BCM_IF_ERROR_RETURN(_bcm_kt2_l3_defip_mem_get(unit, lpm_cfg->defip_flags,
                                              lpm_cfg->defip_sub_len, &mem));

    if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        if (mem == L3_DEFIPm || mem == L3_DEFIP_PAIR_128m) {
            return _bcm_l3_scaled_lpm_del(unit, lpm_cfg);
        }
    }

    switch (mem) {
        case L3_DEFIP_PAIR_128m:
            if (max_v6_entries > 0) {
                return _bcm_l3_defip_pair128_del(unit, lpm_cfg);
            }
            break;
        default: 
            if (soc_mem_index_count(unit, L3_DEFIPm) > 0) {
                return _bcm_fb_lpm_del(unit, lpm_cfg);
            }
            break;
    }

    return (BCM_E_NOT_FOUND);
}

/*
 * Function:
 *      _bcm_kt2_l3_lpm_update_match
 * Purpose:
 *      Update/Delete all entries in defip table matching a certain rule.
 * Parameters:
 *      unit     - (IN)SOC unit number.
 *      trv_data - (IN)Delete pattern + compare,act,notify routines.  
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_kt2_l3_lpm_update_match(int unit, _bcm_l3_trvrs_data_t *trv_data)
{
    int rv1 = BCM_E_NONE;
    int rv2 = BCM_E_NONE;
    uint32 max_v6_entries = BCM_XGS3_L3_IP6_MAX_128B_ENTRIES(unit);

    if (NULL == trv_data) {
        return (BCM_E_PARAM);
    }

    if (soc_feature(unit, soc_feature_l3_lpm_scaling_enable)) {
        return _bcm_l3_scaled_lpm_update_match(unit, trv_data);
    }

    if ((trv_data->flags & BCM_L3_IP6) && (max_v6_entries > 0)) {
        rv2 = _bcm_l3_defip_pair128_update_match(unit, trv_data);
    }

    if (soc_mem_index_count(unit, L3_DEFIPm) > 0) {
        rv1 = _bcm_fb_lpm_update_match(unit, trv_data);
    }
    BCM_IF_ERROR_RETURN(rv1);
    BCM_IF_ERROR_RETURN(rv2);

    return (BCM_E_NONE);
}
#endif /* BCM_KATANA2_SUPPORT  */
#endif /* INCLUDE_L3  */
