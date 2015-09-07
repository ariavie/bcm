/*----------------------------------------------------------------------
 * $Id: tefmod.h,
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
 *  Broadcom Corporation
 *  Proprietary and Confidential information
 *  All rights reserved
 *  This source file is the property of Broadcom Corporation, and
 *  may not be copied or distributed in any isomorphic form without the
 *  prior written consent of Broadcom Corporation.
 *----------------------------------------------------------------------
 *  Description: define enumerators  
 *----------------------------------------------------------------------*/
/*
 * $Id: 8831d06f1510a495f84b71ce18aac86e5ab27906 $ 
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

#ifndef _tefmod_H_
#define _tefmod_H_

#ifndef _DV_TB_
 #define _SDK_TEFMOD_ 1 
#endif

#ifdef _DV_TB_
#ifdef LINUX
#include <stdint.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "errno.h"
#endif

#ifdef _SDK_TEFMOD_
#include <phymod/phymod.h>
#include <phymod/phymod_debug.h>
#endif

#ifdef _SDK_TEFMOD_
#define PHYMOD_ST const phymod_access_t
#else
#define PHYMOD_ST tefmod_st
#endif

#define TEFMOD_FEC_NOT_SUPRTD         0
#define TEFMOD_FEC_SUPRTD_NOT_REQSTD  1
#define TEFMOD_FEC_SUPRTD_REQSTD      3

/* So far 4 bits debug mask are used by TEFMOD */
#define TEFMOD_DBG_REGACC     (1L << 3) /* Print all register accesses */ 
#define TEFMOD_DBG_FUNCVALOUT (1L << 2) /* All values returned by Tier1*/
#define TEFMOD_DBG_FUNCVALIN  (1L << 1) /* All values pumped into Tier1*/
#define TEFMOD_DBG_FUNC       (1L << 0) /* Every time we enter a  Tier1*/

typedef enum {
    TEFMOD_AN_MODE_CL73 = 0,
    TEFMOD_AN_MODE_CL73BAM,
    TEFMOD_AN_MODE_HPAM,       
    TEFMOD_AN_MODE_NONE,
    TEFMOD_AN_MODE_TYPE_COUNT
}tefmod_an_mode_type_t;



typedef enum {
    TEFMOD_AN_SET_RF_DISABLE = 0,
    TEFMOD_AN_SET_SGMII_SPEED,
    TEFMOD_AN_SET_SGMII_MASTER,
    TEFMOD_AN_SET_HG_MODE,
    TEFMOD_AN_SET_FEC_MODE,
    TEFMOD_AN_SET_CL72_MODE,
    TEFMOD_AN_SET_CL37_ATTR,
    TEFMOD_AN_SET_CL48_SYNC,
    TEFMOD_AN_SET_LK_FAIL_INHIBIT_TIMER_NO_CL72,
    TEFMOD_AN_SET_CL73_FEC_OFF,
    TEFMOD_AN_SET_CL73_FEC_ON,
    TEFMOD_AN_SET_SPEED_PAUSE,
    TEFMOD_AN_SET_TYPE_COUNT
}tefmod_an_set_type_t;

typedef union {
    int tefmod_an_set_rf_disable;
    int tefmod_an_set_sgmii_speed;
    int tefmod_an_set_sgmii_master;
    tefmod_an_set_type_t tefmod_an_set_cl37_attr;
}tefmod_an_set_value_t;

typedef enum {
    TEFMOD_CL73_10GBASE_KR1 = 0,  
    TEFMOD_CL73_40GBASE_KR4,  
    TEFMOD_CL73_40GBASE_CR4,  
    TEFMOD_CL73_100GBASE_KR4,
    TEFMOD_CL73_100GBASE_CR4,
    TEFMOD_CL73_1GBASE_KX,
    TEFMOD_CL73_SPEED_COUNT
}tefmod_cl73_speed_t;

typedef enum {
    TEFMOD_CL73_BAM_20GBASE_KR2 = 0,  
    TEFMOD_CL73_BAM_20GBASE_CR2,  
    TEFMOD_CL73_BAM_40GBASE_KR2,  
    TEFMOD_CL73_BAM_40GBASE_CR2,
    TEFMOD_CL73_BAM_RESERVED_1,
    TEFMOD_CL73_BAM_RESERVED_2,
    TEFMOD_CL73_BAM_50GBASE_KR2,  
    TEFMOD_CL73_BAM_50GBASE_CR2,
    TEFMOD_CL73_BAM_50GBASE_KR4,
    TEFMOD_CL73_BAM_50GBASE_CR4,
    TEFMOD_CL73_BAM_SPEED_COUNT
}tefmod_cl73_bam_speed_t;

typedef enum {
    TEFMOD_CL73_BAM_20GBASE_KR1 = 1,  
    TEFMOD_CL73_BAM_20GBASE_CR1,  
    TEFMOD_CL73_BAM_25GBASE_KR1,  
    TEFMOD_CL73_BAM_25GBASE_CR1,
    TEFMOD_CL73_BAM_SPEED1_COUNT
}tefmod_cl73_bam_speed1_t;

typedef enum {
    TEFMOD_NO_PAUSE = 0,  
    TEFMOD_ASYM_PAUSE,
    TEFMOD_SYMM_PAUSE,
    TEFMOD_ASYM_SYMM_PAUSE,  
    TEFMOD_AN_PAUSE_COUNT
}tefmod_an_pause_t;

typedef enum {
  OVERRIDE_CLEAR                  =  0x0000,
  OVERRIDE_NUM_LANES              =  0x0001,
  OVERRIDE_OS_MODE                =  0x0002,
  OVERRIDE_T_FIFO_MODE            =  0x0004,
  OVERRIDE_T_ENC_MODE             =  0x0008,
  OVERRIDE_T_HG2_ENABLE           =  0x0010,
  OVERRIDE_T_PMA_BTMX_MODE_OEN    =  0x0020,
  OVERRIDE_SCR_MODE               =  0x0040,
  OVERRIDE_DESCR_MODE_OEN         =  0x0100,
  OVERRIDE_DEC_TL_MODE            =  0x0200,
  OVERRIDE_DESKEW_MODE            =  0x0400,
  OVERRIDE_DEC_FSM_MODE           =  0x0800,
  OVERRIDE_R_HG2_ENABLE           =  0x8001,
  OVERRIDE_BS_SM_SYNC_MODE_OEN    =  0x8002,
  OVERRIDE_BS_SYNC_EN_OEN         =  0x8004,
  OVERRIDE_BS_DIST_MODE_OEN       =  0x8008,
  OVERRIDE_BS_BTMX_MODE_OEN       =  0x8010,
  OVERRIDE_CL72_EN                =  0x8020,
  OVERRIDE_CLOCKCNT0              =  0xf040,
  OVERRIDE_CLOCKCNT1              =  0x8080,
  OVERRIDE_LOOPCNT0               =  0x8100,
  OVERRIDE_LOOPCNT1               =  0x8200,
  OVERRIDE_MAC_CREDITGENCNT       =  0x8400,
  OVERRIDE_NUM_LANES_DIS          =  0xff01,
  OVERRIDE_OS_MODE_DIS            =  0xff02,
  OVERRIDE_T_FIFO_MODE_DIS        =  0xff03,
  OVERRIDE_T_ENC_MODE_DIS         =  0xff04,
  OVERRIDE_T_HG2_ENABLE_DIS       =  0xff05,
  OVERRIDE_T_PMA_BTMX_MODE_OEN_DIS=  0xff06,
  OVERRIDE_SCR_MODE_DIS           =  0xff07,
  OVERRIDE_DESCR_MODE_OEN_DIS     =  0xff08,
  OVERRIDE_DEC_TL_MODE_DIS        =  0xff09,
  OVERRIDE_DESKEW_MODE_DIS        =  0xff0a,
  OVERRIDE_DEC_FSM_MODE_DIS       =  0xff0b,
  OVERRIDE_R_HG2_ENABLE_DIS       =  0xff0c,
  OVERRIDE_BS_SM_SYNC_MODE_OEN_DIS=  0xff0d,
  OVERRIDE_BS_SYNC_EN_OEN_DIS     =  0xff0e,
  OVERRIDE_BS_DIST_MODE_OEN_DIS   =  0xff0f,
  OVERRIDE_BS_BTMX_MODE_OEN_DIS   =  0xff10,
  OVERRIDE_CL72_EN_DIS            =  0xff11,
  OVERRIDE_CLOCKCNT0_DIS          =  0xff12,
  OVERRIDE_CLOCKCNT1_DIS          =  0xff13,
  OVERRIDE_LOOPCNT0_DIS           =  0xff14,
  OVERRIDE_LOOPCNT1_DIS           =  0xff15,
  OVERRIDE_MAC_CREDITGENCNT_DIS   =  0xff16
} override_type_t;


/**
\struct tefmod_an_adv_ability_s
TBD
*/

typedef struct tefmod_an_adv_ability_s{
  tefmod_cl73_speed_t an_base_speed; 
  tefmod_cl73_bam_speed_t an_bam_speed; 
  tefmod_cl73_bam_speed1_t an_bam_speed1; 
  uint16_t an_nxt_page; 
  tefmod_an_pause_t an_pause; 
  uint16_t an_fec; 
  uint16_t an_cl72;
  uint16_t an_hg2; 
}tefmod_an_adv_ability_t;

typedef struct tefmod_an_ability_s{
  tefmod_an_adv_ability_t cl73_adv; /*includes cl73 and cl73-bam related */
}tefmod_an_ability_t;


typedef struct tefmod_an_control_s {
  tefmod_an_type_t an_type; 
  uint16_t num_lane_adv; 
  uint16_t enable;
  uint16_t pd_kx_en;
  an_property_enable  an_property_type;
} tefmod_an_control_t;

/**
\struct tefmod_an_init_s

This embodies all parameters required to program autonegotiation features in the
PHY for both CL37 and CL73 type autonegotiation and the BAM variants.
For details of individual fields, please refer to microarchitectural doc.
*/
typedef struct tefmod_an_init_s{
  uint16_t  an_fail_cnt;
  uint16_t  linkfailtimer_dis; 
  uint16_t  linkfailtimerqua_en; 
  uint16_t  an_good_check_trap; 
  uint16_t  an_good_trap; 
  uint16_t  bam73_adv_oui;  
  uint16_t  bam73_det_oui;  
  uint16_t  hpam_adv_oui;  
  uint16_t  hpam_det_oui;  
  uint16_t  disable_rf_report; 
  uint16_t  cl73_remote_fault; 
  uint16_t  cl73_nonce_match_over;
  uint16_t  cl73_nonce_match_val;
  uint16_t  cl73_transmit_nonce; 
  uint16_t  base_selector;
} tefmod_an_init_t;


/**
\struct tefmod_an_timers_s
TBD
*/
typedef struct tefmod_an_timers_s{
  uint16_t  value;
} tefmod_an_timers_t;


#ifdef CHECKIFNEEDED

extern int get_mapped_speed(tefmod_spd_intfc_type spd_intf, int *speed);
extern int get_actual_speed(int speed_id, int *speed);

#endif
extern int phymod_debug_check(uint32_t flags, PHYMOD_ST *pc);

extern int configure_st_entry(int st_entry_num, int st_entry_speed, PHYMOD_ST* pc);

extern int tefmod_pmd_osmode_set(PHYMOD_ST* pc, tefmod_spd_intfc_type_t spd_intf, int os_mode);
extern int tefmod_pmd_x4_reset(PHYMOD_ST* pc);
extern int tefmod_tx_loopback_get(PHYMOD_ST* pc, uint32_t *enable);
extern int tefmod_firmware_set(PHYMOD_ST* pc);
extern int tefmod_tx_lane_disable (PHYMOD_ST* pc, int tx);
extern int tefmod_init_pcs_ilkn(PHYMOD_ST* pc);
extern int tefmod_pmd_reset_bypass (PHYMOD_ST* pc, int pmd_reset_control);
extern int tefmod_afe_speed_up_dsc_vga(PHYMOD_ST* pc);
extern int tefmod_get_plldiv(PHYMOD_ST* pc, uint32_t *plldiv_r_val);
extern int tefmod_update_port_mode_select(PHYMOD_ST* pc, tefmod_port_type_t port_type, int master_port, int tsc_clk_freq_pll_by_48, int pll_reset_en);
extern int tefmod_set_port_mode(PHYMOD_ST* pc, int refclk, int plldiv, tefmod_port_type_t port_type, int master_port, int tsc_clk_freq_pll_by_48, int pll_reset_en);
extern int tefmod_tx_loopback_control(PHYMOD_ST* pc, int pcs_gloop_en, int lane);
extern int tefmod_tx_pmd_loopback_control(PHYMOD_ST* pc, int pmd_gloop_en);
extern int tefmod_rx_loopback_control(PHYMOD_ST* pc, int pcs_rloop_en);
extern int tefmod_rx_pmd_loopback_control(PHYMOD_ST* pc, int pcs_rloop_en, int pmd_rloop_en, int lane);
extern int tefmod_credit_set(PHYMOD_ST* pc);
extern int tefmod_encode_set(PHYMOD_ST* pc, int per_lane_control, tefmod_spd_intfc_type_t spd_intf, int tx_am_timer_init_val);
extern int tefmod_lf_rf_control(PHYMOD_ST* pc);
extern int tefmod_decode_set(PHYMOD_ST* pc);
extern int tefmod_trigger_speed_change(PHYMOD_ST* pc);
extern int tefmod_set_override_0(PHYMOD_ST* pc, int per_field_override_en);
extern int tefmod_set_override_1(PHYMOD_ST* pc, int per_lane_control, int per_field_override_en);
extern int tefmod_credit_control(PHYMOD_ST* pc, int per_lane_control);
extern int tefmod_tx_lane_control(PHYMOD_ST* pc, int en, tefmod_tx_disable_enum_t dis);
extern int tefmod_rx_lane_control(PHYMOD_ST* pc, int accData, int per_lane_control);
extern int tefmod_bypass_sc(PHYMOD_ST* pc);
extern int tefmod_revid_read(PHYMOD_ST* pc, uint32_t *revIdV);
extern int tefmod_clause72_control(PHYMOD_ST* pc, int per_lane_control);
extern int tefmod_prbs_check(PHYMOD_ST* pc, int accData);
extern int tefmod_prbs_mode(PHYMOD_ST* pc, int per_lane_control);
extern int tefmod_prbs_control(PHYMOD_ST* pc, int per_lane_control);
extern int tefmod_cjpat_crpat_control(PHYMOD_ST* pc);
extern int tefmod_cjpat_crpat_check(PHYMOD_ST* pc);
extern int tefmod_tx_bert_control(PHYMOD_ST* pc);
extern int tefmod_FEC_control(PHYMOD_ST* pc, int fec_en, int fec_dis, int cl74or91);
extern int tefmod_power_control(PHYMOD_ST* pc, int tx, int rx);
extern int tefmod_duplex_control(PHYMOD_ST* pc);
extern int tefmod_refclk_set(PHYMOD_ST* pc, int ref_clk);
extern int tefmod_disable_pcs_falcon(PHYMOD_ST* pc);
extern int tefmod_pmd_addr_lane_swap(PHYMOD_ST *pc, int per_lane_control);
extern int tefmod_pmd_lane_swap_tx(PHYMOD_ST *pc, int per_lane_control);
extern int tefmod_pmd_reset_remove(PHYMOD_ST *pc, int pmd_touched);
extern int tefmod_pcs_lane_swap(PHYMOD_ST *pc, int per_lane_control);
extern int tefmod_init_pcs_falcon(PHYMOD_ST* pc);
extern int tefmod_init_pmd_falcon(PHYMOD_ST* pc);
extern int tefmod_set_sc_speed(PHYMOD_ST* pc);
extern int tefmod_poll_for_sc_done(PHYMOD_ST* pc, int mapped_speed);
extern int tefmod_check_status(PHYMOD_ST* pc);
extern int tefmod_set_port_mode_sel(PHYMOD_ST* pc, int tsc_touched, tefmod_port_type_t port_type);
extern int tefmod_init_pcs_fs(PHYMOD_ST* pc);
extern int tefmod_init_pcs_an(PHYMOD_ST* pc);
extern int tefmod_set_an_override(PHYMOD_ST* pc);

extern int tefmod_get_mapped_speed(tefmod_spd_intfc_type_t spd_intf, int *speed);

#ifdef _DV_TB_
extern int tefmod_autoneg_set(PHYMOD_ST* pc);
extern int tefmod_autoneg_get(PHYMOD_ST* pc);
extern int tefmod_autoneg_control(PHYMOD_ST* pc);
extern int tefmod_set_spd_intf(PHYMOD_ST* pc);
extern int tefmod_set_an_port_mode(PHYMOD_ST* pc);
#endif /* _DV_TB_ */
#ifdef _SDK_TEFMOD_
int tefmod_set_an_port_mode(PHYMOD_ST* pc, int num_of_lanes, int starting_lane, int single_port);
extern int tefmod_set_spd_intf(PHYMOD_ST *pc, tefmod_spd_intfc_type_t spd_intf);
extern int tefmod_autoneg_set_init(PHYMOD_ST* pc, tefmod_an_init_t *an_init_st);
extern int tefmod_autoneg_timer_init(PHYMOD_ST* pc);
extern int tefmod_autoneg_control(PHYMOD_ST* pc, tefmod_an_control_t *an_control);
extern int tefmod_autoneg_set(PHYMOD_ST* pc, tefmod_an_adv_ability_t *an_ability_st);
extern int tefmod_diag(PHYMOD_ST *ws, tefmod_diag_type_t diag_type);

#endif /* _SDK_TEFMOD_ */

extern int tefmod_pll_lock_get(PHYMOD_ST* pc, int* lockStatus);
extern int tefmod_pmd_lock_get(PHYMOD_ST* pc, uint32_t* lockStatus);
extern int tefmod_set_pll_mode(PHYMOD_ST* pc, int pmd_touched, tefmod_spd_intfc_type_t spd_intf, int pll_mode);           /* SET_PLL_MODE */
extern int tefmod_speed_id_get(PHYMOD_ST* pc, int *speed_id);
extern int tefmod_pmd_reset_seq(PHYMOD_ST* pc, int pmd_touched); /* PMD_RESET_SEQ */
extern int tefmod_master_port_num_set( PHYMOD_ST *pc,  int port_num);
extern int tefmod_update_port_mode( PHYMOD_ST *pa, int *pll_restart);
extern int tefmod_pll_reset_enable_set (PHYMOD_ST *pa , int enable);
extern int tefmod_get_pcs_link_status(PHYMOD_ST* pc, uint32_t *link);
extern int tefmod_disable_get(PHYMOD_ST* pc, uint32_t* enable);
extern int tefmod_disable_set(PHYMOD_ST* pc);
extern int tefmod_plldiv_lkup_get(PHYMOD_ST* pc, tefmod_spd_intfc_type_t spd_intf, uint32_t *plldiv);
extern int tefmod_osmode_lkup_get(PHYMOD_ST* pc, tefmod_spd_intfc_type_t spd_intf, uint32_t *osmode);
extern int tefmod_pcs_lane_swap_get ( PHYMOD_ST *pc,  uint32_t *tx_rx_swap);
extern int tefmod_pmd_lane_swap_tx_get ( PHYMOD_ST *pc, uint32_t *tx_lane_map);
extern int tefmod_rx_lane_control_set(PHYMOD_ST* pc, int enable);         /* RX_LANE_CONTROL */
extern int tefmod_tx_rx_polarity_get ( PHYMOD_ST *pc, uint32_t* tx_polarity, uint32_t* rx_polarity);
extern int tefmod_tx_rx_polarity_set ( PHYMOD_ST *pc, uint32_t tx_polarity, uint32_t rx_polarity);

#endif  /*  _tefmod_H_ */

