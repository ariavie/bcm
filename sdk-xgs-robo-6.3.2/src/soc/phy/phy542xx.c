/*
 * $Id: phy542xx.c 1.58 Broadcom SDK $ 
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
 */

/*
 * File:        phy542xx.c
 * Purpose:     PHY driver for BCM542xx (Non-MacSec) 
 *
 * Supported BCM542X Family of PHY devices:
 *
 *      Device  Ports   Media                           MAC Interface
 *      54280   8       Copper(no Fiber)                SGMII
 *      54282   8       Copper(no Fiber)                2 QSGMII
 *      54285   8       Copper & Fiber                  2 QSGMII
 *      54240   4       Copper & Fiber                  SGMII

 * Workarounds:
 *
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



#if defined(INCLUDE_PHY_542XX)
#include "phyconfig.h"    /* Must be the first phy include after phydefs.h */
#include "phyident.h"
#include "phyreg.h"
#include "phyfege.h"
#include "phy542xx_int.h"
#include "phy542xx_regdefs.h"

#include <soc/oam.h>

#define DISABLE_CLK125          0       /* Disable if unused to reduce */
                                        /*  EMI emissions */


#define LC_CM_DISABLE_WORKAROUND
#define AUTO_MDIX_WHEN_AN_DIS 0

/*Copper*/
#define ADVERT_ALL_COPPER_FLOW_CTRL   (_SHR_PA_PAUSE_TX | _SHR_PA_PAUSE_RX)
#define ADVERT_ALL_COPPER_HD_SPEED    (_SHR_PA_SPEED_10MB |  _SHR_PA_SPEED_100MB | _SHR_PA_SPEED_1000MB) 
#define ADVERT_ALL_COPPER_FD_SPEED    (_SHR_PA_SPEED_10MB |  _SHR_PA_SPEED_100MB | _SHR_PA_SPEED_1000MB)

/*Fiber*/
#define ADVERT_ALL_FIBER_FLOW_CTRL    (_SHR_PA_PAUSE_TX | _SHR_PA_PAUSE_RX)
#define ADVERT_ALL_FIBER_FD_SPEED     (_SHR_PA_SPEED_100MB | _SHR_PA_SPEED_1000MB)


#define SET_FLOW_CTRL_ABILITY(ability, p) ((ability)->pause |= p)
#define SET_HD_SPEED_ABILITY(ability, s)  ((ability)->speed_half_duplex |= s)
#define SET_FD_SPEED_ABILITY(ability, s)  ((ability)->speed_full_duplex |= s)

#define ADVERT_ALL_COPPER(ability) {\
        SET_FLOW_CTRL_ABILITY(ability, ADVERT_ALL_COPPER_FLOW_CTRL);\
        SET_HD_SPEED_ABILITY(ability, ADVERT_ALL_COPPER_HD_SPEED);\
        SET_FD_SPEED_ABILITY(ability, ADVERT_ALL_COPPER_FD_SPEED);\
        }

#define ADVERT_ALL_FIBER(ability) {\
        SET_FLOW_CTRL_ABILITY(ability, ADVERT_ALL_FIBER_FLOW_CTRL);\
        SET_FD_SPEED_ABILITY(ability, ADVERT_ALL_FIBER_FD_SPEED);\
        }

#define PHY_IS_BCM54280(_pc) (PHY_MODEL_CHECK((_pc), \
                                  PHY_BCM54280_OUI, \
                                  PHY_BCM54280_MODEL)) 

#define PHY_IS_BCM54282(_pc) (PHY_MODEL_CHECK((_pc), \
                                  PHY_BCM54282_OUI, \
                                  PHY_BCM54282_MODEL) \
                                  && ((_pc)->phy_rev & 0x8))

#define PHY_IS_BCM54240(_pc) (PHY_MODEL_CHECK((_pc), \
                                  PHY_BCM54240_OUI, \
                                  PHY_BCM54240_MODEL))

#define PHY_IS_BCM54285(_pc) (PHY_MODEL_CHECK((_pc), \
                                  PHY_BCM54285_OUI, \
                                  PHY_BCM54285_MODEL) \
                                  && (!((_pc)->phy_rev & 0x8)))

#define PHY_IS_BCM5428x(pc) (PHY_IS_BCM54280(pc) \
                            || PHY_IS_BCM54282(pc) \
                            || PHY_IS_BCM54240(pc) \
                            || PHY_IS_BCM54285(pc))

#if 0
#define PHY_BCM54282_QSGMII_DEV_ADDR(phy_base_addr) (phy_base_addr + 8)
#define PHY_BCM54282_QSGMII_DEV_ADDR_SHARED(phy_base_addr) (phy_base_addr + 3)
#endif

#define PHY_BCM54282_QSGMII_DEV_ADDR(_pc) \
            (PHY_BCM542XX_FLAGS((_pc)) & PHY_BCM542XX_PHYA_REV) ? (PHY_BCM542XX_DEV_PHY_ID_BASE(_pc) + 1) : \
            (PHY_BCM542XX_DEV_PHY_ID_BASE(_pc) + 8)

#define PHY_BCM54282_QSGMII_DEV_ADDR_SHARED(_pc) PHY_BCM542XX_SLICE_ADDR((_pc), 3)

#define PHY_BCM542XX_REV_BO(model, rev)  ( \
                                          ((rev & 0x7) == 0x1) && \
                                           ((model == PHY_BCM54280_MODEL) || \
                                            (model == PHY_BCM54282_MODEL) || \
                                            (model == PHY_BCM54240_MODEL) || \
                                            (model == PHY_BCM54285_MODEL)))
#define PHY_BCM542XX_REV_C1(model, rev)  ( \
                                          ((rev & 0x7) == 0x3) && \
                                           ((model == PHY_BCM54280_MODEL) || \
                                            (model == PHY_BCM54282_MODEL) || \
                                            (model == PHY_BCM54240_MODEL) || \
                                            (model == PHY_BCM54285_MODEL)))

#define PHY_BCM542XX_REV_C0_OR_OLDER(model, rev)  ( \
                                          ((rev & 0x7) < 0x3) && \
                                           ((model == PHY_BCM54280_MODEL) || \
                                            (model == PHY_BCM54282_MODEL) || \
                                            (model == PHY_BCM54240_MODEL) || \
                                            (model == PHY_BCM54285_MODEL)))

#define PHY_IS_BCM54290(_pc) (PHY_MODEL_CHECK((_pc), \
                                  PHY_BCM54290_OUI, \
                                  PHY_BCM54290_MODEL))

#define PHY_IS_BCM54292(_pc) (PHY_MODEL_CHECK((_pc), \
                                  PHY_BCM54295_OUI, \
                                  PHY_BCM54295_MODEL))
#define PHY_IS_BCM54294(_pc) (PHY_MODEL_CHECK((_pc), \
                                  PHY_BCM54294_OUI, \
                                  PHY_BCM54294_MODEL)\
                                  && ((_pc)->phy_rev & 0x8))

#define PHY_IS_BCM54295(_pc) (PHY_MODEL_CHECK((_pc), \
                                  PHY_BCM54295_OUI, \
                                  PHY_BCM54295_MODEL) \
                                  && (((_pc)->phy_rev & 0x8)))
#define PHY_IS_BCM5429X(pc) (PHY_IS_BCM54290(pc) \
                           || PHY_IS_BCM54294(pc) \
                           || PHY_IS_BCM54292(pc) \
                           || PHY_IS_BCM54295(pc))   

STATIC int _phy_bcm542xx_medium_change(int unit, soc_port_t port, 
                          int force_update, soc_port_medium_t force_medium);
STATIC int phy_bcm542xx_duplex_set(int unit, soc_port_t port, int duplex);
STATIC int phy_bcm542xx_speed_set(int unit, soc_port_t port, int speed);
STATIC int phy_bcm542xx_master_set(int unit, soc_port_t port, int master);
STATIC int phy_bcm542xx_autoneg_get(int unit, soc_port_t port,
                     int *autoneg, int *autoneg_done);
STATIC int phy_bcm542xx_autoneg_set(int unit, soc_port_t port, int autoneg);
STATIC int phy_bcm542xx_mdix_set(int unit, soc_port_t port, soc_port_mdix_t mode);
STATIC int phy_bcm542xx_speed_get(int unit, soc_port_t port, int *speed);
STATIC int phy_bcm542xx_ability_advert_set(int unit, soc_port_t port, soc_port_ability_t *ability);
STATIC int  phy_bcm542xx_init_setup(int unit, soc_port_t port, int reset, int automedium, 
                                             int fiber_preferred, int fiber_detect, 
                                             int fiber_enable, int copper_enable);
STATIC int
phy_bcm542xx_control_set(int unit, soc_port_t port, soc_phy_control_t type, uint32 value);
extern int
phy_ecd_cable_diag_init_40nm(int unit, soc_port_t port);
extern int
phy_ecd_cable_diag_40nm(int unit, soc_port_t port,
            soc_port_cable_diag_t *status, uint16 ecd_ctrl);

/***********************************************************************
 *
 * HELPER FUNCTIONS
 *
 ***********************************************************************/

int
phy_bcm542xx_reg_read(int unit, phy_ctrl_t *pc, uint32 flags, uint16 reg_bank,
                   uint8 reg_addr, uint16 *data)
{
    int rv;
    uint16 val;

    rv = SOC_E_NONE;
    if (flags & PHY_BCM542XX_REG_QSGMII) {
        /*
         * Do not apply shadow register logic for QSGMII register access
         */
        rv = PHY_BCM542XX_READ_PHY_REG(unit, pc, reg_addr, data);
    } else if (flags & PHY_BCM542XX_REG_1000X) {
        if (reg_addr <= 0x000f) {
            uint16 blk_sel;

            /* Map 1000X page */
            val = 0x7c00;
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x1c, val));
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_READ_PHY_REG(unit, pc, 0x1c, &blk_sel));
            val = blk_sel | 0x8001;
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x1c, val));

            /* Read 1000X IEEE register */
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_READ_PHY_REG(unit, pc, reg_addr, data));

           /* Restore IEEE mapping */
            val = (blk_sel & 0xfffe) | 0x8000;
            SOC_IF_ERROR_RETURN
                
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x1c, val));
        } else {
            rv = SOC_E_PARAM;
        }
    } else {
        switch(reg_addr) {
        /* Map shadow registers */
        case 0x15:
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x17, reg_bank));
            break;
        case 0x18:
            if (reg_bank <= 0x0007) {
                val = (reg_bank << 12) | 0x7;
                SOC_IF_ERROR_RETURN
                    (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, reg_addr, val));
            } else {
                rv = SOC_E_PARAM;
            }
            break;
        case 0x1C:
            if (reg_bank <= 0x001F) {
                val = (reg_bank << 10);
                SOC_IF_ERROR_RETURN
                    (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, reg_addr, val));
            } else {
                rv = SOC_E_PARAM;
            }
            break;
        case 0x1D:
            if (reg_bank <= 0x0001) {
                val = reg_bank << 15; 
                SOC_IF_ERROR_RETURN
                    (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, reg_addr, val));
            } else {
                rv = SOC_E_PARAM;
            }
            break;
        }
        if (SOC_SUCCESS(rv)) {
            rv = PHY_BCM542XX_READ_PHY_REG(unit, pc, reg_addr, data);
        }
    } 
    if (SOC_FAILURE(rv)) {
        SOC_DEBUG_PRINT((DK_PHY,"phy_bcm542xx_reg_read: failed:"
                        "phy_id=0x%2x reg_bank=0x%04x reg_addr=0x%02x "
                        "rv=%d\n", pc->phy_id, reg_bank, reg_addr, rv)); 
    }

    return rv;
}


int
phy_bcm542xx_reg_write(int unit, phy_ctrl_t *pc, uint32 flags, uint16 reg_bank,
                           uint8 reg_addr, uint16 data)
{
    int     rv;
    uint16 val;

    rv       = SOC_E_NONE;
    if (flags & PHY_BCM542XX_REG_QSGMII) {
        /*
         * Do not apply shadow register logic for QSGMII register access
         */
        SOC_IF_ERROR_RETURN
            (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, reg_addr, data));
    } else if (flags & PHY_BCM542XX_REG_1000X) {
        if (reg_addr <= 0x000f) {
            uint16 blk_sel;

            /* Map 1000X page */
            val = 0x7c00;
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x1c, val));
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_READ_PHY_REG(unit, pc, 0x1c, &blk_sel));
            val = blk_sel | 0x8001;
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x1c, val));

            /* Write 1000X IEEE register */
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, reg_addr, data));

           /* Restore IEEE mapping */
            val = (blk_sel & 0xfffe) | 0x8000;
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x1c, val));
        } else {
            rv = SOC_E_PARAM;
        }
    } else {
        switch(reg_addr) {
        /* Map shadow registers */
        case 0x15:
            SOC_IF_ERROR_RETURN
                    (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x17, reg_bank));
            break;
        case 0x18:
            if (reg_bank <= 0x0007) {
                if (reg_bank == 0x0007) {
                    data |= 0x8000;
                }
                data = (data & ~(0x0007)) | reg_bank;
            } else {
                rv = SOC_E_PARAM;
            }
            break;
        case 0x1C:
            if (reg_bank <= 0x001F) {
                data = 0x8000 | (reg_bank << 10) | (data & 0x03FF);
            } else {
                rv = SOC_E_PARAM;
            }
            break;
        case 0x1D:
            if (reg_bank == 0x0000) {
                data = data & 0x07FFF;
            } else {
                rv = SOC_E_PARAM;
            }
            break;
        }
        if (SOC_SUCCESS(rv)) {
            rv = PHY_BCM542XX_WRITE_PHY_REG(unit, pc, reg_addr, data);
        }
    } 

    if (SOC_FAILURE(rv)) {
        SOC_DEBUG_PRINT((DK_PHY,"phy_bcm542xx_reg_write: failed:"
                        "phy_id=0x%2x reg_bank=0x%04x reg_addr=0x%02x "
                        "rv=%d\n", pc->phy_id, reg_bank, reg_addr, rv)); 
    }
    
    return rv;
}

int
phy_bcm542xx_reg_modify(int unit, phy_ctrl_t *pc, uint32 flags, uint16 reg_bank,
                            uint8 reg_addr, uint16 data, uint16 mask)
{
    int     rv;
    uint16 val;

    rv       = SOC_E_NONE;
    if (flags & PHY_BCM542XX_REG_1000X) {
        if (reg_addr <= 0x000f) {
            uint16 blk_sel;

            /* Map 1000X page */
            val = 0x7c00; 
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x1c, val));
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_READ_PHY_REG(unit, pc, 0x1c, &blk_sel));
            val = blk_sel | 0x8001;
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x1c, val));

            /* Modify 1000X IEEE register */
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_MODIFY_PHY_REG(unit, pc, reg_addr, data, mask));

           /* Restore IEEE mapping */
            val = (blk_sel & 0xfffe) | 0x8000;
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x1c, val));
        } else {
            rv = SOC_E_PARAM;
        }
    } else {
        switch(reg_addr) {
        /* Map shadow registers */
        case 0x15:
            SOC_IF_ERROR_RETURN
                (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x17, reg_bank));
            break;
        case 0x18:
            if (reg_bank <= 0x0007) {
                val = (reg_bank << 12) | 0x7;
                SOC_IF_ERROR_RETURN
                    (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, reg_addr, val));

                if (reg_bank == 0x0007) {
                    data |= 0x8000;
                    mask |= 0x8000;
                }
                mask &= ~(0x0007);
            } else {
                rv = SOC_E_PARAM;
            }
            break;
        case 0x1C:
            if (reg_bank <= 0x001F) {
                val = (reg_bank << 10);
                SOC_IF_ERROR_RETURN
                    (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, reg_addr, val));
                data |= 0x8000;
                mask |= 0x8000;
                mask &= ~(0x1F << 10);
            } else {
                rv = SOC_E_PARAM;
            }
            break;
        case 0x1D:
            if (reg_bank == 0x0000) {
                mask &= 0x07FFF;
            } else {
                rv = SOC_E_PARAM;
            }
            break;
        }
        if (SOC_SUCCESS(rv)) {
            rv = PHY_BCM542XX_MODIFY_PHY_REG(unit, pc, reg_addr, data, mask);
        }
    }

    if (SOC_FAILURE(rv)) {
        SOC_DEBUG_PRINT((DK_PHY,"phy_bcm542xx_reg_modify: failed:"
                        "phy_id=0x%2x reg_bank=0x%04x reg_addr=0x%02x "
                        "rv=%d\n", pc->phy_id, reg_bank, reg_addr, rv)); 
    }
    return rv;
}

int 
phy_bcm542xx_reg_read_modify_write(int unit, phy_ctrl_t *pc, uint32 reg_addr,
                             uint16 reg_data, uint16 reg_mask)
{
    uint16  tmp, otmp;

    reg_data = reg_data & reg_mask;

    SOC_IF_ERROR_RETURN
        (PHY_BCM542XX_READ_PHY_REG(unit, pc, reg_addr, &tmp));
    otmp = tmp;
    tmp &= ~(reg_mask);
    tmp |= reg_data;

    if (otmp != tmp) {
        SOC_IF_ERROR_RETURN
            (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, reg_addr, tmp));
    }
    return SOC_E_NONE;
}


/*
 * Function:
 *     phy_bcm542xx_direct_reg_read
 * Purpose:
 * Parameters:
 * Returns:
 */
int phy_bcm542xx_direct_reg_read(int unit, phy_ctrl_t *pc, uint16 reg_addr, uint16 *data) 
{

    uint16 temp;

    PHY_BCM542XX_EN_RDB_MODE(temp);
    reg_addr &= PHY_BCM542XX_RDB_ADDR_REG_ADDR;
    /*
     MDIO write to reg 0x1E[15:0] for the address
     */
    SOC_IF_ERROR_RETURN(
        PHY_BCM542XX_WRITE_PHY_REG(unit, pc, PHY_BCM542XX_RDB_ADDR_REG_OFFSET, reg_addr));


    /*
     MDIO read from reg 0x1F[15:0] for the data
     */
    SOC_IF_ERROR_RETURN(
        PHY_BCM542XX_READ_PHY_REG(unit, pc, PHY_BCM542XX_RDB_DATA_REG_OFFSET, data ));
    PHY_BCM542XX_DIS_RDB_MODE(temp);

    return SOC_E_NONE;
}


/*
 * Function:
 *    phy_bcm542xx_direct_reg_write
 * Purpose:
 * Parameters:
 * Returns:
 */
int phy_bcm542xx_direct_reg_write(int unit, phy_ctrl_t *pc, uint16 reg_addr, uint16 data) 
{

    uint16 temp;

    PHY_BCM542XX_EN_RDB_MODE(temp);

    reg_addr &= PHY_BCM542XX_RDB_ADDR_REG_ADDR;
    /*
     MDIO write to reg 0x1E[15:0] for the address
     */
    SOC_IF_ERROR_RETURN(
        PHY_BCM542XX_WRITE_PHY_REG(unit, pc, PHY_BCM542XX_RDB_ADDR_REG_OFFSET, reg_addr));


    data &= PHY_BCM542XX_RDB_DATA_REG_DATA;
    /*
     MDIO write to reg 0x1F[15:0] for the data
     */
    SOC_IF_ERROR_RETURN(
        PHY_BCM542XX_WRITE_PHY_REG(unit, pc, PHY_BCM542XX_RDB_DATA_REG_OFFSET, data ));

    PHY_BCM542XX_DIS_RDB_MODE(temp);
    return SOC_E_NONE;
}

/*
 * Function:
 *    phy_bcm542xx_direct_reg_modify
 * Purpose:
 * Parameters:
 * Returns:
 */
int phy_bcm542xx_direct_reg_modify(int unit, phy_ctrl_t *pc, uint16 reg_addr, uint16 data, uint16 mask) 
{

    uint16 temp;

    PHY_BCM542XX_EN_RDB_MODE(temp);
    /*
     MDIO write to reg 0x1E[15:0] for the address
     */
    SOC_IF_ERROR_RETURN(
        PHY_BCM542XX_WRITE_PHY_REG(unit, pc, PHY_BCM542XX_RDB_ADDR_REG_OFFSET, reg_addr));

    /*
     MDIO modify reg 0x1F[15:0] for the data & mask. 
     */
    SOC_IF_ERROR_RETURN(
        PHY_BCM542XX_MODIFY_PHY_REG(unit, pc, PHY_BCM542XX_RDB_DATA_REG_OFFSET, data,  mask));

    PHY_BCM542XX_DIS_RDB_MODE(temp);
    return SOC_E_NONE;
}



/*
 *
 * QSGMII register READ
 */
int 
phy_bcm542xx_qsgmii_reg_read(int unit, phy_ctrl_t *pc,
                          int dev_port, uint16 block, uint8 reg, 
                          uint16 *data)
{
    uint16 val;
    
    /* Lanes from 0 to 7 */
    if ((dev_port < 0) || (dev_port > 7)) {
        return SOC_E_FAIL;
    }

    /* Set BAR to AER */
    val = 0xFFD0;
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_reg_write(unit, pc,
                                    PHY_BCM542XX_REG_QSGMII, 0, 0x1F, val));

    /* Set AER reg to access SGMII lane */
    val = dev_port;
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_reg_write(unit, pc,
                                    PHY_BCM542XX_REG_QSGMII, 0, 0x1E, val));

    /* Set BAR to Register Block */
    val = (block & 0xfff0);
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_reg_write(unit, pc,
                                    PHY_BCM542XX_REG_QSGMII, 0, 0x1F, val));
    /* Read the register */
    if (block >= 0x8000) {
        reg |= 0x10;
    }

    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_reg_read(unit, pc,
                                   PHY_BCM542XX_REG_QSGMII, 0, reg, data));

    return SOC_E_NONE;

}

/*
 * QSGMII register WRITE
 */
int 
phy_bcm542xx_qsgmii_reg_write(int unit, phy_ctrl_t *pc, 
        int dev_port, uint16 block, uint8 reg, 
        uint16 data)
{
    uint16 val;

    /* Lanes from 0 to 7 */
    if ((dev_port < 0) || (dev_port > 7)) {
        return SOC_E_FAIL;
    }

    /* set bar to aer */
    val = 0xffd0;
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_reg_write(unit, pc,
                                PHY_BCM542XX_REG_QSGMII, 0, 0x1F, val));

    /* set aer reg to access sgmii lane */
    val = dev_port;
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_reg_write(unit, pc,
                                PHY_BCM542XX_REG_QSGMII, 0, 0x1E, val));

    /* set bar to register block */
    val = (block & 0xfff0);
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_reg_write(unit, pc,
                                PHY_BCM542XX_REG_QSGMII, 0, 0x1F, val));

    /* Write the register */
    if (block >= 0x8000) {
        reg |= 0x10;
    }

    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_reg_write(unit, pc,
                                PHY_BCM542XX_REG_QSGMII, 0, reg, data));

    return SOC_E_NONE;
}



int 
phy_bcm542xx_qsgmii_reg_modify(int unit, phy_ctrl_t *pc, 
                            int dev_port, uint16 block, 
                            uint8 reg_addr, uint16 reg_data, 
                            uint16 reg_mask)
{
    uint16  tmp, otmp;

    reg_data = reg_data & reg_mask;

    phy_bcm542xx_qsgmii_reg_read(unit, pc,
                              dev_port, block, reg_addr, &tmp);
    otmp = tmp;
    tmp &= ~(reg_mask);
    tmp |= reg_data;

    if (otmp != tmp) {
            phy_bcm542xx_qsgmii_reg_write(unit, pc, 
                                       dev_port, block, reg_addr, tmp);
    }
    return SOC_E_NONE;
}


/*
 * Function:
 *      phy_bcm542xx_cl45_reg_read
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

int
phy_bcm542xx_cl45_reg_read(int unit, phy_ctrl_t *pc,  
                         uint8 dev_addr, uint16 reg_addr, uint16 *val)
{

    uint16 temp16 = (dev_addr & 0x001f);
    /* Write Device Address to register 0x0D (Set Function field to Address)*/
    SOC_IF_ERROR_RETURN
        (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x0D, temp16));

    /* Select the register by writing to register address to register 0x0E */
    SOC_IF_ERROR_RETURN
        (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x0E, reg_addr));


    temp16 = 0x4000 | (dev_addr & 0x001f);
    /* Write Device Address to register 0x0D (Set Function field to Data)*/
    SOC_IF_ERROR_RETURN
        (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x0D, temp16)); 

    /* Read register 0x0E to get the value */
    SOC_IF_ERROR_RETURN
        (PHY_BCM542XX_READ_PHY_REG(unit, pc, 0x0E, val));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_bcm542xx_cl45_reg_write
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

int
phy_bcm542xx_cl45_reg_write(int unit, phy_ctrl_t *pc,  
                         uint8 dev_addr, uint16 reg_addr, uint16 val)
{
    uint16 temp16 = (dev_addr & 0x001f);
    /* Write Device Address to register 0x0D (Set Function field to Address)*/
    SOC_IF_ERROR_RETURN
        (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x0D, temp16));

    /* Select the register by writing to register address to register 0x0E */
    SOC_IF_ERROR_RETURN
        (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x0E, reg_addr));

    temp16 = 0x4000 | (dev_addr & 0x001f);
    /* Write Device Address to register 0x0D (Set Function field to Data)*/
    SOC_IF_ERROR_RETURN
        (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x0D, temp16)); 

    /* Write register 0x0E to write the value */
    SOC_IF_ERROR_RETURN
        (PHY_BCM542XX_WRITE_PHY_REG(unit, pc, 0x0E, val));

    return SOC_E_NONE;
}

/*
 * Function:
 *     _phy_bcm542xx_cl45_reg_modify
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

int
phy_bcm542xx_cl45_reg_modify(int unit, phy_ctrl_t *pc, 
                          uint8 dev_addr, uint16 reg_addr, uint16 val, 
                          uint16 mask)
{

    uint16 value = 0;

    phy_bcm542xx_cl45_reg_read(unit, pc, dev_addr, reg_addr, &value);

    value = (val & mask) | (value & ~mask);

    phy_bcm542xx_cl45_reg_write(unit, pc, dev_addr, reg_addr, value);
    
    return SOC_E_NONE;
}

/*
 * Function:     
 *    _phy_bcm542xx_get_model_rev
 * Purpose:    
 *    Get OUI, Model and Revision of the PHY
 * Parameters:
 *    phy_dev_addr - PHY Device Address
 *    oui          - (OUT) Organization Unique Identifier
 *    model        - (OUT)Device Model number`
 *    rev          - (OUT)Device Revision number
 * Notes: 
 */
STATIC int 
_phy_bcm542xx_get_model_rev(int unit, phy_ctrl_t *pc,
                               int *oui, int *model, int *rev)
{
    uint16  id0, id1;

    SOC_IF_ERROR_RETURN
        (PHY_READ_BCM542XX_MII_PHY_ID0r(unit, pc, &id0));

    SOC_IF_ERROR_RETURN
        (PHY_READ_BCM542XX_MII_PHY_ID1r(unit, pc, &id1));

    *oui   = PHY_OUI(id0, id1);
    *model = PHY_MODEL(id0, id1);
    *rev   = PHY_REV(id0, id1);

    return SOC_E_NONE;
}


/*
 * Function:     
 *    _phy_bcm542xx_auto_negotiate_ew (Autoneg-ed mode with E@W on).
 * Purpose:    
 *    Determine autoneg-ed mode between
 *    two ends of a link
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #. 
 *    speed - (OUT) greatest common speed.
 *    duplex - (OUT) greatest common duplex.
 *    link - (OUT) Boolean, true indicates link established.
 * Returns:    
 *    SOC_E_XXX
 * Notes: 
 *    No synchronization performed at this level.
 */

static int
_phy_bcm542xx_auto_negotiate_ew(int unit, phy_ctrl_t *pc, int *speed, int *duplex)
{
    int        t_speed, t_duplex;
    uint16  mii_assr;

    SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_MII_AUX_STATUSr(unit, pc, &mii_assr));

    switch ((mii_assr >> 8) & 0x7) {
    case 0x7:
        t_speed = 1000;
        t_duplex = TRUE;
        break;
    case 0x6:
        t_speed = 1000;
        t_duplex = FALSE;
        break;
    case 0x5:
        t_speed = 100;
        t_duplex = TRUE;
        break;
    case 0x3:
        t_speed = 100;
        t_duplex = FALSE;
        break;
    case 0x2:
        t_speed = 10;
        t_duplex = TRUE;
        break;
    case 0x1:
        t_speed = 10;
        t_duplex = FALSE;
        break;
    default:
        t_speed = 0; /* 0x4 is 100BASE-T4 which is not supported */
        t_duplex = FALSE;
        break;
    }

    if (speed)  *speed  = t_speed;
    if (duplex)    *duplex = t_duplex;

    return(SOC_E_NONE);
}



/*
 * Function:     
 *    _phy_bcm542xx_auto_negotiate_gcd (greatest common denominator).
 * Purpose:    
 *    Determine the current greatest common denominator between 
 *    two ends of a link
 * Parameters:
 *    speed - (OUT) greatest common speed.
 *    duplex - (OUT) greatest common duplex.
 *    link - (OUT) Boolean, true indicates link established.
 * Notes: 
 */

STATIC int
_phy_bcm542xx_auto_negotiate_gcd(int unit, phy_ctrl_t *pc, int *speed, int *duplex)
{
    int       t_speed, t_duplex;
    uint16 mii_ana, mii_anp, mii_stat;
    uint16 mii_gb_stat, mii_esr, mii_gb_ctrl;

    mii_gb_stat = 0;            /* Start off 0 */
    mii_gb_ctrl = 0;            /* Start off 0 */

    SOC_IF_ERROR_RETURN(
        PHY_READ_BCM542XX_MII_ANAr(unit, pc, &mii_ana));
    SOC_IF_ERROR_RETURN(
        PHY_READ_BCM542XX_MII_ANPr(unit, pc, &mii_anp));
    SOC_IF_ERROR_RETURN(
        PHY_READ_BCM542XX_MII_STATr(unit, pc, &mii_stat));



    if (mii_stat & PHY_BCM542XX_MII_STAT_ES) {    /* Supports extended status */
        /*
         * If the PHY supports extended status, check if it is 1000MB
         * capable.  If it is, check the 1000Base status register to see
         * if 1000MB negotiated.
         */
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_MII_ESRr(unit, pc, &mii_esr));

        if (mii_esr & (PHY_BCM542XX_MII_ESR_1000_X_FD | PHY_BCM542XX_MII_ESR_1000_X_HD | 
                       PHY_BCM542XX_MII_ESR_1000_T_FD | PHY_BCM542XX_MII_ESR_1000_T_HD)) {
            SOC_IF_ERROR_RETURN(
                PHY_READ_BCM542XX_MII_GB_STATr(unit, pc, 
                                                   &mii_gb_stat));
            SOC_IF_ERROR_RETURN(
                PHY_READ_BCM542XX_MII_GB_CTRLr(unit, pc, 
                                                   &mii_gb_ctrl));
        }
    }

    /*
     * At this point, if we did not see Gig status, one of mii_gb_stat or 
     * mii_gb_ctrl will be 0. This will cause the first 2 cases below to 
     * fail and fall into the default 10/100 cases.
     */


    mii_ana &= mii_anp;

    if ((mii_gb_ctrl & PHY_BCM542XX_MII_GB_CTRL_ADV_1000FD) &&
        (mii_gb_stat & PHY_BCM542XX_MII_GB_STAT_LP_1000FD)) {
        t_speed  = 1000;
        t_duplex = 1;
    } else if ((mii_gb_ctrl & PHY_BCM542XX_MII_GB_CTRL_ADV_1000HD) &&
               (mii_gb_stat & PHY_BCM542XX_MII_GB_STAT_LP_1000HD)) {
        t_speed  = 1000;
        t_duplex = 0;
    } else if (mii_ana & PHY_BCM542XX_MII_ANA_FD_100) {        /* [a] */
        t_speed = 100;
        t_duplex = 1;
    } else if (mii_ana & PHY_BCM542XX_MII_ANA_T4) {            /* [b] */
        t_speed = 100;
        t_duplex = 0;
    } else if (mii_ana & PHY_BCM542XX_MII_ANA_HD_100) {        /* [c] */
        t_speed = 100;
        t_duplex = 0;
    } else if (mii_ana & PHY_BCM542XX_MII_ANA_FD_10) {         /* [d] */
        t_speed = 10;
        t_duplex = 1 ;
    } else if (mii_ana & PHY_BCM542XX_MII_ANA_HD_10) {         /* [e] */
        t_speed = 10;
        t_duplex = 0;
    } else {
        return(SOC_E_FAIL);
    }

    if (speed) {
        *speed  = t_speed;
    }
    if (duplex) {
        if (t_duplex) {
            *duplex = TRUE;
        } else {
            *duplex = FALSE;
        }
    }

    return SOC_E_NONE;
}
    





/*
 * Function:
 *      _phy_bcm542xx_medium_check
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
_phy_bcm542xx_medium_check(int unit, soc_port_t port, int *medium)
{

    phy_ctrl_t    *pc = EXT_PHY_SW_STATE(unit, port);
    uint16 mode_ctrl;

    *medium = SOC_PORT_MEDIUM_COPPER;

    if (pc->automedium) {
        
        SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_MODE_CTRLr(unit, pc, &mode_ctrl));

        if (pc->fiber.preferred) {
           if ((mode_ctrl & 0x30) == 0x20) {
                /* Only Copper is linked up */
                *medium = SOC_PORT_MEDIUM_COPPER;
            } else {
                *medium = SOC_PORT_MEDIUM_FIBER;
            }
        } else {
            if ((mode_ctrl & 0x30) == 0x10) {
                /* Only Fiber is linked up */
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
                     "_phy_bcm542xx_medium_check: "
                     "u=%d p=%d fiber_pref=%d fiber=%d\n",
                     unit, port, pc->fiber.preferred,
                     (*medium == SOC_PORT_MEDIUM_FIBER) ? 1 : 0));

    return SOC_E_NONE;
}
 

/*
 * Function:
 *      phy_bcm542xx_medium_status
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
phy_bcm542xx_medium_status(int unit, soc_port_t port, soc_port_medium_t *medium)
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
 *      _phy_bcm542xx_medium_config_update
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
_phy_bcm542xx_medium_config_update(int unit, soc_port_t port,
                                soc_phy_config_t *cfg)
{
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_speed_set(unit, port, cfg->force_speed));
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_duplex_set(unit, port, cfg->force_duplex));
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_master_set(unit, port, cfg->master));
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_ability_advert_set(unit, port, &cfg->advert_ability));
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_autoneg_set(unit, port, cfg->autoneg_enable));
    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_mdix_set(unit, port, cfg->mdix));

    return SOC_E_NONE;
}


/***********************************************************************
 *
 * PHY542xx DRIVER ROUTINES
 *
 ***********************************************************************/





/*
 * Function:
 *      phy_bcm542xx_medium_config_set
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
phy_bcm542xx_medium_config_set(int unit, soc_port_t port, 
                           soc_port_medium_t  medium,
                           soc_phy_config_t  *cfg)
{
    phy_ctrl_t    *pc;
    soc_phy_config_t *active_medium;  /* Currently active medium */
    soc_phy_config_t *change_medium;  /* Requested medium */
    soc_phy_config_t *other_medium;   /* The other medium */
    int               medium_update;
    soc_port_ability_t advert_ability;
    sal_memset(&advert_ability, 0, sizeof(soc_port_ability_t));

    if (NULL == cfg) {
        return SOC_E_PARAM;
    }

    pc            = EXT_PHY_SW_STATE(unit, port);
    medium_update = FALSE;

   switch (medium) {
    case SOC_PORT_MEDIUM_COPPER:
        if (!pc->automedium && !PHY_COPPER_MODE(unit, port)) {
            return SOC_E_UNAVAIL;
        }
        change_medium  = &pc->copper;
        other_medium   = &pc->fiber;
        ADVERT_ALL_COPPER(&advert_ability);

        break;
    case SOC_PORT_MEDIUM_FIBER:
        if (!pc->automedium && !PHY_FIBER_MODE(unit, port)) {
            return SOC_E_UNAVAIL;
        }
        change_medium  = &pc->fiber;
        other_medium   = &pc->copper;
        ADVERT_ALL_FIBER(&advert_ability);
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
    change_medium->advert_ability.pause &= advert_ability.pause;
    change_medium->advert_ability.speed_half_duplex &= advert_ability.speed_half_duplex;
    change_medium->advert_ability.speed_full_duplex &= advert_ability.speed_full_duplex;
    change_medium->advert_ability.interface &= advert_ability.interface;
    change_medium->advert_ability.medium &= advert_ability.medium;
    change_medium->advert_ability.loopback &= advert_ability.loopback;
    change_medium->advert_ability.flags &= advert_ability.flags;

    

    if (medium_update) {
        /* The new configuration may cause medium change. Check
         * and update medium.
         */
        SOC_IF_ERROR_RETURN
            (_phy_bcm542xx_medium_change(unit, port, TRUE, medium));
    } else {
        active_medium = (PHY_COPPER_MODE(unit, port)) ?  
                            &pc->copper : &pc->fiber;
        if (active_medium == change_medium) {
            /* If the medium to update is active, update the configuration */
            SOC_IF_ERROR_RETURN
                (_phy_bcm542xx_medium_config_update(unit, port, change_medium));
        }
    }
    return SOC_E_NONE;
}


/*
 * Function:
 *      phy_bcm542xx_medium_config_get
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
phy_bcm542xx_medium_config_get(int unit, soc_port_t port, 
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
 *      _phy_bcm542xx_no_reset_setup
 * Purpose:
 *      Setup the operating parameters of the PHY without resetting PHY
 * Parameters:
 *      unit - StrataSwitch unit #
 *      port - Port number
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
_phy_bcm542xx_no_reset_setup(int unit, soc_port_t port)
{
    phy_ctrl_t    *pc;
    int rv = SOC_E_NONE;

    SOC_DEBUG_PRINT((DK_PHY, "_phy_bcm542xx_reset_setup: u=%d p=%d medium=%s\n",
                     unit, port,
                     PHY_COPPER_MODE(unit, port) ? "COPPER" : "FIBER"));

    pc      = EXT_PHY_SW_STATE(unit, port);

    rv = phy_bcm542xx_init_setup(unit, port, 0, 
                                     pc->automedium,
                                     pc->fiber.preferred,
                                     pc->fiber_detect,
                                     pc->fiber.enable,
                                     pc->copper.enable); 

    if (rv != SOC_E_NONE) {
        return SOC_E_FAIL;
    }
    return rv;
}

/*
 * Function:     
 *    phy_bcm542xx_set_led_selectors
 * Purpose:    
 *    Set LED modes and Control.
 * Parameters:
 *    phy_dev_addr    - PHY's device address
 *    led_mode0       - LED Mode 0
 *    led_mode1       - LED Mode 1
 *    led_mode2       - LED Mode 2
 *    led_mode3       - LED Mode 3
 *    led_select      - LED select 
 *    led_select_ctrl        - LED Control
 * Returns:    
 */
int
phy_bcm542xx_set_led_selectors(int unit, phy_ctrl_t *pc)
{
    uint16 data;
    uint16 led_link_speed_mode;

    /* Configure LED selectors */
    data = (((pc->ledmode[1] & 0xf) << 4) | (pc->ledmode[0] & 0xf));
    SOC_IF_ERROR_RETURN
        (PHY_WRITE_BCM542XX_LED_SELECTOR_1r(unit, pc, data));

    data = ((pc->ledmode[3] & 0xf) << 4) | (pc->ledmode[2] & 0xf);
    SOC_IF_ERROR_RETURN
        (PHY_WRITE_BCM542XX_LED_SELECTOR_2r(unit, pc, data));

    data = (pc->ledctrl & 0x3ff);
    SOC_IF_ERROR_RETURN
        (PHY_WRITE_BCM542XX_LED_CTRLr(unit, pc, data));

    SOC_IF_ERROR_RETURN
        (PHY_WRITE_BCM542XX_EXP_LED_SELECTORr(unit, pc, pc->ledselect));

    led_link_speed_mode = soc_property_port_get(unit, pc->port, 
                              spn_PHY_LED_LINK_SPEED_MODE, 0x0);
    switch(led_link_speed_mode) {
        case 1:
            SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_SPARE_CTRLr(unit, pc, 
                     PHY_BCM542XX_SPARE_CTRL_REG_LINK_LED, 
                     PHY_BCM542XX_SPARE_CTRL_REG_LINK_LED
                     | PHY_BCM542XX_SPARE_CTRL_REG_LINK_SPEED_LED));
            break;
        case 2:
            SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_SPARE_CTRLr(unit, pc, 
                     PHY_BCM542XX_SPARE_CTRL_REG_LINK_SPEED_LED, 
                     PHY_BCM542XX_SPARE_CTRL_REG_LINK_SPEED_LED
                     | PHY_BCM542XX_SPARE_CTRL_REG_LINK_LED));
            break;
        default:
            SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_SPARE_CTRLr(unit, pc, 
                     0,
                     PHY_BCM542XX_SPARE_CTRL_REG_LINK_LED  
                     | PHY_BCM542XX_SPARE_CTRL_REG_LINK_SPEED_LED));
            break;
    }   

    return SOC_E_NONE;
}



/*
 * Function:     
 *    phy_bcm54280_init
 * Purpose:    
 *    54280 Device specific Initialization 
 * Parameters:
 *    unit, port          - 
 *    automedium          - Automedium select in combo ports (Don't care)
 * Notes: 
 *
 */
STATIC int 
phy_bcm54280_init(int unit, soc_port_t port, 
                       int automedium)
{
    phy_ctrl_t    *pc;
    pc      = EXT_PHY_SW_STATE(unit, port);

    /* 
    * Configure extended control register for led mode and 
    * jumbo frame support
    */
    SOC_IF_ERROR_RETURN
       (PHY_MODIFY_BCM542XX_MII_AUX_CTRLr(unit, pc, 
                                              AUX_CTRL_REG_EXT_PKT_LEN, 
                                              AUX_CTRL_REG_EXT_PKT_LEN));

    SOC_IF_ERROR_RETURN
    (PHY_MODIFY_BCM542XX_EXP_PATT_GEN_STATr(unit, pc, 
                                      PHY_BCM542XX_PATT_GEN_STAT_FIFO_ELAST_1,
                                      PHY_BCM542XX_PATT_GEN_STAT_FIFO_ELAST_1));
    
    SOC_IF_ERROR_RETURN
    (PHY_MODIFY_BCM542XX_MII_ECRr(unit, pc, 
                                      PHY_BCM542XX_MII_ECR_FIFO_ELAST_0
                                      | PHY_BCM542XX_MII_ECR_EN_LEDT, 
                                      PHY_BCM542XX_MII_ECR_FIFO_ELAST_0
                                      | PHY_BCM542XX_MII_ECR_EN_LEDT));

    return SOC_E_NONE;
}




/*
 * Function:     
 *    phy_bcm54282_init
 * Purpose:    
 *    Initalize the PHY device. 
 * Parameters:
 *    unit, port          - 
 *    automedium          - Automedium select in combo ports
 * Notes: 
 */
int
phy_bcm54282_init(int unit, soc_port_t port, int automedium) 
{
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* 
    * Configure extended control register for led mode and 
    * jumbo frame support
    */
    
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc); /* RV */

    /* Enable QSGMII MDIO sharing feature 
       - Enable access to QSGMII reg space using port3's MDIO address
       - Use PHYA[4:0]+3 (Port 3's MDIO) instead of PHYA[4:0)+8 
         as MDIO address..
       - It saves one MDIO address for customer.
       - Access to this top level register via port 0 only
     */
    phy_bcm542xx_direct_reg_modify(unit, pc,
                               PHY_BCM542XX_TOP_MISC_TOP_CFG_REG_OFFSET,
                               PHY_BCM542XX_TOP_MISC_CFG_REG_QSGMII_SEL
                               | PHY_BCM542XX_TOP_MISC_CFG_REG_QSGMII_PHYA,
                               PHY_BCM542XX_TOP_MISC_CFG_REG_QSGMII_SEL
                               | PHY_BCM542XX_TOP_MISC_CFG_REG_QSGMII_PHYA);
    
    /* replace it with qsgmii phy id */
    pc->phy_id = PHY_BCM54282_QSGMII_DEV_ADDR_SHARED(pc);    
    
    /* QSGMII FIFO Elasticity */
    phy_bcm542xx_qsgmii_reg_write(unit, pc, 
                                      (int)PHY_BCM542XX_DEV_PHY_SLICE(pc) /*dev_port*/,
                                      0x8300, 0x12, 0x0006);
    
    /* Restore access to Copper/Fiber register space.
       TOP lvl register, access through port0 only
    */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);

    phy_bcm542xx_direct_reg_modify(unit, pc,
                               PHY_BCM542XX_TOP_MISC_TOP_CFG_REG_OFFSET,
                               0,
                               PHY_BCM542XX_TOP_MISC_CFG_REG_QSGMII_SEL
                               | PHY_BCM542XX_TOP_MISC_CFG_REG_QSGMII_PHYA);
    /* Restore the phy mdio address */
    pc->phy_id =  PHY_BCM542XX_DEV_PHY_ID_ORIG(pc); 

    SOC_IF_ERROR_RETURN
       (PHY_MODIFY_BCM542XX_MII_AUX_CTRLr(unit, pc, 
                                              AUX_CTRL_REG_EXT_PKT_LEN, 
                                              AUX_CTRL_REG_EXT_PKT_LEN));

    SOC_IF_ERROR_RETURN
    (PHY_MODIFY_BCM542XX_EXP_PATT_GEN_STATr(unit, pc, 
                                      PHY_BCM542XX_PATT_GEN_STAT_FIFO_ELAST_1,
                                      PHY_BCM542XX_PATT_GEN_STAT_FIFO_ELAST_1));
    
    SOC_IF_ERROR_RETURN
    (PHY_MODIFY_BCM542XX_MII_ECRr(unit, pc, 
                                      PHY_BCM542XX_MII_ECR_FIFO_ELAST_0
                                      | PHY_BCM542XX_MII_ECR_EN_LEDT, 
                                      PHY_BCM542XX_MII_ECR_FIFO_ELAST_0
                                      | PHY_BCM542XX_MII_ECR_EN_LEDT));

    /* In case of QSGMII devices, there is a LPI pass through mode
       which has to be enabled for native EEE to work. In case of VNG, 
       it is already set by default. To enable it: 
        
    SOC_IF_ERROR_RETURN
       (phy_bcm542xx_qsgmii_reg_write(PHY_BCM54282_QSGMII_DEV_ADDR(_pc), 
                                      (int)PHY_BCM542XX_DEV_PHY_SLICE(pc),
                                      0x833e, 0x0e, 0xc000));
    */
    return SOC_E_NONE;
}

/*
 * Function:     
 *    phy_bcm54240_init
 * Purpose:    
 *    Initalize the PHY device. If MACSEC is enabled, initialize the MACS.
 * Parameters:
 *    automedium          - Automedium select in combo ports
 *    fiber_preferred     - Fiber preferrence over copper
 *    fiber_detect        - Fiber Signal Detect
 * Notes: 
 * Fiber Detect  Detect Fiber signal 
 * Automedium        Fiber Preferred         Active Medium
 * -----------------------------------------------------------------
 *     0                 1                   Fiber mode is forced
 *     0                 0                   Copper mode is forced
 *     1                 1                   Auto medium is enabled
 *                                           (with Fiber preferred)
 *     1                 0                   Auto medium is enabled
 *                                           (with Copper preferred)
 */
int
phy_bcm54240_init(int unit, soc_port_t port, int automedium, 
                      int fiber_preferred, int fiber_detect,
                      int fiber_enable, int copper_enable)
{
    uint16         data, mask;
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);


    SOC_IF_ERROR_RETURN
        (PHY_READ_BCM542XX_MODE_CTRLr(unit, pc, &data));
#if 0  
    /*----------- COPPER INTERFACE---------- */
    if(data & BMACSEC_MODE_CNTL_REG_CU_ENG_DET) 
#endif
    {

        /* SGMII to copper mode */
        SOC_IF_ERROR_RETURN
           (PHY_MODIFY_BCM542XX_MODE_CTRLr(unit, pc,MODE_SEL_OPTION_COPPER_2_SGMII, 0x0006));

        /* copper regs */
        if (!copper_enable || (!automedium && fiber_preferred)) {
            SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_MII_CTRLr(unit, pc,
                                       PHY_BCM542XX_MII_CTRL_PWR_DOWN,
                                       PHY_BCM542XX_MII_CTRL_PWR_DOWN));
        } else {

            SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_MII_CTRLr(unit, pc,
                                       0,
                                       PHY_BCM542XX_MII_CTRL_PWR_DOWN));

            SOC_IF_ERROR_RETURN
                (PHY_WRITE_BCM542XX_MII_GB_CTRLr(unit, pc,  
                                        PHY_BCM542XX_MII_GB_CTRL_ADV_1000FD 
                                        | PHY_BCM542XX_MII_GB_CTRL_PT));
            SOC_IF_ERROR_RETURN
                (PHY_WRITE_BCM542XX_MII_CTRLr(unit, pc, 
                                        PHY_BCM542XX_MII_CTRL_FD 
                                        | PHY_BCM542XX_MII_CTRL_SS_10 
                                        | PHY_BCM542XX_MII_CTRL_SS_100 
                                        | PHY_BCM542XX_MII_CTRL_AN_EN
                                        | PHY_BCM542XX_MII_CTRL_RST_AN));
        }
        /* Enable auto-detection between SGMII-slave and 1000BASE-X */
        SOC_IF_ERROR_RETURN
               (PHY_MODIFY_BCM542XX_SGMII_SLAVEr(unit, pc, 1, 1));

         /* Power down SerDes */
         SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_1000X_MII_CTRLr(unit, pc,
                                         PHY_BCM542XX_MII_CTRL_PWR_DOWN,
                                         PHY_BCM542XX_MII_CTRL_PWR_DOWN));

    } 
#if 0
    if (data & BMACSEC_MODE_CNTL_REG_FBER_SIG_DET) 
#endif
    {
    /*----------- FIBER INTERFACE---------- */
 
         if (fiber_enable && (automedium || fiber_preferred)) {

             /* remove power down of SerDes */
             SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_1000X_MII_CTRLr(unit, pc,
                                         0,
                                         PHY_BCM542XX_MII_CTRL_PWR_DOWN));
             /* set the advertisement of serdes */
             /* Disable half-duplex, full-duplex capable */
             SOC_IF_ERROR_RETURN  
                (PHY_MODIFY_BCM542XX_1000X_MII_ANAr(unit, pc,
                                             (1 << 5) | (3 << 7) | (1 << 6), 
                                             (1 << 5) | (1 << 6)| (3 <<7)));

             /* Enable auto-detection between SGMII-slave and 1000BASE-X */
             SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_SGMII_SLAVEr(unit, pc, 1, 1));
          
             /* Restart SerDes autonegotiation */
             SOC_IF_ERROR_RETURN
                (PHY_WRITE_BCM542XX_1000X_MII_CTRLr(unit, pc,
                    PHY_BCM542XX_MII_CTRL_FD 
                    | PHY_BCM542XX_MII_CTRL_SS_1000 
                    | PHY_BCM542XX_MII_CTRL_AN_EN
                    | PHY_BCM542XX_MII_CTRL_RST_AN));

             

#if 0 
             SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_MISC_1000X_CONTROLr(unit, pc, 0x0000, 0x0020));
#endif

             SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_MODE_CTRLr(unit, pc, MODE_SEL_OPTION_FIBER_2_SGMII , 0x0006 ));    
             

         }

    }
     
    /* Configure Auto-detect Medium (0x1c shadow 11110) */
    mask = 0x33f;               /* Auto-detect bit mask */
    data = 0;
    if (automedium) {
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
    if (fiber_preferred) {
        data |= 1 << 1;    /* Fiber selected when both media are active */
        data |= 1 << 2;    /* Fiber selected when no medium is active */
    }

    /* Qulalify fiber link with SYNC from PCS. */
    data |= 1 << 5;
    
    /*
     * Enable internal inversion of LED2/SD input pin.  Used only if
     * the fiber module provides RX_LOS instead of Signal Detect.
     */
    if (fiber_detect < 0) {
         data |= 1 << 8;    /* Fiber signal detect is active low from pin */
    }
    
    SOC_IF_ERROR_RETURN
        (PHY_MODIFY_BCM542XX_AUTO_DETECT_MEDIUMr(unit, pc, data, mask));

     /* Configure extended control register for led mode and jumbo frame support
     */
    mask = PHY_BCM542XX_MII_ECR_EN_LEDT;

    /* Enable LEDs to indicate traffic status */
    data =  PHY_BCM542XX_MII_ECR_EN_LEDT; 

    
    SOC_IF_ERROR_RETURN
        (PHY_MODIFY_BCM542XX_MII_ECRr(unit, pc, data, mask));

    /* Enable extended packet length (4.5k through 25k) */
    SOC_IF_ERROR_RETURN
        (PHY_MODIFY_BCM542XX_MII_AUX_CTRLr(unit, pc, AUX_CTRL_REG_EXT_PKT_LEN, AUX_CTRL_REG_EXT_PKT_LEN));

    return SOC_E_NONE;
}

/*
 * Function:     
 *    phy_bcm54285_init
 * Purpose:    
 *    Initalize the PHY device. If MACSEC is enabled, initialize the MACS.
 * Parameters:
 *    automedium          - Automedium select in combo ports
 *    fiber_preferred     - Fiber preferrence over copper
 *    fiber_detect        - Fiber Signal Detect
 * Notes: 
 * Fiber Detect  Detect Fiber signal 
 * Automedium        Fiber Preferred         Active Medium
 * -----------------------------------------------------------------
 *     0                 1                   Fiber mode is forced
 *     0                 0                   Copper mode is forced
 *     1                 1                   Auto medium is enabled
 *                                           (with Fiber preferred)
 *     1                 0                   Auto medium is enabled
 *                                           (with Copper preferred)
 */
int
phy_bcm54285_init(int unit, soc_port_t port, int automedium, 
                      int fiber_preferred, int fiber_detect,
                      int fiber_enable, int copper_enable)
{
    uint16         data, mask;
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (PHY_READ_BCM542XX_MODE_CTRLr(unit, pc, &data));
#if 0  
    /*----------- COPPER INTERFACE---------- */
    if(data & BMACSEC_MODE_CNTL_REG_CU_ENG_DET) 
#endif
    {

        /* SGMII to copper mode */
        SOC_IF_ERROR_RETURN
           (PHY_MODIFY_BCM542XX_MODE_CTRLr(unit, pc,MODE_SEL_OPTION_COPPER_2_SGMII, 0x0006));

        /* copper regs */
        if (!copper_enable || (!automedium && fiber_preferred)) {
            SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_MII_CTRLr(unit, pc,
                                       PHY_BCM542XX_MII_CTRL_PWR_DOWN,
                                       PHY_BCM542XX_MII_CTRL_PWR_DOWN));
        } else {

            SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_MII_CTRLr(unit, pc,
                                       0,
                                       PHY_BCM542XX_MII_CTRL_PWR_DOWN));

            SOC_IF_ERROR_RETURN
                (PHY_WRITE_BCM542XX_MII_GB_CTRLr(unit, pc,  
                                        PHY_BCM542XX_MII_GB_CTRL_ADV_1000FD 
                                        | PHY_BCM542XX_MII_GB_CTRL_PT));
            SOC_IF_ERROR_RETURN
                (PHY_WRITE_BCM542XX_MII_CTRLr(unit, pc, 
                                        PHY_BCM542XX_MII_CTRL_FD 
                                        | PHY_BCM542XX_MII_CTRL_SS_10 
                                        | PHY_BCM542XX_MII_CTRL_SS_100 
                                        | PHY_BCM542XX_MII_CTRL_AN_EN
                                        | PHY_BCM542XX_MII_CTRL_RST_AN));
        }
        /* Enable auto-detection between SGMII-slave and 1000BASE-X */
        SOC_IF_ERROR_RETURN
               (PHY_MODIFY_BCM542XX_SGMII_SLAVEr(unit, pc, 1, 1));

         /* Power down SerDes */
         SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_1000X_MII_CTRLr(unit, pc,
                                         PHY_BCM542XX_MII_CTRL_PWR_DOWN,
                                         PHY_BCM542XX_MII_CTRL_PWR_DOWN));

    } 
#if 0
    if (data & BMACSEC_MODE_CNTL_REG_FBER_SIG_DET) 
#endif
    {
    /*----------- FIBER INTERFACE---------- */
 
         if (fiber_enable && (automedium || fiber_preferred)) {

             /* remove power down of SerDes */
             SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_1000X_MII_CTRLr(unit, pc,
                                         0,
                                         PHY_BCM542XX_MII_CTRL_PWR_DOWN));
             /* set the advertisement of serdes */
             /* Disable half-duplex, full-duplex capable */
             SOC_IF_ERROR_RETURN  
                (PHY_MODIFY_BCM542XX_1000X_MII_ANAr(unit, pc,
                                             (1 << 5) | (3 << 7) | (1 << 6), 
                                             (1 << 5) | (1 << 6)| (3 <<7)));

             /* Enable auto-detection between SGMII-slave and 1000BASE-X */
             SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_SGMII_SLAVEr(unit, pc, 1, 1));
          
             /* Restart SerDes autonegotiation */
             SOC_IF_ERROR_RETURN
                (PHY_WRITE_BCM542XX_1000X_MII_CTRLr(unit, pc,
                    PHY_BCM542XX_MII_CTRL_FD 
                    | PHY_BCM542XX_MII_CTRL_SS_1000 
                    | PHY_BCM542XX_MII_CTRL_AN_EN
                    | PHY_BCM542XX_MII_CTRL_RST_AN));

             

#if 0 
             SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_MISC_1000X_CONTROLr(unit, pc, 0x0000, 0x0020));
#endif

             SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_MODE_CTRLr(unit, pc, MODE_SEL_OPTION_FIBER_2_SGMII , 0x0006 ));    
             

         }

    }
     
    /* Configure Auto-detect Medium (0x1c shadow 11110) */
    mask = 0x33f;               /* Auto-detect bit mask */
    data = 0;
    if (automedium) {
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
    if (fiber_preferred) {
        data |= 1 << 1;    /* Fiber selected when both media are active */
        data |= 1 << 2;    /* Fiber selected when no medium is active */
    }

    /* Qulalify fiber link with SYNC from PCS. */
    data |= 1 << 5;
    
    /*
     * Enable internal inversion of LED2/SD input pin.  Used only if
     * the fiber module provides RX_LOS instead of Signal Detect.
     */
    if (fiber_detect < 0) {
         data |= 1 << 8;    /* Fiber signal detect is active low from pin */
    }
    
    SOC_IF_ERROR_RETURN
        (PHY_MODIFY_BCM542XX_AUTO_DETECT_MEDIUMr(unit, pc, data, mask));

     /* Configure extended control register for led mode and jumbo frame support
     */
    mask = PHY_BCM542XX_MII_ECR_EN_LEDT;

    /* Enable LEDs to indicate traffic status */
    data =  PHY_BCM542XX_MII_ECR_EN_LEDT; 

    
    SOC_IF_ERROR_RETURN
        (PHY_MODIFY_BCM542XX_MII_ECRr(unit, pc, data, mask));

    /* Enable extended packet length (4.5k through 25k) */
    SOC_IF_ERROR_RETURN
        (PHY_MODIFY_BCM542XX_MII_AUX_CTRLr(unit, pc, AUX_CTRL_REG_EXT_PKT_LEN, AUX_CTRL_REG_EXT_PKT_LEN));

    return SOC_E_NONE;
}


/*
 * Function:     
 *    phy_bcm542xx_dev_init
 * Purpose:    
 *    Initialize the PHY without reset.
 * Parameters:
 *    phy_dev_addr        - PHY Device Address
 *    automedium          - Automedium select in combo ports
 *    fiber_preferred     - Fiber preferrence over copper
 *    fiber_detect        - Fiber Signal Detect
 * Notes: 
 */
STATIC int
phy_bcm542xx_dev_init(int unit, soc_port_t port, int automedium,
                        int fiber_preferred, int fiber_detect,
                        int fiber_enable, int copper_enable)
{
    int oui = 0, model = 0, rev = 0;
    int rv = SOC_E_NONE;
    phy_ctrl_t    *pc;

    pc = EXT_PHY_SW_STATE(unit, port);
 
    rv = _phy_bcm542xx_get_model_rev(unit, pc, &oui, &model, &rev);
    if (rv != SOC_E_NONE) {
        return SOC_E_FAIL;
    }

    switch(model) {
        case PHY_BCM54280_MODEL:
            rv = phy_bcm54280_init(unit, port, automedium);
            break;
        case (PHY_BCM54282_MODEL|PHY_BCM54285_MODEL):
        case PHY_BCM5428X_MODEL:
            if(rev & 0x8) { /*PHY_BCM54282_MODEL*/
                rv = phy_bcm54282_init(unit, port, automedium);
            
            } else { /*PHY_BCM54285_MODEL*/
                rv = phy_bcm54285_init(unit, port, automedium,
                                       fiber_preferred, 
                                       fiber_detect,
                                       fiber_enable, 
                                       copper_enable);
            }
            break;
         case PHY_BCM54240_MODEL:
            rv = phy_bcm54240_init(unit, port, automedium,
                                   fiber_preferred, 
                                   fiber_detect,
                                   fiber_enable, 
                                   copper_enable);

            break;
        case (PHY_BCM54292_MODEL | PHY_BCM54295_MODEL):
            if(rev & 0x8) { /*PHY_BCM54295_MODEL*/
                rv = phy_bcm54285_init(unit, port, automedium,
                                       fiber_preferred, 
                                       fiber_detect,
                                       fiber_enable, 
                                       copper_enable);
            } else { /*PHY_BCM54292_MODEL*/
                rv = phy_bcm54282_init(unit, port, automedium);
            }
            break;
        case (PHY_BCM54290_MODEL | PHY_BCM54294_MODEL):
            if(rev & 0x8) { /*PHY_BCM54294_MODEL */
                rv = phy_bcm54240_init(unit, port, automedium,
                                       fiber_preferred, 
                                       fiber_detect,
                                       fiber_enable, 
                                       copper_enable);
            } else { /*PHY_BCM54290_MODEL */
                rv = phy_bcm54280_init(unit, port, automedium);
            }   
            break; 
        default :
            return SOC_E_FAIL;
    }

    return rv;
}

/*
 * Function:     
 *    phy_bcm542xx_power_seq_war
 * Purpose:   A failure to link can sometimes 
 *            occur after initial power up.  
 *            This is the workaround. 
 *    
 * Parameters:
 *    phy_id        - PHY Device Address
 * Notes: 
 */
STATIC int
phy_bcm542xx_power_seq_war(int unit, soc_port_t port)
{
    phy_ctrl_t    *pc;
    int index=0, num_ports=8;
    int oui=0, model=0, rev=0;
    
    pc = EXT_PHY_SW_STATE(unit, port);

    _phy_bcm542xx_get_model_rev(unit, pc, &oui, &model, &rev);
 
    /* Recommended: 
       To be done in this fixed sequence for a power sequencing
       bug. All the ports in one-go as it is not advisable to enter and exit 
       test mode for each port.
       Use pc_base->phy_id as the filter to escape this in context of other ports
       since it will be already be done during pc_base's turn. 
        - Do it after power recycle.
     */
    if( pc->phy_id == PHY_BCM542XX_DEV_PHY_ID_BASE(pc)) {
        /* POWER SEQUENCE FIX */
 
        num_ports = 8;   
        if ((model == PHY_BCM54240_MODEL) || (model == PHY_BCM54294_MODEL)) {
            num_ports = 4;
        }
        for (index = 0; index < num_ports; ++index) {
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc) + index;
            
            /* enable dsp clock */
            PHY_MODIFY_BCM542XX_MII_AUX_CTRLr(unit, pc, 
                                           AUX_CTRL_REG_EN_DSP_CLK, 
                                           AUX_CTRL_REG_EN_DSP_CLK);
            phy_bcm542xx_direct_reg_write(unit, pc,
                                           PHY_BCM542XX_LED_2_SEL_REG_OFFSET,
                                           0xB8EE);
            phy_bcm542xx_direct_reg_write(unit, pc,
                                           PHY_BCM542XX_LED_1_SEL_REG_OFFSET,
                                           0xB4EE);
            phy_bcm542xx_direct_reg_write(unit, pc,
                                           PHY_BCM542XX_DSP_TAP17_C0_REG_OFFSET,
                                           0x0009);
            phy_bcm542xx_direct_reg_write(unit, pc,
                                           PHY_BCM542XX_DSP_TAP17_C1_REG_OFFSET,
                                           0x0001);
            phy_bcm542xx_direct_reg_write(unit, pc,
                                           PHY_BCM542XX_DSP_TAP17_C1_REG_OFFSET,
                                           0x0003);
            /* Disable dsp clock */
            PHY_MODIFY_BCM542XX_MII_AUX_CTRLr(unit, pc, 
                                           0, 
                                           AUX_CTRL_REG_EN_DSP_CLK);

        }   
        /***********************************************************************
         * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
         * To access TOP level registers use phy_id of port0. Override this ports 
         */
        pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);

#if 0 
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_TOP_MISC_BOOT_REG_1_OFFSET,
                                       0x8800);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_TOP_MISC_BOOT_REG_1_OFFSET,
                                       0x0000);
        
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_TOP_MISC_BOOT_REG_0_OFFSET,
                                       0x0DA3);

        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_TOP_MISC_BOOT_REG_0_OFFSET,
                                       0x0D23);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_TOP_MISC_BOOT_REG_1_OFFSET,
                                       0x8000);
#else
            WRITE_PHY_REG(unit, pc, 0x17, 0xD19);
            WRITE_PHY_REG(unit, pc, 0x15, 0x8800);
            WRITE_PHY_REG(unit, pc, 0x17, 0xD19);
            WRITE_PHY_REG(unit, pc, 0x15, 0x0000);
            WRITE_PHY_REG(unit, pc, 0x17, 0xD18);
            WRITE_PHY_REG(unit, pc, 0x15, 0x0DA3);
            WRITE_PHY_REG(unit, pc, 0x17, 0xD18);
            WRITE_PHY_REG(unit, pc, 0x15, 0x0D23);
            WRITE_PHY_REG(unit, pc, 0x17, 0xD19);
            WRITE_PHY_REG(unit, pc, 0x15, 0x8000);

#endif

        /***********************************************************************
         * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
         * To access TOP level registers use phy_id of port0. Now restore back 
         */
        pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
    }
    return SOC_E_NONE;
} 


/*
 * Function:     
 *    phy_bcm542xx_reset_phy 
 * Purpose:    
 *    Reset the PHY.
 * Parameters:
 *    phy_id - PHY Device Address of the PHY to reset
 */
int 
phy_bcm542xx_reset_setup(int unit, soc_port_t port, 
                         int automedium,
                         int fiber_preferred, 
                         int fiber_detect,
                         int fiber_enable,
                         int copper_enable)
{
    uint16 ctrl;
    soc_port_t     primary_port;
    int            index;
    phy_ctrl_t    *pc;
    int oui = 0, model = 0, rev = 0;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* Set primary port & offset */
    if (soc_phy_primary_and_offset_get(unit, port, &primary_port, &index) == SOC_E_NONE) {
        if (index & 0x80) {
            PHY_BCM542XX_FLAGS(pc) |= PHY_BCM542XX_PHYA_REV;
        } else {
            PHY_BCM542XX_FLAGS(pc) &= ~PHY_BCM542XX_PHYA_REV;
        } 

        /* Do not change the order */
        index &= ~0x80; /* clear reverse bit (PHYA_REV) */
        PHY_BCM542XX_DEV_PHY_ID_ORIG(pc) = pc->phy_id;
        PHY_BCM542XX_DEV_PHY_SLICE(pc) = index;
        PHY_BCM542XX_DEV_SET_BASE_ADDR(pc); /* phy address of port 0 */

        SOC_IF_ERROR_RETURN
            (phy_bcm542xx_control_set( unit, port, 
                                       SOC_PHY_CONTROL_PORT_PRIMARY, 
                                       primary_port));
        SOC_IF_ERROR_RETURN
            (phy_bcm542xx_control_set( unit, port, 
                                       SOC_PHY_CONTROL_PORT_OFFSET, 
                                       index ));
    } else {
        SOC_DEBUG_PRINT((DK_WARN, "phy_bcm542xx_reset_setup: Config property 'phy_port_primary_and_offset'"
           " not set for u=%d, p=%d\n", unit, port));
        return SOC_E_FAIL;
    }
        
    SOC_IF_ERROR_RETURN(
        PHY_READ_BCM542XX_MII_CTRLr(unit, pc, &ctrl));

    SOC_IF_ERROR_RETURN(
        PHY_WRITE_BCM542XX_MII_CTRLr(unit, pc, (ctrl |
                                         PHY_BCM542XX_MII_CTRL_RESET)));
    
    /* Reset Top level register block*/
    phy_bcm542xx_direct_reg_write(unit, pc,
                                   PHY_BCM542XX_TOP_MISC_GLOBAL_RESET_OFFSET,
                                   0x8400);

    _phy_bcm542xx_get_model_rev(unit, pc, &oui, &model, &rev);

    /* WAR only for B0 rev */
    if(PHY_BCM542XX_REV_BO(model, rev)) {
        phy_bcm542xx_power_seq_war(unit, port);
    }


    /* WAR for C0 and revisions before C0 */
    if(PHY_BCM542XX_REV_C0_OR_OLDER(model, rev)) {
        /* EEE: Suggested defailt settings for these registers */
        /* ENable dsp clock */
        phy_bcm542xx_direct_reg_write(unit, pc,
                                      PHY_BCM542XX_AUX_CTRL_REG_OFFSET,
                                      0x0C00);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                      PHY_BCM542XX_DSP_TAP33_C2_REG_OFFSET,
                                      0x8787);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                      PHY_BCM542XX_DSP_TAP34_C2_REG_OFFSET,
                                      0x017D);
        /* DISable dsp clock */
        phy_bcm542xx_direct_reg_write(unit, pc,
                                      PHY_BCM542XX_AUX_CTRL_REG_OFFSET,
                                      0x0400);

        /* Recommended INIT default settings for registers */
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_AFE_RXCONFIG_0_REG_OFFSET,
                                       0xD771);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_AFE_RXCONFIG_2_REG_OFFSET,
                                       0x0072);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_AFE_HPF_TRIM_REG_OFFSET,
                                       0x0006);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_HALT_AGC_CTRL_REG_OFFSET,
                                       0x0020);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_AFE_AFE_PLLCTRL_4_REG_OFFSET,
                                       0x0500);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_AFE_VDACCTRL_1_REG_OFFSET,
                                       0xC100);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_ANLOG_PWR_CTRL_OVRIDE_REG_OFFSET,
                                       0x1000);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_TDR_OVRIDE_VAL_REG_OFFSET,
                                       0x7C00);
        phy_bcm542xx_direct_reg_write(unit, pc,
                                       PHY_BCM542XX_TDR_OVRIDE_VAL_REG_OFFSET,
                                       0x0000);
    }
    
    /* WAR for C1 rev */
    if(PHY_BCM542XX_REV_C1(model,rev)) {
        phy_bcm542xx_direct_reg_write(unit, pc,
                                   PHY_BCM542XX_AFE_VDACCTRL_2_REG_OFFSET,
                                   0x7);
    }
    
    /* Recommended DSP_TAP10 setting for all the revisions */
    phy_bcm542xx_direct_reg_write(unit, pc,
            PHY_BCM542XX_AUX_CTRL_REG_OFFSET,
            0x0C00);
    phy_bcm542xx_direct_reg_write(unit, pc,
            PHY_BCM542XX_DSP_TAP10_REG_OFFSET,
            0x011B);

    /* Disable SuperIsolate */
    phy_bcm542xx_direct_reg_write(unit, pc,
                                   PHY_BCM542XX_POWER_MII_CTRL_REG_OFFSET,
                                   0x06C2);

    /* Enable current mode LED */
    phy_bcm542xx_direct_reg_write(unit, pc,
                                   PHY_BCM542XX_LED_GPIO_CTRL_STATUS_REG_OFFSET,
                                   0xBC00);
    
    SOC_IF_ERROR_RETURN(
            phy_bcm542xx_dev_init(unit, port, 
                                         automedium, 
                                         fiber_preferred, fiber_detect,
                                         fiber_enable,
                                         copper_enable));

    return SOC_E_NONE;
}


/*
 * Function:     
 *    phy_bcm542xx_init_setup
 * Purpose:    
 *    Initialize the PHY with PHY reset.
 * Parameters:
 *    phy_dev_addr        - PHY Device Address
 *    automedium          - Automedium select in combo ports
 *    fiber_preferred     - Fiber preferrence over copper
 *    fiber_detect        - Fiber Signal Detect
 * Notes: 
 */
STATIC int 
phy_bcm542xx_init_setup(int unit,
                         soc_port_t port,
                         int reset, 
                         int automedium,
                         int fiber_preferred, 
                         int fiber_detect,
                         int fiber_enable,
                         int copper_enable)
{
    phy_ctrl_t    *pc;
    int dev_port;

    pc = EXT_PHY_SW_STATE(unit, port);
   
    if(reset) {
        SOC_IF_ERROR_RETURN(
            phy_bcm542xx_reset_setup(unit, port, automedium, 
                                         fiber_preferred, fiber_detect,
                                         fiber_enable,
                                         copper_enable));
    } else {
        SOC_IF_ERROR_RETURN(
            phy_bcm542xx_dev_init(unit, port, automedium, 
                                        fiber_preferred, fiber_detect,
                                        fiber_enable,
                                        copper_enable));
    }
 
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);
    
    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }
    

    /* Reset EEE to default state i.e disable */
    
    /*
     * Native
    */

    /* Disable LPI feature */ 
    SOC_IF_ERROR_RETURN(
                 PHY_MODIFY_BCM542XX_EEE_803Dr(unit, pc, 0x0000, 0xC000));
    /* Do not advertise EEE capability */   
    SOC_IF_ERROR_RETURN(
                  PHY_MODIFY_BCM542XX_EEE_ADVr(unit, pc, 0x0000, 0x0006));
    /* Reset counters and other settings */   
    SOC_IF_ERROR_RETURN(
            PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc, 0x0000, 0x3fff));

    /*
     * AutogrEEEn
     */

    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
    /* Disable AutogrEEEn and reset other settings */
    phy_bcm542xx_direct_reg_write(unit, pc, 
            PHY_BCM542XX_TOP_MISC_MII_BUFF_CNTL_PHYN(dev_port), 
            0x8000);  
   /* restore original phy_id */ 
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);


#if AUTO_MDIX_WHEN_AN_DIS
    /* Enable Auto-MDIX When autoneg disabled */
    SOC_IF_ERROR_RETURN(
        PHY_MODIFY_BCM542XX_MII_MISC_CTRLr(unit, pc, 
                                   PHY_BCM542XX_MII_FORCE_AUTO_MDIX_MODE, 
                                   PHY_BCM542XX_MII_FORCE_AUTO_MDIX_MODE));
#endif 

#ifdef LC_CM_DISABLE_WORKAROUND
    phy_bcm542xx_direct_reg_write(unit, pc, 0x1F, 0xBC0F); /* Disable LED current mode */
#endif

    return SOC_E_NONE;
}



/*
 * Function:
 *      phy_bcm542xx_init
 * Purpose:
 *      Init function for 542xx PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 * Returns:
 *      SOC_E_NONE
 */

STATIC int
phy_bcm542xx_init(int unit, soc_port_t port)
{
    phy_ctrl_t *pc;
    int        fiber_capable = 0, rv;
    int        fiber_preferred;
    PHY_BCM542XX_DEV_DESC_t *pDesc;

    SOC_DEBUG_PRINT((DK_PHY, "phy_bcm542xxx_init: u=%d p=%d\n",
                     unit, port));

    /* Only SGMII interface to switch MAC is supported */ 
    if (IS_GMII_PORT(unit, port)) {
        return SOC_E_CONFIG; 
    }

    pc = EXT_PHY_SW_STATE(unit, port);
    pc->interface = SOC_PORT_IF_SGMII;

    PHY_FLAGS_SET(unit, port, PHY_FLAGS_EEE_CAPABLE);

    pDesc = (PHY_BCM542XX_DEV_DESC_t *)(pc+1);
    /* clear the memory */
    sal_memset(pDesc, 0, sizeof(PHY_BCM542XX_DEV_DESC_t));

    /* fiber_capable? */
    if(PHY_IS_BCM54240(pc) || PHY_IS_BCM54285(pc) 
          || PHY_IS_BCM54294(pc) || PHY_IS_BCM54295(pc)) {
        fiber_capable = 1;
    }

    /* Change it to copper prefer for default.
     */
    fiber_preferred =
        soc_property_port_get(unit, port, spn_PHY_FIBER_PREF, 0);
    pc->automedium =
        soc_property_port_get(unit, port, spn_PHY_AUTOMEDIUM, 0);
    pc->fiber_detect =
        soc_property_port_get(unit, port, spn_PHY_FIBER_DETECT, -4);

    SOC_DEBUG_PRINT((DK_PHY, "phy_542xx_init: "
            "u=%d p=%d type=bcm542xx%s automedium=%d fiber_pref=%d detect=%d\n",
            unit, port, fiber_capable ? "S" : "",
            pc->automedium, fiber_preferred, pc->fiber_detect));

   

    if (fiber_capable == 0) { /* not fiber capable */
        fiber_preferred  = 0;
        pc->automedium   = 0;
        pc->fiber_detect = 0;
    }

    pc->copper.enable = TRUE;
    pc->copper.preferred = !fiber_preferred;
    pc->copper.autoneg_enable = TRUE;
    ADVERT_ALL_COPPER(&pc->copper.advert_ability);
    pc->copper.force_speed = 1000;
    pc->copper.force_duplex = TRUE;
    pc->copper.master = SOC_PORT_MS_AUTO;
    pc->copper.mdix = SOC_PORT_MDIX_AUTO;

    pc->fiber.enable = fiber_capable;
    pc->fiber.preferred = fiber_preferred;
    pc->fiber.autoneg_enable = fiber_capable;
    ADVERT_ALL_FIBER(&pc->fiber.advert_ability);
    pc->fiber.force_speed = 1000;
    pc->fiber.force_duplex = TRUE;
    pc->fiber.master = SOC_PORT_MS_NONE;
    pc->fiber.mdix = SOC_PORT_MDIX_NORMAL;

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
 
    /* Init PHYS and MACs to defaults */
    rv = phy_bcm542xx_init_setup( unit, port, 1, 
                                pc->automedium,
                                fiber_preferred,
                                pc->fiber_detect,  
                                pc->fiber.enable,
                                pc->copper.enable); 

    if (rv != SOC_E_NONE) {
        return SOC_E_FAIL;
    }
    /* Set LED Modes and Control */
    rv = phy_bcm542xx_set_led_selectors(unit, pc);
    if (rv != SOC_E_NONE) {
        return SOC_E_FAIL;
    }

    SOC_IF_ERROR_RETURN
        (_phy_bcm542xx_medium_config_update(unit, port,
                                        PHY_COPPER_MODE(unit, port) ?
                                        &pc->copper :
                                        &pc->fiber));


    return rv;
}


/*
 * Function:    
 *  phy_bcm542xx_reset
 * Purpose: 
 *  Reset PHY and wait for it to come out of reset.
 * Parameters:
 *  unit - StrataSwitch unit #.
 *  port - StrataSwitch port #. 
 * Returns: 
 *  SOC_E_XXX
 */

int
phy_bcm542xx_reset(int unit, soc_port_t port, void *user_arg)
{
    int rv;
    rv = phy_fe_ge_reset(unit, port, NULL);
    return (SOC_FAILURE(rv));
}

/*
 * Function:
 *      phy_bcm542xx_enable_set
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
phy_bcm542xx_enable_set(int unit, soc_port_t port, int enable)
{
    phy_ctrl_t    *pc;
    uint16 power;
    pc     = EXT_PHY_SW_STATE(unit, port);

    power  = (enable) ? 0 : PHY_BCM542XX_MII_CTRL_PWR_DOWN;

    if(PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (PHY_MODIFY_BCM542XX_MII_CTRLr(unit, pc, power, 
                                                 PHY_BCM542XX_MII_CTRL_PWR_DOWN));
    } else {
        if(PHY_FIBER_MODE(unit, port)) {
            SOC_IF_ERROR_RETURN
                (PHY_MODIFY_BCM542XX_1000X_MII_CTRLr(unit, pc, 
                                                     power, 
                                                     PHY_BCM542XX_MII_CTRL_PWR_DOWN));

        }
    }

    if (pc->automedium || PHY_FIBER_MODE(unit, port)) {
        phy_ctrl_t  *int_pc;

        int_pc = INT_PHY_SW_STATE(unit, port);
        if (NULL != int_pc) {
            SOC_IF_ERROR_RETURN
                (PHY_ENABLE_SET(int_pc->pd, unit, port, enable));
        }
    
        SOC_DEBUG_PRINT((DK_PHY, "phy_bcm542xx_enable_set: "
                "Power %s fiber medium\n", (enable) ? "up" : "down"));
    }

    /* Update software state */
    SOC_IF_ERROR_RETURN(phy_fe_ge_enable_set(unit, port, enable));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_bcm542xx_enable_get
 * Purpose:
 *      Enable or disable the physical interface for a 542xx device.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      enable - (OUT) Boolean, true = enable PHY, false = disable.
 * Returns:
 *      SOC_E_XXX
 */

STATIC int
phy_bcm542xx_enable_get(int unit, soc_port_t port, int *enable)
{
    return phy_fe_ge_enable_get(unit, port, enable);
}


/*
 * Function:
 *      _phy_bcm542xx_medium_change
 * Purpose:
 *      Change medium when media change detected or forced
 * Parameters:
 *      unit         - StrataSwitch unit #.
 *      port         - StrataSwitch port #.
 *      force_update - Force update
 *      force_medium - Force update medium
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
_phy_bcm542xx_medium_change(int unit, soc_port_t port, int force_update,
                         soc_port_medium_t force_medium)
{
    phy_ctrl_t    *pc;
    int            medium;

    pc    = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (_phy_bcm542xx_medium_check(unit, port, &medium));

    if (medium == SOC_PORT_MEDIUM_COPPER) {
        if (!PHY_COPPER_MODE(unit, port) || force_update) { /* Was fiber */ 
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_COPPER);
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_FIBER);
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_100FX);
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_MEDIUM_CHANGE);
            SOC_IF_ERROR_RETURN
                (_phy_bcm542xx_no_reset_setup(unit, port));

            /* Do Not power up the interface if medium is disabled */
            if (pc->copper.enable) {
                SOC_IF_ERROR_RETURN
                    (_phy_bcm542xx_medium_config_update(unit, port, &pc->copper));
            }
            SOC_DEBUG_PRINT((DK_PHY,
                         "_phy_bcm542xx_link_auto_detect: u=%d p=%d [F->C]\n",
                          unit, port));
        }
    } else {        /* Fiber */
        if (PHY_COPPER_MODE(unit, port) || force_update) { /* Was copper */
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_COPPER);
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_FIBER);
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_MEDIUM_CHANGE);
            SOC_IF_ERROR_RETURN
                (_phy_bcm542xx_no_reset_setup(unit, port));

            if (pc->fiber.enable) {
                SOC_IF_ERROR_RETURN
                    (_phy_bcm542xx_medium_config_update(unit, port, &pc->fiber));
            }
            SOC_DEBUG_PRINT((DK_PHY,
                          "_phy_bcm542xx_link_auto_detect: u=%d p=%d [C->F]\n",
                          unit, port));
        }
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_bcm542xx_link_get
 * Purpose:
 *      Determine the current link up/down status for a 542xx device.
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
phy_bcm542xx_link_get(int unit, soc_port_t port, int *link)
{
    phy_ctrl_t    *pc;
    uint16         power;
    uint16  mii_stat, temp16;

    pc    = EXT_PHY_SW_STATE(unit, port);
    *link = FALSE;      /* Default return */

    if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE)) {
        return SOC_E_NONE;
    }

    if (!pc->fiber.enable && !pc->copper.enable) {
        return SOC_E_NONE;
    }

    if (PHY_IS_BCM54240(pc) || PHY_IS_BCM54285(pc)) {
        SOC_IF_ERROR_RETURN
            (phy_bcm542xx_direct_reg_read(unit, pc, PHY_BCM542XX_OPER_MODE_STATUS_REG_OFFSET, &temp16));

        if ((!(temp16 & (1U << 5)))  && ((temp16 & 0x1f) == 0xb)) { /* no sync and in SGMII slave */

            /* Disable auto-detection between SGMII-slave and 1000BASE-X and set mode to 1000BASE-X */
            SOC_IF_ERROR_RETURN
                   (PHY_MODIFY_BCM542XX_SGMII_SLAVEr(unit, pc, 0, 0x3));

            /* Now enable auto-detection between SGMII-slave and 1000BASE-X */
            SOC_IF_ERROR_RETURN
                   (PHY_MODIFY_BCM542XX_SGMII_SLAVEr(unit, pc, 1, 1));
        }
    }

    PHY_FLAGS_CLR(unit, port, PHY_FLAGS_MEDIUM_CHANGE);
    if (pc->automedium) {
        /* Check for medium change and update HW/SW accordingly. */
        SOC_IF_ERROR_RETURN
            (_phy_bcm542xx_medium_change(unit, port, FALSE, 0));
    }

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(phy_fe_ge_link_get(unit, port, link));
    } else {
        SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_1000X_MII_STATr(unit, pc, &mii_stat));
        if(!(mii_stat & PHY_BCM542XX_MII_STAT_LA)){
            sal_udelay(200);
            SOC_IF_ERROR_RETURN
                (PHY_READ_BCM542XX_1000X_MII_STATr(unit, pc, &mii_stat));
        }
        *link = (mii_stat & PHY_BCM542XX_MII_STAT_LA) ? TRUE : FALSE;
    }

    /* If preferred medium is up, disable the other medium 
     * to prevent false link. */
    if (pc->automedium) {
        if (pc->copper.preferred) {
            if (pc->fiber.enable) {
                /* Power down Serdes if Copper mode is up */
                power = (*link && PHY_COPPER_MODE(unit, port)) ? PHY_BCM542XX_MII_CTRL_PWR_DOWN : 0;
            } else {
                power = PHY_BCM542XX_MII_CTRL_PWR_DOWN;

            }
            SOC_IF_ERROR_RETURN
                 (PHY_MODIFY_BCM542XX_1000X_MII_CTRLr(unit, pc, 
                                                      power, 
                                                      PHY_BCM542XX_MII_CTRL_PWR_DOWN));

        } else {  /* pc->fiber.preferred */
            if (pc->copper.enable) {
                power = (*link && PHY_FIBER_MODE(unit, port)) ? PHY_BCM542XX_MII_CTRL_PWR_DOWN : 0;
            } else {
                power = PHY_BCM542XX_MII_CTRL_PWR_DOWN;
            }
        
            SOC_IF_ERROR_RETURN
                 (PHY_MODIFY_BCM542XX_MII_CTRLr(unit, pc,
                                                power,
                                                PHY_BCM542XX_MII_CTRL_PWR_DOWN));
        
        }
    }

    SOC_DEBUG_PRINT((DK_PHY | DK_VERBOSE,
                     "phy_bcm542xx_link_get: u=%d p=%d mode=%s%s link=%d\n",
                      unit, port,
                      PHY_COPPER_MODE(unit, port) ? "C" : "F",
                      PHY_FIBER_100FX_MODE(unit, port)? "(100FX)" : "",
                      *link));

    return SOC_E_NONE;
}


/*
 * Function:
 *      phy_bcm542xx_duplex_set
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
phy_bcm542xx_duplex_set(int unit, soc_port_t port, int duplex)
{
    int                          rv;
    phy_ctrl_t                   *pc;
    uint16 data = 0;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    if(PHY_COPPER_MODE(unit, port)) {
        if(duplex) {
            SOC_IF_ERROR_RETURN(
                PHY_MODIFY_BCM542XX_MII_CTRLr(unit, pc, 
                                            PHY_BCM542XX_MII_CTRL_FD, 
                                            PHY_BCM542XX_MII_CTRL_FD));
        } else {
            SOC_IF_ERROR_RETURN(
                PHY_MODIFY_BCM542XX_MII_CTRLr(unit, pc, 
                                            0, PHY_BCM542XX_MII_CTRL_FD));

        }
        pc->copper.force_duplex = duplex;

    } else if(PHY_FIBER_MODE(unit, port)) {
        if (PHY_FIBER_100FX_MODE(unit, port)) {

            if(duplex == SOC_PORT_DUPLEX_FULL) {
                data = PHY_BCM542XX_100FX_CTRL_REG_FD_FX_SERDES;
            }
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                    PHY_BCM542XX_100FX_CTRL_REG_OFFSET, 
                    data, PHY_BCM542XX_100FX_CTRL_REG_FD_FX_SERDES); 

            pc->fiber.force_duplex = duplex;
        } else {
            if(!duplex) {
                return SOC_E_UNAVAIL;
            }
        }

    }

    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_bcm542xx_duplex_set: u=%d p=%d d=%d rv=%d\n",
                     unit, port, duplex, rv));
    return (SOC_FAILURE(rv));
}

/*
 * Function:
 *      phy_bcm542xx_duplex_get
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
phy_bcm542xx_duplex_get(int unit, soc_port_t port, int *duplex)
{

    int rv = SOC_E_NONE;
    phy_ctrl_t                   *pc;
    uint16 opt_mode;
    uint16   mii_ctrl, mii_stat;

    pc = EXT_PHY_SW_STATE(unit, port);


    if (PHY_FIBER_MODE(unit, port)) {
       *duplex = TRUE; 
    }

    if (PHY_COPPER_MODE(unit, port)) {

        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_MII_CTRLr(unit, pc, &mii_ctrl));
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_MII_STATr(unit, pc, &mii_stat));


        if (mii_ctrl & PHY_BCM542XX_MII_CTRL_AN_EN) {     /* Auto-negotiation enabled */
            if (!(mii_stat & PHY_BCM542XX_MII_STAT_AN_DONE)) { /* Auto-neg NOT complete */
                *duplex = FALSE;
            } else {
                uint16 misc_ctrl;
                SOC_IF_ERROR_RETURN(
                    PHY_READ_BCM542XX_MII_MISC_CTRLr(unit, pc, &misc_ctrl));
                /* First check for Ethernet@Wirespeed */
                if (misc_ctrl & (1U << 4)) {   /* Ethernet@Wirespeed enabled */
                    rv = _phy_bcm542xx_auto_negotiate_ew(unit, pc, NULL, duplex);
                } else {
                    rv =_phy_bcm542xx_auto_negotiate_gcd(unit, pc, NULL, duplex);
                }
            }
        } else {                /* Auto-negotiation disabled */
            *duplex = (mii_ctrl & PHY_BCM542XX_MII_CTRL_FD) ? TRUE : FALSE;
        }

    } else {    /* Fiber */
        if (PHY_FIBER_100FX_MODE(unit, port)) {
            SOC_IF_ERROR_RETURN
                (PHY_READ_BCM542XX_EXP_OPT_MODE_STATr(unit, pc, &opt_mode));
            if(opt_mode & PHY_BCM542XX_OPER_MODE_STATUS_SERDES_DUPLEX) {
                *duplex = TRUE;
            } else {
                *duplex = FALSE;
            }
        } else { /* 1000X */
            *duplex = TRUE;
        }
    }
    return rv;
}

/*
 * Function:
 *      phy_bcm542xx_speed_set
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
phy_bcm542xx_speed_set(int unit, soc_port_t port, int speed)
{
    int            rv; 
    phy_ctrl_t    *pc;
    uint16 mii_ctrl = 0;
    uint16 serdes_100fx_ctrl = 0; 
    uint16 serdex_100fx_ctrl_mask = 0;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    if(PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_MII_CTRLr(unit, pc, &mii_ctrl));

    } else if(PHY_FIBER_MODE(unit, port)) {
        if (speed == 10) {
            return SOC_E_UNAVAIL;
        }
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_1000X_MII_CTRLr(unit, pc, &mii_ctrl));

    }
    
    mii_ctrl &= ~(PHY_BCM542XX_MII_CTRL_SS_LSB | PHY_BCM542XX_MII_CTRL_SS_MSB);
    switch(speed) {
    case 10:
        mii_ctrl |= PHY_BCM542XX_MII_CTRL_SS_10;
        break;
    case 100:
        mii_ctrl |= PHY_BCM542XX_MII_CTRL_SS_100;
        serdes_100fx_ctrl = PHY_BCM542XX_100FX_CTRL_REG_FX_SERDES_EN;
        serdex_100fx_ctrl_mask = PHY_BCM542XX_100FX_CTRL_REG_FX_SERDES_EN 
                                | PHY_BCM542XX_100FX_CTRL_REG_FD_FX_SERDES;
        if(pc->fiber.force_duplex) {
            serdes_100fx_ctrl |= PHY_BCM542XX_100FX_CTRL_REG_FD_FX_SERDES;
        }
        break;
    case 1000:  
        mii_ctrl |= PHY_BCM542XX_MII_CTRL_SS_1000;
        serdes_100fx_ctrl = 0x0;
        serdex_100fx_ctrl_mask = PHY_BCM542XX_100FX_CTRL_REG_FX_SERDES_EN;
        break;
    default:
        return SOC_E_CONFIG;
    }

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(
                PHY_WRITE_BCM542XX_MII_CTRLr(unit, pc, mii_ctrl));
        pc->copper.force_speed = speed;
    }else {
        /* Select 1000x/100fx and duplexity in case of 100fx*/ 
        phy_bcm542xx_direct_reg_modify(unit, pc, 
                PHY_BCM542XX_100FX_CTRL_REG_OFFSET, 
                serdes_100fx_ctrl, serdex_100fx_ctrl_mask);
       
       /* Disable autoneg for 100fx speed */ 
        if (speed == 100) {
            pc->fiber.autoneg_enable = FALSE;
        }
        if (pc->fiber.autoneg_enable) {
            mii_ctrl |= MII_CTRL_AE | MII_CTRL_RAN | MII_CTRL_FD;
        } else {
            mii_ctrl |= MII_CTRL_FD;
        }
        if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE)) { 
            mii_ctrl |= PHY_BCM542XX_MII_CTRL_PWR_DOWN; 
        } 
        SOC_IF_ERROR_RETURN(
            PHY_WRITE_BCM542XX_1000X_MII_CTRLr(unit, pc, mii_ctrl));
      
        if (speed == 100) {
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_100FX);
        }
        /* Enforce duplexity to full for 1000X */
        if (speed == 1000) {
            pc->fiber.force_duplex = TRUE;
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_100FX);
        }

        pc->fiber.force_speed = speed;

    }

    SOC_DEBUG_PRINT((DK_PHY,
                     "phy_bcm542xx_speed_set: u=%d p=%d s=%d fiber=%d rv=%d\n",
                     unit, port, speed, PHY_FIBER_MODE(unit, port), rv));

    return SOC_FAILURE(rv);
}

/*
 * Function:
 *      phy_bcm542xx_speed_get
 * Purpose:
 *      Get the current operating speed for a bcm542xx device.
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
phy_bcm542xx_speed_get(int unit, soc_port_t port, int *speed)
{
    int rv = SOC_E_NONE;
    phy_ctrl_t    *pc;
    uint16 mii_ctrl, mii_aux_stat, temp16;

    pc = EXT_PHY_SW_STATE(unit, port);

    if(PHY_COPPER_MODE(unit, port)) {

        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_MII_CTRLr(unit, pc, &mii_ctrl));

        if (mii_ctrl & PHY_BCM542XX_MII_CTRL_AN_EN) {   /* Auto-negotiation enabled */
                SOC_IF_ERROR_RETURN(
                    PHY_READ_BCM542XX_MII_AUX_STATUSr(unit, pc, &mii_aux_stat));

            if (!(mii_aux_stat & PHY_BCM542XX_MII_ASSR_ANC)) {
                /* Auto-neg NOT complete */
                *speed = 0;
            } else {

                switch(mii_aux_stat & PHY_BCM542XX_MII_ASSR_AUTONEG_HCD_MASK)
                {
                    case PHY_BCM542XX_MII_ASSR_HCD_FD_1000:
                    case PHY_BCM542XX_MII_ASSR_HCD_HD_1000:
                        *speed = 1000;
                        break;
                    case PHY_BCM542XX_MII_ASSR_HCD_FD_100:
                    case PHY_BCM542XX_MII_ASSR_HCD_T4_100:    
                    case PHY_BCM542XX_MII_ASSR_HCD_HD_100:   
                        *speed = 100;
                        break; 
                    case PHY_BCM542XX_MII_ASSR_HCD_FD_10:
                    case PHY_BCM542XX_MII_ASSR_HCD_HD_10:
                        *speed = 10;
                        break;
                    default:
                        *speed = 0;    
                }


            }   
        } else {    /* Auto-negotiation disabled */
        /*
         * Pick up the values we force in CTRL register.
         */
            switch(PHY_BCM542XX_MII_CTRL_SS(mii_ctrl)) {
                case PHY_BCM542XX_MII_CTRL_SS_10:
                *speed = 10;
            break;
                case PHY_BCM542XX_MII_CTRL_SS_100:
                *speed = 100;
            break;
                case PHY_BCM542XX_MII_CTRL_SS_1000:
                *speed = 1000;
                break;
            default:   /* Pass error back */
                return SOC_E_UNAVAIL;
            }
        }

    } else if(PHY_FIBER_MODE(unit, port)) {

        phy_bcm542xx_direct_reg_read(unit, pc, PHY_BCM542XX_OPER_MODE_STATUS_REG_OFFSET, &temp16);
        switch(temp16 & PHY_BCM542XX_OPER_MODE_STATUS_REG_SERDES_SPEED)  {
            case PHY_BCM542XX_OPER_MODE_STATUS_SERDES_SPEED_10:
                *speed = 10;
                break; 
            case PHY_BCM542XX_OPER_MODE_STATUS_SERDES_SPEED_100:
                *speed = 100; 
                break;
            case PHY_BCM542XX_OPER_MODE_STATUS_SERDES_SPEED_1000:
                *speed = 1000; 
                break;
            default:
                return SOC_E_UNAVAIL;  
        }

    }

    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_bcm542xx_speed_get: u=%d p=%d sp=%d\n",
                     unit, port, *speed));

    return SOC_FAILURE(rv);
}

/*
 * Function:
 *      phy_bcm542xx_master_set
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
phy_bcm542xx_master_set(int unit, soc_port_t port, int master)
{
    int                  rv;
    phy_ctrl_t    *pc;
    uint16 mii_gb_ctrl;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    if(PHY_COPPER_MODE(unit, port)) {
          
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_MII_GB_CTRLr(unit, pc, &mii_gb_ctrl));
        switch (master) {
        case SOC_PORT_MS_SLAVE:
            mii_gb_ctrl |= PHY_BCM542XX_MII_GB_CTRL_MS_MAN;
            mii_gb_ctrl &= ~PHY_BCM542XX_MII_GB_CTRL_MS;
        break;
        case SOC_PORT_MS_MASTER:
            mii_gb_ctrl |= PHY_BCM542XX_MII_GB_CTRL_MS_MAN;
            mii_gb_ctrl |= PHY_BCM542XX_MII_GB_CTRL_MS;
        break;
        case SOC_PORT_MS_AUTO:
            mii_gb_ctrl &= ~PHY_BCM542XX_MII_GB_CTRL_MS_MAN;
            break;
        case SOC_PORT_MS_NONE:
            break;

        default:
            return SOC_E_CONFIG;
        }

        SOC_IF_ERROR_RETURN(
            PHY_WRITE_BCM542XX_MII_GB_CTRLr(unit, pc, mii_gb_ctrl));
        pc->copper.master = master;


    } 

    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_bcm542xx_master_set: u=%d p=%d master=%d fiber=%d rv=%d\n",
                    unit, port, master, PHY_FIBER_MODE(unit, port), rv));

    return SOC_FAILURE(rv);
}

/*
 * Function:
 *      phy_bcm542xx_master_get
 * Purpose:
 *      Get the current master mode for a bcm542xx device.
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
phy_bcm542xx_master_get(int unit, soc_port_t port, int *master)
{
    int        rv;
    phy_ctrl_t *pc;
    uint16 mii_gb_ctrl;
    
    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    if(PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_MII_GB_CTRLr(unit, pc, &mii_gb_ctrl));
        if (!(mii_gb_ctrl & PHY_BCM542XX_MII_GB_CTRL_MS_MAN)) {
            *master = SOC_PORT_MS_AUTO;
        } else if (mii_gb_ctrl & PHY_BCM542XX_MII_GB_CTRL_MS) {
            *master = SOC_PORT_MS_MASTER;
        } else {
            *master = SOC_PORT_MS_SLAVE;
        }
    } else {
        if(PHY_FIBER_MODE(unit, port)) {
            *master = SOC_PORT_MS_NONE;
            return SOC_E_NONE;
        }
    }

    return SOC_FAILURE(rv);
}

/*
 * Function:
 *      phy_bcm542xx_autoneg_set
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
phy_bcm542xx_autoneg_set(int unit, soc_port_t port, int autoneg)
{
    int                 rv;
    phy_ctrl_t   *pc;
    uint16 mii_ctrl;
    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    if(PHY_COPPER_MODE(unit, port)) {

        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_MII_CTRLr(unit, pc, &mii_ctrl));

        if (autoneg) {
            mii_ctrl |= PHY_BCM542XX_MII_CTRL_AN_EN | PHY_BCM542XX_MII_CTRL_RST_AN;
        } else {
            mii_ctrl &= ~(PHY_BCM542XX_MII_CTRL_AN_EN | PHY_BCM542XX_MII_CTRL_RST_AN);
        }
        SOC_IF_ERROR_RETURN(
                PHY_WRITE_BCM542XX_MII_CTRLr(unit, pc, mii_ctrl));
        
        pc->copper.autoneg_enable = autoneg ? TRUE : FALSE;

    } else if(PHY_FIBER_MODE(unit, port)) {

        if(autoneg) {
           /* When enabling autoneg, set the default speed to 1000Mbps first.
             * PHY will not enable autoneg if the PHY is in 100FX mode.
             */
            SOC_IF_ERROR_RETURN
               (phy_bcm542xx_speed_set(unit, port, 1000));
        }

        /* Disable/Enable auto-detection between SGMII-slave and 1000BASE-X */
        SOC_IF_ERROR_RETURN(
            PHY_MODIFY_BCM542XX_SGMII_SLAVEr(unit, pc, autoneg?1:0, 1)); 
        
        SOC_IF_ERROR_RETURN(
            phy_bcm542xx_ability_advert_set(unit, port, &pc->fiber.advert_ability));
 

        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_1000X_MII_CTRLr(unit, pc, &mii_ctrl));

        if (autoneg) {
            mii_ctrl |= PHY_BCM542XX_MII_CTRL_AN_EN | PHY_BCM542XX_MII_CTRL_RST_AN;
        } else {
            mii_ctrl &= ~(PHY_BCM542XX_MII_CTRL_AN_EN | PHY_BCM542XX_MII_CTRL_RST_AN);
        }
        SOC_IF_ERROR_RETURN(
                PHY_WRITE_BCM542XX_1000X_MII_CTRLr(unit, pc, mii_ctrl));
        
        pc->fiber.autoneg_enable = autoneg ? TRUE : FALSE;

    }

    SOC_DEBUG_PRINT((DK_PHY,
                     "phy_bcm542xx_autoneg_set: u=%d p=%d autoneg=%d rv=%d\n",
                     unit, port, autoneg, rv));
    return SOC_FAILURE(rv);
}

/*
 * Function:
 *      phy_bcm542xx_autoneg_get
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
phy_bcm542xx_autoneg_get(int unit, soc_port_t port,
                     int *autoneg, int *autoneg_done)
{
    int           rv;
    phy_ctrl_t   *pc;
    uint16 mii_ctrl, mii_stat;

    pc = EXT_PHY_SW_STATE(unit, port);

    rv            = SOC_E_NONE;
    *autoneg      = FALSE;
    *autoneg_done = FALSE;

    if(PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_MII_CTRLr(unit, pc, &mii_ctrl));
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_MII_STATr(unit, pc, &mii_stat));

        *autoneg= (mii_ctrl & PHY_BCM542XX_MII_CTRL_AN_EN) ? 1 : 0;
        *autoneg_done= (mii_stat & PHY_BCM542XX_MII_STAT_AN_DONE) ? 1 : 0;

    } else if(PHY_FIBER_MODE(unit, port)) {
        /* autoneg is not enabled in 100FX mode */
        if (PHY_FIBER_100FX_MODE(unit, port)) {
            *autoneg = FALSE;
            *autoneg_done = TRUE;
            return rv;
        }
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_1000X_MII_CTRLr(unit, pc, &mii_ctrl));
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_1000X_MII_STATr(unit, pc, &mii_stat));

        *autoneg= (mii_ctrl & PHY_BCM542XX_MII_CTRL_AN_EN) ? 1 : 0;
        *autoneg_done= (mii_stat & PHY_BCM542XX_MII_STAT_AN_DONE) ? 1 : 0;

    }

    return SOC_FAILURE(rv);
}

/*
 * Function:
 *      phy_bcm542xx_ability_advert_set
 * Purpose:
 *      Set the current advertisement for auto-negotiation.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      mode - Port mode mask indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The advertisement is set only for the ACTIVE medium.
 *      No synchronization performed at this level.
 */
STATIC int
phy_bcm542xx_ability_advert_set(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    int rv;
    phy_ctrl_t    *pc;
    uint16 mii_adv, mii_gb_ctrl;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;


    if(PHY_COPPER_MODE(unit, port)) {
        mii_adv = PHY_BCM542XX_MII_ANA_ASF_802_3;
        SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_MII_GB_CTRLr(unit, pc, &mii_gb_ctrl));

        mii_gb_ctrl &= ~(PHY_BCM542XX_MII_GB_CTRL_ADV_1000HD | 
                         PHY_BCM542XX_MII_GB_CTRL_ADV_1000FD);
        mii_gb_ctrl |= PHY_BCM542XX_MII_GB_CTRL_PT; /* repeater / switch port */

        if ((ability->speed_half_duplex) & SOC_PA_SPEED_10MB) {
            mii_adv |= PHY_BCM542XX_MII_ANA_HD_10;
        }
        if ((ability->speed_full_duplex) & SOC_PA_SPEED_10MB) {
            mii_adv |= PHY_BCM542XX_MII_ANA_FD_10;
        }
        if ((ability->speed_half_duplex) & SOC_PA_SPEED_100MB) {
            mii_adv |= PHY_BCM542XX_MII_ANA_HD_100;
        }
        if ((ability->speed_full_duplex) & SOC_PA_SPEED_100MB) {
            mii_adv |= PHY_BCM542XX_MII_ANA_FD_100;
        }
        if ((ability->speed_half_duplex) & SOC_PA_SPEED_1000MB) {
            mii_gb_ctrl |= PHY_BCM542XX_MII_GB_CTRL_ADV_1000HD;
        }
        if ((ability->speed_full_duplex) & SOC_PA_SPEED_1000MB){
            mii_gb_ctrl |= PHY_BCM542XX_MII_GB_CTRL_ADV_1000FD;
        }
        
        if (((ability->pause) & SOC_PA_PAUSE) == SOC_PA_PAUSE) {
            /* Advertise symmetric pause */
            mii_adv |= PHY_BCM542XX_MII_ANA_PAUSE;
        } else {
            if ((ability->pause) & SOC_PA_PAUSE_TX) {
                mii_adv |= PHY_BCM542XX_MII_ANA_ASYM_PAUSE;
            } else { 
                if ((ability->pause) & SOC_PA_PAUSE_RX) {
                    mii_adv |= PHY_BCM542XX_MII_ANA_ASYM_PAUSE;
                    mii_adv |= PHY_BCM542XX_MII_ANA_PAUSE;
                }
            }
        }

        SOC_IF_ERROR_RETURN
            (PHY_WRITE_BCM542XX_MII_ANAr(unit, pc, mii_adv));
        SOC_IF_ERROR_RETURN
            (PHY_WRITE_BCM542XX_MII_GB_CTRLr(unit, pc, mii_gb_ctrl));
    
        
        
    } else if(PHY_FIBER_MODE(unit, port)) {
         mii_adv = PHY_BCM542XX_MII_ANA_C37_FD;  /* Always advertise 1000X full duplex */
        switch (ability->pause & (SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX)) {
        case SOC_PA_PAUSE_TX:
            mii_adv |= PHY_BCM542XX_MII_ANA_C37_ASYM_PAUSE;
            break;
        case SOC_PA_PAUSE_RX:
            mii_adv |= (PHY_BCM542XX_MII_ANA_C37_ASYM_PAUSE | PHY_BCM542XX_MII_ANA_C37_PAUSE);
            break;
        case SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX:
            mii_adv |= PHY_BCM542XX_MII_ANA_C37_PAUSE;
            break;
        }
        /* Write to SerDes Auto-Neg Advertisement Reg */
        rv = PHY_WRITE_BCM542XX_1000X_MII_ANAr(unit, pc, mii_adv);

    }
    if(SOC_SUCCESS(rv)) {
        if(PHY_COPPER_MODE(unit, port)) {
            pc->copper.advert_ability = *ability;
        } else {
            if(PHY_FIBER_MODE(unit, port)) {
                pc->fiber.advert_ability = *ability;
        } else {
            }
        }
    }

    SOC_DEBUG_PRINT((DK_PHY,
                  "phy_bcm542xx_adv_local_set: u=%d p=%d ability_hd_speed=0x%x, ability_fd_speed=0x%x, ability_pause=0x%x,  rv=%d\n",
                   unit, port, ability->speed_half_duplex, ability->speed_full_duplex, ability->pause, rv));
    return (SOC_FAILURE(rv));
}

/*
 * Function:
 *      phy_bcm542xx_ability_advert_get
 * Purpose:
 *      Get the current advertisement for auto-negotiation.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      mode - (OUT) Port mode mask indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The advertisement is retrieved for the ACTIVE medium.
 *      No synchronization performed at this level.
 */

STATIC int
phy_bcm542xx_ability_advert_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{

    int rv;
    phy_ctrl_t   *pc;
    uint16 mii_ana, mii_gb_ctrl, mii_adv;

    if (NULL == ability) {
        return SOC_E_PARAM;
    }
    sal_memset(ability, 0,  sizeof(soc_port_ability_t)); /* zero initialize */
    
    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    if(PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_MII_ANAr(unit, pc, &mii_ana));
        SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_MII_GB_CTRLr(unit, pc, &mii_gb_ctrl));

        if (mii_ana & PHY_BCM542XX_MII_ANA_HD_10) {
            ability->speed_half_duplex |= SOC_PA_SPEED_10MB;
        }
        if (mii_ana & PHY_BCM542XX_MII_ANA_FD_10) {
            ability->speed_full_duplex |= SOC_PA_SPEED_10MB;
        }
        if (mii_ana & PHY_BCM542XX_MII_ANA_HD_100) {
            ability->speed_half_duplex |= SOC_PA_SPEED_100MB;
        }
        if (mii_ana & PHY_BCM542XX_MII_ANA_FD_100) {
            ability->speed_full_duplex |= SOC_PA_SPEED_100MB;
        }

        if (mii_ana & PHY_BCM542XX_MII_ANA_ASYM_PAUSE) {
            if (mii_ana & PHY_BCM542XX_MII_ANA_PAUSE) {
            ability->pause |= SOC_PA_PAUSE_RX;
        } else {
            ability->pause |= SOC_PA_PAUSE_TX;
        }
        } else if (mii_ana & PHY_BCM542XX_MII_ANA_PAUSE) {
            ability->pause |= SOC_PA_PAUSE;
        }

        /* GE Specific values */

        if (mii_gb_ctrl & PHY_BCM542XX_MII_GB_CTRL_ADV_1000FD) {
            ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
        }
        if (mii_gb_ctrl & PHY_BCM542XX_MII_GB_CTRL_ADV_1000HD) {
            ability->speed_half_duplex |= SOC_PA_SPEED_1000MB;
        }

    } else if(PHY_FIBER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_1000X_MII_ANAr(unit, pc, &mii_adv));

        if (mii_adv & PHY_BCM542XX_MII_ANA_C37_FD) {
            ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
        }
        if (mii_adv & PHY_BCM542XX_MII_ANA_C37_HD) {
            ability->speed_half_duplex |= SOC_PA_SPEED_1000MB;
        }
        switch (mii_adv & 
                (PHY_BCM542XX_MII_ANA_C37_PAUSE | PHY_BCM542XX_MII_ANA_C37_ASYM_PAUSE)) {
        case PHY_BCM542XX_MII_ANA_C37_PAUSE:
            ability->pause = SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX;
            break;
        case PHY_BCM542XX_MII_ANA_C37_ASYM_PAUSE:
            ability->pause = SOC_PA_PAUSE_TX;
            break;
        case (PHY_BCM542XX_MII_ANA_C37_PAUSE |   PHY_BCM542XX_MII_ANA_C37_ASYM_PAUSE):
            ability->pause = SOC_PA_PAUSE_RX;
            break;

    
        }
    }
    SOC_DEBUG_PRINT((DK_PHY,
                  "phy_bcm542xx_adv_local_get: u=%d p=%d ability_hd_speed=0x%x, ability_fd_speed=0x%x, ability_pause=0x%x,  rv=%d\n",
                   unit, port, ability->speed_half_duplex, ability->speed_full_duplex, ability->pause, rv));

    
    return  (SOC_FAILURE(rv));

}

/*
 * Function:
 *      phy_bcm542xx0_ability_remote_get
 * Purpose:
 *      Get partners current advertisement for auto-negotiation.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      mode - (OUT) Port mode mask indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The remote advertisement is retrieved for the ACTIVE medium.
 *      No synchronization performed at this level. If Autonegotiation is
 *      disabled or in progress, this routine will return an error.
 */

STATIC int
phy_bcm542xx_ability_remote_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    int rv;
    phy_ctrl_t *pc;
    uint16 mii_ctrl, mii_stat, mii_anp, mii_gb_stat, eee_resolution;

    if (NULL == ability) {
        return SOC_E_PARAM;
    }

    sal_memset(ability, 0, sizeof(soc_port_ability_t)); /*zero init*/

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    if(PHY_COPPER_MODE(unit, port)) {
    
        SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_MII_CTRLr(unit, pc, &mii_ctrl));
        SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_MII_STATr(unit, pc, &mii_stat));

        if (!(mii_ctrl & PHY_BCM542XX_MII_CTRL_AN_EN)) {
            return SOC_E_DISABLED;
        } else  {
            if (!(mii_stat & PHY_BCM542XX_MII_STAT_AN_DONE)) {
            return SOC_E_NONE;
            }
        }

        SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_MII_ANPr(unit, pc, &mii_anp));
        SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_MII_GB_STATr(unit, pc, &mii_gb_stat));
        
        if (mii_anp & PHY_BCM542XX_MII_ANA_HD_10) {
            ability->speed_half_duplex |= SOC_PA_SPEED_10MB;
        }
        if (mii_anp & PHY_BCM542XX_MII_ANA_FD_10) {
            ability->speed_full_duplex |= SOC_PA_SPEED_10MB;
        }
        if (mii_anp & PHY_BCM542XX_MII_ANA_HD_100) {
            ability->speed_half_duplex |= SOC_PA_SPEED_100MB;
        }
        if (mii_anp & PHY_BCM542XX_MII_ANA_FD_100) {
            ability->speed_full_duplex |= SOC_PA_SPEED_100MB;
        }
        if (mii_gb_stat & PHY_BCM542XX_MII_GB_STAT_LP_1000HD) {
            ability->speed_half_duplex |= SOC_PA_SPEED_1000MB;
        }
        if (mii_gb_stat & PHY_BCM542XX_MII_GB_STAT_LP_1000FD) {
            ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
        }

        if (mii_anp & PHY_BCM542XX_MII_ANA_ASYM_PAUSE) {
            if (mii_anp & PHY_BCM542XX_MII_ANA_PAUSE) {
                ability->pause |= SOC_PA_PAUSE_RX;
            } else {
                ability->pause |= SOC_PA_PAUSE_TX;
            }
        } else {
            if (mii_anp & PHY_BCM542XX_MII_ANA_PAUSE) {
                ability->pause |= SOC_PA_PAUSE;
            }
        }

        /* EEE settings */
        if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_EEE_RESOLUTION_STATr(unit, pc, &eee_resolution));
            if (eee_resolution & 0x04) {
                ability->eee |= SOC_PA_EEE_1GB_BASET;
            }
            if (eee_resolution & 0x02) {
                ability->eee |= SOC_PA_EEE_100MB_BASETX;
            }
        }



    } else if(PHY_FIBER_MODE(unit, port)) {
    
        SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_1000X_MII_CTRLr(unit, pc, &mii_ctrl));
        if (!(mii_ctrl & PHY_BCM542XX_1000BASE_X_CTRL_REG_AE)) {
            return SOC_E_DISABLED;
        }

        SOC_IF_ERROR_RETURN
            (PHY_READ_BCM542XX_1000X_MII_ANPr(unit, pc, &mii_anp));
        if (mii_anp & PHY_BCM542XX_MII_ANP_SGMII_MODE) { /* Link partner is SGMII master */
            if (mii_anp & PHY_BCM542XX_MII_ANP_SGMII_FD) { /* Full Duplex */
                switch(mii_anp & PHY_BCM542XX_MII_ANP_SGMII_SS_MASK) {
                case PHY_BCM542XX_MII_ANP_SGMII_SS_10:
                    ability->speed_full_duplex |= SOC_PA_SPEED_10MB;
                    break;
                case PHY_BCM542XX_MII_ANP_SGMII_SS_100:
                    ability->speed_full_duplex |= SOC_PA_SPEED_100MB;
                    break;
                case PHY_BCM542XX_MII_ANP_SGMII_SS_1000:
                    ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
                    break;
                }
            } else { /* Half duplex */
                switch(mii_anp & PHY_BCM542XX_MII_ANP_SGMII_SS_MASK) {
                case PHY_BCM542XX_MII_ANP_SGMII_SS_10:
                    ability->speed_half_duplex |= SOC_PA_SPEED_10MB;
                    break;
                case PHY_BCM542XX_MII_ANP_SGMII_SS_100:
                    ability->speed_half_duplex |= SOC_PA_SPEED_100MB;
                    break;
                case PHY_BCM542XX_MII_ANP_SGMII_SS_1000:
                    ability->speed_half_duplex |= SOC_PA_SPEED_1000MB;
                    break;
                }
            }
        } else {
            if (mii_anp & PHY_BCM542XX_MII_ANP_C37_FD) {
                ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
            }
            if (mii_anp & PHY_BCM542XX_MII_ANP_C37_HD) {
                ability->speed_half_duplex |= SOC_PA_SPEED_1000MB;
            }
            switch (mii_anp & (PHY_BCM542XX_MII_ANP_C37_PAUSE | PHY_BCM542XX_MII_ANP_C37_ASYM_PAUSE)) {
            case PHY_BCM542XX_MII_ANP_C37_PAUSE:
                ability->pause = SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX;
                break;
            case PHY_BCM542XX_MII_ANP_C37_ASYM_PAUSE:
                ability->pause = SOC_PA_PAUSE_TX;
                break;
            case (PHY_BCM542XX_MII_ANP_C37_PAUSE | PHY_BCM542XX_MII_ANP_C37_ASYM_PAUSE):
                ability->pause = SOC_PA_PAUSE_RX;
                break;
            }
        }
    
    }
    
    SOC_DEBUG_PRINT((DK_PHY, 
"phy_bcm542xx_adv_remote_get: u=%d p=%d ability_hd_speed=0x%x ability_fd_speed=0x%x ability_pause=0x%x rv=%d\n", unit, port, ability->speed_half_duplex, ability->speed_full_duplex, ability->pause, rv));

    return  (SOC_FAILURE(rv));
}


/*
 * Function:
 *      phy_bcm542xx_ability_local_get
 * Purpose:
 *      Get the abilities of the local gigabit PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      mode - (OUT) Port mode mask indicating supported options/speeds.
 * Returns:
 *      SOC_E_NONE
 * Notes:
 *      The ability is retrieved only for the ACTIVE medium.
 */

STATIC int
phy_bcm542xx_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    int         rv;
    phy_ctrl_t *pc;

    rv = SOC_E_NONE;

    if (NULL == ability) {
        return SOC_E_PARAM;
    }
    
    pc = EXT_PHY_SW_STATE(unit, port);

    sal_memset(ability, 0,  sizeof(soc_port_ability_t)); /* zero initialize */

    if(PHY_COPPER_MODE(unit, port)) {


        ability->speed_half_duplex  = SOC_PA_SPEED_100MB | SOC_PA_SPEED_10MB;
        ability->speed_full_duplex  = SOC_PA_SPEED_1000MB |
                                      SOC_PA_SPEED_100MB | 
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
    } else if(PHY_FIBER_MODE(unit, port)) {
        ability->speed_half_duplex  = SOC_PA_SPEED_100MB;
        ability->speed_full_duplex  = SOC_PA_SPEED_1000MB |SOC_PA_SPEED_100MB;
                                      
        ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
        ability->interface = SOC_PA_INTF_SGMII;
        ability->medium    = SOC_PA_MEDIUM_FIBER; 
        ability->loopback  = SOC_PA_LB_PHY;
        ability->flags     = SOC_PA_AUTONEG;

        if (pc->automedium) {
            ability->flags     |= SOC_PA_COMBO;
        }
    }

    SOC_DEBUG_PRINT((DK_PHY,
                  "phy_bcm542xx_ability_local_get: u=%d p=%d ability_hd_speed=0x%x, ability_fd_speed=0x%x, ability_pause=0x%x,  rv=%d\n",
                   unit, port, ability->speed_half_duplex, ability->speed_full_duplex, ability->pause, rv));
    return rv;
}

/*
 * Function:
 *      phy_bcm542xx_lb_set
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
phy_bcm542xx_lb_set(int unit, soc_port_t port, int enable)
{
    int           rv = SOC_E_NONE;
    phy_ctrl_t    *pc;
    uint16 mii_ctrl, temp16;
    pc = EXT_PHY_SW_STATE(unit, port);

    if(PHY_COPPER_MODE(unit, port)) {
    
#if AUTO_MDIX_WHEN_AN_DIS
        {
            /* Disable Auto-MDIX When autoneg disabled */
            /* Enable Auto-MDIX When autoneg disabled */
            temp16 = (enable) ? 0x0000 : PHY_BCM542XX_MII_FORCE_AUTO_MDIX_MODE;
            PHY_MODIFY_BCM542XX_MII_MISC_CTRLr(unit, pc, temp16, 
                                        PHY_BCM542XX_MII_FORCE_AUTO_MDIX_MODE);

         }
#endif

        /* Set loopback setting for 1000M */
        SOC_IF_ERROR_RETURN(
                PHY_READ_BCM542XX_MII_CTRLr(unit, pc, &mii_ctrl));
        mii_ctrl &= ~PHY_BCM542XX_MII_CTRL_LPBK_EN;
        mii_ctrl |= (enable) ? PHY_BCM542XX_MII_CTRL_LPBK_EN : 0;
        /* Clear loopback setting for 100/10M */
        SOC_IF_ERROR_RETURN(
                PHY_WRITE_BCM542XX_MII_CTRLr(unit, pc, mii_ctrl));
        /* Force link is required for 100/10M speeds to work
           in loopback */
        phy_bcm542xx_direct_reg_modify(unit, pc, 
                PHY_BCM542XX_TEST1_REG_OFFSET, 
                enable? PHY_BCM542XX_TEST1_REG_FORCE_LINK : 0,
                PHY_BCM542XX_TEST1_REG_FORCE_LINK);

    } else  if(PHY_FIBER_MODE(unit, port)) {
        temp16 = (enable) ? PHY_BCM542XX_1000BASE_X_CTRL_REG_LE : 0;
        rv   = PHY_MODIFY_BCM542XX_1000X_MII_CTRLr(unit, pc, temp16, PHY_BCM542XX_1000BASE_X_CTRL_REG_LE);


         if (enable && SOC_SUCCESS(rv)) {
            uint16        stat;
            int           link;
            soc_timeout_t to;

            /* Wait up to 5000 msec for link up */
            soc_timeout_init(&to, 5000000, 0);
            link = 0;
            while (!soc_timeout_check(&to)) {
                rv = PHY_READ_BCM542XX_1000X_MII_STATr(unit, pc, &stat);
                link = stat & PHY_BCM542XX_1000BASE_X_STAT_REG_LA;
                if (link || SOC_FAILURE(rv)) {
                    break;
                }
            }
            if (!link) {
                SOC_DEBUG_PRINT((DK_WARN,
                             "phy_bcm542xx_lb_set: u=%d p=%d TIMEOUT\n",
                             unit, port));

                rv = SOC_E_TIMEOUT;
            }
        }
    }

    SOC_DEBUG_PRINT((DK_PHY,
                   "phy_bcm542xx_lb_set: u=%d p=%d en=%d rv=%d\n", 
                    unit, port, enable, rv));

    return rv; 
}

/*
 * Function:
 *      phy_bcm542xx_lb_get
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
phy_bcm542xx_lb_get(int unit, soc_port_t port, int *enable)
{
    int rv;
    uint16 mii_ctrl;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    if(PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(
            PHY_READ_BCM542XX_MII_CTRLr(unit, pc, &mii_ctrl));
        *enable = (mii_ctrl & PHY_BCM542XX_MII_CTRL_LPBK_EN) ? 1 : 0;
    } else if(PHY_FIBER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
           (PHY_READ_BCM542XX_1000X_MII_CTRLr(unit, pc, &mii_ctrl));
        *enable = (mii_ctrl & PHY_BCM542XX_1000BASE_X_CTRL_REG_LE) ? 1 : 0;
    }
    return SOC_FAILURE(rv); 
}

/*
 * Function:
 *      phy_bcm542xx_interface_set
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
phy_bcm542xx_interface_set(int unit, soc_port_t port, soc_port_if_t pif)
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
 *      phy_bcm542xx_interface_get
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
phy_bcm542xx_interface_get(int unit, soc_port_t port, soc_port_if_t *pif)
{
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(port);

    *pif = SOC_PORT_IF_SGMII;

    return SOC_E_NONE;
}


/*
 * Function:
 *      phy_bcm542xx_mdix_set
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
phy_bcm542xx_mdix_set(int unit, soc_port_t port, soc_port_mdix_t mode)
{

    int rv = SOC_E_NONE;
    phy_ctrl_t *pc;
    int speed = 0;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        switch(mode) {
        case SOC_PORT_MDIX_AUTO:
            /* Clear bit 14 for automatic MDI crossover */
            SOC_IF_ERROR_RETURN(
                PHY_MODIFY_BCM542XX_MII_ECRr(unit, pc, 0, PHY_BCM542XX_MII_ECR_DAMC));

            /* Clear bit 9 to disable forced auto MDI xover */
            SOC_IF_ERROR_RETURN(
                PHY_MODIFY_BCM542XX_MII_MISC_CTRLr(unit, pc, 0, 
                                            PHY_BCM542XX_MII_FORCE_AUTO_MDIX_MODE));
            break;
        case SOC_PORT_MDIX_FORCE_AUTO:
            /* Clear bit 14 for automatic MDI crossover */
            SOC_IF_ERROR_RETURN(
                PHY_MODIFY_BCM542XX_MII_ECRr(unit, pc, 0, PHY_BCM542XX_MII_ECR_DAMC));

            /* Set bit 9 to enable forced auto MDI xover */
            SOC_IF_ERROR_RETURN(
                PHY_MODIFY_BCM542XX_MII_MISC_CTRLr(unit, pc, 
                                               PHY_BCM542XX_MII_FORCE_AUTO_MDIX_MODE, 
                                               PHY_BCM542XX_MII_FORCE_AUTO_MDIX_MODE));
            break;
        case SOC_PORT_MDIX_NORMAL:
            rv = phy_bcm542xx_speed_get(unit, port, &speed);
            if (SOC_FAILURE(rv)) {
                return SOC_E_FAIL;
            }
            if (speed == 0 || speed == 10 || speed == 100) {
                /* Set bit 14 for automatic MDI crossover */
                SOC_IF_ERROR_RETURN(
                    PHY_MODIFY_BCM542XX_MII_ECRr(unit, pc, 
                                                 PHY_BCM542XX_MII_ECR_DAMC, 
                                                 PHY_BCM542XX_MII_ECR_DAMC));
                SOC_IF_ERROR_RETURN(
                    PHY_WRITE_BCM542XX_TEST1r(unit, pc, 0));
            } else {
                return SOC_E_UNAVAIL;
            }
            break;
        case SOC_PORT_MDIX_XOVER:
            rv = phy_bcm542xx_speed_get(unit, port, &speed);
            if (SOC_FAILURE(rv)) {
                return SOC_E_FAIL;
            }
            if (speed == 0 || speed == 10 || speed == 100) {
                /* Set bit 14 for automatic MDI crossover */
                SOC_IF_ERROR_RETURN(
                    PHY_MODIFY_BCM542XX_MII_ECRr(unit, pc, 
                                                 PHY_BCM542XX_MII_ECR_DAMC, 
                                                 PHY_BCM542XX_MII_ECR_DAMC));
                SOC_IF_ERROR_RETURN(
                    PHY_WRITE_BCM542XX_TEST1r(unit, pc, 0x0080));
            } else {
                return SOC_E_UNAVAIL;
            }
            break;
        default :
            return SOC_E_CONFIG;
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
 *      phy_bcm542xx_mdix_get
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
phy_bcm542xx_mdix_get(int unit, soc_port_t port, soc_port_mdix_t *mode)
{
    phy_ctrl_t *pc;
    int speed = 0;

    if (mode == NULL) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);

    if (PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN(phy_bcm542xx_speed_get(unit, port, &speed));
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
 *      phy_bcm542xx_mdix_status_get
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
phy_bcm542xx_mdix_status_get(int unit, soc_port_t port, 
                         soc_port_mdix_status_t *status)
{
    phy_ctrl_t    *pc;
    uint16 mii_esrr;

    if (status == NULL) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);
    if(PHY_COPPER_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (phy_bcm542xx_direct_reg_read(unit, pc,
                        PHY_BCM542XX_PHY_EXT_STATUS_REG_OFFSET,
                        &mii_esrr));
        if (mii_esrr & 0x2000) {
            *status = SOC_PORT_MDIX_STATUS_XOVER;
        } else {
            *status = SOC_PORT_MDIX_STATUS_NORMAL;
        }
    } else {
        if(PHY_FIBER_MODE(unit, port)) {
            *status = SOC_PORT_MDIX_STATUS_NORMAL;
        }
    }

    return SOC_E_NONE;
}    

/*
 * Function:
 *     phy_bcm542xx_encode_gmode 
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
void phy_bcm542xx_encode_gmode(soc_port_phy_timesync_global_mode_t mode,
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

/*
 * Function:
 *     phy_bcm542xx_decode_gmode
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
void phy_bcm542xx_decode_gmode(uint16 value,
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

/*
 * Function:
 *    phy_bcm542xx_encode_framesync_mode 
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
void phy_bcm542xx_encode_framesync_mode(soc_port_phy_timesync_framesync_mode_t mode,
                                            uint16 *value)
{

    switch (mode & 0xf) {
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

/*
 * Function:
 *     phy_bcm542xx_decode_framesync_mode
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
void phy_bcm542xx_decode_framesync_mode(uint16 value,
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


/*
 * Function:
 *     phy_bcm542xx_encode_sync_out_mode
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
void phy_bcm542xx_encode_sync_out_mode(soc_port_phy_timesync_syncout_mode_t mode,
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


/*
 * Function:
 *     phy_bcm542xx_decode_sync_out_mode
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
void phy_bcm542xx_decode_sync_out_mode(uint16 value,
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

/*
 * Function:
 *     phy_bcm542xx_encode_egress_message_mode
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
void phy_bcm542xx_encode_egress_message_mode(soc_port_phy_timesync_event_message_egress_mode_t mode,
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
        *value |= (0x4 << offset);
        break; 
    default:
        break;
    }

}


/*
 * Function:
 *     phy_bcm542xx_encode_ingress_message_mode
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
void phy_bcm542xx_encode_ingress_message_mode(soc_port_phy_timesync_event_message_ingress_mode_t mode,
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

/*
 * Function:
 *    phy_bcm542xx_decode_egress_message_mode
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
void phy_bcm542xx_decode_egress_message_mode(uint16 value, int offset,
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


/*
 * Function:
 *    phy_bcm542xx_decode_ingress_message_mode 
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
void phy_bcm542xx_decode_ingress_message_mode(uint16 value, int offset,
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


/*
 * Function:
 *    phy_bcm542xx_timesync_config_set
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
int
phy_bcm542xx_timesync_config_set(int unit, soc_port_t port, soc_port_phy_timesync_config_t *conf)
{
    uint16  dev_port = 0;
    phy_ctrl_t *pc;
    uint16 rx_control_reg = 0, tx_control_reg = 0, rx_crc_control_reg = 0;
    uint16 en_control_reg = 0, tx_capture_en_reg = 0, rx_capture_en_reg = 0;
    uint16 nse_nco_6 = 0, nse_nco_2c = 0, value, gmode =0, framesync_mode=0;
    uint16 syncout_mode=0, tx_ts_event_cap = 0, rx_ts_event_cap = 0;
    uint16 mpls_control_reg_tx = 0, mpls_control_reg_rx = 0, i;
    uint16 data16 = 0, mask16 = 0;

    if(NULL == conf) {
        return SOC_E_PARAM;
    }
   
    pc = EXT_PHY_SW_STATE(unit, port);

    /* Dev port numbers from 0-7 */
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);
    
    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }
    
    if (conf->flags & SOC_PORT_PHY_TIMESYNC_ENABLE) {
        en_control_reg |= ((1 << (dev_port + 8)) | (1 << dev_port));
        SOC_IF_ERROR_RETURN
            (PHY_MODIFY_BCM542XX_TIME_SYNC_CTRLr(unit, pc, 1 << 0, 1 << 0)); /* exp reg f5 */
    } else {
        SOC_IF_ERROR_RETURN
            (PHY_MODIFY_BCM542XX_TIME_SYNC_CTRLr(unit, pc, 0, 1 << 0)); /* exp reg f5 */
    }

    /* Enable TX SOP & RX SOP capability in 10bt. */
    phy_bcm542xx_direct_reg_modify(unit, pc,
                        PHY_BCM542XX_PATT_GEN_CTRL_REG_OFFSET,
                        PHY_BCM542XX_PATT_GEN_CTRL_REG_TX_CRC_CHECK_EN,
                        PHY_BCM542XX_PATT_GEN_CTRL_REG_TX_CRC_CHECK_EN);

    phy_bcm542xx_direct_reg_modify(unit, pc,
                        PHY_BCM542XX_EXPF5_TIME_SYNC_REG_OFFSET,
                        PHY_BCM542XX_EXPF5_TIME_SYNC_TX_SOP_10BT_EN|
                          PHY_BCM542XX_EXPF5_TIME_SYNC_RX_SOP_10BT_EN,
                        PHY_BCM542XX_EXPF5_TIME_SYNC_TX_SOP_10BT_EN|
                          PHY_BCM542XX_EXPF5_TIME_SYNC_RX_SOP_10BT_EN);

    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
     * To access TOP level registers use phy_id of port0. Override this ports 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);

    if (conf->flags & SOC_PORT_PHY_TIMESYNC_ENABLE) {
        SOC_IF_ERROR_RETURN
            (phy_bcm542xx_direct_reg_modify(unit, pc, PHY_BCM542XX_TOP_INTERRUPT_MASK, 0, 0xff1));
    } else {
        SOC_IF_ERROR_RETURN
            (phy_bcm542xx_direct_reg_modify(unit, pc, PHY_BCM542XX_TOP_INTERRUPT_MASK, 0xff1, 0xff1));
    }

    /* Enable/disable 1588 tx/rx per port*/
    phy_bcm542xx_direct_reg_modify(unit, pc, 
                     PHY_BCM542XX_TOP_1588_SLICE_EN_CTRL_REG_OFFSET,  
                     en_control_reg, 
                     en_control_reg);

    phy_bcm542xx_direct_reg_read(unit, pc,
                     PHY_BCM542XX_TOP_1588_SLICE_EN_CTRL_REG_OFFSET,  
                     &en_control_reg);

    /* Enable TS capture + Select TS capture for event message type */
    if (conf->flags & SOC_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE) {
        tx_capture_en_reg |= (1 << dev_port);
        rx_capture_en_reg |= (1 << dev_port);

        /* Initialize NSE Block */
        phy_bcm542xx_direct_reg_modify(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_OFFSET,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_NSE_INIT,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_NSE_INIT);                    

        if(conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE) {
            tx_ts_event_cap |= BIT8;
        }
        if(conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE) { 
            rx_ts_event_cap |= BIT8;
        }
        if(conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE) {
            tx_ts_event_cap |= BIT9;
        }
        if(conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE) { 
            rx_ts_event_cap |= BIT9;
        }
        if(conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE) {
            tx_ts_event_cap |= BIT10;
        }
        if(conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE) { 
            rx_ts_event_cap |= BIT10;
        }
        if(conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE) {
            tx_ts_event_cap |= BIT11;
        }
        if(conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE) { 
            rx_ts_event_cap |= BIT11;
        }
    }

    /* Select TS capture for event message type */
    phy_bcm542xx_direct_reg_modify(unit, pc,
                     PHY_BCM542XX_TOP_1588_TX_PORT_N_TS_OFFSET_MSB(dev_port),
                     tx_ts_event_cap,
                     tx_ts_event_cap);
    phy_bcm542xx_direct_reg_modify(unit, pc,
                     PHY_BCM542XX_TOP_1588_RX_PORT_N_TS_OFFSET_MSB(dev_port),
                     rx_ts_event_cap,
                     rx_ts_event_cap);


     /* Timestamp capture enable for Rx & TX */
    phy_bcm542xx_direct_reg_modify(unit, pc, 
                     PHY_BCM542XX_TOP_1588_TX_TS_CAP_REG_OFFSET, 
                     tx_capture_en_reg, 
                     (1 << dev_port)); 

    phy_bcm542xx_direct_reg_modify(unit, pc, 
                     PHY_BCM542XX_TOP_1588_RX_TS_CAP_REG_OFFSET, 
                     rx_capture_en_reg, 
                     (1 << dev_port)); 
             

    /* Enable timestamp capture on the next frame sync event */
    if (conf->flags & SOC_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE) {
        nse_nco_6 |= PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_TS_CAP;
    }

     phy_bcm542xx_direct_reg_modify(unit, pc, 
                      PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_OFFSET, 
                      nse_nco_6, 
                      PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_TS_CAP); 
                
     /* Enable the CRC check in PTP detection receiving side */
      if (conf->flags & SOC_PORT_PHY_TIMESYNC_RX_CRC_ENABLE) {
        rx_crc_control_reg |= PHY_BCM542XX_TOP_1588_RX_TX_CTL_REG_RX_CRC_EN;
    }

    phy_bcm542xx_direct_reg_modify(unit, pc, 
                     PHY_BCM542XX_TOP_1588_RX_TX_CTL_REG_OFFSET, 
                     rx_crc_control_reg, 
                     PHY_BCM542XX_TOP_1588_RX_TX_CTL_REG_RX_CRC_EN);

    /* Enable different packet detections & checks in Rx & Tx direction */
    if (conf->flags & SOC_PORT_PHY_TIMESYNC_8021AS_ENABLE) {
        rx_control_reg |= PHY_BCM542XX_TOP_1588_RX_CTL_REG_RX_AS_EN;
        tx_control_reg |= PHY_BCM542XX_TOP_1588_TX_CTL_REG_TX_AS_EN;
    }
    if (conf->flags & SOC_PORT_PHY_TIMESYNC_L2_ENABLE) {
        rx_control_reg |= PHY_BCM542XX_TOP_1588_RX_CTL_REG_RX_L2_EN;
        tx_control_reg |= PHY_BCM542XX_TOP_1588_TX_CTL_REG_TX_L2_EN;
    }
    if (conf->flags & SOC_PORT_PHY_TIMESYNC_IP4_ENABLE) {
        rx_control_reg |= PHY_BCM542XX_TOP_1588_RX_CTL_REG_RX_IPV4_UDP_EN;
        tx_control_reg |= PHY_BCM542XX_TOP_1588_TX_CTL_REG_TX_IPV4_UDP_EN;
    }
    if (conf->flags & SOC_PORT_PHY_TIMESYNC_IP6_ENABLE) {
        rx_control_reg |= PHY_BCM542XX_TOP_1588_TX_CTL_REG_TX_IPV6_UDP_EN;
        tx_control_reg |= PHY_BCM542XX_TOP_1588_RX_CTL_REG_RX_IPV6_UDP_EN;
    }
    phy_bcm542xx_direct_reg_write(unit, pc,
                     PHY_BCM542XX_TOP_1588_TX_CTL_REG_OFFSET, 
                     tx_control_reg);
    phy_bcm542xx_direct_reg_write(unit, pc, 
                     PHY_BCM542XX_TOP_1588_RX_CTL_REG_OFFSET, 
                     rx_control_reg);

    /* Set input for NCO adder */
    if (conf->flags & SOC_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT) {
        nse_nco_2c &= ~PHY_BCM542XX_TOP_1588_NSE_NCO_2_2_FREQ_MDIO_SEL;
    } else {
        nse_nco_2c |= PHY_BCM542XX_TOP_1588_NSE_NCO_2_2_FREQ_MDIO_SEL;
    }

     phy_bcm542xx_direct_reg_modify(unit, pc, 
                      PHY_BCM542XX_TOP_1588_NSE_NCO_2_2_REG_OFFSET, 
                      nse_nco_2c, 
                      PHY_BCM542XX_TOP_1588_NSE_NCO_2_2_FREQ_MDIO_SEL); 
    
    
    /* VLAN Tags */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ITPID) {

        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_VLAN_ITPID_REG_OFFSET, 
                         conf->itpid); 
    }
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID) {

        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_VLAN_OTPID_REG_OFFSET, 
                         conf->otpid); 
    }
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID2) {
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_OTHER_OTPID_REG_OFFSET,
                         conf->otpid2); 
    }
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TS_DIVIDER) {
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_NCO_4_REG_OFFSET,
                         conf->ts_divider & 0xfff); 
    }
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_LINK_DELAY) {
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_RX_PORTN_LINK_DELAY_LSB(dev_port),
                         conf->rx_link_delay & 0xffff); 
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_RX_PORTN_LINK_DELAY_MSB(dev_port),
                         (conf->rx_link_delay>>16) & 0xffff); 

    }

    /* TIMECODE */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE) {
    
        phy_bcm542xx_direct_reg_write(unit, pc, 
                     PHY_BCM542XX_TOP_1588_TIME_CODE_0_REG_OFFSET, 
                     (uint16)(conf->original_timecode.nanoseconds & 0xffff));
        phy_bcm542xx_direct_reg_write(unit, pc, 
                     PHY_BCM542XX_TOP_1588_TIME_CODE_1_REG_OFFSET, 
                     (uint16)((conf->original_timecode.nanoseconds >> 16) & 0xffff));
        phy_bcm542xx_direct_reg_write(unit, pc, 
                     PHY_BCM542XX_TOP_1588_TIME_CODE_2_REG_OFFSET, 
                     (uint16)(COMPILER_64_LO(conf->original_timecode.seconds) & 0xffff));
        phy_bcm542xx_direct_reg_write(unit, pc, 
                     PHY_BCM542XX_TOP_1588_TIME_CODE_3_REG_OFFSET, 
                     (uint16)((COMPILER_64_LO(conf->original_timecode.seconds) >> 16) & 0xffff));
        phy_bcm542xx_direct_reg_write(unit, pc, 
                     PHY_BCM542XX_TOP_1588_TIME_CODE_4_REG_OFFSET, 
                     (uint16)(COMPILER_64_HI(conf->original_timecode.seconds) & 0xffff));
    
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_GMODE) {
        phy_bcm542xx_encode_gmode(conf->gmode,&gmode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE) {
        phy_bcm542xx_encode_framesync_mode(conf->framesync.mode, &framesync_mode);
        phy_bcm542xx_direct_reg_write(unit, pc,
                         PHY_BCM542XX_1588_NSE_DPLL_NCO_7_0_REG_OFFSET,
                          conf->framesync.length_threshold);
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_1588_NSE_DPLL_NCO_7_1_REG_OFFSET,
                          conf->framesync.event_offset);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE) {

        phy_bcm542xx_encode_sync_out_mode(conf->syncout.mode, &syncout_mode);

        /* Interval & pulse width of syncout pulse */
        phy_bcm542xx_direct_reg_write(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_3_0_REG_OFFSET,
                         conf->syncout.interval & 0xffff); 
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_NCO_3_1_REG_OFFSET,
                         ((conf->syncout.pulse_1_length & 0x3) << 14) 
                          | ((conf->syncout.interval >> 16) & 0x3fff));
        phy_bcm542xx_direct_reg_write(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_3_2_REG_OFFSET ,
                         ((conf->syncout.pulse_2_length & 0x1ff) << 7) 
                          | ((conf->syncout.pulse_1_length >> 2) & 0x7f));

        /* Local timer for SYNC_OUT */
        phy_bcm542xx_direct_reg_write(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_5_0_REG_OFFSET,
                         (uint16)(COMPILER_64_LO(conf->syncout.syncout_ts) & 0xffff));
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_NCO_5_1_REG_OFFSET,
                         (uint16)((COMPILER_64_LO(conf->syncout.syncout_ts) >> 16) & 0xffff));
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_NCO_5_2_REG_OFFSET,
                         (uint16)(COMPILER_64_HI(conf->syncout.syncout_ts)  & 0xffff));
    }
    /* GMODE */ 
     phy_bcm542xx_direct_reg_modify(unit, pc, 
                      PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_OFFSET, 
                      (gmode << 14) | (framesync_mode << 2) | (syncout_mode << 0), 
                      ( PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_GMODE  
                      | PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_FRAMESYN_MODE  
                      | PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_SYNCOUT_MODE)
                      );


    /* TX TS OFFSET */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_TIMESTAMP_OFFSET) {
        phy_bcm542xx_direct_reg_write(unit, pc,
                         PHY_BCM542XX_TOP_1588_TX_PORT_N_TS_OFFSET_LSB(dev_port), 
                         (uint16)(conf->tx_timestamp_offset & 0xffff));
        phy_bcm542xx_direct_reg_modify(unit, pc,
                         PHY_BCM542XX_TOP_1588_TX_PORT_N_TS_OFFSET_MSB(dev_port), 
                         (uint16)((conf->tx_timestamp_offset >> 16) & 0xf ),
                         PHY_BCM542XX_TOP_1588_TX_PORT_N_TS_OFFSET_TX_MSB);
    }

    /* RX TS OFFSET */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_TIMESTAMP_OFFSET) {
        phy_bcm542xx_direct_reg_write(unit, pc,
                         PHY_BCM542XX_TOP_1588_RX_PORT_N_TS_OFFSET_LSB(dev_port), 
                         (uint16)(conf->rx_timestamp_offset & 0xffff));
        phy_bcm542xx_direct_reg_modify(unit, pc,
                         PHY_BCM542XX_TOP_1588_RX_PORT_N_TS_OFFSET_MSB(dev_port), 
                         (uint16)((conf->rx_timestamp_offset >> 16) & 0xf),
                         PHY_BCM542XX_TOP_1588_RX_PORT_N_TS_OFFSET_RX_MSB);
    }


    value = 0;
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE) {
        phy_bcm542xx_encode_egress_message_mode(conf->tx_sync_mode, 0, &value);
    }
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE) {
        phy_bcm542xx_encode_egress_message_mode(conf->tx_delay_request_mode, 2, &value);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE) {
        phy_bcm542xx_encode_egress_message_mode(conf->tx_pdelay_request_mode, 4, &value);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE) {
        phy_bcm542xx_encode_egress_message_mode(conf->tx_pdelay_response_mode, 6, &value);
    }

     phy_bcm542xx_direct_reg_modify(unit, pc,  PHY_BCM542XX_TOP_1588_TX_MODE_PORT_N_OFFSET(dev_port), value, 0x00ff);


    value = 0;

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE) {
        phy_bcm542xx_encode_ingress_message_mode(conf->rx_sync_mode, 0, &value);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE) {
        phy_bcm542xx_encode_ingress_message_mode(conf->rx_delay_request_mode, 2, &value);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE) {
        phy_bcm542xx_encode_ingress_message_mode(conf->rx_pdelay_request_mode, 4, &value);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE) {
        phy_bcm542xx_encode_ingress_message_mode(conf->rx_pdelay_response_mode, 6, &value);
    }

     phy_bcm542xx_direct_reg_modify(unit, pc,  PHY_BCM542XX_TOP_1588_RX_MODE_PORT_N_OFFSET(dev_port), value, 0x00ff);


    /* DPLL initial reference phase */
     
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE) {
    
        /*[15:0]*/
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_2_0_REG_OFFSET,
                         (uint16)(COMPILER_64_LO(conf->phy_1588_dpll_ref_phase) & 0xffff));   
        /*[31:16]*/
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_2_1_REG_OFFSET,
                         (uint16)((COMPILER_64_LO(conf->phy_1588_dpll_ref_phase) >> 16) & 0xffff));   
        /*[47:32]*/    
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_2_2_REG_OFFSET,
                         (uint16)(COMPILER_64_HI(conf->phy_1588_dpll_ref_phase) & 0xffff));   

    }


    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE_DELTA) {
            /* Ref phase delta [15:0] */
            phy_bcm542xx_direct_reg_write(unit, pc,
                             PHY_BCM542XX_TOP_1588_NSE_DPLL_3_LSB_REG_OFFSET,
                             (uint16)(conf->phy_1588_dpll_ref_phase_delta & 0xffff));
            /* Ref phase delta [31:16]  */
            phy_bcm542xx_direct_reg_write(unit, pc,
                             PHY_BCM542XX_TOP_1588_NSE_DPLL_3_MSB_REG_OFFSET,
                             (uint16)((conf->phy_1588_dpll_ref_phase_delta >> 16)& 0xffff));
    }



    /* DPLL K1 */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K1) {
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_4_REG_OFFSET,
                         conf->phy_1588_dpll_k1 & 0xff);
    }

    /* DPLL K2 */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K2) {
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_5_REG_OFFSET,
                         conf->phy_1588_dpll_k2 & 0xff);
    }

    /* DPLL K3 */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K3) {
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_6_REG_OFFSET,
                         conf->phy_1588_dpll_k3 & 0xff);    
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_LOOP_FILTER) {
        /* Initial loop filter[15:0] */
        phy_bcm542xx_direct_reg_write(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_7_0_REG_OFFSET,
                         (uint16)(COMPILER_64_LO(conf->phy_1588_dpll_loop_filter) & 0xffff));
        /* Initial loop filter[31:16]  */
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_7_1_REG_OFFSET,
                         (uint16)((COMPILER_64_LO(conf->phy_1588_dpll_loop_filter) >> 16) & 0xffff));

        /*  Initial loop filter[47:32] */
        phy_bcm542xx_direct_reg_write(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_7_2_REG_OFFSET,
                         (uint16)(COMPILER_64_HI(conf->phy_1588_dpll_loop_filter) & 0xffff));
 

        /* Initial loop filter[63:48]  */
        phy_bcm542xx_direct_reg_write(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_7_3_REG_OFFSET,
                         (uint16)((COMPILER_64_HI(conf->phy_1588_dpll_loop_filter) >> 16) & 0xffff));

    }
     
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL) {

        /* MPLS controls */
        if (conf->mpls_control.flags & SOC_PORT_PHY_TIMESYNC_MPLS_ENTROPY_ENABLE) {
            mpls_control_reg_tx |= PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_ENTROPY_EN;
            mpls_control_reg_rx |= PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_ENTROPY_EN;
        }

        if (!(conf->mpls_control.flags & SOC_PORT_PHY_TIMESYNC_MPLS_CONTROL_WORD_ENABLE)) {
            mpls_control_reg_tx |= PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_CW_DIS;
            mpls_control_reg_rx |= PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_CW_DIS;
        }

        if (conf->mpls_control.flags & SOC_PORT_PHY_TIMESYNC_MPLS_ENABLE) {
            mpls_control_reg_tx |= PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_EN;
            mpls_control_reg_rx |= PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_EN;
        }

        if (conf->mpls_control.flags & SOC_PORT_PHY_TIMESYNC_MPLS_SPECIAL_LABEL_ENABLE) {
            mpls_control_reg_tx |= PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_SP_LBL_EN; 
            mpls_control_reg_rx |= PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_SP_LBL_EN;
        }

        phy_bcm542xx_direct_reg_modify(unit, pc, 
                         PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_OFFSET, 
                         mpls_control_reg_tx,
                         PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_ENTROPY_EN
                         | PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_CW_DIS 
                         | PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_EN
                         | PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_SP_LBL_EN);


        phy_bcm542xx_direct_reg_modify(unit, pc, 
                         PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_OFFSET, 
                         mpls_control_reg_rx,
                         PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_ENTROPY_EN
                         | PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_CW_DIS 
                         | PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_EN
                         | PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_SP_LBL_EN);

        for (i = 1 ; i <= conf->mpls_control.size; i++ ) {
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_LABEL_N_LSB_MASK_REG_OFFSET(i), 
                    (conf->mpls_control.labels[i-1].mask & 0xffff));
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                    PHY_BCM542XX_TOP_1588_LABEL_N_MSB_MASK_REG_OFFSET(i), 
                    (conf->mpls_control.labels[i-1].mask >> 16) & 0xf,
                    0xf);
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_LABEL_N_LSB_VAL_REG_OFFSET(i), 
                    (conf->mpls_control.labels[i-1].value & 0xffff));
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                    PHY_BCM542XX_TOP_1588_LABEL_N_MSB_VAL_REG_OFFSET(i), 
                    (conf->mpls_control.labels[i-1].value >> 16) & 0xf,
                    0xf);
            /*Enable comparison for MPLS label value N */
            phy_bcm542xx_direct_reg_modify(unit, pc,
                    PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_OFFSET,
                    (1U << 6) << (i-1),
                    (1U << 6) << (i-1));
            phy_bcm542xx_direct_reg_modify(unit, pc,
                    PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_OFFSET,
                    (1U << 6) << (i-1),
                    (1U << 6) << (i-1));
        }
    }
    data16 = 0;
    mask16 = 0;



    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL && (PHY_IS_BCM5429X(pc))) {
        phy_bcm542xx_direct_reg_read(unit, pc,
                PHY_BCM542XX_TOP_1588_INBAND_CNTL_N_REG_OFFSET(dev_port),
                &data16);
        data16 = (data16  & PHY_BCM542XX_TOP_1588_INBAND_CNTL_RESV0_ID); 
        conf->inband_ts_control.resv0_id = conf->inband_ts_control.resv0_id 
            << PHY_BCM542XX_TOP_1588_INBAND_CNTL_RESV0_ID_SHIFT;
        if(data16 != conf->inband_ts_control.resv0_id) {
            data16 = conf->inband_ts_control.resv0_id; 
        } 
        if (conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_CHECK) {
            data16 |= PHY_BCM542XX_TOP_1588_INBAND_CNTL_RESV0_ID_CHECK;
        }
        if (conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_SYNC_ENABLE) {
            data16 |= PHY_BCM542XX_TOP_1588_INBAND_CNTL_SYNC_EN;
        }
        if (conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_DELAY_RQ_ENABLE) {
            data16 |= PHY_BCM542XX_TOP_1588_INBAND_CNTL_DELAY_REQ_EN;
        }
        if (conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RQ_ENABLE) {
            data16 |= PHY_BCM542XX_TOP_1588_INBAND_CNTL_PDELAY_REQ_EN;
        }
        if (conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RESP_ENABLE) {
            data16 |= PHY_BCM542XX_TOP_1588_INBAND_CNTL_PDELAY_RESP_EN;
        }
        if(conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_UPDATE) {
            data16 |= PHY_BCM542XX_TOP_1588_INBAND_CNTL_RESV0_ID_UPDATE;
        }
        if(conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_IP_ADDR) {
            data16 |= PHY_BCM542XX_TOP_1588_INBAND_CNTL_IP_ADDR_EN; 
        }
        if(conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_MAC_ADDR) {
            data16 |= (PHY_BCM542XX_TOP_1588_INBAND_CNTL_IP_ADDR_EN 
                    | PHY_BCM542XX_TOP_1588_INBAND_CNTL_IP_MAC_SEL);
        }

        mask16 = PHY_BCM542XX_TOP_1588_INBAND_CNTL_RESV0_ID_CHECK
            | PHY_BCM542XX_TOP_1588_INBAND_CNTL_RESV0_ID
            | PHY_BCM542XX_TOP_1588_INBAND_CNTL_EN
            | PHY_BCM542XX_TOP_1588_INBAND_CNTL_RESV0_ID_UPDATE
            | PHY_BCM542XX_TOP_1588_INBAND_CNTL_IP_ADDR_EN
            | PHY_BCM542XX_TOP_1588_INBAND_CNTL_IP_MAC_SEL;

        phy_bcm542xx_direct_reg_modify(unit, pc,
                PHY_BCM542XX_TOP_1588_INBAND_CNTL_N_REG_OFFSET(dev_port),
                data16,
                mask16);

        data16 = 0;
        mask16 = 0;
        if(conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_CAP_SRC_PORT_CLK_ID) {
            data16 |= PHY_BCM542XX_TOP_1588_CAP_SRC_PORT_CLK_ID;
        } 
        if(conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_SRC_PORT_NUM) {
            data16 |= PHY_BCM542XX_TOP_1588_MATCH_SRC_PORT_NUM;
        } 
        if(conf->inband_ts_control.flags & SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_VLAN_ID) {
            data16 |= PHY_BCM542XX_TOP_1588_MATCH_VLAN_ID;
        }

        mask16 = PHY_BCM542XX_TOP_1588_CAP_SRC_PORT_CLK_ID
            | PHY_BCM542XX_TOP_1588_MATCH_SRC_PORT_NUM
            | PHY_BCM542XX_TOP_1588_MATCH_VLAN_ID;

        phy_bcm542xx_direct_reg_modify(unit, pc,
                PHY_BCM542XX_TOP_1588_LABEL_N_MSB_MASK_REG_OFFSET(dev_port),
                data16,
                mask16);

        /* Timestamp capture enable for Rx & TX */
        phy_bcm542xx_direct_reg_modify(unit, pc, 
                PHY_BCM542XX_TOP_1588_TX_TS_CAP_REG_OFFSET, 
                (1 << dev_port), 
                (1 << dev_port)); 

        phy_bcm542xx_direct_reg_modify(unit, pc, 
                PHY_BCM542XX_TOP_1588_RX_TS_CAP_REG_OFFSET, 
                (1 << dev_port), 
                (1 << dev_port)); 

		phy_bcm542xx_direct_reg_modify(unit, pc,
				PHY_BCM542XX_TOP_1588_RX_CF_REG_OFFSET,
				PHY_BCM542XX_TOP_1588_RX_CF_UPD(dev_port)
				| PHY_BCM542XX_TOP_1588_RX_CS_UPD(dev_port),
				PHY_BCM542XX_TOP_1588_RX_CF_UPD(dev_port)
				| PHY_BCM542XX_TOP_1588_RX_CS_UPD(dev_port));

		phy_bcm542xx_direct_reg_modify(unit, pc,
				PHY_BCM542XX_TOP_1588_TX_CS_UPD_REG_OFFSET,
				PHY_BCM542XX_TOP_1588_TX_CS_UPD_MODE2(dev_port)
				| PHY_BCM542XX_TOP_1588_TX_CS_UPD_MODE3(dev_port),
				PHY_BCM542XX_TOP_1588_TX_CS_UPD_MODE2(dev_port)
				| PHY_BCM542XX_TOP_1588_TX_CS_UPD_MODE3(dev_port));


		phy_bcm542xx_direct_reg_modify(unit, pc,
				PHY_BCM542XX_TOP_1588_64b_TIMECODE_SEL_REG_OFFSET,
				PHY_BCM542XX_TOP_1588_64b_TIMECODE_SEL_RX_N(dev_port)
				| PHY_BCM542XX_TOP_1588_64b_TIMECODE_SEL_TX_N(dev_port),
				PHY_BCM542XX_TOP_1588_64b_TIMECODE_SEL_RX_N(dev_port)
				| PHY_BCM542XX_TOP_1588_64b_TIMECODE_SEL_TX_N(dev_port));

		phy_bcm542xx_direct_reg_modify(unit, pc,
				PHY_BCM542XX_TOP_1588_DPLL_DB_SEL_REG_OFFSET, 
				PHY_BCM542XX_TOP_1588_DPLL_DB_SEL_RSVD,
				PHY_BCM542XX_TOP_1588_DPLL_DB_SEL_RSVD);


	}

    /* Tx/Rx gphycore SOP or internal 1588 SOP:
       Select internal 1588 SOP (1)
     */ 
    phy_bcm542xx_direct_reg_modify(unit, pc,
            PHY_BCM542XX_TOP_1588_SOP_SEL_REG_OFFSET,
            PHY_BCM542XX_TOP_1588_SOP_SEL_TX_PORT_N(dev_port)
            | PHY_BCM542XX_TOP_1588_SOP_SEL_RX_PORT_N(dev_port),
            PHY_BCM542XX_TOP_1588_SOP_SEL_TX_PORT_N(dev_port)
            | PHY_BCM542XX_TOP_1588_SOP_SEL_RX_PORT_N(dev_port));


    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
     * To access TOP level registers use phy_id of port0. Now restore back 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
    
    return SOC_E_NONE;
}


/*
 * Function:
 *    phy_bcm542xx_timesync_config_get
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
int
phy_bcm542xx_timesync_config_get(int unit, soc_port_t port, soc_port_phy_timesync_config_t *conf)
{
    uint16 dev_port, tx_control_reg = 0, en_control_reg = 0; 
    uint16 tx_capture_en_reg = 0, rx_capture_en_reg=0, rx_tx_control_reg;
    uint16 nse_nco_6 = 0, nse_nco_2_2 = 0;
    uint16 i, temp1, temp2, temp3, temp4, value, mpls_control_reg_rx; 
    uint16 data16;
    soc_port_phy_timesync_global_mode_t gmode = SOC_PORT_PHY_TIMESYNC_MODE_FREE;
    soc_port_phy_timesync_framesync_mode_t framesync_mode = SOC_PORT_PHY_TIMESYNC_FRAMESYNC_SYNCIN1;
    soc_port_phy_timesync_syncout_mode_t syncout_mode = SOC_PORT_PHY_TIMESYNC_SYNCOUT_DISABLE;
    phy_ctrl_t *pc;

    if(NULL == conf) {
        return SOC_E_PARAM;
    }
    
    pc = EXT_PHY_SW_STATE(unit, port);

    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
     * To access TOP level registers use phy_id of port0. Override this ports 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
    
    /* Dev port numbers from 0-7 */
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);
    
    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }

    conf->flags = 0;

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_FLAGS) {

        /* SLICE ENABLE CONTROL register */
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_SLICE_EN_CTRL_REG_OFFSET, &en_control_reg); 

        /* Tx & Rx (+8)*/
        if ((en_control_reg & (1U<<dev_port)) && (en_control_reg & (1U<<(dev_port + 8)))) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_ENABLE;
        }

        /* TXRX SOP TS CAPTURE ENABLE reg */
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_TX_TS_CAP_REG_OFFSET, 
                         &tx_capture_en_reg);

        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_RX_TS_CAP_REG_OFFSET, 
                         &rx_capture_en_reg); 

        if( (tx_capture_en_reg & (1U << dev_port)) && (rx_capture_en_reg & (1U << dev_port))) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;
        }

        phy_bcm542xx_direct_reg_read(unit, pc, 
                                        PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_OFFSET,
                                        &nse_nco_6);

        if (nse_nco_6 & PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_TS_CAP) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE;
        }

        /* RX TX CONTROL register */
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_RX_TX_CTL_REG_OFFSET,
                         &rx_tx_control_reg);

        if (rx_tx_control_reg & PHY_BCM542XX_TOP_1588_RX_TX_CTL_REG_RX_CRC_EN) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_RX_CRC_ENABLE;
        }

        /* TX CONTROL register */
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_TX_CTL_REG_OFFSET,
                         &tx_control_reg);
                                          
        if (tx_control_reg & PHY_BCM542XX_TOP_1588_TX_CTL_REG_TX_AS_EN) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_8021AS_ENABLE;
        }

        if (tx_control_reg & PHY_BCM542XX_TOP_1588_TX_CTL_REG_TX_L2_EN) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_L2_ENABLE;
        }

        if (tx_control_reg & PHY_BCM542XX_TOP_1588_TX_CTL_REG_TX_IPV4_UDP_EN) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_IP4_ENABLE;
        }

        if (tx_control_reg & PHY_BCM542XX_TOP_1588_TX_CTL_REG_TX_IPV6_UDP_EN) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_IP6_ENABLE;
        }
        phy_bcm542xx_direct_reg_read(unit, pc,
                                     PHY_BCM542XX_TOP_1588_NSE_NCO_2_2_REG_OFFSET,
                                     &nse_nco_2_2);

        if (!(nse_nco_2_2 & PHY_BCM542XX_TOP_1588_NSE_NCO_2_2_FREQ_MDIO_SEL)) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT;
         }

    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_GMODE) {
        phy_bcm542xx_decode_gmode((nse_nco_6 >> 14),&gmode);
        conf->gmode = gmode;
    }

    /* framesync */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE) {
        
        phy_bcm542xx_decode_framesync_mode((nse_nco_6 >> 2), &framesync_mode);
        conf->framesync.mode = framesync_mode;

        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_7_0_REG_OFFSET,
                         &value);
        conf->framesync.length_threshold = value;
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_7_0_REG_OFFSET,
                         &value);
        conf->framesync.event_offset = value;

    }

    /* Syncout */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE) {
    
        phy_bcm542xx_decode_sync_out_mode((nse_nco_6 ), &syncout_mode); 
        conf->syncout.mode = syncout_mode;

        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_3_0_REG_OFFSET,
                         &temp1);
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_3_1_REG_OFFSET,
                         &temp2);
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_3_2_REG_OFFSET,
                         &temp3);

        conf->syncout.pulse_1_length =  ((temp3 & 0x7f) << 2) | ((temp2 >> 14) & 0x3);
        conf->syncout.pulse_2_length = (temp3 >> 7) & 0x1ff;
        conf->syncout.interval =  ((temp2 & 0x3fff) << 16) | temp1;

        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_5_0_REG_OFFSET,
                         &temp1);
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_5_1_REG_OFFSET,
                         &temp2);
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_NCO_5_2_REG_OFFSET,
                         &temp3);

        COMPILER_64_SET(conf->syncout.syncout_ts, ((uint32)temp3),  (((uint32)temp2<<16)|((uint32)temp1)));
    }

    /* VLAN Tags */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ITPID) {

        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_VLAN_ITPID_REG_OFFSET, 
                         &conf->itpid); 
    }
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID) {

        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_VLAN_OTPID_REG_OFFSET, 
                         &conf->otpid); 
    }
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID2) {
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_OTHER_OTPID_REG_OFFSET,
                         &conf->otpid2); 
    }
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TS_DIVIDER) {
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_NCO_4_REG_OFFSET,
                         &conf->ts_divider); 
    }
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_LINK_DELAY) {
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_RX_PORTN_LINK_DELAY_LSB(dev_port),
                         &temp1); 
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_RX_PORTN_LINK_DELAY_MSB(dev_port),
                         &temp2);
        conf->rx_link_delay = ((uint32)temp2 << 16) | temp1; 
    }

    /* TIMECODE */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE) {
    
        phy_bcm542xx_direct_reg_read(unit, pc, 
                     PHY_BCM542XX_TOP_1588_TIME_CODE_0_REG_OFFSET, 
                     &temp1);
        phy_bcm542xx_direct_reg_read(unit, pc, 
                     PHY_BCM542XX_TOP_1588_TIME_CODE_1_REG_OFFSET, 
                     &temp2);

        conf->original_timecode.nanoseconds = ((uint32)temp2 << 16) | temp1;

        phy_bcm542xx_direct_reg_read(unit, pc, 
                     PHY_BCM542XX_TOP_1588_TIME_CODE_2_REG_OFFSET, 
                     &temp1);
        phy_bcm542xx_direct_reg_read(unit, pc, 
                     PHY_BCM542XX_TOP_1588_TIME_CODE_3_REG_OFFSET, 
                     &temp2);
        phy_bcm542xx_direct_reg_read(unit, pc, 
                     PHY_BCM542XX_TOP_1588_TIME_CODE_4_REG_OFFSET, 
                     &temp3);
        COMPILER_64_SET(conf->original_timecode.seconds, ((uint32)temp3),  (((uint32)temp2<<16)|((uint32)temp1)));
    }

    /* TX TS OFFSET */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_TIMESTAMP_OFFSET) {
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_TX_PORT_N_TS_OFFSET_LSB(dev_port), 
                         &temp1);
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_TX_PORT_N_TS_OFFSET_MSB(dev_port), 
                         &temp2);
        conf->tx_timestamp_offset = ((temp2 & 0xf) << 16) | temp1;
    }   

    /* RX TS OFFSET */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_TIMESTAMP_OFFSET) {
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_RX_PORT_N_TS_OFFSET_LSB(dev_port), 
                         &temp1);
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_RX_PORT_N_TS_OFFSET_MSB(dev_port), 
                         &temp2);
        conf->rx_timestamp_offset = ((temp2 & 0xf) << 16) | temp1;
    }   

     phy_bcm542xx_direct_reg_read(unit, pc,  PHY_BCM542XX_TOP_1588_TX_MODE_PORT_N_OFFSET(dev_port), &value);

    
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE) {
        phy_bcm542xx_decode_egress_message_mode(value, 0, &conf->tx_sync_mode);
    }
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE) {
        phy_bcm542xx_decode_egress_message_mode(value, 2, &conf->tx_delay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE) {
        phy_bcm542xx_decode_egress_message_mode(value, 4, &conf->tx_pdelay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE) {
        phy_bcm542xx_decode_egress_message_mode(value, 6, &conf->tx_pdelay_response_mode);
    }

     phy_bcm542xx_direct_reg_read(unit, pc,  PHY_BCM542XX_TOP_1588_RX_MODE_PORT_N_OFFSET(dev_port), &value);


    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE) {
        phy_bcm542xx_decode_ingress_message_mode(value, 0, &conf->rx_sync_mode);
    }
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE) {
        phy_bcm542xx_decode_ingress_message_mode(value, 2, &conf->rx_delay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE) {
        phy_bcm542xx_decode_ingress_message_mode(value, 4, &conf->rx_pdelay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE) {
        phy_bcm542xx_decode_ingress_message_mode(value, 6, &conf->rx_pdelay_response_mode);
    }


    /* DPLL initial reference phase */
     
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE) {
    
        /*[15:0]*/
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_2_0_REG_OFFSET,
                         &temp1);   
        /*[31:16]*/
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_2_1_REG_OFFSET,
                         &temp2);   
        /*[47:32]*/    
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_2_2_REG_OFFSET,
                         &temp3);
         COMPILER_64_SET(conf->phy_1588_dpll_ref_phase, ((uint32)temp3), (((uint32)temp2<<16)|((uint32)temp1)));
    }


    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE_DELTA) {
            /* Ref phase delta [15:0] */
            phy_bcm542xx_direct_reg_read(unit, pc,
                             PHY_BCM542XX_TOP_1588_NSE_DPLL_3_LSB_REG_OFFSET,
                             &temp1);
            /* Ref phase delta [31:16]  */
            phy_bcm542xx_direct_reg_read(unit, pc,
                             PHY_BCM542XX_TOP_1588_NSE_DPLL_3_MSB_REG_OFFSET,
                             &temp2);
            conf->phy_1588_dpll_ref_phase_delta = (temp2 << 16) | temp1;
    }



    /* DPLL K1 */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K1) {
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_4_REG_OFFSET,
                         &temp1);
        conf->phy_1588_dpll_k1 = temp1 & 0xff;
    }

    /* DPLL K2 */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K2) {
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_5_REG_OFFSET,
                         &temp1);
        conf->phy_1588_dpll_k2 = temp1 & 0xff;
    }

    /* DPLL K3 */
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K3) {
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_6_REG_OFFSET,
                         &temp1);
        conf->phy_1588_dpll_k3 = temp1 & 0xff;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_LOOP_FILTER) {
        /* Initial loop filter[15:0] */
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_7_0_REG_OFFSET,
                         &temp1);

        /* Initial loop filter[31:16]  */
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_7_1_REG_OFFSET,
                         &temp2);

        /*  Initial loop filter[47:32] */
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_7_2_REG_OFFSET,
                         &temp3);

        /* Initial loop filter[63:48]  */
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_NSE_DPLL_7_3_REG_OFFSET,
                         &temp4);

        COMPILER_64_SET(conf->phy_1588_dpll_loop_filter, (((uint32)temp4<<16)|((uint32)temp3)), (((uint32)temp2<<16)|((uint32)temp1)));

    }

 
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL) {
    
        /* MPLS controls */
#if 0
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_OFFSET, 
                         &mpls_control_reg_tx);
#endif
        phy_bcm542xx_direct_reg_read(unit, pc, 
                         PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_OFFSET, 
                         &mpls_control_reg_rx);

        if ( (mpls_control_reg_rx & PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_ENTROPY_EN)
            /*&& (mpls_control_reg_tx & PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_ENTROPY_EN)*/) {
            conf->mpls_control.flags |= SOC_PORT_PHY_TIMESYNC_MPLS_ENTROPY_ENABLE;
        }
        if ( !(mpls_control_reg_rx & PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_CW_DIS))
            /*&& (mpls_control_reg_tx & PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_CW_DIS)*/ {
            conf->mpls_control.flags |= SOC_PORT_PHY_TIMESYNC_MPLS_CONTROL_WORD_ENABLE;
        }
        if ( (mpls_control_reg_rx & PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_EN)
            /*&& (mpls_control_reg_tx & PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_EN)*/) {
            conf->mpls_control.flags |= SOC_PORT_PHY_TIMESYNC_MPLS_ENABLE;
        }
        if ( (mpls_control_reg_rx & PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_SP_LBL_EN)
            /*&& (mpls_control_reg_tx & PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_SP_LBL_EN)*/) {
            conf->mpls_control.flags |= SOC_PORT_PHY_TIMESYNC_MPLS_SPECIAL_LABEL_ENABLE;
        }

        for (i = 1 ; i <= conf->mpls_control.size; i++ ) {
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_LABEL_N_LSB_MASK_REG_OFFSET(i), 
                    &temp1);
            conf->mpls_control.labels[i-1].mask = temp1;
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_LABEL_N_MSB_MASK_REG_OFFSET(i), 
                    &temp1);
            conf->mpls_control.labels[i-1].mask |= (temp1 & 0xf) << 16;


            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_LABEL_N_LSB_VAL_REG_OFFSET(i), 
                    &temp1);
            conf->mpls_control.labels[i-1].value = temp1;
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_LABEL_N_MSB_VAL_REG_OFFSET(i), 
                    &temp1);
            conf->mpls_control.labels[i-1].value |= (temp1 & 0xf) << 16;
        }
    }

    data16 = 0;
    if ((conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_INBAND_TS_CONTROL) && (PHY_IS_BCM5429X(pc))) {
        phy_bcm542xx_direct_reg_read(unit, pc,
                PHY_BCM542XX_TOP_1588_INBAND_CNTL_N_REG_OFFSET(dev_port),
                &data16);
        conf->inband_ts_control.resv0_id = (data16  & PHY_BCM542XX_TOP_1588_INBAND_CNTL_RESV0_ID) >> PHY_BCM542XX_TOP_1588_INBAND_CNTL_RESV0_ID_SHIFT;
        if(data16 & PHY_BCM542XX_TOP_1588_INBAND_CNTL_RESV0_ID_CHECK) {
            conf->inband_ts_control.flags |= SOC_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_CHECK;
        }
        if(data16 & PHY_BCM542XX_TOP_1588_INBAND_CNTL_SYNC_EN) {
            conf->inband_ts_control.flags |= SOC_PORT_PHY_TIMESYNC_INBAND_TS_SYNC_ENABLE;
        }
        if(data16 & PHY_BCM542XX_TOP_1588_INBAND_CNTL_DELAY_REQ_EN) {
            conf->inband_ts_control.flags |= SOC_PORT_PHY_TIMESYNC_INBAND_TS_DELAY_RQ_ENABLE;
        }
        if(data16 & PHY_BCM542XX_TOP_1588_INBAND_CNTL_PDELAY_REQ_EN) {
            conf->inband_ts_control.flags |= SOC_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RQ_ENABLE;
        }
        if(data16 & PHY_BCM542XX_TOP_1588_INBAND_CNTL_PDELAY_RESP_EN) {
            conf->inband_ts_control.flags |= SOC_PORT_PHY_TIMESYNC_INBAND_TS_PDELAY_RESP_ENABLE;
        }

        if(data16 & PHY_BCM542XX_TOP_1588_INBAND_CNTL_RESV0_ID_UPDATE) {
            conf->inband_ts_control.flags |= SOC_PORT_PHY_TIMESYNC_INBAND_TS_RESV0_ID_UPDATE;
        }
        if(data16 & PHY_BCM542XX_TOP_1588_INBAND_CNTL_IP_ADDR_EN) {
            if(data16 & PHY_BCM542XX_TOP_1588_INBAND_CNTL_IP_MAC_SEL) {
                conf->inband_ts_control.flags |= SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_MAC_ADDR;
            } else {
                conf->inband_ts_control.flags |= SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_IP_ADDR;
            }
        }

        phy_bcm542xx_direct_reg_read(unit, pc,
                PHY_BCM542XX_TOP_1588_LABEL_N_MSB_MASK_REG_OFFSET(dev_port),
                &data16);

        if(data16 & PHY_BCM542XX_TOP_1588_CAP_SRC_PORT_CLK_ID) {
            conf->inband_ts_control.flags |= SOC_PORT_PHY_TIMESYNC_INBAND_TS_CAP_SRC_PORT_CLK_ID;
        }
        if(data16 & PHY_BCM542XX_TOP_1588_MATCH_SRC_PORT_NUM) {
            conf->inband_ts_control.flags |= PHY_BCM542XX_TOP_1588_MATCH_SRC_PORT_NUM;
        }
        if(data16 & PHY_BCM542XX_TOP_1588_MATCH_VLAN_ID) {
            conf->inband_ts_control.flags |= SOC_PORT_PHY_TIMESYNC_INBAND_TS_MATCH_VLAN_ID;
        }

    }
    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
     * To access TOP level registers use phy_id of port0. Now restore back 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);

    return SOC_E_NONE;
}



/*
 * Function:
 *    phy_bcm542xx_timesync_control_set
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
int
phy_bcm542xx_timesync_control_set(int unit, soc_port_t port, soc_port_control_phy_timesync_t type, uint64 value)
{
    uint16 dev_port, temp1, temp2;
    uint32 value32;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
     * To access TOP level registers use phy_id of port0. Override this ports 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
    
    /* Dev port numbers from 0-7 */
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);
    
    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }
    

   switch (type) {

       case SOC_PORT_CONTROL_PHY_TIMESYNC_CAPTURE_TIMESTAMP:
       case SOC_PORT_CONTROL_PHY_TIMESYNC_HEARTBEAT_TIMESTAMP:
            return SOC_E_FAIL;
       
       case SOC_PORT_CONTROL_PHY_TIMESYNC_NCOADDEND:
            break;

       case SOC_PORT_CONTROL_PHY_TIMESYNC_FRAMESYNC:
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                             PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_OFFSET,  
                             (COMPILER_64_LO(value) & 0xF) << 2, (0xF << 2));
            break;  

       case SOC_PORT_CONTROL_PHY_TIMESYNC_LOCAL_TIME:
           phy_bcm542xx_direct_reg_write(unit, pc, 
                            PHY_BCM542XX_TOP_1588_NSE_NCO_2_0_REG_OFFSET, 
                            (uint16)(COMPILER_64_LO(value) & 0xffff));
           phy_bcm542xx_direct_reg_write(unit, pc, 
                            PHY_BCM542XX_TOP_1588_NSE_NCO_2_1_REG_OFFSET, 
                            (uint16)((COMPILER_64_LO(value) >> 16) & 0xffff));
           phy_bcm542xx_direct_reg_modify(unit, pc, 
                            PHY_BCM542XX_TOP_1588_NSE_NCO_2_2_REG_OFFSET, 
                            (uint16)(COMPILER_64_HI(value) & 0xfff), 0xfff); 
           break;

       case SOC_PORT_CONTROL_PHY_TIMESYNC_LOAD_CONTROL:
           temp1 = 0;
           temp2 = 0;
           value32 = COMPILER_64_LO(value); 

           if (value32 &  SOC_PORT_PHY_TIMESYNC_TN_LOAD) {
               temp1 |= 1U << 11;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_TN_ALWAYS_LOAD) {
               temp2 |= 1U << 11;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_TIMECODE_LOAD) {
               temp1 |= 1U << 10;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_TIMECODE_ALWAYS_LOAD) {
               temp2 |= 1U << 10;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_SYNCOUT_LOAD) {
               temp1 |= 1U << 9;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_SYNCOUT_ALWAYS_LOAD) {
               temp2 |= 1U << 9;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_NCO_DIVIDER_LOAD) {
               temp1 |= 1U << 8;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_NCO_DIVIDER_ALWAYS_LOAD) {
               temp2 |= 1U << 8;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_LOCAL_TIME_LOAD) {
               temp1 |= 1U << 7;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_LOCAL_TIME_ALWAYS_LOAD) {
               temp2 |= 1U << 7;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_NCO_ADDEND_LOAD) {
               temp1 |= 1U << 6;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_NCO_ADDEND_ALWAYS_LOAD) {
               temp2 |= 1U << 6;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_LOAD) {
               temp1 |= 1U << 5;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_ALWAYS_LOAD) {
               temp2 |= 1U << 5;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_LOAD) {
               temp1 |= 1U << 4;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_ALWAYS_LOAD) {
               temp2 |= 1U << 4;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_LOAD) {
               temp1 |= 1U << 3;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_ALWAYS_LOAD) {
               temp2 |= 1U << 3;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_K3_LOAD) {
               temp1 |= 1U << 2;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_K3_ALWAYS_LOAD) {
               temp2 |= 1U << 2;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_K2_LOAD) {
               temp1 |= 1U << 1;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_K2_ALWAYS_LOAD) {
               temp2 |= 1U << 1;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_K1_LOAD) {
               temp1 |= 1U << 0;
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_DPLL_K1_ALWAYS_LOAD) {
               temp2 |= 1U << 0;
           }

            phy_bcm542xx_direct_reg_write(unit, pc, 
                             PHY_BCM542XX_TOP_1588_SHD_CTL_REG_OFFSET,
                             temp1); 
            phy_bcm542xx_direct_reg_write(unit, pc, 
                             PHY_BCM542XX_TOP_1588_SHD_LD_REG_OFFSET,
                             temp2); 
           break;

       case SOC_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT_MASK:
           /* disables the interrupt */
           temp1 = 0;
           value32 = COMPILER_64_LO(value);
           if (value32 &  SOC_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_MASK) {
               temp1 |= 1U << (dev_port + 1);
           }
           if (value32 &  SOC_PORT_PHY_TIMESYNC_FRAMESYNC_INTERRUPT_MASK) {
               temp1 |= 1U << 0;
           }
           phy_bcm542xx_direct_reg_modify(unit, pc, 
                             PHY_BCM542XX_TOP_1588_INT_MASK_REG_OFFSET,
                             temp1, (1U << (dev_port + 1)) | (1U << 0));
           break;
       
       case SOC_PORT_CONTROL_PHY_TIMESYNC_TX_TIMESTAMP_OFFSET:
           value32 = COMPILER_64_LO(value);
           phy_bcm542xx_direct_reg_write(unit, pc,
                         PHY_BCM542XX_TOP_1588_TX_PORT_N_TS_OFFSET_LSB(dev_port), 
                         (uint16)(value32 & 0xffff));
           phy_bcm542xx_direct_reg_modify(unit, pc,
                         PHY_BCM542XX_TOP_1588_TX_PORT_N_TS_OFFSET_MSB(dev_port), 
                         (uint16)((value32 >> 16) & 0xf ),
                         PHY_BCM542XX_TOP_1588_TX_PORT_N_TS_OFFSET_TX_MSB);

           break;
       case SOC_PORT_CONTROL_PHY_TIMESYNC_RX_TIMESTAMP_OFFSET:
           value32 = COMPILER_64_LO(value);
           phy_bcm542xx_direct_reg_write(unit, pc,
                         PHY_BCM542XX_TOP_1588_RX_PORT_N_TS_OFFSET_LSB(dev_port), 
                         (uint16)(value32 & 0xffff));
           phy_bcm542xx_direct_reg_modify(unit, pc,
                         PHY_BCM542XX_TOP_1588_RX_PORT_N_TS_OFFSET_MSB(dev_port), 
                         (uint16)((value32 >> 16) & 0xf),
                         PHY_BCM542XX_TOP_1588_RX_PORT_N_TS_OFFSET_RX_MSB);

           break;    
           
       default:
           /***********************************************************************
            * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
            * To access TOP level registers use phy_id of port0. Now restore back 
            */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
            return SOC_E_UNAVAIL;
            break;
    } 

   /***********************************************************************
    * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
    * To access TOP level registers use phy_id of port0. Now restore back 
    */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);

    return SOC_E_NONE;
}



/*
 * Function:
 *    phy_bcm542xx_timesync_control_get
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
int
phy_bcm542xx_timesync_control_get(int unit, soc_port_t port, soc_port_control_phy_timesync_t type, uint64 *value)
{
    uint32 value32 =0;
    uint16 dev_port, value0, value1, value2;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
     * To access TOP level registers use phy_id of port0. Override this ports 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);

    /* Dev port numbers from 0-7 */
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);
    
    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }
    

    switch (type) {
    case SOC_PORT_CONTROL_PHY_TIMESYNC_HEARTBEAT_TIMESTAMP:

        phy_bcm542xx_direct_reg_modify(unit, pc, 
                                PHY_BCM542XX_TOP_1588_CNTR_DBG_REG_OFFSET,  
                                PHY_BCM542XX_TOP_1588_CNTR_DBG_HB_RD_START,
                                PHY_BCM542XX_TOP_1588_CNTR_DBG_HB_RD_START
                                | PHY_BCM542XX_TOP_1588_CNTR_DBG_HB_RD_END);

        phy_bcm542xx_direct_reg_read(unit, pc, 
                                PHY_BCM542XX_TOP_1588_HEARTBEAT_0_REG_OFFSET,  
                                &value0);

        phy_bcm542xx_direct_reg_read(unit, pc, 
                                PHY_BCM542XX_TOP_1588_HEARTBEAT_1_REG_OFFSET,  
                                &value1);

        phy_bcm542xx_direct_reg_read(unit, pc, 
                                PHY_BCM542XX_TOP_1588_HEARTBEAT_2_REG_OFFSET,  
                                &value2);
        
        phy_bcm542xx_direct_reg_modify(unit, pc, 
                                PHY_BCM542XX_TOP_1588_CNTR_DBG_REG_OFFSET,  
                                PHY_BCM542XX_TOP_1588_CNTR_DBG_HB_RD_END,
                                PHY_BCM542XX_TOP_1588_CNTR_DBG_HB_RD_START
                                | PHY_BCM542XX_TOP_1588_CNTR_DBG_HB_RD_END);

        COMPILER_64_SET((*value), ((uint32)value2),  (((uint32)value1<<16)|((uint32)value0)));

        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_CAPTURE_TIMESTAMP:

        phy_bcm542xx_direct_reg_modify(unit, pc, 
                                PHY_BCM542XX_TOP_1588_CNTR_DBG_REG_OFFSET,  
                                (dev_port << CNTR_DBG_TS_SLICE_SEL_SHIFT),
                                PHY_BCM542XX_TOP_1588_CNTR_DBG_TS_SLICE_SEL);

        phy_bcm542xx_direct_reg_write(unit, pc, 
                                PHY_BCM542XX_TOP_1588_TS_RD_START_END_REG_OFFSET,  
                                PHY_BCM542XX_TOP_1588_TS_RD_START_END_REG_TS_START(dev_port));
        
        phy_bcm542xx_direct_reg_read(unit, pc, 
                                PHY_BCM542XX_TOP_1588_TIMESTAMP_0_REG_OFFSET, 
                                &value0);

        phy_bcm542xx_direct_reg_read(unit, pc, 
                                PHY_BCM542XX_TOP_1588_TIMESTAMP_1_REG_OFFSET, 
                                &value1);

        phy_bcm542xx_direct_reg_read(unit, pc, 
                                PHY_BCM542XX_TOP_1588_TIMESTAMP_2_REG_OFFSET, 
                                &value2);
        
        phy_bcm542xx_direct_reg_write(unit, pc, 
                                PHY_BCM542XX_TOP_1588_TS_RD_START_END_REG_OFFSET,  
                                PHY_BCM542XX_TOP_1588_TS_RD_START_END_REG_TS_END(dev_port));
        phy_bcm542xx_direct_reg_write(unit, pc, 
                                PHY_BCM542XX_TOP_1588_TS_RD_START_END_REG_OFFSET,  
                                0);

        COMPILER_64_SET((*value), ((uint32)value2),  (((uint32)value1<<16)|((uint32)value0)));

        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_NCOADDEND:
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_FRAMESYNC: 
        phy_bcm542xx_direct_reg_read(unit, pc, PHY_BCM542XX_TOP_1588_NSE_NCO_6_REG_OFFSET, &value0); 
        COMPILER_64_SET((*value), 0,  (uint32)((value0 & (1U << 5)) > 0));   
        break;
    
    case SOC_PORT_CONTROL_PHY_TIMESYNC_LOAD_CONTROL:

         phy_bcm542xx_direct_reg_read(unit, pc, 
                             PHY_BCM542XX_TOP_1588_SHD_CTL_REG_OFFSET,
                             &value1); 
         phy_bcm542xx_direct_reg_read(unit, pc, 
                             PHY_BCM542XX_TOP_1588_SHD_LD_REG_OFFSET,
                             &value2); 
        
        if (value1 & (1U << 11)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_TN_LOAD;
        }
        if (value2 & (1U << 11)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_TN_ALWAYS_LOAD;
        }
        if (value1 & (1U << 10)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_TIMECODE_LOAD;
        }
        if (value2 & (1U << 10)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_TIMECODE_ALWAYS_LOAD;
        }
        if (value1 & (1U << 9)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_SYNCOUT_LOAD;
        }
        if (value2 & (1U << 9)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_SYNCOUT_ALWAYS_LOAD;
        }
        if (value1 & (1U << 8)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_NCO_DIVIDER_LOAD;
        }
        if (value1 & (1U << 8)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_NCO_DIVIDER_ALWAYS_LOAD;
        }
        if (value1 & (1U << 7)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_LOCAL_TIME_LOAD;
        }
        if (value1 & (1U << 7)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_LOCAL_TIME_ALWAYS_LOAD;
        }
        if (value1 & (1U << 6)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_NCO_ADDEND_LOAD;
        }
        if (value2 & (1U << 6)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_NCO_ADDEND_ALWAYS_LOAD;
        }
        if (value1 & (1U << 5)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_LOAD;
        }
        if (value2 & (1U << 5)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_ALWAYS_LOAD;
        }
        if (value1 & (1U << 4)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_LOAD;
        }
        if (value2 & (1U << 4)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_ALWAYS_LOAD;
        }
        if (value1 & (1U << 3)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_LOAD;
        }
        if (value2 & (1U << 3)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_ALWAYS_LOAD;
        }
        if (value1 & (1U << 2)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_K3_LOAD;
        }
        if (value2 & (1U << 2)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_K3_ALWAYS_LOAD;
        }
        if (value1 & (1U << 1)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_K2_LOAD;
        }
        if (value2 & (1U << 1)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_K2_ALWAYS_LOAD;
        }
        if (value1 & (1U << 0)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_K1_LOAD;
        }
        if (value2 & (1U << 0)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_DPLL_K1_ALWAYS_LOAD;
        }

        COMPILER_64_SET((*value), (0), (value32));
        break;
 
    case SOC_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT:
       
        /* This doesn't clear the interrupts. 
           To clear read the timestamp.
         */
        phy_bcm542xx_direct_reg_read(unit, pc, 
                             PHY_BCM542XX_TOP_1588_INT_STATUS_REG_OFFSET,
                             &value1);
        if (value1 & (1U << (dev_port + 1))) {
            value32 |= SOC_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT;
        }
        if (value1 & (1U << 0)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_FRAMESYNC_INTERRUPT;
        }
        COMPILER_64_SET((*value), (uint32)((value1 >> 1) & 0xff), (value32));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT_MASK:
       
        phy_bcm542xx_direct_reg_read(unit, pc, 
                             PHY_BCM542XX_TOP_1588_INT_MASK_REG_OFFSET,
                             &value1);
        if (value1 & (1U << (dev_port + 1))) {
            value32 |= SOC_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_MASK;
        }
        if (value1 & (1U << 0)) {
            value32 |= SOC_PORT_PHY_TIMESYNC_FRAMESYNC_INTERRUPT_MASK;
        }
        COMPILER_64_SET((*value), (0), (value32));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_TX_TIMESTAMP_OFFSET:

        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_TX_PORT_N_TS_OFFSET_LSB(dev_port), 
                         &value1);
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_TX_PORT_N_TS_OFFSET_MSB(dev_port), 
                         &value2);
        COMPILER_64_SET((*value), 0, (((value2 & 0xf) << 16) | value1)); 
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_RX_TIMESTAMP_OFFSET:
         phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_RX_PORT_N_TS_OFFSET_LSB(dev_port), 
                         &value1);
        phy_bcm542xx_direct_reg_read(unit, pc,
                         PHY_BCM542XX_TOP_1588_RX_PORT_N_TS_OFFSET_MSB(dev_port), 
                         &value2);
        COMPILER_64_SET((*value), 0, (((value2 & 0xf) << 16) | value1)); 
        break; 

    default:
       /***********************************************************************
        * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
        * To access TOP level registers use phy_id of port0. Now restore back 
        */
        pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
        return SOC_E_UNAVAIL;
        break;
    }

   /***********************************************************************
    * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
    * To access TOP level registers use phy_id of port0. Now restore back 
    */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
    
    return SOC_E_NONE;
}

STATIC int
_phy_phy542xx_inband_timesync_get_msg_type(uint16 value, soc_port_phy_timesync_msg_t *msg_type) {
    value = (value  >> PHY_BCM542XX_TOP_1588_TS_INFO_MSG_TYPE_SHIFT)
        & PHY_BCM542XX_TOP_1588_TS_INFO_MSG_TYPE;
    switch(value) {
        case 0:
            *msg_type = SOC_PORT_PHY_TIMESYNC_MSG_SYNC;
            break;
        case 1:
            *msg_type = SOC_PORT_PHY_TIMESYNC_MSG_DELAY_REQ;
            break;
        case 2:
            *msg_type = SOC_PORT_PHY_TIMESYNC_MSG_PDELAY_REQ;
            break;
        case 3:
            *msg_type = SOC_PORT_PHY_TIMESYNC_MSG_PDELAY_RESP;
            break;
        default:
            return SOC_E_UNAVAIL;
    }
    return SOC_E_NONE;
}

STATIC int
_phy_phy542xx_inband_timesync_get_protocol(uint16 value, soc_port_phy_timesync_prot_t *protocol) {
    value = (value  >> PHY_BCM542XX_TOP_1588_TS_INFO_DOMAIN_NUM_SHIFT)
        & PHY_BCM542XX_TOP_1588_TS_INFO_DOMAIN_NUM;
    switch(value) {
        case 0:
            *protocol = SOC_PORT_PHY_TIMESYNC_PROT_LAYER2;
            break;
        case 1:
            *protocol = SOC_PORT_PHY_TIMESYNC_PROT_IPV4_UDP;
            break;
        case 2:
            *protocol = SOC_PORT_PHY_TIMESYNC_PROT_IPV6_UDP;
            break;
        default:
            return SOC_E_UNAVAIL;
    }
    return SOC_E_NONE;
}

STATIC int
phy_bcm542xx_timesync_enhanced_capture_get(int unit, soc_port_t port, 
                            soc_port_phy_timesync_enhanced_capture_t *value) {
    uint16 dev_port, value0, value1, value2, value3;
    phy_ctrl_t *pc;
    uint16 data16 = 0;

    pc = EXT_PHY_SW_STATE(unit, port);

    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
     * To access TOP level registers use phy_id of port0. Override this ports 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);

    /* Dev port numbers from 0-7 */
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);

    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }

    /* MSG_TYPE, PROTOCOL, DIRECTION */
    phy_bcm542xx_direct_reg_modify(unit, pc, 
            PHY_BCM542XX_TOP_1588_CNTR_DBG_REG_OFFSET,  
            (dev_port << CNTR_DBG_TS_SLICE_SEL_SHIFT),
            PHY_BCM542XX_TOP_1588_CNTR_DBG_TS_SLICE_SEL);

    phy_bcm542xx_direct_reg_read(unit, pc, 
            PHY_BCM542XX_TOP_1588_TS_INFO_1_REG_OFFSET, 
            &value0);
    _phy_phy542xx_inband_timesync_get_msg_type(value0, &value->msg_type);
    _phy_phy542xx_inband_timesync_get_protocol(value0, &value->protocol);
    value->direction = ((value0 >> PHY_BCM542XX_TOP_1588_TS_INFO_DIR_SHIFT) 
                      & PHY_BCM542XX_TOP_1588_TS_INFO_DIR)  == 0 ? 
                      SOC_PORT_PHY_TIMESYNC_DIR_EGRESS : 
                      SOC_PORT_PHY_TIMESYNC_DIR_INGRESS;      

    /* SEQ_ID */
    phy_bcm542xx_direct_reg_read(unit, pc, 
            PHY_BCM542XX_TOP_1588_TS_INFO_2_REG_OFFSET, 
            &value0);
    value->seq_id = (value0 >> PHY_BCM542XX_TOP_1588_TS_INFO_SEQ_ID_SHIFT)
                      & PHY_BCM542XX_TOP_1588_TS_INFO_SEQ_ID;

    /* DOMAIN */

    /* TIMESTAMP */

    /* SRC PORT ID selected? */
    phy_bcm542xx_direct_reg_read(unit, pc,
            PHY_BCM542XX_TOP_1588_LABEL_N_MSB_MASK_REG_OFFSET(dev_port),
            &data16);

    if(data16 & PHY_BCM542XX_TOP_1588_CAP_SRC_PORT_CLK_ID) {
        phy_bcm542xx_direct_reg_read(unit, pc, 
                PHY_BCM542XX_TOP_1588_TS_INFO_3_REG_OFFSET, 
                &value0);
        value0 &= PHY_BCM542XX_TOP_1588_TS_INFO_SRC_PORT_CLK_ID_1;
        phy_bcm542xx_direct_reg_read(unit, pc, 
                PHY_BCM542XX_TOP_1588_TS_INFO_4_REG_OFFSET, 
                &value1);
        value1 &= PHY_BCM542XX_TOP_1588_TS_INFO_SRC_PORT_CLK_ID_2;
        phy_bcm542xx_direct_reg_read(unit, pc, 
                PHY_BCM542XX_TOP_1588_TS_INFO_5_REG_OFFSET, 
                &value2);
        value2 &= PHY_BCM542XX_TOP_1588_TS_INFO_SRC_PORT_CLK_ID_3;
        phy_bcm542xx_direct_reg_read(unit, pc, 
                PHY_BCM542XX_TOP_1588_TS_INFO_6_REG_OFFSET, 
                &value3);
        value3 &= PHY_BCM542XX_TOP_1588_TS_INFO_SRC_PORT_CLK_ID_4;
        COMPILER_64_SET((value->source_port_identity), 
                (value3 << 16) | value2, 
                (value1<< 16) | value0); 
    } else {
        COMPILER_64_SET((value->source_port_identity),0,0); 
    }


    /* SRC PORT */
    phy_bcm542xx_direct_reg_read(unit, pc, 
            PHY_BCM542XX_TOP_1588_TS_INFO_7_REG_OFFSET, 
            &value0);
    value->source_port &= PHY_BCM542XX_TOP_1588_TS_INFO_SRC_PORT_NUM;

    /* VLAN ID */
    phy_bcm542xx_direct_reg_read(unit, pc, 
            PHY_BCM542XX_TOP_1588_TS_INFO_8_REG_OFFSET, 
            &value0);
    value->vlan_id &= PHY_BCM542XX_TOP_1588_TS_INFO_VLAN_ID;

   /***********************************************************************
    * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
    * To access TOP level registers use phy_id of port0. Now restore back 
    */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
        
    return SOC_E_NONE;
}


/*
 * Function:
 *      phy_54280_cable_diag_dispatch
 * Purpose:
 *      Run 54280 cable diagnostics
 * Parameters:
 *      unit - device number
 *      port - port number
 *      status - (OUT) cable diagnotic status structure
 * Returns:     
 *      SOC_E_XXX
 */
STATIC int
phy_54280_cable_diag_dispatch(int unit, soc_port_t port,
                    soc_port_cable_diag_t *status)
{
 /*ECD_CTRL : break link */
    uint16 ecd_ctrl = 0x1000;
    
    
    SOC_IF_ERROR_RETURN(
        phy_ecd_cable_diag_init_40nm(unit, port));

    SOC_IF_ERROR_RETURN(
        phy_ecd_cable_diag_40nm(unit, port, status, ecd_ctrl));
    return SOC_E_NONE;
}

/*
 * Function:     
 *    phy_bcm542xx_clock_enable_set
 * Purpose:    
 * Parameters:
 * Returns:    
 */
int 
phy_bcm542xx_clock_enable_set(int unit, phy_ctrl_t *pc, int offset, uint32 value)
{
    uint16 data;    

    phy_bcm542xx_direct_reg_read(unit, pc, 
            PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_OFFSET, 
            &data);

    if ( value ) {
        phy_bcm542xx_direct_reg_write(unit, pc, 
                PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_OFFSET, 
                (( data & 0xf0 ) | ( offset & PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_REC_CLK1_SEL)));
    } else {
        phy_bcm542xx_direct_reg_write(unit, pc, 
                PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_OFFSET, 
                ( data & 0xf0 ) | PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_REC_CLK1_DIS );
    }
    
    return SOC_E_NONE;
    
}


/*
 * Function:     
 *    phy_bcm542xx_clock_enable_get
 * Purpose:    
 * Parameters:
 * Returns:    
 */
int 
phy_bcm542xx_clock_enable_get(int unit, phy_ctrl_t *pc, int offset,  uint32 *value)
{
    uint16 data;    
    *value = TRUE;

    phy_bcm542xx_direct_reg_read(unit, pc, PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_OFFSET, &data);
            
    if((data & PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_REC_CLK1_DIS) || 
        ((data & PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_REC_CLK1_SEL) != offset)) {

        *value = FALSE;
    }

    return SOC_E_NONE;
    
}

/*
 * Function:     
 *    phy_bcm542xx_clock_secondary_enable_set
 * Purpose:    
 * Parameters:
 * Returns:    
 */
int 
phy_bcm542xx_clock_secondary_enable_set(int unit, phy_ctrl_t *pc, int offset, uint32 value)
{
    uint16 data;    
    
    phy_bcm542xx_direct_reg_read(unit, pc, 
            PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_OFFSET, 
            &data);

    if ( value ) {
        phy_bcm542xx_direct_reg_write(unit, pc, 
                PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_OFFSET, 
                (( data & 0x0f ) | ( offset & PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_REC_CLK2_SEL)));
    } else {
        phy_bcm542xx_direct_reg_write(unit, pc, 
                PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_OFFSET, 
                ( data & 0x0f ) | PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_REC_CLK2_DIS );
    }
    
    return SOC_E_NONE;
    
}


/*
 * Function:     
 *    phy_bcm542xx_clock_secondary_enable_get
 * Purpose:    
 * Parameters:
 * Returns:    
 */
void 
phy_bcm542xx_clock_secondary_enable_get(int unit, phy_ctrl_t *pc,int offset, uint32 *value)
{
    uint16 data;    
    *value = TRUE;

    phy_bcm542xx_direct_reg_read(unit, pc, PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_OFFSET, &data);
            
    if((data & PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_REC_CLK2_DIS) || 
        ((data & PHY_BCM542XX_SYNCE_RECOVERY_CLK_REG_REC_CLK2_SEL) != offset)) {
    
        *value = FALSE;
    }
    
}



/*
 * Function:     
 *    phy_bcm542xx_power_mode_set
 * Purpose:    
 * Parameters:
 * Returns:    
 */
int
phy_bcm542xx_power_mode_set(int unit, phy_ctrl_t *pc, int mode)
{

    if (pc->power_mode == mode) {
        return SOC_E_NONE;
    }

    if (mode == SOC_PHY_CONTROL_POWER_LOW) {
        /* enable dsp clock */
        PHY_MODIFY_BCM542XX_MII_AUX_CTRLr(unit, pc, 
                            AUX_CTRL_REG_EN_DSP_CLK, 
                            AUX_CTRL_REG_EN_DSP_CLK);

        if (1)  /* CHECK if the interface is SGMII? */
        {
            /* Select mode: SGMII->Copper */
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                            PHY_BCM542XX_MODE_CONTROL_REG_OFFSET, 
                            PHY_BCM542XX_MODE_CONTROL_REG_MODE_SEL 
                                & MODE_SELECT_SGMII_2_COPPER, 
                            PHY_BCM542XX_MODE_CONTROL_REG_MODE_SEL);

           /* Power down SGMII Interface  */ 
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                            PHY_BCM542XX_SGMII_CONTROL_REG_OFFSET, 
                            PHY_BCM542XX_SGMII_CONTROL_REG_POWER_DOWN, 
                            PHY_BCM542XX_SGMII_CONTROL_REG_POWER_DOWN);

        }

        /* disable dsp clock */
        PHY_MODIFY_BCM542XX_MII_AUX_CTRLr(unit, pc, 0x00, AUX_CTRL_REG_EN_DSP_CLK);

    } else if (mode == SOC_PHY_CONTROL_POWER_FULL) {

         /* enable dsp clock */
         PHY_MODIFY_BCM542XX_MII_AUX_CTRLr(unit, pc, 
                              AUX_CTRL_REG_EN_DSP_CLK, 
                              AUX_CTRL_REG_EN_DSP_CLK);

         if (1)  /* CHECK if the interface is SGMII? */
        {
            /* Select mode: SGMII->Copper */
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                            PHY_BCM542XX_MODE_CONTROL_REG_OFFSET, 
                            PHY_BCM542XX_MODE_CONTROL_REG_MODE_SEL 
                               & MODE_SELECT_SGMII_2_COPPER, 
                            PHY_BCM542XX_MODE_CONTROL_REG_MODE_SEL);

           /* Normal operation mode - disable power down - SGMII Interface  */ 
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                            PHY_BCM542XX_SGMII_CONTROL_REG_OFFSET, 
                            0x00, 
                            PHY_BCM542XX_SGMII_CONTROL_REG_POWER_DOWN);

        }

         /* disable dsp clock */
         PHY_MODIFY_BCM542XX_MII_AUX_CTRLr(unit, pc, 0x00, AUX_CTRL_REG_EN_DSP_CLK);


        /* disable the auto power mode */
        phy_bcm542xx_direct_reg_modify( unit, pc, 
                        PHY_BCM542XX_POWER_AUTO_DOWN_REG_OFFSET, 
                        0x00, 
                        PHY_BCM542XX_POWER_AUTO_DOWN_REG_APD_MODE);
    } else if (mode == SOC_PHY_CONTROL_POWER_AUTO) {

         phy_bcm542xx_direct_reg_modify( unit, pc, 
                         PHY_BCM542XX_POWER_AUTO_DOWN_REG_OFFSET, 
                         PHY_BCM542XX_POWER_AUTO_DOWN_REG_APD_MODE, 
                         PHY_BCM542XX_POWER_AUTO_DOWN_REG_APD_MODE);
    } else if (mode == SOC_PHY_CONTROL_POWER_AUTO_DISABLE) {

         phy_bcm542xx_direct_reg_modify( unit, pc, 
                         PHY_BCM542XX_POWER_AUTO_DOWN_REG_OFFSET, 
                         0, 
                         PHY_BCM542XX_POWER_AUTO_DOWN_REG_APD_MODE);
    } else {
        return SOC_E_UNAVAIL;
    }

    pc->power_mode = mode;
    return SOC_E_NONE;
}

/*
 * Function:     
 *    phy_bcm542xx_power_auto_wake_time_set
 * Purpose:    
 * Parameters:
 * Returns:    
 */
int
phy_bcm542xx_power_auto_wake_time_set(int unit, phy_ctrl_t *pc, int value)
{
    value = (value <  PHY_BCM542XX_AUTO_PWRDWN_WAKEUP_MAX)? value : PHY_BCM542XX_AUTO_PWRDWN_WAKEUP_MAX;
            /* at least one unit */
    value = (value < PHY_BCM542XX_AUTO_PWRDWN_WAKEUP_UNIT)? PHY_BCM542XX_AUTO_PWRDWN_WAKEUP_UNIT : value;

    phy_bcm542xx_direct_reg_modify(unit, pc, 
             PHY_BCM542XX_POWER_AUTO_DOWN_REG_OFFSET, 
             PHY_BCM542XX_POWER_AUTO_DOWN_REG_WAKE_UP_TIMER_SEL 
               & (value/PHY_BCM542XX_AUTO_PWRDWN_WAKEUP_UNIT), 
             PHY_BCM542XX_POWER_AUTO_DOWN_REG_WAKE_UP_TIMER_SEL);

    return SOC_E_NONE;
    
}

/*
 * Function:     
 *    phy_bcm542xx_power_auto_wake_time_get
 * Purpose:    
 * Parameters:
 * Returns:    
 */
int
phy_bcm542xx_power_auto_wake_time_get(int unit, phy_ctrl_t *pc, uint32 *value)
{
    uint16 data;
    phy_bcm542xx_direct_reg_read(unit, pc, 
            PHY_BCM542XX_POWER_AUTO_DOWN_REG_OFFSET, &data);

    data &= PHY_BCM542XX_POWER_AUTO_DOWN_REG_WAKE_UP_TIMER_SEL;
    *value = data * PHY_BCM542XX_AUTO_PWRDWN_WAKEUP_UNIT;
    return SOC_E_NONE;
}


/*
 * Function:     
 *    phy_bcm542xx_power_auto_sleep_time_set
 * Purpose:    
 * Parameters:
 * Returns:    
 */
int
phy_bcm542xx_power_auto_sleep_time_set(int unit, phy_ctrl_t *pc, int value)
{
    /* sleep time configuration is either 2.7s or 5.4 s, default is 2.7s */
    value = (value < PHY_BCM542XX_AUTO_PWRDWN_SLEEP_MAX)? 0x00 : PHY_BCM542XX_POWER_AUTO_DOWN_REG_WAKE_UP_TIMER_SEL;
    phy_bcm542xx_direct_reg_modify( unit, pc, 
            PHY_BCM542XX_POWER_AUTO_DOWN_REG_OFFSET, 
            value, 
            PHY_BCM542XX_POWER_AUTO_DOWN_REG_SLEEP_TIMER_SEL);

    return SOC_E_NONE;
    
}


/*
 * Function:     
 *    phy_bcm542xx_power_auto_sleep_time_get
 * Purpose:    
 * Parameters:
 * Returns:    
 */
int
phy_bcm542xx_power_auto_sleep_time_get(int unit, phy_ctrl_t *pc, uint32 *value)
{

    uint16 data;
    phy_bcm542xx_direct_reg_read(unit, pc, 
            PHY_BCM542XX_POWER_AUTO_DOWN_REG_OFFSET, &data);
   
    *value = (data & PHY_BCM542XX_POWER_AUTO_DOWN_REG_SLEEP_TIMER_SEL)? PHY_BCM542XX_AUTO_PWRDWN_SLEEP_MAX : PHY_BCM542XX_AUTO_PWRDWN_SLEEP_MIN;

    return SOC_E_NONE;
    
}



/*
 * Function:
 *      phy_bcm542xx_eee_enable
 * Purpose:
 *      Enable or disable EEE (Native)
 * Parameters:
 *      unit   - StrataSwitch unit #.
 *      port   - StrataSwitch port #.
 *      enable - Enable Native EEE
 * Returns:
 *      SOC_E_NONE
 */
int
phy_bcm542xx_eee_enable(int unit, soc_port_t port, soc_port_ability_t *ability, int enable)
{
    int rv = SOC_E_NONE;
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);

    if(enable) {
    
        /* Enable EEE 
         *
         * 1. Enable EEE mode
         * 2. Advertise EEE auto-negotiation capabilities
         * < Normal auto-negotiation properties >
         * 3. Intiate auto-negotiation
         *
        */
          
        SOC_IF_ERROR_RETURN
           (PHY_MODIFY_BCM542XX_EEE_803Dr(unit, pc, 0xc000, 0xc000)); /* 7.803d */
        
        SOC_IF_ERROR_RETURN
           (PHY_MODIFY_BCM542XX_EEE_ADVr(unit, pc, 0x0006, 0x0006));

        phy_bcm542xx_autoneg_set(unit, port, 1);
        
        /* 
           Enable stat counters 
           Disable rollover
           Clear BIT14 to get lower 16bits of the counters 
        */
         SOC_IF_ERROR_RETURN(
                PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc, 
                          PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_EN
                          | PHY_BCM542XX_EEE_STAT_CTRL_SATURATE_MODE,
                          PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_EN
                          | PHY_BCM542XX_EEE_STAT_CTRL_SATURATE_MODE
                          | PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL));
        ability->eee |= (SOC_PA_EEE_1GB_BASET | SOC_PA_EEE_100MB_BASETX);


    } else {

        SOC_IF_ERROR_RETURN
           (PHY_MODIFY_BCM542XX_EEE_803Dr(unit, pc, 0x0000, 0xc000)); /* 7.803d */
        
        SOC_IF_ERROR_RETURN
           (PHY_MODIFY_BCM542XX_EEE_ADVr(unit, pc, 0x0000, 0x0006));

        phy_bcm542xx_autoneg_set(unit, port, 1);

        /* 
          Reset stat counters 
        */
        SOC_IF_ERROR_RETURN(
               PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc, 
                        PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_RESET,
                        PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_RESET));

        /* 
          Disable stat counters 
        */
        SOC_IF_ERROR_RETURN(
               PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc, 
                        0,
                        PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_RESET
                        | PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_EN));

        ability->eee &= ~(SOC_PA_EEE_1GB_BASET | SOC_PA_EEE_100MB_BASETX);

    }

    return rv;
}

int
phy_bcm542xx_eee_auto_enable(int unit, soc_port_t port, int enable)
{
    int rv = SOC_E_NONE;
    int dev_port;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* Dev port numbers from 0-7 */
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);

    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }
    
    if(enable) {
    
        /* Enable /disable AutogrEEEn 
         *
         * 1. Enable EEE mode
         * 2. Advertise EEE auto-negotiation capabilities
         * < Normal auto-negotiation properties >
         * < Enable optional variable/fixed latency modes >
         * 3. Intiate auto-negotiation
         * 4. Enable AutogrEEEn
         *
        */
          
        SOC_IF_ERROR_RETURN
           (PHY_MODIFY_BCM542XX_EEE_803Dr(unit, pc, 0xc000, 0xc000)); /* 7.803d */
        SOC_IF_ERROR_RETURN
           (PHY_MODIFY_BCM542XX_EEE_ADVr(unit, pc, 0x0006, 0x0006));
        phy_bcm542xx_autoneg_set(unit, port,  1);
        /***********************************************************************
         * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
         * To access TOP level registers use phy_id of port0. Override this ports 
         */
        pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
        
        phy_bcm542xx_direct_reg_modify(unit, pc, 
                         PHY_BCM542XX_TOP_MISC_MII_BUFF_CNTL_PHYN(dev_port),  
                         PHY_BCM542XX_TOP_MISC_MII_BUFF_EN, 
                         PHY_BCM542XX_TOP_MISC_MII_BUFF_EN);
        /***********************************************************************
         * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
         * To access TOP level registers use phy_id of port0. Now restore back 
         */
        pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);

        /* 
           Enable stat counters 
           Disable rollover
           Clear BIT14 to get lower 16bits of the counters 
        */
         SOC_IF_ERROR_RETURN(
                PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc, 
                          PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_EN
                          | PHY_BCM542XX_EEE_STAT_CTRL_SATURATE_MODE,
                          PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_EN
                          | PHY_BCM542XX_EEE_STAT_CTRL_SATURATE_MODE
                          | PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL));

    } else {

        SOC_IF_ERROR_RETURN
           (PHY_MODIFY_BCM542XX_EEE_803Dr(unit, pc, 0x0000, 0xc000)); /* 7.803d */
        
        SOC_IF_ERROR_RETURN
           (PHY_MODIFY_BCM542XX_EEE_ADVr(unit, pc, 0x0000, 0x0006));

        phy_bcm542xx_autoneg_set(unit, port, 1);

        /***********************************************************************
         * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
         * To access TOP level registers use phy_id of port0. Override this ports 
         */
        pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
        
        phy_bcm542xx_direct_reg_modify(unit, pc, 
                         PHY_BCM542XX_TOP_MISC_MII_BUFF_CNTL_PHYN(dev_port),  
                         0x00, 
                         PHY_BCM542XX_TOP_MISC_MII_BUFF_EN);
        /***********************************************************************
         * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
         * To access TOP level registers use phy_id of port0. Now restore back 
         */
        pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);

        /* 
           Reset stat counters 
        */
         SOC_IF_ERROR_RETURN(
                PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc, 
                         PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_RESET,
                         PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_RESET));
        /* 
           Disable stat counters 
        */
         SOC_IF_ERROR_RETURN(
                PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc, 
                         0,
                         PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_RESET
                         | PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_EN));

    }

    return rv;
}


int
phy_bcm542xx_get_eee_control_status(int unit, phy_ctrl_t *pc, uint32 *value)
{
    int rv = SOC_E_NONE;
    uint16 temp16 = 0;
    SOC_IF_ERROR_RETURN
                (PHY_READ_BCM542XX_EEE_803Dr(unit, pc, &temp16)); /* 7.803d */


    *value = ((temp16 & 0xC000) == 0xC000) ?  1 : 0; 
    return rv;
}
int
phy_bcm542xx_get_auto_eee_control_status(int unit, soc_port_t port, uint32 *value)
{
    int rv = SOC_E_NONE;
    uint16 temp16 = 0;
    int dev_port;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* Dev port numbers from 0-7 */
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);
    
    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }
    
    
    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
     * To access TOP level registers use phy_id of port0. Override this ports 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
    
    phy_bcm542xx_direct_reg_read(unit, pc, 
                     PHY_BCM542XX_TOP_MISC_MII_BUFF_CNTL_PHYN(dev_port), 
                     &temp16);

    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
     * To access TOP level registers use phy_id of port0. Now restore back 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);

    *value = (temp16 & PHY_BCM542XX_TOP_MISC_MII_BUFF_EN)? 1: 0;

    return rv;
}


/*
 * Function:     
 *    phy_bcm542xx_eee_control_set
 * Purpose:    
 *    Implements different 'setting' controls
 *    for EEE
 * Parameters:
 *    phy_id    - PHY's device address
 *    type      - type of control
 *    value     - helps decide what needs to be
 *                for the control type 'type'
 * Returns:    
 */
int
phy_bcm542xx_eee_control_set(int unit, soc_port_t port, soc_phy_control_t type, uint32 value)
{
    int rv = SOC_E_NONE;
    uint16 temp16 = 0;
    int dev_port;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* Dev port numbers from 0-7 */
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);
    
    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }
    

    switch(type) {

        case (SOC_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD):
            /***********************************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
             * To access TOP level registers use phy_id of port0. Override this ports 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
    
            phy_bcm542xx_direct_reg_modify(unit, pc,
                             PHY_BCM542XX_TOP_MISC_MII_BUFF_CNTL_PHYN(dev_port), 
                             value << 8,
                             PHY_BCM542XX_TOP_MISC_MII_BUFF_WAIT_IFG_LEN);
            /***********************************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
             * To access TOP level registers use phy_id of port0. Now restore back 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);

                                        
            break;

        case (SOC_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY):
            /***********************************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
             * To access TOP level registers use phy_id of port0. Override this ports 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
    
            temp16 = (value == 1)?  0: PHY_BCM542XX_TOP_MISC_MII_BUFF_VAR_LAT_EN;
            phy_bcm542xx_direct_reg_modify(unit, pc,
                             PHY_BCM542XX_TOP_MISC_MII_BUFF_CNTL_PHYN(dev_port), 
                             temp16,
                             PHY_BCM542XX_TOP_MISC_MII_BUFF_VAR_LAT_EN);
            /***********************************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
             * To access TOP level registers use phy_id of port0. Now restore back 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);

            /*
             * Need to set MII buffer register as well : phy54682 ?
            */       
            break;
        case (SOC_PHY_CONTROL_EEE_STATISTICS_CLEAR):
            if(1 == value) {
                /* Enable stat counters, Soft reset the counters  
                */
                SOC_IF_ERROR_RETURN(
                     PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc, 
                                PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_RESET
                                | PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_EN,
                                PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_RESET
                                | PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_EN));
                /*Clear the counter reset bit*/
                SOC_IF_ERROR_RETURN(
                     PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc, 
                                0x0,
                                PHY_BCM542XX_EEE_STAT_CTRL_COUNTERS_RESET));

            }
            break;  
        default:
            break;
    } 
    return rv;
}


/*
 * Function:     
 *    phy_bcm542xx_eee_control_get
 * Purpose:    
 *    Implements different 'get' controls 
 *    for EEE
 * Parameters:
 *    phy_id    - PHY's device address
 *    type      - type of control
 *    value     - gets the value for the 
 *                control 'type
 *                
 * Returns:    
 */
int
phy_bcm542xx_eee_control_get(int unit, soc_port_t port, soc_phy_control_t type, uint32 *value)
{
    int rv = SOC_E_NONE;
    uint16 temp16 = 0;
    int dev_port;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* Dev port numbers from 0-7 */
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);
    
    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }
    
    switch(type) {

        case (SOC_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD):
            /***********************************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
             * To access TOP level registers use phy_id of port0. Override this ports 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
    
            phy_bcm542xx_direct_reg_read(unit, pc,
                             PHY_BCM542XX_TOP_MISC_MII_BUFF_CNTL_PHYN(dev_port), 
                             &temp16); 
            /***********************************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
             * To access TOP level registers use phy_id of port0. Now restore back 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);

            *value = (temp16  & PHY_BCM542XX_TOP_MISC_MII_BUFF_WAIT_IFG_LEN) >> 8; 
            break;

        case (SOC_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY):
            /***********************************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
             * To access TOP level registers use phy_id of port0. Override this ports 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
    
            phy_bcm542xx_direct_reg_read(unit, pc,
                             PHY_BCM542XX_TOP_MISC_MII_BUFF_CNTL_PHYN(dev_port), 
                             &temp16);
            /***********************************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
             * To access TOP level registers use phy_id of port0. Now restore back 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);

            *value = temp16 & PHY_BCM542XX_TOP_MISC_MII_BUFF_VAR_LAT_EN;
            break;
        
        case SOC_PHY_CONTROL_EEE_TRANSMIT_EVENTS:
            /* Lower 16 bits */
            SOC_IF_ERROR_RETURN(
                     PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc,
                                0,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL)); 
            SOC_IF_ERROR_RETURN(       
                PHY_READ_BCM542XX_EEE_STAT_TX_EVENTSr(unit, pc,
                              &temp16));
            *value = (temp16); 
            /* Upper 16 bits */
            SOC_IF_ERROR_RETURN(
                     PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL)); 
            SOC_IF_ERROR_RETURN(       
                PHY_READ_BCM542XX_EEE_STAT_TX_EVENTSr(unit, pc,
                              &temp16));
            *value |= ((temp16) << 16); 
            break;
        
        case SOC_PHY_CONTROL_EEE_TRANSMIT_DURATION:
            /* Lower 16 bits */
            SOC_IF_ERROR_RETURN(
                     PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc,
                                0,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL)); 
            SOC_IF_ERROR_RETURN(       
                PHY_READ_BCM542XX_EEE_STAT_TX_DURATIONr(unit, pc,
                              &temp16));            
            *value = (temp16); 
            /* Upper 16 bits */
            SOC_IF_ERROR_RETURN(
                     PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL)); 
            SOC_IF_ERROR_RETURN(       
                PHY_READ_BCM542XX_EEE_STAT_TX_DURATIONr(unit, pc,
                              &temp16));
            *value |= ((temp16) << 16); 
            break;

        case SOC_PHY_CONTROL_EEE_RECEIVE_EVENTS:
            /* Lower 16 bits */
            SOC_IF_ERROR_RETURN(
                     PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc,
                                0,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL)); 
            SOC_IF_ERROR_RETURN(       
                PHY_READ_BCM542XX_EEE_STAT_RX_EVENTSr(unit, pc,
                              &temp16));            
            *value = (temp16); 
            /* Upper 16 bits */
            SOC_IF_ERROR_RETURN(
                     PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL)); 
            SOC_IF_ERROR_RETURN(       
                PHY_READ_BCM542XX_EEE_STAT_RX_EVENTSr(unit, pc,
                              &temp16));
            *value |= ((temp16) << 16); 
            break;
    
        case SOC_PHY_CONTROL_EEE_RECEIVE_DURATION:
            /* Lower 16 bits */
            SOC_IF_ERROR_RETURN(
                     PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc,
                                0,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL)); 
            SOC_IF_ERROR_RETURN(       
                PHY_READ_BCM542XX_EEE_STAT_RX_DURATIONr(unit, pc,
                              &temp16));            
            *value = (temp16); 
            /* Upper 16 bits */
            SOC_IF_ERROR_RETURN(
                     PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(unit, pc,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL,
                                PHY_BCM542XX_EEE_STAT_CTRL_16U_BIT_SEL)); 
            SOC_IF_ERROR_RETURN(       
                PHY_READ_BCM542XX_EEE_STAT_RX_DURATIONr(unit, pc,
                              &temp16));
            *value |= ((temp16) << 16); 
            break;

        default:
            break;
    }
    return rv;
}

/*
 * Function:
 *      phy_bcm542xx_control_set
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
phy_bcm542xx_control_set(int unit, soc_port_t port,
                     soc_phy_control_t type, uint32 value)
{
    int rv = SOC_E_NONE;
    phy_ctrl_t *pc;
    soc_phy_config_t phy_config;
   
    pc = EXT_PHY_SW_STATE(unit, port);

    switch(type) {

/* CLOCK CONTROLS  */
        case SOC_PHY_CONTROL_CLOCK_ENABLE:
            phy_bcm542xx_clock_enable_set(unit, pc, (int)PHY_BCM542XX_DEV_PHY_SLICE(pc), value);
            break;    

        case SOC_PHY_CONTROL_CLOCK_SECONDARY_ENABLE:
            phy_bcm542xx_clock_secondary_enable_set(unit, pc, (int)PHY_BCM542XX_DEV_PHY_SLICE(pc), value);
            break;

        case SOC_PHY_CONTROL_CLOCK_FREQUENCY:
            return SOC_E_UNAVAIL;
            break;
   
        case SOC_PHY_CONTROL_POWER:
             rv = phy_bcm542xx_power_mode_set(unit, pc, value);
            break;

        case SOC_PHY_CONTROL_POWER_AUTO_WAKE_TIME:
            phy_bcm542xx_power_auto_wake_time_set(unit, pc, value);
            break;

        case SOC_PHY_CONTROL_POWER_AUTO_SLEEP_TIME:
            phy_bcm542xx_power_auto_sleep_time_set(unit, pc, value);
            break;

         case SOC_PHY_CONTROL_PORT_PRIMARY:
            SOC_IF_ERROR_RETURN
                (soc_phyctrl_primary_set(unit, port, (soc_port_t)value));
        break;

        case SOC_PHY_CONTROL_PORT_OFFSET:
            SOC_IF_ERROR_RETURN
                (soc_phyctrl_offset_set(unit, port, (int)value));
        break;

    case SOC_PHY_CONTROL_EEE:
        phy_config = pc->copper;
        if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            rv = SOC_E_UNAVAIL;
            break;
        }
        if(PHY_FIBER_MODE(unit, port)) {
            phy_config = pc->fiber;
        }
        if (value == 1) {
            /* Enable EEE */
            SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_enable(unit, port,  
                                      &phy_config.advert_ability, 1));

        } else {
            /* Disable EEE */
            SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_enable(unit, port, 
                                      &phy_config.advert_ability, 0)); 
        }

        break;

    case SOC_PHY_CONTROL_EEE_AUTO:
        if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            rv = SOC_E_UNAVAIL;
            break;
        }
        if(PHY_FIBER_MODE(unit, port)) {
            phy_config = pc->fiber;
        }
        if (value == 1) {
            /* Enable AutoGrEEEn */
            SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_auto_enable(unit, port, 1));

        } else {
            /* Disable AutoGrEEEn */
            SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_auto_enable(unit, port, 0));
        }

        break;

    case SOC_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD:
        if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            rv = SOC_E_UNAVAIL;
            break;
        }
        SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_control_set(unit, port, 
                               SOC_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD,
                               value));
        break;       

    case SOC_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY:
        if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            rv = SOC_E_UNAVAIL;
            break;
        }
        SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_control_set(unit, port, 
                               SOC_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY, 
                               value));
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
        return SOC_E_UNAVAIL;
    case SOC_PHY_CONTROL_EEE_STATISTICS_CLEAR:
        if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            rv = SOC_E_UNAVAIL;
            break;
        }
         SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_control_set(unit, port, 
                               SOC_PHY_CONTROL_EEE_STATISTICS_CLEAR, 
                               value));

        break;
    default:
        return SOC_E_UNAVAIL;
    }

    if (rv != SOC_E_NONE) {
        return SOC_FAILURE(rv);
    }
    return rv;
}

/*
 * Function:
 *      phy_bcm542xx_control_get
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
phy_bcm542xx_control_get(int unit, soc_port_t port,
                     soc_phy_control_t type, uint32 *value)
{
    int rv;
    phy_ctrl_t    *pc;
    soc_port_t primary;

    pc = EXT_PHY_SW_STATE(unit, port);
    
    if (NULL == value) {
        return SOC_E_PARAM;
    }

    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return SOC_E_PARAM;
    }
    rv = SOC_E_NONE;
    switch(type) {
        case SOC_PHY_CONTROL_POWER:
            *value = pc->power_mode;
            break;

        case SOC_PHY_CONTROL_POWER_AUTO_SLEEP_TIME:
            phy_bcm542xx_power_auto_sleep_time_get(unit, pc, value); 
            break;
        
        case SOC_PHY_CONTROL_POWER_AUTO_WAKE_TIME:
            phy_bcm542xx_power_auto_wake_time_get(unit, pc, value); 
            break;

        case SOC_PHY_CONTROL_PORT_PRIMARY:
            SOC_IF_ERROR_RETURN
                (soc_phyctrl_primary_get(unit, port, &primary));
            *value = (uint32) primary;
            break;

        case SOC_PHY_CONTROL_PORT_OFFSET:
            *value = (uint32) PHY_BCM542XX_DEV_PHY_SLICE(pc);
            break;

        case SOC_PHY_CONTROL_CLOCK_ENABLE:
            phy_bcm542xx_clock_enable_get(unit, pc, (int)PHY_BCM542XX_DEV_PHY_SLICE(pc), value);
            break;

        case SOC_PHY_CONTROL_CLOCK_SECONDARY_ENABLE:
            phy_bcm542xx_clock_secondary_enable_get(unit, pc, (int)PHY_BCM542XX_DEV_PHY_SLICE(pc), value); 
            break;

        case SOC_PHY_CONTROL_EEE:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
                break;
            }
            phy_bcm542xx_get_eee_control_status(unit, pc, value);
        break;
        
        case SOC_PHY_CONTROL_EEE_AUTO:
            if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
                rv = SOC_E_UNAVAIL;
                break;
            }
            phy_bcm542xx_get_auto_eee_control_status(unit, port, value);
        break;

        case SOC_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY:
        if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            rv = SOC_E_UNAVAIL;
            break;
        }
        SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_control_get(unit, port,
                               SOC_PHY_CONTROL_EEE_AUTO_FIXED_LATENCY,  
                               value));

        break;
        
        case SOC_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD:
        if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            rv = SOC_E_UNAVAIL;
            break;
        }
        SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_control_get(unit, port,
                              SOC_PHY_CONTROL_EEE_AUTO_IDLE_THRESHOLD,  
                               value));

        break;
        
        case SOC_PHY_CONTROL_EEE_TRANSMIT_EVENTS:
        if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            rv = SOC_E_UNAVAIL;
            break;
        }
        SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_control_get(unit, port,
                               SOC_PHY_CONTROL_EEE_TRANSMIT_EVENTS,  
                               value));

        break;
        
        case SOC_PHY_CONTROL_EEE_TRANSMIT_DURATION:
        if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            rv = SOC_E_UNAVAIL;
            break;
        }
        SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_control_get(unit, port,
                               SOC_PHY_CONTROL_EEE_TRANSMIT_DURATION,  
                               value));

        break;

        case SOC_PHY_CONTROL_EEE_RECEIVE_EVENTS:
        if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            rv = SOC_E_UNAVAIL;
            break;
        }
        SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_control_get(unit, port,
                               SOC_PHY_CONTROL_EEE_RECEIVE_EVENTS,  
                               value));

        break;
    
        case SOC_PHY_CONTROL_EEE_RECEIVE_DURATION:
        if (!PHY_FLAGS_TST(unit, port, PHY_FLAGS_EEE_CAPABLE)) {
            rv = SOC_E_UNAVAIL;
            break;
        }
        SOC_IF_ERROR_RETURN
                (phy_bcm542xx_eee_control_get(unit, port,
                               SOC_PHY_CONTROL_EEE_RECEIVE_DURATION,  
                               value));

        break;

        default:
            return SOC_E_UNAVAIL;    
        }
    return rv;
}

STATIC int 
phy_bcm542xx_probe(int unit, phy_ctrl_t *pc) {
    int oui = 0, model = 0, rev = 0;
    int rv = SOC_E_NONE;
    soc_phy_info_t *pi;

    pi = &SOC_PHY_INFO(unit, pc->port);

    rv = _phy_bcm542xx_get_model_rev(unit, pc, &oui, &model, &rev);
    if (rv != SOC_E_NONE) {
        return SOC_E_FAIL;
    }

    switch(model) {
        case PHY_BCM54280_MODEL:
            /*passthru*/
        case PHY_BCM5428X_MODEL:
            /*passthru*/
        case PHY_BCM54240_MODEL:
            break;
        case (PHY_BCM54282_MODEL | PHY_BCM54285_MODEL):
            if(rev & 0x8) { /*PHY_BCM54282_MODEL*/
                pi->phy_name = "BCM54282"; 
            } else { /*PHY_BCM54285_MODEL*/
                pi->phy_name = "BCM54285"; 
            }
            break;
        case (PHY_BCM54290_MODEL | PHY_BCM54294_MODEL):
            pi->phy_name = "BCM54290";
            if(rev & 0x8) { /*PHY_BCM54294_MODEL*/
                pi->phy_name = "BCM54294";
            } 
            break;
        case (PHY_BCM54292_MODEL | PHY_BCM54295_MODEL):
            pi->phy_name = "BCM54292";
            if(rev & 0x8) { /*PHY_BCM54295_MODEL*/
                pi->phy_name = "BCM54295";
            } 
            break;
        default :
            return SOC_E_NOT_FOUND;
    }

    pc->size = sizeof(PHY_BCM542XX_DEV_DESC_t);
    return rv;

}


STATIC int
phy_bcm542xx_link_up(int unit, soc_port_t port)
{
    int speed = 0;

    SOC_IF_ERROR_RETURN
        (phy_bcm542xx_speed_get(unit, port, &speed));
    
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

/*
 * Function:
 *    phy_bcm542xx_oam_config_set
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
int
phy_bcm542xx_oam_config_set(int unit, soc_port_t port, soc_port_config_phy_oam_t *conf) {
    uint16  data = 0, mask = 0;
    uint16 dm_tx_cntl_data = 0, dm_rx_cntl_data = 0;
    phy_ctrl_t *pc;
    soc_port_phy_timesync_config_t timesync_conf;
    
    sal_memset(&timesync_conf, 0, sizeof(soc_port_phy_timesync_config_t));

    if(NULL == conf) {
        return SOC_E_PARAM;
    }
   
    pc = EXT_PHY_SW_STATE(unit, port);

    /* 1. en_1588_softrst */ 
    /* 2. en_RGMII_Timing_RGMII */
    
    /* Setup timesync 1588 */ 
    timesync_conf.flags = SOC_PORT_PHY_TIMESYNC_ENABLE;
    phy_bcm542xx_timesync_config_set(unit, port, &timesync_conf);
    
    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
     * To access TOP level registers use phy_id of port0. Override this ports 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
  
    /* TX: Delay measurement mode */
    switch(conf->tx_dm_config.mode) {
        case SOC_PORT_CONFIG_PHY_OAM_DM_Y1731:
            dm_tx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_TX_CNTL_EN;
            if(conf->tx_dm_config.flags & SOC_PORT_PHY_OAM_DM_TS_FORMAT) {
                dm_tx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_TX_CNTL_Y1731_TS_SEL;
            }   
            break;
        case SOC_PORT_CONFIG_PHY_OAM_DM_BHH:
            data = 0;
            mask = 0;
            if(!(conf->tx_dm_config.flags & SOC_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE)) {
                data |= PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_CW_DIS;
            }    
            if(conf->tx_dm_config.flags & SOC_PORT_PHY_OAM_DM_ENTROPY_ENABLE) {
                data |= PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_ENTROPY_EN;
            }   
            data |=  PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_EN;
            mask =  PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_EN
                    | PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_CW_DIS
                    | PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_ENTROPY_EN;
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                    PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_OFFSET,
                    data, mask);

            if(conf->tx_dm_config.flags & SOC_PORT_PHY_OAM_DM_TS_FORMAT) {
                dm_tx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_TX_CNTL_BHH_TS_SEL;
            }   
            break;
        case SOC_PORT_CONFIG_PHY_OAM_DM_IETF:
            data = 0;
            mask = 0;
            if(!(conf->tx_dm_config.flags & SOC_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE)) {
                data |= PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_CW_DIS;
            }    
            if(conf->tx_dm_config.flags & SOC_PORT_PHY_OAM_DM_ENTROPY_ENABLE) {
                data |= PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_ENTROPY_EN;
            }   
            data |=  PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_EN;
            mask =  PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_EN
                    | PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_CW_DIS
                    | PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_ENTROPY_EN;
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                    PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_OFFSET,
                    data, mask);

            dm_tx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_TX_CNTL_IETF_EN;
            if(conf->tx_dm_config.flags & SOC_PORT_PHY_OAM_DM_TS_FORMAT) {
                /* Same bit as for BHH */
                dm_tx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_TX_CNTL_BHH_TS_SEL;
            }   
            break;
        default:
            if(conf->tx_dm_config.mode == 0) {
                break;
            }
            /********************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
             * To access TOP level registers use phy_id of port0. 
               Now restore back 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
            return SOC_E_CONFIG;
    }    
    /* RX: Delay measurement mode */
    switch(conf->rx_dm_config.mode) {
        case SOC_PORT_CONFIG_PHY_OAM_DM_Y1731:
            dm_rx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_RX_CNTL_EN;
            if(conf->rx_dm_config.flags & SOC_PORT_PHY_OAM_DM_TS_FORMAT) {
                dm_rx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_RX_CNTL_Y1731_TS_SEL;
            }   
            break;
        case SOC_PORT_CONFIG_PHY_OAM_DM_BHH:
            data = 0;
            mask = 0;
            if(!(conf->rx_dm_config.flags & SOC_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE)) {
                data |= PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_CW_DIS;
            }    
            if(conf->rx_dm_config.flags & SOC_PORT_PHY_OAM_DM_ENTROPY_ENABLE) {
                data |= PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_ENTROPY_EN;
            }   
            data |=  PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_EN;
            mask =  PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_EN
                    | PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_CW_DIS
                    | PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_ENTROPY_EN;
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                    PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_OFFSET,
                    data, mask);
            
            if(conf->rx_dm_config.flags & SOC_PORT_PHY_OAM_DM_TS_FORMAT) {
                dm_rx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_RX_CNTL_BHH_TS_SEL;
            }   
            break;
        case SOC_PORT_CONFIG_PHY_OAM_DM_IETF:
            data = 0;
            mask = 0;
            if(!(conf->rx_dm_config.flags & SOC_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE)) {
                data |= PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_CW_DIS;
            }    
            if(conf->rx_dm_config.flags & SOC_PORT_PHY_OAM_DM_ENTROPY_ENABLE) {
                data |= PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_ENTROPY_EN;
            }   
            data |=  PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_EN;
            mask =  PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_EN
                    | PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_CW_DIS
                    | PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_ENTROPY_EN;
            phy_bcm542xx_direct_reg_modify(unit, pc, 
                    PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_OFFSET,
                    data, mask);

            dm_rx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_RX_CNTL_IETF_EN;
            if(conf->rx_dm_config.flags & SOC_PORT_PHY_OAM_DM_TS_FORMAT) {
                /* Same bit as for BHH */
                dm_rx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_RX_CNTL_BHH_TS_SEL;
            }   
            break;
        default:
            if(conf->rx_dm_config.mode == 0) {
                break;
            }
            /********************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
             * To access TOP level registers use phy_id of port0. 
               Now restore back 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
            return SOC_E_CONFIG;
    }    

    /* Enable the 1588 counter includes the delay measurement packet */   
   dm_tx_cntl_data |= (dm_tx_cntl_data)? PHY_BCM542XX_TOP_1588_DM_TX_CNTL_1588_CNT_EN : dm_tx_cntl_data;
   dm_rx_cntl_data |= (dm_rx_cntl_data)? PHY_BCM542XX_TOP_1588_DM_TX_CNTL_1588_CNT_EN : dm_rx_cntl_data;

   if(conf->tx_dm_config.flags & SOC_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE) {
       dm_tx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_TX_CNTL_MAC_EN;
   }    
   if(conf->tx_dm_config.flags & SOC_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE) {
       dm_tx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_TX_CNTL_CW_EN;
   }   
   if(conf->tx_dm_config.flags & SOC_PORT_PHY_OAM_DM_ENTROPY_ENABLE) {
       dm_tx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_TX_CNTL_ENTROPY_EN;
   }   

   if(conf->rx_dm_config.flags & SOC_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE) {
       dm_rx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_RX_CNTL_MAC_EN;
   }    
   if(conf->rx_dm_config.flags & SOC_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE) {
       dm_rx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_RX_CNTL_CW_EN;
   }   
   if(conf->rx_dm_config.flags & SOC_PORT_PHY_OAM_DM_ENTROPY_ENABLE) {
       dm_rx_cntl_data |= PHY_BCM542XX_TOP_1588_DM_RX_CNTL_ENTROPY_EN;
   }   

   mask = PHY_BCM542XX_TOP_1588_DM_TX_CNTL_EN 
          | PHY_BCM542XX_TOP_1588_DM_TX_CNTL_MAC_EN 
          | PHY_BCM542XX_TOP_1588_DM_TX_CNTL_CW_EN 
          | PHY_BCM542XX_TOP_1588_DM_TX_CNTL_ENTROPY_EN 
          | PHY_BCM542XX_TOP_1588_DM_TX_CNTL_Y1731_TS_SEL 
          | PHY_BCM542XX_TOP_1588_DM_TX_CNTL_BHH_TS_SEL  
          | PHY_BCM542XX_TOP_1588_DM_TX_CNTL_IETF_EN 
          | PHY_BCM542XX_TOP_1588_DM_TX_CNTL_1588_CNT_EN;

   phy_bcm542xx_direct_reg_modify(unit, pc, 
           PHY_BCM542XX_TOP_1588_DM_TX_CNTL_REG_OFFSET,
           dm_tx_cntl_data, mask);
   
   mask = PHY_BCM542XX_TOP_1588_DM_RX_CNTL_EN 
          | PHY_BCM542XX_TOP_1588_DM_RX_CNTL_MAC_EN 
          | PHY_BCM542XX_TOP_1588_DM_RX_CNTL_CW_EN 
          | PHY_BCM542XX_TOP_1588_DM_RX_CNTL_ENTROPY_EN 
          | PHY_BCM542XX_TOP_1588_DM_RX_CNTL_Y1731_TS_SEL 
          | PHY_BCM542XX_TOP_1588_DM_RX_CNTL_BHH_TS_SEL  
          | PHY_BCM542XX_TOP_1588_DM_RX_CNTL_IETF_EN 
          | PHY_BCM542XX_TOP_1588_DM_RX_CNTL_1588_CNT_EN;
   
   phy_bcm542xx_direct_reg_modify(unit, pc, 
           PHY_BCM542XX_TOP_1588_DM_RX_CNTL_REG_OFFSET,
           dm_rx_cntl_data, mask);

    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
     * To access TOP level registers use phy_id of port0. Now restore back 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);

    /* Save the config */
    sal_memcpy(PHY_BCM542XX_DEV_OAM_CONFIG_PTR(pc), conf, sizeof(soc_port_config_phy_oam_t));


    return SOC_E_NONE;

}


/*
 * Function:
 *    phy_bcm542xx_oam_config_get
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
int
phy_bcm542xx_oam_config_get(int unit, soc_port_t port, soc_port_config_phy_oam_t *conf) {
    uint16 dm_tx_cntl_data = 0, dm_rx_cntl_data = 0;
    uint16 mpls_tx_cntl_data = 0, mpls_rx_cntl_data = 0;
    phy_ctrl_t *pc;
   
    sal_memset(conf, 0, sizeof(soc_port_config_phy_oam_t));

    pc = EXT_PHY_SW_STATE(unit, port);


    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
     * To access TOP level registers use phy_id of port0. Override this ports 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);
   
   /* Read config registers */ 
   phy_bcm542xx_direct_reg_read(unit, pc, 
           PHY_BCM542XX_TOP_1588_DM_TX_CNTL_REG_OFFSET,
           &dm_tx_cntl_data);
   phy_bcm542xx_direct_reg_read(unit, pc, 
           PHY_BCM542XX_TOP_1588_DM_RX_CNTL_REG_OFFSET,
           &dm_rx_cntl_data);
    phy_bcm542xx_direct_reg_read(unit, pc, 
            PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_OFFSET,
            &mpls_tx_cntl_data);
    phy_bcm542xx_direct_reg_read(unit, pc, 
            PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_OFFSET,
            &mpls_rx_cntl_data);

    /* Tx Config Mode */
    if(dm_tx_cntl_data & PHY_BCM542XX_TOP_1588_DM_TX_CNTL_EN) {
        conf->tx_dm_config.mode = SOC_PORT_CONFIG_PHY_OAM_DM_Y1731;
        if(dm_tx_cntl_data & PHY_BCM542XX_TOP_1588_DM_TX_CNTL_Y1731_TS_SEL) {
            conf->tx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_TS_FORMAT;
        }     
    } else if((dm_tx_cntl_data & PHY_BCM542XX_TOP_1588_DM_TX_CNTL_IETF_EN) 
            && (mpls_tx_cntl_data & PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_EN)){
        conf->tx_dm_config.mode = SOC_PORT_CONFIG_PHY_OAM_DM_IETF;
        if(dm_tx_cntl_data & PHY_BCM542XX_TOP_1588_DM_TX_CNTL_BHH_TS_SEL) {
            conf->tx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_TS_FORMAT;
        }     
    } else if(mpls_tx_cntl_data & PHY_BCM542XX_TOP_1588_MPLS_TX_CNTL_REG_MPLS_EN) {
        conf->tx_dm_config.mode = SOC_PORT_CONFIG_PHY_OAM_DM_BHH;
        if(dm_tx_cntl_data & PHY_BCM542XX_TOP_1588_DM_TX_CNTL_BHH_TS_SEL) {
            conf->tx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_TS_FORMAT;
        }     
    }

    /* Rx Config Mode */
    if(dm_rx_cntl_data & PHY_BCM542XX_TOP_1588_DM_RX_CNTL_EN) {
        conf->rx_dm_config.mode = SOC_PORT_CONFIG_PHY_OAM_DM_Y1731;
        if(dm_rx_cntl_data & PHY_BCM542XX_TOP_1588_DM_TX_CNTL_Y1731_TS_SEL) {
            conf->rx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_TS_FORMAT;
        }     
    } else if((dm_rx_cntl_data & PHY_BCM542XX_TOP_1588_DM_RX_CNTL_IETF_EN) 
            && (mpls_rx_cntl_data & PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_EN)){
        conf->rx_dm_config.mode = SOC_PORT_CONFIG_PHY_OAM_DM_IETF;
        if(dm_rx_cntl_data & PHY_BCM542XX_TOP_1588_DM_TX_CNTL_BHH_TS_SEL) {
            conf->rx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_TS_FORMAT;
        }     
    } else if(mpls_rx_cntl_data & PHY_BCM542XX_TOP_1588_MPLS_RX_CNTL_REG_MPLS_EN) {
        conf->rx_dm_config.mode = SOC_PORT_CONFIG_PHY_OAM_DM_BHH;
        if(dm_rx_cntl_data & PHY_BCM542XX_TOP_1588_DM_TX_CNTL_BHH_TS_SEL) {
            conf->rx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_TS_FORMAT;
        }     
    }
    
    /* Tx Config Flags */
    if(dm_tx_cntl_data & PHY_BCM542XX_TOP_1588_DM_TX_CNTL_MAC_EN) {
        conf->tx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE;
    }    
    if(dm_tx_cntl_data & PHY_BCM542XX_TOP_1588_DM_TX_CNTL_CW_EN) {
        conf->tx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE; 
    }     
    if(dm_tx_cntl_data & PHY_BCM542XX_TOP_1588_DM_TX_CNTL_ENTROPY_EN) {
        conf->tx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_ENTROPY_ENABLE;
    }     

    /* Rx Config Flags */
    if(dm_rx_cntl_data & PHY_BCM542XX_TOP_1588_DM_RX_CNTL_MAC_EN) {
        conf->rx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_MAC_CHECK_ENABLE;
    }    
    if(dm_rx_cntl_data & PHY_BCM542XX_TOP_1588_DM_RX_CNTL_CW_EN) {
        conf->rx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_CONTROL_WORD_ENABLE; 
    }     
    if(dm_rx_cntl_data & PHY_BCM542XX_TOP_1588_DM_RX_CNTL_ENTROPY_EN) {
        conf->rx_dm_config.flags |= SOC_PORT_PHY_OAM_DM_ENTROPY_ENABLE;
    }     

    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
     * To access TOP level registers use phy_id of port0. Now restore back 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
    
    /* Save the config */
    sal_memcpy(PHY_BCM542XX_DEV_OAM_CONFIG_PTR(pc), conf, sizeof(soc_port_config_phy_oam_t));
    return SOC_E_NONE;
}


/*
 * Function:
 *    phy_bcm542xx_oam_control_set
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
int
phy_bcm542xx_oam_control_set(int unit, soc_port_t port, soc_port_control_phy_oam_t type, uint64 value)
{
    uint16  dev_port = 0;
    phy_ctrl_t *pc = NULL;
    soc_port_config_phy_oam_t *config = NULL;
    soc_port_config_phy_oam_dm_mode_t tx_mode = 0;
    soc_port_config_phy_oam_dm_mode_t rx_mode = 0;
    uint32 tx_flags;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* Dev port numbers from 0-7 */
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);
    
    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }
    
   
    /* Config saved during config _set or read during config _get */ 
    config = PHY_BCM542XX_DEV_OAM_CONFIG_PTR(pc); 
    tx_mode = config->tx_dm_config.mode;
    tx_flags = config->tx_dm_config.flags;
    rx_mode = config->rx_dm_config.mode;
    
    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
     * To access TOP level registers use phy_id of port0. Override this ports 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);

    switch(type) {
        case SOC_PORT_CONTROL_PHY_OAM_DM_TX_ETHERTYPE:
            switch(tx_mode) {
                case SOC_PORT_CONFIG_PHY_OAM_DM_Y1731:
                    if(tx_flags & SOC_PORT_PHY_OAM_DM_TS_FORMAT) { /*NTP */
                        phy_bcm542xx_direct_reg_write(unit, pc, 
                                PHY_BCM542XX_TOP_1588_DM_ETHTYPE2_REG_OFFSET,
                                (COMPILER_64_LO(value) & 0xffff));
                    } else { /*PTP */
                        phy_bcm542xx_direct_reg_write(unit, pc, 
                                PHY_BCM542XX_TOP_1588_DM_ETHTYPE1_REG_OFFSET,
                                (COMPILER_64_LO(value) & 0xffff));
                    }
                    break;
                case SOC_PORT_CONFIG_PHY_OAM_DM_BHH:
                    if(tx_flags & SOC_PORT_PHY_OAM_DM_TS_FORMAT) { /*NTP */
                        phy_bcm542xx_direct_reg_write(unit, pc, 
                                PHY_BCM542XX_TOP_1588_DM_ETHTYPE4_REG_OFFSET,
                                (COMPILER_64_LO(value) & 0xffff));
                    } else { /*PTP */
                        phy_bcm542xx_direct_reg_write(unit, pc, 
                                PHY_BCM542XX_TOP_1588_DM_ETHTYPE3_REG_OFFSET,
                                (COMPILER_64_LO(value) & 0xffff));
                    }
                    break;
                case SOC_PORT_CONFIG_PHY_OAM_DM_IETF:    
                    phy_bcm542xx_direct_reg_write(unit, pc, 
                            PHY_BCM542XX_TOP_1588_DM_ETHTYPE9_REG_OFFSET,
                            (COMPILER_64_LO(value) & 0xffff));
                    break;
                default:
                    /********************************************************
                     * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
                     * To access TOP level registers use phy_id of port0. 
                     Now restore back 
                     */
                    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
                    return SOC_E_CONFIG;    
            }

            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_RX_ETHERTYPE:
            switch(rx_mode) {
                case SOC_PORT_CONFIG_PHY_OAM_DM_Y1731:
                    phy_bcm542xx_direct_reg_write(unit, pc, 
                            PHY_BCM542XX_TOP_1588_DM_ETHTYPE5_REG_OFFSET,
                            (COMPILER_64_LO(value) & 0xffff));
                    break;
                case SOC_PORT_CONFIG_PHY_OAM_DM_BHH:
                    phy_bcm542xx_direct_reg_write(unit, pc, 
                            PHY_BCM542XX_TOP_1588_DM_ETHTYPE8_REG_OFFSET,
                            (COMPILER_64_LO(value) & 0xffff));
                    break;
                case SOC_PORT_CONFIG_PHY_OAM_DM_IETF:    
                    phy_bcm542xx_direct_reg_write(unit, pc, 
                            PHY_BCM542XX_TOP_1588_DM_ETHTYPE10_REG_OFFSET,
                            (COMPILER_64_LO(value) & 0xffff));
                    break;
                default:
                    /********************************************************
                     * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
                     * To access TOP level registers use phy_id of port0. 
                     Now restore back 
                     */
                    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
                    return SOC_E_CONFIG;    
            }
            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_TX_PORT_MAC_ADDRESS_INDEX:
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_CTL_0_REG_OFFSET,
                    (COMPILER_64_LO(value) & 0x0003) << (dev_port*2));
            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_RX_PORT_MAC_ADDRESS_INDEX:
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_CTL_1_REG_OFFSET,
                    (COMPILER_64_LO(value) & 0x0003) << (dev_port*2));
            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_MAC_ADDRESS_1:    
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L1_0_REG_OFFSET,
                    (COMPILER_64_LO(value) & 0xffff));
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L1_1_REG_OFFSET,
                    (COMPILER_64_LO(value) >> 16) & 0xffff);
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L1_2_REG_OFFSET,
                    (COMPILER_64_HI(value) & 0xffff));
            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_MAC_ADDRESS_2:    
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L2_0_REG_OFFSET,
                    (COMPILER_64_LO(value) & 0xffff));
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L2_1_REG_OFFSET,
                    (COMPILER_64_LO(value) >> 16) & 0xffff);
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L2_2_REG_OFFSET,
                    (COMPILER_64_HI(value) & 0xffff));
            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_MAC_ADDRESS_3:    
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L3_0_REG_OFFSET,
                    (COMPILER_64_LO(value) & 0xffff));
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L3_1_REG_OFFSET,
                    (COMPILER_64_LO(value) >> 16) & 0xffff);
            phy_bcm542xx_direct_reg_write(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L3_2_REG_OFFSET,
                    (COMPILER_64_HI(value) & 0xffff));
            break;
        default:
            /********************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
             * To access TOP level registers use phy_id of port0. 
               Now restore back 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
            return SOC_E_PARAM;
    }     
    

    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
     * To access TOP level registers use phy_id of port0. Now restore back 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
                
    return SOC_E_NONE;
}

/*
 * Function:
 *    phy_bcm542xx_oam_control_get
 * Purpose:
 *
 * Parameters:
 *
 * Returns:
 */
int
phy_bcm542xx_oam_control_get(int unit, soc_port_t port, soc_port_control_phy_oam_t type, uint64 *value) {
    uint16  dev_port = 0;
    phy_ctrl_t *pc = NULL;
    soc_port_config_phy_oam_t *config = NULL;
    uint16 value01 = 0, value02 = 0, value03 = 0;
    soc_port_config_phy_oam_dm_mode_t tx_mode = 0, rx_mode = 0;
    uint32 tx_flags;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* Dev port numbers from 0-7 */
    dev_port = PHY_BCM542XX_DEV_PHY_SLICE(pc);
    
    /* For BCM54294 package the logical ports p0-p3 are on port4-7 
       of the device. For example to enable autogrEEEn on port0, 
       rdb register for port 4 0x808 need to be used. 
       Refer package configuration document.
     */
    if(PHY_IS_BCM54294(pc)) {
        dev_port += 4;
    }
    
    
    /* Config saved during config _set */ 
    config = PHY_BCM542XX_DEV_OAM_CONFIG_PTR(pc); 
    tx_mode = config->tx_dm_config.mode;
    tx_flags = config->tx_dm_config.flags;
    rx_mode = config->rx_dm_config.mode;

    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> OVERRIDE  
     * To access TOP level registers use phy_id of port0. Override this ports 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_BASE(pc);

    switch(type) {
        case SOC_PORT_CONTROL_PHY_OAM_DM_TX_ETHERTYPE:
            switch(tx_mode) {
                case SOC_PORT_CONFIG_PHY_OAM_DM_Y1731:
                    if(tx_flags & SOC_PORT_PHY_OAM_DM_TS_FORMAT) { /*NTP */
                        phy_bcm542xx_direct_reg_read(unit, pc, 
                                PHY_BCM542XX_TOP_1588_DM_ETHTYPE2_REG_OFFSET,
                                &value01);
                    } else { /*PTP */
                        phy_bcm542xx_direct_reg_read(unit, pc, 
                                PHY_BCM542XX_TOP_1588_DM_ETHTYPE1_REG_OFFSET,
                                &value01);
                    }
                    break;
                case SOC_PORT_CONFIG_PHY_OAM_DM_BHH:
                    if(tx_flags & SOC_PORT_PHY_OAM_DM_TS_FORMAT) { /*NTP */
                        phy_bcm542xx_direct_reg_read(unit, pc, 
                                PHY_BCM542XX_TOP_1588_DM_ETHTYPE4_REG_OFFSET,
                                &value01);
                    } else { /*PTP */
                        phy_bcm542xx_direct_reg_read(unit, pc, 
                                PHY_BCM542XX_TOP_1588_DM_ETHTYPE3_REG_OFFSET,
                                &value01);
                    }
                    break;
                case SOC_PORT_CONFIG_PHY_OAM_DM_IETF:    
                    phy_bcm542xx_direct_reg_read(unit, pc, 
                            PHY_BCM542XX_TOP_1588_DM_ETHTYPE9_REG_OFFSET,
                                &value01);
                    break;
                default:
                    break;    
            }
 
            COMPILER_64_SET((*value), 0,  (uint32)value01);

            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_RX_ETHERTYPE:
            switch(rx_mode) {
                case SOC_PORT_CONFIG_PHY_OAM_DM_Y1731:
                    phy_bcm542xx_direct_reg_read(unit, pc, 
                            PHY_BCM542XX_TOP_1588_DM_ETHTYPE5_REG_OFFSET,
                                &value01);
                    break;
                case SOC_PORT_CONFIG_PHY_OAM_DM_BHH:
                    phy_bcm542xx_direct_reg_read(unit, pc, 
                            PHY_BCM542XX_TOP_1588_DM_ETHTYPE8_REG_OFFSET,
                                &value01);
                    break;
                case SOC_PORT_CONFIG_PHY_OAM_DM_IETF:    
                    phy_bcm542xx_direct_reg_read(unit, pc, 
                            PHY_BCM542XX_TOP_1588_DM_ETHTYPE10_REG_OFFSET,
                                &value01);
                    break;
                default:
                    break;    
            }
            COMPILER_64_SET((*value), 0, (uint32)value01);
            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_TX_PORT_MAC_ADDRESS_INDEX:
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_CTL_0_REG_OFFSET,
                    &value01);
            COMPILER_64_SET((*value), 0, ((uint32)value01 >> (dev_port*2)) & 0x3);
            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_RX_PORT_MAC_ADDRESS_INDEX:
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_CTL_1_REG_OFFSET,
                    &value01);
            COMPILER_64_SET((*value), 0, ((uint32)value01 >> (dev_port*2)) & 0x3);
            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_MAC_ADDRESS_1:    
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L1_0_REG_OFFSET,
                    &value01);
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L1_1_REG_OFFSET,
                    &value02);
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L1_2_REG_OFFSET,
                    &value03);
            COMPILER_64_SET((*value), (uint32)value03, 
                             ((uint32)value02 << 16) | ((uint32)value01));
            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_MAC_ADDRESS_2:    
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L2_0_REG_OFFSET,
                    &value01);
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L2_1_REG_OFFSET,
                    &value02);
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L2_2_REG_OFFSET,
                    &value03);
            COMPILER_64_SET((*value), (uint32)value03, 
                             ((uint32)value02 << 16) | ((uint32)value01));
            break;
        case SOC_PORT_CONTROL_PHY_OAM_DM_MAC_ADDRESS_3:    
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L3_0_REG_OFFSET,
                    &value01);
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L3_1_REG_OFFSET,
                    &value02);
            phy_bcm542xx_direct_reg_read(unit, pc, 
                    PHY_BCM542XX_TOP_1588_DM_MAC_L3_2_REG_OFFSET,
                    &value03);
            COMPILER_64_SET((*value), (uint32)value03, 
                             ((uint32)value02 << 16) | ((uint32)value01));
            break;
        default:
            /********************************************************
             * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
             * To access TOP level registers use phy_id of port0. 
               Now restore back 
             */
            pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
            return SOC_E_PARAM;
    }
    /***********************************************************************
     * NOTE: TOP LEVEL REGISTER ACCESS -> RESTORE
     * To access TOP level registers use phy_id of port0. Now restore back 
     */
    pc->phy_id = PHY_BCM542XX_DEV_PHY_ID_ORIG(pc);
    
    return SOC_E_NONE;
}

#ifdef BROADCOM_DEBUG

int
phy_bcm542xx_rdb_reg_set(int unit, soc_port_t port, int rdb, int value) {

    phy_ctrl_t *pc = NULL;
    uint16 valuer;

    pc = EXT_PHY_SW_STATE(unit, port);

    phy_bcm542xx_direct_reg_write(unit, pc, (uint16)rdb, (uint16)(value & 0xffff));

    phy_bcm542xx_direct_reg_read(unit, pc, (uint16)rdb, &valuer);

    soc_cm_print("u=%d p=%d rdb=0x%04x value read back=0x%04x\n", unit, port, (uint16)rdb, valuer);

    return SOC_E_NONE;
}

int
phy_bcm542xx_rdb_reg_get(int unit, soc_port_t port, int rdb) {

    phy_ctrl_t *pc = NULL;
    uint16 value;

    pc = EXT_PHY_SW_STATE(unit, port);

    phy_bcm542xx_direct_reg_read(unit, pc, (uint16)rdb, &value);

    soc_cm_print("u=%d p=%d rdb=0x%04x value=0x%04x\n", unit, port, (uint16)rdb, value);

    return SOC_E_NONE;
}

#endif

/*
 * Variable:    phy_542xxdrv_ge
 * Purpose:     PHY driver for 542XX
 */
phy_driver_t phy_542xxdrv_ge = {
    "542XX Gigabit PHY Driver",
    phy_bcm542xx_init,
    phy_fe_ge_reset,
    phy_bcm542xx_link_get,
    phy_bcm542xx_enable_set,
    phy_bcm542xx_enable_get,
    phy_bcm542xx_duplex_set,
    phy_bcm542xx_duplex_get,
    phy_bcm542xx_speed_set,
    phy_bcm542xx_speed_get,
    phy_bcm542xx_master_set,
    phy_bcm542xx_master_get,
    phy_bcm542xx_autoneg_set,
    phy_bcm542xx_autoneg_get,
    NULL,                       /* pd_adv_local_set */
    NULL,                       /* pd_adv_local_get */
    NULL,                       /* pd_adv_remote_get */
    phy_bcm542xx_lb_set,
    phy_bcm542xx_lb_get,
    phy_bcm542xx_interface_set,
    phy_bcm542xx_interface_get,
    NULL,                       /* pd_ability */
    phy_bcm542xx_link_up,       /* pd_linkup */
    NULL,                       /* pd_linkdn_evt */
    phy_bcm542xx_mdix_set,
    phy_bcm542xx_mdix_get,
    phy_bcm542xx_mdix_status_get,
    phy_bcm542xx_medium_config_set,
    phy_bcm542xx_medium_config_get,
    phy_bcm542xx_medium_status,
    phy_54280_cable_diag_dispatch,
    NULL,                       /* pd_link_change */
    phy_bcm542xx_control_set,      
    phy_bcm542xx_control_get,      
    phy_ge_reg_read,
    phy_ge_reg_write,
    phy_ge_reg_modify,
    NULL,                       /* pd_notify */
    phy_bcm542xx_probe,                       /* pd_probe */
    phy_bcm542xx_ability_advert_set,  
    phy_bcm542xx_ability_advert_get,  
    phy_bcm542xx_ability_remote_get,  
    phy_bcm542xx_ability_local_get,
    NULL,                        /* pd_firmware_set */ 
    phy_bcm542xx_timesync_config_set,
    phy_bcm542xx_timesync_config_get,
    phy_bcm542xx_timesync_control_set,
    phy_bcm542xx_timesync_control_get,
    NULL, /* pd_diag_ctrl */
    NULL, /* pd_lane_control_set */
    NULL, /* pd_lane_control_get */
    NULL, /* pd_lane_reg_read */
    NULL, /* pd_lane_reg_write */
    phy_bcm542xx_oam_config_set,
    phy_bcm542xx_oam_config_get,
    phy_bcm542xx_oam_control_set,
    phy_bcm542xx_oam_control_get,
    phy_bcm542xx_timesync_enhanced_capture_get
};


#else /* INCLUDE_PHY_542XX */
int _soc_phy_542xx_not_empty;
#endif /* INCLUDE_PHY_542XX */


