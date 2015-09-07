/*
 * $Id: smbus.c 1.11 Broadcom SDK $
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
 * SMB driver interface for I2C controller operating in master mode
 * onboard the CMIC.
 * Some devices use the SMBus (System Management Bus) protocol, which
 * is a subset of commands from the I2C protocol. Fortunately, many
 * devices use the same subset, which makes it possible to derive
 * the following SMBus operations/commands.
 *
 * This module provides for SMB r/w byte and word commands based on
 * top of the bus driver module. Note that locking is performed in
 * this module, such that all drivers which use SMB commands are
 * automatically MT-safe.
 *
 * When possible, try to use the SMB commands for developing your I2C
 * device driver, as this will simplify debugging and development.
 *
 * Below is a list of SMBus commands, all of the commands are based
 * on I2C protocol concepts.
 *
 * Key to symbols
 * ==============
 *
 * S     (1 bit) : Start bit
 * P     (1 bit) : Stop bit
 * Rd/Wr (1 bit) : Read/Write bit. Rd equals 1, Wr equals 0.
 * A, NA (1 bit) : ACK and Not ACK (NACK)  bit.
 * Addr  (7 bits): I2C 7 bit address. Note that this can be expanded
 *                 as usual to get a 10 bit I2C address.
 * Comm  (8 bits): Command byte, a data byte which often selects
 *                 a register or function on  the device.
 * Data  (8 bits): A plain data byte. Sometimes, I write DataLow,
 *                 DataHigh for 16 bit data.
 * Count (8 bits): A data byte containing the length of a block
 *                 operation.
 *
 * [..]: Data sent by I2C device, as opposed to data sent by the
 *       host adapter.
 *
 * See also: bus.c
 */
#include <sal/types.h>
#include <soc/error.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/i2c.h>
#ifdef BCM_CMICM_SUPPORT
#include <soc/cmicm.h>
#endif

#define I2C_MORE_TO_READ TRUE
#define I2C_FINAL_READ   FALSE
#ifdef BCM_CMICM_SUPPORT
/* Returns OK or TIMEOUT. Calling routine has to check for ACK/NAK?Error */ 
int
smbus_start_wait(int unit)
{
    int rv = SOC_E_TIMEOUT;
    soc_timeout_t to;
    uint32 rval, status;

    /* Start Transaction */
    READ_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit, &rval);
    soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr, &rval, MASTER_START_BUSY_COMMANDf, 1);
    WRITE_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit,rval);

    soc_timeout_init(&to, 10000, 1000);
    
    do {
        READ_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit, &rval);
        if (soc_reg_field_get(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr,rval, MASTER_START_BUSY_COMMANDf) == 0) {
            rv = SOC_E_NONE;
            break;
        }
    } while(!(soc_timeout_check(&to)));

    READ_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit, &rval);
    status = soc_reg_field_get(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr,rval, MASTER_STATUSf);

    if ((rv == 0) && (status == 0)) {
        rv = SOC_E_NONE;
    } else {
        READ_CMIC_I2CM_SMBUS_MASTER_FIFO_CONTROLr(unit,&rval);
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_FIFO_CONTROLr, &rval, MASTER_TX_FIFO_FLUSHf, 1);
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_FIFO_CONTROLr, &rval, MASTER_RX_FIFO_FLUSHf, 1);
        WRITE_CMIC_I2CM_SMBUS_MASTER_FIFO_CONTROLr(unit,rval);
        if (rv != SOC_E_TIMEOUT) {
            rv = SOC_E_INTERNAL;
        }
    }
    return rv;
}

int
smbus_quick_command(int unit, uint8 saddr)
{
    int rv = SOC_E_NONE;
    uint32 rval;

    soc_cm_debug(DK_I2C, "i2c%d: smbus_quick_command @ %02x\n", unit, saddr);
    
    I2C_LOCK(unit);

    rval = SOC_I2C_TX_ADDR(saddr);
    soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr, &rval, MASTER_WR_STATUSf, 1); /* Last Byte */
    WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);

    rval = 0;
    soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr, &rval, SMBUS_PROTOCOLf, SMBUS_QUICK_CMD);
    WRITE_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit,rval);

    rv = smbus_start_wait(unit);

    I2C_UNLOCK(unit);
    return rv;
}

#endif

/*
 * Function: soc_i2c_write_byte
 *
 * Purpose: SMBus Write Quick/Byte
 *
 * This sends a single byte to the device.
 *
 *
 * Algorithm: S Addr Wr [A] Data P
 *
 * Parameters:
 *    unit - StrataSwitch device number or I2C bus number
 *    saddr - device base I2C address (datasheet).
 *    data - 8 bit device-specific data value
 *
 * Returns:
 *    SOC_E_TIMEOUT - the device can not be contacted or is offline.
 *    SOC_E_BUSY - the device timed out or is busy.
 *    SOC_E_NONE - no error, operation succeeded.
 *
 * Notes:
 *    none
 */
int
soc_i2c_write_byte(int unit, i2c_saddr_t saddr, uint8 data)
{
    int rv = SOC_E_NONE;
#ifdef BCM_CMICM_SUPPORT
    uint32 rval;
#endif

    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_byte @ %02x\n", unit, saddr);

#ifdef BCM_CALADAN3_SVK
    if (unit == -1) {
        int datum = data;
        return cpu_i2c_write(saddr, 0, CPU_I2C_ALEN_NONE_DLEN_BYTE, datum);
    }
#endif
    /* Lock all access to I2C */
    I2C_LOCK(unit);

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) && !SOC_IS_SAND(unit)) {
        soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_byte: saddr 0x%02x, data 0x%02x\n",
                     unit, (int) saddr, (int) data);

        rval = SOC_I2C_TX_ADDR(saddr);
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);

        rval = data;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr, &rval, MASTER_WR_STATUSf, 1); /* Last Byte */
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        
        rval = 0;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr, &rval, SMBUS_PROTOCOLf, SMBUS_SEND_BYTE);
        WRITE_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit,rval);
        rv = smbus_start_wait(unit);
    } else
#endif
    {
        if ( (rv = soc_i2c_start(unit, SOC_I2C_TX_ADDR(saddr) ) ) < 0 ) {
            soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_byte:"
                         " failed to generate start.\n",
                           unit);
            I2C_UNLOCK(unit);
            return rv;
        }

        if ( (rv = soc_i2c_write_one_byte(unit, data) ) < 0 ) {
            soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_byte:"
                         " failed to write data byte.\n",
                         unit);
        }

        soc_i2c_stop(unit);
    }
    I2C_UNLOCK(unit);
    return rv;
}

/*
 * Function: soc_i2c_read_byte
 *
 * Purpose: SMBus Read Quick/Byte
 *
 * This Reads a single byte from the device
 *
 * Algorithm: S Addr Rd [A] [Data] NA P
 *
 * Parameters:
 *    unit - StrataSwitch device number or I2C bus number
 *    saddr - device base I2C address (datasheet).
 *    data - 8 bit device-specific data value to read into.
 *
 * Returns:
 *    SOC_E_TIMEOUT - the device can not be contacted or is offline.
 *    SOC_E_INTERNAL - Unexpected or internal error
 *    SOC_E_NONE - no error, operation succeeded.
 *
 * Notes:
 *    none
 */
int
soc_i2c_read_byte(int unit, i2c_saddr_t saddr, uint8* data)
{
    int rv;
#ifdef BCM_CMICM_SUPPORT
    uint32 rval;
#endif

    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_byte @ %02x\n", unit, saddr);

#ifdef BCM_CALADAN3_SVK
    if (unit == -1) {
        int datum = 0;
        rv = cpu_i2c_read(saddr, 0, CPU_I2C_ALEN_NONE_DLEN_BYTE, &datum);
        *data = (char)datum;
        return rv;
    }
#endif

    I2C_LOCK(unit);
#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) && !SOC_IS_SAND(unit)) {
        rval = SOC_I2C_RX_ADDR(saddr);
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr, &rval, MASTER_WR_STATUSf, 1); /* Last Byte */
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);

        rval = 0;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr, &rval, SMBUS_PROTOCOLf, SMBUS_RECEIVE_BYTE);
        WRITE_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit,rval);
        rv = smbus_start_wait(unit);
        if (rv == SOC_E_NONE) {
            READ_CMIC_I2CM_SMBUS_MASTER_DATA_READr(unit, &rval);
            *data = (uint8) (rval & 0xff);
        }
        soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_byte: saddr 0x%02x, data 0x%02x\n",
                     unit, (int) saddr, (int) *data);

    } else
#endif
    {
        if ((rv = soc_i2c_start(unit, SOC_I2C_RX_ADDR(saddr))) < 0) {
            soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_byte:"
                         " failed to generate start.\n",
                         unit);
            I2C_UNLOCK(unit);
            return rv;
        }

        if ((rv = soc_i2c_read_one_byte(unit, data, 
                                        I2C_FINAL_READ)) < 0) {
            soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_byte:"
                         " failed to read data byte.\n",
                         unit);
        }

        soc_i2c_stop(unit);
    }
    I2C_UNLOCK(unit);
    return rv;
}



/*
 * Function: soc_i2c_write_word
 *
 * Purpose: SMBus Write Word
 *
 * This command writes a single word to an I2C device.
 *
 * Algorithm: S Addr Wr [A] DataHigh [A] DataLow [A] P
 *
 * Parameters:
 *
 *    unit - StrataSwitch device number or I2C bus number
 *    saddr - device base I2C address (datasheet).
 *    value - 16 bit data value to write
 *
 * Returns:
 *
 *    SOC_E_TIMEOUT - the device can not be contacted or is offline.
 *    SOC_E_BUSY - the device timed out or is busy.
 *    SOC_E_INTERNAL - Unexpected or internal error
 *    SOC_E_NONE - no error, operation succeeded.
 *
 * Notes:
 *    None
 */
int
soc_i2c_write_word(int unit, i2c_saddr_t saddr, uint16 value)
{
    int rv = SOC_E_NONE;
    uint8 b0 = (uint8) (value & 0x00ff);
    uint8 b1 = (uint8) ((value & 0xff00) >> 8);
#ifdef BCM_CMICM_SUPPORT
    uint32 rval;
#endif

    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_word @ %02x\n", unit, saddr);

#ifdef BCM_CALADAN3_SVK
    if (unit == -1) {
        int datum = value;
        rv = cpu_i2c_write(saddr, 0, CPU_I2C_ALEN_NONE_DLEN_WORD, datum);
        return rv;
    }
#endif

    I2C_LOCK(unit);
#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) && !SOC_IS_SAND(unit)) {
        /* Write Word is achieved with SMBUS_WRITE_BYTE, with LSB of data instead of teh command */
        rval = SOC_I2C_TX_ADDR(saddr);
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);

        rval = b1;
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        rval = b0;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr, &rval, MASTER_WR_STATUSf, 1); /* Last Byte */
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        
        rval = 0;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr, &rval, SMBUS_PROTOCOLf, SMBUS_WRITE_BYTE);
        WRITE_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit,rval);
        rv = smbus_start_wait(unit);
    } else
#endif
    {
        if ( (rv = soc_i2c_start(unit, SOC_I2C_TX_ADDR(saddr) ) ) < 0 ) {
    	soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_word_data: "
    		     "failed to generate start.\n",
    		     unit);
    	I2C_UNLOCK(unit);
    	return rv;
        }

        do {
            if ( (rv = soc_i2c_write_one_byte(unit, b1) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_word_data: "
    			 "failed to send data MSB.\n",
    			 unit);
    	    break;
    	}
    	
    	if ( (rv = soc_i2c_write_one_byte(unit, b0) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_word_data: "
    			 "failed to send data LSB.\n",
    			 unit);
    	}
        } while (0);

        soc_i2c_stop(unit);
    }
    I2C_UNLOCK(unit);
    return rv;
}

/*
 *
 * Function: soc_i2c_read_word
 *
 * Purpose: SMBus Read Word
 *          Reads a single word from the device. 
 *
 * Algorithm: S Addr Rd [A] [Data] [A] [Data] NA P
 *
 * Parameters:
 *    unit - StrataSwitch device number or I2C bus number
 *    saddr - device base I2C address (datasheet).
 *    value - 16 bit device-specific data value to read.
 *
 * Returns:
 *
 *    SOC_E_TIMEOUT - the device can not be contacted or is offline.
 *    SOC_E_INTERNAL - Unexpected or internal error
 *    SOC_E_NONE - no error, operation succeeded.
 *
 * Notes:
 *    none
 */
int
soc_i2c_read_word(int unit, i2c_saddr_t saddr, uint16* value)
{
    int rv = SOC_E_NONE;
#ifdef BCM_CMICM_SUPPORT
    uint8 b0, b1;
    uint32 rval;
#endif

    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_word @ %02x\n", unit, saddr);

#ifdef BCM_CALADAN3_SVK
    if (unit == -1) {
        int datum = 0;
        rv = cpu_i2c_read(saddr, 0, CPU_I2C_ALEN_NONE_DLEN_WORD, &datum);
        *value = (uint16)datum;
        return rv;
    }
#endif

    I2C_LOCK(unit);
#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) && !SOC_IS_SAND(unit)) {
        rval = SOC_I2C_RX_ADDR(saddr);
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr, &rval, MASTER_WR_STATUSf, 1); /* Last Byte */
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);

        rval = 0;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr, &rval, SMBUS_PROTOCOLf, SMBUS_READ_WORD);
        WRITE_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit,rval);
        rv = smbus_start_wait(unit);
        if (rv == SOC_E_NONE) {
            READ_CMIC_I2CM_SMBUS_MASTER_DATA_READr(unit, &rval);
            b0 = (uint8) (rval & 0xff);
            READ_CMIC_I2CM_SMBUS_MASTER_DATA_READr(unit, &rval);
            b1 = (uint8) (rval & 0xff);
            rval = (b1 << 8) + b0;
            *value = (uint16) rval;
        }
    } else
#endif
    {
        if ((rv = soc_i2c_start(unit, SOC_I2C_RX_ADDR(saddr))) < 0) {
            soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_word: "
    		     "failed to generate start.\n",
    		     unit);
    	I2C_UNLOCK(unit);
    	return rv;
        }

        if ( (rv = soc_i2c_read_short(unit, value, 
    				  I2C_FINAL_READ)) < 0 ) {
            soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_word: "
    		     "failed to read data word.\n",
    		     unit);
        }

        soc_i2c_stop(unit);
    }
    I2C_UNLOCK(unit);
    return rv;
}

/*
 * Function: soc_i2c_read_byte_data
 *
 * Purpose: SMBus read byte
 *
 * This reads a single byte from a device, from a designated register.
 * The register is specified through the Comm byte.
 *
 * Algorithm: S Addr Wr [A] Comm [A] S Addr Rd [A] [Data] NA P
 *
 * Parameters:
 *
 *    unit - StrataSwitch device number or I2C bus number
 *    saddr - device base I2C address (datasheet).
 *    com - 8 bit device address or com port
 *    value - address of where data byte should be stored
 *
 * Returns: data byte in value parameter and one of the following:
 *
 *    SOC_E_TIMEOUT - the device can not be contacted or is offline.
 *    SOC_E_INTERNAL - Unexpected or internal error
 *    SOC_E_NONE - no error, operation succeeded.
 *
 * Notes:
 *    none
 */
int
soc_i2c_read_byte_data(int unit, i2c_saddr_t saddr, uint8 com, uint8* value)
{
    int rv = SOC_E_NONE;
#ifdef BCM_CMICM_SUPPORT
    uint32 rval;
#endif

    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_byte_data @ %02x\n", unit, saddr);

#ifdef BCM_CALADAN3_SVK
    if (unit == -1) {
        int datum = 0;
        rv = cpu_i2c_read(saddr, com, CPU_I2C_ALEN_BYTE_DLEN_BYTE, &datum);
        *value = (char)datum;
        return rv;
    }
#endif

    /* Lock all access to I2C */
    I2C_LOCK(unit);

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) && !SOC_IS_SAND(unit)) {
        rval = SOC_I2C_TX_ADDR(saddr);
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        rval = com;
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        rval = SOC_I2C_RX_ADDR(saddr);
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr, &rval, MASTER_WR_STATUSf, 1); /* Last Byte */
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);

        rval = 0;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr, &rval, SMBUS_PROTOCOLf, SMBUS_READ_BYTE);
        WRITE_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit,rval);
        rv = smbus_start_wait(unit);
        if (rv == SOC_E_NONE) {
            READ_CMIC_I2CM_SMBUS_MASTER_DATA_READr(unit, &rval);
            *value = (uint8) (rval & 0xff);
        }
    } else
#endif
    {
        if ( (rv = soc_i2c_start(unit, SOC_I2C_TX_ADDR(saddr) ) ) < 0 ) {
    	soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_byte_data: "
    		     "failed to generate start.\n",
    		     unit);
    	I2C_UNLOCK(unit);
    	return rv;
        }

        do {
            if ( (rv = soc_i2c_write_one_byte(unit, com) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_byte_data: "
    			 "failed to send com byte.\n",
    			 unit);
    	    break;
    	}
    	if( (rv = soc_i2c_rep_start(unit, SOC_I2C_RX_ADDR(saddr) ) ) < 0 ){
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_byte_data: "
    			 "failed to gen rep start.\n",
    			 unit);
    	    break;
    	}
    	if ( (rv = soc_i2c_read_one_byte(unit, value, 
    					 I2C_FINAL_READ)) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_byte_data: "
    			 "failed to read data byte.\n",
    			 unit);
    	}
        } while (0);

        soc_i2c_stop(unit);
    }
    I2C_UNLOCK(unit);
    return rv;
}

/*
 * Function: soc_i2c_write_byte_data
 *
 * Purpose: SMBus write byte
 *
 * This writes a single byte to a device, to a designated register. The
 * register is specified through the Comm byte. This is the opposite of
 * the Read Byte Data command.
 *
 * Algorithm: S Addr Wr [A] Comm [A] Data [A] P
 *
 * Parameters:
 *
 *    unit - StrataSwitch device number or I2C bus number
 *    saddr - device base I2C address (datasheet).
 *    com - 8 bit device address or com port
 *    value - data value to write
 *
 * Returns:
 *
 *    SOC_E_TIMEOUT - the device can not be contacted or is offline.
 *    SOC_E_INTERNAL - Unexpected or internal error
 *    SOC_E_NONE - no error, operation succeeded.
 *
 * Notes:
 *    none
 */
int
soc_i2c_write_byte_data(int unit, i2c_saddr_t saddr, uint8 com, uint8 value)
{
    int rv = SOC_E_NONE;
#ifdef BCM_CMICM_SUPPORT
    uint32 rval;
#endif

    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_byte_data @ %02x\n", unit, saddr);

#ifdef BCM_CALADAN3_SVK
    if (unit == -1) {
        int datum = value;
        rv = cpu_i2c_write(saddr, com, CPU_I2C_ALEN_BYTE_DLEN_BYTE, datum);
        return rv;
        
    }
#endif

    I2C_LOCK(unit);

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) && !SOC_IS_SAND(unit)) {
        rval = SOC_I2C_TX_ADDR(saddr);
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        rval = com;
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        rval = value;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr, &rval, MASTER_WR_STATUSf, 1); /* Last Byte */
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        
        rval = 0;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr, &rval, SMBUS_PROTOCOLf, SMBUS_WRITE_BYTE);
        WRITE_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit,rval);
        rv = smbus_start_wait(unit);
    } else
#endif
    {
        if ( (rv = soc_i2c_start(unit, SOC_I2C_TX_ADDR(saddr) ) ) < 0 ) {
    	soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_byte_data: "
    		     "failed to generate start.\n",
    		     unit);
    	I2C_UNLOCK(unit);
    	return rv;
        }

        do {
            if ( (rv = soc_i2c_write_one_byte(unit, com) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_byte_data: "
    			 "failed to send com byte.\n",
    			 unit);
    	    break;
    	}
    	if ( (rv = soc_i2c_write_one_byte(unit, value) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_byte_data: "
    			 "failed to send data byte.\n",
    			 unit);
    	}
        } while (0);

        soc_i2c_stop(unit);
    }
    I2C_UNLOCK(unit);
    return rv;
}

/*
 * Function: soc_i2c_read_word_data
 *
 * Purpose: SMBus Read Word Data
 *
 * This command is very like Read Byte Data; again, data is read from a
 * device, from a designated register that is specified through the Comm
 * byte. But this time, the data is a complete word (16 bits).
 *
 * Algorithm:
 *     S Addr Wr [A] Comm [A] S Addr Rd [A] [DataLow] A [DataHigh] NA P
 *
 * Parameters:
 *
 *    unit - StrataSwitch device number or I2C bus number
 *    saddr - device base I2C address (datasheet).
 *    com - 8 bit device address or com port
 *    value - address where 16bit data value should be stored.
 *
 * Returns: 16bit data value on success, and one of the following :
 *
 *    SOC_E_TIMEOUT - the device can not be contacted or is offline.
 *    SOC_E_INTERNAL - Unexpected or internal error
 *    SOC_E_NONE - no error, operation succeeded.
 *
 * Notes:
 *   None
 */
int
soc_i2c_read_word_data(int unit, i2c_saddr_t saddr, uint8 com, uint16* value)
{
    int rv = SOC_E_NONE;

#ifdef BCM_CALADAN3_SVK
    if (unit == -1) {
        int datum = 0;
        rv = cpu_i2c_read(saddr, com, CPU_I2C_ALEN_BYTE_DLEN_WORD, &datum);
        *value = (uint16)datum;
        return rv;
    }
#endif
    I2C_LOCK(unit);

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) && !SOC_IS_SAND(unit)) {
        soc_cm_print("Yet to Implement soc_i2c_read_word_data\n");
    } else
#endif
    {
        if ( (rv = soc_i2c_start(unit, SOC_I2C_TX_ADDR(saddr) ) ) < 0 ) {
    	soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_word_data: "
    		     "failed to generate start.\n",
    		     unit);
    	I2C_UNLOCK(unit);
    	return rv;
        }

        do {
            if ( (rv = soc_i2c_write_one_byte(unit, com) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_word_data: "
    			 "failed to send com byte.\n",
    			 unit);
    	    break;
    	}
    	if( (rv = soc_i2c_rep_start(unit, SOC_I2C_RX_ADDR(saddr) ) ) < 0 ){
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_word_data: "
    			 "failed to gen rep start.\n",
    			 unit);
    	    break;
    	}
    	if( (rv = soc_i2c_read_short(unit, value, 
    				     I2C_FINAL_READ)) < 0){
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_read_word_data: "
    			 "failed to read data word.\n",
    			 unit);
    	}
        } while (0);

        soc_i2c_stop(unit);
    }
    I2C_UNLOCK(unit);
    return rv;
}

/*
 * Function: soc_i2c_write_word_data
 *
 * Purpose: SMBus Write Word Data
 *
 * This is the opposite operation of the Read Word Data command. 16 bits
 * of data is read from a device, from a designated register that is
 * specified through the Comm byte.
 *
 * Algorithm: S Addr Wr [A] Comm [A] DataLow [A] DataHigh [A] P
 *
 * Parameters:
 *
 *    unit - StrataSwitch device number or I2C bus number
 *    saddr - device base I2C address (datasheet).
 *    com - 8 bit device address or com port
 *    value - 16 bit data value to write
 *
 * Returns:
 *
 *    SOC_E_TIMEOUT - the device can not be contacted or is offline.
 *    SOC_E_INTERNAL - Unexpected or internal error
 *    SOC_E_NONE - no error, operation succeeded.
 *
 * Notes:
 *    None
 */
int
soc_i2c_write_word_data(int unit, i2c_saddr_t saddr, uint8 com, uint16 value)
{
    int rv = SOC_E_NONE;
    uint8 b0 = (uint8) (value & 0x00ff);
    uint8 b1 = (uint8) ((value & 0xff00) >> 8);

#ifdef BCM_CALADAN3_SVK
    if (unit == -1) {
        int datum = value;
        rv = cpu_i2c_write(saddr, com, CPU_I2C_ALEN_BYTE_DLEN_WORD, datum);
        return rv;
    }
#endif

    I2C_LOCK(unit);

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) && !SOC_IS_SAND(unit)) {
        soc_cm_print("Yet to Implement soc_i2c_write_word_data\n");
    } else
#endif
    {
        if ( (rv = soc_i2c_start(unit, SOC_I2C_TX_ADDR(saddr) ) ) < 0 ) {
    	soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_word_data: "
    		     "failed to generate start.\n",
    		     unit);
    	I2C_UNLOCK(unit);
    	return rv;
        }

        do {
            if ( (rv = soc_i2c_write_one_byte(unit, com) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_word_data: "
    			 "failed to send com byte.\n",
    			 unit);
    	    break;
    	}
    	if ( (rv = soc_i2c_write_one_byte(unit, b0) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_word_data: "
    			 "failed to send data LSB.\n",
    			 unit);
    	    break;
    	}

    	if ( (rv = soc_i2c_write_one_byte(unit, b1) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_write_word_data: "
    			 "failed to send data MSB.\n",
    			 unit);
    	}
        } while (0);

        soc_i2c_stop(unit);
    }
    I2C_UNLOCK(unit);
    return rv;
}

/*
 * Function: soc_i2c_block_read
 *
 * Purpose: SMBus Block Read
 *
 * This command reads a block of up to 32 bytes from a device, from a
 * designated register that is specified through the Comm byte.
 * The amount of data is specified by the device in the Count byte.
 *
 * Algorithm:
 *         S Addr Wr [A] Comm [A]
 *         S Addr Rd [A] [Count] A [Data] A [Data] A ... A [Data] NA P
 *
 * Parameters:
 *
 *    unit - StrataSwitch device number or I2C bus number
 *    saddr - device base I2C address (datasheet).
 *    com - 8 bit device address or com port
 *    count - address where number of bytes read is stored.
 *    data - address where data buffer should be stored.
 *
 * Returns:
 *    count - updated with the number of bytes successfullly read.
 *
 *    SOC_E_TIMEOUT - the device can not be contacted or is
 *                        offline, or the number of bytes specified
 *                        could not be read from the device.
 *
 *    SOC_E_INTERNAL - Unexpected or internal error
 *    SOC_E_NONE - no error, operation succeeded.
 *
 * Notes:
 *    None
 */
int
soc_i2c_block_read(int unit, i2c_saddr_t saddr,
		   uint8 com, uint8* count, uint8* data)
{
    int i, ack, rv = SOC_E_NONE;
    uint8* ptr = NULL;
#ifdef BCM_CMICM_SUPPORT
    uint32 rval;
#endif

soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_block_read %02x bytes @ %02x - %02x\n", unit, (int)*count, saddr, com);

#ifdef BCM_CALADAN3_SVK
    if (unit == -1) {
        return cpu_i2c_block_read(unit, saddr, com, data, count);
    }
#endif

    I2C_LOCK(unit);

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) && !SOC_IS_SAND(unit)) {
        rval = SOC_I2C_TX_ADDR(saddr);
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        rval = com;
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        rval = SOC_I2C_RX_ADDR(saddr);
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr, &rval, MASTER_WR_STATUSf, 1); /* Last Byte */
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);

        rval = 0;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr, &rval, SMBUS_PROTOCOLf, SMBUS_BLOCK_READ);
        WRITE_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit,rval);
        rv = smbus_start_wait(unit);
        if (rv == SOC_E_NONE) {
            READ_CMIC_I2CM_SMBUS_MASTER_DATA_READr(unit, &rval);
            *count = (uint8) (rval & 0xff);
            ptr = data;
            for( i = 0; i < *count; i++, ptr++) {
                READ_CMIC_I2CM_SMBUS_MASTER_DATA_READr(unit, &rval);
                *ptr = (uint8) (rval & 0xff);
            }
        }
    } else
#endif
    {
        if ( (rv = soc_i2c_start(unit, SOC_I2C_TX_ADDR(saddr) ) ) < 0 ) {
    	soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_block_read: "
    		     "failed to generate start.\n",
    		     unit);
    	I2C_UNLOCK(unit);
    	return rv;
        }

        do {
            if ( (rv = soc_i2c_write_one_byte(unit, com) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_block_read: "
    			 "failed to send com byte.\n",
    			 unit);
    	    break;
    	}

    	if( (rv = soc_i2c_rep_start(unit, SOC_I2C_RX_ADDR(saddr))) 
    	    < 0 ){
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_block_read: "
    			 "failed to gen rep start.\n",
    			 unit);
    	    break;
    	}
    	/* Read expected byte count from chip */
    	if ( (rv = soc_i2c_read_one_byte(unit, count, 
    					 I2C_MORE_TO_READ)) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_block_read: "
    			 "failed to read data count byte.\n",
    			 unit);
    	    break;
    	}
    	ptr = data;
    	for( i = 0; i < *count; i++, ptr++) {
    	    if (i == (*count - 1)) {
    	        ack = I2C_FINAL_READ;
    	    }
    	    else {	
    	        ack = I2C_MORE_TO_READ;
    	    }
    	    if ( (rv = soc_i2c_read_one_byte(unit, ptr, ack) ) < 0 ) {
    	        soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_block_read: "
    			     "failed to read data byte %d.\n",
    			     unit, i);
    		*count = i; /* actual number read */
    		break; /* out of for-loop, fall out of while */
    	    }
    	}
        } while (0);

        soc_i2c_stop(unit);
    }
    I2C_UNLOCK(unit);
    return rv;
}

/*
 * Function: soc_i2c_block_write
 *
 * Purpose: SMBus Block Write
 *
 * The opposite of the Block Read command, this writes upto 32 bytes to
 * a device, to a designated register that is specified through the
 * Comm byte. The amount of data is specified in the Count byte.
 *
 * Algorithm: S Addr Wr [A] Comm [A]
 *            Count [A] Data [A] Data [A] ... [A] Data [A] P
 *
 *
 * Parameters:
 *
 *    unit - StrataSwitch device number or I2C bus number
 *    saddr - device base I2C address (datasheet).
 *    com - 8 bit device address or com port
 *    count - number of bytes to write from data buff.
 *    data - address of data to write (data buffer).
 *
 * Returns:
 *
 *    SOC_E_TIMEOUT - the device can not be contacted or is
 *                        offline, or the specified number of bytes
 *                        could not be written to the device.
 *
 *    SOC_E_NONE - no error, operation succeeded.
 *
 * Notes:
 *    None
 */
int
soc_i2c_block_write(int unit, i2c_saddr_t saddr,
		    uint8 com, uint8 count, uint8* data)
{
    int i,rv = SOC_E_NONE;
    uint8* ptr = NULL;
#ifdef BCM_CMICM_SUPPORT
    uint32 rval;
#endif

soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_block_write %02x bytes @ %02x - %02x\n", unit, count, saddr, com);

#ifdef BCM_CALADAN3_SVK
    if (unit == -1) {
        return cpu_i2c_block_write(unit, saddr, com, data, count);
    }
#endif

    I2C_LOCK(unit);

#ifdef BCM_CMICM_SUPPORT
    if(soc_feature(unit, soc_feature_cmicm) && !SOC_IS_SAND(unit)) {
        rval = SOC_I2C_TX_ADDR(saddr);
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        rval = com;
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        rval = count;
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);

        ptr = data;
        for( i = 0; i < (int) (count - 1); i++, ptr++) {
            rval = (uint32) *ptr;
            WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        }
        rval = (uint32) *ptr;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr, &rval, MASTER_WR_STATUSf, 1); /* Last Byte */
        WRITE_CMIC_I2CM_SMBUS_MASTER_DATA_WRITEr(unit, rval);
        
        rval = 0;
        soc_reg_field_set(unit, CMIC_I2CM_SMBUS_MASTER_COMMANDr, &rval, SMBUS_PROTOCOLf, SMBUS_BLOCK_WRITE);
        WRITE_CMIC_I2CM_SMBUS_MASTER_COMMANDr(unit,rval);
        rv = smbus_start_wait(unit);
    } else
#endif
    {
        if ( (rv = soc_i2c_start(unit, SOC_I2C_TX_ADDR(saddr) ) ) < 0 ) {
    	soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_block_write: "
    		     "failed to generate start.\n",
    		     unit);
    	I2C_UNLOCK(unit);
    	return rv;
        }

        do {
            if ( (rv = soc_i2c_write_one_byte(unit, com) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_block_write: "
    			 "failed to send com byte.\n",
    			 unit);
    	    break;
    	}

    	if ( (rv = soc_i2c_write_one_byte(unit, count) ) < 0 ) {
    	    soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_block_write: "
    			 "failed to send count byte.\n",
    			 unit);
    	    break;
    	}
    	ptr = data;
    	for( i = 0; i < (int) count; i++, ptr++) {
    	    if ( (rv = soc_i2c_write_one_byte(unit, *ptr) ) < 0 ) {
    	        soc_cm_debug(DK_I2C, "i2c%d: soc_i2c_block_write: "
    			     "failed to send byte %d.\n",
    			     unit, i );
    		break;
    	    }
    	}
        } while (0);

        soc_i2c_stop(unit);
    }
    I2C_UNLOCK(unit);
    return rv;
}

