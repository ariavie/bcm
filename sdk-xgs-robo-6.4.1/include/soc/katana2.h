/*
 * $Id: katana2.h,v 1.39 Broadcom SDK $
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
 * File:        katana2.h
 */

#ifndef _SOC_KATANA2_H_
#define _SOC_KATANA2_H_

#include <soc/drv.h>
#include <soc/mem.h>
#include <soc/error.h>

typedef struct kt2_pbmp_s {
    soc_pbmp_t *pbmp_gport_stack;
    soc_pbmp_t *pbmp_mxq;
    soc_pbmp_t *pbmp_mxq1g;
    soc_pbmp_t *pbmp_mxq2p5g;
    soc_pbmp_t *pbmp_mxq10g;
    soc_pbmp_t *pbmp_mxq13g;
    soc_pbmp_t *pbmp_mxq21g;
    soc_pbmp_t *pbmp_xport_xe;
    soc_pbmp_t *pbmp_valid;
    soc_pbmp_t *pbmp_pp;
    soc_pbmp_t *pbmp_linkphy;
}kt2_pbmp_t;

typedef enum bcmMxqConnection_s {
    bcmMqxConnectionUniCore=0, 
    bcmMqxConnectionWarpCore=1
}bcmMxqConnection_t;

typedef enum bcmMxqCorePortMode_s {
    bcmMxqCorePortModeSingle=0, 
    bcmMxqCorePortModeDual=1, 
    bcmMxqCorePortModeQuad=2 
}bcmMxqCorePortMode_t;

typedef enum bcmMxqPhyPortMode_s {
    bcmMxqPhyPortModeSingle=0, 
    bcmMxqPhyPortModeDual=1,  /* Suspicious */
    bcmMxqPhyPortModeQuad=2
}bcmMxqPhyPortMode_t;

enum kt2_l2_hash_key_type_e {
    /* WARNING: values given match hardware register; do not modify */
    KT2_L2_HASH_KEY_TYPE_BRIDGE = 0,
    KT2_L2_HASH_KEY_TYPE_SINGLE_CROSS_CONNECT = 1,
    KT2_L2_HASH_KEY_TYPE_DOUBLE_CROSS_CONNECT = 2,
    KT2_L2_HASH_KEY_TYPE_VFI = 3,
    KT2_L2_HASH_KEY_TYPE_BFD = 4,
    KT2_L2_HASH_KEY_TYPE_VIF = 5,
    KT2_L2_HASH_KEY_TYPE_PE_VID = 6,
    KT2_L2_HASH_KEY_TYPE_COUNT
};

enum kt2_l3_hash_key_type_e {
    /* WARNING: values given match hardware register; do not modify */
    KT2_L3_HASH_KEY_TYPE_V4UC = 0,
    KT2_L3_HASH_KEY_TYPE_V4MC = 1,
    KT2_L3_HASH_KEY_TYPE_V6UC = 2,
    KT2_L3_HASH_KEY_TYPE_V6MC = 3,
    KT2_L3_HASH_KEY_TYPE_LMEP = 4,
    KT2_L3_HASH_KEY_TYPE_RMEP = 5,
/*    KT2_L3_HASH_KEY_TYPE_TRILL = 6, UNSUPPORTED */
    KT2_L3_HASH_KEY_TYPE_COUNT
};

enum kt2_vlxlt_hash_key_type_e {
    /* WARNING: values given match hardware register; do not modify */
    KT2_VLXLT_HASH_KEY_TYPE_IVID_OVID = 0,
    KT2_VLXLT_HASH_KEY_TYPE_OTAG = 1,
    KT2_VLXLT_HASH_KEY_TYPE_ITAG = 2,
    KT2_VLXLT_HASH_KEY_TYPE_VLAN_MAC = 3,
    KT2_VLXLT_HASH_KEY_TYPE_OVID = 4,
    KT2_VLXLT_HASH_KEY_TYPE_IVID = 5,
    KT2_VLXLT_HASH_KEY_TYPE_PRI_CFI = 6,
    KT2_VLXLT_HASH_KEY_TYPE_HPAE = 7,
    KT2_VLXLT_HASH_KEY_TYPE_VIF = 8,
    KT2_VLXLT_HASH_KEY_TYPE_VIF_VLAN = 9,
    KT2_VLXLT_HASH_KEY_TYPE_VIF_CVLAN = 10,
    KT2_VLXLT_HASH_KEY_TYPE_VIF_OTAG = 11,
    KT2_VLXLT_HASH_KEY_TYPE_VIF_ITAG = 12,
    KT2_VLXLT_HASH_KEY_TYPE_LLTAG_VID = 13,
    KT2_VLXLT_HASH_KEY_TYPE_LLVID_IVID = 14,
    KT2_VLXLT_HASH_KEY_TYPE_LLVID_OVID = 15,
    KT2_VLXLT_HASH_KEY_TYPE_COUNT
};

enum kt2_evlxlt_hash_key_type_e {
    /* WARNING: values given match hardware register; do not modify */
    KT2_EVLXLT_HASH_KEY_TYPE_VLAN_XLATE = 0,
    KT2_EVLXLT_HASH_KEY_TYPE_VLAN_XLATE_DVP = 1,
    KT2_EVLXLT_HASH_KEY_TYPE_ISID_XLATE = 3,
    KT2_EVLXLT_HASH_KEY_TYPE_ISID_DVP_XLATE = 4,
    KT2_EVLXLT_HASH_KEY_TYPE_COUNT
};

enum kt2_mpls_hash_key_type_e {
    /* WARNING: values given match hardware register; do not modify */
    KT2_MPLS_HASH_KEY_TYPE_MPLS = 0,
    KT2_MPLS_HASH_KEY_TYPE_MIM_NVP = 1,
    KT2_MPLS_HASH_KEY_TYPE_MIM_ISID = 2,
    KT2_MPLS_HASH_KEY_TYPE_MIM_ISID_SVP = 3,
    KT2_MPLS_HASH_KEY_TYPE_COUNT
};

#define KT2_MAX_TDM_FREQUENCY         9 /* 80,110,155,185:80,185:90
                                           205:108 205:88 205:99, 95:16*/
#define KT2_MAX_SPEEDS                7 /* 1G,2G,2.5G,10G,13G,20G,21G */

#define KT2_MAX_MXQBLOCKS             11
#define KT2_MAX_MXQPORTS_PER_BLOCK    4
#define KT2_LPBK_PORT                 41
#define KT2_CMIC_PORT                 0
#define KT2_OLP_PORT                  40
#define KT2_IDLE1                     62 /* Consecutive IDLE slots */
#define KT2_IDLE                      63
/* 0:CMIC Port  1-40:Physical Ports  41:Loopback Port */
#define KT2_MAX_LOGICAL_PORTS         42
#define KT2_MAX_PHYSICAL_PORTS        40


#define KT2_MIN_SUBPORT_INDEX                 42
#define KT2_MAX_SUBPORT_INDEX                 169
#define KT2_MAX_SUBPORTS                      128
#define KT2_MAX_LINKPHY_SUBPORTS_PER_PORT     64
typedef struct tdm_cycles_info_s {
        uint32 worse_tdm_slot_spacing;
        uint32 optimal_tdm_slot_spacing;
        uint32 min_tdm_cycles;
}tdm_cycles_info_t;

typedef struct  kt2_tdm_pos_info_s {
        uint16 total_slots;
        int16  pos[24];
} kt2_tdm_pos_info_t;

typedef struct bcm56450_tdm_info_s {
        uint8  tdm_freq;
        uint8  tdm_size;
        /* Display purpose only */
        uint8  row;
        uint8  col;
}bcm56450_tdm_info_t;

typedef uint32 mxqspeeds_t[KT2_MAX_MXQBLOCKS][KT2_MAX_MXQPORTS_PER_BLOCK];
typedef uint32 kt2_mxqblock_ports_t[KT2_MAX_MXQBLOCKS]
                                   [KT2_MAX_MXQPORTS_PER_BLOCK];

typedef uint32 kt2_speed_t[KT2_MAX_PHYSICAL_PORTS];
typedef uint32 kt2_port_to_mxqblock_t[KT2_MAX_PHYSICAL_PORTS];
typedef uint32 kt2_port_to_mxqblock_subports_t[KT2_MAX_PHYSICAL_PORTS];


extern uint32 kt2_current_tdm[256];
extern uint32 kt2_current_tdm_size;

extern kt2_port_to_mxqblock_subports_t 
       *kt2_port_to_mxqblock_subports[SOC_MAX_NUM_DEVICES];
extern kt2_tdm_pos_info_t kt2_tdm_pos_info[KT2_MAX_MXQBLOCKS];
extern soc_error_t soc_katana2_reconfigure_tdm(int unit,uint32 new_tdm_size,
                                               uint32 *new_tdm);

extern kt2_mxqblock_ports_t *kt2_mxqblock_ports[SOC_MAX_NUM_DEVICES];
extern mxqspeeds_t *mxqspeeds[SOC_MAX_NUM_DEVICES];
extern uint8 kt2_tdm_update_flag;
extern tdm_cycles_info_t kt2_current_tdm_cycles_info[KT2_MAX_SPEEDS];



extern int soc_kt2_vlan_xlate_base_entry_to_key(int unit, void *entry, uint8 *key);
extern int soc_kt2_l2x_base_entry_to_key(int unit, uint32 *entry, uint8 *key);
extern int soc_kt2_l3x_base_entry_to_key(int unit, uint32 *entry, uint8 *key);
extern int soc_kt2_egr_vlan_xlate_base_entry_to_key(int unit, void *entry, uint8 *key);
extern int soc_kt2_mpls_base_entry_to_key(int unit, void *entry, uint8 *key);
extern uint32 soc_kt2_vlan_xlate_hash(int unit, int hash_sel, int key_nbits, void *base_entry, uint8 *key);
extern uint32 soc_kt2_egr_vlan_xlate_hash(int unit, int hash_sel, int key_nbits, void *base_entry, uint8 *key);

extern soc_functions_t soc_katana2_drv_funs;

extern int soc_katana2_pipe_mem_clear(int unit);
extern int soc_katana2_linkphy_mem_clear(int unit);


extern int soc_kt2_ser_mem_clear(int unit, soc_mem_t mem);
extern void soc_kt2_ser_fail(int unit);

extern int soc_kt2_mem_config(int unit, int dev_id);
extern int soc_kt2_temperature_monitor_get(int unit,
          int temperature_max,
          soc_switch_temperature_monitor_t *temperature_array,
          int *temperature_count);
extern int soc_kt2_show_ring_osc(int unit);
extern int soc_kt2_show_material_process(int unit);
extern int soc_kt2_show_voltage(int unit);

extern int soc_kt2_linkphy_port_reg_blk_idx_get(
    int unit, int port, int blktype, int *block, int *index);
extern int soc_kt2_linkphy_port_blk_idx_get(
    int unit, int port, int *block, int *index);
extern int soc_kt2_linkphy_get_portid(int unit, int block, int index);
extern void soc_katana2_pbmp_init(int unit, kt2_pbmp_t kt2_pbmp);
extern void soc_katana2_subport_init(int unit);
extern soc_error_t 
       soc_katana2_tdm_feasibility_check(int unit, soc_port_t port, int speed);
extern soc_error_t
       soc_katana2_update_tdm(int unit, soc_port_t port, int speed);
extern soc_error_t 
       soc_katana2_get_core_port_mode( 
       int unit,soc_port_t port,bcmMxqCorePortMode_t *mode);
extern soc_error_t 
       soc_katana2_get_phy_port_mode(
       int unit, soc_port_t port,int speed, bcmMxqPhyPortMode_t *mode);
extern soc_error_t soc_katana2_get_port_mxqblock(
       int unit, soc_port_t port,uint8 *mxqblock);
extern soc_error_t soc_katana2_get_phy_connection_mode(
       int unit,soc_port_t port,int mxqblock, bcmMxqConnection_t *connection);
extern soc_error_t soc_katana2_port_lanes_get(
       int unit, soc_port_t port, int *lanes);
extern soc_error_t soc_katana2_port_lanes_set(
       int unit, soc_port_t port, int lanes);
extern void _katana2_phy_addr_default(
       int unit, int port, uint16 *phy_addr, uint16 *phy_addr_int);
extern soc_error_t soc_katana2_port_enable_set(
       int unit, soc_port_t port, int enable);
extern void soc_katana2_mxqblock_reset(int unit, uint8 mxqblock,int active_low);
extern int _soc_katana2_mmu_reconfigure(int unit);
extern int soc_katana2_num_cosq_init(int unit);
extern void soc_kt2_blk_counter_config(int unit);
extern soc_error_t katana2_get_wc_phy_info(int unit,
                                           soc_port_t port,
                                           uint8 *lane_num,
                                           uint8 *phy_mode,
                                           uint8 *chip_num);

/* KT2 OAM */
typedef int (*soc_kt2_oam_handler_t)(int unit, soc_field_t fault_field);
typedef int (*soc_kt2_oam_ser_handler_t)(int unit, soc_mem_t mem, int index);
extern void soc_kt2_oam_handler_register(int unit, soc_kt2_oam_handler_t handler);

extern void soc_kt2_parity_error(void *unit_vp, void *d1, void *d2,
                         void *d3, void *d4);
extern void soc_kt2_oam_ser_handler_register(int unit, 
                                soc_kt2_oam_ser_handler_t handler);
extern int soc_kt2_oam_ser_process(int unit, soc_mem_t mem, int index);

/* KT2 cosq soc functions */
extern int soc_kt2_cosq_stream_mapping_set(int unit);
extern int soc_kt2_cosq_port_coe_linkphy_status_set(int unit, soc_port_t port,
                                                    int enable);
extern int soc_kt2_cosq_s1_range_set(int unit, soc_port_t port,
                                     int start_s1, int end_s1, int type);
extern int soc_kt2_cosq_repl_map_set(int unit, soc_port_t port,
                                     int start, int end, int enable);
extern int soc_kt2_cosq_s0_sched_set(int unit, int hw_index, int mode);

/**********************************************************************
 * MMU, COSQ related prototypes.
 **********************************************************************/
typedef enum {
    SOC_KT2_SCHED_MODE_UNKNOWN = 0,
    SOC_KT2_SCHED_MODE_STRICT,
    SOC_KT2_SCHED_MODE_WRR,
    SOC_KT2_SCHED_MODE_WDRR
} soc_kt2_sched_mode_e;

typedef enum {
    SOC_KT2_NODE_LVL_ROOT = 0,
    SOC_KT2_NODE_LVL_L0,
    SOC_KT2_NODE_LVL_L1,
    SOC_KT2_NODE_LVL_L2,
    SOC_KT2_NODE_LVL_MAX
} soc_kt2_node_lvl_e;

#define _SOC_KT2_NODE_CONFIG_MEM(n)                                 \
    (((n)==SOC_KT2_NODE_LVL_ROOT) ? LLS_PORT_CONFIGm :              \
     (((n) == SOC_KT2_NODE_LVL_L0) ? LLS_L0_CONFIGm :               \
      ((n) == SOC_KT2_NODE_LVL_L1 ? LLS_L1_CONFIGm :  -1  )))

#define _SOC_KT2_NODE_PARENT_MEM(n)                                 \
    (((n)==SOC_KT2_NODE_LVL_L2) ? LLS_L2_PARENTm :                  \
     (((n) == SOC_KT2_NODE_LVL_L1) ? LLS_L1_PARENTm :               \
      (((n) == SOC_KT2_NODE_LVL_L0) ? LLS_L0_PARENTm :  -1  )))

#define _SOC_KT2_NODE_WIEGHT_MEM(n)                                 \
    (((n)==SOC_KT2_NODE_LVL_L0) ? LLS_L0_CHILD_WEIGHT_CFG_CNTm  : \
     (((n) == SOC_KT2_NODE_LVL_L1) ? LLS_L1_CHILD_WEIGHT_CFG_CNTm : \
      (((n) == SOC_KT2_NODE_LVL_L2) ?  LLS_L2_CHILD_WEIGHT_CFG_CNTm : -1 )))

extern int soc_kt2_dump_port_lls(int unit, int port);
extern int _soc_katana2_get_cfg_num(int unit, int *new_cfg_num);
extern int _soc_katana2_flexio_scache_allocate(int unit);
extern int _soc_katana2_flexio_scache_retrieve(int unit);
extern int _soc_katana2_flexio_scache_sync(int unit);
extern int soc_kt2_l3_defip_index_map(int unit, soc_mem_t mem, int index);
extern int soc_kt2_l3_defip_index_remap(int unit, soc_mem_t mem, int index);
extern int soc_kt2_l3_defip_mem_index_get(int unit, int pindex, soc_mem_t *mem);

#if defined(SER_TR_TEST_SUPPORT)
extern int soc_katana2_ser_inject_error(int unit, uint32 flags, soc_mem_t mem, int pipe_target, int block, int index);
extern int _soc_katana2_mem_nack_error_test(int unit, _soc_ser_test_t test_type, int *test_errors);
extern int soc_katana2_ser_mem_test(int unit, soc_mem_t mem, _soc_ser_test_t test_type, int cmd);
extern int soc_katana2_ser_test(int unit, _soc_ser_test_t test_type);
#endif
extern int soc_kt2_lls_bmap_alloc(int unit);
#endif    /* !_SOC_KATANA2_H_ */
