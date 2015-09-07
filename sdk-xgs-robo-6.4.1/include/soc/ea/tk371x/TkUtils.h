/*
 * $Id: TkUtils.h,v 1.1 Broadcom SDK $
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
 * File:     tkUtils.h
 * Purpose: 
 *
 */

#ifndef _SOC_EA_TKUTILS_H
#define _SOC_EA_TKUTILS_H

#if defined(__cplusplus)
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/Oam.h>
#include <soc/ea/tk371x/CtcOam.h>

#define TkMakeU8(x)     (uint8)(*(uint8 *)x)
#define TkMakeU16(x)    (uint16)((*(uint8 *)x<<8)|*((uint8 *)x+1))
#define TkMakeU32(x)    (uint32)((*(uint8 *)x<<24)|(*((uint8 *)x+1)<<16) \
                                |(*((uint8 *)x+2)<<8)|(*((uint8 *)x+3)))


/************************************************************************
 * BufInfo Stuff
 ************************************************************************/

typedef struct {
    uint8     * start;       /* Pointer to the start of the buffer*/
    uint8     * curr;        /* Pointer to the current position in the buffer*/
    uint16      len;         /* Total Length of buffer*/
} BufInfo;

void    InitBufInfo (BufInfo * buf, uint16 size, uint8 * start);
Bool    BufSkip (BufInfo * buf, uint16 len);
Bool    BufRead (BufInfo * buf, uint8 * to, uint16 len);
Bool    BufReadU8 (BufInfo * buf, uint8 * val);
Bool    BufReadU16 (BufInfo * buf, uint16 * val);
Bool    BufWrite (BufInfo * buf, const uint8 * from, uint16 len);
Bool    BufWriteU16 (BufInfo * buf, uint16 val);
uint16  BufGetUsed (const BufInfo * buf);
uint16  BufGetRemainingSize (const BufInfo * buf);

/************************************************************************
 * Miscellaneous Stuff
 ************************************************************************/

void    htonll (uint64 src, uint64 *dst);

void Tk2BufU8 (uint8 * buf, uint8 val);
void Tk2BufU16 (uint8 * buf, uint16 val);
void Tk2BufU32 (uint8 * buf, uint32 val);

void    BufDump (char *title, uint8 * buf, uint16 len);

/************************************************************************
 * OAM Handling Routines
 ************************************************************************/
Bool    AddOamTlv (BufInfo * bufInfo, OamVarBranch branch, uint16 leaf,
                uint8 len, const uint8 * value);
Bool    FormatBranchLeaf (BufInfo * bufInfo, uint8 branch, OamAttrLeaf leaf);
Bool    OamAddAttrLeaf (BufInfo * bufInfo, uint16 leaf);
Bool    OamAddCtcExtAttrLeaf (BufInfo * bufInfo, uint16 leaf);
Bool    GetNextOamVar (BufInfo * pBufInfo, tGenOamVar * pOamVar, uint8 * tlvRet);
Bool    GetEventTlv (BufInfo * pBufInfo, OamEventTlv ** pOamEventTlv, 
                int *tlvError);
uint8   SearchBranchLeaf (void *oamResp, OamVarBranch branch, uint16 leaf,
                tGenOamVar * pOamVar);

void    RuleDebug (uint8 * buf, uint32 len);
void    CtcPortInstShow (CtcPortInst port);
void    conditonBubbleSort (OamNewRuleCondition * pData, int32 count);



#if defined(__cplusplus)
}
#endif

#endif /* _SOC_EA_TKUTILS_H */
