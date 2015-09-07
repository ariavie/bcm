/*
 * $Id: bradley.h 1.12 Broadcom SDK $
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
 * File:        firebolt.h
 * Purpose:     Function declarations for Bradley bcm functions
 */

#ifndef _BCM_INT_BRADLEY_H_
#define _BCM_INT_BRADLEY_H_

#include <bcm_int/esw/mbcm.h>

/****************************************************************
 *
 * Bradley functions
 *
 ****************************************************************/

extern int bcm_bradley_mcast_addr_add(int, bcm_mcast_addr_t *);
extern int bcm_bradley_mcast_addr_remove(int, bcm_mac_t, bcm_vlan_t);
extern int bcm_bradley_mcast_port_get(int, bcm_mac_t, bcm_vlan_t,
				      bcm_mcast_addr_t *);
extern int bcm_bradley_mcast_init(int);
extern int bcm_bradley_mcast_addr_add_w_l2mcindex(int unit,
						  bcm_mcast_addr_t *mcaddr);
extern int bcm_bradley_mcast_addr_remove_w_l2mcindex(int unit,
						     bcm_mcast_addr_t *mcaddr);
extern int bcm_bradley_mcast_port_add(int unit, bcm_mcast_addr_t *mcaddr);
extern int bcm_bradley_mcast_port_remove(int unit, bcm_mcast_addr_t *mcaddr);

extern int bcm_bradley_cosq_init(int unit);
extern int bcm_bradley_cosq_detach(int unit, int software_state_only);
extern int bcm_bradley_cosq_config_set(int unit, int numq);
extern int bcm_bradley_cosq_config_get(int unit, int *numq);
extern int bcm_bradley_cosq_mapping_set(int unit,
					bcm_port_t port,
					bcm_cos_t priority,
					bcm_cos_queue_t cosq);
extern int bcm_bradley_cosq_mapping_get(int unit,
					bcm_port_t port,
					bcm_cos_t priority,
					bcm_cos_queue_t *cosq);
extern int bcm_bradley_cosq_port_sched_set(int unit, bcm_pbmp_t, int mode,
					   const int weights[], int delay);
extern int bcm_bradley_cosq_port_sched_get(int unit, bcm_pbmp_t, int *mode,
					   int weights[], int *delay);
extern int bcm_bradley_cosq_sched_weight_max_get(int unit, int mode,
						 int *weight_max);
extern int bcm_bradley_cosq_port_bandwidth_set(int unit, bcm_port_t port,
					       bcm_cos_queue_t cosq,
					       uint32 kbits_sec_min,
					       uint32 kbits_sec_max,
                                               uint32 kbits_sec_burst,
					       uint32 flags);
extern int bcm_bradley_cosq_port_bandwidth_get(int unit, bcm_port_t port,
					       bcm_cos_queue_t cosq,
					       uint32 *kbits_sec_min,
					       uint32 *kbits_sec_max,
                                               uint32 *kbits_sec_burst,
					       uint32 *flags);
extern int bcm_bradley_cosq_discard_set(int unit, uint32 flags);
extern int bcm_bradley_cosq_discard_get(int unit, uint32 *flags);
extern int bcm_bradley_cosq_discard_port_set(int unit, bcm_port_t port,
					     bcm_cos_queue_t cosq,
					     uint32 color,
					     int drop_start,
					     int drop_slope,
					     int average_time);
extern int bcm_bradley_cosq_discard_port_get(int unit, bcm_port_t port,
					     bcm_cos_queue_t cosq,
					     uint32 color,
					     int *drop_start,
					     int *drop_slope,
					     int *average_time);
#ifdef BCM_WARM_BOOT_SUPPORT
extern int bcm_bradley_cosq_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void bcm_bradley_cosq_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

extern int _bcm_xgs3_ipmc_bitmap_set(int unit, int ipmc_idx,
                                     bcm_pbmp_t pbmp);
extern int _bcm_xgs3_ipmc_bitmap_get(int unit, int ipmc_idx,
                                     bcm_pbmp_t *pbmp);
extern int _bcm_xgs3_ipmc_bitmap_del(int unit, int ipmc_idx,
                                     bcm_pbmp_t pbmp);
extern int _bcm_xgs3_ipmc_bitmap_clear(int unit, int ipmc_idx);

#endif  /* !_BCM_INT_BRADLEY_H_ */
