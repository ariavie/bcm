/*
 * $Id: trap.c 1.1 Broadcom SDK $
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

/*
 *  Function : drv_dino8_trap_set
 *
 *  Purpose :
 *      Set the trap frame type to CPU.
 *
 *  Parameters :
 *      unit      :  unit id.
 *      bmp       :  port bitmap.
 *      trap_mask :  the mask of trap type.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 */
int 
drv_dino8_trap_set(int unit, soc_pbmp_t bmp, uint32 trap_mask)
{
    uint32  reg_value, temp;

    soc_cm_debug(DK_VERBOSE, 
        "drv_dino8_trap_set: unit = %d, trap mask = 0x%x\n", unit, trap_mask);

    SOC_IF_ERROR_RETURN(REG_READ_GMNGCFGr
        (unit, &reg_value));
    if (trap_mask & DRV_SWITCH_TRAP_IGMP) {
        temp = 1;
    } else {
        temp = 0;
    }

    soc_GMNGCFGr_field_set(unit, &reg_value, 
        IGMPIP_SNOP_ENf, &temp);

    if (trap_mask & DRV_SWITCH_TRAP_BPDU1) {    
        temp = 1;
    } else {
        temp = 0;
    }
    soc_GMNGCFGr_field_set(unit, &reg_value, 
        RXBPDU_ENf, &temp);
    SOC_IF_ERROR_RETURN(REG_WRITE_GMNGCFGr
        (unit, &reg_value));
    
    if ((trap_mask & DRV_SWITCH_TRAP_8021X) ||
        (trap_mask & DRV_SWITCH_TRAP_IPMC) || 
        (trap_mask & DRV_SWITCH_TRAP_GARP) || 
        (trap_mask & DRV_SWITCH_TRAP_ARP) ||
        (trap_mask & DRV_SWITCH_TRAP_8023AD) ||
        (trap_mask & DRV_SWITCH_TRAP_ICMP) ||
        (trap_mask & DRV_SWITCH_TRAP_BPDU2) ||
        (trap_mask & DRV_SWITCH_TRAP_RARP) ||
        (trap_mask & DRV_SWITCH_TRAP_8023AD_DIS) ||
        (trap_mask & DRV_SWITCH_TRAP_BGMP) ||
        (trap_mask & DRV_SWITCH_TRAP_LLDP)) {
        return SOC_E_UNAVAIL;
    }

    /* Enable IP_Multicast */
    if (trap_mask & DRV_SWITCH_TRAP_IGMP) {
        SOC_IF_ERROR_RETURN(REG_READ_NEW_CONTROLr
            (unit, &reg_value));
        temp = 1;
        soc_NEW_CONTROLr_field_set(unit, &reg_value, 
            IP_MULTICASTf, &temp);
        SOC_IF_ERROR_RETURN(REG_WRITE_NEW_CONTROLr
            (unit, &reg_value));
    }

    /* Broadcast packet */
    SOC_IF_ERROR_RETURN(REG_READ_MII_PCTLr
        (unit, CMIC_PORT(unit), &reg_value));
    if (trap_mask & DRV_SWITCH_TRAP_BCST) {
        temp = 1;
    } else {
        temp = 0;
    }
    SOC_IF_ERROR_RETURN(soc_MII_PCTLr_field_set
        (unit, &reg_value, MIRX_BC_ENf, &temp));    
    SOC_IF_ERROR_RETURN(REG_WRITE_MII_PCTLr
        (unit, CMIC_PORT(unit), &reg_value));

    return SOC_E_NONE;
}

/*
 *  Function : drv_dino8_trap_get
 *
 *  Purpose :
 *      Get the trap frame type to CPU.
 *
 *  Parameters :
 *      unit      :  unit id.
 *      port      :  port id.
 *      trap_mask :  the mask of trap type.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 */
int 
drv_dino8_trap_get(int unit, soc_port_t port, uint32 *trap_mask)
{
    uint32  reg_value, temp = 0;

    SOC_IF_ERROR_RETURN(REG_READ_GMNGCFGr
        (unit, &reg_value));
    soc_GMNGCFGr_field_get(unit, &reg_value, 
        IGMPIP_SNOP_ENf, &temp);
    if (temp) {
        *trap_mask |= DRV_SWITCH_TRAP_IGMP;
    }

    soc_GMNGCFGr_field_get(unit, &reg_value, 
        RXBPDU_ENf, &temp);
    if (temp) {
        *trap_mask |= DRV_SWITCH_TRAP_BPDU1;
    }

    /* Broadcast packet */
    SOC_IF_ERROR_RETURN(REG_READ_MII_PCTLr
        (unit, CMIC_PORT(unit), &reg_value));
    SOC_IF_ERROR_RETURN(soc_MII_PCTLr_field_get
        (unit, &reg_value, MIRX_BC_ENf, &temp));

    if (temp) {
        *trap_mask |= DRV_SWITCH_TRAP_BCST;
    }

    soc_cm_debug(DK_VERBOSE, 
        "drv_dino8_trap_get: unit = %d, trap mask = 0x%x\n", unit, *trap_mask);

    return SOC_E_NONE;
}

/*
 *  Function : drv_dino8_snoop_set
 *
 *  Purpose :
 *      Set the Snoop type.
 *
 *  Parameters :
 *      unit      :  unit id.
 *      snoop_mask:  the mask of snoop type.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 */
int 
drv_dino8_snoop_set(int unit, uint32 snoop_mask)
{
    uint32  reg_value, temp;

    soc_cm_debug(DK_VERBOSE, "drv_dino8_snoop_set: \
        unit = %d, snoop mask = 0x%x\n", unit, snoop_mask);

    SOC_IF_ERROR_RETURN(REG_READ_GMNGCFGr
        (unit, &reg_value));
    if (snoop_mask & DRV_SNOOP_IGMP) {    
        temp = 1;
    } else {
        temp = 0;
    }

    soc_GMNGCFGr_field_set(unit, &reg_value, 
        IGMPIP_SNOP_ENf, &temp);
    SOC_IF_ERROR_RETURN(REG_WRITE_GMNGCFGr
        (unit, &reg_value));

    /* enable IP_Multicast */
    if (snoop_mask & DRV_SNOOP_IGMP) {
        SOC_IF_ERROR_RETURN(REG_READ_NEW_CONTROLr
            (unit, &reg_value));
        temp = 1;
        soc_NEW_CONTROLr_field_set(unit, &reg_value, 
            IP_MULTICASTf, &temp);
        SOC_IF_ERROR_RETURN(REG_WRITE_NEW_CONTROLr
            (unit, &reg_value)) ;
    }

    if ((snoop_mask & DRV_SNOOP_ARP) ||
        (snoop_mask & DRV_SNOOP_RARP) ||
        (snoop_mask & DRV_SNOOP_ICMP) ||
        (snoop_mask & DRV_SNOOP_ICMPV6)) {
        return SOC_E_UNAVAIL;
    }
     
    return SOC_E_NONE; 
     
}

/*
 *  Function : drv_dino8_snoop_get
 *
 *  Purpose :
 *      Get the Snoop type.
 *
 *  Parameters :
 *      unit      :  unit id.
 *      snoop_mask:  the mask of snoop type.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 */
int 
drv_dino8_snoop_get(int unit, uint32 *snoop_mask)
{
    uint32  reg_value, temp = 0;
    
    SOC_IF_ERROR_RETURN(REG_READ_GMNGCFGr
        (unit, &reg_value));
    SOC_IF_ERROR_RETURN(soc_GMNGCFGr_field_get
        (unit, &reg_value, IGMPIP_SNOP_ENf, &temp));

    if (temp) {
       *snoop_mask = DRV_SNOOP_IGMP;
    }

    soc_cm_debug(DK_VERBOSE, "drv_dino8_snoop_get: \
        unit = %d, snoop mask = 0x%x\n", unit, *snoop_mask);

    return SOC_E_NONE;
}

/*
 *  Function : drv_dino8_igmp_mld_snoop_mode_get
 *
 *  Purpose :
 *      Get the Snoop mode for IGMP/MLD.
 *
 *  Parameters :
 *      unit      :  unit id.
 *      type      :  (IN) indicate a snoop type.
 *      mode      :  (OUT) indicate a snoop mode.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note :
 */
int 
drv_dino8_igmp_mld_snoop_mode_get(int unit, int type, int *mode)
{
    uint32  reg_val = 0, temp = 0;
    
    if (type != DRV_IGMP_MLD_TYPE_IGMP) {
        return SOC_E_UNAVAIL;
    }

    SOC_IF_ERROR_RETURN(REG_READ_GMNGCFGr
        (unit, &reg_val));
    soc_GMNGCFGr_field_get(unit, &reg_val, 
        IGMPIP_SNOP_ENf, &temp);

    if (!temp) {
        *mode = DRV_IGMP_MLD_MODE_DISABLE;
    } else {
        *mode = DRV_IGMP_MLD_MODE_TRAP;
    }

    return SOC_E_NONE;
}

/*
 *  Function : drv_dino8_igmp_mld_snoop_mode_set
 *
 *  Purpose :
 *      Set the Snoop mode for IGMP/MLD.
 *
 *  Parameters :
 *      unit      :  unit id.
 *      type      :  (IN) indicate a snoop type.
 *      mode      :  (IN) indicate a snoop mode.
 *
 *  Return :
 *      SOC_E_XXX
 *
 *  Note : 
 */
int 
drv_dino8_igmp_mld_snoop_mode_set(int unit, int type, int mode)
{
    uint32  reg_val = 0, temp = 0;

    if (type != DRV_IGMP_MLD_TYPE_IGMP){
        return SOC_E_UNAVAIL;
    }

    /* bcm5389 support trap mode only */
    if (mode == DRV_IGMP_MLD_MODE_SNOOP){
        return SOC_E_CONFIG;
    }

    SOC_IF_ERROR_RETURN(REG_READ_GMNGCFGr
        (unit, &reg_val));

    temp = (mode != DRV_IGMP_MLD_MODE_DISABLE) ? TRUE : FALSE;
    soc_GMNGCFGr_field_set(unit, &reg_val, 
        IGMPIP_SNOP_ENf, &temp);
    SOC_IF_ERROR_RETURN(REG_WRITE_GMNGCFGr
        (unit, &reg_val));
        
    return SOC_E_NONE; 
}

