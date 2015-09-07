/*
 * $Id: bcm89500util.c 1.29 Broadcom SDK $
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
 * Arm Processor Subsystem remote management CLI commands
 */

#include <sal/core/libc.h>
#include <sal/types.h>
#include <sal/appl/sal.h>
#include <sal/appl/io.h>
#include <sal/core/libc.h>

#include <appl/diag/shell.h>
#include <appl/diag/system.h>
#include <appl/diag/parse.h>
#include <appl/diag/diag.h>

#include <appl/diag/aps/bcm89500lib.h>

/* This is only valid for keystone CPU */
#define KS_SPI_DEV_NAME     "/dev/spidev0.0"
#define KS_FAST_SPI_CLOCK_SPEED  8000000        /* 8MHz */
#define KS_SLOW_SPI_CLOCK_SPEED  72000          /* 72KHz */

char *_power_state_str[] = _POWER_STATE_STR_INIT;

extern void *buffer_from_file(mgmt_info_t *info, const char *name, uint32_t *len);

#if (!defined(LINUX) || !defined(__KERNEL__))
static int
_robo_aps_open_spi(mgmt_info_t *info, uint32 speed)
{
    int errno;

    errno = mgmt_open_spi(info, KS_SPI_DEV_NAME, speed);

    if (errno) {
        printk("could not open mgmt connection, errno = %d\n", errno);
    }

    return errno;
}
#endif

cmd_result_t
cmd_robo_aps(int unit, args_t *args)
{
#if (defined(LINUX) && defined(__KERNEL__))
    printk("APS command not supported in this configuration\n");
    return CMD_FAIL;
#else
    int i, errno, spi, slot, sector_addr, dmu_config, rv = CMD_OK;
    uint32_t state;
    uint32 spi_speed;
    mgmt_info_t *info;
    char *c, *filename;
    char *  mac_addr;
    char *  serial_number;

    uint32 page;
    uint8 *buf = NULL;
    uint32_t buflen;

    uint32 spread;

    uint32 port;
    uint32 command;
    mgmt_reply_acd_results_t results;

    if (ARG_CNT(args) < 1) {
        return CMD_USAGE;
    }

    c = ARG_GET(args);                  /* Command or prefix */
    if (!sal_strcmp(c, "slow")) {
        if (ARG_CNT(args) < 1) {
            return CMD_USAGE;
        }
        c = ARG_GET(args);
        spi_speed = KS_SLOW_SPI_CLOCK_SPEED;
    } else {
        spi_speed = KS_FAST_SPI_CLOCK_SPEED;    
    } 

    info = mgmt_new(NULL, NULL, NULL);
    if (!info) {
        printk("could not allocate mgmt_info_t\n");
        return CMD_FAIL;
    }

    /* Now parse and handle the commands */
    if (!sal_strcmp(c, "version")) {
        mgmt_reply_version_t version;

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        errno = mgmt_get_version(info, &version);
        if (errno) {
            printk("could not get version info, errno = %d\n", errno);
            rv = CMD_FAIL;
            goto done;
        }
        printk("mode  %d\n", version.mode);
        printk("model 0x%x\n", version.model_id);
        printk("chip  0x%x\n", version.chip_id);
        printk("args: 0x%08x 0x%08x 0x%08x 0x%08x\n",
                version.args[0], version.args[1], version.args[2], version.args[3]);
        printk("version 0x%08x\n", version.mos_version);
        printk("otp   0x%08x\n", version.otp_bits);
    } else if (!sal_strcmp(c, "reboot")) {
        if (ARG_CNT(args) != 0) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        errno = mgmt_reboot(info, 0xcafe0000, 0xcafe0001, 0xcafe0002, 0xcafe0003);
        if (errno) {
            printk("could not reboot polar, errno = %d\n", errno);
            rv = CMD_FAIL;
        }
    } else if (!sal_strcmp(c, "initialize-flash")) {
        if (ARG_CNT(args) != 3) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        spi = parse_integer(c);
        c = ARG_GET(args);
        dmu_config = parse_integer(c);
        c = ARG_GET(args);
        spi_speed = parse_integer(c);
        errno = mgmt_initialize_flash(info, spi, (uint32)dmu_config, (uint32)spi_speed);
        if (errno) {
            printk("initialize_flash failed, errno = %d\n", errno);
            rv = CMD_FAIL;
        }
    } else if (!sal_strcmp(c, "install-images")) {
        if (ARG_CNT(args) != 3) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        spi = parse_integer(c);
        c = ARG_GET(args);
        filename = ARG_GET(args);
        errno = mgmt_install_images(info, spi, c, filename);
        if (errno) {
            printk("install_images failed, errno = %d\n", errno);
            rv = CMD_FAIL;
        }
    } else if (!sal_strcmp(c, "install-arm-image")) {
        if (ARG_CNT(args) != 2) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        spi = parse_integer(c);
        filename = ARG_GET(args);
        errno = mgmt_install_arm_images(info, spi, filename);
        if (errno) {
            printk("install arm image failed, errno = %d\n", errno);
            rv = CMD_FAIL;
        }
    } else if (!sal_strcmp(c, "download-arm-image")) {
        if (ARG_CNT(args) != 1) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        filename = ARG_GET(args);
        buf = buffer_from_file(info, filename, &buflen);
        if (!buf) {
            printk("could not open file %s\n", filename);
            rv = CMD_FAIL;
            goto done;
        }
        errno = mgmt_download_image(info, buf, buflen);
        if (errno) {
            printk("download arm image failed, errno = %d\n", errno);
            rv = CMD_FAIL;
        }
    } else if (!sal_strcmp(c, "install-switch-cfg")) {
        if (ARG_CNT(args) != 2) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        spi = parse_integer(c);
        filename = ARG_GET(args);
        buf = buffer_from_file(info, filename, &buflen);
        if (!buf) {
            printk("could not open file %s\n", filename);
            rv = CMD_FAIL;
            goto done;
        }
        errno = mgmt_install_switch_images(info, spi, buf, buflen);
        if (errno) {
            printk("install switch config failed, errno = %d\n", errno);
            rv = CMD_FAIL;
        }
    } else if (!sal_strcmp(c, "install-avb-cfg")) {
        if (ARG_CNT(args) != 2) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        spi = parse_integer(c);
        filename = ARG_GET(args);
        buf = buffer_from_file(info, filename, &buflen);
        if (!buf) {
            printk("Could not open file %s\n", filename);
            rv = CMD_FAIL;
            goto done;
        }
        errno = mgmt_install_avb_configs(info, spi, buf, buflen);
        if (errno) {
            printk("Install AVB config failed, errno = %d\n", errno);
            rv = CMD_FAIL;
        }
    } else if (!sal_strcmp(c, "vpd")) {

        mgmt_reply_version_t version;
        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        errno = mgmt_get_version(info, &version);
        if (errno) {
            printk("Could not get version info, errno = %d\n", errno);
            rv = CMD_FAIL;
            goto done;
        }

        /* If in flash-init mode, we need to download and execute 
                                                 the ARM image first */
        if (version.mode == MGMT_FLASH_INIT_MODE) {
            printk("Arm is in flash-init mode\n");
            rv = CMD_FAIL;
            goto done;
        }

        i = ARG_CNT(args);
        if (i == 1){
            c = ARG_GET(args);
            spi = parse_integer(c);
            errno = mgmt_vpd_show(info, spi);
            if (errno) {
                printk("Error accessing the vpd data, errno = %d\n", errno);
                rv = CMD_FAIL;
            }
            else {
                printk("VPD programming successful\n");
            }
        }
        else if (i == 3) {
            c = ARG_GET(args);
            spi = parse_integer(c);
            mac_addr = ARG_GET(args);
            serial_number = ARG_GET(args);
            errno = mgmt_vpd_set(info, spi, (uint8 *)mac_addr, (uint8 *)serial_number);
            if (errno) {
                printk("Error programming the vpd data, errno = %d\n", errno);
                rv = CMD_FAIL;
            }
        }
        else{
            rv = CMD_USAGE;
            goto done;
        }
    } else if (!sal_strcmp(c, "read-flash-header")) {
        flash_header_t header;
        if (ARG_CNT(args) != 2) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        spi = parse_integer(c);
        c = ARG_GET(args);
        slot = parse_integer(c);
        errno = mgmt_read_flash_header(info, spi, slot, &header);
        if (errno) {
            printk("error reading flash header, errno = %d\n", errno);
            rv = CMD_FAIL;
            goto done;
        }
        printk("magic 0x%08x (%s)\n", header.magic,
               (header.magic == FLASH_HEADER_MAGIC) ? "valid" : "invalid");
        printk("cksum 0x%08x\n", header.cksum);
        for (i = 0; i < FLASH_NUM_IMAGES; ++i) {
            printk("image %d: 0x%08x (%d)\n",
                   i, header.image_offset[i], header.image_offset[i]);
        }
        for (i = 0; i < FLASH_NUM_CONFIGS; ++i) {
            printk("config %d: 0x%08x (%d)\n",
                   i, header.config_offset[i], header.config_offset[i]);
        }
        for (i = 0; i < FLASH_NUM_CORE_DUMPS; ++i) {
            printk("coredump %d: 0x%08x (%d)\n",
                   i, header.core_dump_offset[i], header.core_dump_offset[i]);
        }
        for (i = 0; i < FLASH_NUM_EXPANSION_BLOCKS; ++i) {
            printk("expansion %d: 0x%08x (%d)\n",
                   i, header.expansion[i].offset, header.expansion[i].offset);
        }

    } else if (!sal_strcmp(c, "flash-info")) {
        flash_info_t flash_info;

        if (ARG_CNT(args) != 1) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        spi = parse_integer(c);
        errno = mgmt_get_flash_info(info, spi, &flash_info);
        if (errno) {
            printk("could not read flash info\n");
            rv = CMD_FAIL;
            goto done;
        }
        printk("flash type %s / %06x\n", flash_info.name, flash_info.rdid);
        printk("flash size %d\n", flash_info.flash_size);
        printk("page size %d\n", flash_info.page_size);
        printk("erase page size %d\n", flash_info.erase_page_size);
        printk("erase sector size %d\n", flash_info.erase_sector_size);
    } else if (!sal_strcmp(c, "erase-flash-sector")) {
        if (ARG_CNT(args) != 2) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        spi = parse_integer(c);
        c = ARG_GET(args);
        sector_addr = parse_integer(c);
        errno = mgmt_erase_flash_sector(info, spi, sector_addr);
        if (errno) {
            printk("error erasing flash sector, errno = %d\n", errno);
            rv = CMD_FAIL;
        }
    } else if (!sal_strcmp(c, "read-flash")) {
        FILE *fp;
        int start_page, end_page;

        if (ARG_CNT(args) != 4) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        spi = parse_integer(c);
        c = ARG_GET(args);
        start_page = parse_integer(c);
        c = ARG_GET(args);
        end_page = parse_integer(c);
        filename = ARG_GET(args);
        fp = fopen(filename, "w");
        if (!fp) {
            printk("could not open file %s for writing", filename);
            rv = CMD_FAIL;
            goto done;
        }

        errno = mgmt_read_flash(info, spi, start_page, end_page, fp);
        fclose(fp);
        if (errno) {
            printk("error reading flash, errno = %d\n", errno);
            rv = CMD_FAIL;
        }
    } else if (!sal_strcmp(c, "write-flash")) {
        if (ARG_CNT(args) != 2) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        spi = parse_integer(c);
        filename = ARG_GET(args);
        buf = buffer_from_file(info, filename, &buflen);
        if (!buf) {
            printk("could not open file %s\n", filename);
            rv = CMD_FAIL;
            goto done;
        }
        errno = mgmt_erase_flash(info, spi, 0, (buflen + SPIFLASH_PAGE_SIZE - 1)/SPIFLASH_PAGE_SIZE);
        if (errno) {
            printk("error erasing flash, errno = %d\n", errno);
            rv = CMD_FAIL;
            goto done;
        }
        for (page = 0; page < (buflen + SPIFLASH_PAGE_SIZE - 1)/SPIFLASH_PAGE_SIZE; ++page) {

            /* only write pages with non-erased content */
            for (i = 0; i < (int)SPIFLASH_PAGE_SIZE; ++i) {
                if (buf[page*SPIFLASH_PAGE_SIZE + i] != 0xff) {
                    printk("writing flash page %d\n", page);
                    errno = mgmt_write_flash_page(info, spi, page, buf + page*SPIFLASH_PAGE_SIZE);
                    if (errno) {
                        printk("error writing flash, errno = %d\n", errno);
                        rv = CMD_FAIL;
                        goto done;
                    }
                    break;
                }
            }
        }
    } else if (!sal_strcmp(c, "shell-cmd")) {
        if (ARG_CNT(args) != 1) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        errno = mgmt_shell_cmd(info, c);
        if (errno) {
            printk("could not execute shell command, errno = %d\n", errno);
            goto done;
        }
    } else if (!sal_strcmp(c, "set-power-state")) {
        if (ARG_CNT(args) != 1) {
            rv = CMD_USAGE;
            goto done;
        }
        c = ARG_GET(args);
        for (state = 0; state < _POWER_STATE_LIMIT; state++) {
            if (!sal_strcmp(c, _power_state_str[state])) {
                break;
            }
        }
        if (state == _POWER_STATE_LIMIT) {
            printk("unknown state '%s'\n", c);
            rv = CMD_FAIL;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        errno = mgmt_set_power_state(info, state);
        if (errno) {
            printk("could not set power state(%s), errno = %d\n", _POWER_STATE_STR(state), errno);
            rv = CMD_FAIL;
            goto done;
        }
    } else if (!sal_strcmp(c, "get-power-state")) {
        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }
        errno = mgmt_get_power_state(info, &state);
        if (errno) {
            printk("could not get power state, errno = %d\n", errno);
            rv = CMD_FAIL;
            goto done;
        }
        printk("power state : %s\n", _POWER_STATE_STR(state));
    } else if (!sal_strcmp(c, "pll-spread")) {
        if (ARG_CNT(args) != 1) {
            rv = CMD_USAGE;
            goto done;
        }

        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        spread = parse_integer(c);
        errno = mgmt_pll_spread(info, spread);
        if (errno) {
            printk("could not issue pll-spread command (%x), errno = %d\n",
                   spread, errno);
            rv = CMD_FAIL;
            goto done;
        }
    } else if (!sal_strcmp(c, "acd")) {
        if (ARG_CNT(args) != 2) {
            rv = CMD_USAGE;
            goto done;
        }
        errno = _robo_aps_open_spi(info, spi_speed);
        if (errno) {
            rv = CMD_FAIL;
            goto done;
        }

        c = ARG_GET(args);
        port = parse_integer(c);
        c = ARG_GET(args);
        command = parse_integer(c);
        errno = mgmt_acd(info, port, command, &results);
        if (errno) {
            printk("could not issue ACD port %d command 0x%08x, errno = %d\n",
                   port, command, errno);
            rv = CMD_FAIL;
            goto done;
        }
        printk("fault code = 0x%08x\n", results.fault);
        printk("index = %d\n", results.index);
        printk("peak amplitude = %d\n", results.peak_amplitude);
    } else {
        printk("unknown command '%s'\n", c);
    }

done:
    mgmt_close(info);
    if (buf) {
        sal_free(buf);
    }
    return rv;
#endif
}
