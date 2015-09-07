/*
 * $Id: link.c 1.23 Broadcom SDK $
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
 * File:        link.c
 * Purpose:
 * Requires:
 */

#include <shared/alloc.h>
#include <sal/core/sync.h>

#include <bcm/types.h>
#include <bcm/port.h>
#include <bcm/link.h>

#include <bcmx/bcmx.h>
#include <bcmx/link.h>

#include "bcmx_int.h"

#define BCMX_LINK_SET_ERROR_CHECK(_unit, _check, _rv)    \
    BCMX_SET_ERROR_CHECK(_unit, _check, _rv)

#define BCMX_LINK_GET_IS_VALID(_unit, _rv)    \
    BCMX_ERROR_IS_VALID(_unit, _rv)

typedef struct lsx_handler_s {
    struct lsx_handler_s         *lh_next;
    bcmx_link_notify_f            lh_f;
} lsx_handler_t;

static sal_mutex_t         lsx_lock;     /* Synchronization */
static lsx_handler_t      *lsx_handler; /* List of handlers */

/* lsx_interval < 0 implies it hasn't been set yet */
static int                 lsx_interval = -1;   /* STATUS ONLY */

#define LSX_CHECK_INIT    do {                 \
    BCMX_READY_CHECK;                          \
    if (lsx_lock == NULL)                      \
        BCM_IF_ERROR_RETURN(_bcmx_lsx_init()); \
    } while (0)

#define LSX_LOCK sal_mutex_take(lsx_lock, sal_mutex_FOREVER)
#define LSX_UNLOCK sal_mutex_give(lsx_lock)

STATIC int
_bcmx_lsx_init(void)
{
    lsx_lock = sal_mutex_create("bcmx_l2_notify");
    if (lsx_lock == NULL) {
        return BCM_E_MEMORY;
    }

    return BCM_E_NONE;
}


/* Callback registered with BCM layer */
STATIC void
bcmx_bcm_ls_cb(int unit, bcm_port_t port, bcm_port_info_t *info)
{
    bcmx_lport_t lport;
    lsx_handler_t *cb; /* List of handlers */

    BCMX_DEBUG(BCMX_DBG_LINK | BCMX_DBG_VERBOSE,
               ("BCMX LS callback, u %d. p %d\n", unit, port));

    if (lsx_lock == NULL || lsx_handler == NULL) {
        return;
    }

    /* Translate unit/port into lport */
    if (BCM_FAILURE(_bcmx_dest_from_unit_port(&lport, unit, port,
                                              BCMX_DEST_CONVERT_DEFAULT))) {
        return;
    }

    LSX_LOCK;
    cb = lsx_handler;
    while (cb != NULL) {
        cb->lh_f(lport, info);
        cb = cb->lh_next;
    }
    LSX_UNLOCK;
}

/* Make connection to BCM layer for all known units. */

STATIC int
_bcmx_linkscan_rlink_register(void)
{
    int bcm_unit, i;
    int rv = BCM_E_UNAVAIL, tmp_rv;

    BCMX_UNIT_ITER(bcm_unit, i) {
        tmp_rv = bcm_linkscan_register(bcm_unit, bcmx_bcm_ls_cb);
        BCM_IF_ERROR_RETURN(BCMX_LINK_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
    }

    return rv;
}

/*
 * Function:
 *      bcmx_linkscan_register/unregister
 * Purpose:
 *      Register/unregister a BCMX level link change notification callback
 * Parameters:
 *      ln_cb          - The callback function pointer
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      Register may be called multiple times without burning more
 *      registration entries.  In addition, it always calls down to
 *      the lower layer to make sure registration exists on all
 *      currently known units.  In this way, after more units are
 *      added, call this again to ensure connections.
 */

int
bcmx_linkscan_register(bcmx_link_notify_f ln_cb)
{
    lsx_handler_t *cb, *last;
    int registered = FALSE;  /* Successfully registered locally? */
    int rv = BCM_E_NONE;

    LSX_CHECK_INIT;

    BCMX_DEBUG(BCMX_DBG_LINK | BCMX_DBG_VERBOSE, ("BCMX LS reg, %p\n", ln_cb));

    LSX_LOCK;
    last = NULL;
    cb = lsx_handler;
    while (cb != NULL) {
        if (cb->lh_f == ln_cb) {
            registered = TRUE;
            break;
        }
        last = cb;
        cb = cb->lh_next;
    }

    if (!registered) { /* Not found yet; add to list */
        cb = sal_alloc(sizeof(lsx_handler_t), "bcmx_ls_reg");
        if (cb != NULL) {

            /* Put into end of list */
            cb->lh_f = ln_cb;
            cb->lh_next = NULL;
            if (last != NULL) {
                last->lh_next = cb;
            } else {  /* List currently empty */
                lsx_handler = cb;
            }
            registered = TRUE;
        } else {
            rv = BCM_E_MEMORY;
        }
    }

    LSX_UNLOCK;

    if (registered) { /* Make sure registered with lower layer */
        _bcmx_linkscan_rlink_register();
    }

    return rv;
}


int
bcmx_linkscan_unregister(bcmx_link_notify_f ln_cb)
{
    lsx_handler_t *cb, *prev = NULL;

    LSX_CHECK_INIT;

    BCMX_DEBUG(BCMX_DBG_LINK | BCMX_DBG_VERBOSE, ("BCMX LS unreg, %p\n", ln_cb));

    LSX_LOCK;
    cb = lsx_handler;
    while (cb != NULL) {
        if (cb->lh_f == ln_cb) {
            if (prev != NULL) {
                prev->lh_next = cb->lh_next;
            } else {
                lsx_handler = cb->lh_next;
            }
            sal_free(cb);
            break;
        }
        prev = cb;
        cb = cb->lh_next;
    }
    LSX_UNLOCK;

    return BCM_E_NONE;
}


/*
 * Function:
 *      bcmx_linkscan_enable_set
 * Purpose:
 *      Set linkscan state
 * Parameters:
 *      us              - Interval in microseconds; see notes
 * Returns:
 *      BCM_E_XXX
 * Notes:
 *      If us == 0, disables linkscan at lowest layer.  Note that
 *      registrations may still exist in this case.
 *
 *      If us < 0, then a sync is performed by looking for a unit
 *      which returns success; this value is then used to set all
 *      other devices.
 *
 *      If no device found indicating current setting, _E_FAIL
 *      is returned.
 */

int
bcmx_linkscan_enable_set(int us)
{
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int found = FALSE;
    int interval;
    int bcm_unit, i;

    LSX_CHECK_INIT;

    BCMX_DEBUG(BCMX_DBG_LINK | BCMX_DBG_VERBOSE, ("BCMX LS enable set, %d\n", us));

    if (us < 0) {  /* Sync:  See notes above */

        /* Try to get current enable setting */
        BCMX_UNIT_ITER(bcm_unit, i) {
            tmp_rv = bcm_linkscan_enable_get(bcm_unit, &interval);
            if (BCMX_LINK_GET_IS_VALID(bcm_unit, tmp_rv)) {
                rv = tmp_rv;
                if (BCM_SUCCESS(tmp_rv)) {
                    found = TRUE;
                }
                break;
            }
        }

        if (found) {  /* Sync everyone to this setting */
            lsx_interval = interval;
            BCMX_UNIT_ITER(bcm_unit, i) {
                tmp_rv = bcm_linkscan_register(bcm_unit, bcmx_bcm_ls_cb);
                BCM_IF_ERROR_RETURN
                    (BCMX_LINK_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
            }
        } else {
            return BCM_E_FAIL;  /* Could not synchronize */
        }

    } else { /* Just set on all current units */

        if (us > 0) { /* Enabling:  Ensure registered */
            (void)_bcmx_linkscan_rlink_register();
        }
        lsx_interval = us;
        BCMX_UNIT_ITER(bcm_unit, i) {
            tmp_rv = bcm_linkscan_enable_set(bcm_unit, us);
            BCM_IF_ERROR_RETURN(BCMX_LINK_SET_ERROR_CHECK(bcm_unit, tmp_rv, &rv));
        }
    }

    return rv;
}


/*
 * Function:
 *      bcmx_linkscan_enable_get
 * Purpose:
 *      Get linkscan state
 * Parameters:
 *      us              - (OUT) Current interval in microseconds
 *      consistent      - (OUT) If non-NULL, check for consistency
 *                        on all BCMX units and return Boolean value here.
 * Returns:
 *      BCM_E_XXX
 */

int
bcmx_linkscan_enable_get(int *us, int *consistent)
{
    int bcm_unit, i;
    int rv = BCM_E_UNAVAIL, tmp_rv;
    int tmp_us;

    LSX_CHECK_INIT;


    if (lsx_interval >= 0) { /* Has been initialized */
        *us = lsx_interval;
    } else {  /* Look for a unit with info */
        BCMX_UNIT_ITER(bcm_unit, i) {
            tmp_rv = bcm_linkscan_enable_get(bcm_unit, &tmp_us);
            if (BCMX_LINK_GET_IS_VALID(bcm_unit, tmp_rv)) {
                rv = tmp_rv;
                if (BCM_SUCCESS(tmp_rv)) {
                    *us = lsx_interval = tmp_us;
                }
                break;
            }
        }
    }

    if (lsx_interval < 0) { /* Still unitialized */
        return rv;
    }

    if (consistent != NULL) { /* Check for consistency across units */
        *consistent = TRUE;

        BCMX_UNIT_ITER(bcm_unit, i) {
            tmp_rv = bcm_linkscan_enable_get(bcm_unit, &tmp_us);
            if (BCMX_LINK_GET_IS_VALID(bcm_unit, tmp_rv)) {
                rv = tmp_rv;
                if (BCM_SUCCESS(tmp_rv)) {
                    if (tmp_us != lsx_interval) {
                        *consistent = FALSE;
                        break;
                    }
                } else {
                    break;
                }
            }
        }
    }

    return rv;
}


/*
 * Function:
 *     bcmx_linkscan_enable_port_get
 * Purpose:
 *     Determine whether or not linkscan is managing a given port
 * Parameters:
 *     lport - Logical port to check.
 * Returns:
 *     BCM_E_NONE - port being scanned
 *     BCM_E_PARAM - invalid logical port
 *     BCM_E_DISABLED - port not being scanned
 */

int
bcmx_linkscan_enable_port_get(bcmx_lport_t lport)
{
    int         bcm_unit;
    bcm_port_t  bcm_port;

    LSX_CHECK_INIT;

    if (BCM_FAILURE
        (_bcmx_dest_to_unit_port(lport, &bcm_unit, &bcm_port,
                                 BCMX_DEST_CONVERT_DEFAULT))) {
        return BCM_E_PARAM;
    }

    return bcm_linkscan_enable_port_get(bcm_unit, bcm_port);
}

/*
 * Function:
 *      bcmx_linkscan_device_add
 * Purpose:
 *      Add a unit to linkscanning
 * Notes:
 *      Registers linkscan callback
 */

int
bcmx_linkscan_device_add(int bcm_unit)
{
    int rv;

    LSX_CHECK_INIT;

    if (lsx_interval >= 0) { /* Has been initialized */
        rv = bcm_linkscan_enable_set(bcm_unit, lsx_interval);
        if (rv < 0) {
            BCMX_DEBUG(BCMX_DBG_WARN, ("WARN: BCMX LS dev add enable, %d: %s\n",
                        rv, bcm_errmsg(rv)));
        }
    }

    /* Always register if possible */
    rv = bcm_linkscan_register(bcm_unit, bcmx_bcm_ls_cb);

    if (rv < 0) {
        BCMX_DEBUG(BCMX_DBG_WARN, ("WARN: BCMX LS device add reg, %d: %s\n",
                    rv, bcm_errmsg(rv)));
    }

    return rv;
}

/*
 * Function:
 *      bcmx_linkscan_device_remove
 * Purpose:
 *      Remove a unit from linkscanning
 */

int
bcmx_linkscan_device_remove(int bcm_unit)
{
    LSX_CHECK_INIT;

    return bcm_linkscan_unregister(bcm_unit, bcmx_bcm_ls_cb);
}



#if defined(BROADCOM_DEBUG)
void
bcmx_linkscan_dump(void)
{
    lsx_handler_t *ent;

    if (lsx_lock == NULL) {
        BCMX_DEBUG(BCMX_DBG_NORMAL, ("BCMX linkscan not initialized\n"));
        return;
    }

    BCMX_DEBUG(BCMX_DBG_NORMAL,
               ("BCMX linkscan interval %d%s\n", lsx_interval,
                lsx_interval < 0 ? " (un-init)" : ""));

    BCMX_DEBUG(BCMX_DBG_NORMAL, ("LSX handler list:  \n"));
    for (ent = lsx_handler; ent != NULL; ent = ent->lh_next) {
        BCMX_DEBUG(BCMX_DBG_NORMAL, ("    Fn %p.\n", ent->lh_f));
    }

}

void
bcmx_rlink_dump(void)
{
#ifdef  BCM_RPC_SUPPORT
    extern void bcm_rlink_dump(void);

    bcm_rlink_dump();
#endif  /* BCM_RPC_SUPPORT */
    bcmx_linkscan_dump();
}
#endif /* BROADCOM_DEBUG */
