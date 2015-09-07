/*
 * $Id: arl.c,v 1.5 Broadcom SDK $
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
 * File:    arl.c
 * Purpose: 
 *      Provide some TB specific ARL soc drivers.
 */
 
#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/robo.h>
#include <soc/robo/mcm/driver.h>
#include "robo_tbx.h"

/*
 * Function:
 *  drv_tbx_arl_learn_enable_set
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
drv_tbx_arl_learn_enable_set(int unit, soc_pbmp_t pbmp, uint32 mode)
{
    int         rv = SOC_E_NONE;

    if (SOC_PBMP_IS_NULL(pbmp)){
        return SOC_E_PARAM;
    }
  
    switch (mode ) {
        case DRV_PORT_HW_LEARN:
            /*  Enable software learning :
             *  - SW_LEARN will be stopped internally.
             */
            SOC_IF_ERROR_RETURN(DRV_PORT_SET(
                    unit, pbmp, DRV_PORT_PROP_SW_LEARN_MODE, 0));
            /* Enable SA learning */
            SOC_IF_ERROR_RETURN(DRV_PORT_SET(
                    unit, pbmp, DRV_PORT_PROP_DISABLE_LEARN, 0));
            break;
        case DRV_PORT_DISABLE_LEARN:
            /* Disable software learning */
            SOC_IF_ERROR_RETURN(DRV_PORT_SET(
                    unit, pbmp, DRV_PORT_PROP_SW_LEARN_MODE, 0));
            /* Disable SA learning */
            SOC_IF_ERROR_RETURN(DRV_PORT_SET(
                    unit, pbmp, DRV_PORT_PROP_DISABLE_LEARN, 1));
       	    break;
       	case DRV_PORT_SW_LEARN:
       	    /*  Enable software learning :
       	     *  - HW_LEARN will be stopped internally.
       	     */
            SOC_IF_ERROR_RETURN(DRV_PORT_SET(
                    unit, pbmp, DRV_PORT_PROP_SW_LEARN_MODE, 1));
            /* Enable SA learning */
            SOC_IF_ERROR_RETURN(DRV_PORT_SET(
                    unit, pbmp, DRV_PORT_PROP_DISABLE_LEARN, 0));
       	    break;
        default:
            rv = SOC_E_PARAM;
    }
    
    return rv;
}

/*
 * Function:
 *  drv_tbx_arl_learn_enable_get
 * Purpose:
 *  Setting per port SA learning process.
 * Parameters:
 *  unit    - RoboSwitch unit #
 *  port    - port
 *  mode   - Port learn mode
 */
int
drv_tbx_arl_learn_enable_get(int unit, soc_port_t port, uint32 *mode)
{
    uint32  sw_learn = 0, dis_salearn = 0;

    /* Check software learn setting */
    SOC_IF_ERROR_RETURN(DRV_PORT_GET(
            unit, port, DRV_PORT_PROP_SW_LEARN_MODE, &sw_learn));
    /* Check SA learn setting */
    SOC_IF_ERROR_RETURN(DRV_PORT_GET(
            unit, port, DRV_PORT_PROP_DISABLE_LEARN, &dis_salearn));
    if (dis_salearn){
        *mode = DRV_PORT_DISABLE_LEARN;
    } else {
        if (sw_learn){
            *mode = DRV_PORT_SW_LEARN;
        } else {
            *mode = DRV_PORT_HW_LEARN;
        }
    }
    
    return SOC_E_NONE;
}

/*
 * Function:
 *  drv_tbx_arl_learn_count_set
 * Purpose:
 *  Get the ARL port basis learning count related information.
 * Parameters:
 *  unit    - RoboSwitch unit #
 *  port    - (-1) is allowed to indicate the system based parameter
 *  type    -  list of types : DRV_ARL_LRN_CNT_LIMIT, DRV_ARL_LRN_CNT_INCRASE
 *              DRV_ARL_LRN_CNT_DECREASE, DRV_ARL_LRN_CNT_RESET, 
 *              
 *  value   - the set value for indicated type.
 *               
 */
int
drv_tbx_arl_learn_count_set(int unit, uint32 port, 
        uint32 type, int value)
{
    int     rv = SOC_E_NONE;
    
    if ((type == DRV_PORT_SA_LRN_CNT_LIMIT) || 
            (type == DRV_PORT_SA_LRN_CNT_INCREASE) || 
            (type == DRV_PORT_SA_LRN_CNT_DECREASE) || 
            (type == DRV_PORT_SA_LRN_CNT_RESET)){
        /* call to a internal driver routine for all related process is 
         * designed as SA_LRN_CNT table operation
         */
        if (type == DRV_PORT_SA_LRN_CNT_LIMIT){
            /* special limit value at -1 is used for unlimited setting */
            if (value == -1) {
                /* learn limit in TBX at 0 for unlimited setting */
                value = 0;
            } else if (value > 
                (soc_robo_mem_index_max(unit, INDEX(L2_ARLm)) + 1)) {
                /* TBX's MAX limit is ARL table size */
                return SOC_E_PARAM;
            }
        }
        rv = _drv_tbx_mem_sa_lrncnt_control_set(unit, port, type, value);
    } else {
        rv = SOC_E_UNAVAIL;
    }
    
    return rv;
}

/*
 * Function:
 *  drv_tbx_arl_learn_count_get
 * Purpose:
 *  Get the ARL port basis learning count related information.
 * Parameters:
 *  unit    - RoboSwitch unit #
 *  port    - (-1) is allowed to indicate the system based parameter
 *  type    -  list of types : DRV_ARL_LRN_CNT_NUMBER, DRV_ARL_LRN_CNT_LIMIT
 *  value   - (OUT)the get value for indicated type.
 */
int
drv_tbx_arl_learn_count_get(int unit, uint32 port, 
        uint32 type, int *value)
{
    int rv;
    uint32  count_num = 0, count_limit = 0;
    sa_lrn_cnt_entry_t  temp_entry;
    
    /* read entry value on SA_LRN_CNT_TABLE */
    rv = MEM_READ_SA_LRN_CNTm(unit, port, (uint32 *)&temp_entry);
    SOC_IF_ERROR_RETURN(rv);

    /* reset the proper value per user requested type. */
    *value = 0;
    
    /* CHECKME : 
    *   1.port_set with DRV_PORT_PROP_L2_LEARN_LIMIT_PORT_ACTION_DROP, 
     *      DRV_PORT_PROP_L2_LEARN_LIMIT_PORT_ACTION_CPU and 
     *      DRV_PORT_PROP_L2_LEARN_LIMIT_PORT_ACTION_NONE may set the same 
     *      register when type == DRV_PORT_SA_LRN_CNT_LIMIT!!
     *  2. when type == DRV_PORT_SA_LRN_CNT_NUMBER, check if there is any 
     *      other process in SDK modified the same field but not through this
     *      driver service interface.
     */
    if (type == DRV_PORT_SA_LRN_CNT_LIMIT){
        rv = soc_SA_LRN_CNTm_field_get(unit, (uint32 *)&temp_entry, 
            SA_LRN_CNT_LIMf, &count_limit);
        SOC_IF_ERROR_RETURN(rv);

        *value = count_limit;
    } else if (type == DRV_PORT_SA_LRN_CNT_NUMBER) {
        rv = soc_SA_LRN_CNTm_field_get(unit, (uint32 *)&temp_entry, 
            SA_LRN_CNT_NOf, &count_num);
        SOC_IF_ERROR_RETURN(rv);
        *value = count_num;
    } else {
        return SOC_E_UNAVAIL;
    }
    
    return SOC_E_NONE;
}

