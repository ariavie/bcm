/*
 * $Id: time.c,v 1.77 Broadcom SDK $
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
 * File:       time.c
 *
 * Remarks:    Broadcom StrataSwitch Time BroadSync API
 *
 * Functions:
 *      Private Interface functions
 *
 *      _bcm_esw_time_hw_clear
 *      _bcm_esw_time_deinit
 *      _bcm_esw_time_reinit
 *      _bcm_esw_time_interface_id_validate
 *      _bcm_esw_time_interface_input_validate
 *      _bcm_esw_time_interface_allocate_id
 *      _bcm_esw_time_interface_heartbeat_install
 *      _bcm_esw_time_interface_accuracy_time2hw
 *      _bcm_esw_time_interface_drift_install
 *      _bcm_esw_time_interface_offset_install
 *      _bcm_esw_time_interface_ref_clock_install
 *      _bcm_esw_time_interface_install
 *      _bcm_esw_time_interface_dual_bs_install
 *      _bcm_time_interface_free
 *      _bcm_esw_time_accuracy_parse
 *      _bcm_esw_time_input_parse
 *      _bcm_esw_time_capture_counter_read
 *      _bcm_esw_time_capture_get
 *      _bcm_esw_time_interface_offset_get
 *      _bcm_esw_time_interface_drift_get
 *      _bcm_esw_time_interface_ref_clock_get
 *      _bcm_esw_time_interface_accuracy_get
 *      _bcm_esw_time_interface_get
 *      _bcm_esw_time_interface_dual_bs_get
 *      _bcm_esw_time_hw_interrupt_dflt
 *      _bcm_esw_time_hw_interrupt
 *      _bcm_esw_time_trigger_to_timestamp_mode
 *      _bcm_esw_time_trigger_from_timestamp_mode
 *      _bcm_esw_time_bs_init
 *      _bcm_time_sw_dump
 *
 *      Public Interface functions
 *
 *      bcm_esw_time_init
 *      bcm_esw_time_deinit
 *      bcm_esw_time_interface_add
 *      bcm_esw_time_interface_delete
 *      bcm_esw_time_interface_get
 *      bcm_esw_time_interface_delete_all
 *      bcm_esw_time_interface_traverse
 *      bcm_esw_time_capture_get
 *      bcm_esw_time_heartbeat_enable_set
 *      bcm_esw_time_heartbeat_enable_get
 *      bcm_esw_time_trigger_enable_set
 *      bcm_esw_time_trigger_enable_get
 *      bcm_esw_time_heartbeat_register
 *      bcm_esw_time_heartbeat_unregister
 */

#include <shared/util.h>
#include <shared/bsl.h>
#include <bcm/time.h>
#include <bcm/error.h>

#include <soc/mem.h>

#include <bcm_int/common/time.h>
#include <bcm_int/common/time-mbox.h>
#include <bcm_int/common/mbox.h>

#include <bcm_int/esw/mbcm.h>
#include <bcm_int/esw_dispatch.h>

#define BROAD_SYNC_TIME_CAPTURE_TIMEOUT      (10) /* useconds */
#define BROAD_SYNC_OUTPUT_TOGGLE_TIME_DELAY  (3)  /* seconds */ 

#define SYNT_TIME_SECONDS(unit, id) \
        TIME_INTERFACE_CONFIG(unit, id).time_capture.syntonous.seconds
#define SYNT_TIME_NANOSECONDS(unit, id) \
        TIME_INTERFACE_CONFIG(unit, id).time_capture.syntonous.nanoseconds

/****************************************************************************/
/*                      LOCAL VARIABLES DECLARATION                         */
/****************************************************************************/
static _bcm_time_config_p _bcm_time_config[BCM_MAX_NUM_UNITS] = {NULL};

static bcm_time_spec_t _bcm_time_accuracy_arr[TIME_ACCURACY_CLK_MAX] = {
      {0, COMPILER_64_INIT(0,0),  25},        /* HW value = 32, accuracy up tp 25 nanosec */
      {0, COMPILER_64_INIT(0,0),  100},       /* HW value = 33, accuracy up to 100 nanosec */
      {0, COMPILER_64_INIT(0,0),  250},       /* HW value = 34, accuracy up to 250 nanosec */
      {0, COMPILER_64_INIT(0,0),  1000},      /* HW value = 35, accuracy up to 1 microsec */
      {0, COMPILER_64_INIT(0,0),  2500},      /* HW value = 36, accuracy up to 2.5 microsec */
      {0, COMPILER_64_INIT(0,0),  10000},     /* HW value = 37, accuracy up to 10 microsec */
      {0, COMPILER_64_INIT(0,0),  25000},     /* HW value = 38, accuracy up to 25 microsec */
      {0, COMPILER_64_INIT(0,0),  100000},    /* HW value = 39, accuracy up to 100 microsec */
      {0, COMPILER_64_INIT(0,0),  250000},    /* HW value = 40, accuracy up to 250 microsec */
      {0, COMPILER_64_INIT(0,0),  1000000},   /* HW value = 41, accuracy up to 1 milisec */
      {0, COMPILER_64_INIT(0,0),  2500000},   /* HW value = 42, accuracy up to 2.5 milisec */
      {0, COMPILER_64_INIT(0,0),  10000000},  /* HW value = 43, accuracy up to 10 milisec */
      {0, COMPILER_64_INIT(0,0),  25000000},  /* HW value = 44, accuracy up to 25 milisec */
      {0, COMPILER_64_INIT(0,0),  100000000}, /* HW value = 45, accuracy up to 100 milisec */
      {0, COMPILER_64_INIT(0,0),  250000000}, /* HW value = 46, accuracy up to 250 milisec */
      {0, COMPILER_64_INIT(0,1),  0},         /* HW value = 47, accuracy up to 1 sec */
      {0, COMPILER_64_INIT(0,10), 0},        /* HW value = 48, accuracy up to 10 sec */
      /* HW value = 49, accuracy greater than 10 sec */
      {0, COMPILER_64_INIT(0,TIME_ACCURACY_INFINITE), TIME_ACCURACY_INFINITE},         
      /* HW value = 254 accuracy unknown */
      {0, COMPILER_64_INIT(0,TIME_ACCURACY_UNKNOWN), TIME_ACCURACY_UNKNOWN}          
};


/****************************************************************************/
/*                      Internal functions implmentation                    */
/****************************************************************************/
STATIC int 
_bcm_esw_time_capture_get (int unit, bcm_time_if_t id, bcm_time_capture_t *time);
STATIC int 
_bcm_esw_time_interface_offset_get(int unit, bcm_time_if_t id, bcm_time_spec_t *offset);
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)||defined(BCM_TRIDENT2_SUPPORT)||defined(BCM_HELIX4_SUPPORT)
STATIC int
_bcm_esw_time_bs_init(int unit, bcm_time_interface_t *intf);
STATIC int
_bcm_esw_time_hw_clear(int unit, bcm_time_if_t intf_id);
STATIC int
_bcm_esw_time_hitless_reset(int unit, bcm_time_if_t intf_id);
#endif

/*
 * Function:
 *    _bcm_esw_time_hw_clear
 * Purpose:
 *    Internal routine used to clear all HW registers and table to default values
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    intf_id        - (IN) Time interface identifier
 * Returns:
 *    BCM_E_XXX
 */
STATIC int 
_bcm_esw_time_hw_clear(int unit, bcm_time_if_t intf_id)
{
    uint32 regval;

#if defined(BCM_TIME_V3_SUPPORT) || defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    /*****************************************************************************/
    /*         Perform KATANA2 and HELIX4 IPROC HW configuration                 */
    /*****************************************************************************/

    /* Reset Config register */
    if (soc_feature(unit, soc_feature_time_v3) || SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        if (SOC_REG_IS_VALID(unit, CMIC_BS_DRIFT_RATEr)) {
            /* Reset Drift Rate */
            READ_CMIC_BS_DRIFT_RATEr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_DRIFT_RATEr, &regval, SIGNf, 0);
            soc_reg_field_set(unit, CMIC_BS_DRIFT_RATEr, &regval, FRAC_NSf, 0);
            WRITE_CMIC_BS_DRIFT_RATEr(unit, regval);
        }
    
        if (SOC_REG_IS_VALID(unit, CMIC_BS_OFFSET_ADJUST_0r)) {
            /* Reset Offset Adjust */
            READ_CMIC_BS_OFFSET_ADJUST_0r(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_OFFSET_ADJUST_0r, &regval, SECONDf, 0);
            WRITE_CMIC_BS_OFFSET_ADJUST_0r(unit, regval);

            READ_CMIC_BS_OFFSET_ADJUST_1r(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_OFFSET_ADJUST_1r, &regval, SIGN_BITf, 0);
            soc_reg_field_set(unit, CMIC_BS_OFFSET_ADJUST_1r, &regval, NSf, 0);
            WRITE_CMIC_BS_OFFSET_ADJUST_1r(unit, regval);
        }

        READ_CMIC_BS0_CONFIGr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, MODEf, TIME_MODE_OUTPUT);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, 0);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, 0);
        WRITE_CMIC_BS0_CONFIGr(unit, regval);

        READ_CMIC_BS1_CONFIGr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &regval, MODEf, TIME_MODE_OUTPUT);
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, 0);
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, 0);
        WRITE_CMIC_BS1_CONFIGr(unit, regval);

        READ_CMIC_BS0_OUTPUT_TIME_0r(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_OUTPUT_TIME_0r, &regval, ACCURACYf, 0);
        soc_reg_field_set(unit, CMIC_BS0_OUTPUT_TIME_0r, &regval, LOCKf, 0);
        soc_reg_field_set(unit, CMIC_BS0_OUTPUT_TIME_0r, &regval, EPOCHf, 0);
        WRITE_CMIC_BS0_OUTPUT_TIME_0r(unit, regval);

        READ_CMIC_BS1_OUTPUT_TIME_0r(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_OUTPUT_TIME_0r, &regval, ACCURACYf, 0);
        soc_reg_field_set(unit, CMIC_BS1_OUTPUT_TIME_0r, &regval, LOCKf, 0);
        soc_reg_field_set(unit, CMIC_BS1_OUTPUT_TIME_0r, &regval, EPOCHf, 0);
        WRITE_CMIC_BS1_OUTPUT_TIME_0r(unit, regval);

        /* Reset Clock Control */
        READ_CMIC_BS0_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_CLK_CTRLr, &regval, ENABLEf, 0);
        WRITE_CMIC_BS0_CLK_CTRLr(unit, regval);

        READ_CMIC_BS1_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_CLK_CTRLr, &regval, ENABLEf, 0);
        WRITE_CMIC_BS1_CLK_CTRLr(unit, regval);

        /* Reset HeartBeat */
        READ_CMIC_BS0_HEARTBEAT_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_HEARTBEAT_CTRLr, &regval, ENABLEf, 0);
        WRITE_CMIC_BS0_HEARTBEAT_CTRLr(unit, regval);

        READ_CMIC_BS1_HEARTBEAT_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_HEARTBEAT_CTRLr, &regval, ENABLEf, 0);
        WRITE_CMIC_BS1_HEARTBEAT_CTRLr(unit, regval);

        /* Reset Capture */
        READ_CMIC_TIMESYNC_INTERRUPT_ENABLEr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_INTERRUPT_ENABLEr, &regval, INT_ENABLEf, 0);
        WRITE_CMIC_TIMESYNC_INTERRUPT_ENABLEr(unit, regval);

        READ_CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, &regval, TIME_CAPTURE_ENABLEf, 0);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, &regval, BSYNC0_TX_HB_STATUS_ENABLEf, 0);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, &regval, BSYNC0_RX_HB_STATUS_ENABLEf, 0);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, &regval, BSYNC1_TX_HB_STATUS_ENABLEf, 0);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, &regval, BSYNC1_RX_HB_STATUS_ENABLEf, 0);
        WRITE_CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr(unit, regval);

        READ_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_MODEr, &regval, TIME_CAPTURE_MODEf, TIME_CAPTURE_MODE_DISABLE);
        WRITE_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, regval);

    } else
#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */

    /*****************************************************************************/
    /*                Non-KATANA2 or HELIX4 HW configuration                     */
    /*****************************************************************************/
    {
        if (SOC_REG_IS_VALID(unit, CMIC_BS_DRIFT_RATEr)) {
            /* Reset Drift Rate  */
            READ_CMIC_BS_DRIFT_RATEr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_DRIFT_RATEr, &regval, SIGNf, 0);
            soc_reg_field_set(unit, CMIC_BS_DRIFT_RATEr, &regval, FRAC_NSf, 0);
            WRITE_CMIC_BS_DRIFT_RATEr(unit, regval);
        }

        if (SOC_REG_IS_VALID(unit, CMIC_BS_OFFSET_ADJUST_0r)) {
            /* Reset Offset Adjust */
            READ_CMIC_BS_OFFSET_ADJUST_0r(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_OFFSET_ADJUST_0r, &regval, SECONDf, 0);
            WRITE_CMIC_BS_OFFSET_ADJUST_0r(unit, regval);
            READ_CMIC_BS_OFFSET_ADJUST_1r(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_OFFSET_ADJUST_1r, &regval, SIGN_BITf, 0);
            soc_reg_field_set(unit, CMIC_BS_OFFSET_ADJUST_1r, &regval, NSf, 0);
            WRITE_CMIC_BS_OFFSET_ADJUST_1r(unit, regval);
        }

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_KATANA(unit) || SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
            READ_CMIC_BS_CONFIGr(unit, &regval);
            /* Configure Broadsync in output mode */
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, MODEf, TIME_MODE_OUTPUT);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, 0);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, 0);
            WRITE_CMIC_BS_CONFIGr(unit, regval);

            READ_CMIC_BS_OUTPUT_TIME_0r(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_OUTPUT_TIME_0r, &regval, ACCURACYf, 0);
            soc_reg_field_set(unit, CMIC_BS_OUTPUT_TIME_0r, &regval, LOCKf, 0);
            soc_reg_field_set(unit, CMIC_BS_OUTPUT_TIME_0r, &regval, EPOCHf, 0);
            WRITE_CMIC_BS_OUTPUT_TIME_0r(unit, regval);
        } else
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */

        {
            READ_CMIC_BS_CONFIGr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, ENABLEf, 0);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, MODEf, TIME_MODE_OUTPUT);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, TIME_CODE_ENABLEf, 0);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, LOCKf, 0);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, ACCURACYf, TIME_ACCURACY_UNKNOWN_HW_VAL);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, EPOCHf, 0);
            WRITE_CMIC_BS_CONFIGr(unit, regval);
        }

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_KATANA(unit) || SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
            READ_CMIC_BS_CLK_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CLK_CTRLr, &regval, ENABLEf, 0);
            WRITE_CMIC_BS_CLK_CTRLr(unit, regval);
        } else 
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */
        {
            READ_CMIC_BS_CLK_CTRL_0r(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CLK_CTRL_0r, &regval, ENABLEf, 0);
            soc_reg_field_set(unit, CMIC_BS_CLK_CTRL_0r, &regval, NSf, 0);
            WRITE_CMIC_BS_CLK_CTRL_0r(unit, regval);
        }

        if (SOC_REG_IS_VALID(unit, CMIC_BS_CLK_CTRL_1r)) {
            READ_CMIC_BS_CLK_CTRL_1r(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CLK_CTRL_1r, &regval, FRAC_NSf, 0);
            WRITE_CMIC_BS_CLK_CTRL_1r(unit, regval);
        }

        /* Reset Clock Toggle  */
        if (SOC_REG_IS_VALID(unit, CMIC_BS_CLK_TOGGLE_TIME_0r)) {
            READ_CMIC_BS_CLK_TOGGLE_TIME_0r(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CLK_TOGGLE_TIME_0r, &regval, SECf, 0);
            WRITE_CMIC_BS_CLK_TOGGLE_TIME_0r(unit, regval);
        }

        if (SOC_REG_IS_VALID(unit, CMIC_BS_CLK_TOGGLE_TIME_1r)) {
            READ_CMIC_BS_CLK_TOGGLE_TIME_1r(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CLK_TOGGLE_TIME_1r, &regval, NSf, 0);
            WRITE_CMIC_BS_CLK_TOGGLE_TIME_1r(unit, regval);
        }
    
        if (SOC_REG_IS_VALID(unit, CMIC_BS_CLK_TOGGLE_TIME_2r)) {
            READ_CMIC_BS_CLK_TOGGLE_TIME_2r(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CLK_TOGGLE_TIME_2r, &regval, FRAC_NSf, 0);
            WRITE_CMIC_BS_CLK_TOGGLE_TIME_2r(unit, regval);
        }

        READ_CMIC_BS_HEARTBEAT_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS_HEARTBEAT_CTRLr, &regval, ENABLEf, 0);
        if (!(SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
              SOC_IS_TD2_TT2(unit))) {
            soc_reg_field_set(unit, CMIC_BS_HEARTBEAT_CTRLr, &regval, THRESHOLDf, 0);
        }
        WRITE_CMIC_BS_HEARTBEAT_CTRLr(unit, regval);

#if (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT))
        if (SOC_IS_KATANA(unit) || SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
            READ_CMIC_TS_TIME_CAPTURE_CTRLr(unit, &regval);
            /* Configure the HW to give interrupt on every heartbeat */
            soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, 
                              INT_ENABLEf, 0);
            /* Configure the HW to capture time on every heartbeat */
            soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, 
                            TIME_CAPTURE_MODEf, TIME_CAPTURE_MODE_HEARTBEAT);
            WRITE_CMIC_TS_TIME_CAPTURE_CTRLr(unit, regval);
        } else
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */
        {
            READ_CMIC_BS_CAPTURE_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CAPTURE_CTRLr, &regval, INT_ENf, 0);
            soc_reg_field_set(unit, CMIC_BS_CAPTURE_CTRLr, &regval, TIME_CAPTURE_MODEf,
                              TIME_CAPTURE_MODE_DISABLE);
            WRITE_CMIC_BS_CAPTURE_CTRLr(unit, regval);
        }
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *    _bcm_esw_time_hitless_reset
 * Purpose:
 *    Internal routine used to check if hitless reset is present in
 *    the hardware, and is active.
 *    If present, hitless reset preserves the BroadSync operation
 *    and registers through a chip reset.
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    intf_id        - (IN) Time interface identifier
 * Returns:
 *    BCM_E_NONE if hitless reset is active
 *    BCM_E_UNAVAIL if it is not.
 */
int _bcm_esw_time_hitless_reset(int unit, bcm_time_if_t intf_id)
{
#if defined(BCM_HITLESS_RESET_SUPPORT)
    if (SOC_IS_KATANA2(unit)) {
        /* if BS0 has its output enabled, this indicates that
         * drv.c determined that hitless reset was in effect
         * and so did not clear the output.
         */
        uint32 regval;
        READ_CMIC_BS0_CONFIGr(unit, &regval);
        if (soc_reg_field_get(unit, CMIC_BS0_CONFIGr,
                              regval, BS_CLK_OUTPUT_ENABLEf) != 0) {
            return BCM_E_NONE;
        }
    }
#endif

    return BCM_E_UNAVAIL;
}

/*
 * Function:
 *    _bcm_esw_time_deinit
 * Purpose:
 *    Internal routine used to free time software module
 *    control structures. 
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    time_cfg_pptr  - (IN) Pointer to pointer to time config structure.
 * Returns:
 *    BCM_E_XXX
 */
STATIC int
_bcm_esw_time_deinit(int unit, _bcm_time_config_p *time_cfg_pptr)
{
    int                 idx;
    _bcm_time_config_p  time_cfg_ptr;
    soc_control_t       *soc = SOC_CONTROL(unit);

    /* Sanity checks. */
    if (NULL == time_cfg_pptr) {
        return (BCM_E_PARAM);
    }

    time_cfg_ptr = *time_cfg_pptr;
    /* If time config was not allocated we are done. */
    if (NULL == time_cfg_ptr) {
        return (BCM_E_NONE);
    }

    /* Free time interface */
    if (NULL != time_cfg_ptr->intf_arr) {
        for (idx = 0; idx < NUM_TIME_INTERFACE(unit); idx++) {
            if (NULL !=  time_cfg_ptr->intf_arr[idx].user_cb) {
                sal_free(time_cfg_ptr->intf_arr[idx].user_cb);
            }
        }
        sal_free(time_cfg_ptr->intf_arr);
    }

    /* Destroy protection mutex. */
    if (NULL != time_cfg_ptr->mutex) {
        sal_mutex_destroy(time_cfg_ptr->mutex);
    }

    /* If any registered function - deregister */
    soc->time_call_ref_count = 0;
    soc->soc_time_callout = NULL;

    /* Free module configuration structue. */
    sal_free(time_cfg_ptr);
    *time_cfg_pptr = NULL;
    return (BCM_E_NONE);
}

#ifdef BCM_WARM_BOOT_SUPPORT
/*
 * Function:
 *    _bcm_esw_time_reinit
 * Purpose:
 *    Internal routine used to reinitialize time module based on HW settings
 *    during Warm boot 
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    intf_id        - (IN) Time interface identifier
 * Returns:
 *    BCM_E_XXX
 */
STATIC int 
_bcm_esw_time_reinit(int unit, bcm_time_if_t intf_id)
{
    int hb_enable; 

    /* Read the status of heartbeat interrupt */
    BCM_IF_ERROR_RETURN(
        bcm_esw_time_heartbeat_enable_get(unit, intf_id, &hb_enable));

    /* If heartbeat was enabled, restore handling functionality */
    if (hb_enable) {
        BCM_IF_ERROR_RETURN(
            bcm_esw_time_heartbeat_enable_set(unit, intf_id, hb_enable));
    }

    return (BCM_E_NONE);
}
#endif /* BCM_WARM_BOOT_SUPPORT */
/*
 * Function:
 *    _bcm_esw_time_interface_id_validate
 * Purpose:
 *    Internal routine used to validate interface identifier 
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    id             - (IN) Interface id to validate
 * Returns:
 *    BCM_E_XXX
 */
STATIC int 
_bcm_esw_time_interface_id_validate(int unit, bcm_time_if_t id)
{
    if (0 == TIME_INIT(unit)) {
        return (BCM_E_INIT);
    }
    if (id < 0 || id > TIME_INTERFACE_ID_MAX(unit)) {
        return (BCM_E_PARAM);
    }
    if (NULL == TIME_INTERFACE(unit, id)) {
        return (BCM_E_NOT_FOUND);
    }

    return (BCM_E_NONE);
}


/*
 * Function:
 *    _bcm_esw_time_interface_input_validate
 * Purpose:
 *    Internal routine used to validate interface input 
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    intf           - (IN) Interface to validate
 * Returns:
 *    BCM_E_XXX
 */
STATIC int 
_bcm_esw_time_interface_input_validate(int unit, bcm_time_interface_t *intf)
{
    /* Sanity checks. */
    if (NULL == intf) {
        return (BCM_E_PARAM);
    }
    if (intf->flags & BCM_TIME_WITH_ID) {
        if (intf->id < 0 || intf->id > TIME_INTERFACE_ID_MAX(unit) ) {
            return (BCM_E_PARAM);
        }
    }

#if defined(BCM_TIME_V3_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3)) {
        /* Drift/Offset is not supported by 3rd generation time architecture */
        if (intf->flags & BCM_TIME_DRIFT || intf->flags & BCM_TIME_OFFSET) {
            return BCM_E_UNAVAIL;
        }
    }
#endif /* defined(BCM_TIME_V3_SUPPORT) */

    if (intf->flags & BCM_TIME_DRIFT) {
        if (intf->drift.nanoseconds > TIME_DRIFT_MAX) {
            return BCM_E_PARAM;
        }
    }

    if (intf->flags & BCM_TIME_OFFSET) {
        if (intf->offset.nanoseconds > TIME_NANOSEC_MAX) {
            return BCM_E_PARAM;
}
    }

    return (BCM_E_NONE);
}


/*
 * Function:
 *    _bcm_esw_time_interface_allocate_id
 * Purpose:
 *    Internal routine used to allocate time interface id 
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    id             - (OUT) Interface id to be allocated
 * Returns:
 *    BCM_E_XXX
 */
STATIC int 
_bcm_esw_time_interface_allocate_id(int unit, bcm_time_if_t *id)
{
    int                              idx;  /* Time interfaces iteration index.*/
    _bcm_time_interface_config_p     intf; /* Time interface description.     */

    /* Input parameters check. */
    if (NULL == id) {
        return (BCM_E_PARAM);
    }

    /* Find & allocate time interface. */
    for (idx = 0; idx < TIME_CONFIG(unit)->intf_count; idx++) {
        intf = TIME_CONFIG(unit)->intf_arr + idx;
        if (intf->ref_count) {  /* In use interface */
            continue;
        }
        intf->ref_count++;  /* If founf mark interface as in use */
        *id = intf->time_interface.id; /* Assign ID */
        return (BCM_E_NONE);
    }

    /* No available interfaces */
    return (BCM_E_FULL);
}



/*
 * Function:
 *    _bcm_esw_time_interface_heartbeat_install
 * Purpose:
 *    Internal routine used to install interface heartbeat rate into a HW 
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    id             - (IN) Interface id to be installed into a HW
 * Returns:
 *    BCM_E_XXX
 */
STATIC int 
_bcm_esw_time_interface_heartbeat_install(int unit, bcm_time_if_t id)
{
    uint32  regval;
    uint32  hb_hz;  /* Number of heartbeats in HZ */
    uint32  ns_hp;  /* Number of nanosec per half clock cycle*/
    uint32  threshold; /* Heartbeat threshold */

    bcm_time_interface_t    *intf;

    intf = TIME_INTERFACE(unit, id);

    hb_hz = (intf->heartbeat_hz > TIME_HEARTBEAT_HZ_MAX) ? 
        TIME_HEARTBEAT_HZ_MAX : intf->heartbeat_hz;
    hb_hz = (intf->heartbeat_hz < TIME_HEARTBEAT_HZ_MIN) ? 
        TIME_HEARTBEAT_HZ_MIN : hb_hz;

    threshold = intf->bitclock_hz / hb_hz;

    if (threshold < 100) {
        /* BitClock must have a frequency at least 100 times that of heartbeat. */
        return BCM_E_PARAM;
    }

    /* Calculate nanoseconds per half period of cycle */
    ns_hp = TIME_HEARTBEAT_NS_HZ / intf->bitclock_hz / 2;

    /* program the half period of broadsync clock */
#if defined(BCM_TIME_V3_SUPPORT) || defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3) || 
        SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        uint32  interval = 0;

        READ_CMIC_BS0_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_CLK_CTRLr, &regval, ENABLEf, 1);
        WRITE_CMIC_BS0_CLK_CTRLr(unit, regval);

        READ_CMIC_BS1_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_CLK_CTRLr, &regval, ENABLEf, 1);
        WRITE_CMIC_BS1_CLK_CTRLr(unit, regval);

        /* enable Heartbeat generation */
        READ_CMIC_BS0_HEARTBEAT_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_HEARTBEAT_CTRLr, &regval, ENABLEf, 1); 
        WRITE_CMIC_BS0_HEARTBEAT_CTRLr(unit, regval);

        /* Install heartbeat up and down interval */ 
        READ_CMIC_BS0_HEARTBEAT_UP_DURATIONr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_HEARTBEAT_UP_DURATIONr, &regval, UP_DURATIONf, threshold);
        WRITE_CMIC_BS0_HEARTBEAT_UP_DURATIONr(unit, regval);

        READ_CMIC_BS0_HEARTBEAT_DOWN_DURATIONr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_HEARTBEAT_DOWN_DURATIONr, &regval, DOWN_DURATIONf, threshold);
        WRITE_CMIC_BS0_HEARTBEAT_DOWN_DURATIONr(unit, regval);

        READ_CMIC_BS1_HEARTBEAT_UP_DURATIONr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_HEARTBEAT_UP_DURATIONr, &regval, UP_DURATIONf, threshold);
        WRITE_CMIC_BS1_HEARTBEAT_UP_DURATIONr(unit, regval);

        READ_CMIC_BS1_HEARTBEAT_DOWN_DURATIONr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_HEARTBEAT_DOWN_DURATIONr, &regval, DOWN_DURATIONf, threshold);
        WRITE_CMIC_BS1_HEARTBEAT_DOWN_DURATIONr(unit, regval);

        /* Enable heartbeat output */
        READ_CMIC_BS0_HEARTBEAT_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_HEARTBEAT_CTRLr, &regval, ENABLEf, 1);
        WRITE_CMIC_BS0_HEARTBEAT_CTRLr(unit, regval);

        READ_CMIC_BS1_HEARTBEAT_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_HEARTBEAT_CTRLr, &regval, ENABLEf, 1);
        WRITE_CMIC_BS1_HEARTBEAT_CTRLr(unit, regval);

        /* 
         * KT-1381 :
         *  TS_GPIO_LOW = (UP_EVENT_INTERVAL x 4) + 1 TS_REF_CLK_time_period
         *  TS_GPIO_HIGH = (DOWN_EVENT_INTERVAL x 4) + 1 TS_REF_CLK_time_period
         */ 
        threshold = (threshold * 4) + SOC_TIMESYNC_PLL_CLOCK_NS(unit);

        /* Install GPIO timesync trigger interval */
        /* GPIO_EVENT_1 */
        READ_CMIC_TIMESYNC_GPIO_0_DOWN_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_GPIO_0_DOWN_EVENT_CTRLr, &interval, INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TIMESYNC_GPIO_0_DOWN_EVENT_CTRLr(unit, interval);

        READ_CMIC_TIMESYNC_GPIO_0_UP_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_GPIO_0_UP_EVENT_CTRLr, &interval, INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TIMESYNC_GPIO_0_UP_EVENT_CTRLr(unit, interval);

        /* GPIO_EVENT_2 */
        READ_CMIC_TIMESYNC_GPIO_1_DOWN_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_GPIO_1_DOWN_EVENT_CTRLr, &interval, INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TIMESYNC_GPIO_1_DOWN_EVENT_CTRLr(unit, interval);

        READ_CMIC_TIMESYNC_GPIO_1_UP_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_GPIO_1_UP_EVENT_CTRLr, &interval, INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TIMESYNC_GPIO_1_UP_EVENT_CTRLr(unit, interval);

        /* GPIO_EVENT_3 */
        READ_CMIC_TIMESYNC_GPIO_2_DOWN_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_GPIO_2_DOWN_EVENT_CTRLr, &interval, INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TIMESYNC_GPIO_2_DOWN_EVENT_CTRLr(unit, interval);

        READ_CMIC_TIMESYNC_GPIO_2_UP_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_GPIO_2_UP_EVENT_CTRLr, &interval, INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TIMESYNC_GPIO_2_UP_EVENT_CTRLr(unit, interval);

        /* GPIO_EVENT_4 */
        READ_CMIC_TIMESYNC_GPIO_3_DOWN_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_GPIO_3_DOWN_EVENT_CTRLr, &interval, INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TIMESYNC_GPIO_3_DOWN_EVENT_CTRLr(unit, interval);

        READ_CMIC_TIMESYNC_GPIO_3_UP_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_GPIO_3_UP_EVENT_CTRLr, &interval, INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TIMESYNC_GPIO_3_UP_EVENT_CTRLr(unit, interval);

        /* GPIO_EVENT_5 */
        READ_CMIC_TIMESYNC_GPIO_4_DOWN_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_GPIO_4_DOWN_EVENT_CTRLr, &interval, INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TIMESYNC_GPIO_4_DOWN_EVENT_CTRLr(unit, interval);

        READ_CMIC_TIMESYNC_GPIO_4_UP_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_GPIO_4_UP_EVENT_CTRLr, &interval, INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TIMESYNC_GPIO_4_UP_EVENT_CTRLr(unit, interval);

    } else 

#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        READ_CMIC_BS_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS_CLK_CTRLr, &regval, ENABLEf, 1);
        WRITE_CMIC_BS_CLK_CTRLr(unit, regval);

    } else 
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */
    {
        READ_CMIC_BS_CLK_CTRL_0r(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS_CLK_CTRL_0r, &regval, ENABLEf, 1);
        soc_reg_field_set(unit, CMIC_BS_CLK_CTRL_0r, &regval, NSf, ns_hp);
        WRITE_CMIC_BS_CLK_CTRL_0r(unit, regval);
    }

    READ_CMIC_BS_HEARTBEAT_CTRLr(unit, &regval);
    soc_reg_field_set(unit, CMIC_BS_HEARTBEAT_CTRLr, &regval, ENABLEf, 1); 
    if (!(SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
          SOC_IS_TD2_TT2(unit))) {
        soc_reg_field_set(unit, CMIC_BS_HEARTBEAT_CTRLr, &regval, 
                          THRESHOLDf, threshold); 
    }
    WRITE_CMIC_BS_HEARTBEAT_CTRLr(unit, regval);

#if (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT))
    /* Install heartbeat and external trigger clock interval */
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        uint32  interval = 0;

        /* Install heartbeat up and down interval */ 
        READ_CMIC_BS_HEARTBEAT_UP_DURATIONr(unit, &interval);
        soc_reg_field_set(unit, CMIC_BS_HEARTBEAT_UP_DURATIONr, &interval, 
                          UP_DURATIONf, threshold); 
        WRITE_CMIC_BS_HEARTBEAT_UP_DURATIONr(unit, interval);

        READ_CMIC_BS_HEARTBEAT_DOWN_DURATIONr(unit, &interval);
        soc_reg_field_set(unit, CMIC_BS_HEARTBEAT_DOWN_DURATIONr, &interval, 
                          DOWN_DURATIONf, threshold); 
        WRITE_CMIC_BS_HEARTBEAT_DOWN_DURATIONr(unit, interval);

        /* Install divisor for LCPLL trigger interval */
        READ_CMIC_TS_LCPLL_CLK_COUNT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TS_LCPLL_CLK_COUNT_CTRLr, &interval, 
                          DIVISORf, threshold);
        WRITE_CMIC_TS_LCPLL_CLK_COUNT_CTRLr(unit, interval);

       /* Install divisor for Primary L1 trigger interval */
        READ_CMIC_TS_L1_CLK_RECOVERED_CLK_COUNT_CTRLr(unit, &interval);
        soc_reg_field_set(unit,  CMIC_TS_L1_CLK_RECOVERED_CLK_COUNT_CTRLr, &interval, 
                          DIVISORf, threshold);
        WRITE_CMIC_TS_L1_CLK_RECOVERED_CLK_COUNT_CTRLr(unit, interval);

        /* Install divisor for backup L1 trigger interval */
        READ_CMIC_TS_L1_CLK_RECOVERED_CLK_BKUP_COUNT_CTRLr(unit, &interval);
        soc_reg_field_set(unit,  CMIC_TS_L1_CLK_RECOVERED_CLK_BKUP_COUNT_CTRLr, &interval, 
                          DIVISORf, threshold);
        WRITE_CMIC_TS_L1_CLK_RECOVERED_CLK_BKUP_COUNT_CTRLr(unit, interval);

        /* 
         * KT-1381 :
         *  TS_GPIO_LOW = (UP_EVENT_INTERVAL x 4) + 1 TS_REF_CLK_time_period
         *  TS_GPIO_HIGH = (DOWN_EVENT_INTERVAL x 4) + 1 TS_REF_CLK_time_period
         */ 
        threshold = (threshold * 4) + SOC_TIMESYNC_PLL_CLOCK_NS(unit);

        /* Install GPIO timesync trigger interval */
        /* GPIO_EVENT_1 */
        READ_CMIC_TS_GPIO_1_DOWN_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TS_GPIO_1_DOWN_EVENT_CTRLr, &interval,
                                INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TS_GPIO_1_DOWN_EVENT_CTRLr(unit, interval);
        READ_CMIC_TS_GPIO_1_UP_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TS_GPIO_1_UP_EVENT_CTRLr, &interval,
                                INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TS_GPIO_1_UP_EVENT_CTRLr(unit, interval);

        /* GPIO_EVENT_2 */
        READ_CMIC_TS_GPIO_2_DOWN_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TS_GPIO_2_DOWN_EVENT_CTRLr, &interval,
                                INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TS_GPIO_2_DOWN_EVENT_CTRLr(unit, interval);
        READ_CMIC_TS_GPIO_2_UP_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TS_GPIO_2_UP_EVENT_CTRLr, &interval,
                                INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TS_GPIO_2_UP_EVENT_CTRLr(unit, interval);

        /* GPIO_EVENT_3 */
        READ_CMIC_TS_GPIO_3_DOWN_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TS_GPIO_3_DOWN_EVENT_CTRLr, &interval,
                                INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TS_GPIO_3_DOWN_EVENT_CTRLr(unit, interval);
        READ_CMIC_TS_GPIO_3_UP_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TS_GPIO_3_UP_EVENT_CTRLr, &interval,
                                INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TS_GPIO_3_UP_EVENT_CTRLr(unit, interval);

        /* GPIO_EVENT_4 */
        READ_CMIC_TS_GPIO_4_DOWN_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TS_GPIO_4_DOWN_EVENT_CTRLr, &interval,
                                INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TS_GPIO_4_DOWN_EVENT_CTRLr(unit, interval);
        READ_CMIC_TS_GPIO_4_UP_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TS_GPIO_4_UP_EVENT_CTRLr, &interval,
                                INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TS_GPIO_4_UP_EVENT_CTRLr(unit, interval);

        /* GPIO_EVENT_5 */
        READ_CMIC_TS_GPIO_5_DOWN_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TS_GPIO_5_DOWN_EVENT_CTRLr, &interval,
                                INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TS_GPIO_5_DOWN_EVENT_CTRLr(unit, interval);
        READ_CMIC_TS_GPIO_5_UP_EVENT_CTRLr(unit, &interval);
        soc_reg_field_set(unit, CMIC_TS_GPIO_5_UP_EVENT_CTRLr, &interval,
                                INTERVAL_LENGTHf, threshold);
        WRITE_CMIC_TS_GPIO_5_UP_EVENT_CTRLr(unit, interval);
    }
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */

    return (BCM_E_NONE);
}


/*
 * Function:
 *    _bcm_esw_time_interface_accuracy_time2hw
 * Purpose:
 *    Internal routine used to compute HW accuracy value from interface 
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    id             - (IN) Interface id 
 *    accuracy       - (OUT) HW value to be programmed 
 * Returns:
 *    BCM_E_XXX
 */
STATIC int 
_bcm_esw_time_interface_accuracy_time2hw(int unit, bcm_time_if_t id, 
                                         uint32 *accuracy)
{
    int                     idx;    /* accuracy itterator */
    bcm_time_interface_t    *intf;

    if (NULL == accuracy) {
        return BCM_E_PARAM;
    }

    intf = TIME_INTERFACE(unit, id);

    /* Find the right accuracy */
    for (idx = 0; idx < TIME_ACCURACY_CLK_MAX; idx++) {
        if (intf->accuracy.nanoseconds <= _bcm_time_accuracy_arr[idx].nanoseconds && 
             COMPILER_64_LO(intf->accuracy.seconds) <= COMPILER_64_LO(_bcm_time_accuracy_arr[idx].seconds) ) {
            break;
        }
    }
    /* if no match - return error */
    if (idx == TIME_ACCURACY_CLK_MAX) {
        return BCM_E_NOT_FOUND;
    }

    /* Return the correct HW value */

    *accuracy = TIME_ACCURACY_SW_IDX_2_HW(idx);

    return (BCM_E_NONE);
}

/*
 * Function:
 *    _bcm_esw_time_interface_drift_install
 * Purpose:
 *    Internal routine used to install interface drift into a HW 
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    id             - (IN) Interface id to be installed into a HW
 * Returns:
 *    BCM_E_XXX
 */
STATIC int 
_bcm_esw_time_interface_drift_install(int unit, bcm_time_if_t id)
{
    uint32 regval, hw_val, sign;
    bcm_time_interface_t    *intf;

#if defined(BCM_TIME_V3_SUPPORT)
    /* Not supported in 3rd generation time arch */
    if (soc_feature(unit, soc_feature_time_v3)) {
        return BCM_E_UNAVAIL;
    }
#endif /* defined(BCM_TIME_V3_SUPPORT) */
    
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    /* Katana does not support clock drift */
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        return BCM_E_UNAVAIL;
    }
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */

    intf = TIME_INTERFACE(unit, id);
    sign = intf->drift.isnegative;

    /* Requested drift value should not be more then 1/8 of drift denominator */
    if (intf->drift.nanoseconds <= TIME_DRIFT_MAX) {
        hw_val = 8 * intf->drift.nanoseconds;
    } else {
        return (BCM_E_PARAM);
    }
    READ_CMIC_BS_DRIFT_RATEr(unit, &regval);
    soc_reg_field_set(unit, CMIC_BS_DRIFT_RATEr, &regval, SIGNf, sign);
    soc_reg_field_set(unit, CMIC_BS_DRIFT_RATEr, &regval, FRAC_NSf, hw_val);
    WRITE_CMIC_BS_DRIFT_RATEr(unit, regval);

    return (BCM_E_NONE);
}

/*
 * Function:
 *    _bcm_esw_time_interface_offset_install
 * Purpose:
 *    Internal routine used to install interface offset into a HW 
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    id             - (IN) Interface id to be installed into a HW
 * Returns:
 *    BCM_E_XXX
 */
STATIC int 
_bcm_esw_time_interface_offset_install(int unit, bcm_time_if_t id)
{
    uint32 regval, sign, hw_val;
    bcm_time_interface_t    *intf;

#if defined(BCM_TIME_V3_SUPPORT)
    /* Not supported in 3rd generation time arch */
    if (soc_feature(unit, soc_feature_time_v3)) {
        return BCM_E_UNAVAIL;
    }
#endif /* defined(BCM_TIME_V3_SUPPORT) */
    
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    /* Katana does not support clock offset */
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit) || SOC_IS_HELIX4(unit)) {
        return BCM_E_UNAVAIL;
    }
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_HELIX4_SUPPORT) */

    intf = TIME_INTERFACE(unit, id);
    
    /* Negative value if local clock is faster and need negative adjustment */
    sign = intf->offset.isnegative;
    /* Write second's values into the HW */
    READ_CMIC_BS_OFFSET_ADJUST_0r(unit, &regval);
    hw_val = COMPILER_64_LO(intf->offset.seconds);
    soc_reg_field_set(unit, CMIC_BS_OFFSET_ADJUST_0r, &regval, SECONDf, hw_val);
    WRITE_CMIC_BS_OFFSET_ADJUST_0r(unit, regval);
    /* Write sign and nansecond's values into the HW */
    READ_CMIC_BS_OFFSET_ADJUST_1r(unit, &regval);
    soc_reg_field_set(unit, CMIC_BS_OFFSET_ADJUST_1r, &regval, 
                      SIGN_BITf, sign);
    soc_reg_field_set(unit, CMIC_BS_OFFSET_ADJUST_1r, &regval, NSf, 
                      intf->offset.nanoseconds);
    WRITE_CMIC_BS_OFFSET_ADJUST_1r(unit, regval);

    /* If necessary install the epoch */
    if (COMPILER_64_HI(intf->offset.seconds) > 0) {
        READ_CMIC_BS_CONFIGr(unit, &regval);
        hw_val = COMPILER_64_HI(intf->offset.seconds);
        if (hw_val > TIME_EPOCH_MAX) {
            hw_val = TIME_EPOCH_MAX;
        }
        soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, EPOCHf, hw_val);
        WRITE_CMIC_BS_CONFIGr(unit, regval);
    }

    return (BCM_E_NONE);
}

#if defined(BCM_TIME_V3_SUPPORT) || defined(BCM_KATANA_SUPPORT) || \
    defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
/*
 * Function:
 *    _bcm_esw_time_interface_ref_clock_install
 * Purpose:
 *    Internal routine to install timesync clock divisor to 
 *    enable broadsync reference clock.
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    id             - (IN) Interface id to be installed into a HW
 * Returns:
 *    BCM_E_XXX
 */
STATIC int 
_bcm_esw_time_interface_ref_clock_install(int unit, bcm_time_if_t id)
{
    uint32 regval, ndiv, hdiv, ref_clk;
    bcm_time_interface_t    *intf;

    intf = TIME_INTERFACE(unit, id);
    
    /* Validate and calculate ts_clk divisor to generate reference clock */
    if (intf->clk_resolution <= 0) {
        return BCM_E_PARAM;
    }
    
    /* Maximum ts_clk frequency is of 25Mhz(40ns) */
    ref_clk = (intf->clk_resolution > TIME_TS_CLK_FREQUENCY_40NS) ?
              TIME_TS_CLK_FREQUENCY_40NS : intf->clk_resolution;
   
    /* Divisor is half period for reference clock */ 
    ndiv = TIME_TS_CLK_FREQUENCY_40NS / ref_clk;
    
    /* Divisor is ceiling of half period */
    hdiv = (ndiv + 1)/2 ? ndiv : 1;

    /* Enable Broadsync reference clock */
#if defined(BCM_TIME_V3_SUPPORT) || defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3) || SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        READ_CMIC_BROADSYNC_REF_CLK_GEN_CTRLr(unit, &regval); 
        soc_reg_field_set(unit, CMIC_BROADSYNC_REF_CLK_GEN_CTRLr, &regval, 
                          ENABLEf, 1);
        soc_reg_field_set(unit, CMIC_BROADSYNC_REF_CLK_GEN_CTRLr, &regval, 
                          DIVISORf, hdiv);
        WRITE_CMIC_BROADSYNC_REF_CLK_GEN_CTRLr(unit, regval);
    } else 
#endif /* defined(BCM_TIME_V3_SUPPORT) */
    {
        READ_CMIC_BS_REF_CLK_GEN_CTRLr(unit, &regval); 
        soc_reg_field_set(unit, CMIC_BS_REF_CLK_GEN_CTRLr, &regval, 
                          ENABLEf, 1);
        soc_reg_field_set(unit, CMIC_BS_REF_CLK_GEN_CTRLr, &regval, 
                          DIVISORf, hdiv);
        WRITE_CMIC_BS_REF_CLK_GEN_CTRLr(unit, regval);
    }

    return (BCM_E_NONE);
}
#endif /* BCM_TIME_V3_SUPPORT || KATANA || TRIUMPH3 || TRIDENT2 */

/*
 * Function:
 *    _bcm_esw_time_interface_install
 * Purpose:
 *    Internal routine used to install interface settings into a HW 
 * Parameters:
 *    unit           - (IN) BCM device number.
 *    intf_id        - (IN) Interface id to be installed into a HW
 * Returns:
 *    BCM_E_XXX
 */
STATIC int
_bcm_esw_time_interface_install(int unit, bcm_time_if_t intf_id)
{
    bcm_time_interface_t    *intf;  /* Time interface */
    uint32                  regval; /* For register read and write operations */
    uint32                  hw_val; /* Value to program into a HW */
    uint32                  second, diff, delay = BROAD_SYNC_OUTPUT_TOGGLE_TIME_DELAY;
    int                     enable = 0;
    soc_reg_t               reg; 
    bcm_time_spec_t         toggle_time;

    if (NULL == TIME_INTERFACE(unit, intf_id)) {
        return BCM_E_PARAM;
    }

    intf = TIME_INTERFACE(unit, intf_id);

    READ_CMIC_BS_CONFIGr(unit, &regval);

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        if ((intf->flags & BCM_TIME_ENABLE)) {
            /* Enable all three broadsync pins:
             *     Bit clock, HeartBeat and Timecode 
             */
            soc_reg_field_set(
                    unit, CMIC_BS_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, 1);
            soc_reg_field_set(
                    unit, CMIC_BS_CONFIGr, &regval, BS_HB_OUTPUT_ENABLEf, 1);
            soc_reg_field_set(
                    unit, CMIC_BS_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, 1);
        } else {
            soc_reg_field_set(
                    unit, CMIC_BS_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, 0);
            soc_reg_field_set(
                    unit, CMIC_BS_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, 0);
        }

        if ((intf->flags & BCM_TIME_LOCKED)) {
            soc_reg_field32_modify(
                    unit, CMIC_BS_OUTPUT_TIME_0r, REG_PORT_ANY, LOCKf, 1);
        } else {
            soc_reg_field32_modify(
                    unit, CMIC_BS_OUTPUT_TIME_0r, REG_PORT_ANY, LOCKf, 0);
        }

    } else 
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */
    {
        if (intf->flags & BCM_TIME_ENABLE) {
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, ENABLEf, 1);
            soc_reg_field_set(
                unit, CMIC_BS_CONFIGr, &regval, TIME_CODE_ENABLEf, 1);
        } else {
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, ENABLEf, 0);
            soc_reg_field_set(
                unit, CMIC_BS_CONFIGr, &regval, TIME_CODE_ENABLEf, 0);
        }
    
        if ((intf->flags & BCM_TIME_LOCKED)) {
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, LOCKf, 1);
        } else {
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, LOCKf, 0);
        }
    }

    if (intf->flags & BCM_TIME_INPUT) {
        /* Set Broadsync in Input mode */
        soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, MODEf, 
                          TIME_MODE_INPUT);
    } else {
        /* Configure Accuracy, offset and drift for broadsync output mode */
        soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, MODEf, 
                          TIME_MODE_OUTPUT);
        if (intf->flags & BCM_TIME_ACCURACY) {
            BCM_IF_ERROR_RETURN(
                _bcm_esw_time_interface_accuracy_time2hw(unit, intf_id, &hw_val));
            if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                SOC_IS_TD2_TT2(unit)) {
                soc_reg_field32_modify(unit, CMIC_BS_OUTPUT_TIME_0r, 
                                       REG_PORT_ANY, ACCURACYf, hw_val);
            } else {
                soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, ACCURACYf, hw_val);
            }
        }
        if (intf->flags & BCM_TIME_HEARTBEAT) {
            BCM_IF_ERROR_RETURN(
                _bcm_esw_time_interface_heartbeat_install(unit, intf_id));

            if (!(intf->flags & BCM_TIME_OFFSET)) {
                BCM_IF_ERROR_RETURN(
                    _bcm_esw_time_capture_get(unit, intf_id, TIME_CAPTURE(unit, intf_id)));
                if (!(SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
                      SOC_IS_TD2_TT2(unit))) {
                    second = COMPILER_64_LO(SYNT_TIME_SECONDS(unit, intf_id));
                    second += delay;
                    WRITE_CMIC_BS_CLK_TOGGLE_TIME_0r(unit, second);
                    WRITE_CMIC_BS_CLK_TOGGLE_TIME_1r(
                                unit, SYNT_TIME_NANOSECONDS(unit, intf_id));
                }
            }
        }
    }
    WRITE_CMIC_BS_CONFIGr(unit, regval);

    if (intf->flags & BCM_TIME_OFFSET) {
        if(SOC_REG_IS_VALID(unit, CMIC_BS_CLK_CTRL_0r)) {
            reg   =   CMIC_BS_CLK_CTRL_0r;
            READ_CMIC_BS_CLK_CTRL_0r(unit, &regval);
        } else {
            reg   =   CMIC_BS_CLK_CTRLr;
            READ_CMIC_BS_CLK_CTRLr(unit, &regval);
        }

        /* Stop broadsync output if already enabled */
        enable = soc_reg_field_get(unit, reg, regval, ENABLEf);
        if (1 == enable) {
            soc_reg_field_set(unit, reg, &regval, ENABLEf, 0);
            if (SOC_REG_IS_VALID(unit, CMIC_BS_CLK_CTRL_0r)) {
                WRITE_CMIC_BS_CLK_CTRL_0r(unit, regval);
            } else {
                WRITE_CMIC_BS_CLK_CTRLr(unit, regval);
            }
            BCM_IF_ERROR_RETURN(
                _bcm_esw_time_capture_get(unit, intf_id, TIME_CAPTURE(unit, intf_id)));
        }

        BCM_IF_ERROR_RETURN(
            _bcm_esw_time_interface_offset_install(unit, intf_id));

        if (intf->offset.isnegative) {
            second = COMPILER_64_LO(SYNT_TIME_SECONDS(unit, intf_id)) -
                     COMPILER_64_LO(intf->offset.seconds);

            if (intf->offset.nanoseconds <= SYNT_TIME_NANOSECONDS(unit, intf_id)) {
                toggle_time.nanoseconds = SYNT_TIME_NANOSECONDS(unit, intf_id) -
                                          intf->offset.nanoseconds;
            } else {
                /* Wrapped */
                diff = intf->offset.nanoseconds - SYNT_TIME_NANOSECONDS(unit, intf_id);
                toggle_time.nanoseconds = TIME_NANOSEC_MAX - diff;
                second--;
            }
            COMPILER_64_SET(toggle_time.seconds, 0, second); 
        } else {
            second = COMPILER_64_LO(SYNT_TIME_SECONDS(unit, intf_id)) +
                     COMPILER_64_LO(intf->offset.seconds);
            toggle_time.nanoseconds = 
                SYNT_TIME_NANOSECONDS(unit, intf_id) + intf->offset.nanoseconds;
            if (toggle_time.nanoseconds >= TIME_NANOSEC_MAX) {
                toggle_time.nanoseconds -= TIME_NANOSEC_MAX;
                second++;
            }
            COMPILER_64_SET(toggle_time.seconds, 0, second); 
        }

        if (1 == enable) {
            /* Reenable broadsync output */
            COMPILER_64_TO_32_LO(second, toggle_time.seconds);
            second += delay;
            if (SOC_REG_IS_VALID(unit, CMIC_BS_CLK_TOGGLE_TIME_0r)) {
                WRITE_CMIC_BS_CLK_TOGGLE_TIME_0r(unit, second);
            }

            if (SOC_REG_IS_VALID(unit, CMIC_BS_CLK_TOGGLE_TIME_1r)) {
                WRITE_CMIC_BS_CLK_TOGGLE_TIME_1r(unit, toggle_time.nanoseconds);
            }
            
            if (SOC_REG_IS_VALID(unit, CMIC_BS_CLK_TOGGLE_TIME_1r)) {
                READ_CMIC_BS_CLK_CTRL_0r(unit, &regval);
                soc_reg_field_set(unit, CMIC_BS_CLK_CTRL_0r, &regval, ENABLEf, 1);
                WRITE_CMIC_BS_CLK_CTRL_0r(unit, regval);
            } else {
                READ_CMIC_BS_CLK_CTRLr(unit, &regval);
                soc_reg_field_set(unit, CMIC_BS_CLK_CTRLr, &regval, ENABLEf, 1);
                WRITE_CMIC_BS_CLK_CTRLr(unit, regval);
            }
        }
    }

    if (intf->flags & BCM_TIME_DRIFT) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_time_interface_drift_install(unit, intf_id));
    }

    return (BCM_E_NONE);
}

#if defined(BCM_TIME_V3_SUPPORT) || defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
STATIC int
_bcm_esw_time_interface_dual_bs_install(int unit, bcm_time_if_t intf_id)
{
    bcm_time_interface_t    *intf;  /* Time interface */
    uint32                  regval; /* For register read and write operations */
    uint32                  hw_val; /* Value to program into a HW */

    if (NULL == TIME_INTERFACE(unit, intf_id)) {
        return BCM_E_PARAM;
    }

    intf = TIME_INTERFACE(unit, intf_id);

    READ_CMIC_BS0_CONFIGr(unit, &regval);

    if ((intf->flags & BCM_TIME_LOCKED)) {
        soc_reg_field32_modify(unit, CMIC_BS0_OUTPUT_TIME_0r, REG_PORT_ANY, LOCKf, 1);
    } else {
        soc_reg_field32_modify(unit, CMIC_BS0_OUTPUT_TIME_0r, REG_PORT_ANY, LOCKf, 0);
    }

    if (intf->flags & BCM_TIME_INPUT) {
        /* Set Broadsync in Input mode */
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, MODEf, TIME_MODE_INPUT);

        /* ensure all output controls are disabled in input mode */
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, 0);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, 0);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_HB_OUTPUT_ENABLEf, 0);
    } else {
        /* Configure Accuracy, offset and drift for broadsync output mode */
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, MODEf, TIME_MODE_OUTPUT);

        if ((intf->flags & BCM_TIME_ENABLE)) {
            /* Enable all three broadsync pins: Bit clock, HeartBeat and Timecode */
            soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, 1);
            soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, 1);
            soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_HB_OUTPUT_ENABLEf, 1);
        } else {
            /* ensure all output controls are disabled */
            soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, 0);
            soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, 0);
            soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_HB_OUTPUT_ENABLEf, 0);
        }

        if (intf->flags & BCM_TIME_ACCURACY) {
            BCM_IF_ERROR_RETURN(
                _bcm_esw_time_interface_accuracy_time2hw(unit, intf_id, &hw_val));
            soc_reg_field32_modify(unit, CMIC_BS0_OUTPUT_TIME_0r, REG_PORT_ANY, ACCURACYf, hw_val);
        }

        if (intf->flags & BCM_TIME_HEARTBEAT) {
            BCM_IF_ERROR_RETURN(
                _bcm_esw_time_interface_heartbeat_install(unit, intf_id));
        }
    }

    WRITE_CMIC_BS0_CONFIGr(unit, regval);

    return (BCM_E_NONE);
}
#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */


/*
 * Function:
 *     _bcm_time_interface_free
 * Purpose:
 *     Free time interface.
 * Parameters:
 *      unit            - (IN) BCM device number. 
 *      intf_id         - (IN) time interface id.
 * Returns:
 *      BCM_X_XXX
 */

STATIC int 
_bcm_time_interface_free(int unit, bcm_time_if_t intf_id) 
{
    _bcm_time_interface_config_p  intf_cfg; /* Time interface config.*/

    intf_cfg = &TIME_INTERFACE_CONFIG(unit, intf_id); 

    if (intf_cfg->ref_count > 0) {
        intf_cfg->ref_count--;
    }

    if (0 == intf_cfg->ref_count) {
        sal_memset(&intf_cfg->time_interface, 0, sizeof(bcm_time_interface_t));
        intf_cfg->time_interface.id = intf_id;
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_esw_time_accuracy_parse
 * Purpose:
 *      Internal routine to parse accuracy hw value into bcm_time_spec_t format
 * Parameters:
 *      unit        - (IN) StrataSwitch Unit #.
 *      accuracy    - (IN) Accuracy HW value 
 *      time        - (OUT) bcm_time_spec_t structure to contain accuracy 
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
STATIC int 
_bcm_esw_time_accuracy_parse(int unit, uint32 accuracy, bcm_time_spec_t *time)
{
    if (accuracy < TIME_ACCURACY_LOW_HW_VAL || 
        (accuracy > TIME_ACCURACY_HIGH_HW_VAL && 
         accuracy != TIME_ACCURACY_UNKNOWN_HW_VAL)) {
        return (BCM_E_PARAM);
    }

    time->isnegative = 
        _bcm_time_accuracy_arr[TIME_ACCURACY_HW_2_SW_IDX(accuracy)].isnegative;
    time->nanoseconds = 
        _bcm_time_accuracy_arr[TIME_ACCURACY_HW_2_SW_IDX(accuracy)].nanoseconds;
    time->seconds = 
        _bcm_time_accuracy_arr[TIME_ACCURACY_HW_2_SW_IDX(accuracy)].seconds;
    time->isnegative = 0;   /* Can't be negative */

    return (BCM_E_NONE);
}


/*
 * Function:
 *      _bcm_esw_time_input_parse
 * Purpose:
 *      Internal routine to parse input time information stored in 3 registeres
 * Parameters:
 *      unit    - (IN) StrataSwitch Unit #.
 *      data0   - (IN) Data stored in register 0 conrain input time information
 *      data1   - (IN) Data stored in register 1 conrain input time information
 *      data2   - (IN) Data stored in register 2 conrain input time information
 *      time    - (OUT) Structure to contain input time information
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
STATIC int
_bcm_esw_time_input_parse(int unit, uint32 data0, uint32 data1, uint32 data2, 
                          bcm_time_capture_t *time)
{
    uint32 accuracy, sec_hi, sec_lo;

    if (data0 >> 31) {
        time->flags |= BCM_TIME_CAPTURE_LOCKED;
    }
    time->received.isnegative = 0;
    sec_hi = ((data0 << 1) >> 1);
    sec_lo = (data1 >> 14);
    COMPILER_64_SET(time->received.seconds, sec_hi, sec_lo);  
    time->received.nanoseconds = (data1 << 16);
    time->received.nanoseconds |= (data2 >> 8);

    accuracy = ((data2 << 24) >> 24);

    return _bcm_esw_time_accuracy_parse(unit,accuracy, 
                                        &(time->received_accuracy));
}

/*
 * Function:
 *      _bcm_esw_time_capture_counter_read
 * Purpose:
 *      Internal routine to read HW clocks
 * Parameters:
 *      unit    - (IN) StrataSwitch Unit #.
 *      id      - (IN) Time interface identifier
 *      time    - (OUT) Structure to contain HW clocks values
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */

STATIC int 
_bcm_esw_time_capture_counter_read(int unit, bcm_time_if_t id, 
                                   bcm_time_capture_t *time)
{
    uint32                  regval;
    bcm_time_interface_t    *intf;
    
#if defined(BCM_TIME_V3_SUPPORT) || defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3) || SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        uint32 regval2, event_id, lower;

        /* Read timestamp from FIFO */
        sal_memset(time, 0, sizeof(bcm_time_capture_t));
        READ_CMIC_TIMESYNC_CAPTURE_STATUS_1r(unit, &regval);
        if (0 == soc_reg_field_get(unit, CMIC_TIMESYNC_CAPTURE_STATUS_1r, 
                                   regval, FIFO_DEPTHf)) {
            return (BCM_E_EMPTY);
        }
        
        /* Must read LOWER regiser first */
        READ_CMIC_TIMESYNC_INPUT_TIME_FIFO_TS_LOWERr(unit, &regval2);
        
        /* Then read UPPER register */
        READ_CMIC_TIMESYNC_INPUT_TIME_FIFO_TS_UPPERr(unit, &regval);
        
        /* If it's not valid, something is wrong (since FIFO is not empty) */
        if (0 == soc_reg_field_get(
                        unit, CMIC_TIMESYNC_INPUT_TIME_FIFO_TS_UPPERr, regval, 
                        VALIDf)) {
            return (BCM_E_FAIL);
        }
        
        /* Check capture event id */
        event_id = soc_reg_field_get(
                        unit, CMIC_TIMESYNC_INPUT_TIME_FIFO_TS_UPPERr, 
                        regval, EVENT_IDf);
        if (event_id == 0) {
            time->flags |= BCM_TIME_CAPTURE_IMMEDIATE;
        }

        /* Calculate seconds/nanoseconds from the timestamp value */
        lower = soc_reg_field_get(
                        unit, CMIC_TIMESYNC_INPUT_TIME_FIFO_TS_LOWERr,
                        regval2, TIMESTAMPf);
        COMPILER_64_SET(time->synchronous.seconds, 0, lower / 1000000000);
        time->synchronous.nanoseconds = lower % 1000000000;
        
        /* For backward compatibility */
        time->free = time->synchronous;
        time->syntonous = time->synchronous;
        
    } else 
#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_KATANA(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        uint32 event_id;
         
        /* Check FIFO depth */
        sal_memset(time, 0, sizeof(bcm_time_capture_t));
        READ_CMIC_TS_CAPTURE_STATUSr(unit, &regval);
        if (0 == soc_reg_field_get(unit, CMIC_TS_CAPTURE_STATUSr,
                                   regval, FIFO_DEPTHf)) {
            return (BCM_E_EMPTY);
        }

        /* Read timestamp from FIFO */
        READ_CMIC_TS_INPUT_TIME_FIFO_TSr(unit, &regval);
        time->synchronous.nanoseconds =  soc_reg_field_get(unit,
                                          CMIC_TS_INPUT_TIME_FIFO_TSr, regval,
                                          TIMESTAMPf);

        /* If it's not valid, something is wrong (since FIFO is not empty) */
        READ_CMIC_TS_INPUT_TIME_FIFO_IDr(unit, &regval);
        if (0 == soc_reg_field_get(
                        unit, CMIC_TS_INPUT_TIME_FIFO_IDr, regval,
                        VALIDf)) {
            return (BCM_E_FAIL);
        }

        /* Check capture event id */
        event_id = soc_reg_field_get(
                        unit, CMIC_TS_INPUT_TIME_FIFO_IDr,
                        regval, EVENT_IDf);

        if (event_id == 0) {
            time->flags |= BCM_TIME_CAPTURE_IMMEDIATE;
        }
   
        /* For backward compatibility */
        time->free = time->synchronous;
        time->syntonous = time->synchronous;

    } else 
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */
    {
        /* Read free running time capture */
        READ_CMIC_BS_CAPTURE_FREE_RUN_TIME_0r(unit, &regval);
        COMPILER_64_SET(time->free.seconds, 0,  
            soc_reg_field_get(unit, CMIC_BS_CAPTURE_FREE_RUN_TIME_0r, 
                              regval, SECONDf)); 
        READ_CMIC_BS_CAPTURE_FREE_RUN_TIME_1r(unit, &regval);
        time->free.nanoseconds =  
         soc_reg_field_get(unit, CMIC_BS_CAPTURE_FREE_RUN_TIME_1r, regval, NSf); 
        /* Read syntonous time capture */
        READ_CMIC_BS_CAPTURE_SYNT_TIME_0r(unit, &regval);
        COMPILER_64_SET(time->syntonous.seconds, 0,
         soc_reg_field_get(unit, CMIC_BS_CAPTURE_SYNT_TIME_0r, regval, SECONDf));
        READ_CMIC_BS_CAPTURE_SYNT_TIME_1r(unit, &regval);
        time->syntonous.nanoseconds = 
            soc_reg_field_get(unit, CMIC_BS_CAPTURE_SYNT_TIME_1r, regval, NSf);
        /* Read synchronous time capture */
        READ_CMIC_BS_CAPTURE_SYNC_TIME_0r(unit, &regval);
        COMPILER_64_SET(time->synchronous.seconds, 0, 
         soc_reg_field_get(unit, CMIC_BS_CAPTURE_SYNC_TIME_0r, regval, SECONDf));
        READ_CMIC_BS_CAPTURE_SYNC_TIME_1r(unit, &regval);
        time->synchronous.nanoseconds = 
            soc_reg_field_get(unit, CMIC_BS_CAPTURE_SYNC_TIME_1r, regval, NSf);
    }

    time->free.isnegative = time->synchronous.isnegative = 
        time->syntonous.isnegative = 0;

    /* If interface is in input mode read time provided by the master */
    intf = TIME_INTERFACE(unit, id);
    if (intf->flags & BCM_TIME_INPUT) {
        uint32  data0 = 0, data1 = 0, data2 = 0;

#if defined(BCM_TIME_V3_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
        if (soc_feature(unit, soc_feature_time_v3) || 
            SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
            if (!soc_feature(unit, soc_feature_time_v3_no_bs)) {
                READ_CMIC_BS0_INPUT_TIME_2r(unit, &regval);
                if (soc_reg_field_get(unit, CMIC_BS0_INPUT_TIME_2r, regval, 
                                      CHECKSUM_ERRORf)) {
                    return BCM_E_INTERNAL;
                }
                data2 = soc_reg_field_get(
                            unit, CMIC_BS0_INPUT_TIME_2r, regval, DATAf);
                READ_CMIC_BS0_INPUT_TIME_1r(unit, &regval);
                data1 = soc_reg_field_get(
                            unit, CMIC_BS0_INPUT_TIME_1r, regval, DATAf);
                READ_CMIC_BS0_INPUT_TIME_0r(unit, &regval);
                data0 = soc_reg_field_get(
                            unit, CMIC_BS0_INPUT_TIME_0r, regval, DATAf);
            }
        } else
#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */
        {
            READ_CMIC_BS_INPUT_TIME_2r(unit, &regval);
            if (soc_reg_field_get(unit, CMIC_BS_INPUT_TIME_2r, regval, 
                                  CHECKSUM_ERRORf)) {
                return BCM_E_INTERNAL;
            }
            data2 = soc_reg_field_get(unit, CMIC_BS_INPUT_TIME_2r, regval, DATAf);
            READ_CMIC_BS_INPUT_TIME_1r(unit, &regval);
            data1 = soc_reg_field_get(unit, CMIC_BS_INPUT_TIME_1r, regval, DATAf);
            READ_CMIC_BS_INPUT_TIME_0r(unit, &regval);
            data0 = soc_reg_field_get(unit, CMIC_BS_INPUT_TIME_0r, regval, DATAf);
        }

        BCM_IF_ERROR_RETURN(
            _bcm_esw_time_input_parse(unit, data0, data1, data2, time));
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_esw_time_capture_get
 * Purpose:
 *      Internal routine to read HW clocks
 * Parameters:
 *      unit    - (IN) StrataSwitch Unit #.
 *      id      - (IN) Time interface identifier
 *      time    - (OUT) Structure to contain HW clocks values
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
STATIC int 
_bcm_esw_time_capture_get (int unit, bcm_time_if_t id, bcm_time_capture_t *time)
{
    uint32          regval, orgval; /* To keep register value */
    int             hw_complete;    /* HW read completion indicator*/
    soc_timeout_t   timeout;        /* Timeout in case of HW error */

#if defined(BCM_TIME_V3_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3) || 
        SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {

        /* Read capture mode */
        READ_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, &regval);
        orgval = soc_reg_field_get(unit, CMIC_TIMESYNC_TIME_CAPTURE_MODEr, 
                                   regval, TIME_CAPTURE_MODEf);

        /* If any trigger condition is set, get the timestamp from FIFO */
        if (orgval != TIME_CAPTURE_MODE_DISABLE) {
            return _bcm_esw_time_capture_counter_read(unit, id, time);
        }

        /* Otherwise, do an immediate capture */
        
        /* Make sure timestamp capture is enabled */
        READ_CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr(unit, &regval);
        if (0 == soc_reg_field_get(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, 
                                   regval, TIME_CAPTURE_ENABLEf)) {
            soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, 
                            &regval, TIME_CAPTURE_ENABLEf, 1 );
            WRITE_CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr(unit, regval);
        }

        /* Clear the FIFO if not empty */
        for(;;) {
            READ_CMIC_TIMESYNC_CAPTURE_STATUS_1r(unit, &regval);
            if (0 == soc_reg_field_get(unit, CMIC_TIMESYNC_CAPTURE_STATUS_1r, 
                                       regval, FIFO_DEPTHf)) {
                break;
            }
            /* There is still something in the FIFO, pop it */
            READ_CMIC_TIMESYNC_INPUT_TIME_FIFO_TS_LOWERr(unit, &regval);
            READ_CMIC_TIMESYNC_INPUT_TIME_FIFO_TS_UPPERr(unit, &regval);
        }
        
        /* Make sure the capture status is cleared */
        READ_CMIC_TIMESYNC_CAPTURE_STATUS_1r(unit, &regval);
        if (regval != 0) {
            soc_reg_field_set(unit, CMIC_TIMESYNC_CAPTURE_STATUS_CLR_1r, 
                              &regval, TIME_CAPTURE_COMPLETE_CLRf, 1);
            soc_reg_field_set(unit, CMIC_TIMESYNC_CAPTURE_STATUS_CLR_1r, 
                              &regval, FIFO_WRITE_COMPLETE_CLRf, 1);
            WRITE_CMIC_TIMESYNC_CAPTURE_STATUS_CLR_1r(unit, regval);
        }
        
        /* Program HW to capture time immediately */
        READ_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_MODEr, &regval, 
                          TIME_CAPTURE_MODEf, TIME_CAPTURE_MODE_IMMEDIATE);
        WRITE_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, regval);

        /* Wait for HW time capture completion */
        hw_complete = 0;
        soc_timeout_init(&timeout, BROAD_SYNC_TIME_CAPTURE_TIMEOUT, 0);
        while (!hw_complete) {
            READ_CMIC_TIMESYNC_CAPTURE_STATUS_1r(unit, &regval);
            hw_complete = soc_reg_field_get(unit, CMIC_TIMESYNC_CAPTURE_STATUS_1r,
                                            regval, TIME_CAPTURE_COMPLETEf); 
            if (soc_timeout_check(&timeout)) {
                return (BCM_E_TIMEOUT);
            }
        }

        /* Program HW to disable time capture */
        READ_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_MODEr, &regval, 
                            TIME_CAPTURE_MODEf, TIME_CAPTURE_MODE_DISABLE);
        WRITE_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, regval);
        
        /* Read the HW time */
        BCM_IF_ERROR_RETURN(
            _bcm_esw_time_capture_counter_read(unit, id, time));

    } else
#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */
#if (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT))
    if (SOC_IS_KATANA(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        /* Read capture mode */
        READ_CMIC_TS_TIME_CAPTURE_CTRLr(unit, &regval);
        /* If any trigger condition is set, get the timestamp from FIFO */
        if (TIME_CAPTURE_MODE_DISABLE != soc_reg_field_get(unit,
                   CMIC_TS_TIME_CAPTURE_CTRLr, regval, TIME_CAPTURE_MODEf) ) {
            return _bcm_esw_time_capture_counter_read(unit, id, time);
        }

        /* if different than interrupt remember original capture mode */
        orgval = regval;

        /* Program HW to disable time capture */
        soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, 
                            TIME_CAPTURE_MODEf, TIME_CAPTURE_MODE_DISABLE);
        if (orgval != regval) {
            WRITE_CMIC_TS_TIME_CAPTURE_CTRLr(unit, regval);
        }
        
        /* Make sure timestamp capture is enabled */
        READ_CMIC_TS_TIME_CAPTURE_CTRLr(unit, &regval);
        if (0 == soc_reg_field_get(unit, CMIC_TS_TIME_CAPTURE_CTRLr,
                                   regval, TIME_CAPTURE_ENABLEf)) {
            soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr,
                            &regval, TIME_CAPTURE_ENABLEf, 1 );
            WRITE_CMIC_TS_TIME_CAPTURE_CTRLr(unit, regval);
        }       

        /* Clear the FIFO if not empty */
        for(;;) {
            READ_CMIC_TS_CAPTURE_STATUSr(unit, &regval);
            if (0 == soc_reg_field_get(unit, CMIC_TS_CAPTURE_STATUSr,
                                       regval, FIFO_DEPTHf)) {
                break;
            }
            /* There is still something in the FIFO, pop it */
            READ_CMIC_TS_INPUT_TIME_FIFO_TSr(unit, &regval);
        }

        /* Make sure the capture status is cleared */
        READ_CMIC_TS_CAPTURE_STATUSr(unit, &regval);
        if (regval != 0) {
            soc_reg_field_set(unit, CMIC_TS_CAPTURE_STATUS_CLRr,
                              &regval, TIME_CAPTURE_COMPLETE_CLRf, 1);
            soc_reg_field_set(unit, CMIC_TS_CAPTURE_STATUS_CLRr,
                              &regval, FIFO_WRITE_COMPLETE_CLRf, 1);
            WRITE_CMIC_TS_CAPTURE_STATUS_CLRr(unit, regval);
        }
 
        /* Program HW to capture time immediately */
        READ_CMIC_TS_TIME_CAPTURE_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, 
                        TIME_CAPTURE_MODEf,TIME_CAPTURE_MODE_IMMEDIATE);
        WRITE_CMIC_TS_TIME_CAPTURE_CTRLr(unit, regval);

        /* Wait for HW time capture completion */
        hw_complete = 0;
        soc_timeout_init(&timeout, BROAD_SYNC_TIME_CAPTURE_TIMEOUT, 0);

        while (!hw_complete) {
            READ_CMIC_TS_CAPTURE_STATUSr(unit, &regval);
            hw_complete = soc_reg_field_get(unit, CMIC_TS_CAPTURE_STATUSr,
                                            regval, TIME_CAPTURE_COMPLETEf); 
            if (soc_timeout_check(&timeout)) {
                return (BCM_E_TIMEOUT);
            }
        }
    
        /* Read the HW time */
        BCM_IF_ERROR_RETURN(
            _bcm_esw_time_capture_counter_read(unit, id, time));

        /* Program HW to original time capture value */
        WRITE_CMIC_TS_TIME_CAPTURE_CTRLr(unit, orgval);

    } else
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */
    {
        /* Read capture mode */
        READ_CMIC_BS_CAPTURE_CTRLr(unit, &regval);
        /* If in interrupt mode, return time captured on last interrupt */
        if (TIME_CAPTURE_MODE_HEARTBEAT == soc_reg_field_get(unit, 
                            CMIC_BS_CAPTURE_CTRLr, regval, TIME_CAPTURE_MODEf)) {
            return _bcm_esw_time_capture_counter_read(unit, id, time);
        }
        /* if different than interrupt remember original capture mode */
        orgval = regval;

        /* Program HW to disable time capture */
        soc_reg_field_set(unit, CMIC_BS_CAPTURE_CTRLr, &regval, TIME_CAPTURE_MODEf,
                          TIME_CAPTURE_MODE_DISABLE);
        if (orgval != regval) {
            WRITE_CMIC_BS_CAPTURE_CTRLr(unit, regval);
        }
        
        /* Program HW to capture time immediately */
        soc_reg_field_set(unit, CMIC_BS_CAPTURE_CTRLr, &regval, TIME_CAPTURE_MODEf,
                          TIME_CAPTURE_MODE_IMMEDIATE);
        WRITE_CMIC_BS_CAPTURE_CTRLr(unit, regval);

        /* Wait for HW time capture completion */
        hw_complete = 0;
        soc_timeout_init(&timeout, BROAD_SYNC_TIME_CAPTURE_TIMEOUT, 0);

        while (!hw_complete) {
            READ_CMIC_BS_CAPTURE_STATUSr(unit, &regval);
            hw_complete = soc_reg_field_get(unit, CMIC_BS_CAPTURE_STATUSr, regval,
                                            TIME_CAPTURE_COMPLETEf); 
            if (soc_timeout_check(&timeout)) {
                return (BCM_E_TIMEOUT);
            }
        }
        /* Read the HW time */
        BCM_IF_ERROR_RETURN(
            _bcm_esw_time_capture_counter_read(unit, id, time));

        /* Program HW to original time capture value */
        WRITE_CMIC_BS_CAPTURE_CTRLr(unit, orgval);
    }

    return BCM_E_NONE;
}

/*
 * Function:
 *      _bcm_esw_time_interface_offset_get
 * Purpose:
 *      Internal routine to read HW offset value and convert it into 
 *      bcm_time_spec_t structure 
 * Parameters:
 *      unit    -  (IN) StrataSwitch Unit #.
 *      id      -  (IN) Time interface identifier
 *      offset  - (OUT) Time interface  offset 
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
STATIC int 
_bcm_esw_time_interface_offset_get(int unit, bcm_time_if_t id, 
                                   bcm_time_spec_t *offset)
{
    uint32 regval, sec, epoch;

#if defined(BCM_TIME_V3_SUPPORT)
    /* Not supported in 3rd generation time arch */
    if (soc_feature(unit, soc_feature_time_v3)) {
        return BCM_E_UNAVAIL;
    }
#endif /* defined(BCM_TIME_V3_SUPPORT) */
    
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    /* Katana does not support clock offset */
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        return BCM_E_UNAVAIL;
    }
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */

    READ_CMIC_BS_OFFSET_ADJUST_0r(unit, &regval);
    sec = soc_reg_field_get(unit, CMIC_BS_OFFSET_ADJUST_0r, regval, SECONDf);
    READ_CMIC_BS_OFFSET_ADJUST_1r(unit, &regval);
    offset->nanoseconds= soc_reg_field_get(unit, CMIC_BS_OFFSET_ADJUST_1r,
                                           regval, NSf); 
    offset->isnegative = soc_reg_field_get(unit, CMIC_BS_OFFSET_ADJUST_1r, 
                                           regval, SIGN_BITf);
    /* If the epoch was installed read it */
    READ_CMIC_BS_CONFIGr(unit, &regval);
    epoch = soc_reg_field_get(unit,CMIC_BS_CONFIGr, regval, EPOCHf);

    COMPILER_64_SET(offset->seconds, epoch, sec);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      _bcm_esw_time_interface_drift_get
 * Purpose:
 *      Internal routine to read HW drift value and convert it into 
 *      bcm_time_spec_t structure 
 * Parameters:
 *      unit    -  (IN) StrataSwitch Unit #.
 *      id      -  (IN) Time interface identifier
 *      drift   - (OUT) Time interface  drift 
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
STATIC int 
_bcm_esw_time_interface_drift_get(int unit, bcm_time_if_t id,
                                  bcm_time_spec_t *drift)
{
    uint32 regval, val, ns;

#if defined(BCM_TIME_V3_SUPPORT)
    /* Not supported in 3rd generation time arch */
    if (soc_feature(unit, soc_feature_time_v3)) {
        return BCM_E_UNAVAIL;
    }
#endif /* defined(BCM_TIME_V3_SUPPORT) */
    
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    /* Katana does not support clock drift */
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        return BCM_E_UNAVAIL;
    }
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */

    
    READ_CMIC_BS_DRIFT_RATEr(unit, &regval);
    val = soc_reg_field_get(unit, CMIC_BS_DRIFT_RATEr, regval, FRAC_NSf);

    ns = (val / 8);

    drift->nanoseconds = (ns > TIME_DRIFT_MAX) ? TIME_DRIFT_MAX : ns;

    drift->isnegative = soc_reg_field_get(unit, CMIC_BS_DRIFT_RATEr, 
                                          regval, SIGNf);
    return (BCM_E_NONE);
}

#if defined(BCM_TIME_V3_SUPPORT) || defined(BCM_KATANA_SUPPORT) || \
    defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
/*
 * Function:
 *      _bcm_esw_time_interface_ref_clock_get
 * Purpose:
 *      Internal routine to read ref clock divisor 
 * Parameters:
 *      unit    -  (IN) StrataSwitch Unit #.
 *      id      -  (IN) Time interface identifier
 *      divisor - (OUT) Time interface  ref clock resolution 
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
STATIC int 
_bcm_esw_time_interface_ref_clock_get(int unit, bcm_time_if_t id,
                                  int *clk_resolution)
{
    uint32 regval, val, hdiv = 0;
   
#if defined(BCM_TIME_V3_SUPPORT) || defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3) || SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        READ_CMIC_BROADSYNC_REF_CLK_GEN_CTRLr(unit, &regval); 
        val = soc_reg_field_get(unit, CMIC_BROADSYNC_REF_CLK_GEN_CTRLr, 
                                regval, ENABLEf);
        hdiv = soc_reg_field_get(unit, CMIC_BROADSYNC_REF_CLK_GEN_CTRLr, 
                                 regval, DIVISORf);
    } else
#endif /* defined(BCM_TIME_V3_SUPPORT) */
    {
        READ_CMIC_BS_REF_CLK_GEN_CTRLr(unit, &regval); 
        val = soc_reg_field_get(unit, CMIC_BS_REF_CLK_GEN_CTRLr, regval, 
                                ENABLEf);
        hdiv = soc_reg_field_get(unit, CMIC_BS_REF_CLK_GEN_CTRLr, regval, 
                                 DIVISORf);
    }

    if (val) {
        /* Divisor is half period for reference clock */ 
        *clk_resolution = TIME_TS_CLK_FREQUENCY_40NS / (hdiv * 2);
    } else {
        *clk_resolution = 0;
    }

    return (BCM_E_NONE);
}
#endif /* BCM_TIME_V3_SUPPORT || KATANA || TRIUMPH3 || TRIDENT2 */

/*
 * Function:
 *      _bcm_esw_time_interface_accuracy_get
 * Purpose:
 *      Internal routine to read HW accuracy value and convert it into 
 *      bcm_time_spec_t structure 
 * Parameters:
 *      unit        -  (IN) StrataSwitch Unit #.
 *      id          -  (IN) Time interface identifier
 *      accuracy    - (OUT) Time interface  accuracy 
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
STATIC int 
_bcm_esw_time_interface_accuracy_get(int unit, bcm_time_if_t id,
                                     bcm_time_spec_t *accuracy)
{
    uint32  regval, val; 

#if defined(BCM_TIME_V3_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3) || 
        SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        READ_CMIC_BS0_OUTPUT_TIME_0r(unit, &regval);
        val = soc_reg_field_get(unit, CMIC_BS0_OUTPUT_TIME_0r, regval, ACCURACYf);
    } else
#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        READ_CMIC_BS_OUTPUT_TIME_0r(unit, &regval);
        val = soc_reg_field_get(unit, CMIC_BS_OUTPUT_TIME_0r, regval, ACCURACYf);
    } else 
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */
    {
        READ_CMIC_BS_CONFIGr(unit, &regval);
        val = soc_reg_field_get(unit, CMIC_BS_CONFIGr, regval, ACCURACYf);
    }

    return _bcm_esw_time_accuracy_parse(unit, val, accuracy);
}

/*
 * Function:
 *      _bcm_esw_time_interface_get
 * Purpose:
 *      Internal routine to get a time sync interface by id
 * Parameters:
 *      unit -  (IN) StrataSwitch Unit #.
 *      id   -  (IN) Time interface identifier
 *      intf - (IN/OUT) Time Sync Interface to get
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
STATIC int 
_bcm_esw_time_interface_get(int unit, bcm_time_if_t id, bcm_time_interface_t *intf)
{
    uint32                  regval;
    bcm_time_interface_t    *intf_ptr;
    uint32                  orig_flags;

    intf_ptr = TIME_INTERFACE(unit, id);
    orig_flags = intf_ptr->flags;
    intf_ptr->flags = intf->flags;
    intf_ptr->id = id;
    READ_CMIC_BS_CONFIGr(unit, &regval);

    /* Update output flags */
    if (TIME_MODE_INPUT == soc_reg_field_get(unit, CMIC_BS_CONFIGr, 
                                             regval, MODEf)) {
        intf_ptr->flags |= BCM_TIME_INPUT;
    } else {
        intf_ptr->flags &= ~BCM_TIME_INPUT;
    }

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
       if (soc_reg_field_get(unit, CMIC_BS_CONFIGr, 
                             regval, BS_CLK_OUTPUT_ENABLEf)) {
            intf_ptr->flags |= BCM_TIME_ENABLE;
        } else {
            intf_ptr->flags &= ~BCM_TIME_ENABLE;
        }

        READ_CMIC_BS_OUTPUT_TIME_0r(unit, &regval);
        if (soc_reg_field_get(unit, CMIC_BS_OUTPUT_TIME_0r,
                              regval, LOCKf)) {
            intf_ptr->flags |= BCM_TIME_LOCKED;
        } else {
            intf_ptr->flags &= ~BCM_TIME_LOCKED;
        }
    } else
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */
    {
        if (soc_reg_field_get(unit, CMIC_BS_CONFIGr, regval, ENABLEf)) {
            intf_ptr->flags |= BCM_TIME_ENABLE;
        } else {
            intf_ptr->flags &= ~BCM_TIME_ENABLE;
        }
        if (soc_reg_field_get(unit, CMIC_BS_CONFIGr, regval, LOCKf)) {
            intf_ptr->flags |= BCM_TIME_LOCKED;
        } else {
            intf_ptr->flags &= ~BCM_TIME_LOCKED;
        }
    }

    if (intf->flags & BCM_TIME_ACCURACY) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_time_interface_accuracy_get(unit, id, 
                                                 &(intf_ptr->accuracy)));
    }

    if (intf->flags & BCM_TIME_OFFSET) {
        if (orig_flags & BCM_TIME_SYNC_STAMPER) {
            
        } else {
            /* Get value from hardware */
            BCM_IF_ERROR_RETURN(
                _bcm_esw_time_interface_offset_get(unit, id, &(intf_ptr->offset)));
        }
    }

    if (intf->flags & BCM_TIME_DRIFT) {
        if (orig_flags & BCM_TIME_SYNC_STAMPER) {
            
        } else {
            /* Get value from hardware */
            BCM_IF_ERROR_RETURN(
                _bcm_esw_time_interface_drift_get(unit, id, &(intf_ptr->drift)));
        }
    }

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        if (intf->flags & BCM_TIME_REF_CLOCK) {
            BCM_IF_ERROR_RETURN(
                _bcm_esw_time_interface_ref_clock_get(unit, id,
                                            &(intf_ptr->clk_resolution)));
        }
    }
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */

    if (orig_flags & BCM_TIME_SYNC_STAMPER) {
        intf_ptr->flags = orig_flags;
    }

    *intf = *(TIME_INTERFACE(unit, id));

    return (BCM_E_NONE);
}

#if defined(BCM_TIME_V3_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
STATIC int 
_bcm_esw_time_interface_dual_bs_get(int unit, bcm_time_if_t id, bcm_time_interface_t *intf)
{
    uint32                  regval;
    bcm_time_interface_t    *intf_ptr;
    uint32                  orig_flags;

    if (soc_feature(unit, soc_feature_time_v3_no_bs)) {
        return BCM_E_UNAVAIL;
    }

    intf_ptr = TIME_INTERFACE(unit, id);
    orig_flags = intf_ptr->flags;
    intf_ptr->flags = intf->flags;
    intf_ptr->id = id;
    READ_CMIC_BS0_CONFIGr(unit, &regval);

    /* Update output flags */
    if (TIME_MODE_INPUT == soc_reg_field_get(unit, CMIC_BS0_CONFIGr, 
                                             regval, MODEf)) {
        intf_ptr->flags |= BCM_TIME_INPUT;
    } else {
        intf_ptr->flags &= ~BCM_TIME_INPUT;
    }

   if (soc_reg_field_get(unit, CMIC_BS0_CONFIGr, 
                         regval, BS_CLK_OUTPUT_ENABLEf)) {
        intf_ptr->flags |= BCM_TIME_ENABLE;
    } else {
        intf_ptr->flags &= ~BCM_TIME_ENABLE;
    }

    READ_CMIC_BS0_OUTPUT_TIME_0r(unit, &regval);
    if (soc_reg_field_get(unit, CMIC_BS0_OUTPUT_TIME_0r,
                          regval, LOCKf)) {
        intf_ptr->flags |= BCM_TIME_LOCKED;
    } else {
        intf_ptr->flags &= ~BCM_TIME_LOCKED;
    }

    if (intf->flags & BCM_TIME_ACCURACY) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_time_interface_accuracy_get(unit, id, 
                                                 &(intf_ptr->accuracy)));
    }

    if (intf->flags & BCM_TIME_OFFSET) {
        if (orig_flags & BCM_TIME_SYNC_STAMPER) {
            
        } else {
            /* Get value from hardware */
            BCM_IF_ERROR_RETURN(
                _bcm_esw_time_interface_offset_get(unit, id, &(intf_ptr->offset)));
        }
    }

    if (intf->flags & BCM_TIME_DRIFT) {
        if (orig_flags & BCM_TIME_SYNC_STAMPER) {
            
        } else {
            /* Get value from hardware */
            BCM_IF_ERROR_RETURN(
                _bcm_esw_time_interface_drift_get(unit, id, &(intf_ptr->drift)));
        }
    }

    if (intf->flags & BCM_TIME_REF_CLOCK) {
        BCM_IF_ERROR_RETURN(
            _bcm_esw_time_interface_ref_clock_get(unit, id,
                                        &(intf_ptr->clk_resolution)));
    }

    if (orig_flags & BCM_TIME_SYNC_STAMPER) {
        intf_ptr->flags = orig_flags;
    }

    *intf = *(TIME_INTERFACE(unit, id));

    return (BCM_E_NONE);
}
#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */

/*
 * Function:    
 *      _bcm_esw_time_hw_interrupt_dflt
 * Purpose:     
 *      Default handler for broadsync heartbeat interrupt
 * Parameters:  
 *      unit - StrataSwitch unit #.
 * Returns:     
 *      Nothing
 */

STATIC void
_bcm_esw_time_hw_interrupt_dflt(int unit)
{
    uint32  regval;

    /* Due to HW constrains we need to reinable the interrupt on every rising */
    /* edge and update the capture mode */

#if defined(BCM_TIME_V3_SUPPORT) || defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    uint32  intr, mode;

    if (soc_feature(unit, soc_feature_time_v3)) {
        /* Read original values */
        READ_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, &mode);
        READ_CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr(unit, &regval);
        READ_CMIC_TIMESYNC_INTERRUPT_ENABLEr(unit, &intr);
        
        /* Disable all */
        WRITE_CMIC_TIMESYNC_INTERRUPT_ENABLEr(unit, 0);
        WRITE_CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr(unit, 0);
        WRITE_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, 0);
        
        /* Enable with the original settings */
        WRITE_CMIC_TIMESYNC_INTERRUPT_ENABLEr(unit, intr);
        WRITE_CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr(unit, regval);
        WRITE_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, mode);
        
    } else
#endif /* defined(BCM_TIME_V3_SUPPORT) */
#if (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT))
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        READ_CMIC_TS_TIME_CAPTURE_CTRLr(unit, &regval);
        WRITE_CMIC_TS_TIME_CAPTURE_CTRLr(unit, 0);
        WRITE_CMIC_TS_TIME_CAPTURE_CTRLr(unit, regval);
    } else
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */
    {
        READ_CMIC_BS_CAPTURE_CTRLr(unit, &regval);
        WRITE_CMIC_BS_CAPTURE_CTRLr(unit, 0);
        WRITE_CMIC_BS_CAPTURE_CTRLr(unit, regval);
    }
    return;
}

/*
 * Function:    
 *      _bcm_esw_time_hw_interrupt
 * Purpose:     
 *      Handles broadsync heartbeat interrupt
 * Parameters:  
 *      unit - StrataSwitch unit #.
 * Returns:     
 *      Nothing
 */
STATIC void
_bcm_esw_time_hw_interrupt(int unit)
{
    void                    *u_data; 
    bcm_time_heartbeat_cb   u_cb;
    int                     idx;    /* interface itterator */
    bcm_time_capture_t      time;
    
    for (idx = 0; idx < NUM_TIME_INTERFACE(unit); idx++) {
        if (NULL != TIME_INTERFACE(unit, idx) &&
            NULL != TIME_INTERFACE_CONFIG(unit,idx).user_cb) {
            u_cb = TIME_INTERFACE_CONFIG(unit,idx).user_cb->heartbeat_cb;
            u_data = TIME_INTERFACE_CONFIG(unit,idx).user_cb->user_data;
            _bcm_esw_time_capture_counter_read(unit, idx, &time);
            if (NULL != u_cb) {
                u_cb(unit, idx, &time, u_data);
            }
        }
    }

    _bcm_esw_time_hw_interrupt_dflt(unit);

    return;
}


/****************************************************************************/
/*                      API functions implmentation                         */
/****************************************************************************/

/*
 * Function:
 *      bcm_esw_time_init
 * Purpose:
 *      Initialize time module 
 * Parameters:
 *      unit - (IN) StrataSwitch Unit #.
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_init (int unit)
{
    _bcm_time_config_p      time_cfg_ptr;   /* Pointer to Time module config */     
    bcm_time_interface_t    *intf;          /* Time interfaces iterator.     */
    int                     alloc_sz;       /* Memory allocation size.       */
    int                     idx;            /* Time interface array iterator */
    int                     rv;             /* Return Value                  */
    soc_control_t *soc = SOC_CONTROL(unit); /* Soc control structure         */

    /* Check if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }

    /* If already initialized then deinitialize time module */
    if (TIME_INIT(unit)) {
        _bcm_esw_time_deinit(unit, &TIME_CONFIG(unit));
    }

    /* Allocate time config structure. */
    alloc_sz = sizeof(_bcm_time_config_t);
    time_cfg_ptr = sal_alloc(alloc_sz, "Time module");
    if (NULL == time_cfg_ptr) {
        return (BCM_E_MEMORY);
    }
    sal_memset(time_cfg_ptr, 0, alloc_sz);

    /* Currently only one interface per unit */
    time_cfg_ptr->intf_count = NUM_TIME_INTERFACE(unit); 

    /* Allocate memory for all time interfaces, supported */
    alloc_sz = time_cfg_ptr->intf_count * sizeof(_bcm_time_interface_config_t);
    time_cfg_ptr->intf_arr = sal_alloc(alloc_sz, "Time Interfaces");
    if (NULL == time_cfg_ptr->intf_arr) {
        _bcm_esw_time_deinit(unit, &time_cfg_ptr);
        return (BCM_E_MEMORY);
    }
    sal_memset(time_cfg_ptr->intf_arr, 0, alloc_sz);
    for (idx = 0; idx < time_cfg_ptr->intf_count; idx++) {
        intf = &time_cfg_ptr->intf_arr[idx].time_interface;
        intf->id = idx;
    }

    /* For each time interface allocate memory for tuser_cb */
    alloc_sz = sizeof(_bcm_time_user_cb_t);
    for (idx = 0; idx < NUM_TIME_INTERFACE(unit); idx++) {
        time_cfg_ptr->intf_arr[idx].user_cb = 
            sal_alloc(alloc_sz, "Time Interface User Callback");
        if (NULL == time_cfg_ptr->intf_arr[idx].user_cb) {
            _bcm_esw_time_deinit(unit,  &time_cfg_ptr);
            return (BCM_E_MEMORY);
        }
        sal_memset(time_cfg_ptr->intf_arr[idx].user_cb, 0, alloc_sz);
    }

    /* Interrupt handling function initialization */
    soc->time_call_ref_count = 0;
    soc->soc_time_callout = NULL;

    /* Create protection mutex. */
    time_cfg_ptr->mutex = sal_mutex_create("Time mutex");
    if (NULL == time_cfg_ptr->mutex) {
        _bcm_esw_time_deinit(unit, &time_cfg_ptr);
        return (BCM_E_MEMORY);
    } 

    sal_mutex_take(time_cfg_ptr->mutex, sal_mutex_FOREVER);

    TIME_CONFIG(unit) = time_cfg_ptr;

    /* Clear memories/registers on Cold boot only */
    if (!SOC_WARM_BOOT(unit) &&
        _bcm_esw_time_hitless_reset(unit, idx) != BCM_E_NONE) {
    for (idx = 0; idx < NUM_TIME_INTERFACE(unit); idx++) {
        rv  = _bcm_esw_time_hw_clear(unit, idx);
        if (BCM_FAILURE(rv)) {
            TIME_UNLOCK(unit);
            _bcm_esw_time_deinit(unit, &time_cfg_ptr);
            TIME_CONFIG(unit) = NULL;
            return (BCM_E_MEMORY);
        }
    }
    } else {
        /* If Warm boot reinitialize module based on HW state */
#ifdef BCM_WARM_BOOT_SUPPORT
        for (idx = 0; idx < NUM_TIME_INTERFACE(unit); idx++) {
            rv = _bcm_esw_time_reinit(unit, idx);
            if (BCM_FAILURE(rv)) {
                TIME_UNLOCK(unit);
                _bcm_esw_time_deinit(unit, &time_cfg_ptr);
                TIME_CONFIG(unit) = NULL;
                return (rv);
            }
        }

#endif /* BCM_WARM_BOOT_SUPPORT */
    }

    TIME_UNLOCK(unit);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_esw_time_deinit
 * Purpose:
 *      Uninitialize time module 
 * Parameters:
 *      unit - (IN) StrataSwitch Unit #.
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_deinit (int unit)
{
    /* Chek if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }

    if (0 == TIME_INIT(unit)) {
        return (BCM_E_INIT);
    }

    return _bcm_esw_time_deinit(unit, &TIME_CONFIG(unit));
}

/*
 * Function:
 *      bcm_esw_time_interface_add
 * Purpose:
 *      Adding a time sync interface to a specified unit 
 * Parameters:
 *      unit - (IN) StrataSwitch Unit #.
 *      intf - (IN/OUT) Time Sync Interface
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_interface_add (int unit, bcm_time_interface_t *intf)
{
    int rv = BCM_E_NONE; /* Return value */
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    int replacing = 0;
#endif
    if (!SOC_UNIT_VALID(unit)) {
        return BCM_E_UNIT;
    }

    /* Check if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }

    BCM_IF_ERROR_RETURN(
        _bcm_esw_time_interface_input_validate(unit, intf));

    if (TIME_CONFIG(unit) == NULL) {
        return (BCM_E_INIT);
    }

    TIME_LOCK(unit);
    if (intf->flags & BCM_TIME_WITH_ID) {
        /* If interface already been in use */
        if (0 != TIME_INTERFACE_CONFIG_REF_COUNT(unit, intf->id)) {
            if (0 == (intf->flags & BCM_TIME_REPLACE)) {
                TIME_UNLOCK(unit);
                return BCM_E_EXISTS;
            }
#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
            replacing = 1;
#endif
        } else {
            TIME_INTERFACE_CONFIG_REF_COUNT(unit, intf->id) = 1;
        }
    } else {
        rv = _bcm_esw_time_interface_allocate_id(unit, &(intf->id));
        if (BCM_FAILURE(rv)) {
            TIME_UNLOCK(unit);
            return rv;
        }
    }

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) || defined(BCM_HELIX4_SUPPORT) || defined(BCM_KATANA2_SUPPORT)
    if (intf->flags & BCM_TIME_SYNC_STAMPER) {
        int reset_hardware = 0;

        /* Set time interface configuration. */
        if (replacing) {
            if (((TIME_INTERFACE(unit, intf->id))->flags &
                 (BCM_TIME_ENABLE | BCM_TIME_INPUT)) !=
                (intf->flags & (BCM_TIME_ENABLE | BCM_TIME_INPUT))) {
                /* either master or enabled state changed, so copy those bits in flags */
                (TIME_INTERFACE(unit, intf->id))->flags &= ~(BCM_TIME_ENABLE | BCM_TIME_INPUT);
                (TIME_INTERFACE(unit, intf->id))->flags |=
                    intf->flags & (BCM_TIME_ENABLE | BCM_TIME_INPUT);
                /* either master or enabled state changed, so re-init */
                reset_hardware = 1;
            }
            if (intf->flags & BCM_TIME_DRIFT) {
                (TIME_INTERFACE(unit, intf->id))->drift = intf->drift;
            }
            if (intf->flags & BCM_TIME_OFFSET) {
                (TIME_INTERFACE(unit, intf->id))->offset = intf->offset;
            }
            if (intf->flags & BCM_TIME_ACCURACY) {
                (TIME_INTERFACE(unit, intf->id))->accuracy = intf->accuracy;
            }
            if (intf->flags & BCM_TIME_HEARTBEAT) {
                if ((TIME_INTERFACE(unit, intf->id))->heartbeat_hz != intf->heartbeat_hz) {
                    (TIME_INTERFACE(unit, intf->id))->heartbeat_hz = intf->heartbeat_hz;
                    /* Also use heartbeat flag to indicate whether to replace bitclock freq */
                    (TIME_INTERFACE(unit, intf->id))->bitclock_hz = intf->bitclock_hz;
                    reset_hardware = 1;
                }
            }
        } else {
            /* Is new, so just copy wholesale */
            *(TIME_INTERFACE(unit, intf->id)) = *intf;
            reset_hardware = 1;
        }

        if (reset_hardware) {
            /* Enable/Setup TIME API BroadSync on the unit */
            rv = _bcm_esw_time_bs_init(unit, TIME_INTERFACE(unit, intf->id));
            if (BCM_FAILURE(rv)) {
                TIME_UNLOCK(unit);
                return rv;
            }
        }

        if (!replacing) {
            /* Enable BS firmware on the unit */
            rv = _bcm_mbox_comm_init(unit, MOS_MSG_MBOX_APPL_BS);
            if (BCM_FAILURE(rv)) {
                TIME_UNLOCK(unit);
                return rv;
            }
        }

        /* Set specified offsets */
        if (intf->flags & BCM_TIME_DRIFT) {
            rv = _bcm_time_bs_frequency_offset_set(unit, intf->drift);
            if (BCM_FAILURE(rv)) {
                TIME_UNLOCK(unit);
                return rv;
            }
        }

        if (intf->flags & BCM_TIME_OFFSET) {
            rv = _bcm_time_bs_phase_offset_set(unit, intf->offset);
            if (BCM_FAILURE(rv)) {
                TIME_UNLOCK(unit);
                return rv;
            }
        }

        TIME_UNLOCK(unit);
        return (BCM_E_NONE);
    }


    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        uint32          regval = 0;


#if (defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT))
    if (SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        /* Enable Timestamp Generation */
        READ_CMIC_TIMESYNC_TS0_FREQ_CTRL_FRACr(unit, &regval);
        /* 250 Mhz - 4.000 ns */
        soc_reg_field_set(unit, CMIC_TIMESYNC_TS0_FREQ_CTRL_FRACr, &regval, FRACf,
                        ((soc_property_get(unit, spn_PTP_TS_PLL_FREF, 25000000)
                        == 12800000 ) ? 0x40842108 : 0x40000000));
        WRITE_CMIC_TIMESYNC_TS0_FREQ_CTRL_FRACr(unit, regval);

        READ_CMIC_TIMESYNC_TS0_FREQ_CTRL_UPPERr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TS0_FREQ_CTRL_UPPERr, &regval, NSf, 0);
        WRITE_CMIC_TIMESYNC_TS0_FREQ_CTRL_UPPERr(unit, regval);

        READ_CMIC_TIMESYNC_TS0_COUNTER_ENABLEr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TS0_COUNTER_ENABLEr, &regval, ENABLEf, 1);
        WRITE_CMIC_TIMESYNC_TS0_COUNTER_ENABLEr(unit, regval);
    } else
#endif
    {
        /* Enable Timestamp Generation */
        READ_CMIC_TS_FREQ_CTRL_LOWERr(unit, &regval);
        /* 250 Mhz - 4.000 ns */
        soc_reg_field_set(unit, CMIC_TS_FREQ_CTRL_LOWERr, &regval, FRACf,
                        ((soc_property_get(unit, spn_PTP_TS_PLL_FREF, 25000000)
                        == 12800000 ) ? 0x40842108 : 0x40000000));
        WRITE_CMIC_TS_FREQ_CTRL_LOWERr(unit, regval);

        READ_CMIC_TS_FREQ_CTRL_UPPERr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TS_FREQ_CTRL_UPPERr, &regval, NSf, 0);
        soc_reg_field_set(unit, CMIC_TS_FREQ_CTRL_UPPERr, &regval, ENABLEf, 1);
        WRITE_CMIC_TS_FREQ_CTRL_UPPERr(unit, regval);
    }

        if (intf->flags & BCM_TIME_REF_CLOCK) {
            rv = _bcm_esw_time_interface_ref_clock_install(unit, intf->id);
            if (BCM_FAILURE(rv)) {
                TIME_INTERFACE_CONFIG_REF_COUNT(unit, intf->id) -= 1;
                TIME_UNLOCK(unit);
                return rv;
            }
        }
    }
#endif /*  defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined (BCM_TRIDENT2_SUPPORT) */

#if defined(BCM_TIME_V3_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3) || SOC_IS_HELIX4(unit)) {
        uint32 regval;

        /* Enable common control for both counters */
        READ_CMIC_TIMESYNC_COUNTER_CONFIG_SELECTr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_COUNTER_CONFIG_SELECTr, &regval, 
                          ENABLE_COMMON_CONTROLf, 1);
        WRITE_CMIC_TIMESYNC_COUNTER_CONFIG_SELECTr(unit, regval);
        
        /* 250 Mhz - 4.000 ns */
        READ_CMIC_TIMESYNC_TS0_FREQ_CTRL_FRACr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TS0_FREQ_CTRL_FRACr, &regval, 
                          FRACf, 0x40000000);
        WRITE_CMIC_TIMESYNC_TS0_FREQ_CTRL_FRACr(unit, regval);

        READ_CMIC_TIMESYNC_TS0_FREQ_CTRL_UPPERr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TS0_FREQ_CTRL_UPPERr, &regval, 
                          NSf, 0);
        WRITE_CMIC_TIMESYNC_TS0_FREQ_CTRL_UPPERr(unit, regval);

        READ_CMIC_TIMESYNC_TS0_FREQ_CTRL_LOWERr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TS0_FREQ_CTRL_LOWERr, &regval, 
                          NSf, 0);
        WRITE_CMIC_TIMESYNC_TS0_FREQ_CTRL_LOWERr(unit, regval);

        /* Enable Timestamp Generation */
        READ_CMIC_TIMESYNC_TS0_COUNTER_ENABLEr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TS0_COUNTER_ENABLEr, &regval, 
                          ENABLEf, 1);
        WRITE_CMIC_TIMESYNC_TS0_COUNTER_ENABLEr(unit, regval);
        
        if (intf->flags & BCM_TIME_REF_CLOCK) {
            rv = _bcm_esw_time_interface_ref_clock_install(unit, intf->id);
            if (BCM_FAILURE(rv)) {
                TIME_INTERFACE_CONFIG_REF_COUNT(unit, intf->id) -= 1;
                TIME_UNLOCK(unit);
                return rv;
            }
        }
    }
#endif /* defined(BCM_TIME_V3_SUPPORT) */

    /* Set time interface configuration. */
    *(TIME_INTERFACE(unit, intf->id)) = *intf; 

    /* Install the interface into the HW */
#if defined(BCM_TIME_V3_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3) || 
        SOC_IS_HELIX4(unit) || SOC_IS_KATANA2(unit)) {
        if (!soc_feature(unit, soc_feature_time_v3_no_bs)) {
            rv = _bcm_esw_time_interface_dual_bs_install(unit, intf->id);
        }
    } else 
#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */
    {
        rv = _bcm_esw_time_interface_install(unit, intf->id);
    }

    if (BCM_FAILURE(rv)) {
        TIME_INTERFACE_CONFIG_REF_COUNT(unit, intf->id) -= 1;
        TIME_UNLOCK(unit);
        return rv;
    }

    TIME_UNLOCK(unit);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_esw_time_interface_delete
 * Purpose:
 *      Deleting a time sync interface from unit 
 * Parameters:
 *      unit    - (IN) StrataSwitch Unit #.
 *      intf_id - (IN) Time Sync Interface id to remove
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_interface_delete (int unit, bcm_time_if_t intf_id)
{
    int rv;

    /* Chek if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }

    /* Initialization check. */
    BCM_IF_ERROR_RETURN(
        _bcm_esw_time_interface_id_validate(unit, intf_id));

    TIME_LOCK(unit);

    /* If interface still in use return an Error */
    if (1 < TIME_INTERFACE_CONFIG_REF_COUNT(unit, intf_id)) {
        TIME_UNLOCK(unit);
        return (BCM_E_BUSY);
    }

    /* Free the interface */
    rv = _bcm_time_interface_free(unit, intf_id); 
    if (BCM_FAILURE(rv)) {
        TIME_UNLOCK(unit);
        return (rv);
    }

    rv = _bcm_esw_time_hw_clear(unit, intf_id);

    TIME_UNLOCK(unit);
    return (rv);
}

/*
 * Function:
 *      bcm_esw_time_interface_get
 * Purpose:
 *      Get a time sync interface on a specified unit 
 * Parameters:
 *      unit -  (IN) StrataSwitch Unit #.
 *      intf - (IN/OUT) Time Sync Interface to get
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_interface_get (int unit, bcm_time_interface_t *intf)
{
    int rv; 

    /* Chek if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }

    /* Validation checks */
    if (NULL == intf) {
        return BCM_E_PARAM;
    }

    BCM_IF_ERROR_RETURN(
        _bcm_esw_time_interface_id_validate(unit, intf->id));

    TIME_LOCK(unit);
#if defined(BCM_TIME_V3_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3) || 
        SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        rv = _bcm_esw_time_interface_dual_bs_get(unit, intf->id, intf);
    } else
#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */
    {
        rv = _bcm_esw_time_interface_get(unit, intf->id, intf);
    }
    TIME_UNLOCK(unit);

    return (rv);
}

/*
 * Function:
 *      bcm_esw_time_interface_delete_all
 * Purpose:
 *      Deleting all time sync interfaces on a unit 
 * Parameters:
 *      unit    - (IN) StrataSwitch Unit #.
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_interface_delete_all (int unit)
{
    bcm_time_if_t   intf;

    /* Chek if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }

    /* Initialization check */
    if (0 == TIME_INIT(unit)) {
        return BCM_E_INIT;
    }

    for (intf = 0; intf < NUM_TIME_INTERFACE(unit); intf++ ) {
        BCM_IF_ERROR_RETURN(
            bcm_esw_time_interface_delete(unit, intf));
    }

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_esw_time_interface_traverse
 * Purpose:
 *      Itterates over all time sync interfaces and calls given callback 
 * Parameters:
 *      unit        - (IN) StrataSwitch Unit #.
 *      cb          - (IN) Call back function
 *      user_data   - (IN) void pointer to store any user information
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_interface_traverse (int unit, bcm_time_interface_traverse_cb cb, 
                                 void *user_data)
{
    bcm_time_if_t   intf;
    
    /* Chek if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }

    /* Initialization check. */
    if (0 == TIME_INIT(unit)) {
        return BCM_E_INIT;
    }
    
    if (NULL == cb) {
        return (BCM_E_PARAM);
    }

    for (intf = 0; intf < NUM_TIME_INTERFACE(unit); intf++ ) {
        if (NULL != TIME_INTERFACE(unit, intf)) {
            BCM_IF_ERROR_RETURN(cb(unit, TIME_INTERFACE(unit, intf), user_data));
        }
    }
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_esw_time_capture_get
 * Purpose:
 *      Gets a time captured by HW clock
 * Parameters:
 *      unit    - (IN) StrataSwitch Unit #.
 *      id      - (IN) Time interface identifier 
 *      time    - (OUT) Structure to contain HW clocks values
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_capture_get (int unit, bcm_time_if_t id, bcm_time_capture_t *time)
{
    int rv;   /* Operation return status. */

    /* Chek if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }

    /* Initialization check. */
    BCM_IF_ERROR_RETURN(
        _bcm_esw_time_interface_id_validate(unit, id));

    if (NULL == time) {
        return (BCM_E_PARAM);
    }
    if (NULL == TIME_CAPTURE(unit, id)) {
        return (BCM_E_NOT_FOUND);
    }

    TIME_LOCK(unit);
    rv = _bcm_esw_time_capture_get(unit, id, TIME_CAPTURE(unit, id));
    if (BCM_FAILURE(rv)) {
        TIME_UNLOCK(unit);
        return (rv);
    }

    *time = *(TIME_CAPTURE(unit, id));

    TIME_UNLOCK(unit);

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_esw_time_heartbeat_enable_set
 * Purpose:
 *      Enables/Disables interrupt handling for each heartbeat provided by a 
 *      HW clock
 * Parameters:
 *      unit    - (IN) StrataSwitch Unit #.
 *      id      - (IN) Time Sync Interface Id
 *      enable  - (IN) Enable/Disable parameter
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_heartbeat_enable_set (int unit, bcm_time_if_t id, int enable)
{
    uint32          regval;
    soc_control_t   *soc = SOC_CONTROL(unit);

    /* Check if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }

    /* Param validation check. */
    BCM_IF_ERROR_RETURN(
        _bcm_esw_time_interface_id_validate(unit, id));

    TIME_LOCK(unit);

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
    if ((TIME_INTERFACE(unit, id))->flags & BCM_TIME_SYNC_STAMPER) {
        int rv = _bcm_time_bs_debug_1pps_set(unit, enable);
        if (BCM_FAILURE(rv)) {
            TIME_UNLOCK(unit);
            return rv;
        }

        TIME_UNLOCK(unit);
        return BCM_E_NONE;
    }
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */

    /* Install heartbeat and external trigger clock interval */
#if defined(BCM_TIME_V3_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3) || SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        
        READ_CMIC_TIMESYNC_INTERRUPT_ENABLEr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_INTERRUPT_ENABLEr, &regval, INT_ENABLEf,
            (enable) ? IPROC_TIME_CAPTURE_MODE_HB_BS_0 : IPROC_TIME_CAPTURE_MODE_DISABLE);
        WRITE_CMIC_TIMESYNC_INTERRUPT_ENABLEr(unit, regval);

        if (enable) {
            /* Configure the HW to raise interrupt on every heartbeat */
            READ_CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, &regval, TIME_CAPTURE_ENABLEf, 1);
            soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, &regval, BSYNC0_TX_HB_STATUS_ENABLEf, 1);
            soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, &regval, BSYNC0_RX_HB_STATUS_ENABLEf, 1);
            WRITE_CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr(unit, regval);
        } else {
            /* Leave TIME_CAPTURE_ENABLEf for other trigger sources */
            READ_CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, &regval, BSYNC0_RX_HB_STATUS_ENABLEf, 0);
            soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr, &regval, BSYNC0_TX_HB_STATUS_ENABLEf, 0);
            WRITE_CMIC_TIMESYNC_TIME_CAPTURE_CONTROLr(unit, regval);
        }

        /* Configure the HW to capture time on every heartbeat */
        READ_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TIME_CAPTURE_MODEr, &regval, TIME_CAPTURE_MODEf,
            (enable) ? IPROC_TIME_CAPTURE_MODE_HB_BS_0 : IPROC_TIME_CAPTURE_MODE_DISABLE);

        WRITE_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, regval);

        /* set interrupt enables to match the mode */
        WRITE_CMIC_TIMESYNC_INTERRUPT_ENABLEr(unit, regval);

    } else

#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */
#if (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT))
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
        READ_CMIC_TS_TIME_CAPTURE_CTRLr(unit, &regval);
        /* Configure the HW to raise interrupt on every heartbeat */
        soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, INT_ENABLEf, 
                          (enable) ? TIME_CAPTURE_MODE_HEARTBEAT : 0);
        soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, TIME_CAPTURE_ENABLEf, 1);
        soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, TX_HB_STATUS_ENABLEf, 1);
        soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, RX_HB_STATUS_ENABLEf, 1);

        /* Configure the HW to capture time on every heartbeat */
        soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, TIME_CAPTURE_MODEf,
                          (enable) ? TIME_CAPTURE_MODE_HEARTBEAT : TIME_CAPTURE_MODE_DISABLE);
        WRITE_CMIC_TS_TIME_CAPTURE_CTRLr(unit, regval);
    } else
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */
    {
        READ_CMIC_BS_CAPTURE_CTRLr(unit, &regval);
        /* Configure the HW to give interrupt on every heartbeat */
        soc_reg_field_set(unit, CMIC_BS_CAPTURE_CTRLr, &regval, INT_ENf, (enable) ? 1: 0);
        /* Configure the HW to capture time on every heartbeat */
        soc_reg_field_set(unit, CMIC_BS_CAPTURE_CTRLr, &regval, TIME_CAPTURE_MODEf,
                          (enable) ? TIME_CAPTURE_MODE_HEARTBEAT : TIME_CAPTURE_MODE_DISABLE);
        WRITE_CMIC_BS_CAPTURE_CTRLr(unit, regval);
    }

    TIME_UNLOCK(unit);

    /* Enable/disable broadsync interrupt */
    if (enable) {
        soc_intr_enable(unit, IRQ_BROADSYNC_INTR);
        if (!soc->time_call_ref_count) {
            soc->soc_time_callout = _bcm_esw_time_hw_interrupt_dflt;
        }
        
    } else {
        soc_intr_disable(unit, IRQ_BROADSYNC_INTR);
        if (!soc->time_call_ref_count) {
            soc->soc_time_callout = NULL;
        }
    }
    
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_esw_time_heartbeat_enable_get
 * Purpose:
 *      Gets interrupt handling status for each heartbeat provided by a 
 *      HW clock
 * Parameters:
 *      unit    - (IN) StrataSwitch Unit #.
 *      id      - (IN) Time Sync Interface Id
 *      enable  - (OUT) Enable status of interrupt handling
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_heartbeat_enable_get (int unit, bcm_time_if_t id, int *enable)
{
    uint32      regval;

    /* Chek if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }
    
    /* Param validation check. */
    if (enable == NULL) {
        return (BCM_E_PARAM);
    }
    BCM_IF_ERROR_RETURN(
        _bcm_esw_time_interface_id_validate(unit, id));

    /* Read HW Configuration */
#if defined(BCM_TIME_V3_SUPPORT) || \
    defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
    if (soc_feature(unit, soc_feature_time_v3) || 
        SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        
        READ_CMIC_TIMESYNC_TIME_CAPTURE_MODEr(unit, &regval);
        *enable = (regval & TIME_CAPTURE_MODE_HEARTBEAT)? 1 : 0;
    } else
#endif /* BCM_TIME_V3_SUPPORT || BCM_KATANA2_SUPPORT || BCM_HELIX4_SUPPORT */
#if (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT))
    if (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
        SOC_IS_TD2_TT2(unit)) {
        READ_CMIC_TS_TIME_CAPTURE_CTRLr(unit, &regval);
        *enable = soc_reg_field_get(unit, CMIC_TS_TIME_CAPTURE_CTRLr,
                                    regval, INT_ENABLEf);
        *enable = (*enable ==  TIME_CAPTURE_MODE_HEARTBEAT) ? 1 : 0; 
    } else
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */
    {
        READ_CMIC_BS_CAPTURE_CTRLr(unit, &regval);
        *enable = soc_reg_field_get(unit, CMIC_BS_CAPTURE_CTRLr, regval, INT_ENf); 
    }

    return (BCM_E_NONE);
}


#if (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT))
/*
 * Function:
 *      _bcm_esw_time_trigger_to_timestamp_mode
 * Purpose: 
 *      Convert capture mode flags to time control capture modes
 *
 * Parameters: 
 *      unit       - (IN) mode_flags
 *      
 * Returns:
 *      Timesync captude modes 
 * Notes:
 */
STATIC uint32
_bcm_esw_time_trigger_to_timestamp_mode(int unit, uint32 mode_flags)
{
    uint32 capture_mode = 0;
    uint32 regval;
    int    bit_cnt;

    /* processing on user mode flags reduces the dependencies on 
     * hardware bit maps, byte shift of mode flags
     * should have been efficient 
     */
    for (bit_cnt = 0; bit_cnt < sizeof(uint32); bit_cnt++)
    {
        switch (mode_flags & (1 << bit_cnt))
        {
        case BCM_TIME_CAPTURE_GPIO_0:
            capture_mode |=  TIME_CAPTURE_MODE_GPIO_0;
            READ_CMIC_TS_GPIO_1_DOWN_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_1_DOWN_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_1_DOWN_EVENT_CTRLr(unit, regval);
            READ_CMIC_TS_GPIO_1_UP_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_1_UP_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_1_UP_EVENT_CTRLr(unit, regval);
            break;

        case BCM_TIME_CAPTURE_GPIO_1:
            capture_mode |=  TIME_CAPTURE_MODE_GPIO_1;
            READ_CMIC_TS_GPIO_1_DOWN_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_1_DOWN_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_1_DOWN_EVENT_CTRLr(unit, regval);
            READ_CMIC_TS_GPIO_1_UP_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_1_UP_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_1_UP_EVENT_CTRLr(unit, regval);
            break;

        case BCM_TIME_CAPTURE_GPIO_2:
            capture_mode |=  TIME_CAPTURE_MODE_GPIO_2;
            READ_CMIC_TS_GPIO_2_DOWN_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_2_DOWN_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_2_DOWN_EVENT_CTRLr(unit, regval);
            READ_CMIC_TS_GPIO_2_UP_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_2_UP_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_2_UP_EVENT_CTRLr(unit, regval);
            break;

        case BCM_TIME_CAPTURE_GPIO_3:
            capture_mode |=  TIME_CAPTURE_MODE_GPIO_3;
            READ_CMIC_TS_GPIO_3_DOWN_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_3_DOWN_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_3_DOWN_EVENT_CTRLr(unit, regval);
            READ_CMIC_TS_GPIO_3_UP_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_3_UP_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_3_UP_EVENT_CTRLr(unit, regval);
            break;

        case BCM_TIME_CAPTURE_GPIO_4:
            capture_mode |=  TIME_CAPTURE_MODE_GPIO_4;
            READ_CMIC_TS_GPIO_4_DOWN_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_4_DOWN_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_4_DOWN_EVENT_CTRLr(unit, regval);
            READ_CMIC_TS_GPIO_4_UP_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_4_UP_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_4_UP_EVENT_CTRLr(unit, regval);
            break;

        case BCM_TIME_CAPTURE_GPIO_5:
            capture_mode |=  TIME_CAPTURE_MODE_GPIO_5;
            READ_CMIC_TS_GPIO_5_DOWN_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_5_DOWN_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_5_DOWN_EVENT_CTRLr(unit, regval);
            READ_CMIC_TS_GPIO_5_UP_EVENT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_GPIO_5_UP_EVENT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_GPIO_5_UP_EVENT_CTRLr(unit, regval);
            break;

        case BCM_TIME_CAPTURE_L1_CLOCK_PRIMARY:
            capture_mode |=  TIME_CAPTURE_MODE_L1_CLOCK_PRIMARY;
            READ_CMIC_TS_L1_CLK_RECOVERED_CLK_COUNT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_L1_CLK_RECOVERED_CLK_COUNT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_L1_CLK_RECOVERED_CLK_COUNT_CTRLr(unit, regval);
            break;

        case BCM_TIME_CAPTURE_L1_CLOCK_SECONDARY:
            capture_mode |=  TIME_CAPTURE_MODE_L1_CLOCK_SECONDARY;
            READ_CMIC_TS_L1_CLK_RECOVERED_CLK_BKUP_COUNT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_L1_CLK_RECOVERED_CLK_BKUP_COUNT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_L1_CLK_RECOVERED_CLK_BKUP_COUNT_CTRLr(unit, regval);
            break;

        case BCM_TIME_CAPTURE_LCPLL:
            capture_mode |= TIME_CAPTURE_MODE_LCPLL;
            READ_CMIC_TS_LCPLL_CLK_COUNT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_LCPLL_CLK_COUNT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_LCPLL_CLK_COUNT_CTRLr(unit, regval);
            break;

        case BCM_TIME_CAPTURE_IP_DM_0:
            capture_mode |= TIME_CAPTURE_MODE_IP_DM_0;
            break;

        case BCM_TIME_CAPTURE_IP_DM_1:
            capture_mode |= TIME_CAPTURE_MODE_IP_DM_1;
            break;

        default:
            break;
        }
    }

    return capture_mode;
}

/*
 * Function:
 *      _bcm_esw_time_trigger_from_timestamp_mode
 * Purpose: 
 *      Convert Timesync capture modes to mode flags 
 *
 * Parameters: 
 *      unit       - (IN) capture_mode
 *      
 * Returns:
 *      Capture mode flags
 * Notes:
 */

STATIC uint32
_bcm_esw_time_trigger_from_timestamp_mode (uint32 capture_mode)
{
    uint32 user_flags = 0;
    int    bit_cnt;

    /* processing on user mode flags reduces the dependencies on 
     * hardware bit maps, byte shift of mode flags
     * should have been efficient 
     */
    for (bit_cnt = 0; bit_cnt < sizeof(uint32); bit_cnt++)
    {
        switch (capture_mode & (1 << bit_cnt))
        {
        case TIME_CAPTURE_MODE_GPIO_0:
            user_flags |=  BCM_TIME_CAPTURE_GPIO_0;
            break;
        case TIME_CAPTURE_MODE_GPIO_1:
            user_flags |=  BCM_TIME_CAPTURE_GPIO_1;
            break;
        case TIME_CAPTURE_MODE_GPIO_2:
            user_flags |=  BCM_TIME_CAPTURE_GPIO_2;
            break;
        case TIME_CAPTURE_MODE_GPIO_3:
            user_flags |=  BCM_TIME_CAPTURE_GPIO_3;
            break;
        case TIME_CAPTURE_MODE_GPIO_4:
            user_flags |=  BCM_TIME_CAPTURE_GPIO_4;
            break;
        case TIME_CAPTURE_MODE_GPIO_5:
            user_flags |=  BCM_TIME_CAPTURE_GPIO_5;
            break;
        case TIME_CAPTURE_MODE_L1_CLOCK_PRIMARY:
            user_flags |=  BCM_TIME_CAPTURE_L1_CLOCK_PRIMARY;
            break;
        case TIME_CAPTURE_MODE_L1_CLOCK_SECONDARY:
            user_flags |=  BCM_TIME_CAPTURE_L1_CLOCK_SECONDARY;
            break;
        case TIME_CAPTURE_MODE_LCPLL:
            user_flags |=  BCM_TIME_CAPTURE_LCPLL;
            break;
        case TIME_CAPTURE_MODE_IP_DM_0:
            user_flags |=  BCM_TIME_CAPTURE_IP_DM_0;
            break;
        case TIME_CAPTURE_MODE_IP_DM_1:
            user_flags |= BCM_TIME_CAPTURE_IP_DM_1;
            break;
        default:
            break;
        }
    }

    return user_flags;
}

#endif /* (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)) */

/*
 * Function:
 *      bcm_esw_time_trigger_enable_set
 * Purpose:
 *      Enables/Disables interrupt handling for external triggers
 * Parameters:
 *      unit        - (IN) StrataSwitch Unit #.
 *      id          - (IN) Time Sync Interface Id
 *      mode_flags  - (IN) Enable/Disable parameter
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_trigger_enable_set(int unit, bcm_time_if_t id, uint32 mode_flags)
{
#if (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT))
    uint32          regval, capture_mode = 0;
    soc_control_t   *soc = SOC_CONTROL(unit);
#endif

    /* Check if time feature is supported on a device */
    if (!(soc_feature(unit, soc_feature_time_support) &&
          (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
           SOC_IS_TD2_TT2(unit)))) {
        return (BCM_E_UNAVAIL);
    }

    /* Param validation check. */
    BCM_IF_ERROR_RETURN(
        _bcm_esw_time_interface_id_validate(unit, id));

    TIME_LOCK(unit);

#if (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT))
    /* 
     *   Read time capture control capture modes and 
     *   Convert to hardware cature mode bitmap.
     *   Enable interrupt for all capture mode bitmap and
     *   Write time capture mode.
     *
     *   If for ALL capture mode, ignore individual capture modes and
     *   Enable Interrupts.
     */
    READ_CMIC_TS_TIME_CAPTURE_CTRLr(unit, &regval);

    if (mode_flags & BCM_TIME_CAPTURE_ALL) {
        /* Enable all Time capture mechanisms */
        soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, 
                            TIME_CAPTURE_ENABLEf, 1 );
        
        /* Return and  ignore setting of individual capture modes */
        return BCM_E_NONE;
    }

    /* Set time capture modes */
    capture_mode = _bcm_esw_time_trigger_to_timestamp_mode (unit, mode_flags);

    /* Configure the HW with capture modes */
    soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, 
                    TIME_CAPTURE_MODEf, (capture_mode) ? capture_mode : 
                    TIME_CAPTURE_MODE_DISABLE);

    /* Configure the HW to trigger interrupt for capture events */
    soc_reg_field_set(unit, CMIC_TS_TIME_CAPTURE_CTRLr, &regval, INT_ENABLEf, 
                      (capture_mode) ? capture_mode : 0);
    
    WRITE_CMIC_TS_TIME_CAPTURE_CTRLr(unit, regval);

    TIME_UNLOCK(unit);

    /* Enable/disable broadsync interrupt */
    if (capture_mode) {
        /* Enable Interrupt */
        soc_intr_enable(unit, IRQ_BROADSYNC_INTR);
        if (!soc->time_call_ref_count) {
            soc->soc_time_callout = _bcm_esw_time_hw_interrupt_dflt;
        }
        
    } else {
        /* Check on the references and disable interrupt and handler */
        soc_intr_disable(unit, IRQ_BROADSYNC_INTR);
        if (!soc->time_call_ref_count) {
            soc->soc_time_callout = NULL;
        }
    }
#endif /* defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */

    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_esw_time_trigger_enable_get
 * Purpose:
 *      Gets interrupt handling status for each heartbeat provided by a 
 *      HW clock
 * Parameters:
 *      unit    - (IN) StrataSwitch Unit #.
 *      id      - (IN) Time Sync Interface Id
 *      enable  - (OUT) Enable status of interrupt handling
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_trigger_enable_get(int unit, bcm_time_if_t id, uint32 *mode_flags)
{
#if (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT))
    uint32      regval, capture_mode = 0;
#endif

    /* Chek if time feature is supported on a device */
    if (!(soc_feature(unit, soc_feature_time_support) &&
          (SOC_IS_KATANAX(unit) || SOC_IS_TRIUMPH3(unit) ||
           SOC_IS_TD2_TT2(unit)))) {
        return (BCM_E_UNAVAIL);
    }

    /* Param validation check. */
    BCM_IF_ERROR_RETURN(
        _bcm_esw_time_interface_id_validate(unit, id));

#if (defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT))
    /* Install heartbeat and external trigger clock interval */
    if (SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        /* Read HW Configuration */
        READ_CMIC_TIMESYNC_INTERRUPT_ENABLEr(unit, &regval);

        /* Get interrupt enabled capture modes */
        capture_mode = soc_reg_field_get(unit, CMIC_TIMESYNC_INTERRUPT_ENABLEr, regval, INT_ENABLEf); 
        *mode_flags = _bcm_esw_time_trigger_from_timestamp_mode (capture_mode);
    } else 
#endif
    {
#if (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT))
        /* Read HW Configuration */
        READ_CMIC_TS_TIME_CAPTURE_CTRLr(unit, &regval);

        /* Get interrupt enabled capture modes */
        capture_mode = soc_reg_field_get(unit, CMIC_TS_TIME_CAPTURE_CTRLr, regval, INT_ENABLEf); 

        *mode_flags = _bcm_esw_time_trigger_from_timestamp_mode (capture_mode);
#endif /* (defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)) */
    }
    
    return (BCM_E_NONE);
}


/*
 * Function:
 *      bcm_esw_time_heartbeat_register
 * Purpose:
 *      Registers a call back function to be called on each heartbeat
 * Parameters:
 *      unit        - (IN) StrataSwitch Unit #.
 *      id          - (IN) Time Sync Interface Id
 *      f           - (IN) Function to register
 *      user_data   - (IN) void pointer to store any user information
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_heartbeat_register (int unit, bcm_time_if_t id, bcm_time_heartbeat_cb f,
                                 void *user_data)
{
    soc_control_t   *soc = SOC_CONTROL(unit);

    /* Chek if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }

    /* Param validation check. */
    BCM_IF_ERROR_RETURN(
        _bcm_esw_time_interface_id_validate(unit, id));
    if (NULL == f) {
        return (BCM_E_PARAM);
    }

    TIME_LOCK(unit);
    /* If HW interrupt handler been registered protect from race condition */
    if (soc->time_call_ref_count) {
        soc->soc_time_callout = _bcm_esw_time_hw_interrupt_dflt;
    }

    /* Register user call back */
    TIME_INTERFACE_CONFIG(unit, id).user_cb->heartbeat_cb = f;
    TIME_INTERFACE_CONFIG(unit, id).user_cb->user_data = user_data;

    /* After registering user cb function and user_data turn on 
    HW interrupt handler */
    soc->soc_time_callout = _bcm_esw_time_hw_interrupt;
    /* Only single call back at a time is currently supported */
    soc->time_call_ref_count = 1; 

    TIME_UNLOCK(unit);
    
    return (BCM_E_NONE);
}

/*
 * Function:
 *      bcm_esw_time_heartbeat_unregister
 * Purpose:
 *      Unregisters a call back function to be called on each heartbeat
 * Parameters:
 *      unit        - (IN) StrataSwitch Unit #.
 *      id          - (IN) Time Sync Interface Id
 * Returns:
 *      BCM_E_NONE
 *      BCM_E_XXX
 *
 */
int 
bcm_esw_time_heartbeat_unregister (int unit, bcm_time_if_t id)
{
    soc_control_t   *soc = SOC_CONTROL(unit);

    /* Chek if time feature is supported on a device */
    if (!soc_feature(unit, soc_feature_time_support)) {
        return (BCM_E_UNAVAIL);
    }

    /* Param validation check. */
    BCM_IF_ERROR_RETURN(
        _bcm_esw_time_interface_id_validate(unit, id));

    TIME_LOCK(unit);

    if (--soc->time_call_ref_count <= 0) {
        soc->time_call_ref_count = 0;
        soc->soc_time_callout = _bcm_esw_time_hw_interrupt_dflt;
    }
    TIME_INTERFACE_CONFIG(unit, id).user_cb->heartbeat_cb = NULL;
    TIME_INTERFACE_CONFIG(unit, id).user_cb->user_data = NULL;

    TIME_UNLOCK(unit);
    
    return (BCM_E_NONE);
}


#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
/*
 * Function:
 *     _bcm_time_sw_dump
 * Purpose:
 *     Displays time information maintained by software.
 * Parameters:
 *     unit - Device unit number
 * Returns:
 *     None
 */
void
_bcm_time_sw_dump(int unit)
{
    soc_control_t   *soc = SOC_CONTROL(unit);

    LOG_CLI((BSL_META_U(unit,
                        "\nSW Information TIME - Unit %d\n"), unit));
    LOG_CLI((BSL_META_U(unit,
                        " Time call reference counter = %d\n"), soc->time_call_ref_count));
    if (NULL != soc->soc_time_callout) {
        LOG_CLI((BSL_META_U(unit,
                            " Time interrupt handler address is = %p\n"),
                 (void *)(unsigned long)soc->soc_time_callout));
    } else {
        LOG_CLI((BSL_META_U(unit,
                            " Time interrupt handler is NULL \n")));
    }
}
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */


/********************************** ESW BroadSync Time Support ***********************************/

#if defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)||defined(BCM_TRIDENT2_SUPPORT)||defined(BCM_HELIX4_SUPPORT)||defined(BCM_KATANA2_SUPPORT)
STATIC
int
_bcm_esw_time_bs_init(int unit, bcm_time_interface_t *intf)
{
    const int bspll_freq = 20000000; /* 20 MHz */

    uint32 regval = 0;

    int master = ((intf->flags & BCM_TIME_INPUT) == 0);
    int output_enable = master && ((intf->flags & BCM_TIME_ENABLE) != 0);

    int bitclk_divisor = bspll_freq / intf->bitclock_hz;
    int bitclk_high = bitclk_divisor / 2;
    int bitclk_low = bitclk_divisor - bitclk_high;
    int hb_divisor = intf->bitclock_hz / intf->heartbeat_hz;
    int hb_up = (hb_divisor > 200) ? 100 : (hb_divisor / 2);
    int hb_down = hb_divisor - hb_up;
    int bs_ndiv_int = 0;

    uint32 freq_ctrl_lower = 0x40000000;  /* 4ns increment, for 250MHz TSPLL output */

#if defined(BCM_TIME_V3_SUPPORT) || defined(BCM_KATANA2_SUPPORT) || defined(BCM_HELIX4_SUPPORT)
/*****************************************************************************/
/*         Perform KATANA2 and HELIX4 IPROC HW configuration                 */
/*****************************************************************************/
    if (soc_feature(unit, soc_feature_time_v3) || SOC_IS_KATANA2(unit) || SOC_IS_HELIX4(unit)) {
        freq_ctrl_lower = 4;  /* 4ns increment, for 250MHz TSPLL output */

        if (SOC_IS_HELIX4(unit)) {
            /* read NDIV_FRAC part of Feedback Divider Rate */
            READ_TOP_BS_PLL0_CTRL_3r(unit, &regval);
            bs_ndiv_int = soc_reg_field_get(unit, TOP_BS_PLL0_CTRL_3r, regval, NDIV_INTf);
        }
        else {
            /* read NDIV_FRAC part of Feedback Divider Rate */
            READ_TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r(unit, &regval);
            bs_ndiv_int = soc_reg_field_get(unit, TOP_TIME_SYNC_PLL_CTRL_REGISTER_3r, regval, NDIV_INTf);
        }

        /* configure BS0/BS1 Clock control registers (HIGH_DURATION / LOW_DURATION) */
        READ_CMIC_BS0_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_CLK_CTRLr, &regval, LOW_DURATIONf, bitclk_low);
        soc_reg_field_set(unit, CMIC_BS0_CLK_CTRLr, &regval, HIGH_DURATIONf, bitclk_high);
        WRITE_CMIC_BS0_CLK_CTRLr(unit, regval);

        READ_CMIC_BS0_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_CLK_CTRLr, &regval, ENABLEf, 1);
        WRITE_CMIC_BS0_CLK_CTRLr(unit, regval);

        READ_CMIC_BS1_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_CLK_CTRLr, &regval, LOW_DURATIONf, bitclk_low);
        soc_reg_field_set(unit, CMIC_BS1_CLK_CTRLr, &regval, HIGH_DURATIONf, bitclk_high);
        WRITE_CMIC_BS1_CLK_CTRLr(unit, regval);

        READ_CMIC_BS1_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_CLK_CTRLr, &regval, ENABLEf, 1);
        WRITE_CMIC_BS1_CLK_CTRLr(unit, regval);

        /* configure BS0/BS1 Clock configuration registers */
        READ_CMIC_BS0_CONFIGr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, MODEf, master ? TIME_MODE_OUTPUT : TIME_MODE_INPUT);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, output_enable);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_HB_OUTPUT_ENABLEf, output_enable);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, output_enable);
        WRITE_CMIC_BS0_CONFIGr(unit, regval);

        READ_CMIC_BS1_CONFIGr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &regval, MODEf, master ? TIME_MODE_OUTPUT : TIME_MODE_INPUT);
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, output_enable);
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &regval, BS_HB_OUTPUT_ENABLEf, output_enable);
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, output_enable);
        WRITE_CMIC_BS1_CONFIGr(unit, regval);

        READ_CMIC_TIMESYNC_TS0_FREQ_CTRL_FRACr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TS0_FREQ_CTRL_FRACr, &regval, FRACf, freq_ctrl_lower);
        WRITE_CMIC_TIMESYNC_TS0_FREQ_CTRL_FRACr(unit, regval);

        READ_CMIC_TIMESYNC_TS0_FREQ_CTRL_LOWERr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TS0_FREQ_CTRL_LOWERr, &regval, NSf, 0);
        WRITE_CMIC_TIMESYNC_TS0_FREQ_CTRL_LOWERr(unit, regval);

        READ_CMIC_TIMESYNC_TS0_FREQ_CTRL_UPPERr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TS0_FREQ_CTRL_UPPERr, &regval, NSf, 0);
        WRITE_CMIC_TIMESYNC_TS0_FREQ_CTRL_UPPERr(unit, regval);

        READ_CMIC_TIMESYNC_TS0_COUNTER_ENABLEr(unit, &regval);
        soc_reg_field_set(unit, CMIC_TIMESYNC_TS0_COUNTER_ENABLEr, &regval, ENABLEf, 1);
        WRITE_CMIC_TIMESYNC_TS0_COUNTER_ENABLEr(unit, regval);

        /* Setup BS0/BS1 HeartBeat Control registers */
        READ_CMIC_BS0_HEARTBEAT_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_HEARTBEAT_CTRLr, &regval, ENABLEf, 1);
        WRITE_CMIC_BS0_HEARTBEAT_CTRLr(unit, regval);

        READ_CMIC_BS1_HEARTBEAT_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_HEARTBEAT_CTRLr, &regval, ENABLEf, 1);
        WRITE_CMIC_BS1_HEARTBEAT_CTRLr(unit, regval);

        READ_CMIC_BS0_HEARTBEAT_DOWN_DURATIONr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_HEARTBEAT_DOWN_DURATIONr, &regval, DOWN_DURATIONf, hb_down);
        WRITE_CMIC_BS0_HEARTBEAT_DOWN_DURATIONr(unit, regval);

        READ_CMIC_BS1_HEARTBEAT_DOWN_DURATIONr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_HEARTBEAT_DOWN_DURATIONr, &regval, DOWN_DURATIONf, hb_down);
        WRITE_CMIC_BS1_HEARTBEAT_DOWN_DURATIONr(unit, regval);

        READ_CMIC_BS0_HEARTBEAT_UP_DURATIONr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_HEARTBEAT_UP_DURATIONr, &regval, UP_DURATIONf, hb_up);
        WRITE_CMIC_BS0_HEARTBEAT_UP_DURATIONr(unit, regval);

        READ_CMIC_BS1_HEARTBEAT_UP_DURATIONr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_HEARTBEAT_UP_DURATIONr, &regval, UP_DURATIONf, hb_up);
        WRITE_CMIC_BS1_HEARTBEAT_UP_DURATIONr(unit, regval);

        READ_CMIC_BS0_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_CLK_CTRLr, &regval, ENABLEf, 0);
        WRITE_CMIC_BS0_CLK_CTRLr(unit, regval);

        READ_CMIC_BS1_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_CLK_CTRLr, &regval, ENABLEf, 0);
        WRITE_CMIC_BS1_CLK_CTRLr(unit, regval);

        READ_CMIC_BROADSYNC_REF_CLK_GEN_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BROADSYNC_REF_CLK_GEN_CTRLr, &regval, ENABLEf, 0);
        WRITE_CMIC_BROADSYNC_REF_CLK_GEN_CTRLr(unit, regval);

        /* configure BS0/BS1 Clock control registers (HIGH_DURATION / LOW_DURATION) */
        READ_CMIC_BS0_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_CLK_CTRLr, &regval, LOW_DURATIONf, bitclk_low);
        soc_reg_field_set(unit, CMIC_BS0_CLK_CTRLr, &regval, HIGH_DURATIONf, bitclk_high);
        WRITE_CMIC_BS0_CLK_CTRLr(unit, regval);

        READ_CMIC_BS0_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_CLK_CTRLr, &regval, ENABLEf, 1);
        WRITE_CMIC_BS0_CLK_CTRLr(unit, regval);

        READ_CMIC_BS1_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_CLK_CTRLr, &regval, LOW_DURATIONf, bitclk_low);
        soc_reg_field_set(unit, CMIC_BS1_CLK_CTRLr, &regval, HIGH_DURATIONf, bitclk_high);
        WRITE_CMIC_BS1_CLK_CTRLr(unit, regval);

        READ_CMIC_BS1_CLK_CTRLr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_CLK_CTRLr, &regval, ENABLEf, 1);
        WRITE_CMIC_BS1_CLK_CTRLr(unit, regval);

        /* configure BS0/BS1 Clock configuration registers */
        READ_CMIC_BS0_CONFIGr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, MODEf, master ? TIME_MODE_OUTPUT : TIME_MODE_INPUT);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, output_enable);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_HB_OUTPUT_ENABLEf, output_enable);
        soc_reg_field_set(unit, CMIC_BS0_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, output_enable);
        WRITE_CMIC_BS0_CONFIGr(unit, regval);

        READ_CMIC_BS1_CONFIGr(unit, &regval);
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &regval, MODEf, master ? TIME_MODE_OUTPUT : TIME_MODE_INPUT);
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, output_enable);
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &regval, BS_HB_OUTPUT_ENABLEf, output_enable);
        soc_reg_field_set(unit, CMIC_BS1_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, output_enable);
        WRITE_CMIC_BS1_CONFIGr(unit, regval);

    } else
#endif
/*****************************************************************************/
/*                Non-KATANA2 or HELIX4 HW configuration                     */
/*****************************************************************************/
    {
#if defined(BCM_KATANA_SUPPORT)
        if (SOC_IS_KATANA(unit)) {
            READ_TOP_BROAD_SYNC_PLL_CTRL_REGISTER_3r(unit, &regval);
            bs_ndiv_int = soc_reg_field_get(unit, TOP_BROAD_SYNC_PLL_CTRL_REGISTER_3r,
                                            regval, NDIV_INTf);
        }
#endif /* defined(BCM_KATANA_SUPPORT) */

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TRIUMPH3(unit)) {
            READ_TOP_BS_PLL_CTRL_3r(unit, &regval);
            bs_ndiv_int = soc_reg_field_get(unit, TOP_BS_PLL_CTRL_3r,
                                            regval, NDIV_INTf);
            bs_ndiv_int = (regval & 0x3ff);
        }
#endif /* defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_TRIDENT2_SUPPORT) */

        if (bs_ndiv_int == 300) {
            /* Take a BroadSync NDIV_INT value of 0 as a signal that the input
             * reference frequency is 12.8 MHz, so TSPLL will produce 248MHz,
             * not 250MHz
             */
            freq_ctrl_lower = 0x40842108;
        }

#if defined(BCM_TRIDENT2_SUPPORT)
        if (SOC_IS_TD2_TT2(unit)) {
            /* m top_soft_reset_reg_2 top_bs_pll_rst_l=0 top_bs_pll_post_rst_l=0 */
            READ_TOP_SOFT_RESET_REG_2r(unit, &regval);
            soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &regval, TOP_BS_PLL_RST_Lf, 0);
            soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &regval, TOP_BS_PLL_POST_RST_Lf, 0);
            WRITE_TOP_SOFT_RESET_REG_2r(unit, regval);

            /* m TOP_BS_PLL_CTRL_0 VCO_DIV2=1 */
            READ_TOP_BS_PLL_CTRL_0r(unit, &regval);
            soc_reg_field_set(unit, TOP_BS_PLL_CTRL_0r, &regval, VCO_DIV2f, 1);
            WRITE_TOP_BS_PLL_CTRL_0r(unit, regval);

            /* m TOP_BS_PLL_CTRL_2 PDIV=1 CH0_MDIV=175 */
            READ_TOP_BS_PLL_CTRL_2r(unit, &regval);
            soc_reg_field_set(unit, TOP_BS_PLL_CTRL_2r, &regval, PDIVf, 1);
            soc_reg_field_set(unit, TOP_BS_PLL_CTRL_2r, &regval, CH0_MDIVf, 175);
            WRITE_TOP_BS_PLL_CTRL_2r(unit, regval);

            /* m top_soft_reset_reg_2 top_bs_pll_rst_l=1 */
            READ_TOP_SOFT_RESET_REG_2r(unit, &regval);
            soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &regval, TOP_BS_PLL_RST_Lf, 1);
            WRITE_TOP_SOFT_RESET_REG_2r(unit, regval);

            /* m top_soft_reset_reg_2 top_bs_pll_post_rst_l=1 */
            READ_TOP_SOFT_RESET_REG_2r(unit, &regval);
            soc_reg_field_set(unit, TOP_SOFT_RESET_REG_2r, &regval, TOP_BS_PLL_POST_RST_Lf, 1);
            WRITE_TOP_SOFT_RESET_REG_2r(unit, regval);
        }
#endif /* defined(BCM_TRIDENT2_SUPPORT) */

        if (SOC_IS_KATANA(unit) || SOC_IS_TRIUMPH3(unit) || SOC_IS_TD2_TT2(unit)) {
            /* m CMIC_BS_CLK_CTRL LOW_DURATION=1 HIGH_DURATION=1 */
            READ_CMIC_BS_CLK_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CLK_CTRLr, &regval, LOW_DURATIONf, bitclk_low);
            soc_reg_field_set(unit, CMIC_BS_CLK_CTRLr, &regval, HIGH_DURATIONf, bitclk_high);
            WRITE_CMIC_BS_CLK_CTRLr(unit, regval);

            /* m CMIC_BS_CLK_CTRL ENABLE=1 */
            READ_CMIC_BS_CLK_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CLK_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_BS_CLK_CTRLr(unit, regval);

            /* m CMIC_BS_CONFIG BS_CLK_OUTPUT_ENABLE=1 BS_HB_OUTPUT_ENABLE=1 BS_TC_OUTPUT_ENABLE=1 */
            READ_CMIC_BS_CONFIGr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, output_enable);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, BS_HB_OUTPUT_ENABLEf, output_enable);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, BS_TC_OUTPUT_ENABLEf, output_enable);
            WRITE_CMIC_BS_CONFIGr(unit, regval);

            READ_CMIC_TS_FREQ_CTRL_LOWERr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_FREQ_CTRL_LOWERr, &regval, FRACf, freq_ctrl_lower);
            WRITE_CMIC_TS_FREQ_CTRL_LOWERr(unit, regval);

            /* m CMIC_TS_FREQ_CTRL_UPPER ENABLE=1 */
            READ_CMIC_TS_FREQ_CTRL_UPPERr(unit, &regval);
            soc_reg_field_set(unit, CMIC_TS_FREQ_CTRL_UPPERr, &regval, ENABLEf, 1);
            WRITE_CMIC_TS_FREQ_CTRL_UPPERr(unit, regval);

            /* m CMIC_BS_HEARTBEAT_CTRL ENABLE=1 */
            READ_CMIC_BS_HEARTBEAT_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_HEARTBEAT_CTRLr, &regval, ENABLEf, 1);
            WRITE_CMIC_BS_HEARTBEAT_CTRLr(unit, regval);

            /* m CMIC_BS_HEARTBEAT_DOWN_DURATION DOWN_DURATION=2400 */
            READ_CMIC_BS_HEARTBEAT_DOWN_DURATIONr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_HEARTBEAT_DOWN_DURATIONr, &regval, DOWN_DURATIONf, hb_down);
            WRITE_CMIC_BS_HEARTBEAT_DOWN_DURATIONr(unit, regval);

            /* m CMIC_BS_HEARTBEAT_UP_DURATION UP_DURATION=100 */
            READ_CMIC_BS_HEARTBEAT_UP_DURATIONr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_HEARTBEAT_UP_DURATIONr, &regval, UP_DURATIONf, hb_up);
            WRITE_CMIC_BS_HEARTBEAT_UP_DURATIONr(unit, regval);

            /* m CMIC_BS_CLK_CTRL ENABLE=0 */
            READ_CMIC_BS_CLK_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CLK_CTRLr, &regval, ENABLEf, 0);
            WRITE_CMIC_BS_CLK_CTRLr(unit, regval);

            /* m CMIC_BS_REF_CLK_GEN_CTRL ENABLE=0 */
            /* BS_REF_CLK from BS_PLL_CLK rather than BS_REF_CLK_generated */

            READ_CMIC_BS_REF_CLK_GEN_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_REF_CLK_GEN_CTRLr, &regval, ENABLEf, 0);
            WRITE_CMIC_BS_REF_CLK_GEN_CTRLr(unit, regval);

            /* m CMIC_BS_CLK_CTRL ENABLE=1 LOW_DURATION=1 HIGH_DURATION=1 */
            READ_CMIC_BS_CLK_CTRLr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CLK_CTRLr, &regval, ENABLEf, 1);
            soc_reg_field_set(unit, CMIC_BS_CLK_CTRLr, &regval, LOW_DURATIONf, bitclk_low);
            soc_reg_field_set(unit, CMIC_BS_CLK_CTRLr, &regval, HIGH_DURATIONf, bitclk_high);
            WRITE_CMIC_BS_CLK_CTRLr(unit, regval);

            /* m CMIC_BS_CONFIG MODE=1 BS_CLK_OUTPUT_ENABLE=1 BS_HB_OUTPUT_ENABLE=1 */
            READ_CMIC_BS_CONFIGr(unit, &regval);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, MODEf, master ? TIME_MODE_OUTPUT : TIME_MODE_INPUT);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, BS_CLK_OUTPUT_ENABLEf, output_enable);
            soc_reg_field_set(unit, CMIC_BS_CONFIGr, &regval, BS_HB_OUTPUT_ENABLEf, output_enable);
            WRITE_CMIC_BS_CONFIGr(unit, regval);
        }
    }

    return BCM_E_NONE;
}
#endif /*defined(BCM_KATANA_SUPPORT) || defined(BCM_TRIUMPH3_SUPPORT)||defined(BCM_TRIDENT2_SUPPORT)||defined(BCM_KATANA2_SUPPORT)||defined(BCM_HELIX4_SUPPORT)||defined(BCM_KATANA2_SUPPORT)*/

