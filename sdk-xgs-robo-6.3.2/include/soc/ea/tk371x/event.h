/*
 * $Id: event.h 1.1 Broadcom SDK $
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
 * File:     events.h
 * Purpose:
 *
 */
#ifndef _SOC_EA_EVENTS_H
#define _SOC_EA_EVENTS_H
#include <soc/ea/tk371x/CtcMiscApi.h>

extern uint32 CtcEventBcmEventsToCtcAlarm(uint32 event_id);

#define _soc_ea_alarm_threshold_t CtcOamAlarmThreshold

#define _soc_ea_alarm_enable_get(unit, port, alarm_id, state) \
    CtcExtOamGetAlarmState(unit, 0, port, \
    CtcEventBcmEventsToCtcAlarm(alarm_id), state)

#define _soc_ea_alarm_enable_set(unit, port, alarm_id, state) \
    CtcExtOamSetAlarmState(unit, 0, port, \
    CtcEventBcmEventsToCtcAlarm(alarm_id), state)

#define _soc_ea_alarm_threshold_get(unit, port, alarm_id, threshold) \
    CtcExtOamGetAlarmThreshold(unit, 0, port, \
    CtcEventBcmEventsToCtcAlarm(alarm_id), threshold)

#define _soc_ea_alarm_threshold_set(unit, port, alarm_id, threshold) \
    CtcExtOamSetAlarmThreshold(unit, 0, port, \
    CtcEventBcmEventsToCtcAlarm(alarm_id), threshold)

typedef enum soc_ea_events_val_e {
    SOC_EA_EVENT_EQUIPMENT_ALARM,
    SOC_EA_EVENT_POWER_ALARM,
    SOC_EA_EVENT_BATTERY_MISSING_ALARM,
    SOC_EA_EVENT_BATTERY_FAILURE_ALARM,
    SOC_EA_EVENT_BATTERY_VOLT_LOW_ALARM,
    SOC_EA_EVENT_INTRUSION_ALARM,
    SOC_EA_EVENT_TEST_FAILURE_ALARM,
    SOC_EA_EVENT_ONU_TEMP_HIGH_ALARM,
    SOC_EA_EVENT_ONU_TEMP_LOW_ALARM,
    SOC_EA_EVENT_IF_SWITCH_ALARM,
    SOC_EA_EVENT_RX_POWER_HIGH_ALARM,
    SOC_EA_EVENT_RX_POWER_LOW_ALARM,
    SOC_EA_EVENT_TX_POWER_HIGH_ALARM,
    SOC_EA_EVENT_TX_POWER_LOW_ALARM,
    SOC_EA_EVENT_TX_BIAS_HIGH_ALARM,
    SOC_EA_EVENT_TX_BIAS_LOW_ALARM,
    SOC_EA_EVENT_VCC_HIGH_ALARM,
    SOC_EA_EVENT_VCC_LOW_ALARM,
    SOC_EA_EVENT_TEMP_HIGH_ALARM,
    SOC_EA_EVENT_TEMP_LOW_ALARM,
    SOC_EA_EVENT_RX_POWER_HIGH_WARNING,
    SOC_EA_EVENT_RX_POWER_LOW_WARNING,
    SOC_EA_EVENT_TX_POWER_HIGH_WARNING,
    SOC_EA_EVENT_TX_POWER_LOW_WARNING,
    SOC_EA_EVENT_TX_BIAS_HIGH_WARNING,
    SOC_EA_EVENT_TX_BIAS_LOW_WARNING,
    SOC_EA_EVENT_VCC_HIGH_WARNING,
    SOC_EA_EVENT_VCC_LOW_WARNING,
    SOC_EA_EVENT_TEMP_HIGH_WARNING,
    SOC_EA_EVENT_TEMP_LOW_WARNING,
    SOC_EA_EVENT_AUTONEG_FAILURE_ALARM,
    SOC_EA_EVENT_LOS_ALARM,
    SOC_EA_EVENT_PORT_FAILURE_ALARM,
    SOC_EA_EVENT_PORT_LOOPBACK_ALARM,
    SOC_EA_EVENT_PORT_CONGESTION_ALARM,
    SOC_EA_EVENT_OAM_TIMEOUT_ALARM,
    SOC_EA_EVENT_KEY_EXCHANGE_FAILURE_ALARM,
    SOC_EA_EVENT_GPIO_LINK_FAULT_ALARM,
    SOC_EA_EVENT_LOOPBACK_TEST_ENABLE_ALARM,
    SOC_EA_EVENT_GPIO_CRICTICAL_EVENT_ALARM,
    SOC_EA_EVENT_GPIO_EXTERNAL_ALARM,
    SOC_EA_EVENT_AUTH_FAILURE_ALARM,
    SOC_EA_EVENT_STATS_ALARM,
    SOC_EA_EVENT_FLASH_BUSY_ALARM,
    SOC_EA_EVENT_ONU_READY_ALARM,
    SOC_EA_EVENT_DISABLE_ALARM,
    SOC_EA_EVENT_CTC_DISCOVERY_SUCCESS_ALARM,
    SOC_EA_EVENT_LAYSER_ALWAYS_ON_ALARM,
    SOC_EA_EVENT_RESERVED_ALARM,
    SOC_EA_EVENT_COUNT
} soc_ea_events_val_t;

#endif /* _SOC_EA_EVENTS_H */
