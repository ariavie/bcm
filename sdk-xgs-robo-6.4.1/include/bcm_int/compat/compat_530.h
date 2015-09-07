/*
 * $Id: compat_530.h,v 1.2 Broadcom SDK $
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
 * RPC Compatibility with sdk-5.3.0 routines
 */

#ifndef _COMPAT_530_H_
#define _COMPAT_530_H_

#ifdef	BCM_RPC_SUPPORT

#include <bcm/types.h>
#include <bcm/l3.h>
#include <bcm/tunnel.h>

typedef struct bcm_compat530_l2_addr_s {
    uint32              flags;          /* BCM_L2_XXX flags */
    bcm_mac_t           mac;            /* 802.3 MAC address */
    bcm_vlan_t          vid;            /* VLAN identifier */
    int                 port;           /* Zero-based port number */
    int                 modid;          /* XGS: modid */
    bcm_trunk_t         tgid;           /* Trunk group ID */
    int                 rtag;           /* Trunk port select formula */
    bcm_cos_t           cos_dst;        /* COS based on dst addr */
    bcm_cos_t           cos_src;        /* COS based on src addr */
    int                 l2mc_index;     /* XGS: index in L2MC table */
    bcm_pbmp_t          block_bitmap;   /* XGS: blocked egress bitmap */
    int			auth;		/* Used if auth enabled on port */
} bcm_compat530_l2_addr_t;

extern int bcm_compat530_l2_addr_add(
    int unit,
    bcm_compat530_l2_addr_t *l2addr);
extern int bcm_compat530_l2_conflict_get(
    int unit,
    bcm_compat530_l2_addr_t *addr,
    bcm_compat530_l2_addr_t *cf_array,
    int cf_max,
    int *cf_count);
extern int bcm_compat530_l2_addr_get(
    int unit,
    bcm_mac_t mac_addr,
    bcm_vlan_t vid,
    bcm_compat530_l2_addr_t *l2addr);


#ifdef INCLUDE_L3

typedef struct bcm_compat530_l3_tunnel_initiator_s {
    uint32               l3t_flags;      /* Config flags */
    bcm_tunnel_type_t    l3t_type;       /* Tunnel type */
    int                  l3t_ttl;        /* Tunnel header TTL */
    bcm_mac_t            l3t_dmac;       /* DA MAC */
    int                  l3t_ip4_df_sel; /* IP tunnel hdr DF bit for IPv4 */
    int                  l3t_ip6_df_sel; /* IP tunnel hdr DF bit for IPv6 */
    bcm_ip_t             l3t_dip;        /* Tunnel header DIP address */
    bcm_ip_t             l3t_sip;        /* Tunnel header SIP address */
    int                  l3t_dscp_sel;   /* Tunnel header DSCP select */
    int                  l3t_dscp;       /* Tunnel header DSCP value */
} bcm_compat530_l3_tunnel_initiator_t;

typedef struct bcm_compat530_l3_tunnel_terminator_s {
    uint32               l3t_flags;    /* Config flags */
    bcm_ip_t             l3t_sip;      /* SIP for tunnel header match */
    bcm_ip_t             l3t_dip;      /* DIP for tunnel header match */
    bcm_ip_t             l3t_sip_mask; /* SIP mask */
    bcm_ip_t             l3t_dip_mask; /* DIP mask */
    uint32               l3t_udp_dst_port;  /* UDP dst port for UDP packets */
    uint32               l3t_udp_src_port;  /* UDP src port for UDP packets */
    bcm_tunnel_type_t    l3t_type;     /* Tunnel type */
    bcm_pbmp_t           l3t_pbmp;     /* Port bitmap for this tunnel */
    bcm_vlan_t           l3t_vlan;     /* The VLAN ID for IPMC lookup */
} bcm_compat530_l3_tunnel_terminator_t;

extern int bcm_compat530_l3_tunnel_initiator_set(
    int unit,
    bcm_l3_intf_t *intf,
    bcm_compat530_l3_tunnel_initiator_t *tunnel);
extern int bcm_compat530_l3_tunnel_initiator_get(
    int unit,
    bcm_l3_intf_t *intf,
    bcm_compat530_l3_tunnel_initiator_t *tunnel);
extern int bcm_compat530_l3_tunnel_terminator_add(
    int unit,
    bcm_compat530_l3_tunnel_terminator_t *info);
extern int bcm_compat530_l3_tunnel_terminator_delete(
    int unit,
    bcm_compat530_l3_tunnel_terminator_t *info);
extern int bcm_compat530_l3_tunnel_terminator_update(
    int unit,
    bcm_compat530_l3_tunnel_terminator_t *info);
extern int bcm_compat530_l3_tunnel_terminator_get(
    int unit,
    bcm_compat530_l3_tunnel_terminator_t *info);

#endif	/* INCLUDE_L3 */

#endif	/* BCM_RPC_SUPPORT */

#endif	/* !_COMPAT_530_H */
