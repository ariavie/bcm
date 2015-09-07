/*
 * $Id: arl.c 1.9 Broadcom SDK $
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
 */
#include <soc/robo/mcm/driver.h>

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
drv_harrier_arl_learn_enable_set(int unit, soc_pbmp_t pbmp, uint32 mode)
{
    int     rv = SOC_E_NONE;
    uint32  reg_v32, fld_v32 = 0;
    soc_port_t port;

    switch (mode ) {
    case DRV_PORT_HW_LEARN:
    case DRV_PORT_DISABLE_LEARN:
    /* per port */
    PBMP_ITER(pbmp, port) {
        if ((rv = REG_READ_PORT_SEC_CONr(unit, port, &reg_v32)) < 0) {
            return rv;
        }

        if (mode == DRV_PORT_HW_LEARN) { /* enable */
            fld_v32 = 0;
        } else { /* disable */
            fld_v32 = 1;
        }
        soc_PORT_SEC_CONr_field_set(unit, &reg_v32,
            DIS_LEARNf, &fld_v32);
    
        if ((rv = REG_WRITE_PORT_SEC_CONr(unit, port, &reg_v32)) < 0) {
            return rv;
        }
    }
    break;

   /* no support section */    
   case DRV_PORT_SW_LEARN:
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
drv_harrier_arl_learn_enable_get(int unit, soc_port_t port, uint32 *mode)
{
    int     rv = SOC_E_NONE;
    uint32  reg_v32, fld_v32 = 0;


    if ((rv = REG_READ_PORT_SEC_CONr(unit, port, &reg_v32)) < 0) {
        return rv;
    }

    soc_PORT_SEC_CONr_field_get(unit, &reg_v32,
        DIS_LEARNf, &fld_v32);


    if (fld_v32) {
        *mode = DRV_PORT_DISABLE_LEARN; /* This port is in DISABLE SA learn state */
    } else {
        *mode = DRV_PORT_HW_LEARN;
    }
    return rv;
}
 /*
  * Function:
  *  drv_arl_learn_count_set
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
 drv_harrier_arl_learn_count_set(int unit, uint32 port, 
         uint32 type, int value)
 {
     uint32  reg_value = 0, temp = 0, retry = 0;
     int     rv = SOC_E_NONE;
     
     if ((type == DRV_PORT_SA_LRN_CNT_LIMIT) || 
             (type == DRV_PORT_SA_LRN_CNT_INCREASE) || 
             (type == DRV_PORT_SA_LRN_CNT_DECREASE) || 
             (type == DRV_PORT_SA_LRN_CNT_RESET)){
         if (type == DRV_PORT_SA_LRN_CNT_LIMIT) {

             /* Config Port x Dynamic Learning Threshold register */
             SOC_IF_ERROR_RETURN(REG_READ_PORT_MAX_LEARNr
                 (unit, port, &reg_value));
             /* special limit value at -1 is used for unlimited setting */
             if (value == -1) {
                 /* learn limit in Harrier at 0 for unlimited setting */
                 temp = 0;
             } else if (value <= soc_robo_mem_index_max(unit, INDEX(L2_ARLm))) {
                 /* harrier's MAX limit is (ARL size - 1) */
                 temp = (uint32)value;
             } else {
                 return SOC_E_PARAM;
             }
             SOC_IF_ERROR_RETURN(soc_PORT_MAX_LEARNr_field_set
                 (unit, &reg_value, DYN_MAX_MAC_NOf, &temp));
             SOC_IF_ERROR_RETURN(REG_WRITE_PORT_MAX_LEARNr
                 (unit, port, &reg_value));
         } else if (type == DRV_PORT_SA_LRN_CNT_RESET) {
             /* --- make sure the current learned SA number is zero --- */
             /*      1. force the per port's learned SA counter reset.
              *      2. set back to enable normal learned SA counter. 
              */
             SOC_IF_ERROR_RETURN(REG_READ_LRN_CNT_CTLr
                 (unit, &reg_value));

             /* reset port's learned SA # */
             temp = port;
             SOC_IF_ERROR_RETURN(soc_LRN_CNT_CTLr_field_set
                 (unit, &reg_value, PORT_NUM_Rf, &temp));
     
             /* set access control field to 3(RESET) */
             temp = 3;
             SOC_IF_ERROR_RETURN(soc_LRN_CNT_CTLr_field_set
                 (unit, &reg_value, ACC_CTLf, &temp));
             
             /* start learning counter control Process */
             temp = 1;
             SOC_IF_ERROR_RETURN(soc_LRN_CNT_CTLr_field_set
                 (unit, &reg_value, START_DONEf, &temp));

             SOC_IF_ERROR_RETURN(REG_WRITE_LRN_CNT_CTLr
                 (unit, &reg_value));                
     
             /* wait for complete */
             for (retry = 0; retry < SOC_TIMEOUT_VAL; retry++) {
                 SOC_IF_ERROR_RETURN(REG_READ_LRN_CNT_CTLr
                     (unit, &reg_value));

                 SOC_IF_ERROR_RETURN(soc_LRN_CNT_CTLr_field_get
                     (unit, &reg_value, START_DONEf, &temp));
                 if (!temp) {
                     break;
                 }
             }
             if (retry >= SOC_TIMEOUT_VAL) {
                 return SOC_E_TIMEOUT;
             }        
         } else {
             /* DRV_PORT_SA_LRN_CNT_INCREASE || DRV_PORT_SA_LRN_CNT_DECREASE */
             SOC_IF_ERROR_RETURN(REG_READ_LRN_CNT_CTLr(unit, &reg_value));
             
             /* set access control field to 1(INCREASE) or 2(DECREASE) */
             temp = port;
             SOC_IF_ERROR_RETURN(soc_LRN_CNT_CTLr_field_set
                     (unit, &reg_value, PORT_NUM_Rf, &temp));

             /* set access control field to 1(INCREASE) or 2(DECREASE) */
             temp = (type == DRV_PORT_SA_LRN_CNT_INCREASE) ? 1 : 2;
             SOC_IF_ERROR_RETURN(soc_LRN_CNT_CTLr_field_set
                     (unit, &reg_value, ACC_CTLf, &temp));
             
             /* start learning counter control Process */
             temp = 1;
             SOC_IF_ERROR_RETURN(soc_LRN_CNT_CTLr_field_set
                     (unit, &reg_value, START_DONEf, &temp));

             SOC_IF_ERROR_RETURN(REG_WRITE_LRN_CNT_CTLr
                     (unit, &reg_value));
             
             /* wait for complete */
             for (retry = 0; retry < SOC_TIMEOUT_VAL; retry++) {
                 SOC_IF_ERROR_RETURN(REG_READ_LRN_CNT_CTLr
                         (unit, &reg_value));

                 SOC_IF_ERROR_RETURN(soc_LRN_CNT_CTLr_field_get
                         (unit, &reg_value, START_DONEf, &temp));
                 if (!temp) {
                     break;
                 }
             }
             if (retry >= SOC_TIMEOUT_VAL) {
                 return SOC_E_TIMEOUT;
             }        
         }
     } else {
         /* DRV_PORT_SA_LRN_CNT_NUMBER : for read only */
         rv = SOC_E_UNAVAIL;
     }
 
     return rv;
 }
 
 /*
  * Function:
  *  drv_arl_learn_count_get
  * Purpose:
  *  Get the ARL port basis learning count related information.
  * Parameters:
  *  unit    - RoboSwitch unit #
  *  port    - (-1) is allowed to indicate the system based parameter
  *  type    -  list of types : DRV_ARL_LRN_CNT_NUMBER, DRV_ARL_LRN_CNT_LIMIT
  *  value   - (OUT)the get value for indicated type.
  */
 int
 drv_harrier_arl_learn_count_get(int unit, uint32 port, 
         uint32 type, int *value)
 {
     uint32  reg_value = 0, temp = 0;
 
     if (type == DRV_PORT_SA_LRN_CNT_LIMIT){

         /* Get Port x Dynamic Learning Threshold register */
         SOC_IF_ERROR_RETURN(REG_READ_PORT_MAX_LEARNr
             (unit, port, &reg_value));

         SOC_IF_ERROR_RETURN(soc_PORT_MAX_LEARNr_field_get
             (unit, &reg_value, DYN_MAX_MAC_NOf, &temp));

         *value = (int)temp;
     } else if (type == DRV_PORT_SA_LRN_CNT_NUMBER) {

         /* Get Port x Dynamic Learning counter register */
         SOC_IF_ERROR_RETURN(REG_READ_PORT_SA_CNTr
                 (unit, port, &reg_value));
         
         SOC_IF_ERROR_RETURN(soc_PORT_SA_CNTr_field_get
                 (unit, &reg_value, CUR_SA_CNTf, &temp));

         *value = (int)temp;
         
     } else {
         return SOC_E_UNAVAIL;
     }
     
     return SOC_E_NONE;
 }
 
 
 

