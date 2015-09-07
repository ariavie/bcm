/*
 * $Id: cfp.c 1.3 Broadcom SDK $
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
 */
#include <assert.h>
#include <soc/types.h>
#include <soc/error.h>
#include <soc/debug.h>
#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/drv_if.h>
#include <soc/cfp.h>

/* Ram select code for northstarplus */
#define CFP_53020_RAM_SEL_RED_STAT 0x18
#define CFP_53020_RAM_SEL_YELLOW_STAT 0x10
#define CFP_53020_RAM_SEL_GREEN_STAT 0x8
#define CFP_53020_RAM_SEL_METER 0x4
#define CFP_53020_RAM_SEL_ACT 0x2
#define CFP_53020_RAM_SEL_TCAM 0x1

/* OP code */
#define CFP_53020_OP_NONE 0x0
#define CFP_53020_OP_READ 0x1
#define CFP_53020_OP_WRITE 0x2
#define CFP_53020_OP_SEARCH_VALID 0x4

/* Meter */
#define CFP_53020_METER_RATE_UNIT       4000
#define CFP_53020_METER_RATE_MAX        (524287 * CFP_53020_METER_RATE_UNIT)
#define CFP_53020_METER_BURST_UNIT      8 /* byte */
#define CFP_53020_METER_BURST_MAX       (1048575 * CFP_53020_METER_BURST_UNIT)





/*
 * Function: _drv_nsp_cfp_meter_rate2chip
 *
 * Purpose:
 *     Translate the driver rate value to register value.
 *
 * Parameters:
 *     unit - BCM device number
 *     kbits_sec - driver rate value
 *     chip_val(OUT) - register value
 *
 * Returns:
 *     Nothing
 */
void
_drv_nsp_cfp_meter_rate2chip(int unit, uint32 kbits_sec, uint32 *chip_val)
{

    if (kbits_sec > CFP_53020_METER_RATE_MAX) {
        return;
    } else {
        *chip_val = ((kbits_sec * 1000) / CFP_53020_METER_RATE_UNIT);
    }
}

/*
 * Function: _drv_nsp_cfp_meter_chip2rate
 *
 * Purpose:
 *     Translate the register value to driver rate value.
 *
 * Parameters:
 *     unit - BCM device number
 *     kbits_sec(OUT) - driver rate value
 *     chip_val - register value
 *
 * Returns:
 *     Nothing
 */
void
_drv_nsp_cfp_meter_chip2rate(int unit, uint32 *kbits_sec, uint32 chip_val)
{
    *kbits_sec = (chip_val * CFP_53020_METER_RATE_UNIT) / 1000;
}

/*
 * Function: _drv_nsp_cfp_meter_burst2chip
 *
 * Purpose:
 *     Translate the driver burst value to register value.
 *
 * Parameters:
 *     unit - BCM device number
 *     kbits_burst - driver burst value
 *     chip_val(OUT) - chip value
 *
 * Returns:
 *     Nothing
 */
void
_drv_nsp_cfp_meter_burst2chip(int unit, uint32 kbits_burst, uint32 *chip_val)
{

    *chip_val = (kbits_burst * 1000) / CFP_53020_METER_BURST_UNIT;
}

/*
 * Function: _drv_nsp_cfp_meter_chip2burst
 *
 * Purpose:
 *     Translate the register value to driver burst value.
 *
 * Parameters:
 *     unit - BCM device number
 *     kbits_burst(OUT) - driver burst value
 *     chip_val - register value
 *
 * Returns:
 *     Nothing
 */
void
_drv_nsp_cfp_meter_chip2burst(int unit, uint32 *kbits_burst, uint32 chip_val)
{
    *kbits_burst = (chip_val * CFP_53020_METER_BURST_UNIT) / 1000;
}


/*
 * Function: _drv_gex_cfp_stat_read
 *
 * Purpose:
 *     Read the counter raw data from chip.
 *
 * Parameters:
 *     unit - BCM device number
 *     counter type - in-band/ out-band
 *     index -entry index
 *     entry(OUT) -counter raw data
 *
 * Returns:
 *     SOC_E_NONE
 *     SOC_E_XXX
 *
 * Note:
 */
int
_drv_nsp_cfp_stat_read(int unit, uint32 counter_type, 
                                uint32 index, uint32 *counter)
{
    int rv = SOC_E_NONE;
    uint32  mem_id  = 0;
    int ram_val, retry;
    uint32 reg_val, fld_val;
    int index_max;

    assert(counter);
    
    switch (counter_type) {
        case DRV_CFP_RAM_STAT_GREEN:
            mem_id = INDEX(CFP_GREEN_STATm);
            ram_val = CFP_53020_RAM_SEL_GREEN_STAT;
            break;
        case DRV_CFP_RAM_STAT_RED:
            mem_id = INDEX(CFP_RED_STATm);
            ram_val = CFP_53020_RAM_SEL_RED_STAT;
            break; 
        case DRV_CFP_RAM_STAT_YELLOW:
            mem_id = INDEX(CFP_YELLOW_STATm);
            ram_val = CFP_53020_RAM_SEL_YELLOW_STAT;
            break;
        default:
            rv = SOC_E_PARAM;
            return rv;
    }

    index_max = soc_robo_mem_index_max(unit, mem_id);
    if (index > index_max) {
        rv = SOC_E_PARAM;
        return rv;
    }

    /*
     * Perform read operation 
     */

    MEM_LOCK(unit, INDEX(CFP_TCAM_IPV4_SCm));
    if (REG_READ_CFP_ACCr(unit, &reg_val) < 0) {
        goto cfp_stat_read_exit;
    }
    fld_val = CFP_53020_OP_READ;
    soc_CFP_ACCr_field_set(unit, &reg_val, 
        OP_SELf, &fld_val);
    
    fld_val = ram_val;
    soc_CFP_ACCr_field_set(unit, &reg_val, 
        RAM_SELf, &fld_val);

    fld_val = index;
    soc_CFP_ACCr_field_set(unit, &reg_val, 
        XCESS_ADDRf, &fld_val);

    fld_val = 1;
    soc_CFP_ACCr_field_set(unit, &reg_val, 
        OP_STR_DONEf, &fld_val);

    if (REG_WRITE_CFP_ACCr(unit, &reg_val) < 0) {
        goto cfp_stat_read_exit;
    }

    /* wait for complete */
    for (retry = 0; retry < SOC_TIMEOUT_VAL; retry++) {
        if (REG_READ_CFP_ACCr(unit, &reg_val) < 0) {
            goto cfp_stat_read_exit;
        }
        soc_CFP_ACCr_field_get(unit, &reg_val, 
            OP_STR_DONEf, &fld_val);
        if (!fld_val) {
            break;
        }
    }
    if (retry >= SOC_TIMEOUT_VAL) {
        rv = SOC_E_TIMEOUT;
        goto cfp_stat_read_exit;
    }
    
    switch (counter_type) {
    case DRV_CFP_RAM_STAT_GREEN:
        if ((rv = REG_READ_STAT_GREEN_CNTRr(unit,&reg_val)) < 0) {
            goto cfp_stat_read_exit;
        }
        break;
    case DRV_CFP_RAM_STAT_YELLOW:
        if ((rv = REG_READ_STAT_YELLOW_CNTRr(unit,&reg_val)) < 0) {
            goto cfp_stat_read_exit;
        }
        break;
    case DRV_CFP_RAM_STAT_RED:
        if ((rv = REG_READ_STAT_RED_CNTRr(unit,&reg_val)) < 0) {
            goto cfp_stat_read_exit;
        }
        break;
    default:
        rv = SOC_E_UNAVAIL;
        goto cfp_stat_read_exit;
    }

    *counter = reg_val;
cfp_stat_read_exit:
        MEM_UNLOCK(unit, INDEX(CFP_TCAM_IPV4_SCm));
    return rv;
}

/*
 * Function: _drv_nsp_cfp_stat_write
 *
 * Purpose:
 *     Set the counter raw data to chip.
 *
 * Parameters:
 *     unit - BCM device number
 *     counter type - in-band/ out-band
 *     index -entry index
 *     entry -counter raw data
 *
 * Returns:
 *     SOC_E_NONE
 *     SOC_E_XXX
 *
 * Note:
 */
int
_drv_nsp_cfp_stat_write(int unit, uint32 counter_type, 
                                      uint32 index, uint32 counter)
{
    int rv = SOC_E_NONE;
    uint32  mem_id  = 0;
    int ram_val, retry;
    uint32 reg_val, fld_val;
    int index_max;
    uint32  data_reg_val;
    
    switch (counter_type) {
        case DRV_CFP_RAM_STAT_GREEN:
            mem_id = INDEX(CFP_GREEN_STATm);
            ram_val = CFP_53020_RAM_SEL_GREEN_STAT;
            break;
        case DRV_CFP_RAM_STAT_YELLOW:
            mem_id = INDEX(CFP_YELLOW_STATm);
            ram_val = CFP_53020_RAM_SEL_YELLOW_STAT;
            break;
        case DRV_CFP_RAM_STAT_RED:
            mem_id = INDEX(CFP_RED_STATm);
            ram_val = CFP_53020_RAM_SEL_RED_STAT;
            break;
        default:
            rv = SOC_E_PARAM;
            return rv;
    }

    index_max = soc_robo_mem_index_max(unit, mem_id);
    if (index > index_max) {
        rv = SOC_E_PARAM;
        return rv;
    }

    /*
     * Perform write operation 
     */

    MEM_LOCK(unit, INDEX(CFP_TCAM_IPV4_SCm));
    if (REG_READ_CFP_ACCr(unit, &reg_val) < 0) {
        goto cfp_stat_write_exit;
    }
    fld_val = CFP_53020_OP_WRITE;
    soc_CFP_ACCr_field_set(unit, &reg_val, 
        OP_SELf, &fld_val);
    
    fld_val = ram_val;
    soc_CFP_ACCr_field_set(unit, &reg_val, 
        RAM_SELf, &fld_val);

    fld_val = index;
    soc_CFP_ACCr_field_set(unit, &reg_val, 
        XCESS_ADDRf, &fld_val);

    fld_val = 1;
    soc_CFP_ACCr_field_set(unit, &reg_val, 
        OP_STR_DONEf, &fld_val);

    /* Set counter value */
    data_reg_val = counter;
    switch (counter_type) {
    case DRV_CFP_RAM_STAT_GREEN:
        if ((rv = REG_WRITE_STAT_GREEN_CNTRr(unit,&data_reg_val)) < 0) {
            goto cfp_stat_write_exit;
        }
        break;
    case DRV_CFP_RAM_STAT_YELLOW:
        if ((rv = REG_WRITE_STAT_YELLOW_CNTRr(unit,&data_reg_val)) < 0) {
            goto cfp_stat_write_exit;
        }
        break;
    case DRV_CFP_RAM_STAT_RED:
        if ((rv = REG_WRITE_STAT_RED_CNTRr(unit,&data_reg_val)) < 0) {
            goto cfp_stat_write_exit;
        }
        break;
    }

    if (REG_WRITE_CFP_ACCr(unit, &reg_val) < 0) {
        goto cfp_stat_write_exit;
    }

    /* wait for complete */
    for (retry = 0; retry < SOC_TIMEOUT_VAL; retry++) {
        if (REG_READ_CFP_ACCr(unit, &reg_val) < 0) {
            goto cfp_stat_write_exit;
        }
        soc_CFP_ACCr_field_get(unit, &reg_val, 
            OP_STR_DONEf, &fld_val);
        if (!fld_val) {
            break;
        }
    }
    if (retry >= SOC_TIMEOUT_VAL) {
        rv = SOC_E_TIMEOUT;
        goto cfp_stat_write_exit;
    }

    cfp_stat_write_exit:
        MEM_UNLOCK(unit, INDEX(CFP_TCAM_IPV4_SCm));
    return rv;
}


/*
 * Function: drv_nsp_cfp_stat_get
 *
 * Purpose:
 *     Get the counter value from chip.
 *
 * Parameters:
 *     unit - BCM device number
 *     stat_type - In-band/Out-band counter/Both
 *     index -entry index
 *     counter(OUT) -counter value
 *
 * Returns:
 *     SOC_E_NONE
 *     SOC_E_XXX
 *
 * Note:
 */
int
drv_nsp_cfp_stat_get(int unit, uint32 stat_type, uint32 index, uint32* counter)
{
    int rv = SOC_E_NONE;
    uint32  index_max, temp;
    
    assert(counter);

    index_max = soc_robo_mem_index_max(unit, INDEX(CFP_GREEN_STATm));
    if (index > index_max) {
        rv = SOC_E_PARAM;
        return rv;
    }
    
    switch (stat_type) {
        case DRV_CFP_STAT_GREEN:
            _drv_nsp_cfp_stat_read(unit, DRV_CFP_RAM_STAT_GREEN, index, 
                counter);
            break;
        case DRV_CFP_STAT_RED:
            _drv_nsp_cfp_stat_read(unit, DRV_CFP_RAM_STAT_RED, index, 
                counter);
            break;
        case DRV_CFP_STAT_YELLOW:
            _drv_nsp_cfp_stat_read(unit, DRV_CFP_RAM_STAT_YELLOW, index, 
                counter);
            break;
        case DRV_CFP_STAT_ALL:
            _drv_nsp_cfp_stat_read(unit, DRV_CFP_RAM_STAT_GREEN, index, 
                &temp);
            *counter = temp;
            _drv_nsp_cfp_stat_read(unit, DRV_CFP_RAM_STAT_RED, index, 
                &temp);
            *counter += temp;
            _drv_nsp_cfp_stat_read(unit, DRV_CFP_RAM_STAT_YELLOW, index, 
                &temp);
            *counter += temp;
            
            break;
        default:
            rv = SOC_E_PARAM;
    }
    return rv;
}

/*
 * Function: drv_nsp_cfp_stat_set
 *
 * Purpose:
 *     Set the CFP counter value to chip.
 *
 * Parameters:
 *     unit - BCM device number
 *     stat_type - In-band/Out-band counter/Both
 *     index -entry index
 *     counter -counter value
 *
 * Returns:
 *     SOC_E_NONE
 *     SOC_E_XXX
 *
 * Note:
 */
int
drv_nsp_cfp_stat_set(int unit, uint32 stat_type, uint32 index, uint32 counter)
{
    int rv = SOC_E_NONE;
    uint32  index_max;

    index_max = soc_robo_mem_index_max(unit, INDEX(CFP_GREEN_STATm));

    if (index > index_max) {
        rv = SOC_E_PARAM;
        return rv;
    }
    
    switch (stat_type) {
        case DRV_CFP_STAT_GREEN:
            _drv_nsp_cfp_stat_write(unit, DRV_CFP_RAM_STAT_GREEN, index, 
                counter);
            break;
        case DRV_CFP_STAT_RED:
            _drv_nsp_cfp_stat_write(unit, DRV_CFP_RAM_STAT_RED, index, 
                counter);
            break;
        case DRV_CFP_STAT_YELLOW:
            _drv_nsp_cfp_stat_write(unit, DRV_CFP_RAM_STAT_YELLOW, index, 
                counter);
            break;         
        case DRV_CFP_STAT_ALL:
            _drv_nsp_cfp_stat_write(unit, DRV_CFP_RAM_STAT_GREEN, index, 
                counter);
            _drv_nsp_cfp_stat_write(unit, DRV_CFP_RAM_STAT_RED, index, 
                counter);
            _drv_nsp_cfp_stat_write(unit, DRV_CFP_RAM_STAT_YELLOW, index, 
                counter);
            break;           
            break;
        default:
            rv = SOC_E_PARAM;
    }
    return rv;
}


int
drv_nsp_cfp_meter_rate_transform(int unit, uint32 kbits_sec, uint32 kbits_burst, 
                uint32 *bucket_size, uint32 *ref_cnt, uint32 *ref_unit)
{
    int rv = SOC_E_NONE;

    if (kbits_sec) {
        if ((kbits_sec > CFP_53020_METER_RATE_MAX)) {
            soc_cm_debug(DK_ERR, 
                "drv_nsp_cfp_meter_rate_transform : rate unsupported. \n");
            rv = SOC_E_UNAVAIL;
            return rv;
        }
        _drv_nsp_cfp_meter_rate2chip(unit, kbits_sec, ref_cnt);
    } else {
        *ref_cnt = 0;
    }

    if (kbits_burst) {
        if ((kbits_burst > CFP_53020_METER_BURST_MAX)) {
            soc_cm_debug(DK_ERR, 
                "drv_nsp_cfp_meter_rate_transform : burst size unsupported. \n");
            rv = SOC_E_UNAVAIL;
            return rv;
        }
        
        _drv_nsp_cfp_meter_burst2chip(unit, kbits_burst, bucket_size);
    } else {
        *bucket_size = 0;
    }

    return rv;
}



