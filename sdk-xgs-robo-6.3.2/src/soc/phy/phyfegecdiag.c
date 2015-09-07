/*
 * $Id: phyfegecdiag.c 1.1 Broadcom SDK $
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
 * File:        phycdiag.c
 * Purpose:	Cable diagnostic algorithm for default phy.
 *
 */
#if defined(BCM_ROBO_SUPPORT)
#include <sal/types.h>
#include <sal/core/thread.h>

#include <soc/phy.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/phyreg.h>

#include <soc/ll.h>
#include <soc/phy/phyctrl.h>

#include "phyreg.h"
#include "phyfege.h"
#include "phy54xx.h"

int phy_cdMain(int unit,soc_port_t l1,soc_port_cable_diag_t*l2){uint16 l3;
uint16 l4;uint8 l5;phy_ctrl_t*l6;l6 = EXT_PHY_SW_STATE(unit,l1);
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_BRCM_TEST_REG,
MII_BRCM_TEST_REG_DEFAULT|MII_BRCM_TEST_ENSHA));for(l4 = 0;l4<1000;l4++){l2->
npairs = 2;}SOC_IF_ERROR_RETURN(READ_PHY_REG(unit,l6,MII_SHA_CD_SEL_REG,&l3))
;if(l3&0x0040){SOC_IF_ERROR_RETURN(READ_PHY_REG(unit,l6,MII_SHA_AUX_STAT2_REG
,&l3));l5 = ((l3&MII_SHA_AUX_STAT2_LEN)>>12)*20;if(l5>= GOOD_CABLE_LEN_TUNING
){l5-= GOOD_CABLE_LEN_TUNING;}else{l5 = 0;}l2->fuzz_len = CABLE_FUZZY_LEN1;l2
->state = l2->pair_state[0] = l2->pair_state[1] = SOC_PORT_CABLE_STATE_OK;l2
->pair_len[0] = l2->pair_len[1] = CABLE_FUZZY_LEN1+l5;SOC_IF_ERROR_RETURN(
WRITE_PHY_REG(unit,l6,MII_BRCM_TEST_REG,MII_BRCM_TEST_REG_DEFAULT));return
SOC_E_NONE;}SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_SHA_CD_SEL_REG,0x0)
);SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_SHA_CD_CTRL_REG,0x0));
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_SHA_CD_SEL_REG,0x0200));
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_SHA_CD_CTRL_REG,0x4824));
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_SHA_CD_CTRL_REG,0x0400));
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_SHA_CD_CTRL_REG,0x0500));
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_SHA_CD_CTRL_REG,0xC404));
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_SHA_CD_CTRL_REG,0x0100));
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_SHA_CD_CTRL_REG,0x004F));
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_SHA_CD_SEL_REG,0x0));
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_SHA_CD_CTRL_REG,0x8006));do{for
(l4 = 0;l4<1000;l4++){l2->npairs = 2;}SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,
l6,MII_SHA_CD_SEL_REG,0x0));SOC_IF_ERROR_RETURN(READ_PHY_REG(unit,l6,
MII_SHA_CD_CTRL_REG,&l3));}while(l3&MII_SHA_CD_CTRL_START);if((l3&0x03c0)!= 
0x0300){SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6,MII_BRCM_TEST_REG,
MII_BRCM_TEST_REG_DEFAULT));return SOC_E_FAIL;}l2->fuzz_len = 
CABLE_FUZZY_LEN2;switch((l3&MII_SHA_CD_CTRL_PA_STAT)>>10){case 0:l2->
pair_state[0] = SOC_PORT_CABLE_STATE_OK;break;case 1:l2->pair_state[0] = 
SOC_PORT_CABLE_STATE_OPEN;break;case 2:l2->pair_state[0] = 
SOC_PORT_CABLE_STATE_SHORT;break;default:l2->pair_state[0] = 
SOC_PORT_CABLE_STATE_UNKNOWN;break;}switch((l3&MII_SHA_CD_CTRL_PB_STAT)>>12){
case 0:l2->pair_state[1] = SOC_PORT_CABLE_STATE_OK;break;case 1:l2->
pair_state[1] = SOC_PORT_CABLE_STATE_OPEN;break;case 2:l2->pair_state[1] = 
SOC_PORT_CABLE_STATE_SHORT;break;default:l2->pair_state[1] = 
SOC_PORT_CABLE_STATE_UNKNOWN;break;}l2->state = l2->pair_state[0];if(l2->
pair_state[1]>l2->state){l2->state = l2->pair_state[1];}SOC_IF_ERROR_RETURN(
READ_PHY_REG(unit,l6,MII_SHA_CD_CTRL_REG,&l3));l2->pair_len[0] = ((l3&
MII_SHA_CD_CTRL_PA_LEN)*80)/100;l2->pair_len[1] = (((l3&
MII_SHA_CD_CTRL_PB_LEN)>>8)*80)/100;SOC_IF_ERROR_RETURN(WRITE_PHY_REG(unit,l6
,MII_BRCM_TEST_REG,MII_BRCM_TEST_REG_DEFAULT));return SOC_E_NONE;}

#endif /* BCM_ROBO_SUPPORT */
int _phy_fege_cdiag_not_empty;
