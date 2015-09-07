/**********************************************************************************/
/**********************************************************************************/
/*                                                                                */
/*  Revision      :  $Id: falcon_api_uc_common.h 518 2014-05-21 17:26:16Z mageshv $  */
/*                                                                                */
/*  Description   :  Defines and Enumerations required by Falcon ucode            */
/*                                                                                */
/*  Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.                 */
/*  All Rights Reserved                                                             */
/*  No portions of this material may be reproduced in any form without              */
/*  the written permission of:                                                      */
/*      Broadcom Corporation                                                        */
/*      5300 California Avenue                                                      */
/*      Irvine, CA  92617                                                           */
/*                                                                                  */
/*  All information contained in this document is Broadcom Corporation              */
/*  company private proprietary, and trade secret.                                  */
/*                                                                                  */
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
 */

/** @file falcon_api_uc_common.h
 * Defines and Enumerations shared by Falcon IP Specific API and Microcode
 */

#ifndef FALCON_API_UC_COMMON_H
#define FALCON_API_UC_COMMON_H

/*-----------------------------*/
/*  Generic Serdes items first */
/*-----------------------------*/

/** DSC_STATES Enum */
enum falcon_tsc_dsc_state_enum {
  DSC_STATE_UNKNOWN      = 255,
  DSC_STATE_RESET        = 0,  
  DSC_STATE_RESTART      = 1,
  DSC_STATE_CONFIG       = 2,
  DSC_STATE_WAIT_FOR_SIG = 3,
  DSC_STATE_ACQ_CDR      = 4,
  DSC_STATE_CDR_SETTLE   = 5,
  DSC_STATE_HW_TUNE      = 6,
  DSC_STATE_UC_TUNE      = 7,
  DSC_STATE_MEASURE      = 8,
  DSC_STATE_DONE         = 9
};

/** SERDES_PMD_UC_COMMANDS Enum */
enum falcon_tsc_pmd_uc_cmd_enum {
  CMD_NULL                =  0,
  CMD_UC_CTRL             =  1,
  CMD_HEYE_OFFSET         =  2,
  CMD_VEYE_OFFSET         =  3,
  CMD_READ_DIE_TEMP       =  4,
  CMD_DIAG_EN             =  5,
  CMD_READ_UC_LANE_BYTE   =  6,
  CMD_WRITE_UC_LANE_BYTE  =  7,
  CMD_READ_UC_CORE_BYTE   =  8,
  CMD_WRITE_UC_CORE_BYTE  =  9,
  CMD_READ_UC_LANE_WORD   = 10,
  CMD_WRITE_UC_LANE_WORD  = 11,
  CMD_READ_UC_CORE_WORD   = 12,
  CMD_WRITE_UC_CORE_WORD  = 13,
  CMD_EVENT_LOG_CTRL      = 14,
  CMD_EVENT_LOG_READ      = 15,
  CMD_CAPTURE_BER_START   = 16,
  CMD_READ_DIAG_DATA_BYTE = 17,
  CMD_READ_DIAG_DATA_WORD = 18,
  CMD_CAPTURE_BER_END     = 19,
  CMD_CALC_CRC            = 20,
  CMD_FREEZE_STEADY_STATE = 21
};
/** SERDES_UC_CTRL_COMMANDS Enum */
enum falcon_tsc_pmd_uc_ctrl_cmd_enum {
  CMD_UC_CTRL_STOP_GRACEFULLY   = 0,
  CMD_UC_CTRL_STOP_IMMEDIATE    = 1,
  CMD_UC_CTRL_RESUME            = 2,
  CMD_UC_CTRL_STOP_HOLD         = 3,
  CMD_UC_CTRL_RELEASE_HOLD_ONCE = 4
};

/** SERDES_UC_DIAG_COMMANDS Enum */
enum falcon_tsc_pmd_uc_diag_cmd_enum {
  CMD_UC_DIAG_DISABLE         = 3,
  CMD_UC_DIAG_PASSIVE         = 1,
  CMD_UC_DIAG_DENSITY         = 2,
  CMD_UC_DIAG_START_VSCAN_EYE = 4,
  CMD_UC_DIAG_START_HSCAN_EYE = 5,
  CMD_UC_DIAG_GET_EYE_SAMPLE  = 6
};
	
/** SERDES_EVENT_LOG_READ Enum */
enum falcon_tsc_pmd_event_rd_cmd_enum {
  CMD_EVENT_LOG_READ_START      = 0,
  CMD_EVENT_LOG_READ_NEXT       = 1,
  CMD_EVENT_LOG_READ_DONE       = 2
};

/** Media type Enum */
enum falcon_tsc_media_type_enum {
  MEDIA_TYPE_PCB_TRACE_BACKPLANE = 0,
  MEDIA_TYPE_COPPER_CABLE        = 1,
  MEDIA_TYPE_OPTICS              = 2
};

/** DIAG_BER mode settings **/
enum falcon_tsc_diag_ber_mode_enum {
	DIAG_BER_POS  = 0,
	DIAG_BER_NEG  = 1,
	DIAG_BER_VERT = 0,
	DIAG_BER_HORZ = 1<<1,
	DIAG_BER_PASS = 0,
	DIAG_BER_INTR = 1<<2,
	DIAG_BER_P1_NARROW = 1<<3,
	DIAG_BER_P1_WIDE = 0,
    DIAG_BER_FAST = 1<<6

};

typedef uint8_t float8_t;

/* Add Falcon specific items below this */ 

/** OSR_MODES Enum */
enum falcon_tsc_osr_mode_enum {
  OSX1    = 0,
  OSX2    = 1,
  OSX4    = 2,
  OSX16P5 = 8,
  OSX20P625 = 12
};

/** CDR mode Enum **/
enum cdr_mode_enum {
  CDR_MODE_OS_ALL_EDGES         = 0,
  CDR_MODE_OS_PATTERN           = 1,
  CDR_MODE_OS_PATTERN_ENHANCED  = 2,  
  CDR_MODE_BR_PATTERN           = 3
};

/** VCO_RATE Enum */
enum falcon_tsc_vco_rate_enum {
  VCO_5P5G = 0,
  VCO_5P75G,
  VCO_6G,
  VCO_6P25G,
  VCO_6P5G,
  VCO_6P75G,
  VCO_7G,
  VCO_7P25G,
  VCO_7P5G,
  VCO_7P75G,
  VCO_8G,
  VCO_8P25G,
  VCO_8P5G,
  VCO_8P75G,
  VCO_9G,
  VCO_9P25G,
  VCO_9P5G,
  VCO_9P75G,
  VCO_10G,
  VCO_10P25G,
  VCO_10P5G,
  VCO_10P75G,
  VCO_11G,
  VCO_11P25G,
  VCO_11P5G,
  VCO_11P75G,
  VCO_12G,
  VCO_12P25G,
  VCO_12P5G,
  VCO_12P75G,
  VCO_13G,
  VCO_13P25G
};

/** Lane User Control Clause93/72 Force Value **/
enum cl93n72_frc_val_enum {
  CL93N72_FORCE_OS  = 0,
  CL93N72_FORCE_BR  = 1  
};


/** Core User Control Event Log Level **/
#define USR_CTRL_CORE_EVENT_LOG_UPTO_FULL (0xFF)

/*------------------------------*/
/** Serdes Event Code Structure */
/*------------------------------*/

#define event_code_t uint8_t
#define log_level_t  uint8_t

enum event_code_enum {       
	EVENT_CODE_UNKNOWN = 0,
	EVENT_CODE_ENTRY_TO_DSC_RESET,
	EVENT_CODE_RELEASE_USER_RESET,
	EVENT_CODE_EXIT_FROM_DSC_RESET,
	EVENT_CODE_ENTRY_TO_CORE_RESET,
	EVENT_CODE_RELEASE_USER_CORE_RESET,
	EVENT_CODE_ACTIVE_RESTART_CONDITION,
	EVENT_CODE_EXIT_FROM_RESTART,
	EVENT_CODE_WRITE_TR_COARSE_LOCK,
	EVENT_CODE_CL72_READY_FOR_COMMAND,
	EVENT_CODE_EACH_WRITE_TO_CL72_TX_CHANGE_REQUEST,
	EVENT_CODE_REMOTE_RX_READY,
	EVENT_CODE_LOCAL_RX_TRAINED,
	EVENT_CODE_DSC_LOCK,
	EVENT_CODE_FIRST_RX_PMD_LOCK,
	EVENT_CODE_PMD_RESTART_FROM_CL72_CMD_INTF_TIMEOUT,
	EVENT_CODE_LP_RX_READY,
	EVENT_CODE_STOP_EVENT_LOG,
	EVENT_CODE_GENERAL_EVENT_0,
	EVENT_CODE_GENERAL_EVENT_1,
	EVENT_CODE_GENERAL_EVENT_2,

	/* !!! add new event code above this line !!! */
	EVENT_CODE_MAX,
	EVENT_CODE_TIMESTAMP_WRAP_AROUND = 255
};

#endif
