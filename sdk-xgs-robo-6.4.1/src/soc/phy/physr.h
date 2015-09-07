/*
 * $Id: physr.h,v 1.2 Broadcom SDK $
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
 * File:        physr.h
 * Purpose:     Basic defines for PHY specific data that gets pointed to 
 *              by phy_ctrl. The data gets allocated and freed outside the
 *              driver.
 * Note:        This header file is for PHY driver module. Should not include
 *              this header file in SOC or BCM layers.
 */

#ifndef _PHY_SR_H
#define _PHY_SR_H

/*
 * phy56xxx_5601x private data holds shadow registers
 */
typedef struct serdes_5601x_sregs {
    uint16      mii_ctrl;               /* 0*0  */
    uint16      mii_ana;                /* 0*4  */
    uint16      r1000x_ctrl1;           /* 0*10 */
    uint16      r1000x_ctrl2;           /* 0*11 */
    uint16      analog_tx;              /* 2*1b */
    uint16      digi3_ctrl;             /* 2*10 */
 } serdes_5601x_sregs_t;


/*
 * serdescombo_5601x private data holds shadow registers
 */
typedef struct serdescombo_5601x_sregs {
    uint16      mii_ctrl;               /* 0*0  */
    uint16      mii_ana;                /* 0*4  */
    uint16      r1000x_ctrl1;           /* 0*10 */
    uint16      r1000x_ctrl2;           /* 0*11 */
    uint16      analog_tx;              /* 2*1b */
    uint16      digi3_ctrl;             /* 2*10 */
    uint16      misc_misc2;             /* 5*1e */
    uint16      misc_tx_actrl3;         /* 5*17 */
} serdescombo_5601x_sregs_t;

/*
 * serdescombo65_5602x private data
 */
typedef struct serdescombo65_5602x_sregs {
    uint16      ieee0_mii_ctrl;         /* 0000ffe0 */
    uint16      ieee0_mii_ana;          /* 0000ffe4 */
    uint16      xgxs_blk0_ctrl;         /* 00008000 */
    uint16      tx_all_driver;          /* 000080a7 */
    uint16      over_1g_up1;            /* 00008329 */
    uint16      serdes_digital_ctrl1;   /* 00008300 */
    uint16      serdes_digital_misc1;   /* 00008308 */
    uint16      cl73_ieeeb0_an_ctrl;    /* 38000000 */
    uint16      cl73_ieeeb0_an_adv1;    /* 38000010 */
    uint16      cl73_ieeeb0_an_adv2;    /* 38000011 */
    uint16      cl73_ieeeb0_bamctrl1;   /* 00008372 */
    uint16      cl73_ieeeb0_bamctrl3;   /* 00008374 */
} serdescombo65_5602x_sregs_t;


#endif /* _PHY_SR_H */
