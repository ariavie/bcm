/*******************************************************************************/
/*******************************************************************************/
/*  File Name     :  eagle_pll_config.c                                        */
/*  Created On    :  14/07/2013                                                */
/*  Created By    :  Kiran Divakar                                             */
/*  Description   :  Eagle PLL Configuration API                               */
/*  Revision      :  $Id: eagle_pll_config.c 406 2014-03-28 19:09:25Z kirand $ */
/*                                                                             */
/*  Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.              */
/*  All Rights Reserved                                                        */
/*  No portions of this material may be reproduced in any form without         */
/*  the written permission of:                                                 */
/*      Broadcom Corporation                                                   */
/*      5300 California Avenue                                                 */
/*      Irvine, CA  92617                                                      */
/*                                                                             */
/*  All information contained in this document is Broadcom Corporation         */
/*  company private proprietary, and trade secret.                             */
/*                                                                             */
/*******************************************************************************/
/*******************************************************************************/
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
 *  $Id: b459e89cf42de049e1765ee450259b833682bcf9 $
*/


/** @file eagle_pll_config.c
 * Eagle PLL Configuration
 */

#include "eagle_tsc_enum.h"

err_code_t eagle_tsc_configure_pll( const phymod_access_t *pa, enum eagle_tsc_pll_enum pll_cfg) { 

  /*Use this to restore defaults if reprogramming the PLL under dp-reset (typically Auto-Neg FW) */
     /* wrc_pll_mode(0xA); */
     /* wrc_ams_pll_fracn_ndiv_int       (0x0); */
     /* wrc_ams_pll_fracn_div_h          (0x0); */
     /* wrc_ams_pll_fracn_div_l          (0x0); */
     /* wrc_ams_pll_fracn_bypass         (0x0); */
     /* wrc_ams_pll_fracn_divrange       (0x0); */
     /* wrc_ams_pll_fracn_sel            (0x0); */
     /* wrc_ams_pll_ditheren             (0x0); */
     /* wrc_ams_pll_force_kvh_bw         (0x0); */
     /* wrc_ams_pll_vco_div2             (0x0); */
     /* wrc_ams_pll_vco_div4             (0x0); */
     /* wrc_ams_pll_refclk_doubler       (0x0); */

  switch (pll_cfg) {

    /******************/
    /*  Integer Mode  */
    /******************/

    case pll_div_80x :
      /* pll_mode<3:0> = 1101, VCO = 8.5G,  Refclk = 106.25MHz  */
      /* pll_mode<3:0> = 1101, VCO = 10G,   Refclk = 125MHz */
      /* pll_mode<3:0> = 1101, VCO = 12.5G, Refclk = 156.25MHz */
      wrc_pll_mode(0xD);
      break;
   
    case pll_div_46x :
      /* pll_mode<3:0> = 0000, VCO = 5.75G, Refclk = 125MHz */
      wrc_pll_mode(0x0);
      break;
   
    case pll_div_50x :
      /* pll_mode<3:0> = 0101, VCO = 6.25G, Refclk = 125MHz */
      wrc_pll_mode(0x5);
      break;
   
    case pll_div_68x :
      /* pll_mode<3:0> = 1011, VCO = 8.5G, Refclk = 125MHz */
      wrc_pll_mode(0xB);
      break;
   
    case pll_div_92x : 
      /* pll_mode<3:0> = 1110, VCO = 11.5G, Refclk = 125MHz */
      wrc_pll_mode(0xE);
      break;
   
    case pll_div_100x :
      /* pll_mode<3:0> = 1111, VCO = 12.5G, Refclk = 125MHz */
      wrc_pll_mode(0xF);
      break;
   
    case pll_div_40x :
      /* pll_mode<3:0> = 0010, VCO = 6.25G, Refclk = 156.25MHz  */
      wrc_pll_mode(0x2);
      break;
   
    case pll_div_42x :
      /* pll_mode<3:0> = 0011, VCO = 6.5625G, Refclk = 156.25MHz */
      wrc_pll_mode(0x3);
      break;
   
    case pll_div_52x : 
      /* pll_mode<3:0> = 0110, VCO = 8.125G, Refclk = 156.25MHz */
      wrc_pll_mode(0x6);
      break;
   
    case pll_div_54x :
      /* pll_mode<3:0> = 0111, VCO = 8.4375G, Refclk = 156.25MHz  */
      wrc_pll_mode(0x7);
      break;
   
    case pll_div_60x :
      /* pll_mode<3:0> = 1000, VCO = 9.375G, Refclk = 156.25MHz */
      wrc_pll_mode(0x8);
      break;
   
    case pll_div_64x : 
      /* pll_mode<3:0> = 1001, VCO = 10G, Refclk = 156.25MHz */
      /* pll_mode<3:0> = 1001, VCO = 10.3125G, Refclk = 161.13MHz */
      wrc_pll_mode(0x9);
      break;
   
    case pll_div_66x :
      /* pll_mode<3:0> = 1010, VCO = 10.3125G, Refclk = 156.25MHz */
      wrc_pll_mode(0xA);
      break;
   
    case pll_div_70x : 
      /* pll_mode<3:0> = 1100, VCO = 10.9375G, Refclk = 156.25MHz  */
      wrc_pll_mode(0xC);
      break;
      
    case pll_div_72x : 
      /* pll_mode<3:0> = 0001, VCO = 11.25G, Refclk = 156.25MHz */
      wrc_pll_mode(0x1);
      break;
     
 
    /*****************/
    /*  Frac-N Mode  */
    /*****************/
      
    case pll_div_82p5x :         /* Divider value 82.5, VCO = 10.3125G, Refclk = 125MHz */
      wrc_ams_pll_fracn_ndiv_int(0x52);
      wrc_ams_pll_fracn_div_l   (0x0000);
      wrc_ams_pll_fracn_div_h   (0x2);
      wrc_ams_pll_fracn_bypass  (0x0);
      wrc_ams_pll_fracn_divrange(0x0);
      wrc_ams_pll_fracn_sel     (0x1);
      wrc_ams_pll_ditheren      (0x1);
      wrc_ams_pll_force_kvh_bw  (0x1);
      wrc_ams_pll_vco_div2      (0x0);
      wrc_ams_pll_vco_div4      (0x0);
      wrc_ams_pll_refclk_doubler(0x0);
      break;
   
    case pll_div_87p5x :         /* Divider value 87.5, VCO = 10.9375G, Refclk = 125MHz */
      wrc_ams_pll_fracn_ndiv_int(0x57);
      wrc_ams_pll_fracn_div_l   (0x0000);
      wrc_ams_pll_fracn_div_h   (0x2);
      wrc_ams_pll_fracn_bypass  (0x0);
      wrc_ams_pll_fracn_divrange(0x0);
      wrc_ams_pll_fracn_sel     (0x1);
      wrc_ams_pll_ditheren      (0x1);
      wrc_ams_pll_force_kvh_bw  (0x1);
      wrc_ams_pll_vco_div2      (0x0);
      wrc_ams_pll_vco_div4      (0x0);
      wrc_ams_pll_refclk_doubler(0x0);
      break;
      
    case pll_div_73p6x :         /* Divider value 73.6, VCO = 11.5G, Refclk = 156.25MHz */
      wrc_ams_pll_fracn_ndiv_int(0x49);
      wrc_ams_pll_fracn_div_l   (0x6666);
      wrc_ams_pll_fracn_div_h   (0x2);
      wrc_ams_pll_fracn_bypass  (0x0);
      wrc_ams_pll_fracn_divrange(0x0);
      wrc_ams_pll_fracn_sel     (0x1);
      wrc_ams_pll_ditheren      (0x1);
      wrc_ams_pll_force_kvh_bw  (0x1);
      wrc_ams_pll_vco_div2      (0x0);
      wrc_ams_pll_vco_div4      (0x0);
      wrc_ams_pll_refclk_doubler(0x0);
      break;
      
    case pll_div_199p04x :       /* Divider value 199.04, VCO = 9.952G, Refclk = 25MHz */
      wrc_ams_pll_fracn_ndiv_int(0xc7);
      wrc_ams_pll_fracn_div_l   (0x28f5);
      wrc_ams_pll_fracn_div_h   (0x0);
      wrc_ams_pll_fracn_bypass  (0x0);
      wrc_ams_pll_fracn_divrange(0x0);
      wrc_ams_pll_fracn_sel     (0x1);
      wrc_ams_pll_ditheren      (0x1);
      wrc_ams_pll_force_kvh_bw  (0x1);
      wrc_ams_pll_vco_div2      (0x0);
      wrc_ams_pll_vco_div4      (0x0);
      wrc_ams_pll_refclk_doubler(0x1);
      break;
  
    default:                     /* Invalid pll_cfg value  */
       return (_error(ERR_CODE_INVALID_PLL_CFG));
       break; 

  } /* switch (pll_cfg) */
  
  return (ERR_CODE_NONE);

} /* eagle_tsc_configure_pll */


