/*
 * $Id: TkMsgProcess.c 1.4 Broadcom SDK $
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
 * File:     TkMsgProcess.c
 * Purpose: 
 *
 */
 
#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkOsUtil.h>
#include <soc/ea/tk371x/Ethernet.h>
#include <soc/ea/tk371x/Oam.h>
#include <soc/ea/tk371x/OamUtils.h>
#include <soc/ea/tk371x/OamProcess.h>
#include <soc/ea/tk371x/TkMsg.h>
#include <soc/ea/tk371x/TkOamMem.h>
#include <soc/ea/tk371x/TkInit.h>  
#include <soc/ea/tk371x/TkDebug.h>


extern const MacAddr OamMcMacAddr;


static uint8(*ieeeAlmProcessFn) (uint8 , uint8 *, short) = NULL;

Bool
TkDataProcessHandle(uint8 pathId, TkMsgBuff * pMsg, uint16 len)
{
    OamFrame       *pOamFrame = (OamFrame *) pMsg->buff;
    OamMsg         *pOamMsg = &(pOamFrame->Data.OamHead);

    if (EthertypeOam == soc_ntohs(pOamFrame->EthHeader.type)
        && (0x03 == (pOamFrame->Data.OamHead.subtype))) {
            if ((uint8) OamOpEventNotification == pOamMsg->opcode)  
            {
                pMsg->mtype = 0;
                if (TkDbgLevelIsSet(TkDbgMsgEnable)) {
                    TkDbgPrintf(("\r\nSend a message to Alm task\n"));
                    TkDbgDataDump((uint8 *) pMsg, len, 16);
                }
                sal_msg_snd(gTagLinkCfg[pathId].almMsgQid,(char *) pMsg, len, WAIT_FOREVER);
            } else {
                pMsg->mtype = pathId;
                if (TkDbgLevelIsSet(TkDbgMsgEnable)) {
                    TkDbgPrintf(("\r\nSend a message to response queue. pathId:%d\n", pathId));
                    TkDbgDataDump((uint8 *) pMsg, len, 16);
                }
                sal_msg_snd(gTagLinkCfg
                            [pathId].resMsgQid,
                            (char *) pMsg, len, WAIT_FOREVER);
            }
    } else {
        if (TkDbgLevelIsSet(TkDbgMsgEnable)) {
            TkDbgPrintf(("\r\nIs not oam package\r\n"));
            TkDbgDataDump((uint8 *) pMsg, len, 16);
        }
    }

    return TRUE;
}

void
_TkDataProcessHandle(uint8 pathId, char *data, uint16 len)
{
    TkMsgBuff      *pMsgBuf = (TkMsgBuff *) TkOamMemGet(pathId);

    if (NULL == pMsgBuf) {
        TkDbgTrace(TkDbgErrorEnable);
        return;
    }

    sal_memcpy(pMsgBuf->buff, data, len);
    TkDataProcessHandle(pathId, pMsgBuf, len);
    TkOamMemPut(pathId,(void *) pMsgBuf);
    return;
}


void
TkAlarmRxThread(void *cookie)
{
    int16           MsgLen = 0;
    TkMsgBuff      *pMsgBuf ;
    uint32          pathId = PTR_TO_INT(cookie);

    pMsgBuf = (TkMsgBuff *) TkOamMemGet(pathId);

    while (1) {
        if ((MsgLen =
             sal_msg_rcv(gTagLinkCfg[pathId].almMsgQid,
                         (char *) pMsgBuf,
                         TK_MAX_RX_TX_DATA_LENGTH, WAIT_FOREVER)) < 0) {
            sal_usleep(1);
            continue;
        } else {
            if (TkDbgLevelIsSet(TkDbgMsgEnable | TkDbgAlmEnable)) {
                TkDbgPrintf(("\r\nAlmTask received a msg\n"));
                TkDbgDataDump((uint8 *) pMsgBuf, MsgLen, 16);
            }
            if (ieeeAlmProcessFn)
                ieeeAlmProcessFn(pathId, pMsgBuf->buff, MsgLen);
        }
        sal_usleep(1);
    }
    TkOamMemPut(pathId,(void *) pMsgBuf);
}

void
TkAlmHandlerRegister(uint8 pathId, uint8(*funp)
                         (uint8 , uint8 *, short))
{
    if(pathId);
    
    if (NULL == funp) {
        return;
    }

    ieeeAlmProcessFn = funp;

    return;
}
