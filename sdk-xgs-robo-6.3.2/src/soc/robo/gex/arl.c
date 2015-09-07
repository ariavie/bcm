/*
 * $Id: arl.c 1.1 Broadcom SDK $
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
 * File:	arl.c
 */
#include <soc/robo/mcm/driver.h>



STATIC int
_drv_gex_hw_learning_set(int unit, soc_pbmp_t pbmp, uint32 value)
{
    int     rv = SOC_E_NONE;
    uint32  reg_v32, fld_v32 = 0;
    soc_pbmp_t current_pbmp, temp_pbmp;

    if ((rv = REG_READ_DIS_LEARNr(unit, &reg_v32)) < 0) {
        return rv;
    }

    soc_DIS_LEARNr_field_get(unit, &reg_v32,
        DIS_LEARNf, &fld_v32);
    SOC_PBMP_WORD_SET(current_pbmp, 0, fld_v32);

    if (value) { /* enable */
        SOC_PBMP_CLEAR(temp_pbmp);
        SOC_PBMP_NEGATE(temp_pbmp, pbmp);
        SOC_PBMP_AND(current_pbmp, temp_pbmp);
    } else { /* disable */
        SOC_PBMP_OR(current_pbmp, pbmp);
    }

    fld_v32 = SOC_PBMP_WORD_GET(current_pbmp, 0);
    soc_DIS_LEARNr_field_set(unit, &reg_v32,
        DIS_LEARNf, &fld_v32);

    if ((rv = REG_WRITE_DIS_LEARNr(unit, &reg_v32)) < 0) {
        return rv;
    }

    return rv;
}

STATIC int
_drv_gex_hw_learning_get(int unit, soc_port_t port, uint32 *value)
{
    int     rv = SOC_E_NONE;
    uint32  reg_v32, fld_v32 = 0;
    soc_pbmp_t current_pbmp;

    if ((rv = REG_READ_DIS_LEARNr(unit, &reg_v32)) < 0) {
        return rv;
    }

    soc_DIS_LEARNr_field_get(unit, &reg_v32,
        DIS_LEARNf, &fld_v32);
    SOC_PBMP_WORD_SET(current_pbmp, 0, fld_v32);

    if (SOC_PBMP_MEMBER(current_pbmp, port)) {
        *value = FALSE; /* This port is in DISABLE SA learn state */
    } else {
        *value = TRUE;
    }

    return rv;
}


STATIC int
_drv_gex_sw_learning_set(int unit, soc_pbmp_t pbmp, uint32 value)
{
    int     rv = SOC_E_NONE;
    uint32  reg_v32, fld_v32 = 0;
    soc_pbmp_t current_pbmp, temp_pbmp;
    
    if ((rv = REG_READ_SFT_LRN_CTLr(unit, &reg_v32)) < 0) {
        return rv;
    }

    soc_SFT_LRN_CTLr_field_get(unit, &reg_v32, 
        SW_LEARN_CNTLf, &fld_v32);
    SOC_PBMP_WORD_SET(current_pbmp, 0, fld_v32);

    if (value) { /* enable */
        SOC_PBMP_OR(current_pbmp, pbmp);
    } else { /* disable */
        SOC_PBMP_CLEAR(temp_pbmp);
        SOC_PBMP_NEGATE(temp_pbmp, pbmp);
        SOC_PBMP_AND(current_pbmp, temp_pbmp);
    }

    fld_v32 = SOC_PBMP_WORD_GET(current_pbmp, 0);
    soc_SFT_LRN_CTLr_field_set(unit, &reg_v32, 
        SW_LEARN_CNTLf, &fld_v32);

    if ((rv = REG_WRITE_SFT_LRN_CTLr(unit, &reg_v32)) < 0) {
        return rv;
    }

    return rv;
}

STATIC int
_drv_gex_sw_learning_get(int unit, soc_port_t port, uint32 *value)
{
    int     rv = SOC_E_NONE;
    uint32  reg_v32, fld_v32 = 0;
    soc_pbmp_t current_pbmp;

    if ((rv = REG_READ_SFT_LRN_CTLr(unit, &reg_v32)) < 0) {
        return rv;
    }

    soc_SFT_LRN_CTLr_field_get(unit, &reg_v32, 
        SW_LEARN_CNTLf, &fld_v32);
    SOC_PBMP_WORD_SET(current_pbmp, 0, fld_v32);

    if (SOC_PBMP_MEMBER(current_pbmp, port)) {
        *value = TRUE; /* This port is in SW learn state */
    } else {
        *value = FALSE;
    }

    return rv;
}


/*
 * Function:
 *  drv_arl_learn_enable_set
 * Purpose:
 *  Setting per port SA learning process.
 * Parameters:
 *  unit    - RoboSwitch unit #
 *  pbmp    - port bitmap
 *  mode   - DRV_PORT_HW_LEARN
 *               DRV_PORT_DISABLE_LEARN
 *               DRV_PORT_SW_LEARN
 */
int
drv_gex_arl_learn_enable_set(int unit, soc_pbmp_t pbmp, uint32 mode)
{
    int     rv = SOC_E_NONE;

    switch (mode ) {
        case DRV_PORT_HW_LEARN:
            /* Disable software learning */
            SOC_IF_ERROR_RETURN(
                _drv_gex_sw_learning_set(unit, pbmp, 0));
            /* Enable hardware learning */
            SOC_IF_ERROR_RETURN(
                _drv_gex_hw_learning_set(unit, pbmp, 1));
            break;
        case DRV_PORT_DISABLE_LEARN:
            /* Disable software learning */
            SOC_IF_ERROR_RETURN(
                _drv_gex_sw_learning_set(unit, pbmp, 0));
            /* Disable hardware learning */
            SOC_IF_ERROR_RETURN(
                _drv_gex_hw_learning_set(unit, pbmp, 0));
       	    break;
       	case DRV_PORT_SW_LEARN:
            /* Enable learning */
            SOC_IF_ERROR_RETURN(
                _drv_gex_hw_learning_set(unit, pbmp, 1));
            /* Enable software learning */
       	    SOC_IF_ERROR_RETURN(
       	        _drv_gex_sw_learning_set(unit, pbmp, 1));
       	    break;

       	/* no support section */
       	case DRV_PORT_HW_SW_LEARN:
       	case DRV_PORT_DROP:
       	case DRV_PORT_SWLRN_DROP:
       	case DRV_PORT_HWLRN_DROP:
       	case DRV_PORT_SWHWLRN_DROP:
            rv = SOC_E_UNAVAIL;
            break;
        default:
            rv = SOC_E_PARAM;
    }

    return rv;
}

/*
 * Function:
 *  drv_arl_learn_enable_get
 * Purpose:
 *  Setting per port SA learning process.
 * Parameters:
 *  unit    - RoboSwitch unit #
 *  port    - port
 *  mode   - Port learn mode
 */
int
drv_gex_arl_learn_enable_get(int unit, soc_port_t port, uint32 *mode)
{
    int     rv = SOC_E_NONE;
    uint32  temp;

    /* Check software learn setting */
    SOC_IF_ERROR_RETURN(
        _drv_gex_sw_learning_get(unit, port, &temp));
    
    if (temp) {
        *mode = DRV_PORT_SW_LEARN;
    } else {
        /* Check hardware learn setting */
        SOC_IF_ERROR_RETURN(
            _drv_gex_hw_learning_get(unit, port, &temp));
        if (temp) {
            *mode = DRV_PORT_HW_LEARN;
        } else {
            *mode = DRV_PORT_DISABLE_LEARN;
        }
    }
    
    return rv;
}

