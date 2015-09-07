/*
 * $Id: tsce.c,v 1.1.2.25 Broadcom SDK $
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
 * File:        tsce.c
 * Purpose:     Support Broadcom TSC/Eagle internal SerDes
 * 
 */

/*
 *   This module implements an NTSW SDK Phy driver for the TSC/Eagle Serdes.
 *  
 *   LAYERING.
 *
 *   This driver is built on top of the PHYMOD library, which is a PHY
 *   driver library that can operate on a family of PHYs, including
 *   TSC/Eagle.  PHYMOD can be built in a standalone enviroment and be
 *   provided independently from the SDK.  PHYMOD APIs consist of
 *   "core" APIs and "phy" APIs.
 *
 *
 *   KEY IDEAS AND MAIN FUNCTIONS
 *
 *   Some key ideas in this architecture are:
 *
 *   o A PHMOD "phy" consists of one more more lanes, all residing in a single
 *     core.  An SDK "phy" is a logical port, which can consist of one or
 *     more lanes in a single core, OR, can consist of multiples lanes in
 *     more than one core.
 *   o PHYMOD code is "stateless" in the sense that all calls to PHYMOD
 *     APIs are fully parameterized and no per-phy state is held by a PHYMOD
 *     driver.
 *   o PHYMOD APIs do not map 1-1 with SDK .pd_control_set or .pd_control_get
 *     APIs (nor ".pd_xx" calls, nor to .phy_tsce_per_lane_control_set and
 *     .phy_tsce_per_lane_control_get APIs.
 *
 *   The purpose of this code, then, is to provide the necessary translations
 *   between the SDK interfaces and the PHYMOD interfaces.  This includes:
 * 
 *   o Looping over the cores in a logical PHY in order to operate on the
 *     supporting PHMOD PHYs
 *   o Determining the configuration data for the phy, based on programmed
 *     defaults and SOC properties.
 *   o Managing the allocation and freeing of phy data structures required for
 *     working storage.  Locating these structures on procedure invocation.
 *   o Mapping SDK API concepts to PHYMOD APIs.  In some cases, local state is
 *     required in this module in order to present the legacy API.
 *
 * 
 *   MAIN DATA STRUCTURES
 * 
 *   phy_ctrl_t
 *   The PHY control structure defines the SDK notion of a PHY
 *   (existing) structure owned by SDK phy driver modules.  In order
 *   to support PHYMOD PHYs, one addition was made to this structure
 *   (soc_phymod_ctrl_t phymod_ctrl;)
 *
 *   tsce_config_t
 *   Driver specific data.  This structure is used by tsce to hold
 *   logical port specific working storage.
 *
 *   soc_phymod_ctrl_t
 *   PHYMOD drivers.  Resides in phy_ctrl_t specifies how many cores
 *   are in this phy and contains an array of pointers to
 *   soc_phymod_phy_t structures, which define a PHYMOD PHY.
 *
 *   soc_phymod_phy_t
 *   This structure contains a pointer to phymod_phy_access_t and a
 *   pointer to the associated soc_phymod_core_t.  Instances if this
 *   structure are created during device probe/init and are maintained
 *   by the SOC.
 *
 *   soc_phymod_core_t
 *   This structure contains per-core information.  Multiple PHYMOD
 *   PHYS can point to a single core.
 *
 *   phymod_phy_access_t
 *   This structure contains information about how to read/write PHY
 *   registers.  A required parameter for PHYMOD PHY APIs
 * 
 *   phymod_core_access_t
 *   This structure contains information about how to read/write PHY
 *   registers.  A required parameter for PHYMOD core APIs.
 *
 */

#include <shared/bsl.h>

#include <sal/types.h>
#include <sal/types.h>
#include <sal/core/spl.h>
#include <shared/bsl.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/phyreg.h>
#include <soc/port_ability.h>
#include <soc/phy.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>
#include <soc/phy/phymod_ids.h>

#include "phydefs.h"      /* Must include before other phy related includes */ 

#include "phyconfig.h"     /* Must include before other phy related includes */
#include "phyident.h"
#include "phyreg.h"
#include "phynull.h"
#include "xgxs.h"
#include "serdesid.h"



int (*_phy_tsce_firmware_set_helper[SOC_MAX_NUM_DEVICES])(int, int, uint8 *, int) = { NULL };

#if defined(INCLUDE_XGXS_TSCE)

#include <phymod/phymod.h>
#include <phymod/phymod_debug.h>
#include <phymod/phymod_diagnostics.h>
#include <phymod/chip/bcmi_tsce_xgxs_sym.h>
#include <phymod/chip/tsce.h>

#include "../../../libs/phymod/chip/tsce/tier1/temod_device.h"

#define NUM_LANES             4    /* num of lanes per core */
#define MAX_NUM_LANES         4    /* max num of lanes per port */
#define TSCE_NO_CFG_VALUE     (-1)


#define PHYMOD_IF_CR4    (1 << SOC_PORT_IF_CR4)

#define TSCE_40G_10G_INTF(_if)  ((_if) == SOC_PORT_IF_KR4 || (_if) == SOC_PORT_IF_XLAUI || \
                             (_if) == SOC_PORT_IF_CR4 || (_if) == SOC_PORT_IF_SR || \
                             (_if) == SOC_PORT_IF_KR || (_if) == SOC_PORT_IF_CR || \
                             (_if) == SOC_PORT_IF_XFI || (_if) == SOC_PORT_IF_SFI)

#define TSCE_40G_10G_INTF_ALL   (TSCE_IF_KR4 | TSCE_IF_XLAUI | TSCE_IF_CR4 | TSCE_IF_SR | \
                        TSCE_IF_KR | TSCE_IF_CR | TSCE_IF_XFI | TSCE_IF_SFI)


typedef struct tsce_speed_config_s {
    uint32  port_refclk_int;
    int     speed;
    int     port_num_lanes;
    int     port_is_higig;
    int     rxaui_mode;
    int     scrambler_en;
    int     line_interface;
    int     pcs_bypass;
    int     fiber_pref;
    int     phy_supports_dual_rate;
    int     pll_divider_req  ;
} tsce_speed_config_t;

#define NUM_PATTERN_DATA_INTS    8
typedef struct tsce_pattern_s {
    uint32 pattern_data[NUM_PATTERN_DATA_INTS];
    uint32 pattern_length;
} tsce_pattern_t;

#define TSCE_LANE_NAME_LEN   30

typedef struct {
    uint16 serdes_id0;
    char   name[TSCE_LANE_NAME_LEN];
} tsce_dev_info_t;

/* index to TX drive configuration table for each VCO setting.
 */
typedef enum txdrv_inxs {
    TXDRV_XFI_INX = 0,     /* 10G XFI */
    TXDRV_SFI_INX,     /* 10G SR fiber mode */
    TXDRV_SFIDAC_INX,  /* 10G SFI DAC mode */
    TXDRV_AN_INX,      /* a common entry for autoneg mode */
    TXDRV_DFT_INX,     /* temp for any missing speed modes */
    TXDRV_LAST_INX      /* always last */
} TXDRV_INXS_t;

#define TXDRV_ENTRY_NUM     (TXDRV_LAST_INX)



/*
   Config - per logical port
*/
typedef struct {
    phymod_polarity_t               phy_polarity_config;
    phymod_phy_reset_t              phy_reset_config;
    tsce_pattern_t                  pattern;
    tsce_speed_config_t             speed_config;

    int fiber_pref;                 /* spn_SERDES_FIBER_PREF */
    int cl73an;                     /* spn_PHY_AN_C73 */
    int cx4_10g;                    /* spn_10G_IS_CX4 */
    int pdetect1000x;
    int cl37an;                     /* spn_PHY_AN_C37 */
    int an_cl72;                    /* spn_PHY_AN_C72 */
    int forced_init_cl72;           /* spn_FORCED_INIT_CL72 */
    int hg_mode;                    /* higig port */
    int an_fec;                     /*enable FEC for AN mode */
    int sgmii_mstr;                 /* sgmii master mode */
    int serdes_1000x_at_6250_vco;          /* 1g speed at 6250 vco*/
    tsce_dev_info_t info;           /*serdes id info */
    uint32 line_intf;               /*line interface info */
    phymod_tx_t tx_drive[TXDRV_ENTRY_NUM];

    int pcs_table_override ;
    temod_device_aux_modes_t        *core_device_aux_modes ;


} tsce_config_t;

#define TSCE_IF_NULL   (1 << SOC_PORT_IF_NULL)
#define TSCE_IF_SR     (1 << SOC_PORT_IF_SR)
#define TSCE_IF_KR     (1 << SOC_PORT_IF_KR)
#define TSCE_IF_KR4    (1 << SOC_PORT_IF_KR4)
#define TSCE_IF_CR     (1 << SOC_PORT_IF_CR)
#define TSCE_IF_CR4    (1 << SOC_PORT_IF_CR4)
#define TSCE_IF_XFI    (1 << SOC_PORT_IF_XFI)
#define TSCE_IF_SFI    (1 << SOC_PORT_IF_SFI)
#define TSCE_IF_XLAUI  (1 << SOC_PORT_IF_XLAUI)
#define TSCE_IF_XGMII  (1 << SOC_PORT_IF_XGMII)
#define TSCE_IF_ILKN   (1 << SOC_PORT_IF_ILKN)

#define TSCE_40G_10G_INTF(_if)  ((_if) == SOC_PORT_IF_KR4 || (_if) == SOC_PORT_IF_XLAUI || \
                             (_if) == SOC_PORT_IF_CR4 || (_if) == SOC_PORT_IF_SR || \
                             (_if) == SOC_PORT_IF_KR || (_if) == SOC_PORT_IF_CR || \
                             (_if) == SOC_PORT_IF_XFI || (_if) == SOC_PORT_IF_SFI)

#define TSCE_40G_10G_INTF_ALL   (TSCE_IF_KR4 | TSCE_IF_XLAUI | TSCE_IF_CR4 | TSCE_IF_SR | \
                        TSCE_IF_KR | TSCE_IF_CR | TSCE_IF_XFI | TSCE_IF_SFI)



#define TEMOD_CL73_CL37              0x5
#define TEMOD_CL73_HPAM              0x4
#define TEMOD_CL73_HPAM_VS_SW        0x8
#define TEMOD_CL73_WO_BAM            0x2
#define TEMOD_CL73_W_BAM             0x1
#define TEMOD_CL73_DISABLE           0x0

#define TEMOD_CL37_HR2SPM_W_10G 5
#define TEMOD_CL37_HR2SPM       4
#define TEMOD_CL37_W_10G     3
#define TEMOD_CL37_WO_BAM    2
#define TEMOD_CL37_W_BAM     1
#define TEMOD_CL37_DISABLE   0

STATIC int tsce_cl72_enable_set(soc_phymod_ctrl_t *pmc, uint32 value);
STATIC int phy_tsce_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability);
STATIC int phy_tsce_ability_advert_set(int unit, soc_port_t port,
                          soc_port_ability_t *ability);

/*
 * Function:
 *      _tsce_reg_read
 * Doc:
 *      register read operations
 * Parameters:
 *      unit            - (IN)  unit number
 *      core_addr       - (IN)  core address
 *      reg_addr        - (IN)  address to read
 *      val             - (OUT) read value
 */
STATIC int 
_tsce_reg_read(void* user_acc, uint32_t core_addr, uint32_t reg_addr, uint32_t *val)
{
    uint16 data16;
    int rv;
    soc_phymod_core_t *core = (soc_phymod_core_t *)user_acc;

    rv = core->read(core->unit, core_addr, reg_addr, &data16);
    *val = data16;
	PHYMOD_VDBG(TEMOD_DBG_REGACC,NULL,("-22%s: core_addr: 0x%08x reg_addr: 0x%08x, data: 0x%04x\n", __func__, core_addr, reg_addr, *val ));

    return rv;
}

/*
 * Function:
 *      _tsce_reg_write
 * Doc:
 *      register write operations
 * Parameters:
 *      unit            - (IN)  unit number
 *      core_addr       - (IN)  core address
 *      reg_addr        - (IN)  address to write
 *      val             - (IN)  write value
 */
STATIC int 
_tsce_reg_write(void* user_acc, uint32_t core_addr, uint32_t reg_addr, uint32_t val)
{
    soc_phymod_core_t *core = (soc_phymod_core_t *)user_acc;
    uint16 data16 = (uint16)val;
    uint16 mask = (uint16)(val >> 16);

    if (mask) {
        if (core->wrmask) {
            return core->wrmask(core->unit, core_addr, reg_addr, data16, mask);
        }
        (void)core->read(core->unit, core_addr, reg_addr, &data16);
        data16 &= ~mask;
        data16 |= (val & mask);
    }
	PHYMOD_VDBG(TEMOD_DBG_REGACC,NULL,("-22%s: core_addr: 0x%08x reg_addr: 0x%08x, data: 0x%04x\n", __func__, core_addr, reg_addr, val ));
    return core->write(core->unit, core_addr, reg_addr, data16);
}

/*
 * Function:
 *      tsce_firmware_loader
 * Purpose:
 *      Download PHY firmware using fast interface such as S-channel.
 * Parameters:
 *      pm_core         - (IN)  PHY core oject
 *      fw_len          - (IN)  Number of bytes to download
 *      fw_data         - (IN)  Firmware as byte array
 */
STATIC int
tsce_firmware_loader(const phymod_core_access_t *pm_core,
                     uint32 fw_len, const uint8 *fw_data)
{
    soc_phymod_core_t *core;
    int unit, port;

    if (pm_core == NULL) {
        return PHYMOD_E_PARAM;
    }

    /* Retrieve SOC information from PHYMOD object */
    core = (soc_phymod_core_t *)(pm_core->access.user_acc);

    if (core == NULL) {
        return PHYMOD_E_PARAM;
    }

    /* Get associated unit/port */
    unit = core->unit;
    port = core->port;

    return _phy_tsce_firmware_set_helper[unit](unit, port,
                                               (uint8 *)fw_data, fw_len);
}


#define IS_DUAL_LANE_PORT(_pc) ((_pc)->phy_mode == PHYCTRL_DUAL_LANE_PORT)  
#define IND_LANE_MODE(_pc) (((_pc)->phy_mode == PHYCTRL_DUAL_LANE_PORT) || ((_pc)->phy_mode == PHYCTRL_ONE_LANE_PORT))  
#define MAIN0_SERDESID_REV_LETTER_SHIFT  0x14
#define MAIN0_SERDESID_REV_NUMBER_SHIFT  0x11

/*
 * Function:
 *      tsce_firmware_loader
 * Purpose:
 *      Download PHY firmware using fast interface such as S-channel.
 * Parameters:
 *      pc             - (IN)  phyctrl  oject
 *      pInfo          - (IN)  device info object
 *      rev_id         - (IN)  serdes revid
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
tsce_show_serdes_info(phy_ctrl_t *pc, tsce_dev_info_t *pInfo, phymod_core_info_t *core_info)
{
    uint32_t serdes_id0, len;

    pInfo->serdes_id0 = core_info->serdes_id ;

    /* This is TSC/Eagle device */
    serdes_id0 = pInfo->serdes_id0;
    if (core_info->core_version == phymodCoreVersionTsce4A0) {
        sal_strcpy(pInfo->name,"TSCE4-");
    } else {
        sal_strcpy(pInfo->name,"TSCE12-");
    }
    len = sal_strlen(pInfo->name);
    /* add rev letter */
    pInfo->name[len++] = 'A' + ((serdes_id0 >>
                          MAIN0_SERDESID_REV_LETTER_SHIFT) & 0x3);
    /* add rev number */
    pInfo->name[len++] = '0' + ((serdes_id0 >>
                          MAIN0_SERDESID_REV_NUMBER_SHIFT) & 0x7);
    pInfo->name[len++] = '/';
    pInfo->name[len++] = (pc->chip_num / 10) % 10 + '0';
    pInfo->name[len++] = pc->chip_num % 10 + '0';
    pInfo->name[len++] = '/';

    /* phy_mode: 0 single port mode, port uses all four lanes of Warpcore
     *           1 dual port mode, port uses 2 lanes
     *           2 quad port mode, port uses 1 lane
     */
    if (IS_DUAL_LANE_PORT(pc)) { /* dual-xgxs mode */
        if (pc->lane_num < 2) { /* the first dual-xgxs port */
            pInfo->name[len++] = '0';
            pInfo->name[len++] = '-';
            pInfo->name[len++] = '1';
        } else {
            pInfo->name[len++] = '2';
            pInfo->name[len++] = '-';
            pInfo->name[len++] = '3';
        }
    } else if (pc->phy_mode == PHYCTRL_ONE_LANE_PORT) {
        pInfo->name[len++] = pc->lane_num + '0';
    } else {
        pInfo->name[len++] = '4';
    }
    pInfo->name[len] = 0;  /* string terminator */

    if (len > TSCE_LANE_NAME_LEN) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("TSCE info string length %d exceeds max length 0x%x: u=%d p=%d\n"),
                   len,TSCE_LANE_NAME_LEN, pc->unit, pc->port));
        return SOC_E_MEMORY;
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      tsce_speed_to_interface_config_get
 * Purpose:     
 *      Convert speed to interface_config struct
 * Parameters:
 *      unit                - BCM unit number.
 *      port                - Port number.
 *      speed               - speed to convert
 *      interface_config    - output interface config struct
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int 
tsce_speed_to_interface_config_get(tsce_speed_config_t* speed_config, phymod_phy_inf_config_t* interface_config)
{
    int port_is_higig;
    int port_num_lanes;
    int scr_enabled;
    int fiber_pref;

    port_num_lanes = speed_config->port_num_lanes;
    port_is_higig = speed_config->port_is_higig;
    scr_enabled = speed_config->scrambler_en;
    fiber_pref = speed_config->fiber_pref;

    SOC_IF_ERROR_RETURN(phymod_phy_inf_config_t_init(interface_config));

    interface_config->interface_modes = 0;
    interface_config->data_rate       = speed_config->speed;
    interface_config->pll_divider_req = speed_config->pll_divider_req ;
    interface_config->device_aux_modes= NULL ;

    if (port_is_higig) {
        PHYMOD_INTERFACE_MODES_HIGIG_SET(interface_config);
    }
    switch (speed_config->speed) {
        case 10:
            interface_config->interface_type = phymodInterfaceSGMII;
            break;
        case 100:
        case 1000:
            if (speed_config->fiber_pref) {
                interface_config->interface_type = phymodInterface1000X;
            } else {
                interface_config->interface_type = phymodInterfaceSGMII;
            }
            break;
        case 2500:
            interface_config->interface_type = phymodInterface1000X;
            break;
		case 5000: 
            interface_config->interface_type = phymodInterfaceXFI;
            break;
        case 3125:
        case 5750:
            interface_config->interface_type = phymodInterfaceBypass;
            break;
        case 6250:
            interface_config->interface_type = phymodInterfaceBypass;
            if(speed_config->phy_supports_dual_rate) { /* for dual rate OS2 is required */
                PHYMOD_INTERFACE_MODES_OS2_SET(interface_config);
            }
            break;
        case 8125:
        case 8500:
            interface_config->interface_type = phymodInterfaceBypass;
            break;
        case 10000:
            if (port_num_lanes == 4) {
                interface_config->interface_type = phymodInterfaceXGMII;
                
            } else if (speed_config->rxaui_mode == 2) {
                interface_config->interface_type = phymodInterfaceRXAUI;
                interface_config->interface_modes = interface_config->interface_modes  | (2 << speed_config->scrambler_en);
          #if 0
            } else if (port_num_lanes == 2) {
                if (speed_config->rxaui_mode)  {
                    interface_config->interface_type = phymodInterfaceX2;
                } else {
                    interface_config->interface_type = phymodInterfaceRXAUI;
                }
                if (scr_enabled) {
                    PHYMOD_INTERFACE_MODES_SCR_SET(interface_config);
                }
            #endif
            } else {
               if ((speed_config->line_interface & TSCE_IF_SFI) ||
                   (speed_config->line_interface & TSCE_IF_SR) ||
                   (speed_config->line_interface & TSCE_IF_CR)) {
                    interface_config->interface_type = phymodInterfaceSFI;
               } else {
                    interface_config->interface_type = phymodInterfaceXFI;
               } 
            }
            break;
    case 10312:
        interface_config->interface_type = phymodInterfaceBypass;
        break;
    case 10500: 
        break ;
    case 10937:
        interface_config->interface_type = phymodInterfaceBypass;
        break;
    case 11000:
        interface_config->interface_type = phymodInterfaceXFI;
        break;
    case 11250:
        interface_config->interface_type = phymodInterfaceBypass;
        break;
    case 11500:
        if (speed_config->pcs_bypass) {
            interface_config->interface_type = phymodInterfaceBypass;
        } else {
            interface_config->interface_type = phymodInterfaceXFI;
        }
        break;
    case 12000:
        if (port_num_lanes == 4) {
            interface_config->interface_type = phymodInterfaceXGMII;
        } else if (port_num_lanes == 2) {
            interface_config->interface_type = phymodInterfaceRXAUI;
        } else {
            interface_config->interface_type = phymodInterfaceXFI;
        }
        break ;
    case 12500:
        if (speed_config->pcs_bypass) {
            interface_config->interface_type = phymodInterfaceBypass;
        } else {
            interface_config->interface_type = phymodInterfaceXFI;
        }         
        break;
    case 12773:   /*  12733Mbps */
        interface_config->interface_type = phymodInterfaceRXAUI;
        break ;
    case 13000:
        interface_config->interface_type = phymodInterfaceXGMII;
        break;
    case 15000:
        interface_config->interface_type = phymodInterfaceXGMII;
        break;
    case 15750:
    case 16000:  /* setting RX66_CONTROL_CC_EN/CC_DATA_SEL failed this 16000 HI_DXGXS mode */
        if (port_num_lanes == 4){
            interface_config->interface_type = phymodInterfaceXGMII;
        } else if (port_num_lanes == 2) {
            interface_config->interface_type = phymodInterfaceRXAUI;
        } 
        break ;
        /*  WCMOD_SPD_21G_HI_DXGXS ?? */
    case 20000: 
        if (port_num_lanes == 4) {
            if (scr_enabled) {
                PHYMOD_INTERFACE_MODES_SCR_SET(interface_config);
            }
            interface_config->interface_type = phymodInterfaceXGMII;
        } else if(port_num_lanes == 2) {
            if (port_is_higig) {
                interface_config->interface_type = phymodInterfaceRXAUI;
            } else {
                if( (speed_config->line_interface & TSCE_IF_KR4) ||   
                    (speed_config->line_interface & TSCE_IF_KR) ) {
                    interface_config->interface_type = phymodInterfaceKR2;
                } else if( (speed_config->line_interface & TSCE_IF_CR4) || 
                           (speed_config->line_interface & TSCE_IF_CR) ) {
                    interface_config->interface_type = phymodInterfaceCR2;
                } else {
                    interface_config->interface_type = phymodInterfaceRXAUI;
                }
            }
        } 
        break ;
    case 21000:
        if (port_num_lanes == 4) {
            interface_config->interface_type = phymodInterfaceXGMII;
        }else if(port_num_lanes == 2) {
            interface_config->interface_type = phymodInterfaceRXAUI;
        }
        break ;
    case 23000:
       if (port_num_lanes == 2) {
          interface_config->interface_type = phymodInterfaceX2;
       }
       break ;
    case 25000:
    case 25455:
    case 30000:
    case 31500:
        interface_config->interface_type = phymodInterfaceXGMII;
        break;
    case 40000:
        if(port_is_higig) {
            interface_config->interface_type = phymodInterfaceXGMII;
        }else if(fiber_pref){
            PHYMOD_INTERFACE_MODES_FIBER_SET(interface_config);
            interface_config->interface_type = phymodInterfaceKR4;
        }else{
            interface_config->interface_type = phymodInterfaceXLAUI;
        }
        break;
    case 42000:
        if (port_is_higig) {
            interface_config->interface_type = phymodInterfaceXGMII;
        }
        break;
    default:
        return SOC_E_PARAM;
    }

    switch (speed_config->port_refclk_int)
    {
    case 156:
        interface_config->ref_clock = phymodRefClk156Mhz;
        break;
    case 125:
        interface_config->ref_clock = phymodRefClk125Mhz;
        break;
    default:
        return SOC_E_PARAM;
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      tsce_config_init
 * Purpose:     
 *      Determine phy configuration data for purposes of PHYMOD initialization.
 * 
 *      A side effect of this procedure is to save some per-logical port
 *      information in (tsce_cfg_t *) &pc->driver_data;
 *
 * Parameters:
 *      unit                  - BCM unit number.
 *      port                  - Port number.
 *      logical_lane_offset   - starting logical lane number
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
tsce_config_init(int unit, soc_port_t port, int logical_lane_offset,
                 phymod_core_init_config_t *core_init_config, 
                 phymod_phy_init_config_t  *pm_phy_init_config)
{
    phy_ctrl_t *pc;
    tsce_speed_config_t *speed_config;
    tsce_config_t *pCfg;
    phymod_tx_t *p_tx;
    phymod_firmware_load_method_t fw_ld_method;
    int port_refclk_int;
    int port_num_lanes;
    int core_num;
    int phy_num_lanes;
    int port_is_higig;
    int phy_passthru; 
    int phy_supports_dual_rate;
    int lane_map_rx, lane_map_tx, lane_map_tx_l=0;
    int i;
    int fiber_pref = 1;

    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }
    pCfg = (tsce_config_t *) pc->driver_data;

    /* determine physical port (mapping logical to physical if necessary) */
    /* if (soc_feature(unit, soc_feature_logical_port_num)) {
        phy_port = SOC_INFO(unit).port_l2p_mapping[port];
    } else {
        phy_port = port;
    } */

    /* extract data from SOC_INFO */
    port_refclk_int = SOC_INFO(unit).port_refclk_int[port];
#if 0
    
    port_refclk_int = soc_property_port_get(unit,port, spn_XGXS_PHY_VCO_FREQ, pCfg->refclk);
#endif
    port_refclk_int = 156;  
    port_refclk_int = soc_property_port_get(unit, port,spn_XGXS_LCPLL_XTAL_REFCLK, port_refclk_int);
    port_num_lanes = SOC_INFO(unit).port_num_lanes[port];
    port_is_higig = (PBMP_MEMBER(SOC_HG2_PBM(unit), port) || PBMP_MEMBER(PBMP_HG_ALL(unit), port));
    phy_supports_dual_rate = PHY_IS_SUPPORT_DUAL_RATE(unit, port);

    /* figure out how many lanes are in this phy */
    core_num = (logical_lane_offset / 4);
    phy_num_lanes = (port_num_lanes - logical_lane_offset);
    if (phy_num_lanes > MAX_NUM_LANES) {
       phy_num_lanes = MAX_NUM_LANES;
    }
    
    /* 
        CORE configuration
    */
    phymod_core_init_config_t_init(core_init_config);
    fw_ld_method = phymodFirmwareLoadMethodInternal;
    if (_phy_tsce_firmware_set_helper[unit]) {
        core_init_config->firmware_loader = tsce_firmware_loader;
        fw_ld_method = phymodFirmwareLoadMethodExternal;
    }
    if (soc_property_port_get(pc->unit, pc->port, "tsce_sim", 0)) {
        fw_ld_method = phymodFirmwareLoadMethodNone;
    }

    
    core_init_config->firmware_load_method  = soc_property_port_get(unit, port, 
                                              spn_LOAD_FIRMWARE, fw_ld_method);
    core_init_config->firmware_load_method &= 0xff; /* clear checksum bits */
    core_init_config->lane_map.num_of_lanes = NUM_LANES;
    
    /* For TSC-E/F family,  lane_map_rx is logic lane base, and lane_map_tx is phy lane base */
    /* Both internal data structure is logic lane base */
    lane_map_rx = soc_property_port_suffix_num_get(unit, port, core_num,
                                        spn_XGXS_RX_LANE_MAP, "core", 0x3210);
    for (i=0; i<NUM_LANES; i++) {
        core_init_config->lane_map.lane_map_rx[i] = (lane_map_rx >> (i * 4 /* 4 bit per lane */)) & 0xf;
    }
 
    lane_map_tx = soc_property_port_suffix_num_get(unit, port, core_num,
                                        spn_XGXS_TX_LANE_MAP, "core", 0x3210);
    for (i=0; i<NUM_LANES; i++) {
        lane_map_tx_l |= i << 4*((lane_map_tx >> (i*4)) & 0xf) ;
    }
    for (i=0; i<NUM_LANES; i++) {
        core_init_config->lane_map.lane_map_tx[i] = (lane_map_tx_l >> (i * 4 /*4 bit per lane*/)) & 0xf;
    }

    /* next get the tx/rx polairty info */
    pm_phy_init_config->polarity.tx_polarity = soc_property_port_get(unit, port,
                            spn_PHY_XAUI_TX_POLARITY_FLIP, 0);
    pm_phy_init_config->polarity.rx_polarity = soc_property_port_get(unit, port,
                            spn_PHY_XAUI_RX_POLARITY_FLIP, 0);

    speed_config = &(pCfg->speed_config);
    speed_config->port_refclk_int  = port_refclk_int;
    speed_config->speed  = soc_property_port_get(unit, port, spn_PORT_INIT_SPEED, pc->speed_max);
    speed_config->port_num_lanes   = phy_num_lanes;
    speed_config->port_is_higig    = port_is_higig;
    speed_config->rxaui_mode       = soc_property_port_get(unit, port, spn_SERDES_RXAUI_MODE, 1);
    speed_config->scrambler_en     = soc_property_port_get(unit, port, spn_SERDES_SCRAMBLER_ENABLE, 0);
    speed_config->line_interface   = soc_property_port_get(unit, port, spn_SERDES_IF_TYPE, 0);
    speed_config->fiber_pref = soc_property_port_get(unit, port, spn_SERDES_FIBER_PREF, fiber_pref);
    speed_config->pcs_bypass       = 0;
    speed_config->phy_supports_dual_rate = phy_supports_dual_rate;
    speed_config->pll_divider_req      = soc_property_port_get(unit, port, spn_XGXS_PHY_PLL_DIVIDER, 0);

    if (port_num_lanes == 1) {
        if (PHY_EXTERNAL_MODE(unit, port) || !(speed_config->fiber_pref)) {
            speed_config->line_interface =  TSCE_IF_XFI;
        } else {
            speed_config->line_interface =  TSCE_IF_SFI;
        }
    } else { 
            speed_config->line_interface =  TSCE_IF_XLAUI;
    }

    SOC_IF_ERROR_RETURN
        (tsce_speed_to_interface_config_get(speed_config, &(core_init_config->interface)));

    /* 
        PHY configuration
    */

    /* set the default tx parameters */
    p_tx = &pCfg->tx_drive[TXDRV_DFT_INX];
    p_tx->amp = 0xc;
    p_tx->pre = 0xc;
    p_tx->main = 0x66;
    p_tx->post = 0x0;
    p_tx->post2 = 0x00;
    p_tx->post3 = 0x00;

    p_tx = &pCfg->tx_drive[TXDRV_XFI_INX];
    p_tx->amp = 0xc;
    p_tx->pre = 0x00;
    p_tx->main = 0x30;
    p_tx->post = 0x0;
    p_tx->post2 = 0x00;
    p_tx->post3 = 0x00;

    p_tx = &pCfg->tx_drive[TXDRV_SFIDAC_INX];
    p_tx->amp = 0xc;
    p_tx->pre = 0x00;
    p_tx->main = 0x18;
    p_tx->post = 0x18;
    p_tx->post2 = 0x00;
    p_tx->post3 = 0x00;

    p_tx = &pCfg->tx_drive[TXDRV_AN_INX];
    p_tx->amp = 0xc;
    p_tx->pre = 0xc;
    p_tx->main = 0x66;
    p_tx->post = 0x00;
    p_tx->post2 = 0x00;
    p_tx->post3 = 0x00;

    phymod_phy_init_config_t_init(pm_phy_init_config);
    /* initialize the tx taps and driver current */
    for (i = 0; i < 4; i++) { 
        pm_phy_init_config->tx[i].pre = 0xc;
        pm_phy_init_config->tx[i].main = 0x66;
        pm_phy_init_config->tx[i].post = 0x0;
        pm_phy_init_config->tx[i].post2 = 0x0;
        pm_phy_init_config->tx[i].post3 = 0x0;
        pm_phy_init_config->tx[i].amp = 0xc;
    }
    pm_phy_init_config->polarity.rx_polarity 
                                 = soc_property_port_get(unit, port,
                                   spn_PHY_XAUI_TX_POLARITY_FLIP, FALSE);
    pm_phy_init_config->polarity.tx_polarity 
                                 = soc_property_port_get(unit, port,
                                   spn_PHY_XAUI_RX_POLARITY_FLIP, FALSE);

    for (i = 0; i < MAX_NUM_LANES; i++) {
        uint32_t preemphasis;
        preemphasis = soc_property_port_suffix_num_get(unit, port,
                                    i + core_num * 4, spn_SERDES_PREEMPHASIS,
                                    "lane", TSCE_NO_CFG_VALUE);
        if (preemphasis != TSCE_NO_CFG_VALUE) {
            pm_phy_init_config->tx[i].pre = preemphasis & 0xff;
            pm_phy_init_config->tx[i].main = (preemphasis & 0xff00) >> 8;
            pm_phy_init_config->tx[i].post = (preemphasis & 0xff0000) >> 16;
        }     
    }

    for (i = 0; i < MAX_NUM_LANES; i++) {
        pm_phy_init_config->tx[i].amp = soc_property_port_suffix_num_get(unit, port,
                                    i + core_num * 4, spn_SERDES_DRIVER_CURRENT,
                                    "lane", pm_phy_init_config->tx[i].amp);
    }

    

    
    pm_phy_init_config->cl72_en = soc_property_port_get(unit, port, spn_PHY_AN_C72, TRUE);
    pm_phy_init_config->an_en = TRUE;

    pCfg->serdes_1000x_at_6250_vco = 0;
    pCfg->serdes_1000x_at_6250_vco = soc_property_port_get(unit, port,
                                        spn_SERDES_1000X_AT_6250_VCO,
                                        pCfg->serdes_1000x_at_6250_vco);

    /* check the higig port mode, then set the default cl73 mode */
    if (port_is_higig) {
        pCfg->cl73an = FALSE;
    } else {
        pCfg->cl73an = PHYMOD_AN_CAPABILITIES_CL73;
    }

    /* by default disable cl37 */
    pCfg->cl37an = FALSE;
    pCfg->an_cl72 = TRUE;
    pCfg->forced_init_cl72 = FALSE;
    pCfg->an_fec = FALSE; 

    pCfg->cl73an  = soc_property_port_get(unit, port,
                                          spn_PHY_AN_C73, pCfg->cl73an); /* no L: C73, not CL73 */

    pCfg->cl37an = soc_property_port_get(unit, port,
                                        spn_PHY_AN_C37, pCfg->cl37an); /* no L: C37, not CL37 */

    pCfg->an_cl72 = soc_property_port_get(unit, port,
                                        spn_PHY_AN_C72, pCfg->an_cl72);
    pCfg->an_fec = soc_property_port_get(unit, port,
                                        spn_PHY_AN_FEC, pCfg->an_fec);
    pCfg->forced_init_cl72 = soc_property_port_get(unit, port,
                                        spn_PORT_INIT_CL72, pCfg->forced_init_cl72);

    
    /* 
        phy_ctrl_t configuration (LOGICAL PORT BASED)
        Only do this once, for the first core of the logical port
    */
    if (core_num == 0) {
        

        /* phy_mode, PHYCTRL_MDIO_ADDR_SHARE, PHY_FLAGS_INDEPENDENT_LANE */
        if (port_num_lanes == 4) {
            pc->phy_mode = PHYCTRL_QUAD_LANE_PORT;
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_INDEPENDENT_LANE);
        } else if (port_num_lanes == 2) {
            pc->phy_mode = PHYCTRL_DUAL_LANE_PORT;
            pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_INDEPENDENT_LANE);
        } else if (port_num_lanes == 1) {
            pc->phy_mode = PHYCTRL_ONE_LANE_PORT;
            pc->flags |= PHYCTRL_MDIO_ADDR_SHARE;
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_INDEPENDENT_LANE);
        }

        /* PHY_FLAGS_PASSTHRU */
        phy_passthru = 0;
        if (PHY_EXTERNAL_MODE(unit, port)) {
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_PASSTHRU);
            phy_passthru = soc_property_port_get(unit, port,
                                                 spn_PHY_PCS_REPEATER, 0);
            if (phy_passthru) {
                PHY_FLAGS_SET(unit, port, PHY_FLAGS_PASSTHRU);
            }
        }

        /* PHY_FLAGS_FIBER */
        PHY_FLAGS_CLR(unit, port, PHY_FLAGS_FIBER);
        if (!PHY_EXTERNAL_MODE(unit, port)) {

            fiber_pref = FALSE;
            if (PHY_FIBER_MODE(unit, port) ||
                (phy_passthru) ||
                (!PHY_INDEPENDENT_LANE_MODE(unit, port)) ||
                (pc->speed_max >= 10000)) {
                fiber_pref = TRUE;
            }

            fiber_pref = soc_property_port_get(unit, port,
                                               spn_SERDES_FIBER_PREF, fiber_pref);
            if (fiber_pref) {
                PHY_FLAGS_SET(unit, port, PHY_FLAGS_FIBER);
            }
        }

        
        PHY_FLAGS_SET(unit,port,PHY_FLAGS_SERVICE_INT_PHY_LINK_GET);

        
        /* not used  */
        /* pCfg->cl73an = soc_property_port_get(unit, port, spn_PHY_AN_C73, TSCMOD_CL73_WO_BAM);

        if (!PHY_EXTERNAL_MODE(unit, port)) {
            if (pCfg->cl73an) {
                PHY_FLAGS_SET(unit, port, PHY_FLAGS_C73);
            }
        }
        */

        
        if ((PHY_FIBER_MODE(unit, port) && !PHY_EXTERNAL_MODE(unit, port)) ||
            phy_passthru) {
            pCfg->pdetect1000x = TRUE;
        } else {
            pCfg->pdetect1000x = FALSE;
        }

#if 0
        
        pCfg->sw_rx_los.enable = soc_property_port_get(unit, port,
                                spn_SERDES_RX_LOS, pCfg->sw_rx_los.enable);
#endif

#if 0
#endif
        pCfg->cx4_10g            = soc_property_port_get(unit, port, spn_10G_IS_CX4, TRUE);
    }

    return SOC_E_NONE;
}

STATIC int
_tsce_device_print(temod_device_aux_modes_t *device, soc_phymod_core_t *core)
{
    if(device && core) {
        PHYMOD_VDBG(TEMOD_DBG_MEM, NULL, ("core_id=%0x this=%p st_current_entry=%0d\n", device->core_id, (void *)device, device->st_current_entry)) ;
        PHYMOD_VDBG(TEMOD_DBG_MEM, NULL, ("ref_cnt=%0d core_p=%p\n", core->ref_cnt, (void *)core)) ;
    } else {
        PHYMOD_VDBG(TEMOD_DBG_MEM, NULL, ("Error: no defined device aux modes.\n"));
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_tsce_init
 *  
 *      An SDK "phy" is a logical port, which can consist of from 1-10 lanes,
 *          (A phy with more than 4 lanes requires more than one core).
 *      Per-logical port information is saved in (tsce_cfg_t *) &pc->driver_data.
 *      An SDK phy is implemented as one or more PHYMOD "phy"s.
 *  
 *      A PHYMOD "phy" resides completely within a single core, which can be
 *      from 1 to 4 lanes.
 *      Per-phymod phy information is kept in (soc_phymod_ctrl_t) *pc->phymod_ctrl
 *      A phymod phy points to its core.  Up to 4 phymod phys can be on a single core
 *  
 * Purpose:     
 *      Initialize a tsce phy
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_init(int unit, soc_port_t port)
{
    phy_ctrl_t *pc; 
    soc_phymod_ctrl_t *pmc;
    soc_phymod_phy_t *phy = NULL;
    soc_phymod_core_t *core;
    tsce_config_t *pCfg;
    tsce_speed_config_t *speed_config;
    tsce_dev_info_t *pInfo;
    soc_phy_info_t *pi;
    phymod_phy_inf_config_t interface_config;
    phymod_core_status_t core_status;
    phymod_core_info_t core_info;
    int idx;
    int logical_lane_offset;
    soc_port_ability_t ability;

    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }
    pc->driver_data = (void*)(pc+1);
    pmc = &pc->phymod_ctrl;
    pCfg = (tsce_config_t *) pc->driver_data;
    pInfo = &pCfg->info; 
    
    sal_memset(pCfg, 0, sizeof(*pCfg));
    speed_config = &(pCfg->speed_config);

    sal_memset(&ability, 0, sizeof(ability));

    /* Loop through all phymod phys that support this SDK phy */
    logical_lane_offset = 0;
    for (idx = 0; idx < pmc->num_phys; idx++) {
        phy = pmc->phy[idx];
        core = phy->core;
        pCfg->core_device_aux_modes = (temod_device_aux_modes_t *)(core->device_aux_modes) ;
        _tsce_device_print(pCfg->core_device_aux_modes, core) ;
        

        /* determine configuration data structure to default values, based on SOC properties */
        SOC_IF_ERROR_RETURN
            (tsce_config_init(unit, port,
                              logical_lane_offset,
                              &core->init_config, &phy->init_config));

        
        if (!core->init) {
           core_status.pmd_active = 0;
           SOC_IF_ERROR_RETURN
               (phymod_core_init(&core->pm_core, &core->init_config, &core_status));
           core->init = TRUE;
        }

        SOC_IF_ERROR_RETURN
            (phymod_phy_init(&phy->pm_phy, &phy->init_config));

        /*read serdes id info */
        SOC_IF_ERROR_RETURN
            (phymod_core_info_get(&core->pm_core, &core_info)); 


        /* for multicore phys, need to ratchet up to the next batch of lanes */
        logical_lane_offset += core->init_config.lane_map.num_of_lanes;
    }

    /* fill up the pInfo table for displaying the serdes driver info */
    SOC_IF_ERROR_RETURN
        (tsce_show_serdes_info(pc, pInfo, &core_info));

    /* retrieve chip level information for serdes driver info */
    pi = &SOC_PHY_INFO(unit, port);

    if (!PHY_EXTERNAL_MODE(unit, port)) {
        pi->phy_name = pInfo->name;
    }

    /*next check if we need to run 1G at 6.25VCO */
    if (pCfg->serdes_1000x_at_6250_vco) {
        speed_config->pll_divider_req = 40;
    }

    /* set the port to its max or init_speed */
    SOC_IF_ERROR_RETURN
        (tsce_speed_to_interface_config_get(speed_config, &interface_config));

    SOC_IF_ERROR_RETURN
        (phymod_phy_interface_config_set(&phy->pm_phy,
                                         0 /* flags */, &interface_config));
    /*next check if cl72 needs to be enabled */
    if (pCfg->forced_init_cl72) {
        SOC_IF_ERROR_RETURN
            (tsce_cl72_enable_set(pmc, 1));
    }

    /* setup the port's an cap */
    SOC_IF_ERROR_RETURN
        (phy_tsce_ability_local_get(unit, port, &ability));
 
    SOC_IF_ERROR_RETURN
        (phy_tsce_ability_advert_set(unit, port, &ability));

    LOG_INFO(BSL_LS_SOC_PHY,
             (BSL_META_U(pc->unit,
                         "phy_tsce_init: u=%d p=%d\n"), unit, port));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_tsce_link_get
 * Purpose:
 *      Get layer2 connection status.
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 *      link - address of memory to store link up/down state.
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_link_get(int unit, soc_port_t port, int *link)
{
    phy_ctrl_t                *pc;
    soc_phymod_ctrl_t         *pmc;
    phymod_phy_access_t       *pm_phy;

    *link = 0;
    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    pmc = &pc->phymod_ctrl;
    pm_phy = &pmc->phy[0]->pm_phy;

    SOC_IF_ERROR_RETURN
        (phymod_phy_link_status_get(pm_phy, (uint32_t *) link));
    return (SOC_E_NONE);
}

/*
 * Function:
 *      phy_tsce_enable_set
 * Purpose:
 *      Enable/Disable phy 
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 *      enable - on/off state to set
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_enable_set(int unit, soc_port_t port, int enable)
{
    phy_ctrl_t                *pc;
    soc_phymod_ctrl_t         *pmc;
    phymod_phy_access_t       *pm_phy;
    phymod_phy_power_t        phy_power;
    int idx;

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    if (enable) {
        phy_power.tx = phymodPowerOn;
        phy_power.rx = phymodPowerOn;
    } else {
        phy_power.tx = phymodPowerOff;
        phy_power.rx = phymodPowerOff;
    }

    pmc = &pc->phymod_ctrl;
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;
        SOC_IF_ERROR_RETURN
            (phymod_phy_power_set(pm_phy, &phy_power));
    }
    return (SOC_E_NONE);
}

/*
 * Function:
 *      phy_tsce_enable_get
 * Purpose:
 *      Enable/Disable phy 
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 *      enable - on/off state to set
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_enable_get(int unit, soc_port_t port, int *enable)
{
    phy_ctrl_t                *pc;
    soc_phymod_ctrl_t         *pmc;
    phymod_phy_access_t       *pm_phy;
    phymod_phy_power_t        phy_power;

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    pmc = &pc->phymod_ctrl;
    pm_phy = &pmc->phy[0]->pm_phy;
    SOC_IF_ERROR_RETURN
        (phymod_phy_power_get(pm_phy, &phy_power));
    if ((phy_power.tx == phymodPowerOff) && (phy_power.rx == phymodPowerOff)) {
        *enable = 0;
    } else if ((phy_power.tx == phymodPowerOn) && (phy_power.rx == phymodPowerOn)) {
        *enable = 1;
    } else if ((phy_power.tx == phymodPowerOffOn) && (phy_power.rx == phymodPowerOffOn)){
        *enable = 1;
    } else {
        *enable = 0;
    }

    return (SOC_E_NONE);
}


/*
 * Function:
 *      phy_tsce_duplex_get
 * Purpose:
 *      Get PHY duplex mode
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 *      duplex - current duplex mode
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_duplex_get(int unit, soc_port_t port, int *duplex)
{
    *duplex = TRUE;
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_tsce_speed_set
 * Purpose:
 *      Set PHY speed
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 *      speed - link speed in Mbps
 * Returns:     
 *      SOC_E_NONE
 */

STATIC int
phy_tsce_speed_set(int unit, soc_port_t port, int speed)
{
    phy_ctrl_t                *pc;
    tsce_config_t             *pCfg;
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy;
    int                       idx;
    tsce_speed_config_t       speed_config;
    phymod_phy_inf_config_t   interface_config;

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    if (speed == 0) {
        return SOC_E_NONE; 
    }

    pmc = &pc->phymod_ctrl;
    pCfg = (tsce_config_t *) pc->driver_data;

    /* take a copy of core and phy configuration values, and set the desired speed */
    sal_memcpy(&speed_config, &pCfg->speed_config, sizeof(speed_config));
    speed_config.speed = speed;

    /*next check if we need to run 1G at 6.25VCO */
    if (pCfg->serdes_1000x_at_6250_vco) {
        speed_config.pll_divider_req = 40;
    }

    /* determine the interface configuration */
    SOC_IF_ERROR_RETURN
        (tsce_speed_to_interface_config_get(&speed_config, &interface_config));

    /* now loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        phy = pmc->phy[idx];
        if (phy == NULL) {
            return SOC_E_INTERNAL;
        }

        /* note that the flags have an option to indicate whether it's ok to reprogram the PLL */
        SOC_IF_ERROR_RETURN
            (phymod_phy_interface_config_set(&phy->pm_phy,
                                             0 /* flags */, &interface_config));
    }

    /* record success */
    pCfg->speed_config.speed = speed;

    return (SOC_E_NONE);
}

/*
 * Function:
 *      phy_tsce_speed_get
 * Purpose:
 *      Get PHY speed
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 *      speed - current link speed in Mbps
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_speed_get(int unit, soc_port_t port, int *speed)
{
    phy_ctrl_t                *pc;
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy;
    phymod_phy_inf_config_t   interface_config;
    int flag = 0;

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    pmc = &pc->phymod_ctrl;

    /* initialize the data structure */
    interface_config.data_rate = 0;

    /* now loop through all cores */
    phy = pmc->phy[0];
    if (phy == NULL) {
        return SOC_E_INTERNAL;
    }
    /* note that the flags have an option to indicate whether it's ok to reprogram the PLL */
    SOC_IF_ERROR_RETURN
        (phymod_phy_interface_config_get(&phy->pm_phy,
                                         flag, &interface_config));
    *speed = interface_config.data_rate;
    return (SOC_E_NONE);
}

/*
 * Function:    
 *      phy_tsce_an_set
 * Purpose:     
 *      Enable or disable auto-negotiation on the specified port.
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 *      an   - Boolean, if true, auto-negotiation is enabled 
 *              (and/or restarted). If false, autonegotiation is disabled.
 * Returns:     
 *      SOC_E_XXX  _soc_triumph_tx
 */
STATIC int
phy_tsce_an_set(int unit, soc_port_t port, int enable)
{
    phy_ctrl_t                *pc;
    tsce_config_t             *pCfg;
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy;
    phymod_autoneg_control_t  an;
    soc_info_t *si;

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    phymod_autoneg_control_t_init(&an);
    pmc = &pc->phymod_ctrl;
    pCfg = (tsce_config_t *) pc->driver_data;
    si = &SOC_INFO(unit);

    /* only request autoneg on the first core */
    phy = pmc->phy[0];
    if (phy == NULL) {
        return SOC_E_INTERNAL;
    }
    an.enable = enable;
    an.num_lane_adv = si->port_num_lanes[port];
    an.an_mode = phymod_AN_MODE_NONE;
    if(pCfg->cl73an) {
        switch (pCfg->cl73an) {
            case TEMOD_CL73_W_BAM:  
                an.an_mode = phymod_AN_MODE_CL73BAM;  
                break ;
            case TEMOD_CL73_WO_BAM:
                an.an_mode = phymod_AN_MODE_CL73;  
                break ;
            case TEMOD_CL73_HPAM_VS_SW:    
                an.an_mode = phymod_AN_MODE_HPAM;  
                break ;  /* against TD+ */
            case TEMOD_CL73_HPAM:
                an.an_mode = phymod_AN_MODE_HPAM;  
                break ;  /* against TR3 */
            case TEMOD_CL73_CL37:    
                an.an_mode = phymod_AN_MODE_CL73;  
                break ;
            default:
                break;
        }
    } else if (pCfg->cl37an) {
        switch (pCfg->cl37an) {
            case TEMOD_CL37_WO_BAM:
                an.an_mode = phymod_AN_MODE_CL37;
                break;
            case TEMOD_CL37_W_BAM:
                an.an_mode = phymod_AN_MODE_CL37BAM;
                break;
            default:
                break;
        } 
    } else {
         if(pCfg->hg_mode) {
            an.an_mode = phymod_AN_MODE_CL37BAM;  /* default mode */
         } else {        
            if(pCfg->fiber_pref) {
                an.an_mode = phymod_AN_MODE_CL37;
            } else {
                an.an_mode = phymod_AN_MODE_SGMII;
            }
         }
    }
    SOC_IF_ERROR_RETURN
        (phymod_phy_autoneg_set(&phy->pm_phy, &an));

    return (SOC_E_NONE);
}

/*
 * Function:    
 *      phy_tsce_an_get
 * Purpose:     
 *      Get the current auto-negotiation status (enabled/busy)
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 *      an   - (OUT) if true, auto-negotiation is enabled.
 *      an_done - (OUT) if true, auto-negotiation is complete. This
 *              value is undefined if an == false.
 * Returns:     
 *      SOC_E_XXX
 */

STATIC int
phy_tsce_an_get(int unit, soc_port_t port, int *an, int *an_done)
{
    phy_ctrl_t* pc; 
    soc_phymod_ctrl_t *pmc;
    phymod_phy_access_t *pm_phy;
    phymod_autoneg_control_t an_control;
    int idx, an_complete;

    pc = INT_PHY_SW_STATE(unit, port);
    pmc = &pc->phymod_ctrl;

    PHYMOD_MEMSET(&an_control, 0x0, sizeof(an_control));

    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;
        SOC_IF_ERROR_RETURN
            (phymod_phy_autoneg_get(pm_phy, &an_control, (uint32_t *) &an_complete)); 
    }

    if (an_control.enable) {
        *an = 1;
        *an_done = an_complete;
    } else {
        *an = 0;
        *an_done = 0;
    }

    return (SOC_E_NONE);
}

/*
 * Function:
 *      phy_tsce_ability_advert_set
 * Purpose:
 *      Set the current advertisement for auto-negotiation.
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number.
 *      ability - Port mode mask indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The advertisement is set only for the ACTIVE medium.
 *      No synchronization performed at this level.
 */

STATIC int
phy_tsce_ability_advert_set(int unit, soc_port_t port,
                          soc_port_ability_t *ability)
{
    phy_ctrl_t                *pc;
    tsce_config_t             *pCfg;
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy;
    int                       port_num_lanes;
    int                       line_interface;
    uint32_t                  an_tech_ability;
    uint32_t                  an_bam37_ability;
    uint32_t                  an_bam73_ability;
    _shr_port_mode_t          speed_full_duplex;
    phymod_autoneg_ability_t  phymod_autoneg_ability; 

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    phymod_autoneg_ability_t_init(&phymod_autoneg_ability);

    pmc = &pc->phymod_ctrl;
    pCfg = (tsce_config_t *) pc->driver_data;

    /* only set abilities on the first core */
    phy = pmc->phy[0];
    if (phy == NULL) {
        return SOC_E_INTERNAL;
    }

    pCfg = (tsce_config_t *) pc->driver_data;
    port_num_lanes = pCfg->speed_config.port_num_lanes;
    line_interface = pCfg->speed_config.line_interface;

    an_tech_ability  = 0;
    an_bam37_ability = 0;
    an_bam73_ability = 0;
    speed_full_duplex = ability->speed_full_duplex;

    sal_memset(ability, 0, sizeof(*ability));
    /* 
         an_tech_ability
    */
    if (port_num_lanes == 4) {
        if (speed_full_duplex & SOC_PA_SPEED_40GB) {
            if (line_interface & PHYMOD_IF_CR4) {
                PHYMOD_AN_TECH_ABILITY_40G_CR4_SET(an_tech_ability);
            } else {
                PHYMOD_AN_TECH_ABILITY_40G_KR4_SET(an_tech_ability);
            }
        }
        if(speed_full_duplex & SOC_PA_SPEED_10GB) {
            PHYMOD_AN_TECH_ABILITY_10G_KX4_SET(an_tech_ability);
        }
    } else if (port_num_lanes == 2) {
        if(speed_full_duplex & SOC_PA_SPEED_20GB) {
            if (line_interface & PHYMOD_IF_CR4) {
                PHYMOD_BAM_CL73_ABL_20G_CR2_SET(an_bam73_ability);
            } else {
                PHYMOD_BAM_CL73_ABL_20G_KR2_SET(an_bam73_ability);
            }
        }
    }

    /* 
         an_bam37_ability
    */
    if (port_num_lanes == 4) {          /* 4 lanes */

        if(speed_full_duplex & SOC_PA_SPEED_40GB)
            PHYMOD_BAM_CL37_ABL_40G_SET(an_bam37_ability);
        /* if(speed_full_duplex & SOC_PA_SPEED_33GB)
            an_bam37_ability |= (1<<PHYMOD_BAM37ABL_32P7G); */
         if(speed_full_duplex & SOC_PA_SPEED_30GB)
            PHYMOD_BAM_CL37_ABL_31P5G_SET(an_bam37_ability);
        if(speed_full_duplex & SOC_PA_SPEED_25GB)
            PHYMOD_BAM_CL37_ABL_25P455G_SET(an_bam37_ability);
        if(speed_full_duplex & SOC_PA_SPEED_21GB)
            PHYMOD_BAM_CL37_ABL_21G_X4_SET(an_bam37_ability);
        if(speed_full_duplex & SOC_PA_SPEED_20GB){
            PHYMOD_BAM_CL37_ABL_20G_X4_SET(an_bam37_ability);
            PHYMOD_BAM_CL37_ABL_20G_X4_CX4_SET(an_bam37_ability);
        }
        if(speed_full_duplex & SOC_PA_SPEED_16GB)
            PHYMOD_BAM_CL37_ABL_16G_X4_SET(an_bam37_ability);
        if(speed_full_duplex & SOC_PA_SPEED_15GB)
            PHYMOD_BAM_CL37_ABL_15G_X4_SET(an_bam37_ability);
        if(speed_full_duplex & SOC_PA_SPEED_13GB)
            PHYMOD_BAM_CL37_ABL_13G_X4_SET(an_bam37_ability);
        if(speed_full_duplex & SOC_PA_SPEED_12P5GB)
            PHYMOD_BAM_CL37_ABL_12P5_X4_SET(an_bam37_ability);
        if(speed_full_duplex & SOC_PA_SPEED_12GB)
            PHYMOD_BAM_CL37_ABL_12G_X4_SET(an_bam37_ability);
        if(speed_full_duplex & SOC_PA_SPEED_10GB) {
            if (pCfg->cx4_10g) {
                PHYMOD_BAM_CL37_ABL_10G_CX4_SET(an_bam37_ability);
            } else {
                PHYMOD_BAM_CL37_ABL_10G_HIGIG_SET(an_bam37_ability);
            }
        }     
        if(speed_full_duplex & SOC_PA_SPEED_6000MB)
            PHYMOD_BAM_CL37_ABL_6G_X4_SET(an_bam37_ability);
        if(speed_full_duplex & SOC_PA_SPEED_5000MB)
            PHYMOD_BAM_CL37_ABL_5G_X4_SET(an_bam37_ability);
        if(speed_full_duplex & SOC_PA_SPEED_2500MB)
            PHYMOD_BAM_CL37_ABL_2P5G_SET(an_bam37_ability);
        if(speed_full_duplex & SOC_PA_SPEED_1000MB)
            PHYMOD_AN_TECH_ABILITY_1G_KX_SET(an_tech_ability);

        

    } else if (port_num_lanes == 2) {     /* 2 lanes */

        if(speed_full_duplex & SOC_PA_SPEED_20GB){
            PHYMOD_BAM_CL37_ABL_20G_X2_SET(an_bam37_ability);
            PHYMOD_BAM_CL37_ABL_20G_X2_CX4_SET(an_bam37_ability);
        }
        if (speed_full_duplex & SOC_PA_SPEED_16GB)
            PHYMOD_BAM_CL37_ABL_15P75G_R2_SET(an_bam37_ability);
        if (speed_full_duplex & SOC_PA_SPEED_13GB)
            PHYMOD_BAM_CL37_ABL_12P7_DXGXS_SET(an_bam37_ability);
        if (speed_full_duplex & SOC_PA_SPEED_11GB)
            PHYMOD_BAM_CL37_ABL_10P5G_DXGXS_SET(an_bam37_ability);
        if (speed_full_duplex & SOC_PA_SPEED_10GB){
            PHYMOD_BAM_CL37_ABL_10G_X2_CX4_SET(an_bam37_ability);
            PHYMOD_BAM_CL37_ABL_10G_DXGXS_SET(an_bam37_ability);
        }
        if (speed_full_duplex & SOC_PA_SPEED_2500MB)
            PHYMOD_BAM_CL37_ABL_2P5G_SET(an_bam37_ability);
        if (speed_full_duplex & SOC_PA_SPEED_1000MB)
            PHYMOD_AN_TECH_ABILITY_1G_KX_SET(an_tech_ability);

        

    } else {                              /* 1 lane */

        if (speed_full_duplex & SOC_PA_SPEED_10GB)
            PHYMOD_AN_TECH_ABILITY_10G_KR_SET(an_tech_ability);
        if (speed_full_duplex & SOC_PA_SPEED_2500MB)
            PHYMOD_BAM_CL37_ABL_2P5G_SET(an_bam37_ability);
        if (speed_full_duplex & SOC_PA_SPEED_1000MB)
            PHYMOD_AN_TECH_ABILITY_1G_KX_SET(an_tech_ability);

        
    }
    phymod_autoneg_ability.an_tech_ability = an_tech_ability;
    phymod_autoneg_ability.cl73bam_ability = an_bam73_ability; 
    phymod_autoneg_ability.cl37bam_ability = an_bam37_ability; 

    

    switch (ability->pause & (SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX)) {
    case SOC_PA_PAUSE_TX:
        PHYMOD_AN_CAPABILITIES_ASYM_PAUSE_SET(&phymod_autoneg_ability);
        break;
    case SOC_PA_PAUSE_RX:
        /* an_adv |= MII_ANA_C37_PAUSE | MII_ANA_C37_ASYM_PAUSE; */
        PHYMOD_AN_CAPABILITIES_ASYM_PAUSE_SET(&phymod_autoneg_ability);
        PHYMOD_AN_CAPABILITIES_SYMM_PAUSE_SET(&phymod_autoneg_ability);
        break;
    case SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX:
        PHYMOD_AN_CAPABILITIES_SYMM_PAUSE_SET(&phymod_autoneg_ability);
        break;
    }

    /* also set the sgmii speed */
    if(ability->speed_full_duplex & SOC_PA_SPEED_1000MB) {
        PHYMOD_AN_CAPABILITIES_SGMII_SET(&phymod_autoneg_ability);
        phymod_autoneg_ability.sgmii_speed = phymod_CL37_SGMII_1000M;
    } else if(ability->speed_full_duplex & SOC_PA_SPEED_100MB) {
        PHYMOD_AN_CAPABILITIES_SGMII_SET(&phymod_autoneg_ability);
        phymod_autoneg_ability.sgmii_speed = phymod_CL37_SGMII_100M;
    } else if(ability->speed_full_duplex & SOC_PA_SPEED_10MB) {
        PHYMOD_AN_CAPABILITIES_SGMII_SET(&phymod_autoneg_ability);
        phymod_autoneg_ability.sgmii_speed = phymod_CL37_SGMII_10M;
    } else {
        PHYMOD_AN_CAPABILITIES_SGMII_SET(&phymod_autoneg_ability);
        phymod_autoneg_ability.sgmii_speed = phymod_CL37_SGMII_1000M;
    }

    /* next check if we need to set cl37 attribute */
    if (pCfg->an_cl72) {
        phymod_autoneg_ability.an_cl72 = 1;
    }
    if (pCfg->hg_mode) {
        phymod_autoneg_ability.an_hg2 = 1;
    }
    if (pCfg->an_fec) {
        phymod_autoneg_ability.an_fec = 1;
    }
    SOC_IF_ERROR_RETURN
        (phymod_phy_autoneg_ability_set(&phy->pm_phy, &phymod_autoneg_ability));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_tsce_ability_advert_get
 * Purpose:
 *      Get the current advertisement for auto-negotiation.
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number.
 *      ability - (OUT) Port mode mask indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The advertisement is retrieved for the ACTIVE medium.
 *      No synchronization performed at this level.
 */

STATIC int
phy_tsce_ability_advert_get(int unit, soc_port_t port,
                              soc_port_ability_t *ability)
{
    phy_ctrl_t                *pc;
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy;
    int                       reg37_ability;
    int                       reg73_ability;
    int                       reg_ability;
    _shr_port_mode_t          speed_full_duplex;
    phymod_autoneg_ability_t  phymod_autoneg_ability; 

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    pmc = &pc->phymod_ctrl;

    /* only get abilities from the first core */
    phy = pmc->phy[0];
    if (phy == NULL) {
        return SOC_E_INTERNAL;
    }

    phymod_autoneg_ability_t_init(&phymod_autoneg_ability);
    SOC_IF_ERROR_RETURN
        (phymod_phy_autoneg_ability_get(&phy->pm_phy, &phymod_autoneg_ability));

    
    speed_full_duplex = 0;

    /* retrieve CL73 abilities */
    reg73_ability = phymod_autoneg_ability.an_tech_ability;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_40G_CR4(reg73_ability) ?SOC_PA_SPEED_40GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_40G_KR4(reg73_ability) ?SOC_PA_SPEED_40GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_10G_KR(reg73_ability)  ?SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_10G_KX4(reg73_ability) ?SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_1G_KX(reg73_ability)   ?SOC_PA_SPEED_1000MB:0;

    /* retrieve CL73bam abilities */
    reg73_ability = phymod_autoneg_ability.cl73bam_ability;
    speed_full_duplex|= PHYMOD_BAM_CL73_ABL_IS_20G_CR2(reg73_ability) ?SOC_PA_SPEED_20GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL73_ABL_IS_20G_KR2(reg73_ability) ?SOC_PA_SPEED_20GB:0;
    
    /* retrieve CL37 abilities */
    reg37_ability = phymod_autoneg_ability.cl37bam_ability;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_40G(reg37_ability) ? SOC_PA_SPEED_40GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_31P5G(reg37_ability)? SOC_PA_SPEED_30GB:0; 
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_32P7G(reg37_ability)? SOC_PA_SPEED_32GB:0; 
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_25P455G(reg37_ability) ?    SOC_PA_SPEED_25GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_21G_X4(reg37_ability)?     SOC_PA_SPEED_21GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_20G_X2_CX4(reg37_ability)? SOC_PA_SPEED_20GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_20G_X2(reg37_ability)?     SOC_PA_SPEED_20GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_20G_X4(reg37_ability)?     SOC_PA_SPEED_20GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_20G_X4_CX4(reg37_ability)? SOC_PA_SPEED_20GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_16G_X4(reg37_ability)?     SOC_PA_SPEED_16GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_15P75G_R2(reg37_ability)?  SOC_PA_SPEED_16GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_15G_X4(reg37_ability)?     SOC_PA_SPEED_15GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_13G_X4(reg37_ability)?     SOC_PA_SPEED_13GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_12P7_DXGXS(reg37_ability)? SOC_PA_SPEED_13GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_12P5_X4(reg37_ability)?    SOC_PA_SPEED_12P5GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_12G_X4(reg37_ability)?     SOC_PA_SPEED_12GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_10P5G_DXGXS(reg37_ability)?SOC_PA_SPEED_11GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_10G_X2_CX4(reg37_ability)? SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_10G_DXGXS(reg37_ability)?  SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_10G_CX4(reg37_ability)?    SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_10G_HIGIG(reg37_ability)?  SOC_PA_SPEED_10GB:0; /* 4-lane */
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_6G_X4(reg37_ability)?      SOC_PA_SPEED_6000MB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_5G_X4(reg37_ability)?      SOC_PA_SPEED_5000MB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_2P5G(reg37_ability)?       SOC_PA_SPEED_2500MB:0;
    speed_full_duplex|= SOC_PA_SPEED_1000MB ;

    /* retrieve "pause" abilities */
    reg_ability = phymod_autoneg_ability.capabilities;

    ability->pause = 0;
    if (reg_ability == PHYMOD_AN_CAPABILITIES_ASYM_PAUSE) {
        ability->pause = SOC_PA_PAUSE_TX;
    } else if (reg_ability == (PHYMOD_AN_CAPABILITIES_SYMM_PAUSE|PHYMOD_AN_CAPABILITIES_ASYM_PAUSE)) {
        ability->pause = SOC_PA_PAUSE_RX;
    } else if (reg_ability == PHYMOD_AN_CAPABILITIES_SYMM_PAUSE) {
        ability->pause = (SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX);
    }

    ability->speed_full_duplex = speed_full_duplex;

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_tsce_ability_remote_get
 * Purpose:
 *      Get the current advertisement for auto-negotiation.
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number.
 *      ability - (OUT) Port mode mask indicating supported options/speeds.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      The advertisement is retrieved for the ACTIVE medium.
 *      No synchronization performed at this level.
 */

STATIC int
phy_tsce_ability_remote_get(int unit, soc_port_t port,
                               soc_port_ability_t *ability)
{
    phy_ctrl_t                *pc;
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy;
    int                       reg37_ability;
    int                       reg73_ability;
    int                       reg_ability;
    _shr_port_mode_t          speed_full_duplex;
    phymod_autoneg_ability_t  phymod_autoneg_ability; 

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    pmc = &pc->phymod_ctrl;

    /* only get abilities from the first core */
    phy = pmc->phy[0];
    if (phy == NULL) {
        return SOC_E_INTERNAL;
    }

    phymod_autoneg_ability_t_init(&phymod_autoneg_ability);
    SOC_IF_ERROR_RETURN
        (phymod_phy_autoneg_remote_ability_get(&phy->pm_phy, &phymod_autoneg_ability));

    
    speed_full_duplex = 0;

    /* retrieve CL73 abilities */
    reg73_ability = phymod_autoneg_ability.an_tech_ability;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_40G_CR4(reg73_ability) ?SOC_PA_SPEED_40GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_40G_KR4(reg73_ability) ?SOC_PA_SPEED_40GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_10G_KR(reg73_ability)  ?SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_10G_KX4(reg73_ability) ?SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_1G_KX(reg73_ability)   ?SOC_PA_SPEED_1000MB:0;

    /* retrieve CL37 abilities */
    reg73_ability = phymod_autoneg_ability.cl73bam_ability;
    speed_full_duplex|= PHYMOD_BAM_CL73_ABL_IS_20G_CR2(reg73_ability) ?SOC_PA_SPEED_20GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL73_ABL_IS_20G_KR2(reg73_ability) ?SOC_PA_SPEED_20GB:0;
 
    /* retrieve CL37 abilities */
    reg37_ability = phymod_autoneg_ability.cl37bam_ability;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_40G(reg37_ability) ? SOC_PA_SPEED_40GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_31P5G(reg37_ability)? SOC_PA_SPEED_30GB:0; 
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_32P7G(reg37_ability)? SOC_PA_SPEED_32GB:0; 
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_25P455G(reg37_ability) ?    SOC_PA_SPEED_25GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_21G_X4(reg37_ability)?     SOC_PA_SPEED_21GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_20G_X2_CX4(reg37_ability)? SOC_PA_SPEED_20GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_20G_X2(reg37_ability)?     SOC_PA_SPEED_20GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_20G_X4(reg37_ability)?     SOC_PA_SPEED_20GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_20G_X4_CX4(reg37_ability)? SOC_PA_SPEED_20GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_16G_X4(reg37_ability)?     SOC_PA_SPEED_16GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_15P75G_R2(reg37_ability)?  SOC_PA_SPEED_16GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_15G_X4(reg37_ability)?     SOC_PA_SPEED_15GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_13G_X4(reg37_ability)?     SOC_PA_SPEED_13GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_12P7_DXGXS(reg37_ability)? SOC_PA_SPEED_13GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_12P5_X4(reg37_ability)?    SOC_PA_SPEED_12P5GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_12G_X4(reg37_ability)?     SOC_PA_SPEED_12GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_10P5G_DXGXS(reg37_ability)?SOC_PA_SPEED_11GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_10G_X2_CX4(reg37_ability)? SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_10G_DXGXS(reg37_ability)?  SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_10G_CX4(reg37_ability)?    SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_10G_HIGIG(reg37_ability)?  SOC_PA_SPEED_10GB:0; /* 4-lane */
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_6G_X4(reg37_ability)?      SOC_PA_SPEED_6000MB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_5G_X4(reg37_ability)?      SOC_PA_SPEED_5000MB:0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_2P5G(reg37_ability)?       SOC_PA_SPEED_2500MB:0;
    speed_full_duplex|= SOC_PA_SPEED_1000MB ;

    /* retrieve "pause" abilities */
    reg_ability = phymod_autoneg_ability.capabilities;

    ability->pause = 0;
    if (reg_ability == PHYMOD_AN_CAPABILITIES_ASYM_PAUSE) {
        ability->pause = SOC_PA_PAUSE_TX;
    } else if (reg_ability == (PHYMOD_AN_CAPABILITIES_SYMM_PAUSE|PHYMOD_AN_CAPABILITIES_ASYM_PAUSE)) {
        ability->pause = SOC_PA_PAUSE_RX;
    } else if (reg_ability == PHYMOD_AN_CAPABILITIES_SYMM_PAUSE) {
        ability->pause = (SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX);
    }

    ability->speed_full_duplex = speed_full_duplex;

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_tsce_lb_set
 * Purpose:
 *      Enable/disable PHY loopback mode
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 *      enable - binary value for on/off (1/0)
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_lb_set(int unit, soc_port_t port, int enable)
{
    phy_ctrl_t* pc; 
    soc_phymod_ctrl_t *pmc;
    phymod_phy_access_t *pm_phy;
    int idx;

    pc = INT_PHY_SW_STATE(unit, port);
    pmc = &pc->phymod_ctrl;

    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;
        SOC_IF_ERROR_RETURN
            (phymod_phy_loopback_set(pm_phy, phymodLoopbackGlobal, enable)); 
    }

    return (SOC_E_NONE);
}

/*
 * Function:
 *      phy_tsce_lb_get
 * Purpose:
 *      Get current PHY loopback mode
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 *      enable - address of location to store binary value for on/off (1/0)
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_lb_get(int unit, soc_port_t port, int *enable)
{
    phy_ctrl_t* pc; 
    soc_phymod_ctrl_t *pmc;
    phymod_phy_access_t *pm_phy;
    uint32 lb_enable;
    int idx;

    pc = INT_PHY_SW_STATE(unit, port);
    pmc = &pc->phymod_ctrl;

    *enable = 0;

    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;
        SOC_IF_ERROR_RETURN
            (phymod_phy_loopback_get(pm_phy, phymodLoopbackGlobal, &lb_enable));
        if (lb_enable) {
            *enable = 1;
        }
        /* Check first PHY only */
        break;
    }
     
    return (SOC_E_NONE);
}

/*
 * Function:
 *      phy_tsce_ability_local_get
 * Purpose:
 *      xx
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 *      ability - xx
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    phy_ctrl_t          *pc; 
    /*int                 hg_mode;
    int                 max_speed;
    int                 cfg_init_speed;
    */
    tsce_config_t             *pCfg;

    pc = INT_PHY_SW_STATE(unit, port);
    pCfg = (tsce_config_t *) pc->driver_data;

    if (NULL == ability) {
        return SOC_E_PARAM;
    }

    sal_memset(ability, 0, sizeof(*ability));

    ability->loopback  = SOC_PA_LB_PHY;
    ability->medium    = SOC_PA_MEDIUM_FIBER;
    ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
    ability->flags     = SOC_PA_AUTONEG;

    /* $$$ what does this logic do ? */
#if 0
    if(pCfg->init_speed < pc->speed_max ) max_speed = pCfg->init_speed ;
    else                                  max_speed = pc->max_speed ;
#endif

    if (IND_LANE_MODE(pc)) { 
        ability->speed_full_duplex  = SOC_PA_SPEED_1000MB;
        if (pCfg->speed_config.fiber_pref) {
            ability->speed_full_duplex  |= SOC_PA_SPEED_2500MB |
                                           SOC_PA_SPEED_10GB;
                ability->speed_full_duplex  |= SOC_PA_SPEED_100MB;
                ability->speed_half_duplex  = SOC_PA_SPEED_100MB;
        } else {
            ability->speed_half_duplex  = SOC_PA_SPEED_10MB |
                                          SOC_PA_SPEED_100MB;
            ability->speed_full_duplex  |= SOC_PA_SPEED_10MB |
                                           SOC_PA_SPEED_100MB;
        } 
        switch(pc->speed_max) {  /* must include all the supported speedss */
            case 21000:
                ability->speed_full_duplex |= SOC_PA_SPEED_21GB;
                /* fall through */
            case 20000:
                ability->speed_full_duplex |= SOC_PA_SPEED_20GB;
                /* fall through */
            case 16000: 
                ability->speed_full_duplex |= SOC_PA_SPEED_16GB; 
                /* fall through */
                /* not supported speed
                  case 15750:
                  ability->speed_full_duplex |= SOC_PA_SPEED_15P75GB; */
                /* fall through */
            case 15000:
                ability->speed_full_duplex |= SOC_PA_SPEED_15GB;
                /* fall through */
                /* not supported speed
                   case 12730:
                   ability->speed_full_duplex |= SOC_PA_SPEED_12P73GB;  */
                /* fall through */
            case 12000:
                ability->speed_full_duplex |= SOC_PA_SPEED_12GB;
                /* fall through */
            case 11000:
                ability->speed_full_duplex |= SOC_PA_SPEED_11GB;
                /* not supported speed 
                   case 10500: 
                   ability->speed_full_duplex |= SOC_PA_SPEED_10P5GB; */
                /* fall through */
            case 10000:
                ability->speed_full_duplex |= SOC_PA_SPEED_10GB;
                /* fall through */
            case 5000:
                ability->speed_full_duplex |= SOC_PA_SPEED_5000MB;
                /* fall through */
            default:
                break;
        }
        ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
        ability->interface = SOC_PA_INTF_GMII | SOC_PA_INTF_SGMII;
        if (pc->speed_max >= 10000) {
            ability->interface |= SOC_PA_INTF_XGMII;
        } 
        ability->medium    = SOC_PA_MEDIUM_FIBER;
        ability->loopback  = SOC_PA_LB_PHY;
        /* if dual lane port */
        if (IS_DUAL_LANE_PORT(pc)) {
            ability->flags     = 0;
        } else {
            ability->flags     = SOC_PA_AUTONEG;
        }
    } else {
        ability->speed_half_duplex  = SOC_PA_ABILITY_NONE;
        ability->speed_full_duplex  = SOC_PA_SPEED_1000MB;
        if (pCfg->fiber_pref)   {
            ability->speed_full_duplex  |= SOC_PA_SPEED_2500MB;
                ability->speed_full_duplex  |= SOC_PA_SPEED_100MB;
                ability->speed_half_duplex  = SOC_PA_SPEED_100MB;
        } else {
            ability->speed_half_duplex  = SOC_PA_SPEED_10MB |
                                      SOC_PA_SPEED_100MB;
            ability->speed_full_duplex  |= SOC_PA_SPEED_10MB |
                                       SOC_PA_SPEED_100MB;
        }
        switch(pc->speed_max) {
            case 127000:
                ability->speed_full_duplex |= SOC_PA_SPEED_127GB;
            case 106000:
                ability->speed_full_duplex |= SOC_PA_SPEED_106GB;
            case 120000:
                ability->speed_full_duplex |= SOC_PA_SPEED_120GB;
            case 100000:
                ability->speed_full_duplex |= SOC_PA_SPEED_100GB;
            case 42000:
                ability->speed_full_duplex |= SOC_PA_SPEED_42GB;
            case 40000:
                ability->speed_full_duplex |= SOC_PA_SPEED_40GB;
            case 30000:
                ability->speed_full_duplex |= SOC_PA_SPEED_30GB;
            case 25000:
                ability->speed_full_duplex |= SOC_PA_SPEED_25GB;
            case 21000:
                ability->speed_full_duplex |= SOC_PA_SPEED_21GB;
            case 20000:
                ability->speed_full_duplex |= SOC_PA_SPEED_20GB;
                /* fall through */
            case 16000:
                ability->speed_full_duplex |= SOC_PA_SPEED_16GB; 
                /* fall through */
            case 15000:
                ability->speed_full_duplex |= SOC_PA_SPEED_15GB; 
                /* fall through */
            case 13000:
                ability->speed_full_duplex |= SOC_PA_SPEED_13GB;
                /* fall through */
            case 12000:
                ability->speed_full_duplex |= SOC_PA_SPEED_12GB;
                /* fall through */
            default:
                ability->speed_full_duplex |= SOC_PA_SPEED_10GB;
                break; 
        }
        ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
        ability->interface = SOC_PA_INTF_XGMII;
        ability->medium    = SOC_PA_MEDIUM_FIBER;
        ability->loopback  = SOC_PA_LB_PHY;
        ability->flags     = SOC_PA_AUTONEG;
    }

    LOG_INFO(BSL_LS_SOC_PHY,
             (BSL_META_U(pc->unit,
                         "phy_tsce_ability_local_get:unit=%d p=%d sp=%08x\n"),
              unit, port, ability->speed_full_duplex));
    return (SOC_E_NONE);
}

/*
 * Function:
 *      phy_tsce_firmware_set
 * Purpose:
 *      xx
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 

 * Returns:     
 *      SOC_E_NONE
 */

STATIC int
phy_tsce_firmware_set(int unit, soc_port_t port, int x, uint8 *y, int z)
{
   return (SOC_E_NONE);
}

STATIC int
phy_tsce_interface_set(int unit, soc_port_t port, soc_port_if_t pif)
{
    phy_ctrl_t      *pc;
    uint32 intf;
    tsce_config_t *pCfg;

    pc = INT_PHY_SW_STATE(unit, port);
    pCfg = (tsce_config_t *) pc->driver_data;

    if (pif > 31) {
        return SOC_E_PARAM;
    }

    /* need to check valid interfaces
     * ALL 40G/10G interfaces are mutually exclusive
     */
    intf = pCfg->line_intf;

    if (TSCE_40G_10G_INTF(pif)) {
        intf &= ~TSCE_40G_10G_INTF_ALL;  /* clear all 10G/40G interfaces */
        intf |= 1 << pif;
    } else {
        intf |= 1 << pif;
    }

    pCfg->line_intf = intf;
    return SOC_E_NONE;
}

STATIC int
phy_tsce_interface_get(int unit, soc_port_t port, soc_port_if_t *pif)
{
    phy_ctrl_t                *pc;
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy;
    phymod_phy_inf_config_t   interface_config;
    tsce_config_t *pCfg;
    int flag = 0;

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }
    pCfg = (tsce_config_t *) pc->driver_data;

    pmc = &pc->phymod_ctrl;

    /* initialize the data structure */
    PHYMOD_MEMSET(&interface_config, 0x0, sizeof(phymod_phy_inf_config_t));

    /* now loop through all cores */
    phy = pmc->phy[0];
    if (phy == NULL) {
        return SOC_E_INTERNAL;
    }
    /* note that the flags have an option to indicate whether it's ok to reprogram the PLL */
    SOC_IF_ERROR_RETURN
        (phymod_phy_interface_config_get(&phy->pm_phy,
                                         flag, &interface_config));
    switch (interface_config.interface_type) {
    case phymodInterfaceXFI:
        *pif = SOC_PORT_IF_XFI;
        break;
    case phymodInterfaceSFI:
        *pif = SOC_PORT_IF_SFI;
        break;
    case phymodInterfaceXLAUI:
        *pif = SOC_PORT_IF_XLAUI;
        break;
    case phymodInterface1000X:
        *pif = SOC_PORT_IF_GMII;
        break;
    case phymodInterfaceKX:
        *pif = SOC_PORT_IF_KX;
        break;
    case phymodInterfaceSGMII:
    {
        if(interface_config.data_rate == 1000) {
            if (PHY_EXTERNAL_MODE(unit, port) || !(pCfg->speed_config.fiber_pref)) {
                *pif = SOC_PORT_IF_SGMII;
            } else {
                *pif = SOC_PORT_IF_GMII;
            }
        } else {
            *pif = SOC_PORT_IF_SGMII;
        }
        break;
    }
    case phymodInterfaceRXAUI:
        *pif = SOC_PORT_IF_RXAUI;
        break;
    case phymodInterfaceXGMII:
        *pif = SOC_PORT_IF_XGMII;
        break;
    case phymodInterfaceKR4:
        *pif = SOC_PORT_IF_KR4;
        break;
    case phymodInterfaceKR:
    {
        int an, an_done;
        an = 0;
        an_done = 0;
        /* first check if an is enable or not */
        phy_tsce_an_get(unit, port, &an, &an_done);
        if (!an) { 
            if (PHY_EXTERNAL_MODE(unit, port) || !(pCfg->speed_config.fiber_pref)) {
                *pif = SOC_PORT_IF_XFI;
            } else {
                *pif = SOC_PORT_IF_SFI;
            }
        } else {
            *pif = SOC_PORT_IF_KR; 
        }
         break;
    }
    case phymodInterfaceCR4:
        *pif = SOC_PORT_IF_CR4;
        break;
    case phymodInterfaceCR10:
        *pif = SOC_PORT_IF_CAUI;
        break;
    case phymodInterfaceBypass:
        *pif = SOC_PORT_IF_ILKN;
        break;
    default:
        *pif =  SOC_PORT_IF_XGMII;
        break;
    }
    return (SOC_E_NONE);

}


/*
 * Function:
 *      tsce_uc_status_dump
 * Purpose:
 *      display all the serdes related parameters
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 

 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
tsce_uc_status_dump(int unit, soc_port_t port, void *arg)
{
    phy_ctrl_t* pc; 
    soc_phymod_ctrl_t *pmc;
    phymod_phy_access_t *pm_phy;
    int idx;

    pc = INT_PHY_SW_STATE(unit, port);
    pmc = &pc->phymod_ctrl;

    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;
        SOC_IF_ERROR_RETURN
            (phymod_phy_pmd_info_dump(pm_phy, arg)); 
    }
    return (SOC_E_NONE);
}

/*
 * Function:
 *      tsce_pcs_status_dump
 * Purpose:
 *      display all the serdes related parameters
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 

 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
tsce_pcs_status_dump(int unit, soc_port_t port, void *arg)
{
    phy_ctrl_t* pc; 
    soc_phymod_ctrl_t *pmc;
    phymod_phy_access_t *pm_phy;
    int idx;

    pc = INT_PHY_SW_STATE(unit, port);
    pmc = &pc->phymod_ctrl;

    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;
        SOC_IF_ERROR_RETURN
            (phymod_phy_pcs_info_dump(pm_phy, arg)); 
    }
    return (SOC_E_NONE);
}


/* 
 * given a pc (phymod_ctrl_t) and logical lane number, 
 *    find the correct soc_phymod_phy_t object and lane
 */
STATIC int
_tsce_find_soc_phy_lane(soc_phymod_ctrl_t *pmc, uint32_t lane, 
                        soc_phymod_phy_t **p_phy, uint32 *lane_map)
{
    phymod_phy_access_t *pm_phy;
    int idx, lnx, ln_cnt, found;
    uint32 lane_map_copy;

    /* Traverse lanes belonging to this port */
    found = 0;
    ln_cnt = 0;
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;
        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }
        lane_map_copy = pm_phy->access.lane;
        for (lnx = 0; lnx < 4; lnx++) {
            if ((1 << lnx) & lane_map_copy) {
                if (ln_cnt == lane) {
                    found = 1;
                    break;
                }
                ln_cnt++;
            }
        }
        if (found) {
            *p_phy = pmc->phy[idx];
            *lane_map = (1 << lnx);
            break;
        }
    } 

    if (!found) {
        /* No such lane */
        return SOC_E_PARAM;
    }
    return (SOC_E_NONE);
}

/* 
 * tsce_per_lane_preemphasis_set
 */
STATIC int
tsce_per_lane_preemphasis_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_tx_t         phymod_tx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    SOC_IF_ERROR_RETURN(phymod_phy_tx_get(&pm_phy_copy, &phymod_tx));
    phymod_tx.pre = value;
    SOC_IF_ERROR_RETURN(phymod_phy_tx_set(&pm_phy_copy, &phymod_tx));

    return(SOC_E_NONE);
}

/* 
 * tsce_preemphasis_set
 */
STATIC int
tsce_preemphasis_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        phymod_tx.pre = value;
        SOC_IF_ERROR_RETURN(phymod_phy_tx_set(pm_phy, &phymod_tx));

    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_fir_pre_set
 */
STATIC int
tsce_tx_fir_pre_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        phymod_tx.pre = value;
        SOC_IF_ERROR_RETURN(phymod_phy_tx_set(pm_phy, &phymod_tx));
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_fir_main_set
 */
STATIC int
tsce_tx_fir_main_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        phymod_tx.main = value;
        SOC_IF_ERROR_RETURN(phymod_phy_tx_set(pm_phy, &phymod_tx));
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_fir_post_set
 */
STATIC int
tsce_tx_fir_post_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        phymod_tx.post = value;
        SOC_IF_ERROR_RETURN(phymod_phy_tx_set(pm_phy, &phymod_tx));
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_fir_post2_set
 */
STATIC int
tsce_tx_fir_post2_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        phymod_tx.post2 = value;
        SOC_IF_ERROR_RETURN(phymod_phy_tx_set(pm_phy, &phymod_tx));
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_fir_post3_set
 */
STATIC int
tsce_tx_fir_post3_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        phymod_tx.post3 = value;
        SOC_IF_ERROR_RETURN(phymod_phy_tx_set(pm_phy, &phymod_tx));
    }

    return(SOC_E_NONE);
}


/* 
 * tsce_per_lane_driver_current_set
 */
STATIC int
tsce_per_lane_driver_current_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_tx_t         phymod_tx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    SOC_IF_ERROR_RETURN(phymod_phy_tx_get(&pm_phy_copy, &phymod_tx));
    phymod_tx.amp = value;
    SOC_IF_ERROR_RETURN(phymod_phy_tx_set(&pm_phy_copy, &phymod_tx));

    return(SOC_E_NONE);
}

/* 
 * tsce_driver_current_set
 */
STATIC int
tsce_driver_current_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        phymod_tx.amp = value;
        SOC_IF_ERROR_RETURN(phymod_phy_tx_set(pm_phy, &phymod_tx));
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_per_lane_rx_dfe_tap_control_set
 */
STATIC int
tsce_per_lane_rx_dfe_tap_control_set(soc_phymod_ctrl_t *pmc, int lane, int tap, int enable, uint32 value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t          phymod_rx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    if ((tap<0)||(tap>COUNTOF(phymod_rx.dfe))) {
        /* this can only happen with a coding error */
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_rx_get(&pm_phy_copy, &phymod_rx));
    phymod_rx.dfe[tap].enable = enable;
    phymod_rx.dfe[tap].value = value;
    SOC_IF_ERROR_RETURN(phymod_phy_rx_set(&pm_phy_copy, &phymod_rx));

    return(SOC_E_NONE);
}


/* 
 * tsce_tx_lane_squelch
 */
STATIC int
tsce_tx_lane_squelch(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_phy_power_t  phy_power;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        if (value == 1) {
            phy_power.tx = phymodPowerOff;
            phy_power.rx = phymodPowerNoChange;
        } else {
            phy_power.tx = phymodPowerOn;
            phy_power.rx = phymodPowerNoChange;
        }
        SOC_IF_ERROR_RETURN
            (phymod_phy_power_set(pm_phy, &phy_power));
    }
    return(SOC_E_NONE);
}

/* 
 * tsce_per_lane_rx_peak_filter_set
 */
STATIC int
tsce_per_lane_rx_peak_filter_set(soc_phymod_ctrl_t *pmc, int lane, int enable, uint32 value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t         phymod_rx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    SOC_IF_ERROR_RETURN(phymod_phy_rx_get(&pm_phy_copy, &phymod_rx));
    phymod_rx.peaking_filter.enable = TRUE;
    phymod_rx.peaking_filter.value = value;
    SOC_IF_ERROR_RETURN(phymod_phy_rx_set(pm_phy, &phymod_rx));

    return(SOC_E_NONE);
}

/* 
 * tsce_rx_peak_filter_set
 */
STATIC int 
tsce_rx_peak_filter_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_rx_t         phymod_rx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_rx_get(pm_phy, &phymod_rx));
        phymod_rx.peaking_filter.enable = TRUE;
        phymod_rx.peaking_filter.value = value;
        SOC_IF_ERROR_RETURN(phymod_phy_rx_set(pm_phy, &phymod_rx));
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_per_lane_rx_vga_set
 */
STATIC int
tsce_per_lane_rx_vga_set(soc_phymod_ctrl_t *pmc, int lane, int enable, uint32 value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t         phymod_rx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    SOC_IF_ERROR_RETURN(phymod_phy_rx_get(&pm_phy_copy, &phymod_rx));
    phymod_rx.vga.enable = TRUE;
    phymod_rx.vga.value = value;
    SOC_IF_ERROR_RETURN(phymod_phy_rx_set(pm_phy, &phymod_rx));

    return(SOC_E_NONE);
}

/* 
 * tsce_rx_vga_set
 */
STATIC int 
tsce_rx_vga_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_rx_t         phymod_rx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_rx_get(pm_phy, &phymod_rx));
        phymod_rx.vga.enable = TRUE;
        phymod_rx.vga.value = value;
        SOC_IF_ERROR_RETURN(phymod_phy_rx_set(pm_phy, &phymod_rx));
    }

    return(SOC_E_NONE);
}

STATIC int
tsce_rx_tap_release(soc_phymod_ctrl_t *pmc, int tap)
{
    phymod_phy_access_t *pm_phy;
    phymod_rx_t         phymod_rx;
    int                 idx;

    /* bounds check "tap" */
    if ((tap < 0) || (tap > COUNTOF(phymod_rx.dfe))) {
        return SOC_E_INTERNAL;
    }

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }
        SOC_IF_ERROR_RETURN(phymod_phy_rx_get(pm_phy, &phymod_rx));
        phymod_rx.dfe[tap].enable = FALSE;
        SOC_IF_ERROR_RETURN(phymod_phy_rx_set(pm_phy, &phymod_rx));
    }
    return(SOC_E_NONE);
}

/* 
 * tsce_rx_tap_set
 */
STATIC int 
tsce_rx_tap_set(soc_phymod_ctrl_t *pmc, int tap, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_rx_t         phymod_rx;
    int                 idx;

    /* bounds check "tap" */
    if ((tap < 0) || (tap > COUNTOF(phymod_rx.dfe))) {
        return SOC_E_INTERNAL;
    }

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }
        SOC_IF_ERROR_RETURN(phymod_phy_rx_get(pm_phy, &phymod_rx));
        phymod_rx.dfe[tap].enable = TRUE;
        phymod_rx.dfe[tap].value = value;
        SOC_IF_ERROR_RETURN(phymod_phy_rx_set(pm_phy, &phymod_rx));
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_pi_control_set
 */
STATIC int 
tsce_pi_control_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_tx_override_t tx_override;
    int                  idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        phymod_tx_override_t_init(&tx_override);
        tx_override.phase_interpolator.enable = (value == 0) ? 0 : 1;
        tx_override.phase_interpolator.value = value;
        SOC_IF_ERROR_RETURN(phymod_phy_tx_override_set(pm_phy, &tx_override));
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_polarity_set
 */
STATIC int 
tsce_tx_polarity_set(soc_phymod_ctrl_t *pmc, phymod_polarity_t *cfg_polarity, uint32 value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_polarity_t    polarity;
    int                  idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        sal_memcpy(&polarity, cfg_polarity, sizeof(polarity));
        polarity.tx_polarity = value;
        SOC_IF_ERROR_RETURN(phymod_phy_polarity_set(pm_phy, &polarity));

        /* after successfully setting the parity, update the configured value */
        cfg_polarity->tx_polarity = value;
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_rx_polarity_set
 */

STATIC int 
tsce_rx_polarity_set(soc_phymod_ctrl_t *pmc, phymod_polarity_t *cfg_polarity, uint32 value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_polarity_t    polarity;
    int                  idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        sal_memcpy(&polarity, cfg_polarity, sizeof(polarity));
        polarity.rx_polarity = value;
        SOC_IF_ERROR_RETURN(phymod_phy_polarity_set(pm_phy, &polarity));

        /* after successfully setting the parity, update the configured value */
        cfg_polarity->rx_polarity = value;
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_rx_reset
 */
STATIC int
tsce_rx_reset(soc_phymod_ctrl_t *pmc, phymod_phy_reset_t *cfg_reset, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_phy_reset_t  reset;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        sal_memcpy(&reset, cfg_reset, sizeof(reset));
        reset.rx = (phymod_reset_direction_t) value;
        SOC_IF_ERROR_RETURN(phymod_phy_reset_set(pm_phy, &reset));

        /* after successfully setting the parity, update the configured value */
        cfg_reset->rx = (phymod_reset_direction_t) value;
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_reset
 */
STATIC int
tsce_tx_reset(soc_phymod_ctrl_t *pmc, phymod_phy_reset_t *cfg_reset, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    phymod_phy_reset_t  reset;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        sal_memcpy(&reset, cfg_reset, sizeof(reset));
        reset.tx = (phymod_reset_direction_t) value;
        SOC_IF_ERROR_RETURN(phymod_phy_reset_set(pm_phy, &reset));

        /* after successfully setting the parity, update the configured value */
        cfg_reset->tx = (phymod_reset_direction_t) value;
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_cl72_enable_set
 */
STATIC int
tsce_cl72_enable_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t *pm_phy;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_cl72_set(pm_phy, value));
    }

    return(SOC_E_NONE);
}

STATIC int
tsce_lane_map_set(soc_phymod_ctrl_t *pmc, uint32 value){
    int idx;
    phymod_lane_map_t lane_map;

    lane_map.num_of_lanes = NUM_LANES;
    if(pmc->phy[0] == NULL){
        return(SOC_E_INTERNAL);
    }
    for (idx=0; idx < NUM_LANES; idx++) {
        lane_map.lane_map_rx[idx] = (value >> (idx * 4 /* 4 bit per lane */)) & 0xf;
    }
    for (idx=0; idx < NUM_LANES; idx++) {
        lane_map.lane_map_tx[idx] = (value >> (16 + idx * 4 /* 4 bit per lane */)) & 0xf;
    }
    SOC_IF_ERROR_RETURN(phymod_core_lane_map_set(&pmc->phy[0]->core->pm_core, &lane_map));
    return(SOC_E_NONE);
}


STATIC int
tsce_lane_map_get(soc_phymod_ctrl_t *pmc, uint32 *value){
    int idx;
    phymod_lane_map_t lane_map;

    *value = 0;
    if(pmc->phy[0] == NULL){
        return(SOC_E_INTERNAL);
    }
    SOC_IF_ERROR_RETURN(phymod_core_lane_map_get(&pmc->phy[0]->core->pm_core, &lane_map));
    if(lane_map.num_of_lanes != NUM_LANES){
        return SOC_E_INTERNAL;
    }
    for (idx=0; idx < NUM_LANES; idx++) {
        value += (lane_map.lane_map_rx[idx] << (idx * 4));
    }
    for (idx=0; idx < NUM_LANES; idx++) {
        value += (lane_map.lane_map_tx[idx]<< (idx * 4 + 16));
    }
    
    return(SOC_E_NONE);
}

/* 
 * tsce_firmware_mode_set
 */
STATIC int
tsce_firmware_mode_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t    *pm_phy;
    phymod_firmware_lane_config_t fw_config;
    int                    idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        if (pmc->phy[idx] == NULL) {
            return(SOC_E_INTERNAL);
        }

        pm_phy = &pmc->phy[idx]->pm_phy;
        if (pm_phy == NULL) {
            return(SOC_E_INTERNAL);
        }

        SOC_IF_ERROR_RETURN(phymod_phy_firmware_lane_config_get(pm_phy, &fw_config));

        
        switch (value) {
            case SOC_PHY_FIRMWARE_DEFAULT: 
                fw_config.LaneConfigFromPCS = 0;
                break;
            case SOC_PHY_FIRMWARE_FORCE_OSDFE:
                fw_config.DfeOn = 1;
                break;
            case SOC_PHY_FIRMWARE_FORCE_BRDFE:
                fw_config.ForceBrDfe = 1;
                break;
            case SOC_PHY_FIRMWARE_CL72_WITHOUT_AN:
                fw_config.Cl72Enable = 1;
                break;
            case SOC_PHY_FIRMWARE_SFP_DAC:
                fw_config.MediaType = phymodFirmwareMediaTypeCopperCable;
                break;
            case SOC_PHY_FIRMWARE_XLAUI:
                fw_config.MediaType = phymodFirmwareMediaTypePcbTraceBackPlane;
                break;
            case SOC_PHY_FIRMWARE_SFP_OPT_SR4:
                fw_config.MediaType = phymodFirmwareMediaTypeOptics;
                break;
            default:
                fw_config.LaneConfigFromPCS = 0;
                break;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_firmware_lane_config_set(pm_phy, fw_config));
    }

    return(SOC_E_NONE);
}



STATIC int
tsce_pattern_len_set(soc_phymod_ctrl_t *pmc, uint32_t value)
{
    phymod_phy_access_t    *pm_phy;
    phymod_pattern_t       phymod_pattern;
    int                    idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        phymod_pattern_t_init(&phymod_pattern);
        SOC_IF_ERROR_RETURN
            (phymod_phy_pattern_config_get(pm_phy, &phymod_pattern));
        phymod_pattern.pattern_len = value;
        SOC_IF_ERROR_RETURN(phymod_phy_pattern_config_set(pm_phy, &phymod_pattern));
    }
    return(SOC_E_NONE);
}

STATIC int
tsce_pattern_enable_set(soc_phymod_ctrl_t *pmc, uint32_t value)
{
    phymod_phy_access_t    *pm_phy;
    phymod_pattern_t       phymod_pattern;
    int                    idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        phymod_pattern_t_init(&phymod_pattern);
        SOC_IF_ERROR_RETURN
            (phymod_phy_pattern_config_get(pm_phy, &phymod_pattern));
        SOC_IF_ERROR_RETURN(phymod_phy_pattern_enable_set(pm_phy,  value, &phymod_pattern));
    }
    return(SOC_E_NONE);
}

/* 
 * tsce_pattern_data_set 
 *    Sets 32 bits of the 256 bit data pattern.
 */
STATIC int
tsce_pattern_data_set(int idx, tsce_pattern_t *pattern, uint32 value)
{
    if ((idx<0) || (idx >= COUNTOF(pattern->pattern_data))) {
        return SOC_E_INTERNAL;
    }

    /* update pattern data */
    pattern->pattern_data[idx] = value;

    return(SOC_E_NONE);
}

/* 
 * tsce_per_lane_prbs_poly_set
 */
STATIC int
tsce_per_lane_prbs_poly_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
   return(SOC_E_UNAVAIL);
}


STATIC
int
tsce_sdk_poly_to_phymod_poly(uint32 sdk_poly, uint32 *phymod_poly){
    switch(sdk_poly){
    case SOC_PHY_PRBS_POLYNOMIAL_X7_X6_1:
        *phymod_poly = phymodPrbsPoly7;
        break;
    case SOC_PHY_PRBS_POLYNOMIAL_X15_X14_1:
        *phymod_poly = phymodPrbsPoly15;
        break;
    case SOC_PHY_PRBS_POLYNOMIAL_X23_X18_1:
        *phymod_poly = phymodPrbsPoly23;
        break;
    case SOC_PHY_PRBS_POLYNOMIAL_X31_X28_1:
        *phymod_poly = phymodPrbsPoly31;
        break;
    case SOC_PHY_PRBS_POLYNOMIAL_X9_X5_1:
        *phymod_poly = phymodPrbsPoly9;
        break;
    case SOC_PHY_PRBS_POLYNOMIAL_X11_X9_1:
        *phymod_poly = phymodPrbsPoly11;
        break;
    case SOC_PHY_PRBS_POLYNOMIAL_X58_X31_1:
        *phymod_poly = phymodPrbsPoly58;
        break;
    default:
        return SOC_E_INTERNAL;
    }
    return SOC_E_NONE;
}

/* 
 * tsce_prbs_tx_poly_set
 */
STATIC int
tsce_prbs_tx_poly_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_prbs_t        prbs;
    uint32_t flags = 0;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }
    PHYMOD_PRBS_DIRECTION_TX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_get(pm_phy,  flags, &prbs));
    SOC_IF_ERROR_RETURN(tsce_sdk_poly_to_phymod_poly(value, &prbs.poly));
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_set(pm_phy, flags, &prbs));
    return(SOC_E_NONE);
}

/* 
 * tsce_prbs_rx_poly_set
 */
STATIC int
tsce_prbs_rx_poly_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_prbs_t        prbs;
    uint32_t flags = 0;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }
    PHYMOD_PRBS_DIRECTION_RX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_get(pm_phy, flags, &prbs));
    SOC_IF_ERROR_RETURN(tsce_sdk_poly_to_phymod_poly(value, &prbs.poly));
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_set(pm_phy, flags, &prbs));
    return(SOC_E_NONE);
}


/* 
 * tsce_per_lane_prbs_tx_invert_data_set
 */
STATIC int
tsce_per_lane_prbs_tx_invert_data_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_prbs_tx_invert_data_set
 */
STATIC int
tsce_prbs_tx_invert_data_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_prbs_t        prbs;
    uint32_t flags = 0;
    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    PHYMOD_PRBS_DIRECTION_TX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_get(pm_phy, flags, &prbs));
    prbs.invert = value;
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_set(pm_phy, flags,  &prbs));
    return(SOC_E_NONE);
}

/* 
 * tsce_prbs_rx_invert_data_set
 */
STATIC int
tsce_prbs_rx_invert_data_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_prbs_t        prbs;
    uint32_t flags = 0;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    PHYMOD_PRBS_DIRECTION_RX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_get(pm_phy, flags,  &prbs));
    prbs.invert = value;
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_set(pm_phy, flags, &prbs));
    return(SOC_E_NONE);
}

/* 
 * tsce_per_lane_prbs_tx_enable_set
 */
STATIC int
tsce_per_lane_prbs_tx_enable_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
   return(SOC_E_UNAVAIL);
}
/* 
 * tsce_prbs_tx_enable_set
 */
STATIC int
tsce_prbs_tx_enable_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;
    uint32_t flags = 0;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }
    PHYMOD_PRBS_DIRECTION_TX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_enable_set(pm_phy, flags, value));
    return(SOC_E_NONE);
}

/* 
 * tsce_prbs_rx_enable_set
 */
STATIC int
tsce_prbs_rx_enable_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;
    uint32_t flags = 0;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    PHYMOD_PRBS_DIRECTION_RX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_enable_set(pm_phy, flags,  value));
    return(SOC_E_NONE);
}

/* 
 * tsce_loopback_internal_set
 */
STATIC int 
tsce_loopback_internal_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN
        (phymod_phy_loopback_set(pm_phy, phymodLoopbackGlobal, value));

    return(SOC_E_NONE);
}

/* 
 * tsce_loopback_internal_pmd_set
 */
STATIC int 
tsce_loopback_internal_pmd_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN
        (phymod_phy_loopback_set(pm_phy, phymodLoopbackGlobalPMD, value));

    return(SOC_E_NONE);
}

/* 
 * tsce_loopback_remote_set
 */
STATIC int 
tsce_loopback_remote_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN
        (phymod_phy_loopback_set(pm_phy, phymodLoopbackRemotePMD, value));

    return(SOC_E_NONE);
}

/* 
 * tsce_loopback_remote_pcs_set
 */
STATIC int 
tsce_loopback_remote_pcs_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN
        (phymod_phy_loopback_set(pm_phy, phymodLoopbackRemotePCS, value));

    return(SOC_E_NONE);
}

/* 
 * tsce_fec_enable_set
 */
STATIC int 
tsce_fec_enable_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t     *pm_phy;
    int                     idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }
        SOC_IF_ERROR_RETURN(phymod_phy_fec_enable_set(pm_phy, value));
    }
    return(SOC_E_NONE);
}

/* 
 * tsce_scrambler_set
 */
STATIC int
tsce_scrambler_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t     *pm_phy;
    phymod_phy_inf_config_t config;
    int                     idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

    SOC_IF_ERROR_RETURN(phymod_phy_interface_config_get(pm_phy, 0 /* flags */, &config));
    PHYMOD_INTERFACE_MODES_SCR_SET(&config);
    SOC_IF_ERROR_RETURN(phymod_phy_interface_config_set(pm_phy, PHYMOD_IF_FLAGS_DONT_OVERIDE_TX_PARAMS, &config));
    }

#if 0
   /* tscmod had the following side effect; assume this is not needed */
   DEV_CFG_PTR(pc)->scrambler_en = value? TRUE: FALSE;
#endif

    return(SOC_E_NONE);
}

/* 
 * tsce_8b10b_set
 */
STATIC int
tsce_8b10b_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
In discussion: whether encoding API should be separate
Setting interface and speed:
typedef struct phymod_phy_inf_config_s {
    phymod_interface_t interface_type;          
    uint32                      data_rate;          /* Use PHYMOD_DEFAULT_RATE of interface default */
    uint32                      interface_modes;    
} phymod_phy_inf_config_t;

#define PHYMOD_DEFAULT_RATE 0xFFFFFFFF

int phymod_phy_interface_config_set(int unit, uint32 phy_id, uint32 flags, const phymod_phy_inf_config_t* config);
int phymod_phy_interface_config_get(int unit, uint32 phy_id, uint32 flags, phymod_phy_inf_config_t* config);
#endif

   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_64b65b_set
 */
STATIC int
tsce_64b65b_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
In discussion: whether encoding API should be separate
Setting interface and speed:
typedef struct phymod_phy_inf_config_s {
    phymod_interface_t interface_type;          
    uint32                      data_rate;          /* Use PHYMOD_DEFAULT_RATE of interface default */
    uint32                      interface_modes;    
} phymod_phy_inf_config_t;

#define PHYMOD_DEFAULT_RATE 0xFFFFFFFF

int phymod_phy_interface_config_set(int unit, uint32 phy_id, uint32 flags, const phymod_phy_inf_config_t* config);
int phymod_phy_interface_config_get(int unit, uint32 phy_id, uint32 flags, phymod_phy_inf_config_t* config);
#endif

   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_power_set
 */
STATIC int
tsce_power_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t     *pm_phy;
    phymod_phy_power_t      power;
    int                     idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        phymod_phy_power_t_init(&power);
        if (value) {
            power.tx = phymodPowerOn;
            power.rx = phymodPowerOn;
        } 
        else {
            power.tx = phymodPowerOff;
            power.rx = phymodPowerOff;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_power_set(pm_phy, &power));
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_per_lane_rx_low_freq_filter_set
 */
STATIC int
tsce_per_lane_rx_low_freq_filter_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_rx_low_freq_filter_set
 */
STATIC int
tsce_rx_low_freq_filter_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_rx_t          phymod_rx;
    int                  idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_rx_get(pm_phy, &phymod_rx));

        phymod_rx.low_freq_peaking_filter.enable = TRUE;
        phymod_rx.low_freq_peaking_filter.value = value;
        SOC_IF_ERROR_RETURN(phymod_phy_rx_set(pm_phy, &phymod_rx));
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_ppm_adjust_set
 */
STATIC int
tsce_tx_ppm_adjust_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_tx_override_t tx_override;
    int                  idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        tx_override.phase_interpolator.enable = 1;
        tx_override.phase_interpolator.value = value;
        SOC_IF_ERROR_RETURN(phymod_phy_tx_override_set(pm_phy, &tx_override));
    }
    return(SOC_E_NONE);
}

/* 
 * tsce_vco_freq_set
 */
STATIC int
tsce_vco_freq_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
The following 3 controls are used to override pre-defined speed in the phy.
What we discussed in SJ is that in case of rate which is not officially supported the driver will try to 
understand this parameters by itself.
Questions:
1. Do we want to support direct configuration of PLL\OS?
2. If yes - same API or new API

phymod_phy_interface_config_set (?)
#endif

   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_pll_divider_set
 */
STATIC int
tsce_pll_divider_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
phymod_phy_interface_config_set (?)
#endif

   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_oversample_mode_set
 */
STATIC int
tsce_oversample_mode_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
phymod_phy_interface_config_set (?)
#endif

   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_ref_clk_set
 */
STATIC int
tsce_ref_clk_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
The control stores the ref clock in SW DB. 
As discussed the phymod will be stateless. 
So itll be added to interface_config_set as input parameter.

phymod_phy_interface_config_set
#endif

   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_rx_seq_toggle_set 
 *  
 */
STATIC int
tsce_rx_seq_toggle_set(soc_phymod_ctrl_t *pmc)
{
    phymod_phy_access_t    *pm_phy;
    int                    idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        if (pmc->phy[idx] == NULL) {
            return(SOC_E_INTERNAL);
        }

        pm_phy = &pmc->phy[idx]->pm_phy;
        if (pm_phy == NULL) {
            return(SOC_E_INTERNAL);
        }
        SOC_IF_ERROR_RETURN(phymod_phy_rx_restart(pm_phy));
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_driver_supply_set
 */
STATIC int
tsce_driver_supply_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
Can be added as init parameter.
Do you think set\get API are required?
#endif

   return(SOC_E_UNAVAIL);
}


STATIC int
tsce_user_speed_set(soc_phymod_ctrl_t *pmc, 
                    temod_device_aux_modes_t *device, 
                    phymod_pcs_userspeed_mode_t mode, 
                    phymod_pcs_userspeed_param_t param, 
                    uint32 value)
{
    phymod_phy_access_t    *pm_phy;
    soc_phymod_phy_t       *p_phy;
    phymod_pcs_userspeed_config_t config ;
    int                    idx;
    uint32                 lane_map;
    
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, 0, &p_phy, &lane_map));
    config.mode          = mode ; 
    config.param         = param ;
    config.current_entry = (lane_map&0x1)? 0:((lane_map&0x2)? 1:((lane_map&0x4)? 2: ((lane_map&0x8)? 3:0)));
    config.value         = value ;

    if(mode==phymodPcsUserSpeedModeST) {
        if(device !=NULL) {
            config.current_entry = device->st_current_entry ;
        }
    }
    if(device!=NULL) {
        switch(param) {
        case phymodPcsUserSpeedParamEntry:
            if(mode==phymodPcsUserSpeedModeST && value<4 && value>=0) {
                device->st_current_entry = value ;
            }
            return (SOC_E_NONE);
            break ;
        case phymodPcsUserSpeedParamHCD:
            if(mode==phymodPcsUserSpeedModeST) {
                device->st_hcd[device->st_current_entry] = value ;
            }
            /* need to program HW */
            break ;
        case phymodPcsUserSpeedParamClear:
            if(mode==phymodPcsUserSpeedModeST) {
                device->st_hcd[device->st_current_entry] = 0 ;
            } else {
                device->hto_enable[config.current_entry] = 0 ;
            }
            /* need to program HW */
            break ;
        case phymodPcsUserSpeedParamPllDiv:
            if(mode==phymodPcsUserSpeedModeHTO) {
                device->hto_enable[config.current_entry] =1 ;
                device->hto_pll_div[config.current_entry] =value ;
            } else {
                device->st_pll_div[config.current_entry] =value ;
            }
            return (SOC_E_NONE);
            break ;
        case phymodPcsUserSpeedParamPmaOS:
            if(mode==phymodPcsUserSpeedModeHTO) {
                device->hto_enable[config.current_entry] =1 ;
                device->hto_os[config.current_entry] =value ;
            } else {
                device->st_os[config.current_entry] =value ;
            }
            return (SOC_E_NONE);
            break ;
        default:
            break ;   
        }
    }
    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        if (pmc->phy[idx] == NULL) {
            return(SOC_E_INTERNAL);
        }
        
        pm_phy = &pmc->phy[idx]->pm_phy;
        if (pm_phy == NULL) {
            return(SOC_E_INTERNAL);
        }
        
        if(mode==phymodPcsUserSpeedModeST) {
            if(device !=NULL) {
                config.current_entry = device->st_current_entry ;
            }
        }
        SOC_IF_ERROR_RETURN(phymod_phy_pcs_userspeed_set(pm_phy, &config));
    }
    
    return(SOC_E_NONE);
}

STATIC int
tsce_user_speed_get(soc_phymod_ctrl_t *pmc, 
                    temod_device_aux_modes_t *device, 
                    phymod_pcs_userspeed_mode_t mode, 
                    phymod_pcs_userspeed_param_t param, 
                    uint32 *value)
{
    phymod_phy_access_t    *pm_phy;
    soc_phymod_phy_t       *p_phy;
    phymod_pcs_userspeed_config_t config ;
    int                    idx;
    uint32                 lane_map;
    
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, 0, &p_phy, &lane_map));
    config.mode          = mode ; 
    config.param         = param ;
    config.current_entry = (lane_map&0x1)? 0:((lane_map&0x2)? 1:((lane_map&0x4)? 2: ((lane_map&0x8)? 3:0)));
    config.value         = 0 ;
    
    if(mode==phymodPcsUserSpeedModeST) {
        if(device !=NULL) {
            config.current_entry = device->st_current_entry ;
        }
    }

    *value = 0 ;
    /* current device_aux_modes is per logic port even for multi-core port */
    if(device!=NULL) {
        switch(param) {
        case phymodPcsUserSpeedParamEntry:
            if(mode==phymodPcsUserSpeedModeST) {
                * value = device->st_current_entry;
            }
            return (SOC_E_NONE);
            break ;
        case phymodPcsUserSpeedParamHCD:
            if(mode==phymodPcsUserSpeedModeST) {
                *value = device->st_hcd[device->st_current_entry] ;
            }
            /* need to double check with HW */
            break ;
        case phymodPcsUserSpeedParamClear:
            if(mode==phymodPcsUserSpeedModeHTO) {
                *value = device->hto_enable[config.current_entry] ;
            }
            /* do we need to double check with HW ? */
            return (SOC_E_NONE);
            break ;
        case phymodPcsUserSpeedParamPllDiv:
            if(mode==phymodPcsUserSpeedModeHTO) {
                if(device->hto_enable[config.current_entry]) {
                    *value = device->hto_pll_div[config.current_entry] ;
                }
            } else {
                *value = device->st_pll_div[config.current_entry] ;
            }
            /* do we need to double check with HW ? */
            return (SOC_E_NONE);
            break ;
        case phymodPcsUserSpeedParamPmaOS:
            if(mode==phymodPcsUserSpeedModeHTO) {
                if(device->hto_enable[config.current_entry] ==1){
                    *value = device->hto_os[config.current_entry] ;
                }
            } else {
                *value = device->st_os[config.current_entry] ;
            }
            /* do we need to double check with HW ? */
            return (SOC_E_NONE);
            break ;
        default:
            break ;   
        }
    }
    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        if (pmc->phy[idx] == NULL) {
            return(SOC_E_INTERNAL);
        }
        
        pm_phy = &pmc->phy[idx]->pm_phy;
        if (pm_phy == NULL) {
            return(SOC_E_INTERNAL);
        }
        
        SOC_IF_ERROR_RETURN(phymod_phy_pcs_userspeed_get(pm_phy, &config));
        *value = config.value ;
    }
    
    return(SOC_E_NONE);
}

/*
 * Function:
 *      phy_tsce_control_set
 * Purpose:
 *      Configure PHY device specific control fucntion. 
 * Parameters:
 *      unit  - BCM unit number.
 *      port  - Port number. 
 *      type  - Control to update 
 *      value - New setting for the control 
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_control_set(int unit, soc_port_t port, soc_phy_control_t type, uint32 value)
{
    int                 rv;
    phy_ctrl_t          *pc;
    soc_phymod_ctrl_t   *pmc;
    tsce_config_t       *pCfg;

    rv = SOC_E_UNAVAIL;

    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return (SOC_E_PARAM);
    }

    /* locate phy control, phymod control, and the configuration data */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }
    pmc = &pc->phymod_ctrl;
    pCfg = (tsce_config_t *) pc->driver_data;

    switch(type) {

    case SOC_PHY_CONTROL_TX_FIR_PRE:
        rv = tsce_tx_fir_pre_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_MAIN:
        rv = tsce_tx_fir_main_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST:
        rv = tsce_tx_fir_post_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST2:
        rv = tsce_tx_fir_post2_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST3:
        rv = tsce_tx_fir_post3_set(pmc, value);
        break;
    /* PREEMPHASIS */
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE0:
        rv = tsce_per_lane_preemphasis_set(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE1:
        rv = tsce_per_lane_preemphasis_set(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE2:
        rv = tsce_per_lane_preemphasis_set(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE3:
        rv = tsce_per_lane_preemphasis_set(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS:
        rv = tsce_preemphasis_set(pmc, value);
        break;

    /* DRIVER CURRENT */
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE0:
        rv = tsce_per_lane_driver_current_set(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE1:
        rv = tsce_per_lane_driver_current_set(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE2:
        rv = tsce_per_lane_driver_current_set(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE3:
        rv = tsce_per_lane_driver_current_set(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT:
        rv = tsce_driver_current_set(pmc, value);
        break;

    /* PRE_DRIVER CURRENT  not supported anymore */
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE0:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE1:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE2:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE3:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;

    /* POST2_DRIVER CURRENT not supported anymore */
    case SOC_PHY_CONTROL_DRIVER_POST2_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_DUMP:
        /* "N/A: will be part of common phymod application" */
        /*     was rv = tscmod_uc_status_dump(unit, port); */
        break;

    /* TX LANE SQUELCH */
    case SOC_PHY_CONTROL_TX_LANE_SQUELCH:
        rv = tsce_tx_lane_squelch(pmc, value);
        break;

    /* RX PEAK FILTER */
    case SOC_PHY_CONTROL_RX_PEAK_FILTER:
        rv = tsce_rx_peak_filter_set(pmc, value);
        break;

    /* RX VGA */
    case SOC_PHY_CONTROL_RX_VGA:
        rv = tsce_rx_vga_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_RX_VGA_RELEASE:
       /* $$$ tbd $$$ */
       break;

    /* RX TAP */
    case SOC_PHY_CONTROL_RX_TAP1:
        rv = tsce_rx_tap_set(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP2:
        rv = tsce_rx_tap_set(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP3:
        rv = tsce_rx_tap_set(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP4:
        rv = tsce_rx_tap_set(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP5:
        rv = tsce_rx_tap_set(pmc, 4, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP1_RELEASE:       /* $$$ tbd $$$ */
       rv = tsce_rx_tap_release(pmc, 0);
       break;
    case SOC_PHY_CONTROL_RX_TAP2_RELEASE:       /* $$$ tbd $$$ */
       rv = tsce_rx_tap_release(pmc, 1);
       break;
    case SOC_PHY_CONTROL_RX_TAP3_RELEASE:       /* $$$ tbd $$$ */
       rv = tsce_rx_tap_release(pmc, 2);
       break;
    case SOC_PHY_CONTROL_RX_TAP4_RELEASE:       /* $$$ tbd $$$ */
       rv = tsce_rx_tap_release(pmc, 3);
       break;
    case SOC_PHY_CONTROL_RX_TAP5_RELEASE:       /* $$$ tbd $$$ */
       rv = tsce_rx_tap_release(pmc, 4);
       break;
    /* RX SLICER */
    case SOC_PHY_CONTROL_RX_PLUS1_SLICER:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_RX_MINUS1_SLICER:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_RX_D_SLICER:
        rv = SOC_E_UNAVAIL;
        break;

    /* PHASE INTERPOLATOR */
    case SOC_PHY_CONTROL_PHASE_INTERP:
        rv = tsce_pi_control_set(pmc, value);
        break;

    /* POLARITY */
    case SOC_PHY_CONTROL_RX_POLARITY:
        rv = tsce_rx_polarity_set(pmc, &pCfg->phy_polarity_config, value);;
        break;
    case SOC_PHY_CONTROL_TX_POLARITY:
        rv = tsce_tx_polarity_set(pmc, &pCfg->phy_polarity_config, value);
        break;

    /* RESET */
    case SOC_PHY_CONTROL_RX_RESET:
        rv = tsce_rx_reset(pmc, &pCfg->phy_reset_config, value);
        break;
    case SOC_PHY_CONTROL_TX_RESET:
        rv = tsce_tx_reset(pmc, &pCfg->phy_reset_config, value);
        break;

    /* CL72 ENABLE */
    case SOC_PHY_CONTROL_CL72:
        rv = tsce_cl72_enable_set(pmc, value);
        break;

    /* LANE SWAP */
    case SOC_PHY_CONTROL_LANE_SWAP:
        rv = tsce_lane_map_set(pmc, value);
        break;

    /* FIRMWARE MODE */
    case SOC_PHY_CONTROL_FIRMWARE_MODE:
        rv = tsce_firmware_mode_set(pmc, value);
        break; 

    /* TX PATTERN */
    case SOC_PHY_CONTROL_TX_PATTERN_20BIT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_256BIT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_LENGTH:
        rv = tsce_pattern_len_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_GEN_ENABLE:
        rv = tsce_pattern_enable_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA0:
        rv = tsce_pattern_data_set(0, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA1:
        rv = tsce_pattern_data_set(1, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA2:
        rv = tsce_pattern_data_set(2, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA3:
        rv = tsce_pattern_data_set(3, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA4:
        rv = tsce_pattern_data_set(4, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA5:
        rv = tsce_pattern_data_set(5, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA6:
        rv = tsce_pattern_data_set(6, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA7:
        rv = tsce_pattern_data_set(7, &pCfg->pattern, value);
        break;

    /* decoupled PRBS */
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_POLYNOMIAL:
        rv = tsce_prbs_tx_poly_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_INVERT_DATA:
        rv = tsce_prbs_tx_invert_data_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_ENABLE:
        rv = tsce_prbs_tx_enable_set(pmc, value);
        break; 
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_POLYNOMIAL:
        rv = tsce_prbs_rx_poly_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_INVERT_DATA:
        rv = tsce_prbs_rx_invert_data_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_ENABLE:
        rv = tsce_prbs_rx_enable_set(pmc, value);
        break;
    /* for legacy prbs usage mainly set both tx/rx the same */
    case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
        rv = tsce_prbs_tx_poly_set(pmc, value);
        rv = tsce_prbs_rx_poly_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
        rv = tsce_prbs_tx_invert_data_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
        rv = tsce_prbs_tx_enable_set(pmc, value);
        rv = tsce_prbs_rx_enable_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
        rv = tsce_prbs_tx_enable_set(pmc, value);
        rv = tsce_prbs_rx_enable_set(pmc, value);
        break;

    /* LOOPBACK */
    case SOC_PHY_CONTROL_LOOPBACK_INTERNAL:
        rv = tsce_loopback_internal_set(pmc, value);
       break;
    case SOC_PHY_CONTROL_LOOPBACK_PMD:
        rv = tsce_loopback_internal_pmd_set(pmc, value);
       break;
    case SOC_PHY_CONTROL_LOOPBACK_REMOTE:
        rv = tsce_loopback_remote_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_LOOPBACK_REMOTE_PCS_BYPASS:
        rv = tsce_loopback_remote_pcs_set(pmc, value);
        break;

    /* FEC */
    case SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION:
        rv = tsce_fec_enable_set(pmc, value);
        break;

    /* CUSTOM */
    case SOC_PHY_CONTROL_CUSTOM1:
       /* Obsolete ??
       DEV_CFG_PTR(pc)->custom1 = value? TRUE: FALSE ;
       rv = SOC_E_NONE; */
       rv = SOC_E_UNAVAIL;
       break;

    /* SCRAMBLER */
    case SOC_PHY_CONTROL_SCRAMBLER:
        rv = tsce_scrambler_set(pmc, value);
        break;

    /* 8B10B */
    case SOC_PHY_CONTROL_8B10B:
        rv = tsce_8b10b_set(pmc, value);
        break;

    /* 64B65B */
    case SOC_PHY_CONTROL_64B66B:
        rv = tsce_64b65b_set(pmc, value);
        break;

    /* POWER */
    case SOC_PHY_CONTROL_POWER:
       rv = tsce_power_set(pmc, value);
       break;

    /* RX_LOW_FREQ_PEAK_FILTER */
    case SOC_PHY_CONTROL_RX_LOW_FREQ_PEAK_FILTER:
       rv = tsce_rx_low_freq_filter_set(pmc, value);
       break;

    /* TX_PPM_ADJUST */
    case SOC_PHY_CONTROL_TX_PPM_ADJUST:
       rv = tsce_tx_ppm_adjust_set(pmc, value);
       break;

    /* VCO_FREQ */
    case SOC_PHY_CONTROL_VCO_FREQ:
       rv = tsce_vco_freq_set(pmc, value);
       break;

    /* PLL_DIVIDER */
    case SOC_PHY_CONTROL_PLL_DIVIDER:
       rv = tsce_pll_divider_set(pmc, value);
       break;

    /* OVERSAMPLE_MODE */
    case SOC_PHY_CONTROL_OVERSAMPLE_MODE:
       rv = tsce_oversample_mode_set(pmc, value);
       break;

    /* REF_CLK */
    case SOC_PHY_CONTROL_REF_CLK:
       rv = tsce_ref_clk_set(pmc, value);
       break;

    /* RX_SEQ_TOGGLE */
    case SOC_PHY_CONTROL_RX_SEQ_TOGGLE:
       rv = tsce_rx_seq_toggle_set(pmc);
       break;

    /* DRIVER_SUPPLY */
    case SOC_PHY_CONTROL_DRIVER_SUPPLY:
       rv = tsce_driver_supply_set(pmc, value);
       break;

    /* EEE */
#ifdef TSC_EEE_SUPPORT
    case SOC_PHY_CONTROL_EEE:
    case SOC_PHY_CONTROL_EEE_AUTO:
        rv = SOC_E_NONE;   /* not supported */
        break;
#endif
#if 0
       /* Added to support WA for HW bug; probably not required. */
    case SOC_PHY_CONTROL_SOFTWARE_RX_LOS:
        DEV_CFG_PTR(pc)->sw_rx_los.enable = (value == 0? 0: 1);
        DEV_CFG_PTR(pc)->sw_rx_los.sys_link = 0; 
        DEV_CFG_PTR(pc)->sw_rx_los.state = RXLOS_RESET;
        DEV_CFG_PTR(pc)->sw_rx_los.link_status = 0;
        /* THE FLAG IS SET IN INIT. FOR TSCMOD DRIVER
           TO WORK IT'S LINK_GET MUST BE CALLED */
        /* Manage the _SERVICE_INT_PHY_LINK_GET flag so that 
           if external phy is present link_get for 
           internal phy is still called to process 
           software rx los feature 
        if(value) {
            PHY_FLAGS_SET(unit, port, PHY_FLAGS_SERVICE_INT_PHY_LINK_GET);
        } else {
            PHY_FLAGS_CLR(unit, port, PHY_FLAGS_SERVICE_INT_PHY_LINK_GET);
        }
        */
        rv = SOC_E_NONE;
        break;
    /* UNAVAIL */
    case SOC_PHY_CONTROL_LINKDOWN_TRANSMIT:
    case SOC_PHY_CONTROL_PARALLEL_DETECTION:
    case SOC_PHY_CONTROL_BERT_PATTERN:
    case SOC_PHY_CONTROL_BERT_RUN:
    case SOC_PHY_CONTROL_BERT_PACKET_SIZE:
    case SOC_PHY_CONTROL_BERT_IPG:
    case SOC_PHY_CONTROL_RX_PEAK_FILTER_TEMP_COMP:
    case SOC_PHY_CONTROL_CL73_FSM_AUTO_RECOVER:
#endif
    case SOC_PHY_CONTROL_PCS_SPEED_ST_ENTRY:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamEntry, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_HCD:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamHCD, value) ;
        rv = SOC_E_NONE ;
        break ;
        /*
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CLEAR:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamClear, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PLL_DIVIDER:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamPllDiv, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PLL_DIVIDER:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamPllDiv, value) ;
        rv = SOC_E_NONE ;
        break ;
        */
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PMA_OS:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamPmaOS, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PMA_OS:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamPmaOS, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_SCR_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamScramble, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_SCR_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamScramble, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_ENCODE_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamEncode, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_ENCODE_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamEncode, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CL48_CHECK_END:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamCl48CheckEnd, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_CL48_CHECK_END:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamCl48CheckEnd, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_BLK_SYNC_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamBlkSync, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_BLK_SYNC_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamBlkSync, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_REORDER_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamReorder, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_REORDER_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamReorder, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CL36_EN:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamCl36Enable, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_CL36_EN:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamCl36Enable, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_DESCR1_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamDescr1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_DESCR1_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamDescr1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_DEC1_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamDecode1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_DEC1_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamDecode1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_DESKEW_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamDeskew, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_DESKEW_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamDeskew, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_DESC2_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamDescr2, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_DESC2_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamDescr2, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_DESC2_BYTE_DEL:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamDescr2ByteDel, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_DESC2_BYTE_DEL:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamDescr2ByteDel, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_BRCM64B66_DESCR:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamBrcm64B66, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_BRCM64B66_DESCR:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamBrcm64B66, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_SGMII_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamSgmii, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_SGMII_MODE:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamSgmii, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CLKCNT0:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamClkcnt0, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_CLKCNT0:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamClkcnt0, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CLKCNT1:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamClkcnt1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_CLKCNT1:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamClkcnt1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_LPCNT0:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamLpcnt0, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_LPCNT0:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamLpcnt0, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_LPCNT1:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamLpcnt1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_LPCNT1:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamLpcnt1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_MAC_CGC:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamMacCGC, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_MAC_CGC:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamMacCGC, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PCS_REPCNT:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamRepcnt, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PCS_REPCNT:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamRepcnt, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PCS_CREDTEN:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamCrdtEn, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PCS_CREDTEN:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamCrdtEn, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PCS_CLKCNT:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamPcsClkcnt, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PCS_CLKCNT:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamPcsClkcnt, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PCS_CGC:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamPcsCGC, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PCS_CGC:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamPcsCGC, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CL72_EN:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamCl72En, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_CL72_EN:
        tsce_user_speed_set(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamCl72En, value) ;
        rv = SOC_E_NONE ;
        break ;
    default:
        rv = SOC_E_UNAVAIL;
        break; 
    }
    return rv;
}


/* 
 * tsce_tx_fir_pre_get
 */
STATIC int
tsce_tx_fir_pre_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        *value = phymod_tx.pre;
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_fir_main_get
 */
STATIC int
tsce_tx_fir_main_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        *value = phymod_tx.main;
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_fir_post_get
 */
STATIC int
tsce_tx_fir_post_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        *value = phymod_tx.post;
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_fir_post2_get
 */
STATIC int
tsce_tx_fir_post2_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        *value = phymod_tx.post2;
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_tx_fir_post3_get
 */
STATIC int
tsce_tx_fir_post3_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t *pm_phy;
    phymod_tx_t         phymod_tx;
    int                 idx;

    /* loop through all cores */
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;

        if (pm_phy == NULL) {
            return SOC_E_INTERNAL;
        }

        SOC_IF_ERROR_RETURN(phymod_phy_tx_get(pm_phy, &phymod_tx));
        *value = phymod_tx.post3;
    }

    return(SOC_E_NONE);
}

/* 
 * tsce_per_lane_preemphasis_get
 */
STATIC int
tsce_per_lane_preemphasis_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_tx_t         phymod_tx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    SOC_IF_ERROR_RETURN(phymod_phy_tx_get(&pm_phy_copy, &phymod_tx));
    *value = phymod_tx.pre;

    return(SOC_E_NONE);
}

/* 
 * tsce_per_lane_driver_current_get
 */
STATIC int
tsce_per_lane_driver_current_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_tx_t         phymod_tx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    SOC_IF_ERROR_RETURN(phymod_phy_tx_get(&pm_phy_copy, &phymod_tx));
    *value = phymod_tx.amp;

    return(SOC_E_NONE);
}

/* 
 * tsce_per_lane_prbs_poly_get
 */
STATIC int
tsce_per_lane_prbs_poly_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_per_lane_prbs_tx_invert_data_get
 */
STATIC int
tsce_per_lane_prbs_tx_invert_data_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_per_lane_prbs_tx_enable_get
 */
STATIC int
tsce_per_lane_prbs_tx_enable_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_per_lane_prbs_rx_enable_get
 */
STATIC int
tsce_per_lane_prbs_rx_enable_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_per_lane_prbs_rx_status_get
 */
STATIC int
tsce_per_lane_prbs_rx_status_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   /* $$$ Need to add diagnostic API */
   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_per_lane_rx_peak_filter_get
 */
STATIC int
tsce_per_lane_rx_peak_filter_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t         phymod_rx;

    *value = 0;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    SOC_IF_ERROR_RETURN(phymod_phy_rx_get(&pm_phy_copy, &phymod_rx));
    if (phymod_rx.peaking_filter.enable)
        *value = phymod_rx.peaking_filter.value;

    return(SOC_E_NONE);
}

/* 
 * tsce_per_lane_rx_vga_get
 */
STATIC int
tsce_per_lane_rx_vga_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t         phymod_rx;

    *value = 0;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    SOC_IF_ERROR_RETURN(phymod_phy_rx_get(&pm_phy_copy, &phymod_rx));
    if (phymod_rx.vga.enable)
        *value = phymod_rx.vga.value;

    return(SOC_E_NONE);
}

/* 
 * tsce_per_lane_rx_dfe_tap_control_get
 */
STATIC int
tsce_per_lane_rx_dfe_tap_control_get(soc_phymod_ctrl_t *pmc, int lane, int tap, uint32 *value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t          phymod_rx;

    *value = 0;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_tsce_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    if ((tap<0)||(tap>COUNTOF(phymod_rx.dfe))) {
        /* this can only happen with a coding error */
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_rx_get(&pm_phy_copy, &phymod_rx));
    if (phymod_rx.dfe[tap].enable)
       *value = phymod_rx.dfe[tap].value;

    return(SOC_E_NONE);
}


/* 
 * tsce_per_lane_rx_low_freq_filter_get
 */
STATIC int
tsce_per_lane_rx_low_freq_filter_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_rx_peak_filter_get
 */
STATIC int 
tsce_rx_peak_filter_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_rx_t          phymod_rx;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_rx_get(pm_phy, &phymod_rx));
    *value = phymod_rx.peaking_filter.value;

    return(SOC_E_NONE);
}

/* 
 * tsce_rx_vga_set
 */
STATIC int 
tsce_rx_vga_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_rx_t          phymod_rx;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_rx_get(pm_phy, &phymod_rx));
    *value = phymod_rx.vga.value;

    return(SOC_E_NONE);
}

/* 
 * tsce_rx_tap_get
 */
STATIC int 
tsce_rx_tap_get(soc_phymod_ctrl_t *pmc, int tap, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_rx_t          phymod_rx;

    if ((tap<0)||(tap>COUNTOF(phymod_rx.dfe))) {
        /* this can only happen with a coding error */
        return SOC_E_INTERNAL;
    }
    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_rx_get(pm_phy, &phymod_rx));
    *value = phymod_rx.dfe[tap].value;

    return(SOC_E_NONE);
}

/* 
 * tsce_pi_control_get
 */
STATIC int 
tsce_pi_control_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_tx_override_t tx_override;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_tx_override_get(pm_phy, &tx_override));
    *value = tx_override.phase_interpolator.value;

    return(SOC_E_NONE);
}

/* 
 * tsce_rx_signal_detect_get
 */
STATIC int
tsce_rx_signal_detect_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
   /* $$$ Need to add diagnostic API */
   return(SOC_E_UNAVAIL);
}

/* 
 * tsce_rx_seq_done_get
 */
STATIC int
tsce_rx_seq_done_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
         return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_rx_pmd_locked_get(pm_phy, value));

    return(SOC_E_NONE);
}

/* 
 * tsce_rx_ppm_get
 */
STATIC int
tsce_rx_ppm_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    /* $$$ Need to add diagnostic API */
    return(SOC_E_UNAVAIL);
}

/* 
 * tsce_cl72_enable_get
 */
STATIC int
tsce_cl72_enable_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_cl72_status_t status;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
         return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_cl72_status_get(pm_phy, &status));
    *value = status.enabled;

    return(SOC_E_NONE);
}

/* 
 * tsce_cl72_status_get
 */
STATIC int
tsce_cl72_status_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_cl72_status_t status;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_cl72_status_get(pm_phy, &status));
    *value = status.locked;

    return(SOC_E_NONE);
}

/* 
 * tsce_prbs_tx_poly_get
 */
STATIC int
tsce_prbs_tx_poly_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_prbs_t        prbs;
    uint32_t flags = 0;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    PHYMOD_PRBS_DIRECTION_TX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_get(pm_phy, flags, &prbs));

    *value = (int) prbs.poly;

    /* convert from PHYMOD enum to SDK enum */
    switch(prbs.poly){
    case phymodPrbsPoly7:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X7_X6_1;
        break;
    case phymodPrbsPoly9:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X9_X5_1;
        break;
    case phymodPrbsPoly15:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X15_X14_1;
        break;
    case phymodPrbsPoly23:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X23_X18_1;
        break;
    case phymodPrbsPoly31:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X31_X28_1;
        break;
    case phymodPrbsPoly11:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X11_X9_1;
        break;
    case phymodPrbsPoly58:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X58_X31_1; 
        break;
    default:
        return SOC_E_INTERNAL;
    }
    return SOC_E_NONE;
}
/* 
 * tsce_prbs_rx_poly_get
 */
STATIC int
tsce_prbs_rx_poly_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_prbs_t        prbs;
    uint32_t flags = 0;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    PHYMOD_PRBS_DIRECTION_RX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_get(pm_phy, flags, &prbs));

    *value = (int) prbs.poly;

    /* convert from PHYMOD enum to SDK enum */
    switch(prbs.poly){
    case phymodPrbsPoly7:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X7_X6_1;
        break;
    case phymodPrbsPoly9:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X9_X5_1;
        break;
    case phymodPrbsPoly15:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X15_X14_1;
        break;
    case phymodPrbsPoly23:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X23_X18_1;
        break;
    case phymodPrbsPoly31:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X31_X28_1;
        break;
    case phymodPrbsPoly11:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X11_X9_1;
        break;
    case phymodPrbsPoly58:
        *value = SOC_PHY_PRBS_POLYNOMIAL_X58_X31_1;
        break;
    default:
        return SOC_E_INTERNAL;
    }
    return SOC_E_NONE;
}

/* 
 * tsce_prbs_tx_invert_data_get
 */
STATIC int
tsce_prbs_tx_invert_data_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_prbs_t        prbs;
    uint32_t flags = 0;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    PHYMOD_PRBS_DIRECTION_TX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_get(pm_phy, flags, &prbs));
    *value = prbs.invert;

    return(SOC_E_NONE);
}

/* 
 * tsce_prbs_rx_invert_data_get
 */
STATIC int
tsce_prbs_rx_invert_data_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    phymod_prbs_t        prbs;
    uint32_t flags = 0;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    PHYMOD_PRBS_DIRECTION_RX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_get(pm_phy, flags, &prbs));
    *value = prbs.invert;

    return(SOC_E_NONE);
}


/* 
 * tsce_prbs_tx_enable_get
 */
STATIC int
tsce_prbs_tx_enable_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    uint32_t flags = 0;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }
    PHYMOD_PRBS_DIRECTION_TX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_enable_get(pm_phy, flags, value));

    return(SOC_E_NONE);
}

/* 
 * tsce_prbs_rx_enable_get
 */
STATIC int
tsce_prbs_rx_enable_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t  *pm_phy;
    uint32_t flags = 0;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    PHYMOD_PRBS_DIRECTION_RX_SET(flags);
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_enable_get(pm_phy, flags, value));

    return(SOC_E_NONE);
}

/* 
 * tsce_prbs_rx_status_get
 */
STATIC int
tsce_prbs_rx_status_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t    *pm_phy;
    phymod_prbs_status_t   prbs_tmp;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }
    /* $$$ Need to add diagnostic API */
    SOC_IF_ERROR_RETURN
        (phymod_phy_prbs_status_get(pm_phy, 0, &prbs_tmp));
    if (prbs_tmp.prbs_lock == 0) {
        *value = -1;
    } else if ((prbs_tmp.prbs_lock_loss == 1) && (prbs_tmp.prbs_lock == 1)) {
        *value = -2;
    } else {
        *value = prbs_tmp.error_count;
    }     

    return(SOC_E_NONE);
}

/* 
 * tsce_loopback_internal_pmd_get (this is the PMD global loopback)
 */
STATIC int 
tsce_loopback_internal_pmd_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t    *pm_phy;
    uint32_t               enable;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_loopback_get(pm_phy, phymodLoopbackGlobalPMD, &enable));
    *value = enable;
    return(SOC_E_NONE);
}


/* 
 * tsce_loopback_remote_get
 */
STATIC int 
tsce_loopback_remote_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t    *pm_phy;
    uint32_t               enable;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_loopback_get(pm_phy, phymodLoopbackRemotePMD, &enable));
    *value = enable;
    return(SOC_E_NONE);
}

/* 
 * tsce_loopback_remote_pcs_get
 */
STATIC int 
tsce_loopback_remote_pcs_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t    *pm_phy;
    uint32_t               enable;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_loopback_get(pm_phy, phymodLoopbackRemotePCS, &enable));
    *value = enable;
    return(SOC_E_NONE);
}

/* 
 * tsce_fec_get
 */
STATIC int 
tsce_fec_enable_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t    *pm_phy;
    uint32_t               enable;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_fec_enable_get(pm_phy,  &enable));
    *value = enable;

    return(SOC_E_NONE);
}
/* 
 * tsce_pattern_256bit_get 
 *  
 * CHECK SEMANTICS: assumption is that this procedure will retrieve the pattern 
 * from Tier1 and return whether the tx pattern is enabled.  Note that this 
 * is different from the 20 bit pattern retrieval semantics 
 */
/* 
 * tsce_pattern_256bit_get 
 *  
 * CHECK SEMANTICS: assumption is that this procedure will retrieve the pattern 
 * from Tier1 and return whether the tx pattern is enabled.  Note that this 
 * is different from the 20 bit pattern retrieval semantics 
 */
STATIC int
tsce_pattern_get(soc_phymod_ctrl_t *pmc, tsce_pattern_t* cfg_pattern)
{
    phymod_phy_access_t    *pm_phy;
    phymod_pattern_t       phymod_pattern;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_pattern_config_get(pm_phy, &phymod_pattern));
    sal_memcpy(cfg_pattern , phymod_pattern.pattern, sizeof(cfg_pattern));
    return(SOC_E_NONE);
}

STATIC int
tsce_pattern_len_get(soc_phymod_ctrl_t *pmc, uint32_t *value)
{
    phymod_phy_access_t    *pm_phy;
    phymod_pattern_t       phymod_pattern;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_pattern_config_get(pm_phy, &phymod_pattern));
    *value = phymod_pattern.pattern_len;
    return(SOC_E_NONE);
}

/* 
 * tsce_scrambler_get
 */
STATIC int
tsce_scrambler_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t     *pm_phy;
    phymod_phy_inf_config_t config;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_interface_config_get(pm_phy, 0 /* flags */, &config));
    *value = PHYMOD_INTERFACE_MODES_IS_SCR(&config);

    return(SOC_E_NONE);
}

/* 
 * tsce_firmware_mode_get
 */
STATIC int
tsce_firmware_mode_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    phymod_phy_access_t    *pm_phy;
    phymod_firmware_lane_config_t fw_config;

    /* just take the value from the first phy */
    if (pmc->phy[0] == NULL) {
        return SOC_E_INTERNAL;
    }

    pm_phy = &pmc->phy[0]->pm_phy;
    if (pm_phy == NULL) {
        return SOC_E_INTERNAL;
    }

    SOC_IF_ERROR_RETURN(phymod_phy_firmware_lane_config_get(pm_phy, &fw_config));

    if (fw_config.LaneConfigFromPCS) {
        *value = SOC_PHY_FIRMWARE_DEFAULT;
    } else if (fw_config.DfeOn) {
        *value = SOC_PHY_FIRMWARE_FORCE_OSDFE;
    } else if (fw_config.ForceBrDfe) {
        *value = SOC_PHY_FIRMWARE_FORCE_BRDFE;
    } else if (SOC_PHY_FIRMWARE_CL72_WITHOUT_AN) {
        *value = SOC_PHY_FIRMWARE_CL72_WITHOUT_AN;
    } else {
        /*leave it blank for now */
    }                      

    return(SOC_E_NONE);
}

/*
 * Function:
 *      phy_tsce_control_get
 * Purpose:
 *      Get current control settings of the PHY. 
 * Parameters:
 *      unit  - BCM unit number.
 *      port  - Port number. 
 *      type  - Control to update 
 *      value - (OUT) Current setting for the control 
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_control_get(int unit, soc_port_t port, soc_phy_control_t type, uint32 *value)
{
    int                 rv;
    phy_ctrl_t          *pc;
    soc_phymod_ctrl_t   *pmc;
    tsce_config_t       *pCfg;

    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return (SOC_E_PARAM);
    }

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    pmc = &pc->phymod_ctrl;
    pCfg = (tsce_config_t *) pc->driver_data;

    switch(type) {

    /* PREEMPHASIS */
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE0:
        rv = tsce_per_lane_preemphasis_get(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE1:
        rv = tsce_per_lane_preemphasis_get(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE2:
        rv = tsce_per_lane_preemphasis_get(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE3:
        rv = tsce_per_lane_preemphasis_get(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_PRE:
        /* assume they are all the same as lane 0 */
        rv = tsce_tx_fir_pre_get(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_MAIN:
        /* assume they are all the same as lane 0 */
        rv = tsce_tx_fir_main_get(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST:
        /* assume they are all the same as lane 0 */
        rv = tsce_tx_fir_post_get(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST2:
        /* assume they are all the same as lane 0 */
        rv = tsce_tx_fir_post2_get(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST3:
        /* assume they are all the same as lane 0 */
        rv = tsce_tx_fir_post3_get(pmc, value);
        break;

    /* DRIVER CURRENT */
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE0:
        rv = tsce_per_lane_driver_current_get(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE1:
        rv = tsce_per_lane_driver_current_get(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE2:
        rv = tsce_per_lane_driver_current_get(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE3:
        rv = tsce_per_lane_driver_current_get(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT:
        rv = tsce_per_lane_driver_current_get(pmc, 0, value);
        break;

    /* PRE-DRIVER CURRENT */
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE0:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE1:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE2:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT_LANE3:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;

    /* POST2_DRIVER CURRENT */
    case SOC_PHY_CONTROL_DRIVER_POST2_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;

    /* RX PEAK FILTER */
    case SOC_PHY_CONTROL_RX_PEAK_FILTER:
        rv = tsce_rx_peak_filter_get(pmc, value);
        break;

    /* RX VGA */
    case SOC_PHY_CONTROL_RX_VGA:
        rv = tsce_rx_vga_get(pmc, value);
        break;

    /* RX TAP */
    case SOC_PHY_CONTROL_RX_TAP1:
        rv = tsce_rx_tap_get(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP2:
        rv = tsce_rx_tap_get(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP3:
        rv = tsce_rx_tap_get(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP4:
        rv = tsce_rx_tap_get(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP5:
        rv = tsce_rx_tap_get(pmc, 4, value);
        break;

    /* RX SLICER */
    case SOC_PHY_CONTROL_RX_PLUS1_SLICER:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_RX_MINUS1_SLICER:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_RX_D_SLICER:
        rv = SOC_E_UNAVAIL;
        break;

    /* PHASE INTERPOLATOR */
    case SOC_PHY_CONTROL_PHASE_INTERP:
        rv = tsce_pi_control_get(pmc, value);
        break;

    /* RX SIGNAL DETECT */
    case SOC_PHY_CONTROL_RX_SIGNAL_DETECT:
      rv = tsce_rx_signal_detect_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_RX_SEQ_DONE:
      rv = tsce_rx_seq_done_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_RX_PPM:
      rv = tsce_rx_ppm_get(pmc, value);
      break;

    /* CL72 */
    case SOC_PHY_CONTROL_CL72:
      rv = tsce_cl72_enable_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_CL72_STATUS:
      rv = tsce_cl72_status_get(pmc, value);
      break;

    /* FIRMWARE MODE */
    case SOC_PHY_CONTROL_FIRMWARE_MODE:
       rv = tsce_firmware_mode_get(pmc, value);
       break;

    /* PRBS */
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_POLYNOMIAL:
      rv = tsce_prbs_tx_poly_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_INVERT_DATA:
      rv = tsce_prbs_tx_invert_data_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_ENABLE:
      rv = tsce_prbs_tx_enable_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_POLYNOMIAL:
      rv = tsce_prbs_rx_poly_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_INVERT_DATA:
      rv = tsce_prbs_rx_invert_data_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_ENABLE:
      rv = tsce_prbs_rx_enable_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
      rv = tsce_prbs_tx_poly_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
      rv = tsce_prbs_tx_invert_data_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
      rv = tsce_prbs_tx_enable_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
      rv = tsce_prbs_rx_enable_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_RX_STATUS:
      rv = tsce_prbs_rx_status_get(pmc, value);
      break;

    /* LOOPBACK */
    case SOC_PHY_CONTROL_LOOPBACK_REMOTE:
       rv = tsce_loopback_remote_get(pmc, value);
       break;
    case SOC_PHY_CONTROL_LOOPBACK_REMOTE_PCS_BYPASS:
       rv = tsce_loopback_remote_pcs_get(pmc, value);
       break;
    case SOC_PHY_CONTROL_LOOPBACK_PMD:
       rv = tsce_loopback_internal_pmd_get(pmc, value);
       break;

    /* FEC */
    case SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION:
      rv = tsce_fec_enable_get(pmc, value);
      break;

    /* TX PATTERN */
    case SOC_PHY_CONTROL_TX_PATTERN_256BIT:
       rv = tsce_pattern_get(pmc, &pCfg->pattern);
      break;
    case SOC_PHY_CONTROL_TX_PATTERN_LENGTH:
       rv = tsce_pattern_len_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_TX_PATTERN_20BIT:
       rv = tsce_pattern_get(pmc, &pCfg->pattern);
      break;

    /* SCRAMBLER */
    case SOC_PHY_CONTROL_SCRAMBLER:
       rv = tsce_scrambler_get(pmc, value);
       break;
    case SOC_PHY_CONTROL_LANE_SWAP:
        rv = tsce_lane_map_get(pmc, value);
        break;
    /* UNAVAIL */

#if 0
    /* BERT */
    case SOC_PHY_CONTROL_BERT_TX_PACKETS:       /* fall through */
    case SOC_PHY_CONTROL_BERT_RX_PACKETS:       /* fall through */
    case SOC_PHY_CONTROL_BERT_RX_ERROR_BITS:    /* fall through */
    case SOC_PHY_CONTROL_BERT_RX_ERROR_BYTES:   /* fall through */
    case SOC_PHY_CONTROL_BERT_RX_ERROR_PACKETS: /* fall through */
    case SOC_PHY_CONTROL_BERT_PATTERN:          /* fall through */
    case SOC_PHY_CONTROL_BERT_PACKET_SIZE:      /* fall through */
    case SOC_PHY_CONTROL_BERT_IPG:              /* fall through */
        rv = SOC_E_NONE;
        break;

    /* CUSTOM */
    case SOC_PHY_CONTROL_CUSTOM1:
      /* Obsolete ??
      *value = DEV_CFG_PTR(pc)->custom1;
      rv = SOC_E_NONE; */
      break;

    /* SOFTWARE RX LOS */
    case SOC_PHY_CONTROL_SOFTWARE_RX_LOS:
       /* Was 
       *value = DEV_CFG_PTR(pc)->sw_rx_los.enable ; */
       rv = SOC_E_NONE ;
       break ;

    /* UNAVAIL */
    case SOC_PHY_CONTROL_LINKDOWN_TRANSMIT:
    case SOC_PHY_CONTROL_PARALLEL_DETECTION:
       rv = SOC_E_UNAVAIL;
       break;
#endif
    case SOC_PHY_CONTROL_PCS_SPEED_ST_ENTRY:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamEntry, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_HCD:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamHCD, value) ;
        rv = SOC_E_NONE ;
        break ;
        /*
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CLEAR:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamClear, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PLL_DIVIDER:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamPllDiv, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PLL_DIVIDER:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamPllDiv, value) ;
        rv = SOC_E_NONE ;
        break ;
        */
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PMA_OS:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamPmaOS, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PMA_OS:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamPmaOS, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_SCR_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamScramble, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_SCR_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamScramble, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_ENCODE_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamEncode, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_ENCODE_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamEncode, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CL48_CHECK_END:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamCl48CheckEnd, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_CL48_CHECK_END:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamCl48CheckEnd, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_BLK_SYNC_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamBlkSync, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_BLK_SYNC_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamBlkSync, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_REORDER_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamReorder, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_REORDER_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamReorder, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CL36_EN:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamCl36Enable, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_CL36_EN:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamCl36Enable, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_DESCR1_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamDescr1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_DESCR1_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamDescr1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_DEC1_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamDecode1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_DEC1_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamDecode1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_DESKEW_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamDeskew, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_DESKEW_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamDeskew, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_DESC2_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamDescr2, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_DESC2_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamDescr2, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_DESC2_BYTE_DEL:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamDescr2ByteDel, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_DESC2_BYTE_DEL:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamDescr2ByteDel, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_BRCM64B66_DESCR:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamBrcm64B66, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_BRCM64B66_DESCR:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamBrcm64B66, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_SGMII_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamSgmii, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_SGMII_MODE:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamSgmii, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CLKCNT0:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamClkcnt0, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_CLKCNT0:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamClkcnt0, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CLKCNT1:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamClkcnt1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_CLKCNT1:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamClkcnt1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_LPCNT0:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamLpcnt0, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_LPCNT0:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamLpcnt0, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_LPCNT1:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamLpcnt1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_LPCNT1:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamLpcnt1, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_MAC_CGC:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamMacCGC, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_MAC_CGC:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamMacCGC, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PCS_REPCNT:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamRepcnt, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PCS_REPCNT:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamRepcnt, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PCS_CREDTEN:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamCrdtEn, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PCS_CREDTEN:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamCrdtEn, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PCS_CLKCNT:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamPcsClkcnt, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PCS_CLKCNT:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamPcsClkcnt, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_PCS_CGC:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamPcsCGC, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_PCS_CGC:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamPcsCGC, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_HTO_CL72_EN:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeHTO, phymodPcsUserSpeedParamCl72En, value) ;
        rv = SOC_E_NONE ;
        break ;
    case SOC_PHY_CONTROL_PCS_SPEED_ST_CL72_EN:
        tsce_user_speed_get(pmc, pCfg->core_device_aux_modes, phymodPcsUserSpeedModeST, phymodPcsUserSpeedParamCl72En, value) ;
        rv = SOC_E_NONE ;
        break ;
    default:
        rv = SOC_E_UNAVAIL;
        break; 
    }
    return rv;
}     

/*
 * Function:
 *      phy_tsce_per_lane_control_set
 * Purpose:
 *      Configure PHY device specific control fucntion. 
 * Parameters:
 *      unit  - StrataSwitch unit #.
 *      port  - StrataSwitch port #. 
 *      lane  - lane number
 *      type  - Control to update 
 *      value - New setting for the control 
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_per_lane_control_set(int unit, soc_port_t port, int lane, soc_phy_control_t type, uint32 value)
{
    int                 rv;
    phy_ctrl_t          *pc;
    soc_phymod_ctrl_t   *pmc;

    /* locate phy control, phymod control, and the configuration data */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }
    pmc = &pc->phymod_ctrl;

    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return SOC_E_PARAM;
    }

    switch(type) {
    case SOC_PHY_CONTROL_PREEMPHASIS:
        rv = tsce_per_lane_preemphasis_set(pmc, lane, value);
        break; 
    case SOC_PHY_CONTROL_DRIVER_CURRENT:
        rv = tsce_per_lane_driver_current_set(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_DRIVER_POST2_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_RX_TAP1:
        rv = tsce_per_lane_rx_dfe_tap_control_set (pmc, lane, 0 /* tap */, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP1_RELEASE:
        rv = tsce_per_lane_rx_dfe_tap_control_set (pmc, lane, 0 /* tap */, 0 /* release */, 0x8000);
        break;
    case SOC_PHY_CONTROL_RX_TAP2:
        rv = tsce_per_lane_rx_dfe_tap_control_set (pmc, lane, 1 /* tap */, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP2_RELEASE:
        rv = tsce_per_lane_rx_dfe_tap_control_set (pmc, lane, 1 /* tap */, 0 /* release */, 0x8000);
        break;
    case SOC_PHY_CONTROL_RX_TAP3:
        rv = tsce_per_lane_rx_dfe_tap_control_set (pmc, lane, 2 /* tap */, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP3_RELEASE:
        rv = tsce_per_lane_rx_dfe_tap_control_set (pmc, lane, 2 /* tap */, 0 /* release */, 0x8000);
        break;
    case SOC_PHY_CONTROL_RX_TAP4:
        rv = tsce_per_lane_rx_dfe_tap_control_set (pmc, lane, 3 /* tap */, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP4_RELEASE:
        rv = tsce_per_lane_rx_dfe_tap_control_set (pmc, lane, 3 /* tap */, 0 /* release */, 0x8000);
        break;
    case SOC_PHY_CONTROL_RX_TAP5:
        rv = tsce_per_lane_rx_dfe_tap_control_set (pmc, lane, 4 /* tap */, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP5_RELEASE:
        rv = tsce_per_lane_rx_dfe_tap_control_set (pmc, lane, 4 /* tap */, 0 /* release */, 0x8000);
        break;

    case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
        rv = tsce_per_lane_prbs_poly_set(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
        rv = tsce_per_lane_prbs_tx_invert_data_set(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
        /* TX_ENABLE does both tx and rx */
        rv = tsce_per_lane_prbs_tx_enable_set(pmc, lane, value);
        break; 
    case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
        rv = SOC_E_NONE;
        break;
    case SOC_PHY_CONTROL_RX_PEAK_FILTER:
        rv = tsce_per_lane_rx_peak_filter_set(pmc, lane, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_LOW_FREQ_PEAK_FILTER:
        rv = tsce_per_lane_rx_low_freq_filter_set(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_RX_VGA:
        rv = tsce_per_lane_rx_vga_set(pmc, lane, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_VGA_RELEASE:
        rv = tsce_per_lane_rx_vga_set(pmc, lane, 0 /* release */, 0x8000);
        break;
    case SOC_PHY_CONTROL_RX_PLUS1_SLICER:
        rv = SOC_E_UNAVAIL;
        break; 
    case SOC_PHY_CONTROL_RX_MINUS1_SLICER:
        rv = SOC_E_UNAVAIL;
        break; 
    case SOC_PHY_CONTROL_RX_D_SLICER:
        rv = SOC_E_UNAVAIL;
        break;
    default:
        rv = SOC_E_UNAVAIL;
        break; 
    }

    return rv;
}

/*
 * Function:
 *      phy_tsce_per_lane_control_get
 * Purpose:
 *      Configure PHY device specific control fucntion. 
 * Parameters:
 *      unit  - StrataSwitch unit #.
 *      port  - StrataSwitch port #. 
 *      lane  - lane number
 *      type  - Control to update 
 *      value - New setting for the control 
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_tsce_per_lane_control_get(int unit, soc_port_t port, int lane,
                     soc_phy_control_t type, uint32 *value)
{
    int                 rv;
    phy_ctrl_t          *pc;
    soc_phymod_ctrl_t   *pmc;

    /* locate phy control, phymod control, and the configuration data */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }
    pmc = &pc->phymod_ctrl;

    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return SOC_E_PARAM;
    }

    switch(type) {

    case SOC_PHY_CONTROL_PREEMPHASIS:
        rv = tsce_per_lane_preemphasis_get(pmc, lane, value);
        break; 
    case SOC_PHY_CONTROL_DRIVER_CURRENT:
        rv = tsce_per_lane_driver_current_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_DRIVER_POST2_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
        rv = tsce_per_lane_prbs_poly_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
        rv = tsce_per_lane_prbs_tx_invert_data_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
        rv = tsce_per_lane_prbs_tx_enable_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
        rv = tsce_per_lane_prbs_rx_enable_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_RX_STATUS:
        rv = tsce_per_lane_prbs_rx_status_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_RX_PEAK_FILTER:
        rv = tsce_per_lane_rx_peak_filter_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_RX_VGA:
        rv = tsce_per_lane_rx_vga_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP1:
        rv = tsce_per_lane_rx_dfe_tap_control_get(pmc, lane, 0, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP2:
        rv = tsce_per_lane_rx_dfe_tap_control_get(pmc, lane, 1, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP3:
        rv = tsce_per_lane_rx_dfe_tap_control_get(pmc, lane, 2, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP4:
        rv = tsce_per_lane_rx_dfe_tap_control_get(pmc, lane, 3, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP5:
        rv = tsce_per_lane_rx_dfe_tap_control_get(pmc, lane, 4, value);
        break;
    case SOC_PHY_CONTROL_RX_LOW_FREQ_PEAK_FILTER:
        rv = tsce_per_lane_rx_low_freq_filter_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_RX_PLUS1_SLICER:
        rv = SOC_E_UNAVAIL;
        break; 
    case SOC_PHY_CONTROL_RX_MINUS1_SLICER:
        rv = SOC_E_UNAVAIL;
        break; 
    case SOC_PHY_CONTROL_RX_D_SLICER:
        rv = SOC_E_UNAVAIL;
        break; 
    default:
        rv = SOC_E_UNAVAIL;
        break; 
    }
    return rv;
}

/*
 * Function:
 *      phy_tsce_reg_read
 * Purpose:
 *      Routine to read PHY register
 * Parameters:
 *      unit         - BCM unit number
 *      port         - Port number
 *      flags        - Flags which specify the register type
 *      phy_reg_addr - Encoded register address
 *      phy_data     - (OUT) Value read from PHY register
 * Note:
 *      This register read function is not thread safe. Higher level
 * function that calls this function must obtain a per port lock
 * to avoid overriding register page mapping between threads.
 */
STATIC int
phy_tsce_reg_read(int unit, soc_port_t port, uint32 flags,
                  uint32 phy_reg_addr, uint32 *phy_reg_data)
{
    phy_ctrl_t *pc;
    soc_phymod_ctrl_t *pmc;
    phymod_phy_access_t *pm_phy;
    int idx;

    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    pmc = &pc->phymod_ctrl;
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;
        return phymod_phy_reg_read(pm_phy, phy_reg_addr, phy_reg_data);
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_tsce_reg_write
 * Purpose:
 *      Routine to write PHY register
 * Parameters:
 *      uint         - BCM unit number
 *      pc           - PHY state
 *      flags        - Flags which specify the register type
 *      phy_reg_addr - Encoded register address
 *      phy_data     - Value write to PHY register
 * Note:
 *      This register read function is not thread safe. Higher level
 * function that calls this function must obtain a per port lock
 * to avoid overriding register page mapping between threads.
 */
STATIC int
phy_tsce_reg_write(int unit, soc_port_t port, uint32 flags,
                   uint32 phy_reg_addr, uint32 phy_reg_data)
{
    phy_ctrl_t *pc;
    soc_phymod_ctrl_t *pmc;
    phymod_phy_access_t *pm_phy;
    int idx;

    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    pmc = &pc->phymod_ctrl;
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;
        SOC_IF_ERROR_RETURN
            (phymod_phy_reg_write(pm_phy, phy_reg_addr, phy_reg_data));
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_tsce_reg_modify
 * Purpose:
 *      Routine to write PHY register
 * Parameters:
 *      uint         - BCM unit number
 *      pc           - PHY state
 *      flags        - Flags which specify the register type
 *      phy_reg_addr - Encoded register address
 *      phy_mo_data  - New value for the bits specified in phy_mo_mask
 *      phy_mo_mask  - Bit mask to modify
 * Note:
 *      This function is not thread safe. Higher level
 * function that calls this function must obtain a per port lock
 * to avoid overriding register page mapping between threads.
 */
STATIC int
phy_tsce_reg_modify(int unit, soc_port_t port, uint32 flags,
                    uint32 phy_reg_addr, uint32 phy_data,
                    uint32 phy_data_mask)
{
    phy_ctrl_t *pc;
    soc_phymod_ctrl_t *pmc;
    phymod_phy_access_t *pm_phy;
    uint32 data32, tmp;
    int idx;

    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    pmc = &pc->phymod_ctrl;
    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;
        SOC_IF_ERROR_RETURN
            (phymod_phy_reg_read(pm_phy, phy_reg_addr, &data32));
        tmp = data32;
        data32 &= ~(phy_data_mask);
        data32 |= (phy_data & phy_data_mask);
        if (data32 != tmp) {
            SOC_IF_ERROR_RETURN
                (phymod_phy_reg_write(pm_phy, phy_reg_addr, data32));
        }
    }
    return SOC_E_NONE;
}

STATIC int
_tsce_device_destroy(temod_device_aux_modes_t *device)
{
    if (device == NULL) {
        return SOC_E_PARAM;
    }
    sal_free(device);

    return SOC_E_NONE;    
}

STATIC void
phy_tsce_cleanup(soc_phymod_ctrl_t *pmc)
{
    int idx;
    soc_phymod_phy_t *phy;
    soc_phymod_core_t *core;
    temod_device_aux_modes_t *device ;

    for (idx = 0; idx < pmc->num_phys ; idx++) {
        phy = pmc->phy[idx];
        core = phy->core;
        device = (temod_device_aux_modes_t *)core->device_aux_modes ;
        /* Destroy core object if not used anymore */
        if (core && core->ref_cnt) {
            if (--core->ref_cnt == 0) {
                PHYMOD_VDBG(TEMOD_DBG_MEM, NULL,("clean_up device=%p core_p=%p\n", (void *)device, (void *)core)) ;
                _tsce_device_destroy(device) ;
                soc_phymod_core_destroy(pmc->unit, core);
            }
        }

        /* Destroy phy object */
        if (phy) {
            PHYMOD_VDBG(TEMOD_DBG_MEM, NULL,("clean_up phy=%p\n", (void *)phy)) ;
            soc_phymod_phy_destroy(pmc->unit, phy);
        }
    }
    pmc->num_phys = 0;
}

STATIC void
phy_tsce_core_init(phy_ctrl_t *pc, soc_phymod_core_t *core,
                   phymod_bus_t *core_bus, uint32 core_addr)
{
    phymod_core_access_t *pm_core;
    phymod_access_t *pm_acc;

    core->unit = pc->unit;
    core->port = pc->port;
    core->read = pc->read;
    core->write = pc->write;
    core->wrmask = pc->wrmask;

    pm_core = &core->pm_core;
    phymod_core_access_t_init(pm_core);
    pm_acc = &pm_core->access;
    phymod_access_t_init(pm_acc);
    PHYMOD_ACC_USER_ACC(pm_acc) = core;
    PHYMOD_ACC_BUS(pm_acc) = core_bus;
    PHYMOD_ACC_ADDR(pm_acc) = core_addr;

    if (soc_property_port_get(pc->unit, pc->port, "tsce_sim", 0) == 45) {
        PHYMOD_PHYMOD_ACC_F_CLAUSE45_SET(pm_acc);
    }

    return;
}

STATIC int
_tsce_device_create_attach(soc_phymod_core_t *core, uint32_t core_id) 
{
    temod_device_aux_modes_t *new_device ;
    new_device = sal_alloc(sizeof(temod_device_aux_modes_t), "temod_device_aux_modes");
    if (new_device == NULL) {
        return SOC_E_MEMORY;
    }
    sal_memset(new_device, 0 ,sizeof(temod_device_aux_modes_t));

    new_device->core_id = core_id ;
    core->device_aux_modes = (void *)new_device ;
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_tsce_probe
 * Purpose:
 *      xx
 * Parameters:
 *      pc->phy_id   (IN)
 *      pc->unit     (IN)
 *      pc->port     (IN)
 *      pc->size     (OUT) - memory required by phy port
 *      pc->dev_name (OUT) - set to port device name
 *  
 * Note:
 */
STATIC int
phy_tsce_probe(int unit, phy_ctrl_t *pc)
{
    int rv, idx;
    uint32 lane_map, num_phys, core_id, phy_id, found;
    phymod_bus_t core_bus;
    phymod_dispatch_type_t phy_type;
    phymod_core_access_t *pm_core;
    phymod_phy_access_t *pm_phy;
    phymod_access_t *pm_acc;
    soc_port_info_t  *soc_port_info;
    soc_phymod_ctrl_t *pmc;
    soc_phymod_phy_t *phy;
    soc_phymod_core_t *core;
    soc_phymod_core_t core_probe;
    soc_info_t *si;
    phyident_core_info_t core_info[8];  
    int array_max = 8;
    int array_size = 0;
    int port;
    int phy_port;  /* physical port number */

    /* Initialize PHY bus */
    SOC_IF_ERROR_RETURN(phymod_bus_t_init(&core_bus));
    core_bus.bus_name = "tsce_sim"; 
    core_bus.read = _tsce_reg_read; 
    core_bus.write = _tsce_reg_write;

    /* Configure PHY bus properties */
    if (pc->wrmask) {
        PHYMOD_BUS_CA_WR_MODIFY_SET(&core_bus);
        PHYMOD_BUS_CA_LANE_CTRL_SET(&core_bus);
    }

    port = pc->port;
    pmc = &pc->phymod_ctrl;
    si = &SOC_INFO(unit);
    if (soc_feature(unit, soc_feature_logical_port_num)) {
        phy_port = si->port_l2p_mapping[port];
    } else {
        phy_port = port;
    }

    /* Install clean up function */
    pmc->unit = pc->unit;
    pmc->cleanup = phy_tsce_cleanup;
    pmc->symbols = &bcmi_tsce_xgxs_symbols;

    
    if (SOC_IS_GREYHOUND(unit)) {
        int i, blk;
        blk = -1;
        pc->lane_num = (phy_port  + 2) % 4;
        for (i=0; i< SOC_DRIVER(unit)->port_num_blktype; i++){
            blk = SOC_PORT_IDX_BLOCK(unit, phy_port, i);
            if (PBMP_MEMBER(SOC_BLOCK_BITMAP(unit, blk), port)){
                break;
            }
        }
        if (blk == -1) {
            pc->chip_num = -1;
        } else {
            pc->chip_num = SOC_BLOCK_NUMBER(unit, blk);
        }
    } else {
        pc->lane_num = (phy_port - 1) % 4;
        soc_port_info = &(SOC_DRIVER(unit)->port_info[phy_port]);
        pc->chip_num = SOC_BLOCK_NUMBER(unit, soc_port_info->blk);
    }

    /* request memory for the configuration structure */
    pc->size = sizeof(tsce_config_t);

    /* Bit N corresponds to lane N in lane_map */
    lane_map = 0xf;
    num_phys = 1;
    switch (si->port_num_lanes[port]) {
    case 10:
        num_phys = 3;
        pc->phy_mode = PHYCTRL_MULTI_CORE_PORT; 
        break;
    case 4:
        pc->phy_mode = PHYCTRL_QUAD_LANE_PORT; 
        break;
    case 2:
        lane_map = 0x3;
        pc->phy_mode = PHYCTRL_DUAL_LANE_PORT;
        break;
    case 1:
    case 0:
        lane_map = 0x1;
        pc->phy_mode = PHYCTRL_ONE_LANE_PORT;
        break;
    default:
        return SOC_E_CONFIG;
    }
    lane_map <<= pc->lane_num;

    /* we need to get the other core id if more than 1 core per port */
    if (num_phys > 1) {
        /* get the other core address */
        SOC_IF_ERROR_RETURN
            (soc_phy_addr_multi_get(unit, port, array_max, &array_size, &core_info[0]));
    } else {
        core_info[0].mdio_addr = pc->phy_id;
    }

    phy_type = phymodDispatchTypeTsce;

    /* Probe cores */
    for (idx = 0; idx < num_phys ; idx++) {
        phy_tsce_core_init(pc, &core_probe, &core_bus,
                           core_info[idx].mdio_addr);
        /* Check that core is indeed an TSC/Eagle core */
        pm_core = &core_probe.pm_core;
        pm_core->type = phy_type;
        rv = phymod_core_identify(pm_core, 0, &found);
        if (SOC_FAILURE(rv)) {
            return rv;
        }
        if (!found) {
            return SOC_E_NOT_FOUND;
        }
    }

    rv = SOC_E_NONE;
    for (idx = 0; idx < num_phys ; idx++) {
        /* Set core and phy IDs based on PHY address */
        core_id = pc->phy_id + idx;
        phy_id = (lane_map << 16) | core_id;

        /* Create PHY object */
        rv = soc_phymod_phy_create(unit, phy_id, &pmc->phy[idx]);
        if (SOC_FAILURE(rv)) {
            break;
        }
        pmc->num_phys++;

        /* Initialize phy object */
        phy = pmc->phy[idx];
        pm_phy = &phy->pm_phy;
        phymod_phy_access_t_init(pm_phy);

        /* Find or create associated core object */
        rv = soc_phymod_core_find_by_id(unit, core_id, &phy->core);
        if (rv == SOC_E_NOT_FOUND) {
            rv = soc_phymod_core_create(unit, core_id, &phy->core);
            rv |= _tsce_device_create_attach(phy->core, core_id) ;
        }
        if (SOC_FAILURE(rv)) {
            break;
        }        
    }
    if (SOC_FAILURE(rv)) {
        phy_tsce_cleanup(pmc);
        return rv;
    }

    for (idx = 0; idx < pmc->num_phys ; idx++) {
        phy = pmc->phy[idx];
        core = phy->core;
        pm_core = &core->pm_core;

        /* Initialize core object if newly created */
        if (core->ref_cnt == 0) {
            sal_memcpy(&core->pm_bus, &core_bus, sizeof(core->pm_bus));
            phy_tsce_core_init(pc, core, &core->pm_bus,
                               core_info[idx].mdio_addr);
            /* Set dispatch type */
            pm_core->type = phy_type;
        }
        core->ref_cnt++;        

        /* Initialize phy access based on associated core */
        pm_acc = &phy->pm_phy.access;
        sal_memcpy(pm_acc, &pm_core->access, sizeof(*pm_acc));
        phy->pm_phy.type = phy_type;
        PHYMOD_ACC_LANE(pm_acc) = lane_map;
    }

    return SOC_E_NONE;
}


/***********************************************************************
 *
 * PASS-THROUGH NOTIFY ROUTINES
 *
 ***********************************************************************/

/*
 * Function:
 *      _tsce_notify_duplex
 * Purpose:
 *      Program duplex if (and only if) serdestsce is an intermediate PHY.
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number.
 *      duplex - Boolean, TRUE indicates full duplex, FALSE
 *              indicates half.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      If PHY_FLAGS_FIBER is set, it indicates the PHY is being used to
 *      talk directly to an external fiber module.
 *
 *      If PHY_FLAGS_FIBER is clear, the PHY is being used in
 *      pass-through mode to talk to an external SGMII PHY.
 *
 *      When used in pass-through mode, autoneg must be turned off and
 *      the speed/duplex forced to match that of the external PHY.
 */

STATIC int
_tsce_notify_duplex(int unit, soc_port_t port, uint32 duplex)
{
    return SOC_E_NONE;
}

/*
 * Function:
 *      _tsce_stop
 * Purpose:
 *      Put serdestsce SERDES in or out of reset depending on conditions
 */

STATIC int
_tsce_stop(int unit, soc_port_t port)
{
    phy_ctrl_t* pc; 
    soc_phymod_ctrl_t *pmc;
    phymod_phy_access_t *pm_phy;
    phymod_phy_power_t phy_power;
    int  stop, copper, speed;

    pc = INT_PHY_SW_STATE(unit, port);
    pmc = &pc->phymod_ctrl;
    pm_phy = &pmc->phy[0]->pm_phy;

    if (pc->phy_mode != PHYCTRL_ONE_LANE_PORT) {
        return SOC_E_NONE;
    }

    /* 'Stop' only for speeds < 10G. */ 
    SOC_IF_ERROR_RETURN
        (phy_tsce_speed_get(unit,port,&speed));
    if(10000 <= speed) {
        return SOC_E_NONE;
    }
    copper = (pc->stop &
              PHY_STOP_COPPER) != 0;

    stop = ((pc->stop &
             (PHY_STOP_PHY_DIS |
              PHY_STOP_DRAIN)) != 0 ||
            (copper &&
             (pc->stop &
              (PHY_STOP_MAC_DIS |
               PHY_STOP_DUPLEX_CHG |
               PHY_STOP_SPEED_CHG)) != 0));

    LOG_INFO(BSL_LS_SOC_PHY,
             (BSL_META_U(pc->unit,
                         "qsgmiie_stop: u=%d p=%d copper=%d stop=%d flg=0x%x\n"),
              unit, port, copper, stop,
              pc->stop));
    if (stop) {
        phy_power.tx = phymodPowerOff;
        phy_power.rx = phymodPowerOff;
    } else {
        phy_power.tx = phymodPowerOn;
        phy_power.rx = phymodPowerOn;
    }
    SOC_IF_ERROR_RETURN
        (phymod_phy_power_set(pm_phy, &phy_power));
    return SOC_E_NONE;
}

/*
 * Function:
 *      _tsce_notify_stop
 * Purpose:
 *      Add a reason to put serdestsce PHY in reset.
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number.
 *      flags - Reason to stop
 * Returns:     
 *      SOC_E_XXX
 */

STATIC int
_tsce_notify_stop(int unit, soc_port_t port, uint32 flags)
{
    INT_PHY_SW_STATE(unit, port)->stop |= flags;
    return _tsce_stop(unit, port);
}

/*  
 * Function:
 *      _tsce_notify_resume
 * Purpose:
 *      Remove a reason to put serdestsce PHY in reset.
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number.
 *      flags - Reason to stop
 * Returns:     
 *      SOC_E_XXX
 */

STATIC int
_tsce_notify_resume(int unit, soc_port_t port, uint32 flags)
{   
    INT_PHY_SW_STATE(unit, port)->stop &= ~flags;
    return _tsce_stop(unit, port);
}

/*
 * Function:
 *      _tsce_notify_speed
 * Purpose:
 *      Program duplex if (and only if) serdestsce is an intermediate PHY.
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number.
 *      speed - Speed to program.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *      If PHY_FLAGS_FIBER is set, it indicates the PHY is being used to
 *      talk directly to an external fiber module.
 *
 *      If PHY_FLAGS_FIBER is clear, the PHY is being used in
 *      pass-through mode to talk to an external SGMII PHY.
 *
 *      When used in pass-through mode, autoneg must be turned off and
 *      the speed/duplex forced to match that of the external PHY.
 */

STATIC int
_tsce_notify_speed(int unit, soc_port_t port, uint32 speed)
{
    phy_ctrl_t* pc; 
    tsce_config_t *pCfg;
    int  fiber;

    pc = INT_PHY_SW_STATE(unit, port);
    pCfg = (tsce_config_t *) pc->driver_data;
    fiber = pCfg->fiber_pref;

    LOG_INFO(BSL_LS_SOC_PHY,
             (BSL_META_U(pc->unit,
                         "_qsgmiie_notify_speed: "
                         "u=%d p=%d speed=%d fiber=%d\n"),
              unit, port, speed, fiber));

    if (SAL_BOOT_SIMULATION) {
        return SOC_E_NONE;
    }

    /* Put SERDES PHY in reset */
    SOC_IF_ERROR_RETURN
        (_tsce_notify_stop(unit, port, PHY_STOP_SPEED_CHG));

    /* Update speed */
    SOC_IF_ERROR_RETURN
        (phy_tsce_speed_set(unit, port, speed));

    /* Take SERDES PHY out of reset */
    SOC_IF_ERROR_RETURN
        (_tsce_notify_resume(unit, port, PHY_STOP_SPEED_CHG));

    /* Autonegotiation must be turned off to talk to external PHY */
    if (!PHY_SGMII_AUTONEG_MODE(unit, port) && PHY_EXTERNAL_MODE(unit, port)) {
        SOC_IF_ERROR_RETURN
            (phy_tsce_an_set(unit, port, FALSE));
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      tsce_media_setup
 * Purpose:     
 *      Configure 
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number.
 *      fiber_mode - Configure for fiber mode
 *      fiber_pref - Fiber preferrred (if fiber mode)
 * Returns:     
 *      SOC_E_XXX
 */
STATIC int
_tsce_notify_interface(int unit, soc_port_t port, uint32 intf)
{
    return SOC_E_NONE;
}

/*
 * Function:
 *      _tsce_notify_mac_loopback
 * Purpose:
 *      Turn on rx clock compensation in mac loopback mode for
 *      applicable XGXS devices
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number.
 *      enable - TRUE: In MAC loopback mode, FALSE: Not in MAC loopback mode 
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
_tsce_notify_mac_loopback(int unit, soc_port_t port, uint32 enable)
{
    return SOC_E_NONE;
}

STATIC int
phy_tsce_notify(int unit, soc_port_t port,
                       soc_phy_event_t event, uint32 value)
{
    int             rv;
    rv = SOC_E_NONE ;
    if (event >= phyEventCount) {
        return SOC_E_PARAM;
    }

    switch(event) {
    case phyEventInterface:
        rv = (_tsce_notify_interface(unit, port, value));
        break;
    case phyEventDuplex:
        rv = (_tsce_notify_duplex(unit, port, value));
        break;
    case phyEventSpeed:
        rv = (_tsce_notify_speed(unit, port, value));
        break;
    case phyEventStop:
        rv = (_tsce_notify_stop(unit, port, value));
        break;
    case phyEventResume:
        rv = (_tsce_notify_resume(unit, port, value));
        break;
    case phyEventAutoneg:
#if 0
        rv = (_tsce_an_set(unit, port, value));
#endif
        break;
    case phyEventMacLoopback:
        rv = (_tsce_notify_mac_loopback(unit, port, value));
        break;
    default:
        rv = SOC_E_UNAVAIL;
        break;
    }

    return rv;
}

/*
 * Function:
 *      phy_tsce_diag_ctrl
 * Purpose:
 *      xx
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 

 * Returns:     
 *      SOC_E_NONE
 */

STATIC int
phy_tsce_diag_ctrl(
    int unit,        /* unit */
    soc_port_t port, /* port */
    uint32 inst,     /* the specific device block the control action directs to */
    int op_type,     /* operation types: read,write or command sequence */
    int op_cmd,      /* command code */
    void *arg)       /* command argument based on op_type/op_cmd */
{
    switch(op_cmd) {
        case PHY_DIAG_CTRL_DSC:
            LOG_INFO(BSL_LS_SOC_PHY,
                     (BSL_META_U(unit,
                                 "phy_temod_diag_ctrl: "
                                 "u=%d p=%d PHY_DIAG_CTRL_DSC 0x%x\n"),
                      unit, port, PHY_DIAG_CTRL_DSC));
			SOC_IF_ERROR_RETURN(tsce_uc_status_dump(unit, port, arg));
            break;
        case PHY_DIAG_CTRL_PCS:
            LOG_INFO(BSL_LS_SOC_PHY,
                     (BSL_META_U(unit,
                                 "phy_temod_diag_ctrl: "
                                 "u=%d p=%d PHY_DIAG_CTRL_PCS 0x%x\n"),
                      unit, port, PHY_DIAG_CTRL_PCS));
			SOC_IF_ERROR_RETURN(tsce_pcs_status_dump(unit, port, arg));
            break;
        default:
            if (op_type == PHY_DIAG_CTRL_SET) {
                SOC_IF_ERROR_RETURN(phy_tsce_control_set(unit,port,op_cmd,PTR_TO_INT(arg)));
            } else if (op_type == PHY_DIAG_CTRL_GET) {
                SOC_IF_ERROR_RETURN(phy_tsce_control_get(unit,port,op_cmd,(uint32 *)arg));
            }
            break ;
        }
    return (SOC_E_NONE);
}


#ifdef BROADCOM_DEBUG
void
tsce_state_dump(int unit, soc_port_t port)
{
    /* Show PHY configuration summary */
    /* Lane status */
    /* Show Counters */
    /* Dump Registers */
}
#endif /* BROADCOM_DEBUG */

/*
 * Variable:
 *      tsce_drv
 * Purpose:
 *      Phy Driver for TSC/Eagle 
 */
phy_driver_t phy_tsce_drv = {
    /* .drv_name                      = */ "TSC/Eagle PHYMOD PHY Driver",
    /* .pd_init                       = */ phy_tsce_init,
    /* .pd_reset                      = */ phy_null_reset,
    /* .pd_link_get                   = */ phy_tsce_link_get,
    /* .pd_enable_set                 = */ phy_tsce_enable_set,
    /* .pd_enable_get                 = */ phy_tsce_enable_get,
    /* .pd_duplex_set                 = */ phy_null_set,
    /* .pd_duplex_get                 = */ phy_tsce_duplex_get,
    /* .pd_speed_set                  = */ phy_tsce_speed_set,
    /* .pd_speed_get                  = */ phy_tsce_speed_get,
    /* .pd_master_set                 = */ phy_null_set,
    /* .pd_master_get                 = */ phy_null_zero_get,
    /* .pd_an_set                     = */ phy_tsce_an_set,
    /* .pd_an_get                     = */ phy_tsce_an_get,
    /* .pd_adv_local_set              = */ NULL, /* Deprecated */
    /* .pd_adv_local_get              = */ NULL, /* Deprecated */
    /* .pd_adv_remote_get             = */ NULL, /* Deprecated */ 
    /* .pd_lb_set                     = */ phy_tsce_lb_set,
    /* .pd_lb_get                     = */ phy_tsce_lb_get,
    /* .pd_interface_set              = */ phy_tsce_interface_set,
    /* .pd_interface_get              = */ phy_tsce_interface_get,
    /* .pd_ability                    = */ NULL, /* Deprecated */ 
    /* .pd_linkup_evt                 = */ NULL,
    /* .pd_linkdn_evt                 = */ NULL,
    /* .pd_mdix_set                   = */ phy_null_mdix_set,
    /* .pd_mdix_get                   = */ phy_null_mdix_get,
    /* .pd_mdix_status_get            = */ phy_null_mdix_status_get,
    /* .pd_medium_config_set          = */ NULL,
    /* .pd_medium_config_get          = */ NULL,
    /* .pd_medium_get                 = */ phy_null_medium_get,
    /* .pd_cable_diag                 = */ NULL,
    /* .pd_link_change                = */ NULL,
    /* .pd_control_set                = */ phy_tsce_control_set,
    /* .pd_control_get                = */ phy_tsce_control_get,
    /* .pd_reg_read                   = */ phy_tsce_reg_read,
    /* .pd_reg_write                  = */ phy_tsce_reg_write,
    /* .pd_reg_modify                 = */ phy_tsce_reg_modify,
    /* .pd_notify                     = */ phy_tsce_notify,
    /* .pd_probe                      = */ phy_tsce_probe,
    /* .pd_ability_advert_set         = */ phy_tsce_ability_advert_set, 
    /* .pd_ability_advert_get         = */ phy_tsce_ability_advert_get,
    /* .pd_ability_remote_get         = */ phy_tsce_ability_remote_get,
    /* .pd_ability_local_get          = */ phy_tsce_ability_local_get,
    /* .pd_firmware_set               = */ phy_tsce_firmware_set,
    /* .pd_timesync_config_set        = */ NULL,
    /* .pd_timesync_config_get        = */ NULL,
    /* .pd_timesync_control_set       = */ NULL,
    /* .pd_timesync_control_set       = */ NULL,
    /* .pd_diag_ctrl                  = */ phy_tsce_diag_ctrl,
    /* .pd_lane_control_set           = */ phy_tsce_per_lane_control_set,    
    /* .pd_lane_control_get           = */ phy_tsce_per_lane_control_get
};

#else
int _tsce_not_empty;
#endif /*  INCLUDE_XGXS_TSCE */

