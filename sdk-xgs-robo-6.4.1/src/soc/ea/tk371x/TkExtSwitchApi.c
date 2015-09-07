/*
 * $Id: TkExtSwitchApi.c,v 1.4 Broadcom SDK $
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
 * File:     TkExtSwitchApi.c
 * Purpose: 
 *
 */

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkDefs.h>
#include <soc/ea/tk371x/TkOsUtil.h> 
#include <soc/ea/tk371x/TkOsAlloc.h>
#include <soc/ea/tk371x/Oam.h>
#include <soc/ea/tk371x/OamUtils.h>
#include <soc/ea/tk371x/TkUtils.h>
#include <soc/ea/tk371x/TkInit.h>
#include <soc/ea/tk371x/TkDebug.h>
#include <soc/ea/tk371x/TkExtSwitchApi.h>

/*******************************************************************************
* TkExtOamSetPortMirror
*/
uint8           
TkExtOamSetPortMirror(uint8 pathId, uint8 linkId, uint8 ingPort,
     uint8 egPort, uint16 ingMask, uint16 egMask) 
{
    uint8           buf[24];
    uint16          tmp;

    if (linkId != 0) {
        return (OamVarErrActBadParameters);
    }

    if (ingPort == 0 || egPort == 0) {
        return (OamVarErrActBadParameters);
    }

    buf[0] = ingPort;
    buf[1] = egPort;
    tmp = soc_htons(ingMask);
    bcopy((uint8 *) & tmp, &buf[2], sizeof(ingMask));
    tmp = soc_htons(egMask);
    bcopy((uint8 *) & tmp, &buf[4], sizeof(ingMask));

    return (TkExtOamSet
            (pathId, linkId, OamBranchAttribute,
             OamExtAttrSwitchPortMir, buf, 6));
}

uint8
TkExtOamGetPortMirror(uint8 pathId, uint8 linkId, TagPortMirrorCfg * cfg)
{
    uint8           buf[24];
    uint32          size;
    uint8           ret;

    /* Only a base link id of 0 is supported for this interface*/
    if ((linkId != 0) || (cfg == NULL)) {
        return (OamVarErrActBadParameters);
    }

    ret =
        TkExtOamGet(pathId, linkId,
                    OamBranchAttribute, OamExtAttrSwitchPortMir, buf,
                    &size);
    if ((ret != OK) || (size != 6)) {
        return OamVarErrUnknow;
    }

    cfg->ingPort = buf[0];
    cfg->egPort = buf[1];
    cfg->ingMask = TkMakeU16(&buf[2]);
    cfg->egMask = TkMakeU16(&buf[4]);

    return (uint8) size;
}

/*
 * send TK extension OAM message Get Local Switching states 
 */
int
TkExtOamGetLocalSwitch(uint8 pathId, uint8 LinkId, Bool * Status)
{
    uint32          DataLen;

    if ((LinkId > 7) || (NULL == Status)) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    if (OK ==
        TkExtOamGet(pathId, LinkId,
                    OamBranchAttribute,
                    OamExtAttrLocalSwitching, (uint8 *) Status, &DataLen))
        return (OK);
    else {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }
}

/*
 * send TK extension OAM message Set Local Switching states 
 */
uint8
TkExtOamSetLocalSwitch(uint8 pathId, uint8 LinkId, Bool Status)
{

    if ((LinkId > 7)
        || (Status != TRUE && Status != FALSE))
        return (OamVarErrActBadParameters);

    return (TkExtOamSet
            (pathId, LinkId, OamBranchAttribute,
             OamExtAttrLocalSwitching, (Bool *) & Status, sizeof(Bool)));
}

int
TkOnuSetPortVlanPolicy(uint8 pathId,
                       uint8 linkId,
                       uint8 portId,
                       TkOnuVlanPolicyCfg * portVlanPolicyCfg)
{
    OamObjIndex     index;
    TkOnuVlanPolicyCfg tmpPortVlanCfg;

    if ((linkId > SDK_MAX_NUM_OF_LINK) ||
           (portId > SDK_MAX_NUM_OF_PORT)
           || (NULL == portVlanPolicyCfg)
        ) {
        return (OamVarErrActBadParameters);
    }

    index.portId = portId;

    sal_memset((uint8 *) & tmpPortVlanCfg, 0x0,
               sizeof(TkOnuVlanPolicyCfg));

    tmpPortVlanCfg.options = soc_htons(portVlanPolicyCfg->options);
    tmpPortVlanCfg.ingressPolicy = portVlanPolicyCfg->ingressPolicy;
    tmpPortVlanCfg.egressPolicy = portVlanPolicyCfg->egressPolicy;
    tmpPortVlanCfg.defaultVlan = soc_htons(portVlanPolicyCfg->defaultVlan);

    return (TkExtOamObjSet
            (pathId, linkId, OamNamePhyName,
             &index, OamBranchAttribute,
             OamExtAttrPortVlanPolicy,
             (uint8 *) & tmpPortVlanCfg, sizeof(TkOnuVlanPolicyCfg)));
}

int
TkOnuGetPortVlanPolicy(uint8 pathId,
                       uint8 linkId,
                       uint8 portId,
                       TkOnuVlanPolicyCfg * portVlanPolicyCfg)
{
    OamObjIndex     index;
    TkOnuVlanPolicyCfg tmpPortVlanCfg;
    uint32          len = 0;
 
    if ((linkId > SDK_MAX_NUM_OF_LINK) ||
           (portId > SDK_MAX_NUM_OF_PORT)
           || (NULL == portVlanPolicyCfg)
        ) {
        return (OamVarErrActBadParameters);
    }

    sal_memset((uint8 *) & tmpPortVlanCfg, 0x0,
               sizeof(TkOnuVlanPolicyCfg));

    index.portId = portId;

    if (OK ==
        TkExtOamObjGet(pathId, linkId,
                       OamNamePhyName, &index,
                       OamBranchAttribute,
                       OamExtAttrPortVlanPolicy,
                       (uint8 *) & tmpPortVlanCfg, &len)) {
        portVlanPolicyCfg->options = soc_ntohs(tmpPortVlanCfg.options);
        portVlanPolicyCfg->ingressPolicy = tmpPortVlanCfg.ingressPolicy;
        portVlanPolicyCfg->egressPolicy = tmpPortVlanCfg.egressPolicy;
        portVlanPolicyCfg->defaultVlan =
            soc_ntohs(tmpPortVlanCfg.defaultVlan);
        return (OK);
    } else {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }
}

int
TkOnuSetPortVlanMembership(uint8 pathId,
                           uint8 linkId,
                           uint8 portId,
                           uint16 numOfEntry,
                           TkOnuVlanMemberCfg * portVlanMemberCfg)
{
    BufInfo         bufInfo;
    Bool            ok;
    uint8           buf[4];
    uint32          rxLen;
    uint8           ret;
    tGenOamVar      var;
    Bool            more;
    uint16          tmpCnt = 0;
    OamVlanCfgTlv   tmpOamVlanCfg;
    uint8           *rxTmpBuf = (uint8*)TkGetApiBuf(pathId);

    if ((linkId > SDK_MAX_NUM_OF_LINK) 
        ||(portId > SDK_MAX_NUM_OF_PORT) 
        ||((NULL == portVlanMemberCfg)
        && (numOfEntry == 0))
        || ((NULL == portVlanMemberCfg)
        && (numOfEntry != 0))) {
        return (OamVarErrActBadParameters);
    }

    if (TkOamMsgPrepare(pathId,&bufInfo, OamExtOpVarSet)
        != OK) {
        return RcNoResource;
    }

    Tk2BufU16(buf, portId);
    ok = AddOamTlv(&bufInfo, OamBranchNameBinding, OamNamePhyName, 2, buf);

    for (tmpCnt = 0; (tmpCnt < numOfEntry)
         && (numOfEntry != 0); tmpCnt++) {
        sal_memset(&tmpOamVlanCfg, 0x00, sizeof(OamVlanCfgTlv));
        tmpOamVlanCfg.numVlans = soc_htons(1);
        tmpOamVlanCfg.member[0].ingressVlan =
            soc_htons(portVlanMemberCfg[tmpCnt].ingressVlan);
        tmpOamVlanCfg.member[0].egressVlan =
            soc_htons(portVlanMemberCfg[tmpCnt].egressVlan);
        tmpOamVlanCfg.member[0].ingressTagging =
            portVlanMemberCfg[tmpCnt].ingressTagging;
        tmpOamVlanCfg.member[0].egressTagging =
            portVlanMemberCfg[tmpCnt].egressTagging;
        tmpOamVlanCfg.member[0].flags = portVlanMemberCfg[tmpCnt].flags;
        ok = ok
            && AddOamTlv(&bufInfo,
                         OamBranchAttribute,
                         OamExtAttrPortVlanMembership,
                         sizeof(OamVlanCfgTlv), (uint8 *) & tmpOamVlanCfg);
    }

    if (numOfEntry == 0) {
        sal_memset(&tmpOamVlanCfg, 0x00, sizeof(OamVlanCfgTlv));
        tmpOamVlanCfg.numVlans = 0;
        ok = ok
            && AddOamTlv(&bufInfo,
                         OamBranchAttribute,
                         OamExtAttrPortVlanMembership,
                         2, (uint8 *) & tmpOamVlanCfg);
    }

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
        && (var.Leaf == OamNamePhyName);
    if (!ok)
        return RcBadOnuResponse;

    /* Followed by vlan entries*/

    for (tmpCnt = 0; tmpCnt < numOfEntry; tmpCnt++) {
        more = GetNextOamVar(&bufInfo, &var, &ret);
        ok = more && (ret == RcOk)
            && (var.Leaf == OamExtAttrPortVlanMembership)
            && (var.Width == 0);
        if (!ok)
            return RcBadOnuResponse;
    }

    return RcOk;
}

int
TkOnuGetPortVlanMembership(uint8 pathId,
                           uint8 linkId,
                           uint8 portId,
                           uint16 * numOfEntry,
                           TkOnuVlanMemberCfg * portVlanMemberCfg)
{
    BufInfo         bufInfo;
    Bool            ok;
    uint32          rxLen;
    uint8           ret;
    Bool            more;
    tGenOamVar      var;
    uint8           buf[4];
    uint16          tmpCnt = 0;
    OamVlanCfgTlv   tmpOamVlanCfg;
    uint8           *rxTmpBuf = (uint8*)TkGetApiBuf(pathId);

    if ( (linkId > SDK_MAX_NUM_OF_LINK) 
        ||(portId > SDK_MAX_NUM_OF_PORT)
        || (NULL == portVlanMemberCfg)){
        return (OamVarErrActBadParameters);
    }


    if (TkOamMsgPrepare(pathId,&bufInfo, OamExtOpVarRequest) != OK) {
        return RcNoResource;
    }

    Tk2BufU16(buf, portId);
    ok = AddOamTlv(&bufInfo, OamBranchNameBinding, OamNamePhyName, 2, buf);
    ok = ok && OamAddAttrLeaf(&bufInfo, OamExtAttrPortVlanMembership);

    ret = TxOamDeliver(pathId, linkId, &bufInfo, (uint8 *) rxTmpBuf, &rxLen);
    if (ret != RcOk)
        return ret;
    
    if (TkDbgLevelIsSet(TkDbgLogTraceEnable))
        BufDump(NULL, rxTmpBuf, rxLen);

    InitBufInfo(&bufInfo, 1500, (uint8 *) rxTmpBuf);

    more = GetNextOamVar(&bufInfo, &var, &ret);
    ok = more && (ret == RcOk)
        && (var.Leaf == OamNamePhyName);
    if (!ok)
        return RcBadOnuResponse;

    tmpCnt = 0;
    *numOfEntry = 0;
    while (1) {
        more = GetNextOamVar(&bufInfo, &var, &ret);
        ok = more && (ret == RcOk)
            && (var.Leaf == OamExtAttrPortVlanMembership);
        if (!ok && tmpCnt == 0) {
            *numOfEntry = tmpCnt;
            return RcBadOnuResponse;
        }
        if (var.pValue == NULL) {
            *numOfEntry = tmpCnt;
            return RcFail;
        }
        sal_memset(&tmpOamVlanCfg, 0x00, sizeof(OamVlanCfgTlv));
        sal_memcpy(&tmpOamVlanCfg, var.pValue, sizeof(OamVlanCfgTlv));
        var.pValue = NULL;
        if (tmpOamVlanCfg.numVlans == 0) {
            *numOfEntry = tmpCnt;
            break;
        }
        portVlanMemberCfg[tmpCnt].ingressVlan =
            soc_ntohs(tmpOamVlanCfg.member[0].ingressVlan);
        portVlanMemberCfg[tmpCnt].egressVlan =
            soc_ntohs(tmpOamVlanCfg.member[0].egressVlan);
        portVlanMemberCfg[tmpCnt].ingressTagging =
            tmpOamVlanCfg.member[0].ingressTagging;
        portVlanMemberCfg[tmpCnt].egressTagging =
            tmpOamVlanCfg.member[0].egressTagging;
        portVlanMemberCfg[tmpCnt].flags = tmpOamVlanCfg.member[0].flags;
        tmpCnt++;
    }
    *numOfEntry = tmpCnt;
    return RcOk;
}

