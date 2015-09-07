/*
 * $Id: phy54616.c 1.10 Broadcom SDK $
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
 * File:        phy54616.c
 * Purpose:     PHY driver for BCM54616 and BCM54616S
 *
 */                        

#include <sal/types.h>
#include <sal/core/spl.h>

#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/phyreg.h>

#include <soc/phy.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>

#include "phydefs.h"      /* Must include before other phy related includes */

#if defined(INCLUDE_PHY_54616)
#include "phyconfig.h"    /* Must be the first phy include after phydefs.h */
#include "phyident.h"
#include "phyreg.h"
#include "phyfege.h"
#include "phy54616.h"

#define AUTO_MDIX_WHEN_AN_DIS   0       /* Non-standard-compliant option */
#define DISABLE_CLK125          0       /* Disable if unused to reduce */
                                        /*  EMI emissions */

#define PHY_IS_BCM54616S(_pc)  PHY_FLAGS_TST((_pc)->unit, (_pc)->port, \
                                                        PHY_FLAGS_SECONDARY_SERDES)

STATIC int _phy_54616_no_reset_setup(int unit, soc_port_t port);
STATIC int _phy_54616_medium_change(int unit, soc_port_t port,int force_update);
STATIC int _phy_54616_copper_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability);
STATIC int _phy_54616_fiber_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability);
STATIC int _phy_54616_fiber_ability_advert_set(int unit, soc_port_t port,soc_port_ability_t *ability);
STATIC int phy_54616_duplex_set(int unit, soc_port_t port, int duplex);
STATIC int phy_54616_speed_set(int unit, soc_port_t port, int speed);
STATIC int phy_54616_master_set(int unit, soc_port_t port, int master);
STATIC int phy_54616_autoneg_set(int unit, soc_port_t port, int autoneg);
STATIC int phy_54616_mdix_set(int unit, soc_port_t port, soc_port_mdix_t mode);
STATIC int phy_54616_speed_get(int unit, soc_port_t port, int *speed);
STATIC int phy_54616_ability_advert_set(int unit, soc_port_t port, soc_port_ability_t *ability);

/***********************************************************************
 *
 * HELPER FUNCTIONS
 *
 ***********************************************************************/
/*
 * Function:
 *      _phy_54616_medium_check
 * Purpose:
 *      Determine if chip should operate in copper or fiber mode.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      medium - (OUT) SOC_PORT_MEDIUM_XXX
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
_phy_54616_medium_check(int unit, soc_port_t port, int *medium)
{
    phy_ctrl_t    *pc = EXT_PHY_SW_STATE(unit, port);
    uint16         tmp = 0xffff;

    *medium = SOC_PORT_MEDIUM_COPPER;

    if (PHY_PRIMARY_SERDES_MODE(unit, port)) {
        /* Ie fiber capable */

        if (pc->automedium) {
            /* Read Mode Register (0x1c shadow 11111) */
            SOC_IF_ERROR_RETURN
                (READ_PHY54616_MODE_CTRLr(unit, pc, &tmp));

            if (pc->fiber.preferred) {
                /* 0x10 Fiber Signal Detect
                 * 0x20 Copper Energy Detect
                 */
                if ((tmp & 0x30) == 0x20) {
                    /* Only copper is link up */
                    *medium = SOC_PORT_MEDIUM_COPPER;
                } else {
                    *medium = SOC_PORT_MEDIUM_FIBER;
                }
            } else {
                if ((tmp & 0x30) == 0x10) {
                    /* Only fiber is link up */
                    *medium = SOC_PORT_MEDIUM_FIBER;
                } else {
                    *medium = SOC_PORT_MEDIUM_COPPER;
                }
            }
        } else {
            *medium = pc->fiber.preferred ? SOC_PORT_MEDIUM_FIBER:
                      SOC_PORT_MEDIUM_COPPER;
        } 
    }

    SOC_DEBUG_PRINT((DK_PHY | DK_VERBOSE,
                     "_phy_54616_medium_check: "
                     "u=%d p=%d fiber_pref=%d 0x1c(11111)=%04x fiber=%d\n",
                     unit, port, pc->fiber.preferred, tmp, 
                     (*medium == SOC_PORT_MEDIUM_FIBER) ? 1 : 0));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_medium_status
 * Purpose:
 *      Indicate the current active medium
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      medium - (OUT) One of:
 *              SOC_PORT_MEDIUM_COPPER
 *              SOC_PORT_MEDIUM_FIBER
 * Returns:
 *      SOC_E_NONE
 */

STATIC int
phy_54616_medium_status(int unit, soc_port_t port, soc_port_medium_t *medium)
{
    if (PHY_COPPER_MODE(unit, port)) {
        *medium = SOC_PORT_MEDIUM_COPPER;
    } else {
        *medium = SOC_PORT_MEDIUM_FIBER;
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_54616_medium_config_update
 * Purpose:
 *      Update the PHY with config parameters
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      cfg - Config structure.
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
_phy_54616_medium_config_update(int unit, soc_port_t port,
                                soc_phy_config_t *cfg)
{
    SOC_IF_ERROR_RETURN
        (phy_54616_speed_set(unit, port, cfg->force_speed));
    SOC_IF_ERROR_RETURN
        (phy_54616_duplex_set(unit, port, cfg->force_duplex));
    SOC_IF_ERROR_RETURN
        (phy_54616_master_set(unit, port, cfg->master));
    SOC_IF_ERROR_RETURN
        (phy_54616_ability_advert_set(unit, port, &cfg->advert_ability));
    SOC_IF_ERROR_RETURN
        (phy_54616_autoneg_set(unit, port, cfg->autoneg_enable));
    SOC_IF_ERROR_RETURN
        (phy_54616_mdix_set(unit, port, cfg->mdix));

    return SOC_E_NONE;
}

/***********************************************************************
 *
 * PHY54616 DRIVER ROUTINES
 *
 ***********************************************************************/

/*
 * Function:
 *      phy_54616_medium_config_set
 * Purpose:
 *      Set the operating parameters that are automatically selected
 *      when medium switches type.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      port - Port number
 *      medium - SOC_PORT_MEDIUM_COPPER/FIBER
 *      cfg - Operating parameters
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
phy_54616_medium_config_set(int unit, soc_port_t port, 
                           soc_port_medium_t  medium,
                           soc_phy_config_t  *cfg)
{
    phy_ctrl_t    *pc;
    soc_phy_config_t *active_medium;  /* Currently active medium */
    soc_phy_config_t *change_medium;  /* Requested medium */
    soc_phy_config_t *other_medium;   /* The other medium */
    int               medium_update;

    if (NULL == cfg) {
        return SOC_E_PARAM;
    }

    pc            = EXT_PHY_SW_STATE(unit, port);
    medium_update = FALSE;

    switch (medium) {
    case SOC_PORT_MEDIUM_COPPER:
        if (!pc->automedium) {
            if (!PHY_COPPER_MODE(unit, port)) {
                return SOC_E_UNAVAIL;
            }
            /* check if device is fiber capable before switching */
            if (cfg->preferred == 0) {

                if (!PHY_IS_BCM54616S(pc)) {
                    /* return if not fiber capable*/
                    return SOC_E_UNAVAIL;
                }
            }
        }
        
        change_medium  = &pc->copper;
        other_medium   = &pc->fiber;
        break;
    case SOC_PORT_MEDIUM_FIBER:
        if (!pc->automedium && !PHY_FIBER_MODE(unit, port)) {
            return SOC_E_UNAVAIL;
        }
        change_medium  = &pc->fiber;
        other_medium   = &pc->copper;
        break;
    default:
        return SOC_E_PARAM;
    }

    /*
     * Changes take effect immediately if the target medium is active or
     * the preferred medium changes.
     */
    if (change_medium->enable != cfg->enable) {
        medium_update = TRUE;
    }
    if (change_medium->preferred != cfg->preferred) {
        /* Make sure that only one medium is preferred */
        other_medium->preferred = !cfg->preferred;
        medium_update = TRUE;
    }

    sal_memcpy(change_medium, cfg, sizeof(*change_medium));

    if (medium_update) {
        /* The new configuration may cause medium change. Check
         * and update medium.
         */
        SOC_IF_ERROR_RETURN
            (_phy_54616_medium_change(unit, port,TRUE));
    } else {
        active_medium = (PHY_COPPER_MODE(unit, port)) ?  
                            &pc->copper : &pc->fiber;
        if (active_medium == change_medium) {
            /* If the medium to update is active, update the configuration */
            SOC_IF_ERROR_RETURN
                (_phy_54616_medium_config_update(unit, port, change_medium));
        }
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_medium_config_get
 * Purpose:
 *      Get the operating parameters that are automatically selected
 *      when medium switches type.
 * Parameters:
 *      unit - StrataSwitch unit #
 *      port - Port number
 *      medium - SOC_PORT_MEDIUM_COPPER/FIBER
 *      cfg - (OUT) Operating parameters
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
phy_54616_medium_config_get(int unit, soc_port_t port, 
                           soc_port_medium_t medium,
                           soc_phy_config_t *cfg)
{
    int            rv;
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    switch (medium) {
    case SOC_PORT_MEDIUM_COPPER:
        if (pc->automedium || PHY_COPPER_MODE(unit, port)) {
            sal_memcpy(cfg, &pc->copper, sizeof (*cfg));
        } else {
            rv = SOC_E_UNAVAIL;
        }
        break;
    case SOC_PORT_MEDIUM_FIBER:
        if (pc->automedium || PHY_FIBER_MODE(unit, port)) {
            sal_memcpy(cfg, &pc->fiber, sizeof (*cfg));
        } else {
            rv = SOC_E_UNAVAIL;
        }
        break;
    default:
        rv = SOC_E_PARAM;
        break;
    }

    return rv;
}

/*
 * Function:
 *      _phy_54616_reset_setup
 * Purpose:
 *      Function to reset the PHY and set up initial operating registers.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      Configures for copper/fiber according to the current
 *      setting of PHY_FLAGS_FIBER.
 */

STATIC int
_phy_54616_reset_setup(int unit, soc_port_t port)
{
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* Reset the PHY with previously registered callback function. 
     */
    SOC_IF_ERROR_RETURN(soc_phy_reset(unit, port));

    sal_usleep(10000);

    /* Disable super-isolate */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54616_MII_POWER_CTRLr(unit, pc, 0x0, 1U<<5));

    SOC_IF_ERROR_RETURN
        (_phy_54616_no_reset_setup(unit, port));

    return SOC_E_NONE;
}

STATIC int
_phy_54616_no_reset_setup(int unit, soc_port_t port)
{
    phy_ctrl_t    *int_pc;
    phy_ctrl_t    *pc;
    uint16         data, mask;

    SOC_DEBUG_PRINT((DK_PHY, "_phy_54616_reset_setup: u=%d p=%d medium=%s\n",
                     unit, port,
                     PHY_COPPER_MODE(unit, port) ? "COPPER" : "FIBER"));

    pc      = EXT_PHY_SW_STATE(unit, port);
    int_pc  = INT_PHY_SW_STATE(unit, port);


    /* copper regs */
    if (!pc->copper.enable || (!pc->automedium && pc->fiber.preferred)) {
        /* Copper interface is not used. Powered down. */
        SOC_DEBUG_PRINT((DK_PHY, "%s: u=%d p=%d Copper PowerDown=%d!\n",
                        FUNCTION_NAME(), unit, port, 1));
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54616_MII_CTRLr(unit, pc, MII_CTRL_PD, MII_CTRL_PD));
    } else {
        SOC_DEBUG_PRINT((DK_PHY, "%s: u=%d p=%d Copper PowerDown=%d!\n",
                        FUNCTION_NAME(), unit, port, 0));
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54616_MII_CTRLr(unit, pc, 0, MII_CTRL_PD));
        SOC_IF_ERROR_RETURN
            (WRITE_PHY54616_MII_GB_CTRLr(unit, pc,
                            MII_GB_CTRL_ADV_1000FD | MII_GB_CTRL_PT));
        SOC_IF_ERROR_RETURN
            (WRITE_PHY54616_MII_CTRLr(unit, pc,
                MII_CTRL_FD | MII_CTRL_SS_1000 | MII_CTRL_AE));
    }

    if (NULL != int_pc) {
        SOC_IF_ERROR_RETURN
            (PHY_INIT(int_pc->pd, unit, port));
    }

    /* Configure Auto-detect Medium (0x1c shadow 11110) */
    mask = 0x33f;               /* Auto-detect bit mask */

    /* In BCM54616S the SD pin is internally tied. For proper operation
       SD_INV and QUALIFY_SD_WITH_SYNC should be set.                  */
    data = (1 << 8) | (1 << 5);

    if (pc->automedium) {
        data |= 1 << 0;    /* Enable auto-detect medium */
    }
    /* When no link partner exists, fiber should be default medium */
    /* When both link partners exipc, fiber should be prefered medium
     * for the following reasons.
     * 1. Most fiber module generates signal detect when the fiber cable is
     *    plugged in regardless of any activity even if the link partner is
     *    powered down.
     * 2. Fiber mode consume much lesser power than copper mode.
     */
    if (pc->fiber.preferred) {
        data |= 1 << 1;    /* Fiber selected when both media are active */
        data |= 1 << 2;    /* Fiber selected when no medium is active */
    }

    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54616_AUTO_DETECT_MEDIUMr(unit, pc, data, mask));

#if DISABLE_CLK125
    /* Reduce EMI emissions by disabling the CLK125 pin if not used  */
    data = 0x001c;  /* When write, bit 9 and 7 must be zero and */ 
    data = 0x029c;  /* bit 2, 3, and 4 must be one. */ 
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54616_SPARE_CTRL_3r(unit, pc, 0, 1));
#endif

#if AUTO_MDIX_WHEN_AN_DIS
    /* Enable Auto-MDIX When autoneg disabled */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54616_MII_MISC_CTRLr(unit, pc, 0x200, 0x200));
#endif
    /*
     * Configure Auxiliary 1000BASE-X control register to turn off
     * carrier extension.  The Intel 7131 NIC does not accept carrier
     * extension and gets CRC errors.
     */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54616_AUX_1000X_CTRLr(unit, pc, 0x0040, 0x0040));

    /* Configure extended control register for led mode and jumbo frame support
     */
    mask  = (1 << 5) | (1 << 0);

    /* if using led link/activity mode, disable led traffic mode */
    if (pc->ledctrl & 0x10 || pc->ledselect == 0x01) {
        data = 0;
    } else {
        data  = 1 << 5;      /* Enable LEDs to indicate traffic status */
    }
    /* Configure Extended Control Register for Jumbo frames support */
    data |= 1 << 0;      /* Set high FIFO elasticity to support jumbo frames */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54616_MII_ECRr(unit, pc, data, mask));

    /* Enable extended packet length (4.5k through 25k) */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54616_MII_AUX_CTRLr(unit, pc, 0x4000, 0x4000));

    /* Configure LED selectors */
    data = ((pc->ledmode[1] & 0xf) << 4) | (pc->ledmode[0] & 0xf);
    SOC_IF_ERROR_RETURN
        (WRITE_PHY54616_LED_SELECTOR_1r(unit, pc, data));

    data = ((pc->ledmode[3] & 0xf) << 4) | (pc->ledmode[2] & 0xf);
    SOC_IF_ERROR_RETURN
        (WRITE_PHY54616_LED_SELECTOR_2r(unit, pc, data));

    data = (pc->ledctrl & 0x3ff);
    SOC_IF_ERROR_RETURN
        (WRITE_PHY54616_LED_CTRLr(unit, pc, data));

    SOC_IF_ERROR_RETURN
        (WRITE_PHY54616_EXP_LED_SELECTORr(unit, pc, pc->ledselect));

   return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_init
 * Purpose:
 *      Init function for 54616 PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      SOC_E_NONE
 */

STATIC int
phy_54616_init(int unit, soc_port_t port)
{
    phy_ctrl_t *pc;
    int        fiber_capable;
    int        fiber_preferred;
    uint16     data;

    SOC_DEBUG_PRINT((DK_PHY, "phy_54616_init: u=%d p=%d\n",
                     unit, port));

    pc = EXT_PHY_SW_STATE(unit, port);

    if ( PHY_IS_BCM54616S(pc) ) {
        /* check the for the precense of the SERDES block and and read
          the INTF_SEL[1:0] strap values */

        SOC_IF_ERROR_RETURN
            (READ_PHY54616_MODE_CTRLr(unit, pc, &data));

        if ((data & (0x2 << 1)) == (0x2 << 1)) {
            pc->interface = SOC_PORT_IF_SGMII;
            fiber_capable = FALSE;
        } else {
            /* pc->interface = SOC_PORT_IF_RGMII; */
            pc->interface = SOC_PORT_IF_GMII; /* MAC unable to handle SOC_PORT_IF_RGMII */
            PHY_FLAGS_SET(pc->unit, pc->port, PHY_FLAGS_PRIMARY_SERDES);
            fiber_capable = TRUE;
        }
    } else {
        pc->interface = SOC_PORT_IF_GMII;
        fiber_capable = FALSE;
    }

    /*
     * In automedium mode, the property phy_fiber_pref_port<X> is used
     * to set a preference for fiber mode in the event both copper and
     * fiber links are up.
     *
     * If NOT in automedium mode, phy_fiber_pref_port<X> is used to
     * select which of fiber or copper is forced.
     */
    if (!fiber_capable) {
        fiber_preferred = 0;
        pc->automedium      = 0;
    } else {
        fiber_preferred =
            soc_property_port_get(unit, port, spn_PHY_FIBER_PREF, 1);
        pc->automedium =
            soc_property_port_get(unit, port, spn_PHY_AUTOMEDIUM, 1);
    }

    SOC_DEBUG_PRINT((DK_PHY, "phy_54616_init: "
            "u=%d p=%d type=54616%s automedium=%d fiber_pref=%d\n",
            unit, port, fiber_capable ? "S" : "",
            pc->automedium, fiber_preferred));

    pc->copper.enable = TRUE;
    pc->copper.preferred = !fiber_preferred;
    pc->copper.autoneg_enable = TRUE;
    pc->copper.force_speed = 1000;
    pc->copper.force_duplex = TRUE;
    pc->copper.master = SOC_PORT_MS_AUTO;
    pc->copper.mdix = SOC_PORT_MDIX_AUTO;

    pc->fiber.enable = fiber_capable;
    pc->fiber.preferred = fiber_preferred;
    pc->fiber.autoneg_enable = fiber_capable;
    pc->fiber.force_speed = 1000;
    pc->fiber.force_duplex = TRUE;
    pc->fiber.master = SOC_PORT_MS_NONE;
    pc->fiber.mdix = SOC_PORT_MDIX_NORMAL;

    SOC_IF_ERROR_RETURN
        (_phy_54616_copper_ability_local_get(unit, port, &pc->copper.advert_ability));
    if (fiber_capable) {
        (_phy_54616_fiber_ability_local_get(unit, port, &pc->fiber.advert_ability));
    }

    /* Initially configure for the preferred medium. */
    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_COPPER);
    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_FIBER);
    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_PASSTHRU);
    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_100FX);
    if (pc->fiber.preferred) {
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_FIBER);
    } else {
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_COPPER);
    }

    /* Get Requested LED selectors (defaults are hardware defaults) */
    pc->ledmode[0] = soc_property_port_get(unit, port, spn_PHY_LED1_MODE, 0);
    pc->ledmode[1] = soc_property_port_get(unit, port, spn_PHY_LED2_MODE, 1);
    pc->ledmode[2] = soc_property_port_get(unit, port, spn_PHY_LED3_MODE, 3);
    pc->ledmode[3] = soc_property_port_get(unit, port, spn_PHY_LED4_MODE, 6);
    pc->ledctrl = soc_property_port_get(unit, port, spn_PHY_LED_CTRL, 0x8);
    pc->ledselect = soc_property_port_get(unit, port, spn_PHY_LED_SELECT, 0);

    SOC_IF_ERROR_RETURN
        (_phy_54616_reset_setup(unit, port));

    SOC_IF_ERROR_RETURN
        (_phy_54616_medium_config_update(unit, port,
                                        PHY_COPPER_MODE(unit, port) ?
                                        &pc->copper :
                                        &pc->fiber));
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_enable_set
 * Purpose:
 *      Enable or disable the physical interface.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      enable - Boolean, true = enable PHY, false = disable.
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
phy_54616_enable_set(int unit, soc_port_t port, int enable)
{
    uint16         power;
    phy_ctrl_t    *pc;

    pc     = EXT_PHY_SW_STATE(unit, port);
    power  = (enable) ? 0 : MII_CTRL_PD;

    /* Do not incorrectly Power up the interface if medium is disabled and 
     * global enable = true
     */
    if (pc->copper.enable && (pc->automedium || PHY_COPPER_MODE(unit, port))) {
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54616_MII_CTRLr(unit, pc, power, MII_CTRL_PD));

        SOC_DEBUG_PRINT((DK_PHY, "phy_54616_enable_set: "
                "Power %s copper medium\n", (enable) ? "up" : "down"));
    }

    if (pc->fiber.enable && (pc->automedium || PHY_FIBER_MODE(unit, port))) {
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54616_1000X_MII_CTRLr(unit, pc, power, 
                                                   MII_CTRL_PD));

        SOC_DEBUG_PRINT((DK_PHY, "phy_54616_enable_set: "
                "Power %s fiber medium\n", (enable) ? "up" : "down"));
    }

    /* Update software state */
    SOC_IF_ERROR_RETURN(phy_fe_ge_enable_set(unit, port, enable));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_enable_get
 * Purpose:
 *      Enable or disable the physical interface for a 54616 device.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      enable - (OUT) Boolean, true = enable PHY, false = disable.
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
phy_54616_enable_get(int unit, soc_port_t port, int *enable)
{
    return phy_fe_ge_enable_get(unit, port, enable);
}

/*
 * Function:
 *      _phy_54616_100fx_setup
 * Purpose:
 *      Switch from 1000X mode to 100FX.  
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
_phy_54616_fiber_100fx_setup(int unit, soc_port_t port) 
{
    phy_ctrl_t    *pc;
    uint16        data;

    pc          = EXT_PHY_SW_STATE(unit, port);

    SOC_DEBUG_PRINT((DK_PHY | DK_VERBOSE, 
                     "_phy_54616_1000x_to_100fx: u=%d p=%d \n",
                     unit, port));

    /* clear the power down bit of the SerDes */
    SOC_DEBUG_PRINT((DK_PHY,
            "%s:p=%d,Pri-SerDes mode. Fiber PowerDown(%s)!\n",
            FUNCTION_NAME(), port, "No"));
    SOC_IF_ERROR_RETURN(
        MODIFY_PHY54616_1000X_MII_CTRLr(unit, pc, 0, MII_CTRL_PD));

    /* set to fiber mode */ /* bit[2:1]=1 */
    SOC_IF_ERROR_RETURN(
        MODIFY_PHY54616_MODE_CTRLr(unit, pc, 0x2,0x6));

    /* set loopback */
    SOC_IF_ERROR_RETURN(
        MODIFY_PHY54616_1000X_MII_CTRLr(unit, pc, MII_CTRL_LE, MII_CTRL_LE));

    /* select 100FX mode */
        /* bit[0]=1 */
    data = 1;
    data |= pc->fiber.force_duplex? PHY_54616_PRIMARY_SERDES_100FX_FD:0;

    SOC_IF_ERROR_RETURN(
        MODIFY_PHY54616_100FX_CTRLr(unit, pc, data,
          1 | PHY_54616_PRIMARY_SERDES_100FX_FD));

    /* Power down SerDes RX path (Expention register)*/
    /* Power up SerDes RX path */ /* select Exp_reg 05 */
    SOC_IF_ERROR_RETURN(
        WRITE_PHY54616_EXP_LED_FLASH_CTRLr(unit, pc, 0x0C3B));
    SOC_IF_ERROR_RETURN(
        WRITE_PHY54616_EXP_LED_FLASH_CTRLr(unit, pc, 0x0C3A));

    /* reset loopback to switch clock back to SerDes RX clock */
        /* bit[14]=0 */
    SOC_IF_ERROR_RETURN(
        MODIFY_PHY54616_1000X_MII_CTRLr(unit, pc, PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE) ? 
        MII_CTRL_PD : 0, MII_CTRL_LE | MII_CTRL_PD));

    PHY_FLAGS_SET(unit, port, PHY_FLAGS_100FX);
    pc->fiber.force_speed    = 100;
    pc->fiber.autoneg_enable = FALSE;

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_54616_1000x_setup
 * Purpose:
 *      Switch from 100FX mode to 1000X.  
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
_phy_54616_fiber_1000x_setup(int unit, soc_port_t port) 
{
    phy_ctrl_t    *pc;

    pc          = EXT_PHY_SW_STATE(unit, port);

    SOC_DEBUG_PRINT((DK_PHY | DK_VERBOSE,
                     "_phy_54616_fiber_1000x_setup: u=%d p=%d \n",
                      unit, port));

    /* set to Fiber mode and enable primary SerDes register */
        /* bit[2:1]=01*/
    SOC_IF_ERROR_RETURN(
        MODIFY_PHY54616_MODE_CTRLr(unit, pc, 0x2,0x6));
    /* enable 1000X, ie. disable 100FX */
        /* bit[0]=0 */
    SOC_IF_ERROR_RETURN(
        MODIFY_PHY54616_100FX_CTRLr(unit,pc, 0, 1));

    /* clear the SerDes power down bit */
        /* bit[11]=0 */
    SOC_DEBUG_PRINT((DK_PHY,
            "%s:p=%d,Pri-SerDes Mode. Fiber PowerDown(%s)!\n",
            FUNCTION_NAME(), port, "No"));
    SOC_IF_ERROR_RETURN(
        MODIFY_PHY54616_1000X_MII_CTRLr(unit, pc, PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE) ? 
        MII_CTRL_PD : 0, MII_CTRL_PD));
 
    pc->fiber.force_duplex = TRUE;
    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_100FX);

    return SOC_E_NONE;
}

STATIC int
_phy_54616_medium_change(int unit, soc_port_t port, int force_update)
{
    phy_ctrl_t    *pc;
    int            medium;

    pc    = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (_phy_54616_medium_check(unit, port, &medium));

    if (medium == SOC_PORT_MEDIUM_COPPER) {
        if ((!PHY_COPPER_MODE(unit, port)) || force_update) { /* Was fiber */ 
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_COPPER);
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_FIBER);
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_100FX);
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_MEDIUM_CHANGE);
            SOC_IF_ERROR_RETURN
                (_phy_54616_no_reset_setup(unit, port));

            /* Do not power up the interface if medium is disabled. */
            if (pc->copper.enable) {
                SOC_IF_ERROR_RETURN
                    (_phy_54616_medium_config_update(unit, port, &pc->copper));
            }
            SOC_DEBUG_PRINT((DK_PHY,
                         "_phy_54616_link_auto_detect: u=%d p=%d [F->C]\n",
                          unit, port));
        }
    } else {        /* Fiber */
        if (PHY_COPPER_MODE(unit, port) || force_update) { /* Was copper */
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_COPPER);
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_FIBER);
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_MEDIUM_CHANGE);
            SOC_IF_ERROR_RETURN
                (_phy_54616_no_reset_setup(unit, port));

            if (pc->fiber.enable) {
                SOC_IF_ERROR_RETURN
                    (_phy_54616_medium_config_update(unit, port, &pc->fiber));
            }
            SOC_DEBUG_PRINT((DK_PHY,
                          "_phy_54616_link_auto_detect: u=%d p=%d [C->F]\n",
                          unit, port));
        }
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_link_get
 * Purpose:
 *      Determine the current link up/down status for a 54616 device.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      link - (OUT) Boolean, true indicates link established.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      If using automedium, also switches the mode.
 */
STATIC int
phy_54616_link_get(int unit, soc_port_t port, int *link)
{
    phy_ctrl_t    *pc;
    uint16         data; 
    uint16         mask;

    pc    = EXT_PHY_SW_STATE(unit, port);
    *link = FALSE;      /* Default return */

    if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE)) {
        return SOC_E_NONE;
    }

    if (!pc->fiber.enable && !pc->copper.enable) {
        return SOC_E_NONE;
    }

    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_MEDIUM_CHANGE);
    if (pc->automedium) {
        /* Check for medium change and update HW/SW accordingly. */
        SOC_IF_ERROR_RETURN
            (_phy_54616_medium_change(unit, port,FALSE));
    }
    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(phy_fe_ge_link_get(unit, port, link));
    } else {
        SOC_IF_ERROR_RETURN
            (READ_PHY54616_1000X_MII_STATr(unit, pc, &data));
        *link = (data & MII_STAT_LA) ? TRUE : FALSE;
    }

    /* If preferred medium is up, disable the other medium 
     * to prevent false link. */
    if (pc->automedium) {
        mask = MII_CTRL_PD;
        if (pc->copper.preferred) {
            if (pc->fiber.enable) {
                /* power down secondary serdes */
                data = (*link && PHY_COPPER_MODE(unit, port)) ? MII_CTRL_PD : 0;
            } else {
                data = MII_CTRL_PD;
            }
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54616_1000X_MII_CTRLr(unit, pc, data, mask));
        } else {  /* pc->fiber.preferred */
            if (pc->copper.enable) {
                data = (*link && PHY_FIBER_MODE(unit, port)) ? MII_CTRL_PD : 0;
            } else {
                data = MII_CTRL_PD;
            }
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54616_MII_CTRLr(unit, pc, data, mask));
        }
    }

    SOC_DEBUG_PRINT((DK_PHY | DK_VERBOSE,
                     "phy_54616_link_get: u=%d p=%d mode=%s%s link=%d\n",
                      unit, port,
                      PHY_COPPER_MODE(unit, port) ? "C" : "F",
                      PHY_FIBER_100FX_MODE(unit, port)? "(100FX)" : "",
                      *link));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_duplex_set
 * Purpose:
 *      Set the current duplex mode (forced).
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      duplex - Boolean, true indicates full duplex, false indicates half.
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_UNAVAIL - Half duplex requested, and not supported.
 * Notes:
 *      The duplex is set only for the ACTIVE medium.
 *      No synchronization performed at this level.
 *      Autonegotiation is not manipulated.
 */

STATIC int
phy_54616_duplex_set(int unit, soc_port_t port, int duplex)
{
    int                  rv;
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;
    if (PHY_COPPER_MODE(unit, port)) {
        rv = phy_fe_ge_duplex_set(unit, port, duplex);
        if (SOC_SUCCESS(rv)) {
            pc->copper.force_duplex = duplex;
        }
    } else {    /* Fiber */
        if (PHY_FIBER_100FX_MODE(unit, port)) {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54616_100FX_CTRLr(unit, pc,
                       duplex? PHY_54616_PRIMARY_SERDES_100FX_FD:0, PHY_54616_PRIMARY_SERDES_100FX_FD));
            pc->fiber.force_duplex = duplex;
        } else { /* 1000X always full duplex */
            if (!duplex) {
                rv = SOC_E_UNAVAIL;
            }
        }
    }
    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_54616_duplex_set: u=%d p=%d d=%d rv=%d\n",
                     unit, port, duplex, rv));

    return rv;
}

/*
 * Function:
 *      phy_54616_duplex_get
 * Purpose:
 *      Get the current operating duplex mode. If autoneg is enabled,
 *      then operating mode is returned, otherwise forced mode is returned.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      duplex - (OUT) Boolean, true indicates full duplex, false
 *              indicates half.
 * Returns:
 *      SOC_E_NONE
 * Notes:
 *      The duplex is retrieved for the ACTIVE medium.
 *      No synchronization performed at this level. Autonegotiation is
 *      not manipulated.
 */

STATIC int
phy_54616_duplex_get(int unit, soc_port_t port, int *duplex)
{
    phy_ctrl_t    *pc;
    uint16 data16;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        return phy_fe_ge_duplex_get(unit, port, duplex);    
    } else {
        if (PHY_FIBER_100FX_MODE(unit, port)) {
            SOC_IF_ERROR_RETURN
                (READ_PHY54616_100FX_CTRLr(unit, pc, &data16));
            *duplex = (data16 & PHY_54616_PRIMARY_SERDES_100FX_FD) ?
                      TRUE: FALSE;
        } else { /* 1000X */ 
            *duplex = TRUE;
        }
    }
        
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_speed_set
 * Purpose:
 *      Set the current operating speed (forced).
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      speed - Requested speed, only 1000 supported.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The speed is set only for the ACTIVE medium.
 */

STATIC int
phy_54616_speed_set(int unit, soc_port_t port, int speed)
{
    int            rv; 
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;
    if (PHY_COPPER_MODE(unit, port)) {
        /* Note: mac.c uses phy_5690_notify_speed to update 5690 */
        rv = phy_fe_ge_speed_set(unit, port, speed);
        if (SOC_SUCCESS(rv)) {
            pc->copper.force_speed = speed;
        }
    } else {    /* Fiber */
        if (speed == 100) {
            rv = _phy_54616_fiber_100fx_setup(unit, port);
       } else  if (speed == 0 || speed == 1000) {
            rv = _phy_54616_fiber_1000x_setup(unit, port);
       } else {
            rv = SOC_E_CONFIG;
       }
       if (SOC_SUCCESS(rv)) {
           pc->fiber.force_speed = speed;
       }
    }
    if (SOC_SUCCESS(rv) && !PHY_SGMII_AUTONEG_MODE(unit, port)) {
        uint16 mii_ctrl;

        SOC_IF_ERROR_RETURN
            (READ_PHY54616_1000X_MII_CTRLr(unit, pc, &mii_ctrl));

        mii_ctrl &= ~(MII_CTRL_SS_LSB | MII_CTRL_SS_MSB);
        switch(speed) {
        case 10:
            mii_ctrl |= MII_CTRL_SS_10;
            break;
        case 100:
            mii_ctrl |= MII_CTRL_SS_100;
            break;
        case 0: 
        case 1000:
            mii_ctrl |= MII_CTRL_SS_1000;
            break;
        default:
            return(SOC_E_CONFIG);
        }

        SOC_IF_ERROR_RETURN
            (WRITE_PHY54616_1000X_MII_CTRLr(unit, pc, mii_ctrl));
    }

    SOC_DEBUG_PRINT((DK_PHY,
                     "phy_54616_speed_set: u=%d p=%d s=%d fiber=%d rv=%d\n",
                     unit, port, speed, PHY_FIBER_MODE(unit, port), rv));

    return rv;
}

/*
 * Function:
 *      phy_54616_speed_get
 * Purpose:
 *      Get the current operating speed for a 54616 device.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      duplex - (OUT) Boolean, true indicates full duplex, false
 *              indicates half.
 * Returns:
 *      SOC_E_NONE
 * Notes:
 *      The speed is retrieved for the ACTIVE medium.
 */

STATIC int
phy_54616_speed_get(int unit, soc_port_t port, int *speed)
{
    phy_ctrl_t    *pc;
    uint16         opt_mode;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        return phy_fe_ge_speed_get(unit, port, speed);
    } else {
        *speed = 1000;
        SOC_IF_ERROR_RETURN
            (READ_PHY54616_EXP_OPT_MODE_STATr(unit, pc, &opt_mode));
        switch(opt_mode & (0x3 << 13)) {
        case (0 << 13):
            *speed = 10;
            break;
        case (1 << 13):
            *speed = 100;
            break;
        case (2 << 13):
            *speed = 1000;
            break;
        default:
            SOC_DEBUG_PRINT((DK_WARN,
                            "phy_54616_speed_get: u=%d p=%d invalid speed\n",
                                unit, port));
        }
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_master_set
 * Purpose:
 *      Set the current master mode
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      master - SOC_PORT_MS_*
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The master mode is set only for the ACTIVE medium.
 */
STATIC int
phy_54616_master_set(int unit, soc_port_t port, int master)
{
    int                  rv;
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;
    if (PHY_COPPER_MODE(unit, port)) {
        rv = phy_fe_ge_master_set(unit, port, master);
        if (SOC_SUCCESS(rv)) {
            pc->copper.master = master;
        }
    }
    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_54616_master_set: u=%d p=%d master=%d fiber=%d rv=%d\n",
                    unit, port, master, PHY_FIBER_MODE(unit, port), rv));
    return rv;
}

/*
 * Function:
 *      phy_54616_master_get
 * Purpose:
 *      Get the current master mode for a 54616 device.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      master - (OUT) SOC_PORT_MS_*
 * Returns:
 *      SOC_E_NONE
 * Notes:
 *      The master mode is retrieved for the ACTIVE medium.
 */

STATIC int
phy_54616_master_get(int unit, soc_port_t port, int *master)
{
    if (PHY_COPPER_MODE(unit, port)) {
        return phy_fe_ge_master_get(unit, port, master);
    } else {
        *master = SOC_PORT_MS_NONE;
        return SOC_E_NONE;
    }
}

/*
 * Function:
 *      phy_54616_autoneg_set
 * Purpose:
 *      Enable or disable auto-negotiation on the specified port.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      autoneg - Boolean, if true, auto-negotiation is enabled
 *              (and/or restarted). If false, autonegotiation is disabled.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The autoneg mode is set only for the ACTIVE medium.
 */

STATIC int
phy_54616_autoneg_set(int unit, soc_port_t port, int autoneg)
{
    int                 rv;
    uint16              data, mask;
    phy_ctrl_t   *pc;

    /* don't allow AN if port is disabled  */

    if (autoneg) {
        if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE)) {
            return SOC_E_NONE;
        }
    }

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;
    if (PHY_COPPER_MODE(unit, port)) {
        rv = phy_fe_ge_an_set(unit, port, autoneg);
        if (SOC_SUCCESS(rv)) {
            pc->copper.autoneg_enable = autoneg ? 1 : 0;
        }
    } else { 
        data = (autoneg) ? MII_CTRL_AE | MII_CTRL_RAN : 0;
        mask = MII_CTRL_AE | MII_CTRL_RAN | MII_CTRL_FD;
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54616_1000X_MII_CTRLr(unit, pc, data, mask));

        pc->fiber.autoneg_enable = autoneg ? TRUE : FALSE;
    }
 
    SOC_DEBUG_PRINT((DK_PHY,
                     "phy_54616_autoneg_set: u=%d p=%d autoneg=%d rv=%d\n",
                     unit, port, autoneg, rv));

    return rv;
}

/*
 * Function:
 *      phy_54616_autoneg_get
 * Purpose:
 *      Get the current auto-negotiation status (enabled/busy).
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      autoneg - (OUT) if true, auto-negotiation is enabled.
 *      autoneg_done - (OUT) if true, auto-negotiation is complete. This
 *              value is undefined if autoneg == FALSE.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The autoneg mode is retrieved for the ACTIVE medium.
 */

STATIC int
phy_54616_autoneg_get(int unit, soc_port_t port,
                     int *autoneg, int *autoneg_done)
{
    int           rv;
    uint16        data;
    phy_ctrl_t   *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    rv            = SOC_E_NONE;
    *autoneg      = FALSE;
    *autoneg_done = FALSE;
    if (PHY_COPPER_MODE(unit, port)) {
        rv = phy_fe_ge_an_get(unit, port, autoneg, autoneg_done);
    } else {
        /* autoneg is not enabled in 100FX mode */
        if (PHY_FIBER_100FX_MODE(unit, port)) {
            *autoneg = FALSE;
            *autoneg_done = TRUE;
            return rv;
        }
        SOC_IF_ERROR_RETURN
            (READ_PHY54616_1000X_MII_CTRLr(unit, pc, &data));
        if (data & MII_CTRL_AE) {
            *autoneg = TRUE;

            SOC_IF_ERROR_RETURN
                (READ_PHY54616_1000X_MII_STATr(unit, pc, &data));
            if (data & MII_STAT_AN_DONE) {
                *autoneg_done = TRUE;
            }
        }
        
    }

    return rv;
}

/*
 * Function:
 *      phy_54616_ability_advert_set
 * Purpose:
 *      Set the current advertisement for auto-negotiation.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ability - Port ability indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The advertisement is set only for the ACTIVE medium.
 *      No synchronization performed at this level.
 */

STATIC int
_phy_54616_fiber_ability_advert_set(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    phy_ctrl_t  *pc;
    uint16      an_adv;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* Support only full duplex */
    an_adv = (ability->speed_full_duplex & SOC_PA_SPEED_1000MB) ? MII_ANP_C37_FD : 0;
    switch (ability->pause & (SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX)) {
    case SOC_PA_PAUSE_TX:
        an_adv |= MII_ANP_C37_ASYM_PAUSE;
        break;
    case SOC_PA_PAUSE_RX:
        an_adv |= MII_ANP_C37_ASYM_PAUSE | MII_ANP_C37_PAUSE;
        break;
    case SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX:
        an_adv |= MII_ANP_C37_PAUSE;
        break;
    }

    SOC_IF_ERROR_RETURN
        (WRITE_PHY54616_1000X_MII_ANAr(unit, pc, an_adv));

    return SOC_E_NONE;
}


STATIC int
phy_54616_ability_advert_set(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (phy_fe_ge_ability_advert_set(unit, port, ability));
        pc->copper.advert_ability = *ability;
    } else { 
        SOC_IF_ERROR_RETURN
            (_phy_54616_fiber_ability_advert_set(unit, port, ability));
        pc->fiber.advert_ability = *ability;
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_ability_advert_get
 * Purpose:
 *      Get the current advertisement for auto-negotiation.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ability - Port ability indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The advertisement is retrieved for the ACTIVE medium.
 *      No synchronization performed at this level.
 */

STATIC int
_phy_54616_fiber_ability_advert_get(int unit, soc_port_t port,soc_port_ability_t *ability)
{
    phy_ctrl_t  *pc;
    uint16      an_adv;

    pc = EXT_PHY_SW_STATE(unit, port);

    sal_memset(ability, 0, sizeof(*ability));

    SOC_IF_ERROR_RETURN
        (READ_PHY54616_1000X_MII_ANAr(unit, pc, &an_adv));

    switch (an_adv & (MII_ANP_C37_ASYM_PAUSE | MII_ANP_C37_PAUSE)) {
    case MII_ANP_C37_ASYM_PAUSE:
        ability->pause |= SOC_PA_PAUSE_TX;
        break;
    case (MII_ANP_C37_ASYM_PAUSE | MII_ANP_C37_PAUSE):
        ability->pause |= SOC_PA_PAUSE_RX;
        break;
    case MII_ANP_C37_PAUSE:
        ability->pause |= SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX;
        break;
    }

    ability->speed_full_duplex |= (an_adv & MII_ANP_C37_FD) ? SOC_PA_SPEED_1000MB : 0;

    return SOC_E_NONE;
}

STATIC int
phy_54616_ability_advert_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (phy_fe_ge_ability_advert_get(unit, port, ability));
    } else {    /* PHY_FIBER_MODE */
        SOC_IF_ERROR_RETURN
            (_phy_54616_fiber_ability_advert_get(unit, port, ability));
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_ability_remote_get
 * Purpose:
 *      Get partners current advertisement for auto-negotiation.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ability - Port ability indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The remote advertisement is retrieved for the ACTIVE medium.
 *      No synchronization performed at this level. If Autonegotiation is
 *      disabled or in progress, this routine will return an error.
 */

STATIC int
_phy_54616_fiber_ability_remote_get(int unit, soc_port_t port,
                               soc_port_ability_t *ability)
{
    uint16            an_adv, data;
    phy_ctrl_t       *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    sal_memset(ability, 0, sizeof(*ability));

    SOC_IF_ERROR_RETURN
        (READ_PHY54616_1000X_MII_CTRLr(unit, pc, &data));
    if (!(data & MII_CTRL_AE)) {
        return SOC_E_DISABLED;
    }
    SOC_IF_ERROR_RETURN
        (READ_PHY54616_1000X_MII_STATr(unit, pc, &data));

    if (data & MII_STAT_AN_DONE) {
        /* Decode remote advertisement only when link is up and autoneg is
         * completed.
         */

        SOC_IF_ERROR_RETURN
            (READ_PHY54616_1000X_MII_ANPr(unit, pc, &an_adv));

        ability->speed_full_duplex |= (an_adv & MII_ANP_C37_FD) ? SOC_PA_SPEED_1000MB : 0;
        ability->speed_half_duplex |= (an_adv & MII_ANP_C37_HD) ? SOC_PA_SPEED_1000MB : 0;

        switch (an_adv & (MII_ANP_C37_PAUSE | MII_ANP_C37_ASYM_PAUSE)) {
            case MII_ANP_C37_PAUSE:
                ability->pause |= SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX;
                break;
            case MII_ANP_C37_ASYM_PAUSE:
                ability->pause |= SOC_PA_PAUSE_TX;
                break;
            case MII_ANP_C37_PAUSE | MII_ANP_C37_ASYM_PAUSE:
                ability->pause |= SOC_PA_PAUSE_RX;
                break;
        }

    } else {
        /* Simply return local abilities */
        SOC_IF_ERROR_RETURN
            (phy_54616_ability_advert_get(unit, port, ability));
    }

    return (SOC_E_NONE);
}

STATIC int
phy_54616_ability_remote_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (phy_fe_ge_ability_remote_get(unit, port, ability));
    } else {    /* PHY_FIBER_MODE */
        SOC_IF_ERROR_RETURN
            (_phy_54616_fiber_ability_remote_get(unit, port, ability));
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_lb_set
 * Purpose:
 *      Set the local PHY loopback mode.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      loopback - Boolean: true = enable loopback, false = disable.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The loopback mode is set only for the ACTIVE medium.
 *      No synchronization performed at this level.
 */

STATIC int
phy_54616_lb_set(int unit, soc_port_t port, int enable)
{
    phy_ctrl_t    *pc;
    int           rv;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

#if AUTO_MDIX_WHEN_AN_DIS
    {
        /* Disable Auto-MDIX When autoneg disabled */
        /* Enable Auto-MDIX When autoneg disabled */
        data = (enable) ? 0x0000 : 0x0200;
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY_54616_MII_MISC_CTRLr(unit, pc, data, 0x0200)); 
    }
#endif

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (phy_fe_ge_lb_set(unit, port, enable));
    } else { 
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54616_1000X_MII_CTRLr(unit, pc, (enable) ? MII_CTRL_LE : 0, MII_CTRL_LE));
                  
    }
    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_54616_lb_set: u=%d p=%d en=%d rv=%d\n", 
                    unit, port, enable, rv));

    return SOC_E_NONE; 
}

/*
 * Function:
 *      phy_54616_lb_get
 * Purpose:
 *      Get the local PHY loopback mode.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      loopback - (OUT) Boolean: true = enable loopback, false = disable.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The loopback mode is retrieved for the ACTIVE medium.
 */

STATIC int
phy_54616_lb_get(int unit, soc_port_t port, int *enable)
{
    uint16               data;
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (phy_fe_ge_lb_get(unit, port, enable));
    } else {
       SOC_IF_ERROR_RETURN
           (READ_PHY54616_1000X_MII_CTRLr(unit, pc, &data));
        *enable = (data & MII_CTRL_LE) ? 1 : 0;
    }

    return SOC_E_NONE; 
}

/*
 * Function:
 *      phy_54616_interface_set
 * Purpose:
 *      Set the current operating mode of the PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      pif - one of SOC_PORT_IF_*
 * Returns:
 *      SOC_E_NONE - success
 *      SOC_E_UNAVAIL - unsupported interface
 */

STATIC int
phy_54616_interface_set(int unit, soc_port_t port, soc_port_if_t pif)
{
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(port);
    COMPILER_REFERENCE(pif);

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_interface_get
 * Purpose:
 *      Get the current operating mode of the PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      pif - (OUT) one of SOC_PORT_IF_*
 * Returns:
 *      SOC_E_NONE
 */

STATIC int
phy_54616_interface_get(int unit, soc_port_t port, soc_port_if_t *pif)
{
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    *pif = pc->interface;
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54616_ability_local_get
 * Purpose:
 *      Get the abilities of the local gigabit PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ability - return device's abilities.
 * Returns:
 *      SOC_E_NONE
 * Notes:
 *      The ability is retrieved only for the ACTIVE medium.
 */

STATIC int
_phy_54616_copper_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    ability->speed_half_duplex  = SOC_PA_SPEED_100MB | SOC_PA_SPEED_10MB;
    ability->speed_full_duplex  = SOC_PA_SPEED_1000MB | SOC_PA_SPEED_100MB | SOC_PA_SPEED_10MB;

    ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
    ability->interface = SOC_PA_INTF_SGMII | SOC_PA_INTF_RGMII;
    ability->medium    = SOC_PA_MEDIUM_COPPER;
    ability->loopback  = SOC_PA_LB_PHY;
    ability->flags     = SOC_PA_AUTONEG;

    return (SOC_E_NONE);
}

STATIC int
_phy_54616_fiber_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{

    ability->speed_half_duplex  = SOC_PA_SPEED_1000MB;
    ability->speed_full_duplex  = SOC_PA_SPEED_1000MB;

    ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
    ability->interface = SOC_PA_INTF_RGMII;
    ability->medium    = SOC_PA_MEDIUM_FIBER;
    ability->loopback  = SOC_PA_LB_PHY;
    ability->flags     = SOC_PA_ABILITY_NONE;

    return (SOC_E_NONE);
}

STATIC int
phy_54616_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    if (NULL == ability) {
        return SOC_E_PARAM;
    }

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(_phy_54616_copper_ability_local_get(unit, port, ability));
    } else {
        SOC_IF_ERROR_RETURN(_phy_54616_fiber_ability_local_get(unit, port, ability));
    }

    return (SOC_E_NONE);
}


/*
 * Function:
 *      phy_54616_mdix_set
 * Description:
 *      Set the Auto-MDIX mode of a port/PHY
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      mode - One of:
 *              SOC_PORT_MDIX_AUTO
 *                      Enable auto-MDIX when autonegotiation is enabled
 *              SOC_PORT_MDIX_FORCE_AUTO
 *                      Enable auto-MDIX always
 *              SOC_PORT_MDIX_NORMAL
 *                      Disable auto-MDIX
 *              SOC_PORT_MDIX_XOVER
 *                      Disable auto-MDIX, and swap cable pairs
 * Return Value:
 *      SOC_E_XXX
 */
STATIC int
phy_54616_mdix_set(int unit, soc_port_t port, soc_port_mdix_t mode)
{
    phy_ctrl_t    *pc;
    int                  speed;

    pc = EXT_PHY_SW_STATE(unit, port);
    if (PHY_COPPER_MODE(unit, port)) {
        switch (mode) {
        case SOC_PORT_MDIX_AUTO:
            /* Clear bit 14 for automatic MDI crossover */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54616_MII_ECRr(unit, pc, 0, 0x4000));

            /* Clear bit 9 to disable forced auto MDI xover */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54616_MII_MISC_CTRLr(unit, pc, 0, 0x0200));
            break;

        case SOC_PORT_MDIX_FORCE_AUTO:
            /* Clear bit 14 for automatic MDI crossover */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54616_MII_ECRr(unit, pc, 0, 0x4000));

            /* Set bit 9 to disable forced auto MDI xover */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54616_MII_MISC_CTRLr(unit, pc, 0x0200, 0x0200));
            break;

        case SOC_PORT_MDIX_NORMAL:
            SOC_IF_ERROR_RETURN(phy_54616_speed_get(unit, port, &speed));
            if (speed == 0 || speed == 10 || speed == 100) {
                /* Set bit 14 for automatic MDI crossover */
                SOC_IF_ERROR_RETURN
                    (MODIFY_PHY54616_MII_ECRr(unit, pc, 0x4000, 0x4000));

                SOC_IF_ERROR_RETURN
                    (WRITE_PHY54616_TEST1r(unit, pc, 0));
            } else {
                return SOC_E_UNAVAIL;
            }
            break;

        case SOC_PORT_MDIX_XOVER:
            SOC_IF_ERROR_RETURN(phy_54616_speed_get(unit, port, &speed));
            if (speed == 0 || speed == 10 || speed == 100) {
                /* Set bit 14 for automatic MDI crossover */
                SOC_IF_ERROR_RETURN
                    (MODIFY_PHY54616_MII_ECRr(unit, pc, 0x4000, 0x4000));

                SOC_IF_ERROR_RETURN
                    (WRITE_PHY54616_TEST1r(unit, pc, 0x0080));
            } else {
                return SOC_E_UNAVAIL;
            }
            break;

        default:
            return SOC_E_PARAM;
        }
        pc->copper.mdix = mode;
    } else {    /* fiber */
        if (mode != SOC_PORT_MDIX_NORMAL) {
            return SOC_E_UNAVAIL;
        }
    }
    return SOC_E_NONE;
}        

/*
 * Function:
 *      phy_54616_mdix_get
 * Description:
 *      Get the Auto-MDIX mode of a port/PHY
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      mode - (Out) One of:
 *              SOC_PORT_MDIX_AUTO
 *                      Enable auto-MDIX when autonegotiation is enabled
 *              SOC_PORT_MDIX_FORCE_AUTO
 *                      Enable auto-MDIX always
 *              SOC_PORT_MDIX_NORMAL
 *                      Disable auto-MDIX
 *              SOC_PORT_MDIX_XOVER
 *                      Disable auto-MDIX, and swap cable pairs
 * Return Value:
 *      SOC_E_XXX
 */
STATIC int
phy_54616_mdix_get(int unit, soc_port_t port, soc_port_mdix_t *mode)
{
    phy_ctrl_t    *pc;
    int            speed;

    if (mode == NULL) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(phy_54616_speed_get(unit, port, &speed));
        if (speed == 1000) {
           *mode = SOC_PORT_MDIX_AUTO;
        } else {
            *mode = pc->copper.mdix;
        }
    } else {
        *mode = SOC_PORT_MDIX_NORMAL;
    }

    return SOC_E_NONE;
}    

/*
 * Function:
 *      phy_54616_mdix_status_get
 * Description:
 *      Get the current MDIX status on a port/PHY
 * Parameters:
 *      unit    - Device number
 *      port    - Port number
 *      status  - (OUT) One of:
 *              SOC_PORT_MDIX_STATUS_NORMAL
 *                      Straight connection
 *              SOC_PORT_MDIX_STATUS_XOVER
 *                      Crossover has been performed
 * Return Value:
 *      SOC_E_XXX
 */
STATIC int
phy_54616_mdix_status_get(int unit, soc_port_t port, 
                         soc_port_mdix_status_t *status)
{
    phy_ctrl_t    *pc;
    uint16          data;

    if (status == NULL) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (READ_PHY54616_MII_ESRr(unit, pc, &data));
        if (data & 0x2000) {
            *status = SOC_PORT_MDIX_STATUS_XOVER;
        } else {
            *status = SOC_PORT_MDIX_STATUS_NORMAL;
        }
    } else {
        *status = SOC_PORT_MDIX_STATUS_NORMAL;
    }

    return SOC_E_NONE;
}    

STATIC int
_phy_54616_power_mode_set (int unit, soc_port_t port, int mode)
{
    phy_ctrl_t    *pc;

    pc       = EXT_PHY_SW_STATE(unit, port);

    if (pc->power_mode == mode) {
        return SOC_E_NONE;
    }

    if (mode == SOC_PHY_CONTROL_POWER_FULL) {
        /* disable the auto power mode */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54616_AUTO_POWER_DOWNr(unit,pc,
                        0,
                        PHY_54616_AUTO_PWRDWN_EN));
        pc->power_mode = mode;
    } else if (mode == SOC_PHY_CONTROL_POWER_AUTO) {
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54616_AUTO_POWER_DOWNr(unit,pc,
                        PHY_54616_AUTO_PWRDWN_EN,
                        PHY_54616_AUTO_PWRDWN_EN));
        pc->power_mode = mode;
    }
    return SOC_E_NONE;
}



/*
 * Function:
 *      phy_54616_control_set
 * Purpose:
 *      Configure PHY device specific control fucntion.
 * Parameters:
 *      unit  - StrataSwitch unit #.
 *      port  - StrataSwitch port #.
 *      type  - Control to update
 *      value - New setting for the control
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
phy_54616_control_set(int unit, soc_port_t port,
                     soc_phy_control_t type, uint32 value)
{
    int rv;
    phy_ctrl_t *pc;
    uint16 data16;

    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    switch(type) {
    case SOC_PHY_CONTROL_CLOCK_ENABLE:
        if (pc->phy_rev == 0xB) {
            SOC_IF_ERROR_RETURN
                (WRITE_PHY54616_EXP_SGMII_REC_CTRLr(unit, pc, value ? (1U<<4) : 0));
        }
        break;

    case SOC_PHY_CONTROL_POWER:
        rv = _phy_54616_power_mode_set(unit,port,value);
        break;

    case SOC_PHY_CONTROL_POWER_AUTO_WAKE_TIME:
        if (value <= PHY_54616_AUTO_PWRDWN_WAKEUP_MAX) {

            /* at least one unit */
            if (value < PHY_54616_AUTO_PWRDWN_WAKEUP_UNIT) {
                value = PHY_54616_AUTO_PWRDWN_WAKEUP_UNIT;
            }
        } else { /* anything more then max, set to the max */
            value = PHY_54616_AUTO_PWRDWN_WAKEUP_MAX;
        }
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54616_AUTO_POWER_DOWNr(unit,pc,
                      value/PHY_54616_AUTO_PWRDWN_WAKEUP_UNIT,
                      PHY_54616_AUTO_PWRDWN_WAKEUP_MASK));
        break;

    case SOC_PHY_CONTROL_POWER_AUTO_SLEEP_TIME:
        /* sleep time configuration is either 2.7s or 5.4 s, default is 2.7s */
        if (value < PHY_54616_AUTO_PWRDWN_SLEEP_MAX) {
            data16 = 0; /* anything less than 5.4s, default to 2.7s */
        } else {
            data16 = PHY_54616_AUTO_PWRDWN_SLEEP_MASK;
        }
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54616_AUTO_POWER_DOWNr(unit,pc,
                      data16,
                      PHY_54616_AUTO_PWRDWN_SLEEP_MASK));
        break;


    default:
        rv = SOC_E_UNAVAIL;
        break;
    }
    return rv;
}

/*
 * Function:
 *      phy_54616_control_get
 * Purpose:
 *      Get current control settign of the PHY.
 * Parameters:
 *      unit  - StrataSwitch unit #.
 *      port  - StrataSwitch port #.
 *      type  - Control to update
 *      value - (OUT)Current setting for the control
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
phy_54616_control_get(int unit, soc_port_t port,
                     soc_phy_control_t type, uint32 *value)
{
    int rv;
    phy_ctrl_t *pc;
    uint16 data;

    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    switch(type) {
    case SOC_PHY_CONTROL_CLOCK_ENABLE:
        if (pc->phy_rev == 0xB) {
            SOC_IF_ERROR_RETURN
                (READ_PHY54616_EXP_SGMII_REC_CTRLr(unit, pc, &data));
            *value = (data & (1U << 4)) ? TRUE : FALSE;
        }
        break;

    case SOC_PHY_CONTROL_POWER:
        *value = pc->power_mode;
        break;

    case SOC_PHY_CONTROL_POWER_AUTO_SLEEP_TIME:
        SOC_IF_ERROR_RETURN
            (READ_PHY54616_AUTO_POWER_DOWNr(unit,pc, &data));

        if (data & PHY_54616_AUTO_PWRDWN_SLEEP_MASK) {
            *value = PHY_54616_AUTO_PWRDWN_SLEEP_MAX;
        } else {
            *value = 2700;
        }
        break;

    case SOC_PHY_CONTROL_POWER_AUTO_WAKE_TIME:
        SOC_IF_ERROR_RETURN
            (READ_PHY54616_AUTO_POWER_DOWNr(unit,pc, &data));

        data &= PHY_54616_AUTO_PWRDWN_WAKEUP_MASK;
        *value = data * PHY_54616_AUTO_PWRDWN_WAKEUP_UNIT;
        break;
        
    default:
        rv = SOC_E_UNAVAIL;
        break;
    }
    return rv;
}

/*
 * Function:
 *      phy_54616_cable_diag
 * Purpose:
 *      Run 546x cable diagnostics
 * Parameters:
 *      unit - device number
 *      port - port number
 *      status - (OUT) cable diagnotic status structure
 * Returns:     
 *      SOC_E_XXX
 */
STATIC int
phy_54616_cable_diag(int unit, soc_port_t port,
                    soc_port_cable_diag_t *status)
{
    int                 rv=SOC_E_NONE, rv2, i;

    extern int phy_5464_cable_diag_sw(int, soc_port_t , 
                                      soc_port_cable_diag_t *);

    if (status == NULL) {
        return SOC_E_PARAM;
    }

    status->state = SOC_PORT_CABLE_STATE_OK;
    status->npairs = 4;
    status->fuzz_len = 0;
    for (i = 0; i < 4; i++) {
        status->pair_state[i] = SOC_PORT_CABLE_STATE_OK;
    }

    MIIM_LOCK(unit);    /* this locks out linkscan, essentially */
    rv = phy_5464_cable_diag_sw(unit,port, status);
    MIIM_UNLOCK(unit);
    rv2 = 0;
    if (rv <= 0) {      /* don't reset if > 0 -- link was up */
        rv2 = _phy_54616_reset_setup(unit, port);
    }
    if (rv >= 0 && rv2 < 0) {
        return rv2;
    }
    return rv;
}

/*
 * Function:
 *      phy_54616_probe
 * Purpose:
 *      Complement the generic phy probe routine to identify this phy when its
 *      phy id0 and id1 is same as some other phy's.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      pc   - phy ctrl descriptor.
 * Returns:
 *      SOC_E_NONE,SOC_E_NOT_FOUND and SOC_E_<error>
 */
STATIC int
phy_54616_probe(int unit, phy_ctrl_t *pc)
{
    uint16 id0, id1, data;
    soc_phy_info_t *pi;

    if (READ_PHY54616_MII_PHY_ID0r(unit, pc, &id0) < 0) {
        return SOC_E_NOT_FOUND;
    }
    if (READ_PHY54616_MII_PHY_ID1r(unit, pc, &id1) < 0) {
        return SOC_E_NOT_FOUND;
    }

    pi = &SOC_PHY_INFO(unit, pc->port);

    switch (PHY_MODEL(id0, id1)) {

    case PHY_BCM54616_MODEL:
        SOC_IF_ERROR_RETURN
            (READ_PHY54616_MODE_CTRLr(unit, pc, &data));
        if (data & (1U << 3)) {
            /* Serdes present so it's a 54616S */
            pi->phy_name = "BCM54616S";
            PHY_FLAGS_SET(pc->unit, pc->port, PHY_FLAGS_SECONDARY_SERDES);
        }
    break;

    default:
        return SOC_E_NOT_FOUND;
    break;

    }
    return SOC_E_NONE;
}


#ifdef BROADCOM_DEBUG

/*
 * Function:
 *      phy_54616_shadow_dump
 * Purpose:
 *      Debug routine to dump all shadow registers.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 */

void
phy_54616_shadow_dump(int unit, soc_port_t port)
{
    phy_ctrl_t *pc;
    uint16      tmp;
    int         i;

    pc       = EXT_PHY_SW_STATE(unit, port);

    /* Register 0x18 Shadows */
    for (i = 0; i <= 7; i++) {
        WRITE_PHY_REG(unit, pc, 0x18, (i << 12) | 0x7);
        READ_PHY_REG(unit, pc, 0x18, &tmp);
        if ((tmp & ~7) == 0x0000) {
            continue;
        }
        soc_cm_print("0x18[0x%x]=0x%04x\n", i, tmp);
    }

    /* Register 0x1c Shadows */
    for (i = 0; i <= 0x1f; i++) {
        WRITE_PHY_REG(unit, pc, 0x1c, i << 10);
        READ_PHY_REG(unit, pc, 0x1c, &tmp);
        if ((tmp & ~0x7c00) == 0x0000) {
            continue;
        }
        soc_cm_print("0x1c[0x%x]=0x%04x\n", i, tmp);
    }

    /* Register 0x17/0x15 Shadows */
    for (i = 0; i <= 0x13; i++) {
        WRITE_PHY_REG(unit, pc, 0x17, 0xf00 | i);
        READ_PHY_REG(unit, pc, 0x15, &tmp);
        if (tmp  == 0x0000) {
            continue;
        }
        soc_cm_print("0x17[0x%x]=0x%04x\n", i, tmp);
    }
}

#endif /* BROADCOM_DEBUG */

/*
 * Variable:    phy_54616drv_ge
 * Purpose:     PHY driver for 54616
 */

phy_driver_t phy_54616drv_ge = {
    "54616/801x Gigabit PHY Driver",
    phy_54616_init,
    phy_fe_ge_reset,
    phy_54616_link_get,
    phy_54616_enable_set,
    phy_54616_enable_get,
    phy_54616_duplex_set,
    phy_54616_duplex_get,
    phy_54616_speed_set,
    phy_54616_speed_get,
    phy_54616_master_set,
    phy_54616_master_get,
    phy_54616_autoneg_set,
    phy_54616_autoneg_get,
    NULL, /* Local Advert set, deprecated */
    NULL, /* Local Advert get, deprecated */
    NULL, /* Remote Advert get, deprecated */
    phy_54616_lb_set,
    phy_54616_lb_get,
    phy_54616_interface_set,
    phy_54616_interface_get,
    NULL,
    NULL,                        /* Phy link up event */
    NULL,                        /* Phy link down event */
    phy_54616_mdix_set,
    phy_54616_mdix_get,
    phy_54616_mdix_status_get,
    phy_54616_medium_config_set,
    phy_54616_medium_config_get,
    phy_54616_medium_status,
    phy_54616_cable_diag,
    NULL,                       /* phy_link_change */
    phy_54616_control_set,       /* phy_control_set */ 
    phy_54616_control_get,       /* phy_control_get */
    phy_ge_reg_read,
    phy_ge_reg_write,
    phy_ge_reg_modify,
    NULL,                        /* Phy event notify */ 
    phy_54616_probe,          /* pd_probe  */
    phy_54616_ability_advert_set,  /* pd_ability_advert_set */
    phy_54616_ability_advert_get,  /* pd_ability_advert_get */
    phy_54616_ability_remote_get,  /* pd_ability_remote_get */
    phy_54616_ability_local_get,   /* pd_ability_local_get  */
    NULL /* pd_firmware_set */
};

#else /* INCLUDE_PHY_54616_ESW */
int _soc_phy_54616_not_empty;
#endif /* INCLUDE_PHY_54616 */

