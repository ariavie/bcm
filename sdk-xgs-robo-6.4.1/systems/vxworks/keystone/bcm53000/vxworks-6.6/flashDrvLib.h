/*
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
 * $Id: flashDrvLib.h,v 1.3 Broadcom SDK $
 */
 
#ifndef	FLASH_DRV_LIB_H
#define	FLASH_DRV_LIB_H

#define	FLASH_BASE_ADDRESS_PLCC_BOOT    0xBFC00000
#define FLASH_BASE_ADDRESS_FLASH_BOOT   0xBFC00000
#define FLASH_BASE_ADDRESS_ALIAS        0xBC000000

#define	FLASH_DEVICE_COUNT              1
#define FLASH_DEVICE_SECTOR_SIZE        flashDevSectorSize

#define FLASH_SECTOR_SIZE (FLASH_DEVICE_COUNT * FLASH_DEVICE_SECTOR_SIZE)

#define FLASH_SIZE                  flashSize
#define FLASH_SIZE_SECTORS          (FLASH_SIZE / FLASH_SECTOR_SIZE)

#define FLASH_BASE_ADDRESS	flashBaseAddress
#define FLASH_SECTOR_ADDRESS(sector) \
    (FLASH_BASE_ADDRESS + (sector) * FLASH_SECTOR_SIZE)

#define	FLASH_ERASE_TIMEOUT_COUNT	750
#define	FLASH_ERASE_TIMEOUT_TICKS	2

#define	FLASH_PROGRAM_TIMEOUT_POLLS	100000

/*
 * AMD Flash commands and magic offsets
 */

#define AMD_FLASH_MAGIC_ADDR16_1  0x555 /* 555 for 16-bit devices in 16-bit mode */
#define AMD_FLASH_MAGIC_ADDR16_2  0x2AA /* 2AA for 16-bit devices in 16-bit mode */
#define AMD_FLASH_MAGIC_ADDR8_1  0xAAA /* AAA for 16-bit devices in 8-bit mode */
#define AMD_FLASH_MAGIC_ADDR8_2  0x555 /* 555 for 16-bit devices in 8-bit mode */

#define AMD_FLASH_RESET  0xF0
#define AMD_FLASH_MAGIC_1	 0xAA
#define AMD_FLASH_MAGIC_2	 0x55
#define AMD_FLASH_AUTOSEL  0x90
#define AMD_FLASH_DEVCODE8  0x1
#define AMD_FLASH_DEVCODE16  0x1
#define AMD_FLASH_DEVCODE16B  0x2
#define AMD_FLASH_MANID  0x0
#define AMD_FLASH_PROGRAM	 0xA0
#define AMD_FLASH_UNLOCK_BYPASS  0x20
#define AMD_FLASH_ERASE_3	 0x80
#define AMD_FLASH_ERASE_4	 0xAA
#define AMD_FLASH_ERASE_5	 0x55
#define AMD_FLASH_ERASE_ALL_6  0x10
#define AMD_FLASH_ERASE_SEC_6  0x30
#define AMD_FLASH_WRITE_BUFFER  0x25
#define AMD_FLASH_WRITE_CONFIRM  0x29

typedef enum {
    AMD = 0x01,
    ALLIANCE = 0x52,
    INTEL = 0x89,
    MXIC = 0xc2
} FLASH_VENDORS;

typedef enum {
    FLASH_2F320 = 0x16,
    FLASH_2F040 = 0xA4,
    FLASH_2F080 = 0xD5,
    FLASH_2L081 = 0x38,
    FLASH_2L160 = 0x49,
    FLASH_2L017 = 0xC8,
    FLASH_2L640 = 0x7E,
    FLASH_2L320 = 0xF9,
    FLASH_MX2L640 = 0xcb,
    FLASH_29GL128 = 0x227E,
    FLASH_S29GL128P = 0x2101,
    FLASH_S29GL256P = 0x2201,
    FLASH_S29GL512P = 0x2301,
    FLASH_S29GL01GP = 0x2801
} FLASH_TYPES;

struct flash_drv_funcs_s {
    FLASH_TYPES dev;
    FLASH_VENDORS vendor;
    void (*flashAutoSelect)(FLASH_TYPES *dev, FLASH_VENDORS *vendor);
    int (*flashEraseSector)(int sectorNum);
    int (*flashRead)(int sectorNum, char *buff,
          unsigned int offset, unsigned int count);
    int (*flashWrite)(int sectorNum, char *buff,
                unsigned int offset, unsigned int count);
    int (*flashFlushLoadedSector)(void);
};

extern int             flashVerbose; /* DEBUG */
extern int             flashSectorCount;
extern int             flashDevSectorSize;
extern int             flashSize;
extern unsigned int    flashBaseAddress;
extern struct flash_drv_funcs_s flash28f320;
extern struct flash_drv_funcs_s flash29l160;
extern struct flash_drv_funcs_s flash29l640;
extern struct flash_drv_funcs_s flash29l320;
extern struct flash_drv_funcs_s flash29gl128;
extern struct flash_drv_funcs_s flashs29gl256p;
extern struct flash_drv_funcs_s flashsflash;

int             flashDrvLibInit(void);
int             flashGetSectorCount(void);
int             flashEraseBank(int firstSector, int nSectors);
int             flashBlkRead(int sectorNum, char *buff,
                unsigned int offset, unsigned int count);
int             flashBlkWrite(int sectorNum, char *buff,
                unsigned int offset, unsigned int count);
int             flashSyncFilesystem(void);
int             flashDiagnostic(void);

#endif /* FLASH_DRV_LIB_H */

