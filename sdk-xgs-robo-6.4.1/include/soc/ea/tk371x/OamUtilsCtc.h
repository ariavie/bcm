/*
 * $Id: OamUtilsCtc.h,v 1.3 Broadcom SDK $
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
 * File:     OamUtilsCtc.h
 * Purpose: 
 *
 */


 
#ifndef _SOC_EA_OamUtilsCtc_H
#define _SOC_EA_OamUtilsCtc_H

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkUtils.h>

#define Ctc21ObjPortInstPack(type,frame_id,slot_id,port_id) \
    (type)<<24|(frame_id)<<22|(slot_id)<<16|(port_id)

typedef struct {
    uint8 uniPortType;
    uint8 frameNum;
    uint8 slowNum;
    uint16 uniPortNum;
} CtcOamObjPortInstant;

typedef struct{
    uint16 type;
    CtcOamObjPortInstant inst;
} CtcOamObj21;

uint8   CtcOamMsgPrepare (uint8 pathId, BufInfo * bufInfo, uint8 tkOpCode);

/* send TK extension OAM Get message with object instance and Get response from the ONU */
int     CtcExtOamObjGet (uint8 pathId, uint8 linkId, uint8 objBranch, uint16 objLeaf,
                uint32 objIndex, uint8 branch, uint16 leaf,
                uint8 * pRxBuf, uint32 * pRxLen);

/* send TK extension OAM Get message with object instance and Get response from the ONU */
int     CtcExtOamObjActGet (uint8 pathId, uint8 linkId, uint8 objBranch,
                uint16 objLeaf, uint32 objIndex, uint8 branch,
                uint16 leaf, uint8 paramLen, uint8 * params,
                uint8 * pRxBuf, uint32 * pRxLen);

/* send TK extension OAM Get message with object instance and Get response(multi TLVs) 
   from the ONU */
int     CtcExtOamObjGetMulti (uint8 pathId, uint8 linkId, uint8 objBranch,
                uint16 objLeaf, uint32 objIndex, uint8 branch,
                uint16 leaf, uint8 * pRxBuf, uint32 * pRxLen);

/* send TK extension OAM Get message without object instance and Get response
   from the ONU */
int     CtcExtOamGet (uint8 pathId, uint8 linkId, uint8 branch, uint16 leaf,
                uint8 * pRxBuf, uint32 * pRxLen);

/* send TK extension OAM Get message without object instance and Get response(multi TLVs)
   from the ONU */
int     CtcExtOamGetMulti (uint8 pathId, uint8 linkId, uint8 branch, uint16 leaf,
                uint8 * pRxBuf, uint32 * pRxLen);

/* send TK extension OAM Set message with object instance and Get return code */
uint8   CtcExtOamObjSet (uint8 pathId, uint8 linkId, uint8 objBranch,
                uint16 objLeaf, uint32 objIndex, uint8 branch,
                uint16 leaf, uint8 * pTxBuf, uint8 txLen);

/* send TK extension OAM Set message without object instance and Get return code
   from the ONU */
uint8   CtcExtOamSet (uint8 pathId, uint8 linkId, uint8 branch, uint16 leaf,
                uint8 * pTxBuf, uint8 txLen);

int32   CtcExtOam21ObjInstPack(uint8 type, uint16 index, uint32 *pObjInst);

#if defined(__cplusplus)
}
#endif

#endif /* _SOC_EA_OamUtilsCtc_H */
