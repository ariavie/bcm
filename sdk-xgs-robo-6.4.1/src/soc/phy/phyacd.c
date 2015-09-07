/*
 * $Id: phyacd.c,v 1.7 Broadcom SDK $
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
 * File:        phyacd.c
 * Purpose:     PHY ACD routines
 */

#include <sal/core/thread.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/phy/phyctrl.h>
#include <shared/bsl.h>
#include "phyreg.h"

#define READ_PHY_EXP_REG(_unit, _phy_ctrl, reg, _val) \
                phy_reg_ge_read((_unit), (_phy_ctrl), 0x00, 0x0f00 | ((reg) & 0xff), 0x15, (_val))
#define WRITE_PHY_EXP_REG(_unit, _phy_ctrl, reg, _val) \
                phy_reg_ge_write((_unit), (_phy_ctrl), 0x00, 0x0f00 | ((reg) & 0xff), \
                                                       0x15, (_val))
#define MODIFY_PHY_EXP_REG(_unit, _phy_ctrl, reg, _val, _mask) \
                phy_reg_ge_modify((_unit), (_phy_ctrl), 0x00, 0x0f00 | ((reg) & 0xff), \
                                                        0x15, (_val), (_mask))

int phy_acd_cable_diag_init(int unit,soc_port_t port){phy_ctrl_t*l1;l1 = 
EXT_PHY_SW_STATE(unit,port);SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,
0x90,0x0000));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xC7,0xA01A));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xC8,0x0000));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xC9,0x00EF));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xCC,0x200));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xCE,0x4000));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xCF,0x3000));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE0,0x0010));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE1,0xD0D));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE2,0x0));SOC_IF_ERROR_RETURN(
WRITE_PHY_EXP_REG(unit,l1,0xE3,0x1000));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG
(unit,l1,0xE4,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE7,0xA0))
;SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xEF,0x409F));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xCD,0x1000));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xCD,0x0));SOC_IF_ERROR_RETURN(
WRITE_PHY_EXP_REG(unit,l1,0xE0,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(
unit,l1,0xE1,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE2,0x0));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE3,0x0));SOC_IF_ERROR_RETURN(
WRITE_PHY_EXP_REG(unit,l1,0xE4,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(
unit,l1,0xE7,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xEF,0x0));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE0,0x3600));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE1,0xC));SOC_IF_ERROR_RETURN(
WRITE_PHY_EXP_REG(unit,l1,0xE1,0x343A));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG
(unit,l1,0xE2,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE3,0x0000
));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE4,0x8000));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE5,0x000C));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE7,0x0000));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE9,0x0400));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xED,0x0000));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xEF,0xA1BF));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xCD,0x1000));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xCD,0x0));SOC_IF_ERROR_RETURN(
WRITE_PHY_EXP_REG(unit,l1,0xE0,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(
unit,l1,0xE1,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE2,0x0));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE3,0x0));SOC_IF_ERROR_RETURN(
WRITE_PHY_EXP_REG(unit,l1,0xE4,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(
unit,l1,0xE5,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE7,0x0));
SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xE8,0x0));SOC_IF_ERROR_RETURN(
WRITE_PHY_EXP_REG(unit,l1,0xE9,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(
unit,l1,0xED,0x0));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG(unit,l1,0xEF,0x0));
return SOC_E_NONE;}static int _phy54680_read_exp_c_array(int unit,phy_ctrl_t*
l1,uint16 l2[]){SOC_IF_ERROR_RETURN(READ_PHY_EXP_REG(unit,l1,0xc0,&l2[0]));
SOC_IF_ERROR_RETURN(READ_PHY_EXP_REG(unit,l1,0xc1,&l2[1]));
SOC_IF_ERROR_RETURN(READ_PHY_EXP_REG(unit,l1,0xc2,&l2[2]));
SOC_IF_ERROR_RETURN(READ_PHY_EXP_REG(unit,l1,0xc3,&l2[3]));
SOC_IF_ERROR_RETURN(READ_PHY_EXP_REG(unit,l1,0xc4,&l2[4]));
SOC_IF_ERROR_RETURN(READ_PHY_EXP_REG(unit,l1,0xc5,&l2[5]));
SOC_IF_ERROR_RETURN(READ_PHY_EXP_REG(unit,l1,0xc6,&l2[6]));bsl_normal(BSL_SOC_PHY, unit, 
"u=%d p=%d EXP_C C0=%04x C1=%04x C2=%04x C3=%04x C4=%04x C5=%04x C6=%04x\n",
unit,l1->port,l2[0],l2[1],l2[2],l2[3],l2[4],l2[5],l2[6]);return SOC_E_NONE;}
static int _exec_ACD(int unit,phy_ctrl_t*l1,int*l3,uint16*l4,int*l5,int*l6){
uint16 l7[7];int l8,l9,l10;l10 = *l3;l7[0] = l7[1] = l7[2] = l7[3] = l7[4] = 
l7[5] = l7[6] = 0;SOC_IF_ERROR_RETURN(_phy54680_read_exp_c_array(unit,l1,l7))
;if((l7[0]&0x0800)){bsl_normal(BSL_SOC_PHY, unit,
"ACD Engine still busy u=%d p=%d\n",unit,l1->port);}SOC_IF_ERROR_RETURN(
WRITE_PHY_EXP_REG(unit,l1,0xC0,0x2000));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG
(unit,l1,0xC0,0xA000));l9 = 0;for(l8 = 0;l8<300;l8++){SOC_IF_ERROR_RETURN(
_phy54680_read_exp_c_array(unit,l1,l7));if(l7[0]&0x4){l9 = 1;}if(!(l7[0]&
0x0800)){break;}}if(l8>= 300){l10|= 0x0001;bsl_normal(BSL_SOC_PHY, unit,
"u=%d p=%d ERROR_ACD_BUSY_ON_DEMAND\n",unit,l1->port);}if(!l9){l10|= 0x0008;
bsl_normal(BSL_SOC_PHY, unit,"u=%d p=%d ERROR_ACD_NO_NEW_RESULT\n",unit,l1->port)
;}if(l7[0]&0x8){l10|= 0x0004;bsl_normal(BSL_SOC_PHY, unit,
"u=%d p=%d ERROR_ACD_INVALID\n",unit,l1->port);}*l4 = l7[1];*l5 = (l7[6]+l7[
4])/2;*l6 = (l7[5]+l7[3])/2;return l10;}int phy_acd_cable_diag(int unit,
soc_port_t port,soc_port_cable_diag_t*l11){phy_ctrl_t*l1;uint16 l4 =0;int l3,
l5 =0,l6;l1 = EXT_PHY_SW_STATE(unit,port);l3 = 0;SOC_IF_ERROR_RETURN(
WRITE_PHY_EXP_REG(unit,l1,0xA4,0x0008));SOC_IF_ERROR_RETURN(WRITE_PHY_EXP_REG
(unit,l1,0xA4,0x4008));l3 = _exec_ACD(unit,l1,&l3,&l4,&l5,&l6);if(l3){
bsl_warn(BSL_SOC_PHY, unit,
"u=%d p=%d cable diag test failed error_flag = 0x%04x\n",unit,l1->port,l3);
return SOC_E_FAIL;}l11->npairs = 1;l11->fuzz_len = 0;bsl_normal(BSL_SOC_PHY, unit,
"u=%d p=%d fault = %x\n",unit,l1->port,l4);if(l4 == 0x2222){l11->state = 
SOC_PORT_CABLE_STATE_OPEN;l11->pair_state[0] = SOC_PORT_CABLE_STATE_OPEN;l11
->pair_len[0] = (l5*1000)/1325;}else if(l4 == 0x3333){l11->state = 
SOC_PORT_CABLE_STATE_SHORT;l11->pair_state[0] = SOC_PORT_CABLE_STATE_SHORT;
l11->pair_len[0] = (l5*1000)/1325;}else{l11->state = SOC_PORT_CABLE_STATE_OK;
l11->pair_state[0] = SOC_PORT_CABLE_STATE_OK;l11->pair_len[0] = 0;}return
SOC_E_NONE;}
