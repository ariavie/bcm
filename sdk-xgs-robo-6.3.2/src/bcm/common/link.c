/*
 * $Id: link.c 1.41 Broadcom SDK $
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
 * BCM Link Scan Module
 *
 * Linkscan should always run for all chips in a system. It manages
 * the current chip LINK state (EPC_LINK or equivalent), and programs
 * MACs to match auto-negotiated links. 
 *
 * Linkscan is also responsible for determining the link up/down
 * status for a port. Since link down events signaled by most PHYs are
 * latched low and cleared on read, it is important that accesses to
 * to the PHY itself be managed carefully. Linkscan is intended to be
 * the only process that reads the actual PHY (using
 * _bcm_port_link_get). All other calls to retrieve link status
 * results in calls to _bcm_link_get which returns the linkscan view
 * of the link status. This ensures linkscan is the only process that
 * reads the actual PHYs.
 *
 * All modifications to the linkscan state are protected by LS_LOCK.
 *
 * Linkscan maintains the following port bitmaps
 *
 *     pbm_link:
 *                 Current physical link up/down status. When a bit
 *                 in this mask is set, a link up or link down event
 *                 is recognized and signaled to any registered
 *                 handlers.
 *
 *     pbm_link_change:
 *                 Mask of ports that need to recognize a link
 *                 down event. Ports are added to this mask by the
 *                 function bcm_link_change. 
 *
 *     pbm_override_ports:
 *                 Bitmap of ports that are currently
 *                 being explicitly set to a value. These actual value
 *                 is determined by pbm_override_link. Ports are
 *                 added and removed from this mode by the routine
 *                 _bcm_link_force. 
 *
 *                 Ports that are forced to an UP state do NOT result
 *                 in a call to bcm_port_update. It is the
 *                 responsibility of the caller to configure the
 *                 correct MAC and PHY state.
 *
 *     pbm_override_link:
 *                 Bitmap indicating the link-up/link-down
 *                 status for those ports with override set.
 *
 *     pbm_sgmii_autoneg:
 *                 Bitmap of the port that is configured in SGMII
 *                 autoneg mode based on spn_PHY_SGMII_AUTONEG.
 *                 Maintaining this bitmap avoids the overhead
 *                 of the soc_property_port_get call. 
 *
 * Calls to _bcm_link_get always returns the current status as
 * indicated by pbm_link. 
 *
 * NOTE:
 * Original file is src/bcm/esw/link.c, version 1.49.
 * That file should eventually be removed, and XGS specific code
 * should be placed in corresponding routines to be attached to the
 * 'driver' in 'link_control' during module initialization
 * 'soc_linkctrl_init()'.
 */


#include <sal/types.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>

#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/phyctrl.h>
#include <soc/linkctrl.h>

#include <bcm/error.h>
#include <bcm/port.h>

#include <bcm/link.h>

#include <bcm_int/common/lock.h>
#include <bcm_int/common/link.h>

#ifdef BCM_WARM_BOOT_SUPPORT
#include <bcm/module.h>
#endif


#define NUM_PORTS    SOC_MAX_NUM_PORTS

/*
 * Handler to registered callbacks for link changes
 */
typedef struct _ls_handler_cb_s {
    struct _ls_handler_cb_s  *next;
    bcm_linkscan_handler_t   cb_f;
} _ls_handler_cb_t;

/*
 * Linkscan error state
 */
typedef struct _ls_errstate_s {
    int          limit;    /* # errors to enter error state */
    int          delay;    /* Length of error state in seconds */
    int          count;    /* # of errors so far */
    int          wait;     /* Boolean, TRUE when in error state */
    sal_usecs_t  time;     /* Time error state was entered */
} _ls_errstate_t;

/*
 * Linkscan Module control structure
 */
typedef struct _ls_control_s {
    _bcm_ls_driver_t  *driver;            /* Device specific routines */
    sal_mutex_t       lock;               /* Synchronization */
    char              taskname[16];       /* Linkscan thread name */
    VOL sal_thread_t  thread_id;          /* Linkscan thread id */
    VOL int           interval_us;        /* Time between scans (us) */
    sal_sem_t         sema;               /* Linkscan semaphore */
    pbmp_t            pbm_hw;             /* Hardware link scan ports */
    pbmp_t            pbm_sw;             /* Software link scan ports */
    pbmp_t            pbm_hw_upd;         /* Ports requiring HW link re-scan */
    pbmp_t            pbm_sgmii_autoneg;  /* Ports with SGMII autoneg */
    pbmp_t            pbm_link;           /* Ports currently up */
    pbmp_t            pbm_link_change;    /* Ports needed to recognize down */
    pbmp_t            pbm_override_ports; /* Force up/Down ports */
    pbmp_t            pbm_override_link;  /* Force up/Down status */
    int               hw_change;          /* HW Link state has changed */
    _ls_errstate_t    error[NUM_PORTS];   /* Link error state */
    _ls_handler_cb_t  *handler_cb;        /* Handler to list of callbacks */
    bcm_linkscan_port_handler_t
                      port_link_f[NUM_PORTS]; /* Port link fn */
} _ls_control_t;

/*
 * Variable:
 *     _linkscan_control
 * Purpose:
 *     Hold per-unit status for linkscan module
 */
static _ls_control_t            *_linkscan_control[BCM_LOCAL_UNITS_MAX];

#define LS_CONTROL(unit)        (_linkscan_control[unit])
#define LS_CONTROL_DRV(unit)   (_linkscan_control[unit]->driver)

#define _LS_CALL(_ld, _lf, _la) \
    ((_ld) == 0 ? BCM_E_PARAM : \
     ((_ld)->_lf == 0 ? BCM_E_UNAVAIL : (_ld)->_lf _la))

#define LS_HW_INTERRUPT(_u, _b) \
    do { \
        if (LS_CONTROL_DRV(_u) && (LS_CONTROL_DRV(_u)->ld_hw_interrupt)) { \
            LS_CONTROL_DRV(_u)->ld_hw_interrupt((_u), (_b)); \
        } \
    } while (0)

#define LS_PORT_LINK_GET(_u, _p, _h, _l) \
    _LS_CALL(LS_CONTROL_DRV(_u), ld_port_link_get, ((_u), (_p), (_h), (_l)))

#define LS_INTERNAL_SELECT(_u, _p) \
    _LS_CALL(LS_CONTROL_DRV(_u), ld_internal_select, ((_u), (_p)))

#define LS_UPDATE_ASF(_u, _p, _l, _s, _d) \
    _LS_CALL(LS_CONTROL_DRV(_u), ld_update_asf, ((_u), (_p), (_l), (_s), (_d)))

#define LS_TRUNK_SW_FAILOVER_TRIGGER(_u, _a, _s) \
    _LS_CALL(LS_CONTROL_DRV(_u), ld_trunk_sw_failover_trigger, \
             ((_u), (_a), (_s)))


/*
 * Define:
 *     LS_LOCK/LS_UNLOCK
 * Purpose:
 *     Serialization Macros for access to _linkscan_control structure.
 */

#define LS_LOCK(unit) \
        sal_mutex_take(_linkscan_control[unit]->lock, sal_mutex_FOREVER)

#define LS_UNLOCK(unit) \
        sal_mutex_give(_linkscan_control[unit]->lock)

/*
 * General util macros
 */
#define UNIT_VALID_CHECK(unit) \
    if (((unit) < 0) || ((unit) >= BCM_LOCAL_UNITS_MAX)) { return BCM_E_UNIT; }

#define UNIT_INIT_CHECK(unit) \
    do { \
        UNIT_VALID_CHECK(unit); \
        if (_linkscan_control[unit] == NULL) { return BCM_E_INIT; } \
    } while (0)

#ifdef BCM_WARM_BOOT_SUPPORT
#define LINK_WB_VERSION_1_0                SOC_SCACHE_VERSION(1,0)
#define LINK_WB_CURRENT_VERSION            LINK_WB_VERSION_1_0
#endif /* BCM_WARM_BOOT_SUPPORT */


/* Local Function Declaration */
int bcm_common_linkscan_enable_set(int unit, int us);

/*
 * Function:
 *     _bcm_linkscan_pbm_init
 * Purpose:
 *     Initialize the port bitmaps in the link_control structure.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 * Notes:
 *     Assumes: valid unit number, lock is held.
 */
STATIC void
_bcm_linkscan_pbm_init(int unit)
{
    _ls_control_t  *lc = LS_CONTROL(unit);
    /*
     * Force initial scan by setting link change while pretending link
     * was initially up.
     */
    BCM_PBMP_ASSIGN(lc->pbm_link, PBMP_ALL(unit));
    BCM_PBMP_ASSIGN(lc->pbm_link_change, PBMP_PORT_ALL(unit));

    BCM_PBMP_CLEAR(lc->pbm_override_ports);
    BCM_PBMP_CLEAR(lc->pbm_override_link);

    BCM_PBMP_ASSIGN(lc->pbm_hw_upd, PBMP_ALL(unit));

}

/*
 * Function:    
 *     _bcm_linkscan_update_port
 * Purpose:     
 *     Check for and process a link event on one port.
 * Parameters:  
 *     unit - Device unit number
 *     port - Device port to process
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     Assumes: valid unit number, lock is held.
 */
STATIC int
_bcm_linkscan_update_port(int unit, int port)
{
    _ls_control_t       *lc = LS_CONTROL(unit);
    int                 cur_link, change, new_link = FALSE;
    bcm_port_info_t     info;
    _ls_handler_cb_t    *lh, *lh_next = NULL;
    int                 rv;

    assert(SOC_PORT_VALID(unit, port)); 
    /*
     * Current link status is calculated in the following order:
     *   1) If link status is in an override state, the override
     *      state is used.
     *   2) If link is required to recognize a link down event, the
     *      current scan will recognize the link down (if the link
     *      was previously down, this will result in no action
     *      being taken)
     *   3) Use real detected link status.
     *        a) If using Hardware link scanning, use captured H/W
     *           value since the H/W scan will clear the latched
     *           link down event.
     *        b) If using S/W link scanning, retrieve the current
     *           link status from either:
     *           - registered port link routine (registered
     *             with 'bcm_linkscan_port_register'),
     *           - or the PHY.
     */

    cur_link = BCM_PBMP_MEMBER(lc->pbm_link, port);

    change = BCM_PBMP_MEMBER(lc->pbm_link_change, port);
    BCM_PBMP_PORT_REMOVE(lc->pbm_link_change, port);

    if (change) {                                                  /* 2) */
        new_link = FALSE;
        rv = BCM_E_NONE;

    } else if (BCM_PBMP_MEMBER(lc->pbm_override_ports, port)) {    /* 1) */
        new_link = BCM_PBMP_MEMBER(lc->pbm_override_link, port);
        rv = BCM_E_NONE;

    } else if (BCM_PBMP_MEMBER(lc->pbm_hw, port)) {                /* 3a) */
        new_link = BCM_PBMP_MEMBER(lc->pbm_hw_upd, port);
        /*
         * If link up, we should read the link status from the PHY because
         * the PHY may link up but autneg may not have completed.
         */
        if (new_link || 
            BCM_PBMP_MEMBER(lc->pbm_sgmii_autoneg, port)) {
            /* Fall back on SW linkscan to detect possible medium change */
            rv = LS_PORT_LINK_GET(unit, port, 1, &new_link);
        } else {
            rv = BCM_E_NONE;
        }

    } else if (BCM_PBMP_MEMBER(lc->pbm_sw, port)) {                /* 3b) */
        if (lc->port_link_f[port]) {    /* Use registered port link function */
            int state;

            rv = lc->port_link_f[port](unit, port, &state);
            if (rv == BCM_E_NONE) {
                new_link = state ? TRUE : FALSE;
            } else if (rv == BCM_E_UNAVAIL) {  /* Retrieve PHY link */
                rv = LS_PORT_LINK_GET(unit, port, 0, &new_link);
            }
        } else {
            rv = LS_PORT_LINK_GET(unit, port, 0, &new_link);
        }
        LINK_VERB(("SW link p=%d %s\n", port, new_link ? "up" : "down"));
    } else {
        return BCM_E_NONE;    /* Port not being scanned */
    }

    if (BCM_FAILURE(rv)) {
        LINK_ERR(("Port %s: Failed to recover link status: %s\n", 
                  SOC_PORT_NAME(unit, port), bcm_errmsg(rv)));
        return rv;
    }

    if (((cur_link) && (new_link)) || ((!cur_link) && (!new_link))) {
        /* No change */
        return BCM_E_NONE;
    }

    /*
     * If disabling, stop ingresses from sending any more traffic to
     * this port.
     */

    if (!new_link) {
        BCM_PBMP_PORT_REMOVE(lc->pbm_link, port);

        rv = soc_linkctrl_link_fwd_set(unit, lc->pbm_link);

        if (BCM_FAILURE(rv)) {
            LINK_ERR(("Port %s: soc_linkctrl_link_fwd_set failed: %s\n",
                      SOC_PORT_NAME(unit, port), bcm_errmsg(rv)));
            return rv;
        }
    }

    /* Program MACs (only if port is not forced) */

    if (!BCM_PBMP_MEMBER(lc->pbm_override_ports, port) &&
        (cur_link != new_link)) {
        rv = bcm_port_update(unit, port, new_link);

        if (BCM_FAILURE(rv)) {
            LINK_ERR(("Port %s: bcm_port_update failed: %s\n",
                      SOC_PORT_NAME(unit, port), bcm_errmsg(rv)));
            return rv;
        }
    }

    /*
     * If enabling, allow traffic to go to this port.
     */

    if (new_link) {
        BCM_PBMP_PORT_ADD(lc->pbm_link, port);

        rv = soc_linkctrl_link_fwd_set(unit, lc->pbm_link);

        if (BCM_FAILURE(rv)) {
            LINK_ERR(("Port %s: soc_linkctrl_link_fwd_set failed: %s\n",
                      SOC_PORT_NAME(unit, port), bcm_errmsg(rv)));
            return rv;
        }
    }

    /*
     * Call registered handlers with complete link info.
     * Display link status message, if requested.
     *
     * In case link status changed again for bcm_port_info_get,
     * overwrite the linkstatus field with new_link.  This ensures
     * the handler is presented with a consistent alternating
     * sequence of link up/down.
     */

    rv = bcm_port_info_get(unit, port, &info);

    if (BCM_FAILURE(rv)) {
        LINK_ERR(("Port %s: bcm_port_info_get failed: %s\n", 
                  SOC_PORT_NAME(unit, port), bcm_errmsg(rv)));
        return rv;
    }

    if (soc_feature(unit, soc_feature_asf)) {
        rv = LS_UPDATE_ASF(unit, port, new_link, info.speed, info.duplex);
        if (BCM_FAILURE(rv)) {
            LINK_ERR(("Port %s: linkscan ASF update failed: %s\n",
                      SOC_PORT_NAME(unit, port), bcm_errmsg(rv)));
            return rv;
        }
    }

    if (new_link) {
        LINK_VERB(("Port %s: link up (%dMb %s %s)\n",
                   SOC_PORT_NAME(unit, port),
                   info.speed,
                   info.duplex ? "Full Duplex" : "Half Duplex",
                   PHY_FIBER_MODE(unit, port) ?
                   "Fiber" : "Copper"));
    } else {
        LINK_VERB(("Port %s: link down\n",
                   SOC_PORT_NAME(unit, port)));
    }

    info.linkstatus = new_link;

    for (lh = lc->handler_cb; lh; lh = lh_next) {
        /*
         * save the next linkscan handler first, in case current handler
         * unregister itself inside the handler function
         */
        lh_next = lh->next;        
        lh->cb_f(unit, port, &info);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     _bcm_linkscan_update
 * Purpose:     
 *     Check for a change in link status on each link.  If a change
 *     is detected, call bcm_port_update to program the MACs for that
 *     link, and call the list of registered handlers.
 * Parameters:  
 *     unit - Device unit number
 *     pbm  - Bitmap of ports to scan
 * Returns:
 *     Nothing.
 * Notes:
 *     Assumes: valid unit number.
 */
STATIC void
_bcm_linkscan_update(int unit, pbmp_t pbm)
{
    soc_persist_t  *sop = SOC_PERSIST(unit);
    _ls_control_t  *lc = LS_CONTROL(unit);
    pbmp_t         save_link_change;
    int            rv;
    bcm_port_t     port;

    LS_LOCK(unit);

    /*
     * Suspend hardware link scan here to avoid overhead of pause/resume
     * on MDIO accesses. Ideally this would not be required if the source
     * of the interrupt is the signal from the internal Serdes but we still
     * need to do this due to work-arounds in phy drivers where we need to
     * rely on SW linkscan to detect link up.
     */
    soc_linkctrl_linkscan_pause(unit);           /* Suspend linkscan */

    PBMP_ITER(pbm, port) {
        
        _ls_errstate_t *err = &lc->error[port];

        if (err->wait) {    /* Port waiting in error state */
            if (SAL_USECS_SUB(sal_time_usecs(), err->time) >= err->delay) {
                err->wait = 0;    /* Exit error state */
                err->count = 0;

                LINK_WARN(("Port %s: restored\n",
                           SOC_PORT_NAME(unit, port)));
            } else {
                continue;
            }
        }

        save_link_change = sop->lc_pbm_link_change;

        rv = _bcm_linkscan_update_port(unit, port);

        if (BCM_FAILURE(rv)) {
            sop->lc_pbm_link_change = save_link_change;

            if (++err->count >= err->limit && err->limit > 0) {
                /* Enter error state */
                LINK_ERR(("Port %s: temporarily removed from linkscan\n",
                          SOC_PORT_NAME(unit, port)));

                err->time = sal_time_usecs();
                err->wait = 1;
            }
        } else if (err->count > 0) {
            err->count--;    /* Reprieve */
        }

    }

    soc_linkctrl_linkscan_continue(unit);        /* Restart H/W link scan */

    LS_UNLOCK(unit);
}

/*
 * Function:    
 *     _bcm_linkscan_hw_interrupt
 * Purpose:     
 *     Link scan interrupt handler.
 * Parameters:  
 *     unit - Device unit number
 * Returns:     
 *     Nothing
 * Notes:
 *     Assumes: valid unit number.
 */
STATIC void
_bcm_linkscan_hw_interrupt(int unit)
{
    _ls_control_t  *lc = LS_CONTROL(unit);

    if ((NULL != lc) && (NULL != lc->sema)) {
        lc->hw_change = 1;
        sal_sem_give(lc->sema);
    }
    soc_cm_debug(DK_LINK | DK_VERBOSE, "Linkscan interrupt unit %d\n", unit);
}

/*
 * Function:    
 *     _bcm_linkscan_thread
 * Purpose:     
 *     Scan the ports on the specified unit for link status
 *     changes and process them as they occur.
 * Parameters:  
 *     unit - Device unit number
 * Returns:     
 *     Nothing
 * Notes:
 *     Assumes: valid unit number.
 */
STATIC void
_bcm_linkscan_thread(int unit)
{
    soc_persist_t  *sop = SOC_PERSIST(unit);
    _ls_control_t  *lc = LS_CONTROL(unit);
    sal_usecs_t    interval;
    int            rv;
    soc_port_t     port;

    LINK_OUT(("Linkscan starting on unit %d\n", unit));

    /* Do not clear the pbm_override_ports and pbm_override_link.
     * If a port in Loopback mode forces link up before enabling linkscan,
     * the port status should be still forced to up after enabling linkscan.
     */

    /*
     * Force initial scan by setting link change while pretending link
     * was initially up.
     */
    BCM_PBMP_ASSIGN(lc->pbm_link, PBMP_ALL(unit));
    BCM_PBMP_ASSIGN(lc->pbm_link_change, PBMP_PORT_ALL(unit));

    sal_memset(lc->error, 0, sizeof (lc->error));

    PBMP_ITER(PBMP_PORT_ALL(unit), port) {
        lc->error[port].limit = soc_property_port_get(unit, port,
                                         spn_BCM_LINKSCAN_MAXERR, 5);
        lc->error[port].delay = soc_property_port_get(unit, port,
                                         spn_BCM_LINKSCAN_ERRDELAY, 10000000);
    }

    /* Clear initial value of forwarding ports. */
    rv = soc_linkctrl_link_fwd_set(unit, lc->pbm_link);

    if (BCM_FAILURE(rv)) {
        LINK_ERR(("Failed to clear forwarding ports: %s\n", 
                  bcm_errmsg(rv)));
        sal_thread_exit(0);
    }

    /* Register for hardware linkscan interrupt. */
    rv = soc_linkctrl_linkscan_register(unit, _bcm_linkscan_hw_interrupt);

    if (BCM_FAILURE(rv)) {
        LINK_ERR(("Failed to register handler: %s\n",
                  bcm_errmsg(rv)));
        sal_thread_exit(0);
    }

    lc->thread_id = sal_thread_self();

    while ((interval = lc->interval_us) != 0) {
        pbmp_t  change;
        pbmp_t  hw_link, hw_update;

        if (BCM_PBMP_IS_NULL(lc->pbm_sw)) {
            interval = sal_sem_FOREVER;
        }

        /* sample changed */
        BCM_PBMP_ASSIGN(change, sop->lc_pbm_link_change);

        if (lc->hw_change) {
            soc_linkctrl_linkscan_pause(unit);
            lc->hw_change = 0;

            (void)soc_linkctrl_hw_link_get(unit, &hw_link);
            BCM_PBMP_AND(hw_link, lc->pbm_hw);

#ifdef BCM_TRUNK_FAILOVER_SUPPORT
            LS_TRUNK_SW_FAILOVER_TRIGGER(unit, lc->pbm_hw, lc->pbm_hw_upd);
#endif /* BCM_TRUNK_FAILOVER_SUPPORT */

            /* Make sure that only valid ports are scaned */
            BCM_PBMP_ASSIGN(hw_update, hw_link);
            BCM_PBMP_XOR(hw_update, sop->lc_pbm_link);
            BCM_PBMP_AND(hw_update, lc->pbm_hw);
            
            /* Make sure that only valid ports are scaned */
            _bcm_linkscan_update(unit, hw_update);

            /* Make sure that only valid ports are scaned */
            BCM_PBMP_AND(change, PBMP_PORT_ALL(unit));
            _bcm_linkscan_update(unit, change);
            soc_linkctrl_linkscan_continue(unit);
        }

        /* After processing the link status changes of the ports
         * indicated by interrupt handler (mainly to trigger swfailover),
         * scan all the ports again to make sure that the link status
         * is stable.
         * For some PHYs such as 5228, hardware linkscan may say the
         * link is up while the PHY is actually not quite done
         * autonegotiating. Rescanning make sure that the PHY link is
         * in sync with switch HW link state.
         */ 
        _bcm_linkscan_update(unit, PBMP_PORT_ALL(unit));

        if (!BCM_PBMP_IS_NULL(change)) {
            /* Re-scan due to hardware force */
            continue;
        }

        (void)sal_sem_take(lc->sema, interval);
    }

    (void)soc_linkctrl_linkscan_register(unit, NULL);

    /*
     * Before exiting, re-enable all ports that were being scanned.
     *
     * For administrative reloads, application can enter reload
     * mode to avoid this disturbing of ports.
     */

    BCM_PBMP_ITER(lc->pbm_sw, port) {
        int enable;

        if (BCM_SUCCESS(bcm_port_enable_get(unit, port, &enable))) {
            (void)bcm_port_update(unit, port, enable);
        }
    }

    BCM_PBMP_ITER(lc->pbm_hw, port) {
        int enable;

        if (BCM_SUCCESS(bcm_port_enable_get(unit, port, &enable))) {
            (void)bcm_port_update(unit, port, enable);
        }
    }

    LINK_OUT(("Linkscan exiting\n"));

    lc->thread_id = NULL;
    sal_thread_exit(0);
}


/*
 * Internal BCM link routines
 */

/*
 * Function:    
 *     _bcm_link_get
 * Purpose:
 *     Return linkscan's current link status for the given port.
 * Parameters:  
 *     unit - Device unit number
 *     port - Device port number
 *     link - (OUT) Current link status
 * Returns:
 *     BCM_E_NONE     - Success
 *     BCM_E_DISABLED - Port not being scanned
 * Note:
 *     This routine does not acquire the LS_LOCK, as it only reads a 
 *     snapshot of the link bitmaps.  It also cannot hold the LS_LOCK 
 *     since it is called indirectly from the linkscan thread 
 *     when requesting port info.
 */
int
_bcm_link_get(int unit, bcm_port_t port, int *link)
{
    int            rv = BCM_E_NONE;
    _ls_control_t  *lc;

    UNIT_VALID_CHECK(unit);

    if ((lc = LS_CONTROL(unit)) == NULL) {
        return BCM_E_DISABLED;
    }

    if (BCM_PBMP_MEMBER(lc->pbm_override_ports, port)) {
        *link = BCM_PBMP_MEMBER(lc->pbm_override_link, port);
        return BCM_E_NONE;
    }

    rv = bcm_linkscan_enable_port_get(unit, port);
    if (BCM_SUCCESS(rv)) {
        *link = BCM_PBMP_MEMBER(lc->pbm_link, port);
    }

    return rv;
}

/*
 * Function:    
 *     _bcm_link_force
 * Purpose:
 *     Set linkscan's current link status for a port.
 * Parameters:  
 *     unit  - Device unit number
 *     port  - Device port number
 *     force - If TRUE, link status is forced to new link status;
 *             if FALSE, link status is no longer forced
 *     link  - New link status
 * Returns:
 *     BCM_E_NONE - Success
 *     BCM_E_INIT - Not initialized.
 * Notes:
 *     When a link is forced up or down, linkscan will stop scanning
 *     that port and _bcm_link_get will always return the forced status.
 *     This is used for purposes such as when a link is placed in MAC
 *     loopback.  If software forces a link up, it is responsible for
 *     configuring that port.
 */
int
_bcm_link_force(int unit, bcm_port_t port, int force, int link)
{
    _ls_control_t  *lc;
    pbmp_t         pbm;

    UNIT_INIT_CHECK(unit);

    if (!SOC_PORT_VALID(unit, port) || !IS_PORT(unit, port)) {
        return BCM_E_PORT;
    }

    LS_LOCK(unit);
    lc = LS_CONTROL(unit);

    if (force) {
        BCM_PBMP_PORT_REMOVE(lc->pbm_override_link, port);
        if (link) {
            BCM_PBMP_PORT_ADD(lc->pbm_override_link, port);
        }
        BCM_PBMP_PORT_ADD(lc->pbm_override_ports, port);
    } else {
        BCM_PBMP_PORT_REMOVE(lc->pbm_override_ports, port);
        BCM_PBMP_PORT_REMOVE(lc->pbm_override_link, port);
        BCM_PBMP_PORT_ADD(lc->pbm_link_change, port);
    }

    /*
     * Force immediate update to just this port - this allows loopback 
     * forces to take effect immediately.
     */
    BCM_PBMP_CLEAR(pbm);
    BCM_PBMP_PORT_ADD(pbm, port);
    _bcm_linkscan_update(unit, pbm);

    LS_UNLOCK(unit);

    /*
     * Wake up master thread to notice changes - required if using hardware
     * link scanning.
     */
    if (lc->sema != NULL) {
        sal_sem_give(lc->sema);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_linkscan_init
 * Purpose:
 *     Initialize the linkscan software module.
 * Parameters:
 *     unit   - Device unit number
 *     driver - Device specific link routines
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     Device specific bcm_linkscan_init() must call this routine
 *     in order to initialize BCM linkscan common module.
 */
int
_bcm_linkscan_init(int unit, _bcm_ls_driver_t *driver)
{
    _ls_control_t   *lc;
    bcm_port_if_t   intf;
    int             rv = BCM_E_NONE;
    uint32          size;
    soc_port_t      port;
#ifdef BCM_WARM_BOOT_SUPPORT
    uint32          scache_handle;
    uint8           *scache_ptr = NULL;
    uint16          default_ver = LINK_WB_CURRENT_VERSION;
    uint16          recovered_ver = LINK_WB_CURRENT_VERSION;
    _ls_control_t   *free_lc = NULL;
#endif

    UNIT_VALID_CHECK(unit);

    if (_linkscan_control[unit] != NULL) {
        BCM_IF_ERROR_RETURN(bcm_linkscan_detach(unit));
    }

    size = sizeof(_ls_control_t);
    if ((lc = sal_alloc(size, "link_control")) == NULL) {
        return BCM_E_MEMORY;
    }
    sal_memset(lc, 0, size);

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_LINKSCAN, 0);

    /* on cold boot, setup scache */
    rv = soc_versioned_scache_ptr_get(unit, scache_handle,
                                      (SOC_WARM_BOOT(unit) ? FALSE : TRUE),
                                      &size, &scache_ptr,
                                      default_ver, &recovered_ver);
    if (BCM_FAILURE(rv) && (rv != SOC_E_NOT_FOUND)) {
        BCM_ERR(("%s: Error(%s) reading scache. scache_ptr:%p and len:%d\n",
                 FUNCTION_NAME(), soc_errmsg(rv), scache_ptr, size));
        /* If warmboot initialization fails, skip using warmboot for linkscan */
        rv = SOC_E_NOT_FOUND;
        goto skip_wb;
    }

    LINK_VERB(("LINKSCAN: allocating 0x%x (%d) bytes of scache:",  size, size));

    rv = soc_scache_handle_used_set(unit, scache_handle, size);
    
    /* Get the pointer for the Level 2 cache */
    free_lc = lc;
    
    if (scache_ptr) {
        /* if supporting warmboot, use scache */
        sal_free(lc);
        free_lc = NULL;
        lc = (_ls_control_t*) scache_ptr;

        if (SOC_WARM_BOOT(unit)) {
            /* clear out non-recoverable resources */
            lc->thread_id = NULL;
            lc->sema = NULL;
            lc->lock = NULL;
            lc->driver = NULL;
            lc->handler_cb = NULL;
            sal_memset(lc->port_link_f, 0, sizeof(lc->port_link_f));
        } else {
            sal_memset(lc, 0, size);
        }
    }

skip_wb:
#endif /* BCM_WARM_BOOT_SUPPORT */

    if (BCM_SUCCESS(rv) || (rv == SOC_E_NOT_FOUND)) {
        lc->lock = sal_mutex_create("bcm_link_LOCK");
        if (lc->lock == NULL) {
            rv = BCM_E_MEMORY;
        }
    }

    if (BCM_SUCCESS(rv) || (rv == SOC_E_NOT_FOUND)) {
        lc->sema = sal_sem_create("bcm_link_SLEEP", 
                                  sal_sem_BINARY, 0);
        if (lc->sema == NULL) {
            sal_mutex_destroy(lc->lock);
            rv = BCM_E_MEMORY;
        }
    }

    if (BCM_FAILURE(rv) && (rv != SOC_E_NOT_FOUND)) {
#ifdef BCM_WARM_BOOT_SUPPORT
        if (free_lc) {
            sal_free(free_lc);
        } else
#endif
        {
            sal_free(lc);
        }

        return rv;
    }

    _linkscan_control[unit] = lc;
    _linkscan_control[unit]->driver = driver;

    /*
     * Initialize link_control port bitmaps so bcm_port_update works
     * reasonably even if the linkscan thread is never started.
     */
    if (!SOC_WARM_BOOT(unit)) {
        _bcm_linkscan_pbm_init(unit);
    }

    /* 1. Select between C45 and C22 for HW linkscan
     * 2. Select appropriate MDIO Bus
     * 3. Select between MDIO scan vs. link status from internal PHY
     * 4. Initialize HW linkscan PHY address map.
     */
    BCM_IF_ERROR_RETURN(soc_linkctrl_linkscan_hw_init(unit));

    /*
     * Select the source of the CMIC link status interrupt
     * to be the Internal Serdes on SGMII ports
     */ 
    BCM_PBMP_CLEAR(lc->pbm_sgmii_autoneg);
    if (soc_feature(unit, soc_feature_sgmii_autoneg)) {
        PBMP_ITER(PBMP_PORT_ALL(unit), port) {
            if (soc_property_port_get(unit, port,
                                      spn_PHY_SGMII_AUTONEG, FALSE)) {
                rv = bcm_port_interface_get(unit, port, &intf);
                if ( BCM_SUCCESS(rv) && (intf == BCM_PORT_IF_SGMII)){
                    BCM_PBMP_PORT_ADD(lc->pbm_sgmii_autoneg, port);
                    LS_INTERNAL_SELECT(unit, port);
                }
            }
        }
    }

    if (SOC_IS_PETRAB(unit) && SOC_WARM_BOOT(unit) && (lc->interval_us != 0)) {
        rv = bcm_linkscan_enable_set(unit, lc->interval_us);
    }

    return BCM_E_NONE;
}

#ifdef BCM_WARM_BOOT_SUPPORT
/* This function compresses the info in _ls_control_t and stores it
 * to stable memory.
 * Input param: sync --> indicates whether to sync scache to Persistent memory
 */
int
bcm_linkscan_sync(int unit, int sync)
{
    int                     rv = BCM_E_NONE;
    uint8                   *scache_ptr = NULL;
    uint32                  scache_len = 0;
    soc_scache_handle_t     scache_handle;
    uint16                  default_ver = LINK_WB_CURRENT_VERSION;
    uint16                  recovered_ver = LINK_WB_CURRENT_VERSION;

    if (SOC_WARM_BOOT(unit)) {
        SOC_ERROR_PRINT((DK_ERR, "Cannot write to SCACHE during WarmBoot\n"));
        return SOC_E_INTERNAL;
    }

    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_LINKSCAN, 0);

    rv = soc_versioned_scache_ptr_get(unit, scache_handle,
                                      FALSE, &scache_len, &scache_ptr,
                                      default_ver, &recovered_ver);
    if (SOC_FAILURE(rv) && (rv != SOC_E_NOT_FOUND)) {
        BCM_ERR(("%s: Error(%s) reading scache. scache_ptr:%p and len:%d\n",
                 FUNCTION_NAME(), soc_errmsg(rv), scache_ptr, scache_len));
        return rv;
    }

    rv = soc_scache_handle_used_set(unit, scache_handle, sizeof(_ls_control_t));

    if (sync) {
        rv = soc_scache_commit(unit);
        if (rv != SOC_E_NONE) {
            BCM_ERR(("%s: Error(%s) sync'ing scache to Persistent memory. \n",
                     FUNCTION_NAME(), soc_errmsg(rv)));
            return rv;
        }
    }
    return BCM_E_NONE;
}
#endif /* BCM_WARM_BOOT_SUPPORT */

/************************************************************************
 *********                                                      *********
 *********         Start of BCM API Exported Routines           *********
 *********                                                      *********
 ************************************************************************/

/*
 * Function:
 *     bcm_linkscan_detach
 * Purpose:
 *     Prepare linkscan module to detach specified unit.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     BCM_E_NONE - Detach successful
 *     BCM_E_XXX  - Detach failed
 * Notes:
 *     This is safe to call at any time, but linkscan should only be
 *     initialized or detached from the main application thread.
 */
int
bcm_common_linkscan_detach(int unit)
{
    _ls_control_t     *lc;
    _ls_handler_cb_t  *lh;
    pbmp_t            empty_pbm;
#ifdef BCM_WARM_BOOT_SUPPORT
    int               size, rv;
    uint32            scache_handle;
    uint8            *ptr;
    soc_wb_cache_t   *wb_cache = NULL;
#endif

    UNIT_VALID_CHECK(unit);

    lc = LS_CONTROL(unit);
    if (lc == NULL) {
        return BCM_E_NONE;
    }

    BCM_PBMP_CLEAR(empty_pbm);

    SOC_IF_ERROR_RETURN(soc_linkctrl_linkscan_config(unit,
                                                     empty_pbm, empty_pbm));

    BCM_IF_ERROR_RETURN(bcm_common_linkscan_enable_set(unit, 0));

    /* Clean up list of handlers */

    while (lc->handler_cb != NULL) {
        lh = lc->handler_cb;
        lc->handler_cb = lh->next;
        sal_free(lh);
    }

    /* Mark and not initialized and free mutex */

    if (lc->sema != NULL) {
        sal_sem_destroy(lc->sema);
        lc->sema = NULL;
    }

    if (lc->lock != NULL) {
        sal_mutex_destroy(lc->lock);
        lc->lock = NULL;
    }

    LS_CONTROL(unit) = NULL;

#ifdef BCM_WARM_BOOT_SUPPORT
    SOC_SCACHE_HANDLE_SET(scache_handle, unit, BCM_MODULE_LINKSCAN, 0);
    rv = soc_stable_size_get(unit, &size);
    if (BCM_SUCCESS(rv) && size) {
        rv = soc_scache_ptr_get(unit, scache_handle, &ptr, (uint32*)&size);
        wb_cache = (soc_wb_cache_t*)ptr;
    }

    /* lc was allocated when Warm Boot is compiled and 
     * the scache is not configured */
    if (BCM_SUCCESS(rv) && (wb_cache == NULL)) {
        sal_free(lc);
    }
    
#else
    sal_free(lc);
#endif

    return BCM_E_NONE;
}

/*
 * Function:    
 *     bcm_linkscan_enable_set
 * Purpose:
 *     Enable or disable the link scan feature.
 * Parameters:
 *     unit - Device unit number
 *     us   - Specifies the software polling interval in micro-seconds;
 *            the value 0 disables linkscan
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_common_linkscan_enable_set(int unit, int us)
{
    _ls_control_t   *lc;
    int             rv = BCM_E_NONE;
    soc_timeout_t   to;
    pbmp_t          empty_pbm;

    UNIT_VALID_CHECK(unit);

    lc = LS_CONTROL(unit);
    if (!us && lc == NULL) {    /* No error to disable if not initialized */
        return BCM_E_NONE;
    }

    UNIT_INIT_CHECK(unit);

    sal_snprintf(lc->taskname,
                 sizeof (lc->taskname),
                 "bcmLINK.%d",
                 unit);

    BCM_PBMP_CLEAR(empty_pbm);

    if (us) {
        if (us < BCM_LINKSCAN_INTERVAL_MIN) {
            us = BCM_LINKSCAN_INTERVAL_MIN;
        }

        lc->interval_us = us;

        if (lc->thread_id != NULL) {
            /* Linkscan is already running; just update the period */
            sal_sem_give(lc->sema);
            return BCM_E_NONE;
        }


        if (sal_thread_create(lc->taskname,
                              SAL_THREAD_STKSZ,
                              soc_property_get(unit,
                                               spn_LINKSCAN_THREAD_PRI,
                                               50),
                              (void (*)(void*))_bcm_linkscan_thread,
                              INT_TO_PTR(unit)) == SAL_THREAD_ERROR) {
            lc->interval_us = 0;
            rv = BCM_E_MEMORY;
        } else {
            soc_timeout_init(&to, 3000000, 0);

            while (lc->thread_id == NULL) {
                if (soc_timeout_check(&to)) {
                    LINK_ERR(("%s: Thread did not start\n",
                              lc->taskname));
                    lc->interval_us = 0;
                    rv = BCM_E_INTERNAL;
                    break;
                }
            }
            if (BCM_SUCCESS(rv)) {
                /* Make sure HW linkscanning is enabled on HW linkscan ports */
                rv = soc_linkctrl_linkscan_config(unit,
                                                  lc->pbm_hw, empty_pbm);
            }

            sal_sem_give(lc->sema);
        }
    } else if (lc->thread_id != NULL) {
        lc->interval_us = 0;

        /* To prevent getting HW linkscan interrupt after linkscan is disabled,
         * HW linkscanning must be disabled. */ 
        rv = soc_linkctrl_linkscan_config(unit, empty_pbm, empty_pbm);

        sal_sem_give(lc->sema);

        soc_timeout_init(&to, 10000000, 0);   /* Enough time for Quickturn */

        while (lc->thread_id != NULL) {
            if (soc_timeout_check(&to)) {
                LINK_ERR(("%s: Thread did not exit\n",
                          lc->taskname));
                rv = BCM_E_INTERNAL;
                break;
            }
        }
    }

    return rv;
}

/*
 * Function:    
 *     bcm_linkscan_enable_get
 * Purpose:
 *     Retrieve the current linkscan mode.
 * Parameters:
 *     unit - Device unit number
 *     us   - (OUT) Pointer to microsecond scan time for software scanning, 
 *             0 if not enabled.
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_common_linkscan_enable_get(int unit, int *us)
{
    UNIT_INIT_CHECK(unit);

    *us = LS_CONTROL(unit)->interval_us;

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_linkscan_enable_port_get
 * Purpose:
 *     Determine whether or not linkscan is managing a given port.
 * Parameters:
 *     unit - Device unit number
 *     port - Device port to check
 * Returns:
 *     BCM_E_NONE     - Port being scanned
 *     BCM_E_DISABLED - Port not being scanned
 */
int
bcm_common_linkscan_enable_port_get(int unit, bcm_port_t port)
{
    int            rv = BCM_E_NONE;
    _ls_control_t  *lc;

    UNIT_VALID_CHECK(unit);

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_port_local_get(unit, port, &port));
    }

    if (!SOC_PORT_VALID(unit, port) || !IS_PORT(unit, port)) {
        return BCM_E_PORT;
    }

    if ((lc = LS_CONTROL(unit)) == NULL) {
        return BCM_E_DISABLED;
    }

    if (lc->interval_us == 0 ||
        (!BCM_PBMP_MEMBER(lc->pbm_sw, port) &&
         !BCM_PBMP_MEMBER(lc->pbm_hw, port) &&
         !BCM_PBMP_MEMBER(lc->pbm_override_ports, port))) {
        rv = BCM_E_DISABLED;
    }
    return rv;
}

/*
 * Function:    
 *     bcm_linkscan_mode_set
 * Purpose:
 *     Set the current scanning mode for the specified port.
 * Parameters:
 *     unit - Device unit number
 *     port - Device port number
 *     mode - New scan mode for specified port
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_common_linkscan_mode_set(int unit, bcm_port_t port, int mode)
{
    _ls_control_t   *lc;
    int             rv = BCM_E_NONE;
    pbmp_t          empty_pbm;
    int             added = 0, sw_member = 0;

    UNIT_INIT_CHECK(unit);

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_port_local_get(unit, port, &port));
    }

    if (!SOC_PORT_VALID(unit, port) || !IS_PORT(unit, port)) {
        return BCM_E_PORT;
    }

    BCM_PBMP_CLEAR(empty_pbm);

    LS_LOCK(unit);

    lc = LS_CONTROL(unit);

    /* First, remove from current configuration */
    if (BCM_PBMP_MEMBER(lc->pbm_sw, port)) {
        sw_member = 1;
    }
 
    BCM_PBMP_PORT_REMOVE(lc->pbm_sw, port);
    BCM_PBMP_PORT_REMOVE(lc->pbm_hw, port);

    /* Now add back to proper map */
    switch (mode) {
    case BCM_LINKSCAN_MODE_NONE:
        break;
    case BCM_LINKSCAN_MODE_SW:
        BCM_PBMP_PORT_ADD(lc->pbm_sw, port);
        added = 1;
        break;
    case BCM_LINKSCAN_MODE_HW:
        BCM_PBMP_PORT_ADD(lc->pbm_hw, port);

        if (BCM_PBMP_MEMBER(lc->pbm_sgmii_autoneg, port)) {
            /* Need to run SW link scan as well on ports where the source 
             * of the link status is the internal Serdes - only SGMII ports */
            BCM_PBMP_PORT_ADD(lc->pbm_sw, port);
        }
        added = 1;
        lc->hw_change = 1;
        break;
    default:
        rv = BCM_E_PARAM;
        break;
    }

    /* Reconfigure HW linkscan in case changed */
    rv = soc_linkctrl_linkscan_config(unit, lc->pbm_hw, empty_pbm);

    /* Prime the HW linkscan pump */
    if (SOC_PBMP_NOT_NULL(lc->pbm_hw)) {
        lc->hw_change = 1;
        _bcm_linkscan_hw_interrupt(unit);
    }

    if (rv == BCM_E_UNAVAIL && added && sw_member) {
        BCM_PBMP_PORT_ADD(lc->pbm_sw, port);
        BCM_PBMP_PORT_REMOVE(lc->pbm_hw, port); 
    }

    LS_UNLOCK(unit);

    if (lc->sema != NULL) {
        sal_sem_give(lc->sema);    /* register change now */
    }

    /* When no longer scanning a port, return it to the enabled state. */
    if (BCM_SUCCESS(rv) && !added) {
        int enable;

        rv = bcm_port_enable_get(unit, port, &enable);
        if (BCM_SUCCESS(rv)) {
            rv = bcm_port_update(unit, port, enable);
        }
    }

    return rv;
}

/*
 * Function:    
 *     bcm_linkscan_mode_set_pbm
 * Purpose:
 *     Set the current scanning mode for the specified ports.
 * Parameters:
 *     unit - Device unit number
 *     pbm  - Port bitmap indicating port to set
 *     mode - New scan mode for specified ports
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_common_linkscan_mode_set_pbm(int unit, pbmp_t pbm, int mode)
{
    bcm_port_t  port;

    UNIT_INIT_CHECK(unit);

    PBMP_ITER(pbm, port) {
        BCM_IF_ERROR_RETURN
            (bcm_linkscan_mode_set(unit, port, mode));
    }

    return BCM_E_NONE;
}

/*
 * Function:    
 *     bcm_linkscan_mode_get
 * Purpose:
 *     Recover the current scanning mode for the specified port.
 * Parameters:
 *     unit - Device unit number
 *     port - Device port number
 *     mode - (OUT) current scan mode for specified port
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_common_linkscan_mode_get(int unit, bcm_port_t port, int *mode)
{
    _ls_control_t  *lc;

    UNIT_INIT_CHECK(unit);

    lc = LS_CONTROL(unit);

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_port_local_get(unit, port, &port));
    }

    if (!SOC_PORT_VALID(unit, port) || !IS_PORT(unit, port)) {
        return BCM_E_PORT;
    }

    if (PBMP_MEMBER(lc->pbm_hw, port)) {
        *mode = BCM_LINKSCAN_MODE_HW;
    } else if (PBMP_MEMBER(lc->pbm_sw, port)) {
        *mode = BCM_LINKSCAN_MODE_SW;
    } else {
        *mode = BCM_LINKSCAN_MODE_NONE;
    }

    return BCM_E_NONE;
}

/*
 * Function:    
 *     bcm_linkscan_update
 * Purpose:     
 *     Check for a change in link status on each link.  If a change
 *     is detected, call bcm_port_update to program the MACs for that
 *     link, and call the list of registered handlers.
 * Parameters:  
 *     unit - Device unit number
 *     pbm  - Bitmap of ports to scan
 * Returns:
 *     BCM_E_XXX
 */
int 
bcm_common_linkscan_update(int unit, bcm_pbmp_t pbm)
{                       
    UNIT_INIT_CHECK(unit);

    if (BCM_PBMP_IS_NULL(pbm)) {
        return BCM_E_NONE;
    }

    _bcm_linkscan_update(unit, pbm);

    return BCM_E_NONE;
}

/*
 * Function:    
 *     bcm_linkscan_register
 * Purpose:
 *     Register a handler to be called when a link status change is noticed.
 * Parameters:
 *     unit - Device unit number
 *     f    - Pointer to function to call when link status change is seen
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_common_linkscan_register(int unit, bcm_linkscan_handler_t f)
{
    _ls_control_t     *lc;
    _ls_handler_cb_t  *lh;
    int               found = FALSE;

    UNIT_INIT_CHECK(unit);
    
    if (f == NULL){
        return BCM_E_PARAM;
    }
    
    LS_LOCK(unit);

    lc = LS_CONTROL(unit);

    /* First, see if this handler already registered */
    for (lh = lc->handler_cb; lh != NULL; lh = lh->next) {
        if (lh->cb_f == f) {
            found = TRUE;
            break;
        }
    }

    if (found) {
        LS_UNLOCK(unit);
        return BCM_E_NONE;
    }

    if ((lh = sal_alloc(sizeof(*lh), "bcm_linkscan_register")) == NULL) {
        LS_UNLOCK(unit);
        return BCM_E_MEMORY;
    }

    lh->next = lc->handler_cb;
    lh->cb_f = f;
    lc->handler_cb = lh;

    LS_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_linkscan_unregister
 * Purpose:
 *     Remove a previously registered handler from the callout list.
 * Parameters:
 *     unit - Device unit number
 *     f    - Pointer to function registered in call to 
 *            bcm_linkscan_register
 * Returns:
 *     BCM_E_NONE      - Success
 *     BCM_E_NOT_FOUND - Could not find matching handler
 */
int
bcm_common_linkscan_unregister(int unit, bcm_linkscan_handler_t f)
{
    _ls_control_t     *lc;
    _ls_handler_cb_t  *lh, *p;

    UNIT_INIT_CHECK(unit);

    LS_LOCK(unit);

    lc = LS_CONTROL(unit);

    for (p = NULL, lh = lc->handler_cb; lh; p = lh, lh = lh->next) { 
        if (lh->cb_f == f) {
            if (p == NULL) {
                lc->handler_cb = lh->next;
            } else {
                p->next = lh->next;
            }
            break;
        }
    }

    LS_UNLOCK(unit);

    if (lh == NULL) {
        return BCM_E_NOT_FOUND;
    }        
    
    sal_free(lh);

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_linkscan_port_register
 * Purpose:
 *     Register a handler to be called when a link status
 *     is to be determined by a caller provided function.
 * Parameters:
 *     unit - Device unit number
 *     port - Device port number
 *     f    - Pointer to function to call for true link status
 * Returns:
 *     BCM_E_XXX
 * Notes:
 *     This function works with software linkscan only.  
 */
int
bcm_common_linkscan_port_register(int unit, bcm_port_t port,
                                  bcm_linkscan_port_handler_t f)
{
    _ls_control_t  *lc;
  
    UNIT_INIT_CHECK(unit);

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_port_local_get(unit, port, &port));
    }

    if (!SOC_PORT_VALID(unit, port) || !IS_PORT(unit, port)) {
        return BCM_E_PORT;
    }

    LS_LOCK(unit);
    lc = LS_CONTROL(unit);
    lc->port_link_f[port] = f;
    LS_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:
 *     bcm_linkscan_port_unregister
 * Purpose:
 *     Remove a previously registered handler that is used
 *     for setting link status. 
 * Parameters:
 *     unit - Device unit number
 *     port - Device port number
 *     f    - Pointer to function registered in call to 
 *            bcm_linkscan_port_register
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_common_linkscan_port_unregister(int unit, bcm_port_t port,
                                    bcm_linkscan_port_handler_t f)
{
    _ls_control_t  *lc;

    UNIT_INIT_CHECK(unit);

    if (BCM_GPORT_IS_SET(port)) {
        BCM_IF_ERROR_RETURN(bcm_port_local_get(unit, port, &port));
    }

    if (!SOC_PORT_VALID(unit, port) || !IS_PORT(unit, port)) {
        return BCM_E_PORT;
    }

    LS_LOCK(unit);
    lc = LS_CONTROL(unit);
    lc->port_link_f[port] = NULL;
    LS_UNLOCK(unit);

    return BCM_E_NONE;
}

/*
 * Function:    
 *     bcm_link_wait
 * Purpose:
 *     Wait for all links in the mask to be "link up".
 * Parameters:  
 *     unit - Device unit number
 *     pbm  - (IN/OUT) Port bitmap to wait for, mask of those link up on 
 *            return
 *     us   - Number of microseconds to wait
 * Returns:
 *     BCM_E_NONE     - all links are ready.
 *     BCM_E_TIMEOUT  - not all links ready in specified time.
 *     BCM_E_DISABLED - linkscan not running on one or more of the ports.
 */
int
bcm_common_link_wait(int unit, pbmp_t *pbm, sal_usecs_t us)
{
    _ls_control_t   *lc;
    soc_timeout_t   to;
    pbmp_t          sofar_pbm;
    soc_port_t      port;

    UNIT_VALID_CHECK(unit);

    BCM_PBMP_ITER(*pbm, port) {
        BCM_IF_ERROR_RETURN
            (bcm_linkscan_enable_port_get(unit, port));
    }

    /*
     * If a port was just configured, it may have gone down but
     * pbm_link may not reflect that until the next time linkscan
     * runs.  This is avoided by forcing an update of pbm_link.
     */
    _bcm_linkscan_update(unit, *pbm);

    lc = LS_CONTROL(unit);

    soc_timeout_init(&to, us, 0);

    for (;;) {
        BCM_PBMP_ASSIGN(sofar_pbm, lc->pbm_link);
        BCM_PBMP_AND(sofar_pbm, *pbm);
        if (BCM_PBMP_EQ(sofar_pbm, *pbm)) {
            break;
        }

        if (soc_timeout_check(&to)) {
            BCM_PBMP_AND(*pbm, lc->pbm_link);
            return BCM_E_TIMEOUT;
        }

        sal_usleep(lc->interval_us / 4);
    }

    return BCM_E_NONE;
}

/*
 * Function:    
 *     bcm_link_change
 * Purpose:
 *     Force a transient link down event to be recognized,
 *     regardless of the current physical up/down state of the
 *     port.  This does not affect the physical link status. 
 * Parameters:  
 *     unit - Device unit number
 *     pbm  - Bitmap of ports to operate on
 * Returns:
 *     BCM_E_XXX
 */
int
bcm_common_link_change(int unit, pbmp_t pbm)
{
    _ls_control_t  *lc;

    UNIT_INIT_CHECK(unit);

    LS_LOCK(unit);

    lc = LS_CONTROL(unit);
    BCM_PBMP_AND(pbm, PBMP_PORT_ALL(unit));
    BCM_PBMP_OR(lc->pbm_link_change, pbm);

    LS_UNLOCK(unit);

    /*
     * Wake up master thread to notice changes - required if using hardware
     * link scanning.
     */
    if (lc->sema != NULL) {
        sal_sem_give(lc->sema);
    }

    return BCM_E_NONE;
}

#if defined(BROADCOM_DEBUG)
int
bcm_common_linkscan_dump(int unit)
{
    _ls_handler_cb_t  *ent;

    if (LS_CONTROL(unit) == NULL) {
        soc_cm_print("BCM linkscan not initialized for unit %d\n", unit);
        return BCM_E_PARAM;
    }

    LINK_OUT(("BCM linkscan callbacks for unit %d\n", unit));
    for (ent = LS_CONTROL(unit)->handler_cb; ent != NULL;
         ent = ent->next) {
#if !defined(__PEDANTIC__)
        LINK_OUT(("    Fn %p\n", (void *)ent->cb_f));
#else /* !defined(__PEDANTIC__) */
        LINK_OUT(("    Function pointer unprintable\n"));
#endif /* !defined(__PEDANTIC__) */
    }

    return BCM_E_NONE;
}
#endif  /* BROADCOM_DEBUG */

