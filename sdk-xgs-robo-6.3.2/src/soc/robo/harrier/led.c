/*
 * $Id: led.c 1.6 Broadcom SDK $
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
 */
 
#include <soc/robo.h>
#include <soc/drv.h>
#include <soc/robo/mcm/driver.h>
#include <soc/error.h>

#include <soc/debug.h>

/* ROBO LED solution about LED mode :
 *  
 * - Can be retrieved from led_mode_map_0 and led_mode_map_1 
 *   at specific port bit. The combined value is the led mode.
 *  >> Mode[1:0]
 *  >> b00 : Off
 *  >> b01 : On
 *  >> b10 : Blink
 *  >> b11 : Auto
 */
#define ROBO_LED_MODE_RETRIEVE(_mode0, _mode1)   \
        (((_mode1) == 0) ? \
            (((_mode0) == 0) ? DRV_LED_MODE_OFF : DRV_LED_MODE_ON) : \
            (((_mode0) == 0) ? DRV_LED_MODE_BLINK : DRV_LED_MODE_AUTO))

#define ROBO_LED_MODE_RESOLVE(mode, _mode0, _mode1)   \
        if((mode) == DRV_LED_MODE_OFF) {\
            (_mode1) = 0;   \
            (_mode0) = 0;   \
        } else if ((mode) == DRV_LED_MODE_ON){  \
            (_mode1) = 0;   \
            (_mode0) = 1;   \
        } else if ((mode) == DRV_LED_MODE_BLINK){   \
            (_mode1) = 1;   \
            (_mode0) = 0;   \
        } else if ((mode) == DRV_LED_MODE_AUTO){    \
            (_mode1) = 1;   \
            (_mode0) = 1;   \
        } else {    \
            (_mode1) = -1;  \
            (_mode0) = -1;  \
        }
        


/*
 * Function: 
 *	    drv_harrier_led_mode_get
 * Purpose:
 *	    Get the port based LED working mode.
 * Parameters:
 *	    port        - (IN) local port id.
 *      led_mode    - (OUT) led function group id
 * 
 * Note:
 *	1. this routine is designed for 64bits length process.
 */
int
drv_harrier_led_mode_get(int unit, 
                int port, uint32 *led_mode)
{
    uint64  mode64_val0, mode64_val1;
    uint32  fld32_val = 0;
    int     temp0, temp1;

    SOC_IF_ERROR_RETURN(
            REG_READ_LED_MODE_MAP_0r(unit, (uint32 *)&mode64_val0));
    SOC_IF_ERROR_RETURN(
            REG_READ_LED_MODE_MAP_1r(unit, (uint32 *)&mode64_val1));
    
    SOC_IF_ERROR_RETURN(soc_LED_MODE_MAP_0r_field_get
            (unit, (uint32 *)&mode64_val0, LED_MODE_MAPf, 
            &fld32_val));
    temp0 = (fld32_val & (0x1 << port)) ? 1 : 0;
    
    SOC_IF_ERROR_RETURN(soc_LED_MODE_MAP_1r_field_get
            (unit,  (uint32 *)&mode64_val1, LED_MODE_MAPf, 
            &fld32_val));
    temp1 = (fld32_val & (0x1 << port)) ? 1 : 0;
    
    *led_mode = ROBO_LED_MODE_RETRIEVE(temp0, temp1);
    
    return SOC_E_NONE;
}

/*
 * Function: 
 *	    drv_harrier_led_mode_set
 * Purpose:
 *	    Set the port based LED working mode.
 * Parameters:
 *	    port        - (IN) local port id.
 *      led_mode    - (OUT) led function group id
 * 
 * Note:
 *	1. this routine is designed for 64bits length process.
 */
int
drv_harrier_led_mode_set(int unit,
                int port, uint32 led_mode)
{
    uint64  mode64_val0, mode64_val1;
    uint64  temp64;
    uint32  fld32_val = 0, temp_h = 0, temp_l = 0;
    int     temp0, temp1;

    ROBO_LED_MODE_RESOLVE(led_mode, temp0, temp1);
    if (temp0 == -1 && temp1 == -1 ) {
        return SOC_E_PARAM;
    } else {
        SOC_IF_ERROR_RETURN(
                REG_READ_LED_MODE_MAP_0r(unit, (uint32 *)&mode64_val0));
        SOC_IF_ERROR_RETURN(
                REG_READ_LED_MODE_MAP_1r(unit, (uint32 *)&mode64_val1));
    }

    /* set the target port bit in the uint64 bitmap format */
    temp_h = 0;
    temp_l = 1;
    COMPILER_64_SET(temp64, temp_h, temp_l);
    COMPILER_64_SHL(temp64, port);
        
    SOC_IF_ERROR_RETURN(soc_LED_MODE_MAP_0r_field_get
            (unit, (uint32 *)&mode64_val0, LED_MODE_MAPf, 
            &fld32_val));
    if (temp0 == 0){
        fld32_val &= ~(0x1 << port); 
    } else {
        fld32_val |= (0x1 << port); 
    }
    SOC_IF_ERROR_RETURN(soc_LED_MODE_MAP_0r_field_set
            (unit, (uint32 *)&mode64_val0, LED_MODE_MAPf, 
            &fld32_val));
    
    SOC_IF_ERROR_RETURN(soc_LED_MODE_MAP_1r_field_get
            (unit, (uint32 *)&mode64_val1, LED_MODE_MAPf, 
            &fld32_val));
    if (temp1 == 0){
        fld32_val &= ~(0x1 << port); 
    } else {
        fld32_val |= (0x1 << port); 
    }
    SOC_IF_ERROR_RETURN(soc_LED_MODE_MAP_1r_field_set
            (unit, (uint32 *)&mode64_val1, LED_MODE_MAPf, 
            &fld32_val));

    SOC_IF_ERROR_RETURN(
            REG_WRITE_LED_MODE_MAP_0r(unit, (uint32 *)&mode64_val0));
    SOC_IF_ERROR_RETURN(
            REG_WRITE_LED_MODE_MAP_1r(unit, (uint32 *)&mode64_val1));
    
    return SOC_E_NONE;
}

/*
 * Function: 
 *	    drv_harrier_led_funcgrp_select_get
 * Purpose:
 *	    Get the port based selctions of working LED function group.
 * Parameters:
 *	    port        - (IN) local port id.
 *      led_group   - (OUT) led function group id
 * 
 * Note:
 *	1. this routine is designed for 64bits length process.
 */
int
drv_harrier_led_funcgrp_select_get(int unit,
                int port, int *led_group)
{
    uint32  fld32_val = 0;
    uint64  reg64_val;
    
    SOC_IF_ERROR_RETURN(
            REG_READ_LED_FUNC_MAPr(unit, (uint32 *)&reg64_val));

    SOC_IF_ERROR_RETURN(soc_LED_FUNC_MAPr_field_get
            (unit, (uint32 *)&reg64_val, LED_FUNC_MAPf, 
             (uint32 *)&fld32_val));
    *led_group = (fld32_val & (0x1 << port)) ? 
            DRV_LED_FUNCGRP_1 : DRV_LED_FUNCGRP_0;

    return SOC_E_NONE;
}

/*
 * Function: 
 *	    drv_harrier_led_funcgrp_select_set
 * Purpose:
 *	    Set the port based selctions of working LED function group.
 * Parameters:
 *	    port        - (IN) local port id
 *      led_group   - (IN) led function group id
 * 
 * Note:
 *	1. this routine is designed for 64bits length process.
 */
int
drv_harrier_led_funcgrp_select_set(int unit, 
                int port, int led_group)
{
    uint64  reg64_val;
    uint32  fld32_val = 0;
    int     ledgrp = 0;
    
    ledgrp = (led_group == DRV_LED_FUNCGRP_0) ? 0 : 
            (led_group == DRV_LED_FUNCGRP_1) ? 1 : -1;
    if (ledgrp == -1){
        return SOC_E_PARAM;
    }
    
    SOC_IF_ERROR_RETURN(
            REG_READ_LED_FUNC_MAPr(unit, (uint32 *)&reg64_val));
    
    SOC_IF_ERROR_RETURN(soc_LED_FUNC_MAPr_field_get
            (unit, (uint32 *)&reg64_val, LED_FUNC_MAPf, 
             (uint32 *)&fld32_val));
    if (ledgrp == 0){
        fld32_val &= ~(0x1 << port);
    } else {
        fld32_val |= 0x1 << port;
    }
    SOC_IF_ERROR_RETURN(soc_LED_FUNC_MAPr_field_set
            (unit, (uint32 *)&reg64_val, LED_FUNC_MAPf, 
             (uint32 *)&fld32_val));
    SOC_IF_ERROR_RETURN(REG_WRITE_LED_FUNC_MAPr
            (unit, (uint32 *)&reg64_val));

    return SOC_E_NONE;
}

