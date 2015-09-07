/*
 * $Id: switch.h 1.12 Broadcom SDK $
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
 * Common internal definitions for BCM switch module
 */

#ifndef _BCM_INT_SWITCH_H_
#define _BCM_INT_SWITCH_H_

#ifdef BCM_WARM_BOOT_SUPPORT

#include<soc/drv.h>

#if (defined(BCM_DPP_SUPPORT) && defined(BCM_WARM_BOOT_SUPPORT))
#include <bcm_int/dpp/switch.h>
#endif /* BCM_WARM_BOOT_SUPPORT */


/* Externs for the required functions */
#define BCM_DLIST_ENTRY(_dtype) \
extern int bcm_##_dtype##_switch_control_set( \
    int unit, bcm_switch_control_t type, int arg);
#include <bcm_int/bcm_dlist.h>

/* Dispatch table for this function, which uses the externs above */
#define BCM_DLIST_ENTRY(_dtype)\
bcm_##_dtype##_switch_control_set,
static int (*_switch_control_set_disp[])(
    int unit, 
    bcm_switch_control_t type, 
    int arg) =
{
#include <bcm_int/bcm_dlist.h>
0
};

/* Externs for the required functions */
#define BCM_DLIST_ENTRY(_dtype) \
extern int bcm_##_dtype##_switch_control_get( \
    int unit, bcm_switch_control_t type, int *arg);
#include <bcm_int/bcm_dlist.h>

/* Dispatch table for this function, which uses the externs above */
#define BCM_DLIST_ENTRY(_dtype)\
bcm_##_dtype##_switch_control_get,
static int (*_switch_control_get_disp[])(
    int unit, 
    bcm_switch_control_t type, 
    int *arg) =
{
#include <bcm_int/bcm_dlist.h>
0
};

#if defined(BCM_DPP_SUPPORT) && defined(BCM_WARM_BOOT_API_TEST)
extern int _bcm_switch_warmboot_reset_test(int unit);
#define BCM_WARMBOOT_RESET_TEST(_u)                                     \
    {                                                                   \
	   int warmboot_test_mode_enable;                                   \
       int no_wb_test;                                                  \
       if(sal_thread_self() == sal_thread_main_get()) {                 \
          _bcm_dpp_switch_warmboot_test_mode_get(unit,&warmboot_test_mode_enable); \
          _bcm_dpp_switch_warmboot_no_wb_test_get(unit,&no_wb_test);    \
                                                                        \
          if ((warmboot_test_mode_enable == 1) && (!no_wb_test)) {      \
              BCM_DEBUG(BCM_DBG_WARN, ("**** WB BCM API %s ****\n", FUNCTION_NAME())); \
          }                                                             \
          _bcm_switch_warmboot_reset_test(_u);                          \
       }                                                                \
    }                                                         
#else
#define BCM_WARMBOOT_RESET_TEST(_u)
#endif /* BCM_DPP_SUPPORT */
#define BCM_STATE_SYNC(_u)                                            \
    {                                                                 \
        if (SOC_UNIT_VALID(_u)) {                                     \
             int arg3;                                                \
             int rv = _switch_control_get_disp[dtype](_u,             \
                          bcmSwitchControlAutoSync, &arg3);           \
             if (BCM_SUCCESS(rv) && (arg3 != 0)) {                    \
                 if (SOC_CONTROL(_u)->scache_dirty) {                 \
                     r_rv = _switch_control_set_disp[dtype](_u,       \
                              bcmSwitchControlSync, 1);               \
                 }                                                    \
             }                                                        \
        BCM_WARMBOOT_RESET_TEST(_u);                                  \
        }                                                             \
    }
#else 
#define BCM_STATE_SYNC(_u)
#endif /* BCM_WARM_BOOT_SUPPORT */




#endif /* _BCM_INT_SWITCH_H_ */
