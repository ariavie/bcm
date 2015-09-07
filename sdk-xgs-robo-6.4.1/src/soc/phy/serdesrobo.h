/*
 * $Id: serdesrobo.h,v 1.3 Broadcom SDK $
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
 * File:        serdes.h
 * Purpose:     
 */

#ifndef   _SERDES_ROBO_H_
#define   _SERDES_ROBO_H_

/*
 * Definitions for Quad Serdes Internal 10/100/1000 PHY
 * From: Serdes MII Register Specification: v2.0
 */

/* MII Control register */
#define QS_MII_CTRL_REG        MII_CTRL_REG /* 0x00 */
/* QS: MII Control bit definitions, same as Standard 10/100 PHY */
#define QS_MII_CTRL_RESET      MII_CTRL_RESET   /* Pulse 1us, for SW reset */
#define QS_MII_CTRL_SS_LSB     MII_CTRL_SS_LSB  /* Speed Select, LSB */
#define QS_MII_CTRL_SS_MSB     MII_CTRL_SS_MSB  /* Speed Select, MSB */
#define	QS_MII_CTRL_SS(_x)     MII_CTRL_SS(_x)
#define	QS_MII_CTRL_SS_10      MII_CTRL_SS_10
#define	QS_MII_CTRL_SS_100     MII_CTRL_SS_100
#define	QS_MII_CTRL_SS_1000    MII_CTRL_SS_1000
#define	QS_MII_CTRL_SS_INVALID MII_CTRL_SS_INVALID
#define	QS_MII_CTRL_SS_MASK    MII_CTRL_SS_MASK
#define QS_MII_CTRL_FD         MII_CTRL_FD     /* Full Duplex */
#define QS_MII_CTRL_RAN MII_CTRL_RAN /*restart auto-nego */
#define QS_MII_CTRL_AE  MII_CTRL_AE     /* enable auto-nego*/
#define QS_MII_CTRL_LE  MII_CTRL_LE

#define QS_MII_STAT_REG        MII_STAT_REG /* 0x01 */
/* QS: MII Status Register: not the same as standard 10/100 PHY */
#define QS_MII_STAT_RF         MII_STAT_RF     /* Remote fault */
#define QS_MII_STAT_LA         MII_STAT_LA     /* Link active */
#define QS_MII_FD_CAPABLE      (1 << 6)
#define QS_MII_HD_CAPABLE      (1 << 5)
#define QS_MII_ANA_COMPLETE MII_STAT_AN_DONE

/* Auto-negotiation advertisement register */
#define QS_ANA_REG		0x04

/* Auto-Negotiation Link-Partner Ability Register */
#define QS_ANP_REG		0x05

#define QS_MII_ANP_SGMII_LINK		(1 << 15)
#define QS_MII_ANP_SGMII_FD		(1 << 12)
#define QS_MII_ANP_SGMII_SPEED_SHFT   	10
#define QS_MII_ANP_SGMII_SPEED_MASK   	0x0c00

#define QS_MII_ANP_FIBER_NEXT_PG	(1 << 15)
#define QS_MII_ANP_FIBER_ACK   		(1 << 14)
#define QS_MII_ANP_FIBER_RF_SHFT  	12	/* Remote fault */
#define QS_MII_ANP_FIBER_RF_MASK   	0x3000
#define QS_MII_ANP_FIBER_PAUSE_ASYM 	(1 << 8)
#define QS_MII_ANP_FIBER_PAUSE_SYM  	(1 << 7)
#define QS_MII_ANP_FIBER_HD  		(1 << 6)
#define QS_MII_ANP_FIBER_FD  		(1 << 5)

/* Auto-Negotiation Expansion Register */
#define QS_ANA_EXPANSION_REG   MII_AN_EXP_REG
#define QS_ANA_EXPANSION_PR    (1 << 1) /* Page received */

/* SGMII Control #1 Register: Controls 10B/SGMII mode */
#define QS_SGMII_CTRL1_REG     0x10
#define QS_SGMII_AUTO_DETECT  (1 << 4)     /* Enable auto-detect mode*/
#define QS_SGMII_EN10B_MODE   (1 << 1) 	/* Enable TBI 10B interface */
#define QS_SGMII_FIBER_MODE   (1 << 0) 	/* Enable SGMII fiber mode */


#endif /* _SERDES_ROBO_H_ */
