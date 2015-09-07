/* $Id: katana.h,v 1.20 Broadcom SDK $
 * $Id: 
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
 * File:        katana.h
 */

#ifndef _SOC_KATANA_H_
#define _SOC_KATANA_H_

#include <soc/drv.h>

typedef enum {
    _SOC_KT_COSQ_NODE_LEVEL_ROOT,
    _SOC_KT_COSQ_NODE_LEVEL_L0,
    _SOC_KT_COSQ_NODE_LEVEL_L1,
    _SOC_KT_COSQ_NODE_LEVEL_L2,
    _SOC_KT_COSQ_NODE_LEVEL_MAX
}_soc_katana_cosq_node_level_t;

extern soc_functions_t soc_katana_drv_funs;

extern int _soc_katana_mem_parity_control(int unit, soc_mem_t mem,
                                                       int copyno, int enable);
extern void soc_katana_stat_nack(int unit, int *fixed);
extern void soc_katana_parity_error(void *unit_vp, void *d1, void *d2,
                                     void *d3, void *d4);
extern void soc_katana_mem_nack(void *unit_vp, void *d1, void *d2,
                                 void *d3, void *d4);
extern int soc_katana_pipe_mem_clear(int unit);
extern int soc_katana_ser_mem_clear(int unit, soc_mem_t mem);
extern void soc_katana_ser_fail(int unit);
extern int soc_katana_get_egr_perq_xmt_counters_size(int unit,
                                                      int *num_entries_x,
                                                      int *num_entries_y);
extern int soc_katana_get_port_mapping(int unit, uint16 dev_id,
                                        int *bandwidth,
                                        const int **speed_max,
                                        const int **p2l_mapping,
                                        const int **p2m_mapping);
extern int soc_katana_init_port_mapping(int unit);
extern int soc_katana_num_cosq_init(int unit);
extern void _kt_lls_workaround(int unit);
extern int soc_katana_temperature_monitor_get(int unit,
          int temperature_max,
          soc_switch_temperature_monitor_t *temperature_array,
          int *temperature_count);
extern int soc_katana_show_ring_osc(int unit);

typedef int (*soc_kt_oam_handler_t)(int unit, soc_field_t fault_field);
extern void soc_kt_oam_handler_register(int unit, soc_kt_oam_handler_t handler);

#if defined(BCM_KATANA_SUPPORT) && defined(BCM_CMICM_SUPPORT)
extern int _soc_mem_kt_fifo_dma_start (int unit,
                                       int chan,
                                       soc_mem_t mem,
                                       int copyno,
                                       int host_entries,
                                       void * host_buf);
extern int _soc_mem_kt_fifo_dma_stop (int unit, int chan);
extern int _soc_mem_kt_fifo_dma_get_read_ptr(int unit,
                                             int chan,
                                             void **host_ptr,
                                             int *count);
extern int _soc_mem_kt_fifo_dma_advance_read_ptr (int unit,
                                                  int chan,
                                                  int count);
extern soc_reg_t _soc_kt_fifo_reg_get(int unit, int cmc, int chan, int type);
#endif

/*
 * Base queue definitions
 */
#define KATANA_BASE_QUEUES_MAX     1024

/*
 * Shaper definitions
 */
#define KATANA_SHAPER_RATE_MIN_EXPONENT                           0
#define KATANA_SHAPER_RATE_MAX_EXPONENT                           13
#define KATANA_SHAPER_RATE_MIN_MANTISSA                           0
#define KATANA_SHAPER_RATE_MAX_MANTISSA                           1023
#define KATANA_SHAPER_BURST_MIN_EXPONENT                          0
#define KATANA_SHAPER_BURST_MAX_EXPONENT                          14
#define KATANA_SHAPER_BURST_MIN_MANTISSA                          0
#define KATANA_SHAPER_BURST_MAX_MANTISSA                          127

typedef struct katanaRateInfo_s {
    uint32 rate;
    uint32 exponent;
    uint32 mantissa;
} katanaRateInfo_t;

typedef struct katanaShaperInfo_s {
    katanaRateInfo_t     *basic_rate_info;
    katanaRateInfo_t     *burst_size_info;
} katanaShaperInfo_t;

extern int
soc_katana_get_shaper_rate_info(int unit, uint32 rate,
                                 uint32 *rate_mantissa, uint32 *rate_exponent);
extern int
soc_katana_get_shaper_burst_info(int unit, uint32 rate,
                                 uint32 *burst_mantissa, uint32 *burst_exponent,
                                 uint32 flags);
extern int
soc_katana_compute_shaper_rate(int unit, uint32 rate_mantissa, 
                               uint32 rate_exponent, uint32 *rate);
extern int
soc_katana_compute_refresh_rate(int unit, uint32 rate_exp, 
                                uint32 rate_mantissa, 
                                uint32 burst_exp, uint32 burst_mantissa,
                                uint32 *cycle_sel);
extern int
soc_katana_compute_shaper_burst(int unit, uint32 burst_mantissa, 
                                uint32 burst_exponent, uint32 *rate);
extern void soc_kt_oam_error(void *unit_vp, void *d1, void *d2,
                                      void *d3, void *d4);
extern int soc_kt_lls_bmap_alloc(int unit);
extern int soc_kt_port_flush(int unit, int port, int enable);

extern int soc_kt_cosq_max_bucket_set(int unit, int port,
                                   int index, uint32 level);
#endif	/* !_SOC_KATANA_H_ */

