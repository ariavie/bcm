/*
 * $Id: arl.c,v 1.1.2.1 Broadcom SDK $
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
 *      Provide some NorthStar-Plus specific ARL soc drivers.
 */
 
#include <shared/bsl.h>

#include <soc/mem.h>
#include <soc/debug.h>
#include <soc/cm.h>
#include <soc/robo.h>
#include <soc/robo/mcm/driver.h>
#include "robo_northstarplus.h"

#define _NSP_LEARN_LIMIT_PBMP_ALL     (0x1BF)
#define _NSP_LEARN_LIMIT_PBMP_ETH     (0x3F)
/*
 * Function:
 *  drv_nsp_arl_learn_count_set
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
drv_nsp_arl_learn_count_set(int unit, uint32 port, 
        uint32 type, int value)
{
    int     sys_based = FALSE;
    uint32  reg_value = 0, temp = 0, en_pbmp = 0;
    uint32  sys_counter = 0, port_counter = 0, limit = 0;
    uint32  reg_sys_cnt = 0, reg_port_cnt = 0;

    if ((int)port == -1){
        sys_based = TRUE;
    }
    
    LOG_INFO(BSL_LS_SOC_ARL,
             (BSL_META_U(unit,
                         "%s,%d: port=%d,type=%d,value=%d\n"),
              FUNCTION_NAME(),__LINE__,port,type,value));
    if (type == DRV_PORT_SA_LRN_CNT_LIMIT) {
        /* In BCM API guide, the limit value at nagtive value means disable. */

        /*
         * 1. max value check
         * 2. assigning the pbmp for enable/disable 
         */
        if (sys_based){
            /* system based setting : */
            if (value > DRV_NSP_SYS_MAX_LEARN_LIMIT) {
                return SOC_E_PARAM;
            }
            
            /* exclude IMP port for enable/disable setting :
             *
             *  - Need to check if port 5 is a ether port
             */
            if (SOC_PBMP_MEMBER(PBMP_E_ALL(unit), 5)){
                en_pbmp = _NSP_LEARN_LIMIT_PBMP_ETH;
            } else {
                en_pbmp = _NSP_LEARN_LIMIT_PBMP_ETH & 
                        ~(1 << 5);
            }
        } else {
            /* port based */
            if (value > DRV_NSP_PORT_MAX_LEARN_LIMIT) {
                return SOC_E_PARAM;
            }
            
            en_pbmp = ((1 << port) & _NSP_LEARN_LIMIT_PBMP_ALL);
        }
        
        SOC_IF_ERROR_RETURN(REG_READ_SA_LIMIT_ENABLEr(unit, 
                &reg_value));
        SOC_IF_ERROR_RETURN(
                soc_SA_LIMIT_ENABLEr_field_get(unit, &reg_value, 
                SA_LIMIT_ENf, &temp));
       if (value < 0) {
            /* feature disable and clear limit value */
            temp &= ~(en_pbmp);
        } else {
            /* feature enable and set limit value */
            temp |= en_pbmp;
        }
        SOC_IF_ERROR_RETURN(
                soc_SA_LIMIT_ENABLEr_field_set(unit, &reg_value, 
                SA_LIMIT_ENf, &temp));
        SOC_IF_ERROR_RETURN(REG_WRITE_SA_LIMIT_ENABLEr(unit, 
                &reg_value));

        /* SA Learn Limit setting */
        limit = (value < 0) ? 0 :  value;
        if (sys_based){
            SOC_IF_ERROR_RETURN(REG_READ_TOTAL_SA_LIMIT_CTLr(unit, 
                    &reg_value));
            SOC_IF_ERROR_RETURN(
                    soc_TOTAL_SA_LIMIT_CTLr_field_set(unit, &reg_value, 
                    TOTAL_SA_LRN_CNT_LIMf, &limit));
            SOC_IF_ERROR_RETURN(REG_WRITE_TOTAL_SA_LIMIT_CTLr(unit, 
                    &reg_value));
        } else {
            if (port == 7) {
                SOC_IF_ERROR_RETURN(REG_READ_PORT_7_SA_LIMIT_CTLr(unit, 
                        &reg_value));
            } else if (port == 8){
                SOC_IF_ERROR_RETURN(REG_READ_PORT_8_SA_LIMIT_CTLr(unit, 
                        &reg_value));
            } else {
                SOC_IF_ERROR_RETURN(REG_READ_PORT_N_SA_LIMIT_CTLr(unit, 
                        port, &reg_value));
            }
            SOC_IF_ERROR_RETURN(
                    soc_PORT_N_SA_LIMIT_CTLr_field_set(unit, &reg_value, 
                    SA_LRN_CNT_LIMf, &limit));
            if (port == 7) {
                SOC_IF_ERROR_RETURN(REG_WRITE_PORT_7_SA_LIMIT_CTLr(unit, 
                        &reg_value));
            } else if (port == 8){
                SOC_IF_ERROR_RETURN(REG_WRITE_PORT_8_SA_LIMIT_CTLr(unit, 
                        &reg_value));
            } else {
                SOC_IF_ERROR_RETURN(REG_WRITE_PORT_N_SA_LIMIT_CTLr(unit, 
                        port, &reg_value));
            }
        }
    } else if ((type == DRV_PORT_SA_LRN_CNT_INCREASE) || 
            (type == DRV_PORT_SA_LRN_CNT_DECREASE)) {
        /* direct increase/decrease one on SA Learn count register :
         *
         * Note : 
         *  1. both system and port based counter must be handled.
         *  2. The process is not trusted, once the learn counter 
         *      increase/decrease process reqeusted but L2 table 
         *      is not frozen.
         *  3. The max value is "SA_LRN_CNT_LIM"
         *  4. Check items :
         *      - will SA learn counter must be updated while 
         *          learn limit feature is not enabled?
         *      - Should both SYS/PORT learned counter been 
         *          updated?
         */

        /* get current counter value and limit value */
        SOC_IF_ERROR_RETURN(REG_READ_TOTAL_SA_LRN_CNTRr(unit, 
                &reg_sys_cnt));
        SOC_IF_ERROR_RETURN(
                soc_TOTAL_SA_LRN_CNTRr_field_get(unit, &reg_sys_cnt, 
                TOTAL_SA_LRN_CNT_NOf, &sys_counter));
        if (port == 7) {
            SOC_IF_ERROR_RETURN(REG_READ_PORT_7_SA_LRN_CNTRr(unit, 
                    &reg_port_cnt));
        } else if (port == 8){
            SOC_IF_ERROR_RETURN(REG_READ_PORT_8_SA_LRN_CNTRr(unit, 
                    &reg_port_cnt));
        } else {
            SOC_IF_ERROR_RETURN(REG_READ_PORT_N_SA_LRN_CNTRr(unit, 
                    port, &reg_port_cnt));
        }
        SOC_IF_ERROR_RETURN(
                soc_PORT_N_SA_LRN_CNTRr_field_get(unit, &reg_port_cnt, 
                SA_LRN_CNT_NOf, &port_counter));

        if ((sys_counter >= DRV_NSP_SYS_MAX_LEARN_LIMIT) || 
                (port_counter >= DRV_NSP_PORT_MAX_LEARN_LIMIT)){
            /* return FULL while leanred counter reach MAX value */
            if (type == DRV_PORT_SA_LRN_CNT_INCREASE) {
                return SOC_E_FULL;
            }
        } else if ((sys_counter == 0) || 
                (port_counter == 0)){
            if (type == DRV_PORT_SA_LRN_CNT_DECREASE) {
                /* improper condition, show warning message */
                LOG_INFO(BSL_LS_SOC_ARL,
                         (BSL_META_U(unit,
                                     "SA learned count is zero already.\n")));
            }
        }

        /* check if learn limit is enabled :
        *   
        *   Increase/decrease learn counter occurred on the learn limit 
        *   enabled ports only. (To comply the same behavior with Chip 
        *   behavior.)
        */
        SOC_IF_ERROR_RETURN(REG_READ_SA_LIMIT_ENABLEr(unit, 
                &reg_value));
        SOC_IF_ERROR_RETURN(
                soc_SA_LIMIT_ENABLEr_field_get(unit, &reg_value, 
                SA_LIMIT_ENf, &temp));
        if ((temp & _NSP_LEARN_LIMIT_PBMP_ALL) & (1 << port)) {
            /* check if limit is reached (both sys and port counters) */
            SOC_IF_ERROR_RETURN(REG_READ_TOTAL_SA_LIMIT_CTLr(unit, 
                    &reg_value));
            SOC_IF_ERROR_RETURN(
                    soc_TOTAL_SA_LIMIT_CTLr_field_get(unit, &reg_value, 
                    TOTAL_SA_LRN_CNT_LIMf, &limit));
            if (sys_counter >= limit) {
                LOG_INFO(BSL_LS_SOC_ARL,
                         (BSL_META_U(unit,
                                     "%s,%d,rach System Learn LIMIT!!\n"),
                          FUNCTION_NAME(),__LINE__));
                return SOC_E_FULL;
            }

            if (port == 7) {
                SOC_IF_ERROR_RETURN(REG_READ_PORT_7_SA_LIMIT_CTLr(unit, 
                        &reg_value));
            } else if (port == 8){
                SOC_IF_ERROR_RETURN(REG_READ_PORT_8_SA_LIMIT_CTLr(unit, 
                        &reg_value));
            } else {
                SOC_IF_ERROR_RETURN(REG_READ_PORT_N_SA_LIMIT_CTLr(unit, 
                        port, &reg_value));
            }
            SOC_IF_ERROR_RETURN(
                    soc_PORT_N_SA_LIMIT_CTLr_field_get(unit, &reg_value, 
                    SA_LRN_CNT_LIMf, &limit));
            if (port_counter >= limit) {
                LOG_INFO(BSL_LS_SOC_ARL,
                         (BSL_META_U(unit,
                                     "%s,%d,rach Port %d Learn LIMIT!!\n"),
                          FUNCTION_NAME(),__LINE__, port));
                return SOC_E_FULL;
            }
    
            /* update counter value */
            if (type == DRV_PORT_SA_LRN_CNT_INCREASE){
                sys_counter ++;
                port_counter ++;
            } else {
                sys_counter --;
                port_counter --;
            }
    
            SOC_IF_ERROR_RETURN(
                    soc_TOTAL_SA_LRN_CNTRr_field_set(unit, &reg_sys_cnt, 
                    TOTAL_SA_LRN_CNT_NOf, &sys_counter));
            SOC_IF_ERROR_RETURN(REG_WRITE_TOTAL_SA_LRN_CNTRr(unit, 
                    &reg_sys_cnt));
    
            SOC_IF_ERROR_RETURN(
                    soc_PORT_N_SA_LRN_CNTRr_field_set(unit, &reg_port_cnt, 
                    SA_LRN_CNT_NOf, &port_counter));
            if (port == 7) {
                SOC_IF_ERROR_RETURN(REG_WRITE_PORT_7_SA_LRN_CNTRr(unit, 
                        &reg_port_cnt));
            } else if (port == 8){
                SOC_IF_ERROR_RETURN(REG_WRITE_PORT_8_SA_LRN_CNTRr(unit, 
                        &reg_port_cnt));
            } else {
                SOC_IF_ERROR_RETURN(REG_WRITE_PORT_N_SA_LRN_CNTRr(unit, 
                        port, &reg_port_cnt));
            }
        }

    } else if  (type == DRV_PORT_SA_LRN_CNT_RESET) {
        /* reset port based SA Learn count register 
         *
         * Note :
         *  1. both port and system learn counters and overlimit counters 
         *      will be all reset.
         */
        SOC_IF_ERROR_RETURN(REG_READ_SA_LRN_CNTR_RSTr(unit, 
                &reg_value));
        temp = 1;
        SOC_IF_ERROR_RETURN(
                soc_SA_LRN_CNTR_RSTr_field_set(unit, &reg_value, 
                TOTAL_SA_LRN_CNTR_RSTf, &temp));
        temp = _NSP_LEARN_LIMIT_PBMP_ALL;
        SOC_IF_ERROR_RETURN(
                soc_SA_LRN_CNTR_RSTr_field_set(unit, &reg_value, 
                PORT_SA_LRN_CNTR_RSTf, &temp));
        SOC_IF_ERROR_RETURN(REG_WRITE_SA_LRN_CNTR_RSTr(unit, 
                &reg_value));
        
        SOC_IF_ERROR_RETURN(REG_READ_SA_OVERLIMIT_CNTR_RSTr(unit, 
                &reg_value));
        temp = _NSP_LEARN_LIMIT_PBMP_ALL;
        SOC_IF_ERROR_RETURN(
                soc_SA_OVERLIMIT_CNTR_RSTr_field_set(unit, &reg_value, 
                PORT_SA_OVER_LIMIT_CNTR_RSTf, &temp));
        SOC_IF_ERROR_RETURN(REG_WRITE_SA_OVERLIMIT_CNTR_RSTr(unit, 
                &reg_value));
        
    } else {
        return SOC_E_UNAVAIL;
    }
    
    return SOC_E_NONE;
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
drv_nsp_arl_learn_count_get(int unit, uint32 port, 
        uint32 type, int *value)
{
    int     sys_based = FALSE, is_enabled = FALSE;
    uint32  reg_value = 0, temp = 0;

    if ((int)port == -1){
        sys_based = TRUE;
    }

    LOG_INFO(BSL_LS_SOC_ARL,
             (BSL_META_U(unit,
                         "%s,%d: port=%d,type=%d\n"),
              FUNCTION_NAME(),__LINE__,port,type));
    if (type == DRV_PORT_SA_LRN_CNT_LIMIT){

        SOC_IF_ERROR_RETURN(REG_READ_SA_LIMIT_ENABLEr(unit, 
                &reg_value));
        SOC_IF_ERROR_RETURN(
                soc_SA_LIMIT_ENABLEr_field_get(unit, &reg_value, 
                SA_LIMIT_ENf, &temp));

        if (sys_based) {
            if ((temp & _NSP_LEARN_LIMIT_PBMP_ALL) != 0){
                /* means not all ports is disabled */
                is_enabled = TRUE;
            }
        } else {
            if ((temp & _NSP_LEARN_LIMIT_PBMP_ALL) & (1 << port)) {
                is_enabled = TRUE;
            }
        }

        if (is_enabled) {
            if (sys_based){
                SOC_IF_ERROR_RETURN(REG_READ_TOTAL_SA_LIMIT_CTLr(unit, 
                        &reg_value));
                SOC_IF_ERROR_RETURN(
                        soc_TOTAL_SA_LIMIT_CTLr_field_get(unit, &reg_value, 
                        TOTAL_SA_LRN_CNT_LIMf, &temp));
            } else {
                if (port == 7) {
                    SOC_IF_ERROR_RETURN(REG_READ_PORT_7_SA_LIMIT_CTLr(unit, 
                            &reg_value));
                } else if (port == 8){
                    SOC_IF_ERROR_RETURN(REG_READ_PORT_8_SA_LIMIT_CTLr(unit, 
                            &reg_value));
                } else {
                    SOC_IF_ERROR_RETURN(REG_READ_PORT_N_SA_LIMIT_CTLr(unit, 
                            port, &reg_value));
                }
                SOC_IF_ERROR_RETURN(
                        soc_PORT_N_SA_LIMIT_CTLr_field_get(unit, &reg_value, 
                        SA_LRN_CNT_LIMf, &temp));
            }
            *value = temp;
        } else {
            *value = -1;
        }

    } else if (type == DRV_PORT_SA_LRN_CNT_NUMBER) {

        if (sys_based){
            SOC_IF_ERROR_RETURN(REG_READ_TOTAL_SA_LRN_CNTRr(unit, 
                    &reg_value));
            SOC_IF_ERROR_RETURN(
                    soc_TOTAL_SA_LRN_CNTRr_field_get(unit, &reg_value, 
                    TOTAL_SA_LRN_CNT_NOf, &temp));
        } else {
            if (port == 7) {
                SOC_IF_ERROR_RETURN(REG_READ_PORT_7_SA_LRN_CNTRr(unit, 
                        &reg_value));
            } else if (port == 8){
                SOC_IF_ERROR_RETURN(REG_READ_PORT_8_SA_LRN_CNTRr(unit, 
                        &reg_value));
            } else {
                SOC_IF_ERROR_RETURN(REG_READ_PORT_N_SA_LRN_CNTRr(unit, 
                        port, &reg_value));
            }
            SOC_IF_ERROR_RETURN(
                    soc_PORT_N_SA_LRN_CNTRr_field_get(unit, &reg_value, 
                    SA_LRN_CNT_NOf, &temp));
        }

        *value = temp;
    } else {
        return SOC_E_UNAVAIL;
    }
    
    return SOC_E_NONE;
}

