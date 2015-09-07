/*
 * $Id: phy84756_i2c.c 1.1 Broadcom SDK $
 *
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
 * File:        phy84756.c
 * Purpose:     SDK PHY driver for BCM84756 (FCMAP)
 *
 * Supported BCM546X Family of PHY devices:
 *
 *      Device  Ports    Media                           MAC Interface
 *      84756    4       4 10G SFP+                      XFI
 *      84757    4       4 10G SFP+/8(4/2) FC            XFI
 *      84759    4       4 10G SFP+                      XFI
 *
 *	             OUI	    Model	Revision
 *  BCM84756	18-C0-86	100111	00xx
 *  BCM84757	18-C0-86	100111	10xx
 *  BCM84759	18-C0-86	100111	01xx
 *
 *
 * Workarounds:
 *
 * References:
 *     
 * Notes:
 */ 


#include <sal/types.h>
#include <sal/core/spl.h>

#include <soc/drv.h>
#include <soc/debug.h>
#include <soc/error.h>
#include <soc/phyreg.h>

#include <soc/phy.h>
#include <soc/phy/phyctrl.h>
#include <soc/phy/drv.h>

#include "phydefs.h"      /* Must include before other phy related includes */



#if defined(INCLUDE_FCMAP) || defined(INCLUDE_MACSEC)
#if defined(INCLUDE_PHY_84756)
#include "phyconfig.h"    /* Must be the first phy include after phydefs.h */
#include "phyident.h"
#include "phyreg.h"
#include "phynull.h"
#include "phyfege.h"
#include "phyxehg.h"

#if defined(INCLUDE_FCMAP)
#include <soc/fcmapphy.h>
#endif
#if defined(INCLUDE_MACSEC)
#include <soc/macsecphy.h>
#endif
#include "phy_mac_ctrl.h"
#include "phy_xmac.h"
#include "phy84756_fcmap_int.h"
#include "phy84756_fcmap.h"
#include "phy84756_i2c.h"

int
_phy_84756_bsc_rw(phy_ctrl_t *pc, int dev_addr, int opr,
                    int addr, int count, void *data_array,buint32_t ram_start)
{
    int rv = SOC_E_NONE;
    int iter = 0;
    buint16_t data16;
    int i;
    int access_type;
    int data_type;

    if (!data_array) {
        return SOC_E_PARAM;
    }

    if (count > PHY84756_BSC_XFER_MAX) {
        return SOC_E_PARAM;
    }

    data_type = PHY84756_I2C_DATA_TYPE(opr);
    access_type = PHY84756_I2C_ACCESS_TYPE(opr);

    if (access_type == PHY84756_I2CDEV_WRITE) {
        for (i = 0; i < count; i++) {
            if (data_type == PHY84756_I2C_8BIT) {
                SOC_IF_ERROR_RETURN
                    (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, ram_start + i,
                          ((buint8_t *)data_array)[i]));
            } else {  /* 16 bit */
                SOC_IF_ERROR_RETURN
                    (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, ram_start + i,
                          ((buint16_t *)data_array)[i]));
            }
        }
    }

    data16 = ram_start;
    SOC_IF_ERROR_RETURN
        (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0x8004, data16));
    SOC_IF_ERROR_RETURN
        (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0x8003, addr));
    SOC_IF_ERROR_RETURN
        (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0x8002, count));

    data16 = 1;
    data16 |= (dev_addr<<9);
    if (access_type == PHY84756_I2CDEV_WRITE) {
        data16 |= PHY84756_WR_FREQ_400KHZ;
    }

    SOC_IF_ERROR_RETURN
        (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0x8005,data16));

    if (access_type == PHY84756_I2CDEV_WRITE) {
        data16 =  0x8000 | PHY84756_BSC_WRITE_OP;
    } else {
        data16 =  0x8000 | PHY84756_BSC_READ_OP;
    }

    if (data_type == PHY84756_I2C_16BIT) {
        data16 |= (1 << 12);
    }

    /* for single port mode, there should be only one I2C interface active
     * from lane0. The 0x800x register block is bcst type registers. If writing
     * to 0x8000 directly, it will enable all four I2C masters. Use indirect access
     * to enable only the lane 0.
     */

    SOC_IF_ERROR_RETURN
        (BFCMAP_WR_PHY84756_LN_DEV1_PMDr(pc, 0x8000, data16));

    while (iter < 100000) {
        rv = BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, 0x8000, &data16);
        if (((data16 & PHY84756_2W_STAT) == PHY84756_2W_STAT_COMPLETE)) {
            break;
        }
        iter++;
    }

    /* need some delays */
    sal_usleep(10000);

    SOC_DEBUG_PRINT((DK_PHY,"BSC command status %d\n",(data16 & PHY84756_2W_STAT)));

    if (access_type == PHY84756_I2CDEV_WRITE) {
        return SOC_E_NONE;
    }

    if ((data16 & PHY84756_2W_STAT) == PHY84756_2W_STAT_COMPLETE) {
        for (i = 0; i < count; i++) {
            SOC_IF_ERROR_RETURN
                (BFCMAP_RD_PHY84756_LN_DEV1_PMDr(pc, (ram_start+i), &data16));
            if (data_type == PHY84756_I2C_16BIT) {
                ((buint16_t *)data_array)[i] = data16;
                SOC_DEBUG_PRINT((DK_PHY,"%04x ", data16));
            } else {
                ((buint8_t *)data_array)[i] = (buint8_t)data16;
                SOC_DEBUG_PRINT((DK_PHY,"%02x ", data16));
            }
        }
    }
    return SOC_E_NONE;
}

/*
 * Read a slave device such as NVRAM/EEPROM connected to the 84740's I2C
 * interface. This function will be mainly used for diagnostic or workaround
 * purpose.
 * Note:
 * The size of read_array buffer must be greater than or equal to the
 * parameter nbytes.
 * usage example:
 *   Retrieve the first 100 byte data of the non-volatile storage device with
 *   I2C bus device address 0x50(default SPF eeprom I2C address) on unit 0,
 *   port 2.
 *   buint8_t data8[100];
 *   phy_84740_i2cdev_read(0,2,0x50,0,100,data8);
 */
int
phy_84756_i2cdev_read(phy_ctrl_t *pc,
                     int dev_addr,  /* 7 bit I2C bus device address */
                     int offset,    /* starting data address to read */
                     int nbytes,    /* number of bytes to read */
                     buint8_t *read_array)   /* buffer to hold retrieved data */
{
    return _phy_84756_bsc_rw(pc, dev_addr,PHY84756_I2CDEV_READ,
             offset, nbytes, (void *)read_array,PHY84756_READ_START_ADDR);

}

/*
 * Write to a slave device such as NVRAM/EEPROM connected to the 84740's I2C
 * interface. This function will be mainly used for diagnostic or workaround
 * purpose.
 * Note:
 * The size of write_array buffer should be equal to the parameter nbytes.
 * The EEPROM may limit the maximun write size to 16 bytes
 * usage example:
 *   Write to first 100 byte space of the non-volatile storage device with
 *   I2C bus device address 0x50(default SPF eeprom I2C address) on unit 0,
 *   port 2, with written data specified in array data8.
 *   buint8_t data8[100];
 *   *** initialize the data8 array with written data ***
 *
 *   phy_84740_i2cdev_write(0,2,0x50,0,100,data8);
 */

int
phy_84756_i2cdev_write(phy_ctrl_t *pc,
                     int dev_addr,  /* I2C bus device address */
                     int offset,    /* starting data address to write to */
                     int nbytes,    /* number of bytes to write */
                     buint8_t *write_array)   /* buffer to hold written data */
{
    int j;
    int rv = SOC_E_NONE;

    for (j = 0; j < (nbytes/PHY84756_BSC_WR_MAX); j++) {
        rv = _phy_84756_bsc_rw(pc, dev_addr,PHY84756_I2CDEV_WRITE,
                    offset + j * PHY84756_BSC_WR_MAX, PHY84756_BSC_WR_MAX,
                    (void *)(write_array + j * PHY84756_BSC_WR_MAX),
                    PHY84756_WRITE_START_ADDR);
        if (rv != SOC_E_NONE) {
            return rv;
        }
        sal_usleep(20000);
    }
    if (nbytes%PHY84756_BSC_WR_MAX) {
        rv = _phy_84756_bsc_rw(pc, dev_addr,PHY84756_I2CDEV_WRITE,
                offset + j * PHY84756_BSC_WR_MAX, nbytes%PHY84756_BSC_WR_MAX,
                (void *)(write_array + j * PHY84756_BSC_WR_MAX),
                PHY84756_WRITE_START_ADDR);
    }
    return rv;
}
#else /* INCLUDE_PHY_84756 */
int _phy_84756_fcmap_not_empty;
#endif /* INCLUDE_PHY_84756 */
#endif /* INCLUDE_FCMAP */

