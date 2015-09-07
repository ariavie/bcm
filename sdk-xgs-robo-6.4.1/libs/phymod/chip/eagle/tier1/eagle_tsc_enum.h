/******************************************************************************/
/******************************************************************************/
/*  Revision      :  $Id: eagle_tsc_enum.h 406 2014-03-28 19:09:25Z kirand $ */
/*                                                                            */
/*  Description   :  Enum types used by Serdes API functions                  */
/*                                                                            */
/*  Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.             */
/*  All Rights Reserved                                                       */
/*  No portions of this material may be reproduced in any form without        */
/*  the written permission of:                                                */
/*      Broadcom Corporation                                                  */
/*      5300 California Avenue                                                */
/*      Irvine, CA  92617                                                     */
/*                                                                            */
/*  All information contained in this document is Broadcom Corporation        */
/*  company private proprietary, and trade secret.                            */
/*                                                                            */
/******************************************************************************/
/******************************************************************************/
/*
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
 *  $Id: f734a57d68de5904ef855c70517d948d310b9daf $
*/


/** @file eagle_tsc_enum.h
 * Enum types used by Serdes API functions
 */

#ifndef EAGLE_TSC_API_ENUM_H
#define EAGLE_TSC_API_ENUM_H



/** Eagle PLL Config Enum */ 
enum eagle_tsc_pll_enum {
  pll_div_40x,
  pll_div_42x,
  pll_div_46x,
  pll_div_50x,
  pll_div_52x,
  pll_div_54x,
  pll_div_60x,
  pll_div_64x,
  pll_div_66x,
  pll_div_68x,
  pll_div_70x,
  pll_div_72x,
  pll_div_73p6x,
  pll_div_80x,
  pll_div_82p5x,
  pll_div_87p5x,
  pll_div_92x,
  pll_div_100x,
  pll_div_199p04x
};






/** PRBS Polynomial Enum */ 
enum eagle_tsc_prbs_polynomial_enum {
  PRBS_7  = 0,
  PRBS_9  = 1,
  PRBS_11 = 2,
  PRBS_15 = 3,
  PRBS_23 = 4,
  PRBS_31 = 5,
  PRBS_58 = 6
};


/** PRBS Checker Mode Enum */
enum eagle_tsc_prbs_checker_mode_enum {
  PRBS_SELF_SYNC_HYSTERESIS       = 0,
  PRBS_INITIAL_SEED_HYSTERESIS    = 1,
  PRBS_INITIAL_SEED_NO_HYSTERESIS = 2
};


/** TX_PI Fixed Frequency and Jitter Generation Modes Enum */
enum eagle_tsc_tx_pi_freq_jit_gen_enum {
  TX_PI_FIXED_FREQ      = 0,
  TX_PI_SSC_LOW_FREQ    = 1,
  TX_PI_SSC_HIGH_FREQ   = 2,
  TX_PI_SJ              = 3
} ;


enum eagle_tsc_rx_afe_settings_enum {
	RX_AFE_PF,
	RX_AFE_PF2,
	RX_AFE_VGA,
	RX_AFE_DFE1,
	RX_AFE_DFE2,
	RX_AFE_DFE3,
	RX_AFE_DFE4,
	RX_AFE_DFE5
};

enum eagle_tsc_tx_afe_settings_enum {
	TX_AFE_PRE,
	TX_AFE_MAIN,
	TX_AFE_POST1,
	TX_AFE_POST2,
	TX_AFE_POST3,
	TX_AFE_AMP,
    TX_AFE_DRIVERMODE
};

enum eagle_tsc_afe_drivermode_enum {
    DM_DEFAULT = 0,
    DM_NOT_SUPPORTED,
    DM_HALF_AMPLITUDE,
    DM_HALF_AMPLITUDE_HI_IMPED
};

enum core_pwrdn_mode_enum {
	PWR_ON,
	PWRDN,
	PWRDN_DEEP,
	PWRDN_TX,
	PWRDN_RX
};


#endif
