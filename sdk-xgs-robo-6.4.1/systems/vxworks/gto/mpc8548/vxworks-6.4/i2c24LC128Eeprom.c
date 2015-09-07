/*
 * $Id: i2c24LC128Eeprom.c,v 1.7 Broadcom SDK $
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

/* i2c24LC128Eeprom.c - EEPROM NVRAM routines */

/* includes */
#include <vxWorks.h>            /* vxWorks generics */
#include "config.h"
#include "sysMotI2c.h"
#include "i2c24LC128Eeprom.h"

/* defines */

/* globals */

/* locals */

/* forward declarations */

/* externals */

/***************************************************************************
*
* eepromReadByte - read one byte of non-volatile RAM
*
* This routine reads one byte of non-volatile RAM.
*
* RETURNS: One byte of data.
*
* ERRNO
*
* SEE ALSO: eepromWriteByte()
*/

UINT8 eepromReadByte
    (
    int offset
    )
    {
    UINT8 buffer;

    buffer = 0;
    i2cRead(EEPROM_24LC128_SMBUS_CHAN, 
            EEPROM_24LC128_CCR_ADDRESS, 
            I2C_DEVICE_TYPE_EEPROM_24LC128,
            offset, 1, &buffer);
    return buffer;
    }

/***************************************************************************
*
* eepromWriteByte - write one byte to non-volatile RAM
*
* This routine writes one byte of data to non-volatile RAM.
*
* RETURNS: OK or ERROR if write fails
*
* ERRNO
*
* SEE ALSO: eepromReadByte(), eepromUnlock(), eepromLock()
*/

STATUS eepromWriteByte
    (
    int   offset,
    UINT8 data
    )
    {
    UINT8 buffer;
    int   status;
    buffer = 0;

    i2cWrite(EEPROM_24LC128_SMBUS_CHAN, 
            EEPROM_24LC128_CCR_ADDRESS, 
             I2C_DEVICE_TYPE_EEPROM_24LC128,
             offset, 1, &data);

    status = i2cRead(1, I2C_DEVICE_TYPE_EEPROM_24LC128, 
                     I2C_DEVICE_TYPE_EEPROM_24LC128,
                     offset, 1, &buffer);

    if ( status != OK || buffer != data )
        {
        return (ERROR);
        }
    return (OK);
    }


/***************************************************************************
*
* eepromUnlock - Unlock the eeprom via the software protection mechanism
*
* Software unlock mechanism is not supported in Microchip 24LC128 EEPROM
*
* RETURNS: N/A
*
* SEE ALSO: eepromReadByte(), eepromWriteByte(), eepromLock()
*
*/
void eepromUnlock (void)
    {
    return ;
    }

/***************************************************************************
*
* eepromLock - Lock the eeprom via the software protection mechanism
*
* Software lock mechanism is not supported in Microchip 24LC128 EEPROM
*
* RETURNS: N/A
*
* SEE ALSO: eepromReadByte(), eepromWriteByte(), eepromUnlock()
*
*/
void eepromLock (void)
    {
    return ;
    }

#define INCLUDE_I2C_DEBUG
#ifdef INCLUDE_I2C_DEBUG
void eeprom_mac_set(int index, UINT8 mac3, UINT8 mac4, UINT8 mac5) {
    int mac_offset;
    mac_offset = NV_MAC_ADRS_OFFSET + (index * 8);
    eepromWriteByte(mac_offset, 0xff);
    eepromWriteByte(mac_offset + 1, 0xff);
    eepromWriteByte(mac_offset + 2, 0xff);
    eepromWriteByte(mac_offset + 3, mac3);
    eepromWriteByte(mac_offset + 4, mac4);
    eepromWriteByte(mac_offset + 5, mac5);
}

void eeprom_mac_show(int index) {
    int mac0, mac1, mac2, mac3, mac4, mac5;
    int mac_offset;
    mac_offset = NV_MAC_ADRS_OFFSET + (index * 8);
    mac0 = eepromReadByte(mac_offset);
    mac1 = eepromReadByte(mac_offset + 1);
    mac2 = eepromReadByte(mac_offset + 2);
    mac3 = eepromReadByte(mac_offset + 3);
    mac4 = eepromReadByte(mac_offset + 4);
    mac5 = eepromReadByte(mac_offset + 5);
    printf ("MAC address = %02x:%02x:%02x:%02x:%02x:%02x\n",
            mac0, mac1, mac2, mac3, mac4, mac5);
}
#endif
