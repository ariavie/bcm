/*----------------------------------------------------------------------
 * $Id: temod_cfg_seq.c,v 1.6 Broadcom SDK $
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
 * File       : temod_cfg_seq.c
 * Description: c functions implementing Tier1s for TEMod Serdes Driver
 *---------------------------------------------------------------------*/

#ifndef _DV_TB_
#define _SDK_TEMOD_ 1 
#endif

#ifdef _DV_TB_
#include <stdint.h>
#include "temod_main.h"
#include "temod_defines.h"
#include "tePCSRegEnums.h"
#include "tePMDRegEnums.h"
#include "phy_tsc_iblk.h"
#include "bcmi_tsce_xgxs_defs.h"
#endif /* _DV_TB_ */

#ifdef _SDK_TEMOD_
#include <phymod/phymod.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_debug.h>
#include <phymod/chip/bcmi_tsce_xgxs_defs.h>
#include "temod_enum_defines.h"
#include "temod.h"
#include "temod_device.h"
#include "temod_sc_lkup_table.h"
#include "tePCSRegEnums.h"
#endif /* _SDK_TEMOD_ */

#ifndef PHYMOD_ST  
#ifdef _SDK_TEMOD_
  #define PHYMOD_ST  const phymod_access_t
#else
  #define PHYMOD_ST  temod_st
#endif /* _SDK_TEMOD_ */
#endif /* PHYMOD_ST */

#ifdef _SDK_TEMOD_
  #define TEMOD_DBG_IN_FUNC_INFO(pc) \
    PHYMOD_VDBG(TEMOD_DBG_FUNC,pc,("-22%s: Adr:%08x Ln:%02d\n", __func__, pc->addr, pc->lane))
  #define TEMOD_DBG_IN_FUNC_VIN_INFO(pc,_print_) \
    PHYMOD_VDBG(TEMOD_DBG_FUNCVALIN,pc,_print_)
  #define TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,_print_) \
    PHYMOD_VDBG(TEMOD_DBG_FUNCVALOUT,pc,_print_)
#endif

#ifdef _DV_TB_
  #define TEMOD_DBG_IN_FUNC_INFO(pc) \
    PHYMOD_VDBG(TEMOD_DBG_FUNC, pc, \
      ("TEMOD IN Function : %s Port Add : %d Lane No: %d\n", \
      __func__, pc->prt_ad, pc->this_lane))
  #define TEMOD_DBG_IN_FUNC_VIN_INFO(pc,_print_) \
    PHYMOD_VDBG(TEMOD_DBG_FUNCVALIN,pc,_print_)
  #define TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,_print_) \
    PHYMOD_VDBG(TEMOD_DBG_FUNCVALOUT,pc,_print_)
int phymod_debug_check(uint32_t flags, PHYMOD_ST *pc);
#endif

static eagle_sc_pmd_entry_st eagle_sc_pmd_entry[] = {

/*SPD_10M              0*/ { 1, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  1},
/*SPD_10M              1*/ { 1, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  1},
/*SPD_100M             2*/ { 1, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  1},
/*SPD_1000M            3*/ { 1, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  1},
/*SPD_1G_CX1           4*/ { 1, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  1},
/*SPD_1G_KX1           5*/ { 1, TEMOD_PMA_OS_MODE_8_25, TEMOD_PLL_MODE_DIV_66,  0, 0,  1},
/*SPD_2p5G             6*/ { 1, TEMOD_PMA_OS_MODE_2,  TEMOD_PLL_MODE_DIV_40,  0, 0,  1},
/*SPD_5G_X1            7*/ { 1, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_40,  1, 0,  0},
/*SPD_10G_CX4          8*/ { 4, TEMOD_PMA_OS_MODE_2,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_10G_KX4          9*/ { 4, TEMOD_PMA_OS_MODE_2,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_10G_X4          10*/ { 4, TEMOD_PMA_OS_MODE_2,  TEMOD_PLL_MODE_DIV_40,  0, 0,  1},
/*SPD_13G_X4          11*/ { 4, TEMOD_PMA_OS_MODE_2,  TEMOD_PLL_MODE_DIV_52,  0, 0,  1},
/*SPD_15G_X4          12*/ { 4, TEMOD_PMA_OS_MODE_2,  TEMOD_PLL_MODE_DIV_60,  0, 0,  1},
/*SPD_16G_X4          13*/ { 4, TEMOD_PMA_OS_MODE_2,  TEMOD_PLL_MODE_DIV_64,  0, 0,  1},
/*SPD_20G_CX4         14*/ { 4, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_40,  0, 0,  1},
/*SPD_10G_CX2         15*/ { 2, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_40,  0, 0,  1},
/*SPD_10G_X2          16*/ { 2, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_40,  1, 0,  1},
/*SPD_20G_X4          17*/ { 4, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_40,  1, 0,  1},
/*SPD_10p5G_X2        18*/ { 2, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_42,  1, 0,  0},
/*SPD_21G_X4          19*/ { 4, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_42,  1, 0,  0},
/*SPD_12p7G_X2        20*/ { 2, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_42,  1, 0,  0},
/*SPD_25p45G_X4       21*/ { 4, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_42,  1, 0,  0},
/*SPD_15p75G_X2       22*/ { 2, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_52,  1, 0,  0},
/*SPD_31p5G_X4        23*/ { 4, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_52,  1, 0,  0},
/*SPD_31p5G_KR4       24*/ { 4, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_52,  1, 0,  0},
/*SPD_20G_CX2         25*/ { 2, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_66,  1, 0,  0},
/*SPD_20G_X2          26*/ { 2, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_66,  1, 0,  0},
/*SPD_40G_X4          27*/ { 4, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_66,  1, 0,  0},
/*SPD_10G_KR1         28*/ { 0, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_66,  1, 0,  0},
/*SPD_10p6_X1         29*/ { 0, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_70,  1, 0,  0},
/*SPD_20G_KR2         30*/ { 2, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_66,  1, 0,  0},
/*SPD_20G_CR2         31*/ { 2, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_66,  1, 0,  0},
/*SPD_21G_X2          32*/ { 2, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_70,  1, 0,  0},
/*SPD_40G_KR4         33*/ { 4, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_66,  1, 0,  0},
/*SPD_40G_CR4         34*/ { 4, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_66,  1, 0,  0},
/*SPD_42G_X4          35*/ { 4, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_70,  1, 0,  0},
/*SPD_100G_CR10       36*/ {10, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_66,  1, 0,  0},
/*SPD_107G_CR10       37*/ {10, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_70,  1, 0,  0},
/*SPD_120G_X12        38*/ {12, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_66,  1, 0,  0},
/*SPD_127G_X12        39*/ {12, TEMOD_PMA_OS_MODE_1,  TEMOD_PLL_MODE_DIV_70,  1, 0,  0},
/*SPD_ILLEGAL         40*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_ILLEGAL         41*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_ILLEGAL         42*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_ILLEGAL         43*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_ILLEGAL         44*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_ILLEGAL         45*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_ILLEGAL         46*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_ILLEGAL         47*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_ILLEGAL         48*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_5G_KR1          49*/ { 0, TEMOD_PMA_OS_MODE_2,  TEMOD_PLL_MODE_DIV_66,  0, 0,  0},
/*SPD_10p5G_X4        50*/ { 4, TEMOD_PMA_OS_MODE_2,  TEMOD_PLL_MODE_DIV_42,  0, 0,  0},
/*SPD_ILLEGAL         51*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_ILLEGAL         52*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0},
/*SPD_10M_10p3125     53*/ { 1, TEMOD_PMA_OS_MODE_8_25, TEMOD_PLL_MODE_DIV_66,  0, 0,  0},
/*SPD_100M_10p3125    54*/ { 1, TEMOD_PMA_OS_MODE_8_25, TEMOD_PLL_MODE_DIV_66,  0, 0,  0},
/*SPD_1000M_10p3125   55*/ { 1, TEMOD_PMA_OS_MODE_8_25, TEMOD_PLL_MODE_DIV_66,  0, 0,  0},
/*SPD_2p5G_X1_10p3125 56*/ { 1, TEMOD_PMA_OS_MODE_3_3,TEMOD_PLL_MODE_DIV_66,  0, 0,  0},
/*SPD_ILLEGAL         57*/ { 0, TEMOD_PMA_OS_MODE_5,  TEMOD_PLL_MODE_DIV_40,  0, 0,  0}

};

int _temod_wait_sc_stats_set(PHYMOD_ST* pc);

#ifdef _DV_TB_
int _configure_sc_speed_configuration(PHYMOD_ST* pc);
int _init_pcs_fs(PHYMOD_ST* pc);
int _init_pcs_an(PHYMOD_ST* pc);
int _configure_st_entry(int st_entry_num, int st_entry_speed, PHYMOD_ST* pc);
int temod_set_an_port_mode(PHYMOD_ST* pc);
int temod_autoneg_set(PHYMOD_ST* pc);
int temod_autoneg_control(PHYMOD_ST* pc);
int temod_clause72_control(PHYMOD_ST* pc, cl72_type_t cl_72_type);
int temod_prbs_check(PHYMOD_ST* pc, int real_check, int *prbs_status);
int temod_prbs_mode(PHYMOD_ST* pc, int port, int prbs_inv, int pat, int check_mode);
int temod_prbs_control(PHYMOD_ST* pc, int prbs_enable);
int temod_prbs_rx_enable_set(PHYMOD_ST* pc, int enable);
int temod_prbs_tx_enable_set(PHYMOD_ST* pc, int enable);
int temod_prbs_rx_invert_data_set(PHYMOD_ST* pc, int invert_data);
int temod_prbs_rx_check_mode_set(PHYMOD_ST* pc, int check_mode);
int temod_prbs_tx_invert_data_set(PHYMOD_ST* pc, int invert_data);
int temod_prbs_tx_polynomial_set(PHYMOD_ST* pc, eagle_prbs_polynomial_type_t prbs_polynomial);
int temod_prbs_rx_polynomial_set(PHYMOD_ST* pc, eagle_prbs_polynomial_type_t prbs_polynomial);
int temod_pmd_addr_lane_swap(PHYMOD_ST *pc, uint32_t addr_lane_index);
int temod_check_sc_stats(PHYMOD_ST* pc);
#endif /* _DV_TB_ */

/*!
@brief   This function returns lock status of PLL
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   lockStatus reference which is updated with lock status.
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details This function reads TX-PLL PLL_LOCK bit to get the PLL lock status.
Returns 1 or 0 (locked/not).
*/

int temod_pll_lock_get(PHYMOD_ST* pc, int* lockStatus)
{
  PMD_X1_STSr_t  reg_pmd_x1_sts;
  TEMOD_DBG_IN_FUNC_INFO(pc);
  READ_PMD_X1_STSr(pc, &reg_pmd_x1_sts);

  *lockStatus =  PMD_X1_STSr_PLL_LOCK_STSf_GET(reg_pmd_x1_sts);
  TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("PLL lockStatus: %d", *lockStatus));
  return PHYMOD_E_NONE;
}

/**
@brief   This function  returns the locked status of PMD.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   lockStatus reference which is updated with pmd_lock status
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details This function reads TX-PMD PMD_LOCK bit to get the locked status of
PMD. PMD lock status also indicates CDR (clock recovered from data received).
Returns 1 or 0 (locked/not)
*/

int temod_pmd_lock_get(PHYMOD_ST* pc, uint32_t* lockStatus)
{
  PMD_X1_STSr_t  reg_pmd_x1_sts;
  TEMOD_DBG_IN_FUNC_INFO(pc);
  READ_PMD_X1_STSr(pc, &reg_pmd_x1_sts);

  *lockStatus =  PMD_X1_STSr_TX_CLK_VLD_STSf_GET(reg_pmd_x1_sts);
  TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("PMD lockStatus: %d", *lockStatus));

  return PHYMOD_E_NONE;
}

#ifdef _DV_TB_
/*
@brief   This function waits for TX-PLL PLL_LOCK bit. The register is one copy
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Wait for PLL to lock. Cannot be used in SDK.
*/

int temod_pll_lock_wait(PHYMOD_ST* pc, int timeOutValue)
{
   int rv;
   TEMOD_DBG_IN_FUNC_INFO(pc);
   TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("pll_lock_wait timeOutValue: %d", timeOutValue));

   rv = temod_regbit_set_wait_check(pc,
                 0x9012,
                 1, 1, timeOutValue); 
                 /* TBD PMD_X1_STATUS_PLL_LOCK_STS_MASK, 1, timeOutValue); */
   if (rv == SOC_E_TIMEOUT) {
     PHYMOD_DEBUG_ERROR(("%-22s ERROR: p=%0d Timeout PLL lock:\n", __func__, pc->port)); 
     return (SOC_E_TIMEOUT);
   }
  return PHYMOD_E_NONE;
}
#endif

#ifdef _DV_TB_
/*
@brief   This function waits for the the RX-PMD to lock.
@param  pc handle to current TSCE context (#PHYMOD_ST)
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details  This function has a time out limit (TE_PLL_WAIT*50) micro-second in driver.
or TE_PLL_WAIT/20 pulling count in simualtion, if per_lane_control is set.  Otherwise, 
if per_lane_control==0, it reads back PMD_LIVE_STATUS and put it on accData.          
*/
int temod_wait_pmd_lock(PHYMOD_ST* pc, int timeOutValue, int* lockStatus)
{
   int rv; 
   PMD_X4_STSr_t reg_pmd_x4_stats;
   TEMOD_DBG_IN_FUNC_INFO(pc);
   TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("pmd lock wait timeOutValue: %d", timeOutValue));

   PMD_X4_STSr_CLR(reg_pmd_x4_stats);

   if(pc->per_lane_control) {
      rv = temod_regbit_set_wait_check(pc, 0xc012, 1,1,timeOutValue);
             /* TBD PMD_X4_STATUS_RX_LOCK_STS_MASK,1,timeOutValue); */

      if (rv == SOC_E_TIMEOUT) {
        PHYMOD_DEBUG_ERROR(("%-22s ERROR: P=%0d Timeout RX PMD lock:\n", __func__, pc->port));
         pc->accData = 0;
         *lockStatus = 0;
         return rv;
      } else {
         pc->accData = 1;
        *lockStatus = 1;
      }
   } else {
      READ_PMD_X4_STSr(pc, &reg_pmd_x4_stats);
      if(PMD_X4_STSr_RX_LOCK_STSf_GET(reg_pmd_x4_stats)) {
         pc->accData = 1;
        *lockStatus = 1;
      } else {
         pc->accData = 0;
         *lockStatus = 0;
      }
   }
   TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("pmd lockStatus: %d", *lockStatus));

   return (PHYMOD_E_NONE);
}
#endif

#ifdef _DV_TB_
/**
@brief   controls PCS Bypass Control function.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   cntl Control value (bypass, un-bypass)
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Set cntl <B>0 to disable</B> PCS and <B>1 to enable</B> PCS When
bypassed, PCS relinquishes control of PMD resets. The PHY driver, TEMod can take
complete control of PMD programming. Bypass is also used in ILKN mode where the
PCS is not used.
*/

int temod_pcs_bypass_ctl (PHYMOD_ST* pc, int cntl)
{
  SC_X4_BYPASSr_t reg_sc_bypass;
  
  TEMOD_DBG_IN_FUNC_INFO(pc);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("Cntl is:%d",cntl));

  SC_X4_BYPASSr_CLR(reg_sc_bypass);

  if (cntl==TEMOD_SC_MODE_BYPASS) {
    SC_X4_BYPASSr_SC_BYPASSf_SET(reg_sc_bypass, 1);  
  } else {
    SC_X4_BYPASSr_SC_BYPASSf_SET(reg_sc_bypass, 0);  
  }

  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X4_BYPASSr( pc, reg_sc_bypass));

  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */


#ifdef _DV_TB_
/**
@brief   Controls PMD datapath resets.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   cntl Controls if PMD should be reset or unset
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Set cntl arg <B>0</B>:disable <B>1</B>: enable
 The PMD datapath is reset even if PCS is actively controlling PMD resets.
 There are two types of control. One per lane and one per Core.
*/
int temod_pmd_reset_bypass (PHYMOD_ST* pc, int cntl)     /* PMD_RESET_BYPASS */
{
  int lnCntl, coreCntl;
  lnCntl   = cntl & 0x10 ; 
  coreCntl = cntl & 0x1 ; 
  PMD_X4_OVRRr_t reg_pmd_x4_or;
  PMD_X4_CTLr_t  reg_pmd_x4_ctrl;
  PMD_X1_OVRRr_t reg_pmd_x1_or;
  PMD_X1_CTLr_t  reg_pmd_x1_ctrl;
  SC_X4_CTLr_t reg_sc_ctrl;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("Cntl is:%d",cntl));

  PMD_X4_OVRRr_CLR(reg_pmd_x4_or);
  PMD_X4_CTLr_CLR(reg_pmd_x4_ctrl);
  PMD_X1_OVRRr_CLR(reg_pmd_x1_or);
  PMD_X1_CTLr_CLR(reg_pmd_x1_ctrl);
  SC_X4_CTLr_CLR(reg_sc_ctrl);

  if (lnCntl) {
    PMD_X4_OVRRr_LN_DP_H_RSTB_OENf_SET(reg_pmd_x4_or, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, reg_pmd_x4_or));

    PMD_X4_CTLr_LN_DP_H_RSTBf_SET(reg_pmd_x4_ctrl, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, reg_pmd_x4_ctrl));

    /* toggle added */
    PMD_X4_OVRRr_CLR(reg_pmd_x4_or);
    PMD_X4_CTLr_CLR(reg_pmd_x4_ctrl);

    PMD_X4_CTLr_LN_DP_H_RSTBf_SET(reg_pmd_x4_ctrl, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, reg_pmd_x4_ctrl));

    PMD_X4_OVRRr_LN_DP_H_RSTB_OENf_SET(reg_pmd_x4_or, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, reg_pmd_x4_or));

  } else {
    PMD_X4_OVRRr_LN_DP_H_RSTB_OENf_SET(reg_pmd_x4_or, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, reg_pmd_x4_or));

    PMD_X4_CTLr_LN_DP_H_RSTBf_SET(reg_pmd_x4_ctrl, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, reg_pmd_x4_ctrl));
  }

  if (coreCntl) {
    PMD_X1_OVRRr_CORE_DP_H_RSTB_OENf_SET(reg_pmd_x1_or, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_OVRRr(pc, reg_pmd_x1_or));

    PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(reg_pmd_x1_ctrl, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_CTLr(pc, reg_pmd_x1_ctrl));

    /* toggle added */
    PMD_X1_OVRRr_CLR(reg_pmd_x1_or);
    PMD_X1_CTLr_CLR(reg_pmd_x1_ctrl);
    PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(reg_pmd_x1_ctrl, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_CTLr(pc, reg_pmd_x1_ctrl));

    PMD_X1_OVRRr_CORE_DP_H_RSTB_OENf_SET(reg_pmd_x1_or, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_OVRRr(pc, reg_pmd_x1_or));

  } else {
    PMD_X1_OVRRr_CORE_DP_H_RSTB_OENf_SET(reg_pmd_x1_or, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_OVRRr(pc, reg_pmd_x1_or));

    PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(reg_pmd_x1_ctrl, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_CTLr(pc, reg_pmd_x1_ctrl));
  }

  /* sw_speed_change toggled */
  SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg_sc_ctrl, 0);
  PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc, reg_sc_ctrl));

  SC_X4_CTLr_CLR(reg_sc_ctrl);
  SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg_sc_ctrl, 1);
  PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc, reg_sc_ctrl));

  return PHYMOD_E_NONE;

} /* PMD_RESET_BYPASS */
#endif

#ifdef _SDK_TEMOD_
/*!
@brief   Controls the initial setting/resetting of autoneg related registers.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   an_init_st structure with all the onr time autoneg init cfg.
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Get aneg features from upper software layers via #temod_an_init_t.
  and prepare the PHY for autoneg by populating the advertisements
  and autonegotiation modes. It does not start or control the autoneg process. That is done
  in #temod_autoneg_control
*/
int temod_autoneg_set_init(PHYMOD_ST* pc, temod_an_init_t *an_init_st) /* AUTONEG_SET */
{
  AN_X4_CTLSr_t      reg_an_ctrl;
  DIG_CTL1000X2r_t   reg_ctrl1000x2;
  AN_X4_LOC_DEV_CL73_BASE_ABIL1r_t reg_an_cl73_abl_1;
  AN_X4_LOC_DEV_CL73_BASE_ABIL0r_t    reg_cl73_abiilities;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  AN_X4_CTLSr_CLR(reg_an_ctrl);
  AN_X4_CTLSr_AN_FAIL_COUNT_LIMITf_SET(reg_an_ctrl, an_init_st->an_fail_cnt);
  AN_X4_CTLSr_OUI_CONTROLf_SET(reg_an_ctrl, an_init_st->an_oui_ctrl);
  AN_X4_CTLSr_LINKFAILTIMER_DISf_SET(reg_an_ctrl, an_init_st->linkfailtimer_dis);
  AN_X4_CTLSr_LINKFAILTIMERQUAL_ENf_SET(reg_an_ctrl, an_init_st->linkfailtimerqua_en);
  AN_X4_CTLSr_AN_GOOD_CHECK_TRAPf_SET(reg_an_ctrl, an_init_st->an_good_check_trap);
  AN_X4_CTLSr_AN_GOOD_TRAPf_SET(reg_an_ctrl,  an_init_st->an_good_trap);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_CTLSr(pc, reg_an_ctrl));

  DIG_CTL1000X2r_CLR(reg_ctrl1000x2);
  DIG_CTL1000X2r_DISABLE_REMOTE_FAULT_REPORTINGf_SET(reg_ctrl1000x2, an_init_st->disable_rf_report);
  PHYMOD_IF_ERR_RETURN(MODIFY_DIG_CTL1000X2r(pc, reg_ctrl1000x2));

  AN_X4_LOC_DEV_CL73_BASE_ABIL1r_CLR(reg_an_cl73_abl_1);
  AN_X4_LOC_DEV_CL73_BASE_ABIL1r_CL73_NONCE_MATCH_OVERf_SET(reg_an_cl73_abl_1, an_init_st->cl73_nonce_match_over);
  AN_X4_LOC_DEV_CL73_BASE_ABIL1r_CL73_NONCE_MATCH_VALf_SET(reg_an_cl73_abl_1, an_init_st->cl73_nonce_match_val);
  AN_X4_LOC_DEV_CL73_BASE_ABIL1r_BASE_SELECTORf_SET(reg_an_cl73_abl_1, an_init_st->base_selector);
  AN_X4_LOC_DEV_CL73_BASE_ABIL1r_TRANSMIT_NONCEf_SET(reg_an_cl73_abl_1, ( an_init_st->cl73_transmit_nonce & 0x1f));
  PHYMOD_IF_ERR_RETURN ( MODIFY_AN_X4_LOC_DEV_CL73_BASE_ABIL1r(pc, reg_an_cl73_abl_1));

  AN_X4_LOC_DEV_CL73_BASE_ABIL0r_CLR(reg_cl73_abiilities);
  AN_X4_LOC_DEV_CL73_BASE_ABIL0r_CL73_REMOTE_FAULTf_SET(reg_cl73_abiilities, ( an_init_st->cl73_remote_fault & 1)); 
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LOC_DEV_CL73_BASE_ABIL0r(pc, reg_cl73_abiilities));

  return PHYMOD_E_NONE;
} 
#endif

#ifdef _SDK_TEMOD_
/*!
@brief   Controls the setting/resetting of autoneg timers.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details This function gets specific aneg timer related information from upper
software layers via #PHYMOD_ST->an_tech_ability and updates the autoneg pages.
This does not start autonegotiation. That is done in #temod_autoneg_control
*/

int temod_autoneg_timer_init(PHYMOD_ST* pc)               /* AUTONEG timer set*/
{
  AN_X1_CL37_RESTARTr_t                reg_cl37_restart;
  AN_X1_CL37_ACKr_t                    reg_cl37_ack;
  AN_X1_CL73_BRK_LNKr_t                reg_cl73_break_link;
  AN_X1_CL73_DME_LOCKr_t               reg_cl73_dme_lock;
  AN_X1_LNK_UPr_t                      reg_link_up;
  AN_X1_LNK_FAIL_INHBT_TMR_CL72r_t     reg_inhibit_timer;
  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_t reg_inhibit_not_timer;
  AN_X1_PD_SD_TMRr_t                   reg_pd_sd_timer;
  AN_X1_CL37_SYNC_STS_FILTER_TMRr_t    reg_sync_status_filter_timer;
  AN_X1_PD_TO_CL37_LNK_WAIT_TMRr_t     reg_link_wait_timer;
  AN_X1_IGNORE_LNK_TMRr_t              reg_ignore_link_timer;
  AN_X1_DME_PAGE_TMRr_t                reg_dme_page_timer;
  AN_X1_SGMII_CL73_TMR_TYPEr_t         reg_sgmii_cl73_timer;
   
  TEMOD_DBG_IN_FUNC_INFO(pc);
  /*0x9250 AN_X1_TIMERS_cl37_restart */
  AN_X1_CL37_RESTARTr_CLR(reg_cl37_restart);
  AN_X1_CL37_RESTARTr_SET(reg_cl37_restart, 0x29a );
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL37_RESTARTr(pc, reg_cl37_restart));

  /*0x9251 AN_X1_TIMERS_cl37_ack */
  AN_X1_CL37_ACKr_CLR(reg_cl37_ack);
  AN_X1_CL37_ACKr_SET(reg_cl37_ack,  0x29a);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL37_ACKr(pc, reg_cl37_ack));

   /*0x9253 AN_X1_TIMERS_cl73_break_link */
  AN_X1_CL73_BRK_LNKr_CLR(reg_cl73_break_link);
  AN_X1_CL73_BRK_LNKr_SET(reg_cl73_break_link, 0x10ed);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL73_BRK_LNKr(pc, reg_cl73_break_link));

/*TBD::0x9255 AN_X1_TIMERS_cl73_dme_lock*/ 
  AN_X1_CL73_DME_LOCKr_CLR(reg_cl73_dme_lock);
  AN_X1_CL73_DME_LOCKr_SET(reg_cl73_dme_lock, 0x14d4);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL73_DME_LOCKr(pc, reg_cl73_dme_lock));

/*TBD::0x9256 AN_X1_TIMERS_link_up*/ 
  AN_X1_LNK_UPr_CLR(reg_link_up);
  AN_X1_LNK_UPr_SET(reg_link_up, 0x29a);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_UPr(pc, reg_link_up));

/*0x9257 AN_X1_TIMERS_link_fail_inhibit_timer_cl72*/ 
  AN_X1_LNK_FAIL_INHBT_TMR_CL72r_CLR(reg_inhibit_timer);
  AN_X1_LNK_FAIL_INHBT_TMR_CL72r_SET(reg_inhibit_timer, 0x8382);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72r(pc, reg_inhibit_timer));

/*0x9258 AN_X1_TIMERS_link_fail_inhibit_timer_not_cl72*/ 
  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_CLR(reg_inhibit_not_timer);
  /* AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_SET(reg_inhibit_not_timer, 0xbb8); */
  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_SET(reg_inhibit_not_timer, 0x14d5);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r(pc, reg_inhibit_not_timer));

/*0x9259 AN_X1_TIMERS_pd_sd_timer*/ 
 AN_X1_PD_SD_TMRr_CLR(reg_pd_sd_timer);
 AN_X1_PD_SD_TMRr_SET(reg_pd_sd_timer, 0xa6a);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_PD_SD_TMRr(pc, reg_pd_sd_timer));

/*0x925a AN_X1_TIMERS_cl72_max_wait_timer*/ 
  AN_X1_CL37_SYNC_STS_FILTER_TMRr_CLR(reg_sync_status_filter_timer);
  AN_X1_CL37_SYNC_STS_FILTER_TMRr_SET(reg_sync_status_filter_timer, 0x29a);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL37_SYNC_STS_FILTER_TMRr(pc, reg_sync_status_filter_timer));

/*0x925b AN_X1_TIMERS_PD_TO_CL37_LINK_WAIT_TIMER*/ 
  AN_X1_PD_TO_CL37_LNK_WAIT_TMRr_CLR(reg_link_wait_timer);
  AN_X1_PD_TO_CL37_LNK_WAIT_TMRr_SET(reg_link_wait_timer, 0xa6a);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_PD_TO_CL37_LNK_WAIT_TMRr(pc, reg_link_wait_timer));

  /*0x925c AN_X1_TIMERS_ignore_link_timer*/ 
  AN_X1_IGNORE_LNK_TMRr_CLR(reg_ignore_link_timer);
  AN_X1_IGNORE_LNK_TMRr_SET(reg_ignore_link_timer, 0x29a);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_IGNORE_LNK_TMRr(pc, reg_ignore_link_timer));

  /*0x925d AN_X1_TIMERS_dme_page_timer*/  
  AN_X1_DME_PAGE_TMRr_CLR(reg_dme_page_timer);
  AN_X1_DME_PAGE_TMRr_SET(reg_dme_page_timer, 0x3b5f);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_DME_PAGE_TMRr(pc, reg_dme_page_timer));

  /*0x925e AN_X1_TIMERS_sgmii_cl73_timer_type*/ 
  AN_X1_SGMII_CL73_TMR_TYPEr_CLR(reg_sgmii_cl73_timer);
  AN_X1_SGMII_CL73_TMR_TYPEr_SET(reg_sgmii_cl73_timer, 0x6b);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_SGMII_CL73_TMR_TYPEr(pc, reg_sgmii_cl73_timer));
  return PHYMOD_E_NONE;
}
#endif

/**
@brief   Controls the setting/resetting of autoneg advertisement registers.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details This function gets specific aneg related information from upper
software layers via #PHYMOD_ST->an_tech_ability and updates the autoneg pages.
This does not start autonegotiation. That is done in #temod_autoneg_control
*/
#ifdef _DV_TB_
int temod_autoneg_set(PHYMOD_ST* pc)               /* AUTONEG_SET */
{
   AN_X4_CTLSr_t                     reg_an_ctrl;
   DIG_CTL1000X2r_t                  reg_ctrl1000x2;
   AN_X4_LOC_DEV_CL37_BASE_ABILr_t   reg_cl37_base_abilities;
   AN_X4_LOC_DEV_CL37_BAM_ABILr_t    reg_cl37_bam_abilities;
   AN_X4_LOC_DEV_OVER1G_ABIL0r_t     reg_over1g_abilities;
   AN_X4_LOC_DEV_OVER1G_ABIL1r_t     reg_over1g_abilities1;
   AN_X4_LOC_DEV_CL73_BAM_ABILr_t    reg_cl73_bam_abilities;
   AN_X4_LOC_DEV_CL73_BASE_ABIL0r_t  reg_cl73_base_abilities;
 
   TEMOD_DBG_IN_FUNC_INFO(pc);
   AN_X4_CTLSr_CLR(reg_an_ctrl);
 
   AN_X4_CTLSr_AN_FAIL_COUNT_LIMITf_SET(reg_an_ctrl, pc->an_fail_cnt);
   AN_X4_CTLSr_OUI_CONTROLf_SET(reg_an_ctrl, pc->an_oui_ctrl);
   AN_X4_CTLSr_PD_KX_ENf_SET(reg_an_ctrl, pc->pd_kx_en);
   AN_X4_CTLSr_PD_KX4_ENf_SET(reg_an_ctrl, pc->pd_kx4_en);
   AN_X4_CTLSr_LINKFAILTIMER_DISf_SET(reg_an_ctrl, pc->linkfailtimer_dis);
   AN_X4_CTLSr_LINKFAILTIMERQUAL_ENf_SET(reg_an_ctrl, pc->linkfailtimerqua_en);
   AN_X4_CTLSr_AN_GOOD_CHECK_TRAPf_SET(reg_an_ctrl, pc->an_good_check_trap);
   AN_X4_CTLSr_AN_GOOD_TRAPf_SET(reg_an_ctrl,  pc->an_good_trap);
   PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_CTLSr(pc, reg_an_ctrl));
 
 
   DIG_CTL1000X2r_CLR(reg_ctrl1000x2);
   DIG_CTL1000X2r_DISABLE_REMOTE_FAULT_REPORTINGf_SET(reg_ctrl1000x2, pc->disable_rf_report);
   PHYMOD_IF_ERR_RETURN(MODIFY_DIG_CTL1000X2r(pc, reg_ctrl1000x2));
 
   if(pc->sc_mode == TEMOD_SC_MODE_AN_CL37) {
     /* an37 */
     if(!(pc->cl37_bam_en)){
      AN_X4_LOC_DEV_CL37_BASE_ABILr_CLR(reg_cl37_base_abilities);
      AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_AN_RESTART_RESET_DISABLEf_SET(reg_cl37_base_abilities, 1);
      AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_SW_RESTART_RESET_DISABLEf_SET (reg_cl37_base_abilities, 1);     
      PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LOC_DEV_CL37_BASE_ABILr(pc, reg_cl37_base_abilities));
     } 
     
     /*Local device cl37 base abilities setting*/
     /******* Pause Settings ********/
     AN_X4_LOC_DEV_CL37_BASE_ABILr_CLR(reg_cl37_base_abilities);
     if(pc->cl37_an_pause){ 
         AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_PAUSEf_SET(reg_cl37_base_abilities, (pc->cl37_an_pause & 3));
     }
     /******* Half duplex Settings ********/
     if(pc->cl37_an_hd){
        AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_HALF_DUPLEXf_SET(reg_cl37_base_abilities, (pc->cl37_an_hd & 1));
     }   
     /******* SGMII Master mode Settings ********/
     if(pc->cl37_sgmii_master_mode){ 
              AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_MASTER_MODEf_SET(reg_cl37_base_abilities, (pc->cl37_sgmii_master_mode & 1));
     }
     /******* SGMII Speed Settings ********/
     /*if(pc->cl37_sgmii_speed){ */
             AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_SPEEDf_SET(reg_cl37_base_abilities, (pc->cl37_sgmii_speed & 3));
     /*}*/
     /******* CL37 Next page setting ********/
     if(pc->cl37_an_np){ 
              AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_NEXT_PAGEf_SET(reg_cl37_base_abilities, (pc->cl37_an_np & 1));
     }
     /******* CL37 full duplex ********/
     if(pc->cl37_an_fd){ 
              AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_FULL_DUPLEXf_SET(reg_cl37_base_abilities, (pc->cl37_an_fd & 1));
     }
     /******* SGMII duplex Settings ********/
     if(pc->cl37_sgmii_duplex){ 
              AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_FULL_DUPLEXf_SET(reg_cl37_base_abilities, (pc->cl37_sgmii_duplex & 1));
     }
         

     /***** Setting AN_X4_ABILITIES_ld_base_abilities_1 0xC181 *******/
      PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LOC_DEV_CL37_BASE_ABILr(pc, reg_cl37_base_abilities));

     /*Local device cl37 bam abilities setting*/

     /******* CL37 Bam AN_X4_ABILITIES_local_device_over1g_abilities_0 setting********/
     /* cl37 bam abilities 0xc182 */
     AN_X4_LOC_DEV_CL37_BAM_ABILr_CLR(reg_cl37_bam_abilities);
        AN_X4_LOC_DEV_CL37_BAM_ABILr_CL37_BAM_CODEf_SET(reg_cl37_bam_abilities, (pc->cl37_bam_code & 0x1ff));
        AN_X4_LOC_DEV_CL37_BAM_ABILr_OVER1G_ABILITYf_SET(reg_cl37_bam_abilities, (pc->cl37_bam_ovr1g_en & 1));
        AN_X4_LOC_DEV_CL37_BAM_ABILr_OVER1G_PAGE_COUNTf_SET(reg_cl37_bam_abilities, (pc->cl37_bam_ovr1g_pgcnt & 3));

     PHYMOD_IF_ERR_RETURN 
            (MODIFY_AN_X4_LOC_DEV_CL37_BAM_ABILr(pc, reg_cl37_bam_abilities));


     AN_X4_LOC_DEV_OVER1G_ABIL0r_CLR(reg_over1g_abilities); 
     if(pc->cl37_bam_speed) 
         AN_X4_LOC_DEV_OVER1G_ABIL0r_SET(reg_over1g_abilities, (pc->cl37_bam_speed & 0x7ff));    
     /***** Setting AN_X4_ABILITIES_local_device_over1g_abilities_0  0xC184 *******/
     PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LOC_DEV_OVER1G_ABIL0r(pc, reg_over1g_abilities));

     /******* CL37 Bam AN_X4_ABILITIES_local_device_over1g_abilities_1 setting********/
     AN_X4_LOC_DEV_OVER1G_ABIL1r_CLR(reg_over1g_abilities1);
     if(pc->cl37_bam_speed)  
          AN_X4_LOC_DEV_OVER1G_ABIL1r_SET(reg_over1g_abilities1,pc->cl37_bam_speed >> 11);
       /* AN_LOCAL_DEVICE_OVER1G_ABILITIES_1r_BAM_13GBASE_X4f_SET(reg_over1g_abilities1, (pc->cl37_bam_speed >> 11)); */
     if(pc->cl37_bam_fec)
             AN_X4_LOC_DEV_OVER1G_ABIL1r_FECf_SET(reg_over1g_abilities1, (pc->cl37_bam_fec & 1));
    if(pc->cl37_bam_hg2) 
              AN_X4_LOC_DEV_OVER1G_ABIL1r_HG2f_SET(reg_over1g_abilities1, (pc->cl37_bam_hg2 & 1));
     if(pc->an_abilities_CL72) 
              AN_X4_LOC_DEV_OVER1G_ABIL1r_CL72f_SET(reg_over1g_abilities1, (pc->an_abilities_CL72 & 1));
     /***** Setting AN_X4_ABILITIES_local_device_over1g_abilities_0  0xC183 *******/
     PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LOC_DEV_OVER1G_ABIL1r(pc, reg_over1g_abilities1));
  } /*Completion of CL37 abilities*/
  /*CL73 abilities*/
  else if(pc->sc_mode == TEMOD_SC_MODE_AN_CL73) {
    /*PD to CL37 Enable*/
    if(pc->an_pd_to_cl37_enable) {
      AN_X4_LOC_DEV_CL37_BASE_ABILr_CLR(reg_cl37_base_abilities);
      AN_X4_LOC_DEV_CL37_BASE_ABILr_AN_PD_TO_CL37_ENABLEf_SET(reg_cl37_base_abilities, 0x1);
      PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LOC_DEV_CL37_BASE_ABILr(pc, reg_cl37_base_abilities));
      AN_X4_LOC_DEV_CL37_BASE_ABILr_CLR(reg_cl37_base_abilities);
      if(pc->cl37_an_pause){ 
               AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_PAUSEf_SET(reg_cl37_base_abilities, (pc->cl37_an_pause & 3));
      }
      /******* Half duplex Settings ********/
      if(pc->cl37_an_hd){
               AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_HALF_DUPLEXf_SET(reg_cl37_base_abilities, (pc->cl37_an_hd & 1));
      }   
      /******* SGMII Master mode Settings ********/
      if(pc->cl37_sgmii_master_mode){ 
               AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_MASTER_MODEf_SET(reg_cl37_base_abilities, (pc->cl37_sgmii_master_mode & 1));
      }
      /******* SGMII Speed Settings ********/
      if(pc->cl37_sgmii_speed){ 
               AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_SPEEDf_SET(reg_cl37_base_abilities, (pc->cl37_sgmii_speed & 3));
      }
      /******* CL37 Next page setting ********/
      if(pc->cl37_an_np){ 
               AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_NEXT_PAGEf_SET(reg_cl37_base_abilities, (pc->cl37_an_np & 1));
      }
      /******* CL37 full duplex ********/
      if(pc->cl37_an_fd){ 
               AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_FULL_DUPLEXf_SET(reg_cl37_base_abilities, (pc->cl37_an_fd & 1));
      }
      /******* SGMII duplex Settings ********/
      if(pc->cl37_sgmii_duplex){ 
               AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_FULL_DUPLEXf_SET(reg_cl37_base_abilities, (pc->cl37_sgmii_duplex & 1));
      }

      /***** Setting AN_X4_ABILITIES_ld_base_abilities_1 0xC181 *******/
      PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LOC_DEV_CL37_BASE_ABILr(pc, reg_cl37_base_abilities));
    }
    /*CL73 bam abilities setting*/
    AN_X4_LOC_DEV_CL73_BAM_ABILr_CLR(reg_cl73_bam_abilities);
    AN_X4_LOC_DEV_CL73_BAM_ABILr_BAM_20GBASE_KR2f_SET(reg_cl73_bam_abilities,( pc->cl73_bam_speed & 1));
    AN_X4_LOC_DEV_CL73_BAM_ABILr_BAM_20GBASE_CR2f_SET(reg_cl73_bam_abilities,((pc->cl73_bam_speed >> 1) & 0x1));
    AN_X4_LOC_DEV_CL73_BAM_ABILr_HPAM_20GKR2f_SET(reg_cl73_bam_abilities,  ((pc->cl73_bam_speed & 4) >> 2));
    AN_X4_LOC_DEV_CL73_BAM_ABILr_CL73_BAM_CODEf_SET(reg_cl73_bam_abilities, (pc->cl73_bam_code & 0x1ff));
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LOC_DEV_CL73_BAM_ABILr(pc, reg_cl73_bam_abilities));
    /*CL73 base abilities_0 setting*/
    AN_X4_LOC_DEV_CL73_BASE_ABIL0r_CLR(reg_cl73_base_abilities);
    AN_X4_LOC_DEV_CL73_BASE_ABIL0r_NEXT_PAGEf_SET(reg_cl73_base_abilities, (pc->cl73_nxt_page & 1));
    AN_X4_LOC_DEV_CL73_BASE_ABIL0r_BASE_100GBASE_CR10f_SET(reg_cl73_base_abilities, ((pc->cl73_speed >> 0)& 0x1));
    AN_X4_LOC_DEV_CL73_BASE_ABIL0r_BASE_40GBASE_CR4f_SET(reg_cl73_base_abilities,   ((pc->cl73_speed >> 1)& 0x1));
    AN_X4_LOC_DEV_CL73_BASE_ABIL0r_BASE_40GBASE_KR4f_SET(reg_cl73_base_abilities,   ((pc->cl73_speed >> 2)& 0x1));
    AN_X4_LOC_DEV_CL73_BASE_ABIL0r_BASE_10GBASE_KRf_SET(reg_cl73_base_abilities,    ((pc->cl73_speed >> 3)& 0x1));
    AN_X4_LOC_DEV_CL73_BASE_ABIL0r_BASE_10GBASE_KX4f_SET(reg_cl73_base_abilities,   ((pc->cl73_speed >> 4)& 0x1));
    AN_X4_LOC_DEV_CL73_BASE_ABIL0r_BASE_1000BASE_KXf_SET(reg_cl73_base_abilities,   ((pc->cl73_speed >> 5)& 0x1));

    AN_X4_LOC_DEV_CL73_BASE_ABIL0r_CL73_PAUSEf_SET(reg_cl73_base_abilities, ( pc->cl73_pause & 3));
    AN_X4_LOC_DEV_CL73_BASE_ABIL0r_FECf_SET(reg_cl73_base_abilities, ( pc->cl73_fec & 3));
    AN_X4_LOC_DEV_CL73_BASE_ABIL0r_CL73_REMOTE_FAULTf_SET(reg_cl73_base_abilities, ( pc->cl73_remote_fault & 1));
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LOC_DEV_CL73_BASE_ABIL0r(pc, reg_cl73_base_abilities));
  }
  return PHYMOD_E_NONE;
}
#endif

#ifdef _SDK_TEMOD_
int temod_autoneg_set(PHYMOD_ST* pc,
    temod_an_ability_t *an_ability_st) /* AUTONEG_SET */
 {
  AN_X4_LOC_DEV_CL37_BASE_ABILr_t    reg_cl37_base_abilities;
  AN_X4_LOC_DEV_CL37_BAM_ABILr_t     reg_cl37_bam_abilities;
  AN_X4_LOC_DEV_OVER1G_ABIL0r_t     reg_over1g_abilities;
  AN_X4_LOC_DEV_OVER1G_ABIL1r_t     reg_over1g_abilities1;
  AN_X4_LOC_DEV_CL73_BAM_ABILr_t     reg_cl73_bam_abilities;
  AN_X4_LOC_DEV_CL73_BASE_ABIL0r_t  reg_cl73_base_abilities;
  AN_X4_LOC_DEV_CL73_BASE_ABIL1r_t  reg_an_cl73_abl_1;

  TEMOD_DBG_IN_FUNC_INFO(pc);

 /* if((an_ability_st->cl37_adv.an_type ==  TEMOD_AN_MODE_CL37) || ( an_ability_st->cl37_adv.an_type ==  TEMOD_AN_MODE_CL37BAM) || 
     (an_ability_st->cl37_adv.an_type ==  TEMOD_AN_MODE_SGMII) )  */ {

      /* an37 */
      /*if((an_ability_st->cl37_adv.an_type !=  TEMOD_AN_MODE_CL37BAM)) */{ 
       AN_X4_LOC_DEV_CL37_BASE_ABILr_CLR(reg_cl37_base_abilities);
       AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_AN_RESTART_RESET_DISABLEf_SET(reg_cl37_base_abilities, 1);
       AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_SW_RESTART_RESET_DISABLEf_SET (reg_cl37_base_abilities, 1);
       PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LOC_DEV_CL37_BASE_ABILr(pc, reg_cl37_base_abilities));
      }

      /*Local device cl37 base abilities setting*/
      AN_X4_LOC_DEV_CL37_BASE_ABILr_CLR(reg_cl37_base_abilities);
      AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_PAUSEf_SET(reg_cl37_base_abilities, (an_ability_st->cl37_adv.an_pause & 3));
      AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_SPEEDf_SET(reg_cl37_base_abilities, (an_ability_st->cl37_adv.cl37_sgmii_speed & 3));

      AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_FULL_DUPLEXf_SET(reg_cl37_base_abilities, 1);
      AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_FULL_DUPLEXf_SET(reg_cl37_base_abilities, 1);

      /***** Setting AN_X4_ABILITIES_ld_base_abilities_1 0xC181 *******/
       PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LOC_DEV_CL37_BASE_ABILr(pc, reg_cl37_base_abilities));

      /*Local device cl37 bam abilities setting*/
      /******* CL37 Bam AN_X4_ABILITIES_local_device_over1g_abilities_0 setting********/
      /* cl37 bam abilities 0xc182 */
      /* Once we put this code in init, can remove teh over1g* cfg from here */
      /*if((an_ability_st->cl37_adv.an_type ==  TEMOD_AN_MODE_CL37BAM)) */{ 
         AN_X4_LOC_DEV_CL37_BAM_ABILr_CLR(reg_cl37_bam_abilities);
         AN_X4_LOC_DEV_CL37_BAM_ABILr_CL37_BAM_CODEf_SET(reg_cl37_bam_abilities, 0x2);
         AN_X4_LOC_DEV_CL37_BAM_ABILr_OVER1G_ABILITYf_SET(reg_cl37_bam_abilities, 0x1);
         AN_X4_LOC_DEV_CL37_BAM_ABILr_OVER1G_PAGE_COUNTf_SET(reg_cl37_bam_abilities, 0x5);

         PHYMOD_IF_ERR_RETURN
             (MODIFY_AN_X4_LOC_DEV_CL37_BAM_ABILr(pc, reg_cl37_bam_abilities));

         AN_X4_LOC_DEV_OVER1G_ABIL0r_CLR(reg_over1g_abilities);
         AN_X4_LOC_DEV_OVER1G_ABIL1r_CLR(reg_over1g_abilities1);

         if(an_ability_st->cl37_adv.an_bam_speed)
             AN_X4_LOC_DEV_OVER1G_ABIL0r_SET(reg_over1g_abilities, (an_ability_st->cl37_adv.an_bam_speed & 0x7ff));
         if(an_ability_st->cl37_adv.an_bam_speed1)
             AN_X4_LOC_DEV_OVER1G_ABIL1r_SET(reg_over1g_abilities1, (an_ability_st->cl37_adv.an_bam_speed1 & 0x1fff ));


         /******* CL37 Bam AN_X4_ABILITIES_local_device_over1g_abilities_1 setting********/
         if(an_ability_st->cl37_adv.an_fec)
                 AN_X4_LOC_DEV_OVER1G_ABIL1r_FECf_SET(reg_over1g_abilities1, (an_ability_st->cl37_adv.an_fec & 1));
          if(an_ability_st->cl37_adv.an_hg2)
                  AN_X4_LOC_DEV_OVER1G_ABIL1r_HG2f_SET(reg_over1g_abilities1, (an_ability_st->cl37_adv.an_hg2 & 1));
         if(an_ability_st->cl37_adv.an_cl72)
                  AN_X4_LOC_DEV_OVER1G_ABIL1r_CL72f_SET(reg_over1g_abilities1, (an_ability_st->cl37_adv.an_cl72 & 1));
         /***** Setting AN_X4_ABILITIES_local_device_over1g_abilities_0  0xC183 *******/
         PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LOC_DEV_OVER1G_ABIL0r(pc, reg_over1g_abilities));
         PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LOC_DEV_OVER1G_ABIL1r(pc, reg_over1g_abilities1));
      } /* TEMOD_AN_MODE_CL37BAM */

  }/*Completion of CL37 abilities*/
  /*CL73 abilities*/
  /*if((an_ability_st->cl73_adv.an_type ==  TEMOD_AN_MODE_CL73) || ( an_ability_st->cl73_adv.an_type ==  TEMOD_AN_MODE_CL73BAM)) */  {
      /*CL73 bam abilities setting*/
      AN_X4_LOC_DEV_CL73_BAM_ABILr_CLR(reg_cl73_bam_abilities);
      AN_X4_LOC_DEV_CL73_BAM_ABILr_BAM_20GBASE_KR2f_SET(reg_cl73_bam_abilities,( an_ability_st->cl73_adv.an_bam_speed & 1));
      AN_X4_LOC_DEV_CL73_BAM_ABILr_BAM_20GBASE_CR2f_SET(reg_cl73_bam_abilities,((an_ability_st->cl73_adv.an_bam_speed >> 1) & 0x1));
      AN_X4_LOC_DEV_CL73_BAM_ABILr_HPAM_20GKR2f_SET(reg_cl73_bam_abilities,  ((an_ability_st->cl73_adv.an_bam_speed & 4) >> 2));
      AN_X4_LOC_DEV_CL73_BAM_ABILr_CL73_BAM_CODEf_SET(reg_cl73_bam_abilities,  0x4);
      PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LOC_DEV_CL73_BAM_ABILr(pc, reg_cl73_bam_abilities));
      /*CL73 base abilities_0 setting*/
      AN_X4_LOC_DEV_CL73_BASE_ABIL0r_CLR(reg_cl73_base_abilities);

      AN_X4_LOC_DEV_CL73_BASE_ABIL0r_SET(reg_cl73_base_abilities, an_ability_st->cl73_adv.an_base_speed & 0x3f);

      AN_X4_LOC_DEV_CL73_BASE_ABIL0r_CL73_PAUSEf_SET(reg_cl73_base_abilities, ( an_ability_st->cl73_adv.an_pause & 3));
      if (an_ability_st->cl73_adv.an_fec == 1) {
        AN_X4_LOC_DEV_CL73_BASE_ABIL0r_FECf_SET(reg_cl73_base_abilities, 0x3);
      } else {
        AN_X4_LOC_DEV_CL73_BASE_ABIL0r_FECf_SET(reg_cl73_base_abilities, 0x0);
      }
      PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LOC_DEV_CL73_BASE_ABIL0r(pc, reg_cl73_base_abilities));

      /* set cl73 nonce set 0xc340 */
      AN_X4_LOC_DEV_CL73_BASE_ABIL1r_CLR(reg_an_cl73_abl_1);
      AN_X4_LOC_DEV_CL73_BASE_ABIL1r_BASE_SELECTORf_SET(reg_an_cl73_abl_1, 0x1);
      PHYMOD_IF_ERR_RETURN ( MODIFY_AN_X4_LOC_DEV_CL73_BASE_ABIL1r(pc, reg_an_cl73_abl_1));

      
      if((an_ability_st->cl73_adv.an_cl72 & 0x1) == 1) {
         temod_override_set(pc, TEMOD_OVERRIDE_RESET, 0x0);
      } else {
         temod_override_set(pc, TEMOD_OVERRIDE_CL72, 0x0);
      }
  }
  return PHYMOD_E_NONE;
}
#endif /* _SDK_TEMOD_*/

/**
@brief   To get autoneg control registers.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   an_control reference of type  #temod_an_control_t which is updated here
@param   an_complete  reference which is updated with an_complete info
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Upper software layers call this function to get the status of autoneg.
This function  does not return autoneg abilities, but only the status of
completion.
*/

int temod_autoneg_control_get(PHYMOD_ST* pc, temod_an_control_t *an_control, int *an_complete) 
{
    AN_X4_LOC_DEV_CL37_BASE_ABILr_t reg_an_cl37_abl;
    AN_X4_ENSr_t reg_an_enb;
    AN_X4_CTLSr_t an_x4_abl_ctrl;
    AN_X4_AN_MISC_STSr_t an_misc_sts;

    TEMOD_DBG_IN_FUNC_INFO(pc);
    READ_AN_X4_LOC_DEV_CL37_BASE_ABILr(pc, &reg_an_cl37_abl);
    
    if(AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_MASTER_MODEf_GET(reg_an_cl37_abl) == 1) {
      an_control->an_property_type = TEMOD_AN_PROPERTY_ENABLE_SGMII_MASTER_MODE;
    }

    READ_AN_X4_CTLSr(pc, &an_x4_abl_ctrl);
    an_control->pd_kx_en = AN_X4_CTLSr_PD_KX_ENf_GET(an_x4_abl_ctrl);
    an_control->pd_kx4_en = AN_X4_CTLSr_PD_KX4_ENf_GET(an_x4_abl_ctrl);

    /*Setting X4 abilities*/
    READ_AN_X4_ENSr(pc, &reg_an_enb);
    if(AN_X4_ENSr_CL37_BAM_ENABLEf_GET(reg_an_enb) == 1){
       an_control->an_type = TEMOD_AN_MODE_CL37BAM;
       an_control->enable = 1;
    } else if (AN_X4_ENSr_CL73_BAM_ENABLEf_GET(reg_an_enb) == 1){
       an_control->an_type = TEMOD_AN_MODE_CL73BAM;
       an_control->enable = 1;
    } else if (AN_X4_ENSr_CL73_HPAM_ENABLEf_GET(reg_an_enb) == 1) {
       an_control->an_type = TEMOD_AN_MODE_HPAM;
       an_control->enable = 1;
    } else if ( AN_X4_ENSr_CL73_ENABLEf_GET(reg_an_enb) == 1) {
       an_control->an_type = TEMOD_AN_MODE_CL73;
       an_control->enable = 1;
    } else if ( AN_X4_ENSr_CL37_SGMII_ENABLEf_GET(reg_an_enb) == 1) {
       an_control->an_type = TEMOD_AN_MODE_SGMII;
       an_control->enable = 1;
    } else if ( AN_X4_ENSr_CL37_ENABLEf_GET(reg_an_enb) == 1){
       an_control->an_type = TEMOD_AN_MODE_CL37;;
       an_control->enable = 1;
    } else {
       an_control->an_type = TEMOD_AN_MODE_NONE;
       an_control->enable = 0;
    }

    if(AN_X4_ENSr_HPAM_TO_CL73_AUTO_ENABLEf_GET(reg_an_enb) == 1) {
      an_control->an_property_type = TEMOD_AN_PROPERTY_ENABLE_HPAM_TO_CL73_AUTO;
    } else if(AN_X4_ENSr_CL73_BAM_TO_HPAM_AUTO_ENABLEf_GET(reg_an_enb) == 1) {
      an_control->an_property_type = TEMOD_AN_PROPERTY_ENABLE_CL73_BAM_TO_HPAM_AUTO;
    } else if(AN_X4_ENSr_SGMII_TO_CL37_AUTO_ENABLEf_GET(reg_an_enb) == 1) {
      an_control->an_property_type = TEMOD_AN_PROPERTY_ENABLE_SGMII_TO_CL37_AUTO;
    } else if(AN_X4_ENSr_CL37_BAM_TO_SGMII_AUTO_ENABLEf_GET(reg_an_enb) == 1) {
      an_control->an_property_type = TEMOD_AN_PROPERTY_ENABLE_CL37_BAM_to_SGMII_AUTO;
    }

    an_control->num_lane_adv = AN_X4_ENSr_NUM_ADVERTISED_LANESf_GET(reg_an_enb);

    /* an_complete status */
    AN_X4_AN_MISC_STSr_CLR(an_misc_sts);

    READ_AN_X4_AN_MISC_STSr(pc, &an_misc_sts);

    *an_complete = AN_X4_AN_MISC_STSr_AN_COMPLETEf_GET(an_misc_sts);

  return PHYMOD_E_NONE;
}

/**
@brief   To get local autoneg advertisement information.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   an_ability_st reference of type #temod_an_ability_t updated by this function.
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Upper software layers call this function to get local autoneg
abilities info. This function must be called after advertisement is set. This
function does not return the progress of autoneg.
*/
   
int temod_autoneg_local_ability_get(PHYMOD_ST* pc, temod_an_ability_t *an_ability_st)
{
  AN_X4_LOC_DEV_CL37_BASE_ABILr_t    reg_cl37_base_abilities;
  AN_X4_LOC_DEV_OVER1G_ABIL0r_t      reg_over1g_abilities;
  AN_X4_LOC_DEV_OVER1G_ABIL1r_t      reg_over1g_abilities1;
  AN_X4_LOC_DEV_CL73_BAM_ABILr_t     reg_cl73_bam_abilities;
  AN_X4_LOC_DEV_CL73_BASE_ABIL0r_t   reg_cl73_base_abilities;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  READ_AN_X4_LOC_DEV_CL37_BASE_ABILr(pc, &reg_cl37_base_abilities);
  an_ability_st->cl37_adv.an_pause = AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_PAUSEf_GET(reg_cl37_base_abilities);
  an_ability_st->cl37_adv.cl37_sgmii_speed = AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_SPEEDf_GET(reg_cl37_base_abilities); 

  READ_AN_X4_LOC_DEV_OVER1G_ABIL0r(pc, &reg_over1g_abilities);
  an_ability_st->cl37_adv.an_bam_speed = AN_X4_LOC_DEV_OVER1G_ABIL0r_GET(reg_over1g_abilities) & 0x7ff;

  READ_AN_X4_LOC_DEV_OVER1G_ABIL1r(pc, &reg_over1g_abilities1);
  an_ability_st->cl37_adv.an_bam_speed1 = AN_X4_LOC_DEV_OVER1G_ABIL1r_GET(reg_over1g_abilities1) & 0xfff;

  an_ability_st->cl37_adv.an_fec = AN_X4_LOC_DEV_OVER1G_ABIL1r_FECf_GET(reg_over1g_abilities1);
  an_ability_st->cl37_adv.an_cl72 = AN_X4_LOC_DEV_OVER1G_ABIL1r_CL72f_GET(reg_over1g_abilities1);
  an_ability_st->cl37_adv.an_hg2 = AN_X4_LOC_DEV_OVER1G_ABIL1r_HG2f_GET(reg_over1g_abilities1);
  
  READ_AN_X4_LOC_DEV_CL73_BAM_ABILr(pc, &reg_cl73_bam_abilities); 
  an_ability_st->cl73_adv.an_bam_speed = AN_X4_LOC_DEV_CL73_BAM_ABILr_GET(reg_cl73_bam_abilities) & 0x7;
  READ_AN_X4_LOC_DEV_CL73_BASE_ABIL0r(pc, &reg_cl73_base_abilities);
  an_ability_st->cl73_adv.an_base_speed = AN_X4_LOC_DEV_CL73_BASE_ABIL0r_GET(reg_cl73_base_abilities) & 0x3f;

  an_ability_st->cl73_adv.an_pause = AN_X4_LOC_DEV_CL73_BASE_ABIL0r_CL73_PAUSEf_GET(reg_cl73_base_abilities);
  an_ability_st->cl73_adv.an_fec = AN_X4_LOC_DEV_CL73_BASE_ABIL0r_FECf_GET(reg_cl73_base_abilities);

  return PHYMOD_E_NONE;
}

/**
@brief   Function to get autoneg remote advertisement information.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   an_ability_st structurewith the remote abilities
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details Upper software layers call this function to get remote autoneg info.
Call this function only after page exchange. 
*/
   
int temod_autoneg_remote_ability_get(PHYMOD_ST* pc, temod_an_ability_t *an_ability_st)
{
  AN_X4_LP_MP1024_UP1r_t an_x4_up1;
  AN_X4_LP_MP1024_UP3r_t an_x4_up3;
  AN_X4_LP_MP1024_UP4r_t an_x4_up4;
  AN_X4_LP_BASE_PAGE1r_t an_x4_pg1;
  AN_X4_LP_BASE_PAGE2r_t an_x4_pg2;
  AN_X4_LP_BASE_PAGE3r_t an_x4_pg3;
  AN_X4_LP_MP5_UP3r_t an_x4_mp5_up3;
  AN_X4_LP_MP5_UP4r_t an_x4_mp5_up4;

  uint32_t data;
  uint32_t user_code;

  TEMOD_DBG_IN_FUNC_INFO(pc);

  AN_X4_LP_MP1024_UP1r_CLR(an_x4_up1);
  AN_X4_LP_MP1024_UP3r_CLR(an_x4_up3);
  AN_X4_LP_MP1024_UP4r_CLR(an_x4_up4);
  AN_X4_LP_BASE_PAGE1r_CLR(an_x4_pg1);
  AN_X4_LP_BASE_PAGE2r_CLR(an_x4_pg2);
  AN_X4_LP_BASE_PAGE3r_CLR(an_x4_pg3);
  AN_X4_LP_MP5_UP3r_CLR(an_x4_mp5_up3);
  AN_X4_LP_MP5_UP4r_CLR(an_x4_mp5_up4);

  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_MP1024_UP1r(pc, &an_x4_up1));
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_MP1024_UP3r(pc, &an_x4_up3));
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_MP1024_UP4r(pc, &an_x4_up4));
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_BASE_PAGE1r(pc, &an_x4_pg1));
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_BASE_PAGE2r(pc, &an_x4_pg2));
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_BASE_PAGE3r(pc, &an_x4_pg3));
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_MP5_UP3r(pc, &an_x4_mp5_up3));
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_MP5_UP4r(pc, &an_x4_mp5_up4));

  /*data:  get the   BAM_12GBASE_X4 and   BAM_12p5GBASE_X4 */
  data = ((((an_x4_up1.v[0] >> 5) & 0x3) << 8) & 0x0300);
  /* data: get BAM_10GBASE_X2_CX4, BAM_10GBASE_X2_CX4 and BAM_10p5GBASE_X2 */
  data = data | ((an_x4_up4.v[0] >> 1) & 0x7) << 5;
  /* get BAM_12p7GBASE_X2 */
  data = data | (((an_x4_up4.v[0] >> 4) & 0x1) << 10);
  an_ability_st->cl37_adv.an_bam_speed = (an_x4_up1.v[0] & 0x1f) | data;

  /* data - get   BAM_13GBASE_X4, BAM_15GBASE_X4,   BAM_16GBASE_X4 and   BAM_20GBASE_CX4 */
  data = ((an_x4_up1.v[0] >> 7) & 0xf);
  data = (data & 0x3) | (((data >> 2) && 0x3) << 3);
  an_ability_st->cl37_adv.an_bam_speed1 = data;
  /* data -    BAM_15p75GBASE_X2 ,   BAM_20GBASE_X2*/
  data = (((an_x4_up4.v[0] >> 5 ) & 0x1) << 6) | (((an_x4_up4.v[0] >> 7) & 0x1) << 2);
  an_ability_st->cl37_adv.an_bam_speed1 = an_ability_st->cl37_adv.an_bam_speed1 | data;
  /* data -       -  21G-X4 ,   25.45G-X4,  31.5G-X4,   32.7G, 40G-X4 */
  data = (((an_x4_up3.v[0] >> 9) & 0x1) << 8) |
         (((an_x4_up3.v[0] >> 8) & 0x1) << 9) | 
         (((an_x4_up3.v[0] >> 7) & 0x1) << 10) | 
         (((an_x4_up3.v[0] >> 6) & 0x1) << 11) | 
         (((an_x4_up3.v[0] >> 5) & 0x1) << 12) ; 
  an_ability_st->cl37_adv.an_bam_speed1 = an_ability_st->cl37_adv.an_bam_speed1 | data;

  
  /*an_ability_st->cl73_adv.an_bam_speed = 0xdead;*/
  an_ability_st->cl73_adv.an_bam_speed = 0x0;

  user_code = (an_x4_mp5_up3.v[0] & 0x1ff) << 11 ;

  data = an_x4_mp5_up4.v[0];
  /* 0xc193 */
  if(an_x4_mp5_up4.v[0]&(1<<1)) {
     an_ability_st->cl73_adv.an_bam_speed |=(1<<0) ;
  } else {
     user_code |= (an_x4_mp5_up4.v[0] & 0x7ff) ;
     if(user_code == 0xabe20)
        an_ability_st->cl73_adv.an_bam_speed  |=(1<<0) ;
  }
  if(data&(1<<0)) an_ability_st->cl73_adv.an_bam_speed |=(1<<1) ;


  data = (((an_x4_pg2.v[0] >> 10) & 0x1) << 0) |
         (((an_x4_pg2.v[0] >> 9) & 0x1) << 1) |
         (((an_x4_pg2.v[0] >> 8) & 0x1) << 2) |
         (((an_x4_pg2.v[0] >> 7) & 0x1) << 3) |
         (((an_x4_pg2.v[0] >> 6) & 0x1) << 4) |
         (((an_x4_pg2.v[0] >> 5) & 0x1) << 5) ;

  an_ability_st->cl37_adv.cl37_sgmii_speed = (an_x4_pg1.v[0] >> 10) & 0x3; /*  Check the speed ability and pass it as well */
  an_ability_st->cl37_adv.an_pause = (an_x4_pg1.v[0] >> 7) & 0x3;
  an_ability_st->cl37_adv.an_hg2 = an_x4_up3.v[0] & 0x1;
  an_ability_st->cl37_adv.an_fec = (an_x4_up3.v[0] >> 1) & 0x1;
  an_ability_st->cl37_adv.an_cl72 = (an_x4_up3.v[0] >> 2) & 0x1;

  an_ability_st->cl73_adv.an_pause = (an_x4_pg1.v[0] >> 10 ) & 0x3;;
  an_ability_st->cl73_adv.an_fec = (an_x4_pg3.v[0] >> 14) & 0x3;
  
  return PHYMOD_E_NONE;
}

/**
@brief   Controls setting/resetting of autoneg and enabling/disabling
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   an_control structure with AN controls
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
@details This function programs auto-negotiation (AN) modes for the TSCE. It can
enable/disable clause37/clause73/BAM autonegotiation capabilities. Call this
function once for combo mode and once per lane in independent lane mode.

The autonegotiation mode is indicated by setting an_type with

\li TEMOD_AN_NONE   (disable AN)
\li TEMOD_CL73
\li TEMOD_CL73_BAM
\li TEMOD_HPAM
*/

#ifdef _DV_TB_
int temod_autoneg_control(PHYMOD_ST* pc)
{
  uint16_t data ;
  uint16_t an_setup_enable, num_advertised_lanes, cl37_bam_enable, cl73_bam_enable  ;
  uint16_t cl73_hpam_enable, cl73_enable, cl37_sgmii_enable, cl37_enable;
  uint16_t cl73_nonce_match_over, cl73_nonce_match_val  ,cl73_transmit_nonce;
  uint16_t base_selector, cl73_bam_code  ;
  uint16_t hppam_20gkr2, next_page  ;
  uint16_t cl37_next_page,  cl37_full_duplex; 
  uint16_t sgmii_full_duplex ;
  uint16_t cl37_bam_code, over1g_ability, over1g_page_count;
  uint16_t cl37_restart, cl73_restart ;
  int    no_an ;
AN_X4_LOC_DEV_CL37_BAM_ABILr_t reg_an_cl37_bam_abilities;
/*AN_X4_LOC_DEV_CL73_BASE_ABIL0r_t reg_an_cl73_abl_0; */
AN_X4_LOC_DEV_CL73_BASE_ABIL1r_t reg_an_cl73_abl_1;
/* AN_X4_LOC_DEV_CL73_BAM_ABILr_t reg_an_cl73_abl; */
/* UC_COMMAND3r_t reg_com3; */
AN_X4_ENSr_t reg_an_enb;
  TEMOD_DBG_IN_FUNC_INFO(pc);

/* TSCE_COMMANDr_t reg_com;
*/
 
  no_an                         = 0 ; 

  an_setup_enable               = 0x1;
  num_advertised_lanes          = 0x2;   
  cl37_bam_enable               = 0x0;
  cl73_bam_enable               = 0x0;
  cl73_hpam_enable              = 0x0;
  cl73_enable                   = 0x0;
  cl37_sgmii_enable             = 0x0;
  cl37_enable                   = 0x0;
  cl73_nonce_match_over         = 0x1;
  cl73_transmit_nonce           = 0x0;
  cl73_nonce_match_val          = 0x0;
  base_selector                 = 0x1;
  cl73_bam_code                 = 0x0;
  hppam_20gkr2                  = 0x0;
  next_page                     = 0x0;
  cl37_next_page                = 0x0;
  cl37_full_duplex              = 0x1; 
  sgmii_full_duplex             = 0x1;
  cl37_bam_code                 = 0x0;
  over1g_ability                = 0x0;
  over1g_page_count             = 0x0;
  cl37_restart                  = 0x0;
  cl73_restart                  = 0x0;

  num_advertised_lanes = pc->no_of_lanes;

  /*if (pc-> an_type == TSCMOD_CL73) {
    cl73_restart                = 0x1;
    cl73_enable                 = 0x1;
    cl73_nonce_match_over       = 0x1;
    cl73_nonce_match_val        = 0x0;
    base_selector               = 0x1;
  } else if (pc-> an_type == TSCMOD_CL37) {
    cl37_restart                = 0x1;
    cl37_enable                 = 0x1;
    cl37_bam_code               = 0x0;
    over1g_ability              = 0x0;
    over1g_page_count           = 0x0;
    sgmii_full_duplex           = 0x0;
  } else if (pc-> an_type == TSCMOD_CL37_BAM) {
    cl37_restart                = 0x1;
    cl37_enable                 = 0x1;
    cl37_bam_enable             = 0x1;
    cl37_bam_code               = 0x1;
    over1g_ability              = 0x1;
    over1g_page_count           = 0x1; */  /* PHY-863: =1: 5 pages; =0: 3 pages; WC uses 3 pages */
    /*sgmii_full_duplex           = 0x0;
    cl37_next_page              = 0x1;
  } else if (pc-> an_type == TSCMOD_CL37_SGMII) {
    cl37_restart                = 0x1;
    cl37_sgmii_enable           = 0x1;
    cl37_enable                 = 0x1;
    cl37_bam_code               = 0x0;
    over1g_ability              = 0x0;
    over1g_page_count           = 0x0;
  } else if (pc-> an_type == TSCMOD_CL73_BAM) {
    cl73_restart                = 0x1;
    cl73_enable                 = 0x1; 
    cl73_bam_enable             = 0x1;
    cl73_bam_code               = 0x3;
    next_page                   = 0x1;
    cl73_nonce_match_over       = 0x1;
    cl73_nonce_match_val        = 0x0;
    base_selector               = 0x1;
  } else if (pc-> an_type == TSCMOD_HPAM) {
    cl73_restart                = 0x1;
    cl73_enable                 = 0x1; 
    cl73_hpam_enable            = 0x1;
    cl73_bam_code               = 0x3;
    hppam_20gkr2                = 0x1;
    next_page                   = 0x1;
    cl73_nonce_match_over       = 0x1;
    cl73_nonce_match_val        = 0x0;
    base_selector               = 0x1;
  } else {
     no_an = 1 ; 
     printf("%-22s ERROR: u=%0d p=%0d Autoneg mode not defined\n", __func__, pc->unit, pc->port);
  }*/
#if defined (_DV_TB_)
  if (pc->sc_mode == TEMOD_SC_MODE_AN_CL37) {
    cl37_restart                = 0x0;
    cl37_enable                 = pc->cl37_an_en;
    cl37_bam_enable             = pc->cl37_bam_en;
    cl37_sgmii_enable           = pc->cl37_sgmii_en;
    cl37_bam_code               = pc->cl37_bam_code & 0x1ff;
    over1g_ability              = pc->cl37_bam_ovr1g_en & 1;
    over1g_page_count           = pc->cl37_bam_ovr1g_pgcnt & 3;   /* PHY-863: =1: 5 pages; =0: 3 pages; WC uses 3 pages */
    sgmii_full_duplex           = pc->cl37_sgmii_duplex & 1;
    cl37_next_page              = pc->cl37_an_np & 1;
  } else if (pc->sc_mode == TEMOD_SC_MODE_AN_CL73) {
    cl73_restart                = 0x0;
    cl73_enable                 = pc->cl73_an_en; 
    cl73_bam_enable             = pc->cl73_bam_en;
    cl73_hpam_enable            = pc->cl73_hpam_en;
    cl73_bam_code               = pc->cl73_bam_code & 0x1ff;
    next_page                   = pc->cl73_nxt_page & 1;
    cl73_nonce_match_over       = pc->cl73_nonce_match_over & 1;
    cl73_transmit_nonce         = pc->transmit_nonce;
    cl73_nonce_match_val        = pc->cl73_nonce_match_val & 1;
    base_selector               = pc->base_selector & 0x1f;
  } else {
     no_an = 1 ; 
     printf("%-22s ERROR: u=%0d p=%0d Autoneg mode not defined\n", __func__, pc->unit, pc->port);
     return PHYMOD_E_FAIL;
  }
#endif /*_DV_TB_*/

 
  /* cl37 bam abilities 0xc182 */
  AN_X4_LOC_DEV_CL37_BAM_ABILr_CLR(reg_an_cl37_bam_abilities);
  AN_X4_LOC_DEV_CL37_BAM_ABILr_CL37_BAM_CODEf_SET(reg_an_cl37_bam_abilities, cl37_bam_code);
  AN_X4_LOC_DEV_CL37_BAM_ABILr_OVER1G_ABILITYf_SET(reg_an_cl37_bam_abilities, over1g_ability);
  AN_X4_LOC_DEV_CL37_BAM_ABILr_OVER1G_PAGE_COUNTf_SET(reg_an_cl37_bam_abilities, over1g_page_count);

  PHYMOD_IF_ERR_RETURN 
         (MODIFY_AN_X4_LOC_DEV_CL37_BAM_ABILr(pc, reg_an_cl37_bam_abilities));

  /* set next page  bit set 0xc186 */
  /*
  TSCE_READ_AN_LOCAL_DEVICE_CL73_BASE_ABILITIES_0r(pc, &reg_an_cl73_abl_0);
  TSCE_AN_LOCAL_DEVICE_CL73_BASE_ABILITIES_0r_NEXT_PAGEf_SET(reg_an_cl73_abl_0, next_page);
  PHYMOD_IF_ERR_RETURN ( TSCE_WRITE_AN_LOCAL_DEVICE_CL73_BASE_ABILITIES_0r(pc, reg_an_cl73_abl_0));
 */

  /* set cl73_bam_code set 0xc187 */
  /*

  TSCE_READ_AN_LOCAL_DEVICE_CL73_BAM_ABILITIESr(pc, &reg_an_cl73_abl);
  TSCE_AN_LOCAL_DEVICE_CL73_BAM_ABILITIESr_CL73_BAM_CODEf_SET(reg_an_cl73_abl, cl73_bam_code);
  PHYMOD_IF_ERR_RETURN ( TSCE_WRITE_AN_LOCAL_DEVICE_CL73_BAM_ABILITIESr(pc, reg_an_cl73_abl));
  */
  /* disable ecc of uC 0xffcc */
  /* 
      TSCE_COMMAND3r_SET(reg_com3, 0x0c04);
      PHYMOD_IF_ERR_RETURN (TSCE_WRITE_COMMAND3r(pc, reg_com3)); 
  */

  /* enable micro_controller 0xffc2 */
  /*
    TSCE_READ_COMMANDr(pc, &reg_com);
    TSCE_COMMANDr_MICRO_MDIO_DW8051_RESET_Nf_SET(reg_com,1);
                UC_COMMAND_MDIO_UC_RESET_N_MASK));
  */ 
   /* set cl73 nonce set 0xc340 */
   AN_X4_LOC_DEV_CL73_BASE_ABIL1r_CLR(reg_an_cl73_abl_1);
   AN_X4_LOC_DEV_CL73_BASE_ABIL1r_CL73_NONCE_MATCH_OVERf_SET(reg_an_cl73_abl_1, cl73_nonce_match_over);
   AN_X4_LOC_DEV_CL73_BASE_ABIL1r_CL73_NONCE_MATCH_VALf_SET(reg_an_cl73_abl_1, cl73_nonce_match_val);
   PHYMOD_IF_ERR_RETURN ( MODIFY_AN_X4_LOC_DEV_CL73_BASE_ABIL1r(pc, reg_an_cl73_abl_1)); 
  
   /* set cl73 base_selector to 802.3 set 0xc185 */
   AN_X4_LOC_DEV_CL73_BASE_ABIL1r_CLR(reg_an_cl73_abl_1); 
   AN_X4_LOC_DEV_CL73_BASE_ABIL1r_BASE_SELECTORf_SET(reg_an_cl73_abl_1, base_selector);
   PHYMOD_IF_ERR_RETURN ( MODIFY_AN_X4_LOC_DEV_CL73_BASE_ABIL1r(pc, reg_an_cl73_abl_1)); 
  
   AN_X4_LOC_DEV_CL73_BASE_ABIL1r_CLR(reg_an_cl73_abl_1); 
   AN_X4_LOC_DEV_CL73_BASE_ABIL1r_TRANSMIT_NONCEf_SET(reg_an_cl73_abl_1, ( cl73_transmit_nonce & 0x1f));
   PHYMOD_IF_ERR_RETURN ( MODIFY_AN_X4_LOC_DEV_CL73_BASE_ABIL1r(pc, reg_an_cl73_abl_1));

  /* 0x924a for PHY_892 TBD*/ 
  /*if((no_an==0)&&(pc->port_type==TSCMOD_SINGLE_PORT)) {*/
     data = 0x1a0a ; /* 100ms TX_RESET_TIMER_PERIOD */
     data = 0xd05 ;  /* 50ms */
  /*} else {
     data = 0x5 ;
  }
    PHYMOD_IF_ERR_RETURN 
    TSC_REG_WRITE(pc->unit, pc, 0x00, 0x0000924a, data)) ;*/

    /*Disabling AN if already enabled required for dynamic speed change tests*/

    AN_X4_ENSr_CLR(reg_an_enb);

    AN_X4_ENSr_CL37_ENABLEf_SET(reg_an_enb,0);
    AN_X4_ENSr_CL73_ENABLEf_SET(reg_an_enb,0);
    PHYMOD_IF_ERR_RETURN (MODIFY_AN_X4_ENSr(pc, reg_an_enb)); 

    /*Setting X4 abilities*/
    AN_X4_ENSr_CLR(reg_an_enb);
    AN_X4_ENSr_CL37_BAM_ENABLEf_SET(reg_an_enb,cl37_bam_enable);
    AN_X4_ENSr_CL73_BAM_ENABLEf_SET(reg_an_enb,cl73_bam_enable);
    AN_X4_ENSr_CL73_HPAM_ENABLEf_SET(reg_an_enb,cl73_hpam_enable);
    AN_X4_ENSr_CL73_ENABLEf_SET(reg_an_enb,cl73_enable);
    AN_X4_ENSr_CL37_SGMII_ENABLEf_SET(reg_an_enb,cl37_sgmii_enable);
    AN_X4_ENSr_CL37_ENABLEf_SET(reg_an_enb,cl37_enable);
    AN_X4_ENSr_CL37_AN_RESTARTf_SET(reg_an_enb,cl37_restart);
    AN_X4_ENSr_CL73_AN_RESTARTf_SET(reg_an_enb,cl73_restart);
    AN_X4_ENSr_HPAM_TO_CL73_AUTO_ENABLEf_SET(reg_an_enb,pc->hpam_to_cl73_auto_enable);
    AN_X4_ENSr_CL73_BAM_TO_HPAM_AUTO_ENABLEf_SET(reg_an_enb,pc->cl73_bam_to_hpam_auto_enable);
    AN_X4_ENSr_SGMII_TO_CL37_AUTO_ENABLEf_SET(reg_an_enb,pc->sgmii_to_cl37_auto_enable);
    AN_X4_ENSr_CL37_BAM_TO_SGMII_AUTO_ENABLEf_SET(reg_an_enb,pc->cl37_bam_to_sgmii_auto_enable);
    AN_X4_ENSr_NUM_ADVERTISED_LANESf_SET(reg_an_enb,num_advertised_lanes);

  PHYMOD_IF_ERR_RETURN (MODIFY_AN_X4_ENSr(pc, reg_an_enb)); 

  return PHYMOD_E_NONE;
} /* temod_autoneg_control */
#endif
#ifdef _SDK_TEMOD_
int temod_autoneg_control(PHYMOD_ST* pc, temod_an_control_t *an_control)
{
  uint16_t num_advertised_lanes, cl37_bam_enable, cl73_bam_enable  ;
  uint16_t cl73_hpam_enable, cl73_enable, cl37_sgmii_enable, cl37_enable;
  /* uint16_t cl73_bam_code; */
  uint16_t cl73_next_page;
  uint16_t cl37_next_page;
  uint16_t cl37_bam_code;
  uint16_t cl37_restart, cl73_restart ;

  AN_X4_LOC_DEV_CL37_BASE_ABILr_t reg_an_cl37_abl;
  AN_X4_LOC_DEV_CL37_BAM_ABILr_t reg_an_cl37_bam_abilities;
  AN_X4_LOC_DEV_CL73_BASE_ABIL0r_t reg_an_cl73_abl_0;
  AN_X4_ENSr_t reg_an_enb;
  AN_X4_CTLSr_t an_x4_abl_ctrl;

  AN_X1_CL37_ERRr_t      reg_cl37_err;
  AN_X1_CL73_ERRr_t      reg_cl73_err;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  num_advertised_lanes          = an_control->num_lane_adv;  
  cl37_bam_enable               = 0x0;
  cl73_bam_enable               = 0x0;
  cl73_hpam_enable              = 0x0;
  cl73_enable                   = 0x0;
  cl37_sgmii_enable             = 0x0;
  cl37_enable                   = 0x0;
  /* cl73_bam_code                 = 0x0; */
  cl73_next_page                = 0x0;
  cl37_next_page                = 0x0;
  cl37_bam_code                 = 0x0;
  cl37_restart                  = 0x0;
  cl73_restart                  = 0x0;

  switch (an_control->an_type) {
      case TEMOD_CL73:  
      {  
        cl73_restart                = an_control->enable;
        cl73_enable                 = an_control->enable; 
        break;
      }
      case TEMOD_CL37:
        cl37_restart                = an_control->enable;
        cl37_enable                 = an_control->enable;
        cl37_bam_code               = 0x0;
        break;
      case TEMOD_CL37_BAM:
        cl37_restart                = an_control->enable;
        cl37_enable                 = an_control->enable;
        cl37_bam_enable             = 0x1;
        cl37_bam_code               = 0x1;
        cl37_next_page              = 0x1;
        break;
      case TEMOD_CL37_SGMII:
        cl37_restart                = an_control->enable;
        cl37_sgmii_enable           = an_control->enable;
        cl37_enable                 = an_control->enable;
        cl37_bam_code               = 0x0;
        break;
      case TEMOD_CL73_BAM:
        cl73_restart                = an_control->enable;
        cl73_enable                 = an_control->enable; 
        cl73_bam_enable             = an_control->enable;
        /* cl73_bam_code               = 0x4; */
        cl73_next_page              = 0x1;
        break;
      case TEMOD_HPAM:
        cl73_restart                = an_control->enable;
        cl73_enable                 = an_control->enable; 
        cl73_hpam_enable            = an_control->enable;
        /* cl73_bam_code               = 0x4; */
        cl73_next_page              = 0x1;
        break;
     default:
        return PHYMOD_E_FAIL;
        break;
  }

    /* RESET Speed Change bit */
    if(an_control->enable){
      temod_disable_set(pc);
    }

    
     /* AN TIMERS */
     /*0x9252 AN_X1_TIMERS_cl37_error */
     AN_X1_CL37_ERRr_CLR(reg_cl37_err);
     if(an_control->an_type == TEMOD_AN_MODE_CL37){
       AN_X1_CL37_ERRr_SET(reg_cl37_err, 0);
     } else if (an_control->an_type == TEMOD_AN_MODE_CL37BAM) {
       AN_X1_CL37_ERRr_SET(reg_cl37_err, 0x55d);
     }
     PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL37_ERRr(pc, reg_cl37_err));

     /*0x9254 AN_X1_TIMERS_cl73_error*/ 
      AN_X1_CL73_ERRr_CLR(reg_cl73_err);
      if(an_control->an_type == TEMOD_AN_MODE_CL73) {
         AN_X1_CL73_ERRr_SET(reg_cl73_err, 0);
      } else if(an_control->an_type == TEMOD_AN_MODE_HPAM){
         AN_X1_CL73_ERRr_SET(reg_cl73_err, 0xfff0);
      } else if (an_control->an_type == TEMOD_AN_MODE_CL73BAM) {
         AN_X1_CL73_ERRr_SET(reg_cl73_err, 0x1a10);
      }
      PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL73_ERRr(pc, reg_cl73_err));


      /* cl37 bam abilities 0xc182 */
      AN_X4_LOC_DEV_CL37_BAM_ABILr_CLR(reg_an_cl37_bam_abilities);
      AN_X4_LOC_DEV_CL37_BAM_ABILr_CL37_BAM_CODEf_SET(reg_an_cl37_bam_abilities, cl37_bam_code);
      PHYMOD_IF_ERR_RETURN
           (MODIFY_AN_X4_LOC_DEV_CL37_BAM_ABILr(pc, reg_an_cl37_bam_abilities));
      
      /*need to set the next page for cl37bam */
      AN_X4_LOC_DEV_CL37_BASE_ABILr_CLR(reg_an_cl37_abl);
      if(an_control->an_property_type & TEMOD_AN_PROPERTY_ENABLE_SGMII_MASTER_MODE )
         AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_MASTER_MODEf_SET(reg_an_cl37_abl, 0x1);
      else
         AN_X4_LOC_DEV_CL37_BASE_ABILr_SGMII_MASTER_MODEf_SET(reg_an_cl37_abl, 0x0);
      AN_X4_LOC_DEV_CL37_BASE_ABILr_CL37_NEXT_PAGEf_SET(reg_an_cl37_abl, cl37_next_page);
      PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LOC_DEV_CL37_BASE_ABILr(pc, reg_an_cl37_abl));

      /*need to set cl73 BAM next page probably*/
      AN_X4_LOC_DEV_CL73_BASE_ABIL0r_CLR(reg_an_cl73_abl_0);
      AN_X4_LOC_DEV_CL73_BASE_ABIL0r_NEXT_PAGEf_SET(reg_an_cl73_abl_0, cl73_next_page & 1); 
      PHYMOD_IF_ERR_RETURN ( MODIFY_AN_X4_LOC_DEV_CL73_BASE_ABIL0r(pc, reg_an_cl73_abl_0));


      AN_X4_CTLSr_CLR(an_x4_abl_ctrl);
      AN_X4_CTLSr_PD_KX_ENf_SET(an_x4_abl_ctrl, an_control->pd_kx_en & 0x1);
      AN_X4_CTLSr_PD_KX4_ENf_SET(an_x4_abl_ctrl, an_control->pd_kx_en & 0x1);

      AN_X4_ENSr_CLR(reg_an_enb);

      AN_X4_ENSr_CL37_ENABLEf_SET(reg_an_enb,0);
      AN_X4_ENSr_CL73_ENABLEf_SET(reg_an_enb,0);
      AN_X4_ENSr_CL37_AN_RESTARTf_SET(reg_an_enb,0);
      AN_X4_ENSr_CL73_AN_RESTARTf_SET(reg_an_enb,0);
      PHYMOD_IF_ERR_RETURN (MODIFY_AN_X4_ENSr(pc, reg_an_enb));

      /*Setting X4 abilities*/
      AN_X4_ENSr_CLR(reg_an_enb);
      AN_X4_ENSr_CL37_BAM_ENABLEf_SET(reg_an_enb,cl37_bam_enable);
      AN_X4_ENSr_CL73_BAM_ENABLEf_SET(reg_an_enb,cl73_bam_enable);
      AN_X4_ENSr_CL73_HPAM_ENABLEf_SET(reg_an_enb,cl73_hpam_enable);
      AN_X4_ENSr_CL73_ENABLEf_SET(reg_an_enb,cl73_enable);
      AN_X4_ENSr_CL37_SGMII_ENABLEf_SET(reg_an_enb,cl37_sgmii_enable);
      AN_X4_ENSr_CL37_ENABLEf_SET(reg_an_enb,cl37_enable);
      AN_X4_ENSr_CL37_AN_RESTARTf_SET(reg_an_enb,cl37_restart);
      AN_X4_ENSr_CL73_AN_RESTARTf_SET(reg_an_enb,cl73_restart);
      if(an_control->an_property_type & TEMOD_AN_PROPERTY_ENABLE_HPAM_TO_CL73_AUTO )
        AN_X4_ENSr_HPAM_TO_CL73_AUTO_ENABLEf_SET(reg_an_enb,0x1);
      else
        AN_X4_ENSr_HPAM_TO_CL73_AUTO_ENABLEf_SET(reg_an_enb,0x0);
      if(an_control->an_property_type & TEMOD_AN_PROPERTY_ENABLE_CL73_BAM_TO_HPAM_AUTO )
        AN_X4_ENSr_CL73_BAM_TO_HPAM_AUTO_ENABLEf_SET(reg_an_enb,0x1);
      else
        AN_X4_ENSr_CL73_BAM_TO_HPAM_AUTO_ENABLEf_SET(reg_an_enb,0x0);
      if(an_control->an_property_type & TEMOD_AN_PROPERTY_ENABLE_SGMII_TO_CL37_AUTO )
        AN_X4_ENSr_SGMII_TO_CL37_AUTO_ENABLEf_SET(reg_an_enb,0x1);
      else
        AN_X4_ENSr_SGMII_TO_CL37_AUTO_ENABLEf_SET(reg_an_enb,0x0);
      if(an_control->an_property_type & TEMOD_AN_PROPERTY_ENABLE_CL37_BAM_to_SGMII_AUTO )
        AN_X4_ENSr_CL37_BAM_TO_SGMII_AUTO_ENABLEf_SET(reg_an_enb,0x1);
      else
        AN_X4_ENSr_CL37_BAM_TO_SGMII_AUTO_ENABLEf_SET(reg_an_enb,0x0);
      AN_X4_ENSr_NUM_ADVERTISED_LANESf_SET(reg_an_enb,num_advertised_lanes);

      PHYMOD_IF_ERR_RETURN (MODIFY_AN_X4_ENSr(pc, reg_an_enb));
      /* Disable the cl72 , when AN port is disabled. */
      if(an_control->enable == 0) {
       temod_clause72_control(pc, 0);
      }
  return PHYMOD_E_NONE;
} /* temod_autoneg_control */
#endif /* _SDK_TEMOD_*/

#ifdef _DV_TB_
/*
@brief   Speeds up DSC and VGA functions for simulation purpose only
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion.
@details This function speeds up PLL, DSC and VGA functions for any speed mode.
Please don't call this function in normal operation.  Set
#PHYMOD_ST::per_lane_control to 1 for speed up.

This function is controlled by the following register bit:

\li acq2_timeout, acq1_timeout, acqcdr_timeout (0xc210) -> 0x0421
\li acqvga_timeout (0xc211) -> 0x0800

*/
int temod_afe_speed_up_dsc_vga(PHYMOD_ST* pc, int pmd_touched)   /* AFE_SPEED_UP_DSC_VGA */
{
PLL_CAL_CTL0r_t    reg_ctrl0;
PLL_CAL_CTL1r_t    reg_ctrl1;
PLL_CAL_CTL2r_t    reg_ctrl2;
PLL_CAL_CTL5r_t    reg_ctrl5;
PLL_CAL_CTL6r_t    reg_ctrl6;
DSC_SM_CTL4r_t    reg_ctrl4;

TEMOD_DBG_IN_FUNC_INFO(pc);
PLL_CAL_CTL5r_CLR(reg_ctrl5);
PLL_CAL_CTL6r_CLR(reg_ctrl6);
PLL_CAL_CTL0r_CLR(reg_ctrl0);
PLL_CAL_CTL1r_CLR(reg_ctrl1);
PLL_CAL_CTL2r_CLR(reg_ctrl2);
DSC_SM_CTL4r_CLR(reg_ctrl4);

  if (pmd_touched == 0) {
/* PLL speed_up begin */
  /* refclk_divcnt = 5 (default 16'h186a ) */
  PLL_CAL_CTL5r_REFCLK_DIVCNTf_SET(reg_ctrl5, 0x5);
  PHYMOD_IF_ERR_RETURN (MODIFY_PLL_CAL_CTL5r(pc, reg_ctrl5));

  /* refclk_divcnt_sel = 7 */
  /* data = 0x7 << PLL_CAL_COM_CTL_6_REFCLK_DIVCNT_SEL_SHIFT ; */
  PLL_CAL_CTL6r_REFCLK_DIVCNT_SELf_SET(reg_ctrl6, 0x7);
  PHYMOD_IF_ERR_RETURN (MODIFY_PLL_CAL_CTL6r(pc, reg_ctrl6));

  /* vco_start_time = 5 */
  /* data = 0x5 << PLL_CAL_COM_CTL_0_VCO_START_TIME_SHIFT ; */
  PLL_CAL_CTL0r_VCO_START_TIMEf_SET(reg_ctrl0, 0x5);
  PHYMOD_IF_ERR_RETURN (MODIFY_PLL_CAL_CTL0r(pc, reg_ctrl0));

  /* pre_freq_det_time = 8'd100 , retry_time = 8'd100 */
  PLL_CAL_CTL1r_PRE_FREQ_DET_TIMEf_SET(reg_ctrl1, 0xc8);
  PLL_CAL_CTL1r_RETRY_TIMEf_SET(reg_ctrl1, 0xff);
  PHYMOD_IF_ERR_RETURN (MODIFY_PLL_CAL_CTL1r(pc, reg_ctrl1));

  /* res_cal_cntr = 0 win_cal_cntr=1 */
  PLL_CAL_CTL2r_RES_CAL_CNTRf_SET(reg_ctrl2, 0x2);
  PLL_CAL_CTL2r_WIN_CAL_CNTRf_SET(reg_ctrl2, 0x1);

  PHYMOD_IF_ERR_RETURN (MODIFY_PLL_CAL_CTL2r(pc, reg_ctrl2));

  }
  /* PLL speed_up end */
  /* DSC speed_up begin */

  DSC_SM_CTL4r_HW_TUNE_TIMEOUTf_SET(reg_ctrl4, 0x1);
  DSC_SM_CTL4r_CDR_SETTLE_TIMEOUTf_SET(reg_ctrl4, 0x1);
  DSC_SM_CTL4r_ACQ_CDR_TIMEOUTf_SET(reg_ctrl4, 0x4);

  PHYMOD_IF_ERR_RETURN (MODIFY_DSC_SM_CTL4r(pc, reg_ctrl4));

  /* DSC speed_up end */
  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */

static int _temod_getRevDetails(PHYMOD_ST* pc)
{
  MAIN0_SERDESIDr_t reg_serdesid;

  MAIN0_SERDESIDr_CLR(reg_serdesid);
  PHYMOD_IF_ERR_RETURN(READ_MAIN0_SERDESIDr(pc,&reg_serdesid));
  return (MAIN0_SERDESIDr_GET(reg_serdesid));
}

#ifdef _DV_TB_
/**
@brief   Set the PLL divider parameter in PMD for desired link speed.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   pmd_touched   Is this the first time we are visiting the PMD.
@param   spd_intf  #temod_spd_intfc_type
@param   pll_mode to override the pll div 
@returns PHYMOD_E_NONE if successful. PHYMOD_E_FAIL else.
@details This function is called once per TSCE. It cannot be called multiple times
and should be called immediately after reset. Elements of #PHYMOD_ST should be
initialized to required values prior to calling this function. The following
sub-configurations are performed here.

\li Set pll divider for VCO setting in PMD. pll divider is calculated from max_speed. 
*/

int temod_set_pll_mode(PHYMOD_ST* pc, int pmd_touched, temod_spd_intfc_type spd_intf, int pll_mode)           /* SET_PLL_MODE */
{
  PLL_CAL_CTL7r_t    reg_ctl7;
  int speed;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("pmd_touched: %d, spd_intf: %d, pll_mode: %d", pmd_touched, spd_intf, pll_mode));

  PLL_CAL_CTL7r_CLR(reg_ctl7);
  if (pmd_touched == 0) {
    get_mapped_speed(spd_intf, &speed);
    /*Support Override PLL DIV */
    if(pll_mode & TEMOD_OVERRIDE_CFG) {
      PLL_CAL_CTL7r_PLL_MODEf_SET(reg_ctl7, (pll_mode) & 0x0000ffff);
    } else {
       PLL_CAL_CTL7r_PLL_MODEf_SET(reg_ctl7, (eagle_sc_pmd_entry[speed].pll_mode)); 
    }

#ifdef _DV_TB_
    if( pc->verbosity) {
      printf("%-22s: plldiv:%d data:%x\n", __func__, pc->plldiv, PLL_CAL_CTL7r_PLL_MODEf_GET(reg_ctl7));
      printf("%-22s: main0_setup=%x\n", __func__, PLL_CAL_CTL7r_PLL_MODEf_GET(reg_ctl7));
    }
#endif
    PHYMOD_IF_ERR_RETURN (MODIFY_PLL_CAL_CTL7r(pc, reg_ctl7));
  }
  return PHYMOD_E_NONE;
} /* temod_set_pll_mode(PHYMOD_ST* pc) */
#endif /* _DV_TB_ */


/**
@brief   This function gets the PLL divider information
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   pll_div the reference which is updated with the pll divider value
@returns PHYMOD_E_NONE if successful.
@details Get the PLL divider value.
*/
int temod_get_plldiv(PHYMOD_ST *pc,  uint32_t *pll_div)
{
  PLL_CAL_CTL7r_t    reg_ctl7;
  READ_PLL_CAL_CTL7r(pc, &reg_ctl7);
  *pll_div = PLL_CAL_CTL7r_PLL_MODEf_GET(reg_ctl7);
  TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("pll_div: %d", *pll_div));

  return PHYMOD_E_NONE;
}

#ifdef _DV_TB_
/**
@brief   set the AN tick overrides
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   tick_override
@param   numerator
@param   denominator
@returns PHYMOD_E_NONE if successful.
@details

**/
int temod_tick_override_set(PHYMOD_ST* pc, int tick_override, int numerator, int denominator)
{
  MAIN0_TICK_CTL1r_t    reg_ctrl1;
  MAIN0_TICK_CTL0r_t    reg_ctrl0;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  MAIN0_TICK_CTL1r_CLR(reg_ctrl1);
  MAIN0_TICK_CTL0r_CLR(reg_ctrl0);

  /*Tick over_write enable*/
  MAIN0_TICK_CTL1r_TICK_OVERRIDEf_SET(reg_ctrl1, tick_override);
  MAIN0_TICK_CTL1r_TICK_NUMERATOR_UPPERf_SET(reg_ctrl1, (numerator & 0x7fff0));
  PHYMOD_IF_ERR_RETURN(WRITE_MAIN0_TICK_CTL1r(pc, reg_ctrl1));

  MAIN0_TICK_CTL0r_TICK_DENOMINATORf_SET(reg_ctrl0, (denominator & 0x3ff) );
  MAIN0_TICK_CTL0r_TICK_NUMERATOR_LOWERf_SET(reg_ctrl0, (numerator & 0xf));
  PHYMOD_IF_ERR_RETURN(WRITE_MAIN0_TICK_CTL0r(pc, reg_ctrl0));

  return PHYMOD_E_NONE;

}
#endif /* _DV_TB_ */

#ifdef _DV_TB_
/**
@brief   Set the AN Portmode and Single Port for AN of TSCE.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns PHYMOD_E_NONE if successful. PHYMOD_E_FAIL else.
@details

This function can be called multiple times.Elements of #PHYMOD_ST should be
initialized to required values prior to calling this function. The following
sub-configurations are performed here.

\li Set pll divider for VCO setting (0x9000[12, 11:8]). pll divider is calculated from max_speed. 

*/
int temod_set_an_port_mode(PHYMOD_ST* pc)    /* SET_AN_PORT_MODE */
{
  uint16_t data, mask,rdata;
  PHYMOD_ST  pc_10G_X1;
  PHYMOD_ST*  pc_10G_X1_tmp;
  PHYMOD_ST  pc_40G_X4;
  PHYMOD_ST*  pc_40G_X4_tmp;
  PHYMOD_ST  pc_100G_X4;
  PHYMOD_ST*  pc_100G_X4_tmp;

  MAIN0_SETUPr_t reg_main0_setup; 
  AN_X1_CL37_RESTARTr_t reg_cl37_restart_timers;
  AN_X1_CL37_ACKr_t reg_cl37_ack_timers;
  AN_X1_CL37_ERRr_t reg_cl37_err_timers;
  AN_X1_CL73_BRK_LNKr_t reg_cl73_brk_lnk_timers;
  AN_X1_CL73_ERRr_t reg_cl73_err_timers;
  MAIN0_TICK_CTL1r_t reg_tick_ctrl;
  MAIN0_TICK_CTL0r_t reg_tick_ctrl0;

  AN_X1_CFG_CTLr_t reg_an_retry_cnt;
  AN_X1_BAM_SPD_PRI_35_30r_t reg_pri35_30;
  AN_X1_BAM_SPD_PRI_29_24r_t reg_pri29_24;
  AN_X1_BAM_SPD_PRI_23_18r_t reg_pri23_18;
  AN_X1_BAM_SPD_PRI_17_12r_t reg_pri17_12;
  AN_X1_BAM_SPD_PRI_11_6r_t reg_pri11_6;
  AN_X1_BAM_SPD_PRI_5_0r_t reg_bam_pri5;
  AN_X1_OUI_LWRr_t reg_an_oui_lower;
  AN_X1_OUI_UPRr_t reg_an_oui_upper;
  SC_X1_TX_RST_CNTr_t reg_sc_rst_cnt;
  AN_X1_SGMII_CL73_TMR_TYPEr_t reg_an_cl73_sgmii_tmr;
  AN_X1_IGNORE_LNK_TMRr_t reg_an_ig_link_tmr;
  AN_X1_PD_TO_CL37_LNK_WAIT_TMRr_t reg_an_pd_to_cl37;
  AN_X1_CL37_SYNC_STS_FILTER_TMRr_t reg_an_sync_stat_fil_timer;
  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_t reg_lk_fail_not72;
  AN_X1_CL73_DME_LOCKr_t reg_cl73_dme;
  AN_X1_LNK_UPr_t reg_an_lp;
  AN_X1_LNK_FAIL_INHBT_TMR_CL72r_t reg_lk_fail;
  AN_X1_PD_SD_TMRr_t reg_an_pd_sd_tmr;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  MAIN0_SETUPr_CLR(reg_main0_setup);

  data=0;
  mask=0;
  /*Ref clock selection*/
  data = main0_refClkSelect_clk_156p25MHz;
  /*Selectng div 40 for CL37*/
  /*if(pc->sc_mode == TEMOD_SC_MODE_AN_CL37)
    data = data | (MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div40 <<
    MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_SHIFT); */
  /*Selectng div 66 for CL77*/
  /*if(pc->sc_mode == TEMOD_SC_MODE_AN_CL73)
    data = data | (MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_div66 <<
    MAIN0_SETUP_DEFAULT_PLL_MODE_AFE_SHIFT);*/
    
  /*For AN port mode should be always QUAD*/

  PHYMOD_IF_ERR_RETURN (READ_MAIN0_SETUPr(pc, &reg_main0_setup));
  rdata = MAIN0_SETUPr_PORT_MODE_SELf_GET(reg_main0_setup);
  
  MAIN0_SETUPr_REFCLK_SELf_SET(reg_main0_setup, data);
  data = 0;
  if((pc->no_of_lanes == 2) || (pc->no_of_lanes == 3)) {
    MAIN0_SETUPr_PORT_MODE_SELf_SET(reg_main0_setup, 0);
  } else {
    if(pc->this_lane == 0 || pc->this_lane == 1 || pc->this_lane == 4 || pc->this_lane == 5 || pc->this_lane == 8 || pc->this_lane == 9){
      if(rdata == 0x1 || rdata == 0x2 || rdata == 0x3){
        data = data | 0x1;
      }  
      if(rdata == 0x4){
        data = data | 0x0;
      }  
    }
    if(pc->this_lane == 2 || pc->this_lane == 3 || pc->this_lane == 6 || pc->this_lane == 7 || pc->this_lane == 10 || pc->this_lane == 11){
      if(rdata == 0x1 || rdata == 0x2 || rdata == 0x3){
        data = data | 0x2 ;
      }  
      if(rdata == 0x4){
        data = data | 0x0 ;
      }  
    }
    MAIN0_SETUPr_PORT_MODE_SELf_SET(reg_main0_setup, data);
  }
  
  PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(pc,reg_main0_setup));

  /*Tick over_write enable*/
  MAIN0_TICK_CTL1r_CLR(reg_tick_ctrl);
  MAIN0_TICK_CTL0r_CLR(reg_tick_ctrl0);

  MAIN0_TICK_CTL1r_TICK_OVERRIDEf_SET(reg_tick_ctrl, pc->an_tick_override);
  MAIN0_TICK_CTL1r_TICK_NUMERATOR_UPPERf_SET(reg_tick_ctrl, (pc->an_tick_numerator & 0x7fff0) >> 4);
  PHYMOD_IF_ERR_RETURN(WRITE_MAIN0_TICK_CTL1r(pc, reg_tick_ctrl));

  data=0;
  if((pc->sc_mode == TEMOD_SC_MODE_AN_CL37))
    {
       /*Setting numerator to 0x10 and denominator to 0x1 for CL37*/
       MAIN0_TICK_CTL0r_TICK_DENOMINATORf_SET(reg_tick_ctrl0,(pc->an_tick_denominator & 0x3ff));
       MAIN0_TICK_CTL0r_TICK_NUMERATOR_LOWERf_SET(reg_tick_ctrl0,(pc->an_tick_numerator & 0xf));
       PHYMOD_IF_ERR_RETURN(WRITE_MAIN0_TICK_CTL0r(pc, reg_tick_ctrl0));
    }
  if((pc->sc_mode == TEMOD_SC_MODE_AN_CL73))
    {
       /*Setting numerator to 0x80 and denominator to 0x1 for CL37*/
       MAIN0_TICK_CTL0r_TICK_DENOMINATORf_SET(reg_tick_ctrl0,(pc->an_tick_denominator & 0x3ff));
       MAIN0_TICK_CTL0r_TICK_NUMERATOR_LOWERf_SET(reg_tick_ctrl0,(pc->an_tick_numerator & 0xf));
       PHYMOD_IF_ERR_RETURN(WRITE_MAIN0_TICK_CTL0r(pc, reg_tick_ctrl0));
    }


#if defined (_DV_TB_)
/*AN from ST */
  if(pc->per_lane_control & 0x4) {
     pc_10G_X1 = *pc;
     pc_10G_X1.speed = digital_operationSpeeds_SPEED_10G_KR1; 
     pc_10G_X1_tmp = &pc_10G_X1;
     temod_get_ht_entries(pc_10G_X1_tmp);
     if(pc->cl72_en & TEMOD_OVERRIDE_CFG)
       pc_10G_X1_tmp->cl72_en = 0x0;
     else  
       pc_10G_X1_tmp->cl72_en = 0x1;
     pc_10G_X1_tmp->cl72_en = pc_10G_X1_tmp->cl72_en | 0x40000000;
     _configure_st_entry(1, digital_operationSpeeds_SPEED_40G_KR4, pc_10G_X1_tmp); 
  }
  else if(pc->per_lane_control & 0x8) {
     pc_40G_X4 = *pc;
     pc_40G_X4.speed = digital_operationSpeeds_SPEED_40G_KR4; 
     pc_40G_X4_tmp = &pc_40G_X4;
     temod_get_ht_entries(pc_40G_X4_tmp);
     if(pc->cl72_en & TEMOD_OVERRIDE_CFG)
       pc_40G_X4_tmp->cl72_en = 0x0;
     else  
       pc_40G_X4_tmp->cl72_en = 0x1;
     pc_40G_X4_tmp->cl72_en = pc_40G_X4_tmp->cl72_en | 0x40000000;
     _configure_st_entry(1, digital_operationSpeeds_SPEED_10G_KR1, pc_40G_X4_tmp); 
  }
  else if(pc->per_lane_control & 0x20) {
     pc_100G_X4 = *pc;
     pc_100G_X4.speed = digital_operationSpeeds_SPEED_100M; 
     pc_100G_X4_tmp = &pc_100G_X4;
     temod_get_ht_entries(pc_100G_X4_tmp);
     pc_100G_X4_tmp->cl72_en = pc_100G_X4_tmp->cl72_en | 0x40000000;
     pc_100G_X4_tmp->this_lane = 0;
     pc_100G_X4_tmp->prt_ad = 1;
     _configure_st_entry(1, digital_operationSpeeds_SPEED_100G_CR10, pc_100G_X4_tmp); 
     pc_100G_X4_tmp->this_lane = 4;
     pc_100G_X4_tmp->prt_ad = 5;
     _configure_st_entry(1, digital_operationSpeeds_SPEED_100G_CR10, pc_100G_X4_tmp); 
     pc_100G_X4_tmp->this_lane = 8;
     pc_100G_X4_tmp->prt_ad = 9;
     _configure_st_entry(1, digital_operationSpeeds_SPEED_100G_CR10, pc_100G_X4_tmp); 
  }
#endif


/*0x9250 AN_X1_TIMERS_cl37_restart */
  AN_X1_CL37_RESTARTr_CLR(reg_cl37_restart_timers);
  data = pc->cl37_restart_timer_period;
  if(data == 0)
    data = 0x1f;
  AN_X1_CL37_RESTARTr_CL37_RESTART_TIMER_PERIODf_SET(reg_cl37_restart_timers, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL37_RESTARTr(pc, reg_cl37_restart_timers));
/*0x9251 AN_X1_TIMERS_cl37_ack */
  AN_X1_CL37_ACKr_CLR(reg_cl37_ack_timers);
  data = pc->cl37_ack_timer_period;
  if(data == 0)
    data = 0x1f;
  AN_X1_CL37_ACKr_CL37_ACK_TIMER_PERIODf_SET(reg_cl37_ack_timers, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL37_ACKr(pc, reg_cl37_ack_timers));
/*0x9252 AN_X1_TIMERS_cl37_error */
  AN_X1_CL37_ERRr_CLR(reg_cl37_err_timers);
  data = pc->cl37_error_timer_period;
  AN_X1_CL37_ERRr_CL37_ERROR_TIMER_PERIODf_SET(reg_cl37_err_timers, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL37_ERRr(pc, reg_cl37_err_timers));
/*0x9253 AN_X1_TIMERS_cl73_break_link */
  AN_X1_CL73_BRK_LNKr_CLR(reg_cl73_brk_lnk_timers);
  data = pc->cl73_break_link_timer_period;
  AN_X1_CL73_BRK_LNKr_TX_DISABLE_TIMER_PERIODf_SET(reg_cl73_brk_lnk_timers, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL73_BRK_LNKr(pc, reg_cl73_brk_lnk_timers));
/*0x9254 AN_X1_TIMERS_cl73_error*/ 
  AN_X1_CL73_ERRr_CLR(reg_cl73_err_timers);
  data = pc->cl73_error_timer_period;
  AN_X1_CL73_ERRr_CL73_ERROR_TIMER_PERIODf_SET(reg_cl73_err_timers, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL73_ERRr(pc, reg_cl73_err_timers));
/*TBD::0x9255 AN_X1_TIMERS_cl73_dme_lock*/ 
  data = 100;
  AN_X1_CL73_DME_LOCKr_CLR(reg_cl73_dme);
  AN_X1_CL73_DME_LOCKr_PD_DME_LOCK_TIMER_PERIODf_SET(reg_cl73_dme, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL73_DME_LOCKr(pc, reg_cl73_dme));
/*TBD::0x9256 AN_X1_TIMERS_link_up*/ 
  data = 32;
  AN_X1_LNK_UPr_CLR(reg_an_lp);
  AN_X1_LNK_UPr_CL73_LINK_UP_TIMER_PERIODf_SET(reg_an_lp, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_UPr(pc, reg_an_lp));
/*0x9257 AN_X1_TIMERS_link_fail_inhibit_timer_cl72*/ 
  data = pc->link_fail_inhibit_timer_cl72_period;
  AN_X1_LNK_FAIL_INHBT_TMR_CL72r_CLR(reg_lk_fail);
  AN_X1_LNK_FAIL_INHBT_TMR_CL72r_LINK_FAIL_INHIBIT_TIMER_CL72_PERIODf_SET(reg_lk_fail, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72r(pc, reg_lk_fail));
/*0x9258 AN_X1_TIMERS_link_fail_inhibit_timer_not_cl72*/ 
  data = pc->link_fail_inhibit_timer_not_cl72_period;
  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_CLR(reg_lk_fail_not72);
  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_LINK_FAIL_INHIBIT_TIMER_NCL72_PERIODf_SET(reg_lk_fail_not72, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r(pc, reg_lk_fail_not72));
/*0x9259 AN_X1_TIMERS_pd_sd_timer*/ 
  data = pc->pd_sd_timer_period;
  AN_X1_PD_SD_TMRr_CLR(reg_an_pd_sd_tmr);
  AN_X1_PD_SD_TMRr_PD_SD_TIMER_PERIODf_SET(reg_an_pd_sd_tmr, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_PD_SD_TMRr(pc, reg_an_pd_sd_tmr));
/*0x925a AN_X1_TIMERS_cl72_max_wait_timer*/ 
  data = pc->cl72_max_wait_timer;
  AN_X1_CL37_SYNC_STS_FILTER_TMRr_CLR(reg_an_sync_stat_fil_timer);
  AN_X1_CL37_SYNC_STS_FILTER_TMRr_CL37_SYNC_STATUS_FILTER_TIMER_PERIODf_SET(reg_an_sync_stat_fil_timer, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL37_SYNC_STS_FILTER_TMRr(pc, reg_an_sync_stat_fil_timer));
/*0x925b AN_X1_TIMERS_PD_TO_CL37_LINK_WAIT_TIMER*/ 
  data = pc->pd_to_cl37_wait_timer;
  AN_X1_PD_TO_CL37_LNK_WAIT_TMRr_CLR(reg_an_pd_to_cl37);
  AN_X1_PD_TO_CL37_LNK_WAIT_TMRr_PD_TO_CL37_LINK_WAIT_TIMERf_SET(reg_an_pd_to_cl37, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_PD_TO_CL37_LNK_WAIT_TMRr(pc, reg_an_pd_to_cl37));
/*0x925c AN_X1_TIMERS_ignore_link_timer*/ 
  data = pc->an_ignore_link_timer;
  AN_X1_IGNORE_LNK_TMRr_CLR(reg_an_ig_link_tmr);
  AN_X1_IGNORE_LNK_TMRr_IGNORE_LINK_TIMER_PERIODf_SET(reg_an_ig_link_tmr, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_IGNORE_LNK_TMRr(pc, reg_an_ig_link_tmr));
/*TBD::0x925d AN_X1_TIMERS_dme_page_timer*/  
/*  data = 100;
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_TIMERS_DME_PAGE_TIMERr(pc->unit, pc, data));
*/  
/*TBD::0x925e AN_X1_TIMERS_sgmii_cl73_timer_type*/ 
  data = 100;
  AN_X1_SGMII_CL73_TMR_TYPEr_CLR(reg_an_cl73_sgmii_tmr);
  AN_X1_SGMII_CL73_TMR_TYPEr_SGMII_TIMERf_SET(reg_an_cl73_sgmii_tmr, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_SGMII_CL73_TMR_TYPEr(pc, reg_an_cl73_sgmii_tmr));
/*TBD::0x9263 AN_X1_TIMERS_sgmii_cl73_timer_type*/ 
  data = pc->tx_reset_count;
  SC_X1_TX_RST_CNTr_CLR(reg_sc_rst_cnt);
  SC_X1_TX_RST_CNTr_TX_RESET_COUNTf_SET(reg_sc_rst_cnt, data);
  PHYMOD_IF_ERR_RETURN(WRITE_SC_X1_TX_RST_CNTr(pc, reg_sc_rst_cnt));
/*TBD::0x924a AN_X1_CONTROL_tx_reset_timer_period*/ 
  /*data = 20;
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CONTROL_TX_RESET_TIMER_PERIODr(pc->unit, pc,
  data)); */
/*TBD::0x924a AN_X1_CONTROL_pll_reset_timer */
  /* data = 20;
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CONTROL_TX_RESET_TIMER_PERIODr(pc->unit, pc,
  data)); */
/*0x9240 AN_X1_CONTROL_oui_upper  */
  data =  (pc->oui_upper_data & 0xff);
  AN_X1_OUI_UPRr_CLR(reg_an_oui_upper);
  AN_X1_OUI_UPRr_OUI_UPPER_DATAf_SET(reg_an_oui_upper, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_OUI_UPRr(pc, reg_an_oui_upper));
/*0x9241 AN_X1_CONTROL_oui_lower  */
  data =  ((pc->oui_lower_data));
  AN_X1_OUI_LWRr_CLR(reg_an_oui_lower);
  AN_X1_OUI_LWRr_OUI_LOWER_DATAf_SET(reg_an_oui_lower, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_OUI_LWRr(pc, reg_an_oui_lower));
/*0x9242 AN_X1_CONTROL_BAM_SPEED_PRI_5_0  */
  data =  ((pc->bam_spd_pri_5_0));
  AN_X1_BAM_SPD_PRI_5_0r_CLR(reg_bam_pri5);
  AN_X1_BAM_SPD_PRI_5_0r_SET(reg_bam_pri5, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_BAM_SPD_PRI_5_0r(pc, reg_bam_pri5));
/*0x9243 AN_X1_CONTROL_BAM_SPEED_PRI_11_6  */
  data =  ((pc->bam_spd_pri_11_6));
  AN_X1_BAM_SPD_PRI_11_6r_CLR(reg_pri11_6);
  AN_X1_BAM_SPD_PRI_11_6r_SET(reg_pri11_6, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_BAM_SPD_PRI_11_6r(pc, reg_pri11_6));
/*0x9244 AN_X1_CONTROL_BAM_SPEED_PRI_17_12  */
  data =  ((pc->bam_spd_pri_17_12));
  AN_X1_BAM_SPD_PRI_17_12r_CLR(reg_pri17_12);
  AN_X1_BAM_SPD_PRI_17_12r_SET(reg_pri17_12, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_BAM_SPD_PRI_17_12r(pc, reg_pri17_12));
/*0x9245 AN_X1_CONTROL_BAM_SPEED_PRI_23_18  */
  data =  ((pc->bam_spd_pri_23_18) );
  AN_X1_BAM_SPD_PRI_23_18r_CLR(reg_pri23_18);
  AN_X1_BAM_SPD_PRI_23_18r_SET(reg_pri23_18, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_BAM_SPD_PRI_23_18r(pc, reg_pri23_18));
/*0x9246 AN_X1_CONTROL_BAM_SPEED_PRI_29_24  */
  data =  ((pc->bam_spd_pri_29_24));
  AN_X1_BAM_SPD_PRI_29_24r_CLR(reg_pri29_24);
  AN_X1_BAM_SPD_PRI_29_24r_SET(reg_pri29_24, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_BAM_SPD_PRI_29_24r(pc, reg_pri29_24));
/*0x9247 AN_X1_CONTROL_BAM_SPEED_PRI_35_30  */
  data =  ((pc->bam_spd_pri_35_30));
  AN_X1_BAM_SPD_PRI_35_30r_CLR(reg_pri35_30);
  AN_X1_BAM_SPD_PRI_35_30r_SET(reg_pri35_30, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_BAM_SPD_PRI_35_30r(pc, reg_pri35_30));
/*0x9248 AN_X1_CONTROL_CONFIG_CONTROL  */
  data =  pc->pd_to_cl37_retry_cnt;
  AN_X1_CFG_CTLr_CLR(reg_an_retry_cnt);
  AN_X1_CFG_CTLr_SET(reg_an_retry_cnt, data);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CFG_CTLr(pc, reg_an_retry_cnt));
  return PHYMOD_E_NONE;
} /* temod_set_an_port_mode(PHYMOD_ST* pc) */
#endif

#ifdef _SDK_TEMOD_
/**
@brief    Set the Port mode related fields for AN enabled ports
@param    pc handle to current TSCE context (#PHYMOD_ST)
@param    num_of_lanes Number of lanes in this port
@param    starting_lane first lane in the port
@param    single_port Single port or not
@returns  PHYMOD_E_NONE if successful. PHYMOD_E_FAIL else.
@details  This function sets up the port for an be called multiple times. Elements of #PHYMOD_ST
should be initialized to required values prior to calling this function. The
following sub-configurations are performed here.
*/
int temod_set_an_port_mode(PHYMOD_ST* pc, int num_of_lanes, int starting_lane, int single_port)    /* SET_AN_PORT_MODE */
{
  uint16_t rdata;
  uint32_t current_plldiv;
  MAIN0_SETUPr_t  reg_setup;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("num_of_lanes: %d, starting_lane: %d, single_port: %d", num_of_lanes, starting_lane, single_port));

  MAIN0_SETUPr_CLR(reg_setup);
  PHYMOD_IF_ERR_RETURN (READ_MAIN0_SETUPr(pc, &reg_setup));

  /*Ref clock selection tied to 156.25MHz */
  MAIN0_SETUPr_REFCLK_SELf_SET(reg_setup, 0x3);

  rdata = MAIN0_SETUPr_PORT_MODE_SELf_GET(reg_setup);
  
  if((num_of_lanes == 2) || (num_of_lanes == 3)) {
    MAIN0_SETUPr_PORT_MODE_SELf_SET(reg_setup, 0);
  } else {
    if(starting_lane == 0 || starting_lane == 1){
      if(rdata == 0x1 || rdata == 0x2 || rdata == 0x3){
              MAIN0_SETUPr_PORT_MODE_SELf_SET(reg_setup, 1);
      }  
      if(rdata == 0x4){
        MAIN0_SETUPr_PORT_MODE_SELf_SET(reg_setup, 0);
      }  
    }
    if(starting_lane == 2 || starting_lane == 3){
      if(rdata == 0x1 || rdata == 0x2 || rdata == 0x3){
       MAIN0_SETUPr_PORT_MODE_SELf_SET(reg_setup, 2);
      }  
      if(rdata == 0x4){
        MAIN0_SETUPr_PORT_MODE_SELf_SET(reg_setup, 0);
      }  
    }
  }
  /*Setting single port mode*/  
  if(single_port)  {
    MAIN0_SETUPr_SINGLE_PORT_MODEf_SET(reg_setup, 1);
  }

  /* Setting CL37_HVCO bit */
  temod_get_plldiv(pc,&current_plldiv);
  if(current_plldiv == 0xa)
     MAIN0_SETUPr_CL37_HIGH_VCOf_SET(reg_setup, 1);
  else
     MAIN0_SETUPr_CL37_HIGH_VCOf_SET(reg_setup, 0);


  PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(pc,reg_setup));

  return PHYMOD_E_NONE;
} /* temod_set_an_port_mode(PHYMOD_ST* pc, int num_of_lanes, int starting_lane, int single_port) */
#endif /* _SDK_TEMOD_ */

/**
@brief   Sets loopback mode at PCS/PMD parallel interface.(gloop).
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable controls enabling loopback
@param   starting_lane lane 0 of this port
@param   num_lanes Number of lanes of this port
@returns The value PHYMOD_E_NONE upon successful completion
@details This function sets the TX-to-RX digital loopback mode at the PCS/PMD parallel
interface, which is set based on port_type.

\li 0:1 : Enable  TX->RX loopback
\li 0:0 : Disable TX->RX loopback
*/
int temod_tx_loopback_control(PHYMOD_ST* pc, int enable, int starting_lane, int num_lanes)           /* TX_LOOPBACK_CONTROL  */
{
  MAIN0_LPBK_CTLr_t          reg_lb_ctrl;
  PMD_X4_OVRRr_t             reg_pmd_ovr;
  PMD_X4_CTLr_t              reg_pmd_ctrl;
  uint16_t                   lane_mask, i, data, tmp_data;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("enable: %d, starting_lane: %d, num_lane: %d", enable, starting_lane, num_lanes));

  lane_mask = 0;
  READ_MAIN0_LPBK_CTLr(pc, &reg_lb_ctrl);
  tmp_data = MAIN0_LPBK_CTLr_LOCAL_PCS_LOOPBACK_ENABLEf_GET(reg_lb_ctrl);
  PMD_X4_OVRRr_CLR(reg_pmd_ovr);
  PMD_X4_CTLr_CLR(reg_pmd_ctrl);

  lane_mask = 0;
  data = 0;
  for (i = 0; i < num_lanes; i++) {
    lane_mask |= 1 << (starting_lane + i);
    data |= enable << (starting_lane + i);
  }
  
  tmp_data &= ~lane_mask;
  tmp_data |= data;
  MAIN0_LPBK_CTLr_LOCAL_PCS_LOOPBACK_ENABLEf_SET(reg_lb_ctrl, tmp_data);   
#if 0
   switch(port_type) {
     case TEMOD_MULTI_PORT:   lane_mask = (1 << (starting_lane%4)); break;
     case TEMOD_TRI1_PORT:    lane_mask = ((starting_lane%4)==2)?0xc : (1 << (starting_lane%4)); break;
     case TEMOD_TRI2_PORT:    lane_mask = ((starting_lane%4)==0)?0x3 : (1 << (starting_lane%4)); break;
     case TEMOD_DXGXS:        lane_mask = ((starting_lane%4)==0)?0x3 : 0xc; break;
     case TEMOD_SINGLE_PORT:  lane_mask = 0xf; break;
     default: break ;
   }
   MAIN0_LPBK_CTLr_LOCAL_PCS_LOOPBACK_ENABLEf_SET(reg_lb_ctrl, (enable ? lane_mask: 0));
#endif

   PHYMOD_IF_ERR_RETURN
      (MODIFY_MAIN0_LPBK_CTLr(pc, reg_lb_ctrl));

/* signal_detect and rx_lock */
  if (enable) {
       PMD_X4_OVRRr_RX_LOCK_OVRDf_SET(reg_pmd_ovr, 1);
       PMD_X4_OVRRr_SIGNAL_DETECT_OVRDf_SET(reg_pmd_ovr, 1);
       PMD_X4_OVRRr_TX_DISABLE_OENf_SET(reg_pmd_ovr, 1);
  } else {
       PMD_X4_OVRRr_RX_LOCK_OVRDf_SET(reg_pmd_ovr, 0);
       PMD_X4_OVRRr_SIGNAL_DETECT_OVRDf_SET(reg_pmd_ovr, 0);
       PMD_X4_OVRRr_TX_DISABLE_OENf_SET(reg_pmd_ovr, 0);
  }

   PHYMOD_IF_ERR_RETURN
      (MODIFY_PMD_X4_OVRRr(pc, reg_pmd_ovr));

/* set tx_disable */
  if (enable) {
    PMD_X4_CTLr_TX_DISABLEf_SET(reg_pmd_ctrl, 1);
  } else {
    PMD_X4_CTLr_TX_DISABLEf_SET(reg_pmd_ctrl, 0);
  }
   /* mask = PMD_X4_CONTROL_TX_DISABLE_MASK ; */
   PHYMOD_IF_ERR_RETURN (MODIFY_PMD_X4_CTLr(pc, reg_pmd_ctrl));
  return PHYMOD_E_NONE;
}

int temod_tx_loopback_get(PHYMOD_ST* pc, uint32_t *enable)           /* TX_LOOPBACK_get  */
{
  MAIN0_LPBK_CTLr_t loopReg;
  READ_MAIN0_LPBK_CTLr(pc, &loopReg);
  *enable = MAIN0_LPBK_CTLr_LOCAL_PCS_LOOPBACK_ENABLEf_GET(loopReg);

  return PHYMOD_E_NONE;
}


#ifdef _DV_TB_
/*!
@brief   Sets loopback mode at PMD serial interface. (gloop(PCS) + PMD)
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   cntl select tx/rx loopback
@returns The value PHYMOD_E_NONE upon successful completion

This function sets the PMD TX-to-RX digital loopback mode.

\li 0:1 : Enable  TX->RX loopback
\li 0:0 : Disable TX->RX loopback

Note that this function can program <B>multiple lanes simultaneously</B>.
As an example, to enable gloop on all lanes,
set #PHYMOD_ST::per_lane_control to 0x01010101

*/
int temod_tx_pmd_loopback_control(PHYMOD_ST* pc, int cntl)           /* TX_PMD_LOOPBACK_CONTROL  */
{

  TLB_RX_DIG_LPBK_CFGr_t    reg_lpbk_cfg;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TLB_RX_DIG_LPBK_CFGr_CLR(reg_lpbk_cfg);
  READ_TLB_RX_DIG_LPBK_CFGr(pc, &reg_lpbk_cfg);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("cntl: %d", cntl));


   /*cntl = (pc-> pmd_gloop);*/
   if (cntl) {
      TLB_RX_DIG_LPBK_CFGr_DIG_LPBK_ENf_SET(reg_lpbk_cfg, 1);
   } else {
      TLB_RX_DIG_LPBK_CFGr_DIG_LPBK_ENf_SET(reg_lpbk_cfg, 0);
   }
   PHYMOD_IF_ERR_RETURN
      (WRITE_TLB_RX_DIG_LPBK_CFGr(pc, reg_lpbk_cfg));

  return PHYMOD_E_NONE;

}
#endif 

/**
@brief   Sets loopback mode at PMD serial interface. (pcs -> rloop)
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable Set of reset loopback
@param   starting_lane The first logical lane of this port
@param   port_type Port type (single/dual/quad)
@returns The value PHYMOD_E_NONE upon successful completion

This function sets the RX-to-TX(PCS)  Remote loopback mode, based on port_type.

\li 0:1 : Enable  RX->TX loopback
\li 0:0 : Disable RX->TX loopback
*/
int temod_rx_loopback_control(PHYMOD_ST* pc, int enable, int starting_lane, int port_type)           /* RX_LOOPBACK_CONTROL  */
{
   MAIN0_LPBK_CTLr_t                 reg_lb_ctrl;
   TLB_TX_TX_PI_LOOP_TIMING_CFGr_t   reg_lb_timing;
   TX_PI_CTL0r_t                     reg_com_ctrl;
   uint16_t lane_mask;

   TEMOD_DBG_IN_FUNC_INFO(pc);
   MAIN0_LPBK_CTLr_CLR(reg_lb_ctrl);
   TLB_TX_TX_PI_LOOP_TIMING_CFGr_CLR(reg_lb_timing);
   TX_PI_CTL0r_CLR(reg_com_ctrl);
   TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("enable: %d, starting_lane: %d, port_type: %d", enable, starting_lane, port_type));


   lane_mask = 0;
   switch(port_type) {
       case TEMOD_MULTI_PORT:   lane_mask = (1 << (starting_lane%4)); break;
       case TEMOD_TRI1_PORT:    lane_mask = ((starting_lane%4)==2)?0xc : (1 << (starting_lane%4)); break;
       case TEMOD_TRI2_PORT:    lane_mask = ((starting_lane%4)==0)?0x3 : (1 << (starting_lane%4)); break;
       case TEMOD_DXGXS:        lane_mask = ((starting_lane%4)==0)?0x3 : 0xc; break;
       case TEMOD_SINGLE_PORT:  lane_mask = 0xf; break;
       default: break ;
   }

   MAIN0_LPBK_CTLr_REMOTE_PCS_LOOPBACK_ENABLEf_SET(reg_lb_ctrl, (enable ?  lane_mask : 0 ));
   PHYMOD_IF_ERR_RETURN
        (MODIFY_MAIN0_LPBK_CTLr(pc, reg_lb_ctrl));

   /* set Tx_PI */
   TLB_TX_TX_PI_LOOP_TIMING_CFGr_TX_PI_LOOP_TIMING_SRC_SELf_SET(reg_lb_timing, 1);

   PHYMOD_IF_ERR_RETURN
        (MODIFY_TLB_TX_TX_PI_LOOP_TIMING_CFGr(pc, reg_lb_timing));

   TX_PI_CTL0r_TX_PI_ENf_SET(reg_com_ctrl, 1);
   PHYMOD_IF_ERR_RETURN
        (MODIFY_TX_PI_CTL0r(pc, reg_com_ctrl));

   return PHYMOD_E_NONE;
}

/**
@brief   set tx encoder of any particular lane
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   spd_intf Speed info as provided by #temod_spd_intfc_type
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function is used to select encoder of any transmit lane.
It selects the encode type based on the speed of the port.
*/
#ifdef _DV_TB_
int temod_encode_set(PHYMOD_ST* pc, temod_spd_intfc_type spd_intf)         /* ENCODE_SET */
{
  uint16_t  hg2_message_invalid_code_enable, hg2_codec, hg2_en ;
  TX_X4_ENC0r_t    reg_encode;
  int cntl;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("spd_intf: %d", spd_intf));
  TX_X4_ENC0r_CLR(reg_encode);


  printf("HiGig_Info : %-22s : pc->per_lane_control %08x : tx_higig2_en %08x : tx_higig2_codec %08x\n", __func__, pc->per_lane_control, pc->tx_higig2_en, pc->tx_higig2_codec) ;

  cntl = pc->per_lane_control;

  hg2_message_invalid_code_enable = 1 ;
  hg2_en            = 0 ;    /* 001 cl48 8b10b
                                010 cl48 8b10b rxaui
                                011 cl36 8b10b
                                100 cl82 64b66b
                                101 cl49 64b66b
                                110 brcm 64b66b */
  hg2_codec         = 0 ;

  if((spd_intf == TEMOD_SPD_10_SGMII) |
     (spd_intf == TEMOD_SPD_100_SGMII) |
     (spd_intf == TEMOD_SPD_1000_SGMII) |
     (spd_intf == TEMOD_SPD_2500)) {
      hg2_message_invalid_code_enable = 0x0 ;
  } else if((spd_intf == TEMOD_SPD_10000_XFI)||
            (spd_intf == TEMOD_SPD_10600_XFI_HG)||
            (spd_intf == TEMOD_SPD_5000)) {
     hg2_message_invalid_code_enable = 1 ;
  } else if (spd_intf == TEMOD_SPD_10000 ) {   /* 10G XAUI */
     hg2_message_invalid_code_enable = 0 ;
  } else if (spd_intf == TEMOD_SPD_10000_HI ) {   /* 10.6G HI XAUI */
     hg2_message_invalid_code_enable = 0 ;
  } else if (spd_intf == TEMOD_SPD_20G_MLD_DXGXS ) {
     hg2_message_invalid_code_enable = 1 ;
  } else if ((spd_intf ==TEMOD_SPD_21G_HI_MLD_DXGXS)||
             (spd_intf ==TEMOD_SPD_12773_HI_DXGXS)) {
     hg2_message_invalid_code_enable = 1 ;
     hg2_en            = 1 ;
     hg2_codec         = 1 ;
  } else if ((spd_intf == TEMOD_SPD_20G_DXGXS)||
             (spd_intf == TEMOD_SPD_21G_HI_DXGXS)) {
     hg2_message_invalid_code_enable = 0 ;
  } else if ((spd_intf == TEMOD_SPD_42G_X4)||
             (spd_intf == TEMOD_SPD_40G_X4)||
             (spd_intf == TEMOD_SPD_25455) ||
             (spd_intf == TEMOD_SPD_12773_DXGXS)) {
     hg2_message_invalid_code_enable = 0 ;
  } else if (spd_intf == TEMOD_SPD_40G_XLAUI ) {
     hg2_message_invalid_code_enable = 1 ;
  } else if (spd_intf == TEMOD_SPD_42G_XLAUI ) {
     hg2_message_invalid_code_enable = 1 ;
     hg2_en            = 1 ;
     hg2_codec         = 1 ;
  } else if (spd_intf == TEMOD_SPD_127G_HG_CR12) {
     hg2_message_invalid_code_enable = 1 ;
     hg2_en            = 1 ;
     hg2_codec         = 1 ;
  } else if (spd_intf == TEMOD_SPD_107G_HG_CR10) {
     hg2_message_invalid_code_enable = 1 ;
     hg2_en            = 1 ;
     hg2_codec         = 1 ;
  }

  /* NICK */
  if(cntl & TEMOD_OVERRIDE_CFG) {
    READ_TX_X4_ENC0r(pc, &reg_encode);
    printf("HiGig_Info : %-22s : pc->per_lane_control %08x : TX_X4_CONTROL0_ENCODE_0 0x%04x : tx_higig2_en %01x : tx_higig2_codec %01x\n", __func__, pc->per_lane_control,
               reg_encode, (TX_X4_ENC0r_HG2_ENABLEf_GET(reg_encode)) & 0x1, (TX_X4_ENC0r_HG2_CODECf_GET(reg_encode)) & 0x1) ;
     hg2_en    = pc->tx_higig2_en ;
     hg2_codec = pc->tx_higig2_codec ;
  }
  /* set encoder  0xc111 */
  TX_X4_ENC0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(reg_encode, hg2_message_invalid_code_enable);
     TX_X4_ENC0r_HG2_CODECf_SET(reg_encode, hg2_codec);
     TX_X4_ENC0r_HG2_ENABLEf_SET(reg_encode, hg2_en);

  PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_ENC0r(pc, reg_encode));

  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */

#ifdef _SDK_TEMOD_
int temod_encode_set(PHYMOD_ST* pc, temod_spd_intfc_type spd_intf)
{
  uint16_t  hg2_message_invalid_code_enable, hg2_codec, hg2_en ;
  TX_X4_ENC0r_t    reg_encode;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TX_X4_ENC0r_CLR(reg_encode);

  hg2_message_invalid_code_enable = 1 ;
  hg2_en            = 0 ;    /* 001 cl48 8b10b
                                010 cl48 8b10b rxaui
                                011 cl36 8b10b
                                100 cl82 64b66b
                                101 cl49 64b66b
                                110 brcm 64b66b */
  hg2_codec         = 0 ;

  if((spd_intf == TEMOD_SPD_10_SGMII) |
     (spd_intf == TEMOD_SPD_100_SGMII) |
     (spd_intf == TEMOD_SPD_1000_SGMII) |
     (spd_intf == TEMOD_SPD_2500)) {
      hg2_message_invalid_code_enable = 0x0 ;
  } else if((spd_intf == TEMOD_SPD_10000_XFI)||
            (spd_intf == TEMOD_SPD_10600_XFI_HG)||
            (spd_intf == TEMOD_SPD_5000)) {
     hg2_message_invalid_code_enable = 1 ;
  } else if (spd_intf == TEMOD_SPD_10000 ) {   /* 10G XAUI */
     hg2_message_invalid_code_enable = 0 ;
  } else if (spd_intf == TEMOD_SPD_10000_HI ) {   /* 10.6G HI XAUI */
     hg2_message_invalid_code_enable = 0 ;
  } else if (spd_intf == TEMOD_SPD_20G_MLD_DXGXS ) {
     hg2_message_invalid_code_enable = 1 ;
  } else if ((spd_intf ==TEMOD_SPD_21G_HI_MLD_DXGXS)||
             (spd_intf ==TEMOD_SPD_12773_HI_DXGXS)) {
     hg2_message_invalid_code_enable = 1 ;
     hg2_en            = 1 ;
     hg2_codec         = 1 ;
  } else if ((spd_intf == TEMOD_SPD_20G_DXGXS)||
             (spd_intf == TEMOD_SPD_21G_HI_DXGXS)) {
     hg2_message_invalid_code_enable = 0 ;
  } else if ((spd_intf == TEMOD_SPD_42G_X4)||
             (spd_intf == TEMOD_SPD_40G_X4)||
             (spd_intf == TEMOD_SPD_25455) ||
             (spd_intf == TEMOD_SPD_12773_DXGXS)) {
     hg2_message_invalid_code_enable = 0 ;
  } else if (spd_intf == TEMOD_SPD_40G_XLAUI ) {
     hg2_message_invalid_code_enable = 1 ;
  } else if (spd_intf == TEMOD_SPD_42G_XLAUI ) {
     hg2_message_invalid_code_enable = 1 ;
     hg2_en            = 1 ;
     hg2_codec         = 1 ;
  } else if (spd_intf == TEMOD_SPD_127G_HG_CR12) {
     hg2_message_invalid_code_enable = 1 ;
     hg2_en            = 1 ;
     hg2_codec         = 1 ;
  } else if (spd_intf == TEMOD_SPD_107G_HG_CR10) {
     hg2_message_invalid_code_enable = 1 ;
     hg2_en            = 1 ;
     hg2_codec         = 1 ;
  }

  /* set encoder  0xc111 */
  TX_X4_ENC0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(reg_encode, hg2_message_invalid_code_enable);
     TX_X4_ENC0r_HG2_CODECf_SET(reg_encode, hg2_codec);
     TX_X4_ENC0r_HG2_ENABLEf_SET(reg_encode, hg2_en);

  PHYMOD_IF_ERR_RETURN
      (MODIFY_TX_X4_ENC0r(pc, reg_encode));

  return PHYMOD_E_NONE;

}
#endif /* _SDK_TEMOD_ */

/**
@brief   set rx decoder of any particular lane
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   spd_intf Speed info as provided by #temod_spd_intfc_type
@returns The value PHYMOD_E_NONE upon successful completion.
@details This function is used to select decoder of any receive lane.
It selects the encode type based on the speed of the port.
*/

#ifdef _DV_TB_
int temod_decode_set(PHYMOD_ST* pc, temod_spd_intfc_type spd_intf)         /* DECODE_SET */
{
  uint16_t hg2_message_ivalid_code_enable, hg2_en, hg2_codec;
  RX_X4_DEC_CTL0r_t    reg_decode_ctrl;
  int cntl;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  RX_X4_DEC_CTL0r_CLR(reg_decode_ctrl);

  /* NICK */
  printf("HiGig_Info : %-22s : pc->per_lane_control %08x : rx_higig2_en %08x : rx_higig2_codec %08x\n", __func__, pc->per_lane_control, pc->rx_higig2_en, pc->rx_higig2_codec);

  /* NICK */
  cntl = pc->per_lane_control | 0x1 ;
  /* cntl = 1; */

  hg2_message_ivalid_code_enable = 0x1 ;
  hg2_en          = 0 ;
  hg2_codec       = 0 ;
  if (cntl) { /* set decoder */


    if(spd_intf == TEMOD_SPD_10_SGMII) {
        hg2_message_ivalid_code_enable = 0x0 ;
    } else if(spd_intf == TEMOD_SPD_100_SGMII) {
        hg2_message_ivalid_code_enable = 0x0 ;
    } else if(spd_intf == TEMOD_SPD_1000_SGMII) {
        hg2_message_ivalid_code_enable = 0x0 ;
    } else if(spd_intf == TEMOD_SPD_2500) {
        hg2_message_ivalid_code_enable = 0x0 ;
    } else if((spd_intf == TEMOD_SPD_10000_XFI)||
              (spd_intf == TEMOD_SPD_10600_XFI_HG)||
              (spd_intf == TEMOD_SPD_5000)) {
        hg2_message_ivalid_code_enable = 0x1 ;
    } else if( spd_intf == TEMOD_SPD_10000 ) {  /* 10G XAUI */
        hg2_message_ivalid_code_enable = 0x0 ;
    } else if( spd_intf == TEMOD_SPD_10000_HI ) {  /* 10G XAUI */
        hg2_message_ivalid_code_enable = 0x0 ;
    } else if( spd_intf == TEMOD_SPD_20G_MLD_DXGXS ) {
        hg2_message_ivalid_code_enable = 0x1 ;
    } else if((spd_intf ==TEMOD_SPD_21G_HI_MLD_DXGXS)||
              (spd_intf ==TEMOD_SPD_12773_HI_DXGXS)) {
        hg2_message_ivalid_code_enable = 0x1 ;
        hg2_en          = 1 ;
        hg2_codec       = 1 ;
    } else if((spd_intf == TEMOD_SPD_20G_DXGXS)||
              (spd_intf == TEMOD_SPD_21G_HI_DXGXS)) {
        hg2_message_ivalid_code_enable = 0x0 ;
    } else if((spd_intf == TEMOD_SPD_42G_X4) ||
              (spd_intf == TEMOD_SPD_40G_X4) ||
              (spd_intf == TEMOD_SPD_25455) ||
              (spd_intf == TEMOD_SPD_12773_DXGXS)) {
        hg2_message_ivalid_code_enable = 0x1 ;
    } else if( spd_intf == TEMOD_SPD_40G_XLAUI ) {
        hg2_message_ivalid_code_enable = 0x1 ;
    } else if( spd_intf == TEMOD_SPD_42G_XLAUI ) {  /* 40 MLD Hig2 */
        hg2_message_ivalid_code_enable = 0x1 ;
        hg2_en          = 1 ;
        hg2_codec       = 1 ;
    } else if( spd_intf == TEMOD_SPD_127G_HG_CR12 ) {
        hg2_message_ivalid_code_enable = 0x1 ;
        hg2_en          = 1 ;
        hg2_codec       = 1 ;
    } else if( spd_intf == TEMOD_SPD_107G_HG_CR10 ) {
        hg2_message_ivalid_code_enable = 0x1 ;
        hg2_en          = 1 ;
        hg2_codec       = 1 ;
    } else {
       printf("%-22s ERROR: p=%0d undefined spd_intf=%0d(%s)\n", __func__, pc->port, spd_intf,
              e2s_temod_spd_intfc_type[spd_intf]) ;
    }
    /* HG setting */
    if((pc->ctrl_type & TEMOD_CTRL_TYPE_HG)) {
       /* not CL48 */
       hg2_en          = 1 ;
       hg2_codec       = 1 ;
    }

    /* NICK */
    if(cntl & TEMOD_OVERRIDE_CFG) {
     READ_RX_X4_DEC_CTL0r(pc, &reg_decode_ctrl);
#ifdef _DV_TB_
     printf("HiGig_Info : %-22s : pc->per_lane_control %08x : RX_X4_CONTROL0_DECODE_CONTROL_0 0x%04x : rx_higig2_en %01x : rx_higig2_codec %01x\n", __func__, pc->per_lane_control, reg_decode_ctrl,
               (RX_X4_DEC_CTL0r_HG2_ENABLEf_GET(reg_decode_ctrl)) & 0x1, (RX_X4_DEC_CTL0r_HG2_CODECf_GET(reg_decode_ctrl)) & 0x1) ;
#endif

      hg2_en    = pc->rx_higig2_en ;
      hg2_codec = pc->rx_higig2_codec ;
    }

    if(pc->sc_mode != TEMOD_SC_MODE_AN_CL37 && pc->sc_mode !=TEMOD_SC_MODE_AN_CL73){

      RX_X4_DEC_CTL0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(reg_decode_ctrl, hg2_message_ivalid_code_enable);
         RX_X4_DEC_CTL0r_HG2_ENABLEf_SET(reg_decode_ctrl, hg2_en);
         RX_X4_DEC_CTL0r_HG2_CODECf_SET(reg_decode_ctrl, hg2_codec);
      PHYMOD_IF_ERR_RETURN
          (MODIFY_RX_X4_DEC_CTL0r(pc, reg_decode_ctrl));
    }
  } else {  /* if ~cntl */
    /* no op */
     return PHYMOD_E_NONE;
  }
  if ((pc->pmd_reset_control) != 0) {
    temod_pmd_reset_bypass(pc, 1);
  }
  return PHYMOD_E_NONE;
}  /* DECODE_SET */
#endif
#ifdef _SDK_TEMOD_
int temod_decode_set(PHYMOD_ST* pc, temod_spd_intfc_type spd_intf)
{
  uint16_t hg2_message_ivalid_code_enable, hg2_en, hg2_codec;
  RX_X4_DEC_CTL0r_t    reg_decode_ctrl;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("spd_intf: %d", spd_intf));
  RX_X4_DEC_CTL0r_CLR(reg_decode_ctrl);

   hg2_message_ivalid_code_enable = 0x1 ;
   hg2_en          = 0 ;
   hg2_codec       = 0 ;

   if(spd_intf == TEMOD_SPD_10_SGMII) {
     hg2_message_ivalid_code_enable = 0x0 ;
   } else if(spd_intf == TEMOD_SPD_100_SGMII) {
     hg2_message_ivalid_code_enable = 0x0 ;
   } else if(spd_intf == TEMOD_SPD_1000_SGMII) {
     hg2_message_ivalid_code_enable = 0x0 ;
   } else if(spd_intf == TEMOD_SPD_2500) {
     hg2_message_ivalid_code_enable = 0x0 ;
   } else if((spd_intf == TEMOD_SPD_10000_XFI)||
             (spd_intf == TEMOD_SPD_10600_XFI_HG)||
             (spd_intf == TEMOD_SPD_5000)) {
     hg2_message_ivalid_code_enable = 0x1 ;
   } else if( spd_intf == TEMOD_SPD_10000 ) {  /* 10G XAUI */
     hg2_message_ivalid_code_enable = 0x0 ;
   } else if( spd_intf == TEMOD_SPD_10000_HI ) {  /* 10G XAUI */
     hg2_message_ivalid_code_enable = 0x0 ;
   } else if( spd_intf == TEMOD_SPD_20G_MLD_DXGXS ) {
     hg2_message_ivalid_code_enable = 0x1 ;
   } else if((spd_intf ==TEMOD_SPD_21G_HI_MLD_DXGXS)||
             (spd_intf ==TEMOD_SPD_12773_HI_DXGXS)) {
     hg2_message_ivalid_code_enable = 0x1 ;
     hg2_en          = 1 ;
     hg2_codec       = 1 ;
   } else if((spd_intf == TEMOD_SPD_20G_DXGXS)||
             (spd_intf == TEMOD_SPD_21G_HI_DXGXS)) {
       hg2_message_ivalid_code_enable = 0x0 ;
   } else if((spd_intf == TEMOD_SPD_42G_X4) ||
             (spd_intf == TEMOD_SPD_40G_X4) ||
             (spd_intf == TEMOD_SPD_25455) ||
             (spd_intf == TEMOD_SPD_12773_DXGXS)) {
       hg2_message_ivalid_code_enable = 0x1 ;
   } else if( spd_intf == TEMOD_SPD_40G_XLAUI ) {
       hg2_message_ivalid_code_enable = 0x1 ;
   } else if( spd_intf == TEMOD_SPD_42G_XLAUI ) {  /* 40 MLD Hig2 */
       hg2_message_ivalid_code_enable = 0x1 ;
       hg2_en          = 1 ;
       hg2_codec       = 1 ;
   } else if( spd_intf == TEMOD_SPD_127G_HG_CR12 ) {
       hg2_message_ivalid_code_enable = 0x1 ;
       hg2_en          = 1 ;
       hg2_codec       = 1 ;
   } else if( spd_intf == TEMOD_SPD_107G_HG_CR10 ) {
       hg2_message_ivalid_code_enable = 0x1 ;
       hg2_en          = 1 ;
       hg2_codec       = 1 ;
   } else {
      return PHYMOD_E_FAIL;
   }
   RX_X4_DEC_CTL0r_HG2_MESSAGE_INVALID_CODE_ENABLEf_SET(reg_decode_ctrl, hg2_message_ivalid_code_enable);
   RX_X4_DEC_CTL0r_HG2_ENABLEf_SET(reg_decode_ctrl, hg2_en);
   RX_X4_DEC_CTL0r_HG2_CODECf_SET(reg_decode_ctrl, hg2_codec);
   PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_DEC_CTL0r(pc, reg_decode_ctrl));
   return PHYMOD_E_NONE;
}  /* DECODE_SET */
#endif /* SDK_TEMOD_ */

/*!
@brief   get  port speed id configured
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   speed_id Receives the the resolved speed cfg as recorded in h/w
@returns The value PHYMOD_E_NONE upon successful completion.
@details get  port speed configured
*/
int temod_speed_id_get(PHYMOD_ST* pc, int *speed_id)  
{
  SC_X4_RSLVD_SPDr_t sc_final_resolved_speed;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  SC_X4_RSLVD_SPDr_CLR(sc_final_resolved_speed);
  READ_SC_X4_RSLVD_SPDr(pc,&sc_final_resolved_speed);
  *speed_id = SC_X4_RSLVD_SPDr_SPEEDf_GET(sc_final_resolved_speed);
  TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("speed_id: %d", *speed_id));

  return PHYMOD_E_NONE;
}

/*!
@brief   get  port speed intf configured
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   spd_intf A pointer to speed type. 
@returns The value PHYMOD_E_NONE upon successful completion.
@details get  port speed configured in the PHY. The value returned
is the actual value in the resolved speed register and not the enumerated type
used in the driver.
*/
int temod_spd_intf_get(PHYMOD_ST* pc, int *spd_intf) 
{
  SC_X4_RSLVD_SPDr_t sc_final_resolved_speed;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  SC_X4_RSLVD_SPDr_CLR(sc_final_resolved_speed);
  READ_SC_X4_RSLVD_SPDr(pc,&sc_final_resolved_speed);
  *spd_intf = get_actual_speed(SC_X4_RSLVD_SPDr_SPEEDf_GET(sc_final_resolved_speed), spd_intf); 
  TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("spd_intf: %d", *spd_intf));
  return PHYMOD_E_NONE;
}

/*!
@brief   tx lane reset and enable of any particular lane
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable Enable/disable traffic or reset/unreset lane
@param   tx_dis_type enum type #tx_lane_disable_type_t
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
int temod_tx_lane_control(PHYMOD_ST* pc, int enable, tx_lane_disable_type_t tx_dis_type)         /* TX_LANE_CONTROL */
{
  TX_X4_MISCr_t    reg_misc_ctrl;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TX_X4_MISCr_CLR(reg_misc_ctrl);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("enable: %d, tx_dis_type: %d", enable, tx_dis_type));


  if (enable) {
    /* set encoder */
    TX_X4_MISCr_CLR(reg_misc_ctrl);
    TX_X4_MISCr_RSTB_TX_LANEf_SET(reg_misc_ctrl, 1);
    PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, reg_misc_ctrl));
    TX_X4_MISCr_CLR(reg_misc_ctrl);
    TX_X4_MISCr_ENABLE_TX_LANEf_SET(reg_misc_ctrl, 1);
    PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, reg_misc_ctrl));
  } else {
    /* data = 0 ;  */
    if(tx_dis_type & TEMOD_TX_LANE_TRAFFIC ){
      TX_X4_MISCr_ENABLE_TX_LANEf_SET(reg_misc_ctrl, 0);
    } else if (tx_dis_type & TEMOD_TX_LANE_RESET) {
      TX_X4_MISCr_RSTB_TX_LANEf_SET(reg_misc_ctrl, 0);
    } else {
      TX_X4_MISCr_ENABLE_TX_LANEf_SET(reg_misc_ctrl, 0);
      TX_X4_MISCr_RSTB_TX_LANEf_SET(reg_misc_ctrl, 0);
    }
     PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, reg_misc_ctrl));
  }
  return PHYMOD_E_NONE;
}

/**
@brief   Read the 16 bit rev. id value etc.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   *revid int ref. to return revision id to calling function.
@returns The value PHYMOD_E_NONE upon successful completion.
@details This function reads the revid register that contains core number,
revision number and returns the 16-bit value in the revid
*/

int temod_revid_read(PHYMOD_ST* pc, uint32_t *revid)              /* REVID_READ */
{
  TEMOD_DBG_IN_FUNC_INFO(pc);
  *revid=_temod_getRevDetails(pc);
  TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("revid: %x", *revid));
  return PHYMOD_E_NONE;
}

#ifdef _DV_TB_
/**
@brief   Read the 16 bit rev. id value etc.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per Port PCS Init

*/
int temod_init_pcs(PHYMOD_ST* pc)              /* INIT_PCS */
{
  TEMOD_DBG_IN_FUNC_INFO(pc);
  _temod_set_port_mode_select(pc);
  temod_rx_lane_control_set(pc, 1);
  temod_tx_lane_control(pc, 1, 0);         /* TX_LANE_CONTROL */

  /*Re-Visit, Assuming the value is non xero in case mld only */
  temod_mld_am_timers_set(pc, pc->rx_am_timer_init_val, pc->tx_am_timer_init_val);

  if((pc->sc_mode == TEMOD_SC_MODE_AN_CL37) || (pc->sc_mode == TEMOD_SC_MODE_AN_CL73))
    _init_pcs_an(pc);
  else /* Forced Speed */
    _init_pcs_fs(pc);
  
  return PHYMOD_E_NONE;
}
#endif

/**
@brief   Init the PMD
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   pmd_touched If the PMD is already initialized
@returns The value PHYMOD_E_NONE upon successful completion
@details Per core PMD resets (both datapath and entire core)
We only intend to use this function if the PMD has never been initialized.
*/
int temod_pmd_reset_seq(PHYMOD_ST* pc, int pmd_touched) /* PMD_RESET_SEQ */
{
  PMD_X1_CTLr_t reg_pmd_x1_ctrl;
  TEMOD_DBG_IN_FUNC_INFO(pc);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("pmd_touched: %x", pmd_touched));

  PMD_X1_CTLr_CLR(reg_pmd_x1_ctrl);
  
  if (pmd_touched == 0) {
    PMD_X1_CTLr_POR_H_RSTBf_SET(reg_pmd_x1_ctrl,0);
    PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(reg_pmd_x1_ctrl,0);
    PHYMOD_IF_ERR_RETURN(WRITE_PMD_X1_CTLr(pc,reg_pmd_x1_ctrl));
    PMD_X1_CTLr_POR_H_RSTBf_SET(reg_pmd_x1_ctrl,1);
    PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(reg_pmd_x1_ctrl,1);
    PHYMOD_IF_ERR_RETURN(WRITE_PMD_X1_CTLr(pc,reg_pmd_x1_ctrl));
  }
  return PHYMOD_E_NONE;
}

#ifdef _DV_TB_
/**
@brief   Wait for SC done bit set.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   data handle to return SC_X4_CONTROL_STATUS reg.
@returns The value PHYMOD_E_NONE upon successful completion
@details Wait for SC done bit set. This is a very short wait Hence we allow a
loop here. We never allow looping in tier1 as a rule. All looping should be done
in tier2 and above.
*/
int temod_wait_sc_done(PHYMOD_ST* pc, uint16_t *data)
{
  static int timeOut = 15;
  int localTimeOut = timeOut;
  SC_X4_STSr_t reg_sc_ctrl_sts;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  SC_X4_STSr_CLR(reg_sc_ctrl_sts);

  while(1) {
    localTimeOut--;
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_STSr(pc, &reg_sc_ctrl_sts));
    if((SC_X4_STSr_SW_SPEED_CHANGE_DONEf_GET(reg_sc_ctrl_sts) == 1) || localTimeOut < 1)
      break;
  }
  TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("sc_done: %x", *data));

  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */

/**
@brief   Control pram ablity
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable Enable value for PRAM set.
@returns The value PHYMOD_E_NONE upon successful completion
@details Per core PMD resets (both datapath and entire core)
We only intend to use this function if the PMD has never been initialized.
*/
int temod_pram_abl_enable_set(PHYMOD_ST* pc, int enable) /* PMD_RESET_SEQ */
{
  PMD_X1_CTLr_t reg_pmd_x1_ctrl;
  TEMOD_DBG_IN_FUNC_INFO(pc);

  PMD_X1_CTLr_CLR(reg_pmd_x1_ctrl);

  PMD_X1_CTLr_PRAM_ABILITYf_SET(reg_pmd_x1_ctrl,enable);
  PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_CTLr(pc,reg_pmd_x1_ctrl));

  return PHYMOD_E_NONE;
}



/**
@brief   Set the port speed and enable the port
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   spd_intf the speed to set the port enum #temod_set_spd_intf
@returns The value PHYMOD_E_NONE upon successful completion
@details Sets the port to the specified speed and triggers the port, and waits 
         for the port to be configured
*/
int temod_set_spd_intf(PHYMOD_ST *pc, temod_spd_intfc_type spd_intf)
{
  SC_X4_CTLr_t xgxs_x4_ctrl;
  phymod_access_t pa_copy;
  int speed_id = 0, start_lane = 0, num_lane = 0;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("spd_intf: %d", spd_intf));

  /* need to figure out what's the starting lane */
  PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
  PHYMOD_IF_ERR_RETURN
      (phymod_util_lane_config_get(pc, &start_lane, &num_lane));
  pa_copy.lane = 0x1 << start_lane; 

  PHYMOD_IF_ERR_RETURN (get_mapped_speed(spd_intf, &speed_id));
  /* write the speed_id into the speed_change register */
  SC_X4_CTLr_CLR(xgxs_x4_ctrl);
  SC_X4_CTLr_SW_SPEEDf_SET(xgxs_x4_ctrl, speed_id);
  MODIFY_SC_X4_CTLr(pc, xgxs_x4_ctrl);
  /*next call the speed_change routine */
  PHYMOD_IF_ERR_RETURN (temod_trigger_speed_change(&pa_copy));
  /*check the speed_change_done nit*/
  PHYMOD_IF_ERR_RETURN (_temod_wait_sc_stats_set(pc));

  return PHYMOD_E_NONE;
}

/**
@brief   Checks config valid status for the port 
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   port_num Port Number
@returns The value PHYMOD_E_NONE upon successful completion
@details This register bit indicates that PCS is now programmed as required by
the HT entry for that speed
*/
int temod_master_port_num_set( PHYMOD_ST *pc,  int port_num)
{
  MAIN0_SETUPr_t main_reg;
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc,("port_num: %d", port_num));

  MAIN0_SETUPr_CLR(main_reg);
  MAIN0_SETUPr_MASTER_PORT_NUMf_SET(main_reg, port_num);
  MODIFY_MAIN0_SETUPr(pc, main_reg);  

  return PHYMOD_E_NONE;
}

/**
@brief   update the port mode 
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   pll_restart Indicates if required to restart PLL.
@returns The value PHYMOD_E_NONE upon successful completion
*/
int temod_update_port_mode( PHYMOD_ST *pc, int *pll_restart)
{
    MAIN0_SETUPr_t mode_reg;
    int port_mode_sel, port_mode_sel_reg, temp_pll_restart;
    uint32_t single_port_mode;
    uint32_t first_couple_mode = 0, second_couple_mode = 0;

    port_mode_sel = 0 ;
    single_port_mode = 0 ;
    temp_pll_restart = 0; 

    READ_MAIN0_SETUPr(pc, &mode_reg);
    port_mode_sel_reg = MAIN0_SETUPr_PORT_MODE_SELf_GET(mode_reg);

    if(pc->lane == 0xf){
        port_mode_sel = 4;
        if( port_mode_sel_reg != 4){
            temp_pll_restart = 1;
        }
    } else{  
        first_couple_mode = ((port_mode_sel_reg == 2) || (port_mode_sel_reg == 3) || (port_mode_sel_reg == 4));
        second_couple_mode = ((port_mode_sel_reg == 1) || (port_mode_sel_reg == 3) || (port_mode_sel_reg == 4));
        switch(pc->lane){
        case 1:
        case 2:
            first_couple_mode = 0;
            break;
        case 4:
        case 8:
            second_couple_mode = 0;
            break;
        case 3:
            first_couple_mode = 1;
            break;
        case 0xc:
            second_couple_mode = 1;
            break;
        default:
        /* dprintf("%-22s: ERROR port_mode_sel=%0d undefined\n", 
             __func__, port_mode_sel_reg); */
            break ;
        }
        
        if(first_couple_mode ){
            port_mode_sel =(second_couple_mode)? 3: 2;
        }
        else if(second_couple_mode){
            port_mode_sel = 1;
        }
        else{
            port_mode_sel = 0 ;
        }
    }
    
    *pll_restart = temp_pll_restart;

    /*if(pc->verbosity & TEMOD_DBG_INIT)
     dprintf("%-22s u=%0d p=%0d port_mode_sel old=%0d new=%0d\n", __func__, 
            pc->unit, pc->port, port_mode_sel_reg, port_mode_sel) ; */

    MAIN0_SETUPr_SINGLE_PORT_MODEf_SET(mode_reg, single_port_mode);
    MAIN0_SETUPr_PORT_MODE_SELf_SET(mode_reg, port_mode_sel);
    MODIFY_MAIN0_SETUPr(pc, mode_reg);  
    TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("pll_restart: %d", *pll_restart));
    return PHYMOD_E_NONE ; 
}

/**
@brief   enable the pll reset bit  
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable pll reset enable or not 
@returns The value PHYMOD_E_NONE upon successful completion
*/
int temod_pll_reset_enable_set (PHYMOD_ST *pc , int enable)
{
  MAIN0_SETUPr_t main_reg;

  MAIN0_SETUPr_CLR(main_reg);
  MAIN0_SETUPr_PLL_RESET_ENf_SET(main_reg, enable);
  MODIFY_MAIN0_SETUPr(pc, main_reg);
  return PHYMOD_E_NONE ; 
}

/**
@brief    Checks config valid status for the port 
@param    pc handle to current TSCE context (#PHYMOD_ST)
@returns  The value PHYMOD_E_NONE upon successful completion
@details  This function is only used by other Tier1 functions. It is not exposed
to Tier2s. It reads a specific register bit that indicates that PCS has
configured all its blocks for the HT entry for the desired speed.
*/
int _temod_wait_sc_stats_set(PHYMOD_ST* pc)
{
  uint16_t data;
  uint16_t i;
  SC_X4_STSr_t reg_sc_ctrl_sts;

  SC_X4_STSr_CLR(reg_sc_ctrl_sts);
   
#ifdef _DV_TB_
  while(1){
   i=i+1; /* added only to eliminate compile warning */
   PHYMOD_IF_ERR_RETURN(READ_SC_X4_STSr(pc, &reg_sc_ctrl_sts));
   data = SC_X4_STSr_SW_SPEED_CONFIG_VLDf_GET(reg_sc_ctrl_sts);
   if(data == 1)
     break;
  }
  return PHYMOD_E_NONE;
#endif /* _DV_TB_ */
#ifdef _SDK_TEMOD_
  for (i= 0; i < 10; i++) {
    PHYMOD_USLEEP(1);   
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_STSr(pc, &reg_sc_ctrl_sts));
    data = SC_X4_STSr_SW_SPEED_CONFIG_VLDf_GET(reg_sc_ctrl_sts);
    if(data == 1)
        break;
  }
  if (data != 1) {
      return PHYMOD_E_TIMEOUT;
  }
  else {
      return PHYMOD_E_NONE;
  }
#endif /* _SDK_TEMOD_ */
}

/**
@brief   Read PCS Link status
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   *link Reference for Status of PCS Link
@returns The value PHYMOD_E_NONE upon successful completion
@details Return the status of the PCS link. The link up implies the PCS is able
to decode the digital bits coming in on the serdes. It automatically implies
that the PLL is stable and that the PMD sublayer has successfully recovered the
clock on the receive line.
*/
int temod_get_pcs_link_status(PHYMOD_ST* pc, uint32_t *link)
{
  RX_X4_PCS_LIVE_STSr_t reg_pcs_live_sts;
  
  TEMOD_DBG_IN_FUNC_INFO(pc);
  RX_X4_PCS_LIVE_STSr_CLR(reg_pcs_live_sts);
  PHYMOD_IF_ERR_RETURN (READ_RX_X4_PCS_LIVE_STSr(pc, &reg_pcs_live_sts));
  *link = RX_X4_PCS_LIVE_STSr_LINK_STATUSf_GET(reg_pcs_live_sts);
  TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("pcs_live_stats_link: %d", *link));

  return PHYMOD_E_NONE;
}

#ifdef _DV_TB_
/**
@brief   Read the 16 bit rev. id value etc.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Timers are set for verifying and speeding up Verification
*/
int temod_mld_am_timers_set(PHYMOD_ST* pc,int rx_am_timer_init_val,int tx_am_timer_init_val) /*MLD_AM_TIMERS_SET */
{
  CL82_RX_AM_TMRr_t reg_rx_am_timer;
  TX_X2_MLD_SWP_CNTr_t reg_mld_swap_cnt;
    
  TEMOD_DBG_IN_FUNC_INFO(pc);
  CL82_RX_AM_TMRr_CLR(reg_rx_am_timer);
  TX_X2_MLD_SWP_CNTr_CLR(reg_mld_swap_cnt);

  if(rx_am_timer_init_val != 0) {
    CL82_RX_AM_TMRr_SET(reg_rx_am_timer, rx_am_timer_init_val);
    PHYMOD_IF_ERR_RETURN (WRITE_CL82_RX_AM_TMRr(pc, reg_rx_am_timer));
  }
  if(tx_am_timer_init_val != 0) {
    TX_X2_MLD_SWP_CNTr_SET(reg_mld_swap_cnt, tx_am_timer_init_val);
    PHYMOD_IF_ERR_RETURN (WRITE_TX_X2_MLD_SWP_CNTr(pc, reg_mld_swap_cnt));
  }
  return PHYMOD_E_NONE;
}
#endif
  
/**
@brief   Get the Port status
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable receives disable status of the port.
@returns The value PHYMOD_E_NONE upon successful completion
@details Ports can be disabled in several ways. In this function we simply write
0 to the speed change which will bring the PCS down for that lane.

*/
int temod_disable_get(PHYMOD_ST* pc, uint32_t* enable)              
{
  SC_X4_CTLr_t reg_sc_ctrl;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  SC_X4_CTLr_CLR(reg_sc_ctrl);

  PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc,&reg_sc_ctrl));
  *enable = SC_X4_CTLr_SW_SPEED_CHANGEf_GET(reg_sc_ctrl);

  return PHYMOD_E_NONE;
}

/**
@brief   Get info on Disable status of the Port
@param  pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Disables the port by writing 0 to the speed config logic in PCS.
This makes the PCS to bring down the PCS blocks and also apply lane datapath
reset to the PMD. There is no control input to this function since it only does
one thing.

*/
int temod_disable_set(PHYMOD_ST* pc)             
{
  SC_X4_CTLr_t reg_sc_ctrl;
  
  TEMOD_DBG_IN_FUNC_INFO(pc);
  SC_X4_CTLr_CLR(reg_sc_ctrl); 

  /* write 0 to the speed change */
  SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg_sc_ctrl, 0);
  PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc,reg_sc_ctrl));

  return PHYMOD_E_NONE;
}

/**
@brief   Get the plldiv from lookup table
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   spd_intf speed as specified by enum #temod_spd_intfc_type
@param   plldiv Receives the pll divider value
@param   speed_vec  Receives the speed id.
@returns The value PHYMOD_E_NONE upon successful completion
@details Get the plldiv from lookup table
*/
int temod_plldiv_lkup_get(PHYMOD_ST* pc, temod_spd_intfc_type spd_intf, uint32_t *plldiv, uint16_t *speed_vec)
{
  int speed_id;

  get_mapped_speed(spd_intf, &speed_id);
  *plldiv = eagle_sc_pmd_entry[speed_id].pll_mode;
  *speed_vec = speed_id ;
  TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("plldiv: %d", *plldiv));

  return PHYMOD_E_NONE;
}

/**
@brief   Get the osmode from lookup table for the given speed
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   spd_intf the speed type as in enum #temod_spd_intfc_type
@param   osmode receives the osmode
@returns The value PHYMOD_E_NONE upon successful completion
@details Get the osmode from software copy of Speed table
*/
int temod_osmode_lkup_get(PHYMOD_ST* pc, temod_spd_intfc_type spd_intf, uint32_t *osmode)
{
  int speed_id;

  get_mapped_speed(spd_intf, &speed_id);
  *osmode = eagle_sc_pmd_entry[speed_id].t_pma_os_mode; 
  TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("osmode: %d", *osmode));

  return PHYMOD_E_NONE;
}

/**
@brief   Get the osdfe from lookup table for the given speed
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   spd_intf the speed type as in enum #temod_spd_intfc_type
@param   osdfe_on receives the osdfe_on
@returns The value PHYMOD_E_NONE upon successful completion
@details Get the osdfe_on from software copy of Speed table. Dfe_on:1, Dfe_off=0
*/
int temod_osdfe_on_lkup_get(PHYMOD_ST* pc, temod_spd_intfc_type spd_intf, uint32_t* osdfe_on)
{
  int speed_id;

  get_mapped_speed(spd_intf, &speed_id);
  *osdfe_on = eagle_sc_pmd_entry[speed_id].osdfe_on;
  TEMOD_DBG_IN_FUNC_VOUT_INFO(pc,("osdfe_on: %d", *osdfe_on));

  return PHYMOD_E_NONE;
}


#ifdef _DV_TB_
/**
@brief   Select the ILKN path and bypass TSCE PCS
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details This will enable ILKN path. PCS is set to bypass to relinquish PMD
control. Expect PMD to be programmed elsewhere. If we need the QSGMII PCS expect
the QSGMII to be already programmed at the port layer. If not, we will feed
garbage to the ILKN path.
*/
int temod_init_pcs_ilkn(PHYMOD_ST* pc)              /* INIT_PCS */
{
  SC_X4_BYPASSr_t reg_sc_bypass;
  ILKN_CTL0r_t reg_ilkn_ctrl0;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  SC_X4_BYPASSr_CLR(reg_sc_bypass);
  ILKN_CTL0r_CLR(reg_ilkn_ctrl0);

  SC_X4_BYPASSr_SC_BYPASSf_SET(reg_sc_bypass, 1);
  PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_BYPASSr(pc,reg_sc_bypass));

  ILKN_CTL0r_ILKN_SELf_SET(reg_ilkn_ctrl0, 1);
  ILKN_CTL0r_CREDIT_ENf_SET(reg_ilkn_ctrl0, 1);
  PHYMOD_IF_ERR_RETURN(MODIFY_ILKN_CTL0r(pc, reg_ilkn_ctrl0));

  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */


/**
@brief   PMD per lane reset
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per lane PMD ln_rst and ln_dp_rst by writing to PMD_X4_CONTROL in pcs space
*/
int temod_pmd_x4_reset(PHYMOD_ST* pc)              /* PMD_X4_RESET */
{
  PMD_X4_CTLr_t reg_pmd_x4_ctrl;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  PMD_X4_CTLr_CLR(reg_pmd_x4_ctrl);
  PMD_X4_CTLr_LN_H_RSTBf_SET(reg_pmd_x4_ctrl,0);
  PMD_X4_CTLr_LN_DP_H_RSTBf_SET(reg_pmd_x4_ctrl,0);
  PHYMOD_IF_ERR_RETURN (MODIFY_PMD_X4_CTLr(pc, reg_pmd_x4_ctrl));
  
  PMD_X4_CTLr_CLR(reg_pmd_x4_ctrl);
  PMD_X4_CTLr_LN_H_RSTBf_SET(reg_pmd_x4_ctrl,1);
  PMD_X4_CTLr_LN_DP_H_RSTBf_SET(reg_pmd_x4_ctrl,1);
  PHYMOD_IF_ERR_RETURN (MODIFY_PMD_X4_CTLr(pc, reg_pmd_x4_ctrl));

  return PHYMOD_E_NONE;
}

#ifdef _DV_TB_
/**
@brief   Initialize the PMD in independent mode. (No PCS involved)
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   pmd_touched Is the pmd already programmed (i.e. pll running)
@param   uc_active  Is the uctlr already active (implies firmware loaded)
@param   spd_intf enum #temod_spd_intfc_type 
@param   t_pma_os_mode per lane OS Mode in PMD
@returns The value PHYMOD_E_NONE upon successful completion
@details Per Port PMD Init done by software
*/
int temod_init_pmd_sw(PHYMOD_ST* pc, int pmd_touched, int uc_active,
                      temod_spd_intfc_type spd_intf,  int t_pma_os_mode)  /* INIT_PMD_SW */
{
  TEMOD_DBG_IN_FUNC_INFO(pc);
  temod_init_pmd(pc, pmd_touched,  uc_active);
  temod_pmd_osmode_set(pc, spd_intf,  t_pma_os_mode);

  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */


#ifdef _DV_TB_
/**
@brief   Initialize PMD. for both PCS and independent modes.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   pmd_touched If set, indicates PMD was previously initialized for another
port in the same core.
@param   uc_active implies the ucntlr is active
@returns PHYMOD_E_NONE upon success, PHYMOD_E_FAIL else.
@details Per Port PMD Init

*/
int temod_init_pmd(PHYMOD_ST* pc, int pmd_touched, int uc_active)   /* INIT_PMD */
{
  CKRST_LN_CLK_RST_N_PWRDWN_CTLr_t    reg_pwrdwn_ctrl;
  DIG_TOP_USER_CTL0r_t                    reg_usr_ctrl;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TEMOD_DBG_IN_FUNC_VIN_INFO(pc, ("pmd_touched: %d, uc_active: %d",pmd_touched, uc_active));

  CKRST_LN_CLK_RST_N_PWRDWN_CTLr_CLR(reg_pwrdwn_ctrl);
  DIG_TOP_USER_CTL0r_CLR(reg_usr_ctrl);

  CKRST_LN_CLK_RST_N_PWRDWN_CTLr_LN_DP_S_RSTBf_SET(reg_pwrdwn_ctrl, 1);
  PHYMOD_IF_ERR_RETURN(MODIFY_CKRST_LN_CLK_RST_N_PWRDWN_CTLr(pc, reg_pwrdwn_ctrl));

  if (pmd_touched == 0) {
    if(uc_active == 1){
      DIG_TOP_USER_CTL0r_UC_ACTIVEf_SET(reg_usr_ctrl, 1);
      DIG_TOP_USER_CTL0r_CORE_DP_S_RSTBf_SET(reg_usr_ctrl, 1);
      /* release reset to pll data path. TBD need for all lanes  and uc_active set */
      PHYMOD_IF_ERR_RETURN (MODIFY_DIG_TOP_USER_CTL0r(pc, reg_usr_ctrl));
    } else{
      DIG_TOP_USER_CTL0r_CORE_DP_S_RSTBf_SET(reg_usr_ctrl, 1);
      PHYMOD_IF_ERR_RETURN (MODIFY_DIG_TOP_USER_CTL0r(pc, reg_usr_ctrl));
    }
  }
  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */


/**
@brief   Set Per lane OS mode set in PMD
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   spd_intf speed type #temod_spd_intfc_type
@param   os_mode over sample rate.
@returns The value PHYMOD_E_NONE upon successful completion
@details Per Port PMD Init

*/
int temod_pmd_osmode_set(PHYMOD_ST* pc, temod_spd_intfc_type spd_intf, int os_mode)   /* INIT_PMD */
{
  CKRST_OSR_MODE_CTLr_t    reg_osr_mode;
  int speed;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  CKRST_OSR_MODE_CTLr_CLR(reg_osr_mode);
  get_mapped_speed(spd_intf, &speed);

  /*os_mode         = 0x0; */ /* 0= OS MODE 1;  1= OS MODE 2; 2=OS MODE 3; 
                             3=OS MODE 3.3; 4=OS MODE 4; 5=OS MODE 5; 
                             6=OS MODE 8;   7=OS MODE 8.25; 8: OS MODE 10*/
  if(os_mode & TEMOD_OVERRIDE_CFG) {
    os_mode =  (os_mode) & 0x0000ffff;
  }
 else {
    os_mode =  eagle_sc_pmd_entry[speed].t_pma_os_mode; 
  }

  CKRST_OSR_MODE_CTLr_OSR_MODE_FRCf_SET(reg_osr_mode, 1);
  CKRST_OSR_MODE_CTLr_OSR_MODE_FRC_VALf_SET(reg_osr_mode, os_mode);

  PHYMOD_IF_ERR_RETURN
     (MODIFY_CKRST_OSR_MODE_CTLr(pc, reg_osr_mode));

  return PHYMOD_E_NONE;
}

/**
@brief   Enable FEC for forced Speeds. Will be rewritten in FEC_control
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   fec_enable To enable or not.
@returns The value PHYMOD_E_NONE upon successful completion
@details Enable FEC for forced speeds.
*/
int temod_fecmode_set(PHYMOD_ST* pc, int fec_enable)              /* FECMODE_SET */
{
	TX_X4_MISCr_t        reg_misc;
	RX_X4_DEC_CTL0r_t    reg_decode;

	TEMOD_DBG_IN_FUNC_INFO(pc);
	TX_X4_MISCr_CLR(reg_misc);
	RX_X4_DEC_CTL0r_CLR(reg_decode);

    TX_X4_MISCr_FEC_ENABLEf_SET(reg_misc, fec_enable);
    PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_MISCr(pc, reg_misc));

    RX_X4_DEC_CTL0r_BLOCK_SYNC_MODEf_SET(reg_decode, (fec_enable << 2));
    PHYMOD_IF_ERR_RETURN (MODIFY_RX_X4_DEC_CTL0r(pc, reg_decode));
	return PHYMOD_E_NONE;
}

/**
@brief   Get FEC Setting for forced Speeds. Will be rewritten in FEC_control
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   fec_enable To get status
@returns The value PHYMOD_E_NONE upon successful completion
@details FEC Status for forced speeds.
*/
int temod_fecmode_get(PHYMOD_ST* pc, uint32_t *fec_enable)              /* FECMODE_GET */
{
  TX_X4_MISCr_t        reg_misc;
  SC_X4_CTLr_t    reg;
  AN_X4_ENSr_t reg_an_enb;
  AN_X4_AN_ABIL_RESOLUTION_STSr_t an_hcd_status;


  TEMOD_DBG_IN_FUNC_INFO(pc);
  TX_X4_MISCr_CLR(reg_misc);
  SC_X4_CTLr_CLR(reg);
  AN_X4_ENSr_CLR(reg_an_enb);

  PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc, &reg));
  PHYMOD_IF_ERR_RETURN (READ_AN_X4_ENSr(pc, &reg_an_enb));

  *fec_enable = 0;
  if(AN_X4_ENSr_CL73_ENABLEf_GET(reg_an_enb) == 1){
    READ_AN_X4_AN_ABIL_RESOLUTION_STSr(pc,&an_hcd_status);
    *fec_enable = AN_X4_AN_ABIL_RESOLUTION_STSr_AN_HCD_FECf_GET(an_hcd_status);
  } else if(SC_X4_CTLr_SW_SPEED_CHANGEf_GET(reg) == 1) {
    PHYMOD_IF_ERR_RETURN (READ_TX_X4_MISCr(pc, &reg_misc));
    *fec_enable = TX_X4_MISCr_FEC_ENABLEf_GET(reg_misc);
  }

  return PHYMOD_E_NONE;
}


/**
@brief   Generic Override mechanism. Any PCS parameter can be overridden.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   or_type enum type #override_type_t
@param   or_val Value to override. (could be 1/0 or actual value)
@returns The value PHYMOD_E_NONE upon successful completion
@details The cntl encoding is as follows.
          NUM_LANES_OEN |
          OS_MODE_OEN |
          FEC_ENABLE_OEN |
          DESKEWMODE_OEN |
          DESC2_MODE_OEN |
          CL36BYTEDELETEMODE_OEN |
          BRCM64B66_DESCRAMBLER_ENABLE_OEN |
          CHK_END_EN_OEN |
          BLOCK_SYNC_MODE_OEN |
          DECODERMODE_OEN |
          ENCODEMODE_OEN |
          DESCRAMBLERMODE_OEN |
          SCR_MODE_OEN
*/
int temod_override_set(PHYMOD_ST* pc, override_type_t or_type, uint16_t or_val)   /* OVERRIDE_SET */
{
  int or_value;
  SC_X4_FLD_OVRR_EN0_TYPEr_t reg_or_en0;
  SC_X4_FLD_OVRR_EN1_TYPEr_t reg_or_en1;
  CL72_LNK_CTLr_t reg_cl72_link_ctrl;
  SC_X4_LN_NUM_OVRRr_t  reg_ln_num_or_val;
  RX_X4_PMA_CTL0r_t reg_os_mode_or_val;
  TX_X4_MISCr_t reg_misc_or_val;
  RX_X4_PCS_CTL0r_t reg_pcs_or_val;
  RX_X2_MISC0r_t reg_misc0_or_val;
  RX_X4_DEC_CTL0r_t reg_dec_or_val;
  RX_X4_CL36_RX0r_t reg_cl36_or_val;
  TX_X4_ENC0r_t reg_encode_or_val;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  SC_X4_FLD_OVRR_EN0_TYPEr_CLR(reg_or_en0);
  SC_X4_FLD_OVRR_EN1_TYPEr_CLR(reg_or_en1);

  or_value = or_val & 0x0000ffff;
  switch(or_type) {
  case TEMOD_OVERRIDE_RESET :
   /* reset credits */
   WRITE_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);

   SC_X4_FLD_OVRR_EN1_TYPEr_CL72_EN_OENf_SET(reg_or_en1, 0);
   SC_X4_FLD_OVRR_EN1_TYPEr_REORDER_EN_OENf_SET(reg_or_en1, 0);
   SC_X4_FLD_OVRR_EN1_TYPEr_CL36_EN_OENf_SET(reg_or_en1, 0);
   MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc,reg_or_en1);
  break;
  case TEMOD_OVERRIDE_ALL :
    /* get info. from table and apply it to all credit regs. */
  break;
  case TEMOD_OVERRIDE_SPDID :
  break;
  case TEMOD_OVERRIDE_CL72 :
       CL72_LNK_CTLr_CLR(reg_cl72_link_ctrl);
       CL72_LNK_CTLr_LINK_CONTROL_FORCEVALf_SET(reg_cl72_link_ctrl, or_value);
       MODIFY_CL72_LNK_CTLr(pc,reg_cl72_link_ctrl);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_CL72_EN_OENf_SET(reg_or_en1, 1);
       MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc,reg_or_en1);
  break;
  case TEMOD_OVERRIDE_NUM_LANES :
       SC_X4_LN_NUM_OVRRr_CLR(reg_ln_num_or_val);
       SC_X4_LN_NUM_OVRRr_NUM_LANES_OVERRIDE_VALUEf_SET(reg_ln_num_or_val,or_value);
       MODIFY_SC_X4_LN_NUM_OVRRr(pc,reg_ln_num_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_NUM_LANES_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_OS_MODE:
       RX_X4_PMA_CTL0r_CLR(reg_os_mode_or_val);
       RX_X4_PMA_CTL0r_OS_MODEf_SET(reg_os_mode_or_val,or_value);
       MODIFY_RX_X4_PMA_CTL0r(pc,reg_os_mode_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_OS_MODE_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_FEC_EN :
       TX_X4_MISCr_CLR(reg_misc_or_val);
       TX_X4_MISCr_FEC_ENABLEf_SET(reg_misc_or_val,or_value);
       MODIFY_TX_X4_MISCr(pc,reg_misc_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_FEC_ENABLE_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_DESKEW_MODE:
       RX_X4_PCS_CTL0r_CLR(reg_pcs_or_val);
       RX_X4_PCS_CTL0r_DESKEWMODEf_SET(reg_pcs_or_val,or_value);
       MODIFY_RX_X4_PCS_CTL0r(pc,reg_pcs_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_DESKEWMODE_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_DESC2_MODE :
       RX_X4_PCS_CTL0r_CLR(reg_pcs_or_val);
       RX_X4_PCS_CTL0r_DESC2_MODEf_SET(reg_pcs_or_val,or_value);
       MODIFY_RX_X4_PCS_CTL0r(pc,reg_pcs_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_DESC2_MODE_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_CL36BYTEDEL_MODE:
       RX_X4_PCS_CTL0r_CLR(reg_pcs_or_val);
       RX_X4_PCS_CTL0r_CL36BYTEDELETEMODEf_SET(reg_pcs_or_val,or_value);
       MODIFY_RX_X4_PCS_CTL0r(pc,reg_pcs_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_CL36BYTEDELETEMODE_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_BRCM64B66_DESCR_MODE:
       RX_X4_PCS_CTL0r_CLR(reg_pcs_or_val);
       RX_X4_PCS_CTL0r_BRCM64B66_DESCRAMBLER_ENABLEf_SET(reg_pcs_or_val,or_value);
       MODIFY_RX_X4_PCS_CTL0r(pc,reg_pcs_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_BRCM64B66_DESCRAMBLER_ENABLE_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_CHKEND_EN:
       RX_X2_MISC0r_CLR(reg_misc0_or_val);
       RX_X2_MISC0r_CHK_END_ENf_SET(reg_misc0_or_val, or_value);
       MODIFY_RX_X2_MISC0r(pc,reg_misc0_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_CHK_END_EN_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_BLKSYNC_MODE:
       RX_X4_DEC_CTL0r_CLR(reg_dec_or_val);
       RX_X4_DEC_CTL0r_BLOCK_SYNC_MODEf_SET(reg_dec_or_val,or_value);
       MODIFY_RX_X4_DEC_CTL0r(pc, reg_dec_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_BLOCK_SYNC_MODE_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_REORDER_EN :
       RX_X4_CL36_RX0r_CLR(reg_cl36_or_val);
       RX_X4_CL36_RX0r_REORDER_ENf_SET(reg_cl36_or_val, or_value);
       MODIFY_RX_X4_CL36_RX0r(pc,reg_cl36_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_REORDER_EN_OENf_SET(reg_or_en1, 1);
       MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc,reg_or_en1);
  break;
  case TEMOD_OVERRIDE_CL36_EN: 
       RX_X4_CL36_RX0r_CLR(reg_cl36_or_val);
       RX_X4_CL36_RX0r_CL36_ENf_SET(reg_cl36_or_val, or_value);
       MODIFY_RX_X4_CL36_RX0r(pc,reg_cl36_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_CL36_EN_OENf_SET(reg_or_en1, 1);
       MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc,reg_or_en1);
  break;
  case TEMOD_OVERRIDE_SCR_MODE:
       TX_X4_MISCr_CLR(reg_misc_or_val);
       TX_X4_MISCr_SCR_MODEf_SET(reg_misc_or_val,or_value);
       MODIFY_TX_X4_MISCr(pc,reg_misc_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_SCR_MODE_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_DESCR_MODE:
       RX_X4_PCS_CTL0r_CLR(reg_pcs_or_val);
       RX_X4_PCS_CTL0r_DESCRAMBLERMODEf_SET(reg_pcs_or_val,or_value);
       MODIFY_RX_X4_PCS_CTL0r(pc,reg_pcs_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_DESCRAMBLERMODE_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_ENCODE_MODE:
       TX_X4_ENC0r_CLR(reg_encode_or_val);
       TX_X4_ENC0r_ENCODEMODEf_SET(reg_encode_or_val, or_value);
       MODIFY_TX_X4_ENC0r(pc,reg_encode_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_ENCODEMODE_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  case TEMOD_OVERRIDE_DECODE_MODE: 
       RX_X4_PCS_CTL0r_CLR(reg_pcs_or_val);
       RX_X4_PCS_CTL0r_DECODERMODEf_SET(reg_pcs_or_val,or_value);
       MODIFY_RX_X4_PCS_CTL0r(pc,reg_pcs_or_val);
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN0_TYPEr_DECODERMODE_OENf_SET(reg_or_en0,1);
       MODIFY_SC_X4_FLD_OVRR_EN0_TYPEr(pc,reg_or_en0);
  break;
  }
  return PHYMOD_E_NONE;
}

/**
@brief   Override the credits.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   credit_type type of credit to override #credit_type_t
@param   userCredit User override value
@returns The value PHYMOD_E_NONE upon successful completion
@details Credit override mechanism is a seperate function. However it is similar
to other override mechanisms.
*/
int temod_credit_override_set(PHYMOD_ST* pc, credit_type_t credit_type, int userCredit)   /* CREDIT_OVERRIDE_SET */
{
  SC_X4_FLD_OVRR_EN1_TYPEr_t    reg_enable_type;
  TX_X4_CRED0r_t          reg_credit0;
  TX_X4_CRED1r_t          reg_credit1;
  TX_X4_LOOPCNTr_t                        reg_loopcnt;
  TX_X4_MAC_CREDGENCNTr_t               reg_maccreditgen;
  TX_X4_PCS_CLKCNT0r_t                  reg_pcs_clk_cnt;
  TX_X4_PCS_CREDGENCNTr_t               reg_pcs_creditgen;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  SC_X4_FLD_OVRR_EN1_TYPEr_CLR(reg_enable_type);
  TX_X4_CRED0r_CLR(reg_credit0);
  TX_X4_CRED1r_CLR(reg_credit1);
  TX_X4_LOOPCNTr_CLR(reg_loopcnt);
  TX_X4_MAC_CREDGENCNTr_CLR(reg_maccreditgen);
  TX_X4_PCS_CLKCNT0r_CLR(reg_pcs_clk_cnt);
  TX_X4_PCS_CREDGENCNTr_CLR(reg_pcs_creditgen);

  switch ( credit_type ) {
    case TEMOD_CREDIT_RESET: 
    /* reset credits */
       SC_X4_FLD_OVRR_EN1_TYPEr_CLOCKCNT0_OENf_SET(reg_enable_type, 0);
       SC_X4_FLD_OVRR_EN1_TYPEr_CLOCKCNT1_OENf_SET(reg_enable_type, 0);
       SC_X4_FLD_OVRR_EN1_TYPEr_LOOPCNT0_OENf_SET(reg_enable_type, 0);
       SC_X4_FLD_OVRR_EN1_TYPEr_LOOPCNT1_OENf_SET(reg_enable_type, 0);
       SC_X4_FLD_OVRR_EN1_TYPEr_REPLICATION_CNT_OENf_SET(reg_enable_type, 0);
       SC_X4_FLD_OVRR_EN1_TYPEr_MAC_CREDITGENCNT_OENf_SET(reg_enable_type, 0);
       SC_X4_FLD_OVRR_EN1_TYPEr_PCS_CREDITENABLE_OENf_SET(reg_enable_type, 0);
       SC_X4_FLD_OVRR_EN1_TYPEr_PCS_CLOCKCNT0_OENf_SET(reg_enable_type, 0);
       SC_X4_FLD_OVRR_EN1_TYPEr_PCS_CREDITGENCNT_OENf_SET(reg_enable_type, 0);
       SC_X4_FLD_OVRR_EN1_TYPEr_SGMII_SPD_SWITCH_OENf_SET(reg_enable_type, 0);

       PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_FLD_OVRR_EN1_TYPEr(pc, reg_enable_type));
      break;
    case TEMOD_CREDIT_TABLE: 
    /* get info. from table and apply it to all credit regs. */
    break;
    case  TEMOD_CREDIT_CLOCK_COUNT_0:
       /* write to clockcnt0 */
       TX_X4_CRED0r_CLOCKCNT0f_SET(reg_credit0, userCredit);
       PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_CRED0r(pc, reg_credit0));
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_CLOCKCNT0_OENf_SET(reg_enable_type, 1);
       PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, reg_enable_type));
      break;
    case  TEMOD_CREDIT_CLOCK_COUNT_1:  /* (cntl & 0x00020000) */
       /* write to clockcnt1 */
       TX_X4_CRED1r_CLOCKCNT1f_SET(reg_credit1, userCredit);
       PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_CRED1r(pc, reg_credit1));
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_CLOCKCNT1_OENf_SET(reg_enable_type, 1);
       PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, reg_enable_type));
       break;
    case  TEMOD_CREDIT_LOOP_COUNT_0: 
       /* write to loopcnt0 */
       TX_X4_LOOPCNTr_LOOPCNT0f_SET(reg_loopcnt, userCredit);
       PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_LOOPCNTr(pc, reg_loopcnt));
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_LOOPCNT0_OENf_SET(reg_enable_type, 1);
       PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, reg_enable_type));
       break;
    case TEMOD_CREDIT_LOOP_COUNT_1:   /* (cntl & 0x00080000) */
       /* write to pcs_clockcnt0 */
       TX_X4_LOOPCNTr_LOOPCNT1f_SET(reg_loopcnt, userCredit);
       PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_LOOPCNTr(pc, reg_loopcnt));
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_LOOPCNT1_OENf_SET(reg_enable_type, 1);
       PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, reg_enable_type));
       break;
     case TEMOD_CREDIT_MAC:   /* (cntl & 0x00100000) */
       /* write to mac_creditgencnt */
       TX_X4_MAC_CREDGENCNTr_MAC_CREDITGENCNTf_SET(reg_maccreditgen, userCredit);
       PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_MAC_CREDGENCNTr(pc, reg_maccreditgen));
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_MAC_CREDITGENCNT_OENf_SET(reg_enable_type, 1);
       PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, reg_enable_type));
      break;
    case  TEMOD_CREDIT_PCS_CLOCK_COUNT_0:   /* (cntl & 0x00200000) */
       /* write to pcs_clockcnt0 */
       TX_X4_PCS_CLKCNT0r_PCS_CLOCKCNT0f_SET(reg_pcs_clk_cnt, userCredit);
       PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_PCS_CLKCNT0r(pc, reg_pcs_clk_cnt));
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_PCS_CLOCKCNT0_OENf_SET(reg_enable_type, 1);
       PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, reg_enable_type));
      break;
    case TEMOD_CREDIT_PCS_GEN_COUNT:   /* (cntl & 0x00400000) */
       /* write to pcs_creditgencnt */
       TX_X4_PCS_CREDGENCNTr_PCS_CREDITGENCNTf_SET(reg_pcs_creditgen, userCredit) ;
       PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_PCS_CREDGENCNTr(pc, reg_pcs_creditgen));
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_PCS_CREDITGENCNT_OENf_SET(reg_enable_type, 1);
       PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, reg_enable_type));
      break;
    case TEMOD_CREDIT_EN: /* (cntl & 0x00800000) */
       /* write to pcs_crden */
       TX_X4_PCS_CLKCNT0r_PCS_CREDITENABLEf_SET(reg_pcs_clk_cnt, userCredit);
       PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_PCS_CLKCNT0r(pc, reg_pcs_clk_cnt));
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_PCS_CREDITENABLE_OENf_SET(reg_enable_type, 1);
       PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, reg_enable_type));
       break;
    case  TEMOD_CREDIT_PCS_REPCNT:    /* (cntl & 0x01000000) */
       /* write to pcs_repcnt */
       TX_X4_PCS_CLKCNT0r_REPLICATION_CNTf_SET(reg_pcs_clk_cnt, userCredit);
       PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_PCS_CLKCNT0r(pc, reg_pcs_clk_cnt));
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_REPLICATION_CNT_OENf_SET(reg_enable_type, 1);
       PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, reg_enable_type));
       break;
    case  TEMOD_CREDIT_SGMII_SPD: /* (cntl & 0x02000000) */
       /* write to sgmii */
       TX_X4_CRED0r_SGMII_SPD_SWITCHf_SET(reg_credit0, userCredit);
       PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_CRED0r(pc, reg_credit0));
       /*Set the override enable*/
       SC_X4_FLD_OVRR_EN1_TYPEr_SGMII_SPD_SWITCH_OENf_SET(reg_enable_type, 1);
       PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_FLD_OVRR_EN1_TYPEr(pc, reg_enable_type));
       break;
   default:
      return PHYMOD_E_FAIL;
      break;
  }
  return PHYMOD_E_NONE;
} /* CREDIT_SET */

int temod_clause72_control(PHYMOD_ST* pc, uint32_t cl_72_en)                /* CLAUSE_72_CONTROL */
{
  CL72_TXBASE_R_PMD_CTLr_t    reg_cl72_ctrl ;
  phymod_access_t pa_copy;
  int start_lane = 0, num_lane = 0;
  uint32_t enable=0;


  TEMOD_DBG_IN_FUNC_INFO(pc);
  CL72_TXBASE_R_PMD_CTLr_CLR(reg_cl72_ctrl) ;


  /* need to figure out what's the starting lane */
  PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
  PHYMOD_IF_ERR_RETURN
      (phymod_util_lane_config_get(pc, &start_lane, &num_lane));
  pa_copy.lane = 0x1 << start_lane;

  if (cl_72_en) {
       CL72_TXBASE_R_PMD_CTLr_CL72_IEEE_TRAINING_ENABLEf_SET(reg_cl72_ctrl, 1) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL72_TXBASE_R_PMD_CTLr(pc, reg_cl72_ctrl));
       CL72_TXBASE_R_PMD_CTLr_CL72_IEEE_RESTART_TRAININGf_SET(reg_cl72_ctrl, 1) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL72_TXBASE_R_PMD_CTLr(pc, reg_cl72_ctrl));
  } else {
       CL72_TXBASE_R_PMD_CTLr_CL72_IEEE_TRAINING_ENABLEf_SET(reg_cl72_ctrl, 0) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL72_TXBASE_R_PMD_CTLr(pc, reg_cl72_ctrl));
       CL72_TXBASE_R_PMD_CTLr_CL72_IEEE_RESTART_TRAININGf_SET(reg_cl72_ctrl, 0) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL72_TXBASE_R_PMD_CTLr(pc, reg_cl72_ctrl));
  }

  temod_disable_get(&pa_copy,&enable);
  if(enable == 0x1) {
     PHYMOD_IF_ERR_RETURN (temod_trigger_speed_change(&pa_copy));
  }

  return PHYMOD_E_NONE;
}






#ifdef _DV_TB_
/**
@brief   Control per lane clause 72 auto tuning function  (training patterns)
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   cl_72_type #cl72_type_t
@returns The value PHYMOD_E_NONE upon successful completion
@details
This function enables/disables clause-72 auto-tuning per lane.  It is used
in 40G_KR4, 40G_CR4, 10G_KR modes etc.
Lane is selected via #PHYMOD_ST::lane_select.
This Function is controlled by cl_72_type as follows.

\li 0Xxxxxxxx3      (b11): Enable  CL72 hardware block
\li 0Xxxxxxxx2      (b01): Disable CL72 hardware block
\li 0Xxxxxxxx0      (b00): no changed.
\li 0Xxxxxxx3- (b011----): Forced  enable CL72 under AN.
\li 0Xxxxxxx7- (b111----): Forced  disable CL72 under AN.
\li 0Xxxxxxx1- (b001----): Default CL72 under AN.
\li 0Xxxxxxx0- (b000----): no changed
\li 0Xxxxxx100           : read back CL72 HW en bit and put in accData. 

*/
int temod_clause72_control(PHYMOD_ST* pc, cl72_type_t cl_72_type)                /* CLAUSE_72_CONTROL */
{
  CL72_TXBASE_R_PMD_CTLr_t    reg_cl72_ctrl ;
  CL72_RXMISC1_CTLr_t         reg_misc1_ctrl ;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  CL72_TXBASE_R_PMD_CTLr_CLR(reg_cl72_ctrl) ;

  CL72_RXMISC1_CTLr_CLR(reg_misc1_ctrl) ;

  switch (cl_72_type) {
    case TSCMOD_CL72_HW_ENABLE:
       CL72_TXBASE_R_PMD_CTLr_CL72_IEEE_TRAINING_ENABLEf_SET(reg_cl72_ctrl, 1) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL72_TXBASE_R_PMD_CTLr(pc, reg_cl72_ctrl));
       break;
    case TSCMOD_CL72_HW_DISABLE:
       CL72_TXBASE_R_PMD_CTLr_CL72_IEEE_TRAINING_ENABLEf_SET(reg_cl72_ctrl, 0) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL72_TXBASE_R_PMD_CTLr(pc, reg_cl72_ctrl));
       break;
    case TSCMOD_CL72_HW_RESTART:
       CL72_TXBASE_R_PMD_CTLr_CL72_IEEE_RESTART_TRAININGf_SET(reg_cl72_ctrl, 1) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL72_TXBASE_R_PMD_CTLr(pc, reg_cl72_ctrl));
       break;
    case TSCMOD_CL72_HW_RESTART_STOP:
       CL72_TXBASE_R_PMD_CTLr_CL72_IEEE_RESTART_TRAININGf_SET(reg_cl72_ctrl, 0) ;
       PHYMOD_IF_ERR_RETURN (MODIFY_CL72_TXBASE_R_PMD_CTLr(pc, reg_cl72_ctrl));
       break;
    default :
       break;
  }
#ifdef _DV_TB_
  /* this should be done by uC */
  if (cl_72_type == TSCMOD_CL72_HW_RESTART) {
    CL72_TXMISC2_CTLr_t         reg_misc2_ctrl ;
    CL72_TXMISC2_CTLr_CLR(reg_misc2_ctrl) ;

    /* set coarse lock */
    PHYMOD_IF_ERR_RETURN (READ_CL72_RXMISC1_CTLr(pc, &reg_misc1_ctrl));
    CL72_RXMISC1_CTLr_CL72_TR_COARSE_LOCKf_SET(reg_misc1_ctrl, 1);
    PHYMOD_IF_ERR_RETURN (WRITE_CL72_RXMISC1_CTLr(pc, reg_misc1_ctrl));
    /* set rx_trained */
    PHYMOD_IF_ERR_RETURN (READ_CL72_TXMISC2_CTLr(pc, &reg_misc2_ctrl));
    CL72_TXMISC2_CTLr_CL72_RX_TRAINEDf_SET(reg_misc2_ctrl, 1);
    PHYMOD_IF_ERR_RETURN (WRITE_CL72_TXMISC2_CTLr(pc, reg_misc2_ctrl));
    } else {
      /* unset coarse lock */
      /* unset rx_trained  */
    }
#endif /* _DV_TB_ */
  return PHYMOD_E_NONE;
} /* clause72_control */
#endif /* _DV_TB_ */

#ifdef _DV_TB_

int temod_prbs_check(PHYMOD_ST* pc, int real_check, int *prbs_status)
{
   TLB_RX_PRBS_CHK_LOCK_STSr_t      reg_prbs_chk_lock;
   TLB_RX_PRBS_CHK_ERR_CNT_MSB_STSr_t    reg_prbs_err_cnt_msb;
   TLB_RX_PRBS_CHK_ERR_CNT_LSB_STSr_t     reg_prbs_err_cnt_lsb;
   int lock, lock_lost, err ;

   TEMOD_DBG_IN_FUNC_INFO(pc);
   pc->accData   = 0 ;

   TLB_RX_PRBS_CHK_LOCK_STSr_CLR(reg_prbs_chk_lock);
   TLB_RX_PRBS_CHK_ERR_CNT_MSB_STSr_CLR(reg_prbs_err_cnt_msb);
   TLB_RX_PRBS_CHK_ERR_CNT_LSB_STSr_CLR(reg_prbs_err_cnt_lsb);

   PHYMOD_IF_ERR_RETURN
      ( READ_TLB_RX_PRBS_CHK_LOCK_STSr(pc, &reg_prbs_chk_lock));
   PHYMOD_IF_ERR_RETURN
      (READ_TLB_RX_PRBS_CHK_ERR_CNT_MSB_STSr(pc, &reg_prbs_err_cnt_msb));
   PHYMOD_IF_ERR_RETURN
      (READ_TLB_RX_PRBS_CHK_ERR_CNT_LSB_STSr(pc, &reg_prbs_err_cnt_lsb));

   lock      = TLB_RX_PRBS_CHK_LOCK_STSr_PRBS_CHK_LOCKf_GET(reg_prbs_chk_lock) ? 1 : 0 ;
   lock_lost = TLB_RX_PRBS_CHK_ERR_CNT_MSB_STSr_PRBS_CHK_LOCK_LOST_LHf_GET(reg_prbs_err_cnt_msb) ? 1 : 0 ;
   err       = TLB_RX_PRBS_CHK_ERR_CNT_MSB_STSr_PRBS_CHK_ERR_CNT_MSBf_GET(reg_prbs_err_cnt_msb) ;
   err       = (err << 16) | TLB_RX_PRBS_CHK_ERR_CNT_LSB_STSr_PRBS_CHK_ERR_CNT_LSBf_GET(reg_prbs_err_cnt_lsb) ; 
   
#ifdef _DV_TB_
   printf("prbs_check u=%0d p=%0d ln=%0d lck=%0d lost=%0d err=%0d(H%x L%x)\n",
             pc->unit, pc->port, pc->this_lane, lock, lock_lost, err, reg_prbs_err_cnt_msb, reg_prbs_err_cnt_lsb);
#endif

   if (TLB_RX_PRBS_CHK_LOCK_STSr_PRBS_CHK_LOCKf_GET(reg_prbs_chk_lock)) {
      if (TLB_RX_PRBS_CHK_ERR_CNT_MSB_STSr_PRBS_CHK_LOCK_LOST_LHf_GET(reg_prbs_err_cnt_msb)) {
         /* locked now but lost before */
         *prbs_status = TLB_RX_PRBS_CHK_ERR_CNT_MSB_STSr_PRBS_CHK_ERR_CNT_MSBf_GET(reg_prbs_err_cnt_msb);
         *prbs_status = (*prbs_status << 16) | TLB_RX_PRBS_CHK_ERR_CNT_LSB_STSr_PRBS_CHK_ERR_CNT_LSBf_GET(reg_prbs_err_cnt_lsb) | 1  ;  /* locked once, so one error at least */
          pc->accData   = real_check ? 1 : 0 ;
          return PHYMOD_E_FAIL;
      } else {
         *prbs_status = TLB_RX_PRBS_CHK_ERR_CNT_MSB_STSr_PRBS_CHK_ERR_CNT_MSBf_GET(reg_prbs_err_cnt_msb) ; 
         *prbs_status = (*prbs_status << 16)| TLB_RX_PRBS_CHK_ERR_CNT_LSB_STSr_PRBS_CHK_ERR_CNT_LSBf_GET(reg_prbs_err_cnt_lsb) ;  
          if (err) {
            pc->accData   = real_check ? 1 : 0 ;
         return PHYMOD_E_FAIL;
          }
      }
   } else {
      if(TLB_RX_PRBS_CHK_ERR_CNT_MSB_STSr_PRBS_CHK_LOCK_LOST_LHf_GET(reg_prbs_err_cnt_msb)) {
         *prbs_status = -2 ;  /* locked before but lost now */
          pc->accData   = real_check ? 1 : 0 ;
          return PHYMOD_E_FAIL;
      } else {
         *prbs_status = -1 ;  /* never locked */
          pc->accData   = real_check ? 1 : 0 ;
         return PHYMOD_E_FAIL;
      }
   }
  return PHYMOD_E_NONE ;
}
#endif /* _DV_TB_ */

#ifdef _DV_TB_

int temod_prbs_mode(PHYMOD_ST* pc, int port, int prbs_inv, int pattern_type, int prbs_check_mode)    /* PRBS_MODE */
{
  TLB_TX_PRBS_GEN_CFGr_t    reg_prbs_gen_cfg;
  TLB_RX_PRBS_CHK_CFGr_t    reg_prbs_chk_cfg;
  /* SC_X4_BYPASSr_t      reg_bypass; */

  /*int prbs_check_mode ; */
  int prbs_sel_tx, prbs_inv_tx ;
  int prbs_sel_rx, prbs_inv_rx ;


  TEMOD_DBG_IN_FUNC_INFO(pc);
  TLB_TX_PRBS_GEN_CFGr_CLR(reg_prbs_gen_cfg);
  TLB_RX_PRBS_CHK_CFGr_CLR(reg_prbs_chk_cfg);

  prbs_sel_tx =  pattern_type;
  prbs_sel_rx =  pattern_type;
  prbs_inv_tx =  prbs_inv;
  prbs_inv_rx =  prbs_inv;

  TLB_TX_PRBS_GEN_CFGr_PRBS_GEN_MODE_SELf_SET(reg_prbs_gen_cfg, prbs_sel_tx);
  TLB_TX_PRBS_GEN_CFGr_PRBS_GEN_INVf_SET(reg_prbs_gen_cfg, prbs_inv_tx);

  PHYMOD_IF_ERR_RETURN (MODIFY_TLB_TX_PRBS_GEN_CFGr(pc, reg_prbs_gen_cfg));

  TLB_RX_PRBS_CHK_CFGr_PRBS_CHK_MODEf_SET(reg_prbs_chk_cfg, prbs_check_mode);
  TLB_RX_PRBS_CHK_CFGr_PRBS_CHK_MODE_SELf_SET(reg_prbs_chk_cfg, prbs_sel_rx);
  TLB_RX_PRBS_CHK_CFGr_PRBS_CHK_INVf_SET(reg_prbs_chk_cfg, prbs_inv_rx);

  PHYMOD_IF_ERR_RETURN (MODIFY_TLB_RX_PRBS_CHK_CFGr(pc, reg_prbs_chk_cfg));

  return PHYMOD_E_NONE;

} /* PRBS_MODE */
#endif /* _DV_TB_ */

#ifdef _DV_TB_

int temod_prbs_control(PHYMOD_ST* pc, int prbs_enable)                /* PRBS_CONTROL */
{
  TLB_TX_PRBS_GEN_CFGr_t    reg_prbs_gen_cfg;
  TLB_RX_PRBS_CHK_CFGr_t    reg_prbs_chk_cfg;
  int prbs_rx_en, prbs_tx_en ;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TLB_TX_PRBS_GEN_CFGr_CLR(reg_prbs_gen_cfg);
  TLB_RX_PRBS_CHK_CFGr_CLR(reg_prbs_chk_cfg);
  
  prbs_tx_en = prbs_enable;
  prbs_rx_en = prbs_enable;

  TLB_TX_PRBS_GEN_CFGr_PRBS_GEN_ENf_SET(reg_prbs_gen_cfg, prbs_tx_en);
  PHYMOD_IF_ERR_RETURN (MODIFY_TLB_TX_PRBS_GEN_CFGr(pc, reg_prbs_gen_cfg));

  TLB_RX_PRBS_CHK_CFGr_PRBS_CHK_ENf_SET(reg_prbs_chk_cfg, prbs_rx_en);
  PHYMOD_IF_ERR_RETURN (MODIFY_TLB_RX_PRBS_CHK_CFGr(pc, reg_prbs_chk_cfg));

  return PHYMOD_E_NONE;

}  /* PRBS_CONTROL */
#endif /* _DV_TB_ */

#ifdef _DV_TB_
/**
@brief   Sets CJPAT/CRPAT parameters for a particular port
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   spd_intf speed type #temod_spd_intfc_type
@param   port port num
@param   pattern_type type of CJPat
@param   pkt_number number of packets
@param   pkt_size size of packets
@param   ipg_size IPG between packets
@returns The value PHYMOD_E_NONE upon successful completion

This function enables either a CJPAT or CRPAT for a particular port. 
The speed mode of the lane must be chosen from the enumerated type
#temod_spd_intfc_type and set in the spd_intf field.
To enable CJPAT, TBD
To enable CRPAT, TBD
This function is used along with temod_cjpat_crpat_check().
*/

int temod_cjpat_crpat_mode_set(PHYMOD_ST* pc,
                                temod_spd_intfc_type spd_intf,
                                int port,
                                int pattern_type,
                                int pkt_number,
                                int pkt_size,
                                int ipg_size)   /* CJPAT_CRPAT_MODE_SET  */
{
   int number_pkt, crc_check, intf_type, rx_port_sel, tx_port_sel, pkt_or_prtp ;
   int pkt_size_int, ipg_size_int, payload_type, prtp_data_pattern_sel, rx_prtp_en ;
   int tx_prtp_en;
   uint16_t data16 ;

   int rand_num;

   PKTGEN_CTL1r_t           reg_pktctrl1;
   PKTGEN_CTL2r_t           reg_pktctrl2;
   PKTGEN_PRTPCTLr_t        reg_prtpctrl;
   PKTGEN_PCS_SEEDA0r_t     reg_seeda0;
   PKTGEN_PCS_SEEDA1r_t     reg_seeda1;
   PKTGEN_PCS_SEEDA2r_t     reg_seeda2;
   PKTGEN_PCS_SEEDA3r_t     reg_seeda3;
   PKTGEN_PCS_SEEDB0r_t     reg_seedb0;
   PKTGEN_PCS_SEEDB1r_t     reg_seedb1;
   PKTGEN_PCS_SEEDB2r_t     reg_seedb2;
   PKTGEN_PCS_SEEDB3r_t     reg_seedb3;
   PKTGEN_PAYLOADBYTESr_t   reg_payload_b;

   TEMOD_DBG_IN_FUNC_INFO(pc);
   PKTGEN_CTL1r_CLR(reg_pktctrl1);
   PKTGEN_CTL2r_CLR(reg_pktctrl2);
   PKTGEN_PRTPCTLr_CLR(reg_prtpctrl);
   PKTGEN_PCS_SEEDA0r_CLR(reg_seeda0);
   PKTGEN_PCS_SEEDA1r_CLR(reg_seeda1);
   PKTGEN_PCS_SEEDA2r_CLR(reg_seeda2);
   PKTGEN_PCS_SEEDA3r_CLR(reg_seeda3);
   PKTGEN_PCS_SEEDB0r_CLR(reg_seedb0);
   PKTGEN_PCS_SEEDB1r_CLR(reg_seedb1);
   PKTGEN_PCS_SEEDB2r_CLR(reg_seedb2);
   PKTGEN_PCS_SEEDB3r_CLR(reg_seedb3);
   PKTGEN_PAYLOADBYTESr_CLR(reg_payload_b);

   number_pkt = pkt_number ; /* 2: unlimited, 0: idles, 1: single pkt */
   crc_check  = 1 ;
   rx_port_sel= port ;
   tx_port_sel= port ;
   pkt_or_prtp = 0 ; /* 0: pakcet generator;  1: PRTP(pseudo random test pattern) */
   rx_prtp_en = 0;
   tx_prtp_en = 0;
   if ((pattern_type == 0x9) | (pattern_type == 0xA)) {
     pkt_or_prtp = 1;
     rx_prtp_en = 1;
     tx_prtp_en = 1;
   }

   if (pattern_type == 0xA) {
     prtp_data_pattern_sel = 1;
   } else {
     prtp_data_pattern_sel = 0;
   }

/*   pkt_size_int   = pkt_size ; */
   pkt_size_int   = 1;
   ipg_size_int   = 8 ;
 /*  ipg_size_int   = ipg_size ; */

   /* need to check the interface type */
   if ((pattern_type == 0x9) | (pattern_type == 0xA)) {
     payload_type = 0;
   } else {
     payload_type = pattern_type;
   }


   /* intf_type  0: XLGMII/XGMII;  1: MIII/GMII */

   intf_type = ((spd_intf == TEMOD_SPD_10_X1_SGMII) |
     (spd_intf == TEMOD_SPD_100_X1_SGMII) |
     (spd_intf == TEMOD_SPD_1000_X1_SGMII) |
     (spd_intf == TEMOD_SPD_10_SGMII) |
     (spd_intf == TEMOD_SPD_100_SGMII) |
     (spd_intf == TEMOD_SPD_1000_SGMII) |
     (spd_intf == TEMOD_SPD_2500) |
     (spd_intf == TEMOD_SPD_2500_X1)) ? 1 : 0;

 /* if payload type is Repeat 2 Bytes in 0x9040*/

  if (payload_type == 0) {
    data16 = 0x2323;
    PKTGEN_PAYLOADBYTESr_BYTE1f_SET(reg_payload_b, ((data16 >> 8)  & 0xff));
    PKTGEN_PAYLOADBYTESr_BYTE0f_SET(reg_payload_b, (data16 & 0xff));
    WRITE_PKTGEN_PAYLOADBYTESr(pc, reg_payload_b);
  }

   /* 0x9030 */
   PKTGEN_CTL1r_PKT_OR_PRTPf_SET(reg_pktctrl1, pkt_or_prtp);
   PKTGEN_CTL1r_TX_TEST_PORT_SELf_SET(reg_pktctrl1, tx_port_sel);
   PKTGEN_CTL1r_RX_PORT_SELf_SET(reg_pktctrl1, rx_port_sel);
   PKTGEN_CTL1r_RX_PKT_CHECK_ENf_SET(reg_pktctrl1, crc_check);
   PKTGEN_CTL1r_RX_MSBUS_TYPEf_SET(reg_pktctrl1, intf_type);
   PKTGEN_CTL1r_NUMBER_PKTf_SET(reg_pktctrl1, number_pkt);
   PKTGEN_CTL1r_TX_PRTP_ENf_SET(reg_pktctrl1, tx_prtp_en);
   PKTGEN_CTL1r_PRTP_DATA_PATTERN_SELf_SET(reg_pktctrl1, prtp_data_pattern_sel);

   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_CTL1r(pc, reg_pktctrl1));

   /* 0x9031 */
   PKTGEN_CTL2r_IPG_SIZEf_SET(reg_pktctrl2, ipg_size_int);
   PKTGEN_CTL2r_PKT_SIZEf_SET(reg_pktctrl2, pkt_size_int);
   PKTGEN_CTL2r_PAYLOAD_TYPEf_SET(reg_pktctrl2, payload_type);
   PKTGEN_CTL2r_TX_MSBUS_TYPEf_SET(reg_pktctrl2, intf_type);

   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_CTL2r(pc, reg_pktctrl2));

   /* 0x9032 */
   PKTGEN_PRTPCTLr_RX_PRTP_ENf_SET(reg_prtpctrl,rx_prtp_en);
   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_PRTPCTLr(pc, reg_prtpctrl));

/* program seeds if necessary */
   rand_num = 1234;  /* assign random number */
#ifdef _DV_TB_
   rand_num = rand ();
#endif /* _DV_TB_ */
   
   data16 = rand_num & 0xFFFF;
   if ((pattern_type == 0x9) | (pattern_type == 0xA)) {
   PKTGEN_PCS_SEEDA0r_SEEDA0f_SET(reg_seeda0, data16);
   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_PCS_SEEDA0r(pc, reg_seeda0));
   data16 = data16 + 1;
   PKTGEN_PCS_SEEDA1r_SEEDA1f_SET(reg_seeda1, data16);
   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_PCS_SEEDA1r(pc, reg_seeda1));
   data16 = data16 + 1;
   PKTGEN_PCS_SEEDA2r_SEEDA2f_SET(reg_seeda2, data16);
   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_PCS_SEEDA2r( pc, reg_seeda2));
   data16 = data16 + 1;
   PKTGEN_PCS_SEEDA3r_SEEDA3f_SET(reg_seeda3, data16);
   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_PCS_SEEDA3r(pc, reg_seeda3));
   data16 = data16 + 1;
   PKTGEN_PCS_SEEDB0r_SEEDB0f_SET(reg_seedb0, data16);
   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_PCS_SEEDB0r( pc, reg_seedb0));
   data16 = data16 + 1;
   PKTGEN_PCS_SEEDB1r_SEEDB1f_SET(reg_seedb1, data16);
   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_PCS_SEEDB1r( pc, reg_seedb1));
   data16 = data16 + 1;
   PKTGEN_PCS_SEEDB2r_SEEDB2f_SET(reg_seedb2, data16);
   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_PCS_SEEDB2r(pc, reg_seedb2));
   data16 = data16 + 1;
   PKTGEN_PCS_SEEDB3r_SEEDB3f_SET(reg_seedb3, data16);
   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_PCS_SEEDB3r( pc, reg_seedb3));
   }
   return PHYMOD_E_NONE;
} /* int temod_cjpat_crpat_mode_set */

/**
@brief   Checks CJPAT/CRPAT parameters for a particular port
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion

This function checks for CJPAT or CRPAT for a particular port.  The speed mode
of the lane is provided as enumerated type #temod_spd_intfc_type.  CJPAT/CRPAT
must first enabled using #temod_cjpat_crpat_control().  The duration of packet
transmission is controlled externally and is the time duration between the two
function calls above.

This function compares the CJPAT/CRPAT TX and RX counters and prints the
results.  Currently only packet count is supported.

This function reads the following registers:

\li PATGEN1_TXPKTCNT_U (0xC040) Upper 16b of count of Transmitted packets
\li PATGEN1_TXPKTCNT_L (0xC041) Lower 16b of count of Transmitted packets
\li PATGEN1_RXPKTCNT_U (0xC042) Upper 16b of count of Recieved    packets
\li PATGEN1_RXPKTCNT_L (0xC043) Lower 16b of count of Recieved    packets
\li PKTGEN0_CRCERRORCOUNT (0x9033) CRC error conunt 

TBD: Make it return status/tx_cnt or something?
*/

int temod_cjpat_crpat_check(PHYMOD_ST* pc)     /* CJPAT_CRPAT_CHECK */
{
   uint32_t tx_cnt, rx_cnt ;

   PATGEN_TXPKTCNT_Ur_t    reg_tx_u;
   PATGEN_TXPKTCNT_Lr_t    reg_tx_l;
   PATGEN_RXPKTCNT_Ur_t    reg_rx_u;
   PATGEN_RXPKTCNT_Lr_t    reg_rx_l;
   PKTGEN_CRCERRCNTr_t reg_crc_cnt;

   TEMOD_DBG_IN_FUNC_INFO(pc);
   PATGEN_TXPKTCNT_Ur_CLR(reg_tx_u);
   READ_PATGEN_TXPKTCNT_Ur(pc, &reg_tx_u);
   PATGEN_TXPKTCNT_Lr_CLR(reg_tx_l);
   READ_PATGEN_TXPKTCNT_Lr(pc, &reg_tx_l);
   PATGEN_RXPKTCNT_Ur_CLR(reg_rx_u);
   READ_PATGEN_RXPKTCNT_Ur(pc, &reg_rx_u);
   PATGEN_RXPKTCNT_Lr_CLR(reg_rx_l);
   READ_PATGEN_RXPKTCNT_Lr(pc, &reg_rx_l);
   PKTGEN_CRCERRCNTr_CLR(reg_crc_cnt);
   READ_PKTGEN_CRCERRCNTr(pc, &reg_crc_cnt);

   tx_cnt = PATGEN_TXPKTCNT_Ur_GET(reg_tx_u) <<16;
   tx_cnt |= PATGEN_TXPKTCNT_Lr_GET(reg_tx_l);

   rx_cnt = PATGEN_RXPKTCNT_Ur_GET(reg_rx_u) <<16;
   rx_cnt |= PATGEN_RXPKTCNT_Lr_GET(reg_rx_l);

#ifdef _DV_TB_
   pc->accData = 1 ;
#endif /* _DV_TB_ */
   if( (tx_cnt != rx_cnt) ||(tx_cnt==0) ) {
#ifdef _DV_TB_
  
      PHYMOD_DEBUG_ERROR(("%-22s ERROR: u=%0d p=%0d tx_cnt=%0d rx_cnt=%0d mismatch\n", __func__, pc->unit, pc->port, tx_cnt, rx_cnt));
      pc->accData = 0;
#endif /* _DV_TB_ */
      return PHYMOD_E_FAIL;
   }

   if(reg_crc_cnt.pktgen_crcerrcnt[0]) {
#ifdef _DV_TB_
  
      PHYMOD_DEBUG_ERROR(("%-22s ERROR: u=%0d p=%0d crc error=%0d\n", __func__, pc->unit, pc->port, reg_crc_cnt.pktgen_crcerrcnt[0]));
      pc->accData = 0;
#endif /* _DV_TB_ */

      return PHYMOD_E_FAIL;
   }
   return PHYMOD_E_NONE;
}


int temod_cjpat_crpat_control(PHYMOD_ST* pc, int enable)         /* CJPAT_CRPAT_CONTROL  */
{
  PKTGEN_CTL2r_t    reg;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  PKTGEN_CTL2r_CLR(reg);
  /* 0x9031 */
  PHYMOD_IF_ERR_RETURN
     (READ_PKTGEN_CTL2r(pc, &reg));

  if(enable) {
    PKTGEN_CTL2r_PKTGEN_ENf_SET(reg,  1);
  } else {
    PKTGEN_CTL2r_PKTGEN_ENf_SET(reg, 0);
  }
   PHYMOD_IF_ERR_RETURN
      (WRITE_PKTGEN_CTL2r(pc, reg));

  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */


int temod_pcs_lane_swap_get ( PHYMOD_ST *pc,  uint32_t *tx_rx_swap) 
{
  unsigned int pcs_map;
  MAIN0_LN_SWPr_t reg;
  TEMOD_DBG_IN_FUNC_INFO(pc);

  MAIN0_LN_SWPr_CLR(reg);

  PHYMOD_IF_ERR_RETURN
      (READ_MAIN0_LN_SWPr(pc, &reg)) ;

  pcs_map = MAIN0_LN_SWPr_GET(reg);

  *tx_rx_swap = (((pcs_map >> 0)  & 0x3) << 0) |
                (((pcs_map >> 2)  & 0x3) << 4) |
                (((pcs_map >> 4)  & 0x3) << 8) |
                (((pcs_map >> 6)  & 0x3) << 12) ;

  return PHYMOD_E_NONE ;
}



int temod_pcs_lane_swap(PHYMOD_ST *pc, int lane_map)
{
  unsigned int pcs_map;
  MAIN0_LN_SWPr_t reg;
  TEMOD_DBG_IN_FUNC_INFO(pc);

  MAIN0_LN_SWPr_CLR(reg);

  pcs_map = (((lane_map >> 0)  & 0x3) << 0) |
            (((lane_map >> 4)  & 0x3) << 2) |
            (((lane_map >> 8)  & 0x3) << 4) |
            (((lane_map >> 12) & 0x3) << 6) ;

  MAIN0_LN_SWPr_SET(reg, pcs_map);

  PHYMOD_IF_ERR_RETURN
      (WRITE_MAIN0_LN_SWPr(pc, reg)) ;

  return PHYMOD_E_NONE ;
}

/**
@brief   sets PMD TX lane swap values for all lanes simultaneously.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   addr_lane_index lane swap info as described in description
@returns The value PHYMOD_E_NONE upon successful completion

This function sets the TX lane swap values for all lanes simultaneously.

The TSCE has several two sets of lane swap registers per lane.  This function
uses the lane swap registers closest to the pads.  For TX, the lane swap occurs
after all other analog/digital functionality.  For RX, the lane swap occurs
prior to all other analog/digital functionality.

It is not advisable to swap lanes without going through reset.

How the swap is done is indicated in the lane control field;
bits 0 through 15 represent the swap settings for RX while bits 16
through 31 represent the swap settings for TX.

Each 4 bit chunk within both the RX and TX sections represent the swap settings
for a particular lane as follows:

Bits lane assignment

\li 15:12  TX lane 3
\li 11:8   TX lane 2
\li 7:4    TX lane 1
\li 3:0    TX lane 0

Each 4-bit value may be either 0, 1, 2 or 3, with the value indicating the lane
mapping.  By default, the equivalent addr_lane_index settings are 0x3210_3210,
indicating that there is no re-routing of traffic.  As an example, to swap lanes
0 and 1 for both RX and TX, the addr_lane_index should be set to 0x3201_3201.
To swap lanes 0 and 1, and also lanes 2 and 3 for both RX and TX, the
addr_lane_index should be set to 0x2301_2301.

*/
int temod_pmd_addr_lane_swap(PHYMOD_ST *pc, uint32_t addr_lane_index)
{
  DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r_t reg_adr_01;
  DIG_LN_ADDR_2_3r_t reg_adr_23;
  unsigned int map;
  unsigned int lane_addr_0, lane_addr_1, lane_addr_2, lane_addr_3;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r_CLR(reg_adr_01);
  DIG_LN_ADDR_2_3r_CLR(reg_adr_23);

#ifdef _DV_TB_
  map = (unsigned int)pc->per_lane_control;

  lane_addr_0 = (map >> 16) & 0xf;
  lane_addr_1 = (map >> 20) & 0xf;
  lane_addr_2 = (map >> 24) & 0xf;
  lane_addr_3 = (map >> 28) & 0xf;

#else
  map = addr_lane_index ;

  lane_addr_0 = (map >> 0) & 0xf;
  lane_addr_1 = (map >> 4) & 0xf;
  lane_addr_2 = (map >> 8) & 0xf;
  lane_addr_3 = (map >> 12) & 0xf;
#endif

#ifdef _DV_TB_
  pc->adjust_port_mode = 1;
#endif

#ifdef _SDK_TEMOD_

#endif

  DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r_LANE_ADDR_0f_SET(reg_adr_01, lane_addr_0);
  DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r_LANE_ADDR_1f_SET(reg_adr_01, lane_addr_1);
  PHYMOD_IF_ERR_RETURN
     (MODIFY_DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r(pc, reg_adr_01));

  DIG_LN_ADDR_2_3r_LANE_ADDR_2f_SET(reg_adr_23, lane_addr_2);
  DIG_LN_ADDR_2_3r_LANE_ADDR_3f_SET(reg_adr_23, lane_addr_3);

  PHYMOD_IF_ERR_RETURN
     (WRITE_DIG_LN_ADDR_2_3r(pc, reg_adr_23));

#ifdef _DV_TB_
  pc->adjust_port_mode = 0;
#endif
#ifdef _SDK_TEMOD_

#endif

  return PHYMOD_E_NONE ;
}


/**
@brief   Gets PMD TX lane swap values for all lanes simultaneously.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   tx_lane_map receives  the pmd tx lane map
@returns The value PHYMOD_E_NONE upon successful completion

This function gets the TX lane swap values for all lanes simultaneously.

The TSCE has several two sets of lane swap registers per lane.  This function
uses the lane swap registers closest to the pads.  For TX, the lane swap occurs
after all other analog/digital functionality.  For RX, the lane swap occurs
prior to all other analog/digital functionality.

It is not allowed to swap lanes without going through reset.

How the swap is done is indicated in the tx_lane_map arg.
bits 0 through 15 represent the swap settings for RX while bits 16
through 31 represent the swap settings for TX.

Each 4 bit chunk within both the RX and TX sections represent the swap settings
for a particular lane as follows:

Bits lane assignment

\li 15:12  TX lane 3
\li 11:8   TX lane 2
\li 7:4    TX lane 1
\li 3:0    TX lane 0

Each 4-bit value may be either 0, 1, 2 or 3, with the value indicating the lane
mapping.  By default, the equivalent per_lane_control settings are 0x3210_3210,
indicating that there is no re-routing of traffic.  As an example, to swap lanes
0 and 1 for both RX and TX, the per_lane_control should be set to 0x3201_3201.
To swap lanes 0 and 1, and also lanes 2 and 3 for both RX and TX, the
per_lane_control should be set to 0x2301_2301.
*/
int temod_pmd_lane_swap_tx_get ( PHYMOD_ST *pc, uint32_t *tx_lane_map)  
{
  uint16_t tx_lane_map_0, tx_lane_map_1, tx_lane_map_2, tx_lane_map_3;
  DIG_TX_LN_MAP_0_1_2r_t              reg;
  DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r_t    reg1;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  DIG_TX_LN_MAP_0_1_2r_CLR(reg);
  DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r_CLR(reg1);

  PHYMOD_IF_ERR_RETURN
     (READ_DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r(pc, &reg1));
  PHYMOD_IF_ERR_RETURN
     (READ_DIG_TX_LN_MAP_0_1_2r(pc, &reg));


  tx_lane_map_0 = DIG_TX_LN_MAP_0_1_2r_TX_LANE_MAP_0f_GET(reg);
  tx_lane_map_1 = DIG_TX_LN_MAP_0_1_2r_TX_LANE_MAP_1f_GET(reg);
  tx_lane_map_2 = DIG_TX_LN_MAP_0_1_2r_TX_LANE_MAP_2f_GET(reg);
  tx_lane_map_3 = DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r_TX_LANE_MAP_3f_GET(reg1);

  *tx_lane_map = ((tx_lane_map_0 & 0xf) << 0) 
              | ((tx_lane_map_1 & 0xf) << 4) 
              | ((tx_lane_map_2 & 0xf) << 8)
              | ((tx_lane_map_3 & 0xf) << 12);

  return PHYMOD_E_NONE ;

}

/**
@brief   sets PMD TX lane swap values for all lanes simultaneously.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   tx_lane_map
@returns The value PHYMOD_E_NONE upon successful completion

This function sets the TX lane swap values for all lanes simultaneously.

The TSCE has several two sets of lane swap registers per lane.  This function
uses the lane swap registers closest to the pads.  For TX, the lane swap occurs
after all other analog/digital functionality.  For RX, the lane swap occurs
prior to all other analog/digital functionality.

It is not allowed to swap lanes without going through reset.
*/
int temod_pmd_lane_swap_tx(PHYMOD_ST *pc, uint32_t tx_lane_map)
{
  uint16_t tx_lane_map_0, tx_lane_map_1, tx_lane_map_2, tx_lane_map_3;
  DIG_TX_LN_MAP_0_1_2r_t                reg;
  DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r_t    reg1;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  DIG_TX_LN_MAP_0_1_2r_CLR(reg);
  DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r_CLR(reg1);

  tx_lane_map_0 = (tx_lane_map >> 0) & 0xf;
  tx_lane_map_1 = (tx_lane_map >> 4) & 0xf;
  tx_lane_map_2 = (tx_lane_map >> 8) & 0xf;
  tx_lane_map_3 = (tx_lane_map >> 12) & 0xf;

#ifdef _DV_TB_
  pc->adjust_port_mode = 1;
#endif
#ifdef _SDK_TEMOD_

#endif

  DIG_TX_LN_MAP_0_1_2r_TX_LANE_MAP_0f_SET(reg, tx_lane_map_0);
  DIG_TX_LN_MAP_0_1_2r_TX_LANE_MAP_1f_SET(reg, tx_lane_map_1);
  DIG_TX_LN_MAP_0_1_2r_TX_LANE_MAP_2f_SET(reg, tx_lane_map_2);

  PHYMOD_IF_ERR_RETURN
     (WRITE_DIG_TX_LN_MAP_0_1_2r(pc, reg));

  DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r_TX_LANE_MAP_3f_SET(reg1, tx_lane_map_3);
  PHYMOD_IF_ERR_RETURN
     (MODIFY_DIG_TX_LN_MAP_3_N_LN_ADDR_0_1r(pc, reg1));

#ifdef _DV_TB_
  pc->adjust_port_mode = 0;
#endif
#ifdef _SDK_TEMOD_

#endif
  return PHYMOD_E_NONE ;
}

#ifdef _DV_TB_
/**
@brief   Read the 16 bit rev. id value etc.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful
@details PCS Init calls this Per Port PCS Init routine
*/
int temod_set_port_mode_select(PHYMOD_ST* pc)
{

 int         port_mode_sel ;
 uint16_t      single_port_mode;
 MAIN0_SETUPr_t    reg     ;

 MAIN0_SETUPr_CLR(reg);
 port_mode_sel = 0 ;
  if(pc->tsc_touched == 1) {
     single_port_mode = 0 ;
     MAIN0_SETUPr_CLR(reg);
  } else {
     MAIN0_SETUPr_MASTER_PORT_NUMf_SET(reg, pc->master_portnum);
     MAIN0_SETUPr_PLL_RESET_ENf_SET(reg, 1);
  }

 switch(pc->port_type) {
   case TEMOD_MULTI_PORT:   port_mode_sel = 0;  break ;
   case TEMOD_TRI1_PORT:    port_mode_sel = 1;  break ;
   case TEMOD_TRI2_PORT:    port_mode_sel = 2;  break ;
   case TEMOD_DXGXS:        port_mode_sel = 3;  break ;
   case TEMOD_SINGLE_PORT:  port_mode_sel = 4;  break ;
   default: break ;
 }

  if((pc->sc_mode == TEMOD_SC_MODE_AN_CL37) || (pc->sc_mode == TEMOD_SC_MODE_AN_CL73)) {
      /*data = port_mode_sel << MAIN0_SETUP_PORT_MODE_SEL_SHIFT |*/
   MAIN0_SETUPr_SINGLE_PORT_MODEf_SET(reg, pc->single_port_mode);
   MAIN0_SETUPr_CL37_HIGH_VCOf_SET(reg, pc->cl37_high_vco);
   MAIN0_SETUPr_CL73_LOW_VCOf_SET(reg, pc->cl73_low_vco);
   MAIN0_SETUPr_REFCLK_SELf_SET(reg, pc->refclk);

    PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_SETUPr(pc, reg));
  } else {
   MAIN0_SETUPr_SINGLE_PORT_MODEf_SET(reg, pc->single_port_mode);
   MAIN0_SETUPr_CL37_HIGH_VCOf_SET(reg, pc->cl37_high_vco);
   MAIN0_SETUPr_CL73_LOW_VCOf_SET(reg, pc->cl73_low_vco);
   MAIN0_SETUPr_REFCLK_SELf_SET(reg, pc->refclk);
   MAIN0_SETUPr_PORT_MODE_SELf_SET(reg, port_mode_sel);

   PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_SETUPr(pc, reg));
  }
  return PHYMOD_E_NONE;
}
#endif

#ifdef _DV_TB_
/**
@brief   Read the 16 bit rev. id value etc.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per Port PCS Init

*/
/* TBD  PARAMETERIZE  CODE for SDK*/
int _configure_st_entry(int st_entry_num, int st_entry_speed, PHYMOD_ST* pc)
{
  SC_X1_SPD_OVRR0_SPDr_t    reg0_ovr_spd;
  sC_X1_SPD_OVRR0_0r_t        reg0_ovr;
  SC_X1_SPD_OVRR0_1r_t        reg0_ovr1;
  SC_X1_SPD_OVRR0_2r_t        reg0_ovr2;
  SC_X1_SPD_OVRR0_3r_t        reg0_ovr3;
  SC_X1_SPD_OVRR0_4r_t        reg0_ovr4;
  SC_X1_SPD_OVRR0_5r_t        reg0_ovr5;
  SC_X1_SPD_OVRR0_6r_t        reg0_ovr6;
  SC_X1_SPD_OVRR0_7r_t        reg0_ovr7;
  SC_X1_SPD_OVRR0_8r_t        reg0_ovr8;

  SC_X1_SPD_OVRR1_SPDr_t    reg1_ovr_spd;
  SC_X1_SPD_OVRR1_0r_t        reg1_ovr;
  SC_X1_SPD_OVRR1_1r_t        reg1_ovr1;
  SC_X1_SPD_OVRR1_2r_t        reg1_ovr2;
  SC_X1_SPD_OVRR1_3r_t        reg1_ovr3;
  SC_X1_SPD_OVRR1_4r_t        reg1_ovr4;
  SC_X1_SPD_OVRR1_5r_t        reg1_ovr5;
  SC_X1_SPD_OVRR1_6r_t        reg1_ovr6;
  SC_X1_SPD_OVRR1_7r_t        reg1_ovr7;
  SC_X1_SPD_OVRR1_8r_t        reg1_ovr8;

  SC_X1_SPD_OVRR2_SPDr_t    reg2_ovr_spd;
  SC_X1_SPD_OVRR2_0r_t        reg2_ovr;
  SC_X1_SPD_OVRR2_1r_t        reg2_ovr1;
  SC_X1_SPD_OVRR2_2r_t        reg2_ovr2;
  SC_X1_SPD_OVRR2_3r_t        reg2_ovr3;
  SC_X1_SPD_OVRR2_4r_t        reg2_ovr4;
  SC_X1_SPD_OVRR2_5r_t        reg2_ovr5;
  SC_X1_SPD_OVRR2_6r_t        reg2_ovr6;
  SC_X1_SPD_OVRR2_7r_t        reg2_ovr7;
  SC_X1_SPD_OVRR2_8r_t        reg2_ovr8;

  SC_X1_SPD_OVRR3_SPDr_t    reg3_ovr_spd;
  SC_X1_SPD_OVRR3_0r_t        reg3_ovr;
  SC_X1_SPD_OVRR3_1r_t        reg3_ovr1;
  SC_X1_SPD_OVRR3_2r_t        reg3_ovr2;
  SC_X1_SPD_OVRR3_3r_t        reg3_ovr3;
  SC_X1_SPD_OVRR3_4r_t        reg3_ovr4;
  SC_X1_SPD_OVRR3_5r_t        reg3_ovr5;
  SC_X1_SPD_OVRR3_6r_t        reg3_ovr6;
  SC_X1_SPD_OVRR3_7r_t        reg3_ovr7;
  SC_X1_SPD_OVRR3_8r_t        reg3_ovr8;

if(st_entry_num == 0) {

  SC_X1_SPD_OVRR0_SPDr_CLR(reg0_ovr_spd);
  SC_X1_SPD_OVRR0_SPDr_NUM_LANESf_SET(reg0_ovr_spd, pc->num_lanes);
  SC_X1_SPD_OVRR0_SPDr_SPEEDf_SET(reg0_ovr_spd, st_entry_speed);

  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_SPDr(pc, reg0_ovr_spd));

  SC_X1_SPD_OVRR0_0r_CLR(reg0_ovr);
  SC_X1_SPD_OVRR0_1r_CLR(reg0_ovr1);
  SC_X1_SPD_OVRR0_2r_CLR(reg0_ovr2);
  SC_X1_SPD_OVRR0_3r_CLR(reg0_ovr3);
  SC_X1_SPD_OVRR0_4r_CLR(reg0_ovr4);
  SC_X1_SPD_OVRR0_5r_CLR(reg0_ovr5);
  SC_X1_SPD_OVRR0_6r_CLR(reg0_ovr6);
  SC_X1_SPD_OVRR0_7r_CLR(reg0_ovr7);
  SC_X1_SPD_OVRR0_8r_CLR(reg0_ovr8);

  SC_X1_SPD_OVRR0_0r_ENCODEMODEf_SET(reg0_ovr, pc->t_encode_mode);
  SC_X1_SPD_OVRR0_0r_SCR_MODEf_SET(reg0_ovr, pc->t_scr_mode);
  SC_X1_SPD_OVRR0_0r_OS_MODEf_SET(reg0_ovr, pc->t_pma_os_mode);


  if(pc->cl72_en &  0x40000000)
      SC_X1_SPD_OVRR0_0r_CL72_ENABLEf_SET(reg0_ovr, pc->cl72_en) ;

  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, reg0_ovr));

  SC_X1_SPD_OVRR0_1r_BRCM64B66_DESCRAMBLER_ENABLEf_SET(reg0_ovr1, pc->r_dec1_brcm64b66_descr);
  SC_X1_SPD_OVRR0_1r_CL36BYTEDELETEMODEf_SET(reg0_ovr1, pc->r_desc2_byte_deletion);
  SC_X1_SPD_OVRR0_1r_DESC2_MODEf_SET(reg0_ovr1, pc->r_desc2_mode);
  SC_X1_SPD_OVRR0_1r_DESKEWMODEf_SET(reg0_ovr1, pc->r_deskew_mode);
  SC_X1_SPD_OVRR0_1r_DECODERMODEf_SET(reg0_ovr1, pc->r_dec1_mode);
  SC_X1_SPD_OVRR0_1r_DESCRAMBLERMODEf_SET(reg0_ovr1, pc->r_descr1_mode);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, reg0_ovr1));

  SC_X1_SPD_OVRR0_2r_CL36_ENf_SET(reg0_ovr2, pc->cl36_en);
  SC_X1_SPD_OVRR0_2r_REORDER_ENf_SET(reg0_ovr2, pc->r_reorder_mode);
  SC_X1_SPD_OVRR0_2r_BLOCK_SYNC_MODEf_SET(reg0_ovr2, pc->blk_sync_mode);
  SC_X1_SPD_OVRR0_2r_CHK_END_ENf_SET(reg0_ovr2, pc->cl48_check_end);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_2r(pc, reg0_ovr2));

  SC_X1_SPD_OVRR0_3r_CLOCKCNT0f_SET(reg0_ovr3, pc->clkcnt0);
  SC_X1_SPD_OVRR0_3r_SGMII_SPD_SWITCHf_SET(reg0_ovr3, pc->sgmii);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_3r(pc, reg0_ovr3));

  SC_X1_SPD_OVRR0_4r_CLOCKCNT1f_SET(reg0_ovr4, pc->clkcnt1);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_4r(pc, reg0_ovr4));

  SC_X1_SPD_OVRR0_5r_LOOPCNT1f_SET(reg0_ovr5, pc->lpcnt1);
  SC_X1_SPD_OVRR0_5r_LOOPCNT0f_SET(reg0_ovr5, pc->lpcnt0);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_5r(pc, reg0_ovr5));

  SC_X1_SPD_OVRR0_6r_MAC_CREDITGENCNTf_SET(reg0_ovr6, pc->mac_cgc);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_6r(pc, reg0_ovr6));
  SC_X1_SPD_OVRR0_7r_PCS_CLOCKCNT0f_SET(reg0_ovr7, pc->pcs_clkcnt);
  SC_X1_SPD_OVRR0_7r_PCS_CREDITENABLEf_SET(reg0_ovr7, pc->pcs_crdten);
  SC_X1_SPD_OVRR0_7r_REPLICATION_CNTf_SET(reg0_ovr7, pc->pcs_repcnt);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_7r(pc, reg0_ovr7));

  SC_X1_SPD_OVRR0_8r_PCS_CREDITGENCNTf_SET(reg0_ovr8, pc->pcs_cgc);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_8r(pc, reg0_ovr8));
} /* (st_entry_num == 0) */

else if(st_entry_num == 1) {


 SC_X1_SPD_OVRR1_SPDr_CLR(reg1_ovr_spd);
  SC_X1_SPD_OVRR1_SPDr_NUM_LANESf_SET(reg1_ovr_spd, pc->num_lanes);
  SC_X1_SPD_OVRR1_SPDr_SPEEDf_SET(reg1_ovr_spd, st_entry_speed);

  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_SPDr(pc, reg1_ovr_spd));

  SC_X1_SPD_OVRR1_0r_CLR(reg1_ovr);
  SC_X1_SPD_OVRR1_1r_CLR(reg1_ovr1);
  SC_X1_SPD_OVRR1_2r_CLR(reg1_ovr2);
  SC_X1_SPD_OVRR1_3r_CLR(reg1_ovr3);
  SC_X1_SPD_OVRR1_4r_CLR(reg1_ovr4);
  SC_X1_SPD_OVRR1_5r_CLR(reg1_ovr5);
  SC_X1_SPD_OVRR1_6r_CLR(reg1_ovr6);
  SC_X1_SPD_OVRR1_7r_CLR(reg1_ovr7);
  SC_X1_SPD_OVRR1_8r_CLR(reg1_ovr8);

  SC_X1_SPD_OVRR1_0r_ENCODEMODEf_SET(reg1_ovr, pc->t_encode_mode);
  SC_X1_SPD_OVRR1_0r_SCR_MODEf_SET(reg1_ovr, pc->t_scr_mode);
  SC_X1_SPD_OVRR1_0r_OS_MODEf_SET(reg1_ovr, pc->t_pma_os_mode);


  if(pc->cl72_en &  0x40000000)
      SC_X1_SPD_OVRR1_0r_CL72_ENABLEf_SET(reg1_ovr, pc->cl72_en) ;

  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, reg1_ovr));

  SC_X1_SPD_OVRR1_1r_BRCM64B66_DESCRAMBLER_ENABLEf_SET(reg1_ovr1, pc->r_dec1_brcm64b66_descr);
  SC_X1_SPD_OVRR1_1r_CL36BYTEDELETEMODEf_SET(reg1_ovr1, pc->r_desc2_byte_deletion);
  SC_X1_SPD_OVRR1_1r_DESC2_MODEf_SET(reg1_ovr1, pc->r_desc2_mode);
  SC_X1_SPD_OVRR1_1r_DESKEWMODEf_SET(reg1_ovr1, pc->r_deskew_mode);
  SC_X1_SPD_OVRR1_1r_DECODERMODEf_SET(reg1_ovr1, pc->r_dec1_mode);
  SC_X1_SPD_OVRR1_1r_DESCRAMBLERMODEf_SET(reg1_ovr1, pc->r_descr1_mode);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, reg1_ovr1));

  SC_X1_SPD_OVRR1_2r_CL36_ENf_SET(reg1_ovr2, pc->cl36_en);
  SC_X1_SPD_OVRR1_2r_REORDER_ENf_SET(reg1_ovr2, pc->r_reorder_mode);
  SC_X1_SPD_OVRR1_2r_BLOCK_SYNC_MODEf_SET(reg1_ovr2, pc->blk_sync_mode);
  SC_X1_SPD_OVRR1_2r_CHK_END_ENf_SET(reg1_ovr2, pc->cl48_check_end);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_2r(pc, reg1_ovr2));

  SC_X1_SPD_OVRR1_3r_CLOCKCNT0f_SET(reg1_ovr3, pc->clkcnt0);
  SC_X1_SPD_OVRR1_3r_SGMII_SPD_SWITCHf_SET(reg1_ovr3, pc->sgmii);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_3r(pc, reg1_ovr3));

  SC_X1_SPD_OVRR1_4r_CLOCKCNT1f_SET(reg1_ovr4, pc->clkcnt1);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_4r(pc, reg1_ovr4));

  SC_X1_SPD_OVRR1_5r_LOOPCNT1f_SET(reg1_ovr5, pc->lpcnt1);
  SC_X1_SPD_OVRR1_5r_LOOPCNT0f_SET(reg1_ovr5, pc->lpcnt0);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_5r(pc, reg1_ovr5));

  SC_X1_SPD_OVRR1_6r_MAC_CREDITGENCNTf_SET(reg1_ovr6, pc->mac_cgc);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_6r(pc, reg1_ovr6));
  SC_X1_SPD_OVRR1_7r_PCS_CLOCKCNT0f_SET(reg1_ovr7, pc->pcs_clkcnt);
  SC_X1_SPD_OVRR1_7r_PCS_CREDITENABLEf_SET(reg1_ovr7, pc->pcs_crdten);
  SC_X1_SPD_OVRR1_7r_REPLICATION_CNTf_SET(reg1_ovr7, pc->pcs_repcnt);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_7r(pc, reg1_ovr7));

  SC_X1_SPD_OVRR1_8r_PCS_CREDITGENCNTf_SET(reg1_ovr8, pc->pcs_cgc);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_8r(pc, reg1_ovr8));



} /* (st_entry_num == 1) */

else if(st_entry_num == 2) {

 SC_X1_SPD_OVRR2_SPDr_CLR(reg2_ovr_spd);
  SC_X1_SPD_OVRR2_SPDr_NUM_LANESf_SET(reg2_ovr_spd, pc->num_lanes);
  SC_X1_SPD_OVRR2_SPDr_SPEEDf_SET(reg2_ovr_spd, st_entry_speed);

  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_SPDr(pc, reg2_ovr_spd));

  SC_X1_SPD_OVRR2_0r_CLR(reg2_ovr);
  SC_X1_SPD_OVRR2_1r_CLR(reg2_ovr1);
  SC_X1_SPD_OVRR2_2r_CLR(reg2_ovr2);
  SC_X1_SPD_OVRR2_3r_CLR(reg2_ovr3);
  SC_X1_SPD_OVRR2_4r_CLR(reg2_ovr4);
  SC_X1_SPD_OVRR2_5r_CLR(reg2_ovr5);
  SC_X1_SPD_OVRR2_6r_CLR(reg2_ovr6);
  SC_X1_SPD_OVRR2_7r_CLR(reg2_ovr7);
  SC_X1_SPD_OVRR2_8r_CLR(reg2_ovr8);

  SC_X1_SPD_OVRR2_0r_ENCODEMODEf_SET(reg2_ovr, pc->t_encode_mode);
  SC_X1_SPD_OVRR2_0r_SCR_MODEf_SET(reg2_ovr, pc->t_scr_mode);
  SC_X1_SPD_OVRR2_0r_OS_MODEf_SET(reg2_ovr, pc->t_pma_os_mode);


  if(pc->cl72_en &  0x40000000)
      SC_X1_SPD_OVRR2_0r_CL72_ENABLEf_SET(reg2_ovr, pc->cl72_en) ;

  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, reg2_ovr));

  SC_X1_SPD_OVRR2_1r_BRCM64B66_DESCRAMBLER_ENABLEf_SET(reg2_ovr1, pc->r_dec1_brcm64b66_descr);
  SC_X1_SPD_OVRR2_1r_CL36BYTEDELETEMODEf_SET(reg2_ovr1, pc->r_desc2_byte_deletion);
  SC_X1_SPD_OVRR2_1r_DESC2_MODEf_SET(reg2_ovr1, pc->r_desc2_mode);
  SC_X1_SPD_OVRR2_1r_DESKEWMODEf_SET(reg2_ovr1, pc->r_deskew_mode);
  SC_X1_SPD_OVRR2_1r_DECODERMODEf_SET(reg2_ovr1, pc->r_dec1_mode);
  SC_X1_SPD_OVRR2_1r_DESCRAMBLERMODEf_SET(reg2_ovr1, pc->r_descr1_mode);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, reg2_ovr1));

  SC_X1_SPD_OVRR2_2r_CL36_ENf_SET(reg2_ovr2, pc->cl36_en);
  SC_X1_SPD_OVRR2_2r_REORDER_ENf_SET(reg2_ovr2, pc->r_reorder_mode);
  SC_X1_SPD_OVRR2_2r_BLOCK_SYNC_MODEf_SET(reg2_ovr2, pc->blk_sync_mode);
  SC_X1_SPD_OVRR2_2r_CHK_END_ENf_SET(reg2_ovr2, pc->cl48_check_end);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_2r(pc, reg2_ovr2));

  SC_X1_SPD_OVRR2_3r_CLOCKCNT0f_SET(reg2_ovr3, pc->clkcnt0);
  SC_X1_SPD_OVRR2_3r_SGMII_SPD_SWITCHf_SET(reg2_ovr3, pc->sgmii);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_3r(pc, reg2_ovr3));

  SC_X1_SPD_OVRR2_4r_CLOCKCNT1f_SET(reg2_ovr4, pc->clkcnt1);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_4r(pc, reg2_ovr4));

  SC_X1_SPD_OVRR2_5r_LOOPCNT1f_SET(reg2_ovr5, pc->lpcnt1);
  SC_X1_SPD_OVRR2_5r_LOOPCNT0f_SET(reg2_ovr5, pc->lpcnt0);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_5r(pc, reg2_ovr5));

  SC_X1_SPD_OVRR2_6r_MAC_CREDITGENCNTf_SET(reg2_ovr6, pc->mac_cgc);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_6r(pc, reg2_ovr6));
  SC_X1_SPD_OVRR2_7r_PCS_CLOCKCNT0f_SET(reg2_ovr7, pc->pcs_clkcnt);
  SC_X1_SPD_OVRR2_7r_PCS_CREDITENABLEf_SET(reg2_ovr7, pc->pcs_crdten);
  SC_X1_SPD_OVRR2_7r_REPLICATION_CNTf_SET(reg2_ovr7, pc->pcs_repcnt);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_7r(pc, reg2_ovr7));

  SC_X1_SPD_OVRR2_8r_PCS_CREDITGENCNTf_SET(reg2_ovr8, pc->pcs_cgc);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_8r(pc, reg2_ovr8));
} /* (st_entry_num == 2) */

else  /*(st_entry_num == 3) */ {

 SC_X1_SPD_OVRR3_SPDr_CLR(reg3_ovr_spd);
  SC_X1_SPD_OVRR3_SPDr_NUM_LANESf_SET(reg3_ovr_spd, pc->num_lanes);
  SC_X1_SPD_OVRR3_SPDr_SPEEDf_SET(reg3_ovr_spd, st_entry_speed);

  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_SPDr(pc, reg3_ovr_spd));

  SC_X1_SPD_OVRR3_0r_CLR(reg3_ovr);
  SC_X1_SPD_OVRR3_1r_CLR(reg3_ovr1);
  SC_X1_SPD_OVRR3_2r_CLR(reg3_ovr2);
  SC_X1_SPD_OVRR3_3r_CLR(reg3_ovr3);
  SC_X1_SPD_OVRR3_4r_CLR(reg3_ovr4);
  SC_X1_SPD_OVRR3_5r_CLR(reg3_ovr5);
  SC_X1_SPD_OVRR3_6r_CLR(reg3_ovr6);
  SC_X1_SPD_OVRR3_7r_CLR(reg3_ovr7);
  SC_X1_SPD_OVRR3_8r_CLR(reg3_ovr8);

  SC_X1_SPD_OVRR3_0r_ENCODEMODEf_SET(reg3_ovr, pc->t_encode_mode);
  SC_X1_SPD_OVRR3_0r_SCR_MODEf_SET(reg3_ovr, pc->t_scr_mode);
  SC_X1_SPD_OVRR3_0r_OS_MODEf_SET(reg3_ovr, pc->t_pma_os_mode);


  if(pc->cl72_en &  0x40000000)
      SC_X1_SPD_OVRR3_0r_CL72_ENABLEf_SET(reg3_ovr, pc->cl72_en) ;

  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, reg3_ovr));

  SC_X1_SPD_OVRR3_1r_BRCM64B66_DESCRAMBLER_ENABLEf_SET(reg3_ovr1, pc->r_dec1_brcm64b66_descr);
  SC_X1_SPD_OVRR3_1r_CL36BYTEDELETEMODEf_SET(reg3_ovr1, pc->r_desc2_byte_deletion);
  SC_X1_SPD_OVRR3_1r_DESC2_MODEf_SET(reg3_ovr1, pc->r_desc2_mode);
  SC_X1_SPD_OVRR3_1r_DESKEWMODEf_SET(reg3_ovr1, pc->r_deskew_mode);
  SC_X1_SPD_OVRR3_1r_DECODERMODEf_SET(reg3_ovr1, pc->r_dec1_mode);
  SC_X1_SPD_OVRR3_1r_DESCRAMBLERMODEf_SET(reg3_ovr1, pc->r_descr1_mode);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, reg3_ovr1));

  SC_X1_SPD_OVRR3_2r_CL36_ENf_SET(reg3_ovr2, pc->cl36_en);
  SC_X1_SPD_OVRR3_2r_REORDER_ENf_SET(reg3_ovr2, pc->r_reorder_mode);
  SC_X1_SPD_OVRR3_2r_BLOCK_SYNC_MODEf_SET(reg3_ovr2, pc->blk_sync_mode);
  SC_X1_SPD_OVRR3_2r_CHK_END_ENf_SET(reg3_ovr2, pc->cl48_check_end);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_2r(pc, reg3_ovr2));

  SC_X1_SPD_OVRR3_3r_CLOCKCNT0f_SET(reg3_ovr3, pc->clkcnt0);
  SC_X1_SPD_OVRR3_3r_SGMII_SPD_SWITCHf_SET(reg3_ovr3, pc->sgmii);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_3r(pc, reg3_ovr3));

  SC_X1_SPD_OVRR3_4r_CLOCKCNT1f_SET(reg3_ovr4, pc->clkcnt1);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_4r(pc, reg3_ovr4));

  SC_X1_SPD_OVRR3_5r_LOOPCNT1f_SET(reg3_ovr5, pc->lpcnt1);
  SC_X1_SPD_OVRR3_5r_LOOPCNT0f_SET(reg3_ovr5, pc->lpcnt0);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_5r(pc, reg3_ovr5));

  SC_X1_SPD_OVRR3_6r_MAC_CREDITGENCNTf_SET(reg3_ovr6, pc->mac_cgc);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_6r(pc, reg3_ovr6));
  SC_X1_SPD_OVRR3_7r_PCS_CLOCKCNT0f_SET(reg3_ovr7, pc->pcs_clkcnt);
  SC_X1_SPD_OVRR3_7r_PCS_CREDITENABLEf_SET(reg3_ovr7, pc->pcs_crdten);
  SC_X1_SPD_OVRR3_7r_REPLICATION_CNTf_SET(reg3_ovr7, pc->pcs_repcnt);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_7r(pc, reg3_ovr7));

  SC_X1_SPD_OVRR3_8r_PCS_CREDITGENCNTf_SET(reg3_ovr8, pc->pcs_cgc);
  PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_8r(pc, reg3_ovr8));
} /* (st_entry_num == 3) */

  return PHYMOD_E_NONE;
}
#endif

#ifdef _DV_TB_
/**
@brief   Read the 16 bit rev. id value etc.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per Port PCS Init
*/
int _configure_sc_speed_configuration(PHYMOD_ST* pc)
{
  int data =0;
  int mask =0;
  int st_entry =0;
  int speed_entry = 0;
  PHYMOD_ST pc_st_tmp;
  PHYMOD_ST* pc_st;

  if(pc->sc_mode == TEMOD_SC_MODE_HT_WITH_BASIC_OVERRIDE){
    /* Configure the ffec_eb regsiter */
    data |= (1 << 10);
    mask |= 0x0400;

  }
  else if(pc->sc_mode == TEMOD_SC_MODE_ST_OVERRIDE) {
  }
  else if(pc->sc_mode == TEMOD_SC_MODE_ST) {
     st_entry = pc->per_lane_control & 0xff;
     speed_entry = (pc->per_lane_control & 0xff00) >> 8;

/*
    A pc_st_tmp is created so the overriden expected values passed from SV are not 
    loaded in ST, and teh ST has the same HT values
*/
     init_temod_st(&pc_st_tmp);
     pc_st_tmp.spd_intf = pc->spd_intf;
     pc_st_tmp.prt_ad = pc->prt_ad;
     pc_st_tmp.unit = pc->unit;
     pc_st_tmp.port_type = pc->port_type;
     pc_st_tmp.this_lane = pc->this_lane;
     pc_st_tmp.sc_mode = pc->sc_mode;
     pc_st_tmp.cl72_en = pc->cl72_en;
     pc_st = &pc_st_tmp;
     temod_sc_lkup_table(pc_st);
     /*configure the ST regsiters */
     _configure_st_entry(st_entry, speed_entry,  pc_st);
  }
  return PHYMOD_E_NONE;
}
#endif

#ifdef _DV_TB_
/**
@brief   Read the 16 bit rev. id value etc.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per Port PCS Init
*/
int _init_pcs_an(PHYMOD_ST* pc)             
{
  UC_COMMAND4r_t reg_com4;
  UC_COMMANDr_t reg_com;

  UC_COMMAND4r_CLR(reg_com4);
  UC_COMMANDr_CLR(reg_com);

  UC_COMMAND4r_MICRO_SYSTEM_CLK_ENf_SET(reg_com4,1);
  UC_COMMAND4r_MICRO_SYSTEM_RESET_Nf_SET(reg_com4,1);
  PHYMOD_IF_ERR_RETURN(MODIFY_UC_COMMAND4r(pc, reg_com4));
    
  UC_COMMANDr_MICRO_MDIO_DW8051_RESET_Nf_SET(reg_com,1);
  PHYMOD_IF_ERR_RETURN(MODIFY_UC_COMMANDr(pc, reg_com));

  temod_set_an_port_mode(pc);
  pc->port_type=TEMOD_MULTI_PORT;
  temod_autoneg_set(pc);
  temod_autoneg_control(pc);

   return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */
#ifdef _SDK_TEMOD_TEMP_ 

int _init_pcs_an(PHYMOD_ST* pc)             
{
  temod_an_set_type_t ata_type;
  temod_tech_ability an_tech_ability = 0; 
  temod_cl37bam_ability an_cl37bam_ability = 0;
  temod_an_mode_type_t an_mode = 0;
  int data;
    
  /* temod_autoneg_init(pc); */
  temod_autoneg_timer_init(pc); 
  /* temod_tick_override_set(pc, pc->an_tick_override, pc->an_tick_numerator, pc->an_tick_denominator); */
  temod_set_an_port_mode(pc, pc->no_of_lanes, pc->this_lane, pc->single_port);

  /* TBD: add locgic to port the info from pc to an_ability_st and an_control */
  temod_autoneg_set( pc, an_ability_st);
  temod_autoneg_control(pc, an_control)

  return PHYMOD_E_NONE;
}
#endif /* _SDK_TEMOD_TEMP_ */

/**
@brief   Function to control the configuration of PCS hardware blocks
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details This function triggers the PCS speed-config logic to go ahead and
configure various PCS blocks for the desired speed. The speed (which is an
enumerated type) is already known. This trigger also controls PMD datapath
resets. So sometimes we use this function just to toggle PMD datapath resets as
well.
*/

int temod_trigger_speed_change(PHYMOD_ST* pc)
{
  SC_X4_CTLr_t    reg; 
  
  TEMOD_DBG_IN_FUNC_INFO(pc);
#ifdef _DV_TB_
  pc->adjust_port_mode = 1; 
#endif /* _DV_TB_ */

  /* write 0 to the speed change */
  SC_X4_CTLr_CLR(reg);
  PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc, &reg));
  SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg, 0);
  PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_CTLr(pc, reg));

  /* write 1 to the speed change. No need to read again before write*/
  SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg, 1);
  PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_CTLr(pc, reg));

#ifdef _DV_TB_
  pc->adjust_port_mode = 0; 
#endif /* _DV_TB_ */
  return PHYMOD_E_NONE;
}

#ifdef _DV_TB_
/**
@brief   Compare status registers in PCS with reference values in TEMod
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details After PCS programms various PCS blocks, the status is updated. This
function reads out the updated status and validates them.
*/
int _read_and_compare_final_status(PHYMOD_ST* pc)             
{
  uint16_t fail;
   int  actual_speed;
   PHYMOD_ST   sc_final_stats_actual;
   PHYMOD_ST*  sc_final_stats_actual_tmp;
   SC_X4_RSLVD_SPDr_t    reg_resolved_spd;
   SC_X4_RSLVD0r_t        reg_resolved_0;
   SC_X4_RSLVD1r_t        reg_resolved_1;
   SC_X4_RSLVD2r_t        reg_resolved_2;
   SC_X4_RSLVD3r_t        reg_resolved_3;
   SC_X4_RSLVD4r_t        reg_resolved_4;
   SC_X4_RSLVD5r_t        reg_resolved_5;
   SC_X4_RSLVD6r_t        reg_resolved_6;
   SC_X4_RSLVD7r_t        reg_resolved_7;
   SC_X4_RSLVD8r_t        reg_resolved_8;

   SC_X4_RSLVD_SPDr_CLR(reg_resolved_spd);
   PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD_SPDr(pc, &reg_resolved_spd));
   sc_final_stats_actual.num_lanes = SC_X4_RSLVD_SPDr_NUM_LANESf_GET(reg_resolved_spd);
   actual_speed = SC_X4_RSLVD_SPDr_SPEEDf_GET(reg_resolved_spd);
   sc_final_stats_actual.speed = actual_speed;

   SC_X4_RSLVD0r_CLR(reg_resolved_0);
   PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD0r(pc, &reg_resolved_0));
   sc_final_stats_actual.t_encode_mode = SC_X4_RSLVD0r_ENCODEMODEf_GET(reg_resolved_0);
   sc_final_stats_actual.cl72_en = SC_X4_RSLVD0r_CL72_ENABLEf_GET(reg_resolved_0);
   sc_final_stats_actual.fec_en = SC_X4_RSLVD0r_FEC_ENABLEf_GET(reg_resolved_0);
   sc_final_stats_actual.t_scr_mode = SC_X4_RSLVD0r_SCR_MODEf_GET(reg_resolved_0);
   sc_final_stats_actual.t_pma_os_mode = SC_X4_RSLVD0r_OS_MODEf_GET(reg_resolved_0);

   SC_X4_RSLVD1r_CLR(reg_resolved_1);
   PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD1r(pc, &reg_resolved_1));
   sc_final_stats_actual.r_dec1_brcm64b66_descr = SC_X4_RSLVD1r_BRCM64B66_DESCRAMBLER_ENABLEf_GET(reg_resolved_1);
   sc_final_stats_actual.r_desc2_byte_deletion = SC_X4_RSLVD1r_CL36BYTEDELETEMODEf_GET(reg_resolved_1);
   sc_final_stats_actual.r_desc2_mode = SC_X4_RSLVD1r_DESC2_MODEf_GET(reg_resolved_1);
   sc_final_stats_actual.r_deskew_mode = SC_X4_RSLVD1r_DESKEWMODEf_GET(reg_resolved_1);
   sc_final_stats_actual.r_dec1_mode = SC_X4_RSLVD1r_DECODERMODEf_GET(reg_resolved_1);
   sc_final_stats_actual.r_descr1_mode = SC_X4_RSLVD1r_DESCRAMBLERMODEf_GET(reg_resolved_1);

   SC_X4_RSLVD2r_CLR(reg_resolved_2);
   PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD2r(pc, &reg_resolved_2));
   sc_final_stats_actual.cl36_en = SC_X4_RSLVD2r_CL36_ENf_GET(reg_resolved_2);
   sc_final_stats_actual.r_reorder_mode = SC_X4_RSLVD2r_REORDER_ENf_GET(reg_resolved_2);
   sc_final_stats_actual.blk_sync_mode =  SC_X4_RSLVD2r_BLOCK_SYNC_MODEf_GET(reg_resolved_2);
   sc_final_stats_actual.cl48_check_end = SC_X4_RSLVD2r_CHK_END_ENf_GET(reg_resolved_2);

   SC_X4_RSLVD3r_CLR(reg_resolved_3);
   PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD3r(pc, &reg_resolved_3));
   sc_final_stats_actual.clkcnt0 = SC_X4_RSLVD3r_CLOCKCNT0f_GET(reg_resolved_3);
   sc_final_stats_actual.sgmii =  SC_X4_RSLVD3r_SGMII_SPD_SWITCHf_GET(reg_resolved_3);

   SC_X4_RSLVD4r_CLR(reg_resolved_4);
   PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD4r(pc, &reg_resolved_4));
   sc_final_stats_actual.clkcnt1 = SC_X4_RSLVD4r_CLOCKCNT1f_GET(reg_resolved_4);

   SC_X4_RSLVD5r_CLR(reg_resolved_5);
   PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD5r(pc, &reg_resolved_5));
   sc_final_stats_actual.lpcnt1 = SC_X4_RSLVD5r_LOOPCNT1f_GET(reg_resolved_5);
   sc_final_stats_actual.lpcnt0 = SC_X4_RSLVD5r_LOOPCNT0f_GET(reg_resolved_5);

   SC_X4_RSLVD6r_CLR(reg_resolved_6);
   PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD6r(pc, &reg_resolved_6));
   sc_final_stats_actual.mac_cgc = SC_X4_RSLVD6r_MAC_CREDITGENCNTf_GET(reg_resolved_6);

   SC_X4_RSLVD7r_CLR(reg_resolved_7);
   PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD7r(pc, &reg_resolved_7));
   sc_final_stats_actual.pcs_clkcnt = SC_X4_RSLVD7r_PCS_CLOCKCNT0f_GET(reg_resolved_7);
   sc_final_stats_actual.pcs_crdten = SC_X4_RSLVD7r_PCS_CREDITENABLEf_GET(reg_resolved_7);
   sc_final_stats_actual.pcs_repcnt = SC_X4_RSLVD7r_REPLICATION_CNTf_GET(reg_resolved_7);

   SC_X4_RSLVD8r_CLR(reg_resolved_8);
   PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD8r(pc, &reg_resolved_8));
   sc_final_stats_actual.pcs_cgc = SC_X4_RSLVD8r_PCS_CREDITGENCNTf_GET(reg_resolved_8);

   /* Compare the actual and expected stats */
   fail = 0;


   if(pc->sc_mode == TEMOD_SC_MODE_ST) {
     if(actual_speed != (pc->per_lane_control >> 8))
        fail = 1; 
   } else if((pc->per_lane_control & 0xffff) == (0x004 | (TEMOD_SPD_10000_XFI << 8))) { /* DV Verification Only */
     if(actual_speed != digital_operationSpeeds_SPEED_10G_KR1)
        fail = 1; 
   } else if((pc->per_lane_control & 0xffff) == (0x004 | (TEMOD_SPD_10_SGMII << 8)))  { /* DV Verification Only */
     if(actual_speed != digital_operationSpeeds_SPEED_10M)
        fail = 1; 
   } else if((pc->per_lane_control & 0xffff) == (0x004 | (TEMOD_SPD_5000_XFI << 8)))  { /* DV Verification Only */
     if(actual_speed != digital_operationSpeeds_SPEED_5G_KR1)
        fail = 1; 
   } else {
     if(actual_speed != pc->speed)
        fail = 1; 
   }

   if(sc_final_stats_actual.num_lanes != pc->num_lanes)
      fail = 1;
   if(sc_final_stats_actual.t_pma_os_mode != pc->t_pma_os_mode)
      fail = 1;
   if(sc_final_stats_actual.t_scr_mode != pc->t_scr_mode)
      fail = 1;
   if(sc_final_stats_actual.fec_en != pc->fec_en)
      fail = 1;
   if(sc_final_stats_actual.t_encode_mode != pc->t_encode_mode)
      fail = 1;

    
    if(pc->cl72_en  & 0x08000000) {
     if(sc_final_stats_actual.cl72_en != (pc->cl72_en & 0xffff))
         fail = 1;
    }

   if(sc_final_stats_actual.r_descr1_mode != pc->r_descr1_mode)
      fail = 1;
   if(sc_final_stats_actual.r_dec1_mode != pc->r_dec1_mode)
      fail = 1;
   if(sc_final_stats_actual.r_deskew_mode != pc->r_deskew_mode)
      fail = 1;
   if(sc_final_stats_actual.r_desc2_mode != pc->r_desc2_mode)
      fail = 1;
   if(sc_final_stats_actual.r_desc2_byte_deletion != pc->r_desc2_byte_deletion)
      fail = 1;
   if(sc_final_stats_actual.r_dec1_brcm64b66_descr != pc->r_dec1_brcm64b66_descr)
      fail = 1;

   if(sc_final_stats_actual.cl48_check_end != pc->cl48_check_end)
      fail = 1;
   if(sc_final_stats_actual.blk_sync_mode != pc->blk_sync_mode)
      fail = 1;
   if(sc_final_stats_actual.r_reorder_mode != pc->r_reorder_mode)
      fail = 1;
   if(sc_final_stats_actual.cl36_en != pc->cl36_en)
      fail = 1;

   if(sc_final_stats_actual.sgmii != pc->sgmii)
      fail = 1;
   if(sc_final_stats_actual.clkcnt0 != pc->clkcnt0)
      fail = 1;
   if(sc_final_stats_actual.clkcnt1 != pc->clkcnt1)
      fail = 1;
   if(sc_final_stats_actual.lpcnt0 != pc->lpcnt0)
      fail = 1;
   if(sc_final_stats_actual.lpcnt1 != pc->lpcnt1)
      fail = 1;
   if(sc_final_stats_actual.mac_cgc != pc->mac_cgc)
      fail = 1;
   if(sc_final_stats_actual.pcs_repcnt != pc->pcs_repcnt)
      fail = 1;
   if(sc_final_stats_actual.pcs_crdten != pc->pcs_crdten)
      fail = 1;
   if(sc_final_stats_actual.pcs_clkcnt != pc->pcs_clkcnt)
      fail = 1;
   if(sc_final_stats_actual.pcs_cgc != pc->pcs_cgc)
      fail = 1;

   pc->accData   = fail;
   if(fail == 1) {
#ifdef _DV_TB_
      printf("Actual stats:\n");
      sc_final_stats_actual_tmp = &sc_final_stats_actual;
      print_temod_sc_lkup_table(sc_final_stats_actual_tmp);
      printf("Expected stats:\n");
      print_temod_sc_lkup_table(pc);
#endif
      return PHYMOD_E_FAIL;
   }
   return PHYMOD_E_NONE;
}
#endif

#ifdef _DV_TB_
/**
@brief   A DV only Tier2 function to initialize the entire PCS
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per Port PCS Init for forced speeds
*/
int _init_pcs_fs(PHYMOD_ST* pc)             
{
  uint16_t data;
  SC_X4_CTLr_t    reg_swmode;

  _get_sc_speed_configuration(pc);
  _configure_sc_speed_configuration(pc);
   
  /* Set speed */
  if(pc->sc_mode == TEMOD_SC_MODE_ST) 
    data = (pc->per_lane_control & 0xff00) >> 8;
  else if((pc->per_lane_control & 0xffff) == (0x004 | (TEMOD_SPD_10000_XFI << 8))) /* DV Verification Only */
    data = digital_operationSpeeds_SPEED_10G_KR1;
  else if((pc->per_lane_control & 0xffff) == (0x004 | (TEMOD_SPD_10_SGMII << 8))) /* DV Verification Only */
    data = digital_operationSpeeds_SPEED_10M;
  else if((pc->per_lane_control & 0xffff) == (0x004 | (TEMOD_SPD_5000_XFI << 8))) /* DV Verification Only */
    data = digital_operationSpeeds_SPEED_5G_KR1;
  else
    data = pc->speed;

   SC_X4_CTLr_CLR(reg_swmode);
   SC_X4_CTLr_SW_SPEEDf_SET(reg_swmode, data);
   PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc, reg_swmode));

   temod_trigger_speed_change(pc); 
   _temod_wait_sc_stats_set(pc);

  return PHYMOD_E_NONE;
}
#endif


#ifdef _DV_TB_
/**
@brief   Initialize the PMD for QSGMII PCS. (different from TSCE PCS)
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   pmd_touched   Is this the first time we are visiting the PMD.
@param   uc_active  is the uctlr already active (implies firmware loaded)
@returns The value PHYMOD_E_NONE upon successful completion
@details Details pending. 
*/
int temod_init_pmd_qsgmii(PHYMOD_ST* pc, int pmd_touched, int uc_active) /* INIT_PMD_SGMII */
{
  CKRST_LN_CLK_RST_N_PWRDWN_CTLr_t   reg_pwr_dwn_ctrl;
  DIG_TOP_USER_CTL0r_t               reg_usr_ctrl;
  PLL_CAL_CTL7r_t                    reg_ctrl7;
  CKRST_OSR_MODE_CTLr_t              reg_osr_mode_ctrl;

  TEMOD_DBG_IN_FUNC_INFO(pc);

  CKRST_LN_CLK_RST_N_PWRDWN_CTLr_CLR(reg_pwr_dwn_ctrl);
  CKRST_LN_CLK_RST_N_PWRDWN_CTLr_LN_DP_S_RSTBf_SET(reg_pwr_dwn_ctrl, 1);
  PHYMOD_IF_ERR_RETURN(MODIFY_CKRST_LN_CLK_RST_N_PWRDWN_CTLr(pc, reg_pwr_dwn_ctrl));

  if (pmd_touched == 0) {
    DIG_TOP_USER_CTL0r_CLR(reg_usr_ctrl);
    if(uc_active == 1){

     DIG_TOP_USER_CTL0r_UC_ACTIVEf_SET(reg_usr_ctrl, 1);
     DIG_TOP_USER_CTL0r_CORE_DP_S_RSTBf_SET(reg_usr_ctrl, 1);

      /* release reset to pll data path. TBD need for all lanes  and uc_active set */
      PHYMOD_IF_ERR_RETURN (MODIFY_DIG_TOP_USER_CTL0r(pc, reg_usr_ctrl));
    }
    else{
      DIG_TOP_USER_CTL0r_CORE_DP_S_RSTBf_SET(reg_usr_ctrl, 1);
      PHYMOD_IF_ERR_RETURN (MODIFY_DIG_TOP_USER_CTL0r(pc, reg_usr_ctrl));
    }
  }

  /* set vco at 10G */
  PLL_CAL_CTL7r_CLR(reg_ctrl7);
  PLL_CAL_CTL7r_PLL_MODEf_SET(reg_ctrl7, 0x9);
  PHYMOD_IF_ERR_RETURN (MODIFY_PLL_CAL_CTL7r(pc, reg_ctrl7));

  /* set OS2 mode */
  CKRST_OSR_MODE_CTLr_CLR(reg_osr_mode_ctrl);
  CKRST_OSR_MODE_CTLr_OSR_MODE_FRCf_SET(reg_osr_mode_ctrl, 1);
  CKRST_OSR_MODE_CTLr_OSR_MODE_FRC_VALf_SET(reg_osr_mode_ctrl, 1);

  PHYMOD_IF_ERR_RETURN(MODIFY_CKRST_OSR_MODE_CTLr(pc, reg_osr_mode_ctrl));

  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */


/**
@brief   TSC12 DV stuff. Incomplete
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   cl82_multi_pipe_mode  Multi Mode must be set when a core is part of 100G
@param   cl82_mld_phys_map  lane allocation for this 100G setup   
@returns The value PHYMOD_E_NONE upon successful completion.
@details Called to Configure tsc12 for 100G+ speeds.
For 100/120G you will need to configure the triple-core-set to either have 20 or 24
virtual lanes. Further for 100G you can have 3-4-3 or 2-4-4 or 4-4-2 type usage
of lanes
*/

int temod_tsc12_control(PHYMOD_ST* pc, int cl82_multi_pipe_mode, int cl82_mld_phys_map)   /* TSC12_CONTROL */
{
  MAIN0_MISCr_t reg_misc;
  TEMOD_DBG_IN_FUNC_INFO(pc);
  MAIN0_MISCr_CLR(reg_misc);

  MAIN0_MISCr_CL82_MULTI_PIPE_MODEf_SET(reg_misc, (cl82_multi_pipe_mode & 0x3));
  MAIN0_MISCr_CL82_MLD_PHYS_MAPf_SET(reg_misc, (cl82_mld_phys_map & 0x3));

  PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_MISCr(pc, reg_misc));
  return PHYMOD_E_NONE;
}

#ifdef _DV_TB_
/**
@brief   Checks to ensure SC has programmed the PCS as expected
@param   pc handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion.
@details Called to Configure tsc12 for 100G+ speeds
*/

int temod_check_sc_stats(PHYMOD_ST* pc)   /* CHECK_SC_STATS */
{
  uint16_t data ;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  data = _read_and_compare_final_status(pc); 
  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */

#ifdef _DV_TB_
/**
@brief   get the rx lane reset  staus for any lane
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   value get the status as *value
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function enables/disables rx lane (RSTB_LANE) or read back control bit for
that based on per_lane_control being 1 or 0. If per_lane_control is 0xa, only
read back RSTB_LANE.
*/
int temod_rx_lane_control_get(PHYMOD_ST* pc, int *value)         /* RX_LANE_CONTROL */
{
  RX_X4_PMA_CTL0r_t    reg_pma_ctrl;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  RX_X4_PMA_CTL0r_CLR(reg_pma_ctrl);

  PHYMOD_IF_ERR_RETURN (READ_RX_X4_PMA_CTL0r(pc, &reg_pma_ctrl));
  if( RX_X4_PMA_CTL0r_RSTB_LANEf_GET(reg_pma_ctrl) )
    *value = 1;
  else
    *value = 0;

  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_ */


/**
@brief   rx lane reset and enable of any particular lane
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable to reset the lane.
@returns The value PHYMOD_E_NONE upon successful completion.
@details This function enables/disables rx lane (RSTB_LANE).
*/
int temod_rx_lane_control_set(PHYMOD_ST* pc, int enable)         /* RX_LANE_CONTROL */
{
  RX_X4_PMA_CTL0r_t    reg_pma_ctrl;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  RX_X4_PMA_CTL0r_CLR(reg_pma_ctrl);
  if (enable) {
    RX_X4_PMA_CTL0r_RSTB_LANEf_SET(reg_pma_ctrl, 1);
    PHYMOD_IF_ERR_RETURN (MODIFY_RX_X4_PMA_CTL0r(pc, reg_pma_ctrl));
  } else {
     /* bit set to 0 for disabling RXP */
    RX_X4_PMA_CTL0r_RSTB_LANEf_SET( reg_pma_ctrl, 0);
    PHYMOD_IF_ERR_RETURN (MODIFY_RX_X4_PMA_CTL0r(pc, reg_pma_ctrl));
  }
  return PHYMOD_E_NONE;
}

#ifdef _DV_TB_
int temod_prbs_rx_enable_set(PHYMOD_ST* pc, int enable)
{
  TLB_RX_PRBS_CHK_CFGr_t    reg_prbs_chk_cfg;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TLB_RX_PRBS_CHK_CFGr_CLR(reg_prbs_chk_cfg);

  TLB_RX_PRBS_CHK_CFGr_PRBS_CHK_ENf_SET(reg_prbs_chk_cfg, enable);
  PHYMOD_IF_ERR_RETURN (MODIFY_TLB_RX_PRBS_CHK_CFGr(pc, reg_prbs_chk_cfg));

  return PHYMOD_E_NONE;
}
#endif

#ifdef _DV_TB_
int temod_prbs_tx_enable_set(PHYMOD_ST* pc, int enable)                /* PRBS_MODE */
{
  TLB_TX_PRBS_GEN_CFGr_t    reg_prbs_gen_cfg;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TLB_TX_PRBS_GEN_CFGr_CLR(reg_prbs_gen_cfg);

  TLB_TX_PRBS_GEN_CFGr_PRBS_GEN_ENf_SET(reg_prbs_gen_cfg, enable);
  PHYMOD_IF_ERR_RETURN (MODIFY_TLB_TX_PRBS_GEN_CFGr(pc, reg_prbs_gen_cfg));

  return PHYMOD_E_NONE;

} /* PRBS_MODE */
#endif

#ifdef _DV_TB_
int temod_prbs_rx_invert_data_set(PHYMOD_ST* pc, int invert_data)                /* PRBS_MODE */
{
  TLB_RX_PRBS_CHK_CFGr_t    reg_prbs_chk_cfg;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TLB_RX_PRBS_CHK_CFGr_CLR(reg_prbs_chk_cfg);

  TLB_RX_PRBS_CHK_CFGr_PRBS_CHK_INVf_SET(reg_prbs_chk_cfg, invert_data) ;
  PHYMOD_IF_ERR_RETURN (MODIFY_TLB_RX_PRBS_CHK_CFGr(pc, reg_prbs_chk_cfg));

  return PHYMOD_E_NONE;

} /* PRBS_MODE */
#endif

#ifdef _DV_TB_
int temod_prbs_rx_check_mode_set(PHYMOD_ST* pc, int check_mode)                /* PRBS_MODE */
{
  TLB_RX_PRBS_CHK_CFGr_t    reg_prbs_chk_cfg;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TLB_RX_PRBS_CHK_CFGr_CLR(reg_prbs_chk_cfg);

  TLB_RX_PRBS_CHK_CFGr_PRBS_CHK_MODEf_SET(reg_prbs_chk_cfg, check_mode);
  PHYMOD_IF_ERR_RETURN (MODIFY_TLB_RX_PRBS_CHK_CFGr(pc, reg_prbs_chk_cfg));

  return PHYMOD_E_NONE;

} /* PRBS_MODE */
#endif

#ifdef _DV_TB_
int temod_prbs_tx_invert_data_set(PHYMOD_ST* pc, int invert_data)                /* PRBS_MODE */
{
  TLB_TX_PRBS_GEN_CFGr_t    reg_prbs_gen_cfg;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TLB_TX_PRBS_GEN_CFGr_CLR(reg_prbs_gen_cfg);

  TLB_TX_PRBS_GEN_CFGr_PRBS_GEN_INVf_SET(reg_prbs_gen_cfg, invert_data);
  PHYMOD_IF_ERR_RETURN(MODIFY_TLB_TX_PRBS_GEN_CFGr(pc, reg_prbs_gen_cfg));

  return PHYMOD_E_NONE;

} /* PRBS_MODE */
#endif

#ifdef _DV_TB_
int temod_prbs_tx_polynomial_set(PHYMOD_ST* pc, eagle_prbs_polynomial_type_t prbs_polynomial)                /* PRBS_MODE */
{
  TLB_TX_PRBS_GEN_CFGr_t    reg_prbs_gen_cfg;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TLB_TX_PRBS_GEN_CFGr_CLR(reg_prbs_gen_cfg);

  TLB_TX_PRBS_GEN_CFGr_PRBS_GEN_MODE_SELf_SET(reg_prbs_gen_cfg, prbs_polynomial);
  PHYMOD_IF_ERR_RETURN (MODIFY_TLB_TX_PRBS_GEN_CFGr(pc, reg_prbs_gen_cfg));

  return PHYMOD_E_NONE;

} /* PRBS_MODE */
#endif

#ifdef _DV_TB_
int temod_prbs_rx_polynomial_set(PHYMOD_ST* pc, eagle_prbs_polynomial_type_t prbs_polynomial)                /* PRBS_MODE */
{
  TLB_RX_PRBS_CHK_CFGr_t    reg_prbs_chk_cfg;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  TLB_RX_PRBS_CHK_CFGr_CLR(reg_prbs_chk_cfg);

  TLB_RX_PRBS_CHK_CFGr_PRBS_CHK_MODE_SELf_SET(reg_prbs_chk_cfg, prbs_polynomial);
  PHYMOD_IF_ERR_RETURN (MODIFY_TLB_RX_PRBS_CHK_CFGr(pc, reg_prbs_chk_cfg));

  return PHYMOD_E_NONE;

} /* PRBS_MODE */
#endif

/*!
@brief   Override the default PCS reset controls on PMD.
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   cntl  input (see details)
@param   value handle to current TSCE context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per Port PMD Override.
         Can be enhanced to also configure per core PMD Override
  Various bits are enabled as bit encoded by the cntl parameter
  \li 0x8000 ln_dp_h_rstb_oen
  \li 0x4000 tx_disable_oen
  \li 0x2000 rx_dme_en_oen
  \li 0x1000 osr_mode_oen
  \li 0x0800 lane_mode_oen
  \li 0x0400 rx_clk_vld_ovrd
  \li 0x0200 signal_detect_ovrd
  \li 0x0100 rx_lock_ovrd
*/
int temod_pmd_override_control(PHYMOD_ST* pc, int cntl, int value)           /* PMD_OVERRIDE_CONTROL */
{
  PMD_X4_OVRRr_t    reg_ovr;

  TEMOD_DBG_IN_FUNC_INFO(pc);
  PMD_X4_OVRRr_CLR(reg_ovr);

  if(cntl & 0x8000) {  /*  ln_dp_h_rstb_oen */
    PMD_X4_OVRRr_LN_DP_H_RSTB_OENf_SET(reg_ovr,(value & 0x0080));
  }
  if(cntl & 0x4000) {  /*  tx_disable_oen */
    PMD_X4_OVRRr_TX_DISABLE_OENf_SET(reg_ovr, (value & 0x0040));
  }
  if(cntl & 0x2000) {  /*  rx_dme_en_oen */
    PMD_X4_OVRRr_RX_DME_EN_OENf_SET (reg_ovr, (value & 0x0020));
  }
  if(cntl & 0x1000) {  /*  osr_mode_oen */
    PMD_X4_OVRRr_OSR_MODE_OENf_SET (reg_ovr, (value & 0x0010));
  }
  if(cntl & 0x0800) {  /*  lane_mode_oen */
    PMD_X4_OVRRr_LANE_MODE_OENf_SET (reg_ovr, (value & 0x0008));
  }
  if(cntl & 0x0400) {  /*  rx_clk_vld_ovrd */
    PMD_X4_OVRRr_RX_CLK_VLD_OVRDf_SET (reg_ovr, (value & 0x0004));
  }
  if(cntl & 0x0200) {  /*  signal_detect_ovrd */
    PMD_X4_OVRRr_SIGNAL_DETECT_OVRDf_SET (reg_ovr, (value & 0x0002));
  }
  if(cntl & 0x0100) {  /*  rx_lock_ovrd */
    PMD_X4_OVRRr_RX_LOCK_OVRDf_SET (reg_ovr, (value & 0x0001));
  }
  PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, reg_ovr));
  return PHYMOD_E_NONE;
}

/**
@brief   Gets the TX And RX Polarity 
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   tx_polarity Receive Polarity for TX side
@param   rx_polarity Receive Polarity for RX side
@returns The value PHYMOD_E_NONE upon successful completion.
@details Gets the TX And RX Polarity
*/
int temod_tx_rx_polarity_get ( PHYMOD_ST *pc, uint32_t* tx_polarity, uint32_t* rx_polarity) 
{
  TLB_TX_TLB_TX_MISC_CFGr_t tx_pol_inv;
  TLB_RX_TLB_RX_MISC_CFGr_t rx_pol_inv;

  PHYMOD_IF_ERR_RETURN(READ_TLB_TX_TLB_TX_MISC_CFGr(pc, &tx_pol_inv));
  *tx_polarity = TLB_TX_TLB_TX_MISC_CFGr_TX_PMD_DP_INVERTf_GET(tx_pol_inv);

  PHYMOD_IF_ERR_RETURN(READ_TLB_RX_TLB_RX_MISC_CFGr(pc, &rx_pol_inv));
  *rx_polarity = TLB_RX_TLB_RX_MISC_CFGr_RX_PMD_DP_INVERTf_GET(rx_pol_inv);
 
  return PHYMOD_E_NONE;
}

/**
@brief   Sets the TX And RX Polarity 
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   tx_polarity Control Polarity for TX side
@param   rx_polarity Control Polarity for RX side
@returns The value PHYMOD_E_NONE upon successful completion.
@details Sets the TX And RX Polarity
*/
int temod_tx_rx_polarity_set ( PHYMOD_ST *pc, uint32_t tx_polarity, uint32_t rx_polarity) 
{
  TLB_TX_TLB_TX_MISC_CFGr_t tx_pol_inv;
  TLB_RX_TLB_RX_MISC_CFGr_t rx_pol_inv;
  
  TLB_TX_TLB_TX_MISC_CFGr_CLR(tx_pol_inv);
  TLB_RX_TLB_RX_MISC_CFGr_CLR(rx_pol_inv);

  TLB_TX_TLB_TX_MISC_CFGr_TX_PMD_DP_INVERTf_SET(tx_pol_inv, tx_polarity);
  PHYMOD_IF_ERR_RETURN(MODIFY_TLB_TX_TLB_TX_MISC_CFGr(pc, tx_pol_inv));

  TLB_RX_TLB_RX_MISC_CFGr_RX_PMD_DP_INVERTf_SET(rx_pol_inv, rx_polarity);
  PHYMOD_IF_ERR_RETURN(MODIFY_TLB_RX_TLB_RX_MISC_CFGr(pc, rx_pol_inv));

  return PHYMOD_E_NONE;
}

/**
@brief   Controls port TX/RX squelch
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable is the control to  TX/RX  squelch. Enable=1 means enable the port,no squelch
@returns The value PHYMOD_E_NONE upon successful completion.
@details Controls port TX/RX squelch
*/
int temod_port_enable_set(PHYMOD_ST *pc, int enable)
{
  if (enable)  {
      temod_rx_squelch_set(pc, 0);
      temod_tx_squelch_set(pc, 0);
  } else {
      temod_rx_squelch_set(pc, 1);
      temod_tx_squelch_set(pc, 1);
  }

  return PHYMOD_E_NONE;
}

/**
@brief   Controls port TX squelch
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable is the control to  TX  squelch
@returns The value PHYMOD_E_NONE upon successful completion.
@details Controls port TX squelch
*/
int temod_tx_squelch_set(PHYMOD_ST *pc, int enable)
{
  TXFIR_MISC_CTL1r_t tx_misc_ctl;

  TXFIR_MISC_CTL1r_CLR(tx_misc_ctl);

  TXFIR_MISC_CTL1r_SDK_TX_DISABLEf_SET(tx_misc_ctl, enable);
  PHYMOD_IF_ERR_RETURN(MODIFY_TXFIR_MISC_CTL1r(pc, tx_misc_ctl));

  return PHYMOD_E_NONE;
}

/**
@brief   Controls port RX squelch
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   enable is the control to  RX  squelch
@returns The value PHYMOD_E_NONE upon successful completion.
@details Controls port RX squelch
*/
int temod_rx_squelch_set(PHYMOD_ST *pc, int enable)
{
  SIGDET_CTL1r_t sigdet_ctl;

  SIGDET_CTL1r_CLR(sigdet_ctl);

  if(enable){
     SIGDET_CTL1r_SIGNAL_DETECT_FRCf_SET(sigdet_ctl, 1);
     SIGDET_CTL1r_SIGNAL_DETECT_FRC_VALf_SET(sigdet_ctl, 0);
     temod_rx_lane_control_set(pc, 0);
  } else {
     SIGDET_CTL1r_SIGNAL_DETECT_FRCf_SET(sigdet_ctl, 0);
     SIGDET_CTL1r_SIGNAL_DETECT_FRC_VALf_SET(sigdet_ctl, 0);
     temod_rx_lane_control_set(pc, 1);
  }
  PHYMOD_IF_ERR_RETURN(MODIFY_SIGDET_CTL1r(pc, sigdet_ctl));

  return PHYMOD_E_NONE;
}

/**
@brief   Get port TX/RX squelch Settings
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   tx_enable - get the TX  squelch settings
@param   rx_enable - get the RX  squelch settings
@returns The value PHYMOD_E_NONE upon successful completion.
@details Controls port TX/RX squelch
*/
int temod_port_enable_get(PHYMOD_ST *pc, int *tx_enable, int *rx_enable)
{

  temod_rx_squelch_get(pc, rx_enable);
  temod_tx_squelch_get(pc, tx_enable);

  return PHYMOD_E_NONE;
}

/**
@brief   Get port TX squelch control settings
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   val Receiver for status of TX squelch 
@returns The value PHYMOD_E_NONE upon successful completion.
@details Get port TX squelch control settings
*/
int temod_tx_squelch_get(PHYMOD_ST *pc, int *val)
{
  TXFIR_MISC_CTL1r_t tx_misc_ctl;

  TXFIR_MISC_CTL1r_CLR(tx_misc_ctl);

  PHYMOD_IF_ERR_RETURN(READ_TXFIR_MISC_CTL1r(pc, &tx_misc_ctl));
  *val = TXFIR_MISC_CTL1r_SDK_TX_DISABLEf_GET(tx_misc_ctl);

  return PHYMOD_E_NONE;
}

/**
@brief   Gets port RX squelch settings
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   val Receiver for status of  RX  squelch
@returns The value PHYMOD_E_NONE upon successful completion.
@details Gets port RX squelch settings
*/
int temod_rx_squelch_get(PHYMOD_ST *pc, int *val)
{
  SIGDET_CTL1r_t sigdet_ctl;

  SIGDET_CTL1r_CLR(sigdet_ctl);

  PHYMOD_IF_ERR_RETURN(READ_SIGDET_CTL1r(pc, &sigdet_ctl));
  *val = SIGDET_CTL1r_SIGNAL_DETECT_FRCf_GET(sigdet_ctl);

  return PHYMOD_E_NONE;
}



/**
@brief   Sets the field of the Soft Table
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   st_entry_no: The ST to write to(0..3)
@param   st_field: The ST field to write to
@param   st_value: The ST value to write to the field
returns  The value PHYMOD_E_NONE upon successful completion.
@details Sets the field of the Soft Table
*/
int temod_st_control_field_set (PHYMOD_ST* pc,uint16_t st_entry_no, override_type_t  st_control_field, uint16_t st_field_value){

  SC_X1_SPD_OVRR0_SPDr_t    reg0_ovr_spd;
  SC_X1_SPD_OVRR0_0r_t        reg0_ovr;
  SC_X1_SPD_OVRR0_1r_t        reg0_ovr1;
  SC_X1_SPD_OVRR0_2r_t        reg0_ovr2;

  SC_X1_SPD_OVRR1_SPDr_t    reg1_ovr_spd;
  SC_X1_SPD_OVRR1_0r_t        reg1_ovr;
  SC_X1_SPD_OVRR1_1r_t        reg1_ovr1;
  SC_X1_SPD_OVRR1_2r_t        reg1_ovr2;

  SC_X1_SPD_OVRR2_SPDr_t    reg2_ovr_spd;
  SC_X1_SPD_OVRR2_0r_t        reg2_ovr;
  SC_X1_SPD_OVRR2_1r_t        reg2_ovr1;
  SC_X1_SPD_OVRR2_2r_t        reg2_ovr2;

  SC_X1_SPD_OVRR3_SPDr_t    reg3_ovr_spd;
  SC_X1_SPD_OVRR3_0r_t        reg3_ovr;
  SC_X1_SPD_OVRR3_1r_t        reg3_ovr1;
  SC_X1_SPD_OVRR3_2r_t        reg3_ovr2;

  SC_X1_SPD_OVRR0_SPDr_CLR(reg0_ovr_spd);
  SC_X1_SPD_OVRR0_0r_CLR(reg0_ovr);
  SC_X1_SPD_OVRR0_1r_CLR(reg0_ovr1);
  SC_X1_SPD_OVRR0_2r_CLR(reg0_ovr2);

  SC_X1_SPD_OVRR1_SPDr_CLR(reg1_ovr_spd);
  SC_X1_SPD_OVRR1_0r_CLR(reg1_ovr);
  SC_X1_SPD_OVRR1_1r_CLR(reg1_ovr1);
  SC_X1_SPD_OVRR1_2r_CLR(reg1_ovr2);

  SC_X1_SPD_OVRR2_SPDr_CLR(reg2_ovr_spd);
  SC_X1_SPD_OVRR2_0r_CLR(reg2_ovr);
  SC_X1_SPD_OVRR2_1r_CLR(reg2_ovr1);
  SC_X1_SPD_OVRR2_2r_CLR(reg2_ovr2);

  SC_X1_SPD_OVRR3_SPDr_CLR(reg3_ovr_spd);
  SC_X1_SPD_OVRR3_0r_CLR(reg3_ovr);
  SC_X1_SPD_OVRR3_1r_CLR(reg3_ovr1);
  SC_X1_SPD_OVRR3_2r_CLR(reg3_ovr2);

  switch(st_control_field) {
  case TEMOD_OVERRIDE_RESET :
       /* CLear the table entry */
  break;
  case TEMOD_OVERRIDE_ALL :
       /* NA */
  break;
  case TEMOD_OVERRIDE_SPDID :
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_SPDr_SPEEDf_SET(reg0_ovr_spd, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_SPDr(pc, reg0_ovr_spd));
              break;
      case 1: SC_X1_SPD_OVRR1_SPDr_SPEEDf_SET(reg1_ovr_spd, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_SPDr(pc, reg1_ovr_spd));
              break;
      case 2: SC_X1_SPD_OVRR2_SPDr_SPEEDf_SET(reg2_ovr_spd, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_SPDr(pc, reg2_ovr_spd));
              break;
      case 3: SC_X1_SPD_OVRR3_SPDr_SPEEDf_SET(reg3_ovr_spd, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_SPDr(pc, reg3_ovr_spd));
              break;
    }
  break;
  case TEMOD_OVERRIDE_CL72 :
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_0r_CL72_ENABLEf_SET(reg0_ovr, st_field_value); 
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, reg0_ovr));
              break;
      case 1: SC_X1_SPD_OVRR1_0r_CL72_ENABLEf_SET(reg1_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, reg1_ovr));
              break;
      case 2: SC_X1_SPD_OVRR2_0r_CL72_ENABLEf_SET(reg2_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, reg2_ovr));
              break;
      case 3: SC_X1_SPD_OVRR3_0r_CL72_ENABLEf_SET(reg3_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, reg3_ovr));
              break;
    }
  break;
  case TEMOD_OVERRIDE_NUM_LANES :
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_SPDr_NUM_LANESf_SET(reg0_ovr_spd, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_SPDr(pc, reg0_ovr_spd));
              break;
      case 1: SC_X1_SPD_OVRR1_SPDr_NUM_LANESf_SET(reg1_ovr_spd, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_SPDr(pc, reg1_ovr_spd));
              break;
      case 2: SC_X1_SPD_OVRR2_SPDr_NUM_LANESf_SET(reg2_ovr_spd, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_SPDr(pc, reg2_ovr_spd));
              break;
      case 3: SC_X1_SPD_OVRR3_SPDr_NUM_LANESf_SET(reg3_ovr_spd, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_SPDr(pc, reg3_ovr_spd));
              break;
    }
  break;
  case TEMOD_OVERRIDE_OS_MODE:
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_0r_OS_MODEf_SET(reg0_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, reg0_ovr));
              break;
      case 1: SC_X1_SPD_OVRR1_0r_OS_MODEf_SET(reg1_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, reg1_ovr));
              break;
      case 2: SC_X1_SPD_OVRR2_0r_OS_MODEf_SET(reg2_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, reg2_ovr));
              break;
      case 3: SC_X1_SPD_OVRR3_0r_OS_MODEf_SET(reg3_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, reg3_ovr));
              break;
    }
  break;
  case TEMOD_OVERRIDE_FEC_EN :
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_0r_FEC_ENABLEf_SET(reg0_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, reg0_ovr));
              break;
      case 1: SC_X1_SPD_OVRR1_0r_FEC_ENABLEf_SET(reg1_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, reg1_ovr));
              break;
      case 2: SC_X1_SPD_OVRR2_0r_FEC_ENABLEf_SET(reg2_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, reg2_ovr));
              break;
      case 3: SC_X1_SPD_OVRR3_0r_FEC_ENABLEf_SET(reg3_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, reg3_ovr));
              break;
    }
  break;
  case TEMOD_OVERRIDE_DESKEW_MODE:
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_1r_DESKEWMODEf_SET(reg0_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, reg0_ovr1));
              break;
      case 1: SC_X1_SPD_OVRR1_1r_DESKEWMODEf_SET(reg1_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, reg1_ovr1));
              break;
      case 2: SC_X1_SPD_OVRR2_1r_DESKEWMODEf_SET(reg2_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, reg2_ovr1));
              break;
      case 3: SC_X1_SPD_OVRR3_1r_DESKEWMODEf_SET(reg3_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, reg3_ovr1));
              break;
    }
  break;
  case TEMOD_OVERRIDE_DESC2_MODE :
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_1r_DESC2_MODEf_SET(reg0_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, reg0_ovr1));
              break;
      case 1: SC_X1_SPD_OVRR1_1r_DESC2_MODEf_SET(reg1_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, reg1_ovr1));
              break;
      case 2: SC_X1_SPD_OVRR2_1r_DESC2_MODEf_SET(reg2_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, reg2_ovr1));
              break;
      case 3: SC_X1_SPD_OVRR3_1r_DESC2_MODEf_SET(reg3_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, reg3_ovr1));
              break;
    }
  break;
  case TEMOD_OVERRIDE_CL36BYTEDEL_MODE:
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_1r_CL36BYTEDELETEMODEf_SET(reg0_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, reg0_ovr1));
              break;
      case 1: SC_X1_SPD_OVRR1_1r_CL36BYTEDELETEMODEf_SET(reg1_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, reg1_ovr1));
              break;
      case 2: SC_X1_SPD_OVRR2_1r_CL36BYTEDELETEMODEf_SET(reg2_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, reg2_ovr1));
              break;
      case 3: SC_X1_SPD_OVRR3_1r_CL36BYTEDELETEMODEf_SET(reg3_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, reg3_ovr1));
              break;
    }
  break;
  case TEMOD_OVERRIDE_BRCM64B66_DESCR_MODE:
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_1r_BRCM64B66_DESCRAMBLER_ENABLEf_SET(reg0_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, reg0_ovr1));
              break;
      case 1: SC_X1_SPD_OVRR1_1r_BRCM64B66_DESCRAMBLER_ENABLEf_SET(reg1_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, reg1_ovr1));
              break;
      case 2: SC_X1_SPD_OVRR2_1r_BRCM64B66_DESCRAMBLER_ENABLEf_SET(reg2_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, reg2_ovr1));
              break;
      case 3: SC_X1_SPD_OVRR3_1r_BRCM64B66_DESCRAMBLER_ENABLEf_SET(reg3_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, reg3_ovr1));
              break;
    }
  break;
  case TEMOD_OVERRIDE_CHKEND_EN:
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_2r_CHK_END_ENf_SET(reg0_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_2r(pc, reg0_ovr2));
              break;
      case 1: SC_X1_SPD_OVRR1_2r_CHK_END_ENf_SET(reg1_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_2r(pc, reg1_ovr2));
              break;
      case 2: SC_X1_SPD_OVRR2_2r_CHK_END_ENf_SET(reg2_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_2r(pc, reg2_ovr2));
              break;
      case 3: SC_X1_SPD_OVRR3_2r_CHK_END_ENf_SET(reg3_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_2r(pc, reg3_ovr2));
              break;
    }
  break;
  case TEMOD_OVERRIDE_BLKSYNC_MODE:
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_2r_BLOCK_SYNC_MODEf_SET(reg0_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_2r(pc, reg0_ovr2));
              break;
      case 1: SC_X1_SPD_OVRR1_2r_BLOCK_SYNC_MODEf_SET(reg1_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_2r(pc, reg1_ovr2));
              break;
      case 2: SC_X1_SPD_OVRR2_2r_BLOCK_SYNC_MODEf_SET(reg2_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_2r(pc, reg2_ovr2));
              break;
      case 3: SC_X1_SPD_OVRR3_2r_BLOCK_SYNC_MODEf_SET(reg3_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_2r(pc, reg3_ovr2));
              break;
    }
  break;
  case TEMOD_OVERRIDE_REORDER_EN :
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_2r_REORDER_ENf_SET(reg0_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_2r(pc, reg0_ovr2));
              break;
      case 1: SC_X1_SPD_OVRR1_2r_REORDER_ENf_SET(reg1_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_2r(pc, reg1_ovr2));
              break;
      case 2: SC_X1_SPD_OVRR2_2r_REORDER_ENf_SET(reg2_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_2r(pc, reg2_ovr2));
              break;
      case 3: SC_X1_SPD_OVRR3_2r_REORDER_ENf_SET(reg3_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_2r(pc, reg3_ovr2));
              break;
    }
  break;
  case TEMOD_OVERRIDE_CL36_EN:
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_2r_CL36_ENf_SET(reg0_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_2r(pc, reg0_ovr2));
              break;
      case 1: SC_X1_SPD_OVRR1_2r_CL36_ENf_SET(reg1_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_2r(pc, reg1_ovr2));
              break;
      case 2: SC_X1_SPD_OVRR2_2r_CL36_ENf_SET(reg2_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_2r(pc, reg2_ovr2));
              break;
      case 3: SC_X1_SPD_OVRR3_2r_CL36_ENf_SET(reg3_ovr2, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_2r(pc, reg3_ovr2));
              break;
    }

  break;
  case TEMOD_OVERRIDE_SCR_MODE:
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_0r_SCR_MODEf_SET(reg0_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, reg0_ovr));
              break;
      case 1: SC_X1_SPD_OVRR1_0r_SCR_MODEf_SET(reg1_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, reg1_ovr));
              break;
      case 2: SC_X1_SPD_OVRR2_0r_SCR_MODEf_SET(reg2_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, reg2_ovr));
              break;
      case 3: SC_X1_SPD_OVRR3_0r_SCR_MODEf_SET(reg3_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, reg3_ovr));
              break;
    }
  break;
  case TEMOD_OVERRIDE_DESCR_MODE:
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_1r_DESCRAMBLERMODEf_SET(reg0_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, reg0_ovr1));
              break;
      case 1: SC_X1_SPD_OVRR1_1r_DESCRAMBLERMODEf_SET(reg1_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, reg1_ovr1));
              break;
      case 2: SC_X1_SPD_OVRR2_1r_DESCRAMBLERMODEf_SET(reg2_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, reg2_ovr1));
              break;
      case 3: SC_X1_SPD_OVRR3_1r_DESCRAMBLERMODEf_SET(reg3_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, reg3_ovr1));
              break;
    }
  break;
  case TEMOD_OVERRIDE_ENCODE_MODE:
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_0r_ENCODEMODEf_SET(reg0_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_0r(pc, reg0_ovr));
              break;
      case 1: SC_X1_SPD_OVRR1_0r_ENCODEMODEf_SET(reg1_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_0r(pc, reg1_ovr));
              break;
      case 2: SC_X1_SPD_OVRR2_0r_ENCODEMODEf_SET(reg2_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_0r(pc, reg2_ovr));
              break;
      case 3: SC_X1_SPD_OVRR3_0r_ENCODEMODEf_SET(reg3_ovr, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_0r(pc, reg3_ovr));
              break;
    }

  break;
  case TEMOD_OVERRIDE_DECODE_MODE:
    switch(st_entry_no){
      case 0: SC_X1_SPD_OVRR0_1r_DECODERMODEf_SET(reg0_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_1r(pc, reg0_ovr1));
              break;
      case 1: SC_X1_SPD_OVRR1_1r_DECODERMODEf_SET(reg1_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_1r(pc, reg1_ovr1));
              break;
      case 2: SC_X1_SPD_OVRR2_1r_DECODERMODEf_SET(reg2_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_1r(pc, reg2_ovr1));
              break;
      case 3: SC_X1_SPD_OVRR3_1r_DECODERMODEf_SET(reg3_ovr1, st_field_value);
              PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_1r(pc, reg3_ovr1));
              break;
    }
    break;
}

return PHYMOD_E_NONE;
  
}

/**
@brief   Sets the credit field of the Soft Table
@param   pc handle to current TSCE context (#PHYMOD_ST)
@param   st_entry_no: The ST to write to(0..3)
@param   credit_type: The ST field to write to
@param   st_value: The ST value to write to the field
returns  The value PHYMOD_E_NONE upon successful completion.
@details Sets the field of the Soft Table
*/
int temod_st_credit_field_set (PHYMOD_ST* pc,uint16_t st_entry_no,credit_type_t  credit_type,uint16_t st_field_value){

  SC_X1_SPD_OVRR0_3r_t        reg0_ovr3;
  SC_X1_SPD_OVRR0_4r_t        reg0_ovr4;
  SC_X1_SPD_OVRR0_5r_t        reg0_ovr5;
  SC_X1_SPD_OVRR0_6r_t        reg0_ovr6;
  SC_X1_SPD_OVRR0_7r_t        reg0_ovr7;
  SC_X1_SPD_OVRR0_8r_t        reg0_ovr8;

  SC_X1_SPD_OVRR1_3r_t        reg1_ovr3;
  SC_X1_SPD_OVRR1_4r_t        reg1_ovr4;
  SC_X1_SPD_OVRR1_5r_t        reg1_ovr5;
  SC_X1_SPD_OVRR1_6r_t        reg1_ovr6;
  SC_X1_SPD_OVRR1_7r_t        reg1_ovr7;
  SC_X1_SPD_OVRR1_8r_t        reg1_ovr8;

  SC_X1_SPD_OVRR2_3r_t        reg2_ovr3;
  SC_X1_SPD_OVRR2_4r_t        reg2_ovr4;
  SC_X1_SPD_OVRR2_5r_t        reg2_ovr5;
  SC_X1_SPD_OVRR2_6r_t        reg2_ovr6;
  SC_X1_SPD_OVRR2_7r_t        reg2_ovr7;
  SC_X1_SPD_OVRR2_8r_t        reg2_ovr8;

  SC_X1_SPD_OVRR3_3r_t        reg3_ovr3;
  SC_X1_SPD_OVRR3_4r_t        reg3_ovr4;
  SC_X1_SPD_OVRR3_5r_t        reg3_ovr5;
  SC_X1_SPD_OVRR3_6r_t        reg3_ovr6;
  SC_X1_SPD_OVRR3_7r_t        reg3_ovr7;
  SC_X1_SPD_OVRR3_8r_t        reg3_ovr8;

  SC_X1_SPD_OVRR0_3r_CLR(reg0_ovr3);
  SC_X1_SPD_OVRR0_4r_CLR(reg0_ovr4);
  SC_X1_SPD_OVRR0_5r_CLR(reg0_ovr5);
  SC_X1_SPD_OVRR0_6r_CLR(reg0_ovr6);
  SC_X1_SPD_OVRR0_7r_CLR(reg0_ovr7);
  SC_X1_SPD_OVRR0_8r_CLR(reg0_ovr8);

  SC_X1_SPD_OVRR1_3r_CLR(reg1_ovr3);
  SC_X1_SPD_OVRR1_4r_CLR(reg1_ovr4);
  SC_X1_SPD_OVRR1_5r_CLR(reg1_ovr5);
  SC_X1_SPD_OVRR1_6r_CLR(reg1_ovr6);
  SC_X1_SPD_OVRR1_7r_CLR(reg1_ovr7);
  SC_X1_SPD_OVRR1_8r_CLR(reg1_ovr8);

  SC_X1_SPD_OVRR2_3r_CLR(reg2_ovr3);
  SC_X1_SPD_OVRR2_4r_CLR(reg2_ovr4);
  SC_X1_SPD_OVRR2_5r_CLR(reg2_ovr5);
  SC_X1_SPD_OVRR2_6r_CLR(reg2_ovr6);
  SC_X1_SPD_OVRR2_7r_CLR(reg2_ovr7);
  SC_X1_SPD_OVRR2_8r_CLR(reg2_ovr8);

  SC_X1_SPD_OVRR3_3r_CLR(reg3_ovr3);
  SC_X1_SPD_OVRR3_4r_CLR(reg3_ovr4);
  SC_X1_SPD_OVRR3_5r_CLR(reg3_ovr5);
  SC_X1_SPD_OVRR3_6r_CLR(reg3_ovr6);
  SC_X1_SPD_OVRR3_7r_CLR(reg3_ovr7);
  SC_X1_SPD_OVRR3_8r_CLR(reg3_ovr8);

  switch ( credit_type ) {
    /* case TEMOD_CREDIT_RESET:
     //reset credits 
      break;
    */
    /*case TEMOD_CREDIT_TABLE:
    // get info. from table and apply it to all credit regs. */
    case  TEMOD_CREDIT_CLOCK_COUNT_0:
      switch(st_entry_no){
        case 0: SC_X1_SPD_OVRR0_3r_CLOCKCNT0f_SET(reg0_ovr3, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_3r(pc, reg0_ovr3));
                break;
        case 1: SC_X1_SPD_OVRR1_3r_CLOCKCNT0f_SET(reg1_ovr3, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_3r(pc, reg1_ovr3));
                break;
        case 2: SC_X1_SPD_OVRR2_3r_CLOCKCNT0f_SET(reg2_ovr3, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_3r(pc, reg2_ovr3));
                break;
        case 3: SC_X1_SPD_OVRR3_3r_CLOCKCNT0f_SET(reg3_ovr3, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_3r(pc, reg3_ovr3));
                break;
      }
    break;
    case  TEMOD_CREDIT_CLOCK_COUNT_1: 
      switch(st_entry_no){
        case 0: SC_X1_SPD_OVRR0_4r_CLOCKCNT1f_SET(reg0_ovr4, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_4r(pc, reg0_ovr4));
                break;
        case 1: SC_X1_SPD_OVRR1_4r_CLOCKCNT1f_SET(reg1_ovr4, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_4r(pc, reg1_ovr4));
                break;
        case 2: SC_X1_SPD_OVRR2_4r_CLOCKCNT1f_SET(reg2_ovr4, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_4r(pc, reg2_ovr4));
                break;
        case 3: SC_X1_SPD_OVRR3_4r_CLOCKCNT1f_SET(reg3_ovr4, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_4r(pc, reg3_ovr4));
                break;
      }
    break;
    case  TEMOD_CREDIT_LOOP_COUNT_0:
      switch(st_entry_no){
        case 0: SC_X1_SPD_OVRR0_5r_LOOPCNT0f_SET(reg0_ovr5, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_5r(pc, reg0_ovr5));
                break;
        case 1: SC_X1_SPD_OVRR1_5r_LOOPCNT0f_SET(reg1_ovr5, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_5r(pc, reg1_ovr5));
                break;
        case 2: SC_X1_SPD_OVRR2_5r_LOOPCNT0f_SET(reg2_ovr5, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_5r(pc, reg2_ovr5));
                break;
        case 3: SC_X1_SPD_OVRR3_5r_LOOPCNT0f_SET(reg3_ovr5, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_5r(pc, reg3_ovr5));
                break;
        }
    break;
    case TEMOD_CREDIT_LOOP_COUNT_1:   
      switch(st_entry_no){
        case 0: SC_X1_SPD_OVRR0_5r_LOOPCNT1f_SET(reg0_ovr5, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_5r(pc, reg0_ovr5));
                break;
        case 1: SC_X1_SPD_OVRR1_5r_LOOPCNT1f_SET(reg1_ovr5, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_5r(pc, reg1_ovr5));
                break;
        case 2: SC_X1_SPD_OVRR2_5r_LOOPCNT1f_SET(reg2_ovr5, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_5r(pc, reg2_ovr5));
                break;
        case 3: SC_X1_SPD_OVRR3_5r_LOOPCNT1f_SET(reg3_ovr5, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_5r(pc, reg3_ovr5));
                break;
        }
    break;
    case TEMOD_CREDIT_MAC:   
      switch(st_entry_no){
        case 0: SC_X1_SPD_OVRR0_6r_MAC_CREDITGENCNTf_SET(reg0_ovr6, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_6r(pc, reg0_ovr6));
                break;
        case 1: SC_X1_SPD_OVRR1_6r_MAC_CREDITGENCNTf_SET(reg1_ovr6, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_6r(pc, reg1_ovr6));
                break;
        case 2: SC_X1_SPD_OVRR2_6r_MAC_CREDITGENCNTf_SET(reg2_ovr6, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_6r(pc, reg2_ovr6));
                break;
        case 3: SC_X1_SPD_OVRR3_6r_MAC_CREDITGENCNTf_SET(reg3_ovr6, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_6r(pc, reg3_ovr6));
                break;
      }
    break;
    case  TEMOD_CREDIT_PCS_CLOCK_COUNT_0:
      switch(st_entry_no){
        case 0: SC_X1_SPD_OVRR0_7r_PCS_CLOCKCNT0f_SET(reg0_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_7r(pc, reg0_ovr7));
                break;
        case 1: SC_X1_SPD_OVRR1_7r_PCS_CLOCKCNT0f_SET(reg1_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_7r(pc, reg1_ovr7));
                break;
        case 2: SC_X1_SPD_OVRR2_7r_PCS_CLOCKCNT0f_SET(reg2_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_7r(pc, reg2_ovr7));
                break;
        case 3: SC_X1_SPD_OVRR3_7r_PCS_CLOCKCNT0f_SET(reg3_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_7r(pc, reg3_ovr7));
                break;
      }
    break;
    case TEMOD_CREDIT_PCS_GEN_COUNT:   
      switch(st_entry_no){
        case 0: SC_X1_SPD_OVRR0_8r_PCS_CREDITGENCNTf_SET(reg0_ovr8, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_8r(pc, reg0_ovr8));
                break;
        case 1: SC_X1_SPD_OVRR1_8r_PCS_CREDITGENCNTf_SET(reg1_ovr8, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_8r(pc, reg1_ovr8));
                break;
        case 2: SC_X1_SPD_OVRR2_8r_PCS_CREDITGENCNTf_SET(reg2_ovr8, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_8r(pc, reg2_ovr8));
                break;
        case 3: SC_X1_SPD_OVRR3_8r_PCS_CREDITGENCNTf_SET(reg3_ovr8, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_8r(pc, reg3_ovr8));
                break;
      }
    break;
    case TEMOD_CREDIT_EN: 
      switch(st_entry_no){
        case 0: SC_X1_SPD_OVRR0_7r_REPLICATION_CNTf_SET(reg0_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_7r(pc, reg0_ovr7));
                break;
        case 1: SC_X1_SPD_OVRR1_7r_REPLICATION_CNTf_SET(reg1_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_7r(pc, reg1_ovr7));
                break;
        case 2: SC_X1_SPD_OVRR2_7r_REPLICATION_CNTf_SET(reg2_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_7r(pc, reg2_ovr7));
                break;
        case 3: SC_X1_SPD_OVRR3_7r_REPLICATION_CNTf_SET(reg3_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_7r(pc, reg3_ovr7));
                break;
      }
    break;
    case  TEMOD_CREDIT_PCS_REPCNT:  
      switch(st_entry_no){
        case 0: SC_X1_SPD_OVRR0_7r_REPLICATION_CNTf_SET(reg0_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_7r(pc, reg0_ovr7));
                break;
        case 1: SC_X1_SPD_OVRR1_7r_REPLICATION_CNTf_SET(reg1_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_7r(pc, reg1_ovr7));
                break;
        case 2: SC_X1_SPD_OVRR2_7r_REPLICATION_CNTf_SET(reg2_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_7r(pc, reg2_ovr7));
                break;
        case 3: SC_X1_SPD_OVRR3_7r_REPLICATION_CNTf_SET(reg3_ovr7, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_7r(pc, reg3_ovr7));
                break;
      }
    break;
    case  TEMOD_CREDIT_SGMII_SPD:
      switch(st_entry_no){
        case 0: SC_X1_SPD_OVRR0_3r_SGMII_SPD_SWITCHf_SET(reg0_ovr3, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR0_3r(pc, reg0_ovr3));
                break;
        case 1: SC_X1_SPD_OVRR1_3r_SGMII_SPD_SWITCHf_SET(reg1_ovr3, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR1_3r(pc, reg1_ovr3));
                break;
        case 2: SC_X1_SPD_OVRR2_3r_SGMII_SPD_SWITCHf_SET(reg2_ovr3, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR2_3r(pc, reg2_ovr3));
                break;
        case 3: SC_X1_SPD_OVRR3_3r_SGMII_SPD_SWITCHf_SET(reg3_ovr3, st_field_value);
                PHYMOD_IF_ERR_RETURN (MODIFY_SC_X1_SPD_OVRR3_3r(pc, reg3_ovr3));
                break;
      }
    break;
    default:
      return PHYMOD_E_FAIL;
    break;
  }
  return PHYMOD_E_NONE;
} 

#ifdef _SDK_TEMOD_
int temod_pll_sequencer_control(PHYMOD_ST *pa, int enable) { return 0; }
/*
int temod_pll_lock_wait ( PHYMOD_ST *pc, int timeOutValue) { return 0; } 
int temod_tx_pi_control_get ( PHYMOD_ST *pa,  uint32_t *value) { return 0; }
int temod_get_ht_entries(PHYMOD_ST *pa) { return 0; } 
*/
#endif
