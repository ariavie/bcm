/*
 * $Id: katana2.h 1.33.6.1 Broadcom SDK $
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
 * File:        katana2.h
 * Purpose:     Function declarations for Trident  bcm functions
 */

#ifndef _BCM_INT_KATANA2_H_
#define _BCM_INT_KATANA2_H_
#if defined(BCM_KATANA2_SUPPORT)
#include <bcm_int/esw/subport.h>
#include <bcm_int/esw/firebolt.h>
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/oam.h>
#include <bcm_int/esw/field.h>
#include <bcm/qos.h>
#include <bcm/failover.h>

extern int bcm_kt2_port_cfg_init(int, bcm_port_t, bcm_vlan_data_t *);

#if defined(BCM_FIELD_SUPPORT)
extern int _bcm_field_kt2_init(int unit, _field_control_t *fc);
extern int _bcm_field_kt2_qual_tcam_key_mask_get(int unit,
                                                 _field_entry_t *f_ent,
                                                 _field_tcam_t  *tcam
                                                 );
extern int _bcm_field_kt2_qual_tcam_key_mask_set(int unit,
                                                 _field_entry_t *f_ent,
                                                 unsigned       validf
                                                 );
extern int _bcm_field_kt2_qualify_class (int unit, 
                                         bcm_field_entry_t entry, 
                                         bcm_field_qualify_t qual, 
                                         uint32 *data, 
                                         uint32 *mask 
                                        ); 
extern int soc_katana2_port_lanes_set_post_operation(int unit, bcm_port_t port);
extern int _bcm_kt2_port_lanes_set_post_operation(int unit, bcm_port_t port);
extern int _bcm_kt2_port_lanes_set(int unit, bcm_port_t port, int value);
extern int _bcm_kt2_port_lanes_get(int unit, bcm_port_t port, int *value);

#endif /* BCM_FIELD_SUPPORT */

extern int bcm_kt2_subport_init(int unit);
extern int bcm_kt2_subport_cleanup(int unit);
extern int bcm_kt2_subport_group_create(int unit,
                                     bcm_subport_group_config_t *config,
                                     bcm_gport_t *group);
extern int bcm_kt2_subport_group_destroy(int unit, bcm_gport_t group);
extern int bcm_kt2_subport_group_get(int unit, bcm_gport_t group,
                                     bcm_subport_group_config_t *config);
extern int bcm_kt2_subport_group_traverse(int unit, bcm_gport_t group,
                                     bcm_subport_port_traverse_cb cb,
                                     void *user_data);
extern int bcm_kt2_subport_port_add(int unit, bcm_subport_config_t *config,
                                   bcm_gport_t *port);
extern int bcm_kt2_subport_port_delete(int unit, bcm_gport_t port);
extern int bcm_kt2_subport_port_get(int unit, bcm_gport_t port,
                                   bcm_subport_config_t *config);
extern int bcm_kt2_subport_port_traverse(int unit,
                                      bcm_subport_port_traverse_cb cb,
                                        void *user_data);
extern int bcm_kt2_subport_group_linkphy_config_get(int unit, 
                          bcm_gport_t group, 
                          bcm_subport_group_linkphy_config_t *linkphy_config);
extern int bcm_kt2_subport_group_linkphy_config_set(int unit,
                          bcm_gport_t group,
                          bcm_subport_group_linkphy_config_t *linkphy_config);
extern int bcm_kt2_subport_port_stat_set(int unit, 
                          bcm_gport_t port, 
                          int stream_id, 
                          bcm_subport_stat_t stat_type, 
                          uint64 val);
extern int bcm_kt2_subport_port_stat_get(
                          int unit, 
                          bcm_gport_t port, 
                          int stream_id, 
                          bcm_subport_stat_t stat_type, 
                          uint64 *val);
extern int bcm_kt2_subport_port_stat_show(int unit, uint32 flag, int sid);
extern int bcm_kt2_subport_group_resolve(int unit,
                              bcm_gport_t subport_group_gport,
                              bcm_module_t *modid, bcm_port_t *port,
                              bcm_trunk_t *trunk_id, int *id);
extern int bcm_kt2_subport_port_resolve(int unit,
                              bcm_gport_t subport_port_gport,
                              bcm_module_t *modid, bcm_port_t *port,
                              bcm_trunk_t *trunk_id, int *id);
extern int bcm_kt2_subport_pp_port_subport_info_get(int unit,
                              bcm_port_t pp_port,
                              _bcm_kt2_subport_info_t *subport_info);
extern int bcm_kt2_subport_counter_init(int unit);
extern int bcm_kt2_subport_counter_cleanup(int unit);

extern int bcm_kt2_subport_egr_subtag_dot1p_map_add(int unit,
                                        bcm_qos_map_t *map);
extern int bcm_kt2_subport_egr_subtag_dot1p_map_delete(int unit,
                                        bcm_qos_map_t *map);
extern int bcm_kt2_subport_subtag_port_tpid_set(int unit,
                                 bcm_gport_t gport, uint16 tpid);
extern int bcm_kt2_subport_subtag_port_tpid_get(int unit,
                                 bcm_gport_t gport, uint16 *tpid);
extern int bcm_kt2_subport_subtag_port_tpid_delete(int unit,
                                 bcm_gport_t gport, uint16 tpid);
extern int bcm_kt2_port_control_subtag_status_set(int unit,
                                 bcm_port_t port, int value);
extern int bcm_kt2_port_control_subtag_status_get(int unit,
                                 bcm_port_t port, int *value);
extern int bcm_kt2_oam_init(int unit);

extern int bcm_kt2_oam_detach(int unit);

extern int bcm_kt2_oam_group_create(int unit, bcm_oam_group_info_t *group_info);

extern int bcm_kt2_oam_group_get(int unit, bcm_oam_group_t group,
                                 bcm_oam_group_info_t *group_info);

extern int bcm_kt2_oam_group_destroy(int unit, bcm_oam_group_t group);

extern int bcm_kt2_oam_group_destroy_all(int unit);

extern int bcm_kt2_oam_group_traverse(int unit, bcm_oam_group_traverse_cb cb,
                                      void *user_data);

extern int bcm_kt2_oam_endpoint_create(int unit,
                                       bcm_oam_endpoint_info_t *endpoint_info);

extern int bcm_kt2_oam_endpoint_get(int unit, bcm_oam_endpoint_t endpoint,
                                    bcm_oam_endpoint_info_t *endpoint_info);

extern int bcm_kt2_oam_endpoint_destroy(int unit, bcm_oam_endpoint_t endpoint);

extern int bcm_kt2_oam_endpoint_destroy_all(int unit, bcm_oam_group_t group);

extern int bcm_kt2_oam_endpoint_traverse(int unit, bcm_oam_group_t group,
                                         bcm_oam_endpoint_traverse_cb cb,
                                         void *user_data);

extern int bcm_kt2_oam_event_register(int unit, 
                                      bcm_oam_event_types_t event_types,
                                      bcm_oam_event_cb cb, void *user_data);

extern int bcm_kt2_oam_event_unregister(int unit, 
                                      bcm_oam_event_types_t event_types,
                                      bcm_oam_event_cb cb);

extern int bcm_kt2_oam_endpoint_action_set(int unit, bcm_oam_endpoint_t id, 
                                           bcm_oam_endpoint_action_t *action); 

extern int _bcm_kt2_port_control_oam_loopkup_with_dvp_set(int unit, 
                                                          bcm_port_t port, 
                                                          int val);
extern int _bcm_kt2_port_control_oam_loopkup_with_dvp_get(int unit, 
                                                          bcm_port_t port, 
                                                          int *val);
extern int bcm_kt2_stg_stp_init(int unit, bcm_stg_t stg);
extern int bcm_kt2_stg_stp_set(int unit, bcm_stg_t stg, 
                         bcm_port_t port, int stp_state);
extern int bcm_kt2_stg_stp_get(int unit, bcm_stg_t stg, 
                         bcm_port_t port, int *stp_state);
extern int bcm_kt2_oam_control_get(int unit, 
                                   bcm_oam_control_type_t type,
                                   uint64  *arg); 
extern int bcm_kt2_oam_control_set(int unit, 
                                   bcm_oam_control_type_t type,
                                   uint64 arg); 

#ifdef BCM_WARM_BOOT_SUPPORT /* BCM_WARM_BOOT_SUPPORT */
extern int _bcm_kt2_oam_sync(int unit);
extern int _bcm_kt2_ipmc_repl_reload(int unit);
extern void bcm_kt2_ipmc_subscriber_sw_dump(int unit);
#endif /* !BCM_WARM_BOOT_SUPPORT */
extern int _bcm_kt2_init_check(int unit, int ipmc_id);
extern int bcm_kt2_ipmc_repl_init(int unit);
extern int
_bcm_kt2_ipmc_egress_intf_set(int unit, int repl_group, bcm_port_t port,
                             int ipmc_ptr, int if_count, bcm_if_t *if_array,
                             int *new_ipmc_ptr, int *last_ipmc_flag,
                             int check_port);
extern int
_bcm_kt2_ipmc_egress_intf_add(int unit, int ipmc_id, bcm_port_t port,
                              int ipmc_ptr, int id, int  *new_ipmc_ptr,
                              int *last_flag);
extern int
_bcm_kt2_ipmc_egress_intf_get(int unit, int repl_group,
                             int ipmc_ptr, bcm_if_t *if_array,
                             int *if_count);
extern int
_bcm_kt2_ipmc_egress_intf_delete(int unit, int ipmc_id, bcm_port_t port,
                                int ipmc_ptr, int id, int  *new_ipmc_ptr,
                                int *last_flag);

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP /* BCM_WARM_BOOT_SUPPORT_SW_DUMP*/
extern void _bcm_kt2_oam_sw_dump(int unit);
#endif /* !BCM_WARM_BOOT_SUPPORT_SW_DUMP */

typedef enum {
    _BCM_KT2_COSQ_INDEX_STYLE_BUCKET,
    _BCM_KT2_COSQ_INDEX_STYLE_WRED,
    _BCM_KT2_COSQ_INDEX_STYLE_SCHEDULER,
    _BCM_KT2_COSQ_INDEX_STYLE_UCAST_QUEUE,
    _BCM_KT2_COSQ_INDEX_STYLE_PERQ_XMT
}_bcm_kt2_cosq_index_style_t;

extern int bcm_kt2_cosq_init(int unit);
extern int bcm_kt2_cosq_detach(int unit, int software_state_only);
extern int bcm_kt2_cosq_config_set(int unit, int numq);
extern int bcm_kt2_cosq_config_get(int unit, int *numq);
extern int bcm_kt2_cosq_mapping_set(int unit, bcm_port_t port,
                                   bcm_cos_t priority, bcm_cos_queue_t cosq);
extern int bcm_kt2_cosq_mapping_get(int unit, bcm_port_t port,
                                   bcm_cos_t priority, bcm_cos_queue_t *cosq);
extern int bcm_kt2_cosq_port_sched_set(int unit, bcm_pbmp_t pbm,
                                      int mode, const int *weights, int delay);
extern int bcm_kt2_cosq_port_sched_get(int unit, bcm_pbmp_t pbm,
                                      int *mode, int *weights, int *delay);
extern int bcm_kt2_cosq_sched_weight_max_get(int unit, int mode,
                                            int *weight_max);
extern int bcm_kt2_cosq_port_bandwidth_set(int unit, bcm_port_t port,
                                          bcm_cos_queue_t cosq,
                                          uint32 min_quantum,
                                          uint32 max_quantum,
                                          uint32 burst_quantum,
                                          uint32 flags);
extern int bcm_kt2_cosq_port_bandwidth_get(int unit, bcm_port_t port,
                                          bcm_cos_queue_t cosq,
                                          uint32 *min_quantum,
                                          uint32 *max_quantum,
                                          uint32 *burst_quantum,
                                          uint32 *flags);
extern int bcm_kt2_cosq_discard_set(int unit, uint32 flags);
extern int bcm_kt2_cosq_discard_get(int unit, uint32 *flags);
extern int bcm_kt2_cosq_discard_port_set(int unit, bcm_port_t port,
                                        bcm_cos_queue_t cosq, uint32 color,
                                        int drop_start, int drop_slope,
                                        int average_time);
extern int bcm_kt2_cosq_discard_port_get(int unit, bcm_port_t port,
                                        bcm_cos_queue_t cosq, uint32 color,
                                        int *drop_start, int *drop_slope,
                                        int *average_time);
extern int bcm_kt2_cosq_gport_add(int unit, bcm_gport_t port, int numq,
                                 uint32 flags, bcm_gport_t *gport);
extern int bcm_kt2_cosq_gport_delete(int unit, bcm_gport_t gport);
extern int bcm_kt2_cosq_gport_get(int unit, bcm_gport_t gport,
                                 bcm_gport_t *port, int *numq, uint32 *flags);
extern int bcm_kt2_cosq_gport_traverse(int unit, bcm_cosq_gport_traverse_cb cb,
                                      void *user_data);
extern int bcm_kt2_cosq_gport_sched_set(int unit, bcm_gport_t gport,
                                       bcm_cos_queue_t cosq, int mode,
                                       int weight);
extern int bcm_kt2_cosq_gport_sched_get(int unit, bcm_gport_t gport,
                                       bcm_cos_queue_t cosq,
                                       int *mode, int *weight);
extern int bcm_kt2_cosq_gport_attach(int unit, bcm_gport_t sched_gport,
                                    bcm_gport_t input_gport,
                                    bcm_cos_queue_t cosq);
extern int bcm_kt2_cosq_gport_detach(int unit, bcm_gport_t sched_gport,
                                    bcm_gport_t input_gport,
                                    bcm_cos_queue_t cosq);
extern int bcm_kt2_cosq_gport_bandwidth_set(int unit, bcm_gport_t gport,
                                           bcm_cos_queue_t cosq,
                                           uint32 kbits_sec_min,
                                           uint32 kbits_sec_max, uint32 flags);
extern int bcm_kt2_cosq_gport_bandwidth_get(int unit, bcm_gport_t gport,
                                           bcm_cos_queue_t cosq,
                                           uint32 *kbits_sec_min,
                                           uint32 *kbits_sec_max,
                                           uint32 *flags);
extern int bcm_kt2_cosq_gport_bandwidth_burst_set(int unit, bcm_gport_t gport,
                                                 bcm_cos_queue_t cosq,
                                                 uint32 kbits_burst_min,
                                                 uint32 kbits_burst_max);
extern int bcm_kt2_cosq_gport_bandwidth_burst_get(int unit, bcm_gport_t gport,
                                                 bcm_cos_queue_t cosq,
                                                 uint32 *kbits_burst_min,
                                                 uint32 *kbits_burst_max);
extern int bcm_kt2_cosq_gport_attach_get(int unit, bcm_gport_t sched_gport,
                                        bcm_gport_t *input_gport,
                                        bcm_cos_queue_t *cosq);
extern int bcm_kt2_cosq_gport_discard_set(int unit, bcm_gport_t gport,
                                         bcm_cos_queue_t cosq,
                                         bcm_cosq_gport_discard_t *discard);
extern int bcm_kt2_cosq_gport_discard_get(int unit, bcm_gport_t gport,
                                         bcm_cos_queue_t cosq,
                                         bcm_cosq_gport_discard_t *discard);
extern int bcm_kt2_cosq_stat_set(int unit, bcm_gport_t port, 
                                bcm_cos_queue_t cosq,
                                bcm_cosq_stat_t stat, uint64 value);
extern int bcm_kt2_cosq_stat_get(int unit, bcm_gport_t port, 
                                bcm_cos_queue_t cosq,
                                bcm_cosq_stat_t stat, uint64 *value);
extern int bcm_kt2_cosq_gport_stat_set(int unit, bcm_gport_t port, 
                                bcm_cos_queue_t cosq,
                                bcm_cosq_gport_stats_t stat, uint64 value);
extern int bcm_kt2_cosq_gport_stat_get(int unit, bcm_gport_t port, 
                                bcm_cos_queue_t cosq,
                                bcm_cosq_gport_stats_t stat, uint64 *value);
extern int bcm_kt2_cosq_port_get(int unit, int queue_id, bcm_port_t *port);
extern int _bcm_kt2_cosq_index_resolve(int unit, bcm_port_t port,
                           bcm_cos_queue_t cosq,
                           _bcm_kt2_cosq_index_style_t style,
                           bcm_port_t *local_port, int *index, int *count);
#ifndef BCM_KATANA_SUPPORT
extern int bcm_kt_port_rate_egress_set(int unit, bcm_port_t port,
                           uint32 kbits_sec, uint32 kbits_burst, uint32 mode);
extern int bcm_kt_port_rate_egress_get(int unit, bcm_port_t port,
                           uint32 *kbits_sec, uint32 *kbits_burst, uint32 *mode);
extern int bcm_kt_port_pps_rate_egress_set(int unit, bcm_port_t port,
                                           uint32 pps, uint32 burst);
#endif
extern int bcm_kt2_cosq_drop_status_enable_set(int unit, bcm_port_t port,
                                              int enable);
extern int bcm_kt2_cosq_control_set(int unit, bcm_gport_t gport,
                                   bcm_cos_queue_t cosq,
                                   bcm_cosq_control_t type, int arg);

extern int bcm_kt2_cosq_control_get(int unit, bcm_gport_t gport,
                                   bcm_cos_queue_t cosq,
                                   bcm_cosq_control_t type, int *arg);
extern int _bcm_kt2_cosq_port_resolve(int unit, bcm_gport_t gport,
                                     bcm_module_t *modid, bcm_port_t *port,
                                     bcm_trunk_t *trunk_id, int *id,
                                     int *qnum);
extern int
bcm_kt2_cosq_port_pps_set(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                         int pps);
extern int
bcm_kt2_cosq_port_burst_set(int unit, bcm_port_t port, bcm_cos_queue_t cosq,
                           int burst);

/* Routines to program IFP_COS_MAP table. */
extern int
bcm_kt2_cosq_field_classifier_id_create(int unit,
                                        bcm_cosq_classifier_t *classifier,
                                        int *classifier_id);
extern int 
bcm_kt2_cosq_field_classifier_map_set(int unit,
                                      int classifier_id,
                                      int array_count,
                                      bcm_cos_t *priority_array,
                                      bcm_cos_queue_t *cosq_array);
extern int
bcm_kt2_cosq_field_classifier_map_get(int unit,
                                      int classifier_id,
                                      int array_max,
                                      bcm_cos_t *priority_array,
                                      bcm_cos_queue_t *cosq_array,
                                      int *array_count);
extern int
bcm_kt2_cosq_field_classifier_map_clear(int unit, int classifier_id);

extern int
bcm_kt2_cosq_field_classifier_id_destroy(int unit, int classifier_id);

extern int
bcm_kt2_cosq_field_classifier_get(int unit, int classifier_id,
                                  bcm_cosq_classifier_t *classifier);

/* Routines to program SERVICE_COS_MAP table. */
extern int 
bcm_kt2_cosq_service_classifier_id_create(int unit,
        bcm_cosq_classifier_t *classifier,
        int *classifier_id);

extern int 
bcm_kt2_cosq_service_classifier_id_destroy(int unit,
        int classifier_id);

extern int 
bcm_kt2_cosq_service_classifier_get(int unit,
        int classifier_id,
        bcm_cosq_classifier_t *classifier);

extern int 
bcm_kt2_cosq_service_map_set(int unit, bcm_port_t port,
        int classifier_id,
        bcm_gport_t queue_group,
        int array_count,
        bcm_cos_t *priority_array,
        bcm_cos_queue_t *cosq_array);

extern int 
bcm_kt2_cosq_service_map_get(int unit, bcm_port_t port,
        int classifier_id,
        bcm_gport_t *queue_group,
        int array_max,
        bcm_cos_t *priority_array,
        bcm_cos_queue_t *cosq_array,
        int *array_count);

extern int 
bcm_kt2_cosq_service_map_clear(int unit,
        bcm_gport_t port,
        int classifier_id);


#if defined(INCLUDE_L3)
extern int bcm_kt2_failover_prot_nhi_cleanup(int unit,
                                                         int nh_index);
extern int bcm_kt2_failover_status_set(int unit,
                                            bcm_failover_element_t *failover,
                                            int value);
extern int bcm_kt2_failover_status_get(int unit,
                                            bcm_failover_element_t *failover,
                                            int  *value);

#endif /* INCLUDE_L3 */

#ifdef BCM_WARM_BOOT_SUPPORT
extern int bcm_kt2_cosq_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void bcm_kt2_cosq_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

extern int
bcm_kt2_modid_set(int unit, int *my_modid_list, int *my_modid_valid_list ,
        int *base_port_ptr_list);

extern int 
bcm_kt2_modid_get(int unit, int *my_modid_list, int *my_modid_valid_list ,
        int *base_port_ptr_list);
extern int
_bcm_kt2_pp_port_to_modport_get(int unit, bcm_port_t pp_port, int *modid,
                              bcm_port_t *port);
extern int
_bcm_kt2_modport_to_pp_port_get(int unit, int modid, bcm_port_t port,
                            bcm_port_t *pp_port);
extern int
_bcm_kt2_port_gport_validate(int unit, bcm_port_t port_in, bcm_port_t *port_out);

extern void
_bcm_kt2_subport_pbmp_update(int unit, bcm_pbmp_t *pbmp);

extern int
_bcm_kt2_flexio_pbmp_update(int unit, bcm_pbmp_t *pbmp);

extern int
bcm_kt2_port_dscp_map_set(int unit, bcm_port_t port, int srccp,
                            int mapcp, int prio, int cng);

extern int
_bcm_kt2_vlan_protocol_data_update(
    int unit, bcm_pbmp_t update_pbm, int prot_idx,
    bcm_vlan_action_set_t *action);

extern int
_bcm_kt2_vlan_port_default_action_set(int unit, bcm_port_t port,
                                      bcm_vlan_action_set_t *action);

extern int
_bcm_kt2_vlan_port_default_action_delete(int unit, bcm_port_t port);

#define KT2_MAX_MODIDS_PER_TRANSLATION_TABLE 4
#define KT2_MAX_PP_PORT_GPP_TRANSLATION_TABLES 4 
#define KT2_MAX_EGR_PP_PORT_GPP_TRANSLATION_TABLES 2 
#define KT2_MAX_PHYSICAL_PORTS 40

#endif /* BCM_KATANA2_SUPPORT */
#endif  /* !_BCM_INT_KATANA2_H_ */

