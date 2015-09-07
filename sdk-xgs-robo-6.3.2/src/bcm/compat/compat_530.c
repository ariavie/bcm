/*
 * $Id: compat_530.c 1.9 Broadcom SDK $
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

#ifdef	BCM_RPC_SUPPORT

#include <sal/core/libc.h>
#include <shared/alloc.h>

#include <bcm/types.h>
#include <bcm/error.h>
#include <bcm/l2.h>
#include <bcm/l3.h>

#include <bcm_int/compat/compat_530.h>

/* bcm_l2_addr_t conversion functions */
STATIC void
_bcm_compat530in_l2_addr_add_t(
    bcm_compat530_l2_addr_t *from,
    bcm_l2_addr_t *to)
{
    to->flags = from->flags;
    sal_memcpy(to->mac, from->mac, sizeof(bcm_mac_t));
    to->vid = from->vid;
    to->port = from->port;
    to->modid = from->modid;
    to->tgid = from->tgid;
    to->cos_dst = from->cos_dst;
    to->cos_src = from->cos_src;
    to->l2mc_index = from->l2mc_index;
    to->block_bitmap = from->block_bitmap;
    to->auth = from->auth;
    to->group = 0;
}

STATIC void
_bcm_compat530out_l2_addr_add_t(
    bcm_l2_addr_t *from,
    bcm_compat530_l2_addr_t *to)
{
    to->flags = from->flags;
    sal_memcpy(to->mac, from->mac, sizeof(bcm_mac_t));
    to->vid = from->vid;
    to->port = from->port;
    to->modid = from->modid;
    to->tgid = from->tgid;
    to->cos_dst = from->cos_dst;
    to->cos_src = from->cos_src;
    to->l2mc_index = from->l2mc_index;
    to->block_bitmap = from->block_bitmap;
    to->auth = from->auth;
    /* ignore from->group */
}

int
bcm_compat530_l2_addr_add(
    int unit,
    bcm_compat530_l2_addr_t *l2addr)
{
    int			rv;
    bcm_l2_addr_t	nl2addr;

    if (l2addr == NULL) {
        return BCM_E_PARAM;
    }
    _bcm_compat530in_l2_addr_add_t(l2addr, &nl2addr);
    rv = bcm_l2_addr_add(unit, &nl2addr);
    return rv;
}

int
bcm_compat530_l2_conflict_get(
    int unit,
    bcm_compat530_l2_addr_t *addr,
    bcm_compat530_l2_addr_t *cf_array,
    int cf_max,
    int *cf_count)
{
    int			rv, i;
    bcm_l2_addr_t	naddr;
    bcm_l2_addr_t	*ncf_array;

    if (addr == NULL || cf_array == NULL) {
        return BCM_E_PARAM;
    }
    ncf_array = sal_alloc(sizeof(bcm_l2_addr_t) * cf_max,
			  "compat530_l2_conflict_get");
    if (ncf_array == NULL) {
	return BCM_E_MEMORY;
    }
    _bcm_compat530in_l2_addr_add_t(addr, &naddr);
    rv = bcm_l2_conflict_get(unit, &naddr, ncf_array, cf_max, cf_count);
    if (rv >= 0) {
	for (i = 0; i < *cf_count; i++) {
	    _bcm_compat530out_l2_addr_add_t(&ncf_array[i], &cf_array[i]);
	}
    }
    sal_free(ncf_array);
    return rv;
}

int
bcm_compat530_l2_addr_get(
    int unit,
    bcm_mac_t mac_addr,
    bcm_vlan_t vid,
    bcm_compat530_l2_addr_t *l2addr)
{
    int			rv;
    bcm_l2_addr_t	nl2addr;

    if (l2addr == NULL || mac_addr == NULL) {
        return BCM_E_PARAM;
    }
    bcm_l2_addr_t_init(&nl2addr, mac_addr, vid);
    rv = bcm_l2_addr_get(unit, mac_addr, vid, &nl2addr);
    if (rv >= 0) {
        _bcm_compat530out_l2_addr_add_t(&nl2addr, l2addr);
    }
    return rv;
}


#ifdef INCLUDE_L3

/* bcm_tunnel_initiator_t conversion functions */
STATIC void
_bcm_compat530in_l3_tunnel_initiator_t(
    bcm_compat530_l3_tunnel_initiator_t *from,
    bcm_tunnel_initiator_t *to)
{
    to->flags = from->l3t_flags;
    to->type = from->l3t_type;
    to->ttl = from->l3t_ttl;
    sal_memcpy(to->dmac, from->l3t_dmac, sizeof(bcm_mac_t));
    if(from->l3t_ip4_df_sel) {
        if(from->l3t_ip4_df_sel == 1) {
            to->flags |= BCM_TUNNEL_INIT_IPV4_SET_DF;
        } else {
            to->flags |= BCM_TUNNEL_INIT_USE_INNER_DF;
        }
    }
    if(from->l3t_ip6_df_sel) {
        to->flags |= BCM_TUNNEL_INIT_IPV6_SET_DF;
    }
    to->dip = from->l3t_dip;
    to->sip = from->l3t_sip;
    to->dscp_sel = from->l3t_dscp_sel;
    to->dscp = from->l3t_dscp;
    to->dscp_map = 0;
}

STATIC void
_bcm_compat530out_l3_tunnel_initiator_t(
    bcm_tunnel_initiator_t *from,
    bcm_compat530_l3_tunnel_initiator_t *to)
{
    to->l3t_flags = from->flags;
    to->l3t_type = from->type;
    to->l3t_ttl = from->ttl;
    sal_memcpy(to->l3t_dmac, from->dmac, sizeof(bcm_mac_t));
    if(from->flags & BCM_TUNNEL_INIT_USE_INNER_DF) {
       to->l3t_ip4_df_sel = 0x2;
    }
    if(from->flags & BCM_TUNNEL_INIT_IPV4_SET_DF) {
       to->l3t_ip4_df_sel = 0x1;
    }
    if(from->flags & BCM_TUNNEL_INIT_IPV6_SET_DF) {
       to->l3t_ip6_df_sel = 0x1;
    }
    to->l3t_dip = from->dip;
    to->l3t_sip = from->sip;
    to->l3t_dscp_sel = from->dscp_sel;
    to->l3t_dscp = from->dscp;
    /* ignore from->l3t_dscp_map */
}

/* bcm_tunnel_terminator_t conversion functions */
STATIC void
_bcm_compat530in_l3_tunnel_terminator_t(
    bcm_compat530_l3_tunnel_terminator_t *from,
    bcm_tunnel_terminator_t *to)
{
    to->flags = from->l3t_flags;
    to->vrf = 0;
    to->sip = from->l3t_sip;
    to->dip = from->l3t_dip;
    to->sip_mask = from->l3t_sip_mask;
    to->dip_mask = from->l3t_dip_mask;
    to->udp_dst_port = from->l3t_udp_dst_port;
    to->udp_src_port = from->l3t_udp_src_port;
    to->type = from->l3t_type;
    to->pbmp = from->l3t_pbmp;
    to->vlan = from->l3t_vlan;
}

STATIC void
_bcm_compat530out_l3_tunnel_terminator_t(
    bcm_tunnel_terminator_t *from,
    bcm_compat530_l3_tunnel_terminator_t *to)
{
    to->l3t_flags = from->flags;
    /* ignore from->vrf */
    to->l3t_sip = from->sip;
    to->l3t_dip = from->dip;
    to->l3t_sip_mask = from->sip_mask;
    to->l3t_dip_mask = from->dip_mask;
    to->l3t_udp_dst_port = from->udp_dst_port;
    to->l3t_udp_src_port = from->udp_src_port;
    to->l3t_type = from->type;
    to->l3t_pbmp = from->pbmp;
    to->l3t_vlan = from->vlan;
}

int
bcm_compat530_l3_tunnel_initiator_set(
    int unit,
    bcm_l3_intf_t *intf,
    bcm_compat530_l3_tunnel_initiator_t *tunnel)
{
    int		rv;
    bcm_tunnel_initiator_t	ntunnel;

    if (tunnel == NULL) {
        return BCM_E_PARAM;
    }
    _bcm_compat530in_l3_tunnel_initiator_t(tunnel, &ntunnel);
    rv = bcm_tunnel_initiator_set(unit, intf, &ntunnel);
    return rv;
}

int
bcm_compat530_l3_tunnel_initiator_get(
    int unit,
    bcm_l3_intf_t *intf,
    bcm_compat530_l3_tunnel_initiator_t *tunnel)
{
    int		rv;
    bcm_tunnel_initiator_t	ntunnel;

    if (tunnel == NULL) {
        return BCM_E_PARAM;
    }
    rv = bcm_tunnel_initiator_get(unit, intf, &ntunnel);
    if (rv >= 0) {
	_bcm_compat530out_l3_tunnel_initiator_t(&ntunnel, tunnel);
    }
    return rv;
}

int
bcm_compat530_l3_tunnel_terminator_add(
    int unit,
    bcm_compat530_l3_tunnel_terminator_t *info)
{
    int		rv;
    bcm_tunnel_terminator_t	ninfo;

    if (info == NULL) {
        return BCM_E_PARAM;
    }
    _bcm_compat530in_l3_tunnel_terminator_t(info, &ninfo);
    rv = bcm_tunnel_terminator_add(unit, &ninfo);
    return rv;
}

int
bcm_compat530_l3_tunnel_terminator_delete(
    int unit,
    bcm_compat530_l3_tunnel_terminator_t *info)
{
    int		rv;
    bcm_tunnel_terminator_t	ninfo;

    if (info == NULL) {
        return BCM_E_PARAM;
    }
    _bcm_compat530in_l3_tunnel_terminator_t(info, &ninfo);
    rv = bcm_tunnel_terminator_delete(unit, &ninfo);
    return rv;
}

int
bcm_compat530_l3_tunnel_terminator_update(
    int unit,
    bcm_compat530_l3_tunnel_terminator_t *info)
{
    int		rv;
    bcm_tunnel_terminator_t	ninfo;

    if (info == NULL) {
        return BCM_E_PARAM;
    }
    _bcm_compat530in_l3_tunnel_terminator_t(info, &ninfo);
    rv = bcm_tunnel_terminator_update(unit, &ninfo);
    return rv;
}

int
bcm_compat530_l3_tunnel_terminator_get(
    int unit,
    bcm_compat530_l3_tunnel_terminator_t *info)
{
    int		rv;
    bcm_tunnel_terminator_t	ninfo;

    if (info == NULL) {
        return BCM_E_PARAM;
    }
    _bcm_compat530in_l3_tunnel_terminator_t(info, &ninfo);
    rv = bcm_tunnel_terminator_get(unit, &ninfo);
    if (rv >= 0) {
	_bcm_compat530out_l3_tunnel_terminator_t(&ninfo, info);
    }
    return rv;
}

#endif	/* INCLUDE_L3 */

#endif	/* BCM_RPC_SUPPORT */
