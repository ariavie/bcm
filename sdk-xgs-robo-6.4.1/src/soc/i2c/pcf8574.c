/*
 * $Id: pcf8574.c,v 1.4 Broadcom SDK $
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
 * BCM56xx I2C Device Driver for Phillips PCF8574 Parallel port.
 *
 * On P48 systems, the parallel port is typically connected to two
 * MUX's used to control I2C slave addressing on the common I2C bus.
 *
 *
 *       o-------[MUX1]--[MUX2]
 *       |         |       |
 *       |         |       |
 *       |         |       |
 *   [PCF8574]   (SxA)   (SyB)
 *       |         |       |
 *       0=========O=======0================I2C
 *
 *
 * SxA = Slave X with Address Bits A
 * SxB = Slave Y with Address Bits B
 *
 *
 */

#include <shared/bsl.h>

#include <sal/types.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/error.h>
#include <soc/i2c.h>
#include <shared/bsl.h>

STATIC int
pcf8574_read(int unit, int devno,
	  uint16 addr, uint8* data, uint32* len)
{
    
    int rv;
    uint8 saddr = soc_i2c_addr(unit, devno);

    rv = soc_i2c_read_byte(unit, saddr, data);
    *len = 1; /* Byte device */
    /* LOG_CLI((BSL_META_U(unit,
                           "lpt0: read 0x%x from HW\n"),*data)); */
    soc_i2c_device(unit, devno)->rbyte++;   
    return rv;
}

STATIC int
pcf8574_write(int unit, int devno,
	   uint16 addr, uint8* data, uint32 len)
{
    int rv;
    uint8 saddr = soc_i2c_addr(unit, devno);

    rv = soc_i2c_write_byte(unit, saddr, *data);
    /* LOG_CLI((BSL_META_U(unit,
                           "lpt0: wrote 0x%x to HW\n"),*data)); */
    soc_i2c_device(unit, devno)->tbyte++;   
    return rv;
}



STATIC int
pcf8574_ioctl(int unit, int devno,
	   int opcode, void* data, int len)
{
    return SOC_E_NONE;
}

STATIC int
pcf8574_init(int unit, int devno,
	  void* data, int len)
{
    uint32 bytes;
    uint8 lpt_val = 0;
    uint8 saddr = soc_i2c_addr(unit, devno);        

    if (saddr == I2C_LPT_SADDR0) {
        pcf8574_read(unit, devno, 0, &lpt_val, &bytes);
	if (lpt_val == 0xFF) {
	    /* This is probably immediately after power-on; reset to zero. */
	    lpt_val = 0;
	    pcf8574_write(unit, devno, 0, &lpt_val, 1);
	}
	soc_i2c_devdesc_set(unit, devno, "PCF8574 MUX control");
	LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: mux control 0x%x\n"),
                     soc_i2c_devname(unit, devno), lpt_val));
    } else if (saddr == I2C_LPT_SADDR1) {
        /* This device used as input; write 0xFF to disable quasi-outputs */
        lpt_val = 0xFF;
	pcf8574_write(unit, devno, 0, &lpt_val, 1);

	/* Now read what is being driven onto the quasi-inputs */
	pcf8574_read(unit, devno, 0, &lpt_val, &bytes);
	soc_i2c_devdesc_set(unit, devno, "PCF8574 Baseboard ID");
	LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: baseboard id 0x%x\n"),
                     soc_i2c_devname(unit, devno), lpt_val));
	SOC_CONTROL(unit)->board_type = lpt_val;
    } else if (saddr == I2C_LPT_SADDR2) {
        /* 
	 * The power-up state is the appropriate default; 
	 * no need to manually initialize the HCLK PCF8574.
	 */
	soc_i2c_devdesc_set(unit, devno, "PCF8574 HCLK control");
	LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: hclk control 0x%x\n"),
                     soc_i2c_devname(unit, devno), lpt_val));
    } else if (saddr == I2C_LPT_SADDR3) {
        pcf8574_read(unit, devno, 0, &lpt_val, &bytes);
	if (lpt_val == 0xFF) {
	    /* This is probably immediately after power-on; reset to zero. */
	    lpt_val = 0x0;
	    pcf8574_write(unit, devno, 0, &lpt_val, 1);
	}
	pcf8574_write(unit, devno, 0, &lpt_val, 1);
	soc_i2c_devdesc_set(unit, devno, "PCF8574 POE control");
	LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: poe control 0x%x\n"),
                     soc_i2c_devname(unit, devno), lpt_val));
    } else if (saddr == I2C_LPT_SADDR4) {
	soc_i2c_devdesc_set(unit, devno, "PCF8574 synthesizer frequency selector M");
	LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: synth freq select B 0x%x\n"),
                     soc_i2c_devname(unit, devno), lpt_val));
    } else if (saddr == I2C_LPT_SADDR5) {
	soc_i2c_devdesc_set(unit, devno, "PCF8574 synthesizer frequency selector N");
	LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: synth freq select A 0x%x\n"),
                     soc_i2c_devname(unit, devno), lpt_val));
    } else if (saddr == I2C_LPT_SADDR6) {
	soc_i2c_devdesc_set(unit, devno, "PCF8574 PPD clock delay");
	LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: clock selector 0x%x\n"),
                     soc_i2c_devname(unit, devno), lpt_val));
    } else if (saddr == I2C_LPT_SADDR7) {
	soc_i2c_devdesc_set(unit, devno, "PCF8574 PPD clock divider");
	LOG_VERBOSE(BSL_LS_SOC_COMMON,
                    (BSL_META_U(unit,
                                "%s: clock selector 0x%x\n"),
                     soc_i2c_devname(unit, devno), lpt_val));
    } else {
	soc_i2c_devdesc_set(unit, devno, "PCF8574 Parallel Port");
    }
    return SOC_E_NONE;
}


/* PCF8574 Clock Chip Driver callout */
i2c_driver_t _soc_i2c_pcf8574_driver = {
    0x0, 0x0, /* System assigned bytes */
    PCF8574_DEVICE_TYPE,
    pcf8574_read,
    pcf8574_write,
    pcf8574_ioctl,
    pcf8574_init,
};

