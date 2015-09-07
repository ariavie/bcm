/*
 * $Id: fcmap_cmn.c 1.4 Broadcom SDK $
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
 * File:    fcmap_cmn.c
 * Purpose: FCMAP software module intergation support
 */

#ifdef INCLUDE_FCMAP

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/drv.h>
#include <soc/register.h>
#include <soc/memory.h>
#include <soc/phy.h>
#include <soc/ll.h>
#include <soc/ptable.h>
#include <soc/tucana.h>
#include <soc/firebolt.h>
#include <soc/xaui.h>
#include <soc/phyctrl.h>
#include <soc/phyreg.h>
#ifdef BCM_SBX_SUPPORT
#include <soc/sbx/sbx_drv.h>
#endif
#include <soc/fcmapphy.h>
#include <sal/core/libc.h>
#include <bcm/debug.h>

#include <bcm_int/common/fcmap_cmn.h>

#include <bcm_int/control.h>
#include <bcm_int/esw_dispatch.h>

#define UNIT_VALID(unit) \
{ \
  if (!BCM_UNIT_VALID(unit)) { return BCM_E_UNIT; } \
}

static bfcmap_error_t error_tbl[] = {
                                BCM_E_NONE,
                                BCM_E_INTERNAL,
                                BCM_E_MEMORY,
                                BCM_E_PARAM,
                                BCM_E_EMPTY,
                                BCM_E_FULL,
                                BCM_E_NOT_FOUND,
                                BCM_E_EXISTS,
                                BCM_E_TIMEOUT,
                                BCM_E_FAIL,
                                BCM_E_DISABLED,
                                BCM_E_BADID,
                                BCM_E_RESOURCE,
                                BCM_E_CONFIG,
                                BCM_E_UNAVAIL,
                                BCM_E_INIT,
                                BCM_E_PORT,
                            };
/*
 * Convert FCMAP return codes to BCM return code.
 */
#define BCM_ERROR(e)    error_tbl[-(e)]

typedef struct _bcm_fcmap_cb_args_s {
    void *user_arg;
    void *user_cb;
    int   unit;
    int   port;
} _bcm_fcmap_cb_args_t;

static int fcmap_cb_registered = 0;

/*
 * Function:
 *      bcm_common_fcmap_init
 * Purpose:
 *      Intiialize FCMAP for the specified unit.
 */
int _bcm_common_fcmap_init(int unit)
{
    static int _bfcmap_init_done = 0;

    /*
     * Do FCMAP init and MDIO registration for the FCMAP module.
     */
    if (!_bfcmap_init_done) {
        /* Initilaize the FCMAP module */
        bfcmap_init();
        /* register MDIO routines */
        bfcmap_dev_mmi_mdio_register(soc_fcmapphy_miim_read, 
                                      soc_fcmapphy_miim_write);
        /* Initialize the default print vectors. */
        bfcmap_config_print_set(BFCMAP_PRINT_PRINTF, soc_cm_print);
        _bfcmap_init_done = 1;
    }
    return BCM_E_NONE;
}

/*
 * Local port traverse handler to filter out ports belonging to specified
 * unit.
 */
STATIC int 
_bcm_fcmap_port_traverse_cb(bfcmap_port_t p, 
                             bfcmap_core_t dev_core, 
                             bfcmap_dev_addr_t dev_addr, 
                             int dev_port, 
                             bfcmap_dev_io_f devio_f, 
                             void *user_data)
{
    int rv = BFCMAP_E_NONE ;
    _bcm_fcmap_cb_args_t *ua;
    bcm_fcmap_port_traverse_cb callback;

    ua = (_bcm_fcmap_cb_args_t*) user_data;

    if (SOC_FCMAP_PORTID2UNIT(p) == ua->unit) {
        callback = (bcm_fcmap_port_traverse_cb) ua->user_cb;
        rv = callback(SOC_FCMAP_PORTID2UNIT(p), 
                 SOC_FCMAP_PORTID2PORT(p), 
                 dev_core, dev_addr, 
                 dev_port, devio_f, ua->user_arg);
    }
    return rv;
}

/*
 * Function:
 *      bcm_fcmap_port_traverse
 * Purpose:
 *      Destroy a FCMAP Port
 */
int 
bcm_common_fcmap_port_traverse(int unit, 
                             bcm_fcmap_port_traverse_cb callback, 
                             void *user_data)
{
    int rv = BCM_E_UNAVAIL;
    _bcm_fcmap_cb_args_t ua;

    UNIT_VALID(unit);

    ua.user_arg = user_data;
    ua.user_cb = (void*) callback;
    ua.unit = unit;

    rv = bfcmap_port_traverse(_bcm_fcmap_port_traverse_cb, (void*) &ua);
#ifdef BCM_CB_ABORT_ON_ERR
    return BCM_ERROR(rv);
#else
    return BCM_E_NONE;
#endif
}

/*
 * Function:
 *      bcm_fcmap_port_config_set
 * Purpose:
 *      Set the FCMAP Port configuration.
 */
int
bcm_common_fcmap_port_config_set(int unit, bcm_port_t port, 
                               bcm_fcmap_port_config_t *cfg)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_port_config_set(p, cfg);
    return BCM_ERROR(rv);
}

/*
 * Function:
 *      bcm_fcmap_port_config_get
 * Purpose:
 *      Get the current FCMAP Port configuration.
 */
int
bcm_common_fcmap_port_config_get(int unit, bcm_port_t port, 
                               bcm_fcmap_port_config_t *cfg)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_port_config_get(p, cfg);
    return BCM_ERROR(rv);
}

/*
 * Function:
 *      bcm_fcmap_port_speed_set
 * Purpose:
 *      Set speed attribute of the FC port.
 */
int
bcm_common_fcmap_port_speed_set(int unit, bcm_port_t port, 
                               bcm_fcmap_port_speed_t speed)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_port_speed_set(p, speed);
    return BCM_ERROR(rv);
}

/*
 * Function:
 *      bcm_fcmap_port_enable
 * Purpose:
 *      Enable the FC port
 */
int
bcm_common_fcmap_port_enable(int unit, bcm_port_t port)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_port_link_enable(p);
    return BCM_ERROR(rv);
}

/*
 * Function:
 *      bcm_fcmap_port_shutdown
 * Purpose:
 *      Disable FC port
 */
int
bcm_common_fcmap_port_shutdown(int unit, bcm_port_t port)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_port_shutdown(p);
    return BCM_ERROR(rv);
}

/*
 * Function:
 *      bcm_fcmap_port_link_reset
 * Purpose:
 *      Issue a link reset on the FC port.
 */
int
bcm_common_fcmap_port_link_reset(int unit, bcm_port_t port)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_port_reset(p);
    return BCM_ERROR(rv);
}
/*
 * Function:
 *      bcm_fcmap_vlan_map_add
 * Purpose:
 *      Add entry to VLAN - VSAN MAP for FCMAP port
 */
int
bcm_common_fcmap_vlan_map_add(int unit, bcm_port_t port, 
                               bcm_fcmap_vlan_vsan_map_t *vlan)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_vlan_map_add(p, vlan);
    return BCM_ERROR(rv);
}

/*
 * Function:
 *      bcm_fcmap_vlan_map_get
 * Purpose:
 *      Get entry from VLAN - VSAN MAP for FCMAP port
 */
int
bcm_common_fcmap_vlan_map_get(int unit, bcm_port_t port, 
                               bcm_fcmap_vlan_vsan_map_t *vlan)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_vlan_map_get(p, vlan);
    return BCM_ERROR(rv);
}

/*
 * Function:
 *      bcm_fcmap_vlan_map_delete
 * Purpose:
 *      Delete entry from VLAN - VSAN MAP for FCMAP port
 */
int
bcm_common_fcmap_vlan_map_delete(int unit, bcm_port_t port, 
                               bcm_fcmap_vlan_vsan_map_t *vlan)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_vlan_map_delete(p, vlan);
    return BCM_ERROR(rv);
}

#if 0
/*
 * Function:
 *      bcm_fcmap_port_capability_get
 * Purpose:
 *      Get the FCMAP Port capability
 */
int
bcm_common_fcmap_port_capability_get(int unit, bcm_port_t port, 
                               bcm_fcmap_port_capability_t *cap)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_port_capability_get(p, cap);
    return BCM_ERROR(rv);
}
#endif


/*
 * Function:
 *      bcm_fcmap_stat_clear
 * Purpose:
 *      Clear stats for the specified port.
 */
int 
bcm_common_fcmap_stat_clear(int unit, bcm_port_t port)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_stat_clear(p);
    return BCM_ERROR(rv);
}

int 
bcm_common_fcmap_stat_get(int unit, bcm_port_t port, bcm_fcmap_stat_t stat, 
                    uint64 *val)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_stat_get(p, stat, (buint64_t *)val);
    return BCM_ERROR(rv);
}

int 
bcm_common_fcmap_stat_get32(int unit, bcm_port_t port, 
                          bcm_fcmap_stat_t stat, uint32 *val)
{
    int rv = BCM_E_UNAVAIL;
    bfcmap_port_t p;

    UNIT_VALID(unit);

    p = SOC_FCMAP_PORTID(unit, port);
    rv = bfcmap_stat_get32(p, stat, (buint32_t*)val);
    return BCM_ERROR(rv);
}


typedef struct _bcm_fcmap_event_data_s {
    bcm_fcmap_event_cb callback;
    uint32              event_map;
} _bcm_fcmap_event_data_t;

STATIC _bcm_fcmap_event_data_t _fcmap_event_data[BCM_UNITS_MAX];

STATIC int
_bcm_fcmap_event_cb(bfcmap_port_t p,      /* Port ID          */
                     bfcmap_event_t event, /* Event            */
                     void *user_data)
{
    int     unit;
    bcm_fcmap_event_cb callback;
    _bcm_fcmap_event_data_t *event_data;

    unit = SOC_FCMAP_PORTID2UNIT(p);

    event_data = &_fcmap_event_data[unit];
    callback = (bcm_fcmap_event_cb) event_data->callback;

    if (callback && (event_data->event_map & (1 << (uint32) event))) {
        callback(SOC_FCMAP_PORTID2UNIT(p), 
                 SOC_FCMAP_PORTID2PORT(p), 
                 event, user_data);
    }
    return BFCMAP_E_NONE;
}

int 
bcm_common_fcmap_event_register(int unit, bcm_fcmap_event_cb cb, void *user_data)
{
    int rv = BCM_E_NONE;
    _bcm_fcmap_event_data_t *event_data;

    UNIT_VALID(unit);

    event_data = &_fcmap_event_data[unit];
    event_data->callback = cb;
    event_data->event_map = 0;
    if (fcmap_cb_registered == 0) {
        rv = bfcmap_event_register(_bcm_fcmap_event_cb, user_data);
        if (rv == BFCMAP_E_NONE) {
            fcmap_cb_registered = 1;
        }
    }
    return BCM_ERROR(rv);
}

int 
bcm_common_fcmap_event_unregister(int unit, bcm_fcmap_event_cb cb)
{
    int rv = BCM_E_NONE, unregister = 1, ii;
    _bcm_fcmap_event_data_t *event_data;

    UNIT_VALID(unit);

    event_data = &_fcmap_event_data[unit];
    if (event_data->callback) {
        event_data->callback = NULL;
    }

    for (ii = 0; ii < BCM_UNITS_MAX; ii++) {
        event_data = &_fcmap_event_data[ii];
        if (event_data->callback) {
            unregister = 0;
            break;
        }
    }
    if (unregister) {
        fcmap_cb_registered = 0;
        rv = bfcmap_event_unregister(_bcm_fcmap_event_cb);
    }
    return BCM_ERROR(rv);
}

int
bcm_common_fcmap_event_enable_set(int unit,
                                bcm_fcmap_event_t event, int enable)
{
    int rv = BCM_E_NONE, disable, ii;
    _bcm_fcmap_event_data_t *event_data;

    UNIT_VALID(unit);

    event_data = &_fcmap_event_data[unit];
    if (enable) {
        event_data->event_map |= (1 << (uint32) event);
        rv = bfcmap_event_enable_set(event, enable);
    } else {
        event_data->event_map &= ~(1 << (uint32) event);
        disable = 1;
        for (ii = 0; ii < BCM_UNITS_MAX; ii++) {
            event_data = &_fcmap_event_data[ii];
            if (event_data->event_map & (1 << (uint32) event)) {
                disable = 0;
                break;
            }
        }
        if (disable) {
            rv = bfcmap_event_enable_set(event, enable);
        }
    }
    return BCM_ERROR(rv);
}

int
bcm_common_fcmap_event_enable_get(int unit,
                                bcm_fcmap_event_t event, int *enable)
{
    int rv = BCM_E_NONE;
    _bcm_fcmap_event_data_t *event_data;

    UNIT_VALID(unit);

    event_data = &_fcmap_event_data[unit];
    *enable = (event_data->event_map & (1 << (uint32) event)) ? 1 : 0;
    return BCM_ERROR(rv);
}


/*
 * Configure FCMAP print function for debug.
 */
int bcm_common_fcmap_config_print(uint32 level)
{
    int rv = BCM_E_NONE;
    uint32  flag = BFCMAP_PRINT_PRINTF;

#if defined(BROADCOM_DEBUG)
    if (level & BCM_DBG_MACSEC) {
        flag |= BFCMAP_PRINT_DEBUG;
    }
#endif /* BROADCOM_DEBUG */

    rv = bfcmap_config_print_set(flag, soc_cm_print);
    return BCM_ERROR(rv);
}


#else
int _fcmap_cmn_not_empty;
#endif /* INCLUDE_FCMAP */

