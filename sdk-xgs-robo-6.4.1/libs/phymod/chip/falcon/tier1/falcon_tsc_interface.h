/***********************************************************************************/
/***********************************************************************************/
/*                                                                                 */
/*  Revision    :  $Id: falcon_tsc_interface.h 503 2014-05-15 21:01:42Z kirand $ */
/*                                                                                 */
/*  Description :  Interface functions targeted to IP user                         */
/*                                                                                 */
/*  Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.                  */
/*  All Rights Reserved                                                            */
/*  No portions of this material may be reproduced in any form without             */
/*  the written permission of:                                                     */
/*      Broadcom Corporation                                                       */
/*      5300 California Avenue                                                     */
/*      Irvine, CA  92617                                                          */
/*                                                                                 */
/*  All information contained in this document is Broadcom Corporation             */
/*  company private proprietary, and trade secret.                                 */
/*                                                                                 */
/***********************************************************************************/
/***********************************************************************************/
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

/*! \page Serdes API documentation
 *
 * \section intro_sec Introduction
 *
 * This documentation files are intended to guide a developer in using the 
 * Broadcom SerDes core within a larger ecosystem by providing specific API
 * function calls which can be used to perform all necessary operations.
 *
 *
 * \section API_sec API
 *
 * falcon_tsc_interface.h - provides the primary functionality.
 *
 * falcon_tsc_dependencies.h - defines the functions to be provided by the IP user
 *
 *
 * Copyright 2013 Broadcom Corporation all rights reserved
 */

/** @file falcon_tsc_interface.h
 * Main interface functions provided to IP User
 */

#ifndef FALCON_TSC_API_INTERFACE_H
#define FALCON_TSC_API_INTERFACE_H

#include "falcon_tsc_usr_includes.h"

#include "falcon_tsc_ipconfig.h"
#include "falcon_tsc_enum.h"
#include "falcon_tsc_err_code.h"


/*----------------------------------------*/
/*  Lane/Core structs (without bitfields) */
/*----------------------------------------*/

/** Lane Config Variable Structure in Microcode */
struct falcon_tsc_uc_lane_config_field_st {
  uint8_t  lane_cfg_from_pcs ;
  uint8_t  an_enabled        ;
  uint8_t  dfe_on            ;
  uint8_t  force_brdfe_on    ;
  uint8_t  media_type        ;
  uint8_t  unreliable_los    ;
  uint8_t  scrambling_dis    ;
  uint8_t  cl72_emulation_en ;
  uint8_t  reserved          ;
};

/** Core Config Variable Structure in Microcode */
struct falcon_tsc_uc_core_config_field_st {
  uint8_t  vco_rate          ;
  uint8_t  core_cfg_from_pcs ;
  uint8_t  reserved          ;
};

/** Lane Config Struct */
struct  falcon_tsc_uc_lane_config_st {
  struct falcon_tsc_uc_lane_config_field_st field;  
  uint16_t word;                       
                  
}; 

/** Core Config Struct */
struct  falcon_tsc_uc_core_config_st {
  struct falcon_tsc_uc_core_config_field_st field; 
  uint16_t word;         
};

/** Eyescan Options Struct */
struct falcon_tsc_eyescan_options_st {
   uint32_t linerate_in_khz;
   uint16_t timeout_in_milliseconds;
   int8_t horz_max;
   int8_t horz_min;
   int8_t hstep;
   int8_t vert_max;
   int8_t vert_min;
   int8_t vstep;
   int8_t mode;
};

/****************************************************/
/*  CORE Based APIs - Required to be used per Core  */
/****************************************************/
/* Returns API Version Number */
/** API Version Number.
 * @param pa phymod_access_t struct
 * @param *api_version API Version Number returned by the function
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_version( const phymod_access_t *pa, uint32_t *api_version);

/*------------------------------------------------------*/
/*  APIs to Read/Write Core Config variables in uC RAM  */
/*------------------------------------------------------*/
/** Write to core_config uC RAM variable.
 * @param pa phymod_access_t struct
 * @param struct_val Value to be written into core_config RAM variable. 
          Note that struct_val.word must be = 0, only the fields are used
 * @param pa phymod_access_t struct
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_set_uc_core_config( const phymod_access_t *pa, struct falcon_tsc_uc_core_config_st struct_val);

/** Read value of core_config uC RAM variable
 * @param pa phymod_access_t struct
 * @param *struct_val Value to be written into core_config RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_get_uc_core_config( const phymod_access_t *pa, struct falcon_tsc_uc_core_config_st *struct_val);

/*----------------------------------------*/
/*  Display Core Config and Debug Status  */
/*----------------------------------------*/
/** Display Core configurations (RAM config variables and config register fields)
 * @param pa phymod_access_t struct
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_core_config( const phymod_access_t *pa );


/** Display current Core state. Read and displays core status variables and fields
 * @param pa phymod_access_t struct
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_core_state( const phymod_access_t *pa );



/*-----------------------------------*/
/*  Microcode Load/Verify Functions  */
/*-----------------------------------*/

/** Load Microcode into Micro through Register (MDIO) Interface.
 * Once the microcode is loaded, de-assert reset to 8051 to start executing microcode "wrc_micro_mdio_dw8051_reset_n(0x1)"
 * @param pa phymod_access_t struct
 * @param *ucode_image pointer to the Microcode image organized in bytes
 * @param ucode_len Length of Microcode Image (number of bytes)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_ucode_mdio_load( const phymod_access_t *pa, uint8_t *ucode_image, uint16_t ucode_len);


/** To verify the Microcode image loaded in the Micro.
 * Read back the microcode from Micro and check against expected microcode image.
 * @param pa phymod_access_t struct
 * @param *ucode_image pointer to the expeted Microcode image organized in bytes
 * @param ucode_len Length of Microcode Image (number of bytes)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_ucode_load_verify( const phymod_access_t *pa, uint8_t *ucode_image, uint16_t ucode_len);

/** To verify the Microcode CRC loaded in the Micro.
 * Instruct uC to read image and calculate CRC and check against expected CRC.
 * @param pa phymod_access_t struct
 * @param ucode_len Length of Microcode Image (number of bytes)
 * @param expected_crc_value to verify
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_ucode_crc_verify( const phymod_access_t *pa, uint16_t ucode_len,uint16_t expected_crc_value);



/*------------------------------*/
/*  Shared TX Pattern Generator */
/*------------------------------*/
/** Configure Shared TX Pattern function. 
 * An input string (hex or binary) and pattern length are taken in as inputs, based on which the Pattern Generator registers 
 * are programmed to the required values to generate that pattern. 
 * API returns the fixed pattern generator mode select which needs to be passed to the falcon_tsc_tx_shared_patt_gen_en() function
 * (along with enable = 1) to enable the Pattern generator for that particular lane.
 * @param pa phymod_access_t struct
 * @param patt_length Pattern length 
 * @param pattern Input Pattern - Can be in hex (eg: "0xB055") or in binary (eg: "011011")
 * @return Error Code generated by invalid input pattern or pattern length (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_config_shared_tx_pattern( const phymod_access_t *pa, uint8_t patt_length, const char pattern[]);


/*-----------------------*/
/*  IDDQ / Clkgate APIs  */
/*-----------------------*/
/** Core configurations for IDDQ.
 * User needs to configure all lanes through falcon_tsc_lane_config_for_iddq() before enabling IDDQ by asserting IDDQ pin.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_core_config_for_iddq( const phymod_access_t *pa );


/** Serdes Core Powerdown.
 * Along with falcon_tsc_core_pwrdn(), all lanes powerdowns should also be issued using falcon_tsc_lane_pwrdn() to complete a Core Powerdown 
 * @param pa phymod_access_t struct
 * @param mode based on enum core_pwrdn_mode_enum select from ON, CORE, DEEP power down modes
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_core_pwrdn( const phymod_access_t *pa, enum core_pwrdn_mode_enum mode);


/**************************************************/
/* MISC reg access                                */
/**************************************************/

/** Masked Register Write to the currently selected Serdes IP core/lane through MDIO.
 * If using MDIO interface to access the registers, use this function to implement falcon_tsc_pmd_mwr_reg(..)
 * @param pa phymod_access_t struct
 * @param addr Address of register to be written
 * @param mask 16-bit mask indicating the position of the field with bits of 1s
 * @param lsb  LSB of the field
 * @param val  16bit value to be written
 */
err_code_t falcon_tsc_pmd_mdio_mwr_reg( const phymod_access_t *pa, uint16_t addr, uint16_t mask, uint8_t lsb, uint16_t val);


/**************************************************/
/* LANE Based APIs - Required to be used per Lane */
/**************************************************/

/*------------------------------------------------------------*/
/*  APIs to Write Lane Config and User variables into uC RAM  */
/*------------------------------------------------------------*/
/** Write to lane_config uC RAM variable.
 *  Note: This function should be used only during configuration under dp_reset
 * @param pa phymod_access_t struct
 * @param struct_val Value to be written into lane_config RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_set_uc_lane_cfg( const phymod_access_t *pa, struct falcon_tsc_uc_lane_config_st struct_val);


/*-----------------------------------------------------------*/
/*  APIs to Read Lane Config and User variables from uC RAM  */
/*-----------------------------------------------------------*/
/** Read value of lane_config uC RAM variable
 * @param pa phymod_access_t struct
 * @param *struct_val Value read from lane_config RAM variable
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_get_uc_lane_cfg( const phymod_access_t *pa, struct falcon_tsc_uc_lane_config_st *struct_val);


/*-----------------*/
/*  Configure PLL  */
/*-----------------*/
/** Configure PLL.
 * Configures PLL registers to obtain the required VCO frequency
 * @param pa phymod_access_t struct
 * @param pll_cfg Required PLL configuration
 * @return Error Code, if generated (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_configure_pll( const phymod_access_t *pa, enum falcon_tsc_pll_enum pll_cfg); 


/*-------------------------*/
/*  Merlin TX Analog APIs  */
/*-------------------------*/


/*-------------------------------*/
/*  Falcon/Eagle TX Analog APIs  */
/*-------------------------------*/

/** Write TX AFE parameters
 * @param pa phymod_access_t struct
 * @param param selects the parameter to write based on falcon_tsc_tx_afe_settings_enum
 * @param val is the signed input value to the parameter
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_write_tx_afe( const phymod_access_t *pa, enum falcon_tsc_tx_afe_settings_enum param, int8_t val);

/** Read TX AFE parameters
 * @param pa phymod_access_t struct
 * @param param selects the parameter to write based on falcon_tsc_tx_afe_settings_enum
 * @param *val is the returned signed value of the parameter
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_read_tx_afe( const phymod_access_t *pa, enum falcon_tsc_tx_afe_settings_enum param, int8_t *val);


/** Validates Eagle TXFIR tap settings.
 * Returns failcodes if TXFIR settings are invalid
 * @param pa phymod_access_t struct
 * @param pre   TXFIR pre tap value (0..31)
 * @param main  TXFIR main tap value (0..112)
 * @param post1 TXFIR post tap value (0..63)
 * @param post2 TXFIR post2 tap value (-15..15)
 * @param post3 TXFIR post3 tap value (-15..15)
 * @return Error Code generated by invalid tap settings (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_validate_txfir_cfg( const phymod_access_t *pa, int8_t pre, int8_t main, int8_t post1, int8_t post2, int8_t post3);


/*---------------------------------*/
/* Display Eye Scan / Eye Density  */
/*---------------------------------*/

/** Displays Passive Eye Scan from -0.5 UI to 0.5UI to BER 1e-7.
 *  Function uses uC to acquire data.
 *  it also retrieves the data and displays it in ASCII-art style
 * 
 * This function retrieves the from uC in horizontal stripe fashion
 *
 * @param pa phymod_access_t struct
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_eye_scan( const phymod_access_t *pa );

/** Displays Passive Eye Scan Data.
 * This function can be used to display data from meas_lowber_eye_scan() function.
 *  
 *  eyescan_options.horz_max = 31;
 *  eyescan_options.horz_min = -31;
 *  eyescan_options.vert_max = 31;
 *  eyescan_options.vert_min = -31;
 *  eyescan_options.hstep = 1;
 *  eyescan_options.vstep = 1;
 *  eyescan_options.timeout_in_milliseconds between 4 and 400000
 *     larger numbers will greatly increase test time!
 * @param pa phymod_access_t struct
 * @param eyescan_options is structure of options which control min,max,step, time, and linerate
 * @param *buffer is pointer to array which contains all samples.
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_lowber_eye( const phymod_access_t *pa, const struct falcon_tsc_eyescan_options_st eyescan_options, uint32_t *buffer);

/** Measure Passive Eye Scan based on eyescan_options provided and return data in buffer.
 * It relies on the accuracy of falcon_tsc_delay_us() function and recommend:
 *  eyescan_options.horz_max = 31;
 *  eyescan_options.horz_min = -31;
 *  eyescan_options.vert_max = 31;
 *  eyescan_options.vert_min = -31;
 *  eyescan_options.hstep = 1;
 *  eyescan_options.vstep = 1;
 *  eyescan_options.timeout_in_milliseconds between 4 and 400000
 *     larger numbers will greatly increase test time!
 * @param pa phymod_access_t struct
 * @param eyescan_options is structure of options which control min,max,step, time, and linerate
 * @param *buffer is pointer to array which is large enough to store all samples.
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_meas_lowber_eye( const phymod_access_t *pa, const struct falcon_tsc_eyescan_options_st eyescan_options, uint32_t *buffer);

/** Measure Eye Density based on eyescan_options provided and return data in buffer.
 * Due to sensitivity of the eye density test only internal trnsum timing can be used
 * Therefore timeout_in_milliseconds has no affect
 * recommend:
 *  eyescan_options.horz_max = 31;
 *  eyescan_options.horz_min = -31;
 *  eyescan_options.vert_max = 31;
 *  eyescan_options.vert_min = -31;
 *  eyescan_options.hstep = 1;
 *  eyescan_options.vstep = 1;
 *  eyescan_options.timeout_in_milliseconds - not used for density measurements!
 * @param pa phymod_access_t struct
 * @param eyescan_options is structure of options which control min,max,step, time, and linerate
 * @param *buffer is pointer to array which is large enough to store all samples.
 * @param *buffer_size returns an unsigned integer indicating number of elements used in array
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_meas_eye_density_data(const struct falcon_tsc_eyescan_options_st eyescan_options, int32_t *buffer,uint16_t *buffer_size);

/** Displays Eye Density Data measured with falcon_tsc_meas_eye_density_data()
 *  
 *  eyescan_options.horz_max = 31;
 *  eyescan_options.horz_min = -31;
 *  eyescan_options.vert_max = 31;
 *  eyescan_options.vert_min = -31;
 *  eyescan_options.hstep = 1;
 *  eyescan_options.vstep = 1;
 *  eyescan_options.timeout_in_milliseconds - Not used for Density measurements
 * 
 * @param pa phymod_access_t struct
 * @param eyescan_options is structure of options which control min,max,step, time, and linerate
 * @param *buffer is pointer to array which contains all samples.
 * @param buffer_size must provide buffer_size which is returned from falcon_tsc_meas_eye_density_data()
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_eye_density_data(const struct falcon_tsc_eyescan_options_st eyescan_options, int32_t *buffer,uint16_t buffer_size);

/** Start uC controller eye scan Function
 * which will provide a stripe of data at time either vertical or horizontal
 * this function only initiates the processor actions.  user must use read_eye_scan_stripe function
 * to get the data from uC
 * @param pa phymod_access_t struct
 * @param direction specifies either EYE_SCAN_VERTICAL or EYE_SCAN_HORIZ striping
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_meas_eye_scan_start( const phymod_access_t *pa, uint8_t direction);

/** Read Stripe of eye scan data from uC
 * @param pa phymod_access_t struct
 * @param *buffer must be of size 64
 * @param *status returns a status word
 *    bit 15 - indicates the ey scan is complete
 *    bit 14 - indicates uC is slower than read access
 *    bit 13 - indicates uC is faster than read access
 *    bit 12-8 - reserved
 *    bit 7-0 - indicates amount of data in the uC buffer
 *
 * @param pa phymod_access_t struct
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_read_eye_scan_stripe( const phymod_access_t *pa, uint32_t *buffer,uint16_t *status);

/** Display Stripe of eye scan data to stdout and log
 * @param pa phymod_access_t struct
 * @param y is the vertical step -31 to 31
 * @param *buffer must be of size 64
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_eye_scan_stripe( const phymod_access_t *pa, int8_t y,uint32_t *buffer);

/** Display Eye scan header to stdout and log
 * @param pa phymod_access_t struct
 * @param i indicates the number of headers to display for parallel eye scan
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_eye_scan_header( const phymod_access_t *pa, int8_t i);

/** Display Eye scan footer to stdout and log
 * @param pa phymod_access_t struct
 * @param i indicates the number of footers to display for parallel eye scan
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_eye_scan_footer( const phymod_access_t *pa, int8_t i);

/** Check status of eye scan operation in uC 
 * @param pa phymod_access_t struct
 * @param *status returns a status word
 *    bit 15 - indicates the eye scan is complete
 *    bit 14 - indicates uC is slower than read access
 *    bit 13 - indicates uC is faster than read access
 *    bit 12-8  reserved
 *    bit 7-0 - indicates amount of data in the uC buffer
 *
 * @param pa phymod_access_t struct
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors)
 */
err_code_t falcon_tsc_read_eye_scan_status( const phymod_access_t *pa, uint16_t *status);

/** Restores uC after running diagnostic eye scans
 * @param pa phymod_access_t struct
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_meas_eye_scan_done( const phymod_access_t *pa );

/** Start a uC controlled BER scan function
 * This will tell the uC to make a number of BER measurements at different offsets
 * and provide data back to API as a block of data. Several types of tests can be
 * made including Passive which can be run on any data pattern and does not affect 
 * datapath or Intrusive which can be run only when PRBS pattern is being used 
 * and will cause errors to occur.  Intrusive test has a limited vertical range!
 *
 * @param pa phymod_access_t struct
 * @param ber_scan_mode configures the type of test(use enum ber_scan_mode_enum)
 * bit 7 : reserved
 * bit 6 : 1 = BER FAST scan mode (reduce minimum sample time from 0.1sec to 0.02sec
 * bit 5-4 : used for vertical intrusive test only (not recommended)
 *          00=move 1 slicer in direction bit0 (slicer selected for max range)
 *          11=move both, independent direction(not depend on bit0) legacy 40nm mode
 *          01=move only odd(depends on bit0)
 *          10=move only even(depends on bit0)
 * bit 3 : 1 = set passive scan to narrow vertical range(150mV); 0 = full range(250mV)
 * bit 2 : 1 = intrusive eye scan; 0 = passive 
 * bit 1 : 1 = scan horizontal direction; 0 = scan vertical
 * bit 0 : 1 = scan negative portion of eye to center; 1 = scan positive
 *
 * @param pa phymod_access_t struct
 * @param timer_control sets the total test time in units of ~1.31 seconds
 * @param max_error_control sets the error threshold for test in units of 16.(4=64 error threshold)
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_start_ber_scan_test( const phymod_access_t *pa, uint8_t ber_scan_mode, uint8_t timer_control, uint8_t max_error_control);

/** Reads the BER scan data from uC after test has completed 
 * @param pa phymod_access_t struct
 * @param *errors is pointer to 32 element array of uint32 which will contain error data
 * @param *timer_values is pointer to 32 element array of uint32 which will contain time data
 * @param *cnt returns the number of samples
 * @param timeout for polling data from uC (typically 2000)
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_read_ber_scan_data( const phymod_access_t *pa, uint32_t *errors, uint32_t *timer_values, uint8_t *cnt, uint32_t timeout);

/** Extrapolate BER and display margin information
 * @param pa phymod_access_t struct
 * @param rate specifies the data rate in Hz
 * @param ber_scan_mode the type of test used to take the data(use enum ber_scan_mode_enum)
 * @param *total_errs is pointer to 32 element array of uint32 containing the error data
 * @param *total_time is pointer to 32 element array of uint32 containing the time data
 * @param max_offset is the maximum offset setting which is present in data (usually 31)
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_ber_scan_data(USR_DOUBLE rate, uint8_t ber_scan_mode, uint32_t *total_errs, uint32_t *total_time,uint8_t max_offset);

/** This is an example function which uses the following API's to measure and display BER margin projections
 * falcon_tsc_start_ber_scan_test, falcon_tsc_read_ber_scan_data, falcon_tsc_display_ber_scan_data
 *
 * @param pa phymod_access_t struct
 * @param rate specifies the data rate in Hz
 * @param ber_scan_mode the type of test used to take the data(use enum ber_scan_mode_enum)
 * @param timer_control sets the total test time in units of ~1.31 seconds
 * @param max_error_control sets the error threshold for test in units of 16.(4=64 errors)
 * @return Error Code during data collection (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_eye_margin_proj( const phymod_access_t *pa, USR_DOUBLE rate, uint8_t ber_scan_mode, uint8_t timer_control, uint8_t max_error_control);

/*-----------------------------------------------*/
/*  Display Serdes Lane Config and Debug Status  */
/*-----------------------------------------------*/
/** Display current lane configuration.
 * Reads and displays all important lane configuration RAM variables and register fields.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_lane_config( const phymod_access_t *pa );


/** Display current lane debug status.
 * Reads and displays all vital lane user status and debug status
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_lane_debug_status( const phymod_access_t *pa );


/*-----------------------------*/
/*  Display Serdes Lane State  */
/*-----------------------------*/
/** Display current lane state.
 * Reads and displays all important lane state values.
 * Call functions falcon_tsc_display_lane_state_hdr() before and falcon_tsc_display_lane_state_legend() after 
 * to get a formatted lane state display with legend 
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_lane_state( const phymod_access_t *pa );


/** Column definition header for falcon_tsc display state.
 * To be called before falcon_tsc_display_lane_state() function 
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_lane_state_hdr( const phymod_access_t *pa );


/** Detailed explanation of each column in falcon_tsc display state.
 * To be called after falcon_tsc_display_lane_state() function to display the legends 
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_lane_state_legend( const phymod_access_t *pa );



/*-------------------------------------*/
/*   PMD_RX_LOCK and CL72/CL93 Status  */
/*-------------------------------------*/

/** PMD rx lock status of current lane.
 * @param pa phymod_access_t struct
 * @param *pmd_rx_lock PMD_RX_LOCK status of current lane returned by API
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_pmd_lock_status( const phymod_access_t *pa, uint8_t *pmd_rx_lock);


/** Display CL93n72 Status of current lane.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_display_cl93n72_status( const phymod_access_t *pa );




/*--------------------------------*/
/*  Serdes TX disable/RX Restart  */ 
/*--------------------------------*/
/** TX Disable.
 * @param pa phymod_access_t struct
 * @param enable Enable/Disable TX disable (1 = TX Disable asserted; 0 = TX Disable removed) 
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_tx_disable( const phymod_access_t *pa, uint8_t enable);


/** Enable/disable Restart RX and hold. (Reset DSC state machine into RESTART State and hold it till disabled)
 * @param pa phymod_access_t struct
 * @param enable Enable/Disable Restart RX and hold (1 = RX restart and hold; 0 = Release hold in restart state)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_rx_restart( const phymod_access_t *pa, uint8_t enable);


/*-----------------------------*/
/*  Stop/Resume RX Adaptation  */
/*-----------------------------*/
/** Stop RX Adaptation on a Lane.
 * RX Adaptation needs to be stopped before modifying any of the VGA, PF or DFE taps
 * @param pa phymod_access_t struct
 * @param enable Enable RX Adaptation stop (1 = Stop RX Adaptation on lane; 0 = Resume RX Adaptation on lane) 
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_stop_rx_adaptation( const phymod_access_t *pa, uint8_t enable);

/*------------------------------------*/
/*  Read/Write all RX AFE parameters  */
/*------------------------------------*/

/** Write to RX AFE settings.
 * RX Adaptation needs to be stopped before modifying any of the VGA, PF or DFE taps
 * @param pa phymod_access_t struct
 * @param param Enum (falcon_tsc_rx_afe_settings_enum) to select the required RX AFE setting to be modified
 * @param val Value to be written to the selected AFE setting
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_write_rx_afe( const phymod_access_t *pa, enum falcon_tsc_rx_afe_settings_enum param, int8_t val);

/** Read from RX AFE settings.
 * @param pa phymod_access_t struct
 * @param param Enum (falcon_tsc_rx_afe_settings_enum) to select the required RX AFE setting to be read
 * @param *val Value to be written to the selected AFE setting
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_read_rx_afe( const phymod_access_t *pa, enum falcon_tsc_rx_afe_settings_enum param, int8_t *val);

/*----------------------------*/
/*  Enable Pattern Generator  */
/*----------------------------*/
/** Enable/Disable Shared TX pattern generator.
 * Note: The patt_length input to the function should be the value sent to the falcon_tsc_config_shared_tx_pattern() function
 * @param pa phymod_access_t struct
 * @param enable Enable shared fixed pattern generator (1 = Enable; 0 = Disable)
 * @param patt_length length of the pattern used in falcon_tsc_config_shared_tx_pattern()
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_tx_shared_patt_gen_en( const phymod_access_t *pa, uint8_t enable, uint8_t patt_length);


/*----------------------------*/
/*  Configure PRBS Functions  */
/*----------------------------*/
/**  Configure PRBS Generator.
 * Once the PRBS generator is configured, it can be enabled using the falcon_tsc_tx_prbs_en() function
 * @param pa phymod_access_t struct
 * @param prbs_poly_mode PRBS generator mode select (selects required PRBS polynomial)
 * @param prbs_inv PRBS invert enable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_config_tx_prbs( const phymod_access_t *pa, enum falcon_tsc_prbs_polynomial_enum prbs_poly_mode, uint8_t prbs_inv);

/**  Get PRBS Generator Configuration.
 * @param pa phymod_access_t struct
 * @param *prbs_poly_mode PRBS generator mode select (selects required PRBS polynomial)
 * @param *prbs_inv PRBS invert enable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_get_tx_prbs_config( const phymod_access_t *pa, enum falcon_tsc_prbs_polynomial_enum *prbs_poly_mode, uint8_t *prbs_inv);


/** PRBS Generator Enable/Disable
 * @param pa phymod_access_t struct
 * @param enable Enable PRBS Generator (1 = Enable; 0 = Disable)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_tx_prbs_en( const phymod_access_t *pa, uint8_t enable);

/** Get PRBS Generator Enable/Disable
 * @param pa phymod_access_t struct
 * @param *enable returns the value of Enable PRBS Generator (1 = Enable; 0 = Disable)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_get_tx_prbs_en( const phymod_access_t *pa, uint8_t *enable);

/** PRBS Generator Single Bit Error Injection
 * @param pa phymod_access_t struct
 * @param enable (1 = error is injected; 0 = no error is injected)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_tx_prbs_err_inject( const phymod_access_t *pa, uint8_t enable);


/**  Configure PRBS Checker.
 * Once the PRBS checker is configured, it can be enabled using the falcon_tsc_rx_prbs_en() function
 * @param pa phymod_access_t struct
 * @param prbs_poly_mode PRBS checker mode select (selects required PRBS polynomial)
 * @param prbs_checker_mode Checker Mode to select PRBS LOCK state machine 
 * @param prbs_inv PRBS invert enable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_config_rx_prbs( const phymod_access_t *pa, enum falcon_tsc_prbs_polynomial_enum prbs_poly_mode, enum falcon_tsc_prbs_checker_mode_enum prbs_checker_mode, uint8_t prbs_inv);


/**  Get PRBS Checker congifuration.
 * @param pa phymod_access_t struct
 * @param *prbs_poly_mode PRBS checker mode select (selects required PRBS polynomial)
 * @param *prbs_checker_mode Checker Mode to select PRBS LOCK state machine 
 * @param *prbs_inv PRBS invert enable
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_get_rx_prbs_config( const phymod_access_t *pa, enum falcon_tsc_prbs_polynomial_enum *prbs_poly_mode, enum falcon_tsc_prbs_checker_mode_enum *prbs_checker_mode, uint8_t *prbs_inv);

/** PRBS Checker Enable/Disable
 * @param pa phymod_access_t struct
 * @param enable Enable PRBS Checker (1 = Enable; 0 = Disable)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_rx_prbs_en( const phymod_access_t *pa, uint8_t enable);

/** Get PRBS Checker Enable/Disable
 * @param pa phymod_access_t struct
 * @param *enable returns with the value of Enable PRBS Checker (1 = Enable; 0 = Disable)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */ 
err_code_t falcon_tsc_get_rx_prbs_en( const phymod_access_t *pa, uint8_t *enable);

/** PRBS Checker LOCK status (live status).
 * @param pa phymod_access_t struct
 * @param *chk_lock Live lock status read by API
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_prbs_chk_lock_state( const phymod_access_t *pa, uint8_t *chk_lock);


/** PRBS Error Count and Lock Lost status.
 * Error count and lock lost read back as a single 32bit value. Bit 31 is lock lost and [30:0] is error count.  
 * @param pa phymod_access_t struct
 * @param *prbs_err_cnt 32bit value returned by API ([30:0] = Error Count; [31] = Lock lost) 
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_prbs_err_count_ll( const phymod_access_t *pa, uint32_t *prbs_err_cnt);


/** PRBS Error Count and Lock Lost status.
 * Error count and lock lost read back on separate variables
 * @param pa phymod_access_t struct
 * @param *prbs_err_cnt 32bit Error count value
 * @param *lock_lost Lock Lost status (1 = if lock was even lost)
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_prbs_err_count_state( const phymod_access_t *pa, uint32_t *prbs_err_cnt, uint8_t *lock_lost);


/*--------------------------------------------------------------*/
/*  IDDQ / Powerdown / Deep Powerdown / Isolate Pins  */
/*--------------------------------------------------------------*/
/** Lane configurations for IDDQ.
 * User needs to configure all lanes through falcon_tsc_lane_config_for_iddq() and also call falcon_tsc_core_config_for_iddq() 
 * before enabling IDDQ by asserting IDDQ pin.
 * @param pa phymod_access_t struct
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_lane_config_for_iddq( const phymod_access_t *pa );


/** Serdes Lane Powerdown.
 * Powers down TX,RX,LANE, DEEP, PWR_ON
 * @param pa phymod_access_t struct
 * @param mode Enable/Disable lane powerdown based on enum core_pwrdn_mode_enum
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors) 
 */
err_code_t falcon_tsc_lane_pwrdn( const phymod_access_t *pa, enum core_pwrdn_mode_enum mode);

/** TX_PI Fixed Frequency Mode
 * @param pa phymod_access_t struct
 * @param enable Enable/Disable TX_PI (1 = Enable; 0 = Disable)
 * @param freq_override_val Fixed Frequency Override value (Rang: -8192 to + 8192; ppm = freq_override_val*790/8192)
 * @return Error Code generated by invalid TX_PI settings (returns ERR_CODE_NONE if no errors)
 */
err_code_t falcon_tsc_tx_pi_freq_override( const phymod_access_t *pa, uint8_t enable, int16_t freq_override_val);

/** Set the uC active mode
 * @param pa phymod_access_t struct
 * @param enable Enable/Disable uC Active (1 = Enable; 0 = Disable)
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors)
 */
err_code_t falcon_tsc_uc_active_enable( const phymod_access_t *pa, uint8_t enable);

/** Enable or Disable the uC reset
 * @param pa phymod_access_t struct
 * @param enable Enable/Disable uC reset (1 = Enable; 0 = Disable)
 * @return Error Code generated by invalid access (returns ERR_CODE_NONE if no errors)
 */
err_code_t falcon_tsc_uc_reset( const phymod_access_t *pa, uint8_t enable);

/*--------------------------------------------*/
/*  Loopback and Ultra-Low Latency Functions  */
/*--------------------------------------------*/


/** Locks TX_PI to Loop timing
 * @param pa phymod_access_t struct
 * @param enable Enable TX_PI lock to loop timing (1 = Enable Lock; 0 = Disable Lock)  
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t falcon_tsc_loop_timing( const phymod_access_t *pa, uint8_t enable);       


/** Enable/Disable Digital Loopback
 * @param pa phymod_access_t struct
 * @param enable Enable Digital Loopback (1 = Enable dig_lpbk; 0 = Disable dig_lpbk)  
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t falcon_tsc_dig_lpbk( const phymod_access_t *pa, uint8_t enable);

/** Enable/Disable Remote Loopback
 * @param pa phymod_access_t struct
 * @param enable Enable Remote Loopback (1 = Enable rmt_lpbk; 0 = Disable rmt_lpbk)  
 * @return Error Code generated by API (returns ERR_CODE_NONE if no errors)
 */
err_code_t falcon_tsc_rmt_lpbk( const phymod_access_t *pa, uint8_t enable);

#endif
