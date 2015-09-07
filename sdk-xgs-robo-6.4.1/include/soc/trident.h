/*
 * $Id: trident.h,v 1.26 Broadcom SDK $
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
 * File:        trident.h
 */

#ifndef _SOC_TRIDENT_H_
#define _SOC_TRIDENT_H_

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/error.h>

typedef enum soc_td_drop_limit_alpha_value_e {
    SOC_TD_COSQ_DROP_LIMIT_ALPHA_1_64, /* Use 1/64 as the alpha value for
                                           dynamic threshold */
    SOC_TD_COSQ_DROP_LIMIT_ALPHA_1_32, /* Use 1/32 as the alpha value for
                                           dynamic threshold */
    SOC_TD_COSQ_DROP_LIMIT_ALPHA_1_16, /* Use 1/16 as the alpha value for
                                           dynamic threshold */
    SOC_TD_COSQ_DROP_LIMIT_ALPHA_1_8, /* Use 1/8 as the alpha value for
                                           dynamic threshold */                                          
    SOC_TD_COSQ_DROP_LIMIT_ALPHA_1_4, /* Use 1/4 as the alpha value for
                                           dynamic threshold */
    SOC_TD_COSQ_DROP_LIMIT_ALPHA_1_2, /* Use 1/2 as the alpha value for
                                           dynamic threshold */
    SOC_TD_COSQ_DROP_LIMIT_ALPHA_1,  /* Use 1 as the alpha value for dynamic
                                           threshold */
    SOC_TD_COSQ_DROP_LIMIT_ALPHA_2,  /* Use 2 as the alpha value for dynamic
                                           threshold */
    SOC_TD_COSQ_DROP_LIMIT_ALPHA_4,  /* Use 4 as the alpha value for dynamic
                                           threshold */
    SOC_TD_COSQ_DROP_LIMIT_ALPHA_8,  /* Use 8 as the alpha value for dynamic
                                           threshold */
    SOC_TD_COSQ_DROP_LIMIT_ALPHA_COUNT /* Must be the last entry! */
} soc_td_drop_limit_alpha_value_t;

extern soc_functions_t soc_trident_drv_funs;
extern int _soc_trident_mem_parity_control(int unit, soc_mem_t mem,
                                           int copyno, int enable);
extern int soc_trident_ser_mem_clear(int unit, soc_mem_t mem);
extern int soc_trident_pipe_select(int unit, int egress, int pipe);
extern int soc_trident_get_egr_perq_xmt_counters_size(int unit,
                                                      int *num_entries_x,
                                                      int *num_entries_y);
extern int soc_trident_port_config_init(int unit, uint16 dev_id);
extern int soc_trident_num_cosq_init(int unit);
extern int soc_trident_mmu_config_init(int unit, int test_only);
extern int soc_trident_mem_config(int unit);
extern int soc_trident_temperature_monitor_get(int unit,
          int temperature_max,
          soc_switch_temperature_monitor_t *temperature_array,
          int *temperature_count);
extern int soc_trident_show_material_process(int unit);
extern int soc_trident_show_ring_osc(int unit);

extern int soc_trident_cmic_rate_param_get(int unit, int *dividend, int *divisor);
extern int soc_trident_cmic_rate_param_set(int unit, int dividend, int divisor);

#if defined(SER_TR_TEST_SUPPORT)
extern soc_error_t soc_td_ser_inject_error(int unit, uint32 flags,
                                           soc_mem_t mem, int pipeTarget,
                                           int block, int index);
extern soc_error_t soc_td_ser_mem_test(int unit, soc_mem_t mem,
                                       _soc_ser_test_t test_type, int cmd);
extern soc_error_t soc_td_ser_cmd_print(int unit, soc_mem_t mem);
extern soc_error_t soc_td_ser_test(int unit,_soc_ser_test_t test_type);
extern soc_error_t soc_td_ser_error_injection_support(int unit,
                                                      soc_mem_t mem,
                                                      int pipe);
extern int soc_td_ser_test_overlay (int unit, _soc_ser_test_t test_type);
#endif /*#if defined(SER_TR_TEST_SUPPORT)*/

#endif	/* !_SOC_TRIDENT_H_ */
