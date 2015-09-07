/*
 * $Id: TkOamFileTrans.c,v 1.1 Broadcom SDK $
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
 * File:     TkOamFileTrans.c
 * Purpose: 
 *
 */

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkOsUtil.h>
#include <soc/ea/tk371x/TkOamMem.h>
#include <soc/ea/tk371x/Ethernet.h>
#include <soc/ea/tk371x/Oam.h>
#include <soc/ea/tk371x/FileTransTk.h>
#include <soc/ea/tk371x/OamUtils.h>
#include <soc/ea/tk371x/TkDebug.h>
#include <soc/ea/tk371x/TkOnuApi.h>        

extern const MacAddr OamMcMacAddr;
extern OamFileSession tkServerSession;

/*
 * send TK extension OAM file read or write request to the ONU and waiting 
 * for 
 * response
 * ReqType : 0 - read, 1 - write
 * 
 */
int
TkExtOamFileTranReq(uint8 pathId, uint8 ReqType,
                    uint8 loadType, OamTkFileAck * pTkFileAck)
{
    OamTkFileRequest *pTkFileReq = NULL;
    uint32          size,
                    RespLen;
    uint16          flags = 0x0050;
    uint8          *pMsgBuf = NULL;

    if ((loadType > (uint8) OamTkFileNumTypes)
        || (ReqType > FILE_WRITE_REQ)
        || (NULL == pTkFileAck)) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    pMsgBuf = (uint8 *) TkOamMemGet(pathId);  /* get memory */
    if (NULL == pMsgBuf) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    pTkFileReq = (OamTkFileRequest *)
        OamFillExtHeader(flags, &TeknovusOui, pMsgBuf);
    pTkFileReq->ext.opcode = ReqType + OamExtOpFileRdReq;   /* 09/0A */
    pTkFileReq->file = loadType;

    size = sizeof(OamOuiVendorExt) + sizeof(OamTkFileRequest);
    /*
     * The first reply need about > 3s. The last reply need about > 10s
     * for App. The middle reply need about > 0.02s. 
     */
    if (OK ==
        TkOamRequest(pathId, 0,
                     (OamFrame *) pMsgBuf, size,
                     (OamPdu *) pMsgBuf, &RespLen)) {
        bcopy((void *) INT_TO_PTR(PTR_TO_INT(pMsgBuf) +
                        sizeof(OamOuiVendorExt)),
              (void *) pTkFileAck, sizeof(OamTkFileAck));
        TkOamMemPut(pathId,(void *) pMsgBuf);  /* free memory */
        return (OK);
    } else {
        TkOamMemPut(pathId,(void *) pMsgBuf);  /* free memory */
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }
}

/*
 * 
 * TkExtOamFileSendData: fills in and sends data message and wait for ack
 * 
 * Parameters:
 * \param blockNum Block number of the current segment
 * \param dataLen Length of the data to send
 * \param data Pointer to the data to send
 * 
 * \return 
 * OK or ERROR
 */
int
TkExtOamFileSendData(uint8 pathId, uint16 blockNum,
                     uint16 dataLen, uint8 * data,
                     OamTkFileAck * pTkFileAck)
{
    OamTkFileData  *pTkFileData = NULL;
    uint32          size,
                    RespLen;
    uint16          flags = 0x0050;
    uint8          *pMsgBuf = NULL;

    if ((NULL == data) || (NULL == pTkFileAck)) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    pMsgBuf = (uint8 *) TkOamMemGet(pathId);
    if (NULL == pMsgBuf) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    pTkFileData =
        (OamTkFileData *) OamFillExtHeader(flags, &TeknovusOui, pMsgBuf);
    pTkFileData->ext.opcode = OamExtOpFileData; /* 0B */
    pTkFileData->blockNum = soc_htons(blockNum);
    pTkFileData->size = soc_htons(dataLen);

    size = sizeof(OamOuiVendorExt) + sizeof(OamTkFileData) + dataLen;

    bcopy ((void *)data, (void *)(pTkFileData + 1), dataLen);
    
   /*
     * The first reply need about > 3s. The last reply need about > 10s
     * for App. The middle reply need about > 0.02s. 
     */
    if (OK ==
        TkOamRequest(pathId, 0,
                     (OamFrame *) pMsgBuf, size,
                     (OamPdu *) pMsgBuf, &RespLen)) {
        bcopy((void *) INT_TO_PTR(PTR_TO_INT( pMsgBuf) +
                        sizeof(OamOuiVendorExt)),
              (void *) pTkFileAck, sizeof(OamTkFileAck));
        TkOamMemPut(pathId,(void *) pMsgBuf);
        return (OK);
    } else {
        TkOamMemPut(pathId,(void *) pMsgBuf);
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }
}

/*
 * TkExtOamFileSendAck: fills in and sends ack message to ONU
 * 
 * Parameters:
 * \param err Error to use in ack message
 * 
 * \return 
 * None
 */
int
TkExtOamFileSendAck(uint8 pathId, uint8 err)
{
    OamTkFileAck   *pTkFileAck = NULL;
    uint32          size,
                    RespLen;
    uint16          flags = 0x0050;
    uint8          *pMsgBuf = NULL;
    uint32          origOut = TkExtOamGetReplyTimeout(pathId);

    pMsgBuf = (uint8 *) TkOamMemGet(pathId);
    if (NULL == pMsgBuf) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    pTkFileAck =
        (OamTkFileAck *) OamFillExtHeader(flags, &TeknovusOui, pMsgBuf);
    pTkFileAck->ext.opcode = (uint8) OamExtOpFileAck;   /* 0C */
    pTkFileAck->blockNum = 0;
    pTkFileAck->err = err;

    size = sizeof(OamOuiVendorExt) + sizeof(OamTkFileAck);
    /*
     * The first reply need about > 3s. The last reply need about > 10s
     * for App. The middle reply need about > 0.02s. 
     */
    TkExtOamSetReplyTimeout(pathId, origOut*10);
    if (OK ==
        TkOamRequest(pathId, 0,
                     (OamFrame *) pMsgBuf, size,
                     (OamPdu *) pMsgBuf, &RespLen)) {
        OamTkFileAck   *pTkRcvAck =
            (OamTkFileAck *) INT_TO_PTR(PTR_TO_INT(pMsgBuf) + sizeof(OamOuiVendorExt));
        TkExtOamSetReplyTimeout(pathId,origOut);
        if (((uint8) OamExtOpFileAck == pTkRcvAck->ext.opcode)
            && (0 == pTkRcvAck->blockNum)
            && ((uint8) OamTkFileErrOk == pTkRcvAck->err)) {
            TkOamMemPut(pathId,(void *) pMsgBuf);  /* free memory */
            return (OK);
        } else {
            TkOamMemPut(pathId,(void *) pMsgBuf);
            TkDbgTrace(TkDbgErrorEnable);
            return ERROR;
        }
    } else {
        TkExtOamSetReplyTimeout(pathId,origOut);
        TkOamMemPut(pathId,(void *) pMsgBuf);
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }
}

void
OamFileDone(uint8 pathId)
{
    sal_memset(&tkServerSession, 0, sizeof(OamFileSession));
}

