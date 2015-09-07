/*
 * $Id: ptp.c 1.2 Broadcom SDK $
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
 * File:    ptp.c
 *
 * Purpose: 
 *
 * Functions:
 *      bcm_esw_ptp_acceptable_master_add
 *      bcm_esw_ptp_acceptable_master_enabled_get
 *      bcm_esw_ptp_acceptable_master_enabled_set
 *      bcm_esw_ptp_acceptable_master_list
 *      bcm_esw_ptp_acceptable_master_remove
 *      bcm_esw_ptp_acceptable_master_table_clear
 *      bcm_esw_ptp_acceptable_master_table_size_get
 *      bcm_esw_ptp_cb_register
 *      bcm_esw_ptp_cb_unregister
 *      bcm_esw_ptp_clock_accuracy_get
 *      bcm_esw_ptp_clock_accuracy_set
 *      bcm_esw_ptp_clock_create
 *      bcm_esw_ptp_clock_current_dataset_get
 *      bcm_esw_ptp_clock_default_dataset_get
 *      bcm_esw_ptp_clock_domain_get
 *      bcm_esw_ptp_clock_domain_set
 *      bcm_esw_ptp_clock_get
 *      bcm_esw_ptp_clock_parent_dataset_get
 *      bcm_esw_ptp_clock_port_announce_receipt_timeout_get
 *      bcm_esw_ptp_clock_port_announce_receipt_timeout_set
 *      bcm_esw_ptp_clock_port_configure
 *      bcm_esw_ptp_clock_port_dataset_get
 *      bcm_esw_ptp_clock_port_delay_mechanism_get
 *      bcm_esw_ptp_clock_port_delay_mechanism_set
 *      bcm_esw_ptp_clock_port_disable
 *      bcm_esw_ptp_clock_port_enable
 *      bcm_esw_ptp_clock_port_identity_get
 *      bcm_esw_ptp_clock_port_info_get
 *      bcm_esw_ptp_clock_port_latency_get
 *      bcm_esw_ptp_clock_port_latency_set
 *      bcm_esw_ptp_clock_port_log_announce_interval_get
 *      bcm_esw_ptp_clock_port_log_announce_interval_set
 *      bcm_esw_ptp_clock_port_log_min_delay_req_interval_get
 *      bcm_esw_ptp_clock_port_log_min_delay_req_interval_set
 *      bcm_esw_ptp_clock_port_log_min_pdelay_req_interval_get
 *      bcm_esw_ptp_clock_port_log_min_pdelay_req_interval_set
 *      bcm_esw_ptp_clock_port_log_sync_interval_get
 *      bcm_esw_ptp_clock_port_log_sync_interval_set
 *      bcm_esw_ptp_clock_port_mac_get
 *      bcm_esw_ptp_clock_port_protocol_get
 *      bcm_esw_ptp_clock_port_type_get
 *      bcm_esw_ptp_clock_port_version_number_get
 *      bcm_esw_ptp_clock_priority1_get
 *      bcm_esw_ptp_clock_priority1_set
 *      bcm_esw_ptp_clock_priority2_get
 *      bcm_esw_ptp_clock_priority2_set
 *      bcm_esw_ptp_clock_slaveonly_get
 *      bcm_esw_ptp_clock_slaveonly_set
 *      bcm_esw_ptp_clock_time_get
 *      bcm_esw_ptp_clock_time_properties_get
 *      bcm_esw_ptp_clock_time_set
 *      bcm_esw_ptp_clock_timescale_get
 *      bcm_esw_ptp_clock_timescale_set
 *      bcm_esw_ptp_clock_traceability_get
 *      bcm_esw_ptp_clock_traceability_set
 *      bcm_esw_ptp_clock_user_description_set
 *      bcm_esw_ptp_clock_utc_get
 *      bcm_esw_ptp_clock_utc_set
 *      bcm_esw_ptp_ctdev_alarm_callback_register
 *      bcm_esw_ptp_ctdev_alarm_callback_unregister
 *      bcm_esw_ptp_ctdev_alpha_get
 *      bcm_esw_ptp_ctdev_alpha_set
 *      bcm_esw_ptp_ctdev_enable_get
 *      bcm_esw_ptp_ctdev_enable_set
 *      bcm_esw_ptp_ctdev_verbose_get
 *      bcm_esw_ptp_ctdev_verbose_set
 *      bcm_esw_ptp_detach
 *      bcm_esw_ptp_foreign_master_dataset_get
 *      bcm_esw_ptp_lock
 *      bcm_esw_ptp_unlock
 *      bcm_esw_ptp_init
 *      bcm_esw_ptp_input_channel_precedence_mode_set
 *      bcm_esw_ptp_input_channel_switching_mode_set
 *      bcm_esw_ptp_input_channels_get
 *      bcm_esw_ptp_input_channels_set
 *      bcm_esw_ptp_modular_enable_get
 *      bcm_esw_ptp_modular_enable_set
 *      bcm_esw_ptp_modular_phyts_get
 *      bcm_esw_ptp_modular_phyts_set
 *      bcm_esw_ptp_modular_portbitmap_get
 *      bcm_esw_ptp_modular_portbitmap_set
 *      bcm_esw_ptp_modular_verbose_get
 *      bcm_esw_ptp_modular_verbose_set
 *      bcm_esw_ptp_packet_counters_get
 *      bcm_petra_ptp_peer_dataset_get
 *      bcm_esw_ptp_primary_domain_get
 *      bcm_esw_ptp_primary_domain_set
 *      bcm_esw_ptp_servo_configuration_get
 *      bcm_esw_ptp_servo_configuration_set
 *      bcm_esw_ptp_servo_status_get
 *      bcm_esw_ptp_signal_output_get
 *      bcm_esw_ptp_signal_output_remove
 *      bcm_esw_ptp_signal_output_set
 *      bcm_esw_ptp_signaled_unicast_master_add
 *      bcm_esw_ptp_signaled_unicast_master_remove
 *      bcm_esw_ptp_signaled_unicast_slave_list
 *      bcm_esw_ptp_signaled_unicast_slave_table_clear
 *      bcm_esw_ptp_stack_create
 *      bcm_esw_ptp_static_unicast_master_add
 *      bcm_esw_ptp_static_unicast_master_list
 *      bcm_esw_ptp_static_unicast_master_table_clear
 *      bcm_esw_ptp_static_unicast_master_table_size_get
 *      bcm_esw_ptp_static_unicast_slave_add
 *      bcm_esw_ptp_static_unicast_slave_list
 *      bcm_esw_ptp_static_unicast_slave_remove
 *      bcm_esw_ptp_static_unicast_slave_table_clear
 *      bcm_esw_ptp_sync_phy
 *      bcm_esw_ptp_telecom_g8265_init
 *      bcm_esw_ptp_telecom_g8265_network_option_get
 *      bcm_esw_ptp_telecom_g8265_network_option_set
 *      bcm_esw_ptp_telecom_g8265_packet_master_add
 *      bcm_esw_ptp_telecom_g8265_packet_master_best_get
 *      bcm_esw_ptp_telecom_g8265_packet_master_get
 *      bcm_esw_ptp_telecom_g8265_packet_master_list
 *      bcm_esw_ptp_telecom_g8265_packet_master_lockout_set
 *      bcm_esw_ptp_telecom_g8265_packet_master_non_reversion_set
 *      bcm_esw_ptp_telecom_g8265_packet_master_priority_override
 *      bcm_esw_ptp_telecom_g8265_packet_master_priority_set
 *      bcm_esw_ptp_telecom_g8265_packet_master_remove
 *      bcm_esw_ptp_telecom_g8265_packet_master_wait_duration_set
 *      bcm_esw_ptp_telecom_g8265_pktstats_thresholds_get
 *      bcm_esw_ptp_telecom_g8265_pktstats_thresholds_set
 *      bcm_esw_ptp_telecom_g8265_quality_level_set
 *      bcm_esw_ptp_telecom_g8265_receipt_timeout_get
 *      bcm_esw_ptp_telecom_g8265_receipt_timeout_set
 *      bcm_esw_ptp_telecom_g8265_shutdown
 *      bcm_esw_ptp_time_format_set
 *      bcm_esw_ptp_timesource_input_status_get
 *      bcm_esw_ptp_tod_input_sources_get
 *      bcm_esw_ptp_tod_input_sources_set
 *      bcm_esw_ptp_tod_output_get
 *      bcm_esw_ptp_tod_output_remove
 *      bcm_esw_ptp_tod_output_set
 *      bcm_esw_ptp_transparent_clock_default_dataset_get
 *      bcm_esw_ptp_transparent_clock_port_dataset_get
 *      bcm_esw_ptp_unicast_request_duration_get
 *      bcm_esw_ptp_unicast_request_duration_max_get
 *      bcm_esw_ptp_unicast_request_duration_max_set
 *      bcm_esw_ptp_unicast_request_duration_min_get
 *      bcm_esw_ptp_unicast_request_duration_min_set
 *      bcm_esw_ptp_unicast_request_duration_set
 */


#if defined(INCLUDE_PTP)

#include <soc/defs.h>
#include <soc/drv.h>

#include <sal/core/dpc.h>

#include <bcm/ptp.h>
#include <bcm_int/esw_dispatch.h>
#include <bcm_int/common/ptp.h>
#include <bcm_int/ptp_common.h>
#include <bcm/error.h>

/*
 * Function:
 *      bcm_esw_ptp_acceptable_master_add
 * Purpose:
 *      Add an entry to the acceptable master table of a PTP clock.
 * Parameters:
 *      unit - Unit number
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      priority1_alt_value - Alternate priority1
 *      *master_info - Acceptable master address
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_acceptable_master_add(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    int priority1_alt_value,
    bcm_ptp_clock_peer_address_t *master_info)
{
    int rc;

    rc = bcm_common_ptp_acceptable_master_add(unit, ptp_id, clock_num, port_num, 
             priority1_alt_value, master_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_acceptable_master_enabled_get
 * Purpose:
 *      Get maximum number of acceptable master table entries of a PTP clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *enabled - Acceptable master table enabled Boolean
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_acceptable_master_enabled_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint8 *enabled)
{
    int rc;

    rc = bcm_common_ptp_acceptable_master_enabled_get(unit, ptp_id, clock_num, port_num, 
             enabled);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_acceptable_master_enabled_set
 * Purpose:
 *      
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      enabled - Acceptable master table enabled Boolean
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_acceptable_master_enabled_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint8 enabled)
{
    int rc;

    rc = bcm_common_ptp_acceptable_master_enabled_set(unit, ptp_id, clock_num, port_num, 
             enabled);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_acceptable_master_list
 * Purpose:
 *      List the acceptable master table of a PTP clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      max_num_masters - Maximum number of acceptable master table entries
 *      *num_masters - Number of acceptable master table entries
 *      *master_addr - Acceptable master table
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_acceptable_master_list(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    int max_num_masters,
    int *num_masters,
    bcm_ptp_clock_peer_address_t *master_addr)
{
    int rc;

    rc = bcm_common_ptp_acceptable_master_list(unit, ptp_id, clock_num, port_num, 
             max_num_masters, num_masters, master_addr);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_acceptable_master_remove
 * Purpose:
 *      Remove an entry from the acceptable master table of a PTP clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *master_info - Acceptable master address
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_acceptable_master_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_peer_address_t *master_info)
{
    int rc;

    rc = bcm_common_ptp_acceptable_master_remove(unit, ptp_id, clock_num, port_num, 
             master_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_acceptable_master_table_clear
 * Purpose:
 *      Clear (i.e. remove all entries from) the acceptable master table of a PTP clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_acceptable_master_table_clear(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num)
{
    int rc;

    rc = bcm_common_ptp_acceptable_master_table_clear(unit, ptp_id, clock_num, port_num);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_acceptable_master_table_size_get
 * Purpose:
 *      Get maximum number of acceptable master table entries of a PTP clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *max_table_entries - Maximum number of acceptable master table entries
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_acceptable_master_table_size_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    int *max_table_entries)
{
    int rc;

    rc = bcm_common_ptp_acceptable_master_table_size_get(unit, ptp_id, clock_num, port_num, 
             max_table_entries);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_cb_register
 * Purpose:
 *      Register a callback for handling PTP events.
 * Parameters:
 *      unit - Unit number 
 *      cb_types - The set of PTP callbacks types for which the specified callback should be called
 *      cb - A pointer to the callback function to call for the specified PTP events
 *      *user_data - Pointer to user data to supply in the callback
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_cb_register(
    int unit,
    bcm_ptp_cb_types_t cb_types,
    bcm_ptp_cb cb,
    void *user_data)
{
    int rc;

    rc = bcm_common_ptp_cb_register(unit, cb_types, cb, user_data);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_cb_unregister
 * Purpose:
 *      Unregister a callback for handling PTP events.
 * Parameters:
 *      unit - Unit number 
 *      cb_types - The set of PTP events to unregister for the specified callback
 *      cb - A pointer to the callback function to unregister from the specified PTP events
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_cb_unregister(
    int unit,
    bcm_ptp_cb_types_t cb_types,
    bcm_ptp_cb cb)
{
    int rc;

    rc = bcm_common_ptp_cb_unregister(unit, cb_types, cb);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_accuracy_get
 * Purpose:
 *      Get clock quality, clock accuracy member of a PTP clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *accuracy - PTP clock accuracy
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_accuracy_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_clock_accuracy_t *accuracy)
{
    int rc;

    rc = bcm_common_ptp_clock_accuracy_get(unit, ptp_id, clock_num, accuracy);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_accuracy_set
 * Purpose:
 *      Set clock quality, clock accuracy member of a PTP clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *accuracy - PTP clock accuracy
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_accuracy_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_clock_accuracy_t *accuracy)
{
    int rc;

    rc = bcm_common_ptp_clock_accuracy_set(unit, ptp_id, clock_num, accuracy);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_create
 * Purpose:
 *      Create a PTP clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      *clock_info - PTP clock information
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_create(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    bcm_ptp_clock_info_t *clock_info)
{
    int rc;

    rc = bcm_common_ptp_clock_create(unit, ptp_id, clock_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_current_dataset_get
 * Purpose:
 *      Get PTP clock current dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *dataset - Current dataset
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_current_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_current_dataset_t *dataset)
{
    int rc;

    rc = bcm_common_ptp_clock_current_dataset_get(unit, ptp_id, clock_num, dataset);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_default_dataset_get
 * Purpose:
 *      Get PTP clock current dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *dataset - Current dataset
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_default_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_default_dataset_t *dataset)
{
    int rc;

    rc = bcm_common_ptp_clock_default_dataset_get(unit, ptp_id, clock_num, dataset);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_domain_get
 * Purpose:
 *      Get PTP domain member of a PTP clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *domain - PTP clock domain
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_domain_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 *domain)
{
    int rc;

    rc = bcm_common_ptp_clock_domain_get(unit, ptp_id, clock_num, domain);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_domain_set
 * Purpose:
 *      Set PTP domain member of a PTP clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      domain - PTP clock domain
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_domain_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 domain)
{
    int rc;

    rc = bcm_common_ptp_clock_domain_set(unit, ptp_id, clock_num, domain);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_get
 * Purpose:
 *      Get configuration information (data and metadata) of a PTP clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *clock_info - PTP clock information
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_clock_info_t *clock_info)
{
    int rc;

    rc = bcm_common_ptp_clock_get(unit, ptp_id, clock_num, clock_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_parent_dataset_get
 * Purpose:
 *      Get PTP clock parent dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *dataset - Current dataset
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_parent_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_parent_dataset_t *dataset)
{
    int rc;

    rc = bcm_common_ptp_clock_parent_dataset_get(unit, ptp_id, clock_num, dataset);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_announce_receipt_timeout_get
 * Purpose:
 *      Get announce receipt timeout member of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *timeout - PTP clock port announce receipt timeout
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_announce_receipt_timeout_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 *timeout)
{
    int rc;

    rc = bcm_common_ptp_clock_port_announce_receipt_timeout_get(unit, ptp_id, clock_num, 
             clock_port, timeout);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_announce_receipt_timeout_set
 * Purpose:
 *      Set announce receipt timeout member of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      timeout - PTP clock port announce receipt timeout
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_announce_receipt_timeout_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 timeout)
{
    int rc;

    rc = bcm_common_ptp_clock_port_announce_receipt_timeout_set(unit, ptp_id, clock_num, 
             clock_port, timeout);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_configure
 * Purpose:
 *      Configure a PTP clock port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *info - PTP clock port configuration information
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_configure(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_clock_port_info_t *info)
{
    int rc;

    rc = bcm_common_ptp_clock_port_configure(unit, ptp_id, clock_num, clock_port, info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_dataset_get
 * Purpose:
 *      Get PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *dataset - Current dataset
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_port_dataset_t *dataset)
{
    int rc;

    rc = bcm_common_ptp_clock_port_dataset_get(unit, ptp_id, clock_num, clock_port, dataset);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_delay_mechanism_get
 * Purpose:
 *      Get delay mechanism member of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *delay_mechanism - PTP clock port delay mechanism
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_delay_mechanism_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 *delay_mechanism)
{
    int rc;

    rc = bcm_common_ptp_clock_port_delay_mechanism_get(unit, ptp_id, clock_num, clock_port, 
             delay_mechanism);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_delay_mechanism_set
 * Purpose:
 *      Set delay mechanism member of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      delay_mechanism - PTP clock port delay mechanism
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_delay_mechanism_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 delay_mechanism)
{
    int rc;

    rc = bcm_common_ptp_clock_port_delay_mechanism_set(unit, ptp_id, clock_num, clock_port, 
             delay_mechanism);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_disable
 * Purpose:
 *      Disable a PTP clock port of an ordinary clock (OC) or a boundary clock (BC).
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_disable(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port)
{
    int rc;

    rc = bcm_common_ptp_clock_port_disable(unit, ptp_id, clock_num, clock_port);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_enable
 * Purpose:
 *      Enable a PTP clock port of an ordinary clock (OC) or a boundary clock (BC).
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_enable(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port)
{
    int rc;

    rc = bcm_common_ptp_clock_port_enable(unit, ptp_id, clock_num, clock_port);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_identity_get
 * Purpose:
 *      Get port identity of a PTP clock port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *identity - PTP port identity
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_identity_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_port_identity_t *identity)
{
    int rc;

    rc = bcm_common_ptp_clock_port_identity_get(unit, ptp_id, clock_num, clock_port, identity);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_info_get
 * Purpose:
 *      Get configuration information (data and metadata) of a PTP clock port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *info - PTP clock port configuration information
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_info_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_clock_port_info_t *info)
{
    int rc;

    rc = bcm_common_ptp_clock_port_info_get(unit, ptp_id, clock_num, clock_port, info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_latency_get
 * Purpose:
 *      Get PTP clock port expected asymmetric latencies.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *latency_in - PTP clock port inbound latency (ns)
 *      *latency_out - PTP clock port outbound latency (ns)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_latency_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 *latency_in,
    uint32 *latency_out)
{
    int rc;

    rc = bcm_common_ptp_clock_port_latency_get(unit, ptp_id, clock_num, clock_port, latency_in, 
             latency_out);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_latency_set
 * Purpose:
 *      Set PTP clock port expected asymmetric latencies.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      latency_in - PTP clock port inbound latency (ns)
 *      latency_out - PTP clock port outbound latency (ns)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_latency_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 latency_in,
    uint32 latency_out)
{
    int rc;

    rc = bcm_common_ptp_clock_port_latency_set(unit, ptp_id, clock_num, clock_port, latency_in, 
             latency_out);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_log_announce_interval_get
 * Purpose:
 *      Get log announce interval of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *interval - PTP clock port log announce interval
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_log_announce_interval_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int *interval)
{
    int rc;

    rc = bcm_common_ptp_clock_port_log_announce_interval_get(unit, ptp_id, clock_num, clock_port, 
             interval);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_log_announce_interval_set
 * Purpose:
 *      Set log announce interval of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      interval - PTP clock port log announce interval
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_log_announce_interval_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int interval)
{
    int rc;

    rc = bcm_common_ptp_clock_port_log_announce_interval_set(unit, ptp_id, clock_num, clock_port, 
             interval);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_log_min_delay_req_interval_get
 * Purpose:
 *      Get log minimum delay request interval member of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *interval - PTP clock port log minimum delay request interval
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_log_min_delay_req_interval_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int *interval)
{
    int rc;

    rc = bcm_common_ptp_clock_port_log_min_delay_req_interval_get(unit, ptp_id, clock_num, 
             clock_port, interval);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_log_min_delay_req_interval_set
 * Purpose:
 *      Set log minimum delay request interval member of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      interval - PTP clock port log minimum delay request interval
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_log_min_delay_req_interval_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int interval)
{
    int rc;

    rc = bcm_common_ptp_clock_port_log_min_delay_req_interval_set(unit, ptp_id, clock_num, 
             clock_port, interval);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_log_min_pdelay_req_interval_get
 * Purpose:
 *      Get log minimum peer delay (PDelay) request interval member of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *interval - PTP clock port log minimum peer delay (PDelay) request interval
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_log_min_pdelay_req_interval_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int *interval)
{
    int rc;

    rc = bcm_common_ptp_clock_port_log_min_pdelay_req_interval_get(unit, ptp_id, clock_num, 
             clock_port, interval);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_log_min_pdelay_req_interval_set
 * Purpose:
 *      Set log minimum peer delay (PDelay) request interval member of a PTP
 *      clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      interval - PTP clock port log minimum peer delay (PDelay) request interval
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_log_min_pdelay_req_interval_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int interval)
{
    int rc;

    rc = bcm_common_ptp_clock_port_log_min_pdelay_req_interval_set(unit, ptp_id, clock_num, 
             clock_port, interval);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_log_sync_interval_get
 * Purpose:
 *      Get log sync interval member of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *interval - PTP clock port log sync interval
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_log_sync_interval_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int *interval)
{
    int rc;

    rc = bcm_common_ptp_clock_port_log_sync_interval_get(unit, ptp_id, clock_num, clock_port, 
             interval);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_log_sync_interval_set
 * Purpose:
 *      Set log sync interval member of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      interval - PTP clock port log sync interval
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_log_sync_interval_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    int interval)
{
    int rc;

    rc = bcm_common_ptp_clock_port_log_sync_interval_set(unit, ptp_id, clock_num, clock_port, 
             interval);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_mac_get
 * Purpose:
 *      Get MAC address of a PTP clock port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *mac - PTP clock port MAC address
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_mac_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_mac_t *mac)
{
    int rc;

    rc = bcm_common_ptp_clock_port_mac_get(unit, ptp_id, clock_num, clock_port, mac);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_protocol_get
 * Purpose:
 *      Get network protocol of a PTP clock port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *protocol - PTP clock port network protocol
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_protocol_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_protocol_t *protocol)
{
    int rc;

    rc = bcm_common_ptp_clock_port_protocol_get(unit, ptp_id, clock_num, clock_port, protocol);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_type_get
 * Purpose:
 *      Get port type of a PTP clock port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *type - PTP clock port type
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_type_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    bcm_ptp_port_type_t *type)
{
    int rc;

    rc = bcm_common_ptp_clock_port_type_get(unit, ptp_id, clock_num, clock_port, type);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_port_version_number_get
 * Purpose:
 *      Get PTP version number member of a PTP clock port dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *version - PTP clock port PTP version number
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_port_version_number_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 clock_port,
    uint32 *version)
{
    int rc;

    rc = bcm_common_ptp_clock_port_version_number_get(unit, ptp_id, clock_num, clock_port, 
             version);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_priority1_get
 * Purpose:
 *      Get priority1 member of a PTP clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *priority1 - PTP clock priority1.
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_priority1_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 *priority1)
{
    int rc;

    rc = bcm_common_ptp_clock_priority1_get(unit, ptp_id, clock_num, priority1);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_priority1_set
 * Purpose:
 *      Set priority1 member of a PTP clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      priority1 - PTP clock priority1
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_priority1_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 priority1)
{
    int rc;

    rc = bcm_common_ptp_clock_priority1_set(unit, ptp_id, clock_num, priority1);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_priority2_get
 * Purpose:
 *      Get priority2 member of a PTP clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *priority2 - PTP clock priority2
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_priority2_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 *priority2)
{
    int rc;

    rc = bcm_common_ptp_clock_priority2_get(unit, ptp_id, clock_num, priority2);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_priority2_set
 * Purpose:
 *      Set priority2 member of a PTP clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      priority2 - PTP clock priority2
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_priority2_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 priority2)
{
    int rc;

    rc = bcm_common_ptp_clock_priority2_set(unit, ptp_id, clock_num, priority2);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_slaveonly_get
 * Purpose:
 *      Get slave-only (SO) flag of a PTP clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *slaveonly - PTP clock slave-only (SO) flag
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_slaveonly_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 *slaveonly)
{
    int rc;

    rc = bcm_common_ptp_clock_slaveonly_get(unit, ptp_id, clock_num, slaveonly);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_slaveonly_set
 * Purpose:
 *      Set slave-only (SO) flag of a PTP clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      slaveonly - PTP clock slave-only (SO) flag.
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_slaveonly_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint32 slaveonly)
{
    int rc;

    rc = bcm_common_ptp_clock_slaveonly_set(unit, ptp_id, clock_num, slaveonly);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_time_get
 * Purpose:
 *      Get local time of a PTP node.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *time - PTP timestamp
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_time_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_timestamp_t *time)
{
    int rc;

    rc = bcm_common_ptp_clock_time_get(unit, ptp_id, clock_num, time);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_time_properties_get
 * Purpose:
 *      Get PTP clock time properties dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *data - Time properties dataset
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_time_properties_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_time_properties_t *data)
{
    int rc;

    rc = bcm_common_ptp_clock_time_properties_get(unit, ptp_id, clock_num, data);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_time_set
 * Purpose:
 *      Set local time of a PTP node.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *time - PTP timestamp
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_time_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_timestamp_t *time)
{
    int rc;

    rc = bcm_common_ptp_clock_time_set(unit, ptp_id, clock_num, time);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_timescale_get
 * Purpose:
 *      Get timescale members of a PTP clock time properties dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *timescale - PTP clock timescale properties
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_timescale_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_timescale_t *timescale)
{
    int rc;

    rc = bcm_common_ptp_clock_timescale_get(unit, ptp_id, clock_num, timescale);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_timescale_set
 * Purpose:
 *      Set timescale members of a PTP clock time properties dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *timescale - PTP clock timescale properties
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_timescale_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_timescale_t *timescale)
{
    int rc;

    rc = bcm_common_ptp_clock_timescale_set(unit, ptp_id, clock_num, timescale);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_traceability_get
 * Purpose:
 *      Get traceability members of a PTP clock time properties dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *trace - PTP clock traceability properties
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_traceability_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_trace_t *trace)
{
    int rc;

    rc = bcm_common_ptp_clock_traceability_get(unit, ptp_id, clock_num, trace);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_traceability_set
 * Purpose:
 *      Set traceability members of a PTP clock time properties dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *trace - PTP clock traceability properties.
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_traceability_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_trace_t *trace)
{
    int rc;

    rc = bcm_common_ptp_clock_traceability_set(unit, ptp_id, clock_num, trace);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_user_description_set
 * Purpose:
 *      Set user description member of the default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *desc - User description
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_user_description_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 *desc)
{
    int rc;

    rc = bcm_common_ptp_clock_user_description_set(unit, ptp_id, clock_num, desc);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_utc_get
 * Purpose:
 *      Get UTC members of a PTP clock time properties dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *utc - PTP clock coordinated universal time (UTC) properties
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_utc_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_utc_t *utc)
{
    int rc;

    rc = bcm_common_ptp_clock_utc_get(unit, ptp_id, clock_num, utc);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_clock_utc_set
 * Purpose:
 *      Set UTC members of a PTP clock time properties dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *utc - PTP clock coordinated universal time (UTC) properties
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_clock_utc_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_utc_t *utc)
{
    int rc;

    rc = bcm_common_ptp_clock_utc_set(unit, ptp_id, clock_num, utc);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_ctdev_alarm_callback_register
 * Purpose:
 *      Register a C-TDEV alarm callback function.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      alarm_cb - pointer to C-TDEV alarm callback function
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_ctdev_alarm_callback_register(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_ctdev_alarm_cb alarm_cb)
{
    int rc;

    rc = bcm_common_ptp_ctdev_alarm_callback_register(unit, ptp_id, clock_num, alarm_cb);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_ctdev_alarm_callback_unregister
 * Purpose:
 *      Unregister a C-TDEV alarm callback function.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_ctdev_alarm_callback_unregister(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num)
{
    int rc;

    rc = bcm_common_ptp_ctdev_alarm_callback_unregister(unit, ptp_id, clock_num);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_ctdev_alpha_get
 * Purpose:
 *      Get C-TDEV recursive algorithm forgetting factor (alpha).
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *alpha_numerator - Forgetting factor numerator
 *      *alpha_denominator - Forgetting factor denominator
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_ctdev_alpha_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint16 *alpha_numerator,
    uint16 *alpha_denominator)
{
    int rc;

    rc = bcm_common_ptp_ctdev_alpha_get(unit, ptp_id, clock_num, alpha_numerator, 
             alpha_denominator);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_ctdev_alpha_set
 * Purpose:
 *      Set C-TDEV recursive algorithm forgetting factor (alpha).
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      alpha_numerator - Forgetting factor numerator
 *      alpha_denominator - Forgetting factor denominator
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_ctdev_alpha_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint16 alpha_numerator,
    uint16 alpha_denominator)
{
    int rc;

    rc = bcm_common_ptp_ctdev_alpha_set(unit, ptp_id, clock_num, alpha_numerator, 
             alpha_denominator);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_ctdev_enable_get
 * Purpose:
 *      Get enable/disable state of C-TDEV processing.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *enable - Enable Boolean
 *      *flags - C-TDEV control flags (UNUSED)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_ctdev_enable_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *enable,
    uint32 *flags)
{
    int rc;

    rc = bcm_common_ptp_ctdev_enable_get(unit, ptp_id, clock_num, enable, flags);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_ctdev_enable_set
 * Purpose:
 *      Set enable/disable state of C-TDEV processing.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      enable - Enable Boolean
 *      flags - C-TDEV control flags (UNUSED)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_ctdev_enable_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int enable,
    uint32 flags)
{
    int rc;

    rc = bcm_common_ptp_ctdev_enable_set(unit, ptp_id, clock_num, enable, flags);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_ctdev_verbose_get
 * Purpose:
 *      Get verbose program control option of C-TDEV processing.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *verbose - Verbose Boolean
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_ctdev_verbose_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *verbose)
{
    int rc;

    rc = bcm_common_ptp_ctdev_verbose_get(unit, ptp_id, clock_num, verbose);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_ctdev_verbose_set
 * Purpose:
 *      Set verbose program control option of C-TDEV processing.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      verbose - Verbose Boolean
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_ctdev_verbose_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int verbose)
{
    int rc;

    rc = bcm_common_ptp_ctdev_verbose_set(unit, ptp_id, clock_num, verbose);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_detach
 * Purpose:
 *      Shut down the PTP subsystem
 * Parameters:
 *      unit - Unit number 
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_detach(
    int unit)
{
    int rc;

    rc = bcm_common_ptp_detach(unit);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_foreign_master_dataset_get
 * Purpose:
 *      Get the foreign master dataset of a PTP clock port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *data_set - Foreign master dataset
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_foreign_master_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_foreign_master_dataset_t *data_set)
{
    int rc;

    rc = bcm_common_ptp_foreign_master_dataset_get(unit, ptp_id, clock_num, port_num, data_set);

    return (rc);
}


/*
 * Function:
 *      bcm_esw_ptp_lock
 * Purpose:
 *      Locks the unit mutex
 * Parameters:
 *      unit - Unit number
 * Returns:
 *      int
 * Notes:
 */
int bcm_esw_ptp_lock(
   int unit)
{
    int rc;

    rc = bcm_common_ptp_lock(unit);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_unlock
 * Purpose:
 *      UnLocks the unit mutex
 * Parameters:
 *      unit - Unit number
 * Returns:
 *      int
 * Notes:
 */
int bcm_esw_ptp_unlock(
   int unit)
{
    int rc;

    rc = bcm_common_ptp_unlock(unit);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_init
 * Purpose:
 *      Initialize the PTP subsystem
 * Parameters:
 *      unit - Unit number 
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_init(
    int unit)
{
    int rc;

    rc = bcm_common_ptp_init(unit);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_input_channel_precedence_mode_set
 * Purpose:
 *      Set PTP input channels precedence mode.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      channel_select_mode - Input channel precedence mode selector
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_input_channel_precedence_mode_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int channel_select_mode)
{
    int rc;

    rc = bcm_common_ptp_input_channel_precedence_mode_set(unit, ptp_id, clock_num, 
             channel_select_mode);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_input_channel_switching_mode_set
 * Purpose:
 *      Set PTP input channels switching mode.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      channel_switching_mode - Channel switching mode selector
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_input_channel_switching_mode_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int channel_switching_mode)
{
    int rc;

    rc = bcm_common_ptp_input_channel_switching_mode_set(unit, ptp_id, clock_num, 
             channel_switching_mode);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_input_channels_get
 * Purpose:
 *      Set PTP input channels.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *num_channels - (IN) Max number of channels (time sources)
 *                      (OUT) Number of returned channels (time sources).
 *      *channels - Channels (time sources)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_input_channels_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *num_channels,
    bcm_ptp_channel_t *channels)
{
    int rc;

    rc = bcm_common_ptp_input_channels_get(unit, ptp_id, clock_num, num_channels, channels);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_input_channels_set
 * Purpose:
 *      Set PTP input channels.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      num_channels - (IN)  Max number of channels (time sources)
 *                     (OUT) Number of returned channels (time sources).
 *      *channels - Channels (time sources)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_input_channels_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int num_channels,
    bcm_ptp_channel_t *channels)
{
    int rc;

    rc = bcm_common_ptp_input_channels_set(unit, ptp_id, clock_num, num_channels, channels);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_synce_output_get
 * Purpose:
 *      Gets SyncE output control state.
 * Parameters:
 *      unit - Unit number.
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      state - Non-zero to indicate that SyncE output should be controlled by TOP
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_synce_output_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *state)
{
  int rc;

  rc = bcm_common_ptp_synce_output_get(unit, ptp_id, clock_num, state);

  return (rc);
}
/*
 * Function:
 *      bcm_esw_ptp_synce_output_set
 * Purpose:
 *      Set SyncE output control on/off.
 * Parameters:
 *      unit  - Unit number.
 *      ptp_id  - PTP stack ID
 *      clock_num - PTP clock number
 *      synce_port - PTP SyncE clock port
 *      state - Non-zero to indicate that SyncE output should be controlled by TOP
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_synce_output_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int synce_port,
    int state)
{
  int rc;

  rc = bcm_common_ptp_synce_output_set(unit, ptp_id, clock_num, synce_port, state);

  return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_modular_enable_get
 * Purpose:
 *      Get enable/disable state of modular system functionality.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *enable - Enable Boolean
 *      *flags - Modular system control flags (UNUSED)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_modular_enable_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *enable,
    uint32 *flags)
{
    int rc;

    rc = bcm_common_ptp_modular_enable_get(unit, ptp_id, clock_num, enable, flags);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_modular_enable_set
 * Purpose:
 *      Set enable/disable state of modular system functionality.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      enable - Enable Boolean
 *      flags - Modular system control flags (UNUSED)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_modular_enable_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int enable,
    uint32 flags)
{
    int rc;

    rc = bcm_common_ptp_modular_enable_set(unit, ptp_id, clock_num, enable, flags);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_modular_phyts_get
 * Purpose:
 *      Get PHY timestamping configuration state and data.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *phyts - PHY timestamping Boolean
 *      *framesync_pin - Framesync GPIO pin
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_modular_phyts_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *phyts,
    int *framesync_pin)
{
    int rc;

    rc = bcm_common_ptp_modular_phyts_get(unit, ptp_id, clock_num, phyts, framesync_pin);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_modular_phyts_set
 * Purpose:
 *      Set PHY timestamping configuration state and data.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      phyts - PHY timestamping Boolean
 *      framesync_pin - Framesync GPIO pin
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_modular_phyts_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int phyts,
    int framesync_pin)
{
    int rc;

    rc = bcm_common_ptp_modular_phyts_set(unit, ptp_id, clock_num, phyts, framesync_pin);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_modular_portbitmap_get
 * Purpose:
 *      Get PHY port bitmap.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *pbmp - Port bitmap
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_modular_portbitmap_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_pbmp_t *pbmp)
{
    int rc;

    rc = bcm_common_ptp_modular_portbitmap_get(unit, ptp_id, clock_num, pbmp);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_modular_portbitmap_set
 * Purpose:
 *      Set PHY port bitmap.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      pbmp - Port bitmap
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_modular_portbitmap_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_pbmp_t pbmp)
{
    int rc;

    rc = bcm_common_ptp_modular_portbitmap_set(unit, ptp_id, clock_num, pbmp);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_modular_verbose_get
 * Purpose:
 *      Get verbose program control option of modular system functionality.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *verbose - Verbose Boolean
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_modular_verbose_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *verbose)
{
    int rc;

    rc = bcm_common_ptp_modular_verbose_get(unit, ptp_id, clock_num, verbose);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_modular_verbose_set
 * Purpose:
 *      Set verbose program control option of modular system functionality.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      verbose - Verbose Boolean
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_modular_verbose_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int verbose)
{
    int rc;

    rc = bcm_common_ptp_modular_verbose_set(unit, ptp_id, clock_num, verbose);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_packet_counters_get
 * Purpose:
 *      Get packet counters.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *counters - Packet counts/statistics
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_packet_counters_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_packet_counters_t *counters)
{
    int rc;

    rc = bcm_common_ptp_packet_counters_get(unit, ptp_id, clock_num, counters);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_peer_dataset_get
 * Purpose:
 *      Get Peer Dataset
 * Parameters:
 *      unit - Unit number
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP port number
 *      max_num_peers -
 *      peers - list of peer entries
 *      *num_peers - # of peer entries returned in peers
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_peer_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    int max_num_peers,
    bcm_ptp_peer_entry_t *peers,
    int *num_peers)
{
    int rc;

    rc = bcm_common_ptp_peer_dataset_get(unit, ptp_id, clock_num, port_num, max_num_peers, peers, num_peers);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_primary_domain_get
 * Purpose:
 *      Get primary domain member of a PTP transparent clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *primary_domain - PTP transparent clock primary domain
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_primary_domain_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *primary_domain)
{
    int rc;

    rc = bcm_common_ptp_primary_domain_get(unit, ptp_id, clock_num, primary_domain);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_primary_domain_set
 * Purpose:
 *      Set primary domain member of a PTP transparent clock default dataset.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      primary_domain - PTP transparent clock primary domain.
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_primary_domain_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int primary_domain)
{
    int rc;

    rc = bcm_common_ptp_primary_domain_set(unit, ptp_id, clock_num, primary_domain);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_servo_configuration_get
 * Purpose:
 *      Get PTP servo configuration properties.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *config - PTP servo configuration properties
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_servo_configuration_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_servo_config_t *config)
{
    int rc;

    rc = bcm_common_ptp_servo_configuration_get(unit, ptp_id, clock_num, config);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_servo_configuration_set
 * Purpose:
 *      Set PTP servo configuration properties.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *config - PTP servo configuration properties
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_servo_configuration_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_servo_config_t *config)
{
    int rc;

    rc = bcm_common_ptp_servo_configuration_set(unit, ptp_id, clock_num, config);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_servo_status_get
 * Purpose:
 *      Get PTP servo state and status information.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *status - PTP servo state and status information
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_servo_status_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_servo_status_t *status)
{
    int rc;

    rc = bcm_common_ptp_servo_status_get(unit, ptp_id, clock_num, status);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_signal_output_get
 * Purpose:
 *      Get PTP signal output.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *signal_output_count - (IN)  Max number of signal outputs
 *                             (OUT) Number signal outputs returned
 *      *signal_output_id - Array of signal outputs
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_signal_output_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *signal_output_count,
    bcm_ptp_signal_output_t *signal_output_id)
{
    int rc;

    rc = bcm_common_ptp_signal_output_get(unit, ptp_id, clock_num, signal_output_count, 
             signal_output_id);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_signal_output_remove
 * Purpose:
 *      Remove PTP signal output.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      signal_output_id - Signal to remove/invalidate
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_signal_output_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int signal_output_id)
{
    int rc;

    rc = bcm_common_ptp_signal_output_remove(unit, ptp_id, clock_num, signal_output_id);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_signal_output_set
 * Purpose:
 *      Set PTP signal output.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *signal_output_id - ID of signal
 *      *output_info - Signal information
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_signal_output_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *signal_output_id,
    bcm_ptp_signal_output_t *output_info)
{
    int rc;

    rc = bcm_common_ptp_signal_output_set(unit, ptp_id, clock_num, signal_output_id, 
             output_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_signaled_unicast_master_add
 * Purpose:
 *      Add an entry to the signaled unicast master table.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *master_info - Acceptable master address
 *      mask - Signaled unicast master configuration flags
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_signaled_unicast_master_add(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_unicast_master_t *master_info,
    uint32 mask)
{
    int rc;

    rc = bcm_common_ptp_signaled_unicast_master_add(unit, ptp_id, clock_num, port_num, 
             master_info, mask);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_signaled_unicast_master_remove
 * Purpose:
 *      Remove an entry from the signaled unicast master table.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *master_info - Acceptable master address
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_signaled_unicast_master_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_peer_address_t *master_info)
{
    int rc;

    rc = bcm_common_ptp_signaled_unicast_master_remove(unit, ptp_id, clock_num, port_num, 
             master_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_signaled_unicast_slave_list
 * Purpose:
 *      List the signaled unicast slaves.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      max_num_slaves - Maximum number of signaled unicast slaves
 *      *num_slaves - Number of signaled unicast slaves
 *      *slave_info - Signaled unicast slave table
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_signaled_unicast_slave_list(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    int max_num_slaves,
    int *num_slaves,
    bcm_ptp_clock_peer_t *slave_info)
{
    int rc;

    rc = bcm_common_ptp_signaled_unicast_slave_list(unit, ptp_id, clock_num, port_num, 
             max_num_slaves, num_slaves, slave_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_signaled_unicast_slave_table_clear
 * Purpose:
 *      Cancel active unicast services for slaves and clear signaled unicast
 *      slave table entries that correspond to caller-specified master port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      callstack - Boolean to unsubscribe/unregister unicast services.
 *          TRUE : Cancellation message to slave and PTP stack.
 *          FALSE: Cancellation message tunneled to slave only.
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_signaled_unicast_slave_table_clear(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    int callstack)
{
    int rc;

    rc = bcm_common_ptp_signaled_unicast_slave_table_clear(unit, ptp_id, clock_num, port_num, 
             callstack);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_stack_create
 * Purpose:
 *      Create a PTP stack instance
 * Parameters:
 *      unit - Unit number 
 *      *ptp_info - Pointer to an PTP Stack Info structure
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_stack_create(
    int unit,
    bcm_ptp_stack_info_t *ptp_info)
{
    int rc;

    rc = bcm_common_ptp_stack_create(unit, ptp_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_static_unicast_master_add
 * Purpose:
 *      Add an entry to the unicast master table of a PTP clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *master_info - Acceptable master address
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_static_unicast_master_add(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_unicast_master_t *master_info)
{
    int rc;

    rc = bcm_common_ptp_static_unicast_master_add(unit, ptp_id, clock_num, port_num, 
             master_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_static_unicast_master_list
 * Purpose:
 *      List the unicast master table of a PTP clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      max_num_masters - Maximum number of unicast master table entries
 *      *num_masters - Number of unicast master table entries
 *      *master_addr - Unicast master table
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_static_unicast_master_list(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    int max_num_masters,
    int *num_masters,
    bcm_ptp_clock_peer_address_t *master_addr)
{
    int rc;

    rc = bcm_common_ptp_static_unicast_master_list(unit, ptp_id, clock_num, port_num, 
             max_num_masters, num_masters, master_addr);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_static_unicast_master_remove
 * Purpose:
 *      Remove an entry from the static unicast master table.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *master_addr - Unicast master table
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_static_unicast_master_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_peer_address_t *master_addr)
{
    int rc;

    rc = bcm_common_ptp_static_unicast_master_remove(unit, ptp_id, clock_num, port_num, 
             master_addr);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_static_unicast_master_table_clear
 * Purpose:
 *      Clear (i.e. remove all entries from) the unicast master table 
 *      of a PTP clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_static_unicast_master_table_clear(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num)
{
    int rc;

    rc = bcm_common_ptp_static_unicast_master_table_clear(unit, ptp_id, clock_num, port_num);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_static_unicast_master_table_size_get
 * Purpose:
 *      Get maximum number of unicast master table entries of a PTP clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *max_table_entries - Maximum number of unicast master table entries
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_static_unicast_master_table_size_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    int *max_table_entries)
{
    int rc;

    rc = bcm_common_ptp_static_unicast_master_table_size_get(unit, ptp_id, clock_num, port_num, 
             max_table_entries);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_static_unicast_slave_add
 * Purpose:
 *      Add a unicast slave to a PTP clock port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *slave_info - Unicast slave information.
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_static_unicast_slave_add(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_peer_t *slave_info)
{
    int rc;

    rc = bcm_common_ptp_static_unicast_slave_add(unit, ptp_id, clock_num, port_num, slave_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_static_unicast_slave_list
 * Purpose:
 *      List the unicast slaves of a PTP clock port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      max_num_slaves - Maximum number of unicast slave table entries
 *      *num_slaves - Number of unicast slave table entries
 *      *slave_info - Unicast slave table
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_static_unicast_slave_list(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    int max_num_slaves,
    int *num_slaves,
    bcm_ptp_clock_peer_t *slave_info)
{
    int rc;

    rc = bcm_common_ptp_static_unicast_slave_list(unit, ptp_id, clock_num, port_num, 
             max_num_slaves, num_slaves, slave_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_static_unicast_slave_remove
 * Purpose:
 *      Remove a unicast slave of a PTP clock port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *slave_info - Unicast slave information
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_static_unicast_slave_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_peer_t *slave_info)
{
    int rc;

    rc = bcm_common_ptp_static_unicast_slave_remove(unit, ptp_id, clock_num, port_num, 
             slave_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_static_unicast_slave_table_clear
 * Purpose:
 *      Clear (i.e. remove all) unicast slaves of a PTP clock port.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_static_unicast_slave_table_clear(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num)
{
    int rc;

    rc = bcm_common_ptp_static_unicast_slave_table_clear(unit, ptp_id, clock_num, port_num);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_sync_phy
 * Purpose:
 *      Instruct TOP to send a PHY Sync pulse, and mark the time that the PHYs are now synced to.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      sync_input - PTP PHY SYNC inputs
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_sync_phy(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_sync_phy_input_t sync_input)
{
    int rc;

    rc = bcm_common_ptp_sync_phy(unit, ptp_id, clock_num, sync_input);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_init
 * Purpose:
 *      Initialize the telecom profile.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_init(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_init(unit, ptp_id, clock_num);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_network_option_get
 * Purpose:
 *      Get telecom profile ITU-T G.781 network option.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *network_option - Telecom profile ITU-T G.781 network option
 *          | Disable | Option I | Option II | Option III |
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_network_option_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_network_option_t *network_option)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_network_option_get(unit, ptp_id, clock_num, network_option);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_network_option_set
 * Purpose:
 *      Set telecom profile ITU-T G.781 network option.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      network_option - Telecom profile ITU-T G.781 network option.
 *          | Disable | Option I | Option II | Option III |
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_network_option_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_network_option_t network_option)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_network_option_set(unit, ptp_id, clock_num, network_option);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_packet_master_add
 * Purpose:
 *      Add a packet master.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *address - Packet master address
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_packet_master_add(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_port_address_t *address)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_packet_master_add(unit, ptp_id, clock_num, port_num, 
             address);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_packet_master_best_get
 * Purpose:
 *      Get current best packet master clock as defined by telecom profile
 *      master selection logic.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *pktmaster - Best packet master clock
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_packet_master_best_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_packet_master_best_get(unit, ptp_id, clock_num, pktmaster);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_packet_master_get
 * Purpose:
 *      Get packet master by address.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *address - Packet master address
 *      *pktmaster - Packet master
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_packet_master_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_clock_port_address_t *address,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_packet_master_get(unit, ptp_id, clock_num, address, 
             pktmaster);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_packet_master_list
 * Purpose:
 *      Get list of in-use packet masters.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      max_master_count - Maximum number of packet masters
 *      *num_masters - Number of in-use packet masters
 *      *best_master - Best packet master list index
 *      *pktmaster - Packet masters
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_packet_master_list(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int max_master_count,
    int *num_masters,
    int *best_master,
    bcm_ptp_telecom_g8265_pktmaster_t *pktmaster)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_packet_master_list(unit, ptp_id, clock_num, 
             max_master_count, num_masters, best_master, pktmaster);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_packet_master_lockout_set
 * Purpose:
 *      Set packet master lockout to exclude from master selection process.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      lockout - Packet master lockout Boolean
 *      *address - Packet master address
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_packet_master_lockout_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 lockout,
    bcm_ptp_clock_port_address_t *address)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_packet_master_lockout_set(unit, ptp_id, clock_num, lockout, 
             address);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_packet_master_non_reversion_set
 * Purpose:
 *      Set packet master non-reversion to control master selection process
 *      if master fails or is unavailable.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      nonres - Packet master non-reversion Boolean
 *      *address - Packet master address
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_packet_master_non_reversion_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 nonres,
    bcm_ptp_clock_port_address_t *address)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_packet_master_non_reversion_set(unit, ptp_id, clock_num, 
             nonres, address);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_packet_master_priority_override
 * Purpose:
 *      Set priority override of packet master.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      override - Packet master priority override Boolean.
 *      *address - Packet master address
 * Returns:
 *      int
 * Notes: 
 *      Priority override controls whether a packet master's priority value
 *      is set automatically using grandmaster priority1 / priority2 in PTP
 *      announce message (i.e., override equals TRUE) or via host API calls
 *      (i.e., override equals FALSE).
 */
int
bcm_esw_ptp_telecom_g8265_packet_master_priority_override(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint8 override,
    bcm_ptp_clock_port_address_t *address)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_packet_master_priority_override(unit, ptp_id, clock_num, 
             override, address);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_packet_master_priority_set
 * Purpose:
 *      Set priority of packet master.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      priority - Packet master priority
 *      *address - Packet master address
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_packet_master_priority_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint16 priority,
    bcm_ptp_clock_port_address_t *address)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_packet_master_priority_set(unit, ptp_id, clock_num, 
             priority, address);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_packet_master_remove
 * Purpose:
 *      Remove a packet master.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *address - Packet master address
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_packet_master_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    bcm_ptp_clock_port_address_t *address)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_packet_master_remove(unit, ptp_id, clock_num, port_num, 
             address);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_packet_master_wait_duration_set
 * Purpose:
 *      Set packet master wait-to-restore duration to control master selection
 *      process if master fails or is unavailable.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      wait_sec - Packet master wait-to-restore duration
 *      *address - Packet master address
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_packet_master_wait_duration_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint64 wait_sec,
    bcm_ptp_clock_port_address_t *address)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_packet_master_wait_duration_set(unit, ptp_id, clock_num, 
             wait_sec, address);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_pktstats_thresholds_get
 * Purpose:
 *      Get packet timing and PDV statistics thresholds req'd for packet timing
 *      signal fail (PTSF) analysis.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *thresholds - Packet timing and PDV statistics thresholds
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_pktstats_thresholds_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_pktstats_t *thresholds)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_pktstats_thresholds_get(unit, ptp_id, clock_num, 
             thresholds);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_pktstats_thresholds_set
 * Purpose:
 *      Set packet timing and PDV statistics thresholds req'd for packet timing
 *      signal fail (PTSF) analysis.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      thresholds - Packet timing and PDV statistics thresholds
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_pktstats_thresholds_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_pktstats_t thresholds)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_pktstats_thresholds_set(unit, ptp_id, clock_num, 
             thresholds);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_quality_level_set
 * Purpose:
 *      Set the ITU-T G.781 quality level (QL) of local clock.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      ql - Quality level
 * Returns:
 *      int
 * Notes: 
 *      Ref. ITU-T G.781 and ITU-T G.8265.1.
 *      Quality level is mapped to PTP clockClass.
 *      Quality level is used to infer ITU-T G.781 synchronization network
 *      option. ITU-T G.781 synchronization network option must be uniform
 *      among slave and its packet masters.
 */
int
bcm_esw_ptp_telecom_g8265_quality_level_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_telecom_g8265_quality_level_t ql)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_quality_level_set(unit, ptp_id, clock_num, ql);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_receipt_timeout_get
 * Purpose:
 *      Get PTP |Announce|Sync|Delay_Resp| message receipt timeout req'd for
 *      packet timing signal fail (PTSF) analysis.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      message_type - PTP message type
 *      *receipt_timeout - PTP |Announce|Sync|Delay_Resp| message receipt timeout
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_receipt_timeout_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_message_type_t message_type,
    uint32 *receipt_timeout)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_receipt_timeout_get(unit, ptp_id, clock_num, message_type, 
             receipt_timeout);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_receipt_timeout_set
 * Purpose:
 *      Set PTP |Announce|Sync|Delay_Resp| message receipt timeout req'd for
 *      packet timing signal fail (PTSF) analysis.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      message_type - PTP message type
 *      receipt_timeout - PTP |Announce|Sync|Delay_Resp| message receipt timeout
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_receipt_timeout_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_message_type_t message_type,
    uint32 receipt_timeout)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_receipt_timeout_set(unit, ptp_id, clock_num, message_type, 
             receipt_timeout);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_telecom_g8265_shutdown
 * Purpose:
 *      Shut down the telecom profile.
 * Parameters:
 *      unit - Unit number 
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_telecom_g8265_shutdown(
    int unit)
{
    int rc;

    rc = bcm_common_ptp_telecom_g8265_shutdown(unit);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_time_format_set
 * Purpose:
 *      Set Time Of Day format for PTP stack instance.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      type - Time of Day format
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_time_format_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    bcm_ptp_time_type_t type)
{
    int rc;

    rc = bcm_common_ptp_time_format_set(unit, ptp_id, type);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_timesource_input_status_get
 * Purpose:
 *      Get time source status.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *status - Time source status
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_timesource_input_status_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_timesource_status_t *status)
{
    int rc;

    rc = bcm_common_ptp_timesource_input_status_get(unit, ptp_id, clock_num, status);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_tod_input_sources_get
 * Purpose:
 *      Get PTP ToD input source(s).
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *num_tod_sources - (IN) Maximum Number of ToD inputs
 *                         (OUT) Number of ToD inputs
 *      *tod_sources - Array of ToD inputs
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_tod_input_sources_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *num_tod_sources,
    bcm_ptp_tod_input_t *tod_sources)
{
    int rc;

    rc = bcm_common_ptp_tod_input_sources_get(unit, ptp_id, clock_num, num_tod_sources, 
             tod_sources);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_tod_input_sources_set
 * Purpose:
 *      Set PTP TOD input(s).
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      num_tod_sources - Number of ToD inputs
 *      *tod_sources - PTP ToD input configuration
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_tod_input_sources_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int num_tod_sources,
    bcm_ptp_tod_input_t *tod_sources)
{
    int rc;

    rc = bcm_common_ptp_tod_input_sources_set(unit, ptp_id, clock_num, num_tod_sources, 
             tod_sources);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_tod_output_get
 * Purpose:
 *      Get PTP TOD output(s).
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *tod_output_count - (IN)  Max number of ToD outputs
 *                          (OUT) Number ToD outputs returned
 *      *tod_output - Array of ToD outputs
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_tod_output_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *tod_output_count,
    bcm_ptp_tod_output_t *tod_output)
{
    int rc;

    rc = bcm_common_ptp_tod_output_get(unit, ptp_id, clock_num, tod_output_count, tod_output);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_tod_output_remove
 * Purpose:
 *      Remove a ToD output.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      tod_output_id - ToD output ID
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_tod_output_remove(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int tod_output_id)
{
    int rc;

    rc = bcm_common_ptp_tod_output_remove(unit, ptp_id, clock_num, tod_output_id);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_tod_output_set
 * Purpose:
 *      Set PTP ToD output.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *tod_output_id - ToD output ID
 *      *output_info - ToD output configuration
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_tod_output_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int *tod_output_id,
    bcm_ptp_tod_output_t *output_info)
{
    int rc;

    rc = bcm_common_ptp_tod_output_set(unit, ptp_id, clock_num, tod_output_id, output_info);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_transparent_clock_default_dataset_get
 * Purpose:
 *      Get PTP Transparent clock default dataset
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      *data_set - PTP transparent clock default dataset
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_transparent_clock_default_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    bcm_ptp_transparent_clock_default_dataset_t *data_set)
{
    int rc;

    rc = bcm_common_ptp_transparent_clock_default_dataset_get(unit, ptp_id, clock_num, data_set);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_transparent_clock_port_dataset_get
 * Purpose:
 *      Get PTP transparent clock port dataset
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      clock_port - PTP clock port number
 *      *data_set - PTP transparent clock port dataset
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_transparent_clock_port_dataset_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    uint16 clock_port,
    bcm_ptp_transparent_clock_port_dataset_t *data_set)
{
    int rc;

    rc = bcm_common_ptp_transparent_clock_port_dataset_get(unit, ptp_id, clock_num, clock_port, 
             data_set);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_unicast_request_duration_get
 * Purpose:
 *      Get durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *duration - durationField (sec)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_unicast_request_duration_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 *duration)
{
    int rc;

    rc = bcm_common_ptp_unicast_request_duration_get(unit, ptp_id, clock_num, port_num, 
             duration);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_unicast_request_duration_max_get
 * Purpose:
 *      Get maximum durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *duration_max - maximum durationField (sec)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_unicast_request_duration_max_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 *duration_max)
{
    int rc;

    rc = bcm_common_ptp_unicast_request_duration_max_get(unit, ptp_id, clock_num, port_num, 
             duration_max);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_unicast_request_duration_max_set
 * Purpose:
 *      Set maximum durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      duration_max - maximum durationField (sec)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_unicast_request_duration_max_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 duration_max)
{
    int rc;

    rc = bcm_common_ptp_unicast_request_duration_max_set(unit, ptp_id, clock_num, port_num, 
             duration_max);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_unicast_request_duration_min_get
 * Purpose:
 *      Set minimum durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      *duration_min - minimum durationField (sec)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_unicast_request_duration_min_get(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 *duration_min)
{
    int rc;

    rc = bcm_common_ptp_unicast_request_duration_min_get(unit, ptp_id, clock_num, port_num, 
             duration_min);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_unicast_request_duration_min_set
 * Purpose:
 *      Set minimum durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      duration_min - minimum durationField (sec)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_unicast_request_duration_min_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 duration_min)
{
    int rc;

    rc = bcm_common_ptp_unicast_request_duration_min_set(unit, ptp_id, clock_num, port_num, 
             duration_min);

    return (rc);
}

/*
 * Function:
 *      bcm_esw_ptp_unicast_request_duration_set
 * Purpose:
 *      Set durationField of REQUEST_UNICAST_TRANSMISSION TLV.
 * Parameters:
 *      unit - Unit number 
 *      ptp_id - PTP stack ID
 *      clock_num - PTP clock number
 *      port_num - PTP clock port number
 *      duration - durationField (sec)
 * Returns:
 *      int
 * Notes:
 */
int
bcm_esw_ptp_unicast_request_duration_set(
    int unit,
    bcm_ptp_stack_id_t ptp_id,
    int clock_num,
    int port_num,
    uint32 duration)
{
    int rc;

    rc = bcm_common_ptp_unicast_request_duration_set(unit, ptp_id, clock_num, port_num, 
             duration);

    return (rc);
}

#endif /* defined(INCLUDE_PTP)*/

