/*
 * $Id: IeeeAlarmProcess.c 1.2 Broadcom SDK $
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
 * File:     IeeeAlarmProcess.c
 * Purpose: 
 *
 */

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/Oam.h>
#include <soc/ea/tk371x/OamUtils.h>
#include <soc/ea/tk371x/AlarmProcess.h>
#include <soc/ea/tk371x/AlarmProcessIeee.h>
#include <soc/ea/tk371x/AlarmProcessCtc.h>
#include <soc/ea/tk371x/AlarmProcessTk.h>
#include <soc/ea/tk371x/TkDebug.h>


static OamEventVector  StdOamEventVector[] = {
    {OamEventErrSymbolPeriod,
     (OamEventHandler) IeeeEventErrSymPeriod,
     "OamEventErrSymbolPeriod", 0}
    ,
    {OamEventErrFrameCount,
     (OamEventHandler) IeeeEventErrFrame,
     "OamEventErrFrameCount", 0}
    ,
    {OamEventErrFramePeriod,
     (OamEventHandler) IeeeEventErrFrPeriod,
     "OamEventErrFramePeriod", 0}
    ,
    {OamEventErrFrameSecSummary,
     (OamEventHandler) IeeeEventErrFrSecondsSum,
     "OamEventErrFrameSecSummary", 0}
    ,
    {OamEventErrVendor,
     (OamEventHandler) IeeeEventVendorHandler,
     "OamEventErrVendor", 0}
    ,
    {OamEventErrVendorOld, NULL,
     "OamEventErrVendorOld", 0}
    ,
    {OamEventEndOfTlvMarker, NULL,
     "OamEventEndOfTlvMarker", 0}
    ,
};

Bool
IeeeEventIsTkAlarm(uint8 pathId, uint8 linkId, OamEventTlv * pOamEventTlv)
{
    OamEventExt    *pEvent = (OamEventExt *) pOamEventTlv;
    Bool            ret = TRUE;

    if (NULL == pOamEventTlv) {
        ret = FALSE;
    }

    if (OamEventErrVendor == pEvent->tlv.type
        && !sal_memcmp((void *) &(pEvent->oui),
                       (void *) &TeknovusOui, sizeof(IeeeOui))) {
        ret = TRUE;
    } else {
        ret = FALSE;
    }

    return ret;
}

Bool
IeeeEventIsCtcAlarm(uint8 pathId, uint8 linkId, OamEventTlv * pOamEventTlv)
{
    OamEventExt    *pEvent = (OamEventExt *) pOamEventTlv;
    Bool            ret = TRUE;

    if (NULL == pOamEventTlv) {
        ret = FALSE;
    }

    if (OamEventErrVendor == pEvent->tlv.type
        && !sal_memcmp((void *) &(pEvent->oui),
                       (void *) &CTCOui, sizeof(IeeeOui))) {
        ret = TRUE;
    } else {
        ret = FALSE;
    }

    return ret;
}

int32
IeeeEventErrSymPeriod(uint8 pathId, uint8 linkId,
                                OamEventTlv * pOamEventTlv)
{
    if (NULL == pOamEventTlv) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    return OK;
}

int32
IeeeEventErrFrame(uint8 pathId, uint8 linkId,OamEventTlv * pOamEventTlv)
{
    if (NULL == pOamEventTlv) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    return OK;
}

int32
IeeeEventErrFrPeriod(uint8 pathId, uint8 linkId,OamEventTlv * pOamEventTlv)
{
    if (NULL == pOamEventTlv) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    return OK;
}

int32
IeeeEventErrFrSecondsSum(uint8 pathId,
                         uint8 linkId, OamEventTlv * pOamEventTlv)
{
    if (NULL == pOamEventTlv) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    return OK;
}

int32
IeeeEventVendorHandler(uint8 pathId,
                       uint8 linkId, OamEventTlv * pOamEventTlv)
{
    OamEventExt    *pEvent;
    int32           ret = OK;

    if (NULL == pOamEventTlv) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    pEvent = (OamEventExt *) pOamEventTlv;

    if (!sal_memcmp
        ((void *) &(pEvent->oui), (void *) &CTCOui, sizeof(IeeeOui))) {
        ret = CtcEventProcessHandler(pathId, linkId, pEvent);
    } else
        if (!sal_memcmp
            ((void *) &(pEvent->oui),
             (void *) &TeknovusOui, sizeof(IeeeOui))) {
        ret = TkEventProcessHandler(pathId, linkId, pEvent);
    } else {
        ret = ERROR;
        TkDbgTrace(TkDbgErrorEnable);
    }

    return ret;
}

int32
IeeeEventHandler(uint8 pathId, uint8 linkId, OamEventTlv * pOamEventTlv)
{
    uint8           id = 0;
    int32           ret = OK;

    if (NULL == pOamEventTlv) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    while (StdOamEventVector[id].eventId != OamEventEndOfTlvMarker) {
        if (StdOamEventVector[id].eventId == pOamEventTlv->type) {
            if (StdOamEventVector[id].handler) {
                ret =
                    StdOamEventVector[id].handler(pathId, linkId,
                                                  pOamEventTlv);
                StdOamEventVector[id].Stats++;
            } else {
                TkDbgTrace(TkDbgErrorEnable);
                ret = ERROR;
            }
            break;
        }
        id++;
    }

    if (StdOamEventVector[id].eventId == OamEventEndOfTlvMarker) {
        TkDbgInfoTrace(TkDbgErrorEnable, ("Error Ieee Alarm Message.\n"));
    }
    return ret;
}

