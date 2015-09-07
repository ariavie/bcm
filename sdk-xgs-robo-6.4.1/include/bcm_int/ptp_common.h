/*
 * $Id: ptp_common.h,v 1.1 Broadcom SDK $
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
 * File:	ptp_common.h
 * Purpose:	Common PTP API functions
 */

#if defined(INCLUDE_PTP)

#include <soc/defs.h>
#include <soc/drv.h>

#include <sal/core/dpc.h>

#include <bcm/ptp.h>
#include <bcm/error.h>

extern int bcm_common_ptp_acceptable_master_add(int,bcm_ptp_stack_id_t,int,int,int,bcm_ptp_clock_peer_address_t *);
extern int bcm_common_ptp_acceptable_master_enabled_get(int,bcm_ptp_stack_id_t,int,int,uint8 *);
extern int bcm_common_ptp_acceptable_master_enabled_set(int,bcm_ptp_stack_id_t,int,int,uint8);
extern int bcm_common_ptp_acceptable_master_list(int,bcm_ptp_stack_id_t,int,int,int,int *,bcm_ptp_clock_peer_address_t *);
extern int bcm_common_ptp_acceptable_master_remove(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_clock_peer_address_t *);
extern int bcm_common_ptp_acceptable_master_table_clear(int,bcm_ptp_stack_id_t,int,int);
extern int bcm_common_ptp_acceptable_master_table_size_get(int,bcm_ptp_stack_id_t,int,int,int *);
extern int bcm_common_ptp_cb_register(int,bcm_ptp_cb_types_t,bcm_ptp_cb,void *);
extern int bcm_common_ptp_cb_unregister(int,bcm_ptp_cb_types_t,bcm_ptp_cb);
extern int bcm_common_ptp_clock_accuracy_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_clock_accuracy_t *);
extern int bcm_common_ptp_clock_accuracy_set(int,bcm_ptp_stack_id_t,int,bcm_ptp_clock_accuracy_t *);
extern int bcm_common_ptp_clock_create(int,bcm_ptp_stack_id_t,bcm_ptp_clock_info_t *);
extern int bcm_common_ptp_clock_current_dataset_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_current_dataset_t *);
extern int bcm_common_ptp_clock_default_dataset_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_default_dataset_t *);
extern int bcm_common_ptp_clock_domain_get(int,bcm_ptp_stack_id_t,int,uint32 *);
extern int bcm_common_ptp_clock_domain_set(int,bcm_ptp_stack_id_t,int,uint32);
extern int bcm_common_ptp_clock_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_clock_info_t *);
extern int bcm_common_ptp_clock_parent_dataset_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_parent_dataset_t *);
extern int bcm_common_ptp_clock_port_announce_receipt_timeout_get(int,bcm_ptp_stack_id_t,int,uint32,uint32 *);
extern int bcm_common_ptp_clock_port_announce_receipt_timeout_set(int,bcm_ptp_stack_id_t,int,uint32,uint32);
extern int bcm_common_ptp_clock_port_configure(int,bcm_ptp_stack_id_t,int,uint32,bcm_ptp_clock_port_info_t *);
extern int bcm_common_ptp_clock_port_dataset_get(int,bcm_ptp_stack_id_t,int,uint32,bcm_ptp_port_dataset_t *);
extern int bcm_common_ptp_clock_port_delay_mechanism_get(int,bcm_ptp_stack_id_t,int,uint32,uint32 *);
extern int bcm_common_ptp_clock_port_delay_mechanism_set(int,bcm_ptp_stack_id_t,int,uint32,uint32);
extern int bcm_common_ptp_clock_port_disable(int,bcm_ptp_stack_id_t,int,uint32);
extern int bcm_common_ptp_clock_port_enable(int,bcm_ptp_stack_id_t,int,uint32);
extern int bcm_common_ptp_clock_port_identity_get(int,bcm_ptp_stack_id_t,int,uint32,bcm_ptp_port_identity_t *);
extern int bcm_common_ptp_clock_port_info_get(int,bcm_ptp_stack_id_t,int,uint32,bcm_ptp_clock_port_info_t *);
extern int bcm_common_ptp_clock_port_latency_get(int,bcm_ptp_stack_id_t,int,uint32,uint32 *,uint32 *);
extern int bcm_common_ptp_clock_port_latency_set(int,bcm_ptp_stack_id_t,int,uint32,uint32,uint32);
extern int bcm_common_ptp_clock_port_log_announce_interval_get(int,bcm_ptp_stack_id_t,int,uint32,int *);
extern int bcm_common_ptp_clock_port_log_announce_interval_set(int,bcm_ptp_stack_id_t,int,uint32,int);
extern int bcm_common_ptp_clock_port_log_min_delay_req_interval_get(int,bcm_ptp_stack_id_t,int,uint32,int *);
extern int bcm_common_ptp_clock_port_log_min_delay_req_interval_set(int,bcm_ptp_stack_id_t,int,uint32,int);
extern int bcm_common_ptp_clock_port_log_min_pdelay_req_interval_get(int,bcm_ptp_stack_id_t,int,uint32,int *);
extern int bcm_common_ptp_clock_port_log_min_pdelay_req_interval_set(int,bcm_ptp_stack_id_t,int,uint32,int);
extern int bcm_common_ptp_clock_port_log_sync_interval_get(int,bcm_ptp_stack_id_t,int,uint32,int *);
extern int bcm_common_ptp_clock_port_log_sync_interval_set(int,bcm_ptp_stack_id_t,int,uint32,int);
extern int bcm_common_ptp_clock_port_mac_get(int,bcm_ptp_stack_id_t,int,uint32,bcm_mac_t *);
extern int bcm_common_ptp_clock_port_protocol_get(int,bcm_ptp_stack_id_t,int,uint32,bcm_ptp_protocol_t *);
extern int bcm_common_ptp_clock_port_type_get(int,bcm_ptp_stack_id_t,int,uint32,bcm_ptp_port_type_t *);
extern int bcm_common_ptp_clock_port_version_number_get(int,bcm_ptp_stack_id_t,int,uint32,uint32 *);
extern int bcm_common_ptp_clock_priority1_get(int,bcm_ptp_stack_id_t,int,uint32 *);
extern int bcm_common_ptp_clock_priority1_set(int,bcm_ptp_stack_id_t,int,uint32);
extern int bcm_common_ptp_clock_priority2_get(int,bcm_ptp_stack_id_t,int,uint32 *);
extern int bcm_common_ptp_clock_priority2_set(int,bcm_ptp_stack_id_t,int,uint32);
extern int bcm_common_ptp_clock_slaveonly_get(int,bcm_ptp_stack_id_t,int,uint32 *);
extern int bcm_common_ptp_clock_slaveonly_set(int,bcm_ptp_stack_id_t,int,uint32);
extern int bcm_common_ptp_clock_time_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_timestamp_t *);
extern int bcm_common_ptp_clock_time_properties_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_time_properties_t *);
extern int bcm_common_ptp_clock_time_set(int,bcm_ptp_stack_id_t,int,bcm_ptp_timestamp_t *);
extern int bcm_common_ptp_clock_timescale_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_timescale_t *);
extern int bcm_common_ptp_clock_timescale_set(int,bcm_ptp_stack_id_t,int,bcm_ptp_timescale_t *);
extern int bcm_common_ptp_clock_traceability_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_trace_t *);
extern int bcm_common_ptp_clock_traceability_set(int,bcm_ptp_stack_id_t,int,bcm_ptp_trace_t *);
extern int bcm_common_ptp_clock_user_description_set(int,bcm_ptp_stack_id_t,int,uint8 *);
extern int bcm_common_ptp_clock_utc_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_utc_t *);
extern int bcm_common_ptp_clock_utc_set(int,bcm_ptp_stack_id_t,int,bcm_ptp_utc_t *);
extern int bcm_common_ptp_ctdev_alarm_callback_register(int,bcm_ptp_stack_id_t,int,bcm_ptp_ctdev_alarm_cb);
extern int bcm_common_ptp_ctdev_alarm_callback_unregister(int,bcm_ptp_stack_id_t,int);
extern int bcm_common_ptp_ctdev_alpha_get(int,bcm_ptp_stack_id_t,int,uint16 *,uint16 *);
extern int bcm_common_ptp_ctdev_alpha_set(int,bcm_ptp_stack_id_t,int,uint16,uint16);
extern int bcm_common_ptp_ctdev_enable_get(int,bcm_ptp_stack_id_t,int,int *,uint32 *);
extern int bcm_common_ptp_ctdev_enable_set(int,bcm_ptp_stack_id_t,int,int,uint32);
extern int bcm_common_ptp_ctdev_verbose_get(int,bcm_ptp_stack_id_t,int,int *);
extern int bcm_common_ptp_ctdev_verbose_set(int,bcm_ptp_stack_id_t,int,int);
extern int bcm_common_ptp_detach(int);
extern int bcm_common_ptp_foreign_master_dataset_get(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_foreign_master_dataset_t *);
extern int bcm_common_ptp_lock(int);
extern int bcm_common_ptp_unlock(int);
extern int bcm_common_ptp_init(int);
extern int bcm_common_ptp_input_channel_precedence_mode_set(int,bcm_ptp_stack_id_t,int,int);
extern int bcm_common_ptp_input_channel_switching_mode_set(int,bcm_ptp_stack_id_t,int,int);
extern int bcm_common_ptp_input_channels_get(int,bcm_ptp_stack_id_t,int,int *,bcm_ptp_channel_t *);
extern int bcm_common_ptp_input_channels_set(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_channel_t *);
extern int bcm_common_ptp_modular_enable_get(int,bcm_ptp_stack_id_t,int,int *,uint32 *);
extern int bcm_common_ptp_modular_enable_set(int,bcm_ptp_stack_id_t,int,int,uint32);
extern int bcm_common_ptp_modular_phyts_get(int,bcm_ptp_stack_id_t,int,int *,int *);
extern int bcm_common_ptp_modular_phyts_set(int,bcm_ptp_stack_id_t,int,int,int);
extern int bcm_common_ptp_modular_portbitmap_get(int,bcm_ptp_stack_id_t,int,bcm_pbmp_t *);
extern int bcm_common_ptp_modular_portbitmap_set(int,bcm_ptp_stack_id_t,int,bcm_pbmp_t);
extern int bcm_common_ptp_modular_verbose_get(int,bcm_ptp_stack_id_t,int,int *);
extern int bcm_common_ptp_modular_verbose_set(int,bcm_ptp_stack_id_t,int,int);
extern int bcm_common_ptp_packet_counters_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_packet_counters_t *);
extern int bcm_common_ptp_peer_dataset_get(int, bcm_ptp_stack_id_t,int,int,int,bcm_ptp_peer_entry_t *,int *);
extern int bcm_common_ptp_primary_domain_get(int,bcm_ptp_stack_id_t,int,int *);
extern int bcm_common_ptp_primary_domain_set(int,bcm_ptp_stack_id_t,int,int);
extern int bcm_common_ptp_servo_configuration_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_servo_config_t *);
extern int bcm_common_ptp_servo_configuration_set(int,bcm_ptp_stack_id_t,int,bcm_ptp_servo_config_t *);
extern int bcm_common_ptp_servo_status_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_servo_status_t *);
extern int bcm_common_ptp_signal_output_get(int,bcm_ptp_stack_id_t,int,int *,bcm_ptp_signal_output_t *);
extern int bcm_common_ptp_signal_output_remove(int,bcm_ptp_stack_id_t,int,int);
extern int bcm_common_ptp_signal_output_set(int,bcm_ptp_stack_id_t,int,int *,bcm_ptp_signal_output_t *);
extern int bcm_common_ptp_signaled_unicast_master_add(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_clock_unicast_master_t *,uint32);
extern int bcm_common_ptp_signaled_unicast_master_remove(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_clock_peer_address_t *);
extern int bcm_common_ptp_signaled_unicast_slave_list(int,bcm_ptp_stack_id_t,int,int,int,int *,bcm_ptp_clock_peer_t *);
extern int bcm_common_ptp_signaled_unicast_slave_table_clear(int,bcm_ptp_stack_id_t,int,int,int);
extern int bcm_common_ptp_stack_create(int,bcm_ptp_stack_info_t *);
extern int bcm_common_ptp_static_unicast_master_add(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_clock_unicast_master_t *);
extern int bcm_common_ptp_static_unicast_master_list(int,bcm_ptp_stack_id_t,int,int,int,int *,bcm_ptp_clock_peer_address_t *);
extern int bcm_common_ptp_static_unicast_master_remove(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_clock_peer_address_t *);
extern int bcm_common_ptp_static_unicast_master_table_clear(int,bcm_ptp_stack_id_t,int,int);
extern int bcm_common_ptp_static_unicast_master_table_size_get(int,bcm_ptp_stack_id_t,int,int,int *);
extern int bcm_common_ptp_static_unicast_slave_add(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_clock_peer_t *);
extern int bcm_common_ptp_static_unicast_slave_list(int,bcm_ptp_stack_id_t,int,int,int,int *,bcm_ptp_clock_peer_t *);
extern int bcm_common_ptp_static_unicast_slave_remove(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_clock_peer_t *);
extern int bcm_common_ptp_static_unicast_slave_table_clear(int,bcm_ptp_stack_id_t,int,int);
extern int bcm_common_ptp_sync_phy(int,bcm_ptp_stack_id_t,int,bcm_ptp_sync_phy_input_t);
extern int bcm_common_ptp_telecom_g8265_init(int,bcm_ptp_stack_id_t,int);
extern int bcm_common_ptp_telecom_g8265_network_option_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_telecom_g8265_network_option_t *);
extern int bcm_common_ptp_telecom_g8265_network_option_set(int,bcm_ptp_stack_id_t,int,bcm_ptp_telecom_g8265_network_option_t);
extern int bcm_common_ptp_telecom_g8265_packet_master_add(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_clock_port_address_t *);
extern int bcm_common_ptp_telecom_g8265_packet_master_best_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_telecom_g8265_pktmaster_t *);
extern int bcm_common_ptp_telecom_g8265_packet_master_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_clock_port_address_t *,bcm_ptp_telecom_g8265_pktmaster_t *);
extern int bcm_common_ptp_telecom_g8265_packet_master_list(int,bcm_ptp_stack_id_t,int,int,int *,int *,bcm_ptp_telecom_g8265_pktmaster_t *);
extern int bcm_common_ptp_telecom_g8265_packet_master_lockout_set(int,bcm_ptp_stack_id_t,int,uint8,bcm_ptp_clock_port_address_t *);
extern int bcm_common_ptp_telecom_g8265_packet_master_non_reversion_set(int,bcm_ptp_stack_id_t,int,uint8,bcm_ptp_clock_port_address_t *);
extern int bcm_common_ptp_telecom_g8265_packet_master_priority_override(int,bcm_ptp_stack_id_t,int,uint8,bcm_ptp_clock_port_address_t *);
extern int bcm_common_ptp_telecom_g8265_packet_master_priority_set(int,bcm_ptp_stack_id_t,int,uint16,bcm_ptp_clock_port_address_t *);
extern int bcm_common_ptp_telecom_g8265_packet_master_remove(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_clock_port_address_t *);
extern int bcm_common_ptp_telecom_g8265_packet_master_wait_duration_set(int,bcm_ptp_stack_id_t,int,uint64,bcm_ptp_clock_port_address_t *);
extern int bcm_common_ptp_telecom_g8265_pktstats_thresholds_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_telecom_g8265_pktstats_t *);
extern int bcm_common_ptp_telecom_g8265_pktstats_thresholds_set(int,bcm_ptp_stack_id_t,int,bcm_ptp_telecom_g8265_pktstats_t);
extern int bcm_common_ptp_telecom_g8265_quality_level_set(int,bcm_ptp_stack_id_t,int,bcm_ptp_telecom_g8265_quality_level_t);
extern int bcm_common_ptp_telecom_g8265_receipt_timeout_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_message_type_t,uint32 *);
extern int bcm_common_ptp_telecom_g8265_receipt_timeout_set(int,bcm_ptp_stack_id_t,int,bcm_ptp_message_type_t,uint32);
extern int bcm_common_ptp_telecom_g8265_shutdown(int);
extern int bcm_common_ptp_time_format_set(int,bcm_ptp_stack_id_t,bcm_ptp_time_type_t);
extern int bcm_common_ptp_timesource_input_status_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_timesource_status_t *);
extern int bcm_common_ptp_tod_input_sources_get(int,bcm_ptp_stack_id_t,int,int *,bcm_ptp_tod_input_t *);
extern int bcm_common_ptp_tod_input_sources_set(int,bcm_ptp_stack_id_t,int,int,bcm_ptp_tod_input_t *);
extern int bcm_common_ptp_tod_output_get(int,bcm_ptp_stack_id_t,int,int *,bcm_ptp_tod_output_t *);
extern int bcm_common_ptp_tod_output_remove(int,bcm_ptp_stack_id_t,int,int);
extern int bcm_common_ptp_tod_output_set(int,bcm_ptp_stack_id_t,int,int *,bcm_ptp_tod_output_t *);
extern int bcm_common_ptp_synce_output_set(int,bcm_ptp_stack_id_t,int,int,int);
extern int bcm_common_ptp_synce_output_get(int,bcm_ptp_stack_id_t,int,int *);
extern int bcm_common_ptp_transparent_clock_default_dataset_get(int,bcm_ptp_stack_id_t,int,bcm_ptp_transparent_clock_default_dataset_t *);
extern int bcm_common_ptp_transparent_clock_port_dataset_get(int,bcm_ptp_stack_id_t,int,uint16,bcm_ptp_transparent_clock_port_dataset_t *);
extern int bcm_common_ptp_unicast_request_duration_get(int,bcm_ptp_stack_id_t,int,int,uint32 *);
extern int bcm_common_ptp_unicast_request_duration_max_get(int,bcm_ptp_stack_id_t,int,int,uint32 *);
extern int bcm_common_ptp_unicast_request_duration_max_set(int,bcm_ptp_stack_id_t,int,int,uint32);
extern int bcm_common_ptp_unicast_request_duration_min_get(int,bcm_ptp_stack_id_t,int,int,uint32 *);
extern int bcm_common_ptp_unicast_request_duration_min_set(int,bcm_ptp_stack_id_t,int,int,uint32);
extern int bcm_common_ptp_unicast_request_duration_set(int,bcm_ptp_stack_id_t,int,int,uint32);

extern int bcm_common_esmc_tx(int, int, bcm_pbmp_t, bcm_esmc_pdu_data_t *);
extern int bcm_common_esmc_rx_callback_register(int, int, bcm_esmc_rx_cb);
extern int bcm_common_esmc_rx_callback_unregister(int, int);
extern int bcm_common_esmc_tunnel_get(int, int, int *);
extern int bcm_common_esmc_tunnel_set(int, int, int);
extern int bcm_common_esmc_g781_option_get(int, int, bcm_esmc_network_option_t *);
extern int bcm_common_esmc_g781_option_set(int, int, bcm_esmc_network_option_t);
extern int bcm_common_esmc_QL_SSM_map(int, bcm_esmc_network_option_t, bcm_esmc_quality_level_t, uint8 *);
extern int bcm_common_esmc_SSM_QL_map(int, bcm_esmc_network_option_t, uint8, bcm_esmc_quality_level_t *);

extern int bcm_common_tdpll_dpll_bindings_get(int, int, int, bcm_tdpll_dpll_bindings_t *);
extern int bcm_common_tdpll_dpll_bindings_set(int, int, int, bcm_tdpll_dpll_bindings_t *);
extern int bcm_common_tdpll_dpll_reference_get(int, int, int, int *, int *);
extern int bcm_common_tdpll_dpll_bandwidth_get(int, int, int, bcm_tdpll_dpll_bandwidth_t *);
extern int bcm_common_tdpll_dpll_bandwidth_set(int, int, int, bcm_tdpll_dpll_bandwidth_t *);
extern int bcm_common_tdpll_dpll_phase_control_get(int, int, int, bcm_tdpll_dpll_phase_control_t *);
extern int bcm_common_tdpll_dpll_phase_control_set(int, int, int, bcm_tdpll_dpll_phase_control_t *);

extern int bcm_common_tdpll_input_clock_control(int, int, int);
extern int bcm_common_tdpll_input_clock_mac_get(int, int, int, bcm_mac_t *);
extern int bcm_common_tdpll_input_clock_mac_set(int, int, int, bcm_mac_t *);
extern int bcm_common_tdpll_input_clock_frequency_error_get(int, int, int, int *);
extern int bcm_common_tdpll_input_clock_threshold_state_get(int, int, int, bcm_tdpll_input_clock_monitor_type_t, int *);
extern int bcm_common_tdpll_input_clock_enable_get(int, int, int, int *);
extern int bcm_common_tdpll_input_clock_enable_set(int, int, int, int);
extern int bcm_common_tdpll_input_clock_l1mux_get(int, int, int, bcm_tdpll_input_clock_l1mux_t *);
extern int bcm_common_tdpll_input_clock_l1mux_set(int, int, int, bcm_tdpll_input_clock_l1mux_t *);
extern int bcm_common_tdpll_input_clock_valid_get(int, int, int, int *);
extern int bcm_common_tdpll_input_clock_valid_set(int, int, int, int);
extern int bcm_common_tdpll_input_clock_frequency_get(int, int, int, uint32 *, uint32 *);
extern int bcm_common_tdpll_input_clock_frequency_set(int, int, int, uint32, uint32);
extern int bcm_common_tdpll_input_clock_ql_get(int, int, int, bcm_esmc_quality_level_t *);
extern int bcm_common_tdpll_input_clock_ql_set(int, int, int, bcm_esmc_quality_level_t);
extern int bcm_common_tdpll_input_clock_priority_get(int, int, int, int *);
extern int bcm_common_tdpll_input_clock_priority_set(int, int, int, int);
extern int bcm_common_tdpll_input_clock_lockout_get(int, int, int, int *);
extern int bcm_common_tdpll_input_clock_lockout_set(int, int, int, int);
extern int bcm_common_tdpll_input_clock_monitor_interval_get(int, int, uint32 *);
extern int bcm_common_tdpll_input_clock_monitor_interval_set(int, int, uint32);
extern int bcm_common_tdpll_input_clock_monitor_threshold_get(int, int, bcm_tdpll_input_clock_monitor_type_t, uint32 *);
extern int bcm_common_tdpll_input_clock_monitor_threshold_set(int, int, bcm_tdpll_input_clock_monitor_type_t, uint32);
extern int bcm_common_tdpll_input_clock_ql_enabled_get(int, int, int, int *);
extern int bcm_common_tdpll_input_clock_ql_enabled_set(int, int, int, int);
extern int bcm_common_tdpll_input_clock_revertive_get(int, int, int, int *);
extern int bcm_common_tdpll_input_clock_revertive_set(int, int, int, int);
extern int bcm_common_tdpll_input_clock_best_get(int, int, int, int *);
extern int bcm_common_tdpll_input_clock_monitor_callback_register(int, int, bcm_tdpll_input_clock_monitor_cb);
extern int bcm_common_tdpll_input_clock_monitor_callback_unregister(int, int);
extern int bcm_common_tdpll_input_clock_selector_callback_register(int, int, bcm_tdpll_input_clock_selector_cb);
extern int bcm_common_tdpll_input_clock_selector_callback_unregister(int, int);

extern int bcm_common_tdpll_output_clock_enable_get(int, int, int, int *);
extern int bcm_common_tdpll_output_clock_enable_set(int, int, int, int);
extern int bcm_common_tdpll_output_clock_synth_frequency_get(int, int, int, uint32 *, uint32 *);
extern int bcm_common_tdpll_output_clock_synth_frequency_set(int, int, int, uint32, uint32);
extern int bcm_common_tdpll_output_clock_deriv_frequency_get(int, int, int, uint32 *);
extern int bcm_common_tdpll_output_clock_deriv_frequency_set(int, int, int, uint32);
extern int bcm_common_tdpll_output_clock_holdover_data_get(int, int, int, bcm_tdpll_holdover_data_t *);
extern int bcm_common_tdpll_output_clock_holdover_frequency_set(int, int, int, bcm_tdpll_frequency_correction_t);
extern int bcm_common_tdpll_output_clock_holdover_mode_get(int, int, int, bcm_tdpll_holdover_mode_t *);
extern int bcm_common_tdpll_output_clock_holdover_mode_set(int, int, int, bcm_tdpll_holdover_mode_t);
extern int bcm_common_tdpll_output_clock_holdover_reset(int, int, int);

extern int bcm_common_tdpll_esmc_rx_state_machine(int, int, int, bcm_esmc_pdu_data_t *);
extern int bcm_common_tdpll_esmc_ql_get(int, int, int, bcm_esmc_quality_level_t *);
extern int bcm_common_tdpll_esmc_ql_set(int, int, int, bcm_esmc_quality_level_t);
extern int bcm_common_tdpll_esmc_holdover_ql_get(int, int, int, bcm_esmc_quality_level_t *);
extern int bcm_common_tdpll_esmc_holdover_ql_set(int, int, int, bcm_esmc_quality_level_t);
extern int bcm_common_tdpll_esmc_mac_get(int, int, int, bcm_mac_t *);
extern int bcm_common_tdpll_esmc_mac_set(int, int, int, bcm_mac_t *);
extern int bcm_common_tdpll_esmc_rx_enable_get(int, int, int *);
extern int bcm_common_tdpll_esmc_rx_enable_set(int, int, int);
extern int bcm_common_tdpll_esmc_tx_enable_get(int, int, int, int *);
extern int bcm_common_tdpll_esmc_tx_enable_set(int, int, int, int);
extern int bcm_common_tdpll_esmc_rx_portbitmap_get(int, int, int, bcm_pbmp_t *);
extern int bcm_common_tdpll_esmc_rx_portbitmap_set(int, int, int, bcm_pbmp_t);
extern int bcm_common_tdpll_esmc_tx_portbitmap_get(int, int, int, bcm_pbmp_t *);
extern int bcm_common_tdpll_esmc_tx_portbitmap_set(int, int, int, bcm_pbmp_t);

#endif	/* defined(INCLUDE_PTP) */

