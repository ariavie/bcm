/*
 * $Id: greyhound.h,v 1.1.8.8 Broadcom SDK $
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
 * File:        greyhound.h
 */

#ifndef _SOC_GREYHOUND_H_
#define _SOC_GREYHOUND_H_

#include <soc/drv.h>
#include <soc/mem.h>
#include <shared/sram.h>


enum gh_l2_hash_key_type_e {
    /* WARNING: values given match hardware register; do not modify */
    GH_L2_HASH_KEY_TYPE_BRIDGE = 0,
    GH_L2_HASH_KEY_TYPE_SINGLE_CROSS_CONNECT = 1,
    GH_L2_HASH_KEY_TYPE_DOUBLE_CROSS_CONNECT = 2,    
    GH_L2_HASH_KEY_TYPE_VIF = 5,
    GH_L2_HASH_KEY_TYPE_PE_VID = 6,
    GH_L2_HASH_KEY_TYPE_COUNT
};

enum soc_gh_port_mode_e {
    /* WARNING: values given match hardware register; do not modify */
    SOC_GH_PORT_MODE_QUAD = 0,
    SOC_GH_PORT_MODE_TRI_012 = 1,
    SOC_GH_PORT_MODE_TRI_023 = 2,
    SOC_GH_PORT_MODE_DUAL = 3,
    SOC_GH_PORT_MODE_SINGLE = 4
};

/* QSGMII MDIO address : MDIO address for por2-port17
 *  - this address is just a shadow address for QSGMII PCS register access 
 *    but will be really used for PMD register access.
 */
#define GH_QSGMII_MDIO_ADDR     0x81

#define GH_PORT_IS_QSGMII(_unit, _port) \
        (IS_GE_PORT((_unit), (_port)) && !IS_XL_PORT((_unit), (_port)))

extern int soc_greyhound_l2x_base_entry_to_key(int unit, uint32 *entry, uint8 *key);

typedef int (*soc_greyhound_oam_handler_t)(int unit, 
    soc_field_t fault_field);
typedef int (*soc_greyhound_oam_ser_handler_t)(int unit, 
    soc_mem_t mem, int index);

extern int soc_greyhound_port_config_init(int unit, uint16 dev_id);
extern int soc_greyhound_mem_config(int unit, int dev_id);
extern void soc_greyhound_oam_handler_register(int unit, 
                                         soc_greyhound_oam_handler_t handler);
extern void soc_greyhound_oam_ser_handler_register(int unit, 
                               soc_greyhound_oam_ser_handler_t handler);
extern int soc_greyhound_oam_ser_process(int unit, 
                                soc_mem_t mem, int index);
extern void soc_greyhound_parity_error(void *unit_vp, void *d1, void *d2,
                                     void *d3, void *d4);
extern void soc_greyhound_ser_fail(int unit);
extern int soc_greyhound_ser_mem_clear(int unit, soc_mem_t mem);

extern int _soc_greyhound_mem_parity_control(int unit, soc_mem_t mem,
                                            int copyno, int enable);
extern int soc_gh_l2_entry_limit_count_update(int unit);

#if defined(SER_TR_TEST_SUPPORT)
extern int soc_gh_ser_inject_error(int unit, uint32 flags, soc_mem_t mem,
                                    int pipeTarget, int block, int index);
extern int soc_gh_ser_mem_test(int unit, soc_mem_t mem, 
                                        _soc_ser_test_t test_type, int cmd);
extern int soc_gh_ser_test(int unit, _soc_ser_test_t test_type);
#endif

extern int soc_greyhound_port_lanes_set(int unit, soc_port_t port_base,
                                       int lanes, int *cur_lanes,
                                       int *phy_ports, int *phy_ports_len);
extern int soc_greyhound_port_lanes_get(int unit, soc_port_t port_base,
                                       int *cur_lanes);
extern int soc_greyhound_chip_reset(int unit);
extern int soc_greyhound_tsc_reset(int unit);

extern int soc_gh_temperature_monitor_get(int unit,
          int temperature_max,
          soc_switch_temperature_monitor_t *temperature_array,
          int *temperature_count);

extern soc_functions_t soc_greyhound_drv_funs;

extern int soc_gh_xq_mem(int unit, soc_port_t port, soc_mem_t *xq);

extern int soc_greyhound_pgw_reg_blk_index_get(int unit, 
    soc_reg_t reg, soc_port_t port, 
    pbmp_t *bm, int *block, int *index, int invalid_blk_check);
extern int _soc_greyhound_features(int unit, soc_feature_t feature);

extern int soc_greyhound_pgw_encap_field_modify(int unit, 
                                soc_port_t lport, 
                                soc_field_t field, 
                                uint32 val);
extern int soc_greyhound_pgw_encap_field_get(int unit, 
                                soc_port_t lport, 
                                soc_field_t field, 
                                uint32 *val);
extern void soc_gh_xport_type_update(int unit, soc_port_t port, int hg_mode);

extern int _soc_gh_sbus_tsc_block(int unit, 
            int phy_port, soc_mem_t wc_ucmem_data, int *block);
#endif /* !_SOC_GREYHOUND_H_ */
