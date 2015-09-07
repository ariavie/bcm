/*
 * $Id: mpls.h 1.34 Broadcom SDK $
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
 * This file contains mpls definitions internal to the BCM library.
 */

#ifndef _BCM_INT_MPLS_H
#define _BCM_INT_MPLS_H

#include <bcm_int/esw/vpn.h>

#define _BCM_TR_MPLS_EXP_MAP_TABLE_TYPE_MASK    0x300
#define _BCM_TR_MPLS_EXP_MAP_TABLE_TYPE_INGRESS 0x100
#define _BCM_TR_MPLS_EXP_MAP_TABLE_TYPE_EGRESS  0x300
#define _BCM_TR_MPLS_EXP_MAP_TABLE_TYPE_EGRESS_L2  0x200
#define _BCM_TR_MPLS_EXP_MAP_TABLE_NUM_MASK     0x0ff

/*
 * MPLS port state - need to remember where to find the port.
 * Flags indicate the type of the port and hence the tables where the data is.
 * The match attributes are the keys for the various hash tables.
 */
#define _BCM_MPLS_PORT_MATCH_TYPE_NONE         (1 << 0)
#define _BCM_MPLS_PORT_MATCH_TYPE_VLAN   (1 << 1)
#define _BCM_MPLS_PORT_MATCH_TYPE_INNER_VLAN (1 << 2)
#define _BCM_MPLS_PORT_MATCH_TYPE_VLAN_STACKED        (1 << 3)
#define _BCM_MPLS_PORT_MATCH_TYPE_VLAN_PRI  (1 << 4)
#define _BCM_MPLS_PORT_MATCH_TYPE_PORT         (1 << 5)
#define _BCM_MPLS_PORT_MATCH_TYPE_TRUNK       (1 << 6)
#define _BCM_MPLS_PORT_MATCH_TYPE_LABEL        (1 << 7)
#define _BCM_MPLS_PORT_MATCH_TYPE_LABEL_PORT     (1 << 8)

/*
 * MPLS LABEL ACTION - indicate the ACTION associated with the LABEL 
 */

#define _BCM_MPLS_ACTION_NOOP  0x0
#define _BCM_MPLS_ACTION_PUSH  0x1
#define _BCM_MPLS_ACTION_SWAP  0x2
#define _BCM_MPLS_ACTION_RESERVED  0x3
#define _BCM_MPLS_ACTION_SEQUENCE 0x4

/*
 * MPLS FORWARDING ACTION - indicate the FORWARDING ACTION associated with the LABEL 
 */
#define _BCM_MPLS_FORWARD_NEXT_HOP  0x0
#define _BCM_MPLS_FORWARD_ECMP_GROUP  0x1
#define _BCM_MPLS_FORWARD_MULTICAST_GROUP  0x2

typedef struct _bcm_tr_mpls_match_port_info_s {
    uint32     flags;
    uint32     index; /* Index of memory where port info is programmed */
    bcm_trunk_t trunk_id; /* Trunk group ID */
    bcm_module_t modid; /* Module id */
    bcm_port_t port; /* Port */
    bcm_vlan_t match_vlan;
    bcm_vlan_t match_inner_vlan;
    bcm_mpls_label_t match_label; /* MPLS label */
    int match_count; /* Number of matches from VLAN_XLATE configured */
    bcm_trunk_t fo_trunk_id; /* vp-less failover port Trunk group ID */
    bcm_module_t fo_modid; /* vp-less failover port Module id */
    bcm_port_t fo_port; /* vp-less failover port, flags and label same as primary*/
} _bcm_tr_mpls_match_port_info_t;

typedef struct _bcm_mpls_vpws_vp_map_info_s {
    int             vp1; /* Access VP */
    int             vp2; /* Network VP */
} _bcm_mpls_vpws_vp_map_info_t;

/*
 * Software book keeping for MPLS related information
 */
typedef struct _bcm_tr_mpls_bookkeeping_s {
    int         initialized;        /* Set to TRUE when MPLS module initialized */
    SHR_BITDCL  *vrf_bitmap;        /* VRF bitmap */
    SHR_BITDCL  *vpws_bitmap;       /* VPWS vpn usage bitmap  */
    SHR_BITDCL  *vc_c_bitmap;       /* VC_AND_LABEL index bitmap (1st 4K)*/
    SHR_BITDCL  *vc_nc_bitmap;      /* VC_AND_LABEL index bitmap (2nd 4K)*/
    SHR_BITDCL  *pw_term_bitmap;    /* ING_PW_TERM_COUNTERS index bitmap */
    SHR_BITDCL  *pw_init_bitmap;    /* ING_PW_TERM_COUNTERS index bitmap */
    SHR_BITDCL  *tnl_bitmap;        /* EGR_IP_TUNNEL_MPLS index bitmap */
    SHR_BITDCL  *ip_tnl_bitmap;     /* EGR_IP_TUNNEL index bitmap */
    SHR_BITDCL  *egr_mpls_bitmap;   /* Egress EXP/PRI maps usage bitmap */
    uint32      *egr_mpls_hw_idx;   /* Actual profile number used */
    SHR_BITDCL  *ing_exp_map_bitmap;/* Ingress EXP map usage bitmap */
    SHR_BITDCL  *egr_l2_exp_map_bitmap; /* Egress_L2 EXP map usage bitmap */
    sal_mutex_t mpls_mutex;	        /* Protection mutex. */
    _bcm_tr_mpls_match_port_info_t *match_key; /* Match Key for Dual Hash */
    uint16       *vc_swap_ref_count; /* Reference count for VC_AND_SWAP entries */
    uint16       *egr_tunnel_ref_count; /* Reference count for EGR_MPLS_TUNNEL entries */
    _bcm_mpls_vpws_vp_map_info_t       *vpws_vp_map; /* VP map for VPWS */
} _bcm_tr_mpls_bookkeeping_t;

extern _bcm_tr_mpls_bookkeeping_t  _bcm_tr_mpls_bk_info[BCM_MAX_NUM_UNITS];

/* MPLS VC_AND_SWAP NextHop Indices List.  */
typedef struct _bcm_mpls_vp_nh_list_s {
    int  vp_nh_idx;
    struct _bcm_mpls_vp_nh_list_s  *link;
}_bcm_mpls_vp_nh_list_t;

/* MPLS Egress NhopList */
typedef struct _bcm_mpls_egr_nhopList_s {
    struct _bcm_mpls_egr_nhopList_s	*link;
    bcm_if_t  egr_if;                        /* Egress Object If */
    _bcm_mpls_vp_nh_list_t	*vp_head_ptr;
}_bcm_mpls_egr_nhopList_t;

#define _BCM_MPLS_VPN_TYPE_L3           _BCM_VPN_TYPE_L3
#define _BCM_MPLS_VPN_TYPE_VPWS         _BCM_VPN_TYPE_VPWS
#define _BCM_MPLS_VPN_TYPE_VPLS         _BCM_VPN_TYPE_VFI
#define _BCM_MPLS_VPN_TYPE_INVALID       _BCM_VPN_TYPE_INVALID

#define _BCM_MPLS_VPN_TYPE_SHIFT        _BCM_VPN_TYPE_SHIFT
#define _BCM_MPLS_VPN_TYPE_MASK         _BCM_VPN_TYPE_MASK

#define _BCM_MPLS_VPN_ID_MASK           _BCM_VPN_ID_MASK

#define _BCM_MPLS_VPN_SET(_vpn_, _type_, _id_) \
    ((_vpn_) = ((_id_) + ((_type_) << _BCM_MPLS_VPN_TYPE_SHIFT)))

#define _BCM_MPLS_VPN_GET(_id_, _type_,  _vpn_) \
    ((_id_) = ((_vpn_) - ((_type_) << _BCM_MPLS_VPN_TYPE_SHIFT)))

#define _BCM_MPLS_VPN_TYPE_GET(_vpn_) \
    (((_vpn_) >> _BCM_MPLS_VPN_TYPE_SHIFT) & _BCM_MPLS_VPN_TYPE_MASK)

#define _BCM_MPLS_VPN_IS_L3(_vpn_)    \
        ((_BCM_MPLS_VPN_TYPE_GET(_vpn_) >= _BCM_MPLS_VPN_TYPE_L3) \
        && (_BCM_MPLS_VPN_TYPE_GET(_vpn_) < _BCM_MPLS_VPN_TYPE_VPWS))

#define _BCM_MPLS_VPN_IS_VPLS(_vpn_)    \
        ((_BCM_MPLS_VPN_TYPE_GET(_vpn_) >= _BCM_MPLS_VPN_TYPE_VPLS) \
        && (_BCM_MPLS_VPN_TYPE_GET(_vpn_) < _BCM_MPLS_VPN_TYPE_INVALID))

#define _BCM_MPLS_VPN_IS_VPWS(_vpn_)    \
        ((_BCM_MPLS_VPN_TYPE_GET(_vpn_) >= _BCM_MPLS_VPN_TYPE_VPWS) \
        && (_BCM_MPLS_VPN_TYPE_GET(_vpn_) < _BCM_MPLS_VPN_TYPE_VPLS))

#define _BCM_MPLS_VPN_IS_SET(_vpn_)    \
        (_BCM_MPLS_VPN_IS_L3(_vpn_) \
        || _BCM_MPLS_VPN_IS_VPLS(_vpn_) \
        || _BCM_MPLS_VPN_IS_VPWS(_vpn_))

#define _BCM_MPLS_GPORT_FAILOVER_VPLESS  (1 << 24)
#define _BCM_MPLS_GPORT_FAILOVER_VPLESS_SET(_gp) ((_gp) | _BCM_MPLS_GPORT_FAILOVER_VPLESS)
#define _BCM_MPLS_GPORT_FAILOVER_VPLESS_GET(_gp) ((_gp) & _BCM_MPLS_GPORT_FAILOVER_VPLESS)
#define _BCM_MPLS_GPORT_FAILOVER_VPLESS_CLEAR(_gp) ((_gp) & (~_BCM_MPLS_GPORT_FAILOVER_VPLESS))

/* 
 * L3_IIF table is used to get the VRF & classid and is 
 * carved out as follows:
 *      0 - 4095 : 4K entries indexed by VLAN for non-tunnel/non-mpls packets
 *   4096 - 4607 : 512 entries directly indexed from L3_TUNNEL
 *   4608 - 6655 : 2K entries for MPLS pop & route packet (one for each VRF)
 *   6656 - 8191 : unused
 */
#define _BCM_TR_MPLS_L3_IIF_BASE        4095

/*
 * Index 63 of the ING_PRI_CNG_MAP table is a special value
 * in HW indicating no to trust the packet's 802.1p pri/cfi.
 * Here, we use index 62 as an identity mapping
 * (pkt_pri => int_pri) and (pkt_cfi => int_color)
 */
#define _BCM_TR_MPLS_PRI_CNG_MAP_IDENTITY   62
#define _BCM_TR_MPLS_PRI_CNG_MAP_NONE       63
#define _BCM_TR_MPLS_HASH_ELEMENTS 1024

#ifdef BCM_WARM_BOOT_SUPPORT
extern int _bcm_esw_mpls_sync(int unit);
extern void _bcm_mpls_sw_dump(int unit);
extern int _bcm_esw_mpls_match_key_recover(int unit);
#endif
#endif	/* !_BCM_INT_MPLS_H */
