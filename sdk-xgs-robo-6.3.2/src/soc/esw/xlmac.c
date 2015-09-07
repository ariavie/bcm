/*
 * $Id: xlmac.c 1.34 Broadcom SDK $
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
 * XLMAC driver
 */

#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>
#include <sal/core/boot.h>

#include <soc/drv.h>
#include <soc/error.h>
#include <soc/cmic.h>
#include <soc/portmode.h>
#include <soc/ll.h>
#include <soc/counter.h>
#include <soc/macutil.h>
#include <soc/phyctrl.h>
#include <soc/debug.h>

#ifdef BCM_XLMAC_SUPPORT

#define DBG_10G_OUT(flags, stuff) SOC_DEBUG(SOC_DBG_10G | flags, stuff)
#define DBG_10G_VERB(stuff) DBG_10G_OUT(SOC_DBG_VERBOSE, stuff)
#define DBG_10G_WARN(stuff) DBG_10G_OUT(SOC_DBG_WARN, stuff)

/*
 * XLMAC Register field definitions.
 */

#define JUMBO_MAXSZ              0x3fe8 /* Max legal value (per regsfile) */

mac_driver_t soc_mac_xl;

#ifdef BROADCOM_DEBUG
static char *mac_xl_encap_mode[] = SOC_ENCAP_MODE_NAMES_INITIALIZER;
static char *mac_xl_port_if_names[] = SOC_PORT_IF_NAMES_INITIALIZER;
#endif /* BROADCOM_DEBUG */

#define SOC_XLMAC_SPEED_10     0x0
#define SOC_XLMAC_SPEED_100    0x1
#define SOC_XLMAC_SPEED_1000   0x2
#define SOC_XLMAC_SPEED_2500   0x3
#define SOC_XLMAC_SPEED_10000  0x4

struct {
    int speed;
    uint32 clock_rate;
}_mac_xl_clock_rate[] = {
    { 40000, 312 },
    { 20000, 156 },
    { 10000, 78  },
    { 5000,  78  },
    { 2500,  312 },
    { 1000,  125 },
    { 0,     25  },
};

void
_mac_xl_speed_to_clock_rate(int unit, soc_port_t port, int speed,
                            uint32 *clock_rate)
{
    int idx;


    for (idx = 0;
         idx < sizeof(_mac_xl_clock_rate) / sizeof(_mac_xl_clock_rate[0]);
         idx++) {
        if (speed >=_mac_xl_clock_rate[idx].speed) {
            *clock_rate = _mac_xl_clock_rate[idx].clock_rate;
            return;
        }
    }
    *clock_rate = 0;
}

STATIC int
_mac_xl_drain_cells(int unit, soc_port_t port)
{
    int         rv;
    int         pause_tx, pause_rx, pfc_rx, llfc_rx;
    soc_field_t fields[2];
    uint32      values[2], fval;
    uint64      mac_ctrl, rval64;
    soc_timeout_t to;

    rv  = SOC_E_NONE;

    if (SOC_IS_RELOADING(unit)) {
        return rv;
    }

    /* Disable pause/pfc function */
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_pause_get(unit, port, &pause_tx, &pause_rx));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_pause_set(unit, port, pause_tx, 0));

    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_get(unit, port, SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                   &pfc_rx));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, port, SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                   0));

    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_get(unit, port, SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                   &llfc_rx));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, port, SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                   0));

    /* Drain data in TX FIFO without egressing */
    SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, port, &mac_ctrl));
    soc_reg64_field32_set(unit, XLMAC_CTRLr, &mac_ctrl, SOFT_RESETf, 1);
    SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, port, mac_ctrl));
    fields[0] = DISCARDf;
    values[0] = 1;
    fields[1] = EP_DISCARDf;
    values[1] = 1;
    SOC_IF_ERROR_RETURN(soc_reg_fields32_modify(unit, XLMAC_TX_CTRLr, port, 2,
                                                fields, values));
    soc_reg64_field32_set(unit, XLMAC_CTRLr, &mac_ctrl, SOFT_RESETf, 0);
    SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, port, mac_ctrl));

    /* Notify PHY driver */
    SOC_IF_ERROR_RETURN
        (soc_phyctrl_notify(unit, port, phyEventStop, PHY_STOP_DRAIN));

    /* Wait until mmu cell count is 0 */
    SOC_IF_ERROR_RETURN(soc_egress_drain_cells(unit, port, 250000));

    /* Wait until TX fifo cell count is 0 */
    soc_timeout_init(&to, 250000, 0);
    for (;;) {
        SOC_IF_ERROR_RETURN(READ_XLMAC_TXFIFO_CELL_CNTr(unit, port, &rval64));
        fval = soc_reg64_field32_get(unit, XLMAC_TXFIFO_CELL_CNTr, rval64,
                                     CELL_CNTf);
        if (fval == 0) {
            break;
        }
        if (soc_timeout_check(&to)) {
            soc_cm_debug(DK_ERR,
                         "ERROR: port %d:%s: "
                         "timeout draining TX FIFO (%d cells remain)\n",
                         unit, SOC_PORT_NAME(unit, port), fval);
            return SOC_E_INTERNAL;
        }
    }

    /* Notify PHY driver */
    SOC_IF_ERROR_RETURN
        (soc_phyctrl_notify(unit, port, phyEventResume, PHY_STOP_DRAIN));
  
    /* Stop TX FIFO drainging */
    values[0] = 0;
    values[1] = 0;
    SOC_IF_ERROR_RETURN(soc_reg_fields32_modify(unit, XLMAC_TX_CTRLr, port, 2,
                                                fields, values));

    /* Restore original pause/pfc/llfc configuration */
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_pause_set(unit, port, pause_tx, pause_rx));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, port, SOC_MAC_CONTROL_PFC_RX_ENABLE,
                                   pfc_rx));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, port, SOC_MAC_CONTROL_LLFC_RX_ENABLE,
                                   llfc_rx));

    return rv;
}

/*
 * Function:
 *      mac_xl_init
 * Purpose:
 *      Initialize Xlmac into a known good state.
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 *
 */
STATIC int
mac_xl_init(int unit, soc_port_t port)
{
    soc_info_t *si;
    uint64 mac_ctrl, rx_ctrl, tx_ctrl, rval;
    uint32 ipg;
    int mode;

    DBG_10G_VERB(("mac_xl_init: unit %d port %s\n",
                  unit, SOC_PORT_NAME(unit, port)));

    si = &SOC_INFO(unit);

    /* Disable Tx/Rx, assume that MAC is stable (or out of reset) */
    SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, port, &mac_ctrl));

    /* Reset EP credit before de-assert SOFT_RESET */
    if (soc_reg64_field32_get(unit, XLMAC_CTRLr, mac_ctrl, SOFT_RESETf)) {
        SOC_IF_ERROR_RETURN(soc_port_credit_reset(unit, port));
    }

    soc_reg64_field32_set(unit, XLMAC_CTRLr, &mac_ctrl, SOFT_RESETf, 0);
    soc_reg64_field32_set(unit, XLMAC_CTRLr, &mac_ctrl, RX_ENf, 0);
    soc_reg64_field32_set(unit, XLMAC_CTRLr, &mac_ctrl, TX_ENf, 0);
    soc_reg64_field32_set(unit, XLMAC_CTRLr, &mac_ctrl,
                          XGMII_IPG_CHECK_DISABLEf,
                          IS_HG_PORT(unit, port) ? 1 : 0);
    SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, port, mac_ctrl));

    SOC_IF_ERROR_RETURN(READ_XLMAC_RX_CTRLr(unit, port, &rx_ctrl));
    soc_reg64_field32_set(unit, XLMAC_RX_CTRLr, &rx_ctrl, STRIP_CRCf, 0);
    soc_reg64_field32_set(unit, XLMAC_RX_CTRLr, &rx_ctrl, STRICT_PREAMBLEf,
                          si->port_speed_max[port] >= 10000 &&
                          IS_XE_PORT(unit, port) ? 1 : 0);
    SOC_IF_ERROR_RETURN(WRITE_XLMAC_RX_CTRLr(unit, port, rx_ctrl));

    SOC_IF_ERROR_RETURN(READ_XLMAC_TX_CTRLr(unit, port, &tx_ctrl));
    ipg = IS_HG_PORT(unit,port)?  SOC_PERSIST(unit)->ipg[port].fd_hg:
                                  SOC_PERSIST(unit)->ipg[port].fd_xe;
    soc_reg64_field32_set(unit, XLMAC_TX_CTRLr, &tx_ctrl, AVERAGE_IPGf,
                          (ipg / 8) & 0x1f);
    soc_reg64_field32_set(unit, XLMAC_TX_CTRLr, &tx_ctrl, CRC_MODEf, 2);
    SOC_IF_ERROR_RETURN(WRITE_XLMAC_TX_CTRLr(unit, port, tx_ctrl));

    if (IS_ST_PORT(unit, port)) {
        soc_mac_xl.md_pause_set(unit, port, FALSE, FALSE);
    } else {
        soc_mac_xl.md_pause_set(unit, port, TRUE, TRUE);
    }

    SOC_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, XLMAC_PFC_CTRLr, port, PFC_REFRESH_ENf,
                                1));

    if (soc_property_port_get(unit, port, spn_PHY_WAN_MODE, FALSE)) {
        SOC_IF_ERROR_RETURN
            (soc_mac_xl.md_control_set(unit, port,
                                       SOC_MAC_CONTROL_FRAME_SPACING_STRETCH,
                                       13));
    }

    /* Set jumbo max size (8000 byte payload) */
    COMPILER_64_ZERO(rval);
    soc_reg64_field32_set(unit, XLMAC_RX_MAX_SIZEr, &rval, RX_MAX_SIZEf,
                          JUMBO_MAXSZ);
    SOC_IF_ERROR_RETURN(WRITE_XLMAC_RX_MAX_SIZEr(unit, port, rval));

    /* Setup header mode, check for property for higig2 mode */
    COMPILER_64_ZERO(rval);
    if (!IS_XE_PORT(unit, port) && !IS_GE_PORT(unit, port)) {
        mode = soc_property_port_get(unit, port, spn_HIGIG2_HDR_MODE, 0) ?
            2 : 1;
        soc_reg64_field32_set(unit, XLMAC_MODEr, &rval, HDR_MODEf, mode);
    }
    switch (si->port_speed_max[port]) {
    case 10:
        mode = SOC_XLMAC_SPEED_10;
        break;
    case 100:
        mode = SOC_XLMAC_SPEED_100;
        break;
    case 1000:
        mode = SOC_XLMAC_SPEED_1000;
        break;
    case 2500:
        mode = SOC_XLMAC_SPEED_2500;
        break;
    default:
        mode = SOC_XLMAC_SPEED_10000;
        break;
    }
    soc_reg64_field32_set(unit, XLMAC_MODEr, &rval, SPEED_MODEf, mode);
    SOC_IF_ERROR_RETURN(WRITE_XLMAC_MODEr(unit, port, rval));

    SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_CTRLr(unit, port, &rval));
    soc_reg64_field32_set(unit, XLMAC_RX_LSS_CTRLr, &rval,
                          DROP_TX_DATA_ON_LOCAL_FAULTf, 1);
    soc_reg64_field32_set(unit, XLMAC_RX_LSS_CTRLr, &rval,
                          DROP_TX_DATA_ON_REMOTE_FAULTf, 1);
    SOC_IF_ERROR_RETURN(WRITE_XLMAC_RX_LSS_CTRLr(unit, port, rval));

    /* Disable loopback and bring XLMAC out of reset */
    soc_reg64_field32_set(unit, XLMAC_CTRLr, &mac_ctrl, LOCAL_LPBKf, 0);
    soc_reg64_field32_set(unit, XLMAC_CTRLr, &mac_ctrl, RX_ENf, 1);
    soc_reg64_field32_set(unit, XLMAC_CTRLr, &mac_ctrl, TX_ENf, 1);
    SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, port, mac_ctrl));

    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_enable_set
 * Purpose:
 *      Enable or disable MAC
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 *      enable - TRUE to enable, FALSE to disable
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_enable_set(int unit, soc_port_t port, int enable)
{
    uint64 ctrl, octrl;
    pbmp_t mask;

    DBG_10G_VERB(("mac_xl_enable_set: unit %d port %s enable=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  enable ? "True" : "False"));

    SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, port, &ctrl));
    octrl = ctrl;
    /* Don't disable TX since it stops egress and hangs if CPU sends */
    soc_reg64_field32_set(unit, XLMAC_CTRLr, &ctrl, TX_ENf, 1);
    soc_reg64_field32_set(unit, XLMAC_CTRLr, &ctrl, RX_ENf, enable ? 1 : 0);

    if (COMPILER_64_EQ(ctrl, octrl)) {
        return SOC_E_NONE;
    }

    if (enable) {
        /* Reset EP credit before de-assert SOFT_RESET */
        SOC_IF_ERROR_RETURN(soc_port_credit_reset(unit, port));

        /* Enable both TX and RX, deassert SOFT_RESET */
        soc_reg64_field32_set(unit, XLMAC_CTRLr, &ctrl, SOFT_RESETf, 0);
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, port, ctrl));

        /* Add port to EPC_LINK */
        soc_link_mask2_get(unit, &mask);
        SOC_PBMP_PORT_ADD(mask, port);
        SOC_IF_ERROR_RETURN(soc_link_mask2_set(unit, mask));
    } else {
        /* Disable RX */
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, port, ctrl));

        /* Remove port from EPC_LINK */
        soc_link_mask2_get(unit, &mask);
        SOC_PBMP_PORT_REMOVE(mask, port);
        SOC_IF_ERROR_RETURN(soc_link_mask2_set(unit, mask));

        /* Drain cells */
        SOC_IF_ERROR_RETURN(_mac_xl_drain_cells(unit, port));


        /* Reset port FIFO */
        SOC_IF_ERROR_RETURN(soc_port_fifo_reset(unit, port));

        /* Put port into SOFT_RESET */
        soc_reg64_field32_set(unit, XLMAC_CTRLr, &ctrl, SOFT_RESETf, 1);
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, port, ctrl));
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_enable_get
 * Purpose:
 *      Get MAC enable state
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 *      enable - (OUT) TRUE if enabled, FALSE if disabled
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_enable_get(int unit, soc_port_t port, int *enable)
{
    uint64 ctrl;

    SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, port, &ctrl));

    *enable = soc_reg64_field32_get(unit, XLMAC_CTRLr, ctrl, RX_ENf);

    DBG_10G_VERB(("mac_xl_enable_get: unit %d port %s enable=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  *enable ? "True" : "False"));

    return SOC_E_NONE;
}

/*
 * Function:
 *      _mac_xl_timestamp_delay_set
 * Purpose:
 *      Set Timestamp delay for one-step to account for lane and pipeline delay.
 * Parameters:
 *      unit - XGS unit #.
 *      port - Port number on unit.
 *      speed - Speed
 *      phy_mode - single/dual/quad phy mode
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
_mac_xl_timestamp_delay_set(int unit, soc_port_t port, int speed)
{
    uint64 ctrl;
    uint32 clk_rate, tx_clk_ns;
    int delay;
    int divisor;

    DBG_10G_VERB(("mac_xl_timestamp_delay_set: unit %d port %s\n",
                  unit, SOC_PORT_NAME(unit, port)));

    SOC_IF_ERROR_RETURN(READ_XLMAC_TIMESTAMP_ADJUSTr(unit, port, &ctrl));

    _mac_xl_speed_to_clock_rate(unit, port, speed, &clk_rate);
    /* Tx clock rate for single/dual/quad phy mode */
    if ((speed >= 5000) && (speed <= 40000)) {
        divisor = speed > 20000 ? 1 : speed > 10000 ? 2 : 4;
        /* tx clock rate in ns */
        tx_clk_ns = ((1000 / clk_rate) / divisor); 
    } else {
        /* Same tx clk rate for < 10G  for all phy modes*/
        tx_clk_ns = 1000 / clk_rate;
    }
    
    /* 
     * MAC pipeline delay for XGMII/XGMII mode is:
     *          = (5.5 * TX line clock period) + (Timestamp clock period)
     */
    /* signed value of pipeline delay in ns */
    delay = SOC_TIMESYNC_PLL_CLOCK_NS(unit) - ((11 * tx_clk_ns ) / 2);
    soc_reg64_field32_set(unit, XLMAC_TIMESTAMP_ADJUSTr, &ctrl,
                          TS_OSTS_ADJUSTf, delay );

#if 0 
    /* 
     * Lane delay for xlmac lanes
     *   Lane_0(0-3)  : 1 * TX line clock period
     *   Lane_1(4-7)  : 2 * TX line clock period
     *   Lane_2(8-11) : 3 * TX line clock period
     *   Lane_3(12-15): 4 * TX line clock period
     */
    /* unsigned value of lane delay in ns */
    delay = 1 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl, 
                          TS_ADJUST_DEMUX_DELAY_0f, delay );
    delay = 2 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl, 
                          TS_ADJUST_DEMUX_DELAY_1f, delay );
    delay = 3 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl, 
                          TS_ADJUST_DEMUX_DELAY_2f, delay );
    delay = 4 * tx_clk_ns;
    soc_reg64_field32_set(unit, XLMAC_OSTS_TIMESTAMP_ADJUSTr, &ctrl, 
                          TS_ADJUST_DEMUX_DELAY_3f, delay );
#endif
    SOC_IF_ERROR_RETURN(WRITE_XLMAC_TIMESTAMP_ADJUSTr(unit, port, ctrl));

    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_duplex_set
 * Purpose:
 *      Set XLMAC in the specified duplex mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      duplex - Boolean: true --> full duplex, false --> half duplex.
 * Returns:
 *      SOC_E_XXX
 * Notes:
 */
STATIC int
mac_xl_duplex_set(int unit, soc_port_t port, int duplex)
{
    DBG_10G_VERB(("mac_xl_duplex_set: unit %d port %s duplex=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  duplex ? "Full" : "Half"));
    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_duplex_get
 * Purpose:
 *      Get XLMAC duplex mode.
 * Parameters:
 *      unit - XGS unit #.
 *      duplex - (OUT) Boolean: true --> full duplex, false --> half duplex.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_duplex_get(int unit, soc_port_t port, int *duplex)
{
    *duplex = TRUE; /* Always full duplex */

    DBG_10G_VERB(("mac_xl_duplex_get: unit %d port %s duplex=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  *duplex ? "Full" : "Half"));
    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_pause_set
 * Purpose:
 *      Configure XLMAC to transmit/receive pause frames.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_tx - Boolean: transmit pause or -1 (don't change)
 *      pause_rx - Boolean: receive pause or -1 (don't change)
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_pause_set(int unit, soc_port_t port, int pause_tx, int pause_rx)
{
    static soc_field_t fields[2] = { TX_PAUSE_ENf, RX_PAUSE_ENf };
    uint32 values[2];

    DBG_10G_VERB(("mac_xl_pause_set: unit %d port %s TX=%s RX=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  pause_tx != FALSE ? "on" : "off",
                  pause_rx != FALSE ? "on" : "off"));

    values[0] = pause_tx != FALSE ? 1 : 0;
    values[1] = pause_rx != FALSE ? 1 : 0;
    return soc_reg_fields32_modify(unit, XLMAC_PAUSE_CTRLr, port, 2,
                                   fields, values);
}

/*
 * Function:
 *      mac_xl_pause_get
 * Purpose:
 *      Return the pause ability of XLMAC
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_tx - Boolean: transmit pause
 *      pause_rx - Boolean: receive pause
 *      pause_mac - MAC address used for pause transmission.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_pause_get(int unit, soc_port_t port, int *pause_tx, int *pause_rx)
{
    uint64 rval;

    SOC_IF_ERROR_RETURN(READ_XLMAC_PAUSE_CTRLr(unit, port, &rval));
    *pause_tx =
        soc_reg64_field32_get(unit, XLMAC_PAUSE_CTRLr, rval, TX_PAUSE_ENf);
    *pause_rx =
        soc_reg64_field32_get(unit, XLMAC_PAUSE_CTRLr, rval, RX_PAUSE_ENf);
    DBG_10G_VERB(("mac_xl_pause_get: unit %d port %s TX=%s RX=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  *pause_tx ? "on" : "off",
                  *pause_rx ? "on" : "off"));
    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_speed_set
 * Purpose:
 *      Set XLMAC in the specified speed.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      speed - 10, 100, 1000, 2500, 10000, ...
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_speed_set(int unit, soc_port_t port, int speed)
{
    uint32 mode;
    int enable;
    static soc_field_t fields[2] = {
        LOCAL_FAULT_DISABLEf, REMOTE_FAULT_DISABLEf
    };
    uint32 values[2];

    DBG_10G_VERB(("mac_xl_speed_set: unit %d port %s speed=%dMb\n",
                  unit, SOC_PORT_NAME(unit, port),
                  speed));

    switch (speed) {
    case 10:
        mode = SOC_XLMAC_SPEED_10;
        break;
    case 100:
        mode = SOC_XLMAC_SPEED_100;
        break;
    case 1000:
        mode = SOC_XLMAC_SPEED_1000;
        break;
    case 2500:
        mode = SOC_XLMAC_SPEED_2500;
        break;
    case 5000:
        mode = SOC_XLMAC_SPEED_10000;
        break;
    case 0:
        return SOC_E_NONE;              /* Support NULL PHY */
    default:
        if (speed < 10000) {
            return SOC_E_PARAM;
        }
        mode = SOC_XLMAC_SPEED_10000;
        break;
    }

    SOC_IF_ERROR_RETURN(mac_xl_enable_get(unit, port, &enable));
        
    if (enable) {
        /* Turn off TX/RX enable */
        SOC_IF_ERROR_RETURN(mac_xl_enable_set(unit, port, 0));
    }

    /* Update the speed */
    SOC_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, XLMAC_MODEr, port, SPEED_MODEf, mode));

    SOC_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, XLMAC_RX_CTRLr, port, STRICT_PREAMBLEf,
                                speed >= 10000 && IS_XE_PORT(unit, port) ?
                                1 : 0));


    values[0] = values[1] = speed < 5000 ? 1 : 0;
    SOC_IF_ERROR_RETURN
        (soc_reg_fields32_modify(unit, XLMAC_RX_LSS_CTRLr, port, 2, fields,
                                 values));

    /* Update port speed related setting in components other than MAC/SerDes*/
    SOC_IF_ERROR_RETURN(soc_port_speed_update(unit, port, speed));

    if (enable) {
        /* Re-enable transmitter and receiver */
        SOC_IF_ERROR_RETURN(mac_xl_enable_set(unit, port, 1));
    }

    /* Set Timestamp Mac Delays */
    SOC_IF_ERROR_RETURN(_mac_xl_timestamp_delay_set(unit, port, speed));

    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_speed_get
 * Purpose:
 *      Get XLMAC speed
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      speed - (OUT) speed in Mb
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_speed_get(int unit, soc_port_t port, int *speed)
{
    uint64 rval;

    SOC_IF_ERROR_RETURN(READ_XLMAC_MODEr(unit, port, &rval));
    switch (soc_reg64_field32_get(unit, XLMAC_MODEr, rval, SPEED_MODEf)) {
    case SOC_XLMAC_SPEED_10:
        *speed = 10;
        break;
    case SOC_XLMAC_SPEED_100:
        *speed = 100;
        break;
    case SOC_XLMAC_SPEED_1000:
        *speed = 1000;
        break;
    case SOC_XLMAC_SPEED_2500:
        *speed = 2500;
        break;
    case SOC_XLMAC_SPEED_10000:
    default:
        *speed = 10000;
        break;
    }

    DBG_10G_VERB(("mac_xl_speed_get: unit %d port %s speed=%dMb\n",
                  unit, SOC_PORT_NAME(unit, port),
                  *speed));
    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_loopback_set
 * Purpose:
 *      Set a XLMAC into/out-of loopback mode
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS unit # on unit.
 *      loopback - Boolean: true -> loopback mode, false -> normal operation
 * Note:
 *      On Xlmac, when setting loopback, we enable the TX/RX function also.
 *      Note that to test the PHY, we use the remote loopback facility.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_loopback_set(int unit, soc_port_t port, int lb)
{
    DBG_10G_VERB(("mac_xl_loopback_set: unit %d port %s loopback=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  lb ? "local" : "no"));

    /* need to enable clock compensation for applicable serdes device */
    (void)soc_phyctrl_notify(unit, port, phyEventMacLoopback, lb ? 1 : 0);

    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, port,
                                   SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE,
                                   lb ? 0 : 1));
    SOC_IF_ERROR_RETURN
        (soc_mac_xl.md_control_set(unit, port,
                                   SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE,
                                   lb ? 0 : 1));

    return soc_reg_field32_modify(unit, XLMAC_CTRLr, port, LOCAL_LPBKf,
                                  lb ? 1 : 0);

    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_loopback_get
 * Purpose:
 *      Get current XLMAC loopback mode setting.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      loopback - (OUT) Boolean: true = loopback, false = normal
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_loopback_get(int unit, soc_port_t port, int *lb)
{
    uint64 ctrl;

    SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, port, &ctrl));

    *lb = soc_reg64_field32_get(unit, XLMAC_CTRLr, ctrl, LOCAL_LPBKf);

    DBG_10G_VERB(("mac_xl_loopback_get: unit %d port %s loopback=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  *lb ? "True" : "False"));
    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_pause_addr_set
 * Purpose:
 *      Configure PAUSE frame source address.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_mac - (OUT) MAC address used for pause transmission.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_pause_addr_set(int unit, soc_port_t port, sal_mac_addr_t mac)
{
    static soc_field_t fields[2] = { SA_HIf, SA_LOf };
    uint32 values[2];

    DBG_10G_VERB(("mac_xl_pause_addr_set: unit %d port %s MAC=<"
                  "%02x:%02x:%02x:%02x:%02x:%02x>\n",
                  unit, SOC_PORT_NAME(unit, port),
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));

    values[0] = (mac[0] << 8) | mac[1];
    values[1] = (mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];

    SOC_IF_ERROR_RETURN
        (soc_reg_fields32_modify(unit, XLMAC_TX_MAC_SAr, port, 2, fields,
                                 values));
    SOC_IF_ERROR_RETURN
        (soc_reg_fields32_modify(unit, XLMAC_RX_MAC_SAr, port, 2, fields,
                                 values));

    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_pause_addr_get
 * Purpose:
 *      Retrieve PAUSE frame source address.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pause_mac - (OUT) MAC address used for pause transmission.
 * Returns:
 *      SOC_E_XXX
 * NOTE: We always write the same thing to TX & RX SA
 *       so, we just return the contects on RX_MAC_SA.
 */
STATIC int
mac_xl_pause_addr_get(int unit, soc_port_t port, sal_mac_addr_t mac)
{
    uint64 rval64;
    uint32 values[2];

    SOC_IF_ERROR_RETURN(READ_XLMAC_RX_MAC_SAr(unit, port, &rval64));
    values[0] = soc_reg64_field32_get(unit, XLMAC_RX_MAC_SAr, rval64, SA_HIf);
    values[1] = soc_reg64_field32_get(unit, XLMAC_RX_MAC_SAr, rval64, SA_LOf);

    mac[0] = (values[0] & 0x0000ff00) >> 8;
    mac[1] = values[0] & 0x000000ff;
    mac[2] = (values[1] & 0xff000000) >> 24;
    mac[3] = (values[1] & 0x00ff0000) >> 16;
    mac[4] = (values[1] & 0x0000ff00) >> 8;
    mac[5] = values[1] & 0x000000ff;

    DBG_10G_VERB(("mac_xl_pause_addr_get: unit %d port %s MAC=<"
                  "%02x:%02x:%02x:%02x:%02x:%02x>\n",
                  unit, SOC_PORT_NAME(unit, port),
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]));
    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_interface_set
 * Purpose:
 *      Set a XLMAC interface type
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pif - one of SOC_PORT_IF_*
 * Returns:
 *      SOC_E_NONE
 *      SOC_E_UNAVAIL - requested mode not supported.
 * Notes:
 *
 */
STATIC int
mac_xl_interface_set(int unit, soc_port_t port, soc_port_if_t pif)
{
    DBG_10G_VERB(("mac_xl_interface_set: unit %d port %s interface=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  mac_xl_port_if_names[pif]));
    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_interface_get
 * Purpose:
 *      Retrieve XLMAC interface type
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      pif - (OUT) one of SOC_PORT_IF_*
 * Returns:
 *      SOC_E_NONE
 */
STATIC int
mac_xl_interface_get(int unit, soc_port_t port, soc_port_if_t *pif)
{
    *pif = SOC_PORT_IF_MII;

    DBG_10G_VERB(("mac_xl_interface_get: unit %d port %s interface=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  mac_xl_port_if_names[*pif]));
    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_frame_max_set
 * Description:
 *      Set the maximum receive frame size for the port
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      size - Maximum frame size in bytes
 * Return Value:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_frame_max_set(int unit, soc_port_t port, int size)
{
    DBG_10G_VERB(("mac_xl_frame_max_set: unit %d port %s size=%d\n",
                  unit, SOC_PORT_NAME(unit, port),
                  size));

    if (IS_XE_PORT(unit, port)) {
        /* For VLAN tagged packets */
        size += 4;
    }
    return soc_reg_field32_modify(unit, XLMAC_RX_MAX_SIZEr, port, RX_MAX_SIZEf,
                                  size);
}

/*
 * Function:
 *      mac_xl_frame_max_get
 * Description:
 *      Set the maximum receive frame size for the port
 * Parameters:
 *      unit - Device number
 *      port - Port number
 *      size - Maximum frame size in bytes
 * Return Value:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_frame_max_get(int unit, soc_port_t port, int *size)
{
    uint64 rval;

    SOC_IF_ERROR_RETURN(READ_XLMAC_RX_MAX_SIZEr(unit, port, &rval));
    *size = soc_reg64_field32_get(unit, XLMAC_RX_MAX_SIZEr, rval,
                                  RX_MAX_SIZEf);
    if (IS_XE_PORT(unit, port)) {
        /* For VLAN tagged packets */
        *size -= 4;
    }

    DBG_10G_VERB(("mac_xl_frame_max_get: unit %d port %s size=%d\n",
                  unit, SOC_PORT_NAME(unit, port),
                  *size));
    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_ifg_set
 * Description:
 *      Sets the new ifg (Inter-frame gap) value
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      speed  - the speed for which the IFG is being set
 *      duplex - the duplex for which the IFG is being set
 *      ifg - number of bits to use for average inter-frame gap
 * Return Value:
 *      SOC_E_XXX
 * Notes:
 *      The function makes sure the IFG value makes sense and updates the
 *      IPG register in case the speed/duplex match the current settings
 */
STATIC int
mac_xl_ifg_set(int unit, soc_port_t port, int speed,
                soc_port_duplex_t duplex, int ifg)
{
    int         cur_speed;
    int         cur_duplex;
    int         real_ifg;
    soc_ipg_t  *si = &SOC_PERSIST(unit)->ipg[port];
    uint64      rval, orval;
    soc_port_ability_t ability;
    uint32      pa_flag;

    DBG_10G_VERB(("mac_xl_ifg_set: unit %d port %s speed=%dMb duplex=%s "
                  "ifg=%d\n",
                  unit, SOC_PORT_NAME(unit, port),
                  speed, duplex ? "True" : "False", ifg));

    pa_flag = SOC_PA_SPEED(speed); 
    sal_memset(&ability, 0, sizeof(soc_port_ability_t));
    soc_mac_xl.md_ability_local_get(unit, port, &ability);
    if (!(pa_flag & ability.speed_full_duplex)) {
        return SOC_E_PARAM;
    }

    /* Silently adjust the specified ifp bits to valid value */
    /* valid value: 8 to 31 bytes (i.e. multiple of 8 bits) */
    real_ifg = ifg < 64 ? 64 : (ifg + 7) & (0x1f << 3);

    if (IS_XE_PORT(unit, port)) {
        si->fd_xe = real_ifg;
    } else {
        si->fd_hg = real_ifg;
    }

    SOC_IF_ERROR_RETURN(mac_xl_duplex_get(unit, port, &cur_duplex));
    SOC_IF_ERROR_RETURN(mac_xl_speed_get(unit, port, &cur_speed));

    /* XLMAC_MODE supports only 4 speeds with 4 being max as LINK_10G_PLUS */ 
    if ((speed > 10000) && (cur_speed == 10000)) { 
        cur_speed = speed; 
    } 

    if (cur_speed == speed &&
        cur_duplex == (duplex == SOC_PORT_DUPLEX_FULL ? TRUE : FALSE)) {
        SOC_IF_ERROR_RETURN(READ_XLMAC_TX_CTRLr(unit, port, &rval));
        orval = rval;
        soc_reg64_field32_set(unit, XLMAC_TX_CTRLr, &rval, AVERAGE_IPGf,
                              real_ifg / 8);
        if (COMPILER_64_NE(rval, orval)) {
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_TX_CTRLr(unit, port, rval));
        }
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_ifg_get
 * Description:
 *      Sets the new ifg (Inter-frame gap) value
 * Parameters:
 *      unit   - Device number
 *      port   - Port number
 *      speed  - the speed for which the IFG is being set
 *      duplex - the duplex for which the IFG is being set
 *      size - Maximum frame size in bytes
 * Return Value:
 *      SOC_E_XXX
 * Notes:
 *      The function makes sure the IFG value makes sense and updates the
 *      IPG register in case the speed/duplex match the current settings
 */
STATIC int
mac_xl_ifg_get(int unit, soc_port_t port, int speed,
                soc_port_duplex_t duplex, int *ifg)
{
    soc_ipg_t  *si = &SOC_PERSIST(unit)->ipg[port];
    soc_port_ability_t ability;
    uint32      pa_flag;

    if (!duplex) {
        return SOC_E_PARAM;
    }

    pa_flag = SOC_PA_SPEED(speed); 
    sal_memset(&ability, 0, sizeof(soc_port_ability_t));
    soc_mac_xl.md_ability_local_get(unit, port, &ability);
    if (!(pa_flag & ability.speed_full_duplex)) {
        return SOC_E_PARAM;
    }

    if (IS_XE_PORT(unit, port)) {
        *ifg = si->fd_xe;
    } else {
        *ifg = si->fd_hg;
    }

    DBG_10G_VERB(("mac_xl_ifg_get: unit %d port %s speed=%dMb duplex=%s "
                  "ifg=%d\n",
                  unit, SOC_PORT_NAME(unit, port),
                  speed, duplex ? "True" : "False", *ifg));
    return SOC_E_NONE;
}

/*
 * Function:
 *      _mac_xl_port_mode_update
 * Purpose:
 *      Set the XLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      to_hg_port - (TRUE/FALSE)
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
_mac_xl_port_mode_update(int unit, soc_port_t port, int to_hg_port)
{
    uint32              rval;
    uint64              val64;
    soc_pbmp_t          ctr_pbmp;
    int                 rv = SOC_E_NONE;

    /* Pause linkscan */
    soc_linkscan_pause(unit);

    /* Pause counter collection */
    COUNTER_LOCK(unit);

    soc_xport_type_update(unit, port, to_hg_port);

    rv = soc_phyctrl_init(unit, port);

    if (SOC_SUCCESS(rv)) {
        rv = mac_xl_init(unit, port);
    }

    if (SOC_SUCCESS(rv)) {
        rv = mac_xl_enable_set(unit, port, 0);
    }

    if (SOC_SUCCESS(rv)) {
        SOC_PBMP_CLEAR(ctr_pbmp);
        SOC_PBMP_PORT_SET(ctr_pbmp, port);
        COMPILER_64_SET(val64, 0, 0);
        rv = soc_counter_set_by_port(unit, ctr_pbmp, val64);
    }

    COUNTER_UNLOCK(unit);
    soc_linkscan_continue(unit);

    if (SOC_REG_IS_VALID(unit, XLPORT_CONFIGr)) {
        SOC_IF_ERROR_RETURN(READ_XLPORT_CONFIGr(unit, port, &rval));
        soc_reg_field_set(unit, XLPORT_CONFIGr, &rval, HIGIG_MODEf,
                          to_hg_port ? 1 : 0);
        SOC_IF_ERROR_RETURN(WRITE_XLPORT_CONFIGr(unit, port, rval));
    }
    if (SOC_REG_IS_VALID(unit, PORT_CONFIGr)) {
        SOC_IF_ERROR_RETURN(READ_PORT_CONFIGr(unit, port, &rval));
        if (to_hg_port) {
            soc_reg_field_set(unit, PORT_CONFIGr, &rval, HIGIG_MODEf, 1);
        } else {
            soc_reg_field_set(unit, PORT_CONFIGr, &rval, HIGIG_MODEf, 0);
        }
        SOC_IF_ERROR_RETURN(WRITE_XPORT_CONFIGr(unit, port, rval));
    }
    return rv;
}

/*
 * Function:
 *      mac_xl_encap_set
 * Purpose:
 *      Set the XLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (IN) encap bits (defined above)
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_encap_set(int unit, soc_port_t port, int mode)
{
    int enable, encap, rv;

    DBG_10G_VERB(("mac_xl_encap_set: unit %d port %s encapsulation=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  mac_xl_encap_mode[mode]));

    switch (mode) {
    case SOC_ENCAP_IEEE:
        encap = 0;
        break;
    case SOC_ENCAP_HIGIG:
        encap = 1;
        break;
    case SOC_ENCAP_HIGIG2:
        encap = 2;
        break;
    default:
        return SOC_E_PARAM;
    }

    if (!soc_feature(unit, soc_feature_xport_convertible)) {
        if ((IS_E_PORT(unit, port) && mode != SOC_ENCAP_IEEE) ||
            (IS_ST_PORT(unit, port) && mode == SOC_ENCAP_IEEE)) {
            return SOC_E_PARAM;
        }
    }

    SOC_IF_ERROR_RETURN(mac_xl_enable_get(unit, port, &enable));

    if (enable) {
        /* Turn off TX/RX enable */
        SOC_IF_ERROR_RETURN(mac_xl_enable_set(unit, port, 0));
    }

    if (IS_E_PORT(unit, port) && mode != SOC_ENCAP_IEEE) {
        /* XE -> HG */
        SOC_IF_ERROR_RETURN(_mac_xl_port_mode_update(unit, port, TRUE));
    } else if (IS_ST_PORT(unit, port) && mode == SOC_ENCAP_IEEE) {
        /* HG -> XE */
        SOC_IF_ERROR_RETURN(_mac_xl_port_mode_update(unit, port, FALSE));
    }

    /* Update the encapsulation mode */
    rv = soc_reg_field32_modify(unit, XLMAC_MODEr, port, HDR_MODEf, encap);

    SOC_IF_ERROR_RETURN
        (soc_reg_field32_modify(unit, XLMAC_RX_CTRLr, port, STRICT_PREAMBLEf,
                                mode == SOC_ENCAP_IEEE ? 1 : 0));

    if (enable) {
        /* Re-enable transmitter and receiver */
        SOC_IF_ERROR_RETURN(mac_xl_enable_set(unit, port, 1));
    }

    return rv;
}

/*
 * Function:
 *      mac_xl_encap_get
 * Purpose:
 *      Get the XLMAC port encapsulation mode.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (INT) encap bits (defined above)
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_encap_get(int unit, soc_port_t port, int *mode)
{
    uint64 rval;

    if (!mode) {
        return SOC_E_PARAM;
    }

    SOC_IF_ERROR_RETURN(READ_XLMAC_MODEr(unit, port, &rval));
    switch (soc_reg64_field32_get(unit, XLMAC_MODEr, rval, HDR_MODEf)) {
    case 0:
        *mode = SOC_ENCAP_IEEE;
        break;
    case 1:
        *mode = SOC_ENCAP_HIGIG;
        break;
    case 2:
        *mode = SOC_ENCAP_HIGIG2;
        break;
    default:
        *mode = SOC_ENCAP_COUNT;
    }

    DBG_10G_VERB(("mac_xl_encap_get: unit %d port %s encapsulation=%s\n",
                  unit, SOC_PORT_NAME(unit, port),
                  mac_xl_encap_mode[*mode]));
    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_control_set
 * Purpose:
 *      To configure MAC control properties.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      type - MAC control property to set.
 *      int  - New setting for MAC control.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_control_set(int unit, soc_port_t port, soc_mac_control_t type,
                  int value)
{
    uint64 rval, copy;
    uint32 fval;

    DBG_10G_VERB(("mac_xl_control_set: unit %d port %s type=%d value=%d\n",
                  unit, SOC_PORT_NAME(unit, port),
                  type, value));

    switch (type) {
    case SOC_MAC_CONTROL_RX_SET:
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, port, &rval));
        copy = rval;
        soc_reg64_field32_set(unit, XLMAC_CTRLr, &rval, RX_ENf, value ? 1 : 0);
        if (COMPILER_64_NE(rval, copy)) {
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_CTRLr(unit, port, rval));
        }    
        break;
    case SOC_MAC_CONTROL_FRAME_SPACING_STRETCH:
        if (value < 0 || value > 255) {
            return SOC_E_PARAM;
        } else {
            SOC_IF_ERROR_RETURN(READ_XLMAC_TX_CTRLr(unit, port, &rval));
            if (value >= 8) {
                soc_reg64_field32_set(unit, XLMAC_TX_CTRLr, &rval,
                                      THROT_DENOMf, value);
                soc_reg64_field32_set(unit, XLMAC_TX_CTRLr, &rval, THROT_NUMf,
                                      1);
            } else {
                soc_reg64_field32_set(unit, XLMAC_TX_CTRLr, &rval,
                                      THROT_DENOMf, 0);
                soc_reg64_field32_set(unit, XLMAC_TX_CTRLr, &rval, THROT_NUMf,
                                      0);
            }
            SOC_IF_ERROR_RETURN(WRITE_XLMAC_TX_CTRLr(unit, port, rval));
        }
        return SOC_E_NONE;

    case SOC_MAC_PASS_CONTROL_FRAME:
        /* this is always true */
        break;

    case SOC_MAC_CONTROL_PFC_TYPE:
        SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, XLMAC_PFC_TYPEr, port,
                                                   PFC_ETH_TYPEf, value));
        break;

    case SOC_MAC_CONTROL_PFC_OPCODE:
        SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, XLMAC_PFC_OPCODEr,
                                                   port, PFC_OPCODEf, value));
        break;

    case SOC_MAC_CONTROL_PFC_CLASSES:
        if (value != 8) {
            return SOC_E_PARAM;
        }
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_OUI:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, port, &rval));
        fval = soc_reg64_field32_get(unit, XLMAC_PFC_DAr, rval, PFC_MACDA_LOf);
        fval &= 0x00ffffff;
        fval |= (value & 0xff) << 24;
        soc_reg64_field32_set(unit, XLMAC_PFC_DAr, &rval, PFC_MACDA_LOf, fval);

        soc_reg64_field32_set(unit, XLMAC_PFC_DAr, &rval, PFC_MACDA_HIf,
                              value >> 8);
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_DAr(unit, port, rval));
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, port, &rval));
        fval = soc_reg64_field32_get(unit, XLMAC_PFC_DAr, rval, PFC_MACDA_LOf);
        fval &= 0xff000000;
        fval |= value;
        soc_reg64_field32_set(unit, XLMAC_PFC_DAr, &rval, PFC_MACDA_LOf, fval);
        SOC_IF_ERROR_RETURN(WRITE_XLMAC_PFC_DAr(unit, port, rval));
        break;

    case SOC_MAC_CONTROL_PFC_RX_PASS:
        /* this is always true */
        break;

    case SOC_MAC_CONTROL_PFC_RX_ENABLE:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLMAC_PFC_CTRLr, port, RX_PFC_ENf,
                                    value ? 1 : 0));
        break;

    case SOC_MAC_CONTROL_PFC_TX_ENABLE:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLMAC_PFC_CTRLr, port, TX_PFC_ENf,
                                    value ? 1 : 0));
        break;

    case SOC_MAC_CONTROL_PFC_FORCE_XON:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLMAC_PFC_CTRLr, port, FORCE_PFC_XONf,
                                    value ? 1 : 0));
        break;

    case SOC_MAC_CONTROL_PFC_STATS_ENABLE:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLMAC_PFC_CTRLr, port, PFC_STATS_ENf,
                                    value ? 1 : 0));
        break;

    case SOC_MAC_CONTROL_PFC_REFRESH_TIME:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLMAC_PFC_CTRLr, port, 
                                    PFC_REFRESH_TIMERf, value));
        break;

    case SOC_MAC_CONTROL_PFC_XOFF_TIME:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLMAC_PFC_CTRLr, port, 
                                    PFC_XOFF_TIMERf, value));
        break;

    case SOC_MAC_CONTROL_LLFC_RX_ENABLE:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLMAC_LLFC_CTRLr, port, RX_LLFC_ENf,
                                    value ? 1 : 0));
        break;

    case SOC_MAC_CONTROL_LLFC_TX_ENABLE:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLMAC_LLFC_CTRLr, port, TX_LLFC_ENf,
                                    value ? 1 : 0));
        break;

    case SOC_MAC_CONTROL_EEE_ENABLE:
        if (!soc_feature(unit, soc_feature_eee)) {
            return SOC_E_UNAVAIL;
        }
        SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, XLMAC_EEE_CTRLr, 
                                                   port, EEE_ENf, value));
        break;

    case SOC_MAC_CONTROL_EEE_TX_IDLE_TIME:
        if (!soc_feature(unit, soc_feature_eee)) {
            return SOC_E_UNAVAIL;
        }
        SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, XLMAC_EEE_TIMERSr, 
                                    port, EEE_DELAY_ENTRY_TIMERf, value));
        break;

    case SOC_MAC_CONTROL_EEE_TX_WAKE_TIME:
        if (!soc_feature(unit, soc_feature_eee)) {
            return SOC_E_UNAVAIL;
        }
        SOC_IF_ERROR_RETURN(soc_reg_field32_modify(unit, XLMAC_EEE_TIMERSr, 
                                    port, EEE_WAKE_TIMERf, value));
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLMAC_RX_LSS_CTRLr, port,
                                    LOCAL_FAULT_DISABLEf, value ? 0 : 1));
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE:
        SOC_IF_ERROR_RETURN
            (soc_reg_field32_modify(unit, XLMAC_RX_LSS_CTRLr, port,
                                    REMOTE_FAULT_DISABLEf, value ? 0 : 1));
        break;
    case SOC_MAC_CONTROL_FAILOVER_RX_SET:
        break;
    default:
        return SOC_E_UNAVAIL;
    }

    return SOC_E_NONE;
}

/*
 * Function:
 *      mac_xl_control_get
 * Purpose:
 *      To get current MAC control setting.
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      type - MAC control property to set.
 *      int  - New setting for MAC control.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_control_get(int unit, soc_port_t port, soc_mac_control_t type,
                  int *value)
{
    int rv;
    uint64 rval;
    uint32 fval0, fval1;

    if (value == NULL) {
        return SOC_E_PARAM;
    }

    rv = SOC_E_NONE;
    switch (type) {
    case SOC_MAC_CONTROL_RX_SET:
        SOC_IF_ERROR_RETURN(READ_XLMAC_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_CTRLr, rval, RX_ENf);
        break;

    case SOC_MAC_CONTROL_FRAME_SPACING_STRETCH:
        SOC_IF_ERROR_RETURN(READ_XLMAC_TX_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_TX_CTRLr, rval,
                                       THROT_DENOMf);
        break;

    case SOC_MAC_CONTROL_TIMESTAMP_TRANSMIT:
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_TX_TIMESTAMP_FIFO_STATUSr(unit, port, &rval));
        if (soc_reg64_field32_get(unit, XLMAC_TX_TIMESTAMP_FIFO_STATUSr, rval,
                                  ENTRY_COUNTf) == 0) {
            return SOC_E_EMPTY;
        }
        SOC_IF_ERROR_RETURN
            (READ_XLMAC_TX_TIMESTAMP_FIFO_DATAr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_TX_TIMESTAMP_FIFO_DATAr,
                                       rval, TIME_STAMPf);
        break;

    case SOC_MAC_PASS_CONTROL_FRAME:
        *value = TRUE;
        break;

    case SOC_MAC_CONTROL_PFC_TYPE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_TYPEr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_PFC_TYPEr, rval,
                                       PFC_ETH_TYPEf);
        break;

    case SOC_MAC_CONTROL_PFC_OPCODE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_OPCODEr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_PFC_OPCODEr, rval,
                                       PFC_OPCODEf);
        break;

    case SOC_MAC_CONTROL_PFC_CLASSES:
        *value = 8;
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_OUI:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, port, &rval));
        fval0 = soc_reg64_field32_get(unit, XLMAC_PFC_DAr, rval,
                                      PFC_MACDA_LOf);
        fval1 = soc_reg64_field32_get(unit, XLMAC_PFC_DAr, rval,
                                      PFC_MACDA_HIf);
        *value = (fval0 >> 24) | (fval1 << 8);
        break;

    case SOC_MAC_CONTROL_PFC_MAC_DA_NONOUI:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_DAr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_PFC_DAr, rval,
                                       PFC_MACDA_LOf) & 0x00ffffff;
        break;

    case SOC_MAC_CONTROL_PFC_RX_PASS:
        *value = TRUE;
        break;

    case SOC_MAC_CONTROL_PFC_RX_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_PFC_CTRLr, rval,
                                       RX_PFC_ENf);
        break;

    case SOC_MAC_CONTROL_PFC_TX_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_PFC_CTRLr, rval,
                                       TX_PFC_ENf);
        break;

    case SOC_MAC_CONTROL_PFC_FORCE_XON:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_PFC_CTRLr, rval,
                                       FORCE_PFC_XONf);
        break;

    case SOC_MAC_CONTROL_PFC_STATS_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_PFC_CTRLr, rval,
                                       PFC_STATS_ENf);
        break;

    case SOC_MAC_CONTROL_PFC_REFRESH_TIME:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_PFC_CTRLr, rval,
                                       PFC_REFRESH_TIMERf);
        break;

    case SOC_MAC_CONTROL_PFC_XOFF_TIME:
        SOC_IF_ERROR_RETURN(READ_XLMAC_PFC_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_PFC_CTRLr, rval,
                                       PFC_XOFF_TIMERf);
        break;

    case SOC_MAC_CONTROL_LLFC_RX_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_LLFC_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_LLFC_CTRLr, rval,
                                       RX_LLFC_ENf);
        break;

    case SOC_MAC_CONTROL_LLFC_TX_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_LLFC_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_LLFC_CTRLr, rval,
                                       TX_LLFC_ENf);
        break;

    case SOC_MAC_CONTROL_EEE_ENABLE:
        if (!soc_feature(unit, soc_feature_eee)) {
            return SOC_E_UNAVAIL;
        }
        SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_EEE_CTRLr, rval, EEE_ENf);
        break;

    case SOC_MAC_CONTROL_EEE_TX_IDLE_TIME:
        if (!soc_feature(unit, soc_feature_eee)) {
            return SOC_E_UNAVAIL;
        }
        SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_TIMERSr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_EEE_TIMERSr, rval,
                EEE_DELAY_ENTRY_TIMERf);
        break;

    case SOC_MAC_CONTROL_EEE_TX_WAKE_TIME:
        if (!soc_feature(unit, soc_feature_eee)) {
            return SOC_E_UNAVAIL;
        }
        SOC_IF_ERROR_RETURN(READ_XLMAC_EEE_TIMERSr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_EEE_TIMERSr, rval,
                EEE_WAKE_TIMERf);
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_RX_LSS_CTRLr, rval,
                                       LOCAL_FAULT_DISABLEf) ? 0 : 1;
        break;

    case SOC_MAC_CONTROL_FAULT_LOCAL_STATUS:
        SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_STATUSr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_RX_LSS_STATUSr, rval,
                                       LOCAL_FAULT_STATUSf);
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_ENABLE:
        SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_CTRLr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_RX_LSS_CTRLr, rval,
                                       REMOTE_FAULT_DISABLEf) ? 0 : 1;
        break;

    case SOC_MAC_CONTROL_FAULT_REMOTE_STATUS:
        SOC_IF_ERROR_RETURN(READ_XLMAC_RX_LSS_STATUSr(unit, port, &rval));
        *value = soc_reg64_field32_get(unit, XLMAC_RX_LSS_STATUSr, rval,
                                       REMOTE_FAULT_STATUSf);
        break;

    default:
        return SOC_E_UNAVAIL;
    }

    DBG_10G_VERB(("mac_xl_control_get: unit %d port %s type=%d value=%d "
                  "rv=%d\n",
                  unit, SOC_PORT_NAME(unit, port),
                  type, *value, rv));
    return rv;
}

/*
 * Function:
 *      mac_xl_ability_local_get
 * Purpose:
 *      Return the abilities of XLMAC
 * Parameters:
 *      unit - XGS unit #.
 *      port - XGS port # on unit.
 *      mode - (OUT) Supported operating modes as a mask of abilities.
 * Returns:
 *      SOC_E_XXX
 */
STATIC int
mac_xl_ability_local_get(int unit, soc_port_t port,
                          soc_port_ability_t *ability)
{
    soc_info_t *si = &SOC_INFO(unit);
    int blk, bindex, port_speed_max, i;
    int phy_port, active_port;
    uint32 active_mask;

    if (NULL == ability) {
        return SOC_E_PARAM;
    }

    ability->speed_half_duplex  = SOC_PA_ABILITY_NONE;
    ability->pause     = SOC_PA_PAUSE | SOC_PA_PAUSE_ASYMM;
    ability->interface = SOC_PA_INTF_MII | SOC_PA_INTF_XGMII;
    ability->medium    = SOC_PA_ABILITY_NONE;
    ability->loopback  = SOC_PA_LB_MAC;
    ability->flags     = SOC_PA_ABILITY_NONE;
    ability->encap = SOC_PA_ENCAP_IEEE | SOC_PA_ENCAP_HIGIG |
        SOC_PA_ENCAP_HIGIG2;

    /* Adjust port_speed_max according to the port config */
    phy_port = si->port_l2p_mapping[port];
    port_speed_max = SOC_INFO(unit).port_speed_max[port];
    bindex = -1;
    for (i = 0; i < SOC_DRIVER(unit)->port_num_blktype; i++) {
        blk = SOC_PORT_IDX_BLOCK(unit, phy_port, i);
        if (SOC_BLOCK_INFO(unit, blk).type == SOC_BLK_XLPORT) {
            bindex = SOC_PORT_IDX_BINDEX(unit, phy_port, i);
            break;
        }
    }
    if (port_speed_max > 10000) {
        active_mask = 0;
        for (i = bindex + 1; i <= 3; i++) {
            active_port = si->port_p2l_mapping[phy_port - bindex + i];
            if (active_port != -1 &&
                !SOC_PBMP_MEMBER(SOC_PORT_DISABLED_BITMAP(unit, all),
                                 active_port)) {
                active_mask |= 1 << i;
            }
        }
        if (bindex == 0) { /* Lanes 0 */
            if (active_mask & 0x2) { /* lane 1 is in use */
                port_speed_max = 10000;
            } else if (port_speed_max > 20000 && active_mask & 0xc) {
                /* Lane 1 isn't in use, lane 2 or 3 (or both) is (are) in use */
                port_speed_max = 20000;
            }
        } else { /* (Must be) lanes 2 */
            if (active_mask & 0x8) { /* lane 3 is in use */
                port_speed_max = 10000;
            }
        }
    }

    if (IS_HG_PORT(unit , port)) {
        switch (port_speed_max) {
        case 42000:
            ability->speed_full_duplex |= SOC_PA_SPEED_42GB;
            /* fall through */
        case 40000:
            ability->speed_full_duplex |= SOC_PA_SPEED_40GB;
            if(soc_feature(unit, soc_feature_higig_misc_speed_support)) {
                ability->speed_full_duplex |= SOC_PA_SPEED_42GB;
            }
            /* fall through */
        case 30000:
            ability->speed_full_duplex |= SOC_PA_SPEED_30GB;
            /* fall through */
        case 25000:
            ability->speed_full_duplex |= SOC_PA_SPEED_25GB;
            /* fall through */
        case 21000:
            ability->speed_full_duplex |= SOC_PA_SPEED_21GB;
            /* fall through */
        case 20000:
            ability->speed_full_duplex |= SOC_PA_SPEED_20GB;
            if(soc_feature(unit, soc_feature_higig_misc_speed_support)) {
                ability->speed_full_duplex |= SOC_PA_SPEED_21GB;
            }
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
        case 11000:
            /* This speed is taken care in the 10G case */
            /* fall through */
        case 10000:
            ability->speed_full_duplex |= SOC_PA_SPEED_10GB;
            if(soc_feature(unit, soc_feature_higig_misc_speed_support)) {
                ability->speed_full_duplex |= SOC_PA_SPEED_11GB;
                ability->speed_full_duplex |= SOC_PA_SPEED_5000MB;
                /* For 11G and 5G speeds, MAC should be in 10G */
            }
            break;
        default:
            break;
        }
    } else {
        if (port_speed_max >= 40000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_40GB;
        }
        if (port_speed_max >= 20000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_20GB;
        }
        if (port_speed_max >= 10000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_10GB;
        }
        if (port_speed_max >= 5000) {
            ability->speed_full_duplex |= SOC_PA_SPEED_5000MB;
			/* For 5G speed, MAC will actually be set to 10G */
		}
        if (soc_feature(unit, soc_feature_unified_port)) {
            if (port_speed_max >= 2500) {
                ability->speed_full_duplex |= SOC_PA_SPEED_2500MB;
            }
            if (port_speed_max >= 1000) {
                ability->speed_full_duplex |= SOC_PA_SPEED_1000MB;
            }
            if (port_speed_max >= 100) {
                ability->speed_full_duplex |= SOC_PA_SPEED_100MB;
            }
            if (port_speed_max >= 10) {
                ability->speed_full_duplex |= SOC_PA_SPEED_10MB;
            }
        }
    }

    DBG_10G_VERB(("mac_xl_ability_local_get: unit %d port %s "
                  "speed_half=0x%x speed_full=0x%x encap=0x%x pause=0x%x "
                  "interface=0x%x medium=0x%x loopback=0x%x flags=0x%x\n",
                  unit, SOC_PORT_NAME(unit, port),
                  ability->speed_half_duplex, ability->speed_full_duplex,
                  ability->encap, ability->pause, ability->interface,
                  ability->medium, ability->loopback, ability->flags));
    return SOC_E_NONE;
}

/* Exported XLMAC driver structure */
mac_driver_t soc_mac_xl = {
    "XLMAC Driver",               /* drv_name */
    mac_xl_init,                  /* md_init  */
    mac_xl_enable_set,            /* md_enable_set */
    mac_xl_enable_get,            /* md_enable_get */
    mac_xl_duplex_set,            /* md_duplex_set */
    mac_xl_duplex_get,            /* md_duplex_get */
    mac_xl_speed_set,             /* md_speed_set */
    mac_xl_speed_get,             /* md_speed_get */
    mac_xl_pause_set,             /* md_pause_set */
    mac_xl_pause_get,             /* md_pause_get */
    mac_xl_pause_addr_set,        /* md_pause_addr_set */
    mac_xl_pause_addr_get,        /* md_pause_addr_get */
    mac_xl_loopback_set,          /* md_lb_set */
    mac_xl_loopback_get,          /* md_lb_get */
    mac_xl_interface_set,         /* md_interface_set */
    mac_xl_interface_get,         /* md_interface_get */
    NULL,                         /* md_ability_get - Deprecated */
    mac_xl_frame_max_set,         /* md_frame_max_set */
    mac_xl_frame_max_get,         /* md_frame_max_get */
    mac_xl_ifg_set,               /* md_ifg_set */
    mac_xl_ifg_get,               /* md_ifg_get */
    mac_xl_encap_set,             /* md_encap_set */
    mac_xl_encap_get,             /* md_encap_get */
    mac_xl_control_set,           /* md_control_set */
    mac_xl_control_get,           /* md_control_get */
    mac_xl_ability_local_get      /* md_ability_local_get */
 };

#endif /* BCM_XLMAC_SUPPORT */
