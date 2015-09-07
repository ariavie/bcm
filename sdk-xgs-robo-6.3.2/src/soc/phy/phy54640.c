/*
 * $Id: phy54640.c 1.103 Broadcom SDK $
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
 * File:        phy54640.c
 * Purpose:     PHY driver for BCM54640
 *
 * Supported BCM54640 PHY:
 *
 *      Device  Ports   Media              MAC Interface
 *      54640   4       Copper, Serdes     SGMII
 *
 * Workarounds:
 *
 * References:
 * 54640-DS02-R.pdf          BCM54640 Data Sheet
 *     
 *    +----------+   +---------------------------+
 *    |          |   |            +->Copper      |<--> Magnetic
 *    |  SGMII   |<->| SGMII    <-+              |
 *    | (SerDes) |   |            |              |
 *    |          |   |            +->SGMII/      |<--> (PHY)SFP (10/100/1000)
 *    +----------+   |               1000BASE-X/ |<--> 1000BASE-X SFP
 *        MAC        |               100BASE-FX  |<--> 100BASE-FX SFP
 *                   |                           |
 *                   |                           |
 *                   +---------------------------+
 *                               54640
 *
 * Notes:
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

#if defined(INCLUDE_PHY_54640) || defined(INCLUDE_PHY_54640E)
#include "phyconfig.h"    /* Must be the first phy include after phydefs.h */
#include "phyident.h"
#include "phyreg.h"
#include "phyfege.h"
#include "phy54640.h"

#define AUTO_MDIX_WHEN_AN_DIS       0       /* Non-standard-compliant option */
#define WORKAROUND_FIXUPS           1

#define ECD_ENABLE 0

#define ADVERT_ALL_COPPER \
        (SOC_PM_PAUSE | SOC_PM_10MB | SOC_PM_100MB | SOC_PM_1000MB)
#define ADVERT_ALL_FIBER \
        (SOC_PM_PAUSE | SOC_PM_1000MB_FD)

#define PHY_IS_BCM54640E(_pc)   (PHY_MODEL_CHECK((_pc), \
                                 PHY_BCM54680E_OUI, \
                                 PHY_BCM54680E_MODEL) \
                                 && (((_pc)->phy_rev & 0x8) == 0))

#define PHY_IS_BCM54640E_A0(_pc) (PHY_MODEL_CHECK((_pc), \
                                 PHY_BCM54680E_OUI, \
                                 PHY_BCM54680E_MODEL) \
                                 && ((_pc)->phy_rev == 0x0 ))

#define PHY_IS_BCM54640E_A1(_pc) (PHY_MODEL_CHECK((_pc), \
                                 PHY_BCM54680E_OUI, \
                                 PHY_BCM54680E_MODEL) \
                                 && ((_pc)->phy_rev == 0x1 ))

#define PHY_IS_BCM54640E_A0A1(_pc) (PHY_MODEL_CHECK((_pc), \
                                 PHY_BCM54680E_OUI, \
                                 PHY_BCM54680E_MODEL) \
                                 && (((_pc)->phy_rev & 0xe)== 0x0 ))

STATIC int _phy_54640_no_reset_setup(int unit, soc_port_t port);
STATIC int _phy_54640_medium_change(int unit, soc_port_t port, int force_update);
STATIC int phy_54640_duplex_set(int unit, soc_port_t port, int duplex);
STATIC int phy_54640_speed_set(int unit, soc_port_t port, int speed);
STATIC int phy_54640_master_set(int unit, soc_port_t port, int master);
STATIC int phy_54640_ability_advert_set(int unit, soc_port_t port, soc_port_ability_t *ability);
STATIC int phy_54640_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability);
STATIC int _phy_54640_ability_cu_local_get(int unit, soc_port_t port, 
                                       soc_port_ability_t *ability);
STATIC int _phy_54640_ability_fiber_local_get(int unit, soc_port_t port, 
                                       soc_port_ability_t *ability);
STATIC int phy_54640_autoneg_set(int unit, soc_port_t port, int autoneg);
STATIC int phy_54640_mdix_set(int unit, soc_port_t port, soc_port_mdix_t mode);
STATIC int phy_54640_speed_get(int unit, soc_port_t port, int *speed);
STATIC int phy_54640_control_set(int unit, soc_port_t port, soc_phy_control_t type, uint32 value);
STATIC int phy_54640_control_get(int unit, soc_port_t port, soc_phy_control_t type, uint32 *value);
                    
extern int
phy_ecd_cable_diag_init(int unit, soc_port_t port);

extern int
phy_ecd_cable_diag(int unit, soc_port_t port,
            soc_port_cable_diag_t *status);

/***********************************************************************
 *
 * HELPER FUNCTIONS
 *
 ***********************************************************************/
/*
 * Function:
 *      _phy54640_toplvl_reg_read
 * Purpose:
 *      Read a top level register
 * Parameters:
 *      unit - SOC Unit #.
 *      port - Port number.
 *      reg_offset  - Offset to the reg
 *      data  - Pointer to data returned
 * Returns:
 *      SOC_E_XXX
 */
int
_phy54640_toplvl_reg_read(int unit, soc_port_t port,
                          uint8 reg_offset, uint16 *data)

{
    phy_ctrl_t    *pc;
    uint16 reg_data, status = 0;
    uint8 phyid, phyid_port0;

    pc       = EXT_PHY_SW_STATE(unit, port);
    if (!pc) {
        return SOC_E_FAIL;
    }

    phyid = pc->phy_id;
    phyid_port0 = pc->phy_id & 0xfc; /* lower 2 bits to 00 */

    pc->phy_id = phyid_port0;  /* Base port Phy address */
    /* 
     * To access top level registers, 
     * Write 1Ch shadow 14h.6:5 =0'b10 , 1Ch shadow 14h.6:5 =0'b00.  
     * Then MDIO address 1 can access MDIO port 5 and 
     * MDIO address 2 can access MDIO port 6.
     */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_EXT_SERDES_CTRLr(unit, pc, 0x0040, 0x0040));
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_EXT_SERDES_CTRLr(unit, pc, 0x0020, 0x0040));

    pc->phy_id = phyid_port0 + 1;  /* As such this is port 5 */
    /* Write Reg address to Port 5's register 0x1C, shadow 0x0B */
    /* Status READ from Port 3's register 0x15 */

    /* Write Reg offset to Port 5's register 0x1C, shadow 0x0B */
    reg_data = (0xAC00 | reg_offset);
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, 0x00, 0x000b, 0x1c, reg_data));

    pc->phy_id = phyid_port0;  /* Base port Phy address */
    /* Getting back to normal mode */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_EXT_SERDES_CTRLr(unit, pc, 0x0040, 0x0040));
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_EXT_SERDES_CTRLr(unit, pc, 0x0000, 0x0040));

    pc->phy_id = phyid_port0 + 2; /* as such port 3 */
    /* Read data from Top level MII Status register(0x15h) */
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_READ(unit, pc, 0x00, 0x0f50, 0x15, &status));

    pc->phy_id = phyid;
    *data = (status & 0xff);

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy54640_toplvl_reg_write
 * Purpose:
 *      Write a top level register
 * Parameters:
 *      unit - SOC Unit #.
 *      port - Port number.
 *      reg_offset  - Offset to the reg
 *      data  - data to be written
 * Returns:
 *      SOC_E_XXX
 */
int
_phy54640_toplvl_reg_write(int unit, soc_port_t port,
                          uint8 reg_offset, uint16 data)

{
    phy_ctrl_t    *pc;
    uint16 reg_data;
    uint8 phyid, phyid_port0;

    pc       = EXT_PHY_SW_STATE(unit, port);
    if (!pc) {
        return SOC_E_FAIL;
    }

    phyid = pc->phy_id;
    phyid_port0 = pc->phy_id & 0xfc; /* lower 2 bits to 00 */

    pc->phy_id = phyid_port0;  /* Base port Phy address */
    /* 
     * To access top level registers, 
     * Write 1Ch shadow 14h.6:5 =0'b10 , 1Ch shadow 14h.6:5 =0'b00.  
     * Then MDIO address 1 can access MDIO port 5 and 
     * MDIO address 2 can access MDIO port 6.
     */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_EXT_SERDES_CTRLr(unit, pc, 0x0040, 0x0040));
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_EXT_SERDES_CTRLr(unit, pc, 0x0020, 0x0040));

    pc->phy_id = phyid_port0 + 2;  /* As such this is port 6 */
    /* Write Reg address to Port 5's register 0x1C, shadow 0x0B */
    /* Status READ from Port 3's register 0x15 */
    reg_data = (0xB000 | (data & 0xff));
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, 0x00, 0x000b, 0x1c, reg_data));

    pc->phy_id = phyid_port0 + 1;  /* As such this is port 5 */
    /* Write Reg offset to Port 5's register 0x1C, shadow 0x0B */
    reg_data = (0xAC80 | reg_offset);
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, 0x00, 0x000b, 0x1c, reg_data));

    /* Disable Write ( Port 5's register 0x1C, shadow 0x0B) Bit 7 = 0 */
    reg_data = (0xAC00 | reg_offset);
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, 0x00, 0x000b, 0x1c, reg_data));

    pc->phy_id = phyid_port0;  /* Base port Phy address */
    /* getting back to normal mode */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_EXT_SERDES_CTRLr(unit, pc, 0x0040, 0x0040));
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_EXT_SERDES_CTRLr(unit, pc, 0x0000, 0x0040));

    pc->phy_id = phyid;

    return SOC_E_NONE;
}
/*
 * Function:
 *      _phy_54640e_cl45_reg_read
 * Purpose:
 *      Read Clause 45 Register using Clause 22 register access
 * Parameters:
 *      unit      - StrataSwitch unit #.
 *      pc        - Phy control
 *      flags     - Flags
 *      dev_addr  - Clause 45 Device
 *      reg_addr  - Register Address to read
 *      val       - Value Read
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
_phy_54640e_cl45_reg_read(int unit, phy_ctrl_t *pc, uint32 flags, 
                         uint8 dev_addr, uint16 reg_addr, uint16 *val)
{

    /* Write Device Address to register 0x0D (Set Function field to Address)*/
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, flags, 0x00, 0x0D, (dev_addr & 0x001f)));

    /* Select the register by writing to register address to register 0x0E */
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, flags, 0x00, 0x0E, reg_addr));

    /* Write Device Address to register 0x0D (Set Function field to Data)*/
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, flags, 0x00, 0x0D, 
                           ((0x4000) | (dev_addr & 0x001f))));

    /* Read register 0x0E to get the value */
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_READ(unit, pc, flags, 0x00, 0x0E, val));

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_54640e_cl45_reg_write
 * Purpose:
 *      Write Clause 45 Register content using Clause 22 register access
 * Parameters:
 *      unit      - StrataSwitch unit #.
 *      pc        - Phy control
 *      flags     - Flags
 *      dev_addr  - Clause 45 Device
 *      reg_addr  - Register Address to read
 *      val       - Value to be written
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
_phy_54640e_cl45_reg_write(int unit, phy_ctrl_t *pc, uint32 flags, 
                         uint8 dev_addr, uint16 reg_addr, uint16 val)
{

    /* Write Device Address to register 0x0D (Set Function field to Address)*/
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, flags, 0x00, 0x0D, (dev_addr & 0x001f)));

    /* Select the register by writing to register address to register 0x0E */
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, flags, 0x00, 0x0E, reg_addr));

    /* Write Device Address to register 0x0D (Set Function field to Data)*/
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, flags, 0x00, 0x0D, 
                           ((0x4000) | (dev_addr & 0x001f))));

    /* Write register 0x0E to write the value */
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, flags, 0x00, 0x0E, val));

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_54640e_cl45_reg_modify
 * Purpose:
 *      Modify Clause 45 Register contents using Clause 22 register access
 * Parameters:
 *      unit      - StrataSwitch unit #.
 *      pc        - Phy control
 *      flags     - Flags
 *      reg_bank  - Register bank
 *      dev_addr  - Clause 45 Device
 *      reg_addr  - Register Address to read
 *      val       - Value 
 *      mask      - Mask for modifying the contents of the register
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
_phy_54640e_cl45_reg_modify(int unit, phy_ctrl_t *pc, uint32 flags, 
                          uint8 dev_addr, uint16 reg_addr, uint16 val, 
                          uint16 mask)
{

    uint16 value = 0;

    SOC_IF_ERROR_RETURN
        (PHY54640E_CL45_REG_READ(unit, pc, flags, dev_addr, reg_addr, &value));

    value = (val & mask) | (value & ~mask);

    SOC_IF_ERROR_RETURN
        (PHY54640E_CL45_REG_WRITE(unit, pc, flags, dev_addr, reg_addr, value));

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_54640e_blk_top_lvl_reg_read
 * Purpose:
 *      New mechanism to Read Top Level Registers for EEE and 1588.
 * Parameters:
 *      unit       - StrataSwitch unit #.
 *      pc         - Phy control
 *      flags      - Flags
 *      reg_offset - Register Address to read
 *      val        - Value Read
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
_phy_54640e_blk_top_lvl_reg_read(int unit, phy_ctrl_t *pc, uint32 flags, 
                         uint16 reg_offset, uint16 *val)
{
    /* Write register 0x17 with the top level reg offset to read (handled in PHY54640_REG_READ) */
    /* Read register 0x15 to get the value */
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_READ(unit, pc, flags, (0x0D00|(reg_offset & 0xff)), 0x15, val));

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_54640e_blk_top_lvl_reg_write
 * Purpose:
 *      New mechanism to Write Top Level Registers for EEE and 1588
 * Parameters:
 *      unit      - StrataSwitch unit #.
 *      pc        - Phy control
 *      flags     - Flags
 *      reg_offset  - Register Address to read
 *      val       - Value to be written
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
_phy_54640e_blk_top_lvl_reg_write(int unit, phy_ctrl_t *pc, uint32 flags, 
                                  uint16 reg_offset, uint16 val)
{
    /* Write register 0x17 with the top level reg offset to write (handled in PHY54640_REG_WRITE) */
    /* Write register 0x15 the value to write to the top level register offset */
    SOC_IF_ERROR_RETURN
        (PHY54640_REG_WRITE(unit, pc, flags, (0x0D00|(reg_offset & 0xff)), 0x15, val));

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_54640e_blk_top_lvl_reg_modify
 * Purpose:
 *      Modify Clause 45 Register contents using Clause 22 register access
 * Parameters:
 *      unit      - StrataSwitch unit #.
 *      pc        - Phy control
 *      flags     - Flags
 *      reg_bank  - Register bank
 *      dev_addr  - Clause 45 Device
 *      reg_addr  - Register Address to read
 *      val       - Value 
 *      mask      - Mask for modifying the contents of the register
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
_phy_54640e_blk_top_lvl_reg_modify(int unit, phy_ctrl_t *pc, uint32 flags,
                          uint16 reg_offset, uint16 val, uint16 mask)
{

    uint16 value = 0;

    /* Write register 0x17 with the top level reg offset to read */
    SOC_IF_ERROR_RETURN
        (_phy_54640e_blk_top_lvl_reg_read(unit, pc, flags, 
                                          reg_offset, &value));

    value = (val & mask) | (value & ~mask);

    /* Write register 0x17 with the top level reg offset to read */

    SOC_IF_ERROR_RETURN
        (_phy_54640e_blk_top_lvl_reg_write(unit, pc, flags, 
                                          reg_offset, value));

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_54640_medium_check
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
_phy_54640_medium_check(int unit, soc_port_t port, int *medium)
{
    phy_ctrl_t    *pc = EXT_PHY_SW_STATE(unit, port);
    uint16         tmp = 0xffff;

    *medium = SOC_PORT_MEDIUM_COPPER;

    if (pc->automedium) {
        /* Read Mode Register (0x1c shadow 11111) */
        SOC_IF_ERROR_RETURN
            (READ_PHY54640_MODE_CTRLr(unit, pc, &tmp));

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
   
    SOC_DEBUG_PRINT((DK_PHY | DK_VERBOSE,
                     "_phy_54640_medium_check: "
                     "u=%d p=%d fiber_pref=%d 0x1c(11111)=%04x fiber=%d\n",
                     unit, port, pc->fiber.preferred, tmp, 
                     (*medium == SOC_PORT_MEDIUM_FIBER) ? 1 : 0));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_medium_status
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
phy_54640_medium_status(int unit, soc_port_t port, soc_port_medium_t *medium)
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
 *      _phy_54640_medium_config_update
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
_phy_54640_medium_config_update(int unit, soc_port_t port,
                                soc_phy_config_t *cfg)
{
    SOC_IF_ERROR_RETURN
        (phy_54640_speed_set(unit, port, cfg->force_speed));
    SOC_IF_ERROR_RETURN
        (phy_54640_duplex_set(unit, port, cfg->force_duplex));
    SOC_IF_ERROR_RETURN
        (phy_54640_master_set(unit, port, cfg->master));
    SOC_IF_ERROR_RETURN
        (phy_54640_ability_advert_set(unit, port, &cfg->advert_ability));
    SOC_IF_ERROR_RETURN
        (phy_54640_autoneg_set(unit, port, cfg->autoneg_enable));
    SOC_IF_ERROR_RETURN
        (phy_54640_mdix_set(unit, port, cfg->mdix));

    return SOC_E_NONE;
}

/***********************************************************************
 *
 * PHY54640 DRIVER ROUTINES
 *
 ***********************************************************************/

/*
 * Function:
 *      phy_54640_medium_config_set
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
phy_54640_medium_config_set(int unit, soc_port_t port, 
                           soc_port_medium_t  medium,
                           soc_phy_config_t  *cfg)
{
    phy_ctrl_t    *pc;
    soc_phy_config_t *active_medium;  /* Currently active medium */
    soc_phy_config_t *change_medium;  /* Requested medium */
    soc_phy_config_t *other_medium;   /* The other medium */
    int               medium_update;
    soc_port_mode_t   advert_mask;

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
                uint16 data;

                SOC_IF_ERROR_RETURN
                    (READ_PHY54640_MODE_CTRLr(unit, pc, &data));
 
                /* return if not fiber capable*/
                if (!(data & 0x8)) {
                    return SOC_E_UNAVAIL;
                }
            }
        }

        change_medium  = &pc->copper;
        other_medium   = &pc->fiber;
        advert_mask    = ADVERT_ALL_COPPER;
        break;
    case SOC_PORT_MEDIUM_FIBER:
        if (!pc->automedium && !PHY_FIBER_MODE(unit, port)) {
            return SOC_E_UNAVAIL;
        }
        change_medium  = &pc->fiber;
        other_medium   = &pc->copper;
        advert_mask    = ADVERT_ALL_FIBER;
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
    change_medium->autoneg_advert &= advert_mask;

    if (medium_update) {
        /* The new configuration may cause medium change. Check
         * and update medium.
         */
        SOC_IF_ERROR_RETURN
            (_phy_54640_medium_change(unit, port, TRUE));
    } else {
        active_medium = (PHY_COPPER_MODE(unit, port)) ?  
                            &pc->copper : &pc->fiber;
        if (active_medium == change_medium) {
            /* If the medium to update is active, update the configuration */
            SOC_IF_ERROR_RETURN
                (_phy_54640_medium_config_update(unit, port, change_medium));
        }
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_medium_config_get
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
phy_54640_medium_config_get(int unit, soc_port_t port, 
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
 *      _phy_54640_reset_setup
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
_phy_54640_reset_setup(int unit, soc_port_t port)
{

    phy_ctrl_t    *pc;
    uint16         phy_addr;
    soc_port_t     primary_port;
    int            index;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* Reset the PHY with previously registered callback function. 
     */
    SOC_IF_ERROR_RETURN(soc_phy_reset(unit, port));

    if (soc_phy_primary_and_offset_get(unit, port, &primary_port, &index) == SOC_E_NONE) {
        SOC_IF_ERROR_RETURN
            (phy_54640_control_set( unit, port, SOC_PHY_CONTROL_PORT_PRIMARY, primary_port));
        SOC_IF_ERROR_RETURN
            (phy_54640_control_set( unit, port, SOC_PHY_CONTROL_PORT_OFFSET, index ));
        if (index == 0) {
            pc->flags  |= PHYCTRL_IS_PORT0;
            /* Switch to MDI registers */
            SOC_IF_ERROR_RETURN(
                PHY54640_REG_WRITE(unit, pc, 0, 0x0D01, 0x15, 0x0001));
        }
    } else {
        SOC_DEBUG_PRINT((DK_WARN, "_phy_54640_reset_setup: Config property 'phy_port_primary_and_offset' not set for u=%d, p=%d\n",
            unit, port));

        phy_addr = pc->phy_id;
        index = phy_addr & 3;

        /* set the default values that are valid for many boards */
        SOC_IF_ERROR_RETURN
            (phy_54640_control_set( unit, port, SOC_PHY_CONTROL_PORT_PRIMARY, 
            (port - index) < 0 ? 0:(port - index)));
        SOC_IF_ERROR_RETURN
            (phy_54640_control_set( unit, port, SOC_PHY_CONTROL_PORT_OFFSET, index ));
    }

    /* Disable super-isolate */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_MII_POWER_CTRLr(unit, pc, 0x0, 1U<<5));

    /* BCM54640E is EEE capable */
    if (PHY_IS_BCM54640E(pc)) {
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_EEE_CAPABLE);
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_modify(unit, pc, 0, 7,
                                                   0x0000, 0x0003));
        SOC_IF_ERROR_RETURN
            (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0F7F,0x15, 0x0, 0x7)); /* exp 7f */

    }

    if (ECD_ENABLE && ((PHY_IS_BCM54640E(pc)) && ((pc->phy_rev & 0x7) > 0x2)))  {
        SOC_IF_ERROR_RETURN(
            phy_ecd_cable_diag_init(unit, port));
    }

    SOC_IF_ERROR_RETURN
        (_phy_54640_no_reset_setup(unit, port));

    return SOC_E_NONE;
}

STATIC int
_phy_54640_no_reset_setup(int unit, soc_port_t port)
{
    phy_ctrl_t    *pc;
    uint16         data, mask;

    SOC_DEBUG_PRINT((DK_PHY, "_phy_54640_reset_setup: u=%d p=%d medium=%s\n",
                     unit, port,
                     PHY_COPPER_MODE(unit, port) ? "COPPER" : "FIBER"));

    pc      = EXT_PHY_SW_STATE(unit, port);

    /* SGMII to copper mode */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_MODE_CTRLr(unit, pc, 0x0, 0x0006));        
    /* copper regs */
    if (!pc->copper.enable || (!pc->automedium && pc->fiber.preferred)) {
        /* Copper interface is not used. Powered down. */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MII_CTRLr(unit, pc, MII_CTRL_PD, MII_CTRL_PD));
    } else {
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MII_CTRLr(unit, pc, 0, MII_CTRL_PD));
        SOC_IF_ERROR_RETURN
            (WRITE_PHY54640_MII_GB_CTRLr(unit, pc,  
                            MII_GB_CTRL_ADV_1000FD | MII_GB_CTRL_PT));
        SOC_IF_ERROR_RETURN
            (WRITE_PHY54640_MII_CTRLr(unit, pc, 
                MII_CTRL_FD | MII_CTRL_SS_10 | 
                MII_CTRL_SS_100 | MII_CTRL_AE));
    }

    /* Adjust timing and enable link speed led mode */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_SPARE_CTRLr(unit, pc, 0x0006, 0x0006));

    if (PHY_IS_BCM54640(pc)) {
        /* Invert 10BT TX clock */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MII_10BASE_Tr(unit, pc, 1U<<11, 1U<<11));
    }


    /* Power down SerDes */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_1000X_MII_CTRLr(unit, pc, MII_CTRL_PD, 
                                               MII_CTRL_PD));

    if (pc->fiber.enable && (pc->automedium || pc->fiber.preferred)) {
        /* remove power down of SerDes */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_1000X_MII_CTRLr(unit, pc, 0, MII_CTRL_PD));

        /* set the advertisement of serdes */
        /* Disable half-duplex, full-duplex capable */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_1000X_MII_ANAr(unit, pc, (1 << 5), 
                                                (1 << 5) | (1 << 6)));

        /* Enable auto-detection between SGMII-slave and 1000BASE-X */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_SGMII_SLAVEr(unit, pc, 1, 1));

        /* Restart SerDes autonegotiation */
        SOC_IF_ERROR_RETURN
            (WRITE_PHY54640_1000X_MII_CTRLr(unit, pc,
                MII_CTRL_FD | MII_CTRL_SS_1000 | MII_CTRL_AE));
        
        switch (pc->fiber_detect) {
            case  2:         /* LED 2 */
            case -2:
            case  4:         /* LED 4 : kept for backward compatibility */
            case -4:
            case  10:        /* EN_10B for signal detect */
            case -10:
            case 0:         /* default */
                /* GPIO Control/Status Register (0x1c shadow 01111) */
                /* Change LED2 pin into an input */
                SOC_IF_ERROR_RETURN
                    (MODIFY_PHY54640_LED_GPIO_CTRLr(unit, pc, 0x08, 0x08));
                break;
            case  1:         /* PECL , no such pin in 54640 data sheet */
            case -1:
            default:
                return SOC_E_CONFIG;
        }
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MISC_1000X_CONTROLr(unit, pc, 0x0000, 0x0020));
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MODE_CTRLr(unit, pc, 0x0002, 0x0006));        
    }

    /* Configure Auto-detect Medium (0x1c shadow 11110) */
    mask = 0x33f;               /* Auto-detect bit mask */
    data = 0;
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

    /* Qulalify fiber link with SYNC from PCS. */
    data |= 1 << 5;

    /*
     * Enable internal inversion of LED2/SD input pin.  Used only if
     * the fiber module provides RX_LOS instead of Signal Detect.
     */
    if (pc->fiber_detect < 0) {
         data |= 1 << 8;    /* Fiber signal detect is active low from pin */
    }

    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_AUTO_DETECT_MEDIUMr(unit, pc, data, mask));

#if AUTO_MDIX_WHEN_AN_DIS
    /* Enable Auto-MDIX When autoneg disabled */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_MII_MISC_CTRLr(unit, pc, 0x200, 0x200));
#endif

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
        (MODIFY_PHY54640_MII_ECRr(unit, pc, data, mask));

    /* Enable extended packet length (4.5k through 25k) */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, 0x4000, 0x4000));

    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_MISC_1000X_CTRL2r(unit, pc, 0x0001, 0x0001));

    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_AUX_CTRLr(unit, pc, 0x0002, 0x0002));

    /* Configure LED selectors */
    data = ((pc->ledmode[1] & 0xf) << 4) | (pc->ledmode[0] & 0xf);
    SOC_IF_ERROR_RETURN
        (WRITE_PHY54640_LED_SELECTOR_1r(unit, pc, data));

    data = ((pc->ledmode[3] & 0xf) << 4) | (pc->ledmode[2] & 0xf);
    SOC_IF_ERROR_RETURN
        (WRITE_PHY54640_LED_SELECTOR_2r(unit, pc, data));

    data = (pc->ledctrl & 0x3ff);
    SOC_IF_ERROR_RETURN
        (WRITE_PHY54640_LED_CTRLr(unit, pc, data));

    SOC_IF_ERROR_RETURN
        (WRITE_PHY54640_EXP_LED_SELECTORr(unit, pc, pc->ledselect));

    if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
        return SOC_E_NONE;
    }

    /* 100BASE-TX initialization change for EEE and autogreen mode */
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, 0x0c00, 0x0c00));
    SOC_IF_ERROR_RETURN(
        WRITE_PHY_REG(unit, pc, 0x17, 0x4022));
    SOC_IF_ERROR_RETURN(
        WRITE_PHY_REG(unit, pc, 0x15, 0x017b));
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, 0x0400, 0x0c00));


    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_init
 * Purpose:
 *      Init function for 54640 PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      SOC_E_NONE
 */

STATIC int
phy_54640_init(int unit, soc_port_t port)
{
    phy_ctrl_t        *pc;
    int                fiber_capable;
    int                fiber_preferred;

    SOC_DEBUG_PRINT((DK_PHY, "phy_54640_init: u=%d p=%d\n",
                     unit, port));

    /* Only SGMII interface to switch MAC is supported */ 
    if (IS_GMII_PORT(unit, port)) {
        return SOC_E_CONFIG; 
    }

    pc = EXT_PHY_SW_STATE(unit, port);

    pc->interface = SOC_PORT_IF_SGMII;

    fiber_capable = 1;

    fiber_preferred =
        soc_property_port_get(unit, port, spn_PHY_FIBER_PREF, 0);
    pc->automedium =
        soc_property_port_get(unit, port, spn_PHY_AUTOMEDIUM, 0);
    pc->fiber_detect =
        soc_property_port_get(unit, port, spn_PHY_FIBER_DETECT, -4);

    SOC_DEBUG_PRINT((DK_PHY, "phy_54640_init: "
            "u=%d p=%d type=54640%s automedium=%d fiber_pref=%d detect=%d\n",
            unit, port, fiber_capable ? "S" : "",
            pc->automedium, fiber_preferred, pc->fiber_detect));

    pc->copper.enable = TRUE;
    pc->copper.preferred = !fiber_preferred;
    pc->copper.autoneg_enable = TRUE;
    pc->copper.autoneg_advert = ADVERT_ALL_COPPER;
    SOC_IF_ERROR_RETURN
        (_phy_54640_ability_cu_local_get(unit, port, &(pc->copper.advert_ability)));
    pc->copper.advert_ability.medium = SOC_PA_MEDIUM_COPPER;
    pc->copper.force_speed = 1000;
    pc->copper.force_duplex = TRUE;
    pc->copper.master = SOC_PORT_MS_AUTO;
    pc->copper.mdix = SOC_PORT_MDIX_AUTO;

    pc->fiber.enable = fiber_capable;
    pc->fiber.preferred = fiber_preferred;
    pc->fiber.autoneg_enable = fiber_capable;
    pc->fiber.autoneg_advert = ADVERT_ALL_FIBER;
    SOC_IF_ERROR_RETURN
        (_phy_54640_ability_fiber_local_get(unit, port, &(pc->fiber.advert_ability)));
    pc->copper.advert_ability.medium = SOC_PA_MEDIUM_FIBER;
    pc->fiber.force_speed = 1000;
    pc->fiber.force_duplex = TRUE;
    pc->fiber.master = SOC_PORT_MS_NONE;
    pc->fiber.mdix = SOC_PORT_MDIX_NORMAL;

    /* Initially configure for the preferred medium. */
    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_COPPER);
    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_FIBER);
    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_PASSTHRU);
    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_100FX);
    PHY_FLAGS_CLR(unit, pc->port, PHY_FLAGS_EEE_ENABLED);
    PHY_FLAGS_CLR(unit, pc->port, PHY_FLAGS_EEE_MODE);

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

#ifdef BCM_WARM_BOOT_SUPPORT
    if (SOC_WARM_BOOT(unit)) {
        uint16 data;
        SOC_IF_ERROR_RETURN
                (READ_PHY54640_SERDES_100FX_CTRLr(unit, pc, &data));
        if (data & 0x1) {
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_100FX);
            pc->fiber.force_speed = 100;
            pc->fiber.autoneg_enable = FALSE;
            if (!(data & 0x2)) {
                pc->fiber.force_duplex = FALSE;
            }
        } else {
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_1000X_MII_CTRLr(unit, pc, &data));
            if (!(data & MII_CTRL_FD)) {
                pc->fiber.force_duplex = FALSE;
            }
            if (!(data & MII_CTRL_AE)) {
                pc->fiber.autoneg_enable = FALSE;
            }
        }
    }
#endif

    SOC_IF_ERROR_RETURN
        (_phy_54640_reset_setup(unit, port));

    SOC_IF_ERROR_RETURN
        (_phy_54640_medium_config_update(unit, port,
                                        PHY_COPPER_MODE(unit, port) ?
                                        &pc->copper :
                                        &pc->fiber));
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_enable_set
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
phy_54640_enable_set(int unit, soc_port_t port, int enable)
{
    uint16         power, mii_stat;
    soc_timeout_t  to;
    phy_ctrl_t    *pc;

    pc     = EXT_PHY_SW_STATE(unit, port);
    power  = (enable) ? 0 : MII_CTRL_PD;

    /* Do not incorrectly Power up the interface if medium is disabled and 
     * global enable = true
     */
    if (pc->copper.enable && (pc->automedium || PHY_COPPER_MODE(unit, port))) {
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MII_CTRLr(unit, pc, power, MII_CTRL_PD));

        if (!enable) {
            if ((PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) &&
               ((pc->phy_rev & 0x4) == 0x0) ){ /* A0,A1,B0,B1 */

                soc_timeout_init(&to, 2000000, 0);
                do {
                    SOC_IF_ERROR_RETURN
                        (READ_PHY54640_MII_STATr(unit, pc, &mii_stat));

                    if (soc_timeout_check(&to)) {
                        SOC_DEBUG_PRINT((DK_WARN,
                                 "phy54640.c: copper link didn't go down after power down: u=%d p=%d\n",
                                 unit, port));
                        break;
                    }
                } while(mii_stat & MII_STAT_LA);
            }
        }

        SOC_DEBUG_PRINT((DK_PHY, "phy_54640_enable_set: "
                "Power %s copper medium\n", (enable) ? "up" : "down"));
    }

    if (pc->fiber.enable && (pc->automedium || PHY_FIBER_MODE(unit, port))) {
        phy_ctrl_t  *int_pc;
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_1000X_MII_CTRLr(unit, pc, power, 
                                                   MII_CTRL_PD));

        int_pc = INT_PHY_SW_STATE(unit, port);
        if (NULL != int_pc) {
            SOC_IF_ERROR_RETURN
                (PHY_ENABLE_SET(int_pc->pd, unit, port, enable));
        }

        /*
         * When enable/disable the fiber port,
         * the Tx disable bit of Expansion register 0x52 should be set also.
         * Otherwise the following issue happens.
         * After the fiber port is powered down
         * (verified bit 11 MII cotnrol register 0 is set to 1),
         * the link partner still shows linkup
         */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_EXP_SERDES_SGMII_RX_CTRLr(unit, pc, enable?0:1, 0x1));
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_EXP_SERDES_SGMII_CTRLr(unit, pc, enable?0:1, 0x1));

        if (!enable) {
            soc_timeout_init(&to, 2000000, 0);
            do {
                SOC_IF_ERROR_RETURN
                    (READ_PHY54640_1000X_MII_STATr(unit, pc, &mii_stat));

                if (soc_timeout_check(&to)) {
                    SOC_DEBUG_PRINT((DK_WARN,
                             "phy54640.c: fiber link didn't go down after power down: u=%d p=%d\n",
                             unit, port));
                    break;
                }
            } while(mii_stat & MII_STAT_LA);
        }
    
        SOC_DEBUG_PRINT((DK_PHY, "phy_54640_enable_set: "
                "Power %s fiber medium\n", (enable) ? "up" : "down"));
    }

    /* Update software state */
    SOC_IF_ERROR_RETURN(phy_fe_ge_enable_set(unit, port, enable));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_enable_get
 * Purpose:
 *      Enable or disable the physical interface for a 54640 device.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      enable - (OUT) Boolean, true = enable PHY, false = disable.
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
phy_54640_enable_get(int unit, soc_port_t port, int *enable)
{
    return phy_fe_ge_enable_get(unit, port, enable);
}

/*
 * Function:
 *      _phy_54640_fiber_100fx_setup
 * Purpose:
 *      Switch from 1000X mode to 100FX.  
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
_phy_54640_fiber_100fx_setup(int unit, soc_port_t port) 
{
    phy_ctrl_t    *pc;

    pc          = EXT_PHY_SW_STATE(unit, port);

    SOC_DEBUG_PRINT((DK_PHY | DK_VERBOSE, 
                     "_phy_54640_1000x_to_100fx: u=%d p=%d \n",
                     unit, port));
    /* Operate SerDes in 100FX  */

    /* SerDes 100FX control register - register 1CH shadow 13H 
      * BIT0 : 100FX link SerDes enable
      * BIT1 : 100FX SerDes full duplex
      */
    if(pc->fiber.force_duplex) {
        SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_SERDES_100FX_CTRLr(unit, pc, 0x3, 0x3));
    } else {
        SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_SERDES_100FX_CTRLr(unit, pc, 0x1, 0x3));
    }

    /* Power up the SerDes      */ 
    SOC_IF_ERROR_RETURN
        (WRITE_PHY54640_1000X_MII_CTRLr(unit, pc, 
                        MII_CTRL_FD | MII_CTRL_SS_100 | 
                        (PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE) ? MII_CTRL_PD : 0)));

    PHY_FLAGS_SET(unit, port, PHY_FLAGS_100FX);
    pc->fiber.autoneg_enable = FALSE;

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_54640_fiber_1000x_setup
 * Purpose:
 *      Switch from 100FX mode to 1000X.  
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
_phy_54640_fiber_1000x_setup(int unit, soc_port_t port) 
{
    uint16         data;
    phy_ctrl_t    *pc;

    pc          = EXT_PHY_SW_STATE(unit, port);

    SOC_DEBUG_PRINT((DK_PHY | DK_VERBOSE,
                     "_phy_54640_fiber_1000x_setup: u=%d p=%d \n",
                      unit, port));

    /* Operate SerDes in 1000X  */
    SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_SERDES_100FX_CTRLr(unit, pc, 0x0, 0x1));

    /* Power up the SerDes      */ 
    if (pc->fiber.autoneg_enable) {
            data = MII_CTRL_AE | MII_CTRL_RAN | MII_CTRL_FD | MII_CTRL_SS_1000;
    } else {
            data = MII_CTRL_RAN | MII_CTRL_FD | MII_CTRL_SS_1000;
    }

    if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE)) { 
            data |= MII_CTRL_PD; 
    } 

    SOC_IF_ERROR_RETURN
        (WRITE_PHY54640_1000X_MII_CTRLr(unit, pc, data));

    /* When speed is 1000Mbps, set duplex to HD mode.
     * This is because our MAC does not support 1G half duplex.
     */
    pc->fiber.force_duplex = TRUE;
    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_100FX);

    return SOC_E_NONE;
}

STATIC int
_phy_54640_medium_change(int unit, soc_port_t port, int force_update)
{
    phy_ctrl_t    *pc;
    int            medium;

    pc    = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (_phy_54640_medium_check(unit, port, &medium));

    if (medium == SOC_PORT_MEDIUM_COPPER) {
        if ((!PHY_COPPER_MODE(unit, port)) || force_update) { /* Was fiber */ 
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_COPPER);
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_FIBER);
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_100FX);
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_MEDIUM_CHANGE);
            SOC_IF_ERROR_RETURN
                (_phy_54640_no_reset_setup(unit, port));

            /* Do not power up the interface if medium is disabled. */
            if (pc->copper.enable) {
                SOC_IF_ERROR_RETURN
                    (_phy_54640_medium_config_update(unit, port, &pc->copper));
            }
            SOC_DEBUG_PRINT((DK_PHY,
                         "_phy_54640_link_auto_detect: u=%d p=%d [F->C]\n",
                          unit, port));
        }
    } else {        /* Fiber */
        if (PHY_COPPER_MODE(unit, port) || force_update) { /* Was copper */
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_COPPER);
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_FIBER);
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_MEDIUM_CHANGE);
            SOC_IF_ERROR_RETURN
                (_phy_54640_no_reset_setup(unit, port));

            /*
             * When enable/disable the fiber port,
             * the Tx disable bit of Expansion register 0x52 should be set also.
             * Otherwise the following issue happens.
             * After the fiber port is powered down
             * (verified bit 11 MII cotnrol register 0 is set to 1),
             * the link partner still shows linkup
             */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EXP_SERDES_SGMII_RX_CTRLr(unit, pc,
                                                 pc->fiber.enable?0:1, 0x1));
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EXP_SERDES_SGMII_CTRLr(unit, pc,
                                                 pc->fiber.enable?0:1, 0x1));

            if (pc->fiber.enable) {
                SOC_IF_ERROR_RETURN
                    (_phy_54640_medium_config_update(unit, port, &pc->fiber));
            }
            SOC_DEBUG_PRINT((DK_PHY,
                          "_phy_54640_link_auto_detect: u=%d p=%d [C->F]\n",
                          unit, port));
        }
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_link_get
 * Purpose:
 *      Determine the current link up/down status for a 54640 device.
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
phy_54640_link_get(int unit, soc_port_t port, int *link)
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

    SOC_IF_ERROR_RETURN
        (READ_PHY54640_EXP_OPT_MODE_STATr(unit, pc, &data));

    if (!(data & (1U << 5))) { /* no sync */
        SOC_IF_ERROR_RETURN
            (READ_PHY54640_1000X_MII_ANPr(unit, pc, &data));

        if (data & MII_ANP_SGMII_MODE) { /* Link partner is SGMII master */
            /* Disable auto-detection between SGMII-slave and 1000BASE-X and set mode to 1000BASE-X */
            SOC_IF_ERROR_RETURN
                   (MODIFY_PHY54640_SGMII_SLAVEr(unit, pc, 0, 0x3));

            /* Now enable auto-detection between SGMII-slave and 1000BASE-X */
            SOC_IF_ERROR_RETURN
                   (MODIFY_PHY54640_SGMII_SLAVEr(unit, pc, 1, 1));
        }
    }

    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_MEDIUM_CHANGE);
    if (pc->automedium) {
        /* Check for medium change and update HW/SW accordingly. */
        SOC_IF_ERROR_RETURN
            (_phy_54640_medium_change(unit, port,FALSE));
    }
    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(phy_fe_ge_link_get(unit, port, link));
    } else {
        SOC_IF_ERROR_RETURN
            (READ_PHY54640_1000X_MII_STATr(unit, pc, &data));
        *link = (data & MII_STAT_LA) ? TRUE : FALSE;
    }

    /* If preferred medium is up, disable the other medium 
     * to prevent false link. */
    if (pc->automedium) {
        mask = MII_CTRL_PD;
        if (pc->copper.preferred) {
            if (pc->fiber.enable) {
                /* power down the serdes */
                data = (*link && PHY_COPPER_MODE(unit, port)) ? MII_CTRL_PD : 0;
            } else {
                data = MII_CTRL_PD;
            }
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_1000X_MII_CTRLr(unit, pc, data, mask));
        } else {  /* pc->fiber.preferred */
            if (pc->copper.enable) {
                data = (*link && PHY_FIBER_MODE(unit, port)) ? MII_CTRL_PD : 0;
            } else {
                data = MII_CTRL_PD;
            }
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_CTRLr(unit, pc, data, mask));
        }
    }

    SOC_DEBUG_PRINT((DK_PHY | DK_VERBOSE,
                     "phy_54640_link_get: u=%d p=%d mode=%s%s link=%d\n",
                      unit, port,
                      PHY_COPPER_MODE(unit, port) ? "C" : "F",
                      PHY_FIBER_100FX_MODE(unit, port)? "(100FX)" : "",
                      *link));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_duplex_set
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
phy_54640_duplex_set(int unit, soc_port_t port, int duplex)
{
    int                  rv;
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;
    if (PHY_COPPER_MODE(unit, port)) {
        /* Note: mac.c uses phy_5690_notify_duplex to update 5690 */
        rv = phy_fe_ge_duplex_set(unit, port, duplex);
        if (SOC_SUCCESS(rv)) {
            pc->copper.force_duplex = duplex;
        }
    } else {    /* Fiber */
        if (PHY_FIBER_100FX_MODE(unit, port)) {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_SERDES_100FX_CTRLr(unit, pc, duplex? 0x2:0x0, 0x2));
            pc->fiber.force_duplex = duplex;
        } else {
            if(!duplex) {
                return SOC_E_UNAVAIL;
            }
        }
    }

    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_54640_duplex_set: u=%d p=%d d=%d rv=%d\n",
                     unit, port, duplex, rv));

    return rv;
}

/*
 * Function:
 *      phy_54640_duplex_get
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
phy_54640_duplex_get(int unit, soc_port_t port, int *duplex)
{
    phy_ctrl_t    *pc;
    uint16         opt_mode;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        return phy_fe_ge_duplex_get(unit, port, duplex);    
    } else {    /* Fiber (1000BaseX, 100BaseFX, SGMII-Slave */
        SOC_IF_ERROR_RETURN
            (READ_PHY54640_EXP_OPT_MODE_STATr(unit, pc, &opt_mode));
        /* The bit 12 of register EXP_OPT_MODE_STAT is serdes duplex */
        *duplex = (opt_mode & 0x1000) ? TRUE : FALSE;
    }
        
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_speed_set
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
phy_54640_speed_set(int unit, soc_port_t port, int speed)
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
            rv = _phy_54640_fiber_100fx_setup(unit, port);
        } else  if (speed == 0 || speed == 1000) {
            rv = _phy_54640_fiber_1000x_setup(unit, port);
        } else {
            rv = SOC_E_CONFIG;
        }
        if (SOC_SUCCESS(rv)) {
            pc->fiber.force_speed = speed;
        }
    }

    SOC_DEBUG_PRINT((DK_PHY,
                     "phy_54640_speed_set: u=%d p=%d s=%d fiber=%d rv=%d\n",
                     unit, port, speed, PHY_FIBER_MODE(unit, port), rv));

    return rv;
}

/*
 * Function:
 *      phy_54640_speed_get
 * Purpose:
 *      Get the current operating speed for a 54640 device.
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
phy_54640_speed_get(int unit, soc_port_t port, int *speed)
{
    phy_ctrl_t    *pc;
    uint16         opt_mode;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        return phy_fe_ge_speed_get(unit, port, speed);
    } else {
        *speed = 1000;
        SOC_IF_ERROR_RETURN
            (READ_PHY54640_EXP_OPT_MODE_STATr(unit, pc, &opt_mode));
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
                            "phy_54640_speed_get: u=%d p=%d invalid speed\n",
                                unit, port));
        }
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_master_set
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
phy_54640_master_set(int unit, soc_port_t port, int master)
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
                    "phy_54640_master_set: u=%d p=%d master=%d fiber=%d rv=%d\n",
                    unit, port, master, PHY_FIBER_MODE(unit, port), rv));
    return rv;
}

/*
 * Function:
 *      phy_54640_master_get
 * Purpose:
 *      Get the current master mode for a 54640 device.
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
phy_54640_master_get(int unit, soc_port_t port, int *master)
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
 *      phy_54640_autoneg_set
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
phy_54640_autoneg_set(int unit, soc_port_t port, int autoneg)
{
    int                 rv;
    uint16              data, mask;
    phy_ctrl_t   *pc;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;
    if (PHY_COPPER_MODE(unit, port)) {
        rv = phy_fe_ge_an_set(unit, port, autoneg);
        if (SOC_SUCCESS(rv)) {
            pc->copper.autoneg_enable = autoneg ? 1 : 0;
        }
    } else { 
        if (autoneg) {
            /* When enabling autoneg, set the default speed to 1000Mbps first.
             * PHY will not enable autoneg if the PHY is in 100FX mode.
             */
            SOC_IF_ERROR_RETURN
                (phy_54640_speed_set(unit, port, 1000));
        }

        /* Disable/Enable auto-detection between SGMII-slave and 1000BASE-X */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_SGMII_SLAVEr(unit, pc, autoneg ?1:0, 1));

        SOC_IF_ERROR_RETURN
            (phy_54640_ability_advert_set(unit, port, &(pc->fiber.advert_ability)));

        data = MII_CTRL_FD;
        data |= (autoneg) ? MII_CTRL_AE | MII_CTRL_RAN : 0;
        mask = MII_CTRL_AE | MII_CTRL_RAN | MII_CTRL_FD;
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_1000X_MII_CTRLr(unit, pc, data, mask));


        pc->fiber.autoneg_enable = autoneg ? TRUE : FALSE;
    }
 
    SOC_DEBUG_PRINT((DK_PHY,
                     "phy_54640_autoneg_set: u=%d p=%d autoneg=%d rv=%d\n",
                     unit, port, autoneg, rv));

    return rv;
}

/*
 * Function:
 *      phy_54640_autoneg_get
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
phy_54640_autoneg_get(int unit, soc_port_t port,
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
            (READ_PHY54640_1000X_MII_CTRLr(unit, pc, &data));
        if (data & MII_CTRL_AE) {
            *autoneg = TRUE;

            SOC_IF_ERROR_RETURN
                (READ_PHY54640_1000X_MII_STATr(unit, pc, &data));
            if (data & MII_STAT_AN_DONE) {
                *autoneg_done = TRUE;
            }
        }
        
    }

    return rv;
}

/*
 * Function:
 *      phy_54640_ability_advert_set
 * Purpose:
 *      Set the current advertisement for auto-negotiation.
 * Parameters:
 *      unit    - StrataSwitch unit #.
 *      port    - StrataSwitch port #.
 *      ability - Port ability indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
phy_54640_ability_advert_set(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    int          rv;
    phy_ctrl_t   *pc;
    uint16       mii_adv, mii_gb_ctrl, eee_ability = 0;

    if (NULL == ability) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    if (PHY_COPPER_MODE(unit, port)) {

        mii_adv     = MII_ANA_ASF_802_3;

        SOC_IF_ERROR_RETURN
            (READ_PHY54640_MII_GB_CTRLr(unit, pc,  &mii_gb_ctrl));

        mii_gb_ctrl &= ~(MII_GB_CTRL_ADV_1000HD | MII_GB_CTRL_ADV_1000FD);
        mii_gb_ctrl |= MII_GB_CTRL_PT; /* repeater / switch port */

        if (ability->speed_half_duplex & SOC_PA_SPEED_10MB)  {
            mii_adv |= MII_ANA_HD_10;
        }
        if (ability->speed_half_duplex & SOC_PA_SPEED_100MB) {
            mii_adv |= MII_ANA_HD_100;
        }
        if (ability->speed_half_duplex & SOC_PA_SPEED_1000MB) {
            mii_gb_ctrl |= MII_GB_CTRL_ADV_1000HD;
        }
        if (ability->speed_full_duplex & SOC_PA_SPEED_10MB)  {
            mii_adv |= MII_ANA_FD_10;
        }
        if (ability->speed_full_duplex & SOC_PA_SPEED_100MB) {
            mii_adv |= MII_ANA_FD_100;
        }
        if (ability->speed_full_duplex & SOC_PA_SPEED_1000MB) {
            mii_gb_ctrl |= MII_GB_CTRL_ADV_1000FD;
        }

        switch (ability->pause & (SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX)) {
        case SOC_PA_PAUSE_TX:
            mii_adv |= MII_ANA_ASYM_PAUSE;
            break;
        case SOC_PA_PAUSE_RX:
            mii_adv |= MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE;
            break;
        case SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX:
            mii_adv |= MII_ANA_PAUSE;
            break;
        }

        /* EEE settings */
        if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            eee_ability = 0;
            if (ability->eee & SOC_PA_EEE_1GB_BASET) {
                eee_ability |= 0x4;
            }
            if (ability->eee & SOC_PA_EEE_100MB_BASETX) {
                eee_ability |= 0x2;
            }
            if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_ENABLED)) {
                SOC_IF_ERROR_RETURN
                    (MODIFY_PHY54640_EEE_ADVr(unit, pc, eee_ability, 0x0006));
            } else {
                SOC_IF_ERROR_RETURN
                    (MODIFY_PHY54640_EEE_ADVr(unit, pc, 0x0000, 0x0006));
            }
        }

        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MII_ANAr(unit, pc, mii_adv,
            MII_ANA_PAUSE|MII_ANA_ASYM_PAUSE|MII_ANA_FD_100|MII_ANA_HD_100|
            MII_ANA_FD_10|MII_ANA_HD_10|MII_ANA_ASF_802_3));

        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MII_GB_CTRLr(unit, pc, mii_gb_ctrl,
            MII_GB_CTRL_PT|MII_GB_CTRL_ADV_1000FD|MII_GB_CTRL_ADV_1000HD));
    } else { 
        uint16  mii_adv, data;

        SOC_IF_ERROR_RETURN
            (READ_PHY54640_1000X_MII_ANPr(unit, pc, &data));
        if (data & MII_ANP_SGMII_MODE) { /* Link partner is SGMII master */
            return SOC_E_NONE;
        }

        mii_adv = MII_ANA_C37_FD;  /* Always advertise 1000X full duplex */
        switch (ability->pause & (SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX)) {
        case SOC_PA_PAUSE_TX:
            mii_adv |= MII_ANA_C37_ASYM_PAUSE;
            break;
        case SOC_PA_PAUSE_RX:
            mii_adv |= (MII_ANA_C37_ASYM_PAUSE | MII_ANA_C37_PAUSE);
            break;
        case SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX:
            mii_adv |= MII_ANA_C37_PAUSE;
            break;
        }
        /* Write to SerDes Auto-Neg Advertisement Reg */
        rv = WRITE_PHY54640_1000X_MII_ANAr(unit, pc, mii_adv);
    }
    return rv;
}

/*
 * Function:
 *      phy_54640_ability_advert_get
 * Purpose:
 *      Get the current advertisement for auto-negotiation.
 * Parameters:
 *      unit    - StrataSwitch unit #.
 *      port    - StrataSwitch port #.
 *      ability - Port ability indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The advertisement is retrieved for the ACTIVE medium.
 *      No synchronization performed at this level.
 */

STATIC int
phy_54640_ability_advert_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    phy_ctrl_t *pc;
    uint16      mii_ana;
    uint16      mii_gb_ctrl;
    uint16      eee_ability = 0;

    if (NULL == ability) {
        return (SOC_E_PARAM);
    }
    sal_memset(ability, 0, sizeof(*ability));

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {

        SOC_IF_ERROR_RETURN
            (READ_PHY54640_MII_ANAr(unit, pc, &mii_ana));
        SOC_IF_ERROR_RETURN
            (READ_PHY54640_MII_GB_CTRLr(unit, pc, &mii_gb_ctrl));

        sal_memset(ability, 0, sizeof(*ability));

        if (mii_ana & MII_ANA_HD_10) {
           ability->speed_half_duplex |= SOC_PA_SPEED_10MB;
        }
        if (mii_ana & MII_ANA_HD_100) {
          ability->speed_half_duplex |= SOC_PA_SPEED_100MB;
        }
        if (mii_ana & MII_ANA_FD_10) {
          ability->speed_full_duplex |= SOC_PA_SPEED_10MB;
        }
        if (mii_ana & MII_ANA_FD_100) {
          ability->speed_full_duplex |= SOC_PA_SPEED_100MB;
        }

        switch (mii_ana & (MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE)) {
        case MII_ANA_PAUSE:
            ability->pause = SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX;
            break;
        case MII_ANA_ASYM_PAUSE:
            ability->pause = SOC_PA_PAUSE_TX;
            break;
        case MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE:
            ability->pause = SOC_PA_PAUSE_RX;
            break;
        }

        /* GE Specific values */

        if (mii_gb_ctrl & MII_GB_CTRL_ADV_1000HD) {
           ability->speed_half_duplex |= SOC_PA_SPEED_1000MB;
        }
        if (mii_gb_ctrl & MII_GB_CTRL_ADV_1000FD) {
           ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
        }

        /* EEE settings */
        if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_EEE_ADVr(unit, pc, &eee_ability));
            if (eee_ability & 0x04) {
                ability->eee |= SOC_PA_EEE_1GB_BASET;
            }
            if (eee_ability & 0x02) {
                ability->eee |= SOC_PA_EEE_100MB_BASETX;
            }
        }
    } else {    /* PHY_FIBER_MODE */
        uint16  mii_adv, data;
        SOC_IF_ERROR_RETURN
            (READ_PHY54640_1000X_MII_ANPr(unit, pc, &data));
        if (data & MII_ANP_SGMII_MODE) { /* Link partner is SGMII master */
            ability->speed_full_duplex = SOC_PA_SPEED_1000MB | SOC_PA_SPEED_100MB | SOC_PA_SPEED_10MB;
            ability->speed_half_duplex = SOC_PA_SPEED_100MB | SOC_PA_SPEED_10MB;
            ability->pause = SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX;
        } else {
            /* read the 1000BASE-X Auto-Neg Advertisement Reg ( Address 0x04 ) */
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_1000X_MII_ANAr(unit, pc, &mii_adv));

            if (mii_adv & MII_ANA_C37_FD) {
                ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
            }
            if (mii_adv & MII_ANA_C37_HD) {
                ability->speed_half_duplex |= SOC_PA_SPEED_1000MB;
            }
            switch (mii_adv & (MII_ANA_C37_PAUSE | MII_ANA_C37_ASYM_PAUSE)) {
            case MII_ANA_C37_PAUSE:
                ability->pause = SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX;
                break;
            case MII_ANA_C37_ASYM_PAUSE:
                ability->pause = SOC_PA_PAUSE_TX;
                break;
            case (MII_ANA_C37_PAUSE | MII_ANA_C37_ASYM_PAUSE):
                ability->pause = SOC_PA_PAUSE_RX;
                break;
            }
        }
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_ability_remote_get
 * Purpose:
 *      Get partners current advertisement for auto-negotiation.
 * Parameters:
 *      unit    - StrataSwitch unit #.
 *      port    - StrataSwitch port #.
 *      ability - Port ability indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The remote advertisement is retrieved for the ACTIVE medium.
 *      No synchronization performed at this level. If Autonegotiation is
 *      disabled or in progress, this routine will return an error.
 */

STATIC int
phy_54640_ability_remote_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    phy_ctrl_t       *pc;
    uint16            mii_stat;
    uint16            mii_anp;
    uint16            mii_gb_stat;
    uint16            eee_resolution;
    uint16            data;

    if (NULL == ability) {
        return (SOC_E_PARAM);
    }
    sal_memset(ability, 0, sizeof(*ability));

    pc = EXT_PHY_SW_STATE(unit, port);
    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (READ_PHY54640_MII_STATr(unit, pc, &mii_stat));

        if ((mii_stat & MII_STAT_AN_DONE & MII_STAT_LA)
                   == (MII_STAT_AN_DONE & MII_STAT_LA)) {
            /* Decode remote advertisement only when link is up and autoneg is
             * completed.
             */
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_MII_ANPr(unit, pc, &mii_anp));
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_MII_GB_STATr(unit, pc, &mii_gb_stat));

            if (mii_anp & MII_ANA_HD_10) {
                ability->speed_half_duplex |= SOC_PA_SPEED_10MB;
            }
            if (mii_anp & MII_ANA_HD_100) {
                ability->speed_half_duplex |= SOC_PA_SPEED_100MB;
            }
            if (mii_anp & MII_ANA_FD_10) {
               ability->speed_full_duplex |= SOC_PA_SPEED_10MB;
            }
            if (mii_anp & MII_ANA_FD_100) {
               ability->speed_full_duplex |= SOC_PA_SPEED_100MB;
            }

            switch (mii_anp & (MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE)) {
            case MII_ANA_PAUSE:
                ability->pause = SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX;
                break;
            case MII_ANA_ASYM_PAUSE:
                ability->pause = SOC_PA_PAUSE_TX;
                break;
            case MII_ANA_PAUSE | MII_ANA_ASYM_PAUSE:
                ability->pause = SOC_PA_PAUSE_RX;
                break;
           }

            /* GE Specific values */

            if (mii_gb_stat & MII_GB_STAT_LP_1000HD) {
            ability->speed_half_duplex |= SOC_PA_SPEED_1000MB;
            }
            if (mii_gb_stat & MII_GB_STAT_LP_1000FD) {
                ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
            }
            /* EEE settings */
            if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                SOC_IF_ERROR_RETURN
                (READ_PHY54640_EEE_RESOLUTION_STATr(unit, pc, &eee_resolution));
                if (eee_resolution & 0x04) {
                    ability->eee |= SOC_PA_EEE_1GB_BASET;
                }
                if (eee_resolution & 0x02) {
                    ability->eee |= SOC_PA_EEE_100MB_BASETX;
                }
            }

        } else {
            /* Simply return local abilities */
            phy_54640_ability_advert_get(unit, port, ability);
        }
    } else {    /* PHY_FIBER_MODE */

        /* read the 1000BASE-X     Control Reg ( Address 0x00 ),
         *   Auto-Neg Link Partner Ability Reg ( Address 0x05 )
         */
        SOC_IF_ERROR_RETURN
            (READ_PHY54640_1000X_MII_CTRLr(unit, pc, &data));
        if (!(data & MII_CTRL_AE)) {
            return SOC_E_DISABLED;
        }

        SOC_IF_ERROR_RETURN
            (READ_PHY54640_1000X_MII_ANPr(unit, pc, &data));

        if (data & MII_ANP_SGMII_MODE) {
            /* Unable to determine remote link partner's advertisement */ 
            return SOC_E_NONE;
        }
        if (data & MII_ANP_C37_FD) {
            ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
        }
        if (data & MII_ANP_C37_HD) {
            ability->speed_half_duplex |= SOC_PA_SPEED_1000MB;
        }
        switch (data & (MII_ANP_C37_PAUSE | MII_ANP_C37_ASYM_PAUSE)) {
        case MII_ANP_C37_PAUSE:
            ability->pause = SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX;
            break;
        case MII_ANP_C37_ASYM_PAUSE:
            ability->pause = SOC_PA_PAUSE_TX;
            break;
        case (MII_ANP_C37_PAUSE | MII_ANP_C37_ASYM_PAUSE):
            ability->pause = SOC_PA_PAUSE_RX;
            break;
        }
    }
    SOC_DEBUG_PRINT((DK_PHY,
         "phy_54640_ability_remote_get:unit=%d p=%d pause=%08x sp=%08x\n",
         unit, port, ability->pause, ability->speed_full_duplex));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_ability_local_get
 * Purpose:
 *      Get the device's complete abilities.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ability - return device's abilities.
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
_phy_54640_ability_cu_local_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    ability->speed_half_duplex  = SOC_PA_SPEED_100MB | SOC_PA_SPEED_10MB;
    ability->speed_full_duplex  = SOC_PA_SPEED_1000MB |SOC_PA_SPEED_100MB | 
                                  SOC_PA_SPEED_10MB;
                                  
    ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
    ability->interface = SOC_PA_INTF_SGMII;
    ability->medium    = SOC_PA_MEDIUM_COPPER; 
    ability->loopback  = SOC_PA_LB_PHY;
    ability->flags     = SOC_PA_AUTONEG;
    
    /* EEE settings */
    if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            ability->eee |= SOC_PA_EEE_1GB_BASET | SOC_PA_EEE_100MB_BASETX;
    }

    if (pc->automedium) {
        ability->flags     |= SOC_PA_COMBO;
    }
    return SOC_E_NONE;
}

STATIC int
_phy_54640_ability_fiber_local_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    phy_ctrl_t *pc;
    uint16  data;

    pc = EXT_PHY_SW_STATE(unit, port);
    

    SOC_IF_ERROR_RETURN
        (READ_PHY54640_1000X_MII_ANPr(unit, pc, &data));
    if (data & MII_ANP_SGMII_MODE) { /* Link partner is SGMII master */
        ability->speed_full_duplex = SOC_PA_SPEED_1000MB | SOC_PA_SPEED_100MB | SOC_PA_SPEED_10MB;
        ability->speed_half_duplex = SOC_PA_SPEED_100MB | SOC_PA_SPEED_10MB;
    } else {
        ability->speed_half_duplex  = SOC_PA_SPEED_100MB;
        ability->speed_full_duplex  = SOC_PA_SPEED_1000MB |SOC_PA_SPEED_100MB;
    }
    ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
    ability->interface = SOC_PA_INTF_SGMII;
    ability->medium    = SOC_PA_MEDIUM_FIBER; 
    ability->loopback  = SOC_PA_LB_PHY;
    ability->flags     = SOC_PA_AUTONEG;

    if (pc->automedium) {
        ability->flags     |= SOC_PA_COMBO;
    }
    return SOC_E_NONE;
}


STATIC int
phy_54640_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    if (NULL == ability) {
        return (SOC_E_PARAM);
    }
    sal_memset(ability, 0, sizeof(*ability));

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (_phy_54640_ability_cu_local_get(unit, port, ability));
    } else {
        SOC_IF_ERROR_RETURN
            (_phy_54640_ability_fiber_local_get(unit, port, ability));
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_lb_set
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
phy_54640_lb_set(int unit, soc_port_t port, int enable)
{
    int            rv;
    uint16         data;
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

#if AUTO_MDIX_WHEN_AN_DIS
    {
        /* Disable Auto-MDIX When autoneg disabled */
        /* Enable Auto-MDIX When autoneg disabled */
        data = (enable) ? 0x0000 : 0x0200;
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY_54640_MII_MISC_CTRLr(unit, pc, data, 0x0200)); 
    }
#endif

    if (PHY_COPPER_MODE(unit, port)) {
        rv = phy_fe_ge_lb_set(unit, port, enable);
    } else { 
        data = (enable) ? MII_CTRL_LE : 0;
        rv   = MODIFY_PHY54640_1000X_MII_CTRLr
                  (unit, pc, data, MII_CTRL_LE);
        if (enable && SOC_SUCCESS(rv)) {
            uint16        stat;
            int           link;
            soc_timeout_t to;

            /* Wait up to 5000 msec for link up */
            soc_timeout_init(&to, 5000000, 0);
            link = 0;
            while (!soc_timeout_check(&to)) {
                rv = READ_PHY54640_1000X_MII_STATr(unit, pc, &stat);
                link = stat & MII_STAT_LA;
                if (link || SOC_FAILURE(rv)) {
                    break;
                }
            }
            if (!link) {
                SOC_DEBUG_PRINT((DK_WARN,
                             "phy_54640_lb_set: u=%d p=%d TIMEOUT\n",
                             unit, port));

                rv = SOC_E_TIMEOUT;
            }
        }
    }
    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_54640_lb_set: u=%d p=%d en=%d rv=%d\n", 
                    unit, port, enable, rv));

    return rv; 
}

/*
 * Function:
 *      phy_54640_lb_get
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
phy_54640_lb_get(int unit, soc_port_t port, int *enable)
{
    uint16               data;
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (phy_fe_ge_lb_get(unit, port, enable));
    } else {
       SOC_IF_ERROR_RETURN
           (READ_PHY54640_1000X_MII_CTRLr(unit, pc, &data));
        *enable = (data & MII_CTRL_LE) ? 1 : 0;
    }

    return SOC_E_NONE; 
}

/*
 * Function:
 *      phy_54640_interface_set
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
phy_54640_interface_set(int unit, soc_port_t port, soc_port_if_t pif)
{
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(port);

    switch (pif) {
    case SOC_PORT_IF_SGMII:
    case SOC_PORT_IF_GMII:
        return SOC_E_NONE;
    default:
        return SOC_E_UNAVAIL;
    }
}

/*
 * Function:
 *      phy_54640_interface_get
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
phy_54640_interface_get(int unit, soc_port_t port, soc_port_if_t *pif)
{
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(port);

    *pif = SOC_PORT_IF_SGMII;

    return SOC_E_NONE;
}


/*
 * Function:
 *      phy_54640_mdix_set
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
phy_54640_mdix_set(int unit, soc_port_t port, soc_port_mdix_t mode)
{
    phy_ctrl_t    *pc;
    int                  speed;

    pc = EXT_PHY_SW_STATE(unit, port);
    if (PHY_COPPER_MODE(unit, port)) {
        switch (mode) {
        case SOC_PORT_MDIX_AUTO:
            /* Clear bit 14 for automatic MDI crossover */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_ECRr(unit, pc, 0, 0x4000));

            /* Clear bit 9 to disable forced auto MDI xover */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_MISC_CTRLr(unit, pc, 0, 0x0200));
            break;

        case SOC_PORT_MDIX_FORCE_AUTO:
            /* Clear bit 14 for automatic MDI crossover */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_ECRr(unit, pc, 0, 0x4000));

            /* Set bit 9 to disable forced auto MDI xover */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_MISC_CTRLr(unit, pc, 0x0200, 0x0200));
            break;

        case SOC_PORT_MDIX_NORMAL:
            SOC_IF_ERROR_RETURN(phy_54640_speed_get(unit, port, &speed));
            if (speed == 0 || speed == 10 || speed == 100) {
                /* Set bit 14 for automatic MDI crossover */
                SOC_IF_ERROR_RETURN
                    (MODIFY_PHY54640_MII_ECRr(unit, pc, 0x4000, 0x4000));

                SOC_IF_ERROR_RETURN
                    (WRITE_PHY54640_TEST1r(unit, pc, 0));
            } else {
                return SOC_E_UNAVAIL;
            }
            break;

        case SOC_PORT_MDIX_XOVER:
            SOC_IF_ERROR_RETURN(phy_54640_speed_get(unit, port, &speed));
            if (speed == 0 || speed == 10 || speed == 100) {
                /* Set bit 14 for automatic MDI crossover */
                SOC_IF_ERROR_RETURN
                    (MODIFY_PHY54640_MII_ECRr(unit, pc, 0x4000, 0x4000));

                SOC_IF_ERROR_RETURN
                    (WRITE_PHY54640_TEST1r(unit, pc, 0x0080));
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
 *      phy_54640_mdix_get
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
phy_54640_mdix_get(int unit, soc_port_t port, soc_port_mdix_t *mode)
{
    phy_ctrl_t    *pc;
    int            speed;

    if (mode == NULL) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(phy_54640_speed_get(unit, port, &speed));
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
 *      phy_54640_mdix_status_get
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
phy_54640_mdix_status_get(int unit, soc_port_t port, 
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
            (READ_PHY54640_MII_ESRr(unit, pc, &data));
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
_phy_54640_power_mode_set (int unit, soc_port_t port, int mode)
{
    phy_ctrl_t    *pc;

    pc       = EXT_PHY_SW_STATE(unit, port);

    if (pc->power_mode == mode) {
        return SOC_E_NONE;
    }

    if (mode == SOC_PHY_CONTROL_POWER_LOW) {
        /* enable dsp clock */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MII_AUX_CTRLr(unit,pc,0x0c00,0x0c00));

        /* enable low power 136 */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_AUTO_POWER_DOWNr(unit,pc,0x80,0x80));

        /* reduce tx bias current to -20% */
        SOC_IF_ERROR_RETURN
            (WRITE_PHY_REG(unit, pc, 0x17, 0x0f75));
        SOC_IF_ERROR_RETURN
            (WRITE_PHY_REG(unit, pc, 0x15, 0x1555));

        /* disable dsp clock */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MII_AUX_CTRLr(unit,pc,0x0400,0x0c00));
        pc->power_mode = mode;

    } else if (mode == SOC_PHY_CONTROL_POWER_FULL) {

        /* back to normal mode */
        /* enable dsp clock */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MII_AUX_CTRLr(unit,pc,0x0c00,0x0c00));

        /* disable low power 136 */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_AUTO_POWER_DOWNr(unit,pc,0x00,0x80));

        /* set tx bias current to nominal */
        SOC_IF_ERROR_RETURN
            (WRITE_PHY_REG(unit, pc, 0x17, 0x0f75));
        SOC_IF_ERROR_RETURN
            (WRITE_PHY_REG(unit, pc, 0x15, 0x0));

        /* disable dsp clock */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_MII_AUX_CTRLr(unit,pc,0x0400,0x0c00));
        pc->power_mode = mode;
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_54640_eee_enable
 * Purpose:
 *      Enable or disable EEE (Native)
 * Parameters:
 *      unit   - StrataSwitch unit #.
 *      port   - StrataSwitch port #.
 *      enable - Enable Native EEE
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
_phy_54640_eee_enable(int unit, soc_port_t port, int enable)
{
    phy_ctrl_t *pc;
    int rv = SOC_E_NONE;
    
    pc = EXT_PHY_SW_STATE(unit, port);
    if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
        rv = SOC_E_FAIL;
    }
    if (enable == 1) {
        if (PHY_IS_BCM54640E_A0A1(pc)) {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, 0x0c00, 0x0c00));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x0ffe));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x0100));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x0fff));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x4000));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x2022));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x01f1));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x4021));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x0887));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x2021));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x8983));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x0021));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x4600));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x0e40));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x0000));

            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, 0x0400, 0x0c00));

            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_TEST_CTRL_Br(unit, pc, 0xc000, 0xc000));
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_TEST_CTRL_Ar(unit, pc, 0xf300, 0xf300));
        } else {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_803Dr(unit, pc, 0xc000, 0xc000)); /* 7.803d */
        }
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_EEE_ADVr(unit, pc, 0x0006, 0x0006));
        pc->copper.advert_ability.eee |= (SOC_PA_EEE_1GB_BASET | SOC_PA_EEE_100MB_BASETX);

        SOC_IF_ERROR_RETURN
            (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 0x1, 0x1)); /* exp af */
    } else {
        if (PHY_IS_BCM54640E_A0A1(pc)) {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_TEST_CTRL_Br(unit, pc, 0x0000, 0xc000));
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_TEST_CTRL_Ar(unit, pc, 0x0000, 0xf300));
        } else {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_803Dr(unit, pc, 0x0000, 0xc000)); /* 7.803d */
        }
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_EEE_ADVr(unit, pc, 0x0000, 0x0006));
        pc->copper.advert_ability.eee &= ~(SOC_PA_EEE_1GB_BASET | SOC_PA_EEE_100MB_BASETX);

        SOC_IF_ERROR_RETURN
            (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 0x0, 0x1)); /* exp af */
    }

    return rv;
}

/*
 * Function:
 *      _phy_54640_eee_auto_enable
 * Purpose:
 *      Enable or disable EEE (Native)
 * Parameters:
 *      unit   - StrataSwitch unit #.
 *      port   - StrataSwitch port #.
 *      enable - Enable Native EEE
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
_phy_54640_eee_auto_enable(int unit, soc_port_t port, int enable)
{
    phy_ctrl_t *pc;
    int rv = SOC_E_NONE;
    
    pc = EXT_PHY_SW_STATE(unit, port);
    if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
        rv = SOC_E_FAIL;
    }

    if (enable == 1) {

        if (PHY_IS_BCM54640E_A0A1(pc)) {

            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, 0x0c00, 0x0c00));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x0ffe));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x0900));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x0fff));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x4000));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x2022));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x01f1));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x2021));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x8983));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x4021));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x0887));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x0021));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x4600));

            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, 0x0400, 0x0c00));

            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_SGMII_SLAVEr(unit, pc, 0x02, 0x02));

            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MODE_CTRLr(unit, pc, 0x00, 0x06));

            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x17, 0x0faf));
            SOC_IF_ERROR_RETURN(
                WRITE_PHY_REG(unit, pc, 0x15, 0x0001));

            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_TEST_CTRL_Br(unit, pc, 0xc000, 0xc000));

            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_TEST_CTRL_Ar(unit, pc, 0xf300, 0x1300));

        } else {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_803Dr(unit, pc, 0xc000, 0xc000)); /* 7.803d */
        }
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_EEE_ADVr(unit, pc, 0x0006, 0x0006));
        pc->copper.advert_ability.eee |= (SOC_PA_EEE_1GB_BASET | SOC_PA_EEE_100MB_BASETX);

        SOC_IF_ERROR_RETURN
           (_phy_54640e_blk_top_lvl_reg_modify(unit, pc, 0, 7,
                                                   0x0001, 0x0003));
        SOC_IF_ERROR_RETURN
            (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 0x1, 0x3)); /* exp af */

    } else {
        SOC_IF_ERROR_RETURN
            (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 0x0, 0x3)); /* exp af */
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY54640_EEE_ADVr(unit, pc, 0x0000, 0x0006));
        pc->copper.advert_ability.eee &= ~(SOC_PA_EEE_1GB_BASET | SOC_PA_EEE_100MB_BASETX);

        if (PHY_IS_BCM54640E_A0A1(pc)) {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_TEST_CTRL_Br(unit, pc, 0x0000, 0xc000)); /* 7.8031 */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_TEST_CTRL_Ar(unit, pc, 0x0000, 0xf300)); /* 7.8030 2 ops */
        } else {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EEE_803Dr(unit, pc, 0x0000, 0xc000)); /* 7.803d */
        }
    }

    return rv;
}

/*
 * Function:
 *      phy_54640_toplvl_reg_read
 * Purpose:
 *      Read a top level register from a supporting chip
 * Parameters:
 *      unit - SOC Unit #.
 *      port - Port number.
 *      primary_port - Primary Port number.
 *      reg_offset  - Offset to the reg
 *      data  - Pointer to data returned
 * Returns:
 *      SOC_E_XXX
 */
int
phy_54640_toplvl_reg_read(int unit, soc_port_t port, soc_port_t primary_port, 
                          uint8 reg_offset, uint16 *data)

{
    phy_ctrl_t    *pc, *pc_port1, *pc_port3;
    uint16 reg_data, status;
    soc_phy_chip_info_t *chip_info;
    int         rv = SOC_E_NONE;

    if ((primary_port == -1) || (SOC_PHY_INFO(unit, primary_port).chip_info == NULL)) {
        return SOC_E_FAIL;
    }

    chip_info = SOC_PHY_INFO(unit, primary_port).chip_info;

    if ((chip_info->offset_to_port[0] == -1) ||
        (chip_info->offset_to_port[2] == -1)) {

        return SOC_E_FAIL;
    }

    pc       = EXT_PHY_SW_STATE(unit, port);
    pc_port1 = EXT_PHY_SW_STATE(unit, chip_info->offset_to_port[0]);
    pc_port3 = EXT_PHY_SW_STATE(unit, chip_info->offset_to_port[2]);

    if ((!pc) || (!pc_port1) || (!pc_port3)) {
        return SOC_E_FAIL;
    }

    reg_data = ((0xAC00 | reg_offset) & 0x7f);
    INT_MCU_LOCK(unit);
    /* Switch to MAC SGMII */
    if (PHY_IS_BCM54640E(pc)) {
        /* Write register 0x17 with the top level reg offset to write (handled in PHY54640_REG_WRITE) */
        /* Write register 0x15 the value to write to the top level register offset */
        rv = PHY54640_REG_WRITE(unit, pc_port1, 0, 0x0D01, 0x15, 0x0001);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }
        rv = PHY54640_REG_WRITE(unit, pc_port1, 0, 0x0D01, 0x15, 0x0003);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }
    } else {
        rv = pc->write(unit, pc_port1->phy_id, 0x1c, 0xd040);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }

        rv = pc->write(unit, pc_port1->phy_id, 0x1c, 0xd020);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }
    }

    rv = pc->write(unit, pc_port1->phy_id, 0x1c, reg_data);
    if (SOC_FAILURE(rv)) {
        INT_MCU_UNLOCK(unit);
        return rv;
    }

    /* Switch back to MDI */
    if (PHY_IS_BCM54640E(pc)) {
        /* Write register 0x17 with the top level reg offset to write (handled in PHY54640_REG_WRITE) */
        /* Write register 0x15 the value to write to the top level register offset */
        rv = PHY54640_REG_WRITE(unit, pc_port1, 0, 0x0D01, 0x15, 0x0001);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }
    } else {
        rv = pc->write(unit, pc_port1->phy_id, 0x1c, 0xd04f);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }

        rv = pc->write(unit, pc_port1->phy_id, 0x1c, 0xd00f);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }

        rv = pc->write(unit, pc_port1->phy_id, 0x1c, 0xd000);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }
    }

    /* Read data from Top level MII Status register(0x15h) */
    rv = pc->write(unit, pc_port3->phy_id, 0x17, 0x8F0B);
    if (SOC_FAILURE(rv)) {
        INT_MCU_UNLOCK(unit);
        return rv;
    }
    rv = pc->read(unit, pc_port3->phy_id, 0x15, &status);
    INT_MCU_UNLOCK(unit);
    if (SOC_FAILURE(rv)) {
        return rv;
    }
    *data = (status & 0xff);

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_toplvl_reg_write
 * Purpose:
 *      Write to a top level register from a supporting chip
 * Parameters:
 *      unit - SOC Unit #.
 *      port - Port number.
 *      primary_port - Primary Port number.
 *      reg_offset  - Offset to the reg
 *      data  - Data to be written
 * Returns:
 *      SOC_E_XXX
 */
int
phy_54640_toplvl_reg_write(int unit, soc_port_t port, soc_port_t primary_port,
                           uint8 reg_offset, uint16 data)

{
    phy_ctrl_t    *pc, *pc_port1, *pc_port2;
    uint16 reg_data;
    soc_phy_chip_info_t *chip_info;
    int         rv = SOC_E_NONE;

    if ((primary_port == -1) || (SOC_PHY_INFO(unit, primary_port).chip_info == NULL)) {
        return SOC_E_FAIL;
    }

    chip_info = SOC_PHY_INFO(unit, primary_port).chip_info;

    if ((chip_info->offset_to_port[0] == -1) ||
        (chip_info->offset_to_port[1] == -1)) {

        return SOC_E_FAIL;
    }

    pc       = EXT_PHY_SW_STATE(unit, port);
    pc_port1 = EXT_PHY_SW_STATE(unit, chip_info->offset_to_port[0]);
    pc_port2 = EXT_PHY_SW_STATE(unit, chip_info->offset_to_port[1]);

    if ((!pc) || (!pc_port1) || (!pc_port2)) {
        return SOC_E_FAIL;
    }


    /* Write Data to port2, register 0x1C, shadow 0x0c */
    INT_MCU_LOCK(unit);

    /* Switch to MAC SGMII */
    if (PHY_IS_BCM54640E(pc)) {
        /* Write register 0x17 with the top level reg offset to write (handled in PHY54640_REG_WRITE) */
        /* Write register 0x15 the value to write to the top level register offset */
        rv = PHY54640_REG_WRITE(unit, pc_port1, 0, 0x0D01, 0x15, 0x0001);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }
        rv = PHY54640_REG_WRITE(unit, pc_port1, 0, 0x0D01, 0x15, 0x0003);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }
    } else {
        rv = pc->write(unit, pc_port1->phy_id, 0x1c, 0xd040);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }
        rv = pc->write(unit, pc_port1->phy_id, 0x1c, 0xd020);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }
    }
    reg_data = (0xB000 | (data & 0xff));
    rv = pc->write(unit, pc_port2->phy_id, 0x1c, reg_data);
    if (SOC_FAILURE(rv)) {
        INT_MCU_UNLOCK(unit);
        return rv;
    }

    /* Write Reg address to Port 1's register 0x1C, shadow 0x0B */
    /* Enable Write ( Port 1's register 0x1C, shadow 0x0B) Bit 7 = 1 */
    reg_data = (0xAC80 | reg_offset);
    rv = pc->write(unit, pc_port1->phy_id, 0x1c, reg_data);
    if (SOC_FAILURE(rv)) {
        INT_MCU_UNLOCK(unit);
        return rv;
    }

    /* Disable Write ( Port 1's register 0x1C, shadow 0x0B) Bit 7 = 0 */
    reg_data = (0xAC00 | reg_offset);
    rv = pc->write(unit, pc_port1->phy_id, 0x1c, reg_data);
    if (SOC_FAILURE(rv)) {
        INT_MCU_UNLOCK(unit);
        return rv;
    }

    /* Switch to MDI SGMII */
    if (PHY_IS_BCM54640E(pc)) {
        /* Write register 0x17 with the top level reg offset to write (handled in PHY54640_REG_WRITE) */
        /* Write register 0x15 the value to write to the top level register offset */
        rv = PHY54640_REG_WRITE(unit, pc_port1, 0, 0x0D01, 0x15, 0x0001);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }
    } else {
        rv = pc->write(unit, pc_port1->phy_id, 0x1c, 0xd04f);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }

        rv = pc->write(unit, pc_port1->phy_id, 0x1c, 0xd00f);
        if (SOC_FAILURE(rv)) {
            INT_MCU_UNLOCK(unit);
            return rv;
        }

        rv = pc->write(unit, pc_port1->phy_id, 0x1c, 0xd000);
    }

    INT_MCU_UNLOCK(unit);

    return rv;
}


STATIC int
phy_54640_control_linkdown_transmit_set(int unit, soc_port_t port, 
                                       uint32 value)
{
    phy_ctrl_t  *pc;
    int         speed;             
    int         rv = SOC_E_NONE;

    if (PHY_COPPER_MODE(unit, port)) {
        return SOC_E_UNAVAIL;
    }

    SOC_IF_ERROR_RETURN
        (phy_54640_speed_get(unit, port, &speed));

    pc = EXT_PHY_SW_STATE(unit, port);

    switch (speed) {
        
    case 1000:

        if (value) {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_1000X_MII_CTRLr(unit, pc, 0, MII_CTRL_AE));
            /* clear bits 1,2,3,5 and 6 */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MISC_1000X_CTRL2r(unit, pc, 0x0000, 0x006e));
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_1000X_MII_CTRLr(unit, pc, (1U << 5), (1U << 5)));
        } else {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_1000X_MII_CTRLr(unit, pc, 0, (1U << 5)));
            SOC_IF_ERROR_RETURN
                (WRITE_PHY54640_1000X_MII_CTRLr(unit, pc, 0x1140));
            SOC_IF_ERROR_RETURN
                (WRITE_PHY54640_1000X_MII_CTRLr(unit, pc, 0x1940));
            /* set bits 1,2,3 and 5 while clearing bit 6 */
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MISC_1000X_CTRL2r(unit, pc, 0x002e, 0x006e));
            if (!pc->fiber.autoneg_enable) {
                SOC_IF_ERROR_RETURN
                    (MODIFY_PHY54640_1000X_MII_CTRLr(unit, pc, 0, MII_CTRL_AE));
            }
        }
        break;
        
    default:
        rv = SOC_E_UNAVAIL;
    }

    return rv;
}

STATIC int
phy_54640_control_linkdown_transmit_get(int unit, soc_port_t port, 
                                       uint32 *value)
{
    phy_ctrl_t  *pc;
    int         speed;             
    uint16      data;
    int         rv = SOC_E_NONE;

    if (PHY_COPPER_MODE(unit, port)) {
        return SOC_E_UNAVAIL;
    }

    SOC_IF_ERROR_RETURN
        (phy_54640_speed_get(unit, port, &speed));

    pc = EXT_PHY_SW_STATE(unit, port);

    switch (speed) {

    case 1000:
        /* Check 1000-X Uni-Tx bit */
        SOC_IF_ERROR_RETURN
            (READ_PHY54640_1000X_MII_CTRLr(unit, pc, &data));
        *value = (data & (1U << 5)) ? 1 : 0;
        break;

    default:
        rv = SOC_E_UNAVAIL;
    }

    return rv;
}


/*
 * Function:
 *      phy_54640_control_set
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
phy_54640_control_set(int unit, soc_port_t port,
                     soc_phy_control_t type, uint32 value)
{
    int rv;
    phy_ctrl_t *pc;
    uint16 data;
    soc_port_t primary;
    int offset, saved_autoneg;

    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;
    switch(type) {

        case SOC_PHY_CONTROL_POWER:
            rv = _phy_54640_power_mode_set(unit,port,value);
        break;

        case SOC_PHY_CONTROL_PORT_PRIMARY:
                SOC_IF_ERROR_RETURN
                    (soc_phyctrl_primary_set(unit, port, (soc_port_t)value));
                break;

        case SOC_PHY_CONTROL_PORT_OFFSET:
                SOC_IF_ERROR_RETURN
                    (soc_phyctrl_offset_set(unit, port, (int)value));
            break;

        case SOC_PHY_CONTROL_CLOCK_ENABLE:
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_primary_get(unit, port, &primary));
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_offset_get(unit, port, &offset));
        SOC_IF_ERROR_RETURN
            (phy_54640_toplvl_reg_read(unit, port, primary, 0x0, &data));
        if ( value ) {
            SOC_IF_ERROR_RETURN
                (phy_54640_toplvl_reg_write(unit, port, primary, 0x0,
                     ( data & 0xf0 ) | ( offset & 0x7 )));
        } else {
            SOC_IF_ERROR_RETURN
                (phy_54640_toplvl_reg_write(unit, port, primary, 0x0,
                     ( data & 0xf0 ) | 0x8 ));
        }
        break;

        case SOC_PHY_CONTROL_CLOCK_SECONDARY_ENABLE:
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_primary_get(unit, port, &primary));
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_offset_get(unit, port, &offset));
        SOC_IF_ERROR_RETURN
            (phy_54640_toplvl_reg_read(unit, port, primary, 0x0, &data));
        if ( value ) {
            SOC_IF_ERROR_RETURN
                (phy_54640_toplvl_reg_write(unit, port, primary, 0x0,
                     ( data & 0x0f ) | (( offset & 0x7 )<<4 )));
        } else {
            SOC_IF_ERROR_RETURN
                (phy_54640_toplvl_reg_write(unit, port, primary, 0x0,
                     ( data & 0x0f ) | (0x8<<4) ));
        }
        break;

        case SOC_PHY_CONTROL_LINKDOWN_TRANSMIT:
            rv = phy_54640_control_linkdown_transmit_set(unit, port, value);
            break;

        case SOC_PHY_CONTROL_EEE:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }

            if (value == 1) {
                /* Enable Native EEE */
                SOC_IF_ERROR_RETURN
                    (_phy_54640_eee_enable(unit, port, 1));
                saved_autoneg = pc->copper.autoneg_enable;
                /* Initiate AN */
                SOC_IF_ERROR_RETURN
                    (phy_54640_autoneg_set(unit, port, 1));
                pc->copper.autoneg_enable = saved_autoneg;
                PHY_FLAGS_SET(unit, pc->port, PHY_FLAGS_EEE_ENABLED);
                PHY_FLAGS_CLR(unit, pc->port, PHY_FLAGS_EEE_MODE);
            } else {
                SOC_IF_ERROR_RETURN
                    (_phy_54640_eee_enable(unit, port, 0));
                /* Initiate AN if administratively enabled */
                SOC_IF_ERROR_RETURN
                    (phy_54640_autoneg_set(unit, port, pc->copper.autoneg_enable ? 1 : 0));
                PHY_FLAGS_CLR(unit, pc->port, PHY_FLAGS_EEE_ENABLED);
            }

            break;

        case SOC_PHY_CONTROL_EEE_AUTO:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }
            if (value == 1) {
                /* Disable Native EEE */
                SOC_IF_ERROR_RETURN
                    (_phy_54640_eee_enable(unit, port, 0));
                /* Enable Auto EEE */
                SOC_IF_ERROR_RETURN
                    (_phy_54640_eee_auto_enable(unit, port, 1));
                saved_autoneg = pc->copper.autoneg_enable;
                /* Initiate AN */
                SOC_IF_ERROR_RETURN
                    (phy_54640_autoneg_set(unit, port, 1));
                pc->copper.autoneg_enable = saved_autoneg;
                PHY_FLAGS_SET(unit, pc->port, PHY_FLAGS_EEE_ENABLED);
                PHY_FLAGS_SET(unit, pc->port, PHY_FLAGS_EEE_MODE);
            } else {
                /* Disable Auto EEE */
                SOC_IF_ERROR_RETURN
                    (_phy_54640_eee_auto_enable(unit, port, 0));
                /* Initiate AN if administratively enabled */
                SOC_IF_ERROR_RETURN
                    (phy_54640_autoneg_set(unit, port, pc->copper.autoneg_enable ? 1 : 0));
                PHY_FLAGS_CLR(unit, pc->port, PHY_FLAGS_EEE_ENABLED);
            }
            break;

        case SOC_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }

            if (value > 7) { /* Values needs to between 0 to 7 (inclusive) */
                rv = SOC_E_CONFIG;
            }

            SOC_IF_ERROR_RETURN
               (_phy_54640e_blk_top_lvl_reg_modify(unit, pc, 0, 7,
                                                       value<<8, 0x0700));
            /* Setting GPHY core, MII buffer register as well */
            SOC_IF_ERROR_RETURN
               (MODIFY_PHY54640_EXP_MII_BUF_CNTL_STAT1r(unit, pc, 
                                                        value<<8, 0x0700));
            break;


        case SOC_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }
            if (value == 1) { /* Sets Fixed Latency */
                SOC_IF_ERROR_RETURN
                   (_phy_54640e_blk_top_lvl_reg_modify(unit, pc, 0, 7,
                                                       0x0000, 0x0004));
                /* Setting GPHY core, MII buffer register as well */
                SOC_IF_ERROR_RETURN
                   (MODIFY_PHY54640_EXP_MII_BUF_CNTL_STAT1r(unit, pc, 
                                                            0x0000, 0x0004));
            } else { /* Sets Variable Latency */
                SOC_IF_ERROR_RETURN
                   (_phy_54640e_blk_top_lvl_reg_modify(unit, pc, 0, 7,
                                                       0x0004, 0x0004));
                /* Setting GPHY core, MII buffer register as well */
                SOC_IF_ERROR_RETURN
                   (MODIFY_PHY54640_EXP_MII_BUF_CNTL_STAT1r(unit, pc, 
                                                            0x0004, 0x0004));
            }
            break;


        case SOC_PHY_CONTROL_EEE_AUTO_BUFFER_LIMIT:
            /* Buffer limit modification is not allowed */

        case SOC_PHY_CONTROL_EEE_TRANSMIT_WAKE_TIME:
        case SOC_PHY_CONTROL_EEE_RECEIVE_WAKE_TIME:
        case SOC_PHY_CONTROL_EEE_TRANSMIT_SLEEP_TIME:
        case SOC_PHY_CONTROL_EEE_RECEIVE_SLEEP_TIME:
        case SOC_PHY_CONTROL_EEE_TRANSMIT_QUIET_TIME:
        case SOC_PHY_CONTROL_EEE_RECEIVE_QUIET_TIME:
        case SOC_PHY_CONTROL_EEE_TRANSMIT_REFRESH_TIME:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }

            /* Time modification not allowed, IEEE EEE spec constants */
            rv = SOC_E_UNAVAIL;
            break;

        case SOC_PHY_CONTROL_EEE_STATISTICS_CLEAR:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }
            
            if (value == 1) {
                /* Clearing the stats: Enable clear on Read  */
                SOC_IF_ERROR_RETURN
                   (MODIFY_PHY54640_EXP_MII_BUF_CNTL_STAT1r(unit, pc, 
                                                            0x1000, 0x1000));

                /* Read all the stats to clear */
                SOC_IF_ERROR_RETURN
                    (READ_PHY54640_EXP_EEE_TX_EVENTSr(unit, pc, &data));
                SOC_IF_ERROR_RETURN
                    (READ_PHY54640_EXP_EEE_TX_EVENTSr(unit, pc, &data));
                SOC_IF_ERROR_RETURN
                    (READ_PHY54640_EXP_EEE_TX_DURATIONr(unit, pc, &data));
                SOC_IF_ERROR_RETURN
                    (READ_PHY54640_EXP_EEE_TX_DURATIONr(unit, pc, &data));
                SOC_IF_ERROR_RETURN
                    (READ_PHY54640_EXP_EEE_RX_EVENTSr(unit, pc, &data));
                SOC_IF_ERROR_RETURN
                    (READ_PHY54640_EXP_EEE_RX_EVENTSr(unit, pc, &data));
                SOC_IF_ERROR_RETURN
                    (READ_PHY54640_EXP_EEE_RX_DURATIONr(unit, pc, &data));
                SOC_IF_ERROR_RETURN
                    (READ_PHY54640_EXP_EEE_RX_DURATIONr(unit, pc, &data));

                /* Disable Clear on Read  */
                SOC_IF_ERROR_RETURN
                   (MODIFY_PHY54640_EXP_MII_BUF_CNTL_STAT1r(unit, pc, 
                                                            0x0000, 0x1000));
            }

            break;
    case SOC_PHY_CONTROL_LOOPBACK_EXTERNAL:
        if (value) {
            int saved_autoneg, saved_speed;

            saved_autoneg = pc->copper.autoneg_enable;
            SOC_IF_ERROR_RETURN
                (phy_54640_autoneg_set(unit, port, 0));
            pc->copper.autoneg_enable = saved_autoneg;
            
            saved_speed = pc->copper.force_speed;
            SOC_IF_ERROR_RETURN
                (phy_54640_speed_set(unit, port, 1000));
            pc->copper.force_speed = saved_speed;

            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_GB_CTRLr(unit, pc, (1U << 12 | 1U << 11), (1U << 12 | 1U << 11)));
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, (1U << 15 | 1U << 10), (1U << 15 | 1U << 10)));
        } else {
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_GB_CTRLr(unit, pc, 0, (1U << 12 | 1U << 11)));
            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, 0, (1U << 15 | 1U << 10)));
            SOC_IF_ERROR_RETURN
                (phy_54640_speed_set(unit, port, pc->copper.force_speed));
            SOC_IF_ERROR_RETURN
                (phy_54640_autoneg_set(unit, port, pc->copper.autoneg_enable ? 1 : 0));
        }
        break;
    default:
        rv = SOC_E_UNAVAIL;
        break;
    }
    return rv;
}
/*
 * Function:
 *      phy_54640_control_get
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
phy_54640_control_get(int unit, soc_port_t port,
                     soc_phy_control_t type, uint32 *value)
{
    int rv;
    phy_ctrl_t *pc;
    uint16 data;
    uint32 temp;
    soc_port_t primary;
    int offset;


    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;
    switch(type) {
        case SOC_PHY_CONTROL_POWER:
            *value = pc->power_mode;
        break;

        case SOC_PHY_CONTROL_PORT_PRIMARY:
                SOC_IF_ERROR_RETURN
                    (soc_phyctrl_primary_get(unit, port, &primary));
                *value = (uint32) primary;
            break;

        case SOC_PHY_CONTROL_PORT_OFFSET:
                SOC_IF_ERROR_RETURN
                    (soc_phyctrl_offset_get(unit, port, &offset));
                *value = (uint32) offset;
            break;

        case SOC_PHY_CONTROL_CLOCK_ENABLE:
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_primary_get(unit, port, &primary));
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_offset_get(unit, port, &offset));
        if (phy_54640_toplvl_reg_read(unit, port, primary, 0x0, &data) == SOC_E_NONE) {
            if (( data & 0x8 ) || (( data & 0x7 ) != offset)) {
                *value = FALSE;
            } else {
                *value = TRUE;
            }
        }
        break;

        case SOC_PHY_CONTROL_CLOCK_SECONDARY_ENABLE:
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_primary_get(unit, port, &primary));
        SOC_IF_ERROR_RETURN
            (soc_phyctrl_offset_get(unit, port, &offset));
        if (phy_54640_toplvl_reg_read(unit, port, primary, 0x0, &data) == SOC_E_NONE) {
            if (( data & (0x8<<4) ) || (( data & (0x7<<4) ) != (offset<<4) )) {
                *value = FALSE;
            } else {
                *value = TRUE;
            }
        }
        break;

        case SOC_PHY_CONTROL_LINKDOWN_TRANSMIT:
            rv = phy_54640_control_linkdown_transmit_get(unit, port, value);
            break;

        case SOC_PHY_CONTROL_EEE:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_EEE_TEST_CTRL_Br(unit, pc, &data));

            if ((data & 0xc000) == 0xc000) {
                *value = 1;
            } else {
                *value = 0;
            }

            break;

        case SOC_PHY_CONTROL_EEE_AUTO:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }
            *value = (PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_ENABLED) &&
                 !PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_MODE)) ? 1 : 0;
            break;

        case SOC_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }
            *value = (PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_ENABLED) &&
                  PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_MODE)) ? 1 : 0;
            break;

        case SOC_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_EXP_MII_BUF_CNTL_STAT1r(unit, pc, &data));
            *value = ((data & 0x0700) > 8);
            break;

        case SOC_PHY_CONTROL_EEE_TRANSMIT_EVENTS:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }
            SOC_IF_ERROR_RETURN
                (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 0x0, 1U<<14)); /* exp af */
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_EXP_EEE_TX_EVENTSr(unit, pc, &data));
            temp = data;
            SOC_IF_ERROR_RETURN
                (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 1U<<14, 1U<<14)); /* exp af */
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_EXP_EEE_TX_EVENTSr(unit, pc, &data));
            temp |= (data << 16);
            *value = temp;
            break;

        case SOC_PHY_CONTROL_EEE_TRANSMIT_DURATION:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }
            SOC_IF_ERROR_RETURN
                (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 0x0, 1U<<14)); /* exp af */
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_EXP_EEE_TX_DURATIONr(unit, pc, &data));
            temp = data;
            SOC_IF_ERROR_RETURN
                (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 1U<<14, 1U<<14)); /* exp af */
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_EXP_EEE_TX_DURATIONr(unit, pc, &data));
            temp |= (data << 16);
            *value = temp;
            break;

        case SOC_PHY_CONTROL_EEE_RECEIVE_EVENTS:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }
            SOC_IF_ERROR_RETURN
                (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 0x0, 1U<<14)); /* exp af */
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_EXP_EEE_RX_EVENTSr(unit, pc, &data));
            temp = data;
            SOC_IF_ERROR_RETURN
                (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 1U<<14, 1U<<14)); /* exp af */
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_EXP_EEE_RX_EVENTSr(unit, pc, &data));
            temp |= (data << 16);
            *value = temp;
            break;

        case SOC_PHY_CONTROL_EEE_RECEIVE_DURATION:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
            }
            SOC_IF_ERROR_RETURN
                (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 0x0, 1U<<14)); /* exp af */
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_EXP_EEE_RX_DURATIONr(unit, pc, &data));
            temp = data;
            SOC_IF_ERROR_RETURN
                (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FAF,0x15, 1U<<14, 1U<<14)); /* exp af */
            SOC_IF_ERROR_RETURN
                (READ_PHY54640_EXP_EEE_RX_DURATIONr(unit, pc, &data));
            temp |= (data << 16);
            *value = temp;

            break;

    case SOC_PHY_CONTROL_LOOPBACK_EXTERNAL:
         SOC_IF_ERROR_RETURN
             (READ_PHY54640_MII_AUX_CTRLr(unit, pc, &data));
         *value = data & (1U<<15) ? 1 : 0;
         break;

    default:
            rv = SOC_E_UNAVAIL;
            break;
    }
    return rv;
}

/*
 * Function:
 *      phy_54640_cable_diag
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
phy_54640_cable_diag(int unit, soc_port_t port,
                    soc_port_cable_diag_t *status)
{
    int rv, rv2, i;
    phy_ctrl_t *pc;

    extern int phy_5464_cable_diag_sw(int, soc_port_t , 
                                      soc_port_cable_diag_t *);

    pc = EXT_PHY_SW_STATE(unit, port);

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
        rv2 = _phy_54640_reset_setup(unit, port);
        if (SOC_SUCCESS(rv2)) {
            rv2 = _phy_54640_medium_config_update(unit, port, &pc->copper);
        }
    }
    if (rv >= 0 && rv2 < 0) {
        return rv2;
    }
    return rv;
}

STATIC int
phy_54640_cable_diag_dispatch(int unit, soc_port_t port,
            soc_port_cable_diag_t *status)
{
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (ECD_ENABLE && ((PHY_IS_BCM54640E(pc)) && ((pc->phy_rev & 0x7) > 0x2)))  {
        SOC_IF_ERROR_RETURN(
            phy_ecd_cable_diag(unit, port, status));
        SOC_IF_ERROR_RETURN(
            _phy_54640_reset_setup(unit, port));
    } else {
        SOC_IF_ERROR_RETURN(
            phy_54640_cable_diag(unit, port, status));
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_54640_probe
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
phy_54640_probe(int unit, phy_ctrl_t *pc)
{
    uint16 id0, id1;

    if (READ_PHY54640_MII_PHY_ID0r(unit, pc, &id0) < 0) {
        return SOC_E_NOT_FOUND;
    }
    if (READ_PHY54640_MII_PHY_ID1r(unit, pc, &id1) < 0) {
        return SOC_E_NOT_FOUND;
    }

    switch (PHY_MODEL(id0, id1)) {

    case PHY_BCM54640_MODEL:
            return SOC_E_NONE;
    break;

    case PHY_BCM54680E_MODEL:
        if ((id1 & 0x8) == 0) {
            return SOC_E_NONE;
        }
    break;

    default:
    break;

    }
    return SOC_E_NOT_FOUND;
}

STATIC int
phy_54640_link_up(int unit, soc_port_t port)
{
    phy_ctrl_t *pc;
    int speed;


    pc = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (phy_54640_speed_get(unit, port, &speed));
    
    if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) { /*non EEE */
        /* do nothing */
    } else if ((pc->phy_rev & 0x6) == 0x0) {
    /* revs A0 and A1 of supported PHYs */
        uint32 eee_auto;

        SOC_IF_ERROR_RETURN
            (phy_54640_control_get(unit, port,
                     SOC_PHY_CONTROL_EEE_AUTO, &eee_auto));
        if ((speed == 100) && (eee_auto == 1)) {
            SOC_IF_ERROR_RETURN(
                PHY54640_REG_WRITE(unit, pc, 0, 0x0D10, 0x15, 0x00ff));

        } else {
            SOC_IF_ERROR_RETURN(
                PHY54640_REG_WRITE(unit, pc, 0, 0x0D10, 0x15, 0x0000));
        }
    } else if ((pc->phy_rev & 0x6) == 0x2) {
        /* revs B0 and B1 of supported PHYs */
        sal_usleep(10000);
        SOC_IF_ERROR_RETURN(
            MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, (1U << 11), (1U << 11)));
        SOC_IF_ERROR_RETURN(
            WRITE_PHY_REG(unit, pc, 0x17, 0x001a));
        SOC_IF_ERROR_RETURN(
            WRITE_PHY_REG(unit, pc, 0x15, 0x0003));
        SOC_IF_ERROR_RETURN(
            MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, 0, (1U << 11)));
    }

    /* Notify interface to internal PHY */
    SOC_IF_ERROR_RETURN
        (soc_phyctrl_notify(unit, port, phyEventInterface, SOC_PORT_IF_SGMII));

    /* Situation observed with Saber devices XGXS16g1l+BCM54640E
       Linkscan is not updating speed for internal phy xgxs16g1l. 
       It was observed that when speed is set to 100M the int phy 
       is still set to 1G and traffic does not flow through untill 
       int phy speed is also update to 100M. This applies to other 
       speeds as well. 
       Explicitly send speed notify to int phy from ext phy.
    */
    SOC_IF_ERROR_RETURN
        (soc_phyctrl_notify(unit, port, phyEventSpeed, speed));

    return SOC_E_NONE;
}

STATIC int
phy_54640_link_down(int unit, soc_port_t port)
{
    phy_ctrl_t *pc;

    if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) { /*non EEE */
        return SOC_E_NONE;
    }

    pc = EXT_PHY_SW_STATE(unit, port);

    if ((pc->phy_rev & 0x6) == 0x2) {
        SOC_IF_ERROR_RETURN(
            MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, (1U << 11), (1U << 11)));
        SOC_IF_ERROR_RETURN(
            WRITE_PHY_REG(unit, pc, 0x17, 0x001a));
        SOC_IF_ERROR_RETURN(
            WRITE_PHY_REG(unit, pc, 0x15, 0x0007));
        SOC_IF_ERROR_RETURN(
            MODIFY_PHY54640_MII_AUX_CTRLr(unit, pc, 0, (1U << 11)));
    }

    return (SOC_E_NONE);
}


void _phy_54640e_encode_egress_message_mode(soc_port_phy_timesync_event_message_egress_mode_t mode,
                                            int offset, uint16 *value)
{

    switch (mode) {
    case SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_NONE:
        *value |= (0x0 << offset);
        break;
    case SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_UPDATE_CORRECTIONFIELD:
        *value |= (0x1 << offset);
        break;
    case SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_REPLACE_CORRECTIONFIELD_ORIGIN:
        *value |= (0x2 << offset);
        break;
    case SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_CAPTURE_TIMESTAMP:
        *value |= (0x3 << offset);
        break;
    default:
        break;
    }

}

void _phy_54640e_encode_ingress_message_mode(soc_port_phy_timesync_event_message_ingress_mode_t mode,
                                            int offset, uint16 *value)
{

    switch (mode) {
    case SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_NONE:
        *value |= (0x0 << offset);
        break;
    case SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_UPDATE_CORRECTIONFIELD:
        *value |= (0x1 << offset);
        break;
    case SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_INSERT_TIMESTAMP:
        *value |= (0x2 << offset);
        break;
    case SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_INSERT_DELAYTIME:
        *value |= (0x3 << offset);
        break;
    default:
        break;
    }

}

void _phy_54640e_decode_egress_message_mode(uint16 value, int offset,
                                            soc_port_phy_timesync_event_message_egress_mode_t *mode)
{

    switch ((value >> offset) & 0x3) {
    case 0x0:
        *mode = SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_NONE;
        break;
    case 0x1:
        *mode = SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_UPDATE_CORRECTIONFIELD;
        break;
    case 0x2:
        *mode = SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_REPLACE_CORRECTIONFIELD_ORIGIN;
        break;
    case 0x3:
        *mode = SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_EGRESS_MODE_CAPTURE_TIMESTAMP;
        break;
    default:
        break;
    }

}

void _phy_54640e_decode_ingress_message_mode(uint16 value, int offset,
                                            soc_port_phy_timesync_event_message_ingress_mode_t *mode)
{

    switch ((value >> offset) & 0x3) {
    case 0x0:
        *mode = SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_NONE;
        break;
    case 0x1:
        *mode = SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_UPDATE_CORRECTIONFIELD;
        break;
    case 0x2:
        *mode = SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_INSERT_TIMESTAMP;
        break;
    case 0x3:
        *mode = SOC_PORT_PHY_TIMESYNC_EVENT_MESSAGE_INGRESS_MODE_INSERT_DELAYTIME;
        break;
    default:
        break;
    }

}

void _phy_54640e_encode_gmode(soc_port_phy_timesync_global_mode_t mode,
                                            uint16 *value)
{

    switch (mode) {
    case SOC_PORT_PHY_TIMESYNC_MODE_FREE:
        *value = 0x1;
        break;
    case SOC_PORT_PHY_TIMESYNC_MODE_SYNCIN:
        *value = 0x2;
        break;
    case SOC_PORT_PHY_TIMESYNC_MODE_CPU:
        *value = 0x3;
        break;
    default:
        break;
    }

}

void _phy_54640e_decode_gmode(uint16 value,
                                            soc_port_phy_timesync_global_mode_t *mode)
{

    switch (value & 0x3) {
    case 0x1:
        *mode = SOC_PORT_PHY_TIMESYNC_MODE_FREE;
        break;
    case 0x2:
        *mode = SOC_PORT_PHY_TIMESYNC_MODE_SYNCIN;
        break;
    case 0x3:
        *mode = SOC_PORT_PHY_TIMESYNC_MODE_CPU;
        break;
    default:
        break;
    }

}

void _phy_54640e_encode_framesync_mode(soc_port_phy_timesync_framesync_mode_t mode,
                                            uint16 *value)
{

    switch (mode) {
    case SOC_PORT_PHY_TIMESYNC_FRAMESYNC_SYNCIN0:
        *value = 1U << 0;
        break;
    case SOC_PORT_PHY_TIMESYNC_FRAMESYNC_SYNCIN1:
        *value = 1U << 1;
        break;
    case SOC_PORT_PHY_TIMESYNC_FRAMESYNC_SYNCOUT:
        *value = 1U << 2;
        break;
    case SOC_PORT_PHY_TIMESYNC_FRAMESYNC_CPU:
        *value = 1U << 3;
        break;
    default:
        break;
    }

}

void _phy_54640e_decode_framesync_mode(uint16 value,
                                            soc_port_phy_timesync_framesync_mode_t *mode)
{

    switch (value & 0xf) {
    case 1U << 0:
        *mode = SOC_PORT_PHY_TIMESYNC_FRAMESYNC_SYNCIN0;
        break;
    case 1U << 1:
        *mode = SOC_PORT_PHY_TIMESYNC_FRAMESYNC_SYNCIN1;
        break;
    case 1U << 2:
        *mode = SOC_PORT_PHY_TIMESYNC_FRAMESYNC_SYNCOUT;
        break;
    case 1U << 3:
        *mode = SOC_PORT_PHY_TIMESYNC_FRAMESYNC_CPU;
        break;
    default:
        break;
    }

}

void _phy_54640e_encode_syncout_mode(soc_port_phy_timesync_syncout_mode_t mode,
                                            uint16 *value)
{

    switch (mode) {
    case SOC_PORT_PHY_TIMESYNC_SYNCOUT_DISABLE:
        *value = 0x0;
        break;
    case SOC_PORT_PHY_TIMESYNC_SYNCOUT_ONE_TIME:
        *value = 0x1;
        break;
    case SOC_PORT_PHY_TIMESYNC_SYNCOUT_PULSE_TRAIN:
        *value = 0x2;
        break;
    case SOC_PORT_PHY_TIMESYNC_SYNCOUT_PULSE_TRAIN_WITH_SYNC:
        *value = 0x3;
        break;
    default:
        break;
    }

}

void _phy_54640e_decode_syncout_mode(uint16 value,
                                            soc_port_phy_timesync_syncout_mode_t *mode)
{

    switch (value & 0x3) {
    case 0x0:
        *mode = SOC_PORT_PHY_TIMESYNC_SYNCOUT_DISABLE;
        break;
    case 0x1:
        *mode = SOC_PORT_PHY_TIMESYNC_SYNCOUT_ONE_TIME;
        break;
    case 0x2:
        *mode = SOC_PORT_PHY_TIMESYNC_SYNCOUT_PULSE_TRAIN;
        break;
    case 0x3:
        *mode = SOC_PORT_PHY_TIMESYNC_SYNCOUT_PULSE_TRAIN_WITH_SYNC;
        break;
    default:
        break;
    }

}


STATIC int
phy_54640_timesync_config_set(int unit, soc_port_t port, soc_port_phy_timesync_config_t *conf)
{
    phy_ctrl_t *pc, *pc_port1;
    soc_phy_chip_info_t *chip_info;
    soc_port_t primary_port;
    int offset;
    uint16 rx_control_reg = 0, tx_control_reg = 0, rx_crc_control_reg = 0,
           en_control_reg = 0, tx_capture_en_reg = 0, rx_capture_en_reg = 0,
           nse_nco_6 = 0, nse_nco_2c = 0, value, mask,
           gmode = 0, framesync_mode = 0, syncout_mode = 0;

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_primary_get(unit, port, &primary_port));

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_offset_get(unit, port, &offset));

	    if ((primary_port == -1) || (SOC_PHY_INFO(unit, primary_port).chip_info == NULL)) {
		return SOC_E_FAIL;
	    }

    chip_info = SOC_PHY_INFO(unit, primary_port).chip_info;

    pc = EXT_PHY_SW_STATE(unit, port);
    pc_port1 = EXT_PHY_SW_STATE(unit, chip_info->offset_to_port[0]);

    if ((!PHY_IS_BCM54640E(pc)) || PHY_IS_BCM54640E_A0A1(pc)) {
        return SOC_E_UNAVAIL;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_FLAGS) {

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_ENABLE) {
            en_control_reg |= ((1U << (offset + 8)) | (1U << offset));
            nse_nco_6 |= (1U << 12); /* NSE init */

            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_SGMII_SLAVEr(unit, pc, 0x02, 0x02));

            SOC_IF_ERROR_RETURN
                (MODIFY_PHY54640_EXT_SERDES_CTRLr(unit, pc, 0x00, 0x01));

            SOC_IF_ERROR_RETURN
                (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FF5,0x15, 1U<<0, 1U<<0)); /* exp reg f5 */
        } else {

            SOC_IF_ERROR_RETURN
                (PHY54640_REG_MODIFY(unit, pc, 0x00, 0x0FF5,0x15, 0, 1U<<0)); /* exp reg f5 */

        }

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x10, en_control_reg, 
                                       ((1U << (offset + 8)) | (1U << offset))));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x10, &en_control_reg));

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE) {
            tx_capture_en_reg |= (1U << offset);
            rx_capture_en_reg |= (1U << (offset + 8));
        }

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x12, tx_capture_en_reg, 
                                       (1U << offset)));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x13, rx_capture_en_reg, 
                                       (1U << (offset + 8))));

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE) {
            nse_nco_6 |= (1U << 13);
        }

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x4f, nse_nco_6, 
                                       (1U << 13)));

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_RX_CRC_ENABLE) {
            rx_crc_control_reg |= (1U << 3);
        }

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x34, rx_crc_control_reg));

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_8021AS_ENABLE) {
            rx_control_reg |= (1U << 3);
            tx_control_reg |= (1U << 3);
        }

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_L2_ENABLE) {
            rx_control_reg |= (1U << 2);
            tx_control_reg |= (1U << 2);
        }

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_IP4_ENABLE) {
            rx_control_reg |= (1U << 1);
            tx_control_reg |= (1U << 1);
        }

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_IP6_ENABLE) {
            rx_control_reg |= (1U << 0);
            tx_control_reg |= (1U << 0);
        }

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x32, tx_control_reg));
                                          
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x33, rx_control_reg));
                                          
        if (conf->flags & SOC_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT) {
            nse_nco_2c &= (uint16) ~(1U << 14);
        } else {
            nse_nco_2c |= (1U << 14);
        }

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x47, nse_nco_2c, 
                                           (1U << 14)));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ITPID) {
        /* VLAN TAG register */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x35, conf->itpid));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID) {
        /* OUTER VLAN TAG register */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x36, conf->otpid));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID2) {
        /* INNER VLAN TAG register */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x52, conf->otpid2));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TS_DIVIDER) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x4b, conf->ts_divider & 0xfff));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_LINK_DELAY) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc, 0, 0x6f, conf->rx_link_delay & 0xffff));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc, 0, 0x70, (conf->rx_link_delay >> 16) & 0xffff));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x25, (uint16)(conf->original_timecode.nanoseconds & 0xffff)));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x26, (uint16)((conf->original_timecode.nanoseconds >> 16) & 0xffff)));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x27, (uint16)(COMPILER_64_LO(conf->original_timecode.seconds) & 0xffff)));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x28, (uint16)((COMPILER_64_LO(conf->original_timecode.seconds) >> 16) & 0xffff)));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x29, (uint16)(COMPILER_64_HI(conf->original_timecode.seconds) & 0xffff)));
    }
    mask = 0;

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_GMODE) {
        _phy_54640e_encode_gmode(conf->gmode,&gmode);
        mask |= (0x3 << 14);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE) {
        _phy_54640e_encode_framesync_mode(conf->framesync.mode, &framesync_mode);
        mask |= (0xf << 2);
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x51, conf->framesync.length_threshold));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x50, conf->framesync.event_offset));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE) {
        _phy_54640e_encode_syncout_mode(conf->syncout.mode, &syncout_mode);
        mask |= (0x3 << 0);
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x48, conf->syncout.interval & 0xffff));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x49, 
                ((conf->syncout.pulse_1_length & 0x3) << 14) | ((conf->syncout.interval >> 16) & 0x3fff)));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x4a, 
                ((conf->syncout.pulse_2_length & 0x1ff) << 7) | ((conf->syncout.pulse_1_length >> 2) & 0x7f)));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x4e, 
                (uint16)(COMPILER_64_LO(conf->syncout.syncout_ts) & 0xffff)));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x4d, 
                (uint16)((COMPILER_64_LO(conf->syncout.syncout_ts) >> 16) & 0xffff)));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x4c, 
                (uint16)(COMPILER_64_HI(conf->syncout.syncout_ts) & 0xffff)));
    }

    SOC_IF_ERROR_RETURN
        (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x4f, (gmode << 14) | (framesync_mode << 2) | (syncout_mode << 0),
                                       mask ));

    /* tx_timestamp_offset */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_TIMESTAMP_OFFSET) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x15 + offset, (uint16)(conf->tx_timestamp_offset & 0xffff)));

        if (offset < 4 ) {
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x63, 
                    ((conf->tx_timestamp_offset & 0xf0000) >> ((4 - offset) << 2)), 0xf << (offset << 2)));
        } else {
            uint16 offset1 = offset - 4;
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x64, 
                    ((conf->tx_timestamp_offset & 0xf0000) >> ((4 - offset1) << 2)), 0xf << (offset1 << 2)));
        }
    }

    /* rx_timestamp_offset */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_TIMESTAMP_OFFSET) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x1d + offset, (uint16)(conf->rx_timestamp_offset & 0xffff)));

        if (offset < 4 ) {
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x65, 
                    ((conf->rx_timestamp_offset & 0xf0000) >> ((4 - offset) << 2)), 0xf << (offset << 2)));
        } else {
            uint16 offset1 = offset - 4;
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x66, 
                    ((conf->rx_timestamp_offset & 0xf0000) >> ((4 - offset1) << 2)), 0xf << (offset1 << 2)));
        }
    }

    value = 0;
    mask = 0;

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE) {
        _phy_54640e_encode_egress_message_mode(conf->tx_sync_mode, 0, &value);
        mask |= 0x3 << 0;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE) {
        _phy_54640e_encode_egress_message_mode(conf->tx_delay_request_mode, 2, &value);
        mask |= 0x3 << 2;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE) {
        _phy_54640e_encode_egress_message_mode(conf->tx_pdelay_request_mode, 4, &value);
        mask |= 0x3 << 4;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE) {
        _phy_54640e_encode_egress_message_mode(conf->tx_pdelay_response_mode, 6, &value);
        mask |= 0x3 << 6;
    }
                                            
    SOC_IF_ERROR_RETURN
        (_phy_54640e_blk_top_lvl_reg_modify(unit, pc, 0, 0x11, value, mask)); 

    value = 0;
    mask = 0;

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE) {
        _phy_54640e_encode_ingress_message_mode(conf->rx_sync_mode, 0, &value);
        mask |= 0x3 << 0;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE) {
        _phy_54640e_encode_ingress_message_mode(conf->rx_delay_request_mode, 2, &value);
        mask |= 0x3 << 2;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE) {
        _phy_54640e_encode_ingress_message_mode(conf->rx_pdelay_request_mode, 4, &value);
        mask |= 0x3 << 4;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE) {
        _phy_54640e_encode_ingress_message_mode(conf->rx_pdelay_response_mode, 6, &value);
        mask |= 0x3 << 6;
    }
                                            
                                            
    SOC_IF_ERROR_RETURN
        (_phy_54640e_blk_top_lvl_reg_modify(unit, pc, 0, 0x12, value << 8, mask << 8)); 

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE) {
        /* Initial ref phase [15:0] */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x38, (uint16)(COMPILER_64_LO(conf->phy_1588_dpll_ref_phase) & 0xffff)));

        /* Initial ref phase [31:16]  */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x39, (uint16)((COMPILER_64_LO(conf->phy_1588_dpll_ref_phase) >> 16) & 0xffff)));

        /*  Initial ref phase [47:32] */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x3a, (uint16)(COMPILER_64_HI(conf->phy_1588_dpll_ref_phase) & 0xffff)));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE_DELTA) {
        /* Ref phase delta [15:0] */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x3b, (uint16)(conf->phy_1588_dpll_ref_phase_delta & 0xffff)));

        /* Ref phase delta [31:16]  */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x3c, (uint16)((conf->phy_1588_dpll_ref_phase_delta >> 16) & 0xffff)));
    }

    /* DPLL K1 & K2 */
    mask = 0;

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K1) {
        mask |= 0xff << 8;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K2) {
        mask |= 0xff;
    }
    SOC_IF_ERROR_RETURN
        (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x3d, (conf->phy_1588_dpll_k1 & 0xff)<<8 | (conf->phy_1588_dpll_k2 & 0xff), mask ));

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K3) {
        /* DPLL K3 */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x3e, conf->phy_1588_dpll_k3 & 0xff));
    }


    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_LOOP_FILTER) {
        /* Initial loop filter[15:0] */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x3f, (uint16)(COMPILER_64_LO(conf->phy_1588_dpll_loop_filter) & 0xffff)));

        /* Initial loop filter[31:16]  */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x40, (uint16)((COMPILER_64_LO(conf->phy_1588_dpll_loop_filter) >> 16) & 0xffff)));

        /*  Initial loop filter[47:32] */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x41, (uint16)(COMPILER_64_HI(conf->phy_1588_dpll_loop_filter) & 0xffff)));

        /* Initial loop filter[63:48]  */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x42, (uint16)((COMPILER_64_HI(conf->phy_1588_dpll_loop_filter) >> 16) & 0xffff)));
    }

    return SOC_E_NONE;
}

STATIC int
phy_54640_timesync_config_get(int unit, soc_port_t port, soc_port_phy_timesync_config_t *conf)
{
    phy_ctrl_t *pc, *pc_port1;
    soc_phy_chip_info_t *chip_info;
    soc_port_t primary_port;
    int offset;
    uint16 tx_control_reg = 0, rx_crc_control_reg = 0,
           en_control_reg = 0, tx_capture_en_reg = 0,
           nse_nco_6 = 0, nse_nco_2c = 0, temp1, temp2, temp3, temp4, value;
    soc_port_phy_timesync_global_mode_t gmode = SOC_PORT_PHY_TIMESYNC_MODE_FREE;
    soc_port_phy_timesync_framesync_mode_t framesync_mode = SOC_PORT_PHY_TIMESYNC_FRAMESYNC_NONE;
    soc_port_phy_timesync_syncout_mode_t syncout_mode = SOC_PORT_PHY_TIMESYNC_SYNCOUT_DISABLE;


    SOC_IF_ERROR_RETURN
        (soc_phyctrl_primary_get(unit, port, &primary_port));

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_offset_get(unit, port, &offset));

    if ((primary_port == -1) || (SOC_PHY_INFO(unit, primary_port).chip_info == NULL)) {
        return SOC_E_FAIL;
    }

    chip_info = SOC_PHY_INFO(unit, primary_port).chip_info;

    pc = EXT_PHY_SW_STATE(unit, port);
    pc_port1 = EXT_PHY_SW_STATE(unit, chip_info->offset_to_port[0]);

    if ((!PHY_IS_BCM54640E(pc)) || PHY_IS_BCM54640E_A0A1(pc)) {
        return SOC_E_UNAVAIL;
    }

    conf->flags = 0;

    SOC_IF_ERROR_RETURN
        (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x4f, &nse_nco_6));

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_FLAGS) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x10, &en_control_reg)); 

        if (en_control_reg & (1U << offset)) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_ENABLE;
        }

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x12, &tx_capture_en_reg)); 

        if (tx_capture_en_reg & (1U << offset)) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;
        }

        /*
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x4f, &nse_nco_6));
         */

        if (nse_nco_6 & (1U << 13)) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE;
        }

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x34, &rx_crc_control_reg));

        if (rx_crc_control_reg & (1U << 3)) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_RX_CRC_ENABLE;
        }

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x32, &tx_control_reg));
                                          
        if (tx_control_reg & (1U << 3)) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_8021AS_ENABLE;
        }

        if (tx_control_reg & (1U << 2)) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_L2_ENABLE;
        }

        if (tx_control_reg & (1U << 1)) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_IP4_ENABLE;
        }

        if (tx_control_reg & (1U << 0)) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_IP6_ENABLE;
        }

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x47, &nse_nco_2c));

        if (!(nse_nco_2c & (1U << 14))) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT;
        }
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_GMODE) {
        _phy_54640e_decode_gmode((nse_nco_6  >> 14),&gmode);
        conf->gmode = gmode;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE) {
        _phy_54640e_decode_framesync_mode((nse_nco_6  >> 2), &framesync_mode);
        conf->framesync.mode = framesync_mode;

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x51, &value));
        conf->framesync.length_threshold = value;
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x50, &value));
        conf->framesync.event_offset = value;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE) {
        _phy_54640e_decode_syncout_mode((nse_nco_6  >> 0), &syncout_mode);
        conf->syncout.mode = syncout_mode;

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x48, &temp1));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x49, &temp2));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x4a, &temp3));

        conf->syncout.pulse_1_length =  ((temp3 & 0x7f) << 2) | ((temp2 >> 14) & 0x3);
        conf->syncout.pulse_2_length = (temp3 >> 7) & 0x1ff;
        conf->syncout.interval =  ((temp2 & 0x3fff) << 16) | temp1;

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x4e, &temp1));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x4d, &temp2));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x4c, &temp3));

        COMPILER_64_SET(conf->syncout.syncout_ts, ((uint32)temp3),  (((uint32)temp2<<16)|((uint32)temp1)));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ITPID) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x35, &conf->itpid));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x36, &conf->otpid));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID2) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x52, &conf->otpid2));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TS_DIVIDER) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x4b, &conf->ts_divider));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_LINK_DELAY) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc, 0, 0x6f, &temp1));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc, 0, 0x70, &temp2));

        conf->rx_link_delay = ((uint32)temp2 << 16) | temp1; 
    }


    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x25, &temp1));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x26, &temp2));

        conf->original_timecode.nanoseconds = ((uint32)temp2 << 16) | temp1; 

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x27, &temp1));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x28, &temp2));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x29, &temp3));

        /* conf->original_timecode.seconds = ((uint64)temp3 << 32) | ((uint32)temp2 << 16) | temp1; */

        COMPILER_64_SET(conf->original_timecode.seconds, ((uint32)temp3),  (((uint32)temp2<<16)|((uint32)temp1)));
    }

    /* tx_timestamp_offset */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_TIMESTAMP_OFFSET) {
        uint32 temp32;

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x15 + offset, &temp1));
        if (offset < 4 ) {
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x63, &temp2));
            temp32 = (temp2 & (0xf << (offset << 2))) << ((4 - offset) << 2);
        } else {
            uint16 offset1 = offset - 4;
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x64, &temp2));
            temp32 = (temp2 & (0xf << (offset1 << 2))) << ((4 - offset1) << 2);
        }
        conf->tx_timestamp_offset = temp32 | temp1;
    }

    /* rx_timestamp_offset */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_TIMESTAMP_OFFSET) {
        uint32 temp32;

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x1d + offset, &temp1));
        if (offset < 4 ) {
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x65, &temp2));
            temp32 = (temp2 & (0xf << (offset << 2))) << ((4 - offset) << 2);
        } else {
            uint16 offset1 = offset - 4;
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x66, &temp2));
            temp32 = (temp2 & (0xf << (offset1 << 2))) << ((4 - offset1) << 2);
        }
        conf->rx_timestamp_offset = temp32 | temp1;
    }

    SOC_IF_ERROR_RETURN
        (_phy_54640e_blk_top_lvl_reg_read(unit, pc, 0, 0x11, &value)); 

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE) {
        _phy_54640e_decode_egress_message_mode(value, 0, &conf->tx_sync_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE) {
        _phy_54640e_decode_egress_message_mode(value, 2, &conf->tx_delay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE) {
        _phy_54640e_decode_egress_message_mode(value, 4, &conf->tx_pdelay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE) {
        _phy_54640e_decode_egress_message_mode(value, 6, &conf->tx_pdelay_response_mode);
    }

    SOC_IF_ERROR_RETURN
        (_phy_54640e_blk_top_lvl_reg_read(unit, pc, 0, 0x12, &value)); 

    value >>= 8;

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE) {
        _phy_54640e_decode_ingress_message_mode(value, 0, &conf->rx_sync_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE) {
        _phy_54640e_decode_ingress_message_mode(value, 2, &conf->rx_delay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE) {
        _phy_54640e_decode_ingress_message_mode(value, 4, &conf->rx_pdelay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE) {
        _phy_54640e_decode_ingress_message_mode(value, 6, &conf->rx_pdelay_response_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE) {
        /* Initial ref phase [15:0] */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x38, &temp1));

        /* Initial ref phase [31:16]  */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x39, &temp2));

        /*  Initial ref phase [47:32] */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x3a, &temp3));

        COMPILER_64_SET(conf->phy_1588_dpll_ref_phase, ((uint32)temp3), (((uint32)temp2<<16)|((uint32)temp1)));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE_DELTA) {
        /* Ref phase delta [15:0] */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x3b, &temp1));

        /* Ref phase delta [31:16]  */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x3c, &temp2));

        conf->phy_1588_dpll_ref_phase_delta = (temp2 << 16) | temp1;
    }

    /* DPLL K1 & K2 */
    SOC_IF_ERROR_RETURN
        (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x3d, &temp1 ));

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K1) {
        conf->phy_1588_dpll_k1 = (temp1 >> 8 ) & 0xff;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K2) {
        conf->phy_1588_dpll_k2 = temp1 & 0xff;
    }

    /* DPLL K3 */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K3) {
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x3e, &temp1));
        conf->phy_1588_dpll_k3 = temp1 & 0xff;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_LOOP_FILTER) {
        /* Initial loop filter[15:0] */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x3f, &temp1));

        /* Initial loop filter[31:16]  */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x40, &temp2));

        /*  Initial loop filter[47:32] */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x41, &temp3));

        /* Initial loop filter[63:48]  */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x42, &temp4));

        COMPILER_64_SET(conf->phy_1588_dpll_loop_filter, (((uint32)temp4<<16)|((uint32)temp3)), (((uint32)temp2<<16)|((uint32)temp1)));
    }
                                            
    return SOC_E_NONE;
}

STATIC int
phy_54640_timesync_control_set(int unit, soc_port_t port, soc_port_control_phy_timesync_t type, uint64 value)
{
    phy_ctrl_t *pc, *pc_port1;
    soc_phy_chip_info_t *chip_info;
    soc_port_t primary_port;
    uint16 temp1, temp2;
    uint32 value0;
    int offset;

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_primary_get(unit, port, &primary_port));

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_offset_get(unit, port, &offset));

    if ((primary_port == -1) || (SOC_PHY_INFO(unit, primary_port).chip_info == NULL)) {
        return SOC_E_FAIL;
    }

    chip_info = SOC_PHY_INFO(unit, primary_port).chip_info;

    pc = EXT_PHY_SW_STATE(unit, port);
    pc_port1 = EXT_PHY_SW_STATE(unit, chip_info->offset_to_port[0]);

    if ((!PHY_IS_BCM54640E(pc)) || PHY_IS_BCM54640E_A0A1(pc)) {
        return SOC_E_UNAVAIL;
    }

    switch (type) {
    case SOC_PORT_CONTROL_PHY_TIMESYNC_CAPTURE_TIMESTAMP:
    case SOC_PORT_CONTROL_PHY_TIMESYNC_HEARTBEAT_TIMESTAMP:
        return SOC_E_FAIL;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_NCOADDEND:
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x44, (uint16)COMPILER_64_LO(value)));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_FRAMESYNC:
       SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x4f, &temp1));
        temp2 = ((temp1 | (0x3 << 14) | (0x1 << 12)) & ~(0xf << 2)) | (0x1 << 5);
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x4f, temp2));
        sal_udelay(1);
        temp2 &= ~((0x1 << 5) | (0x1 << 12));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x4f, temp2));
        sal_udelay(1);
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x4f, temp1));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_LOCAL_TIME:
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x45, (uint16)(COMPILER_64_LO(value) & 0xffff)));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x46, (uint16)((COMPILER_64_LO(value) >> 16) & 0xffff)));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x47, (uint16)(COMPILER_64_HI(value) & 0xfff), 0xfff));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_LOAD_CONTROL:
        temp1 = 0;
        temp2 = 0;
        value0 = COMPILER_64_LO(value);
        if (value0 &  SOC_PORT_PHY_TIMESYNC_TN_LOAD) {
            temp1 |= 1U << 11;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_TN_ALWAYS_LOAD) {
            temp2 |= 1U << 11;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_TIMECODE_LOAD) {
            temp1 |= 1U << 10;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_TIMECODE_ALWAYS_LOAD) {
            temp2 |= 1U << 10;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_SYNCOUT_LOAD) {
            temp1 |= 1U << 9;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_SYNCOUT_ALWAYS_LOAD) {
            temp2 |= 1U << 9;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_NCO_DIVIDER_LOAD) {
            temp1 |= 1U << 8;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_NCO_DIVIDER_ALWAYS_LOAD) {
            temp2 |= 1U << 8;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_LOCAL_TIME_LOAD) {
            temp1 |= 1U << 7;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_LOCAL_TIME_ALWAYS_LOAD) {
            temp2 |= 1U << 7;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_NCO_ADDEND_LOAD) {
            temp1 |= 1U << 6;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_NCO_ADDEND_ALWAYS_LOAD) {
            temp2 |= 1U << 6;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_LOAD) {
            temp1 |= 1U << 5;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_ALWAYS_LOAD) {
            temp2 |= 1U << 5;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_LOAD) {
            temp1 |= 1U << 4;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_ALWAYS_LOAD) {
            temp2 |= 1U << 4;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_LOAD) {
            temp1 |= 1U << 3;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_ALWAYS_LOAD) {
            temp2 |= 1U << 3;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_K3_LOAD) {
            temp1 |= 1U << 2;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_K3_ALWAYS_LOAD) {
            temp2 |= 1U << 2;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_K2_LOAD) {
            temp1 |= 1U << 1;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_K2_ALWAYS_LOAD) {
            temp2 |= 1U << 1;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_K1_LOAD) {
            temp1 |= 1U << 0;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_DPLL_K1_ALWAYS_LOAD) {
            temp2 |= 1U << 0;
        }
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x2e, temp1));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x2f, temp2));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT_MASK:
        temp1 = 0;

        value0 = COMPILER_64_LO(value);

        if (value0 &  SOC_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_MASK) {
            temp1 |= 1U << 1;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_FRAMESYNC_INTERRUPT_MASK) {
            temp1 |= 1U << 0;
        }
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x30, temp1));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_TX_TIMESTAMP_OFFSET:
        value0 = COMPILER_64_LO(value);

        /* tx_timestamp_offset */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x15 + offset, (uint16)(value0 & 0xffff)));

        if (offset < 4 ) {
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x63, 
                    ((value0 & 0xf0000) >> ((4 - offset) << 2)), 0xf << (offset << 2)));
        } else {
            uint16 offset1 = offset - 4;
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x64, 
                    ((value0 & 0xf0000) >> ((4 - offset1) << 2)), 0xf << (offset1 << 2)));
        }
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_RX_TIMESTAMP_OFFSET:
        value0 = COMPILER_64_LO(value);

        /* rx_timestamp_offset */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_write(unit, pc_port1, 0, 0x1d + offset, (uint16)(value0 & 0xffff)));

        if (offset < 4 ) {
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x65, 
                    ((value0 & 0xf0000) >> ((4 - offset) << 2)), 0xf << (offset << 2)));
        } else {
            uint16 offset1 = offset - 4;
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_modify(unit, pc_port1, 0, 0x66, 
                    ((value0 & 0xf0000) >> ((4 - offset1) << 2)), 0xf << (offset1 << 2)));
        }
        break;


    default:
        return SOC_E_FAIL;
    }

    return SOC_E_NONE;
}

STATIC int
phy_54640_timesync_control_get(int unit, soc_port_t port, soc_port_control_phy_timesync_t type, uint64 *value)
{
    phy_ctrl_t *pc, *pc_port1;
    soc_phy_chip_info_t *chip_info;
    soc_port_t primary_port;
    uint16 value0;
    uint16 value1;
    uint16 value2;
    uint32 temp32;
    int offset;

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_primary_get(unit, port, &primary_port));

    SOC_IF_ERROR_RETURN
        (soc_phyctrl_offset_get(unit, port, &offset));

    if ((primary_port == -1) || (SOC_PHY_INFO(unit, primary_port).chip_info == NULL)) {
        return SOC_E_FAIL;
    }

    chip_info = SOC_PHY_INFO(unit, primary_port).chip_info;

    pc = EXT_PHY_SW_STATE(unit, port);
    pc_port1 = EXT_PHY_SW_STATE(unit, chip_info->offset_to_port[0]);

    if ((!PHY_IS_BCM54640E(pc)) || PHY_IS_BCM54640E_A0A1(pc)) {
        return SOC_E_UNAVAIL;
    }

    switch (type) {
    case SOC_PORT_CONTROL_PHY_TIMESYNC_HEARTBEAT_TIMESTAMP:
        SOC_IF_ERROR_RETURN
            (WRITE_PHY54640_EXP_READ_START_END_CTRLr(unit, pc_port1, 0x1));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x56, &value0));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x57, &value1));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x58, &value2));

        SOC_IF_ERROR_RETURN
            (WRITE_PHY54640_EXP_READ_START_END_CTRLr(unit, pc_port1, 0x2));
        SOC_IF_ERROR_RETURN
            (WRITE_PHY54640_EXP_READ_START_END_CTRLr(unit, pc_port1, 0x0));

        COMPILER_64_SET((*value), ((uint32)value2),  (((uint32)value1<<16)|((uint32)value0)));
    /*    *value =  (((uint64)value2) << 32) | (((uint64)value1) << 16) | ((uint64)value0); */
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_CAPTURE_TIMESTAMP:
        SOC_IF_ERROR_RETURN
            (WRITE_PHY54640_EXP_READ_START_END_CTRLr(unit, pc_port1, 0x4));

        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x53, &value0));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x54, &value1));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x55, &value2));

        SOC_IF_ERROR_RETURN
            (WRITE_PHY54640_EXP_READ_START_END_CTRLr(unit, pc_port1, 0x8));
        SOC_IF_ERROR_RETURN
            (WRITE_PHY54640_EXP_READ_START_END_CTRLr(unit, pc_port1, 0x0));

        COMPILER_64_SET((*value), ((uint32)value2),  (((uint32)value1<<16)|((uint32)value0)));
    /*   *value = (((uint64)value2) << 32) | (((uint64)value1) << 16) | ((uint64)value0); */
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_NCOADDEND:
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x44, &value0));
        /* *value = value0; */
        COMPILER_64_SET((*value), 0,  (uint32)value0);
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_LOCAL_TIME:
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x45, &value0));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x46, &value1));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x47, &value2));

        COMPILER_64_SET((*value), (value2 & 0x0fff), ((uint32)value1 << 16) | value0 );
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_LOAD_CONTROL:
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x2e, &value1));
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x2f, &value2));
        value0 = 0;
        if (value1 & (1U << 11)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_TN_LOAD;
        }
        if (value2 & (1U << 11)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_TN_ALWAYS_LOAD;
        }
        if (value1 & (1U << 10)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_TIMECODE_LOAD;
        }
        if (value2 & (1U << 10)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_TIMECODE_ALWAYS_LOAD;
        }
        if (value1 & (1U << 9)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_SYNCOUT_LOAD;
        }
        if (value2 & (1U << 9)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_SYNCOUT_ALWAYS_LOAD;
        }
        if (value1 & (1U << 8)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_NCO_DIVIDER_LOAD;
        }
        if (value1 & (1U << 8)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_NCO_DIVIDER_ALWAYS_LOAD;
        }
        if (value1 & (1U << 7)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_LOCAL_TIME_LOAD;
        }
        if (value1 & (1U << 7)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_LOCAL_TIME_ALWAYS_LOAD;
        }
        if (value1 & (1U << 6)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_NCO_ADDEND_LOAD;
        }
        if (value2 & (1U << 6)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_NCO_ADDEND_ALWAYS_LOAD;
        }
        if (value1 & (1U << 5)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_LOAD;
        }
        if (value2 & (1U << 5)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_ALWAYS_LOAD;
        }
        if (value1 & (1U << 4)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_LOAD;
        }
        if (value2 & (1U << 4)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_ALWAYS_LOAD;
        }
        if (value1 & (1U << 3)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_LOAD;
        }
        if (value2 & (1U << 3)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_ALWAYS_LOAD;
        }
        if (value1 & (1U << 2)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_K3_LOAD;
        }
        if (value2 & (1U << 2)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_K3_ALWAYS_LOAD;
        }
        if (value1 & (1U << 1)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_K2_LOAD;
        }
        if (value2 & (1U << 1)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_K2_ALWAYS_LOAD;
        }
        if (value1 & (1U << 0)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_K1_LOAD;
        }
        if (value2 & (1U << 0)) {
            value0 |= (uint16) SOC_PORT_PHY_TIMESYNC_DPLL_K1_ALWAYS_LOAD;
        }
        COMPILER_64_SET((*value), 0, (uint32)value0);
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT:
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x31, &value1));
        value0 = 0;
        if (value1 & (1U << 1)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT;
        }
        if (value1 & (1U << 0)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_FRAMESYNC_INTERRUPT;
        }
        COMPILER_64_SET((*value), 0, (uint32)value0);
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT_MASK:
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x30, &value1));
        value0 = 0;
        if (value1 & (1U << 1)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_MASK;
        }
        if (value1 & (1U << 0)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_FRAMESYNC_INTERRUPT_MASK;
        }
        COMPILER_64_SET((*value), 0, (uint32)value0);
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_TX_TIMESTAMP_OFFSET:
    /* tx_timestamp_offset */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x15 + offset, &value1));
        if (offset < 4 ) {
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x63, &value2));
            temp32 = (value2 & (0xf << (offset << 2))) << ((4 - offset) << 2);
        } else {
            uint16 offset1 = offset - 4;
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x64, &value2));
            temp32 = (value2 & (0xf << (offset1 << 2))) << ((4 - offset1) << 2);
        }
        /* *value = temp32 | value1; */
        COMPILER_64_SET((*value), 0, (uint32)(temp32 | value1));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_RX_TIMESTAMP_OFFSET:
    /* rx_timestamp_offset */
        SOC_IF_ERROR_RETURN
            (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x1d + offset, &value1));
        if (offset < 4 ) {
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x65, &value2));
            temp32 = (value2 & (0xf << (offset << 2))) << ((4 - offset) << 2);
        } else {
            uint16 offset1 = offset - 4;
            SOC_IF_ERROR_RETURN
                (_phy_54640e_blk_top_lvl_reg_read(unit, pc_port1, 0, 0x66, &value2));
            temp32 = (value2 & (0xf << (offset1 << 2))) << ((4 - offset1) << 2);
        }
        /* *value = temp32 | value1; */
        COMPILER_64_SET((*value), 0, (uint32)(temp32 | value1));
        break;

    default:
        return SOC_E_FAIL;
    }

    return SOC_E_NONE;
}

#ifdef BROADCOM_DEBUG

/*
 * Function:
 *      phy_54640_shadow_dump
 * Purpose:
 *      Debug routine to dump all shadow registers.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 */

void
phy_54640_shadow_dump(int unit, soc_port_t port)
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
 * Variable:    phy_54640drv_ge
 * Purpose:     PHY driver for 54640
 */

phy_driver_t phy_54640drv_ge = {
    "54640 Gigabit PHY Driver",
    phy_54640_init,
    phy_fe_ge_reset,
    phy_54640_link_get,
    phy_54640_enable_set,
    phy_54640_enable_get,
    phy_54640_duplex_set,
    phy_54640_duplex_get,
    phy_54640_speed_set,
    phy_54640_speed_get,
    phy_54640_master_set,
    phy_54640_master_get,
    phy_54640_autoneg_set,
    phy_54640_autoneg_get,
    NULL,
    NULL,
    NULL,
    phy_54640_lb_set,
    phy_54640_lb_get,
    phy_54640_interface_set,
    phy_54640_interface_get,
    NULL,
    phy_54640_link_up,           /* Phy link up event */
    phy_54640_link_down,         /* Phy link down event */
    phy_54640_mdix_set,
    phy_54640_mdix_get,
    phy_54640_mdix_status_get,
    phy_54640_medium_config_set,
    phy_54640_medium_config_get,
    phy_54640_medium_status,
    phy_54640_cable_diag_dispatch,
    NULL,                       /* phy_link_change */
    phy_54640_control_set,      /* phy_control_set */ 
    phy_54640_control_get,      /* phy_control_get */
    phy_ge_reg_read,
    phy_ge_reg_write,
    phy_ge_reg_modify,
    NULL,                        /* Phy event notify */ 
    phy_54640_probe,             /* pd_probe */
    phy_54640_ability_advert_set,
    phy_54640_ability_advert_get,
    phy_54640_ability_remote_get,
    phy_54640_ability_local_get,
    NULL,                        /* f/w set */
    phy_54640_timesync_config_set,
    phy_54640_timesync_config_get,
    phy_54640_timesync_control_set,
    phy_54640_timesync_control_get
};
#else /* INCLUDE_PHY_54640*/
int _soc_phy_54640_not_empty;
#endif /* INCLUDE_PHY_54640*/

