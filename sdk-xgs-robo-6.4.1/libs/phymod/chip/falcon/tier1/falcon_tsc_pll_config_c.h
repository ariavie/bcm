/************************************************************************************/
/************************************************************************************/
/*  File Name     :  falcon_tsc_pll_config.c                                        */
/*  Created On    :  23/12/2013                                                     */
/*  Created By    :  Kiran Divakar                                                  */
/*  Description   :  Falcon TSC PLL Configuration API                               */
/*  Revision      :  $Id: falcon_tsc_pll_config.c 406 2014-03-28 19:09:25Z kirand $ */
/*                                                                                  */
/*  Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.                   */
/*  All Rights Reserved                                                             */
/*  No portions of this material may be reproduced in any form without              */
/*  the written permission of:                                                      */
/*      Broadcom Corporation                                                        */
/*      5300 California Avenue                                                      */
/*      Irvine, CA  92617                                                           */
/*                                                                                  */
/*  All information contained in this document is Broadcom Corporation              */
/*  company private proprietary, and trade secret.                                  */
/*                                                                                  */
/************************************************************************************/
/************************************************************************************/
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
 */


/** @file falcon_tsc_pll_config.c
 * Falcon TSC PLL Configuration
 */

#include "falcon_tsc_enum.h"


err_code_t falcon_tsc_configure_pll( const phymod_access_t *pa, enum falcon_tsc_pll_enum pll_cfg) {

  /* Restore defaults, needed for non-register reset cases */
    /* wrc_pll_mode(0x7); */
  
  switch (pll_cfg) {

    /******************/
    /*  Integer Mode  */
    /******************/

    case pll_div_128x :
      /* pll_mode<3:0> = 0011, VCO = 20.625G, Refclk = 106.133MHz */
      wrc_pll_mode(0x3);
      break;

    case pll_div_132x :
      /* pll_mode<3:0> = 0100 , VCO = 20.625G, Refclk = 156.25MHz */
      /* pll_mode<3:0> = 0100 , VCO = 28.055G, Refclk = 212.5MHz  */
      wrc_pll_mode(0x4);
      break;

    case pll_div_140x :
      /* pll_mode<3:0> = 0101 , VCO = 21.875G, Refclk = 156.25MHz */
      wrc_pll_mode(0x5);
      break;

    case pll_div_160x :
      /* pll_mode<3:0> = 0110 , VCO = 25.0G, Refclk = 156.25MHz    */ 
      /* pll_mode<3:0> = 0110 , VCO = 25.78125G, Refclk = 161.3MHz */
      wrc_pll_mode(0x6);
      break;

    case pll_div_165x : 
      /* pll_mode<3:0> = 0111, VCO = 20.625G, Refclk = 125MHz      */
      /* pll_mode<3:0> = 0111, VCO = 25.78125G, Refclk = 156.25MHz */
      wrc_pll_mode(0x7);
      break;

    case pll_div_168x :
      /* pll_mode<3:0> = 1000, VCO = 26.25G, Refclk = 156.25MHz */
      wrc_pll_mode(0x8);
      break;

    case pll_div_175x :
      /* pll_mode<3:0> = 1010, VCO = 27.34375G, Refclk = 156.25MHz */
      wrc_pll_mode(0xA);
      break;

    case pll_div_180x :
      /* pll_mode<3:0> = 1011 , VCO = 22.5G, Refclk = 125.0MHz    */
      /* pll_mode<3:0> = 1011 , VCO = 28.125G, Refclk = 156.25MHz */
      wrc_pll_mode(0xB);
      break;

    case pll_div_184x : 
      /* pll_mode<3:0> = 1100 , VCO = 23.0G, Refclk = 125.0MHz */
      wrc_pll_mode(0xC);
      break;

    case pll_div_200x :
      /* pll_mode<3:0> = 1101, VCO = 25.0G, Refclk = 125.0MHz */
      wrc_pll_mode(0xD);
      break;

    case pll_div_224x :
      /* pll_mode<3:0> = 1110 , VCO = 28.0G, Refclk = 125.0MHz */
      wrc_pll_mode(0xE);
      break;

    case pll_div_264x :
      /* pll_mode<3:0> = 1111 , VCO = 28.05G, Refclk = 106.25MHz */
      wrc_pll_mode(0xF);
      break;

    default:                     /* Invalid pll_cfg value  */
      return (_error(ERR_CODE_INVALID_PLL_CFG));
      break; 


  } /* switch (pll_cfg) */
  
  return (ERR_CODE_NONE);

} /* falcon_tsc_configure_pll */


