/*
 * $Id: spiflash.c 1.8 Broadcom SDK $
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
 * Arm Processor Subsystem flash utility
 */

#include <sal/core/libc.h>
#include <appl/diag/aps/spiflash.h>

#define ROUND_UP(x,y)           ((((uint32)(x)) + ((uint32)(y)) - 1) \
                                 & ~(uint32)((y) - 1))

flash_info_t spiflash_info[] = {
    { "m25p10a",  0x202011,    256,  64*1024, 32*1024,  128*1024 },
    { "m25p20",   0x202012,    256,  64*1024, 64*1024,  256*1024 },
    { "m25p40",   0x202013,    256,  64*1024, 64*1024,  512*1024 },
    { "m25p80",   0x202014,    256,  64*1024, 64*1024, 1024*1024 },
    { "m25p16",   0x202015,    256,  64*1024, 64*1024, 2048*1024 },

    { "s25fl032p", 0x010215,   256,  64*1024, 64*1024, 4096*1024 },

    { "m45pe10",  0x204011,    256,      256, 64*1024,  128*1024 },
    { "m45pe20",  0x204012,    256,      256, 64*1024,  256*1024 },
    { "m45pe40",  0x204013,    256,      256, 64*1024,  512*1024 },
    { "m45pe80",  0x204014,    256,      256, 64*1024, 1024*1024 },
    { "m45pe16",  0x204015,    256,      256, 64*1024, 2048*1024 },

    { "mx25L0805d/6e", 0xc22014,  256,  4*1024, 64*1024, 1024*1024 },
    { "mx25L1605d/6e", 0xc22015,  256,  4*1024, 64*1024, 2048*1024 },
    { "mx25L3205d/6e", 0xc22016,  256,  4*1024, 64*1024, 4096*1024 },
    { "mx25L6405d/6e", 0xc22017,  256,  4*1024, 64*1024, 8192*1024 },

    { "w25x40bv", 0xef3013,    256,  4*1024, 64*1024,  512*1024 },
    
    { "at45db011d", 0x1f2200,    256,    256, 2*1024, 128*1024 },
    { "at45db021d", 0x1f2300,    256,    256, 2*1024, 256*1024 },
    { "at45db041d", 0x1f2400,    256,    256, 2*1024, 512*1024 },
    { "at45db081d", 0x1f2500,    256,    256, 2*1024, 1024*1024 }
};

/* @api
 * spiflash_info_by_name
 *
 * @brief
 * Lookup the SPI flash info based on it's name.
 *
 * @param=name - name of the flash
 *
 * @returns pointer to the flash_info_t or NULL
 */
flash_info_t *spiflash_info_by_name(char *name)
{
    int i;

    /* determine the minimum erase size for this flash */
    for (i = 0; i < sizeof(spiflash_info)/sizeof(spiflash_info[0]); ++i) {
        if (!sal_strcmp(name, spiflash_info[i].name)) {
            return &spiflash_info[i];
        }
    }
    return NULL;
}

/* @api
 * spiflash_info_by_rdid
 *
 * @brief
 * Lookup the SPI flash info based on it's RDID.
 *
 * @param=rdid - RDID for the flash
 *
 * @returns pointer to the flash_info_t or NULL
 */
flash_info_t *spiflash_info_by_rdid(uint32 rdid)
{
    int i;

    /* determine the minimum erase size for this flash */
    for (i = 0; i < sizeof(spiflash_info)/sizeof(spiflash_info[0]); ++i) {
        if (spiflash_info[i].rdid == rdid) {
            return &spiflash_info[i];
        }
    }
    return NULL;
}

/*@api
 * spiflash_calculate_flash_header
 *
 * @brief
 * Calculates the flash header based on the input parameters
 *
 * @param=flash_info - pointer to the flash_info_t for the flash
 * @param=header - pointer to the flash_header_t to fill in
 * @param=dmu_config - DMU configuration option to choose
 * @param=spi_speed - SPI speed to install (0 = no speed change)
 * @returns 0 - success
 * @returns !0 - error
 *
 * @desc
 */

int spiflash_calculate_header(flash_info_t *flash_info,
                              flash_header_t *header,
                              uint32 dmu_config,
                              uint32 spi_speed)
{
    int i;
    uint32 page;
    uint32 arm_pages;
    uint32 switch_pages;
    uint32 core_dump_pages;
    uint32 arm_image_size;
    uint32 switch_image_size;
    uint32 core_dump_size;
    uint32 avb_config_size;
    uint32 avb_config_pages;

    /* Calculate the avb config size */
    avb_config_size = ROUND_UP(FLASH_AVB_CONFIG_SIZE, SPIFLASH_PAGE_SIZE);
     /* allocate a page for the image header */
    avb_config_size += SPIFLASH_PAGE_SIZE;
    avb_config_pages = avb_config_size / SPIFLASH_PAGE_SIZE;

    /* Calculate the core dump size */
    core_dump_size = ROUND_UP(FLASH_CORE_IMAGE_SIZE, SPIFLASH_PAGE_SIZE);
     /* allocate a page for the image header */
    core_dump_size += SPIFLASH_PAGE_SIZE;
    core_dump_pages = core_dump_size / SPIFLASH_PAGE_SIZE;

    /* Calculate the ARM and switch pages based on the page size */
    arm_image_size = ROUND_UP(FLASH_ARM_IMAGE_SIZE, SPIFLASH_PAGE_SIZE);
    /* allocate a page for the image header */
    arm_image_size += SPIFLASH_PAGE_SIZE;
    arm_pages = arm_image_size / SPIFLASH_PAGE_SIZE;

    switch_image_size = ROUND_UP(FLASH_SWITCH_IMAGE_SIZE, SPIFLASH_PAGE_SIZE);
    /* allocate a page for the switch configuration header */
    switch_image_size += SPIFLASH_PAGE_SIZE;
    switch_pages = switch_image_size / SPIFLASH_PAGE_SIZE;

    sal_memset(header, 0, sizeof(flash_header_t));
	header->magic = FLASH_HEADER_MAGIC;

	/* Program the DMU configuration */
	header->dmu_config = dmu_config;

	/* Program the default spi-speed */
	header->spi_speed = spi_speed;

    /* build the offsets in pages, starting after the TOC copies */
    page = (FLASH_NUM_HEADER_BLOCKS * flash_info->erase_page_size) / SPIFLASH_PAGE_SIZE;
    for (i = 0; i < FLASH_NUM_IMAGES; ++i) {
        header->image_offset[i] = page;
        page += arm_pages;
        page = ROUND_UP(page, flash_info->erase_page_size / SPIFLASH_PAGE_SIZE);
    }
    for (i = 0; i < FLASH_NUM_CONFIGS; ++i) {
        header->config_offset[i] = page;
        page += switch_pages;
        page = ROUND_UP(page, flash_info->erase_page_size / SPIFLASH_PAGE_SIZE);
    }
    for (i = 0; i < FLASH_NUM_CORE_DUMPS; ++i) {
        header->core_dump_offset[i] = page;
        page += core_dump_pages;
        page = ROUND_UP(page, flash_info->erase_page_size / SPIFLASH_PAGE_SIZE);
    }
    for (i = 0; i < FLASH_NUM_AVB_CONFIGS; ++i) {
        header->expansion[AVB_CFG_EXP_OFFSET + i].offset = page;
        page += avb_config_pages;
        page = ROUND_UP(page, flash_info->erase_page_size / SPIFLASH_PAGE_SIZE);
    }

    header->expansion[VPD_EXP_OFFSET].offset = page;
    /* No boot images yet */
    for (i = 0; i < FLASH_NUM_BOOT_IMAGES; ++i) {
        header->boot_image[i] = -1;
    }

	return 0;
}

