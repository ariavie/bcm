/*
 * $Id: OamUtils.h,v 1.4 Broadcom SDK $
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
 * File:     TkOamUtils.h
 * Purpose: 
 *
 */

#ifndef _SOC_EA_TkOamUtils_H
#define _SOC_EA_TkOamUtils_H

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkUtils.h>
#include <soc/ea/tk371x/Oam.h>

#define OnuOamPass           0x01

#define OnuHostIfPhyIfMask   0xF0
#define OnuHostIfPhyIfSft    4
#define OnuHostIfLinkMask    0x0F

#define OamReservedFlagShift 8
#define OamReservedFlagMask  0xFF00
#define OamFlagMask          0x00FF

#define OamFlagLinkMask      0xF0
#define OamFlagLinkShift     4

#define OamFlagSrcIfMask     0x0C
#define OamFlagSrcIfShift    2


/* Teknovus OUI */
extern const IeeeOui    TeknovusOui;

/* CTC OUI */
extern const IeeeOui    CTCOui;


/*
 * OAM flag handling
 */
uint8   GetSourceForFlag (uint16 flags);
void    AttachOamFlag (uint8 source, uint16 * flags);
void    AttachOamFlagNoPass (uint8 source, uint16 * flags);

uint8 * OamFillExtHeader (uint16 flags, const IeeeOui * oui, uint8 * TxFrame);

int32   OamEthSend (uint8 pathId, MacAddr * dstAddr, uint8 * pDataBuf, 
                    uint32 dataLen);

/* send TK extension OAM message(Get/Set) and wait for response from the ONU */
int     TkOamRequest (uint8 pathId, uint8 linkId, OamFrame * txBuf, 
                      uint32 txLen, OamPdu * rxBuf, uint32 * pRxLen);

int     TkOamNoResRequest (uint8 pathId, uint8 linkId, OamFrame * txBuf,
                           uint32 txLen);

/* send TK extension OAM Get message with object instance and Get response 
 * from the ONU 
 */
int     TkExtOamObjGet (uint8 pathId, uint8 linkId, uint8 object,
                        OamObjIndex * index, uint8 branch, uint16 leaf,
                        uint8 * pRxBuf, uint32 * pRxLen);

/* send TK extension OAM Get message with object instance and Get response 
 * from the ONU 
 */
int     TkExtOamObjActGet (uint8 pathId, uint8 linkId, uint8 object,
                       OamObjIndex * index, uint8 branch, uint16 leaf,
                       uint8 paramLen, uint8 * params, uint8 * pRxBuf, 
                       uint32 * pRxLen);

/* send TK extension OAM Get message with object instance and Get response
 * (multi TLVs) from the ONU 
 */
int     TkExtOamObjGetMulti (uint8 pathId, uint8 linkId, uint8 object,
                OamObjIndex * index, uint8 branch, uint16 leaf,
                uint8 * pRxBuf, uint32 * pRxLen);

/* send TK extension OAM Get message without object instance and Get response
 * from the ONU 
 */
int     TkExtOamGet (uint8 pathId, uint8 linkId, uint8 branch, uint16 leaf,
                uint8 * pRxBuf, uint32 * pRxLen);

/* send TK extension OAM Get message without object instance and Get response
 * (multi TLVs) from the ONU 
 */
int     TkExtOamGetMulti (uint8 pathId, uint8 linkId, uint8 branch, uint16 leaf,
                uint8 * pRxBuf, uint32 * pRxLen);

/* send TK extension OAM Set message with object instance and Get return code */
uint8   TkExtOamObjSet (uint8 pathId, uint8 linkId, uint8 object,
                OamObjIndex * index, uint8 branch, uint16 leaf,
                uint8 * pTxBuf, uint8 txLen);

/* send TK extension OAM Set message without object instance and Get return code
 * from the ONU 
 */
uint8   TkExtOamSet (uint8 pathId, uint8 linkId, uint8 branch, uint16 leaf,
                uint8 * pTxBuf, uint8 txLen);

/* send OAM response message to OLT */
int32   TkExtOamResponse (uint8 pathId, uint8 linkId, OamFrame * txBuf,
                uint32 txLen);

uint8   TxOamDeliver (uint8 pathId, uint8 linkId, BufInfo * bufInfo,
                uint8 * pRxBuf, uint32 * pRxLen);

uint8   OamContSize (OamVarContainer const *cont);
OamVarContainer * NextCont (OamVarContainer const *cont);
uint8   TkOamMsgPrepare (uint8 pathId, BufInfo * bufInfo, uint8 tkOpCode);

void    TkExtOamSetReplyTimeout (uint8 pathId, uint32 val);
uint32  TkExtOamGetReplyTimeout (uint8 pathId);

#if defined(__cplusplus)
}
#endif

#endif /* _SOC_EA_TkOamUtils_H */
