/*
 * $Id: wlan.c 1.1 Broadcom SDK $
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
 * All Rights Reserved.$
 *
 * WLAN module
 */
#ifdef INCLUDE_L3

#include <sal/core/libc.h>

#include <soc/defs.h>
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
#include <bcm/wlan.h>
#include <bcm/stack.h>
#include <bcm/topo.h>

/* Initialize the WLAN module. */
int 
bcm_robo_wlan_init(int unit)
{
    int rv = BCM_E_UNAVAIL;
    return rv;
}

/* Detach the WLAN module. */
int 
bcm_robo_wlan_detach(int unit)
{
    int rv = BCM_E_UNAVAIL;
    return rv;
}

/* Add a WLAN client to the database. */
int 
bcm_robo_wlan_client_add(int unit, bcm_wlan_client_t *info)
{
    int rv = BCM_E_UNAVAIL;
    return rv;
}

/* Delete a WLAN client from the database. */
int 
bcm_robo_wlan_client_delete(int unit, bcm_mac_t mac)
{
    int rv = BCM_E_UNAVAIL;
    return rv;
}


/* Delete all WLAN clients. */
int 
bcm_robo_wlan_client_delete_all(int unit)
{
    int rv = BCM_E_UNAVAIL;
    return rv;
}

/* Get a WLAN client by MAC. */
int 
bcm_robo_wlan_client_get(int unit, bcm_mac_t mac, bcm_wlan_client_t *info)
{
    int rv = BCM_E_UNAVAIL;
    return rv;
}

/* bcm_wlan_client_traverse */
int 
bcm_robo_wlan_client_traverse(int unit, bcm_wlan_client_traverse_cb cb, 
                             void *user_data)
{
    return BCM_E_UNAVAIL;
}

/* bcm_wlan_port_add */
int 
bcm_robo_wlan_port_add(int unit, bcm_wlan_port_t *info)
{
    int rv = BCM_E_UNAVAIL;
    return rv;
}

/* bcm_wlan_port_delete */
int 
bcm_robo_wlan_port_delete(int unit, bcm_gport_t wlan_port_id)
{
    int rv = BCM_E_UNAVAIL;
    return rv;
}

/* bcm_wlan_port_delete_all */
int 
bcm_robo_wlan_port_delete_all(int unit)
{
    int rv = BCM_E_UNAVAIL;
    return rv;
}

/* bcm_wlan_port_get */
int 
bcm_robo_wlan_port_get(int unit, bcm_gport_t wlan_port_id, bcm_wlan_port_t *info)
{
    int rv = BCM_E_UNAVAIL;
    return rv;
}

/* bcm_wlan_port_traverse */
int 
bcm_robo_wlan_port_traverse(int unit, bcm_wlan_port_traverse_cb cb, 
                           void *user_data)
{
    return BCM_E_UNAVAIL;
}

/* WLAN tunnel initiator create */
int
bcm_robo_wlan_tunnel_initiator_create(int unit, bcm_tunnel_initiator_t *info)
{
    return BCM_E_UNAVAIL;
}

/* WLAN tunnel initiator destroy */
int
bcm_robo_wlan_tunnel_initiator_destroy(int unit, bcm_gport_t wlan_tunnel_id)
{
    return BCM_E_UNAVAIL;
}

/* WLAN tunnel initiator get */
int
bcm_robo_wlan_tunnel_initiator_get(int unit, bcm_tunnel_initiator_t *info)
{
    return BCM_E_UNAVAIL;
}

#endif
