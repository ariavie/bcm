/*
 * $Id: phy82328.c,
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
 * File:       phy82328.c
 * Purpose:    Phys Driver support for Broadcom 82328 40G phy
 * Note:      
 *
 * Specifications:

 * Repeater mode only, no retimer operation.

 * Supports the following data speeds:
 * 1.25 Gbps line rate (1 GbE data rate, 8x oversampled over 10 Gbps line rate)
 * 10.3125 Gbps line rate (10 GbE data rate)
 * 11.5 Gpbs line rate (for backplane application, proprietary data rate)
 * 4×10.3125 Gbps line rate (40 GbE data rate)
 * 
 * Supports the following line-side connections:
 * 1 GbE and 10 GbE SFP+ SR and LR optical modules
 * 40 GbE QSFP SR4 and LR4 optical modules
 * 1 GbE and 10 GbE SFP+ CR (CX1) copper cable
 * 40 GbE QSFP CR4 copper cable
 * 10 GbE KR, 11.5 Gbps line rate and 40 GbE KR4 backplanes
 *
 * Operates with the following reference clocks:
 * Single 156.25 MHz differential clock for 1.25 Gbps, 10.3125 Gpbs and 11.5 Gbps
 * line rates
 *
 * Supports autonegotiation as follows:
 * Clause 73 only for starting Clause 72 and requesting FEC 
 * No speed resolution performed
 * No Clause 73 in 11.5 Gbps line rate, only Clause 72 supported
 * Clause 72 may be enabled standalone for close systems
 * Clause 37 is not supported
 * No on-chip FEC encoding/decoding, but shall pass-through FEC-encoded data. 
 */

#if defined(INCLUDE_PHY_82328) /* INCLUDE_PHY_82328 */
 
#include <sal/types.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/phyreg.h>
 
#include <soc/phy.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>
 
#include "phydefs.h"      /* Must include before other phy related includes */
#include "phyconfig.h"    /* Must be the first phy include after phydefs.h */
#include "phyident.h"
#include "phyreg.h"
#include "phynull.h"
#include "phyxehg.h"
#include "phy82328.h"

extern unsigned char phy82328_ucode_bin[];
extern unsigned int phy82328_ucode_bin_len;
static char *dev_name_82328_a0 = "BCM82328_A0";

#define PHY82328_DEV_PMA_PMD                     1
#define PHY82328_DEV_AN                          2
#define PHY82328_ENABLE                          1
#define PHY82328_DISABLE                         0
#define PHY82328_TOTAL_WR_BYTE                   0x4000 
#define PHY82328_ID_82328                        0x82328 /* Gallardo_28nm_oct_opt */
#define PHY82328_NUM_LANES                       4
#define PHY82328_ALL_LANES                       0xf
#define PHY82328_POL_DND                         0xFFFF

#define PHY82328_SINGLE_PORT_MODE(_pc)           ((_pc)->speed_max >= 40000)

static const char *phy82328_intf_names[] = SOC_PORT_IF_NAMES_INITIALIZER;
/* interface params */
typedef struct phy82328_intf_cfg {
    int32          speed;
    soc_port_if_t  type;
} phy82328_intf_cfg_t;

typedef	enum {
    PHY82328_DATAPATH_20,
    PHY82328_DATAPATH_4_DEPTH_1,
    PHY82328_DATAPATH_4_DEPTH_2
}phy82328_datapath_t;

typedef enum {
    PHY82328_INTF_SIDE_LINE,
    PHY82328_INTF_SIDE_SYS
} phy82328_intf_side_t;

typedef struct {
    int enable;
    int	count;					/* resume only if count==0 */
} phy82328_micro_ctrl_t;

typedef struct {
    uint32              devid;
    uint32              devrev;
    int32               p2l_map[PHY82328_NUM_LANES];  /* index: physical lane, array element: */
    phy82328_intf_cfg_t line_intf;
    phy82328_intf_cfg_t sys_intf;
    uint16              pol_tx_cfg;
    uint16              pol_rx_cfg;
    int32               mod_auto_detect;			  /* module auto detect enabled */
    phy82328_datapath_t cfg_datapath;
    phy82328_datapath_t cur_datapath;
    int32               an_en;
    int32               sys_forced_cl72;
    int32               sync_init;
    phy82328_micro_ctrl_t  micro_ctrl;
} phy82328_dev_desc_t;

#define DEVID(_pc)              (((phy82328_dev_desc_t *)((_pc) + 1))->devid)
#define DEVREV(_pc)             (((phy82328_dev_desc_t *)((_pc) + 1))->devrev)
#define P2L_MAP(_pc,_ix)        (((phy82328_dev_desc_t *)((_pc) + 1))->p2l_map[(_ix)])
#define LINE_INTF(_pc)          (((phy82328_dev_desc_t *)((_pc) + 1))->line_intf)
#define SYS_INTF(_pc)           (((phy82328_dev_desc_t *)((_pc) + 1))->sys_intf)
#define POL_TX_CFG(_pc)	        (((phy82328_dev_desc_t *)((_pc) + 1))->pol_tx_cfg)
#define POL_RX_CFG(_pc)	        (((phy82328_dev_desc_t *)((_pc) + 1))->pol_rx_cfg)
#define MOD_AUTO_DETECT(_pc)    (((phy82328_dev_desc_t *)((_pc) + 1))->mod_auto_detect)
#define CFG_DATAPATH(_pc)       (((phy82328_dev_desc_t *)((_pc) + 1))->cfg_datapath)
#define CUR_DATAPATH(_pc)       (((phy82328_dev_desc_t *)((_pc) + 1))->cur_datapath)
#define AN_EN(_pc)              (((phy82328_dev_desc_t *)((_pc) + 1))->an_en)
#define SYS_FORCED_CL72(_pc)    (((phy82328_dev_desc_t *)((_pc) + 1))->sys_forced_cl72)
#define SYNC_INIT(_pc)	        (((phy82328_dev_desc_t *)((_pc) + 1))->sync_init)
#define MICRO_CTRL(_pc)	        (((phy82328_dev_desc_t *)((_pc) + 1))->micro_ctrl)

#define PHY_82328_MICRO_PAUSE(_u, _p, _s) \
    do { \
        phy_ctrl_t *_pc; \
        _pc = EXT_PHY_SW_STATE((_u), (_p)); \
        if (DEVREV(_pc) == 0x00a0) { \
            _phy_82328_micro_pause((_u), (_p), (_s)); \
        } \
    } while (0) 

#define PHY_82328_MICRO_RESUME(_u, _p)\
    do { \
        phy_ctrl_t *_pc; \
        _pc = EXT_PHY_SW_STATE((_u), (_p)); \
        if (DEVREV(_pc) == 0x00a0) { \
            _phy_82328_micro_resume((_u), (_p)); \
        } \
    } while (0) 

#define PHY_82328_REGS_SELECT(_u, _pc, _i)                                         \
    SOC_IF_ERROR_RETURN                                                            \
        (WRITE_PHY82328_MMF_PMA_PMD_REG(_u, _pc, MGT_TOP_XPMD_REGS_SEL, _i));

#define POL_CONFIG_LANE_MASK(_l)           (1 << _l)

/* QSFI_RX :: anaRxStatus :: status_sel [02:00] */
#define QSFI_RX_ANARXSTATUS_SM_SEL_MASK            0x0007

/* QSFI_RX :: anaRxStatus :: status_sel [02:00] */
#define QSFI_RX_ANARXSTATUS_PMD_RX_PMD_LOCK_MASK   0x1000     

soc_port_if_t phy82328_sys_to_port_if[] = {
    0,                                  /* PHY_SYSTEM_INTERFACE_DFLT    */
    SOC_PORT_IF_KX,                     /* PHY_SYSTEM_INTERFACE_KX */
    SOC_PORT_IF_KR,                     /* PHY_SYSTEM_INTERFACE_KR      */
    SOC_PORT_IF_SR,                     /* PHY_SYSTEM_INTERFACE_SR      */
    SOC_PORT_IF_LR,                     /* PHY_SYSTEM_INTERFACE_LR      */
    SOC_PORT_IF_CR,                     /* PHY_SYSTEM_INTERFACE_CR      */
    SOC_PORT_IF_KR4,                    /* PHY_SYSTEM_INTERFACE_KR4     */
    SOC_PORT_IF_SR4,                    /* PHY_SYSTEM_INTERFACE_SR4     */
    SOC_PORT_IF_LR4,                    /* PHY_SYSTEM_INTERFACE_LR4     */
    SOC_PORT_IF_CR4,                    /* PHY_SYSTEM_INTERFACE_CR4     */
    SOC_PORT_IF_XFI,                    /* PHY_SYSTEM_INTERFACE_XFI     */
    SOC_PORT_IF_XLAUI                   /* PHY_SYSTEM_INTERFACE_XLAUI   */
};

STATIC int32
_phy_82328_intf_datapath_update(int32 unit,  soc_port_t port);
STATIC int32
_phy_82328_channel_select(int32 unit, soc_port_t port, phy82328_intf_side_t side, int32 lane);
STATIC int32
_phy_82328_polarity_flip(int32 unit, soc_port_t port, uint16 cfg_tx_pol, uint16 cfg_rx_pol);
STATIC int32
phy_82328_control_get(int32 unit, soc_port_t port,
                     soc_phy_control_t type, uint32 *value);
STATIC int32
phy_82328_control_set(int32 unit, soc_port_t port,
                     soc_phy_control_t type, uint32 value);
STATIC void
_phy_82328_micro_pause(int unit, soc_port_t port, const char *loc);
STATIC void
_phy_82328_micro_resume(int unit, soc_port_t port);
STATIC int
_phy_82328_debug_info(int unit, soc_port_t port);

STATIC
int32 _phy_82328_modify_pma_pmd_reg (int32 unit, int32 port, phy_ctrl_t *pc, 
                               uint16 dev, uint16 address, uint8 msb_pos, 
                               uint8 lsb_pos, uint16 data)
{
    uint16 mask = 0, i =0;

    for (i = lsb_pos; i<=msb_pos; i++) {
        mask |= (1 << i);
    }
   
    data = data << lsb_pos; 
    if (dev == PHY82328_DEV_PMA_PMD) {
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, address,
                                        data, mask));
    } else { 
        SOC_IF_ERROR_RETURN
            (MODIFY_PHY82328_MMF_AN_REG(unit, pc, address,
                                        data, mask));
    }
    return SOC_E_NONE;
}

STATIC
int32 _phy_82328_set_bcast_mode(int32 unit, int32 port,
                               int16 ena_dis)
{
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (_phy_82328_modify_pma_pmd_reg (unit, port, pc, 
         PHY82328_DEV_PMA_PMD, MGT_TOP_BROADCAST_CTRL,
         MGT_TOP_PMD_USR_BROADCAST_CTRL_BROADCAST_MODE_ENABLE_SHIFT, 
         MGT_TOP_PMD_USR_BROADCAST_CTRL_BROADCAST_MODE_ENABLE_SHIFT, ena_dis));

    return SOC_E_NONE;
}

STATIC
int32 _phy_82328_soft_reset(int32 unit, int32 port)
{
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN
        (_phy_82328_modify_pma_pmd_reg (unit, port, pc, 
         PHY82328_DEV_PMA_PMD, IEEE0_BLK_PMA_PMD_PMD_CONTROL_REGISTER,
         QSFI_IEEE0_PMD_CONTROL_REGISTER_RESET_SHIFT, 
         QSFI_IEEE0_PMD_CONTROL_REGISTER_RESET_SHIFT, 1));

    return SOC_E_NONE;
}

STATIC 
int32 _phy_82328_rom_fw_dload (int32 unit, int32 port, int32 offset, 
                                uint8 *new_fw, uint32 datalen)
{
    uint32 wr_data = 0, j = 0;
    uint16 data16 = 0;
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port); 
    
    if (offset == PHYCTRL_UCODE_BCST_SETUP) {
        /* Enable Bcast */
        SOC_DEBUG_PRINT((DK_PHY, "PHY82328 Enable Bcast u:%d p:%d\n",
                    unit, port));
        SOC_IF_ERROR_RETURN (_phy_82328_set_bcast_mode(unit, port,
                          PHY82328_ENABLE));
        return SOC_E_NONE;
    } else if (offset == PHYCTRL_UCODE_BCST_uC_SETUP) {
        SOC_DEBUG_PRINT((DK_PHY, "PHY82328 uC setup u:%d p:%d\n",
                    unit, port));

        /* uController reset */
        SOC_IF_ERROR_RETURN
            (_phy_82328_modify_pma_pmd_reg (unit, port, pc, 
             PHY82328_DEV_PMA_PMD, MGT_TOP_GEN_CTRL,
             MGT_TOP_CMN_CTLR_GEN_CTRL_UCRST_SHIFT, 
             MGT_TOP_CMN_CTLR_GEN_CTRL_UCRST_SHIFT, PHY82328_ENABLE));

        SOC_IF_ERROR_RETURN(_phy_82328_soft_reset(unit, port));
        sal_usleep(3000);
        return SOC_E_NONE;
    } else if (offset == PHYCTRL_UCODE_BCST_ENABLE) {
        /* Re Enable Bcast */
        SOC_IF_ERROR_RETURN (_phy_82328_set_bcast_mode(unit, port,
                          PHY82328_ENABLE));
        return SOC_E_NONE;
    } else if (offset == PHYCTRL_UCODE_BCST_LOAD) {
        SOC_DEBUG_PRINT((DK_PHY, "PHY82328 uCode load u:%d p:%d\n",
                    unit, port));

        /*Select MDIO FOR ucontroller download */
        SOC_IF_ERROR_RETURN
         (_phy_82328_modify_pma_pmd_reg (unit, port, pc, 
          PHY82328_DEV_PMA_PMD, MGT_TOP_SPA_CONTROL_STATUS_REGISTER,
          MGT_TOP_PMD_USR_SPA_CONTROL_STATUS_REGISTER_SPI_PORT_USED_SHIFT, 
          MGT_TOP_PMD_USR_SPA_CONTROL_STATUS_REGISTER_SPI_PORT_USED_SHIFT, 
          PHY82328_DISABLE));
        
        /* Clear Download Done */
        SOC_IF_ERROR_RETURN
         (_phy_82328_modify_pma_pmd_reg (unit, port, pc, 
          PHY82328_DEV_PMA_PMD, MGT_TOP_SPA_CONTROL_STATUS_REGISTER,
          MGT_TOP_PMD_USR_SPA_CONTROL_STATUS_REGISTER_SPI_DWLD_DONE_SHIFT, 
          MGT_TOP_PMD_USR_SPA_CONTROL_STATUS_REGISTER_SPI_DWLD_DONE_SHIFT, 
          PHY82328_DISABLE));

        SOC_IF_ERROR_RETURN
         (_phy_82328_modify_pma_pmd_reg (unit, port, pc, 
         PHY82328_DEV_PMA_PMD, MGT_TOP_SPA_CONTROL_STATUS_REGISTER,
         MGT_TOP_PMD_USR_SPA_CONTROL_STATUS_REGISTER_SPI_BOOT_SHIFT, 
         MGT_TOP_PMD_USR_SPA_CONTROL_STATUS_REGISTER_SPI_BOOT_SHIFT, 
         PHY82328_ENABLE));

   
        /* Set Dload size to 32K */
        SOC_IF_ERROR_RETURN
         (_phy_82328_modify_pma_pmd_reg (unit, port, pc, 
          PHY82328_DEV_PMA_PMD, MGT_TOP_MISC_CTRL,
          MGT_TOP_CMN_CTLR_MISC_CTRL_SPI_DOWNLOAD_SIZE_SHIFT,
          MGT_TOP_CMN_CTLR_MISC_CTRL_SPI_DOWNLOAD_SIZE_SHIFT, 
          PHY82328_ENABLE));

        /* uController reset */
        SOC_IF_ERROR_RETURN
         (_phy_82328_modify_pma_pmd_reg (unit, port, pc, 
          PHY82328_DEV_PMA_PMD, MGT_TOP_GEN_CTRL,
          MGT_TOP_CMN_CTLR_GEN_CTRL_UCRST_SHIFT, 
          MGT_TOP_CMN_CTLR_GEN_CTRL_UCRST_SHIFT, PHY82328_DISABLE));

        sal_usleep(3000);    /*wait for 3ms to initialize the RAM*/

        /*Write Address*/
        SOC_IF_ERROR_RETURN
          (WRITE_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_MSG_IN, 0x8000));

        /* Make sure Address word is read by the Micro*/
        sal_usleep(10);

        wr_data = (datalen/2);            /*16k ROM*/
        SOC_IF_ERROR_RETURN
            (WRITE_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_MSG_IN, wr_data));
        sal_usleep(10);
   
        /* Fill in the SRAM */
        wr_data = (datalen - 1);
        for (j = 0; j < wr_data; j+=2) {
            /*Make sure the word is read by the Micro */
            sal_usleep(10);        
            data16 = (new_fw[j] << 8) | new_fw[j+1];
            SOC_IF_ERROR_RETURN
            (WRITE_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_MSG_IN, data16));
        }
        return SOC_E_NONE;
    } else if(offset== PHYCTRL_UCODE_BCST_END) {
        sal_usleep(20);
        SOC_IF_ERROR_RETURN
            (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_MSG_OUT, &data16));
    
        SOC_DEBUG_PRINT((DK_PHY, "PHY82328 FW load done u:%d p:%d Read Done:0x%x\n",
                    unit, port, data16));

        sal_usleep(4000);
        SOC_IF_ERROR_RETURN
            (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_GP_REG_4, &data16));

        SOC_DEBUG_PRINT((DK_PHY, "PHY82328 FW load done u:%d p:%d chksum:0x%x\n",
                    unit, port, data16));

        if (data16 != 0x600D) {
            SOC_DEBUG_PRINT((DK_ERR, "Invalid Checksum:0x%x\n", data16));
            return SOC_E_INTERNAL;
        }
        /*Check the register to read*/
        SOC_IF_ERROR_RETURN
         (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, 0xc161, &data16)); 

        soc_cm_print("PHY82328 u:%d p:%d Rom Version(Reg:0xc161):0x%x\n", 
                unit, port, (data16 & 0xFF));

        /* Disable Bcast */
        SOC_IF_ERROR_RETURN (_phy_82328_set_bcast_mode(unit, port,
                          PHY82328_DISABLE));

        data16 = (PHY82328_SINGLE_PORT_MODE(pc)) ?
                 MGT_TOP_CMN_CTLR_SINGLE_PMD_CTRL_SINGLE_PMD_MODE_MASK : 0;
        SOC_IF_ERROR_RETURN
           (MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
           MGT_TOP_SINGLE_PMD_CTRL, data16,
           MGT_TOP_CMN_CTLR_SINGLE_PMD_CTRL_SINGLE_PMD_MODE_MASK));

        /* Make sure micro completes its initialization */
        sal_udelay(5000);
    } else {
        SOC_DEBUG_PRINT((DK_ERR,
                    "u=%d p=%d PHY82328 firmware_bcst: invalid cmd 0x%x\n",
                      unit, port, offset));
    }

    return SOC_E_NONE;
}

STATIC int32
_phy_82328_intf_is_single_port(soc_port_if_t intf_type)
{
	int32 rv;

    switch (intf_type) {
        case SOC_PORT_IF_CR4:
        case SOC_PORT_IF_KR4:
        case SOC_PORT_IF_XLAUI:
        case SOC_PORT_IF_LR4:
        case SOC_PORT_IF_SR4:
	        rv = TRUE;
		break;
        default:
		    rv = FALSE;
    }
    return rv;
}

STATIC int32
_phy_82328_intf_is_quad_port(soc_port_if_t intf_type)
{
	int32 rv;

    switch (intf_type) {
        case SOC_PORT_IF_SR:
        case SOC_PORT_IF_CR:
        case SOC_PORT_IF_KR:
        case SOC_PORT_IF_XFI:
        case SOC_PORT_IF_SFI:
        case SOC_PORT_IF_LR:
        case SOC_PORT_IF_ZR:
        case SOC_PORT_IF_KX:
        case SOC_PORT_IF_GMII:
        case SOC_PORT_IF_SGMII:
            rv = TRUE;
		break;
        default:
		    rv = FALSE;
    }
    return rv;
}

STATIC int32
_phy_82328_intf_line_sys_params_get(int32 unit, soc_port_t port)
{
    phy82328_intf_cfg_t *line_intf;
    phy82328_intf_cfg_t *sys_intf;
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);

    line_intf = &(LINE_INTF(pc));
    sys_intf = &(SYS_INTF(pc));

	/*
	 * system side interface was specified so make sure it's compatible with
	 * line side speed. If not, overwrite with default compatible interface
	 */
    if (line_intf->speed == 40000) {
        sys_intf->speed = 40000;
        line_intf->type = SOC_PORT_IF_SR4;
        if (!(_phy_82328_intf_is_single_port(sys_intf->type))) {
            /* system side interface is not compatible so overwrite with default */
            SOC_DEBUG_PRINT((DK_ERR, "PHY82328 incompatible 40G system side interface, "
						 "using default: u=%d p=%d\n", unit, port));
            sys_intf->type = SOC_PORT_IF_XLAUI;
        }
    } else { /* 10G/1G */
        if (_phy_82328_intf_is_quad_port(sys_intf->type)) {
            if (sys_intf->type == SOC_PORT_IF_KX) {
                line_intf->speed = 1000;
                line_intf->type = SOC_PORT_IF_GMII;
                sys_intf->speed = 1000;
			} else {
                /* 10G system side so fix line side to be 10G compatible */
                line_intf->speed = 10000;
                line_intf->type = SOC_PORT_IF_SR;
                sys_intf->speed = 10000;
            }
        } else {
            /* system side interface is not compatible so overwrite with default */
            SOC_DEBUG_PRINT((DK_ERR, "PHY82328 incompatible 10G/1G system side interface, "
						 "using default: u=%d p=%d\n", unit, port));
            /* default to 10G interface/speed on both sides */
            line_intf->speed = 10000;
            line_intf->type = SOC_PORT_IF_SR;
            sys_intf->speed = 10000;
            sys_intf->type =  SOC_PORT_IF_XFI;
        }
    }
    return SOC_E_NONE;
}

STATIC int32
_phy_82328_config_update(int32 unit, soc_port_t port)
{
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
    
    /* Change polarity got from config */
    return _phy_82328_polarity_flip(unit, port, POL_TX_CFG(pc), POL_RX_CFG(pc));
}


STATIC int32
_phy_82328_init_pass2(int32 unit, soc_port_t port)
{
    phy_ctrl_t *pc;
    phy82328_intf_cfg_t *line_intf, *sys_intf;
    phy82328_micro_ctrl_t *micro_ctrl;
    uint16 if_sys_idx = 0;
    uint16  chip_rev;

    SOC_DEBUG_PRINT((DK_PHY, "PHY82328 init pass2: u=%d p=%d\n", unit, port));
    pc = EXT_PHY_SW_STATE(unit, port);

	/*
	 * - interfaces are KR4/KR/KX/XAUI/XFI/SR/LR/CR/SR4/LR4/CR4
	 * - Gallardo is capable of being configured symmetrically and can support all 
	 *   interfaces on both sides
	 * - system side interfaces cannot be changed dynamically
	 *   - configured in Gallardo via config variables
	 * - line side mode can be changed dynamically
	 *   - configured specifically with interface set and/or by matching speed
	 *   - automatically when module autodetect is enabled
	 *
	 *    port mode            system side                   line side
	 * ----------------  ------------------------  --------------------------------
	 * 40G(singleport)   dflt:XLAUI or configvar   dflt:XLAUI  or user set if/speed
	 * 1G/10G(quadport)  dflt:XFI or configvar     dflt:SFI    or user set if/speed
	 */
	
    line_intf = &(LINE_INTF(pc));
    sys_intf  = &(SYS_INTF(pc));
    micro_ctrl = &(MICRO_CTRL(pc));

    sal_memset(line_intf, 0, sizeof(phy82328_intf_cfg_t));
    sal_memset(sys_intf,  0, sizeof(phy82328_intf_cfg_t));
    sal_memset(micro_ctrl,  0, sizeof(phy82328_micro_ctrl_t));

    AN_EN(pc) = 0;
    SYNC_INIT(pc) = 1;
    micro_ctrl->enable = 1;
    micro_ctrl->count = 0;

    SYS_FORCED_CL72(pc) = soc_property_port_get(unit, port, spn_PORT_INIT_CL72, 0);

    SOC_IF_ERROR_RETURN(
        READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
									  MGT_TOP_CHIP_REV_REGISTER,
									  &chip_rev));
    DEVREV(pc) = chip_rev;
    pc->flags |= PHYCTRL_INIT_DONE;
	/* Get configured system interface */
    if_sys_idx = soc_property_port_get(unit, port, spn_PHY_SYS_INTERFACE, 0);
    if (if_sys_idx >= (sizeof(phy82328_sys_to_port_if) / sizeof(soc_port_if_t))) {
        SOC_DEBUG_PRINT((DK_ERR, "PHY82328 invalid system side interface: u=%d p=%d intf=%d\n"
						 "Using default interface\n",
						 unit, port, if_sys_idx));
        if_sys_idx = 0;
	}
    sys_intf->type = phy82328_sys_to_port_if[if_sys_idx];
    line_intf->speed = PHY82328_SINGLE_PORT_MODE(pc) ? 40000 : 10000;
    if (sys_intf->type == 0) {
		/* use defaults based on current line side */
        sys_intf->speed = line_intf->speed;
        line_intf->type = (line_intf->speed == 40000) ? SOC_PORT_IF_SR4 : SOC_PORT_IF_SR;
        sys_intf->type =  (line_intf->speed == 40000) ? SOC_PORT_IF_XLAUI : SOC_PORT_IF_XFI;
        soc_cm_print("default sys and line intf are used\n");
    } else {
        SOC_IF_ERROR_RETURN(_phy_82328_intf_line_sys_params_get(unit, port));
        soc_cm_print("After updating intf based on sys config\n");
        soc_cm_print("SYS Intf:%s speed:%d\n", phy82328_intf_names[sys_intf->type],sys_intf->speed);
        soc_cm_print("Line intf:%s speed:%d\n",phy82328_intf_names[line_intf->type],line_intf->speed);
    }
    CFG_DATAPATH(pc) = soc_property_port_get(unit, port, "82328_DATAPATH", PHY82328_DATAPATH_4_DEPTH_1);
    CUR_DATAPATH(pc) = CFG_DATAPATH(pc);
	
    /* Polarity configuration is applied every time interface is configured */
    POL_TX_CFG(pc) = soc_property_port_get(unit, port, spn_PHY_TX_POLARITY_FLIP, PHY82328_POL_DND);
    POL_RX_CFG(pc) = soc_property_port_get(unit, port, spn_PHY_RX_POLARITY_FLIP, PHY82328_POL_DND);

    PHY_82328_MICRO_PAUSE(unit, port, "polarity config in Init");

    /* Push configuration to the device */
    SOC_IF_ERROR_RETURN(_phy_82328_config_update(unit, port));
    
    PHY_82328_MICRO_RESUME(unit, port);
	
    return SOC_E_NONE;
}

/*
 * Check if the core has been initialized. This is done by
 * looking at all ports which belong to the same core and see
 * if any one of them have completed initialization.
 * The primary port is used to determine whether the port is
 * on the same core as all ports in a core share the primary port.
 */
STATIC int32
_phy_82328_core_init_done(int32 unit, struct phy_driver_s *pd, int32 primary_port)
{
    int32 rv;
    soc_port_t port;
    phy_ctrl_t *pc;
    uint32 core_primary_port;

    PBMP_ALL_ITER(unit, port) {
        pc = EXT_PHY_SW_STATE(unit, port);
        if (pc == NULL) {
            continue;
        }
        /* Make sure this port has a 82328 driver */
        if (pc->pd != pd) {
            continue;
        }
        rv = phy_82328_control_get(unit, port, SOC_PHY_CONTROL_PORT_PRIMARY,
                                   &core_primary_port);
        if (rv != SOC_E_NONE) {
            continue;
        }
        if ((primary_port == core_primary_port) && (pc->flags & PHYCTRL_INIT_DONE)) {
            return TRUE;
		}
	}
    return FALSE;
}


STATIC int32
_phy_82328_init_pass1(int32 unit, soc_port_t port)
{
    phy_ctrl_t			*pc;
    int32               offset;
    soc_port_t			primary_port;
		
    pc = EXT_PHY_SW_STATE(unit, port);
	
    SOC_DEBUG_PRINT((DK_PHY, "PHY82328 init pass1: u=%d p=%d\n", unit, port));

    /* Set the primary port & offset */
    if (soc_phy_primary_and_offset_get(unit, port, &primary_port, &offset) != SOC_E_NONE) {
		/* 
		 * Derive primary and offset 
		 * There is an assumption lane 0 on first core on the the MDIO
		 * bus is a primary port
		 */
        if (PHY82328_SINGLE_PORT_MODE(pc)) {
            primary_port = port;
            offset = 0;
        } else {
            primary_port = port - (pc->phy_id & 0x3);
            offset = pc->phy_id & 0x3;
        }
    }
    /* set the default values that are valid for many boards */
    SOC_IF_ERROR_RETURN(phy_82328_control_set(unit, port, SOC_PHY_CONTROL_PORT_PRIMARY,
                                              primary_port));
	
    SOC_IF_ERROR_RETURN(phy_82328_control_set(unit, port,SOC_PHY_CONTROL_PORT_OFFSET,
                                              offset));
    /*
     * Since there is a single micro and therefore a single copy of the firmware
     * shared among the 4 channels in a core, do not download firmware if in quad
     * channel and the other channels are active.
     */
     if (PHY82328_SINGLE_PORT_MODE(pc) ||
        (!PHY82328_SINGLE_PORT_MODE(pc) && 
        (!_phy_82328_core_init_done(unit, pc->pd, primary_port)))) {
        SOC_DEBUG_PRINT((DK_PHY, "PHY82328 Bcast Enabled for Port:%d", port));
        pc->flags |= PHYCTRL_MDIO_BCST;
	}
	
	/* indicate second pass of init is needed */
    PHYCTRL_INIT_STATE_SET(pc,PHYCTRL_INIT_STATE_PASS2);

    return SOC_E_NONE;
}

STATIC int32
_phy_82328_chip_id_get(int32 unit, soc_port_t port, phy_ctrl_t *pc, uint32 *chip_id)
{
	int32 rv = SOC_E_NONE;

    uint16 chip_id_lsb = 0, chip_id_msb = 0;

    *chip_id = 0;

    SOC_IF_ERROR_RETURN(
            READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_CHIP_ID0_REGISTER, &chip_id_lsb));
    SOC_IF_ERROR_RETURN(
            READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_CHIP_ID1_REGISTER, &chip_id_msb));

    if (chip_id_msb == 0x8) {
        if (chip_id_lsb == 0x2328) {
            *chip_id = PHY82328_ID_82328;
        } else {
            SOC_DEBUG_PRINT((DK_ERR, "PHY82328  bad chip id: u=%d p=%d chipid %x%x\n",
                              unit, port, chip_id_msb, chip_id_lsb));
            rv = SOC_E_BADID;
        }
    }
    return rv;
}

STATIC int32
_phy_82328_config_devid(int32 unit,soc_port_t port, phy_ctrl_t *pc, uint32 *devid)
{
    if (soc_property_port_get(unit, port, "phy_82328", FALSE)) {
        return *devid = PHY82328_ID_82328;
    }

    return _phy_82328_chip_id_get(unit, port, pc, devid);
}

/* Function:
 *    phy_82328_init
 * Purpose:
 *    Initialize 82328 phys
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #.
 * Returns:
 *    SOC_E_NONE
 */

STATIC
int32 phy_82328_init(int32 unit, soc_port_t port)
{
    int32					ix;
    uint32				devid;
    phy_ctrl_t			*pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    SOC_DEBUG_PRINT((DK_PHY, "PHY82328 init: u=%d p=%d state=%d\n", 
					 unit, port,PHYCTRL_INIT_STATE(pc)));

    PHY_82328_REGS_SELECT (unit, pc, PHY82328_INTF_SIDE_LINE);

    if ((PHYCTRL_INIT_STATE(pc) == PHYCTRL_INIT_STATE_PASS1) ||
        (PHYCTRL_INIT_STATE(pc) == PHYCTRL_INIT_STATE_DEFAULT)) {

        /* Read the chip id to identify the device */
        SOC_IF_ERROR_RETURN(_phy_82328_config_devid(unit, port, pc, &devid));

        /* Save it in the device description structure for future use */  
        DEVID(pc) = devid;

        /* initialize default p2l map */
        for (ix = 0; ix < PHY82328_NUM_LANES; ix++) {
            P2L_MAP(pc,ix) = ix;
        }
        PHY_FLAGS_SET(unit, port,  PHY_FLAGS_FIBER | PHY_FLAGS_C45 | PHY_FLAGS_REPEATER);
        if (PHYCTRL_INIT_STATE(pc) == PHYCTRL_INIT_STATE_PASS1) {
            SOC_IF_ERROR_RETURN(_phy_82328_init_pass1(unit, port));
            return SOC_E_NONE;
        }
    }
    if ((PHYCTRL_INIT_STATE(pc) == PHYCTRL_INIT_STATE_PASS2) ||
        (PHYCTRL_INIT_STATE(pc) == PHYCTRL_INIT_STATE_DEFAULT)) {
        return _phy_82328_init_pass2(unit, port);
    }
    return SOC_E_NONE;
}

/* Return contents of ANARXSTATUS */
STATIC int32
_phy_82328_intf_link_get(int32 unit, soc_port_t port, int32 *link)
{
    int32 lane=0, max_lane=0;
    uint16 data16=0;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
 
    *link = 0;
    max_lane = PHY82328_SINGLE_PORT_MODE(pc) ? PHY82328_NUM_LANES:1;
    for (lane = 0; lane < max_lane; lane++) {
        if (PHY82328_SINGLE_PORT_MODE(pc)) {
            /* Select the lane on the line side */
            SOC_IF_ERROR_RETURN(
               _phy_82328_channel_select(unit, port, PHY82328_INTF_SIDE_LINE, lane));
        }
        
        /*anaRxStatusSel 1.C0B1.[2:0] 0*/
        SOC_IF_ERROR_RETURN(
            MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, RX_ANARXCONTROL,
                         0x0, QSFI_RX_ANARXSTATUS_SM_SEL_MASK));
        
        /*anaRxStatusSel 1.C0B0.12 1*/
        SOC_IF_ERROR_RETURN(
            READ_PHY82328_MMF_PMA_PMD_REG (unit, pc, RX_ANARXSTATUS, &data16));
        
        /* Check if rxSeq Done in any lane */
        if ((data16 & QSFI_RX_ANARXSTATUS_PMD_RX_PMD_LOCK_MASK) ==
             QSFI_RX_ANARXSTATUS_PMD_RX_PMD_LOCK_MASK) {
            *link = 1;
            break;
        }
    }
    /* Restore to default single port register access */
    if (PHY82328_SINGLE_PORT_MODE(pc)) {
        SOC_IF_ERROR_RETURN
            (_phy_82328_channel_select(unit, port, PHY82328_INTF_SIDE_LINE,
											  PHY82328_ALL_LANES));
    }
    return SOC_E_NONE;
}

/*
 *    phy_82328_link_get
 * Purpose:
 *    Get layer2 connection status.
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #.
 *      link - address of memory to store link up/down state.
 * Returns:
 *    SOC_E_NONE
 */
 
STATIC int32
phy_82328_link_get(int32 unit, soc_port_t port, int32 *link)
{

    phy_ctrl_t  *int_pc ;
    int32 rv = SOC_E_NONE;
    int32 int_phy_link = 0;
	
    if (link == NULL) {
        return SOC_E_PARAM;
    }
    if (PHY_FLAGS_TST(unit, port, PHY_FLAGS_DISABLE)) {
        *link = FALSE;
        return SOC_E_NONE;
    }
    int_pc = INT_PHY_SW_STATE(unit, port);   
    
    if (int_pc != NULL) {
        SOC_IF_ERROR_RETURN(PHY_LINK_GET(int_pc->pd, unit, port, &int_phy_link));
        *link = int_phy_link;
    } else {
       *link = 0;
    }

    
    rv = _phy_82328_intf_link_get( unit, port, link);
    soc_cm_debug(DK_PHY | DK_VERBOSE,
         "phy_82328_link_get: u=%d port%d: link:%s\n",
         unit, port, *link ? "Up": "Down");
    return rv;
}

/*
 * Function:
 *    phy_82328_enable_set
 * Purpose:
 *    Enable/Disable phy
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #.
 *      enable - on/off state to set
 * Returns:
 *    SOC_E_NONE
 */
 
STATIC int32
phy_82328_enable_set(int32 unit, soc_port_t port, int32 enable)
{
    uint16 chan_data, chan_mask;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
    phy_ctrl_t  *int_pc = INT_PHY_SW_STATE(unit, port);

	/* Manually power channel up/down */
    chan_data = enable ? 0 :
        (1 << MGT_TOP_PMD_USR_OPTICAL_CONFIGURATION_MAN_TXON_EN_SHIFT) |
        (1 << MGT_TOP_PMD_USR_OPTICAL_CONFIGURATION_TXOFFT_SHIFT);
    chan_mask = MGT_TOP_PMD_USR_OPTICAL_CONFIGURATION_MAN_TXON_EN_MASK |
        MGT_TOP_PMD_USR_OPTICAL_CONFIGURATION_TXOFFT_MASK;

    SOC_IF_ERROR_RETURN(
        MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_OPTICAL_CONFIGURATION,
                                       chan_data, chan_mask));
    if (enable) {
        SOC_IF_ERROR_RETURN(PHY_ENABLE_SET(int_pc->pd, unit, port, 1));
        /* Let the micro settle down after powering back up */
        sal_usleep(100);
        PHY_FLAGS_CLR(unit, port, PHY_FLAGS_DISABLE);
    } else {
        SOC_IF_ERROR_RETURN(PHY_ENABLE_SET(int_pc->pd, unit, port, 0));
        PHY_FLAGS_SET(unit, port, PHY_FLAGS_DISABLE);
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *    phy_82328_regbit_set_wait_check
 * Purpose:
 *   Waits for a reg. bit to be set. 
 *	 Spins until bit is set or time out. 
 *   The time out is programmable.
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #.
 *    reg  - register to test
 *	  bit_num - register bit to be tested
 *    bitset - to test set/clear
 *    timeout - spin time out
 * Returns:
 *    SOC_E_NONE if no errors, SOC_E_ERROR else.
 */
STATIC
int32 phy_82328_regbit_set_wait_check(int32 unit ,int32 port,
                  int32 reg,              /* register to check */
                  int32 bit_num,          /* bit number to check */
                  int32 bitset,           /* check bit set or bit clear */
                  int32 timeout)          /* max wait time to check */
{                      
    uint16         data16 = 0;
    soc_timeout_t  to;
    int32            rv;
    int32            cond;
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);

    /* override min_polls with iteration count for DV. */
    soc_timeout_init(&to, timeout, 0);
    do {     
        rv = READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, reg, &data16);
        cond = (bitset  && (data16 & bit_num)) ||
               (!bitset && !(data16 & bit_num));
        if (cond || (rv < 0)) {
            break;
        }
    } while (!soc_timeout_check(&to));

    /* Do one final check potentially after timeout as required by soc_timeout */
    if (rv >= 0) {
        rv = READ_PHY82328_MMF_PMA_PMD_REG(unit,pc,reg,&data16);
        cond = (bitset  && (data16 & bit_num)) ||
               (!bitset && !(data16 & bit_num));
    }
    return (cond? SOC_E_NONE: SOC_E_TIMEOUT);
}

STATIC phy82328_intf_side_t
_phy_82328_intf_side_regs_get(int32 unit, soc_port_t port)
{
    int32 rv=0;
    uint16 data=0;
    phy82328_intf_side_t side = PHY82328_INTF_SIDE_LINE;
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);                
 
    rv = READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_XPMD_REGS_SEL, &data);
    if (rv == SOC_E_NONE) {
        side = (data & MGT_TOP_PMD_USR_XPMD_REGS_SEL_SELECT_SYS_REGISTERS_MASK) ? 
                PHY82328_INTF_SIDE_SYS : PHY82328_INTF_SIDE_LINE;
    }
    return side;
}

STATIC int32
_phy_82328_intf_update(int32 unit, soc_port_t port, uint16 reg_data, uint16 reg_mask)
{
    uint16 ucode_csr_bef;
    phy82328_intf_side_t saved_side;
    int32 rv = SOC_E_NONE;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
 
    /* Access line side registers */
    saved_side = _phy_82328_intf_side_regs_get(unit, port);
    if (saved_side == PHY82328_INTF_SIDE_SYS) {                
        PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_LINE);
    }
 
    /* Make sure ucode has acked */
    rv = READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_GP_REGISTER_3, &ucode_csr_bef);
    if (SOC_FAILURE(rv)) {
        SOC_DEBUG_PRINT((DK_ERR, "82328 failed reading ucode csr: u=%d p=%d err=%d\n",
                               unit, port, rv));
        goto fail;
    }
    if ((ucode_csr_bef & MGT_TOP_PMD_USR_GP_REGISTER_3_FINISH_CHANGE_MASK) ==
                MGT_TOP_PMD_USR_GP_REGISTER_3_FINISH_CHANGE_MASK) {

        /* Cmd active and ucode acked, so let ucode know drv saw ack */
        rv = MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_GP_REGISTER_1,
                       0x0,  MGT_TOP_PMD_USR_GP_REGISTER_1_FINISH_CHANGE_MASK);
        if (SOC_FAILURE(rv)) {
            SOC_DEBUG_PRINT((DK_ERR, "82328 failed clearing ack: u=%d p=%d err=%d\n",
                            unit, port, rv));
            goto fail;
        }
				
        /*Spin for finish Flag to Clear*/
        rv = phy_82328_regbit_set_wait_check (unit,port,MGT_TOP_GP_REGISTER_3,
               MGT_TOP_PMD_USR_GP_REGISTER_3_FINISH_CHANGE_MASK,0,1000000);
        if (rv != SOC_E_NONE) {
            goto fail;
        }
    }
    SOC_DEBUG_PRINT((DK_PHY, "82328 intf update register: u=%d, p=%d, 1.%04x=%04x/%04x ucode_csr=%04x\n",
                     unit, port, MGT_TOP_GP_REGISTER_1, reg_data, reg_mask, ucode_csr_bef));
 
    /* Send command */
    rv = MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_GP_REGISTER_1,
                                      reg_data, reg_mask);
    if (SOC_FAILURE(rv)) {
        SOC_DEBUG_PRINT(
            (DK_ERR, "82328 failed sending command to ucode: u=%d p=%d err=%d\n",
            unit, port, rv));
        goto fail;
    }
 
    /* Handshake with microcode by waiting for ack before moving on */
    rv = phy_82328_regbit_set_wait_check (unit, port, MGT_TOP_GP_REGISTER_3,
                 MGT_TOP_PMD_USR_GP_REGISTER_3_FINISH_CHANGE_MASK, 1, 1000000);
				
    if (rv !=SOC_E_NONE) {
        goto fail;
    }

    /* Cmd active and ucode acked - let ucode know we saw ack */
    rv = MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_GP_REGISTER_1,
            0,  MGT_TOP_PMD_USR_GP_REGISTER_1_FINISH_CHANGE_MASK);
    if (rv != SOC_E_NONE) {
        goto fail;
    }
 
fail:
    if (saved_side == PHY82328_INTF_SIDE_SYS) {                
        PHY_82328_REGS_SELECT (unit, pc, PHY82328_INTF_SIDE_SYS);
    }
    return rv;
}
						
STATIC int32
_phy_82328_intf_datapath_reg_get(int32 unit, soc_port_t port, phy82328_datapath_t datapath,
                                                                 uint16 *reg_data, uint16 *reg_mask)
{
    *reg_data = 0;
    *reg_mask = 0;
 
    switch (datapath) {
        case PHY82328_DATAPATH_20:
            *reg_data = 0;
        break;
        case PHY82328_DATAPATH_4_DEPTH_1:
            *reg_data = (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_ENABLE_ULTRA_LOW_LATENCY_DATAPATH_SHIFT);
        break;
        case PHY82328_DATAPATH_4_DEPTH_2:
            *reg_data =	(1 << MGT_TOP_PMD_USR_GP_REGISTER_1_ENABLE_ULTRA_LOW_LATENCY_DATAPATH_SHIFT |
                         1 << MGT_TOP_PMD_USR_GP_REGISTER_1_ULL_DATAPATH_LATENCY_SHIFT);
        break;
        default :
            SOC_DEBUG_PRINT((DK_ERR, "82328 invalid datapath: u=%d p=%d datapath=%d\n",
                                      unit, port, datapath));
            return SOC_E_CONFIG;
    }
    *reg_data |= (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_FINISH_CHANGE_SHIFT);
    *reg_mask = (MGT_TOP_PMD_USR_GP_REGISTER_1_ENABLE_ULTRA_LOW_LATENCY_DATAPATH_MASK |
	             MGT_TOP_PMD_USR_GP_REGISTER_1_ULL_DATAPATH_LATENCY_MASK |
                 MGT_TOP_PMD_USR_GP_REGISTER_1_FINISH_CHANGE_MASK);
				 
    SOC_DEBUG_PRINT((DK_PHY, "82328 datapath set register: u=%d, p=%d, reg=%04x/%04x, datapath=%d\n",
                     unit, port, *reg_data, *reg_mask, datapath));
    return SOC_E_NONE;
}

/*
 * Return the settings in gp_reg_1/gp_register_1 for the inteface speed
 * Register data and mask are returned to modify the register.
 */ 
STATIC int32
_phy_82328_intf_speed_reg_get(int32 unit, soc_port_t port, int32 speed, uint16 *reg_data, uint16 *reg_mask)
{
    *reg_data = 0;
    *reg_mask = 0;		
    switch (speed) {
        case 100000: /*1.C841.3 Not yet implemented */
	       *reg_data = (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_100G_SHIFT);
        break;
        case 42000:/*1.C841.[2-0]*/
           *reg_data = ((1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_40G_SHIFT) | \
                        (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_10G_SHIFT) | \
                        (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_1G_SHIFT));
        break;
        case 40000:/*1.C841.2*/
            *reg_data = (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_40G_SHIFT);
        break;
		case 11000: /*1.C841.0 & 1.C841.2*/
            *reg_data = ((1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_40G_SHIFT)| \
                         (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_1G_SHIFT));
        break;
        case 10000: /*1.C841.1*/
            *reg_data = (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_10G_SHIFT);
        break;
        case 1000:/*1.C841.0*/
		case 100:
		case 10:
            *reg_data = (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_1G_SHIFT);
        break;
        default :
            SOC_DEBUG_PRINT((DK_ERR, "82328 invalid line speed %d: u=%d p=%d\n",
                                      speed, unit, port));
            return SOC_E_CONFIG;
    }
    *reg_data |= (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_FINISH_CHANGE_SHIFT);
    *reg_mask |= MGT_TOP_PMD_USR_GP_REGISTER_1_FINISH_CHANGE_MASK;
    *reg_mask |= (MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_1G_MASK |
                  MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_10G_MASK|
                  MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_40G_MASK |
                  MGT_TOP_PMD_USR_GP_REGISTER_1_SPEED_100G_MASK);
 
    SOC_DEBUG_PRINT((DK_PHY, "82328 speed set register: u=%d, p=%d, reg=%04x/%04x), speed=%d\n",
                            unit, port, *reg_data, *reg_mask, speed));
    return SOC_E_NONE;
}

STATIC int32
_phy_82328_intf_type_reg_get(int32 unit, soc_port_t port, soc_port_if_t intf_type,
                            phy82328_intf_side_t side, uint16 *reg_data, uint16 *reg_mask)
{
    uint16 data = 0;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
 
    assert((side == PHY82328_INTF_SIDE_LINE) || (side == PHY82328_INTF_SIDE_SYS));
    *reg_data = 0;
    *reg_mask = 0;
 
    switch (intf_type) {			
        case SOC_PORT_IF_KX:
        case SOC_PORT_IF_KR:
        case SOC_PORT_IF_KR4:
            data = (side == PHY82328_INTF_SIDE_SYS) ?
                   (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_TYPE_SHIFT):
                   (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_LINE_TYPE_SHIFT);
            *reg_mask |= (side == PHY82328_INTF_SIDE_SYS) ?
                (MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_TYPE_MASK):
                (MGT_TOP_PMD_USR_GP_REGISTER_1_LINE_TYPE_MASK);
        break;
        case SOC_PORT_IF_SR:
        case SOC_PORT_IF_SFI:/*Nothing but 10G, but not in list*/
        case SOC_PORT_IF_GMII:
        case SOC_PORT_IF_SGMII:
            data = 0;
        break;
        case SOC_PORT_IF_CR4: 
            /* If CR4 line side and autoneg enabled, then CR4, otherwise, XFI/XLAUI_DFE*/
            if (side == PHY82328_INTF_SIDE_LINE) {
                data = (AN_EN(pc)) ? (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_LINE_CU_TYPE_SHIFT) :
                          ((1 << MGT_TOP_PMD_USR_GP_REGISTER_1_LINE_FORCED_CL72_MODE_SHIFT) |
                           (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_LINE_CU_TYPE_SHIFT));
            } else {
                /*System side set to XLAUI_DFE */
                /*CR4 System side is unusual taken  
                  CL72 not supported for Sys Side only DFE*/
                data = (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_FORCED_CL72_MODE_SHIFT) |
                       (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_CU_TYPE_SHIFT);
            }
        break;			
        case SOC_PORT_IF_CR:
            /*CR System side is unusual taken  
              CL72 not supported for Sys Side only DFE*/
            data = (side == PHY82328_INTF_SIDE_SYS) ?
                   ((1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_FORCED_CL72_MODE_SHIFT) |
                   (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_CU_TYPE_SHIFT)) :
                   (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_LINE_CU_TYPE_SHIFT);
        break;
        case SOC_PORT_IF_XFI:
        case SOC_PORT_IF_XLAUI:
        case SOC_PORT_IF_SR4: 
            /*SR4 System Side Implicitly taken as 
              SR4-XAUI/XFI DFE & SR4-Forced CL72 [not supported] */
            data = (side == PHY82328_INTF_SIDE_SYS) ?
                   ((1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_LR_MODE_SHIFT) |
                   (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_CU_TYPE_SHIFT)) :
                    0;
        break;
			
        /*LR/LR4 System Side Implicitly taken as 
          LR4-XAUI/XFI DFE & LR4-Forced CL72 [not supported] 
          Setting LR for system side not going to make effect 
          (will be set by xfi/xlaui) but still doing it*/
        case SOC_PORT_IF_LR:
        case SOC_PORT_IF_LR4:
            data = (side == PHY82328_INTF_SIDE_SYS) ?
                   (1<< MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_LR_MODE_SHIFT)|
                   (1<<MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_CU_TYPE_SHIFT):
                   (1<< MGT_TOP_PMD_USR_GP_REGISTER_1_LINE_LR_MODE_SHIFT);
        break;
        default:
            SOC_DEBUG_PRINT((DK_ERR, "82328 Unsupported interface: u=%d p=%d\n", unit, port));			
            return SOC_E_UNAVAIL;
    }
    *reg_data |= (data | (1 << MGT_TOP_PMD_USR_GP_REGISTER_1_FINISH_CHANGE_SHIFT));
    *reg_mask |=  MGT_TOP_PMD_USR_GP_REGISTER_1_FINISH_CHANGE_MASK;
		
    if (side == PHY82328_INTF_SIDE_LINE) {
        *reg_mask |= (MGT_TOP_PMD_USR_GP_REGISTER_1_LINE_LR_MODE_MASK |
                      /*MGT_TOP_PMD_USR_GP_REGISTER_1_LINE_FORCED_CL72_MODE_MASK |*/
                      MGT_TOP_PMD_USR_GP_REGISTER_1_LINE_CU_TYPE_MASK);
    } else {
        /* System side mask */
        *reg_mask |=  (MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_LR_MODE_MASK |
                       MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_FORCED_CL72_MODE_MASK |
                       MGT_TOP_PMD_USR_GP_REGISTER_1_SYSTEM_CU_TYPE_MASK);
    }
    SOC_DEBUG_PRINT((DK_PHY, "82328 intf type register: u=%d, p=%d, reg=%04x/%04x, intf=%d\n",
                    unit, port, *reg_data, *reg_mask, intf_type));
 
    return SOC_E_NONE;	
}

STATIC int32
_phy_82328_intf_debug_print(int32 unit, soc_port_t port, const char *msg)
{
#if defined(BROADCOM_DEBUG) || defined(DEBUG_PRINT)
    phy_ctrl_t  *pc;
    phy82328_intf_cfg_t *line_intf, *sys_intf;
 
    pc = EXT_PHY_SW_STATE(unit, port);
    line_intf = &(LINE_INTF(pc));
    sys_intf = &(SYS_INTF(pc));
 
    SOC_DEBUG_PRINT((DK_PHY, "%s: ", msg));
    SOC_DEBUG_PRINT((DK_PHY, "[LINE:intf=%s,speed=%d], ",
                           phy82328_intf_names[line_intf->type], line_intf->speed));
    SOC_DEBUG_PRINT((DK_PHY, "[SYS :intf=%s,speed=%d]\n",
                           phy82328_intf_names[sys_intf->type], sys_intf->speed));
#endif /* BROADCOM_DEBUG || DEBUG_PRINT */
    return SOC_E_NONE;
}


STATIC int32
_phy_82328_intf_line_sys_update(int32 unit, soc_port_t port)
{
    phy_ctrl_t  *pc;
    phy_ctrl_t *int_pc; 
    uint16      data = 0, mask = 0;
    uint16      reg_data = 0, reg_mask = 0;
    int32 int_phy_en = 0;
    phy82328_intf_cfg_t *line_intf, *sys_intf;
 
    pc = EXT_PHY_SW_STATE(unit, port);    
    int_pc = INT_PHY_SW_STATE(unit, port);    
    line_intf = &(LINE_INTF(pc));
    sys_intf = &(SYS_INTF(pc));
 
    _phy_82328_intf_debug_print(unit, port, "interface/Speed update");

    reg_data = 0;
    reg_mask = 0;
    SOC_IF_ERROR_RETURN(
        _phy_82328_intf_type_reg_get(unit, port, line_intf->type,
                                     PHY82328_INTF_SIDE_LINE, &data, &mask));
    reg_data |= data;
    reg_mask |= mask;

    SOC_IF_ERROR_RETURN(
        _phy_82328_intf_type_reg_get(unit, port, sys_intf->type,
                                     PHY82328_INTF_SIDE_SYS, &data, &mask));
    reg_data |= data;
    reg_mask |= mask;
	
    SOC_DEBUG_PRINT((DK_PHY,
                     "82328  intf update: line=%s sys=%s speed=%d (1.%x = %04x/%04x): u=%d p=%d\n",
                      phy82328_intf_names[line_intf->type], phy82328_intf_names[sys_intf->type],
                      line_intf->speed, MGT_TOP_GP_REGISTER_1, reg_data, reg_mask,
                      unit, port));	
	
    SOC_IF_ERROR_RETURN(
        _phy_82328_intf_speed_reg_get(unit, port, line_intf->speed, &data, &mask));
    reg_data |= data;
    reg_mask |= mask;

    SOC_IF_ERROR_RETURN(
        _phy_82328_intf_datapath_reg_get(unit, port, CUR_DATAPATH(pc), &data, &mask));
    reg_data |= data;
    reg_mask |= mask;

    SOC_DEBUG_PRINT((DK_PHY,
        "82328  intf update: line=%s sys=%s speed=%d (1.%x = %04x/%04x): u=%d p=%d\n",
         phy82328_intf_names[line_intf->type], phy82328_intf_names[sys_intf->type],
         line_intf->speed, MGT_TOP_GP_REGISTER_1, reg_data, reg_mask,
         unit, port));

    if (SYNC_INIT(pc) == 1) {
        SOC_IF_ERROR_RETURN(PHY_ENABLE_GET(int_pc->pd, unit, port, &int_phy_en));
        if (int_phy_en) {
            /* Turn off internal PHY while the mode changes */
            SOC_IF_ERROR_RETURN(PHY_ENABLE_SET(int_pc->pd, unit, port, 0));
        }
    }
    /* GP registers are only on the line side */
    SOC_IF_ERROR_RETURN(_phy_82328_intf_update(unit, port, reg_data, reg_mask));

    if (SYNC_INIT(pc) == 1) {
        if (int_phy_en) {
            sal_usleep(10000);
            /* Turn internal PHY back on now that the mode has been configured */
            SOC_IF_ERROR_RETURN(PHY_ENABLE_SET(int_pc->pd, unit, port, 1));
        }
    }
    return SOC_E_NONE;
}


/*
 * Return whether AN is enabled or forced with internal SerDes on the system side
 */
STATIC int32
_phy_82328_intf_sys_forced(int unit, soc_port_t port, soc_port_if_t intf_type)
{
    int rv;
    phy_ctrl_t *pc;

    pc = EXT_PHY_SW_STATE(unit, port);

    switch (intf_type) {
        case SOC_PORT_IF_SR:
        case SOC_PORT_IF_SR4:
        case SOC_PORT_IF_LR:
        case SOC_PORT_IF_LR4:
        case SOC_PORT_IF_XLAUI:
        case SOC_PORT_IF_XFI:
        case SOC_PORT_IF_SFI:
        case SOC_PORT_IF_CR:
        case SOC_PORT_IF_CR4:
        case SOC_PORT_IF_ZR:
            rv = TRUE;
        break;
        case SOC_PORT_IF_GMII:
            rv = FALSE;
        break;
        case SOC_PORT_IF_KR:
        case SOC_PORT_IF_KR4:
            rv = SYS_FORCED_CL72(pc) ? TRUE : FALSE;
        break;
        default:
            rv = FALSE;
    }
    return rv;
}

STATIC int32
_phy_82328_speed_set(int32 unit, soc_port_t port, int32 speed)
{
    int cur_an, cur_an_done;
    soc_port_if_t cur_type, new_type;
    phy_ctrl_t  *pc, *int_pc;
    phy82328_intf_cfg_t  *line_intf;
    phy82328_intf_cfg_t  *sys_intf;
 
    pc = EXT_PHY_SW_STATE(unit, port);
    int_pc = INT_PHY_SW_STATE(unit, port);
    line_intf = &(LINE_INTF(pc));
    sys_intf = &(SYS_INTF(pc));
 
    line_intf->speed = speed;
    sys_intf->speed = speed;	
    
    if (int_pc != NULL) {
        if ((sys_intf->type == SOC_PORT_IF_GMII) || (sys_intf->type == SOC_PORT_IF_SGMII)) {
            SOC_IF_ERROR_RETURN(PHY_SPEED_SET(int_pc->pd, unit, port, sys_intf->speed));
        } else if (_phy_82328_intf_sys_forced(unit, port, sys_intf->type) ) {
            /*
             * When setting system side 40G DAC/XLAUI-DFE, tell internal SerDes to match
             * with XLAUI.
             *
             * If flexports, the internal SerDes may be out of sync until it gets a chance 
             *  to convert the port, so these operations may be best effort.
             */
            new_type = (sys_intf->type == SOC_PORT_IF_CR4) ? SOC_PORT_IF_XLAUI : sys_intf->type;
            SOC_IF_ERROR_RETURN(PHY_INTERFACE_GET(int_pc->pd, unit, port, &cur_type));
            if (cur_type != new_type) {
                (void) PHY_INTERFACE_SET(int_pc->pd, unit, port, new_type);
            }

            /* Check if it must change to forced */
            SOC_IF_ERROR_RETURN(PHY_AUTO_NEGOTIATE_GET(int_pc->pd, unit, port, &cur_an, &cur_an_done));
            if (cur_an == TRUE) {
                (void) PHY_AUTO_NEGOTIATE_SET(int_pc->pd, unit,port, FALSE);
            }
            (void) PHY_SPEED_SET(int_pc->pd, unit, port, sys_intf->speed);

            /* For system intf=cr4 use XLAUI_DFE on Serdes (in forced mode)*/
            if (sys_intf->type == SOC_PORT_IF_CR4) {
                (void) PHY_CONTROL_SET(int_pc->pd, unit, port,
                                        SOC_PHY_CONTROL_FIRMWARE_MODE,
                                        SOC_PHY_FIRMWARE_FORCE_OSDFE);
            }
        } else {
            SOC_IF_ERROR_RETURN(PHY_AUTO_NEGOTIATE_SET(int_pc->pd, unit,port, TRUE));
        }
    }
 
    /* Update interface type/speed */        
    return (_phy_82328_intf_line_sys_update(unit, port));
}
/*
 * Function:
 *    _phy_82328_intf_type_1G
 * Purpose:
 *   Check the interface for 1G
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #.
 *    intf_type - Interface type to test
 * Returns:
 *    TRUE/FALSE
 */
STATIC int32
_phy_82328_intf_type_1G(soc_port_if_t intf_type)
{
    int32 rv;
 
    switch (intf_type) {
        case SOC_PORT_IF_KX:
        case SOC_PORT_IF_GMII:
        case SOC_PORT_IF_SGMII:
            rv = TRUE;
        break;
        default:
            rv = FALSE;
    }
    return rv;
}

/*
 * Function:
 *    _phy_82328_intf_type_10G
 * Purpose:
 *   Check the interface for 10G
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #.
 *    intf_type - Interface type to test
 * Returns:
 *    TRUE/FALSE
 */
STATIC int32
_phy_82328_intf_type_10G(soc_port_if_t intf_type)
{
    int32 rv;
    
    switch (intf_type) {
        case SOC_PORT_IF_SR:
        case SOC_PORT_IF_CR:
		case SOC_PORT_IF_LR:
        case SOC_PORT_IF_KR:
        case SOC_PORT_IF_ZR:/*ARUN to comment on what is ZR*/
        case SOC_PORT_IF_XFI:
        case SOC_PORT_IF_SFI:/*ARUN to comment on what is SFI*/
            rv = TRUE;
        break;
        default:
            rv = FALSE;
    }
    return rv;
}

/*
 * Function:
 *      phy_82328_speed_set
 * Purpose:
 *      Set PHY speed
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      speed - link speed in Mbps
 * Returns:
 *      SOC_E_NONE
 */
 
STATIC int32
phy_82328_speed_set(int32 unit, soc_port_t port, int32 speed)
{
    int32 rv;
    phy_ctrl_t  *pc;
    phy82328_intf_cfg_t  *line_intf;
    phy82328_intf_cfg_t  *sys_intf;
 
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(port);
 
    pc = EXT_PHY_SW_STATE(unit, port);
    line_intf = &(LINE_INTF(pc));
    sys_intf = &(SYS_INTF(pc));
 
    SOC_DEBUG_PRINT((DK_PHY,"phy_82328_speed_set: u=%d p=%d speed=%d\n",
                                    unit, port,speed));
    if (PHY82328_SINGLE_PORT_MODE(pc)) {
        if (!_phy_82328_intf_is_single_port(line_intf->type)) {
             SOC_DEBUG_PRINT((DK_PHY,
                 "82328 speed set does not match interface: "
                 "u=%d p=%d speed=%d intf=%d\n",
                  unit, port, speed, sys_intf->type));
             return SOC_E_PARAM;
        }
        /*Single port mode can supported 40/40HG/42HG/100G*/
        switch (speed) {
            case 40000: 
                /* Both 40G And 40HG share same
                 * register config */
            break;
            case 42000:
                if (! IS_HG_PORT(unit, port)) {
                    speed = 40000;
                }
            break;
            case 100000:  
            break;
            default:
                SOC_DEBUG_PRINT((DK_ERR, 
                       "82328  invalid speed Single Port: u=%d p=%d speed=%d\n",
                        unit, port, speed));
            return SOC_E_PARAM;
        }
    } else {
        /*Check for the valid interface*/
        if(_phy_82328_intf_is_single_port(line_intf->type)){
            SOC_DEBUG_PRINT((DK_ERR, 
                    "82328  invalid intf in quad port: u=%d p=%d intf=%d\n",
                     unit, port, line_intf->type));
            return SOC_E_PARAM;
		}
        switch (speed) {
            case 1000:            
                if(!_phy_82328_intf_type_1G(line_intf->type)){
                    line_intf->type = SOC_PORT_IF_GMII;
                }
                /*
                 * Changing speed to 1000
                 * - if backplane interface(KR) then change to KX
                 * - if not KX, then set to GMII
                 */
                if (sys_intf->type == SOC_PORT_IF_KR) {
                    sys_intf->type = SOC_PORT_IF_KX;
                } else if (sys_intf->type != SOC_PORT_IF_KX) {
                    sys_intf->type = line_intf->type;
                }
            break;
            case 100:  
            case 10:
                if (line_intf->type != SOC_PORT_IF_SGMII) {
                    line_intf->type = SOC_PORT_IF_SGMII;
                }
                sys_intf->type = line_intf->type;
            break;
				
            /*10G/11G LR/SR/CR XFI/XFI DFE/Forced CL72*/
            case 11000:
                /* 11G only valid for HG[11] */
                if (!IS_HG_PORT(unit, port)) {
                    speed = 10000;
                }
            break;
            case 10000:
                /* If changing from 1000 to 10000/11000, then adjust interface*/
                if (line_intf->speed <= 1000) {
                    /* If changing speed from 1000 to 10000, set line interface to SR*/
                    if (!_phy_82328_intf_type_10G(line_intf->type)) {
                        line_intf->type = SOC_PORT_IF_SR;
                    }
                    if (!_phy_82328_intf_type_10G(sys_intf->type)) {
                        sys_intf->type = SOC_PORT_IF_XFI;
                    }
                }
            break;
            case 40000:
            case 42000:/*42G*/
            case 100000: 
            default:
                SOC_DEBUG_PRINT((DK_ERR, 
                      "82328  invalid speed Quad Port: u=%d p=%d speed=%d\n",
                       unit, port, speed));
            return SOC_E_PARAM;
        }
    }
	
    rv =  _phy_82328_speed_set(unit, port, speed);
    if (SOC_FAILURE(rv)) {
        SOC_DEBUG_PRINT((DK_ERR, "82328  %s failed: u=%d p=%d\n", FUNCTION_NAME(), unit, port));
    }
    return rv;
}

/*
 * Function:
 *      phy_82328_speed_get
 * Purpose:
 *      Get PHY speed
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      speed - current link speed in Mbps
 * Returns:
 *      SOC_E_NONE
 */
 
STATIC int32
phy_82328_speed_get(int32 unit, soc_port_t port, int32 *speed)
{
    phy82328_intf_cfg_t *line_intf;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
	
    if ((pc !=NULL )&& (speed !=NULL)) {
        line_intf = &(LINE_INTF(pc));
        *speed = line_intf->speed;
    } else {
        return SOC_E_PARAM;
    }
    return SOC_E_NONE;
}

/*
 * Directly select channel to access based on interface side and lane number.
 */
STATIC int32
_phy_82328_channel_direct_select(int32 unit, soc_port_t port, phy82328_intf_side_t side, int32 lane)
{
    uint16      lane_sel;
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);

    /*
     * If all lanes, restore single PMD access with lane 0 broadcast to all lanes
     * For single lane, select lane in Single PMD 1.ca86[5:4] and [3..0] that corresponds
     * to the lane with direct lane selection in 1.ca86[8]
     */
    if (lane == PHY82328_ALL_LANES) {
        lane_sel = 
            (1 << MGT_TOP_CMN_CTLR_SINGLE_PMD_CTRL_PHY_CH_TO_ACCESS_IN_SINGLE_PMD_SHIFT) |
            MGT_TOP_CMN_CTLR_SINGLE_PMD_CTRL_SINGLE_PMD_MODE_MASK |
            MGT_TOP_CMN_CTLR_SINGLE_PMD_CTRL_PHY_CH_TO_BE_DISABLE_IN_SINGLE_PMD_MASK;
    } else {
        lane_sel = (lane << MGT_TOP_CMN_CTLR_SINGLE_PMD_CTRL_PHY_CH_TO_ACCESS_IN_SINGLE_PMD_SHIFT) |
            ((1 << lane) & MGT_TOP_CMN_CTLR_SINGLE_PMD_CTRL_PHY_CH_TO_BE_DISABLE_IN_SINGLE_PMD_MASK);
    }
#if 0
    SOC_IF_ERROR_RETURN(
          _phy_82328_modify_pma_pmd_reg (unit, port, pc, 
          PHY82328_DEV_PMA_PMD,  MGT_TOP_SINGLE_PMD_CTRL, 
          MGT_TOP_CMN_CTLR_SINGLE_PMD_CTRL_SINGLE_PMD_CH_SEL_SHIFT, 
          MGT_TOP_CMN_CTLR_SINGLE_PMD_CTRL_SINGLE_PMD_CH_SEL_SHIFT, side));
#endif    
    SOC_IF_ERROR_RETURN(
        _phy_82328_modify_pma_pmd_reg (unit, port, pc, 
        PHY82328_DEV_PMA_PMD,  MGT_TOP_SINGLE_PMD_CTRL, 
        MGT_TOP_CMN_CTLR_SINGLE_PMD_CTRL_PHY_CH_TO_BE_DISABLE_IN_SINGLE_PMD_SHIFT +3/*+ 5*/, 
        MGT_TOP_CMN_CTLR_SINGLE_PMD_CTRL_PHY_CH_TO_BE_DISABLE_IN_SINGLE_PMD_SHIFT, lane_sel));
    READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, 0xca86, &lane_sel);
    return SOC_E_NONE;
}

/*
 * Select channel to access based on interface side and lane number.
 * The side is selected before returning.
 */
STATIC int32
_phy_82328_channel_select(int32 unit, soc_port_t port, phy82328_intf_side_t side, int32 lane)
{
    int32 rv;
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);

    if (!(PHY82328_SINGLE_PORT_MODE(pc))) {
        return SOC_E_PARAM;
    }

    if (!((lane == PHY82328_ALL_LANES) || ((lane >= 0) && (lane <= 3)))) {
        return SOC_E_PARAM;
    }
    PHY_82328_REGS_SELECT(unit, pc, side);

    rv  = _phy_82328_channel_direct_select(unit, port, side, lane);

    return rv;
}

/*
 *   20-bit: set txpol in line side 1.DOE3[0]
 *   4-bit:  set txpol in line side 1.D0A0[9]
 */
STATIC int32
_phy_82328_polarity_flip_tx_set(int32 unit, soc_port_t port, int32 flip)
{	
    uint16		data = 0;
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);

	/* Clear to flip, set to unflip */
    if (CUR_DATAPATH(pc) == PHY82328_DATAPATH_20) {
        data = flip ? TLB_RX_TLB_RX_MISC_CONFIG_RX_PMD_DP_INVERT_MASK : 0;
        SOC_IF_ERROR_RETURN(
          _phy_82328_modify_pma_pmd_reg (unit, port, pc, 
          PHY82328_DEV_PMA_PMD,  TLB_TX_TLB_TX_MISC_CONFIG, 
          TLB_TX_TLB_TX_MISC_CONFIG_TX_PMD_DP_INVERT_SHIFT, 
          TLB_TX_TLB_TX_MISC_CONFIG_TX_PMD_DP_INVERT_SHIFT, data));
    } else {
        data = flip ? 1 : 0;
        SOC_IF_ERROR_RETURN(
          _phy_82328_modify_pma_pmd_reg (unit, port, pc, 
          PHY82328_DEV_PMA_PMD,  AMS_TX_TX_CTRL_0,
          AMS_TX_TX_CTRL_0_LL_POLARITY_FLIP_SHIFT, 
          AMS_TX_TX_CTRL_0_LL_POLARITY_FLIP_SHIFT, data));
    }
    return SOC_E_NONE;
}

STATIC int32
_phy_82328_polarity_flip_tx(int32 unit, soc_port_t port, phy82328_intf_side_t side, uint16 cfg_pol)
{
    int32			flip;
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);
    int32	lane = 0;
    
    if (side != PHY82328_INTF_SIDE_LINE) {
        PHY_82328_REGS_SELECT(unit, pc, side);
    }
    if ((PHY82328_SINGLE_PORT_MODE(pc))) {
        if (cfg_pol != PHY82328_POL_DND) {
            for (lane = 0; lane < PHY82328_NUM_LANES; lane++) {
	            /* Check for all lanes or whether this lane has polarity flipped */
                if ((cfg_pol & POL_CONFIG_LANE_MASK(lane)) == POL_CONFIG_LANE_MASK(lane)) {
				    flip = 1;
                } else {
                    flip = 0;
			    }
			    SOC_DEBUG_PRINT((DK_PHY, "82328 tx polarity flip=%d: u=%d p=%d lane=%d\n", flip, unit, port, lane));

			    /* Select the lane on the line side */
                SOC_IF_ERROR_RETURN
                    (_phy_82328_channel_select(unit, port, side, lane));
                SOC_IF_ERROR_RETURN
                    (_phy_82328_polarity_flip_tx_set(unit, port, flip));
            }
            /* Restore to default single port register access */
            SOC_IF_ERROR_RETURN(_phy_82328_channel_select(unit, port, PHY82328_INTF_SIDE_LINE, PHY82328_ALL_LANES));
        }
    } else {  /* quad mode */
        if (cfg_pol != PHY82328_POL_DND) {
            flip = (cfg_pol) ? 1 : 0;
            SOC_IF_ERROR_RETURN
                (_phy_82328_polarity_flip_tx_set(unit, port, flip));
            SOC_DEBUG_PRINT
                ((DK_PHY, "82328  polarity flip: u=%d p=%d\n", unit, port));
        }
	}
    if (side != PHY82328_INTF_SIDE_LINE) {
        PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_LINE);
    }

    return SOC_E_NONE;
}

STATIC int32
_phy_82328_polarity_flip_rx_20bit(int32 unit, soc_port_t port, int32 flip)
{
    uint16		data = 0;
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);

    data = flip ? 1 : 0;
    SOC_IF_ERROR_RETURN(
          _phy_82328_modify_pma_pmd_reg (unit, port, pc, 
          PHY82328_DEV_PMA_PMD,  TLB_RX_TLB_RX_MISC_CONFIG, 
          TLB_RX_TLB_RX_MISC_CONFIG_RX_PMD_DP_INVERT_SHIFT, 
          TLB_RX_TLB_RX_MISC_CONFIG_RX_PMD_DP_INVERT_SHIFT, data));

    return SOC_E_NONE;
}
			
STATIC int32
_phy_82328_polarity_flip_rx(int32 unit, soc_port_t port, phy82328_intf_side_t side, uint16 cfg_pol)
{
    int32  flip;
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);
    int32  lane;

    if (side != PHY82328_INTF_SIDE_LINE) {
        PHY_82328_REGS_SELECT(unit, pc, side);
    }
    if ((PHY82328_SINGLE_PORT_MODE(pc))) {
        if (cfg_pol != PHY82328_POL_DND) {	
            /* 4-bit polarity handled on system side */
     		if (CUR_DATAPATH(pc) != PHY82328_DATAPATH_20) {
                PHY_82328_MICRO_PAUSE(unit, port, "polarity rx");
            }
            for (lane = 0; lane < PHY82328_NUM_LANES; lane++) {
                flip = 0;
                if ((cfg_pol & POL_CONFIG_LANE_MASK(lane)) == POL_CONFIG_LANE_MASK(lane)) {
                    flip = 1;
                }
                if (CUR_DATAPATH(pc) == PHY82328_DATAPATH_20) {
                    SOC_IF_ERROR_RETURN
                        (_phy_82328_channel_select(unit, port, side, lane));
                    SOC_IF_ERROR_RETURN
                        (_phy_82328_polarity_flip_rx_20bit(unit, port, flip));
                }
                SOC_DEBUG_PRINT((DK_PHY, "82328 rx polarity flip: u=%d p=%d lane=%d\n", unit, port, lane));
            }
            /* Restore to default single port register access */
            SOC_IF_ERROR_RETURN
                (_phy_82328_channel_select(unit, port, PHY82328_INTF_SIDE_LINE, PHY82328_ALL_LANES));
            if (CUR_DATAPATH(pc) != PHY82328_DATAPATH_20) {
                PHY_82328_MICRO_RESUME(unit, port);
            }
        }
    } else {  /* quad mode */
        if (cfg_pol != PHY82328_POL_DND) {
            flip = (cfg_pol)? 1:0;
            SOC_IF_ERROR_RETURN(_phy_82328_polarity_flip_rx_20bit(unit, port, flip));
        }
    }
    
    if (side != PHY82328_INTF_SIDE_LINE) {
        PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_LINE);
    }

    SOC_DEBUG_PRINT((DK_PHY, "82328 rx polarity flip: u=%d p=%d\n", unit, port));
    return SOC_E_NONE;
}

/*                                                                                              
 * Flip configured tx/rx polarities
 *
 * Property values are either 1 for all lanes or
 * lane 0: 0001
 * lane 1: 0002
 * lane 2: 0004
 * lane 3: 0008
 */
STATIC int32
_phy_82328_polarity_flip(int32 unit, soc_port_t port, uint16 cfg_tx_pol, uint16 cfg_rx_pol)
{
    SOC_DEBUG_PRINT((DK_PHY, "82328 polarity flip: u=%d p%d TX=%x RX=%x\n", unit, port, 
                    cfg_tx_pol, cfg_rx_pol));
    if (cfg_tx_pol != PHY82328_POL_DND) {
        SOC_IF_ERROR_RETURN(
            _phy_82328_polarity_flip_tx(unit, port, PHY82328_INTF_SIDE_LINE, cfg_tx_pol));
    }
    if (cfg_rx_pol != PHY82328_POL_DND) {
        SOC_IF_ERROR_RETURN(
            _phy_82328_polarity_flip_rx(unit, port, PHY82328_INTF_SIDE_LINE, cfg_rx_pol));
    }

    return SOC_E_NONE;
}

STATIC void
_phy_82328_micro_pause(int unit, soc_port_t port, const char *loc)
{
    int rv = SOC_E_NONE;
    uint16 data, mask;
    int checks;
    uint16 csr;
    uint16 saved_side;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
    phy82328_micro_ctrl_t *micro_ctrl = &(MICRO_CTRL(pc));

    if (!micro_ctrl->enable) {
        return;
    }

    /* Access line side registers */
    saved_side = _phy_82328_intf_side_regs_get(unit, port);
    if (saved_side == PHY82328_INTF_SIDE_SYS) {
        WRITE_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_XPMD_REGS_SEL,PHY82328_INTF_SIDE_LINE);
    }

    micro_ctrl->count++;
    
    /* Quiet micro */
    data = 0;
    mask = 0xff00;
    rv = MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_GP_REG_0, data, mask);
    if (rv != SOC_E_NONE) {
        goto fail;
    }
    sal_udelay(500);
    checks = 0;
    while (checks < 1000) {
        /* Make sure micro really paused */
        rv = READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_GP_REG_2, &data);
        if (rv != SOC_E_NONE) {
            goto fail;
        }
        if ((data & mask) == 0) {
            break;
        }
        sal_udelay(100);
        checks++;
    }
    if (data & mask) {
        rv = READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_GP_REG_0, &csr);
        if (rv != SOC_E_NONE) {
            goto fail;
        }
        SOC_DEBUG_PRINT((DK_PHY, "82328 microcode did not pause in %s: "
               "u=%d p%d 1.ca18/1.ca19=%04x/%04x checks=%d\n", 
                loc, unit, port, csr, data, checks));
    }

fail:
    if (saved_side != PHY82328_INTF_SIDE_LINE) {
        WRITE_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_XPMD_REGS_SEL, saved_side);
    }
    return;
}

STATIC void
_phy_82328_micro_resume(int unit, soc_port_t port)
{
    phy82328_intf_side_t saved_side;
    int rv = SOC_E_NONE;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
    phy82328_micro_ctrl_t *micro_ctrl = &(MICRO_CTRL(pc));

    if (!micro_ctrl->enable) {
        return;
    }

    /* Access line side registers */
    saved_side = _phy_82328_intf_side_regs_get(unit, port);
    if (saved_side == PHY82328_INTF_SIDE_SYS) {
        WRITE_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_XPMD_REGS_SEL, PHY82328_INTF_SIDE_LINE);
	}

    micro_ctrl->count--;
    if (micro_ctrl->count <= 0) {
        rv = MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_GP_REG_0, 
                                             0xff00, 0xff00);
        if (rv != SOC_E_NONE) {
            goto fail;
        }
        if (micro_ctrl->count < 0) {
            SOC_DEBUG_PRINT((DK_VERBOSE, "82328 unmatched micro resume\n"));
            micro_ctrl->count = 0;;
        }
    }

fail:
    if (saved_side != PHY82328_INTF_SIDE_LINE) {
        WRITE_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_XPMD_REGS_SEL, saved_side);
    }
    return;
}

/*
 * Function:
 *      phy_82328_an_set
 * Purpose:
 *      Configures auto-negotiation 
 *      Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      enable   - if true, auto-negotiation is enabled.
 * Returns:
 *      SOC_E_XXX
 */

STATIC int32
phy_82328_an_set(int32 unit, soc_port_t port, int32 enable)
{
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_82328_an_get
 * Purpose:
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      an   - (OUT) if true, auto-negotiation is enabled.
 *      an_done - (OUT) if true, auto-negotiation is complete. This
 *              value is undefined if an == false.
 * Returns:
 *      SOC_E_XXX
 */
 
STATIC int32
phy_82328_an_get(int32 unit, soc_port_t port, int32 *an, int32 *an_done)
{
    return SOC_E_NONE;
}

/*
 * Squelch/unsquelch tx in 1.reg.bit
 */
STATIC int32
_phy_82328_tx_squelch_enable(int32 unit, soc_port_t port, phy82328_intf_side_t side, uint8 squelch)
{
    phy_ctrl_t *pc = EXT_PHY_SW_STATE(unit, port);

    SOC_IF_ERROR_RETURN( 
          _phy_82328_modify_pma_pmd_reg (unit, port, pc, 
           PHY82328_DEV_PMA_PMD,  TX_ANATXACONTROL7, 
           QSFI_TX_ANATXACONTROL7_TX_DISABLE_FORCE_EN_SM_SHIFT,
           QSFI_TX_ANATXACONTROL7_TX_DISABLE_FORCE_EN_SM_SHIFT, 1));
 
    SOC_IF_ERROR_RETURN( 
          _phy_82328_modify_pma_pmd_reg (unit, port, pc, 
           PHY82328_DEV_PMA_PMD,  TX_ANATXACONTROL7, 
           QSFI_TX_ANATXACONTROL7_TX_DISABLE_FORCE_VAL_SM_SHIFT, 
           QSFI_TX_ANATXACONTROL7_TX_DISABLE_FORCE_VAL_SM_SHIFT, squelch? 1:0));

    return SOC_E_NONE;
}

STATIC int32
_phy_82328_tx_squelch(int32 unit, soc_port_t port, phy82328_intf_side_t side, int32 cfg_sql)
{
    uint8  squelch = 0;
    int32	lane = 0;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
	
    PHY_82328_REGS_SELECT (unit, pc, side);
    if (PHY82328_SINGLE_PORT_MODE(pc)) {
        for (lane = 0; lane < PHY82328_NUM_LANES; lane++) {
            /* Check for all lanes or whether this lane has to be squelched */
            if(cfg_sql & (1<<lane)){
                squelch = 1;
            } else {
                squelch = 0;
            }
            SOC_DEBUG_PRINT((DK_PHY, "82328 tx squelch=%d: u=%d p=%d lane=%d\n", squelch, unit, port, lane));

            /* Select the lane on the line side */
            SOC_IF_ERROR_RETURN(_phy_82328_channel_select (unit, port, side, lane));
            SOC_IF_ERROR_RETURN(_phy_82328_tx_squelch_enable (unit, port, side, squelch));
        }
        /* Restore to default single port register access */
        SOC_IF_ERROR_RETURN(_phy_82328_channel_select (unit, port, PHY82328_INTF_SIDE_LINE, PHY82328_ALL_LANES));
    } else {  
        squelch = (cfg_sql) ? 1 : 0;
        SOC_IF_ERROR_RETURN(_phy_82328_tx_squelch_enable (unit, port, side, squelch));
        SOC_DEBUG_PRINT((DK_PHY, "82328 squelch: u=%d p=%d\n", unit, port));
    }

    if (side != PHY82328_INTF_SIDE_LINE) {
        PHY_82328_REGS_SELECT (unit, pc, PHY82328_INTF_SIDE_LINE);
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *    _phy_82328_remote_loopback_get
 * Purpose:
 *    Get 82328 PHY loopback state
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #.
 *    enable - (OUT) address of location to store binary value for on/off (1/0)
 * Returns:
 *    SOC_E_NONE
 */ 
 STATIC int32
_phy_82328_remote_loopback_get(int32 unit, soc_port_t port, uint32 *enable)
{
    uint16 read_data=0,ret_val = 1,temp=0;		
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
		
    /* Remote Loop Back Set <Line Side>*/
    PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_LINE);		

    /*1 rmt_lpbk_config.rmt_lpbk_en 1.D0E2.0 1*/
    SOC_IF_ERROR_RETURN
        (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                  TLB_TX_RMT_LPBK_CONFIG, &read_data));	

    read_data &=TLB_TX_RMT_LPBK_CONFIG_RMT_LPBK_EN_MASK;
    read_data = read_data >> TLB_TX_RMT_LPBK_CONFIG_RMT_LPBK_EN_SHIFT;
    ret_val &=read_data;		
    
    /*2 tx_pi_control_5.tx_pi_pass_thru_sel 1.D075.2 0*/		
    SOC_IF_ERROR_RETURN
        (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                TX_PI_TX_PI_CONTROL_5, &read_data));

    read_data &=TX_PI_TX_PI_CONTROL_5_TX_PI_PASS_THRU_SEL_MASK;
    read_data = read_data >> TX_PI_TX_PI_CONTROL_5_TX_PI_PASS_THRU_SEL_SHIFT;
    ret_val &=(!read_data);													 
    
    /*3 tx_pi_control_0.tx_pi_rmt_lpbk_bypass_flt 1.D070.9 1*/
    /*4 tx_pi_control_0.tx_pi_ext_ctrl_en 1.D070.2 1*/
    SOC_IF_ERROR_RETURN
        (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                 TX_PI_TX_PI_CONTROL_0, &read_data));
    temp = read_data;
    temp &=TX_PI_TX_PI_CONTROL_0_TX_PI_RMT_LPBK_BYPASS_FLT_MASK;
    temp = temp >> TX_PI_TX_PI_CONTROL_0_TX_PI_RMT_LPBK_BYPASS_FLT_SHIFT;
    ret_val &=temp;		
    read_data &=TX_PI_TX_PI_CONTROL_0_TX_PI_EXT_CTRL_EN_MASK;
    read_data = read_data >> TX_PI_TX_PI_CONTROL_0_TX_PI_EXT_CTRL_EN_SHIFT;
    ret_val &=read_data;
		
    *enable = ret_val;		
    return SOC_E_NONE;
}
/*
 * Function:
 *    _phy_82328_remote_loopback_set
 * Purpose:
 *    Get 82328 PHY loopback state
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #.
 *    enable - address of location to store binary value for on/off (1/0)
 * Returns:
 *    SOC_E_NONE
 */
 STATIC int32
_phy_82328_remote_loopback_set(int32 unit,soc_port_t port,int32 enable)
{
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);

    /* Loopback requires 20-bit datapath */	
    if (CFG_DATAPATH(pc) != PHY82328_DATAPATH_20) {
        CUR_DATAPATH(pc) = enable ? PHY82328_DATAPATH_20:CFG_DATAPATH(pc);
        SOC_IF_ERROR_RETURN(_phy_82328_intf_datapath_update(unit, port));
    }
    /*
     * If enabling loopback
     *    disable tx line
     * else
     *    enable tx line
     */
     SOC_IF_ERROR_RETURN
       (_phy_82328_tx_squelch(unit, port, PHY82328_INTF_SIDE_LINE, (enable)?0xF:0));
     sal_usleep(100);
    /* Remote LoopBk Set <Line Side>*/
    PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_LINE);
    if (enable){
        /*1 rmt_lpbk_config.rmt_lpbk_en 1.D0E2.0 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TLB_TX_RMT_LPBK_CONFIG,
                                                            (1<<TLB_TX_RMT_LPBK_CONFIG_RMT_LPBK_EN_SHIFT), 
                                                            TLB_TX_RMT_LPBK_CONFIG_RMT_LPBK_EN_MASK));
        /*2 tx_pi_control_5.tx_pi_pass_thru_sel 1.D075.2 0*/		
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_5,
                                                            0, 
                                                            TX_PI_TX_PI_CONTROL_5_TX_PI_PASS_THRU_SEL_MASK));
        /*3 tx_pi_control_0.tx_pi_rmt_lpbk_bypass_flt 1.D070.9 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_0,
                                                            (1<<TX_PI_TX_PI_CONTROL_0_TX_PI_RMT_LPBK_BYPASS_FLT_SHIFT), 
	                                                        TX_PI_TX_PI_CONTROL_0_TX_PI_RMT_LPBK_BYPASS_FLT_MASK));
        sal_usleep(100);
        /*4 tx_pi_control_0.tx_pi_ext_ctrl_en 1.D070.2 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_0,
                                                            (1<<TX_PI_TX_PI_CONTROL_0_TX_PI_EXT_CTRL_EN_SHIFT), 
                                                            TX_PI_TX_PI_CONTROL_0_TX_PI_EXT_CTRL_EN_MASK));				
    } else{
        /*1 rmt_lpbk_config.rmt_lpbk_en 1.D0E2.0 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TLB_TX_RMT_LPBK_CONFIG,
                                                            0, 
                                                            TLB_TX_RMT_LPBK_CONFIG_RMT_LPBK_EN_MASK));
        /*2 tx_pi_control_5.tx_pi_pass_thru_sel 1.D075.2 0*/		
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_5,
                                                            (1<<TX_PI_TX_PI_CONTROL_5_TX_PI_PASS_THRU_SEL_SHIFT), 
                                                            TX_PI_TX_PI_CONTROL_5_TX_PI_PASS_THRU_SEL_MASK));
        /*3 tx_pi_control_0.tx_pi_rmt_lpbk_bypass_flt 1.D070.9 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_0,
                                                            0, 
                                                            TX_PI_TX_PI_CONTROL_0_TX_PI_RMT_LPBK_BYPASS_FLT_MASK));
        sal_usleep(100);
        /*4 tx_pi_control_0.tx_pi_ext_ctrl_en 1.D070.2 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_0,
                                                            0, 
                                                            TX_PI_TX_PI_CONTROL_0_TX_PI_EXT_CTRL_EN_MASK));	
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *    phy_82328_lb_get
 * Purpose:
 *    Get 82328 PHY loopback state
 * Parameters:
 *    unit - StrataSwitch unit #.
 *    port - StrataSwitch port #.
 *    enable - (OUT) address of location to store binary value for on/off (1/0)
 * Returns:
 *    SOC_E_NONE
 */ 
STATIC int32
phy_82328_lb_get(int32 unit, soc_port_t port, int32 *enable)
{
    uint16 read_data=0,ret_val=1,temp=0;		
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
		
    /* LB Set API Supports System Side*/
    PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_SYS);

    /*1 rmt_lpbk_config.rmt_lpbk_en 1.D0E2.0 1*/
    SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG (unit, pc,
                         TLB_TX_RMT_LPBK_CONFIG, &read_data));
    
    read_data &=TLB_TX_RMT_LPBK_CONFIG_RMT_LPBK_EN_MASK;
    read_data = read_data >> TLB_TX_RMT_LPBK_CONFIG_RMT_LPBK_EN_SHIFT;
	ret_val &=read_data;

    /*2 tx_pi_control_5.tx_pi_pass_thru_sel 1.D075.2 0*/
    SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                              TX_PI_TX_PI_CONTROL_5, &read_data));

    read_data &=TX_PI_TX_PI_CONTROL_5_TX_PI_PASS_THRU_SEL_MASK;
    read_data = read_data >> TX_PI_TX_PI_CONTROL_5_TX_PI_PASS_THRU_SEL_SHIFT;
    ret_val &=(!read_data);

    /*3 tx_pi_control_0.tx_pi_rmt_lpbk_bypass_flt 1.D070.9 1*/
    /*4 tx_pi_control_0.tx_pi_ext_ctrl_en 1.D070.2 1*/

    SOC_IF_ERROR_RETURN (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                        TX_PI_TX_PI_CONTROL_0, &read_data));

    temp = read_data;
    temp &=TX_PI_TX_PI_CONTROL_0_TX_PI_RMT_LPBK_BYPASS_FLT_MASK;
    temp = temp >> TX_PI_TX_PI_CONTROL_0_TX_PI_RMT_LPBK_BYPASS_FLT_SHIFT;
    ret_val &=temp;
		
    read_data &=TX_PI_TX_PI_CONTROL_0_TX_PI_EXT_CTRL_EN_MASK;
    read_data = read_data >> TX_PI_TX_PI_CONTROL_0_TX_PI_EXT_CTRL_EN_SHIFT;
    ret_val &=read_data;
		
    *enable = ret_val;
    PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_LINE);
		
    return SOC_E_NONE;
}

STATIC int32
phy_82328_lb_set(int32 unit, soc_port_t port, int32 enable)
{
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);

    /* Loopback requires 20-bit datapath */	
    if (CFG_DATAPATH(pc) != PHY82328_DATAPATH_20) {
       CUR_DATAPATH(pc) = enable ? PHY82328_DATAPATH_20:CFG_DATAPATH(pc);
       SOC_IF_ERROR_RETURN(_phy_82328_intf_datapath_update(unit, port));
    }
    /*
     * If enabling loopback
     *    disable tx line
     * else
     *    enable tx line
     */
    SOC_IF_ERROR_RETURN
      (_phy_82328_tx_squelch (unit, port, PHY82328_INTF_SIDE_SYS, (enable)?0xF:0));
    sal_usleep(100);
    /* LB Set API Supports System Side*/
    PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_SYS);
    if (enable){
        /*1 rmt_lpbk_config.rmt_lpbk_en 1.D0E2.0 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TLB_TX_RMT_LPBK_CONFIG,
                                                            (1<<TLB_TX_RMT_LPBK_CONFIG_RMT_LPBK_EN_SHIFT), 
                                                            TLB_TX_RMT_LPBK_CONFIG_RMT_LPBK_EN_MASK));
        /*2 tx_pi_control_5.tx_pi_pass_thru_sel 1.D075.2 0*/		
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_5,
                                                            0, 
                                                            TX_PI_TX_PI_CONTROL_5_TX_PI_PASS_THRU_SEL_MASK));
        /*3 tx_pi_control_0.tx_pi_rmt_lpbk_bypass_flt 1.D070.9 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_0,
                                                            (1<<TX_PI_TX_PI_CONTROL_0_TX_PI_RMT_LPBK_BYPASS_FLT_SHIFT), 
                                                            TX_PI_TX_PI_CONTROL_0_TX_PI_RMT_LPBK_BYPASS_FLT_MASK));
        sal_usleep(100);
        /*4 tx_pi_control_0.tx_pi_ext_ctrl_en 1.D070.2 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_0,
                                                            (1<<TX_PI_TX_PI_CONTROL_0_TX_PI_EXT_CTRL_EN_SHIFT), 
                                                            TX_PI_TX_PI_CONTROL_0_TX_PI_EXT_CTRL_EN_MASK));				
    } else {
        /*1 rmt_lpbk_config.rmt_lpbk_en 1.D0E2.0 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TLB_TX_RMT_LPBK_CONFIG,
                                                            0, 
                                                            TLB_TX_RMT_LPBK_CONFIG_RMT_LPBK_EN_MASK));
        /*2 tx_pi_control_5.tx_pi_pass_thru_sel 1.D075.2 0*/		
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_5,
                                                            (1<<TX_PI_TX_PI_CONTROL_5_TX_PI_PASS_THRU_SEL_SHIFT), 
                                                            TX_PI_TX_PI_CONTROL_5_TX_PI_PASS_THRU_SEL_MASK));

        /*3 tx_pi_control_0.tx_pi_rmt_lpbk_bypass_flt 1.D070.9 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_0,
                                                            0, 
                                                            TX_PI_TX_PI_CONTROL_0_TX_PI_RMT_LPBK_BYPASS_FLT_MASK));
        sal_usleep(100);
        /*4 tx_pi_control_0.tx_pi_ext_ctrl_en 1.D070.2 1*/
        SOC_IF_ERROR_RETURN(MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                                            TX_PI_TX_PI_CONTROL_0,
                                                            0, 
                                                            TX_PI_TX_PI_CONTROL_0_TX_PI_EXT_CTRL_EN_MASK));	
    }	
    PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_LINE);

    return SOC_E_NONE;
}

STATIC int32
_phy_82328_intf_type_set(int32 unit, soc_port_t port, soc_port_if_t pif, int32 must_update)
{
    int32			rv = SOC_E_NONE;
    phy_ctrl_t      *pc = EXT_PHY_SW_STATE(unit, port);
    phy82328_intf_cfg_t *line_intf;
    int32			update = 0;
    uint16          reg_data = 0, reg_mask = 0;
    uint16          data = 0, mask = 0;

    line_intf = &(LINE_INTF(pc));
	
    reg_data = 0;
    reg_mask = 0;

    if (_phy_82328_intf_is_single_port(pif)) {
        if (!PHY82328_SINGLE_PORT_MODE(pc)) {
            SOC_DEBUG_PRINT((DK_ERR, "82328 invalid interface for quad port: u=%d p=%d\n",
                             unit, port));
            return SOC_E_CONFIG;
        }
        /* 
         * If rx polarity is flipped and the interface is CR4, then change the datapath
         * to 20-bit as rx polarity cannot be handle on the line side in 4-bit
         */
        if (pif == SOC_PORT_IF_CR4) {
            if (AN_EN(pc)) {
                if (POL_RX_CFG(pc) && (CUR_DATAPATH(pc) != PHY82328_DATAPATH_20)) {
                    CUR_DATAPATH(pc) = PHY82328_DATAPATH_20;
                    update = 1;
                }
            } else if (CUR_DATAPATH(pc) != CFG_DATAPATH(pc)) {
                CUR_DATAPATH(pc) = CFG_DATAPATH(pc);
                update = 1;
            }
            SOC_IF_ERROR_RETURN(
                _phy_82328_intf_datapath_reg_get(unit, port, CUR_DATAPATH(pc), &data, &mask));
            reg_data |= data;
            reg_mask |= mask;
        }
        if ((pif != line_intf->type) || must_update) {
            update = 1;
        }
    } else if (_phy_82328_intf_is_quad_port(pif)) {
        if (PHY82328_SINGLE_PORT_MODE(pc)) {
            SOC_DEBUG_PRINT((DK_ERR, "82328 invalid interface for single port: u=%d p=%d\n",
                             unit, port));
            return SOC_E_CONFIG;
        }

        if ((line_intf->type != pif) || must_update) {
            update = 1;
            SOC_IF_ERROR_RETURN(
               _phy_82328_intf_type_reg_get(unit, port, pif, PHY82328_INTF_SIDE_LINE,
                                           &reg_data, &reg_mask));
        }
    } else {
        SOC_DEBUG_PRINT((DK_ERR, "82328 invalid interface for port: u=%d p=%d intf=%d\n",
                         unit, port, pif));
        return SOC_E_CONFIG;
    }
    if (update) {
        line_intf->type = pif;
        SOC_IF_ERROR_RETURN(
            _phy_82328_intf_type_reg_get(unit, port, pif, PHY82328_INTF_SIDE_LINE,
                                             &data, &mask));
        reg_data |= data;
        reg_mask |= mask;
        SOC_IF_ERROR_RETURN(_phy_82328_intf_update(unit, port, reg_data, reg_mask));
    }
    return rv;
}

STATIC int32
_phy_82328_intf_line_forced(int32 unit, soc_port_t port, soc_port_if_t intf_type)
{
    int32 rv;
    switch (intf_type) {
        case SOC_PORT_IF_SR:
        case SOC_PORT_IF_SR4:
        case SOC_PORT_IF_LR:
        case SOC_PORT_IF_ZR:
        case SOC_PORT_IF_LR4:
        case SOC_PORT_IF_XLAUI:
        case SOC_PORT_IF_XFI:
        case SOC_PORT_IF_SFI:
        case SOC_PORT_IF_CR:
            rv = TRUE;
        break;
        default:
            rv = FALSE;
    }
    return rv;
}

/*
 * Function:
 *      phy_82328_interface_set
 * Purpose:
 *
 *
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      pif - one of SOC_PORT_IF_*
 * Returns:
 *      SOC_E_NONE - success
 *      SOC_E_UNAVAIL - unsupported interface
 */
 
STATIC int32
phy_82328_interface_set(int32 unit, soc_port_t port, soc_port_if_t pif)
{
    int32 rv;

    soc_cm_print("82328 interface set: u=%d p=%d pif=%s\n", 
                  unit, port, phy82328_intf_names[pif]);

    if (pif == SOC_PORT_IF_XGMII) {
        return phy_null_interface_set(unit, port, pif);
    }

    rv = _phy_82328_intf_type_set(unit, port, pif, FALSE);
    if (SOC_FAILURE(rv)) {
        SOC_DEBUG_PRINT((DK_PHY, "82328  interface set check failed: u=%d p=%d\n",
                          unit, port));
    }
    if (_phy_82328_intf_line_forced(unit, port, pif)) {
        SOC_IF_ERROR_RETURN(phy_82328_an_set(unit, port, 0));
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_82328_interface_get
 * Purpose:
 *      Get the current operating mode of the PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      pif - (OUT) one of SOC_PORT_IF_*
 * Returns:
 *      SOC_E_NONE
 */
STATIC int32
phy_82328_interface_get(int32 unit, soc_port_t port, soc_port_if_t *pif)
{
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
    phy82328_intf_cfg_t *line_intf;

    line_intf = &(LINE_INTF(pc));
    *pif = line_intf->type;

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_82328_intf_datapath_update
 * Purpose:
 *      Update the datapath of the PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #. *      
 * Returns:
 *      SOC_E_NONE
 */
STATIC int32
_phy_82328_intf_datapath_update(int32 unit,  soc_port_t port)
{
    phy_ctrl_t  *pc;
    uint16      data = 0, mask = 0;
 
    pc = EXT_PHY_SW_STATE(unit, port);
    SOC_IF_ERROR_RETURN(
      _phy_82328_intf_datapath_reg_get(unit, port, CUR_DATAPATH(pc), &data, &mask));
 
    /* GP registers are only on the line side */
    SOC_IF_ERROR_RETURN(_phy_82328_intf_update(unit, port, data, mask));
 
    return SOC_E_NONE;
}

STATIC int
_phy_82328_micro_tx_squelch_enable(int unit, soc_port_t port, int enable)
{
    uint16 data;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);

    /* 1.ca18[7] 0=micro squelching enabled, 1=micro squelching disabled */
    data = enable ? 0 : 0x80;
    return MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, MGT_TOP_GP_REG_0, data, 0x80);
}

/*
 * Function:
 *      _phy_82328_control_prbs_enable_set
 * Purpose:
 *      Get the current operating mode of the PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      enable - PRBS Generator Enable
 * Returns:
 *      SOC_E_NONE
 */
STATIC int32
_phy_82328_control_prbs_enable_set(int32 unit, soc_port_t port, int32 enable)
{
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
 
    /*Revisit Need to be implemented*/
    /* SOC_IF_ERROR_RETURN(_phy_82328_sw_rx_los_pause(unit, port, enable));     
     * SOC_IF_ERROR_RETURN(_phy_82328_link_mon_pause(unit, port, enable));*/
 
    /* Disable Prbs Gen/Chkr First -> Refer Programming Guide 1.2*/
    /* Disable Prbs Gen in TLB_TX_PRBS_GEN_CONFIG 1.D0E1.1 */
	  
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
               TLB_TX_PRBS_GEN_CONFIG, 0, TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_EN_MASK));
															
    /* Disable Prbs Chkr in in TLB_RX_PRBS_CHK_CONFIG 1.D0D1.0*/	  
    SOC_IF_ERROR_RETURN
        (MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, 
               TLB_RX_PRBS_CHK_CONFIG, 0, TLB_RX_PRBS_CHK_CONFIG_PRBS_CHK_EN_MASK));

    if (!enable) {
        if (CFG_DATAPATH(pc) != PHY82328_DATAPATH_20) {
            CUR_DATAPATH(pc) = CFG_DATAPATH(pc);
        }
        /* Update datapath */
        SOC_IF_ERROR_RETURN(_phy_82328_intf_datapath_update(unit, port));
    }
    /* PRBS done with 20-bit datapath */
    if (enable) {
        if (CFG_DATAPATH(pc) != PHY82328_DATAPATH_20) {
            CUR_DATAPATH(pc) = PHY82328_DATAPATH_20;
        }
        /* Update datapath */
        SOC_IF_ERROR_RETURN(_phy_82328_intf_datapath_update(unit, port));				
				
        /* Enable Prbs Gen in TLB_TX_PRBS_GEN_CONFIG 1.D0E1.0 */
        /*PRBS Gen Enable*/
        SOC_IF_ERROR_RETURN(
            MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, TLB_TX_PRBS_GEN_CONFIG,
                (1 << TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_EN_SHIFT), 
                TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_EN_MASK));
				
        /*Prbs Checker En in TLB_RX_PRBS_CHK_CONFIG 1.D0D1.0*/
        /*PRBS Chk Enable*/
        SOC_IF_ERROR_RETURN (
           MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc,
            TLB_RX_PRBS_CHK_CONFIG, (1 << TLB_RX_PRBS_CHK_CONFIG_PRBS_CHK_EN_SHIFT), 
            TLB_RX_PRBS_CHK_CONFIG_PRBS_CHK_EN_MASK));				
    }
    /*
     * If enabling prbs
     *    disable micro tx squelch updates
     *    unsquelch tx system
     * else
     *    enable micro tx squelch updates
     */
    SOC_IF_ERROR_RETURN(_phy_82328_micro_tx_squelch_enable(unit, port, !enable));
    if (enable) {
        SOC_IF_ERROR_RETURN(_phy_82328_tx_squelch(unit, port, PHY82328_INTF_SIDE_SYS, 0));
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_82328_control_prbs_invert_data_set
 * Purpose:
 *      Get the current operating mode of the PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      invert - PRBS Invert
 * Returns:
 *      SOC_E_NONE
 */
STATIC int32
_phy_82328_control_prbs_invert_data_set(int32 unit, soc_port_t port, int32 invert)
{
    uint16          data;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
 
    data = invert ? (1 << TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_INV_SHIFT):0;
    
    /*PRBS Gen Invert*/
    SOC_IF_ERROR_RETURN(
        MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, 
              TLB_TX_PRBS_GEN_CONFIG, data,
              TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_INV_MASK));

    /*PRBS Chk Invert*/
    SOC_IF_ERROR_RETURN(
         MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, 
                TLB_RX_PRBS_CHK_CONFIG, data,
                TLB_RX_PRBS_CHK_CONFIG_PRBS_CHK_INV_MASK));

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_82328_control_prbs_polynomial_set
 * Purpose:
 *      Get the current operating mode of the PHY.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      polynomial - PRBS Polynomial
 *       0==prbs7, 
 *       1==prbs9, 
 *       2==prbs11, 
 *       3==prbs=15 
 *       4==prbs=23 
 *       5==prbs=31 
 *       6==prbs=58 
 * Returns:
 *      SOC_E_NONE
 */
STATIC int32
_phy_82328_control_prbs_polynomial_set(int32 unit, soc_port_t port, int32 polynomial)
{
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
 
    SOC_DEBUG_PRINT((DK_PHY, "82328  prbs polynomial set: u=%d p=%d polynomial=%d\n",
                                  unit, port, polynomial));
 
    if (polynomial < 0 || polynomial > 6) {
        SOC_DEBUG_PRINT((DK_ERR, "82328  prbs invalid polynomial: u=%d p=%d polynomial=%d\n",
                                unit, port, polynomial));
        return SOC_E_PARAM;
    } 
        
    /* Prbs Polynomial in TLB_TX_PRBS_GEN_CONFIG 1.D0E1.[3:1] */
    /*Set Gen Poly*/
    SOC_IF_ERROR_RETURN(
        MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, TLB_TX_PRBS_GEN_CONFIG,
                (polynomial << TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_MODE_SEL_SHIFT),
                TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_MODE_SEL_MASK));
		
    /*Prbs Polynomial in TLB_RX_PRBS_CHK_CONFIG 1.D0D1.[3:1]*/
    /*Set Checker Poly*/
    SOC_IF_ERROR_RETURN(
         MODIFY_PHY82328_MMF_PMA_PMD_REG(unit, pc, TLB_RX_PRBS_CHK_CONFIG,
           (polynomial << TLB_RX_PRBS_CHK_CONFIG_PRBS_CHK_MODE_SEL_SHIFT),
           TLB_RX_PRBS_CHK_CONFIG_PRBS_CHK_MODE_SEL_MASK));

    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_82328_control_set
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
STATIC int32
_phy_82328_control_set(int32 unit, soc_port_t port, int32 intf, int32 lane,
                     soc_phy_control_t type, uint32 value)
{
    phy_ctrl_t    *pc;       /* PHY software state */
    int32 rv = SOC_E_UNAVAIL;
 
    pc = EXT_PHY_SW_STATE(unit, port);
 
    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return SOC_E_PARAM;
    }
	
    if (intf == PHY_DIAG_INTF_SYS) {
        PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_SYS);				
    }

    switch (type) {
        case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
            rv = _phy_82328_control_prbs_polynomial_set(unit, port, value);					
        break;
        case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
            rv = _phy_82328_control_prbs_invert_data_set(unit, port, value);					
        break;
        case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
        /* fall through */
        case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
            rv = _phy_82328_control_prbs_enable_set(unit, port, value);
        break;
        case SOC_PHY_CONTROL_LOOPBACK_REMOTE:
            rv = _phy_82328_remote_loopback_set(unit, port, value);
        break;
        case SOC_PHY_CONTROL_TX_LANE_SQUELCH:
		    /*Squelch Tx Line Side*/
            rv =_phy_82328_tx_squelch(unit,port,PHY82328_INTF_SIDE_LINE,value);
        break;
        case SOC_PHY_CONTROL_PORT_PRIMARY:
            rv = soc_phyctrl_primary_set(unit, port, (soc_port_t)value);
        break;
        case SOC_PHY_CONTROL_PORT_OFFSET:
            rv = soc_phyctrl_offset_set(unit, port, (int)value);
        break;
        case SOC_PHY_CONTROL_RX_POLARITY:
            rv = _phy_82328_polarity_flip (unit, port, 
		          PHY82328_POL_DND, (uint16)value);
        break;
        case SOC_PHY_CONTROL_TX_POLARITY:
            rv = _phy_82328_polarity_flip (unit, port, 
		             (uint16)value, PHY82328_POL_DND);
        break;
        case SOC_PHY_CONTROL_DUMP:
            rv = _phy_82328_debug_info(unit, port);
        break;

        default:
            return SOC_E_PARAM;
    }
    return rv;
}
	
	
STATIC int32
phy_82328_control_set(int32 unit, soc_port_t port,
                     soc_phy_control_t type, uint32 value)
{
    SOC_IF_ERROR_RETURN(
            _phy_82328_control_set(unit, port, PHY_DIAG_INTF_LINE,
                                   PHY_DIAG_LN_DFLT, type, value));
    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_82328_control_prbs_enable_get
 * Purpose:
 *      Configure PHY device specific control fucntion.
 * Parameters:
 *      unit  - StrataSwitch unit #.
 *      port  - StrataSwitch port #. *      
 *      value - (OUT)Current setting for the PRBS Gen & Chkr
 * Returns:
 *      SOC_E_NONE
 */
STATIC int32
_phy_82328_control_prbs_enable_get(int32 unit, soc_port_t port, uint32 *value)
{
    uint16  gen_data=0,chkr_data=0;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
 
    *value = 0;
    /*Prbs Gen in TLB_TX_PRBS_GEN_CONFIG 1.D0E1.0*/
    SOC_IF_ERROR_RETURN(
            READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
			  TLB_TX_PRBS_GEN_CONFIG, &gen_data));
    gen_data &= TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_EN_MASK;
    gen_data = gen_data >> TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_EN_SHIFT;
		
    /*Prbs Checker En in TLB_RX_PRBS_CHK_CONFIG 1.D0D1.0*/     
    SOC_IF_ERROR_RETURN(
        READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
            TLB_RX_PRBS_CHK_CONFIG, &chkr_data));
    chkr_data &= TLB_RX_PRBS_CHK_CONFIG_PRBS_CHK_EN_MASK;
    chkr_data = chkr_data >> TLB_RX_PRBS_CHK_CONFIG_PRBS_CHK_EN_SHIFT;
		
    *value = gen_data && chkr_data;
 
    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_82328_control_prbs_rx_status_get
 * Purpose:
 *      Configure PHY device specific control fucntion.
 * Parameters:
 *      unit  - StrataSwitch unit #.
 *      port  - StrataSwitch port #. *      
 *      value - (OUT)PRBS Status Lock & Error Cntr 
 * Returns:
 *      SOC_E_NONE
 */
STATIC int32
_phy_82328_control_prbs_rx_status_get(int32 unit, soc_port_t port, uint32 *value)
{
    uint16  lock = 0, err_lsb = 0,err_msb = 0;
    uint32  err_cnt = 0;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
 
    *value = 0;
    /*Prbs Lock Status in TLB_RX_PRBS_CHK_LOCK_STATUS 1.D0D9.0*/
    SOC_IF_ERROR_RETURN
        (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, 
              TLB_RX_PRBS_CHK_LOCK_STATUS, &lock));
    lock &= TLB_RX_PRBS_CHK_LOCK_STATUS_PRBS_CHK_LOCK_MASK;
		
    /*Prbs Err Cnt MSB in TLB_RX_PRBS_CHK_ERR_CNT_MSB_STATUS 1.D0DA*/     
    SOC_IF_ERROR_RETURN(
         READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
              TLB_RX_PRBS_CHK_ERR_CNT_MSB_STATUS, &err_msb));

    err_msb &= TLB_RX_PRBS_CHK_ERR_CNT_MSB_STATUS_PRBS_CHK_ERR_CNT_MSB_MASK;
		
    /*Prbs Err Cnt LSB in TLB_RX_PRBS_CHK_ERR_CNT_LSB_STATUS 1.D0DB*/     
    SOC_IF_ERROR_RETURN(
         READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
               TLB_RX_PRBS_CHK_ERR_CNT_LSB_STATUS, &err_lsb));
		
	err_cnt = (err_msb << TLB_RX_PRBS_CHK_ERR_CNT_LSB_STATUS_PRBS_CHK_ERR_CNT_LSB_BITS) 
              |err_lsb;
    
    if (lock && !err_cnt) { /*Locked and No errors*/
        *value = 0;
    } else {
        if (!lock) { /*Not Locked*/	
            *value = -1;	
        } else {
            /*Locked With Errors*/
            *value = err_cnt;
        }
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_82328_control_prbs_tx_invert_data_get
 * Purpose:
 *      Configure PHY device specific control fucntion.
 * Parameters:
 *      unit  - StrataSwitch unit #.
 *      port  - StrataSwitch port #. *      
 *      value - (OUT)Current setting for the PRBS Invert
 * Returns:
 *      SOC_E_NONE
 */
STATIC int32
_phy_82328_control_prbs_tx_invert_data_get(int32 unit, soc_port_t port, uint32 *value)
{
    uint16  data=0;		
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
 
    *value = 0;
    /*Prbs Invert in TLB_TX_PRBS_GEN_CONFIG 1.D0E1.4 */ 
    SOC_IF_ERROR_RETURN
        (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
              TLB_TX_PRBS_GEN_CONFIG, &data));

    data &= TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_INV_MASK;

    *value = data >> TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_INV_SHIFT;		
 
    return SOC_E_NONE;
}

/*
 * Function:
 *      _phy_82328_control_prbs_polynomial_get
 * Purpose:
 *      Configure PHY device specific control fucntion.
 * Parameters:
 *      unit  - StrataSwitch unit #.
 *      port  - StrataSwitch port #. *      
 *      value - (OUT)Current setting for the PRBS Polynomial
 *       0=prbs7, 
 *       1=prbs9, 
 *       2=prbs11, 
 *       3=prbs=15 
 *       4=prbs=23 
 *       5=prbs=31 
 *       6=prbs=58 
 * Returns:
 *      SOC_E_NONE
 */
STATIC int32
_phy_82328_control_prbs_polynomial_get(int32 unit, soc_port_t port, uint32 *value)
{
    uint16  data;
    phy_ctrl_t  *pc = EXT_PHY_SW_STATE(unit, port);
 
    *value = 0;
    /* Prbs Polynomial in TLB_TX_PRBS_GEN_CONFIG 1.D0E1.[3:1] */        
    SOC_IF_ERROR_RETURN
        (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
             TLB_TX_PRBS_GEN_CONFIG, &data));

    data &= TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_MODE_SEL_MASK;
    *value = data >> TLB_TX_PRBS_GEN_CONFIG_PRBS_GEN_MODE_SEL_SHIFT;

    return SOC_E_NONE;
}

STATIC int
_phy_82328_debug_info(int unit, soc_port_t port)
{
    phy_ctrl_t			*pc;     
    phy_ctrl_t			*int_pc; 
    phy82328_intf_cfg_t *intf;
    phy82328_intf_cfg_t *line_intf;
    phy82328_intf_cfg_t *sys_intf;
    phy82328_intf_side_t side, saved_side;
    soc_port_if_t		iif;
    int					lane;
    int					ian, ian_done, ilink, ispeed;
    uint16				link_sts[PHY82328_NUM_LANES];
    uint16				pol_sts[PHY82328_NUM_LANES];
    uint16				data0, data1, data2, data3, data4;
    uint32				primary_port, offset;

    pc = EXT_PHY_SW_STATE(unit, port);
    int_pc = INT_PHY_SW_STATE(unit, port);
	
    SOC_IF_ERROR_RETURN(phy_82328_control_get(unit, port, SOC_PHY_CONTROL_PORT_PRIMARY, 
                                             &primary_port));

     SOC_IF_ERROR_RETURN(phy_82328_control_get(unit, port, SOC_PHY_CONTROL_PORT_OFFSET,
                                              &offset));

	/* Access line side registers */
    saved_side = _phy_82328_intf_side_regs_get(unit, port);
    if (saved_side == PHY82328_INTF_SIDE_SYS) {
        PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_LINE);
    }

    /* firware rev: 1.c161  */
    SOC_IF_ERROR_RETURN
        (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                          0xc161, &data0));

    /* micro enabled: 1.ca18 */
    SOC_IF_ERROR_RETURN
        (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                          MGT_TOP_GP_REG_0, &data1));
	/* Link sts 1.c0b0*/
    SOC_IF_ERROR_RETURN
        (READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                          RX_ANARXSTATUS, &data2));

    soc_cm_print("Port %-2d: devid=%d, chiprev=%04x, micro ver(1.%04x)=%04x, micro(1.%04x)=%04x, \n"
                 "     pri:offs=%d:%d, mdio=0x%x, datapath=%d, Link (1.%04x)=%04x\n",
                 port, DEVID(pc), DEVREV(pc), XGXS_BLK8_CL73CONTROL8, data0,
				 MGT_TOP_GP_REG_0, data1,
				 primary_port, offset, pc->phy_id, 
				 CUR_DATAPATH(pc) == PHY82328_DATAPATH_20 ? 20 : 4,
				 RX_ANARXSTATUS, data2);

    /* Single PMD control: 1.ca86 */
    SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                MGT_TOP_SINGLE_PMD_CTRL, &data0));

    /* Broadcast control: 1.c8fe */
    SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                MGT_TOP_BROADCAST_CTRL, &data1));

    /* Global intf type/speed - driver csr: 1.c841 */
    SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                               MGT_TOP_GP_REGISTER_1, &data2));

    /* Micro csr 1.c843 */
    SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                               MGT_TOP_GP_REGISTER_3, &data3));

    soc_cm_print("     PMD(1.%04x)=%04x, bcctrl(1.%04x)=%04x, \n"
				 "     drvcsr(1.%04x)=%04x, ucsr(1.%04x)=%04x\n",
                 MGT_TOP_SINGLE_PMD_CTRL , data0,
                 MGT_TOP_BROADCAST_CTRL, data1, 
                 MGT_TOP_GP_REGISTER_1, data2,
                 MGT_TOP_GP_REGISTER_3, data3);

    /* Interfaces: software, hardware, internal serdes */
    line_intf = &(LINE_INTF(pc));
    sys_intf = &(SYS_INTF(pc));
    for (side = PHY82328_INTF_SIDE_LINE; side <= PHY82328_INTF_SIDE_SYS; side++) {

        PHY_82328_REGS_SELECT(unit, pc, side);

        /* control: 1.0000 */
        SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                           IEEE0_BLK_PMA_PMD_PMD_CONTROL_REGISTER, &data1));
		
        /* PMD type: 1.0007 */
        SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                         IEEE0_BLK_PMA_PMD_PMD_CONTROL_2_REGISTER, &data2));

        /* Loopback 1.d0e2 */
        SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, 
                                           TLB_TX_RMT_LPBK_CONFIG, &data3));

        /* Tx disable 1.c08b */
        SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, TX_ANATXACONTROL7, 
                                           &data4));
        if (PHY82328_SINGLE_PORT_MODE(pc)) {
            if (side == PHY82328_INTF_SIDE_SYS) {
                PHY_82328_MICRO_PAUSE(unit, port, "");
            }
            for (lane = 0; lane < PHY82328_NUM_LANES; lane++) {
                
                /* Select the lane and side to write */
                SOC_IF_ERROR_RETURN(_phy_82328_channel_select(unit, port, side, lane));

                /* Link: 1.c0b0 */
                SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                    RX_ANARXSTATUS, &(link_sts[lane])));
             
                if (CUR_DATAPATH(pc) == PHY82328_DATAPATH_20) {
                    /* Polarity setting: 1.d0e3 */
                    SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                       TLB_TX_TLB_TX_MISC_CONFIG, &(pol_sts[lane])));
                } else {
                    /* Polarity setting: 1.d0a0 */
                    SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                        AMS_TX_TX_CTRL_0, &(pol_sts[lane])));
                }
            }
            /* Back to default channel selection through lane 0 */
            SOC_IF_ERROR_RETURN(_phy_82328_channel_select(unit, port, side, PHY82328_ALL_LANES));
            PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_LINE);
            if (side == PHY82328_INTF_SIDE_SYS) {
                PHY_82328_MICRO_RESUME(unit, port);
            }
        } else {
            /* Link: 1.c0b0 */
            SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                RX_ANARXSTATUS, &(link_sts[0])));
            if (CUR_DATAPATH(pc) == PHY82328_DATAPATH_20) {
                /* Polarity setting: 1.doe3 */
                SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                    TLB_TX_TLB_TX_MISC_CONFIG, 
                                    &(pol_sts[0])));
            } else {
                /* Polarity setting: 1.d0a0 */
                SOC_IF_ERROR_RETURN(READ_PHY82328_MMF_PMA_PMD_REG(unit, pc, 
                                    AMS_TX_TX_CTRL_0, 
                                    &(pol_sts[0])));
            }
        }
        intf = (side == PHY82328_INTF_SIDE_LINE) ? line_intf : sys_intf;			
        if (PHY82328_SINGLE_PORT_MODE(pc)) {
            soc_cm_print(
				" %s: type=%s, speed=%d, forced cl72=%d, mode(1.%04x)=%04x\n"
				"           pmdtype(1.%04x)=%04x, lb(1.%04x)=%04x, txdis(1.%04x)=%04x\n"
				"           link(1.%04x)      : (ln0=%04x, ln1=%04x, ln2=%04x, ln3=%04x)\n"
				"           pol(1.%04x)       : (ln0=%04x, ln1=%04x, ln2=%04x, ln3=%04x)\n",
				(side == PHY82328_INTF_SIDE_LINE) ? "Line    " : "System  ",
				phy82328_intf_names[intf->type], intf->speed, SYS_FORCED_CL72(pc),
				IEEE0_BLK_PMA_PMD_PMD_CONTROL_REGISTER, data1,
				IEEE0_BLK_PMA_PMD_PMD_CONTROL_2_REGISTER, data2,
				TLB_TX_RMT_LPBK_CONFIG, data3,
				TX_ANATXACONTROL7, data4,
				RX_ANARXSTATUS, link_sts[0], link_sts[1], link_sts[2], link_sts[3],
				CUR_DATAPATH(pc) == PHY82328_DATAPATH_20 ? TLB_TX_TLB_TX_MISC_CONFIG : AMS_TX_TX_CTRL_0,
				pol_sts[0], pol_sts[1], pol_sts[2], pol_sts[3]);
        } else {
            soc_cm_print(
				" %s: type=%s, speed=%d, forced cl72=%d, mode(1.%04x)=%04x\n"
				"           pmdtype(1.%04x)=%04x, lb(1.%04x)=%04x, txdis(1.%04x)=%04x\n"
				"           link(1.%04x)=%04x, pol(1.%04x)=%04x", 
				(side == PHY82328_INTF_SIDE_LINE) ? "Line    " : "System  ",
				phy82328_intf_names[intf->type], intf->speed, SYS_FORCED_CL72(pc),
				IEEE0_BLK_PMA_PMD_PMD_CONTROL_REGISTER, data1,
				IEEE0_BLK_PMA_PMD_PMD_CONTROL_2_REGISTER, data2,
				TLB_TX_RMT_LPBK_CONFIG, data3,
				TX_ANATXACONTROL7, data4,
				RX_ANARXSTATUS, link_sts[0],
				CUR_DATAPATH(pc) == PHY82328_DATAPATH_20 ? TLB_TX_TLB_TX_MISC_CONFIG : AMS_TX_TX_CTRL_0,
				pol_sts[0]);
            }
        }
    PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_LINE);
    if (int_pc) {
        int lb;
        SOC_IF_ERROR_RETURN(PHY_INTERFACE_GET(int_pc->pd, unit, port, &iif));
        SOC_IF_ERROR_RETURN(PHY_LINK_GET(int_pc->pd, unit, port, &ilink));
        SOC_IF_ERROR_RETURN(phy_82328_lb_get(unit, port, &lb));
        if (lb) {
            /* Clear the sticky bit if in loopback */
            SOC_IF_ERROR_RETURN(PHY_LINK_GET(int_pc->pd, unit, port, &ilink));
        }
        SOC_IF_ERROR_RETURN(PHY_SPEED_GET(int_pc->pd, unit, port, &ispeed));
        SOC_IF_ERROR_RETURN(PHY_AUTO_NEGOTIATE_GET(int_pc->pd, unit, port, &ian, &ian_done));
        soc_cm_print(" Internal: type=%s, speed=%d, link=%d, an=%d\n===\n", 
					 phy82328_intf_names[iif], ispeed, ilink, ian);
    }
    if (saved_side == PHY82328_INTF_SIDE_SYS) {
        PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_SYS);
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_82328_control_get
 * Purpose:
 *      Get current control settign of the PHY.
 * Parameters:
 *      unit  - StrataSwitch unit #.
 *      port  - StrataSwitch port #.
 *      type  - Control to get
 *      value - (OUT)Current setting for the control
 * Returns:
 *      SOC_E_NONE
 */
STATIC int32
_phy_82328_control_get(int32 unit, soc_port_t port, int32 intf, int32 lane,
                     soc_phy_control_t type, uint32 *value)
{
    phy_ctrl_t *pc;
    soc_port_t offset, primary;
    int32 rv = SOC_E_UNAVAIL;    
    pc = EXT_PHY_SW_STATE(unit, port);
 
    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return SOC_E_PARAM;
    }
	
    if (intf == PHY_DIAG_INTF_SYS) {
       PHY_82328_REGS_SELECT(unit, pc, PHY82328_INTF_SIDE_SYS);
    }
    rv = SOC_E_UNAVAIL;
    switch(type) {
        case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
            rv = _phy_82328_control_prbs_polynomial_get(unit, port, value);
        break;
        case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
            rv = _phy_82328_control_prbs_tx_invert_data_get(unit, port, value);
        break;
        case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
           /* fall through */
        case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
            rv = _phy_82328_control_prbs_enable_get(unit, port, value);
        break;
        case SOC_PHY_CONTROL_PRBS_RX_STATUS:
            rv = _phy_82328_control_prbs_rx_status_get(unit, port, value);
        break;
        case SOC_PHY_CONTROL_LOOPBACK_REMOTE:
            rv = _phy_82328_remote_loopback_get(unit, port, value);
        break;
        case SOC_PHY_CONTROL_PORT_PRIMARY:
            SOC_IF_ERROR_RETURN
              (soc_phyctrl_primary_get(unit, port, &primary));
            *value = (uint32) primary;
            rv = SOC_E_NONE;
        break;
        case SOC_PHY_CONTROL_PORT_OFFSET:
            SOC_IF_ERROR_RETURN
              (soc_phyctrl_offset_get(unit, port, &offset));
            *value = (uint32) offset;
		    rv = SOC_E_NONE;
        break;
        
        default:
            rv = SOC_E_UNAVAIL;
        break;
	}

    return rv;
}

STATIC int32
phy_82328_control_get(int32 unit, soc_port_t port,
                     soc_phy_control_t type, uint32 *value)
{
    SOC_IF_ERROR_RETURN
        (_phy_82328_control_get (unit, port, 
                    PHY_DIAG_INTF_LINE, PHY_DIAG_LN_DFLT, type, value));
 
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_82328_reg_read
 * Purpose:
 *      Routine to read PHY register
 * Parameters:
 *      uint         - BCM unit number
 *      port         - port number
 *      flags        - Flags which specify the register type
 *      phy_reg_addr - Encoded register address
 *      phy_data     - (OUT) Value read from PHY register
 * Note:
 *      This register read function is not thread safe. Higher level
 * function that calls this function must obtain a per port lock
 * to avoid overriding register page mapping between threads.
 */
STATIC int32
phy_82328_reg_read(int32 unit, soc_port_t port, uint32 flags,
                  uint32 phy_reg_addr, uint32 *phy_data)
{
    uint16               data16;
    phy_ctrl_t           *pc;      /* PHY software state */
    int32                rv = SOC_E_NONE;

    pc = EXT_PHY_SW_STATE(unit, port);

    if (flags & SOC_PHY_I2C_DATA8) {
    } else if (flags & SOC_PHY_I2C_DATA16) {
    } else {
        SOC_IF_ERROR_RETURN
            (READ_PHY_REG(unit, pc, phy_reg_addr, &data16));
        *phy_data = data16;
    }
    return rv;
}

/*
 * Function:
 *      phy_82328_reg_write
 * Purpose:
 *      Routine to write PHY register
 * Parameters:
 *      uint         - BCM unit number
 *      port         - port number
 *      flags        - Flags which specify the register type
 *      phy_reg_addr - Encoded register address
 *      phy_data     - Value write to PHY register
 * Note:
 *      This register read function is not thread safe. Higher level
 * function that calls this function must obtain a per port lock
 * to avoid overriding register page mapping between threads.
 */
STATIC int32
phy_82328_reg_write(int32 unit, soc_port_t port, uint32 flags,
                   uint32 phy_reg_addr, uint32 phy_data)
{
    phy_ctrl_t          *pc;      /* PHY software state */
    int32 rv = SOC_E_NONE;

    pc = EXT_PHY_SW_STATE(unit, port);
 
    if (flags & SOC_PHY_I2C_DATA8) {
    } else if (flags & SOC_PHY_I2C_DATA16) {
    } else {
        SOC_IF_ERROR_RETURN
            (WRITE_PHY_REG (unit, pc, phy_reg_addr, 
                                      (uint16)phy_data));
    }
    return rv;
}

/*
 * Function:
 *      phy_82328_probe
 * Purpose:
 *      Complement the generic phy probe routine to identify this phy when its
 *      phy id0 and id1 is same as some other phy's.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      pc   - phy ctrl descriptor.
 * Returns:
 *      SOC_E_NONE,SOC_E_NOT_FOUND and SOC_E_<error>
 */
STATIC int32
phy_82328_probe(int32 unit, phy_ctrl_t *pc)
{
    uint32 devid;
    uint16 chip_rev;
    SOC_IF_ERROR_RETURN(_phy_82328_config_devid(pc->unit, pc->port, pc, &devid));
    SOC_IF_ERROR_RETURN(
        READ_PHY82328_MMF_PMA_PMD_REG(unit, pc,
                                      MGT_TOP_CHIP_REV_REGISTER,
                                      &chip_rev));
    if (devid == PHY82328_ID_82328) {
        if (chip_rev == 0x00a0) {
            SOC_DEBUG_PRINT((DK_PHY, "PHY82328 Dev found"));
            pc->dev_name = dev_name_82328_a0;
        }
    } else {  /* not found */
        SOC_DEBUG_PRINT((DK_WARN, "port %d: BCM84xxx type PHY device detected, please use "
                         "phy_84<xxx> config variable to select the specific type\n",
                             pc->port));
        return SOC_E_NOT_FOUND;
    }
    pc->size = sizeof(phy82328_dev_desc_t);
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_82328_ability_advert_set
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
 
STATIC int32
phy_82328_ability_advert_set(int32 unit, soc_port_t port,
                              soc_port_ability_t *ability)
{
    uint16 pause;
    phy_ctrl_t  *pc, *int_pc;
    phy82328_intf_cfg_t *line_intf;

    SOC_DEBUG_PRINT((DK_PHY, "82328 Ablity advert Set\n"));
    if (ability == NULL) {
        return SOC_E_PARAM;
    }
	pc = EXT_PHY_SW_STATE(unit, port);
    
    line_intf = &(LINE_INTF(pc));
    if ((line_intf->type == SOC_PORT_IF_KX) || (line_intf->type == SOC_PORT_IF_GMII)) {
        int_pc = INT_PHY_SW_STATE(unit, port);
        if (int_pc) {
            SOC_IF_ERROR_RETURN(PHY_ABILITY_ADVERT_SET(int_pc->pd, unit, port, ability));
        }
        return SOC_E_NONE;
    }
	
    switch (ability->pause & (SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX)) {
        case SOC_PA_PAUSE_TX: /*7.0x0010.10 1*/
            pause = (1<<QSFI_AN_IEEE1_AN_ADVERTISEMENT_1_REGISTER_PAUSE_SHIFT);
		break;
        case SOC_PA_PAUSE_RX: /*7.0x0010.11:10 1*/
            pause = (1<<(QSFI_AN_IEEE1_AN_ADVERTISEMENT_1_REGISTER_PAUSE_SHIFT+1)) |
		            (1<<QSFI_AN_IEEE1_AN_ADVERTISEMENT_1_REGISTER_PAUSE_SHIFT);
        break;
        case SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX: /*7.0x0010.11 1*/
            pause = (1<<(QSFI_AN_IEEE1_AN_ADVERTISEMENT_1_REGISTER_PAUSE_SHIFT+1));
        break;
        default:
            pause = 0;
        break;
    }

    SOC_IF_ERROR_RETURN
        (MODIFY_PHY82328_MMF_AN_REG(unit, pc, 
         IEEE1_AN_BLK_AN_ADVERTISEMENT_1_REGISTER,
         pause, QSFI_AN_IEEE1_AN_ADVERTISEMENT_1_REGISTER_PAUSE_MASK)); 
    SOC_DEBUG_PRINT((DK_PHY,
                    "phy_82328_ability_advert_set: u=%d p=%d speed(FD)=%x pause=0x%x\n",
                    unit, port, ability->speed_full_duplex, ability->pause));
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_82328_ability_advert_get
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
 
STATIC int32
phy_82328_ability_advert_get(int32 unit, soc_port_t port,
                           soc_port_ability_t *ability)
{
    phy_ctrl_t  *pc, *int_pc;
    uint16 pause_adv;
    phy82328_intf_cfg_t *line_intf;

    soc_port_mode_t mode = 0;        
    if (ability == NULL) {
        return SOC_E_PARAM;
    } 
    sal_memset(ability, 0, sizeof(soc_port_ability_t)); 
    pc = EXT_PHY_SW_STATE(unit, port);
    int_pc = INT_PHY_SW_STATE(unit, port);
    line_intf = &(LINE_INTF(pc));

    if ((line_intf->type == SOC_PORT_IF_KX) || (line_intf->type == SOC_PORT_IF_GMII)) {
        int_pc = INT_PHY_SW_STATE(unit, port);
        if (int_pc) {
            SOC_IF_ERROR_RETURN(PHY_ABILITY_ADVERT_GET(int_pc->pd, unit, port, ability));
        }
        return SOC_E_NONE;
    }

    ability->speed_full_duplex = PHY82328_SINGLE_PORT_MODE(pc) ? SOC_PA_SPEED_40GB : SOC_PA_SPEED_10GB;
    SOC_IF_ERROR_RETURN
        (READ_PHY82328_MMF_AN_REG
         (unit, pc, IEEE1_AN_BLK_AN_ADVERTISEMENT_1_REGISTER, &pause_adv)); 
    mode = 0;
    
    switch (pause_adv & QSFI_AN_IEEE1_AN_ADVERTISEMENT_1_REGISTER_PAUSE_MASK) {		
        case (1<<QSFI_AN_IEEE1_AN_ADVERTISEMENT_1_REGISTER_PAUSE_SHIFT):  /*7.0x0010.10 1*/
			mode = SOC_PA_PAUSE_TX;		
        break;
        case (1<<(QSFI_AN_IEEE1_AN_ADVERTISEMENT_1_REGISTER_PAUSE_SHIFT+1)): /*7.0x0010.11 1*/
			mode = SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX;
        break;		
        case (1<<(QSFI_AN_IEEE1_AN_ADVERTISEMENT_1_REGISTER_PAUSE_SHIFT+1)) |
				(1<<QSFI_AN_IEEE1_AN_ADVERTISEMENT_1_REGISTER_PAUSE_SHIFT):/*7.0x0010.11:10 1*/
            mode = SOC_PA_PAUSE_RX;
        break;		
        default:
            mode = 0;
        break;
    }
    ability->pause = mode;	 
    SOC_DEBUG_PRINT((DK_PHY,
        "phy_82328_ability_advert_get: u=%d p=%d speed(FD)=0x%x pause=0x%x\n",
        unit, port, ability->speed_full_duplex, ability->pause));
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_82328_ability_local_get
 * Purpose:
 *      Get the device's complete abilities.
 * Parameters:
 *      unit - StrataSwitch unit #.
 *      port - StrataSwitch port #.
 *      ability - return device's abilities.
 * Returns:
 *      SOC_E_XXX
 */
 
STATIC int32
phy_82328_ability_local_get(int32 unit, soc_port_t port, soc_port_ability_t *ability)
{
    uint32		pa_speed = 0;
    phy_ctrl_t  *pc;

    pc = EXT_PHY_SW_STATE(unit, port);
	
    if (ability == NULL) {
        return SOC_E_PARAM;
    }
    sal_memset(ability, 0, sizeof(soc_port_ability_t));
    pa_speed = PHY82328_SINGLE_PORT_MODE(pc) ? (SOC_PA_SPEED_40GB|SOC_PA_SPEED_42GB) :
               (SOC_PA_SPEED_10GB | SOC_PA_SPEED_1000MB | SOC_PA_SPEED_100MB | SOC_PA_SPEED_10MB);
    ability->speed_full_duplex = pa_speed;
    ability->speed_half_duplex = SOC_PA_ABILITY_NONE;
    ability->pause = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
    ability->interface = SOC_PA_INTF_XGMII;
    ability->medium    = SOC_PA_MEDIUM_FIBER;
    ability->loopback  = SOC_PA_LB_PHY;

    SOC_DEBUG_PRINT((DK_PHY,
        "phy_82328_ability_local_get: u=%d p=%d speed=0x%x\n",
        unit, port, ability->speed_full_duplex));
                                                                               
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_82328_firmware_set
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
 
STATIC int32
phy_82328_firmware_set(int32 unit, int32 port, int32 offset, uint8 *array,int32 datalen)
{
    /* overload this function a littl bit if array == NULL 
     * special handling for init. uCode broadcast. Internal use only 
     */
    if (array == NULL) {
        return _phy_82328_rom_fw_dload (unit, port, offset, 
                            phy82328_ucode_bin, phy82328_ucode_bin_len);
    } else {
        SOC_DEBUG_PRINT((DK_WARN, "u=%d p=%d firmware download method not supported\n",
                         unit, port));
        return SOC_E_UNAVAIL;
    }

    return SOC_E_NONE;
}
STATIC int
phy_82328_diag_ctrl(int unit, soc_port_t port, uint32 inst, int op_type,  int op_cmd,  
					void *arg)   
{
    int lane;
    int intf;
    int rv = SOC_E_NONE;

    lane = PHY_DIAG_INST_LN(inst);
    intf = PHY_DIAG_INST_INTF(inst);
    if (intf == PHY_DIAG_INTF_DFLT) {
        intf = PHY_DIAG_INTF_LINE;
    }

    switch (op_cmd) {
	    case PHY_DIAG_CTRL_DSC:
		    rv = SOC_E_UNAVAIL;
        break;
        case PHY_DIAG_CTRL_EYE_ENABLE_LIVELINK:
        case PHY_DIAG_CTRL_EYE_DISABLE_LIVELINK:
        case PHY_DIAG_CTRL_EYE_CLEAR_LIVELINK:
        case PHY_DIAG_CTRL_EYE_START_LIVELINK:
        case PHY_DIAG_CTRL_EYE_STOP_LIVELINK:
        case PHY_DIAG_CTRL_EYE_READ_LIVELINK:
        case PHY_DIAG_CTRL_EYE_GET_INIT_VOFFSET:
        case PHY_DIAG_CTRL_EYE_SET_HOFFSET:
        case PHY_DIAG_CTRL_EYE_SET_VOFFSET:
        case PHY_DIAG_CTRL_EYE_GET_MAX_LEFT_HOFFSET:
        case PHY_DIAG_CTRL_EYE_GET_MAX_RIGHT_HOFFSET:
        case PHY_DIAG_CTRL_EYE_GET_MIN_VOFFSET:
        case PHY_DIAG_CTRL_EYE_GET_MAX_VOFFSET:
            rv = SOC_E_UNAVAIL;
        break;
        case PHY_DIAG_CTRL_EYE_MARGIN_VEYE:
        case PHY_DIAG_CTRL_EYE_MARGIN_HEYE_LEFT:
        case PHY_DIAG_CTRL_EYE_MARGIN_HEYE_RIGHT:
            rv = SOC_E_UNAVAIL;
        break;
        default:
           if (op_type == PHY_DIAG_CTRL_SET) {
               soc_cm_print("PHY82328 Diagctrl Command:0x%x\n",op_cmd);                
               SOC_IF_ERROR_RETURN
                   (_phy_82328_control_set(unit, port, intf, lane, op_cmd, PTR_TO_INT(arg)));
           } else if (op_type == PHY_DIAG_CTRL_GET) {
               SOC_IF_ERROR_RETURN
                   (_phy_82328_control_get(unit, port, intf, lane, op_cmd, (uint32 *)arg));
           } else {
               SOC_DEBUG_PRINT((DK_ERR, "82328 diag_ctrl bad operation: u=%d p=%d ctrl=0x%x\n", 
                    unit, port, op_cmd));
               rv = SOC_E_UNAVAIL;
        }
        break;
    }
    return rv;
}
/*
 * Variable:   phy_82328drv_xe
 * Purpose:    Phy driver callouts for a  phy 82328
 * Notes:     
 */
phy_driver_t phy_82328drv_xe = {
    "82328 40-Gigabit PHY Driver",
    phy_82328_init,              /* phy_init                     */
    phy_null_reset,              /* phy_reset                    */
    phy_82328_link_get,          /* phy_link_get                 */
    phy_82328_enable_set,        /* phy_enable_set               */
    phy_null_enable_get,         /* phy_enable_get               */
    phy_null_duplex_set,         /* phy_duplex_set               */
    phy_null_duplex_get,         /* phy_duplex_get               */
    phy_82328_speed_set,         /* phy_speed_set                */
    phy_82328_speed_get,         /* phy_speed_get                */
    phy_null_set,                /* phy_master_set               */
    phy_null_zero_get,           /* phy_master_get               */
    phy_82328_an_set,            /* phy_an_set                   */
    phy_82328_an_get,            /* phy_an_get                   */
    phy_null_mode_set,           /* phy_adv_local_set            */
    phy_null_mode_get,           /* phy_adv_local_get            */
    phy_null_mode_get,           /* phy_adv_remote_get           */
    phy_82328_lb_set,            /* phy_lb_set                   */
    phy_82328_lb_get,            /* phy_lb_get                   */
    phy_82328_interface_set,     /* phy_interface_set            */
    phy_82328_interface_get,      /* phy_interface_get            */
    phy_null_mode_get,           /* phy_ability                  */
    NULL,                        /* phy_linkup_evt               */
    NULL,                        /* phy_linkdn_evt               */
    phy_null_mdix_set,           /* phy_mdix_set                 */
    phy_null_mdix_get,           /* phy_mdix_get                 */
    phy_null_mdix_status_get,    /* phy_mdix_status_get          */
    NULL,                        /* phy_medium_config_set        */
    NULL,                        /* phy_medium_config_get        */
    phy_null_medium_get,         /* phy_medium_get               */
    NULL,                        /* phy_cable_diag               */
    NULL,                        /* phy_link_change              */
    phy_82328_control_set,       /* phy_control_set              */
    phy_82328_control_get,        /* phy_control_get              */
    phy_82328_reg_read,          /* phy_reg_read                 */
    phy_82328_reg_write,         /* phy_reg_write                */
    NULL,                        /* phy_reg_modify               */
    NULL,                        /* phy_notify                   */
    phy_82328_probe,             /* phy_probe                    */
    phy_82328_ability_advert_set,/* phy_ability_advert_set       */
    phy_82328_ability_advert_get,/* phy_ability_advert_get       */
    NULL,                        /* phy_ability_remote_get       */
    phy_82328_ability_local_get, /* phy_ability_local_get        */
    phy_82328_firmware_set,      /* phy_firmware_set             */
    NULL,                        /* phy_timesync_config_set      */
    NULL,                        /* phy_timesync_config_get      */
    NULL,                        /* phy_timesync_control_set     */
    NULL,                        /* phy_timesync_control_get     */
    phy_82328_diag_ctrl          /* phy_diag_ctrl                */
};
#else /* INCLUDE_PHY_82328 */
int _phy_82328_not_empty;
#endif /* INCLUDE_PHY_82328 */ 
