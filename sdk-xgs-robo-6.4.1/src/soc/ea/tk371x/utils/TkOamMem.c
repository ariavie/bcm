/*
 * $Id: TkOamMem.c,v 1.1 Broadcom SDK $
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
 * File:     TkOamMem.c
 * Purpose: 
 *
 */

#include <soc/ea/tk371x/TkTypes.h>
#include <soc/ea/tk371x/TkOamMem.h>
#include <soc/ea/tk371x/TkDebug.h>


static TkOamMem TkOamBlkMem[8];
static TkOamMem *pTkOamBlkMem = TkOamBlkMem;

static int 
TkOamMemCreate(uint8 pathId, void *addr, uint32 nblks, uint32 MemSize);

/*
 * TkOamMemInit - init the OAM memory management 
 * 
 * Parameters: 
 *
 * \blocks - number of memory blocks. 
 * \size - the size of each block in
 * the memory partition. 
 * 
 * \return: 
 * OK - if init successfully or 
 *
 * ERROR - if init fail. 
 */
int
TkOamMemInit(uint8 pathId, uint32 Blocks, uint32 Size)
{
    uint8           *buf = NULL;
    char            string[20];

    sal_sprintf((char *) string, "OamBuffer%d", pathId);
    
    buf = soc_cm_salloc(pathId, Size * Blocks, string);
    
    if (NULL == buf) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    if (OK != TkOamMemCreate(pathId, buf, Blocks, Size)) {
        if (buf)
            soc_cm_sfree(pathId, buf);
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    } else
        return (OK);
}


/*
 * TkOamMemCreate - Create a fixed-sized memory partition. 
 * 
 * Parameters: 
 * 
 * \addr - the starting address of the memory partition 
 * \nblks -number of memory blocks to create from the partition. 
 * \MemSize - the size (in bytes and must be multipler 4 bytes) of each block 
 * in the memory partition. 
 * 
 * \return: 
 * OK - if the partition was created or
 * 
 * ERROR - if the partition was not created because of invalid arguments. 
 */
static int
TkOamMemCreate(uint8 pathId, void *addr, uint32 nblks, uint32 MemSize)
{
    uint8          *pblk;
    void          **plink;
    uint32          i;

    /* 
        *   Must pass a valid address for the memory part. 
        *   Must have at least 2 blocks per partition 
        */
    if ((NULL == addr) || (nblks < 2)) {          
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    /* 
        *   Must contain space for at least  a pointer 
        *  Must be multipler 4 bytes 
        */
    if ((MemSize < sizeof(void *)) || (0 != MemSize % 4)) {
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    pTkOamBlkMem[pathId].BlkMemSem =
        sal_sem_create("oamMemSem", sal_sem_BINARY, 1);

    plink = (void **) addr;    
    /* Create linked list of free memory blocks */
    pblk = (uint8 *) INT_TO_PTR(PTR_TO_INT(addr) + MemSize);
    for (i = 0; i < (nblks - 1); i++) {
        *plink = (void *) pblk; 
        /* Save pointer to NEXT block in CURRENT block */
        plink = (void **) pblk; 
        /* Position to NEXT block */
        pblk = (uint8 *) INT_TO_PTR(PTR_TO_INT(pblk) + MemSize); 
        /* Point to the FOLLOWING block */
    }

    *plink = (void *) NULL;     
    /* Last memory block points to NULL */
    pTkOamBlkMem[pathId].BlkMemAddr = addr;    
    /* Store start address of memory partition */
    pTkOamBlkMem[pathId].BlkMemFreeList = addr;    
    /* Initialize pointer to pool of free blocks */
    pTkOamBlkMem[pathId].MemNFree = nblks; 
    /* Store number of free blocks in MCB */
    pTkOamBlkMem[pathId].MemNBlks = nblks;
    pTkOamBlkMem[pathId].BlkMemSize = MemSize; 
    /* Store block size of each memory blocks */
#ifdef TKOAM_MEM_DEBUG
    pTkOamBlkMem[pathId].MemGetFailed = 0;
#endif /* TKOAM_MEM_DEBUG */
    return (OK);
}


/*
 * TkOamMemGet - Get a memory block from a partition 
 * 
 * Parameters: 
 * no
 * 
 * 
 * \return: 
 * A pointer to a memory block if no error is detected or
 * 
 * A pointer to NULL if an error is detected. 
 */
void *
TkOamMemGet(uint8 pathId)
{
    void           *pblk;

    /*
     * If the block memory can be operated? 
     */
    if (sal_sem_take(pTkOamBlkMem[pathId].BlkMemSem, 20000) < 0) {
        return NULL;
    }

    if (pTkOamBlkMem[pathId].MemNFree > 0) {   
        /* See if there are any free  memory blocks */
        pblk = pTkOamBlkMem[pathId].BlkMemFreeList;    
        /* Yes, point to next free  memory block */
        pTkOamBlkMem[pathId].BlkMemFreeList = *(void **) pblk; 
        /* Adjust pointer to new free list */
        pTkOamBlkMem[pathId].MemNFree--;   
        /* One less memory block in this partition */
        /* 
              * Give the block memory semaphore. 
              */
        sal_sem_give(pTkOamBlkMem[pathId].BlkMemSem);
        sal_memset(pblk, 0x00, pTkOamBlkMem[pathId].BlkMemSize);
        return (pblk);          
        /* Return memory block to caller */
    } else {
     /*
         * Give the block memory semaphore. 
         */
        sal_sem_give(pTkOamBlkMem[pathId].BlkMemSem);
#ifdef BLOCK_MEM_DEBUG
        pTkOamBlkMem[pathId].MemGetFailed++;
#endif /* BLOCK_MEM_DEBUG */
        return (NULL);
    }
}


/*
 * TkOamMemPut - Returns a memory block to a partition 
 * 
 * Parameters:
 *
 * \pblk - a pointer to the memory block being released. 
 * 
 * \return: 
 *
 * OK - if the memory block was inserted into the partition 
 * ERROR -
 * returning a memory block to an already FULL memory partition or passed
 * a NULL 
 * pointer for 'pblk' 
 */
int
TkOamMemPut(uint8 pathId, void *pblk)
{
    if (NULL == pblk) {         /* Must release a valid block */
        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    /*
     * If the block memory can be operated? 
     */
    if (sal_sem_take(pTkOamBlkMem[pathId].BlkMemSem, sal_sem_FOREVER) < 0) {
        TkDbgInfoTrace(TkDbgErrorEnable,
                       ("Get block memory semaphore FAIL!"));
        return ERROR;
    }

    if (pTkOamBlkMem[pathId].MemNFree >= pTkOamBlkMem[pathId].MemNBlks) { 
        /* Make sure
              * all blocks
              * not already 
              * returned */
        /*
              * Give the block memory semaphore. 
              */
        sal_sem_give(pTkOamBlkMem[pathId].BlkMemSem);

        TkDbgTrace(TkDbgErrorEnable);
        return ERROR;
    }

    *(void **) pblk = pTkOamBlkMem[pathId].BlkMemFreeList; /* Insert released
                                                     * block into free
                                                     * block list */
    pTkOamBlkMem[pathId].BlkMemFreeList = pblk;
    pTkOamBlkMem[pathId].MemNFree++;   /* One more memory block in this partition 
                                 */
    /*
     * Give the block memory semaphore. 
     */
    sal_sem_give(pTkOamBlkMem[pathId].BlkMemSem);

    return (OK);
}


/*
 * TkOamMemStatus - Display the number of free memory blocks and the
 * number of 
 * used memory blocks from a memory partition. 
 * 
 * Parameters: 
 * 
 * no 
 * 
 * \return: 
 * OK or 
 * ERROR - if error 
 */
int
TkOamMemStatus(uint8 pathId)
{
    TkDbgPrintf(("BlkMemSize = %u\n", pTkOamBlkMem[pathId].BlkMemSize));
    TkDbgPrintf(("MemNBlks = %u\n", pTkOamBlkMem[pathId].MemNBlks));
    TkDbgPrintf(("MemNFree = %u\n", pTkOamBlkMem[pathId].MemNFree));
#ifdef BLOCK_MEM_DEBUG
    TkDbgPrintf(("MemGetFailed = 0x%08x\n", pTkOamBlkMem[pathId].MemGetFailed));
#endif /* BLOCK_MEM_DEBUG */

    return (OK);
}


/*
 * TkOamMemsFree - free the memory of one memory partition 
 * 
 * Parameters: 
 * 
 * no 
 * 
 * \return: 
 * no 
 */
void
TkOamMemsFree(uint8 pathId)
{
    if (pTkOamBlkMem[pathId].BlkMemAddr) {
        soc_cm_sfree(pathId, pTkOamBlkMem[pathId].BlkMemAddr);
    }
}
