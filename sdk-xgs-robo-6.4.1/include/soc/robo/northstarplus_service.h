/*
 * $Id: northstarplus_service.h,v 1.1.2.3 Broadcom SDK $
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
 */
 
#ifndef _NORTHSTARPLUS_SERVICE_H
#define _NORTHSTARPLUS_SERVICE_H

int drv_gex_port_set(int unit, soc_pbmp_t bmp, 
    uint32 prop_type, uint32 prop_val);
int drv_gex_port_get(int unit, int port, 
    uint32 prop_type, uint32 *prop_val);

int drv_gex_port_status_get(int unit, uint32 port, 
    uint32 status_type, uint32 *val);

int drv_gex_security_set(int unit, soc_pbmp_t bmp, uint32 state, uint32 mask);
int drv_gex_security_get(int unit, uint32 port, uint32 *state, uint32 *mask);

int drv_gex_security_egress_set(int unit, soc_pbmp_t bmp, int enable);
int drv_gex_security_egress_get(int unit, int port, int *enable);

int drv_northstarplus_dev_prop_get(int unit, uint32 prop_type, 
            uint32 *prop_val);

int drv_northstarplus_dev_prop_set(int unit, uint32 prop_type, 
            uint32 prop_val);

int drv_northstarplus_port_pri_mapop_set(int unit, int port, int op_type, 
                uint32 pri_old, uint32 cfi_old, uint32 pri_new, uint32 cfi_new);
int drv_northstarplus_port_pri_mapop_get(int unit, int port, int op_type, 
           uint32 pri_old, uint32 cfi_old, uint32 *pri_new, uint32 *cfi_new);

int drv_nsp_rate_config_set(int unit, soc_pbmp_t pbmp, 
    uint32 config_type, uint32 value);
int drv_nsp_rate_config_get(int unit, uint32 port, 
    uint32 config_type, uint32 *value);
int drv_nsp_rate_set(int unit, soc_pbmp_t bmp, uint8 queue_n, int direction, 
    uint32 flags, uint32 kbits_sec_min, 
    uint32 kbits_sec_max, uint32 burst_size);
int drv_nsp_rate_get(int unit, uint32 port, uint8 queue_n, int direction, 
    uint32 *flags, uint32 *kbits_sec_min, 
    uint32 *kbits_sec_max, uint32 *burst_size);

int drv_nsp_storm_control_enable_set
    (int unit, uint32 port, uint8 enable);
int drv_nsp_storm_control_enable_get
    (int unit, uint32 port, uint8 *enable);
int drv_nsp_storm_control_set(int unit, soc_pbmp_t bmp, uint32 type, 
    uint32 limit, uint32 burst_size);
int drv_nsp_storm_control_get(int unit, uint32 port, uint32 *type, 
    uint32 *limit, uint32 *burst_size);

int drv_nsp_cfp_stat_get(int unit, uint32 stat_type, 
                                            uint32 index, uint32* counter);
int drv_nsp_cfp_stat_set(int unit, uint32 stat_type, 
                                            uint32 index, uint32 counter);

int drv_nsp_cfp_meter_rate_transform(int unit, uint32 kbits_sec, 
    uint32 kbits_burst, uint32 *bucket_size, uint32 * ref_cnt, uint32 *ref_unit);


int drv_nsp_fp_stat_type_get(int unit, int stage_id, 
        drv_policer_mode_t policer_mode,
        drv_field_stat_t stat, int *type1, int *type2, int *type3);

int drv_nsp_fp_stat_support_check(int unit, int stage_id, 
        int op, int param0, void *mode);

int drv_nsp_fp_policer_control(int unit,  int stage_id, 
        int op, void *entry, drv_policer_config_t *policer_cfg);

int drv_nsp_fp_action_conflict(int unit, int stage_id, 
        drv_field_action_t act1, drv_field_action_t act2);


int drv_nsp_wred_init(int unit);
int drv_nsp_wred_config_create(int unit, uint32 flags, 
                                drv_wred_config_t *config, int *wred_idx);
int drv_nsp_wred_config_set(int unit, int wred_id, 
                                            drv_wred_config_t *config);
int drv_nsp_wred_config_get(int unit, int wred_id, 
                                            drv_wred_config_t *config);
int drv_nsp_wred_config_destroy(int unit, int wred_id);
int drv_nsp_wred_map_attach(int unit, int wred_id, 
                                            drv_wred_map_info_t *map);
int drv_nsp_wred_map_deattach(int unit, int wred_id, 
                                            drv_wred_map_info_t *map);
int drv_nsp_wred_map_get(int unit, int *wred_id, 
                                            drv_wred_map_info_t *map);

int drv_nsp_wred_control_set(int unit, soc_port_t port, int queue, 
            drv_wred_control_t type, uint32 value);
int drv_nsp_wred_control_get(int unit, soc_port_t port, int queue, 
            drv_wred_control_t type, uint32 *value);
int drv_nsp_wred_counter_enable_set(int unit, soc_port_t port, int queue,
    drv_wred_counter_t type, int enable);
int drv_nsp_wred_counter_enable_get(int unit, soc_port_t port, int queue,
    drv_wred_counter_t type, int *enable);
int drv_nsp_wred_counter_set(int unit, soc_port_t port, int queue,
        drv_wred_counter_t type, uint64 value);
int drv_nsp_wred_counter_get(int unit, soc_port_t port, int queue,
        drv_wred_counter_t type, uint64 *value);

int drv_nsp_arl_learn_count_set(int unit, uint32 port, 
        uint32 type, int value);
int drv_nsp_arl_learn_count_get(int unit, uint32 port, 
        uint32 type, int *value);

int drv_nsp_queue_prio_remap_get(int unit, uint32 port, 
        uint8 pre_prio, uint8 *prio);
int drv_nsp_queue_prio_remap_set(int unit, uint32 port, 
        uint8 pre_prio, uint8 prio);
int drv_nsp_queue_prio_get(int unit, uint32 port, uint8 prio, uint8 *queue_n);
int drv_nsp_queue_prio_set(int unit, uint32 port, uint8 prio, uint8 queue_n);
int drv_nsp_queue_port_prio_to_queue_get(int unit, uint8 port, 
        uint8 prio, uint8 *queue_n);
int drv_nsp_queue_port_prio_to_queue_set(int unit, uint8 port, 
        uint8 prio, uint8 queue_n);
int drv_nsp_queue_mode_set(int unit, soc_pbmp_t bmp, uint32 flags, 
        uint32 mode);
int drv_nsp_queue_mode_get(int unit, uint32 port, uint32 flags, 
        uint32 *mode);
int drv_nsp_queue_WRR_weight_set(int unit, uint32 port_type, 
        soc_pbmp_t bmp, uint8 queue, uint32 weight);
int drv_nsp_queue_WRR_weight_get(int unit, uint32 port_type, 
        uint32 port, uint8 queue, uint32 *weight);
int drv_nsp_queue_qos_control_set(int unit, uint32 port, 
        uint32 type, uint32 state);
int drv_nsp_queue_qos_control_get(int unit, uint32 port, 
        uint32 type, uint32 *state);

extern drv_if_t drv_northstarplus_services;

#endif
