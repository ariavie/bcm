/*
 * $Id: max6653.c 1.10 Broadcom SDK $
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
 *               [BSC] - Broadcom Serical Control (I2C constroller)
 *                 |
 *       ==========O=======o================ I2C bus
 *                         |
 *                    [pca9548 - 8-port mux]
 *                      | | | | | | | |
 *           [eeprom]  -  | | | | | | |
 *            [max6653]  -  | | | | | |
 *             [max6653]   -  | | |
 *               [max6653]   -  | |
 *                 [max6653]   -   - [dpm73]
 */


#if defined(INCLUDE_I2C) && defined(BCM_FE2000_SUPPORT)
#include <sal/types.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/error.h>
#include <soc/bsc.h>

extern unsigned int
sbFe2000UtilIICRead(	int unit,
			unsigned int slave_dev_addr,
			unsigned int reg_index,
			unsigned int *data);

extern unsigned int
sbFe2000UtilIICWrite(	int unit,
			unsigned int slave_dev_addr,
			unsigned int reg_index,
			unsigned int data);

static void sensor_show(int unit, int devno);
static int max6653_read(int unit, int devno, uint32 reg, uint32 *data);
static int max6653_write(int unit, int devno, uint32 reg, uint32 data);

int soc_bsc_max6653_set(int unit, int devno, int speed)
{
	int retv;
        uint32 reg_val = 0;

	if (!soc_bsc_is_attached(unit)) {
		soc_bsc_attach(unit);
	}

	if (soc_bsc_devtype(unit, devno) != MAX6653_DEVICE_TYPE) {
		soc_cm_print("Incorrect device type: %d should be %d\n",
			soc_bsc_devtype(unit, devno), MAX6653_DEVICE_TYPE);
		return SOC_E_PARAM;
	}

        /*
         * Reset MAX6653 registers to default values
         * 0x01 - configuration register 2
         */
        retv = max6653_write(unit, devno, 0x01, 0x80);
        if (retv < 0) {
                return retv;
        }
        sal_usleep(1000);
	retv = max6653_read(unit, devno, 0x01, &reg_val);
        if (retv < 0) {
                return retv;
        }

        /*
         * enable fan control and monitor
         * 0x01 - Enable fan-speed control
         * 0x04 - Tachometer digital/analog input selection: analog input
         * 0x08 - Invert the PWM output: PWM active high
         * 0x10 - FAN_FAULT output enable: enabled
         */
        retv = max6653_write(unit, devno, 0x00, 0x1d);
        if (retv < 0) {
                return retv;
        }

        /*
         * set fan speed
         * 0x22 - fan-speed configuration register
         */
        retv = max6653_write(unit, devno, 0x22, speed);
        if (retv < 0) {
                return retv;
        }

	return 0;
}
static unsigned int regmap[0x40] = {
    0xFF, 0x01, 0x02, 0x03, 0x00, 0x00, 0x06, 0x00,
    0x08, 0x00, 0x0A, 0x0B, 0x00, 0x0D, 0x0E, 0x00,
    0x10, 0x00, 0x00, 0x00, 0x00, 0x15, 0x16, 0x00,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x00, 0x00, 0x00,
    0x20, 0x00, 0x22, 0x23, 0x24, 0x25, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3D, 0x3E, 0x3F,
};

void soc_bsc_max6653_show(int unit)
{
	int devno, ndev;

	if (!soc_bsc_is_attached(unit)) {
		soc_bsc_attach(unit);
		sal_usleep(500000);
	}

	ndev = soc_bsc_device_count(unit);
	for (devno = 0; devno < ndev; devno++) {
		if (soc_bsc_devtype(unit, devno) == MAX6653_DEVICE_TYPE) {
                    sensor_show(unit, devno);
                    if (soc_cm_debug_check(DK_I2C)) {
                        int iii, retv;
                        uint32 regv = 0;
                        soc_cm_print("\nChannel MAP 0x%02x\n", BSCBUS(unit)->devs[devno]->chan);
                        for(iii = 0x0; iii <= 0x3F; iii++) { 
                            if ((iii % 8) == 0) {
                                soc_cm_print("\nTS_FC 0x%02x : ", iii);
                            }
                            if (regmap[iii]) {
                                retv = max6653_read(unit, devno, iii, &regv);
                                if (retv != 0) {
                                        return;
                                }
                                soc_cm_print("0x%02x ", regv);
                            } else {
                                soc_cm_print("---- ");
                            }
                        }
                        soc_cm_print("\n");
                    }
		}
	}
}

static void max6653_thread(void *unitp)
{
	soc_bsc_bus_t *bscbus;
	int unit;

	unit = PTR_TO_INT(unitp);
	bscbus = BSCBUS(unit);

	while (bscbus->mon_delay) {
		soc_cm_print("\n -- unit %d: sensor monitoring every %d seconds --\n",
				unit, bscbus->mon_delay);
		soc_bsc_max6653_show(unit);
		sal_usleep(bscbus->mon_delay * SECOND_USEC);
	}
	soc_cm_print("unit %d: sensor monitoring completed\n", unit);
	sal_thread_exit(0);
}

void soc_bsc_max6653_watch(int unit, int enable, int nsecs)
{
	soc_bsc_bus_t *bscbus;

	if (!soc_bsc_is_attached(unit)) {
		soc_bsc_attach(unit);
	}
	bscbus = BSCBUS(unit);

	if (!enable) {
		bscbus->mon_delay = 0;
		return;
	}
	bscbus->mon_delay = nsecs;

	sal_thread_create("",	SAL_THREAD_STKSZ,
				50,
				(void (*)(void*))max6653_thread,
				INT_TO_PTR(unit));
	soc_cm_print("unit %d: sensor monitoring enabled\n", unit);
}

static void sensor_show(int unit, int devno)
{
        int retv;
        uint32 local = 0;
        uint32 remote = 0;
        uint32 local_offset = 0;
        uint32 remote_offset = 0;
        uint32 fan_speed_count = 0;
        uint32 reg_config0 = 0;

        /*
         * Read local temperature
         */
	retv = max6653_read(unit, devno, 0x0A, &local);
	if (retv != 0) {
		return;
	}

        /*
         * Read remote temperature
         */
	retv = max6653_read(unit, devno, 0x0B, &remote);
	if (retv != 0) {
		return;
	}

        /*
         * Read local offset
         */
	retv = max6653_read(unit, devno, 0x0D, &local_offset);
	if (retv != 0) {
		return;
	}

        /*
         * Read remote offset
         */
	retv = max6653_read(unit, devno, 0x0E, &remote_offset);
	if (retv != 0) {
		return;
	}

        soc_cm_print("sensor %d: local: %dC remote: %dC", devno,
                                local + local_offset, remote + remote_offset);

        /*
         * Fan speed count
         */
	retv = max6653_read(unit, devno, 0x00, &reg_config0);
        if (retv < 0) {
		soc_cm_print(" fan speed reading error\n");
                return;
        }

        if (reg_config0 & 0x01) {
		retv = max6653_read(unit, devno, 0x08, &fan_speed_count);
                if (retv < 0) {
			soc_cm_print(" fan speed reading error\n");
                        return;
                }
                soc_cm_print(" fan speed (RPM): %d\n", 675000/(fan_speed_count * 2) - 1323);
        } else {
                soc_cm_print(" fan control inactive\n");
        }
}

static int max6653_read(int unit, int devno, uint32 reg, uint32 *data)
{
	soc_bsc_bus_t *bscbus = BSCBUS(unit);
	int chan = bscbus->devs[devno]->chan;
	int saddr = bscbus->devs[devno]->saddr;
	int mux = bscbus->devs[devno]->mux;
	int retv;

	BSC_LOCK(unit);

	/* Select MUX to slave device */
	retv = sbFe2000UtilIICWrite(unit, mux, chan, chan);
	if (retv != 0) {
		BSC_UNLOCK(unit);
		return retv;
	}
	retv = sbFe2000UtilIICRead(unit, saddr, reg, data);
	if (retv != 0) {
		BSC_UNLOCK(unit);
		return retv;
	}

	BSC_UNLOCK(unit);
	return SOC_E_NONE;
}

static int max6653_write(int unit, int devno, uint32 reg, uint32 data)
{
	soc_bsc_bus_t *bscbus = BSCBUS(unit);
	int chan = bscbus->devs[devno]->chan;
	int saddr = bscbus->devs[devno]->saddr;
	int mux = bscbus->devs[devno]->mux;
	int retv;

	BSC_LOCK(unit);

	/* Select MUX to slave device */
	retv = sbFe2000UtilIICWrite(unit, mux, chan, chan);
	if (retv != 0) {
		BSC_UNLOCK(unit);
		return retv;
	}

	retv = sbFe2000UtilIICWrite(unit, saddr, reg, data);
	if (retv != 0) {
		BSC_UNLOCK(unit);
		return retv;
	}

	BSC_UNLOCK(unit);
	return SOC_E_NONE;
}



static int max6653_ioctl(int unit, int devno, int opcode, void* data, int len)
{
        int retv;
	uint32 device_id, manufacturer_id;

	switch(opcode) {
	case BSC_IOCTL_DEVICE_PRESENT:

		retv = max6653_read(unit, devno, BSC_MAX6653_DEVID_OFF, &device_id);
		if (retv != 0) {
			return 0;
		}

		retv = max6653_read(unit, devno, BSC_MAX6653_MANUF_OFF, &manufacturer_id);
		if (retv != 0) {
			return 0;
		}

		if ((device_id == BSC_MAX6653_DEVID) && (manufacturer_id == BSC_MAX6653_MANUF)) {
			return 1; /* found */
		} else {
			return 0; /* not found */
		}
		break;

	default:
		break;
	}
	return 0;
}

static int max6653_init(int unit, int devno)
{
	int retv;
        uint32 reg_val = 0;

        /*
         * Reset MAX6653 registers to default values
         */
	retv = max6653_write(unit, devno, 0x01, 0x80);
        if (retv < 0) {
                return retv;
        }
        sal_usleep(1000);
	retv = max6653_read(unit, devno, 0x01, &reg_val);
        if (retv < 0) {
                return retv;
        }
        /*
         * enable fan control and monitor
         * 0x01 - Enable fan-speed control
         * 0x04 - Tachometer digital/analog input selection: analog input
         * 0x08 - Invert the PWM output: PWM active high
         * 0x10 - FAN_FAULT output enable: enabled
         */
	retv = max6653_write(unit, devno, 0x00, 0x1d);
        if (retv < 0) {
                return retv;
        }

        /*
         * set fan characteristics register
         */
	retv = max6653_write(unit, devno, 0x20, 0x7d);
        if (retv < 0) {
                return retv;
        }

        /*
         * set fan speed
         */
	retv = max6653_write(unit, devno, 0x22, 15);
        if (retv < 0) {
                return retv;
        }

	return SOC_E_NONE;
}

bsc_driver_t _soc_bsc_max6653_driver = {
	0x0,			/* flags */
	0x0,			/* devno */
	MAX6653_DEVICE_TYPE,	/* id */
	max6653_read,
	max6653_write,
	max6653_ioctl,
	max6653_init,
};

/*
 * Useful debug routine
 */
void soc_bsc_setmux(int unit, int chan)
{
	int retv;

	/* Make sure that we're already attached, or go get attached */
	if (!soc_bsc_is_attached(unit)) {
		soc_bsc_attach(unit);
	}

	BSC_LOCK(unit);

	/* Select MUX to slave device */
	retv = sbFe2000UtilIICWrite(unit, BSC_PCA9548_SADDR, 0, 1 << chan);
	if (retv != 0) {
		BSC_UNLOCK(unit);
		return;
	}
	BSC_UNLOCK(unit);
}


/*
 * Useful debug routine
 * Mux channel is pre-set
 */
void soc_bsc_readmax(int unit)
{
	int retv;
	uint32 device_id = 0, manufacturer_id = 0;

	/* Make sure that we're already attached, or go get attached */
	if (!soc_bsc_is_attached(unit)) {
		soc_bsc_attach(unit);
	}

	BSC_LOCK(unit);

	retv = sbFe2000UtilIICRead(unit,
			BSC_MAX6653_SADDR, BSC_MAX6653_DEVID_OFF, &device_id);
	if (retv != 0) {
		BSC_UNLOCK(unit);
		return;
	}

	retv = sbFe2000UtilIICRead(unit,
			BSC_MAX6653_SADDR, BSC_MAX6653_MANUF_OFF, &manufacturer_id);
	if (retv != 0) {
		BSC_UNLOCK(unit);
		return;
	}
	BSC_UNLOCK(unit);

	soc_cm_print("devid: 0x%02x vend: 0x%02x\n", device_id, manufacturer_id);
}

#else/* BCM_FE2000_SUPPORT && INCLUDE_I2C */
int _soc_i2c_max6653_not_empty;
#endif /* BCM_FE2000_SUPPORT && INCLUDE_I2C */
