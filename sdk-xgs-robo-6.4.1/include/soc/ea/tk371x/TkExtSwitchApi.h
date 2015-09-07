/*
 * $Id: TkExtSwitchApi.h,v 1.4 Broadcom SDK $
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
 * File:     TkExtSwitchApi.h
 * Purpose: 
 *
 */
 
#ifndef _SOC_EA_TkExtSwitchApi_H
#define _SOC_EA_TkExtSwitchApi_H

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/Oam.h>


/* Defines to control UNI ingress/egress VLAN policy*/
typedef OamUniVlanPolicy    TkOnuVlanUniPolicy;

/* Defines to control per VLAN ingress/egress tagging behavior*/
typedef OamVlanTagBehave    TkOnuVlanTagBehave;
typedef OamVlanPolicyCfg    TkOnuVlanPolicyCfg;
typedef OamVlanMemberCfg    TkOnuVlanMemberCfg;

typedef struct {
    uint16              numVlans;   /* Number of VLANs in this message */
    TkOnuVlanMemberCfg  member[1];  /* VLAN specific information */
} PACK OamVlanCfgTlv;

/* Switch port mirror */
typedef struct {
    uint8   ingPort;
    uint8   egPort;
    uint16  ingMask;
    uint16  egMask;
} TagPortMirrorCfg;

uint8   TkExtOamSetPortMirror (uint8 pathId, uint8 linkId, uint8 ingPort, 
                uint8 egPort, uint16 ingMask, uint16 egMask);


uint8   TkExtOamGetPortMirror (uint8 pathId, uint8 linkId, 
                TagPortMirrorCfg * cfg);

/* send TK extension OAM message Get Local Switching states */
int     TkExtOamGetLocalSwitch (uint8 pathId, uint8 LinkId, Bool * Status);

/* send TK extension OAM message Set Local Switching states */
uint8   TkExtOamSetLocalSwitch (uint8 pathId, uint8 LinkId, Bool Status);

int     TkOnuSetPortVlanPolicy (uint8 pathId, uint8 linkId, uint8 portId,
                TkOnuVlanPolicyCfg * portVlanPolicyCfg);

int     TkOnuGetPortVlanPolicy (uint8 pathId, uint8 linkId, uint8 portId,
                TkOnuVlanPolicyCfg * portVlanPolicyCfg);

int     TkOnuSetPortVlanMembership (uint8 pathId, uint8 linkId, uint8 portId,
                uint16 numOfEntry,
                TkOnuVlanMemberCfg * portVlanMemberCfg);

int     TkOnuGetPortVlanMembership (uint8 pathId, uint8 linkId, uint8 portId,
                uint16 * numOfEntry,
                TkOnuVlanMemberCfg * portVlanMemberCfg);

#if defined(__cplusplus)
}
#endif

#endif /* _SOC_EA_TkExtSwitchApi_H */
