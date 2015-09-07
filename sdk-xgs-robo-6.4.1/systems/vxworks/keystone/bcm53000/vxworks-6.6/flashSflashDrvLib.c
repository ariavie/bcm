/*
 * $Id: flashSflashDrvLib.c,v 1.1 Broadcom SDK $
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
 * File:    flashSflashDrvLib.c
 */
#include "flashDrvLib.h"
#include "osl.h"
#include "siutils.h"
#include "hndsoc.h"
#include "sflash.h"

/* Local vars */
static si_t *sih = NULL;
static chipcregs_t *cc = NULL;

extern struct flash_drv_funcs_s flashsflash;

LOCAL void
flashAutoSelect(FLASH_TYPES *dev, FLASH_VENDORS *vendor)
{
    /*
     * Check for serial flash.
     */
    sih = si_kattach(SI_OSH);
    ASSERT(sih);
    
    cc = (chipcregs_t *)si_setcoreidx(sih, SI_CC_IDX);
    ASSERT(cc);

    *vendor = *dev = 0xFF;
}

LOCAL int
flashEraseDevices(volatile unsigned char *sectorBasePtr)
{
    int ret = 0;

    if (flashVerbose) {
        printf("Erasing Sector @ 0x%08x\n",(unsigned int)sectorBasePtr);
    }

    /* Erase block */
    if ((ret = sflash_erase(sih, cc, (unsigned int)sectorBasePtr)) < 0) {
        return (ERROR);
    }

    /* Polling until command completion. Returns zero when complete. */
    while (sflash_poll(sih, cc, (unsigned int)sectorBasePtr));

    if (flashVerbose > 1)
        printf("flashEraseDevices(): all devices erased\n");

    return (OK);
}

LOCAL int
flashEraseSector(int sectorNum)
{
    unsigned char *sectorBasePtr = (unsigned char *)(sectorNum * FLASH_SECTOR_SIZE);

    if (sectorNum < 0 || sectorNum >= flashSectorCount) {
        printf("flashEraseSector(): Sector %d invalid\n", sectorNum);
        return (ERROR);
    }

    if (flashEraseDevices(sectorBasePtr) == ERROR) {
        printf("flashEraseSector(): erase devices failed sector=%d\n",
            sectorNum);
        return (ERROR);
    }

    if (flashVerbose)
        printf("flashEraseSector(): Sector %d erased\n", sectorNum);

    return (OK);
}

LOCAL int
flashRead(int sectorNum, char *buff, unsigned int offset, unsigned int count)
{
    unsigned char *curBuffPtr, *sectorOffsetPtr;
    unsigned int len = count;
    int bytes;

    if (sectorNum < 0 || sectorNum >= flashSectorCount) {
        printf("flashRead(): Illegal sector %d\n", sectorNum);
        return (ERROR);
    }

    curBuffPtr = (unsigned char *)buff;
    sectorOffsetPtr = (unsigned char *)((sectorNum * FLASH_SECTOR_SIZE) + offset);

    /* Read holding block */
    while (len) {
        if ((bytes = sflash_read(sih, cc, (unsigned int)sectorOffsetPtr, len, curBuffPtr)) < 0) {
            printf("flashRead(): Failed: Sector %d, address 0x%x\n",
                sectorNum, (int)sectorOffsetPtr);
            return (ERROR);
        }
        sectorOffsetPtr += bytes;
        len -= bytes;
        curBuffPtr += bytes;
    }

    return (OK);
}

LOCAL int
flashWrite(int sectorNum, char *buff, unsigned int offset, unsigned int count)
{
    unsigned char *curBuffPtr, *sectorOffsetPtr;
    unsigned int len = count;
    int bytes;

    curBuffPtr = (unsigned char *)buff;
    sectorOffsetPtr = (unsigned char *)((sectorNum * FLASH_SECTOR_SIZE) + offset);

    /* Write holding block */
    while (len) {
        if ((bytes = sflash_write(sih, cc, (unsigned int)sectorOffsetPtr, len, curBuffPtr)) < 0) {
            printf("flashWrite(): Failed: Sector %d, address 0x%x\n",
                sectorNum, (int)sectorOffsetPtr);
            return (ERROR);
        }

        /* Polling until command completion. Returns zero when complete. */
        while (sflash_poll(sih, cc, (unsigned int)sectorOffsetPtr));

        sectorOffsetPtr += bytes;
        len -= bytes;
        curBuffPtr += bytes;
    }

    return (OK);
}

struct flash_drv_funcs_s flashsflash = {
    (FLASH_TYPES)0, (FLASH_VENDORS)0,
    flashAutoSelect,
    flashEraseSector,
    flashRead,
    flashWrite
};

