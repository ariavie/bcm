/*
 * $Id: flashFsLib.h 1.4 Broadcom SDK $
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
 */

#ifndef	FLASH_FS_LIB_H
#define	FLASH_FS_LIB_H

#include "gto.h"

/*
 * Support for Flash Filesystem and Flash Boot Area
 * Built on top of flashDrvLib.
 */

STATUS flashFsLibInit(void);
STATUS flashFsSync(void);

extern int             flashBootSize;
extern int             flashFsSize;

extern int             flashFsSectorStart;
extern int             flash_boot_start;
extern int             flash_nvram_start;


#define FLASH_BOOT_START	   0x7f00000
#define FLASH_BOOT_START_SECTOR	   ((FLASH_BOOT_START) / (128 * 1024))
#define FLASH_BOOT_SIZE		   (1024 * 1024) 
#define FLASH_BOOT_SIZE_SECTORS	   8 

#define FLASH_NVRAM_SIZE	   0 
#define FLASH_NVRAM_START	   0x7f00000
#define FLASH_NVRAM_SECTOR_OFFSET  0x7f00000 
#define FLASH_NVRAM_START_SECTOR   ((FLASH_NVRAM_START) / (128 * 1024)) 
#define FLASH_NVRAM_SIZE_SECTORS   0 

#define FLASH_FS_BLOCK_SIZE	   512

#define FLASH_FS_SIZE		   (flashFsSize)
#define FLASH_FS_SIZE_BLOCKS	   ((FLASH_FS_SIZE) / (FLASH_FS_BLOCK_SIZE))
#define FLASH_FS_BLOCK_PER_SECTOR  ((128 * 1024) / (FLASH_FS_BLOCK_SIZE))

#define	FLASH_FS_NAME	"flash:"

typedef enum {

    FLASH_FS_SYNC = 0x10000

} FLASH_FS_CUSTOM_IOCTL_DEFINITIONS;

extern int sysIsFlashProm(void);
extern STATUS fsmNameInstall(char *driver, char *volume);
extern STATUS flashFsSync(void);
#endif /* FLASH_FS_LIB_H */
