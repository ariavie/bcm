/*
 * $Id: max5478.c 1.3 Broadcom SDK $
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
 * All Rights Reserved.$
 *
 * BCM56xx I2C Device Driver for Maxim 5478 Digital Potentiometer
 * The Maxim 5478 digital potentiometer is used to adjust the voltage
 * divider in SDK baseboards. See the Maxim 5478 datasheet for more info.
 *
 */
#include <sal/types.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/error.h>
#include <soc/i2c.h>
#include <sal/core/libc.h>

/* Command byte - see Max5478, Table 2 */
#define MAX5478_CMD_WIPER_A_VREG            0x11
#define MAX5478_CMD_WIPER_A_VREG_X_NVREG    0x51
#define MAX5478_CMD_WIPER_B_CMD             0x12
#define MAX5478_CMD_WIPER_B_VREG_X_NVREG    0x52

STATIC int
max5478_read(int unit, int devno,
	  uint16 addr, uint8* data, uint32* len)
{
    COMPILER_REFERENCE(unit);
    COMPILER_REFERENCE(devno);
    COMPILER_REFERENCE(addr);
    COMPILER_REFERENCE(data);
    COMPILER_REFERENCE(len);

    return SOC_E_NONE;
}

STATIC int
max5478_write(int unit, int devno,
	   uint16 addr, uint8* data, uint32 len)
{
    int rv = SOC_E_NONE;
    return rv;
}

STATIC int
max5478_ioctl(int unit, int devno,
              int request, void* data, int len)
{
    int rv = SOC_E_NONE;
    uint16 msg0;
    uint8  msg1;
    max5478_t *params;

    switch (request) {
    case I2C_POT_IOC_SET:
        params = (max5478_t *)data;
        switch (params->wiper) {
        case I2C_POT_MSG_WIPER_A:
             msg0 = MAX5478_CMD_WIPER_A_VREG;
             msg1 = MAX5478_CMD_WIPER_A_VREG_X_NVREG;
            break;

        case I2C_POT_MSG_WIPER_B:
            msg0 = MAX5478_CMD_WIPER_B_CMD;
            msg1 = MAX5478_CMD_WIPER_B_VREG_X_NVREG;
            break;

        default:
            soc_cm_debug(DK_VERBOSE, "POT%d: invalid wiper %d\n",
                         devno, params->wiper);
            return SOC_E_PARAM;
        }

        /* Command byte VREG + data byte */
        msg0 = (msg0 << 8) | params->value;
        rv = soc_i2c_write_word(unit, soc_i2c_addr(unit, devno), msg0);
        if (rv == SOC_E_NONE) {
            soc_i2c_device(unit, devno)->tbyte +=2;
        }

        /* Transfer VREG to NVREG */
        rv = soc_i2c_write_byte(unit, soc_i2c_addr(unit, devno), msg1);
        if (rv == SOC_E_NONE) {
            soc_i2c_device(unit, devno)->tbyte++;
        }

        break;

    default:
        soc_cm_debug(DK_VERBOSE, "POT%d: invalid request %d\n",
                     devno, request);
        return SOC_E_PARAM;
    }
    return rv;
}

STATIC int
max5478_init(int unit, int devno,
	  void* data, int len)
{
    soc_i2c_devdesc_set(unit, devno, "Maxim 5478 Digital Potentiometer");

    return SOC_E_NONE;
}

/* MAX5478 Clock Chip Driver callout */
i2c_driver_t _soc_i2c_max5478_driver = {
    0x0, 0x0, /* System assigned bytes */
    MAX5478_DEVICE_TYPE,
    max5478_read,
    max5478_write,
    max5478_ioctl,
    max5478_init,
};
