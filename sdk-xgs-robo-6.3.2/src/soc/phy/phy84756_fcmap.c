/*
 * $Id: phy84756_fcmap.c 1.50.2.1 Broadcom SDK $
 *
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
 * File:        phy84756.c
 * Purpose:     SDK PHY driver for BCM84756 (FCMAP)
 *
 * Supported BCM546X Family of PHY devices:
 *
 *      Device  Ports    Media                           MAC Interface
 *      84756    4       4 10G SFP+                      XFI
 *      84757    4       4 10G SFP+/8(4/2) FC            XFI
 *      84759    4       4 10G SFP+                      XFI
 *
 *	             OUI	    Model	Revision
 *  BCM84756	18-C0-86	100111	00xx
 *  BCM84757	18-C0-86	100111	10xx
 *  BCM84759	18-C0-86	100111	01xx
 *
 *
 * Workarounds:
 *
 * References:
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



#if defined(INCLUDE_FCMAP) || defined(INCLUDE_MACSEC)
#if defined(INCLUDE_PHY_84756)
#include "phyconfig.h"    /* Must be the first phy include after phydefs.h */
#include "phyident.h"
#include "phyreg.h"
#include "phynull.h"
#include "phyfege.h"
#include "phyxehg.h"

#if defined(INCLUDE_FCMAP)
#include <soc/fcmapphy.h>
#endif
#if defined(INCLUDE_MACSEC)
#include <soc/macsecphy.h>
#endif
#include "phy_mac_ctrl.h"
#include "phy_xmac.h"
#include "phy84756_fcmap_int.h"
#include "phy84756_fcmap.h"
#include "phy84756_i2c.h"

#define ADVERT_ALL_COPPER \
        (SOC_PA_PAUSE | SOC_PA_SPEED_10MB | \
         SOC_PA_SPEED_100MB | SOC_PA_SPEED_1000MB)

#define ADVERT_ALL_FIBER \
        (SOC_PA_PAUSE | SOC_PA_SPEED_1000MB) 

#define ADVERT_ALL_10GFIBER \
        (SOC_PA_PAUSE | SOC_PA_SPEED_10GB) 

#define PHY84756_REG_READ(_unit, _phy_ctrl, _addr, _val) \
            READ_PHY_REG((_unit), (_phy_ctrl), (_addr), (_val))
            
#define PHY84756_REG_WRITE(_unit, _phy_ctrl, _addr, _val) \
                WRITE_PHY_REG((_unit), (_phy_ctrl), (_addr), (_val))

#define READ_PHY84756_PMA_PMD_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY84756_REG_READ((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(1, (_reg)), (_val))
              
#define WRITE_PHY84756_PMA_PMD_REG(_unit, _phy_ctrl, _reg,_val) \
                PHY84756_REG_WRITE((_unit), (_phy_ctrl), \
                  SOC_PHY_CLAUSE45_ADDR(1, (_reg)), (_val))


#define BFCMAP_PHY84756_LINE_MAC_PORT(p)    (p)
#define BFCMAP_PHY84756_SWITCH_MAC_PORT(p)  ((p) + 1)

#define PHY84756_XFI(unit,_pc)\
BFCMAP_PHY84756_IO_MDIO_WRITE((_pc), BLMI_IO_CL45_ADDRESS(1, 0xffff), 0x1)

#define PHY84756_MMF(unit,_pc)\
BFCMAP_PHY84756_IO_MDIO_WRITE((_pc), BLMI_IO_CL45_ADDRESS(1, 0xffff), 0x0)

/*
 * PHY Map structure for mapping mac drivers
 */
typedef struct phy84756_map_s {
    bfcmap_phy84756_intf_t line_mode;     /* configured line mode */
    phy_mac_ctrl_t      *unimac;
    phy_mac_ctrl_t      *xmac;
    int phy_ext_rom_boot;
    int fcmap_enable;
    int macsec_enable;
    int macsec_support;
    int fiber_force_speed;
    int skip_fw_download;
    int chip_rev_id;
    uint8 *firmware;
    int firmware_len;
}phy84756_map_t;

/* 
 * PHY MDIO Address, Device Address and Device Port Mapping table
 */
#define PHY84756_PHY_INFO(pc)  ((phy84756_map_t *)((pc) + 1))
#define PHY_EXT_ROM_BOOT(_pc)  (((phy84756_map_t *)((_pc) + 1))->phy_ext_rom_boot)
#define FCMAP_ENABLE(_pc)      (((phy84756_map_t *)((_pc) + 1))->fcmap_enable)
#define MACSEC_ENABLE(_pc)     (((phy84756_map_t *)((_pc) + 1))->macsec_enable)
#define MACSEC_SUPPORT(_pc)     (((phy84756_map_t *)((_pc) + 1))->macsec_support)

#define FIBER_FORCE_SPEED(_pc) (((phy84756_map_t *)((_pc) + 1))->fiber_force_speed)
#define SKIP_FW_DOWNLOAD(_pc) (((phy84756_map_t *)((_pc) + 1))->skip_fw_download)
#define FIRMWARE(_pc) (((phy84756_map_t *)((_pc) + 1))->firmware)
#define FIRMWARE_LEN(_pc) (((phy84756_map_t *)((_pc) + 1))->firmware_len)
#define CHIP_REV_ID(_pc) (((phy84756_map_t *)((_pc) + 1))->chip_rev_id)
#define CHIP_IS_A0(_pc)  (CHIP_REV_ID(_pc) == 0xa0a0)
#define CHIP_IS_B0(_pc)  (CHIP_REV_ID(_pc) == 0xb0b0)
#define CHIP_IS_C0(_pc)  (CHIP_REV_ID(_pc) == 0xc0c0)
/*#define CHIP_IS_C0(_pc) \
    ((PHY_REV((_pc)->phy_id0, (_pc)->phy_id1) & 0xe) == 0xa ) 
*/

/* OUI from ID0 and ID1 registers contents */
#define BFCMAP_PHY84756_PHY_OUI(id0, id1)                          \
    bfcmap_phy84756_bit_rev_by_byte_word32((buint32_t)(id0) << 6 | \
                                           ((id1) >> 10 & 0x3f))

/* PHY model from ID0 and ID1 register Contents */
#define BFCMAP_PHY84756_PHY_MODEL(id0, id1) ((id1) >> 4 & 0x3f)

/* PHY revision from ID0 and ID1 register Contents */
#define BFCMAP_PHY84756_PHY_REV(id0, id1)   ((id1) & 0xf)

#define PHY84756_EDC_MODE_MASK          0xff
/* PRBS */
#define PHY84756_FORTYG_PRBS_PATTERN_TESTING_CONTROL_STATUS         0x0135
#define PHY84756_40G_PRBS31             (1 << 7)               
#define PHY84756_40G_PRBS9              (1 << 6) 
#define PHY84756_40G_PRBS_TX_ENABLE     (1 << 3) 
#define PHY84756_40G_PRBS_RX_ENABLE     (1 << 0) 
#define PHY84756_40G_PRBS_ENABLE     (PHY84756_40G_PRBS_RX_ENABLE | \
	PHY84756_40G_PRBS_TX_ENABLE)

#define PHY84756_FORTYG_PRBS_RECEIVE_ERROR_COUNTER_LANE0            0x0140 
#define PHY84756_USER_PRBS_CONTROL_0_REGISTER                       0xCD14 
#define PHY84756_USER_PRBS_TX_INVERT   (1 << 4)
#define PHY84756_USER_PRBS_RX_INVERT   (1 << 15)
#define PHY84756_USER_PRBS_INVERT      (1 << 4 | 1 << 15)
#define PHY84756_USER_PRBS_ENABLE      (1 << 7)
#define PHY84756_USER_PRBS_TYPE_MASK   (0x7)

#define PHY84756_USER_PRBS_STATUS_0_REGISTER                        0xCD15 
#define PHY84756_GENSIG_8071_REGISTER                               0xCD16 
#define PHY84756_RESET_CONTROL_REGISTER                             0xCD17 

/* preemphasis control */
#define PHY84756_PREEMPH_CTRL_FORCE_SHFT       15
#define PHY84756_PREEMPH_GET_FORCE(_val)  \
	(((_val) >> PHY84756_PREEMPH_CTRL_FORCE_SHFT) & 1)
#define PHY84756_PREEMPH_CTRL_POST_TAP_SHFT    10
#define PHY84756_PREEMPH_GET_POST_TAP(_val) \
	(((_val) >> PHY84756_PREEMPH_CTRL_POST_TAP_SHFT) & 0x1f)
#define PHY84756_PREEMPH_CTRL_MAIN_TAP_SHFT    4
#define PHY84756_PREEMPH_GET_MAIN_TAP(_val) \
	(((_val) >> PHY84756_PREEMPH_CTRL_MAIN_TAP_SHFT) & 0x3f) 
#define PHY84756_PREEMPH_REG_FORCE_MASK     (1 << 15)
#define PHY84756_PREEMPH_REG_POST_TAP_SHFT  4
#define PHY84756_PREEMPH_REG_POST_TAP_MASK  (0xf << PHY84756_PREEMPH_REG_POST_TAP_SHFT)
#define PHY84756_PREEMPH_REG_MAIN_TAP_SHFT  11   
#define PHY84756_PREEMPH_REG_MAIN_TAP_MASK  (0x1f << PHY84756_PREEMPH_REG_MAIN_TAP_SHFT)
#define PHY84756_PREEMPH_REG_TX_PWRDN_MASK  (0x1 << 10)

/*
 * Function:     
 *    bfcmap_bit_rev_by_byte_word32
 * Purpose:    
 *    Reverse the bits in each byte of a 32 bit long 
 * Parameters:
 *    n - 32bit input
 * Notes: 
 */
STATIC buint32_t
bfcmap_phy84756_bit_rev_by_byte_word32(buint32_t n)
{
    n = (((n & 0xaaaaaaaa) >> 1) | ((n & 0x55555555) << 1));
    n = (((n & 0xcccccccc) >> 2) | ((n & 0x33333333) << 2));
    n = (((n & 0xf0f0f0f0) >> 4) | ((n & 0x0f0f0f0f) << 4));
    return n;
}

/* Structure used to communicate with the event handler */
typedef struct _phy84756_fcmap_event_callback_data_s{
    int unit;
    soc_port_t port;
    int link;
}_phy84756_fcmap_event_callback_data_t;

STATIC int phy_84756_fcmap_init(int unit, soc_port_t port);
STATIC int phy_84756_fcmap_link_get(int unit, soc_port_t port, int *link);
STATIC int phy_84756_fcmap_enable_set(int unit, soc_port_t port, int enable);
STATIC int phy_84756_fcmap_duplex_set(int unit, soc_port_t port, int duplex);
STATIC int phy_84756_fcmap_duplex_get(int unit, soc_port_t port, int *duplex);
STATIC int phy_84756_fcmap_speed_set(int unit, soc_port_t port, int speed);
STATIC int phy_84756_fcmap_speed_get(int unit, soc_port_t port, int *speed);
STATIC int phy_84756_fcmap_an_set(int unit, soc_port_t port, int autoneg);
STATIC int phy_84756_fcmap_an_get(int unit, soc_port_t port,
                                int *autoneg, int *autoneg_done);
STATIC int phy_84756_fcmap_lb_set(int unit, soc_port_t port, int enable);
STATIC int phy_84756_fcmap_lb_get(int unit, soc_port_t port, int *enable);
STATIC int phy_84756_fcmap_ability_advert_set(int unit, soc_port_t port, 
                                       soc_port_ability_t *ability);
STATIC int phy_84756_fcmap_ability_advert_get(int unit, soc_port_t port,
                                       soc_port_ability_t *ability);
STATIC int phy_84756_fcmap_ability_remote_get(int unit, soc_port_t port,
                                       soc_port_ability_t *ability);
STATIC int phy_84756_fcmap_ability_local_get(int unit, soc_port_t port, 
                                soc_port_ability_t *ability);
STATIC int phy_84756_fcmap_firmware_set(int unit, int port, int offset, 
                                 uint8 *data, int len);
STATIC int phy_84756_fcmap_control_set(int unit, soc_port_t port,
                     soc_phy_control_t type, uint32 value);
STATIC int phy_84756_fcmap_control_get(int unit, soc_port_t port,
                     soc_phy_control_t type, uint32 *value);
STATIC int phy_84756_fcmap_linkup(int unit, soc_port_t port);

STATIC int phy_84756_fcmap_reg_read(int unit, soc_port_t port, uint32 flags,
                  uint32 phy_reg_addr, uint32 *phy_data);

STATIC int phy_84756_fcmap_reg_write(int unit, soc_port_t port, uint32 flags,
                   uint32 phy_reg_addr, uint32 phy_data);
STATIC int
_bfcmap_phy84756_system_sgmii_init(phy_ctrl_t *pc, int dev_port);
STATIC int
_bfcmap_phy84756_system_sgmii_duplex_set(phy_ctrl_t *pc, int dev_port, int duplex);
STATIC int
_bfcmap_phy84756_system_sgmii_sync(phy_ctrl_t *pc, int dev_port);

extern buint8_t bcm84756_fw[];
extern buint32_t bcm84756_fw_length;
extern buint8_t bcm84756_b0_fw[];
extern buint32_t bcm84756_b0_fw_length;

/*
 * Function:     
 *    _phy84756_system_xfi_speed_set
 * Purpose:    
 *    To set system side SGMII speed
 * Parameters:
 *    phy_id    - PHY's device address
 *    speed     - Speed to set
 *               100000 = 10Gbps
 * Returns:    
 */
static int
_phy84756_system_xfi_speed_set(phy_ctrl_t *pc, int dev_port, int speed)
{
    if (speed != 10000) {
        return SOC_E_CONFIG;
    }
    /* Disable Clause 37 Autoneg */
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_XFI_DEV7_AN_MII_CTRLr(pc, 0,
                                  BFCMAP_PHY84756_AN_MII_CTRL_AE));
    /* Force Speed in PDM */
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_SYS_DEV1_PMD_CTRLr(pc, 
                          BFCMAP_PHY84756_PMD_CTRL_SS_10000,
                          BFCMAP_PHY84756_PMD_CTRL_10GSS_MASK));
    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_84756_fcmap_medium_config_update
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
_phy_84756_fcmap_medium_config_update(int unit, soc_port_t port,
                                soc_phy_config_t *cfg)
{

    SOC_IF_ERROR_RETURN
        (phy_84756_fcmap_speed_set(unit, port, cfg->force_speed));
    SOC_IF_ERROR_RETURN
        (phy_84756_fcmap_duplex_set(unit, port, cfg->force_duplex));
    SOC_IF_ERROR_RETURN
        (phy_84756_fcmap_ability_advert_set(unit, port, &cfg->advert_ability));
    SOC_IF_ERROR_RETURN
        (phy_84756_fcmap_an_set(unit, port, cfg->autoneg_enable));

    return SOC_E_NONE;
}

STATIC int
_phy_84756_fcmap_firmware_is_downloaded(int unit, soc_port_t port)
{
    int rv = 0;
    uint16 data16;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    /*
     * To determine whether the firmware has been downloaded, it must
     * pass the following:
     * 1. firmware is in RAM: 1.c848.13==1
     * 2. expected checksum:  1.ca1c==600d
     * 3. version is non-zero:  1.ca1a!=0
     */

    /* 1. Firmware download status */
    rv = BFCMAP_RD_PHY84756_LN_DEV1_SPI_CTRL_STATr(pc, &data16);
    if (rv != SOC_E_NONE) {
        return 0;
    }
    if (PHY_EXT_ROM_BOOT(pc)) {
        if ((data16 & 0xa000) != 0xa000) {
            return 0;
        }
    } else if ((data16 & 0xf000) != 0x7000) {
            return 0;
    }
    SOC_DEBUG_PRINT((DK_PHY, "FW download status=%x: u=%d p=%d\n", data16, unit, port));

    /* 2. Expected checksum */
    rv = BFCMAP_RD_PHY84756_LN_DEV1_CHECKSUMr(pc, &data16);
    if ((rv != SOC_E_NONE) || (data16 != BFCMAP_PHY84756_FW_CHECKSUM)) {
        return 0;
    }
    SOC_DEBUG_PRINT((DK_PHY, "FW checksum=%x: u=%d p=%d\n", data16, unit, port));

    /* 3. non-zero version */
    rv = BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xCA1A, &data16);
    if ((rv != SOC_E_NONE) || (data16 == 0)) {
        return 0;
    }
    SOC_DEBUG_PRINT((DK_PHY, "FW version=%x: u=%d p=%d\n", data16, unit, port));

    return 1;
}

/* Function:
 *    phy_84756_fcmap_init
 * Purpose:    
 *    Initialize 84756 phys
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #. 
 * Returns:    
 *    SOC_E_NONE
 */

STATIC int
phy_84756_fcmap_init(int unit, soc_port_t port)
{
    phy_ctrl_t *pc;
    int rv = SOC_E_NONE;
    buint16_t data;

    pc     = EXT_PHY_SW_STATE(unit, port);

    if (PHYCTRL_INIT_STATE(pc) == PHYCTRL_INIT_STATE_DEFAULT ||
        PHYCTRL_INIT_STATE(pc) == PHYCTRL_INIT_STATE_PASS1   ) {
        int mmi_mdio_addr;

        SOC_DEBUG_PRINT((DK_PHY, "phy_84756_fcmap_init PASS1: u=%d p=%d\n",unit, port));

        sal_memset(PHY84756_PHY_INFO(pc), 0, sizeof(phy84756_map_t));



        FCMAP_ENABLE(pc) = soc_property_port_get(unit, port, spn_FCMAP_ENABLE, 0);

        MACSEC_ENABLE(pc) = soc_property_port_get(unit, port, spn_MACSEC_ENABLE, 0);

        /* Save chip rev */
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV1_CHIP_REVr(pc, &data));
        CHIP_REV_ID(pc) = data;

        if (CHIP_IS_A0(pc)) {
            /* this firmware is used by A0 */
            FIRMWARE(pc) = bcm84756_fw;
            FIRMWARE_LEN(pc) = bcm84756_fw_length;
        } else {
            /* this firmware is used by B0 and C0 */
            FIRMWARE(pc) = bcm84756_b0_fw;
            FIRMWARE_LEN(pc) = bcm84756_b0_fw_length;
        }

        MACSEC_SUPPORT(pc) = 0;

#if defined(INCLUDE_FCMAP)
        if (FCMAP_ENABLE(pc)) {
            pc->fcmap_enable = FCMAP_ENABLE(pc);
            pc->fcmap_dev_port = 0;
            mmi_mdio_addr = pc->phy_id;

            pc->fcmap_uc_dev_addr = SOC_FCMAPPHY_MDIO_ADDR(unit, pc->phy_id, 1);
            pc->fcmap_dev_addr = SOC_FCMAPPHY_MDIO_ADDR(unit, mmi_mdio_addr, 1);

            rv = bfcmap_phy84756_phy_mac_driver_attach(pc, 
                                                  pc->fcmap_dev_addr, 
                                                  pc->fcmap_dev_port,
                                                  blmi_io_mmi1_cl45);
            if(rv != SOC_E_NONE) {
                SOC_DEBUG_PRINT((DK_WARN, "phy_84756_fcmap_init: bfcmap_phy_mac_driver_attach "
                                 "returned error for u=%d p=%d\n", unit, port));
                return SOC_E_FAIL;
            }
        }
#endif /* INCLUDE_FCMAP */

#if defined(INCLUDE_MACSEC)
        MACSEC_SUPPORT(pc) = 1;
        if  (MACSEC_ENABLE(pc)) {
            pc->macsec_enable = MACSEC_ENABLE(pc);
            pc->macsec_dev_port = soc_property_port_get(unit, port, 
                                               spn_MACSEC_PORT_INDEX, -1);

            /* Base PHY ID is used to access MACSEC core */
            mmi_mdio_addr = soc_property_port_get(unit, port, 
                                                spn_MACSEC_DEV_ADDR, -1);
            if (mmi_mdio_addr < 0) {
                soc_cm_debug(DK_WARN, "phy_84756_macsec_init: "
                             "MACSEC_DEV_ADDR property "
                             "not configured for u=%d p=%d\n", unit, port);
                return SOC_E_CONFIG;
            }

            pc->macsec_dev_addr = SOC_MACSECPHY_MDIO_ADDR(unit, mmi_mdio_addr, 1);

            rv = bfcmap_phy84756_phy_mac_driver_attach(pc, 
                                                  pc->macsec_dev_addr, 
                                                  pc->macsec_dev_port,
                                                  blmi_io_mmi1_cl45);

            if(rv != SOC_E_NONE) {
                SOC_DEBUG_PRINT((DK_WARN, "phy_84756_macsec_init: bfcmap_phy_mac_driver_attach "
                                 "returned error for u=%d p=%d\n", unit, port));
                return SOC_E_FAIL;
            }
        }
#endif /* INCLUDE_MACSEC */

        /* If 1G link detected set speed */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xC820, &data));
        if (data & 0x1) {
            FIBER_FORCE_SPEED(pc) = 1000;
        } else {
            FIBER_FORCE_SPEED(pc) = 10000;
        }

        /* By default select ethernet chip mode */
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV1_CHIP_MODEr(pc, &data));
        data &= ~0x3;
        SOC_IF_ERROR_RETURN(
            BFCMAP_WR_PHY84756_LN_DEV1_CHIP_MODEr(pc, data));

        PHY_EXT_ROM_BOOT(pc) = soc_property_port_get(unit, port,
                                                spn_PHY_EXT_ROM_BOOT, 1);

        SKIP_FW_DOWNLOAD(pc) = _phy_84756_fcmap_firmware_is_downloaded(unit, port);

        if ((!PHY_EXT_ROM_BOOT(pc)) && (PHYCTRL_INIT_STATE(pc) != PHYCTRL_INIT_STATE_DEFAULT) && (!SKIP_FW_DOWNLOAD(pc))) {
                pc->flags |= PHYCTRL_MDIO_BCST;
                if (CHIP_IS_A0(pc)) {
                    pc->dev_name = "BCM84756_A0";
                } else {
                    if (CHIP_IS_B0(pc)) 
                        pc->dev_name = "BCM84756_B0";
                    else
                        pc->dev_name = "BCM84756_C0";
                }
        }

        /* reset */
        SOC_IF_ERROR_RETURN(
                bfcmap_phy84756_reset(pc));

        if ( PHYCTRL_INIT_STATE(pc) != PHYCTRL_INIT_STATE_DEFAULT ) {
            /* indicate second pass of init is needed */
            PHYCTRL_INIT_STATE_SET(pc, PHYCTRL_INIT_STATE_PASS2);

            return SOC_E_NONE;
        }

    }

    if ((PHYCTRL_INIT_STATE(pc) == PHYCTRL_INIT_STATE_PASS2) ||
        (PHYCTRL_INIT_STATE(pc) == PHYCTRL_INIT_STATE_DEFAULT)) {
        int line_mode;
        int fiber_preferred;
#if defined(INCLUDE_FCMAP)
        phy_ctrl_t *int_pc;
#endif /* INCLUDE_FCMAP */

        SOC_DEBUG_PRINT((DK_PHY, "phy_84756_fcmap_init PASS2: u=%d p=%d\n",unit, port));

        /* Preferred mode:
         * phy_fiber_pref = 1 ; Port is in SFI mode in 10G Default
         *                      when speed is changed to 1G, its 1000X
         * phy_fiber_pref = 0; Port is in SGMII mode, allows only 1G/100M/10M
         */
        fiber_preferred =
            soc_property_port_get(unit, port, spn_PHY_FIBER_PREF, 1);

        /* Initially configure for the preferred medium. */
        PHY_FLAGS_CLR(unit, port, PHY_FLAGS_COPPER);
        PHY_FLAGS_CLR(unit, port, PHY_FLAGS_FIBER);
        PHY_FLAGS_CLR(unit, port, PHY_FLAGS_PASSTHRU);
        PHY_FLAGS_CLR(unit, port, PHY_FLAGS_100FX);

        if (fiber_preferred) {
            if (FIBER_FORCE_SPEED(pc) == 10000) {
                line_mode = BFCMAP_PHY84756_INTF_SFI;
            } else {
                line_mode = BFCMAP_PHY84756_INTF_1000X;
            }
            pc->copper.enable = FALSE;
            pc->fiber.enable = TRUE;
            pc->interface = SOC_PORT_IF_XFI;
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_FIBER);
        } else {
            pc->interface = SOC_PORT_IF_SGMII;
            line_mode = BFCMAP_PHY84756_INTF_SGMII;
            pc->copper.enable = TRUE;
            pc->fiber.enable = FALSE;
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_COPPER);
        }
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_C45);

        pc->copper.preferred = !fiber_preferred;
        pc->copper.autoneg_enable = TRUE;
        pc->copper.autoneg_advert = ADVERT_ALL_COPPER;
        pc->copper.force_speed = 1000;
        pc->copper.force_duplex = TRUE;
        pc->copper.master = SOC_PORT_MS_AUTO;
        pc->copper.mdix = SOC_PORT_MDIX_AUTO;

        pc->fiber.preferred = TRUE;
        pc->fiber.autoneg_enable = FALSE;
        pc->fiber.autoneg_advert = ADVERT_ALL_FIBER;
        pc->fiber.force_speed = FIBER_FORCE_SPEED(pc);
        pc->fiber.force_duplex = TRUE;
        pc->fiber.master = SOC_PORT_MS_NONE;
        pc->fiber.mdix = SOC_PORT_MDIX_NORMAL;


        SOC_IF_ERROR_RETURN(
                bfcmap_phy84756_no_reset_setup(pc,
                                               line_mode,
                                               PHY_EXT_ROM_BOOT(pc)));

        /* Get Requested LED selectors (defaults are hardware defaults) */
        pc->ledmode[0] = soc_property_port_get(unit, port, spn_PHY_LED1_MODE, 0);
        pc->ledmode[1] = soc_property_port_get(unit, port, spn_PHY_LED2_MODE, 1);
        pc->ledmode[2] = soc_property_port_get(unit, port, spn_PHY_LED3_MODE, 3);
        pc->ledmode[3] = soc_property_port_get(unit, port, spn_PHY_LED4_MODE, 6);
        pc->ledctrl = soc_property_port_get(unit, port, spn_PHY_LED_CTRL, 0x8);
        pc->ledselect = soc_property_port_get(unit, port, spn_PHY_LED_SELECT, 0);


        if (line_mode != BFCMAP_PHY84756_INTF_SGMII)
        {
            SOC_IF_ERROR_RETURN
                (_phy_84756_fcmap_medium_config_update(unit, port,
                                        PHY_COPPER_MODE(unit, port) ?
                                        &pc->copper :
                                        &pc->fiber));
        }

#if defined(INCLUDE_FCMAP)
        if (FCMAP_ENABLE(pc)) {
            if (CHIP_IS_C0(pc)) {
                rv = soc_fcmapphy_init(unit, port, pc, 
                            BFCMAP_CORE_BCM84756_C0, bfcmap_io_mmi1_cl45);
            }
            else {
                rv = soc_fcmapphy_init(unit, port, pc, 
                            BFCMAP_CORE_BCM84756, bfcmap_io_mmi1_cl45);
            }
            if (!SOC_SUCCESS(rv)) {
                soc_cm_debug(DK_ERR, "soc_fcmapphy_init: FCMAP init for"
                     " u=%d p=%d FAILED ", unit, port);
            } else {
                soc_cm_debug(DK_PHY, "soc_fcmapphy_init: FCMAP init for"
                     " u=%d p=%d SUCCESS ", unit, port);
            }

            /* Internal serdes speed should be set to 10G in FC Mode */
            int_pc = INT_PHY_SW_STATE(unit, port);
            if (NULL != int_pc) {
                PHY_AUTO_NEGOTIATE_SET (int_pc->pd, unit, port, 0);
                PHY_SPEED_SET(int_pc->pd, unit, port, 10000);
            }
        } 
#endif
#if defined(INCLUDE_MACSEC)
        if (MACSEC_ENABLE(pc)) {
            if (CHIP_IS_C0(pc)) {
                rv = soc_macsecphy_init(unit, port, pc, 
                        BMACSEC_CORE_BCM84756_C0, bmacsec_io_mmi1_cl45);
            } else {
                rv = soc_macsecphy_init(unit, port, pc, 
                        BMACSEC_CORE_BCM84756, bmacsec_io_mmi1_cl45);
            }
            if (!SOC_SUCCESS(rv)) {
            soc_cm_debug(DK_ERR, "soc_macsec_init: MACSEC init for"
                     " u=%d p=%d FAILED ", unit, port);
            } else {
            soc_cm_debug(DK_PHY, "soc_macsec_init: MACSEC init for"
                     " u=%d p=%d SUCCESS ", unit, port);
            }
        }
#endif
            PHYCTRL_INIT_STATE_SET(pc, PHYCTRL_INIT_STATE_DEFAULT);
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *    phy_84756_fcmap_link_get
 * Purpose:
 *    Get layer2 connection status.
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #. 
 *      link - address of memory to store link up/down state.
 * Returns:    
 *    SOC_E_NONE
 */

STATIC int
phy_84756_fcmap_link_get(int unit, soc_port_t port, int *link)
{
    phy_ctrl_t *pc;
    int rv = SOC_E_NONE;
    bfcmap_phy84756_intf_t bfcmap_mode;
    int bfcmap_link;

    pc    = EXT_PHY_SW_STATE(unit, port);
    *link = FALSE;      /* Default return */

    if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE)) {
        return SOC_E_NONE;
    }

#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        rv = bfcmap_phy84756_link_get(pc, &bfcmap_link);
        if (!SOC_SUCCESS(rv)) {
            return SOC_E_FAIL;
        }
        *link = bfcmap_link ? TRUE : FALSE;
        return SOC_E_NONE;
    }
#endif

    rv = bfcmap_phy84756_line_intf_get(pc, 0, &bfcmap_mode);
    if (!SOC_SUCCESS(rv)) {
        return SOC_E_FAIL;
    }

    if (bfcmap_mode == BFCMAP_PHY84756_INTF_SGMII) {
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_COPPER);
        PHY_FLAGS_CLR(unit, port, PHY_FLAGS_FIBER);
    } else {
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_FIBER);
        PHY_FLAGS_CLR(unit, port, PHY_FLAGS_COPPER);
    }

    rv = bfcmap_phy84756_link_get(pc, &bfcmap_link);
    if (!SOC_SUCCESS(rv)) {
        return SOC_E_FAIL;
    }

    if (bfcmap_link) {
        *link = TRUE;
    } else {
        *link = FALSE;
    }
    SOC_DEBUG_PRINT((DK_PHY | DK_VERBOSE,
                     "phy_84756_fcmap_link_get: u=%d p=%d link=%d\n",
                      unit, port,
                      *link));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_84756_fcmap_enable_set
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
phy_84756_fcmap_enable_set(int unit, soc_port_t port, int enable)
{
    phy_ctrl_t *pc;
    buint16_t power;

    pc     = EXT_PHY_SW_STATE(unit, port);

    if (enable) {
        PHY_FLAGS_CLR(unit, port, PHY_FLAGS_DISABLE);
    } else {
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_DISABLE);
    }

#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        int p;
        int rv = SOC_E_NONE;
        /* Issue a enable request to the FC firmware */
        p = SOC_FCMAP_PORTID(unit, port);
        if (enable) {
            rv = bfcmap_port_link_enable(p);
        } else {
            rv = bfcmap_port_shutdown(p);
        }
        if (rv != BFCMAP_E_NONE) {
            SOC_DEBUG_PRINT((DK_PHY,
                 "phy_84756_fcmap_enable_set: Failed u=%d p=%d enable=%d rv=%d\n",
                      unit, port, enable, rv));
            return SOC_E_FAIL;
        }
        SOC_DEBUG_PRINT((DK_PHY,
                 "phy_84756_fcmap_enable_set:  Success u=%d p=%d enable=%d rv=%d\n",
                      unit, port, enable, rv));
        return SOC_E_NONE;
    }
#endif
    

    /* PMA/PMD */
    power = (enable) ? 0 : 0x0001 ;
    SOC_IF_ERROR_RETURN(
    BFCMAP_MOD_PHY84756_LN_DEV1_PMD_XMIT_DISABLEr(pc, power, 0x0001));


    SOC_DEBUG_PRINT((DK_PHY, "phy_84756_fcmap_enable_set: "
             "Power %s fiber medium\n", (enable) ? "up" : "down"));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_84756_fcmap_duplex_set
 * Purpose:
 *      Set the current duplex mode
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      duplex - Boolean, true indicates full duplex, false indicates half.
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_UNAVAIL - Half duplex requested, and not supported.
 * Notes:
 */
STATIC int
phy_84756_fcmap_duplex_set(int unit, soc_port_t port, int duplex_in)
{
    int                          rv = SOC_E_NONE ;
    phy_ctrl_t                   *pc;
    bfcmap_phy84756_duplex_t    duplex;
    buint16_t mii_ctrl;
    bfcmap_phy84756_intf_t line_mode;

    pc = EXT_PHY_SW_STATE(unit, port);
#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        return SOC_E_NONE;
    }
#endif

    if (duplex_in) {
        duplex = BFCMAP_PHY84756_FULL_DUPLEX;
    } else {
        duplex = BFCMAP_PHY84756_HALF_DUPLEX;
    }

    SOC_IF_ERROR_RETURN(
        bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

    if ((line_mode == BFCMAP_PHY84756_INTF_1000X ) ||
        (line_mode == BFCMAP_PHY84756_INTF_SFI )) {

        if (duplex == BFCMAP_PHY84756_FULL_DUPLEX) {
            rv =  SOC_E_NONE;
            goto exit;
        } else {
            rv =  SOC_E_UNAVAIL;
            goto exit;
        }
    }
    SOC_IF_ERROR_RETURN(
        BFCMAP_RD_PHY84756_LN_DEV7_AN_MII_CTRLr(pc, &mii_ctrl));

    if (duplex == BFCMAP_PHY84756_FULL_DUPLEX) {
        mii_ctrl |= BFCMAP_PHY84756_AN_MII_CTRL_FD;
    } else {
        mii_ctrl &= ~BFCMAP_PHY84756_AN_MII_CTRL_FD;
    }
    SOC_IF_ERROR_RETURN(
        BFCMAP_WR_PHY84756_LN_DEV7_AN_MII_CTRLr(pc, mii_ctrl));

exit:
    if(SOC_SUCCESS(rv)) {
        pc->copper.force_duplex = duplex_in;
    }


    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_84756_fcmap_duplex_set: u=%d p=%d d=%d rv=%d\n",
                     unit, port, duplex, rv));
    return rv;
}

/*
 * Function:
 *      phy_84756_fcmap_duplex_get
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
phy_84756_fcmap_duplex_get(int unit, soc_port_t port, int *duplex)
{

    phy_ctrl_t                   *pc;
    buint16_t	status;
    bfcmap_phy84756_intf_t line_mode;

    pc = EXT_PHY_SW_STATE(unit, port);

#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        *duplex = TRUE;
        return SOC_E_NONE;
    }
#endif

    SOC_IF_ERROR_RETURN(
        bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

    if ((line_mode == BFCMAP_PHY84756_INTF_1000X ) ||
        (line_mode == BFCMAP_PHY84756_INTF_SFI )) {
        *duplex = TRUE;
    } else {
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV7_AN_BASE1000X_STAT1r(pc, &status));
        if (status & (1U << 2)) {
            *duplex = TRUE;
        } else {
            *duplex = FALSE;
        }
    }

    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_84756_fcmap_duplex_get: u=%d p=%d d=%d \n",
                     unit, port, *duplex));
    return SOC_E_NONE;
}


/*
 * Function:
 *      phy_84756_fcmap_speed_set
 * Purpose:
 *      Set PHY speed
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      speed - link speed in Mbps
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
phy_84756_fcmap_speed_set(int unit, soc_port_t port, int speed)
{
    phy_ctrl_t *pc;
    phy_ctrl_t  *int_pc;
    int            rv; 

    pc = EXT_PHY_SW_STATE(unit, port);
    int_pc = INT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        if (NULL != int_pc) {
            PHY_AUTO_NEGOTIATE_SET (int_pc->pd, unit, port, 0);
            PHY_SPEED_SET(int_pc->pd, unit, port, 10000);
        }
        return SOC_E_NONE;
    }
#endif

    if (speed > 10000) {
        return SOC_E_UNAVAIL;
    }

    if(PHY_FIBER_MODE(unit, port)) {
        if (speed < 1000) {
            return SOC_E_UNAVAIL;
        }
    }

    rv = bfcmap_phy84756_speed_set(pc, speed);
    if(SOC_SUCCESS(rv)) {
        if (NULL != int_pc) {
            PHY_AUTO_NEGOTIATE_SET (int_pc->pd, unit, port, 0);
            PHY_SPEED_SET(int_pc->pd, unit, port, speed);
        }
        if(PHY_COPPER_MODE(unit, port)) {
            pc->copper.force_speed = speed;
        } else {
            if(PHY_FIBER_MODE(unit, port)) {
                pc->fiber.force_speed = speed;
            }
        }
    }
    SOC_DEBUG_PRINT((DK_PHY,
                     "phy_84756_fcmap_speed_set: u=%d p=%d s=%d fiber=%d rv=%d\n",
                     unit, port, speed, PHY_FIBER_MODE(unit, port), rv));

    return rv;
}

/*
 * Function:
 *      phy_84756_fcmap_speed_get
 * Purpose:
 *      Get the current operating speed for a 84756 device.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      speed - (OUT) Speed of the phy
 * Returns:
 *      SOC_E_NONE
 * Notes:
 *      The speed is retrieved for the ACTIVE medium.
 */

STATIC int
phy_84756_fcmap_speed_get(int unit, soc_port_t port, int *speed)
{
    int rv = SOC_E_NONE;
    phy_ctrl_t    *pc;
    buint16_t data = 0;
#if defined(INCLUDE_FCMAP)
    buint16_t fval = 0;
#endif
    bfcmap_phy84756_intf_t line_mode;

    if (speed == NULL) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);

#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        BFCMAP_PHY84756_REG_RD(pc, 0, 0x1e, 0x50, &fval);
        *speed = 0;
        if (fval == 0x1a) { /* Link up event */
            BFCMAP_PHY84756_REG_RD(pc, 0, 0x1e, 0x52, &fval);
            switch (fval) {
            case 8: 
                *speed = 8000;
                break;
            case 4: 
                *speed = 4000;
                break;
            case 2: 
                *speed = 2000;
                break;
            default:
                *speed = 0;
                break;
            }
        }
    } else
#endif
    {
        SOC_IF_ERROR_RETURN(
            bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

        if (line_mode == BFCMAP_PHY84756_INTF_SFI) {
            *speed = 10000;
            return SOC_E_NONE;
        }
        if (line_mode == BFCMAP_PHY84756_INTF_1000X) {
            *speed = 1000;
            return SOC_E_NONE;
        }
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV1_SPEED_LINK_DETECT_STATr(pc, &data));
        switch(data & BFCMAP_PHY84756_PMD_SPEED_LD_STATr_LN_PDM_SPEED_MASK) {
        case BFCMAP_PHY84756_PMD_SPEED_LD_STATr_LN_PDM_SPEED_10M:
            *speed = 10;
            break;
        case BFCMAP_PHY84756_PMD_SPEED_LD_STATr_LN_PDM_SPEED_100M:
            *speed = 100;
            break;
        case BFCMAP_PHY84756_PMD_SPEED_LD_STATr_LN_PDM_SPEED_1G:
            *speed = 1000;
            break;
        case BFCMAP_PHY84756_PMD_SPEED_LD_STATr_LN_PDM_SPEED_10G:
            *speed = 10000;
            break;
        default :
            *speed = 0;
            break;
        }
    }

    if(!SOC_SUCCESS(rv)) {
        SOC_DEBUG_PRINT((DK_WARN,
                        "phy_84756_fcmap_speed_get: u=%d p=%d invalid speed\n",
                         unit, port));
    } else {
        SOC_DEBUG_PRINT((DK_PHY,
                        "phy_84756_fcmap_speed_get: u=%d p=%d speed=%d",
                         unit, port, *speed));
    }
    return rv;
}

/*
 * Function:
 *      phy_84756_fcmap_an_set
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
phy_84756_fcmap_an_set(int unit, soc_port_t port, int autoneg)
{
    int                 rv;
    phy_ctrl_t   *pc;
    buint16_t data = 0;
    bfcmap_phy84756_intf_t line_mode;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;
#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        return SOC_E_NONE;
    }
#endif


    SOC_IF_ERROR_RETURN(
        bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

    if (line_mode == BFCMAP_PHY84756_INTF_SFI ) {
        /* force speed  ie an=f */
        autoneg = 0 ;
    }

    /* 
     *  Autoneg
     *        enabled       disabled
     *      7.8301=0007    
     *      1.0000=2040    1.0000=0040
     *      7.8309=0010    7.8309=0030
     *      7.FFE0=1100    7.FFE0=0100
     */

    if (autoneg) {
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV7_AN_BASE1000X_CTRL2r(pc, 0x07, 0x7));
    }

    if (line_mode != BFCMAP_PHY84756_INTF_SFI ) {
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV1_PMD_CTRLr(pc, autoneg ? 0x2040 : 0x0040, 0x2040));
    }

    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_LN_DEV7_AN_MSIC2r(pc, autoneg ? 0x10 : 0x30, 0x30));

    SOC_IF_ERROR_RETURN(
        BFCMAP_RD_PHY84756_LN_DEV7_AN_MII_CTRLr(pc, &data));
    if (autoneg) {
        data |= (BFCMAP_PHY84756_AN_MII_CTRL_AE | 
                 BFCMAP_PHY84756_AN_MII_CTRL_FD | 
                 BFCMAP_PHY84756_AN_MII_CTRL_RAN);
    } else {
        data &= ~(BFCMAP_PHY84756_AN_MII_CTRL_AE |
                  BFCMAP_PHY84756_AN_MII_CTRL_RAN);
        data |=  BFCMAP_PHY84756_AN_MII_CTRL_FD;
    }
    SOC_IF_ERROR_RETURN(
        BFCMAP_WR_PHY84756_LN_DEV7_AN_MII_CTRLr(pc, data));

    if(SOC_SUCCESS(rv)) {
        if(PHY_COPPER_MODE(unit, port)) {
            pc->copper.autoneg_enable = autoneg ? TRUE : FALSE;
        } else {
            if(PHY_FIBER_MODE(unit, port)) {
                pc->fiber.autoneg_enable = autoneg ? TRUE : FALSE;
            }
        }
    }

    SOC_DEBUG_PRINT((DK_PHY,
                     "phy_84756_fcmap_an_set: u=%d p=%d autoneg=%d rv=%d\n",
                     unit, port, autoneg, rv));
    return rv;
}

/*
 * Function:
 *      phy_84756_fcmap_an_get
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
phy_84756_fcmap_an_get(int unit, soc_port_t port,
                     int *autoneg, int *autoneg_done)
{
    int           rv;
    phy_ctrl_t   *pc;
    buint16_t an_mii_ctrl = 0;
    bfcmap_phy84756_intf_t line_mode;

    pc = EXT_PHY_SW_STATE(unit, port);

    rv            = SOC_E_NONE;
    *autoneg      = FALSE;
    *autoneg_done = FALSE;
#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        int bfcmap_link;
        *autoneg = TRUE;  /* Always in Autoneg */
        rv = bfcmap_phy84756_link_get(pc, &bfcmap_link);
        if (!SOC_SUCCESS(rv)) {
            return SOC_E_FAIL;
        }
        *autoneg_done = bfcmap_link ? TRUE : FALSE;
        return SOC_E_NONE;
    }
#endif
    SOC_IF_ERROR_RETURN(
        bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

    if (line_mode == BFCMAP_PHY84756_INTF_SFI ) {
        *autoneg      = FALSE;
        *autoneg_done = FALSE;
    } else {
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV7_AN_MII_CTRLr(pc, &an_mii_ctrl));
        *autoneg = (an_mii_ctrl & BFCMAP_PHY84756_AN_MII_CTRL_AE) ? 1 : 0;
        *autoneg_done = 
            (pc->mii_stat & BFCMAP_PHY84756_AN_MII_STAT_AN_DONE) ? 1 : 0;
    }

    return rv;
}

/*
 * Function:
 *      phy_84756_fcmap_lb_set
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
phy_84756_fcmap_lb_set(int unit, soc_port_t port, int enable)
{
    int           rv;
    phy_ctrl_t    *pc;
    buint16_t data = 0;
    bfcmap_phy84756_intf_t line_mode;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    SOC_IF_ERROR_RETURN(
        bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

    if (line_mode == BFCMAP_PHY84756_INTF_SFI) {
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV1_PMD_CTRLr(pc, &data));
        data &= ~(1);
        data |= (enable) ? 1 : 0x0000;
        SOC_IF_ERROR_RETURN(
            BFCMAP_WR_PHY84756_LN_DEV1_PMD_CTRLr(pc, data));
    } else {
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV7_AN_MII_CTRLr(pc, &data));
        data &= ~BFCMAP_PHY84756_AN_MII_CTRL_LE;
        data |= (enable) ? BFCMAP_PHY84756_AN_MII_CTRL_LE : 0;
        SOC_IF_ERROR_RETURN(
            BFCMAP_WR_PHY84756_LN_DEV7_AN_MII_CTRLr(pc, data));
    }
    if (rv == SOC_E_NONE) {
        /* wait for link up when loopback is enabled */
        
        if (enable) {
            sal_usleep(2000000);
        }
    }
    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_84756_fcmap_lb_set: u=%d p=%d en=%d rv=%d\n", 
                    unit, port, enable, rv));

    return rv; 
}

/*
 * Function:
 *      phy_84756_fcmap_lb_get
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
phy_84756_fcmap_lb_get(int unit, soc_port_t port, int *enable)
{
    int                  rv;
    phy_ctrl_t    *pc;
    buint16_t data = 0;
    bfcmap_phy84756_intf_t line_mode;

    pc = EXT_PHY_SW_STATE(unit, port);
    rv = SOC_E_NONE;

    SOC_IF_ERROR_RETURN(
        bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

    if (line_mode == BFCMAP_PHY84756_INTF_SFI) {
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV1_PMD_CTRLr(pc, &data));
        *enable = (data & 1) ? 1 : 0;
    } else {
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV7_AN_MII_CTRLr(pc, &data));
        *enable = (data & BFCMAP_PHY84756_AN_MII_CTRL_LE) ? 1 : 0;
    }

    return rv; 
}


/*
 * Function:
 *      phy_84756_fcmap_ability_advert_set
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
phy_84756_fcmap_ability_advert_set(int unit, soc_port_t port,soc_port_ability_t *ability)
{
    int        rv;
    phy_ctrl_t *pc;
    bfcmap_phy84756_port_ability_t local_ability;

    pc = EXT_PHY_SW_STATE(unit, port);
#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        return SOC_E_NONE;
    }
#endif

    if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE)) {
        return SOC_E_NONE;
    }

    if (NULL == ability) {
        return (SOC_E_PARAM);
    }

    if (PHY_COPPER_MODE(unit, port)) {
        pc->copper.advert_ability = *ability;
    } else {
        pc->fiber.advert_ability = *ability;
    }

    local_ability = 0;

    if(ability->speed_full_duplex & SOC_PA_SPEED_1000MB) {
        local_ability |= BFCMAP_PHY84756_PA_1000MB_FD;
    }
    if(ability->speed_full_duplex & SOC_PA_SPEED_10GB) {
        local_ability |= BFCMAP_PHY84756_PA_10000MB_FD;
    }

    if(ability->pause & SOC_PA_PAUSE_TX) {
        local_ability |= BFCMAP_PHY84756_PA_PAUSE_TX;
    }
    if(ability->pause & SOC_PA_PAUSE_RX) {
        local_ability |= BFCMAP_PHY84756_PA_PAUSE_RX;
    }
    if(ability->pause & SOC_PA_PAUSE_ASYMM) {
        local_ability |= BFCMAP_PHY84756_PA_PAUSE_ASYMM;
    }

    if(ability->loopback & SOC_PA_LB_NONE) {
        local_ability |= BFCMAP_PHY84756_PA_LB_NONE;
    }
    if(ability->loopback & SOC_PA_LB_PHY) {
        local_ability |= BFCMAP_PHY84756_PA_LB_PHY;
    }
    if(ability->flags & SOC_PA_AUTONEG) {
        local_ability |= BFCMAP_PHY84756_PA_AN;
    }

    rv = bfcmap_phy84756_ability_advert_set(pc, local_ability);

    return rv;
}

/*
 * Function:
 *      phy_84756_fcmap_ability_advert_get
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
phy_84756_fcmap_ability_advert_get(int unit, soc_port_t port,soc_port_ability_t *ability)
{
    phy_ctrl_t      *pc;
    int        rv;
    bfcmap_phy84756_port_ability_t local_ability;

    if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE)) {
        return SOC_E_NONE;
    }

    if (NULL == ability) {
        return (SOC_E_PARAM);
    }

    pc = EXT_PHY_SW_STATE(unit, port);
#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        ability->pause |= SOC_PA_PAUSE_TX;
        ability->pause |= SOC_PA_PAUSE_RX;
        return SOC_E_NONE;
    }
#endif

    local_ability = 0;

    rv = bfcmap_phy84756_ability_advert_get(pc, &local_ability);
    if (rv != SOC_E_NONE) {
        return SOC_E_FAIL;
    }

    if(local_ability & BFCMAP_PHY84756_PA_1000MB_FD) {
        ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
    }
    if(local_ability & BFCMAP_PHY84756_PA_10000MB_FD) {
        ability->speed_full_duplex |= SOC_PA_SPEED_10GB;
    }

    if(local_ability & BFCMAP_PHY84756_PA_PAUSE_TX) {
        ability->pause |= SOC_PA_PAUSE_TX;
    }
    if(local_ability & BFCMAP_PHY84756_PA_PAUSE_RX) {
        ability->pause |= SOC_PA_PAUSE_RX;
    }
    if(local_ability & BFCMAP_PHY84756_PA_PAUSE_ASYMM) {
        ability->pause |= SOC_PA_PAUSE_ASYMM;
    }

    if(local_ability & BFCMAP_PHY84756_PA_LB_NONE) {
        ability->loopback |= SOC_PA_LB_NONE;
    }
    if(local_ability & BFCMAP_PHY84756_PA_LB_PHY) {
        ability->loopback |= SOC_PA_LB_PHY;
    }
    if(local_ability & BFCMAP_PHY84756_PA_AN) {
        ability->flags = SOC_PA_AUTONEG;
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_84756_fcmap_ability_remote_get
 * Purpose:
 *      Get the current remote advertisement for auto-negotiation.
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
phy_84756_fcmap_ability_remote_get(int unit, soc_port_t port,soc_port_ability_t *ability)
{
    phy_ctrl_t      *pc;
    int        rv;
    bfcmap_phy84756_port_ability_t remote_ability;

    if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE)) {
        return SOC_E_NONE;
    }

    if (NULL == ability) {
        return (SOC_E_PARAM);
    }

    pc = EXT_PHY_SW_STATE(unit, port);
#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        ability->pause |= SOC_PA_PAUSE_TX;
        ability->pause |= SOC_PA_PAUSE_RX;
        return SOC_E_NONE;
    }
#endif

    remote_ability = 0;

    rv = bfcmap_phy84756_remote_ability_advert_get(pc,
                                                   &remote_ability);
    if (rv != SOC_E_NONE) {
        return SOC_E_FAIL;
    }

    if(remote_ability & BFCMAP_PHY84756_PA_1000MB_FD) {
        ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
    }
    if(remote_ability & BFCMAP_PHY84756_PA_10000MB_FD) {
        ability->speed_full_duplex |= SOC_PA_SPEED_10GB;
    }

    if(remote_ability & BFCMAP_PHY84756_PA_PAUSE_TX) {
        ability->pause |= SOC_PA_PAUSE_TX;
    }
    if(remote_ability & BFCMAP_PHY84756_PA_PAUSE_RX) {
        ability->pause |= SOC_PA_PAUSE_RX;
    }
    if(remote_ability & BFCMAP_PHY84756_PA_PAUSE_ASYMM) {
        ability->pause |= SOC_PA_PAUSE_ASYMM;
    }

    if(remote_ability & BFCMAP_PHY84756_PA_LB_NONE) {
        ability->loopback |= SOC_PA_LB_NONE;
    }
    if(remote_ability & BFCMAP_PHY84756_PA_LB_PHY) {
        ability->loopback |= SOC_PA_LB_PHY;
    }
    if(remote_ability & BFCMAP_PHY84756_PA_AN) {
        ability->flags = SOC_PA_AUTONEG;
    }

    return SOC_E_NONE;
}



/*
 * Function:
 *      phy_84756_fcmap_ability_local_get
 * Purpose:
 *      Get local abilities 
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
phy_84756_fcmap_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    int         rv = SOC_E_NONE;
    phy_ctrl_t *pc;
    bfcmap_phy84756_port_ability_t local_ability = 0;

    if (NULL == ability) {
        return SOC_E_PARAM;
    }

    pc = EXT_PHY_SW_STATE(unit, port);
#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        ability->pause |= SOC_PA_PAUSE_TX;
        ability->pause |= SOC_PA_PAUSE_RX;
        return SOC_E_NONE;
    }
#endif

    rv = bfcmap_phy84756_ability_local_get(pc, &local_ability);
    if (rv != SOC_E_NONE) {
        return SOC_E_FAIL;
    }

    if(local_ability & BFCMAP_PHY84756_PA_1000MB_FD) {
        ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
    }
    if(local_ability & BFCMAP_PHY84756_PA_10000MB_FD) {
        ability->speed_full_duplex |= SOC_PA_SPEED_10GB;
    }

    if(local_ability & BFCMAP_PHY84756_PA_PAUSE_TX) {
        ability->pause |= SOC_PA_PAUSE_TX;
    }
    if(local_ability & BFCMAP_PHY84756_PA_PAUSE_RX) {
        ability->pause |= SOC_PA_PAUSE_RX;
    }
    if(local_ability & BFCMAP_PHY84756_PA_PAUSE_ASYMM) {
        ability->pause |= SOC_PA_PAUSE_ASYMM;
    }
    if(local_ability & BFCMAP_PHY84756_PA_LB_NONE) {
        ability->loopback |= SOC_PA_LB_NONE;
    }
    if(local_ability & BFCMAP_PHY84756_PA_LB_PHY) {
        ability->loopback |= SOC_PA_LB_PHY;
    }
    if(local_ability & BFCMAP_PHY84756_PA_AN) {
        ability->flags = SOC_PA_AUTONEG;
    }

    return SOC_E_NONE;
}

STATIC int
_phy84756_init_ucode_bcst(int unit, int port, uint32 cmd)
{
    uint16 data16;
    uint16 mask16;
    int j;
    uint16 num_words;
    uint8 *fw_ptr;
    int fw_length;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (cmd == PHYCTRL_UCODE_BCST_SETUP) {
        SOC_DEBUG_PRINT((DK_PHY,
                         "PHY84756 BCST start: u=%d p=%d\n", unit, port));

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc8fe, 0xffff));

        return SOC_E_NONE;
    } else if (cmd == PHYCTRL_UCODE_BCST_uC_SETUP) {
        SOC_DEBUG_PRINT((DK_PHY,
                  "PHY84756 BCST1: u=%d p=%d\n", unit, port));
        /* clear SPA ctrl reg bit 15 and bit 13.
         * bit 15, 0-use MDIO download to SRAM, 1 SPI-ROM download to SRAM
         * bit 13, 0 clear download done status, 1 skip download
         */
        mask16 = (1 << 13) | (1 << 15);
        data16 = 0;

        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_SPI_CTRL_STATr(pc, data16, mask16));

        /* set SPA ctrl reg bit 14, 1 RAM boot, 0 internal ROM boot */
        mask16 = 1 << 14;
        data16 = mask16;

        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_SPI_CTRL_STATr(pc, data16, mask16));

        /* misc_ctrl1 reg bit 3 to 1 for 32k downloading size */
        mask16 = 1 << 3;
        data16 = mask16;

        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_MISC_CNTL2r(pc, data16, mask16));

        /* apply bcst Reset to start download code from MDIO */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMD_CTRLr(pc, 0x8000));

        return SOC_E_NONE;
    } else if (cmd == PHYCTRL_UCODE_BCST_ENABLE) {
        SOC_DEBUG_PRINT((DK_PHY,
                  "PHY84756 BCST2: u=%d p=%d\n", unit, port));

        /* reset clears bcst register */
        /* restore bcst register after reset */

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc8fe, 0xffff));

        return SOC_E_NONE;
    } else if (cmd == PHYCTRL_UCODE_BCST_LOAD) {
        uint16 saved_phy_addr;

        SOC_DEBUG_PRINT((DK_PHY, "firmware_bcst,device name %s: u=%d p=%d\n",
                         pc->dev_name? pc->dev_name: "NULL", unit, port));

        saved_phy_addr = pc->phy_id;
        pc->phy_id &= ~0x1f; /* broadcast on address 0 of the bus */
        fw_ptr =  FIRMWARE(pc);
        fw_length =  FIRMWARE_LEN(pc);

        /* wait for 2ms for M8051 start and 5ms to initialize the RAM */
        sal_usleep(10000); /* Wait for 10ms */

        /* Write Starting Address, where the Code will reside in SRAM */
        data16 = 0x8000;
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_M8501_MSGINr(pc, data16));

        /* make sure address word is read by the micro */
        sal_udelay(10); /* Wait for 10us */

        /* Write SPI SRAM Count Size */
        data16 = (fw_length)/2;
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_M8501_MSGINr(pc, data16));

        /* make sure read by the micro */
        sal_udelay(10); /* Wait for 10us */

        /* Fill in the SRAM */
        num_words = (fw_length - 1);
        for (j = 0; j < num_words; j+=2) {
            /* Make sure the word is read by the Micro */
            sal_udelay(10);

            data16 = (fw_ptr[j] << 8) | fw_ptr[j+1];

            SOC_IF_ERROR_RETURN
                (BFCMAP_WR_PHY84756_LN_DEV1_M8501_MSGINr(pc, data16));
        }
        pc->phy_id = saved_phy_addr;
        return SOC_E_NONE;
    } else if (cmd == PHYCTRL_UCODE_BCST_END) {
        SOC_DEBUG_PRINT((DK_PHY,
                  "PHY84756 BCST end: u=%d p=%d\n", unit, port));

        /* make sure last code word is read by the micro */
        sal_udelay(20);

        /* first disable bcst mode */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc8fe, 0x0));

        /* Read Hand-Shake message (Done) from Micro */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_M8501_MSGOUTr(pc, &data16));

        /* Download done message */
        SOC_DEBUG_PRINT((DK_PHY, "u=%d p=%d MDIO firmware download done message: 0x%x\n",
                     unit, port,data16));

        /* Clear LASI status */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_RX_ALARM_STATr(pc, &data16));

        /* Wait for LASI to be asserted when M8051 writes checksum to MSG_OUTr */
        sal_udelay(100); /* Wait for 100 usecs */

        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_M8501_MSGOUTr(pc, &data16));

        /* Need to check if checksum is correct */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_CHECKSUMr(pc, &data16));

        if (data16 != 0x600D) {
            /* Bad CHECKSUM */
            soc_cm_print("firmware_bcst downlad failure: port %d "
                     "Incorrect Checksum %x\n", port,data16);
            return SOC_E_FAIL;
        }

        /* read Rev-ID */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xCA1A, &data16));
        soc_cm_print("PHY84756: MDIO broadcast firmware download complete: u=%d p=%d version=%x\n",
                     unit, port, data16);
        return SOC_E_NONE;
    } else {
        SOC_DEBUG_PRINT((DK_WARN, "u=%d p=%d firmware_bcst: invalid cmd 0x%x\n",
                         unit, port,cmd));
    }

    return SOC_E_FAIL;
}

/*
 * Function:
 *      phy_84756_firmware_set
 * Purpose:
 *      program the given firmware into the SPI-ROM
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      offset - offset to the data stream
 *      array  - the given data
 *      datalen- the data length
 * Returns:
 *      SOC_E_NONE
 */

STATIC int
phy_84756_fcmap_firmware_set(int unit, int port, int offset, uint8 *data,int len)
{
    int            rv;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* overload this function a littl bit if data == NULL
     * special handling for init. uCode broadcast. Internal use only
     */
    if (data == NULL) {
        return _phy84756_init_ucode_bcst(unit,port,(uint32)offset);
    }

#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        return SOC_E_NONE;
    }
#endif
    rv = bfcmap_phy84756_spi_firmware_update(pc, data, len);
    if (rv != SOC_E_NONE) {
            SOC_DEBUG_PRINT((DK_WARN,
                            "PHY84756 firmware upgrade possibly failed:"
                            "u=%d p=%d\n", unit, port));
        return (SOC_E_FAIL);
    }
    SOC_DEBUG_PRINT((DK_PHY,"PHY84756 firmware upgrade successful:"
                             "u=%d p=%d\n", unit, port));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_84756_fcmap_medium_status
 * Purpose:
 *      Indicate the current active medium
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      medium - (OUT) One of:
 *              SOC_PORT_MEDIUM_COPPER
 *              SOC_PORT_MEDIUM_XAUI
 * Returns:
 *      SOC_E_NONE
 */

STATIC int
phy_84756_fcmap_medium_status(int unit, soc_port_t port, soc_port_medium_t *medium)
{
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(port);

    *medium = SOC_PORT_MEDIUM_FIBER;

    return SOC_E_NONE;
}

STATIC int
_phy_84756_control_tx_driver_set(int unit, soc_port_t port,
                                soc_phy_control_t type, uint32 value)
{
    uint16       data;  /* Temporary holder of reg value to be written */
    uint16       mask;  /* Bit mask of reg value to be updated */
    uint16 post_tap;
    uint16 main_tap;
    phy_ctrl_t  *pc;    /* PHY software state */
      
    pc = EXT_PHY_SW_STATE(unit, port);
    switch(type) {
        case SOC_PHY_CONTROL_DRIVER_CURRENT:
        /* fall through */
            data = (uint16)(value & 0xf);
            data = data << 12;
            mask = 0xf000;
            SOC_IF_ERROR_RETURN
                (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr( pc, 0xca01, data, mask));
        break;

        case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT:
        /* fall through */
            data = (uint16)(value & 0xf);
            data = data << 8;
            mask = 0x0f00;
            SOC_IF_ERROR_RETURN
                (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr( pc, 0xca01, data, mask));
            break;
        case SOC_PHY_CONTROL_PREEMPHASIS:
        /* The value format:
         * bit 15:  1 enable forced preemphasis ctrl, 0 auto config
         * bit 14:10:  post_tap value
         * bit 09:04:  main tap value 
         * 
         * set 1.CA05[15] = 0, 1.CA02[15:11] for main_tap and 1.CA05[7:4] for post tap.
         * main tap control is in the register field 0xCA02[15:11]. 
         * If a wrong value is written to the same register, field 0xCA02[9:7],
         *  it may DAMAGE the device. User must avoid writing to 0xCA02[9:7] 
         */ 
        /* Register bit15, 0 force preemphasis, 1 auto config, 1.CA05 */ 
        post_tap = PHY84756_PREEMPH_GET_FORCE(value)? 0:
                   PHY84756_PREEMPH_REG_FORCE_MASK;

        /* add post tap value */
        post_tap |= PHY84756_PREEMPH_GET_POST_TAP(value) << 
                    PHY84756_PREEMPH_REG_POST_TAP_SHFT;

        /* main tap in 1.CA02 */
        main_tap = PHY84756_PREEMPH_GET_MAIN_TAP(value) <<
                   PHY84756_PREEMPH_REG_MAIN_TAP_SHFT;


        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr( pc, 0xca05, 
                      post_tap, PHY84756_PREEMPH_REG_POST_TAP_MASK |
                                PHY84756_PREEMPH_REG_FORCE_MASK));

        /* always clear tx_pwrdn bit. The read/modify/write may accidently
         * set the bit 0xCA02[10]. Because the CA02[10] read value is: 
         * CA02[10]_read_value = CA02[10] OR Digital_State_Machine 
         * The problem will occur when the Digital_State_Machine output is 1
         * at the time of reading this register.
         */ 
        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr( pc, 0xca02, 
                      main_tap, 
                PHY84756_PREEMPH_REG_MAIN_TAP_MASK |
                PHY84756_PREEMPH_REG_TX_PWRDN_MASK));
        break;
        
        default:
         /* should never get here */
         return SOC_E_PARAM;
    }
    return SOC_E_NONE;
}

STATIC int
_phy_84756_control_tx_driver_get(int unit, soc_port_t port,
                                soc_phy_control_t type, uint32 *value)
{
    uint16         data16;   /* Temporary holder of a reg value */
    uint16         main_tap;
    uint16         post_tap;
    phy_ctrl_t    *pc;       /* PHY software state */

    pc = EXT_PHY_SW_STATE(unit, port);

    data16 = 0;
    switch(type) {
        case SOC_PHY_CONTROL_PREEMPHASIS:
            /* 1.CA05[15] = 0, set 1.CA02[15:11] for main_tap and 
             * 1.CA05[7:4] for postcursor 
             */
            SOC_IF_ERROR_RETURN
                (BFCMAP_RD_PHY84756_LN_DEV1_PMDr( pc, 0xca02, &main_tap));
            SOC_IF_ERROR_RETURN
                (BFCMAP_RD_PHY84756_LN_DEV1_PMDr( pc, 0xca05, &post_tap));

            /* bit15 force flag, 14:10 post_tap, 09:04 main tap */
            *value = (post_tap & PHY84756_PREEMPH_REG_FORCE_MASK)? 0:
                     (1 << PHY84756_PREEMPH_CTRL_FORCE_SHFT); 
            *value |= ((post_tap & PHY84756_PREEMPH_REG_POST_TAP_MASK) >>
                      PHY84756_PREEMPH_REG_POST_TAP_SHFT) << 
                      PHY84756_PREEMPH_CTRL_POST_TAP_SHFT; 
            *value |= ((main_tap & PHY84756_PREEMPH_REG_MAIN_TAP_MASK) >>
                      PHY84756_PREEMPH_REG_MAIN_TAP_SHFT) <<
                      PHY84756_PREEMPH_CTRL_MAIN_TAP_SHFT; 

        break;

        case SOC_PHY_CONTROL_DRIVER_CURRENT:
            SOC_IF_ERROR_RETURN
                (BFCMAP_RD_PHY84756_LN_DEV1_PMDr( pc, 0xca01, &data16));
            *value = (data16 & 0xf000) >> 12;
        break;

        case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT:
            SOC_IF_ERROR_RETURN
                (BFCMAP_RD_PHY84756_LN_DEV1_PMDr( pc, 0xca01, &data16));
            *value = (data16 & 0x0f00) >> 8;
            break;

       default:
         /* should never get here */
         return SOC_E_PARAM;
    }

    return SOC_E_NONE;
}

STATIC int
_phy_84756_control_edc_mode_set(int unit, soc_port_t port, uint32 value)
{
    uint16         data16;
    uint16         mask16;
    phy_ctrl_t    *pc;       /* PHY software state */

    pc = EXT_PHY_SW_STATE(unit, port);


    /* EDC mode programming sequence*/     
    mask16 = 1 << 9;

    /* induce LOS condition: toggle register bit 0xc800.9 */
    SOC_IF_ERROR_RETURN         
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 
                                   0xc800,&data16));

    /* only change toggled bit 9 */
    SOC_IF_ERROR_RETURN
        (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 
             0xc800,~data16,mask16));

    /* program EDC mode */
    SOC_IF_ERROR_RETURN
      (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xCA1A,(uint16)value,
                              PHY84756_EDC_MODE_MASK));

    /* remove LOS condition: restore back original value of bit 0xc800.9 */
    SOC_IF_ERROR_RETURN
        (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 
               0xc800,data16,mask16));

    return SOC_E_NONE;
}


STATIC int
_phy_84756_control_prbs_tx_invert_data_set(int unit, soc_port_t port, int invert)
{
    phy_ctrl_t    *pc;       /* PHY software state */
    uint16 data16;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* tx inversion */
    data16 = invert? PHY84756_USER_PRBS_TX_INVERT: 0;
    
    SOC_IF_ERROR_RETURN
        (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr( pc, 
              PHY84756_USER_PRBS_CONTROL_0_REGISTER, data16,
                       PHY84756_USER_PRBS_TX_INVERT));
    return SOC_E_NONE;
}

STATIC int
_phy_84756_control_prbs_rx_invert_data_set(int unit, soc_port_t port, int invert)
{
    phy_ctrl_t    *pc;       /* PHY software state */
    uint16 data16;
    uint16 mask16;

    pc = EXT_PHY_SW_STATE(unit, port);

    /* rx inversion */
    data16 = invert? PHY84756_USER_PRBS_RX_INVERT: 0;
    mask16 = PHY84756_USER_PRBS_RX_INVERT;
    SOC_IF_ERROR_RETURN
        (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr( pc,
              PHY84756_USER_PRBS_CONTROL_0_REGISTER, data16,
                       mask16));
    return SOC_E_NONE;
}

STATIC int
_phy_84756_control_prbs_polynomial_set(int unit, soc_port_t port,int poly_ctrl,
         int tx)   /* tx or rx */
{
    phy_ctrl_t    *pc;       /* PHY software state */
    uint16 data16;
    uint16 mask16;

        /*
         * poly_ctrl:
         *  0x0 = prbs7
         *  0x1 = prbs15
         *  0x2 = prbs23
         *  0x3 = prbs31
         */
    pc = EXT_PHY_SW_STATE(unit, port);

    /* 001 : prbs7 
       010 : prbs9
       011 : prbs11
       100 : prbs15
       101 : prbs23
       110 : prbs31
     */

    if (poly_ctrl == 0) {
        data16 = 1;
    } else if (poly_ctrl == 1) {
        data16 = 4;
    } else if (poly_ctrl == 2) {
        data16 = 5;
    } else if (poly_ctrl == 3) {
        data16 = 6;
    } else {
        return SOC_E_PARAM;
    }  
    mask16 = PHY84756_USER_PRBS_TYPE_MASK;

    /* default is tx */
    if (!tx) {  /* rx */
        data16 = data16 << 12;
        mask16 = mask16 << 12;
    }
        
    /* both tx/rx types */  
    SOC_IF_ERROR_RETURN
        (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr( pc, 
                PHY84756_USER_PRBS_CONTROL_0_REGISTER, 
           data16, mask16));

    return SOC_E_NONE;
}
STATIC int
_phy_84756_control_prbs_enable_set(int unit, soc_port_t port, int enable)
{
    phy_ctrl_t    *pc;       /* PHY software state */
    int repeater_mode;
    uint16 data16;

    pc = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc81d, &data16));

    repeater_mode = ((data16 & 0x6) == 0x6)? TRUE: FALSE;
    if (repeater_mode) {
        /* Enable retimer prbs clocks inside SFI */
        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr( pc, 
                      PHY84756_GENSIG_8071_REGISTER, 
                            enable?0x3:0,0x3));

        /* Enable the retimer datapath */
        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr( pc, 0xcd58, 
                        enable?0x2:0,0x3));
    }
    /* clear PRBS control if disabled */
    if (!enable) {
        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr( pc, 
                       PHY84756_USER_PRBS_CONTROL_0_REGISTER, 0,
                         PHY84756_USER_PRBS_TYPE_MASK |
                         PHY84756_USER_PRBS_TYPE_MASK << 12)); /* rx type*/
    }         

    /*  program the PRBS enable */
    SOC_IF_ERROR_RETURN
        (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr( pc, 
                 PHY84756_USER_PRBS_CONTROL_0_REGISTER, 
                 enable? PHY84756_USER_PRBS_ENABLE: 0,PHY84756_USER_PRBS_ENABLE));

    return SOC_E_NONE;
}

STATIC int
_phy_84756_control_prbs_enable_get(int unit, soc_port_t port, uint32 *value)
{
    uint16 data16;
    phy_ctrl_t    *pc;       /* PHY software state */

    pc = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr( pc, 
               PHY84756_USER_PRBS_CONTROL_0_REGISTER, &data16));
    if (data16 & PHY84756_USER_PRBS_ENABLE) {
        *value = TRUE;
    } else {
        *value = FALSE;
    }

    return SOC_E_NONE;
}
STATIC int
_phy_84756_control_prbs_polynomial_get(int unit, soc_port_t port, uint32 *value)
{
    phy_ctrl_t    *pc;       /* PHY software state */
    uint16 poly_ctrl;

    pc = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr( pc, 
                   PHY84756_USER_PRBS_CONTROL_0_REGISTER, &poly_ctrl));
    poly_ctrl &= PHY84756_USER_PRBS_TYPE_MASK;

    if (poly_ctrl == 1) {
        *value = 0;
    } else if (poly_ctrl == 4) {
        *value = 1;
    } else if (poly_ctrl == 5) {
        *value = 2;
    } else if (poly_ctrl == 6) {
        *value = 3;
    } else {
        *value = poly_ctrl;
    }

    return SOC_E_NONE;
}

STATIC int
_phy_84756_control_prbs_tx_invert_data_get(int unit, soc_port_t port, uint32 *value)
{
    phy_ctrl_t    *pc;       /* PHY software state */
    uint16 data16;

    pc = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr( pc, 
                  PHY84756_USER_PRBS_CONTROL_0_REGISTER, &data16));
    if (data16 & PHY84756_USER_PRBS_TX_INVERT) {
        *value = TRUE;
    } else {
        *value = FALSE;
    }
    return SOC_E_NONE;
}

STATIC int
_phy_84756_control_prbs_rx_status_get(int unit, soc_port_t port, uint32 *value)
{
    phy_ctrl_t    *pc;       /* PHY software state */
    uint16 data16;

    pc = EXT_PHY_SW_STATE(unit, port);

    *value = 0;
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr( pc, 
                      PHY84756_USER_PRBS_STATUS_0_REGISTER, &data16));
    if (data16 == 0x8000) { /* PRBS lock and error free */
        *value = 0;
    } else if (!(data16 & 0x8000)) {
        *value = -1;
    } else {
        *value = data16 & 0x7fff;
    }

    return SOC_E_NONE;
}

STATIC int
_phy_84756_control_edc_mode_get(int unit, soc_port_t port, uint32 *value)
{
    uint16         data16;
    phy_ctrl_t    *pc;       /* PHY software state */

    pc = EXT_PHY_SW_STATE(unit, port);

    /* get EDC mode */     
    SOC_IF_ERROR_RETURN
      (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xCA1A,&data16));

    *value = data16 & PHY84756_EDC_MODE_MASK;

    return SOC_E_NONE;
}

STATIC char *
_phy_84756_mode_name_get(int unit, soc_port_t port)
{
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);
    if (FCMAP_ENABLE(pc)) {
        return "FC";
    } else if (MACSEC_ENABLE(pc)) {
        return "MACSEC";
    }
    return "ETHERNET";
}

STATIC int
_phy_84756_firmware_info_dump(int unit, soc_port_t port)
{   
    uint16 sts;
    uint16 version;
    uint16 cksum;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xCA1A, &version));
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_SPI_CTRL_STATr(pc, &sts));
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_CHECKSUMr(pc, &cksum));

    soc_cm_print("    Firmware: version=%04x, download status=%04x, checksum=%04x\n", 
                 version, sts, cksum);

    return SOC_E_NONE;
}

STATIC int
_phy_84756_control_dump(int unit, soc_port_t port)
{
    phy_ctrl_t *pc;
    uint16 chip_rev_id;

    pc = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xC801, &chip_rev_id));

    soc_cm_print("Port %d: chiprev=%04x, mode=%s, MACSEC support=%s\n", 
                 port, chip_rev_id, _phy_84756_mode_name_get(unit, port),
                 MACSEC_SUPPORT(pc) ? "yes" : "no");
    SOC_IF_ERROR_RETURN(_phy_84756_firmware_info_dump(unit, port));
    return SOC_E_NONE;
}


/*
 * Function:
 *      phy_84756_control_set
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
_phy_84756_control_set(int unit, soc_port_t port, int intf,int lane,
                     soc_phy_control_t type, uint32 value)
{
    phy_ctrl_t    *pc;       /* PHY software state */
    int rv;

    pc = EXT_PHY_SW_STATE(unit, port);

    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return SOC_E_PARAM;
    }

    rv = SOC_E_UNAVAIL;
    if (intf == PHY_DIAG_INTF_SYS) {
        /* if is targeted to the system side */
        SOC_IF_ERROR_RETURN
            (PHY84756_XFI(unit,pc));
    }
    switch(type) {
    case SOC_PHY_CONTROL_PREEMPHASIS:
        /* fall through */
    case SOC_PHY_CONTROL_DRIVER_CURRENT:
        /* fall through */
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT:
        /* fall through */
        rv = _phy_84756_control_tx_driver_set(unit, port, type, value);
        break;

    case SOC_PHY_CONTROL_EDC_MODE:
        rv = _phy_84756_control_edc_mode_set(unit,port,value);
        break;
    case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
        /* on line side, rx poly type is auto-detected. On system side,
         * need to enable the rx type as well.
         */
        rv = _phy_84756_control_prbs_polynomial_set(unit, port, value, TRUE);
        if (intf == PHY_DIAG_INTF_SYS) {
            rv = _phy_84756_control_prbs_polynomial_set(unit, port,value,FALSE);
        }
        break;
    case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
        /* on line side, rx invertion is auto-detected. On system side,
         * need to enable the rx as well.
         */
        rv = _phy_84756_control_prbs_tx_invert_data_set(unit, port, value);
        if (intf == PHY_DIAG_INTF_SYS) {
            rv = _phy_84756_control_prbs_rx_invert_data_set(unit, port, value);
        }
        break;
    case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
        /* fall through */
    case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
        /* tx/rx is enabled at the same time. no seperate control */
        rv = _phy_84756_control_prbs_enable_set(unit, port, value);
        break;
#if 0
    case SOC_PHY_CONTROL_LOOPBACK_REMOTE:
        rv = _phy_84756_remote_loopback_set(unit, port,PHY_DIAG_INTF_SYS,value);
        break;
#endif
    case SOC_PHY_CONTROL_DUMP:
        _phy_84756_control_dump(unit, port);
        break;

    default:
        rv = SOC_E_UNAVAIL;
        break;
    }
    if (intf == PHY_DIAG_INTF_SYS) {
        /* if it is targeted to the system side, switch back */
        SOC_IF_ERROR_RETURN
            (PHY84756_MMF(unit,pc));
    }
    return rv;
}

/*
 * Function:
 *      phy_84756_fcmap_control_set
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
phy_84756_fcmap_control_set(int unit, soc_port_t port,
                     soc_phy_control_t type, uint32 value)
{
    int intf;

    intf = PHY_DIAG_INTF_LINE;
    SOC_IF_ERROR_RETURN
        (_phy_84756_control_set(unit, port, intf,
            PHY_DIAG_LN_DFLT, type, value));
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_84756_control_get
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
_phy_84756_control_get(int unit, soc_port_t port, int intf,int lane,
                     soc_phy_control_t type, uint32 *value)
{
    phy_ctrl_t    *pc;       /* PHY software state */
    int rv;

    pc = EXT_PHY_SW_STATE(unit, port);

    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return SOC_E_PARAM;
    }

    if (intf == PHY_DIAG_INTF_SYS) {
        /* targeted to the system side */
        SOC_IF_ERROR_RETURN
            (PHY84756_XFI(unit,pc));
    }
    rv = SOC_E_UNAVAIL;
    switch(type) {
    case SOC_PHY_CONTROL_PREEMPHASIS:
        /* fall through */
    case SOC_PHY_CONTROL_DRIVER_CURRENT:
        /* fall through */
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT:
        /* fall through */
        rv = _phy_84756_control_tx_driver_get(unit, port, type, value);
        break;

    case SOC_PHY_CONTROL_EDC_MODE:
        rv = _phy_84756_control_edc_mode_get(unit,port,value);
        break;
    case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
        rv = _phy_84756_control_prbs_polynomial_get(unit, port, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
        rv = _phy_84756_control_prbs_tx_invert_data_get(unit, port, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
        /* fall through */
    case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
        rv = _phy_84756_control_prbs_enable_get(unit, port, value);
        break;
    case SOC_PHY_CONTROL_PRBS_RX_STATUS:
        rv = _phy_84756_control_prbs_rx_status_get(unit, port, value);
        break;
    default:
        rv = SOC_E_UNAVAIL;
        break;
    }

    if (intf == PHY_DIAG_INTF_SYS) {
        /* if it is targeted to the system side, switch back */
        SOC_IF_ERROR_RETURN
            (PHY84756_MMF(unit,pc));
    }
    return rv;
}
/*
 * Function:
 *      phy_84756_fcmap_control_get
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
phy_84756_fcmap_control_get(int unit, soc_port_t port,
                     soc_phy_control_t type, uint32 *value)
{
    int intf;

    intf = PHY_DIAG_INTF_LINE;

    SOC_IF_ERROR_RETURN
        (_phy_84756_control_get(unit, port, intf,
            PHY_DIAG_LN_DFLT, type, value));
    return SOC_E_NONE;
}

STATIC int
phy_84756_diag_ctrl(
   int unit, /* unit */
   soc_port_t port, /* port */
   uint32 inst, /* the specific device block the control action directs to */
   int op_type,  /* operation types: read,write or command sequence */
   int op_cmd,   /* command code */
   void *arg)     /* command argument  */
{
    int lane;
    int intf;

    soc_cm_debug(DK_PHY,"phy_84756_diag_ctrl: u=%d p=%d ctrl=0x%x\n", 
                 unit, port,op_cmd);
   
    lane = PHY_DIAG_INST_LN(inst);
    intf = PHY_DIAG_INST_INTF(inst);
    if (intf == PHY_DIAG_INTF_DFLT) {
        intf = PHY_DIAG_INTF_LINE;
    }

    if (op_type == PHY_DIAG_CTRL_GET) { 
        SOC_IF_ERROR_RETURN
            (_phy_84756_control_get(unit, port, intf, lane,
                  op_cmd,(uint32 *)arg));
    } else if (op_type == PHY_DIAG_CTRL_SET) {
        SOC_IF_ERROR_RETURN
            (_phy_84756_control_set(unit, port, intf, lane,
                  op_cmd, PTR_TO_INT(arg)));
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_84756_fcmap_linkup
 * Purpose:
 *      Link up handler
 * Parameters:
 *      unit  - StrataSwitch unit #.
 *      port  - StrataSwitch port #.
 * Returns:
 *      SOC_E_NONE
 */
STATIC int 
phy_84756_fcmap_linkup(int unit, soc_port_t port)
{

    /* After link up configure the system side to operate at 10G */
    phy_ctrl_t *pc;
#if defined(INCLUDE_FCMAP)
    phy_ctrl_t  *int_pc = INT_PHY_SW_STATE(unit, port);
#endif
    bfcmap_phy84756_intf_t line_mode;

    pc = EXT_PHY_SW_STATE(unit, port);

#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        if (NULL != int_pc) {
            PHY_AUTO_NEGOTIATE_SET (int_pc->pd, unit, port, 0);
            PHY_SPEED_SET(int_pc->pd, unit, port, 10000);
        }
        /* XFI System Side Speed to 10G */
        SOC_IF_ERROR_RETURN(
            _phy84756_system_xfi_speed_set(pc, 0, 10000));

    } else 
#endif
    {
        SOC_IF_ERROR_RETURN(
            bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

        if (line_mode == BFCMAP_PHY84756_INTF_SGMII) {
            SOC_IF_ERROR_RETURN(
                _bfcmap_phy84756_system_sgmii_sync(pc, 0));
        } 
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_84756_fcmap_interface_get
 * Purpose:
 *      Get interface on a given port.
 * Parameters:
 *      unit  - StrataSwitch unit #.
 *      port  - StrataSwitch port #.
 *      pif   - Interface.
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
phy_84756_fcmap_interface_get(int unit, soc_port_t port, soc_port_if_t *pif)
{
    *pif = PHY_COPPER_MODE(unit, port) ? SOC_PORT_IF_SGMII : SOC_PORT_IF_XFI;
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_84756_fcmap_probe
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
phy_84756_fcmap_probe(int unit, phy_ctrl_t *pc)
{
#ifdef INCLUDE_MACSEC
        /* Use of Standalone MACSEC driver is selected by fcmap_enable=0xff */
        if (0xff != soc_property_port_get(unit, pc->port, spn_FCMAP_ENABLE, 0)) {
            soc_cm_print("u=%d p=%d Using SDK PHY driver for BCM84756/7/9\n",
                unit, pc->port);
            pc->size = sizeof(phy84756_map_t);
            return SOC_E_NONE;
        }
        return SOC_E_NOT_FOUND;
#else
        pc->size = sizeof(phy84756_map_t);
        SOC_DEBUG_PRINT((DK_PHY,
                         "u=%d p=%d Using SDK PHY driver for BCM84756/7/9 (no MACSEC support)\n",
                         unit, pc->port));
        return SOC_E_NONE;
#endif
}

/*
 * Function:     
 *    _phy84756_phy_get_dev_addr
 * Purpose:    
 *    For a given PHY Device address retrive Device port
 * Parameters:
 *    phy_dev_addr    - PHY Device Address
 *    fcmap_dev_addr - MACSEC core Device Address
 *    dev_port        - (OUT)Port number
 * Notes: 
 */
STATIC int
_phy84756_phy_get_dev_addr(phy_ctrl_t *pc, 
                          blmi_dev_addr_t *fcmap_dev_addr, int *dev_port)
{
    if (dev_port) {
#if defined(INCLUDE_MACSEC)
        if (pc->macsec_enable) {
            *dev_port = pc->macsec_dev_port;
        } else
#endif
#if defined(INCLUDE_FCMAP)
        if (pc->fcmap_enable) {
            *dev_port = pc->fcmap_dev_port;
        } else
#endif
        {
#if defined(INCLUDE_FCMAP)
            *dev_port = pc->fcmap_dev_port;
#endif
#if defined(INCLUDE_MACSEC)
            *dev_port = pc->macsec_dev_port;
#endif
        }
    }
    if (fcmap_dev_addr) {
#if defined(INCLUDE_MACSEC)
        if (pc->macsec_enable) {
            *fcmap_dev_addr = pc->macsec_dev_addr;
        } else
#endif
#if defined(INCLUDE_FCMAP)
        if (pc->fcmap_enable) {
            *fcmap_dev_addr = pc->fcmap_dev_addr;
        } else
#endif
        {
#if defined(INCLUDE_FCMAP)
            *fcmap_dev_addr = pc->fcmap_dev_addr;
#endif
#if defined(INCLUDE_MACSEC)
            *fcmap_dev_addr = pc->macsec_dev_addr;
#endif
        }
    }
    return SOC_E_NONE;
}

/*
 * Function:     
 *    _phy84756_get_model_rev
 * Purpose:    
 *    Get OUI, Model and Revision of the PHY
 * Parameters:
 *    phy_dev_addr - PHY Device Address
 *    oui          - (OUT) Organization Unique Identifier
 *    model        - (OUT)Device Model number`
 *    rev          - (OUT)Device Revision number
 * Notes: 
 */
int 
_phy84756_get_model_rev(phy_ctrl_t *pc,
                       int *oui, int *model, int *rev)
{
    buint16_t  id0, id1;

    SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMD_ID0r(pc, &id0));

    SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMD_ID1r(pc, &id1));

    *oui   = BFCMAP_PHY84756_PHY_OUI(id0, id1);
    *model = BFCMAP_PHY84756_PHY_MODEL(id0, id1);
    *rev   = BFCMAP_PHY84756_PHY_REV(id0, id1);

    return SOC_E_NONE;
}



/*
 * Function:
 *     bfcmap_phy84756_firmware_spi_download
 * Purpose:
 *     Download firmware via SPI
 *         - Check if firmware download is complete
 *         - Check if the checksum is good
 * Parameters:
 *    phy_id    - PHY Device Address
 * Returns:
 */
/* STATIC int */
int
bfcmap_phy84756_firmware_spi_download(phy_ctrl_t *pc)
{
    buint16_t data = 0;
    int i;

    SOC_DEBUG_PRINT((DK_PHY, "PHY84756: SPI-ROM firmware download start: u=%d p=%d\n",
                     pc->unit, pc->port));

    /* 0xc848[15]=1, SPI-ROM downloading to RAM, 0xc848[14]=1, serial boot */
    /* 0xc848[13]=0, SPI-ROM downloading not done, 0xc848[2]=0, spi port enable */
    SOC_IF_ERROR_RETURN(
           BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc848, 
               (1 << 15)|(1 << 14), 
               ((1 << 15)|(1 << 14)|(1 << 13)|(1 << 2))));

    /* apply software reset to download code from SPI-ROM */
    /* Reset the PHY **/
    SOC_IF_ERROR_RETURN(
           BFCMAP_MOD_PHY84756_LN_DEV1_PMD_CTRLr(pc, 
               BFCMAP_PHY84756_PMD_CTRL_RESET, BFCMAP_PHY84756_PMD_CTRL_RESET));

    for (i = 0; i < 5; i++) {
        sal_usleep(100000);
        SOC_IF_ERROR_RETURN(
               BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc848, &data));
        if (data & 0x2000) {  /* Check for download complete */
            /* Need to check if checksum is correct */
            SOC_IF_ERROR_RETURN(
                   BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xca1c, &data));
            if (data == 0x600D) {
                break;
            }
        }
    }
    if (i >= 5) { /* Bad CHECKSUM */
        soc_cm_print("SPI-Download Firmware download failure:"
                       "Incorrect Checksum %x\n", data);
        return SOC_E_FAIL;
    }

    SOC_IF_ERROR_RETURN(
           BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xca1a, &data));

    soc_cm_print("BCM84756: SPI-ROM download complete: version: 0x%x\n", data);

    return SOC_E_NONE;
}


/*
 * Function:     
 *    bfcmap_phy84756_phy_mac_driver_attach
 * Purpose:    
 *    Add Mapping of PHY device address, Device port
 * Parameters:
 *    phy_dev_addr    - PHY Device Address
 *    dev_port        - Port number
 * Notes: 
 */
int 
bfcmap_phy84756_phy_mac_driver_attach(phy_ctrl_t *pc,
                                 blmi_dev_addr_t fcmap_dev_addr,
                                 int dev_port, blmi_dev_io_f mmi_cbf)
{
    phy84756_map_t *phy84756_map;
    uint32      mac_config_flag = 0;

    /* Delete the mapping if already present */
    (void)bfcmap_phy84756_phy_mac_driver_detach(pc);

    phy84756_map = PHY84756_PHY_INFO(pc);

    if (phy84756_map == NULL) {
        return SOC_E_RESOURCE;
    }

    /* Alloc unimac driver and attach */
    phy84756_map->unimac = 
            phy_mac_driver_attach(fcmap_dev_addr, 
                                     PHY_MAC_CORE_UNIMAC, mmi_cbf);
    if (!phy84756_map->unimac) {
        bfcmap_phy84756_phy_mac_driver_detach(pc);
        return SOC_E_INTERNAL;
    }

    phy84756_map->xmac = 
            phy_mac_driver_attach(fcmap_dev_addr, 
                                     PHY_MAC_CORE_XMAC, mmi_cbf);
    if (!phy84756_map->xmac) {
        bfcmap_phy84756_phy_mac_driver_detach(pc);
        return SOC_E_INTERNAL;
    }


    if (CHIP_IS_C0(pc)) {
        mac_config_flag |= PHY_MAC_CTRL_FLAG_XMAC_V2;
    }

    if(mac_config_flag) { 
        phy_mac_driver_config(phy84756_map->xmac,
                             PHY_MAC_CORE_XMAC,
                             mac_config_flag);
    }

    return SOC_E_NONE;
}



/*
 * Function:     
 *    bfcmap_phy84756_phy_mac_driver_detach
 * Purpose:    
 *    Delete Mapping of PHY device address and Device port
 * Parameters:
 *    phy_dev_addr    - PHY Device Address
 * Notes: 
 */
int 
bfcmap_phy84756_phy_mac_driver_detach(phy_ctrl_t     *pc)
{
    phy84756_map_t *this;
    int rv = SOC_E_NONE;

    this = PHY84756_PHY_INFO(pc);
    if (this->unimac) {
        phy_mac_driver_detach(this->unimac);
        this->unimac = NULL;
    }
    if (this->xmac) {
        phy_mac_driver_detach(this->xmac);
        this->xmac = NULL;
    }
    return rv;
}


/*
 * Function:     
 *    bfcmap_phy84756_line_intf_get
 * Purpose:    
 *    Determine the Line side Interface
 * Parameters:
 *    phy_id    - PHY's device address
 *    dev_port  - Channel number 
 *    mode      - (OUT) Line Port mode status
 * Returns:    
 */
int 
bfcmap_phy84756_line_intf_get(phy_ctrl_t *pc, int dev_port, 
                              bfcmap_phy84756_intf_t *mode)
{

    phy84756_map_t      *phy_info;

    if (mode == NULL) {
        return SOC_E_PARAM;
    }

    phy_info = PHY84756_PHY_INFO(pc);

    *mode = phy_info->line_mode;
    return SOC_E_NONE;
}



/*
 * Function:     
 *    bfcmap_phy84756_line_mode_set
 * Purpose:    
 *    Set the Line side mode
 * Parameters:
 *    phy_id    - PHY's device address
 *    dev_port  - Channel number 
 *    mode      - Desired mode (SGMII/1000x/SFI)
 * Returns:    
 */
int
bfcmap_phy84756_line_mode_set(phy_ctrl_t *pc, int dev_port, 
                              bfcmap_phy84756_intf_t mode)
{

    int rv = SOC_E_NONE;
    phy84756_map_t      *phy_info;

    phy_info = PHY84756_PHY_INFO(pc);

    /* Disable Autodetect */
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_LN_DEV7_AN_BASE1000X_CTRL1r(pc, 0,
                       BFCMAP_PHY84756_AN_BASE1000X_CTRL1r_AUTODETECT_EN));
    switch (mode) {

    case BFCMAP_PHY84756_INTF_SGMII :

        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_REG_WR(pc, 0x00, 0x0007, 0x8300, 0x0184));
        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_REG_WR(pc, 0x00, 0x0007, 0x8308, 0x0000));
        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_REG_MOD(pc, 0x00, 0x0007, 0x8309, 0x6000, 0x6020));
        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_REG_WR(pc, 0x00, 0x0007, 0x835c, 0x0801));
        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_REG_WR(pc, 0x00, 0x0007, 0xffe0, 0x1140));

        break;
        
    case BFCMAP_PHY84756_INTF_1000X :
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV7_AN_BASE1000X_CTRL1r(pc, 1,
                     BFCMAP_PHY84756_AN_BASE1000X_CTRL1r_FIBER_MODE));
        break;

    case BFCMAP_PHY84756_INTF_SFI :
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV7_AN_BASE1000X_CTRL1r(pc, 1,
                     BFCMAP_PHY84756_AN_BASE1000X_CTRL1r_FIBER_MODE));
        /* Set PMA type */
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV1_PMD_CTRL2r(pc, 0x8, 0x003f));
        break;

    case BFCMAP_PHY84756_INTF_XFI :
    default :
        rv = SOC_E_PARAM;
        break;
        
    }

    if (SOC_SUCCESS(rv)) {
        phy_info->line_mode = mode;
    }
    return rv;
}


/*
 * Function:     
 *    bfcmap_phy84756_sys_intf_get
 * Purpose:    
 *    Determine the System side Interface
 * Parameters:
 *    phy_id    - PHY's device address
 *    dev_port  - Channel number 
 *    mode      - (OUT) System side mode status
 * Returns:    
 */
int 
bfcmap_phy84756_sys_intf_get(phy_ctrl_t *pc, int dev_port, 
                             bfcmap_phy84756_intf_t *mode)
{

    phy84756_map_t      *phy_info;

    if (mode == NULL) {
        return SOC_E_PARAM;
    }

    phy_info = PHY84756_PHY_INFO(pc);

    /* System will always follow line side interface */
    *mode = phy_info->line_mode;

    return SOC_E_NONE;
}



/*
 * Function:     
 *    bfcmap_phy84756_sys_mode_set
 * Purpose:    
 *    Set the system side mode
 * Parameters:
 *    phy_id    - PHY's device address
 *    dev_port  - Channel number 
 *    mode      - Desired mode (SGMII/1000x/XFI)
 * Returns:    
 */
int
bfcmap_phy84756_sys_mode_set(phy_ctrl_t *pc, int dev_port, 
                              bfcmap_phy84756_intf_t mode)
{
    int rv = SOC_E_NONE;

    /* Disable Autodetect */
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_XFI_DEV7_AN_BASE1000X_CTRL1r(pc, 0,
                       BFCMAP_PHY84756_AN_BASE1000X_CTRL1r_AUTODETECT_EN));
    switch (mode) {
    case BFCMAP_PHY84756_INTF_SGMII :
        SOC_IF_ERROR_RETURN(
            _bfcmap_phy84756_system_sgmii_init(pc, dev_port));
        break;
        
    case BFCMAP_PHY84756_INTF_1000X :
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_XFI_DEV7_AN_BASE1000X_CTRL1r(pc, 1,
                     BFCMAP_PHY84756_AN_BASE1000X_CTRL1r_FIBER_MODE));
        break;
    case BFCMAP_PHY84756_INTF_XFI :
        break;

    case BFCMAP_PHY84756_INTF_XAUI :
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_XGXS_DEV7_AN_BASE1000X_CTRL1r(pc, 1,
                     BFCMAP_PHY84756_AN_BASE1000X_CTRL1r_FIBER_MODE));
        /* Set PMA type */
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_SYS_DEV1_PMD_CTRL2r(pc, 0x8,
                     0x000f));
        break;
    case BFCMAP_PHY84756_INTF_SFI :
    default :
        rv = SOC_E_PARAM;
        break;
    }

    return rv;
}


/*
 * Function:     
 *    bfcmap_phy84756_no_reset_setup
 * Purpose:    
 *    Initialize the PHY without reset.
 * Parameters:
 *    phy_dev_addr        - PHY Device Address
 *    line_mode           - Line side Mode (SGMII/1000X/SFI).
 *    fcmap_enable        - Fibre Channel enabled or disabled.
 * Notes: 
 */
int
bfcmap_phy84756_no_reset_setup(phy_ctrl_t *pc, int line_mode,
                               int phy_ext_boot)
{
    int dev_port;
    int port;
    phy84756_map_t      *phy_info;
#ifdef INCLUDE_FCMAP
    buint16_t val = 0xDEAD;
#endif
    buint16_t data;

    if (_phy84756_phy_get_dev_addr(pc, NULL,
                                  &dev_port) != SOC_E_NONE) {
        return SOC_E_CONFIG;
    }

    phy_info = PHY84756_PHY_INFO(pc);

    if ((line_mode != BFCMAP_PHY84756_INTF_SGMII) &&
        (line_mode != BFCMAP_PHY84756_INTF_1000X) && 
        (line_mode != BFCMAP_PHY84756_INTF_SFI)) {
        return SOC_E_CONFIG;
    }

    /* Enable Power - PMA/PMD, PCS, AN */
    SOC_IF_ERROR_RETURN(
         BFCMAP_MOD_PHY84756_LN_DEV1_PMD_CTRLr(pc, 0,
                                        BFCMAP_PHY84756_PMD_CTRL_PD));
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_LN_DEV3_PCS_CTRLr(pc, 0,
                                       BFCMAP_PHY84756_PCS_PCS_CTRL_PD));
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_LN_DEV7_AN_MII_CTRLr(pc, 0,
                                       BFCMAP_PHY84756_AN_MII_CTRL_PD));
    if (phy_ext_boot) {
        if (!SKIP_FW_DOWNLOAD(pc)) {
            /* Download firmware via SPI */
            SOC_IF_ERROR_RETURN(bfcmap_phy84756_firmware_spi_download(pc));
        }
    }  else if (!(pc->flags & PHYCTRL_MDIO_BCST) && (!SKIP_FW_DOWNLOAD(pc))) {
        /* MDIO based firmware download to RAM */
        SOC_IF_ERROR_RETURN(
            bfcmap_phy84756_mdio_firmware_download(pc,
                                FIRMWARE(pc),  FIRMWARE_LEN(pc)));
    } 

    if ((!soc_property_port_get(pc->unit, pc->port, spn_FCMAP_ENABLE, 0)) &&
        (!soc_property_port_get(pc->unit, pc->port, spn_MACSEC_ENABLE, 0))) {
        /* Set Line mode */
        SOC_IF_ERROR_RETURN(
            bfcmap_phy84756_line_mode_set(pc, 0, line_mode));

        /* Disable MACSEC */
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV1_MACSEC_BYPASS_CTLRr(pc, &data));

        data &= ~(BFCMAP_PHY84756_LN_DEV1_MACSEC_CTRL_BYPASS_MODE |
                  BFCMAP_PHY84756_LN_DEV1_MACSEC_CTRL_ENABLE_PWRDN_MACSEC |
                  BFCMAP_PHY84756_LN_DEV1_MACSEC_CTRL_UDSW_PWRDW_MACSEC |
                  BFCMAP_PHY84756_LN_DEV1_MACSEC_CTRL_UDSW_RESET_MACSEC);

        data |= BFCMAP_PHY84756_LN_DEV1_MACSEC_CTRL_BYPASS_MODE;
        /* Power down MACSEC to reduce power consumption */
        data |= BFCMAP_PHY84756_LN_DEV1_MACSEC_CTRL_ENABLE_PWRDN_MACSEC;
        data |= BFCMAP_PHY84756_LN_DEV1_MACSEC_CTRL_UDSW_PWRDW_MACSEC;

        SOC_IF_ERROR_RETURN(
            BFCMAP_WR_PHY84756_LN_DEV1_MACSEC_BYPASS_CTLRr(pc, data));

        /* System side mode */
        if (line_mode == BFCMAP_PHY84756_INTF_SFI) {
            phy_info->line_mode = BFCMAP_PHY84756_INTF_SFI;
            SOC_IF_ERROR_RETURN(
                bfcmap_phy84756_sys_mode_set(pc, 0, 
                                     BFCMAP_PHY84756_INTF_XFI));
            /* Force Speed to 10G */
            SOC_IF_ERROR_RETURN(
                _phy84756_system_xfi_speed_set(pc, 0, 10000));
            SOC_IF_ERROR_RETURN(
                bfcmap_phy84756_speed_set(pc, 10000));
        } else {
            if (line_mode == BFCMAP_PHY84756_INTF_1000X) {
                phy_info->line_mode = BFCMAP_PHY84756_INTF_1000X;
                SOC_IF_ERROR_RETURN(
                    bfcmap_phy84756_sys_mode_set(pc, 0, 
                                         BFCMAP_PHY84756_INTF_1000X));
                /* Force Speed to 1G */
                SOC_IF_ERROR_RETURN(
                    bfcmap_phy84756_speed_set(pc, 1000));
            } else { /* SGMII MODE */
                phy_info->line_mode = BFCMAP_PHY84756_INTF_SGMII;
                SOC_IF_ERROR_RETURN(
                    bfcmap_phy84756_sys_mode_set(pc, 0, 
                                         BFCMAP_PHY84756_INTF_SGMII));
            }
        }
    } else {
        if (soc_property_port_get(pc->unit, pc->port, spn_FCMAP_ENABLE, 0)) { 
#ifdef INCLUDE_FCMAP
            /* FCMAP enable */
            sal_usleep(10000);
            /* Enable FC */
            SOC_IF_ERROR_RETURN(
                BFCMAP_RD_PHY84756_LN_DEV1_CHIP_MODEr(pc, &data));
            data &= ~0x3;
            data |= 0x2;
            SOC_IF_ERROR_RETURN(
                BFCMAP_WR_PHY84756_LN_DEV1_CHIP_MODEr(pc, data));

            SOC_IF_ERROR_RETURN(
                bfcmap_phy84756_reset(pc));

            sal_usleep(100000); /* need 100ms delay */

            /* Enable MACSEC */
            SOC_IF_ERROR_RETURN(
            BFCMAP_WR_PHY84756_SGMII_DEV1_MACSEC_BYPASS_CTRLr(pc, 0x0000));

            SOC_IF_ERROR_RETURN(
                BFCMAP_PHY84756_REG_WR(pc, 0, 1, 0xcd58, 0x300));
            sal_usleep(30);
            SOC_IF_ERROR_RETURN(
                BFCMAP_PHY84756_REG_WR(pc, 0, 1, 0xcd53, 0));

            SOC_IF_ERROR_RETURN(
                BFCMAP_PHY84756_REG_RD(pc, 0, 1, 0xcd53, &val));

            /* Set Line mode */
            SOC_IF_ERROR_RETURN(
                bfcmap_phy84756_line_mode_set(pc, 0,
                                            BFCMAP_PHY84756_INTF_SFI));

            /* System side mode */
            SOC_IF_ERROR_RETURN(
               bfcmap_phy84756_sys_mode_set(pc, 0, 
                                            BFCMAP_PHY84756_INTF_XFI));
            /* Force Speed to 10G */
            SOC_IF_ERROR_RETURN(
                _phy84756_system_xfi_speed_set(pc, 0, 10000));


            /* Initialize the switch side XMAC */
            port = BFCMAP_PHY84756_SWITCH_MAC_PORT(dev_port);
            SOC_IF_ERROR_RETURN(
                phy_xmac_mac_fcmap_init(phy_info->xmac, port));
#endif

        } else {
            if (soc_property_port_get(pc->unit, pc->port, spn_MACSEC_ENABLE, 0)) { 
#ifdef INCLUDE_MACSEC
                /* Set Line mode */
                SOC_IF_ERROR_RETURN(
                    bfcmap_phy84756_line_mode_set(pc, 0, line_mode));

                /* System side mode */
                if (line_mode == BFCMAP_PHY84756_INTF_SFI) {
                    phy_info->line_mode = BFCMAP_PHY84756_INTF_SFI;
                    SOC_IF_ERROR_RETURN(
                        bfcmap_phy84756_sys_mode_set(pc, 0, 
                                             BFCMAP_PHY84756_INTF_XFI));
                    /* Force Speed to 10G */
                    SOC_IF_ERROR_RETURN(
                        _phy84756_system_xfi_speed_set(pc, 0, 10000));
                    SOC_IF_ERROR_RETURN(
                        bfcmap_phy84756_speed_set(pc, 10000));
                } else {
                    if (line_mode == BFCMAP_PHY84756_INTF_1000X) {
                        phy_info->line_mode = BFCMAP_PHY84756_INTF_1000X;
                        SOC_IF_ERROR_RETURN(
                            bfcmap_phy84756_sys_mode_set(pc, 0, 
                                                 BFCMAP_PHY84756_INTF_1000X));
                        /* Force Speed to 1G */
                        SOC_IF_ERROR_RETURN(
                            bfcmap_phy84756_speed_set(pc, 1000));
                    } else { /* SGMII MODE */
                        phy_info->line_mode = BFCMAP_PHY84756_INTF_SGMII;
                        SOC_IF_ERROR_RETURN(
                            bfcmap_phy84756_sys_mode_set(pc, 0, 
                                                 BFCMAP_PHY84756_INTF_SGMII));
                    }
                }

                /* Enable MACSEC */
                SOC_IF_ERROR_RETURN(
                BFCMAP_WR_PHY84756_SGMII_DEV1_MACSEC_BYPASS_CTRLr(pc, 0x0000));

                /* 
                 * Set defaults to Line and Swithc side MAC
                 * MTU = 0x3fff
                 * IPG = 12Bytes
                 * Auto Config Mode Enabled
                 */
                /* Initialize the Line side MAC */
                port = BFCMAP_PHY84756_SWITCH_MAC_PORT(dev_port);
                SOC_IF_ERROR_RETURN(
                    BMACSEC_MAC_INIT(phy_info->xmac, port));

                /* Initialize the Line side MAC */
                port = BFCMAP_PHY84756_LINE_MAC_PORT(dev_port);
                SOC_IF_ERROR_RETURN(
                    BMACSEC_MAC_INIT(phy_info->xmac, port));

                /* Initialize the Line side MAC */
                port = BFCMAP_PHY84756_LINE_MAC_PORT(dev_port);
                SOC_IF_ERROR_RETURN(
                    BMACSEC_MAC_INIT(phy_info->unimac, port));

                /* Initialize the Line side MAC */
                port = BFCMAP_PHY84756_SWITCH_MAC_PORT(dev_port);
                SOC_IF_ERROR_RETURN(
                    BMACSEC_MAC_INIT(phy_info->unimac, port));
#endif
            }
        }
    }

    return SOC_E_NONE;
}

/*
 * Function:     
 *    bfcmap_phy84756_reset 
 * Purpose:    
 *    Reset the PHY.
 * Parameters:
 *    phy_id - PHY Device Address of the PHY to reset
 */
int 
bfcmap_phy84756_reset(phy_ctrl_t *pc)
{
    buint16_t          ctrl, tmp;


    /* Reset the PHY **/
    SOC_IF_ERROR_RETURN(
        BFCMAP_RD_PHY84756_LN_DEV1_PMD_CTRLr(pc, &ctrl));

    SOC_IF_ERROR_RETURN(
        BFCMAP_WR_PHY84756_LN_DEV1_PMD_CTRLr(pc, 
                               (ctrl | BFCMAP_PHY84756_PMD_CTRL_RESET)));
    /* Needs minimum of 5us to complete the reset */
    sal_usleep(30);

    SOC_IF_ERROR_RETURN(
        BFCMAP_RD_PHY84756_LN_DEV1_PMD_CTRLr(pc, &tmp));

    if ((tmp & BFCMAP_PHY84756_PMD_CTRL_RESET) != 0) {
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMD_CTRLr(pc, ctrl));
        return SOC_E_FAIL;
    }

    return SOC_E_NONE;
}


/*
 * Function:     
 *    bfcmap_phy84756_speed_set
 * Purpose:    
 *    To set the PHY's speed  
 * Parameters:
 *    phy_id    - PHY's device address
 *    speed     - Speed to set
 *               10   = 10Mbps
 *               100  = 100Mbps
 *               1000 = 1000Mbps
 * Returns:    
 */
int
bfcmap_phy84756_speed_set(phy_ctrl_t *pc, int speed)
{
    int value = 0;
    bfcmap_phy84756_intf_t line_mode;

    if ((speed != 10) && (speed != 100) && 
         (speed != 1000) && (speed != 10000)) {
        return SOC_E_NONE;
    }

    SOC_IF_ERROR_RETURN(
        bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

    switch (line_mode) {
    case BFCMAP_PHY84756_INTF_SFI:
    case BFCMAP_PHY84756_INTF_1000X:
        /* Disable Clause 37 Autoneg */
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV7_AN_MII_CTRLr(pc, 0x0000,
                                   0x3040));
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV7_AN_BASE1000X_CTRL1r(pc, 1,
                                   1));
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV7_AN_MSIC2r(pc, 0x2020,
                                   0x2020));
        /* Disable Clause 37 Autoneg */
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_XFI_DEV7_AN_MII_CTRLr(pc, 0x0000,
                                   0x3040));
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_XFI_DEV7_AN_BASE1000X_CTRL1r(pc, 1,
                                   1));
        if (speed == 10000) {

            /* Set Line mode */
            SOC_IF_ERROR_RETURN(
                bfcmap_phy84756_line_mode_set(pc, 0,
                BFCMAP_PHY84756_INTF_SFI));

            /* Set System side to match Line side */
            SOC_IF_ERROR_RETURN(
                bfcmap_phy84756_sys_mode_set(pc, 0, 
                                         BFCMAP_PHY84756_INTF_XFI));
            /* Force Speed in PMD */
            SOC_IF_ERROR_RETURN(
                BFCMAP_MOD_PHY84756_LN_DEV1_PMD_CTRLr(pc, 
                                     BFCMAP_PHY84756_PMD_CTRL_SS_10000,
                                     BFCMAP_PHY84756_PMD_CTRL_10GSS_MASK));
            /* Select 10G-LRM PMA */
            SOC_IF_ERROR_RETURN(
                BFCMAP_MOD_PHY84756_LN_DEV1_PMD_CTRL2r(pc,
                             BFCMAP_PHY84756_PMD_CTRL2_PMA_10GLRMPMD_TYPE,
                             BFCMAP_PHY84756_PMD_CTRL2_PMA_SELECT_MASK));
            /* Select 10G-LRM PMA */
            SOC_IF_ERROR_RETURN(
                BFCMAP_MOD_PHY84756_SYS_DEV1_PMD_CTRL2r(pc,
                             BFCMAP_PHY84756_PMD_CTRL2_PMA_10GLRMPMD_TYPE,
                             BFCMAP_PHY84756_PMD_CTRL2_PMA_SELECT_MASK));

            /* Force Speed in PMD */
            SOC_IF_ERROR_RETURN(
                BFCMAP_MOD_PHY84756_SYS_DEV1_PMD_CTRLr(pc, 
                                         BFCMAP_PHY84756_PMD_CTRL_SS_10000,
                                         BFCMAP_PHY84756_PMD_CTRL_10GSS_MASK));
        } else {
            if (speed == 1000) {
                /* Set Line mode */
                SOC_IF_ERROR_RETURN(
                    bfcmap_phy84756_line_mode_set(pc, 0,
                                                   BFCMAP_PHY84756_INTF_1000X));
                /* Set System side to match Line side */
                SOC_IF_ERROR_RETURN(
                    bfcmap_phy84756_sys_mode_set(pc, 0, 
                                             BFCMAP_PHY84756_INTF_1000X));
                /* Force Speed in PMD */
                SOC_IF_ERROR_RETURN(
                    BFCMAP_MOD_PHY84756_LN_DEV1_PMD_CTRLr(pc, 
                                         BFCMAP_PHY84756_PMD_CTRL_SS_1000,
                                         BFCMAP_PHY84756_PMD_CTRL_10GSS_MASK));

                /* Force Speed in PMD */
                SOC_IF_ERROR_RETURN(
                    BFCMAP_MOD_PHY84756_SYS_DEV1_PMD_CTRLr(pc, 
                                             BFCMAP_PHY84756_PMD_CTRL_SS_1000,
                                             BFCMAP_PHY84756_PMD_CTRL_10GSS_MASK));
            } else {
                return SOC_E_CONFIG;
            }
        }
    break;

    case BFCMAP_PHY84756_INTF_SGMII:
        if (speed > 1000) {
            return SOC_E_CONFIG;
        }
        /* Disable Clause 37 Autoneg */
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV7_AN_MII_CTRLr(pc, 0x0000,
                                   0x3040));
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV7_AN_BASE1000X_CTRL1r(pc, 0,
                                   1));
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV7_AN_MSIC2r(pc, 0x2020,
                                   0x2020));
        if (speed == 1000) {
            value = BFCMAP_PHY84756_PMD_CTRL_SS_1000;
        }
        if (speed == 100) {
            value = BFCMAP_PHY84756_PMD_CTRL_SS_100;
        }
        if (speed == 100) {
            value = BFCMAP_PHY84756_PMD_CTRL_SS_10;
        }
        /* Force Speed in PMD */
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_LN_DEV1_PMD_CTRLr(pc, 
                                 value,
                                 BFCMAP_PHY84756_PMD_CTRL_10GSS_MASK));

        /* Set System side to match Line side */
        SOC_IF_ERROR_RETURN(
            bfcmap_phy84756_sys_mode_set(pc, 0, 
                                     BFCMAP_PHY84756_INTF_SGMII));
        /* Disable Clause 37 Autoneg */
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_XFI_DEV7_AN_MII_CTRLr(pc, 0x0000,
                                   0x3040));
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_XFI_DEV7_AN_BASE1000X_CTRL1r(pc, 0,
                                   1));
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_XFI_DEV7_AN_MISC2r(pc, 0x2020,
                                   0x2020));
        /* Force Speed in PMD */
        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_SYS_DEV1_PMD_CTRLr(pc, 
                                     value,
                                     BFCMAP_PHY84756_PMD_CTRL_10GSS_MASK));
    break;
    default:
        return SOC_E_CONFIG;
    }

    return SOC_E_NONE;
}


/*
 * Function:
 *      bfcmap_phy84756_link_get
 * Purpose:
 *      Determine the current link up/down status for a 84756 device.
 * Parameters:
 *    phy_id    - PHY Device Address
 *    link      - (OUT) 
 *                1 = Indicates Link is established
 *                0 = No Link.
 * Returns:
 */
int
bfcmap_phy84756_link_get(phy_ctrl_t *pc, int *link)
{

    buint16_t data = 0, pma_mii_stat = 0, pcs_mii_stat = 0;
#if defined(INCLUDE_FCMAP)
    buint16_t fval = 0;
#endif
    int an, an_done;
    bfcmap_phy84756_intf_t line_mode;

    /* Read AN MII STAT once per link get period */
    SOC_IF_ERROR_RETURN(
        BFCMAP_RD_PHY84756_LN_DEV7_AN_MII_STATr(pc, &pc->mii_stat));

#if defined(INCLUDE_FCMAP)
    if (pc->fcmap_enable) {
        BFCMAP_PHY84756_REG_RD(pc, 0, 0x1e, 0x50, &fval);
        if (fval == 0x1a) {
            *link = 1;
        } else {
            *link = 0;
        }
        return SOC_E_NONE;
    }
#endif

    *link = 0;
    SOC_IF_ERROR_RETURN(
        phy_84756_fcmap_an_get(pc->unit, pc->port, &an, &an_done)); 

    if (an && (an_done == 0)) {
        /* Auto neg in progess, return no link */
        *link = 0;
        return SOC_E_NONE;
    }


     SOC_IF_ERROR_RETURN(
         bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

     switch (line_mode) {
        case BFCMAP_PHY84756_INTF_SFI:
            SOC_IF_ERROR_RETURN(
                BFCMAP_RD_PHY84756_LN_DEV1_PMD_CTRL2r(pc, &data));
 
            SOC_IF_ERROR_RETURN(
                BFCMAP_RD_PHY84756_LN_DEV1_PMD_CTRL2r(pc, &data));
 
            SOC_IF_ERROR_RETURN(
                BFCMAP_RD_PHY84756_LN_DEV1_PMD_STATr(pc, &pma_mii_stat));
 
            SOC_IF_ERROR_RETURN(
                BFCMAP_RD_PHY84756_LN_DEV3_PCS_STATr(pc, &pcs_mii_stat));
 
            *link = pma_mii_stat & pcs_mii_stat & BFCMAP_PHY84756_PMD_STAT_RX_LINK_STATUS;
 
            break;
 
        case BFCMAP_PHY84756_INTF_1000X:
            if (!(pc->mii_stat & BFCMAP_PHY84756_AN_MII_STAT_LINK_STATUS)) {
                /* In transition, no link */
                *link = 0;
                break;
            }
            /* fallthrough */
        case BFCMAP_PHY84756_INTF_SGMII:
            SOC_IF_ERROR_RETURN(
                BFCMAP_RD_PHY84756_LN_DEV1_SPEED_LINK_DETECT_STATr(pc, &data));
            if ((data & 0x8080) == 0x8080) {
                /* System side and line side rx signal detect OK */
                if (data & 0x1700) { /* system side link up */
                    if (data & 0x0017) { /* line side line up */
                        *link = 1;
                    }
                }
            }
            break;
        default:
            break;
    }
    return SOC_E_NONE;
}


/*
 * Function:
 *     bfcmap_phy84756_ability_advert_set 
 * Purpose:
 *      Set the current advertisement for auto-negotiation.
 * Parameters:
 *    phy_id    - PHY Device Address
 *    ability   - Ability indicating supported options/speeds.
 *                For Speed & Duplex:
 *                BFCMAP_PM_10MB_HD    = 10Mb, Half Duplex
 *                BFCMAP_PM_10MB_FD    = 10Mb, Full Duplex
 *                BFCMAP_PM_100MB_HD   = 100Mb, Half Duplex
 *                BFCMAP_PM_100MB_FD   = 100Mb, Full Duplex
 *                BFCMAP_PM_1000MB_HD  = 1000Mb, Half Duplex
 *                BFCMAP_PM_1000MB_FD  = 1000Mb, Full Duplex
 *                BFCMAP_PM_PAUSE_TX   = TX Pause
 *                BFCMAP_PM_PAUSE_RX   = RX Pause
 *                BFCMAP_PM_PAUSE_ASYMM = Asymmetric Pause
 * Returns:
 */

int
bfcmap_phy84756_ability_advert_set(phy_ctrl_t *pc,
                               bfcmap_phy84756_port_ability_t ability)
{
    buint16_t data = 0;
    bfcmap_phy84756_intf_t line_mode;

    SOC_IF_ERROR_RETURN(
        bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

    if (line_mode == BFCMAP_PHY84756_INTF_1000X) {
        /* Always advertise 1000X full duplex */
        data = BFCMAP_PHY84756_DEV7_1000X_ANA_C37_FD;  
        if ((ability & BFCMAP_PHY84756_PA_PAUSE) == 
                                             BFCMAP_PHY84756_PA_PAUSE) {
            data |= BFCMAP_PHY84756_DEV7_1000X_ANA_C37_PAUSE;
        } else {
            if (ability & BFCMAP_PHY84756_PA_PAUSE_TX) {
                data |= BFCMAP_PHY84756_DEV7_1000X_ANA_C37_ASYM_PAUSE;
            } else {
                if (ability & BFCMAP_PHY84756_PA_PAUSE_RX) {
                    data |= BFCMAP_PHY84756_DEV7_1000X_ANA_C37_ASYM_PAUSE;
                    data |= BFCMAP_PHY84756_DEV7_1000X_ANA_C37_PAUSE;
                }
            }
        }
        SOC_IF_ERROR_RETURN(
            BFCMAP_WR_PHY84756_LN_DEV7_AN_ANAr(pc, data));

    }
    if (line_mode == BFCMAP_PHY84756_INTF_SGMII) {
        
        
    } else {
        /* No Autoneg in SFI mode */
        /* return SOC_E_UNAVAIL; */
    }
    return SOC_E_NONE;
}


/*
 * Function:
 *      bfcmap_phy84756_ability_advert_get
 * Purpose:
 *      Get the current advertisement for auto-negotiation.
 * Parameters:
 *    phy_id    - PHY Device Address
 *    ability   - (OUT) Port ability mask indicating supported options/speeds.
 *                For Speed & Duplex:
 *                BFCMAP_PM_10MB_HD    = 10Mb, Half Duplex
 *                BFCMAP_PM_10MB_FD    = 10Mb, Full Duplex
 *                BFCMAP_PM_100MB_HD   = 100Mb, Half Duplex
 *                BFCMAP_PM_100MB_FD   = 100Mb, Full Duplex
 *                BFCMAP_PM_1000MB_HD  = 1000Mb, Half Duplex
 *                BFCMAP_PM_1000MB_FD  = 1000Mb, Full Duplex
 *                BFCMAP_PM_PAUSE_TX   = TX Pause
 *                BFCMAP_PM_PAUSE_RX   = RX Pause
 *                BFCMAP_PM_PAUSE_ASYMM = Asymmetric Pause
 * Returns:
 */

int
bfcmap_phy84756_ability_advert_get(phy_ctrl_t *pc,
                               bfcmap_phy84756_port_ability_t *ability)
{
    buint16_t data = 0;
    bfcmap_phy84756_intf_t line_mode;

    if (ability == NULL) {
        return SOC_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(
        bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

    if (line_mode == BFCMAP_PHY84756_INTF_1000X) {
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV7_AN_ANAr(pc, &data));

        *ability |= (data & BFCMAP_PHY84756_DEV7_1000X_ANA_C37_FD) ? 
                  BFCMAP_PHY84756_PA_1000MB_FD : 0;
        *ability |= (data & BFCMAP_PHY84756_DEV7_1000X_ANA_C37_HD) ?
                  BFCMAP_PHY84756_PA_1000MB_HD : 0;
        switch (data & (BFCMAP_PHY84756_DEV7_1000X_ANA_C37_PAUSE | 
                        BFCMAP_PHY84756_DEV7_1000X_ANA_C37_ASYM_PAUSE)) {
        case BFCMAP_PHY84756_DEV7_1000X_ANA_C37_PAUSE:
            *ability |= BFCMAP_PHY84756_PA_PAUSE;
            break;
        case BFCMAP_PHY84756_DEV7_1000X_ANA_C37_ASYM_PAUSE:
            *ability |= BFCMAP_PHY84756_PA_PAUSE_TX;
            break;
        case (BFCMAP_PHY84756_DEV7_1000X_ANA_C37_PAUSE | 
              BFCMAP_PHY84756_DEV7_1000X_ANA_C37_ASYM_PAUSE):
            *ability |= BFCMAP_PHY84756_PA_PAUSE_RX;
            break;
        }
    } else {
        if (line_mode == BFCMAP_PHY84756_INTF_SGMII) {
            *ability = BFCMAP_PHY84756_PA_SPEED_ALL |
                       BFCMAP_PHY84756_PA_AN        |
                       BFCMAP_PHY84756_PA_PAUSE     |
                       BFCMAP_PHY84756_PA_PAUSE_ASYMM;
        
        } else {
            /* No Autoneg in SFI mode */
            *ability = BFCMAP_PHY84756_PA_10000MB_FD |
                       BFCMAP_PHY84756_PA_PAUSE;
        }
    }
    return SOC_E_NONE;
}


/*
 * Function:
 *      bfcmap_phy84756_remote_ability_advert_get
 * Purpose:
 *      Get partners current advertisement for auto-negotiation.
 * Parameters:
 *    phy_id         - PHY Device Address
 *    remote_ability - (OUT) Port ability mask indicating supported options/speeds.
 *                For Speed & Duplex:
 *                BFCMAP_PM_10MB_HD    = 10Mb, Half Duplex
 *                BFCMAP_PM_10MB_FD    = 10Mb, Full Duplex
 *                BFCMAP_PM_100MB_HD   = 100Mb, Half Duplex
 *                BFCMAP_PM_100MB_FD   = 100Mb, Full Duplex
 *                BFCMAP_PM_1000MB_HD  = 1000Mb, Half Duplex
 *                BFCMAP_PM_1000MB_FD  = 1000Mb, Full Duplex
 *                BFCMAP_PM_PAUSE_TX   = TX Pause
 *                BFCMAP_PM_PAUSE_RX   = RX Pause
 *                BFCMAP_PM_PAUSE_ASYMM = Asymmetric Pause
 * Returns:
 */

int
bfcmap_phy84756_remote_ability_advert_get(phy_ctrl_t *pc,
                                bfcmap_phy84756_port_ability_t *remote_ability)
{
    buint16_t data = 0;
    bfcmap_phy84756_intf_t line_mode;

    if (remote_ability == NULL) {
        return SOC_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(
        bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

    if (line_mode == BFCMAP_PHY84756_INTF_1000X) {
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_LN_DEV7_AN_ANPr(pc, &data));

        *remote_ability |= (data & BFCMAP_PHY84756_DEV7_1000X_ANA_C37_FD) ?
                  BFCMAP_PHY84756_PA_1000MB_FD : 0;
        *remote_ability |= (data & BFCMAP_PHY84756_DEV7_1000X_ANA_C37_HD) ?
                               BFCMAP_PHY84756_PA_1000MB_HD : 0;
        switch (data & (BFCMAP_PHY84756_DEV7_1000X_ANA_C37_PAUSE | 
                        BFCMAP_PHY84756_DEV7_1000X_ANA_C37_ASYM_PAUSE)) {
        case BFCMAP_PHY84756_DEV7_1000X_ANA_C37_PAUSE:
            *remote_ability |= BFCMAP_PHY84756_PA_PAUSE;
            break;
        case BFCMAP_PHY84756_DEV7_1000X_ANA_C37_ASYM_PAUSE:
            *remote_ability |= BFCMAP_PHY84756_PA_PAUSE_TX;
            break;
        case (BFCMAP_PHY84756_DEV7_1000X_ANA_C37_PAUSE | 
              BFCMAP_PHY84756_DEV7_1000X_ANA_C37_ASYM_PAUSE):
            *remote_ability |= BFCMAP_PHY84756_PA_PAUSE_RX;
            break;
        }
    } else  if (line_mode == BFCMAP_PHY84756_INTF_SGMII) {
        
        
    } else {
        /* No Autoneg in SFI mode */
        return SOC_E_UNAVAIL;
    }
    return SOC_E_NONE;
}


/*
 * Function:
 *      bfcmap_phy84756_ability_local_get
 * Purpose:
 *      Get the PHY abilities
 * Parameters:
 *    phy_id    - PHY Device Address
 *    mode      - Mask indicating supported options/speeds.
 *                For Speed & Duplex:
 *                BFCMAP_PM_10MB_HD    = 10Mb, Half Duplex
 *                BFCMAP_PM_10MB_FD    = 10Mb, Full Duplex
 *                BFCMAP_PM_100MB_HD   = 100Mb, Half Duplex
 *                BFCMAP_PM_100MB_FD   = 100Mb, Full Duplex
 *                BFCMAP_PM_1000MB_HD  = 1000Mb, Half Duplex
 *                BFCMAP_PM_1000MB_FD  = 1000Mb, Full Duplex
 *                BFCMAP_PM_PAUSE_TX   = TX Pause
 *                BFCMAP_PM_PAUSE_RX   = RX Pause
 *                BFCMAP_PM_PAUSE_ASYMM = Asymmetric Pause
 *                BFCMAP_PM_SGMII       = SGMIII Supported
 *                BFCMAP_PM_XSGMII      = XSGMIII Supported
 * Returns:
 */
int
bfcmap_phy84756_ability_local_get(phy_ctrl_t *pc,
                             bfcmap_phy84756_port_ability_t *ability)
{
    bfcmap_phy84756_intf_t line_mode;

    if (ability == NULL) {
        return SOC_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(
        bfcmap_phy84756_line_intf_get(pc, 0, &line_mode));

    *ability = BFCMAP_PHY84756_PA_LB_PHY | BFCMAP_PHY84756_PA_PAUSE;
    *ability |= BFCMAP_PHY84756_PA_SGMII;

    if (line_mode == BFCMAP_PHY84756_INTF_1000X) {
        *ability |= BFCMAP_PHY84756_PA_1000MB;
    }
    if (line_mode == BFCMAP_PHY84756_INTF_SGMII) {
        *ability |= BFCMAP_PHY84756_PA_AN;
        *ability |= BFCMAP_PHY84756_PA_SPEED_ALL;
        
    } else {
        *ability |= BFCMAP_PHY84756_PA_XSGMII;
        *ability |= BFCMAP_PHY84756_PA_10000MB_FD;
    }

    return(SOC_E_NONE);
}

STATIC int
_bfcmap_phy84756_system_sgmii_speed_set(phy_ctrl_t *pc, int dev_port, int speed)
{
    buint16_t mii_ctrl = 0;

    if (speed == 0) {
        return SOC_E_NONE;
    }

    {
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_XFI_DEV7_AN_MII_CTRLr(pc, &mii_ctrl));

        mii_ctrl &= ~(BFCMAP_PHY84756_SGMII_MII_CTRL_SS_MASK);
        switch(speed) {
        case 10:
            mii_ctrl |= BFCMAP_PHY84756_AN_MII_CTRL_SS_10;
            break;
        case 100:
            mii_ctrl |= BFCMAP_PHY84756_AN_MII_CTRL_SS_100;
            break;
        case 1000:
            mii_ctrl |= BFCMAP_PHY84756_AN_MII_CTRL_SS_1000;
            break;
        default:
            return SOC_E_CONFIG;
        }
        SOC_IF_ERROR_RETURN(
            BFCMAP_WR_PHY84756_XFI_DEV7_AN_MII_CTRLr(pc, mii_ctrl));
    }
    return SOC_E_NONE;
}

STATIC int
_bfcmap_phy84756_system_sgmii_duplex_set(phy_ctrl_t *pc, int dev_port,
                                 int duplex)
{
    buint16_t mii_ctrl;

    {
        SOC_IF_ERROR_RETURN(
            BFCMAP_RD_PHY84756_XFI_DEV7_AN_MII_CTRLr(pc, &mii_ctrl));

        if ( duplex ) {
            mii_ctrl |= BFCMAP_PHY84756_AN_MII_CTRL_FD;
        } else {
            mii_ctrl &= ~BFCMAP_PHY84756_AN_MII_CTRL_FD;
        }
        SOC_IF_ERROR_RETURN(
            BFCMAP_WR_PHY84756_XFI_DEV7_AN_MII_CTRLr(pc, mii_ctrl));
    }

    return SOC_E_NONE;
}

STATIC int
_bfcmap_phy84756_system_sgmii_init(phy_ctrl_t *pc, int dev_port)
{
        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_REG_MOD(pc, 0x01, 0x0007, 0x8000, 0x0000, 0x2000));

        SOC_IF_ERROR_RETURN(
            BFCMAP_WR_PHY84756_XFI_DEV7_AN_BASE1000X_CTRL1r(pc, 0x01a0));

        SOC_IF_ERROR_RETURN(
            BFCMAP_MOD_PHY84756_XFI_DEV7_AN_MISC2r(pc, 0x6000, 0x6020));

        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_REG_WR(pc, 0x01, 0x0007, 0x835c, 0x0001));

        SOC_IF_ERROR_RETURN(
            BFCMAP_WR_PHY84756_XFI_DEV7_AN_MII_CTRLr(pc, 0x0140));

        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_REG_MOD(pc, 0x01, 0x0007, 0x8000, 0x2000, 0x2000));

    return SOC_E_NONE;
}

STATIC int
_bfcmap_phy84756_system_sgmii_sync(phy_ctrl_t *pc, int dev_port)
{
    int speed;
    int duplex;

        SOC_IF_ERROR_RETURN(
            phy_84756_fcmap_speed_get(pc->unit, pc->port, &speed));

        SOC_IF_ERROR_RETURN(
            phy_84756_fcmap_duplex_get(pc->unit, pc->port, &duplex));

        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_REG_MOD(pc, 0x01, 0x0007, 0x8000, 0x0000, 0x2000));

        SOC_IF_ERROR_RETURN(
            _bfcmap_phy84756_system_sgmii_speed_set(pc, dev_port, speed));

        SOC_IF_ERROR_RETURN(
            _bfcmap_phy84756_system_sgmii_duplex_set(pc, dev_port, duplex));

        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_REG_MOD(pc, 0x01, 0x0007, 0x8000, 0x2000, 0x2000));

    return SOC_E_NONE;
}

/*
 * Function:
 *     bfcmap_phy84756_mdio_firmware_download
 * Purpose:
 *     Download new firmware via MDIO.
 * Parameters:
 *    phy_id    - PHY Device Address
 *    new_fw    - Pointer to new firmware
 *    fw_length    - Length of the firmware
 * Returns:
 */
int
bfcmap_phy84756_mdio_firmware_download(phy_ctrl_t *pc,
                                  buint8_t *new_fw, buint32_t fw_length)
{

    buint16_t data16;
    int j;
    buint16_t num_words;

    /* 0xc848[15]=0, MDIO downloading to RAM, 0xc848[14]=1, serial boot */
    /* 0xc848[13]=0, SPI-ROM downloading not done, 0xc848[2]=0, spi port enable */
    SOC_IF_ERROR_RETURN(
           BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc848, 
               (0 << 15)|(1 << 14),
               (1 << 15)|(1 << 14)|((1 << 13) | (1 << 2))));

    /* Reset, to download code from MDIO */
    SOC_IF_ERROR_RETURN(
           BFCMAP_MOD_PHY84756_LN_DEV1_PMD_CTRLr(pc, 
               BFCMAP_PHY84756_PMD_CTRL_RESET, BFCMAP_PHY84756_PMD_CTRL_RESET));

    sal_usleep(50000);

    /* Write Starting Address, where the Code will reside in SRAM */
    data16 = 0x8000;
    SOC_IF_ERROR_RETURN
        (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, BFCMAP_PHY84756_PMAD_M8051_MSGIN_REG, data16));

    /* make sure address word is read by the micro */
    sal_usleep(10); /* Wait for 10us */

    /* Write SPI SRAM Count Size */
    data16 = (fw_length)/2;
    SOC_IF_ERROR_RETURN
        (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, BFCMAP_PHY84756_PMAD_M8051_MSGIN_REG, data16));

    /* make sure read by the micro */
    sal_usleep(10); /* Wait for 10us */

    /* Fill in the SRAM */
    num_words = (fw_length - 1);
    for (j = 0; j < num_words; j+=2) {
        /* Make sure the word is read by the Micro */
        sal_usleep(10);

        data16 = (new_fw[j] << 8) | new_fw[j+1];
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, BFCMAP_PHY84756_PMAD_M8051_MSGIN_REG,
                 data16));
    }

    /* make sure last code word is read by the micro */
    sal_usleep(20);

    /* Read Hand-Shake message (Done) from Micro */
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc,BFCMAP_PHY84756_PMAD_M8051_MSGOUT_REG, &data16)); 

    if (data16 != 0x4321 ) {
        /* Download done message */
        soc_cm_print("MDIO firmware download failed. Message: 0x%x\n", data16);
    }

    /* Clear LASI status */
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0x9003, &data16));

    /* Wait for LASI to be asserted when M8051 writes checksum to MSG_OUTr */
    sal_usleep(100); /* Wait for 100 usecs */

    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc,BFCMAP_PHY84756_PMAD_M8051_MSGOUT_REG, &data16));
    
    /* Need to check if checksum is correct */
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xCA1C, &data16));

    if (data16 != 0x600D) {
        /* Bad CHECKSUM */
        soc_cm_print("MDIO Firmware downlad failure:"
                     "Incorrect Checksum %x\n", data16);
        return SOC_E_FAIL;
    }

    /* read Rev-ID */
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xCA1A, &data16));
    SOC_DEBUG_PRINT((DK_PHY, "MDIO Firmware download completed. Version : 0x%x\n", data16));

    return SOC_E_NONE;
}

/*
 *  Function:
 *       _phy_84756_phy_84756_write_message
 *
 *  Purpose:
 *      Write into Message In and Read from MSG Out register.
 *  Input:
 *      phy_id
 *      wrdata
 *      rddata
 */

STATIC int
_phy_84756_write_message(phy_ctrl_t *pc, buint16_t wrdata, buint16_t *rddata)
{
    buint16_t tmp_data = 0;
    int iter = 0;

    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_M8501_MSGOUTr(pc, &tmp_data));
    SOC_IF_ERROR_RETURN         
        (BFCMAP_WR_PHY84756_LN_DEV1_M8501_MSGINr(pc, wrdata));

    do {
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_LASI_STATr(pc, &tmp_data));

        if (tmp_data & 0x4) {
            break;
        }
        sal_usleep(BFCMAP_WR_TIMEOUT);
        iter++;
    } while (iter < BFCMAP_WR_ITERATIONS);

    if (!(tmp_data & 0x4)) {
        soc_cm_print("write message failed due to timeout\n");
        return SOC_E_FAIL;
    }

    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_M8501_MSGOUTr(pc, &tmp_data));

    *rddata = tmp_data;
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_RX_ALARMr(pc, &tmp_data));

    return SOC_E_NONE;
}


/*
 *  Function:
 *       _phy_84756_rom_wait
 *
 *  Purpose:
 *      Wait for data to be written to the SPI-ROM.
 *  Input:
 *      phy_id
 */

STATIC int
_phy_84756_rom_wait(phy_ctrl_t *pc)
{
    buint16_t  rd_data = 0, wr_data;
    int        count;
    int        SPI_READY;
    int        iter = 0;

    do {
        count = 1;
        wr_data = ((BFCMAP_RD_CPU_CTRL_REGS * 0x0100) | count);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        wr_data = BFCMAP_SPI_CTRL_1_L;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        if (rd_data & 0x0100) {
            break;
        }
        sal_usleep(BFCMAP_WR_TIMEOUT);
        iter++;
    } while (iter < BFCMAP_WR_ITERATIONS);

    if (!(rd_data & 0x0100)) {
        soc_cm_print("_phy_84756_rom_wait: write timeout\n");
        return SOC_E_TIMEOUT;
    }

     SPI_READY = 1;
     while (SPI_READY == 1) {
        /* Set-up SPI Controller To Receive SPI EEPROM Status. */
        count = 1;
        wr_data = ((BFCMAP_WR_CPU_CTRL_REGS * 0x0100) | count);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        wr_data = BFCMAP_SPI_CTRL_2_H;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        wr_data = 0x0100;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Fill-up SPI Transmit Fifo To check SPI Status. */
        count = 2;
        wr_data = ((BFCMAP_WR_CPU_CTRL_FIFO * 0x0100) | count);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Write Tx Fifo Register Address. */
        wr_data = BFCMAP_SPI_TXFIFO;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Write SPI Tx Fifo Control Word-1. */
        wr_data = ((1 * 0x0100) | BFCMAP_MSGTYPE_HRD);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Write SPI Tx Fifo Control Word-2. */
        wr_data = BFCMAP_RDSR_OPCODE;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Write SPI Control Register Write Command. */
        count = 2;
        wr_data = ((BFCMAP_WR_CPU_CTRL_FIFO * 0x0100) | count);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Write SPI Control -1 Register Address. */
        wr_data = BFCMAP_SPI_CTRL_1_L;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Write SPI Control -1 Register Word-1. */
        wr_data = 0x0101;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Write SPI Control -1 Register Word-2. */
        wr_data = 0x0100;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Write SPI Control Register Write Command. */
        count = 1;
        wr_data = ((BFCMAP_WR_CPU_CTRL_REGS * 0x0100) | count);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Write SPI Control -1 Register Address. */
        wr_data = BFCMAP_SPI_CTRL_1_H;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Write SPI Control -1 Register Word-2. */
        wr_data = 0x0103;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Wait For 64 bytes To be written.   */
        rd_data = 0x0000;
        
        do {
            count = 1;
            wr_data = ((BFCMAP_RD_CPU_CTRL_REGS * 0x0100) | count);
            SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
            wr_data = BFCMAP_SPI_CTRL_1_L;
            SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
            if (rd_data & 0x0100) {
                break;
            }
            sal_usleep(BFCMAP_WR_TIMEOUT);
            iter ++;
        } while (iter < BFCMAP_WR_ITERATIONS); 

        if (!(rd_data & 0x0100)) {
            soc_cm_print("_phy_84756_rom_program:timeout 2\n");
            return SOC_E_TIMEOUT;
        }
        /* Write SPI Control Register Read Command. */
        count = 1;
        wr_data = ((BFCMAP_RD_CPU_CTRL_REGS * 0x0100) | count);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        /* Write SPI Control -1 Register Address. */
        wr_data = BFCMAP_SPI_RXFIFO;

        SOC_IF_ERROR_RETURN         
            (BFCMAP_WR_PHY84756_LN_DEV1_M8501_MSGINr(pc, wr_data));
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_M8501_MSGOUTr(pc, &rd_data));

        /* Clear LASI Message Out Status. */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_LASI_STATr(pc, &wr_data));

        if ((rd_data & 0x1) == 0) {
            SPI_READY = 0;
        }
     } /* SPI_READY */
     return SOC_E_NONE;
}


/*
 * Function:
 *      _phy_84756_rom_write_enable_set
 *
 * Purpose:
 *      Enable disable protection on SPI_EEPROM
 *
 * Input:
 *      phy_id
 * Output:
 *      SOC_E_xxx
 *
 * Notes:
 *          25AA256 256Kbit Serial EEPROM
 *          STATUS Register
 *          +------------------------------------------+
 *          | WPEN | x | x | x | BP1 | BP0 | WEL | WIP |
 *          +------------------------------------------+
 *      BP1 BP0  :   Protected Blocks
 *       0   0   :  Protect None
 *       1   1   :  Protect All
 *
 *      WEL : Write Latch Enable
 *       0  : Do not allow writes
 *       1  : Allow writes
 */
STATIC int
_phy_84756_rom_write_enable_set(phy_ctrl_t *pc, int enable)
{
    buint16_t  rd_data, wr_data;
    buint8_t   wrsr_data;
    int        count;

    /*
     * Write SPI Control Register Write Command.
     */
    count = 2;
    wr_data = ((BFCMAP_WR_CPU_CTRL_FIFO * 0x0100) | count);
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write SPI Control -2 Register Address.
     */
    wr_data = BFCMAP_SPI_CTRL_2_L;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write SPI Control -2 Register Word-1.
     */
    wr_data = 0x8200;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write SPI Control -2 Register Word-2.
     */
    wr_data = 0x0100;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Fill-up SPI Transmit Fifo With SPI EEPROM Messages.
     * Write SPI Control Register Write Command.
     */
    count = 4;
    wr_data = ((BFCMAP_WR_CPU_CTRL_FIFO * 0x0100) | count);
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write Tx Fifo Register Address.
     */
    wr_data = BFCMAP_SPI_TXFIFO;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write SPI Tx Fifo Control Word-1.
     */
    wr_data = ((1 * 0x0100) | BFCMAP_MSGTYPE_HWR);
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write SPI Tx Fifo Control Word-2.
     */
    wr_data = ((BFCMAP_MSGTYPE_HWR * 0x0100) | BFCMAP_WREN_OPCODE);
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write SPI Tx Fifo Control Word-3.
     */
    wr_data = ((BFCMAP_WRSR_OPCODE * 0x100) | (0x2));
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write SPI Tx Fifo Control Word-4.
     */
    wrsr_data = enable ? 0x2 : 0xc;
    wr_data = ((wrsr_data * 0x0100) | wrsr_data);
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write SPI Control Register Write Command.
     */
    count = 2;
    wr_data = ((BFCMAP_WR_CPU_CTRL_FIFO * 0x0100) | count);
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write SPI Control -1 Register Address.
     */
    wr_data = BFCMAP_SPI_CTRL_1_L;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write SPI Control -1 Register Word-1.
     */
    wr_data = 0x0101;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Write SPI Control -1 Register Word-2.
     */
    wr_data = 0x0003;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /*
     * Wait For WRSR Command To be written.
     */
    SOC_IF_ERROR_RETURN(_phy_84756_rom_wait(pc));

    return SOC_E_NONE;
}

/*
 * Function:
 *     bfcmap_phy84756_spi_firmware_update
 * Purpose:
 *     Update the firmware in SPI ROM 
 * Parameters:
 *    phy_id    - PHY Device Address
 *    new_fw    - Pointer to new firmware
 *    fw_length    - Length of the firmware
 * Returns:
 */
int
bfcmap_phy84756_spi_firmware_update(phy_ctrl_t *pc,
                                  buint8_t *array, buint32_t datalen)
{

    buint16_t   data = 0;
    int         dev_port, j;
    buint16_t   rd_data, wr_data;
    buint8_t    spi_values[BFCMAP_WR_BLOCK_SIZE];
    int count, i = 0;


    if (array == NULL) {
        return SOC_E_PARAM;
    }

    if (_phy84756_phy_get_dev_addr(pc, NULL, &dev_port) != SOC_E_NONE) {
        return SOC_E_CONFIG;
    }

    /* Set Bit 2 and Bit 0 in SPI Control */
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_LN_DEV1_SPI_CTRL_STATr(pc, 0x80FD, 0xffff));

    /* ser_boot pin high */
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_LN_DEV1_MISC_CNTL2r(pc, 0x1, 0x1));

    /* Read LASI Status */
    SOC_IF_ERROR_RETURN(
        BFCMAP_RD_PHY84756_LN_DEV1_RX_ALARM_STATr(pc, &data));
    SOC_IF_ERROR_RETURN(
        BFCMAP_RD_PHY84756_LN_DEV1_TX_ALARM_STATr(pc, &data));
    SOC_IF_ERROR_RETURN(
        BFCMAP_RD_PHY84756_LN_DEV1_LASI_STATr(pc, &data));

    /* Enable LASI for Message Out */
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_LN_DEV1_RX_ALARM_CNTLr(pc, 0x4, 0x4));

    /* Enable RX Alarm in LASI */
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_LN_DEV1_LASI_CNTLr(pc, 0x4, 0x4));

    /* Read any residual Message out register */
    SOC_IF_ERROR_RETURN(
        BFCMAP_RD_PHY84756_LN_DEV1_M8501_MSGOUTr(pc, &data));

    /* Clear LASI Message OUT status */
    SOC_IF_ERROR_RETURN(
        BFCMAP_RD_PHY84756_LN_DEV1_RX_ALARM_STATr(pc, &data));

    /* Set SPI-ROM write enable */
    SOC_IF_ERROR_RETURN(
        _phy_84756_rom_write_enable_set(pc, 1));


    for (j = 0; j < datalen; j +=BFCMAP_WR_BLOCK_SIZE) {
        /* Setup SPI Controller */
        count = 2;
        wr_data = ((BFCMAP_WR_CPU_CTRL_FIFO * 0x0100) | count);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Write SPI Control -2 Register Address.*/
        wr_data = BFCMAP_SPI_CTRL_2_L;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Write SPI Control -2 Register Word-1. */
        wr_data = 0x8200;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Write SPI Control -2 Register Word-2. */
        wr_data = 0x0100;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
        
        /* Fill-up SPI Transmit Fifo.
         * Write SPI Control Register Write Command.
         */
        count = 4 + (BFCMAP_WR_BLOCK_SIZE / 2);
        wr_data = ((BFCMAP_WR_CPU_CTRL_FIFO * 0x0100) | count);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Write Tx Fifo Register Address. */
        wr_data = BFCMAP_SPI_TXFIFO;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Write SPI Tx Fifo Control Word-1. */
        wr_data = ((1 * 0x0100) | BFCMAP_MSGTYPE_HWR);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Write SPI Tx Fifo Control Word-2. */
        wr_data = ((BFCMAP_MSGTYPE_HWR * 0x0100) | BFCMAP_WREN_OPCODE);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Write SPI Tx Fifo Control Word-3. */
        wr_data = ((BFCMAP_WR_OPCODE * 0x0100) | (0x3 + BFCMAP_WR_BLOCK_SIZE));
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Write SPI Tx Fifo Control Word-4. */
        wr_data = (((j & 0x00FF) * 0x0100) | ((j & 0xFF00) / 0x0100));
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        if (datalen < (j + BFCMAP_WR_BLOCK_SIZE)) {   /* last block */ 
            sal_memset(spi_values,0,BFCMAP_WR_BLOCK_SIZE);
            sal_memcpy(spi_values,&array[j],datalen - j);

            for (i = 0; i < BFCMAP_WR_BLOCK_SIZE; i += 2) {
                /* Write SPI Tx Fifo Data Word-4. */
                wr_data = ((spi_values[i+1] * 0x0100) | spi_values[i]);
                SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
            }           
        } else {        
            for (i = 0; i < BFCMAP_WR_BLOCK_SIZE; i += 2) {
                /* Write SPI Tx Fifo Data Word-4. */
                wr_data = ((array[j+i+1] * 0x0100) | array[j+i]);
                SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));
            }
        }

        /* 
         * Set-up SPI Controller To Transmit.
         * Write SPI Control Register Write Command.
         */
        count = 2;
        wr_data = ((BFCMAP_WR_CPU_CTRL_FIFO * 0x0100) | count);
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        wr_data = BFCMAP_SPI_CTRL_1_L;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Write SPI Control -1 Register Word-1. */
        wr_data = 0x0501;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Write SPI Control -1 Register Word-2. */
        wr_data = 0x0003;
        SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

        /* Wait For 64 bytes To be written.   */
        SOC_IF_ERROR_RETURN(_phy_84756_rom_wait(pc));
    }

    /* Clear SPI-ROM write enable */
    SOC_IF_ERROR_RETURN(
        _phy_84756_rom_write_enable_set(pc, 0));

    /* Disable SPI EEPROM. */
    count = 2;
    wr_data = ((BFCMAP_WR_CPU_CTRL_FIFO * 0x0100) | count);
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /* Write SPI Control -2 Register Address. */
    wr_data = BFCMAP_SPI_CTRL_2_L;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /* Write SPI Control -2 Register Word-1. */
    wr_data = 0x8200;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /* Write SPI Control -2 Register Word-2. */
    wr_data = 0x0100;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));


    /* Fill-up SPI Transmit Fifo With SPI EEPROM Messages. */
    count = 2;
    wr_data = ((BFCMAP_WR_CPU_CTRL_FIFO * 0x0100) | count);
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /* Write Tx Fifo Register Address. */
    wr_data = BFCMAP_SPI_TXFIFO;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /* Write SPI Tx Fifo Control Word-1. */
    wr_data = ((0x1*0x0100) | BFCMAP_MSGTYPE_HWR);
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /* Write SPI Tx Fifo Control Word-2. */
    wr_data = BFCMAP_WRDI_OPCODE;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /* Write SPI Control Register Write Command. */
    count = 2;
    wr_data = ((BFCMAP_WR_CPU_CTRL_FIFO * 0x0100) | count);
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /* Write SPI Control -1 Register Address. */
    wr_data = BFCMAP_SPI_CTRL_1_L;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /* Write SPI Control -1 Register Word-1. */
    wr_data = 0x0101;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /* Write SPI Control -1 Register Word-2. */
    wr_data = 0x0003;
    SOC_IF_ERROR_RETURN(_phy_84756_write_message(pc, wr_data, &rd_data));

    /* ser_boot pin LOW */
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_LN_DEV1_MISC_CNTL2r(pc, 0x0, 0x1));

    /* Disable Bit 2 and Bit 0 in SPI Control */
    SOC_IF_ERROR_RETURN(
        BFCMAP_MOD_PHY84756_LN_DEV1_SPI_CTRL_STATr(pc, 0xc0F9, 0xffff));

    return SOC_E_NONE;
}



void _phy_84756_fcmap_encode_egress_message_mode(soc_port_phy_timesync_event_message_egress_mode_t mode,
                                            int offset, buint16_t *value)
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

void _phy_84756_fcmap_encode_ingress_message_mode(soc_port_phy_timesync_event_message_ingress_mode_t mode,
                                            int offset, buint16_t *value)
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

void _phy_84756_fcmap_decode_egress_message_mode(buint16_t value, int offset,
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

void _phy_84756_fcmap_decode_ingress_message_mode(buint16_t value, int offset,
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

void _phy_84756_fcmap_encode_gmode(soc_port_phy_timesync_global_mode_t mode,
                                            buint16_t *value)
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

void _phy_84756_fcmap_decode_gmode(buint16_t value,
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


void _phy_84756_fcmap_encode_framesync_mode(soc_port_phy_timesync_framesync_mode_t mode,
                                            buint16_t *value)
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

void _phy_84756_fcmap_decode_framesync_mode(buint16_t value,
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

void _phy_84756_fcmap_encode_syncout_mode(soc_port_phy_timesync_syncout_mode_t mode,
                                            buint16_t *value)
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

void _phy_84756_fcmap_decode_syncout_mode(buint16_t value,
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


int
phy_84756_fcmap_timesync_config_set(int unit, soc_port_t port, soc_port_phy_timesync_config_t *conf)
{
    buint16_t rx_control_reg = 0, tx_control_reg = 0, rx_tx_control_reg = 0,
           en_control_reg = 0, capture_en_reg = 0,
           nse_sc_8 = 0, nse_nco_3 = 0, value, mpls_control_reg = 0, temp = 0,
           gmode = 0, framesync_mode = 0, syncout_mode = 0, mask = 0;
    int i;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_FLAGS) {

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_ENABLE) {
            SOC_IF_ERROR_RETURN
                (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc600, 0x8000));
            SOC_IF_ERROR_RETURN
                (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc8f0, 0x0000));
            SOC_IF_ERROR_RETURN
                (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xcd58, 0x0300));
            SOC_IF_ERROR_RETURN
                (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xcd53, 0x0000));
            en_control_reg |= 3U;
            nse_sc_8 |= (1U << 12);
        }

        /* slice enable control reg */
        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc600, en_control_reg, 
                                           3U));

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE) {
            capture_en_reg |= 3U;
        }

        /* TXRX SOP TS CAPTURE ENABLE reg */
        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc607, capture_en_reg, 3U));

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE) {
            nse_sc_8 |= (1U << 13);
        }

        /* NSE SC 8 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc63a, nse_sc_8, 
                                           (1U << 12)| (1U << 13)));

        if (conf->flags & SOC_PORT_PHY_TIMESYNC_RX_CRC_ENABLE) {
            rx_tx_control_reg |= (1U << 3);
        }

        /* RX TX CONTROL register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc61c, rx_tx_control_reg, (1U << 3)));

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

        /* TX CONTROL register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc618, tx_control_reg));
                                          
        /* RX CONTROL register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc61a, rx_control_reg));
                                          
        if (conf->flags & SOC_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT) {
            nse_nco_3 &= ~(1U << 14);
        } else {
            nse_nco_3 |= (1U << 14);
        }

        /* NSE NCO 3 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc630, nse_nco_3, 
                                           (1U << 14)));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ITPID) {
        /* VLAN TAG register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc61d, conf->itpid));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID) {
        /* OUTER VLAN TAG register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc61e, conf->otpid));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID2) {
        /* INNER VLAN TAG register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc61f, conf->otpid2));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TS_DIVIDER) {
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc636, conf->ts_divider & 0xfff));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_LINK_DELAY) {
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc605, conf->rx_link_delay & 0xffff));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc606, (conf->rx_link_delay >> 16) & 0xffff));
    }


    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE) {

        /* TIME CODE 5 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc610, (buint16_t)(conf->original_timecode.nanoseconds & 0xffff)));

        /* TIME CODE 4 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc60f, (buint16_t)((conf->original_timecode.nanoseconds >> 16) & 0xffff)));

        /* TIME CODE 3 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc60e, (buint16_t)(COMPILER_64_LO(conf->original_timecode.seconds) & 0xffff)));

        /* TIME CODE 2 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc60d, (buint16_t)((COMPILER_64_LO(conf->original_timecode.seconds) >> 16) & 0xffff)));

        /* TIME CODE 1 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc60c, (buint16_t)(COMPILER_64_HI(conf->original_timecode.seconds) & 0xffff)));
    }

    mask = 0;

    /* NSE SC 8 register */

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_GMODE) {
        _phy_84756_fcmap_encode_gmode(conf->gmode,&gmode);
        mask |= (0x3 << 14);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE) {
        _phy_84756_fcmap_encode_framesync_mode(conf->framesync.mode, &framesync_mode);
        mask |= (0xf << 2);
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc63c, conf->framesync.length_threshold));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc63b, conf->framesync.event_offset));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE) {
        _phy_84756_fcmap_encode_syncout_mode(conf->syncout.mode, &syncout_mode);
        mask |= (0x3 << 0);

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc635, conf->syncout.interval & 0xffff));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc634,
                ((conf->syncout.pulse_1_length & 0x3) << 14) | ((conf->syncout.interval >> 16) & 0x3fff)));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc633,
                ((conf->syncout.pulse_2_length & 0x1ff) << 7) | ((conf->syncout.pulse_1_length >> 2) & 0x7f)));

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc639,
                (buint16_t)(COMPILER_64_LO(conf->syncout.syncout_ts) & 0xfff0)));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc638,
                (buint16_t)((COMPILER_64_LO(conf->syncout.syncout_ts) >> 16) & 0xffff)));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc637,
                (buint16_t)(COMPILER_64_HI(conf->syncout.syncout_ts) & 0xffff)));
    }

    SOC_IF_ERROR_RETURN
        (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc63a, (gmode << 14) | (framesync_mode << 2) | (syncout_mode << 0), 
                                       mask));

    mask = 0;

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_TIMESTAMP_OFFSET) {
        /* RX TS OFFSET register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc60b, (buint16_t)(conf->rx_timestamp_offset & 0xffff)));
        mask |= 0xf000;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_TIMESTAMP_OFFSET) {
        mask |= 0x0fff;
    }

    /* TXRX TS OFFSET register */
    SOC_IF_ERROR_RETURN
        (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc60a, (buint16_t)(((conf->rx_timestamp_offset & 0xf0000) >> 4) |
                                          (conf->tx_timestamp_offset & 0xfff)), mask));
    value = 0;
    mask = 0;

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE) {
        _phy_84756_fcmap_encode_egress_message_mode(conf->tx_sync_mode, 0, &value);
        mask = 0x3 << 0;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE) {
        _phy_84756_fcmap_encode_egress_message_mode(conf->tx_delay_request_mode, 2, &value);
        mask = 0x3 << 2;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE) {
        _phy_84756_fcmap_encode_egress_message_mode(conf->tx_pdelay_request_mode, 4, &value);
        mask = 0x3 << 4;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE) {
        _phy_84756_fcmap_encode_egress_message_mode(conf->tx_pdelay_response_mode, 6, &value);
        mask = 0x3 << 6;
    }
                                            
    /* TX EVENT MESSAGE MODE SEL register */
    SOC_IF_ERROR_RETURN
        (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc601, value, 0x00ff)); 

    value = 0;
    mask = 0;

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE) {
        _phy_84756_fcmap_encode_ingress_message_mode(conf->rx_sync_mode, 0, &value);
        mask = 0x3 << 0;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE) {
        _phy_84756_fcmap_encode_ingress_message_mode(conf->rx_delay_request_mode, 2, &value);
        mask = 0x3 << 2;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE) {
        _phy_84756_fcmap_encode_ingress_message_mode(conf->rx_pdelay_request_mode, 4, &value);
        mask = 0x3 << 4;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE) {
        _phy_84756_fcmap_encode_ingress_message_mode(conf->rx_pdelay_response_mode, 6, &value);
        mask = 0x3 << 6;
    }
                                            
    /* RX EVENT MESSAGE MODE SEL register */
    SOC_IF_ERROR_RETURN
        (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc603, value, 0x00ff)); 

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE) {

        /* Initial ref phase [15:0] */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc623, (buint16_t)(COMPILER_64_LO(conf->phy_1588_dpll_ref_phase) & 0xffff)));

        /* Initial ref phase [31:16]  */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc622, (buint16_t)((COMPILER_64_LO(conf->phy_1588_dpll_ref_phase) >> 16) & 0xffff)));

        /*  Initial ref phase [47:32] */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc621, (buint16_t)(COMPILER_64_HI(conf->phy_1588_dpll_ref_phase) & 0xffff)));
    }


    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE_DELTA) {
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc625, conf->phy_1588_dpll_ref_phase_delta & 0xffff));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc624, (conf->phy_1588_dpll_ref_phase_delta >> 16) & 0xffff));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K1) {
        /* DPLL K1 */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc626, conf->phy_1588_dpll_k1 & 0xff)); 
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K2) {
        /* DPLL K2 */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc627, conf->phy_1588_dpll_k2 & 0xff)); 
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K3) {
        /* DPLL K3 */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc628, conf->phy_1588_dpll_k3 & 0xff)); 
    }


    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL) {

        /* MPLS controls */
        if (conf->mpls_control.flags & SOC_PORT_PHY_TIMESYNC_MPLS_ENABLE) {
            mpls_control_reg |= (1U << 7) | (1U << 3);
        }

        if (conf->mpls_control.flags & SOC_PORT_PHY_TIMESYNC_MPLS_ENTROPY_ENABLE) {
            mpls_control_reg |= (1U << 6) | (1U << 2);
        }

        if (conf->mpls_control.flags & SOC_PORT_PHY_TIMESYNC_MPLS_SPECIAL_LABEL_ENABLE) {
            mpls_control_reg |= (1U << 4);
        }

        if (conf->mpls_control.flags & SOC_PORT_PHY_TIMESYNC_MPLS_CONTROL_WORD_ENABLE) {
            mpls_control_reg |= (1U << 5) | (1U << 1);
        }

        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc651, mpls_control_reg, 
                (1U << 7) | (1U << 3)| (1U << 6) | (1U << 2) | (1U << 4) | (1U << 5) | (1U << 1))); 

        /* special label [19:16] */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc653, (buint16_t)((conf->mpls_control.special_label >> 16 ) & 0xf)));
                                          
        /* special label [15:0] */
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc652, (buint16_t)(conf->mpls_control.special_label & 0xffff)));

        for (i = 0; i < 10; i++ ) {
            SOC_IF_ERROR_RETURN
                (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc654 + i, (buint16_t)(conf->mpls_control.labels[i].value & 0xffff)));
            SOC_IF_ERROR_RETURN
                (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc661 + i, (buint16_t)(conf->mpls_control.labels[i].mask & 0xffff)));
        }
 
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc65e, (buint16_t)(
                ((conf->mpls_control.labels[3].value >> 4)  & 0xf000) | 
                ((conf->mpls_control.labels[2].value >> 8)  & 0x0f00) | 
                ((conf->mpls_control.labels[1].value >> 12) & 0x00f0) | 
                ((conf->mpls_control.labels[0].value >> 16) & 0x000f))));

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc65f, (buint16_t)(
                ((conf->mpls_control.labels[7].value >> 4)  & 0xf000) | 
                ((conf->mpls_control.labels[6].value >> 8)  & 0x0f00) | 
                ((conf->mpls_control.labels[5].value >> 12) & 0x00f0) | 
                ((conf->mpls_control.labels[4].value >> 16) & 0x000f))));

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc660, (buint16_t)(
                ((conf->mpls_control.labels[9].value >> 12) & 0x00f0) | 
                ((conf->mpls_control.labels[8].value >> 16) & 0x000f))));

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc66b, (buint16_t)(
                ((conf->mpls_control.labels[3].mask >> 4)  & 0xf000) | 
                ((conf->mpls_control.labels[2].mask >> 8)  & 0x0f00) | 
                ((conf->mpls_control.labels[1].mask >> 12) & 0x00f0) | 
                ((conf->mpls_control.labels[0].mask >> 16) & 0x000f))));

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc66c, (buint16_t)(
                ((conf->mpls_control.labels[7].mask >> 4)  & 0xf000) | 
                ((conf->mpls_control.labels[6].mask >> 8)  & 0x0f00) | 
                ((conf->mpls_control.labels[5].mask >> 12) & 0x00f0) | 
                ((conf->mpls_control.labels[4].mask >> 16) & 0x000f))));

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc66d, (buint16_t)(
                ((conf->mpls_control.labels[9].mask >> 12) & 0x00f0) | 
                ((conf->mpls_control.labels[8].mask >> 16) & 0x000f))));

        /* label flags */
        temp = 0;
        for (i = 0; i < 8; i++ ) {
            temp |= conf->mpls_control.labels[i].flags & SOC_PORT_PHY_TIMESYNC_MPLS_LABEL_OUT ? (0x2 << (i<<1)) : 0;
            temp |= (conf->mpls_control.labels[i].flags & SOC_PORT_PHY_TIMESYNC_MPLS_LABEL_IN)  ? (0x1 << (i<<1)) : 0;
        }

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc66e, temp)); 

        temp = 0;
        for (i = 0; i < 2; i++ ) {
            temp |= (conf->mpls_control.labels[i + 8].flags & SOC_PORT_PHY_TIMESYNC_MPLS_LABEL_OUT) ? (0x2 << (i<<1)) : 0;
            temp |= (conf->mpls_control.labels[i + 8].flags & SOC_PORT_PHY_TIMESYNC_MPLS_LABEL_IN)  ? (0x1 << (i<<1)) : 0;
        }

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc66f, temp)); 
    }


    return SOC_E_NONE;
}

int
phy_84756_fcmap_timesync_config_get(int unit, soc_port_t port, soc_port_phy_timesync_config_t *conf)
{
    buint16_t rx_tx_control_reg = 0, tx_control_reg = 0,
              en_control_reg = 0, capture_en_reg = 0,
              nse_sc_8 = 0, nse_nco_3 = 0, temp1, temp2, temp3, value, mpls_control_reg;
    soc_port_phy_timesync_global_mode_t gmode = SOC_PORT_PHY_TIMESYNC_MODE_FREE;
    soc_port_phy_timesync_framesync_mode_t framesync_mode = SOC_PORT_PHY_TIMESYNC_FRAMESYNC_NONE;
    soc_port_phy_timesync_syncout_mode_t syncout_mode = SOC_PORT_PHY_TIMESYNC_SYNCOUT_DISABLE;
    int i;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    conf->flags = 0;

    /* NSE SC 8 register */
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc63a, &nse_sc_8));

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_FLAGS) {
        /* SLICE ENABLE CONTROL register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc600, &en_control_reg)); 

        if (en_control_reg & 1U) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_ENABLE;
        }

        /* TXRX SOP TS CAPTURE ENABLE reg */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc607, &capture_en_reg)); 

        if (capture_en_reg & 1U) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_CAPTURE_TS_ENABLE;
        }

        /* NSE SC 8 register */
        /* SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc63a, &nse_sc_8));
         */

        if (nse_sc_8 & (1U << 13)) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_HEARTBEAT_TS_ENABLE;
        }

         /* RX TX CONTROL register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc61c, &rx_tx_control_reg));

        if (rx_tx_control_reg & (1U << 3)) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_RX_CRC_ENABLE;
        }

        /* TX CONTROL register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc618, &tx_control_reg));
                                          
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
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc630, &nse_nco_3));

        if (!(nse_nco_3 & (1U << 14))) {
            conf->flags |= SOC_PORT_PHY_TIMESYNC_CLOCK_SRC_EXT;
        }
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_GMODE) {
        _phy_84756_fcmap_decode_gmode((nse_sc_8 >> 14),&gmode);
        conf->gmode = gmode;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_FRAMESYNC_MODE) {
        _phy_84756_fcmap_decode_framesync_mode((nse_sc_8 >> 2), &framesync_mode);
        conf->framesync.mode = framesync_mode;
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc63c, &temp1));
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc63b, &temp2));
        conf->framesync.length_threshold = temp1;
        conf->framesync.event_offset = temp2;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_SYNCOUT_MODE) {
        _phy_84756_fcmap_decode_syncout_mode((nse_sc_8 >> 2), &syncout_mode);
        conf->syncout.mode = syncout_mode;

        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc635, &temp1));
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc634, &temp2));
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc633, &temp3));

        conf->syncout.pulse_1_length =  ((temp3 & 0x7f) << 2) | ((temp2 >> 14) & 0x3);
        conf->syncout.pulse_2_length = (temp3 >> 7) & 0x1ff;
        conf->syncout.interval =  ((temp2 & 0x3fff) << 16) | temp1;

        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc639, &temp1));
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc638, &temp2));
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc637, &temp3));

        COMPILER_64_SET(conf->syncout.syncout_ts, ((buint32_t)temp3), (((buint32_t)temp2<<16)|((buint32_t)temp1)));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ITPID) {
        /* VLAN TAG register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc61d, &conf->itpid));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID) {
        /* OUTER VLAN TAG register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc61e, &conf->otpid));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_OTPID2) {
        /* INNER VLAN TAG register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc61f, &conf->otpid2));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TS_DIVIDER) {
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc636, &conf->ts_divider));
        conf->ts_divider &= 0xfff;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_LINK_DELAY) {
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc605, &temp1));
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc606, &temp2));

        conf->rx_link_delay = ((buint32_t)temp2 << 16) | temp1; 
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_ORIGINAL_TIMECODE) {
        /* TIME CODE 5 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc610, &temp1));

        /* TIME CODE 4 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc60f, &temp2));

        conf->original_timecode.nanoseconds = ((buint32_t)temp2 << 16) | temp1; 

        /* TIME CODE 3 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc60e, &temp1));

        /* TIME CODE 2 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc60d, &temp2));

        /* TIME CODE 1 register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc60c, &temp3));

        /* conf->original_timecode.seconds = ((uint64)temp3 << 32) | ((buint32_t)temp2 << 16) | temp1; */

        COMPILER_64_SET(conf->original_timecode.seconds, ((buint32_t)temp3),  (((buint32_t)temp2<<16)|((buint32_t)temp1)));
    }

    /* TXRX TS OFFSET register */
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc60a, &temp1));

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_TIMESTAMP_OFFSET) {
    /* RX TS OFFSET register */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc60b, &temp2));
        conf->rx_timestamp_offset = (((buint32_t)(temp1 & 0xf000)) << 4) | temp2;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_TIMESTAMP_OFFSET) {
        conf->tx_timestamp_offset = temp1 & 0xfff;
    }

    /* TX EVENT MESSAGE MODE SEL register */
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc601, &value)); 

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_SYNC_MODE) {
        _phy_84756_fcmap_decode_egress_message_mode(value, 0, &conf->tx_sync_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_DELAY_REQUEST_MODE) {
        _phy_84756_fcmap_decode_egress_message_mode(value, 2, &conf->tx_delay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_REQUEST_MODE) {
        _phy_84756_fcmap_decode_egress_message_mode(value, 4, &conf->tx_pdelay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_TX_PDELAY_RESPONSE_MODE) {
        _phy_84756_fcmap_decode_egress_message_mode(value, 6, &conf->tx_pdelay_response_mode);
    }
                                            
    /* RX EVENT MESSAGE MODE SEL register */
    SOC_IF_ERROR_RETURN
        (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc603, &value)); 

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_SYNC_MODE) {
        _phy_84756_fcmap_decode_ingress_message_mode(value, 0, &conf->rx_sync_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_DELAY_REQUEST_MODE) {
        _phy_84756_fcmap_decode_ingress_message_mode(value, 2, &conf->rx_delay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_REQUEST_MODE) {
        _phy_84756_fcmap_decode_ingress_message_mode(value, 4, &conf->rx_pdelay_request_mode);
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_RX_PDELAY_RESPONSE_MODE) {
        _phy_84756_fcmap_decode_ingress_message_mode(value, 6, &conf->rx_pdelay_response_mode);
    }
                                            
    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE) {
        /* Initial ref phase [15:0] */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc623, &temp1));

        /* Initial ref phase [31:16] */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc622, &temp2));

        /* Initial ref phase [47:32] */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc621, &temp3));

        /* conf->phy_1588_dpll_phase_initial = ((uint64)temp3 << 32) | ((buint32_t)temp2 << 16) | temp1; */

        COMPILER_64_SET(conf->phy_1588_dpll_ref_phase, ((buint32_t)temp3), (((buint32_t)temp2<<16)|((buint32_t)temp1)));
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_REF_PHASE_DELTA) {
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc625, &temp1));
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc624, &temp2));

        conf->phy_1588_dpll_ref_phase_delta = ((buint32_t)temp2 << 16) | temp1; 
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K1) {
        /* DPLL K1 */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc626, &conf->phy_1588_dpll_k1)); 
        conf->phy_1588_dpll_k1 &= 0xff;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K2) {
        /* DPLL K2 */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc627, &conf->phy_1588_dpll_k2)); 
        conf->phy_1588_dpll_k2 &= 0xff;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_PHY_1588_DPLL_K3) {
        /* DPLL K3 */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc628, &conf->phy_1588_dpll_k3)); 
        conf->phy_1588_dpll_k3 &= 0xff;
    }

    if (conf->validity_mask & SOC_PORT_PHY_TIMESYNC_VALID_MPLS_CONTROL) {
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc651, &mpls_control_reg));
                                          
        conf->mpls_control.flags = 0;

        if (mpls_control_reg & (1U << 7)) {
            conf->mpls_control.flags |= SOC_PORT_PHY_TIMESYNC_MPLS_ENABLE;
        }

        if (mpls_control_reg & (1U << 6)) {
            conf->mpls_control.flags |= SOC_PORT_PHY_TIMESYNC_MPLS_ENTROPY_ENABLE;
        }

        if (mpls_control_reg & (1U << 4)) {
            conf->mpls_control.flags |= SOC_PORT_PHY_TIMESYNC_MPLS_SPECIAL_LABEL_ENABLE;
        }

        if (mpls_control_reg & (1U << 5)) {
            conf->mpls_control.flags |= SOC_PORT_PHY_TIMESYNC_MPLS_CONTROL_WORD_ENABLE;
        }

        /* special label [19:16] */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc653, &temp2));
                                          
        /* special label [15:0] */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc652, &temp1));

        conf->mpls_control.special_label = ((buint32_t)(temp2 & 0x0f) << 16) | temp1; 


        for (i = 0; i < 10; i++ ) {

            SOC_IF_ERROR_RETURN
                (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc654 + i, &temp1));
            conf->mpls_control.labels[i].value = temp1;

            SOC_IF_ERROR_RETURN
                (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc661 + i, &temp1));
            conf->mpls_control.labels[i].mask = temp1;
        }
 
        /* now get [19:16] of labels */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc65e, &temp1));

        conf->mpls_control.labels[0].value |= ((buint32_t)(temp1 & 0x000f) << 16); 
        conf->mpls_control.labels[1].value |= ((buint32_t)(temp1 & 0x00f0) << 12); 
        conf->mpls_control.labels[2].value |= ((buint32_t)(temp1 & 0x0f00) << 8); 
        conf->mpls_control.labels[3].value |= ((buint32_t)(temp1 & 0xf000) << 4); 


        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc65f, &temp1));

        conf->mpls_control.labels[4].value |= ((buint32_t)(temp1 & 0x000f) << 16); 
        conf->mpls_control.labels[5].value |= ((buint32_t)(temp1 & 0x00f0) << 12); 
        conf->mpls_control.labels[6].value |= ((buint32_t)(temp1 & 0x0f00) << 8); 
        conf->mpls_control.labels[7].value |= ((buint32_t)(temp1 & 0xf000) << 4); 


        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc660, &temp1));

        conf->mpls_control.labels[8].value |= ((buint32_t)(temp1 & 0x000f) << 16); 
        conf->mpls_control.labels[9].value |= ((buint32_t)(temp1 & 0x00f0) << 12); 

        /* now get [19:16] of masks */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc66b, &temp1));

        conf->mpls_control.labels[0].mask |= ((buint32_t)(temp1 & 0x000f) << 16); 
        conf->mpls_control.labels[1].mask |= ((buint32_t)(temp1 & 0x00f0) << 12); 
        conf->mpls_control.labels[2].mask |= ((buint32_t)(temp1 & 0x0f00) << 8); 
        conf->mpls_control.labels[3].mask |= ((buint32_t)(temp1 & 0xf000) << 4); 


        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc66c, &temp1));

        conf->mpls_control.labels[4].mask |= ((buint32_t)(temp1 & 0x000f) << 16); 
        conf->mpls_control.labels[5].mask |= ((buint32_t)(temp1 & 0x00f0) << 12); 
        conf->mpls_control.labels[6].mask |= ((buint32_t)(temp1 & 0x0f00) << 8); 
        conf->mpls_control.labels[7].mask |= ((buint32_t)(temp1 & 0xf000) << 4); 


        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc66d, &temp1));

        conf->mpls_control.labels[8].mask |= ((buint32_t)(temp1 & 0x000f) << 16); 
        conf->mpls_control.labels[9].mask |= ((buint32_t)(temp1 & 0x00f0) << 12); 


        /* label flags */
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc66e, &temp1)); 
        for (i = 0; i < 8; i++ ) {
            conf->mpls_control.labels[i].flags |= temp1 & (0x2 << (i<<1)) ? SOC_PORT_PHY_TIMESYNC_MPLS_LABEL_OUT : 0;
            conf->mpls_control.labels[i].flags |= temp1 & (0x1 << (i<<1)) ? SOC_PORT_PHY_TIMESYNC_MPLS_LABEL_IN : 0;
        }
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc66f, &temp2)); 
        for (i = 0; i < 2; i++ ) {
            conf->mpls_control.labels[i + 8].flags |= temp2 & (0x2 << (i<<1)) ? SOC_PORT_PHY_TIMESYNC_MPLS_LABEL_OUT : 0;
            conf->mpls_control.labels[i + 8].flags |= temp2 & (0x1 << (i<<1)) ? SOC_PORT_PHY_TIMESYNC_MPLS_LABEL_IN : 0;
        }

    }
    return SOC_E_NONE;
}

int
phy_84756_fcmap_timesync_control_set(int unit, soc_port_t port, soc_port_control_phy_timesync_t type, uint64 value)
{
    buint16_t temp1, temp2; 
    buint32_t value0;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    switch (type) {

    case SOC_PORT_CONTROL_PHY_TIMESYNC_CAPTURE_TIMESTAMP:
    case SOC_PORT_CONTROL_PHY_TIMESYNC_HEARTBEAT_TIMESTAMP:
        return SOC_E_FAIL;


    case SOC_PORT_CONTROL_PHY_TIMESYNC_NCOADDEND:
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc62f, (buint16_t)(COMPILER_64_LO(value) & 0xffff)));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc62e, (buint16_t)((COMPILER_64_LO(value) >> 16) & 0xffff)));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_FRAMESYNC:

        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc63a, &temp1));
        temp2 = ((temp1 | (0x3 << 14) | (0x1 << 12)) & ~(0xf << 2)) | (0x1 << 5);
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc63a, temp2));
        sal_usleep(1);
        temp2 &= ~((0x1 << 5) | (0x1 << 12));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc63a, temp2));
        sal_usleep(1);
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc63a, temp1));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_LOCAL_TIME:
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc632, (buint16_t)(COMPILER_64_LO(value) & 0xffff)));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc631, (buint16_t)((COMPILER_64_LO(value) >> 16) & 0xffff)));
        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc630, (buint16_t)COMPILER_64_HI(value), 0x0fff));
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
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc614, temp1));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc615, temp2));

        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT:
        temp1 = 0;

        value0 = COMPILER_64_LO(value);

        if (value0 &  SOC_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT) {
            temp1 |= 1U << 1;
        }
        if (value0 &  SOC_PORT_PHY_TIMESYNC_FRAMESYNC_INTERRUPT) {
            temp1 |= 1U << 0;
        }
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc617, temp1));
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
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc616, temp1));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_TX_TIMESTAMP_OFFSET:
        value0 = COMPILER_64_LO(value);

        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc60a, (buint16_t)(value0 & 0x0fff), 0x0fff));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_RX_TIMESTAMP_OFFSET:
        value0 = COMPILER_64_LO(value);

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc60b, (buint16_t)(value0 & 0xffff)));
        SOC_IF_ERROR_RETURN
            (BFCMAP_MOD_PHY84756_LN_DEV1_PMDr(pc, 0xc60a, (buint16_t)((value0 >> 4) & 0xf000), 0xf000));
        break;

    default:
        return SOC_E_NONE;
        break;
    }

    return SOC_E_NONE;
}

int
phy_84756_fcmap_timesync_control_get(int unit, soc_port_t port, soc_port_control_phy_timesync_t type, uint64 *value)
{
    buint16_t value0 = 0;
    buint16_t value1 = 0;
    buint16_t value2 = 0;
    buint16_t value3 = 0;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    switch (type) {
    case SOC_PORT_CONTROL_PHY_TIMESYNC_HEARTBEAT_TIMESTAMP:

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc63d, 0x4));

        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc647, &value0)); 
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc646, &value1)); 
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc645, &value2)); 

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc63d, 0x8));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc63d, 0x0));

    /*    *value = (((uint64)value3) << 48) | (((uint64)value2) << 32) | (((uint64)value1) << 16) | ((uint64)value0); */
        COMPILER_64_SET((*value), (((buint32_t)value3<<16)|((buint32_t)value2)),  (((buint32_t)value1<<16)|((buint32_t)value0)));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_CAPTURE_TIMESTAMP:

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc63d, 0x1));

        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc640, &value0)); 
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc63f, &value1)); 
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc63e, &value2)); 

        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc63d, 0x2));
        SOC_IF_ERROR_RETURN
            (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0xc63d, 0x0));

    /*   *value = (((uint64)value3) << 48) | (((uint64)value2) << 32) | (((uint64)value1) << 16) | ((uint64)value0); */
        COMPILER_64_SET((*value), (((buint32_t)value3<<16)|((buint32_t)value2)),  (((buint32_t)value1<<16)|((buint32_t)value0)));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_NCOADDEND:
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc62f, &value0));
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc62e, &value1));
        COMPILER_64_SET((*value), (buint32_t)value1, (buint32_t)value0);
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_LOCAL_TIME:
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc632, &value0)); 
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc631, &value1)); 
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc630, &value2)); 

        COMPILER_64_SET((*value), ((buint32_t)(value2 & 0x0fff)),  (((buint32_t)value1<<16)|((buint32_t)value0)));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_LOAD_CONTROL:

        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0x2e, &value1));
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0x2f, &value2));

        value0 = 0;

        if (value1 & (1U << 11)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_TN_LOAD;
        }
        if (value2 & (1U << 11)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_TN_ALWAYS_LOAD;
        }
        if (value1 & (1U << 10)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_TIMECODE_LOAD;
        }
        if (value2 & (1U << 10)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_TIMECODE_ALWAYS_LOAD;
        }
        if (value1 & (1U << 9)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_SYNCOUT_LOAD;
        }
        if (value2 & (1U << 9)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_SYNCOUT_ALWAYS_LOAD;
        }
        if (value1 & (1U << 8)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_NCO_DIVIDER_LOAD;
        }
        if (value1 & (1U << 8)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_NCO_DIVIDER_ALWAYS_LOAD;
        }
        if (value1 & (1U << 7)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_LOCAL_TIME_LOAD;
        }
        if (value1 & (1U << 7)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_LOCAL_TIME_ALWAYS_LOAD;
        }
        if (value1 & (1U << 6)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_NCO_ADDEND_LOAD;
        }
        if (value2 & (1U << 6)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_NCO_ADDEND_ALWAYS_LOAD;
        }
        if (value1 & (1U << 5)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_LOAD;
        }
        if (value2 & (1U << 5)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_LOOP_FILTER_ALWAYS_LOAD;
        }
        if (value1 & (1U << 4)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_LOAD;
        }
        if (value2 & (1U << 4)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_ALWAYS_LOAD;
        }
        if (value1 & (1U << 3)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_LOAD;
        }
        if (value2 & (1U << 3)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_REF_PHASE_DELTA_ALWAYS_LOAD;
        }
        if (value1 & (1U << 2)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_K3_LOAD;
        }
        if (value2 & (1U << 2)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_K3_ALWAYS_LOAD;
        }
        if (value1 & (1U << 1)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_K2_LOAD;
        }
        if (value2 & (1U << 1)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_K2_ALWAYS_LOAD;
        }
        if (value1 & (1U << 0)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_K1_LOAD;
        }
        if (value2 & (1U << 0)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_DPLL_K1_ALWAYS_LOAD;
        }
        COMPILER_64_SET((*value), 0, (buint32_t)value0);

        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT:
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc617, &value1));

        value0 = 0;

        if (value1 & (1U << 1)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT;
        }
        if (value1 & (1U << 0)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_FRAMESYNC_INTERRUPT;
        }
        COMPILER_64_SET((*value), 0, (buint32_t)value0);

        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_INTERRUPT_MASK:
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc616, &value1));

        value0 = 0;

        if (value1 & (1U << 1)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_TIMESTAMP_INTERRUPT_MASK;
        }
        if (value1 & (1U << 0)) {
            value0 |= SOC_PORT_PHY_TIMESYNC_FRAMESYNC_INTERRUPT_MASK;
        }
        COMPILER_64_SET((*value), 0, (buint32_t)value0);

        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_TX_TIMESTAMP_OFFSET:
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc60a, &value0));

        COMPILER_64_SET((*value), 0, (buint32_t)(value0 & 0x0fff));
        break;

    case SOC_PORT_CONTROL_PHY_TIMESYNC_RX_TIMESTAMP_OFFSET:
        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc60b, &value0));

        SOC_IF_ERROR_RETURN
            (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0xc60a, &value1));

        COMPILER_64_SET((*value), 0, (buint32_t)(((((buint32_t)value1) << 4) & 0xf0000) | value0));
        break;

    default:
        return SOC_E_FAIL;
    }

    return SOC_E_NONE;
}

int
bfcmap_phy84756_reg_read(phy_ctrl_t *  pc, buint32_t flags, buint8_t reg_bank,
                          buint16_t reg_addr, buint16_t *data)
{
    int rv;
    buint16_t reg_data, xpmd_reg_sel = 0xffff;
    buint32_t cl45_reg_addr;

    rv = SOC_E_NONE;

    if (flags & BFCMAP_PHY84756_SYS_SIDE) {
        /* Select System side registers */
        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(1, xpmd_reg_sel);
        reg_data = 0x01;
        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_IO_MDIO_WRITE(pc, cl45_reg_addr, reg_data));

        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(reg_bank, reg_addr);
        rv = BFCMAP_PHY84756_IO_MDIO_READ(pc, cl45_reg_addr, data);

        /* De-Select System side registers */
        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(1, xpmd_reg_sel);
        reg_data = 0x0;
        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_IO_MDIO_WRITE(pc, cl45_reg_addr, reg_data));

    } else {
        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(reg_bank, reg_addr);
        rv = BFCMAP_PHY84756_IO_MDIO_READ(pc, cl45_reg_addr, data);
    }

    return rv;
}

int
bfcmap_phy84756_reg_write(phy_ctrl_t *  pc, buint32_t flags, buint8_t reg_bank,
                           buint16_t reg_addr, buint16_t data)
{
    int rv;
    buint16_t reg_data, xpmd_reg_sel = 0xffff;
    buint32_t cl45_reg_addr;

    rv = SOC_E_NONE;

    if (flags & BFCMAP_PHY84756_SYS_SIDE) {
        /* Select System side registers */
        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(1, xpmd_reg_sel);
        reg_data = 0x01;
        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_IO_MDIO_WRITE(pc, cl45_reg_addr, reg_data));

        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(reg_bank, reg_addr);
        rv = BFCMAP_PHY84756_IO_MDIO_WRITE(pc, cl45_reg_addr, data);

        /* De-Select System side registers */
        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(1, xpmd_reg_sel);
        reg_data = 0x00;
        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_IO_MDIO_WRITE(pc, cl45_reg_addr, reg_data));

    } else {
        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(reg_bank, reg_addr);
        rv = BFCMAP_PHY84756_IO_MDIO_WRITE(pc, cl45_reg_addr, data);
    }

    return rv;
}


int
bfcmap_phy84756_reg_modify(phy_ctrl_t *  pc, buint32_t flags, buint8_t reg_bank,
                            buint16_t reg_addr, buint16_t data, buint16_t mask)
{
    buint16_t  tmp, otmp, reg_data;
    buint32_t cl45_reg_addr = 0;
    buint16_t xpmd_reg_sel = 0xffff;


    if (flags & BFCMAP_PHY84756_SYS_SIDE) {
        /* Select System side registers */
        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(1, xpmd_reg_sel);
        reg_data = 0x01;
        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_IO_MDIO_WRITE(pc, cl45_reg_addr, reg_data));

        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(reg_bank, reg_addr);

        SOC_IF_ERROR_RETURN
            (BFCMAP_PHY84756_IO_MDIO_READ(pc, cl45_reg_addr, &tmp));

        reg_data = data & mask; /* Mask off other bits */
        otmp = tmp;
        tmp &= ~(mask);
        tmp |= reg_data;

        if (otmp != tmp) {
            SOC_IF_ERROR_RETURN
                (BFCMAP_PHY84756_IO_MDIO_WRITE(pc, cl45_reg_addr, tmp));
        }

        /* De-Select System side registers */
        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(1, xpmd_reg_sel);
        data = 0x00;
        SOC_IF_ERROR_RETURN(
            BFCMAP_PHY84756_IO_MDIO_WRITE(pc, cl45_reg_addr, data));

    } else {
        cl45_reg_addr = BLMI_IO_CL45_ADDRESS(reg_bank, reg_addr);

        SOC_IF_ERROR_RETURN
            (BFCMAP_PHY84756_IO_MDIO_READ(pc, cl45_reg_addr, &tmp));

        reg_data = data & mask; /* Mask off other bits */
        otmp = tmp;
        tmp &= ~(mask);
        tmp |= reg_data;

        if (otmp != tmp) {
            SOC_IF_ERROR_RETURN
                (BFCMAP_PHY84756_IO_MDIO_WRITE(pc, cl45_reg_addr, tmp));
        }
    }
    return SOC_E_NONE;
}



STATIC int 
phy_84756_fcmap_reg_read(int unit, soc_port_t port, uint32 flags,
                                uint32 phy_reg_addr, uint32 *phy_data)
{
	uint16               data16;
    uint16               regdata;
    phy_ctrl_t          *pc;      /* PHY software state */
    int rv = SOC_E_NONE;
	int	rd_cnt;

    pc = EXT_PHY_SW_STATE(unit, port);

    rd_cnt = 1;

    if (flags & SOC_PHY_I2C_DATA8) {

        SOC_IF_ERROR_RETURN
            (phy_84756_i2cdev_read(pc,
                        SOC_PHY_I2C_DEVAD(phy_reg_addr),
                        SOC_PHY_I2C_REGAD(phy_reg_addr),
                        rd_cnt,
                        (uint8 *)&data16));
        *phy_data = *((uint8 *)&data16);

    } else if (flags & SOC_PHY_I2C_DATA16) {
        /* This operation is generally targeted to access 16-bit device,
         * such as PHY IC inside the SFP.  However there is no 16-bit
         * scratch register space on the device.  Use 1.800e
         * for this operation.
         */
        SOC_IF_ERROR_RETURN
            (READ_PHY84756_PMA_PMD_REG(unit, pc, PHY84756_I2C_TEMP_RAM,
                   &regdata));

        rv = _phy_84756_bsc_rw(pc,
               SOC_PHY_I2C_DEVAD(phy_reg_addr),
               PHY84756_I2C_OP_TYPE(PHY84756_I2CDEV_READ,PHY84756_I2C_16BIT),
               SOC_PHY_I2C_REGAD(phy_reg_addr),1,
               (void *)&data16,PHY84756_I2C_TEMP_RAM);

        *phy_data = data16;

        /* restore the ram register value */
        SOC_IF_ERROR_RETURN
            (WRITE_PHY84756_PMA_PMD_REG(unit, pc, PHY84756_I2C_TEMP_RAM,
                 regdata));
    } else {
        SOC_IF_ERROR_RETURN
            (READ_PHY_REG(unit, pc, phy_reg_addr, &data16));
        *phy_data = data16;
    }

    return rv;

}



STATIC int
phy_84756_fcmap_reg_write(int unit, soc_port_t port, uint32 flags,
                          uint32 phy_reg_addr, uint32 phy_data)
{
    uint8  data8[4];
    uint16 data16[2];
    uint16 regdata[2];
    phy_ctrl_t          *pc;      /* PHY software state */
    int rv = SOC_E_NONE;
    int wr_cnt;

    pc = EXT_PHY_SW_STATE(unit, port);

    wr_cnt = 1;

    if (flags & SOC_PHY_I2C_DATA8) {
        data8[0] = (uint8)phy_data;
        SOC_IF_ERROR_RETURN
            (phy_84756_i2cdev_write(pc,
                        SOC_PHY_I2C_DEVAD(phy_reg_addr),
                        SOC_PHY_I2C_REGAD(phy_reg_addr),
                        wr_cnt,
                        data8));
    } else if (flags & SOC_PHY_I2C_DATA16) {

     /* This operation is generally targeted to access 16-bit device,
         * such as PHY IC inside the SFP.  However there is no 16-bit
         * scratch register space on the device.  Use 1.800e
         * for this operation.
         */
        /* save the temp ram register */
        SOC_IF_ERROR_RETURN
            (READ_PHY84756_PMA_PMD_REG(unit, pc, PHY84756_I2C_TEMP_RAM,
                 &regdata[0]));
        data16[0] = phy_data;
        rv = _phy_84756_bsc_rw(pc,
              SOC_PHY_I2C_DEVAD(phy_reg_addr),
              PHY84756_I2C_OP_TYPE(PHY84756_I2CDEV_READ,PHY84756_I2C_16BIT),
              SOC_PHY_I2C_REGAD(phy_reg_addr),wr_cnt,
              (void *)data16,PHY84756_I2C_TEMP_RAM);

        /* restore the ram register value */
        SOC_IF_ERROR_RETURN
            (WRITE_PHY84756_PMA_PMD_REG(unit, pc, PHY84756_I2C_TEMP_RAM,
                  regdata[0]));
    } else {
        SOC_IF_ERROR_RETURN
            (WRITE_PHY_REG(unit, pc, phy_reg_addr, (uint16)phy_data));
    }
    return rv;
}


/*
 * Variable:
 *    phy_84756_fcmap_drv
 * Purpose:
 *    Phy Driver for BCM84756 FCMAP PHY
 */

phy_driver_t phy_84756drv_fcmap_xe = {
    "84756 1G/10-Gigabit PHY-FCMAP Driver",
    phy_84756_fcmap_init,        /* Init */
    phy_null_reset,       /* Reset */
    phy_84756_fcmap_link_get,    /* Link get   */
    phy_84756_fcmap_enable_set,  /* Enable set */
    phy_null_enable_get,  /* Enable get */
    phy_84756_fcmap_duplex_set,  /* Duplex set */
    phy_84756_fcmap_duplex_get,  /* Duplex get */
    phy_84756_fcmap_speed_set,   /* Speed set  */
    phy_84756_fcmap_speed_get,   /* Speed get  */
    phy_null_set,          /* Master set */
    phy_null_zero_get,     /* Master get */
    phy_84756_fcmap_an_set,      /* ANA set */
    phy_84756_fcmap_an_get,      /* ANA get */
    NULL,                 /* Local Advert set, deprecated */
    NULL,                 /* Local Advert get, deprecated */
    NULL,                 /* Remote Advert get, deprecated */
    phy_84756_fcmap_lb_set,      /* PHY loopback set */
    phy_84756_fcmap_lb_get,      /* PHY loopback set */
    phy_null_interface_set, /* IO Interface set */
    phy_84756_fcmap_interface_get, /* IO Interface get */
    NULL,                   /* pd_ability, deprecated */
    phy_84756_fcmap_linkup,
    NULL,
    phy_null_mdix_set,        /* phy_84756_fcmap_mdix_set */
    phy_null_mdix_get,        /* phy_84756_fcmap_mdix_get */
    phy_null_mdix_status_get, /* phy_84756_fcmap_mdix_status_get */
    NULL, /* medium config setting set */
    NULL, /* medium config setting get */
    phy_84756_fcmap_medium_status,        /* active medium */
    NULL,                    /* phy_cable_diag  */
    NULL,                    /* phy_link_change */
    phy_84756_fcmap_control_set,    /* phy_control_set */
    phy_84756_fcmap_control_get,    /* phy_control_get */
    phy_84756_fcmap_reg_read,       /* phy_reg_read */
    phy_84756_fcmap_reg_write,      /* phy_reg_write */
    NULL,                    /* phy_reg_modify */
    NULL,                    /* phy_notify */
    phy_84756_fcmap_probe,         /* pd_probe  */
    phy_84756_fcmap_ability_advert_set,  /* pd_ability_advert_set */
    phy_84756_fcmap_ability_advert_get,  /* pd_ability_advert_get */
    phy_84756_fcmap_ability_remote_get,  /* pd_ability_remote_get */
    phy_84756_fcmap_ability_local_get,   /* pd_ability_local_get  */
    phy_84756_fcmap_firmware_set,
    phy_84756_fcmap_timesync_config_set,
    phy_84756_fcmap_timesync_config_get,
    phy_84756_fcmap_timesync_control_set,
    phy_84756_fcmap_timesync_control_get,
    phy_84756_diag_ctrl             /* .pd_diag_ctrl */
};
#else /* INCLUDE_PHY_84756 */
int _phy_84756_fcmap_not_empty;
#endif /* INCLUDE_PHY_84756 */
#endif /* INCLUDE_FCMAP */
