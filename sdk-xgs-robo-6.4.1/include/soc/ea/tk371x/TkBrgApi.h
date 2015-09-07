/*
 * $Id: TkBrgApi.h,v 1.7 Broadcom SDK $
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
 * File:     TkBrgApi.h
 * Purpose: 
 *
 */
 
#ifndef _SOC_EA_TkBrgApi_H
#define _SOC_EA_TkBrgApi_H

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/Ethernet.h>
#include <soc/ea/tk371x/Oam.h>


typedef enum {
    FlushDynamicMac = 0,
    FlushAllMac     = 1,
    FlushStaticMac  = 2,
} FlushMacType;

typedef enum {
    ArlDisLearning  = 0,
    ArlHwLearning   = 1,
    ArlSftLearning  = 2,
} ArlLearnStatus;

typedef enum {
    ForwardFloodUnknown,
    ForwardDropUnknown,
    ForwardPassUntilFull
} ForwardingMode;

typedef enum {
    LoopbackEnable = 1,
    LoopbackDisable = 2,
    LoopbackCount
} LoopbackState;

#define BRG_ARL_LIMIT_NONE    (-1)

/* send TK extension OAM message Get maximum size of learned address table
 *  of UNI #x from the ONU 
 */
int     TkExtOamGetDynaMacTabSize (uint8 pathId, uint8 LinkId, uint8 port,
                uint16 * pDynaMacTabSize);

/* send TK extension OAM message Set maximum size of learned address table 
 * of UNI #x to the ONU 
 */
int     TkExtOamSetDynaMacTabSize (uint8 pathId, uint8 LinkId, uint8 port,
                uint16 DynaMacTabSize);

/* send TK extension OAM message Get maximum size of learned address table
 *  of UNI #x from the ONU 
 */
int     TkExtOamGetDynaMacTabSizeNew (uint8 pathId, uint8 LinkId, uint8 port,
                int * pDynaMacTabSize);

/* send TK extension OAM message Set maximum size of learned address table 
 * of UNI #x to the ONU 
 */
int     TkExtOamSetDynaMacTabSizeNew (uint8 pathId, uint8 LinkId, uint8 port,
                int DynaMacTabSize);


/* send TK extension OAM message Get learning aging timer of UNI #x from 
 * the ONU */
int     TkExtOamGetDynaMacTabAge (uint8 pathId, uint8 LinkId, uint8 port,
                uint16 * pDynaMacTabAge);

/* send TK extension OAM message Set learning aging timer of UNI #x to the 
 * ONU 
 */
int    TkExtOamSetDynaMacTabAge (uint8 pathId, uint8 LinkId, uint8 port,
                uint16 DynaMacTabAge);

/* send TK extension OAM message Get dynamically learned MAC address of 
 * UNI #x from the ONU */
int     TkExtOamGetDynaMacEntries (uint8 pathId, uint8 LinkId, uint8 port,
                uint8 * pNumEntries, MacAddr * pDynaMacEntries);

/* send TK extension OAM message Set clear dynamically learned MAC address 
 * of UNI#x to the ONU */
int     TkExtOamSetClrDynaMacTable (uint8 pathId, uint8 LinkId, uint8 port);

/* Send TK externsion OAM message Get StaticMacTbl */
int     TkExtOamGetStaticMacEntries (uint8 pathId, uint8 LinkId, uint8 port,
                uint8 * pNumEntries, MacAddr * pStaticMacEntries);

/* Send TK externsion OAM message Add StaticMacEntry */
int     TkExtOamAddStaticMacEntry (uint8 pathId, uint8 LinkId, uint8 port,
                MacAddr * pMacEntry);

/*Send TK externsion OAM message Delete StaticMacEntry */
int     TkExtOamDelStaticMacEntry (uint8 pathId, uint8 LinkId, uint8 port,
                MacAddr * pMacEntry);

/*Send TK externsion OAM message flush mac entry*/
int     TkExtOamFlushMacTable (uint8 pathId, uint8 LinkId, uint8 port,
                uint8 type);
int     TkExtOamFlushMacTableNew(uint8 pathId, uint8 LinkId, uint8 port, 
                uint8 type);

int     TkExtOamSetMacLearning (uint8 pathId, uint8 LinkId, uint8 port,
                uint8 status);
int     TkExtOamSetForwardMode (uint8 pathId, uint8 LinkId, uint8 port,
                ForwardingMode mode);
int     TkExtOamSetAutoNeg (uint8 pathId, uint8 LinkId, uint8 port,
                OamAutoNegAdminState AutoEnable, uint16 speed,
                OamMacDuplexStatus mode);
int     TkExtOamGetAutoNeg (uint8 pathId, uint8 linkId, uint8 port,
                OamAutoNegAdminState * AutoEnable, uint16 * speed,
                OamMacDuplexStatus * mode);
int     TkExtOamSetMtu (uint8 pathId, uint8 LinkId, uint8 port,
                uint16 maxFrameSize);
int     TkExtOamGetMtu (uint8 pathId, uint8 linkId, uint8 port,
                uint16 * maxFrameSize);
int     TkExtOamSetFloodUnknown (uint8 pathId, uint8 LinkId, Bool status);
int     TkExtOamGetFloodUnknown (uint8 pathId, uint8 linkId, Bool * status);
int     TkExtOamSetMacLearnSwitch (uint8 pathId, uint8 LinkId,
                ArlLearnStatus status);
int     TkExtOamGetEthLinkState (uint8 pathId, uint8 linkId, uint8 port,
                uint8 * linkStatus);
int     TkExtOamSetPhyAdminState (uint8 pathId, uint8 LinkId, uint8 port,
                uint8 adminState);
int     TkExtOamGetPhyAdminState (uint8 pathId, uint8 linkId, uint8 port,
                uint8 * adminState);
int     TkExtOamClrAllFilterTbl (uint8 pathId, uint8 LinkId, uint8 port);

int     TkExtOamLlidSetLoopback(uint8 pathId, uint8 linkId, uint8 llid,
                OamLoopbackLoc loc);
int     TkExtOamPortSetLoopback(uint8 pathId, uint8 LinkId, uint8 port, 
                OamLoopbackLoc loc);

#if defined(__cplusplus)
}
#endif

#endif /* _SOC_EA_TkBrgApi_H */
