/*
 * $Id: custom.c 1.15 Broadcom SDK $
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
 * Custom API callouts
 */

#include <bcm/types.h>

#include <bcmx/custom.h>
#include <bcmx/lport.h>
#include <bcmx/bcmx.h>

#include "bcmx_int.h"

#define BCMX_CUSTOM_INIT_CHECK    BCMX_READY_CHECK

#define BCMX_CUSTOM_SET_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_SET_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_CUSTOM_GET_IS_VALID(_unit, _rv)    \
    BCMX_ERROR_IS_VALID(_unit, _rv)

/*
 * Notes:
 *     If 'port' is a physical port, apply to device where port resides.
 *     If 'port' is a virtual port, apply to all devices.
 */
int
bcmx_custom_port_set(bcmx_lport_t port, int type, int len, uint32 *args)
{
    int         rv = BCM_E_UNAVAIL, tmp_rv;
    int         i, bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_CUSTOM_INIT_CHECK;

    BCMX_LPORT_CHECK(port);

    if (BCMX_LPORT_IS_PHYSICAL(port)) {
        /*
         * Convert to old bcm port format to ensure application
         * custom routines continue to work.
         */
        if (BCM_FAILURE(_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                                BCMX_DEST_CONVERT_NON_GPORT))) {
            return BCM_E_PORT;
        }
        return bcm_custom_port_set(bcm_unit, bcm_port, type, len, args);
    }

    /* Virtual port */
    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_custom_port_set(bcm_unit, port, type, len, args);
        BCM_IF_ERROR_RETURN
            (BCMX_CUSTOM_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

int
bcmx_custom_port_get(bcmx_lport_t port, int type, int max_len,
                     uint32 *args, int *actual_len)
{
    int         rv;
    int         i, bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_CUSTOM_INIT_CHECK;

    BCMX_LPORT_CHECK(port);

    if (BCMX_LPORT_IS_PHYSICAL(port)) {
        /*
         * Convert to old bcm port format to ensure application
         * custom routines continue to work.
         */
        if (BCM_FAILURE(_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                                BCMX_DEST_CONVERT_NON_GPORT))) {
            return BCM_E_PORT;
        }
        return bcm_custom_port_get(bcm_unit, bcm_port, type,
                                   max_len, args, actual_len);
    }

    /* Virtual port */
    BCMX_UNIT_ITER(bcm_unit, i) {
        rv = bcm_custom_port_get(bcm_unit, port, type, max_len, args, actual_len);
        if (BCMX_CUSTOM_GET_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}

int
bcmx_custom_unit_set(int type, int len, uint32 *args)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_CUSTOM_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_custom_port_set(bcm_unit, -1, type, len, args);
        BCM_IF_ERROR_RETURN
            (BCMX_CUSTOM_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

int
bcmx_custom_unit_get(int type, int max_len, uint32 *args, int *actual_len)
{
    int rv = BCM_E_UNAVAIL;
    int i, bcm_unit;

    BCMX_CUSTOM_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        rv = bcm_custom_port_get(bcm_unit, -1, type, max_len, args, actual_len);
        if (BCMX_CUSTOM_GET_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}
