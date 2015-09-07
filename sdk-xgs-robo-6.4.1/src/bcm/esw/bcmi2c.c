/*
 * $Id: bcmi2c.c,v 1.4 Broadcom SDK $
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
 * File:	bcmi2c.c
 * Purpose:	BCM I2C API
 */

#ifdef INCLUDE_I2C

#include <sal/types.h>

#include <soc/debug.h>
#include <soc/i2c.h>

#include <bcm/bcmi2c.h>
#include <bcm/error.h>

#include <bcm_int/esw_dispatch.h>
/*
 * Function:
 *	bcm_esw_i2c_open
 * Purpose:
 *	Open device, return valid file descriptor or -1 on error.
 * Parameters:
 *	unit - StrataSwitch device number or I2C bus number
 *	devname - I2C device name string
 *	flags - arguments to pass to attach, default value should be zero
 *	speed - I2C bus speed, if non-zero, this speed is configured, normally
 *            this argument should be zero unless a speed is desired.
 * Returns:
 *      Device identifier for all I2C operations
 * Notes:
 *      This routine should be called before attempting to communicate
 *      with an I2C device which has a registered driver.
 *      A valid driver with this device name must be installed in the system.
 */

int
bcm_esw_i2c_open(int unit, char *devname, uint32 flags, int speed)
{
    return soc_i2c_devopen(unit, devname, flags, speed);
}

/*
 * Function:
 *	bcm_esw_i2c_write
 * Purpose:
 *	Write to a device
 * Parameters:
 *	unit - StrataSwitch device number or I2C bus number
 *	fd - I2C device ID
 *	addr - device register or memory address
 *	data - data byte buffer
 *	nbytes - number of bytes of data
 * Returns:
 *	Number of bytes written on success
 *	BCM_E_XXX on failure.
 * Notes:
 *      This routine requires a driver.
 */

int
bcm_esw_i2c_write(int unit, int fd, uint32 addr, uint8 *data, uint32 nbytes)
{
    if ((fd < 0) || (fd >= soc_i2c_device_count(unit))) {
	return BCM_E_PARAM;
    }

    if (soc_i2c_device(unit, fd)->driver == NULL) {
	return BCM_E_PARAM;
    }

    return soc_i2c_device(unit, fd)->driver->write(unit, fd,
						   addr, data, nbytes);
}

/*
 * Function:
 *	bcm_esw_i2c_read
 * Purpose:
 *	Read from a device
 * Parameters:
 *	unit - StrataSwitch device number or I2C bus number
 *	fd - I2C device ID
 *	addr - device register or memory address
 *	data - data byte buffer to read into
 *	nbytes - number of bytes of data, updated on success.
 * Returns:
 *	On success, number of bytes read; nbytes updated with number
 *	of bytes read from device; BCM_E_XXX on failure.
 * Notes:
 *      This routine requires a driver.
 */

int
bcm_esw_i2c_read(int unit, int fd, uint32 addr, uint8 *data, uint32 * nbytes)
{
    if ((fd < 0) || (fd >= soc_i2c_device_count(unit))) {
	return BCM_E_PARAM;
    }

    if (soc_i2c_device(unit, fd)->driver == NULL) {
	return BCM_E_PARAM;
    }

    return soc_i2c_device(unit, fd)->driver->read(unit, fd,
						  addr, data, nbytes);
}

/*
 * Function:
 *	bcm_esw_i2c_ioctl
 * Purpose:
 *	Device specific I/O control
 * Parameters:
 *	unit - StrataSwitch device number or I2C bus number
 *	fd - I2C device ID
 *	opcode - device command code (device-specific).
 *	data - data byte buffer for command
 *	nbytes - number of bytes of data
 * Returns:
 *	On success, application specific value greater than zero,
 *	BCM_E_XXX otherwise.
 * Notes:
 *      This routine requires a driver.
 */

int
bcm_esw_i2c_ioctl(int unit, int fd, int opcode, void *data, int len)
{
    if ((fd < 0) || (fd >= soc_i2c_device_count(unit))) {
	return BCM_E_PARAM;
    }

    if (soc_i2c_device(unit, fd)->driver == NULL) {
	return BCM_E_PARAM;
    }

    return soc_i2c_device(unit, fd)->driver->ioctl(unit, fd, opcode,
						   data, len);
}

#endif /* INCLUDE_I2C */
