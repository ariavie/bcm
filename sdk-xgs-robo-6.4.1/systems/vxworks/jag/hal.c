/*
 * $Id: hal.c,v 1.5 Broadcom SDK $
 *
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

#include <vxWorks.h>
#include <version.h>
#include <kernelLib.h>
#include <sysLib.h>
#include <stdlib.h>
#include <dosFsLib.h>

#include <sal/appl/sal.h>
#include <sal/appl/config.h>
#include <sal/appl/vxworks/hal.h>
#include "mbz.h"
#include "flashDrvLib.h"
#include "flashFsLib.h"
#include "srecLoad.h"

static int jag_upgrade_image(char *fname, unsigned int flags, 
                 int (*f_loader)(char *fname, char *fbuf, 
                                 int bufSize, int *entry_pt))
{
    char		*buf = 0;
    int			rv = -1;
    int			i = 0;
    int			entry;
    int                 bootImgSize = 512 * 1024;
#if !defined(NSSIOLE)
    char		tmpc;
#endif /* !NSSIOLE */

    if (flashDrvLibInit() == ERROR) {
        printf("flashBoot: Boot flash not found (jumpered right?)\n");
        goto done;
    }

    if ((buf = malloc(bootImgSize)) == 0) {
	printf("flashBoot: out of memory\n");
	goto done;
    }

    if (f_loader(fname, buf, bootImgSize, &entry) < 0) {
	printf("flashBoot: Failed to read image.\n");
	goto done;
    }

#if !defined(NSSIOLE)
    for(i = 0; i < bootImgSize; i += 4) {
        tmpc = buf[i];
        buf[i] = buf[i + 3];
        buf[i + 3] = tmpc;

        tmpc = buf[i + 1];
        buf[i + 1] = buf[i + 2];
        buf[i + 2] = tmpc;
    }
#endif

    printf("%d\nErasing boot area ...", i);

    for (i = FLASH_BOOT_START_SECTOR;
         i < FLASH_BOOT_START_SECTOR + FLASH_BOOT_SIZE_SECTORS; i++) {

        if (flashEraseBank(i, 1) != OK) {
	    printf("\nflashBoot: failed erasing -- PROM DESTROYED\n");
            goto done;
        }

        printf(".");
    }

    printf("done\nWriting boot data ...");

    if (flashBlkWrite(FLASH_BOOT_START_SECTOR, buf,
		      0, bootImgSize) != OK) {
        printf("\nflashBoot: failed writing -- PROM DESTROYED\n");
        goto done;
    }

    printf("\nDone\n");

    rv = 0;

 done:

    if (buf) {
	free(buf);
    }

    return rv;
}

static int jag_print_info(void)
{
    int		core, sb,pci;

    sys47xxClocks(&core, &sb, &pci);

    if (core) {
        printf("CPU: %d MHz, MEM: %d MHz, ", core, core);
    } else {
        printf("CPU: Unknown Mhz, MEM: Unknown Mhz, ");
    }

    if (sb) {
        printf("SB: %d MHz, ", sb);
    } else {
        printf("SB: Unknown MHz, ");
    }

    if (pci) {
        printf("PCI: %d MHz\n", pci);
    } else {
        printf("PCI: Unknown MHz\n");
    }
    return 0;
}


static char * jag_flash_dev_name(void)
{
    return FLASH_FS_NAME;
}

static int jag_format_flash(int format, unsigned int flags)
{
    int	rv = 0;

#if !defined(DOC_IS_FLASH)
    if (format) {
# ifdef DOS_OPT_DEFAULT
	if (dosFsVolFormat(FLASH_FS_NAME, DOS_OPT_DEFAULT, 0)) {
	    rv |= -1;
	}
# else
	if (diskInit(FLASH_FS_NAME)) {
	    rv |= -1;
	}
# endif
        flashFsSync();
    }
#else
    if (format) {
	extern STATUS tffsBCM47xxFormat(void);
	tffsBCM47xxFormat();
    }
#endif /* DOC_IS_FLASH */

    return(rv);
}

static int jag_led_write_string(const char * s)
{
    sysLedDsply(s);
    return 0;
}

static platform_hal_t jag_hal_info = {
    "JAG",                                         /* name */
    0,                                             /* flags */
    (PLATFORM_CAP_DMA_MEM_UNCACHABLE |
     PLATFORM_CAP_PCI | PLATFORM_CAP_FLASH_FS),    /* caps */
    0,                                             /* bus_caps */
    jag_print_info,                                /* f_dump_info */
    jag_upgrade_image,                             /* f_upgrade_image */
    jag_flash_dev_name,                            /* f_flash_device_name */
    jag_format_flash,                              /* f_format_fs */
    flashFsSync,                                   /* f_fs_sync */
    NULL,                                          /* f_fs_check */
    sysTodSet,                                     /* f_tod_set */
    sysTodGet,                                     /* f_tod_get */
    NULL,                                          /* f_start_wdog */
    NULL,                                          /* f_reboot */
    jag_led_write_string                           /* f_led_write_string */
};

int platform_attach(platform_hal_t **platform_info)
{
    if (!platform_info) {
        return -1;
    }

    *platform_info = &jag_hal_info;
    return 0;
}

