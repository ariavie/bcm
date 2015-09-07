/*
 * $Id: hurricane.h 1.18 Broadcom SDK $
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
 * File:        hurricane.h
 * Purpose:     Function declarations for Hurricane  bcm functions
 */

#ifndef _BCM_INT_HURRICANE_H_
#define _BCM_INT_HURRICANE_H_
#include <bcm_int/esw/l2.h>
#ifdef BCM_FIELD_SUPPORT 
#include <bcm_int/esw/field.h>
#endif /* BCM_FIELD_SUPPORT */

#if defined(BCM_HURRICANE_SUPPORT)
#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw/oam.h>


extern int bcm_hu_cosq_init(int unit);
extern int bcm_hu_cosq_detach(int unit, int software_state_only);
extern int bcm_hu_cosq_config_set(int unit, int numq);
extern int bcm_hu_cosq_config_get(int unit, int *numq);
extern int bcm_hu_cosq_mapping_set(int unit, bcm_port_t port,
                        bcm_cos_t priority, bcm_cos_queue_t cosq);
extern int bcm_hu_cosq_mapping_get(int unit, bcm_port_t port,
                        bcm_cos_t priority, bcm_cos_queue_t *cosq);
extern int     
bcm_hu_cosq_port_sched_set(int unit, bcm_pbmp_t pbm,
                              int mode, const int weights[], int delay);
extern int
bcm_hu_cosq_port_sched_get(int unit, bcm_pbmp_t pbm,
                           int *mode, int weights[], int *delay);
extern int
bcm_hu_cosq_port_bandwidth_set(int unit, bcm_port_t port,
                               bcm_cos_queue_t cosq,
                               uint32 kbits_sec_min,
                               uint32 kbits_sec_max,
                               uint32 kbits_sec_burst,
                               uint32 flags);
extern int
bcm_hu_cosq_port_bandwidth_get(int unit, bcm_port_t port,
                               bcm_cos_queue_t cosq,
                               uint32 *kbits_sec_min,
                               uint32 *kbits_sec_max,
                               uint32 *kbits_sec_burst,
                               uint32 *flags);

extern int
bcm_hu_cosq_discard_set(int unit, uint32 flags);
extern int
bcm_hu_cosq_discard_get(int unit, uint32* flags);
extern int
bcm_hu_cosq_discard_port_set(int unit, bcm_port_t port,
                             bcm_cos_queue_t cosq,
                             uint32 color,
                             int drop_start,
                             int drop_slope,
                             int average_time);
extern int
bcm_hu_cosq_discard_port_get(int unit, bcm_port_t port,
                             bcm_cos_queue_t cosq,
                             uint32 color,
                             int *drop_start,
                             int *drop_slope,
                             int *average_time);

extern int
bcm_hu_cosq_sched_weight_max_get(int unit, int mode, int *weight_max);

extern int
bcm_hu_cosq_gport_bandwidth_get(int unit, bcm_gport_t gport,
                                bcm_cos_queue_t cosq, uint32 *kbits_sec_min,
                                uint32 *kbits_sec_max, uint32 *flags);

extern int
bcm_hu_cosq_gport_sched_set(int unit, bcm_gport_t gport,
                            bcm_cos_queue_t cosq, int mode, int weight);

extern int
bcm_hu_cosq_gport_sched_get(int unit, bcm_gport_t gport, 
                            bcm_cos_queue_t cosq, int *mode, int *weight);

extern int
bcm_hu_cosq_gport_bandwidth_burst_set(int unit, bcm_gport_t gport,
                                      bcm_cos_queue_t cosq,
                                      uint32 kbits_burst);

extern int
bcm_hu_cosq_gport_bandwidth_burst_get(int unit, bcm_gport_t gport,
                                      bcm_cos_queue_t cosq,
                                      uint32 *kbits_burst);

extern int
bcm_hu_cosq_gport_bandwidth_set(int unit, bcm_gport_t gport,
                                bcm_cos_queue_t cosq, uint32 kbits_sec_min,
                                uint32 kbits_sec_max, uint32 flags);

extern int
bcm_hu_cosq_gport_discard_set(int unit, bcm_gport_t port, bcm_cos_queue_t cosq,
                              bcm_cosq_gport_discard_t *discard);

extern int
bcm_hu_cosq_gport_discard_get(int unit, bcm_gport_t port, bcm_cos_queue_t cosq,
                              bcm_cosq_gport_discard_t *discard);

#ifdef BCM_WARM_BOOT_SUPPORT
extern int bcm_hu_cosq_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void bcm_hu_cosq_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#ifdef INCLUDE_L3
extern int _bcm_hu_l3_intf_mtu_get(int unit, _bcm_l3_intf_cfg_t *intf_info);
extern int _bcm_hu_l3_intf_mtu_set(int unit, _bcm_l3_intf_cfg_t *intf_info);
#endif  /* INCLUDE_L3 */

extern int _bcm_hu_port_lanes_set(int unit, bcm_port_t port, int value);
extern int _bcm_hu_port_lanes_get(int unit, bcm_port_t port, int *value);

#ifdef BCM_FIELD_SUPPORT 
extern int _bcm_field_hu_init(int unit, _field_control_t *fc);
#endif

#endif  /* BCM_HURRICANE_SUPPORT */
#endif  /* !_BCM_INT_HURRICANE_H_ */
