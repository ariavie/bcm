/*
 * $Id: TkTmApi.c 1.1 Broadcom SDK $
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
 * File:     TkTmApi.c
 * Purpose: 
 *
 */

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/Oam.h>
#include <soc/ea/tk371x/OamUtils.h>
#include <soc/ea/tk371x/TkOsUtil.h>
#include <soc/ea/tk371x/TkTmApi.h>
#include <soc/ea/tk371x/TkDebug.h>
#include <soc/ea/tk371x/TkDefs.h>
#include <soc/ea/tk371x/TkOamMem.h>

/*
 * send TK extension OAM message Get queue configuration from the ONU 
 */
int
TkExtOamGetQueueCfg(uint8 pathId, uint8 LinkId, uint8 * pQueueCfg)
{
    uint32          DataLen;

    if ((LinkId > 7) || (NULL == pQueueCfg)) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    if (OK ==
        TkExtOamGet(pathId, LinkId,
                    OamBranchAction,
                    OamExtActGetQueueConfig, (uint8 *) pQueueCfg,
                    &DataLen)) {
        return (OK);
    } else {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }
}


/*
 * send TK extension OAM message Set enable user traffic to the ONU 
 */
int
TkExtOamSetEnaUserTraffic(uint8 pathId, uint8 LinkId)
{
    int rv;
    uint8 ret;
    
    if (LinkId > 7){
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    ret = TkExtOamSet(pathId, LinkId, OamBranchAction, 
        OamExtActOnuEnableUserTraffic, NULL, 0);

    if(OamVarErrNoError != ret){
        rv = ERROR;
    }else{
        rv = OK;
    }

    return rv;
}


/*
 * send TK extension OAM message Set disable user traffic to the ONU 
 */
int
TkExtOamSetDisUserTraffic(uint8 pathId, uint8 LinkId)
{
    int rv;
    uint8 ret;
    
    if (LinkId > 7){
        return ERROR;
    }

    ret = TkExtOamSet(pathId, LinkId, OamBranchAction, 
        OamExtActOnuDisableUserTraffic, NULL, 0);

    if(OamVarErrNoError != ret){
        rv = ERROR;
    }else{
        rv = OK;
    }

    return rv;
}

/*
 * send TK extension OAM message Get BcastRateLimit ON/OFF 
 */
int
TkExtOamGetBcastRateLimit(uint8 pathId, uint8 LinkId, uint8 port, uint32 *pkts_cnt)
{
    uint32 dataLen;
    int rv;
    uint32 *pBuff = NULL;
    OamObjIndex index;

    index.portId = port;
    
    if((LinkId >= SDK_MAX_NUM_OF_LINK)||(port > SDK_MAX_NUM_OF_PORT)){
        return (ERROR);
    }
    
    pBuff = (uint32 *) TkOamMemGet(pathId);
    if(NULL == pBuff){
        return ERROR;
    }

    if(OK == TkExtOamObjGet(pathId, LinkId, OamNamePhyName,&index,
        OamBranchAttribute, 
        OamExtAttrBcastRateLimit, 
        (uint8 *)pBuff, &dataLen)){
        *pkts_cnt = soc_ntohl(pBuff[0]);
        
        rv = OK;     
    }else{
        
        rv = ERROR;
    }
    
    TkOamMemPut(pathId, (void *)pBuff);
    
    return rv;
}

/*
 * send TK extension OAM message Set BcastRateLimit ON/OFF
 */
int
TkExtOamSetBcastRateLimit(uint8 pathId, uint8 LinkId, uint8 port, uint32 pkts_cnt)
{
    OamObjIndex index;
    uint32 count;

    if((LinkId > SDK_MAX_NUM_OF_LINK)||(port > SDK_MAX_NUM_OF_PORT)){
        return (ERROR);
    }

    index.portId = port;
    count = soc_htonl(pkts_cnt);

    if(OamVarErrNoError == TkExtOamObjSet(pathId, LinkId,
        OamNamePhyName, &index, OamBranchAttribute, 
        OamExtAttrBcastRateLimit, (uint8 *)&count, 
        sizeof(uint32))){
        return (OK);    
    }
    
    TkDbgTrace(TkDbgErrorEnable);
    return ERROR;
}

