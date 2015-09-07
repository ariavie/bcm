/*
 * $Id: timesync.c,v 1.3 Broadcom SDK $
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
 * StrataSwitch port control API
 */
#include <shared/bsl.h>

#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>

#include <soc/drv.h>
#include <soc/phy/drv.h>
#include <soc/debug.h>
#include <soc/timesync.h>

/*
 * Function:
 *      soc_port_phy_timesync_config_set
 * Purpose:
 *      Update PHY's phy_timesync config
 * Parameters:
 *      unit - SOC Unit #.
 *      port - Port number.
 *      conf - Config
 * Returns:
 *      SOC_E_XXX
 */
int
soc_port_phy_timesync_config_set(int unit, soc_port_t port, soc_port_phy_timesync_config_t *conf)
{
    int         rv;
    phy_ctrl_t *pc;

    rv = SOC_E_UNAVAIL;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (NULL != pc) {
        INT_MCU_LOCK(unit);
        rv = PHY_TIMESYNC_CONFIG_SET(pc->pd, unit, port, conf);
        INT_MCU_UNLOCK(unit);
    }

    if (SOC_FAILURE(rv)) {
        LOG_WARN(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "soc_port_phy_timesync_config_set failed %d\n"), rv));
    }
    return rv;
}

/*
 * Function:
 *      soc_port_phy_timesync_config_get
 * Purpose:
 *      Retrive PHY's phy_timesync config
 * Parameters:
 *      unit - SOC Unit #.
 *      port - Port number.
 *      conf - Config
 * Returns:
 *      SOC_E_XXX
 */
int
soc_port_phy_timesync_config_get(int unit, soc_port_t port, soc_port_phy_timesync_config_t *conf)
{
    int         rv;
    phy_ctrl_t *pc;

    rv = SOC_E_UNAVAIL;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (NULL != pc) {
        INT_MCU_LOCK(unit);
        rv = PHY_TIMESYNC_CONFIG_GET(pc->pd, unit, port, conf);
        INT_MCU_UNLOCK(unit);
    }

    if (SOC_FAILURE(rv)) {
        LOG_WARN(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "soc_port_phy_timesync_config_get failed %d\n"), rv));
    }
    return rv;
}

/*
 * Function:
 *      soc_port_control_phy_timesync_set
 * Purpose:
 *      Update PHY's phy_timesync parameter
 * Parameters:
 *      unit - SOC Unit #.
 *      port - Port number.
 *      type - operation
 *      value - parameter
 * Returns:
 *      SOC_E_XXX
 */
int
soc_port_control_phy_timesync_set(int unit, soc_port_t port, soc_port_control_phy_timesync_t type, uint64 value)
{
    int         rv;
    phy_ctrl_t *pc;

    rv = SOC_E_UNAVAIL;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (NULL != pc) {
        INT_MCU_LOCK(unit);
        rv = PHY_TIMESYNC_CONTROL_SET(pc->pd, unit, port, type, value);
        INT_MCU_UNLOCK(unit);
    }

    if (SOC_FAILURE(rv)) {
        LOG_WARN(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "soc_port_control_phy_timesync_set failed %d\n"), rv));
    }
    return rv;
}

/*
 * Function:
 *      soc_port_control_phy_timesync_get
 * Purpose:
 *      Retrive PHY's phy_timesync parameter
 * Parameters:
 *      unit - SOC Unit #.
 *      port - Port number.
 *      type - operation
 *      value - *parameter
 * Returns:
 *      SOC_E_XXX
 */
int
soc_port_control_phy_timesync_get(int unit, soc_port_t port, soc_port_control_phy_timesync_t type, uint64 *value)
{
    int         rv;
    phy_ctrl_t *pc;

    rv = SOC_E_UNAVAIL;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (NULL != pc) {
        INT_MCU_LOCK(unit);
        rv = PHY_TIMESYNC_CONTROL_GET(pc->pd, unit, port, type, value);
        INT_MCU_UNLOCK(unit);
    }

    if (SOC_FAILURE(rv)) {
        LOG_WARN(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "soc_port_control_phy_timesync_get failed %d\n"), rv));
    }
    return rv;
}

/*
 * Function:
 *      soc_port_phy_timesync_enhanced_capture_get
 * Purpose:
 *      Retrive PHY's phy_timesync parameter
 * Parameters:
 *      unit - SOC Unit #.
 *      port - Port number.
 *      type - operation
 *      value - *parameter
 * Returns:
 *      SOC_E_XXX
 */
int
soc_port_phy_timesync_enhanced_capture_get(int unit, soc_port_t port, soc_port_phy_timesync_enhanced_capture_t *value)
{
    int         rv;
    phy_ctrl_t *pc;

    rv = SOC_E_UNAVAIL;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (NULL != pc) {
        INT_MCU_LOCK(unit);
        rv = PHY_TIMESYNC_ENHANCED_CAPTURE_GET(pc->pd, unit, port, value);
        INT_MCU_UNLOCK(unit);
    }

    if (SOC_FAILURE(rv)) {
        LOG_WARN(BSL_LS_SOC_COMMON,
                 (BSL_META_U(unit,
                             "soc_port_phy_timesync_enhanced_capture_get failed %d\n"), rv));
    }
    return rv;
}
