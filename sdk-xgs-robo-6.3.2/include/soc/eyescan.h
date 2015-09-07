/*
 * $Id: eyescan.h 1.12 Broadcom SDK $
 *
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
 * EYESCAN H
 */


#ifndef _SOC_EYESCAN_H_
#define _SOC_EYESCAN_H_

#include <soc/types.h>

#define SOC_PORT_PHY_EYESCAN_MAX_HORIZINAL_RESOLUTION 63
#define SOC_PORT_PHY_EYESCAN_MAX_VERTICAL_RESOLUTION 63 

#define SOC_PORT_PHY_EYESCAN_H_INDEX SOC_PORT_PHY_EYESCAN_MAX_HORIZINAL_RESOLUTION / 2
#define SOC_PORT_PHY_EYESCAN_V_INDEX SOC_PORT_PHY_EYESCAN_MAX_VERTICAL_RESOLUTION / 2

#define SRD_EYESCAN_INVALID 0xffffffff
#define SRD_EYESCAN_INVALID_VOFFSET 0x7fffffff

#define SRD_EYESCAN_FLAG_VERTICAL_ONLY 0x1
#define SRD_EYESCAN_FLAG_AVOID_EXTRAPOLATION 0x2
#define SRD_EYESCAN_FLAG_DSC_PRINT 0x4

#define TIME_MULTIPLIER 2

/* #define BER_EXTRAP_SPD_UP */
#define MAX_EYE_LOOPS 49
#define HI_CONFIDENCE_MIN_ERR_CNT 20 /* bit errors exit condition for high confidence */
#define INDEX_UNINITIALIZED  (-1)
#define VEYE_UNIT 1.75
#define EYESCAN_UTIL_MAX_ROUND_DIGITS (8) 

/*Eyescan test bounds*/
typedef struct soc_stat_resolution_bounds_e { 
    int horizontal_min; /* Typically 0 */
    int horizontal_max; 
    int vertical_min; /* Ignored */
    int vertical_max; /* Actual movement is [-max;max]*/
} soc_port_phy_eye_bounds_t; 

/*Select eyescan errors counting method*/
typedef enum soc_stat_eyscan_counter_e { 
    socPortPhyEyescanCounterRelativePhy = 0, 
    socPortPhyEyescanCounterPrbsPhy,
    socPortPhyEyescanCounterPrbsMac, 
    socPortPhyEyescanCounterCrcMac, 
    socPortPhyEyescanCounterBerMac, 
    socPortPhyEyescanCounterCustom,
    socPortPhyEyescanNofCounters /*last*/
} soc_port_phy_eyscan_counter_t;

/*Eyescan test parameters*/
typedef struct soc_stat_eyescan_params_s { 
    int sample_time; /*milisec*/ 
    int sample_resolution; /*interval between two samples*/ 
    soc_port_phy_eye_bounds_t bounds; 
    soc_port_phy_eyscan_counter_t counter;
    int error_threshold; /*minimum error threshold required, otherwise increase sample time*/
    int time_upper_bound; /*maximum sample time*/
    int nof_threshold_links; /*num of links that should be below error_threshold before increasing time*/
} soc_port_phy_eyescan_params_t;

/*Eyescan test results*/
typedef struct soc_stat_eyescan_results_s { 
    uint32 error_count[SOC_PORT_PHY_EYESCAN_MAX_HORIZINAL_RESOLUTION][SOC_PORT_PHY_EYESCAN_MAX_VERTICAL_RESOLUTION];   
    uint32 run_time[SOC_PORT_PHY_EYESCAN_MAX_HORIZINAL_RESOLUTION][SOC_PORT_PHY_EYESCAN_MAX_VERTICAL_RESOLUTION]; 
    int ext_results_int;
    int ext_results_remainder;
    int ext_better;     /* 1 for better than, 0 for exact, -1 for worse than */
    int ext_done;
} soc_port_phy_eyescan_results_t;

int soc_port_phy_eyescan_run( 
    int unit, 
	uint32 inst,
    int flags, 
    soc_port_phy_eyescan_params_t* params, 
    uint32 nof_ports, 
    soc_port_t* ports,
    int* lane_num,
    soc_port_phy_eyescan_results_t* results /*array of result per port*/);

int
soc_port_phy_eyescan_res_print(
    int unit,
    int sample_resolution,
    soc_port_phy_eye_bounds_t *bounds,
    soc_port_phy_eyescan_results_t *res);

typedef int (*port_phy_eyescan_counter_clear_f) (int unit, soc_port_t port);
typedef int (*port_phy_eyescan_counter_get_f) (int unit, soc_port_t port, uint32* err_count);

typedef struct soc_port_phy_eyescan_counter_cb_s {
    port_phy_eyescan_counter_clear_f clear_func;
    port_phy_eyescan_counter_get_f  get_func;
}soc_port_phy_eyescan_counter_cb_t;

typedef struct {
    int total_errs[MAX_EYE_LOOPS];
    int total_elapsed_time[MAX_EYE_LOOPS];
    int mono_flags[MAX_EYE_LOOPS];
    int offset_max;
    int veye_cnt;
    uint32 rate;      /* frequency in KHZ */
    int first_good_ber_idx;
    int first_small_errcnt_idx;
} EYE_DIAG_INFOt;

int 
soc_port_phy_eyescan_counter_register(
    int unit, 
    soc_port_phy_eyscan_counter_t counter, 
    soc_port_phy_eyescan_counter_cb_t* cf);

int soc_port_phy_eyescan_extrapolate(
    int unit, 
    int flags, 
    soc_port_phy_eyescan_params_t* params, 
    uint32 nof_ports, 
    soc_port_t* ports,
    soc_port_phy_eyescan_results_t* results /*array of result per port*/);

#endif   /* _SOC_EYESCAN_H_  */

