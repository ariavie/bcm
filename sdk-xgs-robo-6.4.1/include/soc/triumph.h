/*
 * $Id: triumph.h,v 1.29 Broadcom SDK $
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
 * File:        triumph.h
 */

#ifndef _SOC_TRIUMPH_H_
#define _SOC_TRIUMPH_H_

#include <soc/drv.h>

typedef struct tr_ext_sram_bist_s {
    /* Parameters for the fields of ESx_DTU_LTE_xxx registers */
    uint32 d0r_0;       /* Data pattern for DQ[17: 0] on first rising edge */
    uint32 d0r_1;       /* Data pattern for DQ[35:18] on first rising edge */
    uint32 d0f_0;       /* Data pattern for DQ[17: 0] on first falling edge */
    uint32 d0f_1;       /* Data pattern for DQ[35:18] on first falling edge */
    uint32 d1r_0;       /* Data pattern for DQ[17: 0] on second rising edge */
    uint32 d1r_1;       /* Data pattern for DQ[35:18] on second rising edge */
    uint32 d1f_0;       /* Data pattern for DQ[17: 0] on second falling edge */
    uint32 d1f_1;       /* Data pattern for DQ[35:18] on second falling edge */
    int adr0;           /* Start address */
    int adr1;           /* End address (can be optional in some adr_mode) */
    int loop_mode;      /* 00:WW, 01:RR, 10:WR 11:WW-RR */
    int adr_mode;       /* 00:ALT_A0A1, 01:INC1, 10:INC2, 11:ONLY_A0 */
    int cmp_em_rdat_fr; /* 1: use fall_rise for compare
                         * 0: use rise_fall for compare */
    int em_latency;     /* External memory read latency */
    int w2r_nops;       /* Number of nops between write to read */
    int r2w_nops;       /* Number of nops between read to write */
    int wdoebr;         /* wdoeb_rise */
    int wdoebf;         /* wdoeb_fall */
    int wdmr;           /* Write data mask rise */
    int wdmf;           /* Write data mask fall */
    int rdmr;           /* Read data mask rise */
    int rdmf;           /* Read data mask fall */
    int err_cnt;        /* Error count */
    int err_bitmap;     /* Error bit map */
    uint32 err_adr;     /* Error address */
    uint32 err_dr_0;    /* Error data for DQ[17: 0] on rising edge */
    uint32 err_dr_1;    /* Error data for DQ[35:18] on rising edge */
    uint32 err_df_0;    /* Error data for DQ[17: 0] on falling edge */
    uint32 err_df_1;    /* Error data for DQ[35:18] on falling edge */

    /* Non-BIST engine parameter */
    uint32 bg_tcam_loop_count; /* Enable tcam bist in background if > 1 */
} tr_ext_sram_bist_t;

typedef struct _soc_tr_l2e_ppa_info_s {
    uint32      data;
    sal_mac_addr_t mac;
} _soc_tr_l2e_ppa_info_t;

typedef struct _soc_tr_l2e_ppa_vlan_s {
    int         vlan_min[VLAN_ID_MAX + 1];
    int         vlan_max[VLAN_ID_MAX + 1];
} _soc_tr_l2e_ppa_vlan_t;

/* defines for bits in the ppa_info data word */
#define _SOC_TR_L2E_LIMIT_COUNTED 0x80000000
#define _SOC_TR_L2E_VALID         0x40000000
#define _SOC_TR_L2E_STATIC        0x20000000
#define _SOC_TR_L2E_T             0x10000000

#define _SOC_TR_L2E_VLAN_MASK   0xfff
#define _SOC_TR_L2E_VLAN_SHIFT  16
#define _SOC_TR_L2E_TRUNK_MASK  0xffff
#define _SOC_TR_L2E_TRUNK_SHIFT 0
#define _SOC_TR_L2E_MOD_MASK    0xff
#define _SOC_TR_L2E_MOD_SHIFT   8
#define _SOC_TR_L2E_PORT_MASK   0xff
#define _SOC_TR_L2E_PORT_SHIFT  0

extern int soc_triumph_misc_init(int);
extern int soc_triumph_mmu_init(int);
extern int soc_triumph_age_timer_get(int, int *, int *);
extern int soc_triumph_age_timer_max_get(int, int *);
extern int soc_triumph_age_timer_set(int, int, int);
extern void soc_triumph_parity_error(void *unit_vp, void *d1, void *d2,
                                     void *d3, void *d4);
extern void soc_triumph_esm_intr_status(void *unit_vp, void *d1, void *d2,
                                        void *d3, void *d4);
extern int soc_triumph_esm_init_read_config(int unit);
extern int soc_triumph_esm_init_set_tcam_freq(int unit, int freq);
extern int soc_triumph_esm_init_set_sram_freq(int unit, int freq);
extern int soc_triumph_esm_init_pvt_comp(int unit, int intf_num);
extern void soc_triumph_esm_init_mem_config(int unit);
extern int soc_triumph_esm_init(int unit);
extern int soc_triumph_ext_age_timer_set(int unit, int age_seconds,
                                         int enable);
extern int soc_triumph_tcam_access(int unit, int op_type, int num_inst,
                                   int num_pre_nop, int num_post_nop,
                                   uint32 *dbus, int *ibus);
extern int soc_triumph_tcam_search_bist(int unit, int part1, int part2,
                                        uint32 *data, int index1, int index2,
                                        int loop_count);
extern int soc_triumph_ext_sram_enable_set(int unit, int intf_num,
                                           int enable, int clear_status);
extern int soc_triumph_ext_sram_bist_setup(int unit, int intf_num,
                                           tr_ext_sram_bist_t *sram_bist);
extern int soc_triumph_ext_sram_op(int unit, int intf_num,
                                   tr_ext_sram_bist_t *sram_bist,
                                   tr_ext_sram_bist_t *opt_sram_bist);
extern int soc_triumph_l2x_to_ext_l2(int unit, l2x_entry_t *l2x_entry,
                                     ext_l2_entry_entry_t *ext_l2_entry);
extern int soc_triumph_ext_l2_to_l2x(int unit,
                                     ext_l2_entry_entry_t *ext_l2_entry,
                                     l2x_entry_t *l2x_entry);
extern int soc_triumph_ext_l2_entry_update(int unit, int index, void *entry);
extern int soc_triumph_learn_count_update(int unit,
                                          ext_l2_entry_entry_t *ext_l2_entry,
                                          int incl_sys_vlan, int diff);
extern int soc_triumph_xgport_mode_change(int unit, soc_port_t port,
                                          soc_mac_mode_t mode);
extern soc_functions_t soc_triumph_drv_funs;

#endif	/* !_SOC_TRIUMPH_H_ */
