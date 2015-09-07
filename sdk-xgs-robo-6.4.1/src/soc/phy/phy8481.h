/*
 * $Id: phy8481.h,v 1.57 Broadcom SDK $
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
 * File:        phy8481.h
 *
 */

#ifndef   _PHY8481_H_
#define   _PHY8481_H_

#include <soc/phy.h>

#define MII_CTRL_PMA_LOOPBACK      (1 << 0)

#define PHY8481_C45_DEV_PMAD           0x01
#define PHY8481_C45_DEV_PCS            0x03
#define PHY8481_C45_DEV_PHYXS_M        0x04
#define PHY8481_C45_DEV_PHYXS_L        0x03
#define PHY8481_C45_DEV_AN             0x07
#define PHY8481_C45_DEV_TOPLVL1        0x1e
#define PHY8481_C45_DEV_TOPLVL2        0x1f

#define PHY8481_REG_READ(_unit, _phy_ctrl, _addr, _val) \
            READ_PHY_REG((_unit), (_phy_ctrl), (_addr), (_val))
#define PHY8481_REG_WRITE(_unit, _phy_ctrl, _addr, _val) \
            WRITE_PHY_REG((_unit), (_phy_ctrl), (_addr), (_val))
#define PHY8481_REG_MODIFY(_unit, _phy_ctrl, _addr, _val, _mask) \
            MODIFY_PHY_REG((_unit), (_phy_ctrl), (_addr), (_val), (_mask))

/*******************************/
/* PMA/PMD Device (Dev Addr 1) */
/*******************************/

#define PMAD_IEEE_CTL1                 0x0000 /* IEEE PMA/PMD control 1 register (reg 1.0) */
#define PMAD_IEEE_CTL1_LPBK            (1U<<0)

#define PMAD_IEEE_ST1                  0x0001 /* IEEE PMA/PMD status 1 register  (reg 1.1) */
#define PMAD_IEEE_DEV_ID0              0x0002 /* IEEE PMA/PMD device identifier part 0 (reg 1.2) */
#define PMAD_IEEE_DEV_ID1              0x0003 /* IEEE PMA/PMD device identifier part 1 (reg 1.3) */
#define PMAD_IEEE_SP_CAP               0x0004 /* IEEE PMA/PMD speed ability (reg 1.4) */
#define PMAD_IEEE_DEV0                 0x0005 /* Devices in package register part 0 (reg 1.5) */
#define PMAD_IEEE_DEV1                 0x0006 /* Devices in package register part 1 (reg 1.6) */
#define PMAD_IEEE_CTL2                 0x0007 /* PMA/PMD control 2 register (reg 1.7) */
#define PMAD_IEEE_ST2                  0x0008 /* 10G PMA/PMD status 2 register (reg 1.8) */
#define PMAD_IEEE_TRAN_DIS             0x0009 /* 10G PMA/PMD transmit disable (reg 1.9) */
#define PMAD_IEEE_RCV_DET              0x000a /* 10G PMD receive signal detect (reg 1.10) */
#define PMAD_IEEE_EXT_AB               0x000b /* 10G PMA/PMD extended ability (reg 1.11) */
#define PMAD_IEEE_PKG_ID0              0x000e /* PMA/PMD package identifier (reg 1.14) */
#define PMAD_IEEE_PKG_ID1              0x000f /* PMA/PMD package identifier (reg 1.15) */
#define PMAD_IEEE_ST_10GT              0x0000 /* 10GBASE-T status (reg 1.129) */
#define PMAD_IEEE_SWAP_POL_10GT        0x0000 /* 10GBASE-T pair swap and polarity (reg 1.130) */
#define PMAD_IEEE_PWRBK_SET_10GT       0x0000 /* 10GBASE-T TX power backoff and PHY short reach setting (reg 1.131) */
#define PMAD_IEEE_TST_MODE_10GT        0x0000 /* 10GBASE-T test mode (reg 1.132) */
#define PMAD_IEEE_SKW_DLY0_10GT        0x0000 /* 10GBASE-T skew delay part 0 (reg 1.145) */
#define PMAD_IEEE_SKW_DLY1_10GT        0x0000 /* 10GBASE-T skew delay part 1 (reg 1.146) */



#define READ_PHY8481_PMAD_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_READ((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PMAD, (_reg)), (_val))
#define WRITE_PHY8481_PMAD_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_WRITE((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PMAD, (_reg)), (_val))
#define MODIFY_PHY8481_PMAD_REG(_unit, _phy_ctrl, _reg, _val, _mask) \
            PHY8481_REG_MODIFY((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PMAD, (_reg)), (_val), (_mask))

/* According to 802.3an the GPHY speed selection bits in reg 1.0 are bit 13 - MSB,
   bit 6 - LSB. However in 8481 bit 13 - LSB and bit 6 - MSB.
 */

#define PMAD_MII_CTRL_SS_LSB         (1 << 13) /* Speed select, LSb */
#define PMAD_MII_CTRL_SS_MSB         (1 << 6) /* Speed select, MSb */
#define PMAD_MII_CTRL_SS_10          0
#define PMAD_MII_CTRL_SS_100         (PMAD_MII_CTRL_SS_LSB)
#define PMAD_MII_CTRL_SS_1000        (PMAD_MII_CTRL_SS_MSB)
#define PMAD_MII_CTRL_SS_MASK        (PMAD_MII_CTRL_SS_LSB | PMAD_MII_CTRL_SS_MSB)


/* Control Register PMAD_IEEE_CTL1 */

#define READ_PHY8481_PMAD_CTRLr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_CTL1,(_val))
#define WRITE_PHY8481_PMAD_CTRLr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_CTL1,(_val))
#define MODIFY_PHY8481_PMAD_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_CTL1, (_val), (_mask))


/* Status Register PMAD_IEEE_ST1 */

#define READ_PHY8481_PMAD_STATr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_ST1,(_val))
#define WRITE_PHY8481_PMAD_STATr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_ST1,(_val))
#define MODIFY_PHY8481_PMAD_STATr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_ST1, (_val), (_mask))


/* ID0 Register (Addr 0002h) */
#define PMAD_ID0_REG    0x2
    
/* ID1 Register (Addr 0003h) */
#define PMAD_ID1_REG    0x3

/* Speed Ability Register (Addr 0004h) */
/* Devices in Package 1 Register (Addr 0005h) */
/* Devices in Package 2 Register (Addr 0006h) */

/* Control 2 Register (Addr 0007h) */
#define PMAD_CTRL2_REG   0x7

#define IEEE_CTRLr_SPEED_10G_MASK   0x003c
#define IEEE_CTRLr_SPEED_10G        0x2040

/* Control Register PMAD_IEEE_CTL2 */

#define READ_PHY8481_PMAD_CTRL2r(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_CTL2,(_val))
#define WRITE_PHY8481_PMAD_CTRL2r(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_CTL2,(_val))
#define MODIFY_PHY8481_PMAD_CTRL2r(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_CTL2, (_val), (_mask))
 
/* Status 2 Register (Addr 0008h) */

/* Transmit Disable Register (Addr 0009h) */

#define READ_PHY8481_PMAD_TX_DISABLEr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_TRAN_DIS,(_val))
#define WRITE_PHY8481_PMAD_TX_DISABLEr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_TRAN_DIS,(_val))
#define MODIFY_PHY8481_PMAD_TX_DISABLEr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PMAD_IEEE_TRAN_DIS, (_val), (_mask))

/* Receive Signal Detect Register (Addr 000Ah) */
/* Extended Ability Register (Addr 000Bh) */
/* Receive Signal Detect Register (Addr 000Ah) */
/* Receive Signal Detect Register (Addr 000Ah) */
/* Organizationlly Unique Identifier 0 Register (Addr 000Eh) */
/* Organizationlly Unique Identifier 1 Register (Addr 000Fh) */

/* EEE Statistics related */

#define READ_PHY8481_PHYC_CTL_AI_LPI_CTLr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa88a : 0xa878,(_val))
#define WRITE_PHY8481_PHYC_CTL_AI_LPI_CTLr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa88a : 0xa878,(_val))
#define MODIFY_PHY8481_PHYC_CTL_AI_LPI_CTLr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa88a : 0xa878, (_val), (_mask))

#define READ_PHY8481_EEE_STATISTICS_CTLr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa8f5 : 0xa907,(_val))
#define WRITE_PHY8481_EEE_STATISTICS_CTLr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa8f5 : 0xa907,(_val))
#define MODIFY_PHY8481_EEE_STATISTICS_CTLr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa8f5 : 0xa907, (_val), (_mask))

#define READ_PHY8481_EXP_EEE_TX_EVENTS_HIGHr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa8f6 : 0xa908,(_val))
#define READ_PHY8481_EXP_EEE_TX_EVENTS_LOWr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa8f7 : 0xa909,(_val))

#define READ_PHY8481_EXP_EEE_RX_EVENTS_HIGHr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa8fa : 0xa90c,(_val))
#define READ_PHY8481_EXP_EEE_RX_EVENTS_LOWr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa8fb : 0xa90d,(_val))

#define READ_PHY8481_EXP_EEE_TX_DURATION_HIGHr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa8f8 : 0xa90a,(_val))
#define READ_PHY8481_EXP_EEE_TX_DURATION_LOWr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa8f9 : 0xa90b,(_val))

#define READ_PHY8481_EXP_EEE_RX_DURATION_HIGHr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa8fc : 0xa90e,(_val))
#define READ_PHY8481_EXP_EEE_RX_DURATION_LOWr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHY_IS_BCM8483_PLUS((_phy_ctrl)) ? 0xa8fd : 0xa90f,(_val))


/****************************/
/* PCS Device (Dev Addr 3)  */
/****************************/

#define PCS_IEEE_CTL1                  0x0000 /* IEEE PCS control 1 register (reg 3.0) */
#define PCS_IEEE_CTL1_LPBK             (1U<<14)

#define PCS_IEEE_ST1                   0x0001 /* IEEE PCS status 1 register  (reg 3.1) */
#define PCS_IEEE_DEV_ID0               0x0002 /* IEEE PCS device identifier part 0 (reg 3.2) */
#define PCS_IEEE_DEV_ID1               0x0003 /* IEEE PCS device identifier part 1 (reg 3.3) */
#define PCS_IEEE_SP_CAP                0x0004 /* IEEE PCS speed ability (reg 3.4) */
#define PCS_IEEE_DEV0                  0x0005 /* Devices in package register part 0 (reg 3.5) */
#define PCS_IEEE_DEV1                  0x0006 /* Devices in package register part 1 (reg 3.6) */
#define PCS_IEEE_CTL2                  0x0007 /* PCS control 2 register (reg 3.7) */
#define PCS_IEEE_ST2                   0x0008 /* 10G PCS status 2 register (reg 3.8) */
#define PCS_IEEE_PKG_ID0               0x000e /* PCS package identifier (reg 3.14) */
#define PCS_IEEE_PKG_ID1               0x000f /* PCS package identifier (reg 3.15) */
#define PCS_IEEE_RT_10G_ST1            0x0020 /* 10GBASE_R & 10GBASE_T PCS status 1 (reg 3.32) */
#define PCS_IEEE_RT_10G_ST2            0x0021 /* 10GBASE_R & 10GBASE_T PCS status 2 (reg 3.33) */


#define READ_PHY8481_PCS_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_READ((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PCS, (_reg)), (_val))
#define WRITE_PHY8481_PCS_REG(_unit, _phy_ctrl, _reg, _val) \
            PHY8481_REG_WRITE((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PCS, (_reg)), (_val))
#define MODIFY_PHY8481_PCS_REG(_unit, _phy_ctrl, _reg, _val, _mask) \
            PHY8481_REG_MODIFY((_unit), (_phy_ctrl), \
             SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PCS, (_reg)), (_val), (_mask))


/* Control 1 Register (Addr 0000h) */
#define READ_PHY8481_PCS_CTRLr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PCS_REG((_unit), (_phy_ctrl), PCS_IEEE_CTL1,(_val))
#define WRITE_PHY8481_PCS_CTRLr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PCS_REG((_unit), (_phy_ctrl), PCS_IEEE_CTL1,(_val))
#define MODIFY_PHY8481_PCS_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PCS_REG((_unit), (_phy_ctrl), PCS_IEEE_CTL1, (_val), (_mask))

/* Status 1 Register (Addr 0001h) */
#define READ_PHY8481_PCS_STATr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PCS_REG((_unit), (_phy_ctrl), PCS_IEEE_ST1,(_val))
#define WRITE_PHY8481_PCS_STATr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PCS_REG((_unit), (_phy_ctrl), PCS_IEEE_ST1,(_val))
#define MODIFY_PHY8481_PCS_STATr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PCS_REG((_unit), (_phy_ctrl), PCS_IEEE_ST1, (_val), (_mask))


/* PCS Identifier 0 Register (Addr 0002h) */

/* PCS Identifier 1 Register (Addr 0003h) */

/* Speed Ability (Addr 0004h) */
 
/* Devices in Package 1 (Addr 0005h) */

/* Devices in Package 2 (Addr 0006h) */

/* PCS Control 2 Register (Addr 0007h) */

/* PCS Status 2 Register (Addr 0008h) */

/* Organizationally Unique Identifier 0 Register (Addr 000eh) */
/* Organizationally Unique Identifier 1 Register (Addr 000fh) */
/* 10GBASE-X PCS Status Register (Addr 0x0018h) */
/* 10GBASE-R PCS Status Register (Addr 0x0020h) */
/* 10GBASE-R PCS Status 2 Register (Addr 0x0021h) */
/* 10GBASE-R PCS Jitter Test Seed A0 Register (Addr 0x0022h) */
/* 10GBASE-R PCS Jitter Test Seed A1 Register (Addr 0x0023h) */
/* 10GBASE-R PCS Jitter Test Seed A2 Register (Addr 0x0024h) */
/* 10GBASE-R PCS Jitter Test Seed A3 Register (Addr 0x0025h) */
/* 10GBASE-R PCS Jitter Test Seed B0 Register (Addr 0x0026h) */
/* 10GBASE-R PCS Jitter Test Seed B1 Register (Addr 0x0027h) */
/* 10GBASE-R PCS Jitter Test Seed B2 Register (Addr 0x0028h) */
/* 10GBASE-R PCS Jitter Test Seed B3 Register (Addr 0x0029h) */
/* 10GBASE-R PCS Jitter Test Control Register (Addr 0x002ah) */
/* 10GBASE-R PCS Jitter Test Error Counter Register (Addr 0x002Bh) */
/* PRBS23 WIN_2 Register (Addr = 0xc92dh) */

/****************/
/* XGXS Devices */
/****************/

#define XGXS_IEEE_CTL1                 0x0000
#define XGXS_IEEE_CTL1_LPBK            (1U<<14)

#define XGXS_IEEE_ST1                  0x0001
#define XGXS_IEEE_ID0                  0x0002
#define XGXS_IEEE_ID1                  0x0003
#define XGXS_IEEE_SP_AB                0x0004
#define XGXS_IEEE_DEV_IN_PKG2          0x0005 /* NOTE: (020426) this compensates for wrong mapping of spec */
#define XGXS_IEEE_DEV_IN_PKG1          0x0006
#define XGXS_IEEE_ST2                  0x0008
#define XGXS_IEEE_OUID1                0x000e /* Note: only available in Clause45 mode (st_strap = 0) */
#define XGXS_IEEE_OUID2                0x000f /* Note: only available in Clause45 mode (st_strap = 0) */
#define XGXS_IEEE_LANE_ST              0x0018
#define XGXS_IEEE_TEST_CTL             0x0019

/******************************/
/* XGXS_M Device (Dev Addr 4) */
/******************************/

#define READ_PHY8481_PHYXS_M_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_READ((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PHYXS_M, (_reg)), (_val))
#define WRITE_PHY8481_PHYXS_M_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_WRITE((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PHYXS_M, (_reg)), (_val))
#define MODIFY_PHY8481_PHYXS_M_REG(_unit, _phy_ctrl, _reg, _val, _mask) \
            PHY8481_REG_MODIFY((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PHYXS_M, (_reg)), (_val), (_mask))

/* Control 1 Register (Addr 0000h) */
#define READ_PHY8481_PHYXS_M_CTRLr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PHYXS_M_REG((_unit), (_phy_ctrl), XGXS_IEEE_CTL1,(_val))
#define WRITE_PHY8481_PHYXS_M_CTRLr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PHYXS_M_REG((_unit), (_phy_ctrl), XGXS_IEEE_CTL1,(_val))
#define MODIFY_PHY8481_PHYXS_M_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PHYXS_M_REG((_unit), (_phy_ctrl), XGXS_IEEE_CTL1, (_val), (_mask))

/* Status 1 Register (Addr 0001h) */
#define READ_PHY8481_PHYXS_M_STATr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PHYXS_M_REG((_unit), (_phy_ctrl), XGXS_IEEE_ST1,(_val))
#define WRITE_PHY8481_PHYXS_M_STATr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PHYXS_M_REG((_unit), (_phy_ctrl), XGXS_IEEE_ST1,(_val))
#define MODIFY_PHY8481_PHYXS_M_STATr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PHYXS_M_REG((_unit), (_phy_ctrl), XGXS_IEEE_ST1, (_val), (_mask))


/* PHY Identifier Register 0 (Addr 0002h) */
#define READ_PHY8481_PHYXS_M_ID0r(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PHYXS_M_REG((_unit), (_phy_ctrl), XGXS_IEEE_ID0,(_val))

/* PHY Identifier Register 1 (Addr 0003h) */
#define READ_PHY8481_PHYXS_M_ID1r(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PHYXS_M_REG((_unit), (_phy_ctrl), XGXS_IEEE_ID1,(_val))

/* Speed Ability (Addr 0004h) */
/* Device in Package 2 (Addr 0005h) */
/* Device in Package 1 (Addr 0006h) */
/* Status 2 Register (Addr 0008h) */
/* Organizatioally Unique Identifier 0 Register (Addr 000eh) */
/* Organizationally Unique Identifier 1 Register (Addr 000fh) */
/* XGXS Lane Status Register (Addr 0018h) */

/* XGXS Lane Status Register (Addr 0018h) */
#define READ_PHY8481_PHYXS_XGXS_M_LANE_STATr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PHYXS_M_REG((_unit), (_phy_ctrl), XGXS_IEEE_LANE_ST,(_val))
#define WRITE_PHY8481_PHYXS_XGXS_M_LANE_STATr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PHYXS_M_REG((_unit), (_phy_ctrl), XGXS_IEEE_LANE_ST,(_val))
#define MODIFY_PHY8481_PHYXS_XGXS_M_LANE_STATr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PHYXS_M_REG((_unit), (_phy_ctrl), XGXS_IEEE_LANE_ST, (_val), (_mask))

/* XGXS Test Control Register (Addr 0019h) */

#define READ_PHY8481_PHYXS_M_XGXSBLK0_XGXSCONTROLr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_M_REG((_unit), (_pc), 0x8000, (_val))
#define WRITE_PHY8481_PHYXS_M_XGXSBLK0_XGXSCONTROLr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_M_REG((_unit), (_pc), 0x8000, (_val))
#define MODIFY_PHY8481_PHYXS_M_XGXSBLK0_XGXSCONTROLr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_M_REG((_unit), (_pc), 0x8000, (_val), (_mask))

/* Miscellaneous 1 control register */
#define READ_PHY8481_PHYXS_M_SERDESDIGITAL_MISC1r(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_M_REG((_unit), (_pc), 0x8308, (_val))
#define WRITE_PHY8481_PHYXS_M_SERDESDIGITAL_MISC1r(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_M_REG((_unit), (_pc), 0x8308, (_val))
#define MODIFY_PHY8481_PHYXS_M_SERDESDIGITAL_MISC1r(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_M_REG((_unit), (_pc), 0x8308, (_val), (_mask))

/* IEEE MII control register */
#define READ_PHY8481_PHYXS_M_COMBO_IEEE0_MIICNTLr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_M_REG((_unit), (_pc), 0xffe0, (_val))
#define WRITE_PHY8481_PHYXS_M_COMBO_IEEE0_MIICNTLr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_M_REG((_unit), (_pc), 0xffe0, (_val))
#define MODIFY_PHY8481_PHYXS_M_COMBO_IEEE0_MIICNTLr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_M_REG((_unit), (_pc), 0xffe0, (_val), (_mask))

/* IEEE MII status register */
#define READ_PHY8481_PHYXS_M_COMBO_IEEE0_MIISTATr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_M_REG((_unit), (_pc), 0xffe1, (_val))
#define WRITE_PHY8481_PHYXS_M_COMBO_IEEE0_STATr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_M_REG((_unit), (_pc), 0xffe1, (_val))
#define MODIFY_PHY8481_PHYXS_M_COMBO_IEEE0_STATr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_M_REG((_unit), (_pc), 0xffe1, (_val), (_mask))

/* XGXG status register */
#define READ_PHY8481_PHYXS_M_XGXSBLK0_XGXSSTATUSr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_M_REG((_unit), (_pc), 0x8001, (_val))
#define WRITE_PHY8481_PHYXS_M_XGXSBLK0_XGXSSTATUSr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_M_REG((_unit), (_pc), 0x8001, (_val))
#define MODIFY_PHY8481_PHYXS_M_XGXSBLK0_XGXSSTATUSr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_M_REG((_unit), (_pc), 0x8001, (_val), (_mask))


/******************************/
/* XGXS_L Device (Dev Addr 3) */
/******************************/
#define READ_PHY8481_PHYXS_L_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_READ((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PHYXS_L, (_reg)), (_val))
#define WRITE_PHY8481_PHYXS_L_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_WRITE((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PHYXS_L, (_reg)), (_val))
#define MODIFY_PHY8481_PHYXS_L_REG(_unit, _phy_ctrl, _reg, _val, _mask) \
            PHY8481_REG_MODIFY((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_PHYXS_L, (_reg)), (_val), (_mask))


/* Control 1 Register (Addr 0000h) */
#define READ_PHY8481_PHYXS_L_CTRLr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PHYXS_L_REG((_unit), (_phy_ctrl), XGXS_IEEE_CTL1,(_val))
#define WRITE_PHY8481_PHYXS_L_CTRLr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PHYXS_L_REG((_unit), (_phy_ctrl), XGXS_IEEE_CTL1,(_val))
#define MODIFY_PHY8481_PHYXS_L_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PHYXS_L_REG((_unit), (_phy_ctrl), XGXS_IEEE_CTL1, (_val), (_mask))

/* Status 1 Register (Addr 0001h) */
#define READ_PHY8481_PHYXS_L_STATr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PHYXS_L_REG((_unit), (_phy_ctrl), XGXS_IEEE_ST1,(_val))
#define WRITE_PHY8481_PHYXS_L_STATr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PHYXS_L_REG((_unit), (_phy_ctrl), XGXS_IEEE_ST1,(_val))
#define MODIFY_PHY8481_PHYXS_L_STATr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PHYXS_L_REG((_unit), (_phy_ctrl), XGXS_IEEE_ST1, (_val), (_mask))

/* XGXS Lane Status Register (Addr 0018h) */
#define READ_PHY8481_PHYXS_XGXS_L_LANE_STATr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PHYXS_L_REG((_unit), (_phy_ctrl), XGXS_IEEE_LANE_ST,(_val))
#define WRITE_PHY8481_PHYXS_XGXS_L_LANE_STATr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PHYXS_L_REG((_unit), (_phy_ctrl), XGXS_IEEE_LANE_ST,(_val))
#define MODIFY_PHY8481_PHYXS_XGXS_L_LANE_STATr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PHYXS_L_REG((_unit), (_phy_ctrl), XGXS_IEEE_LANE_ST, (_val), (_mask))

/* XGXG status 1 register */

/* GP_Status :: xgxsStatus1 :: autoneg_complete [07:07] */
#define GP_STATUS_XGXSSTATUS1_AUTONEG_COMPLETE_MASK                0x0080
#define GP_STATUS_XGXSSTATUS1_AUTONEG_COMPLETE_ALIGN               0
#define GP_STATUS_XGXSSTATUS1_AUTONEG_COMPLETE_BITS                1
#define GP_STATUS_XGXSSTATUS1_AUTONEG_COMPLETE_SHIFT               7

#define READ_PHY8481_PHYXS_L_GP_STATUS_XGXSSTATUS1r(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008122, (_val))
#define WRITE_PHY8481_PHYXS_L_GP_STATUS_XGXSSTATUS1r(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc),  0x00008122, (_val))
#define MODIFY_PHY8481_PHYXS_L_GP_STATUS_XGXSSTATUS1r(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008122, (_val), (_mask))

/* 1000X status 1 register */

/* GP_Status :: Status1000X1 :: sgmii_mode [00:00] */
#define GP_STATUS_STATUS1000X1_SGMII_MODE_MASK                     0x0001
#define GP_STATUS_STATUS1000X1_SGMII_MODE_ALIGN                    0
#define GP_STATUS_STATUS1000X1_SGMII_MODE_BITS                     1
#define GP_STATUS_STATUS1000X1_SGMII_MODE_SHIFT                    0

#define READ_PHY8481_PHYXS_L_GP_STATUS_STATUS1000X1r(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008124, (_val))
#define WRITE_PHY8481_PHYXS_L_GP_STATUS_STATUS1000X1r(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008124, (_val))
#define MODIFY_PHY8481_PHYXS_L_GP_STATUS_STATUS1000X1r(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008124, (_val), (_mask))

/* XGXG status 3 register */

/* GP_Status :: xgxsStatus3 :: link [15:15] */
#define GP_STATUS_XGXSSTATUS3_LINK_MASK                            0x8000
#define GP_STATUS_XGXSSTATUS3_LINK_ALIGN                           0
#define GP_STATUS_XGXSSTATUS3_LINK_BITS                            1
#define GP_STATUS_XGXSSTATUS3_LINK_SHIFT                           15

#define READ_PHY8481_PHYXS_L_GP_STATUS_XGXSSTATUS3r(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008129, (_val))
#define WRITE_PHY8481_PHYXS_L_GP_STATUS_XGXSSTATUS3r(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008129, (_val))
#define MODIFY_PHY8481_PHYXS_L_GP_STATUS_XGXSSTATUS3r(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008129, (_val), (_mask))

/* XgxsBlk0 :: miscControl1 :: Global_PMD_tx_disable [11:11] */
#define XGXSBLK0_MISCCONTROL1_GLOBAL_PMD_TX_DISABLE_MASK           0x0800
#define XGXSBLK0_MISCCONTROL1_GLOBAL_PMD_TX_DISABLE_ALIGN          0
#define XGXSBLK0_MISCCONTROL1_GLOBAL_PMD_TX_DISABLE_BITS           1
#define XGXSBLK0_MISCCONTROL1_GLOBAL_PMD_TX_DISABLE_SHIFT          11

#define READ_PHY8481_PHYXS_L_XGXSBLK0_MISCCONTROL1r(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000800e, (_val))
#define WRITE_PHY8481_PHYXS_L_XGXSBLK0_MISCCONTROL1r(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000800e, (_val))
#define MODIFY_PHY8481_PHYXS_L_XGXSBLK0_MISCCONTROL1r(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000800e, (_val), (_mask))

/* dsc_2_0 :: dsc_ctrl0 :: rxSeqStart [15:15] */
#define DSC_2_0_DSC_CTRL0_RXSEQSTART_MASK                          0x8000
#define DSC_2_0_DSC_CTRL0_RXSEQSTART_ALIGN                         0
#define DSC_2_0_DSC_CTRL0_RXSEQSTART_BITS                          1
#define DSC_2_0_DSC_CTRL0_RXSEQSTART_SHIFT                         15

#define READ_PHY8481_PHYXS_L_RX_ALL_RX_CONTROLr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x000080f1, (_val))
#define WRITE_PHY8481_PHYXS_L_RX_ALL_RX_CONTROLr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x000080f1, (_val))
#define MODIFY_PHY8481_PHYXS_L_RX_ALL_RX_CONTROLr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x000080f1, (_val), (_mask))

/* XgxsBlk0 :: xgxsControl :: start_sequencer [13:13] */
#define XGXSBLK0_XGXSCONTROL_START_SEQUENCER_MASK                  0x2000
#define XGXSBLK0_XGXSCONTROL_START_SEQUENCER_ALIGN                 0
#define XGXSBLK0_XGXSCONTROL_START_SEQUENCER_BITS                  1
#define XGXSBLK0_XGXSCONTROL_START_SEQUENCER_SHIFT                 13

#define READ_PHY8481_PHYXS_L_XGXSBLK0_XGXSCONTROLr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008000, (_val))
#define WRITE_PHY8481_PHYXS_L_XGXSBLK0_XGXSCONTROLr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008000, (_val))
#define MODIFY_PHY8481_PHYXS_L_XGXSBLK0_XGXSCONTROLr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008000, (_val), (_mask))

/* SerdesDigital :: Misc1 :: force_speed_sel [04:04] */
#define SERDESDIGITAL_MISC1_FORCE_SPEED_SEL_MASK                   0x0010
#define SERDESDIGITAL_MISC1_FORCE_SPEED_SEL_ALIGN                  0
#define SERDESDIGITAL_MISC1_FORCE_SPEED_SEL_BITS                   1
#define SERDESDIGITAL_MISC1_FORCE_SPEED_SEL_SHIFT                  4

/* SerdesDigital :: Misc1 :: force_speed [03:00] */
#define SERDESDIGITAL_MISC1_FORCE_SPEED_MASK                       0x000f
#define SERDESDIGITAL_MISC1_FORCE_SPEED_ALIGN                      0
#define SERDESDIGITAL_MISC1_FORCE_SPEED_BITS                       4
#define SERDESDIGITAL_MISC1_FORCE_SPEED_SHIFT                      0

/* Miscellaneous 1 control register */
#define READ_PHY8481_PHYXS_L_SERDESDIGITAL_MISC1r(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008308, (_val))
#define WRITE_PHY8481_PHYXS_L_SERDESDIGITAL_MISC1r(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008308, (_val))
#define MODIFY_PHY8481_PHYXS_L_SERDESDIGITAL_MISC1r(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008308, (_val), (_mask))

/* XgxsBlk0 :: xgxsStatus :: txpll_lock [11:11] */
#define XGXSBLK0_XGXSSTATUS_TXPLL_LOCK_MASK                        0x0800
#define XGXSBLK0_XGXSSTATUS_TXPLL_LOCK_ALIGN                       0
#define XGXSBLK0_XGXSSTATUS_TXPLL_LOCK_BITS                        1
#define XGXSBLK0_XGXSSTATUS_TXPLL_LOCK_SHIFT                       11

/* XGXG status register */
#define READ_PHY8481_PHYXS_L_XGXSBLK0_XGXSSTATUSr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008001, (_val))
#define WRITE_PHY8481_PHYXS_L_XGXSBLK0_XGXSSTATUSr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008001, (_val))
#define MODIFY_PHY8481_PHYXS_L_XGXSBLK0_XGXSSTATUSr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008001, (_val), (_mask))

/* GP_Status :: topANStatus1 :: cl37_autoneg_complete [01:01] */
#define GP_STATUS_TOPANSTATUS1_CL37_AUTONEG_COMPLETE_MASK          0x0002
#define GP_STATUS_TOPANSTATUS1_CL37_AUTONEG_COMPLETE_ALIGN         0
#define GP_STATUS_TOPANSTATUS1_CL37_AUTONEG_COMPLETE_BITS          1
#define GP_STATUS_TOPANSTATUS1_CL37_AUTONEG_COMPLETE_SHIFT         1

/* GP_Status :: topANStatus1 :: cl73_autoneg_complete [00:00] */
#define GP_STATUS_TOPANSTATUS1_CL73_AUTONEG_COMPLETE_MASK          0x0001
#define GP_STATUS_TOPANSTATUS1_CL73_AUTONEG_COMPLETE_ALIGN         0
#define GP_STATUS_TOPANSTATUS1_CL73_AUTONEG_COMPLETE_BITS          1
#define GP_STATUS_TOPANSTATUS1_CL73_AUTONEG_COMPLETE_SHIFT         0

/* CL73 AN status 1 register */
#define READ_PHY8481_PHYXS_L_GP_STATUS_TOPANSTATUS1r(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000812b, (_val))
#define WRITE_PHY8481_PHYXS_L_GP_STATUS_TOPANSTATUS1r(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000812b, (_val))
#define MODIFY_PHY8481_PHYXS_L_GP_STATUS_TOPANSTATUS1r(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000812b, (_val), (_mask))

/* AN link partner user page 1 */
#define READ_PHY8481_PHYXS_L_GP_STATUS_LP_UP1r(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000812c, (_val))
#define WRITE_PHY8481_PHYXS_L_GP_STATUS_LP_UP1r(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000812c, (_val))
#define MODIFY_PHY8481_PHYXS_L_GP_STATUS_LP_UP1r(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000812c, (_val), (_mask))

/* AN73_pdet :: parDet10GControl :: parDet10g_en [00:00] */
#define AN73_PDET_PARDET10GCONTROL_PARDET10G_EN_MASK               0x0001
#define AN73_PDET_PARDET10GCONTROL_PARDET10G_EN_ALIGN              0
#define AN73_PDET_PARDET10GCONTROL_PARDET10G_EN_BITS               1
#define AN73_PDET_PARDET10GCONTROL_PARDET10G_EN_SHIFT              0

/* SerdesDigital :: Control1000X2 :: enable_parallel_detection [00:00] */
#define SERDESDIGITAL_CONTROL1000X2_ENABLE_PARALLEL_DETECTION_MASK 0x0001
#define SERDESDIGITAL_CONTROL1000X2_ENABLE_PARALLEL_DETECTION_ALIGN 0
#define SERDESDIGITAL_CONTROL1000X2_ENABLE_PARALLEL_DETECTION_BITS 1
#define SERDESDIGITAL_CONTROL1000X2_ENABLE_PARALLEL_DETECTION_SHIFT 0

/* MP5, Message Page 5, next page control register */
#define READ_PHY8481_PHYXS_L_BAM_NEXTPAGE_MP5_NEXTPAGECTRLr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008350, (_val))
#define WRITE_PHY8481_PHYXS_L_BAM_NEXTPAGE_MP5_NEXTPAGECTRLr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008350, (_val))
#define MODIFY_PHY8481_PHYXS_L_BAM_NEXTPAGE_MP5_NEXTPAGECTRLr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008350, (_val), (_mask))

/* 10G parallel detect control register */
#define READ_PHY8481_PHYXS_L_AN73_PDET_PARDET10GCONTROLr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008131, (_val))
#define WRITE_PHY8481_PHYXS_L_AN73_PDET_PARDET10GCONTROLr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008131, (_val))
#define MODIFY_PHY8481_PHYXS_L_AN73_PDET_PARDET10GCONTROLr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008131, (_val), (_mask))

/* 1000X control 2 register */
#define READ_PHY8481_PHYXS_L_SERDESDIGITAL_CONTROL1000X2r(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008301, (_val))
#define WRITE_PHY8481_PHYXS_L_SERDESDIGITAL_CONTROL1000X2r(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008301, (_val))
#define MODIFY_PHY8481_PHYXS_L_SERDESDIGITAL_CONTROL1000X2r(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008301, (_val), (_mask))

/* GP_Status :: topANStatus1 :: actual_speed [13:08] */
#define GP_STATUS_TOPANSTATUS1_ACTUAL_SPEED_MASK                   0x3f00
#define GP_STATUS_TOPANSTATUS1_ACTUAL_SPEED_ALIGN                  0
#define GP_STATUS_TOPANSTATUS1_ACTUAL_SPEED_BITS                   6
#define GP_STATUS_TOPANSTATUS1_ACTUAL_SPEED_SHIFT                  8

/* GP_Status :: topANStatus1 :: actual_speed [13:08] */
#define GP_STATUS_TOPANSTATUS1_ACTUAL_SPEED_MASK                   0x3f00
#define GP_STATUS_TOPANSTATUS1_ACTUAL_SPEED_ALIGN                  0
#define GP_STATUS_TOPANSTATUS1_ACTUAL_SPEED_BITS                   6
#define GP_STATUS_TOPANSTATUS1_ACTUAL_SPEED_SHIFT                  8

/* IEEE MII control register */
#define READ_PHY8481_PHYXS_L_COMBO_IEEE0_MIICNTLr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe0, (_val))
#define WRITE_PHY8481_PHYXS_L_COMBO_IEEE0_MIICNTLr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe0, (_val))
#define MODIFY_PHY8481_PHYXS_L_COMBO_IEEE0_MIICNTLr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe0, (_val), (_mask))

/* IEEE MII status register */
#define READ_PHY8481_PHYXS_L_COMBO_IEEE0_MIISTATr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0xffe1, (_val))
#define WRITE_PHY8481_PHYXS_L_COMBO_IEEE0_STATr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0xffe1, (_val))
#define MODIFY_PHY8481_PHYXS_L_COMBO_IEEE0_STATr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0xffe1, (_val), (_mask))

/* IEEE auto-negotiation advertiesed abilities register */
#define READ_PHY8481_PHYXS_L_COMBO_IEEE0_AUTONEGADVr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe4, (_val))
#define WRITE_PHY8481_PHYXS_L_COMBO_IEEE0_AUTONEGADVr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe4, (_val))
#define MODIFY_PHY8481_PHYXS_L_COMBO_IEEE0_AUTONEGADVr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe4, (_val), (_mask))

/* IEEE auto-negotiation link partner abilities register */
#define READ_PHY8481_PHYXS_L_COMBO_IEEE0_AUTONEGLPABILr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe5, (_val))
#define WRITE_PHY8481_PHYXS_L_COMBO_IEEE0_AUTONEGLPABILr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe5, (_val))
#define MODIFY_PHY8481_PHYXS_L_COMBO_IEEE0_AUTONEGLPABILr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe5, (_val), (_mask))

/* IEEE auto-negotiation expansion register */
#define READ_PHY8481_PHYXS_L_COMBO_IEEE0_AUTONEGEXPr(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe6, (_val))
#define WRITE_PHY8481_PHYXS_L_COMBO_IEEE0_AUTONEGEXPr(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe6, (_val))
#define MODIFY_PHY8481_PHYXS_L_COMBO_IEEE0_AUTONEGEXPr(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x0000ffe6, (_val), (_mask))

/* Over1G :: UP1 :: dataRate_10GCX4 [04:04] */
#define OVER1G_UP1_DATARATE_10GCX4_MASK                            0x0010
#define OVER1G_UP1_DATARATE_10GCX4_ALIGN                           0
#define OVER1G_UP1_DATARATE_10GCX4_BITS                            1
#define OVER1G_UP1_DATARATE_10GCX4_SHIFT                           4

/* Over1G :: LP_UP1 :: dataRate_10GCX4 [04:04] */
#define OVER1G_LP_UP1_DATARATE_10GCX4_MASK                         0x0010
#define OVER1G_LP_UP1_DATARATE_10GCX4_ALIGN                        0
#define OVER1G_LP_UP1_DATARATE_10GCX4_BITS                         1
#define OVER1G_LP_UP1_DATARATE_10GCX4_SHIFT                        4

/* AN local device user page 1 */
#define READ_PHY8481_PHYXS_L_OVER1G_UP1r(_unit, _pc, _val) \
             READ_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008329, (_val))
#define WRITE_PHY8481_PHYXS_L_OVER1G_UP1r(_unit, _pc, _val) \
             WRITE_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008329, (_val))
#define MODIFY_PHY8481_PHYXS_L_OVER1G_UP1r(_unit, _pc, _val, _mask) \
             MODIFY_PHY8481_PHYXS_L_REG((_unit), (_pc), 0x00008329, (_val), (_mask))

/************************************************/
/* User Defined Registers (Devices 30 and 31) */
/************************************************/

#define READ_PHY8481_TOPLVL1_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_READ((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_TOPLVL1, (_reg)), (_val))
#define WRITE_PHY8481_TOPLVL1_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_WRITE((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_TOPLVL1, (_reg)), (_val))
#define MODIFY_PHY8481_TOPLVL1_REG(_unit, _phy_ctrl, _reg, _val, _mask) \
            PHY8481_REG_MODIFY((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_TOPLVL1, (_reg)), (_val), (_mask))


#define READ_PHY8481_TOPLVL2_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_READ((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_TOPLVL2, (_reg)), (_val))
#define WRITE_PHY8481_TOPLVL2_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_WRITE((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_TOPLVL2, (_reg)), (_val))
#define MODIFY_PHY8481_TOPLVL2_REG(_unit, _phy_ctrl, _reg, _val, _mask) \
            PHY8481_REG_MODIFY((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_TOPLVL2, (_reg)), (_val), (_mask))

/* PHY Identifier Register 0 (Addr 0002h) */
#define READ_PHY8481_TOPLVL1_ID0r(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), 0x0002,(_val))

/* PHY Identifier Register 1 (Addr 0003h) */
#define READ_PHY8481_TOPLVL1_ID1r(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), 0x0003,(_val))

#define PHY8481_SELECT_REGS_CU(_unit, _phy_ctrl) \
            { \
                if ( !PHY_FORCED_COPPER_MODE((_unit), (_phy_ctrl)->port)) { \
                    SOC_IF_ERROR_RETURN \
                        (WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), 0x4111, 0x0003)); \
                } \
            }

#define PHY8481_SELECT_REGS_XAUI_8481(_unit, _phy_ctrl) \
            { \
                SOC_IF_ERROR_RETURN \
                    (WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), 0x4111, 0x2004)); \
            }


#define PHY8481_SELECT_REGS_XAUI_8482X(_unit, _phy_ctrl) \
            { \
                SOC_IF_ERROR_RETURN \
                    (WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), 0x4111, 0x3008)); \
            }


#define PHY8481_SELECT_REGS_XAUI(_unit, _phy_ctrl) \
            { \
                if (PHY_FORCED_COPPER_MODE((_unit), (_phy_ctrl)->port)) { \
                    return SOC_E_CONFIG; \
                } \
                if (((_phy_ctrl)->phy_model == PHY_BCM84823_MODEL) || \
                    ((_phy_ctrl)->phy_model == PHY_BCM84821_MODEL)) { \
                    PHY8481_SELECT_REGS_XAUI_8482X((_unit), (_phy_ctrl)) \
                } else {\
                    PHY8481_SELECT_REGS_XAUI_8481((_unit), (_phy_ctrl)) \
                } \
            }

/***************************************/
/* Auto-Neotiation device (Dev Addr 7) */
/***************************************/

#define READ_PHY8481_AN_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_READ((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_AN, (_reg)), (_val))
#define WRITE_PHY8481_AN_REG(_unit, _phy_ctrl, _reg,_val) \
            PHY8481_REG_WRITE((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_AN, (_reg)), (_val))
#define MODIFY_PHY8481_AN_REG(_unit, _phy_ctrl, _reg, _val, _mask) \
            PHY8481_REG_MODIFY((_unit), (_phy_ctrl), \
              SOC_PHY_CLAUSE45_ADDR(PHY8481_C45_DEV_AN, (_reg)), (_val), (_mask))

/* autoneg control register */
#define AN_IEEE_CTRL_REG               0x0000
#define AN_IEEE_CTRL_REG_AN_RESET      (1U << 15)
#define AN_IEEE_CTRL_REG_EXT_NXT_PAGE  (1U << 13)
#define AN_IEEE_CTRL_REG_AN_ENA        (1U << 12)
#define AN_IEEE_CTRL_REG_AN_RESTART    (1U << 9)

/* autoneg status register */
#define AN_IEEE_STATUS_REG             0x0001
#define AN_IEEE_STATUS_REG_AN_DONE     (1U << 5)


#define AN_IEEE_DEV_ID_LSB             0x0002 /* Auto Neg Device Identifier Lower 16 bit */
#define AN_IEEE_DEV_ID_MSB             0x0003 /* Auto Neg Device Identifier Upper 16 bit */
#define AN_IEEE_DEV_IN_PKG_LSB         0x0005 /* Auto Neg Device In Package Lower 16 bit */
#define AN_IEEE_DEV_IN_PKG_MSB         0x0006 /* Auto Neg Device In Package Upper 16 bit */
#define AN_IEEE_DEV_PKG_ID_LSB         0x000e /* Auto Neg Package ID Lower 16 bit */
#define AN_IEEE_DEV_PKG_ID_MSB         0x000f /* Auto Neg Package ID Upper 16 bit */
#define AN_IEEE_AD                     0x0010 /* Auto Neg Advertisement */
#define AN_IEEE_ADB                    0x0011 /* */
#define AN_IEEE_ADC                    0x0012 /* */
#define AN_IEEE_LPA                    0x0013 /* */
#define AN_IEEE_LPB                    0x0014 /* */
#define AN_IEEE_LPC                    0x0015 /* */
#define AN_IEEE_XNPA                   0x0016 /* */
#define AN_IEEE_XNPB                   0x0017 /* */
#define AN_IEEE_XNPC                   0x0018 /* */
#define LP_IEEE_XNPA                   0x0019 /* Link Partners A Extended Next Page */
#define LP_IEEE_XNPB                   0x001a /* Link Partners B Extended Next Page */
#define LP_IEEE_XNPC                   0x001b /* Link Partners C Extended Next Page */

#define TENG_IEEE_AN_CTRL              0x0020 /* 10G Base-T AN Control Register */
#define TENG_IEEE_AN_CTRL_10GBT        (1U<<12)

#define TENG_IEEE_AN_STAT              0x0021 /* 10G Base-T AN Status Register */
#define TENG_IEEE_AN_STAT_LP_10GBT     (1U<<11)

/* Control 1 Register (Addr 0000h) */
#define READ_PHY8481_AN_CTRLr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_AN_REG((_unit), (_phy_ctrl), AN_IEEE_CTRL_REG,(_val))
#define WRITE_PHY8481_AN_CTRLr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_AN_REG((_unit), (_phy_ctrl), AN_IEEE_CTRL_REG,(_val))
#define MODIFY_PHY8481_AN_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_AN_REG((_unit), (_phy_ctrl), AN_IEEE_CTRL_REG, (_val), (_mask))


/* Status 1 Register (Addr 0001h) */
#define READ_PHY8481_AN_STATr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_AN_REG((_unit), (_phy_ctrl), AN_IEEE_STATUS_REG,(_val))
#define WRITE_PHY8481_AN_STATr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_AN_REG((_unit), (_phy_ctrl), AN_IEEE_STATUS_REG,(_val))
#define MODIFY_PHY8481_AN_STATr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_AN_REG((_unit), (_phy_ctrl), AN_IEEE_STATUS_REG, (_val), (_mask))


/* 10G Control Register (Addr 0020h) */
#define READ_PHY8481_TENG_AN_CTRLr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_AN_REG((_unit), (_phy_ctrl), TENG_IEEE_AN_CTRL,(_val))
#define WRITE_PHY8481_TENG_AN_CTRLr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_AN_REG((_unit), (_phy_ctrl), TENG_IEEE_AN_CTRL,(_val))
#define MODIFY_PHY8481_TENG_AN_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_AN_REG((_unit), (_phy_ctrl), TENG_IEEE_AN_CTRL, (_val), (_mask))


/* 10G Status Register (Addr 0021h) */
#define READ_PHY8481_TENG_AN_STATr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_AN_REG((_unit), (_phy_ctrl), TENG_IEEE_AN_STAT,(_val))
#define WRITE_PHY8481_TENG_AN_STATr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_AN_REG((_unit), (_phy_ctrl), TENG_IEEE_AN_STAT,(_val))
#define MODIFY_PHY8481_TENG_AN_STATr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_AN_REG((_unit), (_phy_ctrl), TENG_IEEE_AN_STAT, (_val), (_mask))

/* EEE Advertisement Register (Addr 003ch) */
#define READ_PHY8481_EEE_ADVr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_AN_REG((_unit), (_phy_ctrl), 0x3c,(_val))
#define WRITE_PHY8481_EEE_ADVr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_AN_REG((_unit), (_phy_ctrl), 0x3c,(_val))
#define MODIFY_PHY8481_EEE_ADVr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_AN_REG((_unit), (_phy_ctrl), 0x3c, (_val), (_mask))

/* EEE Link Partner Advertisement Register (Addr 003dh) */
#define READ_PHY8481_EEE_LPABILr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_AN_REG((_unit), (_phy_ctrl), 0x3d,(_val))
#define WRITE_PHY8481_EEE_LPABILr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_AN_REG((_unit), (_phy_ctrl), 0x3d,(_val))
#define MODIFY_PHY8481_EEE_LPABILr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_AN_REG((_unit), (_phy_ctrl), 0x3d, (_val), (_mask))

/* 10/100/1000-BaseT regs */

/* PHY register access */
#define PHY8481_D7_FFXX_REG_READ(_unit, _phy_ctrl, _flags, _reg_bank, _reg_addr, _val) \
            phy_reg_xge_read((_unit), (_phy_ctrl), (_flags), (_reg_bank), \
                            PHY8481_C45_DEV_AN,(_reg_addr), (_val))
#define PHY8481_D7_FFXX_REG_WRITE(_unit, _phy_ctrl, _flags, _reg_bank, _reg_addr, _val) \
            phy_reg_xge_write((_unit), (_phy_ctrl), (_flags), (_reg_bank), \
                            PHY8481_C45_DEV_AN,(_reg_addr), (_val))
#define PHY8481_D7_FFXX_REG_MODIFY(_unit, _phy_ctrl, _flags, _reg_bank, _reg_addr,_val, _mask) \
            phy_reg_xge_modify((_unit), (_phy_ctrl), (_flags), (_reg_bank), \
                            PHY8481_C45_DEV_AN,(_reg_addr), (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T MII Control Register (Addr FFE0h) */
#define READ_PHY8481_MII_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe0, (_val))
#define WRITE_PHY8481_MII_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe0, (_val)) 
#define MODIFY_PHY8481_MII_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe0, \
                               (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T MII Status Register (ADDR FFE1h) */
#define READ_PHY8481_MII_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe1, (_val)) 
#define WRITE_PHY8481_MII_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe1, (_val)) 
#define MODIFY_PHY8481_MII_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe1, \
                               (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T PHY Identifier Register (ADDR FFE2h) */
#define READ_PHY8481_MII_PHY_ID0r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe2, (_val))
#define WRITE_PHY8481_MII_PHY_ID0r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe2, (_val))
#define MODIFY_PHY8481_MII_PHY_ID0r(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe2, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T PHY Identifier Register (ADDR FFE3h) */
#define READ_PHY8481_MII_PHY_ID1r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe3, (_val))
#define WRITE_PHY8481_MII_PHY_ID1r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe3, (_val))
#define MODIFY_PHY8481_MII_PHY_ID1r(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe3, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auto-neg Advertisment Register (ADDR FFE4h) */
#define READ_PHY8481_MII_ANAr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe4, (_val))
#define WRITE_PHY8481_MII_ANAr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe4, (_val))
#define MODIFY_PHY8481_MII_ANAr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe4, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auto-neg Link Partner Ability (ADDR FFE5h) */
#define READ_PHY8481_MII_ANPr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe5, (_val))
#define WRITE_PHY8481_MII_ANPr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe5, (_val)) 
#define MODIFY_PHY8481_MII_ANPr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe5, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auto-neg Expansion Register (ADDR FFE6h) */
#define READ_PHY8481_MII_AN_EXPr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe6, (_val))
#define WRITE_PHY8481_MII_AN_EXPr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe6, (_val))
#define MODIFY_PHY8481_MII_AN_EXPr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe6, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Next Page Transmit Register (ADDR FFE7h) */
/* 1000BASE-T/100BASE-TX/10BASE-T Link Partner Received Next Page (ADDR FFE8h) */

/* 1000BASE-T Control Register  (ADDR FFE9h) */
#define READ_PHY8481_MII_GB_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe9, (_val))
#define WRITE_PHY8481_MII_GB_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe9, (_val))
#define MODIFY_PHY8481_MII_GB_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffe9, \
                             (_val), (_mask))

/* 1000BASE-T Status Register (ADDR FFEAh) */
#define READ_PHY8481_MII_GB_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffea, (_val)) 
#define WRITE_PHY8481_MII_GB_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffea, (_val))
#define MODIFY_PHY8481_MII_GB_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffea, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T IEEE Extended Status Register (ADDR FFEFh) */
#define READ_PHY8481_MII_ESRr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffef, (_val)) 
#define WRITE_PHY8481_MII_ESRr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffef, (_val))
#define MODIFY_PHY8481_MII_ESRr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xffef, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T PHY Extended Control Register (ADDR FFF0h) */
#define READ_PHY8481_PHY_ECRr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfff0, (_val)) 
#define WRITE_PHY8481_PHY_ECRr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfff0, (_val))
#define MODIFY_PHY8481_PHY_ECRr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfff0, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T PHY Extended Status Register (ADDR FFF1h) */
#define READ_PHY8481_PHY_ESRr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfff1, (_val)) 
#define WRITE_PHY8481_PHY_ESRr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfff1, (_val))
#define MODIFY_PHY8481_PHY_ESRr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfff1, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Receive Error Counter Register (ADDR FFF2h) */
/* 1000BASE-T/100BASE-TX/10BASE-T False Carrier Sense Counter (ADDR FFF3h) */
/* 1000BASE-T/100BASE-TX/10BASE-T Receive NOT_OK Counter Register (ADDR FFF4h) */

/* 1000BASE-T/100BASE-TX/10BASE-T Auxiliary Control Reg (ADDR FFF8h Shadow 000)*/
#define READ_PHY8481_MII_AUX_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfff8, (_val))
#define WRITE_PHY8481_MII_AUX_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfff8, (_val))
#define MODIFY_PHY8481_MII_AUX_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfff8, \
                             (_val), (_mask))

/* 10BASE-T Register (ADDR FFF8h Shadow 001) */
#define READ_PHY8481_MII_10BASE_Tr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0001, 0xfff8, (_val))
#define WRITE_PHY8481_MII_10BASE_Tr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0001, 0xfff8, (_val))
#define MODIFY_PHY8481_MII_10BASE_Tr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0001, 0xfff8, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Power/MII Control Reg (ADDR FFF8h Shadow 010)*/
#define READ_PHY8481_MII_POWER_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0002, 0xfff8, (_val))
#define WRITE_PHY8481_MII_POWER_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0002, 0xfff8, (_val))
#define MODIFY_PHY8481_MII_POWER_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0002, 0xfff8, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Misc Test Register (ADDR FFF8h Shadow 100) */
#define READ_PHY8481_MII_MISC_TESTr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0004, 0xfff8, (_val))
#define WRITE_PHY8481_MII_MISC_TESTr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0004, 0xfff8, (_val))
#define MODIFY_PHY8481_MII_MISC_TESTr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0004, 0xfff8, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Misc Control Register (ADDR FFF8h Shadow 111)*/
#define READ_PHY8481_MII_MISC_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0007, 0xfff8, (_val))
#define WRITE_PHY8481_MII_MISC_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0007, 0xfff8, (_val))
#define MODIFY_PHY8481_MII_MISC_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0007, 0xfff8, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auxiliary Status Register (ADDR FFF9h) */

/* 1000BASE-T/100BASE-TX/10BASE-T Interrupt Status Register (ADDR FFFAh) */
#define READ_PHY8481_INT_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfffa, (_val)) 
#define WRITE_PHY8481_INT_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfffa, (_val))
#define MODIFY_PHY8481_INT_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfffa, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Interrupt Control Register (ADDR FFFBh) */

/* 1000BASE-T/100BASE-TX/10BASE-T Spare Ctrl Reg (ADDR FFFCh shadow 00010) */
#define READ_PHY8481_SPARE_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0002, 0xfffc, (_val))
#define WRITE_PHY8481_SPARE_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0002, 0xfffc, (_val))
#define MODIFY_PHY8481_SPARE_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0002, 0xfffc, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Clk Alignment Ctrl (ADDR FFFCh shadow 00011) */
#define READ_PHY8481_CLK_ALIGN_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0003, 0xfffc, (_val))
#define WRITE_PHY8481_CLK_ALIGN_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0003, 0xfffc, (_val))
#define MODIFY_PHY8481_CLK_ALIGN_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0003, 0xfffc, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Spare Ctrl 2 Reg (ADDR FFFCh shadow 00100) */
#define READ_PHY8481_SPARE_CTRL_2r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0004, 0xfffc, (_val))
#define WRITE_PHY8481_SPARE_CTRL_2r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0004, 0xfffc, (_val))
#define MODIFY_PHY8481_SPARE_CTRL_2r(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0004, 0xfffc, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Spare Ctrl 3 Reg (ADDR FFFCh shadow 00101) */
#define READ_PHY8481_SPARE_CTRL_3r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0005, 0xfffc, (_val))
#define WRITE_PHY8481_SPARE_CTRL_3r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0005, 0xfffc, (_val))
#define MODIFY_PHY8481_SPARE_CTRL_3r(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0005, 0xfffc, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED Status Reg (ADDR FFFCh shadow 01000) */
#define READ_PHY8481_LED_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0008, 0xfffc, (_val))
#define WRITE_PHY8481_LED_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0008, 0xfffc, (_val))
#define MODIFY_PHY8481_LED_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0008, 0xfffc, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED Ctrl Reg (ADDR FFFCh shadow 01001) */
#define READ_PHY8481_LED_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0009, 0xfffc, (_val))
#define WRITE_PHY8481_LED_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0009, 0xfffc, (_val))
#define MODIFY_PHY8481_LED_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0009, 0xfffc, \
                           (_val), (_mask))

/* Auto Power-Down Reg (ADDR FFFCh shadow 01010) */
#define READ_PHY8481_AUTO_POWER_DOWNr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x000a, 0xfffc, (_val))
#define WRITE_PHY8481_AUTO_POWER_DOWNr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x000a, 0xfffc, (_val))
#define MODIFY_PHY8481_AUTO_POWER_DOWNr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x000a, 0xfffc, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED Selector 1 Reg (ADDR FFFCh shadow 01101) */
#define READ_PHY8481_LED_SELECTOR_1r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x000d, 0xfffc, (_val))
#define WRITE_PHY8481_LED_SELECTOR_1r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x000d, 0xfffc, (_val))
#define MODIFY_PHY8481_LED_SELECTOR_1r(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x000d, 0xfffc, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED Selector 2 Reg (ADDR FFFCh shadow 01110) */
#define READ_PHY8481_LED_SELECTOR_2r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x000e, 0xfffc, (_val))
#define WRITE_PHY8481_LED_SELECTOR_2r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x000e, 0xfffc, (_val))
#define MODIFY_PHY8481_LED_SELECTOR_2r(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x000e, 0xfffc, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED GPIO Ctrl/Stat (ADDR FFFCh shadow 01111) */
#define READ_PHY8481_LED_GPIO_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x000f, 0xfffc, (_val))
#define WRITE_PHY8481_LED_GPIO_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x000f, 0xfffc, (_val))
#define MODIFY_PHY8481_LED_GPIO_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x000f, 0xfffc, \
                           (_val), (_mask))

/* SerDes 100BASE-FX Status Reg (ADDR FFFCh shadow 10001) */
#define READ_PHY8481_100FX_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0011, 0xfffc, (_val))
#define WRITE_PHY8481_100FX_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0011, 0xfffc, (_val))
#define MODIFY_PHY8481_100FX_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0011, 0xfffc, \
                           (_val), (_mask))

/* SerDes 100BASE-FX Control Reg (ADDR FFFCh shadow 10011) */
#define READ_PHY8481_100FX_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0013, 0xfffc, (_val))
#define WRITE_PHY8481_100FX_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0013, 0xfffc, (_val))
#define MODIFY_PHY8481_100FX_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0013, 0xfffc, \
                           (_val), (_mask))

/* Secondary SerDes Control Reg (ADDR FFFCh shadow 10100) */
#define READ_PHY8481_2ND_SERDES_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0014, 0xfffc, (_val))
#define WRITE_PHY8481_2ND_SERDES_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0014, 0xfffc, (_val))
#define MODIFY_PHY8481_2ND_SERDES_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0014, 0xfffc, \
                           (_val), (_mask))

/* SGMII Slave Reg (ADDR FFFCh shadow 10101) */
#define READ_PHY8481_SGMII_SLAVEr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0015, 0xfffc, (_val))
#define WRITE_PHY8481_SGMII_SLAVEr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0015, 0xfffc, (_val))
#define MODIFY_PHY8481_SGMII_SLAVEr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0015, 0xfffc, \
                           (_val), (_mask))

/* Primary SerDes Control Reg (ADDR FFFCh shadow 10110) */
#define READ_PHY8481_MISC_1000X_CTRL2r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0016, 0xfffc, (_val))
#define WRITE_PHY8481_MISC_1000X_CTRL2r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0016, 0xfffc, (_val))
#define MODIFY_PHY8481_MISC_1000X_CTRL2r(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0016, 0xfffc, \
                           (_val), (_mask))

/* 1000BASE-X Auto-Detect SGMII/Media Converter Reg (ADDR FFFCh shadow 11000) */
#define READ_PHY8481_AUTO_DETECT_SGMII_MEDIAr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0018, 0xfffc, (_val))
#define WRITE_PHY8481_AUTO_DETECT_SGMII_MEDIAr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0018, 0xfffc, (_val))
#define MODIFY_PHY8481_AUTO_DETECT_SGMII_MEDIAr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0018, 0xfffc, \
                           (_val), (_mask))

/* 1000BASE-X Auto-neg Debug Reg (ADDR FFFCh shadow 11010) */
#define READ_PHY8481_1000X_AN_DEBUGr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001a, 0xfffc, (_val))
#define WRITE_PHY8481_1000X_AN_DEBUGr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001a, 0xfffc, (_val))
#define MODIFY_PHY8481_1000X_AN_DEBUGr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001a, 0xfffc, \
                           (_val), (_mask))

/* Auxiliary 1000BASE-X Control Reg (ADDR FFFCh shadow 11011) */
#define READ_PHY8481_AUX_1000X_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001b, 0xfffc, (_val))
#define WRITE_PHY8481_AUX_1000X_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001b, 0xfffc, (_val))
#define MODIFY_PHY8481_AUX_1000X_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001b, 0xfffc, \
                           (_val), (_mask))

/* Auxiliary 1000BASE-X Status Reg (ADDR FFFCh shadow 11100) */
#define READ_PHY8481_AUX_1000X_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001c, 0xfffc, (_val))
#define WRITE_PHY8481_AUX_1000X_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001c, 0xfffc, (_val))
#define MODIFY_PHY8481_AUX_1000X_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001c, 0xfffc, \
                           (_val), (_mask))

/* Misc 1000BASE-X Status Reg (ADDR FFFCh shadow 11101) */
#define READ_PHY8481_MISC_1000X_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001d, 0xfffc, (_val))
#define WRITE_PHY8481_MISC_1000X_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001d, 0xfffc, (_val))
#define MODIFY_PHY8481_MISC_1000X_STATr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001d, 0xfffc, \
                           (_val), (_mask))

/* Copper/Fiber Auto-Detect Medium Reg (ADDR FFFCh shadow 11110) */
#define READ_PHY8481_AUTO_DETECT_MEDIUMr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001e, 0xfffc, (_val))
#define WRITE_PHY8481_AUTO_DETECT_MEDIUMr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001e, 0xfffc, (_val))
#define MODIFY_PHY8481_AUTO_DETECT_MEDIUMr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001e, 0xfffc, \
                           (_val), (_mask))

/* Mode Control Reg (ADDR FFFCh shadow 11111) */
#define READ_PHY8481_MODE_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x001f, 0xfffc, (_val))
#define WRITE_PHY8481_MODE_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x001f, 0xfffc, (_val))
#define MODIFY_PHY8481_MODE_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x001f, 0xfffc, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Master/Slave Seed Reg (ADDR FFFDh bit 15 = 0) */
/* 1000BASE-T/100BASE-TX/10BASE-T HDC Status Reg (ADDR FFFDh bit 15 = 1) */

/* 1000BASE-T/100BASE-TX/10BASE-T Test Register 1 (ADDR FFFeh) */
#define READ_PHY8481_TEST1r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfffe, (_val))
#define WRITE_PHY8481_TEST1r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfffe, (_val))
#define MODIFY_PHY8481_TEST1r(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0000, 0xfffe, \
                           (_val), (_mask))


/*          +-------------------------+
 *          |                         |
 *          |   Expansion Registers   |
 *          |                         |
 *          +-------------------------+
 */
/* Receive/Transmit Packet Counter Register (Addr 00h) */
#define READ_PHY8481_EXP_PKT_COUNTERr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f00, 0xfff5, (_val))
#define WRITE_PHY8481_EXP_PKT_COUNTERr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f00, \
                               0xfff5, (_val))
#define MODIFY_PHY8481_EXP_PKT_COUNTERr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f00, \
                                0xfff5, (_val), (_mask))

/* Expansion Interrupt Status Register (Addr 01h) */
#define READ_PHY8481_EXP_INTERRUPT_STATr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f01, 0xfff5, (_val))
#define WRITE_PHY8481_EXP_INTERRUPT_STAT_r(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f01, \
                               0xfff5, (_val))
#define MODIFY_PHY8481_EXP_INTERRUPT_STAT_r(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f01, \
                                0xfff5, (_val), (_mask))

/* Expansion Interrupt Mask Register (Addr 02h) */
#define READ_PHY8481_EXP_INTERRUPT_MASKr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f02, 0xfff5, (_val))
#define WRITE_PHY8481_EXP_INTERRUPT_MASKr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f02, \
                               0xfff5, (_val))
#define MODIFY_PHY8481_EXP_INTERRUPT_MASKr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f02, \
                                0xfff5, (_val), (_mask))

/* Expansion Interrupt Mask Register (Addr 09h) */
#define READ_PHY8481_EXP_AMRR_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f09, 0xfff5, (_val))
#define WRITE_PHY8481_EXP_AMRR_CTRLr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f09, \
                               0xfff5, (_val))
#define MODIFY_PHY8481_EXP_AMRR_CTRLr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f09, \
                                0xfff5, (_val), (_mask))

/* Expansion Pattern Generator Register (Addr 46h) */
#define READ_PHY8481_EXP_PATTERN_GENr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_READ((_unit), (_phy_ctrl), 0x00, 0x0f46, 0xfff5, (_val))
#define WRITE_PHY8481_EXP_PATTERN_GENr(_unit, _phy_ctrl, _val) \
            PHY8481_D7_FFXX_REG_WRITE((_unit), (_phy_ctrl), 0x00, 0x0f46, \
                               0xfff5, (_val))
#define MODIFY_PHY8481_EXP_PATTERN_GENr(_unit, _phy_ctrl, _val, _mask) \
            PHY8481_D7_FFXX_REG_MODIFY((_unit), (_phy_ctrl), 0x00, 0x0f46, \
                                0xfff5, (_val), (_mask))

 
/****************************************************************************
 * CHIP_10G_REG16_100SPC_XGP
 ***************************************************************************/
#define XGP_PD_DEF_REG_02              0x4102 /* "prtad/devad definition register 02" */
#define XGP_PD_DEF_REG_06              0x4106 /* "prtad/devad definition register 06" */

/* XGP :: pd_def_reg_00 :: prtad [09:05] */
#define XGP_PD_DEF_REG_06_PRTAD_MASK                               0x03e0
#define XGP_PD_DEF_REG_06_PRTAD_ALIGN                              0
#define XGP_PD_DEF_REG_06_PRTAD_BITS                               5
#define XGP_PD_DEF_REG_06_PRTAD_SHIFT                              5

/* XGP_PD_DEF_REG_02 (Addr 4102h) */
#define READ_PHY8481_XGP_PD_DEF_REG_02(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XGP_PD_DEF_REG_02,(_val))

#define WRITE_PHY8481_XGP_PD_DEF_REG_02(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XGP_PD_DEF_REG_02,(_val))

#define MODIFY_PHY8481_XGP_PD_DEF_REG_02(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XGP_PD_DEF_REG_02,(_val),(_mask))


/* XGP_PD_DEF_REG_06 (Addr 4106h) */
#define READ_PHY8481_XGP_PD_DEF_REG_06(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XGP_PD_DEF_REG_06,(_val))

#define WRITE_PHY8481_XGP_PD_DEF_REG_06(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XGP_PD_DEF_REG_06,(_val))

#define MODIFY_PHY8481_XGP_PD_DEF_REG_06(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XGP_PD_DEF_REG_06,(_val),(_mask))


#define WRITE_PHY8481_XGP_PD_DEF_REG_06_PRTAD(_unit, _phy_ctrl, _val) \
            MODIFY_PHY8481_XGP_PD_DEF_REG_06(_unit, _phy_ctrl, \
                (_val)<<XGP_PD_DEF_REG_06_PRTAD_SHIFT, XGP_PD_DEF_REG_06_PRTAD_MASK)



#define XAUI1_MDIO_CTL_REG              0x4080
#define XAUI2_MDIO_CTL_REG              0x4090


/****************************************************************************
 * xaui1 :: mdio_ctl
 ***************************************************************************/
/* xaui1 :: mdio_ctl :: reserved0 [15:13] */
#define XAUI1_MDIO_CTL_RESERVED0_MASK                              0xe000
#define XAUI1_MDIO_CTL_RESERVED0_ALIGN                             0
#define XAUI1_MDIO_CTL_RESERVED0_BITS                              3
#define XAUI1_MDIO_CTL_RESERVED0_SHIFT                             13

/* xaui1 :: mdio_ctl :: remotemdioreq [12:12] */
#define XAUI1_MDIO_CTL_REMOTEMDIOREQ_MASK                          0x1000
#define XAUI1_MDIO_CTL_REMOTEMDIOREQ_ALIGN                         0
#define XAUI1_MDIO_CTL_REMOTEMDIOREQ_BITS                          1
#define XAUI1_MDIO_CTL_REMOTEMDIOREQ_SHIFT                         12

/* xaui1 :: mdio_ctl :: md_st [11:11] */
#define XAUI1_MDIO_CTL_MD_ST_MASK                                  0x0800
#define XAUI1_MDIO_CTL_MD_ST_ALIGN                                 0
#define XAUI1_MDIO_CTL_MD_ST_BITS                                  1
#define XAUI1_MDIO_CTL_MD_ST_SHIFT                                 11

/* xaui1 :: mdio_ctl :: multimmds_strap [10:10] */
#define XAUI1_MDIO_CTL_MULTIMMDS_STRAP_MASK                        0x0400
#define XAUI1_MDIO_CTL_MULTIMMDS_STRAP_ALIGN                       0
#define XAUI1_MDIO_CTL_MULTIMMDS_STRAP_BITS                        1
#define XAUI1_MDIO_CTL_MULTIMMDS_STRAP_SHIFT                       10

/* xaui1 :: mdio_ctl :: md_devad [09:05] */
#define XAUI1_MDIO_CTL_MD_DEVAD_MASK                               0x03e0
#define XAUI1_MDIO_CTL_MD_DEVAD_ALIGN                              0
#define XAUI1_MDIO_CTL_MD_DEVAD_BITS                               5
#define XAUI1_MDIO_CTL_MD_DEVAD_SHIFT                              5

/* xaui1 :: mdio_ctl :: prtad [04:00] */
#define XAUI1_MDIO_CTL_PRTAD_MASK                                  0x001f
#define XAUI1_MDIO_CTL_PRTAD_ALIGN                                 0
#define XAUI1_MDIO_CTL_PRTAD_BITS                                  5
#define XAUI1_MDIO_CTL_PRTAD_SHIFT                                 0


/****************************************************************************
 * xaui2 :: mdio_ctl
 ***************************************************************************/
/* xaui2 :: mdio_ctl :: reserved0 [15:13] */
#define XAUI2_MDIO_CTL_RESERVED0_MASK                              0xe000
#define XAUI2_MDIO_CTL_RESERVED0_ALIGN                             0
#define XAUI2_MDIO_CTL_RESERVED0_BITS                              3
#define XAUI2_MDIO_CTL_RESERVED0_SHIFT                             13

/* xaui2 :: mdio_ctl :: remotemdioreq [12:12] */
#define XAUI2_MDIO_CTL_REMOTEMDIOREQ_MASK                          0x1000
#define XAUI2_MDIO_CTL_REMOTEMDIOREQ_ALIGN                         0
#define XAUI2_MDIO_CTL_REMOTEMDIOREQ_BITS                          1
#define XAUI2_MDIO_CTL_REMOTEMDIOREQ_SHIFT                         12

/* xaui2 :: mdio_ctl :: md_st [11:11] */
#define XAUI2_MDIO_CTL_MD_ST_MASK                                  0x0800
#define XAUI2_MDIO_CTL_MD_ST_ALIGN                                 0
#define XAUI2_MDIO_CTL_MD_ST_BITS                                  1
#define XAUI2_MDIO_CTL_MD_ST_SHIFT                                 11

/* xaui2 :: mdio_ctl :: multimmds_strap [10:10] */
#define XAUI2_MDIO_CTL_MULTIMMDS_STRAP_MASK                        0x0400
#define XAUI2_MDIO_CTL_MULTIMMDS_STRAP_ALIGN                       0
#define XAUI2_MDIO_CTL_MULTIMMDS_STRAP_BITS                        1
#define XAUI2_MDIO_CTL_MULTIMMDS_STRAP_SHIFT                       10

/* xaui2 :: mdio_ctl :: md_devad [09:05] */
#define XAUI2_MDIO_CTL_MD_DEVAD_MASK                               0x03e0
#define XAUI2_MDIO_CTL_MD_DEVAD_ALIGN                              0
#define XAUI2_MDIO_CTL_MD_DEVAD_BITS                               5
#define XAUI2_MDIO_CTL_MD_DEVAD_SHIFT                              5

/* xaui2 :: mdio_ctl :: prtad [04:00] */
#define XAUI2_MDIO_CTL_PRTAD_MASK                                  0x001f
#define XAUI2_MDIO_CTL_PRTAD_ALIGN                                 0
#define XAUI2_MDIO_CTL_PRTAD_BITS                                  5
#define XAUI2_MDIO_CTL_PRTAD_SHIFT                                 0


/* XAUI1_MDIO_CTL_REG (Addr 4080h) */
#define READ_PHY8481_XAUI1_MDIO_CTL_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI1_MDIO_CTL_REG,(_val))

#define WRITE_PHY8481_XAUI1_MDIO_CTL_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI1_MDIO_CTL_REG,(_val))

#define MODIFY_PHY8481_XAUI1_MDIO_CTL_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI1_MDIO_CTL_REG,(_val),(_mask))

#define WRITE_PHY8481_XAUI1_MDIO_CTL_REG_MD_DEVAD(_unit, _phy_ctrl, _val) \
            MODIFY_PHY8481_XAUI1_MDIO_CTL_REG((_unit), (_phy_ctrl), \
                (_val)<<XAUI1_MDIO_CTL_MD_DEVAD_SHIFT, XAUI1_MDIO_CTL_MD_DEVAD_MASK)

#define WRITE_PHY8481_XAUI1_MDIO_CTL_REG_MULTIMMDS_STRAP(_unit, _phy_ctrl, _val) \
            MODIFY_PHY8481_XAUI1_MDIO_CTL_REG((_unit), (_phy_ctrl), \
                (_val)<<XAUI1_MDIO_CTL_MULTIMMDS_STRAP_SHIFT, XAUI1_MDIO_CTL_MULTIMMDS_STRAP_MASK)


/* XAUI2_MDIO_CTL_REG (Addr 4090h) */
#define READ_PHY8481_XAUI2_MDIO_CTL_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI2_MDIO_CTL_REG,(_val))

#define WRITE_PHY8481_XAUI2_MDIO_CTL_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI2_MDIO_CTL_REG,(_val))

#define MODIFY_PHY8481_XAUI2_MDIO_CTL_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI2_MDIO_CTL_REG,(_val),(_mask))

#define WRITE_PHY8481_XAUI2_MDIO_CTL_REG_PRTAD(_unit, _phy_ctrl, _val) \
            MODIFY_PHY8481_XAUI2_MDIO_CTL_REG((_unit), (_phy_ctrl), \
                (_val)<<XAUI2_MDIO_CTL_PRTAD_SHIFT, XAUI2_MDIO_CTL_PRTAD_MASK)

#define XAUI1_RESET_CTL_REG              0x4083
#define XAUI2_RESET_CTL_REG              0x4093

/* XAUI1_RESET_CTL (Addr 4083h) */

#define READ_PHY8481_XAUI1_RESET_CTL_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI1_RESET_CTL_REG,(_val))

#define WRITE_PHY8481_XAUI1_RESET_CTL_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI1_RESET_CTL_REG,(_val))

#define MODIFY_PHY8481_XAUI1_RESET_CTL_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI1_RESET_CTL_REG,(_val),(_mask))


/* XAUI2_RESET_CTL (Addr 4093h) */

#define READ_PHY8481_XAUI2_RESET_CTL_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI2_RESET_CTL_REG,(_val))

#define WRITE_PHY8481_XAUI2_RESET_CTL_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI2_RESET_CTL_REG,(_val))

#define MODIFY_PHY8481_XAUI2_RESET_CTL_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), XAUI2_RESET_CTL_REG,(_val),(_mask))

/* IFC XGPHY CTL */

#define IFC_XGPHY_CTL 0x40c0

#define READ_PHY8481_IFC_XGPHY_CTL(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), IFC_XGPHY_CTL,(_val))

#define WRITE_PHY8481_IFC_XGPHY_CTL(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), IFC_XGPHY_CTL,(_val))

#define MODIFY_PHY8481_IFC_XGPHY_CTL(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), IFC_XGPHY_CTL,(_val),(_mask))


/* Macros for MDIO2ARM operations */

#define MDIO2ARM_CTL  0xa817

/* PHYC_CTL :: MDIO2ARM_CTL :: reserved0 [15:06] */
#define MDIO2ARM_CTL_RESERVED0_MASK                       0xffc0
#define MDIO2ARM_CTL_RESERVED0_ALIGN                      0
#define MDIO2ARM_CTL_RESERVED0_BITS                       10
#define MDIO2ARM_CTL_RESERVED0_SHIFT                      6

/* PHYC_CTL :: MDIO2ARM_CTL :: MDIO2ARM_SELF_INC_ADDR [05:05] */
#define MDIO2ARM_CTL_MDIO2ARM_SELF_INC_ADDR_MASK          0x0020
#define MDIO2ARM_CTL_MDIO2ARM_SELF_INC_ADDR_ALIGN         0
#define MDIO2ARM_CTL_MDIO2ARM_SELF_INC_ADDR_BITS          1
#define MDIO2ARM_CTL_MDIO2ARM_SELF_INC_ADDR_SHIFT         5

/* PHYC_CTL :: MDIO2ARM_CTL :: MDIO2ARM_BURST [04:04] */
#define MDIO2ARM_CTL_MDIO2ARM_BURST_MASK                  0x0010
#define MDIO2ARM_CTL_MDIO2ARM_BURST_ALIGN                 0
#define MDIO2ARM_CTL_MDIO2ARM_BURST_BITS                  1
#define MDIO2ARM_CTL_MDIO2ARM_BURST_SHIFT                 4

/* PHYC_CTL :: MDIO2ARM_CTL :: MDIO2ARM_SIZE [03:02] */
#define MDIO2ARM_CTL_MDIO2ARM_SIZE_MASK                   0x000c
#define MDIO2ARM_CTL_MDIO2ARM_SIZE_ALIGN                  0
#define MDIO2ARM_CTL_MDIO2ARM_SIZE_BITS                   2
#define MDIO2ARM_CTL_MDIO2ARM_SIZE_SHIFT                  2

/* PHYC_CTL :: MDIO2ARM_CTL :: MDIO2ARM_RD [01:01] */
#define MDIO2ARM_CTL_MDIO2ARM_RD_MASK                     0x0002
#define MDIO2ARM_CTL_MDIO2ARM_RD_ALIGN                    0
#define MDIO2ARM_CTL_MDIO2ARM_RD_BITS                     1
#define MDIO2ARM_CTL_MDIO2ARM_RD_SHIFT                    1

/* PHYC_CTL :: MDIO2ARM_CTL :: MDIO2ARM_WR [00:00] */
#define MDIO2ARM_CTL_MDIO2ARM_WR_MASK                     0x0001
#define MDIO2ARM_CTL_MDIO2ARM_WR_ALIGN                    0
#define MDIO2ARM_CTL_MDIO2ARM_WR_BITS                     1
#define MDIO2ARM_CTL_MDIO2ARM_WR_SHIFT                    0


#define MDIO2ARM_STS  0xa818

/* PHYC_CTL :: MDIO2ARM_STS :: reserved0 [15:01] */
#define MDIO2ARM_STS_RESERVED0_MASK                       0xfffe
#define MDIO2ARM_STS_RESERVED0_ALIGN                      0
#define MDIO2ARM_STS_RESERVED0_BITS                       15
#define MDIO2ARM_STS_RESERVED0_SHIFT                      1

/* PHYC_CTL :: MDIO2ARM_STS :: MDIO2ARM_DONE [00:00] */
#define MDIO2ARM_STS_MDIO2ARM_DONE_MASK                   0x0001
#define MDIO2ARM_STS_MDIO2ARM_DONE_ALIGN                  0
#define MDIO2ARM_STS_MDIO2ARM_DONE_BITS                   1
#define MDIO2ARM_STS_MDIO2ARM_DONE_SHIFT                  0


#define MDIO2ARM_ADDR_LOW  0xa819
#define MDIO2ARM_ADDR_HIGH 0xa81a

#define MDIO2ARM_DATA_LOW  0xa81b
#define MDIO2ARM_DATA_HIGH 0xa81c

#define WRITE_PHY8481_MDIO2ARM_ADDR_LOW_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_ADDR_LOW, (_val))

#define WRITE_PHY8481_MDIO2ARM_ADDR_HIGH_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_ADDR_HIGH, (_val))


#define READ_PHY8481_MDIO2ARM_DATA_LOW_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_DATA_LOW, (_val))

#define READ_PHY8481_MDIO2ARM_DATA_HIGH_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_DATA_HIGH, (_val))

#define WRITE_PHY8481_MDIO2ARM_DATA_LOW_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_DATA_LOW, (_val))

#define WRITE_PHY8481_MDIO2ARM_DATA_HIGH_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_DATA_HIGH, (_val))


#define READ_PHY8481_MDIO2ARM_CTL_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_CTL,(_val))

#define WRITE_PHY8481_MDIO2ARM_CTL_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_CTL,(_val))

#define MODIFY_PHY8481_MDIO2ARM_CTL_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_CTL,(_val),(_mask))


#define READ_PHY8481_MDIO2ARM_STS_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_STS,(_val))

#define WRITE_PHY8481_MDIO2ARM_STS_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_STS,(_val))

#define MODIFY_PHY8481_MDIO2ARM_STS_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PMAD_REG((_unit), (_phy_ctrl), MDIO2ARM_STS,(_val),(_mask))


/*

The register is TOP_CONFIG_LINK_STATUS_REG, dev = 0x1e, mdio_addr = 0x400d or 0x418b. The bits are:
 
TOP_CONFIG_LINK_STATUS_REG[0] - fiber detected, 1=active, 0=inactive
TOP_CONFIG_LINK_STATUS_REG[1] - copper detected, 1=active, 0=inactive
TOP_CONFIG_LINK_STATUS_REG[2] - priority, 1=fiber priority, 0=copper priority
TOP_CONFIG_LINK_STATUS_REG[4:3] - copper speed, 11=10G, 10=1G, 01=100M, 00=10M
TOP_CONFIG_LINK_STATUS_REG[5] - copper link, 1=link up, 0=link down
TOP_CONFIG_LINK_STATUS_REG[7:6] - fiber speed, 11=10G, 10=1G, 01=100M, 00=10M
TOP_CONFIG_LINK_STATUS_REG[8] - fiber link, 1=link up, 0=link down
 
*/

#define TOP_CONFIG_LINK_STATUS_REG(_unit, _phy_ctrl)   ((PHY_MODEL((_phy_ctrl)->phy_id0, (_phy_ctrl)->phy_id1) == PHY_BCM8481X_MODEL) ? 0x418b : 0x400d )

#define TOP_CONFIG_LINK_STATUS_REG_FIBER_ACT    0x0001 /* fiber detected, 1=active, 0=inactive */
#define TOP_CONFIG_LINK_STATUS_REG_COPPER_ACT   0x0002 /* copper detected, 1=active, 0=inactive */
#define TOP_CONFIG_LINK_STATUS_REG_FIBER_PRIO   0x0004 /* 1=fiber priority, 0=copper priority */
#define TOP_CONFIG_LINK_STATUS_REG_COPPER_LINK  0x0020 /* copper link, 1=link up, 0=link down */
#define TOP_CONFIG_LINK_STATUS_REG_FIBER_LINK   0x0100 /* fiber link, 1=link up, 0=link down */

#define READ_PHY8481_TOP_CONFIG_LINK_STATUS_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_LINK_STATUS_REG((_unit), (_phy_ctrl)), (_val))

#define WRITE_PHY8481_TOP_CONFIG_LINK_STATUS_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_LINK_STATUS_REG((_unit), (_phy_ctrl)), (_val))

#define MODIFY_PHY8481_TOP_CONFIG_LINK_STATUS_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_LINK_STATUS_REG((_unit), (_phy_ctrl)), (_val), (_mask))

#define TOP_CONFIG_AUTOGREEN_CONFIG1_REG    0x400a

#define READ_PHY8481_TOP_CONFIG_AUTOGREEN_CONFIG1r(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_AUTOGREEN_CONFIG1_REG, (_val))

#define WRITE_PHY8481_TOP_CONFIG_AUTOGREEN_CONFIG1r(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_AUTOGREEN_CONFIG1_REG, (_val))

#define MODIFY_PHY8481_TOP_CONFIG_AUTOGREEN_CONFIG1r(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_AUTOGREEN_CONFIG1_REG, (_val), (_mask))

#define TOP_CONFIG_AUTOGREEN_THRESHOLD_LOW_REG    0x400c

#define READ_PHY8481_TOP_CONFIG_AUTOGREEN_THRESHOLD_LOWr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_AUTOGREEN_THRESHOLD_LOW_REG, (_val))

#define WRITE_PHY8481_TOP_CONFIG_AUTOGREEN_THRESHOLD_LOWr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_AUTOGREEN_THRESHOLD_LOW_REG, (_val))

#define MODIFY_PHY8481_TOP_CONFIG_AUTOGREEN_THRESHOLD_LOWr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_AUTOGREEN_THRESHOLD_LOW_REG, (_val), (_mask))


#define TOP_CONFIG_AUTOGREEN_THRESHOLD_HIGH_REG    0x400b

#define READ_PHY8481_TOP_CONFIG_AUTOGREEN_THRESHOLD_HIGHr(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_AUTOGREEN_THRESHOLD_HIGH_REG, (_val))

#define WRITE_PHY8481_TOP_CONFIG_AUTOGREEN_THRESHOLD_HIGHr(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_AUTOGREEN_THRESHOLD_HIGH_REG, (_val))

#define MODIFY_PHY8481_TOP_CONFIG_AUTOGREEN_THRESHOLD_HIGHr(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_AUTOGREEN_THRESHOLD_HIGH_REG, (_val), (_mask))


#define TOP_CONFIG_CABLE_DIAG_CTRL_STAT_REG(_unit, _phy_ctrl)   ((PHY_MODEL((_phy_ctrl)->phy_id0, (_phy_ctrl)->phy_id1) == \
                                                                  PHY_BCM8481X_MODEL) ? 0x402A : 0x4006 )

#define READ_PHY8481_TOP_CONFIG_CABLE_DIAG_CTRL_STAT_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_CABLE_DIAG_CTRL_STAT_REG((_unit), (_phy_ctrl)), (_val))

#define WRITE_PHY8481_TOP_CONFIG_CABLE_DIAG_CTRL_STAT_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_CABLE_DIAG_CTRL_STAT_REG((_unit), (_phy_ctrl)), (_val))

#define MODIFY_PHY8481_TOP_CONFIG_CABLE_DIAG_CTRL_STAT_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_CABLE_DIAG_CTRL_STAT_REG((_unit), (_phy_ctrl)), (_val), (_mask))


#define PHYC_CTL_CABLE_DIAG_RESULT_REG(_unit, _phy_ctrl)   ((PHY_MODEL((_phy_ctrl)->phy_id0, (_phy_ctrl)->phy_id1) == \
                                                             PHY_BCM8481X_MODEL) ? 0xA863 : 0xA896 )
#define READ_PHY8481_PHYC_CTL_CABLE_DIAG_RESULT_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHYC_CTL_CABLE_DIAG_RESULT_REG((_unit), (_phy_ctrl)), (_val))


#define PHYC_CTL_CABLE_DIAG_PAIR12_LEN_REG(_unit, _phy_ctrl)   ((PHY_MODEL((_phy_ctrl)->phy_id0, (_phy_ctrl)->phy_id1) == \
                                                                 PHY_BCM8481X_MODEL) ? 0xA864 : 0xA897 )
#define READ_PHY8481_PHYC_CTL_CABLE_DIAG_PAIR12_LEN_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHYC_CTL_CABLE_DIAG_PAIR12_LEN_REG((_unit), (_phy_ctrl)), (_val))


#define PHYC_CTL_CABLE_DIAG_PAIR34_LEN_REG(_unit, _phy_ctrl)   ((PHY_MODEL((_phy_ctrl)->phy_id0, (_phy_ctrl)->phy_id1) == \
                                                                 PHY_BCM8481X_MODEL) ? 0xA865 : 0xA898 )
#define READ_PHY8481_PHYC_CTL_CABLE_DIAG_PAIR34_LEN_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHYC_CTL_CABLE_DIAG_PAIR34_LEN_REG((_unit), (_phy_ctrl)), (_val))


#define PHYC_CTL_CABLE_DIAG_PAIR56_LEN_REG(_unit, _phy_ctrl)   ((PHY_MODEL((_phy_ctrl)->phy_id0, (_phy_ctrl)->phy_id1) == \
                                                                 PHY_BCM8481X_MODEL) ? 0xA866 : 0xA899 )
#define READ_PHY8481_PHYC_CTL_CABLE_DIAG_PAIR56_LEN_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHYC_CTL_CABLE_DIAG_PAIR56_LEN_REG((_unit), (_phy_ctrl)), (_val))


#define PHYC_CTL_CABLE_DIAG_PAIR78_LEN_REG(_unit, _phy_ctrl)   ((PHY_MODEL((_phy_ctrl)->phy_id0, (_phy_ctrl)->phy_id1) == \
                                                                 PHY_BCM8481X_MODEL) ? 0xA867 : 0xA89A )
#define READ_PHY8481_PHYC_CTL_CABLE_DIAG_PAIR78_LEN_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), PHYC_CTL_CABLE_DIAG_PAIR78_LEN_REG((_unit), (_phy_ctrl)), (_val))


#define READ_PHY8481_PMD_IEEE_TST_MODE_10G_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), 0x0084, (_val))

#define WRITE_PHY8481_PMD_IEEE_TST_MODE_10G_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), 0x0084, (_val))

#define MODIFY_PHY8481_PMD_IEEE_TST_MODE_10G_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PMAD_REG((_unit), (_phy_ctrl), 0x0084, (_val), (_mask))


#define READ_PHY8481_PHYC_CTL_SPARE_GRP5_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), 0xa89b, (_val))

#define WRITE_PHY8481_PHYC_CTL_SPARE_GRP5_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), 0xa89b, (_val))

#define MODIFY_PHY8481_PHYC_CTL_SPARE_GRP5_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PMAD_REG((_unit), (_phy_ctrl), 0xa89b, (_val), (_mask))


#define READ_PHY8481_PHYC_CTL_SPARE_GRP6_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_PMAD_REG((_unit), (_phy_ctrl), 0xa89c, (_val))

#define WRITE_PHY8481_PHYC_CTL_SPARE_GRP6_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_PMAD_REG((_unit), (_phy_ctrl), 0xa89c, (_val))

#define MODIFY_PHY8481_PHYC_CTL_SPARE_GRP6_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_PMAD_REG((_unit), (_phy_ctrl), 0xa89c, (_val), (_mask))


#define TOP_CONFIG_XGPHY_STRAP1_REG  0x401a

#define READ_PHY8481_TOP_CONFIG_XGPHY_STRAP1_REG(_unit, _phy_ctrl, _val) \
            READ_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_XGPHY_STRAP1_REG,(_val))

#define WRITE_PHY8481_TOP_CONFIG_XGPHY_STRAP1_REG(_unit, _phy_ctrl, _val) \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_XGPHY_STRAP1_REG,(_val))

#define MODIFY_PHY8481_TOP_CONFIG_XGPHY_STRAP1_REG(_unit, _phy_ctrl, _val, _mask) \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_phy_ctrl), \
            TOP_CONFIG_XGPHY_STRAP1_REG,(_val),(_mask))


#define READ_PHY84834_TOP_CONFIG_SCRATCH_0r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4005, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_0r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4005, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_0r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4005,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_1r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4006, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_1r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4006, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_1r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4006,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_2r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4007, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_2r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4007, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_2r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4007,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_3r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4008, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_3r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4008, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_3r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4008,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_4r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4009, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_4r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4009, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_4r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4009,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_5r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400a, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_5r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400a, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_5r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400a,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_6r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400b, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_6r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400b, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_6r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400b,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_7r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400c, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_7r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400c, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_7r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400c,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_8r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400d, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_8r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400d, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_8r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400d,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_9r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400e, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_9r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400e, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_9r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400e,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_10r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400f, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_10r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400f, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_10r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x400f,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_11r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4010, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_11r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4010, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_11r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4010,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_12r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4011, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_12r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4011, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_12r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4011,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_13r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4012, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_13r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4012, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_13r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4012,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_14r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4013, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_14r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4013, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_14r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4013,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_15r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4014, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_15r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4014, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_15r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4014,       \
                                     (_val), (_mask))


#define READ_PHY84834_TOP_CONFIG_SCRATCH_26r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4037, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_26r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4037, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_26r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4037,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_27r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4038, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_27r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4038, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_27r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4038,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_28r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4039, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_28r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4039, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_28r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x4039,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_29r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x403a, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_29r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x403a, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_29r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x403a,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_30r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x403b, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_30r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x403b, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_30r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x403b,       \
                                     (_val), (_mask))

#define READ_PHY84834_TOP_CONFIG_SCRATCH_31r(_unit, _pc, _val)                 \
            READ_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x403c, (_val))
#define WRITE_PHY84834_TOP_CONFIG_SCRATCH_31r(_unit, _pc, _val)                 \
            WRITE_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x403c, (_val))
#define MODIFY_PHY84834_TOP_CONFIG_SCRATCH_31r(_unit, _pc, _val, _mask)         \
            MODIFY_PHY8481_TOPLVL1_REG((_unit), (_pc), 0x403c,       \
                                     (_val), (_mask))


#define    PHY84834_DIAG_CMD_NOP                                     0
#define    PHY84834_DIAG_CMD_BLK_LF_RF_ENABLE                        1
#define    PHY84834_DIAG_CMD_PAIR_SWAP_CHANGE                        2
#define    PHY84834_DIAG_CMD_3                                       3
#define    PHY84834_DIAG_CMD_DISABLE_TRAFFIC_LED                     4
#define    PHY84834_DIAG_CMD_5                                       5
#define    PHY84834_DIAG_CMD_6                                       6
#define    PHY84834_DIAG_CMD_7                                       7
#define    PHY84834_DIAG_CMD_DISABLE_RX_LF                           8
#define    PHY84834_DIAG_CMD_GET_HEALTH_MONITOR_STATUS               9
#define    PHY84834_DIAG_CMD_CLEAR_HEALTH_MONITOR_STATUS             10
#define    PHY84834_DIAG_CMD_GET_MATLAB_VARIABLE_ADDRESSES           11
#define    PHY84834_DIAG_CMD_SET_UPPER_TEMP_WARNING_LEVEL            12
#define    PHY84834_DIAG_CMD_GET_UPPER_TEMP_WARNING_LEVEL            13
#define    PHY84834_DIAG_CMD_SET_LOWER_TEMP_WARNING_LEVEL            14
#define    PHY84834_DIAG_CMD_GET_LOWER_TEMP_WARNING_LEVEL            15
#define    PHY84834_DIAG_CMD_16                                      16
#define    PHY84834_DIAG_CMD_GET_CURRENT_TEMP                        17
#define    PHY84834_DIAG_CMD_GET_DATA_BUF_ADDRESSES                  18
#define    PHY84834_DIAG_CMD_PEEK_WORD                               19
#define    PHY84834_DIAG_CMD_POKE_WORD                               20
#define    PHY84834_DIAG_CMD_GET_EXP_VALUES                          21
#define    PHY84834_DIAG_CMD_SET_MASTER_LOG_ENABLE                   22
#define    PHY84834_DIAG_CMD_GET_MASTER_LOG_ENABLE                   23
#define    PHY84834_DIAG_CMD_SET_MASTER_HANG_PC_CAPTURE_ENABLE       24
#define    PHY84834_DIAG_CMD_GET_MASTER_HANG_PC_CAPTURE_ENABLE       25
#define    PHY84834_DIAG_CMD_GET_AUTONEG_RESULTS                     26
#define    PHY84834_DIAG_CMD_GET_XAUI1_REG_VALUES                    27                            
#define    PHY84834_DIAG_CMD_SET_XAUI1_REG_VALUES                    28                            
#define    PHY84834_DIAG_CMD_GET_MACSEC_ENABLE                       33                            
#define    PHY84834_DIAG_CMD_SET_MACSEC_ENABLE                       34                            
#define    PHY84834_DIAG_CMD_GET_1588_ENABLE                         35                            
#define    PHY84834_DIAG_CMD_SET_1588_ENABLE                         36                           
#define    PHY84834_DIAG_CMD_GET_SHORT_REACH_ENABLE                  37
#define    PHY84834_DIAG_CMD_SET_SHORT_REACH_ENABLE                  38
#define    PHY84834_DIAG_CMD_GET_EEE_MODE                            39
#define    PHY84834_DIAG_CMD_SET_EEE_MODE                            40
#define    PHY84834_DIAG_CMD_GET_PAIR_SWAP                           41
#define    PHY84834_DIAG_CMD_GET_EMI_MODE_ENABLE                     42
#define    PHY84834_DIAG_CMD_SET_EMI_MODE_ENABLE                     43
#define    PHY84834_DIAG_CMD_GET_AUTONEG_RESULTS_ENHANCED            44

#define    PHY84834_CMD_RECEIVED                                     0x0001
#define    PHY84834_CMD_IN_PROGRESS                                  0x0002
#define    PHY84834_CMD_COMPLETE_PASS                                0x0004
#define    PHY84834_CMD_COMPLETE_ERROR                               0x0008
#define    PHY84834_CMD_OPEN_FOR_CMDS                                0x0010
#define    PHY84834_CMD_SYSTEM_BOOT                                  0x0020
#define    PHY84834_CMD_NOT_OPEN_FOR_CMDS                            0x0040
#define    PHY84834_CMD_CLEAR_COMPLETE                               0x0080
#define    PHY84834_CMD_OPEN_OVERRIDE                                0xA5A5

/* Command Handler Version 2 */
#define    PHY84834_DIAG_CMD_NOP_V2                                  0x0000

#define    PHY84834_DIAG_CMD_GET_PAIR_SWAP_V2                        0x8000
#define    PHY84834_DIAG_CMD_SET_PAIR_SWAP_V2                        0x8001
#define    PHY84834_DIAG_CMD_GET_MACSEC_ENABLE_V2                    0x8002                            
#define    PHY84834_DIAG_CMD_SET_MACSEC_ENABLE_V2                    0x8003                            
#define    PHY84834_DIAG_CMD_GET_1588_ENABLE_V2                      0x8004                            
#define    PHY84834_DIAG_CMD_SET_1588_ENABLE_V2                      0x8005                           
#define    PHY84834_DIAG_CMD_GET_SHORT_REACH_ENABLE_V2               0x8006
#define    PHY84834_DIAG_CMD_SET_SHORT_REACH_ENABLE_V2               0x8007
#define    PHY84834_DIAG_CMD_GET_EEE_MODE_V2                         0x8008
#define    PHY84834_DIAG_CMD_SET_EEE_MODE_V2                         0x8009
#define    PHY84834_DIAG_CMD_GET_EMI_MODE_ENABLE_V2                  0x800a
#define    PHY84834_DIAG_CMD_SET_EMI_MODE_ENABLE_V2                  0x800b
#define    PHY84834_DIAG_CMD_GET_SNR_V2                              0x8030
#define    PHY84834_DIAG_CMD_GET_CURRENT_TEMP_V2                     0x8031
#define    PHY84834_DIAG_CMD_SET_UPPER_TEMP_WARNING_LEVEL_V2         0x8032
#define    PHY84834_DIAG_CMD_GET_UPPER_TEMP_WARNING_LEVEL_V2         0x8033
#define    PHY84834_DIAG_CMD_SET_LOWER_TEMP_WARNING_LEVEL_V2         0x8034
#define    PHY84834_DIAG_CMD_GET_LOWER_TEMP_WARNING_LEVEL_V2         0x8035

#define    PHY84834_DIAG_CMD_GET_XAUI_M_REG_VALUES_V2                0x8100
#define    PHY84834_DIAG_CMD_SET_XAUI_M_REG_VALUES_V2                0x8101
#define    PHY84834_DIAG_CMD_GET_XFI_M_REG_VALUES_V2                 0x8102
#define    PHY84834_DIAG_CMD_SET_XFI_M_REG_VALUES_V2                 0x8103
#define    PHY84834_DIAG_CMD_GET_XAUI_L_REG_VALUES_V2                0x8104
#define    PHY84834_DIAG_CMD_SET_XAUI_L_REG_VALUES_V2                0x8105
#define    PHY84834_DIAG_CMD_GET_XFI_L_REG_VALUES_V2                 0x8106
#define    PHY84834_DIAG_CMD_SET_XFI_L_REG_VALUES_V2                 0x8107


#define    PHY84834_DIAG_CMD_PEEK_WORD_V2                            0xc000
#define    PHY84834_DIAG_CMD_POKE_WORD_V2                            0xc001
#define    PHY84834_DIAG_CMD_GET_DATA_BUF_ADDRESSES_V2               0xc002

/*
 * PTP_INT_EN
 * 
 *
*/ 
#define PHY84834_PTP_INT_EN_REG (0xd808)

#define PHY84834_PTP_INT_PIC_EN   (1U<<1)
#define PHY84834_PTP_INT_FSYNC_EN (1U<<0)


/*
 * PTP_INT_STATUS
 * 
 *
*/ 
#define PHY84834_PTP_INT_STATUS_REG (0xd809)

#define PHY84834_PTP_INT_STATUS_PIC   (1U<<1)
#define PHY84834_PTP_INT_STATUS_FSYNC (1U<<0)

/*
 * PTP_INT_CLEAR
 * 
 *
*/ 
#define PHY84834_PTP_INT_CLEAR_REG (0xd80a)

#define PHY84834_PTP_INT_CLEAR_PIC   (1U<<1)
#define PHY84834_PTP_INT_CLEAR_FSYNC (1U<<0)

/*
 * PTP_NSE_SHDW_CTL
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_CTL_REG (0xd882)

#define PHY84834_PTP_NSW_SHDW_CTL_LOAD_TIME_HIGH  (1U<<12)
#define PHY84834_PTP_NSW_SHDW_CTL_LOAD_TIME_LOW   (1U<<11)
#define PHY84834_PTP_NSE_SHDW_CTL_LOAD_NCO_FCW    (1U<<10)
#define PHY84834_PTP_NSE_SHDW_CTL_LOAD_DPLL_FCW   (1U<<9)
#define PHY84834_PTP_NSE_SHDW_CTL_LOAD_REF_PHASE  (1U<<8)
#define PHY84834_PTP_NSE_SHDW_CTL_LOAD_REF_PERIOD (1U<<7)
#define PHY84834_PTP_NSE_SHDW_CTL_LOAD_DPLL_GAIN3 (1U<<6)
#define PHY84834_PTP_NSE_SHDW_CTL_LOAD_DPLL_GAIN2 (1U<<5)
#define PHY84834_PTP_NSE_SHDW_CTL_LOAD_DPLL_GAIN1 (1U<<4)
#define PHY84834_PTP_NSE_SHDW_CTL_LOAD_SYNCOUT    (1U<<3)
#define PHY84834_PTP_NSE_SHDW_CTL_LOAD_SYNC_DIV   (1U<<2)
#define PHY84834_PTP_NSE_SHDW_CTL_LOAD_ORIG_TS    (1U<<0)


/*
 * PTP_NSE_SHDW_CLR
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_CLR_REG (0xd883)

#define PHY84834_PTP_NSW_SHDW_CLR_LOAD_TIME_HIGH  (1U<<12)
#define PHY84834_PTP_NSW_SHDW_CLR_LOAD_TIME_LOW   (1U<<11)
#define PHY84834_PTP_NSE_SHDW_CLR_LOAD_NCO_FCW    (1U<<10)
#define PHY84834_PTP_NSE_SHDW_CLR_LOAD_DPLL_FCW   (1U<<9)
#define PHY84834_PTP_NSE_SHDW_CLR_LOAD_REF_PHASE  (1U<<8)
#define PHY84834_PTP_NSE_SHDW_CLR_LOAD_REF_PERIOD (1U<<7)
#define PHY84834_PTP_NSE_SHDW_CLR_LOAD_DPLL_GAIN3 (1U<<6)
#define PHY84834_PTP_NSE_SHDW_CLR_LOAD_DPLL_GAIN2 (1U<<5)
#define PHY84834_PTP_NSE_SHDW_CLR_LOAD_DPLL_GAIN1 (1U<<4)
#define PHY84834_PTP_NSE_SHDW_CLR_LOAD_SYNCOUT    (1U<<3)
#define PHY84834_PTP_NSE_SHDW_CLR_LOAD_SYNC_DIV   (1U<<2)
#define PHY84834_PTP_NSE_SHDW_CLR_LOAD_ORIG_TS    (1U<<0)

/*
 * PTP_NSE_SHDW_ORG_TS_0
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_ORG_TS_0_REG (0xd890)


/*
 * PTP_NSE_SHDW_ORG_TS_1
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_ORG_TS_1_REG (0xd891)


/*
 * PTP_NSE_SHDW_ORG_TS_2
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_ORG_TS_2_REG (0xd892)

/*
 * PTP_NSE_SHDW_ORG_TS_3
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_ORG_TS_3_REG (0xd893)

/*
 * PTP_NSE_SHDW_ORG_TS_4
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_ORG_TS_4_REG (0xd894)


/*
 * PTP_NSE_SC_CFG
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SC_CFG_REG (0xd8a0)

#define PHY84834_PTP_NSE_SC_CFG_FSYNC_TS_EN (1U<<13)
#define PHY84834_PTP_NSE_SC_CFG_FSYNC_EN    (1U<<12)
#define PHY84834_PTP_NSE_SC_CFG_SO_MODE     (1U<<9 | 1U<<8)
#define PHY84834_PTP_NSE_SC_CFG_SI_MODE     (1U<<6 | 1U<<5 | 1U<<4)
#define PHY84834_PTP_NSE_SC_CFG_GS_MODE     (1U<<1 | 1U<<0)


/*
 * PTP_NSE_SYNCIN_FSYNC_LEN_0
 * 
 *
*/ 
#define PHY84834_NSE_SYNCIN_FSYNC_LEN_0_REG (0xd8a1)


/*
 * PTP_NSE_SYNCIN_FSYNC_LEN_1
 * 
 *
*/ 
#define PHY84834_NSE_SYNCIN_FSYNC_LEN_1_REG (0xd8a2)

/*
 * PTP_NSE_SYNCIN_FSYNC_DLY_0
 * 
 *
*/ 
#define PHY84834_NSE_SYNCIN_FSYNC_DLY_0_REG (0xd8a3)


/*
 * PTP_NSE_SYNCIN_FSYNC_DLY_1
 * 
 *
*/ 
#define PHY84834_NSE_SYNCIN_FSYNC_DLY_1_REG (0xd8a4)


/*
 * PTP_NSE_SOUT_TS_CMP_0
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SOUT_TS_CMP_0_REG (0xd8a5)


/*
 * PTP_NSE_SOUT_TS_CMP_1
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SOUT_TS_CMP_1_REG (0xd8a6)

/*
 * PTP_NSE_SOUT_TS_CMP_2
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SOUT_TS_CMP_2_REG (0xd8a7)

/*
 * PTP_NSE_SHDW_DPLL_SYNC_DIV
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_DPLL_SYNC_DIV_REG (0xd8a8)


/*
 * PTP_NSE_SHDW_SOUT_INTERVAL_0
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_SOUT_INTERVAL_0_REG (0xd8a9)


/*
 * PTP_NSE_SHDW_SOUT_INTERVAL_1
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_SOUT_INTERVAL_1_REG (0xd8aa)

/*
 * PTP_NSE_SHDW_SOUT_PULSE1_LEN
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_SOUT_PULSE1_LEN_REG (0xd8ab)


/*
 * PTP_NSE_SHDW_SOUT_PULSE2_LEN
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SHDW_SOUT_PULSE2_LEN_REG (0xd8ac)


/*
 * PTP_NSE_SC_TS_FSYNC_0
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SC_TS_FSYNC_0_REG (0xd8b0)


/*
 * PTP_NSE_SC_TS_FSYNC_1
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SC_TS_FSYNC_1_REG (0xd8b1)


/*
 * PTP_NSE_SC_TS_FSYNC_2
 * 
 *
*/ 
#define PHY84834_PTP_NSE_SC_TS_FSYNC_2_REG (0xd8b2)


/*
 * PTP_NSE_DPLL_SHDW_GAIN_1
 * 
 *
*/ 
#define PHY84834_PTP_NSE_DPLL_SHDW_GAIN_1_REG (0xd8c1)


/*
 * PTP_NSE_DPLL_SHDW_GAIN_2
 * 
 *
*/ 
#define PHY84834_PTP_NSE_DPLL_SHDW_GAIN_2_REG (0xd8c2)


/*
 * PTP_NSE_DPLL_SHDW_GAIN_3
 * 
 *
*/ 
#define PHY84834_PTP_NSE_DPLL_SHDW_GAIN_3_REG (0xd8c3)


/*
 * PTP_NSE_DPLL_SHDW_REF_PERIOD_0
 * 
 *
*/ 
#define PHY84834_PTP_NSE_DPLL_SHDW_REF_PERIOD_0_REG (0xd8c4)

/*
 * PTP_NSE_DPLL_SHDW_REF_PERIOD_1
 * 
 *
*/ 
#define PHY84834_PTP_NSE_DPLL_SHDW_REF_PERIOD_1_REG (0xd8c5)


/*
 * PTP_NSE_DPLL_SHDW_REF_PHASE_0
 * 
 *
*/ 
#define PHY84834_PTP_NSE_DPLL_SHDW_REF_PHASE_0_REG (0xd8c6)


/*
 * PTP_NSE_SHDW_REF_PHASE_1
 * 
 *
*/ 
#define PHY84834_PTP_NSE_DPLL_SHDW_REF_PHASE_1_REG (0xd8c7)


/*
 * PTP_NSE_SHDW_REF_PHASE_2
 * 
 *
*/ 
#define PHY84834_PTP_NSE_DPLL_SHDW_REF_PHASE_2_REG (0xd8c8)


/*
 * PTP_NSE_NCO_CFG
 * 
 *
*/ 
#define PHY84834_PTP_NSE_NCO_CFG_REG (0xd8e0)

#define PHY84834_PTP_NSE_NCO_CFG_FCW_SEL (1U<<0)
#define NCO_SHADOW_REG_STATIC_FCW 0
#define NCO_DYNAMIC_DPLL_FCW 1


/*
 * PTP_NSE_NCO_SHADOW_FCW_0
 * 
 *
*/ 
#define PHY84834_NSE_NCO_SHADOW_FCW_0_REG (0xd8e1)


/*
 * PTP_NSE_NCO_SHADOW_FCW_1
 * 
 *
*/ 
#define PHY84834_NSE_NCO_SHADOW_FCW_1_REG (0xd8e2)


/*
 * PTP_NSE_NCO_SHADOW_TIME_HIGH_0
 * 
 *
*/ 
#define PHY84834_NSE_NCO_SHADOW_TIME_HIGH_0_REG (0xd8e5)


/*
 * PTP_NSE_NCO_SHADOW_TIME_HIGH_1
 * 
 *
*/ 
#define PHY84834_NSE_NCO_SHADOW_TIME_HIGH_1_REG (0xd8e6)


/*
 * PTP_NSE_NCO_SHADOW_TIME_HIGH_2
 * 
 *
*/ 
#define PHY84834_NSE_NCO_SHADOW_TIME_HIGH_2_REG (0xd8e7)


/*
 * PTP_TS_EG_SOP_OFFSET_0
 * 
 *
*/ 
#define PHY84834_PTP_TS_EG_SOP_OFFSET_0_REG (0xd908)


/*
 * PTP_TS_EG_SOP_OFFSET_1
 * 
 *
*/ 
#define PHY84834_PTP_TS_EG_SOP_OFFSET_1_REG (0xd909)

/*
 * PTP_TS_IG_SOP_OFFSET_0
 * 
 *
*/ 
#define PHY84834_PTP_TS_IG_SOP_OFFSET_0_REG (0xd910)


/*
 * PTP_TS_IG_SOP_OFFSET_1
 * 
 *
*/ 
#define PHY84834_PTP_TS_IG_SOP_OFFSET_1_REG (0xd911)



/*
 * PTP_VLAN_1TAG_TPID
 * 
 *
*/ 
#define PHY84834_PTP_VLAN_1TAG_TPID_REG (0xd990)

/*
 * PTP_VLAN_2TAG_OTPID
 * 
 *
*/ 
#define PHY84834_PTP_VLAN_2TAG_OTPID_REG (0xd991)

/*
 * PTP_VLAN_2TAG_OTPID_1
 * 
 *
*/ 
#define PHY84834_PTP_VLAN_2TAG_OTPID_1_REG (0xd992)


/*
 * PTP_EG_CFG
 * 
 *
*/ 
#define PHY84834_PTP_EG_CFG_REG (0xda01)

#define PHY84834_PTP_EG_CFG_L4_IP6_UDP_ACH_EN (1U<<13)
#define PHY84834_PTP_EG_CFG_L4_IP4_UDP_ACH_EN (1U<<9)
#define PHY84834_PTP_EG_CFG_L2_802AS_ACH_EN   (1U<<8)
#define PHY84834_PTP_EG_CFG_L4_IP6_UDP_EN     (1U<<6)
#define PHY84834_PTP_EG_CFG_L4_IP4_UDP_EN     (1U<<5)
#define PHY84834_PTP_EG_CFG_L2_802AS_EN       (1U<<4)
#define PHY84834_PTP_EG_CFG_FCS_CHECK_EN      (1U<<0)

/*
 * PTP_EG_PTP_PKT_CFG
 * 
 *
*/ 
#define PHY84834_PTP_EG_PTP_PKT_CFG_REG (0xda08)

#define PHY84834_PTP_EG_PTP_PKT_CFG_ACTION_PTP_SYNC   (0x7 << 0)
#define PHY84834_PTP_EG_PTP_PKT_CFG_CAPT_PTP_SYNC     (0x8 << 0)
#define PHY84834_PTP_EG_PTP_PKT_CFG_ACTION_PTP_DREQ   (0x7 << 4)
#define PHY84834_PTP_EG_PTP_PKT_CFG_CAPT_PTP_DREQ     (0x8 << 4)
#define PHY84834_PTP_EG_PTP_PKT_CFG_ACTION_PTP_PDREQ  (0x7 << 8)
#define PHY84834_PTP_EG_PTP_PKT_CFG_CAPT_PTP_PDREQ    (0x8 << 8)
#define PHY84834_PTP_EG_PTP_PKT_CFG_ACTION_PTP_PDRESP (0x7 << 12) 
#define PHY84834_PTP_EG_PTP_PKT_CFG_CAPT_PTP_PDRESP   (0x8 << 12) 


/*
 * PTP_IG_CFG
 * 
 *
*/ 
#define PHY84834_PTP_IG_CFG_REG (0xda81)

#define PHY84834_PTP_IG_CFG_L4_IP6_UDP_ACH_EN (1U<<13)
#define PHY84834_PTP_IG_CFG_L4_IP4_UDP_ACH_EN (1U<<9)
#define PHY84834_PTP_IG_CFG_L2_802AS_ACH_EN   (1U<<8)
#define PHY84834_PTP_IG_CFG_L4_IP6_UDP_EN     (1U<<6)
#define PHY84834_PTP_IG_CFG_L4_IP4_UDP_EN     (1U<<5)
#define PHY84834_PTP_IG_CFG_L2_802AS_EN       (1U<<4)
#define PHY84834_PTP_IG_CFG_FCS_CHECK_EN      (1U<<0)


/*
 * PTP_IG_PTP_PKT_CFG
 * 
 *
*/ 
#define PHY84834_PTP_IG_PTP_PKT_CFG_REG (0xda88)

#if 0
#define PHY84834_PTP_IG_PTP_PKT_CFG_ACTION_PTP_SYNC   (0x7)
#define PHY84834_PTP_IG_PTP_PKT_CFG_CAPT_PTP_SYNC     (1U<<3)
#define PHY84834_PTP_IG_PTP_PKT_CFG_ACTION_PTP_DREQ   (0x07)
#define PHY84834_PTP_IG_PTP_PKT_CFG_CAPT_PTP_DREQ     (1U<<7)
#define PHY84834_PTP_IG_PTP_PKT_CFG_ACTION_PTP_PDREQ  (0x007)
#define PHY84834_PTP_IG_PTP_PKT_CFG_CAPT_PTP_PDREQ    (1U<<11)
#define PHY84834_PTP_IG_PTP_PKT_CFG_ACTION_PTP_PDRESP (0x7000) 
#define PHY84834_PTP_IG_PTP_PKT_CFG_CAPT_PTP_PDRESP   (1U<<15) 
#endif

#define PHY84834_PTP_IG_PTP_PKT_CFG_ACTION_PTP_SYNC   (0x7 << 0)
#define PHY84834_PTP_IG_PTP_PKT_CFG_CAPT_PTP_SYNC     (0x8 << 0)
#define PHY84834_PTP_IG_PTP_PKT_CFG_ACTION_PTP_DREQ   (0x7 << 4)
#define PHY84834_PTP_IG_PTP_PKT_CFG_CAPT_PTP_DREQ     (0x8 << 4)
#define PHY84834_PTP_IG_PTP_PKT_CFG_ACTION_PTP_PDREQ  (0x7 << 8)
#define PHY84834_PTP_IG_PTP_PKT_CFG_CAPT_PTP_PDREQ    (0x8 << 8)
#define PHY84834_PTP_IG_PTP_PKT_CFG_ACTION_PTP_PDRESP (0x7 << 12)
#define PHY84834_PTP_IG_PTP_PKT_CFG_CAPT_PTP_PDRESP   (0x8 << 12)


#define PHY84834_PTP_PARSER_EG_CNTR_ALL_PKT_EN        (0xda20)

#define PHY84834_PTP_PARSER_EG_CNTR_ALL_PKT_CLR       (0xda21)

#define PHY84834_PTP_PARSER_EG_CNTR_ALL_PKT_0         (0xda28)

#define PHY84834_PTP_PARSER_EG_CNTR_ALL_PKT_1         (0xda29)

#define PHY84834_PTP_PARSER_EG_CNTR_PTP_PKT_EN        (0xda30)

#define PHY84834_PTP_PARSER_EG_CNTR_PTP_PKT_CLR       (0xda31)

#define PHY84834_PTP_PARSER_EG_CNTR_PTP_PKT           (0xda38)




#define PHY84834_PTP_PARSER_IG_CNTR_ALL_PKT_EN        (0xdaa0)

#define PHY84834_PTP_PARSER_IG_CNTR_ALL_PKT_CLR       (0xdaa1)

#define PHY84834_PTP_PARSER_IG_CNTR_ALL_PKT_0         (0xdaa8)

#define PHY84834_PTP_PARSER_IG_CNTR_ALL_PKT_1         (0xdaa9)

#define PHY84834_PTP_PARSER_IG_CNTR_PTP_PKT_EN        (0xdab0)

#define PHY84834_PTP_PARSER_IG_CNTR_PTP_PKT_CLR       (0xdab1)

#define PHY84834_PTP_PARSER_IG_CNTR_PTP_PKT           (0xdab8)

#define PHY84834_PTP_PIC_FIFO_CTL_REG                 (0xdb08)

#define PHY84834_PTP_PIC_FIFO_STAT_REG                (0xdb09)

/*
 * PTP_PIC_FIFO_RD_PKT_TS_0
 * 
 *
*/ 
#define PHY84834_PTP_PIC_FIFO_RD_PKT_TS_0_REG (0xdb10)


/*
 * PTP_PIC_FIFO_RD_PKT_TS_1
 * 
 *
*/ 
#define PHY84834_PTP_PIC_FIFO_RD_PKT_TS_1_REG (0xdb11)


/*
 * PTP_PIC_FIFO_RD_PKT_TS_2
 * 
 *
*/ 
#define PHY84834_PTP_PIC_FIFO_RD_PKT_TS_2_REG (0xdb12)


#endif  /* _PHY8481_H_ */
