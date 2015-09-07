/*
 * $Id: nat.h,v 1.6 Broadcom SDK $
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
 * NAT - Broadcom StrataSwitch NAT API internal definitions 
 */

#ifndef _BCM_INT_NAT_H
#define _BCM_INT_NAT_H

#ifdef INCLUDE_L3
/* NAT egress state per unit 
  - hash table of nat entry to index
  - ref counts per entry index
  - bitmap for allocation
*/

typedef struct _bcm_l3_nat_egress_hash_entry_s {
    uint32  ip_addr;
    uint16  vrf;
    uint16  l4_port;
    struct _bcm_l3_nat_egress_hash_entry_t *next;
    uint16  nat_id;
} _bcm_l3_nat_egress_hash_entry_t;

typedef struct _bcm_l3_nat_egress_state_s {
    _bcm_l3_nat_egress_hash_entry_t *_bcm_l3_nat_egress_hash_tbl[256];
    uint16 nat_id_refcount[2048];
    SHR_BITDCL *nat_id_bitmap;
} _bcm_l3_nat_egress_state_t;

typedef struct _bcm_l3_nat_ingress_state_s {
    uint32  tbl_cnts[3];
    int  snat_napt_free_idx;
    int  snat_nat_free_idx;
} _bcm_l3_nat_ingress_state_t;

typedef struct _bcm_l3_nat_state_s {
    _bcm_l3_nat_egress_state_t e_state;
    _bcm_l3_nat_ingress_state_t i_state;
    sal_mutex_t lock;
} _bcm_l3_nat_state_t;

extern _bcm_l3_nat_state_t *_bcm_l3_nat_state[BCM_MAX_NUM_UNITS];


/* helper struct for nat ingress age/delete */
typedef struct _bcm_l3_nat_ingress_cb_context_s {
    void *user_data;
    bcm_l3_nat_ingress_traverse_cb user_cb;
    soc_mem_t   mem;     
} bcm_l3_nat_ingress_cb_context_t;

/* index into table counters */
#define BCM_L3_NAT_INGRESS_POOL_CNT    0
#define BCM_L3_NAT_INGRESS_DNAT_CNT    1
#define BCM_L3_NAT_INGRESS_SNAT_CNT    2

#define BCM_L3_NAT_EGRESS_INFO(_unit_) _bcm_l3_nat_state[(_unit_)]->e_state
#define BCM_L3_NAT_INGRESS_INFO(_unit_) _bcm_l3_nat_state[(_unit_)]->i_state

/* nat packet translate table bitmap operations */
#define BCM_L3_NAT_EGRESS_ENTRIES_PER_INDEX    2

#define BCM_L3_NAT_EGRESS_HW_IDX_GET(nat_idx, hw_idx, hw_half) \
do {\
    (hw_idx) = (nat_idx) >> 1;\
    (hw_half) = (nat_idx) & 1;\
} while (0)

#define BCM_L3_NAT_EGRESS_SW_IDX_GET(hw_idx, hw_half) \
    ((hw_idx) << 1 | (hw_half))

#define BCM_L3_NAT_FILL_SW_ENTRY_X(hw_buf, nat_info, num)\
do {\
    int bit_count;\
    if (soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, (hw_buf), \
                            MODIFY_SRC_OR_DST_##num)) { /* DIP */\
        /* fill DNAT/NAPT */\
        (nat_info)->flags |= BCM_L3_NAT_EGRESS_DNAT;\
        (nat_info)->dip_addr = soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, \
                                (hw_buf), IP_ADDRESS_##num);\
        if (soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, (hw_buf), \
                                            MODIFY_L4_PORT_OR_PREFIX_##num)) {\
            /* modify prefix */\
            bit_count = soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, (hw_buf), \
                                    L4_PORT_OR_PREFIX_##num);\
            if (bit_count < 32) {\
                (nat_info)->dip_addr_mask = ((1 << bit_count) - 1) << \
                                                            (32 - bit_count);\
            } else {\
                (nat_info)->dip_addr_mask = 0xffffffff;\
            }\
        } else {\
            /* modify port */\
            (nat_info)->flags |= BCM_L3_NAT_EGRESS_NAPT;\
            (nat_info)->dst_port = soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, \
                                    (hw_buf), L4_PORT_OR_PREFIX_##num);\
        }\
    } else {\
        /* fill SNAT/NAPT */\
        (nat_info)->flags |= BCM_L3_NAT_EGRESS_SNAT;\
        (nat_info)->sip_addr = soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, \
                                (hw_buf), IP_ADDRESS_##num);\
        if (soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, (hw_buf), \
                                            MODIFY_L4_PORT_OR_PREFIX_##num)) {\
            /* modify prefix */\
            bit_count = soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, (hw_buf), \
                                    L4_PORT_OR_PREFIX_##num);\
            if (bit_count < 32) {\
                (nat_info)->sip_addr_mask = ((1 << bit_count) - 1) << \
                                                            (32 - bit_count);\
            } else {\
                (nat_info)->sip_addr_mask = 0xffffffff;\
            }\
        } else {\
            /* modify port */\
            (nat_info)->flags |= BCM_L3_NAT_EGRESS_NAPT;\
            (nat_info)->src_port = soc_EGR_NAT_PACKET_EDIT_INFOm_field32_get(unit, \
                                    (hw_buf), L4_PORT_OR_PREFIX_##num);\
        }\
    }\
} while (0)

#define BCM_L3_NAT_EGRESS_INC_REF_COUNT(unit, nat_id, count) \
do {\
    BCM_L3_NAT_EGRESS_INFO((unit)).nat_id_refcount[(nat_id)]++;\
    /* currently max of only 2 can be incremented per call */\
    if ((count) > 1) {\
        BCM_L3_NAT_EGRESS_INFO((unit)).nat_id_refcount[(nat_id) + 1]++;\
    }\
} while (0)

#define BCM_L3_NAT_EGRESS_DEC_REF_COUNT(unit, nat_id) \
    BCM_L3_NAT_EGRESS_INFO((unit)).nat_id_refcount[(nat_id)]--

#define BCM_L3_NAT_EGRESS_GET_REF_COUNT(unit, nat_id) \
    BCM_L3_NAT_EGRESS_INFO((unit)).nat_id_refcount[(nat_id)]


#define BCM_L3_NAT_INGRESS_GET_MEM_POINTER(unit, nat_info, mem, pool, dnat, \
                                           snat, result, mem_counter) \
do {\
    /* figure out which mem to operate on */\
    if ((nat_info)->flags & BCM_L3_NAT_INGRESS_DNAT) {\
        if ((nat_info)->flags & BCM_L3_NAT_INGRESS_DNAT_POOL) {\
            (mem) = ING_DNAT_ADDRESS_TYPEm;\
            (result) = &(pool);\
            (mem_counter) = BCM_L3_NAT_INGRESS_POOL_CNT;\
        }\
        else {\
            (mem) = L3_ENTRY_IPV4_MULTICASTm;\
            (result) = &(dnat);\
            (mem_counter) = BCM_L3_NAT_INGRESS_DNAT_CNT;\
        }\
    } else {\
        (mem) = ING_SNATm;\
        (result) = &(snat);\
        (mem_counter) = BCM_L3_NAT_INGRESS_SNAT_CNT;\
    }\
} while (0)

/* no counters */
#define BCM_L3_NAT_INGRESS_GET_MEM_ONLY(unit, nat_info, mem, pool, dnat, \
                                           snat, result) \
do {\
    /* figure out which mem to operate on */\
    if ((nat_info)->flags & BCM_L3_NAT_INGRESS_DNAT) {\
        if ((nat_info)->flags & BCM_L3_NAT_INGRESS_DNAT_POOL) {\
            (mem) = ING_DNAT_ADDRESS_TYPEm;\
            (result) = &(pool);\
        }\
        else {\
            (mem) = L3_ENTRY_IPV4_MULTICASTm;\
            (result) = &(dnat);\
        }\
    } else {\
        (mem) = ING_SNATm;\
        (result) = &(snat);\
    }\
} while (0)

#define BCM_L3_NAT_INGRESS_INC_TBL_CNT(unit, tbl) \
    BCM_L3_NAT_INGRESS_INFO((unit)).tbl_cnts[(tbl)]++;

#define BCM_L3_NAT_INGRESS_DEC_TBL_CNT(unit, tbl) \
    BCM_L3_NAT_INGRESS_INFO((unit)).tbl_cnts[(tbl)]--;

#define BCM_L3_NAT_INGRESS_GET_TBL_CNT(unit, tbl) \
    BCM_L3_NAT_INGRESS_INFO((unit)).tbl_cnts[(tbl)];

#define BCM_L3_NAT_INGRESS_CLR_TBL_CNT(unit, tbl) \
    BCM_L3_NAT_INGRESS_INFO((unit)).tbl_cnts[(tbl)] = 0;

#define BCM_L3_NAT_INGRESS_SNAT_NAPT_FREE_IDX(unit) \
    BCM_L3_NAT_INGRESS_INFO((unit)).snat_napt_free_idx

#define BCM_L3_NAT_INGRESS_SNAT_NAT_FREE_IDX(unit) \
    BCM_L3_NAT_INGRESS_INFO((unit)).snat_nat_free_idx

#define BCM_L3_NAT_EGRESS_FLAGS_FULL_NAT(flags) \
    ((flags) & (BCM_L3_NAT_EGRESS_SNAT | BCM_L3_NAT_EGRESS_DNAT)) == \
                    (BCM_L3_NAT_EGRESS_SNAT | BCM_L3_NAT_EGRESS_DNAT)
 
#define BCM_TD2_NAT_INGRESS_BANK_ENTRY_HASH(mem, unit, bidx, bufp, index) \
do {\
    if ((mem) == L3_ENTRY_IPV4_MULTICASTm) {\
        (index) = soc_td2_l3x_bank_entry_hash((unit), (bidx), (bufp));\
    } else {\
        /* mem is ING_DNAT_ADDRESS_TYPEm */\
        (index) = soc_td2_ing_dnat_address_type_bank_entry_hash((unit), (bidx), \
                                                                (bufp));\
    }\
} while (0)

extern int _bcm_esw_l3_nat_init(int unit);

#endif /* INCLUDE_L3 */
#endif /* _BCM_INT_NAT_H */
