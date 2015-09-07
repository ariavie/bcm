/*
 * $Id: fcmap_cmn.h 1.8 Broadcom SDK $
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
#ifndef FCMAP_CMN_H
#define FCMAP_CMN_H

#if defined(INCLUDE_FCMAP)

#include <bcm/fcmap.h>

extern int 
_bcm_common_fcmap_init(int unit);

extern int 
bcm_common_fcmap_port_traverse(int unit, 
                             bcm_fcmap_port_traverse_cb callback, 
                             void *user_data);
extern int
bcm_common_fcmap_port_config_set(int unit, bcm_port_t port, 
                               bcm_fcmap_port_config_t *cfg);

extern int
bcm_common_fcmap_port_config_get(int unit, bcm_port_t port, 
                               bcm_fcmap_port_config_t *cfg);

extern int
bcm_common_fcmap_port_speed_set(int unit, bcm_port_t port, 
                               bcm_fcmap_port_speed_t speed);

extern int
bcm_common_fcmap_port_enable(int unit, bcm_port_t port);


extern int
bcm_common_fcmap_port_shutdown(int unit, bcm_port_t port);


extern int
bcm_common_fcmap_port_link_reset(int unit, bcm_port_t port);

extern int
bcm_common_fcmap_vlan_map_add(int unit, bcm_port_t port, 
                               bcm_fcmap_vlan_vsan_map_t *vlan);

extern int
bcm_common_fcmap_vlan_map_get(int unit, bcm_port_t port, 
                               bcm_fcmap_vlan_vsan_map_t *vlan);

extern int
bcm_common_fcmap_vlan_map_delete(int unit, bcm_port_t port, 
                               bcm_fcmap_vlan_vsan_map_t *vlan);


extern int 
bcm_common_fcmap_stat_clear(int unit, bcm_port_t port);

extern int 
bcm_common_fcmap_stat_get(int unit, bcm_port_t port, 
                        bcm_fcmap_stat_t stat, 
                        uint64 *val);

extern int 
bcm_common_fcmap_stat_get32(int unit, bcm_port_t port, 
                          bcm_fcmap_stat_t stat,
                          uint32 *val);


extern int 
bcm_common_fcmap_event_register(int unit, bcm_fcmap_event_cb cb, void *user_data);

extern int 
bcm_common_fcmap_event_unregister(int unit, bcm_fcmap_event_cb cb);

extern int
bcm_common_fcmap_event_enable_set(int unit,
                                bcm_fcmap_event_t event, int enable);

extern int
bcm_common_fcmap_event_enable_get(int unit,
                                bcm_fcmap_event_t event, int *enable);

/*
 * Debug/Print config set/get.
 */
extern int bcm_common_fcmap_config_print(uint32 level);

#endif /* defined(INCLUDE_FCMAP) */

#endif /* FCMAP_CMN_H */

