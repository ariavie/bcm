/*
 * $Id: flashDrvLib.c 1.4 Broadcom SDK $
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
 * File:    flashDrvLib.c
 */

#include "vxWorks.h"
#include "taskLib.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include "config.h"
#include "flashDrvLib.h"

#include "osl.h"
#include "siutils.h"
#include "hndsoc.h"
#include "sflash.h"

#define TOTAL_LOADED_SECS  3

int             flashVerbose = 0; /* DEBUG */
unsigned int    flashBaseAddress = 0;
int             flashSize = 0x200000;
int             flashDevSectorSize = 0x20000;
int             flashSectorCount = 0;
LOCAL struct flash_drv_funcs_s *flashDrvFuncs = &flashs29gl256p;

LOCAL struct flashLoadedSectorInfo {
    SEM_ID fsSemID;
    int   sector;
    int   dirty;
    char *buffer;
} flashLoadedSectors[TOTAL_LOADED_SECS];
LOCAL int flashDrvInitialized = 0;

#define FS_CACHE_LOCK(_x_) \
    semTake(flashLoadedSectors[(_x_)].fsSemID, WAIT_FOREVER)
#define FS_CACHE_UNLOCK(_x_) \
    semGive(flashLoadedSectors[(_x_)].fsSemID)

/* Local vars */
static si_t *sih = NULL;
static chipcregs_t *cc = NULL;

LOCAL int
flashFlushLoadedSector(int number)
{
    if (flashLoadedSectors[number].sector < 0 ||
        !flashLoadedSectors[number].dirty) {
        if (flashVerbose)
            printf("flashFlushLoadedSector(%d): not dirty\n", number);
        return (OK);
    }

    if (flashVerbose) {
        printf("flashFlushLoadedSector(%d): Flushing %d\n", number,
                flashLoadedSectors[number].sector);
    }

    if (flashDrvFuncs->flashEraseSector(flashLoadedSectors[number].sector)==ERROR) {
        return (ERROR);
    }

    if (flashDrvFuncs->flashWrite(flashLoadedSectors[number].sector,
            flashLoadedSectors[number].buffer, 0, FLASH_SECTOR_SIZE) == ERROR) {
        return (ERROR);
    }

    if (flashVerbose) {
        printf("                           Flushing %d done\n",
                flashLoadedSectors[number].sector);
    }

    flashLoadedSectors[number].sector = -1;
    flashLoadedSectors[number].dirty = 0;

    return (OK);
}

int
flashDrvLibInit(void)
{
    FLASH_TYPES     dev;
    FLASH_VENDORS   vendor;
    int i;
    uint32 fltype = PFLASH;
    osl_t *osh;
    struct sflash *sflash;
    
    if (flashDrvInitialized)
	    return (OK);

    flashBaseAddress = FLASH_BASE_ADDRESS_ALIAS;

    /*
     * Check for serial flash.
     */
    sih = si_kattach(SI_OSH);
    ASSERT(sih);
    
    osh = si_osh(sih);
    
    cc = (chipcregs_t *)si_setcoreidx(sih, SI_CC_IDX);
    ASSERT(cc);

    /* Select SFLASH */
    fltype = R_REG(osh, &cc->capabilities) & CC_CAP_FLASH_MASK;
    if (fltype == SFLASH_ST || fltype == SFLASH_AT) {
        sflash = sflash_init(sih, cc);

        if (sflash == NULL) {
            printf("flashInit(): Unrecognized Device (SFLASH)\n");
            return (ERROR);
        }
        flashDrvFuncs = &flashsflash;
        flashDrvFuncs->flashAutoSelect(&dev, &vendor);

        flashSectorCount = sflash->numblocks;
        flashDevSectorSize = sflash->blocksize;

        if (flashVerbose) {
            printf("flashInit(): SFLASH Found\n");
        }
    } else {
        flashDrvFuncs = &flashs29gl256p;
        flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        if ((vendor == 0xFF) && (dev == 0xFF)) {
            flashDrvFuncs = &flash29gl128;
            flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        }
    
        if ((vendor == 0xFF) && (dev == 0xFF)) {
            flashDrvFuncs = &flash29l640;
            flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        }
        if ((vendor == 0xFF) && (dev == 0xFF)) {
            flashDrvFuncs = &flash29l320;
            flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        }
        if ((vendor == 0xFF) && (dev == 0xFF)) {
            flashDrvFuncs = &flash29l160;
            flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        }
        if ((vendor == 0xFF) && (dev == 0xFF)) {
            flashDrvFuncs = &flash28f320;
            flashDrvFuncs->flashAutoSelect(&dev, &vendor);
        }
    
        switch (vendor) {
            case AMD:
            case ALLIANCE:
            case MXIC:
            switch (dev) {
                case FLASH_2F040:
                    flashSectorCount = 8;
                    flashDevSectorSize = 0x10000;
                    if (flashVerbose)
                        printf("flashInit(): 2F040 Found\n");
                    break;
                case FLASH_2F080:
                    flashSectorCount = 16;
                    flashDevSectorSize = 0x10000;
                    if (flashVerbose)
                        printf("flashInit(): 2F080 Found\n");
                    break;
    
                case FLASH_2L081:
                    flashSectorCount = 16;
                    flashDevSectorSize = 0x10000;
                    if (flashVerbose)
                        printf("flashInit(): 29LV081B Found\n");
                    break;
    
                case FLASH_2L160:
                case FLASH_2L017:
                    flashSectorCount = 32;
                    flashDevSectorSize = 0x10000;
                    if (flashVerbose)
                        printf("flashInit(): 29LV160D Found\n");
                    break;
    
                case FLASH_2L640:
                case FLASH_MX2L640:
                    flashSectorCount = 128;
                    flashDevSectorSize = 0x10000;
                    if (flashVerbose)
                        printf("flashInit(): 29LV640M Found\n");
                    break;
    
                case FLASH_29GL128:
                    /* Spansion 29GL128 is physically 128 sector count
                     * To make flash support backward compatible to old device,
                     * only use 64 for 8MB */
                    flashSectorCount = 64;
                    flashDevSectorSize = 0x20000;
                    if (flashVerbose)
                        printf("flashInit(): 29GL128 Found\n");
                    break;
    
                case FLASH_2L320:
                    flashSectorCount = 64;
                    flashDevSectorSize = 0x10000;
                    if (flashVerbose)
                        printf("flashInit(): 29LV320D Found\n");
                    break;
    
                case FLASH_S29GL128P:
                    flashSectorCount = 128;
                    flashDevSectorSize = 0x20000;
                    if (flashVerbose)
                        printf("flashInit(): FLASH_S29GL128P Found\n");
                    break;
    
                case FLASH_S29GL256P:
                    flashSectorCount = 256;
                    flashDevSectorSize = 0x20000;
                    if (flashVerbose)
                        printf("flashInit(): S29GL256P Found\n");
                    break;
    
                case FLASH_S29GL512P:
                    flashSectorCount = 512;
                    flashDevSectorSize = 0x20000;
                    if (flashVerbose)
                        printf("flashInit(): FLASH_S29GL512P Found\n");
                    break;
                    
                case FLASH_S29GL01GP:
                    flashSectorCount = 1024;
                    flashDevSectorSize = 0x20000;
                    if (flashVerbose)
                        printf("flashInit(): FLASH_S29GL01GP Found\n");
                    break;
    
                default:
                    printf("flashInit(): Unrecognized Device (0x%02X)\n", dev);
                    return (ERROR);
            }
            break;
    
            case INTEL:
            switch (dev) {
                case FLASH_2F320:
                    flashSectorCount = 32;
                    flashDevSectorSize = 0x20000;
                    if (flashVerbose)
                        printf("flashInit(): 28F320 Found\n");
                    break;
    
                default:
                    printf("flashInit(): Unrecognized Device (0x%02X)\n", dev);
                    return (ERROR);
            }
            break;
            default:
                printf("flashInit(): Unrecognized Vendor (0x%02X)\n", vendor);
                return (ERROR);
        }
    }
    flashSize = flashDevSectorSize * flashSectorCount;

    for (i = 0; i < TOTAL_LOADED_SECS; i++) {
        flashLoadedSectors[i].buffer = malloc(FLASH_SECTOR_SIZE);
        if (flashLoadedSectors[i].buffer == NULL) {
            printf("flashInit(): malloc() failed\n");
            for (; i > 0; i--) {
                free(flashLoadedSectors[i-1].buffer);
            }
            return (ERROR);
        }
        flashLoadedSectors[i].sector = -1;
        flashLoadedSectors[i].dirty = 0;
        flashLoadedSectors[i].fsSemID =
            semMCreate (SEM_Q_PRIORITY | SEM_DELETE_SAFE);
    }
    flashDrvInitialized ++;

    return (OK);
}

int
flashGetSectorCount(void)
{
    return (flashSectorCount);
}

int
flashEraseBank(int firstSector, int nSectors)
{
    int             sectorNum, errCnt = 0;

    if (firstSector < 0 || firstSector + nSectors > flashSectorCount) {
        printf("flashEraseBank(): Illegal parms %d, %d\n",
           firstSector, nSectors);
        return ERROR;
    }

    for (sectorNum = firstSector;
         sectorNum < firstSector + nSectors; sectorNum++) {
         printf(".");

        if (flashDrvFuncs->flashEraseSector(sectorNum))
            errCnt++;
    }

    printf("\n");

    if (errCnt)
        return (ERROR);
    else
        return (OK);
}

int
flashBlkRead(int sectorNum, char *buff,
         unsigned int offset, unsigned int count)
{
    int i;

    if (sectorNum < 0 || sectorNum >= flashSectorCount) {
        printf("flashBlkRead(): Sector %d invalid\n", sectorNum);
        return (ERROR);
    }

    if (offset < 0 || offset >= FLASH_SECTOR_SIZE) {
        printf("flashBlkRead(): Offset 0x%x invalid\n", offset);
        return (ERROR);
    }

    if (count < 0 || count > FLASH_SECTOR_SIZE - offset) {
        printf("flashBlkRead(): Count 0x%x invalid\n", count);
        return (ERROR);
    }

    /*
     * If the sector is loaded, read from it.  Else, read from the
     * flash itself (slower).
     */
    for (i = 0; i < TOTAL_LOADED_SECS; i++) {
        if (flashLoadedSectors[i].sector == sectorNum) {
            if (flashVerbose)
                printf("flashBlkRead(): from loaded sector %d\n", sectorNum);
            bcopy(&flashLoadedSectors[i].buffer[offset], buff, count);
            return (OK);
        }
    }

    flashDrvFuncs->flashRead(sectorNum, buff, offset, count);

    return (OK);
}

LOCAL int
flashCheckCanProgram(int sectorNum, unsigned int offset, unsigned int count)
{
    unsigned char   *flashBuffPtr;
    int             i;

    flashBuffPtr = (unsigned char *)(FLASH_SECTOR_ADDRESS(sectorNum) + offset);

    for (i = 0; i < count; i++) {
        if (flashBuffPtr[i] != 0xff)
            return (ERROR);
    }

    return (OK);
}

int
flashBlkWrite(int sectorNum, char *buff,
          unsigned int offset, unsigned int count)
{
    char *save;
    int i;

    if (sectorNum < 0 || sectorNum >= flashSectorCount) {
        printf("flashBlkWrite(): Sector %d invalid\n", sectorNum);
        return (ERROR);
    }

    if (offset < 0 || offset >= FLASH_SECTOR_SIZE) {
        printf("flashBlkWrite(): Offset 0x%x invalid\n", offset);
        return (ERROR);
    }

    /* 
     * Count must be within range and must be a long word multiple, as
     * we always program long words.
     */

    if ((count < 0) || 
        (count > ((flashSectorCount - sectorNum) * FLASH_SECTOR_SIZE - offset))) {
        printf("flashBlkWrite(): Count 0x%x invalid\n", count);
        return (ERROR);
    }

    /*
     * If the Sector is loaded, write it to buffer.  Else check to see
     * if we can program the sector; if so, program it.  Else, flush the
     * first loaded sector (if loaded and dirty), push loaded sectors
     * up by one, load the new one and copy the data into the last one.
     */

    for (i = 0; i < TOTAL_LOADED_SECS; i++) {
        FS_CACHE_LOCK(i);
        if (flashLoadedSectors[i].sector == sectorNum) {
            if (flashVerbose)
                printf("%d ", sectorNum);
            bcopy(buff, &flashLoadedSectors[i].buffer[offset], count);
            flashLoadedSectors[i].dirty = 1;
            FS_CACHE_UNLOCK(i);
            return (OK);
        }
        FS_CACHE_UNLOCK(i);
    }

    if (flashCheckCanProgram(sectorNum, offset, count) != ERROR) {
        return (flashDrvFuncs->flashWrite(sectorNum, buff, offset, count));
    }

    FS_CACHE_LOCK(0);
    if (flashFlushLoadedSector(0) == ERROR) {
        FS_CACHE_UNLOCK(0);
        return (ERROR);
    }
    FS_CACHE_UNLOCK(0);

    save = flashLoadedSectors[0].buffer;
    for (i = 1; i < TOTAL_LOADED_SECS; i++) {
        FS_CACHE_LOCK(i-1);
        flashLoadedSectors[i-1].sector = flashLoadedSectors[i].sector;
        flashLoadedSectors[i-1].dirty  = flashLoadedSectors[i].dirty;
        flashLoadedSectors[i-1].buffer = flashLoadedSectors[i].buffer;
        FS_CACHE_UNLOCK(i-1);
    }
    i--;
    flashLoadedSectors[i].buffer = save;

    FS_CACHE_LOCK(i);
    if (flashDrvFuncs->flashRead(sectorNum, flashLoadedSectors[i].buffer,
                                 0, FLASH_SECTOR_SIZE) == ERROR) {
        flashLoadedSectors[i].sector = -1;
        FS_CACHE_UNLOCK(i);
        return (ERROR);
    }

    flashLoadedSectors[i].sector = sectorNum;
    bcopy(buff, &flashLoadedSectors[i].buffer[offset], count);
    flashLoadedSectors[i].dirty = 1;
    FS_CACHE_UNLOCK(i);

    if (flashVerbose) {
        printf("flashBlkWrite(): load %d (and write to cache only)\n", sectorNum);
        printf("%d ", sectorNum);
    }

    return (OK);
}

int
flashDiagnostic(void)
{
    unsigned int   *flashSectorBuff;
    int             sectorNum, i;

    /*
     * Probe flash; allocate flashLoadedSector Buffer
     */

    flashDrvLibInit();        /* Probe; clear loaded sector */

    flashSectorBuff = (unsigned int *) flashLoadedSectors[0].buffer;

    if (flashVerbose)
        printf("flashDiagnostic(): Executing. Erasing %d Sectors\n",
                flashSectorCount);

    if (flashEraseBank(0, flashSectorCount) == ERROR) {
    if (flashVerbose)
        printf("flashDiagnostic(): flashEraseBank() #1 failed\n");

    return (ERROR);
    }

    /* Write unique counting pattern to each sector. */
    for (sectorNum = 0; sectorNum < flashSectorCount; sectorNum++) {
    if (flashVerbose)
        printf("flashDiagnostic(): writing sector %d\n", sectorNum);

    for (i = 0; i < FLASH_SECTOR_SIZE / sizeof (unsigned int); i++)
        flashSectorBuff[i] = (i + sectorNum);

    if (flashDrvFuncs->flashWrite(sectorNum, (char *)flashSectorBuff,
               0, FLASH_SECTOR_SIZE) == ERROR) {
        if (flashVerbose)
        printf("flashDiagnostic(): flashWrite() failed on %d\n",
               sectorNum);

        return (ERROR);
    }
    }

    /* Verify each sector. */
    for (sectorNum = 0; sectorNum < flashSectorCount; sectorNum++) {
    if (flashVerbose)
        printf("flashDiagnostic(): verifying sector %d\n", sectorNum);

    if (flashDrvFuncs->flashRead(sectorNum, (char *)flashSectorBuff,
              0, FLASH_SECTOR_SIZE) == ERROR) {
        if (flashVerbose)
        printf("flashDiagnostic(): flashRead() failed on %d\n",
               sectorNum);

        return (ERROR);
    }

    for (i = 0; i < FLASH_SECTOR_SIZE / sizeof (unsigned int); i++) {
        if (flashSectorBuff[i] != (i + sectorNum)) {
        if (flashVerbose) {
            printf("flashDiagnostic(): verification failed\n");
            printf("flashDiagnostic(): sector %d, offset 0x%x\n",
               sectorNum, (i * sizeof(unsigned int)));
            printf("flashDiagnostic(): expected 0x%x, got 0x%x\n",
               (i + sectorNum), (int)flashSectorBuff[i]);
        }

        return (ERROR);
        }
    }
    }

    if (flashEraseBank(0, flashSectorCount) == ERROR) {
    if (flashVerbose)
        printf("flashDiagnostic(): flashEraseBank() #2 failed\n");

        return (ERROR);
    }

    if (flashVerbose)
        printf("flashDiagnostic(): Completed without error\n");

    return (OK);
}

int
flashSyncFilesystem(void)
{
    int i;

    for (i = 0; i < TOTAL_LOADED_SECS; i++) {
        FS_CACHE_LOCK(i);
        if (flashFlushLoadedSector(i) != OK) {
            FS_CACHE_UNLOCK(i);
            return (ERROR);
        }
        FS_CACHE_UNLOCK(i);
    }

    return (OK);
}

