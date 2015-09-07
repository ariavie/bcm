/**********************************************************************************/
/**********************************************************************************/
/*  File Name     :  eagle_tsc_err_code.h                                        */
/*  Created On    :  18/09/2013                                                   */
/*  Created By    :  Kiran Divakar                                                */
/*  Description   :  Header file with Error Code enums                            */
/*  Revision      :  $Id: eagle_tsc_err_code.h 406 2014-03-28 19:09:25Z kirand $ */
/*                                                                                */
/*  Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.                */
/*  All Rights Reserved                                                           */
/*  No portions of this material may be reproduced in any form without            */
/*  the written permission of:                                                    */
/*      Broadcom Corporation                                                      */
/*      5300 California Avenue                                                    */
/*      Irvine, CA  92617                                                         */
/*                                                                                */
/*  All information contained in this document is Broadcom Corporation            */
/*  company private proprietary, and trade secret.                                */
/*                                                                                */
/**********************************************************************************/
/**********************************************************************************/
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
 *  $Id: f3939ecc334d98dce7647bcbc678afe12e0d906b $
*/


/** @file eagle_tsc_err_code.h
 * Error Code enumerations
 */

#ifndef EAGLE_TSC_API_ERR_CODE_H
#define EAGLE_TSC_API_ERR_CODE_H

#define err_code_t uint16_t


/** ERROR CODE Enum */
enum err_code_enum {
  ERR_CODE_NONE = 0,
  ERR_CODE_INVALID_RAM_ADDR,
  ERR_CODE_SERDES_DELAY,
  ERR_CODE_POLLING_TIMEOUT,
  ERR_CODE_CFG_PATT_INVALID_PATTERN,
  ERR_CODE_CFG_PATT_INVALID_PATT_LENGTH,
  ERR_CODE_CFG_PATT_LEN_MISMATCH,
  ERR_CODE_CFG_PATT_PATTERN_BIGGER_THAN_MAXLEN,
  ERR_CODE_CFG_PATT_INVALID_HEX,
  ERR_CODE_CFG_PATT_INVALID_BIN2HEX,
  ERR_CODE_CFG_PATT_INVALID_SEQ_WRITE,
  ERR_CODE_PATT_GEN_INVALID_MODE_SEL,
  ERR_CODE_INVALID_UCODE_LEN,
  ERR_CODE_MICRO_INIT_NOT_DONE,
  ERR_CODE_UCODE_LOAD_FAIL,
  ERR_CODE_UCODE_VERIFY_FAIL,
  ERR_CODE_INVALID_TEMP_IDX,
  ERR_CODE_INVALID_PLL_CFG,
  ERR_CODE_TX_HPF_INVALID,
  ERR_CODE_VGA_INVALID,
  ERR_CODE_PF_INVALID,
  ERR_CODE_TX_AMP_CTRL_INVALID,
  ERR_CODE_INVALID_EVENT_LOG_WRITE,
  ERR_CODE_INVALID_EVENT_LOG_READ,
  ERR_CODE_UC_CMD_RETURN_ERROR,
  ERR_CODE_DATA_NOTAVAIL,
  ERR_CODE_BAD_PTR_OR_INVALID_INPUT,
  ERR_CODE_UC_NOT_STOPPED,
  ERR_CODE_UC_CRC_NOT_MATCH,
  ERR_CODE_TXFIR = 1 << 8,
  ERR_CODE_DFE_TAP = 2 << 8,
  ERR_CODE_DIAG = 3 << 8
};

/** TXFIR Error Codes Enum */
enum eagle_tsc_txfir_failcodes {  
  ERR_CODE_TXFIR_PRE_INVALID         = ERR_CODE_TXFIR + 1,
  ERR_CODE_TXFIR_MAIN_INVALID        = ERR_CODE_TXFIR + 2,
  ERR_CODE_TXFIR_POST1_INVALID       = ERR_CODE_TXFIR + 4,
  ERR_CODE_TXFIR_POST2_INVALID       = ERR_CODE_TXFIR + 8,
  ERR_CODE_TXFIR_POST3_INVALID       = ERR_CODE_TXFIR + 16,
  ERR_CODE_TXFIR_V2_LIMIT            = ERR_CODE_TXFIR + 32,
  ERR_CODE_TXFIR_SUM_LIMIT           = ERR_CODE_TXFIR + 64,
  ERR_CODE_TXFIR_PRE_POST1_SUM_LIMIT = ERR_CODE_TXFIR + 128
};

/** DFE Tap Error Codes Enum */
enum eagle_tsc_dfe_tap_failcodes {
  ERR_CODE_DFE1_INVALID = ERR_CODE_DFE_TAP + 1, 
  ERR_CODE_DFE2_INVALID = ERR_CODE_DFE_TAP + 2, 
  ERR_CODE_DFE3_INVALID = ERR_CODE_DFE_TAP + 4, 
  ERR_CODE_DFE4_INVALID = ERR_CODE_DFE_TAP + 8, 
  ERR_CODE_DFE5_INVALID = ERR_CODE_DFE_TAP + 16,
  ERR_CODE_DFE_TAP_IDX_INVALID = ERR_CODE_DFE_TAP + 32
};

enum eagle_tsc_diag_failcodes {
	ERR_CODE_DIAG_TIMEOUT = ERR_CODE_DIAG + 1,
	ERR_CODE_DIAG_INVALID_STATUS_RETURN = ERR_CODE_DIAG + 2,
	ERR_CODE_DIAG_SCAN_NOT_COMPLETE = ERR_CODE_DIAG + 3
};

#endif
