/*
 * $Id: enduro.h,v 1.15 Broadcom SDK $
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
 * File:        enduro.h
 * Purpose:     Function declarations for Enduro  bcm functions
 */

#ifndef _BCM_INT_ENDURO_H_
#define _BCM_INT_ENDURO_H_
#include <bcm_int/esw/l2.h>
#ifdef BCM_FIELD_SUPPORT 
#include <bcm_int/esw/field.h>
#endif /* BCM_FIELD_SUPPORT */
#if defined(BCM_ENDURO_SUPPORT)
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/oam.h>


#if defined(BCM_KATANA_SUPPORT) && defined(INCLUDE_BHH)
extern int bcm_en_oam_loopback_delete(int unit, bcm_oam_loopback_t *loopback);
extern int bcm_en_oam_loopback_add(int unit, bcm_oam_loopback_t *loopback);
extern int bcm_en_oam_loopback_get(int unit, bcm_oam_loopback_t *loopback);
#endif
extern int bcm_en_oam_init(int unit);
extern int bcm_en_oam_detach(int unit);
extern int bcm_en_oam_lock(int unit);
extern int bcm_en_oam_unlock(int unit);
extern int bcm_en_oam_group_create(int unit,
    bcm_oam_group_info_t *group_info);
extern int bcm_en_oam_group_get(int unit, bcm_oam_group_t group, 
    bcm_oam_group_info_t *group_info);
extern int bcm_en_oam_group_destroy(int unit, bcm_oam_group_t group);
extern int bcm_en_oam_group_destroy_all(int unit);
extern int bcm_en_oam_group_traverse(int unit, bcm_oam_group_traverse_cb cb, 
    void *user_data);
extern int bcm_en_oam_endpoint_create(int unit, 
    bcm_oam_endpoint_info_t *endpoint_info);
extern int bcm_en_oam_endpoint_get(int unit, bcm_oam_endpoint_t endpoint, 
    bcm_oam_endpoint_info_t *endpoint_info);
extern int bcm_en_oam_endpoint_destroy(int unit, bcm_oam_endpoint_t endpoint);
extern int bcm_en_oam_endpoint_destroy_all(int unit, bcm_oam_group_t group);
extern int bcm_en_oam_endpoint_traverse(int unit, bcm_oam_group_t group, 
    bcm_oam_endpoint_traverse_cb cb, void *user_data);
extern int bcm_en_oam_event_register(int unit, 
    bcm_oam_event_types_t event_types, bcm_oam_event_cb cb, 
    void *user_data);
extern int bcm_en_oam_event_unregister(int unit, 
    bcm_oam_event_types_t event_types, bcm_oam_event_cb cb);
#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_en_oam_sync(int unit);
#endif
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_en_oam_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */
extern int bcm_port_settings_init(int unit, bcm_port_t p);
extern int _bcm_en_port_lanes_set(int unit, bcm_port_t port, int value);
extern int _bcm_en_port_lanes_get(int unit, bcm_port_t port, int *value);
#if defined(BCM_FIELD_SUPPORT)
extern int _bcm_field_en_init(int unit, _field_control_t *fc);
#endif /* BCM_FIELD_SUPPORT */

extern int bcm_enduro_vlan_virtual_init(int unit);
extern int bcm_enduro_vlan_virtual_detach(int unit);
extern int bcm_enduro_vlan_vp_create(int unit, bcm_vlan_port_t *vlan_vp);
extern int bcm_enduro_vlan_vp_destroy(int unit, bcm_gport_t gport);
extern int bcm_enduro_vlan_vp_find(int unit, bcm_vlan_port_t *vlan_vp);
extern int bcm_enduro_vlan_vp_add(int unit, bcm_vlan_t vlan, bcm_gport_t gport, 
                                   int is_untagged);
extern int bcm_enduro_vlan_vp_delete(int unit, bcm_vlan_t vlan,
                                      bcm_gport_t gport); 
extern int bcm_enduro_vlan_vp_delete_all(int unit, bcm_vlan_t vlan);
extern int bcm_enduro_vlan_vp_get(int unit, bcm_vlan_t vlan, bcm_gport_t gport,
                                   int *is_untagged); 
extern int bcm_enduro_vlan_vp_get_all(int unit, bcm_vlan_t vlan, int array_max,
        bcm_gport_t *gport_array, int *is_untagged_array, int *array_size);
extern int bcm_enduro_vlan_vp_update_vlan_pbmp(int unit, bcm_vlan_t vlan,
        bcm_pbmp_t *pbmp);
extern int _bcm_enduro_vlan_port_resolve(int unit, bcm_gport_t vlan_port_id,
                          bcm_module_t *modid, bcm_port_t *port,
                          bcm_trunk_t *trunk_id, int *id);

#ifdef BCM_WARM_BOOT_SUPPORT
extern int bcm_enduro_vlan_virtual_reinit(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void bcm_enduro_vlan_vp_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif  /* BCM_ENDURO_SUPPORT */
#endif  /* !_BCM_INT_ENDURO_H_ */
