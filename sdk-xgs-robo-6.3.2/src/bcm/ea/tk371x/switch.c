/*
 * $Id: switch.c 1.5 Broadcom SDK $
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
 * File:     switch.c
 * Purpose:
 *
 */

#include <bcm/types.h>
#include <bcm/switch.h>
#include <bcm_int/tk371x_dispatch.h>
#include <bcm/error.h>
#include <soc/ea/tk371x/onu.h>
#include <soc/ea/tk371x/event.h>
#include <soc/drv.h>
#include <bcm_int/ea/tk371x/switch.h>


static bcm_tk371x_glb_alm_map_t
glb_alm_map[] = {
    {bcmSwitchEquipmentAlarmState,   
        SOC_EA_EVENT_EQUIPMENT_ALARM},  
    {bcmSwitchPowerAlarmState,       
        SOC_EA_EVENT_POWER_ALARM},  
    {bcmSwitchBatteryMissingAlarmState,  
        SOC_EA_EVENT_BATTERY_MISSING_ALARM},  
    {bcmSwitchBatteryFailureAlarmState,  
        SOC_EA_EVENT_BATTERY_FAILURE_ALARM },  
    {bcmSwitchBatteryVoltLowAlarmState,           
        SOC_EA_EVENT_BATTERY_VOLT_LOW_ALARM},  
    {bcmSwitchBatteryVoltLowAlarmReportThreshold, 
        SOC_EA_EVENT_BATTERY_VOLT_LOW_ALARM}, 
    {bcmSwitchBatteryVoltLowAlarmClearThreshold,  
        SOC_EA_EVENT_BATTERY_VOLT_LOW_ALARM}, 
    {bcmSwitchPhysicalIntrusionAlarmState,        
        SOC_EA_EVENT_INTRUSION_ALARM},
    {bcmSwitchSelfTestFailureAlarmState,          
        SOC_EA_EVENT_TEST_FAILURE_ALARM}, 
    {bcmSwitchPonIfSwitchAlarmState,              
        SOC_EA_EVENT_IF_SWITCH_ALARM}
};

static int
_bcm_tk371x_switch_get_alarm_id(
        uint16 ctrl_id, 
        uint16 * alarm_id)
{
    int i;
    int cnt = sizeof(glb_alm_map)/sizeof(bcm_tk371x_glb_alm_map_t);
    
    for (i = 0; i < cnt; i++) {
        if (ctrl_id == glb_alm_map[i].ctrl_id) {
            break;
        }
    }
    if (i >= cnt) {
        return BCM_E_PARAM;   
    }    
    * alarm_id = glb_alm_map[i].alm_id;
    return BCM_E_NONE;
}

int
bcm_tk371x_switch_control_get(
		int unit,
	    bcm_switch_control_t type,
	    int *arg)
{
    int rv;
    uint8 state;
    uint16 alarm_id;
    _soc_ea_alarm_threshold_t threshold;    
    _soc_ea_tk_fail_safe_state_t fail_safe;
    
    switch (type) {
        case bcmSwitchPonOamFailsafeState:
            rv = _soc_ea_fail_safe_state_get(unit, &fail_safe); 
            if (rv != OK) {
                return BCM_E_FAIL;   
            }
            if (fail_safe.failsafeCount > 0) {
                * arg = fail_safe.failSafeList[0];
            } else {
                * arg = 0;
            }
            return BCM_E_NONE;
            break;
            
        case bcmSwitchEquipmentAlarmState:
        case bcmSwitchPowerAlarmState:
        case bcmSwitchBatteryMissingAlarmState:
        case bcmSwitchBatteryFailureAlarmState:
        case bcmSwitchBatteryVoltLowAlarmState: 
        case bcmSwitchPhysicalIntrusionAlarmState:
        case bcmSwitchSelfTestFailureAlarmState: 
        case bcmSwitchPonIfSwitchAlarmState:
            BCM_IF_ERROR_RETURN(
                _bcm_tk371x_switch_get_alarm_id(type, &alarm_id));
            rv = _soc_ea_alarm_enable_get(unit, 0, alarm_id, &state);
            if (rv != OK) {
                return BCM_E_FAIL;
            } 
            * arg = state;
            return BCM_E_NONE;
            break;
            
        case bcmSwitchBatteryVoltLowAlarmReportThreshold: 
        case bcmSwitchBatteryVoltLowAlarmClearThreshold: 
            BCM_IF_ERROR_RETURN(
                _bcm_tk371x_switch_get_alarm_id(type, &alarm_id));
            rv = _soc_ea_alarm_threshold_get(unit, 0, alarm_id, &threshold);
            if (rv != OK) {
                return BCM_E_FAIL;   
            }
            if (type == bcmSwitchBatteryVoltLowAlarmReportThreshold) {
                * arg = threshold.raiseThreshold;
            } else {
                * arg = threshold.clearThreshold;
            }
            return BCM_E_NONE;
            break;
            
        default:
            return BCM_E_UNAVAIL;
            break;
    }
}

int
bcm_tk371x_switch_control_set(
		int unit,
	    bcm_switch_control_t type,
	    int arg)
{
    int rv;
    uint8 state;
    uint16 alarm_id;
    _soc_ea_alarm_threshold_t threshold;    
    _soc_ea_tk_fail_safe_state_t fail_safe;
    
	switch (type) {
        case bcmSwitchPonOamFailsafeState: 
            fail_safe.failsafeCount = 1;
            fail_safe.failSafeList[0] = arg;
            rv = _soc_ea_fail_safe_state_set(unit, &fail_safe); 
            if (rv != OK) {
                return BCM_E_FAIL;   
            }
            return BCM_E_NONE;
            break;
                        
        case bcmSwitchEquipmentAlarmState:
        case bcmSwitchPowerAlarmState:
        case bcmSwitchBatteryMissingAlarmState:
        case bcmSwitchBatteryFailureAlarmState: 
        case bcmSwitchBatteryVoltLowAlarmState:
        case bcmSwitchPhysicalIntrusionAlarmState:
        case bcmSwitchSelfTestFailureAlarmState:
        case bcmSwitchPonIfSwitchAlarmState:
            BCM_IF_ERROR_RETURN(
                _bcm_tk371x_switch_get_alarm_id(type, &alarm_id));
            state = (uint8)arg;
            rv = _soc_ea_alarm_enable_set(unit, 0, alarm_id, state);
            if (rv != OK) {
                return BCM_E_FAIL;
            }             
            return BCM_E_NONE;
            break;
            
        case bcmSwitchBatteryVoltLowAlarmReportThreshold:
        case bcmSwitchBatteryVoltLowAlarmClearThreshold: 
            BCM_IF_ERROR_RETURN(
                _bcm_tk371x_switch_get_alarm_id(type, &alarm_id));
                
            rv = _soc_ea_alarm_threshold_get(unit, 0, alarm_id, &threshold);
            if (rv != OK) {
                return BCM_E_FAIL;   
            }
            if (type == bcmSwitchBatteryVoltLowAlarmReportThreshold) {
                threshold.raiseThreshold = arg;
            } else {
                threshold.clearThreshold = arg;
            }            
            rv = _soc_ea_alarm_threshold_set(unit, 0, alarm_id, threshold);
            if (rv != OK) {
                return BCM_E_FAIL;   
            }
            return BCM_E_NONE;
            break;    
                    
        default:
            return BCM_E_UNAVAIL;
            break;
    }
}

int 
bcm_tk371x_switch_control_port_get(
        int unit, 
        bcm_port_t port, 
        bcm_switch_control_t type, 
        int *arg)
{
    return BCM_E_UNAVAIL;   
}

int 
bcm_tk371x_switch_control_port_set(
        int unit, 
        bcm_port_t port, 
        bcm_switch_control_t type, 
        int arg)
{	
	return BCM_E_UNAVAIL;
}

int 
bcm_tk371x_switch_event_register(
        int unit, 
        bcm_switch_event_cb_t cb, 
        void *userdata)
{
    return soc_event_register(unit, (soc_event_cb_t)cb, userdata);
	
}

int 
bcm_tk371x_switch_event_unregister(
        int unit, 
        bcm_switch_event_cb_t cb, 
        void *userdata)
{	
	return soc_event_unregister(unit, (soc_event_cb_t)cb, userdata);
}





