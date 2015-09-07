/*
 * $Id: CtcMiscApi.h 1.1 Broadcom SDK $
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
 * File:     CtcMiscApi.h
 * Purpose: 
 *
 */
 
#ifndef _SOC_EA_CtcMiscApi_H
#define _SOC_EA_CtcMiscApi_H

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/CtcOam.h>


#define RE_ENABLE_LASER_TX_POWER    0
#define POWER_DOWN_LASER_TX_POWER   65535

typedef struct {
    int8    CtcInfoFromONUPONChipsSetInit;
    uint16  CtcONUSNLen;
    uint8   CtcONUSN[64];
    uint16  CtcFirmwareVerLen;
    uint8   CtcFirmwareVer[64];
    uint16  CtcChipsetIdLen;
    uint8   CtcChipsetId[64];
    uint16  CtcOnuCap1Len;
    uint8   CtcOnuCap1[64];
    uint16  CtcOnuCap2Len;
    uint8   CtcOnuCap2[64];
} CtcInfoFromONUPONChipsSet;

typedef struct {
    uint8   Select;
    uint8   MatchVal[6];
    uint8   ValidOperator;
} CtcExtRuleCondtion;

typedef struct {
    uint8   Priority;
    uint8   QueueMapped;
    uint8   EthernetPri;
    uint8   NumOfEntry;
    CtcExtRuleCondtion cond[16];
} CtcExtRule;

typedef struct {
    uint16  Qid;
    uint16  QWrr;
} CtcExtQConfig;

typedef struct {
    uint32  Action;
    uint8   ONUID[6];
    uint32  OpticalTransmitterID;
} CtcExtONUTxPowerSupplyControl;

#define  HOLDOVERDISACTIVATED  0x01     /* default, holdover disactivated */
#define  HOLDOVERACTIVATED	   0x02     /* holdover activated */

typedef struct {
    uint32 state; /* the holdover flag */
    uint32 time;  /* the holdover time */
}PACK TkCtcHoldover;

typedef struct {
    uint32 raiseThreshold;
    uint32 clearThreshold;
} PACK CtcOamAlarmThreshold;

#define CTCALMSTATEDEACTIVED 0X01
#define CTCALMSTATEACTIVED   0x02

typedef struct{
    uint16 alarmId;
    uint32 alarmState;
}PACK CtcOamAlarmState;

void    TKCTCClearONUPONChipsSetInfo (void);

void    TKCTCExtOamFillMacForSN (uint8 * mac);

int32   TKCTCExtOamGetInfoFromONUPONChipsets (uint8 pathId, uint8 linkId,
                CtcInfoFromONUPONChipsSet * pCont);

int32   TKCTCExtOamGetDbaCfg (uint8 pathId, uint8 linkId, OamCtcDbaData * dba);

int32   TKCTCExtOamSetDbaCfg(uint8 pathId,uint8 linkId,OamCtcDbaData * dba); 

int32   TKCTCExtOamGetFecAbility (uint8 pathId, uint8 LinkId,
                uint32 * fecAbility);

int32   TKCTCExtOamNoneObjGetRaw (uint8 pathId, uint8 LinkId, uint8 branch,
                uint16 leaf, uint8 * pBuff, int32 * retLen);

int32   TKCTCExtOamNoneObjSetRaw (uint8 pathId, uint8 LinkId, uint8 branch,
                uint16 leaf, uint8 * pBuff, uint8 retLen, uint8 * reCode);

int32   TKCTCExtOamObjGetRaw (uint8 pathId, uint8 LinkId, uint8 objBranch,
                uint16 objLeaf, uint32 objIndex, uint8 branch,
                uint16 leaf, uint8 * pBuff, int32 * retLen);

int32   TKCTCExtOamObjSetRaw (uint8 pathId, uint8 LinkId, uint8 objBranch,
                uint16 objLeaf, uint32 objIndex, uint8 branch,
                uint16 leaf, uint8 * pBuff, uint8 retLen, uint8 * reCode);

int32   TKCTCExtOamGetONUSN (uint8 pathId, uint8 LinkId,
                CtcOamOnuSN * pCtcOamOnuSN);

int32   TKCTCExtOamGetFirmwareVersion (uint8 pathId, uint8 LinkId,
                CtcOamFirmwareVersion * pCtcOamFirmwareVersion, int32 * retLen);

int32   TKCTCExtOamGetChipsetId (uint8 pathId, uint8 LinkId,
                CtcOamChipsetId * pCtcOamChipsetId);

int32   TKCTCExtOamGetONUCap (uint8 pathId, uint8 LinkId, uint8 * pCap,
                int32 * retLen);

int32   TKCTCExtOamGetONUCap2 (uint8 pathId, uint8 LinkId, uint8 * pCap,
                int32 * retLen);

int32   TKCTCExtOamGetHoldOverConfig (uint8 pathId, uint8 LinkId, uint8 * pCap,
                int32 * retLen);

int32   TKCTCExtOamSetClsMarking (uint8 pathId, uint8 linkId, uint32 portNo,
                uint8 action, uint8 ruleCnt, CtcExtRule * pCtcExtRule);

int32   TKCTCExtOamSetLLIDQueueConfig (uint8 pathId, uint8 linkId, uint32 LLID,
                uint8 numOfQ, CtcExtQConfig * pCtcExtQConfig);

int32   TKCTCExtOamSetLaserTxPowerAdminCtl (uint8 pathId, uint8 linkId,
                CtcExtONUTxPowerSupplyControl * pCtcExtONUTxPowerSupplyControl);

int32   CtcExtOamSetMulLlidCtrl (uint8 pathId, uint32 llidNum);

int32   CtcExtOamSetHoldover (uint8 pathId, uint8 linkId, uint32 state, 
                uint32 time);

int32   CtcExtOamGetHoldover(uint8 pathId, uint8 linkId, uint32 *state, 
                uint32 *time);
	
int32   CtcExtOamGetAlarmState(uint8 pathId, uint8 linkId, uint16 port, 
                uint16 alarmid, uint8 *state);

int32   CtcExtOamSetAlarmState(uint8 pathId,uint8 linkId,  uint16 port,  
                uint16 alarmid, uint8 state);

int32   CtcExtOamGetAlarmThreshold(uint8 pathId,uint8 linkId, uint16 port, 
                uint16 alarmid, CtcOamAlarmThreshold *threshold);
	
int32   CtcExtOamSetAlarmThreshold(uint8 pathId,uint8 linkId, uint16 port,  
                uint16 alarmid, CtcOamAlarmThreshold threshold);	

int32   CtcExtOamGetOptTransDiag(uint8 path_id, uint8 link_id, 
                CtcOamTlvPowerMonDiag *info);

int32   CtcExtOamSetPonIfAdmin(uint8 path_id, uint8 link_id, uint8 optical_no);

int32   CtcExtOamGetPonIfAdmin(uint8 path_id, uint8 link_id, uint8 *optical_no);

#if defined(__cplusplus)
}
#endif

#endif /* _SOC_EA_CtcMiscApi_H */
