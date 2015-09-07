/*****************************************************************************************/
/*****************************************************************************************/
/*                                                                                       */
/*  Revision      :  $Id: falcon_tsc_debug_functions.h 503 2014-05-15 21:01:42Z kirand $ */
/*                                                                                       */
/*  Description   :  Functions used internally and available in debug shell only         */
/*                                                                                       */
/*  Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.                        */
/*  All Rights Reserved                                                                  */
/*  No portions of this material may be reproduced in any form without                   */
/*  the written permission of:                                                           */
/*      Broadcom Corporation                                                             */
/*      5300 California Avenue                                                           */
/*      Irvine, CA  92617                                                                */
/*                                                                                       */
/*  All information contained in this document is Broadcom Corporation                   */
/*  company private proprietary, and trade secret.                                       */
/*                                                                                       */
/*****************************************************************************************/
/*****************************************************************************************/
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

/** @file falcon_tsc_debug_functions.h
 * Functions used internally and available in debug shell only
 */

#ifndef FALCON_TSC_DEBUG_FUNCTIONS_H
#define FALCON_TSC_DEBUG_FUNCTIONS_H

#include "falcon_tsc_usr_includes.h"

#include "falcon_tsc_enum.h"
#include "falcon_tsc_common.h"
#include "falcon_api_uc_common.h"

/** Lane User Control Function Structure in Microcode */
struct falcon_tsc_usr_ctrl_func_st {
  uint16_t pf_adaptation           ;
  uint16_t pf2_adaptation          ;
  uint16_t dc_adaptation           ;
  uint16_t vga_adaptation          ;
  uint16_t slicer_voffset_tuning   ;
  uint16_t slicer_hoffset_tuning   ;
  uint16_t phase_offset_adaptation ;
  uint16_t eye_adaptation          ;
  uint16_t all_adaptation          ;
  uint16_t reserved                ;
};

/** Lane User DFE Control Function Structure in Microcode */
struct falcon_tsc_usr_ctrl_dfe_func_st {
  uint8_t dfe_tap1_adaptation      ;
  uint8_t dfe_fx_taps_adaptation   ;
  uint8_t dfe_fl_taps_adaptation   ;
  uint8_t dfe_dcd_adaptation       ;
};

/** Lane User Control Disable Function Struct */
struct falcon_tsc_usr_ctrl_disable_functions_st {
  struct falcon_tsc_usr_ctrl_func_st field;
  uint16_t word;
};

/** Lane User Control Disable DFE Function Struct */
struct falcon_tsc_usr_ctrl_disable_dfe_functions_st {
  struct falcon_tsc_usr_ctrl_dfe_func_st field;
  uint8_t  byte;                        
};

/** Falcon Lane status Struct */
struct falcon_tsc_detailed_lane_status_st {
    uint16_t tx_sts;
    uint8_t amp_ctrl;
    uint8_t pre_tap;
    uint8_t main_tap;
    uint8_t post1_tap;
    int8_t post2_tap;
    int8_t post3_tap;
    uint8_t sigdet;
    uint8_t pmd_lock;
    uint16_t dsc_sm[2];
    USR_DOUBLE ppm;
    uint8_t vga;
    uint8_t pf;
    uint8_t pf2;
    USR_DOUBLE main_tap_est;
    int8_t data_thresh;
    int8_t phase_thresh;
    int8_t lms_thresh;
    uint8_t ddq_hoffset;
    uint8_t ppq_hoffset;
    uint8_t llq_hoffset;
    uint8_t dp_hoffset;
    uint8_t dl_hoffset;
    int8_t dfe[26][4];
    int8_t dc_offset;
    int8_t thctrl_dp[4];
    int8_t thctrl_dn[4];
    int8_t thctrl_zp[4];
    int8_t thctrl_zn[4];
    int8_t thctrl_l[4];
    uint8_t prbs_chk_en;
    uint8_t prbs_chk_order;
    uint8_t prbs_chk_lock;
    uint32_t prbs_chk_errcnt;
};



/** Isolate Control pins.
 * Can be used for debug to avoid any interference from inputs coming through pins
 * @param pa phymod_access_t struct
 * @param enable Isolate pins enable (1 = Isolate pins; 0 = Pins not isolated)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_isolate_ctrl_pins( const phymod_access_t *pa, uint8_t enable);


/*-----------------------*/
/*  Stop/Resume uC Lane  */
/*-----------------------*/
/** Stop/Resume Micro operations on a Lane (Graceful Stop).
 * @param pa phymod_access_t struct
 * @param enable Enable micro lane stop (1 = Stop Micro opetarions on lane; 0 = Resume Micro operations on lane) 
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_stop_uc_lane( const phymod_access_t *pa, uint8_t enable);


/** Status of whether Micro is stopped on a lane.
 * @param pa phymod_access_t struct
 * @param uc_lane_stopped Micro lane stopped status returned by API
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_stop_uc_lane_status( const phymod_access_t *pa, uint8_t *uc_lane_stopped);


/** Write to lane user control disable startup function uC RAM variable.
 *  Note: This function should be used only during configuration under dp_reset
 * @param pa phymod_access_t struct
 * @param set_val Value to be written into lane user control disable startup function RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */ 
err_code_t falcon_tsc_set_usr_ctrl_disable_startup( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_functions_st set_val);


/** Write to lane user control disable startup dfe function uC RAM variable.
 *  Note: This function should be used only during configuration under dp_reset
 * @param pa phymod_access_t struct
 * @param set_val Value to be written into lane user control disable startup dfe function RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */ 
err_code_t falcon_tsc_set_usr_ctrl_disable_startup_dfe( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_dfe_functions_st set_val);


/** Write to lane user control disable steady-state function uC RAM variable.
 *  Note: This function should be used only during configuration under dp_reset
 * @param pa phymod_access_t struct
 * @param set_val Value to be written into lane user control disable  steady-state function RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */ 
err_code_t falcon_tsc_set_usr_ctrl_disable_steady_state( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_functions_st set_val);


/** Write to lane user control disable steady-state dfe function uC RAM variable.
 *  Note: This function should be used only during configuration under dp_reset
 * @param pa phymod_access_t struct
 * @param set_val Value to be written into lane user control disable  steady-state dfe function RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */ 
err_code_t falcon_tsc_set_usr_ctrl_disable_steady_state_dfe( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_dfe_functions_st set_val);


/** Read value of lane user control disable startup uC RAM variable
 * @param pa phymod_access_t struct
 * @param get_val Value read from lane user control disable startup RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */ 
err_code_t falcon_tsc_get_usr_ctrl_disable_startup( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_functions_st *get_val);


/** Read value of lane user control disable startup dfe uC RAM variable
 * @param pa phymod_access_t struct
 * @param get_val Value read from lane user control disable startup dfe RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_get_usr_ctrl_disable_startup_dfe( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_dfe_functions_st *get_val);


/** Read value of lane user control disable steady-state uC RAM variable
 * @param pa phymod_access_t struct
 * @param get_val Value read from lane user control disable steady-state RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_get_usr_ctrl_disable_steady_state( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_functions_st *get_val);


/** Read value of lane user control disable steady-state dfe uC RAM variable
 * @param pa phymod_access_t struct
 * @param get_val Value read from lane user control disable steady-state dfe RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_get_usr_ctrl_disable_steady_state_dfe( const phymod_access_t *pa, struct falcon_tsc_usr_ctrl_disable_dfe_functions_st *get_val);


/*-------------------------------------------*/
/*  Registers and Core uC RAM Variable Dump  */
/*-------------------------------------------*/
/** Display values of both Core level and (currently selected) Lane level Registers
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_reg_dump( const phymod_access_t *pa );


/** Display values of all Core uC RAM Variables (Core RAM Variable Dump)
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_uc_core_var_dump( const phymod_access_t *pa );


/*-----------------------------*/
/*  uC RAM Lane Variable Dump  */
/*-----------------------------*/
/** Display values of all Lane uC RAM Variables (Lane RAM Variable Dump)
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_uc_lane_var_dump( const phymod_access_t *pa );


/*--------------------------*/
/*  TX_PI Jitter Generation */
/*--------------------------*/

/** Generate TX_PI Sinusoidal or Spread-Spectrum (SSC) jitter
 * @param pa phymod_access_t struct
 * @param enable Enable/Disable jitter generation (1 = Enable; 0 = Disable)
 * @param freq_override_val Fixed Frequency Override value (-8192 to + 8192)
 * @param jit_type Jitter generation mode
 * @param tx_pi_jit_freq_idx Jitter generation frequency index (0 to 63) [see spec for more details]
 * @param tx_pi_jit_amp Jitter generation amplification factor (0 to 63) [max value of this register depends on tx_pi_jit_freq_idx and freq_override values]
 * @return Error Code generated by invalid TX_PI settings (returns ERR_CODE_NONE if no errors)
 */
err_code_t falcon_tsc_tx_pi_jitt_gen( const phymod_access_t *pa, uint8_t enable, int16_t freq_override_val, enum falcon_tsc_tx_pi_freq_jit_gen_enum jit_type, uint8_t tx_pi_jit_freq_idx, uint8_t tx_pi_jit_amp);


/** Read Serdes uC Event Logging.
 * Dump uC events from core memory
 * @param pa phymod_access_t struct
 * @param decode_enable Enable event logs decoding.
 *                      0 - display log in hex octets,
 *                      1 - display log in plain text,
 *                      2 - display log in both hex octects and plain text
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_read_event_log( const phymod_access_t *pa, uint8_t decode_enable);

/** Write to usr_ctrl_lane_event_log_level uC RAM variable.
 *  Note: This function should be used only during configuration under dp_reset
 * @param pa phymod_access_t struct
 * @param lane_event_log_level Value to be written into usr_ctrl_lane_event_log_level RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_set_usr_ctrl_lane_event_log_level( const phymod_access_t *pa, uint8_t lane_event_log_level);


/** Read value of usr_ctrl_lane_event_log_level uC RAM variable
 * @param pa phymod_access_t struct
 * @param lane_event_log_level Value read from usr_ctrl_lane_event_log_level RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_get_usr_ctrl_lane_event_log_level( const phymod_access_t *pa, uint8_t *lane_event_log_level);


/** Write to usr_ctrl_core_event_log_level uC RAM variable 
 * @param pa phymod_access_t struct
 * @param core_event_log_level Value to be written into the usr_ctrl_core_event_log_level RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_set_usr_ctrl_core_event_log_level( const phymod_access_t *pa, uint8_t core_event_log_level);

/** Read value of usr_ctrl_core_event_log_level uC RAM variable
 * @param pa phymod_access_t struct
 * @param core_event_log_level Value read from usr_ctrl_core_event_log_level RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_get_usr_ctrl_core_event_log_level( const phymod_access_t *pa, uint8_t *core_event_log_level);


/*---------------------------------------------*/
/*  Serdes IP RAM access - Lane RAM Variables  */
/*---------------------------------------------*/
/*          rd - read; wr - write              */ 
/*          b  - byte; w  - word               */
/*          l  - lane; c  - core               */
/*          s  - signed                        */
/*---------------------------------------------*/
/** Unsigned Byte Read of a uC RAM Lane variable.
 * Read access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM lane variable to be read
 * @return 8bit unsigned value read from uC RAM
 */ 
uint8_t    falcon_tsc_rdbl_uc_var( const phymod_access_t *pa, err_code_t *err_code, uint16_t addr);


/** Unsigned Word Read of a uC RAM Lane variable.
 * Read access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM lane variable to be read
 * @return 16bit unsigned value read from uC RAM
 */ 
uint16_t   falcon_tsc_rdwl_uc_var( const phymod_access_t *pa, err_code_t *err_code, uint16_t addr);


/** Unsigned Byte Write of a uC RAM Lane variable.
 * Write access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param addr Address of RAM lane variable to be written
 * @param wr_val 8bit unsigned value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrbl_uc_var( const phymod_access_t *pa, uint16_t addr, uint8_t wr_val);


/** Unsigned Word Write of a uC RAM Lane variable.
 * Write access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param addr Address of RAM lane variable to be written
 * @param wr_val 16bit unsigned value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrwl_uc_var( const phymod_access_t *pa, uint16_t addr, uint16_t wr_val);


/* Signed version of above 4 functions */

/** Signed Byte Read of a uC RAM Lane variable.
 * Read access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM lane variable to be read
 * @return 8bit signed value read from uC RAM
 */ 
int8_t     falcon_tsc_rdbls_uc_var( const phymod_access_t *pa, err_code_t *err_code, uint16_t addr);


/** Signed Word Read of a uC RAM Lane variable.
 * Read access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM lane variable to be read
 * @return 16bit signed value read from uC RAM
 */ 
int16_t    falcon_tsc_rdwls_uc_var( const phymod_access_t *pa, err_code_t *err_code, uint16_t addr);


/** Signed Byte Write of a uC RAM Lane variable.
 * Write access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param addr Address of RAM lane variable to be written
 * @param wr_val 8bit signed value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrbls_uc_var( const phymod_access_t *pa, uint16_t addr, int8_t wr_val);


/** Signed Word Write of a uC RAM Lane variable.
 * Write access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param addr Address of RAM lane variable to be written
 * @param wr_val 16bit signed value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrwls_uc_var( const phymod_access_t *pa, uint16_t addr, int16_t wr_val);


/*---------------------------------------------*/
/*  Serdes IP RAM access - Core RAM Variables  */
/*---------------------------------------------*/
/** Unsigned Byte Read of a uC RAM Core variable.
 * Read access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM core variable to be read
 * @return 8bit unsigned value read from uC RAM
 */ 
uint8_t    falcon_tsc_rdbc_uc_var( const phymod_access_t *pa, err_code_t *err_code, uint8_t addr);


/** Unsigned Word Read of a uC RAM Core variable.
 * Read access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM core variable to be read
 * @return 16bit unsigned value read from uC RAM
 */ 
uint16_t   falcon_tsc_rdwc_uc_var( const phymod_access_t *pa, err_code_t *err_code, uint8_t addr);


/** Unsigned Byte Write of a uC RAM Core variable.
 * Write access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param addr Address of RAM core variable to be written
 * @param wr_val 8bit unsigned value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrbc_uc_var( const phymod_access_t *pa, uint8_t addr, uint8_t wr_val);


/** Unsigned Word Write of a uC RAM Core variable.
 * Write access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param addr Address of RAM core variable to be written
 * @param wr_val 16bit unsigned value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrwc_uc_var( const phymod_access_t *pa, uint8_t addr, uint16_t wr_val);


/* Signed version of above 4 functions */

/** Signed Byte Read of a uC RAM Core variable.
 * Read access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM core variable to be read
 * @return 8bit signed value read from uC RAM
 */ 
int8_t     falcon_tsc_rdbcs_uc_var( const phymod_access_t *pa, err_code_t *err_code, uint8_t addr);


/** Signed Word Read of a uC RAM Core variable.
 * Read access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM core variable to be read
 * @return 16bit signed value read from uC RAM
 */ 
int16_t    falcon_tsc_rdwcs_uc_var( const phymod_access_t *pa, err_code_t *err_code, uint8_t addr);


/** Signed Byte Write of a uC RAM Core variable.
 * Write access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param addr Address of RAM core variable to be written
 * @param wr_val 8bit signed value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrbcs_uc_var( const phymod_access_t *pa, uint8_t addr, int8_t wr_val);


/** Signed Word Write of a uC RAM Core variable.
 * Write access through Micro Register Interface for PMD IPs; through Serdes FW DSC Command Interface for Gallardo
 * @param pa phymod_access_t struct
 * @param addr Address of RAM core variable to be written
 * @param wr_val 16bit signed value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrwcs_uc_var( const phymod_access_t *pa, uint8_t addr, int16_t wr_val);


/*---------------------------------------------------*/
/*  Micro Commands through uC DSC Command Interface  */
/*---------------------------------------------------*/
/** Issue a Micro command through the uC DSC Command Interface.
 * @param pa phymod_access_t struct
 * @param cmd Micro command to be issued
 * @param supp_info Supplement information for the Micro command to be issued (RAM read/write address or Micro Control command)
 * @param timeout_ms Time interval in milliseconds inside which the command should be completed; else error issued
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_pmd_uc_cmd( const phymod_access_t *pa, enum falcon_tsc_pmd_uc_cmd_enum cmd, uint8_t supp_info, uint32_t timeout_ms);                         


/** Issue a Micro command with data through the uC DSC Command Interface.
 * @param pa phymod_access_t struct
 * @param cmd Micro command to be issued
 * @param supp_info Supplement information for the Micro command to be issued (RAM write address)
 * @param data Data to be written to dsc_data for use by uC
 * @param timeout_ms Time interval in milliseconds inside which the command should be completed; else error issued
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_pmd_uc_cmd_with_data( const phymod_access_t *pa, enum falcon_tsc_pmd_uc_cmd_enum cmd, uint8_t supp_info, uint16_t data, uint32_t timeout_ms); 


/** Issue a Micro Control command through the uC DSC Command Interface.
 * @param pa phymod_access_t struct
 * @param control Micro Control command to be issued
 * @param timeout_ms Time interval in milliseconds inside which the command should be completed; else error issued
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_pmd_uc_control( const phymod_access_t *pa, enum falcon_tsc_pmd_uc_ctrl_cmd_enum control, uint32_t timeout_ms);                              

/** Issue a Micro Control command through the uC DSC DIAG_EN Command Interface.
 * @param pa phymod_access_t struct
 * @param control Micro DIAG Control command to be issued
 * @param timeout_ms Time interval in milliseconds inside which the command should be completed; else error issued
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_pmd_uc_diag_cmd( const phymod_access_t *pa, enum falcon_tsc_pmd_uc_diag_cmd_enum control, uint32_t timeout_ms);

/** Writes Serdes TXFIR tap settings.
 * Returns failcodes if TXFIR settings are invalid
 * @param pa phymod_access_t struct
 * @param pre   TXFIR pre tap value (0..31)
 * @param main  TXFIR main tap value (0..112)
 * @param post1 TXFIR post tap value (0..63)
 * @param post2 TXFIR post2 tap value (-15..15)
 * @param post3 TXFIR post3 tap value (-15..15)
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_apply_txfir_cfg( const phymod_access_t *pa, int8_t pre, int8_t main, int8_t post1, int8_t post2, int8_t post3);

/** Reads current pmd lane status and populates the provided structure of type falcon_tsc_detailed_lane_status_st
 * @param pa phymod_access_t struct
 * @param *lane_st All detailed lane info read and populated into this structure
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_log_full_pmd_state ( const phymod_access_t *pa, struct falcon_tsc_detailed_lane_status_st *lane_st);

/** Displays the lane status stored in the input struct of type falcon_tsc_detailed_lane_status_st
 * @param pa phymod_access_t struct
 * @param *lane_st  Lane struct to be diplayed
 * @param num_lanes Number of lanes
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_disp_full_pmd_state ( const phymod_access_t *pa, struct falcon_tsc_detailed_lane_status_st *lane_st, uint8_t num_lanes);

/*-----------------------------------------------*/
/*  RAM access through Micro Register Interface  */
/*-----------------------------------------------*/
/*           rd - read; wr - write               */ 
/*           b  - byte; w  - word                */
/*           l  - lane; c  - core                */
/*           s  - signed                         */
/*-----------------------------------------------*/

/** Unsigned Word Write of a uC RAM variable through Micro Register Interface.
 * @param pa phymod_access_t struct
 * @param addr Address of RAM variable to be written
 * @param wr_val 16bit unsigned value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrw_uc_ram( const phymod_access_t *pa, uint16_t addr, uint16_t wr_val);


/** Unsigned Byte Write of a uC RAM variable through Micro Register Interface.
 * @param pa phymod_access_t struct
 * @param addr Address of RAM variable to be written
 * @param wr_val 8bit unsigned value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrb_uc_ram( const phymod_access_t *pa, uint16_t addr, uint8_t wr_val);


/** Unigned Word Read of a uC RAM variable through Micro Register Interface.
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM variable to be read
 * @return 16bit unsigned value read from uC RAM
 */ 
uint16_t   falcon_tsc_rdw_uc_ram( const phymod_access_t *pa, err_code_t *err_code, uint16_t addr);


/** Unigned Byte Read of a uC RAM variable through Micro Register Interface.
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM variable to be read
 * @return 8bit unsigned value read from uC RAM
 */ 
uint8_t    falcon_tsc_rdb_uc_ram( const phymod_access_t *pa, err_code_t *err_code, uint16_t addr);


/* Signed versions of above 4 functions */

/** Signed Word Write of a uC RAM variable through Micro Register Interface.
 * @param pa phymod_access_t struct
 * @param addr Address of RAM variable to be written
 * @param wr_val 16bit signed value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrws_uc_ram( const phymod_access_t *pa, uint16_t addr, int16_t wr_val);


/** Signed Byte Write of a uC RAM variable through Micro Register Interface.
 * @param pa phymod_access_t struct
 * @param addr Address of RAM variable to be written
 * @param wr_val 8bit signed value to be written to RAM variable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_wrbs_uc_ram( const phymod_access_t *pa, uint16_t addr, int8_t wr_val);


/** Signed Word Read of a uC RAM variable through Micro Register Interface.
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM variable to be read
 * @return 16bit signed value read from uC RAM
 */ 
int16_t    falcon_tsc_rdws_uc_ram( const phymod_access_t *pa, err_code_t *err_code, uint16_t addr);


/** Signed Byte Read of a uC RAM variable through Micro Register Interface.
 * @param pa phymod_access_t struct
 * @param err_code Error Code generated by API (returns ERR_CODE_NONE if no errors)
 * @param addr Address of RAM variable to be read
 * @return 8bit signed value read from uC RAM
 */ 
int8_t     falcon_tsc_rdbs_uc_ram( const phymod_access_t *pa, err_code_t *err_code, uint16_t addr);

#endif
