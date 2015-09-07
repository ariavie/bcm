
/*
 * $Id: mpls.c,v 1.24 Broadcom SDK $
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
 * File:    mpls.c
 * Purpose: Manages common MPLS functions
 */

#ifdef INCLUDE_L3

#include <sal/core/libc.h>

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/l2u.h>
#include <soc/util.h>
#include <soc/debug.h>

#include <bcm/l2.h>
#include <bcm/l3.h>
#include <bcm/port.h>
#include <bcm/error.h>
#include <bcm/vlan.h>
#include <bcm/rate.h>
#include <bcm/ipmc.h>
#include <bcm/mpls.h>
#include <bcm/stack.h>
#include <bcm/topo.h>

/*
 * Function:
 *      bcm_mpls_port_t_init
 * Purpose:
 *      Initialize the MPLS port struct
 * Parameters:
 *      port_info - Pointer to the struct to be init'ed
 */

void
bcm_mpls_port_t_init(bcm_mpls_port_t *port_info)
{
    if (port_info != NULL) {
        sal_memset(port_info, 0, sizeof(*port_info));

        port_info->port = BCM_GPORT_INVALID;
        port_info->match_vlan = BCM_VLAN_INVALID;
        port_info->match_inner_vlan = BCM_VLAN_INVALID;
        port_info->egress_service_vlan = BCM_VLAN_INVALID;
        bcm_mpls_egress_label_t_init(&port_info->egress_label);
    }
    return;
}

/*
 * Function:
 *      bcm_mpls_egress_label_t_init
 * Purpose:
 *      Initialize MPLS egress label object struct.
 * Parameters:
 *      label - Pointer to MPLS egress label object struct.
 */

void
bcm_mpls_egress_label_t_init(bcm_mpls_egress_label_t *label)
{
    if (label != NULL) {
        sal_memset(label, 0, sizeof(*label));
        label->label = BCM_MPLS_LABEL_INVALID;
    }
    return;
}

/*
 * Function:
 *      bcm_mpls_tunnel_switch_t_init
 * Purpose:
 *      Initialize MPLS tunnel switch object struct.
 * Parameters:
 *      switch - Pointer to MPLS tunnel switch object struct.
 */

void
bcm_mpls_tunnel_switch_t_init(bcm_mpls_tunnel_switch_t *info)
{
    if (info != NULL) {
        sal_memset(info, 0, sizeof(*info));
        info->label = BCM_MPLS_LABEL_INVALID;
        bcm_mpls_egress_label_t_init(&info->egress_label);
        info->tunnel_if = BCM_IF_INVALID;
    }
    return;
}

/*
 * Function:
 *      bcm_mpls_exp_map_t_init
 * Purpose:
 *      Initialize MPLS EXP map object struct.
 * Parameters:
 *      exp_map - Pointer to MPLS EXP map object struct.
 */

void
bcm_mpls_exp_map_t_init(bcm_mpls_exp_map_t *exp_map)
{
    if (exp_map != NULL) {
        sal_memset(exp_map, 0, sizeof(*exp_map));
    }
    return;
}


/*
 * Function:
 *      bcm_mpls_vpn_config_t_init
 * Purpose:
 *      Initialize the MPLS VPN config structure
 * Parameters:
 *      info - Pointer to the struct to be init'ed
 */

void
bcm_mpls_vpn_config_t_init(bcm_mpls_vpn_config_t *info)
{
    if (info != NULL) {
        sal_memset(info, 0, sizeof(*info));
    }
    return;
}
#else
int _bcm_mpls_not_empty;
#endif  /* INCLUDE_L3 */
