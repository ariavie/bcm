/*
 * $Id: TkSdkInitApi.c 1.2 Broadcom SDK $
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
 * File:     TkSdkInitApi.c
 * Purpose: 
 *
 */

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkOamMem.h>
#include <soc/ea/tk371x/Oam.h>
#include <soc/ea/tk371x/OamUtils.h>
#include <soc/ea/tk371x/OamProcess.h>
#include <soc/ea/tk371x/TkInit.h>
#include <soc/ea/tk371x/TkDebug.h>
#include <soc/ea/tk371x/AlarmProcess.h>

/*
 * Teknovus OUI 
 */
const IeeeOui   TeknovusOui = { {0x00, 0x0D, 0xB6} };

/*
 * CTC OUI 
 */
const IeeeOui   CTCOui = { {0x11, 0x11, 0x11} };

MacAddr         OamMcMacAddr = { {0x01, 0x80, 0xc2, 0x00, 0x00, 0x02} };

                                             

/*
 *  Function:
 *      TkGetVlanLink
 * Purpose:
 *      Get the information about the OAM processing per EPON MAC
 * Parameters:
 *      pathId    - ONU index
 * Returns:
 *      ERROR code or OK 
 * Notes:
 */
VlanLinkConfig *
TkGetVlanLink(uint8 pathId)
{
    return &(gTagLinkCfg[pathId % 8]);
}

/*
 *  Function:
 *      TkGetApiBuf
 * Purpose:
 *      Get the api buffer for storing API response message
 * Parameters:
 *      pathId    - ONU index
 * Returns:
 *      ERROR code or OK 
 * Notes:
 */
void *
TkGetApiBuf(uint8 pathId)
{
    return gTagLinkCfg[pathId % 8].apiRxBuff;
}

/*
 *  Function:
 *      TkExtOamTaskExit
 * Purpose:
 *      Free the resource alloced for the unit.
 * Parameters:
 *      tagCount    - If use vlan to identify the physical path. this should set to no 0 number.
 *                         0 means not use VLAN to identify the physical path. no 0 value identify
 *                         how many physical EPON chipset the system support.
 *      tags    - Vlan tag list, and must fixed as 0x81000xxx or 0x88080xxx
 * Returns:
 *      ERROR code or OK 
 * Notes:
 */
void
TkExtOamTaskExit(uint8 pathId)
{
    if(pathId >= MAX_NUM_OF_PON_CHIP){
        return;
    }
    
    if (SAL_THREAD_ERROR != gTagLinkCfg[pathId].almTaskId)
        sal_thread_destroy(gTagLinkCfg[pathId].almTaskId);    
    if (SEM_VALID(gTagLinkCfg[pathId].semId))
        sal_sem_destroy(gTagLinkCfg[pathId].semId);
    if (gTagLinkCfg[pathId].almMsgQid)
        sal_msg_destory(gTagLinkCfg[pathId].almMsgQid);
    if (gTagLinkCfg[pathId].resMsgQid)
        sal_msg_destory(gTagLinkCfg[pathId].resMsgQid);
    
    TkOamMemsFree(pathId);
    return;
}


/*
 *  Function:
 *      TkExtOamTaskInit
 * Purpose:
 *      Requrest resource for the system
 * Parameters:
 *      pathId    - The EPON chipset unit
 * Returns:
 *      ERROR code or OK 
 * Notes:
 */
int32
TkExtOamTaskInit(uint8 pathId)
{
    uint32          unit = pathId;
    int             ret;

    if(pathId >= MAX_NUM_OF_PON_CHIP){
        return ERROR;
    }

    ret = TkOamMemInit(pathId, 24, 1600);

    if(OK != ret){
        return ret;
    }
    
    sal_sprintf(gTagLinkCfg[pathId].oltSemName, "oltSem%d",pathId);
    sal_sprintf(gTagLinkCfg[pathId].resMsgName, "resMsg%d",pathId);
    sal_sprintf(gTagLinkCfg[pathId].almMsgName, "almMsg%d",pathId);
    sal_sprintf(gTagLinkCfg[pathId].almTaskName, "almTask%d",pathId);

    TkDbgInfoTrace(TkDbgLogTraceEnable,("%s\n",gTagLinkCfg[pathId].oltSemName));
    TkDbgInfoTrace(TkDbgLogTraceEnable,("%s\n",gTagLinkCfg[pathId].resMsgName));
    TkDbgInfoTrace(TkDbgLogTraceEnable,("%s\n",gTagLinkCfg[pathId].almMsgName));
    TkDbgInfoTrace(TkDbgLogTraceEnable,("%s\n",gTagLinkCfg[pathId].almTaskName));
    
    gTagLinkCfg[pathId].vlan = 0x81000001;
    gTagLinkCfg[pathId].resMsgQid = 
        sal_msg_create(gTagLinkCfg[pathId].resMsgName, 5);
    gTagLinkCfg[pathId].semId = 
        sal_sem_create(gTagLinkCfg[pathId].oltSemName, sal_sem_BINARY, 1);
    gTagLinkCfg[pathId].apiRxBuff = 
        (uint8 *)TkOamMemGet(pathId);
    gTagLinkCfg[pathId].timeOut = TK_OAM_REPLY_TIME_OUT_MS;
    gTagLinkCfg[pathId].almMsgQid = 
        sal_msg_create(gTagLinkCfg[pathId].almMsgName, 20);
    gTagLinkCfg[pathId].almTaskId =
        sal_thread_create(gTagLinkCfg[pathId].almTaskName, 
            2000, 7, TkAlarmRxThread, (void *)INT_TO_PTR(unit));
    
    TkAlmHandlerRegister(pathId, AlarmProcessHandler);

    return OK;
}

