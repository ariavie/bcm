/*
 * $Id: mstp.c 1.1 Broadcom SDK $
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
 *  Function : _drv_mstp_enable_set
 *
 *  Purpose :
 *      Set the status of enabling multiple spanning tree(802.1s).
 *
 *  Parameters :
 *      unit    :   unit id
 *      enable  :   enable/disable.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 *      
 *
 */
int 
_drv_mstp_enable_set(int unit, int enable)
{
    uint32  reg_value, temp;

    /* enable 802.1s */
    SOC_IF_ERROR_RETURN(REG_READ_MST_CONr
        (unit, &reg_value));
    if (enable) {
        temp = 1;
    } else {
        temp = 0;
    }
    SOC_IF_ERROR_RETURN(soc_MST_CONr_field_set
        (unit, &reg_value, EN_802_1Sf, &temp));
    SOC_IF_ERROR_RETURN(REG_WRITE_MST_CONr
        (unit, &reg_value));

    return SOC_E_NONE;
}

/*
 *  Function : _drv_mstp_enable_get
 *
 *  Purpose :
 *      Get the status of enabling multiple spanning tree(802.1s).
 *
 *  Parameters :
 *      unit    :   unit id
 *      enable  :   enable/disable.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 *      
 *
 */
int 
_drv_mstp_enable_get(int unit, int *enable)
{
    uint32  reg_value, temp;

    /* enable 802.1s */
    SOC_IF_ERROR_RETURN(REG_READ_MST_CONr
        (unit, &reg_value));
    SOC_IF_ERROR_RETURN(soc_MST_CONr_field_get
        (unit, &reg_value, EN_802_1Sf, &temp));
    if (temp) {
        *enable = TRUE;
    } else {
        *enable = FALSE;
    }

    return SOC_E_NONE;
}


/*
 *  Function : drv_mstp_config_set
 *
 *  Purpose :
 *      Set the STP id to a specific VLAN.
 *
 *  Parameters :
 *      unit        :   unit id
 *      vlan_id     :   vlan id.
 *      mstp_gid    :   multiple spanning tree id.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 *      
 *
 */
int 
drv_mstp_config_set(int unit, uint32 vlan_id, uint32 mstp_gid)
{
    uint32  temp;
    vlan_1q_entry_t  vlan_entry;
    uint32  max_gid;

    soc_cm_debug(DK_STP, 
        "drv_mstp_config_set : unit %d, vlan = %d, STP id = %d \n",
         unit, vlan_id, mstp_gid);

    SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
        (unit, DRV_DEV_PROP_MSTP_NUM, &max_gid));

    if (!soc_feature(unit, soc_feature_mstp)){
        /* error checking */
        if (vlan_id > 4095) {
            return SOC_E_PARAM;
        }
        if (mstp_gid != STG_ID_DEFAULT) {
            return SOC_E_PARAM;
        }

    } else {
        /* error checking */
        if (vlan_id > 4095) {
            return SOC_E_PARAM;
        }
        if ((mstp_gid > max_gid) || (mstp_gid < STG_ID_DEFAULT)) {
            return SOC_E_PARAM;
        }

        /* enable 802.1s */
        SOC_IF_ERROR_RETURN(
            _drv_mstp_enable_set(unit, TRUE));

        /* write mstp id to vlan entry */
        SOC_IF_ERROR_RETURN(DRV_MEM_READ
            (unit, DRV_MEM_VLAN, vlan_id, 1, (uint32 *)&vlan_entry));
        temp = mstp_gid;
        SOC_IF_ERROR_RETURN(DRV_MEM_FIELD_SET
            (unit, DRV_MEM_VLAN, DRV_MEM_FIELD_SPT_GROUP_ID, 
            (uint32 *)&vlan_entry, &temp));
        SOC_IF_ERROR_RETURN(DRV_MEM_WRITE
            (unit, DRV_MEM_VLAN, vlan_id, 1, (uint32 *)&vlan_entry));
    }

    return SOC_E_NONE;
}

/*
 *  Function : drv_mstp_config_Get
 *
 *  Purpose :
 *      Get the STP id to a specific VLAN.
 *
 *  Parameters :
 *      unit    :   unit id
 *      vlan_id :   vlan id.
 *      mstp_gid :  multiple spanning tree id.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 *      
 *
 */
int 
drv_mstp_config_get(int unit, uint32 vlan_id, uint32 *mstp_gid)
{
    uint32  temp;
    int  enable;
    vlan_1q_entry_t  vlan_entry;

    if (!soc_feature(unit, soc_feature_mstp)){
        /* error checking */
        if (vlan_id > 4095) {
            return SOC_E_PARAM;
        }

        *mstp_gid = 1;
    } else {    
        /* error checking */
        if (vlan_id > 4095) {
            return SOC_E_PARAM;
        }

        /* enable 802.1s */
        SOC_IF_ERROR_RETURN(
            _drv_mstp_enable_get(unit, &enable));

        if (!enable) {
            *mstp_gid = 1;
            /* Disabling the 802.1s (multiple-spanning tree protocol)
            return the default VLAN ID(1) */
            return SOC_E_NONE;
        }

        /* write mstp id to vlan entry */
        SOC_IF_ERROR_RETURN(DRV_MEM_READ
            (unit, DRV_MEM_VLAN, vlan_id, 1, (uint32 *)&vlan_entry));
        SOC_IF_ERROR_RETURN(DRV_MEM_FIELD_GET
            (unit, DRV_MEM_VLAN, DRV_MEM_FIELD_SPT_GROUP_ID, 
            (uint32 *)&vlan_entry, &temp));
        *mstp_gid = temp;
    }

    soc_cm_debug(DK_STP, 
        "drv_mstp_config_get : unit %d, vlan = %d, STP id = %d \n",
         unit, vlan_id, *mstp_gid);

    return SOC_E_NONE;
}

