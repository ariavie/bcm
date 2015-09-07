/*
 * $Id: phy84756_i2c.h,v 1.3 Broadcom SDK $
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

#ifndef _PHY84756_I2C_H
#define _PHY84756_I2C_H
/* I2C related defines */
#define PHY84756_BSC_XFER_MAX     0x1F9
#define PHY84756_BSC_WR_MAX       16
#define PHY84756_WRITE_START_ADDR 0x8007
#define PHY84756_READ_START_ADDR  0x8007
#define PHY84756_WR_FREQ_400KHZ   0x0100
#define PHY84756_2W_STAT          0x000C
#define PHY84756_2W_STAT_IDLE     0x0000
#define PHY84756_2W_STAT_COMPLETE 0x0004
#define PHY84756_2W_STAT_IN_PRG   0x0008
#define PHY84756_2W_STAT_FAIL     0x000C
#define PHY84756_BSC_WRITE_OP     0x22
#define PHY84756_BSC_READ_OP      0x2
#define PHY84756_I2CDEV_WRITE     0x1
#define PHY84756_I2CDEV_READ      0x0
#define PHY84756_I2C_8BIT         0
#define PHY84756_I2C_16BIT        1
#define PHY84756_I2C_TEMP_RAM     0xE
#define PHY84756_I2C_OP_TYPE(access_type,data_type) \
        ((access_type) | ((data_type) << 8))
#define PHY84756_I2C_ACCESS_TYPE(op_type) ((op_type) & 0xff)
#define PHY84756_I2C_DATA_TYPE(op_type)   (((op_type) >> 8) & 0xff)

extern int
_phy_84756_bsc_rw(phy_ctrl_t *pc, int dev_addr, int opr,
                    int addr, int count, void *data_array,buint32_t ram_start);

extern int
phy_84756_i2cdev_read(phy_ctrl_t *pc,
                     int dev_addr,  /* 7 bit I2C bus device address */
                     int offset,    /* starting data address to read */
                     int nbytes,    /* number of bytes to read */
                     buint8_t *read_array);   /* buffer to hold retrieved data */


extern int
phy_84756_i2cdev_write(phy_ctrl_t *pc,
                     int dev_addr,  /* I2C bus device address */
                     int offset,    /* starting data address to write to */
                     int nbytes,    /* number of bytes to write */
                     buint8_t *write_array);   /* buffer to hold written data */

#endif /* _PHY84756_I2C_H */


