/*
 * $Id: mstp.c 1.4 Broadcom SDK $
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

/*
 *  Function : drv_tbx_mstp_port_set
 *
 *  Purpose :
 *      Set the port state of a selected stp id.
 *
 *  Parameters :
 *      unit        :  unit id
 *      mstp_gid    :  multiple spanning tree id.
 *      port        :  port number.
 *      port_state  :  state of the port.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 */
int 
drv_tbx_mstp_port_set(int unit, uint32 mstp_gid, uint32 port, 
    uint32 port_state)
{
    uint32  shift;
    mspt_tab_entry_t  mstp_entry;
    uint32  reg_value;
    uint32  max_gid;
    uint64  temp, data64;
    uint32  *entry;


    soc_cm_debug(DK_STP, 
        "drv_mstp_port_set : unit %d, STP id = %d, port = %d, port_state = %d \n",
        unit, mstp_gid, port, port_state);

    SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
        (unit, DRV_DEV_PROP_MSTP_NUM, &max_gid));

    if (!soc_feature(unit, soc_feature_mstp)) {
        /* error checking */
        if (mstp_gid != STG_ID_DEFAULT) {
            return SOC_E_PARAM;
        }

        switch (port_state) {
            case DRV_PORTST_DISABLE:
                COMPILER_64_SET(temp, 0x0, 0x0);
                break;
            case DRV_PORTST_BLOCK:
            case DRV_PORTST_LISTEN:
                /* Block and Listen STP states : indicate the Discarding state for TB */
                COMPILER_64_SET(temp, 0x0, 0x1);
                break;
            case DRV_PORTST_LEARN:
                COMPILER_64_SET(temp, 0x0, 0x2);
                break;
            case DRV_PORTST_FORWARD:
                COMPILER_64_SET(temp, 0x0, 0x3);
                break;
            default:
                return SOC_E_PARAM;
        }
    
        if(IS_CPU_PORT(unit, port)) {
            SOC_IF_ERROR_RETURN(REG_READ_IMP_PCTLr
                (unit, port, &reg_value));
            SOC_IF_ERROR_RETURN(soc_IMP_PCTLr_field_set
                (unit, &reg_value, STP_STATEf, (void *)&temp));
            SOC_IF_ERROR_RETURN(REG_WRITE_G_PCTLr
                (unit, port, &reg_value));
        } else if(IS_GE_PORT(unit, port)) {
            SOC_IF_ERROR_RETURN(REG_READ_G_PCTLr
                (unit, port, &reg_value));
            SOC_IF_ERROR_RETURN(soc_G_PCTLr_field_set
                (unit, &reg_value, G_STP_STATEf, (void *)&temp));
            SOC_IF_ERROR_RETURN(REG_WRITE_G_PCTLr
                (unit, port, &reg_value));
        } else {
            SOC_IF_ERROR_RETURN(REG_READ_TH_PCTLr
                (unit, port, &reg_value));
            SOC_IF_ERROR_RETURN(soc_TH_PCTLr_field_set
                (unit, &reg_value, STP_STATEf, (void *)&temp));
            SOC_IF_ERROR_RETURN(REG_WRITE_TH_PCTLr
                (unit, port, &reg_value));
        }
    } else {
        /* error checking */
        if (mstp_gid > max_gid) {
            return SOC_E_PARAM;
        }

        mstp_gid = mstp_gid % max_gid;
        sal_memset(&mstp_entry, 0, sizeof(mstp_entry));

        /* write mstp id to vlan entry */
        SOC_IF_ERROR_RETURN(MEM_READ_MSPT_TABm
                (unit, mstp_gid, (uint32 *)&mstp_entry));
        
        entry = (uint32 *)&mstp_entry;

        COMPILER_64_SET(data64,entry[1],entry[0]);

        /* 
         * Because the memory field services didn't contain port information, 
         * we can't access the port state by port
         */
        shift = 2 * port;

        COMPILER_64_SET(temp, 0x0, 0x3);
        COMPILER_64_SHL(temp, shift);
        COMPILER_64_NOT(temp);

        COMPILER_64_AND(data64, temp);
        switch (port_state) {
            case DRV_PORTST_DISABLE:
                COMPILER_64_SET(temp, 0x0, 0x0);
                break;
            case DRV_PORTST_BLOCK:
            case DRV_PORTST_LISTEN:
                /* Block and Listen STP states : indicate the Discarding state for TB */
                COMPILER_64_SET(temp, 0x0, 0x1);
                break;
            case DRV_PORTST_LEARN:
                COMPILER_64_SET(temp, 0x0, 0x2);
                break;
            case DRV_PORTST_FORWARD:
                COMPILER_64_SET(temp, 0x0, 0x3);
                break;
            default:
                return SOC_E_PARAM;
        }
        COMPILER_64_SHL(temp, shift);
        COMPILER_64_OR(data64, temp);

        entry[0] = COMPILER_64_LO(data64);
        entry[1] = COMPILER_64_HI(data64);

        SOC_IF_ERROR_RETURN(MEM_WRITE_MSPT_TABm
                (unit, mstp_gid, (uint32 *)&mstp_entry));
    }
    return SOC_E_NONE;
}

/*
 *  Function : drv_tbx_mstp_port_get
 *
 *  Purpose :
 *      Get the port state of a selected stp id.
 *
 *  Parameters :
 *      unit        :  unit id
 *      mstp_gid    :  multiple spanning tree id.
 *      port        :  port number.
 *      port_state  :  state of the port.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 */
int 
drv_tbx_mstp_port_get(int unit, uint32 mstp_gid, uint32 port, 
    uint32 *port_state)
{
    uint32  portstate;
    mspt_tab_entry_t  mstp_entry;
    uint32  reg_value;
    uint32  max_gid;
    uint32  shift;
    uint64  temp, data64;
    uint32  *entry;


    SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
        (unit, DRV_DEV_PROP_MSTP_NUM, &max_gid));

    if (!soc_feature(unit, soc_feature_mstp)){

        /* error checking */
        if (mstp_gid != STG_ID_DEFAULT) {
            return SOC_E_PARAM;
        }

        if (IS_CPU_PORT(unit, port)) {
            SOC_IF_ERROR_RETURN(REG_READ_IMP_PCTLr
                (unit, port, &reg_value));
            SOC_IF_ERROR_RETURN(soc_IMP_PCTLr_field_get
                (unit, &reg_value, STP_STATEf, &portstate));
        } else if (IS_GE_PORT(unit, port)) {
            SOC_IF_ERROR_RETURN(REG_READ_G_PCTLr
                (unit, port, &reg_value));
            SOC_IF_ERROR_RETURN(soc_G_PCTLr_field_get
                (unit, &reg_value, G_STP_STATEf, &portstate));
        } else {
            SOC_IF_ERROR_RETURN(REG_READ_TH_PCTLr
                (unit, port, &reg_value));
            SOC_IF_ERROR_RETURN(soc_TH_PCTLr_field_get
                (unit, &reg_value, STP_STATEf, &portstate));
        }
    
        switch (portstate) {
            case 0:
                *port_state = DRV_PORTST_DISABLE;
                break;
            case 1:
                /* Block and Listen STP states : indicate the Discarding state for TB */
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

        soc_cm_debug(DK_STP, 
            "drv_mstp_port_get : unit %d, STP id = %d, port = %d, port_state = %d \n",
            unit, mstp_gid, port, *port_state);

    } else {
        /* error checking */
        if (mstp_gid > max_gid) {
            return SOC_E_PARAM;
        }
        mstp_gid = mstp_gid % max_gid;

        /* write mstp id to vlan entry */       
        SOC_IF_ERROR_RETURN(MEM_READ_MSPT_TABm
                (unit, mstp_gid, (uint32 *)&mstp_entry));

        entry = (uint32 *)&mstp_entry;

        COMPILER_64_SET(data64,entry[1],entry[0]);
        /* 
         * Because the memory field services didn't contain port information, 
         * we can't access the port state by port
         */
        shift = 2 * port; 
        
        COMPILER_64_SET(temp, 0x0, 0x3);
        COMPILER_64_SHR(data64, shift);
        COMPILER_64_AND(data64, temp);
        COMPILER_64_TO_32_LO(portstate, data64);
        switch (portstate) {
            case 0:
                *port_state = DRV_PORTST_DISABLE;
                break;
            case 1:
                /* Block and Listen STP states : indicate the Discarding state for TB */
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

        soc_cm_debug(DK_STP, 
            "drv_mstp_port_get : unit %d, STP id = %d, port = %d, port_state = %d \n",
            unit, mstp_gid, port, *port_state);
    }
    return SOC_E_NONE;
}

