/*----------------------------------------------------------------------
 * $Id: tqmod_cfg_seq.c,v 1.1.2.4 Broadcom SDK $
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
 * Broadcom Corporation
 * Proprietary and Confidential information
 * All rights reserved
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without the
 * prior written consent of Broadcom Corporation.
 *---------------------------------------------------------------------
 * File       : tqmod_cfg_seq.c
 * Description: c functions implementing Tier1s for TEMod Serdes Driver
 *---------------------------------------------------------------------*/

#include <phymod/phymod.h>
#include <phymod/chip/bcmi_qsgmiie_serdes_defs.h> 
#include "tqmod_cfg_seq.h"


int tqmod_set_spd_intf(const phymod_access_t *pa, int speed)                  /* SET_SPD_INTF */
{
    BCMI_QSGMIIE_SERDES_MIICONTROLr_t reg_mii_ctrl;

    BCMI_QSGMIIE_SERDES_MIICONTROLr_CLR(reg_mii_ctrl);
    BCMI_QSGMIIE_SERDES_READ_MIICONTROLr(pa, &reg_mii_ctrl);
    
    switch(speed){
        case 10: 
            BCMI_QSGMIIE_SERDES_MIICONTROLr_MANUAL_SPEED_0f_SET(reg_mii_ctrl, 0);
            BCMI_QSGMIIE_SERDES_MIICONTROLr_MANUAL_SPEED_1f_SET(reg_mii_ctrl, 0);
            break;
        case 100: 
            BCMI_QSGMIIE_SERDES_MIICONTROLr_MANUAL_SPEED_0f_SET(reg_mii_ctrl, 1);
            BCMI_QSGMIIE_SERDES_MIICONTROLr_MANUAL_SPEED_1f_SET(reg_mii_ctrl, 0);
            break;
        case 1000: 
            BCMI_QSGMIIE_SERDES_MIICONTROLr_MANUAL_SPEED_0f_SET(reg_mii_ctrl, 0);
            BCMI_QSGMIIE_SERDES_MIICONTROLr_MANUAL_SPEED_1f_SET(reg_mii_ctrl, 1);
            break;
        default: 
            break;
    }
    PHYMOD_IF_ERR_RETURN (BCMI_QSGMIIE_SERDES_WRITE_MIICONTROLr(pa,reg_mii_ctrl));

    return PHYMOD_E_NONE;
}



/**
@brief   Initialize PMD. 
@param   pa handle to current phymod_access_t  
@param   pmd_touched Indicates this PMD was previously initialized for another
port in the same core.
@param   uc_active Actually means PMD config. will come from PCS and not TMod
@returns SOC_E_NONE upon success, SOC_E_ERROR else.
@details Per Port PMD Init
*/

int tqmod_init_pmd_qsgmii(const phymod_access_t *pa, int pmd_active, int uc_active)              /* INIT_PMD */
{
    BCMI_QSGMIIE_SERDES_TXFIR_MISC_CTL1r_t                  reg_misc1_ctrl;
    BCMI_QSGMIIE_SERDES_CKRST_LN_CLK_RST_N_PWRDWN_CTLr_t    reg_pwrdwn_ctrl;
    BCMI_QSGMIIE_SERDES_DIG_TOP_USER_CTL0r_t                reg_usr_ctrl;
    BCMI_QSGMIIE_SERDES_PLL_CAL_CTL7r_t                     reg_pll_cal_ctrl;
    BCMI_QSGMIIE_SERDES_CKRST_OSR_MODE_CTLr_t               reg_osr_mode_ctrl;
    BCMI_QSGMIIE_SERDES_PMD_X4_OVRRr_t                      reg_pmd_overr_ctrl;

    BCMI_QSGMIIE_SERDES_TXFIR_MISC_CTL1r_CLR(reg_misc1_ctrl);
    BCMI_QSGMIIE_SERDES_READ_TXFIR_MISC_CTL1r(pa, &reg_misc1_ctrl);
    BCMI_QSGMIIE_SERDES_CKRST_LN_CLK_RST_N_PWRDWN_CTLr_CLR(reg_pwrdwn_ctrl);
    BCMI_QSGMIIE_SERDES_READ_CKRST_LN_CLK_RST_N_PWRDWN_CTLr(pa, &reg_pwrdwn_ctrl);
    BCMI_QSGMIIE_SERDES_DIG_TOP_USER_CTL0r_CLR(reg_usr_ctrl);
    BCMI_QSGMIIE_SERDES_READ_DIG_TOP_USER_CTL0r(pa, &reg_usr_ctrl);
    BCMI_QSGMIIE_SERDES_PLL_CAL_CTL7r_CLR(reg_pll_cal_ctrl);
    BCMI_QSGMIIE_SERDES_READ_PLL_CAL_CTL7r(pa, &reg_pll_cal_ctrl);
    BCMI_QSGMIIE_SERDES_CKRST_OSR_MODE_CTLr_CLR(reg_osr_mode_ctrl);
    BCMI_QSGMIIE_SERDES_READ_CKRST_OSR_MODE_CTLr(pa, &reg_osr_mode_ctrl);
    BCMI_QSGMIIE_SERDES_PMD_X4_OVRRr_CLR(reg_pmd_overr_ctrl);
    BCMI_QSGMIIE_SERDES_READ_PMD_X4_OVRRr(pa, &reg_pmd_overr_ctrl);

    BCMI_QSGMIIE_SERDES_TXFIR_MISC_CTL1r_DP_RESET_TX_DISABLE_DISf_SET(reg_misc1_ctrl, 1);
    PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_WRITE_TXFIR_MISC_CTL1r(pa, reg_misc1_ctrl));

    BCMI_QSGMIIE_SERDES_CKRST_LN_CLK_RST_N_PWRDWN_CTLr_LN_DP_S_RSTBf_SET(reg_pwrdwn_ctrl, 1);
    PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_WRITE_CKRST_LN_CLK_RST_N_PWRDWN_CTLr(pa, reg_pwrdwn_ctrl));


    if (pmd_active == 0) {
        if(uc_active == 1){
             BCMI_QSGMIIE_SERDES_DIG_TOP_USER_CTL0r_UC_ACTIVEf_SET(reg_usr_ctrl, 1);
             BCMI_QSGMIIE_SERDES_DIG_TOP_USER_CTL0r_CORE_DP_S_RSTBf_SET(reg_usr_ctrl, 1);
             /* release reset to pll data path. TBD need for all lanes  and uc_active set */
             PHYMOD_IF_ERR_RETURN (BCMI_QSGMIIE_SERDES_WRITE_DIG_TOP_USER_CTL0r(pa, reg_usr_ctrl));
        } else{
            BCMI_QSGMIIE_SERDES_DIG_TOP_USER_CTL0r_CORE_DP_S_RSTBf_SET(reg_usr_ctrl, 1);
            PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_WRITE_DIG_TOP_USER_CTL0r(pa, reg_usr_ctrl));
        }
    }

    /* set vco at 10G */

    BCMI_QSGMIIE_SERDES_PLL_CAL_CTL7r_RESCAL_FRC_VALf_SET(reg_pll_cal_ctrl, 0x9);
    PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_WRITE_PLL_CAL_CTL7r(pa, reg_pll_cal_ctrl));
    /*
    if( pc->verbosity)
        printf("%-22s: plldiv:%d data:%x\n", __func__, pc->plldiv, data);

    if( pc->verbosity) 
        printf("%-22s: main0_setup=%x\n", __func__, data);
    */
  
    /* set OS2 mode */
    BCMI_QSGMIIE_SERDES_CKRST_OSR_MODE_CTLr_OSR_MODE_FRCf_SET(reg_osr_mode_ctrl,1);
    PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_WRITE_CKRST_OSR_MODE_CTLr(pa, reg_osr_mode_ctrl));
    
    /* Clear pmd_control reset by setting PMD tx_disable override enable*/
    BCMI_QSGMIIE_SERDES_PMD_X4_OVRRr_TX_DISABLE_OENf_SET(reg_pmd_overr_ctrl,1);
    BCMI_QSGMIIE_SERDES_WRITE_PMD_X4_OVRRr(pa, reg_pmd_overr_ctrl);

    return PHYMOD_E_NONE;
}

/**
@brief   Select the ILKN path and bypass TSCE PCS
@param   pa handle to current phymod_access_t
@returns The value SOC_E_NONE upon successful completion
@details This will enable ILKN path. PCS is set to bypass to relinquish PMD
control. Expect PMD to be programmed elsewhere. If we need the QSGMII PCS expect
the QSGMII to be already programmed at the port layer. If not, we will feed
garbage to the ILKN path.

<B>How to call:</B><br>
<p>Call directly or via #temod_tier1_selector() with enum INIT_PCS_ILKN</p>
*/
int tqmod_init_pcs_ilkn(const phymod_access_t *pa)              /* INIT_PCS */
{
    BCMI_QSGMIIE_SERDES_SC_X4_BYPASSr_t reg_bypass_ctrl;
    BCMI_QSGMIIE_SERDES_ILKN_CTL0r_t reg_serdes_ilkn_ctrl;

    BCMI_QSGMIIE_SERDES_SC_X4_BYPASSr_CLR(reg_bypass_ctrl);
    BCMI_QSGMIIE_SERDES_READ_SC_X4_BYPASSr(pa, &reg_bypass_ctrl);
    BCMI_QSGMIIE_SERDES_ILKN_CTL0r_CLR(reg_serdes_ilkn_ctrl);
    /*BCMI_QSGMIIE_SERDES_READ_ILKN_CTL0r(pa, &reg_serdes_ilkn_ctrl);*/

    BCMI_QSGMIIE_SERDES_SC_X4_BYPASSr_SC_BYPASSf_SET(reg_bypass_ctrl, 1);
    PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_WRITE_SC_X4_BYPASSr(pa, reg_bypass_ctrl));

    BCMI_QSGMIIE_SERDES_ILKN_CTL0r_ILKN_SELf_SET(reg_serdes_ilkn_ctrl, 1);
    BCMI_QSGMIIE_SERDES_ILKN_CTL0r_CREDIT_ENf_SET(reg_serdes_ilkn_ctrl, 1);
    PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_WRITE_ILKN_CTL0r(pa, reg_serdes_ilkn_ctrl));

    return PHYMOD_E_NONE;
}

/**
@brief  Get link status 
@param   pa handle to current phymod_access_t
@returns The value SOC_E_NONE upon successful completion
@details This will provide link status
<B>How to call:</B><br>
<p>Call directly or via #temod_tier1_selector() with enum GET_PCS_LINK_STATUS</p>
*/
int tqmod_get_pcs_link_status(const phymod_access_t *pa, uint32_t *link)
{
    BCMI_QSGMIIE_SERDES_MIISTATUSr_t reg_miistatus;

    BCMI_QSGMIIE_SERDES_MIISTATUSr_CLR(reg_miistatus);
    BCMI_QSGMIIE_SERDES_READ_MIISTATUSr(pa, &reg_miistatus);
    *link = BCMI_QSGMIIE_SERDES_MIISTATUSr_LINK_STATUSf_GET(reg_miistatus);

    return PHYMOD_E_NONE;
}

/**
@brief  Get  actual speed
@param   pa handle to current phymod_access_t
@returns The value SOC_E_NONE upon successful completion
@details This will provide actual speed 
<B>How to call:</B><br>
<p>Call directly or via #temod_tier1_selector() with enum GET_PCS_LINK_STATUS</p>
*/

int tqmod_speed_id_get(const phymod_access_t *pa, int *speed_id)
{
    BCMI_QSGMIIE_SERDES_R1000XSTATUS1r_t reg_1000x_status;
    BCMI_QSGMIIE_SERDES_R1000XSTATUS1r_CLR(reg_1000x_status);
    BCMI_QSGMIIE_SERDES_READ_R1000XSTATUS1r(pa, &reg_1000x_status);
    *speed_id = BCMI_QSGMIIE_SERDES_R1000XSTATUS1r_SPEED_STATUSf_GET(reg_1000x_status);
    return PHYMOD_E_NONE;
}

/**
@brief  Enable/disable port
@param   pa handle to current phymod_access_t
@returns The value SOC_E_NONE upon successful completion
@details This will set/unset powersave mode for each port 
<B>How to call:</B><br>
*/

int tqmod_lane_enable_set(const phymod_access_t *pa, int enable)
{
    BCMI_QSGMIIE_SERDES_MIICONTROLr_t reg_mii_ctrl;

    BCMI_QSGMIIE_SERDES_MIICONTROLr_CLR(reg_mii_ctrl);
    BCMI_QSGMIIE_SERDES_READ_MIICONTROLr(pa, &reg_mii_ctrl);
    if(enable)
       BCMI_QSGMIIE_SERDES_MIICONTROLr_PWRDWN_SWf_SET(reg_mii_ctrl, 0);
    else
       BCMI_QSGMIIE_SERDES_MIICONTROLr_PWRDWN_SWf_SET(reg_mii_ctrl, 1);

    PHYMOD_IF_ERR_RETURN (BCMI_QSGMIIE_SERDES_WRITE_MIICONTROLr(pa,reg_mii_ctrl));
    return PHYMOD_E_NONE;
}


/**
@brief   Set Per lane OS mode set in PMD
@param   pa handle to current phymod_access_t
@param   os_mode over sample rate.
@returns The value PHYMOD_E_NONE upon successful completion
@details Per Port PMD Init

*/
int tqmod_pmd_osmode_set(const phymod_access_t *pa, int os_mode)   /* INIT_PMD */
{
  BCMI_QSGMIIE_SERDES_CKRST_OSR_MODE_CTLr_t reg_osr_mode;

  BCMI_QSGMIIE_SERDES_CKRST_OSR_MODE_CTLr_CLR(reg_osr_mode);
  BCMI_QSGMIIE_SERDES_READ_CKRST_OSR_MODE_CTLr(pa, &reg_osr_mode);

  /*os_mode         = 0x0; */ /* 0= OS MODE 1;  1= OS MODE 2; 2=OS MODE 3; 
                             3=OS MODE 3.3; 4=OS MODE 4; 5=OS MODE 5; 
                             6=OS MODE 8;   7=OS MODE 8.25; 8: OS MODE 10*/
  /*if(os_mode & 0x80000000) {*/
    os_mode =  (os_mode) & 0x0000ffff;
  /*}*/

  /* set OS2 mode */
  BCMI_QSGMIIE_SERDES_CKRST_OSR_MODE_CTLr_OSR_MODE_FRCf_SET(reg_osr_mode,1);
  BCMI_QSGMIIE_SERDES_CKRST_OSR_MODE_CTLr_OSR_MODE_FRC_VALf_SET(reg_osr_mode, os_mode);

  PHYMOD_IF_ERR_RETURN
     (BCMI_QSGMIIE_SERDES_WRITE_CKRST_OSR_MODE_CTLr(pa, reg_osr_mode));

  return PHYMOD_E_NONE;
}

/*!
@brief Controls the setting/resetting of autoneg advertisement registers.
@param   pa handle to current phymod_access_t
@param   os_mode over sample rate.
@returns The value PHYMOD_E_NONE upon successful completion
@details This function modifies the following registers:
*/

int tqmod_autoneg_set(const phymod_access_t *pa, int *an_en)               /* AUTONEG_SET */
{
  BCMI_QSGMIIE_SERDES_MIICONTROLr_t reg_mii_ctrl;

  BCMI_QSGMIIE_SERDES_MIICONTROLr_CLR(reg_mii_ctrl);
  BCMI_QSGMIIE_SERDES_READ_MIICONTROLr(pa, &reg_mii_ctrl);
    
  BCMI_QSGMIIE_SERDES_MIICONTROLr_AUTONEG_ENABLEf_SET(reg_mii_ctrl, *an_en);

  PHYMOD_IF_ERR_RETURN (BCMI_QSGMIIE_SERDES_WRITE_MIICONTROLr(pa,reg_mii_ctrl));
  return PHYMOD_E_NONE;
}

/**
@brief   PMD per lane reset
@param   pa handle to current TSCE context (#const phymod_access_t)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per lane PMD ln_rst and ln_dp_rst by writing to PMD_X4_CONTROL in pcs space
*/
int tqmod_pmd_x4_reset(const phymod_access_t *pa)              /* PMD_X4_RESET */
{
  BCMI_QSGMIIE_SERDES_PMD_X4_CTLr_t reg_pmd_x4_ctrl;

  /*TQMOD_DBG_IN_FUNC_INFO(pa);*/
  BCMI_QSGMIIE_SERDES_PMD_X4_CTLr_CLR(reg_pmd_x4_ctrl);
  BCMI_QSGMIIE_SERDES_PMD_X4_CTLr_LN_H_RSTBf_SET(reg_pmd_x4_ctrl,0);
  BCMI_QSGMIIE_SERDES_PMD_X4_CTLr_LN_DP_H_RSTBf_SET(reg_pmd_x4_ctrl,0);
  PHYMOD_IF_ERR_RETURN (BCMI_QSGMIIE_SERDES_MODIFY_PMD_X4_CTLr(pa, reg_pmd_x4_ctrl));
  
  BCMI_QSGMIIE_SERDES_PMD_X4_CTLr_CLR(reg_pmd_x4_ctrl);
  BCMI_QSGMIIE_SERDES_PMD_X4_CTLr_LN_H_RSTBf_SET(reg_pmd_x4_ctrl,1);
  BCMI_QSGMIIE_SERDES_PMD_X4_CTLr_LN_DP_H_RSTBf_SET(reg_pmd_x4_ctrl,1);
  PHYMOD_IF_ERR_RETURN (BCMI_QSGMIIE_SERDES_MODIFY_PMD_X4_CTLr(pa, reg_pmd_x4_ctrl));

  return PHYMOD_E_NONE;
}



/**
@brief   rx lane reset and enable of any particular lane
@param   pa handle to current TSCE context (#const phymod_access_t)
@param   enable to reset the lane.
@returns The value PHYMOD_E_NONE upon successful completion.

This function enables/disables rx lane (RSTB_LANE) or read back control bit for
that based on per_lane_control being 1 or 0. If per_lane_control is 0xa, only
read back RSTB_LANE.

*/
int tqmod_rx_lane_control_set(const phymod_access_t* pa, int enable)         /* RX_LANE_CONTROL */
{
  BCMI_QSGMIIE_SERDES_RX_X4_PMA_CTL0r_t reg_pma_ctrl;

  /*TQMOD_DBG_IN_FUNC_INFO(pa);*/
  BCMI_QSGMIIE_SERDES_RX_X4_PMA_CTL0r_CLR(reg_pma_ctrl);
  if (enable) {
    BCMI_QSGMIIE_SERDES_RX_X4_PMA_CTL0r_RSTB_LANEf_SET(reg_pma_ctrl, 1);
    PHYMOD_IF_ERR_RETURN (BCMI_QSGMIIE_SERDES_MODIFY_RX_X4_PMA_CTL0r(pa, reg_pma_ctrl));
  } else {
     /* bit set to 0 for disabling RXP */
    BCMI_QSGMIIE_SERDES_RX_X4_PMA_CTL0r_RSTB_LANEf_SET( reg_pma_ctrl, 0);
    PHYMOD_IF_ERR_RETURN (BCMI_QSGMIIE_SERDES_MODIFY_RX_X4_PMA_CTL0r(pa, reg_pma_ctrl));
  }
  return PHYMOD_E_NONE;
}


/*!
@brief   tx lane reset and enable of any particular lane
@param   pa handle to current TSCE context (#const phymod_access_t)
@param   enable Enable/disable traffic or reset/unreset lane
@param   tx_dis_type enum type #txq_lane_disable_type_t
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function is used to reset tx lane and enable/disable tx lane of any
transmit lane.  Set enable to enable the TX_LANE (1) or disable the TX lane (0).
When diable TX lane, two types of operations are invoked independently.  

If enable bit [7:4] = 1, only traffic is disabled. 
      (TEMOD_TX_LANE_TRAFFIC =0x10)
If bit [7:4] = 2, only reset function is invoked.
      (TEMOD_TX_LANE_RESET = 0x20)
*/
int tqmod_tx_lane_control(const phymod_access_t* pa, int enable, txq_lane_disable_type_t tx_dis_type)         /* TX_LANE_CONTROL */
{
  BCMI_QSGMIIE_SERDES_TX_X4_MISCr_t reg_misc_ctrl;

  /*TQMOD_DBG_IN_FUNC_INFO(pa);*/
  BCMI_QSGMIIE_SERDES_TX_X4_MISCr_CLR(reg_misc_ctrl);
  /*TEMOD_DBG_IN_FUNC_VIN_INFO(pa,("enable: %d, tx_dis_type: %d", enable, tx_dis_type));*/


  if (enable) {
    /* set encoder */
    BCMI_QSGMIIE_SERDES_TX_X4_MISCr_CLR(reg_misc_ctrl);
    BCMI_QSGMIIE_SERDES_TX_X4_MISCr_RSTB_TX_LANEf_SET(reg_misc_ctrl, 1);
    PHYMOD_IF_ERR_RETURN (BCMI_QSGMIIE_SERDES_MODIFY_TX_X4_MISCr(pa, reg_misc_ctrl));
    BCMI_QSGMIIE_SERDES_TX_X4_MISCr_CLR(reg_misc_ctrl);
    BCMI_QSGMIIE_SERDES_TX_X4_MISCr_ENABLE_TX_LANEf_SET(reg_misc_ctrl, 1);
    PHYMOD_IF_ERR_RETURN (BCMI_QSGMIIE_SERDES_MODIFY_TX_X4_MISCr(pa, reg_misc_ctrl));
  } else {
    /* data = 0 ;  */
    if(tx_dis_type & TEMOD_TX_LANE_TRAFFIC ){
      BCMI_QSGMIIE_SERDES_TX_X4_MISCr_ENABLE_TX_LANEf_SET(reg_misc_ctrl, 0);
    } else if (tx_dis_type & TEMOD_TX_LANE_RESET) {
      BCMI_QSGMIIE_SERDES_TX_X4_MISCr_RSTB_TX_LANEf_SET(reg_misc_ctrl, 0);
    } else {
      BCMI_QSGMIIE_SERDES_TX_X4_MISCr_ENABLE_TX_LANEf_SET(reg_misc_ctrl, 0);
      BCMI_QSGMIIE_SERDES_TX_X4_MISCr_RSTB_TX_LANEf_SET(reg_misc_ctrl, 0);
    }
     PHYMOD_IF_ERR_RETURN (BCMI_QSGMIIE_SERDES_MODIFY_TX_X4_MISCr(pa, reg_misc_ctrl));
  }
  return PHYMOD_E_NONE;
}


/**
@brief   Gets the TX And RX Polarity 
@param   pa handle to current TSCE context (#const phymod_access_t)
@param   tx_polarity Receive Polarity for TX side
@param   rx_polarity Receive Polarity for RX side
@returns The value PHYMOD_E_NONE upon successful completion.
@details Gets the TX And RX Polarity
*/
int tqmod_tx_rx_polarity_get ( const phymod_access_t *pa, uint32_t* tx_polarity, uint32_t* rx_polarity) 
{
  BCMI_QSGMIIE_SERDES_TLB_TX_TLB_TX_MISC_CFGr_t tx_pol_inv;
  BCMI_QSGMIIE_SERDES_TLB_RX_TLB_RX_MISC_CFGr_t rx_pol_inv;

  PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_READ_TLB_TX_TLB_TX_MISC_CFGr(pa, &tx_pol_inv));
  *tx_polarity = BCMI_QSGMIIE_SERDES_TLB_TX_TLB_TX_MISC_CFGr_TX_PMD_DP_INVERTf_GET(tx_pol_inv);

  PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_READ_TLB_RX_TLB_RX_MISC_CFGr(pa, &rx_pol_inv));
  *rx_polarity = BCMI_QSGMIIE_SERDES_TLB_RX_TLB_RX_MISC_CFGr_RX_PMD_DP_INVERTf_GET(rx_pol_inv);
 
  return PHYMOD_E_NONE;
}

/**
@brief   Sets the TX And RX Polarity 
@param   pa handle to current TSCE context (#const phymod_access_t)
@param   tx_polarity Control Polarity for TX side
@param   rx_polarity Control Polarity for RX side
@returns The value PHYMOD_E_NONE upon successful completion.
@details Sets the TX And RX Polarity
*/
int tqmod_tx_rx_polarity_set ( const phymod_access_t *pa, uint32_t tx_polarity, uint32_t rx_polarity) 
{
  BCMI_QSGMIIE_SERDES_TLB_TX_TLB_TX_MISC_CFGr_t tx_pol_inv;
  BCMI_QSGMIIE_SERDES_TLB_RX_TLB_RX_MISC_CFGr_t rx_pol_inv;
  
  BCMI_QSGMIIE_SERDES_TLB_TX_TLB_TX_MISC_CFGr_CLR(tx_pol_inv);
  BCMI_QSGMIIE_SERDES_TLB_RX_TLB_RX_MISC_CFGr_CLR(rx_pol_inv);

  BCMI_QSGMIIE_SERDES_TLB_TX_TLB_TX_MISC_CFGr_TX_PMD_DP_INVERTf_SET(tx_pol_inv, tx_polarity);
  PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_MODIFY_TLB_TX_TLB_TX_MISC_CFGr(pa, tx_pol_inv));

  BCMI_QSGMIIE_SERDES_TLB_RX_TLB_RX_MISC_CFGr_RX_PMD_DP_INVERTf_SET(rx_pol_inv, rx_polarity);
  PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_MODIFY_TLB_RX_TLB_RX_MISC_CFGr(pa, rx_pol_inv));

  return PHYMOD_E_NONE;
}


/**
@brief   Enable TX 
@param   pa handle to current TSCE context (#const phymod_access_t)
@param   tx_polarity Control Polarity for TX side
@param   rx_polarity Control Polarity for RX side
@returns The value PHYMOD_E_NONE upon successful completion.
@details Sets tx disable bit to 0 (enable tx path)
*/
int tqmod_txfir_tx_disable_set(const phymod_access_t *pa) 
{
  BCMI_QSGMIIE_SERDES_TXFIR_MISC_CTL1r_t txfir_misc_ctrl1;
  
  BCMI_QSGMIIE_SERDES_TXFIR_MISC_CTL1r_CLR(txfir_misc_ctrl1);


  BCMI_QSGMIIE_SERDES_TXFIR_MISC_CTL1r_PMD_TX_DISABLE_PIN_DISf_SET(txfir_misc_ctrl1, 1);
  PHYMOD_IF_ERR_RETURN(BCMI_QSGMIIE_SERDES_WRITE_TXFIR_MISC_CTL1r(pa, txfir_misc_ctrl1));

  return PHYMOD_E_NONE;
}


