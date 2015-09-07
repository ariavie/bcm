/*
 * $Id: qsgmiie.c,v 1.1.2.25 Broadcom SDK $
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
 * File:        qsgmiie.c
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
 *     APIs (nor ".pd_xx" calls, nor to .phy_qsgmiie_per_lane_control_set and
 *     .phy_qsgmiie_per_lane_control_get APIs.
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
 *   qsgmiie_config_t
 *   Driver specific data.  This structure is used by qsgmiie to hold
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

#if defined(INCLUDE_SERDES_QSGMIIE)

#include <phymod/phymod.h>
#include <phymod/phymod_diagnostics.h>
#include <phymod/chip/bcmi_qsgmiie_serdes_sym.h>
#include <phymod/chip/qsgmiie.h>

#define NUM_LANES             4    /* num of lanes per core */
#define MAX_NUM_LANES         4    /* max num of lanes per port */
#define QSGMIIE_NO_CFG_VALUE     (-1)

/*
 * The DBG_OK macro will bypass the function body and return success
 * if the PHY simulator is enabled for a particular port. This allows
 * system initialization to complete even if the driver has not been
 * fully tested/debugged. As development progresses, we should be able
 * to remove all instances of this macro.
 */
#define DBG_OK() do { \
    if (soc_property_port_get(unit, port, "qsgmiie_sim", 0)) { \
        return SOC_E_NONE; \
    } \
} while (0)


#define PHYMOD_IF_CR4    (1 << SOC_PORT_IF_CR4)


typedef struct qsgmiie_speed_config_s {
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
} qsgmiie_speed_config_t;

#define NUM_PATTERN_DATA_INTS    8
typedef struct qsgmiie_pattern_s {
    uint32 pattern_data[NUM_PATTERN_DATA_INTS];
    uint32 pattern_length;
} qsgmiie_pattern_t;

#define QSGMIIE_LANE_NAME_LEN   30

typedef struct {
    uint16 serdes_id0;
    char   name[QSGMIIE_LANE_NAME_LEN];
} qsgmiie_dev_info_t;

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
    qsgmiie_pattern_t                  pattern;
    qsgmiie_speed_config_t             speed_config;

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
    qsgmiie_dev_info_t info;          /*serdes id info */
    phymod_tx_t tx_drive[TXDRV_ENTRY_NUM];

} qsgmiie_config_t;

#define QSGMIIE_IF_NULL   (1 << SOC_PORT_IF_NULL)
#define QSGMIIE_IF_SR     (1 << SOC_PORT_IF_SR)
#define QSGMIIE_IF_KR     (1 << SOC_PORT_IF_KR)
#define QSGMIIE_IF_KR4    (1 << SOC_PORT_IF_KR4)
#define QSGMIIE_IF_CR     (1 << SOC_PORT_IF_CR)
#define QSGMIIE_IF_CR4    (1 << SOC_PORT_IF_CR4)
#define QSGMIIE_IF_XFI    (1 << SOC_PORT_IF_XFI)
#define QSGMIIE_IF_SFI    (1 << SOC_PORT_IF_SFI)
#define QSGMIIE_IF_XLAUI  (1 << SOC_PORT_IF_XLAUI)
#define QSGMIIE_IF_XGMII  (1 << SOC_PORT_IF_XGMII)
#define QSGMIIE_IF_ILKN   (1 << SOC_PORT_IF_ILKN)



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

/*
 * Function:
 *      _qsgmiie_reg_read
 * Doc:
 *      register read operations
 * Parameters:
 *      unit            - (IN)  unit number
 *      core_addr       - (IN)  core address
 *      reg_addr        - (IN)  address to read
 *      val             - (OUT) read value
 */
STATIC int 
_qsgmiie_reg_read(void* user_acc, uint32_t core_addr, uint32_t reg_addr, uint32_t *val)
{
    uint16 data16;
    int rv;
    soc_phymod_core_t *core = (soc_phymod_core_t *)user_acc;

    rv = core->read(core->unit, core_addr, reg_addr, &data16);
    *val = data16;

    return rv;
}

/*
 * Function:
 *      _qsgmiie_reg_write
 * Doc:
 *      register write operations
 * Parameters:
 *      unit            - (IN)  unit number
 *      core_addr       - (IN)  core address
 *      reg_addr        - (IN)  address to write
 *      val             - (IN)  write value
 */
STATIC int 
_qsgmiie_reg_write(void* user_acc, uint32_t core_addr, uint32_t reg_addr, uint32_t val)
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
    return core->write(core->unit, core_addr, reg_addr, data16);
}


#define IS_DUAL_LANE_PORT(_pc) ((_pc)->phy_mode == PHYCTRL_DUAL_LANE_PORT) 
#define IND_LANE_MODE(_pc) (((_pc)->phy_mode == PHYCTRL_DUAL_LANE_PORT) || ((_pc)->phy_mode == PHYCTRL_ONE_LANE_PORT))  
#define MAIN0_SERDESID_REV_LETTER_SHIFT  0x14
#define MAIN0_SERDESID_REV_NUMBER_SHIFT  0x11

/*
 * Function:
 *      qsgmiie_show_serdes_info
 * Purpose:
 *      Show SerDes information.
 * Parameters:
 *      pc             - (IN)  phyctrl  oject
 *      pInfo          - (IN)  device info object
 *      rev_id         - (IN)  serdes revid
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
qsgmiie_show_serdes_info(phy_ctrl_t *pc, qsgmiie_dev_info_t *pInfo, phymod_core_info_t *core_info)
{
    uint32_t serdes_id0, len;

    pInfo->serdes_id0 = core_info->serdes_id ;

    /* This is TSC/Eagle device */
    serdes_id0 = pInfo->serdes_id0;
    sal_strcpy(pInfo->name,"QSGMIIE-");
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
    } else if (pc->phy_mode == PHYCTRL_QSGMII_CORE_PORT) {
        pInfo->name[len++] = pc->lane_num + '0';
    } else {
        pInfo->name[len++] = '4';
    }
    pInfo->name[len] = 0;  /* string terminator */

    if (len > QSGMIIE_LANE_NAME_LEN) {
        LOG_ERROR(BSL_LS_SOC_COMMON,
                  (BSL_META("QSGMIIE info string length %d exceeds max length 0x%x: u=%d p=%d\n"),
                   len,QSGMIIE_LANE_NAME_LEN, pc->unit, pc->port));
        return SOC_E_MEMORY;
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      qsgmiie_speed_to_interface_config_get
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
qsgmiie_speed_to_interface_config_get(qsgmiie_speed_config_t* speed_config, phymod_phy_inf_config_t* interface_config)
{
    /*
    int port_is_higig;
    int port_num_lanes;
    int scr_enabled;
    int fiber_pref;
    */

    /*
    port_is_higig = speed_config->port_is_higig;
    port_num_lanes = speed_config->port_num_lanes;
    scr_enabled = speed_config->scrambler_en;
    fiber_pref = speed_config->fiber_pref;
    */

    SOC_IF_ERROR_RETURN(phymod_phy_inf_config_t_init(interface_config));

    interface_config->data_rate = speed_config->speed;

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
    default:
            if (speed_config->fiber_pref) {
                interface_config->interface_type = phymodInterface1000X;
            } else {
                interface_config->interface_type = phymodInterfaceSGMII;
            }
            break;
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
        interface_config->ref_clock = phymodRefClk156Mhz;
        break;
    }
    return SOC_E_NONE;
}

/*
 * Function:
 *      qsgmiie_config_init
 * Purpose:     
 *      Determine phy configuration data for purposes of PHYMOD initialization.
 * 
 *      A side effect of this procedure is to save some per-logical port
 *      information in (qsgmiie_cfg_t *) &pc->driver_data;
 *
 * Parameters:
 *      unit                  - BCM unit number.
 *      port                  - Port number.
 *      logical_lane_offset   - starting logical lane number
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
qsgmiie_config_init(int unit, soc_port_t port, int logical_lane_offset,
                 phymod_core_init_config_t *core_init_config, 
                 phymod_phy_init_config_t  *pm_phy_init_config)
{
    phy_ctrl_t                   *pc;
    qsgmiie_speed_config_t          *speed_config;
    qsgmiie_config_t                *pCfg;
    phymod_tx_t                  *p_tx;
    int                          port_refclk_int;
    int                          port_num_lanes;
    int                          core_num;
    int                          phy_num_lanes;
    int                          port_is_higig;
    int                          phy_passthru;
    int                          phy_supports_dual_rate;
    int                          lane_map_rx, lane_map_tx;
    int                          i;
    int                          fiber_pref = 1;

    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }
    pCfg = (qsgmiie_config_t *) pc->driver_data;

    /* extract data from SOC_INFO */
    port_refclk_int = SOC_INFO(unit).port_refclk_int[port];
#if 0
    
    port_refclk_int = soc_property_port_get(unit,port, spn_XGXS_PHY_VCO_FREQ, pCfg->refclk);
#endif
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

    core_init_config->lane_map.num_of_lanes = NUM_LANES;

    lane_map_rx = soc_property_port_suffix_num_get(unit, port, core_num,
                                        spn_XGXS_RX_LANE_MAP, "core", 0x3210);
    for (i=0; i<NUM_LANES; i++) {
        core_init_config->lane_map.lane_map_rx[i] = (lane_map_rx >> (i * 4 /*4 bit per lane*/)) & 0xf;
    }
 
    lane_map_tx = soc_property_port_suffix_num_get(unit, port, core_num,
                                        spn_XGXS_TX_LANE_MAP, "core", 0x3210);
    for (i=0; i<NUM_LANES; i++) {
        core_init_config->lane_map.lane_map_tx[i] = (lane_map_tx >> (i * 4 /*4 bit per lane*/)) & 0xf;
    }

    speed_config = &(pCfg->speed_config);
    speed_config->port_refclk_int  = port_refclk_int;
    speed_config->speed  = soc_property_port_get(unit, port, spn_PORT_INIT_SPEED, 0);
    speed_config->port_num_lanes   = phy_num_lanes;
    speed_config->port_is_higig    = port_is_higig;
    speed_config->rxaui_mode       = soc_property_port_get(unit, port, spn_SERDES_RXAUI_MODE, 1);
    speed_config->scrambler_en     = soc_property_port_get(unit, port, spn_SERDES_SCRAMBLER_ENABLE, 0);
    speed_config->line_interface   = soc_property_port_get(unit, port, spn_SERDES_IF_TYPE, 0);
    speed_config->fiber_pref = soc_property_port_get(unit, port, spn_SERDES_FIBER_PREF, fiber_pref);
    speed_config->pcs_bypass       = 0;
    speed_config->phy_supports_dual_rate = phy_supports_dual_rate;

    SOC_IF_ERROR_RETURN
        (qsgmiie_speed_to_interface_config_get(speed_config, &(core_init_config->interface)));

    /* 
        PHY configuration
    */

    /*set the default tx parameters */
    p_tx = &pCfg->tx_drive[TXDRV_DFT_INX];
    p_tx->amp = 0xc;
    p_tx->pre = 0x00;
    p_tx->main = 0x70;
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
    p_tx->pre = 0x00;
    p_tx->main = 0x70;
    p_tx->post = 0x00;
    p_tx->post2 = 0x00;
    p_tx->post3 = 0x00;

    phymod_phy_init_config_t_init(pm_phy_init_config);
    /*initialize the tx taps and driver current*/
    for (i = 0; i < 4; i++) { 
        pm_phy_init_config->tx[i].pre = 0x00;
        pm_phy_init_config->tx[i].main = 0x70;
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
                                    "lane", QSGMIIE_NO_CFG_VALUE);
        if (preemphasis != QSGMIIE_NO_CFG_VALUE) {
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

    pCfg->cl73an  = soc_property_port_get(unit, port,
                                          spn_PHY_AN_C73, pCfg->cl73an); /* no L: C73, not CL73 */

    pCfg->cl37an = soc_property_port_get(unit, port,
                                        spn_PHY_AN_C37, pCfg->cl37an); /* no L: C37, not CL37 */

    pCfg->an_cl72 = soc_property_port_get(unit, port,
                                        spn_PHY_AN_C72, pCfg->an_cl72);
    pCfg->forced_init_cl72 = soc_property_port_get(unit, port,
                                        spn_PORT_INIT_CL72, pCfg->forced_init_cl72);

    
    /* 
        phy_ctrl_t configuration (LOGICAL PORT BASED)
        Only do this once, for the first core of the logical port
    */
    if (core_num == 0) {
        
        /* pc->lane_num, pc->chip_num */
#if 0
        soc_port_info = &(SOC_DRIVER(unit)->port_info[phy_port]);
/* mark it for the assertion of lane_num and chip_num is assigned */
        pc->lane_num = soc_port_info->bindex;
        pc->chip_num = SOC_BLOCK_NUMBER(unit, soc_port_info->blk);
#endif

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
        
        pCfg->forced_init_cl72 = soc_property_port_get(unit, port, spn_PORT_INIT_CL72, FALSE);
        /* for forced speed mode */
        if((pCfg->forced_init_cl72>0)&&(pCfg->forced_init_cl72<16)) 
           FORCE_CL72_MODE(pc)    = 1 ;
        else                         
           FORCE_CL72_MODE(pc)    = 0 ;

        FORCE_CL72_ENABLED(pc) = 0;
        FORCE_CL72_STATE(pc)   = TSCMOD_FORCE_CL72_IDLE;
        FORCE_CL72_RESTART_CNT(pc) = 0;
#endif
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

/*
 * Function:
 *      phy_qsgmiie_init
 *  
 *      An SDK "phy" is a logical port, which can consist of from 1-10 lanes,
 *          (A phy with more than 4 lanes requires more than one core).
 *      Per-logical port information is saved in (qsgmiie_cfg_t *) &pc->driver_data.
 *      An SDK phy is implemented as one or more PHYMOD "phy"s.
 *  
 *      A PHYMOD "phy" resides completely within a single core, which can be
 *      from 1 to 4 lanes.
 *      Per-phymod phy information is kept in (soc_phymod_ctrl_t) *pc->phymod_ctrl
 *      A phymod phy points to its core.  Up to 4 phymod phys can be on a single core
 *  
 * Purpose:     
 *      Initialize a qsgmiie phy
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 
 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
phy_qsgmiie_init(int unit, soc_port_t port)
{
    phy_ctrl_t                *pc; 
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy = NULL;
    soc_phymod_core_t         *core;
    qsgmiie_config_t             *pCfg;
    qsgmiie_speed_config_t       *speed_config;
    qsgmiie_dev_info_t           *pInfo;
    soc_phy_info_t            *pi;
    phymod_phy_inf_config_t   interface_config;
    phymod_core_status_t      core_status;
    phymod_core_info_t        core_info;
    int idx;
    int logical_lane_offset;

    DBG_OK();

    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }
    pc->driver_data = (void*)(pc+1);
    pmc = &pc->phymod_ctrl;
    pCfg = (qsgmiie_config_t *) pc->driver_data;
    pInfo = &pCfg->info; 
    
    sal_memset(pCfg, 0, sizeof(*pCfg));
    speed_config = &(pCfg->speed_config);

    /* Loop through all phymod phys that support this SDK phy */
    logical_lane_offset = 0;
    for (idx = 0; idx < pmc->num_phys; idx++) {
        phy = pmc->phy[idx];
        core = phy->core;

        

        /* determine configuration data structure to default values, based on SOC properties */
        SOC_IF_ERROR_RETURN
            (qsgmiie_config_init(unit, port,
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

    /*fill up the pInfo table for displaying the serdes driver info */
    SOC_IF_ERROR_RETURN
        (qsgmiie_show_serdes_info(pc, pInfo, &core_info));

    /* retrieve chip level information for serdes driver info */
    pi = &SOC_PHY_INFO(unit, port);

    if (!PHY_EXTERNAL_MODE(unit, port)) {
        pi->phy_name = pInfo->name;
    }

    /* set the port to its max or init_speed */
    SOC_IF_ERROR_RETURN
        (qsgmiie_speed_to_interface_config_get(speed_config, &interface_config));
    SOC_IF_ERROR_RETURN
        (phymod_phy_interface_config_set(&phy->pm_phy,
                                         0 /* flags */, &interface_config));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_qsgmiie_link_get
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
phy_qsgmiie_link_get(int unit, soc_port_t port, int *link)
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
 *      phy_qsgmiie_enable_set
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
phy_qsgmiie_enable_set(int unit, soc_port_t port, int enable)
{
    phy_ctrl_t                *pc;
    soc_phymod_ctrl_t         *pmc;
    phymod_phy_access_t       *pm_phy;
    phymod_phy_power_t        phy_power;
    int idx;

    DBG_OK();

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
 *      phy_qsgmiie_enable_get
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
phy_qsgmiie_enable_get(int unit, soc_port_t port, int *enable)
{
    phy_ctrl_t                *pc;
    soc_phymod_ctrl_t         *pmc;
    phymod_phy_access_t       *pm_phy;
    phymod_phy_power_t        phy_power;

    DBG_OK();

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
 *      phy_qsgmiie_duplex_get
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
phy_qsgmiie_duplex_get(int unit, soc_port_t port, int *duplex)
{
    *duplex = TRUE;
    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_qsgmiie_speed_set
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
phy_qsgmiie_speed_set(int unit, soc_port_t port, int speed)
{
    phy_ctrl_t                *pc;
    qsgmiie_config_t             *pCfg;
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy;
    int                       idx;
    qsgmiie_speed_config_t       speed_config;
    phymod_phy_inf_config_t   interface_config;

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    DBG_OK();

    pmc = &pc->phymod_ctrl;
    pCfg = (qsgmiie_config_t *) pc->driver_data;

    /* take a copy of core and phy configuration values, and set the desired speed */
    sal_memcpy(&speed_config, &pCfg->speed_config, sizeof(speed_config));
    speed_config.speed = speed;

    /* determine the interface configuration */
    SOC_IF_ERROR_RETURN
        (qsgmiie_speed_to_interface_config_get(&speed_config, &interface_config));

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
 *      phy_qsgmiie_speed_get
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
phy_qsgmiie_speed_get(int unit, soc_port_t port, int *speed)
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

    DBG_OK();

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
 *      phy_qsgmiie_an_set
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
phy_qsgmiie_an_set(int unit, soc_port_t port, int enable)
{
    phy_ctrl_t                *pc;
    qsgmiie_config_t             *pCfg;
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy;
    phymod_autoneg_control_t  an;
    soc_info_t *si;

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    DBG_OK();

    phymod_autoneg_control_t_init(&an);
    pmc = &pc->phymod_ctrl;
    pCfg = (qsgmiie_config_t *) pc->driver_data;
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
 *      phy_qsgmiie_an_get
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
phy_qsgmiie_an_get(int unit, soc_port_t port, int *an, int *an_done)
{
    return (SOC_E_NONE);
}

/*
 * Function:
 *      phy_qsgmiie_ability_advert_set
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
phy_qsgmiie_ability_advert_set(int unit, soc_port_t port,
                          soc_port_ability_t *ability)
{
    phy_ctrl_t                *pc;
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy;
    phymod_autoneg_ability_t  phymod_autoneg_ability; 

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    phymod_autoneg_ability_t_init(&phymod_autoneg_ability);

    pmc = &pc->phymod_ctrl;

    /* only set abilities on the first core */
    phy = pmc->phy[0];
    if (phy == NULL) {
        return SOC_E_INTERNAL;
    }

    

    switch (ability->pause & (SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX)) {
    case SOC_PA_PAUSE_TX:
        PHYMOD_AN_CAPABILITIES_ASYM_PAUSE_SET(&phymod_autoneg_ability);
        break;
    case SOC_PA_PAUSE_RX:
        /*an_adv |= MII_ANA_C37_PAUSE | MII_ANA_C37_ASYM_PAUSE; */
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
    }

    SOC_IF_ERROR_RETURN
        (phymod_phy_autoneg_ability_set(&phy->pm_phy, &phymod_autoneg_ability));

    return SOC_E_NONE;
}

/*
 * Function:
 *      phy_qsgmiie_ability_advert_get
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
phy_qsgmiie_ability_advert_get(int unit, soc_port_t port,
                              soc_port_ability_t *ability)
{
    phy_ctrl_t                *pc;
    soc_phymod_ctrl_t         *pmc;
    soc_phymod_phy_t          *phy;
    int                       reg37_ability;
    int                       reg73_ability;
    int                       reg_ability;
    _shr_port_mode_t          speed_full_duplex;

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    DBG_OK();

    pmc = &pc->phymod_ctrl;

    /* only get abilities from the first core */
    phy = pmc->phy[0];
    if (phy == NULL) {
        return SOC_E_INTERNAL;
    }

    speed_full_duplex = 0;

    
    reg73_ability = 0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_40G_CR4(reg73_ability) ?SOC_PA_SPEED_40GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_40G_KR4(reg73_ability) ?SOC_PA_SPEED_40GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_10G_KR(reg73_ability)  ?SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_10G_KX4(reg73_ability) ?SOC_PA_SPEED_10GB:0;
    speed_full_duplex|= PHYMOD_AN_TECH_ABILITY_IS_1G_KX(reg73_ability)   ?SOC_PA_SPEED_1000MB:0;

    
    reg37_ability = 0;
    speed_full_duplex|= PHYMOD_BAM_CL37_ABL_IS_40G(reg37_ability) ? SOC_PA_SPEED_40GB:0;
    /* speed_full_duplex|= (reg37_ability &(1<<PHYMOD_BAM37ABL_32P7G))?      SOC_PA_SPEED_33GB:0; */
    /* speed_full_duplex|= (reg37_ability &(1<<PHYMOD_BAM37ABL_31P5G))?      SOC_PA_SPEED_32GB:0; */
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

    
    reg_ability = 0 & (PHYMOD_AN_CAPABILITIES_SYMM_PAUSE|PHYMOD_AN_CAPABILITIES_ASYM_PAUSE);

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
 *      phy_qsgmiie_ability_remote_get
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
phy_qsgmiie_ability_remote_get(int unit, soc_port_t port,
                               soc_port_ability_t *ability)
{
   /*
   phy_ctrl_t        *pc;
   TSCMOD_DEV_DESC_t *pDesc;
   tscmod_st         *tsc;
   int cl37_reg, cl73_reg, sgmii_speed ;  uint32 reg73_ability, reg37_ability, reg_ability; soc_port_mode_t  ability_mode;
   int rv ; 
   */
#if 0

   pc    = INT_PHY_SW_STATE(unit, port);
   pDesc = (TSCMOD_DEV_DESC_t *)(pc + 1);
   tsc   =  (tscmod_st *)( pDesc + 1);

   tscmod_sema_lock(unit, port, __func__) ;

   TSCMOD_MEM_SET(ability, 0, sizeof(*ability));

   cl73_reg = 0 ; cl37_reg = 0 ;  sgmii_speed =0 ;  ability_mode = 0 ;  reg73_ability = 0 ; reg37_ability = 0  ;
   if((tsc->an_type==TSCMOD_CL73)||(tsc->an_type==TSCMOD_CL73_BAM)) {
      cl73_reg = 1 ;
   } else if(tsc->an_type==TSCMOD_CL37_BAM) {
      cl37_reg = 1 ;
   } else {
      cl37_reg = 1 ; cl73_reg = 1 ;
   }

   if(cl73_reg) {
      tsc->per_lane_control=TSCMOD_AN_GET_LP_CL73_ABIL ;
      tscmod_tier1_selector("AUTONEG_GET", tsc, &rv);
      reg73_ability = tsc->accData ;
      ability_mode|= (reg73_ability &(1<<TSCMOD_ABILITY_40G_CR4)) ?SOC_PA_SPEED_40GB:0;
      ability_mode|= (reg73_ability &(1<<TSCMOD_ABILITY_40G_KR4)) ?SOC_PA_SPEED_40GB:0;
      ability_mode|= (reg73_ability &(1<<TSCMOD_ABILITY_20G_CR2)) ?SOC_PA_SPEED_20GB:0;
      ability_mode|= (reg73_ability &(1<<TSCMOD_ABILITY_20G_KR2)) ?SOC_PA_SPEED_20GB:0;
      ability_mode|= (reg73_ability &(1<<TSCMOD_ABILITY_10G_KR))  ?SOC_PA_SPEED_10GB:0;
      ability_mode|= (reg73_ability &(1<<TSCMOD_ABILITY_10G_KX4)) ?SOC_PA_SPEED_10GB:0;
      ability_mode|= (reg73_ability &(1<<TSCMOD_ABILITY_1G_KX))   ?SOC_PA_SPEED_1000MB:0;
   }
   if(cl37_reg) {
      tsc->per_lane_control=TSCMOD_AN_GET_LP_CL37_ABIL ;
      tscmod_tier1_selector("AUTONEG_GET", tsc, &rv);
      reg37_ability = tsc->accData ;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_40G))?        SOC_PA_SPEED_40GB:0;
      /* ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_32P7G))?      SOC_PA_SPEED_33GB:0; */
      /* ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_31P5G))?      SOC_PA_SPEED_32GB:0; */
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_25P455G))?    SOC_PA_SPEED_25GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_21G_X4))?     SOC_PA_SPEED_21GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_20G_X2_CX4))? SOC_PA_SPEED_20GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_20G_X2))?     SOC_PA_SPEED_20GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_20G_X4))?     SOC_PA_SPEED_20GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_20G_X4_CX4))? SOC_PA_SPEED_20GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_16G_X4))?     SOC_PA_SPEED_16GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_15P75G_R2))?  SOC_PA_SPEED_16GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_15G_X4))?     SOC_PA_SPEED_15GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_13G_X4))?     SOC_PA_SPEED_13GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_12P7_DXGXS))? SOC_PA_SPEED_13GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_12P5_X4))?    SOC_PA_SPEED_12P5GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_12G_X4))?     SOC_PA_SPEED_12GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_10P5G_DXGXS))?SOC_PA_SPEED_11GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_10G_X2_CX4))? SOC_PA_SPEED_10GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_10G_DXGXS))?  SOC_PA_SPEED_10GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_10G_CX4))?    SOC_PA_SPEED_10GB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_10G_HIGIG))?  SOC_PA_SPEED_10GB:0; /* 4-lane */
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_6G_X4))?      SOC_PA_SPEED_6000MB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_5G_X4))?      SOC_PA_SPEED_5000MB:0;
      ability_mode|= (reg37_ability &(1<<TSCMOD_BAM37ABL_2P5G))?       SOC_PA_SPEED_2500MB:0;
   }

   ability->speed_full_duplex = ability_mode ;

   tsc->per_lane_control=TSCMOD_AN_GET_LP_MISC_ABIL ;
   tscmod_tier1_selector("AUTONEG_GET", tsc, &rv);
   reg_ability = tsc->accData ;
   switch(reg_ability & (TSCMOD_SYMM_PAUSE|TSCMOD_ASYM_PAUSE)) {
   case TSCMOD_SYMM_PAUSE:
      ability->pause = SOC_PA_PAUSE_TX | SOC_PA_PAUSE_RX;
      break ;
   case TSCMOD_ASYM_PAUSE:
      ability->pause = SOC_PA_PAUSE_TX;
      break ;
   case TSCMOD_SYMM_PAUSE|TSCMOD_ASYM_PAUSE:
      ability->pause = SOC_PA_PAUSE_RX;
      break ;
   }

   if(tsc->an_type==TSCMOD_CL37_SGMII) {
      tsc->per_lane_control=TSCMOD_AN_GET_LP_SGMII_ABIL ;
      tscmod_tier1_selector("AUTONEG_GET", tsc, &rv);
      reg_ability = tsc->accData ;

      sgmii_speed = reg_ability & 0x3 ;
   }

   if(tsc->verbosity&TSCMOD_DBG_AN) {
      printf("%-22s u=%0d p=%0d full_duplex REMOTE ability %s(=%0x) pause=%x sgmii_speed=%0d\n",
             __func__, unit, port, tscmod_ability_msg0(ability->speed_full_duplex),
             ability->speed_full_duplex, ability->pause, sgmii_speed);
   }

   tscmod_sema_unlock(unit, port) ;
#endif

   return SOC_E_NONE;
}

/*
 * Function:
 *      phy_qsgmiie_lb_set
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
phy_qsgmiie_lb_set(int unit, soc_port_t port, int enable)
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
            (phymod_phy_loopback_set(pm_phy, phymodLoopbackGlobalPMD, enable)); 
    }

    return (SOC_E_NONE);
}

/*
 * Function:
 *      phy_qsgmiie_lb_get
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
phy_qsgmiie_lb_get(int unit, soc_port_t port, int *enable)
{
    phy_ctrl_t* pc; 
    soc_phymod_ctrl_t *pmc;
    phymod_phy_access_t *pm_phy;
    uint32 lb_enable;
    int idx;

    pc = INT_PHY_SW_STATE(unit, port);
    pmc = &pc->phymod_ctrl;

    *enable = 0;

    DBG_OK();

    for (idx = 0; idx < pmc->num_phys; idx++) {
        pm_phy = &pmc->phy[idx]->pm_phy;
        SOC_IF_ERROR_RETURN
            (phymod_phy_loopback_get(pm_phy, phymodLoopbackGlobalPMD, &lb_enable));
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
 *      phy_qsgmiie_ability_local_get
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
phy_qsgmiie_ability_local_get(int unit, soc_port_t port, soc_port_ability_t *ability)
{
    phy_ctrl_t          *pc; 
    qsgmiie_config_t    *pCfg;

    pc = INT_PHY_SW_STATE(unit, port);
    pCfg = (qsgmiie_config_t *) pc->driver_data;

    if (NULL == ability) {
        return SOC_E_PARAM;
    }

    sal_memset(ability, 0, sizeof(*ability));

    ability->loopback  = SOC_PA_LB_PHY;
    ability->medium    = SOC_PA_MEDIUM_FIBER;
    ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
    ability->flags     = 0;

    ability->speed_full_duplex  = SOC_PA_SPEED_1000MB;
    if (pCfg->fiber_pref)   {
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

    ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
    ability->interface = SOC_PA_INTF_GMII | SOC_PA_INTF_SGMII;
    ability->medium    = SOC_PA_MEDIUM_FIBER;
    ability->loopback  = SOC_PA_LB_PHY;

    LOG_INFO(BSL_LS_SOC_PHY,
             (BSL_META_U(pc->unit,
                         "phy_qsgmiie_ability_local_get:unit=%d p=%d sp=%08x\n"),
              unit, port, ability->speed_full_duplex));
    return (SOC_E_NONE);
}

/*
 * Function:
 *      qsgmiie_uc_status_dump
 * Purpose:
 *      display all the serdes related parameters
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 

 * Returns:     
 *      SOC_E_NONE
 */
STATIC int
qsgmiie_uc_status_dump(int unit, soc_port_t port)
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
            (phymod_phy_status_dump(pm_phy)); 
    }
    return (SOC_E_NONE);
}

/* 
 * given a pc (phymod_ctrl_t) and logical lane number, 
 *    find the correct soc_phymod_phy_t object and lane
 */
STATIC int
_qsgmiie_find_soc_phy_lane(soc_phymod_ctrl_t *pmc, uint32_t lane, 
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
 * qsgmiie_per_lane_preemphasis_set
 */
STATIC int
qsgmiie_per_lane_preemphasis_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_tx_t         phymod_tx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_qsgmiie_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

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
 * qsgmiie_preemphasis_set
 */
STATIC int
qsgmiie_preemphasis_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_tx_fir_pre_set
 */
STATIC int
qsgmiie_tx_fir_pre_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_tx_fir_main_set
 */
STATIC int
qsgmiie_tx_fir_main_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_tx_fir_post_set
 */
STATIC int
qsgmiie_tx_fir_post_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_tx_fir_post2_set
 */
STATIC int
qsgmiie_tx_fir_post2_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_tx_fir_post3_set
 */
STATIC int
qsgmiie_tx_fir_post3_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_per_lane_driver_current_set
 */
STATIC int
qsgmiie_per_lane_driver_current_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_tx_t         phymod_tx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_qsgmiie_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

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
 * qsgmiie_driver_current_set
 */
STATIC int
qsgmiie_driver_current_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_per_lane_rx_dfe_tap_control_set
 */
STATIC int
qsgmiie_per_lane_rx_dfe_tap_control_set(soc_phymod_ctrl_t *pmc, int lane, int tap, int enable, uint32 value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t          phymod_rx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_qsgmiie_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

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
 * qsgmiie_tx_lane_squelch
 */
STATIC int
qsgmiie_tx_lane_squelch(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_per_lane_rx_peak_filter_set
 */
STATIC int
qsgmiie_per_lane_rx_peak_filter_set(soc_phymod_ctrl_t *pmc, int lane, int enable, uint32 value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t         phymod_rx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_qsgmiie_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

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
 * qsgmiie_rx_peak_filter_set
 */
STATIC int 
qsgmiie_rx_peak_filter_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_per_lane_rx_vga_set
 */
STATIC int
qsgmiie_per_lane_rx_vga_set(soc_phymod_ctrl_t *pmc, int lane, int enable, uint32 value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t         phymod_rx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_qsgmiie_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

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
 * qsgmiie_rx_vga_set
 */
STATIC int 
qsgmiie_rx_vga_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
qsgmiie_rx_tap_release(soc_phymod_ctrl_t *pmc, int tap)
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
 * qsgmiie_rx_tap_set
 */
STATIC int 
qsgmiie_rx_tap_set(soc_phymod_ctrl_t *pmc, int tap, uint32 value)
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
 * qsgmiie_pi_control_set
 */
STATIC int 
qsgmiie_pi_control_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_tx_polarity_set
 */
STATIC int 
qsgmiie_tx_polarity_set(soc_phymod_ctrl_t *pmc, phymod_polarity_t *cfg_polarity, uint32 value)
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
 * qsgmiie_rx_polarity_set
 */

STATIC int 
qsgmiie_rx_polarity_set(soc_phymod_ctrl_t *pmc, phymod_polarity_t *cfg_polarity, uint32 value)
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
 * qsgmiie_rx_reset
 */
STATIC int
qsgmiie_rx_reset(soc_phymod_ctrl_t *pmc, phymod_phy_reset_t *cfg_reset, uint32 value)
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
 * qsgmiie_tx_reset
 */
STATIC int
qsgmiie_tx_reset(soc_phymod_ctrl_t *pmc, phymod_phy_reset_t *cfg_reset, uint32 value)
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
 * qsgmiie_cl72_enable_set
 */
STATIC int
qsgmiie_cl72_enable_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
qsgmiie_lane_map_set(soc_phymod_ctrl_t *pmc, uint32 value){
    int idx;
    phymod_lane_map_t lane_map;

    lane_map.num_of_lanes = NUM_LANES;
    if(pmc->phy[0] == NULL){
        return(SOC_E_INTERNAL);
    }
    for (idx=0; idx < NUM_LANES; idx++) {
        lane_map.lane_map_rx[idx] = (value >> (idx * 4 /*4 bit per lane*/)) & 0xf;
    }
    for (idx=0; idx < NUM_LANES; idx++) {
        lane_map.lane_map_tx[idx] = (value >> (16 + idx * 4 /*4 bit per lane*/)) & 0xf;
    }
    SOC_IF_ERROR_RETURN(phymod_core_lane_map_set(&pmc->phy[0]->core->pm_core, &lane_map));
    return(SOC_E_NONE);
}


STATIC int
qsgmiie_lane_map_get(soc_phymod_ctrl_t *pmc, uint32 *value){
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
        value += (lane_map.lane_map_rx[idx]<< (idx * 4 + 16));
    }
    
    return(SOC_E_NONE);
}


STATIC int
qsgmiie_pattern_len_set(soc_phymod_ctrl_t *pmc, uint32_t value)
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
qsgmiie_pattern_enable_set(soc_phymod_ctrl_t *pmc, uint32_t value)
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
 * qsgmiie_pattern_data_set 
 *    Sets 32 bits of the 256 bit data pattern.
 */
STATIC int
qsgmiie_pattern_data_set(int idx, qsgmiie_pattern_t *pattern, uint32 value)
{
    if ((idx<0) || (idx >= COUNTOF(pattern->pattern_data))) {
        return SOC_E_INTERNAL;
    }

    /* update pattern data */
    pattern->pattern_data[idx] = value;

    return(SOC_E_NONE);
}

/* 
 * qsgmiie_per_lane_prbs_poly_set
 */
STATIC int
qsgmiie_per_lane_prbs_poly_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
   return(SOC_E_UNAVAIL);
}


STATIC
int
qsgmiie_sdk_poly_to_phymod_poly(uint32 sdk_poly, uint32 *phymod_poly){
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
    case 6:
        *phymod_poly = phymodPrbsPoly58;
        break;
    default:
        return SOC_E_INTERNAL;
    }
    return SOC_E_NONE;
}

/* 
 * qsgmiie_prbs_tx_poly_set
 */
STATIC int
qsgmiie_prbs_tx_poly_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
    SOC_IF_ERROR_RETURN(qsgmiie_sdk_poly_to_phymod_poly(value, &prbs.poly));
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_set(pm_phy, flags, &prbs));
    return(SOC_E_NONE);
}

/* 
 * qsgmiie_prbs_rx_poly_set
 */
STATIC int
qsgmiie_prbs_rx_poly_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
    SOC_IF_ERROR_RETURN(qsgmiie_sdk_poly_to_phymod_poly(value, &prbs.poly));
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_set(pm_phy, flags, &prbs));
    return(SOC_E_NONE);
}


/* 
 * qsgmiie_per_lane_prbs_tx_invert_data_set
 */
STATIC int
qsgmiie_per_lane_prbs_tx_invert_data_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_prbs_tx_invert_data_set
 */
STATIC int
qsgmiie_prbs_tx_invert_data_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_set(pm_phy, flags, &prbs));
    return(SOC_E_NONE);
}

/* 
 * qsgmiie_prbs_rx_invert_data_set
 */
STATIC int
qsgmiie_prbs_rx_invert_data_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
    prbs.invert = value;
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_config_set(pm_phy, flags,  &prbs));
    return(SOC_E_NONE);
}

/* 
 * qsgmiie_per_lane_prbs_tx_enable_set
 */
STATIC int
qsgmiie_per_lane_prbs_tx_enable_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
   return(SOC_E_UNAVAIL);
}
/* 
 * qsgmiie_prbs_tx_enable_set
 */
STATIC int
qsgmiie_prbs_tx_enable_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_prbs_rx_enable_set
 */
STATIC int
qsgmiie_prbs_rx_enable_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
    SOC_IF_ERROR_RETURN(phymod_phy_prbs_enable_set(pm_phy, flags, value));
    return(SOC_E_NONE);
}

/* 
 * qsgmiie_loopback_internal_set
 */
STATIC int 
qsgmiie_loopback_internal_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_loopback_internal_pmd_set
 */
STATIC int 
qsgmiie_loopback_internal_pmd_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_loopback_remote_set
 */
STATIC int 
qsgmiie_loopback_remote_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_loopback_remote_pcs_set
 */
STATIC int 
qsgmiie_loopback_remote_pcs_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_fec_enable_set
 */
STATIC int 
qsgmiie_fec_enable_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_scrambler_set
 */
STATIC int
qsgmiie_scrambler_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_8b10b_set
 */
STATIC int
qsgmiie_8b10b_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
In discussion: whether encoding API should be separate
Setting interface and speed:
typedef struct phymod_phy_inf_config_s {
    phymod_interface_t interface_type;          
    uint32                      data_rate;          /*Use PHYMOD_DEFAULT_RATE of interface default*/
    uint32                      interface_modes;    
} phymod_phy_inf_config_t;

#define PHYMOD_DEFAULT_RATE 0xFFFFFFFF

int phymod_phy_interface_config_set(int unit, uint32 phy_id, uint32 flags, const phymod_phy_inf_config_t* config);
int phymod_phy_interface_config_get(int unit, uint32 phy_id, uint32 flags, phymod_phy_inf_config_t* config);
#endif

   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_64b65b_set
 */
STATIC int
qsgmiie_64b65b_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
In discussion: whether encoding API should be separate
Setting interface and speed:
typedef struct phymod_phy_inf_config_s {
    phymod_interface_t interface_type;          
    uint32                      data_rate;          /*Use PHYMOD_DEFAULT_RATE of interface default*/
    uint32                      interface_modes;    
} phymod_phy_inf_config_t;

#define PHYMOD_DEFAULT_RATE 0xFFFFFFFF

int phymod_phy_interface_config_set(int unit, uint32 phy_id, uint32 flags, const phymod_phy_inf_config_t* config);
int phymod_phy_interface_config_get(int unit, uint32 phy_id, uint32 flags, phymod_phy_inf_config_t* config);
#endif

   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_power_set
 */
STATIC int
qsgmiie_power_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_per_lane_rx_low_freq_filter_set
 */
STATIC int
qsgmiie_per_lane_rx_low_freq_filter_set(soc_phymod_ctrl_t *pmc, int lane, uint32 value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_rx_low_freq_filter_set
 */
STATIC int
qsgmiie_rx_low_freq_filter_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_tx_ppm_adjust_set
 */
STATIC int
qsgmiie_tx_ppm_adjust_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_vco_freq_set
 */
STATIC int
qsgmiie_vco_freq_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_pll_divider_set
 */
STATIC int
qsgmiie_pll_divider_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
phymod_phy_interface_config_set (?)
#endif

   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_oversample_mode_set
 */
STATIC int
qsgmiie_oversample_mode_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
phymod_phy_interface_config_set (?)
#endif

   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_ref_clk_set
 */
STATIC int
qsgmiie_ref_clk_set(soc_phymod_ctrl_t *pmc, uint32 value)
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
 * qsgmiie_rx_seq_toggle_set 
 *  
 */
STATIC int
qsgmiie_rx_seq_toggle_set(soc_phymod_ctrl_t *pmc)
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
 * qsgmiie_driver_supply_set
 */
STATIC int
qsgmiie_driver_supply_set(soc_phymod_ctrl_t *pmc, uint32 value)
{
#if 0
Can be added as init parameter.
Do you think set\get API are required?
#endif

   return(SOC_E_UNAVAIL);
}

/*
 * Function:
 *      phy_qsgmiie_control_set
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
phy_qsgmiie_control_set(int unit, soc_port_t port, soc_phy_control_t type, uint32 value)
{
    int                 rv;
    phy_ctrl_t          *pc;
    soc_phymod_ctrl_t   *pmc;
    qsgmiie_config_t       *pCfg;

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
    pCfg = (qsgmiie_config_t *) pc->driver_data;

    switch(type) {

    case SOC_PHY_CONTROL_TX_FIR_PRE:
        rv = qsgmiie_tx_fir_pre_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_MAIN:
        rv = qsgmiie_tx_fir_main_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST:
        rv = qsgmiie_tx_fir_post_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST2:
        rv = qsgmiie_tx_fir_post2_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST3:
        rv = qsgmiie_tx_fir_post3_set(pmc, value);
        break;
    /* PREEMPHASIS */
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE0:
        rv = qsgmiie_per_lane_preemphasis_set(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE1:
        rv = qsgmiie_per_lane_preemphasis_set(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE2:
        rv = qsgmiie_per_lane_preemphasis_set(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE3:
        rv = qsgmiie_per_lane_preemphasis_set(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS:
        rv = qsgmiie_preemphasis_set(pmc, value);
        break;

    /* DRIVER CURRENT */
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE0:
        rv = qsgmiie_per_lane_driver_current_set(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE1:
        rv = qsgmiie_per_lane_driver_current_set(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE2:
        rv = qsgmiie_per_lane_driver_current_set(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE3:
        rv = qsgmiie_per_lane_driver_current_set(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT:
        rv = qsgmiie_driver_current_set(pmc, value);
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
        rv = qsgmiie_tx_lane_squelch(pmc, value);
        break;

    /* RX PEAK FILTER */
    case SOC_PHY_CONTROL_RX_PEAK_FILTER:
        rv = qsgmiie_rx_peak_filter_set(pmc, value);
        break;

    /* RX VGA */
    case SOC_PHY_CONTROL_RX_VGA:
        rv = qsgmiie_rx_vga_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_RX_VGA_RELEASE:
       /* $$$ tbd $$$ */
       break;

    /* RX TAP */
    case SOC_PHY_CONTROL_RX_TAP1:
        rv = qsgmiie_rx_tap_set(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP2:
        rv = qsgmiie_rx_tap_set(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP3:
        rv = qsgmiie_rx_tap_set(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP4:
        rv = qsgmiie_rx_tap_set(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP5:
        rv = qsgmiie_rx_tap_set(pmc, 4, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP1_RELEASE:       /* $$$ tbd $$$ */
       rv = qsgmiie_rx_tap_release(pmc, 0);
       break;
    case SOC_PHY_CONTROL_RX_TAP2_RELEASE:       /* $$$ tbd $$$ */
       rv = qsgmiie_rx_tap_release(pmc, 1);
       break;
    case SOC_PHY_CONTROL_RX_TAP3_RELEASE:       /* $$$ tbd $$$ */
       rv = qsgmiie_rx_tap_release(pmc, 2);
       break;
    case SOC_PHY_CONTROL_RX_TAP4_RELEASE:       /* $$$ tbd $$$ */
       rv = qsgmiie_rx_tap_release(pmc, 3);
       break;
    case SOC_PHY_CONTROL_RX_TAP5_RELEASE:       /* $$$ tbd $$$ */
       rv = qsgmiie_rx_tap_release(pmc, 4);
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
        rv = qsgmiie_pi_control_set(pmc, value);
        break;

    /* POLARITY */
    case SOC_PHY_CONTROL_RX_POLARITY:
        rv = qsgmiie_rx_polarity_set(pmc, &pCfg->phy_polarity_config, value);;
        break;
    case SOC_PHY_CONTROL_TX_POLARITY:
        rv = qsgmiie_tx_polarity_set(pmc, &pCfg->phy_polarity_config, value);
        break;

    /* RESET */
    case SOC_PHY_CONTROL_RX_RESET:
        rv = qsgmiie_rx_reset(pmc, &pCfg->phy_reset_config, value);
        break;
    case SOC_PHY_CONTROL_TX_RESET:
        rv = qsgmiie_tx_reset(pmc, &pCfg->phy_reset_config, value);
        break;

    /* CL72 ENABLE */
    case SOC_PHY_CONTROL_CL72:
        rv = qsgmiie_cl72_enable_set(pmc, value);
        break;

    /* LANE SWAP */
    case SOC_PHY_CONTROL_LANE_SWAP:
        rv = qsgmiie_lane_map_set(pmc, value);
        break;

    /* TX PATTERN */
    case SOC_PHY_CONTROL_TX_PATTERN_20BIT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_256BIT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_LENGTH:
        rv = qsgmiie_pattern_len_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_GEN_ENABLE:
        rv = qsgmiie_pattern_enable_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA0:
        rv = qsgmiie_pattern_data_set(0, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA1:
        rv = qsgmiie_pattern_data_set(1, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA2:
        rv = qsgmiie_pattern_data_set(2, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA3:
        rv = qsgmiie_pattern_data_set(3, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA4:
        rv = qsgmiie_pattern_data_set(4, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA5:
        rv = qsgmiie_pattern_data_set(5, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA6:
        rv = qsgmiie_pattern_data_set(6, &pCfg->pattern, value);
        break;
    case SOC_PHY_CONTROL_TX_PATTERN_DATA7:
        rv = qsgmiie_pattern_data_set(7, &pCfg->pattern, value);
        break;

    /* decoupled PRBS */
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_POLYNOMIAL:
        rv = qsgmiie_prbs_tx_poly_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_INVERT_DATA:
        rv = qsgmiie_prbs_tx_invert_data_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_ENABLE:
        rv = qsgmiie_prbs_tx_enable_set(pmc, value);
        break; 
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_POLYNOMIAL:
        rv = qsgmiie_prbs_rx_poly_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_INVERT_DATA:
        rv = qsgmiie_prbs_rx_invert_data_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_ENABLE:
        rv = qsgmiie_prbs_rx_enable_set(pmc, value);
        break;
    /*for legacy prbs usage mainly set both tx/rx the same */
    case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
        rv = qsgmiie_prbs_tx_poly_set(pmc, value);
        rv = qsgmiie_prbs_rx_poly_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
        rv = qsgmiie_prbs_tx_invert_data_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
        rv = qsgmiie_prbs_tx_enable_set(pmc, value);
        rv = qsgmiie_prbs_rx_enable_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
        rv = qsgmiie_prbs_tx_enable_set(pmc, value);
        rv = qsgmiie_prbs_rx_enable_set(pmc, value);
        break;

    /* LOOPBACK */
    case SOC_PHY_CONTROL_LOOPBACK_INTERNAL:
        rv = qsgmiie_loopback_internal_set(pmc, value);
       break;
    case SOC_PHY_CONTROL_LOOPBACK_PMD:
        rv = qsgmiie_loopback_internal_pmd_set(pmc, value);
       break;
    case SOC_PHY_CONTROL_LOOPBACK_REMOTE:
        rv = qsgmiie_loopback_remote_set(pmc, value);
        break;
    case SOC_PHY_CONTROL_LOOPBACK_REMOTE_PCS_BYPASS:
        rv = qsgmiie_loopback_remote_pcs_set(pmc, value);
        break;

    /* FEC */
    case SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION:
        rv = qsgmiie_fec_enable_set(pmc, value);
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
        rv = qsgmiie_scrambler_set(pmc, value);
        break;

    /* 8B10B */
    case SOC_PHY_CONTROL_8B10B:
        rv = qsgmiie_8b10b_set(pmc, value);
        break;

    /* 64B65B */
    case SOC_PHY_CONTROL_64B66B:
        rv = qsgmiie_64b65b_set(pmc, value);
        break;

    /* POWER */
    case SOC_PHY_CONTROL_POWER:
       rv = qsgmiie_power_set(pmc, value);
       break;

    /* RX_LOW_FREQ_PEAK_FILTER */
    case SOC_PHY_CONTROL_RX_LOW_FREQ_PEAK_FILTER:
       rv = qsgmiie_rx_low_freq_filter_set(pmc, value);
       break;

    /* TX_PPM_ADJUST */
    case SOC_PHY_CONTROL_TX_PPM_ADJUST:
       rv = qsgmiie_tx_ppm_adjust_set(pmc, value);
       break;

    /* VCO_FREQ */
    case SOC_PHY_CONTROL_VCO_FREQ:
       rv = qsgmiie_vco_freq_set(pmc, value);
       break;

    /* PLL_DIVIDER */
    case SOC_PHY_CONTROL_PLL_DIVIDER:
       rv = qsgmiie_pll_divider_set(pmc, value);
       break;

    /* OVERSAMPLE_MODE */
    case SOC_PHY_CONTROL_OVERSAMPLE_MODE:
       rv = qsgmiie_oversample_mode_set(pmc, value);
       break;

    /* REF_CLK */
    case SOC_PHY_CONTROL_REF_CLK:
       rv = qsgmiie_ref_clk_set(pmc, value);
       break;

    /* RX_SEQ_TOGGLE */
    case SOC_PHY_CONTROL_RX_SEQ_TOGGLE:
       rv = qsgmiie_rx_seq_toggle_set(pmc);
       break;

    /* DRIVER_SUPPLY */
    case SOC_PHY_CONTROL_DRIVER_SUPPLY:
       rv = qsgmiie_driver_supply_set(pmc, value);
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
    case SOC_PHY_CONTROL_FEC_OFF:
        rv = qsgmiie_fec_enable_set(pmc, 0);
        break;
    case SOC_PHY_CONTROL_FEC_ON:
        rv = qsgmiie_fec_enable_set(pmc, 1);
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
    default:
        rv = SOC_E_UNAVAIL;
        break; 
    }
    return rv;
}


/* 
 * qsgmiie_tx_fir_pre_get
 */
STATIC int
qsgmiie_tx_fir_pre_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_tx_fir_main_get
 */
STATIC int
qsgmiie_tx_fir_main_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_tx_fir_post_get
 */
STATIC int
qsgmiie_tx_fir_post_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_tx_fir_post2_get
 */
STATIC int
qsgmiie_tx_fir_post2_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_tx_fir_post3_get
 */
STATIC int
qsgmiie_tx_fir_post3_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_per_lane_preemphasis_get
 */
STATIC int
qsgmiie_per_lane_preemphasis_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_tx_t         phymod_tx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_qsgmiie_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    SOC_IF_ERROR_RETURN(phymod_phy_tx_get(&pm_phy_copy, &phymod_tx));
    *value = phymod_tx.pre;

    return(SOC_E_NONE);
}

/* 
 * qsgmiie_per_lane_driver_current_get
 */
STATIC int
qsgmiie_per_lane_driver_current_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_tx_t         phymod_tx;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_qsgmiie_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

    /* Make a copy of the phy access and overwrite the desired lane */
    pm_phy = &p_phy->pm_phy;
    sal_memcpy(&pm_phy_copy, pm_phy, sizeof(pm_phy_copy));
    pm_phy_copy.access.lane = lane_map;

    SOC_IF_ERROR_RETURN(phymod_phy_tx_get(&pm_phy_copy, &phymod_tx));
    *value = phymod_tx.amp;

    return(SOC_E_NONE);
}

/* 
 * qsgmiie_per_lane_prbs_poly_get
 */
STATIC int
qsgmiie_per_lane_prbs_poly_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_per_lane_prbs_tx_invert_data_get
 */
STATIC int
qsgmiie_per_lane_prbs_tx_invert_data_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_per_lane_prbs_tx_enable_get
 */
STATIC int
qsgmiie_per_lane_prbs_tx_enable_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_per_lane_prbs_rx_enable_get
 */
STATIC int
qsgmiie_per_lane_prbs_rx_enable_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_per_lane_prbs_rx_status_get
 */
STATIC int
qsgmiie_per_lane_prbs_rx_status_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   /* $$$ Need to add diagnostic API */
   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_per_lane_rx_peak_filter_get
 */
STATIC int
qsgmiie_per_lane_rx_peak_filter_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t         phymod_rx;

    *value = 0;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_qsgmiie_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

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
 * qsgmiie_per_lane_rx_vga_get
 */
STATIC int
qsgmiie_per_lane_rx_vga_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t         phymod_rx;

    *value = 0;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_qsgmiie_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

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
 * qsgmiie_per_lane_rx_dfe_tap_control_get
 */
STATIC int
qsgmiie_per_lane_rx_dfe_tap_control_get(soc_phymod_ctrl_t *pmc, int lane, int tap, uint32 *value)
{
    soc_phymod_phy_t    *p_phy;
    uint32              lane_map;
    phymod_phy_access_t pm_phy_copy, *pm_phy;
    phymod_rx_t          phymod_rx;

    *value = 0;

    /* locate the desired phy and lane */
    SOC_IF_ERROR_RETURN(_qsgmiie_find_soc_phy_lane(pmc, lane, &p_phy, &lane_map));

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
 * qsgmiie_per_lane_rx_low_freq_filter_get
 */
STATIC int
qsgmiie_per_lane_rx_low_freq_filter_get(soc_phymod_ctrl_t *pmc, int lane, uint32 *value)
{
   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_rx_peak_filter_get
 */
STATIC int 
qsgmiie_rx_peak_filter_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_rx_vga_set
 */
STATIC int 
qsgmiie_rx_vga_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_rx_tap_get
 */
STATIC int 
qsgmiie_rx_tap_get(soc_phymod_ctrl_t *pmc, int tap, uint32 *value)
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
 * qsgmiie_pi_control_get
 */
STATIC int 
qsgmiie_pi_control_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_rx_signal_detect_get
 */
STATIC int
qsgmiie_rx_signal_detect_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
   /* $$$ Need to add diagnostic API */
   return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_rx_seq_done_get
 */
STATIC int
qsgmiie_rx_seq_done_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_rx_ppm_get
 */
STATIC int
qsgmiie_rx_ppm_get(soc_phymod_ctrl_t *pmc, uint32 *value)
{
    /* $$$ Need to add diagnostic API */
    return(SOC_E_UNAVAIL);
}

/* 
 * qsgmiie_cl72_enable_get
 */
STATIC int
qsgmiie_cl72_enable_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_cl72_status_get
 */
STATIC int
qsgmiie_cl72_status_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_prbs_tx_poly_get
 */
STATIC int
qsgmiie_prbs_tx_poly_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
    case phymodPrbsPoly58:
        *value = 6; 
        break;
    default:
        return SOC_E_INTERNAL;
    }
    return SOC_E_NONE;
}
/* 
 * qsgmiie_prbs_rx_poly_get
 */
STATIC int
qsgmiie_prbs_rx_poly_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
    case phymodPrbsPoly58:
        *value = 6; 
        break;
    default:
        return SOC_E_INTERNAL;
    }
    return SOC_E_NONE;
}

/* 
 * qsgmiie_prbs_tx_invert_data_get
 */
STATIC int
qsgmiie_prbs_tx_invert_data_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_prbs_rx_invert_data_get
 */
STATIC int
qsgmiie_prbs_rx_invert_data_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_prbs_tx_enable_get
 */
STATIC int
qsgmiie_prbs_tx_enable_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_prbs_rx_enable_get
 */
STATIC int
qsgmiie_prbs_rx_enable_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_prbs_rx_status_get
 */
STATIC int
qsgmiie_prbs_rx_status_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_loopback_internal_pmd_get (this is the PMD global loopback)
 */
STATIC int 
qsgmiie_loopback_internal_pmd_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_loopback_remote_get
 */
STATIC int 
qsgmiie_loopback_remote_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_loopback_remote_pcs_get
 */
STATIC int 
qsgmiie_loopback_remote_pcs_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_fec_get
 */
STATIC int 
qsgmiie_fec_enable_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * qsgmiie_pattern_256bit_get 
 *  
 * CHECK SEMANTICS: assumption is that this procedure will retrieve the pattern 
 * from Tier1 and return whether the tx pattern is enabled.  Note that this 
 * is different from the 20 bit pattern retrieval semantics 
 */
/* 
 * qsgmiie_pattern_256bit_get 
 *  
 * CHECK SEMANTICS: assumption is that this procedure will retrieve the pattern 
 * from Tier1 and return whether the tx pattern is enabled.  Note that this 
 * is different from the 20 bit pattern retrieval semantics 
 */
STATIC int
qsgmiie_pattern_get(soc_phymod_ctrl_t *pmc, qsgmiie_pattern_t* cfg_pattern)
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
qsgmiie_pattern_len_get(soc_phymod_ctrl_t *pmc, uint32_t *value)
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
 * qsgmiie_scrambler_get
 */
STATIC int
qsgmiie_scrambler_get(soc_phymod_ctrl_t *pmc, uint32 *value)
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
 * Function:
 *      phy_qsgmiie_control_get
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
phy_qsgmiie_control_get(int unit, soc_port_t port, soc_phy_control_t type, uint32 *value)
{
    int                 rv;
    phy_ctrl_t          *pc;
    soc_phymod_ctrl_t   *pmc;
    qsgmiie_config_t       *pCfg;

    if ((type < 0) || (type >= SOC_PHY_CONTROL_COUNT)) {
        return (SOC_E_PARAM);
    }

    /* locate phy control */
    pc = INT_PHY_SW_STATE(unit, port);
    if (pc == NULL) {
        return SOC_E_INTERNAL;
    }

    pmc = &pc->phymod_ctrl;
    pCfg = (qsgmiie_config_t *) pc->driver_data;

    switch(type) {

    /* PREEMPHASIS */
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE0:
        rv = qsgmiie_per_lane_preemphasis_get(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE1:
        rv = qsgmiie_per_lane_preemphasis_get(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE2:
        rv = qsgmiie_per_lane_preemphasis_get(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_PREEMPHASIS_LANE3:
        rv = qsgmiie_per_lane_preemphasis_get(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_PRE:
        /* assume they are all the same as lane 0 */
        rv = qsgmiie_tx_fir_pre_get(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_MAIN:
        /* assume they are all the same as lane 0 */
        rv = qsgmiie_tx_fir_main_get(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST:
        /* assume they are all the same as lane 0 */
        rv = qsgmiie_tx_fir_post_get(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST2:
        /* assume they are all the same as lane 0 */
        rv = qsgmiie_tx_fir_post2_get(pmc, value);
        break;
    case SOC_PHY_CONTROL_TX_FIR_POST3:
        /* assume they are all the same as lane 0 */
        rv = qsgmiie_tx_fir_post3_get(pmc, value);
        break;

    /* DRIVER CURRENT */
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE0:
        rv = qsgmiie_per_lane_driver_current_get(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE1:
        rv = qsgmiie_per_lane_driver_current_get(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE2:
        rv = qsgmiie_per_lane_driver_current_get(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT_LANE3:
        rv = qsgmiie_per_lane_driver_current_get(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_DRIVER_CURRENT:
        rv = qsgmiie_per_lane_driver_current_get(pmc, 0, value);
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
        rv = qsgmiie_rx_peak_filter_get(pmc, value);
        break;

    /* RX VGA */
    case SOC_PHY_CONTROL_RX_VGA:
        rv = qsgmiie_rx_vga_get(pmc, value);
        break;

    /* RX TAP */
    case SOC_PHY_CONTROL_RX_TAP1:
        rv = qsgmiie_rx_tap_get(pmc, 0, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP2:
        rv = qsgmiie_rx_tap_get(pmc, 1, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP3:
        rv = qsgmiie_rx_tap_get(pmc, 2, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP4:
        rv = qsgmiie_rx_tap_get(pmc, 3, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP5:
        rv = qsgmiie_rx_tap_get(pmc, 4, value);
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
        rv = qsgmiie_pi_control_get(pmc, value);
        break;

    /* RX SIGNAL DETECT */
    case SOC_PHY_CONTROL_RX_SIGNAL_DETECT:
      rv = qsgmiie_rx_signal_detect_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_RX_SEQ_DONE:
      rv = qsgmiie_rx_seq_done_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_RX_PPM:
      rv = qsgmiie_rx_ppm_get(pmc, value);
      break;

    /* CL72 */
    case SOC_PHY_CONTROL_CL72:
      rv = qsgmiie_cl72_enable_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_CL72_STATUS:
      rv = qsgmiie_cl72_status_get(pmc, value);
      break;

    /* PRBS */
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_POLYNOMIAL:
      rv = qsgmiie_prbs_tx_poly_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_INVERT_DATA:
      rv = qsgmiie_prbs_tx_invert_data_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_TX_ENABLE:
      rv = qsgmiie_prbs_tx_enable_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_POLYNOMIAL:
      rv = qsgmiie_prbs_rx_poly_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_INVERT_DATA:
      rv = qsgmiie_prbs_rx_invert_data_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_DECOUPLED_RX_ENABLE:
      rv = qsgmiie_prbs_rx_enable_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
      rv = qsgmiie_prbs_tx_poly_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
      rv = qsgmiie_prbs_tx_invert_data_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
      rv = qsgmiie_prbs_tx_enable_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
      rv = qsgmiie_prbs_rx_enable_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_PRBS_RX_STATUS:
      rv = qsgmiie_prbs_rx_status_get(pmc, value);
      break;

    /* LOOPBACK */
    case SOC_PHY_CONTROL_LOOPBACK_REMOTE:
       rv = qsgmiie_loopback_remote_get(pmc, value);
       break;
    case SOC_PHY_CONTROL_LOOPBACK_REMOTE_PCS_BYPASS:
       rv = qsgmiie_loopback_remote_pcs_get(pmc, value);
       break;
    case SOC_PHY_CONTROL_LOOPBACK_PMD:
       rv = qsgmiie_loopback_internal_pmd_get(pmc, value);
       break;

    /* FEC */
    case SOC_PHY_CONTROL_FORWARD_ERROR_CORRECTION:
      rv = qsgmiie_fec_enable_get(pmc, value);
      break;

    /* TX PATTERN */
    case SOC_PHY_CONTROL_TX_PATTERN_256BIT:
       rv = qsgmiie_pattern_get(pmc, &pCfg->pattern);
      break;
    case SOC_PHY_CONTROL_TX_PATTERN_LENGTH:
       rv = qsgmiie_pattern_len_get(pmc, value);
      break;
    case SOC_PHY_CONTROL_TX_PATTERN_20BIT:
       rv = qsgmiie_pattern_get(pmc, &pCfg->pattern);
      break;

    /* SCRAMBLER */
    case SOC_PHY_CONTROL_SCRAMBLER:
       rv = qsgmiie_scrambler_get(pmc, value);
       break;
    case SOC_PHY_CONTROL_LANE_SWAP:
        rv = qsgmiie_lane_map_get(pmc, value);
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
    default:
        rv = SOC_E_UNAVAIL;
        break; 
    }
    return rv;
}     

/*
 * Function:
 *      phy_qsgmiie_per_lane_control_set
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
phy_qsgmiie_per_lane_control_set(int unit, soc_port_t port, int lane, soc_phy_control_t type, uint32 value)
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
        rv = qsgmiie_per_lane_preemphasis_set(pmc, lane, value);
        break; 
    case SOC_PHY_CONTROL_DRIVER_CURRENT:
        rv = qsgmiie_per_lane_driver_current_set(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_DRIVER_POST2_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_RX_TAP1:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_set (pmc, lane, 0 /* tap */, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP1_RELEASE:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_set (pmc, lane, 0 /* tap */, 0 /* release */, 0x8000);
        break;
    case SOC_PHY_CONTROL_RX_TAP2:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_set (pmc, lane, 1 /* tap */, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP2_RELEASE:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_set (pmc, lane, 1 /* tap */, 0 /* release */, 0x8000);
        break;
    case SOC_PHY_CONTROL_RX_TAP3:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_set (pmc, lane, 2 /* tap */, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP3_RELEASE:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_set (pmc, lane, 2 /* tap */, 0 /* release */, 0x8000);
        break;
    case SOC_PHY_CONTROL_RX_TAP4:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_set (pmc, lane, 3 /* tap */, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP4_RELEASE:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_set (pmc, lane, 3 /* tap */, 0 /* release */, 0x8000);
        break;
    case SOC_PHY_CONTROL_RX_TAP5:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_set (pmc, lane, 4 /* tap */, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP5_RELEASE:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_set (pmc, lane, 4 /* tap */, 0 /* release */, 0x8000);
        break;

    case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
        rv = qsgmiie_per_lane_prbs_poly_set(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
        rv = qsgmiie_per_lane_prbs_tx_invert_data_set(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
        /* TX_ENABLE does both tx and rx */
        rv = qsgmiie_per_lane_prbs_tx_enable_set(pmc, lane, value);
        break; 
    case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
        rv = SOC_E_NONE;
        break;
    case SOC_PHY_CONTROL_RX_PEAK_FILTER:
        rv = qsgmiie_per_lane_rx_peak_filter_set(pmc, lane, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_LOW_FREQ_PEAK_FILTER:
        rv = qsgmiie_per_lane_rx_low_freq_filter_set(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_RX_VGA:
        rv = qsgmiie_per_lane_rx_vga_set(pmc, lane, 1 /* enable */, value);
        break;
    case SOC_PHY_CONTROL_RX_VGA_RELEASE:
        rv = qsgmiie_per_lane_rx_vga_set(pmc, lane, 0 /* release */, 0x8000);
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
 *      phy_qsgmiie_per_lane_control_get
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
phy_qsgmiie_per_lane_control_get(int unit, soc_port_t port, int lane,
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
        rv = qsgmiie_per_lane_preemphasis_get(pmc, lane, value);
        break; 
    case SOC_PHY_CONTROL_DRIVER_CURRENT:
        rv = qsgmiie_per_lane_driver_current_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRE_DRIVER_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_DRIVER_POST2_CURRENT:
        rv = SOC_E_UNAVAIL;
        break;
    case SOC_PHY_CONTROL_PRBS_POLYNOMIAL:
        rv = qsgmiie_per_lane_prbs_poly_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_INVERT_DATA:
        rv = qsgmiie_per_lane_prbs_tx_invert_data_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_TX_ENABLE:
        rv = qsgmiie_per_lane_prbs_tx_enable_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_RX_ENABLE:
        rv = qsgmiie_per_lane_prbs_rx_enable_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_PRBS_RX_STATUS:
        rv = qsgmiie_per_lane_prbs_rx_status_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_RX_PEAK_FILTER:
        rv = qsgmiie_per_lane_rx_peak_filter_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_RX_VGA:
        rv = qsgmiie_per_lane_rx_vga_get(pmc, lane, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP1:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_get(pmc, lane, 0, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP2:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_get(pmc, lane, 1, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP3:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_get(pmc, lane, 2, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP4:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_get(pmc, lane, 3, value);
        break;
    case SOC_PHY_CONTROL_RX_TAP5:
        rv = qsgmiie_per_lane_rx_dfe_tap_control_get(pmc, lane, 4, value);
        break;
    case SOC_PHY_CONTROL_RX_LOW_FREQ_PEAK_FILTER:
        rv = qsgmiie_per_lane_rx_low_freq_filter_get(pmc, lane, value);
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
 *      phy_qsgmiie_reg_read
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
phy_qsgmiie_reg_read(int unit, soc_port_t port, uint32 flags,
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
 *      phy_qsgmiie_reg_write
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
phy_qsgmiie_reg_write(int unit, soc_port_t port, uint32 flags,
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
 *      phy_qsgmiie_reg_modify
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
phy_qsgmiie_reg_modify(int unit, soc_port_t port, uint32 flags,
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

STATIC void
phy_qsgmiie_cleanup(soc_phymod_ctrl_t *pmc)
{
    int idx;
    soc_phymod_phy_t *phy;
    soc_phymod_core_t *core;

    for (idx = 0; idx < pmc->num_phys ; idx++) {
        phy = pmc->phy[idx];
        core = phy->core;

        /* Destroy core object if not used anymore */
        if (core && core->ref_cnt) {
            if (--core->ref_cnt == 0) {
                soc_phymod_core_destroy(pmc->unit, core);
            }
        }

        /* Destroy phy object */
        if (phy) {
            soc_phymod_phy_destroy(pmc->unit, phy);
        }
    }
    pmc->num_phys = 0;
}

STATIC void
phy_qsgmiie_core_init(phy_ctrl_t *pc, soc_phymod_core_t *core,
                   phymod_bus_t *core_bus, uint32 core_addr)
{
    phymod_core_access_t *pm_core;
    phymod_access_t *pm_acc;

    core->unit = pc->unit;
    core->read = pc->read;
    core->write = pc->write;
    core->wrmask = pc->wrmask;
    core->port = pc->port;

    pm_core = &core->pm_core;
    phymod_core_access_t_init(pm_core);
    pm_acc = &pm_core->access;
    phymod_access_t_init(pm_acc);
    PHYMOD_ACC_USER_ACC(pm_acc) = core;
    PHYMOD_ACC_BUS(pm_acc) = core_bus;
    PHYMOD_ACC_ADDR(pm_acc) = core_addr;

    if (soc_property_port_get(pc->unit, pc->port, "qsgmiie_sim", 0) == 45) {
        PHYMOD_PHYMOD_ACC_F_CLAUSE45_SET(pm_acc);
    }

    return;
}

/*
 * Function:
 *      phy_qsgmiie_probe
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
phy_qsgmiie_probe(int unit, phy_ctrl_t *pc)
{
    int rv, idx;
    uint32 lane_map, num_phys, core_id, phy_id, found;
    phymod_bus_t core_bus;
    phymod_dispatch_type_t phy_type;
    phymod_core_access_t *pm_core;
    phymod_phy_access_t *pm_phy;
    phymod_access_t *pm_acc;
    soc_phymod_ctrl_t *pmc;
    soc_phymod_phy_t *phy;
    soc_phymod_core_t *core;
    soc_phymod_core_t core_probe;
    soc_info_t *si;
    phyident_core_info_t core_info[8];  
#if 0  /* for QSGMII */
    int array_max = 8;
    int array_size = 0;
#endif
    int port;
    int phy_port;  /* physical port number */

    rv = 0;

    /* Initialize PHY bus */
    SOC_IF_ERROR_RETURN(phymod_bus_t_init(&core_bus));
    core_bus.bus_name = "qsgmiie_sim"; 
    core_bus.read = _qsgmiie_reg_read; 
    core_bus.write = _qsgmiie_reg_write;

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
    pmc->cleanup = phy_qsgmiie_cleanup;
    pmc->symbols = &bcmi_qsgmiie_serdes_symbols;

    
    if (SOC_IS_GREYHOUND(unit)) {
        int i, blk;
        blk = -1;
        for (i=0; i< SOC_DRIVER(unit)->port_num_blktype; i++){
            blk = SOC_PORT_IDX_BLOCK(unit, phy_port, i);
            if (PBMP_MEMBER(SOC_BLOCK_BITMAP(unit, blk), port)){
                break;
            }
        }
        if (blk == -1) {
            pc->chip_num = -1;
            return SOC_E_NOT_FOUND;
        } else {
            pc->chip_num = SOC_BLOCK_NUMBER(unit, blk);
        }
        /* current lane number for QSGMII-Eagle is 0-15 */
        pc->lane_num = (SOC_PORT_BINDEX(unit, phy_port) + 
                ((pc->chip_num > 0) ? 8 : 0)) % 16;
    } else {
        /* TBD for other chips */
    }

    /* request memory for the configuration structure */
    pc->size = sizeof(qsgmiie_config_t);

#if 1  /* for QSGMMII only6 */
    num_phys = 1;
    pc->phy_mode = PHYCTRL_QSGMII_CORE_PORT;
    lane_map = 1 << pc->lane_num;
    core_info[0].mdio_addr = pc->phy_id;
#else
    /* Bit N corresponds to lane N in lane_map */
    lane_map = 0xf;
    num_phys = 1;
    switch (si->port_num_lanes[port]) {
    case 10:
        num_phys = 3;
        break;
    case 4:
        break;
    case 2:
        lane_map = 0x3;
        break;
    case 1:
    case 0:
        lane_map = 0x1;
        pc->phy_mode = PHYCTRL_QSGMII_CORE_PORT;
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
#endif

    phy_type = phymodDispatchTypeQsgmiie;

    /* Probe cores */
    for (idx = 0; idx < num_phys ; idx++) {
        phy_qsgmiie_core_init(pc, &core_probe, &core_bus,
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
        }
        if (SOC_FAILURE(rv)) {
            break;
        }
    }
    if (SOC_FAILURE(rv)) {
        phy_qsgmiie_cleanup(pmc);
        return rv;
    }

    for (idx = 0; idx < pmc->num_phys ; idx++) {
        phy = pmc->phy[idx];
        core = phy->core;
        pm_core = &core->pm_core;

        /* Initialize core object if newly created */
        if (core->ref_cnt == 0) {
            sal_memcpy(&core->pm_bus, &core_bus, sizeof(core->pm_bus));
            phy_qsgmiie_core_init(pc, core, &core->pm_bus,
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
 *      _qsgmiie_notify_duplex
 * Purpose:
 *      Program duplex if (and only if) serdesqsgmiie is an intermediate PHY.
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
_qsgmiie_notify_duplex(int unit, soc_port_t port, uint32 duplex)
{
    return SOC_E_NONE;
}

/*
 * Function:
 *      _qsgmiie_stop
 * Purpose:
 *      Put serdesqsgmiie SERDES in or out of reset depending on conditions
 */

STATIC int
_qsgmiie_stop(int unit, soc_port_t port)
{
    phy_ctrl_t* pc; 
    soc_phymod_ctrl_t *pmc;
    phymod_phy_access_t *pm_phy;
    phymod_phy_power_t phy_power;
    int  stop, copper;

    pc = INT_PHY_SW_STATE(unit, port);
    pmc = &pc->phymod_ctrl;
    pm_phy = &pmc->phy[0]->pm_phy;

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
 *      _qsgmiie_notify_stop
 * Purpose:
 *      Add a reason to put serdesqsgmiie PHY in reset.
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number.
 *      flags - Reason to stop
 * Returns:     
 *      SOC_E_XXX
 */

STATIC int
_qsgmiie_notify_stop(int unit, soc_port_t port, uint32 flags)
{
    INT_PHY_SW_STATE(unit, port)->stop |= flags;
    return _qsgmiie_stop(unit, port);
}

/*  
 * Function:
 *      _qsgmiie_notify_resume
 * Purpose:
 *      Remove a reason to put serdesqsgmiie PHY in reset.
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number.
 *      flags - Reason to stop
 * Returns:     
 *      SOC_E_XXX
 */

STATIC int
_qsgmiie_notify_resume(int unit, soc_port_t port, uint32 flags)
{   
    INT_PHY_SW_STATE(unit, port)->stop &= ~flags;
    return _qsgmiie_stop(unit, port);
}

/*
 * Function:
 *      _qsgmiie_notify_speed
 * Purpose:
 *      Program duplex if (and only if) serdesqsgmiie is an intermediate PHY.
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
_qsgmiie_notify_speed(int unit, soc_port_t port, uint32 speed)
{
    phy_ctrl_t* pc; 
    qsgmiie_config_t *pCfg;
    int  fiber;

    pc = INT_PHY_SW_STATE(unit, port);
    pCfg = (qsgmiie_config_t *) pc->driver_data;
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
        (_qsgmiie_notify_stop(unit, port, PHY_STOP_SPEED_CHG));

    /* Update speed */
    SOC_IF_ERROR_RETURN
        (phy_qsgmiie_speed_set(unit, port, speed));

    /* Take SERDES PHY out of reset */
    SOC_IF_ERROR_RETURN
        (_qsgmiie_notify_resume(unit, port, PHY_STOP_SPEED_CHG));

    return SOC_E_NONE;
}

/*
 * Function:
 *      qsgmiie_media_setup
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
_qsgmiie_notify_interface(int unit, soc_port_t port, uint32 intf)
{
    return SOC_E_NONE;
}

/*
 * Function:
 *      _qsgmiie_notify_mac_loopback
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
_qsgmiie_notify_mac_loopback(int unit, soc_port_t port, uint32 enable)
{
    return SOC_E_NONE;
}

STATIC int
phy_qsgmiie_notify(int unit, soc_port_t port,
                       soc_phy_event_t event, uint32 value)
{
    int             rv;
    rv = SOC_E_NONE ;

    if (event >= phyEventCount) {
        return SOC_E_PARAM;
    }

    switch(event) {
    case phyEventInterface:
        rv = (_qsgmiie_notify_interface(unit, port, value));
        break;
    case phyEventDuplex:
        rv = (_qsgmiie_notify_duplex(unit, port, value));
        break;
    case phyEventSpeed:
        rv = (_qsgmiie_notify_speed(unit, port, value));
        break;
    case phyEventStop:
        rv = (_qsgmiie_notify_stop(unit, port, value));
        break;
    case phyEventResume:
        rv = (_qsgmiie_notify_resume(unit, port, value));
        break;
    case phyEventAutoneg:
#if 0
        rv = (_qsgmiie_an_set(unit, port, value));
#endif
        break;
    case phyEventMacLoopback:
        rv = (_qsgmiie_notify_mac_loopback(unit, port, value));
        break;
    default:
        rv = SOC_E_UNAVAIL;
        break;
    }

    return rv;
}

/*
 * Function:
 *      phy_qsgmiie_diag_ctrl
 * Purpose:
 *      xx
 * Parameters:
 *      unit - BCM unit number.
 *      port - Port number. 

 * Returns:     
 *      SOC_E_NONE
 */

STATIC int
phy_qsgmiie_diag_ctrl(
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
                                 "phy_tscmod_diag_ctrl: "
                                 "u=%d p=%d PHY_DIAG_CTRL_DSC 0x%x\n"),
                      unit, port, PHY_DIAG_CTRL_DSC));
            SOC_IF_ERROR_RETURN(qsgmiie_uc_status_dump(unit, port));
            break;
        default:
            if (op_type == PHY_DIAG_CTRL_SET) {
                SOC_IF_ERROR_RETURN(phy_qsgmiie_control_set(unit,port,op_cmd,PTR_TO_INT(arg)));
            } else if (op_type == PHY_DIAG_CTRL_GET) {
                SOC_IF_ERROR_RETURN(phy_qsgmiie_control_get(unit,port,op_cmd,(uint32 *)arg));
            }
            break ;
        }
    return (SOC_E_NONE);
}


#ifdef BROADCOM_DEBUG
void
qsgmiie_state_dump(int unit, soc_port_t port)
{
    /* Show PHY configuration summary */
    /* Lane status */
    /* Show Counters */
    /* Dump Registers */
}
#endif /* BROADCOM_DEBUG */

/*
 * Variable:
 *      qsgmiie_drv
 * Purpose:
 *      Phy Driver for TSC/Eagle 
 */
phy_driver_t phy_qsgmiie_drv = {
    /* .drv_name                      = */ "TSC/Eagle PHYMOD PHY Driver",
    /* .pd_init                       = */ phy_qsgmiie_init,
    /* .pd_reset                      = */ phy_null_reset,
    /* .pd_link_get                   = */ phy_qsgmiie_link_get,
    /* .pd_enable_set                 = */ phy_qsgmiie_enable_set,
    /* .pd_enable_get                 = */ phy_qsgmiie_enable_get,
    /* .pd_duplex_set                 = */ phy_null_set,
    /* .pd_duplex_get                 = */ phy_qsgmiie_duplex_get,
    /* .pd_speed_set                  = */ phy_qsgmiie_speed_set,
    /* .pd_speed_get                  = */ phy_qsgmiie_speed_get,
    /* .pd_master_set                 = */ phy_null_set,
    /* .pd_master_get                 = */ phy_null_zero_get,
    /* .pd_an_set                     = */ phy_qsgmiie_an_set,
    /* .pd_an_get                     = */ phy_qsgmiie_an_get,
    /* .pd_adv_local_set              = */ NULL, /* Deprecated */
    /* .pd_adv_local_get              = */ NULL, /* Deprecated */
    /* .pd_adv_remote_get             = */ NULL, /* Deprecated */ 
    /* .pd_lb_set                     = */ phy_qsgmiie_lb_set,
    /* .pd_lb_get                     = */ phy_qsgmiie_lb_get,
    /* .pd_interface_set              = */ phy_null_interface_set,
    /* .pd_interface_get              = */ phy_null_interface_get,
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
    /* .pd_control_set                = */ phy_qsgmiie_control_set,
    /* .pd_control_get                = */ phy_qsgmiie_control_get,
    /* .pd_reg_read                   = */ phy_qsgmiie_reg_read,
    /* .pd_reg_write                  = */ phy_qsgmiie_reg_write,
    /* .pd_reg_modify                 = */ phy_qsgmiie_reg_modify,
    /* .pd_notify                     = */ phy_qsgmiie_notify,
    /* .pd_probe                      = */ phy_qsgmiie_probe,
    /* .pd_ability_advert_set         = */ phy_qsgmiie_ability_advert_set, 
    /* .pd_ability_advert_get         = */ phy_qsgmiie_ability_advert_get,
    /* .pd_ability_remote_get         = */ phy_qsgmiie_ability_remote_get,
    /* .pd_ability_local_get          = */ phy_qsgmiie_ability_local_get,
    /* .pd_firmware_set               = */ NULL,
    /* .pd_timesync_config_set        = */ NULL,
    /* .pd_timesync_config_get        = */ NULL,
    /* .pd_timesync_control_set       = */ NULL,
    /* .pd_timesync_control_set       = */ NULL,
    /* .pd_diag_ctrl                  = */ phy_qsgmiie_diag_ctrl,
    /* .pd_lane_control_set           = */ phy_qsgmiie_per_lane_control_set,    
    /* .pd_lane_control_get           = */ phy_qsgmiie_per_lane_control_get
};

#else
int _qsgmiie_not_empty;
#endif /*  INCLUDE_SERDES_QSGMIIE */

