/*
 * $Id: TkOamMem.h 1.1 Broadcom SDK $
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
 * File:     TkOamMem.h
 * Purpose: 
 *
 */
 
#ifndef _SOC_EA_TkOamMem_H
#define _SOC_EA_TkOamMem_H

#ifdef __cplusplus
extern "C"  {
#endif

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkOsSync.h>

#define TKOAM_MEM_DEBUG     

typedef struct TkOamMem_s { /* memory control block */
    void    * BlkMemAddr;   /* Pointer to beginning of memory partition */
    void    * BlkMemFreeList;  /* Pointer to list of free memory blocks */
    uint32    BlkMemSize;   /* Size (in bytes) of each block of memory */
    uint32    MemNBlks;     /* Total number of blocks in this partition */
    uint32    MemNFree;     /* Number of memory blocks remaining in this 
                             * partition */
    sal_sem_t BlkMemSem;    /* semphore for block memory safe operation. */
#ifdef TKOAM_MEM_DEBUG
    uint32    MemGetFailed; /* Get failed */
#endif /* TKOAM_MEM_DEBUG */
} TkOamMem;


/* TkOamMemInit - init the OAM memory management */
extern int  TkOamMemInit (uint8 pathId, uint32 Blocks, uint32 Size);

/* TkOamMemGet - Get a memory block from a partition */
extern void *TkOamMemGet (uint8 pathId);

/* TkOamMemPut - Returns a memory block to a partition */
extern int  TkOamMemPut (uint8 pathId, void *pblk);

/* TkOamMemStatus - Display the number of free memory blocks and the number of
                    used memory blocks from a memory partition. */
extern int  TkOamMemStatus (uint8 pathId);

/* TkOamMemsFree - free the memory of one memory partition */
extern void TkOamMemsFree (uint8 pathId);


#ifdef __cplusplus
}
#endif

#endif /* TKOAMMEM_H */
