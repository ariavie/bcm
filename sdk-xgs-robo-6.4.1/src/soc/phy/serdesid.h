/*
 * $Id: serdesid.h,v 1.3 Broadcom SDK $
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
 * File:        serdesid.h
 * Purpose:     Defines useful SerDes ID fields .
 */
#ifndef _PHY_SERDESID_H
#define _PHY_SERDESID_H

#define SERDES_ID0_REV_LETTER_A                      (0 << 14)
#define SERDES_ID0_REV_LETTER_B                      (1 << 14)
#define SERDES_ID0_REV_LETTER_C                      (2 << 14)
#define SERDES_ID0_REV_LETTER_D                      (3 << 14)

#define SERDES_ID0_REV_NUMBER_0                      (0 << 11)
#define SERDES_ID0_REV_NUMBER_1                      (1 << 11)
#define SERDES_ID0_REV_NUMBER_2                      (2 << 11)
#define SERDES_ID0_REV_NUMBER_3                      (3 << 11)
#define SERDES_ID0_REV_NUMBER_4                      (4 << 11)
#define SERDES_ID0_REV_NUMBER_5                      (5 << 11)
#define SERDES_ID0_REV_NUMBER_6                      (6 << 11)
#define SERDES_ID0_REV_NUMBER_7                      (7 << 11)

#define SERDES_ID0_BONDING_WIRE                      (0 << 9)
#define SERDES_ID0_BONDING_FLIP_CHIP                 (1 << 9)

#define SERDES_ID0_TECH_PROC_90NM                    (0 << 6)
#define SERDES_ID0_TECH_PROC_65NM                    (1 << 6)

#define SERDES_ID0_MODEL_NUMBER_SERDES_CL73          (0x00 << 0)
#define SERDES_ID0_MODEL_NUMBER_XGXS_16G             (0x01 << 0)
#define SERDES_ID0_MODEL_NUMBER_HYPERCORE            (0x02 << 0)
#define SERDES_ID0_MODEL_NUMBER_HYPERLITE            (0x03 << 0) 
#define SERDES_ID0_MODEL_NUMBER_PCIE                 (0x04 << 0)
#define SERDES_ID0_MODEL_NUMBER_1P25GBD_SERDES       (0x05 << 0)
#define SERDES_ID0_MODEL_NUMBER_XGXS_CL73_90NM       (0x1d << 0)
#define SERDES_ID0_MODEL_NUMBER_SERDES_CL73_90NM     (0x1e << 0)

#define SERDES_ID1_MULTIPLICITY_1              (1 << 12) 
#define SERDES_ID1_MULTIPLICITY_2              (2 << 12)
#define SERDES_ID1_MULTIPLICITY_3              (3 << 12)
#define SERDES_ID1_MULTIPLICITY_4              (4 << 12) 
#define SERDES_ID1_MULTIPLICITY_5              (5 << 12)
#define SERDES_ID1_MULTIPLICITY_6              (6 << 12)
#define SERDES_ID1_MULTIPLICITY_7              (7 << 12)
#define SERDES_ID1_MULTIPLICITY_8              (8 << 12)
#define SERDES_ID1_MULTIPLICITY_9              (9 << 12)
#define SERDES_ID1_MULTIPLICITY_10             (10 << 12)
#define SERDES_ID1_MULTIPLICITY_11             (11 << 12)
#define SERDES_ID1_MULTIPLICITY_12             (12 << 12)

#endif /* _PHY_SERDESID_H */

