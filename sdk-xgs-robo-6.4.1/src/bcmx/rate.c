/*
 * $Id: rate.c,v 1.9 Broadcom SDK $
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
 * File:    bcmx/rate.c
 * Purpose: BCMX Packet Rate Control APIs
 */

#include <bcm/types.h>

#include <bcmx/rate.h>
#include <bcmx/lport.h>
#include <bcmx/bcmx.h>

#include "bcmx_int.h"

#define BCMX_RATE_INIT_CHECK    BCMX_READY_CHECK

#define BCMX_RATE_SET_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_SET_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_RATE_GET_IS_VALID(_unit, _rv)    \
    BCMX_ERROR_IS_VALID(_unit, _rv)


/*
 * Function:
 *      bcmx_rate_set
 */

int
bcmx_rate_set(int val, int flags)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_RATE_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_rate_set(bcm_unit, val, flags);
        BCM_IF_ERROR_RETURN(BCMX_RATE_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}


/*
 * Function:
 *      bcmx_rate_get
 */

int
bcmx_rate_get(int *val, int *flags)
{
    int rv;
    int i, bcm_unit;

    BCMX_RATE_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(val);
    BCMX_PARAM_NULL_CHECK(flags);

    BCMX_UNIT_ITER(bcm_unit, i) {
        rv = bcm_rate_get(bcm_unit, val, flags);
        if (BCMX_RATE_GET_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcmx_rate_mcast_set
 */

int
bcmx_rate_mcast_set(int limit, int flags, bcmx_lport_t port)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_RATE_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_rate_mcast_set(bcm_unit,
                              limit,
                              flags,
                              bcm_port);
}


/*
 * Function:
 *      bcmx_rate_bcast_set
 */

int
bcmx_rate_bcast_set(int limit, int flags, bcmx_lport_t port)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_RATE_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_rate_bcast_set(bcm_unit,
                              limit,
                              flags,
                              bcm_port);
}


/*
 * Function:
 *      bcmx_rate_dlfbc_set
 */

int
bcmx_rate_dlfbc_set(int limit, int flags, bcmx_lport_t port)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_RATE_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_rate_dlfbc_set(bcm_unit,
                              limit,
                              flags,
                              bcm_port);
}


/*
 * Function:
 *      bcmx_rate_mcast_get
 */

int
bcmx_rate_mcast_get(int *limit, int *flags, bcmx_lport_t port)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_RATE_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(limit);
    BCMX_PARAM_NULL_CHECK(flags);

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_rate_mcast_get(bcm_unit,
                              limit,
                              flags,
                              bcm_port);
}


/*
 * Function:
 *      bcmx_rate_dlfbc_get
 */

int
bcmx_rate_dlfbc_get(int *limit, int *flags, bcmx_lport_t port)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_RATE_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(limit);
    BCMX_PARAM_NULL_CHECK(flags);

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_rate_dlfbc_get(bcm_unit,
                              limit,
                              flags,
                              bcm_port);
}


/*
 * Function:
 *      bcmx_rate_bcast_get
 */

int
bcmx_rate_bcast_get(int *limit, int *flags, bcmx_lport_t port)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_RATE_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(limit);
    BCMX_PARAM_NULL_CHECK(flags);

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_rate_bcast_get(bcm_unit,
                              limit,
                              flags,
                              bcm_port);
}


/*
 * Function:
 *      bcmx_rate_type_get
 */

int
bcmx_rate_type_get(bcm_rate_limit_t *rl)
{
    int rv;
    int i, bcm_unit;

    BCMX_RATE_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(rl);

    BCMX_UNIT_ITER(bcm_unit, i) {
        rv = bcm_rate_type_get(bcm_unit, rl);
        if (BCMX_RATE_GET_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}


/*
 * Function:
 *      bcmx_rate_type_set
 */

int
bcmx_rate_type_set(bcm_rate_limit_t *rl)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_RATE_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(rl);

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_rate_type_set(bcm_unit, rl);
        BCM_IF_ERROR_RETURN(BCMX_RATE_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *      bcmx_rate_bandwidth_get
 */
int
bcmx_rate_bandwidth_get(bcmx_lport_t port, int flags,
                        uint32 *kbits_sec, uint32 *kbits_burst)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_RATE_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(kbits_sec);
    BCMX_PARAM_NULL_CHECK(kbits_burst);

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_rate_bandwidth_get(bcm_unit, bcm_port,
                                  flags,
                                  kbits_sec,
                                  kbits_burst);
}

/*
 * Function:
 *      bcmx_rate_bandwidth_set
 */
int
bcmx_rate_bandwidth_set(bcmx_lport_t port, int flags,
                        uint32 kbits_sec, uint32 kbits_burst)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    BCMX_RATE_INIT_CHECK;

    BCM_IF_ERROR_RETURN
        (_bcmx_dest_to_unit_port(port, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT));

    return bcm_rate_bandwidth_set(bcm_unit, bcm_port,
                                  flags,
                                  kbits_sec,
                                  kbits_burst);
}
