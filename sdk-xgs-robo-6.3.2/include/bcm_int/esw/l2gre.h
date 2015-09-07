/*
 * $Id: l2gre.h 1.11 Broadcom SDK $
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
 * This file contains L2GRE definitions internal to the BCM library.
 */


#ifndef _BCM_INT_L2GRE_H
#define _BCM_INT_L2GRE_H

#include <bcm/types.h>
#include <bcm/l2gre.h>
#include <bcm_int/esw/vpn.h>

#if defined(BCM_TRIUMPH3_SUPPORT)

#define _BCM_MAX_NUM_L2GRE_TUNNEL 2048
#define _BCM_L2GRE_VFI_INVALID 0x3FFF

/* 
 * L2GRE EGR_L3_NEXT_HOP View 
*/
#define _BCM_L2GRE_EGR_NEXT_HOP_L3_VIEW         0x0
#define _BCM_L2GRE_EGR_NEXT_HOP_SDTAG_VIEW  0x2
#define _BCM_L2GRE_EGR_NEXT_HOP_L3MC_VIEW    0x7

/* 
 * L2GRE Lookup Key Types 
 */
#define _BCM_TD2_L2GRE_KEY_TYPE_TUNNEL_VPNID_VFI 0x04
#define _BCM_L2GRE_KEY_TYPE_TUNNEL           0x06
#define _BCM_L2GRE_KEY_TYPE_VPNID_VFI      0x07
#define _BCM_TR3_L2GRE_KEY_TYPE_TUNNEL_VPNID_VFI    0x08

#define _BCM_TD2_L2GRE_KEY_TYPE_LOOKUP_DIP    0x0D
#define _BCM_TR3_L2GRE_KEY_TYPE_LOOKUP_DIP    0x1A

#define _BCM_TR3_L2GRE_KEY_TYPE_LOOKUP_VFI_DVP    0x05
#define _BCM_TD2_L2GRE_KEY_TYPE_LOOKUP_VFI_DVP    0x06

/* L2GRE SVP Type */
#define _BCM_L2GRE_SOURCE_VP_TYPE_INVALID    0x0
#define _BCM_L2GRE_SOURCE_VP_TYPE_VFI            0x1

/* L2GRE DVP Type */
#define  _BCM_L2GRE_DEST_VP_TYPE_ACCESS       0x0
#define  _BCM_L2GRE_DEST_VP_ATTRIBUTE_TYPE  0x3
#define  _BCM_L2GRE_DEST_VP_TYPE_NETWORK   0x2

#define  _BCM_L2GRE_INGRESS_DEST_VP_TYPE  0x2
#define  _BCM_L2GRE_EGRESS_DEST_VP_TYPE   0x3


/* 
 * L2GRE EGR_L3_NEXT_HOP View 
*/
#define _BCM_L2GRE_EGR_NEXT_HOP_L3_VIEW         0x0
#define _BCM_L2GRE_EGR_NEXT_HOP_SDTAG_VIEW  0x2
#define _BCM_TD2_L2GRE_EGR_NEXT_HOP_L3MC_VIEW    0x7

/*
 * L2GRE port state - need to remember where to find the port.
 * Flags indicate the type of the port and hence the tables where the data is.
 * The match attributes are the keys for the various hash tables.
 */

#define _BCM_L2GRE_PORT_MATCH_TYPE_NONE         (1 << 0)
#define _BCM_L2GRE_PORT_MATCH_TYPE_VLAN   (1 << 1)
#define _BCM_L2GRE_PORT_MATCH_TYPE_INNER_VLAN (1 << 2)
#define _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_STACKED        (1 << 3)
#define _BCM_L2GRE_PORT_MATCH_TYPE_VLAN_PRI  (1 << 4)
#define _BCM_L2GRE_PORT_MATCH_TYPE_PORT         (1 << 5)
#define _BCM_L2GRE_PORT_MATCH_TYPE_TRUNK       (1 << 6)
#define _BCM_L2GRE_PORT_MATCH_TYPE_VPNID        (1 << 7)
#define _BCM_L2GRE_PORT_MATCH_TYPE_TUNNEL_VPNID     (1 << 8)

/* L2GRE Multicast state */
#define _BCM_L2GRE_MULTICAST_NONE       0x0
#define _BCM_L2GRE_MULTICAST_BUD         0x1
#define _BCM_L2GRE_MULTICAST_LEAF        0x2
#define _BCM_L2GRE_MULTICAST_TRANSIT   0x4

typedef struct _bcm_tr3_l2gre_nh_info_s {
    int      entry_type;
    int      dvp_is_network;
    int      sd_tag_action_present;
    int      sd_tag_action_not_present;
    int      dvp;
    int      intf_num;
    int      sd_tag_vlan;
    int      sd_tag_pri;
    int      sd_tag_cfi;
    int      vc_swap_index;
    int      tpid_index;
    int      is_eline;
} _bcm_tr3_l2gre_nh_info_t;

typedef struct _bcm_l2gre_match_port_info_s {
    uint32     flags;
    uint32     index; /* Index of memory where port info is programmed */
    bcm_trunk_t trunk_id; /* Trunk group ID */
    bcm_module_t modid; /* Module id */
    bcm_port_t port; /* Port */
    bcm_vlan_t match_vlan;
    bcm_vlan_t match_inner_vlan;
    uint32 match_vpnid; /* VPNID */
    uint32 match_tunnel_sip; /* Tunnel SIP */
    int match_count; /* Number of matches from VLAN_XLATE configured */
} _bcm_l2gre_match_port_info_t;

typedef struct _bcm_l2gre_eline_vp_map_info_s {
    int             vp1; /* Access VP */
    int             vp2; /* Network VP */
} _bcm_l2gre_eline_vp_map_info_t;

typedef struct _bcm_l2gre_tunnel_terminator_endpoint_s {
     uint32      dip; /* Tunnel DIP */
     uint32      sip; /* Tunnel SIP */
} _bcm_l2gre_tunnel_terminator_endpoint_t;

/*
 * Software book keeping for L2GRE  related information
 */
typedef struct _bcm_tr3_l2gre_bookkeeping_s {
    int         initialized;        /* Set to TRUE when L2GRE module initialized */
    sal_mutex_t    l2gre_mutex;    /* Protection mutex. */
    SHR_BITDCL *l2gre_ip_tnl_bitmap; /* EGR_IP_TUNNEL index bitmap */
    _bcm_l2gre_match_port_info_t *match_key; /* Match Key */
    _bcm_l2gre_tunnel_terminator_endpoint_t    l2gre_tunnel[_BCM_MAX_NUM_L2GRE_TUNNEL];
} _bcm_tr3_l2gre_bookkeeping_t;

extern _bcm_tr3_l2gre_bookkeeping_t *_bcm_tr3_l2gre_bk_info[BCM_MAX_NUM_UNITS];
extern void _bcm_l2gre_sw_dump(int unit);

/* Generic memory allocation routine. */
#define BCM_TR3_L2GRE_ALLOC(_ptr_,_size_,_descr_)                      \
            do {                                                     \
                if ((NULL == (_ptr_))) {                             \
                   _ptr_ = sal_alloc((_size_),(_descr_));            \
                }                                                    \
                if((_ptr_) != NULL) {                                \
                    sal_memset((_ptr_), 0, (_size_));                \
                }                                                    \
            } while (0)

#define _BCM_L2GRE_VPN_INVALID 0xffff

#define BCM_STAT_L2GRE_FLEX_COUNTER_MAX_TABLES 4
/*
 * Do not change - Implementation assumes
 * _BCM_L2GRE_VPN_TYPE_ELINE == _BCM_L2GRE_VPN_TYPE_ELAN
 */
#define _BCM_L2GRE_VPN_TYPE_ELINE         _BCM_VPN_TYPE_VFI
#define _BCM_L2GRE_VPN_TYPE_ELAN          _BCM_VPN_TYPE_VFI

#define _BCM_L2GRE_VPN_TYPE_SHIFT        _BCM_VPN_TYPE_SHIFT
#define _BCM_L2GRE_VPN_TYPE_MASK         _BCM_VPN_TYPE_MASK
#define _BCM_L2GRE_VPN_ID_MASK             _BCM_VPN_ID_MASK

#define _BCM_L2GRE_VPN_SET(_vpn_, _type_, _id_) \
    ((_vpn_) = ((_id_) + ((_type_) << _BCM_L2GRE_VPN_TYPE_SHIFT)))

#define _BCM_L2GRE_VPN_GET(_id_, _type_,  _vpn_) \
    ((_id_) = ((_vpn_) - ((_type_) << _BCM_L2GRE_VPN_TYPE_SHIFT)))


#endif /* BCM_TRIUMPH3_SUPPORT */
#endif	/* !_BCM_INT_L2GRE_H */

