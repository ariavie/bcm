/*
 * $Id: port.c,v 1.8 Broadcom SDK $
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
#include <sal/core/libc.h>
#include <shared/alloc.h>
#include <sal/core/spl.h>

#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/portmode.h>
#include <soc/port_ability.h>

int
soc_port_mode_to_ability(soc_port_mode_t mode, soc_port_ability_t *ability)
{
    uint32    port_abil;

    if (NULL == ability) {
        return (SOC_E_PARAM);
    }
 
    /* Half duplex speeds */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_10MB_HD) ? SOC_PA_SPEED_10MB : 0;
    port_abil |= (mode & SOC_PM_100MB_HD) ? SOC_PA_SPEED_100MB : 0;  
    port_abil |= (mode & SOC_PM_1000MB_HD) ? SOC_PA_SPEED_1000MB : 0; 
    port_abil |= (mode & SOC_PM_2500MB_HD) ? SOC_PA_SPEED_2500MB : 0;
    port_abil |= (mode & SOC_PM_3000MB_HD) ? SOC_PA_SPEED_3000MB : 0;
    port_abil |= (mode & SOC_PM_10GB_HD) ? SOC_PA_SPEED_10GB : 0;   
    port_abil |= (mode & SOC_PM_12GB_HD) ? SOC_PA_SPEED_12GB : 0;   
    port_abil |= (mode & SOC_PM_13GB_HD) ? SOC_PA_SPEED_13GB : 0;   
    port_abil |= (mode & SOC_PM_16GB_HD) ? SOC_PA_SPEED_16GB : 0;   
    ability->speed_half_duplex = port_abil;

    /* Full duplex speeds */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_10MB_FD) ? SOC_PA_SPEED_10MB : 0;   
    port_abil |= (mode & SOC_PM_100MB_FD) ? SOC_PA_SPEED_100MB : 0;  
    port_abil |= (mode & SOC_PM_1000MB_FD) ? SOC_PA_SPEED_1000MB : 0;
    port_abil |= (mode & SOC_PM_2500MB_FD) ? SOC_PA_SPEED_2500MB : 0;
    port_abil |= (mode & SOC_PM_3000MB_FD) ? SOC_PA_SPEED_3000MB : 0;
    port_abil |= (mode & SOC_PM_10GB_FD) ? SOC_PA_SPEED_10GB : 0;   
    port_abil |= (mode & SOC_PM_12GB_FD) ? SOC_PA_SPEED_12GB : 0;   
    port_abil |= (mode & SOC_PM_13GB_FD) ? SOC_PA_SPEED_13GB : 0;   
    port_abil |= (mode & SOC_PM_16GB_FD) ? SOC_PA_SPEED_16GB : 0;   
    ability->speed_full_duplex = port_abil;

    /* Pause Modes */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_PAUSE_TX)? SOC_PA_PAUSE_TX : 0;
    port_abil |= (mode & SOC_PM_PAUSE_RX) ? SOC_PA_PAUSE_RX : 0;
    port_abil |= (mode & SOC_PM_PAUSE_ASYMM) ? SOC_PA_PAUSE_ASYMM : 0;
    ability->pause = port_abil;

    /* Interface Types */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_TBI) ? SOC_PA_INTF_TBI : 0;
    port_abil |= (mode & SOC_PM_MII) ? SOC_PA_INTF_MII : 0;
    port_abil |= (mode & SOC_PM_GMII) ? SOC_PA_INTF_GMII : 0;
    port_abil |= (mode & SOC_PM_SGMII) ? SOC_PA_INTF_SGMII : 0; 
    port_abil |= (mode & SOC_PM_XGMII) ? SOC_PA_INTF_XGMII : 0;
    ability->interface = port_abil;

    /* Loopback Mode */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_LB_MAC) ? SOC_PA_LB_MAC : 0;
    port_abil |= (mode & SOC_PM_LB_PHY) ? SOC_PA_LB_PHY : 0;
    port_abil |= (mode & SOC_PM_LB_NONE) ? SOC_PA_LB_NONE : 0;
    ability->loopback = port_abil;

    /* Remaining Flags */
    port_abil = 0;
    port_abil |= (mode & SOC_PM_AN) ? SOC_PA_AUTONEG : 0; 
    port_abil |= (mode & SOC_PM_COMBO) ? SOC_PA_COMBO : 0;
    ability->flags = port_abil;

    return (SOC_E_NONE);
}

int
soc_port_ability_to_mode(soc_port_ability_t *ability, soc_port_mode_t *mode)
{
    uint32          port_abil;
    soc_port_mode_t port_mode;

    if ((NULL == ability) || (NULL == mode)) {
        return (SOC_E_PARAM);
    }
  
    port_mode = 0;

    /* Half duplex speeds */
    port_abil = ability->speed_half_duplex;
    port_mode |= (port_abil & SOC_PA_SPEED_10MB) ? SOC_PM_10MB_HD : 0;
    port_mode |= (port_abil & SOC_PA_SPEED_100MB) ? SOC_PM_100MB_HD : 0;  
    port_mode |= (port_abil & SOC_PA_SPEED_1000MB) ? SOC_PM_1000MB_HD : 0; 
    port_mode |= (port_abil & SOC_PA_SPEED_2500MB) ? SOC_PM_2500MB_HD : 0;
    port_mode |= (port_abil & SOC_PA_SPEED_3000MB) ? SOC_PM_3000MB_HD : 0;
    port_mode |= (port_abil & SOC_PA_SPEED_10GB) ? SOC_PM_10GB_HD : 0;   
    port_mode |= (port_abil & SOC_PA_SPEED_12GB) ? SOC_PM_12GB_HD : 0;   
    port_mode |= (port_abil & SOC_PA_SPEED_13GB) ? SOC_PM_13GB_HD : 0;   
    port_mode |= (port_abil & SOC_PA_SPEED_16GB) ? SOC_PM_16GB_HD : 0;   

    /* Full duplex speeds */
    port_abil = ability->speed_full_duplex;
    port_mode |= (port_abil & SOC_PA_SPEED_10MB) ? SOC_PM_10MB_FD : 0;   
    port_mode |= (port_abil & SOC_PA_SPEED_100MB) ? SOC_PM_100MB_FD : 0;  
    port_mode |= (port_abil & SOC_PA_SPEED_1000MB) ? SOC_PM_1000MB_FD : 0;
    port_mode |= (port_abil & SOC_PA_SPEED_2500MB) ? SOC_PM_2500MB_FD : 0;
    port_mode |= (port_abil & SOC_PA_SPEED_3000MB) ? SOC_PM_3000MB_FD : 0;
    port_mode |= (port_abil & SOC_PA_SPEED_10GB) ? SOC_PM_10GB_FD : 0;   
    port_mode |= (port_abil & SOC_PA_SPEED_12GB) ? SOC_PM_12GB_FD : 0;   
    port_mode |= (port_abil & SOC_PA_SPEED_13GB) ? SOC_PM_13GB_FD : 0;   
    port_mode |= (port_abil & SOC_PA_SPEED_16GB) ? SOC_PM_16GB_FD : 0;   

    /* Pause Modes */
    port_abil = ability->pause;
    port_mode |= (port_abil & SOC_PA_PAUSE_TX)? SOC_PM_PAUSE_TX : 0;
    port_mode |= (port_abil & SOC_PA_PAUSE_RX) ? SOC_PM_PAUSE_RX : 0;
    port_mode |= (port_abil & SOC_PA_PAUSE_ASYMM) ? SOC_PM_PAUSE_ASYMM : 0;

    /* Interface Types */
    port_abil = ability->interface;
    port_mode |= (port_abil & SOC_PA_INTF_TBI) ? SOC_PM_TBI : 0;
    port_mode |= (port_abil & SOC_PA_INTF_MII) ? SOC_PM_MII : 0;
    port_mode |= (port_abil & SOC_PA_INTF_GMII) ? SOC_PM_GMII : 0;
    port_mode |= (port_abil & SOC_PA_INTF_SGMII) ? SOC_PM_SGMII : 0; 
    port_mode |= (port_abil & SOC_PA_INTF_XGMII) ? SOC_PM_XGMII : 0;

    /* Loopback port_abil */
    port_abil = ability->loopback;
    port_mode |= (port_abil & SOC_PA_LB_MAC) ? SOC_PM_LB_MAC : 0;
    port_mode |= (port_abil & SOC_PA_LB_PHY) ? SOC_PM_LB_PHY : 0;
    port_mode |= (port_abil & SOC_PA_LB_NONE) ? SOC_PM_LB_NONE : 0;

    /* Remaining Flags */
    port_abil = ability->flags;
    port_mode |= (port_abil & SOC_PA_AUTONEG) ? SOC_PM_AN : 0; 
    port_mode |= (port_abil & SOC_PA_COMBO) ? SOC_PM_COMBO : 0;

    *mode = port_mode;
    return (SOC_E_NONE);
}

int
soc_port_phy_pll_os_set(int unit, soc_port_t port, uint32 vco_freq, uint32 oversample_mode, uint32 pll_divider)
{
    if(!SOC_PORT_VALID(unit, port)) {
        return SOC_E_PORT;
    }
    
    SOC_IF_ERROR_RETURN(soc_phyctrl_control_set(unit, port, SOC_PHY_CONTROL_VCO_FREQ, vco_freq));
    SOC_IF_ERROR_RETURN(soc_phyctrl_control_set(unit, port, SOC_PHY_CONTROL_OVERSAMPLE_MODE, oversample_mode));
    SOC_IF_ERROR_RETURN(soc_phyctrl_control_set(unit, port, SOC_PHY_CONTROL_PLL_DIVIDER, pll_divider));
    SOC_IF_ERROR_RETURN(soc_phyctrl_speed_set(unit, port, SOC_PHY_NON_CANNED_SPEED));
    
    return SOC_E_NONE;
}

