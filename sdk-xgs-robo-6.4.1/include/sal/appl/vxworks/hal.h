/*
 * $Id: hal.h,v 1.6 Broadcom SDK $
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
 */

#ifndef __BCM_VXWORKS_HAL_H
#define __BCM_VXWORKS_HAL_H

typedef struct platform_hal_s {
    char                *name;

    unsigned int        flags;

    /* Platform Capablities */
#define PLATFORM_CAP_MULTICORE        0x000001
#define PLATFORM_CAP_ENDIAN_FUNNY     0x000002
#define PLATFORM_CAP_DMA_MEM_UNCACHABLE   0x000004
#define PLATFORM_CAP_FLASH_FS         0x000100 /* Flash filesystem implemented */
#define PLATFORM_CAP_WATCHDOG         0x000200
#define PLATFORM_CAP_PCI              0x010000
    unsigned int        caps;

    /* Platform BUS flags TBD */
    unsigned int        bus_caps;
    
    /* Dump Platform Info */
    int     (*f_dump_info)(void);

    /* Upgrade Images */
#define IMAGE_F_BOOT      0x01 /* Only boot images for now */
    int     (*f_upgrade_image)(char *imageName, unsigned int flags, 
                               int (*f_loader)(char *fname, char *fbuf, 
                                               int bufSize, int *entry_pt));

    /* Flash filesystem device name */
    char *  (*f_flash_device_name)(void);

#define FLASH_FORMAT_DOS        0x01
    int     (*f_format_fs)(int format, unsigned int flags);
    int     (*f_fs_sync)(void);     /* Sync Filesystem */
    int     (*f_fs_check)(void);    /* Check Filesystem */

    /* TOD (time of Day) Set interface */
    int     (*f_tod_set)(int year, int month, int day, 
                         int hour, int min, int sec);

    /* TOD (time of Day) Get interface */
    int     (*f_tod_get)(int *year, int *month, int *day,
                         int *hour, int *min, int *sec);

    /* WatchDog */
    int     (*f_start_wdog)(unsigned int usec);

    /* Reboot */
#define REBOOT_COLD         0
#define REBOOT_WARM         1
#define REBOOT_SHUT         2
    int     (*f_reboot)(int opt);

    
    
    
    /* Led functions */
    int     (*f_led_write_string)(const char *s);

    /* get timestamp value(number of ticks since boot) and frequency */
    int (*f_timestamp_get)(uint32 *up,     /* upper 32bit timestamp value */
                           uint32 *low,    /* lower 32bit timestamp value*/
                           uint32 *freq);  /* number of ticks per second */

    /* i2c operation using cpu i2c controller */
    int (*f_i2c_op)(int unit,       /* I2C controller number */
                    uint32 flags,   /* rd/wr options and other options */
                    uint16 slave,   /* slave address on the I2C bus */
                    uint32 addr,    /* internal address on slave */
                    uint8 addr_len, /* length of address in bytes */
                    uint8 *buf,     /* buffer to hold the read data */
                    uint8 buf_len); /* buffer length */
/* i2c operation flags */
#define HAL_I2C_WR              0x0 /* This is a bit flag and is the default. 
                                    * RD bit not set implies write 
                                    */
#define HAL_I2C_RD              0x1  
#define HAL_I2C_FAST_ACCESS     0x2
} platform_hal_t;

extern platform_hal_t *platform_info;

#define SAL_IS_PLATFORM_INFO_VALID     (platform_info != NULL)

int platform_attach(platform_hal_t **platform_info);

#endif /* __BCM_VXWORKS_HAL_H */

