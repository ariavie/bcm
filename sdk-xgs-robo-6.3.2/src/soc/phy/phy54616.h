/*
 * $Id: phy54616.h 1.1 Broadcom SDK $
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
 * File:        phy54616.h
 *
 * This driver applies to both BCM54616 and BCM54616S.
 * Both parts support driving Copper or SERDES interfaces.
 *
 */

#ifndef   _PHY54616_H_
#define   _PHY54616_H_

#include <soc/phy.h>
 
/* PHY register access */
#define PHY54616_REG_READ(_unit, _phy_ctrl, _flags, _reg_bank, _reg_addr, _val) \
            phy_reg_ge_read((_unit), (_phy_ctrl), (_flags), (_reg_bank), \
                            (_reg_addr), (_val))
#define PHY54616_REG_WRITE(_unit, _phy_ctrl, _flags, _reg_bank, _reg_addr, _val) \
            phy_reg_ge_write((_unit), (_phy_ctrl), (_flags), (_reg_bank), \
                             (_reg_addr), (_val))
#define PHY54616_REG_MODIFY(_unit, _phy_ctrl, _flags, _reg_bank, _reg_addr, \
                           _val, _mask) \
            phy_reg_ge_modify((_unit), (_phy_ctrl), (_flags), (_reg_bank), \
                              (_reg_addr), (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T MII Control Register (Addr 00h) */
#define READ_PHY54616_MII_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x00, (_val))
#define WRITE_PHY54616_MII_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x00, (_val)) 
#define MODIFY_PHY54616_MII_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x00, \
                               (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T MII Status Register (ADDR 01h) */
#define READ_PHY54616_MII_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x01, (_val)) 
#define WRITE_PHY54616_MII_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x01, (_val)) 
#define MODIFY_PHY54616_MII_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x01, \
                               (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T PHY Identifier Register (ADDR 02h) */
#define READ_PHY54616_MII_PHY_ID0r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x02, (_val))
#define WRITE_PHY54616_MII_PHY_ID0r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x02, (_val))
#define MODIFY_PHY54616_MII_PHY_ID0r(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x02, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T PHY Identifier Register (ADDR 03h) */
#define READ_PHY54616_MII_PHY_ID1r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x03, (_val))
#define WRITE_PHY54616_MII_PHY_ID1r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x03, (_val))
#define MODIFY_PHY54616_MII_PHY_ID1r(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x03, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auto-neg Advertisment Register (ADDR 04h) */
#define READ_PHY54616_MII_ANAr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x04, (_val))
#define WRITE_PHY54616_MII_ANAr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x04, (_val))
#define MODIFY_PHY54616_MII_ANAr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x04, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auto-neg Link Partner Ability (ADDR 05h) */
#define READ_PHY54616_MII_ANPr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x05, (_val))
#define WRITE_PHY54616_MII_ANPr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x05, (_val)) 
#define MODIFY_PHY54616_MII_ANPr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x05, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auto-neg Expansion Register (ADDR 06h) */
#define READ_PHY54616_MII_AN_EXPr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x06, (_val))
#define WRITE_PHY54616_MII_AN_EXPr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x06, (_val))
#define MODIFY_PHY54616_MII_AN_EXPr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x06, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Next Page Transmit Register (ADDR 07h) */
/* 1000BASE-T/100BASE-TX/10BASE-T Link Partner Received Next Page (ADDR 08h) */

/* 1000BASE-T Control Register  (ADDR 09h) */
#define READ_PHY54616_MII_GB_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x09, (_val))
#define WRITE_PHY54616_MII_GB_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x09, (_val))
#define MODIFY_PHY54616_MII_GB_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x09, \
                             (_val), (_mask))

/* 1000BASE-T Status Register (ADDR 0ah) */
#define READ_PHY54616_MII_GB_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x0a, (_val)) 
#define WRITE_PHY54616_MII_GB_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x0a, (_val))
#define MODIFY_PHY54616_MII_GB_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x0a, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T IEEE Extended Status Register (ADDR 0fh) */

/* 1000BASE-T/100BASE-TX/10BASE-T PHY Extended Control Register (ADDR 10h) */
#define READ_PHY54616_MII_ECRr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x10, (_val)) 
#define WRITE_PHY54616_MII_ECRr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x10, (_val))
#define MODIFY_PHY54616_MII_ECRr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x10, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T PHY Extended Status Register (ADDR 11h) */
#define READ_PHY54616_MII_ESRr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x11, (_val)) 
#define WRITE_PHY54616_MII_ESRr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x11, (_val))
#define MODIFY_PHY54616_MII_ESRr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x11, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Receive Error Counter Register (ADDR 12h) */
/* 1000BASE-T/100BASE-TX/10BASE-T False Carrier Sense Counter (ADDR 13h) */
/* 1000BASE-T/100BASE-TX/10BASE-T Receive NOT_OK Counter Register (ADDR 14h) */

/* 1000BASE-T/100BASE-TX/10BASE-T Auxiliary Control Reg (ADDR 18h Shadow 000)*/
#define READ_PHY54616_MII_AUX_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x18, (_val))
#define WRITE_PHY54616_MII_AUX_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x18, (_val))
#define MODIFY_PHY54616_MII_AUX_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x18, \
                             (_val), (_mask))

/* 10BASE-T Register (ADDR 18h Shadow 001) */
#define READ_PHY54616_MII_10BASE_Tr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0001, 0x18, (_val))
#define WRITE_PHY54616_MII_10BASE_Tr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0001, 0x18, (_val))
#define MODIFY_PHY54616_MII_10BASE_Tr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0001, 0x18, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Power/MII Control Reg (ADDR 18h Shadow 010)*/
#define READ_PHY54616_MII_POWER_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0002, 0x18, (_val))
#define WRITE_PHY54616_MII_POWER_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0002, 0x18, (_val))
#define MODIFY_PHY54616_MII_POWER_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0002, 0x18, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Misc Test Register (ADDR 18h Shadow 100) */
#define READ_PHY54616_MII_MISC_TESTr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0004, 0x18, (_val))
#define WRITE_PHY54616_MII_MISC_TESTr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0004, 0x18, (_val))
#define MODIFY_PHY54616_MII_MISC_TESTr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0004, 0x18, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Misc Control Register (ADDR 18h Shadow 111)*/
#define READ_PHY54616_MII_MISC_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0007, 0x18, (_val))
#define WRITE_PHY54616_MII_MISC_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0007, 0x18, (_val))
#define MODIFY_PHY54616_MII_MISC_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0007, 0x18, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auxiliary Status Register (ADDR 19h) */
/* 1000BASE-T/100BASE-TX/10BASE-T Interrupt Status Register (ADDR 1ah) */
/* 1000BASE-T/100BASE-TX/10BASE-T Interrupt Control Register (ADDR 1bh) */

/* 1000BASE-T/100BASE-TX/10BASE-T Spare Ctrl Reg (ADDR 1ch shadow 00010) */
#define READ_PHY54616_SPARE_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0002, 0x1c, (_val))
#define WRITE_PHY54616_SPARE_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0002, 0x1c, (_val))
#define MODIFY_PHY54616_SPARE_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0002, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Clk Alignment Ctrl (ADDR 1ch shadow 00011) */
#define READ_PHY54616_CLK_ALIGN_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0003, 0x1c, (_val))
#define WRITE_PHY54616_CLK_ALIGN_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0003, 0x1c, (_val))
#define MODIFY_PHY54616_CLK_ALIGN_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0003, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Spare Ctrl 2 Reg (ADDR 1ch shadow 00100) */
#define READ_PHY54616_SPARE_CTRL_2r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0004, 0x1c, (_val))
#define WRITE_PHY54616_SPARE_CTRL_2r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0004, 0x1c, (_val))
#define MODIFY_PHY54616_SPARE_CTRL_2r(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0004, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Spare Ctrl 3 Reg (ADDR 1ch shadow 00101) */
#define READ_PHY54616_SPARE_CTRL_3r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0005, 0x1c, (_val))
#define WRITE_PHY54616_SPARE_CTRL_3r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0005, 0x1c, (_val))
#define MODIFY_PHY54616_SPARE_CTRL_3r(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0005, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED Status Reg (ADDR 1ch shadow 01000) */
#define READ_PHY54616_LED_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0008, 0x1c, (_val))
#define WRITE_PHY54616_LED_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0008, 0x1c, (_val))
#define MODIFY_PHY54616_LED_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0008, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED Ctrl Reg (ADDR 1ch shadow 01001) */
#define READ_PHY54616_LED_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0009, 0x1c, (_val))
#define WRITE_PHY54616_LED_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0009, 0x1c, (_val))
#define MODIFY_PHY54616_LED_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0009, 0x1c, \
                           (_val), (_mask))

/* Auto Power-Down Reg (ADDR 1ch shadow 01010) */
#define READ_PHY54616_AUTO_POWER_DOWNr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x000a, 0x1c, (_val))
#define WRITE_PHY54616_AUTO_POWER_DOWNr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x000a, 0x1c, (_val))
#define MODIFY_PHY54616_AUTO_POWER_DOWNr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x000a, 0x1c, \
                           (_val), (_mask))

#define PHY_54616_AUTO_PWRDWN_EN              (1 << 5) 
#define PHY_54616_AUTO_PWRDWN_WAKEUP_MASK     (0xF << 0)
#define PHY_54616_AUTO_PWRDWN_WAKEUP_UNIT     84  /* ms */
#define PHY_54616_AUTO_PWRDWN_WAKEUP_MAX      1260  /* ms */ 
#define PHY_54616_AUTO_PWRDWN_SLEEP_MASK      (1 << 4)
#define PHY_54616_AUTO_PWRDWN_SLEEP_MAX       5400  /* ms */

/* 1000BASE-T/100BASE-TX/10BASE-T LED Selector 1 Reg (ADDR 1ch shadow 01101) */
#define READ_PHY54616_LED_SELECTOR_1r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x000d, 0x1c, (_val))
#define WRITE_PHY54616_LED_SELECTOR_1r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x000d, 0x1c, (_val))
#define MODIFY_PHY54616_LED_SELECTOR_1r(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x000d, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED Selector 2 Reg (ADDR 1ch shadow 01110) */
#define READ_PHY54616_LED_SELECTOR_2r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x000e, 0x1c, (_val))
#define WRITE_PHY54616_LED_SELECTOR_2r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x000e, 0x1c, (_val))
#define MODIFY_PHY54616_LED_SELECTOR_2r(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x000e, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED GPIO Ctrl/Stat (ADDR 1ch shadow 01111) */
#define READ_PHY54616_LED_GPIO_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x000f, 0x1c, (_val))
#define WRITE_PHY54616_LED_GPIO_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x000f, 0x1c, (_val))
#define MODIFY_PHY54616_LED_GPIO_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x000f, 0x1c, \
                           (_val), (_mask))

/* SerDes 100BASE-FX Status Reg (ADDR 1ch shadow 10001) */
#define READ_PHY54616_100FX_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0011, 0x1c, (_val))
#define WRITE_PHY54616_100FX_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0011, 0x1c, (_val))
#define MODIFY_PHY54616_100FX_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0011, 0x1c, \
                           (_val), (_mask))

/* SerDes 100BASE-FX Control Reg (ADDR 1ch shadow 10011) */
#define READ_PHY54616_100FX_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0013, 0x1c, (_val))
#define WRITE_PHY54616_100FX_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0013, 0x1c, (_val))
#define MODIFY_PHY54616_100FX_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0013, 0x1c, \
                           (_val), (_mask))

/* Secondary SerDes Control Reg (ADDR 1ch shadow 10100) */
#define READ_PHY54616_2ND_SERDES_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0014, 0x1c, (_val))
#define WRITE_PHY54616_2ND_SERDES_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0014, 0x1c, (_val))
#define MODIFY_PHY54616_2ND_SERDES_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0014, 0x1c, \
                           (_val), (_mask))

/* SGMII Slave Reg (ADDR 1ch shadow 10101) */
#define READ_PHY54616_SGMII_SLAVEr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0015, 0x1c, (_val))
#define WRITE_PHY54616_SGMII_SLAVEr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0015, 0x1c, (_val))
#define MODIFY_PHY54616_SGMII_SLAVEr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0015, 0x1c, \
                           (_val), (_mask))

/* Primary SerDes Control Reg (ADDR 1ch shadow 10110) */
#define READ_PHY54616_1ST_SERDES_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0016, 0x1c, (_val))
#define WRITE_PHY54616_1ST_SERDES_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0016, 0x1c, (_val))
#define MODIFY_PHY54616_1ST_SERDES_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0016, 0x1c, \
                           (_val), (_mask))


/* Misc 1000X Control 2 Reg (ADDR 1ch shadow 10111) */
#define READ_PHY54616_MISC_1000X_CTRL2r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0017, 0x1c, (_val))
#define WRITE_PHY54616_MISC_1000X_CTRL2r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0017, 0x1c, (_val))
#define MODIFY_PHY54616_MISC_1000X_CTRL2r(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0017, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-X Auto-Detect SGMII/Media Converter Reg (ADDR 1ch shadow 11000) */
#define READ_PHY54616_AUTO_DETECT_SGMII_MEDIAr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0018, 0x1c, (_val))
#define WRITE_PHY54616_AUTO_DETECT_SGMII_MEDIAr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0018, 0x1c, (_val))
#define MODIFY_PHY54616_AUTO_DETECT_SGMII_MEDIAr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0018, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-X Auto-neg Debug Reg (ADDR 1ch shadow 11010) */
#define READ_PHY54616_1000X_AN_DEBUGr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001a, 0x1c, (_val))
#define WRITE_PHY54616_1000X_AN_DEBUGr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001a, 0x1c, (_val))
#define MODIFY_PHY54616_1000X_AN_DEBUGr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001a, 0x1c, \
                           (_val), (_mask))

/* Auxiliary 1000BASE-X Control Reg (ADDR 1ch shadow 11011) */
#define READ_PHY54616_AUX_1000X_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001b, 0x1c, (_val))
#define WRITE_PHY54616_AUX_1000X_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001b, 0x1c, (_val))
#define MODIFY_PHY54616_AUX_1000X_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001b, 0x1c, \
                           (_val), (_mask))

/* Auxiliary 1000BASE-X Status Reg (ADDR 1ch shadow 11100) */
#define READ_PHY54616_AUX_1000X_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001c, 0x1c, (_val))
#define WRITE_PHY54616_AUX_1000X_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001c, 0x1c, (_val))
#define MODIFY_PHY54616_AUX_1000X_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001c, 0x1c, \
                           (_val), (_mask))

/* Misc 1000BASE-X Status Reg (ADDR 1ch shadow 11101) */
#define READ_PHY54616_MISC_1000X_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001d, 0x1c, (_val))
#define WRITE_PHY54616_MISC_1000X_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001d, 0x1c, (_val))
#define MODIFY_PHY54616_MISC_1000X_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001d, 0x1c, \
                           (_val), (_mask))

/* Copper/Fiber Auto-Detect Medium Reg (ADDR 1ch shadow 11110) */
#define READ_PHY54616_AUTO_DETECT_MEDIUMr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001e, 0x1c, (_val))
#define WRITE_PHY54616_AUTO_DETECT_MEDIUMr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001e, 0x1c, (_val))
#define MODIFY_PHY54616_AUTO_DETECT_MEDIUMr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001e, 0x1c, \
                           (_val), (_mask))

/* Mode Control Reg (ADDR 1ch shadow 11111) */
#define READ_PHY54616_MODE_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001f, 0x1c, (_val))
#define WRITE_PHY54616_MODE_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001f, 0x1c, (_val))
#define MODIFY_PHY54616_MODE_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001f, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Master/Slave Seed Reg (ADDR 1dh bit 15 = 0) */
/* 1000BASE-T/100BASE-TX/10BASE-T HDC Status Reg (ADDR 1dh bit 15 = 1) */

/* 1000BASE-T/100BASE-TX/10BASE-T Test Register 1 (ADDR 1eh) */
#define READ_PHY54616_TEST1r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x1e, (_val))
#define WRITE_PHY54616_TEST1r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x1e, (_val))
#define MODIFY_PHY54616_TEST1r(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x1e, \
                           (_val), (_mask))


/*          +------------------------------+
 *          |                              |
 *          |   Primary SerDes Registers   |
 *          |                              |
 *          +------------------------------+
 */
/* 1000BASE-X MII Control Register (Addr 00h) */
#define READ_PHY54616_1000X_MII_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x00, (_val))
#define WRITE_PHY54616_1000X_MII_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x00, (_val))
#define MODIFY_PHY54616_1000X_MII_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x00, (_val), (_mask))

/* 1000BASE-X MII Status Register (Addr 01h) */
#define READ_PHY54616_1000X_MII_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x01, (_val))
#define WRITE_PHY54616_1000X_MII_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x01, (_val))
#define MODIFY_PHY54616_1000X_MII_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x01, (_val), (_mask))

/* 1000BASE-X MII Auto-neg Advertise Register (Addr 04h) */
#define READ_PHY54616_1000X_MII_ANAr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x04, (_val))
#define WRITE_PHY54616_1000X_MII_ANAr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x04, (_val))
#define MODIFY_PHY54616_1000X_MII_ANAr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x04, (_val), (_mask))

/* 1000BASE-X MII Auto-neg Link Partner Ability Register (Addr 05h) */
#define READ_PHY54616_1000X_MII_ANPr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x05, (_val))
#define WRITE_PHY54616_1000X_MII_ANPr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x05, (_val))
#define MODIFY_PHY54616_1000X_MII_ANPr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x05, (_val), (_mask))

/* 1000BASE-X MII Auto-neg Extended Status Register (Addr 06h) */
#define READ_PHY54616_1000X_MII_AN_EXPr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x06, (_val))
#define WRITE_PHY54616_1000X_MII_AN_EXPr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x06, (_val))
#define MODIFY_PHY54616_1000X_MII_AN_EXPr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x06, (_val), (_mask))

/* 1000BASE-X MII IEEE Extended Status Register (Addr 0fh) */
#define READ_PHY54616_1000X_MII_EXT_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x0f, (_val))
#define WRITE_PHY54616_1000X_MII_EXT_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x0f, (_val))
#define MODIFY_PHY54616_1000X_MII_EXT_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                             0x0000, 0x0f, (_val), (_mask))

/*          +-------------------------+
 *          |                         |
 *          |   Expansion Registers   |
 *          |                         |
 *          +-------------------------+
 */
/* Receive/Transmit Packet Counter Register (Addr 00h) */
#define READ_PHY54616_EXP_PKT_COUNTERr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f00, 0x15, (_val))
#define WRITE_PHY54616_EXP_PKT_COUNTERr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f00, 0x15, (_val))
#define MODIFY_PHY54616_EXP_PKT_COUNTERr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f00, 0x15, \
                                (_val), (_mask))

/* Expansion Interrupt Status Register (Addr 01h) */
#define READ_PHY54616_EXP_INTERRUPT_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f01, 0x15, (_val))
#define WRITE_PHY54616_EXP_r(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f01, 0x15, (_val))
#define MODIFY_PHY54616_EXP_r(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f01, 0x15, \
                                (_val), (_mask))

/* Expansion Interrupt Mask Register (Addr 02h) */
#define READ_PHY54616_EXP_INTERRUPT_MASKr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f02, 0x15, (_val))
#define WRITE_PHY54616_EXP_INTERRUPT_MASKr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f02, 0x15, (_val))
#define MODIFY_PHY54616_EXP_INTERRUPT_MASKr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f02, 0x15, \
                                (_val), (_mask))

/* Multicolor LED Selector Register (Addr 04h) */
#define READ_PHY54616_EXP_LED_SELECTORr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f04, 0x15, (_val))
#define WRITE_PHY54616_EXP_LED_SELECTORr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f04, 0x15, (_val))
#define MODIFY_PHY54616_EXP_LED_SELECTORr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f04, 0x15, \
                                (_val), (_mask))

/* Multicolor LED Flash Rate Controls Register (Addr 05h) */
#define READ_PHY54616_EXP_LED_FLASH_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f05, 0x15, (_val))
#define WRITE_PHY54616_EXP_LED_FLASH_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f05, 0x15, (_val))
#define MODIFY_PHY54616_EXP_LED_FLASH_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f05, 0x15, \
                                (_val), (_mask))

/* Multicolor LED Programmable Blink Controls Register (Addr 06h) */
#define READ_PHY54616_EXP_LED_BLINK_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f06, 0x15, (_val))
#define WRITE_PHY54616_EXP_LED_BLINK_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f06, 0x15, (_val))
#define MODIFY_PHY54616_EXP_LED_BLINK_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f, 0x15, \
                                0x06, (_val), (_mask))

/* Operating Mode Status Register (Addr 42h) */
#define READ_PHY54616_EXP_OPT_MODE_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f42, 0x15, (_val))
#define WRITE_PHY54616_EXP_OPT_MODE_STATr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f42, 0x15, (_val))
#define MODIFY_PHY54616_EXP_OPT_MODE_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f42, 0x15, \
                                (_val), (_mask))

/* Operating Mode Status Register (Addr 44h) */
#define READ_PHY54616_EXP_SGMII_REC_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f44, 0x15, (_val))
#define WRITE_PHY54616_EXP_SGMII_REC_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f44, 0x15, (_val))
#define MODIFY_PHY54616_EXP_SGMII_REC_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f44, 0x15, \
                                (_val), (_mask))

/* SerDes/SGMII Control Register (Addr 52h) */ 
#define READ_PHY54616_EXP_SERDES_SGMII_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f52, 0x15, (_val))
#define WRITE_PHY54616_EXP_SERDES_SGMII_CTRLr(_unit, _phy_ctrl, _val) \
            PHY54616_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f52, 0x15, (_val))
#define MODIFY_PHY54616_EXP_SERDES_SGMII_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY54616_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f52, 0x15, \
                               (_val), (_mask))

/* 100FX full-duplex bit */
#define PHY_54616_2ND_SERDES_100FX_FD  (1 << 5)
#define PHY_54616_PRIMARY_SERDES_100FX_FD  (1 << 1) 
#endif /* _PHY54616_H_ */
