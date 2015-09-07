/*
 * $Id: phy522x.h 1.14 Broadcom SDK $
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
 * File:        phy54xx.h
 * Purpose:     
 */

#ifndef   _PHY522X_H_
#define   _PHY522X_H_

/* If application want to use this header file for accessing PHY registers,
 * simply redefine PHY522X_REG macros to use BCM API as follows.
 *
 * #define PHY522X_REG_READ(_unit, _port, _flags, _reg_bank, \
 *                               _reg_addr, _val) \
 *            bcm_port_phy_get((_unit), (_port), 0,
 *                 BCM_PORT_PHY_REG_INDIRECT_ADDR(_flags, _reg_bank, _reg_addr),
 *                             (_val))
 *
 * #define PHY522X_REG_WRITE(_unit, _port, _flags, _reg_bank, \
 *                                _reg_addr, _val) \
 *            bcm_port_phy_set((_unit), (_port), 0,
 *                 BCM_PORT_PHY_REG_INDIRECT_ADDR(_flags, _reg_bank, _reg_addr),
 *                             (_val))
 *
 * #define PHY522X_REG_MODIFY(_unit, _port, _flags, _reg_bank, \
 *                                 _reg_addr, _val)\
 *            do { \
 *                return BCM_E_UNAVAIL; \
 *            } while(0)
 */

/* PHY register access */
#define PHY522X_REG_READ(_unit, _phy_ctrl, _reg_bank, _reg_addr, _val) \
            phy_reg_fe_read((_unit), (_phy_ctrl), (_reg_bank), \
                            (_reg_addr), (_val))
#define PHY522X_REG_WRITE(_unit, _phy_ctrl, _reg_bank, _reg_addr, _val) \
            phy_reg_fe_write((_unit), (_phy_ctrl), (_reg_bank), \
                            (_reg_addr), (_val))
#define PHY522X_REG_MODIFY(_unit, _phy_ctrl, _reg_bank, \
                          _reg_addr, _val, _mask) \
            phy_reg_fe_modify((_unit), (_phy_ctrl), (_reg_bank), (_reg_addr), \
                                 (_val), (_mask))


/* 100BASE-TX/10BASE-T MII Control Register (Addr 00h) */
#define READ_PHY522X_MII_CTRLr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x00, (_val))
#define WRITE_PHY522X_MII_CTRLr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x00, (_val))
#define MODIFY_PHY522X_MII_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x00, \
                               (_val), (_mask))

/* 100BASE-TX/10BASE-T MII Status Register (ADDR 01h) */
#define READ_PHY522X_MII_STATr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x01, (_val))
#define WRITE_PHY522X_MII_STATr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x01, (_val))
#define MODIFY_PHY522X_MII_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x01, \
                               (_val), (_mask))

/* 100BASE-TX/10BASE-T PHY Identifier Register (ADDR 02h) */
#define READ_PHY522X_MII_PHY_ID0r(_unit, _phy_ctrl, _val) \
            PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x02, (_val))
#define WRITE_PHY522X_MII_PHY_ID0r(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x02, (_val))
#define MODIFY_PHY522X_MII_PHY_ID0r(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x02, \
                             (_val), (_mask))

/* 100BASE-TX/10BASE-T PHY Identifier Register (ADDR 03h) */
#define READ_PHY522X_MII_PHY_ID1r(_unit, _phy_ctrl, _val) \
            PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x03, (_val))
#define WRITE_PHY522X_MII_PHY_ID1r(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x03, (_val))
#define MODIFY_PHY522X_MII_PHY_ID1r(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x03, \
                             (_val), (_mask))

/* 100BASE-TX/10BASE-T Auto-neg Advertisment Register (ADDR 04h) */
#define READ_PHY522X_MII_ANAr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x04, (_val)
#define WRITE_PHY522X_MII_ANAr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x04, (_val)
#define MODIFY_PHY522X_MII_ANAr(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x04, \
                             (_val), (_mask))

/* 100BASE-TX/10BASE-T Auto-neg Link Partner Ability (ADDR 05h) */
#define READ_PHY522X_MII_ANPr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x05, (_val))
#define WRITE_PHY522X_MII_ANPr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x05, (_val))
#define MODIFY_PHY522X_MII_ANPr(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x05, \
                             (_val), (_mask))

/* 100BASE-TX/10BASE-T Auto-neg Expansion Register (ADDR 06h) */
#define READ_PHY522X_MII_AN_EXPr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x06, (_val))
#define WRITE_PHY522X_MII_AN_EXPr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x06, (_val))
#define MODIFY_PHY522X_MII_AN_EXPr(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x06, \
                             (_val), (_mask))

/* 100BASE-TX/10BASE-T Next Page Transmit Register (ADDR 07h) */
/* 100BASE-TX/10BASE-T Link Partner Received Next Page (ADDR 08h) */

/* 100BASE-X Auxiliary Control Register (Addr 10h) */
#define READ_PHY522X_AUX_CTRLr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x10, (_val))
#define WRITE_PHY522X_AUX_CTRLr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x10, (_val))
#define MODIFY_PHY522X_AUX_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x10, \
                             (_val), (_mask))

/* Misc Control Register (Shadow Addr 10h) */
#define READ_PHY522X_MISC_CTRLr(_unit, _phy_ctrl, _val) \
             PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0001, 0x10, (_val))
#define WRITE_PHY522X_MISC_CTRLr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0001, 0x10 (_val))
#define MODIFY_PHY522X_MISC_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0001, 0x10, \
                             (_val), (_mask))

/* 100BASE-X Auxiliary Status Register (ADDR 11h) */
#define READ_PHY522X_AUX_STATr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x11, (_val))
#define WRITE_PHY522X_AUX_STATr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x11, (_val))
#define MODIFY_PHY522X_AUX_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x11, \
                             (_val), (_mask))

/* 100BASE-X Receiver Error Counter Register (Addr 12h) */
#define READ_PHY522X_RECV_ERR_COUNTERr(_unit, _phy_ctrl, _val) \
             PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x12, (_val))
#define WRITE_PHY522X_RECV_ERR_COUNTERr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x12, (_val))
#define MODIFY_PHY522X_RECV_ERR_COUNTERr(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x12, \
                             (_val), (_mask))

/* 100BASE-X False Carrier Sense Counter (Addr 13h) */

/* 100BASE-X Disconnect Counter (Addr 14h) */

/* Ptest Register (Addr 17h) */

/* Auxiliary Control/Status Register (Addr 18h) */

/* Auxiliary Status Summary Register (Addr 19h) */

/* Interrupt Register (Addr 1ah) */

/* Auxiliary Mode 4 Register (Shadow Addr 1ah) */
#define READ_PHY522X_AUX_MODE4r(_unit, _phy_ctrl, _val) \
             PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0001, 0x1a, (_val))
#define WRITE_PHY522X_AUX_MODE4r(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0001, 0x1a, (_val))
#define MODIFY_PHY522X_AUX_MODE4r(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0001, 0x1a, \
                             (_val), (_mask))

/* Auxiliary Mode 2 Register (Addr 1bh) */
#define READ_PHY522X_AUX_MODE2r(_unit, _phy_ctrl, _val) \
             PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x1b, (_val))
#define WRITE_PHY522X_AUX_MODE2r(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x1b, (_val))
#define MODIFY_PHY522X_AUX_MODE2r(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x1b, \
                             (_val), (_mask))

/* Auxiliary Status 2 Register (Shadow Addr 1bh) */
#define READ_PHY522X_AUX_STAT2r(_unit, _phy_ctrl, _val) \
             PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0001, 0x1B, (_val))
#define WRITE_PHY522X_AUX_STAT2r(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0001, 0x1B, (_val))
#define MODIFY_PHY522X_AUX_STAT2r(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0001, 0x1B, \
                             (_val), (_mask))

#define PHY_522X_AUTO_PWRDWN_EN              (1 << 5) 
#define PHY_522X_AUTO_PWRDWN_WAKEUP_MASK     (0xF << 0) 
#define PHY_522X_AUTO_PWRDWN_WAKEUP_UNIT     40   /* ms */ 
#define PHY_522X_AUTO_PWRDWN_WAKEUP_MAX      600  /* ms */ 
#define PHY_522X_AUTO_PWRDWN_SLEEP_MASK      (1 << 4)
#define PHY_522X_AUTO_PWRDWN_SLEEP_MAX       5000 /* ms */ 

/* Auxiliary Error & General Status Register (Addr 1ch) */
#define READ_PHY522X_AUX_ERR_GENERAL_STATr(_unit, _phy_ctrl, _val) \
             PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x1c, (_val))
#define WRITE_PHY522X__AUX_ERR_GENERAL_STATr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x1c (_val))
#define MODIFY_PHY522X_AUX_ERR_GENERAL_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x1c, \
                             (_val), (_mask))

/* Auxiliary Status Register (Shadow Addr 1ch) */

/* Auxiliary Mode Register (Addr 1dh) */

/* Auxiliary Mode 3 Register (Shadow Addr 1dh) */
#define READ_PHY522X_AUX_MODE3r(_unit, _phy_ctrl, _val) \
             PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0001, 0x1d, (_val))
#define WRITE_PHY522X__AUX_MODE3r(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0001, 0x1d (_val))
#define MODIFY_PHY522X_AUX_MODE3r(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0001, 0x1d, \
                             (_val), (_mask))

/* Auxiliary Multiple PHY Register (Addr 1eh) */

#define READ_PHY522X_AUX_MULTIPLE_PHYr(_unit, _phy_ctrl, _val) \
             PHY522X_REG_READ((_unit), (_phy_ctrl), 0x0000, 0x1e, (_val))
#define WRITE_PHY522X_AUX_MULTIPLE_PHYr(_unit, _phy_ctrl, _val) \
            PHY522X_REG_WRITE((_unit), (_phy_ctrl), 0x0000, 0x1e (_val))
#define MODIFY_PHY522X_AUX_MULTIPLE_PHYr(_unit, _phy_ctrl, _val, _mask) \
            PHY522X_REG_MODIFY((_unit), (_phy_ctrl), 0x0000, 0x1e, \
                             (_val), (_mask))
#define PHY522X_SUPER_ISOLATE_MODE    (1<<3)

/* Auxiliary Status 4 Register (Shadow Addr 1eh) */

/* Broadcom Test (Addr 1fh) */
 
#endif /* _PHY522X_H_ */
