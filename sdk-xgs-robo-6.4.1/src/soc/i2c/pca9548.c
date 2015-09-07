/*
 * $Id: pca9548.c,v 1.5.84.2 Broadcom SDK $
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
 * BCM56xx I2C Device Driver for Phillips PCA9548 i2c switch
 *
 *
 */

#include <sal/types.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/error.h>
#include <soc/i2c.h>
#include <shared/bsl.h>

STATIC int
pca9548_read(int unit, int devno, uint16 addr, uint8* data, uint32* len)
{
    
    int rv;
    uint8 saddr = soc_i2c_addr(unit, devno);

    rv = soc_i2c_read_byte(unit, saddr, data);
    *len = 1; /* Byte device */
    /* LOG_CLI((BSL_META_U(unit,
                           "sw%d: read 0x%x from sw at 0x%x\n"), devno, *data, saddr)); */
    soc_i2c_device(unit, devno)->rbyte++;   
    return rv;
}

/*
 * The data is what gets actually written to the channel selection
 * register. The channel must be converted to the appropriate bit
 * by the caller.
 */
STATIC int
pca9548_write(int unit, int devno, uint16 addr, uint8* data, uint32 len)
{
    int rv;
    uint8 saddr = soc_i2c_addr(unit, devno);

    rv = soc_i2c_write_byte(unit, saddr, *data);
    /* LOG_CLI((BSL_META_U(unit,
                           "sw%d: wrote 0x%x to sw at 0x%x\n"), devno, *data, saddr)); */
    soc_i2c_device(unit, devno)->tbyte++;   
    return rv;
}



STATIC int
pca9548_ioctl(int unit, int devno, int opcode, void* data, int len)
{
    return SOC_E_NONE;
}

STATIC int
pca9548_init(int unit, int devno, void* data, int len)
{

    soc_i2c_devdesc_set(unit, devno, "PCA9548 i2c Switch");
    if (!SOC_IS_SHADOW(unit) || !SOC_IS_CALADAN3(unit)) {
        /* enable channel 0 by default */
        return soc_i2c_write_byte(unit, soc_i2c_addr(unit, devno), 1);
    }
    return SOC_E_NONE;
}


/* PCA9548 Clock Chip Driver callout */
i2c_driver_t _soc_i2c_pca9548_driver = {
    0x0, 0x0, /* System assigned bytes */
    PCA9548_DEVICE_TYPE,
    pca9548_read,
    pca9548_write,
    pca9548_ioctl,
    pca9548_init,
};

