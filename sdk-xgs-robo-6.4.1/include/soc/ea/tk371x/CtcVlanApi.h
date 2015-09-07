/*
 * $Id: CtcVlanApi.h,v 1.3 Broadcom SDK $
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
 * File:     CtcVlanApi.h
 * Purpose: 
 *
 */
 
#ifndef _SOC_EA_CtcVlanApi_H
#define _SOC_EA_CtcVlanApi_H

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/Ethernet.h>
#include <soc/ea/tk371x/CtcOam.h>


typedef struct {
    EthernetVlanData    vlanTag;
} CtcVlanTagInfo;

typedef struct {
    uint32              numOfEntry;
    EthernetVlanData    defaultVlanInfo;
    CtcVlanTranslatate  vlanTranslateArry[0];
} CtcVlanTranslateInfo;

typedef struct {
    uint32              numOfEntry;
    EthernetVlanData    defaultVlanInfo;
    EthernetVlanData    vlanTrunkArry[0];
} CtcVlanTrunkInfo;

typedef struct {
    uint32                      mode;
    union {
        CtcVlanTagInfo          tagInfo;
        CtcVlanTranslateInfo    translateInfo;
        CtcVlanTrunkInfo        trunkInfo;
    } CtcVlanArry;
} CtcVlanEntryInfo;

int     CtcExtOamSetVlanTransparent (uint8 pathId, uint8 LinkId, uint32 port);

int     CtcExtOamSetVlanTag (uint8 pathId, uint8 LinkId, uint32 port, 
                CtcOamVlanTag * pCtcOamVlanTag);

int     CtcExtOamSetVlanTranslation (uint8 pathId, uint8 LinkId, uint32 port,
                EthernetVlanData defaultVlan, uint32 numOfTranslateEntry, 
                CtcVlanTranslatate * pCtcVlanTranslatate);

int     CtcExtOamSetVlanTrunk (uint8 pathId, uint8 LinkId, uint32 port,
                EthernetVlanData defaultVlan, uint32 numOfTrunkEntry, 
                EthernetVlanData * pCtcVlanTrunkEntry);

int     CtcExtOamGetVlan (uint8 pathId, uint8 LinkId, uint32 port, 
                CtcVlanEntryInfo * pVlanEntry);

#if defined(__cplusplus)
}
#endif

#endif /* _SOC_EA_CtcVlanApi_H */
