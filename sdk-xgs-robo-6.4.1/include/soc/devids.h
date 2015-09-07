/*
 * $Id: devids.h,v 1.309 Broadcom SDK $
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

#ifndef _SOC_DEVIDS_H
#define _SOC_DEVIDS_H

/*
 * Defines PCI device and revision ID for every recognized device.
 * All driver routines refer to this ID only.
 */

#define BROADCOM_VENDOR_ID      0x14e4

/* Draco */
#define BCM5690_DEVICE_ID       0x5690
#define BCM5690_A0_REV_ID       1
#define BCM5690_A1_REV_ID       2
#define BCM5690_A2_REV_ID       3

/* Draco without HG (Medusa) */
#define BCM5691_DEVICE_ID       0x5691
#define BCM5691_A0_REV_ID       1
#define BCM5691_A1_REV_ID       2
#define BCM5691_A2_REV_ID       3

/* Draco without L3 */
#define BCM5692_DEVICE_ID       0x5692
#define BCM5692_A0_REV_ID       1
#define BCM5692_A1_REV_ID       2
#define BCM5692_A2_REV_ID       3

/* Draco without HG or L3 */
#define BCM5693_DEVICE_ID       0x5693
#define BCM5693_A0_REV_ID       1
#define BCM5693_A1_REV_ID       2
#define BCM5693_A2_REV_ID       3

/* Draco 1.5 */
#define BCM5695_DEVICE_ID       0x5695
#define BCM5695_A0_REV_ID       1
#define BCM5695_A1_REV_ID       2
#define BCM5695_B0_REV_ID       0x11

/* Draco 1.5 without HG */
#define BCM5696_DEVICE_ID       0x5696
#define BCM5696_A0_REV_ID       1
#define BCM5696_A1_REV_ID       2
#define BCM5696_B0_REV_ID       0x11

/* Draco 1.5 without L3 */
#define BCM5697_DEVICE_ID       0x5697
#define BCM5697_A0_REV_ID       1
#define BCM5697_A1_REV_ID       2
#define BCM5697_B0_REV_ID       0x11

/* Draco 1.5 without HG or L3 */
#define BCM5698_DEVICE_ID       0x5698
#define BCM5698_A0_REV_ID       1
#define BCM5698_A1_REV_ID       2
#define BCM5698_B0_REV_ID       0x11

/* Hercules with 8 ports */
#define BCM5670_DEVICE_ID       0x5670
#define BCM5670_A0_REV_ID       1
#define BCM5670_A1_REV_ID       2

/* Hercules with 4 ports */
#define BCM5671_DEVICE_ID       0x5671
#define BCM5671_A0_REV_ID       1
#define BCM5671_A1_REV_ID       2
#define BCM5671_A2_REV_ID       3       /* Maxxus */

/* Hercules 1.5 with 8 ports */
#define BCM5675_DEVICE_ID       0x5675
#define BCM5675_A0_REV_ID       1
#define BCM5675_A1_REV_ID       2

/* Hercules 1.5 with 4 ports */
#define BCM5676_DEVICE_ID       0x5676
#define BCM5676_A0_REV_ID       1
#define BCM5676_A1_REV_ID       2

/* Lynx */
#define BCM5673_DEVICE_ID       0x5673
#define BCM5673_A0_REV_ID       1
#define BCM5673_A1_REV_ID       2
#define BCM5673_A2_REV_ID       3

/* Lynx 1.5 */
#define BCM5674_DEVICE_ID       0x5674
#define BCM5674_A0_REV_ID       1

/* Felix */
#define BCM56100_DEVICE_ID      0xb100
#define BCM56100_A0_REV_ID      1
#define BCM56100_A1_REV_ID      2
#define BCM56101_DEVICE_ID      0xb101
#define BCM56101_A0_REV_ID      1
#define BCM56101_A1_REV_ID      2
#define BCM56102_DEVICE_ID      0xb102
#define BCM56102_A0_REV_ID      1
#define BCM56102_A1_REV_ID      2
#define BCM56105_DEVICE_ID      0xb105
#define BCM56105_A0_REV_ID      1
#define BCM56105_A1_REV_ID      2
#define BCM56106_DEVICE_ID      0xb106
#define BCM56106_A0_REV_ID      1
#define BCM56106_A1_REV_ID      2
#define BCM56107_DEVICE_ID      0xb107
#define BCM56107_A0_REV_ID      1
#define BCM56107_A1_REV_ID      2

/* Felix 1.5 */
#define BCM56110_DEVICE_ID      0xb110
#define BCM56110_A0_REV_ID      1
#define BCM56111_DEVICE_ID      0xb111
#define BCM56111_A0_REV_ID      1
#define BCM56112_DEVICE_ID      0xb112
#define BCM56112_A0_REV_ID      1
#define BCM56115_DEVICE_ID      0xb115
#define BCM56115_A0_REV_ID      1
#define BCM56116_DEVICE_ID      0xb116
#define BCM56116_A0_REV_ID      1
#define BCM56117_DEVICE_ID      0xb117
#define BCM56117_A0_REV_ID      1

/* Helix */
#define BCM56300_DEVICE_ID      0xb300
#define BCM56300_A0_REV_ID      1
#define BCM56300_A1_REV_ID      2
#define BCM56300_B0_REV_ID      0x11
#define BCM56300_B1_REV_ID      0x12
#define BCM56301_DEVICE_ID      0xb301
#define BCM56301_A0_REV_ID      1
#define BCM56301_A1_REV_ID      2
#define BCM56301_B0_REV_ID      0x11
#define BCM56301_B1_REV_ID      0x12
#define BCM56302_DEVICE_ID      0xb302
#define BCM56302_A0_REV_ID      1
#define BCM56302_A1_REV_ID      2
#define BCM56302_B0_REV_ID      0x11
#define BCM56302_B1_REV_ID      0x12
#define BCM56303_DEVICE_ID      0xb303
#define BCM56303_A1_REV_ID      2
#define BCM56303_A0_REV_ID      1
#define BCM56303_B0_REV_ID      0x11
#define BCM56303_B1_REV_ID      0x12
#define BCM56304_DEVICE_ID      0xb304
#define BCM56304_A0_REV_ID      1
#define BCM56304_A1_REV_ID      2
#define BCM56304_B0_REV_ID      0x11
#define BCM56304_B1_REV_ID      0x12
#define BCM56404_DEVICE_ID      0xb404
#define BCM56404_A0_REV_ID      1
#define BCM56404_A1_REV_ID      2
#define BCM56305_DEVICE_ID      0xb305
#define BCM56305_A0_REV_ID      1
#define BCM56305_A1_REV_ID      2
#define BCM56305_B0_REV_ID      0x11
#define BCM56305_B1_REV_ID      0x12
#define BCM56306_DEVICE_ID      0xb306
#define BCM56306_A0_REV_ID      1
#define BCM56306_A1_REV_ID      2
#define BCM56306_B0_REV_ID      0x11
#define BCM56306_B1_REV_ID      0x12
#define BCM56307_DEVICE_ID      0xb307
#define BCM56307_A0_REV_ID      1
#define BCM56307_A1_REV_ID      2
#define BCM56307_B0_REV_ID      0x11
#define BCM56307_B1_REV_ID      0x12
#define BCM56308_DEVICE_ID      0xb308
#define BCM56308_A0_REV_ID      1
#define BCM56308_A1_REV_ID      2
#define BCM56308_B0_REV_ID      0x11
#define BCM56308_B1_REV_ID      0x12
#define BCM56309_DEVICE_ID      0xb309
#define BCM56309_A0_REV_ID      1
#define BCM56309_A1_REV_ID      2
#define BCM56309_B0_REV_ID      0x11
#define BCM56309_B1_REV_ID      0x12

/* Helix 1.5 */
#define BCM56310_DEVICE_ID      0xb310
#define BCM56310_A0_REV_ID      1
#define BCM56311_DEVICE_ID      0xb311
#define BCM56311_A0_REV_ID      1
#define BCM56312_DEVICE_ID      0xb312
#define BCM56312_A0_REV_ID      1
#define BCM56313_DEVICE_ID      0xb313
#define BCM56313_A0_REV_ID      1
#define BCM56314_DEVICE_ID      0xb314
#define BCM56314_A0_REV_ID      1
#define BCM56315_DEVICE_ID      0xb315
#define BCM56315_A0_REV_ID      1
#define BCM56316_DEVICE_ID      0xb316
#define BCM56316_A0_REV_ID      1
#define BCM56317_DEVICE_ID      0xb317
#define BCM56317_A0_REV_ID      1
#define BCM56318_DEVICE_ID      0xb318
#define BCM56318_A0_REV_ID      1
#define BCM56319_DEVICE_ID      0xb319
#define BCM56319_A0_REV_ID      1

#ifndef EXCLUDE_BCM56324
/* Helix 2 */
#define BCM56322_DEVICE_ID      0xb322
#define BCM56322_A0_REV_ID      1
#define BCM56324_DEVICE_ID      0xb324
#define BCM56324_A0_REV_ID      1
#endif /* EXCLUDE_BCM56324 */

#define BCM53300_DEVICE_ID      0xb006
#define BCM53300_A0_REV_ID      0x11
#define BCM53300_A1_REV_ID      0x12
#define BCM53301_DEVICE_ID      0xb206
#define BCM53301_A0_REV_ID      0x11
#define BCM53301_A1_REV_ID      0x12
#define BCM53302_DEVICE_ID      0xb008
#define BCM53302_A0_REV_ID      0x11
#define BCM53302_A1_REV_ID      0x12

/* Firebolt */
#define BCM56500_DEVICE_ID      0xb500
#define BCM56500_A0_REV_ID      1
#define BCM56500_A1_REV_ID      2
#define BCM56500_B0_REV_ID      0x11
#define BCM56500_B1_REV_ID      0x12
#define BCM56500_B2_REV_ID      0x13
#define BCM56501_DEVICE_ID      0xb501
#define BCM56501_A0_REV_ID      1
#define BCM56501_A1_REV_ID      2
#define BCM56501_B0_REV_ID      0x11
#define BCM56501_B1_REV_ID      0x12
#define BCM56501_B2_REV_ID      0x13
#define BCM56502_DEVICE_ID      0xb502
#define BCM56502_A0_REV_ID      1
#define BCM56502_A1_REV_ID      2
#define BCM56502_B0_REV_ID      0x11
#define BCM56502_B1_REV_ID      0x12
#define BCM56502_B2_REV_ID      0x13
#define BCM56503_DEVICE_ID      0xb503
#define BCM56503_A0_REV_ID      1
#define BCM56503_A1_REV_ID      2
#define BCM56503_B0_REV_ID      0x11
#define BCM56503_B1_REV_ID      0x12
#define BCM56503_B2_REV_ID      0x13
#define BCM56504_DEVICE_ID      0xb504
#define BCM56504_A0_REV_ID      1
#define BCM56504_A1_REV_ID      2
#define BCM56504_B0_REV_ID      0x11
#define BCM56504_B1_REV_ID      0x12
#define BCM56504_B2_REV_ID      0x13
#define BCM56505_DEVICE_ID      0xb505
#define BCM56505_A0_REV_ID      1
#define BCM56505_A1_REV_ID      2
#define BCM56505_B0_REV_ID      0x11
#define BCM56505_B1_REV_ID      0x12
#define BCM56505_B2_REV_ID      0x13
#define BCM56506_DEVICE_ID      0xb506
#define BCM56506_A0_REV_ID      1
#define BCM56506_A1_REV_ID      2
#define BCM56506_B0_REV_ID      0x11
#define BCM56506_B1_REV_ID      0x12
#define BCM56506_B2_REV_ID      0x13
#define BCM56507_DEVICE_ID      0xb507
#define BCM56507_A0_REV_ID      1
#define BCM56507_A1_REV_ID      2
#define BCM56507_B0_REV_ID      0x11
#define BCM56507_B1_REV_ID      0x12
#define BCM56507_B2_REV_ID      0x13
#define BCM56508_DEVICE_ID      0xb508
#define BCM56508_A0_REV_ID      1
#define BCM56508_A1_REV_ID      2
#define BCM56508_B0_REV_ID      0x11
#define BCM56508_B1_REV_ID      0x12
#define BCM56508_B2_REV_ID      0x13
#define BCM56509_DEVICE_ID      0xb509
#define BCM56509_A0_REV_ID      1
#define BCM56509_A1_REV_ID      2
#define BCM56509_B0_REV_ID      0x11
#define BCM56509_B1_REV_ID      0x12
#define BCM56509_B2_REV_ID      0x13

/* Easyrider */
#define BCM56600_DEVICE_ID      0xb600
#define BCM56600_A0_REV_ID      1
#define BCM56600_B0_REV_ID      0x11
#define BCM56600_C0_REV_ID      0x21
#define BCM56601_DEVICE_ID      0xb601
#define BCM56601_A0_REV_ID      1
#define BCM56601_B0_REV_ID      0x11
#define BCM56601_C0_REV_ID      0x21
#define BCM56602_DEVICE_ID      0xb602
#define BCM56602_A0_REV_ID      1
#define BCM56602_B0_REV_ID      0x11
#define BCM56602_C0_REV_ID      0x21
#define BCM56603_DEVICE_ID      0xb603
#define BCM56603_A0_REV_ID      1
#define BCM56603_B0_REV_ID      0x11
#define BCM56603_C0_REV_ID      0x21
#define BCM56605_DEVICE_ID      0xb605
#define BCM56605_A0_REV_ID      1
#define BCM56605_B0_REV_ID      0x11
#define BCM56605_C0_REV_ID      0x21
#define BCM56606_DEVICE_ID      0xb606
#define BCM56606_A0_REV_ID      1
#define BCM56606_B0_REV_ID      0x11
#define BCM56606_C0_REV_ID      0x21
#define BCM56607_DEVICE_ID      0xb607
#define BCM56607_A0_REV_ID      1
#define BCM56607_B0_REV_ID      0x11
#define BCM56607_C0_REV_ID      0x21
#define BCM56608_DEVICE_ID      0xb608
#define BCM56608_A0_REV_ID      1
#define BCM56608_B0_REV_ID      0x11
#define BCM56608_C0_REV_ID      0x21

/* Goldwing */
#define BCM56580_DEVICE_ID      0xb580
#define BCM56580_A0_REV_ID      1

/* HUMV */
#define BCM56700_DEVICE_ID      0xb700
#define BCM56700_A0_REV_ID      1
#define BCM56701_DEVICE_ID      0xb701
#define BCM56701_A0_REV_ID      1

/* Bradley */
#define BCM56800_DEVICE_ID      0xb800
#define BCM56800_A0_REV_ID      1
#define BCM56801_DEVICE_ID      0xb801
#define BCM56801_A0_REV_ID      1
#define BCM56802_DEVICE_ID      0xb802
#define BCM56802_A0_REV_ID      1
#define BCM56803_DEVICE_ID      0xb803
#define BCM56803_A0_REV_ID      1

/* Raven */
#define BCM56224_DEVICE_ID      0xb224
#define BCM56224_A0_REV_ID      1
#define BCM56224_B0_REV_ID      0x11
#define BCM56225_DEVICE_ID      0xb225
#define BCM56225_A0_REV_ID      1
#define BCM56225_B0_REV_ID      0x11
#define BCM56226_DEVICE_ID      0xb226
#define BCM56226_A0_REV_ID      1
#define BCM56226_B0_REV_ID      0x11
#define BCM56227_DEVICE_ID      0xb227
#define BCM56227_A0_REV_ID      1
#define BCM56227_B0_REV_ID      0x11
#define BCM56228_DEVICE_ID      0xb228
#define BCM56228_A0_REV_ID      1
#define BCM56228_B0_REV_ID      0x11
#define BCM56229_DEVICE_ID      0xb229
#define BCM56229_A0_REV_ID      1
#define BCM56229_B0_REV_ID      0x11
#define BCM56024_DEVICE_ID      0xb024
#define BCM56024_A0_REV_ID      1
#define BCM56024_B0_REV_ID      0x11
#define BCM56025_DEVICE_ID      0xb025
#define BCM56025_A0_REV_ID      1
#define BCM56025_B0_REV_ID      0x11
#define BCM53724_DEVICE_ID      0xc724
#define BCM53724_A0_REV_ID      1
#define BCM53724_B0_REV_ID      0x11
#define BCM53726_DEVICE_ID      0xc726
#define BCM53726_A0_REV_ID      1
#define BCM53726_B0_REV_ID      0x11

/* Hawkeye */
#define BCM53312_DEVICE_ID      0xc312
#define BCM53312_A0_REV_ID      1
#define BCM53312_B0_REV_ID      0x11
#define BCM53313_DEVICE_ID      0xc313
#define BCM53313_A0_REV_ID      1
#define BCM53313_B0_REV_ID      0x11
#define BCM53314_DEVICE_ID      0xc314
#define BCM53314_A0_REV_ID      1
#define BCM53314_B0_REV_ID      0x11

/* Hawkeye EEE */
#define BCM53322_DEVICE_ID      0xc322
#define BCM53322_A0_REV_ID      1
#define BCM53323_DEVICE_ID      0xc323
#define BCM53323_A0_REV_ID      1
#define BCM53324_DEVICE_ID      0xc324
#define BCM53324_A0_REV_ID      1

/* Raptor */
#define BCM56218_DEVICE_ID              0xB218
#define BCM56218_A0_REV_ID              1
#define BCM56218_A1_REV_ID              2
#define BCM56218_A2_REV_ID              3
#define BCM56218X_DEVICE_ID             0xc710
#define BCM56218X_A0_REV_ID             1
#define BCM56218X_A1_REV_ID             2
#define BCM56218X_A2_REV_ID             3
#define BCM56219_DEVICE_ID              0xB219
#define BCM56219_A0_REV_ID              1
#define BCM56219_A1_REV_ID              2
#define BCM56219_A2_REV_ID              3
#define BCM56218R_DEVICE_ID             0xB21A
#define BCM56218R_A0_REV_ID             1
#define BCM56218R_A1_REV_ID             2
#define BCM56218R_A2_REV_ID             3
#define BCM56219R_DEVICE_ID             0xB21B
#define BCM56219R_A0_REV_ID             1
#define BCM56219R_A1_REV_ID             2
#define BCM56219R_A2_REV_ID             3
#define BCM56214_DEVICE_ID              0xB214
#define BCM56214_A0_REV_ID              1
#define BCM56214_A1_REV_ID              2
#define BCM56214_A2_REV_ID              3
#define BCM56215_DEVICE_ID              0xB215
#define BCM56215_A0_REV_ID              1
#define BCM56215_A1_REV_ID              2
#define BCM56215_A2_REV_ID              3
#define BCM56214R_DEVICE_ID             0xB21C
#define BCM56214R_A0_REV_ID             1
#define BCM56214R_A1_REV_ID             2
#define BCM56214R_A2_REV_ID             3
#define BCM56215R_DEVICE_ID             0xB21D
#define BCM56215R_A0_REV_ID             1
#define BCM56215R_A1_REV_ID             2
#define BCM56215R_A2_REV_ID             3
#define BCM56216_DEVICE_ID              0xB216
#define BCM56216_A0_REV_ID              1
#define BCM56216_A1_REV_ID              2
#define BCM56216_A2_REV_ID              3
#define BCM56217_DEVICE_ID              0xB217
#define BCM56217_A0_REV_ID              1
#define BCM56217_A1_REV_ID              2
#define BCM56217_A2_REV_ID              3
#define BCM56212_DEVICE_ID              0xB212
#define BCM56212_A0_REV_ID              1
#define BCM56212_A1_REV_ID              2
#define BCM56212_A2_REV_ID              3
#define BCM56213_DEVICE_ID              0xB213
#define BCM56213_A0_REV_ID              1
#define BCM56213_A1_REV_ID              2
#define BCM56213_A2_REV_ID              3
#define BCM53718_DEVICE_ID              0xC71A
#define BCM53718_A0_REV_ID              1
#define BCM53718_A1_REV_ID              2
#define BCM53718_A2_REV_ID              3
#define BCM53714_DEVICE_ID              0xC71B
#define BCM53714_A0_REV_ID              1
#define BCM53714_A1_REV_ID              2
#define BCM53714_A2_REV_ID              3
#define BCM53716_DEVICE_ID              0xC716
#define BCM53716_A0_REV_ID              1
#define BCM53716_A1_REV_ID              2
#define BCM53716_A2_REV_ID              3
#define BCM56018_DEVICE_ID              0xB018
#define BCM56018_A0_REV_ID              1
#define BCM56018_A1_REV_ID              2
#define BCM56018_A2_REV_ID              3
#define BCM56014_DEVICE_ID              0xB014
#define BCM56014_A0_REV_ID              1
#define BCM56014_A1_REV_ID              2
#define BCM56014_A2_REV_ID              3

/* Firebolt2 */
#define BCM56510_DEVICE_ID      0xb510
#define BCM56510_A0_REV_ID      1
#define BCM56511_DEVICE_ID      0xb511
#define BCM56511_A0_REV_ID      1
#define BCM56512_DEVICE_ID      0xb512
#define BCM56512_A0_REV_ID      1
#define BCM56513_DEVICE_ID      0xb513
#define BCM56513_A0_REV_ID      1
#define BCM56514_DEVICE_ID      0xb514
#define BCM56514_A0_REV_ID      1
#define BCM56516_DEVICE_ID      0xb516
#define BCM56516_A0_REV_ID      1
#define BCM56517_DEVICE_ID      0xb517
#define BCM56517_A0_REV_ID      1
#define BCM56518_DEVICE_ID      0xb518
#define BCM56518_A0_REV_ID      1
#define BCM56519_DEVICE_ID      0xb519
#define BCM56519_A0_REV_ID      1

/* Triumph */
#define BCM56620_DEVICE_ID      0xb620
#define BCM56620_A0_REV_ID      1
#define BCM56620_A1_REV_ID      2
#define BCM56620_B0_REV_ID      0x11
#define BCM56620_B1_REV_ID      0x12
#define BCM56620_B2_REV_ID      0x13
#define BCM56624_DEVICE_ID      0xb624
#define BCM56624_A0_REV_ID      1
#define BCM56624_A1_REV_ID      2
#define BCM56624_B0_REV_ID      0x11
#define BCM56624_B1_REV_ID      0x12
#define BCM56624_B2_REV_ID      0x13
#define BCM56626_DEVICE_ID      0xb626
#define BCM56626_A0_REV_ID      1
#define BCM56626_A1_REV_ID      2
#define BCM56626_B0_REV_ID      0x11
#define BCM56626_B1_REV_ID      0x12
#define BCM56626_B2_REV_ID      0x13
#define BCM56628_DEVICE_ID      0xb628
#define BCM56628_A0_REV_ID      1
#define BCM56628_A1_REV_ID      2
#define BCM56628_B0_REV_ID      0x11
#define BCM56628_B1_REV_ID      0x12
#define BCM56628_B2_REV_ID      0x13
#define BCM56629_DEVICE_ID      0xb629
#define BCM56629_A0_REV_ID      1
#define BCM56629_A1_REV_ID      2
#define BCM56629_B0_REV_ID      0x11
#define BCM56629_B1_REV_ID      0x12
#define BCM56629_B2_REV_ID      0x13

/* Valkyrie */
#define BCM56680_DEVICE_ID      0xb680
#define BCM56680_A0_REV_ID      1
#define BCM56680_A1_REV_ID      2
#define BCM56680_B0_REV_ID      0x11
#define BCM56680_B1_REV_ID      0x12
#define BCM56680_B2_REV_ID      0x13
#define BCM56684_DEVICE_ID      0xb684
#define BCM56684_A0_REV_ID      1
#define BCM56684_A1_REV_ID      2
#define BCM56684_B0_REV_ID      0x11
#define BCM56684_B1_REV_ID      0x12
#define BCM56684_B2_REV_ID      0x13
#define BCM56686_DEVICE_ID      0xb686
#define BCM56686_B0_REV_ID      0x11
#define BCM56686_B1_REV_ID      0x12
#define BCM56686_B2_REV_ID      0x13

/* Scorpion */
#define BCM56820_DEVICE_ID      0xb820
#define BCM56820_A0_REV_ID      1
#define BCM56820_B0_REV_ID      0x11
#define BCM56821_DEVICE_ID      0xb821
#define BCM56821_A0_REV_ID      1
#define BCM56821_B0_REV_ID      0x11
#define BCM56822_DEVICE_ID      0xb822
#define BCM56822_A0_REV_ID      1
#define BCM56822_B0_REV_ID      0x11
#define BCM56823_DEVICE_ID      0xb823
#define BCM56823_A0_REV_ID      1
#define BCM56823_B0_REV_ID      0x11
#define BCM56825_DEVICE_ID      0xb825
#define BCM56825_B0_REV_ID      0x11

/* HUMV Plus */
#define BCM56720_DEVICE_ID      0xb720
#define BCM56720_A0_REV_ID      1
#define BCM56720_B0_REV_ID      0x11
#define BCM56721_DEVICE_ID      0xb721
#define BCM56721_A0_REV_ID      1
#define BCM56721_B0_REV_ID      0x11

/* Conqueror */
#define BCM56725_DEVICE_ID      0xb725
#define BCM56725_A0_REV_ID      1
#define BCM56725_B0_REV_ID      0x11

/* Triumph2 */
#define BCM56630_DEVICE_ID      0xb630
#define BCM56630_A0_REV_ID      1
#define BCM56630_B0_REV_ID      0x11
#define BCM56634_DEVICE_ID      0xb634
#define BCM56634_A0_REV_ID      1
#define BCM56634_B0_REV_ID      0x11
#define BCM56636_DEVICE_ID      0xb636
#define BCM56636_A0_REV_ID      1
#define BCM56636_B0_REV_ID      0x11
#define BCM56638_DEVICE_ID      0xb638
#define BCM56638_A0_REV_ID      1
#define BCM56638_B0_REV_ID      0x11
#define BCM56639_DEVICE_ID      0xb639
#define BCM56639_A0_REV_ID      1
#define BCM56639_B0_REV_ID      0x11

/* Valkyrie2 */
#define BCM56685_DEVICE_ID      0xb685
#define BCM56685_A0_REV_ID      1
#define BCM56685_B0_REV_ID      0x11
#define BCM56689_DEVICE_ID      0xb689
#define BCM56689_A0_REV_ID      1
#define BCM56689_B0_REV_ID      0x11

/* Apollo */
#define BCM56520_DEVICE_ID      0xb520
#define BCM56520_A0_REV_ID      1
#define BCM56520_B0_REV_ID      0x11
#define BCM56521_DEVICE_ID      0xb521
#define BCM56521_A0_REV_ID      1
#define BCM56521_B0_REV_ID      0x11
#define BCM56522_DEVICE_ID      0xb522
#define BCM56522_A0_REV_ID      1
#define BCM56522_B0_REV_ID      0x11
#define BCM56524_DEVICE_ID      0xb524
#define BCM56524_A0_REV_ID      1
#define BCM56524_B0_REV_ID      0x11
#define BCM56526_DEVICE_ID      0xb526
#define BCM56526_A0_REV_ID      1
#define BCM56526_B0_REV_ID      0x11

/* Firebolt 3 */
#define BCM56534_DEVICE_ID      0xb534
#define BCM56534_B0_REV_ID      0x11
#define BCM56538_DEVICE_ID      0xb538
#define BCM56538_B0_REV_ID      0x11

/* Enduro */
#define BCM56331_DEVICE_ID      0xb331
#define BCM56331_A0_REV_ID      1
#define BCM56331_B0_REV_ID      0x11
#define BCM56331_B1_REV_ID      0x12
#define BCM56333_DEVICE_ID      0xb333
#define BCM56333_A0_REV_ID      1
#define BCM56333_B0_REV_ID      0x11
#define BCM56333_B1_REV_ID      0x12
#define BCM56334_DEVICE_ID      0xb334
#define BCM56334_A0_REV_ID      1
#define BCM56334_B0_REV_ID      0x11
#define BCM56334_B1_REV_ID      0x12
#define BCM56338_DEVICE_ID      0xb338
#define BCM56338_A0_REV_ID      1
#define BCM56338_B0_REV_ID      0x11
#define BCM56338_B1_REV_ID      0x12

/* Helix 3 */
#define BCM56320_DEVICE_ID      0xb320
#define BCM56320_A0_REV_ID      1
#define BCM56320_B0_REV_ID      0x11
#define BCM56320_B1_REV_ID      0x12
#define BCM56321_DEVICE_ID      0xb321
#define BCM56321_A0_REV_ID      1
#define BCM56321_B0_REV_ID      0x11
#define BCM56321_B1_REV_ID      0x12


/* FireScout */
#define BCM56548_DEVICE_ID      0xb548
#define BCM56548_A0_REV_ID      1
#define BCM56547_DEVICE_ID      0xb547
#define BCM56547_A0_REV_ID      1

/* Helix 4 */
#define BCM56347_DEVICE_ID      0xb347
#define BCM56347_A0_REV_ID      1
#define BCM56346_DEVICE_ID      0xb346
#define BCM56346_A0_REV_ID      1
#define BCM56344_DEVICE_ID      0xb344
#define BCM56344_A0_REV_ID      1
#define BCM56342_DEVICE_ID      0xb342
#define BCM56342_A0_REV_ID      1
#define BCM56340_DEVICE_ID      0xb340
#define BCM56340_A0_REV_ID      1

/* Spiral */
#define BCM56049_DEVICE_ID      0xb049
#define BCM56049_A0_REV_ID      1
#define BCM56048_DEVICE_ID      0xb048
#define BCM56048_A0_REV_ID      1
#define BCM56047_DEVICE_ID      0xb047
#define BCM56047_A0_REV_ID      1

/* Ranger */
#define BCM56042_DEVICE_ID      0xb042
#define BCM56042_A0_REV_ID      1
#define BCM56041_DEVICE_ID      0xb041
#define BCM56041_A0_REV_ID      1
#define BCM56040_DEVICE_ID      0xb040
#define BCM56040_A0_REV_ID      1

/* Stardust */
#define BCM56132_DEVICE_ID      0xb132
#define BCM56132_A0_REV_ID      1
#define BCM56132_B0_REV_ID      0x11
#define BCM56132_B1_REV_ID      0x12
#define BCM56134_DEVICE_ID      0xb134
#define BCM56134_A0_REV_ID      1
#define BCM56134_B0_REV_ID      0x11
#define BCM56134_B1_REV_ID      0x12

/* Dagger */
#define BCM56230_DEVICE_ID      0xb230
#define BCM56230_B1_REV_ID      0x12
#define BCM56231_DEVICE_ID      0xb231
#define BCM56231_B1_REV_ID      0x12

/* Hurricane */
#define BCM56140_DEVICE_ID      0xb140
#define BCM56140_A0_REV_ID      1
#define BCM56142_DEVICE_ID      0xb142
#define BCM56142_A0_REV_ID      1
#define BCM56143_DEVICE_ID      0xb143
#define BCM56143_A0_REV_ID      1
#define BCM56144_DEVICE_ID      0xb144
#define BCM56144_A0_REV_ID      1
#define BCM56146_DEVICE_ID      0xb146
#define BCM56146_A0_REV_ID      1
#define BCM56147_DEVICE_ID      0xb147
#define BCM56147_A0_REV_ID      1
#define BCM56149_DEVICE_ID      0xb149
#define BCM56149_A0_REV_ID      1

/* Trident */
#define BCM56840_DEVICE_ID      0xb840
#define BCM56840_A0_REV_ID      1
#define BCM56840_A1_REV_ID      2
#define BCM56840_A2_REV_ID      3
#define BCM56840_A3_REV_ID      4
#define BCM56840_A4_REV_ID      5
#define BCM56840_B0_REV_ID      0x11
#define BCM56840_B1_REV_ID      0x12
#define BCM56841_DEVICE_ID      0xb841
#define BCM56841_A0_REV_ID      1
#define BCM56841_A1_REV_ID      2
#define BCM56841_A2_REV_ID      3
#define BCM56841_A3_REV_ID      4
#define BCM56841_A4_REV_ID      5
#define BCM56841_B0_REV_ID      0x11
#define BCM56841_B1_REV_ID      0x12
#define BCM56843_DEVICE_ID      0xb843
#define BCM56843_A0_REV_ID      1
#define BCM56843_A1_REV_ID      2
#define BCM56843_A2_REV_ID      3
#define BCM56843_A3_REV_ID      4
#define BCM56843_A4_REV_ID      5
#define BCM56843_B0_REV_ID      0x11
#define BCM56843_B1_REV_ID      0x12
#define BCM56845_DEVICE_ID      0xb845
#define BCM56845_A0_REV_ID      1
#define BCM56845_A1_REV_ID      2
#define BCM56845_A2_REV_ID      3
#define BCM56845_A3_REV_ID      4
#define BCM56845_A4_REV_ID      5
#define BCM56845_B0_REV_ID      0x11
#define BCM56845_B1_REV_ID      0x12

/* Titan */
#define BCM56743_DEVICE_ID      0xb743
#define BCM56743_A0_REV_ID      1
#define BCM56743_A1_REV_ID      2
#define BCM56743_A2_REV_ID      3
#define BCM56743_A3_REV_ID      4
#define BCM56743_A4_REV_ID      5
#define BCM56743_B0_REV_ID      0x11
#define BCM56743_B1_REV_ID      0x12
#define BCM56745_DEVICE_ID      0xb745
#define BCM56745_A0_REV_ID      1
#define BCM56745_A1_REV_ID      2
#define BCM56745_A2_REV_ID      3
#define BCM56745_A3_REV_ID      4
#define BCM56745_A4_REV_ID      5
#define BCM56745_B0_REV_ID      0x11
#define BCM56745_B1_REV_ID      0x12

/* Trident Plus */
#define BCM56842_DEVICE_ID      0xb842
#define BCM56842_A0_REV_ID      1
#define BCM56842_A1_REV_ID      2
#define BCM56844_DEVICE_ID      0xb844
#define BCM56844_A0_REV_ID      1
#define BCM56844_A1_REV_ID      2
#define BCM56846_DEVICE_ID      0xb846
#define BCM56846_A0_REV_ID      1
#define BCM56846_A1_REV_ID      2
#define BCM56549_DEVICE_ID      0xb549
#define BCM56549_A0_REV_ID      1
#define BCM56549_A1_REV_ID      2
#define BCM56053_DEVICE_ID      0xb053
#define BCM56053_A0_REV_ID      1
#define BCM56053_A1_REV_ID      2
#define BCM56831_DEVICE_ID      0xb831
#define BCM56831_A0_REV_ID      1
#define BCM56831_A1_REV_ID      2
#define BCM56835_DEVICE_ID      0xb835
#define BCM56835_A0_REV_ID      1
#define BCM56835_A1_REV_ID      2
#define BCM56838_DEVICE_ID      0xb838
#define BCM56838_A0_REV_ID      1
#define BCM56838_A1_REV_ID      2
#define BCM56847_DEVICE_ID      0xb847
#define BCM56847_A0_REV_ID      1
#define BCM56847_A1_REV_ID      2
#define BCM56847_A2_REV_ID      3
#define BCM56847_A3_REV_ID      4
#define BCM56847_A4_REV_ID      5
#define BCM56847_B0_REV_ID      0x11
#define BCM56847_B1_REV_ID      0x12
#define BCM56849_DEVICE_ID      0xb849
#define BCM56849_A0_REV_ID      1
#define BCM56849_A1_REV_ID      2

/* Titan Plus */
#define BCM56744_DEVICE_ID      0xb744
#define BCM56744_A0_REV_ID      1
#define BCM56744_A1_REV_ID      2
#define BCM56746_DEVICE_ID      0xb746
#define BCM56746_A0_REV_ID      1
#define BCM56746_A1_REV_ID      2

/* Sirius */
#define BCM88230_DEVICE_ID      0x0230
#define BCM88230_A0_REV_ID      1
#define BCM88230_B0_REV_ID      0x11
#define BCM88230_C0_REV_ID      0x21
#define BCM88231_DEVICE_ID      0x0231
#define BCM88231_A0_REV_ID      1
#define BCM88231_B0_REV_ID      0x11
#define BCM88231_C0_REV_ID      0x21
#define BCM88235_DEVICE_ID      0x0235
#define BCM88235_A0_REV_ID      1
#define BCM88235_B0_REV_ID      0x11
#define BCM88235_C0_REV_ID      0x21
#define BCM88236_DEVICE_ID      0x0236
#define BCM88236_A0_REV_ID      1
#define BCM88236_B0_REV_ID      0x11
#define BCM88236_C0_REV_ID      0x21
#define BCM88239_DEVICE_ID      0x0239
#define BCM88239_A0_REV_ID      1
#define BCM88239_B0_REV_ID      0x11
#define BCM88239_C0_REV_ID      0x21
#define BCM56613_DEVICE_ID      0xb613
#define BCM56613_A0_REV_ID      1
#define BCM56613_B0_REV_ID      0x11
#define BCM56613_C0_REV_ID      0x21
#define BCM56930_DEVICE_ID      0xb930
#define BCM56930_A0_REV_ID      1
#define BCM56930_B0_REV_ID      0x11
#define BCM56930_C0_REV_ID      0x21
#define BCM56931_DEVICE_ID      0xb931
#define BCM56931_A0_REV_ID      1
#define BCM56931_B0_REV_ID      0x11
#define BCM56931_C0_REV_ID      0x21
#define BCM56935_DEVICE_ID      0xb935
#define BCM56935_A0_REV_ID      1
#define BCM56935_B0_REV_ID      0x11
#define BCM56935_C0_REV_ID      0x21
#define BCM56936_DEVICE_ID      0xb936
#define BCM56936_A0_REV_ID      1
#define BCM56936_B0_REV_ID      0x11
#define BCM56936_C0_REV_ID      0x21
#define BCM56939_DEVICE_ID      0xb939
#define BCM56939_A0_REV_ID      1
#define BCM56939_B0_REV_ID      0x11
#define BCM56939_C0_REV_ID      0x21

/* Shadow */
#define BCM88732_DEVICE_ID      0x0732
#define BCM88732_A0_REV_ID      1
#define BCM88732_A1_REV_ID      2
#define BCM88732_A2_REV_ID      4
#define BCM88732_B0_REV_ID      0x11
#define BCM88732_B1_REV_ID      0x12
#define BCM88732_B2_REV_ID      0x13

/* Triumph 3 */
#define BCM56640_DEVICE_ID      0xb640
#define BCM56640_A0_REV_ID      1
#define BCM56640_A1_REV_ID      2
#define BCM56640_B0_REV_ID      0x11
#define BCM56643_DEVICE_ID      0xb643
#define BCM56643_A0_REV_ID      1
#define BCM56643_A1_REV_ID      2
#define BCM56643_B0_REV_ID      0x11
#define BCM56644_DEVICE_ID      0xb644
#define BCM56644_A0_REV_ID      1
#define BCM56644_A1_REV_ID      2
#define BCM56644_B0_REV_ID      0x11
#define BCM56648_DEVICE_ID      0xb648
#define BCM56648_A0_REV_ID      1
#define BCM56648_A1_REV_ID      2
#define BCM56648_B0_REV_ID      0x11
#define BCM56649_DEVICE_ID      0xb649
#define BCM56649_A0_REV_ID      1
#define BCM56649_A1_REV_ID      2
#define BCM56649_B0_REV_ID      0x11

/* Apollo 2 */
#define BCM56540_DEVICE_ID      0xb540
#define BCM56540_A0_REV_ID      1
#define BCM56540_A1_REV_ID      2
#define BCM56540_B0_REV_ID      0x11
#define BCM56541_DEVICE_ID      0xb541
#define BCM56541_A0_REV_ID      1
#define BCM56541_A1_REV_ID      2
#define BCM56541_B0_REV_ID      0x11
#define BCM56542_DEVICE_ID      0xb542
#define BCM56542_A0_REV_ID      1
#define BCM56542_A1_REV_ID      2
#define BCM56542_B0_REV_ID      0x11
#define BCM56543_DEVICE_ID      0xb543
#define BCM56543_A0_REV_ID      1
#define BCM56543_A1_REV_ID      2
#define BCM56543_B0_REV_ID      0x11
#define BCM56544_DEVICE_ID      0xb544
#define BCM56544_A0_REV_ID      1
#define BCM56544_A1_REV_ID      2
#define BCM56544_B0_REV_ID      0x11

/* Firebolt 4 */
#define BCM56545_DEVICE_ID      0xb545
#define BCM56545_A0_REV_ID      1
#define BCM56545_A1_REV_ID      2
#define BCM56545_B0_REV_ID      0x11
#define BCM56546_DEVICE_ID      0xb546
#define BCM56546_A0_REV_ID      1
#define BCM56546_A1_REV_ID      2
#define BCM56546_B0_REV_ID      0x11

/* Ranger plus */
#define BCM56044_DEVICE_ID      0xb044
#define BCM56044_B0_REV_ID      0x11
#define BCM56045_DEVICE_ID      0xb045
#define BCM56045_A0_REV_ID      1
#define BCM56045_A1_REV_ID      2
#define BCM56045_B0_REV_ID      0x11
#define BCM56046_DEVICE_ID      0xb046
#define BCM56046_A0_REV_ID      1
#define BCM56046_A1_REV_ID      2
#define BCM56046_B0_REV_ID      0x11


/* Katana */
#define BCM56440_DEVICE_ID      0xb440
#define BCM56440_A0_REV_ID      1
#define BCM56440_B0_REV_ID      0x11
#define BCM56441_DEVICE_ID      0xb441
#define BCM56441_A0_REV_ID      1
#define BCM56441_B0_REV_ID      0x11
#define BCM56442_DEVICE_ID      0xb442
#define BCM56442_A0_REV_ID      1
#define BCM56442_B0_REV_ID      0x11
#define BCM56443_DEVICE_ID      0xb443
#define BCM56443_A0_REV_ID      1
#define BCM56443_B0_REV_ID      0x11
#define BCM56445_DEVICE_ID      0xb445
#define BCM56445_A0_REV_ID      1
#define BCM56445_B0_REV_ID      0x11
#define BCM56446_DEVICE_ID      0xb446
#define BCM56446_A0_REV_ID      1
#define BCM56446_B0_REV_ID      0x11
#define BCM56447_DEVICE_ID      0xb447
#define BCM56447_A0_REV_ID      1
#define BCM56447_B0_REV_ID      0x11
#define BCM56448_DEVICE_ID      0xb448
#define BCM56448_A0_REV_ID      1
#define BCM56448_B0_REV_ID      0x11
#define BCM56449_DEVICE_ID      0xb449
#define BCM56449_A0_REV_ID      1
#define BCM56449_B0_REV_ID      0x11
#define BCM56240_DEVICE_ID      0xb240
#define BCM56240_A0_REV_ID      1
#define BCM56240_B0_REV_ID      0x11
#define BCM56241_DEVICE_ID      0xb241
#define BCM56241_A0_REV_ID      1
#define BCM56241_B0_REV_ID      0x11
#define BCM56242_DEVICE_ID      0xb242
#define BCM56242_A0_REV_ID      1
#define BCM56242_B0_REV_ID      0x11
#define BCM56243_DEVICE_ID      0xb243
#define BCM56243_A0_REV_ID      1
#define BCM56243_B0_REV_ID      0x11
#define BCM56245_DEVICE_ID      0xb245
#define BCM56245_A0_REV_ID      1
#define BCM56245_B0_REV_ID      0x11
#define BCM56246_DEVICE_ID      0xb246
#define BCM56246_A0_REV_ID      1
#define BCM56246_B0_REV_ID      0x11
#define BCM55440_DEVICE_ID      0xa440
#define BCM55440_A0_REV_ID      1
#define BCM55440_B0_REV_ID      0x11
#define BCM55441_DEVICE_ID      0xa441
#define BCM55441_A0_REV_ID      1
#define BCM55441_B0_REV_ID      0x11

/* Katana 2 */
#define BCM55450_DEVICE_ID      0xa450
#define BCM55450_A0_REV_ID      1
#define BCM55450_B0_REV_ID      0x11
#define BCM55455_DEVICE_ID      0xa455
#define BCM55455_A0_REV_ID      1
#define BCM55455_B0_REV_ID      0x11
#define BCM56248_DEVICE_ID      0xb248
#define BCM56248_A0_REV_ID      1
#define BCM56248_B0_REV_ID      0x11
#define BCM56450_DEVICE_ID      0xb450
#define BCM56450_A0_REV_ID      1
#define BCM56450_B0_REV_ID      0x11
#define BCM56452_DEVICE_ID      0xb452
#define BCM56452_A0_REV_ID      1
#define BCM56452_B0_REV_ID      0x11
#define BCM56454_DEVICE_ID      0xb454
#define BCM56454_A0_REV_ID      1
#define BCM56454_B0_REV_ID      0x11
#define BCM56455_DEVICE_ID      0xb455
#define BCM56455_A0_REV_ID      1
#define BCM56455_B0_REV_ID      0x11
#define BCM56456_DEVICE_ID      0xb456
#define BCM56456_A0_REV_ID      1
#define BCM56456_B0_REV_ID      0x11
#define BCM56457_DEVICE_ID      0xb457
#define BCM56457_A0_REV_ID      1
#define BCM56457_B0_REV_ID      0x11
#define BCM56458_DEVICE_ID      0xb458
#define BCM56458_A0_REV_ID      1
#define BCM56458_B0_REV_ID      0x11

/* Trident 2 */
#define BCM56850_DEVICE_ID      0xb850
#define BCM56850_A0_REV_ID      1
#define BCM56850_A1_REV_ID      2
#define BCM56850_A2_REV_ID      3
#define BCM56851_DEVICE_ID      0xb851
#define BCM56851_A0_REV_ID      1
#define BCM56851_A1_REV_ID      2
#define BCM56851_A2_REV_ID      3
#define BCM56852_DEVICE_ID      0xb852
#define BCM56852_A0_REV_ID      1
#define BCM56852_A1_REV_ID      2
#define BCM56852_A2_REV_ID      3
#define BCM56853_DEVICE_ID      0xb853
#define BCM56853_A0_REV_ID      1
#define BCM56853_A1_REV_ID      2
#define BCM56853_A2_REV_ID      3
#define BCM56854_DEVICE_ID      0xb854
#define BCM56854_A0_REV_ID      1
#define BCM56854_A1_REV_ID      2
#define BCM56854_A2_REV_ID      3
#define BCM56855_DEVICE_ID      0xb855
#define BCM56855_A0_REV_ID      1
#define BCM56855_A1_REV_ID      2
#define BCM56855_A2_REV_ID      3
#define BCM56834_DEVICE_ID      0xb834
#define BCM56834_A0_REV_ID      1
#define BCM56834_A1_REV_ID      2
#define BCM56834_A2_REV_ID      3

/* Titan 2 */
#define BCM56750_DEVICE_ID      0xb750
#define BCM56750_A0_REV_ID      1
#define BCM56750_A1_REV_ID      2
#define BCM56750_A2_REV_ID      3

/* Scorpion 960 */
#define BCM56830_DEVICE_ID      0xb830
#define BCM56830_A0_REV_ID      1
#define BCM56830_A1_REV_ID      2
#define BCM56830_A2_REV_ID      3

/* Hurricane 2*/
#define BCM56150_DEVICE_ID      0xb150
#define BCM56150_A0_REV_ID      1
#define BCM56151_DEVICE_ID      0xb151
#define BCM56151_A0_REV_ID      1
#define BCM56152_DEVICE_ID      0xb152
#define BCM56152_A0_REV_ID      1

/* Wolfhound*/
#define BCM53342_DEVICE_ID      0x8342
#define BCM53342_A0_REV_ID      1
#define BCM53343_DEVICE_ID      0x8343
#define BCM53343_A0_REV_ID      1
#define BCM53344_DEVICE_ID      0x8344
#define BCM53344_A0_REV_ID      1
#define BCM53346_DEVICE_ID      0x8346
#define BCM53346_A0_REV_ID      1
#define BCM53347_DEVICE_ID      0x8347
#define BCM53347_A0_REV_ID      1

/* Foxhound*/
#define BCM53333_DEVICE_ID      0x8333
#define BCM53333_A0_REV_ID      1
#define BCM53334_DEVICE_ID      0x8334
#define BCM53334_A0_REV_ID      1

/* Deerhound*/
#define BCM53393_DEVICE_ID      0x8393
#define BCM53393_A0_REV_ID      1
#define BCM53394_DEVICE_ID      0x8394
#define BCM53394_A0_REV_ID      1

/* Greyhound */
#define BCM53400_DEVICE_ID      0x8400 
#define BCM53400_A0_REV_ID      1
#define BCM56060_DEVICE_ID      0xb060  
#define BCM56060_A0_REV_ID      1
#define BCM56062_DEVICE_ID      0xb062
#define BCM56062_A0_REV_ID      1
#define BCM56063_DEVICE_ID      0xb063
#define BCM56063_A0_REV_ID      1
#define BCM56064_DEVICE_ID      0xb064
#define BCM56064_A0_REV_ID      1
#define BCM53401_DEVICE_ID      0x8401
#define BCM53411_DEVICE_ID      0x8411
#define BCM53401_A0_REV_ID      1
#define BCM53402_DEVICE_ID      0x8402
#define BCM53412_DEVICE_ID      0x8412
#define BCM53402_A0_REV_ID      1
#define BCM53403_DEVICE_ID      0x8403
#define BCM53413_DEVICE_ID      0x8413
#define BCM53403_A0_REV_ID      1
#define BCM53404_DEVICE_ID      0x8404
#define BCM53414_DEVICE_ID      0x8414
#define BCM53404_A0_REV_ID      1
#define BCM53405_DEVICE_ID      0x8405
#define BCM53415_DEVICE_ID      0x8415
#define BCM53405_A0_REV_ID      1
#define BCM53406_DEVICE_ID      0x8406
#define BCM53416_DEVICE_ID      0x8416
#define BCM53406_A0_REV_ID      1
#define BCM53408_DEVICE_ID      0x8408
#define BCM53418_DEVICE_ID      0x8418
#define BCM53408_A0_REV_ID      1



/*
 * BCM5665: Tucana48 (48+4+1)
 * BCM5665L: Tucana24 (24+4+1)
 * BCM5666: Tucana48 (48+4+1) without L3
 * BCM5666L: Tucana24 (24+4+1) without L3
 *
 * The device ID is 0x5665 for all of these parts.  For BCM5665L and
 * BCM5666L, the pbmp_valid property must be set to invalidate fe24-fe47
 * (see $SDK/rc/config.bcm).
 */
#define BCM5665_DEVICE_ID       0x5665
#define BCM5665_A0_REV_ID       1
#define BCM5665_B0_REV_ID       0x11


/*
 * BCM5655: Titanium48 (48+4)
 * BCM5656: Titanium48 (48+4) without L3
 *
 * The device ID is 0x5655 for both parts.
 */
#define BCM5655_DEVICE_ID       0x5655
#define BCM5655_A0_REV_ID       1
#define BCM5655_B0_REV_ID       0x11


/*
 * BCM5650: Titanium-II (24+4)
 * BCM5651: Titanium-II (24+4) without L3
 *
 * The device ID is 0x5650 for both parts.
 * BCM5650C0 is the first spin of a real 24+4 cost-reduced chip.
 */
#define BCM5650_DEVICE_ID       0x5650
#define BCM5650_A0_REV_ID       1
#define BCM5650_B0_REV_ID       0x11
#define BCM5650_C0_REV_ID       0x21

#define BROADCOM_PHYID_HIGH 0x0040

/* robo devices */
#define BCM5338_PHYID_LOW       0x62b0
#define BCM5338_A0_REV_ID       0
#define BCM5338_A1_REV_ID       1
#define BCM5338_B0_REV_ID       3 

#define BCM5324_PHYID_LOW       0xbc20
#define BCM5324_PHYID_HIGH      0x143
#define BCM5324_A1_PHYID_HIGH   0x153
#define BCM5324_DEVICE_ID       0xbc20
#define BCM5324_A0_REV_ID       0
#define BCM5324_A1_REV_ID       1
#define BCM5324_A2_REV_ID       2

#define BCM5380_PHYID_LOW       0x6250
#define BCM5380_A0_REV_ID       0

#define BCM5388_PHYID_LOW       0x6288
#define BCM5388_A0_REV_ID       0

#define BCM5396_PHYID_LOW       0xbd70
#define BCM5396_PHYID_HIGH      0x143
#define BCM5396_DEVICE_ID       0x96
#define BCM5396_A0_REV_ID       0

#define BCM5389_PHYID_LOW       0xbd70
#define BCM5389_PHYID_HIGH      0x143
#define BCM5389_DEVICE_ID       0x89
#define BCM5389_A0_REV_ID       0
#define BCM5389_A1_DEVICE_ID    0x86
#define BCM5389_A1_REV_ID       1

#define BCM5398_PHYID_LOW       0xbcd0
#define BCM5398_PHYID_HIGH      0x0143
#define BCM5398_DEVICE_ID       0x98
#define BCM5398_A0_REV_ID       0

#define BCM5325_PHYID_LOW       0xbc30
#define BCM5325_PHYID_HIGH      0x143
#define BCM5325_DEVICE_ID       0xbc30
#define BCM5325_A0_REV_ID       0
#define BCM5325_A1_REV_ID       1

#define BCM5348_PHYID_LOW   0xbe40
#define BCM5348_PHYID_HIGH      0x0143
#define BCM5348_DEVICE_ID       0x48
#define BCM5348_A0_REV_ID       0
#define BCM5348_A1_REV_ID       1

#define BCM5397_PHYID_LOW       0xbcd0
#define BCM5397_PHYID_HIGH      0x0143
#define BCM5397_DEVICE_ID       0x97
#define BCM5397_A0_REV_ID       0

#define BCM5347_PHYID_LOW       0xbe40
#define BCM5347_PHYID_HIGH      0x0143
#define BCM5347_DEVICE_ID       0x47
#define BCM5347_A0_REV_ID       0

#define BCM5395_PHYID_LOW       0xbcf0
#define BCM5395_PHYID_HIGH      0x0143
#define BCM5395_DEVICE_ID       0xbcf0
#define BCM5395_A0_REV_ID       0

#define BCM53242_PHYID_LOW      0xbf10
#define BCM53242_PHYID_HIGH     0x0143 
#define BCM53242_DEVICE_ID      0xbf10 
#define BCM53242_A0_REV_ID  0
#define BCM53242_B0_REV_ID  4
#define BCM53242_B1_REV_ID  5

#define BCM53262_PHYID_LOW      0xbf20
#define BCM53262_PHYID_HIGH     0x0143 
#define BCM53262_DEVICE_ID      0xbf20 
#define BCM53262_A0_REV_ID  0
#define BCM53262_B0_REV_ID  4
#define BCM53262_B1_REV_ID  5

#define BCM53115_PHYID_LOW       0xbf80
#define BCM53115_PHYID_HIGH      0x0143
#define BCM53115_DEVICE_ID       0xbf80  
#define BCM53115_A0_REV_ID       0
#define BCM53115_A1_REV_ID       1
/* 53115_b0 is a exception on rev_id 
 *  - Normally, the B0 id definition should be 4
 */
#define BCM53115_B0_REV_ID       2
#define BCM53115_B1_REV_ID       3
#define BCM53115_C0_REV_ID       8

#define BCM53118_PHYID_LOW       0xbfe0
#define BCM53118_PHYID_HIGH      0x0143
#define BCM53118_DEVICE_ID       0xbfe0  
#define BCM53118_A0_REV_ID       0

#define BCM53118_B0_REV_ID       4
#define BCM53118_B1_REV_ID       5

#define BCM53280_PHYID_LOW       0x5e90
#define BCM53280_PHYID_HIGH      0x0362
#define BCM53280_DEVICE_ID       (0x4 | BCM53280_PHYID_LOW)
#define BCM53280_A0_REV_ID       0
#define BCM53280_B0_REV_ID   0x4
#define BCM53280_B1_REV_ID   0x5
#define BCM53280_B2_REV_ID   0x6
#define BCM53286_DEVICE_ID       (0x4 | BCM53280_PHYID_LOW)
#define BCM53288_DEVICE_ID       (0xc | BCM53280_PHYID_LOW)
#define BCM53284_DEVICE_ID       (0x7 | BCM53280_PHYID_LOW)
#define BCM53283_DEVICE_ID       (0x6 | BCM53280_PHYID_LOW)
#define BCM53282_DEVICE_ID       (0x5 | BCM53280_PHYID_LOW)
#define BCM53101_PHYID_LOW       0x5ed0
#define BCM53101_PHYID_HIGH      0x0362
#define BCM53101_DEVICE_ID       0x5ed0
#define BCM53101_A0_REV_ID       0
#define BCM53101_B0_REV_ID       4

#define BCM53125_PHYID_LOW       0x5f20
#define BCM53125_PHYID_HIGH      0x0362
#define BCM53125_DEVICE_ID       0x5f20  
#define BCM53125_A0_REV_ID       0
#define BCM53125_B0_REV_ID       0x4
#define BCM53125_MODEL_ID       0x53125

#define BCM53128_PHYID_LOW       0x5e10
#define BCM53128_PHYID_HIGH      0x0362
#define BCM53128_DEVICE_ID       0x5e10  
#define BCM53128_A0_REV_ID       0
#define BCM53128_B0_REV_ID       0x4
#define BCM53128_MODEL_ID       0x53128

#define BCM53600_PHYID_LOW  0x5f40
#define BCM53600_PHYID_HIGH 0x0362
#define BCM53600_DEVICE_ID  (0x3 | BCM53600_PHYID_LOW)
#define BCM53600_A0_REV_ID  0
#define BCM53602_DEVICE_ID  (0x1 | BCM53600_PHYID_LOW)
#define BCM53603_DEVICE_ID  (0x2 | BCM53600_PHYID_LOW)
#define BCM53604_DEVICE_ID  (0x3 | BCM53600_PHYID_LOW)
#define BCM53606_DEVICE_ID  (0x7 | BCM53600_PHYID_LOW)

#define BCM89500_PHYID_LOW       0x5d30
#define BCM89500_PHYID_HIGH      0x0362
#define BCM89500_DEVICE_ID       0x9500
#define BCM89501_DEVICE_ID       0x9501 
#define BCM89200_DEVICE_ID       0x9200 
#define BCM89500_A0_REV_ID       0
#define BCM89500_B0_REV_ID       0x4
#define BCM89500_MODEL_ID       0x89500

#define BCM53010_PHYID_LOW       0x8760
#define BCM53010_PHYID_HIGH      0x600d
#define BCM53010_DEVICE_ID       0x3010
#define BCM53011_DEVICE_ID       0x3011
#define BCM53012_DEVICE_ID       0x3012
#define BCM53010_A0_REV_ID       0
#define BCM53010_A2_REV_ID       0x2
#define BCM53010_MODEL_ID        0x53010

#define BCM53018_PHYID_LOW       0x87c0
#define BCM53018_PHYID_HIGH      0x600d
#define BCM53017_DEVICE_ID       0x3016
#define BCM53018_DEVICE_ID       0x3018
#define BCM53019_DEVICE_ID       0x3019
#define BCM53018_A0_REV_ID       0
#define BCM53018_MODEL_ID        0x53016

#define BCM53020_PHYID_LOW       0x87f0
#define BCM53020_PHYID_HIGH      0x600d
#define BCM53020_DEVICE_ID       0x8022
#define BCM53022_DEVICE_ID       0x8022
#define BCM53023_DEVICE_ID       0x8023
#define BCM53025_DEVICE_ID       0x8025
#define BCM58625_DEVICE_ID       0x8625
#define BCM58622_DEVICE_ID       0x8622
#define BCM58623_DEVICE_ID       0x8623
#define BCM58525_DEVICE_ID       0x8525
#define BCM58522_DEVICE_ID       0x8522
#define BCM53020_A0_REV_ID       0
#define BCM53020_MODEL_ID        0x3025

/* (out of band) ethernet devices */
#define BCM4713_DEVICE_ID       0x4713
#define BCM4713_A0_REV_ID       0
#define BCM4713_A9_REV_ID       9

/* (out of band) ethernet devices */
#define BCM53000_GMAC_DEVICE_ID       0x4715
#define BCM53000_A0_REV_ID       0

/* (out of band) ethernet devices */
#define BCM53010_GMAC_DEVICE_ID       0x4715

/* CPU devices */
#define BCM53000PCIE_DEVICE_ID  0x5300

/* Define SBX device IDs */
#define SANDBURST_VENDOR_ID     0x17ba
#define BME3200_DEVICE_ID       0x0280
#define BME3200_A0_REV_ID       0x0000
#define BME3200_B0_REV_ID       0x0001
#define BM9600_DEVICE_ID        0x0480
#define BM9600_A0_REV_ID        0x0000
#define BM9600_B0_REV_ID        0x0010
#define QE2000_DEVICE_ID        0x0300
#define QE2000_A1_REV_ID        0x0001
#define QE2000_A2_REV_ID        0x0002
#define QE2000_A3_REV_ID        0x0003
#define QE2000_A4_REV_ID        0x0004
#define BCM88020_DEVICE_ID      0x0380
#define BCM88020_A0_REV_ID      0x0000
#define BCM88020_A1_REV_ID      0x0001
#define BCM88020_A2_REV_ID      0x0002
#define BCM88025_DEVICE_ID      0x0580
#define BCM88025_A0_REV_ID      0x0000
#define BCM88030_DEVICE_ID      0x0038
#define BCM88030_A0_REV_ID      0x0001
#define BCM88030_A1_REV_ID      0x0002
#define BCM88030_B0_REV_ID      0x0011
#define BCM88030_B1_REV_ID      0x0012
#define BCM88034_DEVICE_ID      0x0034
#define BCM88034_A0_REV_ID      (BCM88030_A0_REV_ID)
#define BCM88034_A1_REV_ID      (BCM88030_A1_REV_ID)
#define BCM88034_B0_REV_ID      (BCM88030_B0_REV_ID)
#define BCM88034_B1_REV_ID      (BCM88030_B1_REV_ID)
#define BCM88039_DEVICE_ID      0x0039
#define BCM88039_A0_REV_ID      (BCM88030_A0_REV_ID)
#define BCM88039_A1_REV_ID      (BCM88030_A1_REV_ID)
#define BCM88039_B0_REV_ID      (BCM88030_B0_REV_ID)
#define BCM88039_B1_REV_ID      (BCM88030_B1_REV_ID)
#define BCM88130_DEVICE_ID      0x0480
#define BCM88130_A0_REV_ID      0x0000
#define BCM88130_A1_REV_ID      0x0001
#define BCM88130_B0_REV_ID      0x0010
#define PLX_VENDOR_ID           0x10b5
#define PLX9656_DEVICE_ID       0x9656
#define PLX9656_REV_ID          0x0000
#define PLX9056_DEVICE_ID       0x9056
#define PLX9056_REV_ID          0x0000

/* Define EA device IDs */
#define TK371X_DEVICE_ID 0x8600
#define TK371X_A0_REV_ID 0x0

/* Define Dune device IDs */
#define PETRAB_DEVICE_ID        0xa100 /* 0xfa100 really */
#define PETRAB_A0_REV_ID        0x0001 
#define BCM88640_DEVICE_ID      (PETRAB_DEVICE_ID)
#define BCM88640_A0_REV_ID      (PETRAB_A0_REV_ID)

#define ARAD_DEVICE_ID          0x8650 
#define ARAD_A0_REV_ID          0x0000
#define ARAD_B0_REV_ID          0x0011  
#define ARAD_B1_REV_ID          0x0012
#define BCM88650_DEVICE_ID      ARAD_DEVICE_ID
#define BCM88650_A0_REV_ID      ARAD_A0_REV_ID
#define BCM88650_B0_REV_ID      ARAD_B0_REV_ID
#define BCM88650_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88750_DEVICE_ID      0x8750
#define BCM88750_A0_REV_ID      0x0000
#define BCM88750_B0_REV_ID      0x0011
#define BCM88754_DEVICE_ID      0x8754
#define BCM88754_A0_REV_ID      0x0000
#define BCM88754_ORIGINAL_VENDOR_ID      0x16FC
#define BCM88754_ORIGINAL_DEVICE_ID      0x020F
#define BCM88754_A0_ORIGINAL_REV_ID      0x0001
#define BCM88755_DEVICE_ID      0x8755
#define BCM88755_B0_REV_ID      0x0011
#define BCM88950_DEVICE_ID      0x8950
#define BCM88950_A0_REV_ID      0x0000
#define ARADPLUS_DEVICE_ID      0x8660
#define ARADPLUS_A0_REV_ID      0x0001 
#define BCM88660_DEVICE_ID      ARADPLUS_DEVICE_ID
#define BCM88660_A0_REV_ID      ARADPLUS_A0_REV_ID
#define JERICHO_DEVICE_ID       0x8670 
#define JERICHO_A0_REV_ID       0x0001
#define BCM88670_DEVICE_ID      JERICHO_DEVICE_ID
#define BCM88670_A0_REV_ID      JERICHO_A0_REV_ID
#define ARDON_DEVICE_ID         0x8202 
#define ARDON_A0_REV_ID         0x0000 
#define BCM88202_DEVICE_ID      ARDON_DEVICE_ID 
#define BCM88202_A0_REV_ID      ARDON_A0_REV_ID 
#define BCM2801PM_DEVICE_ID     0x2801
#define BCM2801PM_A0_REV_ID     0x0000
#define BCM88360_DEVICE_ID      0x8360
#define BCM88360_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88361_DEVICE_ID      0x8361
#define BCM88361_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88363_DEVICE_ID      0x8363
#define BCM88363_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88460_DEVICE_ID      0x8460
#define BCM88460_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88461_DEVICE_ID      0x8461
#define BCM88461_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88560_DEVICE_ID      0x8560
#define BCM88560_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88561_DEVICE_ID      0x8561
#define BCM88561_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88562_DEVICE_ID      0x8562
#define BCM88562_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88661_DEVICE_ID      0x8661
#define BCM88661_A0_REV_ID      ARADPLUS_A0_REV_ID
#define BCM88664_DEVICE_ID      0x8664
#define BCM88664_A0_REV_ID      ARADPLUS_A0_REV_ID


/* JERICHO-2-P3 */
#define JERICHO2_DEVICE_ID      0x8850
#define JERICHO2_P3_REV_ID      0x0001 
#define BCM88850_DEVICE_ID      JERICHO2_DEVICE_ID
#define BCM88850_P3_REV_ID      JERICHO2_P3_REV_ID

#define BCM88350_DEVICE_ID      0x8350
#define BCM88350_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88351_DEVICE_ID      0x8351
#define BCM88351_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88450_DEVICE_ID      0x8450
#define BCM88450_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88451_DEVICE_ID      0x8451
#define BCM88451_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88550_DEVICE_ID      0x8550
#define BCM88550_B1_REV_ID      ARAD_B0_REV_ID
#define BCM88551_DEVICE_ID      0x8551
#define BCM88551_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88552_DEVICE_ID      0x8552
#define BCM88552_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88651_DEVICE_ID      0x8651
#define BCM88651_B1_REV_ID      ARAD_B1_REV_ID
#define BCM88654_DEVICE_ID      0x8654
#define BCM88654_B1_REV_ID      ARAD_B1_REV_ID


#define PCP_PCI_VENDOR_ID 0x1172
#define PCP_PCI_DEVICE_ID 0x4

#define ACP_PCI_VENDOR_ID 0x10ee
#define ACP_PCI_DEVICE_ID 0x7011
#define ACP_PCI_REV_ID    0x0001
#endif  /* !_SOC_DEVIDS_H */

