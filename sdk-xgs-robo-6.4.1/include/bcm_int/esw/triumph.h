/*
 * $Id: triumph.h,v 1.143 Broadcom SDK $
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
 * File:        triumph.h
 * Purpose:     Function declarations for Triumph bcm functions
 */

#ifndef _BCM_INT_TRIUMPH_H_
#define _BCM_INT_TRIUMPH_H_
#include <bcm_int/esw/l2.h>
#include <bcm_int/esw/mbcm.h>
#include <bcm/qos.h>
#ifdef BCM_FIELD_SUPPORT 
#include <bcm_int/esw/field.h>
#endif /* BCM_FIELD_SUPPORT */
#if defined(BCM_TRX_SUPPORT)
/*
 * Routines for internal use
 */
extern int _bcm_tr_l2_traverse_mem(int unit, soc_mem_t mem, 
                                   _bcm_l2_traverse_t *trav_st);
extern int _bcm_tr_l2_from_l2x(int unit,
                     bcm_l2_addr_t *l2addr, l2x_entry_t *l2x_entry);
extern int _bcm_tr_l2_from_ext_l2(int unit, 
                   bcm_l2_addr_t *l2addr, ext_l2_entry_entry_t *ext_l2_entry);
extern int _bcm_tr_l2_to_l2x(int unit, l2x_entry_t *l2x_entry,
                             bcm_l2_addr_t *l2addr, int key_only);

/****************************************************************
 *
 * Triumph functions
 *
 ****************************************************************/
extern int bcm_tr_l2_init(int unit);
extern int bcm_tr_l2_term(int unit);
extern int bcm_tr_l2_addr_get(int unit, bcm_mac_t mac_addr, bcm_vlan_t vid,
                              bcm_l2_addr_t *l2addr);
extern int bcm_tr_l2_addr_add(int unit, bcm_l2_addr_t *l2addr);
extern int bcm_tr_l2_addr_delete(int unit, bcm_mac_t mac, bcm_vlan_t vid);
extern int bcm_tr_l2_addr_delete_by_mac(int unit, bcm_mac_t mac, uint32 flags);
extern int bcm_tr_l2_addr_delete_by_mac_port(int unit, bcm_mac_t mac,
                                             bcm_module_t mod, bcm_port_t port,
                                             uint32 flags);
extern int bcm_tr_l2_conflict_get(int unit, bcm_l2_addr_t *addr,
                                  bcm_l2_addr_t *cf_array, int cf_max,
                                  int *cf_count);
extern int bcm_tr_l2_cross_connect_add(int unit, bcm_vlan_t outer_vlan, 
                                       bcm_vlan_t inner_vlan,
                                       bcm_gport_t port_1, bcm_gport_t port_2);
extern int bcm_tr_l2_cross_connect_delete(int unit, bcm_vlan_t outer_vlan, 
                                          bcm_vlan_t inner_vlan);
extern int bcm_tr_l2_cross_connect_delete_all(int unit);
extern int bcm_tr_l2_cross_connect_traverse(int unit,
                                            bcm_vlan_cross_connect_traverse_cb cb,
                                            void *user_data);
extern int _bcm_tr_l2_replace_by_hw(int unit, _bcm_l2_replace_t *rep_st);

#if defined(BCM_TRIUMPH_SUPPORT) /* BCM_TRIUMPH_SUPPORT */
/* Add an entry to L2 Station Table. */
extern int bcm_tr_l2_station_add(int unit,
                                 int *station_id,
                                 bcm_l2_station_t *station);

/* Delete an entry from L2 Station Table. */
extern int bcm_tr_l2_station_delete(int unit, int station_id);

/* Clear all L2 Station Table entries. */
extern int bcm_tr_l2_station_delete_all( int unit);

/* Get L2 station entry detail from Station Table. */
extern int bcm_tr_l2_station_get(int unit,
                              int station_id,
                              bcm_l2_station_t *station);

/* Get size of L2 Station Table. */
extern int bcm_tr_l2_station_size_get(int unit, int *size);

/* L2 station support init routine. */
extern int _bcm_tr_l2_station_control_init(int unit);

/* Deallocate L2 station control resources. */
extern int _bcm_tr_l2_station_control_deinit(int unit);

#endif /* !BCM_TRIUMPH_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_tr_l2_reload_mbi(int unit);
extern int _bcm_tr_qos_sync(int unit);
extern int _bcm_tr_mpls_match_key_recover(int unit);
extern int _bcm_tr_mpls_flex_stat_recover(int unit, int vp);
extern int _bcm_tr_mpls_source_vp_tpid_recover(int unit, int vp);
extern int _bcm_tr_mpls_associated_data_recover(int unit);
extern void _bcm_tr_mpls_label_flex_stat_recover(int unit, mpls_entry_entry_t *ment);

/* L2 station module warmboot recovery routine. */
extern int _bcm_tr_l2_station_reload(int unit);
#else
#define _bcm_tr_l2_reload_mbi(_u)    (BCM_E_UNAVAIL)
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_tr_l2_sw_dump(int unit);
extern void _bcm_tr_qos_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

extern int _bcm_trx_vlan_port_default_action_set(int unit, bcm_port_t port,
                                                bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_port_default_action_get(int unit, bcm_port_t port,
                                                bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_port_default_action_delete(int unit, bcm_port_t port);
extern int _bcm_trx_vlan_port_egress_default_action_set(int unit, bcm_port_t port,
                                                bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_port_egress_default_action_get(int unit, bcm_port_t port,
                                                       bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_port_egress_default_action_delete(int unit, bcm_port_t port);
extern int _bcm_trx_vlan_port_protocol_action_add(int unit, bcm_port_t port,
                                                 bcm_port_frametype_t frame,
                                                 bcm_port_ethertype_t ether,
                                                       bcm_vlan_action_set_t *action);
extern int _bcm_trx_vlan_port_protocol_action_get(int unit, bcm_port_t port,
                                                 bcm_port_frametype_t frame,
                                                 bcm_port_ethertype_t ether,
                                                 bcm_vlan_action_set_t *action);

extern int _bcm_trx_vlan_port_protocol_delete(int unit, bcm_port_t port,
                                             bcm_port_frametype_t frame,
                                             bcm_port_ethertype_t ether);
extern int _bcm_trx_vlan_port_protocol_delete_all(int unit, bcm_port_t port);
extern int _bcm_trx_vlan_port_protocol_action_traverse(int unit, 
                                bcm_vlan_port_protocol_action_traverse_cb cb,
                                           void *user_data);

extern int bcm_tr_port_rate_pause_get(int unit, bcm_port_t port,
                                      uint32 *kbits_pause,
                                      uint32 *kbits_resume);
extern int bcm_tr_port_rate_egress_set(int unit, bcm_port_t port,
                                       uint32 kbits_sec,
                                       uint32 kbits_burst, uint32 mode);
extern int bcm_tr_port_rate_egress_get(int unit, bcm_port_t port,
                                       uint32 *kbits_sec,
                                       uint32 *kbits_burst, uint32 *mode);
extern int bcm_tr_port_pps_rate_egress_set(int unit, bcm_port_t port,
                                           uint32 pps, uint32 burst);
extern int bcm_tr_port_pps_rate_egress_get(int unit, bcm_port_t port,
                                           uint32 *pps, uint32 *burst);
extern int bcm_tr_qos_init(int unit);
extern int bcm_tr_qos_detach(int unit);
extern int bcm_tr_qos_map_create(int unit, uint32 flags, int *map_id);
extern int bcm_tr_qos_map_destroy(int unit, int map_id);
extern int bcm_tr_qos_map_add(int unit, uint32 flags, bcm_qos_map_t *map, 
                               int map_id);
extern int bcm_tr_qos_map_delete(int unit, uint32 flags, 
                                  bcm_qos_map_t *map, int map_id);
extern int bcm_tr_qos_multi_get(int unit, int array_size, int *map_ids_array,
                                 int *flags_array, int *array_count);
extern int bcm_tr_qos_map_multi_get(int unit, uint32 flags,
                                     int map_id, int array_size, 
                                     bcm_qos_map_t *array, int *array_count);
extern int bcm_tr_qos_port_map_set(int unit, bcm_gport_t port, int ing_map, 
                                    int egr_map);
extern int _bcm_tr_qos_id2idx(int unit, int map_id, int *hw_idx);
extern int _bcm_tr_qos_idx2id(int unit, int hw_idx, int type, int *map_id);


#ifdef BCM_FIELD_SUPPORT 
extern int _bcm_field_tr_init(int unit, _field_control_t *fc);
extern int _bcm_field_tr_external_tcam_key_mask_get(int unit, 
                                                    _field_entry_t *f_ent);
extern int _bcm_field_tr_multipath_hashcontrol_get(int unit, int *arg);
extern int _bcm_field_tr_multipath_hashcontrol_set(int unit, int arg);
extern int _bcm_field_tr_external_entry_move(int unit, 
                                                  _field_entry_t *f_ent,
                                                  int index1, int index2);
extern int _bcm_field_tr_external_group_remove(int unit, _field_group_t *fg);
extern int _bcm_field_tr_external_counter_idx_get(int unit, 
                                                  uint8  slice_number, 
                                                  bcm_field_entry_t entry_index, 
                                                  int  *idx);
extern int _bcm_field_tr_external_counters_collect(int unit, 
                                                   _field_stage_t *stage_fc);
extern int _bcm_field_tr_external_policy_mem_get(int unit, 
                                                 _field_action_t *fa, 
                                                 soc_mem_t *mem);
extern int _bcm_field_tr_external_mode_set(int unit, uint8 slice_numb, 
                                           _field_group_t *fg, uint8 flags);
extern int _bcm_field_tr_external_slice_clear(int unit, _field_group_t *fg);
extern int _bcm_field_tr_external_init(int unit, _field_stage_t *stage_fc);
extern int _bcm_field_tr_hw_clear(int unit, _field_stage_t *stage_fc);
extern int _bcm_field_tr_detach(int unit, _field_control_t *fc);
extern int _bcm_field_tr_selcode_get(int unit, _field_stage_t *stage_fc, 
                                     bcm_field_qset_t *qset_req, 
                                     _field_group_t *fg);
extern int _bcm_field_tr_entry_install(int unit, _field_entry_t *f_ent,
                                       int tcam_idx);
extern int _bcm_field_tr_entry_reinstall(int unit, _field_entry_t *f_ent,
                                       int tcam_idx);
extern int _bcm_field_tr_write_slice_map(int unit, _field_stage_t *stage_fc);
extern int _bcm_field_tr_counter_get(int unit, _field_stage_t *stage_fc, 
                                     soc_mem_t counter_x_mem, uint32 *mem_x_buf, 
                                     soc_mem_t counter_y_mem, uint32 *mem_y_buf,
                                     int idx, uint64 *packet_count, 
                                     uint64 *byte_count);
extern int _bcm_field_tr_counter_set(int unit, _field_stage_t *stage_fc,
                                     soc_mem_t counter_x_mem, uint32 *mem_x_buf,
                                     soc_mem_t counter_y_mem, uint32 *mem_y_buf,
                                     int idx, uint64 *packet_count, 
                                     uint64 *byte_count);
extern int _bcm_field_tr_external_entry_install(int unit, 
                                                _field_entry_t *f_ent);
extern int _bcm_field_tr_external_entry_reinstall(int unit, 
                                                _field_entry_t *f_ent);
extern int _bcm_field_tr_external_entry_remove(int unit, _field_entry_t *f_ent);
extern int _bcm_field_tr_external_entry_prio_set(int unit, _field_entry_t *f_ent, 
                                                 int prio);
#endif /* BCM_FIELD_SUPPORT */

#ifdef INCLUDE_L3

struct _bcm_l3_trvrs_data_s;
extern int _bcm_tr_l3_enable(int unit, bcm_port_t port, uint32 flags, int enable);
extern int _bcm_tr_l3_ingress_interface_clr(int unit, int intf_id);
extern int _bcm_tr_l3_intf_class_set(int unit, bcm_vlan_t vid, uint32 intf_class);
extern int _bcm_tr_l3_intf_nat_realm_id_get(int unit, bcm_vlan_t vid, 
                                            int *realm_id);
extern int _bcm_tr_l3_intf_nat_realm_id_set(int unit, bcm_vlan_t vid, 
                                            int realm_id);
extern int _bcm_tr_l3_intf_vrf_get(int unit, bcm_vlan_t vid, bcm_vrf_t *vrf);
extern int _bcm_tr_l3_intf_vrf_bind(int unit, bcm_vlan_t vid, bcm_vrf_t vrf);
extern int _bcm_tr_l3_intf_global_route_enable_get(int unit, bcm_vlan_t vid, 
                                                   int *enable);
extern int _bcm_tr_l3_intf_global_route_enable_set(int unit, bcm_vlan_t vid, 
                                                   int enable); 
extern int _bcm_tr_l3_ipmc_get(int unit, _bcm_l3_cfg_t *l3cfg);
extern int _bcm_tr_l3_ipmc_add(int unit, _bcm_l3_cfg_t *l3cfg);
extern int _bcm_tr_l3_ipmc_del(int unit, _bcm_l3_cfg_t *l3cfg);
extern int _bcm_tr_l3_ipmc_get_by_idx(int unit, void *dma_ptr, int idx,
                                      _bcm_l3_cfg_t *l3cfg);
extern int _bcm_tr_l3_defip_mem_get(int unit, uint32 flags, 
                                    int plen, soc_mem_t *mem);
extern void _bcm_tr_l3_ipmc_ent_init(int unit, uint32 *l3x_entry,
                                     _bcm_l3_cfg_t *l3cfg);
extern int _bcm_tr_defip_init(int unit);
extern int _bcm_tr_defip_deinit(int unit);
extern int _bcm_tr_l3_intf_mtu_get(int unit, _bcm_l3_intf_cfg_t *intf_info);
extern int _bcm_tr_l3_intf_mtu_set(int unit, _bcm_l3_intf_cfg_t *intf_info);
extern int _bcm_tr_ext_lpm_init(int unit, soc_mem_t mem);
extern int _bcm_tr_ext_lpm_deinit(int unit, soc_mem_t mem);
extern int _bcm_tr_ext_lpm_add(int unit, _bcm_defip_cfg_t *data, 
                               int nh_ecmp_idx);
extern int _bcm_tr_ext_lpm_delete(int unit, _bcm_defip_cfg_t *key);
extern int _bcm_tr_ext_lpm_match(int unit, _bcm_defip_cfg_t *key, 
                                 int *next_hop_index);
extern int _bcm_tr_defip_traverse(int unit, struct _bcm_l3_trvrs_data_s *trv_data);

extern int bcm_tr_ipmc_init(int unit);
extern int bcm_tr_ipmc_detach(int unit);
extern int bcm_tr_ipmc_get(int unit, int index, bcm_ipmc_addr_t *ipmc);
extern int bcm_tr_ipmc_lookup(int unit, int *index, bcm_ipmc_addr_t *ipmc);
extern int bcm_tr_ipmc_add(int unit, bcm_ipmc_addr_t *ipmc);
extern int bcm_tr_ipmc_put(int unit, int index, bcm_ipmc_addr_t *ipmc);
extern int bcm_tr_ipmc_delete(int unit, bcm_ipmc_addr_t *ipmc);
extern int bcm_tr_ipmc_delete_all(int unit);
extern int bcm_tr_ipmc_age(int unit, uint32 flags, bcm_ipmc_traverse_cb age_cb,
                           void *user_data);
extern int bcm_tr_ipmc_traverse(int unit, uint32 flags,
                                bcm_ipmc_traverse_cb cb, void *user_data);
extern int bcm_tr_ipmc_enable(int unit, int enable);
extern int bcm_tr_ipmc_src_port_check(int unit, int enable);
extern int bcm_tr_ipmc_src_ip_search(int unit, int enable);
extern int bcm_tr_ipmc_egress_port_set(int unit, bcm_port_t port,
                                       const bcm_mac_t mac, int untag,
                                       bcm_vlan_t vid, int ttl_threshold);
extern int bcm_tr_ipmc_egress_port_get(int unit, bcm_port_t port,
                                       sal_mac_addr_t mac, int *untag,
                                       bcm_vlan_t *vid, int *ttl_threshold);

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_tr_ipmc_reinit(int unit);
#else
#define _bcm_tr_ipmc_reinit(_u)    (BCM_E_UNAVAIL)
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_tr_ipmc_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

extern int bcm_tr_mpls_lock (int unit);
extern void bcm_tr_mpls_unlock(int unit);
extern int bcm_tr_mpls_init(int unit);
extern int bcm_tr_mpls_cleanup(int unit);
#ifdef BCM_WARM_BOOT_SUPPORT
extern int bcm_tr_mpls_sync(int unit);
#endif
extern int bcm_tr_mpls_vpn_id_create(int unit, bcm_mpls_vpn_config_t *info);
extern int bcm_tr_mpls_vpn_id_destroy(int unit, bcm_vpn_t vpn);
extern int bcm_tr_mpls_vpn_id_get( int unit, bcm_vpn_t  vpn, bcm_mpls_vpn_config_t *info);
extern int bcm_tr_mpls_vpn_id_destroy_all(int unit);
extern int bcm_tr_mpls_vpn_traverse(int unit, bcm_mpls_vpn_traverse_cb cb, void *user_data);
extern int bcm_tr_mpls_port_add(int unit, bcm_vpn_t vpn, 
                                bcm_mpls_port_t *mpls_port);
extern int bcm_tr_mpls_port_delete(int unit, bcm_vpn_t vpn, bcm_gport_t mpls_port_id);
extern int bcm_tr_mpls_port_delete_all(int unit, bcm_vpn_t vpn);
extern int bcm_tr_mpls_port_get(int unit, bcm_vpn_t vpn, 
                                bcm_mpls_port_t *mpls_port);
extern int bcm_tr_mpls_port_get_all(int unit, bcm_vpn_t vpn, int port_max,
                                    bcm_mpls_port_t *port_array, int *port_count);
extern int bcm_tr_mpls_tunnel_initiator_set(int unit, bcm_if_t intf, int num_labels,
                                            bcm_mpls_egress_label_t *label_array);
extern int bcm_tr_mpls_tunnel_initiator_clear(int unit, bcm_if_t intf);
extern int bcm_tr_mpls_tunnel_initiator_get(int unit, bcm_if_t intf, int label_max,
                                            bcm_mpls_egress_label_t *label_array,
                                            int *label_count);
extern int bcm_tr_mpls_tunnel_initiator_clear_all(int unit);
extern int bcm_tr_mpls_tunnel_switch_add(int unit, bcm_mpls_tunnel_switch_t *info);
extern int bcm_tr_mpls_tunnel_switch_delete(int unit, bcm_mpls_tunnel_switch_t *info);
extern int bcm_tr_mpls_tunnel_switch_delete_all(int unit);
extern int bcm_tr_mpls_tunnel_switch_get(int unit, bcm_mpls_tunnel_switch_t *info);
extern int bcm_tr_mpls_tunnel_switch_traverse(int unit,
                                              bcm_mpls_tunnel_switch_traverse_cb cb,
                                              void *user_data);
extern int bcm_tr_mpls_exp_map_create(int unit, uint32 flags, int *exp_map_id);
extern int bcm_tr_mpls_exp_map_destroy(int unit, int exp_map_id);
extern int bcm_tr_mpls_exp_map_destroy_all(int unit);
extern int bcm_tr_mpls_exp_map_set(int unit, int exp_map_id,
                                   bcm_mpls_exp_map_t *exp_map);
extern int bcm_tr_mpls_exp_map_get(int unit, int exp_map_id,
                                   bcm_mpls_exp_map_t *exp_map);
extern int bcm_tr_mpls_label_stat_get(int unit, int sync_mode, 
                                      bcm_mpls_label_t label,
                                      bcm_gport_t port,
                                      bcm_mpls_stat_t stat, 
                                      uint64 *val);
extern int bcm_tr_mpls_label_stat_get32(int unit, int sync_mode, 
                                        bcm_mpls_label_t label, 
                                        bcm_gport_t port,
                                        bcm_mpls_stat_t stat, 
                                        uint32 *val);
extern int bcm_tr_mpls_label_stat_clear(int unit, bcm_mpls_label_t label, bcm_gport_t port,
                                    bcm_mpls_stat_t stat);
extern int _bcm_tr_mpls_port_resolve(int unit, bcm_gport_t mpls_port,
                                     bcm_module_t *modid, bcm_port_t *port,
                                     bcm_trunk_t *trunk_id, int *id);
extern int bcm_tr_mpls_port_learn_set(int unit, bcm_gport_t port, uint32 flags);
extern int bcm_tr_mpls_port_learn_get(int unit, bcm_gport_t port, uint32 *flags);
extern int bcm_tr_mpls_mcast_flood_set(int unit, bcm_vlan_t vlan,
                                       bcm_vlan_mcast_flood_t mode);
extern int bcm_tr_mpls_mcast_flood_get(int unit, bcm_vlan_t vlan,
                                       bcm_vlan_mcast_flood_t *mode);
extern int bcm_tr_mpls_port_phys_gport_get(int unit, int vp, bcm_gport_t *gp);
extern int bcm_tr_mpls_l3_nh_info_add(int unit, bcm_mpls_tunnel_switch_t *info, int *nh_index);
extern int bcm_tr_mpls_l3_nh_info_delete(int unit, int nh_index);
extern int bcm_tr_mpls_l3_nh_info_get(int unit, bcm_mpls_tunnel_switch_t *info, int nh_index);
extern int bcm_tr_mpls_get_vp_nh (int unit, int vp_index, bcm_if_t *egress_if);
extern int bcm_tr_mpls_egress_entry_modify(int unit, int nh_index, int new_entry_type);
extern int bcm_tr_mpls_egress_entry_mac_replace(int unit, int nh_index, bcm_l3_egress_t *egr);
extern void bcm_tr_mpls_pw_used_clear(int unit, int pw_cnt);
extern int bcm_tr_mpls_pw_used_get(int unit, int pw_cnt);
extern void bcm_tr_mpls_pw_used_set(int unit, int pw_cnt);
extern int bcm_tr_mpls_vrf_used_get(int unit, int vrf);
extern int bcm_tr_mpls_port_independent_range (int unit, bcm_mpls_label_t label, bcm_gport_t port );
extern int bcm_tr_mpls_tunnel_intf_valid (int unit, int nh_index);
extern int bcm_tr_mpls_get_label_action (int unit, int nh_index, uint32 *label_action );
extern int _bcm_tr_mpls_match_delete(int unit, int vp);
extern int _bcm_tr_mpls_match_add(int unit, bcm_mpls_port_t *mpls_port, int vp);
extern int _bcm_esw_mpls_match_delete(int unit, int vp);
extern int _bcm_esw_mpls_match_add(int unit, bcm_mpls_port_t *mpls_port, int vp);
extern int _bcm_tr_mpls_match_get(int unit, bcm_mpls_port_t *mpls_port, int vp);
extern int _bcm_esw_mpls_match_get(int unit, bcm_mpls_port_t *mpls_port, int vp);
extern int  _bcm_esw_mpls_tunnel_switch_delete_all (int unit);
extern int bcm_tr_mpls_port_untagged_profile_set (int unit, bcm_port_t port);
extern int bcm_tr_mpls_port_untagged_profile_reset(int unit, bcm_port_t port);
extern void bcm_tr_mpls_match_clear (int unit, int vp);
extern void  bcm_tr_mpls_port_match_count_adjust(int unit, int vp, int step);
extern int  bcm_tr_mpls_match_trunk_add(int unit, bcm_trunk_t tgid, int vp);
extern int bcm_tr_mpls_match_trunk_delete(int unit, bcm_trunk_t tgid, int vp);
extern int _bcm_tr_mpls_vpless_failover_nh_index_find (int unit, int vp,
                            int primary_nh_index, int *fo_nh_index);

extern int _bcm_tr_vlan_translate_vp_add(int unit,
                                          bcm_gport_t port,
                                          bcm_vlan_translate_key_t key_type,
                                          bcm_vlan_t outer_vlan,
                                          bcm_vlan_t inner_vlan,
                                          int vp,
                                          bcm_vlan_action_set_t *action);
extern int _bcm_tr_vlan_translate_vp_delete(int unit,
                                          bcm_gport_t port,
                                          bcm_vlan_translate_key_t key_type,
                                          bcm_vlan_t outer_vlan,
                                          bcm_vlan_t inner_vlan,
                                          int vp);

extern int bcm_tr_subport_init(int unit);
extern int bcm_tr_subport_cleanup(int unit);
extern int bcm_tr_subport_group_create(int unit, bcm_subport_group_config_t *config,
                                       bcm_gport_t *group);
extern int bcm_tr_subport_group_destroy(int unit, bcm_gport_t group);
extern int bcm_tr_subport_group_get(int unit, bcm_gport_t group,
                                    bcm_subport_group_config_t *config);
extern int bcm_tr_subport_port_add(int unit, bcm_subport_config_t *config,
                                   bcm_gport_t *port);
extern int bcm_tr_subport_port_delete(int unit, bcm_gport_t port);
extern int bcm_tr_subport_port_get(int unit, bcm_gport_t port,
                                   bcm_subport_config_t *config);
extern int bcm_tr_subport_port_traverse(int unit, bcm_subport_port_traverse_cb cb,
                                        void *user_data);
extern int _bcm_tr_subport_group_resolve(int unit, bcm_gport_t gport,
                                         bcm_module_t *modid, bcm_port_t *port,
                                         bcm_trunk_t *trunk_id, int *id);
extern int _bcm_tr_subport_port_resolve(int unit, bcm_gport_t gport,
                                        bcm_module_t *modid, bcm_port_t *port,
                                        bcm_trunk_t *trunk_id, int *id);
extern int bcm_tr_subport_learn_set(int unit, bcm_gport_t port, uint32 flags);
extern int bcm_tr_subport_learn_get(int unit, bcm_gport_t port, uint32 *flags);
extern int _bcm_tr_subport_group_find(int unit, int vp, int *grp);
extern int bcm_tr_multicast_vpls_encap_get(int unit, bcm_multicast_t group, bcm_gport_t port,
                                           bcm_gport_t mpls_port_id, bcm_if_t *encap_id);
extern int bcm_tr_multicast_subport_encap_get(int unit, bcm_multicast_t group, bcm_gport_t port,
                                              bcm_gport_t subport, bcm_if_t *encap_id);
extern int bcm_trx_multicast_egress_delete_all(int unit, bcm_multicast_t group);
extern int bcm_trx_multicast_egress_set(int unit, bcm_multicast_t group,
                                       int port_count, bcm_gport_t *port_array,
                                       bcm_if_t *encap_array);
extern int bcm_tr_mpls_swap_nh_info_add(int unit, bcm_l3_egress_t *egr, int nh_index, uint32 flags);
extern int bcm_tr_mpls_swap_nh_info_delete(int unit, int nh_index);
extern int bcm_tr_mpls_swap_nh_info_get(int unit, bcm_l3_egress_t *egr, int nh_index);
extern int bcm_tr_mpls_update_vp_nh ( int unit, int index);
extern int bcm_tr_mpls_l3_label_add (int unit, bcm_l3_egress_t *egr,  int index, uint32 flags);
extern int bcm_tr_mpls_l3_label_delete (int unit, int index);
extern int bcm_tr_mpls_l3_label_get (int unit, int index, bcm_l3_egress_t *nh_info);
extern int bcm_tr_mpls_get_entry_type (int unit, int nh_index, uint32 *entry_type );
extern void bcm_tr_mpls_entry_internal_qos_set(int unit, bcm_mpls_port_t *mpls_port, 
                                                                          bcm_mpls_tunnel_switch_t  *info, mpls_entry_entry_t *ment);

#endif /* INCLUDE_L3 */
extern int bcm_tr_cosq_init(int unit);
extern int bcm_tr_cosq_detach(int unit, int software_state_only);
extern int bcm_tr_cosq_config_set(int unit, int numq);
extern int bcm_tr_cosq_config_get(int unit, int *numq);
extern int bcm_tr_cosq_mapping_set(int unit, bcm_port_t port,
                                   bcm_cos_t priority, bcm_cos_queue_t cosq);
extern int bcm_tr_cosq_mapping_get(int unit, bcm_port_t port,
                                   bcm_cos_t priority, bcm_cos_queue_t *cosq);
extern int bcm_tr_cosq_port_bandwidth_set(int unit, bcm_port_t port,
                                          bcm_cos_queue_t cosq,
                                          uint32 kbits_sec_min,
                                          uint32 kbits_sec_max,
                                          uint32 kbits_sec_burst,
                                          uint32 flags);
extern int bcm_tr_cosq_port_bandwidth_get(int unit, bcm_port_t port,
                                          bcm_cos_queue_t cosq,
                                          uint32 *kbits_sec_min,
                                          uint32 *kbits_sec_max,
                                          uint32 *kbits_sec_burst,
                                          uint32 *flags);
extern int bcm_tr_cosq_port_pps_set(int unit, bcm_port_t port,
                                    bcm_cos_queue_t cosq, int pps);
extern int bcm_tr_cosq_port_pps_get(int unit, bcm_port_t port,
                                    bcm_cos_queue_t cosq, int *pps);
extern int bcm_tr_cosq_port_burst_set(int unit, bcm_port_t port,
                                      bcm_cos_queue_t cosq, int burst);
extern int bcm_tr_cosq_port_burst_get(int unit, bcm_port_t port,
                                      bcm_cos_queue_t cosq, int *burst);
extern int bcm_tr_cosq_discard_set(int unit, uint32 flags);
extern int bcm_tr_cosq_discard_get(int unit, uint32 *flags);
extern int bcm_tr_cosq_discard_port_set(int unit, bcm_port_t port,
                                        bcm_cos_queue_t cosq, uint32 color,
                                        int drop_start, int drop_slope,
                                        int average_time);
extern int bcm_tr_cosq_discard_port_get(int unit, bcm_port_t port,
                                        bcm_cos_queue_t cosq, uint32 color,
                                        int *drop_start, int *drop_slope,
                                        int *average_time);
extern int bcm_tr_cosq_gport_discard_set(int unit, bcm_gport_t port, 
                                         bcm_cos_queue_t cosq,
                                         bcm_cosq_gport_discard_t *discard);
extern int bcm_tr_cosq_gport_discard_get(int unit, bcm_gport_t port, 
                                         bcm_cos_queue_t cosq,
                                         bcm_cosq_gport_discard_t *discard);
extern int bcm_tr_cosq_port_sched_set(int unit, bcm_pbmp_t pbm,
                                      int mode, const int weights[], int delay);
extern int bcm_tr_cosq_port_sched_get(int unit, bcm_pbmp_t pbm,
                                      int *mode, int weights[], int *delay);
extern int bcm_tr_cosq_sched_weight_max_get(int unit, int mode,
                                            int *weight_max);
extern int bcm_tr_cosq_gport_add(int unit, bcm_gport_t port, int numq,
                                 uint32 flags, bcm_gport_t *gport);
extern int bcm_tr_cosq_gport_delete(int unit, bcm_gport_t gport);
extern int bcm_tr_cosq_gport_traverse(int unit, bcm_cosq_gport_traverse_cb cb,
                                      void *user_data);

extern int bcm_tr_cosq_gport_bandwidth_set(int unit, bcm_gport_t gport,
                                           bcm_cos_queue_t cosq, 
                                           uint32 kbits_sec_min,
                                           uint32 kbits_sec_max,
                                           uint32 flags);
extern int bcm_tr_cosq_gport_bandwidth_get(int unit, bcm_gport_t gport,
                                           bcm_cos_queue_t cosq, 
                                           uint32 *kbits_sec_min,
                                           uint32 *kbits_sec_max,
                                           uint32 *flags);
extern int bcm_tr_cosq_gport_bandwidth_burst_set(int unit, bcm_gport_t gport,
                                                 bcm_cos_queue_t cosq, 
                                                 uint32 kbits_burst);
extern int bcm_tr_cosq_gport_bandwidth_burst_get(int unit, bcm_gport_t gport,
                                                 bcm_cos_queue_t cosq, 
                                                 uint32 *kbits_burst);
extern int bcm_tr_cosq_gport_sched_set(int unit, bcm_gport_t gport,
                                       bcm_cos_queue_t cosq, int mode,
                                       int weight);
extern int bcm_tr_cosq_gport_sched_get(int unit, bcm_gport_t gport, 
                                       bcm_cos_queue_t cosq, int *mode,
                                       int *weight);
extern int bcm_tr_cosq_gport_attach(int unit, bcm_gport_t sched_gport, 
                                    bcm_cos_queue_t cosq, bcm_gport_t input_gport);
extern int bcm_tr_cosq_gport_detach(int unit, bcm_gport_t sched_gport,
                                    bcm_cos_queue_t cosq, bcm_gport_t input_gport);
extern int bcm_tr_cosq_gport_attach_get(int unit, bcm_gport_t sched_gport,
                                        bcm_cos_queue_t *cosq, bcm_gport_t *input_gport);
extern int _bcm_tr_cosq_port_resolve(int unit, bcm_gport_t gport,
                                     bcm_module_t *modid, bcm_port_t *port,
                                     bcm_trunk_t *trunk_id, int *id);

extern int bcm_bfd_l2_hash_dynamic_replace(int unit, l2x_entry_t *l2x_entry);
#ifdef BCM_WARM_BOOT_SUPPORT
extern int bcm_tr_cosq_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void bcm_tr_cosq_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif  /* BCM_TRX_SUPPORT */
#endif  /* !_BCM_INT_TRIUMPH_H_ */
