/*
 * $Id: nat.c,v 1.33 Broadcom SDK $
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
 * NAT - Broadcom StrataSwitch NAT API.
 */

#include <sal/core/libc.h>
#include <soc/defs.h>
#if defined(INCLUDE_L3) 

#include <soc/drv.h>
#include <bcm/error.h>

#include <bcm/l3.h>
#include <bcm/nat.h>
#include <bcm/tunnel.h>
#include <bcm/debug.h>
#include <bcm/error.h>

#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/trident2.h>
#include <bcm_int/esw/flex_ctr.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/esw/nat.h>

#include <soc/mem.h>
#include <soc/profile_mem.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/trident2.h>

_bcm_l3_nat_state_t *_bcm_l3_nat_state[BCM_MAX_NUM_UNITS];

#ifdef BCM_TRIDENT2_SUPPORT
int 
_bcm_esw_l3_nat_lock(int unit)
{
    if (NULL != _bcm_l3_nat_state[unit]->lock) {
        return sal_mutex_take(_bcm_l3_nat_state[unit]->lock, 
                              sal_mutex_FOREVER);
    }
    return (BCM_E_NONE);
}

int 
_bcm_esw_l3_nat_unlock(int unit)
{
    if (NULL != _bcm_l3_nat_state[unit]->lock) {
        return sal_mutex_give(_bcm_l3_nat_state[unit]->lock);
    }
    return (BCM_E_NONE);
}
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
#ifdef BCM_WARM_BOOT_SUPPORT

STATIC int 
_bcm_td2_l3_nat_warmboot_tblcnt_update(int unit, int idx,
                                       bcm_l3_nat_ingress_t *nat_info,
                                       void *user_data)
{
    int tbl;
    soc_mem_t mem;
    bcm_l3_nat_ingress_cb_context_t *user_context;
    user_context = (bcm_l3_nat_ingress_cb_context_t *) user_data;
    mem = user_context->mem;

    switch (mem) {
      case L3_ENTRY_IPV4_MULTICASTm:
        tbl = BCM_L3_NAT_INGRESS_DNAT_CNT;
        break;
      case ING_SNATm:
        tbl = BCM_L3_NAT_INGRESS_SNAT_CNT;
        break;
      case ING_DNAT_ADDRESS_TYPEm:
        tbl = BCM_L3_NAT_INGRESS_POOL_CNT;
        break;
      default :
        tbl = BCM_L3_NAT_INGRESS_DNAT_CNT;
        break;
    }
    BCM_L3_NAT_INGRESS_INFO((unit)).tbl_cnts[(tbl)]++;

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_nat_reinit
 * Purpose:
 *      Warm boot recovery for the NAT module
 * Parameters:
 *      unit     - Device Number
 * Returns:
 *      BCM_E_XXX
 */
STATIC int 
_bcm_esw_nat_reinit(int unit)
{
    soc_mem_t mem;
    void *egr_nat_entry;
    ing_snat_entry_t snat_entry;
    l3_entry_ipv4_multicast_entry_t dnat_entry;
    bcm_l3_nat_ingress_cb_context_t user_context;
    int tbl, i, idx_min, idx_max, rv;
    egr_nat_packet_edit_info_entry_t pkt_edit_entry;
    uint32 key_type, valid0, valid1, flags, egr_nat_id;
    soc_field_t egr_nat_validf[2] = {VALIDf, VALID_0f};
    soc_mem_t egr_nat_tbl[2] = {ING_SNATm, L3_ENTRY_IPV4_MULTICASTm};
    soc_field_t egr_nat_idxf[2] = {NAT_PACKET_EDIT_IDXf, NAT__PACKET_EDIT_IDXf};

    /* Recover egress nat state */
    for (tbl = 0; tbl < 2; tbl++) {

        idx_min = soc_mem_index_min(unit, egr_nat_tbl[tbl]);
        idx_max = soc_mem_index_max(unit, egr_nat_tbl[tbl]);
        egr_nat_entry = (tbl) ? (void*) &dnat_entry : (void*) &snat_entry;

        for (i = idx_min; i < idx_max; i++) {
            SOC_IF_ERROR_RETURN(soc_mem_read(unit,
                egr_nat_tbl[tbl], MEM_BLOCK_ANY, i, egr_nat_entry));
            if (soc_mem_field32_get(unit, egr_nat_tbl[tbl],
                egr_nat_entry, egr_nat_validf[tbl]) == 0) {
                continue;
            }

            if (tbl) {
                key_type = soc_mem_field32_get(unit, egr_nat_tbl[tbl],
                           egr_nat_entry, KEY_TYPE_0f);
                if ((key_type != 18) && (key_type != 19)) {
                    continue; /* Not NAT entry */
                }
            }

            egr_nat_id = soc_mem_field32_get(unit, egr_nat_tbl[tbl],
                         egr_nat_entry, egr_nat_idxf[tbl]);
            BCM_L3_NAT_EGRESS_INC_REF_COUNT(unit, egr_nat_id, 1);
        }
    }

    mem = EGR_NAT_PACKET_EDIT_INFOm ;
    idx_min = soc_mem_index_min(unit, mem);
    idx_max = soc_mem_index_max(unit, mem);
    for (i = idx_min; i < idx_max; i++) {

        SOC_IF_ERROR_RETURN(soc_mem_read(unit, mem,
                            MEM_BLOCK_ANY, i, &pkt_edit_entry));

        valid0 = soc_mem_field32_get(unit, mem, &pkt_edit_entry, VALID_0f);
        valid1 = soc_mem_field32_get(unit, mem, &pkt_edit_entry, VALID_1f);

        if (!(valid0 || valid1)) {
            continue;
        }
        
        if (valid0) {
            SHR_BITSET(BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap,
            (i << 1));
        }

        if (valid1) {
            SHR_BITSET(BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap,
            ((i << 1) + 1));
        }
    }

    /* Recover ingress nat state */
    mem = L3_ENTRY_IPV4_MULTICASTm;
    flags = BCM_L3_NAT_INGRESS_DNAT;
    user_context.mem = mem;
    
    rv = bcm_esw_l3_nat_ingress_traverse(unit, flags, soc_mem_index_min(unit, mem),
                                     soc_mem_index_max(unit, mem), 
                                     _bcm_td2_l3_nat_warmboot_tblcnt_update,
                                     (void *) &user_context);
    if (BCM_FAILURE(rv)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
        return rv;
    }
    mem = ING_DNAT_ADDRESS_TYPEm;
    flags = BCM_L3_NAT_INGRESS_DNAT | BCM_L3_NAT_INGRESS_DNAT_POOL;
    user_context.mem = mem;
    
    rv = bcm_esw_l3_nat_ingress_traverse(unit, flags, soc_mem_index_min(unit, mem),
                                     soc_mem_index_max(unit, mem), 
                                     _bcm_td2_l3_nat_warmboot_tblcnt_update,
                                     (void *) &user_context);
    if (BCM_FAILURE(rv)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
        return rv;
    }
    mem = ING_SNATm;
    flags = 0;
    user_context.mem = mem;
    
    rv = bcm_esw_l3_nat_ingress_traverse(unit, flags, soc_mem_index_min(unit, mem),
                                     soc_mem_index_max(unit, mem), 
                                     _bcm_td2_l3_nat_warmboot_tblcnt_update,
                                     (void *) &user_context);
    if (BCM_FAILURE(rv)) {
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
        return rv;
    }

    idx_min = soc_mem_index_min(unit, ING_SNATm);
    idx_max = soc_mem_index_max(unit, ING_SNATm);
    for (i = idx_min; i <= idx_max; i++) {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit,
                            ING_SNATm, MEM_BLOCK_ANY, i, &snat_entry));
        if (soc_ING_SNATm_field32_get(unit, &snat_entry, VALIDf) == 0) {
            BCM_L3_NAT_INGRESS_SNAT_NAPT_FREE_IDX(unit) = i;
            break;
        }
    }

    for (i = idx_max; i >= idx_min; i--) {
        SOC_IF_ERROR_RETURN(soc_mem_read(unit,
                            ING_SNATm, MEM_BLOCK_ANY, i, &snat_entry));
        if (soc_ING_SNATm_field32_get(unit, &snat_entry, VALIDf) == 0) {
            BCM_L3_NAT_INGRESS_SNAT_NAT_FREE_IDX(unit) = i;
            break;
        }
    }

    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

int
_bcm_esw_l3_nat_init(int unit) 
{
    int rv = BCM_E_UNAVAIL;

    if (soc_feature(unit, soc_feature_nat)) {
        int mem_sz;

        /* allocate ingress and egress state */
        _bcm_l3_nat_state[unit] = sal_alloc(sizeof(_bcm_l3_nat_state_t), 
                                            "nat_state");

        if (NULL == _bcm_l3_nat_state[unit]) {
            return BCM_E_RESOURCE;
        } 
        sal_memset(_bcm_l3_nat_state[unit], 0, sizeof(_bcm_l3_nat_state_t));


        /* initialize nat_id bitmap, nat_id_refcount */
        mem_sz = SHR_BITALLOCSIZE(soc_mem_index_count(unit, 
                                             EGR_NAT_PACKET_EDIT_INFOm) * 2);
        BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap = sal_alloc(mem_sz,
                                                               "nat_id bitmap");
        if (NULL == BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap) {
            sal_free(_bcm_l3_nat_state[unit]);
            return BCM_E_RESOURCE;
        }
        sal_memset(BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap, 0, mem_sz);

        /* initialize tbl_cnts, free indexes */
        BCM_L3_NAT_INGRESS_SNAT_NAT_FREE_IDX(unit) = soc_mem_index_max(unit,
                                                                    ING_SNATm);
        /* allocate and initialize mutex */
        _bcm_l3_nat_state[unit]->lock = sal_mutex_create("NAT module mutex");
        if (NULL == _bcm_l3_nat_state[unit]->lock) {
            sal_free(BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap);
            sal_free(_bcm_l3_nat_state[unit]);
            return BCM_E_RESOURCE;
        }
        rv = BCM_E_NONE;

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        rv = _bcm_esw_nat_reinit(unit);
        if (BCM_FAILURE(rv)) {
            sal_free(BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap);
            sal_free(_bcm_l3_nat_state[unit]);
            return rv;
        }
    }
#endif /* BCM_WARM_BOOT_SUPPORT */

    }
    return rv;
}
#endif /* BCM_TRIDENT2_SUPPORT */

#ifdef BCM_TRIDENT2_SUPPORT
/* populate the entry using nat_info */
static int  _bcm_td2_nat_egress_sw_to_hw(int unit, int nat_idx, 
                                        bcm_l3_nat_egress_t *nat_info, 
                                        void *hw_buf)
{
    int rv, hw_idx, hw_half;
    int bit_count;
    int twice_nat = BCM_L3_NAT_EGRESS_FLAGS_FULL_NAT(nat_info->flags);
    int twice_nat_val = 0;
    soc_field_t modify_src_or_dst;
    soc_field_t ip_address;
    soc_field_t modify_l4_port_or_prefix;
    soc_field_t l4_port_or_prefix;
    soc_field_t valid;

    /* translate to hw index */
    BCM_L3_NAT_EGRESS_HW_IDX_GET(nat_idx, hw_idx, hw_half);
    
    if (hw_half) {
        modify_src_or_dst = MODIFY_SRC_OR_DST_1f;
        ip_address = IP_ADDRESS_1f;
        modify_l4_port_or_prefix = MODIFY_L4_PORT_OR_PREFIX_1f;
        l4_port_or_prefix = L4_PORT_OR_PREFIX_1f;
        valid = VALID_1f;
    } else {
        modify_src_or_dst = MODIFY_SRC_OR_DST_0f;
        ip_address = IP_ADDRESS_0f;
        modify_l4_port_or_prefix = MODIFY_L4_PORT_OR_PREFIX_0f;
        l4_port_or_prefix = L4_PORT_OR_PREFIX_0f;
        valid = VALID_0f;
    }
    if (nat_info->flags & BCM_L3_NAT_EGRESS_SNAT) {
        if (twice_nat) {
            /* override field selections */
            modify_src_or_dst = MODIFY_SRC_OR_DST_0f;
            ip_address = IP_ADDRESS_0f;
            modify_l4_port_or_prefix = MODIFY_L4_PORT_OR_PREFIX_0f;
            l4_port_or_prefix = L4_PORT_OR_PREFIX_0f;
            valid = VALID_0f;
            twice_nat_val = 1;
        } else {
            /* first read the entry */
            rv = READ_EGR_NAT_PACKET_EDIT_INFOm(unit, SOC_BLOCK_ANY, hw_idx, 
                                                hw_buf);
            SOC_IF_ERROR_RETURN(rv);
        }
        soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, 
                            modify_src_or_dst, 0); /* SIP */
        soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, ip_address, 
                                                  nat_info->sip_addr);
        if (nat_info->flags & BCM_L3_NAT_EGRESS_NAPT) {
            soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf,
                                            modify_l4_port_or_prefix, 0);
            soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, 
                                l4_port_or_prefix, nat_info->src_port);
        } else {
            soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf,
                                            modify_l4_port_or_prefix, 1);
            SHR_BITCOUNT_RANGE(&nat_info->sip_addr_mask, bit_count, 0, 32);
            soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf,
                                                l4_port_or_prefix, bit_count);
        }
        soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, valid, 1);

        /* protect against errors: */
        if (!twice_nat) {
            return BCM_E_NONE;
        }
    } 
    if (nat_info->flags & BCM_L3_NAT_EGRESS_DNAT) {
        if (twice_nat) {
            /* override field selections */
            modify_src_or_dst = MODIFY_SRC_OR_DST_1f;
            ip_address = IP_ADDRESS_1f;
            modify_l4_port_or_prefix = MODIFY_L4_PORT_OR_PREFIX_1f;
            l4_port_or_prefix = L4_PORT_OR_PREFIX_1f;
            valid = VALID_1f;
            twice_nat_val = 1;
        } else {
            /* first read the entry */
            rv = READ_EGR_NAT_PACKET_EDIT_INFOm(unit, SOC_BLOCK_ANY, hw_idx, 
                                                hw_buf);
            SOC_IF_ERROR_RETURN(rv);
        }
        soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, 
                            modify_src_or_dst, 1); /* DIP */
        soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, ip_address, 
                                                  nat_info->dip_addr); 
        if (nat_info->flags & BCM_L3_NAT_EGRESS_NAPT) {
            soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, 
                                            modify_l4_port_or_prefix, 0);
            soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, 
                                l4_port_or_prefix, nat_info->dst_port);
        } else {
            soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, 
                                            modify_l4_port_or_prefix, 1);
            SHR_BITCOUNT_RANGE(&nat_info->dip_addr_mask, bit_count, 0, 32);
            soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, 
                                    l4_port_or_prefix, bit_count);
        }
        soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, valid, 1);
    }
    soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, hw_buf, TWICE_NATf, 
                                              twice_nat_val);

    return BCM_E_NONE;
}

static int
_bcm_td2_nat_egress_hw_to_sw(int unit, void *hw_buf, 
                             bcm_l3_nat_egress_t *nat_info, int hw_half)
{
    int full_nat, valid0, valid1, tmp;
    tmp = nat_info->nat_id;
    sal_memset(nat_info, 0, sizeof(*nat_info));
    nat_info->nat_id = tmp;

    /* decide twice_nat or not first */
    full_nat = soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, hw_buf, 
                                                         TWICE_NATf);
    valid0 = soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, hw_buf, 
                                                         VALID_0f);
    valid1 = soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, hw_buf, 
                                                         VALID_1f);
    if (full_nat) {
        nat_info->flags |= (BCM_L3_NAT_EGRESS_SNAT | BCM_L3_NAT_EGRESS_DNAT);
        /* populate SNAT into entry 0 and DNAT into entry 1 */
        if (!valid0 || !valid1) {
            return BCM_E_EMPTY;
        }
        BCM_L3_NAT_FILL_SW_ENTRY_X(hw_buf, nat_info, 0f);
        BCM_L3_NAT_FILL_SW_ENTRY_X(hw_buf, nat_info, 1f);
    } else {
        /* figure out which half to parse */
        if (hw_half) {
            /* fill from ENTRY_1 */
            if (!valid1) return BCM_E_EMPTY;
            BCM_L3_NAT_FILL_SW_ENTRY_X(hw_buf, nat_info, 1f);
        } else {
            if (!valid0) return BCM_E_EMPTY;
            BCM_L3_NAT_FILL_SW_ENTRY_X(hw_buf, nat_info, 0f);
        }
    }
    return BCM_E_NONE;
}
#endif /* BCM_TRIDENT2_SUPPORT */    

/* allocate either a half or full-nat egress translate entry */
int
_bcm_td2_nat_egress_id_alloc(int unit, uint32 flags, int *id, int *count)
{
    int full_nat = BCM_L3_NAT_EGRESS_FLAGS_FULL_NAT(flags);
    int i = 0, num_ids, step_count, used;
    
    num_ids = soc_mem_index_count(unit, EGR_NAT_PACKET_EDIT_INFOm) * 
              BCM_L3_NAT_EGRESS_ENTRIES_PER_INDEX;
    step_count = full_nat ? 2 : 1;

    if (flags & BCM_L3_NAT_EGRESS_WITH_ID) {
        /* for full nat make sure proper id is used. Even for full nat, any
           for half nat */
        if (*id % step_count) {
            return BCM_E_PARAM;
        }
        /* make sure it is unused */
        SHR_BITTEST_RANGE(BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap, *id, 
                                                            step_count, used);
        if (flags & BCM_L3_NAT_EGRESS_REPLACE) {
            /* make sure it is used */
            if (1 == used) {       
                *count = step_count;
                return BCM_E_NONE;
            } else {
                return BCM_E_NOT_FOUND;
            }
        }
        /* make sure it is unused without BCM_L3_NAT_EGRESS_REPLACE */
        if (0 == used) {       
            SHR_BITSET_RANGE(BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap, *id, 
                             step_count);
            *count = step_count;
            return BCM_E_NONE;
        }
        /* if it is in use then flag error */
        return BCM_E_RESOURCE;
    } else if (flags & BCM_L3_NAT_EGRESS_REPLACE) {
        return BCM_E_PARAM;
    }

    for (i = 0; i < num_ids; i += step_count) {
        SHR_BITTEST_RANGE(BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap, i, 
                                                            step_count, used);
        if (0 == used) {       
            break;
        }
    }
    if (i == num_ids) {
        return BCM_E_RESOURCE;
    }
    SHR_BITSET_RANGE(BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap, i, step_count);
    *count = step_count;
    *id = i;
    return BCM_E_NONE;
}

int
_bcm_td2_nat_egress_id_free(int unit, int nat_id, int count)
{
    int used;
    SHR_BITTEST_RANGE(BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap, nat_id, count,
                      used); 
    if (!used) {
        return BCM_E_RESOURCE;
    }
    SHR_BITCLR_RANGE(BCM_L3_NAT_EGRESS_INFO(unit).nat_id_bitmap, nat_id, count);
    return BCM_E_NONE;
}

/*
 * Function:
 *  bcm_esw_l3_nat_egress_add
 * Description:
 *	Add a NAT translation entry to the egress NAT xlate table.
 * Parameters:
 *	unit - (IN) Device number. 
 *  nat_info - (IN/OUT)
 *      IN: packet edit info: original IP/ports and translated IP/ports
 *      OUT: nat_id field is populated with the index of the xlate table where
 *           this translation entry was added.
 * Returns:
 *	BCM_E_XXX
 */

int
bcm_esw_l3_nat_egress_add(int unit, bcm_l3_nat_egress_t *nat_info)
{
#ifdef BCM_TRIDENT2_SUPPORT
    int rv = BCM_E_NONE, nat_id, count, hw_idx, hw_half;
    egr_nat_packet_edit_info_entry_t  hw_buf;

    if (soc_feature(unit, soc_feature_nat)) {
       /* validate input */
        if (NULL == nat_info) {
            return BCM_E_PARAM;
        }
        if (!(nat_info->flags & (BCM_L3_NAT_EGRESS_SNAT | 
                                                    BCM_L3_NAT_EGRESS_DNAT))) {
            /* must specify at least one of SNAT and DNAT */
            return BCM_E_PARAM;
        }
        /* if WITH_ID is specified, attempt is made to allocate this one, if
           it is free
         */
        nat_id = nat_info->nat_id;

        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_lock(unit));

        /* allocate index: For twice nat nat_id and nat_id + 1 are allocated */
        rv = _bcm_td2_nat_egress_id_alloc(unit, nat_info->flags, &nat_id,
                                              &count);
        if (BCM_FAILURE(rv)) {
            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
            return BCM_E_RESOURCE;
        }

        /* populate the entry using nat_info */
        sal_memset(&hw_buf, 0, sizeof(hw_buf));
        rv = _bcm_td2_nat_egress_sw_to_hw(unit, nat_id, nat_info, (void *)&hw_buf);
        if (rv < 0) {
            _bcm_td2_nat_egress_id_free(unit, nat_id, count);
            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
            return BCM_E_PARAM;
        }

        BCM_L3_NAT_EGRESS_HW_IDX_GET(nat_id, hw_idx, hw_half);

        if (hw_half > 1) {
            return BCM_E_PARAM;
        }

        /* write the hardware entry */
        rv = WRITE_EGR_NAT_PACKET_EDIT_INFOm(unit, MEM_BLOCK_ALL, hw_idx, 
                                            (void *)&hw_buf); 
        if (rv < 0) {
            _bcm_td2_nat_egress_id_free(unit, nat_id, count);
            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
            return BCM_E_INTERNAL;
        }

        /* increment refcount */
        BCM_L3_NAT_EGRESS_INC_REF_COUNT(unit, nat_id, count);
        nat_info->nat_id = nat_id;
            
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
        return BCM_E_NONE;
    }
#endif /* BCM_TRIDENT2_SUPPORT */    
    return BCM_E_UNAVAIL;
}

/* Function:
 *  bcm_esw_l3_nat_delete
 * Description:
 *  Delete the egress NAT packet edit entry at the specified index. Decrements
 *  the reference count for the entry. When refcount falls to one it is 
 *  removed from the table.
 * Parameters:
 *  unit    - (IN) Device number.
 *  nat_id  - IN Index of entry to be deleted.
 */ 
int 
bcm_esw_l3_nat_egress_delete(
    int unit, 
    bcm_l3_nat_id_t nat_id)
{
#ifdef BCM_TRIDENT2_SUPPORT
    int rv = BCM_E_NONE, hw_idx, hw_half, full_nat=0;
    egr_nat_packet_edit_info_entry_t hw_buf;

    if (soc_feature(unit, soc_feature_nat)) {
        
        /* read entry at nat_id */
        BCM_L3_NAT_EGRESS_HW_IDX_GET(nat_id, hw_idx, hw_half);

        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_lock(unit));
        rv = READ_EGR_NAT_PACKET_EDIT_INFOm(unit, SOC_BLOCK_ANY, hw_idx, 
                                            (void *)&hw_buf);
        if (!SOC_SUCCESS(rv)) {
            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
            return BCM_E_INTERNAL; 
        }
		
        if (BCM_L3_NAT_EGRESS_GET_REF_COUNT(unit, nat_id) == 0) {
            /* already deleted */
            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
            return BCM_E_EMPTY;
        }
		
        /* check twice_nat field */
        
        full_nat = soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, (void *)&hw_buf, 
                                                                TWICE_NATf);

        /* Decrement refcount */
        BCM_L3_NAT_EGRESS_DEC_REF_COUNT(unit, nat_id);
        if (full_nat) {
            BCM_L3_NAT_EGRESS_DEC_REF_COUNT(unit, nat_id + 1);
        }

        if (BCM_L3_NAT_EGRESS_GET_REF_COUNT(unit, nat_id) > 0) {
            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
            return BCM_E_NONE;
        }
		
        /* clear valid bit for either 1 half or both halves if full nat */
        if (full_nat) {
            /* clear both halves */
            sal_memset(&hw_buf, 0, sizeof(hw_buf));
        } else {
            if (hw_half) {
                /* clear ENTRY_1 valid bit */
                soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, &hw_buf, 
                                                          VALID_1f, 0);
            } else {
                soc_EGR_NAT_PACKET_EDIT_INFOm_field32_set(unit, &hw_buf, 
                                                          VALID_0f, 0);
            }
        }
        rv = WRITE_EGR_NAT_PACKET_EDIT_INFOm(unit, SOC_BLOCK_ALL, hw_idx, 
                                            (void *) &hw_buf);
        if (!SOC_SUCCESS(rv)) {
            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
            return BCM_E_INTERNAL;
        }
        /* free nat_id and nat_id + 1 */ 
        rv = _bcm_td2_nat_egress_id_free(unit, nat_id, full_nat + 1);
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
        return rv;
    }
#endif /* BCM_TRIDENT2_SUPPORT */    
    return BCM_E_UNAVAIL;
}

/* Function:
 *  bcm_esw_l3_nat_egress_get
 * Description:
 *  Retrieve the egress NAT packet edit entry at the specified index 
 * Parameters:
 *  unit    - (IN) Device number.
 *  nat_id  - IN Index of entry to get.
 */
int bcm_esw_l3_nat_egress_get(
    int unit, 
    bcm_l3_nat_egress_t *nat_info)
{
#ifdef BCM_TRIDENT2_SUPPORT
    int rv = BCM_E_NONE, hw_idx, hw_half;
    egr_nat_packet_edit_info_entry_t hw_buf;

    if (soc_feature(unit, soc_feature_nat)) {
        if (NULL == nat_info) {
            return BCM_E_PARAM;
        }
        /* validate the index */
        BCM_L3_NAT_EGRESS_HW_IDX_GET(nat_info->nat_id, hw_idx, hw_half);
        if ((hw_idx < 0) || (hw_idx > soc_mem_index_max(unit, 
                            EGR_NAT_PACKET_EDIT_INFOm))) {
            return BCM_E_PARAM;
        }

        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_lock(unit));

        /* read entry at index nat_id. */
        rv = READ_EGR_NAT_PACKET_EDIT_INFOm(unit, SOC_BLOCK_ANY, hw_idx, 
                                            &hw_buf);
        if (!SOC_SUCCESS(rv)) {
            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
            return BCM_E_INTERNAL;
        }
        rv = _bcm_td2_nat_egress_hw_to_sw(unit, (void *)&hw_buf, nat_info, 
                                          hw_half);
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
        return rv;
    }
#endif /* BCM_TRIDENT2_SUPPORT */    
    return BCM_E_UNAVAIL;
}

/* Function:
 *  bcm_esw_l3_nat_egress_traverse
 * Description:
 *  Traverse all valid NAT packet edit entries in the egress translate table
 *  and call the supplied callback routine for each.
 * Parameters:
 *  unit    - (IN) Device number.
 *  nat_id  - IN Index of entry to get.
 */
int bcm_esw_l3_nat_egress_traverse(
    int unit,
    uint32 flags, 
    uint32 start, 
    uint32 end, 
    bcm_l3_nat_egress_traverse_cb cb, 
    void *user_data)
{
#ifdef BCM_TRIDENT2_SUPPORT

    int rv = BCM_E_NONE, i;
    int idx_max, full_nat;
    egr_nat_packet_edit_info_entry_t hw_buf;
    bcm_l3_nat_egress_t nat_info;

    if (!soc_feature(unit, soc_feature_nat)) {
        return BCM_E_UNAVAIL;
    }
    if (NULL == cb) {
        return BCM_E_NONE;
    }
    idx_max = soc_mem_index_max(unit, EGR_NAT_PACKET_EDIT_INFOm);
    /* Input indexes sanity */
    if ((start > (uint32)idx_max) || ((end > (uint32)idx_max) 
        && (end != 0xffffffff)) || (start > end)) {
        return BCM_E_PARAM;
    }

    /* End value according to real table size */
    if (end == 0xffffffff) {
        end = (uint32)idx_max;
    }

    BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_lock(unit));

    for (i = start; i <= end; i++) {
        /* traverse either one or 2 entries depending on half or full nat */
        rv = READ_EGR_NAT_PACKET_EDIT_INFOm(unit, SOC_BLOCK_ANY, i, 
                                            &hw_buf);
        if (!SOC_SUCCESS(rv)) {
            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
            return BCM_E_INTERNAL;
        }

        /* assume full nat entry first */
        nat_info.nat_id = BCM_L3_NAT_EGRESS_SW_IDX_GET(i, 0);
        rv = _bcm_td2_nat_egress_hw_to_sw(unit, (void *)&hw_buf, &nat_info, 
                                              0);
        if (rv != BCM_E_EMPTY) {
            /* call callback fn */
            rv = (*cb) (unit, i, &nat_info, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
            if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                break;
            }
#endif
        }

        /* confirm if full nat or not */
        full_nat = BCM_L3_NAT_EGRESS_FLAGS_FULL_NAT(nat_info.flags);

        if (full_nat) {
            continue;
        } 
        nat_info.nat_id = BCM_L3_NAT_EGRESS_SW_IDX_GET(i, 1);
        rv = _bcm_td2_nat_egress_hw_to_sw(unit, (void *)&hw_buf, &nat_info, 1);
        if (rv != BCM_E_EMPTY) {
            /* call callback fn */
            rv = cb(unit, i, &nat_info, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
            if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                break;
            }
#endif
        }

    }
    BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
    return BCM_E_NONE;
#endif /* BCM_TRIDENT2_SUPPORT */    
    return BCM_E_UNAVAIL;
}

/*
 * FLEX STAT FUNCTIONS for L3 NAT
 *
 * This following functions implements flex stat routines to attach, detach and get/set
 * flex counter functions.
 * 
 * FLEX ATTACH 
 *   The function allocates flex counter from the counter pool and configures
 *   attributes to set the base counter index and base offset. Flex counter pool,
 *   base index and offset is associated to the table index request for the
 *   module identifiers like, vpn, port, vp Additionally the reference counter
 *   for the flex counter is incremented for multple references of the counter.
 *
 * FLEX DETACH 
 *   The function disassociates flex counter details assigned during flex attach
 *   and decrements the reference counter.
 *
 * FLEX COUNTER GET/SET 
 *   The function read/set the associate counter values. Set is required for the
 *   purpose of clearing the counter.
 *
 *
 * FLEX COUNTER MULTI GET/SET
 *   The function reads/sets multiple flex stats and returns the counter arrays. 
 *
 * FLEX STAT ID
 *   The funciton retrieves the stat id for the given module identifiers. The
 *   flex counter stat id has the following format.
 *
 *          (8 modes)  (32 groups) (16 pools)  (32 objIds)  (64K index)
 *         +---------+------------+----------+-----------+-----------------+ 
 *         |         |            |          |           |                 | 
 *         | ModeId  |  GroupMode |   PoolId | Account   |  Base Index     |
 *         |         |            |          | objectId  |                 |
 *         +---------+------------+----------+-----------+-----------------+
 *        (31)    (29)(28)     (24)(23)   (20)(19)    (15)(14)            (0)
 *
 *
 *   PoolId      : Counter pool no. 
 *                 ( TD2: Ingress(8 pool x 4k counters)
 *                        Egress (4 pool x 4k counters)
 *   GroupMode   : Attribute group mode determines base offset and no. of counters
 *   ModeId      : Ingress/Egress Modes
 *   Base Idx    : Index into counter block of 256 counters
 *   Account Obj : Flexible accounting objects
 */


#if defined(BCM_TRIDENT2_SUPPORT)
/*
 * Function:
 *      _bcm_td2_l3_route_stat_attach
 * Description:
 *      Attach counters entries to the given  l3 route defip index . Allocate
 *      flex counter and assign counter index and offset for the index pointed
 *      by l3 defip index
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) L3 Route Info
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

/*
 * Function:
 *      _bcm_td2_l3_nat_egress_stat_get_table_info
 * Description:
 *      Provides relevant flex table information(table-name,index with
 *      direction)  for given ingress interface.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) Egress Nat Info
 *      num_of_tables    - (OUT) Number of flex counter tables
 *      table_info       - (OUT) Flex counter tables information
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
static 
bcm_error_t  _bcm_td2_l3_nat_egress_stat_get_table_info(
            int                        unit,
            bcm_l3_nat_egress_t	       *info,
            uint32                     *num_of_tables,
            bcm_stat_flex_table_info_t *table_info)
{
    int rv;
    int index = 0;

    if (!soc_feature(unit,soc_feature_advanced_flex_counter)) {
        return BCM_E_UNAVAIL;
    }

    /* Validate L3 EGRESS NAT Info */
    rv = bcm_esw_l3_nat_egress_get(unit, info);  
    if (!BCM_SUCCESS(rv)) {
        return rv;
    }
    index = info->nat_id;

    /* Ingress L3 Egress NAT */
    table_info[*num_of_tables].table=EGR_NAT_PACKET_EDIT_INFOm;
    table_info[*num_of_tables].index=index;
    table_info[*num_of_tables].direction=bcmStatFlexDirectionEgress;
    (*num_of_tables)++;

    return BCM_E_NONE;
}



/*
 * Function:
 *      _bcm_td2_l3_nat_egress_stat_attach
 * Description:
 *      Attach counters entries to the given  egress nat (egr nat packet edit
 *      info).  Allocate flex counter and assign counter index and offset for
 *      the index pointed by egr nat info ( packet edit index)
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) Egress Nat Info
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
    
int  _bcm_td2_l3_nat_egress_stat_attach (
                            int                 unit,
                            bcm_l3_nat_egress_t	*info,
                            uint32              stat_counter_id)
{
    int                        rv = BCM_E_NONE;
    int                        counter_flag = 0;
    soc_mem_t                  table = 0;
    bcm_stat_flex_direction_t  direction = bcmStatFlexDirectionIngress;
    uint32                     pool_number = 0;
    uint32                     base_index = 0;
    bcm_stat_flex_mode_t       offset_mode = 0;
    bcm_stat_object_t          object = bcmStatObjectIngPort;
    bcm_stat_group_mode_t      group_mode = bcmStatGroupModeSingle;
    uint32                     count = 0;
    uint32                     actual_num_tables = 0;
    uint32                     num_of_tables = 0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];


    /* Get pool, base index group mode and offset modes from stat counter id */
    _bcm_esw_stat_get_counter_id_info(
                  stat_counter_id,
                  &group_mode, &object, &offset_mode, &pool_number, &base_index);

    /* Validate object and group mode */
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_object(unit, object, &direction));
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_validate_group(unit, group_mode));

    /* Get Table index to attach flexible counter */
    BCM_IF_ERROR_RETURN(_bcm_esw_stat_flex_get_table_info(
                        unit, object, 1, &actual_num_tables, &table, &direction));
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_egress_stat_get_table_info(
                        unit, info, &num_of_tables, &table_info[0]));

    for (count = 0; count < num_of_tables; count++) {
         if ((table_info[count].direction == direction) &&
             (table_info[count].table == table) ) {
              if (direction == bcmStatFlexDirectionIngress) {
                  counter_flag = 1;
                  rv = _bcm_esw_stat_flex_attach_ingress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index,
                         offset_mode,
                         base_index,
                         pool_number);
                  if(BCM_FAILURE(rv)) {
                        break;
                  }
              } else {
                  counter_flag = 1;
                  rv = _bcm_esw_stat_flex_attach_egress_table_counters(
                         unit,
                         table_info[count].table,
                         table_info[count].index,
                         offset_mode,
                         base_index,
                         pool_number);

                if(BCM_FAILURE(rv)) {
                        break;
                  }
              } 
         }
    }

    if (counter_flag == 0) {
        rv = BCM_E_NOT_FOUND;
    }

    return rv;
}


/*
 * Function:
 *      _bcm_td2_l3_nat_egress_stat_detach
 * Description:
 *      Detach counter entries to the given  l3 egress nat index. 
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info		     - (IN) Egress Nat Info
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  _bcm_td2_l3_nat_egress_stat_detach (
                            int                 unit,
                            bcm_l3_nat_egress_t	*info)
{
    int                        counter_flag = 0;
    uint32                     count = 0;
    uint32                     num_of_tables = 0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    bcm_error_t                rv[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION] = 
                                  {BCM_E_NONE};

    /* Get the table index to be detached */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_egress_stat_get_table_info(
                        unit, info, &num_of_tables, &table_info[0]));

    for (count = 0; count < num_of_tables; count++) {
         if (table_info[count].direction == bcmStatFlexDirectionIngress) {
             counter_flag = 1;
             rv[bcmStatFlexDirectionIngress]=
                   _bcm_esw_stat_flex_detach_ingress_table_counters(
                        unit, table_info[count].table, table_info[count].index);
         } else {
             counter_flag = 1;
             rv[bcmStatFlexDirectionEgress] =
                   _bcm_esw_stat_flex_detach_egress_table_counters(
                        unit, table_info[count].table, table_info[count].index);
         }
    }

    if (counter_flag == 0) {
        return BCM_E_NOT_FOUND;
    }

    if (rv[bcmStatFlexDirectionIngress] != BCM_E_NONE) {
        return rv[bcmStatFlexDirectionIngress];
    }
    if (rv[bcmStatFlexDirectionEgress] != BCM_E_NONE) {
        return rv[bcmStatFlexDirectionEgress];
    }
    
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_nat_egress_stat_counter_get
 *
 * Description:
 *  Get l3 egress nat counter value for specified l3 egress nat index
 *  if sync_mode is set, sync the sw accumulated count
 *  with hw count value first, else return sw count.  
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) hwcount is to be synced to sw count 
 *      info			 - (IN)  Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  _bcm_td2_l3_nat_egress_stat_counter_get(
                            int                 unit, 
                            int                 sync_mode, 
                            bcm_l3_nat_egress_t	*info,
                            bcm_l3_nat_egress_stat_t    stat, 
                            uint32              num_entries, 
                            uint32              *counter_indexes, 
                            bcm_stat_value_t    *counter_values)
{
    uint32                     table_count = 0;
    uint32                     index_count = 0;
    uint32                     num_of_tables = 0;
    bcm_stat_flex_direction_t  direction = bcmStatFlexDirectionIngress;
    uint32                     byte_flag = 0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    /* Get the flex counter direction - ingress/egress */
    if ((stat == bcmL3NatOutPackets) ||
        (stat == bcmL3NatOutBytes)) {
         direction = bcmStatFlexDirectionEgress;
    } else {
        /* direction = bcmStatFlexDirectionIngress;*/
        return BCM_E_UNAVAIL;
    }
    if (stat == bcmL3NatOutPackets) {
        byte_flag=0;
    } else {
        byte_flag=1;
    }
   
    /*  Get Table index to read the flex counter */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_egress_stat_get_table_info(
                        unit, info, &num_of_tables, &table_info[0]));

    /* Get the flex counter for the attached table index */
    for (table_count = 0; table_count < num_of_tables ; table_count++) {
         if (table_info[table_count].direction == direction) {
             for (index_count = 0; index_count < num_entries ; index_count++) {
                  BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_get(
                                      unit, sync_mode,
                                      table_info[table_count].index,
                                      table_info[table_count].table,
                                      byte_flag,
                                      counter_indexes[index_count],
                                      &counter_values[index_count]));
             }
         }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_nat_egress_stat_counter_set
 *
 * Description:
 *  Set l3 egress nat counter value for specified l3 egress nat index
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
 int  _bcm_td2_l3_nat_egress_stat_counter_set(
                            int                 unit, 
                            bcm_l3_nat_egress_t	*info,
                            bcm_l3_nat_egress_stat_t  stat, 
                            uint32              num_entries, 
                            uint32              *counter_indexes, 
                            bcm_stat_value_t    *counter_values)
{
    uint32                     table_count=0;
    uint32                     index_count=0;
    uint32                     num_of_tables=0;
    bcm_stat_flex_direction_t  direction=bcmStatFlexDirectionIngress;
    uint32                     byte_flag=0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];

    /* Get the flex counter direction - ingress/egress */
    if ((stat == bcmL3NatOutPackets) ||
        (stat == bcmL3NatOutBytes)) {
         direction = bcmStatFlexDirectionEgress;
    } else {
        /* direction = bcmStatFlexDirectionIngress;*/
        return BCM_E_UNAVAIL;
    }
    if (stat == bcmL3NatOutPackets) {
        byte_flag=0;
    } else {
        byte_flag=1;
    }
   
    /*  Get Table index to read the flex counter */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_egress_stat_get_table_info(
                        unit, info, &num_of_tables, &table_info[0]));

    /* Get the flex counter for the attached table index */
    for (table_count = 0; table_count < num_of_tables ; table_count++) {
         if (table_info[table_count].direction == direction) {
             for (index_count = 0; index_count < num_entries; index_count++) {
                  BCM_IF_ERROR_RETURN(_bcm_esw_stat_counter_set(
                                      unit,
                                      table_info[table_count].index,
                                      table_info[table_count].table,
                                      byte_flag,
                                      counter_indexes[index_count],
                                      &counter_values[index_count]));
             }
         }
    }
    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_td2_l3_nat_egress_stat_multi_get
 *
 * Description:
 *  Get Multiple l3 egress nat counter value for specified l3 egress nat index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) Egress Nat Info
 *      nstat            - (IN) Number of elements in stat array
 *      stat_arr         - (IN) Collected statistics descriptors array
 *      value_arr        - (OUT) Collected counters values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

    
int  _bcm_td2_l3_nat_egress_stat_multi_get(
                            int                 unit, 
                            bcm_l3_nat_egress_t	*info,
                            int                 nstat, 
                            bcm_l3_nat_egress_stat_t  *stat_arr,
                            uint64              *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};
    int              sync_mode = 0;

    /* Iterate of all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat ;idx++) {
         BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_egress_stat_counter_get( 
                             unit, sync_mode, info, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
         if ((stat_arr[idx] == bcmL3NatOutPackets)) {
             COMPILER_64_SET(value_arr[idx],
                             COMPILER_64_HI(counter_values.packets64),
                             COMPILER_64_LO(counter_values.packets64));
         } else {
             COMPILER_64_SET(value_arr[idx],
                             COMPILER_64_HI(counter_values.bytes),
                             COMPILER_64_LO(counter_values.bytes));
         }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l3_nat_egress_stat_multi_get32
 *
 * Description:
 *  Get 32bit l3 egress nat counter value for specified l3 egress nat index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info		 	 - (IN) Egress Nat Info
 *      nstat            - (IN) Number of elements in stat array
 *      stat_arr         - (IN) Collected statistics descriptors array
 *      value_arr        - (OUT) Collected counters values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  _bcm_td2_l3_nat_egress_stat_multi_get32(
                            int                 unit, 
                            bcm_l3_nat_egress_t	*info,
                            int                 nstat, 
                            bcm_l3_nat_egress_stat_t  *stat_arr,
                            uint32              *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};
    int              sync_mode = 0;

    /* Iterate of all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat ;idx++) {
         BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_egress_stat_counter_get( 
                             unit, sync_mode, info, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
         if ((stat_arr[idx] == bcmL3NatOutPackets)) {
                value_arr[idx] = counter_values.packets;
         } else {
             value_arr[idx] = COMPILER_64_LO(counter_values.bytes);
         }
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l3_nat_egress_stat_multi_set
 *
 * Description:
 *  Set l3 egress nat counter value for specified l3 egress nat index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  _bcm_td2_l3_nat_egress_stat_multi_set(
                            int                 unit, 
                            bcm_l3_nat_egress_t	*info,
                            int                 nstat, 
                            bcm_l3_nat_egress_stat_t    *stat_arr,
                            uint64              *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};

    /* Iterate f all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat ;idx++) {
         if ((stat_arr[idx] == bcmL3NatOutPackets)) {
             counter_values.packets = COMPILER_64_LO(value_arr[idx]);
         } else {
             COMPILER_64_SET(counter_values.bytes,
                             COMPILER_64_HI(value_arr[idx]),
                             COMPILER_64_LO(value_arr[idx]));
         }
          BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_egress_stat_counter_set( 
                             unit, info, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
    }
    return rv;
}

/*
 * Function:
 *      _bcm_td2_l3_nat_egress_stat_multi_set32
 *
 * Description:
 *  Set 32bit l3 egress nat counter value for specified l3 egress nat index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

int  _bcm_td2_l3_nat_egress_stat_multi_set32(
                            int                 unit, 
                            bcm_l3_nat_egress_t	*info,
                            int                 nstat, 
                            bcm_l3_nat_egress_stat_t    *stat_arr,
                            uint32              *value_arr)
{
    int              rv = BCM_E_NONE;
    int              idx;
    uint32           counter_indexes = 0;
    bcm_stat_value_t counter_values = {0};

    /* Iterate of all stats array to retrieve flex counter values */
    for (idx = 0; idx < nstat ;idx++) {
         if ((stat_arr[idx] == bcmL3NatOutPackets)) {
             counter_values.packets = value_arr[idx];
         } else {
             COMPILER_64_SET(counter_values.bytes,0,value_arr[idx]);
         }

         BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_egress_stat_counter_set( 
                             unit, info, stat_arr[idx], 
                             1, &counter_indexes, &counter_values));
    }
    return rv;
}

/*
 * Function: 
 *      _bcm_td2_l3_nat_egress_stat_id_get
 *
 * Description: 
 *      Get Stat Counter if associated with given l3 egress nat index
 *
 * Parameters: 
 *      unit             - (IN) bcm device
 *      info			 - (IN) Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */

int  _bcm_td2_l3_nat_egress_stat_id_get(
                            int                 unit,
                            bcm_l3_nat_egress_t	*info,
                            bcm_l3_nat_egress_stat_t    stat, 
                            uint32              *stat_counter_id) 
{
    bcm_stat_flex_direction_t  direction = bcmStatFlexDirectionIngress;
    uint32                     num_of_tables = 0;
    bcm_stat_flex_table_info_t table_info[BCM_STAT_FLEX_COUNTER_MAX_DIRECTION];
    uint32                     index = 0;
    uint32                     num_stat_counter_ids = 0;

    if ((stat == bcmL3NatOutPackets) ||
        (stat == bcmL3NatOutBytes)) {
         direction = bcmStatFlexDirectionEgress;
    } else {
        /* direction = bcmStatFlexDirectionIngress;*/
        return BCM_E_UNAVAIL;
    }

    /*  Get Tables, for which flex counter are attached  */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_egress_stat_get_table_info(
                        unit, info, &num_of_tables, &table_info[0]));

    /* Retrieve stat counter id */
    for (index = 0; index < num_of_tables ; index++) {
         if (table_info[index].direction == direction)
             return _bcm_esw_stat_flex_get_counter_id(
                                  unit, 1, &table_info[index],
                                  &num_stat_counter_ids, stat_counter_id);
    }
    return BCM_E_NOT_FOUND;
}
#endif /* BCM_TRIDENT2_SUPPORT */
/*
 * Function:
 *      bcm_esw_l3_nat_egress_stat_attach
 * Description:
 *      Attach counters entries to the given  egress nat (egr nat packet edit
 *      info).  Allocate flex counter and assign counter index and offset for
 *      the index pointed by egr nat info ( packet edit index)
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) Egress Nat Info
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
    
int  bcm_esw_l3_nat_egress_stat_attach (
                            int                 unit,
                            bcm_l3_nat_egress_t	*info,
                            uint32              stat_counter_id)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        rv = _bcm_td2_l3_nat_egress_stat_attach (
                    unit, info, stat_counter_id);
    } 
#endif
    return rv;
}


/*
 * Function:
 *      bcm_esw_l3_nat_egress_stat_detach
 * Description:
 *      Detach counter entries to the given  l3 egress nat index. 
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info		     - (IN) Egress Nat Info
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  bcm_esw_l3_nat_egress_stat_detach (
                            int                 unit,
                            bcm_l3_nat_egress_t	*info)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        rv = _bcm_td2_l3_nat_egress_stat_detach (unit, info);
    } 
#endif
    return rv;
}

/*
 * Function:
 *      _bcm_esw_l3_nat_egress_stat_counter_get
 *
 * Description:
 *  Get l3 egress nat counter value for specified l3 egress nat index
 *  if sync_mode is set, sync the sw accumulated count
 *  with hw count value first, else return sw count.  
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      sync_mode        - (IN) hwcount is to be synced to sw count 
 *      info			 - (IN)  Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
 bcm_error_t  
 _bcm_esw_l3_nat_egress_stat_counter_get(int                 unit, 
                                         int                 sync_mode, 
                                         bcm_l3_nat_egress_t	*info,
                                         bcm_l3_nat_egress_stat_t    stat, 
                                         uint32              num_entries, 
                                         uint32              *counter_indexes, 
                                         bcm_stat_value_t    *counter_values)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        rv = _bcm_td2_l3_nat_egress_stat_counter_get(unit, sync_mode,
                                                     info, stat, num_entries, 
                                                     counter_indexes, 
                                                     counter_values);
    } 
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_nat_egress_stat_counter_get
 *
 * Description:
 *  Get l3 egress nat counter value for specified l3 egress nat index
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN)  Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
 int  
 bcm_esw_l3_nat_egress_stat_counter_get(int                 unit, 
                                        bcm_l3_nat_egress_t	*info,
                                        bcm_l3_nat_egress_stat_t    stat, 
                                        uint32              num_entries, 
                                        uint32              *counter_indexes, 
                                        bcm_stat_value_t    *counter_values)
{
    return _bcm_esw_l3_nat_egress_stat_counter_get(unit, 0, info, 
                                                   stat, num_entries, 
                                                   counter_indexes, 
                                                   counter_values);
}

/*
 * Function:
 *      bcm_esw_l3_nat_egress_stat_counter_sync_get
 *
 * Description:
 *  Get l3 egress nat counter value for specified l3 egress nat index
 *  sw accumulated counters synced with hw count.
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN)  Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
 int  
 bcm_esw_l3_nat_egress_stat_counter_sync_get(int                 unit, 
                                            bcm_l3_nat_egress_t	*info,
                                            bcm_l3_nat_egress_stat_t stat, 
                                            uint32           num_entries, 
                                            uint32           *counter_indexes, 
                                            bcm_stat_value_t *counter_values)
{
    return _bcm_esw_l3_nat_egress_stat_counter_get(unit, 1, info, 
                                                   stat, num_entries, 
                                                   counter_indexes, 
                                                   counter_values);
}

/*
 * Function:
 *      bcm_esw_l3_nat_egress_stat_counter_set
 *
 * Description:
 *  Set l3 egress nat counter value for specified l3 egress nat index
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  bcm_esw_l3_nat_egress_stat_counter_set(
                            int                 unit, 
                            bcm_l3_nat_egress_t	*info,
                            bcm_l3_nat_egress_stat_t  stat, 
                            uint32              num_entries, 
                            uint32              *counter_indexes, 
                            bcm_stat_value_t    *counter_values)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        rv = _bcm_td2_l3_nat_egress_stat_counter_set (
                    unit, info, stat, num_entries, 
                    counter_indexes, counter_values);
    } 
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_nat_egress_stat_multi_get
 *
 * Description:
 *  Get Multiple l3 egress nat counter value for specified l3 egress nat index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) Egress Nat Info
 *      nstat            - (IN) Number of elements in stat array
 *      stat_arr         - (IN) Collected statistics descriptors array
 *      value_arr        - (OUT) Collected counters values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

    
int  bcm_esw_l3_nat_egress_stat_multi_get(
                            int                 unit, 
                            bcm_l3_nat_egress_t	*info,
                            int                 nstat, 
                            bcm_l3_nat_egress_stat_t  *stat_arr,
                            uint64              *value_arr)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        rv = _bcm_td2_l3_nat_egress_stat_multi_get (
                    unit, info, nstat, stat_arr, value_arr);
    } 
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_nat_egress_stat_multi_get32
 *
 * Description:
 *  Get 32bit l3 egress nat counter value for specified l3 egress nat index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info		 	 - (IN) Egress Nat Info
 *      nstat            - (IN) Number of elements in stat array
 *      stat_arr         - (IN) Collected statistics descriptors array
 *      value_arr        - (OUT) Collected counters values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  bcm_esw_l3_nat_egress_stat_multi_get32(
                            int                 unit, 
                            bcm_l3_nat_egress_t	*info,
                            int                 nstat, 
                            bcm_l3_nat_egress_stat_t  *stat_arr,
                            uint32              *value_arr)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
       rv = _bcm_td2_l3_nat_egress_stat_multi_get32 (
                    unit, info, nstat, stat_arr, value_arr);
    } 
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_nat_egress_stat_multi_set
 *
 * Description:
 *  Set l3 egress nat counter value for specified l3 egress nat index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */
int  bcm_esw_l3_nat_egress_stat_multi_set(
                            int                 unit, 
                            bcm_l3_nat_egress_t	*info,
                            int                 nstat, 
                            bcm_l3_nat_egress_stat_t    *stat_arr,
                            uint64              *value_arr)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        rv = _bcm_td2_l3_nat_egress_stat_multi_set (
                    unit, info, nstat, stat_arr, value_arr);
    } 
#endif
    return rv;
}

/*
 * Function:
 *      bcm_esw_l3_nat_egress_stat_multi_set32
 *
 * Description:
 *  Set 32bit l3 egress nat counter value for specified l3 egress nat index for multiple stat
 *  types
 *
 * Parameters:
 *      unit             - (IN) unit number
 *      info			 - (IN) Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      num_entries      - (IN) Number of counter Entries
 *      counter_indexes  - (IN) Pointer to Counter indexes entries
 *      counter_values   - (OUT) Pointer to counter values
 *
 * Return Value:
 *      BCM_E_XXX
 * Notes:
 *
 */

int  bcm_esw_l3_nat_egress_stat_multi_set32(
                            int                 unit, 
                            bcm_l3_nat_egress_t	*info,
                            int                 nstat, 
                            bcm_l3_nat_egress_stat_t    *stat_arr,
                            uint32              *value_arr)
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        rv = _bcm_td2_l3_nat_egress_stat_multi_set32 (
                    unit, info, nstat, stat_arr, value_arr);
    } 
#endif
    return rv;
}

/*
 * Function: 
 *      bcm_esw_l3_nat_egress_stat_id_get
 *
 * Description: 
 *      Get Stat Counter if associated with given l3 egress nat index
 *
 * Parameters: 
 *      unit             - (IN) bcm device
 *      info			 - (IN) Egress Nat Info
 *      stat             - (IN) l3 egress nat counter stat types
 *      stat_counter_id  - (IN) Stat Counter ID.
 *
 * Returns:
 *      BCM_E_NONE - Success
 *      BCM_E_XXX
 * Notes:
 */

int  bcm_esw_l3_nat_egress_stat_id_get(
                            int                 unit,
                            bcm_l3_nat_egress_t	*info,
                            bcm_l3_nat_egress_stat_t    stat, 
                            uint32              *stat_counter_id) 
{
int     rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit,soc_feature_advanced_flex_counter)) {
        rv = _bcm_td2_l3_nat_egress_stat_id_get (
                    unit, info, stat, stat_counter_id);
    } 
#endif
    return rv;
}

#if defined(BCM_TRIDENT2_SUPPORT)
/* Convert from hw to sw representation of memory based on the type of ingress
 * NAT table 
 */
static int
_bcm_td2_l3_nat_ingress_hw_to_sw(int unit, soc_mem_t mem, void *key, 
                                 bcm_l3_nat_ingress_t *result)
{
    int keyt0, keyt1;

    if (NULL == result) {
        return BCM_E_PARAM;
    }
    sal_memset(result, 0, sizeof(*result));

    switch (mem) {
    case ING_DNAT_ADDRESS_TYPEm:
        if (!soc_ING_DNAT_ADDRESS_TYPEm_field32_get(unit, key, VALIDf)) {
            return BCM_E_EMPTY;
        }
        result->flags |= (BCM_L3_NAT_INGRESS_DNAT | BCM_L3_NAT_INGRESS_DNAT_POOL);
        result->ip_addr = 
            soc_ING_DNAT_ADDRESS_TYPEm_field32_get(unit, key, DEST_IPV4_ADDRf);
        result->vrf = 
            soc_ING_DNAT_ADDRESS_TYPEm_field32_get(unit, key, VRFf);
        if (soc_ING_DNAT_ADDRESS_TYPEm_field32_get(unit, key, POOL_ADDRESS_TYPEf))
        {
            result->flags |= BCM_L3_NAT_INGRESS_TYPE_NAPT;
        }
        break;

    case ING_SNATm:
        if (!soc_ING_SNATm_field32_get(unit, key, VALIDf)) {
            return BCM_E_EMPTY;
        }
        result->ip_addr =
                 soc_ING_SNATm_field32_get(unit, key, IP_ADDRESSf);
        result->vrf =
                 soc_ING_SNATm_field32_get(unit, key, VRFf);
        if (soc_mem_field32_get(unit, mem, key, NAT_TYPE_NAPTf)) {
            if (soc_ING_SNATm_field32_get(unit, key, L4_VALIDf)) {
                /* assume other fields are correctly masked */
                result->flags |= BCM_L3_NAT_INGRESS_TYPE_NAPT;
                result->l4_port = 
                    soc_ING_SNATm_field32_get(unit, key, L4_SRC_PORTf);
                result->ip_proto = 
                    soc_ING_SNATm_field32_get(unit, key, IP_PROTOf);
            } else {
                /* something wrong with the entry */
                return BCM_E_INTERNAL;
            }
        }
        result->nat_id = BCM_L3_NAT_EGRESS_SW_IDX_GET(
                        soc_ING_SNATm_field32_get(unit, key, NAT_PACKET_EDIT_IDXf),
                  soc_ING_SNATm_field32_get(unit, key, NAT_PACKET_EDIT_ENTRY_SELf));
        if (soc_ING_SNATm_field32_get(unit, key, HITf)) {
            result->flags |= BCM_L3_NAT_INGRESS_HIT;
        }
        break;

    case L3_ENTRY_IPV4_MULTICASTm:
        if (!(soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, VALID_0f) &&
            soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, VALID_1f))) {
            return BCM_E_EMPTY;
        }
        
        /* check key type also. Needs to be either NAT or NAPT */
        keyt0 = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key,KEY_TYPE_0f);
        keyt1 = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key,KEY_TYPE_1f);

        result->flags |= BCM_L3_NAT_INGRESS_DNAT;
        result->ip_addr =
                 soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, 
                                                                NAT__IP_ADDRf);
        result->vrf = 
                 soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, 
                                                                NAT__VRF_IDf);
        if (soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key,
                                                             NAT__L4_VALIDf)) {
            if ((keyt0 != TD2_L3_HASH_KEY_TYPE_DST_NAPT) ||
                (keyt1 != TD2_L3_HASH_KEY_TYPE_DST_NAPT)) {
                return BCM_E_EMPTY;
            }
            result->flags |= BCM_L3_NAT_INGRESS_TYPE_NAPT;
            
            result->l4_port = 
                 soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, 
                                                            NAT__L4_DEST_PORTf);
            result->ip_proto = 
                 soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, 
                                                                NAT__IP_PROTOf);
        } else {
            if ((keyt0 != TD2_L3_HASH_KEY_TYPE_DST_NAT) ||
                (keyt1 != TD2_L3_HASH_KEY_TYPE_DST_NAT)) {
                return BCM_E_EMPTY;
            }
        }
        if (soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, HIT_0f) &&
            soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, HIT_1f)) {
            result->flags |= BCM_L3_NAT_INGRESS_HIT;
        }

        if (soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, NAT__ECMPf)) {
            result->flags |= BCM_L3_NAT_INGRESS_MULTIPATH;
            result->nexthop = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, 
                                                           key, NAT__ECMP_PTRf);
        } else {
            result->nexthop = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, 
                                                    key, NAT__NEXT_HOP_INDEXf);
        }
        if (soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, 
                                                        NAT__DST_DISCARDf)) {
            result->flags |= BCM_L3_NAT_INGRESS_DST_DISCARD;
        }
        
        if (soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, NAT__RPEf)) {
            result->flags |= BCM_L3_NAT_INGRESS_RPE;
            result->pri =soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, 
                                                                    NAT__PRIf); 
        }
        result->class_id = soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, 
                                                               NAT__CLASS_IDf);
        result->nat_id = BCM_L3_NAT_EGRESS_SW_IDX_GET(
                soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, 
                                                        NAT__PACKET_EDIT_IDXf),
                soc_L3_ENTRY_IPV4_MULTICASTm_field32_get(unit, key, 
                                                NAT__PACKET_EDIT_ENTRY_SELf));
        break;

    default:
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}

static int
_bcm_td2_l3_nat_ingress_hit_update(int unit, soc_mem_t mem, int index, 
                                 bcm_l3_nat_ingress_t *result)
{
    uint32 hit;
    ing_snat_hit_only_x_entry_t snat_hit_x;
    ing_snat_hit_only_y_entry_t snat_hit_y;
    l3_entry_hit_only_x_entry_t l3_hit_x;
    l3_entry_hit_only_y_entry_t l3_hit_y;
    int idx, idx_max, idx_offset, hit_idx_shift;
    soc_field_t hitf[] = {HIT_0f, HIT_1f, HIT_2f, HIT_3f};

    switch (mem) {
    case ING_DNAT_ADDRESS_TYPEm:
        break;

    case ING_SNATm:
        BCM_IF_ERROR_RETURN(
            BCM_XGS3_MEM_READ(unit, ING_SNAT_HIT_ONLY_Xm,
                                      index, &snat_hit_x));
        BCM_IF_ERROR_RETURN(
            BCM_XGS3_MEM_READ(unit, ING_SNAT_HIT_ONLY_Ym,
                                      index, &snat_hit_y));
        if (soc_ING_SNAT_HIT_ONLY_Xm_field32_get(unit, &snat_hit_x, HITf) ||
            soc_ING_SNAT_HIT_ONLY_Ym_field32_get(unit, &snat_hit_y, HITf)) {
            result->flags |= BCM_L3_NAT_INGRESS_HIT;
        }
        break;

    case L3_ENTRY_IPV4_MULTICASTm:
        idx_max = 2;
        hit_idx_shift = 1;
        idx_offset = (index & 0x1) << 1;

        BCM_IF_ERROR_RETURN(
            BCM_XGS3_MEM_READ(unit, L3_ENTRY_HIT_ONLY_Xm,
                              (index >> hit_idx_shift), &l3_hit_x));
        BCM_IF_ERROR_RETURN(
            BCM_XGS3_MEM_READ(unit, L3_ENTRY_HIT_ONLY_Ym,
                              (index >> hit_idx_shift), &l3_hit_y));

        hit = 0;
        for (idx = idx_offset; idx < (idx_offset + idx_max); idx++) {
            hit |= soc_mem_field32_get(unit, L3_ENTRY_HIT_ONLY_Xm,
                                       &l3_hit_x, hitf[idx]);
        }
        
        for (idx = idx_offset; idx < (idx_offset + idx_max); idx++) {
            hit |= soc_mem_field32_get(unit, L3_ENTRY_HIT_ONLY_Ym,
                                       &l3_hit_y, hitf[idx]);
        }

        if (hit) {
            result->flags |= BCM_L3_NAT_INGRESS_HIT;
        }
        break;

    default:
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

static int
_bcm_td2_l3_nat_ingress_set_nexthop(int unit, soc_mem_t mem, void *hw_buf,
                                    bcm_l3_nat_ingress_t *nat_info) 
{
    int rv = BCM_E_NONE, nh_idx;
    uint32 mpath_flag = 0;

    /* figure out hw index from egress object */
    rv = bcm_xgs3_get_nh_from_egress_object(unit, nat_info->nexthop,
                                        &mpath_flag, 0, &nh_idx);
    
    if (BCM_FAILURE(rv)) {
        return rv;
    }
    /* make sure mpath setting is consistent with input */
    if (nat_info->flags & BCM_L3_NAT_INGRESS_MULTIPATH) {
        if (!mpath_flag) {
            return BCM_E_PARAM;
        }
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, NAT__ECMPf, 
                                                1);
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, 
                                                NAT__ECMP_PTRf, 
                                                nh_idx);
    } else {
        if (mpath_flag) {
            return BCM_E_PARAM;
        }
        if (nh_idx != BCM_XGS3_L3_INVALID_INDEX ) {
            soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, 
                                                     NAT__NEXT_HOP_INDEXf, 
                                                     nh_idx);
        } else {
            return BCM_E_PARAM; 
        }
    }
    return rv;
}

static int
_bcm_td2_l3_nat_ingress_sw_to_hw(int unit, soc_mem_t mem, void *hw_buf, 
                                 bcm_l3_nat_ingress_t *nat_info)
{
    /* Fill out key depending on the memory type */
    uint32 mask_len, mask; 
    int hw_idx, hw_half, key_type;
    
    if ((NULL == hw_buf) || (NULL == nat_info)) {
        return BCM_E_PARAM;
    }

    switch (mem) {
    case ING_DNAT_ADDRESS_TYPEm:
        sal_memset(hw_buf, 0, sizeof(ing_dnat_address_type_entry_t));
        soc_ING_DNAT_ADDRESS_TYPEm_field32_set(unit, hw_buf, DEST_IPV4_ADDRf, 
                                                nat_info->ip_addr);
        soc_ING_DNAT_ADDRESS_TYPEm_field32_set(unit, hw_buf, VRFf, 
                                                nat_info->vrf);
    
        /* data portion */
        soc_ING_DNAT_ADDRESS_TYPEm_field32_set(unit, hw_buf, POOL_ADDRESS_TYPEf,
                        nat_info->flags & BCM_L3_NAT_INGRESS_TYPE_NAPT ? 1 : 0);

        
        soc_ING_DNAT_ADDRESS_TYPEm_field32_set(unit, hw_buf, VALIDf, 1);
        break;
    case ING_SNATm:
        sal_memset(hw_buf, 0, sizeof(ing_snat_entry_t));
        soc_ING_SNATm_field32_set(unit, hw_buf, IP_ADDRESSf, nat_info->ip_addr);

        soc_mem_field32_set(unit, mem, hw_buf, IP_ADDRESS_MASKf, 
                                 0xffffffff);

        soc_ING_SNATm_field32_set(unit, hw_buf, VRFf, nat_info->vrf);
        mask_len = soc_mem_field_length(unit, ING_SNATm, VRFf);
        mask = (1 << mask_len) - 1;
        soc_mem_field32_set(unit, mem, hw_buf, VRF_MASKf, mask);

        if (nat_info->flags & BCM_L3_NAT_INGRESS_TYPE_NAPT) {
            soc_ING_SNATm_field32_set(unit, hw_buf, IP_PROTOf, 
                                      nat_info->ip_proto);
            mask_len = soc_mem_field_length(unit, ING_SNATm, IP_PROTOf);
            mask = (1 << mask_len) - 1;
            soc_mem_field32_set(unit, mem, hw_buf, IP_PROTO_MASKf, mask);

            soc_ING_SNATm_field32_set(unit, hw_buf, L4_SRC_PORTf,
                                      nat_info->l4_port);
            mask_len = soc_mem_field_length(unit, ING_SNATm, L4_SRC_PORTf);
            mask = (1 << mask_len) - 1;
            soc_mem_field32_set(unit, mem, hw_buf, L4_SRC_PORT_MASKf, 
                                     mask);

            soc_ING_SNATm_field32_set(unit, hw_buf, L4_VALIDf, 1);
            mask_len = soc_mem_field_length(unit, ING_SNATm, L4_VALIDf);
            mask = (1 << mask_len) - 1;
            soc_mem_field32_set(unit, mem, hw_buf, L4_VALID_MASKf, mask);
            
            /* data part */
            soc_ING_SNATm_field32_set(unit, hw_buf, NAT_TYPE_NAPTf, 1);
        }

        /* data portion */
        BCM_L3_NAT_EGRESS_HW_IDX_GET(nat_info->nat_id, hw_idx, hw_half);
        soc_ING_SNATm_field32_set(unit, hw_buf, NAT_PACKET_EDIT_ENTRY_SELf, 
                                  hw_half);
        soc_ING_SNATm_field32_set(unit, hw_buf, NAT_PACKET_EDIT_IDXf, hw_idx);
        soc_ING_SNATm_field32_set(unit, hw_buf, HITf, 
                         nat_info->flags & BCM_L3_NAT_INGRESS_HIT ? 1 : 0);
        
        soc_ING_SNATm_field32_set(unit, hw_buf, VALIDf, 1);
        
        break;
    case L3_ENTRY_IPV4_MULTICASTm:
        sal_memset(hw_buf, 0, sizeof(l3_entry_ipv4_multicast_entry_t));
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, NAT__IP_ADDRf, 
                                                 nat_info->ip_addr);
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, NAT__VRF_IDf, 
                                                 nat_info->vrf);
        key_type = TD2_L3_HASH_KEY_TYPE_DST_NAT;
        if (nat_info->flags & BCM_L3_NAT_INGRESS_TYPE_NAPT) {
            soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, 
                                                     NAT__IP_PROTOf, 
                                                     nat_info->ip_proto);
            soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, 
                                                     NAT__L4_DEST_PORTf, 
                                                     nat_info->l4_port);
            soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, 
                                                     NAT__L4_VALIDf, 1);
            key_type = TD2_L3_HASH_KEY_TYPE_DST_NAPT;
        }

        /* data portion */
        if (nat_info->flags & BCM_L3_NAT_INGRESS_DST_DISCARD) {
            soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, 
                                                NAT__DST_DISCARDf, 1);
        }
        
        
        if (nat_info->flags & BCM_L3_NAT_INGRESS_RPE) {
            soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, NAT__RPEf,1);
            soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, NAT__PRIf, 
                                                    nat_info->pri);
        }
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, NAT__CLASS_IDf, 
                                                            nat_info->class_id);
        BCM_L3_NAT_EGRESS_HW_IDX_GET(nat_info->nat_id, hw_idx, hw_half);
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, 
                                               NAT__PACKET_EDIT_ENTRY_SELf, 
                                               hw_half);
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, 
                                            NAT__PACKET_EDIT_IDXf, hw_idx);
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, KEY_TYPE_0f, 
                                                 key_type);
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, KEY_TYPE_1f, 
                                                 key_type);

        
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, VALID_0f, 1);
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, VALID_1f, 1);

        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, HIT_0f,
                          nat_info->flags & BCM_L3_NAT_INGRESS_HIT ? 1 : 0);
        soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, HIT_1f, 
                          nat_info->flags & BCM_L3_NAT_INGRESS_HIT ? 1 : 0);
        break;
    default:
        return BCM_E_PARAM;
    }
    return BCM_E_NONE;
}

/* DMA one of the memories specified by mem */
int
_bcm_td2_l3_nat_read_mem(int unit, soc_mem_t tbl_mem, uint16 tbl_entry_sz, 
                         char *descr, char **res_ptr, int *entry_count)
{
    int alloc_size;             
    char *buffer;              
    int tbl_size;             
    
    /* Input parameters sanity check. */
    if ((NULL == res_ptr) || (NULL == descr)) {
        return (BCM_E_PARAM);
    }

    /* If entry size is not deterministic. reject. */
#if 0
    if (tbl_entry_sz == BCM_XGS3_L3_INVALID_ENT_SZ) {
        return (BCM_E_UNAVAIL);
    }
#endif

    if (INVALIDm == tbl_mem) {
        return (BCM_E_NOT_FOUND);
    }

    /* Calculate table size. */
    tbl_size =  soc_mem_index_count(unit, tbl_mem);
    if (!tbl_size) {
        return (BCM_E_NOT_FOUND);
    } else if (NULL != entry_count) {
        *entry_count = tbl_size;
    }
    alloc_size = tbl_entry_sz * tbl_size;

    /* Allocate memory buffer. */
    buffer = soc_cm_salloc(unit, alloc_size, descr);
    if (buffer == NULL) {
        return (BCM_E_MEMORY);
    }

    /* Reset allocated buffer. */
    sal_memset(buffer, 0, alloc_size);

    /* Read table to the buffer. */
    if (soc_mem_read_range(unit, tbl_mem, MEM_BLOCK_ANY,
                           soc_mem_index_min(unit, tbl_mem),
                           soc_mem_index_max(unit, tbl_mem), buffer) < 0) {
        soc_cm_sfree(unit, buffer);
        return (BCM_E_INTERNAL);
    }

    *res_ptr = buffer;
    return (BCM_E_NONE);
}

static int
_bcm_td2_l3_nat_ing_snat_cmp(int unit, bcm_l3_nat_ingress_t *snat_entry, 
                             bcm_l3_nat_ingress_t *hw_snat_entry)
{
    /* filter out flags which are not relevant for match */
    if ((snat_entry->flags & ~(BCM_L3_NAT_INGRESS_HIT | BCM_L3_NAT_INGRESS_REPLACE))
       != (hw_snat_entry->flags & ~BCM_L3_NAT_INGRESS_HIT)) {
        return BCM_E_FAIL;
    }
    if (snat_entry->ip_addr != hw_snat_entry->ip_addr) {
        return BCM_E_FAIL;
    }
    if (snat_entry->vrf != hw_snat_entry->vrf) {
        return BCM_E_FAIL;
    }
    if (snat_entry->vrf != hw_snat_entry->vrf) {
        return BCM_E_FAIL;
    }
    if (!(snat_entry->flags & BCM_L3_NAT_INGRESS_TYPE_NAPT)) {
        return BCM_E_NONE;
    }
    if (snat_entry->ip_proto != hw_snat_entry->ip_proto) {
        return BCM_E_FAIL;
    }
    if (snat_entry->l4_port != hw_snat_entry->l4_port) {
        return BCM_E_FAIL;
    }
    return BCM_E_NONE;
}

static int
_bcm_td2_l3_nat_ing_snat_lookup(int unit, bcm_l3_nat_ingress_t *nat_info, 
                                void *key, int *index, void *result)
{
    ing_snat_entry_t *snat_entryp;
    bcm_l3_nat_ingress_t hw_nat_info;
    soc_mem_t   mem = ING_SNATm;
    char *tbl = NULL;
    int start_idx, end_idx, i, rv, tbl_count;

    snat_entryp = (ing_snat_entry_t *) key;
#if 0
    /* already done in caller */
    rv = _bcm_td2_l3_nat_ingress_sw_to_hw(unit, mem, &snat_entry, nat_info);
#endif

    /* dma memory */
    rv = _bcm_td2_l3_nat_read_mem(unit, mem, sizeof(*snat_entryp), "ing snat", 
                                  &tbl, &tbl_count);

    if (nat_info->flags & BCM_L3_NAT_INGRESS_TYPE_NAPT) {
        /* search NAPT part */
        start_idx = 0;
        end_idx = BCM_L3_NAT_INGRESS_SNAT_NAPT_FREE_IDX(unit);
    } else {
        /* search NAT part */
        start_idx = BCM_L3_NAT_INGRESS_SNAT_NAT_FREE_IDX(unit) + 1;
        end_idx = soc_mem_index_max(unit, mem) + 1;
    }  
    for (i = start_idx; i < end_idx; i++) {
        snat_entryp = soc_mem_table_idx_to_pointer(unit, mem, 
                                                   ing_snat_entry_t *, tbl, i);
        /* compare the dmaed entry with input key */
        rv = _bcm_td2_l3_nat_ingress_hw_to_sw(unit, mem, (void *) snat_entryp, 
                                              &hw_nat_info);
        if (!BCM_SUCCESS(rv)) {
            /* had trouble converting. Likely a mismatch anyway */
            continue;
        }
        rv = _bcm_td2_l3_nat_ing_snat_cmp(unit, nat_info, &hw_nat_info); 
        if (BCM_SUCCESS(rv)) {
            break;
        }
    }
    if (i == end_idx) {
        rv =  BCM_E_NOT_FOUND;
    } else {
        rv = BCM_E_NONE;
    }
    *index = i;
    /* copy entry into result pointer */
    if (BCM_SUCCESS(rv)) {
        sal_memcpy(result, snat_entryp, sizeof(*snat_entryp));
    }
    /* free dmaed table memory */
    soc_cm_sfree(unit, tbl);

    return rv;
}

static int
_bcm_td2_l3_nat_search_ing_mems(int unit, soc_mem_t mem, 
                                bcm_l3_nat_ingress_t *nat_info, 
                                int *index, void *key)
{
    int rv = 0;
    
    if (NULL == key) {
        return BCM_E_PARAM;
    }
    
    rv = _bcm_td2_l3_nat_ingress_sw_to_hw(unit, mem, key, nat_info);
    if (!BCM_SUCCESS(rv)) {
        return rv;
    }

    /* launch lookup */
    if (mem == ING_SNATm) {
        rv = _bcm_td2_l3_nat_ing_snat_lookup(unit, nat_info, key, index, key);
    } else {
        rv = soc_mem_search(unit, mem, MEM_BLOCK_ANY, index, key, key, 0);
        if (rv != SOC_E_NONE) {
            return BCM_E_NOT_FOUND;
        }
    }
    return rv;
}



/* helper function that deletes entries from the ingress SNAT/DNAT table. Just
   an intermediary function to all the delete api function */
int
_bcm_td2_l3_nat_ingress_delete_entry(int unit, int idx, 
                                          bcm_l3_nat_ingress_t *nat_info, 
                                          void *user_data)
{
    return bcm_esw_l3_nat_ingress_delete(unit, nat_info);
}

int
_bcm_td2_l3_nat_ingress_age_entry(int unit, int idx, 
                                  bcm_l3_nat_ingress_t *nat_info,
                                  void *user_data)
{
    /* Extract original user data */   
    bcm_l3_nat_ingress_cb_context_t *user_context;
    l3_entry_ipv4_multicast_entry_t dnat;
    ing_snat_entry_t   snat;
    void    *hw_buf;
    int rv = BCM_E_NONE;

    if ((NULL == nat_info) || (NULL == user_data)) {
        return BCM_E_PARAM;
    }

    user_context = (bcm_l3_nat_ingress_cb_context_t *) user_data;

    if (user_context->mem == ING_SNATm) {
        hw_buf = (void *) &snat;
    } else {
        hw_buf = (void *) &dnat;
    }
    
    /* if HIT bit set then clear it */
    if (nat_info->flags & BCM_L3_NAT_INGRESS_HIT) {
        nat_info->flags &= (~BCM_L3_NAT_INGRESS_HIT);
        rv = _bcm_td2_l3_nat_ingress_sw_to_hw(unit, user_context->mem, hw_buf, 
                                              nat_info);
        if (!BCM_SUCCESS(rv)) {
            return rv;
        }
        /* write ECMP or next hop */
        if (user_context->mem == L3_ENTRY_IPV4_MULTICASTm) {
            if (nat_info->flags & BCM_L3_NAT_INGRESS_MULTIPATH) {
                soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, NAT__ECMPf, 1);
                soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, NAT__ECMP_PTRf, 
                                                         nat_info->nexthop );
            } else {
                soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, hw_buf, NAT__NEXT_HOP_INDEXf,
                                                         nat_info->nexthop );
            }
        }
        /* write entry with cleared hit bit to hw */
        rv = soc_mem_write(unit, user_context->mem, COPYNO_ALL, idx, hw_buf);
    } else {
        /* hit bit already cleared. Delete it */
        rv =  bcm_esw_l3_nat_ingress_delete(unit, nat_info);
    } 

    /* call user callback */
    if ((BCM_SUCCESS(rv)) && (user_context->user_cb)) {
        rv = (*(user_context->user_cb))(unit, idx, nat_info, 
                                        user_context->user_data);
    }

    return rv; 
}

/* Just do plain addition. Replacement already done in caller. */
static int
_bcm_td2_l3_nat_ingress_snat_entry_add(int unit, soc_mem_t mem, 
                                       bcm_l3_nat_ingress_t *nat_info,
                                       ing_snat_entry_t *snat_entryp)
{
    int rv;
    int idx, new_idx, *idx_ptr;

    /* NAPT entries occupy upper side of table, NAT entries occupy lower side
       to ensure NAPT gets higher priority match */
    /* New entry. Not duplicate. */
    /* check if space is available */
    if (BCM_L3_NAT_INGRESS_SNAT_NAPT_FREE_IDX(unit) > 
        BCM_L3_NAT_INGRESS_SNAT_NAT_FREE_IDX(unit))  {
        return BCM_E_FULL;
    }

    if (nat_info->flags & BCM_L3_NAT_INGRESS_TYPE_NAPT) {
        idx = BCM_L3_NAT_INGRESS_SNAT_NAPT_FREE_IDX(unit);
        idx_ptr = &BCM_L3_NAT_INGRESS_SNAT_NAPT_FREE_IDX(unit);
        new_idx = idx + 1;
    } else {
        idx = BCM_L3_NAT_INGRESS_SNAT_NAT_FREE_IDX(unit);
        idx_ptr = &BCM_L3_NAT_INGRESS_SNAT_NAT_FREE_IDX(unit);
        new_idx = idx - 1;
    }
    rv = soc_mem_write(unit, mem, COPYNO_ALL, idx, snat_entryp);
    if (SOC_SUCCESS(rv)) {
        *idx_ptr = new_idx;
        return BCM_E_NONE;
    }
    return BCM_E_FAIL;
}
#endif /* BCM_TRIDENT2_SUPPORT */

/* 
 * Function:
 *      bcm_esw_l3_nat_ingress_add
 *
 * Description:
 *      Add an ingress NAT table entry. Depending on the flags settings an
 *      entry is added to either the ingress DNAT pool table, DNAT session
 *      table or SNAT session table.
 *
 * Parameters:
 *      unit        - (IN) bcm device
 *      nat_info    - (IN) ingress nat_info
 */
int 
bcm_esw_l3_nat_ingress_add(int unit, bcm_l3_nat_ingress_t *nat_info)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    soc_mem_t mem;
    bcm_l3_nat_egress_t tmp_nat;
    bcm_l3_nat_ingress_t tmp_ing_nat;
    ing_dnat_address_type_entry_t pool;
    ing_snat_entry_t snat;
    l3_entry_ipv4_multicast_entry_t dnat;
    int mem_counter;
    int index;
    int max_int_pri = 15;
    void *result;

    if (soc_feature(unit, soc_feature_nat)) {
        if (NULL == nat_info) {
            return BCM_E_PARAM;
        }

        /* figure out which mem to operate on */
        BCM_L3_NAT_INGRESS_GET_MEM_POINTER(unit, nat_info, mem, pool, dnat, 
                                           snat, result, mem_counter);

        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_lock(unit));

        /* validate info in nat_info structure: */
        if (mem != ING_DNAT_ADDRESS_TYPEm) {
            if ((mem == L3_ENTRY_IPV4_MULTICASTm) && 
                (nat_info->flags & BCM_L3_NAT_INGRESS_RPE)) {         
                if ((nat_info->pri < BCM_PRIO_MIN) || 
                    (nat_info->pri > max_int_pri)) {
                    return BCM_E_PARAM;
                }
            }
            /* DNAT and SNAT entries should have valid pkt edit idx */
            /* check if provided nat_id is valid */
            tmp_nat.nat_id = nat_info->nat_id;
            rv = bcm_esw_l3_nat_egress_get(unit, &tmp_nat);
            
            
            if (!BCM_SUCCESS(rv)) {
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return rv;
            }
        }
        /* check if entry already exists in the specified table */
        rv = _bcm_td2_l3_nat_search_ing_mems(unit, mem, nat_info, &index, result);

        /* if found, if replace flag set then replace entry. Otherwise declare error
         */
        if (rv == BCM_E_NONE) {
            if (nat_info->flags & BCM_L3_NAT_INGRESS_REPLACE) {
                /* if nat_id is different, then delete original nat_id */
                if (mem != ING_DNAT_ADDRESS_TYPEm) {
                    /* convert result to sw format to get nat_id */
                    rv = _bcm_td2_l3_nat_ingress_hw_to_sw(unit, mem, result, 
                                                          &tmp_ing_nat);
                    if (!BCM_SUCCESS(rv)) {
                        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                        return BCM_E_INTERNAL;
                    }
                    if (tmp_ing_nat.nat_id != nat_info->nat_id) {
                        /* delete the original nat id */
                        rv = bcm_esw_l3_nat_egress_delete(unit, 
                                                            tmp_ing_nat.nat_id);
                        if (!BCM_SUCCESS(rv)) {
                            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                            return BCM_E_INTERNAL;
                        }
                    }
                }
                if (mem == L3_ENTRY_IPV4_MULTICASTm) {
                    /* set nexthop correctly */
                    rv = _bcm_td2_l3_nat_ingress_set_nexthop(unit, mem, result,
                                    nat_info) ;
                    if (!BCM_SUCCESS(rv)) {
                        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                        return rv;
                    }
                }
                /* overwrite entry with this one */
                rv = soc_mem_write(unit, mem, COPYNO_ALL, index, result);
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return rv;
            } else {
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return BCM_E_EXISTS;
            }
        }
        
        /* new entry. Insert into relevant table */
        if (mem != ING_SNATm) {
            if (mem == L3_ENTRY_IPV4_MULTICASTm) {
                /* set nexthop correctly */
                rv = _bcm_td2_l3_nat_ingress_set_nexthop(unit, mem, result,
                                                         nat_info) ;
                if (!BCM_SUCCESS(rv)) {
                    BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                    return rv;
                }
            }
            rv = soc_mem_insert(unit, mem, COPYNO_ALL, result);
            if (rv == SOC_E_FULL) {
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return BCM_E_FULL;
            }
            if (rv != SOC_E_NONE) {
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return BCM_E_INTERNAL;
            }
        } else {
            rv = _bcm_td2_l3_nat_ingress_snat_entry_add(unit, mem, nat_info, 
                                                   (ing_snat_entry_t *)result);
            if (!BCM_SUCCESS(rv)) {
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return rv;
            }
        }
        /* increment table count */
        BCM_L3_NAT_INGRESS_INC_TBL_CNT(unit, mem_counter);
        rv = BCM_E_NONE;
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
    }
#endif
    return rv;
}

#ifdef BCM_TRIDENT2_SUPPORT
static int
_bcm_td2_l3_nat_ingress_snat_entry_delete(int unit, soc_mem_t mem, 
                                          bcm_l3_nat_ingress_t *nat_info, 
                                          void *key)
{
    int index = -1, rv;
    int last_idx, *idx_ptr;
    ing_snat_entry_t last_snat;

    /* lookup entry. */
    rv = _bcm_td2_l3_nat_ing_snat_lookup(unit, nat_info, key, &index, key);

    if (!BCM_SUCCESS(rv)) {
        return rv;
    }
    
    /* copy last valid entry into this entry index */
    if (nat_info->flags & BCM_L3_NAT_INGRESS_TYPE_NAPT) {
        /* get last napt entry index */
        last_idx = BCM_L3_NAT_INGRESS_SNAT_NAPT_FREE_IDX(unit) - 1;
        idx_ptr = &BCM_L3_NAT_INGRESS_SNAT_NAPT_FREE_IDX(unit);
    } else {
        last_idx = BCM_L3_NAT_INGRESS_SNAT_NAT_FREE_IDX(unit) + 1;
        idx_ptr = &BCM_L3_NAT_INGRESS_SNAT_NAT_FREE_IDX(unit);
    }

    /* read last entry */
    rv = soc_mem_read(unit, mem, MEM_BLOCK_ANY, last_idx, &last_snat);

    if (!SOC_SUCCESS(rv)) {
        return BCM_E_INTERNAL;
    }

    /* write it into the entry to be deleted */
    rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, index, &last_snat);
    if (!SOC_SUCCESS(rv)) {
        return BCM_E_INTERNAL;
    }

    /* delete last entry */
    sal_memset(&last_snat, 0, sizeof(last_snat));
    rv = soc_mem_write(unit, mem, MEM_BLOCK_ALL, last_idx, &last_snat);
    if (!SOC_SUCCESS(rv)) {
        return BCM_E_INTERNAL;
    }

    /* update free index */
    *idx_ptr = last_idx;
    return BCM_E_NONE;
}
#endif /* BCM_TRIDENT2_SUPPORT */

/* 
 * Function:
 *      bcm_esw_l3_nat_ingress_delete
 *
 * Description:
 *      Delete an ingress NAT table entry. Depending on the flags settings an
 *      entry is deleted from  either the ingress DNAT pool table, DNAT session
 *      table or SNAT session table.
 *
 * Parameters:
 *      unit        - (IN) bcm device
 *      nat_info    - (IN) ingress nat_info
 */
int 
bcm_esw_l3_nat_ingress_delete(
    int unit, 
    bcm_l3_nat_ingress_t *nat_info)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    soc_mem_t mem;
    ing_dnat_address_type_entry_t pool;
    ing_snat_entry_t snat;
    l3_entry_ipv4_multicast_entry_t dnat;
    void *result;
    int mem_counter;

    if (soc_feature(unit, soc_feature_nat)) {
        /* validate input */
        if (NULL == nat_info) {
            return BCM_E_PARAM;
        }

        BCM_L3_NAT_INGRESS_GET_MEM_POINTER(unit, nat_info, mem, pool, dnat, 
                                           snat, result, mem_counter);
        BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_ingress_sw_to_hw(unit, mem, result, 
                            nat_info));
        /* Clean PRIf to avoid illegal pri */
        if ((mem ==  L3_ENTRY_IPV4_MULTICASTm) && 
            (nat_info->flags & BCM_L3_NAT_INGRESS_RPE)) {
            soc_L3_ENTRY_IPV4_MULTICASTm_field32_set(unit, result, 
                                                     NAT__PRIf, 0);
        }

        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_lock(unit));

        if (mem != ING_SNATm) {
            /* delete entry from table */
            rv = soc_mem_delete(unit, mem, COPYNO_ALL, result);
            if (SOC_FAILURE(rv)) {
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return rv;
            }
        } else {
            rv = _bcm_td2_l3_nat_ingress_snat_entry_delete(unit, mem, nat_info,
                                                           result);
        }

        if (mem != ING_DNAT_ADDRESS_TYPEm) {
            /* decrement refcount for the egress nat_id */
            rv = bcm_esw_l3_nat_egress_delete(unit, nat_info->nat_id);

            if (rv != BCM_E_NONE) {
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return rv;
            }
        }
        /* decrement table count */
        BCM_L3_NAT_INGRESS_DEC_TBL_CNT(unit, mem_counter);
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
        return BCM_E_NONE;
    }
#endif /* BCM_TRIDENT_SUPPORT */
    return rv;
}

/* 
 * Function:
 *      bcm_esw_l3_nat_ingress_traverse
 *
 * Description:
 *      Traverse through the ingress NAT table specified by flags and invoke a
 *      user provided callback for each valid entry, that matches the flags.
 *
 * Parameters:
 *      unit        - (IN) bcm device
 *      flags       - (IN) Flags to match during traverse
 *      start       - (IN) First index to read
 *      end         - (IN) Last index to read
 *      cb          - (IN) Caller notification callback
 *      user_data   - (IN) User cookie which should be returned in cb
 */
int bcm_esw_l3_nat_ingress_traverse(int unit, uint32 flags, uint32 start, 
                                    uint32 end, bcm_l3_nat_ingress_traverse_cb cb, 
                                    void *user_data)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    soc_mem_t mem;
    bcm_l3_nat_ingress_t tmp_nat;
    ing_dnat_address_type_entry_t *pool;
    ing_snat_entry_t *snat;
    l3_entry_ipv4_multicast_entry_t *dnat;
    int pool_sz = sizeof(ing_dnat_address_type_entry_t);
    int dnat_sz = sizeof(l3_entry_ipv4_multicast_entry_t);
    int snat_sz = sizeof(ing_snat_entry_t);
    int *result, idx, tbl_size;
    char *buffer = NULL;
    void *entry_ptr;
    uint32 tmp_flags = BCM_L3_NAT_INGRESS_HIT
           | BCM_L3_NAT_INGRESS_MULTIPATH | BCM_L3_NAT_INGRESS_RPE
           | BCM_L3_NAT_INGRESS_DST_DISCARD | BCM_L3_NAT_INGRESS_REPLACE;   

    if (soc_feature(unit, soc_feature_nat)) {
        /* pick memory */
        /* make macro-compatible: */
        tmp_nat.flags = flags;
        BCM_L3_NAT_INGRESS_GET_MEM_ONLY(unit, &tmp_nat, mem, pool_sz, dnat_sz, 
                                        snat_sz, result);

        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_lock(unit));

        /* dma memory */
        rv = _bcm_td2_l3_nat_read_mem(unit, mem, *result, "l3 nat ingress", &buffer,
                                      &tbl_size);
        if (!BCM_SUCCESS(rv)) {
            /* free memory */
            if (buffer != NULL) {
                soc_cm_sfree(unit, buffer);
            }
            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
            return rv;
        }

        /* Input indexes sanity */
        if (start >= (uint32)tbl_size || (end >= (uint32)tbl_size 
            && end != 0xffffffff) || start > end) {
            BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
            return (BCM_E_NOT_FOUND);
        }
        /* End value according to real table size */
        if (end == 0xffffffff) {
            end = (uint32)tbl_size - 1;
        }
        
        /* walk from start to end, matching flags */
        for (idx = start; idx <= end; idx++) {
            /* read entry at index */
            switch (mem) {
            case ING_DNAT_ADDRESS_TYPEm:
                pool = soc_mem_table_idx_to_pointer(unit, mem, 
                                     ing_dnat_address_type_entry_t *, buffer, idx);
                entry_ptr = (void *) pool;
                break;
            case ING_SNATm:
                snat = soc_mem_table_idx_to_pointer(unit, mem, 
                                     ing_snat_entry_t *, buffer, idx);
                entry_ptr = (void *) snat;
                break;
            case L3_ENTRY_IPV4_MULTICASTm:
                dnat = soc_mem_table_idx_to_pointer(unit, mem, 
                                   l3_entry_ipv4_multicast_entry_t *, buffer, idx);
                entry_ptr = (void *) dnat;
                break;
            default:
                soc_cm_sfree(unit, buffer);
                return BCM_E_PARAM;
            }
            rv = _bcm_td2_l3_nat_ingress_hw_to_sw(unit, mem, entry_ptr, 
                                                  &tmp_nat);
            /* no valid entry */
            if (BCM_E_EMPTY == rv) {
                continue;
            }
            (void)_bcm_td2_l3_nat_ingress_hit_update(unit, mem, idx, 
                                                  &tmp_nat);

            if ((flags & BCM_L3_NAT_INGRESS_HIT)
                && (!(tmp_nat.flags & BCM_L3_NAT_INGRESS_HIT))) {
                continue;
            }

            if ((flags & BCM_L3_NAT_INGRESS_MULTIPATH)
                && (!(tmp_nat.flags & BCM_L3_NAT_INGRESS_MULTIPATH))) {
                continue;
            }

            if ((flags & BCM_L3_NAT_INGRESS_RPE)
                && (!(tmp_nat.flags & BCM_L3_NAT_INGRESS_RPE))) {
                continue;
            }

            if ((flags & BCM_L3_NAT_INGRESS_DST_DISCARD)
                && (!(tmp_nat.flags & BCM_L3_NAT_INGRESS_DST_DISCARD))) {
                continue;
            }

            if ((flags & (~tmp_flags)) != (tmp_nat.flags & (~tmp_flags))) {
                continue;
            }

            /* call callback */
            rv = (*cb)(unit, idx, &tmp_nat, user_data);
#ifdef BCM_CB_ABORT_ON_ERR
            if (BCM_FAILURE(rv) && SOC_CB_ABORT_ON_ERR(unit)) {
                break;
            }
#endif
        }
        if (idx > end) {
            /* broke out at end of loop */
            rv = BCM_E_NONE;
        }
    }
    if (buffer)  {
        soc_cm_sfree(unit, buffer);
    }
    BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
#endif /* BCM_TRIDENT_SUPPORT */
    return rv;
}

/* 
 * Function:
 *      bcm_esw_l3_nat_ingress_find
 *
 * Description:
 *      Find an ingress NAT table entry. Depending on the flags settings an
 *      entry is search for in either the ingress DNAT pool table, DNAT session
 *      table or SNAT session table.
 *
 * Parameters:
 *      unit        - (IN) bcm device
 *      nat_info    - (IN/OUT) ingress nat_info
 */
int 
bcm_esw_l3_nat_ingress_find(
    int unit, 
    bcm_l3_nat_ingress_t *nat_info)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    soc_mem_t mem;
    ing_dnat_address_type_entry_t pool;
    ing_snat_entry_t snat;
    l3_entry_ipv4_multicast_entry_t dnat;
    void *result;
    int idx;
    if (soc_feature(unit, soc_feature_nat)) {
        if (NULL == nat_info) {
            return BCM_E_PARAM;
        }
        
        /* search for the entry */
        BCM_L3_NAT_INGRESS_GET_MEM_ONLY(unit, nat_info, mem, pool, dnat, 
                                           snat, result);
        BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_ingress_sw_to_hw(unit, mem, result, 
                            nat_info));

        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_lock(unit));
        if (mem != ING_SNATm) {
            rv = soc_mem_search(unit, mem, MEM_BLOCK_ANY, &idx, result, 
                                result, 0);
            if (rv != SOC_E_NONE) {
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return BCM_E_NOT_FOUND;
            }
        } else {
            rv = _bcm_td2_l3_nat_ing_snat_lookup(unit, nat_info, result, &idx,
                                                 result);
            if (rv != BCM_E_NONE) {
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return BCM_E_NOT_FOUND;
            }
        }
        rv = _bcm_td2_l3_nat_ingress_hw_to_sw(unit, mem, result, nat_info);
        (void)_bcm_td2_l3_nat_ingress_hit_update(unit, mem, idx, nat_info);
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
        return rv;
    }
#endif /* BCM_TRIDENT_SUPPORT */
    return rv;
}

/* 
 * Function:
 *      bcm_esw_l3_nat_ingress_delete_all
 *
 * Description:
 *      Traverse through the ingress NAT table specified by flags and invoke a
 *      user provided callback for each valid entry, that matches the flags.
 *
 * Parameters:
 *      unit            - (IN) bcm device
 *      nat_info        - (IN) Only flags field is valid. These are used to figure
 *                             out which table to clear 
 */
int bcm_esw_l3_nat_ingress_delete_all(
    int unit, 
    bcm_l3_nat_ingress_t *nat_info)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    soc_mem_t   mem;
    int mem_counter;
    if (soc_feature(unit, soc_feature_nat)) {
        if (NULL == nat_info) {
            return BCM_E_PARAM;
        }
    
        /* figure out which memory to clear */
        if (nat_info->flags & (BCM_L3_NAT_INGRESS_DNAT)) {
            if (nat_info->flags & BCM_L3_NAT_INGRESS_DNAT_POOL) {
                mem = ING_DNAT_ADDRESS_TYPEm;
                mem_counter = BCM_L3_NAT_INGRESS_POOL_CNT;
            } else {
                mem = L3_ENTRY_IPV4_MULTICASTm; 
                mem_counter = BCM_L3_NAT_INGRESS_DNAT_CNT;
            }
        } else {
            mem = ING_SNATm;
            mem_counter = BCM_L3_NAT_INGRESS_SNAT_CNT;
        } 

        /* if not shared table just clear it */
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_lock(unit));
        if (mem == ING_DNAT_ADDRESS_TYPEm) {
            rv = soc_mem_clear(unit, mem, COPYNO_ALL, 1);
            if (!SOC_SUCCESS(rv)) {
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return BCM_E_INTERNAL;
            }
        } else {
            /* if shared table, traverse and delete individual entries */
            rv = bcm_esw_l3_nat_ingress_traverse(unit, nat_info->flags, 0, 
                                                soc_mem_index_max(unit, mem), 
                                   _bcm_td2_l3_nat_ingress_delete_entry, NULL);
            if (BCM_FAILURE(rv)) {
                BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
                return rv;
            }
        }
        /* clear table count to 0 */
        BCM_L3_NAT_INGRESS_CLR_TBL_CNT(unit, mem_counter);
        BCM_IF_ERROR_RETURN(_bcm_esw_l3_nat_unlock(unit));
        return BCM_E_NONE;
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    return rv;
}

/* 
 * Function:
 *      bcm_esw_l3_nat_ingress_age
 *
 * Description:
 *      Run aging on either DNAT session or SNAT session table as specified
 *      by the flags. Clear the hit bit for the entry or if it is already cleared
 *      delete the entry.
 *
 * Parameters:
 *      unit            - (IN) bcm device
 *      flags           - (IN) flags to match to identify entry to act upon.
 *      age_cb          - (IN) user provided callback fn to call for each entry
 *      user_data       - (IN) parameter to user provided callback
 */
int bcm_esw_l3_nat_ingress_age(int unit, uint32 flags, 
                               bcm_l3_nat_ingress_traverse_cb age_cb, 
                               void *user_data)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    soc_mem_t   mem;
    bcm_l3_nat_ingress_cb_context_t user_context;

    if (soc_feature(unit, soc_feature_nat)) {
        /* validate flags. Aging supported only on DNAT or SNAT session tables */
        if (flags & BCM_L3_NAT_INGRESS_DNAT_POOL) {
            return BCM_E_PARAM;   
        }
        /* call traversal function with an ager callback */
        if (flags & BCM_L3_NAT_INGRESS_DNAT) {
            mem = L3_ENTRY_IPV4_MULTICASTm;
        } else {
            mem = ING_SNATm;
        }
        /* pass original user callback info to intermediate callback function */
        user_context.user_cb = age_cb;
        user_context.user_data = user_data;
        user_context.mem = mem;
        rv = bcm_esw_l3_nat_ingress_traverse(unit, flags, soc_mem_index_min(unit, mem),
                                         soc_mem_index_max(unit, mem), 
                                         _bcm_td2_l3_nat_ingress_age_entry,
                                         (void *) &user_context);
        return rv;
       
    }
#endif /* BCM_TRIDENT2_SUPPORT */
    return rv;
}

#if 0
#if defined(BCM_TRIDENT2_SUPPORT)
int
_bcm_td2_l3_nat_ingress_key_match(int unit, soc_mem_t mem, void *hw_buf)
{
    if (mem == ING_DNAT_ADDRESS_TYPEm) {
        return 1;
    }
    if (mem == L3_ENTRY_IPV4_MULTICASTm) {
          return ((soc_mem_field32_get(unit, mem, hw_buf, KEY_TYPEf) == 
                                                  TD2_L3_HASH_KEY_TYPE_DST_NAT) || 
                  (soc_mem_field32_get(unit, mem, hw_buf, KEY_TYPEf) == 
                                                  TD2_L3_HASH_KEY_TYPE_DST_NAPT));
    }
    return 0;
}
int
_bcm_td2_l3_nat_ingress_conflict_get(int unit, bcm_l3_nat_ingress_t *nat_info, 
                                    bcm_l3_nat_ingress_t *cf_array, int cf_max, 
                                    int *cf_count)
{
    uint32 valid, *bufp;      /* Hardware buffer ptr     */
    soc_mem_t mem = L3_ENTRY_IPV4_MULTICASTm;            
    int index, rv = BCM_E_NONE;      /* Operation status.       */
    uint32 hw_buf[SOC_MAX_MEM_WORDS];    
    uint8 i, bidx, num_ent, shared_l3_banks, start_bidx, end_bidx;
    l3_entry_ipv4_multicast_entry_t l3v4_ext_entry;    
    ing_dnat_address_type_entry_t dnat_pool;

    

    /* Input parameters check. */
    if ((NULL == nat_info) || (NULL == cf_count) || 
        (NULL == cf_array) || (cf_max <= 0)) {
        return (BCM_E_PARAM);
    }

    if (nat_info->flags & BCM_L3_NAT_INGRESS_DNAT) {
        if (nat_info->flags & BCM_L3_NAT_INGRESS_DNAT_POOL) { 
            mem = ING_DNAT_ADDRESS_TYPEm;
            /* Set the valid field */
            valid = VALIDf;
            start_bidx = 0;
            end_bidx = 1;
            bufp =  (void *) &dnat_pool;
        } else {
            mem = L3_ENTRY_IPV4_MULTICASTm;
            /* Set the valid field */
            valid = VALID_0f;
            /* get number of shared L3 banks */
            shared_l3_banks = (soc_mem_index_count(unit, L3_ENTRY_ONLYm) - 
                                                          16 * 1024) / (64 * 1024);
            if (shared_l3_banks > 3) {
                return BCM_E_NOT_FOUND;
            }
            start_bidx = (6 - shared_l3_banks);
            end_bidx = 9;
            bufp =  (void *) &l3v4_ext_entry;
        }
    } else {
        /* Not support for SNAT table */
        return BCM_E_PARAM;
    }

    /* Translate lookup key to hw format. */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_nat_ingress_sw_to_hw(unit, mem, bufp, 
                        nat_info));
    
    
#if 0
    /* Set table fields */
    BCM_TD2_L3_HOST_TABLE_FLD(unit, mem, ipv6, fld);

    /* Assign entry buf based on table being used */
    BCM_TD2_L3_HOST_ENTRY_BUF(ipv6, mem, bufp,
                                    l3v4_entry,
                                    l3v4_ext_entry,
                                    l3v6_entry,
                                    l3v6_ext_entry);
    /* Prepare host entry for addition. */
    BCM_IF_ERROR_RETURN(_bcm_td2_l3_ent_init(unit, mem, &l3cfg, bufp));
#endif


    *cf_count = 0;

#if 0
    /* get number of shared L3 banks */
    shared_l3_banks = (soc_mem_index_count(unit, L3_ENTRY_ONLYm) - 16 * 1024) /
                      (64 * 1024);

    if (shared_l3_banks > 3) {
        return BCM_E_NOT_FOUND;
    }
#endif

    /* bank 9 to 6 are dedicated L3 banks, 5 to 3 are dedicated, depends on
     * mode how many shared banks are assigned to l3 
     */
    for (bidx = start_bidx; bidx <= end_bidx; bidx++) {

        num_ent = 4; /* 4 entries per bucket */

        BCM_TD2_NAT_INGRESS_BANK_ENTRY_HASH(mem, unit, bidx, bufp, index);
        if (index == 0) { 
            return BCM_E_INTERNAL;
        }

        for (i = 0; (i < num_ent) && (*cf_count < cf_max); i++) {
            rv = soc_mem_read(unit, mem, COPYNO_ALL, index + i, hw_buf);
            if (SOC_FAILURE(rv)) {
                return BCM_E_MEMORY;
            }
            if (soc_mem_field32_get(unit, mem, hw_buf, valid) &&
                _bcm_td2_l3_nat_ingress_key_match(unit, mem, hw_buf)) {
                rv = _bcm_td2_l3_nat_ingress_hw_to_sw(unit, mem, hw_buf, 
                                                            cf_array + (*cf_count));
                BCM_IF_ERROR_RETURN(rv);
                if ((++(*cf_count)) >= cf_max) {
                    break;
                }
            }
        }
    } 
    return BCM_E_NONE;
}
#endif /* BCM_TRIDENT2_SUPPORT */

/* 
 * Function:
 *      bcm_esw_l3_nat_ingress_conflict_get
 *
 * Description:
 *      Given a DNAT or DNAT pool table key, return conflicts in the corresponding
 *      table.
 *
 * Parameters:
 *      unit            - (IN) bcm device
 *      nat_info        - (IN) Destination NAT/NAPT lookup key
 *      cf_array        - (OUT) array of conflicting addresses (at most 4)
 *      cf_max          - (IN) max number of conflicts to return.
 *      cf_count        - (OUT) actual number of conflicting entries.
 * Returns:
 *      BCM_E_XXX
 */
int
bcm_esw_l3_nat_ingress_conflict_get(int unit, bcm_l3_nat_ingress_t *nat_info, 
                                    bcm_l3_nat_ingress_t *cf_array, int cf_max, 
                                    int *cf_count)
{
    int rv = BCM_E_UNAVAIL;

#if defined(BCM_TRIDENT2_SUPPORT)
    if (soc_feature(unit, soc_feature_nat)) {
        return _bcm_td2_l3_nat_ingress_conflict_get(unit, nat_info, cf_array, 
                                                    cf_max, cf_count);
    }
#endif /* BCM_TRIDENT2_SUPPORT */

    return rv;
}
#endif /* 0 */

#endif /* INCLUDE_L3 */
