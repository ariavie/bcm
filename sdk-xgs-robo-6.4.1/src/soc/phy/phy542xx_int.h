/*
 * $Id: phy542xx_int.h,v 1.6 Broadcom SDK $ 
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

#ifndef _PHY542XX_INT_H
#define _PHY542XX_INT_H

/* Standard MII Registers */

#define PHY_BCM542XX_MII_CTRL_REG            0x00  /* MII Control Register : r/w */
#define PHY_BCM542XX_MII_STAT_REG            0x01  /* MII Status Register: ro */
#define PHY_BCM542XX_MII_PHY_ID0_REG         0x02  /* MII PHY ID register: r/w */
#define PHY_BCM542XX_MII_ANA_REG             0x04  /* MII Auto-Neg Advertisement: r/w */
#define PHY_BCM542XX_MII_ANP_REG             0x05  /* MII Auto-Neg Link Partner: ro */
#define PHY_BCM542XX_MII_AN_EXP_REG          0x06  /* MII Auti-Neg Expansion: ro */
#define PHY_BCM542XX_MII_GB_CTRL_REG         0x09  /* MII 1000Base-T control register */
#define PHY_BCM542XX_MII_GB_STAT_REG         0x0a  /* MII 1000Base-T Status register */
#define PHY_BCM542XX_MII_ESR_REG             0x0f  /* MII Extended Status register */

/* Non-standard MII Registers */

#define PHY_BCM542XX_MII_ECR_REG             0x10  /* MII Extended Control Register */
#define PHY_BCM542XX_MII_AUX_REG             0x18  /* MII Auxiliary Control/Status Reg */
#define PHY_BCM542XX_MII_ASSR_REG            0x19  /* MII Auxiliary Status Summary Reg */
#define PHY_BCM542XX_MII_GSR_REG             0x1c  /* MII General status (BROADCOM) */

/* MII Control Register: bit definitions */

#define PHY_BCM542XX_MII_CTRL_UNIDIR_EN      (BIT5)  /* Force speed to 2500 Mbps */
#define PHY_BCM542XX_MII_CTRL_SS_MSB         (BIT6)  /* Speed select, MSb */
#define PHY_BCM542XX_MII_CTRL_FD             (BIT8)  /* Full Duplex */
#define PHY_BCM542XX_MII_CTRL_RST_AN         (BIT9)  /* Restart Autonegotiation */
#define PHY_BCM542XX_MII_CTRL_PWR_DOWN       (BIT11) /* Power Down */
#define PHY_BCM542XX_MII_CTRL_AN_EN          (BIT12) /* Autonegotiation enable */
#define PHY_BCM542XX_MII_CTRL_SS_LSB         (BIT13) /* Speed select, LSb */
#define PHY_BCM542XX_MII_CTRL_LPBK_EN        (BIT14) /* Loopback enable */
#define PHY_BCM542XX_MII_CTRL_RESET          (BIT15) /* PHY reset */

#define PHY_BCM542XX_MII_CTRL_SS(_x)         ((_x) &  \
                       (PHY_BCM542XX_MII_CTRL_SS_LSB|PHY_BCM542XX_MII_CTRL_SS_MSB))
#define PHY_BCM542XX_MII_CTRL_SS_10          0
#define PHY_BCM542XX_MII_CTRL_SS_100         (PHY_BCM542XX_MII_CTRL_SS_LSB)
#define PHY_BCM542XX_MII_CTRL_SS_1000        (PHY_BCM542XX_MII_CTRL_SS_MSB)
#define PHY_BCM542XX_MII_CTRL_SS_INVALID     (PHY_BCM542XX_MII_CTRL_SS_LSB | \
                                              PHY_BCM542XX_MII_CTRL_SS_MSB)
#define PHY_BCM542XX_MII_CTRL_SS_MASK        (PHY_BCM542XX_MII_CTRL_SS_LSB | \
                                              PHY_BCM542XX_MII_CTRL_SS_MSB)
/* 
 * MII Status Register: See 802.3, 1998 pg 544 
 */
#define PHY_BCM542XX_MII_STAT_EXT            (BIT0) /* Extended Registers */
#define PHY_BCM542XX_MII_STAT_JBBR           (BIT1) /* Jabber Detected */
#define PHY_BCM542XX_MII_STAT_LA             (BIT2) /* Link Active */
#define PHY_BCM542XX_MII_STAT_AN_CAP         (BIT3) /* Autoneg capable */
#define PHY_BCM542XX_MII_STAT_RF             (BIT4) /* Remote Fault */
#define PHY_BCM542XX_MII_STAT_AN_DONE        (BIT5) /* Autoneg complete */
#define PHY_BCM542XX_MII_STAT_MF_PS          (BIT6) /* Preamble suppression */
#define PHY_BCM542XX_MII_STAT_ES             (BIT8) /* Extended status (R15) */
#define PHY_BCM542XX_MII_STAT_HD_100_T2      (BIT9) /* Half duplex 100Mb/s supported */
#define PHY_BCM542XX_MII_STAT_FD_100_T2      (BIT10)/* Full duplex 100Mb/s supported */
#define PHY_BCM542XX_MII_STAT_HD_10          (BIT11)/* Half duplex 100Mb/s supported */
#define PHY_BCM542XX_MII_STAT_FD_10          (BIT12)/* Full duplex 100Mb/s supported */
#define PHY_BCM542XX_MII_STAT_HD_100         (BIT13)/* Half duplex 100Mb/s supported */
#define PHY_BCM542XX_MII_STAT_FD_100         (BIT14)/* Full duplex 100Mb/s supported */
#define PHY_BCM542XX_MII_STAT_100_T4         (BIT15)/* Full duplex 100Mb/s supported */

/*
 * MII Link Advertisment
 */
#define PHY_BCM542XX_MII_ANA_ASF             (BIT0)  /* Advertise Selector Field */
#define PHY_BCM542XX_MII_ANA_HD_10           (BIT5)  /* Half duplex 10Mb/s supported */
#define PHY_BCM542XX_MII_ANA_FD_10           (BIT6)  /* Full duplex 10Mb/s supported */
#define PHY_BCM542XX_MII_ANA_HD_100          (BIT7)  /* Half duplex 100Mb/s supported */
#define PHY_BCM542XX_MII_ANA_FD_100          (BIT8)  /* Full duplex 100Mb/s supported */
#define PHY_BCM542XX_MII_ANA_T4              (BIT9)  /* T4 */
#define PHY_BCM542XX_MII_ANA_PAUSE           (BIT10) /* Pause supported */
#define PHY_BCM542XX_MII_ANA_ASYM_PAUSE      (BIT11) /* Asymmetric pause supported */
#define PHY_BCM542XX_MII_ANA_RF              (BIT13) /* Remote fault */
#define PHY_BCM542XX_MII_ANA_NP              (BIT15) /* Next Page */

#define PHY_BCM542XX_MII_ANA_ASF_802_3       (1)     /* 802.3 PHY */

/*
 * 1000X MII Link Advertisment
 */
#define PHY_BCM542XX_1000X_MII_ANA_FD           (BIT5) /* Full duplex supported */
#define PHY_BCM542XX_1000X_MII_ANA_HD           (BIT6) /* Half duplex supported */
#define PHY_BCM542XX_1000X_MII_ANA_PAUSE        (BIT7) /* Symmetric Pause supported */
#define PHY_BCM542XX_1000X_MII_ANA_ASYM_PAUSE   (BIT8) /* Asymmetric pause supported */

/*
 * MII Link Advertisment (Clause 37) 
 */
#define PHY_BCM542XX_MII_ANA_C37_NP          (1 << 15)  /* Next Page */
#define PHY_BCM542XX_MII_ANA_C37_RF_OK       (0 << 12)  /* No error, link OK */
#define PHY_BCM542XX_MII_ANA_C37_RF_LINK_FAIL (1 << 12) /* Offline */
#define PHY_BCM542XX_MII_ANA_C37_RF_OFFLINE  (2 << 12)  /* Link failure */
#define PHY_BCM542XX_MII_ANA_C37_RF_AN_ERR   (3 << 12)  /* Auto-Negotiation Error */
#define PHY_BCM542XX_MII_ANA_C37_PAUSE       (1 << 7)   /* Symmetric Pause */
#define PHY_BCM542XX_MII_ANA_C37_ASYM_PAUSE  (1 << 8)   /* Asymmetric Pause */
#define PHY_BCM542XX_MII_ANA_C37_HD          (1 << 6)   /* Half duplex */
#define PHY_BCM542XX_MII_ANA_C37_FD          (1 << 5)   /* Full duplex */ 

/*
 * MII Link Partner Ability (Clause 37)
 */
#define PHY_BCM542XX_MII_ANP_C37_NP          (1 << 15)  /* Next Page */
#define PHY_BCM542XX_MII_ANP_C37_ACK         (1 << 14)  /* Acknowledge */
#define PHY_BCM542XX_MII_ANP_C37_RF_OK       (0 << 12)  /* No error, link OK */
#define PHY_BCM542XX_MII_ANP_C37_RF_LINK_FAIL (1 << 12) /* Offline */
#define PHY_BCM542XX_MII_ANP_C37_RF_OFFLINE  (2 << 12)  /* Link failure */
#define PHY_BCM542XX_MII_ANP_C37_RF_AN_ERR   (3 << 12)  /* Auto-Negotiation Error */
#define PHY_BCM542XX_MII_ANP_C37_PAUSE       (1 << 7)   /* Symmetric Pause */
#define PHY_BCM542XX_MII_ANP_C37_ASYM_PAUSE  (1 << 8)   /* Asymmetric Pause */
#define PHY_BCM542XX_MII_ANP_C37_HD          (1 << 6)   /* Half duplex */
#define PHY_BCM542XX_MII_ANP_C37_FD          (1 << 5)   /* Full duplex */ 

/*
 * MII Link Partner Ability (SGMII)
 */
#define PHY_BCM542XX_MII_ANP_SGMII_MODE      (1 << 0)   /* Link parneter in SGMII mode */
#define PHY_BCM542XX_MII_ANP_SGMII_FD        (1 << 12)  /* SGMII duplex */
#define PHY_BCM542XX_MII_ANP_SGMII_SS_10     (0 << 10)  /* 10Mbps SGMII */
#define PHY_BCM542XX_MII_ANP_SGMII_SS_100    (1 << 10)  /* 100Mbps SGMII */
#define PHY_BCM542XX_MII_ANP_SGMII_SS_1000   (2 << 10)  /* 1000Mbps SGMII */
#define PHY_BCM542XX_MII_ANP_SGMII_SS_MASK   (3 << 10)  /* SGMII speed mask */
#define PHY_BCM542XX_MII_ANP_SGMII_ACK       (1 << 14)  /* SGMII Ackonwledge */
#define PHY_BCM542XX_MII_ANP_SGMII_LINK      (1 << 15)  /* SGMII Link */

/*
 * 1000Base-T Control Register
 */
#define PHY_BCM542XX_MII_GB_CTRL_MS_MAN      (BIT12) /* Manual Master/Slave mode */
#define PHY_BCM542XX_MII_GB_CTRL_MS          (BIT11) /* Master/Slave negotiation mode */
#define PHY_BCM542XX_MII_GB_CTRL_PT          (BIT10) /* Port type */
#define PHY_BCM542XX_MII_GB_CTRL_ADV_1000FD  (BIT9)  /* Advertise 1000Base-T FD */
#define PHY_BCM542XX_MII_GB_CTRL_ADV_1000HD  (BIT8)  /* Advertise 1000Base-T HD */

/*
 * 1000Base-T Status Register
 */
#define PHY_BCM542XX_MII_GB_STAT_MS_FAULT    (BIT15) /* Master/Slave Fault */
#define PHY_BCM542XX_MII_GB_STAT_MS          (BIT14) /* Master/Slave, 1 == Master */
#define PHY_BCM542XX_MII_GB_STAT_LRS         (BIT13) /* Local receiver status */
#define PHY_BCM542XX_MII_GB_STAT_RRS         (BIT12) /* Remote receiver status */
#define PHY_BCM542XX_MII_GB_STAT_LP_1000FD   (BIT11) /* Link partner 1000FD capable */
#define PHY_BCM542XX_MII_GB_STAT_LP_1000HD   (BIT10) /* Link partner 1000HD capable */
#define PHY_BCM542XX_MII_GB_STAT_IDE         (0xff << 0) /* Idle error count */

/*
 * IEEE Extended Status Register
 */
#define PHY_BCM542XX_MII_ESR_1000_X_FD       (BIT15) /* 1000Base-T FD capable */
#define PHY_BCM542XX_MII_ESR_1000_X_HD       (BIT14) /* 1000Base-T HD capable */
#define PHY_BCM542XX_MII_ESR_1000_T_FD       (BIT13) /* 1000Base-T FD capable */
#define PHY_BCM542XX_MII_ESR_1000_T_HD       (BIT12) /* 1000Base-T FD capable */

/*
 * MII Extended Control Register (BROADCOM)
 */
#define PHY_BCM542XX_MII_ECR_FIFO_ELAST_0    (BIT0) /* FIFO Elasticity[0], MSB
                                                             at exp reg 46[14]*/
#define PHY_BCM542XX_MII_ECR_LED_OFF_F       (BIT3) /* Force LED off */
#define PHY_BCM542XX_MII_ECR_LED_ON_F        (BIT4) /* Force LED on */
#define PHY_BCM542XX_MII_ECR_EN_LEDT         (BIT5) /* Enable LED traffic */
#define PHY_BCM542XX_MII_ECR_RST_SCR         (BIT6) /* Reset Scrambler */
#define PHY_BCM542XX_MII_ECR_BYPASS_ALGN     (BIT7) /* Bypass Receive Sym. align */
#define PHY_BCM542XX_MII_ECR_BPASS_MLT3      (BIT8) /* Bypass MLT3 Encoder/Decoder */
#define PHY_BCM542XX_MII_ECR_BPASS_SCR       (BIT9) /* Bypass Scramble/Descramble */
#define PHY_BCM542XX_MII_ECR_BPASS_ENC       (BIT10) /* Bypass 4B/5B Encode/Decode */
#define PHY_BCM542XX_MII_ECR_FORCE_INT       (BIT11) /* Force Interrupt */
#define PHY_BCM542XX_MII_ECR_INT_DIS         (BIT12) /* Interrupt Disable */
#define PHY_BCM542XX_MII_ECR_TX_DIS          (BIT13) /* XMIT Disable */
#define PHY_BCM542XX_MII_ECR_DAMC            (BIT14) /* Disable Auto-MDI Crossover */

/*
 * Auxiliary Status Summary Register (ASSR - Broadcom BCM542xx)
 */
#define PHY_BCM542XX_MII_ASSR_PRTD           (1 << 0) /* Pause resolution/XMIT direction */
#define PHY_BCM542XX_MII_ASSR_PRRD           (1 << 1) /* Pause resolution/RCV direction */
#define PHY_BCM542XX_MII_ASSR_LS             (1 << 2) /* Link Status (1 == link up) */
#define PHY_BCM542XX_MII_ASSR_LPNPA          (1 << 3) /* Link partner next page cap */
#define PHY_BCM542XX_MII_ASSR_LPANA          (1 << 4) /* Link Partner AN capable */
#define PHY_BCM542XX_MII_ASSR_ANPR           (1 << 5) /* Autoneg page received */
#define PHY_BCM542XX_MII_ASSR_RF             (1 << 6) /* Remote Fault */
#define PHY_BCM542XX_MII_ASSR_PDF            (1 << 7) /* Parallel detection fault */
#define PHY_BCM542XX_MII_ASSR_HCD            (7 << 8) /* Current operating speed */
#define PHY_BCM542XX_MII_ASSR_ANNPW          (1 << 11) /* Auto-neg next page wait */
#define PHY_BCM542XX_MII_ASSR_ANABD          (1 << 12) /* Auto-neg Ability detected */
#define PHY_BCM542XX_MII_ASSR_ANAD           (1 << 13) /* Auto-neg ACK detect */
#define PHY_BCM542XX_MII_ASSR_ANCA           (1 << 14) /* Auto-neg complete ACK */
#define PHY_BCM542XX_MII_ASSR_ANC            (1 << 15) /* AUto-neg complete */
#define PHY_BCM542XX_MII_ASSR_HCD_FD_1000    (7 << 8)
#define PHY_BCM542XX_MII_ASSR_HCD_HD_1000    (6 << 8)
#define PHY_BCM542XX_MII_ASSR_HCD_FD_100     (5 << 8)
#define PHY_BCM542XX_MII_ASSR_HCD_T4_100     (4 << 8)
#define PHY_BCM542XX_MII_ASSR_HCD_HD_100     (3 << 8)
#define PHY_BCM542XX_MII_ASSR_HCD_FD_10      (2 << 8)
#define PHY_BCM542XX_MII_ASSR_HCD_HD_10      (1 << 8)
#define PHY_BCM542XX_MII_ASSR_HCD_NC         (0 << 8) /* Not complete */
#define PHY_BCM542XX_MII_ASSR_AUTONEG_HCD_MASK  (0x0700)

/*
 * Auto Detect Medium Register 
 */
#define PHY_BCM542XX_MII_AUTO_DET_MED_2ND_SERDES  (BIT9)
#define PHY_BCM542XX_MII_INV_FIBER_SD             (BIT8)
#define PHY_BCM542XX_MII_FIBER_IN_USE_LED         (BIT7)
#define PHY_BCM542XX_MII_FIBER_LED                (BIT6)
#define PHY_BCM542XX_MII_FIBER_SD_SYNC            (BIT5)
#define PHY_BCM542XX_MII_FIBER_AUTO_PWRDN         (BIT4)
#define PHY_BCM542XX_MII_SD_en_ov                 (BIT3)
#define PHY_BCM542XX_MII_AUTO_DET_MED_DEFAULT     (BIT2)
#define PHY_BCM542XX_MII_AUTO_DET_MED_PRI         (BIT1)
#define PHY_BCM542XX_MII_AUTO_DET_MED_EN          (BIT0)
#define PHY_BCM542XX_MII_AUTO_DET_MASK            (0x033F)

/*
 * Mode Control Register 
 */
#define PHY_BCM542XX_MODE_CNTL_CU_LINK          (BIT7) /* Copper link */
#define PHY_BCM542XX_MODE_CNTL_SERDES_LINK      (BIT6) /* Fiber, SGMII link */
#define PHY_BCM542XX_MODE_CNTL_CU_ENG_DET       (BIT5) /* Copper energy detected */
#define PHY_BCM542XX_MODE_CNTL_FBER_SIG_DET     (BIT4) /* Fiber signal detected */
#define PHY_BCM542XX_MODE_CNTL_SERDES_CAPABLE   (BIT3) /* SerDes capable device */
#define PHY_BCM542XX_MODE_CNTL_MODE_SEL_2       (BIT2) /* Mode select [2] */
#define PHY_BCM542XX_MODE_CNTL_MODE_SEL_1       (BIT1) /* Mode select [1] */
#define PHY_BCM542XX_MODE_CNTL_1000X_EN         (BIT0) /* Select 1000-X (fiber) regs
                                                          vs. Copper regs   */
#define PHY_BCM542XX_MODE_CNTL_MODE_SEL_MASK    (PHY_BCM542XX_MODE_CNTL_MODE_SEL_1 | \
                                                 PHY_BCM542XX_MODE_CNTL_MODE_SEL_2)

#define PHY_BCM542XX_MODE_SEL_COPPER_2_SGMII    (0x0)
#define PHY_BCM542XX_MODE_SEL_FIBER_2_SGMII     (PHY_BCM542XX_MODE_CNTL_MODE_SEL_1)
#define PHY_BCM542XX_MODE_SEL_SGMII_2_COPPER    (PHY_BCM542XX_MODE_CNTL_MODE_SEL_2)
#define PHY_BCM542XX_MODE_SEL_GBIC              (PHY_BCM542XX_MODE_CNTL_MODE_SEL_1 | \
                                                 PHY_BCM542XX_MODE_CNTL_MODE_SEL_2)

/* 1000 BASE X CONTROL REG */
#define PHY_BCM542XX_1000BASE_X_CTRL_REG_LE     (BIT14)
#define PHY_BCM542XX_1000BASE_X_CTRL_REG_AE     (BIT12)

/* 1000 BASE X STATUS REG */
#define PHY_BCM542XX_1000BASE_X_STAT_REG_LA     (BIT2)

/* 
 * MII Broadcom MISC Control Register(PHY_BCM542XX_MII_MISC_CTRL_REG)
 */
#define PHY_BCM542XX_MII_FORCE_AUTO_MDIX_MODE   (BIT9)
#define PHY_BCM542XX_MII_BP_WIRESPEED_TIMER     (BIT10)
#define PHY_BCM542XX_MII_WIRESPEED_EN           (BIT4)
#define PHY_BCM542XX_MII_MISC_CTRL_ALL          (0x0FF8)
#define PHY_BCM542XX_MII_MISC_CTRL_CLEARALL     (0x2007)

/* 
 * MII Broadcom Test Register(PHY_BCM542XX_MII_SHA_AUX_STAT2_REG)
 */                                       /* Enable Shadow registers */
#define PHY_BCM542XX_MII_SHA_AUX_STAT2_LEN      (0x7 << 12)

/* Miscellaneous registers */
#define PHY_BCM542XX_SPARE_CTRL_REG_LINK_LED         (BIT0)
#define PHY_BCM542XX_SPARE_CTRL_REG_LINK_SPEED_LED   (BIT2)


/* Flag indicating that its a SerDes register */

#define PHY_BCM542XX_REG_1000X       (BIT0)
#define PHY_BCM542XX_REG_PRI_SERDES  (BIT1)
#define PHY_BCM542XX_REG_QSGMII      (BIT4)
#define PHY_BCM542XX_REG_FIBER       (PHY_BCM542XX_REG_1000X | \
                                      PHY_BCM542XX_REG_PRI_SERDES)


/***********************************************
 *
 * Generic register accessing macros/functions
 *
 ***********************************************/

int phy_bcm542xx_reg_read(int unit, phy_ctrl_t *pc, uint32 flags,
        uint16 reg_bank, uint8 reg_addr, uint16 *data);
int phy_bcm542xx_reg_write(int unit, phy_ctrl_t *pc, uint32 flags, uint16 reg_bank,
        uint8 reg_addr, uint16 data);
int phy_bcm542xx_reg_modify(int unit, phy_ctrl_t *pc, uint32 flags, uint16 reg_bank,
        uint8 reg_addr, uint16 data, uint16 mask);
int phy_bcm542xx_reg_read_modify_write(int unit, phy_ctrl_t *pc, uint32 reg_addr,
        uint16 reg_data, uint16 reg_mask);

int phy_bcm542xx_direct_reg_read(int unit, phy_ctrl_t *pc, uint16 reg_addr,
        uint16 *data);
int phy_bcm542xx_direct_reg_write(int unit, phy_ctrl_t *pc, uint16 reg_addr,
        uint16 data);
int phy_bcm542xx_direct_reg_modify(int unit, phy_ctrl_t *pc, uint16 reg_addr,
        uint16 data, uint16 mask);

int phy_bcm542xx_qsgmii_reg_read(int unit, phy_ctrl_t *pc, int dev_port,
        uint16 block, uint8 reg, uint16 *data);
int phy_bcm542xx_qsgmii_reg_write(int unit, phy_ctrl_t *pc, int dev_port,
        uint16 block, uint8 reg, uint16 data);
int phy_bcm542xx_qsgmii_reg_modify(int unit, phy_ctrl_t *pc, int dev_port,
        uint16 block, uint8 reg_addr, uint16 reg_data, uint16 reg_mask);

int phy_bcm542xx_cl45_reg_read(int unit, phy_ctrl_t *pc, uint8 dev_addr,
        uint16 reg_addr, uint16 *val);
int phy_bcm542xx_cl45_reg_write(int unit, phy_ctrl_t *pc, uint8 dev_addr,
        uint16 reg_addr, uint16 val);
int phy_bcm542xx_cl45_reg_modify(int unit, phy_ctrl_t *pc, uint8 dev_addr,
        uint16 reg_addr, uint16 val, uint16 mask);

/* PHY register access */
#define PHY_BCM542XX_READ_PHY_REG(_unit, _phy_ctrl,  _reg_addr, _val) \
            READ_PHY_REG((_unit), (_phy_ctrl),  (_reg_addr), (_val))
#define PHY_BCM542XX_WRITE_PHY_REG(_unit, _phy_ctrl, _reg_addr, _val) \
            WRITE_PHY_REG((_unit), (_phy_ctrl), (_reg_addr), (_val))
#define PHY_BCM542XX_MODIFY_PHY_REG(_unit, _phy_ctrl, _reg_addr, _val, _mask) \
            MODIFY_PHY_REG((_unit), (_phy_ctrl), (_reg_addr), (_val), (_mask))

/* General - PHY register access */
#define PHY_BCM542XX_REG_RD(_unit, _pc, _flags, _reg_bank, _reg_addr, _val) \
            phy_bcm542xx_reg_read((_unit), (_pc), (_flags), (_reg_bank), \
                                    (_reg_addr), (_val))
#define PHY_BCM542XX_REG_WR(_unit, _pc, _flags, _reg_bank, _reg_addr, _val) \
            phy_bcm542xx_reg_write((_unit), (_pc), (_flags), (_reg_bank), \
                                    (_reg_addr), (_val))
#define PHY_BCM542XX_REG_MOD(_unit, _pc, _flags, _reg_bank, _reg_addr, _val, _mask) \
            phy_bcm542xx_reg_modify((_unit), (_pc), (_flags), (_reg_bank), \
                                    (_reg_addr), (_val), (_mask))

/* Clause 45 Registers access using Clause 22 register access */
#define PHY_BCM542XX_CL45_REG_READ(_unit, _pc, _dev_addr, _reg_addr, _val) \
            phy_bcm542xx_cl45_reg_read((_unit), (_pc), (_dev_addr), \
                                         (_reg_addr), (_val))
#define PHY_BCM542XX_CL45_REG_WRITE(_unit, _pc, _dev_addr, _reg_addr, _val) \
            phy_bcm542xx_cl45_reg_write((_unit), (_pc), (_dev_addr), \
                                         (_reg_addr), (_val))
#define PHY_BCM542XX_CL45_REG_MODIFY(_unit, _pc,  _dev_addr, _reg_addr, _val, _mask) \
            phy_bcm542xx_cl45_reg_modify((_unit), (_pc), (_dev_addr), \
                                         (_reg_addr), (_val), (_mask))


/* Registers used to enable RDB register access mode */
#define PHY_BCM542XX_REG_17_OFFSET              (0x17)
#define PHY_BCM542XX_REG_17_SELECT_EXP_7E       (0x0F7E)
#define PHY_BCM542XX_REG_15_OFFSET              (0x15)
#define PHY_BCM542XX_REG_15_RDB_EN              (0x0000)
#define PHY_BCM542XX_REG_15_RDB_DIS             (0x8000)
#define PHY_BCM542XX_REG_1E_SELECT_RDB          (0x0087)

/* Registers used to Rd/Wr using RDB mode */
#define PHY_BCM542XX_RDB_ADDR_REG_OFFSET        (0x1E)
#define PHY_BCM542XX_RDB_ADDR_REG_ADDR          (0xffff)
#define PHY_BCM542XX_RDB_DATA_REG_OFFSET        (0x1F)
#define PHY_BCM542XX_RDB_DATA_REG_DATA          (0xffff)

/*
 *  Disable direct RDB addressing mode, write to RDB register 0x87 = 0x8000
 *  - MDIO write to reg 0x1E = 0x0087
 *  - MDIO write to reg 0x1F = 0x8000
 */
#define PHY_BCM542XX_LEGACY_ACCESS_MODE(_u, _pc)  do {  \
    SOC_IF_ERROR_RETURN(  \
        PHY_BCM542XX_WRITE_PHY_REG((_u), (_pc), PHY_BCM542XX_RDB_ADDR_REG_OFFSET, \
                                                PHY_BCM542XX_REG_1E_SELECT_RDB) ); \
    SOC_IF_ERROR_RETURN(  \
        PHY_BCM542XX_WRITE_PHY_REG((_u), (_pc), PHY_BCM542XX_RDB_DATA_REG_OFFSET, \
                                                PHY_BCM542XX_REG_15_RDB_DIS) );   \
} while ( 0 )

/*
 *  Enable direct RDB addressing mode, write to Expansion register 0x7E = 0x0000
 *  - MDIO write to reg 0x17 = 0x0F7E
 *  - MDIO write to reg 0x15 = 0x0000
 */
#define PHY_BCM542XX_RDB_ACCESS_MODE(_u, _pc)  do {  \
    SOC_IF_ERROR_RETURN(  \
        PHY_BCM542XX_WRITE_PHY_REG((_u), (_pc), PHY_BCM542XX_REG_17_OFFSET, \
                                                PHY_BCM542XX_REG_17_SELECT_EXP_7E) ); \
    SOC_IF_ERROR_RETURN(  \
        PHY_BCM542XX_WRITE_PHY_REG((_u), (_pc), PHY_BCM542XX_REG_15_OFFSET,    \
                                                PHY_BCM542XX_REG_15_RDB_EN) ); \
} while ( 0 )

/*  *** works ONLY under RDB register addressing mode ***
 *  RDB register read, read from RDB register <_a>
 *  - MDIO write to reg 0x1E = <_a>
 *  - MDIO read <_v> from reg 0x1F
 */
#define PHY_BCM542XX_READ_RDB_REG(_u, _pc, _a, _v)  do {  \
    SOC_IF_ERROR_RETURN(  \
        PHY_BCM542XX_WRITE_PHY_REG((_u), (_pc), PHY_BCM542XX_RDB_ADDR_REG_OFFSET,  \
                                         (_a) & PHY_BCM542XX_RDB_ADDR_REG_ADDR) ); \
    SOC_IF_ERROR_RETURN(  \
        PHY_BCM542XX_READ_PHY_REG( (_u), (_pc), PHY_BCM542XX_RDB_DATA_REG_OFFSET,  \
                                         (_v)) ); \
} while ( 0 )

/*  *** works ONLY under RDB register addressing mode ***
 *  RDB register write, write to RDB register <_a> = <_v>
 *  - MDIO write to reg 0x1E = <_a>
 *  - MDIO write to reg 0x1F = <_v>
 */
#define PHY_BCM542XX_WRITE_RDB_REG(_u, _pc, _a, _v)  do {  \
    SOC_IF_ERROR_RETURN(  \
        PHY_BCM542XX_WRITE_PHY_REG((_u), (_pc), PHY_BCM542XX_RDB_ADDR_REG_OFFSET,  \
                                         (_a) & PHY_BCM542XX_RDB_ADDR_REG_ADDR) ); \
    SOC_IF_ERROR_RETURN(  \
        PHY_BCM542XX_WRITE_PHY_REG((_u), (_pc), PHY_BCM542XX_RDB_DATA_REG_OFFSET,  \
                                         (_v) & PHY_BCM542XX_RDB_DATA_REG_DATA) ); \
} while ( 0 )

/*  *** works ONLY under RDB register addressing mode ***
 *  RDB register write, write to RDB register <_a> = (<_v> & <_m>)
 *  - MDIO write to reg 0x1E = <_a>
 *  - MDIO write to reg 0x1F = (<_v> & <_m>)
 */
#define PHY_BCM542XX_MODIFY_RDB_REG(_u, _pc, _a, _v, _m)  do {  \
    SOC_IF_ERROR_RETURN(  \
        PHY_BCM542XX_WRITE_PHY_REG((_u), (_pc), PHY_BCM542XX_RDB_ADDR_REG_OFFSET,  \
                                         (_a) & PHY_BCM542XX_RDB_ADDR_REG_ADDR) ); \
    SOC_IF_ERROR_RETURN(  \
        PHY_BCM542XX_MODIFY_PHY_REG((_u),(_pc), PHY_BCM542XX_RDB_DATA_REG_OFFSET, \
                                         (_v) & PHY_BCM542XX_RDB_DATA_REG_DATA,   \
                                         (_m)) ); \
} while ( 0 )



/***********************************************
 *
 * Specific register accessing macros
 *
 ***********************************************/

/* 1000BASE-T/100BASE-TX/10BASE-T MII Control Register (Addr 00h) */
#define PHY_READ_BCM542XX_MII_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, \
                                 PHY_BCM542XX_MII_CTRL_REG, (_val))
#define PHY_WRITE_BCM542XX_MII_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, \
                                 PHY_BCM542XX_MII_CTRL_REG, (_val))
#define PHY_MODIFY_BCM542XX_MII_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, \
                                 PHY_BCM542XX_MII_CTRL_REG, (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T MII Status Register (ADDR 01h) */
#define PHY_READ_BCM542XX_MII_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x01, (_val)) 
#define PHY_WRITE_BCM542XX_MII_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x01, (_val)) 
#define PHY_MODIFY_BCM542XX_MII_STATr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x01, \
                                 (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T PHY Identifier Register (ADDR 02h) */
#define PHY_READ_BCM542XX_MII_PHY_ID0r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x02, (_val))
#define PHY_WRITE_BCM542XX_MII_PHY_ID0r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x02, (_val))
#define PHY_MODIFY_BCM542XX_MII_PHY_ID0r(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x02, \
                                 (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T PHY Identifier Register (ADDR 03h) */
#define PHY_READ_BCM542XX_MII_PHY_ID1r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x03, (_val))
#define PHY_WRITE_BCM542XX_MII_PHY_ID1r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x03, (_val))
#define PHY_MODIFY_BCM542XX_MII_PHY_ID1r(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x03, \
                                 (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auto-neg Advertisment Register (ADDR 04h) */
#define PHY_READ_BCM542XX_MII_ANAr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x04, (_val))
#define PHY_WRITE_BCM542XX_MII_ANAr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x04, (_val))
#define PHY_MODIFY_BCM542XX_MII_ANAr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x04, \
                                 (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auto-neg Link Partner Ability (ADDR 05h) */
#define PHY_READ_BCM542XX_MII_ANPr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x05, (_val))
#define PHY_WRITE_BCM542XX_MII_ANPr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x05, (_val)) 
#define PHY_MODIFY_BCM542XX_MII_ANPr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x05, \
                                 (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auto-neg Expansion Register (ADDR 06h) */
#define PHY_READ_BCM542XX_MII_AN_EXPr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x06, (_val))
#define PHY_WRITE_BCM542XX_MII_AN_EXPr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x06, (_val))
#define PHY_MODIFY_BCM542XX_MII_AN_EXPr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x06, \
                                 (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Next Page Transmit Register (ADDR 07h) */

/* 1000BASE-T/100BASE-TX/10BASE-T Link Partner Received Next Page (ADDR 08h) */

/* 1000BASE-T Control Register  (ADDR 09h) */
#define PHY_READ_BCM542XX_MII_GB_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x09, (_val))
#define PHY_WRITE_BCM542XX_MII_GB_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x09, (_val))
#define PHY_MODIFY_BCM542XX_MII_GB_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x09, \
                                 (_val), (_mask))

/* 1000BASE-T Status Register (ADDR 0ah) */
#define PHY_READ_BCM542XX_MII_GB_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x0a, (_val)) 
#define PHY_WRITE_BCM542XX_MII_GB_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x0a, (_val))
#define PHY_MODIFY_BCM542XX_MII_GB_STATr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x0a, \
                                 (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T IEEE Extended Status Register (ADDR 0fh) */
#define PHY_READ_BCM542XX_MII_ESRr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x0f, (_val)) 
#define PHY_WRITE_BCM542XX_MII_ESRr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x0f, (_val))
#define PHY_MODIFY_BCM542XX_MII_ESRr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x0f, \
                                 (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T PHY Extended Control Register (ADDR 10h) */
#define PHY_READ_BCM542XX_MII_ECRr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, \
                                 PHY_BCM542XX_MII_ECR_REG, (_val))
#define PHY_WRITE_BCM542XX_MII_ECRr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, \
                                 PHY_BCM542XX_MII_ECR_REG, (_val))
#define PHY_MODIFY_BCM542XX_MII_ECRr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, \
                                 PHY_BCM542XX_MII_ECR_REG, (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Receive Error Counter Register (ADDR 12h) */
/* 1000BASE-T/100BASE-TX/10BASE-T False Carrier Sense Counter (ADDR 13h) */
/* 1000BASE-T/100BASE-TX/10BASE-T Receive NOT_OK Counter Register (ADDR 14h) */

#define AUX_CTRL_REG_EXT_PKT_LEN   0x4000 /* BIT14 */
#define AUX_CTRL_REG_EN_DSP_CLK    0x0800 /* BIT11 */

/* 1000BASE-T/100BASE-TX/10BASE-T Auxiliary Control Reg (ADDR 18h Shadow 000)*/
#define PHY_READ_BCM542XX_MII_AUX_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x18, (_val))
#define PHY_WRITE_BCM542XX_MII_AUX_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x18, (_val))
#define PHY_MODIFY_BCM542XX_MII_AUX_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x18, \
                             (_val), (_mask))

/* 10BASE-T Register (ADDR 18h Shadow 001) */
#define PHY_READ_BCM542XX_MII_10BASE_Tr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0001, 0x18, (_val))
#define PHY_WRITE_BCM542XX_MII_10BASE_Tr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0001, 0x18, (_val))
#define PHY_MODIFY_BCM542XX_MII_10BASE_Tr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0001, 0x18, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Power/MII Control Reg (ADDR 18h Shadow 010)*/
#define PHY_READ_BCM542XX_MII_POWER_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0002, 0x18, (_val))
#define PHY_WRITE_BCM542XX_MII_POWER_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0002, 0x18, (_val))
#define PHY_MODIFY_BCM542XX_MII_POWER_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0002, 0x18, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Misc Test Register (ADDR 18h Shadow 100) */
#define PHY_READ_BCM542XX_MII_MISC_TESTr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0004, 0x18, (_val))
#define PHY_WRITE_BCM542XX_MII_MISC_TESTr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0004, 0x18, (_val))
#define PHY_MODIFY_BCM542XX_MII_MISC_TESTr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0004, 0x18, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Misc Control Register (ADDR 18h Shadow 111)*/
#define PHY_READ_BCM542XX_MII_MISC_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0007, 0x18, (_val))
#define PHY_WRITE_BCM542XX_MII_MISC_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0007, 0x18, (_val))
#define PHY_MODIFY_BCM542XX_MII_MISC_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0007, 0x18, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Auxiliary Status Register (ADDR 19h) */
#define PHY_READ_BCM542XX_MII_AUX_STATUSr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x19, (_val))
#define PHY_WRITE_BCM542XX_MII_AUX_STATUSr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x19, (_val))
#define PHY_MODIFY_BCM542XX_MII_AUX_STATUSr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x19, \
                             (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Interrupt Status Register (ADDR 1ah) */
/* 1000BASE-T/100BASE-TX/10BASE-T Interrupt Control Register (ADDR 1bh) */

/* 1000BASE-T/100BASE-TX/10BASE-T Spare Ctrl Reg (ADDR 1ch shadow 00010) */
#define PHY_READ_BCM542XX_SPARE_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0002, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_SPARE_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0002, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_SPARE_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0002, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Clk Alignment Ctrl (ADDR 1ch shadow 00011) */
#define PHY_READ_BCM542XX_CLK_ALIGN_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0003, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_CLK_ALIGN_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0003, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_CLK_ALIGN_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0003, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Spare Ctrl 2 Reg (ADDR 1ch shadow 00100) */
#define PHY_READ_BCM542XX_SPARE_CTRL_2r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0004, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_SPARE_CTRL_2r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0004, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_SPARE_CTRL_2r(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0004, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Spare Ctrl 3 Reg (ADDR 1ch shadow 00101) */
#define PHY_READ_BCM542XX_SPARE_CTRL_3r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0005, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_SPARE_CTRL_3r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0005, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_SPARE_CTRL_3r(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0005, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED Status Reg (ADDR 1ch shadow 01000) */
#define PHY_READ_BCM542XX_LED_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0008, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_LED_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0008, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_LED_STATr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0008, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED Ctrl Reg (ADDR 1ch shadow 01001) */
#define PHY_READ_BCM542XX_LED_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0009, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_LED_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0009, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_LED_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0009, 0x1c, \
                           (_val), (_mask))

/* Auto Power-Down Reg (ADDR 1ch shadow 01010) */
#define PHY_READ_BCM542XX_AUTO_POWER_DOWNr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x000a, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_AUTO_POWER_DOWNr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x000a, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_AUTO_POWER_DOWNr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x000a, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED Selector 1 Reg (ADDR 1ch shadow 01101) */
#define PHY_READ_BCM542XX_LED_SELECTOR_1r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x000d, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_LED_SELECTOR_1r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x000d, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_LED_SELECTOR_1r(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x000d, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED Selector 2 Reg (ADDR 1ch shadow 01110) */
#define PHY_READ_BCM542XX_LED_SELECTOR_2r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x000e, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_LED_SELECTOR_2r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x000e, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_LED_SELECTOR_2r(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x000e, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T LED GPIO Ctrl/Stat (ADDR 1ch shadow 01111) */
#define PHY_READ_BCM542XX_LED_GPIO_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x000f, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_LED_GPIO_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x000f, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_LED_GPIO_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x000f, 0x1c, \
                           (_val), (_mask))

/* SerDes 100BASE-FX Status Reg (ADDR 1ch shadow 10001) */
#define PHY_READ_BCM542XX_100FX_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0011, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_100FX_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0011, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_100FX_STATr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0011, 0x1c, \
                           (_val), (_mask))

/* SerDes 100BASE-FX Control Reg (ADDR 1ch shadow 10011) */
#define PHY_READ_BCM542XX_100FX_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0013, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_100FX_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0013, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_100FX_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0013, 0x1c, \
                           (_val), (_mask))

/* External SerDes Control Reg (ADDR 1ch shadow 10100) */
#define PHY_READ_BCM542XX_EXT_SERDES_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0014, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_EXT_SERDES_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0014, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_EXT_SERDES_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0014, 0x1c, \
                           (_val), (_mask))

/* SGMII Slave Reg (ADDR 1ch shadow 10101) */
#define PHY_READ_BCM542XX_SGMII_SLAVEr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0015, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_SGMII_SLAVEr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0015, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_SGMII_SLAVEr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0015, 0x1c, \
                           (_val), (_mask))

/* Primary SerDes Control Reg (ADDR 1ch shadow 10110) */
#define PHY_READ_BCM542XX_1ST_SERDES_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0016, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_1ST_SERDES_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0016, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_1ST_SERDES_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0016, 0x1c, \
                           (_val), (_mask))

/* Misc 1000BASE-X Control (ADDR 1ch shadow 10111) */
#define PHY_READ_BCM542XX_MISC_1000X_CONTROLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0017, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_MISC_1000X_CONTROLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0017, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_MISC_1000X_CONTROLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0017, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-X Auto-Detect SGMII/Media Converter Reg (ADDR 1ch shadow 11000) */
#define PHY_READ_BCM542XX_AUTO_DETECT_SGMII_MEDIAr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0018, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_AUTO_DETECT_SGMII_MEDIAr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0018, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_AUTO_DETECT_SGMII_MEDIAr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0018, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-X Auto-neg Debug Reg (ADDR 1ch shadow 11010) */
#define PHY_READ_BCM542XX_1000X_AN_DEBUGr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x001a, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_1000X_AN_DEBUGr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x001a, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_1000X_AN_DEBUGr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x001a, 0x1c, \
                           (_val), (_mask))

/* Auxiliary 1000BASE-X Control Reg (ADDR 1ch shadow 11011) */
#define PHY_READ_BCM542XX_AUX_1000X_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x001b, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_AUX_1000X_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x001b, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_AUX_1000X_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x001b, 0x1c, \
                           (_val), (_mask))

/* Auxiliary 1000BASE-X Status Reg (ADDR 1ch shadow 11100) */
#define PHY_READ_BCM542XX_AUX_1000X_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x001c, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_AUX_1000X_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x001c, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_AUX_1000X_STATr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x001c, 0x1c, \
                           (_val), (_mask))

/* Misc 1000BASE-X Status Reg (ADDR 1ch shadow 11101) */
#define PHY_READ_BCM542XX_MISC_1000X_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x001d, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_MISC_1000X_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x001d, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_MISC_1000X_STATr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x001d, 0x1c, \
                           (_val), (_mask))

/* Copper/Fiber Auto-Detect Medium Reg (ADDR 1ch shadow 11110) */
#define PHY_READ_BCM542XX_AUTO_DETECT_MEDIUMr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x001e, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_AUTO_DETECT_MEDIUMr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x001e, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_AUTO_DETECT_MEDIUMr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x001e, 0x1c, \
                           (_val), (_mask))

/* Mode Control Reg (ADDR 1ch shadow 11111) */
#define PHY_READ_BCM542XX_MODE_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x001f, 0x1c, (_val))
#define PHY_WRITE_BCM542XX_MODE_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x001f, 0x1c, (_val))
#define PHY_MODIFY_BCM542XX_MODE_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x001f, 0x1c, \
                           (_val), (_mask))

/* 1000BASE-T/100BASE-TX/10BASE-T Test Register 1 (ADDR 1eh) */
#define PHY_READ_BCM542XX_TEST1r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0000, 0x1e, (_val))
#define PHY_WRITE_BCM542XX_TEST1r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0000, 0x1e, (_val))
#define PHY_MODIFY_BCM542XX_TEST1r(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0000, 0x1e, \
                           (_val), (_mask))

/*          +------------------------------+
 *          |                              |
 *          |   Primary SerDes Registers   |
 *          |                              |
 *          +------------------------------+
 */
/* 1000BASE-X MII Control Register (Addr 00h) */
#define PHY_READ_BCM542XX_1000X_MII_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x00, (_val))
#define PHY_WRITE_BCM542XX_1000X_MII_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x00, (_val))
#define PHY_MODIFY_BCM542XX_1000X_MII_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x00, (_val), (_mask))
#define PHY_MODIFY_BCM542XX_PRI_SERDES_MII_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), PHY_BCM542XX_REG_PRI_SERDES, \
                             0x0000, 0x00, (_val), (_mask))

/* 1000BASE-X MII Status Register (Addr 01h) */
#define PHY_READ_BCM542XX_1000X_MII_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x01, (_val))
#define PHY_WRITE_BCM542XX_1000X_MII_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x01, (_val))
#define PHY_MODIFY_BCM542XX_1000X_MII_STATr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x01, (_val), (_mask))

/* 1000BASE-X MII Auto-neg Advertise Register (Addr 04h) */
#define PHY_READ_BCM542XX_1000X_MII_ANAr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x04, (_val))
#define PHY_WRITE_BCM542XX_1000X_MII_ANAr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x04, (_val))
#define PHY_MODIFY_BCM542XX_1000X_MII_ANAr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x04, (_val), (_mask))

/* 1000BASE-X MII Auto-neg Link Partner Ability Register (Addr 05h) */
#define PHY_READ_BCM542XX_1000X_MII_ANPr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x05, (_val))
#define PHY_WRITE_BCM542XX_1000X_MII_ANPr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x05, (_val))
#define PHY_MODIFY_BCM542XX_1000X_MII_ANPr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x05, (_val), (_mask))

/* 1000BASE-X MII Auto-neg Extended Status Register (Addr 06h) */
#define PHY_READ_BCM542XX_1000X_MII_AN_EXPr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x06, (_val))
#define PHY_WRITE_BCM542XX_1000X_MII_AN_EXPr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x06, (_val))
#define PHY_MODIFY_BCM542XX_1000X_MII_AN_EXPr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x06, (_val), (_mask))

/* 1000BASE-X MII IEEE Extended Status Register (Addr 0fh) */
#define PHY_READ_BCM542XX_1000X_MII_EXT_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x0f, (_val))
#define PHY_WRITE_BCM542XX_1000X_MII_EXT_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x0f, (_val))
#define PHY_MODIFY_BCM542XX_1000X_MII_EXT_STATr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), PHY_BCM542XX_REG_1000X, \
                             0x0000, 0x0f, (_val), (_mask))

/*          +-------------------------+
 *          |                         |
 *          |   Expansion Registers   |
 *          |                         |
 *          +-------------------------+
 */
/* Receive/Transmit Packet Counter Register (Addr 00h) */
#define PHY_READ_BCM542XX_EXP_PKT_COUNTERr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f00, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_PKT_COUNTERr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f00, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_PKT_COUNTERr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f00, 0x15, \
                                (_val), (_mask))

/* Expansion Interrupt Status Register (Addr 01h) */
#define PHY_READ_BCM542XX_EXP_INTERRUPT_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f01, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_r(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f01, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_r(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f01, 0x15, \
                                (_val), (_mask))

/* Expansion Interrupt Mask Register (Addr 02h) */
#define PHY_READ_BCM542XX_EXP_INTERRUPT_MASKr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f02, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_INTERRUPT_MASKr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f02, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_INTERRUPT_MASKr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f02, 0x15, \
                                (_val), (_mask))

/* Multicolor LED Selector Register (Addr 04h) */
#define PHY_READ_BCM542XX_EXP_LED_SELECTORr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f04, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_LED_SELECTORr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f04, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_LED_SELECTORr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f04, 0x15, \
                                (_val), (_mask))

/* Multicolor LED Flash Rate Controls Register (Addr 05h) */
#define PHY_READ_BCM542XX_EXP_LED_FLASH_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f05, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_LED_FLASH_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f05, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_LED_FLASH_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f05, 0x15, \
                                (_val), (_mask))

/* Multicolor LED Programmable Blink Controls Register (Addr 06h) */
#define PHY_READ_BCM542XX_EXP_LED_BLINK_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f06, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_LED_BLINK_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f06, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_LED_BLINK_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f06, 0x15, \
                                (_val), (_mask))

/* Expansion register 0x8 (For auto_early_dac_wake)*/
#define PHY_READ_BCM542XX_EXP_EIGHTr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f08, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_EIGHTr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f08, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_EIGHTr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f08, 0x15, \
                                (_val), (_mask))

/* Operating Mode Status Register (Addr 42h) */
#define PHY_READ_BCM542XX_EXP_OPT_MODE_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f42, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_OPT_MODE_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f42, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_OPT_MODE_STATr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f42, 0x15, \
                                (_val), (_mask))

/* Pattern Generator Status Register (Addr 46h) */
#define PHY_READ_BCM542XX_EXP_PATT_GEN_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f46, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_PATT_GEN_STATr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f46, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_PATT_GEN_STATr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f46, 0x15, \
                                (_val), (_mask))
#define PHY_BCM542XX_PATT_GEN_STAT_FIFO_ELAST_1  (0x4000) /*BIT 14*/

/* SerDes/SGMII RX Control Register (Addr 50h) */
#define PHY_READ_BCM542XX_EXP_SERDES_SGMII_RXCTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f50, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_SERDES_SGMII_RXCTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f50, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_SERDES_SGMII_RXCTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f50, 0x15, \
                               (_val), (_mask))

/* SerDes/SGMII RX/TX Control Register (Addr 52h) */
#define PHY_READ_BCM542XX_EXP_SERDES_SGMII_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f52, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_SERDES_SGMII_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f52, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_SERDES_SGMII_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f52, 0x15, \
                               (_val), (_mask))

/* SerDes/SGMII RX/TX Control Register (Addr 7fh) */
#define PHY_READ_BCM542XX_EXP_EXT_MACSEC_IF_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0f7f, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EXP_EXT_MACSEC_IF_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0f7f, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EXP_EXT_MACSEC_IF_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0f7f, 0x15, \
                               (_val), (_mask))

/* Time Sync */
#define PHY_READ_BCM542XX_TIME_SYNC_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0ff5, 0x15, (_val))
#define PHY_WRITE_BCM542XX_TIME_SYNC_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0ff5, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_TIME_SYNC_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0ff5, 0x15, (_val), (_mask))

/* EEE Statistics Registers*/

/* Statistic Timer_12_hours_lpi remote Reg (Addr aah) */
#define PHY_READ_BCM542XX_EEE_STAT_TX_DURATIONr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0faa, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EEE_STAT_TX_DURATIONr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0faa, 0x15, (_val))
#define PHY_MODIFY_BCM542XX__EEE_STAT_TX_DURATIONr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0faa, 0x15, (_val), (_mask))

/* Statistic Timer_12_hours_lpi local Reg (Addr abh) */
#define PHY_READ_BCM542XX_EEE_STAT_RX_DURATIONr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0fab, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EEE_STAT_RX_DURATIONr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0fab, 0x15, (_val))
#define PHY_MODIFY_BCM542XX__EEE_STAT_RX_DURATIONr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0fab, 0x15, (_val), (_mask))

/* Local LPI request Counter Reg (Addr ACh) */
#define PHY_READ_BCM542XX_EEE_STAT_TX_EVENTSr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0fac, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EEE_STAT_TX_EVENTSr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0fac, 0x15, (_val))
#define PHY_MODIFY_BCM542XX__EEE_STAT_TX_EVENTSr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0fac, 0x15, (_val), (_mask))

/* Remote LPI request Counter Reg (Addr ADh) */
#define PHY_READ_BCM542XX_EEE_STAT_RX_EVENTSr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0fad, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EEE_STAT_RX_EVENTSr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0fad, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EEE_STAT_RX_EVENTSr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0fad, 0x15, (_val), (_mask))

/* EEE Statistic counters ctrl/status (Addr AFh) */
#define PHY_READ_BCM542XX_EEE_STAT_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_RD((_unit), (_pc), 0x00, 0x0faf, 0x15, (_val))
#define PHY_WRITE_BCM542XX_EEE_STAT_CTRLr(_unit, _pc, _val) \
            PHY_BCM542XX_REG_WR((_unit), (_pc), 0x00, 0x0faf, 0x15, (_val))
#define PHY_MODIFY_BCM542XX_EEE_STAT_CTRLr(_unit, _pc, _val, _mask) \
            PHY_BCM542XX_REG_MOD((_unit), (_pc), 0x00, 0x0faf, 0x15, (_val), (_mask))

/*
 *
 * CL45 Registers: EEE
 * 
 */

/* EEE Capability Register */
#define PHY_READ_BCM542XX_EEE_CAPr( _unit, _pc, _val) \
        PHY_BCM542XX_CL45_REG_READ( (_unit), (_pc), 0x3, 0x14, (_val))

/* EEE Advertisement Register */
#define PHY_READ_BCM542XX_EEE_ADVr(_unit, _pc, _val) \
        PHY_BCM542XX_CL45_REG_READ( (_unit), (_pc), 0x7, 0x3c, _val)
#define PHY_WRITE_BCM542XX_EEE_ADVr(_unit, _pc, _val) \
        PHY_BCM542XX_CL45_REG_WRITE((_unit), (_pc),  0x7, 0x3c, (_val))
#define PHY_MODIFY_BCM542XX_EEE_ADVr(_unit, _pc, _val, _mask) \
        PHY_BCM542XX_CL45_REG_MODIFY((_unit), (_pc), 0x7, 0x3c, (_val), (_mask))

#define PHY_READ_BCM542XX_EEE_803Dr(_unit, _pc, _val) \
        PHY_BCM542XX_CL45_REG_READ(_unit, _pc,  0x7, 0x803d, _val)
#define PHY_WRITE_BCM542XX_EEE_803Dr(_unit, _pc, _val) \
        PHY_BCM542XX_CL45_REG_WRITE(_unit, _pc,  0x7, 0x803d, _val)
#define PHY_MODIFY_BCM542XX_EEE_803Dr(_unit, _pc, _val, _mask) \
        PHY_BCM542XX_CL45_REG_MODIFY(_unit, _pc, 0x7, 0x803d, _val, _mask)

/* EEE Resolution Status Register */
#define PHY_READ_BCM542XX_EEE_RESOLUTION_STATr( _unit, _pc, _val) \
        PHY_BCM542XX_CL45_REG_READ(_unit, _pc,  0x7, 0x803e, _val)
#define PHY_WRITE_BCM542XX_EEE_RESOLUTION_STATr(_unit, _pc, _val) \
        PHY_BCM542XX_CL45_REG_WRITE(_unit, _pc, 0x7, 0x803e, _val)
#define PHY_MODIFY_BCM542XX_EEE_RESOLUTION_STATr(_unit, _pc, _val, _mask) \
        PHY_BCM542XX_CL45_REG_MODIFY(_unit, _pc,, 0x7, 0x803e, _val, _mask)

#define PHY_READ_BCM542XX_EEE_TEST_CTRL_Ar(_unit, _pc, _val) \
        PHY_BCM542XX_CL45_REG_READ( _phy_id, 0x7, 0x8031, _val)
#define PHY_WRITE_BCM542XX_EEE_TEST_CTRL_Ar(_unit, _pc, _val) \
        PHY_BCM542XX_CL45_REG_WRITE( _phy_id, 0x7, 0x8031, _val)
#define PHY_MODIFY_BCM542XX_EEE_TEST_CTRL_Ar(_unit, _pc, _val, _mask) \
        PHY_BCM542XX_CL45_REG_MODIFY(_unit, _pc, 0x7, 0x8031, _val, _mask)

typedef struct {
    soc_port_config_phy_oam_t oam_config;
    uint32 flags;
    uint16 phy_id_orig;
    uint16 phy_id_base; /* port 0 addr */
    uint16 phy_slice;
} PHY_BCM542XX_DEV_DESC_t;

#define PHY_BCM542XX_DEV_OAM_CONFIG_PTR(_pc) \
              (&(((PHY_BCM542XX_DEV_DESC_t *)((_pc) + 1))->oam_config))
#define PHY_BCM542XX_DEV_PHY_ID_ORIG(_pc) \
                (((PHY_BCM542XX_DEV_DESC_t *)((_pc) + 1))->phy_id_orig)
#define PHY_BCM542XX_DEV_PHY_ID_BASE(_pc) \
                (((PHY_BCM542XX_DEV_DESC_t *)((_pc) + 1))->phy_id_base)
#define PHY_BCM542XX_DEV_PHY_SLICE(_pc) \
                (((PHY_BCM542XX_DEV_DESC_t *)((_pc) + 1))->phy_slice)
#define PHY_BCM542XX_FLAGS(_pc) \
                (((PHY_BCM542XX_DEV_DESC_t *)((_pc) + 1))->flags)
#define PHY_BCM542XX_PHYA_REV (1U << 0)

#define PHY_BCM542XX_DEV_SET_BASE_ADDR(_pc) \
            PHY_BCM542XX_DEV_PHY_ID_BASE(_pc) = \
                (PHY_BCM542XX_FLAGS((_pc)) & PHY_BCM542XX_PHYA_REV) \
                 ? PHY_BCM542XX_DEV_PHY_ID_ORIG(_pc) + PHY_BCM542XX_DEV_PHY_SLICE(_pc) \
                 : PHY_BCM542XX_DEV_PHY_ID_ORIG(_pc) - PHY_BCM542XX_DEV_PHY_SLICE(_pc)

#define PHY_BCM542XX_SLICE_ADDR(_pc, _slice) \
                (PHY_BCM542XX_FLAGS((_pc)) & PHY_BCM542XX_PHYA_REV) \
                 ? (PHY_BCM542XX_DEV_PHY_ID_BASE(_pc) - (_slice)) \
                 : (PHY_BCM542XX_DEV_PHY_ID_BASE(_pc) + (_slice))

#endif /* _PHY542XX_INT_H */
