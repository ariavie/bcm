/*
 * $Id: phy522xcdiag.c 1.5 Broadcom SDK $
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
 * File:        phy522x.c
 * Purpose:	Broadcom 522x phy driver
 *    	Supports 5218, 5220/5221, 5226, 5228, 5238, 5248
 *      and phy5324(8 ports 10/100 PHY, bcm5324 built-in)
 */
#ifdef INCLUDE_PHY_522X
#include <sal/types.h>
#include <sal/core/thread.h>

#include <soc/phy.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/phyreg.h>

#include <soc/phy.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>

#include "phyident.h"
#include "phyreg.h"
#include "phyfege.h"
#include "phy522x.h"

static int l1[] = {0,0,0,0,0,0,0,4,8,12,15,19,22,26,29,32,35,38,41,44,46,49,
51,54,56,59,61,63,66,68,70,72,74,76,78,80,81,83,85,87,89,90,92,94,95,97,99,
100,102,104,105,107,109,110,112,114,115,117,119,121,122,124,126,128};static
int l2[] = {0,0,0,0,0,0,0,0,0,0,0,5,5,10,10,15,20,25,25,30,30,35,40,45,45,50,
55,60,60,65,65,70,75,80,80,85,90,90,95,100,105,105,110,115,120,125,125,130,
130,135,140,145,145,150,150,150,150,150,150,150,150,150,150,150};int l3(int l4
,soc_port_t l5,soc_port_cable_diag_t*l6){phy_ctrl_t*l7;uint16 l8;uint16 l9;
int l10;int l11 = SOC_E_NONE;int l12;l7 = EXT_PHY_SW_STATE(l4,l5);
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,0x1f,0x008b));for(l12 = 0;l12<2;l12++
){sal_usleep(1000);SOC_IF_ERROR_RETURN(READ_PHY_REG(l4,l7,0x14,&l8));}if(l8&
0x0040){l6->npairs = 2;l6->fuzz_len = 10;l6->state = l6->pair_state[0] = l6->
pair_state[1] = SOC_PORT_CABLE_STATE_OK;SOC_IF_ERROR_RETURN(READ_PHY_REG(l4,
l7,0x18,&l8));l8 = (l8>>9)&0x7f;l10 = l8-0x40;l6->pair_len[0] = l6->pair_len[
1] = l2[l10];SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,0x1f,0x000b));return
SOC_E_NONE;}SOC_IF_ERROR_RETURN(READ_PHY_REG(l4,l7,0x00,&l9));l11 = 
WRITE_PHY_REG(l4,l7,0x00,0x2000);if(l11<0){goto l13;}l11 = WRITE_PHY_REG(l4,
l7,0x14,0x8000);if(l11<0){goto l13;}l11 = WRITE_PHY_REG(l4,l7,0x13,0x0002);if
(l11<0){goto l13;}l11 = WRITE_PHY_REG(l4,l7,0x13,0x0000);if(l11<0){goto l13;}
l11 = WRITE_PHY_REG(l4,l7,0x13,0x4868);if(l11<0){goto l13;}l11 = 
WRITE_PHY_REG(l4,l7,0x13,0x1f00);if(l11<0){goto l13;}l11 = WRITE_PHY_REG(l4,
l7,0x13,0x044e);if(l11<0){goto l13;}l11 = WRITE_PHY_REG(l4,l7,0x13,0x0446);if
(l11<0){goto l13;}l11 = WRITE_PHY_REG(l4,l7,0x13,0x0000);if(l11<0){goto l13;}
l11 = WRITE_PHY_REG(l4,l7,0x13,0x004e);if(l11<0){goto l13;}l11 = 
WRITE_PHY_REG(l4,l7,0x13,0x0c45);if(l11<0){goto l13;}l11 = WRITE_PHY_REG(l4,
l7,0x13,0x1040);if(l11<0){goto l13;}l11 = WRITE_PHY_REG(l4,l7,0x14,0x8000);if
(l11<0){goto l13;}l11 = WRITE_PHY_REG(l4,l7,0x13,0x0006);if(l11<0){goto l13;}
do{sal_usleep(1000);l11 = WRITE_PHY_REG(l4,l7,0x14,0x8000);if(l11<0){goto l13
;}l11 = READ_PHY_REG(l4,l7,0x13,&l8);if(l11<0){goto l13;}}while(l8&0x0004);l6
->npairs = 2;l6->fuzz_len = 0;switch((l8>>10)&0x3){case 0:l6->pair_state[0] = 
SOC_PORT_CABLE_STATE_OK;break;case 1:l6->pair_state[0] = 
SOC_PORT_CABLE_STATE_OPEN;break;case 2:l6->pair_state[0] = 
SOC_PORT_CABLE_STATE_SHORT;break;default:l6->pair_state[0] = 
SOC_PORT_CABLE_STATE_UNKNOWN;break;}switch((l8>>12)&0x3){case 0:l6->
pair_state[1] = SOC_PORT_CABLE_STATE_OK;break;case 1:l6->pair_state[1] = 
SOC_PORT_CABLE_STATE_OPEN;break;case 2:l6->pair_state[1] = 
SOC_PORT_CABLE_STATE_SHORT;break;default:l6->pair_state[1] = 
SOC_PORT_CABLE_STATE_UNKNOWN;break;}l6->state = l6->pair_state[0];if(l6->
pair_state[1]>l6->state){l6->state = l6->pair_state[1];}l11 = READ_PHY_REG(l4
,l7,0x13,&l8);if(l11<0){goto l13;}l6->pair_len[0] = ((l8&0xff)*80)/100;l6->
pair_len[1] = (((l8>>8)&0xff)*80)/100;l11 = WRITE_PHY_REG(l4,l7,0x1f,0x000b);
l13:SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,0x00,l9));return l11;}int
phy_522x_cable_diag(int l4,soc_port_t l5,soc_port_cable_diag_t*l6){phy_ctrl_t
*l7;uint16 l8;int l10;int l12;uint8 l14;int l15;l7 = EXT_PHY_SW_STATE(l4,l5);
if(!(PHY_IS_BCM5248(l7)||PHY_IS_BCM5348(l7)||PHY_IS_BCM5324(l7)||
PHY_IS_BCM5324A1(l7)||PHY_IS_BCM53242(l7)||PHY_IS_BCM53262(l7)||
PHY_IS_BCM53280(l7))){return SOC_E_UNAVAIL;}if(PHY_IS_BCM53280_A0(l7)){return
SOC_E_UNAVAIL;}if(l6 == NULL){return SOC_E_PARAM;}SOC_IF_ERROR_RETURN(
READ_PHY_REG(l4,l7,MII_ASSR_REG,&l8));l14 = (l8&MII_ASSR_LS)?TRUE:FALSE;if(
PHY_IS_BCM5348(l7)||PHY_IS_BCM5324(l7)||PHY_IS_BCM5324A1(l7)||PHY_IS_BCM53242
(l7)||PHY_IS_BCM53262(l7)||PHY_IS_BCM53280(l7)){SOC_IF_ERROR_RETURN(
phy_fe_ge_speed_get(l4,l5,&l15));if((l15!= 100)&&(l14)){soc_cm_debug(DK_WARN,
"Cable diagnostic is only supported for 100Mb mode\n");return SOC_E_UNAVAIL;}
}if(PHY_IS_BCM53280(l7)){return(l3(l4,l5,l6));}SOC_IF_ERROR_RETURN(
WRITE_PHY_REG(l4,l7,0x1f,0x008b));sal_usleep(1000);if(!PHY_IS_BCM5248(l7)){if
(l14){l6->npairs = 2;l6->fuzz_len = 5;l6->state = l6->pair_state[0] = l6->
pair_state[1] = SOC_PORT_CABLE_STATE_OK;SOC_IF_ERROR_RETURN(READ_PHY_REG(l4,
l7,0x18,&l8));l6->pair_len[0] = l6->pair_len[1] = ((((uint32)((l8&0x7E00)>>9)
)*1000L-9163L)*172L/100L/1000L);SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,0x1f,
0x000b));return SOC_E_NONE;}}else{SOC_IF_ERROR_RETURN(READ_PHY_REG(l4,l7,0x14
,&l8));if(l8&0x0040){l6->npairs = 2;l6->fuzz_len = 10;l6->state = l6->
pair_state[0] = l6->pair_state[1] = SOC_PORT_CABLE_STATE_OK;for(l10 = 0,l12 = 
0;l12<10;l12++){SOC_IF_ERROR_RETURN(READ_PHY_REG(l4,l7,0x18,&l8));l8 = (l8>>9
)&0x3f;l10+= l8;}l10 = l10/10;l6->pair_len[0] = l6->pair_len[1] = l1[l10];
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,0x1f,0x000b));return SOC_E_NONE;}}
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,0x14,0x0000));SOC_IF_ERROR_RETURN(
WRITE_PHY_REG(l4,l7,0x13,0x0000));SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,
0x13,0x0000));SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,0x13,0x4824));if(
PHY_IS_BCM53242(l7)||PHY_IS_BCM53262(l7)){SOC_IF_ERROR_RETURN(WRITE_PHY_REG(
l4,l7,0x13,0x3000));}else{SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,0x13,0x4000
));}SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,0x13,0x0500));SOC_IF_ERROR_RETURN
(WRITE_PHY_REG(l4,l7,0x13,0xc404));SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,
0x13,0x0100));if(PHY_IS_BCM53242(l7)||PHY_IS_BCM53262(l7)){
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,0x13,0x004f));}else{
SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7,0x13,0x004c));}SOC_IF_ERROR_RETURN(
WRITE_PHY_REG(l4,l7,0x13,0x8006));do{sal_usleep(1000);SOC_IF_ERROR_RETURN(
WRITE_PHY_REG(l4,l7,0x14,0x0000));SOC_IF_ERROR_RETURN(READ_PHY_REG(l4,l7,0x13
,&l8));}while(l8&0x0004);if((l8&0x03c0)!= 0x0300){SOC_IF_ERROR_RETURN(
WRITE_PHY_REG(l4,l7,0x1f,0x000b));return SOC_E_FAIL;}l6->npairs = 2;l6->
fuzz_len = 0;switch((l8>>10)&0x3){case 0:l6->pair_state[0] = 
SOC_PORT_CABLE_STATE_OK;break;case 1:l6->pair_state[0] = 
SOC_PORT_CABLE_STATE_OPEN;break;case 2:l6->pair_state[0] = 
SOC_PORT_CABLE_STATE_SHORT;break;default:l6->pair_state[0] = 
SOC_PORT_CABLE_STATE_UNKNOWN;break;}switch((l8>>12)&0x3){case 0:l6->
pair_state[1] = SOC_PORT_CABLE_STATE_OK;break;case 1:l6->pair_state[1] = 
SOC_PORT_CABLE_STATE_OPEN;break;case 2:l6->pair_state[1] = 
SOC_PORT_CABLE_STATE_SHORT;break;default:l6->pair_state[1] = 
SOC_PORT_CABLE_STATE_UNKNOWN;break;}switch((l8>>12)&0x3){case 0:l6->
pair_state[1] = SOC_PORT_CABLE_STATE_OK;break;case 1:l6->pair_state[1] = 
SOC_PORT_CABLE_STATE_OPEN;break;case 2:l6->pair_state[1] = 
SOC_PORT_CABLE_STATE_SHORT;break;default:l6->pair_state[1] = 
SOC_PORT_CABLE_STATE_UNKNOWN;break;}l6->state = l6->pair_state[0];if(l6->
pair_state[1]>l6->state){l6->state = l6->pair_state[1];}SOC_IF_ERROR_RETURN(
READ_PHY_REG(l4,l7,0x13,&l8));l6->pair_len[0] = ((l8&0xff)*80)/100;l6->
pair_len[1] = (((l8>>8)&0xff)*80)/100;SOC_IF_ERROR_RETURN(WRITE_PHY_REG(l4,l7
,0x1f,0x000b));return SOC_E_NONE;}

#else
int _phy522xcdiag_not_empty;
#endif /* INCLUDE_PHY_522X */
