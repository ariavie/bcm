/*
 * $Id: switch.h,v 1.8 Broadcom SDK $
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
 * This file contains internal definitions for switch module to the BCM library.
 */

#ifndef _BCM_INT_SWITCH_H
#define _BCM_INT_SWITCH_H

#include <bcm/switch.h>

typedef int (*xlate_arg_f)(int unit, int arg, int set);

typedef struct {
    bcm_switch_control_t        type;
    uint32                      chip;
    soc_reg_t                   reg;
    soc_field_t                 field;
    xlate_arg_f                 xlate_arg;
    soc_feature_t               feature;
} bcm_switch_binding_t;


#define MMRP_ETHERTYPE_DEFAULT      0x88f6
#define SRP_ETHERTYPE_DEFAULT       0x1
#define TS_ETHERTYPE_DEFAULT        0x88f7
#define MMRP_MAC_OUI_DEFAULT        0x0180c2
#define SRP_MAC_OUI_DEFAULT         0x0
#define TS_MAC_OUI_DEFAULT          0x0180c2 
#define MMRP_MAC_NONOUI_DEFAULT     0x000020
#define SRP_MAC_NONOUI_DEFAULT      0x0
#define TS_MAC_NONOUI_DEFAULT       0x00000e  
#define TS_MAC_MSGBITMAP_DEFAULT    0x000d  
#define EP_COPY_TOCPU_DEFAULT       0x1


extern int _bcm_esw_switch_init(int unit);
extern int _bcm_esw_switch_detach(int unit);

extern int _bcm_switch_module_type_get(int unit, bcm_module_t mod,
                                       uint32 *mod_type);

extern int _bcm_switch_pkt_info_ecmp_hash_get(int unit,
                                              bcm_switch_pkt_info_t *pkt_info,
                                              bcm_gport_t *dst_gport,
                                              bcm_if_t *dst_intf);
extern int _bcm_td2_switch_pkt_info_hash_get(int unit,
                                             bcm_switch_pkt_info_t *pkt_info,
                                             bcm_gport_t *dst_gport,
                                             bcm_if_t *dst_intf);
extern int _bcm_tr3_switch_pkt_info_hash_get(int unit,
                                             bcm_switch_pkt_info_t *pkt_info,
                                             bcm_gport_t *dst_gport,
                                             bcm_if_t *dst_intf);
extern int _bcm_tr3_repl_head_entry_info_get(int unit, int *free);
 

#define _BCM_SWITCH_PKT_INFO_FLAG_TEST(_pkt_info, _flag_suffix) \
        (0 != (((_pkt_info)->flags) & BCM_SWITCH_PKT_INFO_##_flag_suffix))

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_scache_ptr_get(int unit, soc_scache_handle_t handle,
                                   int create, uint32 size,
                                   uint8 **scache_ptr, uint16 default_ver, 
                                   uint16 *recovered_ver);
extern int _bcm_mem_scache_sync(int unit);
extern int _bcm_switch_control_scache_sync(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT */

#ifdef BCM_GREYHOUND_SUPPORT
extern int
_bcm_gh_switch_control_port_binding_set(int unit,
                                bcm_port_t port,
                                bcm_switch_control_t type,
                                int arg, int *found);
extern int
_bcm_gh_switch_control_port_binding_get(int unit,
                                bcm_port_t port,
                                bcm_switch_control_t type,
                                int *arg, int *found);
#endif
#endif	/* !_BCM_INT_SWITCH_H */
