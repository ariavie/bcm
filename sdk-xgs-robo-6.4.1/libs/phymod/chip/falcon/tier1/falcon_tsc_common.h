/********************************************************************************/
/********************************************************************************/
/*                                                                              */
/*  Revision      :  $Id: falcon_tsc_common.h 492 2014-05-09 23:03:03Z kirand $ */
/*                                                                              */
/*  Description   :  Defines and Enumerations required by Eagle/Merlin APIs     */
/*                                                                              */
/*  Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.               */
/*  All Rights Reserved                                                         */
/*  No portions of this material may be reproduced in any form without          */
/*  the written permission of:                                                  */
/*      Broadcom Corporation                                                    */
/*      5300 California Avenue                                                  */
/*      Irvine, CA  92617                                                       */
/*                                                                              */
/*  All information contained in this document is Broadcom Corporation          */
/*  company private proprietary, and trade secret.                              */
/*                                                                              */
/********************************************************************************/
/********************************************************************************/
/*
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

/** @file falcon_tsc_common.h
 * Defines and Enumerations shared across Eagle/Merlin/Falcon APIs BUT NOT MICROCODE
 */

#ifndef FALCON_TSC_API_COMMON_H
#define FALCON_TSC_API_COMMON_H

#define sign(x) ((x>=0) ? 1 : -1)

#define UCODE_MAX_SIZE  32768

/*
 * Register Address Defines used by the API that are different between IPs 
 */
#define DSC_A_DSC_UC_CTRL 0xd03d
#define DSC_E_RX_PI_CNT_BIN_D 0xd075
#define DSC_E_RX_PI_CNT_BIN_P 0xd076
#define DSC_E_RX_PI_CNT_BIN_L 0xd077
#define DSC_E_RX_PI_CNT_BIN_PD 0xd070
#define DSC_E_RX_PI_CNT_BIN_LD 0xd071
#define TLB_RX_PRBS_CHK_ERR_CNT_MSB_STATUS 0xD16A
#define TLB_RX_PRBS_CHK_ERR_CNT_LSB_STATUS 0xD16B

/*
 * Register Address Defines used by the API that are COMMON across IPs
 */
#define MDIO_MMDSEL_AER_COM_MDIO_MASKDATA  0xFFDB   

enum falcon_tsc_eye_scan_dir_enum {
      EYE_SCAN_HORIZ = 0,
      EYE_SCAN_VERTICAL = 1
};

#endif   
