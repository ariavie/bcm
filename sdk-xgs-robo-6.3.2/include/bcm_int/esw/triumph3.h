/*
 * $Id: triumph3.h 1.120 Broadcom SDK $
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
 * All Rights Reserved.$
 *
 * File:        triumph3.h
 * Purpose:     Function declarations for Triumph3 Internal functions.
 */

#ifndef _BCM_INT_TRIUMPH3_H_
#define _BCM_INT_TRIUMPH3_H_

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_HURRICANE2_SUPPORT) || defined(BCM_KATANA2_SUPPORT)

#include <bcm_int/esw/mbcm.h>
#include <soc/tnl_term.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/cosq.h>
#include <bcm_int/esw/oam.h>
#include <bcm/failover.h>
#include <bcm/oam.h>
#include <bcm_int/esw/flex_ctr.h>

#define _BCM_TR3_L3_PORT_MASK        0x7f
#define _BCM_TR3_L3_MODID_MASK       0xff
#define _BCM_TR3_L3_MODID_SHIFT      0x7
#define _BCM_TR3_L3_DEFAULT_MTU_SIZE 0x3fff
#define _BCM_TR3_DEFIP_TCAM_DEPTH    1024
#define _BCM_HX4_RANGER_DEFIP_TCAM_DEPTH 128

/* A six bit marker of alternating 1 and 0 used as an
 * invalid entry marker for ESM host entries. This is used
 * in recovering the host free entry pointers as a work-around
 * for unavailablility of soc_tr3_get_vbit() function */ 
#define _TR3_ESM_HOST_ENTRY_INVALID_MARKER 0x2a

/* L2 Internal functions */
extern int bcm_tr3_l2_init(int unit);
extern int bcm_tr3_l2_detach(int unit);
extern int bcm_tr3_l2_addr_add(int unit, bcm_l2_addr_t *l2addr);
extern int bcm_tr3_l2_addr_delete(int unit, bcm_mac_t mac, bcm_vlan_t vid);
extern int bcm_tr3_l2_addr_delete_by_port(int unit,
                                          bcm_module_t mod, bcm_port_t port,
                                          uint32 flags);
extern int bcm_tr3_l2_addr_delete_by_mac(int unit, bcm_mac_t mac,
                                         uint32 flags);
extern int bcm_tr3_l2_addr_delete_by_vlan(int unit, bcm_vlan_t vid,
                                          uint32 flags);
extern int bcm_tr3_l2_addr_delete_by_trunk(int unit, bcm_trunk_t tid,
                                           uint32 flags);
extern int bcm_tr3_l2_addr_delete_by_mac_port(int unit, bcm_mac_t mac,
                                              bcm_module_t mod,
                                              bcm_port_t port,
                                              uint32 flags);
extern int bcm_tr3_l2_addr_delete_by_vlan_port(int unit, bcm_vlan_t vid,
                                               bcm_module_t mod,
                                               bcm_port_t port,
                                               uint32 flags);
extern int bcm_tr3_l2_addr_delete_by_vlan_trunk(int unit, bcm_vlan_t vid,
                                                bcm_trunk_t tid,
                                                uint32 flags);
extern int bcm_tr3_l2_addr_delete_mcast(int unit, int bulk);
extern int bcm_tr3_l2_addr_get(int unit, sal_mac_addr_t mac,
                               bcm_vlan_t vid, bcm_l2_addr_t *l2addr);

extern int bcm_tr3_l2_port_native(int unit, int modid, int port);
extern int bcm_tr3_l2_addr_register(int unit,
                                    bcm_l2_addr_callback_t fn,
                                    void *fn_data);
extern int bcm_tr3_l2_addr_unregister(int unit,
                                      bcm_l2_addr_callback_t fn,
                                      void *fn_data);
extern int bcm_tr3_l2_age_timer_set(int unit, int age_seconds);
extern int bcm_tr3_l2_age_timer_get(int unit, int *age_seconds);
extern int bcm_tr3_l2_tunnel_add(int unit, bcm_mac_t mac, bcm_vlan_t vlan);
extern int bcm_tr3_l2_tunnel_delete(int unit, bcm_mac_t mac, bcm_vlan_t vlan);
extern int bcm_tr3_l2_tunnel_delete_all(int unit);
extern int bcm_tr3_l2_clear(int unit);
extern int bcm_tr3_l2_learn_limit_set(int unit, bcm_l2_learn_limit_t *limit);
extern int bcm_tr3_l2_learn_limit_get(int unit, bcm_l2_learn_limit_t *limit);
extern int bcm_tr3_l2_learn_class_set(int unit, int lclass,
                                      int lclass_prio, uint32 flags);
extern int bcm_tr3_l2_learn_class_get(int unit, int lclass,
                                      int *lclass_prio, uint32 *flags);
extern int bcm_tr3_l2_learn_port_class_set(int unit, bcm_gport_t port,
                                           int lclass);
extern int bcm_tr3_l2_learn_port_class_get(int unit, bcm_gport_t port,
                                           int *lclass);
extern int bcm_tr3_l2_traverse(int unit, _bcm_l2_traverse_t *trav_st);
extern int bcm_tr3_l2_replace(int unit, uint32 flags,
                              bcm_l2_addr_t *match_addr,
                              bcm_module_t new_module, bcm_port_t new_port,
                              bcm_trunk_t new_trunk);
extern int bcm_tr3_l2_conflict_get(int unit, bcm_l2_addr_t *addr, 
                                   bcm_l2_addr_t *cf_array,  int cf_max, 
                                   int *cf_count);

extern int bcm_tr3_l2_cache_init(int unit);
extern int bcm_tr3_l2_cache_set(int unit, int index,
                                bcm_l2_cache_addr_t *addr, int *index_used);
extern int bcm_tr3_l2_cache_get(int unit, int index,
                                bcm_l2_cache_addr_t *addr);
extern int bcm_tr3_l2_cache_delete(int unit, int index);
extern int bcm_tr3_l2_cache_delete_all(int unit);

extern int bcm_tr3_failover_ring_config_set(int unit,
                                            bcm_failover_ring_t *failover_ring);
extern int bcm_tr3_failover_ring_config_get(int unit,
                                            bcm_failover_ring_t *failover_ring);
extern int bcm_tr3_l2_ring_replace(int unit, bcm_l2_ring_t *l2_ring);

#if defined(INCLUDE_L3)

#define BCM_TR3_ESM_HOST_TBL_PRESENT(_u_)                          \
           ((soc_feature(_u_, soc_feature_esm_support))         && \
            (SOC_MEM_IS_ENABLED(_u_, EXT_IPV4_UCASTm))          && \
            (SOC_MEM_IS_ENABLED(_u_, EXT_IPV4_UCAST_WIDEm))     && \
            (SOC_MEM_IS_ENABLED(_u_, EXT_IPV6_128_UCASTm))      && \
            (SOC_MEM_IS_ENABLED(_u_, EXT_IPV6_128_UCAST_WIDEm))) 
            
#define BCM_TR3_ESM_LPM_TBL_PRESENT(_u_, _mem_)                \
           ((soc_feature(_u_, soc_feature_esm_support))     && \
            (SOC_MEM_IS_ENABLED(_u_, _mem_))                && \
            (soc_mem_index_count(_u_, _mem_))) 

#if 0
#define BCM_TR3_ESM_HOST_TBL_PRESENT(_u_) 0

#define BCM_TR3_ESM_LPM_TBL_PRESENT(_u_) 0
#endif /* if 0 */

#define BCM_TR3_MEM_IS_ESM(_u_, _mem_)              \
            ((BCM_TR3_ESM_HOST_TBL_PRESENT(_u_)) && \
             ((EXT_IPV4_UCASTm == _mem_)      ||  \
              (EXT_IPV4_UCAST_WIDEm == _mem_) ||  \
              (EXT_IPV6_128_UCASTm == _mem_)  ||  \
              (EXT_IPV6_128_UCAST_WIDEm == _mem_)))

#define BCM_TR3_L3_USE_EMBEDDED_NEXT_HOP(_u_, _intf_, _nhidx_) \
             ((_nhidx_ == BCM_XGS3_L3_INVALID_INDEX)      &&   \
              (!BCM_XGS3_L3_EGRESS_IDX_VALID(_u_, _intf_)) &&  \
              (BCM_XGS3_L3_EGRESS_MODE_ISSET(_u_))         &&  \
              soc_feature(unit, soc_feature_l3_extended_host_entry))
            
#define BCM_TR3_ISM_L3_HOST_TABLE_MEM(_u_, _intf_, _v6_, _mem_, _nhidx_)                \
            _mem_ = (_v6_) ? ((BCM_TR3_L3_USE_EMBEDDED_NEXT_HOP(_u_, _intf_, _nhidx_))? \
                             L3_ENTRY_4m:L3_ENTRY_2m) :                                 \
                           ((BCM_TR3_L3_USE_EMBEDDED_NEXT_HOP(_u_, _intf_, _nhidx_))?   \
                             L3_ENTRY_2m:L3_ENTRY_1m)

#define BCM_TR3_ESM_L3_HOST_TABLE_MEM(_u_, _intf_, _v6_, _mem_, _nhidx_)                \
            _mem_ = (_v6_) ? ((BCM_TR3_L3_USE_EMBEDDED_NEXT_HOP(_u_, _intf_, _nhidx_))? \
                             EXT_IPV6_128_UCAST_WIDEm:EXT_IPV6_128_UCASTm) :            \
                           ((BCM_TR3_L3_USE_EMBEDDED_NEXT_HOP(_u_, _intf_, _nhidx_))?   \
                             EXT_IPV4_UCAST_WIDEm:EXT_IPV4_UCASTm)

#define  BCM_TR3_ISM_L3_HOST_TABLE_FLD(_u_, _mem_, _v6_, _fld_)                               \
              _fld_ = (_bcm_l3_fields_t *) INT_TO_PTR((_v6_) ?                                \
                       ((_mem_ == L3_ENTRY_2m) ?                                              \
                         BCM_XGS3_L3_MEM_FIELDS(_u_,v6): BCM_XGS3_L3_MEM_FIELDS(_u_, v6_4)) : \
                       ((_mem_ == L3_ENTRY_1m) ?                                              \
                         BCM_XGS3_L3_MEM_FIELDS(_u_,v4): BCM_XGS3_L3_MEM_FIELDS(_u_, v4_2)))

#define  BCM_TR3_ESM_L3_HOST_TABLE_FLD(_u_, _mem_, _v6_, _fld_)                                    \
              _fld_ = (_bcm_l3_fields_t *) INT_TO_PTR((_v6_) ?                                     \
                               ((_mem_ == EXT_IPV6_128_UCASTm) ?                                   \
                                 BCM_XGS3_L3_MEM_FIELDS(_u_,v6_esm): BCM_XGS3_L3_MEM_FIELDS(_u_, v6_esm_wide)) : \
                               ((_mem_ == EXT_IPV4_UCASTm) ?                                       \
                                 BCM_XGS3_L3_MEM_FIELDS(_u_,v4_esm): BCM_XGS3_L3_MEM_FIELDS(_u_, v4_esm_wide)))

#define BCM_TR3_ISM_L3_HOST_ENTRY_BUF(_v6_, _mem_, _buf_, _v4ent_, _v4v6ent_, _v6ent_)  \
            _buf_ = ((_v6_) ?                                                           \
                     ((_mem_ == L3_ENTRY_2m) ? (uint32 *) &_v4v6ent_ :                  \
                       (uint32 *) &_v6ent_) :                                           \
                     ((_mem_ == L3_ENTRY_1m) ? (uint32 *) &_v4ent_ :                    \
                       (uint32 *) &_v4v6ent_)) 

#define BCM_TR3_ESM_L3_HOST_ENTRY_BUF(_v6_, _mem_, _buf_, _v4ent_, _v4wide_, _v6ent_, _v6wide_)  \
            _buf_ = ((_v6_) ?                                                                    \
                     ((_mem_ == EXT_IPV6_128_UCASTm ) ? (uint32 *) &_v6ent_ :                    \
                       (uint32 *) &_v6wide_) :                                                   \
                     ((_mem_ == EXT_IPV4_UCASTm) ? (uint32 *) &_v4ent_ :                         \
                       (uint32 *) &_v4wide_)) 

/* Return array index given the table name */
#define BCM_TR3_ESM_TBL_ENUMERATE(_mem_, _tbl_)              \
            _tbl_ = ((EXT_IPV4_UCASTm == _mem_) ? 0 :          \
                     ((EXT_IPV4_UCAST_WIDEm == _mem_) ? 1:     \
                      ((EXT_IPV6_128_UCASTm == _mem_) ? 2 : 3)))            
                 
/* L3 Internal Functions */

/* Host table support */
extern int _bcm_tr3_l3_add(int unit, _bcm_l3_cfg_t *l3cfg, int nh_idx);
extern int _bcm_tr3_l3_get(int unit, _bcm_l3_cfg_t *l3cfg, int *nh_idx);
extern int _bcm_tr3_l3_del(int unit, _bcm_l3_cfg_t *l3cfg);
extern int _bcm_tr3_l3_del_match(int unit, int flags, void *pattern,
                       bcm_xgs3_ent_op_cb cmp_func,
                       bcm_l3_host_traverse_cb notify_cb, void *user_data);
extern int _bcm_tr3_l3_get_by_idx(int unit, void *dma_ptr, int idx,
                                   _bcm_l3_cfg_t *l3cfg, int *nh_idx);
extern int bcm_tr3_l3_conflict_get(int unit, bcm_l3_key_t *ipkey, bcm_l3_key_t *cf_array,
                                   int cf_max, int *cf_count);
extern int _bcm_tr3_l3_intf_mtu_get(int unit, _bcm_l3_intf_cfg_t *intf_info);
extern int _bcm_tr3_l3_intf_mtu_set(int unit, _bcm_l3_intf_cfg_t *intf_info);
extern int _bcm_tr3_mtu_profile_index_get(int unit, uint32 mtu, uint32 *index);
extern int _bcm_tr3_l3_ecmp_grp_get(int unit, int ecmp_grp, int ecmp_group_size, int *nh_idx);
extern int _bcm_tr3_l3_ecmp_grp_add(int unit, int ecmp_grp, void *buf, int max_paths);
extern int _bcm_tr3_l3_ecmp_grp_del(int unit, int ecmp_grp, int max_grp_size);
extern int _bcm_tr3_nh_update_match(int unit, _bcm_l3_trvrs_data_t *trv_data);
extern int _bcm_tr3_l3_tnl_term_entry_init(int unit,
                                bcm_tunnel_terminator_t *tnl_info,
                                soc_tunnel_term_t *entry);
extern int _bcm_tr3_l3_tnl_term_entry_parse(int unit, soc_tunnel_term_t *entry,
                                  bcm_tunnel_terminator_t *tnl_info);
extern int _bcm_tr3_l3_tnl_term_add(int unit, uint32 *entry_ptr, 
                                bcm_tunnel_terminator_t *tnl_info);

extern int _bcm_tr3_l3_traverse(int unit, int flags, uint32 start, uint32 end,
                                 bcm_l3_host_traverse_cb cb, void *user_data);
                              
extern int bcm_tr3_l3_replace(int unit, _bcm_l3_cfg_t *l3cfg);
extern int bcm_tr3_l3_del_intf(int unit, _bcm_l3_cfg_t *l3cfg, int negate);

/*ISM LPM */
extern int _bcm_tr3_l3_lpm_get(int unit, _bcm_defip_cfg_t *lpm_cfg, int *nh_ecmp_idx);
extern int _bcm_tr3_l3_lpm_add(int unit, _bcm_defip_cfg_t *lpm_cfg, int nh_ecmp_idx);
extern int _bcm_tr3_l3_lpm_del(int unit, _bcm_defip_cfg_t *lpm_cfg);
extern int _bcm_tr3_l3_lpm_update_match(int unit, _bcm_l3_trvrs_data_t *trv_data);
extern int _bcm_tr3_l3_defip_init(int unit);
extern int _bcm_tr3_l3_defip_deinit(int unit);
extern int _bcm_tr3_l3_defip_urpf_enable(int unit, int enable);
/* ESM LPM */
extern int _bcm_tr3_ext_lpm_init(int unit, soc_mem_t mem);
extern int _bcm_tr3_ext_lpm_deinit(int unit, soc_mem_t mem);
extern int _bcm_tr3_ext_lpm_add(int unit, _bcm_defip_cfg_t *data, int nh_ecmp_idx);
extern int _bcm_tr3_ext_lpm_delete(int unit, _bcm_defip_cfg_t *key);
extern int _bcm_tr3_ext_lpm_match(int unit, _bcm_defip_cfg_t *key, int *next_hop_index) ;
extern int _bcm_tr3_defip_traverse(int unit, _bcm_l3_trvrs_data_t *trv_data);
/* ESM host table*/
extern int _bcm_tr3_esm_host_tbl_init(int unit);
extern int _bcm_tr3_esm_host_tbl_deinit(int unit);

/* L3 IPMC */
extern int _bcm_tr3_l3_ipmc_get_by_idx(int unit, void *dma_ptr, int idx,
                                       _bcm_l3_cfg_t *l3cfg);
extern int _bcm_tr3_l3_ipmc_get(int unit, _bcm_l3_cfg_t *l3cfg);
extern int _bcm_tr3_l3_ipmc_add(int unit, _bcm_l3_cfg_t *l3cfg);
extern int _bcm_tr3_l3_ipmc_del(int unit, _bcm_l3_cfg_t *l3cfg);

extern int _bcm_tr3_l3_ipmc_nh_add(int unit, int nh_index,
            bcm_l3_egress_t *l3_egress);
extern int _bcm_tr3_l3_ipmc_nh_entry_parse(int unit, uint32 *ing_nh, 
            uint32 *egr_nh, bcm_l3_egress_t *l3_egress);

/* Internal route table support */
extern int _bcm_tr3_lpm_add(int unit, _bcm_defip_cfg_t *lpm_cfg, int nh_ecmp_idx);

/* Flex stat - L3 host table */
extern bcm_error_t _bcm_tr3_l3_host_stat_attach(
            int unit,
            bcm_l3_host_t *info,
            uint32 stat_counter_id);
extern bcm_error_t _bcm_tr3_l3_host_stat_detach(
            int unit,
            bcm_l3_host_t *info);
extern bcm_error_t _bcm_tr3_l3_host_stat_counter_get(
            int unit,
            bcm_l3_host_t *info,
            bcm_l3_stat_t stat,
            uint32 num_entries,
            uint32 *counter_indexes,
            bcm_stat_value_t *counter_values);
extern bcm_error_t _bcm_tr3_l3_host_stat_counter_set(
            int unit,
            bcm_l3_host_t *info,
            bcm_l3_stat_t stat,
            uint32 num_entries,
            uint32 *counter_indexes,
            bcm_stat_value_t *counter_values);
extern bcm_error_t _bcm_tr3_l3_host_stat_id_get(
            int unit,
            bcm_l3_host_t *info,
            bcm_l3_stat_t stat,
            uint32 *stat_counter_id);

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_tr3_l3_esm_host_state_recover(int unit);
extern int _bcm_tr3_mpls_match_key_recover(int unit);
#else
#define _bcm_tr3_l3_esm_host_state_recover(_u) (BCM_E_UNAVAIL)
#endif /* BCM_WARM_BOOT_SUPPORT */

/* L3 ECMP DLB */
extern void _bcm_tr3_ecmp_dlb_deinit(int unit);
extern int _bcm_tr3_ecmp_dlb_init(int unit);
extern int bcm_tr3_l3_egress_dlb_attr_set(int unit, int nh_index,
        bcm_l3_egress_t *egr);
extern int bcm_tr3_l3_egress_dlb_attr_destroy(int unit, int nh_index);
extern int bcm_tr3_l3_egress_dlb_attr_get(int unit, int nh_index,
        bcm_l3_egress_t *egr);
extern int bcm_tr3_l3_egress_ecmp_dlb_create(int unit,
        bcm_l3_egress_ecmp_t *ecmp, int intf_count, bcm_if_t *intf_array);
extern int bcm_tr3_l3_egress_ecmp_dlb_destroy(int unit,
        bcm_l3_egress_ecmp_t *ecmp);
extern int bcm_tr3_l3_egress_ecmp_dlb_get(int unit,
        bcm_l3_egress_ecmp_t *ecmp);
extern int _bcm_tr3_ecmp_dlb_config_set(int unit, bcm_switch_control_t type,
        int arg);
extern int _bcm_tr3_ecmp_dlb_config_get(int unit, bcm_switch_control_t type,
        int *arg);
extern int bcm_tr3_l3_egress_ecmp_member_status_set(int unit, bcm_if_t intf,
        int status);
extern int bcm_tr3_l3_egress_ecmp_member_status_get(int unit, bcm_if_t intf,
        int *status);
extern int bcm_tr3_l3_egress_ecmp_dlb_ethertype_set(int unit, uint32 flags,
        int ethertype_count, int *ethertype_array);
extern int bcm_tr3_l3_egress_ecmp_dlb_ethertype_get(int unit, uint32 *flags,
        int ethertype_max, int *ethertype_array, int *ethertype_count);

#ifdef BCM_WARM_BOOT_SUPPORT
extern int bcm_tr3_ecmp_dlb_wb_alloc_size_get(int unit, int *size);
extern int bcm_tr3_ecmp_dlb_sync(int unit, uint8 **scache_ptr);
extern int bcm_tr3_ecmp_dlb_scache_recover(int unit, uint8 **scache_ptr);
extern int bcm_tr3_ecmp_dlb_hw_recover(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void bcm_tr3_ecmp_dlb_sw_dump(int unit);
extern void _bcm_tr3_esm_host_tbl_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

/* Wlan Internal functions */
extern int _bcm_tr3_wlan_tunnel_set_roam(int unit, bcm_gport_t tunnel_id);
extern int _bcm_tr3_wlan_tunnel_get_roam(int unit, bcm_gport_t tunnel_id);

extern int bcm_tr3_wlan_client_add(int,bcm_wlan_client_t *);
extern int bcm_tr3_wlan_client_delete(int,bcm_mac_t);
extern int bcm_tr3_wlan_client_delete_all(int);
extern int bcm_tr3_wlan_client_get(int,bcm_mac_t,bcm_wlan_client_t *);
extern int bcm_tr3_wlan_client_traverse(int,bcm_wlan_client_traverse_cb,void *);
extern int bcm_tr3_wlan_detach(int);
extern int bcm_tr3_wlan_init(int);
extern int bcm_tr3_wlan_port_add(int,bcm_wlan_port_t *);
extern int bcm_tr3_wlan_port_delete(int,bcm_gport_t);
extern int bcm_tr3_wlan_port_delete_all(int);
extern int bcm_tr3_wlan_port_get(int,bcm_gport_t,bcm_wlan_port_t *);
extern int bcm_tr3_wlan_port_traverse(int,bcm_wlan_port_traverse_cb,void *);
extern int bcm_tr3_wlan_tunnel_initiator_create(int,bcm_tunnel_initiator_t *);
extern int bcm_tr3_wlan_tunnel_initiator_destroy(int,bcm_gport_t);
extern int bcm_tr3_wlan_tunnel_initiator_get(int,bcm_tunnel_initiator_t *);
extern int _bcm_tr3_wlan_port_resolve(int unit, bcm_gport_t wlan_port,
                                    bcm_module_t *modid, bcm_port_t *port,
                                    bcm_trunk_t *trunk_id, int *id);
extern int bcm_tr3_wlan_port_learn_set(int unit, bcm_gport_t port, uint32 flags);
extern int bcm_tr3_wlan_port_learn_get(int unit, bcm_gport_t port, uint32 *flags);
extern int bcm_tr3_wlan_tunnel_terminator_vlan_set(int unit, bcm_gport_t tunnel, 
                                              bcm_vlan_vector_t vlan_vec);
extern int bcm_tr3_wlan_tunnel_terminator_vlan_get(int unit, bcm_gport_t tunnel, 
                                              bcm_vlan_vector_t *vlan_vec);
extern int bcm_tr3_wlan_lport_field_set(int unit, bcm_gport_t wlan_port_id, 
                                        soc_field_t field, int value);
extern int bcm_tr3_wlan_lport_field_get(int unit, bcm_gport_t wlan_port_id, 
                                        soc_field_t field, int *value);
extern int _bcm_tr3_wlan_port_set(int unit, bcm_gport_t wlan_port_id, 
                                  soc_field_t field, uint32 value);
extern int _bcm_tr3_qos_wlan_port_map_set(int unit, bcm_gport_t port, 
                                    int egr_map, int pri_index);
extern int bcm_tr3_wlan_port_untagged_vlan_get(int unit, bcm_gport_t port, 
                                               bcm_vlan_t *vid_ptr);
extern int bcm_tr3_wlan_port_untagged_vlan_set(int unit, bcm_gport_t port, 
                                               bcm_vlan_t vid);
extern int bcm_tr3_wlan_port_untagged_prio_get(int unit, bcm_gport_t port, 
                                               int *prio_ptr);
extern int bcm_tr3_wlan_port_untagged_prio_set(int unit, bcm_gport_t port, 
                                               int prio);

/* IPMC internal functions */
extern int bcm_tr3_ipmc_repl_init(int unit);
extern int bcm_tr3_ipmc_repl_detach(int unit);
extern int bcm_tr3_ipmc_repl_set(int unit, int ipmc_id, bcm_port_t port,
                         bcm_vlan_vector_t vlan_vec);
extern int bcm_tr3_ipmc_repl_get(int unit, int index, bcm_port_t port,
                         bcm_vlan_vector_t vlan_vec);
extern int bcm_tr3_ipmc_repl_add(int unit, int index, bcm_port_t port,
                         bcm_vlan_t vlan);
extern int bcm_tr3_ipmc_repl_delete(int unit, int index, bcm_port_t port,
                            bcm_vlan_t vlan);
extern int bcm_tr3_ipmc_repl_delete_all(int unit, int index,
                                bcm_port_t port);
extern int bcm_tr3_ipmc_egress_intf_add(int unit, int index, bcm_port_t port,
                                bcm_l3_intf_t *l3_intf);
extern int bcm_tr3_ipmc_egress_intf_delete(int unit, int index, bcm_port_t port,
                                   bcm_l3_intf_t *l3_intf);
extern int bcm_tr3_ipmc_egress_intf_set(int unit, int mc_index,
                                       bcm_port_t port, int if_count,
                                       bcm_if_t *if_array, int is_l3,
                                       int check_port);
extern int bcm_tr3_ipmc_egress_intf_get(int unit, int mc_index,
                                       bcm_port_t port,
                                       int if_max, bcm_if_t *if_array,
                                       int *if_count);
extern int _bcm_tr3_ipmc_egress_intf_add(int unit, int index, bcm_port_t port,
                                       int encap_id, int is_l3);
extern int _bcm_tr3_ipmc_egress_intf_delete(int unit, int index,
                                       bcm_port_t port, int if_max,
                                       int encap_id, int is_l3);
extern int bcm_tr3_ipmc_trill_mac_update(int unit, uint32 mac_field,
                                         uint8 flag);
extern int bcm_tr3_ipmc_l3_intf_next_hop_free(int unit, int intf);
extern int bcm_tr3_ipmc_l3_intf_next_hop_get(int unit, int intf, int *nh_index);
extern int bcm_tr3_ipmc_l3_intf_next_hop_l3_egress_set(int unit, int intf);
#ifdef BCM_WARM_BOOT_SUPPORT
extern int bcm_tr3_ipmc_repl_l3_intf_scache_size_get(int unit, uint32 *size);
extern int bcm_tr3_ipmc_repl_l3_intf_sync(int unit, uint8 **scache_ptr);
extern int bcm_tr3_ipmc_repl_l3_intf_scache_recover(int unit, uint8 **scache_ptr);
#endif /* BCM_WARM_BOOT_SUPPORT */

extern int bcm_tr3_failover_egr_check (int unit, bcm_l3_egress_t  *egr);
extern int bcm_tr3_failover_prot_nhi_set(int unit, uint32 flags, int nh_index, uint32 prot_nh_index, 
                                           bcm_multicast_t  mc_group, bcm_failover_t failover_id);
extern int bcm_tr3_failover_prot_nhi_cleanup  (int unit, int nh_index);
extern int bcm_tr3_failover_prot_nhi_get(int unit, int nh_index, 
            bcm_failover_t  *failover_id, int  *prot_nh_index, bcm_multicast_t  *mc_group);
extern int bcm_tr3_failover_prot_nhi_update  (int unit, int old_nh_index, int new_nh_index);
extern int bcm_tr3_failover_mpls_check (int unit, bcm_mpls_port_t  *mpls_port);
extern int bcm_tr3_failover_status_set(int unit, bcm_failover_element_t *failover, int value);
extern int bcm_tr3_failover_status_get(int unit, bcm_failover_element_t *failover, int  *value);
extern int bcm_tr3_l2gre_egress_set(int unit, int nh_index);
extern int bcm_tr3_l2gre_egress_get(int unit, bcm_l3_egress_t *egr, int nh_index);
extern int bcm_tr3_mpls_tunnel_switch_traverse(int unit,
                                   bcm_mpls_tunnel_switch_traverse_cb cb,
                                   void *user_data);
extern int bcm_tr3_mpls_tunnel_switch_get(int unit, bcm_mpls_tunnel_switch_t *info);
extern int bcm_tr3_mpls_tunnel_switch_delete_all(int unit);
extern  int bcm_tr3_mpls_tunnel_switch_delete(int unit, bcm_mpls_tunnel_switch_t *info);
extern int bcm_tr3_mpls_tunnel_switch_add(int unit, bcm_mpls_tunnel_switch_t *info);
extern int _bcm_tr3_mpls_match_add(int unit, bcm_mpls_port_t *mpls_port, int vp);
extern int _bcm_tr3_mpls_match_delete(int unit, int vp);
extern int _bcm_tr3_mpls_match_get(int unit, bcm_mpls_port_t *mpls_port, int vp);
extern bcm_error_t bcm_tr3_mpls_port_stat_attach(
                   int         unit,
                   bcm_vpn_t   vpn,
                   bcm_gport_t port,
                   uint32      stat_counter_id);
extern bcm_error_t bcm_tr3_mpls_port_stat_detach(
                   int         unit, 
                   bcm_vpn_t   vpn,
                   bcm_gport_t port);
extern bcm_error_t bcm_tr3_mpls_port_stat_counter_get(
                   int                  unit,
                   bcm_vpn_t            vpn,
                   bcm_gport_t          port,
                   bcm_mpls_stat_t      stat,
                   uint32               num_entries,
                   uint32               *counter_indexes,
                   bcm_stat_value_t     *counter_values);
extern bcm_error_t bcm_tr3_mpls_port_stat_counter_set(
                   int                  unit,
                   bcm_vpn_t            vpn,
                   bcm_gport_t          port,
                   bcm_mpls_stat_t      stat,
                   uint32               num_entries,
                   uint32               *counter_indexes,
                   bcm_stat_value_t     *counter_values);
extern bcm_error_t bcm_tr3_mpls_port_stat_id_get(
                   int                  unit,
                   bcm_vpn_t            vpn,
                   bcm_gport_t          port,
                   bcm_mpls_stat_t      stat,
                   uint32               *stat_counter_id);
extern bcm_error_t bcm_tr3_mpls_label_stat_attach(
                   int              unit,
                   bcm_mpls_label_t label,
                   bcm_gport_t      port,
                   uint32           stat_counter_id);
extern bcm_error_t bcm_tr3_mpls_label_stat_detach(
                   int              unit,
                   bcm_mpls_label_t label,
                   bcm_gport_t      port);
extern bcm_error_t bcm_tr3_mpls_label_stat_counter_get(
                   int                  unit,
                   bcm_mpls_label_t     label,
                   bcm_gport_t          port,
                   bcm_mpls_stat_t      stat,
                   uint32               num_entries,
                   uint32               *counter_indexes,
                   bcm_stat_value_t     *counter_values);
extern bcm_error_t bcm_tr3_mpls_label_stat_counter_set(
                   int                  unit,
                   bcm_mpls_label_t     label,
                   bcm_gport_t          port,
                   bcm_mpls_stat_t      stat,
                   uint32               num_entries,
                   uint32               *counter_indexes,
                   bcm_stat_value_t     *counter_values);
extern bcm_error_t bcm_tr3_mpls_label_stat_id_get(
                   int                  unit,
                   bcm_mpls_label_t     label,
                   bcm_gport_t          port,
                   bcm_mpls_stat_t      stat,
                   uint32               *stat_counter_id);
extern int bcm_tr3_mpls_label_stat_enable_set(
           int              unit,
           bcm_mpls_label_t label,
           bcm_gport_t      port,
           int              enable);
extern int bcm_tr3_mpls_label_stat_get(
           int              unit, 
           bcm_mpls_label_t label, 
           bcm_gport_t      port,
           bcm_mpls_stat_t  stat,
           uint64           *val);
extern int bcm_tr3_mpls_label_stat_get32(
           int              unit,
           bcm_mpls_label_t label, 
           bcm_gport_t      port, 
           bcm_mpls_stat_t  stat,
           uint32           *val);
extern int bcm_tr3_mpls_label_stat_clear(
           int              unit,
           bcm_mpls_label_t label, 
           bcm_gport_t      port, 
           bcm_mpls_stat_t  stat);

extern bcm_error_t _bcm_tr3_mim_vpn_stat_get_table_info(
           int                        unit,
           bcm_mim_vpn_t              vpn,
           uint32                     *num_of_tables,
           bcm_stat_flex_table_info_t *table_info);

extern bcm_error_t _bcm_tr3_mim_lookup_id_stat_get_table_info(
           int                        unit,
           int                        lookup_id,
           uint32                     *num_of_tables,
           bcm_stat_flex_table_info_t *table_info);


extern int _bcm_tr3_l2gre_port_resolve(
           int unit, 
           bcm_gport_t l2gre_port_id, 
           bcm_if_t encap_id, 
           bcm_module_t *modid, 
           bcm_port_t *port,
           bcm_trunk_t *trunk_id, 
           int *id);

extern int _bcm_tr3_l2gre_multicast_enable(int unit, bcm_multicast_t group, 
                                        bcm_gport_t l2gre_port_id);
extern  int  bcm_tr3_l2gre_lock(int unit);
extern void bcm_tr3_l2gre_unlock(int unit);
extern void _bcm_tr3_l2gre_sw_dump(int unit);
extern int bcm_tr3_l2gre_init(int unit);
extern int bcm_tr3_l2gre_cleanup(int unit);
extern int bcm_tr3_l2gre_vpn_create(int unit, bcm_l2gre_vpn_config_t *info);
extern int bcm_tr3_l2gre_vpn_destroy(int unit, bcm_vpn_t l2vpn);
extern int bcm_tr3_l2gre_vpn_destroy_all(int unit);
extern int bcm_tr3_l2gre_vpn_get( int unit, bcm_vpn_t l2vpn, bcm_l2gre_vpn_config_t *info);
extern int bcm_tr3_l2gre_port_add(int unit, bcm_vpn_t vpn, bcm_l2gre_port_t  *l2gre_port);
extern int bcm_tr3_l2gre_port_delete(int unit, bcm_vpn_t vpn, bcm_gport_t l2gre_port_id);
extern int bcm_tr3_l2gre_port_delete_all(int unit, bcm_vpn_t vpn);
extern int bcm_tr3_l2gre_port_get(int unit, bcm_vpn_t l2vpn, bcm_l2gre_port_t *l2gre_port);
extern int bcm_tr3_l2gre_port_get_all( int unit, bcm_vpn_t l2vpn, int port_max, bcm_l2gre_port_t *port_array, int *port_count);
extern int bcm_tr3_l2gre_tunnel_initiator_create(int unit, bcm_tunnel_initiator_t *info);
extern int bcm_tr3_l2gre_tunnel_initiator_destroy(int unit, bcm_gport_t l2gre_tunnel_id);
extern int bcm_tr3_l2gre_tunnel_initiator_get(int unit, bcm_tunnel_initiator_t *info);
extern int bcm_tr3_l2gre_tunnel_initiator_traverse(int unit, bcm_tunnel_initiator_traverse_cb cb, void *user_data);
extern int bcm_tr3_l2gre_tunnel_terminator_create(int unit, bcm_tunnel_terminator_t *info);
extern int bcm_tr3_l2gre_tunnel_terminator_destroy(int unit, bcm_gport_t l2gre_tunnel_id);
extern int bcm_tr3_l2gre_tunnel_terminator_get( int unit, bcm_tunnel_terminator_t *info);
extern int bcm_tr3_l2gre_tunnel_terminator_traverse(int unit, bcm_tunnel_terminator_traverse_cb cb, void *user_data);
extern int  _bcm_tr3_l2gre_vpn_get(int unit, int vfi_index, bcm_vlan_t *vid);
extern int bcm_tr3_l2gre_multicast_leaf_entry_check(int unit, bcm_ip_t mc_ip_addr,  int flag);
extern int _bcm_esw_l2gre_multicast_leaf_entry_check(int unit,  bcm_ip_t mc_ip_addr, int flag);
extern int  _bcm_esw_l2gre_match_add(int unit, bcm_l2gre_port_t *l2gre_port, int vp, bcm_vpn_t vpn);
extern int  _bcm_esw_l2gre_match_delete(int unit,  int vp);
extern void  bcm_tr3_l2gre_port_match_count_adjust(int unit, int vp, int step);
extern int bcm_tr3_l2gre_match_add(int unit, bcm_l2gre_port_t *l2gre_port, int vp, bcm_vpn_t vpn);
extern int bcm_tr3_l2gre_match_delete(int unit,  int vp);
extern int  bcm_tr3_l2gre_match_trunk_add(int unit, bcm_trunk_t tgid, int vp);
extern int bcm_tr3_l2gre_match_trunk_delete(int unit, bcm_trunk_t tgid, int vp);
extern int bcm_tr3_l2gre_port_untagged_profile_set (int unit, bcm_port_t port);
extern int bcm_tr3_l2gre_port_untagged_profile_reset(int unit, bcm_port_t port);
extern int bcm_tr3_l2gre_match_vpnid_entry_set(int unit, mpls_entry_entry_t *ment);
extern int bcm_tr3_l2gre_match_vpnid_entry_reset(int unit, mpls_entry_entry_t *ment);
extern int bcm_tr3_l2gre_match_tunnel_entry_set(int unit, mpls_entry_entry_t *ment);
extern int bcm_tr3_l2gre_match_tunnel_entry_reset(int unit, mpls_entry_entry_t *ment);
extern void bcm_tr3_l2gre_match_clear (int unit, int vp);


#if defined(INCLUDE_BFD)
extern int bcmi_tr3_bfd_init(int unit);
#endif /* INCLUDE_BFD */

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_tr3_ipmc_repl_reload(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_tr3_ipmc_repl_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

/* NIV */
extern int bcm_tr3_niv_forward_add(int unit,
        bcm_niv_forward_t *iv_fwd_entry);
extern int bcm_tr3_niv_forward_delete(int unit,
        bcm_niv_forward_t *iv_fwd_entry);
extern int bcm_tr3_niv_forward_delete_all(int unit);
extern int bcm_tr3_niv_forward_get(int unit,
        bcm_niv_forward_t *iv_fwd_entry);
extern int bcm_tr3_niv_forward_traverse(int unit,
        bcm_niv_forward_traverse_cb cb, void *user_data);

/* Port Extender */
extern int bcm_tr3_extender_init(int unit);
extern int bcm_tr3_extender_cleanup(int unit);
extern int bcm_tr3_extender_port_add(int unit,
        bcm_extender_port_t *extender_port);
extern int bcm_tr3_extender_port_delete(int unit,
        bcm_gport_t extender_port_id);
extern int bcm_tr3_extender_port_delete_all(int unit);
extern int bcm_tr3_extender_port_get(int unit,
        bcm_extender_port_t *extender_port);
extern int bcm_tr3_extender_port_traverse(int unit,
        bcm_extender_port_traverse_cb cb, void *user_data);
extern int bcm_tr3_extender_forward_add(int unit,
        bcm_extender_forward_t *extender_forward_entry);
extern int bcm_tr3_extender_forward_delete(int unit,
        bcm_extender_forward_t *extender_forward_entry);
extern int bcm_tr3_extender_forward_delete_all(int unit);
extern int bcm_tr3_extender_forward_get(int unit,
        bcm_extender_forward_t *extender_forward_entry);
extern int bcm_tr3_extender_forward_traverse(int unit,
        bcm_extender_forward_traverse_cb cb, void *user_data);

extern int _bcm_tr3_extender_port_resolve(int unit,
        bcm_gport_t extender_port_id,
        bcm_module_t *modid, bcm_port_t *port,
        bcm_trunk_t *trunk_id, int *id);

extern int bcm_tr3_etag_ethertype_set(int unit, int ethertype);
extern int bcm_tr3_etag_ethertype_get(int unit, int *ethertype);

extern int bcm_tr3_extender_untagged_add(int unit, bcm_vlan_t vlan, int vp,
                                       int flags, int *egr_vt_added);
extern int bcm_tr3_extender_untagged_delete(int unit, bcm_vlan_t vlan, int vp);
extern int bcm_tr3_extender_untagged_get(int unit, bcm_vlan_t vlan, int vp,
                                       int *flags);
extern int bcm_tr3_extender_port_untagged_vlan_set(int unit, bcm_port_t port,
                                       bcm_vlan_t vlan);
extern int bcm_tr3_extender_port_untagged_vlan_get(int unit, bcm_port_t port,
                                       bcm_vlan_t *vid_ptr);

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void bcm_tr3_extender_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif /* INCLUDE_L3 */

#if defined(BCM_FIELD_SUPPORT)
#ifdef BCM_HELIX4_SUPPORT
extern int _bcm_field_helix4_stage_init(int unit, _field_stage_t *stage_fc);
#endif
extern int _bcm_field_tr3_stage_init(int unit, _field_stage_t *stage_fc);
extern int _bcm_field_tr3_external_present(int unit);
extern int _bcm_field_tr3_meter_pool_info(int            unit,
                                          _field_stage_t *stage_fc,
                                          int            *num_pools,
                                          int            *pairs_per_pool,
                                          uint16         *pool_size
                                          );
extern int _bcm_field_tr3_cntr_pool_info(int            unit,
                                         _field_stage_t *stage_fc,
                                         unsigned       *num_pools,
                                         unsigned       *cntrs_per_pool
                                         );
extern int _bcm_field_tr3_slice_enable_set(int            unit,
                                           _field_stage_t *stage_fc,
                                           _field_group_t *fg,
                                           _field_slice_t *fs,
                                           unsigned       endis
                                           );
extern int _bcm_field_tr3_slice_clear(int            unit,
                                      _field_group_t *fg,
                                      _field_slice_t *fs
                                      );
extern int _bcm_field_tr3_group_qset_update(int unit, _field_group_t *fg);
extern int _bcm_field_tr3_group_default_aset_set(int unit, _field_group_t *fg);
extern int _bcm_field_tr3_aset_install(int unit, _field_group_t *fg);
extern int _bcm_field_tr3_qualify_PacketRes(int               unit, 
                                            bcm_field_entry_t entry, 
                                            uint32            *data, 
                                            uint32            *mask
                                            );
extern int _bcm_field_tr3_qualify_PacketRes_get(int               unit, 
                                                bcm_field_entry_t entry, 
                                                uint32            *data, 
                                                uint32            *mask
                                                );
extern int
_bcm_field_tr3_qualify_LoopbackType(bcm_field_LoopbackType_t loopback_type,
                                    uint32                   *tcam_data,
                                    uint32                   *tcam_mask
                                    );
extern int
_bcm_field_tr3_qualify_LoopbackType_get(uint8                    tcam_data,
                                        uint8                    tcam_mask,
                                        bcm_field_LoopbackType_t *loopback_type
                                        );
extern int 
_bcm_field_tr3_qualify_TunnelType(bcm_field_TunnelType_t tunnel_type,
                                  uint32                 *tcam_data,
                                  uint32                 *tcam_mask
                                  );
extern int 
_bcm_field_tr3_qualify_TunnelType_get(uint8                  tcam_data,
                                      uint8                  tcam_mask,
                                      bcm_field_TunnelType_t *tunnel_type
                                      );
extern int _bcm_field_tr3_qualify_class(int                 unit,
                                        bcm_field_entry_t   entry,
                                        bcm_field_qualify_t qual,
                                        uint32              *data,
                                        uint32              *mask
                                        );
extern int _bcm_field_tr3_qualify_class_get(int                 unit,
                                            bcm_field_entry_t   entry,
                                            bcm_field_qualify_t qual,
                                            uint32              *data,
                                            uint32              *mask
                                            );
extern int _bcm_field_tr3_qual_tcam_key_mask_get(int            unit,
                                                 _field_entry_t *f_ent,
                                                 _field_tcam_t  *tcam
                                                 );
extern int _bcm_field_tr3_qual_tcam_key_mask_set(int            unit,
                                                 _field_entry_t *f_ent,
                                                 unsigned       validf
                                                 );
extern int _bcm_field_tr3_action_get(int             unit,
                                     soc_mem_t       mem,
                                     _field_entry_t  *f_ent,
                                     int             tcam_idx,
                                     _field_action_t *fa,
                                     uint32          *buf
                                     );
extern int _bcm_field_tr3_policer_mode_support(int            unit,
                                               _field_entry_t *f_ent,
                                               int            lvl,
                                               _field_policer_t *f_pl
                                               );
extern int _bcm_field_tr3_policer_action_set(int            unit,
                                             _field_entry_t *f_ent,
                                             soc_mem_t      mem,
                                             uint32         *buf
                                             );
extern int _bcm_field_tr3_stat_hw_mode_get(int unit, _field_stat_t *f_st);
extern int _bcm_field_tr3_counter_hw_alloc(int            unit,
                                           _field_entry_t *f_ent
                                           );
extern int _bcm_field_tr3_stat_hw_free(int            unit,
                                       _field_entry_t *f_ent
                                       );
extern int _bcm_field_tr3_stat_action_set(int            unit,
                                          _field_entry_t *f_ent,
                                          soc_mem_t      mem,
                                          int            tcam_idx,
                                          uint32         *buf
                                          );
extern int _bcm_field_tr3_stat_hw_mode_max(_field_stage_id_t stage_id,
                                           int               *hw_mode_max
                                           );
extern int _bcm_field_tr3_stat_hw_mode_to_bmap(int               unit,
                                               uint8             mode, 
                                               _field_stage_id_t stage_id, 
                                               uint32            *hw_bmap,
                                               uint8             *hw_entry_count
                                               );
extern int _bcm_field_tr3_counter_mem_get(int            unit,
                                          _field_stage_t *stage_fc,
                                          soc_mem_t      *counter_x_mem,
                                          soc_mem_t      *counter_y_mem
                                          );
extern int _bcm_field_tr3_external_policy_mem_get(int             unit,
                                                  _field_action_t *fa,
                                                  soc_mem_t       *mem
                                                  );
extern int _bcm_field_tr3_external_policy_install(int            unit,
                                                  _field_stage_t *stage_fc,
                                                  _field_entry_t *f_ent
                                                  );
extern int _bcm_field_tr3_hw_clear(int unit, _field_stage_t *stage_fc);
extern int _bcm_field_tr3_init(int unit, _field_control_t *fc);
#endif /* BCM_FIELD_SUPPORT */

extern int bcm_tr3_cosq_init(int unit);

extern int bcm_tr3_cosq_detach(int unit, int software_state_only);

extern int bcm_tr3_cosq_config_set(int unit, int numq);

extern int bcm_tr3_cosq_config_get(int unit, int *numq);

extern int bcm_tr3_cosq_mapping_set(int unit, bcm_port_t gport, 
                        bcm_cos_t priority, bcm_cos_queue_t cosq);

extern int bcm_tr3_cosq_mapping_get(int unit, bcm_port_t gport,
                        bcm_cos_t priority, bcm_cos_queue_t *cosq);

extern int bcm_tr3_cosq_port_sched_set(int unit, bcm_pbmp_t pbm,
                           int mode, const int *weights, int delay);

extern int bcm_tr3_cosq_port_sched_get(int unit, bcm_pbmp_t pbm,
                           int *mode, int *weights, int *delay);

extern int bcm_tr3_cosq_sched_weight_max_get(int unit, int mode, 
                                                int *weight_max);

extern int bcm_tr3_cosq_discard_set(int unit, uint32 flags);

extern int bcm_tr3_cosq_discard_get(int unit, uint32 *flags);

extern int
bcm_tr3_cosq_gport_discard_set(int unit, bcm_gport_t gport,
                               bcm_cos_queue_t cosq,
                               bcm_cosq_gport_discard_t *discard);

extern int
_bcm_tr3_cosq_pfc_class_resolve(bcm_switch_control_t sctype, int *type,
                               int *class);

extern int
bcm_tr3_cosq_port_pfc_op(int unit, bcm_port_t port,
                        bcm_switch_control_t sctype, _bcm_cosq_op_t op,
                        bcm_gport_t *gport, int gport_count);

extern int
bcm_tr3_cosq_port_pfc_get(int unit, bcm_port_t port,
                         bcm_switch_control_t sctype,
                         bcm_gport_t *gport, int gport_count,
                         int *actual_gport_count);

extern int
bcm_tr3_cosq_gport_discard_get(int unit, bcm_gport_t gport,
                              bcm_cos_queue_t cosq,
                              bcm_cosq_gport_discard_t *discard);

extern int
bcm_tr3_cosq_gport_add(int unit, bcm_gport_t port, int numq, uint32 flags,
                      bcm_gport_t *gport);

extern int
bcm_tr3_cosq_gport_detach(int unit, bcm_gport_t sched_gport,
                         bcm_gport_t input_gport, bcm_cos_queue_t cosq);

extern int
bcm_tr3_cosq_discard_port_get(int unit, bcm_port_t port, 
                 bcm_cos_queue_t cosq, uint32 color, int *drop_start, 
                        int *drop_slope, int *average_time);

extern int
bcm_tr3_cosq_gport_delete(int unit, bcm_gport_t gport);

extern int
bcm_tr3_cosq_gport_get(int unit, bcm_gport_t gport, bcm_gport_t *port,
                      int *numq, uint32 *flags);

extern int
bcm_tr3_cosq_gport_traverse(int unit, bcm_cosq_gport_traverse_cb cb,
                           void *user_data);

extern int
bcm_tr3_cosq_discard_port_set(int unit, bcm_port_t port, 
                            bcm_cos_queue_t cosq,
                            uint32 color, int drop_start, int drop_slope,
                            int average_time);

extern int
bcm_tr3_cosq_gport_attach(int unit, bcm_gport_t sched_gport,
                         bcm_gport_t input_gport, bcm_cos_queue_t cosq);

extern int
bcm_tr3_cosq_gport_attach_get(int unit, bcm_gport_t sched_gport,
                             bcm_gport_t *input_gport, bcm_cos_queue_t *cosq);

extern int
bcm_tr3_cosq_port_bandwidth_set(int unit, bcm_port_t port,
                               bcm_cos_queue_t cosq,
                               uint32 min_quantum, uint32 max_quantum,
                               uint32 burst_quantum, uint32 flags);

extern int
bcm_tr3_cosq_port_bandwidth_get(int unit, bcm_port_t port,
                               bcm_cos_queue_t cosq,
                               uint32 *min_quantum, uint32 *max_quantum,
                               uint32 *burst_quantum, uint32 *flags);

extern int
bcm_tr3_cosq_port_pps_set(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                         int pps);

extern int
bcm_tr3_cosq_port_pps_get(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                         int *pps);

extern int bcm_tr3_cosq_port_burst_set(int unit, bcm_port_t port, 
                                       bcm_cos_queue_t cosq, int burst);

extern int
bcm_tr3_cosq_stat_set(int unit, bcm_gport_t port, bcm_cos_queue_t cosq,
                     bcm_cosq_stat_t stat, uint64 value);

extern int
bcm_tr3_cosq_stat_get(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                     bcm_cosq_stat_t stat, uint64 *value);

extern int
bcm_tr3_cosq_port_burst_get(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                           int *burst);

extern int
bcm_tr3_cosq_control_set(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                        bcm_cosq_control_t type, int arg);

extern int
bcm_tr3_cosq_control_get(int unit, bcm_gport_t gport, bcm_cos_queue_t cosq,
                        bcm_cosq_control_t type, int *arg);

extern int
bcm_tr3_cosq_gport_bandwidth_burst_set(int unit, bcm_gport_t gport,
                                      bcm_cos_queue_t cosq,
                                      uint32 kbits_burst_min, 
                                      uint32 kbits_burst_max);

extern int
bcm_tr3_cosq_gport_bandwidth_burst_get(int unit, bcm_gport_t gport,
                                       bcm_cos_queue_t cosq,
                                       uint32 *kbits_burst_min,
                                       uint32 *kbits_burst_max);
extern int
bcm_tr3_cosq_gport_bandwidth_set(int unit, bcm_gport_t gport,
                                bcm_cos_queue_t cosq, uint32 kbits_sec_min,
                                uint32 kbits_sec_max, uint32 flags);

extern int
bcm_tr3_cosq_gport_bandwidth_get(int unit, bcm_gport_t gport,
                                bcm_cos_queue_t cosq, uint32 *kbits_sec_min,
                                uint32 *kbits_sec_max, uint32 *flags);

extern int bcm_tr3_cosq_congestion_queue_set(int unit, bcm_port_t port,
                                            bcm_cos_queue_t cosq, int index);
extern int bcm_tr3_cosq_congestion_queue_get(int unit, bcm_port_t port,
                                            bcm_cos_queue_t cosq, int *index);
extern int bcm_tr3_cosq_congestion_quantize_set(int unit, bcm_port_t port,
                                               bcm_cos_queue_t cosq,
                                               int weight_code, int setpoint);
extern int bcm_tr3_cosq_congestion_quantize_get(int unit, bcm_port_t port,
                                               bcm_cos_queue_t cosq,
                                               int *weight_code,
                                               int *setpoint);
extern int bcm_tr3_cosq_congestion_sample_int_set(int unit, bcm_port_t port,
                                                 bcm_cos_queue_t cosq,
                                                 int min, int max);
extern int bcm_tr3_cosq_congestion_sample_int_get(int unit, bcm_port_t port,
                                                 bcm_cos_queue_t cosq,
                                                 int *min, int *max);
extern int bcm_tr3_cosq_drop_status_enable_set(int unit, bcm_port_t port,
                                              int enable);
extern int _bcm_tr3_get_def_qbase(int unit, bcm_port_t inport, int qtype, 
                            int *pbase, int *pnumq);

extern int bcm_tr3_cosq_gport_sched_set(int unit, bcm_gport_t gport,
                                       bcm_cos_queue_t cosq, int mode,
                                       int weight);
extern int bcm_tr3_cosq_gport_sched_get(int unit, bcm_gport_t gport,
                                       bcm_cos_queue_t cosq, int *mode,
                                       int *weight);
extern int
_bcm_tr3_cosq_port_resolve(int unit, bcm_gport_t gport, bcm_module_t *modid,
                          bcm_port_t *port, bcm_trunk_t *trunk_id, int *id,
                          int *qnum);


extern int 
bcm_tr3_cosq_gport_congestion_config_set(int unit, bcm_gport_t port, 
                                         bcm_cos_queue_t cosq, 
                                         bcm_cosq_congestion_info_t *config);

extern int 
bcm_tr3_cosq_gport_congestion_config_get(int unit, bcm_gport_t port, 
                                         bcm_cos_queue_t cosq, 
                                         bcm_cosq_congestion_info_t *config);
/* Routines to program IFP_COS_MAP table. */
extern int
bcm_tr3_cosq_field_classifier_id_create(int unit,
                                        bcm_cosq_classifier_t *classifier,
                                        int *classifier_id);
extern int 
bcm_tr3_cosq_field_classifier_map_set(int unit,
                                      int classifier_id,
                                      int array_count,
                                      bcm_cos_t *priority_array,
                                      bcm_cos_queue_t *cosq_array);
extern int
bcm_tr3_cosq_field_classifier_map_get(int unit,
                                      int classifier_id,
                                      int array_max,
                                      bcm_cos_t *priority_array,
                                      bcm_cos_queue_t *cosq_array,
                                      int *array_count);
extern int
bcm_tr3_cosq_field_classifier_map_clear(int unit, int classifier_id);

extern int
bcm_tr3_cosq_field_classifier_id_destroy(int unit, int classifier_id);

extern int
bcm_tr3_cosq_field_classifier_get(int unit, int classifier_id,
                                  bcm_cosq_classifier_t *classifier);
extern int 
bcm_tr3_cosq_service_classifier_id_create(int unit,
        bcm_cosq_classifier_t *classifier,
        int *classifier_id);

extern int 
bcm_tr3_cosq_service_classifier_id_destroy(int unit,
        int classifier_id);

extern int 
bcm_tr3_cosq_service_classifier_get(int unit,
        int classifier_id,
        bcm_cosq_classifier_t *classifier);

extern int 
bcm_tr3_cosq_service_map_set(int unit, bcm_port_t port,
        int classifier_id,
        bcm_gport_t queue_group,
        int array_count,
        bcm_cos_t *priority_array,
        bcm_cos_queue_t *cosq_array);

extern int 
bcm_tr3_cosq_service_map_get(int unit, bcm_port_t port,
        int classifier_id,
        bcm_gport_t *queue_group,
        int array_max,
        bcm_cos_t *priority_array,
        bcm_cos_queue_t *cosq_array,
        int *array_count);

extern int 
bcm_tr3_cosq_service_map_clear(int unit,
        bcm_gport_t port,
        int classifier_id);

extern int
bcm_tr3_cosq_cpu_cosq_enable_set(
        int unit, 
        bcm_cos_queue_t cosq, 
        int enable);

extern int
bcm_tr3_cosq_cpu_cosq_enable_get(
        int unit, 
        bcm_cos_queue_t cosq, 
        int *enable);

typedef enum {
    _BCM_TR3_COSQ_INDEX_STYLE_BUCKET,
    _BCM_TR3_COSQ_INDEX_STYLE_QCN,
    _BCM_TR3_COSQ_INDEX_STYLE_WRED,
    _BCM_TR3_COSQ_INDEX_STYLE_WRED_PORT,
    _BCM_TR3_COSQ_INDEX_STYLE_SCHEDULER,
    _BCM_TR3_COSQ_INDEX_STYLE_UCAST_DROP,
    _BCM_TR3_COSQ_INDEX_STYLE_COS,
    _BCM_TR3_COSQ_INDEX_STYLE_UCAST_QUEUE,
    _BCM_TR3_COSQ_INDEX_STYLE_MCAST_QUEUE,
    _BCM_TR3_COSQ_INDEX_STYLE_EXT_UCAST_QUEUE,
    _BCM_TR3_COSQ_INDEX_STYLE_EGR_POOL,
    _BCM_TR3_COSQ_INDEX_STYLE_COUNT
} _bcm_tr3_cosq_index_style_t;

extern int
_bcm_tr3_cosq_index_resolve(int unit, bcm_port_t port, bcm_cos_queue_t cosq, 
                           _bcm_tr3_cosq_index_style_t style,                 
                           bcm_port_t *local_port, int *index, int *count);


/* Cross Connect */
extern int 
bcm_tr3_l2_cross_connect_add(int unit, bcm_vlan_t outer_vlan, 
                            bcm_vlan_t inner_vlan, bcm_gport_t port_1, 
                            bcm_gport_t port_2);
extern int 
bcm_tr3_l2_cross_connect_delete(int unit, bcm_vlan_t outer_vlan, 
                               bcm_vlan_t inner_vlan);

extern int
bcm_tr3_l2_cross_connect_delete_all(int unit);

extern int
bcm_tr3_l2_cross_connect_traverse(int unit,
                                 bcm_vlan_cross_connect_traverse_cb cb,
                                 void *user_data);


extern int 
bcm_tr3_cosq_bst_profile_set(int unit, 
                             bcm_gport_t port, 
                             bcm_cos_queue_t cosq, 
                             uint32 flags,
                             bcm_cosq_bst_profile_t *profile);

extern int 
bcm_tr3_cosq_bst_profile_get(int unit, 
                             bcm_gport_t port, 
                             bcm_cos_queue_t cosq, 
                             uint32 flags,
                             bcm_cosq_bst_profile_t *profile);

extern int 
bcm_tr3_cosq_bst_stat_get(int unit, 
                              bcm_gport_t port, 
                              bcm_cos_queue_t cosq, 
                              uint32 flags,
                              uint64 *stat);

extern int 
bcm_tr3_cosq_bst_stat_traverse(int unit, 
                               bcm_gport_t port, 
                               bcm_cos_queue_t cosq, 
                               uint32 flags,
                               bcm_cosq_bst_stat_traverse_cb cb,
                               void *user_data);

extern int
bcm_tr3_cosq_gport_mapping_set(int unit, bcm_port_t ing_port,
                              bcm_cos_t int_pri, uint32 flags,
                              bcm_gport_t gport, bcm_cos_queue_t cosq);

extern int
bcm_tr3_cosq_gport_mapping_get(int unit, bcm_port_t ing_port,
                              bcm_cos_t int_pri, uint32 flags,
                              bcm_gport_t *gport, bcm_cos_queue_t *cosq);

extern int bcm_tr3_cosq_sync(int unit);
extern void bcm_tr3_cosq_sw_dump(int unit);

extern int bcm_tr3_oam_init(int unit);

extern int bcm_tr3_oam_detach(int unit);

extern int bcm_tr3_oam_group_create(int unit, bcm_oam_group_info_t *group_info);

extern int bcm_tr3_oam_group_get(int unit, bcm_oam_group_t group,
                                 bcm_oam_group_info_t *group_info);

extern int bcm_tr3_oam_group_destroy(int unit, bcm_oam_group_t group);

extern int bcm_tr3_oam_group_destroy_all(int unit);

extern int bcm_tr3_oam_group_traverse(int unit, bcm_oam_group_traverse_cb cb,
                                      void *user_data);

extern int bcm_tr3_oam_endpoint_create(int unit,
                                       bcm_oam_endpoint_info_t *endpoint_info);

extern int bcm_tr3_oam_endpoint_get(int unit, bcm_oam_endpoint_t endpoint,
                                    bcm_oam_endpoint_info_t *endpoint_info);

extern int bcm_tr3_oam_endpoint_destroy(int unit, bcm_oam_endpoint_t endpoint);

extern int bcm_tr3_oam_endpoint_destroy_all(int unit, bcm_oam_group_t group);

extern int bcm_tr3_oam_endpoint_traverse(int unit, bcm_oam_group_t group,
                                         bcm_oam_endpoint_traverse_cb cb,
                                         void *user_data);

extern int bcm_tr3_oam_event_register(int unit, bcm_oam_event_types_t event_types,
                                      bcm_oam_event_cb cb, void *user_data);

extern int bcm_tr3_oam_event_unregister(int unit, bcm_oam_event_types_t event_types,
                                        bcm_oam_event_cb cb);

extern int bcm_tr3_port_esm_eligible_set(int unit, bcm_port_t port, int value);
extern int bcm_tr3_port_esm_eligible_get(int unit, bcm_port_t port, int *value);
extern int bcm_tr3_port_extender_type_set(int unit, bcm_port_t port,
        int value);
extern int bcm_tr3_port_extender_type_get(int unit, bcm_port_t port,
        int *value);
#if defined(INCLUDE_L3)
extern int bcm_tr3_port_etag_pcp_de_source_set(int unit, bcm_port_t port,
        int value);
extern int bcm_tr3_port_etag_pcp_de_source_get(int unit, bcm_port_t port,
        int *value);
#endif

extern int bcm_tr3_port_ibod_sync_recovery(int unit, bcm_pbmp_t pbmp);

#if defined(INCLUDE_REGEX)
extern int _bcm_tr3_policer_create(int unit, bcm_policer_config_t *pol_cfg,
                            bcm_policer_t *policer_id);
extern int _bcm_tr3_policer_destroy(int unit, bcm_policer_t policer_id);
extern int _bcm_tr3_policer_destroy_all(int unit);
extern int _bcm_tr3_policer_traverse(int unit, bcm_policer_traverse_cb cb, 
                              void *user_data);
#endif

extern int bcm_tr3_port_pps_rate_egress_set(int unit, bcm_port_t port,
                                            uint32 pps, uint32 burst);

extern int bcm_tr3_port_pps_rate_egress_get(int unit, bcm_port_t port,
                                            uint32 *pps, uint32 *burst);

extern int
bcm_tr3_port_rate_egress_set(int unit, bcm_port_t port,
                     uint32 kbits_sec, uint32 kbits_burst, uint32 mode);

extern int
bcm_tr3_port_rate_egress_get(int unit, bcm_port_t port,
                        uint32 *kbits_sec, uint32 *kbits_burst, uint32 *mode);

#if defined(INCLUDE_BHH)
extern int bcm_tr3_oam_loopback_add(int unit, bcm_oam_loopback_t *loopback_p);
extern int bcm_tr3_oam_loopback_get(int unit, bcm_oam_loopback_t *loopback_p);
extern int bcm_tr3_oam_loopback_delete(int unit, bcm_oam_loopback_t *loopback_p);
extern int bcm_tr3_oam_loss_add( int unit, bcm_oam_loss_t *loss_ptr);
extern int bcm_tr3_oam_loss_delete( int unit, bcm_oam_loss_t *loss_ptr);
extern int bcm_tr3_oam_loss_get( int unit, bcm_oam_loss_t *loss_ptr);
extern int bcm_tr3_oam_delay_add( int unit, bcm_oam_delay_t *delay_ptr);
extern int bcm_tr3_oam_delay_delete( int unit, bcm_oam_delay_t *delay_ptr);
extern int bcm_tr3_oam_delay_get( int unit, bcm_oam_delay_t *delay_ptr);
#endif /* INCLUDE_BHH */

/* BHH API */

#ifdef BCM_WARM_BOOT_SUPPORT /* BCM_WARM_BOOT_SUPPORT */
extern int _bcm_tr3_oam_sync(int unit);
#endif /* !BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP /* BCM_WARM_BOOT_SUPPORT_SW_DUMP*/
extern void _bcm_tr3_oam_sw_dump(int unit);
#endif /* !BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif /* BCM_TRIUMPH3_SUPPORT  */

#endif /* !_BCM_INT_TRIUMPH3_H_ */
