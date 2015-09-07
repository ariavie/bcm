/*
 * $Id: trx.h,v 1.107 Broadcom SDK $
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
 * File:        trx.h
 * Purpose:     Function declarations for Triumph Micro Architecture 
 *              based devices.
 */

#ifndef _BCM_INT_TRX_H_
#define _BCM_INT_TRX_H_

#if defined(BCM_TRX_SUPPORT)
#include <bcm_int/esw/firebolt.h>

#if defined(INCLUDE_L3)
#define BCM_TRX_DEFIP128_HASH_SZ            (0x6)
#define BCM_TRX_DEFIP128(_unit_)             l3_trx_defip_128[(_unit_)]
#define BCM_TRX_DEFIP128_ARR(_unit_)         BCM_TRX_DEFIP128((_unit_))->entry_array
#define BCM_TRX_DEFIP128_IDX_MAX(_unit_)     BCM_TRX_DEFIP128((_unit_))->idx_max
#define BCM_TRX_DEFIP128_USED_COUNT(_unit_)  BCM_TRX_DEFIP128((_unit_))->used_count
#define BCM_TRX_DEFIP128_TOTAL(_unit_)       BCM_TRX_DEFIP128((_unit_))->total_count
#define BCM_TRX_DEFIP128_URPF_OFFSET(_unit_) BCM_TRX_DEFIP128((_unit_))->urpf_offset
#define BCM_TRX_DEFIP128_ENTRY_SET(_unit_, _idx_, _plen_, _hash_)          \
            BCM_TRX_DEFIP128_ARR((_unit_))[(_idx_)].prefix_len = (_plen_); \
            BCM_TRX_DEFIP128_ARR((_unit_))[(_idx_)].entry_hash = (_hash_)

/* Macro to know if L3 defip memories are enabled in external tcam.*/ 
#define EXT_TCAM_IS_L3_ENABLED(_u_)   ((SOC_MEM_IS_ENABLED(_u_, EXT_IPV4_DEFIPm) &&   \
                                        soc_mem_index_count(_u_, EXT_IPV4_DEFIPm)) || \
                                       (SOC_MEM_IS_ENABLED(_u_, EXT_IPV6_64_DEFIPm) &&   \
                                        soc_mem_index_count(_u_, EXT_IPV6_64_DEFIPm)) || \
                                       (SOC_MEM_IS_ENABLED(_u_, EXT_IPV6_128_DEFIPm) &&   \
                                        soc_mem_index_count(_u_, EXT_IPV6_128_DEFIPm)))      

typedef struct _bcm_l3_defip_128_entry_s {
    uint8  prefix_len; /* Route entry  prefix length.*/
    uint8  entry_hash; /* Route entry key hash.      */
} _bcm_l3_defip_128_entry_t;

typedef struct _bcm_l3_defip_128_table_s {
    _bcm_l3_defip_128_entry_t *entry_array; /* Cam entries array.          */
    uint16 idx_max;                   /* Last cam entry index.             */
    uint16 used_count;                /* Used cam entry count.             */
    uint16 total_count;               /* Total number of available entries.*/
    uint16 urpf_offset;               /* Src lookup start offset.          */
} _bcm_l3_defip_128_table_t;

extern int _bcm_trx_defip_128_init(int unit);
extern int _bcm_trx_defip_128_deinit(int unit);
extern int _bcm_trx_defip_128_get(int unit, _bcm_defip_cfg_t *lpm_cfg,
                                  int *nh_ecmp_idx); 
extern int _bcm_trx_defip_128_add(int unit, _bcm_defip_cfg_t *lpm_cfg,
                                  int nh_ecmp_idx);
extern int _bcm_trx_defip_128_delete(int unit, _bcm_defip_cfg_t *lpm_cfg);
extern int _bcm_trx_defip_128_update_match(int unit, _bcm_l3_trvrs_data_t *trv_data);
extern int _bcm_trx_l3_defip_mem_get(int unit, uint32 flags, 
                                     int plen, soc_mem_t *mem);
extern int _bcm_trx_l3_defip_verify_internal_mem_usage(int unit);
extern int _bcm_trx_tnl_type_to_hw_code(int unit, bcm_tunnel_type_t tnl_type, 
                                        int *hw_code, int *entry_type);
extern int _bcm_trx_tnl_hw_code_to_type(int unit, int hw_tnl_type, 
                                         int entry_type, bcm_tunnel_type_t
                                         *tunnel_type);
extern int _bcm_trx_tunnel_dscp_map_port_set(int unit, bcm_port_t port,
                                             bcm_tunnel_dscp_map_t *dscp_map);
extern int _bcm_trx_tunnel_dscp_map_port_get(int unit, bcm_port_t port,
                                             bcm_tunnel_dscp_map_t *dscp_map);
extern int bcm_trx_multicast_init(int unit);
extern int bcm_trx_multicast_detach(int unit);
extern int bcm_trx_multicast_group_get(int unit, bcm_multicast_t group,
                                        uint32 *flags);
extern int bcm_trx_multicast_group_traverse(int unit,
                             bcm_multicast_group_traverse_cb_t trav_fn, 
                                            uint32 flags, void *user_data);
#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_trx_multicast_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */
#endif /* INCLUDE_L3 */

#define _BCM_TRX_TAG_ACTION_NONE         0
#define _BCM_TRX_TAG_ACTION_ADD          1
#define _BCM_TRX_TAG_ACTION_REPLACE      2
#define _BCM_TRX_TAG_ACTION_DELETE       3
#define _BCM_TRX_TAG_ACTION_COPY         4
#if defined(BCM_TRIDENT2_SUPPORT)
#define _BCM_TRX_TAG_ACTION_COPY_VSAN    5
#define _BCM_TRX_TAG_ACTION_REPLACE_VSAN 6
#endif

#define _BCM_TRX_PRI_CFI_ACTION_NONE     0
#define _BCM_TRX_PRI_CFI_ACTION_ADD      1
#define _BCM_TRX_PRI_CFI_ACTION_REPLACE  2
#define _BCM_TRX_PRI_CFI_ACTION_COPY     3
#if defined(BCM_TRIDENT2_SUPPORT)
#define _BCM_TRX_PRI_CFI_ACTION_COPY_VSAN    4
#define _BCM_TRX_PRI_CFI_ACTION_REPLACE_VSAN 5
#endif

#define _BCM_TRX_TAG_ACTION_ENCODE(_action_) \
    ((_action_) == bcmVlanActionNone ? _BCM_TRX_TAG_ACTION_NONE :       \
     (_action_) == bcmVlanActionAdd ? _BCM_TRX_TAG_ACTION_ADD :         \
     (_action_) == bcmVlanActionReplace ? _BCM_TRX_TAG_ACTION_REPLACE : \
     (_action_) == bcmVlanActionDelete ? _BCM_TRX_TAG_ACTION_DELETE :   \
     _BCM_TRX_TAG_ACTION_COPY)

#define _BCM_TRX_TAG_ACTION_DECODE(_hw_action_) \
    ((_hw_action_) == _BCM_TRX_TAG_ACTION_NONE ? bcmVlanActionNone :    \
     (_hw_action_) == _BCM_TRX_TAG_ACTION_ADD ? bcmVlanActionAdd :      \
     (_hw_action_) == _BCM_TRX_TAG_ACTION_REPLACE ? bcmVlanActionReplace : \
     (_hw_action_) == _BCM_TRX_TAG_ACTION_DELETE ? bcmVlanActionDelete : \
     bcmVlanActionCopy)

#if defined(BCM_TRIDENT2_SUPPORT)
/* extended version for fcoe capable chips */

#define _BCM_TRX_TAG_ACTION_ENCODE_EGR_FCOE_EXT(_action_) \
    ((_action_) == bcmVlanActionNone ? _BCM_TRX_TAG_ACTION_NONE :       \
     (_action_) == bcmVlanActionAdd ? _BCM_TRX_TAG_ACTION_ADD :         \
     (_action_) == bcmVlanActionReplace ? _BCM_TRX_TAG_ACTION_REPLACE : \
     (_action_) == bcmVlanActionDelete ? _BCM_TRX_TAG_ACTION_DELETE :   \
     (_action_) == bcmVlanActionCopyVsan ? _BCM_TRX_TAG_ACTION_COPY_VSAN : \
     (_action_) == bcmVlanActionReplaceVsan ? _BCM_TRX_TAG_ACTION_REPLACE_VSAN : \
     _BCM_TRX_TAG_ACTION_COPY)

#define _BCM_TRX_TAG_ACTION_DECODE_EGR_FCOE_EXT(_hw_action_) \
    ((_hw_action_) == _BCM_TRX_TAG_ACTION_NONE ? bcmVlanActionNone :    \
     (_hw_action_) == _BCM_TRX_TAG_ACTION_ADD ? bcmVlanActionAdd :      \
     (_hw_action_) == _BCM_TRX_TAG_ACTION_REPLACE ? bcmVlanActionReplace : \
     (_hw_action_) == _BCM_TRX_TAG_ACTION_DELETE ? bcmVlanActionDelete : \
     (_hw_action_) == _BCM_TRX_TAG_ACTION_COPY_VSAN ? bcmVlanActionCopyVsan : \
     (_hw_action_) == _BCM_TRX_TAG_ACTION_REPLACE_VSAN ? bcmVlanActionReplaceVsan : \
     bcmVlanActionCopy)

#endif


#define _BCM_TRX_PRI_CFI_ACTION_ENCODE(_action_) \
    ((_action_) == bcmVlanActionNone ? _BCM_TRX_PRI_CFI_ACTION_NONE :       \
     (_action_) == bcmVlanActionAdd ? _BCM_TRX_PRI_CFI_ACTION_ADD :         \
     (_action_) == bcmVlanActionReplace ? _BCM_TRX_PRI_CFI_ACTION_REPLACE : \
     _BCM_TRX_PRI_CFI_ACTION_COPY)

#define _BCM_TRX_PRI_CFI_ACTION_DECODE(_hw_action_) \
    ((_hw_action_) == _BCM_TRX_PRI_CFI_ACTION_NONE ? bcmVlanActionNone :    \
     (_hw_action_) == _BCM_TRX_PRI_CFI_ACTION_ADD ? bcmVlanActionAdd :      \
     (_hw_action_) == _BCM_TRX_PRI_CFI_ACTION_REPLACE ? bcmVlanActionReplace : \
     bcmVlanActionCopy)

#if defined(BCM_TRIDENT2_SUPPORT)
/* extended version for fcoe capable chips */
#define _BCM_TRX_PRI_CFI_ACTION_ENCODE_FCOE_EXT(_action_) \
    ((_action_) == bcmVlanActionNone ? _BCM_TRX_PRI_CFI_ACTION_NONE :       \
     (_action_) == bcmVlanActionAdd ? _BCM_TRX_PRI_CFI_ACTION_ADD :         \
     (_action_) == bcmVlanActionReplace ? _BCM_TRX_PRI_CFI_ACTION_REPLACE : \
     (_action_) == bcmVlanActionCopyVsan ? _BCM_TRX_PRI_CFI_ACTION_COPY_VSAN : \
     (_action_) == bcmVlanActionReplaceVsan ? _BCM_TRX_PRI_CFI_ACTION_REPLACE_VSAN : \
     _BCM_TRX_PRI_CFI_ACTION_COPY)

#define _BCM_TRX_PRI_CFI_ACTION_DECODE_FCOE_EXT(_hw_action_) \
    ((_hw_action_) == _BCM_TRX_PRI_CFI_ACTION_NONE ? bcmVlanActionNone :    \
     (_hw_action_) == _BCM_TRX_PRI_CFI_ACTION_ADD ? bcmVlanActionAdd :      \
     (_hw_action_) == _BCM_TRX_PRI_CFI_ACTION_REPLACE ? bcmVlanActionReplace : \
     (_hw_action_) == _BCM_TRX_PRI_CFI_ACTION_COPY_VSAN ? bcmVlanActionCopyVsan : \
     (_hw_action_) == _BCM_TRX_PRI_CFI_ACTION_REPLACE_VSAN ? bcmVlanActionReplaceVsan : \
     bcmVlanActionCopy)
#endif

/*
 * Vlan block profile structure.  
 *
 * Purpose  : Implement different block functionality per vlan.
 */
typedef struct _trx_vlan_block_s {
    soc_pbmp_t first_mask;
    soc_pbmp_t second_mask;
    uint8 broadcast_mask_sel;
    uint8 unknown_unicast_mask_sel;
    uint8 unknown_mulitcast_mask_sel;
    uint8 known_mulitcast_mask_sel;
} _trx_vlan_block_t;

extern int _bcm_trx_vlan_action_profile_init(int unit);
extern int _bcm_trx_vlan_action_profile_entry_add(int unit,
                                      bcm_vlan_action_set_t *action,
                                      uint32 *index);
extern void _bcm_trx_vlan_action_profile_entry_increment(int unit, uint32 index);
extern void _bcm_trx_vlan_action_profile_entry_get(int unit,
                                      bcm_vlan_action_set_t *action,
                                      uint32 index);
extern int _bcm_trx_vlan_action_profile_entry_delete(int unit, uint32 index);
extern int _bcm_trx_vlan_action_profile_detach(int unit);

extern int _bcm_trx_vlan_range_profile_entry_delete(int unit, uint32 index);
extern void _bcm_trx_vlan_range_profile_entry_increment(int unit, uint32 index);

extern int _bcm_trx_egr_vlan_action_profile_entry_add(int unit,
                                      bcm_vlan_action_set_t *action,
                                      uint32 *index);
extern void _bcm_trx_egr_vlan_action_profile_entry_increment(int unit, uint32 index);
extern void _bcm_trx_egr_vlan_action_profile_entry_get(int unit,
                                          bcm_vlan_action_set_t *action,
                                          uint32 index);
extern int _bcm_trx_egr_vlan_action_profile_entry_delete(int unit, uint32 index);

extern int _bcm_trx_vlan_ip_action_add(int unit, bcm_vlan_ip_t *vlan_ip,
                                      bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_ip_action_get(int unit, bcm_vlan_ip_t *vlan_ip,
                                      bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_ip_action_traverse(int unit, 
                                            bcm_vlan_ip_action_traverse_cb cb, 
                                            void *user_data);


extern int _bcm_trx_vlan_ip_delete(int unit, bcm_vlan_ip_t *vlan_ip);
extern int _bcm_trx_vlan_ip_delete_all(int unit);
extern int _bcm_trx_vlan_mac_action_add(int unit, bcm_mac_t mac,
                                       bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_mac_delete(int unit, bcm_mac_t mac);
extern int _bcm_trx_vlan_mac_delete_all(int unit);

extern int _bcm_trx_vlan_port_prot_match_get(int unit, 
                                             bcm_port_frametype_t frame, 
                                             bcm_port_ethertype_t ether, 
                                             int *match_idx);
extern int _bcm_trx_vlan_port_prot_empty_get(int unit, int *empty_idx);


extern int _bcm_trx_vlan_translate_entry_assemble(int unit, vlan_xlate_entry_t *vent,
                                      bcm_gport_t port,
                                      bcm_vlan_translate_key_t key_type,
                                      bcm_vlan_t inner_vlan, 
                                      bcm_vlan_t outer_vlan);

extern int _bcm_trx_vlan_translate_action_add(int unit,
                                             bcm_gport_t port,
                                             bcm_vlan_translate_key_t key_type,
                                             bcm_vlan_t outer_vlan,
                                             bcm_vlan_t inner_vlan,
                                             bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_translate_action_delete(int unit,
                                                bcm_gport_t port,
                                                bcm_vlan_translate_key_t key_type,
                                                bcm_vlan_t outer_vlan,
                                                bcm_vlan_t inner_vlan);
extern int _bcm_trx_vlan_translate_action_delete_all(int unit);
extern int _bcm_trx_vlan_translate_action_get (int unit, 
                                              bcm_gport_t port, 
                                              bcm_vlan_translate_key_t key_type, 
                                              bcm_vlan_t outer_vid,
                                              bcm_vlan_t inner_vid,
                                              bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_translate_action_range_add(int unit,
                                                   bcm_gport_t port,
                                                   bcm_vlan_t outer_vlan_low,
                                                   bcm_vlan_t outer_vlan_high,
                                                   bcm_vlan_t inner_vlan_low,
                                                   bcm_vlan_t inner_vlan_high,
                                                   bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_translate_action_range_get(int unit,
                                                   bcm_gport_t port,
                                                   bcm_vlan_t outer_vlan_low,
                                                   bcm_vlan_t outer_vlan_high,
                                                   bcm_vlan_t inner_vlan_low,
                                                   bcm_vlan_t inner_vlan_high,
                                                   bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_translate_action_range_delete(int unit,
                                                      bcm_gport_t port,
                                                      bcm_vlan_t outer_vlan_low,
                                                      bcm_vlan_t outer_vlan_high,
                                                      bcm_vlan_t inner_vlan_low,
                                                      bcm_vlan_t inner_vlan_high);
extern int _bcm_trx_vlan_translate_action_range_delete_all(int unit);
extern int _bcm_trx_vlan_translate_action_range_traverse(int unit, 
                                  _bcm_vlan_translate_traverse_t *trvs_info);

extern int _bcm_trx_vlan_translate_parse(int unit, soc_mem_t mem, uint32 *vent, 
                                  _bcm_vlan_translate_traverse_t *trvs_info);
extern int _bcm_trx_vlan_translate_egress_entry_assemble(int unit,
                                              egr_vlan_xlate_entry_t *vent,
                                              int port_class,
                                              bcm_vlan_t outer_vlan,
                                              bcm_vlan_t inner_vlan);
extern int _bcm_trx_vlan_translate_egress_action_add(int unit, int port_class,
                                         bcm_vlan_t outer_vlan,
                                         bcm_vlan_t inner_vlan,
                                         bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_translate_egress_action_get(int unit, int port_class,
                                         bcm_vlan_t outer_vlan,
                                         bcm_vlan_t inner_vlan,
                                         bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_translate_egress_action_delete(int unit, int port_class,
                                            bcm_vlan_t outer_vlan,
                                            bcm_vlan_t inner_vlan);
extern int _bcm_trx_vlan_mac_action_traverse(int unit, 
                                             bcm_vlan_mac_action_traverse_cb cb, 
                                             void *user_data);
extern int _bcm_trx_vlan_mac_action_get(int unit, bcm_mac_t  mac,
                             bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_translate_egress_action_delete_all(int unit);

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_trx_vlan_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

extern int bcm_trx_metro_l2_tunnel_add(int unit, bcm_mac_t mac, bcm_vlan_t vlan);
extern int bcm_trx_metro_l2_tunnel_delete(int unit, bcm_mac_t mac, bcm_vlan_t vlan);
extern int bcm_trx_metro_l2_tunnel_delete_all(int unit);
extern int _bcm_common_init(int unit);
extern int _bcm_common_cleanup(int unit);
extern int _bcm_mac_da_profile_entry_add(int unit, void **entries, 
                                         int entries_per_set, uint32 *index);
extern int _bcm_mac_da_profile_entry_delete(int unit, int index);

/* LPORT Profile Table */
extern int _bcm_lport_profile_mem_reference(int unit, int index,
                                            int entries_per_set);
extern int _bcm_lport_profile_entry_add(int unit, void **entries, 
                                         int entries_per_set, uint32 *index);
extern int _bcm_lport_profile_entry_delete(int unit, int index);
extern int _bcm_lport_profile_entry_get(int unit, int index, int count,
                                        void **entries);
extern int _bcm_lport_profile_fields32_modify(int unit, int table_id,
                                              int field_count,
                                              soc_field_t *fields,
                                              uint32 *values);
extern int _bcm_lport_profile_field32_modify(int unit, int table_id,
                                             soc_field_t field,
                                             uint32 value);

#define _BCM_TR2_PRI_CNG_MAP_SIZE (16)
extern int _bcm_ing_pri_cng_map_entry_add(int unit, void **entries, 
                                         int entries_per_set, uint32 *index);

extern int _bcm_ing_vft_pri_map_entry_add(int unit, void **entries, 
                                         int entries_per_set, uint32 *index);
extern int _bcm_ing_l2_vlan_etag_map_entry_add(int unit, 
                            void **entries, int entries_per_set, uint32 *index);
extern int _bcm_egr_l2_vlan_etag_map_entry_add(int unit, 
                            void **entries, int entries_per_set, uint32 *index);
extern int _bcm_egr_vft_pri_map_entry_add(int unit, void **entries, 
                                         int entries_per_set, uint32 *index);
extern int _bcm_ing_vft_pri_map_entry_del(int unit, int index);
extern int _bcm_ing_l2_vlan_etag_map_entry_del(int unit, int index);
extern int _bcm_egr_l2_vlan_etag_map_entry_del(int unit, int index);
extern int _bcm_egr_vft_pri_map_entry_del(int unit, int index);
extern int _bcm_ing_vft_pri_map_entry_get(int unit, int index, int count, 
                                          void **entries);
extern int _bcm_ing_l2_vlan_etag_map_entry_get(int unit, int index, 
                                           int count, void **entries);
extern int _bcm_egr_vft_pri_map_entry_get(int unit, int index, int count, 
                                          void **entries);
extern int _bcm_egr_l2_vlan_etag_map_entry_get(int unit, int index, 
                                           int count, void **entries);
extern int _bcm_egr_vsan_intpri_map_entry_add(int unit, void **entries, 
                                              int entries_per_set, 
                                              uint32 *index);
extern int _bcm_egr_vsan_intpri_map_entry_del(int unit, int index);
extern int _bcm_egr_vsan_intpri_map_entry_get(int unit, int index, int count, 
                                              void **entries);
extern int _bcm_egr_vft_fields_entry_add(int unit, void **entries, 
                                         int entries_per_set, 
                                         uint32 *index);
extern int _bcm_egr_vft_fields_entry_del(int unit, int index);
extern int _bcm_egr_vft_fields_entry_get(int unit, int index, int count, 
                                         void **entries);

extern int _bcm_ing_pri_cng_map_entry_delete(int unit, int index);
extern int _bcm_ing_pri_cng_map_entry_get(int unit, int index, int count,
                                          void **entries);
extern int _bcm_ing_pri_cng_map_entry_reference(int unit, int index, 
                                                int entries_per_set_override);
extern int _bcm_egr_mpls_combo_map_entry_add(int unit, void **entries, 
                                             int entries_per_set,
                                             uint32 *index);
extern int _bcm_egr_mpls_combo_map_entry_update(int unit, void **entries, 
                                                uint32 index);
extern int _bcm_egr_mpls_combo_map_entry_delete(int unit, int index) ;
extern int _bcm_egr_mpls_combo_map_entry_reference(int unit, int index, 
                                                   int entries_per_set_override);
extern int _bcm_dscp_table_entry_add(int unit, void **entries, 
                                         int entries_per_set, uint32 *index);
extern int _bcm_dscp_table_entry_delete(int unit, int index);
extern int _bcm_dscp_table_entry_reference(int unit, int index,
                                           int entries_per_set_override);
extern int _bcm_dscp_table_entry_ref_count_get(int unit, uint32 index,
                                                int* ref_count);
extern int _bcm_dscp_table_entry_get(int unit, int index, int count,
                                          void **entries);
extern int _bcm_egr_dscp_table_entry_add(int unit, void **entries, 
                                         int entries_per_set, uint32 *index);
extern int _bcm_egr_dscp_table_entry_delete(int unit, int index);
extern int _bcm_egr_dscp_table_entry_reference(int unit, int index, 
                                               int entries_per_set_override);
extern int _bcm_prot_pkt_ctrl_add(int unit, uint32 prot_pkt_ctrl,
                                  uint32 igmp_mld_pkt_ctrl, uint32 *index);
extern int _bcm_prot_pkt_ctrl_delete(int unit, uint32 index);
extern int _bcm_prot_pkt_ctrl_get(int unit, uint32 index,
                                  uint32 *prot_pkt_ctrl,
                                  uint32 *igmp_mld_pkt_ctrl);
extern int _bcm_prot_pkt_ctrl_reference(int unit, uint32 index);
extern int _bcm_prot_pkt_ctrl_ref_count_get(int unit, uint32 index,
                                            int* ref_count);
extern int _bcm_ing_l3_nh_attrib_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index);
extern int _bcm_ing_l3_nh_attrib_entry_delete(int unit, int index) ;
extern int _bcm_ing_l3_nh_attrib_entry_reference(int unit, int index, 
                                int entries_per_set_override);
extern void _bcm_common_profile_mem_ref_cnt_update(int unit, soc_mem_t mem, 
                                                   int profile_ptr, int count);

#ifdef BCM_TRIDENT2_SUPPORT
extern int _bcm_l3_iif_profile_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index);
extern int _bcm_l3_iif_profile_entry_delete(int unit, int index);
extern int _bcm_l3_iif_profile_entry_get(int unit, int index, int count,
                                         void **entries);
extern int _bcm_l3_iif_profile_entry_reference(int unit, int index, 
                                int entries_per_set_override);
extern int _bcm_l3_iif_profile_entry_update(int unit, void **entries, 
                                            uint32 *index);

extern int _bcm_l3_ip4_options_profile_entry_add(int unit, void **entries, 
                              int entries_per_set, uint32 *index);
extern int _bcm_l3_ip4_options_profile_entry_delete(int unit, int index);
extern int _bcm_l3_ip4_options_profile_entry_get(int unit, int index, 
                                                 int count, void **entries);
extern int _bcm_l3_ip4_options_profile_entry_reference(int unit, int index, 
                                int entries_per_set_override);
extern int _bcm_l3_ip4_options_profile_entry_update(int unit, void **entries,
                                                    uint32 index);
extern int _bcm_fc_map_profile_entry_add(int unit, void **entries, 
                                         int entries_per_set, uint32 *index);
extern int _bcm_fc_map_profile_entry_delete(int unit, int index);
extern int _bcm_fc_map_profile_entry_get(int unit, int index, int count, 
                                         void **entries);
extern int _bcm_l3_iif_profile_sw_state_set(int unit, void **entries,
                                            int entries_per_set, uint32 index);

#endif
#ifdef BCM_KATANA_SUPPORT
extern int _bcm_offset_map_table_entry_add(int unit, void **entries,
                                int entries_per_set, uint32 *index);
extern int _bcm_offset_map_table_entry_delete(int unit, int index);
extern int _bcm_offset_map_table_entry_get(int unit, int index, 
                                           int count, void **entries);
extern int _bcm_offset_map_table_entry_reference(int unit, int index,
                                int entries_per_set_override);
#endif /*BCM_KATANA_SUPPORT */
#ifdef BCM_KATANA2_SUPPORT
extern int _bcm_port_vlan_protocol_data_entry_add(
           int unit, void **entries, int entries_per_set, uint32 *index);
extern int _bcm_port_vlan_protocol_data_entry_delete(
           int unit, int index);
extern int _bcm_port_vlan_protocol_data_entry_get(
           int unit, int index, int count, void **entries);
extern int _bcm_port_vlan_protocol_data_entry_reference(
           int unit, int index, int entries_per_set_override);
#endif /*BCM_KATANA2_SUPPORT */
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_common_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

extern int _bcm_trx_egr_src_port_outer_tpid_set(int unit, int tpid_index,
                                                int enable);
extern int _bcm_trx_source_trunk_map_set(int unit, bcm_port_t port, 
                                         soc_field_t field, uint32 value);
extern int _bcm_trx_source_trunk_map_get(int unit, bcm_port_t port, 
                                         soc_field_t field, uint32 *value);
extern int _bcm_trx_port_force_vlan_set(int unit, bcm_port_t port,
                                        bcm_vlan_t vlan, int pkt_prio,
                                        uint32 flags);

extern int _bcm_trx_port_force_vlan_get(int unit, bcm_port_t port,
                                        bcm_vlan_t *vlan, int *pkt_prio, 
                                        uint32 *flags);
extern int  _bcm_trx_port_dtag_mode_set(int unit, bcm_port_t port, int mode);
extern int  _bcm_trx_port_dtag_mode_get(int unit, bcm_port_t port, int *mode);
extern int _bcm_trx_lport_tab_default_entry_add(int unit, 
                                                soc_profile_mem_t *prof);
extern int _bcm_trx_lport_tab_default_entry_index_get(int unit);
extern int _bcm_trx_port_control_egress_class_select_set(int unit, 
                                                         bcm_port_t port,
                                                         uint32 value);
extern int _bcm_trx_port_control_egress_class_select_get(int unit, 
                                                         bcm_port_t port,
                                                         uint32 *value);

/* HW encodings */
#define _BCM_TRX_PORT_FORCE_FORWARD_DISABLE  0
#define _BCM_TRX_PORT_FORCE_FORWARD_ALL      1
#define _BCM_TRX_PORT_FORCE_FORWARD_LOCAL    2

#if defined(BCM_FIELD_SUPPORT)
#define _BCM_FIELD_SC_INGRESS_SINGLE_WIDE_SLICE_SZ  (128)
#define _BCM_FIELD_SC_INGRESS_DOUBLE_WIDE_SLICE_SZ  (256)
#define _BCM_FIELD_SC_INGRESS_DOUBLE_WIDE_SLICE0 (8) 
#define _BCM_FIELD_TD_INGRESS_SINGLE_WIDE_SLICE_SZ  (128)
#define _BCM_FIELD_TD_INGRESS_DOUBLE_WIDE_SLICE_SZ  (256)
#define _BCM_FIELD_TD_INGRESS_DOUBLE_WIDE_SLICE0 (4) 
#define _BCM_FIELD_TD2_INGRESS_LARGE_SLICE_SZ  (512)
#define _BCM_FIELD_TD2_INGRESS_SMALL_SLICE_SZ  (256)
#define _BCM_FIELD_TD2_INGRESS_DOUBLE_WIDE_SLICE0 (4) 

#define _FP_TRX_RANGE_CHECKER_LOWER_MAX (23)
extern int _bcm_field_trx_tcp_ttl_tos_init(int unit);
extern int _bcm_trx_range_checker_selcodes_update(int unit,
                                                  _field_entry_t *f_ent);
extern int _bcm_field_trx_write_slice_map_vfp(int unit, _field_stage_t *stage_fc);
extern int _bcm_field_trx_write_slice_map_egress(int unit, _field_stage_t *stage_fc);
extern int _bcm_field_trx_mirror_ingress_add(int unit, soc_mem_t mem, 
                                             _field_entry_t *f_ent,
                                             _field_action_t *fa, uint32 *buf);
extern int _bcm_field_trx_mirror_egress_add(int unit, soc_mem_t mem, 
                                            _field_entry_t *f_ent,
                                            _field_action_t *fa, uint32 *buf);
extern int _bcm_field_trx_mirror_ingress_delete(int unit, soc_mem_t mem, 
                                                _field_entry_t *f_ent, 
                                                bcm_gport_t *mirror_dest, 
                                                uint32 *buf);
extern int _bcm_field_trx_mirror_egress_delete(int unit, soc_mem_t mem, 
                                               _field_entry_t *f_ent, 
                                               bcm_gport_t *mirror_dest, 
                                               uint32 *buf);
extern int _bcm_field_trx_action_support_check(int unit, _field_entry_t *f_ent,
                                                bcm_field_action_t action, 
                                                int *result);
extern int _bcm_field_trx_stage_action_support_check(int                unit,
                                                     unsigned           stage,
                                                     bcm_field_action_t action, 
                                                     int                *result
                                                     );
extern int _bcm_field_trx_action_conflict_check(int                unit,
                                                _field_entry_t     *f_ent, 
                                                bcm_field_action_t action1,
                                                bcm_field_action_t action
                                                );
extern int _bcm_field_trx_qualify_ip_type(int unit, _field_entry_t *f_ent, 
                                          bcm_field_IpType_t type);
extern int _bcm_field_trx_qualify_ip_type_get(int unit, _field_entry_t *f_ent, 
                                              bcm_field_IpType_t *type);
extern int _bcm_field_trx_qualify_ip_type_encode_get(int unit, uint32 hw_data,
                                                     uint32 hw_mask,
                                                     bcm_field_IpType_t *type);
extern int _bcm_field_trx_egress_key_match_type_set(int unit, 
                                                    _field_entry_t *f_ent);
extern int _bcm_field_trx_action_delete(int unit, _field_entry_t *f_ent,
                                        _field_action_t *fa, int tcam_idx, 
                                        uint32 param0, uint32 param1);
extern int _bcm_field_trx_tcam_get(int unit, soc_mem_t mem, 
                                   _field_entry_t *f_ent, uint32 *buf);
extern int _bcm_field_trx_action_counter_update(int unit, soc_mem_t mem, 
                                                _field_entry_t *f_ent, 
                                                _field_action_t *fa,
                                                uint32 *buf);
extern int _bcm_field_trx_action_get(int unit, soc_mem_t mem,
                                     _field_entry_t *f_ent, 
                                      int tcam_idx, _field_action_t *fa, 
                                      uint32 *buf);
extern int _bcm_field_trx_range_check_set(int unit, int range, uint32 flags, 
                                          int enable, bcm_l4_port_t min, 
                                          bcm_l4_port_t max);
extern int _bcm_field_trx_redirect_profile_delete(int unit, int index);
extern int _bcm_field_trx_redirect_profile_alloc(int unit, _field_entry_t *f_ent, 
                                          _field_action_t *fa);
extern int _bcm_field_trx_redirect_profile_ref_count_get(int unit, int index,
                                                         int *ref_count);
extern int _bcm_field_trx_stat_hw_mode_to_bmap(int unit, uint8 mode, 
                                               _field_stage_id_t stage_id, 
                                               uint32 *hw_bmap,
                                               uint8 *hw_entry_count);
extern int _bcm_field_trx_stat_index_get(int unit, _field_stat_t *f_st, 
                                         bcm_field_stat_t stat,
                                         int *idx1, int *idx2, int *idx3, 
                                         uint32 *flags);
extern int _bcm_field_trx_stat_action_set(int unit, _field_entry_t *f_ent, 
                                          soc_mem_t mem, int tcam_idx,
                                          uint32 *buf);
extern int _bcm_field_trx_selcodes_install(int unit, _field_group_t *fg, 
                                           uint8 slice_numb, bcm_pbmp_t pbmp, 
                                           int selcode_index);
extern int _bcm_field_trx_policer_install(int unit, _field_entry_t *f_ent, 
                                          _field_policer_t *f_pl);
extern int _bcm_field_trx_policer_action_set(int unit, _field_entry_t *f_ent,
                                             soc_mem_t mem, uint32 *buf);
extern int _bcm_field_trx_control_arp_set(int unit,  
                                          bcm_field_control_t control,
                                          uint32 state);
extern int _bcm_field_trx_egress_selcode_get(int unit, _field_stage_t *stage_fc, 
                               bcm_field_qset_t *qset_req,
                               _field_group_t *fg);
extern int _bcm_field_trx_slice_clear(int unit, _field_group_t *fg, _field_slice_t *fs); 
extern int _bcm_field_trx_ingress_slice_clear(int unit, uint8 slice_numb);
extern int _bcm_field_trx_egress_slice_clear(int unit, uint8 slice_numb);
extern int _bcm_field_trx_action_params_check(int unit,_field_entry_t *f_ent, 
                                               _field_action_t *fa);
extern int
_bcm_field_trx_vlan_format_qualify_is_double_tagged(unsigned data,
                                                    unsigned mask
                                                    );
extern int
_bcm_field_trx_action_depends_check(int             unit,
                                    _field_entry_t  *f_ent, 
                                    _field_action_t *fa
                                    );
extern int _bcm_field_trx2_data_qualifier_ethertype_delete(int unit, int qual_id,
                                 bcm_field_data_ethertype_t *etype);
extern int _bcm_field_trx2_data_qualifier_ethertype_add(int unit, int qual_id,
                                 bcm_field_data_ethertype_t *etype);
extern int _bcm_field_trx2_data_qualifier_ip_protocol_add(int unit,  int qual_id,
                          bcm_field_data_ip_protocol_t *ip_protocol);
extern int _bcm_field_trx2_data_qualifier_ip_protocol_delete(int unit,  int qual_id,
                          bcm_field_data_ip_protocol_t *ip_protocol);
extern int _bcm_field_trx2_data_qualifier_packet_format_add(int unit,int qual_id,
                                bcm_field_data_packet_format_t *packet_format);
extern int _bcm_field_trx2_data_qualifier_packet_format_delete(int unit,int qual_id,
                                bcm_field_data_packet_format_t *packet_format);
extern int _bcm_field_trx2_l2_actions_hw_free(int unit, _field_entry_t *f_ent,
                                             uint32 flags);
extern int _bcm_field_trx2_l2_actions_hw_alloc(int unit, _field_entry_t *f_ent);
extern int _bcm_field_trx2_udf_tcam_entry_parse(int unit, uint32 *hw_buf, uint32 *flags);
extern int _bcm_field_trx_ingress_pfs_bmap_get(int            unit,
                                               _field_group_t *fg,
                                               bcm_pbmp_t     *pbmp,
                                               int            selcode_index,
                                               SHR_BITDCL     *pfs_bmp
                                               );
extern int _bcm_field_trx_ingress_selcodes_install(int            unit,
                                                   _field_group_t *fg,
                                                   uint8          slice_num,
                                                   bcm_pbmp_t     *pbmp,
                                                   int            selcode_idx
                                                   );
extern int _bcm_field_trx_mode_set(int            unit,
                                   uint8          slice_numb,
                                   _field_group_t *fg,
                                   uint8          flags
                                   );
extern int _bcm_field_trx_egress_mode_set(int            unit,
                                          uint8          slice_numb,
                                          _field_group_t *fg,
                                          uint8          flags
                                          );
extern int _field_trx_group_qual_add(_bcm_field_group_qual_t *grp_qual,
                                     unsigned                 qid,
                                     _bcm_field_qual_offset_t *qual_offset
                                     );

extern const soc_field_t _bcm_trx2_aux_tag_1_field[], _bcm_trx2_aux_tag_2_field[];
extern const soc_field_t _bcm_field_trx_vfp_double_wide_sel[];
extern const soc_field_t _bcm_field_trx_vfp_field_sel[][2];
extern const soc_field_t _bcm_field_trx_vfp_ip_header_sel[];
extern const soc_field_t _bcm_field_trx_slice_pairing_field[];
extern const soc_field_t _bcm_field_trx_dw_f4_sel[];
extern const soc_field_t _bcm_field_trx_slice_wide_mode_field[16];
extern const soc_field_t _bcm_field_trx_field_sel[16][3];
extern const soc_field_t _bcm_field_trx_d_type_sel[];
extern const soc_field_t _bcm_field_trx_s_type_sel[];
#endif /* BCM_FIELD_SUPPORT */
extern int _bcm_trx_vlan_block_set (int unit, bcm_vlan_t vid, 
                                    bcm_vlan_block_t *block);
extern int _bcm_trx_vlan_block_get (int unit, bcm_vlan_t vid, 
                                    bcm_vlan_block_t *block);
extern int _bcm_trx_vp_tpid_add(int unit, bcm_gport_t vport, uint16 tpid, int color_select);
extern int _bcm_trx_vp_tpid_delete(int unit, bcm_gport_t vport, uint16 tpid);
extern int _bcm_trx_vp_tpid_delete_all(int unit, bcm_gport_t vport);
extern int _bcm_trx_vp_tpid_set(int unit, bcm_gport_t vport, uint16 tpid);
extern int _bcm_trx_vp_tpid_get(int unit, bcm_gport_t vport, uint16 *tpid);
extern int _bcm_trx_vp_tpid_get_all(int unit, bcm_gport_t vport, int size,
                         uint16 *tpid_array, int *color_array, int *count);

extern int _bcm_trx_port_cml_hw2flags(int unit, uint32 val, uint32 *flgas);
extern int _bcm_trx_port_cml_flags2hw(int unit, uint32 flags, uint32 *val);

extern int _bcm_trx_defip128_used_count_get(int unit);
extern int _bcm_trx_defip128_idx_max_get(int unit);
extern int _bcm_trx_defip128_array_idx_prefix_len_get(int unit, int arr_idx);
extern int _field_trx_meter_table_get(int unit, _field_stage_id_t stage_id,
                                      soc_mem_t *mem_x, soc_mem_t *mem_y);
#endif /* BCM_TRX_SUPPORT */
#if defined(BCM_TRIDENT_SUPPORT)
extern int _bcm_field_trident_stat_hw_mode_to_bmap(int unit, uint8 mode, 
                                               _field_stage_id_t stage_id, 
                                               uint32 *hw_bmap,
                                               uint8 *hw_entry_count);
#endif /* BCM_TRIDENT_SUPPORT */

#if defined(BCM_TRIDENT2_SUPPORT)

extern int _bcm_trx_ing_vlan_action_profile_entry_no_mod_add(
                               int                                 unit, 
                               ing_vlan_tag_action_profile_entry_t *entry, 
                               uint32                              *index);
extern int _bcm_trx_egr_vlan_action_profile_entry_no_mod_add(
                               int                                 unit, 
                               egr_vlan_tag_action_profile_entry_t *entry, 
                               uint32                              *index);
#endif /* BCM_TRIDENT2_SUPPORT */

extern int _bcm_trx_system_reserved_vlan_ing_set(int unit, int arg);
extern int _bcm_trx_system_reserved_vlan_egr_set(int unit, int arg);
extern int _bcm_trx_system_reserved_vlan_ing_get(int unit, int *arg);


#endif  /* !_BCM_INT_TRX_H_ */
