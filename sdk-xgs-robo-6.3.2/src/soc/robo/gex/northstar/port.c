/*
 * $Id: port.c 1.3 Broadcom SDK $
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

#include <soc/robo.h>
#include <soc/drv.h>
#include <soc/debug.h>
#include "../robo_gex.h"

/*
 *  Function : drv_northstar_port_pri_mapop_set
 *
 *  Purpose :
 *      Port basis priority mapping operation configuration set
 *
 *  Parameters :
 *      unit        :   unit id
 *      port        :   port id.
 *      op_type     :   operation type
 *      pri_old     :   old priority.
 *      pri_new     :   new priority.
 *      cfi_new     :   new cfi.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 *  1. This driver service is designed for priority operation exchange.
 *  2. Priority type could be dot1p, DSCP, port based.
 *
 */
int 
drv_northstar_port_pri_mapop_set(int unit, int port, int op_type, 
                uint32 pri_old, uint32 cfi_old, uint32 pri_new, uint32 cfi_new)
{
    uint32  temp;
    uint64  reg_value64;
    uint32  additional_soc_port_num;

    SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
        (unit, DRV_DEV_PROP_ADDITIONAL_SOC_PORT_NUM, &additional_soc_port_num));

    switch (op_type) {
        case DRV_PORT_OP_PCP2TC :
            temp = pri_new & DOT1P_PRI_MASK;
            SOC_IF_ERROR_RETURN(DRV_QUEUE_PRIO_REMAP_SET
                (unit, port, pri_old, (uint8)temp));
            break;
        case DRV_PORT_OP_NORMAL_TC2PCP:
            if (port == additional_soc_port_num) {
                SOC_IF_ERROR_RETURN(REG_READ_EGRESS_PKT_TC2PCP_MAP_P7r
                    (unit, (uint32 *)&reg_value64));
            } else {
                SOC_IF_ERROR_RETURN(REG_READ_EGRESS_PKT_TC2PCP_MAPr
                    (unit, port, (uint32 *)&reg_value64));
            }
            temp = ((cfi_new & DOT1P_CFI_MASK) << 3) | 
                    (pri_new & DOT1P_PRI_MASK);

            /* assigning the field-id */
            switch(pri_old) {
                case 0:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC0f, &temp));
                    break;
                case 1:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC1f, &temp));
                    break;
                case 2:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC2f, &temp));
                    break;
                case 3:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC3f, &temp));
                    break;
                case 4:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC4f, &temp));
                    break;
                case 5:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC5f, &temp));
                    break;
                case 6:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC6f, &temp));
                    break;
                case 7:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC7f, &temp));
                    break;
                default :
                    return SOC_E_PARAM;
                    break;
            }
                
            if (port == additional_soc_port_num) {
                SOC_IF_ERROR_RETURN(REG_WRITE_EGRESS_PKT_TC2PCP_MAP_P7r
                    (unit, (uint32 *)&reg_value64));
            } else {
                SOC_IF_ERROR_RETURN(REG_WRITE_EGRESS_PKT_TC2PCP_MAPr
                    (unit, port, (uint32 *)&reg_value64));
            }
            break;
        case DRV_PORT_OP_OUTBAND_TC2PCP:
            if (port == additional_soc_port_num) {
                SOC_IF_ERROR_RETURN(REG_READ_EGRESS_PKT_TC2PCP_MAP_P7r
                    (unit, (uint32 *)&reg_value64));
            } else {
                SOC_IF_ERROR_RETURN(REG_READ_EGRESS_PKT_TC2PCP_MAPr
                    (unit, port, (uint32 *)&reg_value64));
            }
        
            temp = ((cfi_new & DOT1P_CFI_MASK) << 3) | 
                    (pri_new & DOT1P_PRI_MASK);
            
            /* assigning the field-id */
            switch(pri_old) {
                case 0:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC0f, &temp));
                    break;
                case 1:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC1f, &temp));
                    break;
                case 2:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC2f, &temp));
                    break;
                case 3:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC3f, &temp));
                    break;
                case 4:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC4f, &temp));
                    break;
                case 5:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC5f, &temp));
                    break;
                case 6:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC6f, &temp));
                    break;
                case 7:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_set(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC7f, &temp));
                    break;
                default :
                    return SOC_E_PARAM;
                    break;
            }
        
            if (port == additional_soc_port_num) {
                SOC_IF_ERROR_RETURN(REG_WRITE_EGRESS_PKT_TC2PCP_MAP_P7r
                    (unit, (uint32 *)&reg_value64));
            } else {
                SOC_IF_ERROR_RETURN(REG_WRITE_EGRESS_PKT_TC2PCP_MAPr
                    (unit, port, (uint32 *)&reg_value64));
            }
            break;
        default:
            break;
    }
    
    return SOC_E_NONE;
}

/*
 *  Function : drv_northstar_port_pri_mapop_get
 *
 *  Purpose :
 *      Port basis priority mapping operation configuration get
 *
 *  Parameters :
 *      unit        :   unit id
 *      port        :   port id.
 *      pri_old     :   (in)old priority.
 *      cfi_old     :   (in)old cfi (No used).
 *      pri_new     :   (out)new priority.
 *      cfi_new     :   (out)new cfi.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 *  1. This driver service is designed for priority operation exchange.
 *  2. Priority type could be dot1p, DSCP, port based.
 *
 */
int 
drv_northstar_port_pri_mapop_get(int unit, int port, int op_type, 
                uint32 pri_old, uint32 cfi_old, uint32 *pri_new, uint32 *cfi_new)
{
    uint32  temp;
    uint64  reg_value64;
    uint8  temp8 = 0;
    uint32  additional_soc_port_num;

    SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
        (unit, DRV_DEV_PROP_ADDITIONAL_SOC_PORT_NUM, &additional_soc_port_num));

    switch (op_type) {
        case DRV_PORT_OP_PCP2TC :
            temp8 = 0;
            SOC_IF_ERROR_RETURN(DRV_QUEUE_PRIO_REMAP_GET
                (unit, port, pri_old, &temp8));
            temp = (uint32)temp8;
            *pri_new = temp;
            break;
        case DRV_PORT_OP_NORMAL_TC2PCP:
            if (port == additional_soc_port_num) {
                SOC_IF_ERROR_RETURN(REG_READ_EGRESS_PKT_TC2PCP_MAP_P7r
                    (unit, (uint32 *)&reg_value64));
            } else {
                SOC_IF_ERROR_RETURN(REG_READ_EGRESS_PKT_TC2PCP_MAPr
                    (unit, port, (uint32 *)&reg_value64));
            }
            
            /* assigning the field-id */
            switch(pri_old) {
                case 0:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC0f, &temp));
                    break;
                case 1:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC1f, &temp));
                    break;
                case 2:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC2f, &temp));
                    break;
                case 3:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC3f, &temp));
                    break;
                case 4:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC4f, &temp));
                    break;
                case 5:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC5f, &temp));
                    break;
                case 6:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC6f, &temp));
                    break;
                case 7:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV0_TC7f, &temp));
                    break;
                default :
                    return SOC_E_PARAM;
                    break;
            }
            
            *pri_new = temp & DOT1P_PRI_MASK;
            *cfi_new = (temp >> 3) & DOT1P_CFI_MASK;
            break;
        case DRV_PORT_OP_OUTBAND_TC2PCP:
            if (port == additional_soc_port_num) {
                SOC_IF_ERROR_RETURN(REG_READ_EGRESS_PKT_TC2PCP_MAP_P7r
                    (unit, (uint32 *)&reg_value64));
            } else {
                SOC_IF_ERROR_RETURN(REG_READ_EGRESS_PKT_TC2PCP_MAPr
                    (unit, port, (uint32 *)&reg_value64));
            }
            
            /* assigning the field-id */
            switch(pri_old) {
                case 0:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC0f, &temp));
                    break;
                case 1:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC1f, &temp));
                    break;
                case 2:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC2f, &temp));
                    break;
                case 3:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC3f, &temp));
                    break;
                case 4:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC4f, &temp));
                    break;
                case 5:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC5f, &temp));
                    break;
                case 6:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC6f, &temp));
                    break;
                case 7:
                    SOC_IF_ERROR_RETURN(
                        soc_EGRESS_PKT_TC2PCP_MAPr_field_get(unit, 
                            (uint32 *)&reg_value64, PCP_FOR_RV1_TC7f, &temp));
                    break;
                default :
                    return SOC_E_PARAM;
                    break;
            }
                
            *pri_new = temp & DOT1P_PRI_MASK;
            *cfi_new = (temp >> 3) & DOT1P_CFI_MASK;    
            break;
        default:
            break;
    }

    return SOC_E_NONE;
}

