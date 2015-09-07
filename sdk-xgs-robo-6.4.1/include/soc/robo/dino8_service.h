/*
 * $Id: dino8_service.h,v 1.2 Broadcom SDK $
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
 
#ifndef _DINO8_SERVICE_H
#define _DINO8_SERVICE_H

extern drv_if_t drv_dino8_services;


int drv_dino8_mac_set(int unit, soc_pbmp_t pbmp, uint32 mac_type, 
    uint8* mac, uint32 bpdu_idx);
int drv_dino8_mac_get(int unit, uint32 port, uint32 mac_type, 
    soc_pbmp_t *bmp, uint8* mac);

int drv_dino8_trunk_set(int unit, int tid, soc_pbmp_t bmp, 
    uint32 flag, uint32 hash_op);
int drv_dino8_trunk_get(int unit, int tid, soc_pbmp_t *bmp, 
    uint32 flag, uint32 *hash_op);
int drv_dino8_trunk_hash_field_add(int unit, uint32 field_type);
int drv_dino8_trunk_hash_field_remove(int unit, uint32 field_type);

int drv_dino8_queue_mode_set(int unit, soc_pbmp_t bmp, uint32 flag, 
    uint32 mode);
int drv_dino8_queue_mode_get(int unit, uint32 port, uint32 flag, 
    uint32 *mode);
int drv_dino8_queue_count_set(int unit, uint32 port_type, uint8 count);
int drv_dino8_queue_count_get(int unit, uint32 port_type, uint8 *count);
int drv_dino8_queue_prio_set(int unit, uint32 port, uint8 prio, 
    uint8 queue_n);
int drv_dino8_queue_prio_get(int unit, uint32 port, uint8 prio, 
    uint8 *queue_n);
int drv_dino8_queue_port_prio_to_queue_set(int unit, uint8 port, 
    uint8 prio,uint8 queue_n);
int drv_dino8_queue_port_prio_to_queue_get(int unit, uint8 port, 
    uint8 prio,uint8 * queue_n);
int drv_dino8_queue_prio_remap_set(int unit, uint32 port, uint8 pre_prio,
    uint8 prio);
int drv_dino8_queue_prio_remap_get(int unit, uint32 port, uint8 pre_prio,
    uint8 * prio);

int drv_dino8_trap_set(int unit, soc_pbmp_t bmp, uint32 trap_mask);
int drv_dino8_trap_get(int unit, soc_port_t port, uint32 *trap_mask);
int drv_dino8_snoop_set(int unit, uint32 snoop_mask);
int drv_dino8_snoop_get(int unit, uint32 *snoop_mask);

int drv_dino8_rate_config_set(int unit, soc_pbmp_t pbmp, 
    uint32 config_type, uint32 value);
int drv_dino8_rate_config_get(int unit, uint32 port, 
    uint32 config_type, uint32 *value);
int drv_dino8_rate_set
    (int unit, soc_pbmp_t bmp, uint8 queue_n, int direction, 
    uint32 flags, uint32 kbits_sec_min, 
    uint32 kbits_sec_max, uint32 burst_size);
int drv_dino8_rate_get
    (int unit, uint32 port, uint8 queue_n, int direction, 
    uint32 *flags, uint32 *kbits_sec_min, 
    uint32 *kbits_sec_max, uint32 *burst_size);

int drv_dino8_storm_control_enable_set
    (int unit, uint32 port, uint8 enable);
int drv_dino8_storm_control_enable_get
    (int unit, uint32 port, uint8 *enable);
int drv_dino8_storm_control_set(int unit, soc_pbmp_t bmp, uint32 type, 
    uint32 limit, uint32 burst_size);
int drv_dino8_storm_control_get(int unit, uint32 port, uint32 *type, 
    uint32 *limit, uint32 *burst_size);

int drv_dino8_igmp_mld_snoop_mode_get(int unit, int type, int *mode);
int drv_dino8_igmp_mld_snoop_mode_set(int unit, int type, int mode);

int drv_gex_port_set(int unit, soc_pbmp_t bmp, 
    uint32 prop_type, uint32 prop_val);
int drv_gex_port_get(int unit, int port, 
    uint32 prop_type, uint32 *prop_val);

int drv_gex_port_status_get(int unit, uint32 port, uint32 
    status_type, uint32 *val);

int drv_dino8_mem_read(int unit, uint32 mem, uint32 entry_id, 
    uint32 count, uint32 *entry);
int drv_dino8_mem_write(int unit, uint32 mem, uint32 entry_id, 
    uint32 count, uint32 *entry);
int drv_dino8_mem_field_get(int unit, uint32 mem, uint32 field_index, 
    uint32 *entry, uint32 *fld_data);
int drv_dino8_mem_field_set(int unit, uint32 mem, uint32 field_index, 
     uint32 *entry, uint32 *fld_data);
int drv_dino8_mem_clear(int unit, uint32 mem);
int drv_dino8_mem_search(int unit, uint32 mem, uint32 *key, 
    uint32 *entry, uint32 *entry1, uint32 flags);
int drv_dino8_mem_insert(int unit, uint32 mem, uint32 *entry, 
    uint32 flags);
int drv_dino8_mem_delete(int unit, uint32 mem, uint32 *entry, 
    uint32 flags);

int drv_dino8_dev_prop_get(int unit, uint32 prop_type, uint32 *prop_val);

#endif
