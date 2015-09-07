/*
 * $Id: multicast.h,v 1.10 Broadcom SDK $
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
 * This file contains multicast definitions internal to the BCM library.
 */

#ifndef _BCM_INT_ESW_MULTICAST_H_
#define _BCM_INT_ESW_MULTICAST_H_


#define _BCM_VIRTUAL_TYPES_MASK                                 \
        (BCM_MULTICAST_TYPE_VPLS | BCM_MULTICAST_TYPE_MIM |      \
         BCM_MULTICAST_TYPE_SUBPORT | BCM_MULTICAST_TYPE_TRILL | \
         BCM_MULTICAST_TYPE_WLAN | BCM_MULTICAST_TYPE_VLAN | \
         BCM_MULTICAST_TYPE_NIV)

#define BCM_MULTICAST_PORT_MAX  4096

extern int _bcm_esw_multicast_flags_to_group_type(uint32 flags);
extern uint32 _bcm_esw_multicast_group_type_to_flags(int group_type);

extern int _bcm_tr_multicast_ipmc_group_type_set(int unit,
                                                 bcm_multicast_t group);
extern int _bcm_tr_multicast_ipmc_group_type_get(int unit, uint32 ipmc_id,
                                                 bcm_multicast_t *group);

extern int _bcm_esw_ipmc_egress_intf_add(int unit, int ipmc_id, bcm_port_t port,
                                     int nh_index, int is_l3);
extern int _bcm_esw_ipmc_egress_intf_delete(int unit, int ipmc_id,
                                           bcm_port_t port, int if_max,
                                           int nh_index, int is_l3);
extern int _bcm_esw_multicast_ipmc_read(int unit, int ipmc_id, 
                                        bcm_pbmp_t *l2_pbmp, bcm_pbmp_t *l3_pbmp);
extern int _bcm_esw_multicast_ipmc_write(int unit, int ipmc_id, 
                                         bcm_pbmp_t l2_pbmp, bcm_pbmp_t l3_pbmp, 
                                         int valid);

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_multicast_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_multicast_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif	/* !_BCM_INT_ESW_MULTICAST_H_ */
