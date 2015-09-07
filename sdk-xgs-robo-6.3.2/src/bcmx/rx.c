/*
 * $Id: rx.c 1.28 Broadcom SDK $
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
 * File:        rx.c
 * Purpose:     BCMX RX API implementation
 * Requires:
 *
 */

#include <shared/alloc.h>
#include <sal/core/libc.h>
#include <bcm/pkt.h>
#include <bcm/rx.h>
#include <bcm/error.h>


#include <bcm_int/control.h>
#include <bcm_int/common/rx.h>

#include <bcmx/rx.h>
#include <bcmx/bcmx.h>
#include <bcmx/debug.h>

#include "bcmx_int.h"

#define BCMX_RX_INIT_CHECK    BCMX_READY_CHECK

#define BCMX_RX_SET_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_SET_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_RX_DELETE_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_DELETE_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_RX_GET_IS_VALID(_unit, _rv)    \
    BCMX_ERROR_IS_VALID(_unit, _rv)


/* Exposed to BCMX internals */

int _bcmx_rx_running = FALSE;

static rx_callout_t *bcmx_rco;


/*
 * Function:
 *      bcmx_rx_running
 * Purpose:
 *      Return status of whether BCMX RX is running
 * Returns:
 *      TRUE if BCMX RX is running
 * Notes:
 *      The _bcmx_rx_running variable is exported and visible.  That
 *      can be checked as easily.
 */

int
bcmx_rx_running(void) {
    return _bcmx_rx_running;
}

STATIC INLINE rx_callout_t *
bcmx_rco_find(bcm_rx_cb_f fn, int priority, rx_callout_t **prev)
{
    rx_callout_t *rco = bcmx_rco;

    *prev = NULL;
    while (rco != NULL) {
        if (rco->rco_function == fn && rco->rco_priority == priority) {
            return rco;
        }
        if (rco->rco_priority < priority) {
            return NULL;
        }
        *prev = rco;
        rco = (rx_callout_t *)rco->rco_next;
    }

    return NULL;
}


/*
 * Function:
 *      bcmx_rx_register
 * Purpose:
 *      Register with all BCM units to receive packets
 * Parameters:
 *      name         - char string identifier of callout
 *      fn           - Callout function
 *      priority     - Of callout
 *      cookie       - Passed on callback
 *      flags        - See BCM_RCO_F_...
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Handlers registering in interrupt context will not receive
 *      tunnelled packets.
 *
 *      The callback is registered with each available unit.
 *
 *      The callbacks have to be maintained locally since addition/
 *      removal of units results in making subsequent BCM layer
 *      register/unregister calls.
 */

int
bcmx_rx_register(const char *name,
                 bcm_rx_cb_f fn,
                 int priority,
                 void *cookie,
                 uint32 flags)
{
    int unit, i;
    int rv = BCM_E_NONE, tmp_rv;
    rx_callout_t *rco, *prev, *next;

    BCMX_RX_INIT_CHECK;

#if defined(BROADCOM_DEBUG)
    if (flags & BCM_RCO_F_INTR) {
        BCMX_DEBUG(BCMX_DBG_WARN,
		   ("BCMX RX: Intr RX handler will "
		    "not receive tunnelled pkts: %s\n", name));
    }
#endif  /* BROADCOM_DEBUG */

    BCMX_CONFIG_LOCK;
    rco = bcmx_rco_find(fn, priority, &prev);
    if (rco != NULL) {
        BCMX_CONFIG_UNLOCK;
        return BCM_E_NONE;
    }

    rco = sal_alloc(sizeof(rx_callout_t), "bcmx_rx_reg");
    if (rco == NULL) {
        BCMX_CONFIG_UNLOCK;
        return BCM_E_MEMORY;
    }

    if (prev == NULL) {
        next = bcmx_rco;
        bcmx_rco = rco;
    } else {
        next = (rx_callout_t *)prev->rco_next;
        prev->rco_next = rco;
    }
    SETUP_RCO(rco, name, fn, priority, cookie, next, flags);
    for (i = 0; i < 16; i++) {
        if ((flags & BCM_RCO_F_COS_ACCEPT_MASK) & BCM_RCO_F_COS_ACCEPT(i)) {
            SETUP_RCO_COS_SET(rco, i);
        }
    }
    BCMX_CONFIG_UNLOCK;

    if (_bcmx_rx_running) {
        rv = BCM_E_UNAVAIL;
        BCMX_UNIT_ITER(unit, i) {
            tmp_rv = bcm_rx_register(unit, name, fn, priority, cookie, flags);
            BCM_IF_ERROR_RETURN(BCMX_RX_SET_ERROR_CHECK(unit, tmp_rv, &rv));
        }
    }

    return rv;
}


/*
 * Function:
 *      bcmx_rx_unregister
 * Purpose:
 *      Unregister the RX callback from all BCM units
 * Parameters:
 *      fn           - Callout function
 *      priority     - Of callout
 * Returns:
 *      BCM_E_XXX
 */

int
bcmx_rx_unregister(bcm_rx_cb_f fn, int priority)
{
    int unit, i;
    int rv = BCM_E_UNAVAIL, tmp_rv;
    rx_callout_t *rco, *prev;

    BCMX_RX_INIT_CHECK;

    BCMX_CONFIG_LOCK;
    rco = bcmx_rco_find(fn, priority, &prev);
    if (rco == NULL) {
        BCMX_CONFIG_UNLOCK;
        return BCM_E_NONE;
    }

    if (prev == NULL) {
        bcmx_rco = (rx_callout_t *)rco->rco_next;
    } else {
        prev->rco_next = rco->rco_next;
    }

    sal_free(rco);
    BCMX_CONFIG_UNLOCK;

    /* Now take care of any known remote units */
    BCMX_UNIT_ITER(unit, i) {
        tmp_rv = bcm_rx_unregister(unit, fn, priority);
        BCM_IF_ERROR_RETURN(BCMX_RX_DELETE_ERROR_CHECK(unit, tmp_rv, &rv));
    }

    return rv;
}



/*
 * Function:
 *      bcmx_rx_start
 * Purpose:
 *      Start BCMX RX packet handling
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Calls "add" on all the current units.
 */

int
bcmx_rx_start(void)
{
    int unit, i;
    int rv = BCM_E_UNAVAIL, tmp_rv;

    BCMX_RX_INIT_CHECK;

    if (_bcmx_rx_running) {
        return BCM_E_NONE;
    }

    _bcmx_rx_running = TRUE;

    BCMX_UNIT_ITER(unit, i) {
        tmp_rv = bcmx_rx_device_add(unit);
        if (BCM_FAILURE(BCMX_RX_SET_ERROR_CHECK(unit, tmp_rv, &rv))) {
            BCMX_DEBUG(BCMX_DBG_WARN,
                       ("BCMX RX: Unit %d add failed: %s\n",
                        unit, bcm_errmsg(tmp_rv)));
            break;
        }
    }

    return rv;
}



/*
 * Function:
 *      bcmx_rx_stop
 * Purpose:
 *      Stop BCMX RX packet handling
 * Returns:
 *      BCM_E_XXX
 */

int
bcmx_rx_stop(void)
{
    int unit, i;
    int rv = BCM_E_UNAVAIL, tmp_rv;

    BCMX_RX_INIT_CHECK;

    if (!_bcmx_rx_running) {
        return BCM_E_NONE;
    }

    _bcmx_rx_running = FALSE;

    BCMX_UNIT_ITER(unit, i) {
        tmp_rv = bcmx_rx_device_remove(unit);
        if (BCM_FAILURE(BCMX_RX_DELETE_ERROR_CHECK(unit, tmp_rv, &rv))) {
            BCMX_DEBUG(BCMX_DBG_WARN,
                       ("BCMX RX: Unit %d remove failed: %s\n",
                        unit, bcm_errmsg(tmp_rv)));
            break;
        }
    }

    return rv;
}


/*
 * Function:
 *      bcmx_rx_unit_add
 * Purpose:
 *      Add a BCM unit to BCMX RX registration.
 * Parameters:
 *      unit         - The unit to add
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      May be called when a new BCM unit is added
 */

int
bcmx_rx_device_add(int unit)
{
    rx_callout_t *rco = bcmx_rco;
    int rv = BCM_E_NONE, tmp_rv;

    BCMX_RX_INIT_CHECK;

    if (!BCM_UNIT_VALID(unit)) {
        return BCM_E_PARAM;
    }

    if (_bcmx_rx_running) {
        rv = BCM_E_UNAVAIL;
        while (rco != NULL) {
            tmp_rv = bcm_rx_register(unit,
                                     rco->rco_name,
                                     rco->rco_function,
                                     rco->rco_priority,
                                     rco->rco_cookie,
                                     rco->rco_flags);
            if (BCM_FAILURE(BCMX_RX_SET_ERROR_CHECK(unit, tmp_rv, &rv))) {
                BCMX_DEBUG(BCMX_DBG_WARN,
                           ("BCMX RX: Unit %d register failed: %s\n",
                            unit, bcm_errmsg(tmp_rv)));
            }
            rco = (rx_callout_t *)rco->rco_next;
        }

        /* Start RX */
        tmp_rv = bcm_rx_start(unit, NULL);
        if (tmp_rv == BCM_E_BUSY) {
            tmp_rv = BCM_E_NONE;
        }
        (void) BCMX_RX_SET_ERROR_CHECK(unit, tmp_rv, &rv);
    }

    return rv;
}


/*
 * Function:
 *      bcmx_rx_unit_remove
 * Purpose:
 *      Remove a BCM unit to BCMX RX registration.
 * Parameters:
 *      unit         - The unit to add
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      May be called when a BCM unit is removed
 */

int
bcmx_rx_device_remove(int unit)
{
    rx_callout_t *rco = bcmx_rco;
    int rv = BCM_E_NONE, tmp_rv;

    BCMX_RX_INIT_CHECK;

    if (!BCM_UNIT_VALID(unit)) {
        return BCM_E_PARAM;
    }

    while (rco != NULL) {
        tmp_rv = bcm_rx_unregister(unit,
                                   rco->rco_function,
                                   rco->rco_priority);
        if (BCM_FAILURE(BCMX_RX_DELETE_ERROR_CHECK(unit, tmp_rv, &rv))) {
            BCMX_DEBUG(BCMX_DBG_WARN,
                       ("BCMX RX: Unit %d unregister failed: %s\n",
                        unit, bcm_errmsg(tmp_rv)));
        }

        rco = (rx_callout_t *)rco->rco_next;
    }

    return rv;
}

/*
 * Function:
 *      bcmx_rx_control_set(bcm_rx_control_t type, int value)
 * Description:
 *      Enable/Disable specified RX feature.
 * Parameters:
 *      type - RX control parameter
 *      value - new value of control parameter
 * Return Value:
 *      BCM_E_NONE
 *      BCM_E_UNAVAIL - Functionality not available
 *
 */

int
bcmx_rx_control_set(bcm_rx_control_t type, int arg)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int i, bcm_unit;

    BCMX_RX_INIT_CHECK;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_rx_control_set(bcm_unit, type, arg);
        BCM_IF_ERROR_RETURN(BCMX_RX_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *      bcmx_rx_control_get(bcm_rx_control_t type, int *value)
 * Description:
 *      Get the status of specified RX feature.
 * Parameters:
 *      type - RX control parameter
 *      value - (OUT) Current value of control parameter
 * Return Value:
 *      BCM_E_NONE
 *      BCM_E_UNAVAIL - Functionality not available
 */

int
bcmx_rx_control_get(bcm_rx_control_t type, int *arg)
{
    int rv;
    int i, bcm_unit;

    BCMX_RX_INIT_CHECK;

    BCMX_PARAM_NULL_CHECK(arg);

    BCMX_UNIT_ITER(bcm_unit, i) {
        rv = bcm_rx_control_get(bcm_unit, type, arg);
        if (BCMX_RX_GET_IS_VALID(bcm_unit, rv)) {
            return rv;
        }
    }

    return BCM_E_UNAVAIL;
}

