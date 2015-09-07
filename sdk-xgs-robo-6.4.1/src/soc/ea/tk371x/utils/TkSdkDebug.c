/*
 * $Id: TkSdkDebug.c,v 1.6 Broadcom SDK $
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
 * File:     TkSdkDebug.c
 * Purpose: 
 *
 */

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkDebug.h>
#include <soc/ea/tk371x/TkOsCm.h>

uint32 g_ulTkSdkDbgLevelSwitch = 0x0;

char *TkDbgStr[] = {
    "TkDbgLogTraceEnable",
    "TkDbgRxDataEnable",
    "TkDbgTxDataEnable",
    "TkDbgMsgEnable",
    "TkDbgAlmEnable",
    "TkDbgOamEnable",
    "TkDbgOamTkEnable",
    "TkDbgOamCtcEnable",
    NULL
};

Bool
TkDbgLevelIsSet(uint32 lvl)
{
    if (lvl & g_ulTkSdkDbgLevelSwitch) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void
TkDbgDataDump(uint8 * p, uint16 len, uint16 width)
{
    static char  stringbuff[TkDbgMaxMsgLength];
    int  index;
    char *pstr = stringbuff; 
    int  i;

    if(len*3 >= TkDbgMaxMsgLength){
        len = TkDbgMaxMsgLength/3;
    }
    
    for(index = 0; index < (len/width +1); index++){
        for(i = 0; i < width; i++){
            sal_sprintf((char *)(pstr + i*3), "%02x ", p[index*width + i]);
        }
        TkDbgPrintf(("%s\n", stringbuff));
    }
}

void 
TkDbgLevelSet(uint32 lvl)
{
    g_ulTkSdkDbgLevelSwitch = lvl;
}

void
TkDbgLevelDump(void)
{
    int i;
    
    TkDbgPrintf(("DbgBitMap = 0x%08x\n",g_ulTkSdkDbgLevelSwitch));
    
    for(i = 0; TkDbgStr[i] != NULL; i++){
         TkDbgPrintf(("Bit % 2d:%s\n",i,TkDbgStr[i]));   
    }
}


