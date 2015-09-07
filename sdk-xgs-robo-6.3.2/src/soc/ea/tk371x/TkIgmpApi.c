/*
 * $Id: TkIgmpApi.c 1.2 Broadcom SDK $
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
 * File:     TkIgmpApi.c
 * Purpose: 
 *
 */

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkOsUtil.h> 
#include <soc/ea/tk371x/Oam.h>
#include <soc/ea/tk371x/OamUtils.h>
#include <soc/ea/tk371x/TkInit.h>
#include <soc/ea/tk371x/TkDebug.h>
#include <soc/ea/tk371x/TkIgmpApi.h>

/*
 * send TK extension OAM message Set igmp configuration to the ONU 
 */
uint8
TkExtOamSetIgmpConfig(uint8 pathId, uint8 LinkId,
                      OamIgmpConfig * pIgmpConfig)
{
    uint8           size;

    if ((LinkId > 7) || (NULL == pIgmpConfig))
        return (OamVarErrActBadParameters);

    size = (3 * sizeof(uint8)) +
        (pIgmpConfig->numPorts * sizeof(OamIgmpPortConfig));
    return (TkExtOamSet
            (pathId, LinkId, OamBranchAction,
             OamExtActSetIgmpConfig, (uint8 *) pIgmpConfig, size));
}


/*
 * send TK extension OAM message Get igmp configuration from the ONU 
 */
int
TkExtOamGetIgmpConfig(uint8 pathId, uint8 LinkId,
                      OamIgmpConfig * pIgmpConfig)
{
    uint32          DataLen;

    if ((LinkId > 7) || (NULL == pIgmpConfig)) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }
    if (OK ==
        TkExtOamGet(pathId, LinkId,
                    OamBranchAction,
                    OamExtActGetIgmpConfig, (uint8 *) pIgmpConfig,
                    &DataLen))
        return (OK);
    else {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }
}


/*
 * send TK extension OAM message Get igmp group info from the ONU 
 */
int
TkExtOamGetIgmpGroupInfo(uint8 pathId,
                         uint8 LinkId, OamIgmpGroupConfig * pIgmpGroupInfo)
{
    uint32          DataLen;
    uint8           *rxTmpBuf = (uint8*)TkGetApiBuf(pathId);
    
    if ((LinkId > 7) || (NULL == pIgmpGroupInfo)) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    if (OK ==
        TkExtOamGetMulti(pathId, LinkId,
                         OamBranchAction,
                         OamExtActGetIgmpGroupInfo,
                         (uint8 *) rxTmpBuf, &DataLen)) {
        OamVarContainer *var;
        uint8          *numGroups;
        OamIgmpGroupConfig *info = NULL;
        OamIgmpGroupInfo *pGgroup;

        numGroups = (uint8 *) pIgmpGroupInfo;
        pGgroup = (OamIgmpGroupInfo *) INT_TO_PTR(PTR_TO_INT(pIgmpGroupInfo) + sizeof(uint8));
        *numGroups = 0;
        var = (OamVarContainer *) rxTmpBuf;

        while (var->branch != OamBranchTermination) {
            if ((var->branch == OamBranchAction)
                && (var->leaf == soc_ntohs(OamExtActGetIgmpGroupInfo))) {
                info = (OamIgmpGroupConfig *) (var->value);
                *numGroups += info->numGroups;

                bcopy((uint8 *) info->group,
                      (uint8 *) pGgroup,
                      info->numGroups * sizeof(OamIgmpGroupInfo));
            }
            /*info may be NULL*/
            if(NULL != info){
                pGgroup += info->numGroups;
                var = NextCont(var);
            }else{
                break;
            }   
        }

        return (OK);
    } else {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }
}


/*
 * send TK extension OAM message Set delete igmp group to the ONU 
 */
uint8
TkExtOamSetDelIgmpGroup(uint8 pathId,
                        uint8 LinkId, OamIgmpGroupConfig * pIgmpGroup)
{
    uint8           size;

    if ((LinkId > 7) || (NULL == pIgmpGroup))
        return (OamVarErrActBadParameters);

    size =
        sizeof(uint8) + (pIgmpGroup->numGroups * sizeof(OamIgmpGroupInfo));
    return (TkExtOamSet
            (pathId, LinkId, OamBranchAction, OamExtActDelIgmpGroup,
             (uint8 *) pIgmpGroup, size));
}


/*
 * send TK extension OAM message Set add igmp group to the ONU 
 */
uint8
TkExtOamSetAddIgmpGroup(uint8 pathId,
                        uint8 LinkId, OamIgmpGroupConfig * pIgmpGroup)
{
    uint8           size;

    if ((LinkId > 7) || (NULL == pIgmpGroup))
        return (OamVarErrActBadParameters);

    size =
        sizeof(uint8) + (pIgmpGroup->numGroups * sizeof(OamIgmpGroupInfo));
    return (TkExtOamSet
            (pathId, LinkId, OamBranchAction, OamExtActAddIgmpGroup,
             (uint8 *) pIgmpGroup, size));

}


/*
 * send TK extension OAM message Set igmp VLAN to the ONU 
 */
uint8
TkExtOamSetIgmpVlan(uint8 pathId, uint8 LinkId, IgmpVlanRecord * pIgmpVlan)
{
    uint8           size;
    uint8           numOfGroup = 0;


    if ((LinkId > 7) || (NULL == pIgmpVlan))
        return (OamVarErrActBadParameters);

    for (numOfGroup = 0; numOfGroup < pIgmpVlan->numVlans; numOfGroup++) {
        pIgmpVlan->vlanCfg[numOfGroup].eponVid =
            soc_htons(pIgmpVlan->vlanCfg[numOfGroup].eponVid);
        pIgmpVlan->vlanCfg[numOfGroup].userVid =
            soc_htons(pIgmpVlan->vlanCfg[numOfGroup].userVid);
    }

    size = sizeof(uint16) + (pIgmpVlan->numVlans * sizeof(IgmpVlanCfg));
    return (TkExtOamSet
            (pathId, LinkId, OamBranchAttribute,
             OamExtAttrOnuIgmpVlan, (uint8 *) pIgmpVlan, size));

}


/*
 * send TK extension OAM message Get igmp VLAN from the ONU 
 */
int
TkExtOamGetIgmpVlan(uint8 pathId, uint8 LinkId, IgmpVlanRecord * pIgmpVlan)
{
    uint32          DataLen;
    uint8           numOfGroup = 0;

    if ((LinkId > 7) || (NULL == pIgmpVlan)) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    if (OK ==
        TkExtOamGet(pathId, LinkId,
                    OamBranchAttribute,
                    OamExtAttrOnuIgmpVlan, (uint8 *) pIgmpVlan,
                    &DataLen)) {
        for (numOfGroup = 0; numOfGroup < pIgmpVlan->numVlans;
             numOfGroup++) {
            pIgmpVlan->vlanCfg[numOfGroup].eponVid =
                soc_ntohs(pIgmpVlan->vlanCfg[numOfGroup].eponVid);
            pIgmpVlan->vlanCfg[numOfGroup].userVid =
                soc_ntohs(pIgmpVlan->vlanCfg[numOfGroup].userVid);
        }
        return (OK);
    } else {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }
}
