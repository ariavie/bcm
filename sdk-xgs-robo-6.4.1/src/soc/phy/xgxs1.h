/*
 * $Id: xgxs1.h,v 1.3 Broadcom SDK $
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
 * File:        xgxs.h
 * Purpose:     10G 802.3ae Ethernet PHY
 */

#ifndef   _PHY_XGXS_H_
#define   _PHY_XGXS_H_

/* 10Gig XGMII/Serdes PHY Register definitions
 * From Broadcom XGXS Core Documentation
 */

#define XGXS_IEEE_CTRL_SW_RST  (1 << 15) /* S/W Reset */
#define XGXS_IEEE_CTRL_LB_ENA  (1 << 14) /* XGXS Local Loopback enable */
#define XGXS_IEEE_CTRL_SW_PD   (1 << 13) /* S/W Powerdown */

#define XGXS_BLOCK_REG_SHIFT   0x03  /* Bits 4:9 of register selects blk*/


#define XGXS_BLOCK_0_ASI      0x00  /* Standard, default regs above */
#define XGXS_BLOCK_1_ASI      0x01  
#define XGXS_BLOCK_2_ASI      0x02  
#define XGXS_BLOCK_3_ASI      0x03  
#define XGXS_BLOCK_4_ASI      0x04

/* Bit defines for ANA register */
#define XGXS_ANA_HD          (1 << 6)
#define XGXS_ANA_FD          (1 << 5)
#define XGXS_ANA_PAUSE_NONE  (0 << 7)
#define XGXS_ANA_PAUSE_SYM   (1 << 7)
#define XGXS_ANA_PAUSE_ASYM  (1 << 8)
#define XGXS_ANA_PAUSE_MASK  (3 << 7)

/* Auto-Negotiation Link-Partner Ability Register */
#define XGXS_ANP_NEXT_PG     (1 << 15)
#define XGXS_ANP_ACK         (1 << 14)
#define XGXS_ANP_RF_SHFT     (12)  /* Remote fault */
#define XGXS_ANP_RF_MASK     (0x3000)
#define XGXS_ANP_PAUSE_ASYM  (1 << 8)
#define XGXS_ANP_PAUSE_SYM   (1 << 7)
#define XGXS_ANP_HD          (1 << 6)
#define XGXS_ANP_FD          (1 << 5)


/* If application want to use this header file for accessing PHY registers,
 * simply redefine PHYXGXS_REG macro to use BCM API as follows.
 *
 * #define SOC_PHY_REG_1000X  BCM_PORT_PHY_REG_1000X  
 *  
 * #define PHYXGXS_REG_READ(_unit, _port, _flags, _reg_bank, _reg_addr, _val) \
 *            bcm_port_phy_get((_unit), (_port), 0, 
 *                 BCM_PORT_PHY_REG_INDIRECT_ADDR(_flags, _reg_bank, _reg_addr),
 *                             (_val))
 *
 * #define PHYXGXS_REG_WRITE(_unit, _port, _flags, _reg_bank, _reg_addr, _val) \
 *            bcm_port_phy_set((_unit), (_port), 0, 
 *                 BCM_PORT_PHY_REG_INDIRECT_ADDR(_flags, _reg_bank, _reg_addr),
 *                             (_val))
 *
 * #define PHYXGXS_REG_MODIFY(_unit, _port, _flags, _reg_bank, _reg_addr, _val)\
 *            do { \
 *                return BCM_E_UNAVAIL; \
 *            } while(0)
 */

/* XGXS register access */
#define PHYXGXS_REG_READ(_unit, _phy_ctrl, _flags, _reg_bank, \
                          _reg_addr, _val) \
            phy_reg_xgxs_read((_unit), (_phy_ctrl), (_reg_bank), \
                            (_reg_addr), (_val))
#define PHYXGXS_REG_WRITE(_unit, _phy_ctrl, _flags, _reg_bank, \
                          _reg_addr, _val) \
            phy_reg_xgxs_write((_unit), (_phy_ctrl), (_reg_bank), \
                             (_reg_addr), (_val))
#define PHYXGXS_REG_MODIFY(_unit, _phy_ctrl, _flags, _reg_bank, \
                            _reg_addr, _val, _mask) \
            phy_reg_xgxs_modify((_unit), (_phy_ctrl), (_reg_bank), \
                              (_reg_addr), (_val), (_mask))

/****************************************/
/* XAUI IEEE PMA/PMD Block (blk 0x0000) */
/****************************************/

/* XAUI MII Control Register (Addr 00h) */
#define READ_PHYXGXS_XAUI_MII_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x00, (_val))
#define WRITE_PHYXGXS_XAUI_MII_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x00, (_val))
#define MODIFY_PHYXGXS_XAUI_MII_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x00, \
                               (_val), (_mask))

/* XAUI MII Status Register (ADDR 01h) */
#define READ_PHYXGXS_XAUI_MII_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x01, (_val))
#define WRITE_PHYXGXS_XAUI_MII_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x01, (_val))
#define MODIFY_PHYXGXS_XAUI_MII_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit, _phy_ctrl), 0x00, 0x0000, 0x01, \
                               (_val), (_mask))

/* XAUI PHY Identifier Register (ADDR 02h) */
#define READ_PHYXGXS_XAUI_MII_PHY_ID0r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x02, (_val))
#define WRITE_PHYXGXS_XAUI_MII_PHY_ID0r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x02, (_val))
#define MODIFY_PHYXGXS_XAUI_MII_PHY_ID0r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit, _phy_ctrl), 0x00, 0x0000, 0x02, \
                             (_val), (_mask))

/* XAUI PHY Identifier Register (ADDR 03h) */
#define READ_PHYXGXS_XAUI_MII_PHY_ID1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x03, (_val))
#define WRITE_PHYXGXS_XAUI_MII_PHY_ID1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x03, (_val))
#define MODIFY_PHYXGXS_XAUI_MII_PHY_ID1r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x03, \
                             (_val), (_mask))

/* IEEE Speed Ability (Addr 04h) */
/* IEEE Dev In Package 1 (Addr 05h) */
/* IEEE Dev In Package 2 (Addr 06h) */
/* IEEE Control 2 (Addr 07h) */
/* IEEE Status 2 (Addr 08h) */

/* IEEE Tx Disable (Addr 09h) */
#define READ_PHYXGXS_XAUI_TX_DISABLEr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0x09, (_val))
#define WRITE_PHYXGXS_XAUI_TX_DISABLEr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0x09, (_val))
#define MODIFY_PHYXGXS_XAUI_TX_DISABLEr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0x09, \
                             (_val), (_mask))

/* IEEE RxSigDet (Addr 0Ah) */
/* IEEE PMDExtAbil (Addr 0Bh) */
/* Reserved (Addr 0Ch-0Eh) */

/****************************/
/* IEEE1 Block (blk 0x0010) */
/****************************/
/* Reserved (Addr 0x0010 - 0x0017) */

/* Lane Status (Addr 0x0018) */
#define READ_PHYXGXS_IEEE1_LANE_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                              0x0010, 0x08, (_val))
#define WRITE_PHYXGXS_IEEE1_LANE_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                              0x0010, 0x08, (_val))
#define MODIFY_PHYXGXS_IEEE1_LANE_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), SOC_PHY_REG_1000X, \
                              0x0010, 0x08, (_val), (_mask))

/* IEEE Test Control (Addr 0x0019) */
/* Reserved (Addr 0x001A - 0x001E) */

/****************************/
/* XGXS Block 0 (blk 0x8000) */
/****************************/
/* XGXS Control (Addr 0x8000) */
#define READ_PHYXGXS_BLK0_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x10, (_val))
#define WRITE_PHYXGXS_BLK0_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x10, (_val))
#define MODIFY_PHYXGXS_BLK0_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x10, \
                             (_val), (_mask))

/* XGXS Status (Addr 0x8001) */
#define READ_PHYXGXS_BLK0_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x11, (_val))
#define WRITE_PHYXGXS_BLK0_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x11, (_val))
#define MODIFY_PHYXGXS_BLK0_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x11, \
                             (_val), (_mask))

/* XGMII Idle (Addr 0x8002) */
#define READ_PHYXGXS_BLK0_XGMII_IDLEr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x12, (_val))
#define WRITE_PHYXGXS_BLK0_XGMII_IDLEr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x12, (_val))
#define MODIFY_PHYXGXS_BLK0_XGMII_IDLEr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x12, \
                             (_val), (_mask))

/* XGMII Sync (Addr 0x8003) */
#define READ_PHYXGXS_BLK0_XGMII_SYNCr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x13, (_val))
#define WRITE_PHYXGXS_BLK0_XGMII_SYNCr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x13, (_val))
#define MODIFY_PHYXGXS_BLK0_XGMII_SYNCr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x13, \
                             (_val), (_mask))

/* XGMII Skip (Addr 0x8004) */
#define READ_PHYXGXS_BLK0_XGMII_SKIPr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x14, (_val))
#define WRITE_PHYXGXS_BLK0_XGMII_SKIPr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x14, (_val))
#define MODIFY_PHYXGXS_BLK0_XGMII_SKIPr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x14, \
                             (_val), (_mask))

/* XGMII Sop Eop (Addr 0x8005) */
#define READ_PHYXGXS_BLK0_XGMII_SOP_EOPr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x15, (_val))
#define WRITE_PHYXGXS_BLK0_XGMII_SOP_EOPr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x15, (_val))
#define MODIFY_PHYXGXS_BLK0_XGMII_SOP_EOPr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x15, \
                             (_val), (_mask))

/* XGMII Align (Addr 0x8006) */
#define READ_PHYXGXS_BLK0_XGMII_ALIGNr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x16, (_val))
#define WRITE_PHYXGXS_BLK0_XGMII_ALIGNr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x16, (_val))
#define MODIFY_PHYXGXS_BLK0_XGMII_ALIGNr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x16, \
                             (_val), (_mask))

/* XGMII R Control (Addr 0x8007) */
#define READ_PHYXGXS_BLK0_XGMII_RCTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x17, (_val))
#define WRITE_PHYXGXS_BLK0_XGMII_RCTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x17, (_val))
#define MODIFY_PHYXGXS_BLK0_XGMII_RCTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x17, \
                             (_val), (_mask))

/* XGMII T Control (Addr 0x8008) */
#define READ_PHYXGXS_BLK0_XGMII_TCTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x18, (_val))
#define WRITE_PHYXGXS_BLK0_XGMII_TCTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x18, (_val))
#define MODIFY_PHYXGXS_BLK0_XGMII_TCTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x18, \
                             (_val), (_mask))

/* XGMII Swap (Addr 0x8009) */
#define READ_PHYXGXS_BLK0_XGMII_SWAPr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x19, (_val))
#define WRITE_PHYXGXS_BLK0_XGMII_SWAPr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x19, (_val))
#define MODIFY_PHYXGXS_BLK0_XGMII_SWAPr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x19, \
                             (_val), (_mask))

/* IssLsid (Addr 0x800A) */
#define READ_PHYXGXS_BLK0_ISS_LSIDr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1A, (_val))
#define WRITE_PHYXGXS_BLK0_ISS_LSIDr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1A, (_val))
#define MODIFY_PHYXGXS_BLK0_ISS_LSIDr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1A, \
                             (_val), (_mask))

/* IssTinfo (Addr 0x800B) */
#define READ_PHYXGXS_BLK0_ISS_TINFOr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1B, (_val))
#define WRITE_PHYXGXS_BLK0_ISS_TINFOr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1B, (_val))
#define MODIFY_PHYXGXS_BLK0_ISS_TINFOr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1B, \
                             (_val), (_mask))

/* IssRinfo (Addr 0x800C) */
#define READ_PHYXGXS_BLK0_ISS_RINFOr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1C, (_val))
#define WRITE_PHYXGXS_BLK0_ISS_RINFOr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1C, (_val))
#define MODIFY_PHYXGXS_BLK0_ISS_RINFOr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1C, \
                             (_val), (_mask))

/* CX4 SigDet Cnt (Addr 0x800D) */
#define READ_PHYXGXS_BLK0_CX4_SIGDET_COUNTERr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1D, (_val))
#define WRITE_PHYXGXS_BLK0_CX4_SIGDET_COUNTERr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1D, (_val))
#define MODIFY_PHYXGXS_BLK0_CX4_SIGDET_COUNTERr(_unit, _phy_ctrl, _val, \
                                                 _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1D, \
                             (_val), (_mask))

/* Misc Control 1 (Addr 0x800E) */
#define READ_PHYXGXS_BLK0_MISC_CTRL1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1E, (_val))
#define WRITE_PHYXGXS_BLK0_MISC_CTRL1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1E, (_val))
#define MODIFY_PHYXGXS_BLK0_MISC_CTRL1r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8000, 0x1E, \
                             (_val), (_mask))

/*****************************/
/* XGXS Block 1 (blk 0x8010) */
/*****************************/
/* deskew (Addr 0x8010) */
#define READ_PHYXGXS_BLK1_DESKEWr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8010, 0x10, (_val))
#define WRITE_PHYXGXS_BLK1_DESKEWr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8010, 0x10, (_val))
#define MODIFY_PHYXGXS_BLK1_DESKEWr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8010, 0x10, \
                             (_val), (_mask))

/* link (Addr 0x8011) */
#define READ_PHYXGXS_BLK1_LINKr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8010, 0x011, (_val))
#define WRITE_PHYXGXS_BLK1_LINKr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8010, 0x011,(_val))
#define MODIFY_PHYXGXS_BLK1_LINKr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8010, 0x011, \
                             (_val), (_mask))

/* testRx (Addr 0x8012) */
#define READ_PHYXGXS_BLK1_TESTRXr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8010, 0x012, (_val))
#define WRITE_PHYXGXS_BLK1_TESTRXr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8010, 0x012,(_val))
#define MODIFY_PHYXGXS_BLK1_TESTRXr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8010, 0x012, \
                             (_val), (_mask))

/* testTx (Addr 0x8013) */

/* testXg (Addr 0x8014) */

/* Reserved (Addr 0x8015) */

/* laneCtrl1 (Addr 0x8016) */
#define READ_PHYXGXS_BLK1_LANE_CTRL1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8010, 0x16, (_val))
#define WRITE_PHYXGXS_BLK1_LANE_CTRL1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8010, 0x16, (_val))
#define MODIFY_PHYXGXS_BLK1_LANE_CTRL1r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8010, 0x16, \
                             (_val), (_mask))

/* laneCtrl2 (Addr 0x8017) */
#define READ_PHYXGXS_BLK1_LANE_CTRL2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8010, 0x17, (_val))
#define WRITE_PHYXGXS_BLK1_LANE_CTRL2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8010, 0x17, (_val))
#define MODIFY_PHYXGXS_BLK1_LANE_CTRL2r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8010, 0x17, \
                             (_val), (_mask))

/* laneCtrl3 (Addr 0x8018) */
#define READ_PHYXGXS_BLK1_LANE_CTRL3r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8010, 0x18, (_val))
#define WRITE_PHYXGXS_BLK1_LANE_CTRL3r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8010, 0x18, (_val))
#define MODIFY_PHYXGXS_BLK1_LANE_CTRL3r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8010, 0x18, \
                             (_val), (_mask))

/* lanePrbs (Addr 0x8019) */

/* laneTest (Addr 0x801A) */
#define READ_PHYXGXS_BLK1_LANE_TESTr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8010, 0x1A, (_val))
#define WRITE_PHYXGXS_BLK1_LANE_TESTr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8010, 0x1A, (_val))
#define MODIFY_PHYXGXS_BLK1_LANE_TESTr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8010, 0x1A, \
                             (_val), (_mask))

/* IssRevnt (Addr 0x801B) */

/* dskevnt (Addr 0x801C) */

/* Aerrevnt (Addr 0x801D) */

/* chcmpevnt (Addr 0x801E) */
 

/****************************/
/* 10G Tx BERT (blk 0x8020) */
/****************************/


/****************************/
/* 10G Rx BERT (blk 0x8030) */
/****************************/

/*************************/
/* BcstBert (blk 0x8040) */
/*************************/

/********************/
/* PLL (blk 0x8050) */
/********************/

/* PLL STATUS (Addr 0x8050) */
#define READ_PHYXGXS_PLL_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x10, (_val))
#define WRITE_PHYXGXS_PLL_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x10, (_val))
#define MODIFY_PHYXGXS_PLL_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x10, \
                             (_val), (_mask))

/* PLL Control (Addr 0x8051) */
#define READ_PHYXGXS_PLL_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x11, (_val))
#define WRITE_PHYXGXS_PLL_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x11, (_val))
#define MODIFY_PHYXGXS_PLL_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x11, \
                             (_val), (_mask))

/* PLL Timer 1 (Addr 0x8052) */
#define READ_PHYXGXS_PLL_TIMER1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x12, (_val))
#define WRITE_PHYXGXS_PLL_TIMER1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x12, (_val))
#define MODIFY_PHYXGXS_PLL_TIMER1r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x12, \
                             (_val), (_mask))

/* PLL Timer 2 (Addr 0x8053) */
#define READ_PHYXGXS_PLL_TIMER2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x13, (_val))
#define WRITE_PHYXGXS_PLL_TIMER2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x13, (_val))
#define MODIFY_PHYXGXS_PLL_TIMER2r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x13, \
                             (_val), (_mask))

/* PLL Timer 3 (Addr 0x8054) */
#define READ_PHYXGXS_PLL_TIMER3r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x14, (_val))
#define WRITE_PHYXGXS_PLL_TIMER3r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x14, (_val))
#define MODIFY_PHYXGXS_PLL_TIMER3r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x14, \
                             (_val), (_mask))

/* CAP Control (Addr 0x8055) */
#define READ_PHYXGXS_PLL_CAP_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x15, (_val))
#define WRITE_PHYXGXS_PLL_CAP_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x15, (_val))
#define MODIFY_PHYXGXS_PLL_CAP_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x15, \
                             (_val), (_mask))

/* AMP Control (Addr 0x8056) */
#define READ_PHYXGXS_PLL_AMP_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x16, (_val))
#define WRITE_PHYXGXS_PLL_AMP_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x16, (_val))
#define MODIFY_PHYXGXS_PLL_AMP_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x16, \
                             (_val), (_mask))

/* Freq Det Counter (Addr 0x8057) */
#define READ_PHYXGXS_PLL_FREQ_DET_COUNTERr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x17, (_val))
#define WRITE_PHYXGXS_PLL_FREQ_DET_COUNTERr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x17, (_val))
#define MODIFY_PHYXGXS_PLL_FREQ_DET_COUNTERr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x17, \
                             (_val), (_mask))

/* PLL Astatus 1 (Addr 0x8058) */
#define READ_PHYXGXS_PLL_ASTAT1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x18, (_val))
#define WRITE_PHYXGXS_PLL_ASTAT1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x18, (_val))
#define MODIFY_PHYXGXS_PLL_ASTAT1r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x18, \
                             (_val), (_mask))

/* PLL Astatus 2 (Addr 0x8059) */
#define READ_PHYXGXS_PLL_ASTAT2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x19, (_val))
#define WRITE_PHYXGXS_PLL_ASTAT2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x19, (_val))
#define MODIFY_PHYXGXS_PLL_ASTAT2r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x19, \
                             (_val), (_mask))

/* PLL Clock Gen (Addr 0x805A) */
#define READ_PHYXGXS_PLL_CLK_GENr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1A, (_val))
#define WRITE_PHYXGXS_PLL_CLK_GENr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1A, (_val))
#define MODIFY_PHYXGXS_PLL_CLK_GENr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1A, \
                             (_val), (_mask))

/* PLL Ndiv (Addr 0x805B) */
#define READ_PHYXGXS_PLL_NDIVr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1B, (_val))
#define WRITE_PHYXGXS_PLL_NDIVr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1B, (_val))
#define MODIFY_PHYXGXS_PLL_NDIVr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1B, \
                             (_val), (_mask))

/* PLL Bglqp (Addr 0x805C) */
#define READ_PHYXGXS_PLL_BGLQPr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1C, (_val))
#define WRITE_PHYXGXS_PLL_BGLQP(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1C, (_val))
#define MODIFY_PHYXGXS_PLL_BGLQPr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1C, \
                             (_val), (_mask))

/* PLL Test Kvh (Addr 0x805D) */
#define READ_PHYXGXS_PLL_TEST_KVHr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1D, (_val))
#define WRITE_PHYXGXS_PLL_TEST_KVHr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1D, (_val))
#define MODIFY_PHYXGXS_PLL_TEST_KVHr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1D, \
                             (_val), (_mask))

/* PLL AControl 5 (Addr 0x805E) */
#define READ_PHYXGXS_PLL_ACTRL5r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1E, (_val))
#define WRITE_PHYXGXS_PLL_ACTRL5r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1E, (_val))
#define MODIFY_PHYXGXS_PLL_ACTRL5r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8050, 0x1E, \
                             (_val), (_mask))


/********************/
/* TX0 (blk 0x8060) */
/********************/

/********************/
/* TX1 (blk 0x8070) */
/********************/


/********************/
/* TX2 (blk 0x8080) */
/********************/


/********************/
/* TX3 (blk 0x8090) */
/********************/


/********************/
/* TX All (blk 0x80A0) */
/********************/
/* Tx AStatus 0 (Addr 0x80A0) */
/* Tx AControl 0 (Addr 0x80A1) */
#define READ_PHYXGXS_TXALL_ACTRL0r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80A0, 0x11, (_val))
#define WRITE_PHYXGXS_TXALL_ACTRL0r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80A0, 0x11, (_val))
#define MODIFY_PHYXGXS_TXALL_ACTRL0r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80A0, 0x11, \
                             (_val), (_mask))

/* Tx mdata 0 (Addr 0x80A2) */
/* Tx mdata 1 (Addr 0x80A3) */
/* Tx AStatus 1 (Addr 0x80A4) */
/* Tx BgVcm (Addr 0x80A5) */
/* Tx Ibuff 1T2T (Addr 0x80A6) */

/* Tx Driver (Addr 0x80A7) */
#define READ_PHYXGXS_TXALL_DRIVERr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80A0, 0x17, (_val))
#define WRITE_PHYXGXS_TXALL_DRIVERr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80A0, 0x17, (_val))
#define MODIFY_PHYXGXS_TXALL_DRIVERr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80A0, 0x17, \
                             (_val), (_mask))

/* Reserved (Addr 0x80A8 - 0x80AE) */

/********************/
/* RX0 (blk 0x80B0) */
/********************/


/********************/
/* RX1 (blk 0x80C0) */
/********************/


/********************/
/* RX2 (blk 0x80D0) */
/********************/


/********************/
/* RX3 (blk 0x80E0) */
/********************/


/***********************/
/* RX All (blk 0x80F0) */
/***********************/
/* Rx Status (Address 0x80F0) */
#define READ_PHYXGXS_RXALL_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x10, (_val))
#define WRITE_PHYXGXS_RXALL_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x10, (_val))
#define MODIFY_PHYXGXS_RXALL_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x10, \
                             (_val), (_mask))

/* Rx Control (Address 0x80F1) */
#define READ_PHYXGXS_RXALL_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x11, (_val))
#define WRITE_PHYXGXS_RXALL_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x11, (_val))
#define MODIFY_PHYXGXS_RXALL_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x11, \
                             (_val), (_mask))

/* Rx Timer 1 (Address 0x80F2) */
#define READ_PHYXGXS_RXALL_TIMER1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x12, (_val))
#define WRITE_PHYXGXS_RXALL_TIMER1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x12, (_val))
#define MODIFY_PHYXGXS_RXALL_TIMER1r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x12, \
                             (_val), (_mask))

/* Rx Timer 2 (Address 0x80F3) */
#define READ_PHYXGXS_RXALL_TIMER2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x13, (_val))
#define WRITE_PHYXGXS_RXALL_TIMER2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x13, (_val))
#define MODIFY_PHYXGXS_RXALL_TIMER2r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x13, \
                             (_val), (_mask))

/* Rx SigDet (Address 0x80F4) */
#define READ_PHYXGXS_RXALL_SIG_DETr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x14, (_val))
#define WRITE_PHYXGXS_RXALL_SIG_DETr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x14, (_val))
#define MODIFY_PHYXGXS_RXALL_SIG_DETr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x14, \
                             (_val), (_mask))

/* Rx CdrPhase (Address 0x80F5) */
#define READ_PHYXGXS_RXALL_CDR_PHASEr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x15, (_val))
#define WRITE_PHYXGXS_RXALL_CDR_PHASEr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x15, (_val))
#define MODIFY_PHYXGXS_RXALL_CDR_PHASEr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x15, \
                             (_val), (_mask))

/* Rx CdrFreq (Address 0x80F6) */
#define READ_PHYXGXS_RXALL_CDR_FREQr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x16, (_val))
#define WRITE_PHYXGXS_RXALL_CDR_FREQr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x16, (_val))
#define MODIFY_PHYXGXS_RXALL_CDR_FREQr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x16, \
                             (_val), (_mask))

/* Rx EqConfig (Address 0x80F7) */
#define READ_PHYXGXS_RXALL_EQ_CONFIGr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x17, (_val))
#define WRITE_PHYXGXS_RXALL_EQ_CONFIGr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x17, (_val))
#define MODIFY_PHYXGXS_RXALL_EQ_CONFIGr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x17, \
                             (_val), (_mask))

/* Rx EqForce (Address 0x80F8) */
#define READ_PHYXGXS_RXALL_EQ_FORCEr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x18, (_val))
#define WRITE_PHYXGXS_RXALL_EQ_FORCEr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x18, (_val))
#define MODIFY_PHYXGXS_RXALL_EQ_FORCEr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x18, \
                             (_val), (_mask))

/* Rx Control 1G (Address 0x80F9) */
#define READ_PHYXGXS_RXALL_CTRL_1Gr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x19, (_val))
#define WRITE_PHYXGXS_RXALL_CTRL_1Gr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x19, (_val))
#define MODIFY_PHYXGXS_RXALL_CTRL_1Gr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x19, \
                             (_val), (_mask))

/* Rx Control PCIE (Address 0x80FA) */
#define READ_PHYXGXS_RXALL_CTRL_PCIEr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1A, (_val))
#define WRITE_PHYXGXS_RXALL_CTRL_PCIEr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1A, (_val))
#define MODIFY_PHYXGXS_RXALL_CTRL_PCIEr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1A, \
                             (_val), (_mask))

/* Rx Astatus (Address 0x80FB) */
#define READ_PHYXGXS_RXALL_ASTATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1B, (_val))
#define WRITE_PHYXGXS_RXALL_ASTATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1B, (_val))
#define MODIFY_PHYXGXS_RXALL_ASTATr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1B, \
                             (_val), (_mask))

/* Rx Eq Boost (Address 0x80FC) */
#define READ_PHYXGXS_RXALL_EQ_BOOSTr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1C, (_val))
#define WRITE_PHYXGXS_RXALL_EQ_BOOSTr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1C, (_val))
#define MODIFY_PHYXGXS_RXALL_EQ_BOOSTr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1C, \
                             (_val), (_mask))

/* Rx lb Data Eq (Address 0x80FD) */
#define READ_PHYXGXS_RXALL_LB_DATA_EQr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1D, (_val))
#define WRITE_PHYXGXS_RXALL_LB_DATA_EQr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1D, (_val))
#define MODIFY_PHYXGXS_RXALL_LB_DATA_EQr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1D, \
                             (_val), (_mask))

/* Rx lb ADC Buff (Address 0x80FE) */
#define READ_PHYXGXS_RXALL_LB_ADC_BUFFr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1E, (_val))
#define WRITE_PHYXGXS_RXALL_LB_ADC_BUFFr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1E, (_val))
#define MODIFY_PHYXGXS_RXALL_LB_ADC_BUFFr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x80F0, 0x1E, \
                             (_val), (_mask))

/*****************************/
/* XGXS Block 2 (blk 0x8100) */
/*****************************/
/* Rx Lane Swap (Addr 0x8100) */
/* Tx Lane Swap (Addr 0x8101) */
#define READ_PHYXGXS_BLK2_TX_LANE_SWAPr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8100, 0x11, (_val))
#define WRITE_PHYXGXS_BLK2_TX_LANE_SWAPr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8100, 0x11, (_val))
#define MODIFY_PHYXGXS_BLK2_TX_LANE_SWAPr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8100, 0x11, \
                             (_val), (_mask))

/* QsetLns01 (Addr 0x8102) */
/* QsetLns23 (Addr 0x8103) */ 
/* Unicore Mode 10G (Addr 0x8104) */
/* Reserved (Addr 0x8105 - 0x810E) */

/*****************************/
/* In Band MDIO (blk 0x8110) */
/*****************************/


/**************************/
/* GP Status (blk 0x8120) */
/**************************/


/*******************************/
/* SerDes Digital (blk 0x8300) */
/*******************************/

/* 1000XControl1 (Addr 0x8300) */
#define READ_PHYXGXS_SERDES_DIGITAL_1000X_CTRL1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8300, 0x10, (_val))
#define WRITE_PHYXGXS_SERDES_DIGITAL_1000X_CTRL1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8300, 0x10, (_val))
#define MODIFY_PHYXGXS_SERDES_DIGITAL_1000X_CTRL1r(_unit, _phy_ctrl, \
                                                    _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8300, 0x10, \
                             (_val), (_mask))

/* 1000XControl2 (Addr 0x8301) */
#define READ_PHYXGXS_SERDES_DIGITAL_1000X_CTRL2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8300, 0x11, (_val))
#define WRITE_PHYXGXS_SERDES_DIGITAL_1000X_CTRL2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8300, 0x11, (_val))
#define MODIFY_PHYXGXS_SERDES_DIGITAL_1000X_CTRL2r(_unit, _phy_ctrl, \
                                                    _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8300, 0x11, \
                             (_val), (_mask))

/* 1000XControl3 (Addr 0x8302) */
#define READ_PHYXGXS_SERDES_DIGITAL_1000X_CTRL3r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8300, 0x12, (_val))
#define WRITE_PHYXGXS_SERDES_DIGITAL_1000X_CTRL3r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8300, 0x12, (_val))
#define MODIFY_PHYXGXS_SERDES_DIGITAL_1000X_CTRL3r(_unit, _phy_ctrl, \
                                                    _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8300, 0x12, \
                             (_val), (_mask))

/* 1000XControl4 (Addr 0x8303) */
#define READ_PHYXGXS_SERDES_DIGITAL_1000X_CTRL4r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8300, 0x13, (_val))
#define WRITE_PHYXGXS_SERDES_DIGITAL_1000X_CTRL4r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8300, 0x13, (_val))
#define MODIFY_PHYXGXS_SERDES_DIGITAL_1000X_CTRL4r(_unit, _phy_ctrl, \
                                                    _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8300, 0x13, \
                             (_val), (_mask))

/* 1000XStatus1 (Addr 0x8304) */
#define READ_PHYXGXS_SERDES_DIGITAL_1000X_STAT1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8300, 0x14, (_val))
#define WRITE_PHYXGXS_SERDES_DIGITAL_1000X_STAT1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8300, 0x14, (_val))
#define MODIFY_PHYXGXS_SERDES_DIGITAL_1000X_STAT1r(_unit, _phy_ctrl, \
                                                    _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8300, 0x14, \
                             (_val), (_mask))

/* 1000XStatus2 (Addr 0x8305) */
#define READ_PHYXGXS_SERDES_DIGITAL_1000X_STAT2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8300, 0x15, (_val))
#define WRITE_PHYXGXS_SERDES_DIGITAL_1000X_STAT2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8300, 0x15, (_val))
#define MODIFY_PHYXGXS_SERDES_DIGITAL_1000X_STAT2r(_unit, _phy_ctrl, \
                                                    _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8300, 0x15, \
                             (_val), (_mask))

/* 1000XStatus3 (Addr 0x8306) */
#define READ_PHYXGXS_SERDES_DIGITAL_1000X_STAT3r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8300, 0x16, (_val))
#define WRITE_PHYXGXS_SERDES_DIGITAL_1000X_STAT3r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8300, 0x16, (_val))
#define MODIFY_PHYXGXS_SERDES_DIGITAL_1000X_STAT3r(_unit, _phy_ctrl, \
                                                    _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8300, 0x16, \
                             (_val), (_mask))

/* CrcErr_RxPktCntr (Addr 0x8307) */

/* Misc1 (Addr 0x8308) */
#define READ_PHYXGXS_SERDES_DIGITAL_MISC1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8300, 0x18, (_val))
#define WRITE_PHYXGXS_SERDES_DIGITAL_MISC1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8300, 0x18, (_val))
#define MODIFY_PHYXGXS_SERDES_DIGITAL_MISC1r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8300, 0x18, \
                             (_val), (_mask))

/* Misc2 (Addr 0x8309) */
#define READ_PHYXGXS_SERDES_DIGITAL_MISC2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8300, 0x19, (_val))
#define WRITE_PHYXGXS_SERDES_DIGITAL_MISC2r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8300, 0x19, (_val))
#define MODIFY_PHYXGXS_SERDES_DIGITAL_MISC2r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8300, 0x19, \
                             (_val), (_mask))

/* PatGenControl (Addr 0x830A) */
/* PatGenControl (Addr 0x830B) */
/* TestMode (Addr 0x830C) */
/* TxPktCnt (Addr 0x830D) */
/* RxPktCnt (Addr 0x830E) */

/*********************/
/* Test (blk 0x8310) */
/*********************/

/************************/
/* Over 1G (blk 0x8320) */
/************************/
/* digctrl_3_0 (Addr 0x8320) */
/* digctrl_3_1 (Addr 0x8321) */
/* digctrl_3_2 (Addr 0x8322) */
/* digctrl_3_3 (Addr 0x8323) */
/* digctrl_3_4 (Addr 0x8324) */
/* digctrl_3_5 (Addr 0x8325) */
/* digctrl_3_6 (Addr 0x8326) */
/* TPOUT 1 (Addr 0x8327) */
/* TPOUT 2 (Addr 0x8328) */

/* UP 1 (Addr 0x8329) */
#define READ_PHYXGXS_OVER1G_UP1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8320, 0x19, (_val))
#define WRITE_PHYXGXS_OVER1G_UP1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8320, 0x19, (_val))
#define MODIFY_PHYXGXS_OVER1G_UP1r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8320, 0x19, \
                             (_val), (_mask))

/* UP 2 (Addr 0x832A) */
/* UP 3 (Addr 0x832B) */

/* LP UP 1 (Addr 0x832C) */
#define READ_PHYXGXS_OVER1G_LP_UP1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8320, 0x1C, (_val))
#define WRITE_PHYXGXS_OVER1G_LP_UP1r(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8320, 0x1C, (_val))
#define MODIFY_PHYXGXS_OVER1G_LP_UP1r(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8320, 0x1C, \
                             (_val), (_mask))

/* LP UP 2 (Addr 0x832D) */
/* LP UP 3  (Addr 0x832E) */

/***************************/
/* Remote PHY (blk 0x8330) */
/***************************/

/************************************/
/* 10G Parallel Detect (blk 0x8340) */
/************************************/
/* parDet10GStatus (Addr 0x8340) */
#define READ_PHYXGXS_PARDET10G_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8340, 0x10, (_val))
#define WRITE_PHYXGXS_PARDET10G_STATr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8340, 0x10, (_val))
#define MODIFY_PHYXGXS_PARDET10G_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8340, 0x10, \
                             (_val), (_mask))

/* parDet10GControl (Addr 0x8341) */
#define READ_PHYXGXS_PARDET10G_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8340, 0x11, (_val))
#define WRITE_PHYXGXS_PARDET10G_CTRLr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8340, 0x11, (_val))
#define MODIFY_PHYXGXS_PARDET10G_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8340, 0x11, \
                             (_val), (_mask))

/* parDet10GSigDet (Addr 0x8342) */
#define READ_PHYXGXS_PARDET10G_SIGDETr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8340, 0x12, (_val))
#define WRITE_PHYXGXS_PARDET10G_SIGDETr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8340, 0x12, (_val))
#define MODIFY_PHYXGXS_PARDET10G_SIGDETr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8340, 0x12, \
                             (_val), (_mask))

/* parDet10GLink (Addr 0x8343) */
#define READ_PHYXGXS_PARDET10G_LINKr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8340, 0x13, (_val))
#define WRITE_PHYXGXS_PARDET10G_LINKr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8340, 0x13, (_val))
#define MODIFY_PHYXGXS_PARDET10G_LINKr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8340, 0x13, \
                             (_val), (_mask))

/* parDet10GLosLink (Addr 0x8344) */
#define READ_PHYXGXS_PARDET10G_LOSLINKr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_READ((_unit), (_phy_ctrl), 0x00, 0x8340, 0x14, (_val))
#define WRITE_PHYXGXS_PARDET10G_LOSLINKr(_unit, _phy_ctrl, _val) \
            PHYXGXS_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x8340, 0x14, (_val))
#define MODIFY_PHYXGXS_PARDET10G_LOSLINKr(_unit, _phy_ctrl, _val, _mask) \
            PHYXGXS_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x8340, 0x14, \
                             (_val), (_mask))

/* Reserved (Addr 0x8345 - 0x834E) */

/******************************/
/* BAM Next Page (blk 0x8350) */
/******************************/

#endif /* _PHY_XGXS_H_ */
