/*
 * $Id: mim.h 1.10 Broadcom SDK $
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
 * This file contains mim definitions internal to the BCM library.
 */

#ifndef _BCM_INT_MIM_H
#define _BCM_INT_MIM_H

#include <bcm/mim.h>
#include <bcm_int/esw/vpn.h>

#define _BCM_MIM_VPN_TYPE_MIM         _BCM_VPN_TYPE_VFI
#define _BCM_MIM_VPN_TYPE_INVALID     _BCM_VPN_TYPE_INVALID

#define _BCM_MIM_VPN_TYPE_SHIFT       _BCM_VPN_TYPE_SHIFT
#define _BCM_MIM_VPN_TYPE_MASK        _BCM_VPN_TYPE_MASK

#define _BCM_MIM_VPN_ID_MASK          _BCM_VPN_ID_MASK

#define _BCM_MIM_VPN_TYPE_GET(_vpn_) \
    (((_vpn_) >> _BCM_MIM_VPN_TYPE_SHIFT) & _BCM_MIM_VPN_TYPE_MASK)

#define _BCM_IS_MIM_VPN(_vpn_)    \
        ((_BCM_MIM_VPN_TYPE_GET(_vpn_) >= _BCM_MIM_VPN_TYPE_MIM))

#define _BCM_MIM_VPN_IS_SET(_vpn_)    \
        ((_BCM_MIM_VPN_TYPE_GET(_vpn_) >= _BCM_MIM_VPN_TYPE_MIM))

#define _BCM_MIM_VPN_SET(_vpn_, _type_, _id_) \
    ((_vpn_) = ((_id_) + ((_type_) << _BCM_MIM_VPN_TYPE_SHIFT)))

#define _BCM_MIM_VPN_GET(_id_, _type_,  _vpn_) \
    ((_id_) = ((_vpn_) - ((_type_) << _BCM_MIM_VPN_TYPE_SHIFT)))

#define _BCM_MIM_VPN_INVALID 0xffff

#define _BCM_MIM_PORT_TYPE_NETWORK                    (1 << 0)
#define _BCM_MIM_PORT_TYPE_ACCESS_PORT                (1 << 1)
#define _BCM_MIM_PORT_TYPE_ACCESS_PORT_VLAN           (1 << 2)
#define _BCM_MIM_PORT_TYPE_ACCESS_PORT_VLAN_STACKED   (1 << 3)
#define _BCM_MIM_PORT_TYPE_ACCESS_LABEL               (1 << 4)
#define _BCM_MIM_PORT_TYPE_PEER                       (1 << 5)
#define _BCM_MIM_PORT_TYPE_ACCESS_PORT_TRUNK          (1 << 6)

/*
 * MIM VPN state - need to remember the ISID
 */
typedef struct _bcm_tr2_vpn_info_s {
    int isid;                /* The ISID value */
} _bcm_tr2_vpn_info_t;

/*
 * MIM port state - need to remember where to find the port.
 * Flags indicate the type of the port and hence the tables where the data is.
 * The index is used for the SOURCE_TRUNK_MAP table or for the
 * MPLS_STATION_TCAM.
 * The match attributes are the keys for the various hash tables.
 */
typedef struct _bcm_tr2_mim_port_info_s {
    uint32 flags;
    uint32 index; /* Index of memory where port info is programmed */
    bcm_trunk_t tgid; /* Trunk group ID */
    bcm_module_t modid; /* Module id */
    bcm_port_t port; /* Port */
    bcm_vlan_t match_vlan; /* S-Tag */
    bcm_vlan_t match_inner_vlan; /* C-Tag */
    bcm_mpls_label_t match_label; /* MPLS label */
    bcm_mac_t match_tunnel_srcmac; /* NVP src BMAC */
    bcm_vlan_t match_tunnel_vlan; /* NVP BVID */
    int match_count; /* Number of matches from VLAN_XLATE configured */
} _bcm_tr2_mim_port_info_t;

/*
 * Software book keeping for MIM related information
 */
typedef struct _bcm_tr2_mim_bookkeeping_s {
    _bcm_tr2_vpn_info_t *vpn_info;         /* VPN state */
    _bcm_tr2_mim_port_info_t *port_info;   /* VP state */
    SHR_BITDCL  *intf_bitmap;             /* L3 interfaces used */
} _bcm_tr2_mim_bookkeeping_t;

extern _bcm_tr2_mim_bookkeeping_t  _bcm_tr2_mim_bk_info[BCM_MAX_NUM_UNITS];

#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_mim_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */


#if defined(BCM_TRIUMPH3_SUPPORT) && defined(INCLUDE_L3)
extern int _bcm_tr2_mim_egr_vxlt_sd_tag_actions(int unit, 
                bcm_mim_port_t *mim_port, egr_vlan_xlate_entry_t *evxlt_entry);
extern int _bcm_tr2_mim_match_trunk_add(int unit, bcm_trunk_t tgid, int vp);


extern int _bcm_tr3_mim_peer_port_config_add(int unit, 
                        bcm_mim_port_t *mim_port, int vp, bcm_mim_vpn_t vpn);
extern int _bcm_tr_mim_match_trunk_delete(int unit, bcm_trunk_t tgid, int vp);

extern int _bcm_tr3_mim_peer_port_config_delete(int unit, int vp, 
                        bcm_mim_vpn_t vpn);
extern int _bcm_tr3_mim_match_add(int unit, bcm_mim_port_t *mim_port, int vp);
extern int _bcm_tr3_mim_match_delete(int unit, int vp);

#endif /* BCM_TRIUMPH3_SUPPORT */

#endif	/* !_BCM_INT_MIM_H */
