/*
 * $Id: TkRuleApi.h 1.1 Broadcom SDK $
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
 * File:     TkRuleApi.h
 * Purpose: 
 *
 */

#ifndef _SOC_EA_TkRuleApi_H
#define _SOC_EA_TkRuleApi_H

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkDefs.h>
#include <soc/ea/tk371x/Oam.h>


typedef struct {
    uint8 cntOfUpQ;
    uint8 sizeOfUpQ [MAX_CNT_OF_UP_QUEUE];
} TkLinkConfigInfo;

typedef struct {
    uint8 cntOfDnQ;
    uint8 sizeOfDnQ [MAX_CNT_OF_UP_QUEUE];
} TkPortConfigInfo;

typedef struct {
    uint8 cntOfDnQ;
    uint8 sizeOfDnQ [MAX_CNT_OF_UP_QUEUE];
} TkMcastConfigInfo;

typedef enum{
    TkObjTypeLink = 1,
    TkObjTypePort = 3, 
} TkOamObjectType;

typedef struct {
	TkOamObjectType objType;
	uint8           index;
}LogicalPortIndex;

typedef struct {
    uint8               cntOfLink;
    TkLinkConfigInfo    linkInfo[MAX_CNT_OF_LINK];
    uint8               cntOfPort;
    TkPortConfigInfo    portInfo[2];
    TkMcastConfigInfo   mcastInfo;
} TkQueueConfigInfo;


typedef struct {
    uint8   field;
    union {
        uint8   da[6];
        uint8   sa[6];
        uint16  etherType;
        uint16  vlan;
        uint8   priority;
        uint8   ipProtocol;
        uint8   value[8];
    } common;
    uint8   operator;
} TkRuleCondition;

typedef struct {
    uint32          conditionCount;
    TkRuleCondition conditionList[8];
} TkRuleConditionList;

typedef struct {
    uint8       port_link;
    uint8       queue;
} TkRuleNameQueue;

typedef union {
    TkRuleNameQueue ndest;
    uint16          vid_cos;
} TkRulePara;

typedef struct {
    uint8 volatiles;
    uint8 priority;
    TkRuleConditionList ruleCondition;
    uint8 action;
    TkRulePara param;
}TkPortRuleInfo;

int     TkExtOamGetFilterRulesByPort (uint8 pathId, uint8 linkId,
                OamRuleDirection direction, uint8 portIndex,
                uint8 * pRxBuf, uint32 * pRxLen);

int     TkExtOamClearAllFilterRulesByPort (uint8 pathId, uint8 linkId,
                OamRuleDirection direction,
                uint8 portIndex);

int     TkExtOamClearAllUserRulesByPort (uint8 pathId, uint8 linkId,
                OamRuleDirection direction, uint8 portIndex);

int     TkExtOamClearAllClassifyRulesByPort (uint8 pathId, uint8 linkId,
                OamRuleDirection direction,
                uint8 portIndex);

int32   TkRuleActDiscard (uint8 pathId, uint8 linkId, uint8 volatiles,
                uint8 portIndex, uint8 pri, uint8 numOfConditon,
                OamNewRuleCondition * pCondList, uint8 action);

int32   TkRuleDiscardDsArpWithSpecifiedSenderIpAddr (uint8 pathId,
                uint32 ipAddress, uint8 action);

int32   TkQueueGetConfiguration (uint8 pathId, 
                TkQueueConfigInfo * pQueueConfigInfo);

int32   TkQueueSetConfiguration (uint8 pathId, 
                TkQueueConfigInfo * pQueueConfigInfo);


int32   TkAddOneRuleByPort (uint8 pathId, uint8 linkId, uint8 volatiles,
                uint8 portIndex, uint8 pri,
                TkRuleConditionList * ruleCondition, uint8 action,
                TkRulePara * param);

int32   TkDelOneRuleByPort (uint8 pathId, uint8 linkId, uint8 volatiles,
                uint8 portIndex, uint8 pri,
                TkRuleConditionList * ruleCondition, uint8 action,
                TkRulePara * param);

int32   TkExtOamPortRuleDel(uint8 pathId,uint8 linkId, LogicalPortIndex port, 
                TkPortRuleInfo *ruleInfo);

int32   TkExtOamPortRuleAdd(uint8 pathId,uint8 linkId, LogicalPortIndex port, 
                TkPortRuleInfo *ruleInfo);

#if defined(__cplusplus)
}
#endif

#endif /* _SOC_EA_TkRuleApi_H */
