/*
 * $Id: hurricane2.h,v 1.15 Broadcom SDK $
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
 * File:        hurricane2.h
 */

#ifndef _SOC_HURRICANE2_H_
#define _SOC_HURRICANE2_H_

#include <soc/drv.h>
#include <soc/mem.h>
#include <shared/sram.h>


enum soc_hu2_port_mode_e {
    /* WARNING: values given match hardware register; do not modify */
    SOC_HU2_PORT_MODE_QUAD = 0,
    SOC_HU2_PORT_MODE_TRI_012 = 1,
    SOC_HU2_PORT_MODE_TRI_023 = 2,
    SOC_HU2_PORT_MODE_DUAL = 3,
    SOC_HU2_PORT_MODE_SINGLE = 4
};

typedef int (*soc_hurricane2_oam_handler_t)(int unit, soc_field_t fault_field);
typedef int (*soc_hurricane2_oam_ser_handler_t)(int unit, 
    soc_mem_t mem, int index);
extern int soc_hurricane2_misc_init(int);
extern int soc_hurricane2_mmu_init(int);
extern int soc_hu2_port_config_init(int unit, uint16 dev_id);
extern int soc_hurricane2_age_timer_get(int, int *, int *);
extern int soc_hurricane2_age_timer_max_get(int, int *);
extern int soc_hurricane2_age_timer_set(int, int, int);
extern void soc_hurricane2_oam_handler_register(int unit, 
                                         soc_hurricane2_oam_handler_t handler);
extern void soc_hurricane2_oam_ser_handler_register(int unit, 
                               soc_hurricane2_oam_ser_handler_t handler);
extern int soc_hurricane2_oam_ser_process(int unit, 
                                soc_mem_t mem, int index);


extern void soc_hurricane2_parity_error(void *unit_vp, void *d1, void *d2,
                                     void *d3, void *d4);
extern int _soc_hurricane2_mem_parity_control(int unit, soc_mem_t mem,
                                            int copyno, int enable);
extern int soc_hurricane2_ser_mem_clear(int unit, soc_mem_t mem);
extern int soc_hurricane2_port_lanes_set(int unit, soc_port_t port_base,
                                       int lanes, int *cur_lanes,
                                       int *phy_ports, int *phy_ports_len);
extern int soc_hurricane2_port_lanes_get(int unit, soc_port_t port_base,
                                       int *cur_lanes);
extern int soc_hurricane2_tsc_reset(int unit);
extern int soc_hu2_mem_config(int unit, int dev_id);

extern int soc_hu2_temperature_monitor_get(int unit,
          int temperature_max,
          soc_switch_temperature_monitor_t *temperature_array,
          int *temperature_count);
extern int soc_hu2_show_material_process(int unit);
extern int soc_hu2_show_ring_osc(int unit);

extern soc_functions_t soc_hurricane2_drv_funs;

extern int soc_hu2_xq_mem(int unit, soc_port_t port, soc_mem_t *xq);

#define SOC_HU2_LPM_INIT_CHECK(u)        (soc_hu2_lpm_state[(u)] != NULL)
#define SOC_HU2_LPM_STATE(u)             (soc_hu2_lpm_state[(u)])
#define SOC_HU2_LPM_STATE_START(u, pfx)  (soc_hu2_lpm_state[(u)][(pfx)].start)
#define SOC_HU2_LPM_STATE_END(u, pfx)    (soc_hu2_lpm_state[(u)][(pfx)].end)
#define SOC_HU2_LPM_STATE_PREV(u, pfx)  (soc_hu2_lpm_state[(u)][(pfx)].prev)
#define SOC_HU2_LPM_STATE_NEXT(u, pfx)  (soc_hu2_lpm_state[(u)][(pfx)].next)
#define SOC_HU2_LPM_STATE_VENT(u, pfx)  (soc_hu2_lpm_state[(u)][(pfx)].vent)
#define SOC_HU2_LPM_STATE_FENT(u, pfx)  (soc_hu2_lpm_state[(u)][(pfx)].fent)

typedef struct soc_hu2_lpm_state_s {
    int start;  /* start index for this prefix length */
    int end;    /* End index for this prefix length */
    int prev;   /* Previous (Lo to Hi) prefix length with non zero entry count*/
    int next;   /* Next (Hi to Lo) prefix length with non zero entry count */
    int vent;   /* valid entries */
    int fent;   /* free entries */
} soc_hu2_lpm_state_t, *soc_hu2_lpm_state_p;
extern soc_hu2_lpm_state_p soc_hu2_lpm_state[SOC_MAX_NUM_DEVICES];

extern int soc_hu2_lpm_init(int u);
extern int soc_hu2_lpm_deinit(int u);
extern int soc_hu2_lpm_insert(int unit, void *entry);
extern int soc_hu2_lpm_delete(int u, void *key_data);
extern int soc_hu2_lpm_match(int u, void *key_data, void *e, int *index_ptr);
#ifdef BCM_WARM_BOOT_SUPPORT
extern int soc_hu2_lpm_reinit(int unit, int idx, uint32 *lpm_entry);
extern int soc_hu2_lpm_reinit_done(int unit);
#else
#define soc_hu2_lpm_reinit(u, t, s, c) (SOC_E_UNAVAIL)
#endif /* BCM_WARM_BOOT_SUPPORT */
 
#ifdef BCM_WARM_BOOT_SUPPORT_SW_DUMP
extern void soc_hu2_lpm_sw_dump(int unit);
#endif /* BCM_WARM_BOOT_SUPPORT_SW_DUMP */

#if defined(BCM_HURRICANE2_SUPPORT) 
extern int _soc_mem_hu2_fifo_dma_get_read_ptr(int unit,
                                             int chan,
                                             void **host_ptr,
                                             int *count);
extern int _soc_mem_hu2_fifo_dma_advance_read_ptr (int unit,
                                                  int chan,
                                                  int count);
extern soc_reg_t _soc_hu2_fifo_reg_get(int unit, int cmc, int chan, int type);
#endif

#if defined(SER_TR_TEST_SUPPORT)
extern int soc_hu2_ser_inject_error(int unit, uint32 flags, soc_mem_t mem,
                                    int pipeTarget, int block, int index);
extern int soc_hu2_ser_mem_test(int unit, soc_mem_t mem, 
                                        _soc_ser_test_t test_type, int cmd);
extern int soc_hu2_ser_test(int unit, _soc_ser_test_t test_type);
#endif


#endif /* !_SOC_HURRICANE2_H_ */

