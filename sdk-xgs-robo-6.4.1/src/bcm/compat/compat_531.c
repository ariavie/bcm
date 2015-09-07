/*
 * $Id: compat_531.c,v 1.4 Broadcom SDK $
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

#include <bcm_int/compat/compat_531.h>

/* bcm_l2_addr_t conversion functions */
STATIC void
_bcm_compat531in_l2_cache_addr_t(
    bcm_compat531_l2_cache_addr_t *from,
    bcm_l2_cache_addr_t *to)
{
    to->flags = from->flags;
    sal_memcpy(to->mac, from->mac, sizeof(bcm_mac_t));
    sal_memcpy(to->mac_mask, from->mac_mask, sizeof(bcm_mac_t));
    to->vlan = from->vlan;
    to->vlan_mask = from->vlan_mask;
    to->src_port = from->src_port;
    to->src_port_mask = from->src_port_mask;
    to->dest_modid = from->dest_modid;
    to->dest_port = from->dest_port;
    to->dest_trunk = from->dest_trunk;
    to->prio = from->prio;
    BCM_PBMP_CLEAR(to->dest_ports);
}

STATIC void
_bcm_compat531out_l2_cache_addr_t(
    bcm_l2_cache_addr_t *from,
    bcm_compat531_l2_cache_addr_t *to)
{
    to->flags = from->flags;
    sal_memcpy(to->mac, from->mac, sizeof(bcm_mac_t));
    sal_memcpy(to->mac_mask, from->mac_mask, sizeof(bcm_mac_t));
    to->vlan = from->vlan;
    to->vlan_mask = from->vlan_mask;
    to->src_port = from->src_port;
    to->src_port_mask = from->src_port_mask;
    to->dest_modid = from->dest_modid;
    to->dest_port = from->dest_port;
    to->dest_trunk = from->dest_trunk;
    to->prio = from->prio;
    /* ignore from->dest_ports */
}

int
bcm_compat531_l2_cache_set(
    int unit,
    int index, 
    bcm_compat531_l2_cache_addr_t *addr,
    int *index_used)
{
    int			rv;
    bcm_l2_cache_addr_t	n_addr;

    if (addr == NULL) {
        return BCM_E_PARAM;
    }
    _bcm_compat531in_l2_cache_addr_t(addr, &n_addr);
    rv = bcm_l2_cache_set(unit, index, &n_addr, index_used);
    return rv;
}

int
bcm_compat531_l2_cache_get(
    int unit,
    int index,
    bcm_compat531_l2_cache_addr_t *addr)
{
    int			rv;
    bcm_l2_cache_addr_t	n_addr;

    if (addr == NULL) {
        return BCM_E_PARAM;
    }
    rv = bcm_l2_cache_get(unit, index, &n_addr);
    if (rv >= 0) {
	_bcm_compat531out_l2_cache_addr_t(&n_addr, addr);
    }
    return rv;
}

#endif	/* BCM_RPC_SUPPORT */
