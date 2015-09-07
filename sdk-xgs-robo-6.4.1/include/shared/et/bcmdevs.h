/*
 * $Id: bcmdevs.h,v 1.7 Broadcom SDK $
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
 * Broadcom device-specific manifest constants.
 */

#ifndef	_BCMDEVS_H
#define	_BCMDEVS_H


/* Known PCI vendor Id's */
#define	VENDOR_BROADCOM_ID		0x14e4

/* PCI Device Id's */
#define	BCM4210_DEVICE_ID	0x1072		/* never used */
#define	BCM4211_DEVICE_ID	0x4211
#define	BCM4230_DEVICE_ID	0x1086		/* never used */
#define	BCM4231_DEVICE_ID	0x4231

#define	BCM4410_DEVICE_ID	0x4410		/* bcm44xx family pci iline */
#define	BCM4430_DEVICE_ID	0x4430		/* bcm44xx family cardbus iline */
#define	BCM4412_DEVICE_ID	0x4412		/* bcm44xx family pci enet */
#define	BCM4432_DEVICE_ID	0x4432		/* bcm44xx family cardbus enet */

#define	BCM3352_DEVICE_ID	0x3352		/* bcm3352 device id */
#define	BCM3360_DEVICE_ID	0x3360		/* bcm3360 device id */

#define	EPI41210_DEVICE_ID	0xa0fa		/* bcm4210 */
#define	EPI41230_DEVICE_ID	0xa10e		/* bcm4230 */

#define	BCM4710_DEVICE_ID	0x4704		/* 4710 primary function 0 */
#define	BCM47XX_ILINE_ID	0x4711		/* 4710 iline20 */
#define	BCM47XX_V90_ID		0x4712		/* 4710 v90 codec */
#define	BCM47XX_ENET_ID		0x4713		/* 4710 enet */
#define	BCM47XX_EXT_ID		0x4714		/* 4710 external i/f */
#define	BCM47XX_USB_ID		0x4715		/* 4710 usb */

#define	BCM4610_DEVICE_ID	0x4610		/* 4610 primary function 0 */
#define	BCM4610_ILINE_ID	0x4611		/* 4610 iline100 */
#define	BCM4610_V90_ID		0x4612		/* 4610 v90 codec */
#define	BCM4610_ENET_ID		0x4613		/* 4610 enet */
#define	BCM4610_EXT_ID		0x4614		/* 4610 external i/f */
#define	BCM4610_USB_ID		0x4615		/* 4610 usb */

#define	BCM4402_DEVICE_ID	0x4402		/* 4402 primary function 0 */
#define	BCM4402_ENET_ID		0x4402		/* 4402 enet */
#define	BCM4402_V90_ID		0x4403		/* 4402 v90 codec */

#define	BCM4301_DEVICE_ID	0x4301		/* 4301 primary function 0 */
#define	BCM4301_D11B_ID		0x4301		/* 4301 802.11b */

#define	BCM4307_DEVICE_ID	0x4307		/* 4307 primary function 0 */
#define	BCM4307_V90_ID		0x4305		/* 4307 v90 codec */
#define	BCM4307_ENET_ID		0x4306		/* 4307 enet */
#define	BCM4307_D11B_ID		0x4307		/* 4307 802.11b */

#define	BCM4306_DEVICE_ID	0x4306		/* 4306 chipcommon chipid */
#define	BCM4306_D11G_ID		0x4320		/* 4306 802.11g */
#define	BCM4306_D11A_ID		0x4321		/* 4306 802.11a */
#define	BCM4306_UART_ID		0x4322		/* 4306 uart */
#define	BCM4306_V90_ID		0x4323		/* 4306 v90 codec */
#define	BCM4306_D11DUAL_ID	0x4324		/* 4306 dual A+B */

#define	BCM4310_DEVICE_ID	0x4310		/* 4310 chipcommon chipid */
#define	BCM4310_D11B_ID		0x4311		/* 4310 802.11b */
#define	BCM4310_UART_ID		0x4312		/* 4310 uart */
#define	BCM4310_ENET_ID		0x4313		/* 4310 enet */
#define	BCM4310_USB_ID		0x4315		/* 4310 usb */

#define	BCM4704_DEVICE_ID	0x4704		/* 4704 chipcommon chipid */
#define	BCM4704_V90_ID		0x4705		/* 4704 802.11b */
#define	BCM4704_ENET_ID		0x4706		/* 4704 enet */
#define	BCM4704_USB_ID		0x4707		/* 4704 usb */
#define	BCM4704_IPSEC_ID	0x4708		/* 4704 ipsec */

#define	BCM4317_DEVICE_ID	0x4317		/* 4317 Id */
#define BCM5365_DEVICE_ID       0x5365          /* 5365 chipcommon chipid */

#define BCM53000_CHIP_ID            0x5300          /* 53000 chipcommon chipid*/
#define BCM53000_GMAC_ID      0x4715      /* 53003 gmac id */

#define BCM53010_CHIP_ID      0xcf12      /* 53010 chipcommon chipid */
#define BCM53018_CHIP_ID      0xcf1a      /* 53018 chipcommon chipid */
#define BCM53010_GMAC_ID      0x4715      /* 5301x gmac id */

#define BCM53020_CHIP_ID      0xcf1e      /* 53020 chipcommon chipid */

/* PCMCIA vendor Id's */

/* boardflags */
#define	BFL_BTCOEXIST		0x0001	/* This board implements Bluetooth coexistance */
#define	BFL_PACTRL		0x0002	/* This board has gpio 9 controlling the PA */
#define	BFL_AIRLINEMODE		0x0004	/* This board implements airline mode */
#define	BFL_ADCDIV		0x0008	/* This board has the rssi ADC divider */
#define	BFL_ENETSPI		0x0010	/* This board has ephy roboswitch spi */
#define	BFL_NOPLLDOWN		0x0020	/* Not ok to power down the chip pll and oscillator */

/* Bus types */
#define	SB_BUS			0		/* Silicon Backplane */
#define	PCI_BUS			1		/* PCI target */
#define	PCMCIA_BUS		2		/* PCMCIA target */

/* Reference Board Types */

#define	BU4710_BOARD		0x0400
#define	VSIM4710_BOARD		0x0401
#define	QT4710_BOARD		0x0402

#define	BU4610_BOARD		0x0403
#define	VSIM4610_BOARD		0x0404

#define	BU4307_BOARD		0x0405
#define	BCM94301CB_BOARD	0x0406
#define	BCM94301PC_BOARD	0x0406		/* Pcmcia 5v card */
#define	BCM94301MP_BOARD	0x0407
#define	BCM94307MP_BOARD	0x0408
#define	BCMAP4307_BOARD		0x0409

#define	BU4309_BOARD		0x040a
#define	BCM94309CB_BOARD	0x040b
#define	BCM94309MP_BOARD	0x040c
#define	BCM4309AP_BOARD		0x040d

#define	BCM94302MP_BOARD	0x040e

#define	VSIM4310_BOARD		0x040f
#define	BU4711_BOARD		0x0410
#define	BCM94310U_BOARD		0x0411
#define	BCM94310AP_BOARD	0x0412
#define	BCM94310MP_BOARD	0x0414

#define	BU4306_BOARD		0x0416
#define	BCM94306CB_BOARD	0x0417
#define	BCM94306MP_BOARD	0x0418

#define	BCM94710D_BOARD		0x041a
#define	BCM94710R1_BOARD	0x041b
#define	BCM94710R4_BOARD	0x041c
#define	BCM94710AP_BOARD	0x041d

#define	BCM94301P50_BOARD	0x041e

#define	BU2050_BOARD		0x041f

#define	BCM94306P50_BOARD	0x0420

#define	BCM94309G_BOARD		0x0421

#define	BCM94301PC3_BOARD	0x0422		/* Pcmcia 3.3v card */

#define	BU4704_BOARD		0x0423
#define	BU4302_BOARD		0x0424

#define	BCM94306PC_BOARD	0x0425		/* pcmcia 3.3v 4306 card */

#define	BU4317_BOARD		0x0426

#define	MPSG4306_BOARD		0x0427

#define	BCM94702MN_BOARD	0x0428

/* BCM4702 1U CompactPCI Board */
#define	BCM94702CPCI_BOARD	0x0429

/* BCM4702 with BCM95380 VLAN Router */
#define	BCM95380RR_BOARD	0x042a

/* cb4306 with SiGe PA */
#define	BCM94306CBSG_BOARD	0x042b

/* mp4301 with 2050 radio */
#define	BCM94301MPL_BOARD	0x042c

/* 4306/gprs combo */
#define	BCM94306GPRS_BOARD	0x0432

/* BCM5365/BCM4704 FPGA Bringup Board */
#define BU5365_FPGA_BOARD      0x0433

#endif /* _BCMDEVS_H */
