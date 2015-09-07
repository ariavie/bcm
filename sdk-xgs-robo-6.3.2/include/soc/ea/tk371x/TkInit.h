/*
 * $Id: TkInit.h 1.1 Broadcom SDK $
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
 * File:        name.h
 * Purpose:     Purpose of the file
 */

#ifndef _EA_SOC_TkInit_H_
#define _EA_SOC_TkInit_H_

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkDefs.h>
#include <soc/ea/tk371x/TkOsThread.h>
#include <soc/ea/tk371x/TkOsAlloc.h>
#include <soc/ea/tk371x/TkMsg.h>

#define SEM_VALID(sem)              1   

#define TK_OAM_REPLY_TIME_OUT_MS    40000000

typedef struct {
#define TK_VLAN_LINK_STRING_LEN     20
    int32             vlan;
    uint8             link;
    sal_msg_desc_t  * resMsgQid;
    char              resMsgName[TK_VLAN_LINK_STRING_LEN];
    uint8           * resMsgBuff;
    int16             resMsgLen;
    sal_msg_desc_t  * almMsgQid;
    char              almMsgName[TK_VLAN_LINK_STRING_LEN];
    uint8           * almMsgBuff;
    int16             almMsgLen;
    sal_sem_t         semId;
    char              oltSemName[TK_VLAN_LINK_STRING_LEN];
    sal_thread_t      almTaskId;
    char              almTaskName[TK_VLAN_LINK_STRING_LEN];
    uint8           * apiRxBuff;
    int32             timeOut;
} VlanLinkConfig;

extern VlanLinkConfig gTagLinkCfg[MAX_NUM_OF_PON_CHIP+1];

VlanLinkConfig  * TkGetVlanLink (uint8 pathId);
void            * TkGetApiBuf (uint8 pathId);

/* the system task exit, delete the tasks, message queue, semphore etc. */
void    TkExtOamTaskExit (uint8 pathId);

/* the system task init */
int32   TkExtOamTaskInit (uint8 pathId);



#if defined(__cplusplus)
}
#endif

#endif	/* !_EA_SOC_TkInit_H_ */
