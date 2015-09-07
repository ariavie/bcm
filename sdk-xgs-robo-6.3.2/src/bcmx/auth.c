/*
 * $Id: auth.c 1.9 Broadcom SDK $
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
 * File:	bcmx/auth.c
 * Purpose:	BCMX 802.1x support
 */

#include <bcm/types.h>

#include <bcmx/auth.h>
#include <bcmx/lport.h>
#include <bcmx/bcmx.h>

#include "bcmx_int.h"

#define BCMX_AUTH_INIT_CHECK    BCMX_READY_CHECK

#define BCMX_AUTH_SET_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_SET_ERROR_CHECK(_unit, _check, _rv)

int
bcmx_auth_mode_set(bcmx_lport_t port, uint32 mode)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_AUTH_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_auth_mode_set(bcm_unit, bcm_port, mode);
}

int
bcmx_auth_mode_get(bcmx_lport_t port, uint32 *mode)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_AUTH_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_auth_mode_get(bcm_unit, bcm_port, mode);
}

/* Currently, only one auth_unauth callback is supported
 * at the BCMX layer.
 */

STATIC bcmx_auth_cb_t ap_auth_cb_fn = NULL;
STATIC void *ap_auth_cookie = NULL;

STATIC void
_bcmx_auth_cb(void *cookie, int unit, int port, int reason)
{
    bcmx_lport_t lport;

    if (BCM_FAILURE(_bcmx_dest_from_unit_port(&lport,
                                              unit, (bcm_port_t)port,
                                              BCMX_DEST_CONVERT_DEFAULT))) {
        return;
    }

    if (ap_auth_cb_fn != NULL) {
        (*ap_auth_cb_fn)(ap_auth_cookie, lport, reason);
    }
}


/*
 * This routine registers through rlink to get remote device's
 * calls.
 */

int
bcmx_auth_unauth_callback(bcmx_auth_cb_t func, void *cookie)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;
    bcm_auth_cb_t cb_f = NULL;

    BCMX_AUTH_INIT_CHECK;

    ap_auth_cb_fn = func;
    ap_auth_cookie = cookie;
    if (func != NULL) {
        /* If ap function is non-NULL, register with BCMX's callback */
        cb_f = _bcmx_auth_cb;
    }

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_auth_unauth_callback(bcm_unit, cb_f, cookie);
        BCM_IF_ERROR_RETURN(BCMX_AUTH_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

int
bcmx_auth_egress_set(bcmx_lport_t port, int enable)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_AUTH_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_auth_egress_set(bcm_unit, bcm_port, enable);
}

int
bcmx_auth_egress_get(bcmx_lport_t port, int *enable)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_AUTH_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_auth_egress_get(bcm_unit, bcm_port, enable);
}

int
bcmx_auth_mac_add(bcmx_lport_t port, bcm_mac_t mac)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_AUTH_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_auth_mac_add(bcm_unit, bcm_port, mac);
}

int
bcmx_auth_mac_delete(bcmx_lport_t port, bcm_mac_t mac)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_AUTH_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_auth_mac_delete(bcm_unit, bcm_port, mac);
}

int
bcmx_auth_mac_delete_all(bcmx_lport_t port)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_AUTH_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_auth_mac_delete_all(bcm_unit, bcm_port);
}
