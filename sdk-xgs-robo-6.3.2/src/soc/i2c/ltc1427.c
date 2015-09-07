/*
 * $Id: ltc1427.c 1.5 Broadcom SDK $
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
 * BCM56xx I2C Device Driver for Linear Technology 1427-50 DAC.
 * The LTC1427 DAC is used to control voltage outputs on various
 * SDK baseboards. See the LTC-1427-50 datasheet for more info.
 *
 */
#include <sal/types.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/error.h>
#include <soc/i2c.h>
#include <sal/core/libc.h>

/* Calibration table in effect */
static dac_calibrate_t* dac_params;
static int dac_param_len;

STATIC int
ltc1427_read(int unit, int devno,
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
ltc1427_write(int unit, int devno,
	   uint16 addr, uint8* data, uint32 len)
{
    int rv = SOC_E_NONE;
    uint16 dw = 0;

    if(!data || (len <=0))
	return SOC_E_PARAM;

    /* Convert two bytes into single 16bit value */
    dw = data[0];
    dw <<= 8;
    dw |= data[1];

    dw &= ~0x8000; /* Make sure we never shut down DAC */
    rv = soc_i2c_write_word(unit, soc_i2c_addr(unit, devno), dw);

    soc_i2c_device(unit, devno)->tbyte +=2;
    return rv;
}

/*
 * NOTE NOTE NOTE:
 * All tables (dac_calibrate_t) passed to the ioctl() have size > 1
 * and the index is always within this range.
 */

#define LTC1427_SET_SELECT_MIN  0
#define LTC1427_SET_SELECT_MAX  1
#define LTC1427_SET_SELECT_MID  2

STATIC int
ltc1427_setmin_max(int unit, int devno, int set_sel, int idx)
{
    short data;
    int rv;

    /* set the default voltage */
    switch (set_sel) {
    case LTC1427_SET_SELECT_MAX:
        data = dac_params[idx].dac_max_hwval;
        break;
    case LTC1427_SET_SELECT_MID:
        data = dac_params[idx].dac_mid_hwval;
        break;
    case LTC1427_SET_SELECT_MIN:
        data = dac_params[idx].dac_min_hwval;
        break;
    default:
        return SOC_E_INTERNAL;
    }
    rv = ltc1427_write(unit, devno, 0, (uint8*)&data, sizeof(short));

    if (SOC_SUCCESS(rv)) {
	/* Keep last value since DAC is write-only device */
        dac_params[idx].dac_last_val = data;
    }
    return rv;
}

#define LTC1427_SETMAX(unit,devno, idx) \
   ltc1427_setmin_max(unit, devno, LTC1427_SET_SELECT_MAX, idx)
#define LTC1427_SETMIN(unit,devno, idx) \
   ltc1427_setmin_max(unit, devno, LTC1427_SET_SELECT_MIN, idx)
#define LTC1427_SETMID(unit,devno, idx) \
   ltc1427_setmin_max(unit, devno, LTC1427_SET_SELECT_MID, idx)

STATIC int
ltc1427_ioctl(int unit, int devno,
	   int opcode, void* data, int len)
{
    int rv = SOC_E_NONE;
#ifdef	COMPILER_HAS_DOUBLE
    double fval,tmp;
#else
    int fval, tmp;
#endif
    uint16 dac;
    dac_calibrate_t* params = NULL;

    /* length field is actually used as an index into the dac_params table*/
    if( !data || ( (dac_params != NULL) && (len > dac_param_len)))
	return SOC_E_PARAM;

    switch ( opcode ){

	/* Upload calibration table */
    case I2C_DAC_IOC_SET_CALTAB:
	params = (dac_calibrate_t*)data;
	dac_params = params;
	dac_param_len = len;
	break;

    case I2C_DAC_IOC_SETDAC_MIN:
	/* Set MIN voltage */
	rv = LTC1427_SETMIN(unit, devno, len);
	break;

    case I2C_DAC_IOC_SETDAC_MAX:
	/* Set MAX voltage */
	rv = LTC1427_SETMAX(unit, devno, len);
	break;

    case I2C_DAC_IOC_SETDAC_MID:
	/* Set mid-range voltage */
	rv = LTC1427_SETMID(unit, devno, len);
	break;

    case I2C_DAC_IOC_CALIBRATE_MAX:
	/* Set MAX output value (from ADC) */
#ifdef	COMPILER_HAS_DOUBLE
	fval = *((double*)data);
#else
	fval = *((int *)data);
#endif
	dac_params[len].max = fval;
	break;

    case I2C_DAC_IOC_CALIBRATE_MIN:
	/* Set MIN output value (from ADC) */
#ifdef	COMPILER_HAS_DOUBLE
	fval = *((double*)data);
#else
	fval = *((int *)data);
#endif
	dac_params[len].min = fval;
	break;

    case I2C_DAC_IOC_CALIBRATE_STEP:
	/* Calibrate stepsize */
	dac_params[len].step =
            (dac_params[len].use_max ? -1 : 1) *
	    (dac_params[len].max - dac_params[len].min) / 
               (dac_params[len].dac_max_hwval - dac_params[len].dac_min_hwval);
	soc_cm_debug(DK_VERBOSE,
		     "unit %d i2c %s: LTC1427 calibration on function %s:"
#ifdef	COMPILER_HAS_DOUBLE
		     "(max=%f,min=%f,step=%f)\n",
#else
		     "(max=%d,min=%d,step=%d)\n",
#endif
		     unit, soc_i2c_devname(unit,devno),
		     dac_params[len].name,
		     dac_params[len].max,
		     dac_params[len].min,
		     dac_params[len].step);
	break;

    case I2C_DAC_IOC_SET_VOUT:
	/* Set output voltage */
#ifdef	COMPILER_HAS_DOUBLE
	fval = *((double*)data);
#else
	fval = *((int*)data);
#endif
	if ((fval < dac_params[len].min)||(fval > dac_params[len].max)){
	    soc_cm_debug(DK_VERBOSE,
			 "unit %d i2c %s: calibration/range error :"
#ifdef	COMPILER_HAS_DOUBLE
			 "requested=%f (max=%f,min=%f,step=%f)\n",
#else
			 "requested=%d (max=%d,min=%d,step=%d)\n",
#endif
			 unit, soc_i2c_devname(unit,devno),
			 fval, dac_params[len].max,
			 dac_params[len].min,
			 dac_params[len].step);
	    return SOC_E_PARAM;
	}
	/*
	 * Core (A,B,PHY) : DACVAL  = VMax - VReq / Step
	 * TurboRef: DACVAL = VReq - VMin / Step
	 */
	if(dac_params[len].use_max)
	    tmp = ((dac_params[len].max - fval) / dac_params[len].step);
	else
	    tmp = ((fval - dac_params[len].min) / dac_params[len].step);
	dac = (uint16)tmp;

	dac &= LTC1427_VALID_BITS; /* Clear bits to make 10bit value */

	/* Show what we are doing, for now ... */
	soc_cm_debug(DK_VERBOSE,
		     "unit %d i2c %s: Set V_%s:"
#ifdef	COMPILER_HAS_DOUBLE
		     "request=%f dac=0x%x (max=%f,min=%f,step=%f)\n",
#else
		     "request=%d dac=0x%x (max=%d,min=%d,step=%d)\n",
#endif
		     unit, soc_i2c_devname(unit, devno),
		     dac_params[len].name,
		     fval,
		     dac,
		     dac_params[len].max,
		     dac_params[len].min,
		     dac_params[len].step);
	rv = ltc1427_write(unit, devno, 0, (uint8*)&dac, sizeof(short));
	/* Keep last value since DAC is write-only device */
	dac_params[len].dac_last_val = dac;
	break;

    default:
	soc_cm_debug(DK_VERBOSE,
		     "unit %d i2c %s: ltc1427_ioctl: invalid opcode (%d)\n",
		     unit, soc_i2c_devname(unit,devno),
		     opcode);
	break;
    }

    return rv;
}

STATIC int
ltc1427_init(int unit, int devno,
	  void* data, int len)
{
    dac_params = NULL;

    soc_i2c_devdesc_set(unit, devno, "Linear Technology LTC1427 DAC");

    return SOC_E_NONE;
}

/* LTC1427 Clock Chip Driver callout */
i2c_driver_t _soc_i2c_ltc1427_driver = {
    0x0, 0x0, /* System assigned bytes */
    LTC1427_DEVICE_TYPE,
    ltc1427_read,
    ltc1427_write,
    ltc1427_ioctl,
    ltc1427_init,
};
