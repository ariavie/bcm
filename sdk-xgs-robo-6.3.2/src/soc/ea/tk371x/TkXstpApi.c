/*
 * $Id: TkXstpApi.c 1.1 Broadcom SDK $
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
 * File:     TkXstpApi.c
 * Purpose: 
 *
 */

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/Oam.h>
#include <soc/ea/tk371x/OamUtils.h>
#include <soc/ea/tk371x/TkUtils.h>
#include <soc/ea/tk371x/TkInit.h>
#include <soc/ea/tk371x/TkDebug.h>
#include <soc/ea/tk371x/TkXstpApi.h>

uint8
TkExtOamSetRstpBridge(uint8 pathId, uint8 linkId, TagRstpBridgeCfg * cfg)
{
    BufInfo         bufInfo;
    Bool            ok;
    uint8           buf[4];
    uint32          rxLen;
    uint8           ret;
    tGenOamVar      var;
    Bool            more;
    uint8           *rxTmpBuf = (uint8*)TkGetApiBuf(pathId);
    
    if (linkId != 0) {
        return RcBadParam;
    }

    if (TkOamMsgPrepare(pathId,&bufInfo, OamExtOpVarSet)
        != OK) {
        return RcNoResource;
    }

    Tk2BufU16(buf, 0);
    ok = AddOamTlv(&bufInfo, OamBranchNameBinding, OamExtNameBridge, 2,
                   buf);
    Tk2BufU16(buf, (uint16) cfg->priority);
    ok = ok &&
        AddOamTlv(&bufInfo, OamBranchAttribute,
                  OamAttrRstpBridgePriority, 2, buf);
    Tk2BufU8(buf, (uint8) cfg->bridgeMode);
    ok = ok &&
        AddOamTlv(&bufInfo, OamBranchAttribute,
                  OamExtAttrRstpBridgeMode, 1, buf);
    Tk2BufU16(buf, (uint16) cfg->maxAge);
    ok = ok &&
        AddOamTlv(&bufInfo, OamBranchAttribute,
                  OamAttrRstpBridgeMaxAge, 2, buf);
    Tk2BufU16(buf, (uint16) cfg->fwdDelay);
    ok = ok
        && AddOamTlv(&bufInfo,
                     OamBranchAttribute,
                     OamAttrRstpBridgeForwardDelay, 2, buf);
    Tk2BufU16(buf, (uint16) cfg->holdTime);
    ok = ok &&
        AddOamTlv(&bufInfo, OamBranchAttribute, OamAttrRstpHoldTime, 2,
                  buf);

    ret = TxOamDeliver(pathId, linkId, &bufInfo, (uint8 *) rxTmpBuf, &rxLen);
    if (ret != RcOk)
        return ret;

    if (TkDbgLevelIsSet(TkDbgLogTraceEnable))
        BufDump(NULL, rxTmpBuf, rxLen);

    InitBufInfo(&bufInfo, rxLen, rxTmpBuf);

    /* The first branch is - OamBranchNameBinding*/
    more = GetNextOamVar(&bufInfo, &var, &ret);

    ok = more && (ret == RcOk)
        && (var.Branch == OamBranchNameBinding)
        && (var.Leaf == OamExtNameBridge);
    if (!ok)
        return RcBadOnuResponse;

    /* Followed by OamAttrRstpBridgePriority*/
    more = GetNextOamVar(&bufInfo, &var, &ret);
    ok = more && (ret == RcOk)
        && (var.Leaf == OamAttrRstpBridgePriority)
        && (var.Width == 0);
    if (!ok)
        return RcBadOnuResponse;

    more = GetNextOamVar(&bufInfo, &var, &ret);
    ok = more && (ret == RcOk)
        && (var.Leaf == OamExtAttrRstpBridgeMode)
        && (var.Width == 0);
    if (!ok)
        return RcBadOnuResponse;

    more = GetNextOamVar(&bufInfo, &var, &ret);
    ok = more && (ret == RcOk)
        && (var.Leaf == OamAttrRstpBridgeMaxAge)
        && (var.Width == 0);
    if (!ok)
        return RcBadOnuResponse;

    more = GetNextOamVar(&bufInfo, &var, &ret);
    ok = more && (ret == RcOk)
        && (var.Leaf == OamAttrRstpBridgeForwardDelay)
        && (var.Width == 0);
    if (!ok)
        return RcBadOnuResponse;

    /* Always fail*/
    GetNextOamVar(&bufInfo, &var, &ret);
    ok = (var.Leaf == OamAttrRstpHoldTime);
    if (!ok)
        return RcBadOnuResponse;

    return RcOk;
}

uint8
TkExtOamGetRstpBridge(uint8 pathId, uint8 linkId, TagRstpBridgeCfg * cfg)
{
    BufInfo         bufInfo;
    Bool            ok;
    uint16          tmp16 = 0;
    uint32          rxLen;
    uint8           ret;
    Bool            more;
    tGenOamVar      var;
    uint16          priority;
    uint16          holdTime;
    uint16          maxAge;
    uint16          fwdDelay;
    uint8           mode;
    uint8           *rxTmpBuf = (uint8*)TkGetApiBuf(pathId);

    if (linkId != 0) {
        return RcBadParam;
    }

    if (TkOamMsgPrepare(pathId,&bufInfo, OamExtOpVarRequest) != OK) {
        return RcNoResource;
    }

    ok = AddOamTlv(&bufInfo,
                   OamBranchNameBinding,
                   OamExtNameBridge, 2, (uint8 *) & tmp16);
    ok = ok && OamAddAttrLeaf(&bufInfo, OamAttrRstpBridgePriority);
    ok = ok && OamAddAttrLeaf(&bufInfo, OamExtAttrRstpBridgeMode);
    ok = ok && OamAddAttrLeaf(&bufInfo, OamAttrRstpBridgeMaxAge);
    ok = ok && OamAddAttrLeaf(&bufInfo, OamAttrRstpBridgeForwardDelay);
    ok = ok && OamAddAttrLeaf(&bufInfo, OamAttrRstpHoldTime);

    ret = TxOamDeliver(pathId, linkId, &bufInfo, (uint8 *) rxTmpBuf, &rxLen);
    if (ret != RcOk)
        return ret;
    if (TkDbgLevelIsSet(TkDbgLogTraceEnable))
        BufDump(NULL, rxTmpBuf, rxLen);
    InitBufInfo(&bufInfo, rxLen, rxTmpBuf);
    /* The first branch is - OamBranchNameBinding*/
    more = GetNextOamVar(&bufInfo, &var, &ret);

    ok = more && (ret == RcOk)
        && (var.Branch == OamBranchNameBinding)
        && (var.Leaf == OamExtNameBridge);
    if (!ok)
        return RcBadOnuResponse;

    /* Followed by OamAttrRstpBridgePriority*/
    more = GetNextOamVar(&bufInfo, &var, &ret);
    ok = more && (ret == RcOk)
        && (var.Leaf == OamAttrRstpBridgePriority);
    if (!ok)
        return RcBadOnuResponse;
    priority = TkMakeU16(var.pValue);
    more = GetNextOamVar(&bufInfo, &var, &ret);
    ok = more && (ret == RcOk)
        && (var.Leaf == OamExtAttrRstpBridgeMode);
    if (!ok)
        return RcBadOnuResponse;
    mode = TkMakeU8(var.pValue);

    more = GetNextOamVar(&bufInfo, &var, &ret);
    ok = more && (ret == RcOk)
        && (var.Leaf == OamAttrRstpBridgeMaxAge);
    if (!ok)
        return RcBadOnuResponse;
    maxAge = TkMakeU16(var.pValue);

    more = GetNextOamVar(&bufInfo, &var, &ret);
    ok = more && (ret == RcOk)
        && (var.Leaf == OamAttrRstpBridgeForwardDelay);
    if (!ok)
        return RcBadOnuResponse;
    fwdDelay = TkMakeU16(var.pValue);

    GetNextOamVar(&bufInfo, &var, &ret);
    ok = (ret == RcOk)
        && (var.Leaf == OamAttrRstpHoldTime);
    if (!ok)
        return RcBadOnuResponse;
    holdTime = TkMakeU16(var.pValue);

    cfg->bridgeMode = mode;
    cfg->fwdDelay = fwdDelay;
    cfg->holdTime = holdTime;
    cfg->maxAge = maxAge;
    cfg->priority = priority;

    return ret;
}



uint8
TkExtOamSetRstpPort(uint8 pathId, uint8 linkId,
                    uint8 port, TagRstpPortCfg * cfg)
{
    BufInfo         bufInfo;
    Bool            ok;
    uint8           buf[4];
    uint32          rxLen;
    uint8           ret;
    tGenOamVar      var;
    uint8           *rxTmpBuf = (uint8*)TkGetApiBuf(pathId);
    
    if (linkId != 0) {
        return RcBadParam;
    }

    if (TkOamMsgPrepare(pathId,&bufInfo, OamExtOpVarSet)
        != OK) {
        return RcNoResource;
    }

    Tk2BufU16(buf, port);
    ok = AddOamTlv(&bufInfo,
                   OamBranchNameBinding, OamExtNameBridgePort, 2, buf);

    Tk2BufU8(buf, (uint8) cfg->priority);
    ok = ok &&
        AddOamTlv(&bufInfo, OamBranchAttribute,
                  OamAttrRstpPortPriority, 1, buf);
    Tk2BufU32(buf, (uint32) cfg->pathCost);
    ok = ok &&
        AddOamTlv(&bufInfo, OamBranchAttribute,
                  OamAttrRstpPathCost, sizeof(uint32), buf);

    ret = TxOamDeliver(pathId, linkId, &bufInfo, (uint8 *) rxTmpBuf, &rxLen);
    if (ret != RcOk)
        return ret;

    if (TkDbgLevelIsSet(TkDbgLogTraceEnable))
        BufDump(NULL, rxTmpBuf, rxLen);

    ok = SearchBranchLeaf(rxTmpBuf,
                          OamBranchAttribute, OamAttrRstpPortPriority,
                          &var);
    if ((ok != RcOk) || (var.Width != 0)) {
        return RcFail;
    }

    return RcOk;
}

uint8
TkExtOamGetRstpPort(uint8 pathId, uint8 linkId,
                    uint8 port, TagRstpPortCfg * cfg)
{
    BufInfo         bufInfo;
    Bool            ok;
    uint32          rxLen;
    uint8           ret;
    tGenOamVar      var;
    uint8           buf[4];
    uint8           priority;
    uint32          pathCost;
    uint8           *rxTmpBuf = (uint8*)TkGetApiBuf(pathId);
    
    /* Sanity check*/
    if (linkId != 0) {
        return RcBadParam;
    }

    if (TkOamMsgPrepare(pathId,&bufInfo, OamExtOpVarRequest) != OK) {
        return RcNoResource;
    }

    Tk2BufU16(buf, port);
    ok = AddOamTlv(&bufInfo,
                   OamBranchNameBinding, OamExtNameBridgePort, 2, buf);
    ok = ok && OamAddAttrLeaf(&bufInfo, OamAttrRstpPortPriority);
    ok = ok && OamAddAttrLeaf(&bufInfo, OamAttrRstpPathCost);

    ret = TxOamDeliver(pathId, linkId, &bufInfo, (uint8 *) rxTmpBuf, &rxLen);
    if (ret != RcOk)
        return ret;

    if (TkDbgLevelIsSet(TkDbgLogTraceEnable))
        BufDump(NULL, rxTmpBuf, rxLen);

    var.pValue = NULL;

    ok = SearchBranchLeaf(rxTmpBuf,
                          OamBranchAttribute, OamAttrRstpPortPriority,
                          &var);
    if ((ok != RcOk) || (var.pValue == NULL)) {
        return RcFail;
    }
    priority = TkMakeU8(var.pValue);

    ok = SearchBranchLeaf(rxTmpBuf,
                          OamBranchAttribute, OamAttrRstpPathCost, &var);
    if ((ok != RcOk) || (var.pValue == NULL)) {
        return RcFail;
    }
    pathCost = TkMakeU32(var.pValue);

    cfg->priority = priority;
    cfg->pathCost = pathCost;

    return ret;
}
