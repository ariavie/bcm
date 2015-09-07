/*
 * $Id: spiflash_toc.h,v 1.11 Broadcom SDK $
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

/*
 * Warning:
 * This file is included by both uKernel and host code. Be very careful
 * about what goes into this file, esp. with regards to including other
 * files.
 */

#ifndef SPIFLASH_TOC_H
#define SPIFLASH_TOC_H

#ifdef BCM_UKERNEL
#include "sdk_typedefs.h"
#else
#include <sal/types.h>
#endif

#include <soc/shared/mos_coredump.h>

/*
 * Flash header at the beginning of flash that tells the ROM where
 * to find images and switch configuration info.
 */

#define FLASH_NUM_HEADER_BLOCKS 2
#define FLASH_HEADER_MAGIC      0xa5c35a3c
#define FLASH_HEADER_MAGIC2     0xb6d46b4d
#define FLASH_NUM_BOOT_IMAGES   2
#define FLASH_NUM_IMAGES        4
#define FLASH_NUM_CONFIGS       2
#define FLASH_NUM_AVB_CONFIGS   2
#define FLASH_NUM_CORE_DUMPS    1
#define FLASH_NUM_EXPANSION_BLOCKS  4

#define FLASH_ARM_IMAGE_SIZE    ((128 + 64) * 1024)     /* 128K itcm, 64K dtcm */
#define FLASH_SWITCH_IMAGE_SIZE (8 * 1024)              /* Maximum switch configuration size */
#define FLASH_AVB_IMAGE_SIZE    (8 * 1024)                /* Maximum AVB configuration size */
#define FLASH_CORE_IMAGE_SIZE   ((128 + 64) * 1024)
#define FLASH_AVB_CONFIG_SIZE   (16 * 1024)
#define FLASH_VPD_SIZE          256
#define AVB_CFG_EXP_OFFSET      0
#define VPD_EXP_OFFSET          2

typedef struct flash_expansion_s {
    uint32      offset;
    uint32      length;
} flash_expansion_t;

typedef struct flash_header_s {
    uint32              magic;                                  /* magic number */
    uint32              cksum;                                  /* simple checksum */
    uint32              spi_speed;                              /* new SPI speed */
    uint32              dmu_config;                             /* DMU clock config to use */
    uint32              boot_image[FLASH_NUM_BOOT_IMAGES];      /* boot image numbers */
    uint32              image_offset[FLASH_NUM_IMAGES];         /* images */
    uint32              config_offset[FLASH_NUM_CONFIGS];       /* switch configs */
    uint32              core_dump_offset[FLASH_NUM_CORE_DUMPS]; /* offset for the core dump */
    flash_expansion_t   expansion[FLASH_NUM_EXPANSION_BLOCKS];

    /* For future expansion */
    uint32              unused[16 - 2 * FLASH_NUM_EXPANSION_BLOCKS];

    /* For Polar B0/ARM_CLK_CONFIG - DMU configuration values */
    uint32              magic2;                                 /* magic number */
    uint32              cksum2;                                 /* checksum */
    uint32              dmu_hclk_freq;
    uint32              dmu_hclk_sel;
    uint32              dmu_pclk_freq;
    uint32              dmu_pclk_sel;
    uint32              dmu_p1div;
    uint32              dmu_p2div;
    uint32              dmu_ndiv;
    uint32              dmu_m1div;
    uint32              dmu_m2div;
    uint32              dmu_m3div;
    uint32              dmu_m4div;
    uint32              dmu_pll_num;
    uint32              dmu_frac;
    uint32              dmu_bclk_sel;
} flash_header_t;

/*
 * Image header at the start of each ARM image.
 */

#define IMAGE_HEADER_MAGIC      0x7865cbe3

typedef struct mos_arm_image_header_s {
    uint32      magic;                  /* magic number */
    uint32      cksum;                  /* header checksum */
    uint32      image_cksum;            /* image checksum */
    uint32      length;                 /* image length in bytes */
} mos_arm_image_header_t;

/*
 * Switch configuration header
 */
#define SWITCH_IMAGE_HEADER_MAGIC_ROM   0xe523fc72
#define SWITCH_IMAGE_HEADER_MAGIC_PROD  0xe523fc71

typedef struct mos_switch_config_header_s {
    uint32      magic;                  /* magic number */
    uint32      cksum;                  /* header checksum */
    uint32      image_cksum;            /* image checksum */
    uint32      length;                 /* image length in bytes */
} mos_switch_config_header_t;

/*
 * Switch configuration entry
 */

typedef struct mos_switch_config_entry_s {
    uint32      address;
    uint32      size;
    uint64      value;
} mos_switch_config_entry_t;

/*
 * Core dump image header
 */
#define MOS_COREDUMP_HEADER_MAGIC   0x65fc2d9a
#define MOS_COREDUMP_REGIONS        6           /* max number of memory regions */

typedef struct mos_coredump_header_s {
    uint32    magic;                  /* magic number */
    uint32    start;                  /* first page of data */
    uint32    end;                    /* last page of data + 1 */
    mos_coredump_region_t regions[MOS_COREDUMP_REGIONS + 1];
} mos_coredump_header_t;

/*
 * General expansion header
 */
typedef struct mos_flash_expansion_header_s {
  uint32      magic;        /* magic number*/
  uint32      cksum;        /* header checksum */
  uint32      image_cksum;  /* image checksum in bytes*/
  uint32      length;       /* image length in bytes*/
} mos_flash_expansion_header_t;

/*
 * AVB configuration header
 */

#define AVB_CONFIG_HEADER_MAGIC      0xe523fc72
#endif
