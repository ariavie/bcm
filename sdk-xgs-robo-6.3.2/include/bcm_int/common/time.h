/*
 * $Id: time.h 1.5 Broadcom SDK $
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
 * This file contains Time Interface definitions internal to the BCM library.
 *
 */

#ifndef _BCM_INT_COMMON_TIME_H
#define _BCM_INT_COMMON_TIME_H

#include <sal/core/sync.h>


/* Time module internal defines */
#define TIME_EPOCH_MAX          0x7FFF       /* Max epoch value allowed in HW */
#define TIME_SEC_MAX            0xFFFFFFFF   /* HW supports only 32 bits second */
#define TIME_NANOSEC_MAX        1000000000   /* Max nanosec value is 10^9 */
#define TIME_DRIFT_DENOMINATOR  1000000000   /* Drift denominator = 1 second */
#define TIME_DRIFT_MAX       TIME_DRIFT_DENOMINATOR / 8

#define TIME_ACCURACY_CLK_MAX        19     /* How many accuracy values we can have */

#define TIME_MODE_INPUT              0x0    /* Time interface will recieve the time as input(Slave)*/
#define TIME_MODE_OUTPUT             0x1    /* Time interface will output the time (Master)*/

#define TIME_CAPTURE_MODE_DISABLE    0x0    /* Disable HW time capture */
#define TIME_CAPTURE_MODE_IMMEDIATE  0x1    /* Capture time immediately on CPU request */
#define TIME_CAPTURE_MODE_HEARTBEAT  0x2    /* Capture time on rising edge of heartbeat */
#define TIME_CAPTURE_MODE_GPIO_0     0x4    /* Capture time on GPIO event */
#define TIME_CAPTURE_MODE_GPIO_1     0x8
#define TIME_CAPTURE_MODE_GPIO_2     0x10
#define TIME_CAPTURE_MODE_GPIO_3     0x20
#define TIME_CAPTURE_MODE_GPIO_4     0x40
#define TIME_CAPTURE_MODE_GPIO_5     0x80
#define TIME_CAPTURE_MODE_L1_CLOCK_PRIMARY   0x100 /* Capture on L1 Clock */
#define TIME_CAPTURE_MODE_L1_CLOCK_SECONDARY 0x200 
#define TIME_CAPTURE_MODE_LCPLL      0x400  /* Capture on LCPLL */
#define TIME_CAPTURE_MODE_IP_DM_0    0x800
#define TIME_CAPTURE_MODE_IP_DM_1    0x1000


#define TIME_HEARTBEAT_HZ_CLK       125000000   /* 125 MHz */
#define TIME_HEARTBEAT_HZ_MAX       8000        /* 8 KHz */
#define TIME_HEARTBEAT_HZ_MIN       1           /* 1 Hz */
#define TIME_HEARTBEAT_NS_HZ        1000000000  /* 10^9 ns in 1 HZ */


 /* Full cycle BS clock frequencies values in nanoseconds */
#define TIME_BS_FREQUENCY_1000NS     1000      
#define TIME_BS_FREQUENCY_1024NS     1024
#define TIME_BS_FREQUENCY_1544NS     1544
#define TIME_BS_FREQUENCY_2000NS     2000
#define TIME_BS_FREQUENCY_2048NS     2048

#define TIME_BS_FREQUENCIES_NUM         5       /* Total number of supported frequencies */

#define TIME_TS_CLK_FREQUENCY_40NS  40    /* 25Mhz TS_CLK */

#define TIME_INTERFACE_ID_MAX(unit)  NUM_TIME_INTERFACE(unit) - 1

typedef enum _bcm_time_message_type_e {
    _BCM_TIME_FREQ_OFFSET_SET,
    _BCM_TIME_FREQ_OFFSET_GET,
    _BCM_TIME_PHASE_OFFSET_SET,
    _BCM_TIME_PHASE_OFFSET_GET,
    _BCM_TIME_DEBUG_1PPS_SET,
    _BCM_TIME_DEBUG_1PPS_GET,
    _BCM_TIME_STATUS_GET,
    _BCM_TIME_LOG_SET,
    _BCM_TIME_MAX_MESSAGE_NUM
} _bcm_time_message_type_t;


/* User call back management structure */
typedef struct _bcm_time_user_cb_s {
    bcm_time_heartbeat_cb  heartbeat_cb;    /* Callback function on hearbeat */
    void                   *user_data;      /* User data provided */
} _bcm_time_user_cb_t, *_bcm_time_user_cb_p;

/* Time interface managment structure. */
typedef struct _bcm_time_interface_config_s {
    bcm_time_interface_t    time_interface; /* Time Interface structure */
    bcm_time_capture_t      time_capture;   /* Time capture structure   */
    int                     ref_count;      /* Reference count.         */
    _bcm_time_user_cb_p     user_cb;        /* User call back info      */
} _bcm_time_interface_config_t, *_bcm_time_interface_config_p;
                         

/* Module control structure. */
typedef struct _bcm_time_config_s {
    _bcm_time_interface_config_p    intf_arr;       /* Time Interface config array */
    int                             intf_count;     /* Time interfaces size.       */
    sal_mutex_t                     mutex;          /* Protection mutex.           */
} _bcm_time_config_t, *_bcm_time_config_p;


/* Verify that time module is initialized */
#define TIME_INIT(unit) (NULL != _bcm_time_config[unit])
/* Time control configuration per unit */
#define TIME_CONFIG(unit) (_bcm_time_config[unit])
/* Time interface configuration */
#define TIME_INTERFACE_CONFIG(unit, idx) ((TIME_CONFIG(unit))->intf_arr[idx])
/* Time interface  */
#define TIME_INTERFACE(unit, idx) &TIME_INTERFACE_CONFIG(unit,idx).time_interface
/* Time capture */
#define TIME_CAPTURE(unit, idx) &TIME_INTERFACE_CONFIG(unit, idx).time_capture

/* Time interface configuration reference count */
#define TIME_INTERFACE_CONFIG_REF_COUNT(unit, idx) (TIME_INTERFACE_CONFIG(unit, idx).ref_count)

/* Time module lock */
#define TIME_LOCK(unit) sal_mutex_take(TIME_CONFIG(unit)->mutex, sal_mutex_FOREVER)
/* Time module unlock */
#define TIME_UNLOCK(unit)   sal_mutex_give(TIME_CONFIG(unit)->mutex)


#define TIME_NANOSEC_2_SEC_ROUND(ns)    ((ns > (TIME_NANOSEC_MAX / 2)) ? 1: 0)

#define TIME_ACCURACY_UNKNOWN               0
#define TIME_ACCURACY_INFINITE              TIME_NANOSEC_MAX
#define TIME_ACCURACY_UNKNOWN_IDX           18
#define TIME_ACCURACY_LOW_HW_VAL            32
#define TIME_ACCURACY_HIGH_HW_VAL           49
#define TIME_ACCURACY_UNKNOWN_HW_VAL        254
 
#define TIME_ACCURACY_HW_2_SW_IDX(val)     ((val == TIME_ACCURACY_UNKNOWN_HW_VAL) ? TIME_ACCURACY_UNKNOWN_IDX : val - TIME_ACCURACY_LOW_HW_VAL)
#define TIME_ACCURACY_SW_IDX_2_HW(idx)     ((idx == TIME_ACCURACY_UNKNOWN_IDX) ? TIME_ACCURACY_UNKNOWN_HW_VAL : idx + TIME_ACCURACY_LOW_HW_VAL)


#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void _bcm_time_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#endif /* _BCM_INT_COMMON_TIME_H */

