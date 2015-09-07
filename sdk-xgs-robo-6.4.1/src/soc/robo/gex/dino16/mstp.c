/*
 * $Id: 2002d31888d407fc183d1ec1e187be1ccea97cfb $
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
#include <shared/bsl.h>

#include <soc/drv.h>
#include <soc/debug.h>

int dino16_mst_field[] = { 
               INDEX(SPT_STA_P0f), INDEX(SPT_STA_P1f), INDEX(SPT_STA_P2f), 
               INDEX(SPT_STA_P3f), INDEX(SPT_STA_P4f), INDEX(SPT_STA_P5f), 
               INDEX(SPT_STA_P6f), INDEX(SPT_STA_P7f), INDEX(SPT_STA_P8f), 
               INDEX(SPT_STA_P9f), INDEX(SPT_STA_P10f), INDEX(SPT_STA_P11f),
               INDEX(SPT_STA_P12f), INDEX(SPT_STA_P13f), INDEX(SPT_STA_P14f), 
               INDEX(SPT_STA_P15f), INDEX(SPT_STA_P16f)};

/*
 *  Function : drv_dino16_mstp_port_set
 *
 *  Purpose :
 *      Set the port state of a selected stp id.
 *
 *  Parameters :
 *      unit        :   unit id
 *      mstp_gid : multiple spanning tree id.
 *      port    :   port number.
 *      port_state  :   state of the port.
 *
 *  Return :
 *      SOC_E_XXX
 */
int 
drv_dino16_mstp_port_set(int unit, uint32 mstp_gid, uint32 port, 
    uint32 port_state)
{
    int     rv = SOC_E_NONE;
    uint32  temp;
    uint32  reg32;
    uint64  reg_value;
    uint32  max_gid;

    LOG_INFO(BSL_LS_SOC_STP, \
             (BSL_META_U(unit, \
                         "drv_dino16_mstp_port_set : \
                         unit %d, STP id = %d, port = %d, port_state = %d\n"),
              unit, mstp_gid, port, port_state));

    SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
        (unit, DRV_DEV_PROP_MSTP_NUM, &max_gid));

    if (!soc_feature(unit, soc_feature_mstp)) {
        /* error checking */
        if (mstp_gid != STG_ID_DEFAULT) {
            return SOC_E_PARAM;
        }

        if ((rv = REG_READ_G_PCTLr(unit, port, &reg32)) < 0) {
            return rv;
        }    

        switch (port_state)
        {
            case DRV_PORTST_DISABLE:
                temp = 1;
                break;
            case DRV_PORTST_BLOCK:
                temp = 2;
                break;
            case DRV_PORTST_LISTEN:
            case DRV_PORTST_LEARN:
                temp = 4;
                break;
            case DRV_PORTST_FORWARD:
                temp = 5;
                break;
            default:
                return SOC_E_PARAM;
        }
        
        soc_G_PCTLr_field_set(unit, &reg32, 
            G_MISTP_STATEf, &temp);

        if ((rv = REG_WRITE_G_PCTLr(unit, port, &reg32)) < 0) {
            return rv;
        }

        return rv;
    } else {
        /* error checking */
        if ((mstp_gid > max_gid) || (mstp_gid < STG_ID_DEFAULT)) {
            return SOC_E_PARAM;
        }

        mstp_gid = mstp_gid % max_gid;
        if ((rv = REG_READ_MST_TBLr(unit, 
                mstp_gid, (uint32 *)&reg_value)) < 0) {
            return rv;
        }
    
        switch (port_state)
        {
            case DRV_PORTST_DISABLE:
                temp = 0;
                break;
            case DRV_PORTST_BLOCK:
                temp = 1;
                break;
            case DRV_PORTST_LISTEN:
            case DRV_PORTST_LEARN:
                temp = 2;
                break;
            case DRV_PORTST_FORWARD:
                temp = 3;
                break;
            default:
                return SOC_E_PARAM;
        }

        SOC_IF_ERROR_RETURN(DRV_REG_FIELD_SET(unit, INDEX(MST_TBLr), 
            (uint32 *)&reg_value, dino16_mst_field[port], &temp)); 

        if ((rv = REG_WRITE_MST_TBLr(unit, 
                mstp_gid, (uint32 *)&reg_value)) < 0) {
            return rv;
        }

        return rv;
    }
}

/*
 *  Function : drv_dino16_mstp_port_get
 *
 *  Purpose :
 *      Get the port state of a selected stp id.
 *
 *  Parameters :
 *      unit        :   unit id
 *      mstp_gid : multiple spanning tree id.
 *      port    :   port number.
 *      port_state  :   state of the port.
 *
 *  Return :
 *      SOC_E_XXX
 */
int 
drv_dino16_mstp_port_get(int unit, uint32 mstp_gid, uint32 port, 
    uint32 *port_state)
{
    int     rv = SOC_E_NONE;
    uint32  portstate = 0;
    uint32  reg32;
    uint64  reg_value;
    uint32  max_gid;

    SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
        (unit, DRV_DEV_PROP_MSTP_NUM, &max_gid));

    if (!soc_feature(unit, soc_feature_mstp)) {
        /* error checking */
        if (mstp_gid != STG_ID_DEFAULT) {
            return SOC_E_PARAM;
        }

        if ((rv = REG_READ_G_PCTLr(unit, port, &reg32)) < 0) {
            return rv;
        }

        soc_G_PCTLr_field_get(unit, &reg32, 
            G_MISTP_STATEf, &portstate);
    
        switch (portstate)
        {
            case 1:
                *port_state = DRV_PORTST_DISABLE;
                break;
            case 2:
                *port_state = DRV_PORTST_BLOCK;
                break;
            case 4:
                *port_state = DRV_PORTST_LEARN;
                break;
            case 5:
                *port_state = DRV_PORTST_FORWARD;
                break;
            default:
                return SOC_E_INTERNAL;
        }

        LOG_INFO(BSL_LS_SOC_STP, \
                 (BSL_META_U(unit, \
                             "drv_dino16_mstp_port_get : \
                             unit %d, STP id = %d, port = %d, port_state = %d\n"),
                  unit, mstp_gid, port, *port_state));

        return SOC_E_NONE;
    } else {
        /* error checking */
        if ((mstp_gid > max_gid) || (mstp_gid < STG_ID_DEFAULT)) {
            return SOC_E_PARAM;
        }

        mstp_gid = mstp_gid % max_gid;
        if ((rv = REG_READ_MST_TBLr(unit, mstp_gid, &reg_value)) < 0) {
            return rv;
        }

        SOC_IF_ERROR_RETURN(DRV_REG_FIELD_GET(unit, INDEX(MST_TBLr), 
            (uint32 *)&reg_value, dino16_mst_field[port], &portstate)); 
   
        switch (portstate)
        {
            case 0:
                *port_state = DRV_PORTST_DISABLE;
                break;
            case 1:
                *port_state = DRV_PORTST_BLOCK;
                break;
            case 2:
                *port_state = DRV_PORTST_LEARN;
                break;
            case 3:
                *port_state = DRV_PORTST_FORWARD;
                break;
            default:
                return SOC_E_INTERNAL;
        }

        LOG_INFO(BSL_LS_SOC_STP, \
                 (BSL_META_U(unit, \
                             "drv_dino16_mstp_port_get : \
                             unit %d, STP id = %d, port = %d, port_state = %d\n"),
                  unit, mstp_gid, port, *port_state));

        return SOC_E_NONE;
    }
}

