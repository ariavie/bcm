/*
 * $Id: trunk.c 1.3 Broadcom SDK $
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
#include <soc/drv.h>
#include <soc/debug.h>

#define TBX_TRUNK_HASH_FIELD_MACDA_VALUE  0x1
#define TBX_TRUNK_HASH_FIELD_MACSA_VALUE  0x2
#define TBX_TRUNK_HASH_FIELD_IP_MACDA_VALUE  0x4
#define TBX_TRUNK_HASH_FIELD_IP_MACSA_VALUE  0x8

#define TBX_TRUNK_HASH_FIELD_VALID_VALUE  (DRV_TRUNK_HASH_FIELD_MACDA | \
                                           DRV_TRUNK_HASH_FIELD_MACSA | \
                                           DRV_TRUNK_HASH_FIELD_IP_MACDA | \
                                           DRV_TRUNK_HASH_FIELD_IP_MACSA)

static uint32  tbx_default_trunk_seed  = (TBX_TRUNK_HASH_FIELD_MACDA_VALUE | \
                                          TBX_TRUNK_HASH_FIELD_MACSA_VALUE);
/*
 *  Function : _drv_tbx_trunk_enable_set
 *
 *  Purpose :
 *      Enable trunk function (global).
 *
 *  Parameters :
 *      unit    :  RoboSwitch unit number.
 *      enable  :  status of the trunk id.  
 *
 *  Return :
 *      SOC_E_NONE
 *
 *  Note :
 */
static int 
_drv_tbx_trunk_enable_set(int unit, uint32 enable)
{
    uint32  reg_value, temp;

    /* Enable LOCAL TRUNK */ 
    SOC_IF_ERROR_RETURN(REG_READ_GLOBAL_TRUNK_CTLr
        (unit, &reg_value));

    if (enable) {
        temp = 1;
    } else {
        temp = 0;
    }
    SOC_IF_ERROR_RETURN(soc_GLOBAL_TRUNK_CTLr_field_set
        (unit, &reg_value, EN_TRUNK_LOCALf, &temp));
    SOC_IF_ERROR_RETURN(REG_WRITE_GLOBAL_TRUNK_CTLr
        (unit, &reg_value));

    return SOC_E_NONE;
}

/*
 *  Function : _drv_tbx_trunk_enable_get
 *
 *  Purpose :
 *      Get the status of trunk function (global).
 *
 *  Parameters :
 *      unit    :  RoboSwitch unit number.
 *      enable  :  status of the trunk id.  
 *
 *  Return :
 *      SOC_E_NONE
 *
 *  Note :
 */
static int 
_drv_tbx_trunk_enable_get(int unit, uint32 *enable)
{
    uint32  reg_value, temp;

    /* Get LOCAL TRUNK*/
    SOC_IF_ERROR_RETURN(REG_READ_GLOBAL_TRUNK_CTLr
        (unit, &reg_value));

    temp = 0;
    SOC_IF_ERROR_RETURN(soc_GLOBAL_TRUNK_CTLr_field_get
        (unit, &reg_value, EN_TRUNK_LOCALf, &temp));

    if (temp) {
        *enable = TRUE;
    } else {
        *enable = FALSE;
    }

    return SOC_E_NONE;
}

/*
 *  Function : drv_tbx_trunk_set
 *
 *  Purpose :
 *      Set the member ports to a trunk group.
 *
 *  Parameters :
 *      unit    :  RoboSwitch unit number.
 *      tid     :  trunk id.
 *      bmp     :  trunk member port bitmap.
 *      flag    :  trunk flag.
 *      hash_op :  trunk hash seed.
 *
 *  Return :
 *      SOC_E_NONE
 *
 *  Note :
 */
int 
drv_tbx_trunk_set(int unit, int tid, soc_pbmp_t bmp, 
    uint32 flag, uint32 hash_op)
{
    uint32  reg_value, temp, enable, trunk_prop;

    soc_cm_debug(DK_PORT, "drv_tbx_trunk_set: \
        unit = %d, trunk id = %d, bmp = 0x%x 0x%x, flag = 0x%x, hash_op = 0x%x\n",
        unit, tid, SOC_PBMP_WORD_GET(bmp, 0), SOC_PBMP_WORD_GET(bmp, 1), 
        flag, hash_op);

    /* handle default trunk hash seed */
    if (flag & DRV_TRUNK_FLAG_HASH_DEFAULT) {
        temp = 0;
        if (hash_op & DRV_TRUNK_HASH_FIELD_MACDA) {  /* DA */
            temp |= TBX_TRUNK_HASH_FIELD_MACDA_VALUE;
        }
        if (hash_op & DRV_TRUNK_HASH_FIELD_MACSA) {  /* SA */
            temp |= TBX_TRUNK_HASH_FIELD_MACSA_VALUE;
        }
        if (hash_op & DRV_TRUNK_HASH_FIELD_IP_MACDA) {  /* IP DA */
            temp |= TBX_TRUNK_HASH_FIELD_IP_MACDA_VALUE;
        }
        if (hash_op & DRV_TRUNK_HASH_FIELD_IP_MACSA) {  /* IP SA */
            temp |= TBX_TRUNK_HASH_FIELD_IP_MACSA_VALUE;
        }

        if (temp == 0) {
            return SOC_E_PARAM;
        }
        tbx_default_trunk_seed = temp;

        return SOC_E_NONE;
    }

    /* Check TRUNK MAX ID */
    SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
        (unit, DRV_DEV_PROP_TRUNK_NUM, &trunk_prop));
    if (tid > (trunk_prop - 1)) {
        return SOC_E_PARAM;
    }

    if (flag & DRV_TRUNK_FLAG_ENABLE) {
        SOC_IF_ERROR_RETURN(
            _drv_tbx_trunk_enable_get(unit, &enable));
        if (!enable) {
            SOC_IF_ERROR_RETURN(
                _drv_tbx_trunk_enable_set(unit, TRUE));
        }
    } else if (flag & DRV_TRUNK_FLAG_DISABLE) {
        /* do nothing : don't turn off the global Trunk feature control bit */
    }

    if (flag & DRV_TRUNK_FLAG_BITMAP) {
        temp = SOC_PBMP_WORD_GET(bmp, 0);
        switch (tid) {
            case 0:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_00_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_00_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_00_CTLr
                    (unit, &reg_value));
                break;
            case 1:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_01_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_01_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_01_CTLr
                    (unit, &reg_value));
                break;
            case 2:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_02_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_02_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_02_CTLr
                    (unit, &reg_value));
                break;
            case 3:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_03_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_03_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_03_CTLr
                    (unit, &reg_value));
                break;
            case 4:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_04_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_04_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_04_CTLr
                    (unit, &reg_value));
                break;
            case 5:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_05_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_01_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_05_CTLr
                    (unit, &reg_value));
                break;
            case 6:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_06_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_06_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_06_CTLr
                    (unit, &reg_value));
                break;
            case 7:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_07_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_07_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_07_CTLr
                    (unit, &reg_value));
                break;
            case 8:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_08_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_08_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_08_CTLr
                    (unit, &reg_value));
                break;
            case 9:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_09_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_09_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_09_CTLr
                    (unit, &reg_value));
                break;
            case 10:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_10_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_10_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_10_CTLr
                    (unit, &reg_value));
                break;
            case 11:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_11_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_11_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_11_CTLr
                    (unit, &reg_value));
                break;
            case 12:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_12_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_12_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_12_CTLr
                    (unit, &reg_value));
                break;
            case 13:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_13_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_13_CTLr_field_set
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                SOC_IF_ERROR_RETURN(REG_WRITE_TRUNK_GRP_13_CTLr
                    (unit, &reg_value));
                break;
            default:
                return SOC_E_INTERNAL;
        }
    }    

    return SOC_E_NONE;
}

/*
 *  Function : drv_tbx_trunk_get
 *
 *  Purpose :
 *      Get the member ports to a trunk group.
 *
 *  Parameters :
 *      unit    :  RoboSwitch unit number.
 *      tid     :  trunk id.
 *      bmp     :  trunk member port bitmap.
 *      flag    :  trunk flag.
 *      hash_op :  trunk hash seed.
 *
 *  Return :
 *      SOC_E_NONE
 *
 *  Note :
 */
int 
drv_tbx_trunk_get(int unit, int tid, soc_pbmp_t *bmp, 
    uint32 flag, uint32 *hash_op)
{
    uint32  reg_value, temp, enable, trunk_prop;

    /* handle default trunk hash seed */
    if (flag & DRV_TRUNK_FLAG_HASH_DEFAULT) {
        *hash_op = 0;
        if (tbx_default_trunk_seed & 
                TBX_TRUNK_HASH_FIELD_MACDA_VALUE) {  /* DA */
            *hash_op |= DRV_TRUNK_HASH_FIELD_MACDA;
        }
        if (tbx_default_trunk_seed & 
                TBX_TRUNK_HASH_FIELD_MACSA_VALUE) {  /* SA */
            *hash_op |= DRV_TRUNK_HASH_FIELD_MACSA;
        }
        if (tbx_default_trunk_seed & 
                TBX_TRUNK_HASH_FIELD_IP_MACDA_VALUE) {  /* IP DA */
            *hash_op |= DRV_TRUNK_HASH_FIELD_IP_MACDA;
        }
        if (tbx_default_trunk_seed & 
                TBX_TRUNK_HASH_FIELD_IP_MACSA_VALUE) {  /* IP SA */
            *hash_op |= DRV_TRUNK_HASH_FIELD_IP_MACSA;
        }

        if (*hash_op == 0) {
            return SOC_E_INTERNAL;
        }

        soc_cm_debug(DK_PORT, "drv_tbx_trunk_get: \
            unit = %d, trunk id = %d, flag = 0x%x, *hash_op = 0x%x\n",
            unit, tid, flag, *hash_op);

        return SOC_E_NONE;
    }

    /* Check TRUNK MAX ID */
    SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
        (unit, DRV_DEV_PROP_TRUNK_NUM, &trunk_prop));
    if (tid > (trunk_prop - 1)) {
        return SOC_E_PARAM;
    }

    if (flag & DRV_TRUNK_FLAG_ENABLE) {
        SOC_IF_ERROR_RETURN(
            _drv_tbx_trunk_enable_get(unit, &enable));
        if (!enable) {
            SOC_PBMP_CLEAR(*bmp);
            return SOC_E_NONE;
        }
    }

    /* Get group member port bitmap */
    if (flag & DRV_TRUNK_FLAG_BITMAP) {
        switch (tid) {
            case 0:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_00_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_00_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 1:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_01_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_01_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 2:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_02_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_02_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 3:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_03_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_03_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 4:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_04_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_04_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 5:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_05_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_01_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 6:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_06_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_06_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 7:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_07_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_07_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 8:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_08_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_08_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 9:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_09_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_09_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 10:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_10_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_10_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 11:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_11_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_11_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 12:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_12_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_12_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            case 13:
                SOC_IF_ERROR_RETURN(REG_READ_TRUNK_GRP_13_CTLr
                    (unit, &reg_value));
                SOC_IF_ERROR_RETURN(soc_TRUNK_GRP_13_CTLr_field_get
                    (unit, &reg_value, TRUNK_PORT_MAPf, &temp));
                break;
            default:
                return SOC_E_INTERNAL;
        }

        SOC_PBMP_WORD_SET(*bmp, 0, temp);
    }

    soc_cm_debug(DK_PORT, "drv_tbx_trunk_get: \
        unit = %d, trunk id = %d, flag = 0x%x, *bmp = 0x%x 0x%x\n",
        unit, tid, flag, SOC_PBMP_WORD_GET(*bmp, 0), SOC_PBMP_WORD_GET(*bmp, 1));

    return SOC_E_NONE;
}

/*
 *  Function : drv_tbx_trunk_hash_field_add
 *
 *  Purpose :
 *      Add trunk hash field type.
 *
 *  Parameters :
 *      unit      :  RoboSwitch unit number.
 *      field_type:  trunk hash field type to be add.
 *
 *  Return :
 *      SOC_E_NONE
 *
 *  Note :
 */
int 
drv_tbx_trunk_hash_field_add(int unit, uint32 field_type)
{
    uint32  reg_value, temp;

    soc_cm_debug(DK_PORT, 
        "drv_tbx_trunk_hash_field_add: unit = %d, field type = 0x%x\n",
        unit, field_type);

    /* check the valid trunk hash field types */
    if (field_type & ~TBX_TRUNK_HASH_FIELD_VALID_VALUE) {
        soc_cm_debug(DK_WARN, 
            "drv_tbx_trunk_hash_field_add: hash type = 0x%x, is invalid!\n",
            field_type);
        return SOC_E_UNAVAIL;
    }

    SOC_IF_ERROR_RETURN(REG_READ_GLOBAL_TRUNK_CTLr
        (unit, &reg_value));

    temp = 0;
    if (field_type & DRV_TRUNK_HASH_FIELD_MACDA) {  /* DA */
        temp |= TBX_TRUNK_HASH_FIELD_MACDA_VALUE;
    }
    if (field_type & DRV_TRUNK_HASH_FIELD_MACSA) {  /* SA */
        temp |= TBX_TRUNK_HASH_FIELD_MACSA_VALUE;
    }
    if (field_type & DRV_TRUNK_HASH_FIELD_IP_MACDA) {  /* IP DA */
        temp |= TBX_TRUNK_HASH_FIELD_IP_MACDA_VALUE;
    }
    if (field_type & DRV_TRUNK_HASH_FIELD_IP_MACSA) {  /* IP SA */
        temp |= TBX_TRUNK_HASH_FIELD_IP_MACSA_VALUE;
    }

    if (!temp) {
        temp = tbx_default_trunk_seed;
    }

    SOC_IF_ERROR_RETURN(soc_GLOBAL_TRUNK_CTLr_field_set
        (unit, &reg_value, TRUNK_SEEDf, &temp));
    SOC_IF_ERROR_RETURN(REG_WRITE_GLOBAL_TRUNK_CTLr
        (unit, &reg_value));

    return SOC_E_NONE;
}

/*
 *  Function : drv_tbx_trunk_hash_field_remove
 *
 *  Purpose :
 *      Remove trunk hash field type.
 *
 *  Parameters :
 *      unit      :  RoboSwitch unit number.
 *      field_type:  trunk hash field type to be removed.
 *
 *  Return :
 *      SOC_E_NONE
 *
 *  Note :
 */
int 
drv_tbx_trunk_hash_field_remove(int unit, uint32 field_type)
{
    uint32  reg_value, temp;

    soc_cm_debug(DK_PORT, 
        "drv_tbx_trunk_hash_field_remove: unit = %d, field type = 0x%x\n",
        unit, field_type);

    SOC_IF_ERROR_RETURN(REG_READ_GLOBAL_TRUNK_CTLr
        (unit, &reg_value));
    SOC_IF_ERROR_RETURN(soc_GLOBAL_TRUNK_CTLr_field_get
        (unit, &reg_value, TRUNK_SEEDf, &temp));

    if (field_type & DRV_TRUNK_HASH_FIELD_MACDA) {  /* DA */
        temp &= ~TBX_TRUNK_HASH_FIELD_MACDA_VALUE;
    }
    if (field_type & DRV_TRUNK_HASH_FIELD_MACSA) {  /* SA */
        temp &= ~TBX_TRUNK_HASH_FIELD_MACSA_VALUE;
    }
    if (field_type & DRV_TRUNK_HASH_FIELD_IP_MACDA) {  /* IP DA */
        temp &= ~TBX_TRUNK_HASH_FIELD_IP_MACDA_VALUE;
    }
    if (field_type & DRV_TRUNK_HASH_FIELD_IP_MACSA) {  /* IP SA */
        temp &= ~TBX_TRUNK_HASH_FIELD_IP_MACSA_VALUE;
    }

    if (!temp) {
        temp = tbx_default_trunk_seed;
    }

    SOC_IF_ERROR_RETURN(soc_GLOBAL_TRUNK_CTLr_field_set
        (unit, &reg_value, TRUNK_SEEDf, &temp));
    SOC_IF_ERROR_RETURN(REG_WRITE_GLOBAL_TRUNK_CTLr
        (unit, &reg_value));

    return SOC_E_NONE;
}
