/*
 * $Id: vlan.c 1.3 Broadcom SDK $
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
 *  Function : drv_vlan_mode_set
 *
 *  Purpose :
 *      Set the VLAN mode. (port-base/tag-base)
 *
 *  Parameters :
 *      unit        :   RoboSwitch unit number.
 *      mode   :   vlan mode.
 *
 *  Return :
 *      SOC_E_NONE
 *
 *  Note :
 *      
 *
 */
int 
drv_vlan_mode_set(int unit, uint32 mode)
{
    uint32	reg_value, temp;

    soc_cm_debug(DK_VLAN, 
        "drv_vlan_mode_set: unit = %d, mode = %d\n",
        unit, mode);
    switch (mode) 
    {
        case DRV_VLAN_MODE_TAG:
            /* set 802.1Q VLAN Enable */
            SOC_IF_ERROR_RETURN(
                REG_READ_VLAN_CTRL0r(unit, &reg_value));
            temp = 1;
            soc_VLAN_CTRL0r_field_set(unit, &reg_value, 
                VLAN_ENf, &temp);
            SOC_IF_ERROR_RETURN(
                REG_WRITE_VLAN_CTRL0r(unit, &reg_value));              
            /* enable GMRP/GVRP been sent to CPU :
             *  - GMRP/GVRP frame won't sent to CPU without set this bit.
             */
            SOC_IF_ERROR_RETURN(
                REG_READ_VLAN_CTRL4r(unit, &reg_value));
            temp = 1; /* Drop frame if ingress vid violation */
            soc_VLAN_CTRL4r_field_set(unit, &reg_value, 
                INGR_VID_CHKf, &temp);

            temp = 1;
            soc_VLAN_CTRL4r_field_set(unit, &reg_value, 
                EN_MGE_REV_GVRPf, &temp);
            soc_VLAN_CTRL4r_field_set(unit, &reg_value, 
                EN_MGE_REV_GMRPf, &temp);
            
            SOC_IF_ERROR_RETURN(
                REG_WRITE_VLAN_CTRL4r(unit, &reg_value));
                
            break;
        case DRV_VLAN_MODE_PORT_BASE:
            /* set 802.1Q VLAN Disable */
            SOC_IF_ERROR_RETURN(
                REG_READ_VLAN_CTRL0r(unit, &reg_value));
            temp = 0;
            soc_VLAN_CTRL0r_field_set(unit, &reg_value, 
                VLAN_ENf, &temp);
            SOC_IF_ERROR_RETURN(
                REG_WRITE_VLAN_CTRL0r(unit, &reg_value));
            break;
        default :
            return SOC_E_PARAM;
    }
    return SOC_E_NONE;
}

/*
 *  Function : drv_vlan_mode_get
 *
 *  Purpose :
 *      Get the VLAN mode. (port-base/tag-base)
 *
 *  Parameters :
 *      unit        :   RoboSwitch unit number.
 *      mode   :   vlan mode.
 *
 *  Return :
 *      SOC_E_NONE
 *
 *  Note :
 *      
 *
 */
int 
drv_vlan_mode_get(int unit, uint32 *mode)
{
    uint32	reg_value, temp = 0;

    SOC_IF_ERROR_RETURN(
        REG_READ_VLAN_CTRL0r(unit, &reg_value));
    soc_VLAN_CTRL0r_field_get(unit, &reg_value, 
                VLAN_ENf, &temp);
    if (temp) {
        *mode = DRV_VLAN_MODE_TAG;
    } else {
        *mode = DRV_VLAN_MODE_PORT_BASE;
    }
    soc_cm_debug(DK_VLAN, 
        "drv_vlan_mode_get: unit = %d, mode = %d\n",
        unit, *mode);
    return SOC_E_NONE;
}

/* config port base vlan */
/*
 *  Function : drv_port_vlan_pvid_set
 *
 *  Purpose :
 *      Set the default tag value of the selected port.
 *
 *  Parameters :
 *      unit    :   RoboSwitch unit number.
 *      port    :   port number.
 *      vid     :   vlan value.
 *      prio    :   priority value
 *
 *  Return :
 *      SOC_E_NONE
 *
 *  Note :
 *      
 *
 */
int 
drv_port_vlan_pvid_set(int unit, uint32 port, uint32 outer_tag, uint32 inner_tag)
{
    uint32	reg_value;
#if defined(BCM_POLAR_SUPPORT) || defined(BCM_NORTHSTAR_SUPPORT) || \
    defined(BCM_NORTHSTARPLUS_SUPPORT)
    uint32  specified_port_num;
#endif /* BCM_POLAR_SUPPORT || BCM_NORTHSTAR_SUPPORT || NS+ */

    soc_cm_debug(DK_VLAN, "drv_port_vlan_pvid_set: \
        unit = %d, port = %d, outer_tag = 0x%x, inner_tag = 0x%x\n",
        unit, port, outer_tag, inner_tag);

#if defined(BCM_POLAR_SUPPORT) || defined(BCM_NORTHSTAR_SUPPORT) || \
    defined(BCM_NORTHSTARPLUS_SUPPORT)
    if (SOC_IS_POLAR(unit)) {
        SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
            (unit, DRV_DEV_PROP_INTERNAL_CPU_PORT_NUM, &specified_port_num));
    } else if (SOC_IS_NORTHSTAR(unit) || SOC_IS_NORTHSTARPLUS(unit)) {
        SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
            (unit, DRV_DEV_PROP_ADDITIONAL_SOC_PORT_NUM, &specified_port_num));
    }
#endif /* BCM_POLAR_SUPPORT || BCM_NORTHSTAR_SUPPORT || NS+ */

    reg_value = outer_tag;
#if defined(BCM_POLAR_SUPPORT) || defined(BCM_NORTHSTAR_SUPPORT) || \
    defined(BCM_NORTHSTARPLUS_SUPPORT)
    if ((SOC_IS_POLAR(unit) || SOC_IS_NORTHSTAR(unit) || 
        SOC_IS_NORTHSTARPLUS(unit)) 
        && (port == specified_port_num)) {
        SOC_IF_ERROR_RETURN(
            REG_WRITE_DEFAULT_1Q_TAG_P7r(unit, &reg_value));
    } else
#endif /* BCM_POLAR_SUPPORT || BCM_NORTHSTAR_SUPPORT || NS+ */
    {
        SOC_IF_ERROR_RETURN(
            REG_WRITE_DEFAULT_1Q_TAGr(unit, port, &reg_value));
    }

    return SOC_E_NONE;
}

/*
 *  Function : drv_port_vlan_pvid_get
 *
 *  Purpose :
 *      Get the default tag value of the selected port.
 *
 *  Parameters :
 *      unit        :   RoboSwitch unit number.
 *      port   :   port number.
 *      vid     :   vlan value.
 *      prio    :   priority value
 *
 *  Return :
 *      SOC_E_NONE
 *
 *  Note :
 *      
 *
 */
int 
drv_port_vlan_pvid_get(int unit, uint32 port, uint32 *outer_tag, uint32 *inner_tag)
{
    uint32	reg_value;
#if defined(BCM_POLAR_SUPPORT) || defined(BCM_NORTHSTAR_SUPPORT) || \
    defined(BCM_NORTHSTARPLUS_SUPPORT)
    uint32  specified_port_num;

    if (SOC_IS_POLAR(unit)) {
        SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
            (unit, DRV_DEV_PROP_INTERNAL_CPU_PORT_NUM, &specified_port_num));
    } else if (SOC_IS_NORTHSTAR(unit) || SOC_IS_NORTHSTARPLUS(unit)) {
        SOC_IF_ERROR_RETURN(DRV_DEV_PROP_GET
            (unit, DRV_DEV_PROP_ADDITIONAL_SOC_PORT_NUM, &specified_port_num));
    }

    if ((SOC_IS_POLAR(unit) || SOC_IS_NORTHSTAR(unit) || 
        SOC_IS_NORTHSTARPLUS(unit)) 
        && (port == specified_port_num)) {
        SOC_IF_ERROR_RETURN(
            REG_READ_DEFAULT_1Q_TAG_P7r(unit, &reg_value));
    } else
#endif /* BCM_POLAR_SUPPORT || BCM_NORTHSTAR_SUPPORT || NS+ */
    {
        SOC_IF_ERROR_RETURN(
            REG_READ_DEFAULT_1Q_TAGr(unit, port, &reg_value));
    }

    *outer_tag = reg_value;
    
    soc_cm_debug(DK_VLAN, "drv_port_vlan_pvid_get: \
        unit = %d, port = %d, outer_tag = 0x%x, inner_tag = 0x%x\n",
        unit, port, *outer_tag, *inner_tag);

    return SOC_E_NONE;
}

