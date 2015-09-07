/*
 * $Id: AlarmProcessCtc.h,v 1.4 Broadcom SDK $
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
 * File:     AlarmProcessCtc.h
 * Purpose: 
 *
 */

#ifndef _SOC_EA_AlarmProcessCtc_H
#define _SOC_EA_AlarmProcessCtc_H

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/AlarmProcess.h>    


typedef enum {
    PowerDownAlmState       = 0x00000001,
    PowerVoltOverAlmState   = 0x00000002,
    PowerVoltLessAlmState   = 0x00000003,
    PowerEndAlmState        = 0x7FFFFFFF
} PACK OamPowerAlarmState;

typedef enum {
    PonIfPonLosAlmState         = 0x00000001,
    PonIfMpcpTimeoutAlmState    = 0x00000002,
    PonErThresholdOverAlmState  = 0x00000003,
    PonIfEndAlmState            = 0x7FFFFFFF
} PACK OamPonIfSwitchAlarmState;


/*
 * OamCtcInstNumS - the CTC instance  number struct
 */
typedef struct {
    uint8   portType;
    uint8   slotNum;
    uint16  portNum;
} PACK OamCtcInstNumS;

/*
 * OamCtcInstNumU - the CTC instance number union
 */
typedef union {
    OamCtcInstNumS  sNum;
    uint32          uNum;
} PACK OamCtcInstNumU;

/*
 * OamEventCtcAlarm - the CTC Alarm
 */
 typedef struct {
    OamEventExt     ext;
    uint16          objType;
    OamCtcInstNumU  instNum;
    uint16          almID;
    uint16          timeStamp;
    uint8           state;
    uint8           info[0];
} PACK OamCtcEventAlarm;

typedef struct {
    uint16 obj_type;
    uint32 obj_num;
    uint16 alm_id;
    uint16 time_stamp;
    uint8  alm_state;
    uint8  info[1];
} PACK OamEventCtcAlarm;

typedef struct {
    uint32 info;
} PACK OamEventCtcAlarmPonIfInfo;

typedef struct {
    uint32 info;
} PACK OamEventCtcAlarmOnuIfInfo;


uint32 
CtcEventAlarmToBcmEvents(uint32 alarm_id);

uint32
CtcEventBcmEventsToCtcAlarm(uint32 event_id);

int32 
CtcEventOnuAlarm(uint8 path_id, uint8 link_id, void *event_tlv);

int32 
CtcEventPonIfAlarm(uint8 path_id, uint8 link_id, void *event_tlv);

int32 
CtcEventPortAlarm(uint8 path_id, uint8 link_id, void *event_tlv);

int32   
CtcEventProcessHandler (uint8 pathId, uint8 linkId, 
    OamEventExt * pOamEventTlv);

#if defined(__cplusplus)
}
#endif

#endif /* _SOC_EA_AlarmProcessCtc_H */
