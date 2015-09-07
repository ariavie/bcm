/*
 * $Id: phyi2c.c,v 1.4 Broadcom SDK $
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
 * File:    phyi2c.c
 * Purpose: I2C bus read/write routines for SFP copper PHY devices 
 */

#if defined(BCM_ESW_SUPPORT) && defined(INCLUDE_I2C)
#include <sal/types.h>
#include <soc/drv.h>
#include <soc/phy.h>
#include <soc/i2c.h>
#include <soc/debug.h>

#ifndef BCMSWAP16
#define BCMSWAP16(val) \
        ((uint16)( \
                (((uint16)(val) & (uint16)0x00ffU) << 8) | \
                (((uint16)(val) & (uint16)0xff00U) >> 8) ))
#endif

/* customer specific phy bus read/write hook pointers */
typedef struct {
    soc_phy_bus_rd_t rd;
    soc_phy_bus_wr_t wr;
} soc_phy_bus_hook_t;

STATIC soc_phy_bus_hook_t _soc_phy_bus_hook[SOC_MAX_NUM_DEVICES];
 

/*
 * Function:
 *  phy_i2c_miireg_read
 * Purpose:
 *  Read a MII register of the given PHY device on the I2C bus. Typically it is
 *  a copper SFP PHY device.
 * Parameters:
 *  unit         - StrataSwitch unit #.
 *  phy_id       - the address on the I2C bus.
 *  phy_reg_addr - MII register address.
 *  phy_rd_data  - pointer to a 16bit storage to hold read data.
 * Returns:
 *  SOC_E_XXX
 */

int
phy_i2c_miireg_read (int unit,uint32 phy_id,uint32 phy_reg_addr,
                                             uint16 *phy_rd_data)
{
    int rv = SOC_E_NONE;

    if (_soc_phy_bus_hook[unit].rd) {
        return (*_soc_phy_bus_hook[unit].rd)(unit,phy_id,phy_reg_addr,phy_rd_data);
    }

    /* Make sure that we're already attached, or go get attached */
    if (!soc_i2c_is_attached(unit)) {
        if ((rv = soc_i2c_attach(unit, 0, 0)) <= 0) {
            return rv;
        }
    }
    rv = soc_i2c_read_word_data(unit,phy_id,phy_reg_addr,phy_rd_data);

    if (rv == SOC_E_NONE) {
        *phy_rd_data = BCMSWAP16(*phy_rd_data);
    }
    return rv;
}

/*
 * Function:
 *  phy_i2c_miireg_write
 * Purpose:
 *  Write to a MII register of the given PHY device on the I2C bus. Typically it is
 *  a copper SFP PHY device.
 * Parameters:
 *  unit         - StrataSwitch unit #.
 *  phy_id       - the address on the I2C bus.
 *  phy_reg_addr - MII register address.
 *  phy_wr_data  - 16bit value to write to the MII register.
 * Returns:
 *  SOC_E_XXX
 */

int 
phy_i2c_miireg_write (int unit,uint32 phy_id,uint32 phy_reg_addr, 
                                                uint16 phy_wr_data)
{
    int rv = SOC_E_NONE;

    if (_soc_phy_bus_hook[unit].wr) {
        return (*_soc_phy_bus_hook[unit].wr)(unit,phy_id,phy_reg_addr,phy_wr_data);
    }

    /* Make sure that we're already attached, or go get attached */
    if (!soc_i2c_is_attached(unit)) {
        if ((rv = soc_i2c_attach(unit, 0, 0)) <= 0) {
            return rv;
        }
    }

    rv = soc_i2c_write_word_data(unit,phy_id,phy_reg_addr,BCMSWAP16(phy_wr_data));
    return rv;
}

/*
 * Function:
 *  phy_i2c_bus_func_hook_set
 * Purpose:
 *  Install customer specific I2C bus read/write routines 
 *  
 * Parameters:
 *  unit - StrataSwitch unit #.
 *  rd   - I2C bus read routine.
 *  wr   - I2C bus write routine.
 * Returns:
 *  SOC_E_XXX
 */

int phy_i2c_bus_func_hook_set (int unit, soc_phy_bus_rd_t rd, soc_phy_bus_wr_t wr)
{
    _soc_phy_bus_hook[unit].rd = rd;
    _soc_phy_bus_hook[unit].wr = wr;
    return SOC_E_NONE;
}
#else
int _soc_phy_phyi2c_not_empty;
#endif
