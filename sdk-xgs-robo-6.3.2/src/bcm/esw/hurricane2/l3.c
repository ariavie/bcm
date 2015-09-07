/* $Id: l3.c 1.2 Broadcom SDK $
 * 
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
 * Hurricane2 LPM implementation
 */

#include <bcm/types.h>
#include <soc/hurricane2.h>
#if defined(INCLUDE_L3)
#include <bcm_int/esw/hurricane2.h>
#include <shared/l3.h>

#define SOC_HU2_L3_VRF_DEFAULT   _SHR_L3_VRF_DEFAULT  /* Default vrf id.    */

/*
 * Function:	
 *     _bcm_hu2_mem_ip6_defip_set
 * Purpose:
 *    Set an IP6 address field in L3_DEFIPm
 * Parameters: 
 *    unit    - (IN) SOC unit number; 
 *    lpm_key - (OUT) Buffer to fill. 
 *    lpm_cfg - (IN) Route information. 
 * Note:
 *    See soc_mem_ip6_addr_set()
 */
STATIC void
_bcm_hu2_mem_ip6_defip_set(int unit, void *lpm_key, _bcm_defip_cfg_t *lpm_cfg)
{
    uint8 *ip6;                 /* Ip6 address.       */
    uint8 mask[BCM_IP6_ADDRLEN];        /* Subnet mask.       */
    uint32 ip6_word;            /* Temp storage.      */
    int idx;                    /* Iteration index .  */

    /* Just to keep variable name short. */
    ip6 = lpm_cfg->defip_ip6_addr;

    /* Create mask from prefix length. */
    bcm_ip6_mask_create(mask, lpm_cfg->defip_sub_len);

    /* Apply subnet mask */
    idx = lpm_cfg->defip_sub_len / 8;   /* Unchanged byte count.    */
    ip6[idx] &= mask[idx];      /* Apply mask on next byte. */
    for (idx++; idx < BCM_IP6_ADDRLEN; idx++) {
        ip6[idx] = 0;           /* Reset rest of bytes.     */
    }


    ip6_word = ((ip6[0] << 24) | (ip6[1] << 16) | (ip6[2] << 8) | (ip6[3]));
    soc_L3_DEFIPm_field_set(unit, lpm_key, IP_ADDR_Uf, (void *)&ip6_word);

    ip6_word = ((ip6[4] << 24) | (ip6[5] << 16) | (ip6[6] << 8) | (ip6[7]));
    soc_L3_DEFIPm_field_set(unit, lpm_key, IP_ADDR_Lf, (void *)&ip6_word);

    ip6_word = ((mask[0] << 24) | (mask[1] << 16) | (mask[2] << 8) | (mask[3]));
    soc_L3_DEFIPm_field_set(unit, lpm_key, IP_ADDR_U_MASKf,
                            (void *)&ip6_word);

    ip6_word = ((mask[4] << 24) | (mask[5] << 16) | (mask[6] << 8) | (mask[7]));
    soc_L3_DEFIPm_field_set(unit, lpm_key, IP_ADDR_L_MASKf,
                            (void *)&ip6_word);
}

/*
 * Function:
 *     _bcm_hu2_mem_ip6_defip_get
 * Purpose:
 *    Set an IP6 address field in L3_DEFIPm
 * Note:
 *    See soc_mem_ip6_addr_set()
 */
STATIC void
_bcm_hu2_mem_ip6_defip_get(int unit, const void *lpm_key,
                          _bcm_defip_cfg_t *lpm_cfg)
{
    uint8 *ip6;                 /* Ip6 address.       */
    uint8 mask[BCM_IP6_ADDRLEN];        /* Subnet mask.       */
    uint32 ip6_word;            /* Temp storage.      */

    sal_memset(mask, 0, sizeof (bcm_ip6_t));

    /* Just to keep variable name short. */
    ip6 = lpm_cfg->defip_ip6_addr;
    sal_memset(ip6, 0, sizeof (bcm_ip6_t));

    soc_L3_DEFIPm_field_get(unit, lpm_key, IP_ADDR_Uf, &ip6_word);
    ip6[0] = (uint8) ((ip6_word >> 24) & 0xff);
    ip6[1] = (uint8) ((ip6_word >> 16) & 0xff);
    ip6[2] = (uint8) ((ip6_word >> 8) & 0xff);
    ip6[3] = (uint8) (ip6_word & 0xff);

    soc_L3_DEFIPm_field_get(unit, lpm_key, IP_ADDR_Lf, &ip6_word);
    ip6[4] = (uint8) ((ip6_word >> 24) & 0xff);
    ip6[5] = (uint8) ((ip6_word >> 16) & 0xff);
    ip6[6] = (uint8) ((ip6_word >> 8) & 0xff);
    ip6[7] = (uint8) (ip6_word & 0xff);

    soc_L3_DEFIPm_field_get(unit, lpm_key, IP_ADDR_U_MASKf, &ip6_word);
    mask[0] = (uint8) ((ip6_word >> 24) & 0xff);
    mask[1] = (uint8) ((ip6_word >> 16) & 0xff);
    mask[2] = (uint8) ((ip6_word >> 8) & 0xff);
    mask[3] = (uint8) (ip6_word & 0xff);

    soc_L3_DEFIPm_field_get(unit, lpm_key, IP_ADDR_L_MASKf, &ip6_word);
    mask[4] = (uint8) ((ip6_word >> 24) & 0xff);
    mask[5] = (uint8) ((ip6_word >> 16) & 0xff);
    mask[6] = (uint8) ((ip6_word >> 8) & 0xff);
    mask[7] = (uint8) (ip6_word & 0xff);

    lpm_cfg->defip_sub_len = bcm_ip6_mask_length(mask);
}

/*
 * Function:
 *      _bcm_hu2_lpm_clear_hit
 * Purpose:
 *      Clear prefix entry hit bit.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      lpm_cfg   - (IN)Route info.  
 *      lpm_entry - (IN)Buffer to write to hw. 
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_hu2_lpm_clear_hit(int unit, _bcm_defip_cfg_t *lpm_cfg,
                      uint32 *lpm_entry)
{
    int rv;                        /* Operation return status. */
    int tbl_idx;                   /* Defip table index.       */
    soc_field_t hit_field = HITf; /* HIT bit field id.        */

    /* Input parameters check */
    if ((NULL == lpm_cfg) || (NULL == lpm_entry)) {
        return (BCM_E_PARAM);
    }

    /* If entry was not hit  clear "clear hit" and return */
    if (!(lpm_cfg->defip_flags & BCM_L3_HIT)) {
        return (BCM_E_NONE);
    }

    /* Reset entry hit bit in buffer. */
    tbl_idx = lpm_cfg->defip_index;
    soc_L3_DEFIPm_field32_set(unit, lpm_entry, hit_field, 0);

    /* Write lpm buffer to hardware. */
    rv = soc_mem_write(unit, L3_DEFIPm, MEM_BLOCK_ALL, tbl_idx, lpm_entry);
    return rv;
}

/*
 * Function:
 *      _bcm_hu2_lpm_ent_init
 * Purpose:
 *      Service routine used to initialize lkup key for lpm entry.
 * Parameters:
 *      unit      - (IN)SOC unit number.
 *      lpm_cfg   - (IN)Prefix info.
 *      lpm_entry - (OUT)Hw buffer to fill.
 * Returns:
 *      BCM_E_XXX
 */
STATIC int
_bcm_hu2_lpm_ent_init(int unit, _bcm_defip_cfg_t *lpm_cfg,
                     uint32 *lpm_entry)
{
    bcm_ip_t ip4_mask;

    int ipv6 = lpm_cfg->defip_flags & BCM_L3_IP6;

    /* Set prefix ip address & mask. */
    if (ipv6) {
        _bcm_hu2_mem_ip6_defip_set(unit, lpm_entry, lpm_cfg);
    } else {
        ip4_mask = BCM_IP4_MASKLEN_TO_ADDR(lpm_cfg->defip_sub_len);
        /* Apply subnet mask. */
        lpm_cfg->defip_ip_addr &= ip4_mask;

        /* Set address to the buffer. */
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, IP_ADDR_Lf,
                                  lpm_cfg->defip_ip_addr);
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, IP_ADDR_L_MASKf, ip4_mask);
        /* For IPv4 set the IP_ADDR_Uf to 0 */
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, IP_ADDR_Uf, 0);
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, IP_ADDR_U_MASKf, 0);
    }

    /* Set valid bit. */
    soc_L3_DEFIPm_field32_set(unit, lpm_entry, VALIDf, 1);
    soc_L3_DEFIPm_field32_set(unit, lpm_entry, MODEf, (ipv6) ? 1 : 0);

    if (soc_mem_field_valid(unit,L3_DEFIPm, MODE_MASKf)) {
        soc_mem_field32_set(unit,L3_DEFIPm, lpm_entry,
            MODE_MASKf, (1 << soc_mem_field_length(unit,
                        L3_DEFIPm, MODE_MASKf)) - 1);
    }

    if (soc_mem_field_valid(unit,L3_DEFIPm,
                            RESERVED_MASKf)) {
        soc_mem_field32_set(unit,L3_DEFIPm, 
                            lpm_entry, RESERVED_MASKf, 0);
    }

    if (SOC_MEM_FIELD_VALID(unit,L3_DEFIPm, GLOBAL_ROUTE0f)) {
        if (BCM_L3_VRF_GLOBAL == lpm_cfg->defip_vrf) {
            soc_mem_field32_set(unit,L3_DEFIPm,
                                lpm_entry, GLOBAL_ROUTE0f, 0x1);
        }
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_hu2_lpm_ent_get_key
 * Purpose:
 *      Parse entry key from DEFIP table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (OUT)Buffer to fill defip information. 
 *      lpm_entry   - (IN) Buffer read from hw. 
 * Returns:
 *      void
 */
STATIC void
_bcm_hu2_lpm_ent_get_key(int unit, _bcm_defip_cfg_t *lpm_cfg,
                        uint32 *lpm_entry)
{
    bcm_ip_t v4_mask;
    int ipv6 = lpm_cfg->defip_flags & BCM_L3_IP6;

    /* Set prefix ip address & mask. */
    if (ipv6) {
        _bcm_hu2_mem_ip6_defip_get(unit, lpm_entry, lpm_cfg);
    } else {
        /* Get ipv4 address. */
        lpm_cfg->defip_ip_addr =
            soc_L3_DEFIPm_field32_get(unit, lpm_entry, IP_ADDR_Lf);

        /* Get subnet mask. */
        v4_mask = soc_L3_DEFIPm_field32_get(unit, lpm_entry, IP_ADDR_L_MASKf);

        /* Fill mask length. */
        lpm_cfg->defip_sub_len = bcm_ip_mask_length(v4_mask);
    }

    /* No vrf support on this device. */
    lpm_cfg->defip_vrf = SOC_HU2_L3_VRF_DEFAULT;

    return;
}

/*
 * Function:
 *      _bcm_hu2_lpm_ent_parse
 * Purpose:
 *      Parse an entry from DEFIP table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (OUT)Buffer to fill defip information. 
 *      nh_ecmp_idx - (OUT)Next hop index or ecmp group id.  
 *      lpm_entry   - (IN) Buffer read from hw. 
 * Returns:
 *      void
 */
STATIC INLINE void
_bcm_hu2_lpm_ent_parse(int unit, _bcm_defip_cfg_t *lpm_cfg, int *nh_ecmp_idx,
                      uint32 *lpm_entry)
{
    /* Reset entry flags first. */
    lpm_cfg->defip_flags = 0;

    if (soc_L3_DEFIPm_field32_get(unit, lpm_entry, MODEf)) {
        lpm_cfg->defip_flags |= BCM_L3_IP6;
    }

    /* Check if entry points to ecmp group. */
    if (soc_L3_DEFIPm_field32_get(unit, lpm_entry, ECMP0f)) {
        /* Mark entry as ecmp */
        lpm_cfg->defip_ecmp = 1;
        lpm_cfg->defip_flags |= BCM_L3_MULTIPATH;

        /* Get ecmp group id. */
        if (nh_ecmp_idx) {
            *nh_ecmp_idx =
                soc_L3_DEFIPm_field32_get(unit, lpm_entry, ECMP_PTR0f);
        }
    } else {
        /* Mark entry as non-ecmp. */
        lpm_cfg->defip_ecmp = 0;

        /* Reset ecmp group next hop count. */
        lpm_cfg->defip_ecmp_count = 0;

        /* Get next hop index. */
        if (nh_ecmp_idx) {
            *nh_ecmp_idx =
                soc_L3_DEFIPm_field32_get(unit, lpm_entry, NEXT_HOP_INDEX0f);
        }
    }
    /* Get entry priority. */
    lpm_cfg->defip_prio = soc_L3_DEFIPm_field32_get(unit, lpm_entry, PRI0f);

    /* Get hit bit. */
    if (soc_L3_DEFIPm_field32_get(unit, lpm_entry, HITf)) {
        lpm_cfg->defip_flags |= BCM_L3_HIT;
    }

    /* Get priority override bit. */
    if (soc_L3_DEFIPm_field32_get(unit, lpm_entry, RPE0f)) {
        lpm_cfg->defip_flags |= BCM_L3_RPE;
    }

    /* Get destination discard flag. */
    if (SOC_MEM_FIELD_VALID(unit, L3_DEFIPm, DST_DISCARD0f)) {
        if(soc_L3_DEFIPm_field32_get(unit, lpm_entry, DST_DISCARD0f)) {
            lpm_cfg->defip_flags |= BCM_L3_DST_DISCARD;
        }
    }

    /* Get sourcediscard flag. */
    if (SOC_MEM_FIELD_VALID(unit, L3_DEFIPm, SRC_DISCARD0f)) {
        if(soc_L3_DEFIPm_field32_get(unit, lpm_entry, SRC_DISCARD0f)) {
            lpm_cfg->defip_flags |= BCM_L3_SRC_DISCARD;
        }
    }

    /* Set classification group id. */
    if (SOC_MEM_FIELD_VALID(unit, L3_DEFIPm, CLASS_ID0f)) {
        lpm_cfg->defip_lookup_class = 
            soc_L3_DEFIPm_field32_get(unit, lpm_entry, CLASS_ID0f);
    }

    return;
}

/*
 * Function:
 *      _bcm_hu2_lpm_get
 * Purpose:
 *      Get an entry from DEFIP table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 *      nh_ecmp_idx - (IN)Next hop or ecmp group index
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_hu2_lpm_get(int unit, _bcm_defip_cfg_t *lpm_cfg, int *nh_ecmp_idx)
{
    uint32 lpm_key[SOC_MAX_MEM_FIELD_WORDS];/* Route lookup key.        */
    uint32 lpm_entry[SOC_MAX_MEM_FIELD_WORDS];/* Search result buffer.    */
    int clear_hit;              /* Clear hit indicator.     */
    int rv;                     /* Operation return status. */

    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    /* Zero buffers. */
    sal_memset(lpm_entry, 0, WORDS2BYTES(SOC_MAX_MEM_FIELD_WORDS));
    sal_memset(lpm_key, 0, WORDS2BYTES(SOC_MAX_MEM_FIELD_WORDS));

    /* Check if clear hit bit required. */
    clear_hit = lpm_cfg->defip_flags & BCM_L3_HIT_CLEAR;

    /* Initialize lkup key. */
    BCM_IF_ERROR_RETURN(_bcm_hu2_lpm_ent_init(unit, lpm_cfg, lpm_key));

    /* Perform hw lookup. */
    rv = soc_hu2_lpm_match(unit, lpm_key, lpm_entry, &lpm_cfg->defip_index);
    BCM_IF_ERROR_RETURN(rv);

    /* Parse hw buffer to defip entry. */
    _bcm_hu2_lpm_ent_parse(unit, lpm_cfg, nh_ecmp_idx, lpm_entry);

    /* Clear the HIT bit */
    if (clear_hit) {
        BCM_IF_ERROR_RETURN(_bcm_hu2_lpm_clear_hit(unit, lpm_cfg, lpm_entry));
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_hu2_lpm_add
 * Purpose:
 *      Add an entry to DEFIP table.
 * Parameters:
 *      unit        - (IN)SOC unit number.
 *      lpm_cfg     - (IN)Buffer to fill defip information. 
 *      nh_ecmp_idx - (IN)Next hop or ecmp group index.
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_hu2_lpm_add(int unit, _bcm_defip_cfg_t *lpm_cfg, int nh_ecmp_idx)
{
    uint32 lpm_entry[SOC_MAX_MEM_FIELD_WORDS];/* Hw entry buffer.*/
    int rv;                                   /* Operation return status. */

    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    /* Zero buffers. */
    sal_memset(lpm_entry, 0, WORDS2BYTES(SOC_MAX_MEM_FIELD_WORDS));

    /* Set hit bit. */
    if (lpm_cfg->defip_flags & BCM_L3_HIT) {
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, HITf, 1);
    }

    /* Set priority override bit. */
    if (lpm_cfg->defip_flags & BCM_L3_RPE) {
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, RPE0f, 1);
    }

    /* Write priority field. */
    soc_L3_DEFIPm_field32_set(unit, lpm_entry, PRI0f, lpm_cfg->defip_prio);

    /* Fill next hop information. */
    if (lpm_cfg->defip_flags & BCM_L3_MULTIPATH) {
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, ECMP0f, 1);
        /* NOTE: Order is important set next hop index before ECMP_COUNT.
                * If device doesn't have ECMP_COUNT:  ECMP_PTR field is shorter than 
                * next hop -> we must ensure that write resets high bits of next hop
                * field.
                */
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, NEXT_HOP_INDEX0f, nh_ecmp_idx);
        if (SOC_MEM_FIELD_VALID(unit, L3_DEFIPm, ECMP_COUNT0f)) {
            soc_L3_DEFIPm_field32_set(unit, lpm_entry, ECMP_COUNT0f,
                                      lpm_cfg->defip_ecmp_count);
        }
    } else {
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, NEXT_HOP_INDEX0f,
                                  nh_ecmp_idx);
    }

    /* Set destination discard flag. */
    if (lpm_cfg->defip_flags & BCM_L3_DST_DISCARD) {
        if (SOC_MEM_FIELD_VALID(unit, L3_DEFIPm, DST_DISCARD0f)) {
            soc_L3_DEFIPm_field32_set(unit, lpm_entry, DST_DISCARD0f, 1);
        } else {
            return (BCM_E_UNAVAIL);
        }
    }

    /* Set source discard flag. */
    if (lpm_cfg->defip_flags & BCM_L3_SRC_DISCARD) {
        if (SOC_MEM_FIELD_VALID(unit, L3_DEFIPm, SRC_DISCARD0f)) {
            soc_L3_DEFIPm_field32_set(unit, lpm_entry, SRC_DISCARD0f, 1);
        } else {
            return (BCM_E_UNAVAIL);
        }
    }

    /* Set classification group id. */
    if (SOC_MEM_FIELD_VALID(unit, L3_DEFIPm, CLASS_ID0f)) {
        soc_L3_DEFIPm_field32_set(unit, lpm_entry, CLASS_ID0f, 
                                  lpm_cfg->defip_lookup_class);
    }

    /* Set Global route flag. */
    if (SOC_MEM_FIELD_VALID(unit, L3_DEFIPm, GLOBAL_ROUTE0f)) {
        if (BCM_L3_VRF_GLOBAL == lpm_cfg->defip_vrf) {
            soc_mem_field32_set(unit, L3_DEFIPm, lpm_entry, 
                                GLOBAL_ROUTE0f, 0x1);
        }
    }

    /* 
         * Initialize hw buffer from lpm configuration. 
         * NOTE: DON'T MOVE _bcm_hu2_defip_ent_init CALL UP, 
         * We want to copy flags & nh info, avoid  ipv6 mask & address corruption. 
         */
    BCM_IF_ERROR_RETURN(_bcm_hu2_lpm_ent_init(unit, lpm_cfg, lpm_entry));

    /* Write buffer to hw. */
    rv = soc_hu2_lpm_insert(unit, lpm_entry);

    /* If new route added increment total number of routes.  */
    /* Lack of index indicates a new route. */
    if ((rv >= 0) && (BCM_XGS3_L3_INVALID_INDEX == lpm_cfg->defip_index)) {
        BCM_XGS3_L3_DEFIP_CNT_INC(unit, (lpm_cfg->defip_flags & BCM_L3_IP6));
    }
    return rv;
}

/*
 * Function:
 *      _bcm_hu2_lpm_del
 * Purpose:
 *      Delete an entry from DEFIP table.
 * Parameters:
 *      unit    - (IN)SOC unit number.
 *      lpm_cfg - (IN)Buffer to fill defip information. 
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_hu2_lpm_del(int unit, _bcm_defip_cfg_t *lpm_cfg)
{
    int rv;                     /* Operation return status. */
    uint32 lpm_entry[SOC_MAX_MEM_FIELD_WORDS];/* Hw entry buffer.*/

    /* Input parameters check */
    if (NULL == lpm_cfg) {
        return (BCM_E_PARAM);
    }

    /* Zero buffers. */
    sal_memset(lpm_entry, 0, WORDS2BYTES(SOC_MAX_MEM_FIELD_WORDS));

    /* Initialize hw buffer deletion key. */
    BCM_IF_ERROR_RETURN(_bcm_hu2_lpm_ent_init(unit, lpm_cfg, lpm_entry));

    /* Write buffer to hw. */
    rv = soc_hu2_lpm_delete(unit, lpm_entry);

    /* If new route added increment total number of routes.  */
    if (rv >= 0) {
        BCM_XGS3_L3_DEFIP_CNT_DEC(unit, lpm_cfg->defip_flags & BCM_L3_IP6);
    }
    return rv;
}

/*
 * Function:
 *      _bcm_hu2_lpm_update_match
 * Purpose:
 *      Update/Delete all entries in defip table matching a certain rule.
 * Parameters:
 *      unit     - (IN)SOC unit number.
 *      trv_data - (IN)Delete pattern + compare,act,notify routines.  
 * Returns:
 *      BCM_E_XXX
 */
int
_bcm_hu2_lpm_update_match(int unit, _bcm_l3_trvrs_data_t *trv_data)
{
    uint32 ipv6;                /* Iterate over ipv6 only flag. */
    int idx;                    /* Iteration index.             */
    char *lpm_tbl_ptr;          /* Dma table pointer.           */
    int nh_ecmp_idx;            /* Next hop/Ecmp group index.   */
    int cmp_result;             /* Test routine result.         */
    uint32 *lpm_entry;          /* Hw entry buffer.             */
    _bcm_defip_cfg_t lpm_cfg;   /* Buffer to fill route info.   */
    int defip_table_size;       /* Defip table size.            */
    int rv;                     /* Operation return status.     */
    int idx_start = 0;
    int idx_end = 0;
    soc_mem_t mem = L3_DEFIPm;           /* Route table memory.        */ 

    ipv6 = (trv_data->flags & BCM_L3_IP6);

    /* Table DMA the LPM table to software copy */
    BCM_IF_ERROR_RETURN
        (bcm_xgs3_l3_tbl_dma(unit, mem, BCM_XGS3_L3_ENT_SZ(unit, defip),
                             "lpm_tbl", &lpm_tbl_ptr, &defip_table_size));

    idx_start = 0;
    idx_end = defip_table_size;

    for (idx = idx_start; idx < idx_end; idx++) {
        /* Calculate entry ofset. */
        lpm_entry =
            soc_mem_table_idx_to_pointer(unit, mem, uint32 *, lpm_tbl_ptr, idx);

        /* Each lpm entry contains either 1 ipv4 or 1 ipv6 entry
                * Make sure entry is valid. */
        if (!soc_L3_DEFIPm_field32_get(unit, lpm_entry, VALIDf)) {
            continue;
        }

        /* Zero destination buffer first. */
        sal_memset(&lpm_cfg, 0, sizeof(_bcm_defip_cfg_t));

        /* Parse  the entry. */
        _bcm_hu2_lpm_ent_parse(unit, &lpm_cfg, &nh_ecmp_idx, lpm_entry);
        lpm_cfg.defip_index = idx;
         
        /* If protocol doesn't match skip the entry. */
        if ((lpm_cfg.defip_flags & BCM_L3_IP6) != ipv6) {
            continue;
        }

        /* Fill entry ip address &  subnet mask. */
        _bcm_hu2_lpm_ent_get_key(unit, &lpm_cfg, lpm_entry);

        /* Execute operation routine if any. */
        if (trv_data->op_cb) {
            rv = (*trv_data->op_cb) (unit, (void *)trv_data,
                                     (void *)&lpm_cfg,
                                     (void *)&nh_ecmp_idx, &cmp_result);
            if (rv < 0) {
                soc_cm_sfree(unit, lpm_tbl_ptr);
                return rv;
            }
        }
#ifdef BCM_WARM_BOOT_SUPPORT
        if (SOC_WARM_BOOT(unit)) {
            rv = soc_hu2_lpm_reinit(unit, idx, lpm_entry);
            if (rv < 0) {
                soc_cm_sfree(unit, lpm_tbl_ptr);
                return rv;
            }
        }
#endif /* BCM_WARM_BOOT_SUPPORT */
        
    }

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_IF_ERROR_RETURN(soc_hu2_lpm_reinit_done(unit));
#endif /* BCM_WARM_BOOT_SUPPORT */
    soc_cm_sfree(unit, lpm_tbl_ptr);
    return (BCM_E_NONE);
}

#endif /* INCLUDE_L3 */

